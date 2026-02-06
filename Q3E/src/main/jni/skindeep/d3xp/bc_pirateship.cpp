#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "Player.h"

#include "idlib/LangDict.h"
#include "bc_airlock_accumulator.h"
#include "SmokeParticles.h"
#include "ai/AI.h"
#include "bc_gunner.h"
#include "bc_meta.h"
#include "bc_frobcube.h"
#include "bc_airlock.h"
#include "bc_pirateship.h"

#define BOB_TIME 2.0f //how much seconds in the bob cycle
#define BOB_DIST 8.0f //distance that ship bobs up/down

#define SWAY_TIME 3.3f //sway cycle speed
#define SWAY_DIST 8.0f //distance that ship rolls

#define WIREWIDTH 7

#define HARPOONLENGTH 96 //distance from harpoon model 0,0,0 origin point to cable connection point.

#define SHIP_HALFLENGTH 128 //the midpoint of the pirateship

#define DOCK_SPLINETIME "10" //time to move on spline. In seconds.
#define DOCK_ROTATIONTIME 2 //time it takes to rotate into position.
#define DOCK_MOVEDOCKTIME 1 //after rotating, do a linear move to smack boarding ship's butt into the airlock.

#define DOCK_DOORBREACHDELAYTIME 3000 //after boarding ship docks, how long before airlock doors slam open.
#define SPAWN_DELAYTIME 3000

#define DOOR_FROBINDEX 1

#define SMOKEGRENADE_STATEDELAY		1000
#define SMOKEGRENADE_TOTAL      3

#define PORTAL_FADETIME 9000 //how long does the ftl triangle effect stay onscreen
#define PORTAL_ALPHAPARM 3

#define DOCK_OVERLAP_AMOUNT 7 //we need the ship's butt to overlap into the airlock a little so that we can visportal it shut.

#define BOARDINGCREWONBOARD_MESSAGETIME 5000 //how long to display message 'boarding crew is onboard


#define REINFORCEMENT_ENEMYSPAWNINTERVAL 4000 //delay between pirates spawning in the airlock.

#define EXTENDEDCOMBATSTATE_DURATION 60000

CLASS_DECLARATION(idMover, idPirateship)
END_CLASS

//TODO: Make grappling hooks shoot out and attach to ship before docking with airlock door

idPirateship::idPirateship(void)
{
	dynatipEnt = NULL;	
	smoketrailFlyTime = 0;
	smoketrail = NULL;
	portalActive = false;
	portalFadeTimer = 0;

	countdownStartTime = 0;
	countdownValue = 0;

	keylocationTimer = 0;
	
}

idPirateship::~idPirateship(void)
{
	if (wireOrigin)
		delete this->wireOrigin;

	if (wireTarget)
		delete this->wireTarget;
}

void idPirateship::Spawn(void)
{
	wireOrigin = NULL;
	wireTarget = NULL;

	fl.takedamage = true;

	////Bobbing motion
	//idVec3 bobAmount;
	//bobAmount.x = 0;
	//bobAmount.y = 0;
	//bobAmount.z = BOB_DIST;
	//Event_Bob(BOB_TIME, 0, bobAmount);
    //
	////Swaying motion
	//idAngles swayAmount;
	//swayAmount.pitch = 0;
	//swayAmount.yaw = 0;
	//swayAmount.roll = SWAY_DIST;
	//Event_Sway(SWAY_TIME, 0, swayAmount);
	

    //spawn frob cube.
    idDict args;
    idVec3 forwardDir, rightDir, upDir;
    this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);
    idVec3 frobPos = GetPhysics()->GetOrigin() + (forwardDir * -128) + (rightDir * 24);    
    args.Clear();
    args.Set("model", "models/objects/frobcube/cube4x4.ase");
    args.Set("displayname", "#str_def_gameplay_900145");
    openFrobcube = gameLocal.SpawnEntityType(idFrobcube::Type, &args);
    openFrobcube->SetOrigin(frobPos);
    openFrobcube->GetPhysics()->GetClipModel()->SetOwner(this);
    static_cast<idFrobcube*>(openFrobcube)->SetIndex(DOOR_FROBINDEX);
    openFrobcube->Bind(this, false);
	openFrobcube->Hide(); //BC 3-21-2025: hide the frobcube in dormant state.


	//Spawn the particle emitters.
	for (int i = 0; i < 2; i++)
	{
		idVec3 particlePos = GetPhysics()->GetOrigin() + (forwardDir * -106) + (upDir * -78) + (rightDir * (i <= 0 ? -56 : 56));		

		idAngles particleAngle;
		particleAngle = GetPhysics()->GetAxis().ToAngles();
		particleAngle.pitch += 90;
		particleAngle.yaw += 180;
		idDict args;
		args.Clear();
		args.Set("model", spawnArgs.GetString("model_boosterprt"));
		args.SetVector("origin", particlePos);
		args.SetMatrix("rotation", particleAngle.ToMat3());
		//args.SetBool("start_off", true);
		boosterEmitter[i] = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
		boosterEmitter[i]->Bind(this, true);
		boosterEmitter[i]->SetActive(false);
		boosterEmitter[i]->Hide();
	}


	PostEventMS(&EV_PostSpawn, 0);

	BecomeActive(TH_THINK);

    moveStartTime = 0;
    state = PSH_DORMANT;
    Hide();

    smokegrenadeTimer = 0;
    smokegrenadeCounter = 0;

	spawnIntervalTimer = 0;
	spawnIndex = 0;

	//spawn the portal.
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_portal"));
	args.SetBool("noshadows", true);
	args.SetBool("hide", true);
	ftlPortal = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
}

