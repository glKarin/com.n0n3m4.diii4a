#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// HUMANHEAD nla - Class constants
const int	hhPhysics_AI::NO_FLY_DIRECTION = 1000;

CLASS_DECLARATION( idPhysics_Monster, hhPhysics_AI )
END_CLASS

/*
=====================
hhPhysics_AI::hhPhysics_AI
=====================
*/
hhPhysics_AI::hhPhysics_AI() {
	// HUMANHEAD nla
	flyStepDirection = NO_FLY_DIRECTION;
	//HUMANHEAD END
	lastMoveTouch = NULL;
	useGravity = TRUE;
	bGravClipModelAxis = false;
}

/*
=====================
hhPhysics_AI::SlideMove
=====================
// HUMANHEAD nla - if touched is NULL, the touched entities will be added 
//		directly.  Otherwise they will be added to the list
*/
monsterMoveResult_t hhPhysics_AI::SlideMove( idVec3 &start, idVec3 &velocity, const idVec3 &delta, idList<int> *touched ) {
	trace_t	tr;
	idVec3	move;
	int		i;

	blockingEntity = NULL;
	move = delta;
	for( i = 0; i < 3; i++ ) {
		gameLocal.clip.Translation( tr, start, start + move, clipModel, clipModel->GetAxis(), clipMask, self );

		if ( tr.c.entityNum != ENTITYNUM_WORLD && tr.c.entityNum != ENTITYNUM_NONE && gameLocal.entities[ tr.c.entityNum ]  )
			lastMoveTouch = gameLocal.entities[ tr.c.entityNum ];
		

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

		/*
		// clip the movement delta and velocity
		move.ProjectOntoPlane( tr.c.normal, OVERCLIP );
		velocity.ProjectOntoPlane( tr.c.normal, OVERCLIP );
		*/

		// HUMANHEAD nla - Added logic to allow monsters to push
		// if we can push other entities and not blocked by the world.  Added PushCosine
		if ( self->Pushes() && ( tr.c.entityNum != ENTITYNUM_WORLD ) ) {
			// Early out if we aren't facing enough towards the object
			idVec3 normVelocity( velocity );
			
			normVelocity.NormalizeFast();
			/*
			gameLocal.Printf( "%d Trying %s * %s = %.2f vs %.2f\n", gameLocal.time,
				normVelocity.ToString(), tr.c.normal.ToString(),
				idMath::Fabs( normVelocity * tr.c.normal ), self->PushCosine() );
			*/
			if ( idMath::Fabs( normVelocity * tr.c.normal ) >= self->PushCosine() ) {
				trace_t trPush;
				int pushFlags;
				float totalMass;
	
				clipModel->SetPosition( start, clipModel->GetAxis() );
	
				// clip movement, only push idMoveables, don't push entities the player is standing on
				// apply impact to pushed objects
				pushFlags = PUSHFL_CLIP|PUSHFL_ONLYMOVEABLE|PUSHFL_NOGROUNDENTITIES;
	
				// clip & push
				totalMass = gameLocal.push.ClipTranslationalPush( trPush, self, pushFlags, start + move, move );
	
				if ( totalMass > 0.0f ) {
					// decrease velocity based on the total mass of the objects being pushed ?
					/*? Put back in later?
					if ( velocity.LengthSqr() > Square( crouchSpeed * 0.8f ) ) {
						velocity *= crouchSpeed * 0.8f / velocity.Length();
					}
					pushed = true;
					*/
				}
	
				// Added logic to prevent pushing
				tr = trPush;
				start = tr.endpos;
				// time_left -= time_left * trace.fraction;
	
				// if moved the entire distance
				// Changed from orig. 
				/*
				if ( trace.fraction >= 1.0f ) {
					break;
				}
				*/
				// Changed to this!  = )
				if ( tr.fraction == 1.0f ) {
					if ( i > 0 ) {
						return MM_SLIDING;
					}
					return MM_OK;
				}
			}			
		}
		// HUMANHEAD END
		
		// clip the movement delta and velocity
		move.ProjectOntoPlane( tr.c.normal, OVERCLIP );
		velocity.ProjectOntoPlane( tr.c.normal, OVERCLIP );
	}

/* NOTE: Underlying code changed some, fit this back in if needed.
		//HUMANHEAD nla
		if (touched == NULL) {
			if (tr.fraction < 1.0f) {
				AddTouchEnt(tr.c.entityNum);
			}
		}
		else {
			touched->Append(tr.c.entityNum);
		}
*/
	return MM_BLOCKED;
}

