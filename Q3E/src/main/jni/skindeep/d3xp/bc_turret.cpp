
//TODO: add sound effect to end of the laser beam?
//TOOD: make it bias toward breakable stuff?
//TODO: add sound when rotating.

//TODO: change particle fx to not include so much smoke (it wafts upward, so we don't see much of it)

#include "script/Script_Thread.h"
#include "Game_local.h"
#include "gamesys/SysCvar.h"
#include "Player.h"
#include "Moveable.h"
#include "framework/DeclEntityDef.h"
#include "Fx.h"
#include "WorldSpawn.h"
#include "idlib/LangDict.h"

#include "bc_meta.h"
#include "bc_turret.h"




#define		WAKEUPTIME			2400		//activation time.

#define		MAXVOLLEY			5			//how many shots in one volley.
#define		WARMUPTIME			1900		//how long the pre-volley warning warmup is.
#define		VOLLEYDELAY			250			//ms between each shot in a volley.

#define		IDLESOUNDDELAY		3000
#define		IDLESOUNDRAND		2000

#define		MUZZLEFLASHTIME		30
#define		BEAMWIDTH			4.0f


#define		PITCHADJUSTMENT		-8 //aim a little below the eyes.

#define		THROWTIME_THRESHOLD	1000	//thrown objects are valid targets for X milliseconds after being thrown.

#define		MAX_DISTANCE		1024

#define		BLINDSPOT_SIZE		48


const float	TURRET_ROTATION_SPEED_DEGSEC = 48.0f; // previously was frame based, so 0.8f * 60 = 48
const float	LASER_ROTATION_SPEED_DEGSEC = 64.0f;

const float LASER_YAW_MAX = 45.0f;
const float LASER_PITCH_MAX = 80.0f;

#define		FOV_DOTPRODUCT		-.8f //FOV for activating warmup mode.  -.9f = requires turret to look directly at target.  -.1f = turret only needs to be vaguely looking at target.

#define     MUZZLE_HEIGHT      -12 //muzzle is about this many units vertically from 0,0,0

#define		FINDFREQUENCY		300

#define		LASER_MOVESPEED		.05f

#define		IDLE_MAXSWAYTIME	2000

#define		THROWNITEM_ATTACKTIME	1500

#define		PAIN_COOLDOWNTIME		2000 // when damaged, how long is pain time

#define		TARGETING_LOCKTIME		800 //when sight target, target has XX milliseconds to break line of sight

#define		IDLEVO_MINTIME 3000		//idle VO timers
#define		IDLEVO_MAXTIME 4500


#define		HEADLIGHT_FORWARDOFFSET		8
#define		HEADLIGHT_RADIUS			32

#define		SPOTLIGHT_VERTICALOFFSET	-1

#define		SHADERPARM_VISIONCONE		7 //make the vision cone visualization appear/hide. We want it visible when active, and hidden when the turret is undeployed.

//this is the viewcone used by the targeting system.
//Here is a top down view of the turret and dotproduct diagram:
//             1.0
// 0.1                       0.1
//              ^
// -0.1       turret         -0.1
//
//            -1.0
//So, -1.0 is behind the turret, and 1.0 is in front of the turret.
//If the dotproduct to the target is BELOW the threshold, then we ignore the target.
#define		TARGETING_VIEWTHRESHOLD			0.2f

//When the target is a thrown object, we have a more lenient / wider viewing arc, so that it's more likely for the turret to notice it.
#define		TARGETING_VIEWTHRESHOLD_OBJECTS	-.3f

idCVar g_turret_trackingtype( "g_turret_trackingtype", "1", CVAR_CHEAT | CVAR_SYSTEM | CVAR_BOOL, "switches between old (0) and new (1) tracking types" );

const idEventDef EV_Turret_activate( "turretactivate", "d" );
const idEventDef EV_Turret_isactive( "turretisactive", NULL, 'd' );
const idEventDef EV_Turret_muzzleflashoff( "turretmuzzleoff", NULL );


CLASS_DECLARATION( idAnimatedEntity, idTurret )

	EVENT( EV_Turret_activate,			idTurret::Event_activate)
	EVENT( EV_Turret_isactive,			idTurret::Event_isactive)
	EVENT( EV_Turret_muzzleflashoff,	idTurret::MuzzleflashOff)

END_CLASS

idTurret::idTurret(void)
{
	turretNode.SetOwner(this);
	turretNode.AddToEnd(gameLocal.turretEntities);	

	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
}

idTurret::~idTurret(void)
{
	turretNode.Remove();

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idTurret::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); // int state

	savefile->WriteInt( nextStateTime ); // int nextStateTime
	savefile->WriteInt( volleyCount ); // int volleyCount
	savefile->WriteInt( nextIdleSound ); // int nextIdleSound

	savefile->WriteMat3( bodyAxis ); // idMat3 bodyAxis
	savefile->WriteMat3( turretAxis ); // idMat3 turretAxis
	savefile->WriteVec3( turretSpawnDir ); // idVec3 turretSpawnDir

	savefile->WriteObject( beamStart ); // idBeam* beamStart
	savefile->WriteObject( beamEnd ); // idBeam* beamEnd

	savefile->WriteObject( laserdot ); // idEntity* laserdot

	savefile->WriteVec3( laserAimPos ); // idVec3 laserAimPos
	savefile->WriteInt( laserAimtimer ); // int laserAimtimer
	savefile->WriteInt( laserAimMaxtime ); // int laserAimMaxtime
	savefile->WriteVec3( laserAimPosStart ); // idVec3 laserAimPosStart
	savefile->WriteVec3( laserAimPosEnd ); // idVec3 laserAimPosEnd
	savefile->WriteBool( laserAimIsMoving ); // bool laserAimIsMoving

	savefile->WriteObject( targetEnt ); // idEntityPtr<idEntity> targetEnt
	savefile->WriteInt( lastTargetNum ); // int lastTargetNum

	savefile->WriteInt( nextFindTime ); // int nextFindTime

	savefile->WriteVec3( lastTargetPos ); // idVec3 lastTargetPos

	savefile->WriteDict( &brassDict ); // idDict brassDict

	savefile->WriteFloat( targetYaw ); // float targetYaw
	savefile->WriteFloat( bodyYaw ); // float bodyYaw

	savefile->WriteInt( idleSwayTimer ); // int idleSwayTimer

	savefile->WriteInt( lastTeam ); // int lastTeam

	savefile->WriteInt( idleVOTimer ); // int idleVOTimer

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteBool( electricalActive ); // bool electricalActive

	savefile->WriteObject( spotlight ); // idLight * spotlight

	savefile->WriteBool( playingRotationSound ); // bool playingRotationSound

	savefile->WriteObject( inflictorEnt ); // idEntityPtr<idEntity> inflictorEnt
	savefile->WriteObject( searchEnt ); // idEntityPtr<idEntity> searchEnt
}

