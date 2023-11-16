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

#include "sys/platform.h"
#include "idlib/math/Quat.h"
#include "framework/DeclEntityDef.h"

#include "gamesys/SysCvar.h"
#include "Moveable.h"
#include "Fx.h"
#include "SmokeParticles.h"
#include "Misc.h"
#include "Trigger.h"
#include "WorldSpawn.h"

#include "ai/AI.h"

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
	savefile->WriteInt( (int)moveType );
	savefile->WriteInt( (int)moveCommand );
	savefile->WriteInt( (int)moveStatus );
	savefile->WriteVec3( moveDest );
	savefile->WriteVec3( moveDir );
	goalEntity.Save( savefile );
	savefile->WriteVec3( goalEntityOrigin );
	savefile->WriteInt( toAreaNum );
	savefile->WriteInt( startTime );
	savefile->WriteInt( duration );
	savefile->WriteFloat( speed );
	savefile->WriteFloat( range );
	savefile->WriteFloat( wanderYaw );
	savefile->WriteInt( nextWanderTime );
	savefile->WriteInt( blockTime );
	obstacle.Save( savefile );
	savefile->WriteVec3( lastMoveOrigin );
	savefile->WriteInt( lastMoveTime );
	savefile->WriteInt( anim );
}

/*
=====================
idMoveState::Restore
=====================
*/
void idMoveState::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int &)moveType );
	savefile->ReadInt( (int &)moveCommand );
	savefile->ReadInt( (int &)moveStatus );
	savefile->ReadVec3( moveDest );
	savefile->ReadVec3( moveDir );
	goalEntity.Restore( savefile );
	savefile->ReadVec3( goalEntityOrigin );
	savefile->ReadInt( toAreaNum );
	savefile->ReadInt( startTime );
	savefile->ReadInt( duration );
	savefile->ReadFloat( speed );
	savefile->ReadFloat( range );
	savefile->ReadFloat( wanderYaw );
	savefile->ReadInt( nextWanderTime );
	savefile->ReadInt( blockTime );
	obstacle.Restore( savefile );
	savefile->ReadVec3( lastMoveOrigin );
	savefile->ReadInt( lastMoveTime );
	savefile->ReadInt( anim );
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

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f ), PVSAreas, idEntity::MAX_PVS_AREAS );
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
	blockedMoveTime		= 750;
	blockedAttackTime	= 750;
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

	//ff1.3 start
	lastScriptUpdate			= 0;
	activateTargetsOnDeath		= true;
	focusPosOnlyInRange			= false;
	helltimeMode				= false;
	helltimeAllowed				= false;
	canTouchTriggers			= false;
	forcedMovements				= false;
	meleeDamageScale			= 1.0f;
	projectileDamageScale		= 1.0f;
	helltimeSkin				= NULL;
	postHelltimeSkin			= NULL;
	postHelltimeHeadSkin		= NULL;
	//ff1.3 end
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
}

/*
=====================
idAI::Save
=====================
*/
void idAI::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteInt( travelFlags );
	move.Save( savefile );
	savedMove.Save( savefile );
	savefile->WriteFloat( kickForce );
	savefile->WriteBool( ignore_obstacles );
	savefile->WriteFloat( blockedRadius );
	savefile->WriteInt( blockedMoveTime );
	savefile->WriteInt( blockedAttackTime );

	savefile->WriteFloat( ideal_yaw );
	savefile->WriteFloat( current_yaw );
	savefile->WriteFloat( turnRate );
	savefile->WriteFloat( turnVel );
	savefile->WriteFloat( anim_turn_yaw );
	savefile->WriteFloat( anim_turn_amount );
	savefile->WriteFloat( anim_turn_angles );

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteFloat( fly_speed );
	savefile->WriteFloat( fly_bob_strength );
	savefile->WriteFloat( fly_bob_vert );
	savefile->WriteFloat( fly_bob_horz );
	savefile->WriteInt( fly_offset );
	savefile->WriteFloat( fly_seek_scale );
	savefile->WriteFloat( fly_roll_scale );
	savefile->WriteFloat( fly_roll_max );
	savefile->WriteFloat( fly_roll );
	savefile->WriteFloat( fly_pitch_scale );
	savefile->WriteFloat( fly_pitch_max );
	savefile->WriteFloat( fly_pitch );

	savefile->WriteBool( allowMove );
	savefile->WriteBool( allowHiddenMovement );
	savefile->WriteBool( disableGravity );
	savefile->WriteBool( af_push_moveables );

	savefile->WriteBool( lastHitCheckResult );
	savefile->WriteInt( lastHitCheckTime );
	savefile->WriteInt( lastAttackTime );
	savefile->WriteFloat( melee_range );
	savefile->WriteFloat( projectile_height_to_distance_ratio );

	savefile->WriteInt( missileLaunchOffset.Num() );
	for( i = 0; i < missileLaunchOffset.Num(); i++ ) {
		savefile->WriteVec3( missileLaunchOffset[ i ] );
	}

	idStr projectileName;
	spawnArgs.GetString( "def_projectile", "", projectileName );
	savefile->WriteString( projectileName );
	savefile->WriteFloat( projectileRadius );
	savefile->WriteFloat( projectileSpeed );
	savefile->WriteVec3( projectileVelocity );
	savefile->WriteVec3( projectileGravity );
	projectile.Save( savefile );
	savefile->WriteString( attack );

	savefile->WriteSoundShader( chat_snd );
	savefile->WriteInt( chat_min );
	savefile->WriteInt( chat_max );
	savefile->WriteInt( chat_time );
	savefile->WriteInt( talk_state );
	talkTarget.Save( savefile );

	savefile->WriteInt( num_cinematics );
	savefile->WriteInt( current_cinematic );

	savefile->WriteBool( allowJointMod );
	focusEntity.Save( savefile );
	savefile->WriteVec3( currentFocusPos );
	savefile->WriteInt( focusTime );
	savefile->WriteInt( alignHeadTime );
	savefile->WriteInt( forceAlignHeadTime );
	savefile->WriteAngles( eyeAng );
	savefile->WriteAngles( lookAng );
	savefile->WriteAngles( destLookAng );
	savefile->WriteAngles( lookMin );
	savefile->WriteAngles( lookMax );

	savefile->WriteInt( lookJoints.Num() );
	for( i = 0; i < lookJoints.Num(); i++ ) {
		savefile->WriteJoint( lookJoints[ i ] );
		savefile->WriteAngles( lookJointAngles[ i ] );
	}

	savefile->WriteFloat( shrivel_rate );
	savefile->WriteInt( shrivel_start );

	savefile->WriteInt( particles.Num() );
	for  ( i = 0; i < particles.Num(); i++ ) {
		savefile->WriteParticle( particles[i].particle );
		savefile->WriteInt( particles[i].time );
		savefile->WriteJoint( particles[i].joint );
	}
	savefile->WriteBool( restartParticles );
	savefile->WriteBool( useBoneAxis );

	enemy.Save( savefile );
	savefile->WriteVec3( lastVisibleEnemyPos );
	savefile->WriteVec3( lastVisibleEnemyEyeOffset );
	savefile->WriteVec3( lastVisibleReachableEnemyPos );
	savefile->WriteVec3( lastReachableEnemyPos );
	savefile->WriteBool( wakeOnFlashlight );

	savefile->WriteAngles( eyeMin );
	savefile->WriteAngles( eyeMax );

	savefile->WriteFloat( eyeVerticalOffset );
	savefile->WriteFloat( eyeHorizontalOffset );
	savefile->WriteFloat( eyeFocusRate );
	savefile->WriteFloat( headFocusRate );
	savefile->WriteInt( focusAlignTime );

	savefile->WriteJoint( flashJointWorld );
	savefile->WriteInt( muzzleFlashEnd );

	savefile->WriteJoint( focusJoint );
	savefile->WriteJoint( orientationJoint );
	savefile->WriteJoint( flyTiltJoint );

	savefile->WriteBool( GetPhysics() == static_cast<const idPhysics *>(&physicsObj) );

#ifdef _D3XP
	savefile->WriteInt(funcEmitters.Num());
	for(int i = 0; i < funcEmitters.Num(); i++) {
		funcEmitter_t* emitter = funcEmitters.GetIndex(i);
		savefile->WriteString(emitter->name);
		savefile->WriteJoint(emitter->joint);
		savefile->WriteObject(emitter->particle);
	}

	harvestEnt.Save( savefile);
#endif

	//ff1.3 start
	savefile->WriteInt( lastScriptUpdate ); //ff1.4
	savefile->WriteBool( activateTargetsOnDeath );
	savefile->WriteBool( focusPosOnlyInRange );
	savefile->WriteBool( helltimeMode );
	savefile->WriteBool( helltimeAllowed );
	savefile->WriteBool( canTouchTriggers );
	savefile->WriteBool( forcedMovements );
	savefile->WriteFloat( meleeDamageScale );
	savefile->WriteFloat( projectileDamageScale );
	savefile->WriteSkin( helltimeSkin );
	savefile->WriteSkin( postHelltimeSkin );
	savefile->WriteSkin( postHelltimeHeadSkin );
	//ff1.3 end
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
	idBounds	bounds;

	savefile->ReadInt( travelFlags );
	move.Restore( savefile );
	savedMove.Restore( savefile );
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
	savefile->ReadBool( af_push_moveables );

	savefile->ReadBool( lastHitCheckResult );
	savefile->ReadInt( lastHitCheckTime );
	savefile->ReadInt( lastAttackTime );
	savefile->ReadFloat( melee_range );
	savefile->ReadFloat( projectile_height_to_distance_ratio );

	savefile->ReadInt( num );
	missileLaunchOffset.SetGranularity( 1 );
	missileLaunchOffset.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadVec3( missileLaunchOffset[ i ] );
	}

	idStr projectileName;
	savefile->ReadString( projectileName );
	if ( projectileName.Length() ) {
		projectileDef = gameLocal.FindEntityDefDict( projectileName );
	} else {
		projectileDef = NULL;
	}
	savefile->ReadFloat( projectileRadius );
	savefile->ReadFloat( projectileSpeed );
	savefile->ReadVec3( projectileVelocity );
	savefile->ReadVec3( projectileGravity );
	projectile.Restore( savefile );
	savefile->ReadString( attack );

	savefile->ReadSoundShader( chat_snd );
	savefile->ReadInt( chat_min );
	savefile->ReadInt( chat_max );
	savefile->ReadInt( chat_time );
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

	savefile->ReadBool( wakeOnFlashlight );

	savefile->ReadAngles( eyeMin );
	savefile->ReadAngles( eyeMax );

	savefile->ReadFloat( eyeVerticalOffset );
	savefile->ReadFloat( eyeHorizontalOffset );
	savefile->ReadFloat( eyeFocusRate );
	savefile->ReadFloat( headFocusRate );
	savefile->ReadInt( focusAlignTime );

	savefile->ReadJoint( flashJointWorld );
	savefile->ReadInt( muzzleFlashEnd );

	savefile->ReadJoint( focusJoint );
	savefile->ReadJoint( orientationJoint );
	savefile->ReadJoint( flyTiltJoint );

	savefile->ReadBool( restorePhysics );

	// Set the AAS if the character has the correct gravity vector
	idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
	gravity *= g_gravity.GetFloat();
	if ( gravity == gameLocal.GetGravity() ) {
		SetAAS();
	}

	SetCombatModel();
	LinkCombat();

	InitMuzzleFlash();

	// Link the script variables back to the scriptobject
	LinkScriptVariables();

	if ( restorePhysics ) {
		RestorePhysics( &physicsObj );
	}

