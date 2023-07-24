// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Player.h"
#include "../Entity.h"
#include "../Player.h"
#include "../ContentMask.h"
#include "../Misc.h"
#include "../vehicles/Transport.h"
#include "Physics_RigidBodyMultiple.h"
#include "../botai/Bot.h"

CLASS_DECLARATION( idPhysics_Actor, idPhysics_Player )
END_CLASS

const float PM_OVERCLIP			= 1.001f;

const float CONST_PM_WATERFRAC_WAIST = 0.5f;
const float CONST_PM_WATERFRAC_HEAD = 1.f;

// movement parameters
const float CONST_PM_STOPSPEED			= 100.0f;
const float CONST_PM_SWIMSCALE			= 0.5f;
const float CONST_PM_LADDERSPEED		= 100.0f;
const float CONST_PM_STEPSCALE			= 1.0f;

const float CONST_PM_ACCELERATE			= 10.0f;
const float CONST_PM_AIRACCELERATE		= 1.0f;
const float CONST_PM_WATERACCELERATE	= 4.0f;
const float CONST_PM_FLYACCELERATE		= 8.0f;

// const float CONST_PM_FRICTION			= 6.0f;
const float CONST_PM_AIRFRICTION		= 0.2f;
const float CONST_PM_WATERFRICTION		= 1.0f;
const float CONST_PM_FLYFRICTION		= 6.0f;
const float CONST_PM_NOCLIPFRICTION		= 12.0f;

const float CONST_PM_LEANRATE			= 1.f / 0.3f;
const float CONST_PM_LEANMAX			= 21.f;

const float CONST_PM_LADDERSLIDESCALE	= 0.75f;

const float MIN_WALK_NORMAL		= 0.7f;		// can't walk on very steep slopes


/*
============
sdPlayerPhysicsNetworkData::MakeDefault
============
*/
void sdPlayerPhysicsNetworkData::MakeDefault( void ) {
	origin = vec3_origin;
	velocity = vec3_zero;
	movementTime = 0;
	movementFlags = 0;
}

/*
============
sdPlayerPhysicsNetworkData::Write
============
*/
void sdPlayerPhysicsNetworkData::Write( idFile* file ) const {
	file->WriteVec3( origin );
	file->WriteVec3( velocity );
	file->WriteInt( movementTime );
	file->WriteInt( movementFlags );
}

/*
============
sdPlayerPhysicsNetworkData::Read
============
*/
void sdPlayerPhysicsNetworkData::Read( idFile* file ) {
	file->ReadVec3( origin );
	file->ReadVec3( velocity );
	file->ReadInt( movementTime );
	file->ReadInt( movementFlags );

	origin.FixDenormals();
	velocity.FixDenormals();
}

/*
============
sdPlayerPhysicsBroadcastData::MakeDefault
============
*/
void sdPlayerPhysicsBroadcastData::MakeDefault( void ) {
	pushVelocity = vec3_zero;
	localOrigin = vec3_origin;
	frozen = false;
	proneChangeEndTime = 0;
	jumpAllowedTime = 0;
}

/*
============
sdPlayerPhysicsBroadcastData::Write
============
*/
void sdPlayerPhysicsBroadcastData::Write( idFile* file ) const {
	file->WriteVec3( pushVelocity );
	file->WriteVec3( localOrigin );
	file->WriteBool( frozen );
	file->WriteInt( proneChangeEndTime );
	file->WriteInt( jumpAllowedTime );
}

/*
============
sdPlayerPhysicsBroadcastData::Read
============
*/
void sdPlayerPhysicsBroadcastData::Read( idFile* file ) {
	file->ReadVec3( pushVelocity );
	file->ReadVec3( localOrigin );
	file->ReadBool( frozen );
	file->ReadInt( proneChangeEndTime );
	file->ReadInt( jumpAllowedTime );
}

/*
============
idPhysics_Player::CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
float idPhysics_Player::CmdScale( const usercmd_t &cmd, bool noVertical ) const {
	int		max;
	float	total;
	float	scale;
	int		forwardmove;
	int		rightmove;
	int		upmove;

	forwardmove = cmd.forwardmove;
	rightmove = cmd.rightmove;

	// since the crouch key doubles as downward movement, ignore downward movement when we're on the ground
	// otherwise crouch speed will be lower than specified
	upmove = noVertical ? 0 : cmd.upmove;

	max = abs( forwardmove );
	if ( abs( rightmove ) > max ) {
		max = abs( rightmove );
	}
	if ( abs( upmove ) > max ) {
		max = abs( upmove );
	}

	if ( !max ) {
		return 0.0f;
	}

	total = idMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
	scale = (float) playerSpeed * max / ( 127.0f * total );

	return scale;
}

/*
==============
idPhysics_Player::Accelerate

Handles user intended acceleration
==============
*/
void idPhysics_Player::Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel ) {
#if 1
	// q2 style
	float addspeed, accelspeed, currentspeed;

	currentspeed = current.velocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel * frametime * wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}

	current.velocity += accelspeed * wishdir;
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	idVec3		wishVelocity;
	idVec3		pushDir;
	float		pushLen;
	float		canPush;

	wishVelocity = wishdir * wishspeed;
	pushDir = wishVelocity - current.velocity;
	pushLen = pushDir.Normalize();

	canPush = accel * frametime * wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	current.velocity += canPush * pushDir;
#endif
}

const idBounds	playerProneLegsBounds( idVec3( -13.5f, -13.5f, 0 ), idVec3( 13.5f, 13.5f, 10.4f ) );
const idBounds	playerHeadBounds( idVec3( -6.0f, -6.0f, -6.0f ), idVec3( 6.0f, 6.0f, 6.0f ) );
const float		playerProneLegOffset = -32.f;

/*
==================
idPhysics_Player::GetProneLegsPos
==================
*/
idVec3 idPhysics_Player::GetProneLegsPos( const idVec3& startOrg, const idAngles& angles ) const {
	idVec3 forward = angles.ToForward();
	forward.z = 0.f;
	forward.NormalizeFast();

	idVec3 temp( startOrg );
	temp.x += forward.x * playerProneLegOffset;
	temp.y += forward.y * playerProneLegOffset;
	temp.z += current.proneOffset;

	return temp;
}

/*
==================
idPhysics_Player::ProneCheck
==================
*/
int idPhysics_Player::ProneCheck( const idVec3& startOrg, const idAngles& angles, float* offset, idVec3* modelOffset ) const {
	// don't bother with the prone check if its reprediction & not a local client
	if ( !gameLocal.isNewFrame && self != gameLocal.GetLocalViewPlayer() ) {
		return 0;
	}

	if ( offset ) {
		*offset = 0;
	}

	float downTraceLength = 16.f;
	idVec3 down( 0.f, 0.f, -downTraceLength );

	idVec3 checkForward;
	checkForward = angles.ToForward();
	checkForward.z = 0.f;
	checkForward.Normalize();

	float fabsX = idMath::Fabs( checkForward.x );
	float fabsY = idMath::Fabs( checkForward.y );
	float scale = 1 / Max( fabsX, fabsY );

	checkForward *= scale;

	if ( groundPlane ) {
		trace_t gt;
		idVec3 downStart;

		static const float maxAngle = idMath::Cos( DEG2RAD( 34.f ) );

		if ( groundTrace.c.normal.z < maxAngle ) {
			return PR_BREAK;
		}


		downStart = startOrg;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS_CLIENTINFO( self ) gt, downStart, downStart + down, clipMask, self );

		if ( gt.c.normal.z < maxAngle ) {
			return PR_BREAK;
		}

		if ( gt.fraction == 1.f ) {
			return PR_FAILED | PR_NOGROUND;
		}

		idVec3 offset = gt.endpos - downStart;
		idVec3 orientation = gt.c.normal;

		downStart = startOrg + checkForward * 16;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS_CLIENTINFO( self ) gt, downStart, downStart + down, clipMask, self );

		if ( gt.fraction == 1.f || gt.c.normal.z < maxAngle ) {
			return PR_FAILED | PR_NOGROUND;
		}

		if ( modelOffset != NULL ) {
			*modelOffset = offset;
		}
	}

	idVec3 org = GetProneLegsPos( startOrg, angles );
	idVec3 point( org );

	point.z = startOrg.z - downTraceLength;

	trace_t proneTrace;

	idVec3 checkOrg = startOrg;
	checkOrg.z += pm_proneheight.GetFloat() * 0.5f;

	int z;
	for ( z = 32; z >= 0; z -= 16 ) {
		org.z = startOrg.z + z;

		if ( gameLocal.clip.Contents( CLIP_DEBUG_PARMS_CLIENTINFO( self ) org, proneLegsClipModel, mat3_identity, clipMask, self ) != 0 ) {
			continue;
		}

		trace_t tr;
		gameLocal.clip.TranslationWorld( CLIP_DEBUG_PARMS_CLIENTINFO( self ) tr, checkOrg, org, NULL, mat3_identity, clipMask );
		if ( tr.fraction != 1.f ) {
			continue;
		}

		break;
	}

	if ( z < 0 ) {
		if ( !groundPlane ) {
			return PR_FAILED | PR_BREAK;
		}
		return PR_FAILED;
	}

	gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) proneTrace, org, point, proneLegsClipModel, mat3_identity, clipMask, self );

	if ( offset ) {
		*offset = proneTrace.endpos.z - startOrg.z;
	}

	if ( groundPlane && proneTrace.fraction == 1.f ) {
		return PR_FAILED | PR_BREAK;
	}

	return 0;
}

/*
================
idPhysics_Player::VehiclePush
================
*/
#define VEHICLE_PUSH_EPSILON	0.5f

