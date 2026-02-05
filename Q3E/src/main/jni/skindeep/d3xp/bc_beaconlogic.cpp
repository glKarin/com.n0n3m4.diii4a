#include "script/Script_Thread.h"
#include "framework/DeclEntityDef.h"

#include "Player.h"
#include "bc_meta.h"
#include "bc_lifeboat.h"
#include "bc_beaconlogic.h"
#include "WorldSpawn.h"

const int BL_SPAWNDELAY = 400;		//initial delay.
const int BL_AIM_OFFSET = 256;		//have aim cursor go forward this amount. make the pod trajectory pass through this point
const int BL_AIMTIME = 13000;		//how long player has to aim. 16000
const int BL_FASTBLINKTIME = 3000;	//how long it blinks fast.
const int BL_LOCKONTIME = 2800;		//delay before the launch happens.

const int BL_PODLENGTH = 64;		//bounding box check.
const int BL_PODRADIUS = 32;		//bounding box check.

//if the current aim position and the desired aim position are XX close, then go with it. It's close enough!!! But if it's
//beyond this threshold distance, then we try to find a different/better launch position.
const int BL_DESIREDPROXIMITY_THRESHOLD = 96;


const int BL_LAUNCHPOSITION_LERPTIME = 200; //how long it takes to transition to a new launch position.

CLASS_DECLARATION(idEntity, idBeaconLogic)

END_CLASS

//This is the logic that handles the player aiming where to make the lifeboat land.

//TODO: color it red if tracepoint hits skybox.

idBeaconLogic::idBeaconLogic()
{
	lasersightbeam = NULL;
	lasersightbeamTarget = NULL;
	tubeEnt = NULL;
}

idBeaconLogic::~idBeaconLogic()
{
	lasersightbeam->PostEventMS(&EV_Remove, 0);
	lasersightbeamTarget->PostEventMS(&EV_Remove, 0);
	placerEnt->PostEventMS(&EV_Remove, 0);

	lasersightbeam = nullptr;
	lasersightbeamTarget = nullptr;
	placerEnt = nullptr;

	if (tubeEnt != NULL)
	{
		delete tubeEnt;
		tubeEnt = nullptr;
	}
}


void idBeaconLogic::Spawn()
{
	idDict args;


	//Laser endpoint.
	lasersightbeamTarget = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	lasersightbeamTarget->BecomeActive(TH_PHYSICS);
	lasersightbeamTarget->SetOrigin(GetPhysics()->GetOrigin());

	//Laser startpoint.
	args.Clear();
	args.Set("target", lasersightbeamTarget->name.c_str());
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetBool("start_off", false);
	args.Set("width", spawnArgs.GetString("laserwidth", "16"));
	args.Set("skin", spawnArgs.GetString("laserskin", "skins/beam_beacon"));
	lasersightbeam = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);		
	lasersightbeam->Hide(); //For some reason start_off causes problems.... so spawn it normally, and then do a hide() here. oh well :/


	args.Clear();
	args.SetInt("solid", 0);
	args.Set("model", "models/objects/pod_lifeboat/lifeboat.ase");
	args.Set("skin", "skins/objects/pod_lifeboat/skin_ghost");
	placerEnt = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	placerEnt->Hide();

	args.Clear();
	args.SetInt("solid", 0);
	args.Set("model", "models/objects/ui_podtube/ui_podtube.ase");
	//args.Set("skin", "skins/objects/pod_lifeboat/skin_ghost");
	tubeEnt = (idStaticEntity *)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	tubeEnt->Hide();

	podLaunchPosition = vec3_zero;
	podLaunch_isLerping = false;
	podLaunch_lerpEnd = vec3_zero;
	podLaunch_lerpStart = vec3_zero;
	podLaunch_lerpTimer = 0;
	validLandingPosition = false;
	state = BEACONLOGIC_DELAY;
	stateTimer = gameLocal.time + BL_SPAWNDELAY;
	lastAimDir = vec3_zero;
	lastEyePosition = vec3_zero;
	podType = 0;
}