void idPirateship::Event_PostSpawn(void)
{
    airlockEnt = FindMyAirlock();
    if (!airlockEnt.IsValid())
    {
        gameLocal.Warning("pirateship '%s' has no assigned airlock. Ship must target an airlock.\n", GetName());
        return;
    }

	//Set up the spawn list.
	for (int i = 0; i < targets.Num(); i++)
	{
		if (!targets[i].IsValid())
			continue;

		if (targets[i].GetEntity()->IsType(idAI::Type))
		{
			spawnList.Append(targets[i].GetEntityNum());
		}
	}


	

	//Set up the harpoon stuff.
	//if (IsHidden())
	//	return;
    //
	//if (targets.Num() <= 0)
	//{
	//	gameLocal.Warning("Pirateship '%s' has no harpoon target.", GetName());
	//	return;
	//}
    //
	//if (targets.Num() > 0)
	//{
	//	if (targets[0].GetEntity() == NULL)
	//	{
	//		gameLocal.Error("Pirateship '%s' has invalid harpoon target.", GetName());
	//		return;
	//	}
    //
	//	idVec3 harpoonTargetPos = targets[0].GetEntity()->GetPhysics()->GetOrigin(); //where harpoon model will be.
	//	idVec3 forward, right;
	//	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, NULL);
	//	idVec3 harpoonShotOrigin = GetPhysics()->GetOrigin() + (forward * 128) + (right * -16); //where the harpoon fictionally 'shoots' from.
    //
	//	//Spawn decal.
	//	#define DECALSIZE 56.0f
	//	trace_t tr;
	//	gameLocal.clip.TracePoint(tr, harpoonShotOrigin, harpoonTargetPos, MASK_SOLID, NULL);
	//	gameLocal.ProjectDecal(tr.endpos, -tr.c.normal, 8.0f, true, DECALSIZE, "textures/decals/spearcrack"); //decal fx
    //
	//	
    //
	//	
	//	//Spawn the harpoon model.
	//	idAngles harpoonAngle = (harpoonTargetPos - harpoonShotOrigin).ToAngles();
	//	idEntity *harpoonModel;
	//	idDict args;
	//	args.Set("classname", "func_static");
	//	args.Set("model", "models/objects/pirateship/grapple/grapple.ase");
	//	args.SetVector("origin", harpoonTargetPos);
	//	args.SetMatrix("rotation", harpoonAngle.ToMat3());
	//	gameLocal.SpawnEntityDef(args, &harpoonModel);
	//	
    //
	//	
    //
	//	//Spawn the cable.
	//	args.Clear();
	//	args.SetVector("origin", harpoonShotOrigin);
	//	args.SetFloat("width", WIREWIDTH);
	//	this->wireTarget = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	//	//this->wireTarget->BecomeActive(TH_PHYSICS);
	//	this->wireTarget->Bind(this, false);
    //
	//	args.Clear();
	//	args.Set("target", wireTarget->name.c_str());
	//	args.SetBool("start_off", false);
	//	args.SetVector("origin", harpoonTargetPos + (harpoonAngle.ToForward() * -HARPOONLENGTH));
	//	args.SetFloat("width", WIREWIDTH);
	//	args.Set("skin", "skins/beam_harpoon");
	//	this->wireOrigin = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	//	//this->wireOrigin->BecomeActive(TH_PHYSICS);
	//	this->wireOrigin->Bind(harpoonModel, false);
    //
    //
    //
	//}
}

idEntity * idPirateship::FindMyAirlock()
{
    if (targets.Num() <= 0)
    {
        gameLocal.Warning("pirateship '%s' has no assigned airlock.\n", GetName());
        return NULL;
    }

    for (int i = 0; i < targets.Num(); i++)
    {
        if (!targets[i].IsValid())
        {
            gameLocal.Warning("pirateship '%s' has invalid target.\n", GetName());
            return NULL;
        }

        if (targets[i].GetEntity()->IsType(idAirlock::Type))
        {
            return targets[i].GetEntity();
        }
    }

    return NULL;
}

void idPirateship::Save(idSaveGame *savefile) const
{
	SaveFileWriteArray( boosterEmitter, 2, WriteObject ); // idFuncEmitter *boosterEmitter[2]

	savefile->WriteObject( wireOrigin ); // idBeam* wireOrigin
	savefile->WriteObject( wireTarget ); // idBeam* wireTarget

	savefile->WriteInt( state ); // int state

	savefile->WriteInt( moveStartTime ); // int moveStartTime


	savefile->WriteObject( airlockEnt ); // idEntityPtr<idEntity> airlockEnt

	savefile->WriteObject( openFrobcube ); // idEntity * openFrobcube

	savefile->WriteInt( smokegrenadeCounter ); // int smokegrenadeCounter
	savefile->WriteInt( smokegrenadeTimer ); // int smokegrenadeTimer

	savefile->WriteInt( spawnIndex ); // int spawnIndex
	savefile->WriteInt( spawnIntervalTimer ); // int spawnIntervalTimer
	SaveFileWriteArray( spawnList, spawnList.Num(), WriteInt ); // idList<int> spawnList

	savefile->WriteParticle( smoketrail ); // const idDeclParticle * smoketrail
	savefile->WriteInt( smoketrailFlyTime ); // int smoketrailFlyTime

	savefile->WriteObject( ftlPortal ); // idEntity * ftlPortal
	savefile->WriteInt( portalFadeTimer ); // int portalFadeTimer
	savefile->WriteBool( portalActive ); // bool portalActive

	savefile->WriteInt( countdownValue ); // int countdownValue
	savefile->WriteInt( countdownStartTime ); // int countdownStartTime

	savefile->WriteInt( keylocationTimer ); // int keylocationTimer
	savefile->WriteObject( dynatipEnt ); // idEntity * dynatipEnt
}

