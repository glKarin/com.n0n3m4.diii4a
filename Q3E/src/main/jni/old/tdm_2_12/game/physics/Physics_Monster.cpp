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

CLASS_DECLARATION( idPhysics_Actor, idPhysics_Monster )
END_CLASS

const float OVERCLIP = 1.001f;

/*
=====================
idPhysics_Monster::CheckGround
=====================
*/
void idPhysics_Monster::CheckGround( monsterPState_t &state ) {
	trace_t groundTrace;
	idVec3 down;

	if ( gravityNormal == vec3_zero ) {
		state.onGround = false;
		groundEntityPtr = NULL;
		return;
	}

	down = state.origin + gravityNormal * CONTACT_EPSILON;
	gameLocal.clip.Translation( groundTrace, state.origin, down, clipModel, clipModel->GetAxis(), clipMask, self );

	if ( groundTrace.fraction == 1.0f ) {
		state.onGround = false;
		groundEntityPtr = NULL;
		return;
	}

	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	if ( ( groundTrace.c.normal * -gravityNormal ) < minFloorCosine )
	{
		// grayman #2356 - assumed to be sliding down an incline > 45 degrees, but could also
		// be sitting on an angled piece of a func_static, so check current origin.z against
		// previous origin.z to see if you're really sliding. This prevents excessive buildup
		// of gravity-induced vertical velocity, which leads to death once you get free and
		// fall to the ground, where Crashland() thinks you fell from a great height.

		idVec3 prevMove = static_cast<idAI*>(self)->movementSubsystem->GetLastMove();
		if (prevMove.z < 0) // are you truly falling?
		{
			state.onGround = false;
			return;
		}
	}

	state.onGround = true;

	// let the entity know about the collision
	self->Collide( groundTrace, state.velocity );

	idEntity* groundEnt = groundEntityPtr.GetEntity();

	// greebo: Apply force/impulse to entities below the clipmodel
	if ( groundTrace.c.entityNum != ENTITYNUM_WORLD && groundEnt != NULL )
	{
		idPhysics* groundPhysics = groundEnt->GetPhysics();

		impactInfo_t info;
		groundEnt->GetImpactInfo( self, groundTrace.c.id, groundTrace.c.point, &info );

		// greebo: Don't push entities that already have a velocity towards the ground.
		if ( groundPhysics && info.invMass != 0.0f )
		{
			// grayman #2478 - is this a mine? if so, blow it up now instead of waiting
			// for the Collide() code to blow it up. Waiting allows the physics engine
			// to sink the mine into the floor before it blows. If the mine is not
			// armed yet, nothing happens, but at least the mine isn't pushed into the floor.

			if ( groundEnt->IsType(idProjectile::Type) && static_cast<idProjectile*>(groundEnt)->IsMine() )
			{
				static_cast<idProjectile*>(groundEnt)->MineExplode( self->entityNumber );
			}
			else
			{
				// greebo: Apply a force to the entity below the player
				//gameRenderWorld->DebugArrow(colorCyan, current.origin, current.origin + gravityNormal*20, 1, 16);

				// grayman TODO: When an AI steps on something and applies this force, it's
				// possible that physics will cause the thing to fall through the floor.
				// gameLocal.clip.Motion() might have a problem with something sitting flat
				// on a world brush.

				groundPhysics->AddForce(0, current.origin, gravityNormal, this);
				groundPhysics->Activate();
			}
		}
	}
}

/*
=====================
idPhysics_Monster::SlideMove
=====================
*/
monsterMoveResult_t idPhysics_Monster::SlideMove( idVec3 &start, idVec3 &velocity, const idVec3 &delta ) {
	int i;
	trace_t tr;
	idVec3 move;

	blockingEntity = NULL;
	move = delta;
	for( i = 0; i < 3; i++ ) {
		gameLocal.clip.Translation( tr, start, start + move, clipModel, clipModel->GetAxis(), clipMask, self );
		//gameRenderWorld->DebugArrow(colorWhite, start, tr.endpos, 2, 5000);

		start = tr.endpos;

		if ( tr.fraction == 1.0f ) {
			if ( i > 0 ) {
				return MM_SLIDING;
			}
			return MM_OK;
		}

		if ( tr.c.entityNum != ENTITYNUM_NONE ) {
			blockingEntity = gameLocal.entities[ tr.c.entityNum ];
		} 
		
		// clip the movement delta and velocity
		move.ProjectOntoPlane( tr.c.normal, OVERCLIP );
		velocity.ProjectOntoPlane( tr.c.normal, OVERCLIP );
	}

	return MM_BLOCKED;
}