void idBeaconLogic::Save(idSaveGame* savefile) const
{
	savefile->WriteBool( validLandingPosition ); //  bool validLandingPosition
	savefile->WriteObject( placerEnt ); //  idEntity * placerEnt


	savefile->WriteObject( lasersightbeam ); //  idBeam* lasersightbeam
	savefile->WriteObject( lasersightbeamTarget ); //  idBeam* lasersightbeamTarget


	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( stateTimer ); //  int stateTimer


	savefile->WriteObject( tubeEnt ); //  idEntity * tubeEnt

	savefile->WriteVec3( podLaunchPosition ); //  idVec3 podLaunchPosition
	savefile->WriteVec3( podLaunch_lerpStart ); //  idVec3 podLaunch_lerpStart
	savefile->WriteVec3( podLaunch_lerpEnd ); //  idVec3 podLaunch_lerpEnd
	savefile->WriteBool( podLaunch_isLerping ); //  bool podLaunch_isLerping
	savefile->WriteInt( podLaunch_lerpTimer ); //  int podLaunch_lerpTimer

	savefile->WriteVec3( lastAimDir ); //  idVec3 lastAimDir
	savefile->WriteVec3( lastEyePosition ); //  idVec3 lastEyePosition

	savefile->WriteInt( podType ); //  int podType
}

void idBeaconLogic::Restore(idRestoreGame* savefile)
{
	savefile->ReadBool( validLandingPosition ); //  bool validLandingPosition
	savefile->ReadObject( placerEnt ); //  idEntity * placerEnt


	savefile->ReadObject( CastClassPtrRef(lasersightbeam)); //  idBeam* lasersightbeam
	savefile->ReadObject( CastClassPtrRef(lasersightbeamTarget) ); //  idBeam* lasersightbeamTarget


	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( stateTimer ); //  int stateTimer


	savefile->ReadObject( tubeEnt ); //  idEntity * tubeEnt

	savefile->ReadVec3( podLaunchPosition ); //  idVec3 podLaunchPosition
	savefile->ReadVec3( podLaunch_lerpStart ); //  idVec3 podLaunch_lerpStart
	savefile->ReadVec3( podLaunch_lerpEnd ); //  idVec3 podLaunch_lerpEnd
	savefile->ReadBool( podLaunch_isLerping ); //  bool podLaunch_isLerping
	savefile->ReadInt( podLaunch_lerpTimer ); //  int podLaunch_lerpTimer

	savefile->ReadVec3( lastAimDir ); //  idVec3 lastAimDir
	savefile->ReadVec3( lastEyePosition ); //  idVec3 lastEyePosition

	savefile->ReadInt( podType ); //  int podType
}

//Spawn the lifeboat.
void idBeaconLogic::LaunchPod(idVec3 destinationPosition)
{
	const char* podDefName;
	const idDeclEntityDef *podDef;
	idEntity *podEnt;

	if (podType == BEACONTYPE_EXTRACTION)
	{
		//podDef = gameLocal.FindEntityDef("moveable_lifeboat", false);
		
		// SW: worldspawn arg can be used to control the lifeboat def, 
		// allowing us to spawn lifeboats without a timer or with other properties
		// (for tutorialisation, early levels, etc)
		podDefName = gameLocal.world->spawnArgs.GetString("def_meta_lifeboat", "moveable_catpod");
		podDef = gameLocal.FindEntityDef(podDefName, false);
		
	}
	else if (podType == BEACONTYPE_SHOP)
	{
		podDef = gameLocal.FindEntityDef("moveable_shopboat", false);
	}
	else
	{
		gameLocal.Warning("beaconLogic has invalid pod type.");
		return;
	}

	

	if (!podDef)
	{
		gameLocal.Error("Failed to find def for moveable_lifeboat.");
	}

	gameLocal.SpawnEntityDef(podDef->dict, &podEnt, false);

	if (!podEnt)
	{
		gameLocal.Error("Failed to spawn moveable_lifeboat.");
	}
	else
	{
		idVec3 dirToTarget;
		idVec3 spawnPosition;

		//Pod has spawned.

		//Set pod position.
		spawnPosition = lasersightbeam->GetPhysics()->GetOrigin();
		podEnt->SetOrigin(spawnPosition);

		//Set pod orientation.
		dirToTarget = destinationPosition - spawnPosition;
		dirToTarget.Normalize();
		podEnt->SetAxis(dirToTarget.ToMat3());

		static_cast<idLifeboat *>(podEnt)->Launch(destinationPosition);

		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->signallampEntity = podEnt;
	}

}

