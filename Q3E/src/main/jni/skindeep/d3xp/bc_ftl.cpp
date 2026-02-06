#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"
#include "WorldSpawn.h"

#include "Misc.h"
#include "bc_skullsaver.h"
#include "bc_bossmonster.h"
#include "bc_sabotagelever.h"
#include "bc_meta.h"
#include "bc_ftl.h"



#define NORMALMODE
#ifdef NORMALMODE
	const int MS_BETWEEN_JUMPS		= 30000;	//How long the countdown timer is. This is how long the player has to slam-dunk all the skullsavers they're carrying.
	const int OPEN_ANIMTIME			= 2100;		//Time it takes for sheathe open animation to play.
	const int ACTIVETIME			= 300;		//How long the sheathe stays open (how long FTL white-vignette jump sequence is). Note that this value gets changed by the slowmo factor.
	const int JUMP_AUDIOCUE_LENGTH	= 3000;		//How many ms in the ftl jump audio file before the FWOOM jump sound
	const int CLOSETIME				= 2100;		//How long for sheathe to do its closing animation.
	
	const int CLOSEDELAYTIME		= 100;		//We need a little delay before we do the FTL climactic fwoom sound, because we need time for slowmo to exit before playing sound fx.
	const int PIPE_PAUSE_MAXTIME	= 300000;	//how long the pipe sabotage takes.
	const int BOSS_EXITTIME			= 1500;		//During FTL countdown, boss teleports away this many MS before the countdown ends.
#else
//debug parameters. Makes FTL happen fast.
	const int MS_BETWEEN_JUMPS = 1500;	//45000 How long does it take to charge up FTL between jumps. millseconds.
	const int OPEN_ANIMTIME = 2100;		//Time it takes for sheathe open animation to play.
	const int ACTIVETIME = 300;		//How long the sheathe stays open (how long FTL white-vignette jump sequence is). Note that this value gets changed by the slowmo factor.
	const int JUMP_AUDIOCUE_LENGTH = 3000;		//How many ms in the ftl jump audio file before the FWOOM jump sound
	const int CLOSETIME = 2100;		//How long for sheathe to do its closing animation.	
	const int CLOSEDELAYTIME = 100;		//We need a little delay before we do the FTL climactic fwoom sound, because we need time for slowmo to exit before playing sound fx.
	const int PIPE_PAUSE_MAXTIME = 300000;	//how long the pipe sabotage takes.
	const int BOSS_EXITTIME = 1500;		//During FTL countdown, boss teleports away this many MS before the countdown ends.
#endif


const int FTL_DISPATCHTIME = 5000; //how long the dispatch VO is, in ms.
const int FTL_DISPATCH_DELAYTIME = 1100;


const int OUTSIDE_DAMAGE_INTERVAL = 70; //when player is outside during FTL, inflict damage every XX msec

//const idVec4 RODLIGHT_DEFAULTCOLOR = idVec4(0, .2f, .2f, 1);
const idStr RODLIGHT_DEFAULTCOLOR_STR = idStr("0 .2 .2");
const idVec4 RODLIGHT_DAMAGECOLOR = idVec4(.4f, 0, 0, 1);

const idEventDef EV_ftl_activate("ftl_activate");
const idEventDef EV_ftl_setstate("ftl_setstate", "d");
const idEventDef EV_ftl_getstate("ftl_getstate", NULL, 'd');

CLASS_DECLARATION(idStaticEntity, idFTL)
	EVENT(EV_PostSpawn, idFTL::Event_PostSpawn)
	EVENT(EV_ftl_activate, idFTL::StartCountdown)
	EVENT(EV_ftl_setstate, idFTL::Event_SetState)
	EVENT(EV_ftl_getstate, idFTL::Event_GetState)
END_CLASS


idFTL::idFTL(void)
{
	bafflerNode.SetOwner(this);
	bafflerNode.AddToEnd(gameLocal.bafflerEntities);

	sheatheEnt = nullptr;
	rodLight = nullptr;

	ftlTimer = {};
	ftlState = {};

	ftlPauseTime = {};

	// smokeEmitters = {};
	seamEmitter = nullptr;

	initializedHealth = false;
	lastHealth = {};
	voiceoverIndex = {};
	pipepauseTimer = {};
	openAnimationStarted = {};
	outsideDamageTimer = {};
	additionalCountdownTime = {};
	defaultJumpTime = {};
}

idFTL::~idFTL(void)
{
	bafflerNode.Remove();
}