int idPhysics_Player::VehiclePush( bool stuck, float timeDelta, idVec3& move, idClipModel* pusher, int pushCount ) {

	if ( current.movementType == PM_NOCLIP ) {
		return VPUSH_OK;
	}

	if ( pushCount > 3 ) {
		move.Zero();
		return VPUSH_BLOCKED;
	}

	// remove components into the ground
	if ( groundPlane ) {
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

//		pusher->Disable();
		SlideMove( false, true, false, false, pushCount + 1 );
//		pusher->Enable();

//		SetWaterLevel();
		CheckGround();

		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		shotClipModel->Link( gameLocal.clip, self, 1, current.origin, shotClipModel->GetAxis() );
		if ( IsProne() ) {
			idVec3 legsOrg = GetProneLegsPos( current.origin, viewAngles );
			proneLegsClipModel->Link( gameLocal.clip, self, 2, legsOrg, mat3_identity );
		} else {
			proneLegsClipModel->Unlink( gameLocal.clip );
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
		int numContacts = gameLocal.clip.ContactsModel( CLIP_DEBUG_PARMS_CLIENTINFO( self ) contacts, 2, current.origin, NULL, 4.0f, clipModel, clipModel->GetAxis(), -1, pusher, pusher->GetOrigin(), pusher->GetAxis() );
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
			gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS_CLIENTINFO( self ) normalFinder, normalFinderStart, normalFinderEnd, clipModel,
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
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS_CLIENTINFO( self ) tr, start, end, clipModel, clipModel->GetAxis(), GetClipMask(), pusher,
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

/*
===============
idPhysics_Player::AdjustVertically
given a walkable plane normal, adjust z to be in the plane without changing the horizontal direction
===============
*/
idVec3 idPhysics_Player::AdjustVertically( const idVec3 &normal, const idVec3 &_in ) {
	idVec3 out;
	idVec3 in = _in;

	idVec3 velocityPlane;
	velocityPlane[0] = in[1];
	velocityPlane[1] = -in[0];
	velocityPlane[2] = 0.0f;

	// same length as horizontal input, and in the plane
	out = velocityPlane.Cross( normal );

	// make sure direction is ok
	if ( in * out < 0 ) {
		out = -out;
	}

	if ( pm_slidevelocity.GetInteger() == 1 ) {
		double inSpeed = in.Normalize();
		double hSpeed = out.Normalize();
		// incidence angle, between 0 (just touching, result speed at inSpeed) and M_PI/2 (perpendicular hit, result speed at hSpeed)
		float angle = idMath::ACos( in * out );
		double ratio = idMath::Pow64( angle * 2.0f / idMath::PI, pm_powerslide.GetFloat() );
		float targetSpeed = ratio * hSpeed + ( 1.0 - ratio ) * inSpeed;
		out *= targetSpeed;
	}

	// and maintain an overclip so rounding problems don't get us into solids
	if ( out.z >= 0 ) {
		out.z *= PM_OVERCLIP;
	} else {
		out.z /= PM_OVERCLIP;
	}

	return out;
}

/*
==================
idPhysics_Player::SlideMove

Returns true if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
#define	MAX_BUMPS		4

bool idPhysics_Player::SlideMove( bool gravity, bool stepUp, bool stepDown, bool push, int vehiclePush ) {
	int			i, j, k, pushFlags;
	int			bumpcount, numplanes;
	float		d, time_left, into, totalMass;
	idVec3		dir, planes[MAX_CLIP_PLANES];
	idVec3		start, end, stepEnd, primal_velocity, endVelocity, endClipVelocity, clipVelocity;
	trace_t		trace, stepTrace, downTrace;
	bool		nearGround, stepped, pushed, vehiclePushed;

	primal_velocity = current.velocity;

	if ( gravity ) {
		endVelocity = current.velocity + gravityVector * frametime;
		current.velocity = ( current.velocity + endVelocity ) * 0.5f;
		primal_velocity = endVelocity;
		if ( groundPlane ) {
			// slide along the ground plane
			if ( groundTrace.c.normal.z >= MIN_WALK_NORMAL ) {
				current.velocity = AdjustVertically( groundTrace.c.normal, current.velocity );
			} else {
				current.velocity.ProjectOntoPlane( groundTrace.c.normal, PM_OVERCLIP );
			}
		}
	} else {
		endVelocity = current.velocity;
	}

	time_left = frametime;

	// never turn against the ground plane
	if ( groundPlane ) {
		numplanes = 1;
		planes[0] = groundTrace.c.normal;
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	planes[numplanes] = current.velocity;
	planes[numplanes].Normalize();
	numplanes++;

	start = lastClippedOrigin;

	// HACK: this clipping of too long traces is dumb. fix the actual cause.
	if ( gameLocal.IsLocalViewPlayer( self ) || ( start - current.origin ).LengthSqr() > Square( 500.0f ) ) {
		start = current.origin;
	}

	if ( mergeThisFrame ) {
		// don't step down if not doing traces
		stepDown = false;
	}

	if ( self->GetAORPhysicsLOD() >= 1 ) {
		// don't step down players in the distance
		stepDown = false;
	}

	for ( bumpcount = 0; bumpcount < MAX_BUMPS; bumpcount++ ) {
		// calculate position we are trying to move to
		end = current.origin + time_left * current.velocity;

		// see if we can make it there
		if ( !mergeThisFrame ) {
			GetTraceCollection().Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) trace, start, end, clipModel, clipModel->GetAxis(), clipMask );

			if ( IsProne() && trace.fraction > 0.0f ) {
				float temp;
				idVec3 temp2 = vec3_origin;
				int retVal = ProneCheck( trace.endpos, viewAngles, &temp, &temp2 );
				if ( retVal & PR_NOGROUND ) {
					if ( !groundPlane ) {
						if ( !gameLocal.isClient ) {
							playerSelf->PlayProneFailedToolTip();
							LeaveProne( false );
						}
					} else {
						return true;
					}
				} else {
					proneModelOffset = temp2;

					if ( retVal & PR_BREAK ) {
						if ( !gameLocal.isClient ) {
							playerSelf->PlayProneFailedToolTip();
							LeaveProne( false );
						}
					} else if ( retVal & PR_FAILED ) {
						return true;
					} else {
						current.proneOffset = temp;
					}
				}
			}

			lastClippedOrigin = trace.endpos;
		} else {
			trace.endpos = end;
			trace.fraction = 1.0f;
		}

		time_left -= time_left * trace.fraction;
		current.origin = trace.endpos;

		// if moved the entire distance
		if ( trace.fraction >= 1.0f ) {
			break;
		}

		// FIXME: if for some reason things blew up
/*		if ( trace.c.type == CONTACT_HUGE_TRANSLATION ) {
			current.velocity.Zero();
			endVelocity.Zero();
			break;
		}*/

		stepped = pushed = vehiclePushed = false;

		// if we are allowed to step up
		if ( stepUp && ( trace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {

			nearGround = groundPlane || ladder.IsValid();

			if ( !nearGround ) {
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or on a ladder
			if ( nearGround ) {

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) {

					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) {
						time_left = 0;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						lastClippedOrigin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= pm_stepScale;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) {
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						lastClippedOrigin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= pm_stepScale;
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
			lastClippedOrigin = trace.endpos;
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
			idEntity* ent = gameLocal.entities[ trace.c.entityNum ];

			bool collideDamage = true;
			if ( playerSelf != NULL ) {
				sdTransport* transportEnt = ent->Cast< sdTransport >();
				if ( transportEnt != NULL ) {
					int exitTime = transportEnt->GetPositionManager().GetPlayerExitTime( playerSelf );
					if ( gameLocal.time - exitTime < 1000 ) {
						collideDamage = false;
					}
				}
			}

			if ( collideDamage ) {
				ent->Hit( trace, current.velocity, self );
				self->Collide( trace, current.velocity, -1 );
			}
		}

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// MrElusive: I think we have some relatively high poly LWO models with a lot of slanted tris
			// where it may hit the max clip planes
			current.velocity = vec3_origin;
			return true;
		}

		start = current.origin;

		//
		// if this is the same plane we hit before, nudge velocity out along it,
		// which fixes some epsilon issues with non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( ( trace.c.normal * planes[i] ) > 0.999f ) {
				if ( planes[i].z >= MIN_WALK_NORMAL ) {
					current.velocity = AdjustVertically( trace.c.normal, current.velocity );
				} else {
					// clip into the trace normal just in case this normal is almost but not exactly the same as the groundTrace normal
					current.velocity.ProjectOntoPlane( trace.c.normal, PM_OVERCLIP );
					// also add the normal to nudge the velocity out
					current.velocity += trace.c.normal;
				}
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

			if ( planes[i].z >= MIN_WALK_NORMAL ) {
				clipVelocity = AdjustVertically( planes[i], current.velocity );
				endClipVelocity = AdjustVertically( planes[i], endVelocity );
			} else {
				// slide along the plane
				clipVelocity = current.velocity;
				clipVelocity.ProjectOntoPlane( planes[i], PM_OVERCLIP );

				// slide along the plane
				endClipVelocity = endVelocity;
				endClipVelocity.ProjectOntoPlane( planes[i], PM_OVERCLIP );
			}

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( ( clipVelocity * planes[j] ) >= 0.1f ) {
					continue;		// move doesn't interact with the plane
				}

				if ( planes[j].z >= MIN_WALK_NORMAL ) {
					clipVelocity = AdjustVertically( planes[j], clipVelocity );
					endClipVelocity = AdjustVertically( planes[j], endClipVelocity );
				} else {
					// try clipping the move to the plane
					clipVelocity.ProjectOntoPlane( planes[j], PM_OVERCLIP );
					endClipVelocity.ProjectOntoPlane( planes[j], PM_OVERCLIP );
				}

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
	if ( stepDown && groundPlane ) {
		stepEnd = current.origin + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) {
			current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
			current.origin = downTrace.endpos;
			lastClippedOrigin = downTrace.endpos;
			current.movementFlags |= PMF_STEPPED_DOWN;
			current.velocity *= pm_stepScale;
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
==================
idPhysics_Player::Friction

Handles both ground friction and water friction
==================
*/
void idPhysics_Player::Friction( void ) {
	idVec3	vel;
	float	speed, newspeed, control;
	float	drop;

	vel = current.velocity;
	if ( walking ) {
		// ignore slope movement, remove all velocity in gravity direction
		vel += (vel * gravityNormal) * gravityNormal;
	}

	speed = vel.Length();
	if ( speed < 1.0f ) {
		// remove all movement orthogonal to gravity, allows for sinking underwater
		if ( fabs( current.velocity * gravityNormal ) < 1e-5f ) {
			current.velocity.Zero();
		} else if ( current.movementType == PM_SPECTATOR ) {
			// zero velocity out if spectator
			current.velocity.Zero();
		} else {
			current.velocity = (current.velocity * gravityNormal) * gravityNormal;
		}
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// spectator friction
	if ( current.movementType == PM_SPECTATOR ) {
		drop += speed * pm_flyFriction * frametime;
	} else if ( walking && waterLevel <= WATERLEVEL_FEET ) {
		// apply ground friction
		// no friction on slick surfaces
		if ( !(groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopSpeed ? pm_stopSpeed : speed;
				float friction;
				if ( pm_friction < 0 ) {
					friction = ::pm_friction.GetFloat();
				} else {
					friction = pm_friction;
				}
				drop += control * friction * frametime;
			}
		}
	} else if ( waterLevel ) {
		// apply water friction even if just wading
		drop += speed * pm_waterFriction * waterLevel * frametime;
	} else {
		// apply air friction
		drop += speed * pm_airFriction * frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	current.velocity *= ( newspeed / speed );

	// snap to avoid denormals
	current.velocity.FixDenormals( 1.0e-5f );
}

/*
===================
idPhysics_Player::CalcSpectateBounds
===================
*/
void idPhysics_Player::CalcSpectateBounds( idBounds& bounds ) {
	bounds[ 0 ].Set( -pm_spectatebbox.GetFloat() * 0.5f, -pm_spectatebbox.GetFloat() * 0.5f, -pm_spectatebbox.GetFloat() * 0.5f );
	bounds[ 1 ].Set( pm_spectatebbox.GetFloat() * 0.5f, pm_spectatebbox.GetFloat() * 0.5f, pm_spectatebbox.GetFloat() * 0.5f );
}

/*
===================
idPhysics_Player::CalcNormalBounds
===================
*/
void idPhysics_Player::CalcNormalBounds( idBounds& bounds ) {
	bounds[ 0 ].Set( -pm_bboxwidth.GetFloat() * 0.5f, -pm_bboxwidth.GetFloat() * 0.5f, 0 );
	bounds[ 1 ].Set( pm_bboxwidth.GetFloat() * 0.5f, pm_bboxwidth.GetFloat() * 0.5f, pm_normalheight.GetFloat() );
}

/*
===================
idPhysics_Player::WaterJumpMove

Flying out of the water
===================
*/
void idPhysics_Player::WaterJumpMove( void ) {

	CheckLean( false );

	// waterjump has no control, but falls
	SlideMove( true, true, false, false, 0 );

	// add gravity
	current.velocity += gravityNormal * frametime;
	// if falling down
	if ( current.velocity * gravityNormal > 0.0f ) {
		// cancel as soon as we are falling down again
		current.movementFlags &= ~PMF_ALL_TIMES;
		current.movementTime = 0;
	}
}

idCVar pm_waterFloatValue( "pm_waterFloatValue", "0.6", CVAR_GAME | CVAR_FLOAT, "fraction of water coverage at which the player will try to float" );

/*
===================
idPhysics_Player::WaterMove
===================
*/
void idPhysics_Player::WaterMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;
	float	vel;

	if ( CheckWaterJump() ) {
		WaterJumpMove();
		return;
	}

	CheckLean( false );

	Friction();

	scale = CmdScale( command, false );

	// user intentions
	wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	wishvel -= scale * gravityNormal * command.upmove;

	// force rise to the top
	wishvel -= ( wishvel * gravityNormal ) * gravityNormal;

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	if ( wishspeed > playerSpeed * pm_swimScale ) {
		wishspeed = playerSpeed * pm_swimScale;
	}

	Accelerate( wishdir, wishspeed, pm_waterAccelerate );

	float waterFloatValue = pm_waterFloatValue.GetFloat();
	if ( waterFraction > waterFloatValue ) {
		float frac = ( waterFraction - waterFloatValue ) / ( 1.f - waterFloatValue );
		frac = frac * frac;
		Accelerate( -gravityNormal, pm_waterSpeed.GetFloat() * frac, pm_accelerate );
	}

	// make sure we can go up slopes easily under water
	if ( groundPlane && ( current.velocity * groundTrace.c.normal ) < 0.0f ) {
		vel = current.velocity.Length();
		// slide along the ground plane
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, PM_OVERCLIP );

		current.velocity.Normalize();
		current.velocity *= vel;
	}

	SlideMove( false, true, false, false, 0 );
}

/*
===================
idPhysics_Player::FlyMove
===================
*/
void idPhysics_Player::FlyMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;

	// normal slowdown
	Friction();

	scale = CmdScale( command, false );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	Accelerate( wishdir, wishspeed, pm_flyAccelerate );

	SlideMove( false, false, false, false, 0 );
}

/*
===================
idPhysics_Player::AirMove
===================
*/
void idPhysics_Player::AirMove( bool allowLean ) {
	idVec3		wishvel;
	idVec3		wishdir;
	float		wishspeed;
	float		scale;

	CheckLean( allowLean );

	Friction();

	scale = CmdScale( command, true );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;
	viewForward.Normalize();
	viewRight.Normalize();

	wishvel = viewForward * command.forwardmove + viewRight * command.rightmove;
	wishvel -= (wishvel * gravityNormal) * gravityNormal;
	wishdir = wishvel;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	Accelerate( wishdir, wishspeed, pm_airAccelerate );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( groundPlane ) {
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, PM_OVERCLIP );
	}

	SlideMove( true, false, false, false, 0 );
}

/*
===================
idPhysics_Player::CheckLean
===================
*/
void idPhysics_Player::CheckLean( bool allow ) {
	if ( gameLocal.IsPaused() ) {
		return;
	}

	// never allow leaning if movement is inhibited
	if ( playerSelf->InhibitMovement() ) {
		allow = false;
	}

	float oldLeanOffset = leanOffset;

	if ( !IsFrozen() ) {
		if ( command.upmove > 0 || command.forwardmove != 0 || command.rightmove != 0 || IsProne() || ( masterEntity != NULL ) ) {
			allow = false;
		}

		if ( allow && command.buttons.btn.leanLeft && !command.buttons.btn.leanRight ) {
			leanFraction -= CONST_PM_LEANRATE * MS2SEC( gameLocal.msec );
			if ( leanFraction < -1.f ) {
				leanFraction = -1.f;
			}
		} else if ( allow && !command.buttons.btn.leanLeft && command.buttons.btn.leanRight ) {
			leanFraction += CONST_PM_LEANRATE * MS2SEC( gameLocal.msec );
			if ( leanFraction > 1.f ) {
				leanFraction = 1.f;
			}
		} else {
			if ( leanFraction < 0.f ) {
				leanFraction += CONST_PM_LEANRATE * MS2SEC( gameLocal.msec );
				if ( leanFraction > 0.f ) {
					leanFraction	= 0.f;
					leanOffset		= 0.f;
				}
			} else if ( leanFraction > 0.f ) {
				leanFraction -= CONST_PM_LEANRATE * MS2SEC( gameLocal.msec );
				if ( leanFraction < 0.f ) {
					leanFraction	= 0.f;
					leanOffset		= 0.f;
				}
			}
		}
	}

	if ( leanFraction != 0.f ) {
		leanOffset = leanFraction * CONST_PM_LEANMAX;

		idVec3 start = current.origin;
		if ( IsCrouching() ) {
			start[ 2 ] += pm_crouchviewheight.GetFloat();
		} else {
			start[ 2 ] += pm_normalviewheight.GetFloat();
		}

		idAngles angles = viewAngles;
		angles.roll += leanOffset * 0.5f;

		idVec3 right;
		angles.ToVectors( NULL, &right, NULL );
		idVec3 end = start + ( leanOffset * right );

		trace_t trace;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) trace, start, end, gameLocal.clip.GetLeanOffsetModel(), mat3_identity, MASK_PLAYERSOLID, self );
		leanOffset *= trace.fraction;
	}

	if ( oldLeanOffset != leanOffset ) {
		playerSelf->UpdateCombatModel();
	}
}

/*
===================
idPhysics_Player::CalcDesiredWalkMove
===================
*/
void idPhysics_Player::CalcDesiredWalkMove( int forwardmove, int rightmove, float walkSpeedFwd, float walkSpeedSide, float walkSpeedBack, idVec2& output ) {
	idVec2 adjustedMove( 0.0f, 0.0f );
	float forwardMoveAdjusted = 0.0f;
	float rightMoveAdjusted = 0.0f;

	// divide the desired movement into "max" and "min" axes
	// to smoothly (ish) calculate how we're going to be moving
	float maxMoveSpeedLimit;
	idVec2 maxMoveAxis;
	float maxMoveStrength = idMath::Fabs( ( float )forwardmove );
	if ( forwardmove >= 0 ) {
		maxMoveSpeedLimit = walkSpeedFwd;
		maxMoveAxis.Set( 1.0f, 0.0f );
	} else {
		maxMoveSpeedLimit = walkSpeedBack;
		maxMoveAxis.Set( -1.0f, 0.0f );
	}

	float minMoveSpeedLimit = walkSpeedSide;
	idVec2 minMoveAxis;
	float minMoveStrength = idMath::Fabs( ( float )rightmove );
	if ( rightmove >= 0 ) {
		minMoveAxis.Set( 0.0f, 1.0f );
	} else {
		minMoveAxis.Set( 0.0f, -1.0f );
	}

	if ( minMoveSpeedLimit > maxMoveSpeedLimit ) {
		Swap( minMoveSpeedLimit, maxMoveSpeedLimit );
		Swap( minMoveAxis, maxMoveAxis );
		Swap( minMoveStrength, maxMoveStrength );
	}

	float maxMoveSpeed = maxMoveSpeedLimit * maxMoveStrength / 127.0f;
	float minMoveSpeed = minMoveSpeedLimit * minMoveStrength / 127.0f;

	if ( maxMoveSpeedLimit < idMath::FLT_EPSILON || minMoveSpeedLimit  < idMath::FLT_EPSILON ) {
		// do nothing! - no move
	} else {
		float minMoveAdjusted = 0.0f;
		float maxMoveAdjusted = 0.0f;

		if ( maxMoveSpeed < idMath::FLT_EPSILON && minMoveSpeed < idMath::FLT_EPSILON ) {
			// do nothing! - no move
		} else if ( maxMoveStrength >= minMoveStrength ) {
			// calculate the length of the full-strength move in this direction
			float fullLengthScale = maxMoveSpeedLimit / maxMoveSpeed;
			float fullLength = fullLengthScale * idMath::Sqrt( maxMoveSpeed * maxMoveSpeed + minMoveSpeed * minMoveSpeed );

			// scale-back to fit the maximum speed we're allowed
			maxMoveAdjusted = maxMoveSpeed * maxMoveSpeedLimit / fullLength;
			minMoveAdjusted = minMoveSpeed * maxMoveSpeedLimit / fullLength;
		} else {
			// calculate the length of the full-strength move in this direction
			float fullLengthScale = minMoveSpeedLimit / minMoveSpeed;
			float fullLength = fullLengthScale * idMath::Sqrt( maxMoveSpeed * maxMoveSpeed + minMoveSpeed * minMoveSpeed );

			// linearly blend the max total strength based on how close it is to the junction point between
			// this case and the previous case (maxMoveStrength >= minMoveStrength)
			float maxLength = Lerp( minMoveSpeedLimit, maxMoveSpeedLimit, ( maxMoveSpeed * fullLengthScale ) / maxMoveSpeedLimit );

			// scale-back to fit the maximum speed we're allowed
			maxMoveAdjusted = maxMoveSpeed * maxLength / fullLength;
			minMoveAdjusted = minMoveSpeed * maxLength / fullLength;
		}

		adjustedMove = maxMoveAdjusted * maxMoveAxis + minMoveAdjusted * minMoveAxis;

		// scale everything so that the max possible speed equals the playerspeed
		float absoluteMaxSpeed = walkSpeedFwd;
		if ( walkSpeedBack > absoluteMaxSpeed ) {
			absoluteMaxSpeed = walkSpeedBack;
		}
		if ( walkSpeedSide > absoluteMaxSpeed ) {
			absoluteMaxSpeed = walkSpeedSide;
		}
		if ( absoluteMaxSpeed > idMath::FLT_EPSILON ) {
			adjustedMove /= absoluteMaxSpeed;
		}
	}

	output = adjustedMove;
}

/*
===================
idPhysics_Player::CalcDesiredWalkMove
===================
*/
idVec2 idPhysics_Player::CalcDesiredWalkMove( const usercmd_t& cmd ) const {
	idVec2 adjustedMove;
	CalcDesiredWalkMove( cmd.forwardmove, cmd.rightmove, walkSpeedFwd, walkSpeedSide, walkSpeedBack, adjustedMove );
	adjustedMove *= playerSpeed;
	return adjustedMove;
}

/*
===================
idPhysics_Player::SetupUsercmdForDirection
===================
*/
void idPhysics_Player::SetupUsercmdForDirection( const idVec2 &dir, float forwardSpeed, float backwardSpeed, float sideSpeed, usercmd_t &cmd ) {
	idVec2 normal = dir;
	float speed = normal.Normalize();

	idVec2 move;
	move[0] = Sign( normal[0] ) * 128.0f;
	move[1] = Sign( normal[1] ) * 128.0f;
	cmd.forwardmove = idMath::Ftob( move[0] + 128.0f ) - 128;
	cmd.rightmove = idMath::Ftob( move[1] + 128.0f ) - 128;

	int axis;
	idVec2 adjustedDir;

	CalcDesiredWalkMove( cmd.forwardmove, cmd.rightmove, forwardSpeed, sideSpeed, backwardSpeed, adjustedDir );
	adjustedDir.Normalize();
	axis = fabs( adjustedDir[0] ) < fabs( normal[0] );

	for ( int i = 0; i < 8; i++ ) {
		CalcDesiredWalkMove( cmd.forwardmove, cmd.rightmove, forwardSpeed, sideSpeed, backwardSpeed, adjustedDir );
		adjustedDir.Normalize();
		move[axis] += ( normal[axis] - adjustedDir[axis] ) * 120.0f;
		cmd.forwardmove = idMath::Ftob( move[0] + 128.0f ) - 128;
		cmd.rightmove = idMath::Ftob( move[1] + 128.0f ) - 128;
	}

	move *= speed / 128.0f;
	cmd.forwardmove = idMath::Ftob( move[0] + 128.0f ) - 128;
	cmd.rightmove = idMath::Ftob( move[1] + 128.0f ) - 128;

#if 0
	CalcDesiredWalkMove( cmd.forwardmove, cmd.rightmove, forwardSpeed, sideSpeed, backwardSpeed, adjustedDir );
	adjustedDir.Normalize();
	float angle = idMath::ACos( normal * adjustedDir ) * idMath::M_RAD2DEG;

	common->Printf( "angle difference = %1.1f\n", angle );
#endif
}

/*
===================
idPhysics_Player::WalkMove
===================
*/
void idPhysics_Player::WalkMove( bool allowLean ) {
	idVec3		wishvel;
	idVec3		wishdir;
	float		wishspeed;
	float		accelerate;
	idVec3		vel;

	if ( CheckJump() ) {
		// jumped away
		AirMove( true );
		return;
	}

	CheckLean( allowLean );

	Friction();

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;

	assert( groundTrace.c.normal.z >= MIN_WALK_NORMAL );
	viewForward = AdjustVertically( groundTrace.c.normal, viewForward );
	viewRight = AdjustVertically( groundTrace.c.normal, viewRight );

	viewForward.Normalize();
	viewRight.Normalize();

	// find how we want to move
	idVec2 adjustedMove = CalcDesiredWalkMove( command );
	wishdir = viewForward * adjustedMove.x + viewRight * adjustedMove.y;
	wishspeed = wishdir.Normalize();

	// clamp the speed lower if wading or walking on the bottom
	if ( waterLevel ) {
		float	waterScale;

		waterScale = waterLevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - pm_swimScale ) * waterScale;
		if ( wishspeed > playerSpeed * waterScale ) {
			wishspeed = playerSpeed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose full control, which allows them to be moved a bit
	bool fLowControl = ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK;
	accelerate = fLowControl ? pm_airAccelerate : pm_accelerate;
	Accelerate( wishdir, wishspeed, accelerate );
	if ( fLowControl ) {
		current.velocity += gravityVector * frametime;
	}

	current.velocity = AdjustVertically( groundTrace.c.normal, current.velocity );

	// don't do anything if standing still
	vel = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	if ( !vel.LengthSqr() ) {
		return;
	}

	gameLocal.push.InitSavingPushedEntityPositions();

	SlideMove( false, true, true, true, 0 );
}

/*
==============
idPhysics_Player::DeadMove
==============
*/
void idPhysics_Player::DeadMove( void ) {
	if ( walking ) {
		WalkMove( false );
	} else {
		AirMove( false );
	}
}

/*
===============
idPhysics_Player::NoclipMove
===============
*/
void idPhysics_Player::NoclipMove( void ) {
	float		speed, drop, friction, newspeed, stopspeed;
	float		scale, wishspeed;
	idVec3		wishdir;

	// friction
	speed = current.velocity.Length();
	if ( speed < 20.0f ) {
		current.velocity = vec3_origin;
	}
	else {
		stopspeed = playerSpeed * 0.3f;
		if ( speed < stopspeed ) {
			speed = stopspeed;
		}
		friction = pm_noclipFriction;
		drop = speed * friction * frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0) {
			newspeed = 0;
		}

		current.velocity *= newspeed / speed;
	}

	// accelerate
	scale = CmdScale( command, false );

	wishdir = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	wishdir -= scale * gravityNormal * command.upmove;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	current.origin += frametime * current.velocity;
}

/*
===============
idPhysics_Player::SpectatorMove
===============
*/
void idPhysics_Player::SpectatorMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;

	trace_t	trace;
	idVec3	end;

	// fly movement

	Friction();

	scale = CmdScale( command, false );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove + ( idVec3( 0.f, 0.f, 1.f ) * command.upmove ) );
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	Accelerate( wishdir, wishspeed, pm_flyAccelerate );

	SlideMove( false, false, false, false, 0 );
}

/*
============
idPhysics_Player::LadderMove
============
*/
void idPhysics_Player::LadderMove( void ) {
	CheckLean( false );

	sdLadderEntity* ladderEnt = ladder;
	assert( ladderEnt != NULL );

	const idVec3& ladderNormal = ladderEnt->GetLadderNormal();

	idVec3 offset = ( ladderEnt->GetPhysics()->GetOrigin() + ( ladderEnt->GetPhysics()->GetBounds().GetCenter() * ladderEnt->GetPhysics()->GetAxis() ) ) - current.origin;

	offset -= ( gravityNormal * offset ) * gravityNormal;
	offset -= ( ladderNormal * offset ) * ladderNormal;
	offset *= 0.1f;
	offset -= ladderNormal;
	offset.Normalize();

	// stick to the ladder
	idVec3 wishvel = 100.0f * offset;
	current.velocity = ( gravityNormal * current.velocity ) * gravityNormal + wishvel;

	bool stepUp = false;

	if ( command.upmove >= 0 ) {
		float upDown = 0.f;

		if ( command.upmove > 0 ) {
			upDown = 1.f;
		} else {
			if ( command.forwardmove > 0 ) {
				upDown = 1.f;
			} else if ( command.forwardmove < 0 ) {
				upDown = -1.f;
			}
			upDown *= viewForward.z < 0.f ? -1.f : 1.f;
		}
		stepUp = upDown > 0.f;

		wishvel = -gravityNormal * upDown * playerSpeed;

		// accelerate
		float wishspeed = wishvel.Normalize();
		Accelerate( wishvel, wishspeed, pm_accelerate );

		// cap the vertical velocity
		float upscale = current.velocity * -gravityNormal;
		if ( upscale < -pm_ladderSpeed ) {
			current.velocity += gravityNormal * ( upscale + pm_ladderSpeed );
		} else if ( upscale > pm_ladderSpeed ) {
			current.velocity += gravityNormal * ( upscale - pm_ladderSpeed );
		}

		if ( ( wishvel * gravityNormal ) == 0.0f ) {
			if ( current.velocity * gravityNormal < 0.0f ) {
				current.velocity += gravityVector * frametime;
				if ( current.velocity * gravityNormal > 0.0f ) {
					current.velocity -= ( gravityNormal * current.velocity ) * gravityNormal;
				}
			} else {
				current.velocity -= gravityVector * frametime;
				if ( current.velocity * gravityNormal < 0.0f ) {
					current.velocity -= ( gravityNormal * current.velocity ) * gravityNormal;
				}
			}
		}
	} else {
		if ( !groundPlane ) {
			current.velocity += gravityVector * CONST_PM_LADDERSLIDESCALE * frametime;
		}
	}

	SlideMove( false, stepUp, false, false, 0 );
}

/*
=============
idPhysics_Player::CorrectAllSolid
=============
*/
void idPhysics_Player::CorrectAllSolid( trace_t &trace, int contents ) {

	// FIXME: jitter around to find a free spot ?

	if ( trace.fraction >= 1.0f ) {
		memset( &trace, 0, sizeof( trace ) );
		trace.endpos = current.origin;
		trace.endAxis = clipModelAxis;
		trace.fraction = 0.0f;
		trace.c.dist = current.origin.z;
		trace.c.normal.Set( 0, 0, 1 );
		trace.c.point = current.origin;
		trace.c.entityNum = ENTITYNUM_WORLD;
		trace.c.id = 0;
		trace.c.type = CONTACT_TRMVERTEX;
		trace.c.material = NULL;
		trace.c.surfaceType = NULL;
		trace.c.contents = contents;
	}
}

/*
================
idPhysics_Player::EvaluateContacts
================
*/
bool idPhysics_Player::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	// get all the ground contacts
	ClearContacts();
	groundCheckModel->Translate( clipModel->GetOrigin() - groundCheckModel->GetOrigin(), gameLocal.clip );
	AddGroundContacts( CLIP_DEBUG_PARMS_PASSTHRU groundCheckModel );
	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}