void idPirateship::Restore(idRestoreGame *savefile)
{
	SaveFileReadArrayCast( boosterEmitter, ReadObject, idClass*& ); // idFuncEmitter *boosterEmitter[2]

	savefile->ReadObject( CastClassPtrRef(wireOrigin) ); // idBeam* wireOrigin
	savefile->ReadObject( CastClassPtrRef(wireTarget) ); // idBeam* wireTarget

	savefile->ReadInt( state ); // int state

	savefile->ReadInt( moveStartTime ); // int moveStartTime


	savefile->ReadObject( airlockEnt ); // idEntityPtr<idEntity> airlockEnt

	savefile->ReadObject( openFrobcube ); // idEntity * openFrobcube

	savefile->ReadInt( smokegrenadeCounter ); // int smokegrenadeCounter
	savefile->ReadInt( smokegrenadeTimer ); // int smokegrenadeTimer

	savefile->ReadInt( spawnIndex ); // int spawnIndex
	savefile->ReadInt( spawnIntervalTimer ); // int spawnIntervalTimer
	SaveFileReadList( spawnList, ReadInt ); // idList<int> spawnList

	savefile->ReadParticle( smoketrail ); // const idDeclParticle * smoketrail
	savefile->ReadInt( smoketrailFlyTime ); // int smoketrailFlyTime

	savefile->ReadObject( ftlPortal ); // idEntity * ftlPortal
	savefile->ReadInt( portalFadeTimer ); // int portalFadeTimer
	savefile->ReadBool( portalActive ); // bool portalActive

	savefile->ReadInt( countdownValue ); // int countdownValue
	savefile->ReadInt( countdownStartTime ); // int countdownStartTime

	savefile->ReadInt( keylocationTimer ); // int keylocationTimer
	savefile->ReadObject( dynatipEnt ); // idEntity * dynatipEnt
}