void idTurret::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( state ); // int state

	savefile->ReadInt( nextStateTime ); // int nextStateTime
	savefile->ReadInt( volleyCount ); // int volleyCount
	savefile->ReadInt( nextIdleSound ); // int nextIdleSound

	savefile->ReadMat3( bodyAxis ); // idMat3 bodyAxis
	savefile->ReadMat3( turretAxis ); // idMat3 turretAxis
	savefile->ReadVec3( turretSpawnDir ); // idVec3 turretSpawnDir

	savefile->ReadObject( CastClassPtrRef(beamStart) ); // idBeam* beamStart
	savefile->ReadObject( CastClassPtrRef(beamEnd) ); // idBeam* beamEnd

	savefile->ReadObject( laserdot ); // idEntity* laserdot

	savefile->ReadVec3( laserAimPos ); // idVec3 laserAimPos
	savefile->ReadInt( laserAimtimer ); // int laserAimtimer
	savefile->ReadInt( laserAimMaxtime ); // int laserAimMaxtime
	savefile->ReadVec3( laserAimPosStart ); // idVec3 laserAimPosStart
	savefile->ReadVec3( laserAimPosEnd ); // idVec3 laserAimPosEnd
	savefile->ReadBool( laserAimIsMoving ); // bool laserAimIsMoving

	savefile->ReadObject( targetEnt ); // idEntityPtr<idEntity> targetEnt
	savefile->ReadInt( lastTargetNum ); // int lastTargetNum

	savefile->ReadInt( nextFindTime ); // int nextFindTime

	savefile->ReadVec3( lastTargetPos ); // idVec3 lastTargetPos

	savefile->ReadDict( &brassDict ); // idDict brassDict

	savefile->ReadFloat( targetYaw ); // float targetYaw
	savefile->ReadFloat( bodyYaw ); // float bodyYaw

	savefile->ReadInt( idleSwayTimer ); // int idleSwayTimer

	savefile->ReadInt( lastTeam ); // int lastTeam

	savefile->ReadInt( idleVOTimer ); // int idleVOTimer

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadBool( electricalActive ); // bool electricalActive

	savefile->ReadObject( CastClassPtrRef(spotlight) ); // idLight * spotlight

	savefile->ReadBool( playingRotationSound ); // bool playingRotationSound

	savefile->ReadObject( inflictorEnt ); // idEntityPtr<idEntity> inflictorEnt
	savefile->ReadObject( searchEnt ); // idEntityPtr<idEntity> searchEnt
}


void idTurret::Spawn(void)
{
	jointHandle_t			bodyJoint, turretJoint;
	idVec3 origin;
	idDict args;
	idVec3 turretPos;

	state = TURRET_OFF;
	nextStateTime = 0;
	volleyCount = 0;
	nextIdleSound = 0;
	targetEnt = NULL;
	nextFindTime = 0;
	lastTargetPos = vec3_zero;
	targetYaw = 0;
	bodyYaw = 180;
	laserAimtimer = 0;
	laserAimPos = laserAimPosStart = laserAimPosEnd = vec3_zero;
	laserAimIsMoving = false;
	laserAimMaxtime = 0;
	lastTargetNum = -1;
	idleSwayTimer = 0;
	lastTeam = -1;
	idleVOTimer = 0;
	playingRotationSound = false;

	BecomeActive(TH_THINK);

	//Get joints.
	bodyJoint = animator.GetJointHandle("body");
	animator.GetJointTransform(bodyJoint, gameLocal.time, origin, this->bodyAxis);

	turretJoint = animator.GetJointHandle("turret");
	animator.GetJointTransform(turretJoint, gameLocal.time, turretPos, this->turretAxis);
	turretPos = this->GetPhysics()->GetOrigin() + turretPos * this->GetPhysics()->GetAxis();

	turretSpawnDir = bodyAxis.ToAngles().ToForward();

	SetSkin(declManager->FindSkin("skins/turret/skin")); //Turn muzzle flash off.
	
	

	//spawn beam end.
	args.SetVector("origin", vec3_origin);
	beamEnd = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);

	//spawn beam start.
	args.Clear();
	args.Set("target", beamEnd->name.c_str());
	args.SetVector("origin", turretPos);
	args.SetBool("start_off", true);
	args.Set("skin", "skins/beam_turret");
	args.SetFloat("width", BEAMWIDTH);
	beamStart = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	beamStart->BindToJoint(this, turretJoint, false);
	beamStart->BecomeActive(TH_PHYSICS);
	beamStart->GetRenderEntity()->shaderParms[SHADERPARM_RED] = spawnArgs.GetVector("_color").x;
	beamStart->GetRenderEntity()->shaderParms[SHADERPARM_GREEN] = spawnArgs.GetVector("_color").y;
	beamStart->GetRenderEntity()->shaderParms[SHADERPARM_BLUE] = spawnArgs.GetVector("_color").z;
	beamStart->Hide();
	
	//Spawn the little dot at end of beam.
	args.Clear();
	args.SetVector("origin", vec3_origin);
	args.Set("model", "models/objects/lasersight/tris.ase");
	args.SetInt("solid", 0);
	args.SetBool("noclipmodel", true);
	laserdot = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	laserdot->GetRenderEntity()->shaderParms[SHADERPARM_RED] = spawnArgs.GetVector("_color").x;
	laserdot->GetRenderEntity()->shaderParms[SHADERPARM_GREEN] = spawnArgs.GetVector("_color").y;
	laserdot->GetRenderEntity()->shaderParms[SHADERPARM_BLUE] = spawnArgs.GetVector("_color").z;
	laserdot->Hide();
	laserdot->SetOrigin(beamEnd->GetPhysics()->GetOrigin());
	laserdot->Bind(beamEnd, false);

	team = spawnArgs.GetInt("team", "1"); //Set what team I am on.

	

	const idDeclEntityDef *brassDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_ejectBrass"), false);
	if (brassDef)
	{
		brassDict = brassDef->dict;
	}

	fl.takedamage = false;

	//precache skin.
	//declManager->FindType(DECL_SKIN, spawnArgs.GetString("skin_death"));

    //Can be targeted by ai.
    aimAssistNode.SetOwner(this);
    aimAssistNode.AddToEnd(gameLocal.aimAssistEntities);

	electricalActive = true;


	if (spawnArgs.GetBool("start_on", "0"))
	{
		Event_activate(1);
	}
	else
	{
		//visual visioncone starts off.
		renderEntity.shaderParms[SHADERPARM_VISIONCONE] = 0;
		UpdateVisuals();
	}

	


	//Set up the spotlight. We want the spotlight to always be the same size, regardless of distance from floor to turret, so this requires some spotlight resizing.
	idVec3 lightTargetVec = idVec3(0, 0, -768);
	for (int i = 0; i < 5; i++)
	{
		idVec3 traceStartOffset = vec3_zero;

		if (i == 1)
			traceStartOffset = idVec3(-20, 0, 0);
		else if (i == 2)
			traceStartOffset = idVec3(20, 0, 0);
		else if (i == 3)
			traceStartOffset = idVec3(0, 20, 0);
		else
			traceStartOffset = idVec3(0, -20, 0);

		trace_t downTr;
		gameLocal.clip.TracePoint(downTr, GetPhysics()->GetOrigin() + traceStartOffset + idVec3(0, 0, SPOTLIGHT_VERTICALOFFSET), GetPhysics()->GetOrigin() + traceStartOffset + idVec3(0, 0, -1024), CONTENTS_SOLID, this);
		if (downTr.fraction < 1)
		{
			if (downTr.c.entityNum <= MAX_GENTITIES - 2 && downTr.c.entityNum >= 0)
			{
				if (gameLocal.entities[downTr.c.entityNum]->IsType(idWorldspawn::Type)) //Make sure it's hitting the floor.
				{
					float lightDistance = downTr.fraction * -1024;
					lightTargetVec = idVec3(lightDistance, 0, 0);
					break;
				}
			}
		}
	}

	idAngles turretAngle = GetPhysics()->GetAxis().ToAngles();
	turretAngle.yaw -= 90;
	turretAngle.pitch -= 89.9f;

	idVec3 spotlightRight = idVec3(0, 0, -BLINDSPOT_SIZE);
	idVec3 spotlightUp = idVec3(0, BLINDSPOT_SIZE, 0);

	idDict lightArgs;
	lightArgs.Clear();
	lightArgs.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0,0, SPOTLIGHT_VERTICALOFFSET));
	lightArgs.Set("texture", "textures/lights/turret_spotlight");
	lightArgs.SetInt("noshadows", 0);
	lightArgs.SetInt("start_off", 1);
	lightArgs.Set("_color", "1 0 0 1"); //red.
	lightArgs.SetVector("light_right", spotlightRight);
	lightArgs.SetVector("light_up", spotlightUp);
	lightArgs.SetVector("light_target", lightTargetVec);
	lightArgs.SetMatrix("rotation", turretAngle.ToMat3());
	spotlight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);



	spotlight->BindToJoint(this, bodyJoint, true);
}