/*
=============
idPhysics_Player::CheckGround
=============
*/
void idPhysics_Player::CheckGround( void ) {

	int i;//, contents;
	idVec3 point;
	bool hadGroundContacts = HasGroundContacts();

	// set the clip model origin before getting the contacts
	clipModel->SetPosition( current.origin, clipModel->GetAxis(), gameLocal.clip );

	if ( !skipContactsThisFrame ) {
		EvaluateContacts( CLIP_DEBUG_PARMS_CLIENTINFO_ONLY( self ) );
	} else {
		// make sure the ground entity is still valid
		if ( gameLocal.entities[ groundTrace.c.entityNum ] == NULL ) {
			// somehow the ground entity no longer exists! invalidate
			contacts.SetNum( 0, false );
		}
	}

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
		groundPlane = false;
		walking = false;
		groundEntityPtr = NULL;
		return;
	}

	groundMaterial = groundTrace.c.material;
	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	// check if getting thrown off the ground
	if ( ( current.velocity * -gravityNormal ) > 0.0f && ( current.velocity * groundTrace.c.normal ) > 1.0f ) {
		groundPlane = false;
		walking = false;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( ( groundTrace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {
		groundPlane = true;
		walking = false;
		return;
	}

	groundPlane = true;
	walking = true;

	// hitting solid ground will end a waterjump
	if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		current.movementFlags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		current.movementTime = 0;
	}

	// if the player didn't have ground contacts the previous frame
	if ( !hadGroundContacts ) {

		// don't do landing time if we were just going down a slope
		if ( (current.velocity * -gravityNormal) < -200.0f ) {
			// don't allow another jump for a little while
			current.movementFlags |= PMF_TIME_LAND;
			current.movementTime = gameLocal.time + 250;
		}
	}

	// let the entity know about the collision
	bool collideDamage = true;
	sdTransport* transportEnt = groundEntityPtr->Cast< sdTransport >();
	if ( playerSelf != NULL && transportEnt != NULL ) {
		int exitTime = transportEnt->GetPositionManager().GetPlayerExitTime( playerSelf );
		if ( gameLocal.time - exitTime < 1000 ) {
			collideDamage = false;
		}
	}

	if ( collideDamage ) {
		groundEntityPtr->Hit( groundTrace, current.velocity, self );
		self->Collide( groundTrace, current.velocity, -1 );
	}
}

/*
==============
idPhysics_Player::CanCrouch
==============
*/
bool idPhysics_Player::CanCrouch( void ) {
	if ( waterLevel > WATERLEVEL_FEET ) {
		return false;
	}

	if ( ladder.IsValid() ) {
		return false;
	}

	return true;
}

/*
==============
idPhysics_Player::CanProne
==============
*/
bool idPhysics_Player::CanProne( void ) {
	if ( waterHeightAboveGround > 0.f ) {
		return false;
	}

	if ( playerSelf->InhibitProne() ) {
		return false;
	}

	if ( ProneCheck( current.origin, viewAngles ) != 0 ) {
		return false;
	}

	return true;
}

/*
==============
idPhysics_Player::EnterProne
==============
*/
void idPhysics_Player::EnterProne( void ) {
	if ( ( current.movementFlags & PMF_PRONE ) != 0 ) {
		return;
	}

	proneTimes_t type = PT_STAND_TO_PRONE;
	if( current.movementFlags & PMF_DUCKED ) {
		type = PT_CROUCH_TO_PRONE;
	}

	current.movementFlags |= PMF_PRONE;
	current.movementFlags &= ~PMF_DUCKED;

	proneChangeEndTime = gameLocal.time + proneTimes[ type ];

	// need to use the combat model to get good hit detection when proned
	playerSelf->UpdateCombatModel();
}

/*
==============
idPhysics_Player::LeaveProne
==============
*/
bool idPhysics_Player::LeaveProne( bool crouch ) {
	if ( ( current.movementFlags & PMF_PRONE ) == 0 ) {
		return false;
	}

	idVec3 end = current.origin - ( pm_normalheight.GetFloat() - pm_proneheight.GetFloat() ) * gravityNormal;

	trace_t trace;
	gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
	if ( trace.fraction < 1.f ) {
		crouch = true;

		end = current.origin - ( pm_crouchheight.GetFloat() - pm_proneheight.GetFloat() ) * gravityNormal;

		gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( trace.fraction < 1.f ) {
			return false;
		}
	}

	proneTimes_t type = PT_PRONE_TO_STAND;
	if ( current.movementFlags & PMF_DUCKED || crouch ) {
		type = PT_PRONE_TO_CROUCH;
		current.movementFlags |= PMF_DUCKED;
	}

	current.movementFlags &= ~PMF_PRONE;

	proneChangeEndTime = gameLocal.time + proneTimes[ type ];

	UpdateBounds();

	playerSelf->UpdateCombatModel();

	return true;
}

/*
==============
idPhysics_Player::ResetProne

Forces the player instantly out of prone
==============
*/
void idPhysics_Player::ResetProne( void ) {
	current.movementFlags &= ~PMF_PRONE;
	proneChangeEndTime = 0;

	playerSelf->UpdateCombatModel();
}

/*
==============
idPhysics_Player::FinishProneChange
==============
*/
void idPhysics_Player::FinishProneChange( void ) {
	proneChangeEndTime = 0;

	playerSelf->UpdateCombatModel();
}

/*
==============
idPhysics_Player::TryProne
==============
*/
bool idPhysics_Player::TryProne( void ) {
	if ( current.movementType == PM_DEAD ) {
		return true;
	}

	if ( IsFrozen() ) {
		return false;
	}

	if ( gameLocal.time > proneChangeEndTime ) {
		if ( IsProne() ) {
			LeaveProne( false );
		} else {
			if ( CanProne() ) {
				EnterProne();
			} else {
				return false;
			}
		}
	}

	return true;
}

/*
==============
idPhysics_Player::CheckStance

Sets clip model size
==============
*/
void idPhysics_Player::CheckStance( void ) {
	if ( proneChangeEndTime != 0 ) {
		if ( gameLocal.time > proneChangeEndTime ) {
			FinishProneChange();
		}
	}

	if ( current.movementType != PM_DEAD ) {
		if ( !IsFrozen() ) {
			if ( command.upmove < 0 && CanCrouch() ) { // stand up when up against a ladder
				if ( ( current.movementFlags & PMF_CROUCH_HELD ) == 0 ) {
					if ( IsProne() ) {
						LeaveProne( true );
					} else {
						current.movementFlags |= PMF_DUCKED;
					}
					current.movementFlags |= PMF_CROUCH_HELD;
				}
			} else if ( IsCrouching() ) {
				// try to stand up
				idVec3 end = current.origin - ( pm_normalheight.GetFloat() - pm_crouchheight.GetFloat() ) * gravityNormal;

				trace_t	trace;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
				if ( trace.fraction >= 1.0f ) {
					current.movementFlags &= ~PMF_DUCKED;
				}
			}

			if ( IsProne() ) {
				if ( command.upmove > 0 ) {
					if ( LeaveProne( false ) ) {
						current.movementFlags |= PMF_JUMP_HELD;
					}
				}
			}
		}

		if ( IsProne() ) {
			if ( waterLevel >= WATERLEVEL_FEET ) {
				LeaveProne( false );
			}
		}

		if ( current.movementFlags & PMF_DUCKED ) {
			playerSpeed = crouchSpeed;
		} else if ( current.movementFlags & PMF_PRONE ) {
			playerSpeed = proneSpeed;
		}

		if ( gameLocal.time < proneChangeEndTime ) {
			playerSpeed = 0;
		}
	}

	UpdateBounds();
}

/*
================
idPhysics_Player::UpdateBounds
================
*/
void idPhysics_Player::UpdateBounds( void ) {
	idClipModel* newClipModel = NULL;
	idClipModel* newShotClipModel = NULL;

	idEntity* proxy = playerSelf->GetProxyEntity();

	if ( proxy != NULL && proxy->IsType( sdTransport::Type ) ) {
		sdTransport* transport = static_cast< sdTransport* >( proxy );
		sdVehiclePosition& position = transport->GetPositionManager().PositionForPlayer( playerSelf );
		if ( position.GetPlayerHeight() > 1.0f ) {
			// uses a custom height player model
			SetupVehicleClipModels( position.GetPlayerHeight() );

			newClipModel = clipModel_vehicle;
			newShotClipModel = clipModel_vehicleShot;
		}
	}


	if ( newClipModel == NULL ) {
		playerStance_t stance = PS_NORMAL;
		if ( proxy != NULL ) {
			stance = proxy->GetUsableInterface()->GetPlayerStance( playerSelf );
		} else {
			if ( current.movementType == PM_DEAD ) {
				stance = PS_DEAD;
			} else if ( current.movementFlags & PMF_DUCKED ) {
				stance = PS_CROUCH;
			} else if ( current.movementFlags & PMF_PRONE ) {
				stance = PS_PRONE;
			}
		}

		switch ( stance ) {
			default:
			case PS_NORMAL:
				newClipModel = clipModel_normal;
				newShotClipModel = clipModel_normalShot;
				break;
			case PS_DEAD:
				newClipModel = clipModel_dead;
				newShotClipModel = clipModel_deadShot;
				break;
			case PS_CROUCH:
				newClipModel = clipModel_crouch;
				newShotClipModel = clipModel_crouchShot;
				break;
			case PS_PRONE:
				newClipModel = clipModel_prone;
				newShotClipModel = clipModel_proneShot;
				break;
		}
	}

	if ( clipModel != newClipModel ) {
		bool relink = false;
		if ( clipModel->IsLinked() ) {
			clipModel->Unlink( gameLocal.clip );
			relink = true;
		}
		clipModel = newClipModel;
		if ( relink ) {
			clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		}
	}
	if ( shotClipModel != newShotClipModel ) {
		bool relink = false;
		if ( shotClipModel->IsLinked() ) {
			shotClipModel->Unlink( gameLocal.clip );
			relink = true;
		}
		shotClipModel = newShotClipModel;
		if ( relink ) {
			shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
		}
	}
}

/*
================
idPhysics_Player::FindLadder
================
*/
sdLadderEntity* idPhysics_Player::FindLadder( const idVec3& origin, sdLadderEntity** ladders, int numLadders, const idVec3& direction ) {
	for ( int i = 0; i < numLadders; i++ ) {
		if ( ( ladders[ i ]->GetLadderNormal() * direction ) > 0.f ) {
			continue;
		}

		int contents = gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_CLIENTINFO( self ) origin, clipModel, ladders[ i ]->GetPhysics()->GetAxis(), -1, 
			ladders[ i ]->GetLadderModel(), ladders[ i ]->GetPhysics()->GetOrigin(), ladders[ i ]->GetPhysics()->GetAxis() );

		if ( contents != 0 ) {
			return ladders[ i ];
		}
	}

	return NULL;
}