void idPirateship::Think(void)
{
	idMover::Think();



	if (portalActive)
	{
		float lerp = (portalFadeTimer - gameLocal.time)  / (float)PORTAL_FADETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		ftlPortal->SetShaderParm(PORTAL_ALPHAPARM, lerp);

		//the portal follows the ship on the X and Z axes. The Y remains constant.
		ftlPortal->SetOrigin(idVec3(GetPhysics()->GetOrigin().x, ftlPortal->GetPhysics()->GetOrigin().y, GetPhysics()->GetOrigin().z));
		
		if (gameLocal.time >= portalFadeTimer)
		{
			ftlPortal->PostEventMS(&EV_Remove, 0);
			ftlPortal = nullptr;
			portalActive = false;
		}
	}

    if (state == PSH_SPLINEMOVING)
    {
		UpdateCountdownText();

		if (smoketrail != NULL && smoketrailFlyTime && !IsHidden())
		{
			idVec3 forward, up;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
			idVec3 particlePos = GetPhysics()->GetOrigin() + (forward * -128) + (up * -78);

			idAngles particleAngle = GetPhysics()->GetAxis().ToAngles();			
			particleAngle.pitch = 90;			
			particleAngle.yaw += 180;

			if (!gameLocal.smokeParticles->EmitSmoke(smoketrail, smoketrailFlyTime, gameLocal.random.RandomFloat(), particlePos, particleAngle.ToMat3(), timeGroup))
			{
				smoketrailFlyTime = gameLocal.time;
			}
		}

        if (gameLocal.time >= moveStartTime + (spawnArgs.GetFloat("splinetime", DOCK_SPLINETIME) * 1000))
        {
            //Done with spline. Start rotating butt toward airlock door.
            idVec3 currentPos = GetPhysics()->GetOrigin(); //This is a hack.... stopspline makes the entity do a position snap
            Event_StopSpline();
            SetOrigin(currentPos);
            state = PSH_DOCKROTATING;

            if (!airlockEnt.IsValid())
            {
                gameLocal.Warning("pirateship '%s' has invalid airlock.\n", GetName());
                return;
            }

            idAngles airlockAngles = airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles();
            Event_SetMoveTime(DOCK_ROTATIONTIME);
            Event_SetAccellerationTime(1);
            Event_SetDecelerationTime(1);
            Event_RotateTo(airlockAngles);        
            moveStartTime = gameLocal.time;

			
			
			StartSound("snd_vo_airlockbreach", SND_CHANNEL_VOICE);
			StartSound("snd_move", SND_CHANNEL_ANY);


			for (int i = 0; i < 2; i++)
			{
				boosterEmitter[i]->SetActive(false);
			}

			BecomeInactive(TH_UPDATEPARTICLES);

			SetCountdownStr("#str_def_gameplay_boarding_docking");

			if (dynatipEnt != NULL)
			{
				dynatipEnt->PostEventMS(&EV_Remove, 0);
				dynatipEnt = nullptr;
			}
        }
    }
    else if (state == PSH_DOCKROTATING)
    {
		//Ship is rotating to face the airlock.
		UpdateCountdownText();

        if (gameLocal.time >= moveStartTime + (DOCK_ROTATIONTIME * 1000))
        {
            //done rotating. Connect butt with airlock.

            if (!airlockEnt.IsValid())
            {
                gameLocal.Warning("pirateship '%s' has invalid airlock.\n", GetName());
                return;
            }

            state = PSH_DOCKMOVING;

            //find dock position.
            int airlockOff = airlockEnt.GetEntity()->spawnArgs.GetFloat("exteriorbuttonoffset");
            idVec3 airlockForward = airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToForward();
            idVec3 airlockDockPos = airlockEnt.GetEntity()->GetPhysics()->GetOrigin() + (airlockForward * (airlockOff + SHIP_HALFLENGTH - DOCK_OVERLAP_AMOUNT));
            
            float verticalOffset = airlockEnt.GetEntity()->spawnArgs.GetFloat("airlockheight");
            airlockDockPos += idVec3(0, 0, verticalOffset / 2);
            Event_SetMoveTime(DOCK_MOVEDOCKTIME);
            Event_SetAccellerationTime(0.4f);
            Event_SetDecelerationTime(0);
            Event_MoveToPos(airlockDockPos);
            moveStartTime = gameLocal.time;

			StartSound("snd_move", SND_CHANNEL_ANY);


			//At this time, we unlock the trash chutes and windows.
			//This is to prevent situations where the player gets trapped outside.
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableTrashchutes(true);
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableWindows(true);
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableDoorBarricades();
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableAirlocks(true);

			//Disable the accumulators.
			if (airlockEnt.IsValid())
			{
				#define DISTANCE_TO_OUTSIDEAIRLOCK 144
				idVec3 airlockforward = GetPhysics()->GetAxis().ToAngles().ToForward();
				idVec3 pointOutsideAirlock = airlockEnt.GetEntity()->GetPhysics()->GetOrigin() + idVec3(0, 0, 16) + (airlockForward * -DISTANCE_TO_OUTSIDEAIRLOCK);

				//find what room the accumulators are in
				idLocationEntity *locationEnt = gameLocal.LocationForPoint(pointOutsideAirlock);
				if (!locationEnt)
				{
					gameLocal.Warning("Pirateship '%s' unable to find location entity for airlock's neighboring room.", GetName());
				}
				else
				{
					//find the accumulators in this room.
					DestroyAccumulatorsInRoom(locationEnt);
				}

				static_cast<idAirlock *>(airlockEnt.GetEntity())->SetCanEmergencyPurge(false);				
			}
			
        }
    }
	else if (state == PSH_DOCKMOVING)
	{
		//Ship is moving its butt toward airlock.
		UpdateCountdownText();

		if (gameLocal.time >= moveStartTime + (DOCK_MOVEDOCKTIME * 1000))
		{
			//Ship has successfully connected its butt into the airlock.
            StopSound(SND_CHANNEL_AMBIENT); //stop the music.

			state = PSH_DOORBREACHDELAY;
			moveStartTime = gameLocal.time;
			StartSound("snd_thump", SND_CHANNEL_ANY);

			// SW 5th March 2025:
			// Snap ship to airlock to avoid discrepancies caused by occasional mover wonkiness
			// We need to find the dock position again and then SetOrigin()
			int airlockOff = airlockEnt.GetEntity()->spawnArgs.GetFloat("exteriorbuttonoffset");
			idVec3 airlockForward = airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToForward();
			idVec3 airlockDockPos = airlockEnt.GetEntity()->GetPhysics()->GetOrigin() + (airlockForward * (airlockOff + SHIP_HALFLENGTH - DOCK_OVERLAP_AMOUNT));

			float verticalOffset = airlockEnt.GetEntity()->spawnArgs.GetFloat("airlockheight");
			airlockDockPos += idVec3(0, 0, verticalOffset / 2);
			Event_StopMoving();
			SetOrigin(airlockDockPos);

			//particle effect.			
			idVec3 forward = GetPhysics()->GetAxis().ToAngles().ToForward();
			idVec3 particlePos = GetPhysics()->GetOrigin() + forward * -SHIP_HALFLENGTH;
			idAngles particleAngle = GetPhysics()->GetAxis().ToAngles();
			particleAngle.pitch -= 90;
			gameLocal.DoParticle(spawnArgs.GetString("model_prtthump"), particlePos, particleAngle.ToForward());

			//Tell airlock to ignore button presses. We do this so that the airlock doesn't get confused between the boarding sequence and player interrupting it.
			static_cast<idAirlock *>(airlockEnt.GetEntity())->StartBoardingSequence();

			gameLocal.AddEventLog("#str_def_gameplay_boarding_breaching", GetPhysics()->GetOrigin(), false);
			SetCountdownStr("#str_def_gameplay_boarding_breaching");



			// SW 5th March 2025: changed to coplanar so there's no possibility of us accidentally using the door portal
			// Also changed how the butt position is calculated to avoid ambiguity
			// 
			// Handle the visportal. We expect a visportal near the outer edge of the airlock. It should be coplanar with the airlock's exterior edge. 
			idVec3 buttPos = airlockEnt.GetEntity()->GetPhysics()->GetOrigin() + (airlockForward * airlockOff);
			idBounds shipButtBounds = idBounds(buttPos).Expand(1);

			if (developer.GetBool())
			{
				gameRenderWorld->DebugBounds(idVec4(1, 1, 0, 1), shipButtBounds, vec3_origin, 10000);
			}

			qhandle_t portal = gameRenderWorld->FindPortal(shipButtBounds);
			if (portal)
			{
				gameLocal.SetPortalState(portal, PS_BLOCK_VIEW);
				gameLocal.Printf("Pirateship '%s' closing visportal.\n", GetName());
			}
			else
			{
				gameLocal.Warning("pirateship '%s' failed to find a visportal to close. This will cause vis problems.\n", GetName());
			}
		}
	}
	else if (state == PSH_DOORBREACHDELAY)
	{
		UpdateCountdownText();

		if (gameLocal.time >= moveStartTime + DOCK_DOORBREACHDELAYTIME)
		{
			if (!airlockEnt.IsValid())
			{
				gameLocal.Warning("pirateship '%s' has invalid airlock.\n", GetName());
				return;
			}

			if (!airlockEnt.GetEntity()->IsType(idAirlock::Type))
			{
				gameLocal.Warning("pirateship '%s' has invalid airlock.\n", GetName());
				return;
			}

			//Force the airlock doors open.
			static_cast<idAirlock *>(airlockEnt.GetEntity())->StartBoardingDoorOpen();

			state = PSH_SMOKEGRENADE_DELAY;
            moveStartTime = gameLocal.time;			
		}
	}
    else if (state == PSH_SMOKEGRENADE_DELAY)
    {
		UpdateCountdownText();

        if (gameLocal.time > moveStartTime + SMOKEGRENADE_STATEDELAY)
        {
            state = PSH_SMOKEGRENADE_LAUNCHING;
            smokegrenadeTimer = gameLocal.time;
            smokegrenadeCounter = 0;

			gameLocal.GetLocalPlayer()->hud->SetStateString("reinforcementcountdown", "");
        }
    }
    else if (state == PSH_SMOKEGRENADE_LAUNCHING)
    {
        if (gameLocal.time > smokegrenadeTimer)
        {
            smokegrenadeTimer = gameLocal.time + 1000;

            LaunchSmokeGrenade();

            smokegrenadeCounter++;
            if (smokegrenadeCounter >= SMOKEGRENADE_TOTAL)
            {
				//All done with smoke grenades.
                state = PSH_SPAWNDELAY;
				moveStartTime = gameLocal.time + SPAWN_DELAYTIME;
            }
        }
    }
	else if (state == PSH_SPAWNDELAY)
	{
		if (gameLocal.time > moveStartTime)
		{
			// SW 13th Feb 2025: Adding toggle so we can turn off the swordfish in the VR tutorial
			if (spawnArgs.GetBool("doSwordfish", "1"))
			{
				state = PSH_SPAWNFISH;
				spawnIntervalTimer = gameLocal.time + 500; //a little delay after sound fx + before first spawn.
			}
			else
			{
				// jump straight to spawning bad guys
				state = PSH_SPAWNING;
				spawnIntervalTimer = gameLocal.time + 3000; //delay before enemy spawn.
			}
			
			

			StartSound("snd_dooropen", SND_CHANNEL_ANY);
		}
	}
	else if (state == PSH_SPAWNFISH)
	{
		if (gameLocal.time > spawnIntervalTimer)
		{
			//Spawn the fish.
			idVec3 fishSpawnPos = GetAirlockClearPosition();
			if (fishSpawnPos != vec3_zero)
			{
				//found a spawn position for the fish.
				idDict args;
				idEntity* petEnt;
				args.Clear();
				args.SetVector("origin", fishSpawnPos + idVec3(0,0,30));
				args.SetFloat("angle", airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().yaw + 179);
				args.Set("classname", spawnArgs.GetString("def_pet", "monster_spearbot"));
				gameLocal.SpawnEntityDef(args, &petEnt);

				//if (petEnt)
				//{
				//	idAngles pushDir = airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles();
				//	pushDir.yaw += 179;
				//
				//	//give fish a push
				//	petEnt->GetPhysics()->SetLinearVelocity(pushDir.ToForward() * 1024);
				//}


				//Continue to enemy spawn.
				state = PSH_SPAWNING;
				spawnIntervalTimer = gameLocal.time + 3000; //delay before enemy spawn.
			}
			else
			{
				//Failed to find fish spawn position.
				spawnIntervalTimer = gameLocal.time + 100;
			}			
		}
	}
	else if (state == PSH_SPAWNING)
	{
		if (gameLocal.time > spawnIntervalTimer)
		{
			spawnIntervalTimer = gameLocal.time + REINFORCEMENT_ENEMYSPAWNINTERVAL;

			//Spawn the person.
			if (spawnList.Num() > 0) //if list is not empty....
			{
				int personIdx = spawnList[spawnIndex];
				if (SpawnPersonInAirlockViaEntNum(personIdx))
				{
					spawnIndex++;
				}
				else
				{
					spawnIntervalTimer = gameLocal.time + 100; //failed to spawn. Try again after a very short delay.
				}
			}

			if (spawnIndex >= spawnList.Num())
			{
				//Final reinforcement pirates has spawned.
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetReinforcementPiratesAllSpawned();

				state = PSH_DONESPAWNING;
				StartSound("snd_doorclose", SND_CHANNEL_ANY);
				moveStartTime = gameLocal.time + BOARDINGCREWONBOARD_MESSAGETIME;

				SetCountdownStr("#str_def_gameplay_boarding_onboard");
			}
		}
	}
	else if (state == PSH_DONESPAWNING)
	{
		if (gameLocal.time > moveStartTime)
		{
			state = PSH_REINFORCEMENTMESSAGEDONE;
			gameLocal.GetLocalPlayer()->hud->SetStateBool("showreinforcements", false);
		}
	}
	else if (state == PSH_REINFORCEMENTMESSAGEDONE)
	{
		UpdateKeyLocation();
	}
}

