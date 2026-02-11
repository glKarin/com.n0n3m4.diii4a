/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//BC
// TEAM 0 = PLAYER FRIENDLY.  TEAM 1 = BADDIE.

const int INACCURACY_TIME = 1000; //When I haven't seen enemy for a while, make my shots inaccurate for a short period of time.

const int LASER_RADAR_MOVETIME = 750;
const int LASER_RADAR_STAYTIME = 200;

const int DAMAGEFLASHTIME = 50;

const int ENERGYSHIELD_RECHARGEINTERVAL = 250;

const int SNAPNECK_FROBINDEX = 0;

//const int MELEE_UPDATEINTERVAL = 20;
//const int MELEE_POSTATTACK_CHARGEDELAY = 1000; //after taking damage, how long to pause the melee charge.
//const int MELEE_CHARGEPAUSETIME = 500; //when player loses LOS, how long to pause the melee charge before we start depleting it.


//NOTE: suspicion time constants are located in: BC_GUNNER.CPP (SUSPICION_MAXPIPS)

#include "sys/platform.h"
#include "idlib/math/Quat.h"
#include "framework/DeclEntityDef.h"

#include "gamesys/SysCvar.h"
#include "Moveable.h"
#include "Fx.h"
#include "SmokeParticles.h"
#include "Misc.h"

#include "idlib/LangDict.h"

#include "bc_ventdoor.h"
#include "bc_meta.h"
#include "bc_frobcube.h"
#include "bc_skullsaver.h"
#include "bc_turret.h"
#include "bc_interestpoint.h"
#include "bc_gunner.h"
#include "bc_enemyspawnpoint.h"

#include "ai/AI.h"
#include "bc_ftl.h"
#include "WorldSpawn.h"


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

int idAI::nextYieldPriority = 0;

/*
=====================
idMoveState::idMoveState
=====================
*/
idMoveState::idMoveState() {
	moveType			= MOVETYPE_ANIM;
	moveCommand			= MOVE_NONE;
	moveStatus			= MOVE_STATUS_DONE;
	moveDest.Zero();
	moveDir.Set( 1.0f, 0.0f, 0.0f );
	goalEntity			= NULL;
	goalEntityOrigin.Zero();
	toAreaNum			= 0;
	startTime			= 0;
	duration			= 0;
	speed				= 0.0f;
	range				= 0.0f;
	wanderYaw			= 0;
	nextWanderTime		= 0;
	blockTime			= 0;
	obstacle			= NULL;
	lastMoveOrigin		= vec3_origin;
	lastMoveTime		= 0;
	anim				= 0;
}

/*
=====================
idMoveState::Save
=====================
*/
void idMoveState::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( (int)moveType ); // moveType_t moveType
	savefile->WriteInt( (int)moveCommand ); // moveCommand_t moveCommand
	savefile->WriteInt( (int)moveStatus ); // moveStatus_t moveStatus
	savefile->WriteVec3( moveDest ); // idVec3 moveDest
	savefile->WriteVec3( moveDir ); // idVec3 moveDir
	savefile->WriteObject( goalEntity ); // idEntityPtr<idEntity> goalEntity
	savefile->WriteVec3( goalEntityOrigin ); // idVec3 goalEntityOrigin
	savefile->WriteInt( toAreaNum ); // int toAreaNum
	savefile->WriteInt( startTime ); // int startTime
	savefile->WriteInt( duration ); // int duration
	savefile->WriteFloat( speed ); // float speed
	savefile->WriteFloat( range ); // float range
	savefile->WriteFloat( wanderYaw ); // float wanderYaw
	savefile->WriteInt( nextWanderTime ); // int nextWanderTime
	savefile->WriteInt( blockTime ); // int blockTime
	savefile->WriteObject( obstacle ); // idEntityPtr<idEntity> obstacle
	savefile->WriteVec3( lastMoveOrigin ); // idVec3 lastMoveOrigin
	savefile->WriteInt( lastMoveTime ); // int lastMoveTime
	savefile->WriteInt( anim ); // int anim
}

/*
=====================
idMoveState::Restore
=====================
*/
void idMoveState::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int&)moveType ); // moveType_t moveType
	savefile->ReadInt( (int&)moveCommand ); // moveCommand_t moveCommand
	savefile->ReadInt( (int&)moveStatus ); // moveStatus_t moveStatus
	savefile->ReadVec3( moveDest ); // idVec3 moveDest
	savefile->ReadVec3( moveDir ); // idVec3 moveDir
	savefile->ReadObject( goalEntity ); // idEntityPtr<idEntity> goalEntity
	savefile->ReadVec3( goalEntityOrigin ); // idVec3 goalEntityOrigin
	savefile->ReadInt( toAreaNum ); // int toAreaNum
	savefile->ReadInt( startTime ); // int startTime
	savefile->ReadInt( duration ); // int duration
	savefile->ReadFloat( speed ); // float speed
	savefile->ReadFloat( range ); // float range
	savefile->ReadFloat( wanderYaw ); // float wanderYaw
	savefile->ReadInt( nextWanderTime ); // int nextWanderTime
	savefile->ReadInt( blockTime ); // int blockTime
	savefile->ReadObject( obstacle ); // idEntityPtr<idEntity> obstacle
	savefile->ReadVec3( lastMoveOrigin ); // idVec3 lastMoveOrigin
	savefile->ReadInt( lastMoveTime ); // int lastMoveTime
	savefile->ReadInt( anim ); // int anim
}

/*
============
idAASFindCover::idAASFindCover
============
*/
idAASFindCover::idAASFindCover( const idVec3 &hideFromPos ) {
	int			numPVSAreas;
	idBounds	bounds( hideFromPos - idVec3( 16, 16, 0 ), hideFromPos + idVec3( 16, 16, 64 ) );

	// setup PVS
	numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, idEntity::MAX_PVS_AREAS );
	hidePVS		= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
idAASFindCover::~idAASFindCover
============
*/
idAASFindCover::~idAASFindCover() {
	gameLocal.pvs.FreeCurrentPVS( hidePVS );
}