/*
================
idPhysics_Player::CheckLadder
================
*/
void idPhysics_Player::CheckLadder( void ) {
	if ( IsProne() ) {
		return;
	}

	if ( current.movementTime != 0 ) {
		return;
	}

	// if on the ground moving backwards
	if ( walking && command.forwardmove <= 0 ) {
		return;
	}

	float gravityFactor = gravityNormal * viewForward;
	if ( walking && gravityFactor > 0.f ) {
		return;
	}

	// forward vector orthogonal to gravity
	idVec3 forward = viewForward - ( gravityFactor * gravityNormal );
	forward.Normalize();

	idVec3 startPos = current.origin - ( gravityNormal * 2.f );

	idBounds ladderCheckBounds = clipModel->GetBounds().Translate( startPos );
	ladderCheckBounds.AddBounds( ladderCheckBounds.Translate( - ( gravityNormal * ( maxStepHeight * 0.75f ) ) ) );

	const int MAX_LADDERS = 4;
	sdLadderEntity* ladders[ MAX_LADDERS ];

	int numLadders = gameLocal.clip.FindLadder( CLIP_DEBUG_PARMS_CLIENTINFO( self ) ladderCheckBounds, ladders, MAX_LADDERS );
	if ( numLadders == 0 ) {
		return;
	}

	ladder = FindLadder( startPos, ladders, numLadders, forward );
	if ( ladder.IsValid() ) {
		ladder = FindLadder( startPos - ( gravityNormal * ( maxStepHeight * 0.75f ) ), ladders, numLadders, forward );
	}
}

