#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "WorldSpawn.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
#include "bc_meta.h"
#include "bc_ftl.h"

#include "bc_infoscreen.h"


//This is a generic info screen.
//What it can do:
//- showing info on its gui
//- getting damaged / blown up
//- emitting a soft light glow


CLASS_DECLARATION(idStaticEntity, idInfoScreen)
END_CLASS

idInfoScreen::idInfoScreen(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	idleSmoke = nullptr;

	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
}

idInfoScreen::~idInfoScreen(void)
{
	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
	}
}

void idInfoScreen::Spawn(void)
{
	infoState = INFOSTAT_IDLE;
	
	//Takes damage, but has no collision (so player can slide against it without clipping).
	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	idleSmoke = NULL;	

	//is it repairable.
	if (!spawnArgs.GetBool("repairable", "0"))
	{
		repairNode.Remove(); // blendo eric: inverse (remove if DNE), so that it can regen during saveload
	}

	if (spawnArgs.GetBool("uniquegui", "1"))
	{
		this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.
	}
}

void idInfoScreen::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( infoState ); // int infoState

	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle
}

void idInfoScreen::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( infoState ); // int infoState

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}
}

void idInfoScreen::Think(void)
{	
	//if not in PVS, then don't do anything...
	if (!gameLocal.InPlayerConnectedArea(this))
		return;
	
	idStaticEntity::Think();
}


void idInfoScreen::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idDict args;

	Event_GuiNamedEvent(1, "onDamaged"); //gui event when damaged.

	if (infoState == INFOSTAT_DAMAGED)
		return;

	gameLocal.DoParticle("explosion_gascylinder.prt", GetPhysics()->GetOrigin());
	StartSound("snd_explode", SND_CHANNEL_BODY2);
	StartSound("snd_broken", SND_CHANNEL_AMBIENT);
	

	if (idleSmoke == NULL)
	{
		//Spawn damage smoke.
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

	infoState = INFOSTAT_DAMAGED;

	if (spawnArgs.GetBool("repairable", "0"))
	{
		needsRepair = true;
		repairrequestTimestamp = gameLocal.time;
	}
}

//Give the screen a soft light glow.
void idInfoScreen::SetLight(bool value, idVec3 color)
{
	if (value)
	{
		if (headlightHandle == -1)
		{
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
			headlight.pointLight = true;
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 24.0f;
			headlight.shaderParms[0] = color.x; // R
			headlight.shaderParms[1] = color.y; // G
			headlight.shaderParms[2] = color.z; // B
			headlight.shaderParms[3] = 1.0f;
			headlight.noShadows = true;
			headlight.isAmbient = false;
			headlight.axis = mat3_identity;
			headlight.origin = GetPhysics()->GetOrigin() + (forward * 12);
			headlightHandle = gameRenderWorld->AddLightDef(&headlight);
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
		if (headlightHandle != -1)
		{
			gameRenderWorld->FreeLightDef(headlightHandle);
			headlightHandle = -1;
		}
	}
}

//When screen has been repaired.
void idInfoScreen::DoRepairTick(int amount)
{	
	UpdateVisuals();
	infoState = INFOSTAT_IDLE;

	Event_GuiNamedEvent(1, "onRepaired");

	if (idleSmoke != NULL)
	{
		idleSmoke->SetActive(false);
	}

	health = maxHealth;
	needsRepair = false;

	StopSound(SND_CHANNEL_AMBIENT);

	
	//Remove decals. This is to remove bulletholes/etc from covering the gui.
	if (modelDefHandle >= 0)
	{
		gameRenderWorld->RemoveDecals(modelDefHandle);
	}
}