void idFTL::Spawn(void)
{
	int i;
	idDict args;
	idVec3 sheathePos;
	idVec3 forwardDir, rightDir;	

	BecomeActive(TH_THINK);
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);


	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);
	idVec3 sheatheOffset = spawnArgs.GetVector("sheathe_offset");	
	sheathePos = GetPhysics()->GetOrigin();
	sheathePos += forwardDir * sheatheOffset.x;
	sheathePos.z += sheatheOffset.z;	

	args.Clear();
	args.SetVector("origin", sheathePos);
	args.Set("model", spawnArgs.GetString("model_sheathe"));
	args.SetBool("dynamicspectrum", spawnArgs.GetBool("dynamicspectrum", "0"));
	sheatheEnt = (idAnimated *)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	sheatheEnt->SetAngles(this->GetPhysics()->GetAxis().ToAngles());	

	idVec3 lightPos;
	idVec3 lightOffset = spawnArgs.GetVector("light_offset");
	lightPos = GetPhysics()->GetOrigin();
	lightPos += forwardDir * lightOffset.x;
	lightPos.z += lightOffset.z;

	args.Clear();
	args.SetVector("origin", lightPos);
	args.Set("texture", "lights/ftl_rod");	
	args.SetInt("noshadows", 0);
	args.SetInt("start_off", 1);	
	rodLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &args);
	rodLight->SetRadius(512);
	rodLight->SetColor(spawnArgs.GetVector("color_ftl", RODLIGHT_DEFAULTCOLOR_STR));
	rodLight->Off();
	rodLight->Bind(this, false);


	for (i = 0; i < 3; i++)
	{
		idVec3 smokePos = GetPhysics()->GetOrigin() + forwardDir * 350;
		idAngles smokeAng = GetPhysics()->GetAxis().ToAngles();

		if (i == 0)
		{
			smokePos += idVec3(0, 0, 480); //the top smoke emitter
		}
		else if (i == 1)
		{
			smokePos += idVec3(0, 0, 320) + rightDir * 210;
			smokeAng.yaw += -90;
			smokeAng.pitch += 75;
		}
		else
		{
			smokePos += idVec3(0, 0, 320) + rightDir * -210;
			smokeAng.yaw += 90;
			smokeAng.pitch += 75;
		}

		args.Clear();
		//args.Set("model", "ftl_idlesmoke.prt");
		args.Set("model", "ftl_idlesmoke.prt");
		args.Set("start_off", "1");

		if (i == 0)
		{
			args.Set("snd_whistle", spawnArgs.GetString("snd_whistle"));
		}

		smokeEmitters[i] = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
		smokeEmitters[i]->GetPhysics()->SetOrigin(smokePos);
		smokeEmitters[i]->SetAngles(smokeAng);
		//smokeEmitters[i]->PostEventMS(&EV_Activate, 0, this);
	}

    //TODO: do logic for turning this on and off during FTL jumps.
    args.Clear();
	args.Set("model", spawnArgs.GetString("smoke_seam", "ftl_seamsmoke.prt"));
    idVec3 seamPos = GetPhysics()->GetOrigin() + forwardDir * spawnArgs.GetFloat("seam_distance", "144");
    idAngles seamAngle = idAngles(90, GetPhysics()->GetAxis().ToAngles().yaw, 0);
    seamEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
    seamEmitter->GetPhysics()->SetOrigin(seamPos);
    seamEmitter->GetPhysics()->SetAxis(seamAngle.ToMat3());
    seamEmitter->PostEventMS(&EV_Activate, 0, this);	
	
	initializedHealth = false;
	normalizedHealth = 100;
	pipepauseTimer = 0;
	ftlPauseTime = 0;
	openAnimationStarted = false;

	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_idle")));

	StartSound("snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL);

	outsideDamageTimer = 0;

	PostEventMS(&EV_PostSpawn, 0);

	additionalCountdownTime = 0;
	defaultJumpTime = SEC2MS(gameLocal.world->spawnArgs.GetInt("ftl_default_time"));

	fl.takedamage = false;
}

void idFTL::Event_PostSpawn(void)
{
	idEntity *	healthGuiEnt;

	healthGuiEnt = gameLocal.FindEntity(spawnArgs.GetString("healthgui"));

	//TODO: make this use a targetnull instead. streamline this.
	//if (!healthGuiEnt)
	//{
	//	gameLocal.Error("FTL %s is missing its healthgui entity. Cannot find entity with name: '%s'", GetName(), spawnArgs.GetString("healthgui"));
	//}

	healthGUI = healthGuiEnt;

	//StartIdle();

	if (spawnArgs.GetBool("start_on", "1"))
	{
		ftlState = FTL_OPENING; //Sab 2.0
		ftlTimer = gameLocal.time + 100; //Give things a bit of time to settle in.
	}
	else
	{
		ftlState = FTL_DORMANT;
	}
}