void idPirateship::UpdateKeyLocation()
{
	if (keylocationTimer > gameLocal.time)
		return;

	if (!gameLocal.InPlayerPVS(this))
	{
		keylocationTimer = gameLocal.time + 1000;
		return;
	}

	keylocationTimer = gameLocal.time + 300;

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (idStr::Icmp(ent->spawnArgs.GetString("inv_name"), "shipkey") == 0)
		{
			//found the key.
			idLocationEntity *locEnt = gameLocal.LocationForEntity(ent);
			if (!locEnt)
				break;

			idStr locName = locEnt->GetLocation();

			idVec3 forwardDir, rightDir, upDir;
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);
			idVec3 pirateshipButtPos = GetPhysics()->GetOrigin() + forwardDir * -128;			
			idVec3 keyPos = ent->GetPhysics()->GetOrigin();
			float distance = (pirateshipButtPos - keyPos).Length();
			distance *= DOOM_TO_METERS;

			Event_SetGuiParm("keylocationtext", idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_pirateship_keylocation"), locName.c_str(), distance)); /*KEY LOCATION:\n%s\nKEY DISTANCE:\n%.1f meters*/

			return;
		}		
	}

	//failed to find key.
	
	//Check if key is inside lost and found machine.
	if (gameLocal.GetLocalPlayer()->IsEntityLostInSpace("item_securitycard_shipkey"))
	{
		Event_SetGuiParm("keylocationtext", "#str_def_gameplay_pirateship_keylostfound"); /*KEY LOCATION:\nLost and Found machine*/
		return;
	}

	Event_SetGuiParm("keylocationtext", "#str_def_gameplay_pirateship_keyunknown");/*KEY LOCATION:\nUnknown*/
}

