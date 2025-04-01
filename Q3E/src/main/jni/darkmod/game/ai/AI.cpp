/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "../Game_local.h"
#include "Mind.h"
#include "Subsystem.h"
#include "MovementSubsystem.h"
#include "Memory.h"
#include "States/KnockedOutState.h"
#include "States/DeadState.h"
#include "States/ConversationState.h"
#include "States/PainState.h"
#include "States/IdleState.h"
#include "States/ObservantState.h"
#include "States/SuspiciousState.h"
#include "States/SearchingState.h"
#include "States/AgitatedSearchingState.h"
#include "States/CombatState.h"
#include "States/PocketPickedState.h"
#include "Tasks/SingleBarkTask.h"
#include "Tasks/HandleDoorTask.h" // grayman #3647
#include "Tasks/HandleElevatorTask.h" // grayman #3647
#include "Conversation/ConversationSystem.h"
#include "../Relations.h"
#include "../Objectives/MissionData.h"
#include "../StimResponse/StimResponseCollection.h"
#include "../AbsenceMarker.h"
#include "../BloodMarker.h"
#include "../DarkModGlobals.h"
#include "../MultiStateMover.h"
#include "../MeleeWeapon.h"
#include "../SndProp.h"
#include "../EscapePointManager.h"
#include "../PositionWithinRangeFinder.h"
#include "../TimerManager.h"
#include "../ProjectileResult.h" // grayman #2872
#include "../Grabber.h"
#include "../SearchManager.h"

// For handling the opening of doors and other binary Frob movers
#include "../BinaryFrobMover.h"
#include "../FrobDoor.h"
#include "../FrobDoorHandle.h"
#include "tdmAASFindEscape.h"
#include "AreaManager.h"

#include "EAS/EAS.h" // grayman #3548
#include "EAS/ElevatorStationInfo.h" // grayman #3548

#include "Tasks/PlayAnimationTask.h" // #3597

#include "game/LightEstimateSystem.h" // #6546

const int AUD_ALERT_DELAY_MIN =  500; // grayman #3356 - min amount of time delay (ms) before processing an audio alert
const int AUD_ALERT_DELAY_MAX = 1500; // grayman #3356 - max amount of time delay (ms) before processing an audio alert

//TODO: Move these to AI def:

// Visual detection parameters

/**
* amount of time to normalize by for the % check, in seconds
*  this gets modified by 1 / the cvar dm_ai_sight.
**/
static const float s_VisNormtime = 0.2f;

/**
* In pitch darkness, player is invisible
**/
// full bright minimum distance is 0
static const float s_VisFDMin = 0.0f;

// full bright maximum distance is zero (because probability is always zero anyway)
static const float s_VisFDMax = 0.0f;

// TODO: Move this to def file or INI
static const float s_AITactDist = 1.0f;

const float s_DOOM_TO_METERS = 0.0254f;

// TDM: Maximum flee distance for any AI
const float MAX_FLEE_DISTANCE = 10000.0f;

#define INITIAL_PICKPOCKET_DELAY  2000 // how long to initially wait before proceeding (ms)
#define LATCHED_PICKPOCKET_DELAY 60000 // how long to wait before seeing if a latch has been removed (ms)

#define MIN_SPEED_TO_NOTICE_MOVEABLE 20 // grayman #4304 - minimum speed to notice getting hit by a moveable

class CRelations;
class CsndProp;
class CDarkModPlayer;
class CMissionData;

static const char *moveCommandString[ NUM_MOVE_COMMANDS ] = {
	"MOVE_NONE",
	"MOVE_FACE_ENEMY",
	"MOVE_FACE_ENTITY",
	"MOVE_TO_ENEMY",
	"MOVE_TO_ENEMYHEIGHT",
	"MOVE_TO_ENTITY",
	"MOVE_OUT_OF_RANGE",
	"MOVE_TO_ATTACK_POSITION",
	"MOVE_TO_COVER",
	"MOVE_TO_POSITION",
	"MOVE_TO_POSITION_DIRECT",
	"MOVE_SLIDE_TO_POSITION",
	"MOVE_WANDER"
};

/*
============
idAASFindCover::idAASFindCover
============
*/
idAASFindCover::idAASFindCover( const idActor* hidingActor, const idEntity* hideFromEnt, const idVec3 &hideFromPos ) {
	this->hidingActor = hidingActor; // This should not be NULL
	this->hideFromEnt = hideFromEnt; // May be NULL
	this->hideFromPos = hideFromPos;
}

/*
============
idAASFindCover::~idAASFindCover
============
*/
idAASFindCover::~idAASFindCover() {
}

/*
============
idAASFindCover::TestArea
============
*/
bool idAASFindCover::TestArea( const idAAS *aas, int areaNum )
{
	idVec3	areaCenter;
	trace_t	trace, trace2;

	if (areaNum == aas->PointAreaNum(hidingActor->GetPhysics()->GetOrigin()))
	{
		// We're in this AAS area; assume that our current position can never be cover.
		// (If it was, we probably wouldn't be trying to move into cover.)
		return false;
	}

	// Get location of feet
	// Assumes they're at the centre of the bounding box in the X and Y axes,
	// but at the bottom in the Z axis.
	idBounds bounds = hidingActor->GetPhysics()->GetAbsBounds();
	idVec3 feet = (bounds[0] + bounds[1]) * 0.5f;
	feet.z = bounds[0].z;

	// Get location to trace to
	areaCenter = aas->AreaCenter( areaNum );

	// Adjust areaCenter to factor in height of AI, so we trace from the estimated eye position
	areaCenter += hidingActor->GetEyePosition() - feet;

	gameLocal.clip.TracePoint(trace, hideFromPos, areaCenter, MASK_OPAQUE, hideFromEnt);
	if (trace.fraction < 1.0f)
	{
		// The trace was interrupted, so this location is probably cover.
		//gameRenderWorld->DebugLine( colorGreen, hideFromPos, areaCenter, 5000, true);

		// But before we say for certain, let's look at the floor as well.
		areaCenter = aas->AreaCenter( areaNum );
		gameLocal.clip.TracePoint(trace2, hideFromPos, areaCenter, MASK_OPAQUE, hideFromEnt);
		if (trace2.fraction < 1.0f)
		{
			// Yes, the feet are hidden too, so this is almost certainly cover
			//gameRenderWorld->DebugLine( colorGreen, hideFromPos, areaCenter, 5000, true);
			return true;
		}

		// Oops; the head is hidden but the feet are not, so this isn't very good cover at all.
		//gameRenderWorld->DebugLine( colorRed, hideFromPos, areaCenter, 5000, true);
		return false;
	}

	// The trace found a clear path, so this location is not cover.
	//gameRenderWorld->DebugLine( colorRed, hideFromPos, areaCenter, 5000, true);
	return false;
}

/*
============
idAASFindAreaOutOfRange::idAASFindAreaOutOfRange
============
*/
idAASFindAreaOutOfRange::idAASFindAreaOutOfRange( const idVec3 &targetPos, float maxDist ) {
	this->targetPos		= targetPos;
	this->maxDistSqr	= maxDist * maxDist;
}

/*
============
idAASFindAreaOutOfRange::TestArea
============
*/
bool idAASFindAreaOutOfRange::TestArea( const idAAS *aas, int areaNum ) {
	const idVec3 &areaCenter = aas->AreaCenter( areaNum );
	trace_t	trace;
	float dist;

	dist = ( targetPos.ToVec2() - areaCenter.ToVec2() ).LengthSqr();

	if ( ( maxDistSqr > 0.0f ) && ( dist < maxDistSqr ) ) {
		return false;
	}

	gameLocal.clip.TracePoint( trace, targetPos, areaCenter + idVec3( 0.0f, 0.0f, 1.0f ), MASK_OPAQUE, NULL );
	if ( trace.fraction < 1.0f ) {
		return false;
	}

	return true;
}

/*
============
idAASFindAttackPosition::idAASFindAttackPosition
============
*/
idAASFindAttackPosition::idAASFindAttackPosition(idAI *self, const idMat3 &gravityAxis, idEntity *target, const idVec3 &targetPos, const idVec3 &fireOffset)
{
	int	numPVSAreas;

	this->target		= target;
	this->targetPos		= targetPos;
	this->fireOffset	= fireOffset;
	this->self			= self;
	this->gravityAxis	= gravityAxis;

	excludeBounds		= idBounds( idVec3( -64.0, -64.0f, -8.0f ), idVec3( 64.0, 64.0f, 64.0f ) );
	excludeBounds.TranslateSelf( self->GetPhysics()->GetOrigin() );

	// setup PVS
	idBounds bounds( targetPos - idVec3( 16, 16, 0 ), targetPos + idVec3( 16, 16, 64 ) );
	numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, idEntity::MAX_PVS_AREAS );
	targetPVS	= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
idAASFindAttackPosition::~idAASFindAttackPosition
============
*/
idAASFindAttackPosition::~idAASFindAttackPosition() {
	gameLocal.pvs.FreeCurrentPVS( targetPVS );
}

/*
============
idAASFindAttackPosition::TestArea
============
*/
bool idAASFindAttackPosition::TestArea( const idAAS *aas, int areaNum ) {
	idVec3	dir;
	idVec3	local_dir;
	idVec3	fromPos;
	idMat3	axis;
	idVec3	areaCenter;
	int		numPVSAreas;
	int		PVSAreas[ idEntity::MAX_PVS_AREAS ];

	areaCenter = aas->AreaCenter( areaNum );
	areaCenter[ 2 ] += 1.0f;

	if ( excludeBounds.ContainsPoint( areaCenter ) ) {
		// too close to where we already are
		return false;
	}

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f ), PVSAreas, idEntity::MAX_PVS_AREAS );
	if ( !gameLocal.pvs.InCurrentPVS( targetPVS, PVSAreas, numPVSAreas ) ) {
		return false;
	}

	// calculate the world transform of the launch position
	dir = targetPos - areaCenter;
	gravityAxis.ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	axis = local_dir.ToMat3();
	fromPos = areaCenter + fireOffset * axis;

	return self->GetAimDir( fromPos, target, self, dir );
}


/*
============
idAASFindObservationPosition::idAASFindObservationPosition
============
*/
idAASFindObservationPosition::idAASFindObservationPosition( const idAI *self, const idMat3 &gravityAxis, const idVec3 &targetPos, const idVec3 &eyeOffset, float maxDistanceFromWhichToObserve )
{
	int	numPVSAreas;

	this->targetPos		= targetPos;
	this->eyeOffset		= eyeOffset;
	this->self			= self;
	this->gravityAxis	= gravityAxis;
	this->maxObservationDistance = maxDistanceFromWhichToObserve;
	this->b_haveBestGoal = false;


	// setup PVS
	idBounds bounds( targetPos - idVec3( 16, 16, 0 ), targetPos + idVec3( 16, 16, 64 ) );
	numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, idEntity::MAX_PVS_AREAS );
	targetPVS	= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
idAASFindObservationPosition::~idAASFindObservationPosition
============
*/
idAASFindObservationPosition::~idAASFindObservationPosition() {
	gameLocal.pvs.FreeCurrentPVS( targetPVS );
}

/*
============
idAASFindObservationPosition::TestArea
============
*/
bool idAASFindObservationPosition::TestArea( const idAAS *aas, int areaNum )
{
	idVec3	dir;
	idVec3	local_dir;
	idVec3	fromPos;
	idMat3	axis;
	idVec3	areaCenter;

	areaCenter = aas->AreaCenter( areaNum );
	areaCenter[ 2 ] += 1.0f;

	/*
	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f ), PVSAreas, idEntity::MAX_PVS_AREAS );
	if ( !gameLocal.pvs.InCurrentPVS( targetPVS, PVSAreas, numPVSAreas ) ) {
		return false;
	}
	*/

	// calculate the world transform of the view position
	dir = targetPos - areaCenter;
	gravityAxis.ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	axis = local_dir.ToMat3();

	fromPos = areaCenter + eyeOffset * axis;

	// Run trace
	trace_t results;
	gameLocal.clip.TracePoint( results, fromPos, targetPos, MASK_SOLID, self );
	if (  results.fraction >= 1.0f )
	{
		// What is the observation distance?
		float distance = (fromPos - targetPos).Length();

		// Remember best result, even if outside max distance allowed
		if ((!b_haveBestGoal) || (distance < bestGoalDistance))
		{
			b_haveBestGoal = true;
			bestGoalDistance = distance;
			bestGoal.areaNum = areaNum;
			bestGoal.origin = areaCenter;
		}
		if (distance > maxObservationDistance)
		{
			// Can't use this point, its too far
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}

}

//------------------------------------------------------------------------------

bool idAASFindObservationPosition::getBestGoalResult
(
	float& out_bestGoalDistance,
	aasGoal_t& out_bestGoal
)
{
	if (b_haveBestGoal)
	{
		out_bestGoalDistance = bestGoalDistance;
		out_bestGoal = bestGoal;
		return true;
	}
	else
	{
		return false;
	}
}


/*
=====================
idAI::idAI
=====================
*/
idAI::idAI()
{
	aiNode.SetOwner(this);

	aas					= NULL;
	travelFlags			= TFL_WALK|TFL_AIR|TFL_DOOR;
	lastAreaReevaluationTime = -1;
	maxAreaReevaluationInterval = 2000; // msec
	doorRetryTime		= 120000; // msec

	kickForce			= 60.0f; // grayman #2568 - default Doom 3 value
	ignore_obstacles	= false;
	blockedRadius		= 0.0f;
	blockedMoveTime		= 750;
	blockedAttackTime	= 750;
	turnRate			= 360.0f;
	turnVel				= 0.0f;
	anim_turn_yaw		= 0.0f;
	anim_turn_amount	= 0.0f;
	anim_turn_angles	= 0.0f;
	reachedpos_bbox_expansion = 0.0f;
	aas_reachability_z_tolerance = 75;
	fly_offset			= 0;
	fly_seek_scale		= 1.0f;
	fly_roll_scale		= 0.0f;
	fly_roll_max		= 0.0f;
	fly_roll			= 0.0f;
	fly_pitch_scale		= 0.0f;
	fly_pitch_max		= 0.0f;
	fly_pitch			= 0.0f;
	allowMove			= false;
	allowHiddenMovement	= false;
	fly_speed			= 0.0f;
	fly_bob_strength	= 0.0f;
	fly_bob_vert		= 0.0f;
	fly_bob_horz		= 0.0f;
	lastHitCheckResult	= false;
	lastHitCheckTime	= 0;
	lastAttackTime		= 0;
	fire_range			= 0.0f;
	projectile_height_to_distance_ratio = 1.0f;
	activeProjectile.projEnt = NULL;
	curProjectileIndex	= -1;
	talk_state			= TALK_NEVER;
	talkTarget			= NULL;

	particles.Clear();
	restartParticles	= true;
	useBoneAxis			= false;

	wakeOnFlashlight	= false;
	lastUpdateEnemyPositionTime = -1;

	// grayman #2887 - for keeping track of the total time this AI saw the player
	lastTimePlayerSeen = -1;
	lastTimePlayerLost = -1;

	fleeingEvent = false; // grayman #3317
	fleeingFrom = vec3_zero;
	emitFleeBarks = false;
	lastSearchedSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);

	memset( &worldMuzzleFlash, 0, sizeof ( worldMuzzleFlash ) );
	worldMuzzleFlashHandle = -1;

	enemy				= NULL;
	lastVisibleEnemyPos.Zero();
	lastVisibleEnemyEyeOffset.Zero();
	lastVisibleReachableEnemyPos.Zero();
	lastReachableEnemyPos.Zero();
	enemyReachable		= false;
	shrivel_rate		= 0.0f;
	shrivel_start		= 0;
	fl.neverDormant		= false;		// AI's can go dormant
	current_yaw			= 0.0f;
	ideal_yaw			= 0.0f;

	num_cinematics		= 0;
	current_cinematic	= 0;

	allowEyeFocus		= true;
	allowPain			= true;
	allowJointMod		= true;
	focusEntity			= NULL;
	focusTime			= 0;
	alignHeadTime		= 0;
	forceAlignHeadTime	= 0;

	currentFocusPos.Zero();
	eyeAng.Zero();
	lookAng.Zero();
	destLookAng.Zero();
	lookMin.Zero();
	lookMax.Zero();

	eyeMin.Zero();
	eyeMax.Zero();
	muzzleFlashEnd		= 0;
	flashTime			= 0;
	flashJointWorld		= INVALID_JOINT;

	focusJoint			= INVALID_JOINT;
	orientationJoint	= INVALID_JOINT;
	flyTiltJoint		= INVALID_JOINT;

	eyeVerticalOffset	= 0.0f;
	eyeHorizontalOffset = 0.0f;
	eyeFocusRate		= 0.0f;
	headFocusRate		= 0.0f;
	focusAlignTime		= 0;
	m_tactileEntity		= NULL;  // grayman #2345
	m_canResolveBlock	= true;	 // grayman #2345
	m_leftQueue			= false; // grayman #2345
	m_performRelight	= false; // grayman #2603
	m_ReactingToHit		= false; // grayman #2816
	m_bloodMarker		= NULL;  // grayman #3075
	m_lastKilled		= NULL;	 // grayman #2816
	m_justKilledSomeone = false; // grayman #2816
	m_deckedByPlayer	= false; // grayman #3314
	m_allowAudioAlerts  = true;  // grayman #3424
	m_searchID			= -1;	 // grayman #3857

	m_pathWaitTaskEndtime = 0; // grayman #4046

	m_SoundDir.Zero();
	m_LastSight.Zero();
	m_AlertLevelThisFrame = 0.0f;
	m_lookAtAlertSpot = false; // grayman #3520
	m_lookAtPos = idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY); // grayman #3520
	m_prevAlertIndex = 0;
	m_maxAlertLevel = 0;
	m_maxAlertIndex = 0;
	m_recentHighestAlertLevel = 0;
	m_AlertedByActor = NULL;
	memset(alertTypeWeight, 0, sizeof(alertTypeWeight));

	m_TactAlertEnt = NULL;
	m_AlertGraceActor = NULL;
	m_AlertGraceStart = 0;
	m_AlertGraceTime = 0;
	m_AlertGraceThresh = 0;
	m_AlertGraceCount = 0;
	m_AlertGraceCountLimit = 0;
	m_AudThreshold = 0.0f;
	m_oldVisualAcuity = 0.0f;
	m_sleepFloorZ = 0;  // grayman #2416
	m_getupEndTime = 0; // grayman #2416

	m_barkEndTime = 0; // grayman #3857

	m_bCanDrown = true;
	m_AirCheckTimer = 0;
	m_AirTics = 0;
	m_AirTicksMax = 0;
	m_HeadBodyID = 0;
	m_HeadJointID = INVALID_JOINT;
	m_OrigHeadCM = NULL;
	m_bHeadCMSwapped = false;

	m_bCanBeKnockedOut = true;
	m_HeadCenterOffset = vec3_zero;
	m_FOVRot = mat3_identity;
	m_bKoAlertImmune = false;
	m_KoDotVert = 0;
	m_KoDotHoriz = 0;
	m_KoAlertDotHoriz = 0;
	m_KoRot = mat3_identity;

	m_bCanBeGassed = true;		// grayman #2468
	m_koState = KO_NOT;			// grayman #2604
	m_earlyThinkCounter = 5 + gameLocal.random.RandomInt(5);	// grayman #2654
	m_bCanExtricate = true;		// grayman #2603
	m_ignorePlayer = false;		// grayman #3063

	m_bCanOperateDoors = false;
	m_bCanCloseDoors = true;

	m_lipSyncActive		= false;
	m_lipSyncAnim		= -1;
	m_lipSyncEndTimer	= -1;

	m_bPushOffPlayer	= false;

	m_bCanBeFlatFooted	= false;
	m_bFlatFooted		= false;
	m_FlatFootedTimer	= 0;
	m_FlatFootedTime	= 0;

	m_FlatFootParryNum	= 0;
	m_FlatFootParryMax	= 0;
	m_FlatFootParryTimer = 0;
	m_FlatFootParryTime	= 0;
	m_MeleeCounterAttChance = 0.0f;
	m_bMeleePredictProximity = false;

	m_maxInterleaveThinkFrames = 0;
	m_minInterleaveThinkDist = 1000;
	m_maxInterleaveThinkDist = 3000;
	m_lastThinkTime = 0;
	m_nextThinkTime = 0;

	INIT_TIMER_HANDLE(aiThinkTimer);
	INIT_TIMER_HANDLE(aiMindTimer);
	INIT_TIMER_HANDLE(aiAnimationTimer);
	INIT_TIMER_HANDLE(aiPushWithAFTimer);
	INIT_TIMER_HANDLE(aiUpdateEnemyPositionTimer);
	INIT_TIMER_HANDLE(aiScriptTimer);
	INIT_TIMER_HANDLE(aiAnimMoveTimer);
	INIT_TIMER_HANDLE(aiObstacleAvoidanceTimer);
	INIT_TIMER_HANDLE(aiPhysicsTimer);
	INIT_TIMER_HANDLE(aiGetMovePosTimer);
	INIT_TIMER_HANDLE(aiPathToGoalTimer);
	INIT_TIMER_HANDLE(aiGetFloorPosTimer);
	INIT_TIMER_HANDLE(aiPointReachableAreaNumTimer);
	INIT_TIMER_HANDLE(aiCanSeeTimer);

}

/*
=====================
idAI::~idAI
=====================
*/
idAI::~idAI()
{
	if (m_searchID > 0)
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Destroying AI while search still active. Actively leaving search: SearchID=%d, AI=0x%p\r", m_searchID, this);
		CSearchManager::Instance()->LeaveSearch(m_searchID, this);
	}
 		
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Destroying AI: 0x%p\r", this);

	DeconstructScriptObject();
	scriptObject.Free();
	if ( worldMuzzleFlashHandle != -1 )
	{
		gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
		worldMuzzleFlashHandle = -1;
	}

	aiNode.Remove();

	if( m_OrigHeadCM )
		delete m_OrigHeadCM;
}

/*
=====================
idAI::Save
=====================
*/
void idAI::Save(idSaveGame *savefile) const {
	int i;

	savefile->WriteInt(travelFlags);
	savefile->WriteInt(lastAreaReevaluationTime);
	savefile->WriteInt(maxAreaReevaluationInterval);
	savefile->WriteInt(doorRetryTime);

	move.Save(savefile);
	savedMove.Save(savefile);

	// greebo: save the movestack
	savefile->WriteInt(static_cast<int>(moveStack.size()));
	for ( std::list<idMoveState>::const_iterator m = moveStack.begin(); m != moveStack.end(); ++m )
	{
		m->Save(savefile);
	}

	savefile->WriteFloat(kickForce);
	savefile->WriteBool(ignore_obstacles);
	savefile->WriteFloat(blockedRadius);
	savefile->WriteInt(blockedMoveTime);
	savefile->WriteInt(blockedAttackTime);

	savefile->WriteFloat(ideal_yaw);
	savefile->WriteFloat(current_yaw);
	savefile->WriteFloat(turnRate);
	savefile->WriteFloat(turnVel);
	savefile->WriteFloat(anim_turn_yaw);
	savefile->WriteFloat(anim_turn_amount);
	savefile->WriteFloat(anim_turn_angles);
	savefile->WriteVec3(sitting_turn_pivot);

	savefile->WriteFloat(reachedpos_bbox_expansion);
	savefile->WriteFloat(aas_reachability_z_tolerance);

	savefile->WriteStaticObject(physicsObj);

	savefile->WriteFloat(fly_speed);
	savefile->WriteFloat(fly_bob_strength);
	savefile->WriteFloat(fly_bob_vert);
	savefile->WriteFloat(fly_bob_horz);
	savefile->WriteInt(fly_offset);
	savefile->WriteFloat(fly_seek_scale);
	savefile->WriteFloat(fly_roll_scale);
	savefile->WriteFloat(fly_roll_max);
	savefile->WriteFloat(fly_roll);
	savefile->WriteFloat(fly_pitch_scale);
	savefile->WriteFloat(fly_pitch_max);
	savefile->WriteFloat(fly_pitch);

	savefile->WriteBool(allowMove);
	savefile->WriteBool(allowHiddenMovement);
	savefile->WriteBool(disableGravity);

	savefile->WriteBool(lastHitCheckResult);
	savefile->WriteInt(lastHitCheckTime);
	savefile->WriteInt(lastAttackTime);
	savefile->WriteFloat(fire_range);
	savefile->WriteFloat(projectile_height_to_distance_ratio);

	savefile->WriteInt(missileLaunchOffset.Num());
	for ( i = 0; i < missileLaunchOffset.Num(); i++ ) {
		savefile->WriteVec3(missileLaunchOffset[i]);
	}

	savefile->WriteInt(projectileInfo.Num());

	for ( i = 0; i < projectileInfo.Num(); ++i )
	{
		const ProjectileInfo& info = projectileInfo[i];

		// Save projectile def names instead of dict* pointers, will be resolved at restore time
		// also leave clipmodel pointer alone, will be initialised to NULL on restore and
		// reloaded on demand
		savefile->WriteString(info.defName);
		savefile->WriteFloat(info.radius);
		savefile->WriteFloat(info.speed);
		savefile->WriteVec3(info.velocity);
		savefile->WriteVec3(info.gravity);
	}

	savefile->WriteInt(curProjectileIndex);

	// Active Projectile
	savefile->WriteString(activeProjectile.info.defName);
	savefile->WriteFloat(activeProjectile.info.radius);
	savefile->WriteFloat(activeProjectile.info.speed);
	savefile->WriteVec3(activeProjectile.info.velocity);
	savefile->WriteVec3(activeProjectile.info.gravity);
	activeProjectile.projEnt.Save(savefile);

	savefile->WriteString(attack);

	// grayman #2603 - delayed stim list

	savefile->WriteInt(delayedStims.Num());
	for ( i = 0; i < delayedStims.Num(); i++ )
	{
		DelayedStim ds = delayedStims[i];

		savefile->WriteInt(ds.nextTimeToConsider);
		ds.stim.Save(savefile);
	}

	// grayman #4002 - entity alert list

	savefile->WriteInt(alertQueue.Num());
	for ( i = 0; i < alertQueue.Num(); i++ )
	{
		EntityAlert ea = alertQueue[i];

		savefile->WriteInt(ea.timeAlerted);
		savefile->WriteInt(ea.alertIndex);
		ea.entityResponsible.Save(savefile);
		savefile->WriteBool(ea.ignore);
	}

	savefile->WriteInt(talk_state);
	talkTarget.Save(savefile);

	savefile->WriteInt(num_cinematics);
	savefile->WriteInt(current_cinematic);

	savefile->WriteBool(allowJointMod);
	focusEntity.Save(savefile);
	savefile->WriteVec3(currentFocusPos);
	savefile->WriteInt(focusTime);
	savefile->WriteInt(alignHeadTime);
	savefile->WriteInt(forceAlignHeadTime);
	savefile->WriteAngles(eyeAng);
	savefile->WriteAngles(lookAng);
	savefile->WriteAngles(destLookAng);
	savefile->WriteAngles(lookMin);
	savefile->WriteAngles(lookMax);

	savefile->WriteInt(lookJoints.Num());
	for ( i = 0; i < lookJoints.Num(); i++ ) {
		savefile->WriteJoint(lookJoints[i]);
		savefile->WriteAngles(lookJointAngles[i]);
	}
	savefile->WriteInt(lookJointsCombat.Num());
	for ( i = 0; i < lookJointsCombat.Num(); i++ ) {
		savefile->WriteJoint(lookJointsCombat[i]);
		savefile->WriteAngles(lookJointAnglesCombat[i]);
	}

	savefile->WriteFloat(shrivel_rate);
	savefile->WriteInt(shrivel_start);

	savefile->WriteInt(particles.Num());
	for ( i = 0; i < particles.Num(); i++ ) {
		savefile->WriteParticle(particles[i].particle);
		savefile->WriteInt(particles[i].time);
		savefile->WriteJoint(particles[i].joint);
	}
	savefile->WriteBool(restartParticles);
	savefile->WriteBool(useBoneAxis);

	enemy.Save(savefile);
	savefile->WriteVec3(lastVisibleEnemyPos);
	savefile->WriteVec3(lastVisibleEnemyEyeOffset);
	savefile->WriteVec3(lastVisibleReachableEnemyPos);
	savefile->WriteVec3(lastReachableEnemyPos);
	savefile->WriteBool(enemyReachable);
	savefile->WriteBool(wakeOnFlashlight);
	savefile->WriteInt(lastUpdateEnemyPositionTime);

	savefile->WriteInt(lastTimePlayerSeen);		// grayman #2887
	savefile->WriteInt(lastTimePlayerLost);		// grayman #2887

	savefile->WriteBool(fleeingEvent);			// grayman #3317
	savefile->WriteVec3(fleeingFrom);			// grayman #3848
	savefile->WriteBool(emitFleeBarks);			// grayman #3474
	fleeingFromPerson.Save(savefile);			// grayman #3847

	savefile->WriteAngles(eyeMin);
	savefile->WriteAngles(eyeMax);

	savefile->WriteFloat(eyeVerticalOffset);
	savefile->WriteFloat(eyeHorizontalOffset);
	savefile->WriteFloat(eyeFocusRate);
	savefile->WriteFloat(headFocusRate);
	savefile->WriteInt(focusAlignTime);
	savefile->WriteObject(m_tactileEntity);		// grayman #2345
	m_bloodMarker.Save(savefile);				// grayman #3075
	m_lastKilled.Save(savefile);				// grayman #2816
	savefile->WriteBool(m_justKilledSomeone);	// grayman #2816
	savefile->WriteBool(m_canResolveBlock);		// grayman #2345
	savefile->WriteBool(m_leftQueue);			// grayman #2345
	savefile->WriteBool(m_performRelight);		// grayman #2603
	savefile->WriteBool(m_ReactingToHit);		// grayman #2816
	savefile->WriteBool(m_deckedByPlayer);		// grayman #3314
	savefile->WriteBool(m_allowAudioAlerts);	// grayman #3424
	savefile->WriteInt(m_searchID);				// grayman #3857
	savefile->WriteFloat(m_pathWaitTaskEndtime); // grayman #4046
	savefile->WriteJoint(flashJointWorld);
	savefile->WriteInt(muzzleFlashEnd);

	savefile->WriteJoint(focusJoint);
	savefile->WriteJoint(orientationJoint);
	savefile->WriteJoint(flyTiltJoint);

	// TDM Alerts:
	savefile->WriteInt(m_Acuities.Num());
	for ( i = 0; i < m_Acuities.Num(); i++ )
	{
		savefile->WriteFloat(m_Acuities[i]);
	}
	savefile->WriteFloat(m_oldVisualAcuity);
	savefile->WriteFloat(m_sleepFloorZ); // grayman #2416
	savefile->WriteInt(m_getupEndTime);	 // grayman #2416
	savefile->WriteFloat(m_AudThreshold);
	savefile->WriteVec3(m_SoundDir);
	savefile->WriteVec3(m_LastSight);
	savefile->WriteFloat(m_AlertLevelThisFrame);
	savefile->WriteBool(m_lookAtAlertSpot); // grayman #3520
	savefile->WriteVec3(m_lookAtPos); // grayman #3520
	savefile->WriteInt(m_prevAlertIndex);
	savefile->WriteFloat(m_maxAlertLevel);
	savefile->WriteInt(m_maxAlertIndex);
	savefile->WriteFloat(m_recentHighestAlertLevel);
	savefile->WriteBool(m_bIgnoreAlerts);
	savefile->WriteBool(m_drunk);

	m_AlertedByActor.Save(savefile);

	for ( i = 0; i < ai::EAlertTypeCount; i++ )
	{
		savefile->WriteInt(alertTypeWeight[i]);
	}

	m_TactAlertEnt.Save(savefile);
	m_AlertGraceActor.Save(savefile);
	savefile->WriteInt(m_AlertGraceStart);
	savefile->WriteInt(m_AlertGraceTime);
	savefile->WriteFloat(m_AlertGraceThresh);
	savefile->WriteInt(m_AlertGraceCount);
	savefile->WriteInt(m_AlertGraceCountLimit);

	savefile->WriteInt(m_Messages.Num());
	for ( i = 0; i < m_Messages.Num(); i++ )
	{
		m_Messages[i]->Save(savefile);
	}

	savefile->WriteBool(GetPhysics() == static_cast<const idPhysics *>(&physicsObj));

	/* grayman #3857
	// grayman #3424
	int num = m_randomHidingSpotIndexes.size();
	savefile->WriteInt(num);
	for ( i = 0 ; i < num ; i++ )
	{
		savefile->WriteInt(m_randomHidingSpotIndexes[i]);
	}
	*/

	savefile->WriteInt(m_AirCheckTimer);
	savefile->WriteBool(m_bCanDrown);
	savefile->WriteInt(m_HeadBodyID);
	savefile->WriteJoint(m_HeadJointID);

	savefile->WriteBool(m_bHeadCMSwapped);
	//	if( m_bHeadCMSwapped && m_OrigHeadCM )
	//		m_OrigHeadCM->Save( savefile );

	savefile->WriteInt(m_AirTics);
	savefile->WriteInt(m_AirTicksMax);
	savefile->WriteInt(m_AirCheckInterval);

	savefile->WriteBool(m_bCanBeKnockedOut);
	savefile->WriteVec3(m_HeadCenterOffset);
	savefile->WriteMat3(m_FOVRot);
	savefile->WriteString(m_KoZone);
	savefile->WriteInt(m_KoAlertState);
	savefile->WriteInt(m_KoAlertImmuneState);
	savefile->WriteBool(m_bKoAlertImmune);
	savefile->WriteFloat(m_KoDotVert);
	savefile->WriteFloat(m_KoDotHoriz);
	savefile->WriteFloat(m_KoAlertDotVert);
	savefile->WriteFloat(m_KoAlertDotHoriz);
	savefile->WriteMat3(m_KoRot);

	savefile->WriteBool(m_bCanBeGassed);		// grayman #2468
	savefile->WriteInt(m_koState);				// grayman #2604
	savefile->WriteInt(m_earlyThinkCounter);	// grayman #2654
	savefile->WriteBool(m_bCanExtricate);		// grayman #2603
	savefile->WriteBool(m_ignorePlayer);		// grayman #3063
	//savefile->WriteBounds(m_searchLimits);		// grayman #2422 // grayman #3857

	savefile->WriteFloat(thresh_1);
	savefile->WriteFloat(thresh_2);
	savefile->WriteFloat(thresh_3);
	savefile->WriteFloat(thresh_4);
	savefile->WriteFloat(thresh_5);

	savefile->WriteFloat(m_gracetime_1);
	savefile->WriteFloat(m_gracetime_2);
	savefile->WriteFloat(m_gracetime_3);
	savefile->WriteFloat(m_gracetime_4);
	savefile->WriteFloat(m_gracefrac_1);
	savefile->WriteFloat(m_gracefrac_2);
	savefile->WriteFloat(m_gracefrac_3);
	savefile->WriteFloat(m_gracefrac_4);

	// grayman #3492 - these ints were being saved as floats and read back as ints
	savefile->WriteInt(m_gracecount_1);
	savefile->WriteInt(m_gracecount_2);
	savefile->WriteInt(m_gracecount_3);
	savefile->WriteInt(m_gracecount_4);

	savefile->WriteFloat(atime1);
	savefile->WriteFloat(atime2);
	savefile->WriteFloat(atime3);
	savefile->WriteFloat(atime4);
	savefile->WriteFloat(atime_fleedone);
	savefile->WriteBool(m_canSearch);	// grayman #3069

	savefile->WriteFloat(atime1_fuzzyness);
	savefile->WriteFloat(atime2_fuzzyness);
	savefile->WriteFloat(atime3_fuzzyness);
	savefile->WriteFloat(atime4_fuzzyness);
	savefile->WriteFloat(atime_fleedone_fuzzyness);

	savefile->WriteInt(m_timeBetweenHeadTurnChecks);
	savefile->WriteFloat(m_headTurnChanceIdle);
	savefile->WriteFloat(m_headTurnFactorAlerted);
	savefile->WriteFloat(m_headTurnMaxYaw);
	savefile->WriteFloat(m_headTurnMaxPitch);
	savefile->WriteInt(m_headTurnMinDuration);
	savefile->WriteInt(m_headTurnMaxDuration);

	savefile->WriteInt(static_cast<int>(backboneStates.size()));
	for ( BackboneStateMap::const_iterator i = backboneStates.begin(); i != backboneStates.end(); ++i )
	{
		savefile->WriteInt(i->first);
		savefile->WriteString(i->second);
	}

	savefile->WriteInt(m_maxInterleaveThinkFrames);
	savefile->WriteFloat(m_minInterleaveThinkDist);
	savefile->WriteFloat(m_maxInterleaveThinkDist);

	savefile->WriteInt(m_lastThinkTime);
	savefile->WriteInt(m_nextThinkTime);

	savefile->WriteString(m_barkName); // grayman #3857
	savefile->WriteInt(m_barkEndTime); // grayman #3857

	savefile->WriteBool(m_bPushOffPlayer);

	savefile->WriteBool(m_bCanBeFlatFooted);
	savefile->WriteBool(m_bFlatFooted);
	savefile->WriteInt(m_FlatFootedTimer);
	savefile->WriteInt(m_FlatFootedTime);
	savefile->WriteInt(m_FlatFootParryNum);
	savefile->WriteInt(m_FlatFootParryMax);
	savefile->WriteInt(m_FlatFootParryTimer);
	savefile->WriteInt(m_FlatFootParryTime);
	savefile->WriteFloat(m_MeleeCounterAttChance);
	savefile->WriteBool(m_bMeleePredictProximity);

	savefile->WriteBool(m_bCanOperateDoors);
	savefile->WriteBool(m_bCanCloseDoors);
	savefile->WriteBool(m_HandlingDoor);
	savefile->WriteBool(m_HandlingElevator);
	savefile->WriteBool(m_DoorQueued);		// grayman #3647
	savefile->WriteBool(m_ElevatorQueued);	// grayman #3647
	savefile->WriteBool(m_CanSetupDoor);	// grayman #3029
	savefile->WriteBool(m_RelightingLight);	// grayman #2603
	savefile->WriteBool(m_ExaminingRope);	// grayman #2872
	savefile->WriteBool(m_DroppingTorch);	// grayman #2603
	savefile->WriteBool(m_RestoreMove);		// grayman #2706
	savefile->WriteBool(m_LatchedSearch);	// grayman #2603
	savefile->WriteBool(m_ReactingToPickedPocket); // grayman #3559
	savefile->WriteBool(m_InConversation);	// grayman #3559
	savefile->WriteInt(m_nextWarningTime);	// grayman #5164

	// grayman #2603
	savefile->WriteInt( m_dousedLightsSeen.Num() );
	for ( int i = 0 ; i < m_dousedLightsSeen.Num() ; i++ )
	{
		m_dousedLightsSeen[i].Save(savefile);
	}

	// grayman #3681
	savefile->WriteInt( m_noisemakersHeard.Num() );
	for ( int i = 0 ; i < m_noisemakersHeard.Num() ; i++ )
	{
		m_noisemakersHeard[i].Save(savefile);
	}

    int size = static_cast<int>(unlockableDoors.size());
	savefile->WriteInt(size);
	for (FrobMoverList::const_iterator i = unlockableDoors.begin(); i != unlockableDoors.end(); ++i)
	{
		savefile->WriteObject(*i);
	}

    savefile->WriteInt(static_cast<int>(tactileIgnoreEntities.size()));

	for (TactileIgnoreList::const_iterator i = tactileIgnoreEntities.begin(); i != tactileIgnoreEntities.end(); ++i)
	{
		savefile->WriteObject(*i);
	}
	
	savefile->WriteVec3( lastSearchedSpot ); // grayman #4220

	mind->Save(savefile);

	senseSubsystem->Save(savefile);
	movementSubsystem->Save(savefile);
	commSubsystem->Save(savefile);
	actionSubsystem->Save(savefile);
	searchSubsystem->Save(savefile); // grayman #3857

	SAVE_TIMER_HANDLE(aiThinkTimer, savefile);
	SAVE_TIMER_HANDLE(aiMindTimer, savefile);
	SAVE_TIMER_HANDLE(aiAnimationTimer, savefile);
	SAVE_TIMER_HANDLE(aiPushWithAFTimer, savefile);
	SAVE_TIMER_HANDLE(aiUpdateEnemyPositionTimer, savefile);
	SAVE_TIMER_HANDLE(aiScriptTimer, savefile);
	SAVE_TIMER_HANDLE(aiAnimMoveTimer, savefile);
	SAVE_TIMER_HANDLE(aiObstacleAvoidanceTimer, savefile);
	SAVE_TIMER_HANDLE(aiPhysicsTimer, savefile);
	SAVE_TIMER_HANDLE(aiGetMovePosTimer, savefile);
	SAVE_TIMER_HANDLE(aiPathToGoalTimer, savefile);
	SAVE_TIMER_HANDLE(aiGetFloorPosTimer, savefile);
	SAVE_TIMER_HANDLE(aiPointReachableAreaNumTimer, savefile);
	SAVE_TIMER_HANDLE(aiCanSeeTimer, savefile);

}

/*
=====================
idAI::Restore
=====================
*/
void idAI::Restore( idRestoreGame *savefile ) {
	bool		restorePhysics;
	int			i;
	int			num;

	savefile->ReadInt( travelFlags );
	savefile->ReadInt(lastAreaReevaluationTime);
	savefile->ReadInt(maxAreaReevaluationInterval);
	savefile->ReadInt(doorRetryTime);

	move.Restore( savefile );
	savedMove.Restore( savefile );

	// greebo: restore the movestack
	moveStack.clear();
	savefile->ReadInt(num);
	for (i = 0; i < num; i++)
	{
		moveStack.push_back(idMoveState());
		moveStack.back().Restore(savefile);
	}

	savefile->ReadFloat( kickForce );
	savefile->ReadBool( ignore_obstacles );
	savefile->ReadFloat( blockedRadius );
	savefile->ReadInt( blockedMoveTime );
	savefile->ReadInt( blockedAttackTime );

	savefile->ReadFloat( ideal_yaw );
	savefile->ReadFloat( current_yaw );
	savefile->ReadFloat( turnRate );
	savefile->ReadFloat( turnVel );
	savefile->ReadFloat( anim_turn_yaw );
	savefile->ReadFloat( anim_turn_amount );
	savefile->ReadFloat( anim_turn_angles );
	savefile->ReadVec3(sitting_turn_pivot);

	savefile->ReadFloat(reachedpos_bbox_expansion);
	savefile->ReadFloat(aas_reachability_z_tolerance);

	savefile->ReadStaticObject( physicsObj );

	savefile->ReadFloat( fly_speed );
	savefile->ReadFloat( fly_bob_strength );
	savefile->ReadFloat( fly_bob_vert );
	savefile->ReadFloat( fly_bob_horz );
	savefile->ReadInt( fly_offset );
	savefile->ReadFloat( fly_seek_scale );
	savefile->ReadFloat( fly_roll_scale );
	savefile->ReadFloat( fly_roll_max );
	savefile->ReadFloat( fly_roll );
	savefile->ReadFloat( fly_pitch_scale );
	savefile->ReadFloat( fly_pitch_max );
	savefile->ReadFloat( fly_pitch );

	savefile->ReadBool( allowMove );
	savefile->ReadBool( allowHiddenMovement );
	savefile->ReadBool( disableGravity );

	savefile->ReadBool( lastHitCheckResult );
	savefile->ReadInt( lastHitCheckTime );
	savefile->ReadInt( lastAttackTime );
	savefile->ReadFloat( fire_range );
	savefile->ReadFloat( projectile_height_to_distance_ratio );

	savefile->ReadInt( num );
	missileLaunchOffset.SetGranularity( 1 );
	missileLaunchOffset.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadVec3( missileLaunchOffset[ i ] );
	}

	savefile->ReadInt(num);

	projectileInfo.SetNum(num);

	for (i = 0; i < projectileInfo.Num(); ++i)
	{
		ProjectileInfo& info = projectileInfo[i];

		savefile->ReadString(info.defName);
		// Resolve projectile def pointers from names
		info.def = (info.defName.Length() > 0) ? gameLocal.FindEntityDefDict(info.defName) : NULL;
		
		// leave clipmodel pointer alone, is already initialised to NULL
		savefile->ReadFloat(info.radius);
		savefile->ReadFloat(info.speed);
		savefile->ReadVec3(info.velocity);
		savefile->ReadVec3(info.gravity);
	}

	savefile->ReadInt(curProjectileIndex);

	// Active Projectile
	savefile->ReadString(activeProjectile.info.defName);

	// Resolve projectile def pointers from names
	if (activeProjectile.info.defName.Length() > 0)
	{
		activeProjectile.info.def = gameLocal.FindEntityDefDict(activeProjectile.info.defName);
	}
	else
	{
		activeProjectile.info.def = NULL;
	}

	savefile->ReadFloat(activeProjectile.info.radius);
	savefile->ReadFloat(activeProjectile.info.speed);
	savefile->ReadVec3(activeProjectile.info.velocity);
	savefile->ReadVec3(activeProjectile.info.gravity);
	activeProjectile.projEnt.Restore(savefile);

	savefile->ReadString( attack );

	// grayman #2603 - delayed stim list

	savefile->ReadInt(num);
	delayedStims.SetNum(num);
	for (i = 0 ; i < num ; i++)
	{
		savefile->ReadInt(delayedStims[i].nextTimeToConsider);
		delayedStims[i].stim.Restore(savefile);
	}

	// grayman #4002 - entity alert list

	savefile->ReadInt(num);
	alertQueue.SetNum(num);
	for (i = 0 ; i < num ; i++)
	{
		savefile->ReadInt(alertQueue[i].timeAlerted);
		savefile->ReadInt(alertQueue[i].alertIndex);
		alertQueue[i].entityResponsible.Restore(savefile);
		savefile->ReadBool(alertQueue[i].ignore);
	}

	savefile->ReadInt( i );
	talk_state = static_cast<talkState_t>( i );
	talkTarget.Restore( savefile );

	savefile->ReadInt( num_cinematics );
	savefile->ReadInt( current_cinematic );

	savefile->ReadBool( allowJointMod );
	focusEntity.Restore( savefile );
	savefile->ReadVec3( currentFocusPos );
	savefile->ReadInt( focusTime );
	savefile->ReadInt( alignHeadTime );
	savefile->ReadInt( forceAlignHeadTime );
	savefile->ReadAngles( eyeAng );
	savefile->ReadAngles( lookAng );
	savefile->ReadAngles( destLookAng );
	savefile->ReadAngles( lookMin );
	savefile->ReadAngles( lookMax );

	savefile->ReadInt( num );
	lookJoints.SetGranularity( 1 );
	lookJoints.SetNum( num );
	lookJointAngles.SetGranularity( 1 );
	lookJointAngles.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadJoint( lookJoints[ i ] );
		savefile->ReadAngles( lookJointAngles[ i ] );
	}

	savefile->ReadInt( num );
	lookJointsCombat.SetGranularity( 1 );
	lookJointsCombat.SetNum( num );
	lookJointAnglesCombat.SetGranularity( 1 );
	lookJointAnglesCombat.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadJoint( lookJointsCombat[ i ] );
		savefile->ReadAngles( lookJointAnglesCombat[ i ] );
	}

	savefile->ReadFloat( shrivel_rate );
	savefile->ReadInt( shrivel_start );

	savefile->ReadInt( num );
	particles.SetNum( num );
	for  ( i = 0; i < particles.Num(); i++ ) {
		savefile->ReadParticle( particles[i].particle );
		savefile->ReadInt( particles[i].time );
		savefile->ReadJoint( particles[i].joint );
	}
	savefile->ReadBool( restartParticles );
	savefile->ReadBool( useBoneAxis );

	enemy.Restore( savefile );
	savefile->ReadVec3( lastVisibleEnemyPos );
	savefile->ReadVec3( lastVisibleEnemyEyeOffset );
	savefile->ReadVec3( lastVisibleReachableEnemyPos );
	savefile->ReadVec3( lastReachableEnemyPos );
	savefile->ReadBool( enemyReachable );
	savefile->ReadBool( wakeOnFlashlight );
	savefile->ReadInt(lastUpdateEnemyPositionTime);
	savefile->ReadInt(lastTimePlayerSeen);	// grayman #2887
	savefile->ReadInt(lastTimePlayerLost);	// grayman #2887

	savefile->ReadBool(fleeingEvent);		// grayman #3317
	savefile->ReadVec3(fleeingFrom);		// grayman #3848
	savefile->ReadBool(emitFleeBarks);		// grayman #3474
	fleeingFromPerson.Restore(savefile);	// grayman #3847

	savefile->ReadAngles( eyeMin );
	savefile->ReadAngles( eyeMax );

	savefile->ReadFloat( eyeVerticalOffset );
	savefile->ReadFloat( eyeHorizontalOffset );
	savefile->ReadFloat( eyeFocusRate );
	savefile->ReadFloat( headFocusRate );
	savefile->ReadInt( focusAlignTime );
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_tactileEntity)); // grayman #2345
	m_bloodMarker.Restore(savefile);	// grayman #3075
	m_lastKilled.Restore(savefile); // grayman #2816
	savefile->ReadBool(m_justKilledSomeone); // grayman #2816
	savefile->ReadBool(m_canResolveBlock);	 // grayman #2345
	savefile->ReadBool(m_leftQueue);		 // grayman #2345
	savefile->ReadBool(m_performRelight);	 // grayman #2603
	savefile->ReadBool(m_ReactingToHit);	 // grayman #2816
	savefile->ReadBool(m_deckedByPlayer);	 // grayman #3314
	savefile->ReadBool(m_allowAudioAlerts);	 // grayman #3424
	savefile->ReadInt(m_searchID);			 // grayman #3857
	savefile->ReadFloat(m_pathWaitTaskEndtime); // grayman #4046
	savefile->ReadJoint( flashJointWorld );
	savefile->ReadInt( muzzleFlashEnd );

	savefile->ReadJoint( focusJoint );
	savefile->ReadJoint( orientationJoint );
	savefile->ReadJoint( flyTiltJoint );

	// TDM Alerts:
	savefile->ReadInt( num );
	m_Acuities.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		savefile->ReadFloat( m_Acuities[ i ] );
	}
	savefile->ReadFloat(m_oldVisualAcuity);
	savefile->ReadFloat(m_sleepFloorZ); // grayman #2416
	savefile->ReadInt(m_getupEndTime);	// grayman #2416
	savefile->ReadFloat( m_AudThreshold );
	savefile->ReadVec3( m_SoundDir );
	savefile->ReadVec3( m_LastSight );
	savefile->ReadFloat( m_AlertLevelThisFrame );
	savefile->ReadBool( m_lookAtAlertSpot ); // grayman #3520
	savefile->ReadVec3( m_lookAtPos ); // grayman #3520
	savefile->ReadInt( m_prevAlertIndex );
	savefile->ReadFloat( m_maxAlertLevel );
	savefile->ReadInt( m_maxAlertIndex);
	savefile->ReadFloat(m_recentHighestAlertLevel);
	savefile->ReadBool( m_bIgnoreAlerts );
	savefile->ReadBool( m_drunk );

	m_AlertedByActor.Restore( savefile );
	for (i = 0; i < ai::EAlertTypeCount; i++)
	{
		savefile->ReadInt(alertTypeWeight[i]);
	}
	m_TactAlertEnt.Restore( savefile );
	m_AlertGraceActor.Restore( savefile );
	savefile->ReadInt( m_AlertGraceStart );
	savefile->ReadInt( m_AlertGraceTime );
	savefile->ReadFloat( m_AlertGraceThresh );
	savefile->ReadInt( m_AlertGraceCount );
	savefile->ReadInt( m_AlertGraceCountLimit );

	savefile->ReadInt(num);
	m_Messages.ClearFree();
	for (int i = 0; i < num; i++)
	{
		ai::CommMessagePtr message(new ai::CommMessage);
		message->Restore(savefile);
		m_Messages.Append(message);
	}

	savefile->ReadBool( restorePhysics );

	// Set the AAS if the character has the correct gravity vector
	idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
	gravity *= g_gravity.GetFloat();
	if ( gravity == gameLocal.GetGravity() ) {
		SetAAS();
	}

	/* grayman #3857
	// grayman #3424
	savefile->ReadInt(num);
	m_randomHidingSpotIndexes.clear();
	for ( i = 0 ; i < num ; i++ )
	{
		int n;
		savefile->ReadInt(n);
		m_randomHidingSpotIndexes.push_back(n);
	}
	*/

	savefile->ReadInt(m_AirCheckTimer);
	savefile->ReadBool(m_bCanDrown);
	savefile->ReadInt(m_HeadBodyID);
	savefile->ReadJoint(m_HeadJointID);

	savefile->ReadBool( m_bHeadCMSwapped );
	// Ishtvan: if the head CM was swapped at save, swap it again
	// (I tried to do this more correctly, but the AF itself isn't saving its new clipmodel)
	if( m_bHeadCMSwapped )
	{
		m_OrigHeadCM = NULL;
		m_bHeadCMSwapped = false;
		SwapHeadAFCM( true );
	}

	savefile->ReadInt(m_AirTics);
	savefile->ReadInt(m_AirTicksMax);
	savefile->ReadInt(m_AirCheckInterval);

	savefile->ReadBool(m_bCanBeKnockedOut);
	savefile->ReadVec3(m_HeadCenterOffset);
	savefile->ReadMat3(m_FOVRot);
	savefile->ReadString(m_KoZone);
	savefile->ReadInt(m_KoAlertState);
	savefile->ReadInt(m_KoAlertImmuneState);
	savefile->ReadBool(m_bKoAlertImmune);
	savefile->ReadFloat(m_KoDotVert);
	savefile->ReadFloat(m_KoDotHoriz);
	savefile->ReadFloat(m_KoAlertDotVert);
	savefile->ReadFloat(m_KoAlertDotHoriz);
	savefile->ReadMat3(m_KoRot);

	savefile->ReadBool(m_bCanBeGassed); // grayman #2468
	savefile->ReadInt( i ); // grayman #2604
	m_koState = static_cast<koState_t>( i );
	savefile->ReadInt(m_earlyThinkCounter); // grayman #2654
	savefile->ReadBool(m_bCanExtricate);	// grayman #2603
	savefile->ReadBool(m_ignorePlayer);		// grayman #3063
	//savefile->ReadBounds(m_searchLimits);	// grayman #2422 // grayman #3857

	savefile->ReadFloat(thresh_1);
	savefile->ReadFloat(thresh_2);
	savefile->ReadFloat(thresh_3);
	savefile->ReadFloat(thresh_4);
	savefile->ReadFloat(thresh_5);

	savefile->ReadFloat(m_gracetime_1);
	savefile->ReadFloat(m_gracetime_2);
	savefile->ReadFloat(m_gracetime_3);
	savefile->ReadFloat(m_gracetime_4);
	savefile->ReadFloat(m_gracefrac_1);
	savefile->ReadFloat(m_gracefrac_2);
	savefile->ReadFloat(m_gracefrac_3);
	savefile->ReadFloat(m_gracefrac_4);
	savefile->ReadInt(m_gracecount_1);
	savefile->ReadInt(m_gracecount_2);
	savefile->ReadInt(m_gracecount_3);
	savefile->ReadInt(m_gracecount_4);

	savefile->ReadFloat(atime1);
	savefile->ReadFloat(atime2);
	savefile->ReadFloat(atime3);
	savefile->ReadFloat(atime4);
	savefile->ReadFloat(atime_fleedone);
	savefile->ReadBool(m_canSearch); // grayman #3069

	savefile->ReadFloat(atime1_fuzzyness);
	savefile->ReadFloat(atime2_fuzzyness);
	savefile->ReadFloat(atime3_fuzzyness);
	savefile->ReadFloat(atime4_fuzzyness);
	savefile->ReadFloat(atime_fleedone_fuzzyness);

	savefile->ReadInt(m_timeBetweenHeadTurnChecks);
	savefile->ReadFloat(m_headTurnChanceIdle);
	savefile->ReadFloat(m_headTurnFactorAlerted);
	savefile->ReadFloat(m_headTurnMaxYaw);
	savefile->ReadFloat(m_headTurnMaxPitch);
	savefile->ReadInt(m_headTurnMinDuration);
	savefile->ReadInt(m_headTurnMaxDuration);

	backboneStates.clear();
	savefile->ReadInt(num);

	for (int i = 0; i < num; ++i)
	{
		int state = 0;
		savefile->ReadInt(state);
		
		std::pair<BackboneStateMap::iterator, bool> result = backboneStates.insert(
			BackboneStateMap::value_type(static_cast<ai::EAlertState>(state), idStr())
		);

		savefile->ReadString(result.first->second);
	}
	
	savefile->ReadInt(m_maxInterleaveThinkFrames);
	savefile->ReadFloat(m_minInterleaveThinkDist);
	savefile->ReadFloat(m_maxInterleaveThinkDist);

	savefile->ReadInt(m_lastThinkTime);
	savefile->ReadInt(m_nextThinkTime);

	savefile->ReadString(m_barkName); // grayman #3857
	savefile->ReadInt(m_barkEndTime); // grayman #3857

	savefile->ReadBool(m_bPushOffPlayer);

	savefile->ReadBool(m_bCanBeFlatFooted);
	savefile->ReadBool(m_bFlatFooted);
	savefile->ReadInt(m_FlatFootedTimer);
	savefile->ReadInt(m_FlatFootedTime);
	savefile->ReadInt(m_FlatFootParryNum);
	savefile->ReadInt(m_FlatFootParryMax);
	savefile->ReadInt(m_FlatFootParryTimer);
	savefile->ReadInt(m_FlatFootParryTime);
	savefile->ReadFloat(m_MeleeCounterAttChance);
	savefile->ReadBool(m_bMeleePredictProximity);

	savefile->ReadBool(m_bCanOperateDoors);
	savefile->ReadBool(m_bCanCloseDoors);
	savefile->ReadBool(m_HandlingDoor);
	savefile->ReadBool(m_HandlingElevator);
	savefile->ReadBool(m_DoorQueued);		// grayman #3647
	savefile->ReadBool(m_ElevatorQueued);	// grayman #3647
	savefile->ReadBool(m_CanSetupDoor);		// grayman #3029
	savefile->ReadBool(m_RelightingLight);	// grayman #2603
	savefile->ReadBool(m_ExaminingRope);	// grayman #2872
	savefile->ReadBool(m_DroppingTorch);	// grayman #2603
	savefile->ReadBool(m_RestoreMove);		// grayman #2706
	savefile->ReadBool(m_LatchedSearch);	// grayman #2603
	savefile->ReadBool(m_ReactingToPickedPocket); // grayman #3559
	savefile->ReadBool(m_InConversation);	// grayman #3559
	savefile->ReadInt(m_nextWarningTime);	// grayman #5164

	// grayman #2603
	m_dousedLightsSeen.Clear();
	savefile->ReadInt( num );
	m_dousedLightsSeen.SetNum( num );
	for ( int i = 0 ; i < num ; i++ )
	{
		m_dousedLightsSeen[i].Restore(savefile);
	}

	// grayman #3681
	m_noisemakersHeard.Clear();
	savefile->ReadInt( num );
	m_noisemakersHeard.SetNum( num );
	for ( int i = 0 ; i < num ; i++ )
	{
		m_noisemakersHeard[i].Restore(savefile);
	}

	int size;
	savefile->ReadInt(size);
	unlockableDoors.clear();
	for (int i = 0; i < size; i++)
	{
		CBinaryFrobMover* mover;
		savefile->ReadObject( reinterpret_cast<idClass *&>( mover ) );
		unlockableDoors.insert(mover);
	}

	savefile->ReadInt(size);
	tactileIgnoreEntities.clear();
	for (int i = 0; i < size; i++)
	{
		idEntity* tactEnt;
		savefile->ReadObject(reinterpret_cast<idClass *&>(tactEnt));
		tactileIgnoreEntities.insert(tactEnt);
	}
	
	savefile->ReadVec3(lastSearchedSpot); // grayman #4220

	mind = ai::MindPtr(new ai::Mind(this));
	mind->Restore(savefile);

	// Allocate and install the subsystems
	movementSubsystem = ai::MovementSubsystemPtr(new ai::MovementSubsystem(ai::SubsysMovement, this));
	senseSubsystem = ai::SubsystemPtr(new ai::Subsystem(ai::SubsysSenses, this));
	commSubsystem = ai::CommunicationSubsystemPtr(new ai::CommunicationSubsystem(ai::SubsysCommunication, this));
	actionSubsystem = ai::SubsystemPtr(new ai::Subsystem(ai::SubsysAction, this));
	searchSubsystem = ai::SubsystemPtr(new ai::Subsystem(ai::SubsysSearch, this)); // grayman #3857

	senseSubsystem->Restore(savefile);
	movementSubsystem->Restore(savefile);
	commSubsystem->Restore(savefile);
	actionSubsystem->Restore(savefile);
	searchSubsystem->Restore(savefile); // grayman #3857

	SetCombatModel();
	LinkCombat();

	InitMuzzleFlash();

	// Link the script variables back to the scriptobject
	LinkScriptVariables();

	if ( restorePhysics ) {
		RestorePhysics( &physicsObj );
	}

	RESTORE_TIMER_HANDLE(aiThinkTimer, savefile);
	RESTORE_TIMER_HANDLE(aiMindTimer, savefile);
	RESTORE_TIMER_HANDLE(aiAnimationTimer, savefile);
	RESTORE_TIMER_HANDLE(aiPushWithAFTimer, savefile);
	RESTORE_TIMER_HANDLE(aiUpdateEnemyPositionTimer, savefile);
	RESTORE_TIMER_HANDLE(aiScriptTimer, savefile);
	RESTORE_TIMER_HANDLE(aiAnimMoveTimer, savefile);
	RESTORE_TIMER_HANDLE(aiObstacleAvoidanceTimer, savefile);
	RESTORE_TIMER_HANDLE(aiPhysicsTimer, savefile);
	RESTORE_TIMER_HANDLE(aiGetMovePosTimer, savefile);
	RESTORE_TIMER_HANDLE(aiPathToGoalTimer, savefile);
	RESTORE_TIMER_HANDLE(aiGetFloorPosTimer, savefile);
	RESTORE_TIMER_HANDLE(aiPointReachableAreaNumTimer, savefile);
	RESTORE_TIMER_HANDLE(aiCanSeeTimer, savefile);
}

ai::Subsystem* idAI::GetSubsystem(ai::SubsystemId id)
{
	switch (id)
	{
	case ai::SubsysSenses:
		return senseSubsystem.get();
	case ai::SubsysMovement:
		return movementSubsystem.get();
	case ai::SubsysCommunication:
		return commSubsystem.get();
	case ai::SubsysAction:
		return actionSubsystem.get();
	case ai::SubsysSearch: // grayman #3857
		return searchSubsystem.get();
	default:
		gameLocal.Error("Request for unknown subsystem %d", static_cast<int>(id));
		return NULL;
	};
}

/*
=====================
idAI::Spawn
=====================
*/
void idAI::Spawn( void )
{
	const char			*jointname;
	const idKeyValue	*kv;
	idStr				jointName;
	idAngles			jointScale;
	jointHandle_t		joint;
	idVec3				local_dir;
	bool				talks;

	aiNode.AddToEnd(gameLocal.spawnedAI);

	// Allocate a new default mind
	mind = ai::MindPtr(new ai::Mind(this));

	// Allocate and install the subsystems
	movementSubsystem = ai::MovementSubsystemPtr(new ai::MovementSubsystem(ai::SubsysMovement, this));
	senseSubsystem = ai::SubsystemPtr(new ai::Subsystem(ai::SubsysSenses, this));
	commSubsystem = ai::CommunicationSubsystemPtr(new ai::CommunicationSubsystem(ai::SubsysCommunication, this));
	actionSubsystem = ai::SubsystemPtr(new ai::Subsystem(ai::SubsysAction, this));
	searchSubsystem = ai::SubsystemPtr(new ai::Subsystem(ai::SubsysSearch, this)); // grayman #3857

	if ( !g_monsters.GetBool() ) {
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	spawnArgs.GetInt(	"team",					"1",		team );
	spawnArgs.GetInt(	"rank",					"0",		rank );
	spawnArgs.GetInt(	"fly_offset",			"0",		fly_offset );
	spawnArgs.GetFloat( "fly_speed",			"100",		fly_speed );
	spawnArgs.GetFloat( "fly_bob_strength",		"50",		fly_bob_strength );
	spawnArgs.GetFloat( "fly_bob_vert",			"2",		fly_bob_vert );
	spawnArgs.GetFloat( "fly_bob_horz",			"2.7",		fly_bob_horz );
	spawnArgs.GetFloat( "fly_seek_scale",		"4",		fly_seek_scale );
	spawnArgs.GetFloat( "fly_roll_scale",		"90",		fly_roll_scale );
	spawnArgs.GetFloat( "fly_roll_max",			"60",		fly_roll_max );
	spawnArgs.GetFloat( "fly_pitch_scale",		"45",		fly_pitch_scale );
	spawnArgs.GetFloat( "fly_pitch_max",		"30",		fly_pitch_max );

	maxAreaReevaluationInterval = spawnArgs.GetInt( "max_area_reevaluation_interval", "2000");
	doorRetryTime = SEC2MS(spawnArgs.GetInt( "door_retry_time", "120"));

	spawnArgs.GetFloat( "fire_range",			"0",		fire_range );
	spawnArgs.GetFloat( "projectile_height_to_distance_ratio",	"1", projectile_height_to_distance_ratio );

	spawnArgs.GetFloat( "turn_rate",			"360",		turnRate );

	spawnArgs.GetVector("sitting_turn_pivot",	"-20 0 0",	sitting_turn_pivot);

	reachedpos_bbox_expansion = spawnArgs.GetFloat("reachedpos_bbox_expansion", "0");
	aas_reachability_z_tolerance = spawnArgs.GetFloat("aas_reachability_z_tolerance", "75");

	spawnArgs.GetBool( "talks",					"0",		talks );

	//// DarkMod: Alert level parameters
	// The default values of these spawnargs are normally set in tdm_ai_base.def, so the default values
	// here are somewhat superfluous. It's better than having defaults of 0 here though.
	spawnArgs.GetFloat( "alert_thresh1",		"1.5",		thresh_1 ); // The alert level threshold for reaching ObservantState (bark, but otherwise no reaction)
	spawnArgs.GetFloat( "alert_thresh2",		"6",		thresh_2 ); // The alert level threshold for reaching SuspiciousState (bark, look, may stop and turn)
	spawnArgs.GetFloat( "alert_thresh3",		"10",		thresh_3 ); // The alert level threshold for reaching SearchingState (Investigation) // grayman #3492 - was 8
	spawnArgs.GetFloat( "alert_thresh4",		"18",		thresh_4 ); // The alert level threshold for reaching AgitatedSearchingState
																		// (Investigation, Weapon out, AI is quite sure that there is someone around)
	spawnArgs.GetFloat( "alert_thresh5",		"23",		thresh_5 ); // The alert level threshold for reaching CombatState
	// Grace period info for each alert level
	spawnArgs.GetFloat( "alert_gracetime1",		"2",		m_gracetime_1 );
	spawnArgs.GetFloat( "alert_gracetime2",		"2",		m_gracetime_2 );
	spawnArgs.GetFloat( "alert_gracetime3",		"3",		m_gracetime_3 );
	spawnArgs.GetFloat( "alert_gracetime4",		"2",		m_gracetime_4 );
	spawnArgs.GetFloat( "alert_gracefrac1",		"1.2",		m_gracefrac_1 );
	spawnArgs.GetFloat( "alert_gracefrac2",		"1.2",		m_gracefrac_2 );
	spawnArgs.GetFloat( "alert_gracefrac3",		"1",		m_gracefrac_3 );
	spawnArgs.GetFloat( "alert_gracefrac4",		"1.0",		m_gracefrac_4 );
	spawnArgs.GetInt  ( "alert_gracecount1",	"5",		m_gracecount_1 );
	spawnArgs.GetInt  ( "alert_gracecount2",	"5",		m_gracecount_2 );
	spawnArgs.GetInt  ( "alert_gracecount3",	"4",		m_gracecount_3 );
	spawnArgs.GetInt  ( "alert_gracecount4",	"4",		m_gracecount_4 );
	// De-alert times for each alert level
	spawnArgs.GetFloat( "alert_time1",			"5",		atime1 );
	spawnArgs.GetFloat( "alert_time2",			"8",		atime2 );
	spawnArgs.GetFloat( "alert_time3",			"25",		atime3 ); // grayman #3492 - was 30
	spawnArgs.GetFloat( "alert_time4",			"65",		atime4 );
	spawnArgs.GetFloat( "alert_time1_fuzzyness",			"1.5",		atime1_fuzzyness );
	spawnArgs.GetFloat( "alert_time2_fuzzyness",			"2",		atime2_fuzzyness );
	spawnArgs.GetFloat( "alert_time3_fuzzyness",			"8",		atime3_fuzzyness ); // grayman #3492 - was 10
	spawnArgs.GetFloat( "alert_time4_fuzzyness",			"20",		atime4_fuzzyness );

	spawnArgs.GetFloat( "alert_time_fleedone",				"80",		atime_fleedone );
	spawnArgs.GetFloat( "alert_time_fleedone_fuzzyness",	"40",		atime_fleedone_fuzzyness );
	spawnArgs.GetBool( "canSearch",							"1",		m_canSearch); // grayman #3069

	// State setup
	backboneStates.clear();

	backboneStates[ai::ERelaxed]			= spawnArgs.GetString("state_name_0", STATE_IDLE);
	backboneStates[ai::EObservant]			= spawnArgs.GetString("state_name_1", STATE_OBSERVANT);
	backboneStates[ai::ESuspicious]			= spawnArgs.GetString("state_name_2", STATE_SUSPICIOUS);
	backboneStates[ai::ESearching]			= spawnArgs.GetString("state_name_3", STATE_SEARCHING);
	backboneStates[ai::EAgitatedSearching]	= spawnArgs.GetString("state_name_4", STATE_AGITATED_SEARCHING);
	backboneStates[ai::ECombat]				= spawnArgs.GetString("state_name_5", STATE_COMBAT);
	
	spawnArgs.GetInt( "max_interleave_think_frames",		"12",		m_maxInterleaveThinkFrames );
	spawnArgs.GetFloat( "min_interleave_think_dist",		"1000",		m_minInterleaveThinkDist);
	spawnArgs.GetFloat( "max_interleave_think_dist",		"3000",		m_maxInterleaveThinkDist);

	spawnArgs.GetBool( "ignore_alerts",						"0",		m_bIgnoreAlerts );
	spawnArgs.GetBool( "drunk",								"0",		m_drunk );

	if (spawnArgs.GetBool("canOperateElevators", "0"))
	{
		travelFlags |= TFL_ELEVATOR;
	}

	float headTurnSec;
	spawnArgs.GetFloat( "headturn_delay_min",				"3",		headTurnSec);
	m_timeBetweenHeadTurnChecks = SEC2MS(headTurnSec);

	spawnArgs.GetFloat( "headturn_chance_idle",				"0.1",		m_headTurnChanceIdle);
	spawnArgs.GetFloat( "headturn_factor_alerted",			"2",		m_headTurnFactorAlerted);
	spawnArgs.GetFloat( "headturn_yaw",						"60",		m_headTurnMaxYaw);
	spawnArgs.GetFloat( "headturn_pitch",					"40",		m_headTurnMaxPitch);

	spawnArgs.GetFloat( "headturn_duration_min",			"1",		headTurnSec);
	m_headTurnMinDuration = SEC2MS(headTurnSec);

	spawnArgs.GetFloat( "headturn_duration_max",			"3",		headTurnSec);
	m_headTurnMaxDuration = SEC2MS(headTurnSec);

	// grayman #3424 - Several alert types need to have the same weight,
	// so that a new instance of any of them will overide the previous instance
	// of any of them. Otherwise we have precedence problems when an AI is
	// busy searching because of one of them and a new stimulus arrives.
	alertTypeWeight[ai::EAlertTypeHitByProjectile]		= 40; // grayman #3331
	alertTypeWeight[ai::EAlertTypeEnemy]				= 40; // also covers EAlertTypeFoundEnemy, EAlertTypeLostTrackOfEnemy
	alertTypeWeight[ai::EAlertTypeFailedKO]				= 40;
	alertTypeWeight[ai::EAlertTypeDeadPerson]			= 40;
	alertTypeWeight[ai::EAlertTypeUnconsciousPerson]	= 40;

	//alertTypeWeight[ai::EAlertTypeHitByProjectile]	= 55; // grayman #3331
	//alertTypeWeight[ai::EAlertTypeEnemy]				= 50;
	//alertTypeWeight[ai::EAlertTypeDamage]				= 45;
	//alertTypeWeight[ai::EAlertTypeDeadPerson]			= 41;

	alertTypeWeight[ai::EAlertTypeBlinded]				= 36; // grayman #3857
	//alertTypeWeight[ai::EAlertTypeWeapon]				= 35; // grayman #3992
	alertTypeWeight[ai::EAlertTypeRope]					= 34; // grayman #2872 (dangling rope)
	alertTypeWeight[ai::EAlertTypeSuspiciousItem]		= 33; // grayman #1327 (stuck arrows or flying fireballs)
	alertTypeWeight[ai::EAlertTypeBlood]				= 30; // blood marker
	alertTypeWeight[ai::EAlertTypeBrokenItem]			= 26; // shattered glass
	alertTypeWeight[ai::EAlertTypeMissingItem]			= 25; // stolen loot
	alertTypeWeight[ai::EAlertTypeWeapon]				= 24; // #3992 debug (dropped melee or ranged weapons)
	alertTypeWeight[ai::EAlertTypeDoor]					= 20; // door found open should be closed
	alertTypeWeight[ai::EAlertTypeLightSource]			= 10; // light found off should be on
	alertTypeWeight[ai::EAlertTypeSuspiciousVisual]		=  8; // grayman #3857 (got a glimpse of the player)
	alertTypeWeight[ai::EAlertTypeHitByMoveable]		=  7; // hit by something
	alertTypeWeight[ai::EAlertTypePickedPocket]			=  6; // grayman #3857
	alertTypeWeight[ai::EAlertTypeSuspicious]			=  5; // audio alerts
	alertTypeWeight[ai::EAlertTypeNone]					=  0;

	// grayman #3857 - the following alert types are used to differentiate
	// alert type and search processing, but aren't used explicitly to test
	// alert weighting, and won't appear in the alertTypeWeight[] vector:

	// EAlertTypeEncounter
	// EAlertTypeRequestForHelp
	// EAlertTypeDetectedEnemy
	// EAlertTypeSomethingSuspicious

	// DarkMod: Get the movement type audible volumes from the spawnargs
	spawnArgs.GetFloat( "stepvol_walk",			"0",		m_stepvol_walk );
	spawnArgs.GetFloat( "stepvol_run",			"0",		m_stepvol_run );
	spawnArgs.GetFloat( "stepvol_creep",		"0",		m_stepvol_creep );

	spawnArgs.GetFloat( "stepvol_crouch_walk",			"0",		m_stepvol_crouch_walk );
	spawnArgs.GetFloat( "stepvol_run",			"0",		m_stepvol_crouch_run );
	spawnArgs.GetFloat( "stepvol_creep",		"0",		m_stepvol_crouch_creep );

	if ( spawnArgs.GetString( "npc_name", NULL ) != NULL ) {
		if ( talks ) {
			talk_state = TALK_OK;
		} else {
			talk_state = TALK_BUSY;
		}
	} else {
		talk_state = TALK_NEVER;
	}

	spawnArgs.GetBool( "animate_z",				"0",		disableGravity );
	spawnArgs.GetFloat( "kick_force",			"60",		kickForce ); // grayman #2568 - use default Doom 3 value
	spawnArgs.GetBool( "ignore_obstacles",		"0",		ignore_obstacles );
	spawnArgs.GetFloat( "blockedRadius",		"-1",		blockedRadius );
	spawnArgs.GetInt( "blockedMoveTime",		"750",		blockedMoveTime );
	spawnArgs.GetInt( "blockedAttackTime",		"750",		blockedAttackTime );

	// DarkMod: Set the AI acuities from the spawnargs.

	m_Acuities.Clear();
	for ( int ind = 0 ; ind < g_Global.m_AcuityNames.Num() ; ind++ )
	{
		float tempFloat = spawnArgs.GetFloat( va("acuity_%s", g_Global.m_AcuityNames[ind].c_str()), "100" );
		// angua: divide by 100 to transform percent into fractions
		tempFloat *= 0.01f;
		m_Acuities.Append( tempFloat );
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Acuities Array: index %d, name %s, value %f\r", ind, g_Global.m_AcuityNames[ind].c_str(), m_Acuities[ind]);
	}
	m_oldVisualAcuity = GetBaseAcuity("vis"); // grayman #3552
	//m_oldVisualAcuity = GetAcuity("vis"); // Tels fix #2408

	spawnArgs.GetFloat("alert_aud_thresh", va("%f",gameLocal.m_sndProp->m_SndGlobals.DefaultThreshold), m_AudThreshold );
	spawnArgs.GetInt(	"num_cinematics",		"0",		num_cinematics );
	current_cinematic = 0;

	LinkScriptVariables();

	/**
	* Initialize Darkmod AI vars
	**/
	AI_ALERTED = false;
	AI_CROUCH = false;
	AI_RUN = false;
	AI_CREEP = false;

	AI_LAY_DOWN_LEFT = true;

	AI_HEARDSOUND = false;
	AI_VISALERT = false;
	AI_TACTALERT = false;

	AI_SleepLocation = spawnArgs.GetInt("sleep_location", "1"); // grayman #3820

	fl.takedamage		= !spawnArgs.GetBool( "noDamage" );
	enemy				= NULL;
	allowMove			= true;
	allowHiddenMovement = false;

	animator.RemoveOriginOffset( true );

	// create combat collision hull for exact collision detection
	SetCombatModel();

	lookMin	= spawnArgs.GetAngles( "look_min", "-80 -75 0" );
	lookMax	= spawnArgs.GetAngles( "look_max", "80 75 0" );

	lookJoints.SetGranularity( 1 );
	lookJointAngles.SetGranularity( 1 );
	kv = spawnArgs.MatchPrefix( "look_joint", NULL );
	while( kv ) {
		jointName = kv->GetKey();
		jointName.StripLeadingOnce( "look_joint " );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Warning( "Unknown look_joint '%s' on entity %s", jointName.c_str(), name.c_str() );
		} else {
			jointScale = spawnArgs.GetAngles( kv->GetKey(), "0 0 0" );
			jointScale.roll = 0.0f;

			// if no scale on any component, then don't bother adding it.  this may be done to
			// zero out rotation from an inherited entitydef.
			if ( jointScale != ang_zero ) {
				lookJoints.Append( joint );
				lookJointAngles.Append( jointScale );
			}
		}
		kv = spawnArgs.MatchPrefix( "look_joint", kv );
	}

	lookJointsCombat.SetGranularity( 1 );
	lookJointAnglesCombat.SetGranularity( 1 );
	kv = spawnArgs.MatchPrefix( "combat_look_joint", NULL );
	while( kv ) {
		jointName = kv->GetKey();
		jointName.StripLeadingOnce( "combat_look_joint " );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Warning( "Unknown combat_look_joint '%s' on entity %s", jointName.c_str(), name.c_str() );
		} else {
			jointScale = spawnArgs.GetAngles( kv->GetKey(), "0 0 0" );
			jointScale.roll = 0.0f;

			// if no scale on any component, then don't bother adding it.  this may be done to
			// zero out rotation from an inherited entitydef.
			if ( jointScale != ang_zero ) {
				lookJointsCombat.Append( joint );
				lookJointAnglesCombat.Append( jointScale );
			}
		}
		kv = spawnArgs.MatchPrefix( "combat_look_joint", kv );
	}

	// calculate joint positions on attack frames so we can do proper "can hit" tests
	CalculateAttackOffsets();

	eyeMin				= spawnArgs.GetAngles( "eye_turn_min", "-10 -30 0" );
	eyeMax				= spawnArgs.GetAngles( "eye_turn_max", "10 30 0" );
	eyeVerticalOffset	= spawnArgs.GetFloat( "eye_verticle_offset", "5" );
	eyeHorizontalOffset = spawnArgs.GetFloat( "eye_horizontal_offset", "-8" );
	eyeFocusRate		= spawnArgs.GetFloat( "eye_focus_rate", "0.5" );
	headFocusRate		= spawnArgs.GetFloat( "head_focus_rate", "0.1" );
	focusAlignTime		= SEC2MS( spawnArgs.GetFloat( "focus_align_time", "1" ) );

	// DarkMod: State of mind, allow the FM author to set initial values
	AI_AlertLevel			= spawnArgs.GetFloat( "alert_initial", "0" );

	flashJointWorld = animator.GetJointHandle( "flash" );

	if ( head.GetEntity() ) 
	{
		idAnimator *headAnimator = head.GetEntity()->GetAnimator();

		jointname = spawnArgs.GetString( "bone_focus" );
		if ( *jointname ) 
		{
			focusJoint = headAnimator->GetJointHandle( jointname );
			if ( focusJoint == INVALID_JOINT ) 
			{
				gameLocal.Warning( "Joint '%s' not found on head on '%s'", jointname, name.c_str() );
			}
		}
	} else 
	{
		jointname = spawnArgs.GetString( "bone_focus" );
		if ( *jointname ) 
		{
			focusJoint = animator.GetJointHandle( jointname );
			if ( focusJoint == INVALID_JOINT ) 
			{
				gameLocal.Warning( "Joint '%s' not found on '%s'", jointname, name.c_str() );
			}
		}
	}

	jointname = spawnArgs.GetString( "bone_orientation" );
	if ( *jointname ) {
		orientationJoint = animator.GetJointHandle( jointname );
		if ( orientationJoint == INVALID_JOINT ) {
			gameLocal.Warning( "Joint '%s' not found on '%s'", jointname, name.c_str() );
		}
	}

	jointname = spawnArgs.GetString( "bone_flytilt" );
	if ( *jointname ) {
		flyTiltJoint = animator.GetJointHandle( jointname );
		if ( flyTiltJoint == INVALID_JOINT ) {
			gameLocal.Warning( "Joint '%s' not found on '%s'", jointname, name.c_str() );
		}
	}

	InitMuzzleFlash();

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );

	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	kickForce = 2*physicsObj.GetMass(); // grayman #2568 - equation arrived at empirically

	physicsObj.SetStepUpIncrease(spawnArgs.GetFloat("step_up_increase", "0"));

	if ( spawnArgs.GetBool( "big_monster" ) ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetClipMask( MASK_MONSTERSOLID & ~CONTENTS_BODY );
	} else {
		if ( use_combat_bbox ) {
			physicsObj.SetContents( CONTENTS_BODY|CONTENTS_SOLID );
		} else {
			physicsObj.SetContents( CONTENTS_BODY );
		}
		physicsObj.SetClipMask( MASK_MONSTERSOLID );
	}

// SR CONTENTS_RESPONSE fix:
	if( m_StimResponseColl->HasResponse() )
		physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );

	// move up to make sure the monster is at least an epsilon above the floor
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );

	if ( num_cinematics ) {
		physicsObj.SetGravity( vec3_origin );
	} else {
		idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
		gravity *= g_gravity.GetFloat();
		physicsObj.SetGravity( gravity );
	}

	SetPhysics( &physicsObj );

	physicsObj.GetGravityAxis().ProjectVector( viewAxis[ 0 ], local_dir );
	current_yaw		= local_dir.ToYaw();
	ideal_yaw		= idMath::AngleNormalize180( current_yaw );

	move.blockTime = 0;

	SetAAS();

	InitProjectileInfo();

	delayedStims.Clear(); // grayman #2603

	alertQueue.Clear(); // grayman #4002

	particles.Clear();
	restartParticles = true;
	useBoneAxis = spawnArgs.GetBool( "useBoneAxis" );
	SpawnParticles( "smokeParticleSystem" );

	if ( num_cinematics || spawnArgs.GetBool( "hide" ) || spawnArgs.GetBool( "teleport" ) || spawnArgs.GetBool( "trigger_anim" ) ) {
		fl.takedamage = false;
		physicsObj.SetContents( 0 );
		physicsObj.GetClipModel()->Unlink();
		Hide();
	} else {
		// play a looping ambient sound if we have one
		StartSound( "snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL );
	}

	if ( health <= 0 ) {
		gameLocal.Warning( "entity '%s' doesn't have health set", name.c_str() );
		health = 1;
	}

	// Dark Mod: set up drowning
	// set up drowning timer (add a random bit to make it asynchronous w/ respect to other AI)
	m_bCanDrown = spawnArgs.GetBool( "can_drown", "1" );
	m_AirCheckTimer = gameLocal.time + gameLocal.random.RandomInt( 8000 );
	m_AirTicksMax = spawnArgs.GetInt( "max_air_tics", "5" );
	m_AirTics = m_AirTicksMax;
	m_AirCheckInterval = static_cast<int>(1000.0f * spawnArgs.GetFloat( "air_check_interval", "4.0" ));
	// end drowning setup

	m_bPushOffPlayer = spawnArgs.GetBool("push_off_player", "1");

	m_bCanBeFlatFooted	= spawnArgs.GetBool("can_be_flatfooted", "1");
	m_FlatFootedTime	= spawnArgs.GetInt("flatfooted_time");
	m_FlatFootParryMax = spawnArgs.GetInt("flatfoot_parry_num");
	m_FlatFootParryTime = spawnArgs.GetInt("flatfoot_parry_time");
	// Convert percent chance to fractional
	m_MeleeCounterAttChance = spawnArgs.GetInt("melee_chance_to_counter") / 100.0f;
	m_bMeleePredictProximity = spawnArgs.GetBool("melee_predicts_proximity");

	if (spawnArgs.GetBool("melee_attacks_enabled_at_spawn_time", "0"))
	{
		SetAttackFlag(COMBAT_MELEE, true);
	}

	if (spawnArgs.GetBool("ranged_attacks_enabled_at_spawn_time", "0"))
	{
		SetAttackFlag(COMBAT_RANGED, true);
	}

	m_bCanOperateDoors = spawnArgs.GetBool("canOperateDoors", "0");
	m_bCanCloseDoors = spawnArgs.GetBool("canCloseDoors", "1");
	m_HandlingDoor = false;
	m_DoorQueued = false;		// grayman #3647
	m_ElevatorQueued = false;	// grayman #3647
	m_RestoreMove = false;		// grayman #2706
	m_LatchedSearch = false;	// grayman #2603
	m_dousedLightsSeen.Clear();	// grayman #2603
	m_noisemakersHeard.Clear(); // grayman #3681

	m_HandlingElevator = false;
	m_CanSetupDoor = true;		// grayman #3029
	m_RelightingLight = false;	// grayman #2603
	m_ExaminingRope = false;	// grayman #2872
	m_DroppingTorch = false;	// grayman #2603
	m_ReactingToPickedPocket = false; // grayman #3559
	m_InConversation = false;	// grayman #3559
	m_nextWarningTime = 0;		// grayman #5164

	// =============== Set up KOing and FOV ==============
	const char *HeadJointName = spawnArgs.GetString("head_jointname", "Head");

	m_HeadJointID = animator.GetJointHandle(HeadJointName);
	if( m_HeadJointID == INVALID_JOINT )
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Invalid head joint for joint %s on AI %s \r", HeadJointName, name.c_str());
		m_HeadBodyID = 0;
	}
	else
	{
		m_HeadBodyID = BodyForJoint(m_HeadJointID);
	}

	if( head.GetEntity() )
	{
		CopyHeadKOInfo();
	}

	ParseKnockoutInfo();

	// ================== End KO setup ====================

	// Sneak attack setup
	const char *tempc1;
	tempc1 = spawnArgs.GetString("sneak_attack_alert_state");
	m_SneakAttackThresh = spawnArgs.GetFloat( va("alert_thresh%s", tempc1), va("%f",idMath::INFINITY) );
	m_SneakAttackMult = spawnArgs.GetFloat( "sneak_attack_mult", "1.0" );

	BecomeActive( TH_THINK );

	// init the move variables
	if ( idStr::Icmp( spawnArgs.GetString("movetype"),"FLY" ) == 0 )
	{
		move.moveType = MOVETYPE_FLY;
	}

	StopMove( MOVE_STATUS_DONE );

	// Schedule a post-spawn event to parse the rest of the spawnargs
	PostEventMS( &EV_PostSpawn, 1 );

	m_lastThinkTime = gameLocal.time;

	CREATE_TIMER(aiThinkTimer, name, "Think");
	CREATE_TIMER(aiMindTimer, name, "Mind");
	CREATE_TIMER(aiAnimationTimer, name, "Animation");
	CREATE_TIMER(aiPushWithAFTimer, name, "PushWithAF");
	CREATE_TIMER(aiUpdateEnemyPositionTimer, name, "UpdateEnemyPosition");
	CREATE_TIMER(aiScriptTimer, name, "UpdateScript");
	CREATE_TIMER(aiAnimMoveTimer, name, "AnimMove");
	CREATE_TIMER(aiObstacleAvoidanceTimer, name, "ObstacleAvoidance");
	CREATE_TIMER(aiPhysicsTimer, name, "RunPhysics");
	CREATE_TIMER(aiGetMovePosTimer, name, "GetMovePos");
	CREATE_TIMER(aiPathToGoalTimer, name, "PathToGoal");
	CREATE_TIMER(aiGetFloorPosTimer, name, "GetFloorPos");
	CREATE_TIMER(aiPointReachableAreaNumTimer, name, "PointReachableAreaNum");
	CREATE_TIMER(aiCanSeeTimer, name, "CanSee");

	m_pathRank = rank; // grayman #2345 - rank for path-finding

	m_bCanBeGassed = !(spawnArgs.GetBool( "gas_immune", "0" )); // grayman #2468
	
	mind->InitStateQueue(); // grayman #3714 - initialize the state queue
}

void idAI::InitProjectileInfo()
{
	activeProjectile.projEnt = NULL;

	// greebo: Pre-cache the properties of each projectile def.
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("def_projectile"); kv != NULL; kv = spawnArgs.MatchPrefix("def_projectile", kv))
	{
		const idStr& projectileName = kv->GetValue();
		
		if (projectileName.Length() == 0) continue;

		ProjectileInfo& info = projectileInfo.Alloc();

		// Pass to specialised routine
		InitProjectileInfoFromDict(info, projectileName);
	}

	// Roll a random projectile for starters
	curProjectileIndex = gameLocal.random.RandomInt(projectileInfo.Num());
}

void idAI::InitProjectileInfoFromDict(idAI::ProjectileInfo& info, const char* entityDef) const
{
	const idDict* dict = gameLocal.FindEntityDefDict(entityDef,true); // grayman #3391 - don't create a default 'dict'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message

	if (dict == NULL)
	{
		gameLocal.Error("Cannot load projectile info for entityDef %s", entityDef);
	}
	
	InitProjectileInfoFromDict(info, dict);
}

void idAI::InitProjectileInfoFromDict(idAI::ProjectileInfo& info, const idDict* dict) const
{
	assert(dict != NULL);

	info.defName = dict->GetString("classname");
	info.def = dict;
	
	// Create a projectile of this specific type to retrieve its properties
	idProjectile* proj = SpawnProjectile(info.def);

	info.radius	= proj->GetPhysics()->GetClipModel()->GetBounds().GetRadius();
	info.velocity = idProjectile::GetVelocity(info.def);
	info.gravity = idProjectile::GetGravity(info.def);
	info.speed = info.velocity.Length();

	// dispose after this short use
	delete proj;
}

/*
===================
idAI::InitMuzzleFlash
===================
*/
void idAI::InitMuzzleFlash( void ) {
	const char			*shader;
	idVec3				flashColor;

	spawnArgs.GetString( "mtr_flashShader", "muzzleflash", &shader );
	spawnArgs.GetVector( "flashColor", "0 0 0", flashColor );
	float flashRadius = spawnArgs.GetFloat( "flashRadius" );
	flashTime = SEC2MS( spawnArgs.GetFloat( "flashTime", "0.25" ) );

	memset( &worldMuzzleFlash, 0, sizeof ( worldMuzzleFlash ) );

	worldMuzzleFlash.pointLight = true;
	worldMuzzleFlash.shader = declManager->FindMaterial( shader, false );
	worldMuzzleFlash.shaderParms[ SHADERPARM_RED ] = flashColor[0];
	worldMuzzleFlash.shaderParms[ SHADERPARM_GREEN ] = flashColor[1];
	worldMuzzleFlash.shaderParms[ SHADERPARM_BLUE ] = flashColor[2];
	worldMuzzleFlash.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
	worldMuzzleFlash.shaderParms[ SHADERPARM_TIMESCALE ] = 1.0f;
	worldMuzzleFlash.lightRadius[0] = flashRadius;
	worldMuzzleFlash.lightRadius[1]	= flashRadius;
	worldMuzzleFlash.lightRadius[2]	= flashRadius;

	worldMuzzleFlashHandle = -1;
}

/*
===================
idAI::List_f
===================
*/
void idAI::List_f( const idCmdArgs &args ) {
	int		e;
	idAI	*check;
	int		count;
	const char *statename;

	count = 0;

	gameLocal.Printf( "%-4s  %-20s %s\n", " Num", "EntityDef", "Name" );
	gameLocal.Printf( "------------------------------------------------\n" );
	for( e = 0; e < MAX_GENTITIES; e++ ) {
		check = static_cast<idAI *>(gameLocal.entities[ e ]);
		if ( !check || !check->IsType( idAI::Type ) ) {
			continue;
		}

		if ( check->state ) {
			statename = check->state->Name();
		} else {
			statename = "NULL state";
		}

		gameLocal.Printf( "%4i: %-20s %-20s %s  move: %d\n", e, check->GetEntityDefName(), check->name.c_str(), statename, check->allowMove );
		count++;
	}

	gameLocal.Printf( "...%d monsters\n", count );
}

/*
================
idAI::DormantBegin

called when entity becomes dormant
================
*/
void idAI::DormantBegin( void ) {
	// since dormant happens on a timer, we wont get to update particles to
	// hidden through the think loop, but we need to hide them though.
	if ( particles.Num() ) {
		for ( int i = 0; i < particles.Num(); i++ ) {
			particles[i].time = 0;
		}
	}

	if ( enemyNode.InList() ) {
		// remove ourselves from the enemy's enemylist
		enemyNode.Remove();
	}

	idActor::DormantBegin();
}

/*
================
idAI::DormantEnd

called when entity wakes from being dormant
================
*/
void idAI::DormantEnd( void ) {
	if ( enemy.GetEntity() && !enemyNode.InList() ) {
		// let our enemy know we're back on the trail
		enemyNode.AddToEnd( enemy.GetEntity()->enemyList );
	}

	if ( particles.Num() ) {
		for ( int i = 0; i < particles.Num(); i++ ) {
			particles[i].time = gameLocal.time;
		}
	}

	idActor::DormantEnd();
	m_lastThinkTime = gameLocal.time;
}

/*
=====================
idAI::Think
=====================
*/
void idAI::Think( void ) 
{
	START_SCOPED_TIMING(aiThinkTimer, scopedThinkTimer);
	if (cv_ai_opt_nothink.GetBool()) 
	{
		return; // Thinking is disabled.
	}

	if (!ThinkingIsAllowed())
	{
		return;
	}

	SetNextThinkFrame();

	//PrintGoalData(move.moveDest, 10);
	// grayman #2416 - don't let origin slip below the floor when getting up from lying down
	if ( ( gameLocal.time <= m_getupEndTime ) &&
		 ( idStr(WaitState()) == "wake_up") &&
		 ((AI_SleepLocation == SLEEP_LOC_FLOOR) || (AI_SleepLocation == SLEEP_LOC_BED)))// grayman #3820
	{
		idVec3 origin = physicsObj.GetOrigin();
		if ( origin.z < m_sleepFloorZ )
		{
			origin.z = m_sleepFloorZ;
			physicsObj.SetOrigin(origin);
		}
	}

	// grayman #3989 - don't let origin slip below the floor when lying down
	if ((idStr(WaitState()) == "fall_asleep") &&
		((AI_SleepLocation == SLEEP_LOC_FLOOR) || (AI_SleepLocation == SLEEP_LOC_BED)) )// grayman #3820
	{
		idVec3 origin = physicsObj.GetOrigin();
		if ( origin.z < m_sleepFloorZ )
		{
			origin.z = m_sleepFloorZ;
			physicsObj.SetOrigin(origin);
		}
	}
			
	// if we are completely closed off from the player, don't do anything at all
	// angua: only go dormant while in idle
	// grayman #2536 - move alert check up in front
	if (AI_AlertIndex < ai::EObservant)
	{
		bool outsidePVS = CheckDormant();
		if (outsidePVS && cv_ai_opt_disable.GetBool())
		{
			return;
		}
	}

	// grayman #2536 - if dormant and alert index > ERelaxed, wake up

	if ((AI_AlertIndex > ai::ERelaxed) && fl.isDormant)
	{
		dormantStart = 0;
		fl.hasAwakened = true;
		fl.isDormant = false;
		DormantEnd();
	}

	// grayman #4412 - testing shows that dmap doesn't properly mark all portals for
	// flying AI. So the AI won't see a closed door as a solid wall. To solve this,
	// see if there's a nearby door and mark its AAS area unreachable if the AI
	// can't fly through the door. This allows him to seek an alternate route.

	if ( GetMoveType() == MOVETYPE_FLY )
	{
		idBounds bounds = physicsObj.GetAbsBounds();
		bounds[0].x -= 16;
		bounds[0].y -= 16;
		bounds[0].x += 16;
		bounds[0].y += 16;
		idClip_EntityList ents;
		int num = gameLocal.clip.EntitiesTouchingBounds( bounds, CONTENTS_SOLID, ents );
		for ( int i = 0; i < num; i++ )
		{
			// check if there's a door
			idEntity *e = ents[i];

			if ( e == NULL )
			{
				continue;
			}

			if ( e->IsType(CFrobDoor::Type) )
			{
				CFrobDoor* frobDoor = static_cast<CFrobDoor*>(e);
				bool foundImpassableDoor = false;

				if ( frobDoor->IsOpen() )
				{
					if ( !FitsThrough(frobDoor) )
					{
						foundImpassableDoor = true; // can't fit through the open door
					}
				}
				else // can't go through the closed door
				{
					foundImpassableDoor = true; // can't go through the closed door
				}

				int areaNum = frobDoor->GetAASArea(aas);
				if (areaNum > 0)
				{
					if ( foundImpassableDoor )
					{
						// add AAS area number of the door to forbidden areas
						if ( gameLocal.m_AreaManager.AddForbiddenArea(areaNum, this) ) {
							PostEventMS(&AI_ReEvaluateArea, doorRetryTime, areaNum);
						}
						frobDoor->RegisterAI(this); // grayman #1145 - this AI is interested in this door
					}
					else
					{
						// door is passable, so remove its area number from forbidden areas
						gameLocal.m_AreaManager.RemoveForbiddenArea(areaNum, this);
					}
				}
				break;
			}
		}
	}
			
	// save old origin and velocity for crashlanding
	// grayman #3699 - the physics object for an AI will change
	// from idPhysics_Monster to idPhysics_AF if the AI becomes
	// a ragdoll, so we need to use the current physics object

	idVec3 oldOrigin = GetPhysics()->GetOrigin();
	idVec3 oldVelocity = GetPhysics()->GetLinearVelocity();
	//idVec3 oldOrigin = physicsObj.GetOrigin();
	//idVec3 oldVelocity = physicsObj.GetLinearVelocity();

	// grayman #3424 - clear pain flag
	GetMemory().painStatePushedThisFrame = false;

	if (thinkFlags & TH_THINK)
	{
		// clear out the enemy when he dies
		idActor* enemyEnt = enemy.GetEntity();
		if (enemyEnt != NULL)
		{
			if (enemyEnt->health <= 0)
			{
				EnemyDead();
			}
		}

		// Calculate the new view axis based on the turning settings
		current_yaw += deltaViewAngles.yaw;
		ideal_yaw = idMath::AngleNormalize180(ideal_yaw + deltaViewAngles.yaw);
		deltaViewAngles.Zero();
		viewAxis = idAngles(0, current_yaw, 0).ToMat3();

		// TDM: Fake lipsync
		if (m_lipSyncActive && !cv_ai_opt_nolipsync.GetBool() && GetSoundEmitter())
		{
			if (gameLocal.time < m_lipSyncEndTimer && head.GetEntity() != NULL)
			{
				// greebo: Get the number of frames from the head animator
				int numFrames = head.GetEntity()->GetAnimator()->NumFrames(m_lipSyncAnim);

				int frame = static_cast<int>(numFrames * idMath::Sqrt16(GetSoundEmitter()->CurrentAmplitude()));
				frame = idMath::ClampInt(0, numFrames, frame);
				headAnim.SetFrame(m_lipSyncAnim, frame);
			}
			else
			{
				// We're done; stop the animation
				StopLipSync();
			}
		}

		// Check for tactile alert due to AI movement
		CheckTactile();

		if (health > 0) // grayman #1488 - only do this if you're still alive
		{
			UpdateAir(); // Check air ticks (is interleaved and not checked each frame)
		}

		if (num_cinematics > 0)
		{
			// Active cinematics
			if ( !IsHidden() && torsoAnim.AnimDone( 0 ) )
			{
				PlayCinematic();
			}
			RunPhysics();
		}
		else if (!allowHiddenMovement && IsHidden())
		{
			// hidden monsters
			UpdateScript();
		}
		else
		{
			// clear the ik before we do anything else so the skeleton doesn't get updated twice
			walkIK.ClearJointMods();

			// Update moves, depending on move type 
			switch (move.moveType)
			{
			case MOVETYPE_DEAD :
				// dead monsters
				UpdateScript();
				DeadMove();
				break;

			case MOVETYPE_FLY :
				// flying monsters
				UpdateEnemyPosition();
				UpdateScript();
				FlyMove();
				CheckBlink();
				break;

			case MOVETYPE_STATIC :
				// static monsters
				UpdateEnemyPosition();
				UpdateScript();
				StaticMove();
				CheckBlink();
				break;

			case MOVETYPE_ANIM :
				// animation based movement
				UpdateEnemyPosition();
				UpdateScript();
				if (!cv_ai_opt_noanims.GetBool())
				{
					AnimMove();
				}
				CheckBlink();
				break;

			case MOVETYPE_SLIDE :
				// velocity based movement
				UpdateEnemyPosition();
				UpdateScript();
				SlideMove();
				CheckBlink();
				break;

			case MOVETYPE_SIT :
				// static monsters
				UpdateEnemyPosition();
				UpdateScript();
				// moving not allowed, turning around sitting pivot
				SittingMove();
				CheckBlink();
				break;

			case MOVETYPE_SIT_DOWN :
				// static monsters
				UpdateEnemyPosition();
				UpdateScript();
				// moving and turning not allowed
				NoTurnMove();
				CheckBlink();
				break;

			case MOVETYPE_SLEEP :
				// static monsters
				UpdateEnemyPosition();
				UpdateScript();
				// moving and turning not allowed
				NoTurnMove();
				break;

			case MOVETYPE_FALL_ASLEEP : // grayman #3820 - was MOVETYPE_LAY_DOWN
				// static monsters
				UpdateEnemyPosition();
				UpdateScript();
				// moving and turning not allowed
				SleepingMove();
				break;

			case MOVETYPE_GET_UP :
				// static monsters
				UpdateEnemyPosition();
				UpdateScript();
				// moving not allowed
				SittingMove();
				CheckBlink();
				break;

			case MOVETYPE_WAKE_UP : // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
				// static monsters
				UpdateEnemyPosition();
				UpdateScript();
				// moving and turning not allowed
				SleepingMove();
				break;

			default:
				break;
			}
		}

		if (!cv_ai_opt_nomind.GetBool())
		{
			// greebo: We always rely on having a mind
			assert(mind);
			START_SCOPED_TIMING(aiMindTimer, scopedMindTimer);

			// Let the mind do the thinking (after the move updates)
			mind->Think();
		}

		// Clear DarkMod per frame vars now that the mind had time to think
		AI_ALERTED = false;
		m_AlertLevelThisFrame = 0;
		m_AlertedByActor = NULL;
		m_tactileEntity = NULL; // grayman #2345

		// clear pain flag so that we receive any damage between now and the next time we run the script
		AI_PAIN = false;
		AI_SPECIAL_DAMAGE = 0;
		AI_PUSHED = false;
	}
	else if (thinkFlags & TH_PHYSICS)
	{
		// Thinking not allowed, but physics are still enabled
		RunPhysics();
	}

	if (m_bAFPushMoveables && !movementSubsystem->IsWaitingNonSolid()) // grayman #2345 - if you're waiting for someone to go by, you're non-solid, so you can't push anything
	{
		START_SCOPED_TIMING(aiPushWithAFTimer, scopedPushWithAFTimer)
		PushWithAF();
	}

	if (fl.hidden && allowHiddenMovement)
	{
		// UpdateAnimation won't call frame commands when hidden, so call them here when we allow hidden movement
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}

	UpdateMuzzleFlash();
	if (!cv_ai_opt_noanims.GetBool())
	{
		START_SCOPED_TIMING(aiAnimationTimer, scopedAnimationTimer)
		UpdateAnimation();
	}
	UpdateParticles();

	if (!cv_ai_opt_nopresent.GetBool())
	{
		Present();
		if ( needsDecalRestore ) // #3817
		{
			ReapplyDecals();
			needsDecalRestore = false;
		}
	}

	UpdateDamageEffects();
	LinkCombat();

	// grayman #4412 - flying AI unaffected by CrashLand()
	if ((health > 0) && (GetMoveType() != MOVETYPE_FLY))
	{
		idActor::CrashLand( physicsObj, oldOrigin, oldVelocity );
	}

	m_lastThinkTime = gameLocal.time;

	// Check the CVARs and print debug info, if appropriate
	ShowDebugInfo();
}

/*
=====================
idAI::ThinkingIsAllowed
=====================
*/
bool idAI::ThinkingIsAllowed()
{
	int gameTime = gameLocal.time;

	// Ragdolls think every frame to avoid physics weirdness.
	// stgatilov: it is especially weird when grabbed by player =)
	if ( (health <= 0) || IsKnockedOut() ) // grayman #2840 - you're also a ragdoll if you're KO'ed
		return true;

	// angua: AI think every frame while sitting/laying down and getting up
	// otherwise, the AI might end up in a different sleeping position
	if (move.moveType == MOVETYPE_SIT_DOWN
		|| move.moveType == MOVETYPE_FALL_ASLEEP // grayman #3820 - was MOVETYPE_LAY_DOWN
		|| move.moveType == MOVETYPE_GET_UP
		|| move.moveType == MOVETYPE_WAKE_UP) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
	{
		return true;
	}

	// stgatilov: AIs should never wait for more than what interleaved thinking setting allows
	if (gameTime - m_lastThinkTime >= GetMaxInterleaveThinkFrames() * USERCMD_MSEC)
		return true;

	// stgatilov #5992: this is not the first game tic in current frame?
	// it means that FPS is low, so we probably need to optimize game modelling
	// let's allow AIs think only once per frame, and skip thinking on followup "minor" game tics
	if (gameLocal.minorTic)
		return false;

	// Time to think has come?
	if (gameTime >= m_nextThinkTime)
		return true;

	// skips PVS check, AI will also do interleaved thinking when in player view.
	bool skipPVScheck = cv_ai_opt_interleavethinkskippvscheck.GetBool() || cv_ai_opt_forceopt.GetBool();
	if (skipPVScheck)
		return false;

	// PVS check: let the AI think every frame as long as the player sees them.
	bool inPVS = gameLocal.InPlayerPVS(this);
	if (inPVS)
		return true;

	return false;
}



/*
=====================
idAI::SetNextThinkFrame
=====================
*/
void idAI::SetNextThinkFrame()
{
	int gameTime = gameLocal.time;
	int thinkFrame = GetThinkInterleave();
	int thinkDeltaTime = 1;

	if (thinkFrame > 1)
	{
		// Let them think for the first few frames to initialize state and tasks
		if (m_earlyThinkCounter <= 0) // grayman #2654 - keep a separate counter
		{
			// grayman #2414 - think more often if
			//
			//   * working on a door-handling task
			//   * nearing a goal position
			//   * handling an elevator
			//
			// and, for #2345
			//
			//   * when resolving a block

			bool thinkMore = false;
			ai::Memory& memory = GetMemory();
			CFrobDoor *door = memory.doorRelated.currentDoor.GetEntity();

			if (door)
			{
				idVec3 origin = GetPhysics()->GetOrigin();
				idVec3 doorOrigin = door->GetPhysics()->GetOrigin();
				float distance = (doorOrigin - origin).LengthFast();

				if (distance <= TEMP_THINK_DISTANCE)
				{
					thinkMore = true; // avoid obstruction & damage
				}
			}
			else if (m_HandlingElevator)
			{
				thinkMore = true; // avoid death
			}
			else if ((move.moveCommand == MOVE_TO_POSITION) && (move.moveStatus == MOVE_STATUS_MOVING))
			{
				idVec3 origin = GetPhysics()->GetOrigin();
				float distance = (move.moveDest - origin).LengthFast();

				if (distance <= TEMP_THINK_FACTOR*thinkFrame)
				{
					thinkMore = true; // avoid confusion and becoming stuck
				}
			}
			else if (!movementSubsystem->IsWaiting() && !movementSubsystem->IsNotBlocked())
			{
				thinkMore = true; // avoid chaos
			}

			if (thinkMore)
			{
				thinkFrame = idMath::Imin(thinkFrame, TEMP_THINK_INTERLEAVE);
			}
			thinkDeltaTime = thinkFrame * USERCMD_MSEC;
		}
		else
		{
			m_earlyThinkCounter--; // grayman #2654
		}
	}

	m_nextThinkTime = gameTime + thinkDeltaTime;
}


int idAI::GetMaxInterleaveThinkFrames() const
{
	int cvarOverride = cv_ai_opt_interleavethinkframes.GetInteger();
	if (cvarOverride > 0)
		return cvarOverride;
	return m_maxInterleaveThinkFrames;
}

/*
=====================
idAI::GetThinkInterleave
=====================
*/
int idAI::GetThinkInterleave() const // grayman 2414 - add 'const'
{
	int maxFrames = GetMaxInterleaveThinkFrames();
	if (cv_ai_opt_forceopt.GetBool())
	{
		return maxFrames;	// debug only: assume player is far
	}

	if (maxFrames == 0)
	{
		return 0;
	}

	float minDist = m_minInterleaveThinkDist;
	float maxDist = m_maxInterleaveThinkDist;
	if (cv_ai_opt_interleavethinkmindist.GetFloat() > 0)
	{
		minDist = cv_ai_opt_interleavethinkmindist.GetFloat();
	}
	if (cv_ai_opt_interleavethinkmaxdist.GetFloat() > 0)
	{
		maxDist = cv_ai_opt_interleavethinkmaxdist.GetFloat();
	}

	if (maxDist < minDist)
	{
		gameLocal.Warning("%s - Minimum distance for interleaved thinking (%f) is larger than maximum distance (%f), switching optimization off.",name.c_str(),minDist,maxDist);
		return 0;
	}

	float playerDist = (physicsObj.GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();
	if (playerDist < minDist)
	{
		return 0;
	}
	else if (playerDist > maxDist)
	{
		return maxFrames;
	}
	else
	{
		float fraction = (playerDist - minDist) / (maxDist - minDist);
		int thinkFrames = 1 + static_cast<int>(fraction * maxFrames);
		return thinkFrames;
	}
}

/***********************************************************************

	AI script state management

***********************************************************************/

/*
=====================
idAI::LinkScriptVariables
=====================
*/
void idAI::LinkScriptVariables( void )
{
	// Call the base class first
	idActor::LinkScriptVariables();

	AI_TALK.LinkTo(				scriptObject, "AI_TALK" );
	AI_DAMAGE.LinkTo(			scriptObject, "AI_DAMAGE" );
	AI_PAIN.LinkTo(				scriptObject, "AI_PAIN" );
	AI_SPECIAL_DAMAGE.LinkTo(	scriptObject, "AI_SPECIAL_DAMAGE" );
	AI_KNOCKEDOUT.LinkTo(		scriptObject, "AI_KNOCKEDOUT" );
	AI_ENEMY_VISIBLE.LinkTo(	scriptObject, "AI_ENEMY_VISIBLE" );
	AI_ENEMY_IN_FOV.LinkTo(		scriptObject, "AI_ENEMY_IN_FOV" );
	AI_ENEMY_TACTILE.LinkTo(	scriptObject, "AI_ENEMY_TACTILE" );
	AI_MOVE_DONE.LinkTo(		scriptObject, "AI_MOVE_DONE" );
	AI_ONGROUND.LinkTo(			scriptObject, "AI_ONGROUND" );
	AI_ACTIVATED.LinkTo(		scriptObject, "AI_ACTIVATED" );
	AI_FORWARD.LinkTo(			scriptObject, "AI_FORWARD" );
	AI_JUMP.LinkTo(				scriptObject, "AI_JUMP" );
	AI_BLOCKED.LinkTo(			scriptObject, "AI_BLOCKED" );
	AI_DEST_UNREACHABLE.LinkTo( scriptObject, "AI_DEST_UNREACHABLE" );
	AI_HIT_ENEMY.LinkTo(		scriptObject, "AI_HIT_ENEMY" );
	AI_OBSTACLE_IN_PATH.LinkTo(	scriptObject, "AI_OBSTACLE_IN_PATH" );
	AI_PUSHED.LinkTo(			scriptObject, "AI_PUSHED" );

	//this is only set in a given frame
	AI_ALERTED.LinkTo(			scriptObject, "AI_ALERTED" );

	AI_AlertLevel.LinkTo(			scriptObject, "AI_AlertLevel" );
	AI_AlertIndex.LinkTo(			scriptObject, "AI_AlertIndex" );

	AI_SleepLocation.LinkTo(		scriptObject, "AI_SleepLocation" ); // grayman #3820

	//these are set until unset by the script
	AI_HEARDSOUND.LinkTo(		scriptObject, "AI_HEARDSOUND");
	AI_VISALERT.LinkTo(			scriptObject, "AI_VISALERT");
	AI_TACTALERT.LinkTo(		scriptObject, "AI_TACTALERT");

	AI_CROUCH.LinkTo(			scriptObject, "AI_CROUCH");
	AI_RUN.LinkTo(				scriptObject, "AI_RUN");
	AI_CREEP.LinkTo(			scriptObject, "AI_CREEP");

	AI_LAY_DOWN_LEFT.LinkTo(	scriptObject, "AI_LAY_DOWN_LEFT");
	AI_LAY_DOWN_FACE_DIR.LinkTo(scriptObject, "AI_LAY_DOWN_FACE_DIR");

	AI_SIT_DOWN_ANGLE.LinkTo(scriptObject, "AI_SIT_DOWN_ANGLE");
	AI_SIT_UP_ANGLE.LinkTo(scriptObject, "AI_SIT_UP_ANGLE");
}

/*
=====================
idAI::UpdateAIScript
=====================
*/
void idAI::UpdateScript()
{
	START_SCOPED_TIMING(aiScriptTimer, scopedScriptTimer);

	// greebo: This is overriding idActor::UpdateScript(), where all the state change stuff 
	// is executed, which is not needed for TDM AI

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		scriptThread->EnableDebugInfo();
	} else {
		scriptThread->DisableDebugInfo();
	}

	// don't call script until it's done waiting
	if (!scriptThread->IsWaiting()) {
		scriptThread->Execute();
	}
    
	// clear the hit enemy flag so we catch the next time we hit someone
	AI_HIT_ENEMY = false;

	if ( allowHiddenMovement || !IsHidden() ) {
		// update the animstate if we're not hidden
		if (!cv_ai_opt_noanims.GetBool())
		{
			UpdateAnimState();
		}
	}
}

/***********************************************************************

	navigation

***********************************************************************/

/*
============
idAI::KickObstacles
============
*/
void idAI::KickObstacles( const idVec3 &dir, float force, idEntity *alwaysKick ) {
	int i, numListedClipModels;
	idBounds clipBounds;
	idEntity *obEnt;
	idClipModel *clipModel;
	int clipmask;
	idVec3 org;
	idVec3 forceVec;
	idVec3 delta;
	idVec2 perpendicular;

	org = physicsObj.GetOrigin();

	// find all possible obstacles
	clipBounds = physicsObj.GetAbsBounds();
	clipBounds.TranslateSelf( dir * (clipBounds[1].x - clipBounds[0].x + clipBounds[1].y - clipBounds[0].y)/2); // grayman #2667
//	clipBounds.TranslateSelf( dir * 32.0f ); // grayman #2667 - old way assumed a humanoid
	clipBounds.ExpandSelf( 8.0f );
	clipBounds.AddPoint( org );
	clipmask = physicsObj.GetClipMask();
	idClip_ClipModelList clipModelList;
	numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, clipmask, clipModelList );
	for ( i = 0; i < numListedClipModels; i++ ) {
		clipModel = clipModelList[i];
		obEnt = clipModel->GetEntity();
		if ( obEnt == alwaysKick ) {
			// we'll kick this one outside the loop
			continue;
		}

		if ( !clipModel->IsTraceModel() ) {
			continue;
		}

		if ( obEnt->IsType( idMoveable::Type ) && obEnt->GetPhysics()->IsPushable() )
		{
			delta = obEnt->GetPhysics()->GetOrigin() - org;
			delta.NormalizeFast();
			perpendicular.x = -delta.y;
			perpendicular.y = delta.x;
			delta.z += 0.5f;
			delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
			forceVec = delta * force; // grayman #2568 - remove obEnt->mass from the equation
//			forceVec = delta * force * obEnt->GetPhysics()->GetMass(); // grayman #2568 - old way
			obEnt->ApplyImpulse( this, 0, obEnt->GetPhysics()->GetOrigin(), forceVec );
			if (obEnt->m_SetInMotionByActor.GetEntity() == NULL)
			{
				obEnt->m_SetInMotionByActor = this;
				obEnt->m_MovedByActor = this;
			}
		}
	}

	if ( alwaysKick )
	{
		delta = alwaysKick->GetPhysics()->GetOrigin() - org;
		delta.NormalizeFast();
		perpendicular.x = -delta.y;
		perpendicular.y = delta.x;
		delta.z += 0.5f;
		delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
		forceVec = delta * force; // grayman #2568 - remove obEnt->mass from the equation
//		forceVec = delta * force * alwaysKick->GetPhysics()->GetMass(); // grayman #2568 - old way
		alwaysKick->ApplyImpulse( this, 0, alwaysKick->GetPhysics()->GetOrigin(), forceVec );
		if (alwaysKick->m_SetInMotionByActor.GetEntity() == NULL)
		{
			alwaysKick->m_SetInMotionByActor = this;
			alwaysKick->m_MovedByActor = this;
		}
	}
}

bool idAI::ReEvaluateArea(int areaNum)
{
	// Remember the time
	lastAreaReevaluationTime = gameLocal.time;

	// Let's see if we have a valid door info structure in our memory
	ai::DoorInfoPtr doorInfo = GetMemory().GetDoorInfo(areaNum);

	if (doorInfo != NULL)
	{
		//if (doorInfo->lastTimeTriedToOpen + doorRetryTime < gameLocal.time)
		{
			// Re-try the door after some time
			gameLocal.m_AreaManager.RemoveForbiddenArea(areaNum, this);
			return true;
		}
	}

	return false;
}

/*
============
ValidForBounds
============
*/
bool ValidForBounds( const idAASSettings *settings, const idBounds &bounds ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( bounds[0][i] < settings->boundingBoxes[0][0][i] ) {
			return false;
		}
		if ( bounds[1][i] > settings->boundingBoxes[0][1][i] ) {
			return false;
		}
	}
	return true;
}

/*
=====================
idAI::SetAAS
=====================
*/
void idAI::SetAAS( void ) {
	idStr use_aas;

	spawnArgs.GetString( "use_aas", NULL, use_aas );
	aas = gameLocal.GetAAS( use_aas );
	if ( aas ) {
		const idAASSettings *settings = aas->GetSettings();
		if ( settings ) {
			if ( !ValidForBounds( settings, physicsObj.GetBounds() ) ) {
				gameLocal.Error( "%s cannot use use_aas %s\n", name.c_str(), use_aas.c_str() );
			}
			float height = settings->maxStepHeight;
			physicsObj.SetMaxStepHeight( height );
			return;
		} else {
			aas = NULL;
		}
	}
	gameLocal.Printf( "WARNING: %s has no AAS file\n", name.c_str() );
}

/*
=====================
idAI::DrawRoute
=====================
*/
void idAI::DrawRoute( void ) const {
	if ( aas && move.toAreaNum && move.moveCommand != MOVE_NONE && move.moveCommand != MOVE_WANDER && move.moveCommand != MOVE_FACE_ENEMY && move.moveCommand != MOVE_FACE_ENTITY && move.moveCommand != MOVE_TO_POSITION_DIRECT && move.moveCommand != MOVE_VECTOR ) 
	{
		if ( move.moveType == MOVETYPE_FLY ) {
			aas->ShowFlyPath( physicsObj.GetOrigin(), move.toAreaNum, move.moveDest, NULL ); // grayman #4412
		} else {
			aas->ShowWalkPath( physicsObj.GetOrigin(), move.toAreaNum, move.moveDest );
		}
	}
}

/*
=====================
idAI::ReachedPos
=====================
*/
bool idAI::ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand) const {
	if ( move.moveType == MOVETYPE_SLIDE ) {
		idBounds bnds( idVec3( -4, -4.0f, -8.0f ), idVec3( 4.0f, 4.0f, 64.0f ) );
		bnds.TranslateSelf( physicsObj.GetOrigin() );
		if ( bnds.ContainsPoint( pos ) ) {
			return true;
		}
	} else {
		if ( ( moveCommand == MOVE_TO_ENEMY ) || ( moveCommand == MOVE_TO_ENTITY ) )
		{
			if ( physicsObj.GetAbsBounds().IntersectsBounds( idBounds( pos ).Expand( 8.0f ) ) ) {
				return true;
			}
		}
		else
		{
			// Use the AI bounding box check to see if we've reached the position
			return ReachedPosAABBCheck(pos);
		}
	}
	return false;
}

bool idAI::ReachedPosAABBCheck(const idVec3& pos) const
{
	// angua: use AI bounds for checking
	idBounds bnds(physicsObj.GetBounds());
	if (move.accuracy >= 0)
	{
		// if accuracy is set, replace x and y size of the bounds
		bnds[0][0] = -move.accuracy;
		bnds[0][1] = -move.accuracy;
		bnds[1][0] = move.accuracy;
		bnds[1][1] = move.accuracy;
	}

	bnds.TranslateSelf(physicsObj.GetOrigin());

	bnds.ExpandSelf(reachedpos_bbox_expansion);

	// angua: expand the bounds a bit downwards, so that they can actually reach target positions 
	// that are reported as reachable by PathToGoal.
	bnds[0].z -= aas_reachability_z_tolerance;
	bnds[1].z += 0.4*aas_reachability_z_tolerance; // grayman #2717 - don't look so far up

	return (bnds.ContainsPoint(pos));
}

/*
=====================
idAI::PointReachableAreaNum
=====================
*/
int idAI::PointReachableAreaNum( const idVec3 &pos, const float boundsScale, const idVec3& offset) const
{
	START_SCOPED_TIMING(aiPointReachableAreaNumTimer, scopedPointReachableAreaNumTimer);

	if (aas == NULL) {
		return 0; // no AAS, no area number
	}

	idVec3 size = aas->GetSettings()->boundingBoxes[0][1] * boundsScale;

	// Construct the modifed bounds, based on the parameters (max.z = 32)

	// grayman #3331 - because the default boundsScale is 2.0, this causes min.z
	// to be set to -136 for humanoids. This causes points to be pushed downward
	// to aas areas on the floor below, causing AI to try to walk to where they
	// can't walk to, or shouldn't be walking to. Let's try setting min.z = -32 and see what happens.
	idBounds bounds(idVec3(-size.x, -size.y, -32.0f), idVec3(size.x, size.y, 32.0f));
//	idBounds bounds(-size, idVec3(size.x, size.y, 32.0f));

	idVec3 newPos = pos + offset;

/*	idBounds temp(bounds);
	temp.TranslateSelf(newPos);
	gameRenderWorld->DebugBox(colorGreen, idBox(temp),USERCMD_MSEC);*/

	int areaNum = 0;

	if (move.moveType == MOVETYPE_FLY)
	{
		// Flying monsters
		areaNum = aas->PointReachableAreaNum( newPos, bounds, AREA_REACHABLE_WALK | AREA_REACHABLE_FLY );
	} 
	else
	{
		// Non-flying monsters
		areaNum = aas->PointReachableAreaNum( newPos, bounds, AREA_REACHABLE_WALK );
	}

	return areaNum;
}

/*
=====================
idAI::PathToGoal
=====================
*/
bool idAI::PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, idActor* actor ) const
{
	if ( !aas )
	{
		return false;
	}

	START_SCOPED_TIMING(aiPathToGoalTimer, scopedPathToGoalTimer);

	idVec3 org = origin;
	aas->PushPointIntoAreaNum(areaNum, org);
	if (!areaNum)
	{
		return false;
	}

	idVec3 goal = goalOrigin;
	aas->PushPointIntoAreaNum(goalAreaNum, goal);
	if (!goalAreaNum)
	{
		return false;
	}

	// Sanity check the returned area. If the position isn't within the AI's height + aas_reachability_z_tolerance/2
	// reach, then report it as unreachable.
	const idVec3& grav = physicsObj.GetGravityNormal();

	float height = fabs((goal - aas->AreaCenter(goalAreaNum)) * grav);

	idBounds bounds = GetPhysics()->GetBounds();

	// angua: don't do this check when flying
	if ( (GetMoveType() != MOVETYPE_FLY) && (height > (bounds[1][2] + reachedpos_bbox_expansion + 0.4*aas_reachability_z_tolerance) ) ) // grayman #2717 - don't look so far up, and add reachedpos_bbox_expansion
	{
		goalAreaNum = 0;
		return false;
	}
	
	bool returnval;
	gameLocal.m_AreaManager.DisableForbiddenAreas(this);
	if ( move.moveType == MOVETYPE_FLY )
	{
		returnval = aas->FlyPathToGoal( path, areaNum, org, goalAreaNum, goal, travelFlags, actor ); // grayman #4412

		// grayman #4412 - The elemental walks up steps like a human AI: it travels forward
		// until it hits a step, then goes up and forward, up and forward, etc. until it hits
		// the top of the steps. To smooth out that climb, let's see if the distance from
		// path.moveGoal to the floor beneath it is less than 24. If so, let's raise it so it's
		// 24 off the floor. The exception to this is if path.moveGoal is a path_corner, because
		// we want the elemental to honor a path corner's height off the floor.
		idPathCorner *currentPath = GetMemory().currentPath.GetEntity();
		if ( !currentPath || // is there a path node?
			 (idStr::Icmp(currentPath->spawnArgs.GetString("classname"), "path_corner") != 0 ) || // is it a true path_corner?
			 ((currentPath->GetPhysics()->GetOrigin() - path.moveGoal).LengthFast() > VECTOR_EPSILON )) // are we headed for a path_corner?
		{
			// not headed for a true path_corner
			trace_t trace;
			idVec3 end = path.moveGoal;
			end.z -= 24.0f;
			gameLocal.clip.TracePoint(trace, path.moveGoal, end, MASK_OPAQUE, this);
			if ( trace.fraction < 1.0f )
			{
				path.moveGoal.z = trace.endpos.z + 24.0f;
			}
		}
	}
	else
	{
		int travelTime; // grayman #3548
		returnval = aas->WalkPathToGoal(path, areaNum, org, goalAreaNum, goal, travelFlags, travelTime, actor); // grayman #3548
	}
	gameLocal.m_AreaManager.EnableForbiddenAreas(this);

	// return returnval;
	// grayman #2708 - if returnval is true, return, but if false, check whether the AAS area is above
	// the AI's origin, as it might be if the AI is stuck in an AAS area next to a monster-clipped
	// table top, where it will be at the plane of the table top. If that's the case, return 'true'
	// to let the AI escape this type of AAS area. Normal pathfinding will never let him out.

	if (returnval)
	{
		return true;
	}

	idVec3 myOrigin = GetPhysics()->GetOrigin();
	idBounds areaBounds = aas->GetAreaBounds(areaNum);
	path.moveGoal = goal;
	return (myOrigin.z < areaBounds[0].z);
}


/*
=====================
idAI::TravelDistance

Returns the approximate travel distance from one position to the goal, or if no AAS, the straight line distance.

This is freakin' slow, so it's not good to do it too many times per frame.  It also is slower the farther you
are from the goal, so try to break the goals up into shorter distances.
=====================
*/
float idAI::TravelDistance( const idVec3 &start, const idVec3 &end )
{
	int			fromArea;
	int			toArea;
	float		dist;
	idVec2		delta;

	if ( !aas )
	{
		// no aas, so just take the straight line distance
		delta = end.ToVec2() - start.ToVec2();
		dist = delta.LengthFast();

		if ( ai_debugMove.GetBool() )
		{
			gameRenderWorld->DebugLine( colorBlue, start, end, USERCMD_MSEC, false );
			gameRenderWorld->DebugText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}

		return dist;
	}

	fromArea = PointReachableAreaNum( start );
	toArea = PointReachableAreaNum( end );

	if ( !fromArea || !toArea )
	{
		// can't seem to get there
		return -1;
	}

	if ( fromArea == toArea )
	{
		// same area, so just take the straight line distance
		delta = end.ToVec2() - start.ToVec2();
		dist = delta.LengthFast();

		if ( ai_debugMove.GetBool() )
		{
			gameRenderWorld->DebugLine( colorBlue, start, end, USERCMD_MSEC, false );
			gameRenderWorld->DebugText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}

		return dist;
	}

	idReachability *reach;
	int travelTime;
	if ( !aas->RouteToGoalArea( fromArea, start, toArea, travelFlags, travelTime, &reach, NULL, this ) )
	{
		return -1;
	}

	if ( ai_debugMove.GetBool() )
	{
		if ( move.moveType == MOVETYPE_FLY )
		{
			aas->ShowFlyPath( start, toArea, end, this ); // grayman #4412
		}
		else
		{
			aas->ShowWalkPath( start, toArea, end );
		}
	}

	return travelTime;
}

void idAI::SetStartTime(idVec3 pos)
{
	// grayman #3993 - if the new destination is close to the current
	// destination, don't reset startTime

	if ( (move.moveDest - pos).LengthFast() > 1.0f )
	{
		move.startTime = gameLocal.time;
	}
}

/*
=====================
idAI::StopMove
=====================
*/
void idAI::StopMove( moveStatus_t status )
{
	// grayman #4238 - are we stopping while clipping into a large AI?

//--------------------------------
//	if ( gameLocal.framenum > 30 )
//	{
		if ( physicsObj.GetNumClipModels() )
		{
			idEntity	*hit;
			idClipModel *cm;

			idClip_ClipModelList clipModels;
			int num = gameLocal.clip.ClipModelsTouchingBounds(physicsObj.GetAbsBounds(), physicsObj.GetClipMask(), clipModels);
			for ( int i = 0; i < num; i++ )
			{
				cm = clipModels[i];
				hit = cm->GetEntity();
				if ( hit && (hit != this) )
				{
					if ( hit->IsType(idAI::Type) ) // hit is an AI?
					{
						if ( hit->GetPhysics()->GetMass() > SMALL_AI_MASS ) // hit is not a small AI?
						{
							idAI *hitAI = static_cast<idAI*>(hit);
							if ( !hitAI->AI_FORWARD ) // hit is standing still?
							{
								hitAI->PushWithAF();
								return;
							}
						}
					}
				}
			}
		}
//	}
//----------------------------------

	// Note: you might be tempted to set AI_RUN to false here,
	// but AI_RUN needs to persist when stopping at a point
	// where the AI is just going to start moving again.
	AI_MOVE_DONE		= true;
	AI_FORWARD			= false;
	m_pathRank			= 1000; // grayman #2345
	move.moveCommand	= MOVE_NONE;
	move.moveStatus		= status;
	move.toAreaNum		= 0;
	move.goalEntity		= NULL;
	SetStartTime(physicsObj.GetOrigin()); // grayman #3993
	move.moveDest = physicsObj.GetOrigin();
	AI_DEST_UNREACHABLE	= (status == MOVE_STATUS_DEST_UNREACHABLE);
	AI_OBSTACLE_IN_PATH = false;
	AI_BLOCKED			= false;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.duration		= 0;
	move.range			= 0.0f;
	move.speed			= 0.0f;
	move.anim			= 0;
	move.moveDir.Zero();
	move.lastMoveOrigin.Zero();
	move.lastMoveTime	= gameLocal.time;
	move.accuracy		= -1;
}

const idVec3& idAI::GetMoveDest() const
{
	return move.moveDest;
}

idEntity* idAI::GetTactileEntity(void) // grayman #2345
{
	return m_tactileEntity;
}

/*
=====================
idAI::FaceEnemy

Continually face the enemy's last known position.  MoveDone is always true in this case.
=====================
*/
bool idAI::FaceEnemy( void )
{
	idActor *enemyEnt = enemy.GetEntity();
	if ( !enemyEnt )
	{
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	TurnToward( lastVisibleEnemyPos );
	move.goalEntity		= enemyEnt;
	SetStartTime(physicsObj.GetOrigin()); // grayman #3993
	move.moveDest		= physicsObj.GetOrigin();
	move.moveCommand	= MOVE_FACE_ENEMY;
	move.moveStatus		= MOVE_STATUS_WAITING;
	// move.startTime		= gameLocal.time; // grayman #3993
	move.speed			= 0.0f;
	move.accuracy		= -1;
	AI_MOVE_DONE		= true;
	AI_FORWARD			= false;
	m_pathRank			= 1000; // grayman #2345
	AI_DEST_UNREACHABLE = false;

	return true;
}

/*
=====================
idAI::FaceEntity

Continually face the entity position.  MoveDone is always true in this case.
=====================
*/
bool idAI::FaceEntity( idEntity *ent )
{
	if ( !ent )
	{
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	idVec3 entityOrg = ent->GetPhysics()->GetOrigin();
	TurnToward( entityOrg );
	move.goalEntity		= ent;
	SetStartTime(physicsObj.GetOrigin()); // grayman #3993
	move.moveDest		= physicsObj.GetOrigin();
	move.moveCommand	= MOVE_FACE_ENTITY;
	move.moveStatus		= MOVE_STATUS_WAITING;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.speed			= 0.0f;
	AI_MOVE_DONE		= true;
	AI_FORWARD			= false;
	m_pathRank			= 1000; // grayman #2345
	AI_DEST_UNREACHABLE = false;

	return true;
}

/*
=====================
idAI::DirectMoveToPosition
=====================
*/
bool idAI::DirectMoveToPosition( const idVec3 &pos ) {
	if ( ReachedPos( pos, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	SetStartTime(pos); // grayman #3993
	move.moveDest		= pos;
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_TO_POSITION_DIRECT;
	move.moveStatus		= MOVE_STATUS_MOVING;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.speed			= fly_speed;
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	if ( move.moveType == MOVETYPE_FLY ) {
		idVec3 dir = pos - physicsObj.GetOrigin();
		dir.Normalize();
		dir *= fly_speed;
		physicsObj.SetLinearVelocity( dir );
	}

	return true;
}

/*
=====================
idAI::MoveToEnemyHeight
=====================
*/
bool idAI::MoveToEnemyHeight( void ) {
	idActor	*enemyEnt = enemy.GetEntity();

	if ( !enemyEnt || ( move.moveType != MOVETYPE_FLY ) ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	 // grayman #3993
	idVec3 pos = move.moveDest;
	pos.z = lastVisibleEnemyPos.z + enemyEnt->EyeOffset().z + fly_offset;
	SetStartTime(pos);
	move.moveDest.z = pos.z;
	//move.moveDest.z = lastVisibleEnemyPos.z + enemyEnt->EyeOffset().z + fly_offset;

	move.goalEntity		= enemyEnt;
	move.moveCommand	= MOVE_TO_ENEMYHEIGHT;
	move.moveStatus		= MOVE_STATUS_MOVING;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.speed			= 0.0f;
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= false;
	m_pathRank			= 1000; // grayman #2345

	return true;
}

/*
=====================
idAI::MoveToEnemy
=====================
*/
bool idAI::MoveToEnemy( void ) {
	int			areaNum;
	aasPath_t	path;
	idActor		*enemyEnt = enemy.GetEntity();

	if ( !enemyEnt ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	if ( ReachedPos( lastVisibleReachableEnemyPos, MOVE_TO_ENEMY ) ) {
		if ( !ReachedPos( lastVisibleEnemyPos, MOVE_TO_ENEMY ) || !AI_ENEMY_VISIBLE ) {
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
			AI_DEST_UNREACHABLE = true;
			return false;
		}
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	idVec3 pos = lastVisibleReachableEnemyPos;

	move.toAreaNum = 0;
	if ( aas ) {
		move.toAreaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( move.toAreaNum, pos );

		areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, pos, this ) ) {
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( !move.toAreaNum ) {
		// if only trying to update the enemy position
		if ( move.moveCommand == MOVE_TO_ENEMY ) {
			if ( !aas ) {
				// keep the move destination up to date for wandering
				move.moveDest = pos;
			}
			return false;
		}

		if ( !NewWanderDir( pos ) ) {
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( move.moveCommand != MOVE_TO_ENEMY ) {
		move.moveCommand	= MOVE_TO_ENEMY;
		SetStartTime(pos); // grayman #3993
		//move.startTime		= gameLocal.time; // grayman #3993
	}

	move.moveDest		= pos;
	move.goalEntity		= enemyEnt;
	move.speed			= fly_speed;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	return true;
}

/*
=====================
idAI::MoveToEntity
=====================
*/
bool idAI::MoveToEntity( idEntity *ent ) {
	int			areaNum;
	aasPath_t	path;
	idVec3		pos;

	if ( !ent ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	pos = ent->GetPhysics()->GetOrigin();
	if ( ( move.moveType != MOVETYPE_FLY ) && ( ( move.moveCommand != MOVE_TO_ENTITY ) || ( move.goalEntityOrigin != pos ) ) ) {
		ent->GetFloorPos( 64.0f, pos );
	}

	if ( ReachedPos( pos, MOVE_TO_ENTITY ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	move.toAreaNum = 0;
	if ( aas ) {
		move.toAreaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( move.toAreaNum, pos );

		areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, pos, this ) ) {
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( !move.toAreaNum ) {
		// if only trying to update the entity position
		if ( move.moveCommand == MOVE_TO_ENTITY ) {
			if ( !aas ) {
				// keep the move destination up to date for wandering
				move.moveDest = pos;
			}
			return false;
		}

		if ( !NewWanderDir( pos ) ) {
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( ( move.moveCommand != MOVE_TO_ENTITY ) || ( move.goalEntity.GetEntity() != ent ) ) {
		SetStartTime(pos); // grayman #3993
		//move.startTime		= gameLocal.time; // grayman #3993
		move.goalEntity		= ent;
		move.moveCommand	= MOVE_TO_ENTITY;
	}

	move.moveDest			= pos;
	move.goalEntityOrigin	= ent->GetPhysics()->GetOrigin();
	move.moveStatus			= MOVE_STATUS_MOVING;
	move.speed				= fly_speed;
	move.accuracy			= -1;
	AI_MOVE_DONE			= false;
	AI_DEST_UNREACHABLE		= false;
	AI_FORWARD				= true;
	m_pathRank				= rank; // grayman #2345

	return true;
}

CMultiStateMover* idAI::OnElevator(bool mustBeMoving) const
{
	idEntity* ent = physicsObj.GetGroundEntity(); // grayman #3050 - this MIGHT return a false NULL

	if ( ent == NULL )
	{
		// grayman #3050 - when an elevator is moving down, out from under
		// the feet of this AI, it MIGHT be the case that tracing down 0.25,
		// the normal criteria for finding the ground, isn't enough.
		// So let's try to search a bit farther to see if we're on an elevator.

		trace_t t;
		idVec3 down;
		idVec3 gravityNormal = physicsObj.GetGravityNormal();

		if ( gravityNormal == vec3_zero )
		{
			return NULL;
		}

		idVec3 org = physicsObj.GetOrigin();
		idClipModel* clipModel = physicsObj.GetClipModel();
		down = org + gravityNormal * 4;
		gameLocal.clip.Translation( t, org, down, clipModel, clipModel->GetAxis(), physicsObj.GetClipMask(), this );

		if ( t.fraction == 1.0f )
		{
			return NULL;
		}

		ent = gameLocal.entities[t.c.entityNum];
	}

	// Return false if ground entity is not a mover
	if ( ( ent == NULL ) || !ent->IsType(CMultiStateMover::Type) )
	{
		return NULL;
	}

	CMultiStateMover* mover = static_cast<CMultiStateMover*>(ent);
	if (mustBeMoving)
	{
		return (!mover->IsAtRest()) ? mover : NULL;
	}
	return mover;
}

bool idAI::Flee(idEntity* entityToFleeFrom, bool fleeingEvent, int algorithm, int distanceOption) // grayman #3317
{
	EscapePointAlgorithm algorithmType = static_cast<EscapePointAlgorithm>(algorithm);
	
	if ( !aas || ( !entityToFleeFrom && !fleeingEvent ) ) // grayman #3317
	{
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	// The current AI origin
	idVec3 org = physicsObj.GetOrigin();

	// These two will hold the travel destination info
	idVec3 moveDest;
	int moveAreaNum(-1);

	if (algorithmType != FIND_AAS_AREA_FAR_FROM_THREAT)
	{
		// Use the EscapePointManager to locate a pathFlee entity

		// Setup the escape conditions
		EscapeConditions conditions;
		
		conditions.fromEntity = entityToFleeFrom;

		conditions.threatPosition = fleeingFrom; // grayman #3847
		/* grayman #3317 - need threat position if fleeing an event
		if ( fleeingEvent )
		{
			conditions.threatPosition = GetMemory().posEvidenceIntruders;
		}
		else
		{
			conditions.threatPosition.Zero();
		}
		*/

		conditions.aas = aas;
		conditions.fromPosition = org;
		conditions.self = this;
		conditions.distanceOption = static_cast<EscapeDistanceOption>(distanceOption);
		conditions.algorithm = algorithmType;
		conditions.minDistanceToThreat = 0.0f;

		// Request the escape goal from the manager
		EscapeGoal goal = gameLocal.m_EscapePointManager->GetEscapeGoal(conditions);

		if (goal.escapePointId == -1)
		{
			// Invalid escape point id returned
			return false;
		}

		// Get the actual point (this should never be NULL)
		EscapePoint* targetPoint = gameLocal.m_EscapePointManager->GetEscapePoint(goal.escapePointId);

		moveDest = targetPoint->origin;
		moveAreaNum = targetPoint->areaNum;
	}
	else 
	{
		// algorithm == FIND_AAS_AREA_FAR_FROM_THREAT

		int	areaNum = PointReachableAreaNum(org);
		idVec3 pos;

		// grayman #3548 - Considering the player as an obstacle can cause the AI to get
		// stuck in place if the player bumped him and was standing near him.
		// FindNearestGoal() won't let the flee path intersect the player's expanded boundary,
		// so many checks were failing because the candidate flee path began inside the expanded boundary.
		//aasObstacle_t obstacle;
		//int numObstacles;

		if ( entityToFleeFrom != NULL ) // grayman #3317
		{
			// consider the entity the monster is getting close to as an obstacle
			//obstacle.absBounds = entityToFleeFrom->GetPhysics()->GetAbsBounds(); // grayman #3548

			if ( entityToFleeFrom == enemy.GetEntity() )
			{
				pos = lastVisibleEnemyPos;
			}
			else
			{
				pos = entityToFleeFrom->GetPhysics()->GetOrigin();
			}

			//numObstacles = 1; // grayman #3548
		}
		else // fleeing from an event (i.e. murder)
		{
			pos = fleeingFrom; // grayman #3847
			//pos = GetMemory().posEvidenceIntruders;
			//numObstacles = 0; // grayman #3548
		}

		/* grayman #4229 - abandon this section, since I can't get it
		// to work correctly

		// grayman #3548 - are any elevators nearby? If so, the fleeing AI
		// would probably use it to escape. Find the elevator, pick the
		// elevator station that's farthest from the event, and pretend both
		// the event and the AI are standing at that station. FindNearestGoal(),
		// which only deals with walking reachabilities, should then look for
		// a goal that's on the floor of that station.

		if (canUseElevators)
		{
			eas::tdmEAS* elevatorSystem = GetAAS()->GetEAS();
			CMultiStateMover* elevator = elevatorSystem->GetNearbyElevator(org, 500, 128);

			if (elevator)
			{
				// which elevator station is the farthest from 'pos'?
				// go to that station, and use its origin as a replacement for 'pos' and 'org'
				idVec3 bestElevatorPos;
				float bestElevatorPosDist = 0;
				const idList<MoverPositionInfo>& positionList = elevator->GetPositionInfoList();

				for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
				{
					CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();
		
					if (positionEnt)
					{
						float thisDist = (pos - positionEnt->GetPhysics()->GetOrigin()).LengthFast();
						if (thisDist > bestElevatorPosDist)
						{
							bestElevatorPos = positionEnt->GetPhysics()->GetOrigin();
							bestElevatorPosDist = thisDist;
							int stationIndex = elevatorSystem->GetElevatorStationIndex(positionEnt);
							eas::ElevatorStationInfoPtr stationInfo = elevatorSystem->GetElevatorStationInfo(stationIndex);
							areaNum = stationInfo->areaNum;
						}
					}
				}

				// pretend the AI and the position he's fleeing from
				// are in the same location, at an elevator station
				org = pos = bestElevatorPos;
			}
		}*/

		// Set up the evaluator class
		tdmAASFindEscape findEscapeArea(pos, org, distanceOption, 100, team); // grayman #3548
		aasGoal_t dummy;
		aas->FindNearestGoal(dummy, areaNum, org, pos, travelFlags, NULL, 0, findEscapeArea); // grayman #3317 // grayman #3548
		//aas->FindNearestGoal(dummy, areaNum, org, pos, travelFlags, &obstacle, numObstacles, findEscapeArea); // grayman #3317

		aasGoal_t& goal = findEscapeArea.GetEscapeGoal();

		if (goal.areaNum == -1)
		{
			// Invalid escape point id returned
			return false;
		}

		moveDest = goal.origin;
		moveAreaNum = goal.areaNum;
	}

	StopMove(MOVE_STATUS_DONE);
	MoveToPosition(moveDest,50); // grayman #3357 - stop when you get w/in 50 of goal
	return true;
}

bool idAI::IsAfraid() // grayman #3848
{
	// unarmed?
	if ( ( GetNumMeleeWeapons() == 0 ) && ( GetNumRangedWeapons() == 0 ) )
	{
		return true;
	}

	// non-fighting civilian?
	if ( spawnArgs.GetBool("is_civilian", "0") )
	{
		return true;
	}

	// low health?
	if ( health < spawnArgs.GetInt("health_critical", "0") )
	{
		return true;
	}

	return false;
}


/*
=============================
idAI::FindAttackPosition
=============================
*/

// grayman #3507 - This checks for a reachable position within combat range

bool idAI::FindAttackPosition(int pass, idActor* enemy, idVec3& targetPoint, ECombatType type)
{
	if (enemy == NULL)
	{
		return false;
	}

	int areaNum = PointReachableAreaNum(GetPhysics()->GetOrigin(), 1.0f);

	if ((type == COMBAT_MELEE) || ((type == COMBAT_NONE) && (GetNumMeleeWeapons() > 0) ) )
	{
		idVec3 attackPos = enemy->GetPhysics()->GetOrigin();
		float offset = melee_range + 10;
		switch (pass)
		{
		case 0:
			attackPos.x += offset;
			break;
		case 1:
			attackPos.y += offset;
			break;
		case 2:
			attackPos.x -= offset;
			break;
		case 3:
			attackPos.y -= offset;
			break;
		}

		idVec3 bottomPoint = attackPos;
		bottomPoint.z -= 70;
	
		trace_t result;
		if (gameLocal.clip.TracePoint(result, attackPos, bottomPoint, MASK_OPAQUE, NULL))
		{
			attackPos.z = result.endpos.z + 1;
			int targetAreaNum = PointReachableAreaNum(attackPos, 1.0f);
			aasPath_t path;

			if (PathToGoal(path, areaNum, GetPhysics()->GetOrigin(), targetAreaNum, attackPos, this))
			{
				targetPoint = attackPos;
				return true;
			}
		}
	}

	if ( (type == COMBAT_RANGED) || ( (type == COMBAT_NONE) && ( GetNumRangedWeapons() > 0 ) ) )
	{
		aasGoal_t goal = GetPositionWithinRange(enemy->GetEyePosition());
		if (goal.areaNum != -1)
		{
			// Found a suitable attack position, can we get there?
			aasPath_t path;
			if (PathToGoal(path, areaNum, GetPhysics()->GetOrigin(), goal.areaNum, goal.origin, this))
			{
				targetPoint = goal.origin;
				return true;
			}
		}
	}

	return false;
}

/*
=====================
idAI::GetPositionWithinRange
=====================
*/
aasGoal_t idAI::GetPositionWithinRange(const idVec3& targetPos)
{
	idVec3 org = physicsObj.GetOrigin();
	idVec3 eye = GetEyePosition();
	float fireRange = spawnArgs.GetFloat("fire_range", "0");

	PositionWithinRangeFinder findGoal(this, physicsObj.GetGravityAxis(), targetPos, eye - org, fireRange);

	aasGoal_t goal;
	int areaNum	= PointReachableAreaNum(org);

	aas->FindNearestGoal(goal, areaNum, org, targetPos, travelFlags, NULL, 0, findGoal);
	
	goal.areaNum = -1;

	float bestDistance;
	findGoal.GetBestGoalResult(bestDistance, goal);
	return goal;
}




/*
=====================
idAI::GetObservationPosition
=====================
*/
idVec3 idAI::GetObservationPosition (const idVec3& pointToObserve, const float visualAcuityZeroToOne, const unsigned short maxCost) // grayman #4347
{
	int				areaNum;
	aasGoal_t		goal;
	idVec3			observeFromPos;
	aasPath_t	path;

	if ( !aas ) 
	{	
		observeFromPos = GetPhysics()->GetOrigin();
		AI_DEST_UNREACHABLE = true;
		return observeFromPos;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );

	// Raise point up just a bit so it isn't on the floor of the aas
	idVec3 pointToObserve2 = pointToObserve;
	pointToObserve2.z += 45.0;

	// What is the lighting along the line where the thing to be observed
	// might be.
	float maxDistanceToObserve = GetMaximumObservationDistanceForPoints(pointToObserve, pointToObserve2);
		
	idAASFindObservationPosition findGoal
	(
		this, 
		physicsObj.GetGravityAxis(), 
		pointToObserve2, 
		GetEyePosition() - org,  // Offset of eye from origin
		maxDistanceToObserve // Maximum distance from which we can observe
	);

	if (!aas->FindNearestGoal
	(
		goal, 
		areaNum, 
		org,
		pointToObserve2, // It is also the goal target
		travelFlags, 
		NULL, 
		0, 
		findGoal,
		maxCost
	) ) 
	{
		float bestDistance;

		// See if we can get to the point itself since noplace was good enough
		// for just looking from a distance due to lighting/occlusion/reachability.
		if (PathToGoal( path, areaNum, physicsObj.GetOrigin(), areaNum, org, this ) ) 
		{

			// Can reach the point itself, so walk right up to it
			observeFromPos = pointToObserve; 

			// Draw the AI Debug Graphics
			if (cv_ai_search_show.GetInteger() >= 1.0)
			{
				idVec4 markerColor (0.0, 1.0, 1.0, 1.0);
				idVec3 arrowLength (0.0, 0.0, 50.0);

				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFromPos + arrowLength,
					observeFromPos,
					2,
					cv_ai_search_show.GetInteger()
				);
			}

		}

		else if (findGoal.getBestGoalResult
		(
			bestDistance,
			goal
		))
		{

			// Use closest reachable observation point that we found
			observeFromPos = goal.origin;

			// Draw the AI Debug Graphics
			if (cv_ai_search_show.GetInteger() >= 1.0)
			{
				idVec4 markerColor (1.0, 1.0, 0.0, 1.0);
				idVec3 arrowLength (0.0, 0.0, 50.0);

				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFromPos,
					pointToObserve,
					2,
					cv_ai_search_show.GetInteger()
				);
			}
		}

		else
		{
			// No choice but to try to walk up to it as much as we can
			observeFromPos = pointToObserve; 

			// Draw the AI Debug Graphics
			if (cv_ai_search_show.GetInteger() >= 1.0)
			{
				idVec4 markerColor (1.0, 0.0, 0.0, 1.0);
				idVec3 arrowLength (0.0, 0.0, 50.0);

				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFromPos + arrowLength,
					observeFromPos,
					2,
					cv_ai_search_show.GetInteger()
				);
			}
		}

		return observeFromPos;
	}
	else
	{
		observeFromPos = goal.origin;
		AI_DEST_UNREACHABLE = false;

		// Draw the AI Debug Graphics
		if (cv_ai_search_show.GetInteger() >= 1.0)
		{
			idVec4 markerColor (0.0, 1.0, 0.0, 1.0);
			idVec3 arrowLength (0.0, 0.0, 50.0);

			gameRenderWorld->DebugArrow
			(
				markerColor,
				observeFromPos,
				pointToObserve,
				2,
				cv_ai_search_show.GetInteger()
			);
		}

		return observeFromPos;
	}
}

/*
=====================
idAI::MoveOutOfRange
=====================
*/
bool idAI::MoveOutOfRange( idEntity *ent, float range ) {
	int				areaNum;
	aasObstacle_t	obstacle;
	aasGoal_t		goal;
	idVec3			pos;

	if ( !aas || !ent ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );

	// consider the entity the monster is getting close to as an obstacle
	obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();

	if ( ent == enemy.GetEntity() ) {
		pos = lastVisibleEnemyPos;
	} else {
		pos = ent->GetPhysics()->GetOrigin();
	}

	idAASFindAreaOutOfRange findGoal( pos, range );
	if ( !aas->FindNearestGoal( goal, areaNum, org, pos, travelFlags, &obstacle, 1, findGoal ) ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Best fleeing location is: %f %f %f in area %d\r", goal.origin.x, goal.origin.y, goal.origin.z, goal.areaNum);

	// grayman #4412 - The code assumes we want a goal position with
	// the same z value as the entity we're moving away from. If I'm a
	// flying monster, I want to retain my z value.

	if ( move.moveType == MOVETYPE_FLY )
	{
		goal.origin.z = org.z;
	}

	if ( ReachedPos( goal.origin, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	SetStartTime(goal.origin); // grayman #3993
	move.moveDest		= goal.origin;
	move.toAreaNum		= goal.areaNum;
	move.goalEntity		= ent;
	move.moveCommand	= MOVE_OUT_OF_RANGE;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.range			= range;
	move.speed			= fly_speed;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	return true;
}

/*
=====================
idAI::MoveToAttackPosition
=====================
*/
bool idAI::MoveToAttackPosition( idEntity *ent, int attack_anim ) {
	int				areaNum;
	aasObstacle_t	obstacle;
	aasGoal_t		goal;
	idVec3			pos;

	if ( !aas || !ent ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );

	// consider the entity the monster is getting close to as an obstacle
	obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();

	if ( ent == enemy.GetEntity() ) {
		pos = lastVisibleEnemyPos;
	} else {
		pos = ent->GetPhysics()->GetOrigin();
	}

	// Make sure we have a active projectile or at least the def
	if (activeProjectile.info.def == NULL)
	{
		if (projectileInfo.Num() == 0)
		{
			gameLocal.Warning("AI %s has no projectile info, cannot move to attack position!", name.c_str());
			return false;
		}

		// Move the def pointer of the "next" projectile info into the active one
		activeProjectile.info = projectileInfo[curProjectileIndex];
	}

	idAASFindAttackPosition findGoal( this, physicsObj.GetGravityAxis(), ent, pos, missileLaunchOffset[ attack_anim ] );
	if ( !aas->FindNearestGoal( goal, areaNum, org, pos, travelFlags, &obstacle, 1, findGoal ) ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	SetStartTime(goal.origin); // grayman #3993
	move.moveDest		= goal.origin;
	move.toAreaNum		= goal.areaNum;
	move.goalEntity		= ent;
	move.moveCommand	= MOVE_TO_ATTACK_POSITION;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.speed			= fly_speed;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.anim			= attack_anim;
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	return true;
}

/*
=====================
idAI::MoveToPosition
=====================
*/

bool idAI::MoveToPosition( const idVec3 &pos, float accuracy )
{
	move.accuracy = accuracy; // grayman #3882

	// Clear the "blocked" flag in the movement subsystem
	movementSubsystem->SetBlockedState(ai::MovementSubsystem::ENotBlocked);

	// Check if we already reached the position
	if ( ReachedPos( pos, move.moveCommand ) )
	{
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	if (   ( GetMoveType() == MOVETYPE_SIT )
		|| ( GetMoveType() == MOVETYPE_SLEEP )
		|| ( GetMoveType() == MOVETYPE_SIT_DOWN )
		|| ( GetMoveType() == MOVETYPE_GET_UP )
		|| ( GetMoveType() == MOVETYPE_FALL_ASLEEP ) // grayman #3820 - was MOVETYPE_LAY_DOWN
		|| ( GetMoveType() == MOVETYPE_WAKE_UP ) ) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
	{
		GetUp();
		return true;
	}

	idVec3 org = pos;
	move.toAreaNum = 0;
	aasPath_t path;
	if ( aas )
	{
		move.toAreaNum = PointReachableAreaNum( org );
		aas->PushPointIntoAreaNum( move.toAreaNum, org ); // if this point is outside this area, it will be moved to one of the area's edges

		int areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );

		if (!PathToGoal(path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, org, this))
		{
			StopMove(MOVE_STATUS_DEST_UNREACHABLE);
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if (!move.toAreaNum && !NewWanderDir(org))
	{
		StopMove(MOVE_STATUS_DEST_UNREACHABLE);
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	// Valid path to goal, check if we need to use an elevator
	if (path.type == PATHTYPE_ELEVATOR)
	{
		NeedToUseElevator(path.elevatorRoute);
	}

	SetStartTime(org); // grayman #3993
	//move.startTime = gameLocal.time; // grayman #3993
	move.moveDest		= org;
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_TO_POSITION;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.speed			= fly_speed;
	move.accuracy		= accuracy;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	return true;
}

/*
=====================
idAI::LookForCover
=====================
*/
bool idAI::LookForCover(aasGoal_t& hideGoal,idEntity *hideFromEnt, const idVec3 &hideFromPos) 
{
	if (aas == NULL)
	{
		return false;
	}

	aasObstacle_t obstacle;

	const idVec3 &org = physicsObj.GetOrigin();
	int areaNum	= PointReachableAreaNum( org );

	// consider the entity the monster tries to hide from as an obstacle
	obstacle.absBounds = hideFromEnt->GetPhysics()->GetAbsBounds();

	idAASFindCover findCover( this, hideFromEnt, hideFromPos );
	return aas->FindNearestGoal( hideGoal, areaNum, org, hideFromPos, travelFlags, &obstacle, 1, findCover, 2*spawnArgs.GetInt("taking_cover_max_cost") );
}

/*
=====================
idAI::MoveToCover
=====================
*/
bool idAI::MoveToCover( idEntity *hideFromEnt, const idVec3 &hideFromPos ) {
	//common->Printf("MoveToCover called... ");

	aasGoal_t hideGoal;

	if ( !aas || !hideFromEnt ) {
		common->Printf("MoveToCover failed: null aas or entity\n");
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}
	
	if (!LookForCover(hideGoal, hideFromEnt, hideFromPos)) {
		//common->Printf("MoveToCover failed: destination unreachable\n");
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	if ( ReachedPos( hideGoal.origin, move.moveCommand ) ) {
		//common->Printf("MoveToCover succeeded: Already at hide position\n");
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	SetStartTime(hideGoal.origin); // grayman #3993
	move.moveDest		= hideGoal.origin;
	move.toAreaNum		= hideGoal.areaNum;
	move.goalEntity		= hideFromEnt;
	move.moveCommand	= MOVE_TO_COVER;
	move.moveStatus		= MOVE_STATUS_MOVING;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.speed			= fly_speed;
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	//common->Printf("MoveToCover succeeded: Now moving into cover\n");

	return true;
}

/*
=====================
idAI::SlideToPosition
=====================
*/
bool idAI::SlideToPosition( const idVec3 &pos, float time ) {
	StopMove( MOVE_STATUS_DONE );

	SetStartTime(pos); // grayman #3993
	move.moveDest		= pos;
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_SLIDE_TO_POSITION;
	move.moveStatus		= MOVE_STATUS_MOVING;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.duration		= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( time ) );
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= false;
	m_pathRank			= 1000; // grayman #2345

	if ( move.duration > 0 ) {
		move.moveDir = ( pos - physicsObj.GetOrigin() ) / MS2SEC( move.duration );
		if ( move.moveType != MOVETYPE_FLY ) {
			move.moveDir.z = 0.0f;
		}
		move.speed = move.moveDir.LengthFast();
	}

	return true;
}

/*
=====================
idAI::WanderAround
=====================
*/
bool idAI::WanderAround( void ) {
	StopMove( MOVE_STATUS_DONE );

	// grayman #3993
	idVec3 pos = physicsObj.GetOrigin() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 256.0f;
	if ( !NewWanderDir( pos ) ) {
		move.moveDest = pos;
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	SetStartTime(pos); // grayman #3993
	move.moveDest		= pos;
	move.moveCommand	= MOVE_WANDER;
	move.moveStatus		= MOVE_STATUS_MOVING;
	//move.startTime		= gameLocal.time; // grayman #3993
	move.speed			= fly_speed;
	move.accuracy		= -1;
	AI_MOVE_DONE		= false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	return true;
}

/*
=====================
idAI::MoveDone
=====================
*/
bool idAI::MoveDone( void ) const {
	return ( move.moveCommand == MOVE_NONE );
}

void idAI::SetMoveType( int moveType ) {
	if ( ( moveType < 0 ) || ( moveType >= NUM_MOVETYPES ) ) {
		gameLocal.Error( "Invalid movetype %d", moveType );
	}

	move.moveType = static_cast<moveType_t>( moveType );
	if ( move.moveType == MOVETYPE_FLY ) {
		travelFlags = TFL_WALK|TFL_AIR|TFL_FLY|TFL_DOOR;
	} else {
		travelFlags = TFL_WALK|TFL_AIR|TFL_DOOR;
	}

	// grayman #3548 - what about elevators?
	if (canUseElevators)
	{
		travelFlags |= TFL_ELEVATOR;
	}
}

void idAI::SetMoveType( idStr moveType ) 
{
	if (moveType.Icmp("MOVETYPE_DEAD") == 0)
	{
		SetMoveType(MOVETYPE_DEAD);
	}
	else if (moveType.Icmp("MOVETYPE_ANIM") == 0)
	{
		SetMoveType(MOVETYPE_ANIM);
	}
	else if (moveType.Icmp("MOVETYPE_SLIDE") == 0)
	{
		SetMoveType(MOVETYPE_SLIDE);
	}
	else if (moveType.Icmp("MOVETYPE_FLY") == 0)
	{
		SetMoveType(MOVETYPE_FLY);
	}
	else if (moveType.Icmp("MOVETYPE_STATIC") == 0)
	{
		SetMoveType(MOVETYPE_STATIC);
	}
	else if (moveType.Icmp("MOVETYPE_SIT") == 0)
	{
		SetMoveType(MOVETYPE_SIT);
	}
	else if (moveType.Icmp("MOVETYPE_SIT_DOWN") == 0)
	{
		SetMoveType(MOVETYPE_SIT_DOWN);
	}
	else if (moveType.Icmp("MOVETYPE_SLEEP") == 0)
	{
		SetMoveType(MOVETYPE_SLEEP);
	}
	else if (moveType.Icmp("MOVETYPE_FALL_ASLEEP") == 0) // grayman #3820 - was MOVETYPE_LAY_DOWN
	{
		SetMoveType(MOVETYPE_FALL_ASLEEP);
	}
	else if (moveType.Icmp("MOVETYPE_GET_UP") == 0)
	{
		SetMoveType(MOVETYPE_GET_UP);
	}
	else if (moveType.Icmp("MOVETYPE_WAKE_UP") == 0) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
	{
		SetMoveType(MOVETYPE_WAKE_UP);
	}
	else
	{
		gameLocal.Warning( "Invalid movetype %s", moveType.c_str() );
	}
}

// grayman #4039 - need a way to set move accuracy independent
// of MoveToPosition()
void idAI::SetMoveAccuracy(float accuracy)
{
	move.accuracy = accuracy;
}

/*
================
idAI::StepDirection
================
*/
bool idAI::StepDirection( float dir ) {
	predictedPath_t path;
	idVec3 org;

	move.wanderYaw = dir;
	move.moveDir = idAngles( 0, move.wanderYaw, 0 ).ToForward();

	org = physicsObj.GetOrigin();

	idAI::PredictPath( this, aas, org, move.moveDir * 48.0f, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( path.blockingEntity && ( ( move.moveCommand == MOVE_TO_ENEMY ) || ( move.moveCommand == MOVE_TO_ENTITY ) ) && ( path.blockingEntity == move.goalEntity.GetEntity() ) ) {
		// don't report being blocked if we ran into our goal entity
		return true;
	}

	// SZ: January 7, 2006: Wandering uses this, and currently it fails the test if it would bump into another AI.
	// If we are wandering, we want to make sure we can still bump into the player or enemy AIs, both of which are idActor
	// based.
	if (path.blockingEntity && (move.moveCommand == MOVE_WANDER))
	{
		// What type of entity is it?
		if
		(
			(path.blockingEntity->IsType(idActor::Type) )
		)
		{
			// Bump into enemies all you want while wandering
			if (IsEnemy(path.blockingEntity))
			{
				return true;
			}
		}

	} // End wandering case

	if ( ( move.moveType == MOVETYPE_FLY ) && ( path.endEvent == SE_BLOCKED ) ) {
		float z;

		move.moveDir = path.endVelocity * 1.0f / 48.0f;

		// trace down to the floor and see if we can go forward
		idAI::PredictPath( this, aas, org, idVec3( 0.0f, 0.0f, -1024.0f ), 1000, 1000, SE_BLOCKED, path );

		idVec3 floorPos = path.endPos;
		idAI::PredictPath( this, aas, floorPos, move.moveDir * 48.0f, 1000, 1000, SE_BLOCKED, path );
		if ( !path.endEvent ) {
			move.moveDir.z = -1.0f;
			return true;
		}

		// trace up to see if we can go over something and go forward
		idAI::PredictPath( this, aas, org, idVec3( 0.0f, 0.0f, 256.0f ), 1000, 1000, SE_BLOCKED, path );

		idVec3 ceilingPos = path.endPos;

		for( z = org.z; z <= ceilingPos.z + 64.0f; z += 64.0f ) {
			idVec3 start;
			if ( z <= ceilingPos.z ) {
				start.x = org.x;
				start.y = org.y;
                start.z = z;
			} else {
				start = ceilingPos;
			}
			idAI::PredictPath( this, aas, start, move.moveDir * 48.0f, 1000, 1000, SE_BLOCKED, path );
			if ( !path.endEvent ) {
				move.moveDir.z = 1.0f;
				return true;
			}
		}
		return false;
	}

	return ( path.endEvent == 0 );
}
/*
================
idAI::MoveAlongVector
================
*/
bool idAI::MoveAlongVector( float yaw ) 
{
	StopMove( MOVE_STATUS_DONE );
	move.moveDir = idAngles( 0, yaw, 0 ).ToForward();

	// grayman #3993
	idVec3 pos = physicsObj.GetOrigin() + move.moveDir * 256.0f;
	SetStartTime(pos); // grayman #3993
	move.moveDest = pos;

	move.moveCommand	= MOVE_VECTOR;
	move.moveStatus		= MOVE_STATUS_MOVING;
	//move.startTime		= gameLocal.time; #3993
	move.speed			= fly_speed;
	AI_MOVE_DONE		= false;
	AI_FORWARD			= true;
	m_pathRank			= rank; // grayman #2345

	return true;
}
/*
================
idAI::NewWanderDir
================
*/
bool idAI::NewWanderDir( const idVec3 &dest ) {
	float	deltax, deltay;
	float	d[ 3 ];
	float	tdir, olddir, turnaround;

	move.nextWanderTime = gameLocal.time + ( gameLocal.random.RandomInt(500) + 500 );

	olddir = idMath::AngleNormalize360( ( int )( current_yaw / 45 ) * 45 );
	turnaround = idMath::AngleNormalize360( olddir - 180 );

	idVec3 org = physicsObj.GetOrigin();
	deltax = dest.x - org.x;
	deltay = dest.y - org.y;
	if ( deltax > 10 ) {
		d[ 1 ]= 0;
	} else if ( deltax < -10 ) {
		d[ 1 ] = 180;
	} else {
		d[ 1 ] = DI_NODIR;
	}

	if ( deltay < -10 ) {
		d[ 2 ] = 270;
	} else if ( deltay > 10 ) {
		d[ 2 ] = 90;
	} else {
		d[ 2 ] = DI_NODIR;
	}

	// try direct route
	if ( d[ 1 ] != DI_NODIR && d[ 2 ] != DI_NODIR ) {
		if ( d[ 1 ] == 0 ) {
			tdir = d[ 2 ] == 90 ? 45 : 315;
		} else {
			tdir = d[ 2 ] == 90 ? 135 : 215;
		}

		if ( tdir != turnaround && StepDirection( tdir ) ) {
			return true;
		}
	}

	// try other directions
	if ( ( gameLocal.random.RandomInt() & 1 ) || fabs( deltay ) > fabs( deltax ) ) {
		tdir = d[ 1 ];
		d[ 1 ] = d[ 2 ];
		d[ 2 ] = tdir;
	}

	if ( d[ 1 ] != DI_NODIR && d[ 1 ] != turnaround && StepDirection( d[1] ) ) {
		return true;
	}

	if ( d[ 2 ] != DI_NODIR && d[ 2 ] != turnaround	&& StepDirection( d[ 2 ] ) ) {
		return true;
	}

	// there is no direct path to the player, so pick another direction
	if ( olddir != DI_NODIR && StepDirection( olddir ) ) {
		return true;
	}

	 // randomly determine direction of search
	if ( gameLocal.random.RandomInt() & 1 ) {
		for( tdir = 0; tdir <= 315; tdir += 45 ) {
			if ( tdir != turnaround && StepDirection( tdir ) ) {
                return true;
			}
		}
	} else {
		for ( tdir = 315; tdir >= 0; tdir -= 45 ) {
			if ( tdir != turnaround && StepDirection( tdir ) ) {
				return true;
			}
		}
	}

	if ( turnaround != DI_NODIR && StepDirection( turnaround ) ) {
		return true;
	}

	// can't move
	StopMove( MOVE_STATUS_DEST_UNREACHABLE );
	return false;
}

/*
=====================
idAI::GetMovePos
=====================
*/
bool idAI::GetMovePos(idVec3 &seekPos)
{
	START_SCOPED_TIMING(aiGetMovePosTimer, scopedGetMovePosTimer);

	const idVec3& org = physicsObj.GetOrigin();
	seekPos = org;

	switch( move.moveCommand ) {
	case MOVE_NONE :
		seekPos = move.moveDest;
		return false;
		break;

	case MOVE_FACE_ENEMY :
	case MOVE_FACE_ENTITY :
		seekPos = move.moveDest;
		return false;
		break;

	case MOVE_TO_POSITION_DIRECT :
		seekPos = move.moveDest;
		if ( ReachedPos(move.moveDest, move.moveCommand))
		{
			StopMove( MOVE_STATUS_DONE );
		}
		return false;
		break;

	case MOVE_SLIDE_TO_POSITION :
		seekPos = org;
		return false;
		break;

	case MOVE_VECTOR :
		seekPos = move.moveDest;
		return true;
		break;
	default:
		break; // Handled below (note the returns in all cases above)
		// (default case added to suppress GCC warnings)
	}

	if (move.moveCommand == MOVE_TO_ENTITY) {
		MoveToEntity(move.goalEntity.GetEntity());
	}

	move.moveStatus = MOVE_STATUS_MOVING;
	bool result = false;

	// Check if the blocking time has already expired
	if (gameLocal.time > move.blockTime)
	{
		if (move.moveCommand == MOVE_WANDER)
		{
			move.moveDest = org + viewAxis[0] * physicsObj.GetGravityAxis() * 256.0f;
		} 
		else
		{
			// Check if we already reached the destination position
			if (ReachedPos(move.moveDest, move.moveCommand))
			{
				// Yes, stop the move, move status is DONE
				StopMove(MOVE_STATUS_DONE);
				seekPos	= org;
				return false; // nothing to do, return false
			}
		}

		if (aas != NULL && move.toAreaNum != 0)
		{
			// Get the area number we're currently in.
			int areaNum = PointReachableAreaNum(org);

			// Try to setup a path to the goal
			aasPath_t path;
			if (PathToGoal(path, areaNum, org, move.toAreaNum, move.moveDest, this))
			{
				seekPos = path.moveGoal;
				result = true; // We have a valid Path to the goal
				move.nextWanderTime = 0;

				// angua: check whether there is a door in the path
				if (path.firstDoor != NULL)
				{
					const idVec3& doorOrg = path.firstDoor->GetClosedBox().GetCenter(); // grayman #3755 - use door center, not origin
					//const idVec3& doorOrg = path.firstDoor->GetPhysics()->GetOrigin();
					const idVec3& org = GetPhysics()->GetOrigin();
					idVec3 dir = doorOrg - org;
					dir.z = 0;
					float dist = dir.LengthFast();
					if (dist < 500)
					{
						GetMind()->GetState()->OnFrobDoorEncounter(path.firstDoor);
					}	
				}
			}
			else
			{
				AI_DEST_UNREACHABLE = true;
			}
		}
	}

	if (!result)
	{
		// No path to the goal found, wander around
		if (gameLocal.time > move.nextWanderTime || !StepDirection(move.wanderYaw))
		{
			result = NewWanderDir(move.moveDest);
			if (!result)
			{
				StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				AI_DEST_UNREACHABLE = true;
				seekPos	= org;
				return false;
			}
		}
		else
		{
			result = true;
		}

		// Set the seek position to something far away, but in the right direction
		seekPos = org + move.moveDir * 2048.0f;

		if (ai_debugMove.GetBool())	{
			gameRenderWorld->DebugLine( colorYellow, org, seekPos, USERCMD_MSEC, true );
		}
	}
	else
	{
		// result == true, we have a valid path
		AI_DEST_UNREACHABLE = false;
	}

	if (result && ai_debugMove.GetBool()) {
		gameRenderWorld->DebugLine( colorCyan, physicsObj.GetOrigin(), seekPos );
	}

	return result;
}


/*
=====================
The Dark Mod
idAI::CanSee virtual override
=====================
*/
bool idAI::CanSee( idEntity *ent, bool useFOV ) const
{
	START_SCOPED_TIMING(aiCanSeeTimer, scopedCanSeeTimer);

	// Test if it is occluded, and use field of vision in the check (true as second parameter)
	bool cansee = idActor::CanSee( ent, useFOV );

	// Also consider lighting and visual acuity of AI
	if (cansee)
	{
		cansee = !IsEntityHiddenByDarkness(ent, 0.1f);	// tels: hard-coded threshold of 0.1f
	}

	// Return result
	return cansee;
}

/*
=====================
The Dark Mod
idAI::CanSeeExt

This method can ignore lighting conditions and/or field of vision.
=====================
*/
bool idAI::CanSeeExt( idEntity *ent, const bool useFOV, const bool useLighting ) const
{
	// Test if it is occluded
	bool cansee = idActor::CanSee( ent, useFOV );

	if (cansee && useLighting)
	{
		cansee = !IsEntityHiddenByDarkness(ent, 0.1f);	// tels: hard-coded threshold of 0.1f
	}

	// Return result
	return cansee;
}

// grayman #2859 - Can the AI see a point belonging to a target (not necessarily its origin)?

bool idAI::CanSeeTargetPoint( idVec3 point, idEntity* target , bool checkLighting ) const // grayman #2959
{
	// Check FOV

	if ( !CheckFOV( point ) )
	{
		return false;
	}

	// Check visibility

	trace_t result;
	idVec3 eye(GetEyePosition()); // eye position of the AI

	// Trace from eye to point, ignoring self

	gameLocal.clip.TracePoint(result, eye, point, MASK_OPAQUE, this);
	if ( result.fraction < 1.0 )
	{
		if ( gameLocal.GetTraceEntity(result) != target )
		{
			return false;
		}
	}

	// Check lighting?

	if ( checkLighting )
	{
		idVec3 topPoint = point - (physicsObj.GetGravityNormal() * 32.0);
		float maxDistanceToObserve;
		if ( g_lightQuotientAlgo.GetInteger() > 0 )
			maxDistanceToObserve = GetMaximumObservationDistance( target );
		else
			maxDistanceToObserve = GetMaximumObservationDistanceForPoints(point, topPoint);
		idVec3 ownOrigin = physicsObj.GetOrigin();

		return ( ( ( point - ownOrigin).LengthSqr() ) < Square(maxDistanceToObserve) ); // grayman #2866
	}

	return true;
}

/*
=====================
The Dark Mod
idAI::CanSeeRope

grayman #2872 - This method takes lighting conditions and field of vision under consideration.
=====================
*/
idVec3 idAI::CanSeeRope( idEntity *ent ) const
{
	// Find a point on the rope at the same elevation as the AI's eyes

	// ent is the projectile_result for the rope arrow. It's bound to the stuck rope arrow,
	// which also has the rope bound to it.

	idEntity* bindMaster = ent->GetBindMaster();
	if ( bindMaster != NULL )
	{
		idEntity* rope = bindMaster->FindMatchingTeamEntity( idAFEntity_Generic::Type );

		if ( rope != NULL )
		{
			idAFEntity_Generic* ropeAF = static_cast<idAFEntity_Generic*>(rope);
			const idDeclModelDef *modelDef = rope->GetAnimator()->ModelDef();
			idList<jointHandle_t> jointList;
			idStr jointNames = rope->spawnArgs.GetString("rope_joints"); // names of joints defined in the model file
			modelDef->GetJointList(jointNames,jointList);

			// Randomly select a joint, and randomly select a point along the
			// rope segmentbetween that joint and its lower neighbor

			int joint1Index = gameLocal.random.RandomInt( jointList.Num() - 1 );
			int joint2Index = joint1Index + 1;

			// safety net in case the joint names are corrupt

			if ( ( jointList[joint1Index] == INVALID_JOINT ) || ( jointList[joint2Index] == INVALID_JOINT ) )
			{
				return idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY); // failure
			}

			idMat3 axis;
			idVec3 joint1Org;
			ropeAF->GetJointWorldTransform( jointList[joint1Index], gameLocal.time, joint1Org, axis );
			idVec3 joint2Org;
			ropeAF->GetJointWorldTransform( jointList[joint2Index], gameLocal.time, joint2Org, axis );
			float offset = gameLocal.random.RandomFloat();
			idVec3 spot = joint1Org + ( joint2Org - joint1Org ) * offset;
			if ( CanSeeTargetPoint( spot, rope, true ) ) // grayman #2959
			{
				return spot;
			}
		}
	}

	// Return result
//	return CanSeeTargetPoint( point, ent ); // grayman #2872 - this has to move inside the part search loop above, exiting true at the first segment we can see
	return idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
}

/*
=====================
The Dark Mod
idAI::CanSeePositionExt

This method can ignore lighting conditions and/or field of vision.
=====================
*/
bool idAI::CanSeePositionExt( idVec3 position, const bool useFOV, const bool useLighting )
{
	if ( useFOV && !CheckFOV( position ) )
	{
		return false;
	}

	idVec3 ownOrigin = physicsObj.GetOrigin();

	bool canSee = EntityCanSeePos (this, ownOrigin, position);

	if (canSee && useLighting)
	{
		idVec3 bottomPoint = position;
		idVec3 topPoint = position - (physicsObj.GetGravityNormal() * 32.0);

		float maxDistanceToObserve = GetMaximumObservationDistanceForPoints(bottomPoint, topPoint);

		if ((position - ownOrigin).Length() > maxDistanceToObserve)
		{
			canSee = false;
		}

		// Draw debug graphic?
		if (cv_ai_visdist_show.GetFloat() > 1.0)
		{
			idVec3 midPoint = bottomPoint + ((topPoint-bottomPoint) / 2.0);
			idVec3 observeFrom = GetEyePosition();

			if (!canSee)
			{
				idVec4 markerColor (1.0, 0.0, 0.0, 0.0);
				idVec4 markerColor2 (1.0, 0.0, 1.0, 0.0);
				idVec3 arrowLength = midPoint - observeFrom;
				arrowLength.Normalize();
				arrowLength *= maxDistanceToObserve;

				// Distance we could see
				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFrom,
					observeFrom + arrowLength,
					2,
					cv_ai_visdist_show.GetInteger()
				);

				// Gap to where we want to see
				gameRenderWorld->DebugArrow
				(
					markerColor2,
					observeFrom + arrowLength,
					midPoint,
					2,
					cv_ai_visdist_show.GetInteger()
				);
			}
			else
			{
				idVec4 markerColor (0.0, 1.0, 0.0, 0.0);

				// We can see there
				gameRenderWorld->DebugArrow
				(
					markerColor,
					observeFrom,
					midPoint,
					2,
					cv_ai_visdist_show.GetInteger()
				);
			}
		}
	}

	return canSee;
}

/*
=====================
idAI::EntityCanSeePos
=====================
*/
bool idAI::EntityCanSeePos( idActor *actor, const idVec3 &actorOrigin, const idVec3 &pos )
{
	pvsHandle_t handle = gameLocal.pvs.SetupCurrentPVS( actor->GetPVSAreas(), actor->GetNumPVSAreas() );

	if ( !gameLocal.pvs.InCurrentPVS( handle, GetPVSAreas(), GetNumPVSAreas() ) )
	{
		gameLocal.pvs.FreeCurrentPVS( handle );
		return false;
	}

	gameLocal.pvs.FreeCurrentPVS( handle );

	idVec3 eye = actorOrigin + actor->EyeOffset();

	idVec3 point = pos;
	point[2] += 1.0f;

	physicsObj.DisableClip();

	trace_t results;
	gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, actor );
	if ( ( results.fraction >= 1.0f ) || ( gameLocal.GetTraceEntity( results ) == this ) )
	{
		physicsObj.EnableClip();
		return true;
	}

	const idBounds &bounds = physicsObj.GetBounds();
	point[2] += bounds[1][2] - bounds[0][2];

	gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, actor );
	physicsObj.EnableClip();
	if ( ( results.fraction >= 1.0f ) || ( gameLocal.GetTraceEntity( results ) == this ) )
	{
		return true;
	}

	return false;
}

float idAI::GetReachTolerance()
{
	return aas_reachability_z_tolerance;
}

/*
=====================
idAI::BlockedFailSafe
=====================
*/
void idAI::BlockedFailSafe( void ) {
	if ( !ai_blockedFailSafe.GetBool() || blockedRadius < 0.0f ) {
		return;
	}
	if ( !physicsObj.OnGround() || enemy.GetEntity() == NULL ||
			( physicsObj.GetOrigin() - move.lastMoveOrigin ).LengthSqr() > Square( blockedRadius ) ) {
		move.lastMoveOrigin = physicsObj.GetOrigin();
		move.lastMoveTime = gameLocal.time;
	}
	if ( move.lastMoveTime < gameLocal.time - blockedMoveTime ) {
		if ( lastAttackTime < gameLocal.time - blockedAttackTime ) {
			AI_BLOCKED = true;
			move.lastMoveTime = gameLocal.time;
		}
	}
}

/***********************************************************************

	turning

***********************************************************************/

/*
=====================
idAI::Turn
=====================
*/
void idAI::Turn(const idVec3& pivotOffset) {
	float diff;
	float diff2;
	float turnAmount;
	animFlags_t animflags;

	if (!turnRate) {
		return;
	}

	// check if the animator has marked this anim as non-turning
	if (!legsAnim.Disabled() && !legsAnim.AnimDone(0)) {
		animflags = legsAnim.GetAnimFlags();
	}
	else {
		animflags = torsoAnim.GetAnimFlags();
	}
	if (animflags.ai_no_turn) {
		return;
	}

	// Delay turning for custom idle anims --SteveL #3806
	// grayman - but not when moving, because we can't miss our turns
	if (!AI_FORWARD)
	{
		if ( WaitState() && ( idStr( WaitState() ) == "idle" || idStr( WaitState() ) == "idle_no_voice" ) )
		{
			return;
		}
	}

	idVec3 startPos = viewAxis * pivotOffset;

	if ( anim_turn_angles && animflags.anim_turn ) {
		idMat3 rotateAxis;

		// set the blend between no turn and full turn
		float frac = anim_turn_amount / anim_turn_angles;
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, 1.0f - frac );
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, frac );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, 1.0f - frac );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, frac );

		// get the total rotation from the start of the anim
		animator.GetDeltaRotation( 0, gameLocal.time, rotateAxis );
		current_yaw = idMath::AngleNormalize180( anim_turn_yaw + rotateAxis[ 0 ].ToYaw() );
	} else {
		diff = idMath::AngleNormalize180( ideal_yaw - current_yaw );
		turnVel += AI_TURN_SCALE * diff * MS2SEC( USERCMD_MSEC );
		if ( turnVel > turnRate ) {
			turnVel = turnRate;
		} else if ( turnVel < -turnRate ) {
			turnVel = -turnRate;
		}
		turnAmount = turnVel * MS2SEC( gameLocal.time - m_lastThinkTime );
		if ( ( diff >= 0.0f ) && ( turnAmount >= diff ) ) {
			turnVel = diff / MS2SEC( USERCMD_MSEC );
			turnAmount = diff;
		} else if ( ( diff <= 0.0f ) && ( turnAmount <= diff ) ) {
			turnVel = diff / MS2SEC( USERCMD_MSEC );
			turnAmount = diff;
		}
		current_yaw += turnAmount;
		current_yaw = idMath::AngleNormalize180( current_yaw );
		diff2 = idMath::AngleNormalize180( ideal_yaw - current_yaw );
		if ( idMath::Fabs( diff2 ) < 0.1f ) {
			current_yaw = ideal_yaw;
		}
	}

	viewAxis = idAngles( 0, current_yaw, 0 ).ToMat3();

	idVec3 endPos = viewAxis * pivotOffset;

	GetPhysics()->SetOrigin(GetPhysics()->GetOrigin() - idVec3(endPos - startPos));

	if ( ai_debugMove.GetBool() ) {
		const idVec3 &org = physicsObj.GetOrigin();
		gameRenderWorld->DebugLine( colorRed, org, org + idAngles( 0, ideal_yaw, 0 ).ToForward() * 64, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorGreen, org, org + idAngles( 0, current_yaw, 0 ).ToForward() * 48, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorYellow, org, org + idAngles( 0, current_yaw + turnVel, 0 ).ToForward() * 32, USERCMD_MSEC );
	}
}

/*
=====================
idAI::FacingIdeal
=====================
*/
bool idAI::FacingIdeal( void ) {
	float diff;

	if ( !turnRate ) {
		return true;
	}

	diff = idMath::AngleNormalize180( current_yaw - ideal_yaw );
	if ( idMath::Fabs( diff ) < 0.01f ) {
		// force it to be exact
		current_yaw = ideal_yaw;
		return true;
	}

	return false;
}

/*
=====================
idAI::TurnToward
=====================
*/
bool idAI::TurnToward( float yaw ) {
	ideal_yaw = idMath::AngleNormalize180( yaw );
	bool result = FacingIdeal();
	return result;
}

/*
=====================
idAI::TurnToward
=====================
*/
bool idAI::TurnToward( const idVec3 &pos ) {
	idVec3 dir;
	idVec3 local_dir;
	float lengthSqr;

	// grayman #3009 - you're not allowed to do this if you're sitting or playing a 'get_up' animation.
	// When sitting, it looks like crap when the AI spins in his chair.
	// The 'get_up' animation sometimes relies on turning toward a specific yaw, and the animation
	// will get screwed up if you allow this turn here.

	if ( ( move.moveType != MOVETYPE_SIT ) &&
		 ( move.moveType != MOVETYPE_GET_UP ) &&
		 ( move.moveType != MOVETYPE_WAKE_UP ) ) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
	{
		dir = pos - physicsObj.GetOrigin();
		physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
		local_dir.z = 0.0f;
		lengthSqr = local_dir.LengthSqr();
		if (   ( lengthSqr > Square( 2.0f ) ) ||
			 ( ( lengthSqr > Square( 0.1f ) ) && ( enemy.GetEntity() == NULL ) ) )
		{
			ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() );
		}
	}

	bool result = FacingIdeal();
	return result;
}

/***********************************************************************

	Movement

***********************************************************************/

/*
================
idAI::ApplyImpulse
================
*/
void idAI::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse )
{
	moveType_t moveType = move.moveType;
	if ( (moveType != MOVETYPE_STATIC) && (moveType != MOVETYPE_SLIDE) && (moveType != MOVETYPE_FLY) ) // grayman #4412
	{
		// grayman #4423 - If sitting or sleeping, don't allow the
		// impulse, because it screws up these other animations.

		if ( moveType == MOVETYPE_SIT ||
			moveType == MOVETYPE_SLEEP ||
			moveType == MOVETYPE_SIT_DOWN ||
			moveType == MOVETYPE_FALL_ASLEEP || // grayman #3820 - was MOVETYPE_LAY_DOWN
			moveType == MOVETYPE_GET_UP ||
			moveType == MOVETYPE_WAKE_UP ) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
		{
			// ignore the impulse
		}
		else
		{
			idActor::ApplyImpulse(ent, id, point, impulse);
		}
	}
}

/*
=====================
idAI::GetMoveDelta
=====================
*/
void idAI::GetMoveDelta( const idMat3 &oldaxis, const idMat3 &axis, idVec3 &delta )
{
	// Get the delta from the animation system (for one frame) and transform it
	// animator.GetDelta( gameLocal.time - USERCMD_MSEC, gameLocal.time, delta);

	// angua: distance from last thinking time
	animator.GetDelta( m_lastThinkTime, gameLocal.time, delta);

	delta = axis * delta;

	if ( modelOffset != vec3_zero )
	{
		// the pivot of the monster's model is around its origin, and not around the bounding
		// box's origin, so we have to compensate for this when the model is offset so that
		// the monster still appears to rotate around its origin.
		idVec3 oldModelOrigin = modelOffset * oldaxis;
		idVec3 modelOrigin = modelOffset * axis;	
		
		delta += oldModelOrigin - modelOrigin;
	}

	delta *= physicsObj.GetGravityAxis();
}

/*
=====================
idAI::CheckObstacleAvoidance
=====================
*/
void idAI::CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &newPos )
{
	START_SCOPED_TIMING(aiObstacleAvoidanceTimer, scopedObstacleAvoidanceTimer);

	newPos = goalPos;	// grayman #2345 - initialize newPos in case nothing changes.
						// Without this, there's a return from this function
						// that leaves newPos = [0,0,0], so the AI thinks he
						// has to go to the world origin, which is a bug.

	if (ignore_obstacles || cv_ai_opt_noobstacleavoidance.GetBool())
	{
		move.obstacle = NULL;
		return;
	}

	if (cv_ai_goalpos_show.GetBool()) 
	{
		idVec4 color(colorCyan);
		color.x = gameLocal.random.RandomFloat();
		color.y = gameLocal.random.RandomFloat();
		color.z = gameLocal.random.RandomFloat();
		gameRenderWorld->DebugArrow(color, goalPos, goalPos + idVec3(0,0,15), 2, 16);
	}

	const idVec3& origin = physicsObj.GetOrigin();

	idEntity* obstacle = NULL;
	obstaclePath_t	path;

	AI_OBSTACLE_IN_PATH = false;
	bool foundPath = FindPathAroundObstacles( &physicsObj, aas, NULL, origin, goalPos, path, this ); // grayman #3548
	//bool foundPath = FindPathAroundObstacles( &physicsObj, aas, enemy.GetEntity(), origin, goalPos, path, this );

	if ( ai_showObstacleAvoidance.GetBool())
	{
		gameRenderWorld->DebugLine( colorBlue, goalPos + idVec3( 1.0f, 1.0f, 0.0f ), goalPos + idVec3( 1.0f, 1.0f, 64.0f ), USERCMD_MSEC );
		gameRenderWorld->DebugLine( foundPath ? colorYellow : colorRed, path.seekPos, path.seekPos + idVec3( 0.0f, 0.0f, 64.0f ), USERCMD_MSEC );
	}

	if (!foundPath)
	{
		// couldn't get around obstacles

		if (path.doorObstacle != NULL) 
		{
			// We have a frobmover in our way, raise a signal to the current state
			mind->GetState()->OnFrobDoorEncounter(path.doorObstacle);
		}

		if (path.firstObstacle)
		{
			AI_OBSTACLE_IN_PATH = true;

			if ( physicsObj.GetAbsBounds().Expand( 2.0f ).IntersectsBounds( path.firstObstacle->GetPhysics()->GetAbsBounds() ) )
			{
				// We are already touching the first obstacle
				obstacle = path.firstObstacle;
			}
		}
		else if (path.startPosObstacle)
		{
			AI_OBSTACLE_IN_PATH = true;
			if ( physicsObj.GetAbsBounds().Expand( 2.0f ).IntersectsBounds( path.startPosObstacle->GetPhysics()->GetAbsBounds() ) )
			{
				// We are touching the startpos obstacle
				obstacle = path.startPosObstacle;
			}
		}
		else
		{
			// No firstObstacle, no startPosObstacle, must be blocked by wall
			move.moveStatus = MOVE_STATUS_BLOCKED_BY_WALL;
		}
#if 0
	} else if ( path.startPosObstacle ) {
		// check if we're past where the our origin was pushed out of the obstacle
		dir = goalPos - origin;
		dir.Normalize();
		dist = ( path.seekPos - origin ) * dir;
		if ( dist < 1.0f ) {
			AI_OBSTACLE_IN_PATH = true;
			obstacle = path.startPosObstacle;
		}
#endif
	}
	else
	{
		// We found a path around obstacles

		/* grayman #3786
		if (path.doorObstacle != NULL) // grayman #2712 - do we have to handle a door?
		{
			// We have a frobmover in our way, raise a signal to the current state
			mind->GetState()->OnFrobDoorEncounter(path.doorObstacle);
		}*/

		// still check for the seekPosObstacle
		if (path.seekPosObstacle)
		{
	/*		gameRenderWorld->DebugBox(foundPath ? colorGreen : colorRed, idBox(path.seekPosObstacle->GetPhysics()->GetBounds(), 
													  path.seekPosObstacle->GetPhysics()->GetOrigin(), 
													  path.seekPosObstacle->GetPhysics()->GetAxis()), 16);
	 */

			// greebo: Check if we have a frobdoor entity at our seek position
			if (path.seekPosObstacle->IsType(CFrobDoor::Type)) 
			{
				// We have a frobmover in our way, raise a signal to the current state
				mind->GetState()->OnFrobDoorEncounter(static_cast<CFrobDoor*>(path.seekPosObstacle));
			}

			// if the AI is very close to the path.seekPos already and path.seekPosObstacle != NULL
			// then we want to push the path.seekPosObstacle entity out of the way
			AI_OBSTACLE_IN_PATH = true;

			// check if we're past where the goalPos was pushed out of the obstacle
			idVec3 dir = goalPos - origin;
			dir.NormalizeFast();
			float dist = (path.seekPos - origin) * dir;
			if (dist < 1.0f)
			{
				obstacle = path.seekPosObstacle;
			}
		}
	}

	// If we don't have an obstacle, set the seekpos and return
	if (obstacle == NULL)
	{
		newPos = path.seekPos;
		move.obstacle = NULL;
		return;
	}

	// if we had an obstacle, set our move status based on the type, and kick it out of the way if it's a moveable
	if (obstacle->IsType(idActor::Type))
	{
		// monsters aren't kickable
		move.moveStatus = (obstacle == enemy.GetEntity()) ? MOVE_STATUS_BLOCKED_BY_ENEMY : MOVE_STATUS_BLOCKED_BY_MONSTER;
	}
	else
	{
		// If it's a door handle, switch the obstacle to the door so we don't get all hung
		// up on door handles
		if (obstacle->IsType(CFrobDoorHandle::Type))
		{
			// Make the obstacle the door itself
			obstacle = static_cast<CFrobDoorHandle*>(obstacle)->GetDoor();
		}

		// Handle doors
		if (obstacle->IsType(CFrobDoor::Type))
		{
			// Try to open doors
			CFrobDoor* p_door = static_cast<CFrobDoor*>(obstacle);
		
			// We have a frobmover in our way, raise a signal to the current state
			mind->GetState()->OnFrobDoorEncounter(p_door);
		}
		
		// Try backing away
		newPos = obstacle->GetPhysics()->GetOrigin();
		idVec3 obstacleDelta = obstacle->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();

		obstacleDelta.NormalizeFast();
		obstacleDelta *= 128.0;

		newPos = obstacle->GetPhysics()->GetOrigin() - obstacleDelta;
		move.moveStatus = MOVE_STATUS_BLOCKED_BY_OBJECT;
	}

	move.obstacle = obstacle;
}

/*
=====================
idAI::CanPassThroughDoor - Is the AI allowed through this door? (grayman #2691) 
=====================
*/

bool idAI::CanPassThroughDoor(CFrobDoor* frobDoor)
{
	// grayman #2691 - quick test for spawnarg that appears on many furniture doors

	if (frobDoor->spawnArgs.GetBool("immune_to_target_setfrobable", "0"))
	{
		return false;
	}

	// grayman #2691 - can't pass through doors that don't rotate on the z-axis

	idVec3 rotationAxis = frobDoor->GetRotationAxis();
	if ((rotationAxis.z == 0) && ((rotationAxis.x != 0) || (rotationAxis.y != 0))) // grayman #2712 - handles sliding doors
	{
		return false;
	}

	idBounds door1Bounds = frobDoor->GetPhysics()->GetBounds();
	idBounds myBounds = GetPhysics()->GetBounds();
	idVec3 door1Size = door1Bounds.GetSize();
	idVec3 mySize = myBounds.GetSize();
	bool canPassDoor1 = (door1Size.x > mySize.x) || (door1Size.y > mySize.y);
	if (canPassDoor1)
	{
		return true;
	}

	// The AI can't fit through the first door. Is this door part of a double door?
	
	CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();
	if (doubleDoor != NULL)
	{
		idBounds door2Bounds = doubleDoor->GetPhysics()->GetBounds();
		idVec3 door2Size = door2Bounds.GetSize();
		bool canPassDoors = ((door1Size.x + door2Size.x) > mySize.x) || ((door1Size.y + door2Size.y) > mySize.y);
		if (canPassDoors)
		{
			return true;
		}
	}

	return false;
}

/*
=====================
idAI::GetTorch - Is the AI carrying a torch? (grayman #2603) 
=====================
*/

idEntity* idAI::GetTorch()
{
	idEntity* ent = GetAttachmentByPosition("hand_l");
	if (ent && ent->spawnArgs.GetBool("is_torch","0"))
	{
		return ent; // found a torch
	}

	// Torches are carried in the left hand. If a torch for
	// the right hand, plus accompanying animations, is ever
	// created, uncomment the following section.
/*
	ent = GetAttachmentByPosition("hand_r");
	if (ent && ent->spawnArgs.GetBool("is_torch","0"))
	{
		return ent; // found a torch
	}
 */

	return NULL; // no luck
}

/*
=====================
idAI::GetLantern - Is the AI carrying a lantern? (grayman #3559) 
=====================
*/

idEntity* idAI::GetLantern()
{
	idEntity* ent = GetAttachmentByPosition("hand_l");
	if (ent && ent->spawnArgs.GetBool("is_lantern","0"))
	{
		return ent; // found a lantern
	}

	// Lanterns are carried in the left hand. If a lantern for
	// the right hand, plus accompanying animations, is ever
	// created, uncomment the following section.
/*
	ent = GetAttachmentByPosition("hand_r");
	if (ent && ent->spawnArgs.GetBool("is_lantern","0"))
	{
		return ent; // found a is_lantern
	}
 */

	return NULL; // no luck
}
/*
=====================
idAI::DeadMove
=====================
*/
void idAI::DeadMove( void ) {
	idVec3				delta;
	//monsterMoveResult_t	moveResult;

	idVec3 org = physicsObj.GetOrigin();

	GetMoveDelta( viewAxis, viewAxis, delta );
	physicsObj.SetDelta( delta );

	RunPhysics();

	// stgatilov #6546: keep light quotient always valid for AI bodies
	gameLocal.m_LightEstimateSystem->TrackEntity( this );

	//moveResult = physicsObj.GetMoveResult();
	AI_ONGROUND = physicsObj.OnGround();
}

/*
=====================
idAI::AnimMove
=====================
*/
void idAI::AnimMove()
{
	START_SCOPED_TIMING(aiAnimMoveTimer, scopedAnimMoveTimer);

	idVec3				goalPos;
	idVec3				goalDelta;

	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	AI_BLOCKED = false;

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS ){
		move.lastMoveOrigin.Zero();
		move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;
	if (move.moveCommand == MOVE_FACE_ENEMY && enemy.GetEntity())
	{
		TurnToward( lastVisibleEnemyPos );
		goalPos = oldorigin;
	}
	else if (move.moveCommand == MOVE_FACE_ENTITY && move.goalEntity.GetEntity())
	{
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		goalPos = oldorigin;
	} 
	else if (GetMovePos(goalPos)) 
	{
		// greebo: We have a valid goalposition (not reached the target yet), check for obstacles
		if (move.moveCommand != MOVE_WANDER && move.moveCommand != MOVE_VECTOR) 
		{
			idVec3 newDest;
			CheckObstacleAvoidance( goalPos, newDest );
			TurnToward(newDest);
		} 
		else // MOVE_WANDER || MOVE_VECTOR
		{
			TurnToward(goalPos);
		}
	}

	// greebo: This should take care of rats running around in circles around their goal
	// due to not turning fast enough to their goalPos, sending them into a stable orbit.
	float oldTurnRate = turnRate;

	if ((goalPos - physicsObj.GetOrigin()).LengthSqr() < Square(50))
	{
		turnRate *= gameLocal.random.RandomFloat() + 1;
	}

	// greebo: Now actually turn towards the "ideal" yaw.
	Turn();

	// Revert the turnRate changes
	turnRate = oldTurnRate;

	// Determine the delta depending on the move type
	idVec3 delta(0,0,0);
	if (move.moveCommand == MOVE_SLIDE_TO_POSITION)
	{
		if ( gameLocal.time < move.startTime + move.duration ) {
			goalPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
			delta = goalPos - oldorigin;
			delta.z = 0.0f;
		} else {
			delta = move.moveDest - oldorigin;
			delta.z = 0.0f;
			StopMove(MOVE_STATUS_DONE);
		}
	}
	else if (allowMove)
	{
		// Moving is allowed, get the delta
		GetMoveDelta( oldaxis, viewAxis, delta );
	}

	if ( move.moveCommand == MOVE_TO_POSITION ) {
		goalDelta = move.moveDest - oldorigin;
		float goalDist = goalDelta.LengthFast();
		if ( goalDist < delta.LengthFast() ) {
			delta = goalDelta;
		}
	}

	physicsObj.SetDelta( delta );
	physicsObj.ForceDeltaMove( disableGravity );
	physicsObj.UseFlyMove( false );

	{
		START_SCOPED_TIMING(aiPhysicsTimer, scopedPhysicsTimer);
		RunPhysics();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}

	//monsterMoveResult_t moveResult = physicsObj.GetMoveResult();
	if ( !m_bAFPushMoveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt)
		{
			if (blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() )
			{
				KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
			}
		}
	}

	BlockedFailSafe();

	AI_ONGROUND = physicsObj.OnGround();

	const idVec3& org = physicsObj.GetOrigin();
	// Obsttorte #5319
	//if (oldorigin != org) { 
		TouchTriggers();
	//}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, USERCMD_MSEC );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, USERCMD_MSEC, true );
		DrawRoute();
	}
}

/*
=====================
idAI::SittingMove
=====================
*/
void idAI::SittingMove()
{
	monsterMoveResult_t	moveResult;

	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	// Interleaved thinking
	AI_BLOCKED = false;

	RunPhysics();

	Turn(sitting_turn_pivot);

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}

	moveResult = physicsObj.GetMoveResult();

	AI_ONGROUND = physicsObj.OnGround();

	const idVec3& org = physicsObj.GetOrigin();
	if (oldorigin != org) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, USERCMD_MSEC );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, USERCMD_MSEC, true );
		DrawRoute();
	}
}

void idAI::NoTurnMove()
{
	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	AI_BLOCKED = false;

	RunPhysics();

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}


	AI_ONGROUND = physicsObj.OnGround();

	const idVec3& org = physicsObj.GetOrigin();
	if (oldorigin != org) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, USERCMD_MSEC );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, USERCMD_MSEC, true );
		DrawRoute();
	}

}



void idAI::SleepingMove()
{
	idVec3 oldorigin(physicsObj.GetOrigin());
	idMat3 oldaxis(viewAxis);

	AI_BLOCKED = false;

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}

	AI_ONGROUND = physicsObj.OnGround();

	// angua: Let the animation move the origin onto the bed
	idVec3 delta;
	GetMoveDelta( oldaxis, viewAxis, delta );

	physicsObj.SetDelta( delta );
	physicsObj.ForceDeltaMove( disableGravity );

	RunPhysics();

	const idVec3& org = physicsObj.GetOrigin();
	if (oldorigin != org) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, USERCMD_MSEC );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, USERCMD_MSEC, true );
		DrawRoute();
	}
}


/*
=====================
Seek
=====================
*/
idVec3 Seek( idVec3 &vel, const idVec3 &org, const idVec3 &goal, float prediction ) {
	idVec3 predictedPos;
	idVec3 goalDelta;
	idVec3 seekVel;

	// predict our position
	predictedPos = org + vel * prediction;
	goalDelta = goal - predictedPos;
	seekVel = goalDelta * MS2SEC( USERCMD_MSEC );

	return seekVel;
}

/*
=====================
idAI::SlideMove
=====================
*/
void idAI::SlideMove( void ) {
	idVec3				goalPos;
	idVec3				delta;
	idVec3				goalDelta;
	float				goalDist;
	//monsterMoveResult_t	moveResult;
	idVec3				newDest;

	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	AI_BLOCKED = false;

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS ){
		move.lastMoveOrigin.Zero();
		move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;
	if ( ( move.moveCommand == MOVE_FACE_ENEMY ) && enemy.GetEntity() ) {
		TurnToward( lastVisibleEnemyPos );
		goalPos = move.moveDest;
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		goalPos = move.moveDest;
	} else if ( GetMovePos( goalPos ) ) {
		CheckObstacleAvoidance( goalPos, newDest );
		TurnToward( newDest );
		goalPos = newDest;
	}

	if ( move.moveCommand == MOVE_SLIDE_TO_POSITION ) {
		if ( gameLocal.time < move.startTime + move.duration ) {
			goalPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
		} else {
			goalPos = move.moveDest;
			StopMove( MOVE_STATUS_DONE );
		}
	}

	if ( move.moveCommand == MOVE_TO_POSITION ) {
		goalDelta = move.moveDest - oldorigin;
		goalDist = goalDelta.LengthFast();
		if ( goalDist < delta.LengthFast() ) {
			delta = goalDelta;
		}
	}

	idVec3 vel = physicsObj.GetLinearVelocity();
	float z = vel.z;
	idVec3  predictedPos = oldorigin + vel * AI_SEEK_PREDICTION;

	// seek the goal position
	goalDelta = goalPos - predictedPos;
	vel -= vel * AI_FLY_DAMPENING * MS2SEC( USERCMD_MSEC );
	vel += goalDelta * MS2SEC( USERCMD_MSEC );

	// cap our speed
	vel.Truncate( fly_speed );
	vel.z = z;
	physicsObj.SetLinearVelocity( vel );
	physicsObj.UseVelocityMove( true );
	RunPhysics();

	if ( ( move.moveCommand == MOVE_FACE_ENEMY ) && enemy.GetEntity() ) {
		TurnToward( lastVisibleEnemyPos );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else if ( move.moveCommand != MOVE_NONE ) {
		if ( vel.ToVec2().LengthSqr() > 0.1f ) {
			TurnToward( vel.ToYaw() );
		}
	}

	Turn();

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}

	//moveResult = physicsObj.GetMoveResult();
	if ( !m_bAFPushMoveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		}
	}

	BlockedFailSafe();

	AI_ONGROUND = physicsObj.OnGround();

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, USERCMD_MSEC );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, USERCMD_MSEC, true );
		DrawRoute();
	}
}

/*
=====================
idAI::AdjustFlyingAngles
=====================
*/
void idAI::AdjustFlyingAngles( void ) {
	idVec3	vel;
	float 	speed;
	float 	roll;
	float 	pitch;

	vel = physicsObj.GetLinearVelocity();

	speed = vel.Length();
	if ( speed < 5.0f ) {
		roll = 0.0f;
		pitch = 0.0f;
	} else {
		roll = vel * viewAxis[ 1 ] * -fly_roll_scale / fly_speed;
		if ( roll > fly_roll_max ) {
			roll = fly_roll_max;
		} else if ( roll < -fly_roll_max ) {
			roll = -fly_roll_max;
		}

		pitch = vel * viewAxis[ 2 ] * -fly_pitch_scale / fly_speed;
		if ( pitch > fly_pitch_max ) {
			pitch = fly_pitch_max;
		} else if ( pitch < -fly_pitch_max ) {
			pitch = -fly_pitch_max;
		}
	}

	fly_roll = fly_roll * 0.95f + roll * 0.05f;
	fly_pitch = fly_pitch * 0.95f + pitch * 0.05f;

	if ( flyTiltJoint != INVALID_JOINT ) {
		animator.SetJointAxis( flyTiltJoint, JOINTMOD_WORLD, idAngles( fly_pitch, 0.0f, fly_roll ).ToMat3() );
	} else {
		viewAxis = idAngles( fly_pitch, current_yaw, fly_roll ).ToMat3();
	}
}

/*
=====================
idAI::AddFlyBob
=====================
*/
void idAI::AddFlyBob( idVec3 &vel ) {
	idVec3	fly_bob_add;
	float	t;

	if ( fly_bob_strength ) {
		t = MS2SEC( gameLocal.time + entityNumber * 497 );
		fly_bob_add = ( viewAxis[ 1 ] * idMath::Sin16( t * fly_bob_horz ) + viewAxis[ 2 ] * idMath::Sin16( t * fly_bob_vert ) ) * fly_bob_strength;
		vel += fly_bob_add * MS2SEC( USERCMD_MSEC );
		if ( ai_debugMove.GetBool() ) {
			const idVec3 &origin = physicsObj.GetOrigin();
			gameRenderWorld->DebugArrow( colorOrange, origin, origin + fly_bob_add, 0 );
		}
	}
}

/*
=====================
idAI::AdjustFlyHeight
=====================
*/
void idAI::AdjustFlyHeight( idVec3 &vel, const idVec3 &goalPos ) {
	const idVec3	&origin = physicsObj.GetOrigin();
	predictedPath_t path;
	idVec3			end;
	idVec3			dest;
	trace_t			trace;
	idActor			*enemyEnt;
	bool			goLower;

	// make sure we're not flying too high to get through doors
	goLower = false;
	if ( origin.z > goalPos.z )
	{
		dest = goalPos;
		dest.z = origin.z + 128.0f;
		idAI::PredictPath( this, aas, goalPos, dest - origin, 1000, 1000, SE_BLOCKED, path );
		if ( path.endPos.z < origin.z ) {
			idVec3 addVel = Seek( vel, origin, path.endPos, AI_SEEK_PREDICTION );
			vel.z += addVel.z;
			goLower = true;
		}

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugBounds( goLower ? colorRed : colorGreen, physicsObj.GetBounds(), path.endPos, USERCMD_MSEC );
		}
	}

	if ( !goLower )
	{
		// make sure we don't fly too low
		end = origin;

		enemyEnt = enemy.GetEntity();
		if ( enemyEnt ) {
			end.z = lastVisibleEnemyPos.z + lastVisibleEnemyEyeOffset.z + fly_offset;
		} else {
			// just use the default eye height for the player
			end.z = goalPos.z + DEFAULT_FLY_OFFSET + fly_offset;
		}

		gameLocal.clip.Translation( trace, origin, end, physicsObj.GetClipModel(), mat3_identity, MASK_MONSTERSOLID, this );
		vel += Seek(vel, origin, trace.endpos, AI_SEEK_PREDICTION);
	}
}

/*
=====================
idAI::FlySeekGoal
=====================
*/
void idAI::FlySeekGoal( idVec3 &vel, idVec3 &goalPos ) {
	idVec3 seekVel;

	// seek the goal position
	seekVel = Seek( vel, physicsObj.GetOrigin(), goalPos, AI_SEEK_PREDICTION );
	seekVel *= fly_seek_scale;
	vel += seekVel;
}

/*
=====================
idAI::AdjustFlySpeed
=====================
*/
void idAI::AdjustFlySpeed( idVec3 &vel ) {
	float speed;

	// apply dampening
	vel -= vel * AI_FLY_DAMPENING * MS2SEC( USERCMD_MSEC );

	// gradually speed up/slow down to desired speed
	speed = vel.Normalize();
	speed += ( move.speed - speed ) * MS2SEC( USERCMD_MSEC );
	if ( speed < 0.0f ) {
		speed = 0.0f;
	} else if ( move.speed && ( speed > move.speed ) ) {
		speed = move.speed;
	}

	vel *= speed;
}

/*
=====================
idAI::FlyTurn
=====================
*/
void idAI::FlyTurn( void ) {
	if ( move.moveCommand == MOVE_FACE_ENEMY ) {
		TurnToward( lastVisibleEnemyPos );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else if ( move.speed > 0.0f ) {
		const idVec3 &vel = physicsObj.GetLinearVelocity();
		if ( vel.ToVec2().LengthSqr() > 0.1f ) {
			TurnToward( vel.ToYaw() );
		}
	}
	Turn();
}

/*
=====================
idAI::FlyMove
=====================
*/
void idAI::FlyMove( void ) {
	idVec3	goalPos;
	idVec3	oldorigin;

	AI_BLOCKED = false;
	if ( ( move.moveCommand != MOVE_NONE ) && ReachedPos( move.moveDest, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
	}
	if ( ai_debugMove.GetBool() ) {
		gameLocal.Printf( "%d: %s: %s, vel = %.2f, sp = %.2f, maxsp = %.2f\n", gameLocal.time, name.c_str(), moveCommandString[ move.moveCommand ], physicsObj.GetLinearVelocity().Length(), move.speed, fly_speed );
	}
	if ( move.moveCommand != MOVE_TO_POSITION_DIRECT ) {
		idVec3 vel = physicsObj.GetLinearVelocity();

		if ( GetMovePos( goalPos ) ) {
			// angua: this fucks up the height movement, disabled for now...
			// the height of the seekpos is changed in FindOptimalPath 
			// CheckObstacleAvoidance( goalPos, newDest );
		}

		if ( move.speed	) {
			FlySeekGoal( vel, goalPos );
		}

		// add in bobbing
		AddFlyBob( vel );

		if (enemy.GetEntity() && ( move.moveCommand != MOVE_TO_POSITION ) ) {
			AdjustFlyHeight( vel, goalPos );
		}

		AdjustFlySpeed( vel );

		physicsObj.SetLinearVelocity( vel );
	}

	// turn
	FlyTurn();

	// run the physics for this frame
	oldorigin = physicsObj.GetOrigin();
	physicsObj.UseFlyMove( true );
	physicsObj.UseVelocityMove( false );
	physicsObj.SetDelta( vec3_zero );
	physicsObj.ForceDeltaMove( disableGravity );
	RunPhysics();

	monsterMoveResult_t	moveResult = physicsObj.GetMoveResult();
	if ( !m_bAFPushMoveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		} else if ( moveResult == MM_BLOCKED ) {
			move.blockTime = gameLocal.time + 500;
			AI_BLOCKED = true;
		}
	}

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 4000 );
		gameRenderWorld->DebugBounds( colorOrange, physicsObj.GetBounds(), org, USERCMD_MSEC );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorRed, org, org + physicsObj.GetLinearVelocity(), USERCMD_MSEC, true );
		gameRenderWorld->DebugLine( colorBlue, org, goalPos, USERCMD_MSEC, true );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, USERCMD_MSEC, true );
		DrawRoute();
	}
}

/*
=====================
idAI::StaticMove
=====================
*/
void idAI::StaticMove( void ) {
	idActor	*enemyEnt = enemy.GetEntity();

	if ( AI_DEAD || AI_KNOCKEDOUT ) {
		return;
	}

	if ( ( move.moveCommand == MOVE_FACE_ENEMY ) && enemyEnt ) {
		TurnToward( lastVisibleEnemyPos );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else if ( move.moveCommand != MOVE_NONE ) {
		TurnToward( move.moveDest );
	}
	Turn();

	physicsObj.ForceDeltaMove( true ); // disable gravity
	RunPhysics();

	AI_ONGROUND = false;

	if ( !m_bAFPushMoveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemyEnt );
	}

	if ( ai_debugMove.GetBool() ) {
		const idVec3 &org = physicsObj.GetOrigin();
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, USERCMD_MSEC );
		gameRenderWorld->DebugLine( colorBlue, org, move.moveDest, USERCMD_MSEC, true );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, USERCMD_MSEC, true );
	}
}

void idAI::Bark(const idStr& soundName)
{
	// Allocate a singlebarktask with the given sound and enqueue it
	commSubsystem->AddCommTask(
		ai::CommunicationTaskPtr(new ai::SingleBarkTask(soundName))
	);
}

void idAI::PlayFootStepSound()
{
	idStr				moveType, localSound, sound;
	idMaterial			*material = NULL;
	const idSoundShader	*sndShader = NULL;
	
	if ( !GetPhysics()->HasGroundContacts() ) {
		return;
	}

	// greebo: Don't play footsteps in ragdoll mode
	if (!GetPhysics()->IsType(idPhysics_Actor::Type))
	{
		return;
	}

	// DarkMod: make the string to identify the movement speed (crouch_run, creep, etc)
	// Currently only players have movement flags set up this way, not AI.  We could change that later.
	moveType.Clear();

	if (AI_CROUCH)
	{
		moveType = "_crouch";
	}

	if (AI_RUN)
	{
		moveType += "_run";
	}
	else if (AI_CREEP)
	{
		moveType += "_creep";
	}
	else
	{
		moveType += "_walk";
	}

	// start footstep sound based on material type
	material = const_cast<idMaterial*>(GetPhysics()->GetContact(0).material);
	localSound = ""; // grayman #2787
	if (material != NULL) 
	{
		g_Global.GetSurfName(material, localSound);
		localSound = "snd_footstep_" + localSound;
//		sound = spawnArgs.GetString( localSound.c_str() ); // grayman #2787
	}

	waterLevel_t waterLevel = static_cast<idPhysics_Actor *>(GetPhysics())->GetWaterLevel();
	// If actor is walking in liquid, replace the bottom surface sound with water sounds
	if (waterLevel == WATERLEVEL_FEET )
	{
		localSound = "snd_footstep_puddle";
//		sound = spawnArgs.GetString( localSound.c_str() ); // grayman #2787
	}
	else if (waterLevel == WATERLEVEL_WAIST)
	{
		localSound = "snd_footstep_wading";
//		sound = spawnArgs.GetString( localSound.c_str() ); // grayman #2787
	}
	// greebo: Added this to disable the walking sound when completely underwater
	// this should be replaced by snd_
	else if (waterLevel == WATERLEVEL_HEAD)
	{
		localSound = "snd_footstep_swim";
//		sound = spawnArgs.GetString( localSound.c_str() ); // grayman #2787
	}

	if ( localSound.IsEmpty() && ( waterLevel != WATERLEVEL_HEAD ) ) // grayman #2787
	{
		localSound = "snd_footstep";
	}
	
	sound = spawnArgs.GetString( localSound.c_str() );

	// if a sound was not found for that specific material, use default
	if ( sound.IsEmpty() && ( waterLevel != WATERLEVEL_HEAD ) )
	{
		sound = spawnArgs.GetString( "snd_footstep" );
		localSound = "snd_footstep";
	}

	/***
	* AI footsteps always propagate as snd_footstep for now
	* If we want to add in AI soundprop based on movement speed later,
	*	here is the place to do it.
	**/
/*
	localSound += moveType;
	if( !gameLocal.m_sndProp->CheckSound( localSound.c_str(), false ) )
		localSound -= moveType;
*/

	if ( !sound.IsEmpty() ) 
	{
		// apply the movement type modifier to the volume
		sndShader = declManager->FindSound( sound.c_str() );
		SetSoundVolume( sndShader->GetParms()->volume + GetMovementVolMod() );
		StartSoundShader( sndShader, SND_CHANNEL_BODY, 0, false, NULL );
		SetSoundVolume();

		// propagate the suspicious sound to other AI
		PropSoundDirect( localSound, true, false, 0.0f, 0 ); // grayman #3355
	}
}

/***********************************************************************

	Damage

***********************************************************************/

/*
=====================
idAI::ReactionTo

DarkMod : Added call to AI Relationship Manager
		  We don't have to hardcode this, could be done
		  with scripts, but for now it's hardcoded for
		  testing purposes.
=====================
*/
int idAI::ReactionTo( const idEntity *ent )
{
	if ( ent->fl.hidden )
	{
		// ignore hidden entities
		return ATTACK_IGNORE;
	}

	if ( !ent->IsType( idActor::Type ) ) {
		return ATTACK_IGNORE;
	}

	const idActor *actor = static_cast<const idActor *>( ent );
	if ( actor->IsType( idPlayer::Type ) && static_cast<const idPlayer *>(actor)->noclip ) {
		// ignore players in noclip mode
		return ATTACK_IGNORE;
	}

	// actors will always fight if their teams are enemies
	if ( IsEnemy(actor) )
	{
		if ( ( actor->fl.notarget ) || ( actor->fl.invisible ) ) // grayman #3857
		{
			// don't attack on sight when attacker is notargeted
			return ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE;
		}
		return ATTACK_ON_SIGHT | ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE;
	}

	// monsters will fight when attacked by lower ranked monsters.  rank 0 never fights back.
	if ( rank && ( actor->rank < rank ) )
	{
		return ATTACK_ON_DAMAGE;
	}

	// don't fight back
	return ATTACK_IGNORE;
}


/*
=====================
idAI::Pain
=====================
*/
bool idAI::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const idDict* damageDef )
{
	AI_PAIN = idActor::Pain( inflictor, attacker, damage, dir, location, damageDef );
	AI_DAMAGE = true;

	// force a blink
	blink_time = 0;

	// grayman #2816 - if no pain was registered in idActor::Pain() then exit

	if ( !AI_PAIN )
	{
		return false;
	}

	// ignore damage from self
	if ( attacker != this )
	{
		if ( inflictor )
		{
			AI_SPECIAL_DAMAGE = inflictor->spawnArgs.GetInt( "special_damage" );
		}
		else
		{
			AI_SPECIAL_DAMAGE = 0;
		}

		// grayman #3372 - set up pain animation before raising the alert level

		// Switch to pain state if not in combat
		if ( /*( AI_AlertIndex == ai::ERelaxed ) && */ // grayman #3140 - go to PainState at any alert level
			 ( AI_AlertIndex < ai::ECombat ) && // grayman #3355 - pain anims mess up combat, so don't allow them
			 ( damage > 0 ) && 
			 ( ( damageDef == NULL ) || !damageDef->GetBool("no_pain_anim", "0")) &&
			 !GetMemory().painStatePushedThisFrame ) // grayman #3424 - don't push more than one pain state per frame
		{
			// grayman #3140 - note what caused the damage, in case PainState needs to do something special.
			// Start with the basic causes (arrow, melee, moveable) and expand to the others as needed.
			ai::Memory& memory = GetMemory();
			memory.causeOfPain = ai::EPC_None;
			if ( inflictor )
			{
				if ( inflictor->IsType(idProjectile::Type) )
				{
					memory.causeOfPain = ai::EPC_Projectile;
				}
				else if ( inflictor->IsType(CMeleeWeapon::Type) )
				{
					memory.causeOfPain = ai::EPC_Melee;
				}
				else if ( inflictor->IsType(idMoveable::Type) )
				{
					memory.causeOfPain = ai::EPC_Moveable;
				}
			}
			else
			{
				if ( damageDef->GetBool( "no_air" ) ) 
				{
					memory.causeOfPain = ai::EPC_Drown;
				}
			}
			GetMind()->PushState(ai::StatePtr(new ai::PainState));
			memory.painStatePushedThisFrame = true;
		}
		
		if ( !attacker->fl.notarget ) // grayman #3356
		{
			// AI don't like being attacked
			ChangeEntityRelation(attacker, -10);

			// grayman #3355 - set attacker as enemy?
			if ( attacker->IsType(idActor::Type) )
			{
				idActor* currentEnemy = GetEnemy();
				bool setNewEnemy = false;

				if ( currentEnemy == NULL )
				{
					setNewEnemy = true;
				}
				else // we have an enemy
				{
					if ( attacker != currentEnemy )
					{
						idVec3 ownerOrigin = GetPhysics()->GetOrigin();
						float dist2EnemySqr = ( currentEnemy->GetPhysics()->GetOrigin() - ownerOrigin ).LengthSqr();
						float dist2AttackerSqr = ( attacker->GetPhysics()->GetOrigin() - ownerOrigin ).LengthSqr();
						if ( dist2AttackerSqr < dist2EnemySqr )
						{
							setNewEnemy = true;
						}
					}
				}

				if (setNewEnemy)
				{
					SetEnemy(static_cast<idActor*>(attacker));
					AI_VISALERT = false;
				
					SetAlertLevel(thresh_5 - 0.1); // grayman #3857 - reduce alert level to just under Combat
					//SetAlertLevel(thresh_5*2);
					GetMemory().alertClass = ai::EAlertNone;
					GetMemory().alertType = ai::EAlertTypeEnemy;
				}
			}
		}
	}

	return ( AI_PAIN != 0 );
}


/*
=====================
idAI::SpawnParticles
=====================
*/
void idAI::SpawnParticles( const char *keyName ) {
	const idKeyValue *kv = spawnArgs.MatchPrefix( keyName, NULL );
	while ( kv ) {
		particleEmitter_t pe;

		idStr particleName = kv->GetValue();

		if ( particleName.Length() ) {

			idStr jointName = kv->GetValue();
			int dash = jointName.Find('-');
			if ( dash > 0 ) {
				particleName = particleName.Left( dash );
				jointName = jointName.Right( jointName.Length() - dash - 1 );
			}

			SpawnParticlesOnJoint( pe, particleName, jointName );
			particles.Append( pe );
		}

		kv = spawnArgs.MatchPrefix( keyName, kv );
	}
}

/*
=====================
idAI::SpawnParticlesOnJoint
=====================
*/
const idDeclParticle *idAI::SpawnParticlesOnJoint( particleEmitter_t &pe, const char *particleName, const char *jointName ) {
	idVec3 origin;
	idMat3 axis;

	if ( *particleName == '\0' ) {
		memset( &pe, 0, sizeof( pe ) );
		return pe.particle;
	}

	pe.joint = animator.GetJointHandle( jointName );
	if ( pe.joint == INVALID_JOINT ) {
		gameLocal.Warning( "Unknown particleJoint '%s' on '%s'", jointName, name.c_str() );
		pe.time = 0;
		pe.particle = NULL;
	} else {
		animator.GetJointTransform( pe.joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;

		BecomeActive( TH_UPDATEPARTICLES );
		if ( !gameLocal.time ) {
			// particles with time of 0 don't show, so set the time differently on the first frame
			pe.time = 1;
		} else {
			pe.time = gameLocal.time;
		}
		pe.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particleName ) );
		gameLocal.smokeParticles->EmitSmoke( pe.particle, pe.time, gameLocal.random.CRandomFloat(), origin, axis );
	}

	return pe.particle;
}

/*
=====================
idAI::Killed
=====================
*/
void idAI::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location )
{
	idAngles ang;
	const char *modelDeath;
	bool bPlayerResponsible(false);

	// make sure the monster is activated
	EndAttack();

	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s'\n", animator.GetJointName( ( jointHandle_t )location ),
			GetDamageGroup( location ) );
	}

	if ( inflictor ) {
		AI_SPECIAL_DAMAGE = inflictor->spawnArgs.GetInt( "special_damage" );
	} else {
		AI_SPECIAL_DAMAGE = 0;
	}

	if ( AI_DEAD ) {
		AI_PAIN = true;
		AI_DAMAGE = true;
		return;
	}

	// stop all voice sounds
	StopSound( SND_CHANNEL_VOICE, false );
	if ( head.GetEntity() ) {
		head.GetEntity()->StopSound( SND_CHANNEL_VOICE, false );
		head.GetEntity()->GetAnimator()->ClearAllAnims( gameLocal.time, 100 );
	}

	disableGravity = false;
	move.moveType = MOVETYPE_DEAD;
	m_bAFPushMoveables = false;

	physicsObj.UseFlyMove( false );
	physicsObj.ForceDeltaMove( false );

	// end our looping ambient sound
	StopSound( SND_CHANNEL_AMBIENT, false );

	// activate targets
	ActivateTargets( attacker );

	RemoveAttachments();
	RemoveProjectile();
	StopMove( MOVE_STATUS_DONE );
	GetMemory().StopReacting(); // grayman #3559

	// grayman #3857 - leave an ongoing search
	if (m_searchID > 0)
	{
		gameLocal.m_searchManager->LeaveSearch(m_searchID, this);
	}

	ClearEnemy();
	AI_DEAD	= true;

	// grayman #3317 - mark time of death and allow immediate visual stimming from this dead AI so
	// that nearby AI can react quickly, instead of waiting for the next-scheduled
	// visual stim from this AI
	m_timeFellDown = gameLocal.time;
	gameLocal.AllowImmediateStim( this, ST_VISUAL );

	// make monster nonsolid
	if (spawnArgs.GetBool("nonsolid_on_ragdoll", "1"))
	{
		physicsObj.SetContents(0);
		physicsObj.GetClipModel()->Unlink();
	}

	Unbind();

	if (!AI_KNOCKEDOUT) // grayman #3699 - no death sound if you were KO'ed
	{
		idStr deathSound = MouthIsUnderwater() ? "snd_death_liquid" : "snd_death";
		StartSound( deathSound.c_str(), SND_CHANNEL_VOICE, 0, false, NULL );
	}

	// Go to ragdoll mode immediately, if we don't have a death anim
	// If death anims are enabled, we need to wait with going to ragdoll until PostKilled()
	if (!spawnArgs.GetBool("enable_death_anim", "0"))
	{
		StartRagdoll();
	}

	// swaps the head CM back if a different one was swapped in while conscious
	SwapHeadAFCM( false );

	if ( spawnArgs.GetString( "model_death", "", &modelDeath ) ) {
		//StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL ); // grayman #4412
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		SetModel( modelDeath );
		physicsObj.SetLinearVelocity( vec3_zero );
		physicsObj.PutToRest();
		physicsObj.DisableImpact();
	}

	if (spawnArgs.GetBool("set_frobable_on_death", "1"))
	{
		// AI becomes frobable on death
		// greebo: Add a delay before the AI becomes actually frobable
		PostEventMS(&EV_SetFrobable, 750, 1);
	}
	
	restartParticles = false;

	mind->ClearStates();
	mind->PushState(STATE_DEAD);

	// drop items
	DropOnRagdoll();

	if ( attacker && (attacker == gameLocal.GetLocalPlayer()) && inflictor )
	{
		bPlayerResponsible = true;
	}
	else if ( attacker && attacker->m_SetInMotionByActor.GetEntity() )
	{
		bPlayerResponsible = ( ( attacker != gameLocal.world ) &&
			( attacker->m_SetInMotionByActor.GetEntity() == gameLocal.GetLocalPlayer() ) );
	}

	// grayman #2908 - we need to know if the AI stepped on a player-tossed mine.
	// So that we don't interfere with existing situations, check specifically
	// for the mine. The previous two checked to determine player responsibility
	// say the player is NOT responsible, when the inflictor is a mine.

	if ( !bPlayerResponsible )
	{
		if ( inflictor && inflictor->IsType(idProjectile::Type) )
		{
			idProjectile* proj = static_cast<idProjectile*>(inflictor);
			if ( proj->IsMine() )
			{
				bPlayerResponsible = ( inflictor->m_SetInMotionByActor.GetEntity() == gameLocal.GetLocalPlayer() );
			}
		}
	}

	GetMemory().playerResponsible = bPlayerResponsible; // grayman #3679
	
	// Update TDM objective system
	gameLocal.m_MissionData->MissionEvent( COMP_KILL, this, attacker, bPlayerResponsible );

	// Mark the body as last moved by the player
	if ( bPlayerResponsible )
	{
		m_SetInMotionByActor = gameLocal.GetLocalPlayer(); // grayman #3394
		m_MovedByActor = gameLocal.GetLocalPlayer();
		m_deckedByPlayer = true; // grayman #3314
	}

	// greebo: Set the lipsync flag to FALSE, we're dead
	m_lipSyncActive = false;

	DropBlood(inflictor);

	// grayman #3559 - clear list of doused lights this AI has seen; no longer needed
	m_dousedLightsSeen.Clear();

	// grayman #3848 - note who killed you
	m_killedBy = attacker;

	alertQueue.Clear(); // grayman #4002

	// grayman - print data re: being Killed
	//gameLocal.Printf("'%s' killed at [%s], Player %s responsible, inflictor = '%s', attacker = '%s'\n", GetName(),GetPhysics()->GetOrigin().ToString(),bPlayerResponsible ? "is" : "isn't",inflictor ? inflictor->GetName():"NULL",attacker ? attacker->GetName():"NULL");
}

void idAI::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
	const char *damageDefName, const float damageScale, const int location,
	trace_t *collision)
{
	// Save the current health, to see afterwards how much damage we've been taking
	int preHitHealth = health;

	idActor::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, collision);

	// grayman #2478 - allowed idActor::Damage if dead, for physics reasons.
	// Nothing beyond here is needed for AI that were dead when idAI::Damage()
	// was called.

	if (preHitHealth <= 0)
	{
		return;
	}

	if ( ( inflictor != NULL ) && inflictor->IsType(idProjectile::Type))
	{
		int damageTaken = preHitHealth - health;

		// Send a signal to the current state that we've been hit by something
		GetMind()->GetState()->OnProjectileHit(static_cast<idProjectile*>(inflictor), attacker, damageTaken);
	}

	if ( ( attacker != NULL ) && IsEnemy(attacker) && CanSee(attacker,true) ) // grayman #3857 - only conclude enemy if you can see him
	{
		GetMemory().hasBeenAttackedByEnemy = true;

		if (cv_ai_debug_transition_barks.GetBool())
		{
			gameLocal.Printf("%d: %s is damaged by an enemy, will use Alert Idle\n",gameLocal.time,GetName());
		}
	}
}

void idAI::DropBlood(idEntity *inflictor)
{
	if ( inflictor && spawnArgs.GetBool("bleed","0") ) // grayman #2931
	{
		idStr damageDefName = inflictor->spawnArgs.RandomPrefix("def_damage", gameLocal.random);

		const idDeclEntityDef *def = gameLocal.FindEntityDef(damageDefName, false);
		if ( def == NULL ) 
		{
			return;
		}

		// blood splats are thrown onto nearby surfaces
		idStr splat = def->dict.RandomPrefix("mtr_killed_splat", gameLocal.random);

		if (!splat.IsEmpty()) 
		{
			SpawnBloodMarker(splat, splat + "_fading", 40);
		}
	}
}


void idAI::SpawnBloodMarker(const idStr& splat, const idStr& splatFading, float size)
{
	trace_t result;
	gameLocal.clip.TracePoint(result, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + 100 * idVec3(0, 0, -1), MASK_OPAQUE, this); // grayman #3386 - was looking down 60

	idVec3 markerOrigin = result.endpos + idVec3(0, 0, 2);

	const idDict* markerDef = gameLocal.FindEntityDefDict("atdm:blood_marker", false);

	if ( markerDef == NULL )
	{
		gameLocal.Error( "Failed to find definition of blood marker entity" );
		return;
	}

	idEntity* ent;
	gameLocal.SpawnEntityDef(*markerDef, &ent, false);

	if ( !ent || !ent->IsType(CBloodMarker::Type) ) 
	{
		gameLocal.Error( "Failed to spawn blood marker entity" );
		return;
	}

	CBloodMarker* bloodMarker = static_cast<CBloodMarker*>(ent);
	bloodMarker->SetOrigin(markerOrigin);
	bloodMarker->Init(splat, splatFading, size, this); // grayman #3075 - pass the AI who spilled the blood
	bloodMarker->Event_GenerateBloodSplat();
}


/*
=====================
idAI::PostDeath
=====================
*/
void idAI::PostDeath()
{
	// For death anims, we need to wait with going to ragdoll until here
	if (spawnArgs.GetBool("enable_death_anim", "0"))
	{
		// Start going to ragdoll here, instead of in Killed() to enable death anims
		StartRagdoll();
	}

	headAnim.StopAnim(1);
	legsAnim.StopAnim(1);
	torsoAnim.StopAnim(1);

	headAnim.Disable();
	legsAnim.Disable();
	torsoAnim.Disable();
}

/***********************************************************************

	Targeting/Combat

***********************************************************************/

/*
=====================
idAI::PlayCinematic
=====================
*/
void idAI::PlayCinematic( void ) {
	const char *animname;

	if ( current_cinematic >= num_cinematics ) {
		if ( g_debugCinematic.GetBool() ) {
			gameLocal.Printf( "%d: '%s' stop\n", gameLocal.framenum, GetName() );
		}
		if ( !spawnArgs.GetBool( "cinematic_no_hide" ) ) {
			Hide();
		}
		current_cinematic = 0;
		ActivateTargets( gameLocal.GetLocalPlayer() );
		fl.neverDormant = false;

		return;
	}

	Show();
	current_cinematic++;

	allowJointMod = false;
	allowEyeFocus = false;

	spawnArgs.GetString( va( "anim%d", current_cinematic ), NULL, &animname );
	if ( !animname ) {
		gameLocal.Warning( "missing 'anim%d' key on %s", current_cinematic, name.c_str() );
		return;
	}

	if ( g_debugCinematic.GetBool() ) {
		gameLocal.Printf( "%d: '%s' start '%s'\n", gameLocal.framenum, GetName(), animname );
	}

	headAnim.animBlendFrames = 0;
	headAnim.lastAnimBlendFrames = 0;
	headAnim.BecomeIdle();

	legsAnim.animBlendFrames = 0;
	legsAnim.lastAnimBlendFrames = 0;
	legsAnim.BecomeIdle();

	torsoAnim.animBlendFrames = 0;
	torsoAnim.lastAnimBlendFrames = 0;
	ProcessEvent( &AI_PlayAnim, ANIMCHANNEL_TORSO, animname );

	// make sure our model gets updated
	animator.ForceUpdate();

	// update the anim bounds
	UpdateAnimation();
	UpdateVisuals();
	Present();

	if ( head.GetEntity() ) {
		// since the body anim was updated, we need to run physics to update the position of the head
		RunPhysics();

		// make sure our model gets updated
		head.GetEntity()->GetAnimator()->ForceUpdate();

		// update the anim bounds
		head.GetEntity()->UpdateAnimation();
		head.GetEntity()->UpdateVisuals();
		head.GetEntity()->Present();
	}


	fl.neverDormant = true;

}

/*
=====================
idAI::Activate

Notifies the script that a monster has been activated by a trigger or flashlight

DarkMod: Commented out calls to SetEnemy.  We don't want the AI calling setEnemy,
only the alert state scripts.
=====================
*/
void idAI::Activate( idEntity *activator )
{
	// Fire the TRIGGER response
	TriggerResponse(activator, ST_TRIGGER);

	if ( AI_DEAD || AI_KNOCKEDOUT ) {
		// ignore it when they're dead or KO'd
		return;
	}

	// make sure he's not dormant
	dormantStart = 0;

	if ( num_cinematics ) {
		PlayCinematic();
	} else {
		AI_ACTIVATED = true;

		/*idPlayer *player;
		if ( !activator || !activator->IsType( idPlayer::Type ) ) {
			player = gameLocal.GetLocalPlayer();
		} else {
			player = static_cast<idPlayer *>( activator );
		}

		if ( ReactionTo( player ) & ATTACK_ON_ACTIVATE )
		{
			//SetEnemy( player );
		}*/

		// update the script in cinematics so that entities don't start anims or show themselves a frame late.
		if ( cinematic ) {
            UpdateScript();

			// make sure our model gets updated
			animator.ForceUpdate();

			// update the anim bounds
			UpdateAnimation();
			UpdateVisuals();
			Present();

			if ( head.GetEntity() ) {
				// since the body anim was updated, we need to run physics to update the position of the head
				RunPhysics();

				// make sure our model gets updated
				head.GetEntity()->GetAnimator()->ForceUpdate();

				// update the anim bounds
				head.GetEntity()->UpdateAnimation();
				head.GetEntity()->UpdateVisuals();
				head.GetEntity()->Present();
			}
		}
	}
}

/*
=====================
idAI::EnemyDead
=====================
*/
void idAI::EnemyDead( void ) {
	ClearEnemy();
}

/*
=====================
idAI::TalkTo
=====================
*/
void idAI::TalkTo( idActor *actor ) {
	if ( talk_state != TALK_OK ) {
		return;
	}

	talkTarget = actor;
	if ( actor ) {
		AI_TALK = true;
	} else {
		AI_TALK = false;
	}
}

/*
=====================
idAI::GetEnemy
=====================
*/
idActor	*idAI::GetEnemy( void ) const {
	return enemy.GetEntity();
}

/*
=====================
idAI::GetTalkState
=====================
*/
talkState_t idAI::GetTalkState( void ) const {
	if ( ( talk_state != TALK_NEVER ) && AI_DEAD ) {
		return TALK_DEAD;
	}
	if ( IsHidden() ) {
		return TALK_NEVER;
	}
	return talk_state;
}

/*
=====================
idAI::TouchedByFlashlight
=====================
*/
void idAI::TouchedByFlashlight( idActor *flashlight_owner ) {
	if ( wakeOnFlashlight ) {
		Activate( flashlight_owner );
	}
}

/*
=====================
idAI::ClearEnemy
=====================
*/
void idAI::ClearEnemy( void ) {
	if ( move.moveCommand == MOVE_TO_ENEMY ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
	}

	// grayman #2887 - log the amount of time you saw the player.
	// This is normally taken care of in UpdateEnemyPosition(), but we need
	// to check here in case the AI is killed or KO'ed and doesn't
	// use the normal logging code there.

	if ( AI_ENEMY_VISIBLE || AI_ENEMY_TACTILE )
	{
		if ( ( enemy.GetEntity() != NULL ) && ( enemy.GetEntity()->IsType(idPlayer::Type) ) )
		{
			if ( lastTimePlayerLost < 0 ) // = -1 if the player was never seen or seen but not lost at this point
			{
				lastTimePlayerLost = gameLocal.time;
				if ( lastTimePlayerSeen > 0 )
				{
					gameLocal.m_MissionData->Add2TimePlayerSeen(lastTimePlayerLost - lastTimePlayerSeen);
				}
				lastTimePlayerSeen = -1;
			}
		}
	}

	enemyNode.Remove();
	enemy				= NULL;
	AI_ENEMY_IN_FOV		= false;
	AI_ENEMY_VISIBLE	= false;
	AI_ENEMY_TACTILE	= false;
}

/*
=====================
idAI::EnemyPositionValid
=====================
*/
bool idAI::EnemyPositionValid( void ) const {
	trace_t	tr;

	if ( !enemy.GetEntity() ) {
		return false;
	}

	if ( AI_ENEMY_VISIBLE || AI_ENEMY_TACTILE ) {
		return true;
	}

	gameLocal.clip.TracePoint( tr, GetEyePosition(), lastVisibleEnemyPos + lastVisibleEnemyEyeOffset, MASK_OPAQUE, this );
	if ( tr.fraction < 1.0f ) {
		// can't see the area yet, so don't know if he's there or not
		return true;
	}

	return false;
}

/*
=====================
idAI::SetEnemyPosition
=====================
*/
void idAI::SetEnemyPosition()
{
	idActor* enemyEnt = enemy.GetEntity();

	if (enemyEnt == NULL)
	{
		return;
	}

	int			enemyAreaNum;
	int			areaNum;
	int			lastVisibleReachableEnemyAreaNum = -1;
	aasPath_t	path;
	idVec3		pos;
	bool		onGround;

	lastVisibleReachableEnemyPos = lastReachableEnemyPos;
	lastVisibleEnemyEyeOffset = enemyEnt->EyeOffset();
	lastVisibleEnemyPos = enemyEnt->GetPhysics()->GetOrigin();

	if (move.moveType == MOVETYPE_FLY)
	{
		// Flying AI assume the enemy to be always on ground
		pos = lastVisibleEnemyPos;
		onGround = true;
	} 
	else
	{
		// For non-flying AI, check if the enemy is on the ground
		onGround = enemyEnt->GetFloorPos(64.0f, pos);

		if (enemyEnt->OnLadder()) // greebo: TODO: What about ropes?
		{
			onGround = false;
		}
	}

	// greebo: "pos" now holds the enemy position used for pathing (floor pos or visible pos)

	if (!onGround)
	{
		// greebo: The AI considers a non-grounded entity to be unreachable
		// TODO: Check for climb height? The enemy might still be reachable?
		if (move.moveCommand == MOVE_TO_ENEMY)
		{
			AI_DEST_UNREACHABLE = true;
		}
		return;
	}

	if (aas != NULL)
	{
		// We have a valid AAS attached, try to reach the enemy

		// The default reachable area number is taken from the move command
		lastVisibleReachableEnemyAreaNum = move.toAreaNum;

		// Get the area number of the point the enemy has last been seen in (== enemy origin)
		enemyAreaNum = PointReachableAreaNum(lastVisibleEnemyPos, 1.0f);

		// If the area number is invalid, try to look it up again using the REACHABLE position
		if (!enemyAreaNum)
		{
			enemyAreaNum = PointReachableAreaNum(lastReachableEnemyPos, 1.0f);
			pos = lastReachableEnemyPos;
		}
		
		// Do we have a valid area number now?
		if (enemyAreaNum)
		{
			// We have a valid enemy area number

			// Get own origin and area number
			const idVec3 &org = physicsObj.GetOrigin();
			areaNum = PointReachableAreaNum(org);

			// Try to set up a walk/fly path to the enemy
			if (PathToGoal(path, areaNum, org, enemyAreaNum, pos, this))
			{
				// Succeeded, we have a visible and reachable enemy position
				lastVisibleReachableEnemyPos = pos;
				lastVisibleReachableEnemyAreaNum = enemyAreaNum;

				if (move.moveCommand == MOVE_TO_ENEMY)
				{
					// Enemy is reachable
					AI_DEST_UNREACHABLE = false;
				}
			} 
			else if (move.moveCommand == MOVE_TO_ENEMY)
			{
				// No path to goal available, enemy is unreachable
				AI_DEST_UNREACHABLE = true;
			}
		}
		else
		{
			// The area number lookup failed, fallback to unreachable
			if (move.moveCommand == MOVE_TO_ENEMY)
			{
				AI_DEST_UNREACHABLE = true;
			}
			areaNum = 0;
		}
	}
	else
	{
		// We don't have a valid AAS, we can't tell if an enemy is reachable or not,
		// so just assume that he is.
		lastVisibleReachableEnemyPos = lastVisibleEnemyPos;
		if (move.moveCommand == MOVE_TO_ENEMY)
		{
			AI_DEST_UNREACHABLE = false;
		}
		enemyAreaNum = 0;
		areaNum = 0;
	}

	// General move command update
	if (move.moveCommand == MOVE_TO_ENEMY)
	{
		if (!aas)
		{
			// Invalid AAS keep the move destination up to date for wandering
			move.moveDest = lastVisibleReachableEnemyPos;
		}
		else if (enemyAreaNum)
		{
			// The previous pathing attempt succeeded, update the move command
			move.toAreaNum = lastVisibleReachableEnemyAreaNum;
			move.moveDest = lastVisibleReachableEnemyPos;
		}

		if (move.moveType == MOVETYPE_FLY)
		{
			predictedPath_t path;
			idVec3 end = move.moveDest;
			end.z += enemyEnt->EyeOffset().z + fly_offset;
			idAI::PredictPath( this, aas, move.moveDest, end - move.moveDest, 1000, 1000, SE_BLOCKED, path );
			move.moveDest = path.endPos;
			move.toAreaNum = PointReachableAreaNum( move.moveDest, 1.0f );
		}
	}
}

bool idAI::EntityInAttackCone(idEntity* ent)
{
	float	attack_cone;
	idVec3	delta;
	float	yaw;
	float	relYaw;
	
	if ( !ent ) {
		return false;
	}

	delta = ent->GetPhysics()->GetOrigin() - GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	yaw = delta.ToYaw();

	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	relYaw = idMath::AngleNormalize180( ideal_yaw - yaw );

	return ( idMath::Fabs( relYaw ) < ( attack_cone * 0.5f ) );
}

bool idAI::CanHitEntity(idActor* entity, ECombatType combatType)
{
	if ( (entity == NULL) || entity->IsKnockedOut() || (entity->health <= 0) )
	{
		return false;
	}

	if (combatType == COMBAT_MELEE)
	{
		return TestMelee();
	}

	if (combatType == COMBAT_RANGED)
	{
		return TestRanged();
	}

	// COMBAT_NONE

	// grayman #3507 - allow for having both ranged and melee weapons

	if (GetNumRangedWeapons() > 0)
	{
		bool result = TestRanged();
		if (result)
		{
			return true;
		}
	}

	if (GetNumMeleeWeapons() > 0)
	{
		return TestMelee();
	}

	return false;
}

bool idAI::WillBeAbleToHitEntity(idActor* entity, ECombatType combatType)
{
	if ( (entity == NULL) || entity->IsKnockedOut() || (entity->health <= 0) )
	{
		return false;
	}

	if (combatType == COMBAT_MELEE)
	{
		return TestMeleeFuture();
	}

	if (combatType == COMBAT_RANGED)
	{
		// not supported for ranged combat
		return false;
	}

	// COMBAT_NONE

	// grayman #3507 - allow for having both ranged and melee weapons

	if (GetNumMeleeWeapons() > 0)
	{
		return TestMeleeFuture();
	}

	return false;
}

bool idAI::CanBeHitByEntity(idActor* entity, ECombatType combatType)
{
	if (entity == NULL || entity->IsKnockedOut() || entity->health <= 0 ) 
		return false;

	if (combatType == COMBAT_MELEE)
	{
		if ( !entity->GetAttackFlag(COMBAT_MELEE) 
				|| !entity->melee_range )
			return false;

		const idVec3& org = physicsObj.GetOrigin();
		const idBounds& bounds = physicsObj.GetBounds();

		const idVec3& enemyOrg = entity->GetPhysics()->GetOrigin();
		const idBounds& enemyBounds = entity->GetPhysics()->GetBounds();

		idVec3 ourVel = physicsObj.GetLinearVelocity();
		idVec3 enemyVel = entity->GetPhysics()->GetLinearVelocity();
		idVec3 relVel = enemyVel - ourVel;
		float velDot = ourVel * enemyVel;

		idVec3 dir = enemyOrg - org;
		dir.z = 0;
		float dist = dir.LengthFast();

		float enemyReach = entity->melee_range;

		// generic factor to increase the enemy's threat range (accounts for sudden, quick advances)
		// (TODO: Make spawnarg?)
		float threatFactor = 1.5f; 
		// velocity-based factor to increase the enemy's threat range
		float velFactor;
		if( velDot > 0 )
		{
			// enemy moving away
			velFactor = 1.0f;
		}
		else
		{
			// TODO: Make spawnargs?
			// max speed of 20 MPH
			float maxVel = 352;
			float maxThreatInc = 3.0;

			velFactor = idMath::ClampFloat(1.0f, maxThreatInc, 1.0f + (maxThreatInc-1.0f)*relVel.LengthFast()/maxVel);
		}

		enemyReach *= threatFactor * velFactor;

		float maxdist = enemyReach + bounds[1][0];

		if (dist < maxdist)
		{
			// within horizontal distance
			if (((enemyOrg.z + enemyBounds[1].z + entity->melee_range_vert) > org.z) && ((org.z + bounds[1].z) > enemyOrg.z)) // grayman #2655 - use enemy's origin, not your own, and use melee_range_vert
//			if (((org.z + enemyBounds[1][2] + entity->melee_range) > org.z) && (org.z + bounds[1][2]) > enemyOrg.z) // grayman #2655 - old way
			{
				// within height
				// don't bother with trace for this test
				return true;
			}
		}
		
		return false;
	}
	else if (combatType == COMBAT_RANGED)
	{
		return false; // NYI
	}
	else
		return false;
}

void idAI::UpdateAttachmentContents(bool makeSolid)
{
	// ishtvan: Modified to work only with AF body representing the entity
	for (int i = 0; i < m_Attachments.Num(); i++)
	{
		idEntity* ent = m_Attachments[i].ent.GetEntity();

		if (ent == NULL || !m_Attachments[i].ent.IsValid()) continue;

		if (!ent->IsType(CMeleeWeapon::Type) && !ent->m_bAttachedAlertControlsSolidity) 
			continue;
		
		// Found a melee weapon or other attachment that should become solid on alert
		idAFBody *body;
		if (makeSolid)
		{
			if( (body = AFBodyForEnt(ent)) != NULL )
			{
				// TODO: Read the contents from a stored variable
				// in case they are something other than these standards?
				// CONTENTS_RENDERMODEL allows projectiles to hit it
				body->GetClipModel()->SetContents( CONTENTS_CORPSE | CONTENTS_RENDERMODEL );
				body->SetClipMask( CONTENTS_SOLID | CONTENTS_CORPSE );
			}
		}
		else
		{
			if( (body = AFBodyForEnt(ent)) != NULL )
			{
				body->GetClipModel()->SetContents( 0 );
				body->SetClipMask( 0 );
			}
		}
	}
}

/*
=====================
idAI::UpdateEnemyPosition
=====================
*/
void idAI::UpdateEnemyPosition()
{
	// Interleave this check
	if (gameLocal.time <= lastUpdateEnemyPositionTime + cv_ai_opt_update_enemypos_interleave.GetInteger())
	{
		return;
	}

	idActor* enemyEnt = enemy.GetEntity();
	enemyReachable = false;

	if (enemyEnt == NULL)
	{
		return;
	}

	// Set a new time stamp
	lastUpdateEnemyPositionTime = gameLocal.time;

	START_SCOPED_TIMING(aiUpdateEnemyPositionTimer, scopedUpdateEnemyPositionTimer)

	int				enemyAreaNum(-1);
	idVec3			enemyPos;
	bool			onGround;
	const idVec3&	org = physicsObj.GetOrigin();
	int				lastVisibleReachableEnemyAreaNum = -1;
	bool			enemyDetectable = false;

	if ( move.moveType == MOVETYPE_FLY )
	{
		enemyPos = enemyEnt->GetPhysics()->GetOrigin();
		// greebo: Flying AI always consider their enemies to be on the ground
		onGround = true;
	}
	else
	{
		// non-flying AI, get the floor position of the enemy
		START_TIMING(aiGetFloorPosTimer);
		onGround = enemyEnt->GetFloorPos(64.0f, enemyPos);
		STOP_TIMING(aiGetFloorPosTimer);

		if (enemyEnt->OnLadder())
		{
			onGround = false;
		}
	}

	int areaNum = PointReachableAreaNum(org, 1.0f);

	if (onGround)
	{
		// greebo: Enemy is on ground, hence reachable, try to setup a path
		if (aas != NULL)
		{
			// We have a valid AAS, try to get the area of the enemy (floorpos or origin)
			enemyAreaNum = PointReachableAreaNum(enemyPos, 1.0f);

			if (enemyAreaNum)
			{
				// We have a valid enemy area number
				// Get the own area number

				// Try to setup a path to the goal
				aasPath_t path;
				if (PathToGoal( path, areaNum, org, enemyAreaNum, enemyPos, this))
				{
					// Path successfully setup, store the position as "reachable"
					lastReachableEnemyPos = enemyPos;
					enemyReachable = true;
				}
			}
			else
			{
				// The area number lookup failed, fallback to unreachable
				if (move.moveCommand == MOVE_TO_ENEMY)
				{
					AI_DEST_UNREACHABLE = true;
				}
				areaNum = 0;
			}
		}
		else
		{
			// We don't have an AAS, we can't tell if an enemy is reachable or not,
			// so just assume that he is.
			enemyAreaNum = 0;
			lastReachableEnemyPos = enemyPos;
			enemyReachable = true;
		}
	}
	
	AI_ENEMY_IN_FOV		= false;
	AI_ENEMY_VISIBLE	= false;
	AI_ENEMY_TACTILE	= false;
	
	if (CanSee(enemyEnt, false))
	{
		if (cv_ai_show_enemy_visibility.GetBool())
		{
			// greebo: A trace to the enemy is possible (no FOV check!) and the entity is not hidden in darkness
			gameRenderWorld->DebugArrow(colorGreen, GetEyePosition(), GetEyePosition() + idVec3(0,0,10), 2, 100);
		}

		// Enemy is considered visible if not hidden in darkness and not obscured
		AI_ENEMY_VISIBLE = true;

		// Now perform the FOV check manually
		if (CheckFOV(enemyPos))
		{
			AI_ENEMY_IN_FOV = true;
			// TODO: call SetEnemyPosition here only?

			// The AI won't actually "see" the player until after the Combat pause
			// that occurs at the start of Combat mode.

			// Store the last time the enemy was visible
			mind->GetMemory().lastTimeEnemySeen = gameLocal.time;
		}
		enemyDetectable = true;
	}
	else // enemy is not visible
	{
		// Enemy can't be seen (obscured or hidden in darkness)
		if (cv_ai_show_enemy_visibility.GetBool())
		{
			gameRenderWorld->DebugArrow(colorRed, GetEyePosition(), GetEyePosition() + idVec3(0,0,10), 2, 100);
		}
	}

	// is the enemy in tactile range? dragofer #6186
	idEntity* tactEnt = GetTactEnt();
	if( tactEnt != NULL && tactEnt == enemyEnt )
	{
		AI_ENEMY_TACTILE = true;
	}

	// grayman #2887 - track enemy visibility for statistics
	if ( enemyEnt->IsType(idPlayer::Type) )
	{
		if ( AI_ENEMY_VISIBLE || AI_ENEMY_TACTILE )	//player is visible or in tactile range
		{
			if ( ( lastTimePlayerSeen < 0 ) && ( lastTimePlayerLost < 0 ) )
			{
				// new sighting
				gameLocal.m_MissionData->IncrementPlayerSeen();
				lastTimePlayerSeen = gameLocal.time;
			}
			else if ( lastTimePlayerLost > 0 )
			{
				if ( ( gameLocal.time - lastTimePlayerLost ) > 6000 )
				{
					// new sighting
					gameLocal.m_MissionData->IncrementPlayerSeen();
					lastTimePlayerSeen = gameLocal.time;
					lastTimePlayerLost = -1;
				}
				else // continuation of previous sighting
				{
					lastTimePlayerSeen = lastTimePlayerLost;
					lastTimePlayerLost = -1;
				}
			}
		}
		else // neither visible nor tactile
		{
			// log the amount of time you saw the player

			if ( lastTimePlayerLost < 0 )
			{
				lastTimePlayerLost = gameLocal.time;
				if ( lastTimePlayerSeen > 0 )
				{
					gameLocal.m_MissionData->Add2TimePlayerSeen(lastTimePlayerLost - lastTimePlayerSeen);
				}
				lastTimePlayerSeen = -1;
			}
		}
	}

	if (enemyDetectable)
	{
		// angua: This was merged in from SetEnemyPosition
		// to avoid doing the same reachability testing stuff twice
		lastVisibleEnemyPos = enemyEnt->GetPhysics()->GetOrigin();
		lastVisibleEnemyEyeOffset = enemyEnt->EyeOffset();

		if (aas != NULL && enemyReachable)
		{
			// We have a visible and reachable enemy position
			lastVisibleReachableEnemyPos = enemyPos;
			lastVisibleReachableEnemyAreaNum = enemyAreaNum;
		}
		else
		{
			// We don't have a valid AAS, we can't tell if an enemy is reachable or not,
			// so just assume that he is.
			lastVisibleReachableEnemyPos = lastVisibleEnemyPos;
			enemyAreaNum = 0;
			areaNum = 0;
		}

		// General move command update
		if (move.moveCommand == MOVE_TO_ENEMY)
		{
			if (!aas)
			{
				// Invalid AAS keep the move destination up to date for wandering
				AI_DEST_UNREACHABLE = false;
				move.moveDest = lastVisibleReachableEnemyPos;
			}
			else if (enemyAreaNum)
			{
				// The previous pathing attempt succeeded, update the move command
				move.toAreaNum = lastVisibleReachableEnemyAreaNum;
				move.moveDest = lastVisibleReachableEnemyPos;
				AI_DEST_UNREACHABLE = !enemyReachable;
			}

			if (move.moveType == MOVETYPE_FLY)
			{
				predictedPath_t path;
				idVec3 end = move.moveDest;
				end.z += enemyEnt->EyeOffset().z + fly_offset;
				idAI::PredictPath( this, aas, move.moveDest, end - move.moveDest, 1000, 1000, SE_BLOCKED, path );
				move.moveDest = path.endPos;
				move.toAreaNum = PointReachableAreaNum( move.moveDest, 1.0f );
			}

			if (!onGround)
			{
				// greebo: The AI considers a non-grounded entity to be unreachable
				// TODO: Check for climb height? The enemy might still be reachable?
				AI_DEST_UNREACHABLE = true;
			}
		}
	}

	if (ai_debugMove.GetBool())
	{
		gameRenderWorld->DebugBounds( colorLtGrey, enemyEnt->GetPhysics()->GetBounds(), lastReachableEnemyPos, USERCMD_MSEC );
		gameRenderWorld->DebugBounds( colorWhite, enemyEnt->GetPhysics()->GetBounds(), lastVisibleReachableEnemyPos, USERCMD_MSEC );
	}
}

/*
=====================
idAI::SetEnemy
=====================
*/
bool idAI::SetEnemy(idActor* newEnemy)
{
	// Don't continue if we're dead or knocked out
	if (newEnemy == NULL || AI_DEAD || AI_KNOCKEDOUT)
	{
		ClearEnemy();
		return false; // not a valid enemy
	}

	// greebo: Check if the new enemy is different
	if (enemy.GetEntity() != newEnemy)
	{
		// grayman #3331 - don't reset the 'enemy' pointer if the
		// new enemy is dead

		// Check if the new enemy is dead
		if (newEnemy->health <= 0)
		{
			return false; // not a valid enemy
		}

		// Update the enemy pointer 
		enemy = newEnemy;

		enemyNode.AddToEnd(newEnemy->enemyList);

		int enemyAreaNum(-1);

		// greebo: Get the reachable position and area number of this enemy
		newEnemy->GetAASLocation(aas, lastReachableEnemyPos, enemyAreaNum);

		// SetEnemyPosition() can now try to setup a path, 
		// lastVisibleReachableEnemyPosition is set in ANY CASE by this method
		SetEnemyPosition();

		// greebo: This looks suspicious. It overwrites REACHABLE
		// and VISIBLEREACHABLE with VISIBLE enemy position,
		// regardless of what happened before. WTF? TODO
		// grayman #3507 - let's assume SetEnemyPosition() did the right thing
		//lastReachableEnemyPos = lastVisibleEnemyPos;
		//lastVisibleReachableEnemyPos = lastReachableEnemyPos;

		// Get the area number of the enemy
		enemyAreaNum = PointReachableAreaNum(lastReachableEnemyPos, 1.0f);

		if (aas != NULL && enemyAreaNum)
		{
			aas->PushPointIntoAreaNum( enemyAreaNum, lastReachableEnemyPos );
			lastVisibleReachableEnemyPos = lastReachableEnemyPos;
		}

		ai::Memory& memory = GetMemory();
		memory.lastTimeEnemySeen = gameLocal.time;
		memory.StopReacting(); // grayman #3559
	}

	return true; // a valid enemy
}

/*
============
idAI::FirstVisiblePointOnPath
============
*/
idVec3 idAI::FirstVisiblePointOnPath( const idVec3 origin, const idVec3 &target, int travelFlags ) {
	int i, areaNum, targetAreaNum, curAreaNum, travelTime;
	idVec3 curOrigin;
	idReachability *reach;

	if ( !aas ) {
		return origin;
	}

	areaNum = PointReachableAreaNum( origin );
	targetAreaNum = PointReachableAreaNum( target );

	if ( !areaNum || !targetAreaNum ) {
		return origin;
	}

	if ( ( areaNum == targetAreaNum ) || PointVisible( origin ) ) {
		return origin;
	}

	curAreaNum = areaNum;
	curOrigin = origin;

	for( i = 0; i < 10; i++ ) {

		if ( !aas->RouteToGoalArea( curAreaNum, curOrigin, targetAreaNum, travelFlags, travelTime, &reach, NULL, this ) ) {
			break;
		}

		if ( !reach ) {
			return target;
		}

		curAreaNum = reach->toAreaNum;
		curOrigin = reach->end;

		if ( PointVisible( curOrigin ) ) {
			return curOrigin;
		}
	}

	return origin;
}

/*
===================
idAI::CalculateAttackOffsets

calculate joint positions on attack frames so we can do proper "can hit" tests
===================
*/
void idAI::CalculateAttackOffsets( void ) {
	const idDeclModelDef	*modelDef;
	int						num;
	int						i;
	int						frame;
	const frameCommand_t	*command;
	idMat3					axis;
	const idAnim			*anim;
	jointHandle_t			joint;

	modelDef = animator.ModelDef();
	if ( !modelDef ) {
		return;
	}
	num = modelDef->NumAnims();

	// needs to be off while getting the offsets so that we account for the distance the monster moves in the attack anim
	animator.RemoveOriginOffset( false );

	// anim number 0 is reserved for non-existant anims.  to avoid off by one issues, just allocate an extra spot for
	// launch offsets so that anim number can be used without subtracting 1.
	missileLaunchOffset.SetGranularity( 1 );
	missileLaunchOffset.SetNum( num + 1 );
	missileLaunchOffset[ 0 ].Zero();

	for( i = 1; i <= num; i++ ) {
		missileLaunchOffset[ i ].Zero();
		anim = modelDef->GetAnim( i );
		if ( anim ) {
			frame = anim->FindFrameForFrameCommand( FC_LAUNCHMISSILE, &command );
			if ( frame >= 0 ) {
				joint = animator.GetJointHandle( command->string->c_str() );
				if ( joint == INVALID_JOINT ) {
					gameLocal.Error( "Invalid joint '%s' on 'launch_missile' frame command on frame %d of model '%s'", command->string->c_str(), frame, modelDef->GetName() );
				}
				GetJointTransformForAnim( joint, i, FRAME2MS( frame ), missileLaunchOffset[ i ], axis );
			}
		}
	}

	animator.RemoveOriginOffset( true );
}

/*
=====================
idAI::EnsureActiveProjectileInfo
=====================
*/
idAI::ProjectileInfo& idAI::EnsureActiveProjectileInfo()
{
	// Ensure valid projectile clipmodel
	if (activeProjectile.info.clipModel == NULL)
	{
		// Ensure we have a radius
		if (activeProjectile.info.radius < 0)
		{
			// At least we should have the def
			assert(activeProjectile.info.def != NULL);

			// Load values from the projectile
			InitProjectileInfoFromDict(activeProjectile.info, activeProjectile.info.def);
		}

		idBounds projectileBounds(vec3_origin);
		projectileBounds.ExpandSelf(activeProjectile.info.radius);

		activeProjectile.info.clipModel = new idClipModel(idTraceModel(projectileBounds));
	}

	return activeProjectile.info;
}

/*
=====================
idAI::GetAimDir
=====================
*/
bool idAI::GetAimDir( const idVec3 &firePos, idEntity *aimAtEnt, const idEntity *ignore, idVec3 &aimDir )
{
	idVec3	targetPos1;
	idVec3	targetPos2;
	idVec3	delta;
	float	max_height;
	bool	result;

	// if no aimAtEnt or projectile set
	if (aimAtEnt == NULL || projectileInfo.Num() == 0)
	{
		aimDir = viewAxis[ 0 ] * physicsObj.GetGravityAxis();
		return false;
	}

	// Ensure we have a valid clipmodel
	ProjectileInfo& info = EnsureActiveProjectileInfo();

	if ( aimAtEnt == enemy.GetEntity() ) {
		static_cast<idActor *>( aimAtEnt )->GetAIAimTargets( lastVisibleEnemyPos, targetPos1, targetPos2 );
	} else if ( aimAtEnt->IsType( idActor::Type ) ) {
		static_cast<idActor *>( aimAtEnt )->GetAIAimTargets( aimAtEnt->GetPhysics()->GetOrigin(), targetPos1, targetPos2 );
	} else {
		targetPos1 = aimAtEnt->GetPhysics()->GetAbsBounds().GetCenter();
		targetPos2 = targetPos1;
	}

	// try aiming for chest
	delta = firePos - targetPos1;
	max_height = delta.LengthFast() * projectile_height_to_distance_ratio;
	result = PredictTrajectory( firePos, targetPos1, info.speed, info.gravity, info.clipModel, MASK_SHOT_RENDERMODEL, max_height, ignore, aimAtEnt, ai_debugTrajectory.GetBool() ? 1000 : 0, aimDir );
	if ( result || !aimAtEnt->IsType( idActor::Type ) ) {
		return result;
	}

	// try aiming for head
	delta = firePos - targetPos2;
	max_height = delta.LengthFast() * projectile_height_to_distance_ratio;
	result = PredictTrajectory( firePos, targetPos2, info.speed, info.gravity, info.clipModel, MASK_SHOT_RENDERMODEL, max_height, ignore, aimAtEnt, ai_debugTrajectory.GetBool() ? 1000 : 0, aimDir );

	return result;
}

/*
=====================
idAI::BeginAttack
=====================
*/
void idAI::BeginAttack( const char *name ) {
	attack = name;
	lastAttackTime = gameLocal.time;
}

/*
=====================
idAI::EndAttack
=====================
*/
void idAI::EndAttack( void ) {
	attack = "";
}

/*
=====================
idAI::CreateProjectile
=====================
*/
idProjectile* idAI::CreateProjectile(const idVec3 &pos, const idVec3 &dir, int index)
{
	if (activeProjectile.projEnt.GetEntity() == NULL)
	{
		assert(curProjectileIndex >= 0 && curProjectileIndex < projectileInfo.Num()); // bounds check

		// projectile pointer still NULL, create a new one
		const idDict* def = projectileInfo[curProjectileIndex].def;

		// Fill the current projectile entity pointer, passing the arguments to the specialised routine
		// After this call, the activeProjectile.projEnt pointer will be initialised with the new projectile.
		CreateProjectileFromDict(pos, dir, def);

		// Re-roll the index for the next time
		curProjectileIndex = gameLocal.random.RandomInt(projectileInfo.Num());
	}

	return activeProjectile.projEnt.GetEntity();
}

idProjectile* idAI::CreateProjectileFromDict(const idVec3 &pos, const idVec3 &dir, const idDict* dict)
{
	if (activeProjectile.projEnt.GetEntity() == NULL)
	{
		// Store the def for later use
		activeProjectile.info.def = dict;
		activeProjectile.info.defName = dict->GetString("classname");

		// Fill the current projectile entity pointer
		activeProjectile.projEnt = SpawnProjectile(dict);
	}

	activeProjectile.projEnt.GetEntity()->Create(this, pos, dir);

	return activeProjectile.projEnt.GetEntity();
}

idProjectile* idAI::SpawnProjectile(const idDict* dict) const
{
	idEntity* ent;
	gameLocal.SpawnEntityDef(*dict, &ent, false);

	if (ent == NULL)
	{
		const char* clsname = dict->GetString("classname");
		gameLocal.Error("Could not spawn entityDef '%s'", clsname);
	}

	if (!ent->IsType(idProjectile::Type))
	{
		const char* clsname = ent->GetClassname();
		gameLocal.Error("'%s' is not an idProjectile", clsname);
	}

	return static_cast<idProjectile*>(ent);
}

/*
=====================
idAI::RemoveProjectile
=====================
*/
void idAI::RemoveProjectile()
{
	if (activeProjectile.projEnt.GetEntity())
	{
		activeProjectile.projEnt.GetEntity()->PostEventMS(&EV_Remove, 0);
		activeProjectile.projEnt = NULL;

		activeProjectile.info = ProjectileInfo();
	}
}

/*
=====================
idAI::LaunchProjectile
=====================
*/
idProjectile *idAI::LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone ) {
	idVec3				muzzle;
	idVec3				dir;
	idVec3				start;
	trace_t				tr;
	idBounds			projBounds;
	float				distance;
	const idClipModel	*projClip;
	float				attack_accuracy;
	float				attack_cone;
	float				projectile_spread;
	float				diff;
	float				angle;
	float				spin;
	idAngles			ang;
	int					num_projectiles;
	int					i;
	idMat3				axis;
	idVec3				tmp;

	if (projectileInfo.Num() == 0)
	{
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	num_projectiles = spawnArgs.GetInt( "num_projectiles", "1" );

	GetMuzzle( jointname, muzzle, axis );

	// Ensure we have a set up projectile
	CreateProjectile(muzzle, axis[0]);

	idProjectile* lastProjectile = activeProjectile.projEnt.GetEntity();

	if ( target != NULL ) {
		tmp = target->GetPhysics()->GetAbsBounds().GetCenter() - muzzle;
		tmp.Normalize();
		axis = tmp.ToMat3();
	} else {
		axis = viewAxis;
	}

	// rotate it because the cone points up by default
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = lastProjectile->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( axis );

	// check if the owner bounds is bigger than the projectile bounds
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( muzzle, viewAxis[ 0 ], distance ) ) {
			start = muzzle + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, muzzle, projClip, axis, MASK_SHOT_RENDERMODEL, this );
	muzzle = tr.endpos;

	// set aiming direction
	GetAimDir( muzzle, target, this, dir );
	ang = dir.ToAngles();

	// adjust his aim so it's not perfect.  uses sine based movement so the tracers appear less random in their spread.
	float t = MS2SEC( gameLocal.time + entityNumber * 497 );
	ang.pitch += idMath::Sin16( t * 5.1 ) * attack_accuracy;
	ang.yaw	+= idMath::Sin16( t * 6.7 ) * attack_accuracy;

	if ( clampToAttackCone ) {
		// clamp the attack direction to be within monster's attack cone so he doesn't do
		// things like throw the missile backwards if you're behind him
		diff = idMath::AngleDelta( ang.yaw, current_yaw );
		if ( diff > attack_cone ) {
			ang.yaw = current_yaw + attack_cone;
		} else if ( diff < -attack_cone ) {
			ang.yaw = current_yaw - attack_cone;
		}
	}

	axis = ang.ToMat3();

	float spreadRad = DEG2RAD( projectile_spread );

	for( i = 0; i < num_projectiles; i++ )
	{
		// spread the projectiles out
		angle = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
		spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = axis[ 0 ] + axis[ 2 ] * ( angle * idMath::Sin( spin ) ) - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir.Normalize();

		// create, launch and clear the projectile
		CreateProjectile( muzzle, dir );
		
		lastProjectile = activeProjectile.projEnt.GetEntity();
		lastProjectile->Launch( muzzle, dir, vec3_origin );

		activeProjectile.projEnt = NULL;
	}

	TriggerWeaponEffects( muzzle );

	lastAttackTime = gameLocal.time;

	return lastProjectile;
}

/*
================
idAI::SwapLODModel

Swap the AI model for LOD, preserving current current animations where possible using the idAnimatedEntity method.
AI scripts could be broken if ongoing torso and leg animations were lost, so check for suitable replacements before
making the switch.
================
*/
void idAI::SwapLODModel( const char *modelname )
{
	const idDeclModelDef *newmodel = static_cast<const idDeclModelDef *>(declManager->FindType( DECL_MODELDEF, modelname, false ));
	if ( !newmodel ) {
		gameLocal.Warning( "LOD switch failed for %s. New model %s not found", GetName(), modelname );
		return;
	}

	const char *torsoanim = animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName();
	if ( strlen(torsoanim) > 0 && !newmodel->HasAnim( torsoanim ) )
	{
		gameLocal.Warning( "LOD switch failed for %s. New model %s doesn't support torso anim %s", GetName(), modelname, torsoanim );
		return;
	}

	const char *legsanim = animator.CurrentAnim( ANIMCHANNEL_LEGS )->AnimName();
	if ( strlen( legsanim ) > 0 && !newmodel->HasAnim( legsanim ) )
	{
		gameLocal.Warning( "LOD switch failed for %s. New model %s doesn't support legs anim %s", GetName(), modelname, legsanim );
		return;
	}

	// Ok to proceed
	idAnimatedEntity::SwapLODModel( modelname );
}

/*
================
idAI::DamageFeedback

callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.

FIXME: This gets called when we call idPlayer::CalcDamagePoints from idAI::AttackMelee, which then checks for a saving throw,
possibly forcing a miss.  This is harmless behavior ATM, but is not intuitive.
================
*/
void idAI::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	if ( ( victim == this ) && inflictor->IsType( idProjectile::Type ) ) {
		// monsters only get half damage from their own projectiles
		damage = ( damage + 1 ) / 2;  // round up so we don't do 0 damage

	} else if ( victim == enemy.GetEntity() ) {
		AI_HIT_ENEMY = true;
	}
}

/*
=====================
idAI::DirectDamage

Causes direct damage to an entity

kickDir is specified in the monster's coordinate system, and gives the direction
that the view kick and knockback should go
=====================
*/
void idAI::DirectDamage( const char *meleeDefName, idEntity *ent ) {
	const idDict *meleeDef;
	const char *p;
	const idSoundShader *shader;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown damage def '%s' on '%s'", meleeDefName, name.c_str() );
	}

	if ( !ent->fl.takedamage ) {
		const idSoundShader *shader = declManager->FindSound(meleeDef->GetString( "snd_miss" ));
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		return;
	}

	//
	// do the damage
	//
	p = meleeDef->GetString( "snd_hit" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	idVec3	kickDir;
	meleeDef->GetVector( "kickDir", "0 0 0", kickDir );

	idVec3	globalKickDir;
	globalKickDir = ( viewAxis * physicsObj.GetGravityAxis() ) * kickDir;

	ent->Damage( this, this, globalKickDir, meleeDefName, 1.0f, INVALID_JOINT );

	// end the attack if we're a multiframe attack
	EndAttack();
}

/*
=====================
idAI::TestMelee
=====================
*/
bool idAI::TestMelee( void ) const {
	trace_t trace;
	idActor *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt || !melee_range ) {
		return false;
	}

	if (!GetAttackFlag(COMBAT_MELEE))
	{
		// greebo: Cannot attack with melee weapons yet
		return false;
	}

	//FIXME: make work with gravity vector
	const idVec3& org = physicsObj.GetOrigin();
	const idBounds& bounds = physicsObj.GetBounds();

	const idVec3& enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
	const idBounds& enemyBounds = enemyEnt->GetPhysics()->GetBounds();

	idVec3 dir = enemyOrg - org;
	dir.z = 0;
	float dist = dir.LengthFast();

	float maxdist = melee_range + enemyBounds[1][0];

	if (dist < maxdist)
	{
		// angua: within horizontal distance
		if ((org.z + bounds[1][2] + melee_range_vert) > enemyOrg.z &&	// grayman #2655 - use melee_range_vert
				(enemyOrg.z + enemyBounds[1][2]) > org.z)
		{
			// within height
			// check if there is something in between
			idVec3 start = GetEyePosition();
			idVec3 end = enemyEnt->GetEyePosition();

			gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
			if ( ( trace.fraction == 1.0f ) || ( gameLocal.GetTraceEntity( trace ) == enemyEnt ) ) 
			{
				return true;
			}
		}
	}

	return false;
}

/*
=====================
idAI::TestMeleeFuture
=====================
*/
bool idAI::TestMeleeFuture( void ) const {
	trace_t trace;
	idActor *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt || !melee_range ) {
		return false;
	}

	if (!GetAttackFlag(COMBAT_MELEE))
	{
		// greebo: Cannot attack with melee weapons yet
		return false;
	}

	//FIXME: make work with gravity vector
	idVec3 org = physicsObj.GetOrigin();
	const idBounds& bounds = physicsObj.GetBounds();
	idVec3 vel = physicsObj.GetLinearVelocity();


	idVec3 enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
	const idBounds& enemyBounds = enemyEnt->GetPhysics()->GetBounds();
	idVec3 enemyVel = enemyEnt->GetPhysics()->GetLinearVelocity();

	// update prediction
	float dt = m_MeleePredictedAttTime;
	idVec3 ds = dt * vel;
	org += ds;
	//bounds = bounds.TranslateSelf( ds );

	idVec3 dsEnemy = dt * enemyVel;
	enemyOrg += dsEnemy;
	//enemyBounds = enemyBounds.TranslateSelf( dsEnemy );

	// rest is the same as TestMelee
	idVec3 dir = enemyOrg - org;
	dir.z = 0;
	float dist = dir.LengthFast();

	float maxdist = melee_range + enemyBounds[1][0];

	if (dist < maxdist)
	{
		// angua: within horizontal distance
		if ((org.z + bounds[1].z + melee_range_vert) > enemyOrg.z && // grayman #2655 - use melee_range_vert
			(enemyOrg.z + enemyBounds[1].z) > org.z)
		{
			// within height
			// check if there is something in between
			idVec3 start = GetEyePosition() + ds;
			idVec3 end = enemyEnt->GetEyePosition() + dsEnemy;

			gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
			if ( ( trace.fraction == 1.0f ) || ( gameLocal.GetTraceEntity( trace ) == enemyEnt ) ) 
			{
				return true;
			}
		}
	}

	return false;
}


/*
=====================
idAI::TestRanged
=====================
*/
bool idAI::TestRanged()
{
	idActor *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt) 
	{
		return false;
	}

	if (!GetAttackFlag(COMBAT_RANGED))
	{
		// greebo: Cannot attack with ranged weapons yet
		return false;
	}

	// Calculate the point on enemy AI needs to see
	idVec3 enemyPoint;
	// stgatilov: Look at the UNLEANED player's eye position
	// to achieve this, we treat player as ordinary actor
	if (enemyEnt->IsType(idPlayer::Type))
	{
		enemyPoint = enemyEnt->idActor::GetEyePosition();
	}
	else
	{
		enemyPoint = enemyEnt->GetEyePosition();
	}

	// Test if the enemy is within range, in FOV and not occluded
	float dist = (GetEyePosition() - enemyPoint).LengthFast();

	// grayman #3331 - the call to CanSeePositionExt() below isn't the correct
	// one to make here. That's designed to test if the enemy can see you when
	// you're standing behind something, and shouldn't be given an eye position
	// as the starting point (enemyPoint). It traces to that point, and if that
	// fails, it looks N units above that, where N = the enemy's height, which
	// makes no sense for what we're trying to do here.
	//
	// Instead, use CanSee(), which checks if you can see the enemy's eyes, his
	// shoulders, and his feet, looking for a good trace on any of them.

	return ( ( dist <= fire_range ) && CanSee(enemyEnt, true) );
//	return ( ( dist <= fire_range ) && CanSeePositionExt(enemyPoint, false, false) );
}


/*
=====================
idAI::AttackMelee

jointname allows the endpoint to be exactly specified in the model,
as for the commando tentacle.  If not specified, it will be set to
the facing direction + melee_range.

kickDir is specified in the monster's coordinate system, and gives the direction
that the view kick and knockback should go

DarkMod : Took out saving throws.
=====================
*/
bool idAI::AttackMelee( const char *meleeDefName ) {
	const idDict *meleeDef;
	idActor *enemyEnt = enemy.GetEntity();
	const char *p;
	const idSoundShader *shader;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown melee '%s'", meleeDefName );
	}

	if ( !enemyEnt ) {
		p = meleeDef->GetString( "snd_miss" );
		if ( p && *p ) {
			shader = declManager->FindSound( p );
			StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		}
		return false;
	}

	if ( !TestMelee() )
	{
		// missed
		p = meleeDef->GetString( "snd_miss" );
		if ( p && *p ) {
			shader = declManager->FindSound( p );
			StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		}
		return false;
	}

	//
	// do the damage
	//
	p = meleeDef->GetString( "snd_hit" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	idVec3	kickDir;
	meleeDef->GetVector( "kickDir", "0 0 0", kickDir );

	idVec3	globalKickDir;
	globalKickDir = ( viewAxis * physicsObj.GetGravityAxis() ) * kickDir;

	enemyEnt->Damage( this, this, globalKickDir, meleeDefName, 1.0f, INVALID_JOINT );

	// cause a LARGE tactile alert in the enemy, if it is an AI
	if( enemyEnt->IsType(idAI::Type) )
	{
		static_cast<idAI *>(enemyEnt)->TactileAlert( this, 100 );
	}

	lastAttackTime = gameLocal.time;

	return true;
}

/*
================
idAI::PushWithAF
================
*/
void idAI::PushWithAF( void ) {
	int i, j;
	idEntity *ent;
	idVec3 vel( vec3_origin ), vGravNorm( vec3_origin );

	af.ChangePose( this, gameLocal.time );
	idClip_afTouchList touchList;
	int num = af.EntitiesTouchingAF( touchList );

	idClip_EntityList pushed_ents;
	for( i = 0; i < num; i++ ) {
		if ( touchList[ i ].touchedEnt->IsType( idProjectile::Type ) ) {
			// skip projectiles
			continue;
		}

		// make sure we haven't pushed this entity already.  this avoids causing double damage
		for( j = 0; j < pushed_ents.Num(); j++ ) {
			if ( pushed_ents[ j ] == touchList[ i ].touchedEnt ) {
				break;
			}
		}
		if ( j >= pushed_ents.Num() )
		{
			ent = touchList[ i ].touchedEnt;
			pushed_ents.AddGrow(ent);
			vel = ent->GetPhysics()->GetAbsBounds().GetCenter() - touchList[ i ].touchedByBody->GetWorldOrigin();

			if ( ent->IsType(idPlayer::Type) && static_cast<idPlayer *>(ent)->noclip )
			{
				// skip player when noclip is on
				continue;
			}

			// ishtvan: don't push our own bind children (fix AI floating away when stuff is bound to them)
			if( ent->GetBindMaster() == this )
				continue;

			if( ent->IsType(idActor::Type) )
			{

				// Id code to stop from pushing the enemy back during melee
				// TODO: This will change with new melee system
				if ( attack.Length() )
				{
					// TODO: Don't need to do this right now, but keep in mind for future melee system
					ent->Damage( this, this, vel, attack, 1.0f, INVALID_JOINT );
				} else
				{
					// Ishtvan: Resolve velocity on to XY plane to stop from pushing AI up
					vGravNorm = physicsObj.GetGravityNormal();
					vel -= (vel * vGravNorm ) * vGravNorm;
					vel.Normalize();
					ent->GetPhysics()->SetLinearVelocity( 80.0f * vel, touchList[ i ].touchedClipModel->GetId() );
				}

				// Tactile Alert:

				// grayman #2345 - when an AI is non-solid, waiting for another AI
				// to pass by, there's no need to register a tactile alert from another AI

				if (!movementSubsystem->IsWaitingNonSolid())
				{
					if( ent->IsType(idPlayer::Type) )
					{
						// aesthetics: Dont react to dead player?
						if( ent->health > 0 )
							HadTactile( static_cast<idActor *>(ent) );
					}
					else if( ent->IsType(idAI::Type) && (ent->health > 0) && !static_cast<idAI *>(ent)->AI_KNOCKEDOUT )
					{
						if (!static_cast<idAI *>(ent)->movementSubsystem->IsWaitingNonSolid()) // grayman #2345 - don't call HadTactile() if the bumped AI is waiting
						{
							HadTactile( static_cast<idActor *>(ent) );
						}
					}
					else
					{
						// TODO: Touched a dead or unconscious body, should issue a body alert
						// Touched dead body code goes here: found body alert
					}
				}
			}
			// Ent was not an actor:
			else
			{
				vel.Normalize();
				ent->ApplyImpulse( this, touchList[i].touchedClipModel->GetId(), ent->GetPhysics()->GetOrigin(), cv_ai_bumpobject_impulse.GetFloat() * vel );
				if (ent->m_SetInMotionByActor.GetEntity() == NULL)
				{
					ent->m_SetInMotionByActor = this;
					ent->m_MovedByActor = this;
				}
			}
		}
	}
}

/***********************************************************************

	Misc

***********************************************************************/

/*
================
idAI::GetMuzzle
================
*/
void idAI::GetMuzzle( const char *jointname, idVec3 &muzzle, idMat3 &axis ) {
	jointHandle_t joint;

	if ( !jointname || !jointname[ 0 ] ) {
		muzzle = physicsObj.GetOrigin() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 14;
		muzzle -= physicsObj.GetGravityNormal() * physicsObj.GetBounds()[ 1 ].z * 0.5f;
	} else {
		joint = animator.GetJointHandle( jointname );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
		}
		GetJointWorldTransform( joint, gameLocal.time, muzzle, axis );
	}
}

/*
================
idAI::TriggerWeaponEffects
================
*/
void idAI::TriggerWeaponEffects( const idVec3 &muzzle ) {
	idVec3 org;
	idMat3 axis;

	if ( !g_muzzleFlash.GetBool() ) {
		return;
	}

	// muzzle flash
	// offset the shader parms so muzzle flashes show up
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] = gameLocal.random.CRandomFloat();

	if ( flashJointWorld != INVALID_JOINT ) {
		GetJointWorldTransform( flashJointWorld, gameLocal.time, org, axis );

		if ( worldMuzzleFlash.lightRadius.x > 0.0f ) {
			worldMuzzleFlash.axis = axis;
			worldMuzzleFlash.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
			if ( worldMuzzleFlashHandle != - 1 ) {
				gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
			} else {
				worldMuzzleFlashHandle = gameRenderWorld->AddLightDef( &worldMuzzleFlash );
			}
			muzzleFlashEnd = gameLocal.time + flashTime;
			UpdateVisuals();
		}
	}
}

/*
================
idAI::UpdateMuzzleFlash
================
*/
void idAI::UpdateMuzzleFlash( void ) {
	if ( worldMuzzleFlashHandle != -1 ) {
		if ( gameLocal.time >= muzzleFlashEnd ) {
			gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
			worldMuzzleFlashHandle = -1;
		} else {
			idVec3 muzzle;
			animator.GetJointTransform( flashJointWorld, gameLocal.time, muzzle, worldMuzzleFlash.axis );
			animator.GetJointTransform( flashJointWorld, gameLocal.time, muzzle, worldMuzzleFlash.axis );
			muzzle = physicsObj.GetOrigin() + ( muzzle + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();
			worldMuzzleFlash.origin = muzzle;
			gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
		}
	}
}

/*
================
idAI::Hide
================
*/
void idAI::Hide( void ) {
	idActor::Hide();
	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
	StopSound( SND_CHANNEL_AMBIENT, false );

	AI_ENEMY_IN_FOV		= false;
	AI_ENEMY_VISIBLE	= false;
	AI_ENEMY_TACTILE	= false;
	StopMove( MOVE_STATUS_DONE );
}

/*
================
idAI::Show
================
*/
void idAI::Show( void ) 
{
	idActor::Show();
	physicsObj.SetContents( m_preHideContents );

	physicsObj.GetClipModel()->Link( gameLocal.clip );
	fl.takedamage = !spawnArgs.GetBool( "noDamage" );
	StartSound( "snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL );
}
/*
================
idAI::CanBecomeSolid
================
*/
bool idAI::CanBecomeSolid( void ) {

	idClip_ClipModelList clipModels;
	int num = gameLocal.clip.ClipModelsTouchingBounds( physicsObj.GetAbsBounds(), MASK_MONSTERSOLID, clipModels );
	for ( int i = 0; i < num; i++ ) {
		idClipModel* cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		idEntity* hit = cm->GetEntity();
		if ( ( hit == this ) || !hit->fl.takedamage ) {
			continue;
		}

		if ( physicsObj.ClipContents( cm ) ) {
			return false;
		}
	}
	return true;

}

/*
=====================
idAI::UpdateParticles
=====================
*/
void idAI::UpdateParticles( void ) {
	if ( ( thinkFlags & TH_UPDATEPARTICLES) && !IsHidden() ) {
		idVec3 realVector;
		idMat3 realAxis;

		int particlesAlive = 0;
		for ( int i = 0; i < particles.Num(); i++ ) {
			if ( particles[i].particle && particles[i].time ) {
				particlesAlive++;
				if (af.IsActive()) {
					realAxis = mat3_identity;
					realVector = GetPhysics()->GetOrigin();
				} else {
					animator.GetJointTransform( particles[i].joint, gameLocal.time, realVector, realAxis );
					realAxis *= renderEntity.axis;
					realVector = physicsObj.GetOrigin() + ( realVector + modelOffset ) * ( viewAxis * physicsObj.GetGravityAxis() );
				}

				if ( !gameLocal.smokeParticles->EmitSmoke( particles[i].particle, particles[i].time, gameLocal.random.CRandomFloat(), realVector, realAxis )) {
					if ( restartParticles ) {
						particles[i].time = gameLocal.time;
					} else {
						particles[i].time = 0;
						particlesAlive--;
					}
				}
			}
		}
		if ( particlesAlive == 0 ) {
			BecomeInactive( TH_UPDATEPARTICLES );
		}
	}
}

/*
=====================
idAI::TriggerParticles
=====================
*/
void idAI::TriggerParticles( const char *jointName ) {
	jointHandle_t jointNum;

	jointNum = animator.GetJointHandle( jointName );
	for ( int i = 0; i < particles.Num(); i++ ) {
		if ( particles[i].joint == jointNum ) {
			particles[i].time = gameLocal.time;
			BecomeActive( TH_UPDATEPARTICLES );
		}
	}
}


/***********************************************************************

	Head & torso aiming

***********************************************************************/

/*
================
idAI::UpdateAnimationControllers
================
*/
bool idAI::UpdateAnimationControllers( void ) {
	idVec3		focusPos;
	idVec3		left;
	idVec3 		dir;
	idVec3 		orientationJointPos;
	idVec3 		localDir;
	idAngles 	newLookAng;
	idAngles	diff;
	idMat3		axis;
	idMat3		orientationJointAxis;
	idAFAttachment	*headEnt = head.GetEntity();
	idVec3		eyepos;
	int			i;
	idAngles	jointAng;
	float		orientationJointYaw;

	if ( AI_DEAD || AI_KNOCKEDOUT )
	{
		return idActor::UpdateAnimationControllers();
	}

	if ( orientationJoint == INVALID_JOINT ) {
		orientationJointAxis = viewAxis;
		orientationJointPos = physicsObj.GetOrigin();
		orientationJointYaw = current_yaw;
	} else {
		GetJointWorldTransform( orientationJoint, gameLocal.time, orientationJointPos, orientationJointAxis );
		orientationJointYaw = orientationJointAxis[ 2 ].ToYaw();
		orientationJointAxis = idAngles( 0.0f, orientationJointYaw, 0.0f ).ToMat3();
	}

	if ( focusJoint != INVALID_JOINT ) {
		if ( headEnt ) {
			headEnt->GetJointWorldTransform( focusJoint, gameLocal.time, eyepos, axis );
		} else {
			GetJointWorldTransform( focusJoint, gameLocal.time, eyepos, axis );
		}
		eyeOffset.z = eyepos.z - physicsObj.GetOrigin().z;
		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugLine( colorRed, eyepos, eyepos + orientationJointAxis[ 0 ] * 32.0f, USERCMD_MSEC );
		}
	} else {
		eyepos = GetEyePosition();
	}

	if ( headEnt ) {
		CopyJointsFromBodyToHead();
	}

	// Update the IK after we've gotten all the joint positions we need, but before we set any joint positions.
	// Getting the joint positions causes the joints to be updated.  The IK gets joint positions itself (which
	// are already up to date because of getting the joints in this function) and then sets their positions, which
	// forces the heirarchy to be updated again next time we get a joint or present the model.  If IK is enabled,
	// or if we have a seperate head, we end up transforming the joints twice per frame.  Characters with no
	// head entity and no ik will only transform their joints once.  Set g_debuganim to the current entity number
	// in order to see how many times an entity transforms the joints per frame.
	idActor::UpdateAnimationControllers();

	idEntity *focusEnt = focusEntity.GetEntity();
	if ( !allowJointMod || !allowEyeFocus || ( gameLocal.time >= focusTime ) ) {
	    focusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 512.0f;
	} else if ( focusEnt == NULL ) {
		// keep looking at last position until focusTime is up
		focusPos = currentFocusPos;
	} else if ( focusEnt == enemy.GetEntity() ) {
		focusPos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset - eyeVerticalOffset * enemy.GetEntity()->GetPhysics()->GetGravityNormal();
	} else if ( focusEnt->IsType( idActor::Type ) ) {
		focusPos = static_cast<idActor *>( focusEnt )->GetEyePosition() - eyeVerticalOffset * focusEnt->GetPhysics()->GetGravityNormal();
	} else {
		focusPos = focusEnt->GetPhysics()->GetOrigin();
	}

	currentFocusPos = currentFocusPos + ( focusPos - currentFocusPos ) * eyeFocusRate;

	// determine yaw from origin instead of from focus joint since joint may be offset, which can cause us to bounce between two angles
	dir = focusPos - orientationJointPos;
	newLookAng.yaw = idMath::AngleNormalize180( dir.ToYaw() - orientationJointYaw );
	newLookAng.roll = 0.0f;
	newLookAng.pitch = 0.0f;

#if 0
	gameRenderWorld->DebugLine( colorRed, orientationJointPos, focusPos, USERCMD_MSEC );
	gameRenderWorld->DebugLine( colorYellow, orientationJointPos, orientationJointPos + orientationJointAxis[ 0 ] * 32.0f, USERCMD_MSEC );
	gameRenderWorld->DebugLine( colorGreen, orientationJointPos, orientationJointPos + newLookAng.ToForward() * 48.0f, USERCMD_MSEC );
#endif

	// determine pitch from joint position
	dir = focusPos - eyepos;
	dir.NormalizeFast();
	orientationJointAxis.ProjectVector( dir, localDir );
	newLookAng.pitch = -idMath::AngleNormalize180( localDir.ToPitch() );
	newLookAng.roll	= 0.0f;

	diff = newLookAng - lookAng;

	if ( eyeAng != diff ) {
		eyeAng = diff;
		eyeAng.Clamp( eyeMin, eyeMax );
		idAngles angDelta = diff - eyeAng;
		if ( !angDelta.Compare( ang_zero, 0.1f ) ) {
			alignHeadTime = gameLocal.time;
		} else {
			alignHeadTime = gameLocal.time + static_cast<int>(( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime);
		}
	}

	if ( idMath::Fabs( newLookAng.yaw ) < 0.1f ) {
		alignHeadTime = gameLocal.time;
	}

	if ( ( gameLocal.time >= alignHeadTime ) || ( gameLocal.time < forceAlignHeadTime ) ) {
		alignHeadTime = gameLocal.time + static_cast<int>(( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime);
		destLookAng = newLookAng;
		destLookAng.Clamp( lookMin, lookMax );
	}

	diff = destLookAng - lookAng;
	if ( ( lookMin.pitch == -180.0f ) && ( lookMax.pitch == 180.0f ) ) {
		if ( ( diff.pitch > 180.0f ) || ( diff.pitch <= -180.0f ) ) {
			diff.pitch = 360.0f - diff.pitch;
		}
	}
	if ( ( lookMin.yaw == -180.0f ) && ( lookMax.yaw == 180.0f ) ) {
		if ( diff.yaw > 180.0f ) {
			diff.yaw -= 360.0f;
		} else if ( diff.yaw <= -180.0f ) {
			diff.yaw += 360.0f;
		}
	}
	lookAng = lookAng + diff * headFocusRate;
	lookAng.Normalize180();

	jointAng.roll = 0.0f;

	if (AI_AlertLevel >= thresh_5)
	{
		// use combat look joints in combat

		for( i = 0; i < lookJointsCombat.Num(); i++ ) 
		{
			jointAng.pitch	= lookAng.pitch * lookJointAnglesCombat[ i ].pitch;
			jointAng.yaw	= lookAng.yaw * lookJointAnglesCombat[ i ].yaw;
			animator.SetJointAxis( lookJointsCombat[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
		}
	}
	else
	{
		for( i = 0; i < lookJoints.Num(); i++ ) 
		{
			jointAng.pitch	= lookAng.pitch * lookJointAngles[ i ].pitch;
			jointAng.yaw	= lookAng.yaw * lookJointAngles[ i ].yaw;
			animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
		}
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		// lean into turns
		AdjustFlyingAngles();
	}

	if ( headEnt ) {
		idAnimator *headAnimator = headEnt->GetAnimator();

		if ( allowEyeFocus ) {
			idMat3 eyeAxis = ( lookAng + eyeAng ).ToMat3();
			idMat3 headTranspose = headEnt->GetPhysics()->GetAxis().Transpose();
			axis =  eyeAxis * orientationJointAxis;
			left = axis[ 1 ] * eyeHorizontalOffset;
			eyepos -= headEnt->GetPhysics()->GetOrigin();
			headAnimator->SetJointPos( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + ( axis[ 0 ] * 64.0f + left ) * headTranspose );
			headAnimator->SetJointPos( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + ( axis[ 0 ] * 64.0f - left ) * headTranspose );
		} else {
			headAnimator->ClearJoint( leftEyeJoint );
			headAnimator->ClearJoint( rightEyeJoint );
		}
	} else {
		if ( allowEyeFocus ) {
			idMat3 eyeAxis = ( lookAng + eyeAng ).ToMat3();
			axis =  eyeAxis * orientationJointAxis;
			left = axis[ 1 ] * eyeHorizontalOffset;
			eyepos += axis[ 0 ] * 64.0f - physicsObj.GetOrigin();
			animator.SetJointPos( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + left );
			animator.SetJointPos( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos - left );
		} else {
			animator.ClearJoint( leftEyeJoint );
			animator.ClearJoint( rightEyeJoint );
		}
	}

	return true;
}

/***********************************************************************

idCombatNode

***********************************************************************/

const idEventDef EV_CombatNode_MarkUsed( "markUsed", EventArgs(), EV_RETURNS_VOID, "Disables the combat node if \"use_once\" is set on the entity." );

CLASS_DECLARATION( idEntity, idCombatNode )
	EVENT( EV_CombatNode_MarkUsed,				idCombatNode::Event_MarkUsed )
	EVENT( EV_Activate,							idCombatNode::Event_Activate )
END_CLASS

/*
=====================
idCombatNode::idCombatNode
=====================
*/
idCombatNode::idCombatNode( void ) {
	min_dist = 0.0f;
	max_dist = 0.0f;
	cone_dist = 0.0f;
	min_height = 0.0f;
	max_height = 0.0f;
	cone_left.Zero();
	cone_right.Zero();
	offset.Zero();
	disabled = false;
}

/*
=====================
idCombatNode::Save
=====================
*/
void idCombatNode::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( min_dist );
	savefile->WriteFloat( max_dist );
	savefile->WriteFloat( cone_dist );
	savefile->WriteFloat( min_height );
	savefile->WriteFloat( max_height );
	savefile->WriteVec3( cone_left );
	savefile->WriteVec3( cone_right );
	savefile->WriteVec3( offset );
	savefile->WriteBool( disabled );
}

/*
=====================
idCombatNode::Restore
=====================
*/
void idCombatNode::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( min_dist );
	savefile->ReadFloat( max_dist );
	savefile->ReadFloat( cone_dist );
	savefile->ReadFloat( min_height );
	savefile->ReadFloat( max_height );
	savefile->ReadVec3( cone_left );
	savefile->ReadVec3( cone_right );
	savefile->ReadVec3( offset );
	savefile->ReadBool( disabled );
}

/*
=====================
idCombatNode::Spawn
=====================
*/
void idCombatNode::Spawn( void ) {
	float fov;
	float yaw;
	float height;

	min_dist = spawnArgs.GetFloat( "min" );
	max_dist = spawnArgs.GetFloat( "max" );
	height = spawnArgs.GetFloat( "height" );
	fov = spawnArgs.GetFloat( "fov", "60" );
	offset = spawnArgs.GetVector( "offset" );

	const idVec3 &org = GetPhysics()->GetOrigin() + offset;
	min_height = org.z - height * 0.5f;
	max_height = min_height + height;

	const idMat3 &axis = GetPhysics()->GetAxis();
	yaw = axis[ 0 ].ToYaw();

	idAngles leftang( 0.0f, yaw + fov * 0.5f - 90.0f, 0.0f );
	cone_left = leftang.ToForward();

	idAngles rightang( 0.0f, yaw - fov * 0.5f + 90.0f, 0.0f );
	cone_right = rightang.ToForward();

	disabled = spawnArgs.GetBool( "start_off" );
}

/*
=====================
idCombatNode::IsDisabled
=====================
*/
bool idCombatNode::IsDisabled( void ) const {
	return disabled;
}

/*
=====================
idCombatNode::DrawDebugInfo
=====================
*/
void idCombatNode::DrawDebugInfo( void ) {
	idEntity		*ent;
	idCombatNode	*node;
	idPlayer		*player = gameLocal.GetLocalPlayer();
	idVec4			color;
	idBounds		bounds( idVec3( -16, -16, 0 ), idVec3( 16, 16, 0 ) );

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( !ent->IsType( idCombatNode::Type ) ) {
			continue;
		}

		node = static_cast<idCombatNode *>( ent );
		if ( node->disabled ) {
			color = colorMdGrey;
		} else if ( player && node->EntityInView( player, player->GetPhysics()->GetOrigin() ) ) {
			color = colorYellow;
		} else {
			color = colorRed;
		}

		idVec3 leftDir( -node->cone_left.y, node->cone_left.x, 0.0f );
		idVec3 rightDir( node->cone_right.y, -node->cone_right.x, 0.0f );
		idVec3 org = node->GetPhysics()->GetOrigin() + node->offset;

		bounds[ 1 ].z = node->max_height;

		leftDir.NormalizeFast();
		rightDir.NormalizeFast();

		const idMat3 &axis = node->GetPhysics()->GetAxis();
		float cone_dot = node->cone_right * axis[ 1 ];
		if ( idMath::Fabs( cone_dot ) > 0.1 ) {
			float cone_dist = node->max_dist / cone_dot;
			idVec3 pos1 = org + leftDir * node->min_dist;
			idVec3 pos2 = org + leftDir * cone_dist;
			idVec3 pos3 = org + rightDir * node->min_dist;
			idVec3 pos4 = org + rightDir * cone_dist;

			gameRenderWorld->DebugLine( color, node->GetPhysics()->GetOrigin(), ( pos1 + pos3 ) * 0.5f, USERCMD_MSEC );
			gameRenderWorld->DebugLine( color, pos1, pos2, USERCMD_MSEC );
			gameRenderWorld->DebugLine( color, pos1, pos3, USERCMD_MSEC );
			gameRenderWorld->DebugLine( color, pos3, pos4, USERCMD_MSEC );
			gameRenderWorld->DebugLine( color, pos2, pos4, USERCMD_MSEC );
			gameRenderWorld->DebugBounds( color, bounds, org, USERCMD_MSEC );
		}
	}
}

/*
=====================
idCombatNode::EntityInView
=====================
*/
bool idCombatNode::EntityInView( idActor *actor, const idVec3 &pos ) {
	if ( !actor || ( actor->health <= 0 ) ) {
		return false;
	}

	const idBounds &bounds = actor->GetPhysics()->GetBounds();
	if ( ( pos.z + bounds[ 1 ].z < min_height ) || ( pos.z + bounds[ 0 ].z >= max_height ) ) {
		return false;
	}

	const idVec3 &org = GetPhysics()->GetOrigin() + offset;
	const idMat3 &axis = GetPhysics()->GetAxis();
	idVec3 dir = pos - org;
	float  dist = dir * axis[ 0 ];

	if ( ( dist < min_dist ) || ( dist > max_dist ) ) {
		return false;
	}

	float left_dot = dir * cone_left;
	if ( left_dot < 0.0f ) {
		return false;
	}

	float right_dot = dir * cone_right;
	if ( right_dot < 0.0f ) {
		return false;
	}

	return true;
}

/*
=====================
idCombatNode::Event_Activate
=====================
*/
void idCombatNode::Event_Activate( idEntity *activator ) {
	disabled = !disabled;
}

/*
=====================
idCombatNode::Event_MarkUsed
=====================
*/
void idCombatNode::Event_MarkUsed( void ) {
	if ( spawnArgs.GetBool( "use_once" ) ) {
		disabled = true;
	}
}

// DarkMod sound propagation functions

void idAI::SPLtoLoudness( SSprParms *propParms )
{
	// put in frequency, duration and bandwidth effects here
	propParms->loudness = propParms->propVol;
}

bool idAI::CheckHearing( SSprParms *propParms )
{
	bool returnval(false);

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("CheckHearing to AI %s, loudness %f, threshold %f\r",name.c_str(),propParms->loudness,m_AudThreshold );
	if (propParms->loudness > m_AudThreshold)
	{
		returnval = true;
	}

	return returnval;
}

void idAI::HearSound(SSprParms *propParms, float noise, const idVec3& origin)
{
	if (m_bIgnoreAlerts)
	{
		return;
	}

	idVec3 effectiveOrigin = origin; // grayman #3857
	bool addFuzziness = true; // grayman #3857 - whether to add small amount of randomness to the sound origin or not
	bool noisemaker = false;  // grayman #3857 - if this sound was made by a noisemaker arrow

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("CheckHearing to AI %s, loudness %f, threshold %f\r",name.c_str(),propParms->loudness,m_AudThreshold );
	// TODO:
	// Modify loudness by propVol/noise ratio,
	// looking up a selectivity spawnarg on the AI to
	// see how well the AI distinguishes signal from noise

	//psychLoud = pow(2, (propParms->loudness - threshold)/10 );
	// this scale didn't make much sense for the alerts ingame
	// because the numbers would have been very close together for
	// very different amounts of alert.
	// It is better to keep it in dB.

	// and so alert units are born!

	/**
	* NOTE: an AlertLevel of 1 constitutes just barely seeing something
	* out of the corner of your eye, or just barely hearing a whisper
	* of a sound for a short instant.  An AlertLevel of 10 is seeing/hearing twice
	* as much, 20 is four times as much, etc.
	**/

	// angua: alert increase is scaled by alertFactor (defined on propagated sound). 
	// This way, different sounds can result in different alert increase at the same volume
	float psychLoud = 1 + (propParms->loudness - m_AudThreshold) * propParms->alertFactor;
	
	// angua: alert increase can not exceed alertMax (defined on propagated sound)
	if (psychLoud > propParms->alertMax)
	{
		psychLoud = propParms->alertMax;
	}

	// don't alert the AI if they're deaf, or this is not a strong enough
	// alert to overwrite another alert this frame

	// grayman #3424 - audio alerts shouldn't be allowed in the delays between
	// methods like OnDeadPersonEncounter() and Post_OnDeadPersonEncounter() because
	// they can mess with what's happening in those methods.

	if ( ( GetAcuity("aud") > 0 ) && ( psychLoud > m_AlertLevelThisFrame ) && m_allowAudioAlerts )
	{
		// grayman #3681 - Since noisemakers can emit several instances of
		// the same sound, we want AI to only react to the first instance.
		// If they react to each instance of the sound, their alert level
		// rises too quickly.

		idEntityPtr<idEntity> makerPtr;
		makerPtr = NULL;
		idEntity *maker = propParms->maker;
		if (maker->IsType(idMoveable::Type)) // noisemakers are moveables
		{
			if (idStr(maker->GetName()).IcmpPrefix("idMoveable_atdm:ammo_noisemaker") == 0)
			{
				// Have I already heard this noisemaker?

				makerPtr = maker;
				if (m_noisemakersHeard.Find(makerPtr) != NULL)
				{
					return; // already heard this noisemaker
				}

				noisemaker = true;

				// grayman #3857 - use the original origin of the noisemaker so that
				// searches can happen in the same area, even if the
				// noisemaker moves around

				maker->spawnArgs.GetVector( "firstOrigin", "0 0 0", effectiveOrigin );

				// trace down until you hit something
				idVec3 bottomPoint = effectiveOrigin;
				bottomPoint.z -= 1000;

				trace_t result;
				if ( gameLocal.clip.TracePoint(result, effectiveOrigin, bottomPoint, MASK_OPAQUE, NULL) )
				{
					// Found the floor.
					effectiveOrigin.z = result.endpos.z + 1; // move the target point to just above the floor
				}

				addFuzziness = false; // don't add small amount of randomness to the sound location
			}
		}

		AI_HEARDSOUND = true;

		// grayman #3413 - use sound origin as alert origin if it can be reached.
		// If not, use the apparent sound origin, which might be in a portal.

		idVec3 myOrigin = GetPhysics()->GetOrigin();
		int areaNum = PointReachableAreaNum(myOrigin, 1.0f);
		int soundAreaNum = PointReachableAreaNum(effectiveOrigin, 1.0f);
		aasPath_t path;

		if ( PathToGoal(path, areaNum, myOrigin, soundAreaNum, effectiveOrigin, this) )
		{
			m_SoundDir = effectiveOrigin; // use real sound origin
		}
		else
		{
			m_SoundDir = propParms->direction; // use apparent sound origin
		}
		//m_SoundDir = origin; // original setting

		m_AlertedByActor = NULL; // grayman #2907 - needs to be cleared, otherwise it can be left over from a previous sound this frame

		if (propParms->maker->IsType(idActor::Type))
		{
			// grayman #3394 - maker might have made the sound, but was
			// he put in motion by the player?

			idActor* setInMotionBy = propParms->maker->m_SetInMotionByActor.GetEntity();
			if ( setInMotionBy != NULL )
			{
				m_AlertedByActor = setInMotionBy;
			}
			else
			{
				m_AlertedByActor = static_cast<idActor *>(propParms->maker);
			}
		}
		else
		{
			// greebo: Take the responsible actor for motion sound
			idActor* responsibleActor = propParms->maker->m_MovedByActor.GetEntity();
			if (responsibleActor != NULL)
			{
				m_AlertedByActor = responsibleActor;
			}
		}

		psychLoud *= GetAcuity("aud");

		// angua: the actor who produced this noise is either unknown or an enemy
		// alert now
		// grayman #3394 - it could also have been made by a body hitting the floor,
		// and that body might be a friend
		// grayman #2816 - no alert if it was made by someone we killed
		// grayman #3857 - no alert if it was made by a dead or KO'ed body that was kicked by another AI

		idActor *soundMaker = m_AlertedByActor.GetEntity();
		if ( !soundMaker || // alert if unknown sound maker
			 ( IsEnemy(soundMaker) && ( soundMaker != m_lastKilled.GetEntity() ) && !soundMaker->fl.notarget && !soundMaker->fl.inaudible ) || // alert if enemy and not the last we killed and not in notarget mode
			 ( IsAfraid() && ((propParms->name == "arrow_broad_hit") || (propParms->name == "arrow_broad_break")))) // alert if this is a scary arrow sound
		{
			// greebo: Notify the currently active state
			bool shouldAlert = mind->GetState()->OnAudioAlert(propParms->name,addFuzziness,maker); // grayman #3847 // grayman #3857

			// Decide if you need to remember a noisemaker

			if (noisemaker)
			{
				// Only remember a noisemaker if it puts you into Searching mode or higher
				if ( AI_AlertLevel + psychLoud >= thresh_3 )
				{
					// place this noisemaker on the list of noisemakers I've heard.

					m_noisemakersHeard.Append(makerPtr);

					// Create an event to remove this noisemaker from
					// the list after a certain amount of time has elapsed.
					// This keeps the list as short as possible over time.

					float duration = maker->spawnArgs.GetFloat("active_duration","17");
					PostEventSec(&AI_NoisemakerDone,duration,maker);
				}
			}

			// grayman #3009 - pass the alert position so the AI can look at it
			// grayman #3848 - but not if you've already been told to flee
			if (shouldAlert)
			{
				// sets alert level
				PreAlertAI( "aud", psychLoud, GetMemory().alertPos ); // grayman #3356

				// Log the event if alert level will be high enough to search.
				// There's a delay between PreAlertAI() and Event_AlertAI(), and
				// the latter sets the new alert level, so at this point, the
				// alert level doesn't include 'psychLoud'.

				if ( AI_AlertLevel + psychLoud >= thresh_3 )
				{
					if (maker->IsType(idMoveable::Type) && (idStr(maker->GetName()).IcmpPrefix("idMoveable_atdm:ammo_noisemaker") == 0)) // noisemakers are moveables
					{
						idVec3 initialNoiseOrigin;
						maker->spawnArgs.GetVector( "firstOrigin", "0 0 0", initialNoiseOrigin );

						// don't provide the noisemaker itself as the entity parameter because that might go away
						GetMemory().currentSearchEventID = LogSuspiciousEvent( E_EventTypeNoisemaker, initialNoiseOrigin, NULL, false ); // grayman #3857
					}
					else
					{
						GetMemory().currentSearchEventID = LogSuspiciousEvent( E_EventTypeSound, GetMemory().alertPos, NULL, false ); // grayman #3857
					}
				}
			}
		}
		
		// Retrieve the messages from the other AI, if there are any
		if (propParms->makerAI != NULL)
		{
			for ( int i = 0 ; i < propParms->makerAI->m_Messages.Num() ; i++ )
			{
				// grayman #3355 - Only pass messages that have a zero message tag, or a
				// message tag that matches the sound's.
				ai::CommMessagePtr message = propParms->makerAI->m_Messages[i];
				if ( ( message->m_msgTag == 0 ) || ( message->m_msgTag == propParms->messageTag ) )
				{
					mind->GetState()->OnAICommMessage(*message, psychLoud);
				}
			}
		}

		if ( cv_spr_show.GetBool() )
		{
			gameRenderWorld->DebugText( va("Alert: %.2f", psychLoud), 
				(GetEyePosition() - GetPhysics()->GetGravityNormal() * 55.0f), 0.25f, 
				colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC * 30);
		}

		if ( cv_ai_debug.GetBool() )
		{
			gameLocal.Printf("%s HEARD a sound\n", name.c_str());
		}
	}
}

// grayman - Preprocessing of an alert, for the purpose of inserting
// a delay for audio alerts

void idAI::PreAlertAI(const char *type, float amount, idVec3 lookAt)
{
	// grayman #3424 - don't process if dead or unconscious

	if ( AI_DEAD || AI_KNOCKEDOUT )
	{
		return;
	}

	m_lookAtPos = lookAt; // grayman #3520 - look here when looking at the alert (might be different than alertPos)

	int delay = 0;
	if ( idStr(type) == "aud" )
	{
		delay = AUD_ALERT_DELAY_MIN + gameLocal.random.RandomInt(AUD_ALERT_DELAY_MAX - AUD_ALERT_DELAY_MIN);
	}
	PostEventMS(&AI_AlertAI,delay,type,amount,m_AlertedByActor.GetEntity()); // grayman #3258
}

void idAI::Event_AlertAI(const char *type, float amount, idActor* actor) // grayman #3258
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("idAI::Event_AlertAI - %s called with type %s, amount %f, and actor %s\r",name.c_str(),type,amount,actor ? actor->name.c_str():"NULL");

	if (m_bIgnoreAlerts)
	{
		return;
	}

	if ( AI_DEAD || AI_KNOCKEDOUT ) // grayman #3424
	{
		return;
	}

	// Ignore actors for various reasons
	if (actor)
	{
		if ( actor->fl.notarget ||
			 ( (idStr(type) == "vis") && actor->fl.invisible) || // grayman #3857
			 ( (idStr(type) == "aud") && actor->fl.inaudible) || // grayman #3857
			 ( actor == m_lastKilled.GetEntity() ) ) // grayman #2816 - don't respond to alerts made by someone you last killed
		{
			return;
		}
	}

	m_AlertedByActor = actor; // grayman #3258

	// Calculate the amount the current AI_AlertLevel is about to be increased
	// angua: alert amount already includes acuity
	// float acuity = GetAcuity(type);
	// float alertInc = amount * acuity * 0.01f; // Acuity is defaulting to 100 (= 100%)

	float alertInc = amount;

	if ( m_AlertGraceTime && !AI_VISALERT ) // grayman #3492 - ignore grace periods for player spotting
	{
		if (gameLocal.time > m_AlertGraceStart + m_AlertGraceTime)
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("AI ALERT: %s Grace period has expired. Resetting.\r",name.c_str());
			m_AlertGraceTime = 0;
			m_AlertGraceActor = NULL;
			m_AlertGraceStart = 0;
			m_AlertGraceThresh = 0;
			m_AlertGraceCount = 0;
			m_AlertGraceCountLimit = 0;
		}
		else if (alertInc < m_AlertGraceThresh &&
				  actor != NULL &&
				  actor == m_AlertGraceActor.GetEntity() && 
				  m_AlertGraceCount < m_AlertGraceCountLimit)
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("AI ALERT: %s Waiting for grace period to expire, ignoring alert.\r",name.c_str());
			m_AlertGraceCount++;

			// Quick hack: Large lightgem values and visual alerts override the grace period count faster
			// grayman #3857 - if you're here, AI_VISALERT is FALSE
			/*
			if (AI_VISALERT)
			{
				// greebo: Let the alert grace count increase by 12.5% of the current lightgem value
				// The maximum increase is therefore 32/8 = 4 based on DARKMOD_LG_MAX at the time of writing.
				if (actor->IsType(idPlayer::Type))
				{
					idPlayer* player = static_cast<idPlayer*>(actor);
					m_AlertGraceCount += static_cast<int>(idMath::Round(player->GetCurrentLightgemValue() * 0.125f));
				}
			}*/

			return;
		}
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("AI ALERT: Alert %f above threshold %f, grace count has reached its limit, grace period has expired\r", alertInc, m_AlertGraceThresh);
	}

	// grayman #3520 - look at alert spot, unless it's a 'vis' alert
	if ( idStr(type) != "vis" )
	{
		m_lookAtAlertSpot = true;
	}
	else
	{
		m_lookAtAlertSpot = false;
		m_lookAtPos = idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY);
	}

	// set the last alert value so that simultaneous alerts only overwrite if they are greater than the value
	m_AlertLevelThisFrame = amount;

	// The grace check has failed, increase the AI_AlertLevel float by the increase amount
	float newAlertLevel = AI_AlertLevel + alertInc;
	SetAlertLevel(newAlertLevel);

	if ( cv_ai_debug.GetBool() )
	{
		gameLocal.Printf("[TDM AI] ALERT: %s alerted by alert type \"%s\", base amount %f. Total alert level now: %f\n", name.c_str(), type, amount, (float) AI_AlertLevel );
	}

	if (gameLocal.isNewFrame)
	{
		// AI has been alerted, set the boolean
		AI_ALERTED = true;
	}

	// grayman #3019 - Decide whether to pass the alert
	// to mission statistics or not. You should only pass it if you're rising
	// to an alert index higher than where you were.

	// grayman #3069 - and only if the AI can seek out the player

	// Objectives callback
	if ( AlertIndexIncreased() && m_canSearch )
	{
		RegisterAlert(m_AlertedByActor.GetEntity()); // grayman #4002 - add alert to alert queue
		gameLocal.m_MissionData->AlertCallback( this, m_AlertedByActor.GetEntity(), static_cast<int>(AI_AlertIndex) );
	}
}

void idAI::SetAlertLevel(float newAlertLevel)
{
	if (AI_DEAD || AI_KNOCKEDOUT)  // grayman #3424 - moved earlier
	{
		return;
	}

	float currentAlertLevel = AI_AlertLevel; // grayman #3424

	// grayman #3069 - clamp the alert level to just under ESearching
	// if you can't search

	if ( !m_canSearch && ( newAlertLevel >= thresh_3 ) )
	{
		newAlertLevel = thresh_3 - 0.1;
	}

	// greebo: Clamp the (log) alert number to twice the combat threshold.
	if (newAlertLevel > thresh_5*2)
	{
		newAlertLevel = thresh_5*2;
	}
	else if (newAlertLevel < 0)
	{
		newAlertLevel = 0;
	}

	AI_AlertLevel = newAlertLevel;

	if (AI_AlertLevel > m_maxAlertLevel)
	{
		m_maxAlertLevel = AI_AlertLevel;
	}
	
	// grace period vars
	float grace_time;
	float grace_frac;
	int grace_count;

	// How long should this alert level last, and which alert index should we be in now?
	if (newAlertLevel >= thresh_4)
	{
		// greebo: Only allow switching to combat if a valid enemy is set.
		if (newAlertLevel >= thresh_5)
		{
			if (GetEnemy() != NULL) 
			{
				// We have an enemy, raise the index
				m_prevAlertIndex = AI_AlertIndex;
				AI_AlertIndex = ai::ECombat;
			}
			else
			{
				// No enemy, can't switch to Combat mode
				m_prevAlertIndex = AI_AlertIndex;
				AI_AlertIndex = ai::EAgitatedSearching;
				// Set the alert level back to just below combat threshold
				AI_AlertLevel = thresh_5 - 0.01;
			}
		}
		else
		{
			m_prevAlertIndex = AI_AlertIndex;
			AI_AlertIndex = ai::EAgitatedSearching;
		}
		grace_time = m_gracetime_4;
		grace_frac = m_gracefrac_4;
		grace_count = m_gracecount_4;
	}
	else if (newAlertLevel >= thresh_3)
	{
		m_prevAlertIndex = AI_AlertIndex;
		AI_AlertIndex = ai::ESearching;
		grace_time = m_gracetime_3;
		grace_frac = m_gracefrac_3;
		grace_count = m_gracecount_3;
	}
	else if (newAlertLevel >= thresh_2)
	{
		m_prevAlertIndex = AI_AlertIndex;
		AI_AlertIndex = ai::ESuspicious;
		grace_time = m_gracetime_2;
		grace_frac = m_gracefrac_2;
		grace_count = m_gracecount_2;
	}
	else if (newAlertLevel >= thresh_1)
	{
		m_prevAlertIndex = AI_AlertIndex;
		AI_AlertIndex = ai::EObservant;
		grace_time = m_gracetime_1;
		grace_frac = m_gracefrac_1;
		grace_count = m_gracecount_1;
	}
	else
	{
		m_prevAlertIndex = AI_AlertIndex;
		AI_AlertIndex = ai::ERelaxed;
		grace_time = 0.0;
		grace_frac = 0.0;
		grace_count = 0;
	}

	// Tels: When the AI_AlertIndex increased, detach all bound entities
	// that have set "unbindonalertIndex" higher or equal:
    if (m_prevAlertIndex < AI_AlertIndex)
	{
		DetachOnAlert( AI_AlertIndex );
	}

	// greebo: Remember the highest alert index
	if (AI_AlertIndex > m_maxAlertIndex)
	{
		m_maxAlertIndex = AI_AlertIndex;
	}
	
	bool alertRising = (AI_AlertLevel > currentAlertLevel);

	if (alertRising)
	{
		GetMemory().lastAlertRiseTime = gameLocal.time;
		m_recentHighestAlertLevel = AI_AlertLevel; // grayman #3424
	}

	// Begin the grace period
	if (alertRising)
	{
		Event_SetAlertGracePeriod( grace_frac, grace_time, grace_count );
	}
}

bool idAI::AlertIndexIncreased() 
{
	return (AI_AlertIndex > m_prevAlertIndex);
}

// grayman #4002 - If the AI is alerted by an entity that already appears on
// the alerted entity list, and a certain amount of time has gone by since
// that previous alert occurred, and the new alert index is higher than the
// previous entry, update the entry and return true.
//
// If the alerting entity doesn't appear on the list, create a new entry and
// place it on the list and return true;
//
// Otherwise return false.

void idAI::RegisterAlert(idEntity* alertedBy)
{
	if ( alertedBy )
	{
		EntityAlert ea;

		ea.entityResponsible = alertedBy;	// The entity responsible
		ea.timeAlerted = gameLocal.time;	// The last time this entity raised the alert index
		ea.alertIndex = static_cast<int>(AI_AlertIndex); // The alert index reached
		ea.ignore = false; // This entry hasn't been processed yet
		alertQueue.Insert(ea); // Place at front of queue
	}
}

// grayman #4002 - Examine the alert queue to determine if the current alert supercedes a previous alert.
//				   Return the value of that previous alert.
//                 Alerts are inserted at the front of the queue as they happen, so the earlier ones
//                 have later timestamps than the later ones.

int idAI::ExamineAlerts()
{
	int numberOfAlerts = alertQueue.Num();
	int alertsProcessed = 0;
	idEntity* entityResponsible = NULL;
	int nextTime = 0;
	int nextAlertIndex;
	int result = 0;
	bool foundResult = false;

	while (( alertsProcessed < numberOfAlerts ) && !foundResult)
	{
		bool newEntity = true;

		for ( int i = 0 ; i < numberOfAlerts ; i++ )
		{
			EntityAlert *ea = &alertQueue[i];

			if ( ea->ignore )
			{
				continue;
			}

			if ( newEntity )
			{
				// This first entry represents the highest alert level the AI reached.

				entityResponsible = ea->entityResponsible.GetEntity(); // the entity we're currently processing
				nextTime = ea->timeAlerted;	 // alert time
				nextAlertIndex = ea->alertIndex; // alert index
				//ea->processed = true;
				newEntity = false;
				alertsProcessed++;
				continue;
			}

			idEntity* thisEntityResponsible = ea->entityResponsible.GetEntity();

			if ( thisEntityResponsible != entityResponsible )
			{
				continue; // this entry wasn't caused by the entity we're processing
			}

			ea->ignore = true;
			alertsProcessed++;

			int thisAlertIndex = ea->alertIndex;
			int thisTime = ea->timeAlerted;

			// What is the time difference between this entry
			// and the previous entry (which happens later in time, because the
			// later entries appear earlier in the queue)?

			int duration = nextTime - thisTime;
			float alertDuration; // the average amount of time spent at a particular alert level (atime1, atime2, atime3, atime4)

			switch ( thisAlertIndex )
			{
			case ai::EObservant:
				alertDuration = atime1;
				break;
			case ai::ESuspicious:
				alertDuration = atime2;
				break;
			case ai::ESearching:
				alertDuration = atime3;
				break;
			case ai::EAgitatedSearching:
			case ai::ECombat:
				alertDuration = atime4;
				break;
			default:
				alertDuration = 100000.0f; // should never be here
				break;
			}

			alertDuration *= 1000 * 0.5f; // use half the average duration and convert to ms

			if ( alertDuration > 10000.0f ) // suggested by demagogue
			{
				alertDuration = 10000.f;
			}

			// If the latest alert is too close to the previous alert, we should remove
			// the stats for the previous alert.

			if ( duration < alertDuration )
			{
				result = thisAlertIndex;
				foundResult = true;
				break;
			}

			// set up for reading the next entry

			nextTime = thisTime;
			nextAlertIndex = thisAlertIndex;
		}
	}

/*	// Clear processed flags

	for ( int i = 0 ; i < numberOfAlerts ; i++ )
	{
		EntityAlert *ea = &alertQueue[i];
		ea->processed = false;
	}*/

	return result;
}

void idAI::Event_GetAttacker()	// grayman #3679
{
	idThread::ReturnEntity(GetMemory().attacker.GetEntity());
}

void idAI::Event_IsPlayerResponsibleForDeath() // grayman #3679
{
	idThread::ReturnInt(GetMemory().playerResponsible);
}


// grayman #3552 - get original acuity w/o applying factors like drunkeness

float idAI::GetBaseAcuity(const char *type) const
{
	float returnval(0);

	// Try to look up the ID in the hashindex. This corresponds to an entry in m_acuityNames.
	int ind = g_Global.m_AcuityHash.First( g_Global.m_AcuityHash.GenerateKey(type, false) );

	if ( ind == -1 )
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("AI %s attempted to query nonexistant acuity type: %s", GetName(), type);
		gameLocal.Warning("[AI] AI %s attempted to query nonexistant acuity type: %s", GetName(), type);
	}
	else if ( ind > m_Acuities.Num() )
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Acuity index %d exceeds acuity array size %d!\r", ind, m_Acuities.Num());
	}
	else
	{
		// Look up the acuity value using the index from the hashindex
		returnval = m_Acuities[ind];
	}

	// grayman #3682
	if ( idStr(type) == "aud" )
	{
		// Use the 'AI Hearing' choice on the gameplay menu
		// 0 = nearly deaf
		// 1 = forgiving
		// 2 = challenging
		// 3 = hardcore

		int hearingLevel = cv_ai_hearing.GetInteger(); // returns a number representing 'AI Hearing'
		float hearingFactor;

		switch (hearingLevel)
		{
		case 0:
			hearingFactor = cv_ai_hearing_nearly_deaf.GetFloat();
			break;
		case 1:
			hearingFactor = cv_ai_hearing_forgiving.GetFloat();
			break;
		default:
		case 2:
			hearingFactor = cv_ai_hearing_challenging.GetFloat();
			break;
		case 3:
			hearingFactor = cv_ai_hearing_hardcore.GetFloat();
			break;
		}

		returnval *= hearingFactor;
	}
	
	return returnval;
}

float idAI::GetAcuity(const char *type) const
{
	float returnval = GetBaseAcuity(type);

	/* grayman #3552 - this part is now done by GetBaseAcuity().

	// Try to lookup the ID in the hashindex. This corresponds to an entry in m_acuityNames.
	int ind = g_Global.m_AcuityHash.First( g_Global.m_AcuityHash.GenerateKey(type, false) );

	if (ind == -1 )
	{
		return returnval;
	}
	else if (ind > m_Acuities.Num())
	{
		return returnval;
	}

	// Look up the acuity value using the index from the hashindex
	returnval = m_Acuities[ind];

 */

	if (returnval <= 0 )
	{
		return 0;
	}

	// SZ: June 10, 2007
	// Acuities are now modified by alert level
//	if (returnval > 0.0)
//	{
		if (m_maxAlertLevel >= thresh_5)
		{
			returnval *= cv_ai_acuity_L5.GetFloat();
		}
		else if (m_maxAlertLevel >= thresh_4)
		{
			returnval *= cv_ai_acuity_L4.GetFloat();
		}
		else if (m_maxAlertLevel >= thresh_3)
		{
			returnval *= cv_ai_acuity_L3.GetFloat();
		}
//	}

	// angua: drunken AI have reduced acuity, unless they have seen evidence of intruders
	if ( m_drunk && !HasSeenEvidence() )
	{
		returnval *= spawnArgs.GetFloat("drunk_acuity_factor", "1");
	}

	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Acuity %s = %f\r", type, returnval);

	return returnval;
}

void idAI::SetAcuity( const char *type, float acuity )
{
	int ind;

	ind = g_Global.m_AcuityHash.First( g_Global.m_AcuityHash.GenerateKey( type, false ) );

	if (ind == -1)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Script on %s attempted to set nonexistant acuity type: %s\r",name.c_str(), type);
		gameLocal.Warning("[AI] Script on %s attempted to set nonexistant acuity type: %s",name.c_str(), type);
		goto Quit;
	}
	else if( ind > m_Acuities.Num() )
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Script on %s attempted to set acuity with an index %d greater than acuities array size %d!\r", ind, m_Acuities.Num() );
		goto Quit;
	}

	m_Acuities[ind] = acuity;

Quit:
	return;
}

idVec3 idAI::GetSndDir( void )
{
	return m_SoundDir;
}

idVec3 idAI::GetVisDir( void )
{
	return m_LastSight;
}

idEntity *idAI::GetTactEnt( void )
{
	return m_TactAlertEnt.GetEntity();
}

#if 1
// grayman #3492 - refactored PerformVisualScan() to provide a uniform increase in alert level

void idAI::PerformVisualScan(float timecheck)
{
	// Only perform enemy checks if we are in the player's PVS
	if ( ( GetAcuity("vis") <= 0 ) || !gameLocal.InPlayerPVS(this) )
	{
		return;
	}

	idActor* player = gameLocal.GetLocalPlayer();
	if (m_bIgnoreAlerts || player->fl.notarget || player->fl.invisible) // grayman #3857 - added 'invisible'
	{
		return;
	}

	// Ignore dead actors
	if ( player->health <= 0 )
	{
		return;
	}

	if (!CheckFOV(player->GetEyePosition()))
	{
		// if we can't see the player's eyes, maybe we can see his feet
		if (!CheckFOV(player->GetPhysics()->GetOrigin()))
		{
			return;
		}
	}

	// angua: does not take lighting and FOV into account
	if (!CanSeeExt(player, false, false))
	{
		return;
	}

	// grayman #3338 - if the player is not an enemy, process him like
	// a visual stim to the AI. this allows AI->player warnings and greetings
	if ( !IsEnemy(player) )
	{
		// Recheck visibility, taking light and FOV into account
		if (!CanSee(player, true))
		{
			return;
		}

		// grayman #3424 - immediate time checks to avoid a lot of processing in OnActorEncounter()
		ai::Memory::GreetingInfo& info = GetMemory().GetGreetingInfo(player);
		if ( ( gameLocal.time >= info.nextGreetingTime ) || ( gameLocal.time >= info.nextWarningTime ) )
		{
			mind->GetState()->OnActorEncounter(player,this);
		}
		return;
	}

	// Check the candidate's visibility.
	float vis = GetVisibility(player);

	if ( vis == 0.0f )
	{
		return; // AI can't see player
	}

	// greebo: At this point, the actor is identified as enemy and is visible
	// set AI_VISALERT and the vector for last sighted position
	// Store the position the enemy was visible
	m_LastSight = player->GetPhysics()->GetOrigin();
	AI_VISALERT = true;
	
	// Use the 'AI Vision' choice on the gameplay menu
	// 0 = nearly blind
	// 1 = forgiving
	// 2 = challenging
	// 3 = hardcore

	int visionLevel = cv_ai_vision.GetInteger(); // returns a number representing 'AI Vision'
	float visionFactor;

	switch (visionLevel)
	{
	case 0:
		visionFactor = cv_ai_vision_nearly_blind.GetFloat();
		break;
	default:
	case 1:
		visionFactor = cv_ai_vision_forgiving.GetFloat();
		break;
	case 2:
		visionFactor = cv_ai_vision_challenging.GetFloat();
		break;
	case 3:
		visionFactor = cv_ai_vision_hardcore.GetFloat();
		break;
	}

	float alertInc = visionFactor*vis;
	float newAlertLevel = AI_AlertLevel + alertInc;

	if (newAlertLevel >= thresh_5)
	{
		// grayman #3063
		// Allow ramp up to Combat mode if the distance to the player is less than a cutoff distance.
		// grayman #4348 - Also allow ramp up if the player is unreachable. Otherwise, the AI stand
		// still some distance away because they can't walk closer to the player.

		bool canWalkToPlayer = true;
		int playerAreaNum = PointReachableAreaNum(m_LastSight);
		if ( playerAreaNum == 0 )
		{
			canWalkToPlayer = false;
		}
		else
		{
			GetAAS()->PushPointIntoAreaNum(playerAreaNum, m_LastSight); // if this point is outside this area, it will be moved to one of the area's edges

			idVec3 aiOrigin = GetPhysics()->GetOrigin();
			int aiAreaNum = PointReachableAreaNum(aiOrigin, 1.0f);
			aasPath_t path;
			if ( !PathToGoal(path, aiAreaNum, aiOrigin, playerAreaNum, m_LastSight, this) )
			{
				canWalkToPlayer = false;
			}
		}
								
		if ( !canWalkToPlayer || ((m_LastSight - physicsObj.GetOrigin()).LengthFast()*s_DOOM_TO_METERS ) <= cv_ai_sight_combat_cutoff.GetFloat() )
		{
			SetEnemy(player);
			
			// grayman #3063 - set flag that tells UpDateEnemyPosition() to NOT count this instance of player visibility in the mission data
			// dragofer #5286 - only set this flag if the AI isn't fleeing, as unarmed civilians never enter the combat state
			if( GetMind()->GetState()->GetName() != "Flee" )
			{
				m_ignorePlayer = true; // totalTimePlayerSeen: ignore the player until Combat state begins
			}
		}
		else // player is too far away, but AI will continue to move because he can walk to the player
		{
			newAlertLevel = thresh_5 - 0.1;
			alertInc = newAlertLevel - AI_AlertLevel;
		}
	}

 	// If the alert amount is larger than everything else encountered this frame
	// ignore the previous alerts and remember this actor as enemy.
	if ( alertInc > m_AlertLevelThisFrame )
	{
		// Remember this actor
		m_AlertedByActor = player;

		Event_AlertAI("vis", alertInc, player);

		// Call the visual alert handler on the current state
		mind->GetState()->OnVisualAlert(player);
	}
}

#else

// grayman #3492 - original
void idAI::PerformVisualScan(float timecheck)
{
	// Only perform enemy checks if we are in the player's PVS
	if (GetAcuity("vis") <= 0 || !gameLocal.InPlayerPVS(this))
	{
		return;
	}

	idActor* player = gameLocal.GetLocalPlayer();
	if (m_bIgnoreAlerts || player->fl.notarget)
	{
		// notarget
		return;
	}

	// Ignore dead actors
	// grayman #3338 - move enemy test below, beyond the point
	// where we know we can see him. this allows for greetings.
//	if (player->health <= 0 || !IsEnemy(player))
	if ( player->health <= 0 )
	{
		return;
	}

	if (!CheckFOV(player->GetEyePosition()))
	{
		return;
	}

	// Check the candidate's visibility.
	float visFrac = GetVisibility(player);
	// Do the percentage check
	float randFrac = gameLocal.random.RandomFloat();
	float chance = timecheck / s_VisNormtime * cv_ai_sight_prob.GetFloat() * visFrac;
	if ( randFrac > chance )
	{
		return;
	}

	// angua: does not take lighting and FOV into account
	if (!CanSeeExt(player, false, false))
	{
		return;
	}

	// grayman #3338 - if the player is not an enemy, process him like
	// a visual stim to the AI. this allows AI->player warnings and greetings
	if ( !IsEnemy(player) )
	{
		// Recheck visibility, taking light and FOV into account
		if (!CanSee(player, true))
		{
			return;
		}

		// grayman #3424 - immediate time checks to avoid a lot of processing in OnActorEncounter()
		ai::Memory::GreetingInfo& info = GetMemory().GetGreetingInfo(player);
		if ( ( gameLocal.time >= info.nextGreetingTime ) || ( gameLocal.time >= info.nextWarningTime ) )
		{
			mind->GetState()->OnActorEncounter(player,this);
		}
		return;
	}

	// greebo: At this point, the actor is identified as enemy and is visible
	// set AI_VISALERT and the vector for last sighted position
	// Store the position the enemy was visible
	m_LastSight = player->GetPhysics()->GetOrigin();
	AI_VISALERT = true;
	
	// Get the visual alert amount caused by the CVAR setting
	// float incAlert = GetPlayerVisualStimulusAmount();

	// angua: alert increase depends on brightness, distance and acuity
	float incAlert = 4 + 9 * visFrac;

	if (cv_ai_visdist_show.GetFloat() > 0) 
	{
		gameRenderWorld->DebugText("see you!", GetEyePosition() + idVec3(0,0,70), 0.2f, idVec4( 0.5f, 0.00f, 0.00f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 60 * USERCMD_MSEC);
		gameRenderWorld->DebugText(va("Alert increase: %.2f", incAlert), GetEyePosition() + idVec3(0,0,60), 0.2f, idVec4( 0.5f, 0.00f, 0.00f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 60 * USERCMD_MSEC);
	}

	float newAlertLevel = AI_AlertLevel + incAlert;
	if (newAlertLevel > thresh_5)
	{
		// grayman #3063
		// Allow ramp up to Combat mode if the distance to the player is less than a cutoff distance.

		if ( ((m_LastSight - physicsObj.GetOrigin()).LengthFast()*s_DOOM_TO_METERS ) <= cv_ai_sight_combat_cutoff.GetFloat() )
		{
			SetEnemy(player);
			m_ignorePlayer = true; // grayman #3063 - don't count this instance for mission statistics (defer until Combat state begins)

			// set flag that tells UpDateEnemyPosition() to NOT count this instance of player
			// visibility in the mission data
		}
		else
		{
			newAlertLevel = thresh_5 - 0.1;
			incAlert = newAlertLevel - AI_AlertLevel;
		}
	}

	// If the alert amount is larger than everything else encountered this frame
	// ignore the previous alerts and remember this actor as enemy.
	if (incAlert > m_AlertLevelThisFrame)
	{
		// Remember this actor
		m_AlertedByActor = player;

		PreAlertAI("vis", incAlert, player->GetEyePosition()); // grayman #3356

		// Call the visual alert handler on the current state
		mind->GetState()->OnVisualAlert(player);
	}

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AI %s SAW actor %s\r", name.c_str(), player->name.c_str() );

	return;
}
#endif

#if 1
// grayman #3492 - refactored GetVisibility to provide a uniform increase in alert level

float idAI::GetVisibility( idEntity *ent ) const
{
	// Returns a visibility factor per frame
	
	// for now, only players may have their visibility checked
	if (!ent->IsType( idPlayer::Type ))
	{
		return 0.0f;
	}

	idPlayer* player = static_cast<idPlayer*>(ent);

	// this depends only on the brightness of the light gem and the AI's visual acuity
	float clampVal = GetVisFraction();
	float clampdist = cv_ai_sightmindist.GetFloat() * clampVal;
	float safedist = clampdist + (cv_ai_sightmaxdist.GetFloat() - cv_ai_sightmindist.GetFloat()) * clampVal;

	idVec3 delta = GetEyePosition() - player->GetEyePosition();
	float dist = delta.LengthFast()*s_DOOM_TO_METERS;

	if (dist > clampdist) 
	{
		if (dist >= safedist)
		{
			clampVal = 0.0f;
		}
		else
		{
			clampVal *= ( 1.0f - (dist - clampdist)/(safedist - clampdist) );
		}
	}

	if (cv_ai_visdist_show.GetFloat() > 0) 
	{
		idStr alertText(clampVal);
		alertText = "clampVal: "+ alertText;
		gameRenderWorld->DebugText(alertText.c_str(), GetEyePosition() + idVec3(0,0,1), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		idStr alertText2(clampdist);
		alertText2 = "clampdist: "+ alertText2;
		gameRenderWorld->DebugText(alertText2.c_str(), GetEyePosition() + idVec3(0,0,10), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		gameRenderWorld->DebugCircle(idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), GetPhysics()->GetOrigin(),idVec3(0,0,1), clampdist / s_DOOM_TO_METERS, 100, USERCMD_MSEC);
		idStr alertText3(safedist);
		alertText3 = "safedist: "+ alertText3;
		gameRenderWorld->DebugText(alertText3.c_str(), GetEyePosition() + idVec3(0,0,20), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		gameRenderWorld->DebugCircle(idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), GetPhysics()->GetOrigin(),idVec3(0,0,1), safedist / s_DOOM_TO_METERS, 100, USERCMD_MSEC);
		idStr alertText4(dist);
		alertText4 = "distance: "+ alertText4;
		gameRenderWorld->DebugText(alertText4.c_str(), GetEyePosition() + idVec3(0,0,30), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
	}

	return clampVal;
}

#else

// grayman #3492 - original

float idAI::GetVisibility( idEntity *ent ) const
{
	// Returns the probability that the entity will be seen within half a second
	// depends on light gem brightness, visual acuity and player distance
	
	float returnval(0);

	// for now, only players may have their visibility checked
	if (!ent->IsType( idPlayer::Type ))
	{
		return returnval;
	}
	idPlayer* player = static_cast<idPlayer*>(ent);

	// angua: probability for being seen (within half a second)
	// this depends only on the brightness of the light gem and the AI's visual acuity
	float clampVal = GetCalibratedLightgemValue();

	// angua: within the clampDist, the probability stays constant
	// the probability decreases linearly towards 0 between clampdist and savedist
	// both distances are scaled with clampVal

	float clampdist = cv_ai_sightmindist.GetFloat() * clampVal;
	float safedist = clampdist + (cv_ai_sightmaxdist.GetFloat() - cv_ai_sightmindist.GetFloat()) * clampVal;

	idVec3 delta = GetEyePosition() - player->GetEyePosition();
	float dist = delta.Length()*s_DOOM_TO_METERS;

	if (dist <= clampdist) 
	{
		returnval = clampVal;
	}
	else 
	{
		if (dist >= safedist) 
		{
			returnval = 0;
		}
		else
		{
			returnval = clampVal * (1 - (dist-clampdist)/(safedist-clampdist) );
		}
	}

	if (cv_ai_visdist_show.GetFloat() > 0) 
	{
		idStr alertText(clampVal);
		alertText = "clampVal: "+ alertText;
		gameRenderWorld->DebugText(alertText.c_str(), GetEyePosition() + idVec3(0,0,1), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		idStr alertText2(clampdist);
		alertText2 = "clampdist: "+ alertText2;
		gameRenderWorld->DebugText(alertText2.c_str(), GetEyePosition() + idVec3(0,0,10), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		gameRenderWorld->DebugCircle(idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), GetPhysics()->GetOrigin(),idVec3(0,0,1), clampdist / s_DOOM_TO_METERS, 100, USERCMD_MSEC);
		idStr alertText3(safedist);
		alertText3 = "savedist: "+ alertText3;
		gameRenderWorld->DebugText(alertText3.c_str(), GetEyePosition() + idVec3(0,0,20), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		gameRenderWorld->DebugCircle(idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), GetPhysics()->GetOrigin(),idVec3(0,0,1), safedist / s_DOOM_TO_METERS, 100, USERCMD_MSEC);
		idStr alertText4(returnval);
		alertText4 = "returnval: "+ alertText4;
		gameRenderWorld->DebugText(alertText4.c_str(), GetEyePosition() + idVec3(0,0,30), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		idStr alertText5(dist);
		alertText5 = "distance: "+ alertText5;
		gameRenderWorld->DebugText(alertText5.c_str(), GetEyePosition() + idVec3(0,0,-10), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
	}
	
	return returnval;
}
#endif

float idAI::GetVisFraction() const
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if (player == NULL)
	{
		return 0.0f;
	}

	float clampVal = player->GetCalibratedLightgemValue();
	clampVal *= GetAcuity("vis");

	/* grayman #3492 - allow values > 1
	if (clampVal > 1)
	{
		clampVal = 1;
	} */

	// Debug output
	if (cv_ai_visdist_show.GetFloat() > 0) 
	{
		float lgem = static_cast<float>(player->GetCurrentLightgemValue());
		idStr alertText5(lgem);
		alertText5 = "lgem: "+ alertText5;
		gameRenderWorld->DebugText(alertText5.c_str(), GetEyePosition() + idVec3(0,0,40), 0.2f, idVec4( 0.15f, 0.15f, 0.15f, 1.00f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
	}

	return clampVal;
}

void idAI::TactileAlert(idEntity* tactEnt, float amount)
{
	if (AI_DEAD || AI_KNOCKEDOUT || m_bIgnoreAlerts)
	{
		return;
	}

	if ( tactEnt == NULL )
	{
		return;
	}

	if ( CheckTactileIgnore(tactEnt) )
	{
		return;
	}

	// grayman #2872 - have we encountered a dangling rope from a rope arrow?

	if ( tactEnt->IsType(idAFEntity_Generic::Type) )
	{
		if ( idStr::FindText( tactEnt->name, "env_rope" ) >= 0 )
		{
			// Ignore the rope if you have an enemy.

			if ( GetEnemy() == NULL )
			{
				// Find the bindMaster for this rope, then see if there's a CProjectileResult bound to it.

				idEntity* bindMaster = tactEnt->GetBindMaster();
				if ( bindMaster != NULL )
				{
					idEntity* stimSource = bindMaster->FindMatchingTeamEntity( CProjectileResult::Type );
					if ( stimSource != NULL )
					{
						if ( !stimSource->CheckResponseIgnore(ST_VISUAL,this) )	// only react if you haven't encountered this rope before,
																				// either by bumping it or receiving its stim
						{
							// What's the chance of noticing this rope? If it's zero, you're to ignore the rope.
							// If it's greater than zero, and even if it's less than 1.0, you walked into the
							// rope, so we expect you to notice it.

							float chanceToNotice = spawnArgs.GetFloat("chanceNoticeRope","0.0");
							if ( chanceToNotice > 0.0 )
							{
								if ( mind->GetState()->ShouldProcessAlert(ai::EAlertTypeRope) )
								{
									mind->GetState()->OnVisualStimRope(stimSource,this,GetEyePosition());
								}
							}
							else // This rope stim should stop sending me stims
							{
								stimSource->IgnoreResponse(ST_VISUAL, this);
							}
						}
					}
				}
			}
		}

		return;
	}

	// The actor is either the touched entity or the originator of the tactile alert
	idActor* responsibleActor = 
		(tactEnt->IsType(idActor::Type)) ? static_cast<idActor*>(tactEnt) : tactEnt->m_SetInMotionByActor.GetEntity();

	if ( responsibleActor == NULL )
	{
		return;
	}

	// grayman #2816 - separate into actors and non-actors

	if ( !tactEnt->IsType(idActor::Type) )
	{
		// non-actors

		// Since kicking objects is handled elsewhere, this is
		// an object that most likely was thrown at the AI by
		// the player.

		// We don't care whether the actor responsible for the
		// struck object is a friend, neutral, or an enemy. We
		// need to react to something hitting us.

		// grayman #3075 - if we bumped something that was dropped
		// by an AI, ignore it from now on to avoid
		// re-alerting us every time we touch it

		if ( tactEnt->m_droppedByAI )
		{
			TactileIgnore(tactEnt);
			return;
		}

		// If we put this object in motion, ignore it

		if ( tactEnt->m_SetInMotionByActor.GetEntity() == this )
		{
			//TactileIgnore(tactEnt); // grayman #4304 - ignore this time, but not forever
			return;
		}
	}
	else // actors
	{
		if ( IsFriend(responsibleActor) )
		{
			// grayman #3714 - this check for health and consciousness was removed
			// for issue #3679, but I don't remember doing it. Will need to make
			// sure #3679 doesn't start failing now that I've put the check back.
			if ( ( responsibleActor->health <= 0 ) || responsibleActor->IsKnockedOut() )
			{
				// angua: We've found a friend that is dead or unconscious
				mind->GetState()->OnActorEncounter(tactEnt, this);
			}
		}

		if ( !IsEnemy(responsibleActor) ) 
		{
			return; // not an enemy, no alert
		}

		// greebo: We touched an enemy, check if it's an unconscious body or corpse
		if (tactEnt->IsType(idAI::Type) && 
			(static_cast<idAI*>(tactEnt)->AI_DEAD || static_cast<idAI*>(tactEnt)->AI_KNOCKEDOUT))
		{
			// When AI_DEAD or AI_KNOCKEDOUT ignore this alert from now on to avoid
			// re-alerting us every time we touch it again and again
			TactileIgnore(tactEnt);
		}
	}

	// grayman #2816 - Do we need to react to getting hit by a moveable?

	if ( tactEnt->IsType(idMoveable::Type) )
	{
		// grayman #4304 - ignore the moveable if it isn't traveling fast enough to notice
		if ( tactEnt->GetPhysics()->GetLinearVelocity().LengthFast() < MIN_SPEED_TO_NOTICE_MOVEABLE)
		{
			return;
		}

		// Ignore the moveable if you have an enemy.

		if ( GetEnemy() == NULL )
		{
			if ( !m_ReactingToHit )
			{
				// Wait a bit to turn toward and look at what hit you.
				// Then turn back in the direction the object came from.

				mind->GetState()->OnHitByMoveable(this, tactEnt); // sets m_ReactingToHit to TRUE
			}
			else if ( GetMemory().hitByThisMoveable.GetEntity() != tactEnt ) // hit by something different?
			{
				GetMemory().stopReactingToHit = true; // stop the current reaction
			}
		}
		return; // process moveable hit next time around
	}

	// grayman #3756 - Do we need to react to getting hit by a door?

	if ( tactEnt->IsType(CFrobDoor::Type) )
	{
		// Ignore the door if you have an enemy.

		if ( GetEnemy() == NULL )
		{
			// If handling a door, ignore getting hit by one. We don't need
			// the reaction to getting hit by a door screwing with the
			// door handling task.

			if (m_HandlingDoor)
			{
				return;
			}

			// Getting hit by a door overrides your reaction to other events.

			GetMemory().StopReacting();

			// React to the door hitting you.

			// Look toward the door center for a moment before turning toward the door.

			float duration = 1.5f;
			Event_LookAtPosition(static_cast<CBinaryFrobMover*>(tactEnt)->GetClosedBox().GetCenter(),duration);
			PostEventSec(&AI_OnHitByDoor,duration,tactEnt);
		}
		return;
	}

	// grayman #2816 - if this is the last AI you killed, ignore it
	if ( tactEnt->IsType(idAI::Type) && ( tactEnt == m_lastKilled.GetEntity() ) )
	{
		return;
	}

	// Set the alert amount according to the tactile alert value
	if ( amount == -1 )
	{
		if ( tactEnt->IsType(idActor::Type) ) // grayman #3857 - only use this if you touched an actor
		{
			amount = cv_ai_tactalert.GetFloat();
		}
		else
		{
			amount = 0;
		}
	}

	// If we got this far, we give the alert
	// NOTE: Latest tactile alert always overides other alerts
	m_TactAlertEnt = tactEnt;
	m_AlertedByActor = responsibleActor;

	amount *= GetAcuity("tact");
	// grayman #3009 - pass the location of the player's eyes so the AI can look at them
	idVec3 lookAtPos(0,0,0);
	if ( tactEnt->IsType(idActor::Type) )
	{
		lookAtPos = static_cast<idActor*>(tactEnt)->GetEyePosition();
	}
	else
	{
		lookAtPos = tactEnt->GetPhysics()->GetOrigin();
	}
	PreAlertAI("tact", amount, lookAtPos); // grayman #3356

	// Notify the currently active state
	mind->GetState()->OnTactileAlert(tactEnt);

	// Set last visual contact location to this location as that is used in case
	// the target gets away
	m_LastSight = tactEnt->GetPhysics()->GetOrigin();

	// If no enemy set so far, set the last visible enemy position.
	if (GetEnemy() == NULL)
	{
		lastVisibleEnemyPos = tactEnt->GetPhysics()->GetOrigin();
	}

	AI_TACTALERT = true;

	if ( cv_ai_debug.GetBool() )
	{
		// Note: This can spam the log a lot, so only put it in if cv_ai_debug.GetBool() is true
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AI %s FELT entity %s\r", name.c_str(), tactEnt->name.c_str() );
		gameLocal.Printf( "[DM AI] AI %s FELT entity %s\n", name.c_str(), tactEnt->name.c_str() );
	}
}

void idAI::TactileIgnore(idEntity* tactEnt)
{
	tactileIgnoreEntities.insert(tactEnt);
}

bool idAI::CheckTactileIgnore(idEntity* tactEnt)
{
	TactileIgnoreList::iterator i = tactileIgnoreEntities.find(tactEnt);
	if ( i != tactileIgnoreEntities.end() )
	{
		return true;
	}

	// grayman #3009 - ignore all tactile alerts while getting up

	if ( ( move.moveType == MOVETYPE_GET_UP ) || ( move.moveType == MOVETYPE_WAKE_UP ) ) // grayman #3820 - MOVETYPE_WAKE_UP was MOVETYPE_GET_UP_FROM_LYING
	{
		return true;
	}

	return false;
}

idActor *idAI::FindEnemy(bool useFOV)
{
	// Only perform enemy checks if we are in the player's PVS
	if (!gameLocal.InPlayerPVS(this))
	{
		return NULL;
	}

	// Go through all clients (=players)
	for (int i = 0; i < gameLocal.numClients ; i++)
	{
		idEntity* ent = gameLocal.entities[i];

		if ( (ent == NULL) || ent->fl.notarget || ent->fl.invisible || !ent->IsType(idActor::Type)) // grayman #3857 - added 'invisible'
		{
			// NULL, notarget or non-Actor or invisible, continue
			continue;
		}

		idActor* actor = static_cast<idActor*>(ent);

		// Ignore dead actors or non-enemies
		if (actor->health <= 0 || !IsEnemy(actor))
		{
			continue;
		}

		// angua: does not take lighting into account any more, 
		// this is done afterwards using GetVisibility
		if (CanSeeExt(actor, useFOV, false))
		{
			// Enemy actor found and visible, return it 
			return actor;
		}
	}

	return NULL;
}

idActor* idAI::FindEnemyAI(bool useFOV)
{
	pvsHandle_t pvs = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );

	float bestDist = idMath::INFINITY;
	idActor* bestEnemy = NULL;

	for ( auto iter = gameLocal.activeEntities.Begin(); iter; gameLocal.activeEntities.Next(iter) )
	{
		idEntity *ent = iter.entity;

		if ( ent->fl.hidden || ent->fl.isDormant || ent->fl.notarget || ent->fl.invisible || !ent->IsType( idActor::Type ) ) // grayman #3857 - also use 'invisible'
		{
			continue;
		}

		idActor* actor = static_cast<idActor *>( ent );
		if ( ( actor->health <= 0 ) || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) {
			continue;
		}

		if ( !gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas() ) ) {
			continue;
		}

		idVec3 delta = physicsObj.GetOrigin() - actor->GetPhysics()->GetOrigin();
		float dist = delta.LengthSqr();
		if ( ( dist < bestDist ) && CanSee( actor, useFOV != 0 ) ) {
			bestDist = dist;
			bestEnemy = actor;
		}
	}

	gameLocal.pvs.FreeCurrentPVS(pvs);
	return bestEnemy;
}

idActor* idAI::FindFriendlyAI(int requiredTeam)
{
// This is our return value
	idActor* candidate(NULL);
	// The distance of the nearest found AI
	float bestDist = idMath::INFINITY;

	// Setup the PVS areas of this entity using the PVSAreas set, this returns a handle
	pvsHandle_t pvs(gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas()));

	// Iterate through all active entities and find an AI with the given team.
	for ( auto iter = gameLocal.activeEntities.Begin(); iter; gameLocal.activeEntities.Next(iter) )
	{
		idEntity *ent = iter.entity;
		if ( ent == this || ent->fl.hidden || ent->fl.isDormant || !ent->IsType( idActor::Type ) ) {
			continue;
		}

		idActor* actor = static_cast<idActor *>(ent);
		if (actor->health <= 0) {
			continue;
		}

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Taking actor %s into account\r", actor->name.c_str());

		if (requiredTeam != -1 && actor->team != requiredTeam) {
			// wrong team
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Taking actor %s has wrong team: %d\r", actor->name.c_str(), actor->team);
			continue;
		}

		if (!IsFriend(actor))
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Actor %s is not on friendly team: %d or has specific relation that is not friendly\r", actor->name.c_str(), actor->team);
			// Not friendly
			continue;
		}

		if (!gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas())) {
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Actor %s is not in PVS\r", actor->name.c_str());
			// greebo: This actor is not in our PVS, skip it
			continue;
		}

		float dist = (physicsObj.GetOrigin() - actor->GetPhysics()->GetOrigin()).LengthSqr();
		if ( (dist < bestDist) && CanSee(actor, true) ) {
			// Actor can be seen and is nearer than the best candidate, save it
			bestDist = dist;
			candidate = actor;
		}
	}

	gameLocal.pvs.FreeCurrentPVS(pvs);
	
	return candidate;

}

/*---------------------------------------------------------------------------------*/

float idAI::GetMaximumObservationDistanceForPoints(const idVec3& p1, const idVec3& p2) const
{
	return LAS.queryLightingAlongLine(p1, p2, NULL, true) * cv_ai_sight_scale.GetFloat() * GetAcuity("vis");
}

float idAI::GetMaximumObservationDistance(idEntity* entity) const
{
	assert(entity != NULL); // don't accept NULL input

	float lightQuotient = entity->GetLightQuotient();
	return lightQuotient * cv_ai_sight_scale.GetFloat() * GetAcuity("vis");
}

/*---------------------------------------------------------------------------------*/
/*
float idAI::GetPlayerVisualStimulusAmount() const
{
	float alertAmount = 0.0;

	// Quick fix for blind AI:
	if (GetAcuity("vis") > 0 )
	{
		// float visFrac = GetVisibility( p_playerEntity );
		// float lgem = (float) g_Global.m_DarkModPlayer->m_LightgemValue;
		// Convert to alert units ( 0.6931472 = ln(2) )
		// Old method, commented out
		// alertAmount = 4*log( visFrac * lgem ) / 0.6931472;

		// The current alert number is stored in logarithmic units
		float curAlertLog = AI_AlertLevel;

		// convert current alert from log to linear scale, add, then convert back
		// this might not be as good for performance, but it lets us keep all alerts
		// on the same scale.
		if (curAlertLog > 0)
		{
			// greebo: Convert log to lin units by using Alin = 2^[(Alog-1)/10]
			float curAlertLin = idMath::Pow16(2, (curAlertLog - 1)*0.1f);

			// Increase the linear alert number with the cvar
			curAlertLin += cv_ai_sight_mag.GetFloat();

			// greebo: Convert back to logarithmic alert units
			// The 1.44269 in the equation is the 1/ln(2) used to compensate for using the natural Log16 method.
			curAlertLog = 1 + 10.0f * idMath::Log16(curAlertLin) * 1.442695f;

			// Now calculate the difference in logarithmic units and return it
			alertAmount = curAlertLog - AI_AlertLevel;
		}
		else
		{
			alertAmount = 1;
		}
	}

	return alertAmount;
}
*/
/*---------------------------------------------------------------------------------*/

bool idAI::IsEntityHiddenByDarkness(idEntity* p_entity, const float sightThreshold) const
{
	// Quick test using LAS at entity origin
	idPhysics* p_physics = p_entity->GetPhysics();

	if (p_physics == NULL) 
	{
		return false;	// Not in darkness
	}

	// Use lightgem if it is the player
	if (p_entity->IsType(idPlayer::Type))
	{
		// Get the alert increase amount (log) caused by the CVAR tdm_ai_sight_mag
		// greebo: Commented this out, this is not suitable to detect if player is hidden in darkness
		//float incAlert = GetPlayerVisualStimulusAmount();
		
		// greebo: Check the visibility of the player depending on lgem and visual acuity
		float visFraction = GetVisFraction(); // returns values in [0..1]
/*
		// greebo: Debug output, comment me out
		gameRenderWorld->DebugText(idStr(visFraction), GetEyePosition() + idVec3(0,0,1), 0.11f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
*/
		// Draw debug graphic
		/*if (cv_ai_visdist_show.GetFloat() > 1.0)
		{
			idVec4 markerColor (0.0, 0.0, 0.0, 0.0);

			float percToSeen = 1.0;
			if (sightThreshold > 0 && sightThreshold > incAlert)
			{
				percToSeen = (sightThreshold - incAlert) / sightThreshold;
			}

			// Scale red to green from not perceptable to quickly perceptable
			markerColor.x = idMath::Sin (percToSeen * (idMath::PI / 2.0));
			markerColor.y = idMath::Cos (percToSeen * (idMath::PI / 2.0));

			idVec3 observeFrom = GetEyePosition();

			gameRenderWorld->DebugArrow(markerColor, observeFrom, p_physics->GetOrigin(), 2, cv_ai_visdist_show.GetInteger());
		}*/

		// Very low threshold for visibility
		if (visFraction < sightThreshold)
		{
			// Not visible, entity is hidden in darkness
			return true;
		}

		// Visible, visual stim above threshold
		return false;
	}
	else // Not the player
	{
		float maxDistanceToObserve = GetMaximumObservationDistance(p_entity);

		// Are we close enough to see it in the current light level?
		idVec3 observeFrom = GetEyePosition();
		idVec3 midPoint = p_entity->GetPhysics()->GetAbsBounds().GetCenter();

		if ( (observeFrom - midPoint).LengthSqr() > Square(maxDistanceToObserve) )
		{
			// Draw debug graphic?
			if (cv_ai_visdist_show.GetFloat() > 1.0f)
			{
				idVec3 arrowLength = midPoint - observeFrom;
				arrowLength.Normalize();
				arrowLength *= maxDistanceToObserve;

				// Distance we could see
				gameRenderWorld->DebugArrow(colorRed, observeFrom, observeFrom + arrowLength, 2, cv_ai_visdist_show.GetInteger());

				// Gap to where we want to see
				gameRenderWorld->DebugArrow(colorMagenta, observeFrom + arrowLength, midPoint, 2, cv_ai_visdist_show.GetInteger());
			}

			return true; // hidden by darkness
		}

		// Draw debug graphic?
		if (cv_ai_visdist_show.GetFloat() > 1.0f)
		{
			// We can see to target
			gameRenderWorld->DebugArrow(colorGreen, observeFrom, midPoint, 2, cv_ai_visdist_show.GetInteger());
		}

		return false; // not hidden by darkness
	}
}

/*---------------------------------------------------------------------------------*/

idActor *idAI::FindNearestEnemy( bool useFOV )
{
	idActor		*actor, *playerEnemy;
	idActor		*bestEnemy;
	float		bestDist;
	float		dist;
	idVec3		delta;
	pvsHandle_t pvs;

	pvs = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );

	bestDist = idMath::INFINITY;
	bestEnemy = NULL;

	for ( auto iter = gameLocal.activeEntities.Begin(); iter; gameLocal.activeEntities.Next(iter) )
	{
		idEntity *ent = iter.entity;
		if ( ent->fl.hidden || ent->fl.isDormant || !ent->IsType( idActor::Type ) )
		{
			continue;
		}

		actor = static_cast<idActor *>( ent );
		if ( ( actor->health <= 0 ) || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) )
		{
			continue;
		}

		if ( !gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas() ) )
		{
			continue;
		}

		delta = physicsObj.GetOrigin() - actor->GetPhysics()->GetOrigin();
		dist = delta.LengthSqr();
		if ( ( dist < bestDist ) && CanSee( actor, useFOV ) ) {
			bestDist = dist;
			bestEnemy = actor;
		}
	}

	playerEnemy = FindEnemy(false);
	if( !playerEnemy )
		goto Quit;

	delta = physicsObj.GetOrigin() - playerEnemy->GetPhysics()->GetOrigin();
	dist = delta.LengthSqr();

	if( dist < bestDist )
		bestEnemy = playerEnemy;
		bestDist = dist;

Quit:
	gameLocal.pvs.FreeCurrentPVS( pvs );
	return bestEnemy;
}



void idAI::HadTactile( idActor *actor )
{
	if( !actor )
		goto Quit;

	if( IsEnemy( actor) )
		TactileAlert( actor );
	else
	{
		// TODO: FLAG BLOCKED BY FRIENDLY SO THE SCRIPT CAN DO SOMETHING ABOUT IT

		// grayman #2728 - If blocked by an AI of small mass, kick it aside immediately. If you leave it to normal
		// block resolution, the larger AI will appear to be momentarily stopped by the small AI,
		// which is not what you'd expect to see.

		float actorMass = actor->GetPhysics()->GetMass();
		if ((actorMass <= SMALL_AI_MASS) && (physicsObj.GetMass() > SMALL_AI_MASS))
		{
			if (gameLocal.time >= actor->m_nextKickTime) // but only if not recently kicked
			{
				KickObstacles(viewAxis[0],1.25*kickForce*actorMass,actor);
				actor->m_nextKickTime = gameLocal.time + 1000;	// need to wait before kicking again,
																// otherwise kicks can build up over adjacent frames
			}
		}
	}

	// alert both AI if they bump into each other
	if( actor->IsEnemy(this)
		&& actor->IsType(idAI::Type) )
	{
		static_cast<idAI *>(actor)->TactileAlert( this );
	}

Quit:
	return;
}

/*
=====================
idAI::GetMovementVolMod
=====================
*/

float idAI::GetMovementVolMod( void )
{
	float returnval;
	bool bCrouched(false);

	if( AI_CROUCH )
		bCrouched = true;

	// figure out which of the 6 cases we have:
	if( !AI_RUN && !AI_CREEP )
	{
		if( !bCrouched )
			returnval = m_stepvol_walk;
		else
			returnval = m_stepvol_crouch_walk;
	}

	// NOTE: running always has priority over creeping
	else if( AI_RUN )
	{
		if( !bCrouched )
			returnval = m_stepvol_run;
		else
			returnval = m_stepvol_crouch_run;
	}

	else if( AI_CREEP )
	{
		if( !bCrouched )
			returnval = m_stepvol_creep;
		else
			returnval = m_stepvol_crouch_creep;
	}

	else
	{
		// something unexpected happened
		returnval = 0;
	}

	return returnval;
}

/*
=====================
idAI::CheckTactile

Modified 5/25/06 , removed trace computation, found better way of checking
=====================
*/
void idAI::CheckTactile()
{
	// Only check tactile alerts if we aren't Dead, KO'ed or already engaged in combat.

	// grayman #2345 - changed to handle the waiting, non-solid state for AI

	bool bumped = false;
	idEntity* blockingEnt = physicsObj.GetSlideMoveEntity(); // grayman #3516 - always remember this

	if ( blockingEnt == NULL )
	{
		return;
	}

	if ( blockingEnt != gameLocal.world ) // grayman #4346
	{
		m_tactileEntity = blockingEnt;
	}

	if ( !AI_KNOCKEDOUT &&
		 !AI_DEAD &&
		 (AI_AlertLevel < thresh_5) &&
		 !movementSubsystem->IsWaitingNonSolid() ) // no bump if I'm waiting
	{
		if ( blockingEnt != gameLocal.world ) // ignore bumping into the world
		{
			//m_tactileEntity = blockingEnt; // grayman #3993 // grayman #4346
				
			if ( blockingEnt->IsType(idAI::Type) )
			{
				idAI* blockingAI = static_cast<idAI*>(blockingEnt);

				// grayman #2903 - if both AI are examining a rope, it's possible that
				// they're both trying to reach the same examination spot. If one is not
				// moving, he's examining, so the other has to stop where he is. This
				// prevents the one standing from turning non-solid and allowing the other
				// to merge with him. Looks bad.

				if ( m_ExaminingRope && blockingAI->m_ExaminingRope )
				{
					if ( AI_FORWARD && !blockingAI->AI_FORWARD )
					{
						StopMove(MOVE_STATUS_DONE);
					}
					else if ( !AI_FORWARD && blockingAI->AI_FORWARD )
					{
						blockingAI->StopMove(MOVE_STATUS_DONE);
					}
				}
			}
		}
	}

	if (blockingEnt->IsType(idPlayer::Type)) // player has no movement subsystem
	{
		// aesthetics: Don't react to dead player?
		if (blockingEnt->health > 0)
		{
			bumped = true;
		}
	}
	else if (blockingEnt->IsType(idActor::Type))
	{
		idAI *e = static_cast<idAI*>(blockingEnt);
		if (e && !e->movementSubsystem->IsWaitingNonSolid()) // no bump if other entity is waiting
		{
			bumped = true;
		}
	}

	if (bumped)
	{
		HadTactile(static_cast<idActor*>(blockingEnt));
	}
	else
	{
		// grayman #3516 - If the blocking object is being carried by the player,
		// drop it from the player's hands.

		if ( blockingEnt != gameLocal.world )
		{
			blockingEnt->CheckCollision(this);
		}
	}
}

bool idAI::HasSeenEvidence() const
{
	ai::Memory& memory = GetMemory();

	return memory.enemiesHaveBeenSeen
		|| memory.hasBeenAttackedByEnemy
		|| memory.itemsHaveBeenStolen
		|| memory.itemsHaveBeenBroken
		|| memory.unconsciousPeopleHaveBeenFound
		|| memory.deadPeopleHaveBeenFound
		|| spawnArgs.GetBool("alert_idle", "0");
}

void idAI::HasEvidence( EventType type )
{
	ai::Memory& memory = GetMemory();
	bool printMsg = cv_ai_debug_transition_barks.GetBool();

	switch(type)
	{
	case E_EventTypeEnemy:
		memory.enemiesHaveBeenSeen = true;
		if (printMsg)
		{
			gameLocal.Printf("%d: %s is aware that enemies have been seen, will use Alert Idle\n",gameLocal.time,GetName());
		}
		break;
	case E_EventTypeDeadPerson:
		memory.deadPeopleHaveBeenFound = true;
		if (printMsg)
		{
			gameLocal.Printf("%d: %s is aware that dead people have been seen, will use Alert Idle\n",gameLocal.time,GetName());
		}
		break;
	case E_EventTypeUnconsciousPerson:
		memory.unconsciousPeopleHaveBeenFound = true;
		if (printMsg)
		{
			gameLocal.Printf("%d: %s is aware that unconscious people have been seen, will use Alert Idle\n",gameLocal.time,GetName());
		}
		break;
	case E_EventTypeMissingItem:
		memory.itemsHaveBeenStolen = true;
		if (printMsg)
		{
			gameLocal.Printf("%d: %s is aware that an item has been stolen, will use Alert Idle\n",gameLocal.time,GetName());
		}
		break;
	default:
		break;
	}
}

/**
* ========================== BEGIN TDM KNOCKOUT CODE =============================
**/

/*
=====================
idAI::TestKnockoutBlow
=====================
*/

bool idAI::TestKnockoutBlow( idEntity* attacker, const idVec3& dir, trace_t *tr, int location, bool bIsPowerBlow, bool performAttack )
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idAI::TestKnockoutBlow - Attempted KO of AI %s\r", name.c_str());


	if ( AI_DEAD )
	{
		return false; // already dead
	}

	if ( AI_KNOCKEDOUT )
	{
		AI_PAIN = true;
		AI_DAMAGE = true;

		return false; // already knocked out
	}

	const char* locationName = GetDamageGroup( location );

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idAI::TestKnockoutBlow - %s hit with KO object in joint %d corresponding to damage group %s\r", name.c_str(), location, locationName);
	if (cv_melee_debug.GetBool()) {
		char buff[256];
		idStr::snPrintf(buff, sizeof(buff), "AIHit:%s", locationName);
		gameRenderWorld->DebugText(buff, tr->c.point, 0.1f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 1000);
	}

	// check if we're hitting the right zone (usually the head)
	if ( idStr::Cmp(locationName, m_KoZone) != 0 )
	{
		// Signal the failed KO to the current state
		if (performAttack)
		{
			GetMind()->GetState()->OnFailedKnockoutBlow(attacker, dir, false);
		}
		
		return false; // damage zone not matching
	}

	// check if we hit within the angles
	if ( m_HeadJointID == INVALID_JOINT )
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Invalid head joint for joint found on AI %s when KO attempted \r", name.c_str());
		return false;
	}

	// Get the KO angles
	float minDotVert = m_KoDotVert;
	float minDotHoriz = m_KoDotHoriz;

	// Check if the AI is above the alert threshold for Immunity
	// Defined the name of the alert threshold in the AI def for generality

/*	grayman #2930 - follow these rules to determine immunity:

	1. Civilians can be knocked out at any time, from any angle, at any alert level.
	2. If the AI's alert level is at least m_KoAlertImmuneState, and
	   m_bKoAlertImmune is TRUE, the AI is immune.
	3. If the AI has a sheathed weapon or no weapon, and their alert level
	   is below m_KoAlertImmuneState, a KO can occur from any direction.
	4. If the AI is ready to do combat (usually a drawn weapon), and their
	   m_KoAlertImmuneState == 4 (helmet), they're immune, even if they're at a lower alert level.
	5. If the AI is ready to do combat (usually a drawn weapon), and their
	   m_KoAlertImmuneState == 5 (no helmet), and their current alert level is
	   below m_KoAlertImmuneState, a KO can occur only from behind.
    6. Finally, check the KO angles and determine if the blow has landed in the right place.
	7. A sleeping AI can be KOed from the front, unless he's wearing a helmet with a facemask.
 */

	bool immune2KO = false;
	if ((GetMoveType() == MOVETYPE_SLEEP) && // grayman #3951
		((minDotVert != 1.0f) && (minDotHoriz != 1.0f))) // cos(DEG2RAD(0.0f)) indicates elite faceguard helmet
	{
		// Rule #7 - no immunity
		minDotVert = minDotHoriz = -1.0f; // cos(DEG2RAD(180.0f)) everyone gets KO'ed
	}
	else if (spawnArgs.GetBool("is_civilian", "0"))
	{
		// Rule #1
	}
	// everyone else is a combatant
	else if ( AI_AlertIndex >= m_KoAlertImmuneState )
	{
		// is the AI immune at high alert levels?
		if ( m_bKoAlertImmune )
		{
			immune2KO = true; // Rule #2
		}
		else
		{
			// At high alert levels, use different values
			minDotVert = m_KoAlertDotVert;
			minDotHoriz = m_KoAlertDotHoriz;
		}
	}
	// alert level isn't high enough for carte-blanche immunity, so check other factors
	else if ( GetAttackFlag(COMBAT_MELEE) || GetAttackFlag(COMBAT_RANGED) )
	{
		if ( m_KoAlertImmuneState == 4 ) // wearing helmet?
		{
			immune2KO = true; // Rule #4
		}
		else // Rule #5
		{
			// A KO can occur only from behind, when this AI is more likely to be surprised

			idVec3 aiForward = viewAxis.ToAngles().ToForward();
			if ( dir * aiForward < 0.0f )
			{
				immune2KO = true; // attacked from the front
			}
			else
			{
				// Rule #5 -  // attacked from behind, so not immune
				
				// treat a drawn weapon or an unarmed AI that can fight as a highly alerted state for KO purposes
				minDotVert = m_KoAlertDotVert;
				minDotHoriz = m_KoAlertDotHoriz;
			}
		}
	}
	else
	{
		// Rule #3 - sheathed weapon or no weapon; not immune
	}

	if ( immune2KO )
	{
		// Signal the failed KO to the current state
		if (performAttack)
		{
			GetMind()->GetState()->OnFailedKnockoutBlow(attacker, dir, true);
		}
		return false; // AI is immune, so no KO this time
	}
	idVec3 KOSpot;
	idMat3 headAxis;
	// store head joint base position to KOSpot, axis to HeadAxis
	GetJointWorldTransform( m_HeadJointID, gameLocal.time, KOSpot, headAxis );

	KOSpot += headAxis * m_HeadCenterOffset;
	//idMat3 headAxisR = headAxis * m_KoRot;
	idMat3 headAxisR = viewAxis * m_KoRot; //Obsttorte: Use body orientation instead of head to avoid random head turning preventing knockouts

	idVec3 delta = KOSpot - tr->c.point;
	float lenDelta = delta.Length();

	// First, project delta into the horizontal plane of the head
	// Assume HeadAxis[1] is the up direction in head coordinates
	idVec3 deltaH = delta - headAxisR[1] * (headAxisR[1] * delta);
	float lenDeltaH = deltaH.Normalize();

	float dotHoriz = headAxisR[ 0 ] * deltaH;
	// cos (90-zenith) = adjacent / hypotenuse, applied to these vectors
	float dotVert = idMath::Fabs( lenDeltaH / lenDelta );

	// if hit was within the cone
	if ( ( dotHoriz >= minDotHoriz ) && ( dotVert >= minDotVert) )
	{
		// We just got knocked the taff out!
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idAI::TestKnockoutBlow - %s was knocked out by a blow to the head\r", name.c_str());
		if (performAttack)
		{
			Event_KO_Knockout(attacker); // grayman #2468
		}

		return true;
	}

	// Rule #6 - blow wasn't in the right place

	if (cv_melee_debug.GetBool()) {
		gameRenderWorld->DebugSphere( colorRed, idSphere(tr->c.point, 7.3f), 1000, false);
	}

	// Signal the failed KO to the current state
	if (performAttack)
	{
		GetMind()->GetState()->OnFailedKnockoutBlow(attacker, dir, true);
	}
	
	return false; // knockout angles missed
}

void idAI::KnockoutDebugDraw( void )
{
	float AngVert(0), AngHoriz(0), radius(0);
	idVec3 KOSpot(vec3_zero), ConeDir(vec3_zero);
	idMat3 HeadAxis(mat3_zero);

	const char * testZone = m_KoZone.c_str();
	if ( AI_KNOCKEDOUT || AI_DEAD || testZone[0] == '\0' )
	{
		return;
	}

	// Check if the AI is above the alert threshold for KOing
	// Defined the name of the alert threshold in the AI def for generality
	if ( AI_AlertIndex >= m_KoAlertState)
	{
		// Do not display if immune
		if ( m_bKoAlertImmune && ( AI_AlertIndex >= m_KoAlertImmuneState ) )
		{
			return;
		}

		// reduce the angle on alert, if needed
		AngVert = idMath::ACos( m_KoAlertDotVert );
		AngHoriz = idMath::ACos( m_KoAlertDotHoriz );
	}
	else
	{
		AngVert = idMath::ACos( m_KoDotVert );
		AngHoriz = idMath::ACos( m_KoDotHoriz );
	}

	if( AngVert == 0.0f )
		AngVert = 360.0f;
	if( AngHoriz == 0.0f )
		AngHoriz = 360.0f;

	if( m_HeadJointID == INVALID_JOINT )
	{
		return;
	}

	// store head joint base position to KOSpot, axis to HeadAxis
	GetJointWorldTransform( m_HeadJointID, gameLocal.time, KOSpot, HeadAxis );

	KOSpot += HeadAxis * m_HeadCenterOffset;
	idMat3 HeadAxisR = HeadAxis * m_KoRot;

	// Assumes the head joint is facing the same way as the look joint
	ConeDir = -HeadAxisR[0];

	// vertical angle in green
	radius = AngVert * 0.5f  * 30.0f;
	gameRenderWorld->DebugCone( colorGreen, KOSpot, 30.0f * ConeDir, 0, radius, USERCMD_MSEC );
	// horizontal angle in red
	radius = AngHoriz * 0.5f * 30.0f;
	gameRenderWorld->DebugCone( colorRed, KOSpot, 30.0f * ConeDir, 0, radius, USERCMD_MSEC );
}

/*
=====================
idAI::Event_KO_Knockout
=====================
*/

void idAI::Event_KO_Knockout( idEntity* inflictor )
{
	if (m_bCanBeKnockedOut) // grayman #2468 - new place to test ko immunity
	{
		m_koState = KO_BLACKJACK; // grayman #2604
		Knockout(inflictor);
	}
}

/*
=====================
idAI::Event_Gas_Knockout
=====================
*/

void idAI::Event_Gas_Knockout( idEntity* inflictor )
{
	if (m_bCanBeGassed) // grayman #2468 - new place to test gas immunity
	{
		m_koState = KO_GAS; // grayman #2604
		Knockout(inflictor);
	}
}

void idAI::Fall_Knockout( idEntity* inflictor ) // grayman #3699
{
	m_koState = KO_FALL; // grayman #2604
	Knockout( inflictor );
}

/*
=====================
idAI::Knockout

Based on idAI::Killed
=====================
*/

void idAI::Knockout( idEntity* inflictor )
{
	idAngles ang;

//	if( !m_bCanBeKnockedOut ) // grayman #2468 - this test has been moved up a level
//		return;

	if ( AI_KNOCKEDOUT || AI_DEAD )
	{
		AI_PAIN = true;
		AI_DAMAGE = true;

		return;
	}

	EndAttack();

	// stop all voice sounds
	StopSound( SND_CHANNEL_VOICE, false );
	if ( head.GetEntity() )
	{
		head.GetEntity()->StopSound( SND_CHANNEL_VOICE, false );
		head.GetEntity()->GetAnimator()->ClearAllAnims( gameLocal.time, 100 );
	}

	disableGravity = false;
	move.moveType = MOVETYPE_DEAD;
	m_bAFPushMoveables = false;

	physicsObj.UseFlyMove( false );
	physicsObj.ForceDeltaMove( false );

	// end our looping ambient sound
	StopSound( SND_CHANNEL_AMBIENT, false );

	RemoveAttachments();
	RemoveProjectile();
	StopMove( MOVE_STATUS_DONE );
	GetMemory().StopReacting(); // grayman #3559

	// grayman #3857 - leave an ongoing search
	if (m_searchID > 0)
	{
		gameLocal.m_searchManager->LeaveSearch(m_searchID,this);
	}

	ClearEnemy();

	AI_KNOCKEDOUT = true;

	// grayman #3317 - mark time of KO and allow immediate visual stimming from this KO'ed AI so
	// that nearby AI can react quickly, instead of waiting for the next-scheduled
	// visual stim from this AI
	m_timeFellDown = gameLocal.time;
	gameLocal.AllowImmediateStim( this, ST_VISUAL );

	// greebo: Switch the mind to KO state, this will trigger PostKnockout() when
	// the animation is done
	mind->ClearStates();
	mind->PushState(STATE_KNOCKED_OUT);

	// Update TDM objective system
	bool bPlayerResponsible = ( ( inflictor != NULL ) && ( inflictor == gameLocal.GetLocalPlayer() ) );
	gameLocal.m_MissionData->MissionEvent(COMP_KO, this, inflictor, bPlayerResponsible);

	// Mark the body as last moved by the player
	if ( bPlayerResponsible )
	{
		m_SetInMotionByActor = gameLocal.GetLocalPlayer(); // grayman #3394
		m_MovedByActor = gameLocal.GetLocalPlayer();
		m_deckedByPlayer = true; // grayman #3314
	}

	// greebo: Stop lipsyncing now
	m_lipSyncActive = false;

	// grayman #3559 - clear list of doused lights this AI has seen; no longer needed
	m_dousedLightsSeen.Clear();

	alertQueue.Clear(); // grayman #4002
}

void idAI::PostKnockOut()
{
	// greebo: Removed StopAnim() for head channel, this caused the AI to open its eyes again
	//headAnim.StopAnim(1);

	legsAnim.StopAnim(1);
	torsoAnim.StopAnim(1);

	headAnim.Disable();
	legsAnim.Disable();
	torsoAnim.Disable();
	// make original self nonsolid
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();

	Unbind();

	// swaps the head CM back if a different one was swapped in while conscious
	SwapHeadAFCM( false );

	if ( StartRagdoll() )
	{
		// grayman #2604 - play "gassed" sound if knocked out by gas
		switch (m_koState)
		{
		case KO_BLACKJACK:
			StartSound( "snd_knockout", SND_CHANNEL_VOICE, 0, false, NULL );
			break;
		case KO_GAS:
			StartSound( "snd_airGasp", SND_CHANNEL_VOICE, 0, false, NULL );
			break;
		case KO_NOT:
		case KO_FALL: // grayman #3699
		default:
			break;
		}
	}

	const char *modelKOd;

	if ( spawnArgs.GetString( "model_knockedout", "", &modelKOd ) )
	{
		// fire elemental is only case that does not use a ragdoll and has a model_death so get the death sound in here
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		SetModel( modelKOd );
		physicsObj.SetLinearVelocity( vec3_zero );
		physicsObj.PutToRest();
		physicsObj.DisableImpact();
	}

	if (spawnArgs.GetBool("set_frobable_on_knockout", "1"))
	{
		// AI becomes frobable on KO
		// greebo: Add a delay before the AI becomes actually frobable
		PostEventMS(&EV_SetFrobable, 750, 1);
	}

	restartParticles = false;

	// drop items
	DropOnRagdoll();
}

/**
* ========================== END TDM KNOCKOUT CODE =============================
**/

/*
=====================
idAI::FoundBody
=====================
*/
void idAI::FoundBody( idEntity *body )
{
	// grayman #3314 - if m_deckedByPlayer is true, the player killed or KO'ed this body
	if ( body->IsType(idAI::Type) )
	{
		gameLocal.m_MissionData->MissionEvent( COMP_AI_FIND_BODY, body, static_cast<idAI*>(body)->m_deckedByPlayer );
	}
}

void idAI::AddMessage(const ai::CommMessagePtr& message, int msgTag) // grayman #3355
{
	assert(message != NULL);
	message->m_msgTag = msgTag; // grayman #3355
	m_Messages.Append(message);
}

void idAI::ClearMessages(int msgTag)
{
	if ( msgTag == 0)
	{
		m_Messages.ClearFree();
		return;
	}

	// grayman #3355 - only clear messages that have a matching message tag
	for ( int i = 0 ; i < m_Messages.Num() ; i++ )
	{
		ai::CommMessagePtr message = m_Messages[i];
		if ( message->m_msgTag == msgTag )
		{
			m_Messages.RemoveIndex(i--);
		}
	}
	m_Messages.Condense();
}

// grayman #3424 - Do I have an outgoing message of a certain type
// in the queue for a certain receiver? If so, return TRUE, else FALSE. This
// is useful in preventing the sending of the same message twice.

bool idAI::CheckOutgoingMessages( ai::CommMessage::TCommType type, idActor* receiver)
{
	for ( int i = 0 ; i < m_Messages.Num() ; i++ )
	{
		ai::CommMessagePtr message = m_Messages[i];
		if ( ( message->m_commType == type ) &&
			    ( message->m_p_recipientEntity.GetEntity() == receiver ) )
		{
			return true;
		}
	}

	return false;
}

/*
=====================
idAI::CheckFOV
=====================
*/
bool idAI::CheckFOV( const idVec3 &pos ) const
{
	//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("idAI::CheckFOV called \r");

	float	dotHoriz, dotVert, lenDelta, lenDeltaH;
	idVec3	delta, deltaH, HeadCenter;
	idMat3	HeadAxis;

	// ugliness
	if ( !const_cast<idAI *>(this)->GetJointWorldTransform( m_HeadJointID, gameLocal.time, HeadCenter, HeadAxis ) )	{
		// stgatilov: ai_arx_fish from Seeking Lady Leicester has no head joint...
		return false;
	}
	idMat3 HeadAxisR = m_FOVRot * HeadAxis;

	// Offset to get the center of the head
	HeadCenter += HeadAxis * m_HeadCenterOffset;
	delta = pos - HeadCenter;
	lenDelta = delta.Length(); // Consider LengthFast if error is not too bad

	// First, project delta into the horizontal plane of the head
	// Assume HeadAxis[1] is the up direction in head coordinates
	deltaH = delta - HeadAxisR[1] * (HeadAxisR[1] * delta);
	lenDeltaH = deltaH.Normalize(); // Consider NormalizeFast

	dotHoriz = HeadAxisR[ 0 ] * deltaH;
	// cos (90-zenith) = adjacent / hypotenuse, applied to these vectors
	// Technically this lets them see things behind them, but they won't because 
	// of the horizontal check.
	dotVert = idMath::Fabs( lenDeltaH / lenDelta );

	return ( dotHoriz >= m_fovDotHoriz && dotVert >= m_fovDotVert );
}

void idAI::FOVDebugDraw( void )
{
	float AngVert(0), AngHoriz(0), radius(0);
	idVec3 HeadCenter(vec3_zero), ConeDir(vec3_zero);
	idMat3 HeadAxis(mat3_zero);

	if( AI_KNOCKEDOUT || AI_DEAD || m_HeadJointID == INVALID_JOINT )
	{
		return;
	}

	// probably expensive, but that's okay since this is just for debug mode

	AngVert = idMath::ACos( m_fovDotVert );
	AngHoriz = idMath::ACos( m_fovDotHoriz );

	// store head joint base position to HeadCenter, axis to HeadAxis
	GetJointWorldTransform( m_HeadJointID, gameLocal.time, HeadCenter, HeadAxis );
	idMat3 HeadAxisR =  m_FOVRot * HeadAxis;
	// offset from head joint position to get the true head center
	HeadCenter += HeadAxis * m_HeadCenterOffset;

	ConeDir = HeadAxisR[0];

	// Diverge to keep reasonable cone size
	float coneLength;

	if (AngVert >= (idMath::PI / 4.0f))
	{
		// Fix radius and calculate length
		radius = 60.0f;
		coneLength = radius / idMath::Tan(AngVert);
	}
	else
	{
		// Fix length and calculate radius
		coneLength = 60.0f;
		// SZ: FOVAng is divergence off to one side (idActor::setFOV uses COS(fov/2.0) to calculate m_fovDotHoriz)
		radius = idMath::Tan(AngVert) * coneLength;
	}

	gameRenderWorld->DebugCone( colorBlue, HeadCenter, coneLength * ConeDir, 0, radius, USERCMD_MSEC );
	
	// now do the same for horizontal FOV angle, orange cone
	if (AngHoriz >= (idMath::PI / 4.0f))
	{
		// Fix radius and calculate length
		radius = 60.0f;
		coneLength = radius / idMath::Tan(AngHoriz);
	}
	else
	{
		// Fix length and calculate radius
		coneLength = 60.0f;
		// SZ: FOVAng is divergence off to one side (idActor::setFOV uses COS(fov/2.0) to calculate m_fovDotHoriz)
		radius = idMath::Tan(AngHoriz) * coneLength;
	}

	gameRenderWorld->DebugCone( colorOrange, HeadCenter, coneLength * ConeDir, 0, radius, USERCMD_MSEC );
}

moveStatus_t idAI::GetMoveStatus() const
{
	return move.moveStatus;
}

bool idAI::CanReachEnemy()
{
	aasPath_t	path;
	int			toAreaNum;
	int			areaNum;
	idVec3		pos;
	idActor		*enemyEnt;

	enemyEnt = enemy.GetEntity();
	if ( !enemyEnt ) {
		return false;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( enemyEnt->OnLadder() ) {
			return false;
		}
		enemyEnt->GetAASLocation( aas, pos, toAreaNum );
	}  else {
		pos = enemyEnt->GetPhysics()->GetOrigin();
		toAreaNum = PointReachableAreaNum( pos );
	}

	if ( !toAreaNum ) {
		return false;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );
	return PathToGoal(path, areaNum, org, toAreaNum, pos, this);
}

/*
=====================
idAI::GetEyePosition
=====================
*/
idVec3 idAI::GetEyePosition( void ) const
{
	// grayman #3525 - a more accurate eye position, accounting for standing,
	// sitting, and lying down (KO'ed or killed)

	idVec3 eyePosition;

	idEntity *headEnt = head.GetEntity();

	// *.def file will store the coordinates of the mouth relative ("mouth_offset") to the head origin
	// or relative to the eyes ("eye_height"). This should be defined by the modeler.

	// check for attached head
	if ( headEnt )
	{
		eyePosition = headEnt->GetPhysics()->GetOrigin();

		// add the eye offset oriented by head axis
		eyePosition += headEnt->GetPhysics()->GetAxis() * m_EyeOffset;
	}
	else if ( AI_KNOCKEDOUT || AI_DEAD )
	{
		// grayman #3525 - total kludge, since I can't tell where the head is
		idVec3 center = GetPhysics()->GetAbsBounds().GetCenter();
		eyePosition = GetPhysics()->GetOrigin();
		eyePosition.z = center.z;
		eyePosition += viewAxis.ToAngles().ToForward() * m_EyeOffset.x;
	}
	else // standing
	{
		eyePosition = GetPhysics()->GetOrigin() +
					( GetPhysics()->GetGravityNormal() * -m_EyeOffset.z ) +
					( viewAxis.ToAngles().ToForward() * m_EyeOffset.x);
	}

	return eyePosition;
}

bool idAI::MouthIsUnderwater( void )
{
	bool bReturnVal( false );
	idVec3 MouthPosition;

	idEntity *headEnt = head.GetEntity();

	// *.def file will store the coordinates of the mouth relative ("mouth_offset") to the head origin
	// or relative to the eyes. This should be defined by the modeler.

	// check for attached head
	if ( headEnt )
	{
		MouthPosition = headEnt->GetPhysics()->GetOrigin();

		// add the mouth offset oriented by head axis
		MouthPosition += headEnt->GetPhysics()->GetAxis() * m_MouthOffset;
	}
	else if ( AI_KNOCKEDOUT || AI_DEAD )
	{
		MouthPosition = GetEyePosition();
	}
	else // standing
	{
		MouthPosition = GetEyePosition() +
						( GetPhysics()->GetGravityNormal() * -m_MouthOffset.z ) +
						( viewAxis.ToAngles().ToForward() * m_MouthOffset.x);
	}

	// check if the mouth position is underwater

	int contents = gameLocal.clip.Contents( MouthPosition, NULL, mat3_identity, -1, this );
	bReturnVal = (contents & MASK_WATER) > 0;
	
#if 0
	// grayman #1488
	//
	// For debugging drowning deaths - indicate onscreen whether underwater or breathing, plus health level
	
	idStr str;
	idVec4 color;
	idStr healthLevel;

	if (bReturnVal)
	{
		str = "Drowning";
		color = colorRed;
	}
	else
	{
		str = "Breathing";
		color = colorGreen;
	}
	sprintf( healthLevel, " (%d)", health );
	gameRenderWorld->DebugText((str + healthLevel).c_str(),(GetEyePosition() - GetPhysics()->GetGravityNormal()*60.0f), 
		0.25f, color, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 150 * USERCMD_MSEC);
#endif

	return bReturnVal;
}

void idAI::UpdateAir()
{
	// Are we able to drown at all? Has the time for next air check already come?
	if (!m_bCanDrown || gameLocal.time < m_AirCheckTimer)
	{
		return;
	}

	if (MouthIsUnderwater())
	{
		// grayman #3774 - keep track of responsibility for being underwater
		// and potentially running out of air and dying
		if (m_DroppedInLiquidByActor.GetEntity() == NULL)
		{
			m_DroppedInLiquidByActor = m_SetInMotionByActor.GetEntity();
		}

		// don't let KO'd AI hold their breath
		if (AI_KNOCKEDOUT)
		{
			m_AirTics = 0;
		}

		m_AirTics--;
	}
	else
	{
		// regain breath twice as fast as losing
		if ( m_AirTics < m_AirTicksMax ) { // avoid unnecessary memory writes
			m_AirTics += 2;
			if ( m_AirTics > m_AirTicksMax )
				m_AirTics = m_AirTicksMax;
		}
	}

	if (m_AirTics < 0)
	{
		m_AirTics = 0;

		// do the damage, damage_noair is already defined for the player
		// grayman #3774 - note if anyone was responsible for putting this AI in the water
		Damage(NULL, m_DroppedInLiquidByActor.GetEntity(), vec3_origin, "damage_noair", 1.0f, 0);
	}

	// set the timer for next air check
	m_AirCheckTimer += m_AirCheckInterval;
}

int	idAI::getAirTicks() const {
	return m_AirTics;
}

void idAI::setAirTicks(int airTicks) {
	m_AirTics = airTicks;
	// Clamp to maximum value
	if( m_AirTics > m_AirTicksMax ) {
		m_AirTics = m_AirTicksMax;
	}
}

/*
===================== Lipsync =====================
*/
int idAI::PlayAndLipSync(const char *soundName, const char *animName, int msgTag) // grayman #3355
{
	// Play sound
	int duration;
	StartSound( soundName, SND_CHANNEL_VOICE, 0, false, &duration, 0, msgTag ); // grayman #3355

	// grayman #3857
	if (cv_ai_bark_show.GetBool())
	{
		m_barkName = idStr(soundName);
		m_barkEndTime = gameLocal.time + duration;
	}

	// grayman #3857 - moved to idAI::ShowDebugInfo()
	/*
	if (cv_ai_bark_show.GetBool())
	{
		gameRenderWorld->DebugText(va("%s", soundName), GetPhysics()->GetOrigin() + idVec3(0, 0, 90), 0.25f, colorWhite,
			gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, duration );
	}
	*/

	// Do we want to lipsync this sound?
	StopLipSync(); // Assume not

	const char *sound;
	if (spawnArgs.GetString( soundName, "", &sound ))
	{
		const idSoundShader* shader = declManager->FindSound( sound );

		if (shader != NULL && idStr::Cmp(shader->GetDescription(), "nolipsync") != 0)
		{
			// The sound exists and isn't marked "nolipsync", so start the lipsync
			
			// Get the default animation name if necessary
			if (animName == NULL || animName[0]=='\0')
			{
				// Not specified; get the default from a spawnarg.
				// If even the spawnarg doesn't exist, revert to talk.
				animName = spawnArgs.GetString("lipsync_anim_name", "talk");
			}
			
			m_lipSyncActive = true;
			m_lipSyncAnim = GetAnim( ANIMCHANNEL_HEAD, animName );
			m_lipSyncEndTimer = gameLocal.time + duration;
			headAnim.CycleAnim( m_lipSyncAnim );
		}
	}

	return duration;
}

void idAI::Event_PlayAndLipSync( const char *soundName, const char *animName )
{
	// grayman - might have to add drowning check here if anyone
	// ever uses this function in a script.

	idThread::ReturnInt(static_cast<int>(MS2SEC(PlayAndLipSync(soundName, animName, 0)))); // grayman #3355
}

void idAI::StopLipSync()
{
	if (m_lipSyncActive)
	{
		// Make sure mouth is closed
		headAnim.SetFrame( m_lipSyncAnim, 0 );
		// Halt animation
		headAnim.SetState("Head_Idle", 4);
	}
	m_lipSyncActive = false;
}

/*
===================== Sheathing/drawing weapons =====================
*/

bool idAI::DrawWeapon(ECombatType type) // grayman #3775
{
	/*const function_t* func = scriptObject.GetFunction("DrawWeapon");
	if (func) {
		idThread* thread = new idThread(func);
		thread->CallFunction(this, func, true);
		thread->DelayedStart(0);
	}*/
	// greebo: Replaced the above thread spawn with an animstate switch

	// grayman #3317 - Can't draw a weapon if you don't have one.

	int numMeleeWeapons = GetNumMeleeWeapons();
	int numRangedWeapons = GetNumRangedWeapons();

	if ( ( numMeleeWeapons == 0 ) && ( numRangedWeapons == 0 ) )
	{
		return false; // nothing to do // grayman #3775
	}

	// grayman #3492 - don't draw a weapon if you're already in the act of drawing one

	if ( idStr(WaitState()) == "draw" )
	{
		return false; // grayman #3775
	}

	// grayman #3775 - don't draw a weapon if you're throwing something

	if ( idStr(WaitState()) == "throw" )
	{
		return false; // grayman #3775
	}

	// grayman #3331 - draw the requested weapon

	if ( ( type == COMBAT_MELEE ) && ( numMeleeWeapons > 0 ) )
	{
		SetAnimState(ANIMCHANNEL_TORSO, "Torso_DrawMeleeWeapon", 4);
	}
	else if ( ( type == COMBAT_RANGED ) && ( numRangedWeapons > 0 ) )
	{
		// grayman #3123 - if this AI is carrying something in his
		// left hand, he needs to drop it first

		idEntity* torch = GetTorch();
		if ( torch )
		{
			Event_DropTorch();
		}
		else // anything else in the left hand?
		{
			idEntity* inHand = GetAttachmentByPosition("hand_l");
			if ( inHand )
			{
				// Something in the left hand, so drop it
				Detach(inHand->name.c_str());
			}
		}
		SetAnimState(ANIMCHANNEL_TORSO, "Torso_DrawRangedWeapon", 4);
	}
	SetWaitState("draw"); // grayman #3355
	return true; // grayman #3775
}

void idAI::SheathWeapon() 
{
	// grayman #2416 - only sheathe a weapon if one is drawn
	if ( !GetAttackFlag(COMBAT_MELEE) && !GetAttackFlag(COMBAT_RANGED) )
	{
		return;
	}

	/*const function_t* func = scriptObject.GetFunction("SheathWeapon");
	if (func) {
		idThread* thread = new idThread(func);
		thread->CallFunction(this, func, true);
		thread->DelayedStart(0);
	}*/
	// greebo: Replaced the above thread spawn with an animstate switch
	SetAnimState(ANIMCHANNEL_TORSO, "Torso_SheathWeapon", 4);
	SetWaitState("sheath"); // grayman #3355
}

void idAI::DropOnRagdoll( void )
{
	idEntity *ent = NULL;
	int mask(0);
	// Id style def_drops
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_drops", NULL );
	
	// old D3 code for spawning things at AI's feet when they die
	while( kv )
	{
		idDict args;

		args.Set( "classname", kv->GetValue() );
		args.Set( "origin", physicsObj.GetOrigin().ToString() );
		gameLocal.SpawnEntityDef( args );
		kv = spawnArgs.MatchPrefix( "def_drops", kv );
	}

	// Drop TDM style attachments
	for ( int i = 0 ; i < m_Attachments.Num() ; i++ )
	{
		ent = m_Attachments[i].ent.GetEntity();
		if ( !ent || !m_Attachments[i].ent.IsValid() )
		{
			continue;
		}

		// deactivate any melee weapon actions at this point
		if ( ent->IsType(CMeleeWeapon::Type) )
		{
			CMeleeWeapon *pWeap = static_cast<CMeleeWeapon *>(ent);
			pWeap->DeactivateAttack();
			pWeap->DeactivateParry();
			pWeap->ClearOwner();
			// stgatilov #6546: track dropped melee weapon forever
			gameLocal.m_LightEstimateSystem->TrackEntity( ent, 1000000000 );
		}

		// greebo: Check if we should set some attachments to nonsolid
		// this applies for instance to the builder guard's pauldrons which
		// cause twitching and self-collisions when going down
		if ( ent->spawnArgs.GetBool( "drop_set_nonsolid" ) )
		{
			int curContents = ent->GetPhysics()->GetContents();

			// ishtvan: Also clear the CONTENTS_CORPSE flag (maybe this was a typo in original code?)
			ent->GetPhysics()->SetContents(curContents & ~(CONTENTS_SOLID|CONTENTS_CORPSE));

			// ishtvan: If this entity was added to the AF, set the AF body nonsolid as well
			idAFBody *EntBody = AFBodyForEnt( ent );
			if( EntBody != NULL )
			{
				int curBodyContents = EntBody->GetClipModel()->GetContents();
				EntBody->GetClipModel()->SetContents( curBodyContents  & ~(CONTENTS_SOLID|CONTENTS_CORPSE) );
			}

			// also have to iterate thru stuff attached to this attachment
			// ishtvan: left this commentd out because I'm not sure if GetTeamChildren is bugged or not
			// don't want to accidentally set all attachments to the AI to nonsolid
			/*
			idList<idEntity *> AttChildren;
			ent->GetTeamChildren( &AttChildren );
			gameLocal.Printf("TEST: drop_set_nonsolid, Num team children = %d", AttChildren.Num() );
			for( int i=0; i < AttChildren.Num(); i++ )
			{
				idPhysics *pChildPhys = AttChildren[i]->GetPhysics();
				if( pChildPhys == NULL )
					continue;

				int childContents = pChildPhys->GetContents();
				pChildPhys->SetContents( childContents & ~(CONTENTS_SOLID|CONTENTS_CORPSE) );
			}
			*/
		}

		bool bDrop = ent->spawnArgs.GetBool( "drop_when_ragdoll" );
		
		if ( !bDrop )
		{
			continue;
		}

		bool bDropWhenDrawn = ent->spawnArgs.GetBool( "drop_when_drawn" );
		bool bSetSolid = ent->spawnArgs.GetBool( "drop_add_contents_solid" );
		bool bSetCorpse = ent->spawnArgs.GetBool( "drop_add_contents_corpse" );

		if ( bDropWhenDrawn )
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Testing drop weapon %s\r", ent->name.c_str() );
			
			bool bIsMelee = ent->spawnArgs.GetBool( "is_weapon_melee" );

			if ( bIsMelee && !GetAttackFlag(COMBAT_MELEE) )
			{
				DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Melee weapon was not drawn\r" );
				continue;
			}

			bool bIsRanged = ent->spawnArgs.GetBool( "is_weapon_ranged" );

			if ( bIsRanged && !GetAttackFlag(COMBAT_RANGED) )
			{
				continue;
			}
		}

		// Proceed with droppage
		DetachInd( i );

		CheckAfterDetach( ent ); // grayman #2624 - check for frobability and whether to extinguish

		if ( bSetSolid )
		{
			mask = CONTENTS_SOLID;
		}

		if ( bSetCorpse )
		{
			mask = mask | CONTENTS_CORPSE;
		}

		if ( mask )
		{
			ent->GetPhysics()->SetContents( ent->GetPhysics()->GetContents() | mask );
		}

		// grayman #2624 - now handled by CheckAfterDetach() above
/*
		bool bSetFrob = ent->spawnArgs.GetBool( "drop_set_frobable" );
		bool bExtinguish = ent->spawnArgs.GetBool("extinguish_on_drop", "0");

		if ( bSetFrob )
		{
			ent->m_bFrobable = true;
		}

		// greebo: Check if we should extinguish the attachment, like torches
		if ( bExtinguish )
		{
			// Get the delay in milliseconds
			int delay = SEC2MS(ent->spawnArgs.GetInt("extinguish_on_drop_delay", "3"));
			if (delay < 0)
			{
				delay = 0;
			}

			// Schedule the extinguish event
			ent->PostEventMS(&EV_ExtinguishLights, delay);
		}
*/
		ent->GetPhysics()->Activate();
		ent->m_droppedByAI = true; // grayman #1330

		// grayman #3075 - set m_SetInMotionByActor
		// grayman #3602 - set in motion by dropper
		ent->m_SetInMotionByActor = this;
		ent->m_MovedByActor = this;
		//ent->m_SetInMotionByActor = NULL;
		//ent->m_MovedByActor = NULL;
	}

	// also perform some of these same operations on attachments to our head,
	// because they are stored differently than attachments to us
	if ( head.GetEntity() )
	{
		head.GetEntity()->DropOnRagdoll();
	}
}

bool idAI::IsSearching() // grayman #2603
{
	return (AI_AlertLevel >= thresh_3); // note that this also returns TRUE if in combat mode
}

float idAI::GetArmReachLength()
{
	if (idAAS* aas = GetAAS())
	{
		idVec3 size = aas->GetSettings()->boundingBoxes[0][1];
		return size.z * 0.5;
	}
	else
	{
		return 41;
	}
}

void idAI::NeedToUseElevator(const eas::RouteInfoPtr& routeInfo)
{
	mind->GetState()->NeedToUseElevator(routeInfo);
}


bool idAI::CanUnlock(CBinaryFrobMover *frobMover)
{
	// Look through list of unlockable doors set by spawnarg
	FrobMoverList::iterator i = unlockableDoors.find(frobMover);
	if (i != unlockableDoors.end())
	{
		return true;
	}
	
	// Look through attachments
	int n = frobMover->m_UsedByName.Num();
	for (int i = 0; i < m_Attachments.Num(); i++)
	{
		idEntity* ent = m_Attachments[i].ent.GetEntity();

		if (ent == NULL || !m_Attachments[i].ent.IsValid())
		{
			continue;
		}
		
		for (int j = 0; j < n; j++)
		{
			if (ent->name == frobMover->m_UsedByName[j])
			{
				return true;
			}
		}
	}
	return false;
}

bool idAI::ShouldCloseDoor(CFrobDoor *door, bool lockDoor) // grayman #3523
{
	if (door->spawnArgs.GetBool("ai_should_not_close", "0"))
	{
		return false; // this door should not be closed
	}

	if (AI_AlertLevel >= thresh_5)
	{
		return false; // don't close doors during combat
	}

	if (door->spawnArgs.GetBool("shouldBeClosed", "0"))
	{
		return true; // this door should be closed
	}

	if (IsSearching()) // grayman #2603
	{
		return false; // don't close doors while searching
	}

	if (lockDoor)
	{
		return true; // this door should be closed so it can be relocked
	}

	if (door->spawnArgs.GetBool("canRemainOpen", "0"))
	{
		return false; // prefer that it stay open
	}
	
	return true; // in all other cases, close the door
}

void idAI::PushMove()
{
	// Copy the current movestate structure to the stack
	moveStack.push_back(move);

	if (moveStack.size() > 100)
	{
		gameLocal.Warning("AI MoveStack contains more than 100 moves! (%s)", name.c_str());
	}
}

void idAI::PopMove()
{
	if (moveStack.empty())
	{
		return; // nothing to pop from
	}
	
	const idMoveState& saved = moveStack.back(); // Get a reference of the last element
	RestoreMove(saved);
	moveStack.pop_back(); // Remove the last element
}

// grayman #3647 - same as PopMove() but w/o the RestoreMove()

void idAI::PopMoveNoRestoreMove()
{
	if (moveStack.empty())
	{
		return; // nothing to pop from
	}
	
	moveStack.pop_back(); // Remove the last element
}

void idAI::RestoreMove(const idMoveState& saved)
{
	idVec3 goalPos;

	switch( saved.moveCommand ) {
	case MOVE_NONE :
		StopMove( saved.moveStatus );
		break;

	case MOVE_FACE_ENEMY :
		FaceEnemy();
		break;

	case MOVE_FACE_ENTITY :
		FaceEntity( saved.goalEntity.GetEntity() );
		break;

	case MOVE_TO_ENEMY :
		MoveToEnemy();
		break;

	case MOVE_TO_ENEMYHEIGHT :
		MoveToEnemyHeight();
		break;

	case MOVE_TO_ENTITY :
		MoveToEntity( saved.goalEntity.GetEntity() );
		break;

	case MOVE_OUT_OF_RANGE :
		MoveOutOfRange( saved.goalEntity.GetEntity(), saved.range );
		break;

	case MOVE_TO_ATTACK_POSITION :
		MoveToAttackPosition( saved.goalEntity.GetEntity(), saved.anim );
		break;

	case MOVE_TO_COVER :
		{
		// grayman #3280 - enemies look with their eyes, not their feet
		idEntity* ent = saved.goalEntity.GetEntity();
		if ( ent->IsType(idActor::Type))
		{
			MoveToCover( ent, static_cast<idActor*>(ent)->GetEyePosition() );
		}
		else
		{
			MoveToCover( ent, lastVisibleEnemyPos );
		}
		break;
		}

	case MOVE_TO_POSITION :
		MoveToPosition( saved.moveDest );
		break;

	case MOVE_TO_POSITION_DIRECT :
		DirectMoveToPosition( saved.moveDest );
		break;

	case MOVE_SLIDE_TO_POSITION :
		SlideToPosition( saved.moveDest, saved.duration );
		break;

	case MOVE_WANDER :
		WanderAround();
		break;
		
	default: break;
	}

	if ( GetMovePos( goalPos ) ) {
		//CheckObstacleAvoidance( goalPos, dest );
	}
}

void idAI::ShowDebugInfo()
{
	// DarkMod: Show debug info
	if( cv_ai_ko_show.GetBool() )
	{
		KnockoutDebugDraw();
	}

	if( cv_ai_fov_show.GetBool() )
	{
		FOVDebugDraw();
	}
	
	if ( cv_ai_dest_show.GetBool() )
	{
		gameRenderWorld->DebugArrow(colorYellow, physicsObj.GetOrigin(), move.moveDest, 5, USERCMD_MSEC);
	}

	if (cv_ai_task_show.GetBool())
	{
		idStr str("State: ");
		str += mind->GetState()->GetName() + "\n";

		if (GetSubsystem(ai::SubsysSenses)->IsEnabled()) str += "Senses: " + GetSubsystem(ai::SubsysSenses)->GetDebugInfo() + "\n";
		if (GetSubsystem(ai::SubsysMovement)->IsEnabled()) str += "Movement: " + GetSubsystem(ai::SubsysMovement)->GetDebugInfo() + "\n";
		if (GetSubsystem(ai::SubsysCommunication)->IsEnabled()) str += "Comm: " + GetSubsystem(ai::SubsysCommunication)->GetDebugInfo() + "\n";
		if (GetSubsystem(ai::SubsysAction)->IsEnabled()) str += "Action: " + GetSubsystem(ai::SubsysAction)->GetDebugInfo() + "\n";
		if (GetSubsystem(ai::SubsysSearch)->IsEnabled()) str += "Search: " + GetSubsystem(ai::SubsysSearch)->GetDebugInfo() + "\n"; // grayman #3857

		gameRenderWorld->DebugText(str, (GetEyePosition() - physicsObj.GetGravityNormal()*-25.0f), 0.25f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
	}

	if (cv_ai_alertlevel_show.GetBool() && (health > 0) && !IsKnockedOut())
	{
		gameRenderWorld->DebugText(va("Alert: %f; Index: %d", (float)AI_AlertLevel, (int)AI_AlertIndex), (GetEyePosition() - physicsObj.GetGravityNormal()*45.0f), 0.25f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		// grayman #3857 - add debugging info for coordinated searches
		if ((m_searchID > 0) && ((AI_AlertIndex == ai::ESearching) || (AI_AlertIndex == ai::EAgitatedSearching)))
		{
			Search* search = gameLocal.m_searchManager->GetSearch(m_searchID);
			Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search, this);
			smRole_t r = E_ROLE_NONE;
			idStr role = "none";
			if (assignment)
			{
				r = assignment->_searcherRole;
				if (r == E_ROLE_SEARCHER)
				{
					role = "searcher";
				}
				else if (r == E_ROLE_GUARD)
				{
					role = "guard";
				}
				else if (r == E_ROLE_OBSERVER)
				{
					role = "observer";
				}
			}
			gameRenderWorld->DebugText(va("Event: %d; Search: %d; Role: %s", search->_eventID, search->_searchID, role.c_str()), (GetEyePosition() - physicsObj.GetGravityNormal()*20.0f), 0.25f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		}

		if (m_AlertGraceStart + m_AlertGraceTime - gameLocal.time > 0)
		{
			gameRenderWorld->DebugText(va("Grace time: %d; Alert count: %d / %d",
				m_AlertGraceStart + m_AlertGraceTime - gameLocal.time,
				m_AlertGraceCount, m_AlertGraceCountLimit),
				GetEyePosition(), 0.25f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
		}
	}

	if (cv_ai_animstate_show.GetBool())
	{
		idStr debugText("Torso: ");
		debugText += GetAnimState(ANIMCHANNEL_TORSO);
		debugText += " Waitstate: ";
		debugText += WaitState(ANIMCHANNEL_TORSO);
		debugText += "\nLegs: ";
		debugText += GetAnimState(ANIMCHANNEL_LEGS);
		debugText += " Waitstate: ";
		debugText += WaitState(ANIMCHANNEL_LEGS);
		debugText += "\nHead: ";
		debugText += GetAnimState(ANIMCHANNEL_HEAD);
		debugText += " Waitstate: ";
		debugText += WaitState(ANIMCHANNEL_LEGS);
		debugText += "\n";

		if (WaitState() != NULL)
		{
			debugText += idStr("Waitstate: ") + WaitState();
		}
		gameRenderWorld->DebugText(debugText, (GetEyePosition() - physicsObj.GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC);
	}

	if (cv_ai_aasarea_show.GetBool() && aas != NULL)
	{
		idVec3 org = GetPhysics()->GetOrigin();
		int areaNum = PointReachableAreaNum(org);

		idBounds areaBounds = aas->GetAreaBounds(areaNum);
		idVec3 areaCenter = aas->AreaCenter(areaNum);

		idMat3 playerViewMatrix(gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

		gameRenderWorld->DebugText(va("%d", areaNum), areaCenter, 0.2f, colorGreen, playerViewMatrix, 1, USERCMD_MSEC);
		gameRenderWorld->DebugBox(colorGreen, idBox(areaBounds), USERCMD_MSEC);
	}

	if (cv_ai_elevator_show.GetBool())
	{
		idMat3 playerViewMatrix(gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

		gameRenderWorld->DebugText(m_HandlingElevator ? "Elevator" : "---", physicsObj.GetOrigin(), 0.2f, m_HandlingElevator ? colorRed : colorGreen, playerViewMatrix, 1, USERCMD_MSEC);
	}

	// grayman #3857 - show barking info
	if (cv_ai_bark_show.GetBool() && (gameLocal.time <= m_barkEndTime) && ((physicsObj.GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast() < 1000))
	{
		gameRenderWorld->DebugText(va("%s", m_barkName.c_str()), physicsObj.GetOrigin() + idVec3(0, 0, 90), 0.25f, colorWhite,
			gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 16);
	}

	// grayman #3857 - show AI name
	if (cv_ai_name_show.GetBool() && ((physicsObj.GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthSqr() < Square(1000)))
	{
		idVec4 colour = colorWhite;
		gameRenderWorld->DebugText(va("%s", GetName()), physicsObj.GetOrigin() + idVec3(0, 0, 50), 0.25f, colour,
			gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 16);
	}
}

const idStr& idAI::GetNextIdleAnim() const
{
	return m_NextIdleAnim;
}

void idAI::SetNextIdleAnim(const idStr& nextIdleAnim)
{
	m_NextIdleAnim = nextIdleAnim;
}

// grayman #2603 - delayed stim management

void idAI::SetDelayedStimExpiration(idEntityPtr<idEntity> stimPtr)
{
	for (int i = 0 ; i < delayedStims.Num() ; i++)
	{
		if (delayedStims[i].stim == stimPtr)
		{
			delayedStims[i].nextTimeToConsider = gameLocal.time + 15000;
			return;
		}
	}

	// stim not in list, so add it

	DelayedStim ds;
	ds.nextTimeToConsider = gameLocal.time + 15000;
	ds.stim = stimPtr;
	delayedStims.Append(ds);
}

int idAI::GetDelayedStimExpiration(idEntityPtr<idEntity> stimPtr)
{
	for (int i = 0 ; i < delayedStims.Num() ; i++)
	{
		if (delayedStims[i].stim == stimPtr)
		{
			return (delayedStims[i].nextTimeToConsider);
		}
	}
	return -1;
}

// grayman #3075 - set and get AI's blood marker, if any

void idAI::SetBlood(idEntity *marker)
{
	m_bloodMarker = marker;
}

idEntity* idAI::GetBlood(void) const
{
	return m_bloodMarker.GetEntity();
}

// grayman #2816 - remember who you killed, for barking and tactile alert events

void idAI::SetLastKilled(idActor *killed)
{
	m_lastKilled = killed;
	m_justKilledSomeone = true; // used to skip regular rampdown bark in favor of a "killed" bark
}


bool idAI::SwitchToConversationState(const idStr& conversationName)
{
	ai::ConversationStatePtr state = 
		std::static_pointer_cast<ai::ConversationState>(ai::ConversationState::CreateInstance());

	// Convert the name to an index
	int convIndex = gameLocal.m_ConversationSystem->GetConversationIndex(conversationName);

	if (convIndex == -1) 
	{
		DM_LOG(LC_CONVERSATION, LT_ERROR)LOGSTRING("Could not find conversation '%s'.\r", conversationName.c_str());
		return false;
	}

	// Let the state know which conversation is about to be played
	state->SetConversation(convIndex);

	// Switch to this state
	GetMind()->PushState(state);

	// grayman #3559 - stop dealing with a picked pocket

	GetMemory().stopReactingToPickedPocket = true;

	return true;
}

void idAI::RemoveTarget(idEntity* target)
{
	idEntity::RemoveTarget( target );

	if (!GetMind()->IsEmpty())
	{
		GetMind()->GetState()->OnChangeTarget(this);
	}
}

void idAI::AddTarget(idEntity* target)
{
	idEntity::AddTarget( target );

	if (!GetMind()->IsEmpty())
	{
		GetMind()->GetState()->OnChangeTarget(this);
	}
}

void idAI::SitDown()
{
	idStr waitState(WaitState());
	if (GetMoveType() != MOVETYPE_ANIM)
	{
		return;
	}
	SetMoveType(MOVETYPE_SIT_DOWN);
	SetWaitState("sit_down");
}

void idAI::GetUp()
{
	moveType_t moveType = GetMoveType();

	if ( ( moveType == MOVETYPE_SIT ) || ( moveType == MOVETYPE_SIT_DOWN ) )
	{
		SetMoveType(MOVETYPE_GET_UP);
		SetWaitState("get_up");
	}
	else if ( ( moveType == MOVETYPE_SLEEP ) || ( moveType == MOVETYPE_FALL_ASLEEP ) ) // grayman #3290 - corrected logic // grayman #3820 - MOVETYPE_FALL_ASLEEP was MOVETYPE_LAY_DOWN
	{
		SetMoveType(MOVETYPE_WAKE_UP); // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
		SetWaitState("wake_up"); // grayman #3820
		m_getupEndTime = gameLocal.time + 4300; // failsafe to stop checking m_sleepFloorZ

		// Reset visual, hearing and tactile acuity
		SetAcuity("vis", m_oldVisualAcuity);		// Tels: fix #2408
		SetAcuity("aud", GetBaseAcuity("aud") * 2); // grayman #3552
		SetAcuity("tact", GetBaseAcuity("tact") * 2); // grayman #3552
		//SetAcuity("aud", GetAcuity("aud") * 2);
		//SetAcuity("tact", GetAcuity("tact") * 2);
	}
}

// grayman #3820 - GetUp() w/o standing up

void idAI::WakeUp()
{
	moveType_t moveType = GetMoveType();

	if ( ( moveType == MOVETYPE_SLEEP ) || ( moveType == MOVETYPE_FALL_ASLEEP ) ) // grayman #3820 - MOVETYPE_FALL_ASLEEP was MOVETYPE_LAY_DOWN
	{
		SetMoveType(MOVETYPE_WAKE_UP); // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
		SetWaitState("wake_up"); // grayman #3820
		m_getupEndTime = gameLocal.time + 4300; // failsafe to stop checking m_sleepFloorZ

		// Reset visual, hearing and tactile acuity
		SetAcuity("vis", m_oldVisualAcuity);
		SetAcuity("aud", GetBaseAcuity("aud") * 2);
		SetAcuity("tact", GetBaseAcuity("tact") * 2);
	}
}

void idAI::FallAsleep()
{
	if ((GetMoveType() != MOVETYPE_ANIM) && (GetMoveType() != MOVETYPE_SIT)) // grayman #3820
	{
		return;
	}

	AI_LAY_DOWN_FACE_DIR = idAngles( 0, current_yaw, 0 ).ToForward();

	SetMoveType(MOVETYPE_FALL_ASLEEP);
	SetWaitState("fall_asleep");

	// grayman #2416 - register where the floor is. Can't just use origin.z,
	// because AI who start missions sleeping might not have lowered to the
	// floor yet when mappers start them floating above the floor.

	idVec3 start = physicsObj.GetOrigin();
	idVec3 end = start;
	end.z -= 1000;
	trace_t result;
	gameLocal.clip.TracePoint(result, start, end, MASK_OPAQUE, this);
	m_sleepFloorZ = result.endpos.z; // this point is 0.25 above the floor

	// Tels: Sleepers are blind
	m_oldVisualAcuity = GetBaseAcuity("vis"); // grayman #3552
	SetAcuity("vis", 0);

	// Reduce hearing and tactile acuity by 50%
	// TODO: use spawnargs
	SetAcuity("aud", GetBaseAcuity("aud") * 0.5); // grayman #3552
	SetAcuity("tact", GetBaseAcuity("tact") * 0.5); // grayman #3552
}

float idAI::StealthDamageMult()
{
	// multiply up if unalert
	// Damage to unconscious AI also considered sneak attack
	if( AI_AlertLevel < m_SneakAttackThresh || IsKnockedOut() )
		return m_SneakAttackMult;
	else
		return 1.0;
}

void idAI::SwapHeadAFCM(bool bUseLargerCM)
{
	if (m_HeadJointID == -1) return; // safety check for AI without head

	if (bUseLargerCM && !m_bHeadCMSwapped && spawnArgs.FindKey("blackjack_headbox_mins"))
	{
		idAFBody* headBody = af.GetPhysics()->GetBody(af.BodyForJoint(m_HeadJointID));

		idClipModel *oldClip = headBody->GetClipModel();
		idVec3	CMorig	= oldClip->GetOrigin();
		idMat3	CMaxis	= oldClip->GetAxis();
		int		CMid	= oldClip->GetId();

		oldClip->Unlink();
		// store a copy of the original cm since it will be deleted
		m_OrigHeadCM = new idClipModel(oldClip);

		idBounds HeadBounds;
		spawnArgs.GetVector( "blackjack_headbox_mins", NULL, HeadBounds[0] );
		spawnArgs.GetVector( "blackjack_headbox_maxs", NULL, HeadBounds[1] );

		idTraceModel trm;
		trm.SetupBox( HeadBounds );

		idClipModel *NewClip = new idClipModel( trm );

		// AF bodies want to have their origin at the center of mass
		float MassOut;
		idVec3 COM;
		idMat3 inertiaTensor;
		NewClip->GetMassProperties( 1.0f, MassOut, COM, inertiaTensor );
		NewClip->TranslateOrigin( -COM );
		CMorig += COM * CMaxis;
		
		NewClip->SetContents( oldClip->GetContents() );
		NewClip->Link( gameLocal.clip, this, CMid, CMorig, CMaxis );
		headBody->SetClipModel( NewClip );

		m_bHeadCMSwapped = true;
	}
	// swap back to original CM when going unconscious, if we swapped earlier
	else if( !bUseLargerCM && m_bHeadCMSwapped )
	{
		idAFBody* headBody = af.GetPhysics()->GetBody(af.BodyForJoint(m_HeadJointID));

		idClipModel *oldClip = headBody->GetClipModel();
		idVec3	CMorig	= oldClip->GetOrigin();
		idMat3	CMaxis	= oldClip->GetAxis();
		int		CMid	= oldClip->GetId();

		m_OrigHeadCM->Link( gameLocal.clip, this, CMid, CMorig, CMaxis );
		headBody->SetClipModel( m_OrigHeadCM );

		m_bHeadCMSwapped = false;
		m_OrigHeadCM = NULL;
	}
}

void idAI::CopyHeadKOInfo( void )
{
	idEntity *ent = head.GetEntity();
	if( !ent )
		return;

	// Change this if the list below changes:
	const int numArgs = 14;
	const char *copyArgs[ numArgs ] = { "ko_immune", "ko_spot_offset", "ko_zone", 
		"ko_alert_state", "ko_alert_immune", "ko_alert_immune_state",  "ko_angle_vert", "ko_angle_horiz",
		"ko_angle_alert_vert", "ko_angle_alert_horiz", "ko_rotation", "fov",
		"fov_vert", "fov_rotation"};

	const idKeyValue *tempkv;
	const char *argName;

	// for each spawnarg, overwrite it if it exists on the head
	for( int i=0; i < numArgs; i++ )
	{
		argName = copyArgs[i];
		tempkv = ent->spawnArgs.FindKey( argName );
		if( tempkv != NULL )
			spawnArgs.Set( argName, tempkv->GetValue().c_str() );
	}
}

void idAI::ParseKnockoutInfo()
{
	m_bCanBeKnockedOut = !( spawnArgs.GetBool("ko_immune", "0") );
	m_HeadCenterOffset = spawnArgs.GetVector("ko_spot_offset");
	m_KoZone = spawnArgs.GetString("ko_zone");
	m_KoAlertState = spawnArgs.GetInt("ko_alert_state");
	m_KoAlertImmuneState = spawnArgs.GetInt("ko_alert_immune_state");
	m_bKoAlertImmune = spawnArgs.GetBool("ko_alert_immune");
	idAngles tempAngles = spawnArgs.GetAngles("ko_rotation");
	m_KoRot = tempAngles.ToMat3();

	float tempAng;
	tempAng = spawnArgs.GetFloat("ko_angle_vert", "360");
	m_KoDotVert = (float)cos( DEG2RAD( tempAng * 0.5f ) );
	tempAng = spawnArgs.GetFloat("ko_angle_horiz", "360");
	m_KoDotHoriz = (float)cos( DEG2RAD( tempAng * 0.5f ) );

	// Only set the alert angles if the spawnargs exist
	const char *tempc1, *tempc2;
	tempc1 = spawnArgs.GetString("ko_angle_alert_vert");
	tempc2 = spawnArgs.GetString("ko_angle_alert_horiz");
	if( tempc1[0] != '\0' )
	{
		tempAng = atof( tempc1 );
		m_KoAlertDotVert = (float)cos( DEG2RAD( tempAng * 0.5f ) );
	}
	else
		m_KoAlertDotVert = m_KoDotVert;
	if( tempc2[0] != '\0' )
	{
		tempAng = atof( tempc2 );
		m_KoAlertDotHoriz = (float)cos( DEG2RAD( tempAng * 0.5f ) );
	}
	else
		m_KoAlertDotHoriz = m_KoDotHoriz;

	// ishtvan: Also set the FOV again, as this may be copied from the head
	// TODO: This was done once already in idActor, do we still need it there or can we remove FOV from idActor?
	float fovDegHoriz, fovDegVert;	
	spawnArgs.GetFloat( "fov", "150", fovDegHoriz );
	// If fov_vert is -1, it will be set the same as horizontal
	spawnArgs.GetFloat( "fov_vert", "-1", fovDegVert );
	SetFOV( fovDegHoriz, fovDegVert );
	tempAngles = spawnArgs.GetAngles("fov_rotation");
	m_FOVRot = tempAngles.ToMat3();
}

bool idAI::CanGreet() // grayman #3338
{
	if ( ( greetingState == ECannotGreet )    || // can never greet
		 ( greetingState == ECannotGreetYet ) || // not allowed to greet yet
		 ( AI_AlertIndex >= ai::EObservant)	  || // too alert
		 ( GetMemory().fleeing ) || // grayman #3140 - no greeting if fleeing
		 ( GetAttackFlag(COMBAT_MELEE)  && !spawnArgs.GetBool("unarmed_melee","0") )  || // visible melee weapon drawn
		 ( GetAttackFlag(COMBAT_RANGED) && !spawnArgs.GetBool("unarmed_ranged","0") ) )  // visible ranged weapon drawn
	{
		return false;
	}


	// grayman #3448 - no greeting if involved in a conversation
	// grayman #3559 - use simpler method
/*	ai::ConversationStatePtr convState = std::dynamic_pointer_cast<ai::ConversationState>(GetMind()->GetState());
	if (convState != NULL)
	{
		return false;
	}
 */

	if (m_InConversation)
	{
		return false;
	}

	return true;
}

void idAI::PocketPicked() // grayman #3559
{
	if ( spawnArgs.GetFloat("chanceNoticePickedPocket") == 0.0f )
	{
		return;
	}

	// Post the event to do the next processing step. A picked pocket reaction
	// initially waits 2s to determine if an alert occurred near the
	// picked pocket event.
	PostEventMS(&AI_PickedPocketSetup1,INITIAL_PICKPOCKET_DELAY);
}

void idAI::Event_PickedPocketSetup1() // grayman #3559
{
	// React immediately if some other alert occurred recently, otherwise
	// pause before reacting.

	int delay;
	if ( GetMemory().lastAlertRiseTime + ALERT_WINDOW <= gameLocal.time )
	{
		// The alert window has expired. React after a certain time period if you
		// pass the random chance check.

		if ( gameLocal.random.RandomFloat() > spawnArgs.GetFloat("chanceNoticePickedPocket") )
		{
			return;
		}

		float minDelay = spawnArgs.GetFloat("pickpocket_delay_min","5000");
		float maxDelay = spawnArgs.GetFloat("pickpocket_delay_max","25000");
		delay = minDelay + gameLocal.random.RandomFloat()*(maxDelay - minDelay);
		GetMemory().insideAlertWindow = false;
	}
	else
	{
		// We're inside the alert window. React immediately
		// and every time, regardless of 'chanceNoticePickedPocket' setting. If that
		// setting is "0.0", it was caught earlier, and we won't be here.
		delay = 0;
		GetMemory().insideAlertWindow = true;
	}

	// Post the event to do the next processing step after 'delay' has finished.
	PostEventMS(&AI_PickedPocketSetup2,delay);
}

void idAI::Event_PickedPocketSetup2() // grayman #3559
{
	// All delays are exhausted. It's time to react.

	// Check the conditions that would cause you to abort the reaction.

	ai::Memory& memory = GetMemory();

	if ( AI_DEAD || // stop reacting if dead
		 AI_KNOCKEDOUT || // or unconscious
		 (AI_AlertIndex >= ai::ESearching) || // stop if alert level is too high
		 m_InConversation || // in a conversation
		 memory.fleeing || // fleeing
		 m_ReactingToHit ) // already reacting to having been hit by something
	{
		memory.latchPickedPocket = false;
		memory.insideAlertWindow = false;
		return;
	}

	// Check the conditions that would cause you to delay the reaction.

	moveType_t moveType = GetMoveType();
	if (!memory.latchPickedPocket)
	{
		if ( (moveType == MOVETYPE_SLEEP) ||	// asleep
			 (moveType == MOVETYPE_SIT_DOWN) || // or in the act of sitting down
			 (moveType == MOVETYPE_FALL_ASLEEP) || // or in the act of lying down to sleep // grayman #3820 - was MOVETYPE_LAY_DOWN
			 (moveType == MOVETYPE_GET_UP) ||   // or getting up from sitting
			 (moveType == MOVETYPE_WAKE_UP) ) // or getting up from lying down // grayman #3820 - MOVETYPE_WAKE_UP was MOVETYPE_GET_UP_FROM_LYING
		{
			memory.latchPickedPocket = true;
		}
	}

	if (!memory.latchPickedPocket)
	{
		// Delay the reaction if you're handling a door or elevator and you're too far into
		// that task to have the reaction not interfere with what you're doing.
		// If doing one of those tasks, and you're not too far along, abort the task and proceed with the
		// reaction. Aborting the task allows other AI to proceed if they're handling the same
		// door or elevator.

		const ai::SubsystemPtr& subsys = movementSubsystem;
		ai::TaskPtr task = subsys->GetCurrentTask();

		if ( task != NULL )
		{
			if ( ( m_HandlingDoor && ( std::dynamic_pointer_cast<ai::HandleDoorTask>(task) != NULL ) ) ||
				 ( m_HandlingElevator && ( std::dynamic_pointer_cast<ai::HandleElevatorTask>(task) != NULL ) ) )
			{
				if (task->CanAbort())
				{
					// Abort the task and let the picked pocket reaction occur now.
					// Once the reaction is finished, the aborted task will probably
					// be reinstated, because both tasks involve trying to go somewhere,
					// and the AI will still want to do that.

					subsys->FinishTask();
				}
				else // Can't abort the task, so delay the picked pocket reaction for a while and try again later.
				{
					memory.latchPickedPocket = true;
				}
			}
		}
	}

	// Delay the reaction if another activity wants to complete first.

	if (memory.latchPickedPocket)
	{
		// Try again after a delay
		// Post the event to do the next processing step after delay has finished.
		PostEventMS(&AI_PickedPocketSetup2,LATCHED_PICKPOCKET_DELAY);
		return;
	}

	// React now.

	memory.stopReactingToPickedPocket = false;
	GetMind()->PushState(ai::StatePtr(new ai::PocketPickedState));
}

// grayman #3643

void idAI::SetUpSuspiciousDoor(CFrobDoor* door)
{
	// This is a door that's supposed to be closed.
	// Search for a while. Remember the door so you can close it later. 

	ai::Memory& memory = GetMemory();
	memory.closeMe = door;
	memory.closeSuspiciousDoor = false; // becomes TRUE when it's time to close the door
	int doorSide = GetDoorSide(door,GetPhysics()->GetOrigin()); // grayman #3756 // grayman #4227
	memory.susDoorCloseFromThisSide = doorSide; // which side of the door we're on

	door->SetSearching(this); // keeps other AI from joining in the search

	// Raise alert level

	// One more piece of evidence of something out of place
	memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_DOOR;
	memory.posEvidenceIntruders = GetPhysics()->GetOrigin(); // grayman #2903
	memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903

	if ( AI_AlertLevel < ( thresh_4 - 0.1f ) )
	{
		// grayman #3857 - move alert setup into one method
		idVec3 alertPos = door->GetDoorPosition(doorSide == DOOR_SIDE_FRONT ? DOOR_SIDE_BACK : DOOR_SIDE_FRONT,DOOR_POS_SIDEMARKER); // grayman #3756
		mind->GetState()->SetUpSearchData(ai::EAlertTypeDoor, alertPos, door, false, thresh_3 + (thresh_4 - thresh_3)/2.0f);
	}
}

// grayman #4238 - is point p obstructed by a standing humanoid AI other than yourself?

bool idAI::PointObstructed(idVec3 p)
{
	// if an AI is already there, and it's not you, ignore this spot

	idBounds b = idBounds(idVec3(p.x-16.0f, p.y-16.0f, p.z), idVec3(p.x+16.0f, p.y+16.0f, p.z+60.0f));
	idClip_ClipModelList clipModelList;
	int numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( b, MASK_MONSTERSOLID, clipModelList );
	bool obstructed = false;
	for ( int i = 0 ; i < numListedClipModels ; i++ )
	{
		idEntity* candidate = clipModelList[i]->GetEntity();
		if ( candidate != NULL )
		{
			if ( candidate == this )
			{
				continue; // can't be obstructed by yourself
			}

			if ( candidate->IsType(idAI::Type) && (candidate->GetPhysics()->GetMass() > 5) )
			{
				idAI *candidateAI = static_cast<idAI*>(candidate);
				if ( !candidateAI->AI_FORWARD ) // candidate is standing still?
				{
					obstructed = true;
					break;
				}
			}
		}
	}

	if ( !obstructed )
	{
		return false; // not obstructed
	}

	return true; // obstructed
}

/*
void idAI::PrintGoalData(idVec3 goal, int tag)
{
	idVec3 origin = GetPhysics()->GetOrigin();

	float dist = (goal - origin).LengthFast();
	DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("idAI::PrintGoalData tag = %d - %s origin = [%s], goal = [%s], moveDest = [%s], dist = %f, yaw = %f\r", tag, GetName(), origin.ToString(), move.moveDest.ToString(), goal.ToString(), dist, GetCurrentYaw());
}
*/

/*
===============
idAI::Event_PlayCustomAnim	#3597
===============
*/
void idAI::Event_PlayCustomAnim( const char *animName )
{
	if ( !AI_DEAD && !AI_KNOCKEDOUT )
	{
		actionSubsystem->PushTask(ai::TaskPtr(new ai::PlayAnimationTask( animName, 12 )));
	}
}

// grayman #4412 - idAI::FitsThrough() tries to fit the AI through
// from a head-on direction, which doesn't care about wall thickness.

// Derived from HandleDoorTask::FitsThrough(). All uses of that could be
// replaced with this, but it's used in a dozen places in the door handling task,
// and I'd rather not touch that task if at all possible.

bool idAI::FitsThrough(CFrobDoor* frobDoor)
{
	// This calculates the gap left by a partially open door
	// and checks if it is large enough for the AI to fit through it.

	idBounds aiBounds = GetPhysics()->GetBounds();
	float aiSize = 2*aiBounds[1][0] + 8;

	idAngles rotate = spawnArgs.GetAngles("rotate", "0 90 0"); // grayman #3643
	bool rotates = ( (rotate.yaw != 0) || (rotate.pitch != 0) || (rotate.roll != 0) );
	if (rotates)
	{
		idAngles tempAngle;
		frobDoor->GetMoverPhysics()->GetLocalAngles(tempAngle);

		const idVec3& closedPos = frobDoor->GetClosedPos();
		idVec3 dir = closedPos;
		dir.z = 0;
		float dist = dir.LengthFast();

		idAngles alpha = frobDoor->GetClosedAngles() - tempAngle;
		float absAlpha = idMath::Fabs(alpha.yaw);
		float delta = dist*(1.0 - idMath::Fabs(idMath::Cos(DEG2RAD(absAlpha))));

		return (delta >= aiSize);
	}

	// grayman #3643 - sliding door

	idVec3 origin = frobDoor->GetMoverPhysics()->GetOrigin(); // where origin is now
	idVec3 closedOrigin = frobDoor->GetClosedOrigin(); // where origin is when door is closed
	idVec3 delta = closedOrigin - origin;
	delta.x = idMath::Fabs(delta.x);
	delta.y = idMath::Fabs(delta.y);
	delta.z = idMath::Fabs(delta.z);
	if ( delta.x > 0 )
	{
		if (delta.x >= aiSize)
		{
			return true; // assume vertical fit
		}
	}
	else if (delta.y > 0)
	{
		if (delta.y >= aiSize) // assume vertical fit
		{
			return true;
		}
	}
	else if (delta.z > 0)
	{
		float height = aiBounds.GetSize().z;
		if (delta.z >= height)
		{
			return true; // assume horizontal fit
		}
	}

	return false;
}