void idTurret::ActivateLights()
{
	//Activate lasers.
	laserAimIsMoving = false;
	idVec3 facingAngle, muzzlePos;
	muzzlePos = GetMuzzlePos();
	facingAngle = muzzlePos - idVec3(this->GetPhysics()->GetOrigin().x, this->GetPhysics()->GetOrigin().y, muzzlePos.z);
	facingAngle.Normalize();			
	laserAimPos = GetDefaultLaserAimAngle(facingAngle);
	beamEnd->SetOrigin(laserAimPos);

	this->beamStart->Show();
	this->laserdot->Show();

	SetLight(true);

	renderEntity.shaderParms[SHADERPARM_VISIONCONE] = 1;
	UpdateVisuals();
}

void idTurret::SetLight(bool active)
{
	//spotlight = the spotlight projected on the ground.
	//headlight = the light that illuminates the turret.

	if (active)
	{
		if (headlightHandle != -1)
			return;

		//Create light source.
		headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = HEADLIGHT_RADIUS;

		if (team == TEAM_ENEMY)
		{
			headlight.shaderParms[0] = 0.9f;
			headlight.shaderParms[1] = 0;
			headlight.shaderParms[2] = 0;
		}
		else if (team == TEAM_FRIENDLY)
		{
			headlight.shaderParms[0] = 0;
			headlight.shaderParms[1] = .5f;
			headlight.shaderParms[2] = 1;
		}
		else
		{
			headlight.shaderParms[0] = .7f;
			headlight.shaderParms[1] = .7f;
			headlight.shaderParms[2] = .7f;
		}

		//position the light, or else it will do a weird snap.
		idVec3 forward = GetMuzzlePos() - idVec3(this->GetPhysics()->GetOrigin().x, this->GetPhysics()->GetOrigin().y, GetMuzzlePos().z);
		forward.NormalizeFast();
		headlight.origin = GetMuzzlePos() + (forward * HEADLIGHT_FORWARDOFFSET);

		headlight.shaderParms[3] = 1.0f;
		headlight.noShadows = true;
		headlight.isAmbient = true;
		headlight.axis = mat3_identity;
		headlightHandle = gameRenderWorld->AddLightDef(&headlight);

		spotlight->On();

		// SW 18th Feb 2025: enable vision cone
		renderEntity.shaderParms[SHADERPARM_VISIONCONE] = 1;
	}
	else
	{
		if (headlightHandle != -1)
		{
			gameRenderWorld->FreeLightDef(headlightHandle);
			headlightHandle = -1;
		}

		spotlight->Off();

		// SW 18th Feb 2025: disable vision cone
		renderEntity.shaderParms[SHADERPARM_VISIONCONE] = 0;
	}

	UpdateVisuals();
}


//Script call. Am I active?
void idTurret::Event_isactive()
{
	idThread::ReturnInt(this->state);
}

//Script call. Activate the turret.
void idTurret::Event_activate(int value)
{
	if (health <= 0)
		return;

	if (!electricalActive && value)
		return;

	if (value)
	{
		//turn ON.

		if (state != TURRET_OFF)
		{
			return; //if I'm already on, then skip.
		}

		//open up.
		Event_PlayAnim("opening", 4);
		this->state = TURRET_OPENING;
		StartSound("snd_opening", SND_CHANNEL_BODY3, 0, false, NULL);
		StartSound("snd_vo_activate", SND_CHANNEL_VOICE);
		this->nextStateTime = gameLocal.time + WAKEUPTIME;
		this->nextIdleSound = gameLocal.time + WAKEUPTIME;
		idleVOTimer = gameLocal.time + 3000;

		fl.takedamage = true;
		GetPhysics()->SetClipModel(new idClipModel(spawnArgs.GetString("clipmodel_open")), 1.0f);

		//BC 2-21-2025: make the turret uninspectable when the doors are open.
		SetInspectable(false);
	}
	else
	{
		//turn OFF.

		if (state == TURRET_OFF)
		{
			return; //If I'm already closed, then skip.
		}

		StopSound(SND_CHANNEL_RADIO);

		//close up.
		Event_PlayAnim("closing", 4);
		this->state = TURRET_OFF;
		//this->nextStateTime = doneTime;
		StartSound("snd_closing", SND_CHANNEL_BODY3, 0, false, NULL);
		StartSound("snd_vo_deactivate", SND_CHANNEL_VOICE);
		this->beamStart->Hide(); //Hide laser.
		this->laserdot->Hide(); //Hide laser.
		SetLight(false);

		fl.takedamage = false;
		GetPhysics()->SetClipModel(new idClipModel(spawnArgs.GetString("clipmodel")), 1.0f);

		SetLight(false);

		//Turn off visioncone visualization.
		renderEntity.shaderParms[SHADERPARM_VISIONCONE] = 0;
		UpdateVisuals();

		//BC 2-21-2025: make the turret uninspectable when the doors are open.
		SetInspectable(true);
	}
}

void idTurret::MuzzleflashOff(void)
{
	SetSkin(declManager->FindSkin("skins/turret/skin"));
	
}

int idTurret::IsTargetValid(idEntity *ent)
{
	//Thrown item.
	if (ent->IsType(idItem::Type) && ent->throwTime + THROWNITEM_ATTACKTIME > gameLocal.time )
	{
		return TARG_OBJECTTARGET;
	}

	//if notarget/noclip player, then ignore.
	if (ent->IsType(idPlayer::Type))
	{
		if (static_cast<idPlayer *>(ent)->noclip || static_cast<idPlayer *>(ent)->fl.notarget || static_cast<idPlayer *>(ent)->GetDefibState() || static_cast<idPlayer *>(ent)->GetDarknessValue() <= 0 )
		{
			return TARG_INVALIDTARGET;
		}
	}

	//If throwing a dead body, always shoot it.
	if (ent->IsType(idActor::Type) && ent->health <= 0 && ent->throwTime + THROWNITEM_ATTACKTIME > gameLocal.time)
	{
		return TARG_ACTORTARGET;
	}

	//Ignore dead things. We're on the same team. or it's a neutral. skip.
	if (ent->health <= 0 || ent->team == this->team || ent->team == TEAM_NEUTRAL)
		return TARG_INVALIDTARGET;

	if (!ent->IsType(idActor::Type))
	{
		return TARG_INVALIDTARGET;
	}

	//Target is an actor.
	return TARG_ACTORTARGET;
}

idEntity* idTurret::FindTarget()
{
    int			listedEntities;
    idEntity	*entityList[MAX_GENTITIES];
    float       closestDist = MAX_DISTANCE;
    int         closestIndex = -1;

    //Iterate through all entities that are near turret.
    listedEntities = gameLocal.EntitiesWithinRadius(GetPhysics()->GetOrigin(), MAX_DISTANCE, entityList, MAX_GENTITIES);
    for (int i = 0; i < listedEntities; i++)
    {
        idEntity *ent = entityList[i];
        float curDist;
        idVec3 turretPos;
        idVec3 targetPos;

        if (!ent)
            continue;

		int targetType = IsTargetValid(ent);		
		if (targetType == TARG_INVALIDTARGET)
			continue;

        curDist = (GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin()).LengthFast();

        //Do blind spot check. Don't target things directly below the turret.
        targetPos = ent->GetPhysics()->GetOrigin();
        turretPos = GetPhysics()->GetOrigin();
        if (targetPos.x > turretPos.x - BLINDSPOT_SIZE && targetPos.x < turretPos.x + BLINDSPOT_SIZE && targetPos.y > turretPos.y - BLINDSPOT_SIZE && targetPos.y < turretPos.y + BLINDSPOT_SIZE)
        {
            continue;
        }


		//VIEWING ARC. Only allow turret to look in front of itself.
		if (team == TEAM_ENEMY) //we do a little cheat here. We only do the targeting cone check if the turret is hostile.
		{
			idVec3 entityCenterMass = ent->GetPhysics()->GetAbsBounds().GetCenter();
			idVec3 dirToEntity = targetPos - idVec3(turretPos.x, turretPos.y, targetPos.z);
			dirToEntity.Normalize();
			float facingResult = DotProduct(dirToEntity, GetFacingAngle());

			if (facingResult < ((targetType == TARG_ACTORTARGET) ? TARGETING_VIEWTHRESHOLD : TARGETING_VIEWTHRESHOLD_OBJECTS))
				continue;
		}

        if (curDist >= closestDist || curDist >= MAX_DISTANCE) //Do distance checks
            continue;

		idVec3 potentialHitPos = FindTargetPos(ent);
		if (potentialHitPos == vec3_zero) //Do LOS check.
            continue;

        //Found a new candidate.
        closestDist = curDist;
        closestIndex = i;
    }

    if (closestIndex < 0)
    {
        return NULL;
    }

    return entityList[closestIndex]; //Success. Found a valid target.
}