/*
=============
idPhysics_Player::CheckJump
=============
*/
bool idPhysics_Player::CheckJump( void ) {
	idVec3 addVelocity;

	if ( gameLocal.IsPaused() ) {
		return false;
	}

	if ( command.upmove < 10 ) {
		return false;
	}

	// must wait for jump to be released
	if ( current.movementFlags & PMF_JUMP_HELD ) {
		return false;
	}

	// don't jump if we can't stand up
	if ( current.movementFlags & PMF_DUCKED || IsProne() || gameLocal.time < proneChangeEndTime ) {
		return false;
	}

	// can't jump if jumped recently
	if ( gameLocal.time < jumpAllowedTime ) {
		return false;
	}

	if ( gameLocal.time < proneChangeEndTime ) {
		return false;
	}

	if ( gameLocal.isClient && !gameLocal.IsLocalPlayer( self ) ) {
		return false;
	}

	// only called from WalkMove, which implies walking == true, which means there is a ground plane and normal is good
	assert( groundPlane && groundTrace.c.normal.z >= MIN_WALK_NORMAL );

	// start by setting up a normal ground slide velocity
	// this will make sure that when we add the jump velocity we actually get off of the ground plane
	if ( current.velocity * groundTrace.c.normal < 0.0f ) {
		current.velocity = AdjustVertically( groundTrace.c.normal, current.velocity );
	}

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

	addVelocity = 2.0f * maxJumpHeight * -gravityVector;
	addVelocity *= idMath::Sqrt( addVelocity.Normalize() );
	current.velocity += addVelocity;

	jumpAllowedTime = gameLocal.time + 850;

	return true;
}