idVec3 idPirateship::GetAirlockClearPosition()
{
	//Try to find a clear spot to spawn person.
	if (!airlockEnt.IsValid())
		return vec3_zero;

	//Get the airlock bounds and shrink them a bit.
	idBounds airlockBounds = airlockEnt.GetEntity()->GetPhysics()->GetAbsBounds();
	airlockBounds = airlockBounds.Expand(-60);

	//Brute force a random spot within the airlock.
	idVec3 randomPos;
	randomPos.x = gameLocal.random.RandomInt(airlockBounds[0].x, airlockBounds[1].x);
	randomPos.y = gameLocal.random.RandomInt(airlockBounds[0].y, airlockBounds[1].y);
	randomPos.z = airlockEnt.GetEntity()->GetPhysics()->GetOrigin().z + 4;

	//Check if spot is valid to spawn on.
	idBounds personBounds = idBounds(idVec3(-14, -14, 0), idVec3(14, 14, 76));
	trace_t tr;
	gameLocal.clip.TraceBounds(tr, randomPos, randomPos, personBounds, MASK_SOLID | MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX, NULL);

	if (tr.fraction < 1)
		return vec3_zero; //space is not clear. Exit here.	

	return randomPos;
}

bool idPirateship::SpawnPersonInAirlockViaEntNum(int idx)
{
	//Attempt to spawn a person inside the airlock.
	idEntity *personEnt = gameLocal.entities[idx];
	if (personEnt == NULL)
		return false;

	idVec3 randomPos = GetAirlockClearPosition();

	if (randomPos == vec3_zero)
		return false;
	
	//idAngles spawnAngle = airlockEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles();
	//spawnAngle.yaw += 180;
	//personEnt->Teleport(randomPos, spawnAngle, NULL);
	personEnt->Show();
	personEnt->GetPhysics()->SetOrigin(randomPos);
	//personEnt->GetPhysics()->SetAxis(spawnAngle.ToMat3());

	if (personEnt->IsType(idAI::Type))
	{
		if (personEnt->targets.Num() > 0)
		{
			if (personEnt->targets[0].IsValid())
			{
				idVec3 pathPosition = personEnt->targets[0].GetEntity()->GetPhysics()->GetOrigin();
				static_cast<idAI *>(personEnt)->MoveToPosition(pathPosition);
				//gameRenderWorld->DebugArrow(colorOrange, personEnt->targets[0].GetEntity()->GetPhysics()->GetOrigin(), pathPosition, 8, 90000);
			}
		}	

		// SW 2nd April 2025: keeps the pirates from immediately greeting each other the moment they exit the pirate ship (funny, but not intentional)
		if (personEnt->IsType(idGunnerMonster::Type))
		{
			static_cast<idGunnerMonster*>(personEnt)->ResetCallresponseTimer();
		}
	}
	
	return true;
}