//Attempt to find a valid spot to hit a target ent. Return 0 0 0 if fail.
idVec3 idTurret::FindTargetPos(idEntity *targetEnt)
{
    //Try to find a spot that we can hit this ent.
    
	if (targetEnt == NULL)
	{
		return vec3_zero;
	}

	if (targetEnt->IsType(idPlayer::Type))
	{
		if (static_cast<idPlayer *>(targetEnt)->noclip || static_cast<idPlayer *>(targetEnt)->fl.notarget)
		{
			return vec3_zero;
		}
	}


	//Blindspot check.
	idVec3 targetPos = targetEnt->GetPhysics()->GetOrigin();
	idVec3 turretPos = GetPhysics()->GetOrigin();
	if (targetPos.x > turretPos.x - BLINDSPOT_SIZE && targetPos.x < turretPos.x + BLINDSPOT_SIZE && targetPos.y > turretPos.y - BLINDSPOT_SIZE && targetPos.y < turretPos.y + BLINDSPOT_SIZE)
	{
		//in blind spot.
		return vec3_zero;
	}



    //Do a few checks.
    idBounds bounds = targetEnt->GetPhysics()->GetBounds();
    idVec3			tmax;
    idVec3			targetCandidate;

    tmax[2] = bounds[1][2];

    //Attempt center mass.
    targetCandidate = targetEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, tmax[2] / 2.0f);
    if (CheckTargetLOS(targetEnt, targetCandidate))
    {
        return targetCandidate; //Successful hit on center mass.
    }

    //If it's an actor, try to shoot at their eyes. We don't do this to be gross; we do this because if the player has LOS to the turret, the player will expect the turret to likewise have LOS to the player.
    if (targetEnt->IsType(idActor::Type))
    {
        targetCandidate = static_cast<idActor *>(targetEnt)->GetEyePosition();
        if (CheckTargetLOS(targetEnt, targetCandidate))
        {
            return targetCandidate; //Successful hit on eyes.
        }
    }

    //Attempt origin shot.
    targetCandidate = targetEnt->GetPhysics()->GetOrigin();
    if (CheckTargetLOS(targetEnt, targetCandidate))
    {
        return targetCandidate; //Successful hit on center mass.
    }

    return vec3_zero; //Fail. Return 0 0 0.
}

//LOS check from my origin to the target.
bool idTurret::CheckTargetLOS(idEntity *ent, idVec3 hitPos)
{
    trace_t tr;
    idVec3 selfPos = GetPhysics()->GetOrigin() + idVec3(0, 0, MUZZLE_HEIGHT);//Find a point that's roughly where turret muzzle is.
    gameLocal.clip.TracePoint(tr, selfPos, hitPos, MASK_SOLID | MASK_SHOT_RENDERMODEL | CONTENTS_BODY, this);

    return (tr.c.entityNum == ent->entityNumber); //TRUE = we have clear LOS to the ent.
}

void idTurret::UpdateRotation(float _targetYaw)
{
	idRotation bodyRotation;
	jointHandle_t bodyJoint;
	float	delta_yaw;
	float adjustedYaw;

	targetYaw = _targetYaw;

	if (targetYaw < 0)
		targetYaw += 360.0f;
	else if (targetYaw > 360)
		targetYaw -= 360.0f;

	// Get the shortest  angle towards the target angle.
	delta_yaw = bodyYaw - targetYaw;
	if (idMath::Fabs(delta_yaw) > 180.f)
	{
		if (delta_yaw > 0)
		{
			delta_yaw = delta_yaw - 360;
		}
		else
		{
			delta_yaw = delta_yaw + 360;
		}
	}

	float rotationSpeed = MS2SEC(gameLocal.msec)*TURRET_ROTATION_SPEED_DEGSEC;
	if (delta_yaw > 0)
	{
		delta_yaw = Min(delta_yaw,rotationSpeed);
	}
	else
	{
		delta_yaw = Max(delta_yaw,-rotationSpeed);
	}

	adjustedYaw = bodyYaw;

	if (adjustedYaw > 180)
		adjustedYaw -= 180;
	else
		adjustedYaw += 180;

	if (idMath::Fabs(adjustedYaw - targetYaw) < 5)
		return;

	bodyYaw += delta_yaw;

	if (bodyYaw < 0)
		bodyYaw += 360.0f;
	else if (bodyYaw > 360)
		bodyYaw -= 360.0f;

	bodyJoint = animator.GetJointHandle("body");
	bodyRotation.SetVec(bodyAxis[1]);
	bodyRotation.SetAngle(bodyYaw + 180);
	animator.SetJointAxis(bodyJoint, JOINTMOD_WORLD, bodyRotation.ToMat3());
}

