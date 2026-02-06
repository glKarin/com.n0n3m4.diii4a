#include "sw_skycontroller.h"

CLASS_DECLARATION(idEntity, idSkyController)
END_CLASS

CLASS_DECLARATION(idStaticEntity, idDynamicSkyBrush)
END_CLASS

idSkyController::idSkyController()
{
	skyIndex = -1;
}

void idSkyController::Init(void)
{
	ParseSkies(&spawnArgs);
	FindSkyBrushes();
	FindLinkedLights(); // Needs to be called *after* we parse skies -- we need to tell the lights what to look for
	if (skies.Num())
		SetCurrentSky(0);
}

void idSkyController::ParseSkies(const idDict* spawnArgs)
{
	// Gets our list of skies from the 'skies' keyvalue, which includes a comma-separated list of sky shader names
	idStr skyString = spawnArgs->GetString("skies");
	if (!skyString.IsEmpty())
	{
		idStr nextSky = idStr();
		char nextChar;
		for (int j = 0; j < skyString.Length(); j++)
		{
			nextChar = skyString[j];

			if (nextChar == ',')
			{
				// Terminate the name here, try to find a light associated with it
				TryAddSky(nextSky);
				nextSky.Clear();
			}
			else
			{
				if (nextChar != ' ')
				{
					// Add the char to the current name
					nextSky.Append(nextChar);
				}

				if (j == skyString.Length() - 1)
				{
					// This is also the end of the string -- terminate the name and try to find a light
					TryAddSky(nextSky);
				}
			}
		}

		if (skies.Num() == 0)
			gameLocal.Warning(va("idSkyController: Controller does not have any skies associated with it. This is probably a mistake!"));
	}
	else
	{
		gameLocal.Warning("idSkyController: Could not find 'skies' keyvalue on info_sky_controller.");
	}
}

void idSkyController::FindLinkedLights(void)
{
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(idLight::Type))
		{
			idLight* light = static_cast<idLight*>(gameLocal.entities[i]);
			if (light->spawnArgs.GetBool("dynamicSky"))
			{
				light->ParseSkySettings(skies);
				linkedLights.Append(light);
			}
		}
	}
	if (!linkedLights.Num())
		gameLocal.Warning("idSkyController: No lights linked to controller.\nChanging the sky will not have any effect on the sunlight. This is possibly intended, but most likely not!");
}

void idSkyController::TryAddSky(idStr name)
{
	const idMaterial* sky = declManager->FindMaterial(va("textures/skies/%s", name.c_str()));

	if (!sky)
		gameLocal.Warning(va("idSkyController: Could not find any sky by the name of '%s'. Skipping...", name.c_str()));
	else
		skies.Append(sky);
}

void idSkyController::FindSkyBrushes(void)
{
	// SW: Surely there is a smarter way to do this
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		idEntity* ent = gameLocal.entities[i];
		if (ent && ent->IsType(idDynamicSkyBrush::Type))
		{
			skyBrushes.Append(static_cast<idDynamicSkyBrush*>(ent));
		}
	}
	if (skyBrushes.Num() == 0)
		gameLocal.Warning("idSkyController: Could not find any dynamic sky entities. Remember to bind each sky brush to a func_dynamic_sky!");
}

void idSkyController::SetCurrentSky(const idMaterial* newSky)
{
	int i;
	// For all targeted lights, we check for properties marked with a matching sky name,
	// and apply them (if relevant).
	// If no matching properties are found, we need to switch this light off -- it doesn't participate in this sky.
	for (i = 0; i < linkedLights.Num(); i++)
	{
		linkedLights[i]->ApplySkySetting(newSky);
	}

	// Swap the sky material out
	for (i = 0; i < skyBrushes.Num(); i++)
	{
		skyBrushes[i]->SetSkyMaterial(newSky);
	}
}

void idSkyController::SetCurrentSky(int skyIndex)
{
	if (skyIndex != this->skyIndex && skyIndex < skies.Num())
	{
		this->skyIndex = skyIndex;
		SetCurrentSky(skies[skyIndex]);
	}
}

void idSkyController::NextSky(void)
{
	skyIndex++;
	if (skyIndex >= skies.Num())
		skyIndex = 0;

	SetCurrentSky(skies[skyIndex]);
}

void idSkyController::ReloadSky(void)
{
	// Reload assets and entities, but try to reload the current sky instead of wiping the whole slate
	ParseSkies(&spawnArgs);
	FindSkyBrushes();
	FindLinkedLights();
	if (skies.Num())
	{
		if (skyIndex >= 0)
		{
			if (skyIndex < skies.Num() && skies[skyIndex])
				SetCurrentSky(skies[skyIndex]);
			else
			{
				gameLocal.Warning("idSkyController: Tried to reload sky, but could no longer find it!");
			}
		}
		else
			// We didn't have any skies before, and now we do -- let's load the first one
			SetCurrentSky(0); 
	}
}

idList<const idMaterial*> idSkyController::GetSkyList()
{
	return skies;
}

const idMaterial* idSkyController::GetCurrentSky(void)
{
	return skies[skyIndex];
}

void idDynamicSkyBrush::SetSkyMaterial(const idMaterial* sky)
{
	this->renderEntity.customShader = sky;
	BecomeActive(TH_UPDATEVISUALS);
}