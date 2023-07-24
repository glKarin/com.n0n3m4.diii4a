// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Monster.h"
#include "../Entity.h"
#include "../Actor.h"
#include "../Player.h"
#include "../../decllib/DeclSurfaceType.h"

CLASS_DECLARATION( idPhysics_Actor, idPhysics_Monster )
END_CLASS

const float PM_OVERCLIP			= 1.001f;
const float MIN_WALK_NORMAL		= 0.7f;		// can't walk on very steep slopes
const float CONST_PM_STEPSCALE	= 1.0f;

idCVar g_drawContacts( "g_drawContacts", "0", CVAR_BOOL | CVAR_GAME, "draw physics object contacts" );

/*
=====================
sdMonsterPhysicsNetworkData::MakeDefault
=====================
*/
void sdMonsterPhysicsNetworkData::MakeDefault( void ) {
	origin			= vec3_origin;
	velocity		= vec3_zero;
}

/*
=====================
sdMonsterPhysicsNetworkData::Write
=====================
*/
void sdMonsterPhysicsNetworkData::Write( idFile* file ) const {
	file->WriteVec3( origin );
	file->WriteVec3( velocity );
}

/*
=====================
sdMonsterPhysicsNetworkData::Read
=====================
*/
void sdMonsterPhysicsNetworkData::Read( idFile* file ) {
	file->ReadVec3( origin );
	file->ReadVec3( velocity );

	origin.FixDenormals();
	velocity.FixDenormals();
}

/*
=====================
sdMonsterPhysicsBroadcastData::MakeDefault
=====================
*/
void sdMonsterPhysicsBroadcastData::MakeDefault( void ) {
	pushVelocity	= vec3_zero;
}

/*
=====================
sdMonsterPhysicsBroadcastData::Write
=====================
*/
void sdMonsterPhysicsBroadcastData::Write( idFile* file ) const {
	file->WriteVec3( pushVelocity );
	file->WriteInt( atRest );
}

/*
=====================
sdMonsterPhysicsBroadcastData::Read
=====================
*/
void sdMonsterPhysicsBroadcastData::Read( idFile* file ) {
	file->ReadVec3( pushVelocity );
	file->ReadInt( atRest );
}

