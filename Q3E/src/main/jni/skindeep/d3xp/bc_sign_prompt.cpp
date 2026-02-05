#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "WorldSpawn.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"

#include "idlib/LangDict.h"

#include "bc_ventdoor.h"
#include "bc_meta.h"
#include "bc_ftl.h"

#include "bc_maintpanel.h"
#include "bc_catcage.h"
#include "bc_sign_prompt.h"

#define MAX_KEYPROMPTS 8

#define LIGHT_FORWARDOFFSET 12

#define ACTIVATIONTIME 600


const idEventDef EV_SetSignpromptEnable("SetSignpromptEnable", "d" );


CLASS_DECLARATION(idStaticEntity, idSignPrompt)
	EVENT(EV_SetSignpromptEnable, idSignPrompt::Event_SetSignEnable)
END_CLASS



idSignPrompt::idSignPrompt(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	idleSmoke = nullptr;

	infoState = 0;
	thinktimer = 0;

	spsState = SPS_ACTIVE;
	spsTimer = 0;

	//make it repairable.
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
}

idSignPrompt::~idSignPrompt(void)
{
	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
	}
}

void idSignPrompt::Spawn(void)
{
	infoState = SP_IDLE;	

	//Allow walking through it.
	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	idleSmoke = NULL;

	SetLight(true, spawnArgs.GetVector("_color"));

	if (!spawnArgs.GetBool("start_on", "1"))
	{
		Event_SetSignEnable(false); //start off.
	}

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.

	BecomeActive(TH_THINK);
}

void idSignPrompt::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( infoState ); // int infoState
	savefile->WriteInt( thinktimer ); // int thinktimer

	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteInt( spsState ); // int spsState
	savefile->WriteInt( spsTimer ); // int spsTimer
}

void idSignPrompt::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( infoState ); // int infoState
	savefile->ReadInt( thinktimer ); // int thinktimer

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadInt( spsState ); // int spsState
	savefile->ReadInt( spsTimer ); // int spsTimer
}

void idSignPrompt::Think(void)
{	
	//if not in PVS, then don't do anything...
	if (!gameLocal.InPlayerConnectedArea(this))
		return;	

	if (gameLocal.time > thinktimer)
	{
		thinktimer = gameLocal.time + 2000;
		UpdatePrompts();
	}

	if (headlightHandle != -1)
	{
		//Update position of the light glow.
		idVec3 forward;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
		headlight.origin = GetPhysics()->GetOrigin() + (forward * LIGHT_FORWARDOFFSET);
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}

	if (spsState == SPS_ACTIVATING)
	{
		//in the flicker-on state
		if (gameLocal.time > spsTimer)
		{
			spsState = SPS_ACTIVE;

			int lightRadius = spawnArgs.GetInt("light_radius");
			if (lightRadius > 0)
			{
				headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = lightRadius; //reset to default light size.
			}

			//stop flickering.
			headlight.shader = declManager->FindMaterial(spawnArgs.GetString("mtr_lighton"), false);
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}
	}

	idStaticEntity::Think();
}

void idSignPrompt::UpdatePrompts()
{
	const idKeyValue* kv;
	kv = this->spawnArgs.MatchPrefix("text", NULL);
	idList<idStr> newKeys;
	idList<idStr> newVals;
	while (kv)
	{
		idStr suffix = kv->GetKey().Right(1); //get the suffix number.
		
		idStr keysText = spawnArgs.GetString(idStr::Format("binds%s", suffix.c_str()).c_str());
		if (keysText.Length() <= 0)
		{
			continue;
		}
		
		//There are key prompts in this. Parse it.
		idStrList keyList = keysText.Split(',', true);
		
		for (int i = 0; i < keyList.Num(); i++)
		{
			//Get the keybind name.
			keyList[i] = gameLocal.GetKeyFromBinding(keyList[i].c_str());
		}
		
		if (keyList.Num() < MAX_KEYPROMPTS)
		{
			int keypromptsToAdd = MAX_KEYPROMPTS - keyList.Num();
			for (int i = 0; i < keypromptsToAdd; i++)
			{
				//Just fill it up with empty entries, so that we can safely call idStr::Format
				keyList.Append("");
			}
		}
		
		idStr localizedText = common->GetLanguageDict()->GetString(kv->GetValue().c_str());
		localizedText = idStr::Format(localizedText.c_str(), keyList[0].c_str(), keyList[1].c_str(), keyList[2].c_str(), keyList[3].c_str(), keyList[4].c_str(), keyList[5].c_str(), keyList[6].c_str(), keyList[7].c_str());

		newKeys.Append(idStr::Format("gui_parm%s", suffix.c_str()).c_str());
		newVals.Append(localizedText.c_str());

		kv = this->spawnArgs.MatchPrefix("text", kv);
	}	

	// do this separately, as this changes the arg dict, invalidating the kvp
	for (int i = 0; i < newKeys.Num(); i++)
	{
		Event_SetGuiParm(newKeys[i], newVals[i]);
	}
}