// HUMANHEAD nla
/*
=====================
hhPhysics_AI::FlyMove

  move start into the delta direction
  the velocity is clipped conform any collisions
=====================
*/
#define NUM_DIRECTIONS 2
monsterMoveResult_t hhPhysics_AI::FlyMove( idVec3 &start, idVec3 &velocity, const idVec3 &delta ) {
	trace_t		tr;
	idVec3		up, down, noStepPos, noStepVel, stepPos, stepVel, dirGrav;
	monsterMoveResult_t result1, result2;
	idVec3		originalStart, originalVelocity, bestStart, bestVelocity;
	float		noStepDistSq, distSq, bestDistSq;
	int			bestFlyStepDirection;
	monsterMoveResult_t	    bestReturn;
	idList<int>			noStepEntities, stepEntities, bestEntities;
	
	
	if ( delta == vec3_origin ) {
		return MM_OK;
	}

	// Initialize
	originalStart = start;
	originalVelocity = velocity;
	bestFlyStepDirection = NO_FLY_DIRECTION;

	// try to move without stepping up
	noStepPos = start;
	noStepVel = velocity;
	result1 = SlideMove( noStepPos, noStepVel, delta, &noStepEntities );
	if ( result1 == MM_OK ) {
		AddTouchEntList( noStepEntities );
		start = noStepPos;
		velocity = noStepVel;
		return MM_OK;
	}

	// Assume no stepping is the best
	bestStart = noStepPos;
	bestVelocity = noStepVel;
	bestReturn = result1;
	bestEntities = noStepEntities;
	// Add small fudge factor to take into account float issues
	noStepDistSq = ((noStepPos - originalStart) * 1.001f).LengthSqr();
	bestDistSq = noStepDistSq;

	// Try to move around the obstacle
	for ( int direction = 0; direction < NUM_DIRECTIONS ; direction++ ) {
		stepEntities.Clear();
		
		//! Maybe make this relative to the clip model?
		if (direction == 0) {	// Set the direction.  Assumes 2x
			dirGrav.Set(0, 0, -1);
		}
		else {
			dirGrav *= -1;
		}

		// try to step "up"
		up = start - dirGrav * maxStepHeight;
		gameLocal.clip.Translation( tr, start, up, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( tr.fraction == 0.0f ) {
			start = originalStart;				// Reset
			velocity = originalVelocity;
			continue;
		}
			
		// Add the entity touched
		if ( tr.fraction < 1.0f ) {
			stepEntities.Append( tr.c.entityNum );
		}

		// try to move at the stepped up position
		stepPos = tr.endpos;
		stepVel = velocity;
		result2 = SlideMove( stepPos, stepVel, delta, &stepEntities );
		if ( result2 == MM_BLOCKED ) {					// Couldn't move all the way at stepped pos
			start = originalStart;		// Reset
			velocity = originalVelocity;
			distSq = (stepPos - tr.endpos).LengthSqr();	// See if we moved further up there.  Ignore the step up
			if ((distSq > bestDistSq)) {
				bestStart = stepPos;
				bestVelocity = stepVel;
				bestReturn = result2;
				bestEntities = stepEntities;
				bestDistSq = distSq;
				bestFlyStepDirection = direction;
			}
			continue;
		}

		// Stepping is the best move
		start = originalStart;		// Reset
		velocity = originalVelocity;
		distSq = ( stepPos - originalStart ).LengthSqr();	// Include the distance up we traveled
		if ( ( direction == flyStepDirection ) ||		// Going the same direction as last time
			 ( ( ( direction < flyStepDirection ) ||	// Haven't tested the last direction
				 ( bestFlyStepDirection != flyStepDirection ) ) &&		// Have tested the last, but coundn't go that way
			   ( distSq > bestDistSq ) )				// Closer than the best
			 ) {				
			bestStart = stepPos;	// Save the no step version
			bestVelocity = stepVel;
			bestReturn = MM_STEPPED;
			bestEntities = stepEntities;
			bestDistSq = distSq;
			bestFlyStepDirection = direction;
		}
		
	}	//. Direction loop

	// Didn't work, use the no step return
	start = bestStart;
	velocity = bestVelocity;
	flyStepDirection = bestFlyStepDirection;
	AddTouchEntList( bestEntities );

	return( bestReturn );
}

/*
================
hhPhysics_AI::Evaluate
================
*/
bool hhPhysics_AI::Evaluate( int timeStepMSec, int endTimeMSec ) {
	idVec3 masterOrigin, oldOrigin;
	idMat3 masterAxis;
	float timeStep;
	float oldMasterYaw, oldMasterDeltaYaw;	// HUMANHEAD pdm

	timeStep = MS2SEC( timeStepMSec );

	moveResult = MM_OK;
	blockingEntity = NULL;
	oldOrigin = current.origin;
	oldMasterYaw = masterYaw;			// HUMANHEAD pdm
	oldMasterDeltaYaw = masterDeltaYaw;	// HUMANHEAD pdm
	
	//HUMANHEAD: aob
	HadGroundContacts( HasGroundContacts() );
	//HUMANHEAD END

	// if bound to a master
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		//HUMANHEAD rww
		if (!timeStep) {
			current.velocity = vec3_origin;
		}
		else {
		//HUMANHEAD END
			current.velocity = ( current.origin - oldOrigin ) / timeStep;
		}
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;

		// HUMANHEAD pdm: If we haven't moved and master is at rest, put me to rest
		if (current.origin == oldOrigin && masterYaw == oldMasterYaw && masterDeltaYaw == oldMasterDeltaYaw) {
			if (masterEntity->IsAtRest()) {
				Rest();
			}
			return false;
		}
		// HUMANHEAD END
		return true;
	}

	// if the monster is at rest
	if ( current.atRest >= 0 ) {
		return false;
	}

	assert(timeStep != 0.0f); //HUMANHEAD rww

	ActivateContactEntities();

	// move the monster velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	clipModel->Unlink();

	// check if on the ground
	//HUMANHEAD: aob - moved ground check to after movement

	// if not on the ground or moving upwards
	float upspeed;
	if ( gravityNormal != vec3_zero ) {
		upspeed = -( current.velocity * gravityNormal );
	} else {
		upspeed = current.velocity.z;
	}
	if ( fly || self->fl.isTractored || ( !forceDeltaMove && ( !current.onGround || upspeed > 1.0f ) ) ) {
		if ( upspeed < 0.0f ) {
			moveResult = MM_FALLING;
		}
		else {
			current.onGround = false;
			moveResult = MM_OK;
		}
		delta = current.velocity * timeStep;
		if ( delta != vec3_origin ) {
			//HUMANHEAD: aob - removed scope hardcode
			moveResult = SlideMove( current.origin, current.velocity, delta );
            delta.Zero();
		}

		if ( !fly && !self->fl.isTractored && IsGravityEnabled()) { // HUMANHEAD JRM: Gravity toggle
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
			//HUMANHEAD: aob
			current.velocity = delta / timeStep;
			//HUMANHEAD END

			//HUMANHEAD: aob - removed scope hardcode
			//moveResult = idPhysics_Monster::StepMove( current.origin, current.velocity, delta );
			if( IsGravityEnabled() ) {
				moveResult = StepMove( current.origin, current.velocity, delta );
			} else {
				moveResult = SlideMove( current.origin, current.velocity, delta );
			}
			
			delta.Zero();
		}
	}
	
	//HUMANHEAD: aob - for changing gravity zones
	idVec3 rotationCheckOrigin = GetOrigin() + GetAxis()[2] * GetBounds()[1].z;
	IterativeRotateMove( GetAxis()[2], -GetGravityNormal(), GetOrigin(), rotationCheckOrigin, p_iterRotMoveNumIterations.GetInteger() );
	//HUMANHEAD END

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	
	// check if on the ground
	//HUMANHEAD: aob - removed scope hardcode
	CheckGround( current );

	// get all the ground contacts
	EvaluateContacts();

	// move the monster velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();

	if ( IsOutsideWorld() ) {
		// HUMANHEAD pdm: Allow some things to go outside world without warning
		if (!self->IsType(hhWraith::Type) && !self->IsType( hhTalon::Type ) ) {
			gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString(0) );
		}
		Rest();
	}

	return ( current.origin != oldOrigin );
}

