#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "idlib/LangDict.h"

//#include "SmokeParticles.h"
//#include "Game_local.h"
//#include "trigger.h"
//#include "Misc.h"

#include "BrittleFracture.h"

#include "bc_windowshutter.h"

CLASS_DECLARATION(idStaticEntity, idWindowShutter)
	EVENT(EV_SpectatorTouch, idWindowShutter::Event_SpectatorTouch)
END_CLASS

#define SHUTTER_CLOSETIME 5000

idWindowShutter::idWindowShutter()
{
	assignedWindow = NULL;

	windowshutterNode.SetOwner(this);
	windowshutterNode.AddToEnd(gameLocal.windowshutterEntities);

	shutterState = SHT_IDLE;
	shutterTimer = 0;

	locbox = NULL;
}

idWindowShutter::~idWindowShutter(void)
{
	windowshutterNode.Remove();
}

void idWindowShutter::Spawn(void)
{
	this->fl.takedamage = false;

	//this->Hide();
	renderEntity.noShadow = true;

	if (spawnArgs.GetBool("start_on", "1"))
	{
		//starts on/active/displayed.
		SetShutterOpen(false);
	}
	else
	{
		//Starts off/hidden.
		SetShutterOpen(true);
	}


	//BC 3-26-2025
	idBounds shutterBounds = GetPhysics()->GetAbsBounds();
	idVec3 locboxPos = shutterBounds.GetCenter();
	locboxPos.z = shutterBounds[0].z;
	
	#define LOCBOXRADIUS 12
	idDict args;
	args.Clear();
	args.Set("text", common->GetLanguageDict()->GetString("#str_label_ventbarricade"));
	args.SetVector("origin", locboxPos);
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
	args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
	locbox = static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));


	BecomeActive(TH_THINK);
	PostEventMS(&EV_PostSpawn, 0);
}

void idWindowShutter::Event_PostSpawn(void)
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idBrittleFracture::Type))
		{
			if (DoesWindowIntersect(ent))
			{
				//mark the window as indestructible.
				ent->fl.takedamage = false;
				assignedWindow = ent;
				break;
			}
		}
	}

	if (!assignedWindow.IsValid())
	{
		//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin(), windowEnt->GetPhysics()->GetOrigin(), 4, 500000);
		gameRenderWorld->DebugArrowSimple(GetPhysics()->GetOrigin(), 900000, colorRed);
		gameRenderWorld->DebugTextSimple(idStr::Format("'%s' origin can't find window", GetName()), GetPhysics()->GetOrigin() + idVec3(0, 0, 64), 900000, colorRed);
		common->Warning("WindowShutter '%s' origin is not touching a func_fracture", GetName());
	}
}

bool idWindowShutter::DoesWindowIntersect(idEntity *windowEnt)
{
	idBounds shutterBounds = idBounds(spawnArgs.GetVector("origin")).Expand(6);
	idBounds windowBounds = windowEnt->GetPhysics()->GetAbsBounds();

	if (shutterBounds.IntersectsBounds(windowBounds))
	{
		return true;
	}	
	
	return false;
}

void idWindowShutter::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( assignedWindow ); // idEntityPtr<idEntity> assignedWindow

	savefile->WriteInt( shutterState ); // int shutterState
	savefile->WriteInt( shutterTimer ); // int shutterTimer

	savefile->WriteObject( locbox ); // iidEntity* locbox
}

void idWindowShutter::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( assignedWindow ); // idEntityPtr<idEntity> assignedWindow

	savefile->ReadInt( shutterState ); // int shutterState
	savefile->ReadInt( shutterTimer ); // int shutterTimer

	savefile->ReadObject( locbox ); // iidEntity* locbox
}

void idWindowShutter::SetShutterOpen(bool value)
{
	if (value)
	{
		//The shutter starts disappearing.
		if (shutterState != SHT_SHUTTERING)
		{
			StartSound("snd_open", SND_CHANNEL_ANY);
		}

		shutterState = SHT_SHUTTERING;
		shutterTimer = gameLocal.time;
		
		if (assignedWindow.IsValid())
		{
			assignedWindow.GetEntity()->fl.takedamage = true;
		}

		this->fl.takedamage = false;
		GetPhysics()->SetContents(0);

		locbox->Hide();

	}
	else
	{
		//Make the shutter display/unhide itself.
		shutterState = SHT_IDLE;
		Show();
		GetPhysics()->SetContents(CONTENTS_SOLID);
	}
}

void idWindowShutter::Think(void)
{
	idStaticEntity::Think();

	if (shutterState == SHT_SHUTTERING)
	{
		float lerp = (gameLocal.time - shutterTimer) / (float)SHUTTER_CLOSETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = idMath::CubicEaseOut(lerp);

		renderEntity.shaderParms[7] = lerp;
		UpdateVisuals();

		if (gameLocal.time >= shutterTimer + SHUTTER_CLOSETIME)
		{
			Hide();
			shutterState = SHT_SHUTTERED;
		}
	}
}

// SW 22nd April 2025: mostly copied from BrittleFracture
void idWindowShutter::Event_SpectatorTouch(idEntity* other, trace_t* trace)
{
	idVec3		contact, translate, normal;
	idBounds	bounds;
	idPlayer* p;

	assert(other && other->IsType(idPlayer::Type) && static_cast<idPlayer*>(other)->spectating);

	p = static_cast<idPlayer*>(other);
	// avoid flicker when stopping right at clip box boundaries
	if (p->lastSpectateTeleport > gameLocal.hudTime - 300) { //BC was 1000
		return;
	}

	//Attempt to find a space for player to teleport to.
	idVec3 glassContact = trace->endpos;

	idVec3 playerMovedir = p->GetPhysics()->GetLinearVelocity().ToAngles().ToForward();
	idVec3 candidateSpot = glassContact + playerMovedir * 64;

	if (gameLocal.LocationForPoint(candidateSpot) == NULL)
		return;

	trace_t candidateTr;
	gameLocal.clip.TraceBounds(candidateTr, candidateSpot, candidateSpot + idVec3(0, 0, 1), p->GetPhysics()->GetBounds(), MASK_SOLID, NULL);

	if (candidateTr.fraction < 1.0f)
		return;

	p->SetOrigin(candidateSpot);
	p->lastSpectateTeleport = gameLocal.hudTime;
	gameLocal.GetLocalPlayer()->StartSound("snd_spectate_door", SND_CHANNEL_ANY);
}