void idBeaconLogic::Think(void)
{
	if (state == BEACONLOGIC_DELAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			state = BEACONLOGIC_ACTIVE;
			stateTimer = gameLocal.time + BL_AIMTIME;
			lasersightbeamTarget->SetOrigin(GetPhysics()->GetOrigin());
			lasersightbeam->SetOrigin(GetPhysics()->GetOrigin());
			lasersightbeam->Show();
			placerEnt->Show();

			StartSound("snd_beamaim", SND_CHANNEL_BODY, 0, false, NULL);

			//the tube model.
			tubeEnt->SetOrigin(GetPhysics()->GetOrigin());
			tubeEnt->Show();

			podLaunchPosition = GetPhysics()->GetOrigin();
		}
	}
	else if (state == BEACONLOGIC_ACTIVE || state == BEACONLOGIC_FASTBLINK)
	{
		trace_t beaconTr;
		trace_t LOSTr;
		trace_t playerAimTr;
		idVec3 playerAimPos;
		idVec3 beaconAimDir;

		if (gameLocal.GetLocalPlayer()->weapon.IsValid())
		{
			if (gameLocal.GetLocalPlayer()->weapon.GetEntity()->spawnArgs.GetBool("hasbeacon"))
			{
				//player is using the signal lamp. Update the aim direction to be the player's current view angle.
				lastAimDir = gameLocal.GetLocalPlayer()->firstPersonViewAxis[0];
				lastEyePosition = gameLocal.GetLocalPlayer()->GetEyePosition();
			}

			//if player is NOT using the signal lamp, then we just use the lastAimDir (the last-known direction the player was looking, before they switched away from the signal lamp)
		}

		//Player is aiming where to land the pod...
		
		//This is the position that the lifeboat launches from.
		lasersightbeam->SetOrigin(podLaunchPosition);
		
		//First, see if the player is pointing at a surface. If so, then just aim at that position.
		trace_t directAimTr;
		gameLocal.clip.TracePoint(directAimTr, lastEyePosition, lastEyePosition + (lastAimDir * 2048), CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP);

		if (directAimTr.fraction >= 1 || (directAimTr.c.material && directAimTr.c.material->GetSurfaceFlags() >= 256))
		{
			//player is NOT looking at a surface. In this case, make the pod try to pass through a point in front of the player's eyes.

			//find point XXX units in front of player.
			gameLocal.clip.TracePoint(playerAimTr, lastEyePosition, lastEyePosition + (lastAimDir * BL_AIM_OFFSET), CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP);
			playerAimPos = playerAimTr.endpos;		//Where the player wants the boat to pass through.
		}
		else
		{
			//player is looking at a surface. Make the pod aim toward that spot on the surface.
			playerAimPos = directAimTr.endpos + (directAimTr.c.normal * -1);
		}

		//See if there is LOS between the desired destination point and the launch point.
		trace_t directLOStr;
		gameLocal.clip.TraceBounds(directLOStr, podLaunchPosition, playerAimPos, idBounds(idVec3(-BL_PODRADIUS, -BL_PODRADIUS, -BL_PODRADIUS), idVec3(BL_PODRADIUS, BL_PODRADIUS, BL_PODRADIUS)), CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP);

		float distanceToDesiredPosition = (directLOStr.endpos - playerAimPos).Length();
		if (distanceToDesiredPosition > BL_DESIREDPROXIMITY_THRESHOLD && !podLaunch_isLerping) //current destination point is too far from desired destination point.
		{
			bool findNewAlternateLaunchPosition = false;

			//...if the lifeboat's trajectory does not have clearance to the destination, try the alternate position.
			if (podLaunchPosition != vec3_zero)
			{
				gameLocal.clip.TraceBounds(directLOStr, podLaunchPosition, playerAimPos, idBounds(idVec3(-BL_PODRADIUS, -BL_PODRADIUS, -BL_PODRADIUS), idVec3(BL_PODRADIUS, BL_PODRADIUS, BL_PODRADIUS)), CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP);
				distanceToDesiredPosition = (directLOStr.endpos - playerAimPos).Length();

				if (distanceToDesiredPosition < BL_DESIREDPROXIMITY_THRESHOLD)
				{
					//valid trajectory. Go with it.
					podLaunchPosition = podLaunchPosition;
				}
				else
				{
					findNewAlternateLaunchPosition = true;
				}
			}
			else
			{
				findNewAlternateLaunchPosition = true;
			}

			if (findNewAlternateLaunchPosition)
			{
				//Find a new alternate position.
				idVec3 newLaunchPos = FindAlternateLaunchPosition(playerAimPos);
				if (newLaunchPos != vec3_zero)
				{
					//podLaunchPosition = newLaunchPos;

					//We have a new position. Lerp it.
					podLaunch_lerpStart = podLaunchPosition;
					podLaunch_lerpEnd = newLaunchPos;
					podLaunch_isLerping = true;
					podLaunch_lerpTimer = gameLocal.time;
				}
			}
		}

		if (podLaunch_isLerping)
		{
			float lerp = (gameLocal.time - podLaunch_lerpTimer) / (float)BL_LAUNCHPOSITION_LERPTIME;
			lerp = idMath::ClampFloat(0, 1, lerp);
			lerp = idMath::CubicEaseOut(lerp);
			podLaunchPosition.Lerp(podLaunch_lerpStart, podLaunch_lerpEnd, lerp);

			if (gameLocal.time > podLaunch_lerpTimer + BL_LAUNCHPOSITION_LERPTIME)
			{
				podLaunch_isLerping = false;
			}
		}			

		beaconAimDir = playerAimPos - podLaunchPosition;
		beaconAimDir.Normalize(); //The direction the boat moves...

		//The ghost UI is not mega accurate.
		//The problem is the traceBounds does not orient/rotate the idBounds and assumes it always aligned to the world grid.
		//Whereas the pod we launch is oriented at an angle. So this creates inaccuracy in the placer ghost UI.

		gameLocal.clip.TraceBounds(LOSTr, podLaunchPosition, playerAimPos, idBounds(idVec3(-BL_PODRADIUS, -BL_PODRADIUS, -BL_PODRADIUS), idVec3(BL_PODRADIUS, BL_PODRADIUS, BL_PODRADIUS)), CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP); //This determines where the POD ghost ends at.
		
		gameLocal.clip.TracePoint(beaconTr, podLaunchPosition, podLaunchPosition + (beaconAimDir * 4096), CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP); //This determines where the LINE ends at.


		validLandingPosition = (beaconTr.c.material->GetSurfaceFlags() < 256);


		if (validLandingPosition)
		{
			//Only allow valid landing position if player is looking toward the direction of the landing position.
			//We don't allow no-look behind aiming because it feels confusing and is hard to understand.
			//The HIGHER this value is, the more restrictive it is -- the player has to aim very precisely.
			//0.5 = aimposition needs to be in 45 degree cone in front of player.
			//0.0 = aimposition needs to be in 180 cone in front of player.
			#define AIM_DOTPRODUCT_THRESHOLD .5f
			

			//do a dotproduct check. Only allow if player is looking toward the landing position.
			idVec3 dirToLandingPos;
			dirToLandingPos = beaconTr.endpos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
			dirToLandingPos.Normalize();
			float facingResult = DotProduct(dirToLandingPos, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
			if (facingResult < AIM_DOTPRODUCT_THRESHOLD)
			{
				validLandingPosition = false;
			}
		}

		idVec4 uiColor = (validLandingPosition) ? idVec4(1, 1, 1, 1) : idVec4(1, .1f, .1f, 1);
		
		lasersightbeam->SetColor(uiColor);
		placerEnt->SetColor(uiColor);

		

		//gameRenderWorld->DebugArrow(colorGreen, beaconPos, beaconTr.endpos, 16, 100);
		lasersightbeamTarget->SetOrigin(beaconTr.endpos);


		//Orient and position the pod's ghost image.
		trace_t ghostTr;
		gameLocal.clip.TraceBounds(ghostTr, podLaunchPosition, beaconTr.endpos, idBounds(idVec3(-BL_PODRADIUS, -BL_PODRADIUS, -BL_PODRADIUS), idVec3(BL_PODRADIUS, BL_PODRADIUS, BL_PODRADIUS)), CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP);
		placerEnt->SetAxis(beaconAimDir.ToMat3());		
		//placerEnt->SetOrigin(LOSTr.endpos); //Place it at the position in front of player.
		placerEnt->SetOrigin(ghostTr.endpos); //Place it at the end point of the traceline.
		

		//if (!lasersightbeam->IsHidden())
		//{
		//	lasersightbeam->Hide();
		//	LaunchPod(beaconTr.endpos);
		//}

		//update UI.
		float lerp;
		
		if (state == BEACONLOGIC_ACTIVE)
		{
			lerp = ((stateTimer + BL_FASTBLINKTIME) - gameLocal.time) / (float)(BL_AIMTIME + BL_FASTBLINKTIME);
			lerp = idMath::ClampFloat(0, 1, 1 - lerp);
		}
		else
		{
			//fast blink state.
			lerp = (stateTimer - gameLocal.time) / (float)(BL_FASTBLINKTIME);
			lerp = idMath::ClampFloat(0, 1, 1 - lerp);

			lerp *= BL_FASTBLINKTIME / (float)(BL_FASTBLINKTIME + BL_AIMTIME);
			lerp += BL_AIMTIME / (float)(BL_FASTBLINKTIME + BL_AIMTIME);
		}
		
		gameLocal.GetLocalPlayer()->hud->SetStateFloat("beaconbar", lerp);

		if (gameLocal.time >= stateTimer)
		{
			if (state == BEACONLOGIC_ACTIVE)
			{
				state = BEACONLOGIC_FASTBLINK;
				stateTimer = gameLocal.time + BL_FASTBLINKTIME;
				lasersightbeam->SetSkin(declManager->FindSkin("skins/beam_beacon_fastblink"));
				placerEnt->SetSkin(declManager->FindSkin("skins/objects/pod_lifeboat/skin_ghost_fastblink"));
			}
			else if (state == BEACONLOGIC_FASTBLINK)
			{
				DeployConfirmation(true);
			}
		}


		//the tube model.
		tubeEnt->SetOrigin(LOSTr.endpos);
		tubeEnt->SetAxis(beaconAimDir.ToMat3());
		tubeEnt->SetColor(uiColor);
	}
	else if (state == BEACONLOGIC_LOCKON)
	{
		//Beam is locked on. Wait.

		if (gameLocal.time >= stateTimer)
		{
			//Launch the pod.
			LaunchPod(lasersightbeamTarget->GetPhysics()->GetOrigin());

			DestroySelf();
		}
	}
}