/*
==================
hhPhysics_AI::ApplyFriction

Handles both ground friction and water friction

//HUMANHEAD: aob
==================
*/
idVec3 hhPhysics_AI::ApplyFriction( const idVec3& vel, const float deltaTime ) {
	float	speed = 0.0f;
	float	newSpeed = 0.0f;
	float	control = 0.0f;
	float	drop = 0.0f;
	idVec3	velocity = vel;

	speed = velocity.Length();
	if( speed <= VECTOR_EPSILON ) {
		return vec3_origin;
	}

	if( fly || self->fl.isTractored ) {
		drop += speed * PM_FLYFRICTION * deltaTime;	
	}
	else if( !current.onGround ) {
		drop += speed * PM_AIRFRICTION * deltaTime;
	}
	else if( current.onGround ) {
		if( !(GetGroundSurfaceFlags() & SURF_SLICK) ) {
			drop += speed * PM_FRICTION * deltaTime;
		}
	}

	// scale the velocity
	newSpeed = speed - drop;
	if( newSpeed < 0.0f ) {
		newSpeed = 0.0f;
	}

	return vel * ( newSpeed / speed );
}

/*
================
hhPhysics_AI::AddForce
	HUMANHEAD pdm: added so that springs will work on monsters
================
*/
void hhPhysics_AI::AddForce( const int id, const idVec3 &point, const idVec3 &force ) {
	// HUMANHEAD pdm: so we can use forces on them
	current.velocity += (force / GetMass()) * USERCMD_ONE_OVER_HZ;

	Activate();
}