//Transition FTL into the activate state.
void idFTL::StartActiveState()
{
    if (gameLocal.time < 1000)
        sheatheEnt->Event_PlayAnim("open", 0); //only play anim if it's immediately at gamestart.


	ftlState = FTL_ACTIVE;

	StopSound(SND_CHANNEL_AMBIENT, false);
	StartSound("snd_active", SND_CHANNEL_AMBIENT, 0, false, NULL);
	//Event_SetGuiParm("gui_parm0", "ENGAGED");

	rodLight->On();
	seamEmitter->SetActive(false); //stop the smoke coming out of the seam.
	
	//attempt to swap to the FTL skyportal.
	idEntity		*skyEnt;
	skyEnt = gameLocal.FindEntity("portalsky_ftl");
	if (skyEnt)
	{
		if (skyEnt->IsType(idPortalSky::Type))
		{
			static_cast<idPortalSky *>(skyEnt)->Event_Activate(this);
		}
	}

	
    static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->FTL_JumpStarted();

	if (gameLocal.time > 1000)
	{
		//gameLocal.GetLocalPlayer()->SetImpactSlowmo(true, 100);
		gameLocal.AddEventLog("#str_def_gameplay_ftl_engage", GetPhysics()->GetOrigin());
	}

	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLifeboatsLeave();
}

void idFTL::StartIdle()
{
	ftlState = FTL_IDLE;
	//Event_SetGuiParm("gui_parm0", "RESTING");
	Event_GuiNamedEvent(1, "barsOff");
	seamEmitter->SetActive(true); //start smoke coming out of the seam.	
	
	//Reset is done... now check if escalation is high enough to trigger another FTL jump.	
	//if (gameLocal.metaEnt.IsValid() && gameLocal.time > 1000) //don't do this immediately at game start because the meta entity sometimes thinks we're at max escalation at game start.
	//{
	//	bool maxEscalation = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->IsEscalationMaxed();
	//
	//	if (maxEscalation)
	//	{
	//		//Start the next FTL jump immediately.
	//		StartCountdown();
	//	}
	//}

	ftlTimer = gameLocal.time + 2000; //Sab2.0: a little bit of dead time before the ftl reboot sequence starts
}

void idFTL::StartShutdown()
{
	if (ftlState == FTL_CLOSING || ftlState == FTL_IDLE)
		return;

	gameLocal.AddEventLog("#str_def_gameplay_ftl_disengage", GetPhysics()->GetOrigin());

	StartSound("snd_brakes", SND_CHANNEL_ANY);
	gameLocal.GetLocalPlayer()->FlashScreen();
	DoCloseSequence();

	//Attempt to swap to non-ftl skyportal.
	idEntity		*skyEnt;
	skyEnt = gameLocal.FindEntity("portalsky_debris");
	if (skyEnt)
	{
		if (skyEnt->IsType(idPortalSky::Type))
		{
			static_cast<idPortalSky *>(skyEnt)->Event_Activate(this);
		}
	}

	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->FTL_JumpEnded();
}

//FTL just completed, reset stuff.....
void idFTL::StartCountdown()
{
	if (!gameLocal.world->spawnArgs.GetBool("ftl_enabled", "1"))
		return;

	if (ftlState == FTL_DISPATCHDELAY || ftlState == FTL_DISPATCH || ftlState == FTL_COUNTDOWN || ftlState == FTL_OPENING || ftlState == FTL_ACTIVE || ftlState == FTL_CLOSEDELAY || ftlState == FTL_CLOSING)
		return;

	ftlTimer = gameLocal.time + FTL_DISPATCH_DELAYTIME;
	ftlState = FTL_DISPATCHDELAY;
}

//Return the remaining countdown time -- the function that receives this should then make it human-readable via gameLocal.ParseTimeMS()
int idFTL::GetPublicTimer()
{
	if (ftlState == FTL_COUNTDOWN)
	{
		return Max(ftlTimer - gameLocal.time, 0);
	}
	else if (ftlState == FTL_PIPEPAUSE)
	{
		return Max(ftlTimer - ftlPauseTime, 0);
	}
	
	return 0;
}

//Return remaining time to reboot the pipes after they've been sabotaged.
int idFTL::GetPublicPauseTimer()
{
	return Max(pipepauseTimer - gameLocal.time, 0);
}





void idFTL::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( normalizedHealth ); // int normalizedHealth

	savefile->WriteObject( sheatheEnt ); // idAnimated* sheatheEnt
	savefile->WriteObject( rodLight ); // idLight * rodLight

	savefile->WriteInt( ftlTimer ); // int ftlTimer
	savefile->WriteInt( ftlState ); // int ftlState

	savefile->WriteInt( ftlPauseTime ); // int ftlPauseTime

	SaveFileWriteArray(smokeEmitters, 3, WriteObject); // idFuncEmitter *smokeEmitters[3]

	savefile->WriteObject( seamEmitter ); // idFuncEmitter * seamEmitter

	savefile->WriteBool( initializedHealth ); // bool initializedHealth
	savefile->WriteInt( lastHealth ); // int lastHealth

	savefile->WriteObject( healthGUI ); // idEntityPtr<idEntity> healthGUI

	savefile->WriteInt( voiceoverIndex ); // int voiceoverIndex
	savefile->WriteInt( pipepauseTimer ); // int pipepauseTimer
	savefile->WriteBool( openAnimationStarted ); // bool openAnimationStarted
	savefile->WriteInt( outsideDamageTimer ); // int outsideDamageTimer
	savefile->WriteInt( additionalCountdownTime ); // int additionalCountdownTime
	savefile->WriteInt( defaultJumpTime ); // int defaultJumpTime
}