/*
=============
idPhysics_Player::CheckWaterJump
=============
*/
bool idPhysics_Player::CheckWaterJump( void ) {
	idVec3	spot;
	int		cont;
	idVec3	flatforward;

	if ( current.movementTime != 0 ) {
		return false;
	}

	if ( command.forwardmove <= 0 ) {
		return false;
	}

	// check for water jump
	if ( waterLevel != WATERLEVEL_WAIST ) {
		return false;
	}

	flatforward = viewForward - (viewForward * gravityNormal) * gravityNormal;
	flatforward.Normalize();

	spot = current.origin + 30.0f * flatforward;
	spot -= 4.0f * gravityNormal;
	cont = gameLocal.clip.Contents( CLIP_DEBUG_PARMS_CLIENTINFO( self ) spot, NULL, mat3_identity, CONTENTS_SOLID, self );
	if ( !(cont & CONTENTS_SOLID) ) {
		return false;
	}

	float height = clipModel->GetBounds().Size().z;

	spot -= ( height * 0.75f ) * gravityNormal;
	cont = gameLocal.clip.Contents( CLIP_DEBUG_PARMS_CLIENTINFO( self ) spot, NULL, mat3_identity, -1, self );
	if ( cont ) {
		return false;
	}

	// jump out of water
	current.velocity = 200.0f * viewForward - 400.0f * gravityNormal;
	current.movementFlags |= PMF_TIME_WATERJUMP;
	current.movementTime = gameLocal.time + 2000;

	return true;
}

/*
=============
idPhysics_Player::SetWaterLevel
=============
*/
void idPhysics_Player::SetWaterLevel( void ) {
	waterLevel				= WATERLEVEL_NONE;
	waterFraction			= 0.f;
	waterHeightAboveGround	= 0.f;

	idBounds bounds;
	CalcNormalBounds( bounds );
	idBounds absBounds = bounds.Translate( current.origin );

	const idClipModel* waterModel;
	idCollisionModel* model;
	int count = gameLocal.clip.FindWater( CLIP_DEBUG_PARMS_CLIENTINFO( self ) absBounds, &waterModel, 1 );
	if ( count && waterModel->GetNumCollisionModels() ) {
		model = waterModel->GetCollisionModel( 0 );
		int numPlanes = model->GetNumBrushPlanes();

		const idBounds& modelBounds = model->GetBounds();

		for ( int i = 0; i < numPlanes; i++ ) {
			idPlane plane = model->GetBrushPlane( i );
			plane.TranslateSelf( waterModel->GetOrigin() );
			plane.Normal() *= waterModel->GetAxis();

			if ( plane.Distance( current.origin ) > 0 ) {
				// outside of water clipmodel
				return;
			}
		}

		waterHeightAboveGround = waterModel->GetOrigin().z - current.origin.z + modelBounds.GetMaxs().z;
		waterFraction = waterHeightAboveGround / bounds.Size().z;
		if ( waterFraction > 1.f ) {
			waterFraction = 1.f;
		}

		if ( waterFraction >= CONST_PM_WATERFRAC_HEAD ) {
			waterLevel = WATERLEVEL_HEAD;
		} else if ( waterFraction > CONST_PM_WATERFRAC_WAIST ) {
			waterLevel = WATERLEVEL_WAIST;
		} else if ( waterFraction > 0.f ) {
			waterLevel = WATERLEVEL_FEET;
		}
	}

	if ( waterLevel != WATERLEVEL_NONE && ( current.movementType == PM_NORMAL || current.movementTime == PM_DEAD ) ) {
		playerSelf->CheckWater( waterModel->GetOrigin(), waterModel->GetAxis(), model );
	} else {
		playerSelf->CheckWaterEffectsOnly();
	}
}

/*
================
idPhysics_Player::DropTimers
================
*/
void idPhysics_Player::DropTimers( void ) {
	// drop misc timing counter
	if ( current.movementTime != 0 ) {
		if ( gameLocal.time >= current.movementTime ) {
			current.movementFlags &= ~PMF_ALL_TIMES;
			current.movementTime = 0;
		}
	}
}

/*
================
idPhysics_Player::UpdateCollisionMerge
================
*/
idCVar net_staggerPlayerGroundChecks( "net_staggerPlayerGroundChecks", "1", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT, "skip every other ground check during forward prediction" );
idCVar net_maxPlayerCollisionMerge( "net_maxPlayerCollisionMerge", "3", CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT, "maximum number of player collision steps to merge together during client reprediction" );

void idPhysics_Player::UpdateCollisionMerge( void ) {
	mergeThisFrame = false;
	skipContactsThisFrame = false;

	if ( !gameLocal.isClient ) {
		return;
	}

	if ( gameLocal.IsLocalViewPlayer( self ) ) {
		return;
	}

	if ( ladder != NULL ) {
		return;
	}

	if ( self->IsVisibleOcclusionTest() && self->GetAORPhysicsLOD() < 2 ) {
		return;
	}

	// default to trying to merge
	mergeThisFrame = true;
	skipContactsThisFrame = false;
	int maxMerge = net_maxPlayerCollisionMerge.GetInteger();

	// stagger ground checks during forward prediction
	if ( gameLocal.isNewFrame && net_staggerPlayerGroundChecks.GetBool() ) {
		// note that this only works with odd timesteps!
		if ( gameLocal.msec % 2 ) {
			int staggerTime = gameLocal.time + self->entityNumber;
			skipContactsThisFrame = ( staggerTime % 2 ) == 1;
		}
	}

	if ( gameLocal.isNewFrame ) {
		if ( self->GetAORPhysicsLOD() > 1 ) {
			// merge collision frames when they get a fair distance away
			mergeThisFrame = skipContactsThisFrame;
		} else {
			// don't merge during forward prediction for close-by players
			mergeThisFrame = false;
		}
	}

	// force end the merging, if we've crossed the max merging threshold
	if ( numMergedFrames >= maxMerge ) {
		mergeThisFrame = false;
		skipContactsThisFrame = false;
	} else {
		numMergedFrames++;
	}

	// don't update the contacts if merging in this frame
	if ( mergeThisFrame ) {
		skipContactsThisFrame = true;
	} else {
		numMergedFrames = 0;
	}
}

/*
================
idPhysics_Player::MovePlayer
================
*/
void idPhysics_Player::MovePlayer( int msec ) {

	UpdateCollisionMerge();

	// clear flags if we're not skipping the ground checking this frame
	if ( !skipContactsThisFrame ) {
		walking = false;
		groundPlane = false;
	}

	ladder = NULL;

	// determine the time
	framemsec = msec;
	frametime = framemsec * 0.001f;

	// default speed
	playerSpeed = walkSpeedFwd;
	if ( walkSpeedBack > playerSpeed ) {
		playerSpeed = walkSpeedBack;
	}
	if ( walkSpeedSide > playerSpeed ) {
		playerSpeed = walkSpeedSide;
	}

	// remove jumped and stepped up flag
	current.movementFlags &= ~(PMF_JUMPED|PMF_STEPPED_UP|PMF_STEPPED_DOWN);
	current.stepUp = 0.0f;

	if ( command.upmove < 10 ) {
		// not holding jump
		current.movementFlags &= ~PMF_JUMP_HELD;
	}
	if ( command.upmove > -10 ) {
		// not holding crouch
		current.movementFlags &= ~PMF_CROUCH_HELD;
	}

	// if no movement at all
	if ( current.movementType == PM_FREEZE ) {
		playerSelf->CheckWaterEffectsOnly();
		return;
	}

	// move the player velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	// view vectors
	viewAngles.ToVectors( &viewForward, NULL, NULL );
	viewForward *= clipModelAxis;
	viewRight = gravityNormal.Cross( viewForward );
	viewRight.Normalize();

	// fly in spectator mode
	if ( current.movementType == PM_SPECTATOR ) {
		SpectatorMove();
		playerSelf->CheckWaterEffectsOnly();
		DropTimers();
		return;
	}

	// special no clip mode
	if ( current.movementType == PM_NOCLIP ) {
		NoclipMove();
		playerSelf->CheckWaterEffectsOnly();
		DropTimers();
		return;
	}

	// no control when dead
	if ( current.movementType == PM_DEAD || !movementAllowed ) {
		command.forwardmove = 0;
		command.rightmove = 0;
		command.upmove = 0;
	}

	// set waterlevel
	SetWaterLevel();

	// check for ground
	CheckGround();

	// check if up against a ladder
	CheckLadder();

	// set clip model size
	CheckStance();

	// handle timers
	DropTimers();

	// move
	if ( current.movementType == PM_DEAD ) {
		// dead
		DeadMove();
	} else if ( ladder.IsValid() ) {
		// going up or down a ladder
		LadderMove();
	} else if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		// jumping out of water
		WaterJumpMove();
	} else if ( waterLevel > WATERLEVEL_FEET ) {
		// swimming
		WaterMove();
	} else if ( walking ) {
		// walking on ground
		WalkMove( true );
	} else {
		// airborne
		AirMove( true );
	}

	// set waterlevel and groundentity
//	SetWaterLevel();
//	CheckGround();

	// move the player velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();
}

/*
================
idPhysics_Player::~idPhysics_Player
================
*/
idPhysics_Player::~idPhysics_Player( void ) {
	if ( proneLegsClipModel != NULL ) {
		gameLocal.clip.DeleteClipModel( proneLegsClipModel );
	}
	if ( headClipModel != NULL ) {
		gameLocal.clip.DeleteClipModel( headClipModel );
	}
	if ( clipModel_normal != NULL ) {
		gameLocal.clip.DeleteClipModel( clipModel_normal );
	}
	if ( clipModel_normalShot != NULL ) {
		gameLocal.clip.DeleteClipModel(	clipModel_normalShot );
	}
	if ( clipModel_crouch != NULL ) {
		gameLocal.clip.DeleteClipModel(	clipModel_crouch );
	}
	if ( clipModel_crouchShot != NULL ) {
		gameLocal.clip.DeleteClipModel(	clipModel_crouchShot );
	}
	if ( clipModel_prone != NULL ) {
		gameLocal.clip.DeleteClipModel( clipModel_prone );
	}
	if ( clipModel_proneShot != NULL ) {
		gameLocal.clip.DeleteClipModel( clipModel_proneShot );
	}
	if ( clipModel_dead != NULL ) {
		gameLocal.clip.DeleteClipModel(	clipModel_dead );
	}
	if ( clipModel_deadShot != NULL ) {
		gameLocal.clip.DeleteClipModel(	clipModel_deadShot );
	}

	if ( clipModel_vehicle != NULL ) {
		gameLocal.clip.DeleteClipModel(	clipModel_vehicle );
	}
	if ( clipModel_vehicleShot != NULL ) {
		gameLocal.clip.DeleteClipModel(	clipModel_vehicleShot );
	}

	if ( groundCheckModel != NULL ) {
		gameLocal.clip.DeleteClipModel(	groundCheckModel );
	}

	clipModel = NULL;
}