void idBeaconLogic::DestroySelf()
{
	state = BEACONLOGIC_DORMANT;
	Hide();
	lasersightbeam->Hide();
	PostEventMS(&EV_Remove, 0);
}

//This gets called when the player presses LMB.
void idBeaconLogic::DeployConfirmation(bool force)
{
	if (!force)
	{
		//Check if it's hitting sky
		idVec3 laserDir = lasersightbeamTarget->GetPhysics()->GetOrigin() - lasersightbeam->GetPhysics()->GetOrigin();
		laserDir.Normalize();

		trace_t laserTr;
		gameLocal.clip.TracePoint(laserTr, podLaunchPosition, podLaunchPosition + laserDir * 8192, MASK_SOLID, NULL);		

		//If doesnt hit anything or hits sky, then ignore.
		if (laserTr.fraction >= 1.0f || (laserTr.c.material && laserTr.c.material->GetSurfaceFlags() >= 256))
		{
			StartSound("snd_error", SND_CHANNEL_VOICE, 0, false, NULL);
			return;
		}
	}

	//If timer expires and is in invalid landing position, then cancel the process.
	if (!validLandingPosition)
	{
		StopSound(SND_CHANNEL_BODY, false);
		StartSound("snd_error", SND_CHANNEL_VOICE, 0, false, NULL);
		gameLocal.GetLocalPlayer()->hud->SetStateBool("showbeaconconfirm", false);
		tubeEnt->Hide();
		DestroySelf();
		return;
	}
	

	state = BEACONLOGIC_LOCKON;
	stateTimer = gameLocal.time + BL_LOCKONTIME;
	lasersightbeam->SetSkin(declManager->FindSkin("skins/beam_beacon_locked"));
	StopSound(SND_CHANNEL_BODY, false);
	StartSound("snd_beamlock", SND_CHANNEL_BODY2, 0, false, NULL);
	placerEnt->SetSkin(declManager->FindSkin("skins/objects/pod_lifeboat/skin_ghost_locked"));
	
	//Tell the LMB Confirm ui to turn off.
	validLandingPosition = false;
	gameLocal.GetLocalPlayer()->hud->SetStateBool("showbeaconconfirm", false);

	//the tube model.
	tubeEnt->Hide();


	StartSound("snd_vo_podsummon", SND_CHANNEL_VOICE);
}

