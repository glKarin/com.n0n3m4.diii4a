#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"

#include "Target.h"
#include "bc_frobcube.h"
#include "bc_ventpeek.h"
#include "bc_meta.h"

#include "bc_cryospawn.h"
#include "bc_cryointerior.h"


#define DOOROPEN_ANIMTIME 600 //how long it takes for door anim to complete
#define FADETIME 200
#define MOVEHEADTIME 300

#define ICE_MELT_INITIALDELAY 500
#define ICE_MELT_TIME 3000
#define LIGHTFADE_TIME 500

const idEventDef EV_SetCryoFrobbable("setcryofrobbable", "d");

CLASS_DECLARATION(idStaticEntity, idCryointerior)
	EVENT(EV_SetCryoFrobbable, idCryointerior::SetCryoFrobbable)
END_CLASS

idCryointerior::idCryointerior(void)
{
	ceilingLight = nullptr;
	doorProp = nullptr;
	frobBar = nullptr;
	ventpeekEnt = nullptr;

	iceMeltTimer = 0;
	iceMeltState = 0;

	dripEmitter = nullptr;
}

idCryointerior::~idCryointerior(void)
{
}



void idCryointerior::Spawn(void)
{
	state = IDLE;
	fl.takedamage = false;
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
	

	//Spawn light.
	idDict args;
	args.Clear();
	args.Set("classname", "light");
	args.SetVector("origin", this->GetPhysics()->GetOrigin() + idVec3(0,0,64));
	args.SetVector("light_center", idVec3(0, 0, 16));
	args.SetInt("noshadows", 1);
	ceilingLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &args);
	ceilingLight->SetRadius(80);
	ceilingLight->SetColor(idVec4(0.3f, 0.4f, 0.5f, 1));
	//ceilingLight->SetColor(idVec4(0.54, 0.52, 0.24, 1));
	

	
	
	StartSound("snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL);

	iceMeltState = ICEMELT_INITIALDELAY;
	iceMeltTimer = gameLocal.time;

	
	//dripEmitter->PostEventMS(&EV_Activate, 0, this);

	Event_SetGuiParm("obj_key", gameLocal.GetKeyFromBinding("_contextmenu"));
	Event_SetGuiParm("zoom_key", gameLocal.GetKeyFromBinding("_zoom"));

	//spawn info location
	if (1)
	{
		idDict args;
		args.Clear();
		args.SetVector("origin", GetPhysics()->GetAbsBounds().GetCenter());
		args.Set("location", "#str_loc_cryochamber_00106");
		gameLocal.SpawnEntityType(idLocationEntity::Type, &args);
	}


	
}



void idCryointerior::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( stateTimer ); //  int stateTimer

	savefile->WriteObject( ceilingLight ); //  idLight * ceilingLight
	savefile->WriteObject( doorProp ); //  idAnimated* doorProp

	savefile->WriteObject( frobBar ); //  idEntity* frobBar

	savefile->WriteObject( cryospawn ); //  idEntityPtr<idEntity> cryospawn

	savefile->WriteObject( ventpeekEnt ); //  idEntity * ventpeekEnt

	savefile->WriteInt( iceMeltTimer ); //  int iceMeltTimer

	savefile->WriteInt( iceMeltState ); //  int iceMeltState

	savefile->WriteObject( dripEmitter ); //  idFuncEmitter * dripEmitter

}

void idCryointerior::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( stateTimer ); //  int stateTimer

	savefile->ReadObject( CastClassPtrRef(ceilingLight) ); //  idLight * ceilingLight
	savefile->ReadObject( CastClassPtrRef(doorProp) ); //  idAnimated* doorProp

	savefile->ReadObject( frobBar ); //  idEntity* frobBar

	savefile->ReadObject( cryospawn ); //  idEntityPtr<idEntity> cryospawn

	savefile->ReadObject( ventpeekEnt ); //  idEntity * ventpeekEnt

	savefile->ReadInt( iceMeltTimer ); //  int iceMeltTimer

	savefile->ReadInt( iceMeltState ); //  int iceMeltState

	savefile->ReadObject( CastClassPtrRef(dripEmitter) ); //  idFuncEmitter * dripEmitter
}