void idFTL::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( normalizedHealth ); // int normalizedHealth

	savefile->ReadObject( CastClassPtrRef(sheatheEnt) ); // idAnimated* sheatheEnt
	savefile->ReadObject( CastClassPtrRef(rodLight) ); // idLight * rodLight

	savefile->ReadInt( ftlTimer ); // int ftlTimer
	savefile->ReadInt( ftlState ); // int ftlState

	savefile->ReadInt( ftlPauseTime ); // int ftlPauseTime

	SaveFileReadArrayCast( smokeEmitters, ReadObject, idClass*& ); // idFuncEmitter *smokeEmitters[3]

	savefile->ReadObject( CastClassPtrRef(seamEmitter) ); // idFuncEmitter * seamEmitter

	savefile->ReadBool( initializedHealth ); // bool initializedHealth
	savefile->ReadInt( lastHealth ); // int lastHealth

	savefile->ReadObject( healthGUI ); // idEntityPtr<idEntity> healthGUI

	savefile->ReadInt( voiceoverIndex ); // int voiceoverIndex
	savefile->ReadInt( pipepauseTimer ); // int pipepauseTimer
	savefile->ReadBool( openAnimationStarted ); // bool openAnimationStarted
	savefile->ReadInt( outsideDamageTimer ); // int outsideDamageTimer
	savefile->ReadInt( additionalCountdownTime ); // int additionalCountdownTime
	savefile->ReadInt( defaultJumpTime ); // int defaultJumpTime
}