void idSignPrompt::GuiUpdate(idStr keyname, idStr value)
{
	//Event_SetGuiParm(idStr::Format("gui_parm%s", suffix.c_str()).c_str(), localizedText.c_str());
	Event_SetGuiParm(keyname.c_str(), value.c_str());
}

void idSignPrompt::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idDict args;

	Event_GuiNamedEvent(1, "onDamaged");

	if (infoState == SP_DAMAGED)
		return;

	health = 0;
	
	gameLocal.DoParticle("explosion_gascylinder.prt", GetPhysics()->GetOrigin());
	StartSound("snd_explode", SND_CHANNEL_BODY2);

	if (idleSmoke == NULL)
	{
		idVec3 forward;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

		args.Clear();
		args.Set("model", "machine_damaged_smoke.prt");
		args.Set("start_off", "0");
		idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));		
		idleSmoke->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0, 0, 12) + (forward * 4));

		idAngles angles = GetPhysics()->GetAxis().ToAngles();
		angles.pitch += 90;
		idleSmoke->SetAxis(angles.ToMat3());
	}
	else
	{
		idleSmoke->SetActive(true);
	}

	SetLight(false, vec3_zero);	//turn off light.

	infoState = SP_DAMAGED;
	needsRepair = true;
	repairrequestTimestamp = gameLocal.time;
}

void idSignPrompt::SetLight(bool value, idVec3 color)
{
	if (color == vec3_zero)
		value = false; //turn off light if color is empty.

	if (value)
	{
		//turn light on.

		if (headlightHandle == -1)
		{
			int lightRadius = spawnArgs.GetInt("light_radius");

			if (lightRadius > 0)
			{
				idVec3 forward;
				GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
				headlight.shader = declManager->FindMaterial(spawnArgs.GetString("mtr_lighton"), false);
				headlight.pointLight = true;
				headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = lightRadius;
				headlight.shaderParms[0] = color.x; // R
				headlight.shaderParms[1] = color.y; // G
				headlight.shaderParms[2] = color.z; // B
				headlight.shaderParms[3] = 1.0f;
				headlight.noShadows = true;
				headlight.isAmbient = false;
				headlight.axis = mat3_identity;
				headlight.origin = GetPhysics()->GetOrigin() + (forward * LIGHT_FORWARDOFFSET);
				headlightHandle = gameRenderWorld->AddLightDef(&headlight);
			}
		}
		else
		{
			headlight.shaderParms[0] = color.x; // R
			headlight.shaderParms[1] = color.y; // G
			headlight.shaderParms[2] = color.z; // B
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}
	}
	else
	{
		//kill light.

		if (headlightHandle != -1)
		{
			gameRenderWorld->FreeLightDef(headlightHandle);
			headlightHandle = -1;
		}
	}
}

void idSignPrompt::DoRepairTick(int amount)
{
	UpdateVisuals();
	infoState = SP_IDLE;

	Event_GuiNamedEvent(1, "onRepaired");

	if (idleSmoke != NULL)
	{
		idleSmoke->SetActive(false);
	}

	health = maxHealth;
	needsRepair = false;

	//Remove decals. This is to remove bulletholes/etc from covering the gui.
	if (modelDefHandle >= 0)
	{
		gameRenderWorld->RemoveDecals(modelDefHandle);
	}
}

void idSignPrompt::Event_SetSignEnable(int enable)
{
	if (enable)
	{
		//turn on.
		if (spsState != SPS_ACTIVATING && spsState != SPS_ACTIVE)
		{
			spsState = SPS_ACTIVATING;
			spsTimer = gameLocal.time + ACTIVATIONTIME;
			SetLight(true, spawnArgs.GetVector("_color")); //turn on light.
			StartSound("snd_activate", SND_CHANNEL_BODY2);
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_on")));			
			Event_GuiNamedEvent(1, "flickerOn");

			//do light flicker.
			int lightRadius = spawnArgs.GetInt("light_radius");
			if (lightRadius > 0)
			{
				headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = lightRadius * 3.0f; //exaggerate the flicker size.
			}
			headlight.shader = declManager->FindMaterial( spawnArgs.GetString("mtr_lightflicker"), false);
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

			
			
		}
	}
	else
	{
		//turn off.
		if (spsState != SPS_DEACTIVATED)
		{
			spsState = SPS_DEACTIVATED;
			StartSound("snd_deactivate", SND_CHANNEL_BODY2);
			SetLight(false, vec3_zero);	//turn off light.
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_off")));
		}
	}
}