void idTurret::Think(void)
{
	if (state == TURRET_DEAD)
	{
		return;
	}

	if (lastTeam != team)
	{
		lastTeam = team;

		if (team == TEAM_FRIENDLY)
		{
			beamStart->SetColor(FRIENDLYCOLOR);
			laserdot->SetColor(FRIENDLYCOLOR);
			SetColor(FRIENDLYCOLOR);
			spotlight->SetColor(FRIENDLYCOLOR);
			headlight.shaderParms[0] = FRIENDLYCOLOR.x;
			headlight.shaderParms[1] = FRIENDLYCOLOR.y;
			headlight.shaderParms[2] = FRIENDLYCOLOR.z;
		}
		else
		{
			beamStart->SetColor(ENEMYCOLOR);
			laserdot->SetColor(ENEMYCOLOR);
			SetColor(ENEMYCOLOR);
			spotlight->SetColor(ENEMYCOLOR);
			headlight.shaderParms[0] = ENEMYCOLOR.x;
			headlight.shaderParms[1] = ENEMYCOLOR.y;
			headlight.shaderParms[2] = ENEMYCOLOR.z;
		}

		UpdateVisuals();

		if (gameLocal.time > 1000) //Skip sound if we're at game start.
		{
			StartSound("snd_teamswitch", SND_CHANNEL_ANY, 0, false, NULL);
		}
	}

	UpdatePitch();
	idAnimatedEntity::Think();

	//Move the laser aim target.
	if (laserAimIsMoving)
	{
		float lerp = (laserAimtimer - gameLocal.time) / (float)laserAimMaxtime;
		lerp = 1.0f - lerp;

		laserAimPos.Lerp(laserAimPosStart, laserAimPosEnd, lerp);

		if (lerp >= 1.0f)
		{
			laserAimPos = laserAimPosEnd;
			laserAimIsMoving = false;
		}
	}

	if (headlightHandle != -1)
	{
		idVec3 forward = GetMuzzlePos() - idVec3(this->GetPhysics()->GetOrigin().x, this->GetPhysics()->GetOrigin().y, GetMuzzlePos().z);
		forward.NormalizeFast();
		headlight.origin = GetMuzzlePos() + (forward * HEADLIGHT_FORWARDOFFSET);
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}

	if (state == TURRET_OPENING)
	{
		//Unfolding out of the ceiling.

		if (gameLocal.time > nextStateTime)
		{
			ActivateLights();
			idleSwayTimer = gameLocal.time + IDLE_MAXSWAYTIME;
			lastTargetNum = -1;
			state = TURRET_IDLE; //Go to idle state.
		}
	}
	else if (state == TURRET_IDLE)
	{
		//Searching for any available targets. Doing idle animations.

		//Idle behavior.
		idVec3 facingAngle, muzzlePos;
		muzzlePos = GetMuzzlePos();
		facingAngle = muzzlePos - idVec3(this->GetPhysics()->GetOrigin().x, this->GetPhysics()->GetOrigin().y, muzzlePos.z);
		facingAngle.Normalize();
		//facingAngle.ToAngles().ToVectors(&forward, &right, NULL);


		if (gameLocal.time >= idleVOTimer)
		{
			idleVOTimer = gameLocal.time + gameLocal.random.RandomInt(IDLEVO_MINTIME, IDLEVO_MAXTIME);
			StartSound("snd_vo_search", SND_CHANNEL_VOICE);
			DoSoundParticle();
		}

		FindTargetUpdate();

		//Idle anims.
		if (this->nextIdleSound < gameLocal.time)
		{
			int rand = gameLocal.random.RandomInt(3);
			StartSound("snd_idle", SND_CHANNEL_ANY, 0, false, NULL);
			Event_PlayAnim(idStr::Format("idle%d", rand), 4);			
			this->nextIdleSound = gameLocal.time + IDLESOUNDDELAY + (gameLocal.random.RandomFloat() * IDLESOUNDRAND);
		}
		
		if (lastTargetNum < 0 && !laserAimIsMoving)		 //If NO target and I'm not doing a lerp move.
		{
			//Idle sway.
			idVec3 newAimPos = laserAimPos;
			if (lastTargetPos == vec3_zero)
				newAimPos = GetDefaultLaserAimAngle(facingAngle);
			else
				newAimPos = lastTargetPos;

			

			//Lerp the sway anim, so that it doesn't suddenly snap into place.
			#define MAXSWAYDISTANCE 16.0f
			float swayLerpAmount = 1.0f;
			if (idleSwayTimer > gameLocal.time)
			{
				float swayLerp = (idleSwayTimer - gameLocal.time) / (float)IDLE_MAXSWAYTIME;
				swayLerp = min(1.0f - swayLerp, 1.0f);
				//swayAmount = idMath::Lerp(0.1f, MAXSWAYDISTANCE, swayLerp);				

				//common->Printf("%f\n", swayAmount);
				swayLerpAmount = swayLerp;
			}

			idVec3 desiredLaserPos = idVec3(
				newAimPos.x + idMath::Sin(gameLocal.time * 0.001f) * MAXSWAYDISTANCE,
				newAimPos.y + idMath::Sin(gameLocal.time * 0.0005f) * MAXSWAYDISTANCE,
				newAimPos.z + idMath::Sin(gameLocal.time * 0.0008f) * MAXSWAYDISTANCE);

			laserAimPos.Lerp(newAimPos, desiredLaserPos, swayLerpAmount);
		}
		else if (lastTargetNum >= 0 && !laserAimIsMoving)
		{
			//I have a target sighted.
			laserAimPos.Lerp(beamEnd->GetPhysics()->GetOrigin(), this->GetPhysics()->GetOrigin() + (facingAngle * 1024) + idVec3(0, 0, -256), LASER_MOVESPEED);
		}

		UpdateLaserCollision();

		RotateTowardTarget();
	}
	else if (state == TURRET_TARGETING)
	{

		//Make the laser point in same direction as the gunbarrel.
		idVec3 facingAngle, muzzlePos;
		muzzlePos = GetMuzzlePos();
		facingAngle = muzzlePos - idVec3(this->GetPhysics()->GetOrigin().x, this->GetPhysics()->GetOrigin().y, muzzlePos.z);
		facingAngle.Normalize();
		idVec3 laserFacingPos = GetDefaultLaserAimAngle(facingAngle);
		lastTargetPos = laserAimPosEnd;

		//I have a target. I am turning toward the target.
		//If target leaves my LOS, then I'll exit this state.
		if(g_turret_trackingtype.GetInteger() == 0 )
		{
			UpdateLaserCollision();
			RotateTowardTarget();
		//laserAimPos = this->GetPhysics()->GetOrigin() + (facingAngle * 1024) + idVec3(0, 0, -256);
			MoveLaserAim(GetDefaultLaserAimAngle(laserFacingPos), 300);
		}
		else
		{
			AimToPositionUpdate(targetEnt.IsValid() ? targetEnt.GetEntity()->GetPhysics()->GetOrigin() : lastTargetPos);
		}


		if (gameLocal.time >= nextStateTime)
		{
			bool targetIsValid = true;

			if (!targetEnt.IsValid())
				targetIsValid = false;

			if (targetIsValid)
			{
				if (targetEnt.GetEntity()->IsType(idPlayer::Type))
				{
					if (static_cast<idPlayer *>(targetEnt.GetEntity())->noclip || static_cast<idPlayer *>(targetEnt.GetEntity())->fl.notarget)
					{
						targetIsValid = false;
					}
				}
			}

			if (!targetIsValid)
			{
				//Has lost target. Exit the targeting state.
				lastTargetNum = -1;
				StartSound("snd_lostsight", SND_CHANNEL_ANY, 0, false, NULL);
				StartSound("snd_vo_targetlost", SND_CHANNEL_VOICE);
				
				//return to idle state.
				targetEnt = (idEntity*)nullptr;
				idleSwayTimer = gameLocal.time + IDLE_MAXSWAYTIME;
				this->state = TURRET_IDLE;
				this->nextFindTime = 0;

				return;
			}

			//Check if target is in my FOV.
			idVec3 dirToEntity = GetPhysics()->GetOrigin() - idVec3(targetEnt.GetEntity()->GetPhysics()->GetOrigin().x, targetEnt.GetEntity()->GetPhysics()->GetOrigin().y, GetPhysics()->GetOrigin().z);
			dirToEntity.Normalize();
			idVec3 turretDirection = GetMuzzlePos() - idVec3(GetPhysics()->GetOrigin().x, GetPhysics()->GetOrigin().y, GetMuzzlePos().z);
			turretDirection.Normalize();
			float facingResult = DotProduct(dirToEntity, turretDirection);
			if (facingResult > FOV_DOTPRODUCT) //Is not close enough to being within FOV.
				return;			
			
			//Gunbarrel is facing target. Target is in my firing cone FOV.

			//Do LOS check.
			idVec3 potentialHitPos = FindTargetPos(targetEnt.GetEntity());
			if (potentialHitPos == vec3_zero)
			{
				//No LOS. 
				lastTargetNum = -1;
				StartSound("snd_lostsight", SND_CHANNEL_ANY, 0, false, NULL);
				StartSound("snd_vo_targetlost", SND_CHANNEL_VOICE);

				//return to idle state.
				targetEnt = (idEntity*)nullptr;
				idleSwayTimer = gameLocal.time + IDLE_MAXSWAYTIME;
				this->state = TURRET_IDLE;
				this->nextFindTime = 0;

				return;
			}

			//Ready to start the attack sequence. Target valid, ready to transition to combat, all systems go. Proceed to Warmup state.
			state = TURRET_WARMUP;
			StartSound("snd_warmup", SND_CHANNEL_ANY, 0, false, NULL);
			StartSound("snd_vo_firing", SND_CHANNEL_VOICE);
			this->nextStateTime = gameLocal.time + WARMUPTIME;

			//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPPositionByEntity(targetEnt.GetEntity());
			//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false);

			if (team == TEAM_ENEMY)
			{
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->AlertAIFriends(this);
			}
				

			if (targetEnt.IsValid())
			{
				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_turret_see"), targetEnt.GetEntity()->displayName.c_str()), GetPhysics()->GetOrigin());
			}
			
		}
	}
	else if (state == TURRET_WARMUP)
	{
		//Target is locked, I am warming up gun, ready to fire upon target. My gun will fire imminently!!!
		//If target leaves my LOS, it doesn't matter -- I'll still fire my gun at the last-seen position. I cannot cancel out of this state.

		if (targetEnt.IsValid())
		{
			idVec3 potentialHitPos = FindTargetPos(targetEnt.GetEntity());
			if (potentialHitPos != vec3_zero)
			{
				lastTargetPos = potentialHitPos;
			}
		}

		if(g_turret_trackingtype.GetInteger() == 0 )
		{
			RotateTowardTarget();

			laserAimPos.Lerp(beamEnd->GetPhysics()->GetOrigin(), lastTargetPos, LASER_MOVESPEED);

			UpdateLaserCollision();
		}
		else
		{
			AimToPositionUpdate(lastTargetPos);
		}

		if (gameLocal.time > nextStateTime)
		{
			//Go to volley mode.
			state = TURRET_VOLLEYING;
			this->volleyCount = 0;
			this->nextStateTime = 0; //make it fire immediately.
		}
	}
	else if (state == TURRET_VOLLEYING)
	{
		if(g_turret_trackingtype.GetInteger() == 0 )
		{
			//Firing weapon. I'll fire a few volleys, and then return to idle state.

			RotateTowardTarget();

			//Move laser.
			laserAimPos.Lerp(beamEnd->GetPhysics()->GetOrigin(), lastTargetPos, LASER_MOVESPEED);
			UpdateLaserCollision();
		}
		else
		{
			AimToPositionUpdate(lastTargetPos);
		}

		//Fire the cannon.
		if (gameLocal.time > nextStateTime)
		{
			idVec3 potentialHitPos = vec3_zero;
			
			if (targetEnt.IsValid())
			{				
				potentialHitPos = FindTargetPos(targetEnt.GetEntity());

				if (potentialHitPos != vec3_zero)
				{
					lastTargetPos = potentialHitPos;
				}
			}

			idProjectile *bullet;
			const idDict *	projectileDef;
			idEntity *		bulletEnt;
			idVec3 bulletVelocity;
			idVec3 bulletTrajectory;
			idVec3 bulletSpawnPos = GetPhysics()->GetOrigin() + idVec3(0, 0, MUZZLE_HEIGHT); //We're spawning the bullet from the center of the turret, not the muzzle. This makes it a little easier because this makes the bulletspawn point a static, non-moving point.			

			//bulletTrajectory = lastTargetPos - bulletSpawnPos;
			bulletTrajectory = laserAimPos - bulletSpawnPos;
			bulletTrajectory.Normalize();

			projectileDef = gameLocal.FindEntityDefDict(spawnArgs.GetString("projectile", "projectile_turretbullet"), false);
			bulletVelocity = projectileDef->GetVector("velocity");
			gameLocal.SpawnEntityDef(*projectileDef, &bulletEnt, false);
			bullet = (idProjectile *)bulletEnt;
			bullet->Create(this, bulletSpawnPos, bulletTrajectory);
			bullet->Launch(bulletSpawnPos, bulletTrajectory, bulletTrajectory * bulletVelocity.x);
			

			
			//play animation.
			Event_PlayAnim("fire", 4);

			//play sound.
			StartSound("snd_fire", SND_CHANNEL_ANY, 0, false, NULL);


			//muzzle flash skin.
			SetSkin(declManager->FindSkin("skins/turret/firing"));
			PostEventMS(&EV_Turret_muzzleflashoff, MUZZLEFLASHTIME);
		
			if (g_showBrass.GetBool())
			{
				idEntity *brassEnt;

				gameLocal.SpawnEntityDef(brassDict, &brassEnt, false);

				if (brassEnt && brassEnt->IsType(idDebris::Type))
				{
					idDebris *debris = static_cast<idDebris *>(brassEnt);
					idAngles turretDir = turretAxis.ToAngles();
					idVec3 brassSpawnPos;

					turretDir.pitch = 0;
					turretDir.yaw += 90;

					brassSpawnPos = GetPhysics()->GetOrigin() + (turretDir.ToForward() * -13);

					debris->Create(this, brassSpawnPos, mat3_identity);
					debris->Launch();

					//Spin brass.
					debris->GetPhysics()->SetAngularVelocity(idVec3(400 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat()));

					//Spit out brass.
					turretDir.yaw += gameLocal.random.CRandomFloat() * 40;
					debris->GetPhysics()->SetLinearVelocity(turretDir.ToForward() * (-64 + gameLocal.random.RandomInt(32)));
				}
			}

			this->volleyCount++;
			if (this->volleyCount >= MAXVOLLEY)
			{
				idleSwayTimer = gameLocal.time + IDLE_MAXSWAYTIME;
				this->state = TURRET_IDLE;
				this->nextFindTime = 0;

				

				return;
			}

			this->nextStateTime = gameLocal.time + VOLLEYDELAY;
		}
	}
	else if (state == TURRET_DAMAGESTATE)
	{
		//Damage was inflicted on me. I'm doing a damage cooldown (PAIN_COOLDOWNTIME).

		if (gameLocal.time >= nextStateTime)
		{
			idleSwayTimer = gameLocal.time + IDLE_MAXSWAYTIME;
			this->state = TURRET_SEARCH;
			this->nextFindTime = 0;
			nextStateTime = gameLocal.time;
			StartSound("snd_sighted", SND_CHANNEL_ANY, 0, false, NULL);
			DoSoundParticle();
			Event_PlayAnim("idle0", 1); // reset anim if still in damage state
			return;
		}
	}
	else if (state == TURRET_SEARCH)
	{
		const int SEARCH_INSPECT_TIME = 100.0f; 
		const int MAX_SEARCH_TIME = 3000.0f;
		if (gameLocal.time >= nextStateTime)
		{
			if(FindTargetUpdate())
			{
			}
			else if( inflictorEnt.IsValid() ) 
			{ // search for damage inflictor
				idVec3 inflictorPos = inflictorEnt.GetEntity()->GetPhysics()->GetOrigin();
				if( AimToPositionUpdate(inflictorPos,MAX_SEARCH_TIME))
				{
					StartSound("snd_vo_search", SND_CHANNEL_VOICE);
					StartSound("snd_sighted", SND_CHANNEL_ANY, 0, false, NULL);
					DoSoundParticle();
					inflictorEnt = (idEntity*)nullptr;
					nextStateTime =  gameLocal.time + SEARCH_INSPECT_TIME;
					if(searchEnt.IsValid())
					{
						lastTargetPos = searchEnt.GetEntity()->GetPhysics()->GetOrigin();
					}
				}
			}
			else if( searchEnt.IsValid() )
			{ // search for attacker
				//if( SearchPositionUpdate(lastTargetPos,MAX_SEARCH_TIME))
				{
					lastTargetNum = searchEnt.GetEntityNum();
					targetEnt = searchEnt;
					//nextStateTime =  gameLocal.time + SEARCH_INSPECT_TIME;
					searchEnt = (idEntity*)nullptr;
					state = TURRET_TARGETING;
				}
			}
			else
			{
				state = TURRET_IDLE;
			}
		}
	}

	

	//gameRenderWorld->DebugArrow(colorGreen, beamStart->GetPhysics()->GetOrigin(), beamEnd->GetPhysics()->GetOrigin(), 4, 10);
}