/*
=====================
idPhysics_Monster::CheckGround
=====================
*/
void idPhysics_Monster::CheckGround( void ) {
	int i;//, contents;
	idVec3 point;
	bool hadGroundContacts = HasGroundContacts();

	// set the clip model origin before getting the contacts
	clipModel->SetPosition( current.origin, clipModel->GetAxis(), gameLocal.clip );

	EvaluateContacts( CLIP_DEBUG_PARMS_ONLY );

	// setup a ground trace from the contacts
	groundTrace.endpos = current.origin;
	groundTrace.endAxis = clipModel->GetAxis();
	if ( contacts.Num() ) {
		groundTrace.fraction = 0.0f;
		groundTrace.c = contacts[0];
		for ( i = 1; i < contacts.Num(); i++ ) {
			groundTrace.c.normal += contacts[i].normal;
		}
		groundTrace.c.normal.Normalize();
	} else {
		groundTrace.fraction = 1.0f;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( groundTrace.fraction == 1.0f ) {
		current.onGround = false;
		walking = false;
		groundEntityPtr = NULL;
		return;

	}

	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	// check if getting thrown off the ground
	if ( (current.velocity * -gravityNormal) > 0.0f && ( current.velocity * groundTrace.c.normal ) > 10.0f ) {
		current.onGround = false;
		walking = false;
		return;
	}
	
	current.onGround = true;
	walking = true;

	// let the entity know about the collision
	groundEntityPtr->Hit( groundTrace, current.velocity, self );
	self->Collide( groundTrace, current.velocity, -1 );
}

/*
=====================
idPhysics_Monster::SlideMove
=====================
*/
#define	MAX_CLIP_PLANES	5
#define	MAX_BUMPS		4

bool idPhysics_Monster::SlideMove( bool gravity, bool stepUp, bool stepDown, bool push, int vehiclePush ) {
	int			i, j, k, pushFlags;
	int			bumpcount, numplanes;
	float		d, time_left, into, totalMass;
	idVec3		dir, planes[MAX_CLIP_PLANES];
	idVec3		end, stepEnd, primal_velocity, endVelocity, endClipVelocity, clipVelocity;
	trace_t		trace, stepTrace, downTrace;
	bool		nearGround, stepped, pushed, vehiclePushed;

	primal_velocity = current.velocity;

	if ( gravity ) {
		endVelocity = current.velocity + gravityVector * frametime;
		current.velocity = ( current.velocity + endVelocity ) * 0.5f;
		primal_velocity = endVelocity;
		if ( current.onGround ) {
			// slide along the ground plane
			current.velocity.ProjectOntoPlane( groundTrace.c.normal, PM_OVERCLIP );
		}
	} else {
		endVelocity = current.velocity;
	}

	time_left = frametime;

	// never turn against the ground plane
	if ( current.onGround ) {
		numplanes = 1;
		planes[0] = groundTrace.c.normal;
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	planes[numplanes] = current.velocity;
	planes[numplanes].Normalize();
	numplanes++;

	for ( bumpcount = 0; bumpcount < MAX_BUMPS; bumpcount++ ) {

		// calculate position we are trying to move to
		end = current.origin + time_left * current.velocity;

		// see if we can make it there
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

		time_left -= time_left * trace.fraction;
		current.origin = trace.endpos;

		// if moved the entire distance
		if ( trace.fraction >= 1.0f ) {
			break;
		}

		stepped = pushed = vehiclePushed = false;

		// if we are allowed to step up
		if ( stepUp && ( trace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {

			nearGround = current.onGround;

			if ( !nearGround ) {
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground
			if ( nearGround ) {

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) {

					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) {
						time_left = 0.0f;						
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.velocity *= CONST_PM_STEPSCALE;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) {
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.velocity *= CONST_PM_STEPSCALE;
						trace = stepTrace;
						stepped = true;
					}
				}
			}
		}

		// if we can push other entities and not blocked by the world
		if ( push && trace.c.entityNum != ENTITYNUM_WORLD ) {

			clipModel->SetPosition( current.origin, clipModel->GetAxis(), gameLocal.clip );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP|PUSHFL_ONLYMOVEABLE|PUSHFL_NOGROUNDENTITIES|PUSHFL_APPLYIMPULSE;

			// clip & push
			totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin, clipModel );

			if ( totalMass > 0.0f ) {
				// decrease velocity based on the total mass of the objects being pushed ?
				current.velocity *= 1.0f - idMath::ClampFloat( 0.0f, 1000.0f, totalMass - 20.0f ) * ( 1.0f / 950.0f );
				pushed = true;
			}
	
			current.origin = trace.endpos;
			time_left -= time_left * trace.fraction;

			// if moved the entire distance
			if ( trace.fraction >= 1.0f ) {
				break;
			}
		}

		// try to vehiclepush things out of the way
		if ( !stepped && !pushed && vehiclePush && trace.c.entityNum != ENTITYNUM_WORLD ) {
			idEntity* other = gameLocal.entities[ trace.c.entityNum ];
			idPhysics* otherPhysics = other->GetPhysics();
			idPhysics_Actor* actorPhysics = otherPhysics->Cast< idPhysics_Actor >();

			if ( actorPhysics != NULL ) {
				clipModel->Disable();
				idVec3 move = end - trace.endpos;
				if ( actorPhysics->VehiclePush( false, time_left, move, clipModel, vehiclePush ) == VPUSH_OK ) {
					vehiclePushed = true;
				}
				clipModel->Enable();
			}
		}

		if ( !stepped && !vehiclePushed && !vehiclePush ) {
			// let the entity know about the collision
			idEntity *ent = gameLocal.entities[ trace.c.entityNum ];
			ent->Hit( trace, current.velocity, self );			
			self->Collide( trace, current.velocity, -1 );
		}

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// MrElusive: I think we have some relatively high poly LWO models with a lot of slanted tris
			// where it may hit the max clip planes
			current.velocity = vec3_origin;
			return true;
		}

		//
		// if this is the same plane we hit before, nudge velocity out along it,
		// which fixes some epsilon issues with non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( ( trace.c.normal * planes[i] ) > 0.999f ) {
				// clip into the trace normal just in case this normal is almost but not exactly the same as the groundTrace normal
				current.velocity.ProjectOntoPlane( trace.c.normal, PM_OVERCLIP );
				// also add the normal to nudge the velocity out
				current.velocity += trace.c.normal;
				break;
			}
		}
		if ( i < numplanes ) {
			continue;
		}
		planes[numplanes] = trace.c.normal;
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ ) {
			into = current.velocity * planes[i];
			if ( into >= 0.1f ) {
				continue;		// move doesn't interact with the plane
			}

			// slide along the plane
			clipVelocity = current.velocity;
			clipVelocity.ProjectOntoPlane( planes[i], PM_OVERCLIP );

			// slide along the plane
			endClipVelocity = endVelocity;
			endClipVelocity.ProjectOntoPlane( planes[i], PM_OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( ( clipVelocity * planes[j] ) >= 0.1f ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				clipVelocity.ProjectOntoPlane( planes[j], PM_OVERCLIP );
				endClipVelocity.ProjectOntoPlane( planes[j], PM_OVERCLIP );

				// see if it goes back into the first clip plane
				if ( ( clipVelocity * planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * current.velocity;
				clipVelocity = d * dir;

				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * endVelocity;
				endClipVelocity = d * dir;

				// see if there is a third plane the the new move enters
				for ( k = 0; k < numplanes; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( ( clipVelocity * planes[k] ) >= 0.1f ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					current.velocity = vec3_origin;
					return true;
				}
			}

			// if we have fixed all interactions, try another move
			current.velocity = clipVelocity;
			endVelocity = endClipVelocity;
			break;
		}
	}

	// step down
	if ( stepDown && current.onGround ) {
		stepEnd = current.origin + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) {
			current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
			current.origin = downTrace.endpos;
			current.velocity *= CONST_PM_STEPSCALE;
		}
	}

	if ( gravity ) {
		current.velocity = endVelocity;
	}

	// come to a dead stop when the velocity orthogonal to the gravity flipped
	clipVelocity = current.velocity - gravityNormal * current.velocity * gravityNormal;
	endClipVelocity = endVelocity - gravityNormal * endVelocity * gravityNormal;
	if ( clipVelocity * endClipVelocity < 0.0f ) {
		current.velocity = gravityNormal * current.velocity * gravityNormal;
	}

	return bumpcount == 0;
}

/*
================
idPhysics_Monster::idPhysics_Monster
================
*/
idPhysics_Monster::idPhysics_Monster( void ) {

	memset( &current, 0, sizeof( current ) );
	saved = current;
	
	delta.Zero();
	maxStepHeight = 18.0f;
	useVelocityMove = false;
	noImpact = false;
	blockingEntity = NULL;
	waterLevel = 0.0f;

	isStable = true;

	atRest = -1;
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
idPhysics_Monster::Activate
================
*/
void idPhysics_Monster::Activate( void ) {
	atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
idPhysics_Monster::PutToRest
================
*/
void idPhysics_Monster::PutToRest( void ) {
	atRest = gameLocal.time;
}

/*
================
idPhysics_Monster::IsAtRest
================
*/
bool idPhysics_Monster::IsAtRest( void ) {
	return atRest >= 0;
}

/*
================
idPhysics_Monster::Evaluate
================
*/
bool idPhysics_Monster::Evaluate( int timeStepMSec, int endTimeMSec ) {
	DebugDraw();

	idVec3 masterOrigin, oldOrigin;
	idMat3 masterAxis;

	current.origin.FixDenormals();
	current.velocity.FixDenormals();
	current.pushVelocity.FixDenormals();

	// determine the time
	framemsec = timeStepMSec;
	frametime = MS2SEC( framemsec );

	if ( IsAtRest() ) {
		return false;
	}

	if ( current.onGround ) {
		if ( ( current.velocity - current.pushVelocity ).IsZero() && delta.IsZero() ) {
			if ( ( groundTrace.c.normal * -gravityNormal ) >= MIN_WALK_NORMAL ) {
				PutToRest();
				return false;
			}
		}
	}

	if ( timeStepMSec == 0 ) {
		return false;
	}

	walking = false;
	current.onGround = false;

	blockingEntity = NULL;
	oldOrigin = current.origin;

	current.stepUp = 0.0f;

	ActivateContactEntities();

	// move the monster velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	clipModel->Unlink( gameLocal.clip );

	CheckWater();

	// check if on the ground
	CheckGround();

	idVec3 newVelocity = current.velocity - current.pushVelocity;	

	if ( !isStable ) {
		current.onGround = false;
		walking = false;
	}

	if ( walking ) {
		newVelocity = ( ( newVelocity * gravityNormal ) * gravityNormal );
		newVelocity += ( delta / frametime );
	}

	current.velocity = newVelocity;

	if ( walking ) {
		WalkMove();
	} else {
		AirMove();
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	// get all the ground contacts
	EvaluateContacts( CLIP_DEBUG_PARMS_ONLY );

	// move the monster velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();

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

	EvaluateContacts( CLIP_DEBUG_PARMS_ONLY );
}

/*
================
idPhysics_Player::SetOrigin
================
*/
void idPhysics_Monster::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

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

const float	MONSTER_ORIGIN_MAX				= 32767.0f;
const int	MONSTER_ORIGIN_TOTAL_BITS		= 24;
const int	MONSTER_ORIGIN_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( MONSTER_ORIGIN_MAX ) ) + 1;
const int	MONSTER_ORIGIN_MANTISSA_BITS	= MONSTER_ORIGIN_TOTAL_BITS - 1 - MONSTER_ORIGIN_EXPONENT_BITS;

const float	MONSTER_VELOCITY_MAX			= 4000;
const int	MONSTER_VELOCITY_TOTAL_BITS		= 16;
const int	MONSTER_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( MONSTER_VELOCITY_MAX ) ) + 1;
const int	MONSTER_VELOCITY_MANTISSA_BITS	= MONSTER_VELOCITY_TOTAL_BITS - 1 - MONSTER_VELOCITY_EXPONENT_BITS;

/*
================
idPhysics_Monster::CheckNetworkStateChanges
================
*/
bool idPhysics_Monster::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdMonsterPhysicsNetworkData );

		if ( !baseData.origin.Compare( current.origin, 0.01f ) ) {
			return true;
		}

		if ( !baseData.velocity.Compare( current.velocity, 0.01f ) ) {
			return true;
		}		
		
		return false;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdMonsterPhysicsBroadcastData );

		if ( baseData.pushVelocity != current.pushVelocity ) {
			return true;
		}

		if ( baseData.atRest != atRest ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
idPhysics_Monster::WriteNetworkState
================
*/
void idPhysics_Monster::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdMonsterPhysicsNetworkData );

		newData.origin			= current.origin;
		newData.velocity		= current.velocity;

		msg.WriteDeltaVector( baseData.origin, newData.origin, MONSTER_ORIGIN_EXPONENT_BITS, MONSTER_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.velocity, newData.velocity, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdMonsterPhysicsBroadcastData );

		newData.pushVelocity	= current.pushVelocity;
		newData.atRest			= atRest;

		msg.WriteDeltaVector( baseData.pushVelocity, newData.pushVelocity, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaLong( baseData.atRest, newData.atRest );

		return;
	}
}