/*
================
hhPhysics_AI::SetLinearVelocity
================
*/
void hhPhysics_AI::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;

	Activate();
}

/*
================
hhPhysics_AI::GetLinearVelocity
================
*/
const idVec3& hhPhysics_AI::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
hhPhysics_AI::ApplyImpulse
================
*/
void hhPhysics_AI::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	// HUMANHEAD: aob - so we can use forces on them (crane)
	current.velocity += impulse / GetMass();
	// HUMANHEAD END

	Activate();
}

/*
================
hhPhysics_AI::SetMaster
overridden to set localAxis
================
*/
void hhPhysics_AI::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			masterEntity = master;
			masterYaw = masterAxis[0].ToYaw();
			idAI *entAI = static_cast<idAI*>(self);
			if ( entAI ) {
				idAngles angles( 0, entAI->spawnArgs.GetFloat( "angle" ), 0 );
				localAxis = angles.ToMat3() * masterAxis.Transpose();
			}
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

/*
================
hhPhysics_AI::SetGravity
overridden to call SetClipModelAxis for asteroid gravity
================
*/
void hhPhysics_AI::SetGravity( const idVec3 &newGravity ) {
	idPhysics_Monster::SetGravity( newGravity );
	if ( bGravClipModelAxis ) {
		SetClipModelAxis();
	}
}

//================
//hhPhysics_AI::Save
//================
void hhPhysics_AI::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( flyStepDirection );
	lastMoveTouch.Save( savefile );
	savefile->WriteBool( useGravity );
	savefile->WriteMat3( localAxis );
	savefile->WriteBool( bGravClipModelAxis );
}

//================
//hhPhysics_AI::Restore
//================
void hhPhysics_AI::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( flyStepDirection );
	lastMoveTouch.Restore( savefile );
	savefile->ReadBool( useGravity );
	savefile->ReadMat3( localAxis );
	savefile->ReadBool( bGravClipModelAxis );
}