bool idTurret::FindTargetUpdate()
{
	//Searching for target.
	if (gameLocal.time > nextFindTime)
	{
		nextFindTime = gameLocal.time + FINDFREQUENCY; //How often to search for targets.

		targetEnt = FindTarget();

		if (targetEnt.IsValid())
		{
			if (targetEnt.GetEntityNum() != lastTargetNum)
			{
				//Acquired a new target.
				lastTargetNum = targetEnt.GetEntityNum();
				StartSound("snd_sighted", SND_CHANNEL_ANY, 0, false, NULL);
				StartSound("snd_vo_sighted", SND_CHANNEL_VOICE);


				//Make particle fx play.
				DoSoundParticle();
			}

			nextStateTime = gameLocal.time + TARGETING_LOCKTIME;
			state = TURRET_TARGETING;
			return true;
		}
		else if (lastTargetNum != -1)
		{
			lastTargetNum = -1;
			StartSound("snd_lostsight", SND_CHANNEL_ANY, 0, false, NULL);


			lastTargetPos = laserAimPos;
			idleSwayTimer = gameLocal.time + IDLE_MAXSWAYTIME;

			//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false); //lost target. update the LKP system.
		}
	}
	return false;
}

bool idTurret::AimToPositionUpdate(idVec3 searchPos, float timeOut)
{
	if( timeOut >= 0 )
	{
		bool exceededSearchTime = gameLocal.time  > (nextStateTime + timeOut);
		if(exceededSearchTime)
		{
			return true;
		}
	}

	idVec3 facingDir = GetFacingAngle();
	idVec3 dirToTarget = searchPos - GetPhysics()->GetOrigin();
	dirToTarget.NormalizeFast();

	// get 2d rot for turret rotation
	idVec2 dirToTargetPlane = dirToTarget.ToVec2();
	idVec2 facingDirPlane = facingDir.ToVec2();
	facingDirPlane.NormalizeFast();
	dirToTargetPlane.NormalizeFast();

	float targetDotPlane = dirToTargetPlane*facingDirPlane;

	const float rotFinishedAngleDot = 0.99f;
	bool rotationReached = targetDotPlane > rotFinishedAngleDot;
	if(!rotationReached)
	{
		RotateTowardTarget(searchPos);
	}

	idVec3 laserDir = laserAimPos - GetPhysics()->GetOrigin();
	laserDir.NormalizeFast();
	float laserDotProd = facingDir*laserDir;
	bool laserReached = (laserDotProd > rotFinishedAngleDot);
	if(!laserReached)
	{
		if( rotationReached )
		{
			float targetDot = laserDir*dirToTarget;
			float degreesToTarget = RAD2DEG( idMath::ACos(targetDot) );
			float timeToMove = SEC2MS(degreesToTarget/LASER_ROTATION_SPEED_DEGSEC);
			MoveLaserAim(searchPos, timeToMove);
		}
		else
		{
			idVec3 constrainedPos = ConstrainToTurretDir(searchPos,LASER_YAW_MAX,LASER_PITCH_MAX);
			idVec3 constrainedDir = constrainedPos-GetMuzzlePos();
			constrainedDir.NormalizeFast();
	
			float targetDot = laserDir*constrainedDir;
			float degreesToTarget = RAD2DEG( idMath::ACos(targetDot) );
	
			float timeToMove = SEC2MS(degreesToTarget/LASER_ROTATION_SPEED_DEGSEC);

			MoveLaserAim(constrainedPos, timeToMove);
		}
	}
	
	UpdateLaserCollision();
	return (rotationReached && laserReached);
}