/*
================
idPhysics_Monster::ApplyNetworkState
================
*/
void idPhysics_Monster::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdMonsterPhysicsNetworkData );

		current.origin			= newData.origin;
		current.velocity		= newData.velocity;

		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		self->UpdateVisuals();
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdMonsterPhysicsBroadcastData );

		current.pushVelocity	= newData.pushVelocity;
		atRest					= newData.atRest;

		return;
	}
}

/*
================
idPhysics_Monster::ReadNetworkState
================
*/
void idPhysics_Monster::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdMonsterPhysicsNetworkData );

		newData.origin = msg.ReadDeltaVector( baseData.origin, MONSTER_ORIGIN_EXPONENT_BITS, MONSTER_ORIGIN_MANTISSA_BITS );
		newData.velocity = msg.ReadDeltaVector( baseData.velocity, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );

		newData.origin.FixDenormals();
		newData.velocity.FixDenormals();

		self->OnNewOriginRead( newData.origin );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdMonsterPhysicsBroadcastData );

		newData.pushVelocity	= msg.ReadDeltaVector( baseData.pushVelocity, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
		newData.atRest			= msg.ReadDeltaLong( baseData.atRest );

		return;
	}
}

/*
================
idPhysics_Monster::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idPhysics_Monster::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdMonsterPhysicsNetworkData();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdMonsterPhysicsBroadcastData();
	}
	return NULL;
}

/*
================
idPhysics_Monster::CheckWater
================
*/
void idPhysics_Monster::CheckWater( void ) {
	waterLevel = 0.0f;

	const idBounds& absBounds = GetAbsBounds( -1 );

	const idClipModel* clipModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS absBounds, CONTENTS_WATER, &clipModel, 1, NULL );
	if ( !count ) {
		return;
	}

	if ( !clipModel->GetNumCollisionModels() ) {
		return;
	}

	idCollisionModel* model = clipModel->GetCollisionModel( 0 );
	int numPlanes = model->GetNumBrushPlanes();
	if ( !numPlanes ) {
		return;
	}
	const idBounds& modelBounds = model->GetBounds();

	self->CheckWater( clipModel->GetOrigin(), clipModel->GetAxis(), model );

	idVec3 waterCurrent;
	clipModel->GetEntity()->GetWaterCurrent( waterCurrent );

	float scratch[ MAX_TRACEMODEL_WATER_POINTS ];
	float totalVolume = 0.0f;
	float waterVolume = 0.0f;

	{
		idClipModel* bodyClip = this->clipModel;
		const traceModelWater_t* waterPoints = bodyClip->GetWaterPoints();
		if ( waterPoints == NULL ) {
			return;
		}
		float volume = bodyClip->GetTraceModelVolume();
		totalVolume += volume;

		for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
			scratch[ i ] = waterPoints[ i ].weight * volume;
		}

		for ( int l = 0; l < numPlanes; l++ ) {
			idPlane plane = model->GetBrushPlane( l );
			plane.TranslateSelf( clipModel->GetOrigin() - bodyClip->GetOrigin() );
			plane.Normal() *= clipModel->GetAxis();

			for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
				if ( plane.Distance( waterPoints[ i ].xyz ) > 0 ) {
					scratch[ i ] = 0.f;
				}
			}
		}

		float height = clipModel->GetOrigin().z - bodyClip->GetOrigin().z + modelBounds.GetMaxs().z;
		idPlane plane( mat3_identity[ 2 ], height );

		for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
			if ( !scratch[ i ] ) {
				continue;
			}

			scratch[ i ] *= Min( -plane.Distance( waterPoints[ i ].xyz ) / 16.f, 1.f );