void idPirateship::LaunchSmokeGrenade()
{
    //launch a grenade.
    StartSound("snd_launchsmoke", SND_CHANNEL_ANY);

    idVec3 forwardDir, rightDir, upDir;
    this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);
    idVec3 launchPos = GetPhysics()->GetOrigin() + (forwardDir * -160) + (upDir * 40);


    idEntity *spawnedItem;
    idDict args;
    args.Clear();
    args.SetVector("origin", launchPos);
    args.Set("classname", spawnArgs.GetString("def_smokegrenade"));
    gameLocal.SpawnEntityDef(args, &spawnedItem, false);

    if (spawnedItem)
    {
        int launchPower = gameLocal.random.RandomInt(100, 350);
        int rightwardAmount = gameLocal.random.RandomInt(-80, 80);

        if (smokegrenadeCounter >= SMOKEGRENADE_TOTAL - 1) //final grenade is always max power.
            launchPower = 350;

        spawnedItem->GetPhysics()->SetLinearVelocity((forwardDir * -launchPower) + (rightDir * rightwardAmount));
        spawnedItem->GetPhysics()->SetAngularVelocity(idVec3(128, gameLocal.random.RandomInt(-128,128), 0));
    }
}

void idPirateship::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	
}

bool idPirateship::IsDormant()
{
    return (state == PSH_DORMANT);
}



void idPirateship::StartEntranceSequence()
{
    //sanity check: see if it has a target.    
    idEntity *splinePath = GetSplinePath();
    if (splinePath == NULL)
    {
        gameLocal.Warning("pirateship '%s' couldn't find a spline path.\n", GetName());
        return;
    }

    //hyperspace in.
    state = PSH_SPLINEMOVING;
    idVec3 splineStartPos = splinePath->GetPhysics()->GetOrigin();
    SetOrigin(splineStartPos);	
    Show();

	//Show the portal.
	ftlPortal->SetOrigin(splineStartPos);
	ftlPortal->SetAxis(GetPhysics()->GetAxis());
	ftlPortal->SetShaderParm(PORTAL_ALPHAPARM, .3f);
	ftlPortal->Show();
	portalFadeTimer = gameLocal.time + PORTAL_FADETIME;
	portalActive = true;

    Event_SetAccellerationTime(0);
    Event_SetDecelerationTime(1);
    Event_SetMoveTime(spawnArgs.GetFloat("splinetime", DOCK_SPLINETIME)); //in seconds.
    Event_DisableSplineAngles();
    Event_StartSpline(splinePath);
    moveStartTime = gameLocal.time;	

	StartSound("snd_piratemusic", SND_CHANNEL_AMBIENT);
	StartSound("snd_vo_reinforcements", SND_CHANNEL_VOICE);

	idEntityFx::StartFx(spawnArgs.GetString("fx_jump"), GetPhysics()->GetOrigin(), mat3_identity);

	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->AlertAIFriends(NULL); //activate combat state.
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetCombatDurationTime(EXTENDEDCOMBATSTATE_DURATION); //length of the extended combat state.

	for (int i = 0; i < 2; i++)
	{
		boosterEmitter[i]->Show();
		boosterEmitter[i]->SetActive(true);
	}

	smoketrail = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, spawnArgs.GetString("smoke_smoketrail")));
	smoketrailFlyTime = gameLocal.time;


	//Calculate how long the timer should be....
	countdownValue = (int)(spawnArgs.GetFloat("splinetime", DOCK_SPLINETIME) * 1000.0f);
	countdownValue += DOCK_ROTATIONTIME * 1000;
	countdownValue += DOCK_MOVEDOCKTIME * 1000;
	countdownValue += DOCK_DOORBREACHDELAYTIME;
	countdownValue += SMOKEGRENADE_STATEDELAY;
	countdownStartTime = gameLocal.time;
	

	gameLocal.AddEventLog("#str_def_gameplay_boarding_approach", GetPhysics()->GetOrigin(), false);
	gameLocal.GetLocalPlayer()->hud->SetStateBool("showreinforcements", true);
	SetCountdownStr("#str_def_gameplay_boarding_approach");


	//dynatip for pirateship
	idDict args;
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin()); //the postition doesn't really matter. the icon gets bound to the entity position in the dynatip logic.
	args.Set("classname", "func_dynatip");
	args.Set("text", "");
	args.Set("mtr_icon", spawnArgs.GetString("mtr_hinticon"));
	args.Set("target", this->GetName());
	//args.SetVector("drawOffset", idVec3(0, 0, 80));
	args.SetBool("force_on", true);
	gameLocal.SpawnEntityDef(args, &dynatipEnt);

	openFrobcube->Show(); //BC 3-21-2025: fixed bug where frob was available during dormant state.
}

void idPirateship::UpdateCountdownText()
{
	int deltaTime = (countdownStartTime + countdownValue) - gameLocal.time;
	idStr timeStr = gameLocal.ParseTimeMS_SecAndDecisec(deltaTime);
	gameLocal.GetLocalPlayer()->hud->SetStateString("reinforcementcountdown", timeStr.c_str());

	float lerpAmount = deltaTime / (float)countdownValue;
	lerpAmount = idMath::ClampFloat(0, 1, lerpAmount);
	lerpAmount = 1 - lerpAmount;
	gameLocal.GetLocalPlayer()->hud->SetStateFloat("reinforcementmeter", lerpAmount);
	
}

void idPirateship::SetCountdownStr(idStr text)
{
	gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("reinforcementsUpdate");
	gameLocal.GetLocalPlayer()->hud->SetStateString("reinforcementtext", common->GetLanguageDict()->GetString( text.c_str()));	
}