void idFTL::Think(void)
{
	const char *timeStr = gameLocal.ParseTimeMS(ftlTimer - gameLocal.time);

	idStaticEntity::Think();

	if (!initializedHealth)
	{
		initializedHealth = true;
		UpdateHealthGUI();
	}

	if (ftlState == FTL_IDLE)
	{
		if (gameLocal.time >= ftlTimer)
		{
			ftlState = FTL_DISPATCHDELAY;
			ftlTimer = 0;
		}
	}
	else if (ftlState == FTL_DISPATCHDELAY)
	{
		//Add a short delay so that the dispatch vo doesn't immediately happen on the same frame of when the enemy is killed, as that feels weird.
		if (gameLocal.time >= ftlTimer)
		{

			//StartSound("snd_dispatch", SND_CHANNEL_VOICE);
			StartSound("snd_vo_reboot", SND_CHANNEL_VOICE); //Sab2.0
			
			ftlTimer = gameLocal.time + FTL_DISPATCHTIME;
			ftlState = FTL_DISPATCH;


			//Ship switches to search state
			//if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState() == COMBATSTATE_IDLE)
			//{
			//	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GotoCombatSearchState();
			//}
		}
	}
	else if (ftlState == FTL_DISPATCH)
	{
		//Is currently playing the dispatch VO audio.

		if (gameLocal.time >= ftlTimer)
		{
			//Dispatch VO is done.

			ftlTimer = gameLocal.time + defaultJumpTime + additionalCountdownTime;
			ftlState = FTL_COUNTDOWN;

			//int totalSeconds = (int)((defaultJumpTime + additionalCountdownTime) / 1000.0f);
			//gameLocal.AddEventLog(idStr::Format("FTL rebooting: %d seconds.", totalSeconds), GetPhysics()->GetOrigin());

			//Event_SetGuiParm("gui_parm0", "CALCULATING TELEMETRY...");
			Event_GuiNamedEvent(1, "barsOn");
			voiceoverIndex = ReinitializeVOIndex(defaultJumpTime + additionalCountdownTime);			

			for (int i = 0; i < gameLocal.num_entities; i++)
			{
				if (!gameLocal.entities[i])
					continue;

				if (!gameLocal.entities[i]->IsType(idSabotageLever::Type))
					continue;

				static_cast<idSabotageLever *>(gameLocal.entities[i])->PostFTLReset();
			}


			
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->FTL_CountdownStarted();
		}
	}
	else if (ftlState == FTL_COUNTDOWN)
	{
		//Update gui.
		
		Event_SetGuiParm("gui_parm1", idStr::Format("%s", timeStr)); //The countdown.
		Event_SetGuiFloat("gui_parm2", 1.0f - ((ftlTimer - gameLocal.time) / (float)(defaultJumpTime + additionalCountdownTime))); //The progress bar.



		//update hud.
		float hudValue = 1.0f + ((ftlTimer - gameLocal.time) / 1000.0f);
		gameLocal.GetLocalPlayer()->hud->SetStateInt("ftlcountdown", (int)hudValue);


		//Update the voiceover announcer.
		if (voiceoverIndex == VO_90SEC)
		{
			if (ftlTimer - gameLocal.time <= 90000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_90sec");
				voiceoverIndex = VO_60SEC;
			}
		}
		if (voiceoverIndex == VO_60SEC)
		{
			if (ftlTimer - gameLocal.time <= 60000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_60sec");
				voiceoverIndex = VO_30SEC;
			}
		}
		//else if (voiceoverIndex == VO_45SEC)
		//{
		//	if (ftlTimer - gameLocal.time <= 46000)
		//	{
		//		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_45sec");
		//		voiceoverIndex = VO_30SEC;
		//	}
		//}
		else if (voiceoverIndex == VO_30SEC)
		{
			if (ftlTimer - gameLocal.time <= 30000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_30sec");
				voiceoverIndex = VO_20SEC;
			}
		}
		else if (voiceoverIndex == VO_20SEC)
		{
			if (ftlTimer - gameLocal.time <= 21000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_20sec");
				voiceoverIndex = VO_10SEC;
			}
		}
		else if (voiceoverIndex == VO_10SEC)
		{
			if (ftlTimer - gameLocal.time <= 10000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_10");
				voiceoverIndex = VO_9SEC;
			}
		}
		else if (voiceoverIndex == VO_9SEC)
		{
			if (ftlTimer - gameLocal.time <= 9000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_9");
				voiceoverIndex = VO_8SEC;
			}
		}
		else if (voiceoverIndex == VO_8SEC)
		{
			if (ftlTimer - gameLocal.time <= 8000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_8");
				voiceoverIndex = VO_7SEC;
			}
		}
		else if (voiceoverIndex == VO_7SEC)
		{
			if (ftlTimer - gameLocal.time <= 7000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_7");
				voiceoverIndex = VO_6SEC;
			}
		}
		else if (voiceoverIndex == VO_6SEC)
		{
			if (ftlTimer - gameLocal.time <= 6000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_6");
				voiceoverIndex = VO_5SEC;
			}
		}
		else if (voiceoverIndex == VO_5SEC)
		{
			if (ftlTimer - gameLocal.time <= 5000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_5");
				voiceoverIndex = VO_4SEC;
			}
		}
		else if (voiceoverIndex == VO_4SEC)
		{
			if (ftlTimer - gameLocal.time <= 4000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_4");
				voiceoverIndex = VO_3SEC;
			}
		}
		else if (voiceoverIndex == VO_3SEC)
		{
			if (ftlTimer - gameLocal.time <= 3000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_3");
				voiceoverIndex = VO_2SEC;
			}
		}
		else if (voiceoverIndex == VO_2SEC)
		{
			if (ftlTimer - gameLocal.time <= 2000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_2");
				voiceoverIndex = VO_1SEC;
			}
		}
		else if (voiceoverIndex == VO_1SEC)
		{
			if (ftlTimer - gameLocal.time <= 1000)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->Event_ActivateSpeakers("snd_ftl_1");
				voiceoverIndex = VO_DORMANT;
			}
		}

		

		if (gameLocal.time > ftlTimer)
		{
			//FTL countdown has ended. Start opening the sheathe.

			ftlTimer = gameLocal.time + OPEN_ANIMTIME;
			ftlState = FTL_OPENING;
			rodLight->On();
			sheatheEnt->Event_PlayAnim("open", 0);
			//SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_active")));

			smokeEmitters[0]->StartSound("snd_whistle", SND_CHANNEL_BODY, 0, false, NULL);
			
			seamEmitter->SetActive(false); //stop the smoke coming out of the seam.

			//update hud.
			gameLocal.GetLocalPlayer()->hud->SetStateString("ftlcountdown", "");

			
			//Make any Lifeboats leave.
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLifeboatsLeave();


			//int i;
			//openAnimationStarted = true;
			//
			////Start the opening animation NOW so that when the timer hits zero, the sheathe opens immediately.			
			//
			//for (i = 0; i < 3; i++)
			//{
			//	smokeEmitters[i]->SetModel("smokepuff03.prt");
			//	idEntityFx::StartFx("fx/smoke_ring08", &smokeEmitters[i]->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
			//}

			

			//this->spawnArgs.SetInt("baffleactive", BAFFLE_CAMOUFLAGED);
			//Event_SetGuiParm("gui_parm0", "SPOOLING UP...");
		}


		//FTL countdown is about to end. Logic for teleporting boss away.
		//if (ftlTimer - gameLocal.time <= BOSS_EXITTIME)
		//{
		//	if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->bossEnt.IsValid())
		//	{
		//		idEntity *boss = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->bossEnt.GetEntity();
		//		static_cast<idBossMonster *>(boss)->StartBossExitSequence();
		//	}
		//}


		//if (gameLocal.time > ftlTimer)
		//{
		//	int i;
		//
		//	//FTL sheathe is now open.
		//	ftlState = FTL_ACTIVE;
		//
		//	
		//
		//	//Event_SetGuiParm("gui_parm1", idStr::Format("%s", timeStr));
		//	//Event_SetGuiFloat("gui_parm2", 1.0f - ((ftlTimer - gameLocal.time) / (float)CHARGETIME));
		//
		//	for (i = 0; i < 3; i++)
		//	{
		//		//Make blue jets shoot out.
		//		smokeEmitters[i]->SetModel("ftl_jet.prt");
		//		idEntityFx::StartFx("fx/smoke_ring08", &smokeEmitters[i]->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		//	}
		//
		//	ftlTimer = gameLocal.time + ACTIVETIME;
		//	StopSound(SND_CHANNEL_AMBIENT, false);
		//	StartSound("snd_active", SND_CHANNEL_AMBIENT, 0, false, NULL);
		//
		//	Event_SetGuiParm("gui_parm0", "ENGAGED");
		//
		//	
		//	
		//
		//	//Activate slow mo.
		//	gameLocal.SetSlowmo(true);
		//}
	}
	else if (ftlState == FTL_OPENING)
	{
		//Currently playing the open sheathe animation.

		if (gameLocal.time > ftlTimer)
		{
			////The sheathe is now fully open.
			//ftlState = FTL_ACTIVE;
			//
			//ftlTimer = gameLocal.time + ACTIVETIME;
			//StopSound(SND_CHANNEL_AMBIENT, false);
			//StartSound("snd_active", SND_CHANNEL_AMBIENT, 0, false, NULL);
			//
			//Event_SetGuiParm("gui_parm0", "ENGAGED");
			//
			////Activate slow mo.
			//gameLocal.SetSlowmo(true);

			StartActiveState(); //Sab2.0
		}
	}
	else if (ftlState == FTL_PIPEPAUSE)
	{
		if (gameLocal.time > pipepauseTimer)
		{
			//pipe pause timer has expired.
			int delta = ftlTimer - ftlPauseTime;
			ftlTimer = gameLocal.time + delta;
			ftlState = FTL_COUNTDOWN;

			StartSound("snd_shipstartup", SND_CHANNEL_BODY3, 0, false, NULL);			
		}
	}
	else if (ftlState == FTL_ACTIVE)
	{
		//FTL is currently doing its ftl jump.

		//if (gameLocal.slow.time > outsideDamageTimer)
		//{
		//	outsideDamageTimer = gameLocal.slow.time + OUTSIDE_DAMAGE_INTERVAL;
		//
		//	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->StartFTLRescueSequence(); //if player is outside, then do the ftl rescue sequence.
		//}


		//if (gameLocal.time > ftlTimer)
		//{
		//	//FTL is done. Start closing it up.
		//	ftlState = FTL_CLOSEDELAY;
		//	ftlTimer = gameLocal.time + CLOSEDELAYTIME;
		//
		//	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->ResetEscalationLevel(); //This handles the post-FTL stuff, like sealing up windows and respawning skullsavers.
		//	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->DoUpgradeCargoUnlocks();
		//
		//	gameLocal.SetSlowmo(false);
		//
		//	gameLocal.GetLocalPlayer()->FlashScreen();
		//}
	}
	else if (ftlState == FTL_CLOSEDELAY)
	{

		if (gameLocal.time > ftlTimer)
		{
			Event_SetGuiParm("gui_parm1", idStr::Format("%s", timeStr));
			Event_SetGuiFloat("gui_parm2", 1.0f - ((ftlTimer - gameLocal.time) / (float)ACTIVETIME));

			
			StartSound("snd_jump", SND_CHANNEL_BODY2, 0, false, NULL);

			DoCloseSequence(); //This handles transition to FTL_CLOSING
		}
	}
	else if (ftlState == FTL_CLOSING)
	{
		Event_SetGuiParm("gui_parm1", idStr::Format("%s", timeStr));
		Event_SetGuiFloat("gui_parm2",  (ftlTimer - gameLocal.time) / (float)CLOSETIME);

		if (gameLocal.time > ftlTimer)
		{
			//Done. 
			rodLight->Off();
			StartIdle(); //Return to idle state.
		}
	}
	else if (ftlState == FTL_DAMAGED)
	{
		//when damaged, just kinda do nothing.
	}	
}