/*
			idVec3 impulse = scratch[ i ] * ( ( -gravityNormal * buoyancy ) + ( waterCurrent * inverseMass ) );

			idVec3 org = bodyClip->GetOrigin() + ( bodyClip->GetAxis() * waterPoints[ i ].xyz );

			current.i.linearMomentum += impulse;
			current.i.angularMomentum += ( org - ( current.i.position + centerOfMass * current.i.orientation ) ).Cross( impulse );
*/
			waterVolume += scratch[ i ];
		}
	}

	if ( totalVolume > idMath::FLT_EPSILON ) {
		waterLevel = waterVolume / totalVolume;
	}

/*	if ( waterLevel ) {
		Activate();
	}*/
}

/*
================
idPhysics_Monster::WalkMove
================
*/
void idPhysics_Monster::WalkMove( void ) {
	idVec3		oldVelocity, vel;
	float		oldVel, newVel;

	oldVelocity = current.velocity;

	// slide along the ground plane
	current.velocity.ProjectOntoPlane( groundTrace.c.normal, PM_OVERCLIP );

	// if not clipped into the opposite direction
	if ( oldVelocity * current.velocity > 0.0f ) {
		newVel = current.velocity.LengthSqr();
		if ( newVel > 1.0f ) {
			oldVel = oldVelocity.LengthSqr();
			if ( oldVel > 1.0f ) {
				// don't decrease velocity when going up or down a slope
				current.velocity *= idMath::Sqrt( oldVel / newVel );
			}
		}
	}

	// don't do anything if standing still
	vel = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	if ( !vel.LengthSqr() ) {
		return;
	}

	gameLocal.push.InitSavingPushedEntityPositions();

	SlideMove( false, true, true, true, 0 );
}