/*
=====================
idPhysics_Monster::StepMove

  move start into the delta direction
  the velocity is clipped conform any collisions
=====================
*/
monsterMoveResult_t idPhysics_Monster::StepMove( idVec3 &start, idVec3 &velocity, const idVec3 &delta ) {
	trace_t tr;
	idVec3 up, down, noStepPos, noStepVel, stepPos, stepVel;
	monsterMoveResult_t result1, result2;
	float	stepdist;
	float	nostepdist;

	if ( delta == vec3_origin ) {
		return MM_OK;
	}

	// try to move without stepping up
	noStepPos = start;
	noStepVel = velocity;
	result1 = SlideMove( noStepPos, noStepVel, delta );
	if ( result1 == MM_OK ) {
		velocity = noStepVel;
		if ( gravityNormal == vec3_zero ) {
			start = noStepPos;
			return MM_OK;
		}

		// try to step down so that we walk down slopes and stairs at a normal rate
		down = noStepPos + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( tr, noStepPos, down, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( tr.fraction < 1.0f ) {
			start = tr.endpos;
			return MM_STEPPED;
		} else {
			start = noStepPos;
			return MM_OK;
		}
	}

	if ( blockingEntity && blockingEntity->IsType( idActor::Type ) ) {
		// try to step down in case walking into an actor while going down steps
		down = noStepPos + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( tr, noStepPos, down, clipModel, clipModel->GetAxis(), clipMask, self );
		start = tr.endpos;
		velocity = noStepVel;
		return MM_BLOCKED;
	}

	if ( gravityNormal == vec3_zero ) {
		return result1;
	}

	// try to step up
	up = start - gravityNormal * maxStepHeight;
	gameLocal.clip.Translation( tr, start, up, clipModel, clipModel->GetAxis(), clipMask, self );
	//gameRenderWorld->DebugArrow(colorRed, start, up, 2, 5000);
	if ( tr.fraction == 0.0f ) {
		start = noStepPos;
		velocity = noStepVel;
		return result1;
	}

	// try to move at the stepped up position

	stepPos = tr.endpos;
	stepVel = velocity;
	result2 = SlideMove( stepPos, stepVel, delta );
	if ( result2 == MM_BLOCKED ) {
		start = noStepPos;
		velocity = noStepVel;
		return result1;
	}

	// step down again
	down = stepPos + gravityNormal * maxStepHeight;
	gameLocal.clip.Translation( tr, stepPos, down, clipModel, clipModel->GetAxis(), clipMask, self );
	//gameRenderWorld->DebugArrow(colorGreen, stepPos, down, 2, 5000);
	//gameRenderWorld->DebugArrow(colorBlue, tr.c.point, tr.c.point + 5 * tr.c.normal, 2, 5000);

	float projection = tr.c.normal * -gravityNormal;

	// greebo: We have collided with a steep slope in front of us
	if (projection < minFloorCosine && projection > 0.06f)
	{
		// greebo: Set the endposition a bit more upwards than necessary to prevent gravity from pulling us down immediately again
		stepPos = tr.endpos - gravityNormal * stepUpIncrease;
	}
	else
	{
		// No slope, just use the step position
		stepPos = tr.endpos;
	}

	// if the move is further without stepping up, or the slope is too steep, don't step up
	nostepdist = ( noStepPos - start ).LengthSqr();
	stepdist = ( stepPos - start ).LengthSqr();
	
	// Use the position that brought us the largest forward movement
	if (nostepdist >= stepdist) 
	{
		start = noStepPos;
		velocity = noStepVel;
		return MM_SLIDING;
	}

	// grayman #3989 - if this attempted step is being made during a "wake_up"/"fall_asleep" animation,
	// we can't let the vertical part go down/up. Otherwise, we'll see the animation jump up
	// for a few frames or the AI will end up floating above the bed.

	// use the current velocity so there's still horizontal movement
	idStr anim = idStr(static_cast<idAI*>(self)->WaitState());
	if ( (stepPos.z > noStepPos.z) && self->IsType(idAI::Type) && ((anim == "wake_up") || (anim == "fall_asleep")))
	{
		start = noStepPos;
	}
	else
	{
		start = stepPos;
	}

	velocity = stepVel;

	return MM_STEPPED;
}

/*
================
idPhysics_Monster::Activate
================
*/
void idPhysics_Monster::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
idPhysics_Monster::Rest
================
*/
void idPhysics_Monster::Rest( void ) {
	current.atRest = gameLocal.time;
	current.velocity.Zero();
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
idPhysics_Monster::PutToRest
================
*/
void idPhysics_Monster::PutToRest( void ) {
	Rest();
}

/*
================
idPhysics_Monster::idPhysics_Monster
================
*/
idPhysics_Monster::idPhysics_Monster( void ) {

	memset( &current, 0, sizeof( current ) );
	current.atRest = -1;
	saved = current;
	
	delta.Zero();
	maxStepHeight = 18.0f;
	stepUpIncrease = 10.0f;
	minFloorCosine = 0.7f;
	moveResult = MM_OK;
	forceDeltaMove = false;
	fly = false;
	useVelocityMove = false;
	noImpact = false;
	blockingEntity = NULL;
}

/*
================
idPhysics_Monster_SavePState
================
*/
void idPhysics_Monster_SavePState( idSaveGame *savefile, const monsterPState_t &state ) {
	savefile->WriteVec3( state.origin );
	savefile->WriteVec3( state.velocity );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteVec3( state.pushVelocity );
	savefile->WriteBool( state.onGround );
	savefile->WriteInt( state.atRest );
}

/*
================
idPhysics_Monster_RestorePState
================
*/
void idPhysics_Monster_RestorePState( idRestoreGame *savefile, monsterPState_t &state ) {
	savefile->ReadVec3( state.origin );
	savefile->ReadVec3( state.velocity );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadVec3( state.pushVelocity );
	savefile->ReadBool( state.onGround );
	savefile->ReadInt( state.atRest );
}

/*
================
idPhysics_Monster::Save
================
*/
void idPhysics_Monster::Save( idSaveGame *savefile ) const {

	idPhysics_Monster_SavePState( savefile, current );
	idPhysics_Monster_SavePState( savefile, saved );

	savefile->WriteFloat( maxStepHeight );
	savefile->WriteFloat(stepUpIncrease);
	savefile->WriteFloat( minFloorCosine );
	savefile->WriteVec3( delta );

	savefile->WriteBool( forceDeltaMove );
	savefile->WriteBool( fly );
	savefile->WriteBool( useVelocityMove );
	savefile->WriteBool( noImpact );
	
	savefile->WriteInt( (int)moveResult );
	savefile->WriteObject( blockingEntity );
}

/*
================
idPhysics_Monster::Restore
================
*/
void idPhysics_Monster::Restore( idRestoreGame *savefile ) {

	idPhysics_Monster_RestorePState( savefile, current );
	idPhysics_Monster_RestorePState( savefile, saved );

	savefile->ReadFloat( maxStepHeight );
	savefile->ReadFloat(stepUpIncrease);
	savefile->ReadFloat( minFloorCosine );
	savefile->ReadVec3( delta );

	savefile->ReadBool( forceDeltaMove );
	savefile->ReadBool( fly );
	savefile->ReadBool( useVelocityMove );
	savefile->ReadBool( noImpact );

	savefile->ReadInt( (int &)moveResult );
	savefile->ReadObject( reinterpret_cast<idClass *&>( blockingEntity ) );
}

/*
================
idPhysics_Monster::SetDelta
================
*/
void idPhysics_Monster::SetDelta( const idVec3 &d ) {
	delta = d;
	if ( delta != vec3_origin ) {
		Activate();
	}
}

/*
================
idPhysics_Monster::SetMaxStepHeight
================
*/
void idPhysics_Monster::SetMaxStepHeight( const float newMaxStepHeight ) {
	maxStepHeight = newMaxStepHeight;
}

/*
================
idPhysics_Monster::GetMaxStepHeight
================
*/
float idPhysics_Monster::GetMaxStepHeight( void ) const {
	return maxStepHeight;
}

void idPhysics_Monster::SetStepUpIncrease(float incr) {
	stepUpIncrease = incr;
}

/*
================
idPhysics_Monster::OnGround
================
*/
bool idPhysics_Monster::OnGround( void ) const {
	return current.onGround;
}

/*
================
idPhysics_Monster::GetSlideMoveEntity
================
*/
idEntity *idPhysics_Monster::GetSlideMoveEntity( void ) const {
	return blockingEntity;
}

/*
================
idPhysics_Monster::GetMoveResult
================
*/
monsterMoveResult_t idPhysics_Monster::GetMoveResult( void ) const {
	return moveResult;
}

/*
================
idPhysics_Monster::ForceDeltaMove
================
*/
void idPhysics_Monster::ForceDeltaMove( bool force ) {
	forceDeltaMove = force;
}

/*
================
idPhysics_Monster::UseFlyMove
================
*/
void idPhysics_Monster::UseFlyMove( bool force ) {
	fly = force;
}

/*
================
idPhysics_Monster::UseVelocityMove
================
*/
void idPhysics_Monster::UseVelocityMove( bool force ) {
	useVelocityMove = force;
}

/*
================
idPhysics_Monster::EnableImpact
================
*/
void idPhysics_Monster::EnableImpact( void ) {
	noImpact = false;
}

/*
================
idPhysics_Monster::DisableImpact
================
*/
void idPhysics_Monster::DisableImpact( void ) {
	noImpact = true;
}

/*
================
idPhysics_Monster::Evaluate
================
*/
bool idPhysics_Monster::Evaluate( int timeStepMSec, int endTimeMSec ) {

	if (timeStepMSec == 0)
	{
		// angua: time step can be zero when the AI comes back from being dormant
		return false;
	}

	idVec3 masterOrigin, oldOrigin;
	idMat3 masterAxis;
	float timeStep;

#ifdef MOD_WATERPHYSICS
	waterLevel = WATERLEVEL_NONE;		// MOD_WATERPHYSICS
	waterType = 0;						// MOD_WATERPHYSICS
#endif		// MOD_WATERPHYSICS

	timeStep = MS2SEC( timeStepMSec );

	moveResult = MM_OK;
	blockingEntity = NULL;
	oldOrigin = current.origin;

	// if bound to a master
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		current.velocity = ( current.origin - oldOrigin ) / timeStep;
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
		return true;
	}

	// if the monster is at rest
	if ( current.atRest >= 0 ) {
		return false;
	}

	ActivateContactEntities();

	// move the monster velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	clipModel->Unlink();

#ifdef MOD_WATERPHYSICS
	// check water level / type
	SetWaterLevel(true);		// MOD_WATERPHYSICS
#endif		// MOD_WATERPHYSICS

	// check if on the ground
	idPhysics_Monster::CheckGround( current );

	// if not on the ground or moving upwards
	float upspeed;
	if ( gravityNormal != vec3_zero ) {
		upspeed = -( current.velocity * gravityNormal );
	} else {
		upspeed = current.velocity.z;
	}
	if ( fly || ( !forceDeltaMove && ( !current.onGround || upspeed > 1.0f ) ) ) {
		if ( upspeed < 0.0f ) {
			moveResult = MM_FALLING;
		}
		else {
			current.onGround = false;
			moveResult = MM_OK;
		}
		delta = current.velocity * timeStep;
		if ( delta != vec3_origin ) {
			moveResult = idPhysics_Monster::SlideMove( current.origin, current.velocity, delta );
            delta.Zero();
		}

		if ( !fly ) {
			current.velocity += gravityVector * timeStep;
		}
	} else {
		if ( useVelocityMove ) {
			delta = current.velocity * timeStep;
		} else {
			current.velocity = delta / timeStep;
		}

		current.velocity -= ( current.velocity * gravityNormal ) * gravityNormal;

		if ( delta == vec3_origin ) {
			Rest();
		} else {
			// try moving into the desired direction
			moveResult = idPhysics_Monster::StepMove( current.origin, current.velocity, delta );
			delta.Zero();
		}
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	// get all the ground contacts
	EvaluateContacts();

	// move the monster velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString(0) );
		Rest();
	}

	return ( current.origin != oldOrigin );
}

/*
================
idPhysics_Monster::UpdateTime
================
*/
void idPhysics_Monster::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_Monster::GetTime
================
*/
int idPhysics_Monster::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
idPhysics_Monster::GetImpactInfo
================
*/
void idPhysics_Monster::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
idPhysics_Monster::ApplyImpulse
================
*/
void idPhysics_Monster::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( noImpact ) {
		return;
	}
	current.velocity += impulse * invMass;
	Activate();
}