/*
================
idPhysics_Player::idPhysics_Player
================
*/
idPhysics_Player::idPhysics_Player( void ) {
	clipMask = 0;
	memset( &current, 0, sizeof( current ) );
	saved = current;
	lastClippedOrigin.Zero();
	numMergedFrames = 0;
	mergeThisFrame = false;
	skipContactsThisFrame = false;
	walkSpeedFwd = 0;
	walkSpeedBack = 0;
	walkSpeedSide = 0;
	crouchSpeed = 0;
	maxStepHeight = 0;
	maxJumpHeight = 0;
	memset( &command, 0, sizeof( command ) );
	viewAngles.Zero();
	framemsec = 0;
	frametime = 0;
	playerSpeed = 0;
	viewForward.Zero();
	viewRight.Zero();
	walking = false;
	groundPlane = false;
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	groundMaterial = NULL;
	ladder = NULL;
	waterLevel = WATERLEVEL_NONE;
	frozen = false;
	movementAllowed = true;
	proneChangeEndTime = 0;
	proneModelOffset = vec3_zero;
	leanFraction = 0.f;
	leanOffset = 0.f;

	pm_stopSpeed		= CONST_PM_STOPSPEED;
	pm_swimScale		= CONST_PM_SWIMSCALE;
	pm_ladderSpeed		= CONST_PM_LADDERSPEED;
	pm_stepScale		= CONST_PM_STEPSCALE;

	pm_accelerate		= CONST_PM_ACCELERATE;
	pm_airAccelerate	= CONST_PM_AIRACCELERATE;
	pm_waterAccelerate	= CONST_PM_WATERACCELERATE;
	pm_flyAccelerate	= CONST_PM_FLYACCELERATE;

	pm_friction			= -1; // Gordon: controlled by a cvar now, but if overridden will use the value here
	pm_airFriction		= CONST_PM_AIRFRICTION;
	pm_waterFriction	= CONST_PM_WATERFRICTION;
	pm_flyFriction		= CONST_PM_FLYFRICTION;
	pm_noclipFriction	= CONST_PM_NOCLIPFRICTION;

	proneLegsClipModel	= new idClipModel( idTraceModel( playerProneLegsBounds ), false );
	proneLegsClipModel->SetContents( CONTENTS_SLIDEMOVER );

	headClipModel		= new idClipModel( idTraceModel( playerHeadBounds ), false );
	headClipModel->SetContents( CONTENTS_RENDERMODEL );

	shotClipModel		= NULL;

	clipModel_normal					= NULL;
	clipModel_normalShot				= NULL;
	clipModel_crouch					= NULL;
	clipModel_crouchShot				= NULL;
	clipModel_prone						= NULL;
	clipModel_proneShot					= NULL;
	clipModel_dead						= NULL;
	clipModel_deadShot					= NULL;
	clipModel_vehicle					= NULL;
	clipModel_vehicleShot				= NULL;
	groundCheckModel					= NULL;

	jumpAllowedTime		= 0;
	lastSnapshotTime	= 0;
}


/*
================
idPhysics_Player::BuildClipModel
================
*/
void idPhysics_Player::BuildClipModel( const idBounds& bounds, bool useCylinder, idClipModel*& model ) {
	// delete the old model if the bounds has changed
	if ( model != NULL && !model->GetBounds().Compare( bounds, 0.5f ) ) {
		gameLocal.clip.DeleteClipModel( model );
		model = NULL;
	}

	if ( model == NULL ) {
		// make the new model
		if ( useCylinder ) {
			model = new idClipModel( idTraceModel( bounds, 8 ), false );
		} else {
			model = new idClipModel( idTraceModel( bounds ), false );
		}
	}

	// put it in the right position
	model->Translate( PlayerGetOrigin() - model->GetOrigin(), gameLocal.clip );
}

/*
================
idPhysics_Player::SetupPlayerClipModels
================
*/
void idPhysics_Player::SetupPlayerClipModels( void ) {
	idBounds bounds;

	if ( playerSelf->IsSpectating() ) {
		CalcSpectateBounds( bounds );
	} else {
		CalcNormalBounds( bounds );
	}

	// normal
	BuildClipModel( bounds, false, clipModel_normal );
	BuildClipModel( bounds, true, clipModel_normalShot );

	// crouch
	bounds.GetMaxs().z = pm_crouchheight.GetFloat();
	BuildClipModel( bounds, false, clipModel_crouch );
	BuildClipModel( bounds, true, clipModel_crouchShot );

	// prone
	bounds.GetMaxs().z = pm_proneheight.GetFloat();
	BuildClipModel( bounds, false, clipModel_prone );
	BuildClipModel( bounds, true, clipModel_proneShot );

	// dead
	bounds.GetMaxs().z = pm_deadheight.GetFloat();
	BuildClipModel( bounds, false, clipModel_dead );
	BuildClipModel( bounds, false, clipModel_deadShot );

	clipModel = clipModel_normal;
	shotClipModel = clipModel_normalShot;

	//
	// Build the clip model used for CheckGround - bottom quad only
	//
	bounds.GetMaxs().z = bounds.GetMins().z;
	if ( groundCheckModel != NULL && !groundCheckModel->GetBounds().Compare( bounds, 0.5f ) ) {
		gameLocal.clip.DeleteClipModel( groundCheckModel );
		groundCheckModel = NULL;
	}

	if ( groundCheckModel == NULL ) {
		const idVec3& min = bounds.GetMins();
		const idVec3& max = bounds.GetMaxs();

		// make the new model
		idVec3 verts[ 4 ] = {
			idVec3( max.x, max.y, min.z ),
			idVec3( max.x, min.y, min.z ),
			idVec3( min.x, min.y, min.z ),
			idVec3( min.x, max.y, min.z )
		};

		idTraceModel trm;
		trm.SetupPolygon( verts, 4 );
		groundCheckModel = new idClipModel( trm, false );
	}
}

/*
================
idPhysics_Player::SetupVehicleClipModels
================
*/
void idPhysics_Player::SetupVehicleClipModels( float playerHeight ) {
	idBounds bounds;
	CalcNormalBounds( bounds );

	bounds.GetMaxs().z = playerHeight;
	BuildClipModel( bounds, false, clipModel_vehicle );
	BuildClipModel( bounds, true, clipModel_vehicleShot );
}

/*
================
idPhysics_Player::GetClipModel
================
*/
idClipModel* idPhysics_Player::GetClipModel( int id ) const {
	if ( id == 0 ) {
		return clipModel;
	} else if ( id == 1 ) {
		return shotClipModel;
	} else if ( id == 2 ) {
		return proneLegsClipModel;
	} else if ( id == 3 ) {
		return headClipModel;
	}

	return NULL;
}

/*
================
idPhysics_Player::SetPlayerInput
================
*/
void idPhysics_Player::SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles, bool allowMovement ) {
	command			= cmd;
	viewAngles		= newViewAngles;		// can't use cmd.angles cause of the delta_angles
	movementAllowed	= allowMovement;
}

/*
================
idPhysics_Player::SetSpeed
================
*/
void idPhysics_Player::SetSpeed( float newWalkSpeedFwd, float newWalkSpeedBack, float newWalkSpeedSide, float newCrouchSpeed, float newProneSpeed ) {
	walkSpeedFwd	= newWalkSpeedFwd;
	walkSpeedBack	= newWalkSpeedBack;
	walkSpeedSide	= newWalkSpeedSide;
	crouchSpeed		= newCrouchSpeed;
	proneSpeed		= newProneSpeed;
}

/*
================
idPhysics_Player::SetMaxStepHeight
================
*/
void idPhysics_Player::SetMaxStepHeight( const float newMaxStepHeight ) {
	maxStepHeight = newMaxStepHeight;
}

/*
================
idPhysics_Player::GetMaxStepHeight
================
*/
float idPhysics_Player::GetMaxStepHeight( void ) const {
	return maxStepHeight;
}

/*
================
idPhysics_Player::SetMaxJumpHeight
================
*/
void idPhysics_Player::SetMaxJumpHeight( const float newMaxJumpHeight ) {
	maxJumpHeight = newMaxJumpHeight;
}

/*
================
idPhysics_Player::SetMovementType
================
*/
void idPhysics_Player::SetMovementType( const pmtype_t type ) {
	current.movementType = type;
}

/*
================
idPhysics_Player::SetKnockBack
================
*/
void idPhysics_Player::SetKnockBack( const int knockBackTime ) {
	if ( current.movementTime != 0 ) {
		return;
	}
	current.movementFlags |= PMF_TIME_KNOCKBACK;
	current.movementTime = gameLocal.time + knockBackTime;
}

/*
================
idPhysics_Player::Evaluate
================
*/
bool idPhysics_Player::Evaluate( int timeStepMSec, int endTimeMSec ) {
	idVec3 masterOrigin, oldOrigin;
	idMat3 masterAxis;

	// eliminate any values that are too small - little bit of a hack, but prevents denormals
	current.velocity.FixDenormals();
	current.pushVelocity.FixDenormals();
	current.origin.FixDenormals();
	lastClippedOrigin.FixDenormals();

	waterLevel = WATERLEVEL_NONE;
	oldOrigin = current.origin;

	// if bound to a master
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + localOrigin * masterAxis;
		lastClippedOrigin = current.origin;
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
		if ( timeStepMSec > 0 ) {
			current.velocity = ( current.origin - oldOrigin ) / ( timeStepMSec * 0.001f );
		}
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
		return true;
	}

	ActivateContactEntities();

	MovePlayer( timeStepMSec );

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
	if ( IsProne() && current.movementType == PM_NORMAL ) {
		idVec3 legsOrg = GetProneLegsPos( current.origin, viewAngles );
		proneLegsClipModel->Link( gameLocal.clip, self, 2, legsOrg, mat3_identity );
	} else {
		proneLegsClipModel->Unlink( gameLocal.clip );
	}

//	if ( IsOutsideWorld() ) {
//		gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString(0) );
//	}

	return current.origin != oldOrigin;
}

/*
================
idPhysics_Player::UpdateTime
================
*/
void idPhysics_Player::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_Player::GetTime
================
*/
int idPhysics_Player::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
idPhysics_Player::GetImpactInfo
================
*/
void idPhysics_Player::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
idPhysics_Player::ApplyImpulse
================
*/
void idPhysics_Player::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( current.movementType != PM_NOCLIP ) {
		idVec3 add = impulse * invMass;
		if ( groundPlane ) {
			add -= ( add * groundTrace.c.normal ) * groundTrace.c.normal;
		}
		current.velocity += add;
	}
}

/*
================
idPhysics_Player::IsAtRest
================
*/
bool idPhysics_Player::IsAtRest( void ) const {
	return false;
}

/*
================
idPhysics_Player::GetRestStartTime
================
*/
int idPhysics_Player::GetRestStartTime( void ) const {
	return -1;
}