/*
================
idPhysics_Monster::AirMove
================
*/
void idPhysics_Monster::AirMove( void ) {
	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( current.onGround ) {
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, PM_OVERCLIP );
	}

	SlideMove( true, false, false, false, 0 );
}

/*
================
idPhysics_Monster::DebugDraw
================
*/
void idPhysics_Monster::DebugDraw( void ) {
	if ( g_drawContacts.GetBool() ) {
		for( int i = 0; i < contacts.Num(); i++ ) {
			contactInfo_t& contact = contacts[ i ];

			idVec3 x, y;
			contact.normal.NormalVectors( x, y );
			gameRenderWorld->DebugLine( colorBlue, contact.point, contact.point + 12.0f * contact.normal );
			gameRenderWorld->DebugLine( colorBlue, contact.point - 4.0f * x, contact.point + 4.0f * x );
			gameRenderWorld->DebugLine( colorBlue, contact.point - 4.0f * y, contact.point + 4.0f * y );
			if( contact.surfaceType ) {
				gameRenderWorld->DrawText( contact.surfaceType->GetName(), contact.point, 0.2f, colorWhite, gameLocal.GetLocalViewPlayer()->GetRenderView()->viewaxis );
			}
		}
	}
}

/*
================
idPhysics_Monster::VehiclePush
================
*/
#define VEHICLE_PUSH_EPSILON	0.5f