/*
================
idPhysics_Monster::IsAtRest
================
*/
bool idPhysics_Monster::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
idPhysics_Monster::GetRestStartTime
================
*/
int idPhysics_Monster::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
idPhysics_Monster::SaveState
================
*/
void idPhysics_Monster::SaveState( void ) {
	saved = current;
}

/*
================
idPhysics_Monster::RestoreState
================
*/
void idPhysics_Monster::RestoreState( void ) {
	current = saved;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	EvaluateContacts();
}

/*
================
idPhysics_Monster::SetOrigin
================
*/
void idPhysics_Monster::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.origin = newOrigin;
	}
	clipModel->Link( gameLocal.clip, self, 0, newOrigin, clipModel->GetAxis() );
	Activate();
}

/*
================
idPhysics_Player::SetAxis
================
*/
void idPhysics_Monster::SetAxis( const idMat3 &newAxis, int id ) {
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), newAxis );
	Activate();
}

/*
================
idPhysics_Monster::Translate
================
*/
void idPhysics_Monster::Translate( const idVec3 &translation, int id ) {

	current.localOrigin += translation;
	current.origin += translation;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	Activate();
}

/*
================
idPhysics_Monster::Rotate
================
*/
void idPhysics_Monster::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.origin *= rotation;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	}
	else {
		current.localOrigin = current.origin;
	}
	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
	Activate();
}