/*
============
idAASFindCover::TestArea
============
*/
bool idAASFindCover::TestArea( const idAAS *aas, int areaNum ) {
	idVec3	areaCenter;
	int		numPVSAreas;
	int		PVSAreas[ idEntity::MAX_PVS_AREAS ];

	areaCenter = aas->AreaCenter( areaNum );
	areaCenter[ 2 ] += 1.0f;

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f), PVSAreas, idEntity::MAX_PVS_AREAS );
	if ( !gameLocal.pvs.InCurrentPVS( hidePVS, PVSAreas, numPVSAreas ) ) {
		return true;
	}

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
idAASFindAttackPosition::idAASFindAttackPosition( const idAI *self, const idMat3 &gravityAxis, idEntity *target, const idVec3 &targetPos, const idVec3 &fireOffset ) {
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


//Darkmod: Find Observation Position

/*
============
idAASFindObservationPosition::idAASFindObservationPosition
============
*/
idAASFindObservationPosition::idAASFindObservationPosition(const idAI *self, const idMat3 &gravityAxis, const idVec3 &targetPos, const idVec3 &eyeOffset, float maxDistanceFromWhichToObserve)
{
	int	numPVSAreas;

	this->targetPos = targetPos;
	this->eyeOffset = eyeOffset;
	this->self = self;
	this->gravityAxis = gravityAxis;
	this->maxObservationDistance = maxDistanceFromWhichToObserve;
	this->b_haveBestGoal = false;


	// setup PVS
	idBounds bounds(targetPos - idVec3(16, 16, 0), targetPos + idVec3(16, 16, 64));
	numPVSAreas = gameLocal.pvs.GetPVSAreas(bounds, PVSAreas, idEntity::MAX_PVS_AREAS);
	targetPVS = gameLocal.pvs.SetupCurrentPVS(PVSAreas, numPVSAreas);
}

/*
============
idAASFindObservationPosition::~idAASFindObservationPosition
============
*/
idAASFindObservationPosition::~idAASFindObservationPosition() {
	gameLocal.pvs.FreeCurrentPVS(targetPVS);
}

/*
============
idAASFindObservationPosition::TestArea
============
*/
bool idAASFindObservationPosition::TestArea(const idAAS *aas, int areaNum)
{
	idVec3	dir;
	idVec3	local_dir;
	idVec3	fromPos;
	idMat3	axis;
	idVec3	areaCenter;

	areaCenter = aas->AreaCenter(areaNum);
	areaCenter[2] += 1.0f;

	/*
	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f ), PVSAreas, idEntity::MAX_PVS_AREAS );
	if ( !gameLocal.pvs.InCurrentPVS( targetPVS, PVSAreas, numPVSAreas ) ) {
	return false;
	}
	*/

	// calculate the world transform of the view position
	dir = targetPos - areaCenter;
	gravityAxis.ProjectVector(dir, local_dir);
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	axis = local_dir.ToMat3();

	fromPos = areaCenter + eyeOffset * axis;

	// Run trace
	trace_t results;
	gameLocal.clip.TracePoint(results, fromPos, targetPos, MASK_SOLID, self);
	if (results.fraction >= 1.0f)
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

idVec3 idAI::GetObservationPosition(const idVec3& pointToObserve, const float visualAcuityZeroToOne, const unsigned short maxCost) // grayman #4347
{
	int				areaNum;
	aasObstacle_t	obstacle;
	aasGoal_t		goal;
	idBounds		bounds;
	idVec3			observeFromPos;
	aasPath_t	path;

	if (!aas)
	{
		observeFromPos = GetPhysics()->GetOrigin();
		AI_DEST_UNREACHABLE = true;
		return observeFromPos;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum = PointReachableAreaNum(org);

	// Raise point up just a bit so it isn't on the floor of the aas
	idVec3 pointToObserve2 = pointToObserve;
	pointToObserve2.z += 45.0;

	// What is the lighting along the line where the thing to be observed might be.
	float maxDistanceToObserve = 512; //BC keep this value smaller than actor.cpp VISIONBOX_MAXLENGTH so that the AI can spot the player.


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
		findGoal
	))
	{
		float bestDistance;

		// See if we can get to the point itself since noplace was good enough
		// for just looking from a distance due to lighting/occlusion/reachability.
		if (PathToGoal(path, areaNum, physicsObj.GetOrigin(), areaNum, org))
		{

			// Can reach the point itself, so walk right up to it
			observeFromPos = pointToObserve;

			// Draw the AI Debug Graphics
			//if (cv_ai_search_show.GetInteger() >= 1.0)
			//{
			//	idVec4 markerColor(0.0, 1.0, 1.0, 1.0);
			//	idVec3 arrowLength(0.0, 0.0, 50.0);
			//
			//	gameRenderWorld->DebugArrow
			//	(
			//		markerColor,
			//		observeFromPos + arrowLength,
			//		observeFromPos,
			//		2,
			//		cv_ai_search_show.GetInteger()
			//	);
			//}

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
			//if (cv_ai_search_show.GetInteger() >= 1.0)
			//{
			//	idVec4 markerColor(1.0, 1.0, 0.0, 1.0);
			//	idVec3 arrowLength(0.0, 0.0, 50.0);
			//
			//	gameRenderWorld->DebugArrow
			//	(
			//		markerColor,
			//		observeFromPos,
			//		pointToObserve,
			//		2,
			//		cv_ai_search_show.GetInteger()
			//	);
			//}
		}

		else
		{
			// No choice but to try to walk up to it as much as we can
			observeFromPos = pointToObserve;

			// Draw the AI Debug Graphics
			//if (cv_ai_search_show.GetInteger() >= 1.0)
			//{
			//	idVec4 markerColor(1.0, 0.0, 0.0, 1.0);
			//	idVec3 arrowLength(0.0, 0.0, 50.0);
			//
			//	gameRenderWorld->DebugArrow
			//	(
			//		markerColor,
			//		observeFromPos + arrowLength,
			//		observeFromPos,
			//		2,
			//		cv_ai_search_show.GetInteger()
			//	);
			//}
		}

		return observeFromPos;
	}
	else
	{
		observeFromPos = goal.origin;
		AI_DEST_UNREACHABLE = false;

		// Draw the AI Debug Graphics
		//if (cv_ai_search_show.GetInteger() >= 1.0)
		//{
		//	idVec4 markerColor(0.0, 1.0, 0.0, 1.0);
		//	idVec3 arrowLength(0.0, 0.0, 50.0);
		//
		//	gameRenderWorld->DebugArrow
		//	(
		//		markerColor,
		//		observeFromPos,
		//		pointToObserve,
		//		2,
		//		cv_ai_search_show.GetInteger()
		//	);
		//}

		return observeFromPos;
	}
}









/*
=====================
idAI::idAI
=====================
*/
idAI::idAI() {
	aas					= NULL;
	travelFlags			= TFL_WALK|TFL_AIR;

	kickForce			= 2048.0f;
	ignore_obstacles	= false;
	blockedRadius		= 0.0f;
	blockedMoveTime		= 500;
	blockedAttackTime	= 500;
	turnRate			= 360.0f;
	turnVel				= 0.0f;
	anim_turn_yaw		= 0.0f;
	anim_turn_amount	= 0.0f;
	anim_turn_angles	= 0.0f;
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
	melee_range			= 0.0f;
	projectile_height_to_distance_ratio = 1.0f;
	projectileDef		= NULL;
	projectile			= NULL;
	projectileClipModel	= NULL;
	projectileRadius	= 0.0f;
	projectileVelocity	= vec3_origin;
	projectileGravity	= vec3_origin;
	projectileSpeed		= 0.0f;
	chat_snd			= NULL;
	chat_min			= 0;
	chat_max			= 0;
	chat_time			= 0;
	talk_state			= TALK_NEVER;
	talkTarget			= NULL;

	particles.Clear();
	restartParticles	= true;
	useBoneAxis			= false;

	wakeOnFlashlight	= false;
	memset( &worldMuzzleFlash, 0, sizeof ( worldMuzzleFlash ) );
	worldMuzzleFlashHandle = -1;

	enemy				= NULL;
	lastVisibleEnemyPos.Zero();
	lastVisibleEnemyEyeOffset.Zero();
	lastVisibleReachableEnemyPos.Zero();
	lastReachableEnemyPos.Zero();
	shrivel_rate		= 0.0f;
	shrivel_start		= 0;
	fl.neverDormant		= false;		// AI's can go dormant
	current_yaw			= 0.0f;
	ideal_yaw			= 0.0f;

#ifdef _D3XP
	spawnClearMoveables	= false;
	harvestEnt			= NULL;
#endif

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

	//BC
	laserdot = NULL;
	lasersightbeam = NULL;
	lasersightbeamTarget = NULL;
	laserLockPosition = vec3_zero;
	laserEndLockPosition = vec3_zero;

	lastVisibleEnemyTime = 0;
	canSeeEnemy = false;

	laserRadarEndPos = vec3_zero;
	laserRadarStartPos = vec3_zero;
	laserRadarTimer = 0;
	laserRadarState = 0;

	bleedoutTube = NULL;
	bleedoutTime = 0;
	bleedoutState = BLEEDOUT_NONE;
	bleedoutDamageTimer = 0;

	combatState = 0;


	playedBleedoutbeep1 = false;
	playedBleedoutbeep2 = false;
	playedBleedoutbeep3 = false;
	
	outerspaceUpdateTimer = 0;

	//BC INIT end.

	// SW: set yield priority for this AI (who defers to it if they're on a collision course?) and increments the counter
	yieldPriority = nextYieldPriority;
	nextYieldPriority++;

	skullEnt = NULL;

	currentskin = nullptr;
	damageFlashSkin = nullptr;

	missileLaunchOffset.SetGranularity( 1 );
}

/*
=====================
idAI::~idAI
=====================
*/
idAI::~idAI() {
	delete projectileClipModel;
	DeconstructScriptObject();
	scriptObject.Free();
	if ( worldMuzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
		worldMuzzleFlashHandle = -1;
	}

#ifdef _D3XP
	if ( harvestEnt.GetEntity() ) {
		harvestEnt.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
#endif

	//BC
	if (spawnArgs.GetBool("has_laser", "0") && lasersightbeam != NULL)
	{
		lasersightbeam->PostEventMS(&EV_Remove, 0);
		lasersightbeam = nullptr;
		if ( lasersightbeamTarget ){
			lasersightbeamTarget->PostEventMS(&EV_Remove, 0);
			lasersightbeamTarget = nullptr;
		}
		if ( laserdot ) {
			laserdot->PostEventMS(&EV_Remove, 0);
			laserdot = nullptr;
		}
	}

	//When cleaning up, delete the bleedout gui model.
	if (bleedoutTube.IsValid())
	{
		bleedoutTube.GetEntity()->PostEventMS(&EV_Remove, 0);
		bleedoutTube = nullptr;
	}

	//Update the combat meta state.
	this->health = 0;

    if (gameLocal.metaEnt.IsValid())
    {
        static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false);
    }

	if (helmetPtr.IsValid())
	{
		helmetPtr.GetEntity()->GetPhysics()->GetClipModel()->SetOwner(nullptr);
	}
}

/*
=====================
idAI::Save
=====================
*/
void idAI::Save( idSaveGame *savefile ) const {
	int i;


	savefile->WriteBool( lastFovCheck ); //  bool lastFovCheck
	savefile->WriteInt( combatState ); //  int combatState
	savefile->WriteInt( aiState ); //  int aiState
	savefile->WriteObject( lastEnemySeen ); //  idEntityPtr<idEntity> lastEnemySeen
	savefile->WriteBool( doesRepairVerify ); //  bool doesRepairVerify

	savefile->WriteBool(aas != nullptr ); //  idAAS * aas // restore using SetAAS if active

	savefile->WriteInt( travelFlags ); //  int travelFlags

	move.Save( savefile ); //  idMoveState move
	savedMove.Save( savefile ); //  idMoveState savedMove

	savefile->WriteFloat( kickForce ); //  float kickForce
	savefile->WriteBool( ignore_obstacles ); //  bool ignore_obstacles
	savefile->WriteFloat( blockedRadius ); //  float blockedRadius
	savefile->WriteInt( blockedMoveTime ); //  int blockedMoveTime
	savefile->WriteInt( blockedAttackTime ); //  int blockedAttackTime
	savefile->WriteInt( yieldPriority ); //  int yieldPriority
	savefile->WriteInt( nextYieldPriority ); // static int nextYieldPriority // blendo eric: static, but should be ok

	savefile->WriteFloat( ideal_yaw ); //  float ideal_yaw
	savefile->WriteFloat( current_yaw ); //  float current_yaw
	savefile->WriteFloat( turnRate ); //  float turnRate
	savefile->WriteFloat( turnVel ); //  float turnVel
	savefile->WriteFloat( anim_turn_yaw ); //  float anim_turn_yaw
	savefile->WriteFloat( anim_turn_amount ); //  float anim_turn_amount
	savefile->WriteFloat( anim_turn_angles ); //  float anim_turn_angles

	savefile->WriteStaticObject( idAI::physicsObj ); //  idPhysics_Monster physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	savefile->WriteJoint( flyTiltJoint ); //  saveJoint_t flyTiltJoint
	savefile->WriteFloat( fly_speed ); //  float fly_speed
	savefile->WriteFloat( fly_bob_strength ); //  float fly_bob_strength
	savefile->WriteFloat( fly_bob_vert ); //  float fly_bob_vert
	savefile->WriteFloat( fly_bob_horz ); //  float fly_bob_horz
	savefile->WriteInt( fly_offset ); //  int fly_offset
	savefile->WriteFloat( fly_seek_scale ); //  float fly_seek_scale
	savefile->WriteFloat( fly_roll_scale ); //  float fly_roll_scale
	savefile->WriteFloat( fly_roll_max ); //  float fly_roll_max
	savefile->WriteFloat( fly_roll ); //  float fly_roll
	savefile->WriteFloat( fly_pitch_scale ); //  float fly_pitch_scale
	savefile->WriteFloat( fly_pitch_max ); //  float fly_pitch_max
	savefile->WriteFloat( fly_pitch ); //  float fly_pitch

	savefile->WriteBool( allowMove ); //  bool allowMove
	savefile->WriteBool( allowHiddenMovement ); //  bool allowHiddenMovement
	savefile->WriteBool( disableGravity ); //  bool disableGravity
	savefile->WriteBool( af_push_moveables ); //  bool af_push_moveables
	savefile->WriteBool( lastHitCheckResult ); //  bool lastHitCheckResult
	savefile->WriteInt( lastHitCheckTime ); //  int lastHitCheckTime
	savefile->WriteInt( lastAttackTime ); //  int lastAttackTime
	savefile->WriteFloat( melee_range ); //  float melee_range
	savefile->WriteFloat( projectile_height_to_distance_ratio ); //  float projectile_height_to_distance_ratio

	SaveFileWriteArray(missileLaunchOffset, missileLaunchOffset.Num(), WriteVec3);  //  idList<idVec3> missileLaunchOffset

	//savefile->WriteDict( projectileDef ); // const  idDict * projectileDef
	idStr projectileName;
	spawnArgs.GetString( "def_projectile", "", projectileName );
	savefile->WriteString( projectileName );

	savefile->WriteClipModel( projectileClipModel ); // mutable idClipModel *projectileClipModel

	savefile->WriteFloat( projectileRadius ); //  float projectileRadius
	savefile->WriteFloat( projectileSpeed ); //  float projectileSpeed
	savefile->WriteVec3( projectileVelocity ); //  idVec3 projectileVelocity
	savefile->WriteVec3( projectileGravity ); //  idVec3 projectileGravity

	projectile.Save( savefile ); //  idEntityPtr<idProjectile> projectile

	savefile->WriteString( attack ); //  idString attack

	savefile->WriteSoundShader( chat_snd ); //  const idSoundShader	*chat_snd
	savefile->WriteInt( chat_min ); //  int chat_min
	savefile->WriteInt( chat_max ); //  int chat_max
	savefile->WriteInt( chat_time ); //  int chat_time
	savefile->WriteInt( talk_state ); //  talkState_t talk_state
	talkTarget.Save( savefile ); //  idEntityPtr<idActor> talkTarget

	savefile->WriteInt( num_cinematics ); //  int num_cinematics
	savefile->WriteInt( current_cinematic ); //  int current_cinematic

	savefile->WriteBool( allowJointMod ); //  bool allowJointMod
	savefile->WriteObject( focusEntity ); //  idEntityPtr<idEntity> focusEntity
	savefile->WriteVec3( currentFocusPos ); //  idVec3 currentFocusPos
	savefile->WriteInt( focusTime ); //  int focusTime

	savefile->WriteInt( alignHeadTime ); //  int alignHeadTime
	savefile->WriteInt( forceAlignHeadTime ); //  int forceAlignHeadTime

	savefile->WriteAngles( eyeAng ); //  idAngles eyeAng
	savefile->WriteAngles( lookAng ); //  idAngles lookAng
	savefile->WriteAngles( destLookAng ); //  idAngles destLookAng
	savefile->WriteAngles( lookMin ); //  idAngles lookMin
	savefile->WriteAngles( lookMax ); //  idAngles lookMax

	SaveFileWriteArray(lookJoints, lookJoints.Num(), WriteJoint); //  idList<jointHandle_t> lookJoints
	SaveFileWriteArray(lookJointAngles, lookJointAngles.Num(), WriteAngles); //  idList<idAngles> lookJointAngles

	savefile->WriteFloat( eyeVerticalOffset ); //  float eyeVerticalOffset
	savefile->WriteFloat( eyeHorizontalOffset ); //  float eyeHorizontalOffset
	savefile->WriteFloat( eyeFocusRate ); //  float eyeFocusRate
	savefile->WriteFloat( headFocusRate ); //  float headFocusRate
	savefile->WriteInt( focusAlignTime ); //  int focusAlignTime
	savefile->WriteFloat( shrivel_rate ); //  float shrivel_rate
	savefile->WriteInt( shrivel_start ); //  int shrivel_start

	savefile->WriteBool( restartParticles ); //  bool restartParticles
	savefile->WriteBool( useBoneAxis ); //  bool useBoneAxis

	savefile->WriteInt( particles.Num() ); //  idList<particleEmitter_t> particles
	for  ( i = 0; i < particles.Num(); i++ ) {
		savefile->WriteParticle( particles[i].particle );
		savefile->WriteInt( particles[i].time );
		savefile->WriteJoint( particles[i].joint );
	}

	savefile->WriteRenderLight( worldMuzzleFlash ); //  renderLight_t worldMuzzleFlash
	savefile->WriteInt( worldMuzzleFlashHandle ); //  int worldMuzzleFlashHandle
	savefile->WriteJoint( flashJointWorld ); //  saveJoint_t flashJointWorld
	savefile->WriteInt( muzzleFlashEnd ); //  int muzzleFlashEnd
	savefile->WriteInt( flashTime ); //  int flashTime
	savefile->WriteAngles( eyeMin ); //  idAngles eyeMin
	savefile->WriteAngles( eyeMax ); //  idAngles eyeMax
	savefile->WriteJoint( focusJoint ); //  saveJoint_t focusJoint
	savefile->WriteJoint( orientationJoint ); //  saveJoint_t orientationJoint

	enemy.Save( savefile ); //  idEntityPtr<idActor> enemy
	savefile->WriteVec3( lastVisibleEnemyPos ); //  idVec3 lastVisibleEnemyPos
	savefile->WriteVec3( lastVisibleEnemyEyeOffset ); //  idVec3 lastVisibleEnemyEyeOffset
	savefile->WriteVec3( lastVisibleReachableEnemyPos ); //  idVec3 lastVisibleReachableEnemyPos
	savefile->WriteVec3( lastReachableEnemyPos ); //  idVec3 lastReachableEnemyPos
	savefile->WriteBool( wakeOnFlashlight ); //  bool wakeOnFlashlight


	savefile->WriteBool( spawnClearMoveables ); //  bool spawnClearMoveables

	savefile->WriteInt(funcEmitters.Num());
	for (int idx = 0; idx < funcEmitters.Num(); idx++) { //  idHashTable<funcEmitter_t> funcEmitters
		idStr outKey;
		funcEmitter_t outVal;
		funcEmitters.GetIndex(idx, &outKey, &outVal);
		savefile->WriteString( outKey );
		savefile->WriteString( outVal.name );
		savefile->WriteInt( outVal.joint );
		savefile->WriteObject( outVal.particle );
	}

	harvestEnt.Save( savefile ); //  idEntityPtr<idHarvestable> harvestEnt

	// hook up on load
	// 	LinkScriptVariables();
	//idScriptBool			AI_TALK; //  idScriptBool AI_TALK
	//idScriptBool			AI_DAMAGE; //  idScriptBool AI_DAMAGE
	//idScriptBool			AI_PAIN; //  idScriptBool AI_PAIN
	//idScriptFloat			AI_SPECIAL_DAMAGE; //  idScriptFloat AI_SPECIAL_DAMAGE
	//idScriptBool			AI_DEAD; //  idScriptBool AI_DEAD
	//idScriptBool			AI_ENEMY_VISIBLE; //  idScriptBool AI_ENEMY_VISIBLE
	//idScriptBool			AI_ENEMY_IN_FOV; //  idScriptBool AI_ENEMY_IN_FOV
	//idScriptBool			AI_ENEMY_DEAD; //  idScriptBool AI_ENEMY_DEAD
	//idScriptBool			AI_MOVE_DONE; //  idScriptBool AI_MOVE_DONE
	//idScriptBool			AI_ONGROUND; //  idScriptBool AI_ONGROUND
	//idScriptBool			AI_ACTIVATED; //  idScriptBool AI_ACTIVATED
	//idScriptBool			AI_FORWARD; //  idScriptBool AI_FORWARD
	//idScriptBool			AI_JUMP; //  idScriptBool AI_JUMP
	//idScriptBool			AI_ENEMY_REACHABLE; //  idScriptBool AI_ENEMY_REACHABLE
	//idScriptBool			AI_BLOCKED; //  idScriptBool AI_BLOCKED
	//idScriptBool			AI_OBSTACLE_IN_PATH; //  idScriptBool AI_OBSTACLE_IN_PATH
	//idScriptBool			AI_DEST_UNREACHABLE; //  idScriptBool AI_DEST_UNREACHABLE
	//idScriptBool			AI_HIT_ENEMY; //  idScriptBool AI_HIT_ENEMY
	//idScriptBool			AI_PUSHED; //  idScriptBool AI_PUSHED
	//idScriptBool			AI_BACKWARD; //  idScriptBool AI_BACKWARD
	//idScriptBool			AI_LEFT; //  idScriptBool AI_LEFT
	//idScriptBool			AI_RIGHT; //  idScriptBool AI_RIGHT
	//idScriptBool			AI_SHIELDHIT; //  idScriptBool AI_SHIELDHIT
	//idScriptBool			AI_CUSTOMIDLEANIM; //  idScriptBool AI_CUSTOMIDLEANIM
	//idScriptBool			AI_NODEANIM; //  idScriptBool AI_NODEANIM
	//idScriptBool			AI_DODGELEFT; //  idScriptBool AI_DODGELEFT
	//idScriptBool			AI_DODGERIGHT; //  idScriptBool AI_DODGERIGHT

	savefile->WriteObject( lasersightbeam ); //  idBeam* lasersightbeam
	savefile->WriteObject( lasersightbeamTarget ); //  idBeam* lasersightbeamTarget
	savefile->WriteObject( laserdot ); //  idEntity* laserdot

	savefile->WriteVec3( laserLockPosition ); //  idVec3 laserLockPosition
	savefile->WriteVec3( laserEndLockPosition ); //  idVec3 laserEndLockPosition

	savefile->WriteVec3( laserRadarEndPos ); //  idVec3 laserRadarEndPos
	savefile->WriteVec3( laserRadarStartPos ); //  idVec3 laserRadarStartPos
	savefile->WriteInt( laserRadarTimer ); //  int laserRadarTimer
	savefile->WriteInt( laserRadarState ); //  int laserRadarState
	savefile->WriteVec3( laserRadarDir ); //  idVec3 laserRadarDir

	savefile->WriteInt( lastVisibleEnemyTime ); //  int lastVisibleEnemyTime
	savefile->WriteBool( canSeeEnemy ); //  bool canSeeEnemy

	// idActorIcon actorIcon; //  idActorIcon actorIcon // blendo eric: regens with draw?

	savefile->WriteJoint( brassJoint ); //  saveJoint_t brassJoint
	savefile->WriteDict( &brassDict ); //  idDict brassDict

	savefile->WriteInt( lastPlayerSightTimer ); //  int lastPlayerSightTimer
	savefile->WriteInt( playersightCounter ); //  int playersightCounter

	SaveFileWriteArray(dragButtons, AI_LIMBCOUNT, WriteObject); // idEntity* dragButtons[AI_LIMBCOUNT];

	savefile->WriteSkin( currentskin );// const idDeclSkin *currentskin;
	savefile->WriteSkin( damageFlashSkin ); // const  idDeclSkin* damageFlashSkin
	savefile->WriteInt( damageflashTimer ); //  int damageflashTimer
	savefile->WriteBool( damageFlashActive ); //  bool damageFlashActive

	savefile->WriteInt( lastBrassTime ); //  int lastBrassTime
	savefile->WriteObject( bleedoutTube ); //  idEntityPtr<idEntity> bleedoutTube
	savefile->WriteInt( bleedoutTime ); //  int bleedoutTime
	savefile->WriteInt( bleedoutState ); //  int bleedoutState
	savefile->WriteInt( bleedoutDamageTimer ); //  int bleedoutDamageTimer


	savefile->WriteObject( skullEnt ); //  idEntity* skullEnt

	skullSpawnOrigLoc.Save( savefile ); //  idEntityPtr<idLocationEntity> skullSpawnOrigLoc


	savefile->WriteInt( triggerTimer ); //  int triggerTimer

	savefile->WriteBool( playedBleedoutbeep1 ); //  bool playedBleedoutbeep1
	savefile->WriteBool( playedBleedoutbeep2 ); //  bool playedBleedoutbeep2
	savefile->WriteBool( playedBleedoutbeep3 ); //  bool playedBleedoutbeep3


	savefile->WriteInt( outerspaceUpdateTimer ); //  int outerspaceUpdateTimer

	savefile->WriteObject( helmetPtr ); // idEntityPtr<idEntity> helmetPtr;
}

/*
=====================
idAI::Restore
=====================
*/
void idAI::Restore( idRestoreGame *savefile ) {
	int			num;


	savefile->ReadBool( lastFovCheck ); //  bool lastFovCheck
	savefile->ReadInt( combatState ); //  int combatState
	savefile->ReadInt( aiState ); //  int aiState
	savefile->ReadObject( lastEnemySeen ); //  idEntityPtr<idEntity> lastEnemySeen
	savefile->ReadBool( doesRepairVerify ); //  bool doesRepairVerify

	bool setAAS = false;
	savefile->ReadBool(setAAS); //  idAAS * aas // restore using SetAAS if active

	savefile->ReadInt( travelFlags ); //  int travelFlags

	move.Restore( savefile ); //  idMoveState move
	savedMove.Restore( savefile ); //  idMoveState savedMove

	savefile->ReadFloat( kickForce ); //  float kickForce
	savefile->ReadBool( ignore_obstacles ); //  bool ignore_obstacles
	savefile->ReadFloat( blockedRadius ); //  float blockedRadius
	savefile->ReadInt( blockedMoveTime ); //  int blockedMoveTime
	savefile->ReadInt( blockedAttackTime ); //  int blockedAttackTime
	savefile->ReadInt( yieldPriority ); //  int yieldPriority
	savefile->ReadInt( nextYieldPriority ); // static int nextYieldPriority // blendo eric: static, but should be ok

	savefile->ReadFloat( ideal_yaw ); //  float ideal_yaw
	savefile->ReadFloat( current_yaw ); //  float current_yaw
	savefile->ReadFloat( turnRate ); //  float turnRate
	savefile->ReadFloat( turnVel ); //  float turnVel
	savefile->ReadFloat( anim_turn_yaw ); //  float anim_turn_yaw
	savefile->ReadFloat( anim_turn_amount ); //  float anim_turn_amount
	savefile->ReadFloat( anim_turn_angles ); //  float anim_turn_angles

	savefile->ReadStaticObject( physicsObj ); //  idPhysics_Monster physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	savefile->ReadJoint( flyTiltJoint ); //  saveJoint_t flyTiltJoint
	savefile->ReadFloat( fly_speed ); //  float fly_speed
	savefile->ReadFloat( fly_bob_strength ); //  float fly_bob_strength
	savefile->ReadFloat( fly_bob_vert ); //  float fly_bob_vert
	savefile->ReadFloat( fly_bob_horz ); //  float fly_bob_horz
	savefile->ReadInt( fly_offset ); //  int fly_offset
	savefile->ReadFloat( fly_seek_scale ); //  float fly_seek_scale
	savefile->ReadFloat( fly_roll_scale ); //  float fly_roll_scale
	savefile->ReadFloat( fly_roll_max ); //  float fly_roll_max
	savefile->ReadFloat( fly_roll ); //  float fly_roll
	savefile->ReadFloat( fly_pitch_scale ); //  float fly_pitch_scale
	savefile->ReadFloat( fly_pitch_max ); //  float fly_pitch_max
	savefile->ReadFloat( fly_pitch ); //  float fly_pitch

	savefile->ReadBool( allowMove ); //  bool allowMove
	savefile->ReadBool( allowHiddenMovement ); //  bool allowHiddenMovement
	savefile->ReadBool( disableGravity ); //  bool disableGravity
	savefile->ReadBool( af_push_moveables ); //  bool af_push_moveables
	savefile->ReadBool( lastHitCheckResult ); //  bool lastHitCheckResult
	savefile->ReadInt( lastHitCheckTime ); //  int lastHitCheckTime
	savefile->ReadInt( lastAttackTime ); //  int lastAttackTime
	savefile->ReadFloat( melee_range ); //  float melee_range
	savefile->ReadFloat( projectile_height_to_distance_ratio ); //  float projectile_height_to_distance_ratio

	SaveFileReadList(missileLaunchOffset, ReadVec3);  //  idList<idVec3> missileLaunchOffset

	//savefile->ReadDict( projectileDef ); // const  idDict * projectileDef
	idStr projectileName;
	savefile->ReadString( projectileName );
	if ( projectileName.Length() ) {
		projectileDef = gameLocal.FindEntityDefDict( projectileName );
	} else {
		projectileDef = NULL;
	}

	savefile->ReadClipModel( projectileClipModel ); // mutable idClipModel *projectileClipModel

	savefile->ReadFloat( projectileRadius ); //  float projectileRadius
	savefile->ReadFloat( projectileSpeed ); //  float projectileSpeed
	savefile->ReadVec3( projectileVelocity ); //  idVec3 projectileVelocity
	savefile->ReadVec3( projectileGravity ); //  idVec3 projectileGravity

	projectile.Restore( savefile ); //  idEntityPtr<idProjectile> projectile

	savefile->ReadString( attack ); //  idString attack

	savefile->ReadSoundShader( chat_snd ); //  const idSoundShader	*chat_snd
	savefile->ReadInt( chat_min ); //  int chat_min
	savefile->ReadInt( chat_max ); //  int chat_max
	savefile->ReadInt( chat_time ); //  int chat_time
	savefile->ReadInt( (int&)talk_state ); // enum talkState_t talk_state
	talkTarget.Restore( savefile ); //  idEntityPtr<idActor> talkTarget

	savefile->ReadInt( num_cinematics ); //  int num_cinematics
	savefile->ReadInt( current_cinematic ); //  int current_cinematic

	savefile->ReadBool( allowJointMod ); //  bool allowJointMod
	savefile->ReadObject( focusEntity ); //  idEntityPtr<idEntity> focusEntity
	savefile->ReadVec3( currentFocusPos ); //  idVec3 currentFocusPos
	savefile->ReadInt( focusTime ); //  int focusTime

	savefile->ReadInt( alignHeadTime ); //  int alignHeadTime
	savefile->ReadInt( forceAlignHeadTime ); //  int forceAlignHeadTime

	savefile->ReadAngles( eyeAng ); //  idAngles eyeAng
	savefile->ReadAngles( lookAng ); //  idAngles lookAng
	savefile->ReadAngles( destLookAng ); //  idAngles destLookAng
	savefile->ReadAngles( lookMin ); //  idAngles lookMin
	savefile->ReadAngles( lookMax ); //  idAngles lookMax

	SaveFileReadList(lookJoints, ReadJoint); //  idList<saveJoint_t> lookJoints
	SaveFileReadList(lookJointAngles, ReadAngles); //  idList<idAngles> lookJointAngles

	savefile->ReadFloat( eyeVerticalOffset ); //  float eyeVerticalOffset
	savefile->ReadFloat( eyeHorizontalOffset ); //  float eyeHorizontalOffset
	savefile->ReadFloat( eyeFocusRate ); //  float eyeFocusRate
	savefile->ReadFloat( headFocusRate ); //  float headFocusRate
	savefile->ReadInt( focusAlignTime ); //  int focusAlignTime
	savefile->ReadFloat( shrivel_rate ); //  float shrivel_rate
	savefile->ReadInt( shrivel_start ); //  int shrivel_start

	savefile->ReadBool( restartParticles ); //  bool restartParticles
	savefile->ReadBool( useBoneAxis ); //  bool useBoneAxis

	savefile->ReadInt( num ); //  idList<particleEmitter_t> particles
	particles.SetNum( num );
	for  ( int idx = 0; idx < num; idx++ ) {
		savefile->ReadParticle( particles[idx].particle );
		savefile->ReadInt( particles[idx].time );
		savefile->ReadJoint( particles[idx].joint );
	}

	savefile->ReadRenderLight( worldMuzzleFlash ); //  renderLight_t worldMuzzleFlash
	savefile->ReadInt( worldMuzzleFlashHandle ); //  int worldMuzzleFlashHandle
	if ( worldMuzzleFlashHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
	}

	savefile->ReadJoint( flashJointWorld ); //  saveJoint_t flashJointWorld
	savefile->ReadInt( muzzleFlashEnd ); //  int muzzleFlashEnd
	savefile->ReadInt( flashTime ); //  int flashTime
	savefile->ReadAngles( eyeMin ); //  idAngles eyeMin
	savefile->ReadAngles( eyeMax ); //  idAngles eyeMax
	savefile->ReadJoint( focusJoint ); //  saveJoint_t focusJoint
	savefile->ReadJoint( orientationJoint ); //  saveJoint_t orientationJoint

	enemy.Restore( savefile ); //  idEntityPtr<idActor> enemy
	savefile->ReadVec3( lastVisibleEnemyPos ); //  idVec3 lastVisibleEnemyPos
	savefile->ReadVec3( lastVisibleEnemyEyeOffset ); //  idVec3 lastVisibleEnemyEyeOffset
	savefile->ReadVec3( lastVisibleReachableEnemyPos ); //  idVec3 lastVisibleReachableEnemyPos
	savefile->ReadVec3( lastReachableEnemyPos ); //  idVec3 lastReachableEnemyPos
	savefile->ReadBool( wakeOnFlashlight ); //  bool wakeOnFlashlight


	savefile->ReadBool( spawnClearMoveables ); //  bool spawnClearMoveables

	assert(funcEmitters.Num() == 0);
	savefile->ReadInt(num);
	for (int idx = 0; idx < num; idx++) { //  idHashTable<funcEmitter_t> funcEmitters
		idStr outKey;
		funcEmitter_t outVal;
		savefile->ReadString( outKey );
		savefile->ReadCharArray( outVal.name );
		savefile->ReadJoint( outVal.joint );
		savefile->ReadObject( CastClassPtrRef(outVal.particle) );
		funcEmitters.Set( outKey, outVal );
	}

	harvestEnt.Restore( savefile ); //  idEntityPtr<idHarvestable> harvestEnt

	LinkScriptVariables();
	//idScriptBool			AI_TALK; //  idScriptBool AI_TALK
	//idScriptBool			AI_DAMAGE; //  idScriptBool AI_DAMAGE
	//idScriptBool			AI_PAIN; //  idScriptBool AI_PAIN
	//idScriptFloat			AI_SPECIAL_DAMAGE; //  idScriptFloat AI_SPECIAL_DAMAGE
	//idScriptBool			AI_DEAD; //  idScriptBool AI_DEAD
	//idScriptBool			AI_ENEMY_VISIBLE; //  idScriptBool AI_ENEMY_VISIBLE
	//idScriptBool			AI_ENEMY_IN_FOV; //  idScriptBool AI_ENEMY_IN_FOV
	//idScriptBool			AI_ENEMY_DEAD; //  idScriptBool AI_ENEMY_DEAD
	//idScriptBool			AI_MOVE_DONE; //  idScriptBool AI_MOVE_DONE
	//idScriptBool			AI_ONGROUND; //  idScriptBool AI_ONGROUND
	//idScriptBool			AI_ACTIVATED; //  idScriptBool AI_ACTIVATED
	//idScriptBool			AI_FORWARD; //  idScriptBool AI_FORWARD
	//idScriptBool			AI_JUMP; //  idScriptBool AI_JUMP
	//idScriptBool			AI_ENEMY_REACHABLE; //  idScriptBool AI_ENEMY_REACHABLE
	//idScriptBool			AI_BLOCKED; //  idScriptBool AI_BLOCKED
	//idScriptBool			AI_OBSTACLE_IN_PATH; //  idScriptBool AI_OBSTACLE_IN_PATH
	//idScriptBool			AI_DEST_UNREACHABLE; //  idScriptBool AI_DEST_UNREACHABLE
	//idScriptBool			AI_HIT_ENEMY; //  idScriptBool AI_HIT_ENEMY
	//idScriptBool			AI_PUSHED; //  idScriptBool AI_PUSHED
	//idScriptBool			AI_BACKWARD; //  idScriptBool AI_BACKWARD
	//idScriptBool			AI_LEFT; //  idScriptBool AI_LEFT
	//idScriptBool			AI_RIGHT; //  idScriptBool AI_RIGHT
	//idScriptBool			AI_SHIELDHIT; //  idScriptBool AI_SHIELDHIT
	//idScriptBool			AI_CUSTOMIDLEANIM; //  idScriptBool AI_CUSTOMIDLEANIM
	//idScriptBool			AI_NODEANIM; //  idScriptBool AI_NODEANIM
	//idScriptBool			AI_DODGELEFT; //  idScriptBool AI_DODGELEFT
	//idScriptBool			AI_DODGERIGHT; //  idScriptBool AI_DODGERIGHT

	savefile->ReadObject( CastClassPtrRef(lasersightbeam) ); //  idBeam* lasersightbeam
	savefile->ReadObject( CastClassPtrRef(lasersightbeamTarget) ); //  idBeam* lasersightbeamTarget
	savefile->ReadObject( laserdot ); //  idEntity* laserdot

	savefile->ReadVec3( laserLockPosition ); //  idVec3 laserLockPosition
	savefile->ReadVec3( laserEndLockPosition ); //  idVec3 laserEndLockPosition

	savefile->ReadVec3( laserRadarEndPos ); //  idVec3 laserRadarEndPos
	savefile->ReadVec3( laserRadarStartPos ); //  idVec3 laserRadarStartPos
	savefile->ReadInt( laserRadarTimer ); //  int laserRadarTimer
	savefile->ReadInt( laserRadarState ); //  int laserRadarState
	savefile->ReadVec3( laserRadarDir ); //  idVec3 laserRadarDir

	savefile->ReadInt( lastVisibleEnemyTime ); //  int lastVisibleEnemyTime
	savefile->ReadBool( canSeeEnemy ); //  bool canSeeEnemy

	// idActorIcon actorIcon; //  idActorIcon actorIcon // blendo eric: regens with draw?

	savefile->ReadJoint( brassJoint ); //  saveJoint_t brassJoint
	savefile->ReadDict( &brassDict ); //  idDict brassDict

	savefile->ReadInt( lastPlayerSightTimer ); //  int lastPlayerSightTimer
	savefile->ReadInt( playersightCounter ); //  int playersightCounter

	SaveFileReadArray(dragButtons, ReadObject); // idEntity* dragButtons[AI_LIMBCOUNT];

	savefile->ReadSkin( currentskin );// const idDeclSkin *currentskin;
	savefile->ReadSkin( damageFlashSkin ); // const  idDeclSkin* damageFlashSkin
	savefile->ReadInt( damageflashTimer ); //  int damageflashTimer
	savefile->ReadBool( damageFlashActive ); //  bool damageFlashActive

	savefile->ReadInt( lastBrassTime ); //  int lastBrassTime
	savefile->ReadObject( bleedoutTube ); //  idEntityPtr<idEntity> bleedoutTube
	savefile->ReadInt( bleedoutTime ); //  int bleedoutTime
	savefile->ReadInt( bleedoutState ); //  int bleedoutState
	savefile->ReadInt( bleedoutDamageTimer ); //  int bleedoutDamageTimer

	savefile->ReadObject( skullEnt ); //  idEntity* skullEnt

	skullSpawnOrigLoc.Restore( savefile ); //  idEntityPtr<idLocationEntity> skullSpawnOrigLoc

	savefile->ReadInt( triggerTimer ); //  int triggerTimer

	savefile->ReadBool( playedBleedoutbeep1 ); //  bool playedBleedoutbeep1
	savefile->ReadBool( playedBleedoutbeep2 ); //  bool playedBleedoutbeep2
	savefile->ReadBool( playedBleedoutbeep3 ); //  bool playedBleedoutbeep3


	savefile->ReadInt( outerspaceUpdateTimer ); //  int outerspaceUpdateTimer

	savefile->ReadObject( helmetPtr ); // idEntityPtr<idEntity> helmetPtr;

	SetCombatModel();
	LinkCombat();

	if (setAAS) {
		SetAAS();

		// replaced:
		//// Set the AAS if the character has the correct gravity vector
		//idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
		//gravity *= g_gravity.GetFloat();
		//if ( gravity == gameLocal.GetGravity() )
	}
}

/*
=====================
idAI::Spawn
=====================
*/
void idAI::Spawn( void ) {
	const char			*jointname;
	const idKeyValue	*kv;
	idStr				jointName;
	idAngles			jointScale;
	jointHandle_t		joint;
	idVec3				local_dir;
	bool				talks;

	idDict				args;
	idVec3				muzzlePos;
	idMat3				muzzleAxis;

	const idDeclEntityDef *brassDef;

	idVec3				laserDotColor;

	if ( !g_monsters.GetBool() ) {
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	laserDotColor = spawnArgs.GetVector("laserdotcolor", "1 0 0"); //BC

	spawnArgs.GetInt(	"team",					"1",		team );
	spawnArgs.GetInt(	"rank",					"0",		rank );
	spawnArgs.GetInt(	"fly_offset",			"0",		fly_offset );
	spawnArgs.GetFloat( "fly_speed",			"100",		fly_speed );
	spawnArgs.GetFloat( "fly_bob_strength",		"50",		fly_bob_strength );
	spawnArgs.GetFloat( "fly_bob_vert",			"2",		fly_bob_horz );
	spawnArgs.GetFloat( "fly_bob_horz",			"2.7",		fly_bob_vert );
	spawnArgs.GetFloat( "fly_seek_scale",		"4",		fly_seek_scale );
	spawnArgs.GetFloat( "fly_roll_scale",		"90",		fly_roll_scale );
	spawnArgs.GetFloat( "fly_roll_max",			"60",		fly_roll_max );
	spawnArgs.GetFloat( "fly_pitch_scale",		"45",		fly_pitch_scale );
	spawnArgs.GetFloat( "fly_pitch_max",		"30",		fly_pitch_max );

	spawnArgs.GetFloat( "melee_range",			"64",		melee_range );
	spawnArgs.GetFloat( "projectile_height_to_distance_ratio",	"1", projectile_height_to_distance_ratio );

	spawnArgs.GetFloat( "turn_rate",			"360",		turnRate );

	spawnArgs.GetBool( "talks",					"0",		talks );
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
	spawnArgs.GetBool( "af_push_moveables",		"0",		af_push_moveables );
	spawnArgs.GetFloat( "kick_force",			"4096",		kickForce );
	spawnArgs.GetBool( "ignore_obstacles",		"0",		ignore_obstacles );
	spawnArgs.GetFloat( "blockedRadius",		"20",		blockedRadius );
	spawnArgs.GetInt( "blockedMoveTime",		"500",		blockedMoveTime );
	spawnArgs.GetInt( "blockedAttackTime",		"500",		blockedAttackTime );

	spawnArgs.GetInt(	"num_cinematics",		"0",		num_cinematics );
	current_cinematic = 0;

	LinkScriptVariables();

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

	// calculate joint positions on attack frames so we can do proper "can hit" tests
	CalculateAttackOffsets();

	eyeMin				= spawnArgs.GetAngles( "eye_turn_min", "-10 -30 0" );
	eyeMax				= spawnArgs.GetAngles( "eye_turn_max", "10 30 0" );
	eyeVerticalOffset	= spawnArgs.GetFloat( "eye_verticle_offset", "5" );
	eyeHorizontalOffset = spawnArgs.GetFloat( "eye_horizontal_offset", "-8" );
	eyeFocusRate		= spawnArgs.GetFloat( "eye_focus_rate", "0.5" );
	headFocusRate		= spawnArgs.GetFloat( "head_focus_rate", "0.1" );
	focusAlignTime		= SEC2MS( spawnArgs.GetFloat( "focus_align_time", "1" ) );

	flashJointWorld = animator.GetJointHandle( spawnArgs.GetString("bone_muzzleflash") );
	brassJoint = animator.GetJointHandle(spawnArgs.GetString("bone_brass"));

	brassDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_ejectBrass"), false);
	if (brassDef)
	{
		brassDict = brassDef->dict;
	}	

	if ( head.GetEntity() ) {
		idAnimator *headAnimator = head.GetEntity()->GetAnimator();

		jointname = spawnArgs.GetString( "bone_focus" );
		if ( *jointname ) {
			focusJoint = headAnimator->GetJointHandle( jointname );
			if ( focusJoint == INVALID_JOINT ) {
				gameLocal.Warning( "Joint '%s' not found on head on '%s'", jointname, name.c_str() );
			}
		}
	} else {
		jointname = spawnArgs.GetString( "bone_focus" );
		if ( *jointname ) {
			focusJoint = animator.GetJointHandle( jointname );
			if ( focusJoint == INVALID_JOINT ) {
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

	projectile		= NULL;
	projectileDef	= NULL;
	projectileClipModel	= NULL;
	idStr projectileName;
	if ( spawnArgs.GetString( "def_projectile", "", projectileName ) && projectileName.Length() ) {
		projectileDef = gameLocal.FindEntityDefDict( projectileName );
		CreateProjectile( vec3_origin, viewAxis[ 0 ] );
		projectileRadius	= projectile.GetEntity()->GetPhysics()->GetClipModel()->GetBounds().GetRadius();
		projectileVelocity	= idProjectile::GetVelocity( projectileDef );
		projectileGravity	= idProjectile::GetGravity( projectileDef );
		projectileSpeed		= projectileVelocity.Length();
		delete projectile.GetEntity();
		projectile = NULL;
	}

	particles.Clear();
	restartParticles = true;
	useBoneAxis = spawnArgs.GetBool( "useBoneAxis" );
	SpawnParticles( "smokeParticleSystem" );
	SpawnEmitters("emitterSystem");


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

	// set up monster chatter
	SetChatSound();

	BecomeActive( TH_THINK );

	if ( af_push_moveables ) {
		af.SetupPose( this, gameLocal.time );
		af.GetPhysics()->EnableClip();
	}

	// init the move variables
	StopMove( MOVE_STATUS_DONE );


#ifdef _D3XP
	spawnArgs.GetBool( "spawnClearMoveables", "0", spawnClearMoveables );
#endif


	//BC spawn lasersight beam.	

	//target = end of the traceline.
	if (spawnArgs.GetBool("has_laser", "0"))
	{
		args.SetVector("origin", vec3_origin);
		lasersightbeamTarget = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
		lasersightbeamTarget->BecomeActive(TH_PHYSICS);

		GetJointWorldTransform(flashJointWorld, gameLocal.time, muzzlePos, muzzleAxis);

		//beam origin = bind to weapon muzzle.
		args.Clear();
		args.Set("target", lasersightbeamTarget->name.c_str());
		args.SetVector("origin", muzzlePos);
		args.SetBool("start_off", false);
		args.Set("width", spawnArgs.GetString("laserwidth", "6"));
		args.Set("skin", spawnArgs.GetString("laserskin", "skins/beam_red"));
		lasersightbeam = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
		lasersightbeam->BindToJoint(this, flashJointWorld, false);

		args.Clear();
		args.SetVector("origin", vec3_origin);
		args.Set("model", "models/objects/lasersight/tris.ase");
		args.SetInt("solid", 0);
		args.SetBool("noclipmodel", true);
		laserdot = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
		laserdot->GetRenderEntity()->shaderParms[SHADERPARM_RED] = laserDotColor.x;
		laserdot->GetRenderEntity()->shaderParms[SHADERPARM_GREEN] = laserDotColor.y;
		laserdot->GetRenderEntity()->shaderParms[SHADERPARM_BLUE] = laserDotColor.z;
	}

	lastVisibleEnemyTime = 0;
	canSeeEnemy = false;

	lastPlayerSightTimer = 0;
	playersightCounter = 0;

	lastFovCheck = false;


	//Grab the skin being used.	
	//const char *skinname = spawnArgs.GetString("skin", "");
	//currentskin = this->renderEntity.customSkin;
	//damageFlashSkin = declManager->FindSkin(spawnArgs.GetString("skin_damageflash", "skins/monsters/thug/damageflash"));
	//damageflashTimer = 0;
	//damageFlashActive = false;

	lastBrassTime = 0;



	if (spawnArgs.GetBool("draggable", "0"))
	{
		//Spawn the drag frobcubes at four points: lefthand, righthand, leftfoot, rightfoot
		SpawnDragButton(0, spawnArgs.GetString("handdrag_joint_1"), "Hold hand");
		SpawnDragButton(1, spawnArgs.GetString("handdrag_joint_2"), "Hold hand");
		SpawnDragButton(2, spawnArgs.GetString("footdrag_joint_1"), "Hold foot");
		SpawnDragButton(3, spawnArgs.GetString("footdrag_joint_2"), "Hold foot");
	}

	if (spawnArgs.GetBool("can_snapneck"))
	{
		//The snap neck frob box.
		SpawnDragButton(SNAPNECK_FROBINDEX, spawnArgs.GetString("snapneck_joint", "head"), common->GetLanguageDict()->GetString("#str_def_gameplay_ejecthead"));
	}


	lastDamageOrigin = vec3_zero;
    triggerTimer = 0;
	lastEnemySeen = NULL;


	doesRepairVerify = spawnArgs.GetBool("does_repair_verify", "1");

	aiState = AISTATE_IDLE;
	
	PostEventMS(&EV_PostSpawn, 0);

	skullSpawnOrigLoc = nullptr; // blendo eric: skullSpawnOrigLoc set in event_postspawn
}


#ifdef _D3XP
void idAI::Gib( const idVec3 &dir, const char *damageDefName ) {

	ForceStopDrag();
	SetDragFrobbable(false); //disable drag frobcubes.

	if(harvestEnt.GetEntity()) {
		//Let the harvest ent know that we gibbed
		harvestEnt.GetEntity()->Gib();
	}
	idActor::Gib(dir, damageDefName);

	if (bleedoutTube.IsValid())
	{
		bleedoutTube.GetEntity()->Hide();
	}	

	SpawnSkullsaver();
}
#endif

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
}

/*
=====================
idAI::Think
=====================
*/
void idAI::Think( void ) //AI calls this every frame.
{
	// if we are completely closed off from the player, don't do anything at all
	if ( CheckDormant() ) { return; }

	if (bleedoutState == BLEEDOUT_SKULLBURN && gameLocal.time >= bleedoutTime)
	{
		bleedoutState = BLEEDOUT_DONE;
		Hide();
	}

	if ( thinkFlags & TH_THINK )
	{
		// clear out the enemy when he dies or is hidden
		idActor *enemyEnt = enemy.GetEntity();

		if ( enemyEnt )
		{
			if ( enemyEnt->health <= 0 )
			{
				EnemyDead();
			}
		}

		current_yaw += deltaViewAngles.yaw;
		ideal_yaw = idMath::AngleNormalize180( ideal_yaw + deltaViewAngles.yaw );
		deltaViewAngles.Zero();
		viewAxis = idAngles( 0, current_yaw, 0 ).ToMat3();

		if ( num_cinematics ) {
			if ( !IsHidden() && torsoAnim.AnimDone( 0 ) ) {
				PlayCinematic();
			}
			RunPhysics();
		} else if ( !allowHiddenMovement && IsHidden() ) {
			// hidden monsters
			UpdateAIScript();
		} else {
			// clear the ik before we do anything else so the skeleton doesn't get updated twice
			walkIK.ClearJointMods();

			switch( move.moveType ) {

			case MOVETYPE_ANIM:
				// animation-based movement. This is the main focus of the game: on-foot monsters. For example, the gunner person uses this ai type.
				//UpdateEnemyPosition();	//Update enemy info.
				UpdateAIScript();		//Update anim states.
				AnimMove();				//Handle movement.
				//PlayChatter();
				//CheckBlink();
				break;

			case MOVETYPE_DEAD :
				// dead monsters
				UpdateAIScript();
				DeadMove();
				break;

			case MOVETYPE_FLY :
				// flying monsters. This is another type that we use often.
				UpdateEnemyPosition();
				UpdateAIScript();
				FlyMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_STATIC :
				// static monsters. Not really used.......
				UpdateEnemyPosition();
				UpdateAIScript();
				StaticMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_SLIDE :
				// velocity based movement. Not really used.........
				UpdateEnemyPosition();
				UpdateAIScript();
				SlideMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_PUPPET:
				UpdateAnimState();
				UpdateAnimationControllers();
				break;
			}
		}

		// clear pain flag so that we recieve any damage between now and the next time we run the script
		AI_PAIN = false;
		//AI_SPECIAL_DAMAGE = 0; //BC
		AI_PUSHED = false;
	} else if ( thinkFlags & TH_PHYSICS ) {
		RunPhysics();
	}

	if ( af_push_moveables ) {
		PushWithAF();
	}

	if ( fl.hidden && allowHiddenMovement ) {
		// UpdateAnimation won't call frame commands when hidden, so call them here when we allow hidden movement
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}
/*	this still draws in retail builds.. not sure why.. don't care at this point.
	if ( !aas && developer.GetBool() && !fl.hidden && !num_cinematics ) {
		gameRenderWorld->DrawText( "No AAS", physicsObj.GetAbsBounds().GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, gameLocal.msec );
	}
*/
	UpdateSpacePush();
	UpdateMuzzleFlash();
	UpdateAnimation();
	UpdateParticles();
	Present();
	UpdateDamageEffects();
	LinkCombat();


	if(ai_showHealth.GetBool())
	{
		idVec3 aboveHead(0,0,35);
		gameRenderWorld->DrawText( va( "%d", ( int )health), this->GetEyePosition()+aboveHead, 0.5f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
	}

	if (ai_showAnimState.GetBool())
	{
		int textPos = 8;
		for (int chIdx = ANIMCHANNEL_TORSO; chIdx <= ANIMCHANNEL_HEAD; chIdx++)
		{
			idStr animStateStr = GetAnimState(chIdx);
			if (ai_showAnimState.GetInteger() == 2) { animStateStr.Append(" ["); animStateStr.Append(GetAnimName(chIdx)); animStateStr.Append("]"); }
			gameRenderWorld->DrawText(idStr::Format("%s: %s", ANIMCHANNEL_Names[chIdx], animStateStr.c_str()), this->GetEyePosition() + idVec3(0, 0, textPos), 0.3f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
			textPos += 8;
		}
	}

	UpdateIcon();
	
	
	if (lasersightbeam != NULL && lasersightbeamTarget != NULL && health > 0)
	{
		idVec3 laserStartPos;
		idMat3 rifleAxis;
		idVec3	laserMuzzlePos;
		idMat3	laserAxis;
		trace_t	laserTrace;
		idVec3  laserDir;

		GetJointWorldTransform(brassJoint, gameLocal.time, laserStartPos, rifleAxis);
		GetJointWorldTransform(flashJointWorld, gameLocal.time, laserMuzzlePos, laserAxis);


		if (laserLockPosition == vec3_zero)
		{
			//we do not have laser lock on.

			if (spawnArgs.GetBool("laser_radar",  "0"))
			{
				//radar mode. laser goes everywhere.
				idVec3 currentLaserPos;
				idVec3 tempMuzzlePos = vec3_zero;

				if (laserRadarState == 0) //laser idle. stay in same spot for a while.
				{
					currentLaserPos = laserRadarEndPos;

					if (gameLocal.time >= laserRadarTimer)
					{
						//go to laser radar moving state.
						laserRadarState = 1;
						laserRadarTimer = gameLocal.time + LASER_RADAR_MOVETIME;

						laserRadarEndPos = tempMuzzlePos + idVec3(-32 + gameLocal.random.RandomInt(64), -32 + gameLocal.random.RandomInt(64), -32 + gameLocal.random.RandomInt(64));
					}
				}
				else //in laser radar moving state.
				{
					float lerp = (laserRadarTimer - gameLocal.time) / (float)LASER_RADAR_MOVETIME;
					if (lerp > 1)
						lerp = 1;

					currentLaserPos.x = idMath::Lerp(laserRadarEndPos.x, laserRadarStartPos.x, lerp);
					currentLaserPos.y = idMath::Lerp(laserRadarEndPos.y, laserRadarStartPos.y, lerp);
					currentLaserPos.z = idMath::Lerp(laserRadarEndPos.z, laserRadarStartPos.z, lerp);

					if (gameLocal.time >= laserRadarTimer)
					{
						laserRadarState = 0;
						laserRadarStartPos = laserRadarEndPos;

						laserRadarTimer = gameLocal.time + LASER_RADAR_STAYTIME;
					}
				}

				laserDir = tempMuzzlePos - currentLaserPos;
			}
			else
			{
				//point forward
				laserDir = laserMuzzlePos - laserStartPos;
			}
		}
		else
		{
			laserDir = laserLockPosition - laserMuzzlePos;
		}

		laserDir.Normalize();

		if (laserEndLockPosition == vec3_zero)
		{
			gameLocal.clip.TracePoint(laserTrace, laserMuzzlePos, laserMuzzlePos + (laserDir * 2048), MASK_SHOT_RENDERMODEL, this);

			//gameRenderWorld->DebugArrow(colorGreen,  laserStartPos, laserStartPos + laserDir * 128, 8);

			lasersightbeamTarget->GetPhysics()->SetOrigin(laserTrace.endpos);
		}
		else
		{
			//Lock laser END position at a specific spot.
			lasersightbeamTarget->GetPhysics()->SetOrigin(laserEndLockPosition);
		}

		laserdot->SetOrigin(lasersightbeamTarget->GetPhysics()->GetOrigin());
		laserdot->SetAxis(laserTrace.c.normal.ToMat3());
	}

	if (gameLocal.time > lastPlayerSightTimer + AI_SUSPICIONEXPIRETIMER && playersightCounter > 0)
	{
		playersightCounter = 0;
	}

	//if (gameLocal.time > damageflashTimer && damageFlashActive)
	//{
	//	damageFlashActive = false;
	//
	//	if (!AI_DEAD)
	//	{
	//		SetSkin(currentskin); //Only reset the skin if not dead. otherwise, let the script State_Killed() handle skin stuff.
	//	}
	//}

	if (health <= 0 && bleedoutTube.IsValid() && team == 1 && bleedoutState == BLEEDOUT_ACTIVE)
	{
		TouchTriggers();

		idVec3 tubeOrigin;
		idMat3 tubeAxis;
		jointHandle_t headJoint = animator.GetJointHandle("head");
		GetJointWorldTransform(headJoint, gameLocal.time, tubeOrigin, tubeAxis);
		bleedoutTube.GetEntity()->SetOrigin(tubeOrigin + idVec3(0,0,32));	
		
		bleedoutTube.GetEntity()->Event_SetGuiInt("gui_bleedout0", (bleedoutTime - gameLocal.time) / 1000);



		if (bleedoutTime - gameLocal.time <= 3000 && !playedBleedoutbeep1)
		{
			playedBleedoutbeep1 = true;
			StartSound("snd_bleedoutbeeplow", SND_CHANNEL_ANY, 0, false, NULL);
		}
		else if (bleedoutTime - gameLocal.time <= 2000 && !playedBleedoutbeep2)
		{
			playedBleedoutbeep2 = true;
			StartSound("snd_bleedoutbeeplow", SND_CHANNEL_ANY, 0, false, NULL);
		}
		else if (bleedoutTime - gameLocal.time <= 1000 && !playedBleedoutbeep3)
		{
			playedBleedoutbeep3 = true;
			StartSound("snd_bleedoutbeephigh", SND_CHANNEL_ANY, 0, false, NULL);
		}



		if (gameLocal.time > bleedoutTime)
		{
			//total bleed out. No more blood. Do the skull saver stuff.

			// SW: burnawaytime is the time it takes the corpse to fully burn away. 
			// This should be equal to (or longer) than the time it takes the model shader to fully erase the body once parm7 is set
			bleedoutTime = gameLocal.time + spawnArgs.GetFloat("burnawaytime", "2.5") * 1000; 
			if (bleedoutTube.IsValid()) {
				bleedoutTube.GetEntity()->Hide();
			}

			GetPhysics()->SetContents(0);
			fl.takedamage = false;

			//gib.
			//this->Hide();
			//Gib(idVec3(0, 0, 1), "damage_suicide");

			//body dissolve away.
			Event_PreBurn();
			Event_Burn();
			

			ForceStopDrag();
			SetDragFrobbable(false);

			//Spawn the skullsaver.
			SpawnSkullsaver();
		}
	}


	if (energyShieldMax > 0 && health > 0)
	{
		if (energyShieldState == ENERGYSHIELDSTATE_REGENDELAY)
		{
			if (gameLocal.time >= energyShieldTimer)
			{
				energyShieldState = ENERGYSHIELDSTATE_REGENERATING;
				energyShieldTimer = 0;

				StartSound("snd_energyshield_charge", SND_CHANNEL_HEART, 0, false, NULL);				
			}
		}
		else if (energyShieldState == ENERGYSHIELDSTATE_REGENERATING)
		{
			if (gameLocal.time >= energyShieldTimer)
			{
				energyShieldTimer = gameLocal.time + ENERGYSHIELD_RECHARGEINTERVAL;
				energyShieldCurrent += 1;

				if (energyShieldCurrent >= energyShieldMax)
				{
					energyShieldCurrent = energyShieldMax;
					energyShieldState = ENERGYSHIELDSTATE_IDLE;
				}
			}
		}		
	}

	if (aas == NULL)
	{
		//BC draw an in-game warning to make it clearer.
		gameRenderWorld->DrawText(va("%s has no AAS", name.c_str()), GetPhysics()->GetOrigin(), .5f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 100);
	}
	

}

void idAI::UpdateIcon()
{
	jointHandle_t headJoint = animator.GetJointHandle("head");
	actorIcon.Draw(this, headJoint, aiState);
}

/***********************************************************************

	AI script state management

***********************************************************************/

/*
=====================
idAI::LinkScriptVariables
=====================
*/
void idAI::LinkScriptVariables( void ) {
	AI_TALK.LinkTo(				scriptObject, "AI_TALK" );
	AI_DAMAGE.LinkTo(			scriptObject, "AI_DAMAGE" );
	AI_PAIN.LinkTo(				scriptObject, "AI_PAIN" );
	AI_SPECIAL_DAMAGE.LinkTo(	scriptObject, "AI_SPECIAL_DAMAGE" );
	AI_DEAD.LinkTo(				scriptObject, "AI_DEAD" );
	AI_ENEMY_VISIBLE.LinkTo(	scriptObject, "AI_ENEMY_VISIBLE" );
	AI_ENEMY_IN_FOV.LinkTo(		scriptObject, "AI_ENEMY_IN_FOV" );
	AI_ENEMY_DEAD.LinkTo(		scriptObject, "AI_ENEMY_DEAD" );
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

	//BC
	AI_BACKWARD.LinkTo(scriptObject, "AI_BACKWARD");
	AI_LEFT.LinkTo(scriptObject, "AI_LEFT");
	AI_RIGHT.LinkTo(scriptObject, "AI_RIGHT");
	AI_SHIELDHIT.LinkTo(scriptObject, "AI_SHIELDHIT");
	AI_CUSTOMIDLEANIM.LinkTo(scriptObject, "AI_CUSTOMIDLEANIM");
	AI_NODEANIM.LinkTo(scriptObject, "AI_NODEANIM");
	AI_DODGELEFT.LinkTo(scriptObject, "AI_DODGELEFT");
	AI_DODGERIGHT.LinkTo(scriptObject, "AI_DODGERIGHT");
}

/*
=====================
idAI::UpdateAIScript
=====================
*/
void idAI::UpdateAIScript( void ) {
	UpdateScript();

	// clear the hit enemy flag so we catch the next time we hit someone
	AI_HIT_ENEMY = false;

	if ( allowHiddenMovement || !IsHidden() ) {
		// update the animstate if we're not hidden
		UpdateAnimState();
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
	idClipModel *clipModelList[ MAX_GENTITIES ];
	int clipmask;
	idVec3 org;
	idVec3 forceVec;
	idVec3 delta;
	idVec2 perpendicular;

	org = physicsObj.GetOrigin();

	// find all possible obstacles
	clipBounds = physicsObj.GetAbsBounds();
	clipBounds.TranslateSelf( dir * 32.0f );
	clipBounds.ExpandSelf( 8.0f );
	clipBounds.AddPoint( org );
	clipmask = physicsObj.GetClipMask();
	numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, clipmask, clipModelList, MAX_GENTITIES );
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

		if ( obEnt->IsType( idMoveable::Type ) && obEnt->GetPhysics()->IsPushable() ) {
			delta = obEnt->GetPhysics()->GetOrigin() - org;
			delta.NormalizeFast();
			perpendicular.x = -delta.y;
			perpendicular.y = delta.x;
			delta.z += 0.5f;
			delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
			forceVec = delta * force * obEnt->GetPhysics()->GetMass();
			obEnt->ApplyImpulse( this, 0, obEnt->GetPhysics()->GetOrigin(), forceVec );
		}
	}

	if ( alwaysKick ) {
		delta = alwaysKick->GetPhysics()->GetOrigin() - org;
		delta.NormalizeFast();
		perpendicular.x = -delta.y;
		perpendicular.y = delta.x;
		delta.z += 0.5f;
		delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
		forceVec = delta * force * alwaysKick->GetPhysics()->GetMass();
		alwaysKick->ApplyImpulse( this, 0, alwaysKick->GetPhysics()->GetOrigin(), forceVec );
	}
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
				return;
			}
			float height = settings->maxStepHeight;
			physicsObj.SetMaxStepHeight( height );
			return;
		} else {
			aas = NULL;
		}
	}
	gameLocal.Warning( "WARNING: %s has no AAS file\n", name.c_str() );
}

/*
=====================
idAI::DrawRoute
=====================
*/
void idAI::DrawRoute( void ) const {
	if ( aas && move.toAreaNum && move.moveCommand != MOVE_NONE && move.moveCommand != MOVE_WANDER && move.moveCommand != MOVE_FACE_ENEMY && move.moveCommand != MOVE_FACE_ENTITY && move.moveCommand != MOVE_TO_POSITION_DIRECT ) {
		if ( move.moveType == MOVETYPE_FLY ) {
			aas->ShowFlyPath( physicsObj.GetOrigin(), move.toAreaNum, move.moveDest );
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
bool idAI::ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const {
	if ( move.moveType == MOVETYPE_SLIDE ) {
		idBounds bnds( idVec3( -4, -4.0f, -8.0f ), idVec3( 4.0f, 4.0f, 64.0f ) );
		bnds.TranslateSelf( physicsObj.GetOrigin() );
		if ( bnds.ContainsPoint( pos ) ) {
			return true;
		}
	} else {
		if ( ( moveCommand == MOVE_TO_ENEMY ) || ( moveCommand == MOVE_TO_ENTITY ) ) {
			if ( physicsObj.GetAbsBounds().IntersectsBounds( idBounds( pos ).Expand( 8.0f ) ) ) {
				return true;
			}
		} else {
			idBounds bnds( idVec3( -16.0, -16.0f, -8.0f ), idVec3( 16.0, 16.0f, 64.0f ) );
			bnds.TranslateSelf( physicsObj.GetOrigin() );
			if ( bnds.ContainsPoint( pos ) ) {
				return true;
			}
		}
	}
	return false;
}

/*
=====================
idAI::PointReachableAreaNum
=====================
*/
int idAI::PointReachableAreaNum( const idVec3 &pos, const float boundsScale ) const {
	int areaNum;
	idVec3 size;
	idBounds bounds;

	if ( !aas ) {
		return 0;
	}

	size = aas->GetSettings()->boundingBoxes[0][1] * boundsScale;
	bounds[0] = -size;
	size.z = 32.0f;
	bounds[1] = size;

	if ( move.moveType == MOVETYPE_FLY ) {
		areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK | AREA_REACHABLE_FLY );
	} else {
		areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK );
	}

	return areaNum;
}

/*
=====================
idAI::PathToGoal
=====================
*/
bool idAI::PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const {
	idVec3 org;
	idVec3 goal;

	if ( !aas ) {
		return false;
	}

	org = origin;
	aas->PushPointIntoAreaNum( areaNum, org );
	if ( !areaNum ) {
		return false;
	}

	goal = goalOrigin;
	aas->PushPointIntoAreaNum( goalAreaNum, goal );
	if ( !goalAreaNum ) {
		return false;
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		return aas->FlyPathToGoal( path, areaNum, org, goalAreaNum, goal, travelFlags );
	} else {
		return aas->WalkPathToGoal( path, areaNum, org, goalAreaNum, goal, travelFlags );
	}
}

/*
=====================
idAI::TravelDistance

Returns the approximate travel distance from one position to the goal, or if no AAS, the straight line distance.

This is feakin' slow, so it's not good to do it too many times per frame.  It also is slower the further you
are from the goal, so try to break the goals up into shorter distances.
=====================
*/
float idAI::TravelDistance( const idVec3 &start, const idVec3 &end ) const {
	int			fromArea;
	int			toArea;
	float		dist;
	idVec2		delta;

	if ( !aas ) {
		// no aas, so just take the straight line distance
		delta = end.ToVec2() - start.ToVec2();
		dist = delta.LengthFast();

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugLine( colorBlue, start, end, gameLocal.msec, false );
			gameRenderWorld->DrawText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}

		return dist;
	}

	fromArea = PointReachableAreaNum( start );
	toArea = PointReachableAreaNum( end );

	if ( !fromArea || !toArea ) {
		// can't seem to get there
		return -1;
	}

	if ( fromArea == toArea ) {
		// same area, so just take the straight line distance
		delta = end.ToVec2() - start.ToVec2();
		dist = delta.LengthFast();

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugLine( colorBlue, start, end, gameLocal.msec, false );
			gameRenderWorld->DrawText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}

		return dist;
	}

	idReachability *reach;
	int travelTime;
	if ( !aas->RouteToGoalArea( fromArea, start, toArea, travelFlags, travelTime, &reach ) ) {
		return -1;
	}

	if ( ai_debugMove.GetBool() ) {
		if ( move.moveType == MOVETYPE_FLY ) {
			aas->ShowFlyPath( start, toArea, end );
		} else {
			aas->ShowWalkPath( start, toArea, end );
		}
	}

	return travelTime;
}

/*
=====================
idAI::StopMove
=====================
*/
void idAI::StopMove( moveStatus_t status ) {
	AI_MOVE_DONE		= true;
	AI_FORWARD			= false;
	move.moveCommand	= MOVE_NONE;
	move.moveStatus		= status;
	move.toAreaNum		= 0;
	move.goalEntity		= NULL;
	move.moveDest		= physicsObj.GetOrigin();
	AI_DEST_UNREACHABLE	= false;
	AI_OBSTACLE_IN_PATH = false;
	AI_BLOCKED			= false;
	move.startTime		= gameLocal.time;
	move.duration		= 0;
	move.range			= 0.0f;
	move.speed			= 0.0f;
	move.anim			= 0;
	move.moveDir.Zero();
	move.lastMoveOrigin.Zero();
	move.lastMoveTime	= gameLocal.time;
}

/*
=====================
idAI::FaceEnemy

Continually face the enemy's last known position.  MoveDone is always true in this case.
=====================
*/
bool idAI::FaceEnemy( void ) {
	idActor *enemyEnt = enemy.GetEntity();
	if ( !enemyEnt ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	TurnToward( lastVisibleEnemyPos );
	move.goalEntity		= enemyEnt;
	move.moveDest		= physicsObj.GetOrigin();
	move.moveCommand	= MOVE_FACE_ENEMY;
	move.moveStatus		= MOVE_STATUS_WAITING;
	move.startTime		= gameLocal.time;
	move.speed			= 0.0f;
	AI_MOVE_DONE		= true;
	AI_FORWARD			= false;
	AI_DEST_UNREACHABLE = false;

	return true;
}

/*
=====================
idAI::FaceEntity

Continually face the entity position.  MoveDone is always true in this case.
=====================
*/
bool idAI::FaceEntity( idEntity *ent ) {
	if ( !ent ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	idVec3 entityOrg = ent->GetPhysics()->GetOrigin();
	TurnToward( entityOrg );
	move.goalEntity		= ent;
	move.moveDest		= physicsObj.GetOrigin();
	move.moveCommand	= MOVE_FACE_ENTITY;
	move.moveStatus		= MOVE_STATUS_WAITING;
	move.startTime		= gameLocal.time;
	move.speed			= 0.0f;
	AI_MOVE_DONE		= true;
	AI_FORWARD			= false;
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

	move.moveDest		= pos;
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_TO_POSITION_DIRECT;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.startTime		= gameLocal.time;
	move.speed			= fly_speed;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;

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

	move.moveDest.z		= lastVisibleEnemyPos.z + enemyEnt->EyeOffset().z + fly_offset;
	move.goalEntity		= enemyEnt;
	move.moveCommand	= MOVE_TO_ENEMYHEIGHT;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.startTime		= gameLocal.time;
	move.speed			= 0.0f;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= false;

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

	//BC if player is notarget or noclip, then ignore.
	if (enemyEnt->IsType(idPlayer::Type))
	{
		if (static_cast<idPlayer *>(enemyEnt)->noclip || static_cast<idPlayer *>(enemyEnt)->fl.notarget)
		{
			StopMove(MOVE_STATUS_DEST_NOT_FOUND);
			return false;
		}
	}

	if ( !enemyEnt )
	{
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}


	if ( ReachedPos( lastVisibleReachableEnemyPos, MOVE_TO_ENEMY ) )
	{
		if ( !ReachedPos( lastVisibleEnemyPos, MOVE_TO_ENEMY ) || !AI_ENEMY_VISIBLE )
		{
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
			AI_DEST_UNREACHABLE = true;
			return false;
		}
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	idVec3 pos = lastVisibleReachableEnemyPos;

	move.toAreaNum = 0;
	if ( aas )
	{
		move.toAreaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( move.toAreaNum, pos );

		areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, pos ) )
		{
			//BC Knows WHERE player is, but cannot reach their position.
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( !move.toAreaNum )
	{
		// if only trying to update the enemy position
		if ( move.moveCommand == MOVE_TO_ENEMY )
		{
			if ( !aas )
			{
				// keep the move destination up to date for wandering
				move.moveDest = pos;
			}
			return false;
		}

		if ( !NewWanderDir( pos ) )
		{
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( move.moveCommand != MOVE_TO_ENEMY )
	{
		move.moveCommand	= MOVE_TO_ENEMY;
		move.startTime		= gameLocal.time;
	}

	move.moveDest		= pos;
	move.goalEntity		= enemyEnt;
	move.speed			= fly_speed;
	move.moveStatus		= MOVE_STATUS_MOVING;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;

	//gameRenderWorld->DebugArrow(colorMagenta, pos + idVec3(0, 0, 128), pos, 8, 10000);

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
	if ( ( move.moveType != MOVETYPE_FLY ) && ( ( move.moveCommand != MOVE_TO_ENTITY ) || ( move.goalEntityOrigin != pos ) ) )
	{
		ent->GetFloorPos( 64.0f, pos );
	}

	if ( ReachedPos( pos, MOVE_TO_ENTITY ) )
	{
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	move.toAreaNum = 0;
	if ( aas )
	{
		move.toAreaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( move.toAreaNum, pos );

		areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, pos ) ) {
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( !move.toAreaNum )
	{
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

	if ( ( move.moveCommand != MOVE_TO_ENTITY ) || ( move.goalEntity.GetEntity() != ent ) )
	{
		move.startTime		= gameLocal.time;
		move.goalEntity		= ent;
		move.moveCommand	= MOVE_TO_ENTITY;
	}

	move.moveDest			= pos;
	move.goalEntityOrigin	= ent->GetPhysics()->GetOrigin();
	move.moveStatus			= MOVE_STATUS_MOVING;
	move.speed				= fly_speed;
	AI_MOVE_DONE			= false;
	AI_DEST_UNREACHABLE		= false;
	AI_FORWARD				= true;

	return true;
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
	idBounds		bounds;
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

	if ( ReachedPos( goal.origin, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	move.moveDest		= goal.origin;
	move.toAreaNum		= goal.areaNum;
	move.goalEntity		= ent;
	move.moveCommand	= MOVE_OUT_OF_RANGE;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.range			= range;
	move.speed			= fly_speed;
	move.startTime		= gameLocal.time;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;

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
	idBounds		bounds;
	idVec3			pos;

	if ( !aas || !ent ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}


	if (ent->IsType(idPlayer::Type))
	{
		if (static_cast<idPlayer *>(ent)->noclip || static_cast<idPlayer *>(ent)->fl.notarget)
		{
			StopMove(MOVE_STATUS_DEST_NOT_FOUND);
			return false;
		}
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

	idAASFindAttackPosition findGoal( this, physicsObj.GetGravityAxis(), ent, pos, missileLaunchOffset[ attack_anim ] );
	if ( !aas->FindNearestGoal( goal, areaNum, org, pos, travelFlags, &obstacle, 1, findGoal ) ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	move.moveDest		= goal.origin;
	move.toAreaNum		= goal.areaNum;
	move.goalEntity		= ent;
	move.moveCommand	= MOVE_TO_ATTACK_POSITION;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.speed			= fly_speed;
	move.startTime		= gameLocal.time;
	move.anim			= attack_anim;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;

	return true;
}

/*
=====================
idAI::MoveToPosition
=====================
*/
bool idAI::MoveToPosition( const idVec3 &pos ) {
	idVec3		org;
	int			areaNum;
	aasPath_t	path;

	if ( ReachedPos( pos, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	org = pos;
	move.toAreaNum = 0;
	if ( aas )
	{
		move.toAreaNum = PointReachableAreaNum( org );
		aas->PushPointIntoAreaNum( move.toAreaNum, org );

		areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, org ) )
		{
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( !move.toAreaNum && !NewWanderDir( org ) )
	{
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	move.moveDest		= org;
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_TO_POSITION;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.startTime		= gameLocal.time;
	move.speed			= fly_speed;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;

	return true;
}

/*
=====================
idAI::MoveToCover
=====================
*/
bool idAI::MoveToCover( idEntity *entity, const idVec3 &hideFromPos ) {
	int				areaNum;
	aasObstacle_t	obstacle;
	aasGoal_t		hideGoal;
	idBounds		bounds;

	if ( !aas || !entity ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );

	// consider the entity the monster tries to hide from as an obstacle
	obstacle.absBounds = entity->GetPhysics()->GetAbsBounds();

	idAASFindCover findCover( hideFromPos );
	if ( !aas->FindNearestGoal( hideGoal, areaNum, org, hideFromPos, travelFlags, &obstacle, 1, findCover ) ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	if ( ReachedPos( hideGoal.origin, move.moveCommand ) )
	{
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	move.moveDest		= hideGoal.origin;
	move.toAreaNum		= hideGoal.areaNum;
	move.goalEntity		= entity;
	move.moveCommand	= MOVE_TO_COVER;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.startTime		= gameLocal.time;
	move.speed			= fly_speed;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;

	return true;
}

/*
=====================
idAI::SlideToPosition
=====================
*/
bool idAI::SlideToPosition( const idVec3 &pos, float time ) {
	StopMove( MOVE_STATUS_DONE );

	move.moveDest		= pos;
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_SLIDE_TO_POSITION;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.startTime		= gameLocal.time;
	move.duration		= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( time ) );
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= false;

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

	//Move forward.
	move.moveDest = physicsObj.GetOrigin() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 256.0f;

	if ( !NewWanderDir( move.moveDest ) )
	{
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	move.moveCommand	= MOVE_WANDER;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.startTime		= gameLocal.time;
	move.speed			= fly_speed;
	AI_MOVE_DONE		= false;
	AI_FORWARD			= true;

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
idAI::NewWanderDir
================
*/
bool idAI::NewWanderDir( const idVec3 &dest ) {
	float	deltax, deltay;
	float	d[ 3 ];
	float	tdir, olddir, turnaround;

	move.nextWanderTime = gameLocal.time + ( gameLocal.random.RandomFloat() * 500 + 500 );

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

		if ( tdir != turnaround && StepDirection( tdir ) )
		{
			return true; //Path is clear. Just go forward.
		}
	}

	// try other directions
	if ( ( gameLocal.random.RandomInt() & 1 ) || idMath::Fabs( deltay ) > idMath::Fabs( deltax ) ) {
		tdir = d[ 1 ];
		d[ 1 ] = d[ 2 ];
		d[ 2 ] = tdir;
	}

	//Hit wall at a glancing angle. Continue moving at same trajectory.
	if ( d[ 1 ] != DI_NODIR && d[ 1 ] != turnaround && StepDirection( d[1] ) ) {
		return true;
	}

	//Hit wall at a glancing angle. Continue moving at same trajectory.
	if ( d[ 2 ] != DI_NODIR && d[ 2 ] != turnaround	&& StepDirection( d[ 2 ] ) ) {
		return true;
	}

	// there is no direct path to the player, so pick another direction
	if ( olddir != DI_NODIR && StepDirection( olddir ) ) {
		return true;
	}

	//Hit wall at a head-on angle. Randomly choose a new angle to move toward.
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
bool idAI::GetMovePos( idVec3 &seekPos ) {
	int			areaNum;
	aasPath_t	path;
	bool		result;
	idVec3		org;

	org = physicsObj.GetOrigin();
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
		if ( ReachedPos( move.moveDest, move.moveCommand ) ) {
			StopMove( MOVE_STATUS_DONE );
		}
		return false;
		break;

	case MOVE_SLIDE_TO_POSITION :
		seekPos = org;
		return false;
		break;
	}

	if ( move.moveCommand == MOVE_TO_ENTITY ) {
		MoveToEntity( move.goalEntity.GetEntity() );
	}

	move.moveStatus = MOVE_STATUS_MOVING;
	result = false;
	if ( gameLocal.time > move.blockTime ) {
		if ( move.moveCommand == MOVE_WANDER ) {
			move.moveDest = org + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 256.0f;
		} else {
			if ( ReachedPos( move.moveDest, move.moveCommand ) ) {
				StopMove( MOVE_STATUS_DONE );
				seekPos	= org;
				return false;
			}
		}

		if ( aas && move.toAreaNum ) {
			areaNum	= PointReachableAreaNum( org );
			if ( PathToGoal( path, areaNum, org, move.toAreaNum, move.moveDest ) ) {
				seekPos = path.moveGoal;
				result = true;
				move.nextWanderTime = 0;
			} else {
				AI_DEST_UNREACHABLE = true;
			}
		}
	}

	if ( !result ) {
		// wander around
		if ( ( gameLocal.time > move.nextWanderTime ) || !StepDirection( move.wanderYaw ) ) {
			result = NewWanderDir( move.moveDest );
			if ( !result ) {
				StopMove( MOVE_STATUS_DEST_UNREACHABLE );
				AI_DEST_UNREACHABLE = true;
				seekPos	= org;
				return false;
			}
		} else {
			result = true;
		}

		seekPos = org + move.moveDir * 2048.0f;
		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugLine( colorYellow, org, seekPos, gameLocal.msec, true );
		}
	} else {
		AI_DEST_UNREACHABLE = false;
	}

	if ( result && ( ai_debugMove.GetBool() ) ) {
		gameRenderWorld->DebugLine( colorCyan, physicsObj.GetOrigin(), seekPos );
	}

	return result;
}

/*
=====================
idAI::EntityCanSeePos
=====================
*/
bool idAI::EntityCanSeePos( idActor *actor, const idVec3 &actorOrigin, const idVec3 &pos ) {
	idVec3 eye, point;
	trace_t results;
	pvsHandle_t handle;

	handle = gameLocal.pvs.SetupCurrentPVS( actor->GetPVSAreas(), actor->GetNumPVSAreas() );

	if ( !gameLocal.pvs.InCurrentPVS( handle, GetPVSAreas(), GetNumPVSAreas() ) ) {
		gameLocal.pvs.FreeCurrentPVS( handle );
		return false;
	}

	gameLocal.pvs.FreeCurrentPVS( handle );

	eye = actorOrigin + actor->EyeOffset();

	point = pos;
	point[2] += 1.0f;

	physicsObj.DisableClip();

	gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, actor );
	if ( results.fraction >= 1.0f || ( gameLocal.GetTraceEntity( results ) == this ) ) {
		physicsObj.EnableClip();
		return true;
	}

	const idBounds &bounds = physicsObj.GetBounds();
	point[2] += bounds[1][2] - bounds[0][2];

	gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, actor );
	physicsObj.EnableClip();
	if ( results.fraction >= 1.0f || ( gameLocal.GetTraceEntity( results ) == this ) ) {
		return true;
	}
	return false;
}

/*
=====================
idAI::BlockedFailSafe
=====================
*/
void idAI::BlockedFailSafe( void )
{
	if ( !ai_blockedFailSafe.GetBool() || blockedRadius < 0.0f )
	{
		return;
	}
	
	//BC changed this check.
	//if ( !physicsObj.OnGround() || enemy.GetEntity() == NULL ||	( physicsObj.GetOrigin() - move.lastMoveOrigin ).LengthSqr() > Square( blockedRadius ) )
	if ( (physicsObj.GetOrigin() - move.lastMoveOrigin).LengthSqr() > Square(blockedRadius))
	{
		move.lastMoveOrigin = physicsObj.GetOrigin();
		move.lastMoveTime = gameLocal.time;
	}	

	if ( move.lastMoveTime < gameLocal.time - blockedMoveTime )
	{
		if ( lastAttackTime < gameLocal.time - blockedAttackTime )
		{
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
void idAI::Turn( void ) {
	float diff;
	float diff2;
	float turnAmount;
	animFlags_t animflags;

	if ( !turnRate ) {
		return;
	}

	// check if the animator has marker this anim as non-turning
	if ( !legsAnim.Disabled() && !legsAnim.AnimDone( 0 ) ) {
		animflags = legsAnim.GetAnimFlags();
	} else {
		animflags = torsoAnim.GetAnimFlags();
	}
	if ( animflags.ai_no_turn ) {
		return;
	}

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
		turnVel += AI_TURN_SCALE * diff * MS2SEC( gameLocal.msec );
		if ( turnVel > turnRate ) {
			turnVel = turnRate;
		} else if ( turnVel < -turnRate ) {
			turnVel = -turnRate;
		}
		turnAmount = turnVel * MS2SEC( gameLocal.msec );
		if ( ( diff >= 0.0f ) && ( turnAmount >= diff ) ) {
			turnVel = diff / MS2SEC( gameLocal.msec );
			turnAmount = diff;
		} else if ( ( diff <= 0.0f ) && ( turnAmount <= diff ) ) {
			turnVel = diff / MS2SEC( gameLocal.msec );
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

	if ( ai_debugMove.GetBool() ) {
		const idVec3 &org = physicsObj.GetOrigin();
		gameRenderWorld->DebugLine( colorRed, org, org + idAngles( 0, ideal_yaw, 0 ).ToForward() * 64, gameLocal.msec );
		gameRenderWorld->DebugLine( colorGreen, org, org + idAngles( 0, current_yaw, 0 ).ToForward() * 48, gameLocal.msec );
		gameRenderWorld->DebugLine( colorYellow, org, org + idAngles( 0, current_yaw + turnVel, 0 ).ToForward() * 32, gameLocal.msec );
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

	dir = pos - physicsObj.GetOrigin();
	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	lengthSqr = local_dir.LengthSqr();
	if ( lengthSqr > Square( 2.0f ) || ( lengthSqr > Square( 0.1f ) && enemy.GetEntity() == NULL ) ) {
		ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() );
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
void idAI::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {
	// FIXME: Jim take a look at this and see if this is a reasonable thing to do
	// instead of a spawnArg flag.. Sabaoth is the only slide monster ( and should be the only one for D3 )
	// and we don't want him taking physics impulses as it can knock him off the path

	if ( move.moveType != MOVETYPE_STATIC && move.moveType != MOVETYPE_SLIDE ) {
		idActor::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
=====================
idAI::GetMoveDelta
=====================
*/
void idAI::GetMoveDelta( const idMat3 &oldaxis, const idMat3 &axis, idVec3 &delta ) {
	idVec3 oldModelOrigin;
	idVec3 modelOrigin;

	animator.GetDelta( gameLocal.time - gameLocal.msec, gameLocal.time, delta );
	delta = axis * delta;

	if ( modelOffset != vec3_zero ) {
		// the pivot of the monster's model is around its origin, and not around the bounding
		// box's origin, so we have to compensate for this when the model is offset so that
		// the monster still appears to rotate around it's origin.
		oldModelOrigin = modelOffset * oldaxis;
		modelOrigin = modelOffset * axis;
		delta += oldModelOrigin - modelOrigin;
	}

	delta *= physicsObj.GetGravityAxis();
}

/*
=====================
idAI::CheckObstacleAvoidance
=====================
*/
void idAI::CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &newPos ) {
	idEntity		*obstacle;
	obstaclePath_t	path;
	idVec3			dir;
	float			dist;
	bool			foundPath;

	if ( ignore_obstacles ) {
		newPos = goalPos;
		move.obstacle = NULL;
		return;
	}

	const idVec3 &origin = physicsObj.GetOrigin();

	obstacle = NULL;
	AI_OBSTACLE_IN_PATH = false;
	foundPath = FindPathAroundObstacles(this, &physicsObj, aas, enemy.GetEntity(), origin, goalPos, path );
	if ( ai_showObstacleAvoidance.GetBool() ) {
		gameRenderWorld->DebugLine( colorBlue, goalPos + idVec3( 1.0f, 1.0f, 0.0f ), goalPos + idVec3( 1.0f, 1.0f, 64.0f ), gameLocal.msec );
		gameRenderWorld->DebugLine( foundPath ? colorYellow : colorRed, path.seekPos, path.seekPos + idVec3( 0.0f, 0.0f, 64.0f ), gameLocal.msec );
	}

	if ( !foundPath ) {
		// couldn't get around obstacles
		if ( path.firstObstacle ) {
			AI_OBSTACLE_IN_PATH = true;
			if ( physicsObj.GetAbsBounds().Expand( 2.0f ).IntersectsBounds( path.firstObstacle->GetPhysics()->GetAbsBounds() ) ) {
				obstacle = path.firstObstacle;
			}
		} else if ( path.startPosObstacle ) {
			AI_OBSTACLE_IN_PATH = true;
			if ( physicsObj.GetAbsBounds().Expand( 2.0f ).IntersectsBounds( path.startPosObstacle->GetPhysics()->GetAbsBounds() ) ) {
				obstacle = path.startPosObstacle;
			}
		} else {
			// Blocked by wall
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
	} else if ( path.seekPosObstacle ) {
		// if the AI is very close to the path.seekPos already and path.seekPosObstacle != NULL
		// then we want to push the path.seekPosObstacle entity out of the way
		AI_OBSTACLE_IN_PATH = true;

		// check if we're past where the goalPos was pushed out of the obstacle
		dir = goalPos - origin;
		dir.Normalize();
		dist = ( path.seekPos - origin ) * dir;
		if ( dist < 1.0f ) {
			obstacle = path.seekPosObstacle;
		}
	}

	// if we had an obstacle, set our move status based on the type, and kick it out of the way if it's a moveable
	if ( obstacle ) {
		if ( obstacle->IsType( idActor::Type ) ) {
			// monsters aren't kickable
			if ( obstacle == enemy.GetEntity() ) {
				move.moveStatus = MOVE_STATUS_BLOCKED_BY_ENEMY;
			} else {
				move.moveStatus = MOVE_STATUS_BLOCKED_BY_MONSTER;
			}
		} else {
			// try kicking the object out of the way
			move.moveStatus = MOVE_STATUS_BLOCKED_BY_OBJECT;
		}
		newPos = obstacle->GetPhysics()->GetOrigin();
		//newPos = path.seekPos;
		move.obstacle = obstacle;
	} else {
		newPos = path.seekPos;
		move.obstacle = NULL;
	}
}

/*
=====================
idAI::DeadMove
=====================
*/
void idAI::DeadMove( void ) {
	idVec3				delta;

	GetMoveDelta( viewAxis, viewAxis, delta );
	physicsObj.SetDelta( delta );

	RunPhysics();

	AI_ONGROUND = physicsObj.OnGround();
}

/*
=====================
idAI::AnimMove
=====================
*/
void idAI::AnimMove( void ) {
	idVec3				goalPos;
	idVec3				delta;
	idVec3				goalDelta;
	float				goalDist;
	idVec3				newDest;

	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	AI_BLOCKED = false;

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS )
	{
		move.lastMoveOrigin.Zero();
		move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;

	if ( ( move.moveCommand == MOVE_FACE_ENEMY ) && enemy.GetEntity() )
	{
		TurnToward( lastVisibleEnemyPos );
		goalPos = oldorigin;
	}
	else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() )
	{
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		goalPos = oldorigin;
	}
	else if ( GetMovePos( goalPos ) )
	{
		if (move.moveCommand == MOVE_TO_ATTACK_POSITION)
		{
			//BC turn toward enemy.
			CheckObstacleAvoidance(goalPos, newDest);
			TurnToward(lastVisibleEnemyPos);
			//common->Printf("move to attack move\n");
		}
		else if ( move.moveCommand != MOVE_WANDER )
		{
			CheckObstacleAvoidance( goalPos, newDest );
			TurnToward( newDest );
		}
		else
		{
			TurnToward( goalPos );
		}
	}

	Turn();

	if ( move.moveCommand == MOVE_SLIDE_TO_POSITION )
	{
		if ( gameLocal.time < move.startTime + move.duration )
		{
			goalPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
			delta = goalPos - oldorigin;
			delta.z = 0.0f;
		}
		else
		{
			delta = move.moveDest - oldorigin;
			delta.z = 0.0f;
			StopMove( MOVE_STATUS_DONE );
		}
	}
	else if ( allowMove )
	{
		GetMoveDelta( oldaxis, viewAxis, delta );
	}
	else
	{
		delta.Zero();
	}

	if ( move.moveCommand == MOVE_TO_POSITION ) {
		goalDelta = move.moveDest - oldorigin;
		goalDist = goalDelta.LengthFast();
		if ( goalDist < delta.LengthFast() ) {
			delta = goalDelta;
		}
	}

#ifdef _D3XP
	physicsObj.UseFlyMove( false );
#endif
	physicsObj.SetDelta( delta );
	physicsObj.ForceDeltaMove( disableGravity );

	RunPhysics();

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}

	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
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
	//if ( oldorigin != org )
    if (oldorigin != org || gameLocal.time > triggerTimer) //Bc put this on timer, so it's not every frame.
	{
		TouchTriggers();
        triggerTimer = gameLocal.time + 100;
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
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
	seekVel = goalDelta * MS2SEC( gameLocal.msec );

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
	idVec3				newDest;

	idVec3 oldorigin = physicsObj.GetOrigin();

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
	vel -= vel * AI_FLY_DAMPENING * MS2SEC( gameLocal.msec );
	vel += goalDelta * MS2SEC( gameLocal.msec );

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

	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
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
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
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
	float	speed;
	float	roll;
	float	pitch;

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
		vel += fly_bob_add * MS2SEC( gameLocal.msec );
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
	if ( origin.z > goalPos.z ) {
		dest = goalPos;
		dest.z = origin.z + 128.0f;
		idAI::PredictPath( this, aas, goalPos, dest - origin, 1000, 1000, SE_BLOCKED, path );
		if ( path.endPos.z < origin.z ) {
			idVec3 addVel = Seek( vel, origin, path.endPos, AI_SEEK_PREDICTION );
			vel.z += addVel.z;
			goLower = true;
		}

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugBounds( goLower ? colorRed : colorGreen, physicsObj.GetBounds(), path.endPos, gameLocal.msec );
		}
	}

	if ( !goLower ) {
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
		vel += Seek( vel, origin, trace.endpos, AI_SEEK_PREDICTION );
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
	vel -= vel * AI_FLY_DAMPENING * MS2SEC( gameLocal.msec );

	// gradually speed up/slow down to desired speed
	speed = vel.Normalize();
	speed += ( move.speed - speed ) * MS2SEC( gameLocal.msec );
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

		if (allowMove) //BC during !allowMove, turn off idle wiggling so that monster is forced to turn to specific yaw.
		{
			const idVec3 &vel = physicsObj.GetLinearVelocity();
			if (vel.ToVec2().LengthSqr() > 0.1f)
			{
				TurnToward(vel.ToYaw());
			}
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
	idVec3	newDest;


	AI_BLOCKED = false;
	if ( ( move.moveCommand != MOVE_NONE ) && ReachedPos( move.moveDest, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
	}

	if ( ai_debugMove.GetBool() ) {

		//BC this is what prints in the console.

		if (team == 1 || (ai_debugMove.GetInteger() > 1)) //Only print if on enemy team, OR ai_debugmove == 2
		{
			gameLocal.Printf("%d: %s: %s, vel=%.2f, sp=%.2f, maxsp=%.2f\n", gameLocal.time, name.c_str(), moveCommandString[move.moveCommand], physicsObj.GetLinearVelocity().Length(), move.speed, fly_speed);
		}
	}

	if ( move.moveCommand != MOVE_TO_POSITION_DIRECT)
	{
		idVec3 vel = physicsObj.GetLinearVelocity();

		if ( GetMovePos( goalPos ) ) {
			//BC figure out a better way to do obstacle avoidance. Commenting these out for now....
			//CheckObstacleAvoidance( goalPos, newDest );
			//goalPos = newDest;
		}

		if ( move.speed	) {
			FlySeekGoal( vel, goalPos );
		}

		// add in bobbing
		AddFlyBob( vel );

		if ( enemy.GetEntity() && ( move.moveCommand != MOVE_TO_POSITION ) ) {
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
	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
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
		gameRenderWorld->DebugBounds( colorOrange, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorRed, org, org + physicsObj.GetLinearVelocity(), gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorBlue, org, goalPos, gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
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

	if ( AI_DEAD ) {
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

	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemyEnt );
	}

	if ( ai_debugMove.GetBool() ) {
		const idVec3 &org = physicsObj.GetOrigin();
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugLine( colorBlue, org, move.moveDest, gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
	}
}

/***********************************************************************

	Damage

***********************************************************************/

/*
=====================
idAI::ReactionTo
=====================
*/
int idAI::ReactionTo(idEntity *ent ) {

	if ( ent->fl.hidden ) {
		// ignore hidden entities
		return ATTACK_IGNORE;
	}

	
	//BC Turrets.
	if (ent->IsType(idTurret::Type))
	{
		if (static_cast<idTurret *>(ent)->IsOn())
		{
			return ATTACK_ON_SIGHT;
		}

		return ATTACK_IGNORE;
	}


	if ( !ent->IsType( idActor::Type ) ) {
		return ATTACK_IGNORE;
	}

	const idActor *actor = static_cast<const idActor *>( ent );

	//bc ignore enemy if they are in confined stealth mode.
	if ( actor->IsType( idPlayer::Type ))
	{
		int playerLuminanceState = gameLocal.GetLocalPlayer()->GetHiddenStatus();

		//if (static_cast<const idPlayer *>(actor)->noclip || (static_cast<const idPlayer *>(actor)->inConfinedState && static_cast<const idPlayer *>(actor)->confinedStealthActive &&  playerLuminanceState != LIGHTMETER_NONE))
		if (static_cast<const idPlayer *>(actor)->noclip || (static_cast<const idPlayer *>(actor)->confinedStealthActive &&  playerLuminanceState != LIGHTMETER_NONE))
		{
			// ignore players in noclip or in confined stealth mode.

			//Check if the AI is looking at the actor through an open vent door. We want to allow ai to see the player through open ventdoors.
			//if (!ActorIsNearOpenVentdoor(actor))
			//{
			//	//actor is NOT near an open ventdoor. Therefore, ignore the actor.
			//	return ATTACK_IGNORE;
			//}

			#define UNSEEN_VIEWDISTANCE 80 //at this distance, I can "see" into darkness.
			#define UNSEEN_VIEWDISTANCE_HEAVY 200 // SW 16th April 2025: heavies can see a little further in the dark to compensate for their lack of flashlights
			bool hasFlashlight = spawnArgs.GetBool("flashlight_enabled", "1");
			float distanceToTarget = (GetEyePosition() - actor->GetEyePosition()).Length();
			//common->Printf("%f\n", distanceToTarget);
			if (distanceToTarget > (hasFlashlight ? UNSEEN_VIEWDISTANCE : UNSEEN_VIEWDISTANCE_HEAVY))
			{
				return ATTACK_IGNORE;
			}
		}
	}

	// actors on different teams will always fight each other
	if ( actor->team != team )
	{
		if (team == TEAM_NEUTRAL)
			return ATTACK_IGNORE; // SW: neutral NPCs will never attack

		if (actor->rank > rank) //BC if team rank is higher, then ignore.
			return ATTACK_IGNORE;

		if ( actor->fl.notarget ) {
			// don't attack on sight when attacker is notargeted
			return ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE;
		}
		return ATTACK_ON_SIGHT | ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE;
	}

	// monsters will fight when attacked by lower ranked monsters.  rank 0 never fights back.
	if ( rank && ( actor->rank < rank ) ) {
		return ATTACK_ON_DAMAGE;
	}

	// don't fight back
	return ATTACK_IGNORE;
}

bool idAI::ActorIsNearOpenVentdoor(const idActor *actor)
{
    #define TARGET_VENTDOORPROXIMITY 96 //Notice when target is this close to the ventdoor.

	for (idEntity* entity = gameLocal.ventdoorEntities.Next(); entity != NULL; entity = entity->ventdoorNode.Next())
	{
		float dist_DoorToActor;

		if (!entity)
			continue;

		if (!entity->IsType(idVentdoor::Type))
			continue;

		if (!static_cast<idVentdoor *>(entity)->IsOpen()) //Skip doors that are closed. We only care about ventdoors that are open.
			continue;

		dist_DoorToActor = (actor->GetPhysics()->GetOrigin() - entity->GetPhysics()->GetOrigin()).LengthFast();

		if (dist_DoorToActor > TARGET_VENTDOORPROXIMITY)
			continue;

		//make a check to verify this door is located between the actor and the ai.
        //This can definitely be optimized, as this is a pretty brute force way of implementing this.
        float dist_AIandActor = (actor->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
        float dist_DoorToAI = (entity->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();

        if (dist_DoorToActor > dist_AIandActor || dist_DoorToAI > dist_AIandActor) //if the distance between door and ai/target is MORE than the distance between ai & target, then it's invalid.
            continue;

		return true;
	}

	return false;
}


/*
=====================
idAI::Pain
=====================
*/
bool idAI::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, bool playPainSound, const char *damageDef)
{
	idActor	*actor;

	AI_PAIN = idActor::Pain( inflictor, attacker, damage, dir, location, playPainSound, damageDef );
	AI_DAMAGE = true;

	// force a blink
	blink_time = 0;



	// ignore damage from self
	if ( attacker != this ) {
		if ( inflictor ) {
			AI_SPECIAL_DAMAGE = inflictor->spawnArgs.GetInt( "special_damage" );
		} else {
			AI_SPECIAL_DAMAGE = 0;
		}

		if ( enemy.GetEntity() != attacker && attacker->IsType( idActor::Type ) )
		{
			actor = ( idActor * )attacker;
			if ( ReactionTo( actor ) & ATTACK_ON_DAMAGE )
			{
				//When I get hurt, make the attacker my enemy.
				//gameLocal.AlertAI( actor ); //When I get hurt, alert my nearby buddies and get them mad.
				SetEnemy( actor );
			}
		}
	}

	//damage flash.
	//currentskin = this->renderEntity.customSkin;
	//SetSkin(damageFlashSkin);
	//damageflashTimer = gameLocal.time + DAMAGEFLASHTIME;
	//damageFlashActive = true;

	lastDamageOrigin = inflictor->GetPhysics()->GetOrigin();
	lastAttackerDamageOrigin = attacker->GetPhysics()->GetOrigin();


	lastAttacker = attacker;
	lastDamagedTime = gameLocal.time;



	//special damage states.
	idStr damageGroup = GetDamageGroup(location);
	const idDict *damageDefinition = gameLocal.FindEntityDefDict(damageDef);
	if (damageDefinition)
	{
		const char *forceLocation = damageDefinition->GetString("forcelocation"); //damage type that forces itself to be a specific damage group. This is used for the death from above jump.
		if (forceLocation[0] != '\0')
		{
			damageGroup = forceLocation;
		}

		//if (damageDefinition->GetBool("stundamage"))
		//{
		//	//Check if this damage def only applies to a specific zone.
		//	const char *stun_zone = damageDefinition->GetString("stun_zone");
		//	if (stun_zone[0] == '\0')
		//	{
		//		//No specific zone.
		//		//do the special Stun Damage.
		//		StartStunState(damageDef);
		//	}
		//	else
		//	{
		//		if (idStr::Icmp(damageGroup, stun_zone) >= 0)
		//		{
		//			//damagedef stun_zone matches the hit position on me. Helmet pop off absorbs the damage. Only do stun if helmet has been popped off.
		//			if (idStr::Icmp(damageGroup, "head") >= 0 && hasHelmet)
		//			{
		//				//if I have a helmet and it hit me on head, then ignore....
		//			}
		//			else
		//			{
		//				//AI has had an item clonk them on the head.
		//
		//				//We want the AI to take stun damage ONLY IF: the ai is not in combat/search state, or if thrown object is behind/sideways to AI.
		//				if (IsStundamageValid(dir))
		//				{
		//					//do the special Stun Damage.
		//					StartStunState(damageDef);
		//				}
		//			}
		//
		//
		//			//BC changed how this works; previously, the stun was ignored if actor is wearing a helmet. Now: the stun happens regardless if a helmet is attached or not.
		//			//StartStunState(damageDef);
		//		}
		//	}			
		//}

		//allow damage definition to suppress pain sound.
		if (!damageDefinition->GetBool("playpainsound", "1"))
		{
			playPainSound = false;
		}

		// SW: actor def can also suppress pain sound
		if (!this->spawnArgs.GetBool("playpainsound", "1"))
		{
			playPainSound = false;
		}
	}


	//Handle helmet logic.	
	if (idStr::Icmp(damageGroup, "head") >= 0)
	{
		//damage received on head. Do helmet check.
        //PopoffHelmet(true);
		PopoffHelmet(false); //we used to do slow mo when the helmet popped off, but not anymore.
	}

	if (playPainSound)
	{
		gameLocal.voManager.SayVO(this, "snd_vo_pain", VO_CATEGORY_HITREACTION);


		if (this->aiState == AISTATE_JOCKEYED)
		{
			StartSound("snd_jockeystruggle", SND_CHANNEL_VOICE); //start choke noise.
		}
	}

	return ( AI_PAIN != 0 );
}



void idAI::PopoffHelmet(bool doSlowmo)
{
    if (!hasHelmet)
        return;
    
    //pop off the helmet.
    if (helmetModel != NULL)
    {
        idVec3 helmetPos = helmetModel->GetPhysics()->GetOrigin();
        idEntityFx::StartFx(spawnArgs.GetString("fx_helmetpop"), &helmetPos, &mat3_identity, NULL, false);

        //Spawn a helmet pickup.
        idEntity *helmetEnt;
        idDict args;
        args.Set("classname", spawnArgs.GetString("def_helmet"));
        args.SetVector("origin", helmetPos);
        gameLocal.SpawnEntityDef(args, &helmetEnt);
        if (helmetEnt)
        {
			helmetPtr = helmetEnt;
            helmetEnt->GetPhysics()->SetAngularVelocity(idVec3(128, 0, 0)); //spin the helmet.
            idAngles helmetThrowDir = idAngles(-80, gameLocal.random.RandomInt(359), 0);
            helmetEnt->GetPhysics()->SetLinearVelocity(helmetThrowDir.ToForward() * 128); //throw the helmet.					
            helmetEnt->GetPhysics()->GetClipModel()->SetOwner(this); //so helmet doesnt collide with me.

            if (doSlowmo)
            {
                //do slow mo if player has LOS to the helmet AND helmet is in FRONT of player.
                idVec3 dirToHelmet = helmetEnt->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
                float facingResult = DotProduct(dirToHelmet, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
                if (facingResult > 0)
                {
                    trace_t helmetTr;
                    gameLocal.clip.TracePoint(helmetTr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, helmetEnt->GetPhysics()->GetOrigin(), MASK_SOLID, NULL);
                    if (helmetTr.fraction >= 1)
                    {
                        //has LOS to the helmet. Do the slowmo.
                        gameLocal.GetLocalPlayer()->SetImpactSlowmo(true);
                    }
                }
            }
        }
    }

    SetHelmet(false);    
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
	}
	else
	{
		idAngles emitAngle;		

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

		//BC TODO: FIX THIS. MAKE PARTICLES EMIT AT SAME DIRECTION AS BONE.
		//emitAngle = axis.ToAngles();
		//emitAngle.pitch += 90;
		//axis = emitAngle.ToMat3();

		gameLocal.smokeParticles->EmitSmoke( pe.particle, pe.time, gameLocal.random.CRandomFloat(), origin, axis, timeGroup /*_D3XP*/ );
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

	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false); //To turn off the suspicion noise.

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
	af_push_moveables = false;

	physicsObj.UseFlyMove( false );
	physicsObj.ForceDeltaMove( false );

	// end our looping ambient sound
	StopSound( SND_CHANNEL_AMBIENT, false );

	//BC do not alert nearby enemies when I die.
	/*
	if ( attacker && attacker->IsType( idActor::Type ) )
	{
		gameLocal.AlertAI( ( idActor * )attacker );
	}*/

	// activate targets
	ActivateTargets( attacker );

	RemoveAttachments();
	RemoveProjectile();
	StopMove( MOVE_STATUS_DONE );

	ClearEnemy();
	AI_DEAD	= true;

	// make monster nonsolid
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();

	Unbind();

	if ( StartRagdoll() )
	{
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
	}
	else
	{
		idMoveableItem::DropItems(this, "death", NULL); //BC if there is no ragdoll, then still let me drop items!!!!!!!!
	}

	if ( spawnArgs.GetString( "model_death", "", &modelDeath ) ) {
		// lost soul is only case that does not use a ragdoll and has a model_death so get the death sound in here
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		SetModel( modelDeath );
		physicsObj.SetLinearVelocity( vec3_zero );
		physicsObj.PutToRest();
		physicsObj.DisableImpact();
		// No grabbing if "model_death"
		noGrab = true;
	}

	restartParticles = false;

	state = GetScriptFunction( "State_Killed" );
	SetState( state );
	SetWaitState( "" );


	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_drops", NULL );
	while( kv ) {
		idDict args;

		args.Set( "classname", kv->GetValue() );
		args.Set( "origin", physicsObj.GetOrigin().ToString() );
		gameLocal.SpawnEntityDef( args );
		kv = spawnArgs.MatchPrefix( "def_drops", kv );
	}


	if ( ( attacker && attacker->IsType( idPlayer::Type ) ) && ( inflictor && !inflictor->IsType( idSoulCubeMissile::Type ) ) ) {
		static_cast< idPlayer* >( attacker )->AddAIKill();
	}


/*
	if(spawnArgs.GetBool("harvest_on_death")) {
		const idDict *harvestDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_harvest_type"), false );
		if ( harvestDef ) {
			idEntity *temp;
			gameLocal.SpawnEntityDef( *harvestDef, &temp, false );
			harvestEnt = static_cast<idHarvestable *>(temp);

		}

		if(harvestEnt.GetEntity()) {
			//Let the harvest entity set itself up
			harvestEnt.GetEntity()->Init(this);
			harvestEnt.GetEntity()->BecomeActive( TH_THINK );
		}
	}
*/


	//BC do special audio if headshot death.
	if (!strcmp(animator.GetJointName((jointHandle_t)location), "head"))
	{
		StartSound("snd_ui_headshot", SND_CHANNEL_DEMONIC, 0, false, NULL);		
	}

	if (spawnArgs.GetBool("has_laser", "0"))
	{
		lasersightbeam->Hide();
		lasersightbeamTarget->Hide();
		laserdot->Hide();
	}

	SetDragFrobbable(true); //make the dragpoints frobbable.
	
	

	//spawn the bleed out UI model.
	if (!gibbed && spawnArgs.GetInt("bleedouttime", "60") > 0)
	{
		idDict args;
		args.Clear();
		args.SetVector("origin", GetPhysics()->GetOrigin());
		args.Set("model", spawnArgs.GetString("model_healthtube"));		
		args.Set("start_anim", "spin");
		bleedoutTube = gameLocal.SpawnEntityType(idAnimated::Type, &args);
		bleedoutTube.GetEntity()->GetRenderEntity()->gui[0] = uiManager->FindGui("guis/game/bleedouttube.gui", true, true); //Create a UNIQUE gui so that its number doesn't auto sync with other guis.

		bleedoutTime = gameLocal.time  + (spawnArgs.GetInt("bleedouttime", "60") * 1000);
		bleedoutState = BLEEDOUT_ACTIVE;
	}


	if (team == TEAM_ENEMY)
	{
		gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), "interest_corpse");
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->OnEnemyDestroyed(this); //Keep track of AI deaths so the climactic killcam knows when to activate.
	}

	//If die in zero g, then make body float.
	if (gameLocal.GetAirlessAtPoint(GetPhysics()->GetAbsBounds().GetCenter()))
	{
		this->GetPhysics()->SetGravity(vec3_zero);
	}

    PopoffHelmet(false);

	if (1)
	{
		gameLocal.AddEventlogDeath(this, damage, inflictor, attacker, "", EL_DEATH);
	}
}

//todo: should we just remove this completely??
void idAI::SpawnDragButton(int index, const char * jointName, const char * displayString)
{
	idDict args;
	idVec3 jointPos;
	idMat3 jointAxis;
	jointHandle_t jointHandle;

	jointHandle = animator.GetJointHandle(jointName);
	if (jointHandle == INVALID_JOINT)
	{
		gameLocal.Error("SpawnDragButton failed to find joint '%s' on '%s'", jointName, this->GetName());
	}

	GetJointWorldTransform(jointHandle, gameLocal.time, jointPos, jointAxis);

	//Spawn frobcube.
	args.Clear();
	args.Set("classname", "func_frobcube");
	args.Set("model", "models/objects/frobcube/cube8x8.ase");
	args.Set("displayname", displayString);
	if (gameLocal.SpawnEntityDef(args, &dragButtons[index]))
	{
		dragButtons[index]->BecomeActive(TH_PHYSICS);
		dragButtons[index]->SetOrigin(jointPos);
		dragButtons[index]->GetPhysics()->GetClipModel()->SetOwner(this);
		static_cast<idFrobcube*>(dragButtons[index])->SetIndex(index);
		dragButtons[index]->BindToJoint(this, jointHandle, true);
		dragButtons[index]->isFrobbable = false;
		dragButtons[index]->Hide();
	}	
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
=====================
*/
void idAI::Activate( idEntity *activator ) {
	idPlayer *player;

	if ( AI_DEAD ) {
		// ignore it when they're dead
		return;
	}

	// make sure he's not dormant
	dormantStart = 0;

	if ( num_cinematics ) {
		PlayCinematic();
	} else {
		AI_ACTIVATED = true;
		if ( !activator || !activator->IsType( idPlayer::Type ) ) {
			player = gameLocal.GetLocalPlayer();
		} else {
			player = static_cast<idPlayer *>( activator );
		}

		if ( ReactionTo( player ) & ATTACK_ON_ACTIVATE ) {
			SetEnemy( player );
		}

		// update the script in cinematics so that entities don't start anims or show themselves a frame late.
		if ( cinematic ) {
			UpdateAIScript();

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
	AI_ENEMY_DEAD = true;
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

#ifdef _D3XP
	// Wake up monsters that are pretending to be NPC's
	if ( team == 1 && actor->team != team ) {
		ProcessEvent( &EV_Activate, actor );
	}
#endif

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

	enemyNode.Remove();
	enemy				= NULL;
	AI_ENEMY_IN_FOV		= false;
	AI_ENEMY_VISIBLE	= false;
	AI_ENEMY_DEAD		= true;

	SetChatSound();
}

/*
=====================
idAI::EnemyPositionValid
=====================
*/
bool idAI::EnemyPositionValid( void ) const {
	trace_t	tr;
	idVec3	muzzle;
	idMat3	axis;

	if ( !enemy.GetEntity() ) {
		return false;
	}

	if ( AI_ENEMY_VISIBLE ) {
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
void idAI::SetEnemyPosition( void ) {
	idActor		*enemyEnt = enemy.GetEntity();
	int			enemyAreaNum;
	int			areaNum;
	int			lastVisibleReachableEnemyAreaNum = 0;
	aasPath_t	path;
	idVec3		pos;
	bool		onGround;

	if ( !enemyEnt ) {
		return;
	}

	lastVisibleReachableEnemyPos = lastReachableEnemyPos;
	lastVisibleEnemyEyeOffset = enemyEnt->EyeOffset();
	//lastVisibleEnemyPos = enemyEnt->GetPhysics()->GetOrigin(); //BC why is this here?????
	

	//BC keep track of the last time I saw enemy.
	if (!canSeeEnemy)
	{
		lastVisibleEnemyTime = gameLocal.time;
		canSeeEnemy = true;
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		pos = lastVisibleEnemyPos;
		onGround = true;
	} else {
		onGround = enemyEnt->GetFloorPos( 64.0f, pos );
		if ( enemyEnt->OnLadder() ) {
			onGround = false;
		}
	}

	if ( !onGround ) {
		if ( move.moveCommand == MOVE_TO_ENEMY ) {
			AI_DEST_UNREACHABLE = true;
		}
		return;
	}

	// when we don't have an AAS, we can't tell if an enemy is reachable or not,
	// so just assume that he is.
	if ( !aas ) {
		lastVisibleReachableEnemyPos = lastVisibleEnemyPos;
		if ( move.moveCommand == MOVE_TO_ENEMY ) {
			AI_DEST_UNREACHABLE = false;
		}
		enemyAreaNum = 0;
		areaNum = 0;
	} else {
		lastVisibleReachableEnemyAreaNum = move.toAreaNum;
		enemyAreaNum = PointReachableAreaNum( lastVisibleEnemyPos, 1.0f );
		if ( !enemyAreaNum ) {
			enemyAreaNum = PointReachableAreaNum( lastReachableEnemyPos, 1.0f );
			pos = lastReachableEnemyPos;
		}
		if ( !enemyAreaNum ) {
			if ( move.moveCommand == MOVE_TO_ENEMY ) {
				AI_DEST_UNREACHABLE = true;
			}
			areaNum = 0;
		} else {
			const idVec3 &org = physicsObj.GetOrigin();
			areaNum = PointReachableAreaNum( org );
			if ( PathToGoal( path, areaNum, org, enemyAreaNum, pos ) ) {
				lastVisibleReachableEnemyPos = pos;
				lastVisibleReachableEnemyAreaNum = enemyAreaNum;
				if ( move.moveCommand == MOVE_TO_ENEMY ) {
					AI_DEST_UNREACHABLE = false;
				}
			} else if ( move.moveCommand == MOVE_TO_ENEMY ) {
				AI_DEST_UNREACHABLE = true;
			}
		}
	}

	if ( move.moveCommand == MOVE_TO_ENEMY ) {
		if ( !aas ) {
			// keep the move destination up to date for wandering
			move.moveDest = lastVisibleReachableEnemyPos;
		} else if ( enemyAreaNum ) {
			move.toAreaNum = lastVisibleReachableEnemyAreaNum;
			move.moveDest = lastVisibleReachableEnemyPos;
		}

		if ( move.moveType == MOVETYPE_FLY ) {
			predictedPath_t path;
			idVec3 end = move.moveDest;
			end.z += enemyEnt->EyeOffset().z + fly_offset;
			idAI::PredictPath( this, aas, move.moveDest, end - move.moveDest, 1000, 1000, SE_BLOCKED, path );
			move.moveDest = path.endPos;
			move.toAreaNum = PointReachableAreaNum( move.moveDest, 1.0f );
		}
	}
}

/*
=====================
idAI::UpdateEnemyPosition
=====================
*/
void idAI::UpdateEnemyPosition( void ) {
	idActor *enemyEnt = enemy.GetEntity();
	int				enemyAreaNum;
	int				areaNum;
	aasPath_t		path;
	idVec3			enemyPos;
	bool			onGround;

	if ( !enemyEnt ) { //If we don't currently have an enemy, then exit now.
		return;
	}
	
	//TODO: remove some of the ESP from this section
	const idVec3 &org = physicsObj.GetOrigin();

	if ( move.moveType == MOVETYPE_FLY ) {
		enemyPos = enemyEnt->GetPhysics()->GetOrigin();
		onGround = true;
	} else {
		onGround = enemyEnt->GetFloorPos( 64.0f, enemyPos );
		if ( enemyEnt->OnLadder() ) {
			onGround = false;
		}
	}

	if ( onGround ) {
		//If enemy is on the ground.

		if ( !aas ) {
			//When we don't have an AAS, we can't tell if an enemy is reachable or not, so just assume that they are reachable.
			enemyAreaNum = 0;
			lastReachableEnemyPos = enemyPos;
		} else {
			//Use AAS to find position that will let me reach enemy.
			enemyAreaNum = PointReachableAreaNum( enemyPos, 1.0f );
			if ( enemyAreaNum ) {
				areaNum = PointReachableAreaNum( org );
				if ( PathToGoal( path, areaNum, org, enemyAreaNum, enemyPos ) ) {
					lastReachableEnemyPos = enemyPos;
				}
			}
		}
	}

	AI_ENEMY_IN_FOV		= false;
	AI_ENEMY_VISIBLE	= false;

	if ( CanSee( enemyEnt, false ) )
	{
		AI_ENEMY_VISIBLE = true; //Enemy_visible = ENEMY IS WITHIN MY 360 RADIUS.

		//if ( CheckFOV( enemyEnt->GetEyePosition()  ) || CheckFOV(enemyEnt->GetPhysics()->GetOrigin())) //Check if I can see their HEAD or their FEET.
		if (CheckFOV(enemyEnt->GetEyePosition()))  //Check if I can see their HEAD.
		{
			AI_ENEMY_IN_FOV = true; //Enemy_in_fov = ENEMY IS WITHIN MY VISION CONE.


			//gameRenderWorld->DebugArrow(colorRed, GetPhysics()->GetOrigin(), enemyEnt->GetPhysics()->GetOrigin(), 4, 1000);


			//Set the last known visible enemy position here.

			//Drop the last known position to the ground.
			lastVisibleEnemyPos = enemyEnt->GetPhysics()->GetOrigin();

			// BC We used to drop the LKP to the ground, in order to solve the problem of AI trying to walk to walk toward an investigation point floating in the air. But the problem
			// with this was when the player was teetering on a ledge. Their model's center point would hit a spot on the ground below, which caused the AI to think the enemy was on
			// the ground below.

			//The solution we do here is we check if the enemy target has ground contacts. If they do, then we set the LKP where it is. If no ground contacts, then we trace downward
			//and place the LKP on any ground we find.

			
			if (enemyEnt->GetPhysics()->HasGroundContacts())
			{
				//Has ground contact.
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPReachablePosition(enemyEnt->GetPhysics()->GetContact(0).point);
			}
			else
			{
				trace_t tr;
				//Drop to ground below.
				gameLocal.clip.TracePoint(tr, enemyEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), enemyEnt->GetPhysics()->GetOrigin() + idVec3(0, 0, -1024), MASK_SOLID, this);
				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPReachablePosition(tr.endpos);
			}

			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPPosition(lastVisibleEnemyPos);
			
		}

		SetEnemyPosition();
	}
	else
	{
		// check if we heard any sounds in the last frame

		/* BC remove this sensory system. Replace with SuspiciousNoise system.
		if ( enemyEnt == gameLocal.GetAlertEntity() )
		{
			float dist = ( enemyEnt->GetPhysics()->GetOrigin() - org ).LengthSqr();
			if ( dist < Square( AI_HEARING_RANGE ) )
			{
				SetEnemyPosition();
			}
		}*/

		canSeeEnemy = false;
	}

	if (lastFovCheck != (bool)AI_ENEMY_IN_FOV)
	{
		//The ai just GAINED or LOST los to enemy.
		lastFovCheck = AI_ENEMY_IN_FOV;

		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(lastFovCheck);
	}

	if ( ai_debugMove.GetBool() ) {
		//gameRenderWorld->DebugBounds( colorLtGrey, enemyEnt->GetPhysics()->GetBounds(), lastReachableEnemyPos, gameLocal.msec );
		gameRenderWorld->DebugArrowSimple(lastReachableEnemyPos, gameLocal.msec);
		gameRenderWorld->DrawText("LAST REACHABLE ENEMY POS", lastReachableEnemyPos + idVec3(0, 0, 64), 0.1f, colorLtGrey, gameLocal.GetLocalPlayer()->viewAxis, 1, gameLocal.msec);


		//gameRenderWorld->DebugBounds( colorWhite, enemyEnt->GetPhysics()->GetBounds(), lastVisibleReachableEnemyPos, gameLocal.msec );
		gameRenderWorld->DebugArrowSimple(lastVisibleReachableEnemyPos, gameLocal.msec);
		gameRenderWorld->DrawText("LAST VISIBLE ENEMY POS", lastVisibleReachableEnemyPos + idVec3(0,0,64), 0.1f, colorLtGrey, gameLocal.GetLocalPlayer()->viewAxis, 1, gameLocal.msec);
	}
}

/*
=====================
idAI::SetEnemy
=====================
*/
void idAI::SetEnemy( idActor *newEnemy ) {
	int enemyAreaNum;

	if ( AI_DEAD ) {
		ClearEnemy();
		return;
	}

	AI_ENEMY_DEAD = false;
	if ( !newEnemy ) {
		ClearEnemy();
	} else if ( enemy.GetEntity() != newEnemy ) {
		enemy = newEnemy;
		enemyNode.AddToEnd( newEnemy->enemyList );
		if ( newEnemy->health <= 0 ) {
			EnemyDead();
			return;
		}
		// let the monster know where the enemy is
		newEnemy->GetAASLocation( aas, lastReachableEnemyPos, enemyAreaNum );
		SetEnemyPosition();
		SetChatSound();

		lastReachableEnemyPos = lastVisibleEnemyPos;
		lastVisibleReachableEnemyPos = lastReachableEnemyPos;
		enemyAreaNum = PointReachableAreaNum( lastReachableEnemyPos, 1.0f );
		if ( aas && enemyAreaNum ) {
			aas->PushPointIntoAreaNum( enemyAreaNum, lastReachableEnemyPos );
			lastVisibleReachableEnemyPos = lastReachableEnemyPos;
		}
	}
}

/*
============
idAI::FirstVisiblePointOnPath
============
*/
idVec3 idAI::FirstVisiblePointOnPath( const idVec3 origin, const idVec3 &target, int travelFlags ) const {
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

		if ( !aas->RouteToGoalArea( curAreaNum, curOrigin, targetAreaNum, travelFlags, travelTime, &reach ) ) {
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

			if (frame < 0)
			{
				frame = anim->FindFrameForFrameCommand(FC_LAUNCHMISSILEATLASER, &command); //BC as a fallback, attempt to search for this animevent...
			}

			if (frame < 0)
			{
				//search for the dummy command. This is for things like the combatobserve sidestep, where there is no attack associated with the animation.
				frame = anim->FindFrameForFrameCommand(FC_JOINTMARKER, &command);
			}

			if ( frame >= 0 )
			{
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
idAI::CreateProjectileClipModel
=====================
*/
void idAI::CreateProjectileClipModel( void ) const {
	if ( projectileClipModel == NULL ) {
		idBounds projectileBounds( vec3_origin );
		projectileBounds.ExpandSelf( projectileRadius );
		projectileClipModel	= new idClipModel( idTraceModel( projectileBounds ) );
	}
}

/*
=====================
idAI::GetAimDir
=====================
*/
bool idAI::GetAimDir( const idVec3 &firePos, idEntity *aimAtEnt, const idEntity *ignore, idVec3 &aimDir ) const {
	idVec3	targetPos1;
	idVec3	targetPos2;
	idVec3	delta;
	float	max_height;
	bool	result;

	// if no aimAtEnt or projectile set
	if ( !aimAtEnt || !projectileDef ) {
		aimDir = viewAxis[ 0 ] * physicsObj.GetGravityAxis();
		return false;
	}

	if ( projectileClipModel == NULL ) {
		CreateProjectileClipModel();
	}

	if ( aimAtEnt == enemy.GetEntity() ) {
		static_cast<idActor *>( aimAtEnt )->GetAIAimTargets( lastVisibleEnemyPos, targetPos1, targetPos2 );
	} else if ( aimAtEnt->IsType( idActor::Type ) ) {
		static_cast<idActor *>( aimAtEnt )->GetAIAimTargets( aimAtEnt->GetPhysics()->GetOrigin(), targetPos1, targetPos2 );
	} else {
		targetPos1 = aimAtEnt->GetPhysics()->GetAbsBounds().GetCenter();
		targetPos2 = targetPos1;
	}

#ifdef _D3XP
	if ( this->team == 0 && !idStr::Cmp( aimAtEnt->GetEntityDefName(), "monster_demon_vulgar" ) ) {
		targetPos1.z -= 28.f;
		targetPos2.z -= 12.f;
	}
#endif

	// try aiming for chest
	delta = firePos - targetPos1;
	max_height = delta.LengthFast() * projectile_height_to_distance_ratio;
	result = PredictTrajectory( firePos, targetPos1, projectileSpeed, projectileGravity, projectileClipModel, MASK_SHOT_RENDERMODEL, max_height, ignore, aimAtEnt, ai_debugTrajectory.GetBool() ? 1000 : 0, aimDir );
	if ( result || !aimAtEnt->IsType( idActor::Type ) ) {
		return result;
	}

	// try aiming for head
	delta = firePos - targetPos2;
	max_height = delta.LengthFast() * projectile_height_to_distance_ratio;
	result = PredictTrajectory( firePos, targetPos2, projectileSpeed, projectileGravity, projectileClipModel, MASK_SHOT_RENDERMODEL, max_height, ignore, aimAtEnt, ai_debugTrajectory.GetBool() ? 1000 : 0, aimDir );

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
idProjectile *idAI::CreateProjectile( const idVec3 &pos, const idVec3 &dir ) {
	idEntity *ent;
	const char *clsname;

	if ( !projectile.GetEntity() ) {
		gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
		if ( !ent ) {
			clsname = projectileDef->GetString( "classname" );
			gameLocal.Error( "Could not spawn entityDef '%s'", clsname );
		}

		if ( !ent->IsType( idProjectile::Type ) ) {
			clsname = ent->GetClassname();
			gameLocal.Error( "'%s' is not an idProjectile", clsname );
		}
		projectile = ( idProjectile * )ent;
	}

	projectile.GetEntity()->Create( this, pos, dir );

	return projectile.GetEntity();
}

/*
=====================
idAI::RemoveProjectile
=====================
*/
void idAI::RemoveProjectile( void ) {
	if ( projectile.GetEntity() ) {
		projectile.GetEntity()->PostEventMS( &EV_Remove, 0 );
		projectile = NULL;
	}
}

//BC this is what gets called when AI fires weapon. Launches projectile toward a specified XYZ.
void idAI::LaunchProjectileAtPos(const char *jointname, idVec3 fireTarget)
{
	float				attack_accuracy, attack_cone, projectile_spread;
	int					num_projectiles;
	idMat3				axis;
	idVec3				muzzle;
	idProjectile		*lastProjectile;
	idVec3				dir;
	idVec3				tmp;
	idAngles			ang;
	int					i;

	if (!projectileDef)
	{
		gameLocal.Warning("%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName());
		return;
	}	

	attack_accuracy = spawnArgs.GetFloat("attack_accuracy", "7");
	attack_cone = spawnArgs.GetFloat("attack_cone", "70");
	projectile_spread = spawnArgs.GetFloat("projectile_spread", "0");
	num_projectiles = spawnArgs.GetInt("num_projectiles", "1");

	GetMuzzle(jointname, muzzle, axis);

	if (!projectile.GetEntity())
	{
		CreateProjectile(muzzle, axis[0]);
	}

	lastProjectile = projectile.GetEntity();

	//TODO: if enemy is inside my tight firing cone, then snap fireTarget to the enemy position.

	tmp = fireTarget - muzzle;
	tmp.Normalize();
	axis[2] = axis[0];
	axis[0] = -tmp;
	
	if (1) // make sure the projectile starts inside the monster bounding box
	{
		idBounds			projBounds;		
		const idClipModel	*projClip;
		idVec3				start;
		trace_t				tr;
		float				distance;

		const idBounds &ownerBounds = physicsObj.GetAbsBounds();
		projClip = lastProjectile->GetPhysics()->GetClipModel();
		projBounds = projClip->GetBounds().Rotate(axis);

		// check if the owner bounds is bigger than the projectile bounds
		if (((ownerBounds[1][0] - ownerBounds[0][0]) > (projBounds[1][0] - projBounds[0][0])) &&
			((ownerBounds[1][1] - ownerBounds[0][1]) > (projBounds[1][1] - projBounds[0][1])) &&
			((ownerBounds[1][2] - ownerBounds[0][2]) > (projBounds[1][2] - projBounds[0][2]))) {
			if ((ownerBounds - projBounds).RayIntersection(muzzle, viewAxis[0], distance)) {
				start = muzzle + distance * viewAxis[0];
			}
			else {
				start = ownerBounds.GetCenter();
			}
		}
		else {
			// projectile bounds bigger than the owner bounds, so just start it from the center
			start = ownerBounds.GetCenter();
		}

		gameLocal.clip.Translation(tr, start, muzzle, projClip, axis, MASK_SHOT_RENDERMODEL, this);
		muzzle = tr.endpos;
	}

	dir = fireTarget - muzzle;
	ang = dir.ToAngles();

	// adjust aim so it's not perfect.  uses sine based movement so the tracers appear less random in their spread.
	float t = MS2SEC(gameLocal.time + entityNumber * 497);
	ang.pitch += idMath::Sin16(t * 5.1) * attack_accuracy;
	ang.yaw += idMath::Sin16(t * 6.7) * attack_accuracy;

	if (1)
	{
		// Clamp the attack direction to be within monster's attack cone so they doesn't do things like throw the missile backwards if you're behind them.
		float				diff;
		diff = idMath::AngleDelta(ang.yaw, current_yaw);
		if (diff > attack_cone)
		{
			ang.yaw = current_yaw + attack_cone;
		}
		else if (diff < -attack_cone)
		{
			ang.yaw = current_yaw - attack_cone;
		}
	}

	axis = ang.ToMat3();

	float spreadRad = DEG2RAD(projectile_spread);
	for (i = 0; i < num_projectiles; i++)
	{
		// spread the projectiles out
		float				angle;
		float				spin;

		angle = idMath::Sin(spreadRad * gameLocal.random.RandomFloat());
		spin = (float)DEG2RAD(360.0f) * gameLocal.random.RandomFloat();
		dir = axis[0] + axis[2] * (angle * idMath::Sin(spin)) - axis[1] * (angle * idMath::Cos(spin));
		dir.Normalize();

		// launch the projectile
		if (!projectile.GetEntity())
		{
			CreateProjectile(muzzle, dir);
		}
		lastProjectile = projectile.GetEntity();
		lastProjectile->Launch(muzzle, dir, vec3_origin);
		projectile = NULL;
	}

	TriggerWeaponEffects(muzzle);
	lastAttackTime = gameLocal.time;
	EjectBrass();
	gameLocal.GetLocalPlayer()->SetNoiseEvent(muzzle, NOISETYPE_GUNSHOT);

	//AI fire weapon suspicious noise???	

	if (ai_debugPerception.GetInteger() == 2)
	{
		gameRenderWorld->DebugSphere(colorRed, idSphere(fireTarget, 4), 300);
	}


	if (this->aiState == AISTATE_JOCKEYED)
	{
		//If I'm being jockeyed, then AI gunfire creates an interestpoint. This is so that other AI will investigate the scuffle.
		gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin() + idVec3(0,0,48), "interest_weaponfire");
	}
}

/*
=====================
idAI::LaunchProjectile
=====================
*/
idProjectile *idAI::LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, idVec3 fallbackTarget)
{
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
#ifdef _D3XP
	idMat3				proj_axis;
	bool				forceMuzzle;
#endif
	idVec3				tmp;
	idProjectile		*lastProjectile;

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	//ATTACK_ACCURACY. 1 = highly accurate. 3+ = low accuracy.

	attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	num_projectiles = spawnArgs.GetInt( "num_projectiles", "1" );
	forceMuzzle = spawnArgs.GetBool( "forceMuzzle", "0" );


	//BC Make sustained fire become more accurate. Make first shots inaccurate.
	if (target != NULL)
	{
		if (lastAttackTime < lastVisibleEnemyTime + INACCURACY_TIME || !target->IsType(idActor::Type))
		{
			//common->Printf("inaccurate shot %d\n", gameLocal.time);
			attack_accuracy = Max(attack_accuracy, 6.0f); //Do an inaccurate shot.
		}
	}

	GetMuzzle( jointname, muzzle, axis );

	if ( !projectile.GetEntity() ) {
		CreateProjectile( muzzle, axis[ 0 ] );
	}

	lastProjectile = projectile.GetEntity();

	if (target != NULL)
	{
		tmp = target->GetPhysics()->GetAbsBounds().GetCenter() - muzzle;
	}
	else
	{
		fallbackTarget.z += 48; //Raise it up a little so it aims for center of mass, not the ground.
		tmp = fallbackTarget - muzzle;
	}

	tmp.Normalize();
	axis = tmp.ToMat3();


	// rotate it because the cone points up by default
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

#ifdef _D3XP
	proj_axis = axis;
#endif

	if ( !forceMuzzle ) {	// _D3XP
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
	}

	// set aiming direction
	if (target != NULL)
	{
		GetAimDir(muzzle, target, this, dir);
	}
	else
	{
		dir = fallbackTarget - muzzle;
	}

	ang = dir.ToAngles();

	// adjust aim so it's not perfect.  uses sine based movement so the tracers appear less random in their spread.
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

		// launch the projectile
		if ( !projectile.GetEntity() ) {
			CreateProjectile( muzzle, dir );
		}
		lastProjectile = projectile.GetEntity();
		lastProjectile->Launch( muzzle, dir, vec3_origin );
		projectile = NULL;

		//BC draw debug showing the projectile trajectory.
		//common->Printf("LaunchProjectile target: %s\n", target->GetName());
		//gameRenderWorld->DebugArrow(colorRed, muzzle, muzzle + (dir * 256), 4, 5000);
		//gameRenderWorld->DebugArrow(colorCyan, target->GetPhysics()->GetOrigin() + idVec3(0,0,16), target->GetPhysics()->GetOrigin(), 4, 3000);
	}

	//Muzzle flash fx.
	TriggerWeaponEffects( muzzle );

	lastAttackTime = gameLocal.time;

	//BC shell casing.
	EjectBrass();

	//BC Noise fx
	gameLocal.GetLocalPlayer()->SetNoiseEvent(muzzle, NOISETYPE_GUNSHOT);

	//AI gunshot attracts nearby enemies???

	return lastProjectile;
}

void idAI::EjectBrass()
{	
	if (lastBrassTime > gameLocal.time)
		return;

	if (g_showBrass.GetBool())
	{
		idEntity *brassEnt;

		lastBrassTime = gameLocal.time + 200; //limit brass ejection to every xx milliseconds.
		gameLocal.SpawnEntityDef(brassDict, &brassEnt, false);

		if (brassEnt && brassEnt->IsType(idDebris::Type))
		{
			idDebris *debris = static_cast<idDebris *>(brassEnt);
			idAngles ejectDir;
			idVec3 brassSpawnPos;
			idMat3 brassAxis;

			GetJointWorldTransform(brassJoint, gameLocal.time, brassSpawnPos, brassAxis);

			debris->Create(this, brassSpawnPos, mat3_identity);
			if (debris)
			{
				debris->Launch();

				//Spin brass.
				debris->GetPhysics()->SetAngularVelocity(idVec3(400 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat()));

				//Spit out brass.
				ejectDir = brassAxis.ToAngles();
				ejectDir.pitch = -60;
				ejectDir.yaw += (90 + gameLocal.random.CRandomFloat() * 10);

				//gameRenderWorld->DebugArrow(colorGreen, debris->GetPhysics()->GetOrigin(), debris->GetPhysics()->GetOrigin() + ejectDir.ToForward() * 64, 3, 1000);

				debris->GetPhysics()->SetLinearVelocity(ejectDir.ToForward() * (32 + gameLocal.random.RandomInt(48)));
			}
		}
	}
}

/*
================
idAI::DamageFeedback

callback function for when another entity received damage from this entity.  damage can be adjusted and returned to the caller.

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

	//FIXME: make work with gravity vector
	idVec3 org = physicsObj.GetOrigin();
	const idBounds &myBounds = physicsObj.GetBounds();
	idBounds bounds;

	// expand the bounds out by our melee range
	bounds[0][0] = -melee_range;
	bounds[0][1] = -melee_range;
	bounds[0][2] = myBounds[0][2] - 4.0f;
	bounds[1][0] = melee_range;
	bounds[1][1] = melee_range;
	bounds[1][2] = myBounds[1][2] + 4.0f;
	bounds.TranslateSelf( org );

	idVec3 enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
	idBounds enemyBounds = enemyEnt->GetPhysics()->GetBounds();
	enemyBounds.TranslateSelf( enemyOrg );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorYellow, bounds, vec3_zero, gameLocal.msec );
	}

	if ( !bounds.IntersectsBounds( enemyBounds ) ) {
		return false;
	}

	idVec3 start = GetEyePosition();
	idVec3 end = enemyEnt->GetEyePosition();

	gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
	if ( ( trace.fraction == 1.0f ) || ( gameLocal.GetTraceEntity( trace ) == enemyEnt ) ) {
		return true;
	}

	return false;
}

/*
=====================
idAI::AttackMelee

jointname allows the endpoint to be exactly specified in the model,
as for the commando tentacle.  If not specified, it will be set to
the facing direction + melee_range.

kickDir is specified in the monster's coordinate system, and gives the direction
that the view kick and knockback should go
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

	// check for the "saving throw" automatic melee miss on lethal blow
	// stupid place for this.
	bool forceMiss = false;
	/* BC handle the savingthrow stuff in: player.cpp CalcDamagePoints().
	if ( enemyEnt->IsType( idPlayer::Type ) && g_skill.GetInteger() < 2 ) {
		int	damage, armor;
		idPlayer *player = static_cast<idPlayer*>( enemyEnt );
		player->CalcDamagePoints( this, this, meleeDef, 1.0f, INVALID_JOINT, &damage, &armor );

		if ( enemyEnt->health <= damage ) {
			int	t = gameLocal.time - player->lastSavingThrowTime;
			if ( t > SAVING_THROW_TIME ) {
				player->lastSavingThrowTime = gameLocal.time;
				t = 0;
			}
			if ( t < 1000 ) {
				gameLocal.Printf( "Saving throw.\n" );
				forceMiss = true;
			}
		}
	}*/

	// make sure the trace can actually hit the enemy
	if ( forceMiss || !TestMelee() ) {
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
	afTouch_t touchList[ MAX_GENTITIES ];
	idEntity *pushed_ents[ MAX_GENTITIES ];
	idEntity *ent;
	idVec3 vel;
	int num_pushed;

	num_pushed = 0;
	af.ChangePose( this, gameLocal.time );
	int num = af.EntitiesTouchingAF( touchList );
	for( i = 0; i < num; i++ ) {
		if ( touchList[ i ].touchedEnt->IsType( idProjectile::Type ) ) {
			// skip projectiles
			continue;
		}

		// make sure we havent pushed this entity already.  this avoids causing double damage
		for( j = 0; j < num_pushed; j++ ) {
			if ( pushed_ents[ j ] == touchList[ i ].touchedEnt ) {
				break;
			}
		}
		if ( j >= num_pushed ) {
			ent = touchList[ i ].touchedEnt;
			pushed_ents[num_pushed++] = ent;
			vel = ent->GetPhysics()->GetAbsBounds().GetCenter() - touchList[ i ].touchedByBody->GetWorldOrigin();
			vel.Normalize();
			if ( attack.Length() && ent->IsType( idActor::Type ) ) {
				ent->Damage( this, this, vel, attack, 1.0f, INVALID_JOINT );
			} else {
				ent->GetPhysics()->SetLinearVelocity( 100.0f * vel, touchList[ i ].touchedClipModel->GetId() );
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
	SetChatSound();

	AI_ENEMY_IN_FOV		= false;
	AI_ENEMY_VISIBLE	= false;
	StopMove( MOVE_STATUS_DONE );
}

/*
================
idAI::Show
================
*/
void idAI::Show( void ) {
	idActor::Show();
	if ( spawnArgs.GetBool( "big_monster" ) ) {
		physicsObj.SetContents( 0 );
	} else if ( use_combat_bbox ) {
		physicsObj.SetContents( CONTENTS_BODY|CONTENTS_SOLID );
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
	}
	physicsObj.GetClipModel()->Link( gameLocal.clip );
	fl.takedamage = !spawnArgs.GetBool( "noDamage" );
	SetChatSound();
	StartSound( "snd_ambient", SND_CHANNEL_AMBIENT, 0, false, NULL );
}

/*
=====================
idAI::SetChatSound
=====================
*/
void idAI::SetChatSound( void ) {
	const char *snd;

	if ( IsHidden() ) {
		snd = NULL;
	} else if ( enemy.GetEntity() ) {
		snd = spawnArgs.GetString( "snd_chatter_combat", NULL );
		chat_min = SEC2MS( spawnArgs.GetFloat( "chatter_combat_min", "5" ) );
		chat_max = SEC2MS( spawnArgs.GetFloat( "chatter_combat_max", "10" ) );
	} else if ( !spawnArgs.GetBool( "no_idle_chatter" ) ) {
		snd = spawnArgs.GetString( "snd_chatter", NULL );
		chat_min = SEC2MS( spawnArgs.GetFloat( "chatter_min", "5" ) );
		chat_max = SEC2MS( spawnArgs.GetFloat( "chatter_max", "10" ) );
	} else {
		snd = NULL;
	}

	if ( snd && *snd ) {
		chat_snd = declManager->FindSound( snd );

		// set the next chat time
		chat_time = gameLocal.time + chat_min + gameLocal.random.RandomFloat() * ( chat_max - chat_min );
	} else {
		chat_snd = NULL;
	}
}

/*
================
idAI::CanPlayChatterSounds

Used for playing chatter sounds on monsters.
================
*/
bool idAI::CanPlayChatterSounds( void ) const {
	if ( AI_DEAD ) {
		return false;
	}

	if ( IsHidden() ) {
		return false;
	}

	if ( enemy.GetEntity() ) {
		return true;
	}

	if ( spawnArgs.GetBool( "no_idle_chatter" ) ) {
		return false;
	}

	return true;
}

/*
=====================
idAI::PlayChatter
=====================
*/
void idAI::PlayChatter( void ) {
	// check if it's time to play a chat sound
	if ( AI_DEAD || !chat_snd || ( chat_time > gameLocal.time ) ) {
		return;
	}

	StartSoundShader( chat_snd, SND_CHANNEL_VOICE, 0, false, NULL );

	// set the next chat time
	chat_time = gameLocal.time + chat_min + gameLocal.random.RandomFloat() * ( chat_max - chat_min );
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
#ifdef _D3XP
			// Smoke particles on AI characters will always be "slow", even when held by grabber
			SetTimeState ts(TIME_GROUP1);
#endif
			if ( particles[i].particle && particles[i].time )
			{
				idAngles emitAngle;

				particlesAlive++;
				if (af.IsActive()) {
					realAxis = mat3_identity;
					realVector = GetPhysics()->GetOrigin();
				} else {
					animator.GetJointTransform( particles[i].joint, gameLocal.time, realVector, realAxis );
					realAxis *= renderEntity.axis;
					realVector = physicsObj.GetOrigin() + ( realVector + modelOffset ) * ( viewAxis * physicsObj.GetGravityAxis() );
				}

				//BC THIS MAKES ALL PARTICLES EMIT AT ROLL-90 ANGLE... REVERT THIS IF PARTICLE ANGLES START GOING BONKERS
				emitAngle = realAxis.ToAngles();
				emitAngle.roll -= 90;
				realAxis = emitAngle.ToMat3();

				if ( !gameLocal.smokeParticles->EmitSmoke( particles[i].particle, particles[i].time, gameLocal.random.CRandomFloat(), realVector, realAxis, timeGroup /*_D3XP*/ )) {
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

#ifdef _D3XP
void idAI::TriggerFX( const char* joint, const char* fx ) {

	if( !strcmp(joint, "origin") ) {
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
	} else {
		idVec3	joint_origin;
		idMat3	joint_axis;
		jointHandle_t jointNum;
		jointNum = animator.GetJointHandle( joint );

		if ( jointNum == INVALID_JOINT ) {
			gameLocal.Warning( "Unknown fx joint '%s' on entity %s", joint, name.c_str() );
			return;
		}

		GetJointWorldTransform( jointNum, gameLocal.time, joint_origin, joint_axis );
		idEntityFx::StartFx( fx, &joint_origin, &joint_axis, this, true );
	}
}

idEntity* idAI::StartEmitter( const char* name, const char* joint, const char* particle ) {

	idEntity* existing = GetEmitter(name);
	if(existing) {
		return existing;
	}

	jointHandle_t jointNum;
	jointNum = animator.GetJointHandle( joint );

	idVec3 offset;
	idMat3 axis;

	GetJointWorldTransform( jointNum, gameLocal.time, offset, axis );

	/*animator.GetJointTransform( jointNum, gameLocal.time, offset, axis );
	offset = GetPhysics()->GetOrigin() + offset * GetPhysics()->GetAxis();
	axis = axis * GetPhysics()->GetAxis();*/



	idDict args;

	const idDeclEntityDef *emitterDef = gameLocal.FindEntityDef( "func_emitter", false );
	args = emitterDef->dict;
	args.Set("model", particle);
	args.Set( "origin", offset.ToString() );
	args.SetBool("start_off", true);

	idEntity* ent;
	gameLocal.SpawnEntityDef(args, &ent, false);

	ent->GetPhysics()->SetOrigin(offset);
	//ent->GetPhysics()->SetAxis(axis);

	// align z-axis of model with the direction
	/*idVec3		tmp;
	axis = (viewAxis[ 0 ] * physicsObj.GetGravityAxis()).ToMat3();
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	ent->GetPhysics()->SetAxis(axis);*/

	axis = physicsObj.GetGravityAxis();
	ent->GetPhysics()->SetAxis(axis);


	ent->GetPhysics()->GetClipModel()->SetOwner( this );


	//Keep a reference to the emitter so we can track it
	funcEmitter_t newEmitter;
	strcpy(newEmitter.name, name);
	newEmitter.particle = (idFuncEmitter*)ent;
	newEmitter.joint = jointNum;
	funcEmitters.Set(newEmitter.name, newEmitter);

	//Bind it to the joint and make it active
	newEmitter.particle->BindToJoint(this, jointNum, true);
	newEmitter.particle->BecomeActive(TH_THINK);
	newEmitter.particle->Show();
	newEmitter.particle->PostEventMS(&EV_Activate, 0, this);
	return newEmitter.particle;
}

idEntity* idAI::GetEmitter( const char* name ) {
	funcEmitter_t* emitter;
	funcEmitters.Get(name, &emitter);
	if(emitter) {
		return emitter->particle;
	}
	return NULL;
}

void idAI::StopEmitter( const char* name ) {
	funcEmitter_t* emitter;
	funcEmitters.Get(name, &emitter);
	if(emitter) {
		emitter->particle->Unbind();
		emitter->particle->PostEventMS( &EV_Remove, 0 );
		funcEmitters.Remove(name);
	}
}

#endif


/***********************************************************************

	Head & torso aiming

***********************************************************************/

/*
================
idAI::UpdateAnimationControllers
================
*/
bool idAI::UpdateAnimationControllers( void ) {
	idVec3		local;
	idVec3		focusPos;
	idQuat		jawQuat;
	idVec3		left;
	idVec3		dir;
	idVec3		orientationJointPos;
	idVec3		localDir;
	idAngles	newLookAng;
	idAngles	diff;
	idMat3		mat;
	idMat3		axis;
	idMat3		orientationJointAxis;
	idAFAttachment	*headEnt = head.GetEntity();
	idVec3		eyepos;
	idVec3		pos;
	int			i;
	idAngles	jointAng;
	float		orientationJointYaw;

	if ( AI_DEAD ) {
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
			gameRenderWorld->DebugLine( colorRed, eyepos, eyepos + orientationJointAxis[ 0 ] * 32.0f, gameLocal.msec );
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

	if ( !allowJointMod || !allowEyeFocus || ( gameLocal.time >= focusTime ) )
	{
		focusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 512.0f;
	}
	else if ( focusEnt == NULL )
	{
		// keep looking at last position until focusTime is up
		focusPos = currentFocusPos;
	}
	else if ( focusEnt == enemy.GetEntity() )
	{
		focusPos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset - eyeVerticalOffset * enemy.GetEntity()->GetPhysics()->GetGravityNormal();
	}
	else if ( focusEnt->IsType( idActor::Type ) )
	{
		focusPos = static_cast<idActor*>(focusEnt)->GetEyePosition() - eyeVerticalOffset * focusEnt->GetPhysics()->GetGravityNormal();
	}
	else
	{
		focusPos = focusEnt->GetPhysics()->GetOrigin();
	}

	currentFocusPos = currentFocusPos + ( focusPos - currentFocusPos ) * eyeFocusRate;

	// determine yaw from origin instead of from focus joint since joint may be offset, which can cause us to bounce between two angles
	dir = focusPos - orientationJointPos;
	newLookAng.yaw = idMath::AngleNormalize180( dir.ToYaw() - orientationJointYaw );
	newLookAng.roll = 0.0f;
	newLookAng.pitch = 0.0f;

#if 0
	gameRenderWorld->DebugLine( colorRed, orientationJointPos, focusPos, gameLocal.msec );
	gameRenderWorld->DebugLine( colorYellow, orientationJointPos, orientationJointPos + orientationJointAxis[ 0 ] * 32.0f, gameLocal.msec );
	gameRenderWorld->DebugLine( colorGreen, orientationJointPos, orientationJointPos + newLookAng.ToForward() * 48.0f, gameLocal.msec );
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
			//alignHeadTime = gameLocal.time + ( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime;
			alignHeadTime = gameLocal.time + (1.0f) * focusAlignTime; //bc
		}
	}

	if ( idMath::Fabs( newLookAng.yaw ) < 0.1f ) {
		alignHeadTime = gameLocal.time;
	}

	if ( ( gameLocal.time >= alignHeadTime ) || ( gameLocal.time < forceAlignHeadTime ) ) {
		//alignHeadTime = gameLocal.time + (0.5f + 0.5f * gameLocal.random.RandomFloat()) * focusAlignTime;
		alignHeadTime = gameLocal.time + ( 1.0f ) * focusAlignTime; //bc
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
	for( i = 0; i < lookJoints.Num(); i++ )
	{
		jointAng.pitch	= lookAng.pitch * lookJointAngles[ i ].pitch;
		jointAng.yaw	= lookAng.yaw * lookJointAngles[ i ].yaw;
		animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() ); //this controls the lookjoint bones.
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		// lean into turns
		AdjustFlyingAngles();
	}

	if ( headEnt ) {
		idAnimator *headAnimator = headEnt->GetAnimator();

		if ( allowEyeFocus ) {
			idMat3 eyeAxis = ( lookAng + eyeAng ).ToMat3(); idMat3 headTranspose = headEnt->GetPhysics()->GetAxis().Transpose();
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

const idEventDef EV_CombatNode_MarkUsed( "markUsed" );

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
	savefile->WriteFloat( min_dist ); // float min_dist
	savefile->WriteFloat( max_dist ); // float max_dist
	savefile->WriteFloat( cone_dist ); // float cone_dist
	savefile->WriteFloat( min_height ); // float min_height
	savefile->WriteFloat( max_height ); // float max_height
	savefile->WriteVec3( cone_left ); // idVec3 cone_left
	savefile->WriteVec3( cone_right ); // idVec3 cone_right
	savefile->WriteVec3( offset ); // idVec3 offset
	savefile->WriteBool( disabled ); // bool disabled
}

/*
=====================
idCombatNode::Restore
=====================
*/
void idCombatNode::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( min_dist ); // float min_dist
	savefile->ReadFloat( max_dist ); // float max_dist
	savefile->ReadFloat( cone_dist ); // float cone_dist
	savefile->ReadFloat( min_height ); // float min_height
	savefile->ReadFloat( max_height ); // float max_height
	savefile->ReadVec3( cone_left ); // idVec3 cone_left
	savefile->ReadVec3( cone_right ); // idVec3 cone_right
	savefile->ReadVec3( offset ); // idVec3 offset
	savefile->ReadBool( disabled ); // bool disabled
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

			gameRenderWorld->DebugLine( color, node->GetPhysics()->GetOrigin(), ( pos1 + pos3 ) * 0.5f, gameLocal.msec );
			gameRenderWorld->DebugLine( color, pos1, pos2, gameLocal.msec );
			gameRenderWorld->DebugLine( color, pos1, pos3, gameLocal.msec );
			gameRenderWorld->DebugLine( color, pos3, pos4, gameLocal.msec );
			gameRenderWorld->DebugLine( color, pos2, pos4, gameLocal.msec );
			gameRenderWorld->DebugBounds( color, bounds, org, gameLocal.msec );
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

void idAI::EnterVacuum(void)
{
	AI_SPECIAL_DAMAGE = true;
}

void idAI::SpawnEmitters(const char *keyName) {
	const idKeyValue *kv = spawnArgs.MatchPrefix(keyName, NULL);
	while (kv)
	{
		idStr particleName = kv->GetValue();

		if (particleName.Length())
		{
			idStr jointName = kv->GetValue();
			int dash = jointName.Find('-');
			if (dash > 0)
			{
				//SPAWN THE EMITTER
				idFuncEmitter *emitter;
				idDict args;
				jointHandle_t emitJoint;
				idVec3 emitPos;
				idMat3 emitMat3;
				idAngles emitAngle;

				particleName = particleName.Left(dash);
				jointName = jointName.Right(jointName.Length() - dash - 1);

				emitJoint = this->GetAnimator()->GetJointHandle(jointName);
				this->GetJointWorldTransform(emitJoint, gameLocal.time, emitPos, emitMat3);

				emitAngle = emitMat3.ToAngles();
				emitAngle.roll -= 90;				

				args.Set("model", particleName);
				//args.Set("start_off", "0");
				emitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
				emitter->GetPhysics()->SetOrigin(emitPos);
				emitter->SetAngles(emitAngle);
				//emitter->PostEventMS(&EV_Activate, 0, this);
				emitter->BindToJoint(this, emitJoint, true);

				emitter->SetActive(true);				
			}
		}

		kv = spawnArgs.MatchPrefix(keyName, kv);
	}
}

//This gets called when player drags the enemy body.
bool idAI::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	if (bleedoutState == BLEEDOUT_DONE || bleedoutState == BLEEDOUT_SKULLBURN) //If I am dissolving out of existence, then I am ungrabbable.
		return false;

	//BC disabling the body drag.
	//if (index == 0 || index == 1 || index == 2 || index == 3)
	//{
	//	jointHandle_t jointHandle = INVALID_JOINT;
	//	idVec3 jointPos;
	//	idMat3 jointAxis;
	//	int id = 0;
	//
	//	if (index == 0)
	//	{
	//		//left hand.
	//		jointHandle = animator.GetJointHandle("lhand");
	//		id = af.GetPhysics()->GetBodyId("lloarm"); //the body has to be a named part of the articulated figure.
	//	}
	//	if (index == 1)
	//	{
	//		//right hand.
	//		jointHandle = animator.GetJointHandle("rhand");
	//		id = af.GetPhysics()->GetBodyId("rloarm");
	//	}
	//	if (index == 2)
	//	{
	//		//left foot.
	//		jointHandle = animator.GetJointHandle("lankle");
	//		id = af.GetPhysics()->GetBodyId("lloleg");
	//	}
	//	if (index == 3)
	//	{
	//		//right foot.
	//		jointHandle = animator.GetJointHandle("rankle");
	//		id = af.GetPhysics()->GetBodyId("rloleg");
	//	}
	//
	//	GetJointWorldTransform(jointHandle, gameLocal.time, jointPos, jointAxis);
	//	if (gameLocal.GetLocalPlayer()->bodyDragger.StartDrag(this, jointHandle, jointPos, id))
	//	{
	//		//int i;
	//		//Successful drag.
	//
	//		//Disable the frobcubes.
	//		//for (i = 0; i <  AI_LIMBCOUNT; i++)
	//		//{
	//		//	dragButtons[i]->isFrobbable = false;
	//		//}
	//		
	//		if (spawnArgs.GetBool("can_snapneck"))
	//		{
	//			dragButtons[0]->isFrobbable = false;
	//		}
	//		
	//
	//		//Particle fx.
	//		idEntityFx::StartFx("fx/grab01", &jointPos, &mat3_identity, NULL, false);
	//	}
	//}

	if (index == SNAPNECK_FROBINDEX)
	{
		//make all drag buttons unfrobbable.
		//int i;
		//for (i = 0; i < AI_LIMBCOUNT; i++)
		//{
		//	dragButtons[i]->isFrobbable = false;
		//}

		if (spawnArgs.GetBool("can_snapneck"))
		{
			dragButtons[0]->isFrobbable = false;
		}

		StartSound("snd_necksnap", SND_CHANNEL_ANY);

		idVec3 impulse = idVec3(0, 0, 512);
		GetPhysics()->SetLinearVelocity(impulse);

		//bleed out immediately.
		bleedoutTime = gameLocal.time;
	}

	return true;
}

void idAI::SetDragFrobbable(bool value)
{
	//int i;
	//if (!spawnArgs.GetBool("draggable"))
	//	return;

	//Re-enable frobcubes.
	//for (i = 0; i < AI_LIMBCOUNT; i++)
	//{
	//	dragButtons[i]->isFrobbable = value;
	//}
	
	//BC re-using the dragpoints to instead be the snap neck interaction
	if (spawnArgs.GetBool("can_snapneck"))
	{
		dragButtons[0]->Show();
		dragButtons[0]->isFrobbable = value;
	}
	
	
}

//We do this event when AI takes damage and shield absorbs all the damage. We needed a way to hook into the script system to "wake up" the AI without playing a pain anim.
void idAI::OnShieldHit()
{
	AI_SHIELDHIT = true;
}

void idAI::SetAlertedState(bool investigateLKP)
{
	//Set in gunner ent.
}

void idAI::SetJockeyState(bool value)
{
	//Set in gunner ent.
}

void idAI::DoJockeySlamDamage()
{
	//Set in gunner ent.
}

void idAI::DoJockeyBrutalSlam()
{
}

void idAI::DoWorldDamage()
{
}

//Return whether AI is currently bleeding out.
bool idAI::GetBleedingOut()
{
	if (bleedoutState == BLEEDOUT_ACTIVE || bleedoutState == BLEEDOUT_DONE || bleedoutState == BLEEDOUT_SKULLBURN)
		return true;

	return false;
}

void idAI::Resurrect()
{
	bleedoutState = BLEEDOUT_NONE;
	gibbed = false;

	SetDragFrobbable(false); //disable drag points.

	combatState = 0;
	lastFovCheck = false;

	//shield.
	energyShieldCurrent = energyShieldMax;
	energyShieldTimer = 0;
	energyShieldState = ENERGYSHIELDSTATE_STOWED;

	//reset skin.
	SetSkin(currentskin);

	playedBleedoutbeep1 = false;
	playedBleedoutbeep2 = false;
	playedBleedoutbeep3 = false;


	RemoveInterestpointsBoundToMe();

	SetState("State_Resurrect");	
}

//This gets called when the bleedout timer expires OR the ai gets gibbed.
void idAI::SpawnSkullsaver()
{
	const idDeclEntityDef *botDef;
	idVec3 skullSpawnPos;
	idVec3 candidateSkullPos = skullSpawnPos;
	trace_t skullTr;
	idBounds skullBounds;
	jointHandle_t headJoint;
	idMat3 headAxis;

	if (bleedoutState == BLEEDOUT_SKULLBURN || bleedoutState == BLEEDOUT_DONE)
		return;

	bleedoutState = BLEEDOUT_SKULLBURN;

	skullBounds = idBounds(idVec3(-15, -15, -15), idVec3(15, 15, 15));

	headJoint = animator.GetJointHandle("head");
	GetJointWorldTransform(headJoint, gameLocal.time, skullSpawnPos, headAxis);
	skullSpawnPos += idVec3(0, 0, 16);

	//Ok, bot is either dead or nonexistent. Spawn a new one.
	botDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_skullsaver", "moveable_skullsaver"), false);

	if (!botDef)
	{
		gameLocal.Error("idAI %s failed to find entity definition '%s'\n", GetName(), spawnArgs.GetString("def_skullsaver", ""));
	}

	//Do clearance check. Find a suitable place for the skull to spawn.
	#define AMOUNT_OF_CANDIDATESPOTS 29
	idVec3 skullCandidateSpots[] =
	{
		skullSpawnPos,

		skullSpawnPos + idVec3(0,0,16),
		skullSpawnPos + idVec3(0,0,32),
		skullSpawnPos + idVec3(0,0,48),

		skullSpawnPos + idVec3(8,0,16),
		skullSpawnPos + idVec3(8,0,32),
		skullSpawnPos + idVec3(8,0,64),
		skullSpawnPos + idVec3(-8,0,16),
		skullSpawnPos + idVec3(-8,0,32),
		skullSpawnPos + idVec3(-8,0,48),

		skullSpawnPos + idVec3(0,8,16),
		skullSpawnPos + idVec3(0,8,32),
		skullSpawnPos + idVec3(0,8,64),
		skullSpawnPos + idVec3(0,-8,16),
		skullSpawnPos + idVec3(0,-8,32),
		skullSpawnPos + idVec3(0,-8,48),

		//BC 4-13-2025: additional checks that are more farther away.
		//This is to handle the case where a mischievous actor somehow gets their head super jammed into world geometry.
		skullSpawnPos + idVec3(48,0,0),
		skullSpawnPos + idVec3(-48,0,0),
		skullSpawnPos + idVec3(0,48,0),
		skullSpawnPos + idVec3(0,-48,0),
		skullSpawnPos + idVec3(64,0,0),
		skullSpawnPos + idVec3(-64,0,0),
		skullSpawnPos + idVec3(0,64,0),
		skullSpawnPos + idVec3(0,-64,0),
		skullSpawnPos + idVec3(96,0,0),
		skullSpawnPos + idVec3(-96,0,0),
		skullSpawnPos + idVec3(0,96,0),
		skullSpawnPos + idVec3(0,-96,0),


		skullSpawnPos + idVec3(0,0,-16),
	};

	for (int i = 0; i < AMOUNT_OF_CANDIDATESPOTS; i++)
	{
		//First, do a traceline to see if the headjoint position has clear LOS to the candidate spawn position.
		trace_t skullLosTr;
		gameLocal.clip.TracePoint(skullLosTr, skullSpawnPos, skullCandidateSpots[i], MASK_SOLID, this);
		if (skullLosTr.fraction < 1)
			continue;

		//BC 4-13-2025 do a check to see if point starts inside geometry
		//This is to prevent false positives, as a bound check fully inside the void will return "valid"
		int penetrationContents = gameLocal.clip.Contents(skullCandidateSpots[i], NULL, mat3_identity, CONTENTS_SOLID, NULL);
		if (penetrationContents & MASK_SOLID)
		{
			continue; //If it starts in solid, then exit.
		}

		//Now do a bounds check to see if the space is clear.
		gameLocal.clip.TraceBounds(skullTr, skullCandidateSpots[i], skullCandidateSpots[i], skullBounds, MASK_SOLID, this);
		if (skullTr.fraction >= 1)
		{
			//area is clear.
			skullSpawnPos = skullCandidateSpots[i];
			break;
		}
	}

	//Note: if the clearance check FAILS, it'll just use the head position... fix this if it causes problems.

	if (!spawnArgs.GetBool("do_skullsaver", "1"))
		return;

	gameLocal.SpawnEntityDef(botDef->dict, &skullEnt, false);
	if (skullEnt)
	{
		//Spawned the skullsaver.
		static_cast<idSkullsaver *>(skullEnt)->SetBodyOwner(this);

		skullEnt->SetOrigin(skullSpawnPos);
		idEntityFx::StartFx("fx/skullsaver_spawn", &skullSpawnPos, &mat3_identity, NULL, false);

		static_cast<idSkullsaver*>(skullEnt)->SetSpawnLocOrig(skullSpawnOrigLoc.GetEntity());
	}
	else
	{
		gameLocal.Error("idAI %s failed to spawn '%s'\n", GetName(), spawnArgs.GetString("def_skullsaver", ""));
	}

	RemoveInterestpointsBoundToMe();
}



void idAI::ForceStopDrag()
{
	//if player is dragging this body, then make player drop the body.
	if (gameLocal.GetLocalPlayer()->bodyDragger.isDragging)
	{
		if (gameLocal.GetLocalPlayer()->bodyDragger.dragEnt.IsValid())
		{
			if (gameLocal.GetLocalPlayer()->bodyDragger.dragEnt.GetEntityNum() == this->entityNumber)
			{
				gameLocal.GetLocalPlayer()->bodyDragger.StopDrag(false);
			}
		}
	}
}

//Get how "exposed" the enemy is to me. Checks whether can see left shoulder, right shoulder, and returns the sum of those parts. (2 shoulders = maximum limb value is 2)
int idAI::GetSightExposure(idActor *enemyEnt)
{
	idMat3		myViewAxis;
	idVec3		myGravityDir;
	idVec3		shoulderDir;
	idVec3		enemyPos;
	trace_t		tr;
	int			limbsExposed;

	limbsExposed = 0;
	enemyPos = enemyEnt->GetPhysics()->GetOrigin();
	enemyPos.z = enemyEnt->GetEyePosition().z;

	myViewAxis = (enemyPos - GetEyePosition()).ToAngles().ToForward().ToMat3(); //Get angle of enemy to me.
	myGravityDir = gameLocal.GetLocalPlayer()->GetPhysics()->GetGravityNormal();
	shoulderDir = (myViewAxis[0] - myGravityDir * (myGravityDir * myViewAxis[0])).Cross(myGravityDir); //Get perpendicular angle

	//do a shoulder.
	gameLocal.clip.TracePoint(tr, GetEyePosition(), enemyPos + shoulderDir * 14, MASK_SOLID, this);	
	if (tr.fraction >= 1.0f || tr.c.entityNum == gameLocal.GetLocalPlayer()->entityNumber) { limbsExposed++; }

	//do other shoulder.
	gameLocal.clip.TracePoint(tr, GetEyePosition(), enemyPos + shoulderDir * -14, MASK_SOLID, this);
	if (tr.fraction >= 1.0f || tr.c.entityNum == gameLocal.GetLocalPlayer()->entityNumber) { limbsExposed++; }

	return limbsExposed;
}

bool idAI::InterestPointCheck(idEntity *interestpoint)
{
	return false;
}

void idAI::InterestPointReact(idEntity *interestpoint, int roletype)
{

}

void idAI::ClearInterestPoint()
{

}

idEntityPtr<idEntity> idAI::GetLastInterest()
{
	return idEntityPtr<idEntity>();
}

void idAI::GotoState(int _state)
{
}

// SW
int idAI::GetYieldPriority() const
{
	return yieldPriority;
}

idVec3 idAI::FindValidPosition(idVec3 targetPos)
{
	return vec3_zero;
}

//Am I incapacitated (stunned, suffocating, grappled) or can I receive stimulus
bool idAI::CanAcceptStimulus()
{
	return true;
}

void idAI::RemoveInterestpointsBoundToMe()
{
	//Remove any interestpoints bound to me.
	for (idEntity* entity = gameLocal.interestEntities.Next(); entity != NULL; entity = entity->interestNode.Next())
	{
		if (!entity)
			continue;

		if (entity->GetBindMaster() == NULL)
			continue;

		if (entity->GetBindMaster()->entityNumber != this->entityNumber)
			continue;

		if (entity->IsType(idInterestPoint::Type))
		{
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->InterestPointInvestigated(entity); //delete the interestpoint.
		}
	}
}

moveType_t idAI::GetAIMovetype()
{
	return move.moveType;
}

bool idAI::IsInCombatstate()
{
	return (this->aiState == AISTATE_COMBAT
		|| this->aiState == AISTATE_COMBATOBSERVE
		|| this->aiState == AISTATE_COMBATSTALK
		|| this->aiState == AISTATE_SEARCHING
		|| this->aiState == AISTATE_OVERWATCH);
}

// blendo eric: for calculating hearing through doors and around corners, 0.0f = silent, 1.0f = max vol
float idAI::GetSoundIntensityObstructed(idVec3 soundPos, float soundDistanceMax)
{
	if( soundDistanceMax < 1.0f )
	{
		soundDistanceMax = 1.0f;
	}

	idVec3 listenPos = GetEyePosition();
	float distTraveled = (listenPos - soundPos).Length();
	if(distTraveled > soundDistanceMax)
	{  // out of hearing range early-out
		return 0.0f;
	}

	float distanceAllowed = soundDistanceMax;

	// trace from sound to ai
	bool obstructed = false;
	{
		trace_t results;
		idVec3 start = listenPos;
		idVec3 end = soundPos;
		end[2] += 1.0f;

		gameLocal.clip.TracePoint( results, start, end, MASK_SOLID, this );
		obstructed = results.fraction < 1.0f;
	}

	// use vis portal
	int listenArea = gameRenderWorld->PointInArea(listenPos);
	int soundArea = gameRenderWorld->PointInArea(soundPos);
	bool inArea = listenArea == soundArea;

	if( !obstructed )
	{ // no obstruction, just use unadjusted distance
	}
	else if( inArea )
	{ // in area, use corner obstructing
		distanceAllowed = soundDistanceMax * (1.0f - s_aiMuffleCorner.GetFloat());
	}
	else
	{ // not in area and obstructed, check for door

		const int BLOCKED_BIT_CHECK = ( PS_BLOCK_VIEW | PS_BLOCK_AIR );

		// We use this bool to break out of multiple nested loops at once.
		// Note the '&& !foundSound' inside each loop declaration
		bool foundSound = false;

		// default to corner muffle
		distanceAllowed = soundDistanceMax * (1.0f - s_aiMuffleCorner.GetFloat());

		// STAGE 1: Check our immediate neighbours

		// check neighbor portals to see if sound is in next area
		int numPortals = gameRenderWorld->NumPortalsInArea( listenArea );
		const idWinding * bestPortWindings[3] = {nullptr,nullptr,nullptr};
		for( int p = 0; p < numPortals && !foundSound; p++ )
		{
			exitPortal_t port = gameRenderWorld->GetPortal( listenArea, p );

			if( port.areas[0] == soundArea || port.areas[1] == soundArea )
			{ 
				// We have found the sound in an immediate neighbour
				if(port.blockingBits & BLOCKED_BIT_CHECK)
				{ // replace corner muffle with door muffle
					distanceAllowed = soundDistanceMax * (1.0f - s_aiMuffleDoor.GetFloat());
				}

				bestPortWindings[0] = port.w;
				bestPortWindings[1] = nullptr;
				bestPortWindings[2] = nullptr;
				foundSound = true;
			}
			else
			{
				// STAGE 2: Check the neighbours of our neighbours

				// SW 19th March 2025: Fairly sure this is broken. I think we're supposed to be recursively checking portals in the neighbouring zone,
				// but instead we're just checking the original zone again.
				// I'm going to try and fix this based on what I think it's trying to do.
				
				// One of this portal's areas will be the initial listener area. We want to get the other area and check from there.
				int neighbourArea = (port.areas[0] == listenArea ? port.areas[1] : port.areas[0]);
				int numPortals2 = gameRenderWorld->NumPortalsInArea(neighbourArea);

				for( int p2 = 0; p2 < numPortals2 && !foundSound; p2++ )
				{
					exitPortal_t port2 = gameRenderWorld->GetPortal(neighbourArea, p2 );

					if (port2.portalHandle == port.portalHandle)
					{
						// This is the portal going back into our listener area -- skip
						continue;
					}

					if( port2.areas[0] == soundArea || port2.areas[1] == soundArea )
					{
						// We have found the sound in a neighbour of neighbour
						if(port.blockingBits & BLOCKED_BIT_CHECK)
						{ // replace corner muffle with door muffle
							distanceAllowed = soundDistanceMax * (1.0f - s_aiMuffleDoor.GetFloat());
						}
						if(port2.blockingBits & BLOCKED_BIT_CHECK)
						{ // compound muffle
							distanceAllowed = distanceAllowed * (1.0f - s_aiMuffleDoor.GetFloat());
						}

						bestPortWindings[0] = port.w;
						bestPortWindings[1] = port2.w;
						bestPortWindings[2] = nullptr;
						foundSound = true;
					}
					else
					{
						// SW 20th March 2025: adding a third level of search depth
						// 
						// STAGE 3: Check the neighbours of our neighbours of our neighbours
						// Hello reader. You may be wondering "what the hell is this crap? why not use recursion? isn't there a more elegant way to do this?"
						// The answer is "we are a month away from shipping and overhauling this code carries the risk of introducing fun new bugs"
						// That's gamedev, buckaroo.

						int neighbourOfNeighbourArea = (port2.areas[0] == neighbourArea ? port2.areas[1] : port2.areas[0]);

						if (neighbourOfNeighbourArea != listenArea)
						{
							int numPortals3 = gameRenderWorld->NumPortalsInArea(neighbourOfNeighbourArea);
							for (int p3 = 0; p3 < numPortals3 && !foundSound; p3++)
							{
								exitPortal_t port3 = gameRenderWorld->GetPortal(neighbourOfNeighbourArea, p3);
								if (port3.portalHandle == port2.portalHandle)
								{
									// This is the portal going back into our neighbour area -- skip
									continue;
								}

								if (port3.areas[0] == soundArea || port3.areas[1] == soundArea)
								{
									// We found our sound, neighbour of neighbour of neighbour
									if (port.blockingBits & BLOCKED_BIT_CHECK)
									{ // replace corner muffle with door muffle
										distanceAllowed = soundDistanceMax * (1.0f - s_aiMuffleDoor.GetFloat());
									}
									if (port2.blockingBits & BLOCKED_BIT_CHECK)
									{ // compound muffle
										distanceAllowed = distanceAllowed * (1.0f - s_aiMuffleDoor.GetFloat());
									}
									if (port3.blockingBits & BLOCKED_BIT_CHECK)
									{ // compound muffle
										distanceAllowed = distanceAllowed * (1.0f - s_aiMuffleDoor.GetFloat());
									}

									bestPortWindings[0] = port.w;
									bestPortWindings[1] = port2.w;
									bestPortWindings[2] = port3.w;
									foundSound = true;
								}
							}
						}
						// else: oops we backtracked, skip
					}
				}
			}
		}

		if(!foundSound)
		{ // assume worst case, this code might be bug prone in heavily portald areas?
			return 0.0f;
		}

		// track the distance of the sound to the listener
		distTraveled = 0.0f;
		idVec3 soundPosTravel = soundPos;

		for( int foundIdx = 2; foundIdx >= 0 && distTraveled < distanceAllowed; foundIdx--)
		{
			const idWinding * curWinding = bestPortWindings[foundIdx];
			if( curWinding )
			{
#if 0
				// get closest point on portal
				float closestPortDistSqr = distanceAllowed*distanceAllowed;
				idVec3 closestPortPos = vec3_zero;
				for(int wIdx = 0; wIdx < curWinding->GetNumPoints(); wIdx++)
				{
					idVec3 posCenter = (*curWinding)[wIdx].ToVec3();
					float posDistSqr = (posCenter - soundPosTravel).LengthSqr();
					if( posDistSqr < closestPortDistSqr )
					{
						closestPortDistSqr = posDistSqr;
						closestPortPos = posCenter;
					}
				}
#else
				idVec3 closestPortPos = curWinding->GetCenter();
				float closestPortDistSqr = (closestPortPos - soundPosTravel).LengthSqr(); // SW 19th March 2025: fixing this so it uses the difference instead of absolute portal position
#endif
				if (developer.GetBool())
				{
					gameRenderWorld->DebugArrow(colorBlue, soundPosTravel, closestPortPos, 1, 10000);
				}

				// readjust distance traveled
				float lastDistTravel = idMath::Sqrt(closestPortDistSqr);
				distTraveled += lastDistTravel;
				soundPosTravel = closestPortPos;
			}
		}

		//gameRenderWorld->DebugArrow(colorRed,soundPosTravel,listenPos,1,10000);
		distTraveled += (soundPosTravel - listenPos).LengthFast();
	}

	float intensityPct = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, distTraveled / distanceAllowed );
	return intensityPct;
}

// SW 25th Feb 2025:
// Makes it so ragdolled enemies will be pushed in space
void idAI::UpdateSpacePush()
{
	//if actor is in outer space and is space flailing (or ragdolled), then make it drift toward the 'back' of the ship, to simulate the ship moving through space.
	if (gameLocal.time > outerspaceUpdateTimer)
	{
		#define SPACEPUSH_UPDATEINTERVAL 200
		outerspaceUpdateTimer = gameLocal.time + SPACEPUSH_UPDATEINTERVAL; //how often to update the space check timer.

		if (this->GetBindMaster() != NULL)
			return; //skip if I'm bound to something

		// Only do the push if FTL is active.
		// (This is basically moot since the FTL is always active in this version of the game, but whatever)
		idMeta* meta = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity());
		if (meta)
		{
			idFTL* ftl = static_cast<idFTL*>(meta->GetFTLDrive.GetEntity());
			if (ftl)
			{
				if (!ftl->IsJumpActive(false, false))
				{
					return;
				}
			}
		}

		if (isInOuterSpace() && gameLocal.world->doSpacePush && (af.IsActive() || aiState == AISTATE_SPACEFLAIL))
		{
			#define SPACEPUSH_POWER 8
			idVec3 spacePushPower = idVec3(0, -1, 0); //move toward back of ship.
			spacePushPower = spacePushPower * SPACEPUSH_POWER * this->GetPhysics()->GetMass();
			GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), spacePushPower);
		}
	}
}