void idFTL::DoCloseSequence()
{
	int i;

	for (i = 0; i < 3; i++)
	{
		smokeEmitters[i]->SetModel("ftl_idlesmoke.prt");
		idEntityFx::StartFx("fx/smoke_ring08", &smokeEmitters[i]->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
	}

	ftlState = FTL_CLOSING;
	sheatheEnt->Event_PlayAnim("close", 0);
	StopSound(SND_CHANNEL_AMBIENT, false);
	StartSound("snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL);
	this->spawnArgs.SetInt("baffleactive", 0);
	ftlTimer = gameLocal.time + CLOSETIME;
	//Event_SetGuiParm("gui_parm0", "COOLING DOWN...");

	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_idle")));
}


void idFTL::DoRepairTick(int amount)
{
	health = min(health + amount, maxHealth);
	UpdateHealthGUI();

	if (health >= maxHealth)
	{
		//done with repair.
		needsRepair = false;
		healthGUI.GetEntity()->Event_GuiNamedEvent(1, "onFullyRepaired");

		SetSkin(declManager->FindSkin("skins/objects/ftl/default"));
		sheatheEnt->SetSkin(declManager->FindSkin("skins/objects/ftl/default"));

		DoCloseSequence();

		rodLight->SetColor(spawnArgs.GetVector("color_ftl", RODLIGHT_DEFAULTCOLOR_STR)); //Reset the rod light color.
	}
}

void idFTL::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	if ((ftlState == FTL_ACTIVE || ftlState == FTL_CLOSING || ftlState == FTL_DAMAGED) && materialType != SURFTYPE_METAL) //TODO: figure out why materialType returns the wrong value.
	{
		idStaticEntity::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);

		if (health < 0)
			health = 0; //Clamp health floor to zero
	}

	UpdateHealthGUI();

	if (health <= 0 && ftlState != FTL_DAMAGED)
	{
		int i;

		if (health < 0)
			health = 0;

		

		//All the effects that happen when the FTL enters damaged state.		
		needsRepair = true;
		healthGUI.GetEntity()->Event_GuiNamedEvent(1, "onNeedsRepairs");

		if (ftlState == FTL_CLOSING)
		{
			//If the sheathe was midway closed, then open it back it up again.
			sheatheEnt->Event_PlayAnim("open_fast", 0);
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_active")));
		}

		ftlState = FTL_DAMAGED;
		
		SetSkin(declManager->FindSkin("skins/objects/ftl/damaged"));
		sheatheEnt->SetSkin(declManager->FindSkin("skins/objects/ftl/damaged"));

		rodLight->SetColor(RODLIGHT_DAMAGECOLOR);

		for (i = 0; i < 3; i++)
		{
			smokeEmitters[i]->SetModel(spawnArgs.GetString("cache_particle01", "machine_damaged_smokeheavy2.prt"));
			idEntityFx::StartFx(spawnArgs.GetString("cache_fx01", "fx/smoke_ring08"), &smokeEmitters[i]->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		}		

		Event_SetGuiParm("gui_parm1", "");
		Event_SetGuiFloat("gui_parm2", 0);
		//Event_SetGuiParm("gui_parm0", "DAMAGED. OFFLINE.");
		
		idEntityFx::StartFx(spawnArgs.GetString("cache_fx02", "fx/spark_huge"), &rodLight->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		
		StopSound(SND_CHANNEL_BODY2, false);
		StopSound(SND_CHANNEL_AMBIENT, false);
		this->spawnArgs.SetInt("baffleactive", 0);
		StartSound("snd_shutdown", SND_CHANNEL_BODY, 0, false, NULL);
		StartSound("snd_brokenloop", SND_CHANNEL_AMBIENT, 0, false, NULL);
		
	}
}

void idFTL::UpdateHealthGUI()
{
	int healthAdjusted;
	
	if (health >= maxHealth)
	{
		healthAdjusted = 100;
	}
	else
	{
		//if health is not maxed, then clamp its value to 99 at most.
		healthAdjusted = min(99, (int)(((float)health / maxHealth) * 100.0f));

		if (health > 0)
			healthAdjusted = max(1, health); //always show at LEAST 1% health if health is greater than zero.
		else
			healthAdjusted = 0; //Clamp health to zero if health is less than or zero.
	}

	Event_SetGuiInt("hyperhealth", healthAdjusted);
	normalizedHealth = healthAdjusted;

	if (healthGUI.GetEntity())	
	{
		if (healthAdjusted < 100 && lastHealth > health)
		{
			healthGUI.GetEntity()->Event_GuiNamedEvent(1, "ondamage");
		}
		else if (health > lastHealth)
		{
			healthGUI.GetEntity()->Event_GuiNamedEvent(1, "onRepairTick");
		}

		lastHealth = health;

		healthGUI.GetEntity()->Event_SetGuiInt("gui_parm0", healthAdjusted);
	}
}

bool idFTL::IsJumpActive(bool includesCountdownAndCountdown, bool includesCountdown)
{
	if (includesCountdown)
		return (health > 0 && (ftlState == FTL_ACTIVE || ftlState == FTL_OPENING || ftlState == FTL_COUNTDOWN));
	else if (includesCountdownAndCountdown)
		return (health > 0 && (ftlState == FTL_DISPATCHDELAY || ftlState == FTL_DISPATCH || ftlState == FTL_ACTIVE || ftlState == FTL_OPENING || ftlState == FTL_COUNTDOWN));
	

	return (health > 0 && (ftlState == FTL_ACTIVE));
}


//return TRUE if it's paused.
bool idFTL::GetPipePauseState()
{
	return (ftlState == FTL_PIPEPAUSE);
}

//Tell the FTL to shut down due to sabotaged pipes.
void idFTL::SetPipePauseState()
{
	if (ftlState == FTL_COUNTDOWN)
	{
		pipepauseTimer = gameLocal.time + PIPE_PAUSE_MAXTIME;
		ftlPauseTime = gameLocal.time;
		ftlState = FTL_PIPEPAUSE;
	}
}

//For debug.
void idFTL::FastforwardCountdown()
{
    if (ftlState == FTL_COUNTDOWN)
    {
        ftlTimer = gameLocal.time + 1500;
    }
}

bool idFTL::AddToCountdown(int millisec)
{
	//if (ftlState == FTL_COUNTDOWN || ftlState == FTL_OPENING)
	//{
	//	//Valid!
	//}
	//else
	//{
	//	return false;
	//}

	//common->Printf("Adding time. %d\n", gameLocal.time);

	if (ftlState == FTL_OPENING)
	{
		//If we were midway opening, then close it. Be lenient on player here
		sheatheEnt->Event_PlayAnim("close", 4);
		ftlState = FTL_COUNTDOWN; //knock it back down to countdown state.
	}

	ftlTimer += millisec;
	additionalCountdownTime += millisec; //Sab2.0: this is the pool of total additional time we add to the countdown. Starts as zero, and accumulates as player sabotages more.

	//if (ftlTimer - gameLocal.time > MS_BETWEEN_JUMPS)
	//	ftlTimer = gameLocal.time + MS_BETWEEN_JUMPS; //Don't exceed the max timer amount. Prevent adding more time beyond the max allowed time.

	int deltaMS = ftlTimer - gameLocal.time;
	
	voiceoverIndex = ReinitializeVOIndex(deltaMS);

	return true;
}

//When the countdown is active and time gets added on, we need to reinitialize what VO index is currently valid/appropriate. Do that logic here.
int idFTL::ReinitializeVOIndex(int deltaMS)
{
	if (deltaMS >= 240000)		{ return VO_240SEC; }
	else if (deltaMS >= 210000) { return VO_210SEC; }
	else if (deltaMS >= 180000) { return VO_180SEC; }
	else if (deltaMS >= 120000) { return VO_120SEC; }
	else if (deltaMS >= 90000)	{ return VO_90SEC; }
	else if (deltaMS >= 60000)	{ return VO_60SEC; }
	else if (deltaMS >= 30000)	{ return VO_30SEC; }
	else if (deltaMS >= 20000)	{ return VO_20SEC; }
	else if (deltaMS >= 10000)	{ return VO_10SEC; }
	else if (deltaMS >= 9000)	{ return VO_9SEC; }
	else if (deltaMS >= 8000)	{ return VO_8SEC; }
	else if (deltaMS >= 7000)	{ return VO_7SEC; }
	else if (deltaMS >= 6000)	{ return VO_6SEC; }
	else if (deltaMS >= 5000)	{ return VO_5SEC; }
	else if (deltaMS >= 4000)	{ return VO_4SEC; }
	else if (deltaMS >= 3000)	{ return VO_3SEC; }
	else if (deltaMS >= 2000)	{ return VO_2SEC; }
	
	return VO_1SEC;
}

int idFTL::GetTotalCountdowntime()
{
	return defaultJumpTime + additionalCountdownTime;
}

void idFTL::Event_SetState(int value)
{
	if (value <= 0)
	{
		//turn it OFF and disable countdown.
		StartShutdown();
		ftlState = FTL_DORMANT;
	}
	else if (value == 1)
	{
		//turn it OFF and start countdown.
		StartShutdown();
	}
	else
	{
		//make it active IMMEDIATELY. (skips countdown)
		StartActiveState();
	}
}

void idFTL::Event_GetState()
{
	if (IsJumpActive(false, false))
		idThread::ReturnInt(1);
	else
		idThread::ReturnInt(0);
}