idEntity * idPirateship::GetSplinePath()
{
    if (targets.Num() <= 0)
    {
        gameLocal.Warning("pirateship '%s' has no spline target.\n", GetName());
        return NULL;
    }
	
	float bestDotProduct = -999;
	int bestSplineIdx = -1;
    for (int i = 0; i < targets.Num(); i++)
    {
        if (!targets[i].IsValid())
        {
            gameLocal.Warning("pirateship '%s' has invalid target.\n", GetName());
            return NULL;
        }

        if (targets[i].GetEntity()->IsType(idSplinePath::Type))
        {
            //return targets[i].GetEntity();

			idVec3 splineStartPos = targets[i].GetEntity()->GetPhysics()->GetOrigin();

			idVec3 dirToSplineStart = splineStartPos - gameLocal.GetLocalPlayer()->GetEyePosition();
			dirToSplineStart.Normalize();
			float facingResult = DotProduct(dirToSplineStart, gameLocal.GetLocalPlayer()->viewAngles.ToForward());

			if (facingResult > bestDotProduct)
			{
				bestDotProduct = facingResult;
				bestSplineIdx = i;
			}
        }
    }

	if (bestSplineIdx < 0)
	{
		gameLocal.Error("Pirateship '%s' failed in GetSplinePath().", GetName());
	}


	return targets[bestSplineIdx].GetEntity();
}

bool idPirateship::DoFrob(int index, idEntity * frobber)
{
    if (index == DOOR_FROBINDEX)
    {
        //trying to open the door.

		if (frobber != nullptr)
		{
			if (frobber == gameLocal.GetLocalPlayer())
			{
				if (gameLocal.RequirementMet_Inventory(frobber, spawnArgs.GetString("requires"), 1))
				{
					//player has the required item. Successs.
					openFrobcube->isFrobbable = false;
					static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->StartPostGame();
					return true;
				}
			}
		}
        
        //player does not have the required item. Fail.
        Event_GuiNamedEvent(1, "noKey");
		return false;        
    }

    return true;
}


void idPirateship::DebugFastForward()
{
	if (IsHidden())
		return;

	if (state != PSH_SPLINEMOVING)
		return;

	moveStartTime = -999999999; //snap it to the docking sequence.
}

void idPirateship::DestroyAccumulatorsInRoom(idLocationEntity *locationEnt)
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idAirlockAccumulator::Type))
			continue;		

		if (static_cast<idAirlockAccumulator*>(ent)->IsDeflated())
			continue;

		//Ensure accumulator is in the same room.
		idLocationEntity* accumLocation = gameLocal.LocationForEntity(ent);
		if (accumLocation != nullptr && locationEnt != nullptr)
		{
			if (accumLocation->entityNumber != locationEnt->entityNumber)
			{
				continue;
			}
		}
		
		//ent->DoFrob();
		//Blow up the accumulator.
		ent->Damage(this, this, vec3_zero, "damage_1000", 1.0f, 0);		
	}
}

void idPirateship::Event_PartBlocked(idEntity* blockingEntity)
{
	// SW 3rd June 2025: Handle situation where player decides to position themselves between the ship and the dock
	// (I can't fault them really, I'd try the same thing)
	if (blockingEntity->IsType(idPlayer::Type))
	{
		if (!static_cast<idPlayer*>(blockingEntity)->inDownedState)
		{
			blockingEntity->Damage(this, this, vec3_zero, "damage_1000", 1.0f, 0);
		}
		
		// The primary case here is when the ship is moving towards the dock. Since ships dock horizontally, we can try moving the player up, down, or laterally to the direction of travel
		idVec3 moveDir = this->move.dir;
		if (moveDir != vec3_zero)
		{
			idVec3 forward, right, up;
			moveDir.ToAngles().ToVectors(&forward, &right, &up);
			idVec3 shipOrigin = this->GetPhysics()->GetOrigin();
			idBounds playerBounds = blockingEntity->GetPhysics()->GetBounds();

			// Remember that candidate positions are based on the direction of travel (hardcoded based on ship size, ewww)
			idVec3 candidatePositions[8] = {
				shipOrigin + (forward * 104) + (right * 128),
				shipOrigin + (forward * 104) - (right * 128),
				shipOrigin + (forward * 104) + (up * 96),
				shipOrigin + (forward * 104) - (up * 112),
				shipOrigin + (right * 128),
				shipOrigin - (right * 128),
				shipOrigin + (up * 96),
				shipOrigin - (up * 112),
			};

			idVec3 targetPosition = vec3_zero;
			// Test our candidate positions
			for (int i = 0; i < 8; i++)
			{
				//gameRenderWorld->DebugBounds(idVec4(1, 0, 0, 1), playerBounds, candidatePositions[i], 10000);
				
				if (gameRenderWorld->PointInArea(candidatePositions[i]) == -1)
					continue; // If we're outside the world, continue

				if (gameLocal.clip.Contents(candidatePositions[i], NULL, mat3_identity, CONTENTS_SOLID, NULL) & MASK_SOLID)
					continue; // If we're in a solid, continue

				// Finally, do a bounds trace
				trace_t tr;
				gameLocal.clip.TraceBounds(tr, candidatePositions[i], candidatePositions[i], playerBounds, MASK_SOLID, blockingEntity);
				if (tr.fraction >= 1.0f)
				{
					// Bounds check is clear, let's deposit the player here
					targetPosition = candidatePositions[i];
					break;
				}
			}

			if (targetPosition != vec3_zero)
			{
				blockingEntity->Teleport(targetPosition, blockingEntity->GetPhysics()->GetAxis().ToAngles(), NULL);
			}
		}
	}
	else
	{
		idMover::Event_PartBlocked(blockingEntity);
	}
	


}