idVec3 idTurret::ConstrainToTurretDir(idVec3 targetPos, float maxYaw, float maxPitch)
{
	idVec3 muzzlePos = GetMuzzlePos();
	idVec3 targetVec = (targetPos-muzzlePos);
	idVec3 facingDir = GetFacingAngle();
	idAngles facingAngles = facingDir.ToAngles();
	idAngles targetAngles = targetVec.ToAngles();
	facingAngles.Normalize180();
	targetAngles.Normalize180();
	idAngles facingToTargetAngles =	targetAngles-facingAngles;

	idAngles clamp = idAngles(maxPitch,maxYaw,0.0f);
	idAngles clampNeg = idAngles(-maxPitch,-maxYaw,0.0f);

	facingToTargetAngles.Clamp(clampNeg,clamp);

	idVec3 newDir = (facingToTargetAngles+facingAngles).ToForward();

	return newDir*targetVec.LengthFast() + muzzlePos;
}

void idTurret::UpdatePitch()
{
	jointHandle_t turretJoint;
	idAngles turretRotation;
	idVec3 muzzlePos;
	idVec3 turretDir;
	idMat3 muzzleAxis;

	turretJoint = animator.GetJointHandle("turret");
	GetJointWorldTransform(turretJoint, gameLocal.time, muzzlePos, muzzleAxis);
	turretDir = GetLocalVector( laserAimPos - muzzlePos );
	turretDir.Normalize();
	turretRotation = turretDir.ToAngles();

	turretRotation.yaw = 0;
	turretRotation.roll = 0;
	// Janky to do it this way, but it works
	if ( bodyYaw < 90.0f || bodyYaw > 270.0f )
	{
		turretRotation.pitch *= -1.0f;
	}
	turretRotation.Normalize360();

	animator.SetJointAxis(turretJoint, JOINTMOD_WORLD, turretRotation.ToMat3());
}

idVec3 idTurret::GetEntityCenter(idEntity *ent)
{
	return idVec3(ent->GetPhysics()->GetOrigin().x, ent->GetPhysics()->GetOrigin().y, ent->GetPhysics()->GetAbsBounds().GetCenter().z);
}

idVec3 idTurret::GetMuzzlePos()
{
	idVec3 muzzlePos;
	jointHandle_t muzzleJoint;

	muzzleJoint = animator.GetJointHandle("turret"); //Get muzzle position.
	animator.GetJointTransform(muzzleJoint, gameLocal.time, muzzlePos, this->turretAxis);
	return (this->GetPhysics()->GetOrigin() + muzzlePos * this->GetPhysics()->GetAxis());
}

void idTurret::RotateTowardTarget()
{
	if (targetEnt.IsValid())
	{
		bool doRotation = true;
		if (targetEnt.GetEntity()->IsType(idPlayer::Type))
		{
			if (static_cast<idPlayer *>(targetEnt.GetEntity())->noclip || static_cast<idPlayer *>(targetEnt.GetEntity())->fl.notarget)
			{
				doRotation = false;
			}
		}

		RotateTowardTarget(targetEnt.GetEntity()->GetPhysics()->GetOrigin());
	}
	else if (playingRotationSound)
	{
		playingRotationSound = false;
		StopSound(SND_CHANNEL_RADIO);
	}
}

void idTurret::RotateTowardTarget(idVec3 targetPos)
{
	bool shouldPlayRotationSound = false;

	//We have a target. Rotate toward it.
	idVec3 diff = targetPos - this->GetPhysics()->GetOrigin();
	float yaw = diff.ToYaw();
	yaw -= this->GetPhysics()->GetAxis().ToAngles().yaw;
	UpdateRotation(yaw);

	if (state == TURRET_IDLE || state == TURRET_TARGETING || state == TURRET_SEARCH)
	{
		shouldPlayRotationSound = true;
	}

	if (shouldPlayRotationSound)
	{
		if (!playingRotationSound)
		{
			playingRotationSound = true;
			StartSound("snd_rotate", SND_CHANNEL_RADIO);
			StartSound("snd_rotate_start", SND_CHANNEL_ANY);
		}
	}
	else
	{
		if (playingRotationSound)
		{
			playingRotationSound = false;
			StopSound(SND_CHANNEL_RADIO);
		}
	}
	
}

void idTurret::MoveLaserAim(idVec3 newPos, int movetime)
{
	laserAimPosStart = laserAimPos;
	laserAimPosEnd = newPos;
	laserAimtimer = gameLocal.time + movetime;
	laserAimMaxtime = movetime;
	laserAimIsMoving = true;
}