//Called when player flashes the signal lamp.
void idBeaconLogic::FlashConfirm()
{
	if (state == BEACONLOGIC_ACTIVE || state == BEACONLOGIC_FASTBLINK)
	{
		DeployConfirmation(false); //Press LMB to confirm landing position.
	}
}

bool idBeaconLogic::HasLockedOn()
{
	return (state == BEACONLOGIC_LOCKON);
}

//If the player is aiming at a spot that does not have LOS to the desired landing spot. Try to find a new launching spot that does have LOS.
idVec3 idBeaconLogic::FindAlternateLaunchPosition(idVec3 destinationPoint)
{
	//Try to find a clear spot of Sky for the beacon to launch from, that has LOS to the destinationPoint.
	//We do this by shooting tracelines around the player.

	#define TRACE_DISTANCE 4096

	//This array is a list of angles to check. (forward, right, up)
	#define CHECK_ANGLE_AMOUNT 16
	idVec3 checkDirections[CHECK_ANGLE_AMOUNT]
	{
		//Behind player.
		idVec3(-1,	0,		 .75f),
		idVec3(-1,	0,		-.75f),
		idVec3(-1,	 .75f,	    0),
		idVec3(-1,	-.75f,	    0),

		idVec3(-1,	 .75f,	 .75f),
		idVec3(-1,	 .75f,	-.75f),
		idVec3(-1,	-.75f,	 .75f),
		idVec3(-1,	-.75f,	-.75f),

		//In front of player.
		idVec3(1,	0,		 .75f),
		idVec3(1,	0,		-.75f),
		idVec3(1,	 .75f,	    0),
		idVec3(1,	-.75f,	    0),

		idVec3(1,	 .75f,	 .75f),
		idVec3(1,	 .75f,	-.75f),
		idVec3(1,	-.75f,	 .75f),
		idVec3(1,	-.75f,	-.75f),
	};

	idVec3 forward, right, up;
	gameLocal.GetLocalPlayer()->viewAngles.ToVectors(&forward, &right, &up);

	for (int i = 0; i < CHECK_ANGLE_AMOUNT; i++)
	{
		//Find position behind the player.
		idVec3 targetDir = (forward * checkDirections[i].x) + (right * checkDirections[i].y) + (up * checkDirections[i].z);
		targetDir.NormalizeFast();
		idVec3 targetPos = gameLocal.GetLocalPlayer()->GetEyePosition() + targetDir * TRACE_DISTANCE;

		//Ensure the point touches sky.
		trace_t skyTr;
		gameLocal.clip.TracePoint(skyTr, gameLocal.GetLocalPlayer()->GetEyePosition(), targetPos, MASK_SOLID, gameLocal.GetLocalPlayer());
		if (skyTr.c.material->GetSurfaceFlags() >= 256)
		{
			//We found a patch of sky. Great.
			//See if it has clear LOS to the destinationPoint.

			idVec3 trajectory = destinationPoint - targetPos;
			trajectory.Normalize();

			trace_t LOStr;
			idVec3 adjustedLaunchPos = skyTr.endpos + trajectory * 64.0f; //We need to jut out from the sky a little so that the trace check doesn't immediately collide with the sky.
			gameLocal.clip.TraceBounds(LOStr, adjustedLaunchPos, destinationPoint,
				idBounds(idVec3(-BL_PODRADIUS, -BL_PODRADIUS, -BL_PODRADIUS), idVec3(BL_PODRADIUS, BL_PODRADIUS, BL_PODRADIUS)),
				CONTENTS_SOLID, gameLocal.GetLocalPlayer(), CONTENTS_NOBEACONCLIP);

			float distanceToDesiredPosition = (LOStr.endpos - destinationPoint).Length();
			if (distanceToDesiredPosition < BL_DESIREDPROXIMITY_THRESHOLD) //if the current destination point is close enough to the desired destination point, then just go with it. If too far, then...
			{
				return skyTr.endpos + trajectory * 64.0f;
			}
			
		}
	}

	return vec3_zero;
}

void idBeaconLogic::SetPodType(int _podType)
{
	podType = _podType;
}