/*
================
idPhysics_Player::SaveState
================
*/
void idPhysics_Player::SaveState( void ) {
	saved = current;
}

/*
================
idPhysics_Player::RestoreState
================
*/
void idPhysics_Player::RestoreState( void ) {
	current = saved;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );

	EvaluateContacts( CLIP_DEBUG_PARMS_CLIENTINFO_ONLY( self ) );
}

/*
================
idPhysics_Player::SetOrigin
================
*/
void idPhysics_Player::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( masterEntity != NULL ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
		localOrigin = newOrigin;
	} else {
		current.origin = newOrigin;
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	if ( shotClipModel != NULL ) {
		shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
	}

	ResetCollisionMerge( current.origin );
}

/*
================
idPhysics_Player::ResetCollisionMerge
================
*/
void idPhysics_Player::ResetCollisionMerge( const idVec3& origin ) {
	// reset the clip merging position
	lastClippedOrigin = origin;
	numMergedFrames = 0;
}

/*
================
idPhysics_Player::GetOrigin
================
*/
const idVec3 & idPhysics_Player::PlayerGetOrigin( void ) const {
	return current.origin;
}

/*
================
idPhysics_Player::SetAxis
================
*/
void idPhysics_Player::SetAxis( const idMat3 &newAxis, int id ) {
	clipModel->Link( gameLocal.clip, self, 0, current.origin, newAxis );
	shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
}

/*
================
idPhysics_Player::Translate
================
*/
void idPhysics_Player::Translate( const idVec3 &translation, int id ) {

	if ( masterEntity != NULL ) {
		localOrigin += translation;
	}
	current.origin += translation;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
}

/*
================
idPhysics_Player::Rotate
================
*/
void idPhysics_Player::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.origin *= rotation;
	if ( masterEntity != NULL ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
	shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
}

/*
================
idPhysics_Player::SetContents
================
*/
void idPhysics_Player::SetContents( int contents, int id ) {
	if ( id <= 0 ) {
		clipModel->SetContents( contents );
	} else if ( id == 1 ) {
		shotClipModel->SetContents( contents );
	}
}

/*
================
idPhysics_Player::SetLinearVelocity
================
*/
void idPhysics_Player::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
}

/*
================
idPhysics_Player::GetLinearVelocity
================
*/
const idVec3 &idPhysics_Player::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
idPhysics_Player::SetPushed
================
*/
void idPhysics_Player::SetPushed( int deltaTime ) {
	idVec3 velocity;
	float d;

	// velocity with which the player is pushed
	velocity = ( current.origin - saved.origin ) / ( deltaTime * idMath::M_MS2SEC );

	// remove any downward push velocity
	d = velocity * gravityNormal;
	if ( d > 0.0f ) {
		velocity -= d * gravityNormal;
	}

	current.pushVelocity += velocity;
}

/*
================
idPhysics_Player::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_Player::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

/*
================
idPhysics_Player::ClearPushedVelocity
================
*/
void idPhysics_Player::ClearPushedVelocity( void ) {
	current.pushVelocity.Zero();
}

/*
================
idPhysics_Player::SetMaster

  the binding is never orientated
================
*/
void idPhysics_Player::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			masterEntity = master;
			masterYaw = masterAxis[0].ToYaw();
		}
		ClearContacts();
	}
	else {
		if ( masterEntity ) {
			masterEntity = NULL;
		}
	}
}

const float	PLAYER_ORIGIN_MAX				= 32767;
const int	PLAYER_ORIGIN_TOTAL_BITS		= 24;
const int	PLAYER_ORIGIN_EXPONENT_BITS		= idMath::BitsForInteger( idMath::BitsForFloat( PLAYER_ORIGIN_MAX ) ) + 1;
const int	PLAYER_ORIGIN_MANTISSA_BITS		= PLAYER_ORIGIN_TOTAL_BITS - 1 - PLAYER_ORIGIN_EXPONENT_BITS;

const float	PLAYER_VELOCITY_MAX				= 4000;
const int	PLAYER_VELOCITY_TOTAL_BITS		= 16;
const int	PLAYER_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( PLAYER_VELOCITY_MAX ) ) + 1;
const int	PLAYER_VELOCITY_MANTISSA_BITS	= PLAYER_VELOCITY_TOTAL_BITS - 1 - PLAYER_VELOCITY_EXPONENT_BITS;

const int	PLAYER_MOVEMENT_TYPE_BITS		= 3;
const int	PLAYER_MOVEMENT_FLAGS_BITS		= 10;

/*
================
idPhysics_Player::CheckNetworkStateChanges
================
*/
bool idPhysics_Player::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdPlayerPhysicsNetworkData );

		NET_CHECK_FIELD( origin, current.origin );
		NET_CHECK_FIELD( velocity, current.velocity );
		NET_CHECK_FIELD( movementTime, current.movementTime );
		NET_CHECK_FIELD( movementFlags, current.movementFlags );
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdPlayerPhysicsBroadcastData );

		NET_CHECK_FIELD( pushVelocity, current.pushVelocity );
		NET_CHECK_FIELD( localOrigin, localOrigin );
		NET_CHECK_FIELD( frozen, frozen );
		NET_CHECK_FIELD( proneChangeEndTime, proneChangeEndTime );
		NET_CHECK_FIELD( jumpAllowedTime, jumpAllowedTime );

		return false;
	}

	return false;
}

/*
================
idPhysics_Player::WriteNetworkState
================
*/
void idPhysics_Player::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdPlayerPhysicsNetworkData );

		newData.origin			= current.origin;
		newData.velocity		= current.velocity;
		newData.movementTime	= current.movementTime;
		newData.movementFlags	= current.movementFlags;

		msg.WriteDeltaVector( baseData.origin, newData.origin, PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.velocity, newData.velocity, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaLong( baseData.movementTime, newData.movementTime );
		msg.WriteDelta( baseData.movementFlags, newData.movementFlags, PLAYER_MOVEMENT_FLAGS_BITS );
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPlayerPhysicsBroadcastData );

		newData.pushVelocity		= current.pushVelocity;
		newData.localOrigin			= localOrigin;
		newData.frozen				= frozen;
		newData.proneChangeEndTime	= proneChangeEndTime;
		newData.jumpAllowedTime		= jumpAllowedTime;

		msg.WriteDeltaVector( baseData.pushVelocity, newData.pushVelocity, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.localOrigin, newData.localOrigin );
		msg.WriteBool( newData.frozen );
		msg.WriteDeltaLong( baseData.proneChangeEndTime, newData.proneChangeEndTime );
		msg.WriteDeltaLong( baseData.jumpAllowedTime, newData.jumpAllowedTime );

		return;
	}
}

/*
================
idPhysics_Player::ApplyNetworkState
================
*/
void idPhysics_Player::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdPlayerPhysicsNetworkData );

		current.origin			= newData.origin;
		current.velocity		= newData.velocity;
		current.movementTime	= newData.movementTime;

		int diff = newData.movementFlags ^ current.movementFlags;
		current.movementFlags	= newData.movementFlags;
		if ( diff & ( PMF_DUCKED | PMF_PRONE ) ) {
			UpdateBounds();
		}

		if ( clipModel ) {
			clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		}
		if ( shotClipModel ) {
			shotClipModel->Link( gameLocal.clip, self, 1, current.origin, clipModel->GetAxis() );
		}

		self->UpdateVisuals();


		if ( self != gameLocal.GetLocalViewPlayer() ) {
			lastClippedOrigin		= current.origin;
			numMergedFrames			= 0;
		} else if ( !playerSelf->GetNoClip() ) {
			CheckGround();
		}

		lastSnapshotTime		= gameLocal.time;
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPlayerPhysicsBroadcastData );

		current.pushVelocity	= newData.pushVelocity;
		localOrigin				= newData.localOrigin;
		frozen					= newData.frozen;
		proneChangeEndTime		= newData.proneChangeEndTime;
		jumpAllowedTime			= newData.jumpAllowedTime;
		return;
	}
}

/*
================
idPhysics_Player::ResetNetworkState
================
*/
void idPhysics_Player::ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPlayerPhysicsBroadcastData );

		current.pushVelocity	= newData.pushVelocity;
		localOrigin				= newData.localOrigin;
		frozen					= newData.frozen;
		proneChangeEndTime		= newData.proneChangeEndTime;
		jumpAllowedTime			= newData.jumpAllowedTime;
		return;
	}
}

/*
================
idPhysics_Player::ReadNetworkState
================
*/
void idPhysics_Player::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdPlayerPhysicsNetworkData );

 		newData.origin			= msg.ReadDeltaVector( baseData.origin, PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
		newData.velocity		= msg.ReadDeltaVector( baseData.velocity, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
		newData.movementTime	= msg.ReadDeltaLong( baseData.movementTime );
		newData.movementFlags	= msg.ReadDelta( baseData.movementFlags, PLAYER_MOVEMENT_FLAGS_BITS );

		newData.origin.FixDenormals();
		newData.velocity.FixDenormals();

		self->OnNewOriginRead( newData.origin );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPlayerPhysicsBroadcastData );

		newData.pushVelocity		= msg.ReadDeltaVector( baseData.pushVelocity, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
		newData.localOrigin			= msg.ReadDeltaVector( baseData.localOrigin );
		newData.frozen				= msg.ReadBool();
		newData.proneChangeEndTime	= msg.ReadDeltaLong( baseData.proneChangeEndTime );
		newData.jumpAllowedTime		= msg.ReadDeltaLong( baseData.jumpAllowedTime );
		return;
	}
}

/*
================
idPhysics_Player::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idPhysics_Player::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdPlayerPhysicsNetworkData();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdPlayerPhysicsBroadcastData();
	}
	return NULL;
}

/*
================
idPhysics_Player::GetOrigin
================
*/
const idVec3& idPhysics_Player::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
idPhysics_Player::OnLadder
================
*/
bool idPhysics_Player::OnLadder( void ) const {
	return ladder.IsValid();
}

/*
================
idPhysics_Player::GetLadderNormal
================
*/
idVec3 idPhysics_Player::GetLadderNormal( void ) const {
	if ( !ladder.IsValid() ) {
		return vec3_origin;
	}
	return ladder->GetLadderNormal();
}

/*
================
idPhysics_Player::SetSelf
================
*/
void idPhysics_Player::SetSelf( idEntity *e ) {
	playerSelf = e->Cast< idPlayer >();
	assert( playerSelf != NULL );

	idPhysics_Actor::SetSelf( e );
}

/*
================
idPhysics_Player::EnableHeadClipModel
================
*/
void idPhysics_Player::EnableHeadClipModel( void ) {
	if ( headClipModel == NULL ) {
		return;
	}

	// put the head model in its appropriate position
	idVec3 headOrigin;
	playerSelf->GetHeadModelCenter( headOrigin );

	headClipModel->Link( gameLocal.clip, self, 3, headOrigin, mat3_identity );
}

/*
================
idPhysics_Player::DisableHeadClipModel
================
*/
void idPhysics_Player::DisableHeadClipModel( void ) {
	if ( headClipModel != NULL ) {
		headClipModel->Unlink( gameLocal.clip );
	}
}