int idPhysics_Monster::VehiclePush( bool stuck, float timeDelta, idVec3& move, idClipModel* pusher, int pushCount ) {

	if ( pushCount > 3 ) {
		move.Zero();
		return VPUSH_BLOCKED;
	}

	// remove components into the ground
	if ( current.onGround ) {
		float groundMove = move * groundTrace.c.normal;
		if ( groundMove < 0.0f ) {
			move -= groundMove * groundTrace.c.normal;
		}
	}


	if ( !stuck ) {
		// change the velocity so it satisfies the push
		// note it has to be changed, then restored back
		// could combine it and the required velocity of the move now, but it'd
		// mean that you get double-moved by the current velocity - exploits++
		idVec3 oldVelocity = current.velocity;
		idVec3 oldOrigin = current.origin;
		frametime = timeDelta;
		framemsec = SEC2MS( timeDelta );
		if ( framemsec < 1 ) {
			framemsec = 1;
			frametime = MS2SEC( 1 );
		}
		current.velocity = move / frametime;

		if ( framemsec >= 1 ) {
	//		pusher->Disable();
			SlideMove( false, true, false, false, pushCount + 1 );
	//		pusher->Enable();

			CheckWater();
			CheckGround();	
			
			clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
			self->UpdateVisuals();
		}

		// see if we ended up moving the right way
		idVec3 moveDirection = move;
		float moveNeeded = moveDirection.Normalize();
		idVec3 movedDelta = current.origin - oldOrigin;
		float movedAmount = moveDirection * movedDelta;

		current.velocity = oldVelocity;
		float oldMoveDirSpeed = current.velocity * moveDirection;
		current.pushVelocity = ( moveNeeded - oldMoveDirSpeed ) * moveDirection;
		// remove pushvelocity into the ground so it doesn't think its falling damage
		if ( groundTrace.fraction < 1.0f ) {
			float intoGroundVel = -current.pushVelocity * groundTrace.c.normal;
			if ( intoGroundVel > 0.0f ) {
				current.pushVelocity += intoGroundVel * groundTrace.c.normal;
			}
		}
		current.velocity += current.pushVelocity ;


//		gameRenderWorld->DebugArrow( colorYellow, current.origin + idVec3(0,0,64), current.origin + idVec3(0,0,64) + moveDirection * 128.0f, 4 );
		if ( movedAmount < moveNeeded - VEHICLE_PUSH_EPSILON ) {
			// didn't move all the way!
			// update the amount that we move
			move = move * movedAmount / moveNeeded;
			return VPUSH_BLOCKED;
		}
	} else {
		// that means we're inside the vehicle and its trying to push us out
		// this is normally caused because we're being pushed by the vehicle
		// and we try to walk towards it

		// try to find a point on the outside of the vehicle that we can teleport to
		// start by finding the normal of a nearby surface
		contactInfo_t contacts[ 2 ];
		int numContacts = gameLocal.clip.ContactsModel( CLIP_DEBUG_PARMS contacts, 2, current.origin, NULL, 4.0f, clipModel, clipModel->GetAxis(), -1, pusher, pusher->GetOrigin(), pusher->GetAxis() );
		idVec3 foundNormal = vec3_origin;

		if ( numContacts ) {
			// average out the normals of the contacts - this provides a good approximation
			for ( int i = 0; i < numContacts; i++ ) {
//				gameRenderWorld->DebugSphere( colorGreen, idSphere( contacts[ i ].point, 4.0f ) );
//				gameRenderWorld->DebugArrow( colorGreen, contacts[ i ].point, contacts[ i ].point + contacts[ i ].normal * 128.0f, 4 );

				foundNormal += contacts[ i ].normal;
			}
			foundNormal /= numContacts;
		} else {
			// embedded so far it doesn't find any contacts O_o
			// use a trace to try to find the normal
			idVec3 traceDir = pusher->GetAbsBounds().GetCenter() - current.origin;
			float traceDirLength = traceDir.Normalize();
			if ( traceDirLength == 0.0f ) {
				// well and truly embedded - nothing we can do here
				move.Zero();
				return VPUSH_BLOCKED;
			}

			idVec3 normalFinderStart = current.origin - traceDir * 128.0f;
			idVec3 normalFinderEnd = current.origin + traceDir * 128.0f;

			trace_t normalFinder;
			gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS normalFinder, normalFinderStart, normalFinderEnd, clipModel, 
																clipModel->GetAxis(), GetClipMask(), pusher,
																pusher->GetOrigin(), pusher->GetAxis() );
			if ( normalFinder.fraction == 1.0f ) {
				// well and truly embedded - nothing we can do here
				move.Zero();
				return VPUSH_BLOCKED;
			}

			foundNormal = normalFinder.c.normal;
		}

//			gameRenderWorld->DebugArrow( colorYellow, current.origin, current.origin + foundNormal * 128.0f, 4 );
		idVec3 start = current.origin + foundNormal * 32.0f;
		idVec3 end = current.origin - foundNormal * 32.0f;

		trace_t tr;
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS tr, start, end, clipModel, clipModel->GetAxis(), GetClipMask(), pusher,
																	pusher->GetOrigin(), pusher->GetAxis() );

		if ( tr.fraction < 1.0f ) {
			// push to the new spot
			idEntity* other = pusher->GetEntity();
			if ( other != NULL ) {
				other->DisableClip( false );
			} else {
				pusher->Disable();
			}

			idVec3 pushOutMove = tr.endpos - current.origin;
			int pushOutResult = VehiclePush( false, timeDelta, pushOutMove, pusher, pushCount );

			if ( other != NULL ) {
				other->EnableClip();
			} else {
				pusher->Enable();
			}

			if ( pushOutResult == VPUSH_OK ) {
				// heres a point thats not inside the vehicle, and not inside the world! move there, use it as our new origin
				// proceed with the move, adjust it for the new origin
				return VehiclePush( false, timeDelta, move, pusher, pushCount );
			} else {
				return VPUSH_BLOCKED;
			}
		}
	}

	return VPUSH_OK;
}