void idCryointerior::Think(void)
{	
	if (state == OPENING)
	{
		if (gameLocal.time >= stateTimer)
		{
			//Ok. Door animation is doing its thing. Now we move the player head toward the black void.
			idVec3 forward, up;
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

			idVec3 headDestination = this->GetPhysics()->GetOrigin() + up * 52 + forward * 28;
			gameLocal.GetLocalPlayer()->SetViewposAbsLerp(headDestination, MOVEHEADTIME);

			float targetYaw = this->GetPhysics()->GetAxis().ToAngles().yaw;
			gameLocal.GetLocalPlayer()->SetViewPitchLerp(0, MOVEHEADTIME);
			gameLocal.GetLocalPlayer()->SetViewYawLerp(targetYaw, MOVEHEADTIME);

			stateTimer = gameLocal.time + MOVEHEADTIME + 10;
			state = MOVINGHEAD;
		}
	}
	else if (state == MOVINGHEAD)
	{
		if (gameLocal.time >= stateTimer)
		{
			//Teleport player to the exit point.
			if (!cryospawn.IsValid()) //Check if it exists again.... juuuuusssttt in case it gets gobbled up somehow
			{
				gameLocal.Error("Cryointerior '%s' does not have an assigned cryospawn point.", this->GetName());
				return;
			}

			if (!cryospawn.GetEntity())
			{
				return;
			}

			idVec3 forward, up;
			cryospawn.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

			//Find a spot for player to spawn. We do a bounds check (instead of a point check) because under certain
			//circumstances, the check can sometimes sneak right through the gap between a ventdoor and ventdoorframe.
			trace_t tr;
			idVec3 traceStart = cryospawn.GetEntity()->GetPhysics()->GetOrigin() + forward * 31;
			//gameLocal.clip.TracePoint(tr, traceStart, traceStart + up * -80, MASK_SOLID, NULL);
			#define	CRYOBOUNDSIZE 1
			gameLocal.clip.TraceBounds(tr, traceStart, traceStart + up * -80, idBounds(idVec3(-CRYOBOUNDSIZE, -CRYOBOUNDSIZE, -CRYOBOUNDSIZE), idVec3(CRYOBOUNDSIZE, CRYOBOUNDSIZE, CRYOBOUNDSIZE)), MASK_SOLID, NULL);

			
			gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 0.0f, FADETIME); //Fade into your new location.
			gameLocal.GetLocalPlayer()->Teleport(tr.endpos, cryospawn.GetEntity()->GetPhysics()->GetAxis().ToAngles(), cryospawn.GetEntity(), false); //Teleport player.
			
			if (gameLocal.GetLocalPlayer()->GetPhysics()->HasGroundContacts()) //If there's ground, then do a lerp that continues/matches the previous camera move.
			{
				idVec3 headStartPos = gameLocal.GetLocalPlayer()->GetEyePosition() + forward * -30 + idVec3(0, 0, 16);
				gameLocal.GetLocalPlayer()->SetViewposAbsLerp2(headStartPos, FADETIME + 100);
			}

			cryospawn.GetEntity()->BecomeActive(TH_THINK);
			static_cast<idCryospawn *>(cryospawn.GetEntity())->Event_PlayAnim("opened", 1, false);
			state = DONE;


			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetPlayerExitedCryopod(true);


			//Temp.
			//gameLocal.GetLocalPlayer()->GiveItem("moveable_item_autopistol");
			//gameLocal.GetLocalPlayer()->GiveItem("weapon_autopistol");


			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateCagecageObjectiveText();

			idStr exitCryoScript = gameLocal.world->spawnArgs.GetString("call_cryoexit");
			if (exitCryoScript.Length() > 0)
			{
				gameLocal.RunMapScript(exitCryoScript.c_str());
			}


			common->g_SteamUtilities->SetSteamTimelineEvent("steam_transfer");
		}
	}

	if (iceMeltState == ICEMELT_INITIALDELAY)
	{
		if (gameLocal.time > iceMeltTimer + ICE_MELT_INITIALDELAY)
		{
			dripEmitter->SetActive(true);
			iceMeltState = ICEMELT_MELTING;
			iceMeltTimer = gameLocal.time;
			//StartSound("snd_icemelt", SND_CHANNEL_BODY);
			StartSound("snd_drips", SND_CHANNEL_BODY2);
			
		}
	}
	else if (iceMeltState == ICEMELT_MELTING)
	{
		int meltTimer = gameLocal.time - iceMeltTimer;		
		float iceLerp = meltTimer / (float)ICE_MELT_TIME;
		iceLerp = idMath::ClampFloat(0, 1, iceLerp);

		renderEntity.shaderParms[3] = 1 - iceLerp;
		renderEntity.shaderParms[7] = iceLerp;
		UpdateVisuals();
		doorProp->GetRenderEntity()->shaderParms[3] = 1 - iceLerp;
		doorProp->UpdateVisuals();		

		if (gameLocal.time > iceMeltTimer + ICE_MELT_TIME)
		{
			dripEmitter->SetActive(false);
			iceMeltState = ICEMELT_LIGHTFADE;
			iceMeltTimer = gameLocal.time;

			StartSound("snd_poweron", SND_CHANNEL_BODY3);			
		}
	}
	else if (iceMeltState == ICEMELT_LIGHTFADE)
	{
		int lightTimer = gameLocal.time - iceMeltTimer;
		float lightLerp = lightTimer / (float)LIGHTFADE_TIME;
		lightLerp = idMath::ClampFloat(0, 1, lightLerp);

		idVec4 newColor;
		newColor.Lerp(idVec4(.3f, .4f, .5f, 1), idVec4(0.54f, 0.52f, 0.24f, 1), lightLerp);
		ceilingLight->SetColor(newColor);
	}

	idStaticEntity::Think();
}