#ifdef _D3XP

	//Clean up the emitters
	for(int i = 0; i < funcEmitters.Num(); i++) {
		funcEmitter_t* emitter = funcEmitters.GetIndex(i);
		if(emitter->particle) {
			//Destroy the emitters
			emitter->particle->PostEventMS(&EV_Remove, 0 );
		}
	}
	funcEmitters.Clear();

	int emitterCount;
	savefile->ReadInt( emitterCount );
	for(int i = 0; i < emitterCount; i++) {
		funcEmitter_t newEmitter;
		memset(&newEmitter, 0, sizeof(newEmitter));

		idStr name;
		savefile->ReadString( name );

		strcpy( newEmitter.name, name.c_str() );

		savefile->ReadJoint( newEmitter.joint );
		savefile->ReadObject(reinterpret_cast<idClass *&>(newEmitter.particle));

		funcEmitters.Set(newEmitter.name, newEmitter);
	}

	harvestEnt.Restore(savefile);
	//if(harvestEnt.GetEntity()) {
	//	harvestEnt.GetEntity()->SetParent(this);
	//}

#endif

	//ff1.3 start
	savefile->ReadInt( lastScriptUpdate ); //ff1.4
	savefile->ReadBool( activateTargetsOnDeath );
	savefile->ReadBool( focusPosOnlyInRange );
	savefile->ReadBool( helltimeMode );
	savefile->ReadBool( helltimeAllowed );
	savefile->ReadBool( canTouchTriggers );
	savefile->ReadBool( forcedMovements );
	savefile->ReadFloat( meleeDamageScale );
	savefile->ReadFloat( projectileDamageScale );
	savefile->ReadSkin( helltimeSkin );
	savefile->ReadSkin( postHelltimeSkin );
	savefile->ReadSkin( postHelltimeHeadSkin );
	//ff1.3 end
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

	if ( !g_monsters.GetBool() ) {
		PostEventMS( &EV_Remove, 0 );
		return;
	}

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
	spawnArgs.GetFloat( "blockedRadius",		"-1",		blockedRadius );
	spawnArgs.GetInt( "blockedMoveTime",		"750",		blockedMoveTime );
	spawnArgs.GetInt( "blockedAttackTime",		"750",		blockedAttackTime );

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

	flashJointWorld = animator.GetJointHandle( "flash" );

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

	//ff1.3 start
	if ( team == 0 && spawnArgs.GetBool( "showFriendFx" ) ) {
		StartEmitter( FRIEND_FX_NAME, FRIEND_FX_JOINT, FRIEND_FX_MODEL, false );
	}
	//ff1.3 end

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

	//ff1.3 start

	if ( !spawnArgs.GetBool( "noInfluence" ) ){
		const char *slowmoSkinName;
		if ( spawnArgs.GetString( "skin_spectre", NULL, &slowmoSkinName ) ) {
			helltimeSkin = declManager->FindSkin( slowmoSkinName );
		}
		//black skin fix! - temporary disable slowmo skin
		helltimeAllowed = !spawnArgs.GetBool( "fix_black_skin" );
	}

	canTouchTriggers = spawnArgs.GetBool("touch_triggers", "1");
	forcedMovements = false;
	//ff1.3 end

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
}


#ifdef _D3XP
void idAI::Gib( const idVec3 &dir, const char *damageDefName ) {
	if(harvestEnt.GetEntity()) {
		//Let the harvest ent know that we gibbed
		harvestEnt.GetEntity()->Gib();
	}
	idActor::Gib(dir, damageDefName);
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
void idAI::Think( void ) {
	// if we are completely closed off from the player, don't do anything at all
	if ( CheckDormant() ) {
		return;
	}

	if ( thinkFlags & TH_THINK ) {
		//ff1.3 start
		UpdateAttack();
		//ff1.3 end

		// clear out the enemy when he dies or is hidden
		idActor *enemyEnt = enemy.GetEntity();
		if ( enemyEnt ) {
			if ( enemyEnt->health <= 0 ) {
				EnemyDead();
			}

			//ff1.3 start
			else if ( team == 0 && enemyEnt->team == 0 ) {
				gameLocal.Warning("%s ignored teammate %s", GetName(), enemyEnt->GetName());
				EnemyDead();
			}
			//ff1.3 end
		}
		//ff1.3 start - handle instant removal of enemies (e.g. in cinematics)
		else if ( enemy.GetSpawnId() != 0 ) {
			gameLocal.Warning("%s's enemy has been removed", GetName());
			EnemyDead();
		}
		//ff1.3 end

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
			case MOVETYPE_DEAD :
				// dead monsters
				UpdateAIScript();
				DeadMove();
				break;

			case MOVETYPE_FLY :
				// flying monsters
				UpdateEnemyPosition();
				UpdateAIScript();
				FlyMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_STATIC :
				// static monsters
				UpdateEnemyPosition();
				UpdateAIScript();
				StaticMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_ANIM :
				// animation based movement
				UpdateEnemyPosition();
				UpdateAIScript();
				AnimMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_SLIDE :
				// velocity based movement
				UpdateEnemyPosition();
				UpdateAIScript();
				SlideMove();
				PlayChatter();
				CheckBlink();
				break;
			}
		}

		//ff1.3 start
		if ( health > 0 ) {
			UpdateHelltimeMode( gameLocal.IsSlowmoActive() );
		}
		//ff1.3 end

		// clear pain flag so that we recieve any damage between now and the next time we run the script
		AI_PAIN = false;
		AI_SPECIAL_DAMAGE = 0;
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

	UpdateMuzzleFlash();
	UpdateAnimation();
	UpdateParticles();
	Present();
	UpdateDamageEffects();
	LinkCombat();

#ifdef _D3XP
	if(ai_showHealth.GetBool()) {
		idVec3 aboveHead(0,0,20);
		gameRenderWorld->DrawText( va( "%d", ( int )health), this->GetEyePosition()+aboveHead, 0.5f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
	}
#endif
}

//ff1.3 start
/*
=====================
idAI::UpdateHelltimeMode
=====================
*/
void idAI::UpdateHelltimeMode( bool on ) {

	if ( !helltimeAllowed ) { //this fixes the 'black skin' - TODO: fix it properly
		if ( !AI_ACTIVATED && !AI_PAIN && !enemy.GetEntity() ) {
			//return;
		} else {
			//gameLocal.Warning("AI_ACTIVATED 'black skin' bug\n");
			helltimeAllowed = true;
		}
	}

	if ( helltimeSkin && on != helltimeMode ) {

		//gameLocal.Warning("avoid 'black skin' bug %d\n", animator.NumFrames( ANIMCHANNEL_TORSO ) );

		if ( helltimeMode ) {
			helltimeMode = false;
			renderEntity.customSkin = postHelltimeSkin;
			postHelltimeSkin = NULL;

			if ( head.GetEntity() ) {
				head.GetEntity()->GetRenderEntity()->customSkin = postHelltimeHeadSkin;
			}
			postHelltimeHeadSkin = NULL;
		} else if ( helltimeAllowed ) {
			helltimeMode = true;
			postHelltimeSkin = renderEntity.customSkin;
			renderEntity.customSkin = helltimeSkin;

			if ( head.GetEntity() ) {
				postHelltimeHeadSkin = head.GetEntity()->GetRenderEntity()->customSkin;
				head.GetEntity()->GetRenderEntity()->customSkin = helltimeSkin;
			}

			//BecomeActive( TH_PHYSICS );TH_ANIMATE
			//UpdateModel();
		}

		//BecomeActive( TH_UPDATEVISUALS ); //TH_UPDATEVISUALS
		//animator.ForceUpdate();
		//UpdateVisuals();
		//UpdateModel();

		//gameLocal.Printf("test...\n");

		/*
		if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
			gameLocal.Printf("!( thinkFlags & TH_UPDATEVISUALS )\n");
		}

		// don't do animations if they're not enabled
		if ( !( thinkFlags & TH_ANIMATE ) ) {
			gameLocal.Printf("!( thinkFlags & TH_ANIMATE )\n");
		}

		if ( !animator.IsAnimating( gameLocal.time ) ) {
			gameLocal.Printf("!animator.IsAnimating()\n");
		}


		// is the model an MD5?
		if ( !animator.ModelHandle() ) {
			// no, so nothing to do
			gameLocal.Printf("!animator.ModelHandle()\n");
		}

		// if the model is animating then we have to update it
		if ( !animator.FrameHasChanged( gameLocal.time ) ) {
			// still fine the way it was
			gameLocal.Printf("!animator.FrameHasChanged( gameLocal.time )\n");
		}
		*/

	}
}
//ff1.3 end

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
}