/*
================
idPhysics_Monster::SetLinearVelocity
================
*/
void idPhysics_Monster::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
	Activate();
}

/*
================
idPhysics_Monster::GetLinearVelocity
================
*/
const idVec3 &idPhysics_Monster::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
idPhysics_Monster::SetPushed
================
*/
void idPhysics_Monster::SetPushed( int deltaTime ) {
	// velocity with which the monster is pushed
	current.pushVelocity += ( current.origin - saved.origin ) / ( deltaTime * idMath::M_MS2SEC );
}

/*
================
idPhysics_Monster::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_Monster::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

/*
================
idPhysics_Monster::SetMaster

  the binding is never orientated
================
*/
void idPhysics_Monster::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			masterEntity = master;
			masterYaw = masterAxis[0].ToYaw();
		}
		ClearContacts();
	}
	else {
		if ( masterEntity ) {
			masterEntity = NULL;
			Activate();
		}
	}
}

const float	MONSTER_VELOCITY_MAX			= 4000;
const int	MONSTER_VELOCITY_TOTAL_BITS		= 16;
const int	MONSTER_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( MONSTER_VELOCITY_MAX ) ) + 1;
const int	MONSTER_VELOCITY_MANTISSA_BITS	= MONSTER_VELOCITY_TOTAL_BITS - 1 - MONSTER_VELOCITY_EXPONENT_BITS;

/*
================
idPhysics_Monster::WriteToSnapshot
================
*/
void idPhysics_Monster::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteFloat( current.origin[0] );
	msg.WriteFloat( current.origin[1] );
	msg.WriteFloat( current.origin[2] );
	msg.WriteFloat( current.velocity[0], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[1], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[2], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteLong( current.atRest );
	msg.WriteBits( current.onGround, 1 );
}

/*
================
idPhysics_Monster::ReadFromSnapshot
================
*/
void idPhysics_Monster::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();
	current.velocity[0] = msg.ReadFloat( MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.velocity[1] = msg.ReadFloat( MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.velocity[2] = msg.ReadFloat( MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.atRest = msg.ReadLong();
	current.onGround = msg.ReadBits( 1 ) != 0;
}