bool idCryointerior::DoFrob(int index, idEntity * frobber)
{
	if (state != IDLE)
		return false;

	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	if (index == 1 || index == PEEKFROB_INDEX)
	{
		//Wants to open the door.

		if (!cryospawn.IsValid())
		{
			gameLocal.Error("Cryointerior '%s' does not have an assigned cryospawn point.", this->GetName());
			return false;
		}

		if (index == PEEKFROB_INDEX)
		{
			//Snap player's head toward correct angle.
			idAngles defaultAngle = idAngles(0, GetPhysics()->GetAxis().ToAngles().yaw, 0);
			gameLocal.GetLocalPlayer()->SetViewAngles(defaultAngle);
		}


		ventpeekEnt->Hide();
		frobBar->Hide();
		doorProp->Event_PlayAnim("open", 1, false);
		state = OPENING;
		stateTimer = gameLocal.time + DOOROPEN_ANIMTIME;


		//Turn player head.
		float targetYaw = this->GetPhysics()->GetAxis().ToAngles().yaw;
		gameLocal.GetLocalPlayer()->SetViewPitchLerp(30, DOOROPEN_ANIMTIME);
		gameLocal.GetLocalPlayer()->SetViewYawLerp(targetYaw, DOOROPEN_ANIMTIME);
	}

	return true;
}