void idTurret::UpdateLaserCollision()
{
	trace_t idleTr;
	idVec3 muzzlePos, laserDir;
	jointHandle_t muzzleJoint;

	muzzleJoint = animator.GetJointHandle("turret"); //Get muzzle position.
	animator.GetJointTransform(muzzleJoint, gameLocal.time, muzzlePos, this->turretAxis);
	muzzlePos = this->GetPhysics()->GetOrigin() + muzzlePos * this->GetPhysics()->GetAxis();

	laserDir = laserAimPos - muzzlePos;
	laserDir.NormalizeFast();

	gameLocal.clip.TracePoint(idleTr, muzzlePos, muzzlePos + laserDir * 1536, MASK_SOLID | MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX, this);
	this->beamEnd->SetOrigin(idleTr.endpos);
	laserdot->SetAxis(idleTr.c.normal.ToMat3());
}

bool idTurret::Pain(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	const char *fx = spawnArgs.GetString("fx_damage");
	if (fx[0] != '\0')
	{
		idAngles particleAng = idAngles(110 + gameLocal.random.RandomInt(60), gameLocal.random.RandomInt(359), 0);
		idEntityFx::StartFx(fx, GetPhysics()->GetOrigin(), particleAng.ToMat3(), this, true);
	}

	StopSound(SND_CHANNEL_ANY, false);
	StartSound("snd_pain", SND_CHANNEL_VOICE2, 0, false, NULL);
	StartSound("snd_vo_pain", SND_CHANNEL_VOICE);
	Event_PlayAnim("pain", 1, false);

	if (state != TURRET_OFF && state != TURRET_DEAD)
	{
		ActivateLights(); // force activate lights as it might be in opening state

		//setup turn toward the attacker after damage state. This is so if someone throws something / attacks the turret, the turret turns to look at the attacker.
		if(inflictor != nullptr && inflictor->IsType(idItem::Type))
		{
			inflictorEnt = inflictor;
		}
		if (attacker != nullptr && (state == TURRET_IDLE || state == TURRET_OPENING) )
		{
			if (attacker->team != this->team)
			{
				lastTargetPos = attacker->GetPhysics()->GetOrigin();
				searchEnt = attacker;
				targetEnt = (idEntity*)nullptr;
			}
		}
	
		state = TURRET_DAMAGESTATE;
		nextStateTime = gameLocal.time + PAIN_COOLDOWNTIME;

		if (g_onehitkill.GetBool() && team == TEAM_ENEMY)
		{
			Damage(inflictor, attacker, dir, "damage_1000", 1.0f, 0);
			//Killed(inflictor, attacker, damage, dir, location);
		}

		return true;
	}

	return false;
}

void idTurret::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (state == TURRET_DEAD)
		return;

	gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("def_deathinterest"));

	gameLocal.AddEventlogDeath(this, damage, inflictor, attacker, "", EL_DESTROYED);

	state = TURRET_DEAD;
	BecomeInactive(TH_THINK);

	this->SetSkin(declManager->FindSkin( spawnArgs.GetString("skin_death") ));

	StopSound(SND_CHANNEL_ANY, false);
	const char *fx = spawnArgs.GetString("fx_destroyed");
	if (fx[0] != '\0')
	{
		idEntityFx::StartFx(fx, GetPhysics()->GetOrigin() + idVec3(0,0, MUZZLE_HEIGHT), mat3_identity);
	}

	Event_PlayAnim("deathidle", 4, true);	
	this->beamStart->Hide(); //Hide laser.
	this->laserdot->Hide(); //Hide laser.
	SetLight(false);
	fl.takedamage = false;
	needsRepair = true;
	GetPhysics()->SetContents(0);

	idMoveableItem::DropItemsBurst(this, "gib", idVec3(0, 0, -8), 16);

	idMoveableItem::DropItemsBurst(this, "ammo", idVec3(0, 0, -8), 16);

	// SW 18th Feb 2025: moving Present() call to after the SetLight() call so that the vision cone correctly hides
	Present(); //HAVE TO call Present() here in order for skin update to appear.

}

// SW 12th Feb 2025: adding support for turrets to be repaired
void idTurret::DoRepairTick(int amount)
{
	// first things first, restore its health
	health = spawnArgs.GetInt("health", "9");

	// if turret is actually dead, we need to put it back into a non-killed state
	if (state == TURRET_DEAD)
	{
		state = TURRET_OFF;
		SetSkin(declManager->FindSkin("skins/turret/skin"));
		fl.takedamage = false;
		renderEntity.shaderParms[SHADERPARM_VISIONCONE] = 0;
		GetPhysics()->SetContents(CONTENTS_SOLID);
		BecomeActive(TH_THINK);


		// Do we need to be deployed? (e.g. if we're repaired during a combat state, or if we're friendly)
		if (electricalActive && (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->combatMetastate != COMBATSTATE_IDLE || team == TEAM_FRIENDLY))
		{
			// Activate ourselves
			Event_activate(1);
		}
		else
		{
			// go to idle closed
			Event_PlayAnim("closed", 4, true);

			//BC 2-21-2025: make the turret uninspectable when the doors are open.
			SetInspectable(true);
		}
	}
}

bool idTurret::IsOn()
{
	if (state != TURRET_OFF && state != TURRET_OPENING && state != TURRET_DEAD)
		return true;

	return false;
}

bool idTurret::IsInCombat()
{
	return (state == TURRET_TARGETING || state == TURRET_WARMUP || state == TURRET_VOLLEYING || state == TURRET_DAMAGESTATE);
}

void idTurret::DoSoundParticle()
{
	jointHandle_t turretJoint;
	idAngles turretRotation;
	idVec3 muzzlePos;
	idMat3 muzzleAxis;

	turretJoint = animator.GetJointHandle("turret");
	animator.GetJointTransform(turretJoint, gameLocal.time, muzzlePos, muzzleAxis);
	muzzlePos = this->GetPhysics()->GetOrigin() + muzzlePos * this->GetPhysics()->GetAxis();

	gameLocal.DoParticle("sound_burst_big.prt", muzzlePos);
}

void idTurret::SetElectricalActive(bool value)
{
	electricalActive = value;
}

idVec3 idTurret::GetDefaultLaserAimAngle(idVec3 facingAngle)
{
	return (this->GetPhysics()->GetOrigin() + (facingAngle * 1024) + idVec3(0, 0, -256));
}

idVec3 idTurret::GetFacingAngle()
{
	idVec3 muzzlePos = GetMuzzlePos();
	idVec3 forward = muzzlePos - idVec3(this->GetPhysics()->GetOrigin().x, this->GetPhysics()->GetOrigin().y, muzzlePos.z);
	forward.NormalizeFast();

	return forward;
}

void idTurret::DoHack()
{
	gameLocal.AddEventLog("#str_def_gameplay_turret_hack", GetPhysics()->GetOrigin());

	team = TEAM_FRIENDLY;

	if (state == TURRET_OFF)
	{
		//open it up.
		Event_activate(1);
	}
	else if(state != TURRET_DEAD)
	{
		StopSound(SND_CHANNEL_ANY, false);
		StartSound("snd_pain", SND_CHANNEL_VOICE2, 0, false, NULL);
		Event_PlayAnim("pain", 1, false);
		state = TURRET_DAMAGESTATE;
		nextStateTime = gameLocal.time + PAIN_COOLDOWNTIME;
	}

	// SW: for milestone scripting
	idStr hackScript;
	if (spawnArgs.GetString("callOnHack", "", hackScript))
	{
		gameLocal.RunMapScript(hackScript);
	}
}

//BC 2-21-2025: make the turret uninspectable when the doors are open.
void idTurret::SetInspectable(bool value)
{
	spawnArgs.SetBool("zoominspect", value);
}