/*
=====================
idAI::UpdateAIScript
=====================
*/
void idAI::UpdateAIScript( void ) {
	//ff1.3 start - fix script called twice by triggering a trigger having myself as target
	if ( lastScriptUpdate == gameLocal.time ) {
		return;
	}
	lastScriptUpdate = gameLocal.time;
	//ff1.3 end

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
	if(enemyEnt->GetCurrentVehicle()){
		move.goalEntity	= enemyEnt->GetCurrentVehicle();
	}else{
		move.goalEntity	= enemyEnt;
	}
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
	if ( GetBindMaster() || ReachedPos( pos, move.moveCommand ) ) {
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

	if ( GetBindMaster() || !enemyEnt || ( move.moveType != MOVETYPE_FLY ) ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	if(enemyEnt->GetCurrentVehicle()){
		move.moveDest.z		= lastVisibleEnemyPos.z + enemyEnt->GetCurrentVehicle()->EyeOffset().z + fly_offset;
		move.goalEntity		= enemyEnt;
	}else{
		move.moveDest.z		= lastVisibleEnemyPos.z + enemyEnt->EyeOffset().z + fly_offset;
		move.goalEntity		= enemyEnt;
	}

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

	if ( GetBindMaster() || !enemyEnt ) {
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
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, pos ) ) {
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
		move.startTime		= gameLocal.time;
	}

	move.moveDest		= pos;
	if(enemyEnt->GetCurrentVehicle()){
		move.goalEntity	= enemyEnt->GetCurrentVehicle();
	}else{
		move.goalEntity	= enemyEnt;
	}
	move.speed			= fly_speed;
	move.moveStatus		= MOVE_STATUS_MOVING;
	AI_MOVE_DONE		= false;
	AI_DEST_UNREACHABLE = false;
	AI_FORWARD			= true;

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

	if ( GetBindMaster() || !ent ) {
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
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, pos ) ) {
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

	if ( GetBindMaster() || !aas || !ent ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );

	// consider the entity the monster is getting close to as an obstacle
	obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();

	idActor	*enemyEnt = enemy.GetEntity();
	if ( ent == enemyEnt || (enemyEnt && enemyEnt->GetCurrentVehicle() && ent == enemyEnt->GetCurrentVehicle()) ) {
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

	if ( GetBindMaster() || !aas || !ent ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	const idVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );

	// consider the entity the monster is getting close to as an obstacle
	obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();

	idActor	*enemyEnt = enemy.GetEntity();
	if ( ent == enemyEnt || (enemyEnt && enemyEnt->GetCurrentVehicle() && ent == enemyEnt->GetCurrentVehicle()) ) {
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

	if ( GetBindMaster() ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		AI_DEST_UNREACHABLE = true;
		return false;
	}

	if ( ReachedPos( pos, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	org = pos;
	move.toAreaNum = 0;
	if ( aas ) {
		move.toAreaNum = PointReachableAreaNum( org );
		aas->PushPointIntoAreaNum( move.toAreaNum, org );

		areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), move.toAreaNum, org ) ) {
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
			AI_DEST_UNREACHABLE = true;
			return false;
		}
	}

	if ( !move.toAreaNum && !NewWanderDir( org ) ) {
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

	if ( GetBindMaster() || !aas || !entity ) {
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

	if ( ReachedPos( hideGoal.origin, move.moveCommand ) ) {
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

	move.moveDest = physicsObj.GetOrigin() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 256.0f;
	if ( GetBindMaster() || !NewWanderDir( move.moveDest ) ) {
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

		if ( tdir != turnaround && StepDirection( tdir ) ) {
			return true;
		}
	}

	// try other directions
	if ( ( gameLocal.random.RandomInt() & 1 ) || idMath::Fabs( deltay ) > idMath::Fabs( deltax ) ) {
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

	idEntity *testEnt;
	if( actor->GetCurrentVehicle() ){
		testEnt = actor->GetCurrentVehicle();
		eye = actorOrigin + actor->GetCurrentVehicle()->EyeOffset();
	}else{
		testEnt = actor;
		eye = actorOrigin + actor->EyeOffset();
	}

	point = pos;
	point[2] += 1.0f;

	physicsObj.DisableClip();

	gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, testEnt );
	if ( results.fraction >= 1.0f || ( gameLocal.GetTraceEntity( results ) == this ) ) {
		physicsObj.EnableClip();
		return true;
	}

	const idBounds &bounds = physicsObj.GetBounds();
	point[2] += bounds[1][2] - bounds[0][2];

	gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, testEnt );
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
	idActor	*enemyEnt = enemy.GetEntity();
	if( enemyEnt && enemyEnt->GetCurrentVehicle() ){
		foundPath = FindPathAroundObstacles( &physicsObj, aas, enemyEnt->GetCurrentVehicle(), origin, goalPos, path );
	}else{
		foundPath = FindPathAroundObstacles( &physicsObj, aas, enemyEnt, origin, goalPos, path );
	}
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

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS ){
		move.lastMoveOrigin.Zero();
		move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;
	if ( ( move.moveCommand == MOVE_FACE_ENEMY ) && enemy.GetEntity() ) {
		TurnToward( lastVisibleEnemyPos );
		goalPos = oldorigin;
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		goalPos = oldorigin;
	} else if ( GetMovePos( goalPos ) ) {
		if ( move.moveCommand != MOVE_WANDER ) {
			CheckObstacleAvoidance( goalPos, newDest );
			TurnToward( newDest );
		} else {
			TurnToward( goalPos );
		}
	}

	Turn();

	if ( move.moveCommand == MOVE_SLIDE_TO_POSITION ) {
		if ( gameLocal.time < move.startTime + move.duration ) {
			goalPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
			delta = goalPos - oldorigin;
			delta.z = 0.0f;
		} else {
			delta = move.moveDest - oldorigin;
			delta.z = 0.0f;
			StopMove( MOVE_STATUS_DONE );
		}
	} else if ( allowMove ) {
		GetMoveDelta( oldaxis, viewAxis, delta );
	} else {
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
		idActor *enemyEnt = enemy.GetEntity();
		if( enemyEnt && enemyEnt->GetCurrentVehicle() ){
			DirectDamage( attack, enemyEnt->GetCurrentVehicle() );
			DamageEffect( attack, enemyEnt->GetCurrentVehicle(), GetEyePosition(), melee_range ); //ff1.3
		}else{
			DirectDamage( attack, enemyEnt );
			DamageEffect( attack, enemyEnt, GetEyePosition(), melee_range ); //ff1.3
		}
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		}
	}

	BlockedFailSafe();

	AI_ONGROUND = physicsObj.OnGround();

	idVec3 org = physicsObj.GetOrigin();
	//ff1.3 start
	/* was
	if ( oldorigin != org ) {
		TouchTriggers();
	}
	*/
	if ( canTouchTriggers && ( forcedMovements || oldorigin != org ) ) {
		TouchTriggers();
	}
	//ff1.3 end

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
		idActor *enemyEnt = enemy.GetEntity();
		if( enemyEnt && enemyEnt->GetCurrentVehicle() ){
			DirectDamage( attack, enemyEnt->GetCurrentVehicle() );
			DamageEffect( attack, enemyEnt->GetCurrentVehicle(), GetEyePosition(), melee_range ); //ff1.3
		}else{
			DirectDamage( attack, enemyEnt );
			DamageEffect( attack, enemyEnt, GetEyePosition(), melee_range ); //ff1.3
		}
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		}
	}

	BlockedFailSafe();

	AI_ONGROUND = physicsObj.OnGround();

	idVec3 org = physicsObj.GetOrigin();
	//ff1.3 start
	/* was
	if ( oldorigin != org ) {
		TouchTriggers();
	}
	*/
	if ( canTouchTriggers && ( forcedMovements || oldorigin != org ) ) {
		TouchTriggers();
	}
	//ff1.3 end

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
	//idActor			*enemyEnt;
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

		if ( enemy.GetEntity() ) {
			end.z = lastVisibleEnemyPos.z + lastVisibleEnemyEyeOffset.z + fly_offset;
		}
		else if ( ( team == 0 ) && ( move.moveCommand == MOVE_TO_ENTITY ) && ( move.goalEntity.GetEntity() ) ) { //ff1.3
			end.z = move.goalEntity.GetEntity()->GetPhysics()->GetOrigin().z + FRIEND_FLY_OFFSET;
		}
		/*
		else if(team == 0){
			//end.z = gameLocal.GetLocalPlayer()->GetEyePosition().z;
		}
		*/
		else {
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
	idVec3	newDest;

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
			CheckObstacleAvoidance( goalPos, newDest );
			goalPos = newDest;
		}

		if ( move.speed	) {
			FlySeekGoal( vel, goalPos );
		}

		// add in bobbing
		AddFlyBob( vel );

		if ( !forcedMovements ) { //ff1.3
			if ( enemy.GetEntity() && ( move.moveCommand != MOVE_TO_POSITION ) ) {
				AdjustFlyHeight( vel, goalPos );
			}
			else if ( ( team == 0 ) && ( move.moveCommand == MOVE_TO_ENTITY ) ) { //ff1.3 following player
				AdjustFlyHeight( vel, goalPos );
			}
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
		idActor *enemyEnt = enemy.GetEntity();
		if( enemyEnt && enemyEnt->GetCurrentVehicle() ){
			DirectDamage( attack, enemyEnt->GetCurrentVehicle() );
			DamageEffect( attack, enemyEnt->GetCurrentVehicle(), GetEyePosition(), melee_range ); //ff1.3
		}else{
			DirectDamage( attack, enemyEnt );
			DamageEffect( attack, enemyEnt, GetEyePosition(), melee_range ); //ff1.3
		}
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
	//ff1.3 start
	/* was
	if ( oldorigin != org ) {
		TouchTriggers();
	}
	*/
	if ( canTouchTriggers && ( forcedMovements || oldorigin != org ) ) {
		TouchTriggers();
	}
	//ff1.3 end

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
		if( enemyEnt && enemyEnt->GetCurrentVehicle() ){
			DirectDamage( attack, enemyEnt->GetCurrentVehicle() );
			DamageEffect( attack, enemyEnt->GetCurrentVehicle(), GetEyePosition(), melee_range ); //ff1.3
		}else{
			DirectDamage( attack, enemyEnt );
			DamageEffect( attack, enemyEnt, GetEyePosition(), melee_range ); //ff1.3
		}
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
int idAI::ReactionTo( const idActor *actor ) {
	if( team == 0 && actor->team == 0 ){
		// team 0 don't attack each other
		return ATTACK_IGNORE;
	}

	if ( actor->fl.hidden && ( !actor->GetCurrentVehicle() || actor->GetCurrentVehicle()->fl.hidden ) ) {
		// ignore hidden entities
		return ATTACK_IGNORE;
	}

	/*
	if ( !ent->IsType( idActor::Type ) ) {
		return ATTACK_IGNORE;
	}
	*/

	//const idActor *actor = static_cast<const idActor *>( ent );
	if ( actor->IsType( idPlayer::Type ) && static_cast<const idPlayer *>(actor)->noclip ) {
		// ignore players in noclip mode
		return ATTACK_IGNORE;
	}

	// actors on different teams will always fight each other
	if ( actor->team != team ) {
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


/*
=====================
idAI::Pain
=====================
*/
bool idAI::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idActor	*actor;

	AI_PAIN = idActor::Pain( inflictor, attacker, damage, dir, location );
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

		if ( enemy.GetEntity() != attacker && attacker->IsType( idActor::Type ) ) { //TODO: add support for vehicles?
			//gameLocal.Printf("idAI::Pain from %s\n", attacker->GetName() ); //test
			actor = ( idActor * )attacker;
			if ( ReactionTo( actor ) & ATTACK_ON_DAMAGE ) {
				gameLocal.AlertAI( actor, actor->GetPhysics()->GetOrigin() );
				SetEnemy( actor );
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
		gameLocal.smokeParticles->EmitSmoke( pe.particle, pe.time, gameLocal.random.CRandomFloat(), origin, axis, timeGroup /*_D3XP*/ );
	}

	return pe.particle;
}

//ff1.4 start
/*
================
idAI::UnbindEntityOnDeath
================
*/
void idAI::UnbindEntityOnDeath( void ) {
	const char *boundEntName;
	idEntity *boundEnt;

	if ( spawnArgs.GetString( "unbindondeath", "", &boundEntName ) ) {
		boundEnt = gameLocal.FindEntity( boundEntName );
		if ( boundEnt && boundEnt->GetBindMaster() == this ) {
			boundEnt->Unbind();
		}
	}
}
//ff1.4 end

/*
=====================
idAI::Killed
=====================
*/
void idAI::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idAngles ang;
	const char *modelDeath;

	//ff1.3 start
	if ( this == gameLocal.lastAICoopEnemy.GetEntity() ) {
		gameLocal.lastAICoopEnemy = NULL;
	}
	idActor::Killed(inflictor, attacker, damage, dir, location); //stop dmgfx
	//ff1.3 end

	// make sure the monster is activated
	EndAttack();

	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s'\n", animator.GetJointName( ( jointHandle_t )location ),
			GetDamageGroup( location ) );
	}

	//ff1.3 start
	/*
	if ( inflictor ) {
		AI_SPECIAL_DAMAGE = inflictor->spawnArgs.GetInt( "special_damage" );
	} else {
		AI_SPECIAL_DAMAGE = 0;
	}
	*/

	int special_damage = ( inflictor ) ? inflictor->spawnArgs.GetInt( "special_damage" ) : 0;
	AI_SPECIAL_DAMAGE = special_damage;

	UpdateFriendFx(); //stop fx
	//ff1.3 end

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
	if ( spawnArgs.GetBool( "staticDeath" ) ) { //ff1.3 - turrets fix
		move.moveType = MOVETYPE_STATIC;
	}else{
		move.moveType = MOVETYPE_DEAD;
	}

	af_push_moveables = false;

	physicsObj.UseFlyMove( false );
	physicsObj.ForceDeltaMove( false );

	// end our looping ambient sound
	StopSound( SND_CHANNEL_AMBIENT, false );

	if ( attacker && attacker->IsType( idActor::Type ) ) {
		gameLocal.AlertAI( ( idActor * )attacker, attacker->GetPhysics()->GetOrigin() );
	}

	//ff1.3 start
	UpdateHelltimeMode(false);

	// activate targets
	//was: ActivateTargets( attacker );
	if ( activateTargetsOnDeath ) { //false for dynamic friends and demon possession
		ActivateTargets( attacker );
	}
	//ff1.3 end

	RemoveAttachments();
	RemoveProjectile();
	StopMove( MOVE_STATUS_DONE );

	ClearEnemy();
	AI_DEAD	= true;

	// make monster nonsolid
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();

	Unbind();

	//ff1.4 start
	UnbindEntityOnDeath();
	//ff1.4 end

	if ( StartRagdoll() ) {
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );

		//ff1.3 start - disintegrate ragdolls for BFG
		if ( special_damage == 2 ) {
			//gameLocal.Printf("SetThrown\n");
			SetThrown( true ); //disable collisions with other corpses
			spawnArgs.SetInt( "burnaway", 0 ); //disable burnaway //TODO: do this properly?
			Event_Disintegrate();
		}
		//ff1.3 end
	}

	if ( spawnArgs.GetString( "model_death", "", &modelDeath ) ) {
		// lost soul is only case that does not use a ragdoll and has a model_death so get the death sound in here
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		SetModel( modelDeath );
		physicsObj.SetLinearVelocity( vec3_zero );
		physicsObj.PutToRest();
		physicsObj.DisableImpact();
#ifdef _D3XP
		// No grabbing if "model_death"
		noGrab = true;
#endif
		//ff1.3 start- remove dmgfxs on model_death (usually a prt)
		RemoveDamageFxs();
		allowDmgfxs = false;
		//ff1.3 end
	}

	restartParticles = false;

	state = GetScriptFunction( "state_Killed" );
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

#ifndef _D3XP
	if ( ( attacker && attacker->IsType( idPlayer::Type ) ) && ( inflictor && !inflictor->IsType( idSoulCubeMissile::Type ) ) ) {
		static_cast< idPlayer* >( attacker )->AddAIKill();
	}
#endif

#ifdef _D3XP
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
#endif
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
	idActor *actor;
	//gameLocal.Printf("idAI::Activate %s", activator != NULL ? activator->GetName() : "null" );

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

		/*
		if ( !activator || !activator->IsType( idPlayer::Type ) ) {
			player = gameLocal.GetLocalPlayer();
		} else {
			player = static_cast<idPlayer *>( activator );
		}
		*/

		//find player
		if( activator && activator->IsType( idPlayer::Type ) ) {
			player = static_cast<idPlayer *>( activator );
		} else if( activator && activator->IsType( idAFEntity_Vehicle::Type ) ) {
			player = static_cast<idAFEntity_Vehicle *>( activator )->GetDriver();
			if ( !player ){ //vehicle without driver
				player = gameLocal.GetLocalPlayer();
			}
		} else if( activator && activator->IsType( idAI_Rideable::Type ) ) {
			player = static_cast<idAI_Rideable *>( activator )->GetDriver();
			if ( !player ){ //ai without driver
				player = gameLocal.GetLocalPlayer();
			}
		} else {
			player = gameLocal.GetLocalPlayer();
		}

		//find enemy
		if( player->GetCurrentRiddenAI() ){
			actor = player->GetCurrentRiddenAI();
		}else{
			actor = player;
		}

		if ( ReactionTo( actor ) & ATTACK_ON_ACTIVATE ) {
			SetEnemy( actor );
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
		//ff1.3 start: activate ai by talking
		if ( !AI_ACTIVATED ) {
			AI_ACTIVATED = true;
		}
		AI_TALK = true;
		//ff1.3 end
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
	int			lastVisibleReachableEnemyAreaNum;
	aasPath_t	path;
	idVec3		pos;
	bool		onGround;

	if ( !enemyEnt ) {
		return;
	}

	lastVisibleReachableEnemyPos = lastReachableEnemyPos;
	if( enemyEnt->GetCurrentVehicle() ){
		lastVisibleEnemyEyeOffset = enemyEnt->GetCurrentVehicle()->EyeOffset();
		lastVisibleEnemyPos = enemyEnt->GetCurrentVehicle()->GetPhysics()->GetOrigin();
	}else{
		lastVisibleEnemyEyeOffset = enemyEnt->EyeOffset();
		lastVisibleEnemyPos = enemyEnt->GetPhysics()->GetOrigin();
	}
	if ( move.moveType == MOVETYPE_FLY ) {
		pos = lastVisibleEnemyPos;
		onGround = true;
	} else {
		if( enemyEnt->GetCurrentVehicle() ){
			onGround = enemyEnt->GetCurrentVehicle()->GetFloorPos( 64.0f, pos );
		}else{
			onGround = enemyEnt->GetFloorPos( 64.0f, pos );
			if ( enemyEnt->OnLadder() ) {
				onGround = false;
			}
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
			if( enemyEnt->GetCurrentVehicle() ){
				end.z += enemyEnt->GetCurrentVehicle()->EyeOffset().z + fly_offset;
			}else{
				end.z += enemyEnt->EyeOffset().z + fly_offset;
			}
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

	if ( !enemyEnt ) {
		return;
	}

	const idVec3 &org = physicsObj.GetOrigin();

	idEntity *testEntity;
	if( enemyEnt->GetCurrentVehicle() ){
		testEntity = enemyEnt->GetCurrentVehicle();
	}else{
		testEntity = enemyEnt;
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		enemyPos = testEntity->GetPhysics()->GetOrigin();
		onGround = true;
	} else {
		if( enemyEnt->GetCurrentVehicle() ){
			onGround = enemyEnt->GetCurrentVehicle()->GetFloorPos( 64.0f, enemyPos );
		}else{
			onGround = enemyEnt->GetFloorPos( 64.0f, enemyPos );
			if ( enemyEnt->OnLadder() ) {
				onGround = false;
			}
		}
	}

	if ( onGround ) {
		// when we don't have an AAS, we can't tell if an enemy is reachable or not,
		// so just assume that he is.
		if ( !aas ) {
			enemyAreaNum = 0;
			lastReachableEnemyPos = enemyPos;
		} else {
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

	if ( CanSee( testEntity, false ) ) {
		AI_ENEMY_VISIBLE = true;
		if ( CheckFOV( testEntity->GetPhysics()->GetOrigin(), fovDot ) ) { //ff1.3 - fovDot added
			AI_ENEMY_IN_FOV = true;
		}

		SetEnemyPosition();
	} else {
		// check if we heard any sounds in the last frame
		if ( enemyEnt == gameLocal.GetAlertEntity() ) {
			float dist = ( testEntity->GetPhysics()->GetOrigin() - org ).LengthSqr();
			if ( dist < Square( AI_HEARING_RANGE ) ) {
				SetEnemyPosition();
			}
		}
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorLtGrey, enemyEnt->GetPhysics()->GetBounds(), lastReachableEnemyPos, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorWhite, enemyEnt->GetPhysics()->GetBounds(), lastVisibleReachableEnemyPos, gameLocal.msec );
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

		//ff1.3 start
		if ( team == 0 ) {
			gameLocal.lastAICoopEnemy = newEnemy;
		}
		//ff1.3 end
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
bool idAI::GetAimDir( const idVec3 &firePos, idEntity *aimAtEnt, const idEntity *ignore, idVec3 &aimDir, bool useLastVisiblePos ) const {
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
	idActor * enemyEnt = enemy.GetEntity();
	if ( useLastVisiblePos && aimAtEnt == enemyEnt ) {
		if ( enemyEnt->GetCurrentVehicle() ){
			enemyEnt->GetCurrentVehicle()->GetAIAimTargets( lastVisibleEnemyPos, targetPos1, targetPos2 );
		} else {
			enemyEnt->GetAIAimTargets( lastVisibleEnemyPos, targetPos1, targetPos2 );
		}
	} else if ( aimAtEnt->IsType( idActor::Type ) ) {
		idActor * actor = static_cast<idActor *>( aimAtEnt );
		if ( actor->GetCurrentVehicle() ) {
			actor->GetCurrentVehicle()->GetAIAimTargets( actor->GetCurrentVehicle()->GetPhysics()->GetOrigin(), targetPos1, targetPos2 );
		} else {
			actor->GetAIAimTargets( actor->GetPhysics()->GetOrigin(), targetPos1, targetPos2 );
		}
	} else if ( aimAtEnt->IsType( idAFEntity_Vehicle::Type ) ) {
		static_cast<idAFEntity_Vehicle *>( aimAtEnt )->GetAIAimTargets( aimAtEnt->GetPhysics()->GetOrigin(), targetPos1, targetPos2 );
	} else {
		targetPos1 = aimAtEnt->GetPhysics()->GetAbsBounds().GetCenter();
		targetPos2 = targetPos1;
	}

/* ff1.3 - not needed anymore
#ifdef _D3XP
	if ( this->team == 0 && !idStr::Cmp( aimAtEnt->GetEntityDefName(), "monster_demon_vulgar" ) ) {
		targetPos1.z -= 28.f;
		targetPos2.z -= 12.f;
	}
#endif
*/

	// try aiming for chest
	delta = firePos - targetPos1;
	max_height = delta.LengthFast() * projectile_height_to_distance_ratio;
	result = PredictTrajectory( firePos, targetPos1, projectileSpeed, projectileGravity, projectileClipModel, MASK_SHOT_RENDERMODEL, max_height, ignore, aimAtEnt, ai_debugTrajectory.GetBool() ? 1000 : 0, aimDir );
	if ( result || !aimAtEnt->IsType( idActor::Type ) ) { //TODO: add vehicle check for targetPos2?
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
#ifdef _D3XP
	//idMat3				proj_axis;
	bool				forceMuzzle;
#endif
	idVec3				tmp;
	idProjectile		*lastProjectile;

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	num_projectiles = spawnArgs.GetInt( "num_projectiles", "1" );
#ifdef _D3XP
	forceMuzzle = spawnArgs.GetBool( "forceMuzzle", "0" );
#endif

	GetMuzzle( jointname, muzzle, axis );

	if ( !projectile.GetEntity() ) {
		CreateProjectile( muzzle, axis[ 0 ] );
	}

	lastProjectile = projectile.GetEntity();

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

#ifdef _D3XP
	//proj_axis = axis;
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
	for( i = 0; i < num_projectiles; i++ ) {
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
		//ff1.3 - was: lastProjectile->Launch( muzzle, dir, vec3_origin );
		lastProjectile->Launch( muzzle, dir, vec3_origin, 0.0f, 1.0f, projectileDamageScale );
		projectile = NULL;
	}

	TriggerWeaponEffects( muzzle );

	lastAttackTime = gameLocal.time;

	return lastProjectile;
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
	} else{
		idActor *enemyEnt = enemy.GetEntity();
		if ( victim == enemyEnt || (enemyEnt && victim == enemyEnt->GetCurrentVehicle()) ) {
			AI_HIT_ENEMY = true;
		}
	}
}

//ff1.3 start
/*
=====================
idAI::DamageEffect
=====================
*/
void idAI::DamageEffect( const char *meleeDefName, idEntity *ent, const idVec3 &start, float range ) {
	trace_t			trace;
	//idVec3			start;
	idVec3			end;
	idVec3			damageFxDir;

	//damage effect - could work for any entity but just add blood on actors for now
	if ( ent->IsType( idAI::Type ) ) {
		//start = GetEyePosition();
		if ( !static_cast<idActor *>(ent)->GetInsideBodyAimTarget(end) ) {
			end = ent->GetPhysics()->GetAbsBounds().GetCenter();
		}
		damageFxDir = end - start;
		damageFxDir.Normalize();
		end = start + damageFxDir * (range + 50.0f);

		if ( g_debugDamage.GetBool() ) {
			gameRenderWorld->DebugLine( colorBlue, start, end, 5000 );
		}

		gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
		if ( ( trace.fraction < 1.0f ) && ( gameLocal.GetTraceEntity( trace ) == ent ) ) {
			//gameLocal.Printf("AddDamageEffect\n");
			ent->AddDamageEffect( trace, damageFxDir, meleeDefName, this );
		}
	}
}
//ff1.3 end

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

	ent->Damage( this, this, globalKickDir, meleeDefName, meleeDamageScale /*was: 1.0f*/, INVALID_JOINT );
	//DamageEffect( meleeDefName, ent, GetEyePosition(), melee_range ); //ff1.3

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

	idAFEntity_Vehicle *enemyVehicle = enemyEnt->GetCurrentVehicle();

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

	idVec3 enemyOrg;
	idBounds enemyBounds;
	if( enemyVehicle ){
		enemyOrg = enemyVehicle->GetPhysics()->GetOrigin();
		enemyBounds = enemyVehicle->GetPhysics()->GetBounds();
	}else{
		enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
		enemyBounds = enemyEnt->GetPhysics()->GetBounds();
	}
	enemyBounds.TranslateSelf( enemyOrg );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorYellow, bounds, vec3_zero, gameLocal.msec );
	}

	if ( !bounds.IntersectsBounds( enemyBounds ) ) {
		return false;
	}

	idVec3 start = GetEyePosition();
	idVec3 end = (enemyVehicle) ? enemyVehicle->GetEyePosition() : enemyEnt->GetEyePosition();

	idEntity *testEnt;
	if( enemyVehicle ){
		testEnt = enemyVehicle;
	}else{
		testEnt = enemyEnt;
	}

	gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
	if ( ( trace.fraction == 1.0f ) || ( gameLocal.GetTraceEntity( trace ) == testEnt ) ) {
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

	idEntity *victimEnt;
	if( enemyEnt && enemyEnt->GetCurrentVehicle() ){
		victimEnt = enemyEnt->GetCurrentVehicle();
	}else{
		victimEnt = enemyEnt;
	}

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown melee '%s'", meleeDefName );
	}

	if ( !victimEnt ) {
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
	if ( victimEnt->IsType( idPlayer::Type ) && g_skill.GetInteger() < 2 ) { //this won't be done for vehicles
		int	damage, armor;
		idPlayer *player = static_cast<idPlayer*>( victimEnt );
		player->CalcDamagePoints( this, this, meleeDef, meleeDamageScale /*was: 1.0f*/, INVALID_JOINT, &damage, &armor );

		if ( victimEnt->health <= damage ) {
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
	}

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

	victimEnt->Damage( this, this, globalKickDir, meleeDefName, meleeDamageScale /*was: 1.0f*/, INVALID_JOINT );
	DamageEffect( meleeDefName, victimEnt, GetEyePosition(), melee_range ); //ff1.3

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
			if ( attack.Length() && ( ent->IsType( idActor::Type ) || ent->IsType( idAFEntity_Vehicle::Type ) ) ) {
				ent->Damage( this, this, vel, attack, meleeDamageScale /*1.0f*/, INVALID_JOINT );
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

	//ff1.3 start
	UpdateFriendFx(); //restore hidden fx
	//ff1.3 end
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
/* ff1.1 - moved to idAnimatedEntity
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
*/

idFuncEmitter* idAI::StartEmitter( const char* name, const char* joint, const char* particle, bool orientated ) {

	idFuncEmitter* existing = GetEmitter(name);
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
	newEmitter.particle->BindToJoint(this, jointNum, orientated); //was: true
	newEmitter.particle->BecomeActive(TH_THINK);
	if ( IsHidden() ) { //ff1.3 - start hidden if the monster is hidden
		newEmitter.particle->Hide();
	} else {
		newEmitter.particle->Show();
	}
	newEmitter.particle->PostEventMS(&EV_Activate, 0, this);
	return newEmitter.particle;
}

idFuncEmitter* idAI::GetEmitter( const char* name ) {
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
	if( emitter ) {
		emitter->particle->Unbind();
		emitter->particle->PostEventMS( &EV_Remove, 0 );
		funcEmitters.Remove(name);
	}
}

//ff1.3 start
/*
void idAI::SetEmitterVisible( const char* name, bool on ) {
	funcEmitter_t* emitter;
	funcEmitters.Get(name, &emitter);
	if ( emitter && emitter->particle->IsEmitterHidden() == on ) {
		//slave->CancelEvents( &EV_Mover_ReturnToPos1 );
		emitter->particle->PostEventMS(&EV_Activate, 0, this);
	}
}
*/

void idAI::ChangeTeam( int newTeam ){
	if ( team != newTeam ) {
		EnemyDead(); //make sure the current enemy is cleared and AI goes back to idle state
		team = newTeam;
		UpdateFriendFx();
	}
}

void idAI::UpdateFriendFx( void ){
	//gameLocal.Printf("idAI::UpdateFriendFx\n");
	if ( team == 0 ) {
		idFuncEmitter* emitter = GetEmitter( FRIEND_FX_NAME );
		if ( emitter ) {
			if ( emitter->IsEmitterHidden() == (health > 0) ) {
				//gameLocal.Printf("idAI::UpdateFriendFx -> SetEmitterVisible at %d\n", gameLocal.time);
				//SetEmitterVisible( FRIEND_FX_NAME, health > 0 );
				emitter->CancelEvents(&EV_Activate); //don't trigger twice this frame
				emitter->PostEventMS(&EV_Activate, 0, this);
			}
		} else if ( health > 0 && spawnArgs.GetBool( "showFriendFx" ) ) {
			//gameLocal.Printf("idAI::UpdateFriendFx -> StartEmitter  at %d\n", gameLocal.time);
			StartEmitter( FRIEND_FX_NAME, FRIEND_FX_JOINT, FRIEND_FX_MODEL, false );
		}
	} else {
		StopEmitter( FRIEND_FX_NAME );
		//gameLocal.Printf("idAI::UpdateFriendFx -> StopEmitter  at %d\n", gameLocal.time);
	}
}

/*
================
idAI::CanBeRidden
================
*/
bool idAI::CanBeRidden( void ) {
	return (health > 0) && (team != 0) && spawnArgs.GetBool("canBecomeFriend", "0"); //TODO use its own key?
}

//ff1.3 end

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

	idActor::UpdateAnimationControllers(); //ff1.3 - Update the IK before calculating eyes position

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

	/* ff1.3 - removed
	// Update the IK after we've gotten all the joint positions we need, but before we set any joint positions.
	// Getting the joint positions causes the joints to be updated.  The IK gets joint positions itself (which
	// are already up to date because of getting the joints in this function) and then sets their positions, which
	// forces the heirarchy to be updated again next time we get a joint or present the model.  If IK is enabled,
	// or if we have a seperate head, we end up transforming the joints twice per frame.  Characters with no
	// head entity and no ik will only transform their joints once.  Set g_debuganim to the current entity number
	// in order to see how many times an entity transforms the joints per frame.
	idActor::UpdateAnimationControllers();
	*/

	idEntity *focusEnt = focusEntity.GetEntity();
	if ( !allowJointMod || !allowEyeFocus || ( gameLocal.time >= focusTime ) ) {
		focusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 512.0f;
	} else if ( focusEnt == NULL ) {
		// keep looking at last position until focusTime is up
		focusPos = currentFocusPos;

		//ff1.3 start
		if ( focusPosOnlyInRange ) {
			newLookAng.yaw = idMath::AngleNormalize180( (currentFocusPos - orientationJointPos).ToYaw() - orientationJointYaw );
			//gameLocal.Printf("newLookAng yaw %f\n", newLookAng.yaw);
			if ( newLookAng.yaw < lookMin.yaw || newLookAng.yaw > lookMax.yaw ) {
				focusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 512.0f;
			}
		}
		//ff1.3 end
	} else if ( focusEnt == enemy.GetEntity() ) {
		focusPos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset - eyeVerticalOffset * enemy.GetEntity()->GetPhysics()->GetGravityNormal();
	} else if ( focusEnt->IsType( idActor::Type ) ) {
		focusPos = (static_cast<idActor *>( focusEnt )->GetCurrentVehicle() ? static_cast<idActor *>( focusEnt )->GetCurrentVehicle()->GetEyePosition() : static_cast<idActor *>( focusEnt )->GetEyePosition() ) - eyeVerticalOffset * focusEnt->GetPhysics()->GetGravityNormal();
	} else if ( focusEnt->IsType( idAFEntity_Vehicle::Type ) ) {
		focusPos = static_cast<idAFEntity_Vehicle *>( focusEnt )->GetEyePosition() - eyeVerticalOffset * focusEnt->GetPhysics()->GetGravityNormal();
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

	//ff1.3 start
	//if ( diff.yaw != 0.0f )
	//	gameLocal.Printf("newLookAng yaw %f\n", newLookAng.yaw);
	//ff1.3 end

	if ( eyeAng != diff ) {
		eyeAng = diff;
		eyeAng.Clamp( eyeMin, eyeMax );
		idAngles angDelta = diff - eyeAng;
		if ( !angDelta.Compare( ang_zero, 0.1f ) ) {
			alignHeadTime = gameLocal.time;
		} else {
			alignHeadTime = gameLocal.time + ( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime;
		}
	}

	if ( idMath::Fabs( newLookAng.yaw ) < 0.1f ) {
		alignHeadTime = gameLocal.time;
	}

	if ( ( gameLocal.time >= alignHeadTime ) || ( gameLocal.time < forceAlignHeadTime ) ) {
		alignHeadTime = gameLocal.time + ( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime;
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
	for( i = 0; i < lookJoints.Num(); i++ ) {
		jointAng.pitch	= lookAng.pitch * lookJointAngles[ i ].pitch;
		jointAng.yaw	= lookAng.yaw * lookJointAngles[ i ].yaw;
		animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
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

	const idBounds &bounds = ( actor->GetCurrentVehicle() ) ? actor->GetCurrentVehicle()->GetPhysics()->GetBounds() : actor->GetPhysics()->GetBounds();
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


//ff1.3 start

/*
===============================================================================

  idAI_Rideable

===============================================================================
*/

const idEventDef AI_UnbindDriver( "unbindDriver" );
const idEventDef AI_FaceIdeal( "faceIdeal" );
const idEventDef AI_FaceIdealOffset( "faceIdealOffset", "f" );
const idEventDef AI_GetIdealAngles( "getIdealAngles", "f", 'v' );
const idEventDef AI_LookAtEnemyOrAimPos( "lookAtEnemyOrAimPos", "f" );
const idEventDef AI_LookAtAimPos( "lookAtAimPos", "f" );
const idEventDef AI_StopRiding( "stopRiding" );

CLASS_DECLARATION( idAI, idAI_Rideable )
	EVENT( EV_GetDriver,				idAI_Rideable::Event_GetDriver )
	EVENT( EV_GetFireMode,				idAI_Rideable::Event_GetFireMode )
	EVENT( EV_GetAimAngles,				idAI_Rideable::Event_GetAimAngles )
	EVENT( AI_UnbindDriver,				idAI_Rideable::Event_UnbindDriver )
	EVENT( AI_FaceIdeal,				idAI_Rideable::Event_FaceIdeal )
	EVENT( AI_FaceIdealOffset,			idAI_Rideable::Event_FaceIdealOffset )
	EVENT( AI_GetIdealAngles,			idAI_Rideable::Event_GetIdealAngles )
	EVENT( AI_LookAtEnemyOrAimPos,		idAI_Rideable::Event_LookAtEnemyOrAimPos )
	EVENT( AI_LookAtAimPos,				idAI_Rideable::Event_LookAtAimPos )
	EVENT( EV_Player_GetMove,			idAI_Rideable::Event_GetMove ) //shortcut
	EVENT( AI_AttackMissile,			idAI_Rideable::Event_AttackMissile ) //override AI event
	EVENT( AI_StartRiding,				idAI_Rideable::Event_StartRiding ) //override AI event
	EVENT( AI_StopRiding,				idAI_Rideable::Event_StopRiding)
	EVENT( AI_EnableClip,				idAI_Rideable::Event_EnableClip ) //override AI event
	EVENT( EV_Remove,					idAI_Rideable::Event_Remove ) //override entity event
	//EVENT( AI_MeleeAttackToJoint,		idAI_Rideable::Event_MeleeAttackToJoint ) //override AI event
END_CLASS

/*
================
idAI_Rideable::idAI_Rideable
================
*/
idAI_Rideable::idAI_Rideable( void ) {
	timeMode					= false;
	rideEndTime					= 0;
	lastStopRequestTime			= 0;
	rideStatus					= RIDE_STATUS_NOTREADY;
	driver						= NULL;
	thirdPersonRange			= 0.0f;
	thirdPersonHeight			= 0.0f;
	currentFireMode				= 0;
	weaponBarSlot				= 0;
	numFireModes				= 0;
	hudPulseFlags				= 0;
	cameraOffset.Zero();
	aimPos.Zero();
}

/*
================
idAI_Rideable::Spawn
================
*/
void idAI_Rideable::Spawn( void ) {
	//camera settings
	spawnArgs.GetFloat("thirdPersonRange", "100", thirdPersonRange);
	spawnArgs.GetFloat("thirdPersonHeight", "50", thirdPersonHeight);
	spawnArgs.GetVector("cameraOffset", "0 0 30", cameraOffset); //keep it low so it doesn't intersect walls

	//attack settings
	spawnArgs.GetInt("numFireModes", "1", numFireModes);
	spawnArgs.GetInt("weaponBarSlot", "-1", weaponBarSlot);
	if ( numFireModes > 1 ) {
		currentFireMode = 1;
	}

	//damage multipliers
	meleeDamageScale = spawnArgs.GetFloat("melee_dmg_scale", "1");
	projectileDamageScale = spawnArgs.GetFloat("projectile_dmg_scale", "1");

	//triggers
	canTouchTriggers = false;
	forcedMovements = true;

	//player or monster clip
	if ( spawnArgs.GetBool("monsterClip") ) {
		//use monster clip
	} else {
		physicsObj.SetClipMask( MASK_PLAYERSOLID ); //player clip
	}

	spawnArgs.GetBool(RIDEABLE_TIMEMODE_SPAWNARG, "0", timeMode);
	rideStatus = RIDE_STATUS_READY;
	fl.takedamage = false; //only allow damage while active
}

/*
=====================
idAI_Rideable::Event_EnableClip
Override to use player clip
=====================
*/
void idAI_Rideable::Event_EnableClip( void ) {
	physicsObj.SetClipMask( MASK_PLAYERSOLID );
	disableGravity = false;
}

/*
=====================
idAI_Rideable::LinkScriptVariables
=====================
*/
void idAI_Rideable::LinkScriptVariables( void ) {
	idAI::LinkScriptVariables();
	AI_ATTACK.LinkTo( scriptObject, "AI_ATTACK" );
	AI_SEC_ATTACK.LinkTo( scriptObject, "AI_SEC_ATTACK" );
	AI_RELOAD.LinkTo( scriptObject, "AI_RELOAD" );
}


/*
==================
idAI_Rideable::Save
==================
*/
void idAI_Rideable::Save( idSaveGame *savefile ) const {
	savefile->WriteBool(timeMode);
	savefile->WriteInt(rideEndTime);
	savefile->WriteInt(lastStopRequestTime);
	savefile->WriteInt(rideStatus);
	savefile->WriteObject(driver);
	savefile->WriteFloat(thirdPersonRange);
	savefile->WriteFloat(thirdPersonHeight);
	savefile->WriteInt(currentFireMode);
	savefile->WriteInt(weaponBarSlot);
	savefile->WriteInt(numFireModes);
	savefile->WriteInt(hudPulseFlags);
	savefile->WriteVec3(aimPos);
	savefile->WriteVec3(cameraOffset);
}
/*
==================
idAI_Rideable::Restore
==================
*/
void idAI_Rideable::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool(timeMode);
	savefile->ReadInt(rideEndTime);
	savefile->ReadInt(lastStopRequestTime);
	savefile->ReadInt(rideStatus);
	savefile->ReadObject(reinterpret_cast<idClass *&>(driver));
	savefile->ReadFloat(thirdPersonRange);
	savefile->ReadFloat(thirdPersonHeight);
	savefile->ReadInt(currentFireMode);
	savefile->ReadInt(weaponBarSlot);
	savefile->ReadInt(numFireModes);
	savefile->ReadInt(hudPulseFlags);
	savefile->ReadVec3(aimPos);
	savefile->ReadVec3(cameraOffset);
}

/*
================
idAI_Rideable::UpdateAttack
Invoked by idAI::Think
================
*/
void idAI_Rideable::UpdateAttack( void ) {
	AI_ATTACK = false;
	AI_SEC_ATTACK = false;
	//AI_RELOAD = false; //reset by scripts

	if ( driver ) {
		if ( !gameLocal.inCinematic ) {
			if ( driver->objectiveSystemOpen ) {
				if ( AI_PAIN ) {
					driver->TogglePDA();
				}
			} else {
				if ( driver->usercmd.buttons & BUTTON_ATTACK ) {
					AI_ATTACK = true;
				}
				if ( driver->usercmd.buttons & BUTTON_5 ) {
					AI_SEC_ATTACK = true;
				}
			}
		}

		if ( timeMode ) {
			if ( gameLocal.inCinematic || driver->GetPrivateCameraView() ) {
				rideEndTime += gameLocal.msec;
			} else if ( rideEndTime > 0 && rideEndTime < gameLocal.time ) {
				rideEndTime = 0;
				PostEventMS( &AI_StopRiding, 0 );
			}
		}
	}

	UpdateAim();
}

/*
================
idAI_Rideable::UpdateAim
================
*/
void idAI_Rideable::UpdateAim( void ) {
	if ( !gameLocal.inCinematic && driver && driver->renderView ) {
		renderView_t *driverView = driver->renderView;

		idVec3 end = driverView->vieworg + driverView->viewaxis[ 0 ] * 3000.0f;
		trace_t trace;
		gameLocal.clip.TracePoint( trace, driverView->vieworg, end, MASK_SOLID|CONTENTS_RENDERMODEL, this );
		if ( trace.fraction < 1.0f ) {
			aimPos = trace.endpos;

			//set the enemy so that scripts, melee attacks and guided projectiles work like standard AI
			idEntity* aimTarget = gameLocal.GetTraceEntity( trace );
			if ( aimTarget
				&& aimTarget->health > 0
				&& aimTarget != enemy.GetEntity()
				&& aimTarget->IsType( idActor::Type )
			) {
				idActor *actor = static_cast<idActor *>( aimTarget );
				if ( ReactionTo( actor ) != ATTACK_IGNORE ) {
					//gameLocal.Printf("idAI_Rideable::UpdateAim - enemy %s\n", actor->GetName() );
					SetEnemy( actor );
				}
			}
		} else {
			aimPos = end;
		}
	} else {
		aimPos = physicsObj.GetOrigin() + viewAxis[ 0 ] * 3000.0f;
	}
}

/*
===============
idAI_Rideable::GetCameraPos
===============
*/
void idAI_Rideable::GetCameraPos( idVec3 &origin, idMat3 &axis ) {
	origin = physicsObj.GetOrigin() + cameraOffset * viewAxis;
	axis = viewAxis;
}

/*
============
idAI_Rideable::IgnoreDamage
============
*/
bool idAI_Rideable::IgnoreDamage( idEntity *inflictor, idEntity *attacker, const idDict *damageDef, float *additionalDamageScale ){
	if ( driver && ( driver->godmode || driver->GetPrivateCameraView() ) ) {
		return true;
	}

	if ( gameLocal.inCinematic ) {
		return true;
	}

	if ( damageDef->GetBool( "ignore_rideable" ) ) {
		return true;
	}

	if ( attacker != this ) { //allow self-damage
		if ( attacker->IsType( idActor::Type ) ) {
			if ( static_cast<idActor *>(attacker)->team == 0 ) {
				return true; //disable friendly fire
			}
		}
		//handle damage of projectiles fired by already-removed friends (rare, but could happen)
		else if ( inflictor->IsType( idProjectile::Type ) && static_cast<idProjectile *>(inflictor)->FiredByFriend() ) {
			gameLocal.Warning( "Ignored damage for projectile fired by removed friend");
			return true; //disable friendly fire
		}
	}

	float skillScale = 1.0f;
	if ( !gameLocal.isMultiplayer && inflictor != gameLocal.world ) {
		switch ( g_skill.GetInteger() ) {
			case 0:
				skillScale = 0.9f;
				break;
			case 2:
				skillScale = 1.1f;
				break;
			case 3:
				skillScale = 1.2f;
				break;
			default:
				break;
		}
	}

	*additionalDamageScale = skillScale * damageDef->GetFloat("rideableDamageScale", "1");
	return false;
}

/*
============
idAI_Rideable::Killed
============
*/
void idAI_Rideable::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( driver ) {
		if ( timeMode ) { //usually it cannot die in timemode, but zombie bomb can
			UnbindDriver(false);
		} else {
			driver->Kill(false, false);
		}
	}
	idAI::Killed( inflictor, attacker, damage, dir, location );
}

/*
================
idAI_Rideable::Show
================
*/
void idAI_Rideable::Show( void ) {
	idAI::Show();
	fl.takedamage = (rideStatus == RIDE_STATUS_STARTED);
}

/*
===============
idAI_Rideable::PerformImpulse
===============
*/
bool idAI_Rideable::PerformImpulse( int impulse ) {
	//weapons impulses
	if ( (impulse >= IMPULSE_0 && impulse <= IMPULSE_12) || ( impulse >= IMPULSE_23 && impulse <= IMPULSE_28 ) ) {
		switch( impulse ) { //supports 10 weapons (use same keys as the player)
			case IMPULSE_2: {
				SelectWeapon( 0 );
				break;
			}
			case IMPULSE_3: {
				SelectWeapon( 1 );
				break;
			}
			case IMPULSE_5: {
				SelectWeapon( 2 );
				break;
			}
			case IMPULSE_7: {
				SelectWeapon( 3 );
				break;
			}
			case IMPULSE_8: {
				SelectWeapon( 4 );
				break;
			}
			case IMPULSE_9: {
				SelectWeapon( 5 );
				break;
			}
			case IMPULSE_11: {
				SelectWeapon( 6 );
				break;
			}
			case IMPULSE_23: {
				SelectWeapon( 7 );
				break;
			}
			case IMPULSE_24: {
				SelectWeapon( 8 );
				break;
			}
			case IMPULSE_25: {
				SelectWeapon( 9 );
				break;
			}
		}
		return true;
	}

	//other impulses
	switch( impulse ) {
		case IMPULSE_13: {
			AI_RELOAD = true;
			return true;
		}
		case IMPULSE_14: {
			NextWeapon();
			return true;
		}
		case IMPULSE_15: {
			PrevWeapon();
			return true;
		}
		case IMPULSE_21: {
			/*
			if ( rideStatus == RIDE_STATUS_STARTED ) {
				if ( !timeMode ) { //spawnArgs.GetBool("noExit")
					hudPulseFlags |= HUD_RIDE_PULSE_EXIT_DISABLED;
				} else if ( lastStopRequestTime > 0 && gameLocal.time < lastStopRequestTime + 1000 ) {
					PostEventMS( &AI_StopRiding, 0 );
				} else {
					hudPulseFlags |= HUD_RIDE_PULSE_EXIT_1MORE;
					lastStopRequestTime = gameLocal.time;
				}
			}
			*/
			return true;
		}
	}
	return false;
}

/*
===============
idAI_Rideable::SelectWeapon
===============
*/
void idAI_Rideable::SelectWeapon( int num ){
	if ( num >= 0 && num < numFireModes && num != currentFireMode ) {
		currentFireMode = num;
		hudPulseFlags |= HUD_RIDE_PULSE_WEAPON;
	}
}

/*
===============
idAI_Rideable::NextWeapon
===============
*/
void idAI_Rideable::NextWeapon( void ) {
	currentFireMode++;
	if ( currentFireMode >= numFireModes ) {
		currentFireMode = 0;
	}
	hudPulseFlags |= HUD_RIDE_PULSE_WEAPON;
}

/*
===============
idAI_Rideable::PrevWeapon
===============
*/
void idAI_Rideable::PrevWeapon( void ) {
	currentFireMode--;
	if ( currentFireMode < 0 ) {
		currentFireMode = numFireModes-1;
	}
	hudPulseFlags |= HUD_RIDE_PULSE_WEAPON;
}

/*
===============
idAI_Rideable::InitHudStats
===============
*/
void idAI_Rideable::InitHudStats( idUserInterface *hud, idUserInterface *cursor ) {
	if ( hud ) {
		//hud->SetStateInt( "sentry_health_max", spawnArgs.GetInt("health") );
		if ( timeMode ) {
			hud->HandleNamedEvent( "show_timer" ); //spawnArgs.GetString("hudTimeEvent", "health_mode_time") );
		}
		if ( weaponBarSlot != -1 ) {
			hud->HandleNamedEvent( va( "set_weapon_bar_icon%i", weaponBarSlot ) );
		} else {
			hud->HandleNamedEvent( "hide_weapon_bar");
		}
		hud->HandleNamedEvent( spawnArgs.GetString("hudHealthEvent", "health_mode_soul") );
		hud->HandleNamedEvent( spawnArgs.GetString("hudWeaponEvent", "setup_melee_only") );
		hud->HandleNamedEvent( "start_riding" );
	}

	if ( cursor ) {
		cursor->HandleNamedEvent( spawnArgs.GetString("cursorEvent", "hideCursor") ); //enableSentryCursor
	}
}

/*
===============
idAI_Rideable::UpdateHudStats
===============
*/
void idAI_Rideable::UpdateHudStats( idUserInterface *hud ) {
	if ( !hud ) {
		return;
	}
	if ( timeMode ) {
		if ( rideStatus == RIDE_STATUS_STARTED ) { //stop updating the timer while exiting
			hud->SetStateInt( "sentry_timer", (rideEndTime > gameLocal.time) ? (rideEndTime - gameLocal.time)/1000 : 0 );
		}
	}

	hud->SetStateInt( "sentry_health", health );
	hud->SetStateFloat( "sentry_health_pct", health / spawnArgs.GetFloat("health") );

	if ( hudPulseFlags ) {
		if ( hudPulseFlags & HUD_RIDE_PULSE_WEAPON ) {
			hud->HandleNamedEvent( va( "select_a%i", currentFireMode ) );

			/*
			if ( weaponBarSlot != -1 ) {
				if ( currentFireMode == weaponBarSlot ) {
					hud->HandleNamedEvent( va( "set_weapon_bar_icon%i", weaponBarSlot ) );
				} else {
					hud->HandleNamedEvent( "hide_weapon_bar");
				}
			}
			*/
		}
		/*
		if ( hudPulseFlags & HUD_RIDE_PULSE_EXIT_DISABLED ) {
			hud->HandleNamedEvent( "rideableExitDisabled" );
		}

		if ( hudPulseFlags & HUD_RIDE_PULSE_EXIT_1MORE ) {
			hud->HandleNamedEvent( "rideableExit1More" );
		}
		*/
		hudPulseFlags = 0;
	}
}

/*
=====================
idAI_Rideable::Event_StartRiding
=====================
*/
void idAI_Rideable::Event_StartRiding( idEntity* ent, int checkSkippedTriggers ){
	idPlayer *player;

	if ( rideStatus != RIDE_STATUS_READY ) {
		gameLocal.Warning("Unexpected ride status: %d", rideStatus);
		return;
	}

	if ( driver || AI_DEAD ) {
		return;
	}

	if ( !ent || !ent->IsType( idPlayer::Type ) ) {
		gameLocal.Warning("%s: driver is not an idPlayer", name.c_str());
		return;
	}

	player = static_cast< idPlayer * >( ent );
	if ( !player->CanEnterAI() ) {
		return;
	}

	player->EnterAI(this);
	driver = player;
	canTouchTriggers = true;
	fl.takedamage = true;

	if ( timeMode ) {
		rideEndTime = gameLocal.time + ff_rideable_time.GetInteger()*1000;
	} else {
		rideEndTime = 0;
	}

	const idVec3 oldDriverPos = driver->GetPhysics()->GetAbsBounds().GetCenter();

	// move up to make sure the driver is at least an epsilon above the floor
	driver->GetPhysics()->SetOrigin( physicsObj.GetOrigin() + idVec3( 0, 0, RIDE_DRIVER_OFFSET ) );
	driver->BindToBody( this, 0, false );
	driver->SetViewAngles( idAngles( 0, current_yaw, 0 ) );
	rideStatus = RIDE_STATUS_STARTED;
	hudPulseFlags |= HUD_RIDE_PULSE_WEAPON;
	SetState( "state_RideStarted" );

	if ( checkSkippedTriggers ) {
		TriggerSkippedTriggers( oldDriverPos );
	}

}

/*
================
idAI_Rideable::Event_StopRiding
================
*/
void idAI_Rideable::Event_StopRiding( void ) {
	if ( rideStatus != RIDE_STATUS_STARTED ) {
		gameLocal.Warning("Unexpected ride status: %d", rideStatus);
		return;
	}

	if ( driver && !AI_DEAD ){
		fl.takedamage = false;
		rideStatus = RIDE_STATUS_STOPPED;
		SetState( "state_RideStopped" );
	}
}

/*
================
idAI_Rideable::Event_Remove
================
*/
void idAI_Rideable::Event_Remove( void ) {
	UnbindDriver(false);
	idAI::Event_Remove();
}

/*
==================
idAI_Rideable::Event_UnbindDriver
==================
*/
void idAI_Rideable::Event_UnbindDriver( void ){
	if ( rideStatus != RIDE_STATUS_STOPPED ) {
		gameLocal.Warning("Unexpected ride status: %d", rideStatus);
		return;
	}
	UnbindDriver(false);
}

/*
==================
idAI_Rideable::UnbindDriver
==================
*/
void idAI_Rideable::UnbindDriver( bool driverKilled ){
	if ( driver ) {
		driver->ExitAI(driverKilled);
		driver->Unbind();
		SetDriverInValidPosition();
		driver = NULL;
		canTouchTriggers = false;

		//clear enemies
		idActor *ent;
		for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
			if ( ent->IsType( idAI::Type ) ) {
				static_cast<idAI*>(ent)->PostEventMS( &AI_ClearEnemy, 0 );
			}
		}
		rideStatus = RIDE_STATUS_READY;
	}
}

/*
==================
idAI_Rideable::SetDriverInValidPosition
==================
*/
void idAI_Rideable::SetDriverInValidPosition( void ){
	idVec3 from, to;
	trace_t trace;

	const idBounds &driverBounds = driver->GetPhysics()->GetBounds();
	//gameLocal.Printf("driverBounds[1][2]: %f, driverBounds[0][2]: %f\n", driverBounds[1][2] , driverBounds[0][2]);

	const idBounds &bounds = physicsObj.GetBounds();
	//gameLocal.Printf("bounds[1][2]: %f, bounds[0][2]: %f\n", bounds[1][2] , bounds[0][2]);

	float shorterThanDriverDelta = (driverBounds[1][2] - driverBounds[0][2]) - (bounds[1][2] - bounds[0][2]) + RIDE_DRIVER_OFFSET;
	//gameLocal.Printf("shorterThanDriverDelta: %f\n", shorterThanDriverDelta);
	if ( shorterThanDriverDelta > 0 ) {
		from = physicsObj.GetOrigin();
		to = from;
		to.z -= shorterThanDriverDelta; //check the ground

		gameLocal.clip.TracePoint( trace, from, to, MASK_SOLID, this );
		//gameLocal.Printf("trace.fraction: %f\n", trace.fraction);

		if ( trace.fraction < 1.0f ) {
			driver->GetPhysics()->SetOrigin( trace.endpos + idVec3( 0, 0, RIDE_DRIVER_OFFSET ) );
		} else {
			driver->GetPhysics()->SetOrigin( to + idVec3( 0, 0, RIDE_DRIVER_OFFSET ) );
		}
	}
}


/*
==================
idAI_Rideable::Event_GetDriver
==================
*/
void idAI_Rideable::Event_GetDriver( void ) {
	idThread::ReturnEntity( driver );
}

/*
==================
idAI_Rideable::Event_GetFireMode
==================
*/
void idAI_Rideable::Event_GetFireMode( void ) {
	idThread::ReturnInt( currentFireMode );
}

/*
==================
idAI_Rideable::Event_GetAimAngles
==================
*/
void idAI_Rideable::Event_GetAimAngles( const idVec3 &firePos ) {
	idAngles angles = (aimPos - firePos).ToAngles();
	idThread::ReturnVector( idVec3( angles[0], angles[1], angles[2] ) );
}

/*
==================
idAI_Rideable::Event_FaceIdeal
==================
*/
void idAI_Rideable::Event_FaceIdeal( void ) {
	if ( driver ) {
		TurnToward( driver->viewAngles.yaw );
	}
}

/*
==================
idAI_Rideable::Event_FaceIdealOffset
==================
*/
void idAI_Rideable::Event_FaceIdealOffset( float offset ) {
	if ( driver ) {
		TurnToward( /*idMath::AngleNormalize180(*/ driver->viewAngles.yaw - offset /*)*/ );
	}
}

/*
==================
idAI_Rideable::Event_GetIdealAngles
==================
*/
void idAI_Rideable::Event_GetIdealAngles( float offset ) {
	if ( driver ) {
		float yaw = idMath::AngleNormalize180(driver->viewAngles.yaw - offset);
		idThread::ReturnVector( idVec3( 0, yaw, 0 ) );
	}
}

/*
==================
idAI_Rideable::Event_GetMove
==================
*/
void idAI_Rideable::Event_GetMove( void ) {
	idThread::ReturnVector( driver ? idVec3 ( driver->usercmd.forwardmove, driver->usercmd.rightmove, driver->usercmd.upmove ) : vec3_origin );
}


/*
=====================
idAI_Rideable::Event_AttackMissile
=====================
*/
void idAI_Rideable::Event_AttackMissile( const char *jointname ) {
	idThread::ReturnEntity( LaunchProjectile( jointname, true ) );
}

/*
=====================
idAI_Rideable::LaunchProjectile
=====================
*/
idProjectile *idAI_Rideable::LaunchProjectile( const char *jointname, bool clampToAttackCone ) {
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
	//idMat3				proj_axis;
	bool				forceMuzzle;
#endif
	idVec3				tmp;
	idProjectile		*lastProjectile;

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	num_projectiles = spawnArgs.GetInt( "num_projectiles", "1" );
#ifdef _D3XP
	forceMuzzle = spawnArgs.GetBool( "forceMuzzle", "0" );
#endif

	GetMuzzle( jointname, muzzle, axis );

	if ( !projectile.GetEntity() ) {
		CreateProjectile( muzzle, axis[ 0 ] );
	}

	lastProjectile = projectile.GetEntity();

	//ff start
	/*was:
	if ( target != NULL ) {
		tmp = target->GetPhysics()->GetAbsBounds().GetCenter() - muzzle;
		tmp.Normalize();
		axis = tmp.ToMat3();
	} else {
		axis = viewAxis;
	}
	*/
	tmp = aimPos - muzzle;
	tmp.Normalize();
	axis = tmp.ToMat3();
	//ff end

	// rotate it because the cone points up by default
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

#ifdef _D3XP
	//proj_axis = axis;
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
	//ff start
	//was: GetAimDir( muzzle, target, this, dir );
	dir = aimPos - muzzle;
	dir.Normalize();
	//ff end
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
	for( i = 0; i < num_projectiles; i++ ) {
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
		//ff1.3 - was: lastProjectile->Launch( muzzle, dir, vec3_origin );
		lastProjectile->Launch( muzzle, dir, vec3_origin, 0.0f, 1.0f, projectileDamageScale );
		projectile = NULL;
	}

	TriggerWeaponEffects( muzzle );

	lastAttackTime = gameLocal.time;

	return lastProjectile;
}

/*
=====================
idAI_Rideable::Event_LookAtAimPos
=====================
*/
void idAI_Rideable::Event_LookAtAimPos( float duration ) {
	if ( ( focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity = NULL;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}
	currentFocusPos = aimPos; //explicitly update focus position!
	focusTime = gameLocal.time + SEC2MS( duration );
}

/*
=====================
idAI_Rideable::Event_LookAtEnemyOrAimPos
=====================
*/
void idAI_Rideable::Event_LookAtEnemyOrAimPos( float duration ) {
	if ( !AI_ATTACK && !AI_SEC_ATTACK ) {
		if ( enemy.GetEntity() ) {
			focusPosOnlyInRange = false;
			Event_LookAtEnemy( duration );
			return;
		}
		else if ( focusEntity.GetSpawnId() && focusTime >= gameLocal.time ) { //dead enemy or custom target
			//the main effect of the "duration" parameter is setting for how long we should look at a dead enemy
			focusPosOnlyInRange = false;
			return;
		} else { //just looking around
			focusPosOnlyInRange = true;
		}
	} else {
		focusPosOnlyInRange = false;
	}
	Event_LookAtAimPos( duration );
}

/*
================
idAI_Rideable::Teleport
================
*/
void idAI_Rideable::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	idAI::Teleport(origin, angles, destination);
	current_yaw		= angles.yaw;
	ideal_yaw		= idMath::AngleNormalize180( angles.yaw );

	if ( driver ) {
		driver->SetViewAngles( idAngles( 0, angles.yaw, 0 ) );
	}
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
bool idAI_Rideable::AttackMelee( const char *meleeDefName ) {
	const idDict *	meleeDef;
	const char *	p;
	const idSoundShader *shader;

	idEntity *		ent;
	idEntity *		entityList[ MAX_GENTITIES ];
	int				numListedEntities;
	idBounds		damageBounds;
	idVec3			damagePos;
	idMat3			axis;
	jointHandle_t	joint;
	bool			hit = false;
	trace_t			trace;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown melee '%s'", meleeDefName );
	}

	idVec3	kickDir;
	meleeDef->GetVector( "kickDir", "0 0 0", kickDir );

	idVec3	globalKickDir;
	globalKickDir = ( viewAxis * physicsObj.GetGravityAxis() ) * kickDir;

	const char *jointname = spawnArgs.GetString("melee_joint"); //"RMissile";
	if ( jointname && *jointname ) {
		joint = animator.GetJointHandle( jointname );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Warning( "Unknown joint '%s' in %s for %s", jointname, meleeDefName, GetEntityDefName() );
			return false;
		}

		animator.GetJointTransform( joint, gameLocal.time, damagePos, axis );
		damagePos = physicsObj.GetOrigin() + ( damagePos + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();
		damageBounds = idBounds( damagePos ).Expand( melee_range );
	} else { //bbox like testMelee()
		//gameLocal.Warning( "Missing melee_joint in %s for %s", meleeDefName, GetEntityDefName() );

		//FIXME: make work with gravity vector
		// expand the bounds out by our melee range
		const idBounds &myBounds = physicsObj.GetBounds();

		damageBounds[0][0] = -melee_range;
		damageBounds[0][1] = -melee_range;
		damageBounds[0][2] = myBounds[0][2] - 4.0f;
		damageBounds[1][0] = melee_range;
		damageBounds[1][1] = melee_range;
		damageBounds[1][2] = myBounds[1][2] + 4.0f;
		damageBounds.TranslateSelf( physicsObj.GetOrigin() );

		damagePos = damageBounds.GetCenter();
	}

	float targetFovDot = (float)cos( DEG2RAD( spawnArgs.GetFloat("melee_angle", "180") * 0.5f ) );

	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( damageBounds, MASK_SHOT_RENDERMODEL, entityList, MAX_GENTITIES );
	for ( int e = 0; e < numListedEntities; e++ ) {
		ent = entityList[ e ];
		assert( ent );

		if ( ent == this || ent == driver || ent->IsHidden() /*|| !ent->IsActive()*/ ) {
			continue;
		}

		if ( ent->IsType( idActor::Type ) || ent->IsType( idDamagable::Type ) || ent->IsType( idMoveable::Type ) ) {
			//gameLocal.Printf("found melee target %s\n", ent->GetName());

			if ( !CheckFOV( ent->GetPhysics()->GetOrigin(), targetFovDot ) ) {
				//gameRenderWorld->DebugLine( colorRed, physicsObj.GetOrigin(), ent->GetPhysics()->GetOrigin(), 5000 );
				continue;
			}
			//gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), ent->GetPhysics()->GetOrigin(), 5000 );

			//damage
			ent->Damage( this, this, globalKickDir, meleeDefName, meleeDamageScale, INVALID_JOINT );
			DamageEffect( meleeDefName, ent, damagePos, melee_range );

			if ( !hit ) {
				hit = true;
			}
		}
	}

	//sound
	p = meleeDef->GetString( hit ? "snd_hit" : "snd_miss" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	if ( hit ) {
		lastAttackTime = gameLocal.time;
	}
	return hit;
}

/*
=====================
idAI_Rideable::Event_MeleeAttackToJoint
=====================

void idAI_Rideable::Event_MeleeAttackToJoint( const char *jointname, const char *meleeDefName ) {
	jointHandle_t	joint;
	idVec3			start;
	idVec3			end;
	idMat3			axis;
	trace_t			trace;
	idEntity		*hitEnt;

	idVec3			damageFxDir; //ff1.3

	joint = animator.GetJointHandle( jointname );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Unknown joint '%s' on %s", jointname, GetEntityDefName() );
	}
	animator.GetJointTransform( joint, gameLocal.time, end, axis );
	end = physicsObj.GetOrigin() + ( end + modelOffset ) * viewAxis * physicsObj.GetGravityAxis();
	start = GetEyePosition();

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, start, end, gameLocal.msec );
	}

	gameLocal.clip.TranslationEntities( trace, start, end, NULL, mat3_identity, MASK_SHOT_BOUNDINGBOX, this );
	if ( trace.fraction < 1.0f ) {
		hitEnt = gameLocal.GetTraceEntity( trace );
		if ( hitEnt && (hitEnt->IsType( idActor::Type ) || hitEnt->IsType( idAFEntity_Vehicle::Type )) ) {
			DirectDamage( meleeDefName, hitEnt );

			//damage effect - could work for any entity but just add blood on actors for now
			if ( hitEnt->IsType( idActor::Type ) ) {
				//gameRenderWorld->DebugLine( colorYellow, start, end, 5000 );
				if ( !static_cast<idActor *>(hitEnt)->GetInsideBodyAimTarget(end) ) {
					end = hitEnt->GetPhysics()->GetAbsBounds().GetCenter();
				}
				damageFxDir = end - start;
				damageFxDir.Normalize();
				end = start + damageFxDir * (trace.endpos-start).LengthFast() * 1.4f; //don't go much further than the previous trace

				//gameRenderWorld->DebugLine( colorBlue, start, end, 5000 );
				gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
				if ( ( trace.fraction < 1.0f ) && ( gameLocal.GetTraceEntity( trace ) == hitEnt ) ) {
					//gameLocal.Printf("AddDamageEffect\n");
					hitEnt->AddDamageEffect( trace, damageFxDir, meleeDefName, this );
				}
			}

			idThread::ReturnInt( true );
			return;
		}
	}

	idThread::ReturnInt( false );
}
*/

/*
=================
idAI_Rideable::TriggerSkippedTriggers
=================
*/
void idAI_Rideable::TriggerSkippedTriggers( const idVec3 &startPos ){
	const float POSSESSION_MAX_TRACER_STEP_LENGHT = 4000.0f;
	const int POSSESSION_MAX_HIT_TARGETS = 20;
	idVec3 tempStartPos, tempEndPos;
	idVec3 tracerDir;
	trace_t tr;
	idEntity* ent;
	int hitTargetCounter;
	float idealTracerLenght;
	idBounds hitbox;
	int hitEntityNumbers[POSSESSION_MAX_HIT_TARGETS] = {};
	int i;

	if ( !driver ){
		return;
	}

	const idVec3 endPos = physicsObj.GetAbsBounds().GetCenter();

	//gameRenderWorld->DebugLine( colorBlue, startPos, endPos, 10000, true );

	//init loop vars
	ent = driver;
	hitTargetCounter = 0;

	//prepare for trace
	hitbox = idBounds( vec3_origin ).Expand( 10 );
	tracerDir = (endPos - startPos);
	tracerDir.Normalize();
	tempStartPos = startPos + tracerDir * 5.0f;

	while ( hitTargetCounter < POSSESSION_MAX_HIT_TARGETS ) {

		idealTracerLenght = (endPos - tempStartPos).Length();

		if ( idealTracerLenght > POSSESSION_MAX_TRACER_STEP_LENGHT ) {
			tempEndPos = tempStartPos + tracerDir * POSSESSION_MAX_TRACER_STEP_LENGHT;
		} else {
			tempEndPos = endPos;
		}

		gameLocal.clip.TraceBounds( tr, tempStartPos, tempEndPos, hitbox, CONTENTS_TRIGGER, ent );
		//gameRenderWorld->DebugBounds( colorOrange, hitbox, tempStartPos, 10000 );


		if ( tr.fraction < 1.0f ) {
			hitTargetCounter++;

			//upd startPos
			tempStartPos = tr.endpos + tracerDir * 5.0f; //always move a bit forward
			ent = gameLocal.entities[ tr.c.entityNum ];
			//gameLocal.Printf("hit entity: %s\n", ent->GetName() );
			if ( !ent->IsType( idActor::Type ) && !ent->IsType( idExplodingBarrel::Type ) ){ //don't get the master for ExplodingBarrels
				idEntity* master = gameLocal.entities[ tr.c.entityNum ]->GetBindMaster();
				if ( master ) {
					ent = master;
				}
			}

			//don't hit the same entity twice
			for(i = 0; i < hitTargetCounter; i++){
				if ( ent->entityNumber == hitEntityNumbers[i] ) {
					break;
				}
			}
			if ( i < hitTargetCounter ) {
				//gameLocal.Printf("skip already hit entity: %s\n", ent->GetName() );
				continue;
			}
			hitEntityNumbers[hitTargetCounter] = ent->entityNumber;

			if ( (!ent->RespondsTo( EV_Touch ) && !ent->HasSignal( SIG_TOUCH )) || !ent->IsType( idTrigger::Type ) || ent->spawnArgs.GetBool("ignorePossessionProj") ) {
				//gameLocal.Printf("ignore entity: %s\n", ent->GetName() );
				continue;
			}

			/*
			if ( !GetPhysics()->ClipContents( cm ) ) {
				continue;
			}
			*/

			//gameLocal.Printf("touch entity: %s\n", ent->GetName() );
#ifdef _D3XP
			SetTimeState ts( ent->timeGroup );
#endif

			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, driver, &tr );
		} else if ( idealTracerLenght > POSSESSION_MAX_TRACER_STEP_LENGHT ) {
			tempStartPos = tempEndPos;
		} else {
			break; //endPos reached
		}
	}
}
//ff1.3 end