void idCryointerior::SetExitPoint(idEntity *ent)
{
	//Ok, we now have an exit point.
	cryospawn = ent;

	#define PEEK_FORWARD_DISTANCE 5
	#define PEEK_UP_OFFSET 20

	//Set up the peek stuff.

	//Set up the null that the ventpeek looks at.
	idVec3 exitForward;
	ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&exitForward, NULL, NULL);
	idEntity *nullEnt;
	idDict args;
	args.Clear();
	args.SetVector("origin", ent->GetPhysics()->GetOrigin() + (exitForward * PEEK_FORWARD_DISTANCE) + idVec3(0,0, PEEK_UP_OFFSET));
	args.SetFloat("angle", ent->GetPhysics()->GetAxis().ToAngles().yaw);
	nullEnt = (idTarget *)gameLocal.SpawnEntityType(idTarget::Type, &args);


	//Set up the ventpeek.
	idVec3 interiorForward;
	idVec3 interiorUp;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&interiorForward, NULL, &interiorUp);	
	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin() + interiorForward * 28 + interiorUp * 67);
	args.SetFloat("angle", this->GetPhysics()->GetAxis().ToAngles().yaw + 180);
	args.Set("target", nullEnt->GetName());
	args.Set("displayname", "#str_def_gameplay_peek");
	args.SetInt("yaw_arc", 70);
	args.SetInt("pitch_arc", 70);
	args.Set("snd_activate", "shuffle");
	args.Set("snd_turnhead", "shuffle_quiet");
	args.SetBool("use_targetangle", true);
	ventpeekEnt = (idVentpeek *)gameLocal.SpawnEntityType(idVentpeek::Type, &args);
	static_cast<idVentpeek *>(ventpeekEnt)->ownerEnt = this;

	

	//Spawn everything.

	//Spawn door.
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetFloat("angle", GetPhysics()->GetAxis().ToAngles().yaw);
	args.Set("model", "model_cryodoors");
	doorProp = (idAnimated *)gameLocal.SpawnEntityType(idAnimated::Type, &args);



	//Spawn frobcube for the bar.
	idVec3 forwardDir, upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, &upDir);
	args.Clear();
	args.Set("model", "models/objects/frobcube/cube4x4.ase");
	args.Set("displayname", "#str_def_gameplay_deploy");
	frobBar = gameLocal.SpawnEntityType(idFrobcube::Type, &args);
	frobBar->SetOrigin(GetPhysics()->GetOrigin() + (upDir * 51) + (forwardDir * 24));
	frobBar->GetPhysics()->GetClipModel()->SetOwner(this);
	static_cast<idFrobcube*>(frobBar)->SetIndex(1);

	args.Clear();
	args.Set("model", spawnArgs.GetString("smoke_drips", "icicle_drip.prt"));
	idVec3 seamPos = GetPhysics()->GetOrigin() + (forwardDir * 23) + idVec3(0, 0, 80);
	idAngles seamAngle = idAngles(-90, GetPhysics()->GetAxis().ToAngles().yaw, 0);
	dripEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	dripEmitter->GetPhysics()->SetOrigin(seamPos);
	dripEmitter->GetPhysics()->SetAxis(seamAngle.ToMat3());

	BecomeActive(TH_THINK);



	//spawn the monitor.
	if (1)
	{
		//orient / position the monitor at the correct place

		idVec3 forward, right, up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
		idVec3 monitorPos = GetPhysics()->GetOrigin() + (forward * 6) + (right * -24) + (up * 64);

		idDict args;
		args.Clear();
		args.SetInt("solid", 0);
		args.Set("model", spawnArgs.GetString("model_monitor"));
		args.Set("gui", spawnArgs.GetString("gui_monitor"));
		args.SetVector("origin", monitorPos);
		args.SetFloat("angle", GetPhysics()->GetAxis().ToAngles().yaw - 90);
		args.Set("gui_parm0", gameLocal.world->spawnArgs.GetString("cryo_briefing", "#str_loc_unknown_00104"));
		idEntity *monitor = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
		if (monitor)
		{
			monitor->Event_SetGuiParm("shipname", gameLocal.GetShipName());
			monitor->Event_SetGuiParm("shipdesc", gameLocal.GetShipDesc());
		}
	}

	if (1)
	{
		//Spawn the certificate inspectable
		//this certificate is a shameful hardcoded entity...
		idVec3 forward, right, up;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
		idVec3 pos = GetPhysics()->GetOrigin() + (forward * 24) + (up * 78.5f) + (right * -20);

		idEntity *inspectpoint = NULL;
		idDict args;
		args.Clear();
		args.Set("classname", "info_zoominspectpoint");
		args.SetVector("origin", pos);
		args.SetFloat("angle", GetPhysics()->GetAxis().ToAngles().yaw + 180);
		args.SetVector("zoominspect_campos", idVec3(4.5f, 0, 0));
		args.Set("displayname", "#str_def_gameplay_certificate");
		args.Set("loc_inspectiontext", "#str_label_certificate");
		gameLocal.SpawnEntityDef(args, &inspectpoint);
	}



	
}

void idCryointerior::SetCryoFrobbable(int value)
{
	if (value <= 0)
	{
		frobBar->Hide();
		ventpeekEnt->Hide();
	}
	else
	{
		frobBar->Show();
		ventpeekEnt->Show();
	}
}
