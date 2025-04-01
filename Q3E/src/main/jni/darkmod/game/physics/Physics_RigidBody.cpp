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
#include "../Grabber.h"
#include "../MeleeWeapon.h" // grayman #3992

CLASS_DECLARATION( idPhysics_Base, idPhysics_RigidBody )
END_CLASS

const float STOP_SPEED = 50.0f; // grayman #3452 (was 10) - allow less movement at end to prevent excessive jiggling
const float OLD_STOP_SPEED = 10.0f; // grayman #3452 - still needed at this value for some of the math

#ifdef MOD_WATERPHYSICS
// if linearVelocity < WATER_STOP_LINEAR && angularVelocity < WATER_STOP_ANGULAR then set the RB to rest
// and we need  this->noMoveTime + NO_MOVE_TIME < gameLocal.getTime()
const idVec3 WATER_STOP_LINEAR(10.0f,10.0f,10.0f);
const idVec3 WATER_STOP_ANGULAR(500000.0f,500000.0f,500000.0f);
const int NO_MOVE_TIME          = 200;
static const float MAX_GRABBER_EXT_VELOCITY		= 120.0f;
static const float MAX_GRABBER_EXT_ANGVEL		= 5.0f;
#endif

#undef RB_TIMINGS

#ifdef RB_TIMINGS
static int lastTimerReset = 0;
static int numRigidBodies = 0;
static idTimer timer_total, timer_collision;
#endif

/*
================
RigidBodyDerivatives
================
*/
void RigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives ) {
	const idPhysics_RigidBody *p = (idPhysics_RigidBody *) clientData;
	rigidBodyIState_t *s = (rigidBodyIState_t *) state;
	// NOTE: this struct should conform to rigidBodyIState_t
	struct rigidBodyDerivatives_s {
		idVec3				linearVelocity;
		idMat3				angularMatrix;
		idVec3				force;
		idVec3				torque;
	} *d = (struct rigidBodyDerivatives_s *) derivatives;
	idVec3 angularVelocity;
	idMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = s->orientation * p->inverseInertiaTensor * s->orientation.Transpose();
	angularVelocity = inverseWorldInertiaTensor * s->angularMomentum;
	// derivatives
	d->linearVelocity = p->inverseMass * s->linearMomentum;
	d->angularMatrix = SkewSymmetric( angularVelocity ) * s->orientation;

#ifdef MOD_WATERPHYSICS
    // underwater we have a higher friction
    if( p->GetWaterLevelf() == 0.0f )
	{
        d->force = - p->linearFriction * s->linearMomentum + p->externalForce;
        d->torque = - p->angularFriction * s->angularMomentum + p->externalTorque;
    }
	else
	{
        // don't let water friction go less than 25% of the water viscosity
        float percent = Max(0.25f,p->GetSubmergedPercent(s->position,s->orientation.Transpose()));

        d->force = (-p->linearFriction * p->water->GetViscosity() * percent) * s->linearMomentum + p->externalForce;
        d->torque = (-p->angularFriction * p->water->GetViscosity()) * s->angularMomentum + p->externalTorque;
    }
#else
	d->force = - p->linearFriction * s->linearMomentum + p->externalForce;
	d->torque = - p->angularFriction * s->angularMomentum + p->externalTorque;
#endif
}

#ifdef MOD_WATERPHYSICS
/*
================
idPhysics_RigidBody::GetSubmergedPercent

  Approximates the percentage of the body that is submerged
================
*/
float idPhysics_RigidBody::GetSubmergedPercent( const idVec3 &pos, const idMat3 &rotation ) const
{
  idVec3 depth,bottom(pos);
  idBounds bounds = this->GetBounds();
  float height,d;

  if( this->water == NULL )
    return 0.0f;

  // offset and rotate the bounding box
  bounds += -centerOfMass;
  bounds *= rotation;

  // gets the position of the object relative to the surface of the water
  height = fabs(bounds[1] * gravityNormal * 2);

  // calculates the depth of the bottom of the object
  bottom += (height * 0.5f) * gravityNormal;
  depth = this->water->GetDepth(bottom);
  d = fabs(depth * gravityNormal);

  if( d > height ) {
    // the body is totally submerged
    return 1.0f;
  }
  else if( d <= 0 ) {
    return 0.0f;
  }
  else {
    // the body is partly submerged
    return d / height;
  }
}

/*
================
idPhysics_RigidBody::GetBuoyancy

  Gets buoyancy information for this RB
================
*/
bool idPhysics_RigidBody::GetBuoyancy( const idVec3 &pos, const idMat3 &rotation, idVec3 &bCenter, float &percent ) const
{
  // pos - position of the RB
  // rotation - axis for the RB
  // bCenter - after the function is called this is an approximation for the center of buoyancy
  // percent - rough percentage of the body that is under water
  //        used to calculate the volume of the submersed object (volume * percent) to give
  //        the body somewhat realistic bobbing.
  //
  // return true if the body is in water, false otherwise

  idVec3 tbCenter(pos);
  idBounds bounds = this->GetBounds();
  idTraceModel tm = *this->GetClipModel()->GetTraceModel();
  int i,count;

  percent = this->GetSubmergedPercent(pos,rotation);
  bCenter = pos;

  if( percent == 1.0f ) {
    // the body is totally submerged
    return true;
  }
  else {
    // the body is partly submerged (or not in the water)
    //
    // We do a rough approximation for center of buoyancy.
    // Normally this is done by calculating the volume of the submersed part of the body
    // so the center of buoyancy is the center of mass of the submersed volume.  This is
    // probably a slow computation so what I do is take an average of the submersed
    // vertices of the trace model.
    //
    // I was suprized when this first worked but you can use rb_showBuoyancy to see
    // what the approximation looks like.

    // set up clip model for approximation of center of buoyancy
    tm.Translate( -centerOfMass );
    tm.Rotate( rotation );
    tm.Translate( pos );

    // calculate which vertices are under water
    for( i = 0, count = 1; i < tm.numVerts; i++ ) {
      if( gameLocal.clip.Contents( tm.verts[ i ], NULL, this->GetAxis(), MASK_WATER, NULL ) ) {
        tbCenter += tm.verts[ i ];
        count += 1;
      }
    }

    if( count == 1 )
      bCenter = pos;
    else
      bCenter = tbCenter / count;
    return (count != 1);
  }
}
#endif // MOD_WATERPHYSICS

/*
================
idPhysics_RigidBody::Integrate

  Calculate next state from the current state using an integrator.
================
*/
void idPhysics_RigidBody::Integrate( float deltaTime, rigidBodyPState_t &next ) {
	idVec3 position;

	position = current.i.position;
	current.i.position += centerOfMass * current.i.orientation;

	current.i.orientation.TransposeSelf();

	integrator->Evaluate( (float *) &current.i, (float *) &next.i, 0, deltaTime );
	next.i.orientation.OrthoNormalizeSelf();

#ifdef MOD_WATERPHYSICS
	// apply a water gravity if the body is in water
	if ( this->SetWaterLevelf() != 0.0f )
	{
		idVec3 bCenter;
		idVec3 bForce(gravityVector),rForce(-gravityVector);
		float bMass,fraction,liquidMass;
		bool inWater;

		inWater = this->GetBuoyancy(next.i.position,next.i.orientation.Transpose(),bCenter,fraction);

		// calculate water mass
		liquidMass = this->volume * this->water->GetDensity() * fraction;
		// don't let liquid mass get too high
		liquidMass = Min( liquidMass, 3 * this->mass );

		bMass = this->mass - liquidMass;

		// calculate buoyancy force
		bForce *= deltaTime * bMass;
		rForce *= deltaTime * liquidMass;

		// apply water force
		// basically here we do a ::ApplyImpulse() but we apply it to next, not current
		next.i.linearMomentum += bForce;
		next.i.angularMomentum += (bCenter - next.i.position).Cross(rForce);

		// take the body out of water if it's not in water.
		if( !inWater ) this->SetWater(NULL, 0.0f);
	}
	else
	{
		next.i.linearMomentum += deltaTime * gravityVector * mass; // apply normal gravity
	}
#else
	next.i.linearMomentum += deltaTime * gravityVector * mass; // apply normal gravity
#endif

	current.i.orientation.TransposeSelf();
	next.i.orientation.TransposeSelf();

	current.i.position = position;
	next.i.position -= centerOfMass * next.i.orientation;

	next.atRest = current.atRest;
}

bool idPhysics_RigidBody::PropagateImpulse(const int id, const idVec3& point, const idVec3& impulse)
{
	DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Contacts with this entity %s = %d\r", self->name.c_str(), contacts.Num());

	if (propagateImpulseLock || impulse.LengthSqr() < 1e-5)
	{
		// greebo: Don't process incoming small impulses, quit at once.
		return false;
	}

	// Don't accept other propagations during processing
	propagateImpulseLock = true;

	// greebo: Check all entities touching this physics object
	EvaluateContacts();

	/**
	 * greebo: The incoming impulse goes through 3 stages:
	 * 
	 * 1) The contact friction of this object reduces the impulse
	 * 2) The reduced impulse is distributed among the contact entities
	 * 3) The remaining impulse is applied to self.
	 */
	
	// Save the current state
	rigidBodyPState_t savedState = current;

	// Apply the impulse to the current state, as if the object was resting
	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();
	ApplyImpulse(0, point, impulse);

	//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Linear Momentum before friction: [%s]\r", current.i.linearMomentum.ToString());
	//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Angular Momentum before friction: [%s]\r", current.i.angularMomentum.ToString());

	// Calculate the friction using this state
	//ContactFriction(current.lastTimeStep);

	// greebo: Disabled contact friction for now
	//current.i.linearMomentum *= 1.0f;
	//current.i.angularMomentum *= 1.0f;

	//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Linear Momentum after friction: [%s]", current.i.linearMomentum.ToString());
	//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Angular Momentum after friction: [%s]", current.i.angularMomentum.ToString());

	// The list of all the touching entities
	idList<contactInfo_t> touching;

	// greebo: FIXME: A possible optimisation would be to store the contact indices instead of copying the entire struct
	
	//gameRenderWorld->DebugArrow(colorGreen, point, point + impulseN*10, 1, 5000);

	// greebo: Check for any contact normals that are suitable for this impulse direction
	//         Contact normals with angles larger than pi/2 are discarded.
	for (int i = 0; i < contacts.Num(); i++)
	{
		if (contacts[i].entityNum == ENTITYNUM_WORLD)
		{
			continue;
		}

		idEntity* contactEntity = gameLocal.entities[contacts[i].entityNum];

		//gameRenderWorld->DebugArrow(colorBlue, contacts[i].point, contacts[i].point + contacts[i].normal*20, 1, 5000);
		
		if (contactEntity == NULL)
		{
			continue;
		}

#if 0 // grayman #3001 - keep in case there are future problems with this fix
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::contact entity %d has a contact normal of (%s)\r",i,contacts[i].normal.ToString(6));
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::the impulse is (%s) \r",impulse.ToString(6));
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::the calculation result is %f \r",impulse * -contacts[i].normal);

		float a = impulse.x * -contacts[i].normal.x;
		float b = impulse.y * -contacts[i].normal.y;
		float c = impulse.z * -contacts[i].normal.z;
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::normal.x = %f \r",contacts[i].normal.x);
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::normal.y = %f \r",contacts[i].normal.y);
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::normal.z = %f \r",contacts[i].normal.z);
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::a = %f \r",a);
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::b = %f \r",b);
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("idPhysics_RigidBody::c = %f \r",c);
#endif
		if ((impulse * -contacts[i].normal) <= 0.0f) // grayman #3001 - should have been <=, not <
		{
			DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Entity %s is not in push direction.\r", contactEntity->name.c_str());
			continue;
		}
		
		// Add the contact info to the list, it is significant
		touching.Append(contacts[i]);
	}

	int numTouching = touching.Num();
	DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Contacts with this entity %s without world = %d\r", self->name.c_str(), numTouching);

	if (numTouching > 0)
	{
#if 0 // grayman #3001 - old way (keep for reference, in case there are unwanted side-effects from this fix
		// Distribute the impulse evenly across the touching entities
		idVec3 impulseFraction(current.i.linearMomentum / touching.Num());
		float impulseFractionLen(impulseFraction.LengthFast());

		// Now apply the impulse to the touching entities
		for (int i = 0; i < numTouching; i++)
		{
			idEntity* pushed = gameLocal.entities[touching[i].entityNum];

			if (pushed == NULL)
				continue;

			DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Propagating impulse to entity %s\r", pushed->name.c_str());
			//gameRenderWorld->DebugArrow(colorRed, touching[i].point, touching[i].point - touching[i].normal*10, 1, 1000);

			pushed->GetPhysics()->PropagateImpulse(id, touching[i].point, -touching[i].normal * impulseFractionLen);

			// Substract this propagated impulse from the remaining one
			current.i.linearMomentum -= impulseFraction;
		}

#else // new way

		// Distribute the impulse evenly across the touching entities

		// The impulse needs to be applied across the touching entities, not
		// by their count, but by how much of the impulse they will absorb,
		// based on the relationship between the impulse and each touching
		// entity's contact normal.

		idVec3 impulseFraction;
		idVec3 momentum = current.i.linearMomentum; // capture the original impulse amount

		// Now apply a fractional impulse to the touching entities
		for ( int i = 0 ; i < numTouching ; i++ )
		{
			idEntity* pushed = gameLocal.entities[touching[i].entityNum];

			if (pushed == NULL)
				continue;
			
			idVec3 contactNormal = touching[i].normal;

			impulseFraction.x = momentum.x * -contactNormal.x;
			impulseFraction.y = momentum.y * -contactNormal.y;
			impulseFraction.z = momentum.z * -contactNormal.z;
			
			//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Propagating impulse to entity %s\r", pushed->name.c_str());
			//gameRenderWorld->DebugArrow(colorRed, touching[i].point, touching[i].point - touching[i].normal*10, 1, 1000);

			pushed->GetPhysics()->PropagateImpulse(id, touching[i].point, impulseFraction);

			// Substract this propagated impulse from the remaining one
			current.i.linearMomentum -= impulseFraction;
		}
#endif
	}
	
	// Save the remaining impulse before reverting the physics state
	idVec3 remainingImpulse(current.i.linearMomentum);

	// Revert the state to as it was before
	current = savedState;

	// Apply the remaining impulse to this object
	ApplyImpulse(0, point, remainingImpulse);

	DM_LOG(LC_ENTITY, LT_INFO)LOGVECTOR("Linear Momentum after applyImpulse:", current.i.linearMomentum);
	DM_LOG(LC_ENTITY, LT_INFO)LOGVECTOR("Angular Momentum after applyImpulse:", current.i.angularMomentum);

	propagateImpulseLock = false;

	// Return TRUE if we pushed any neighbours, FALSE if this was a single pushed object
	return (numTouching > 0);
}

/*
================
idPhysics_RigidBody::CollisionImpulse

  Calculates the collision impulse using the velocity relative to the collision object.
  The current state should be set to the moment of impact.
================
*/
bool idPhysics_RigidBody::CollisionImpulse( const trace_t &collision, idVec3 &impulse ) 
{
	idVec3 r, linearVelocity, angularVelocity, velocity;
	idMat3 inverseWorldInertiaTensor;
	float impulseNumerator, impulseDenominator, vel;
	impactInfo_t info;
	idEntity *ent;

	// get info from other entity involved
	ent = gameLocal.entities[collision.c.entityNum];
	ent->GetImpactInfo( self, collision.c.id, collision.c.point, &info );

	// Check if we are grabbed by the grabber, and set collision var if so
	if ( self == gameLocal.m_Grabber->GetSelected() )
	{
		// greebo: Don't collide grabbed entities with its own bindslaves
		if ( (ent->GetBindMaster() == NULL || self != ent->GetBindMaster())
			&& ent != gameLocal.GetLocalPlayer() )
		{
			gameLocal.m_Grabber->m_bIsColliding = true;
			gameLocal.m_Grabber->m_CollNorms.AddUnique( collision.c.normal );
		}
	}

	// Update moved by and set in motion by actor
	if( self->m_SetInMotionByActor.GetEntity() != NULL )
	{
		ent->m_SetInMotionByActor = self->m_SetInMotionByActor.GetEntity();
		ent->m_MovedByActor = self->m_MovedByActor.GetEntity();
	}
	// Note: Actors should not overwrite the moved by other actors when they are hit with something
	// So only overwrite if MovedByActor is NULL
	if ( ent->IsType(idActor::Type) 
		&& self->m_SetInMotionByActor.GetEntity() == NULL
		&& !(static_cast<idActor *>(ent)->IsKnockedOut() || ent->health < 0) )
	{
		self->m_SetInMotionByActor = (idActor *) ent;
		self->m_MovedByActor = (idActor *) ent;
	}

	// collision point relative to the body center of mass
	r = collision.c.point - ( current.i.position + centerOfMass * current.i.orientation );
	// the velocity at the collision point
	linearVelocity = inverseMass * current.i.linearMomentum;
	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
	velocity = linearVelocity + angularVelocity.Cross(r);
	// subtract velocity of other entity
	velocity -= info.velocity;

	// velocity in normal direction
	vel = velocity * collision.c.normal;

	if ( vel > -OLD_STOP_SPEED ) // grayman #3452 - was STOP_SPEED
	{
		impulseNumerator = OLD_STOP_SPEED;
	}
	else
	{
		impulseNumerator = -( 1.0f + bouncyness ) * vel;
	}
	impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( collision.c.normal ) ).Cross( r ) * collision.c.normal );
	if ( info.invMass ) {
		impulseDenominator += info.invMass + ( ( info.invInertiaTensor * info.position.Cross( collision.c.normal ) ).Cross( info.position ) * collision.c.normal );
	}
	impulse = (impulseNumerator / impulseDenominator) * collision.c.normal;

//#define DEBUG_COLLISIONS
#ifdef DEBUG_COLLISIONS
	idVec3 velocityN(GetLinearVelocity());
	velocityN.NormalizeFast();

	idVec3 velocityA(GetAngularVelocity());
	velocityA.NormalizeFast();

	idVec3 impulseN(impulse);
	impulseN.NormalizeFast();

	idVec3 impulseA(r.Cross(impulse));
	impulseA.NormalizeFast();

	idVec3 origin = current.i.position + centerOfMass * current.i.orientation;

	gameRenderWorld->DebugArrow(colorRed, origin, origin + velocityN*10, 1, 15);
	gameRenderWorld->DebugArrow(colorBlue, origin, origin + velocityA*10, 1, 15);

	gameRenderWorld->DebugArrow(colorMagenta, idVec3(1,0,0) + origin + velocityN*10, origin + velocityN*10 + impulseN*10, 1, 15);
	gameRenderWorld->DebugArrow(colorGreen, idVec3(1,0,0) + origin + velocityA*10, origin + velocityA*10 + impulseA*10, 1, 15);

	gameRenderWorld->DebugArrow(colorMdGrey, collision.c.point, collision.c.point + collision.c.normal*10, 1, 15);
	//gameRenderWorld->DebugText(ent->name.c_str(), collision.c.point, 1, colorMdGrey, current.i.orientation, 1, 500);

#endif

	// update linear and angular momentum with impulse
	current.i.linearMomentum += impulse;
	current.i.angularMomentum += r.Cross(impulse);

	//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Collision fraction of %s = %f\r", self->name.c_str(), collision.fraction);

	// if no movement at all don't blow up
	if ( collision.fraction < 0.0001f ) {
		current.i.linearMomentum *= 0.5f;
		current.i.angularMomentum *= 0.5f;
	}

	// callback to self to let the entity know about the collision
	return self->Collide( collision, velocity );
}

/*
================
idPhysics_RigidBody::CheckForCollisions

  Check for collisions between the current and next state.
  If there is a collision the next state is set to the state at the moment of impact.
================
*/
bool idPhysics_RigidBody::CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision ) {
//#define TEST_COLLISION_DETECTION
	idMat3 axis;
	idRotation rotation;
#ifdef MOD_WATERPHYSICS
	idVec3 pos;
	trace_t waterCollision;
#endif
	bool collided = false;

#ifdef TEST_COLLISION_DETECTION
	bool startsolid;
	if ( gameLocal.clip.Contents( current.i.position, clipModel, current.i.orientation, clipMask, self ) ) {
		startsolid = true;
	}
#endif

	TransposeMultiply( current.i.orientation, next.i.orientation, axis );
	rotation = axis.ToRotation();
	rotation.SetOrigin( current.i.position );

	// if there was a collision
#ifdef MOD_WATERPHYSICS
	pos = next.i.position;
#endif

	if ( gameLocal.clip.Motion( collision, current.i.position, next.i.position, rotation, clipModel, current.i.orientation, clipMask, self ) ) {

		// set the next state to the state at the moment of impact
		next.i.position = collision.endpos;
		next.i.orientation = collision.endAxis;
		next.i.linearMomentum = current.i.linearMomentum;
		next.i.angularMomentum = current.i.angularMomentum;
		collided = true;
	}

#ifdef MOD_WATERPHYSICS
	// Check for water collision
	// ideally we could do this check in one step but if a body moves quickly in shallow water
	// they will occasionally clip through a solid entity (ie. fall through the floor)
	if ( gameLocal.clip.Motion( waterCollision, current.i.position, pos, rotation, clipModel, current.i.orientation, MASK_WATER, self ) )
	{
		idEntity *ent = gameLocal.entities[waterCollision.c.entityNum];

		// make sure the object didn't collide with something before hitting the water (we don't splash for that case)
		if( !collided || ( waterCollision.fraction < collision.fraction ) )
		{
			// if the object collides with something with a physics_liquid
			if ( ent->GetPhysics()->IsType( idPhysics_Liquid::Type ) )
			{
				idPhysics_Liquid *liquid = static_cast<idPhysics_Liquid *>(ent->GetPhysics());
				impactInfo_t info;

				self->GetImpactInfo(ent,waterCollision.c.id,waterCollision.c.point,&info);

				// apply water splash friction
				if ( this->water == NULL )
				{
					idVec3 impulse = -info.velocity * this->volume * liquid->GetDensity() * 0.25f;
					impulse = (impulse * gravityNormal) * gravityNormal;

					if ( next.i.linearMomentum.LengthSqr() < impulse.LengthSqr() )
					{
						// cancel falling, maintain sideways movement (lateral?)
						next.i.linearMomentum -= (next.i.linearMomentum * gravityNormal) * gravityNormal;
					}
					else
					{
						next.i.angularMomentum += ( waterCollision.c.point - ( next.i.position + centerOfMass * next.i.orientation ) ).Cross( impulse );
						next.i.linearMomentum += impulse * 0.5f;
					}
				}

				this->SetWater(liquid, ent->spawnArgs.GetFloat("murkiness", "0"));
				this->water->Splash(this->self,this->volume,info,waterCollision);

				// grayman #1104 - we collided with water. should we detonate?

				if ( this->self->IsType(idProjectile::Type) )
				{
					idProjectile* projectile = static_cast<idProjectile*>(this->self);
					if ( projectile->DetonateOnWater() )
					{
						// Detonation doesn't occur here, but we have to tell the calling routines
						// that we impacted something, and that will lead to detonation.

						// Copy the collision data from waterCollision to collision
						// so the calling routines can use it.

						collision.fraction = waterCollision.fraction;	// fraction of movement completed, 1.0 = didn't hit anything
						collision.endpos = waterCollision.endpos;		// final position of trace model
						collision.endAxis = waterCollision.endAxis;		// final axis of trace model
						collision.c = waterCollision.c;					// contact information, only valid if fraction < 1.0

						// set the next state to the state at the moment of impact
						next.i.position = waterCollision.endpos;
						next.i.orientation = waterCollision.endAxis;
						next.i.linearMomentum = current.i.linearMomentum;
						next.i.angularMomentum = current.i.angularMomentum;
						collided = true;

						// Do we apply any splash damage?
						if ( projectile->spawnArgs.GetBool("no_water_splash_damage", "0") )
						{
							projectile->SetNoSplashDamage(true);
						}
					}
				}
			}
		}
	}
#endif

#ifdef TEST_COLLISION_DETECTION
	if ( gameLocal.clip.Contents( next.i.position, clipModel, next.i.orientation, clipMask, self ) ) {
		if ( !startsolid ) {
			int bah = 1;
		}
	}
#endif
	return collided;
}

/*
================
idPhysics_RigidBody::ContactFriction

  Does not solve friction for multiple simultaneous contacts but applies contact friction in isolation.
  Uses absolute velocity at the contact points instead of the velocity relative to the contact object.
================
*/

// grayman #3452 - Average the linear and angular momentums
// across contact points that share the same normal. Otherwise, there's the possibility of building
// up tremendous momentum that will cause an object to bounce way more than it should. Also, don't
// add the momentums to the overall object momentum as each point is considered. All points should be
// considered simultaneously, then averaged to provide the correct resultant momentums. This is what
// was causing falling planks to fall in slow motion: too much friction acting against gravity.
void idPhysics_RigidBody::ContactFriction( float deltaTime )
{
	float magnitude, impulseNumerator, impulseDenominator;
	idMat3 inverseWorldInertiaTensor;
	idVec3 linearVelocity, angularVelocity;
	idVec3 massCenter, r, velocity, normal, impulse, normalVelocity;

	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;

	massCenter = current.i.position + centerOfMass * current.i.orientation;

	linearVelocity = inverseMass * current.i.linearMomentum;
	angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;

	// Keep lists of contact point normals and calculated momentums. Average the
	// momentums across all points that have the same normal.

	idList<idVec3> normals; // list of different contact point normals
	idList<idVec3> lm; // list of summed linear momentum for each set of normals
	idList<idVec3> am; // list of summed angular momentum for each set of normals
	idList<int> normalCount; // list of the number of contributing points for each normal set

	for ( int i = 0 ; i < contacts.Num() ; i++ )
	{
		idVec3 contactNormal = contacts[i].normal;
		r = contacts[i].point - massCenter;

		// fudge factor - Needed to reduce the momentum drag on
		// plank-shaped objects, whose center of mass can be far from its
		// edges, w/o affecting more compact objects like pots and crates and apples,
		// where the center of mass is closer to the edges.
		float rLength = r.Length();
		if ( rLength < 1.0f )
		{
			rLength = 1.0f;
		}
		float ff = rLength/4.0f; // fudge factor

		// calculate velocity at contact point
		velocity = linearVelocity + angularVelocity.Cross(r); // angularVelocity.Cross(r) is torque? (http://iweb.tntech.edu/murdock/books/v2chap2.pdf)

		// velocity along normal vector
		normalVelocity = ( velocity * contactNormal ) * contactNormal;

		// calculate friction impulse
		idVec3 planeVelocity = -( velocity - normalVelocity ); // velocity on the plane the contact point lies in
		magnitude = planeVelocity.Normalize();
		impulseNumerator = contactFriction * magnitude;
		impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( planeVelocity ) ).Cross( r ) * planeVelocity );
		impulse = (impulseNumerator / impulseDenominator) * planeVelocity;

		impulse /= ff; // fudge factor

		// Search for the index of a normal set that's close to this point's normal
		int matchingIndex = -1;
		for ( int j = 0 ; j < normals.Num() ; j++)
		{
			if ( contactNormal.Compare(normals[j],0.01f) ) // Allow [-0 -0 1] to match [0 0 1]
			{
				matchingIndex = j;
				break;
			}
		}

		if ( matchingIndex == -1) // if no match
		{
			// this is the first impulse calculation for this normal
			normals.Append(contactNormal);
			matchingIndex = normals.Num() - 1;
			lm.Append(impulse);
			am.Append(r.Cross(impulse));
			normalCount.Append(1);
		}
		else // if there was a match
		{
			// this is another impulse for an existing normal
			lm[matchingIndex] += impulse;
			am[matchingIndex] += r.Cross(impulse);
			normalCount[matchingIndex]++;
		}

		// if moving towards the surface at the contact point <- original comment
		// grayman - It appears that the following determines the amount of force (impulse) applied to the rigid body
		// by the plane the contact point lies in. When there are multiple contact points, we get multiple
		// forces. These can mount up dramatically, so they need to be damped. Otherwise, dropping a crate
		// on a flat horizontal surface could have upwards of 10 contact points and cause the crate to bounce
		// up to the ceiling. We can't delete this section, because it helps to keep objects from bouncing
		// around when they're trying to settle at the end of a move.
		if ( normalVelocity * contactNormal < 0.0f )
		{
			// calculate impulse
			normal = -normalVelocity; // the component of velocity that pushes on the rb
			impulseNumerator = normal.Normalize();
			impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( normal ) ).Cross( r ) * normal );
			impulse = (impulseNumerator / impulseDenominator) * normal;

			impulse /= ff; // fudge factor

			// this is another impulse for an existing normal
			lm[matchingIndex] += impulse;
			am[matchingIndex] += r.Cross(impulse);
		}
	}

	// To calculate the final momentums, average the momentums for each different
	// normal, then add them together.

	idVec3 linearMomentumFromFriction(0,0,0);
	idVec3 angularMomentumFromFriction(0,0,0);
	for ( int i = 0 ; i < normals.Num() ; i++)
	{
		linearMomentumFromFriction += lm[i]/(float)normalCount[i];
		angularMomentumFromFriction += am[i]/(float)normalCount[i];
	}

	current.i.linearMomentum += linearMomentumFromFriction;
	current.i.angularMomentum += angularMomentumFromFriction;
}

/*
================
idPhysics_RigidBody::SmallMassContactFriction

  grayman #3452 - Objects with small mass have a problem with the new ContactFriction()
  method above. As a temporary workaround, provide the previous version
  for objects with small mass
================
*/
void idPhysics_RigidBody::SmallMassContactFriction( float deltaTime ) {
	int i;
	float magnitude, impulseNumerator, impulseDenominator;
	idMat3 inverseWorldInertiaTensor;
	idVec3 linearVelocity, angularVelocity;
	idVec3 massCenter, r, velocity, normal, impulse, normalVelocity;

	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;

	massCenter = current.i.position + centerOfMass * current.i.orientation;

	for ( i = 0; i < contacts.Num(); i++ ) {

		r = contacts[i].point - massCenter;

		// calculate velocity at contact point
		linearVelocity = inverseMass * current.i.linearMomentum;
		angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
		velocity = linearVelocity + angularVelocity.Cross(r);

		// velocity along normal vector
		normalVelocity = ( velocity * contacts[i].normal ) * contacts[i].normal;

		// calculate friction impulse
		normal = -( velocity - normalVelocity );
		magnitude = normal.Normalize();
		impulseNumerator = contactFriction * magnitude;
		impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( normal ) ).Cross( r ) * normal );
		impulse = (impulseNumerator / impulseDenominator) * normal;

		// apply friction impulse
		current.i.linearMomentum += impulse;
		current.i.angularMomentum += r.Cross(impulse);

		// if moving towards the surface at the contact point
		if ( normalVelocity * contacts[i].normal < 0.0f ) {
			// calculate impulse
			normal = -normalVelocity;
			impulseNumerator = normal.Normalize();
			impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( normal ) ).Cross( r ) * normal );
			impulse = (impulseNumerator / impulseDenominator) * normal;

			// apply impulse
			current.i.linearMomentum += impulse;
			current.i.angularMomentum += r.Cross( impulse );
		}
	}
}

/*
================
idPhysics_RigidBody::TestIfAtRest

  Returns true if the body is considered at rest.
  Does not catch all cases where the body is at rest but is generally good enough.
================
*/
#ifdef MOD_WATERPHYSICS
bool idPhysics_RigidBody::TestIfAtRest( void ) {
#else
bool idPhysics_RigidBody::TestIfAtRest( void ) const {
#endif	
	int i;
	float gv;
	idVec3 v, av, normal, point;
	idMat3 inverseWorldInertiaTensor;
	idFixedWinding contactWinding;

	if ( current.atRest >= 0 )
	{
		return true;
	}
#ifdef MOD_WATERPHYSICS
    // do some special checks if the body is in water
    if ( this->water != NULL )
	{
        if ( this->current.i.linearMomentum.LengthSqr() < WATER_STOP_LINEAR.LengthSqr() &&
            this->current.i.angularMomentum.LengthSqr() < WATER_STOP_ANGULAR.LengthSqr() )
		{

            if ( this->noMoveTime == 0 )
			{
                this->noMoveTime = gameLocal.GetTime();
            }
			else if ( this->noMoveTime+NO_MOVE_TIME < gameLocal.GetTime() )
			{
                this->noMoveTime = 0;
                return true;
            }
        }
		else
		{
            this->noMoveTime = 0;
        }
    }
#endif

	// need at least 3 contact points to come to rest
	if ( contacts.Num() < 3 )
	{
		return false;
	}

	// get average contact plane normal
	// grayman - might be a bug here, because if the object slides into a wall,
	// one or more contacts will be on the floor, and one or more will be on
	// the wall. Averaging these makes no sense, and could cause the object to
	// think it's on an incline when it isn't. It should only average the normals
	// that are close to the gravity vector, and ignore everything else.
	// There are no reported issues with it, however, so I'm leaving it as is. 

	normal.Zero();
	for ( i = 0 ; i < contacts.Num() ; i++ )
	{
		normal += contacts[i].normal;
	}
	normal /= (float) contacts.Num();
	normal.Normalize();

	// if on a too steep surface
	if ( (normal * gravityNormal) > -0.7f )
	{
		return false;
	}

	// create bounds for contact points
	contactWinding.Clear();
	for ( i = 0 ; i < contacts.Num() ; i++ )
	{
		// project point onto plane through origin orthogonal to the gravity
		point = contacts[i].point - (contacts[i].point * gravityNormal) * gravityNormal;
		contactWinding.AddToConvexHull( point, gravityNormal );
	}

	// need at least 3 contact points to come to rest
	if ( contactWinding.GetNumPoints() < 3 )
	{
		return false;
	}

	// center of mass in world space
	point = current.i.position + centerOfMass * current.i.orientation;
	point -= (point * gravityNormal) * gravityNormal;

	// if the point is not inside the winding
	if ( !contactWinding.PointInside( gravityNormal, point, 0 ) )
	{
		return false;
	}

	// linear velocity of body
	v = inverseMass * current.i.linearMomentum;
	// linear velocity in gravity direction
	gv = v * gravityNormal;
	// linear velocity orthogonal to gravity direction
	v -= gv * gravityNormal;

	// if too much velocity orthogonal to gravity direction
	if ( v.Length() > STOP_SPEED )
	{
		return false;
	}
	// if too much velocity in gravity direction
	if ( ( gv > 2.0f * STOP_SPEED ) || ( gv < -2.0f * STOP_SPEED ) )
	{
		return false;
	}

	// calculate rotational velocity
	inverseWorldInertiaTensor = current.i.orientation * inverseInertiaTensor * current.i.orientation.Transpose();
	av = inverseWorldInertiaTensor * current.i.angularMomentum;

	// if too much rotational velocity
	if ( av.LengthSqr() > STOP_SPEED )
	{
		return false;
	}

	return true;
}

/*
================
idPhysics_RigidBody::DropToFloorAndRest

  Drops the object straight down to the floor and verifies if the object is at rest on the floor.
================
*/
void idPhysics_RigidBody::DropToFloorAndRest( void ) {
	idVec3 down;
	trace_t tr;

	if ( testSolid ) {

		testSolid = false;

		if ( gameLocal.clip.Contents( current.i.position, clipModel, current.i.orientation, clipMask, self ) ) {
			gameLocal.DWarning( "rigid body in solid for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
			Rest();
			dropToFloor = false;
			return;
		}
	}

	// put the body on the floor
	down = current.i.position + gravityNormal * 128.0f;
	gameLocal.clip.Translation( tr, current.i.position, down, clipModel, current.i.orientation, clipMask, self );
	current.i.position = tr.endpos;
	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), tr.endpos, current.i.orientation );

	// if on the floor already
	if ( tr.fraction == 0.0f ) {
		// test if we are really at rest
		EvaluateContacts();
		if ( !TestIfAtRest() ) {
			gameLocal.DWarning( "rigid body not at rest for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		}
		Rest();
		dropToFloor = false;
	} else if ( IsOutsideWorld() ) {
		gameLocal.Warning( "rigid body outside world bounds for entity '%s' type '%s' at (%s)",
							self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		Rest();
		dropToFloor = false;
	}
}

/*
================
idPhysics_RigidBody::DebugDraw
================
*/
void idPhysics_RigidBody::DebugDraw( void ) {

	if ( rb_showBodies.GetBool() || ( rb_showActive.GetBool() && current.atRest < 0 ) ) {
		collisionModelManager->DrawModel( clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis(), vec3_origin, 0.0f );
	}

	if ( rb_showMass.GetBool() ) {
#ifdef MOD_WATERPHYSICS
        if( this->water != NULL )  {
            idVec3 pos;
            float  percent, liquidMass;

            pos = this->current.i.position + this->centerOfMass*this->current.i.orientation;
            percent = this->GetSubmergedPercent(pos,this->current.i.orientation.Transpose());

            liquidMass = this->mass - ( this->volume * this->water->GetDensity() * percent );

            gameRenderWorld->DebugText( va( "\n%1.2f", liquidMass), current.i.position, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
        }
        else
#endif
            gameRenderWorld->DebugText( va( "\n%1.2f", mass ), current.i.position, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}

	if ( rb_showInertia.GetBool() ) {
		idMat3 &I = inertiaTensor;
		gameRenderWorld->DebugText( va( "\n\n\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )",
									I[0].x, I[0].y, I[0].z,
									I[1].x, I[1].y, I[1].z,
									I[2].x, I[2].y, I[2].z ),
									current.i.position, 0.05f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}

	if ( rb_showVelocity.GetBool() ) {
		DrawVelocity( clipModel->GetId(), 0.1f, 4.0f );
	}

#ifdef MOD_WATERPHYSICS
    if( rb_showBuoyancy.GetBool() && this->water != NULL ) {
        idVec3 pos;
        idVec3 bCenter;
        float  percent;

        pos = this->current.i.position + this->centerOfMass*this->current.i.orientation;
        this->GetBuoyancy(pos,this->current.i.orientation.Transpose(),bCenter,percent);

        gameRenderWorld->DebugArrow(colorGreen,pos,bCenter,1);
        gameRenderWorld->DebugText( va( "%1.2f",percent), pos, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
    }
#endif
}

/*
================
idPhysics_RigidBody::idPhysics_RigidBody
================
*/
idPhysics_RigidBody::idPhysics_RigidBody( void ) {

	// set default rigid body properties
	SetClipMask( MASK_SOLID );
	SetBouncyness( 0.6f );
	SetFriction( 0.6f, 0.6f, 0.0f );
	clipModel = NULL;

	memset( &current, 0, sizeof( current ) );

	current.atRest = -1;
	current.lastTimeStep = USERCMD_MSEC;

	current.i.position.Zero();
	current.i.orientation.Identity();

	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();

	saved = current;

	mass = 1.0f;
	inverseMass = 1.0f;
	centerOfMass.Zero();
	inertiaTensor.Identity();
	inverseInertiaTensor.Identity();
#ifdef MOD_WATERPHYSICS
	this->water = NULL;
	this->m_fWaterMurkiness = 0.0f;
	this->volume = -FLT_MAX;	//to be set
#endif

	// use the least expensive euler integrator
	integrator = new idODE_Euler( sizeof(rigidBodyIState_t) / sizeof(float), RigidBodyDerivatives, this );

	dropToFloor = false;
	noImpact = false;
	noContact = false;
	testSolid = false;

	hasMaster = false;
	isOrientated = false;

#ifdef MOD_WATERPHYSICS
	this->noMoveTime = 0;
#endif
#ifdef RB_TIMINGS
	lastTimerReset = 0;
#endif

	isBlocked = false;
	propagateImpulseLock = false;

	memset(&collisionTrace, 0, sizeof(collisionTrace));

	// tels
	maxForce.Zero();
	maxTorque.Zero();

	externalForce.Zero();
	externalTorque.Zero();
	externalForcePoint.Zero();
}

/*
================
idPhysics_RigidBody::~idPhysics_RigidBody
================
*/
idPhysics_RigidBody::~idPhysics_RigidBody( void ) {
	if ( clipModel ) {
		delete clipModel;
		clipModel = NULL;
	}
	delete integrator;
}

/*
================
idPhysics_RigidBody_SavePState
================
*/
void idPhysics_RigidBody_SavePState( idSaveGame *savefile, const rigidBodyPState_t &state ) {
	savefile->WriteInt( state.atRest );
	savefile->WriteFloat( state.lastTimeStep );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteMat3( state.localAxis );
	savefile->Write( &state.pushVelocity, sizeof( state.pushVelocity ) );
	state.forceApplications.Save( savefile );

	savefile->WriteVec3( state.i.position );
	savefile->WriteMat3( state.i.orientation );
	savefile->WriteVec3( state.i.linearMomentum );
	savefile->WriteVec3( state.i.angularMomentum );
}

/*
================
idPhysics_RigidBody_RestorePState
================
*/
void idPhysics_RigidBody_RestorePState( idRestoreGame *savefile, rigidBodyPState_t &state ) {
	savefile->ReadInt( state.atRest );
	savefile->ReadFloat( state.lastTimeStep );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadMat3( state.localAxis );
	savefile->Read( &state.pushVelocity, sizeof( state.pushVelocity ) );
	state.forceApplications.Restore( savefile );

	savefile->ReadVec3( state.i.position );
	savefile->ReadMat3( state.i.orientation );
	savefile->ReadVec3( state.i.linearMomentum );
	savefile->ReadVec3( state.i.angularMomentum );
}

/*
================
idPhysics_RigidBody::Save
================
*/
void idPhysics_RigidBody::Save( idSaveGame *savefile ) const {

	idPhysics_RigidBody_SavePState( savefile, current );
	idPhysics_RigidBody_SavePState( savefile, saved );

	savefile->WriteFloat( linearFriction );
	savefile->WriteFloat( angularFriction );
	savefile->WriteFloat( contactFriction );
	savefile->WriteFloat( bouncyness );
	savefile->WriteClipModel( clipModel );

	savefile->WriteFloat( mass );
#ifdef MOD_WATERPHYSICS
    savefile->WriteFloat( volume );
#endif
	savefile->WriteFloat( inverseMass );
	savefile->WriteVec3( centerOfMass );
	savefile->WriteMat3( inertiaTensor );
	savefile->WriteMat3( inverseInertiaTensor );

	savefile->WriteBool( dropToFloor );
	savefile->WriteBool( testSolid );
	savefile->WriteBool( noImpact );
	savefile->WriteBool( noContact );

	savefile->WriteBool( hasMaster );
	savefile->WriteBool( isOrientated );

	savefile->WriteBool(isBlocked);
	// greebo: Note this is not saved (yet). Uncomment this after release 1.00
	//savefile->WriteBool(propagateImpulseLock);
	savefile->WriteTrace(collisionTrace);

	// tels
	savefile->WriteVec3( maxForce );
	savefile->WriteVec3( maxTorque );

	savefile->WriteVec3( externalForce );
	savefile->WriteVec3( externalTorque );
	savefile->WriteVec3( externalForcePoint );
}

/*
================
idPhysics_RigidBody::Restore
================
*/
void idPhysics_RigidBody::Restore( idRestoreGame *savefile ) {

	idPhysics_RigidBody_RestorePState( savefile, current );
	idPhysics_RigidBody_RestorePState( savefile, saved );

	savefile->ReadFloat( linearFriction );
	savefile->ReadFloat( angularFriction );
	savefile->ReadFloat( contactFriction );
	savefile->ReadFloat( bouncyness );
	savefile->ReadClipModel( clipModel );

	savefile->ReadFloat( mass );
#ifdef MOD_WATERPHYSICS
    savefile->ReadFloat( volume );
#endif
	savefile->ReadFloat( inverseMass );
	savefile->ReadVec3( centerOfMass );
	savefile->ReadMat3( inertiaTensor );
	savefile->ReadMat3( inverseInertiaTensor );

	savefile->ReadBool( dropToFloor );
	savefile->ReadBool( testSolid );
	savefile->ReadBool( noImpact );
	savefile->ReadBool( noContact );

	savefile->ReadBool( hasMaster );
	savefile->ReadBool( isOrientated );

	savefile->ReadBool(isBlocked);
	// greebo: Note this is not saved (yet). Uncomment this after release 1.00
	//savefile->ReadBool(propagateImpulseLock);
	savefile->ReadTrace(collisionTrace);

	// tels
	savefile->ReadVec3( maxForce );
	savefile->ReadVec3( maxTorque );

	savefile->ReadVec3( externalForce );
	savefile->ReadVec3( externalTorque );
	savefile->ReadVec3( externalForcePoint );
}

/*
================
idPhysics_RigidBody::SetClipModel
================
*/
#define MAX_INERTIA_SCALE		10.0f

void idPhysics_RigidBody::SetClipModel( idClipModel *model, const float density, int id, bool freeOld ) {
	int minIndex;
	idMat3 inertiaScale;

	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, current.i.position, current.i.orientation );

	// get mass properties from the trace model
	clipModel->GetMassProperties( density, mass, centerOfMass, inertiaTensor );

	// check whether or not the clip model has valid mass properties
	if ( mass <= 0.0f || FLOAT_IS_NAN( mass ) ) {
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING( "idPhysics_RigidBody::SetClipModel: invalid mass for entity '%s' type '%s'",
							self->name.c_str(), self->GetType()->classname );
		mass = 1.0f;
		centerOfMass.Zero();
		inertiaTensor.Identity();
	}

#ifdef MOD_WATERPHYSICS
	this->volume = mass / density;
#endif

	// check whether or not the inertia tensor is balanced
	minIndex = Min3Index( inertiaTensor[0][0], inertiaTensor[1][1], inertiaTensor[2][2] );
	inertiaScale.Identity();
	inertiaScale[0][0] = inertiaTensor[0][0] / inertiaTensor[minIndex][minIndex];
	inertiaScale[1][1] = inertiaTensor[1][1] / inertiaTensor[minIndex][minIndex];
	inertiaScale[2][2] = inertiaTensor[2][2] / inertiaTensor[minIndex][minIndex];

	if ( inertiaScale[0][0] > MAX_INERTIA_SCALE || inertiaScale[1][1] > MAX_INERTIA_SCALE || inertiaScale[2][2] > MAX_INERTIA_SCALE ) {
		gameLocal.DWarning( "idPhysics_RigidBody::SetClipModel: unbalanced inertia tensor for entity '%s' type '%s'",
							self->name.c_str(), self->GetType()->classname );
		float min = inertiaTensor[minIndex][minIndex] * MAX_INERTIA_SCALE;
		inertiaScale[(minIndex+1)%3][(minIndex+1)%3] = min / inertiaTensor[(minIndex+1)%3][(minIndex+1)%3];
		inertiaScale[(minIndex+2)%3][(minIndex+2)%3] = min / inertiaTensor[(minIndex+2)%3][(minIndex+2)%3];
		inertiaTensor *= inertiaScale;
	}

	inverseMass = 1.0f / mass;
	inverseInertiaTensor = inertiaTensor.Inverse() * ( 1.0f / 6.0f );

	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();
}

/*
================
idPhysics_RigidBody::GetClipModel
================
*/
idClipModel *idPhysics_RigidBody::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
idPhysics_RigidBody::GetNumClipModels
================
*/
int idPhysics_RigidBody::GetNumClipModels( void ) const {
	return 1;
}

/*
================
idPhysics_RigidBody::SetMass
================
*/
void idPhysics_RigidBody::SetMass( float mass, int id ) {
	assert( mass > 0.0f );
	inertiaTensor *= mass / this->mass;
	inverseInertiaTensor = inertiaTensor.Inverse() * (1.0f / 6.0f);
	this->mass = mass;
	inverseMass = 1.0f / mass;
}

/*
================
idPhysics_RigidBody::GetMass
================
*/
float idPhysics_RigidBody::GetMass( int id ) const {
#ifdef MOD_WATERPHYSICS
    if( this->water != NULL ) {
        idVec3 pos;
        float  percent,bMass;

        pos = this->current.i.position + this->centerOfMass*this->current.i.orientation;
        percent = this->GetSubmergedPercent(pos,this->current.i.orientation);
        bMass = mass - (this->volume * this->water->GetDensity() * percent);

        return bMass;
    }
    else
#endif
        return mass;
}

/*
================
idPhysics_RigidBody::SetFriction
================
*/
void idPhysics_RigidBody::SetFriction( const float linear, const float angular, const float contact ) {
	if (	linear < 0.0f ||
			angular < 0.0f || angular > 1.0f ||
			contact < 0.0f || contact > 1.0f ) {
		return;
	}
	linearFriction = linear;
	angularFriction = angular;
	contactFriction = contact;
}

/*
================
idPhysics_RigidBody::SetBouncyness
================
*/
void idPhysics_RigidBody::SetBouncyness( const float b ) {
	if ( b < 0.0f || b > 1.0f ) {
		return;
	}
	bouncyness = b;
}

/*
================
idPhysics_RigidBody::Rest
================
*/
void idPhysics_RigidBody::Rest( void ) 
{
	current.atRest = gameLocal.time;
	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();
	self->BecomeInactive( TH_PHYSICS );

	// grayman #2908 - if this is a mine, we can't NULL m_SetInMotionByActor
	// because we need that if the mine ever kills someone

	if ( self->IsType(idProjectile::Type) )
	{
		idProjectile* proj = static_cast<idProjectile*>(self);
		if ( !proj->IsMine() )
		{
			self->m_SetInMotionByActor = NULL;
		}
	}
	else // grayman #2816
	{
		self->m_SetInMotionByActor = NULL;
	}

	// grayman #3992 - dropped weapons should start their visual stims

	if (self->m_droppedByAI)
	{
		if (self->spawnArgs.GetBool("is_weapon_melee") || self->spawnArgs.GetBool("is_weapon_ranged"))
		{
			// reset stim only if some distance from last known resting place
			if ( (self->GetPhysics()->GetOrigin() - self->m_LastRestPos).LengthSqr() > 128 * 128 )
			{
				self->m_LastRestPos = self->GetPhysics()->GetOrigin();
				self->ClearStimIgnoreList(ST_VISUAL);
				self->EnableStim(ST_VISUAL);
			}
		}
	}

//	self->m_SetInMotionByActor = NULL;
//	self->m_droppedByAI = false; // grayman #1330 // grayman #3075 - use this designation while at rest, for collision events
}

/*
================
idPhysics_RigidBody::DropToFloor
================
*/
void idPhysics_RigidBody::DropToFloor( void ) {
	dropToFloor = true;
	testSolid = true;
}

/*
================
idPhysics_RigidBody::NoContact
================
*/
void idPhysics_RigidBody::NoContact( void ) {
	noContact = true;
}

/*
================
idPhysics_RigidBody::Activate
================
*/
void idPhysics_RigidBody::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
idPhysics_RigidBody::PutToRest

  put to rest until something collides with this physics object
================
*/
void idPhysics_RigidBody::PutToRest( void ) {
	Rest();
}

/*
================
idPhysics_RigidBody::EnableImpact
================
*/
void idPhysics_RigidBody::EnableImpact( void ) {
	noImpact = false;
}

/*
================
idPhysics_RigidBody::DisableImpact
================
*/
void idPhysics_RigidBody::DisableImpact( void ) {
	noImpact = true;
}

/*
================
idPhysics_RigidBody::SetContents
================
*/
void idPhysics_RigidBody::SetContents( int contents, int id ) {
	assert(clipModel != NULL);
	clipModel->SetContents( contents );
}

/*
================
idPhysics_RigidBody::GetContents
================
*/
int idPhysics_RigidBody::GetContents( int id ) const {
	assert(clipModel != NULL);
	return clipModel->GetContents();
}

/*
================
idPhysics_RigidBody::GetBounds
================
*/
const idBounds &idPhysics_RigidBody::GetBounds( int id ) const {
	assert(clipModel != NULL);
	return clipModel->GetBounds();
}

/*
================
idPhysics_RigidBody::GetAbsBounds
================
*/
const idBounds &idPhysics_RigidBody::GetAbsBounds( int id ) const {
	assert(clipModel != NULL);
	return clipModel->GetAbsBounds();
}

const trace_t*	idPhysics_RigidBody::GetBlockingInfo() const
{
	return isBlocked ? &collisionTrace : NULL;
}

idEntity* idPhysics_RigidBody::GetBlockingEntity() const
{
	return isBlocked ? gameLocal.entities[collisionTrace.c.entityNum] : NULL;
}

void idPhysics_RigidBody::DampenMomentums(float linear, float angular)
{
	current.i.linearMomentum *= linear;
	current.i.angularMomentum *= angular;
}


/*
================
idPhysics_RigidBody::Evaluate

  Evaluate the impulse based rigid body physics.
  When a collision occurs an impulse is applied at the moment of impact but
  the remaining time after the collision is ignored.
================
*/
bool idPhysics_RigidBody::Evaluate( int timeStepMSec, int endTimeMSec ) {
	rigidBodyPState_t next;
	idAngles angles;
	trace_t collision;
	idVec3 impulse;
	idEntity *ent(NULL);
	idVec3 oldOrigin, masterOrigin;
	idMat3 oldAxis, masterAxis;
	float timeStep;
	bool collided = false;
	bool cameToRest = false;

	// greebo: For now, we aren't blocked
	isBlocked = false;

	// stgatilov: avoid doing zero steps (useless and causes division by zero)
	if (timeStepMSec <= 0)
		return false;

	timeStep = MS2SEC( timeStepMSec );
	current.lastTimeStep = timeStep;

	// stgatilov #5992: compute total force/torque
	current.forceApplications.ComputeTotal(
		current.i.position + centerOfMass * current.i.orientation,
		&externalForce, &externalTorque, &externalForcePoint
	);
	// end of game tic: clear force list
	// forces will not show up on next game tic unless reapplied via AddForce
	current.forceApplications.Clear();

	if ( hasMaster ) {
		oldOrigin = current.i.position;
		oldAxis = current.i.orientation;
		self->GetMasterPosition( masterOrigin, masterAxis );

		current.i.position = masterOrigin + current.localOrigin * masterAxis;
		current.i.orientation = (isOrientated) ? current.localAxis * masterAxis : current.localAxis;

		// greebo: Only check for collisions for "solid" bind slaves and if the master is non-AF
		// Ishtvan: Do not block an AF attachment that is itself attached to an AF.  
		// This causes problems with AF evaluation.
		// TODO: Use advanced AF binding code to add AF body for entity in this case, as if it were bound directly to AF
		if ( 
				(clipModel->GetContents() & (CONTENTS_SOLID|CONTENTS_CORPSE)) 
				&& !self->GetBindMaster()->IsType(idAnimatedEntity::Type)
				&& !(
						self->GetBindMaster()->IsType(idAFAttachment::Type) 
						&& static_cast<idAFAttachment *>(self->GetBindMaster())->GetBody()
					)
			)
		{
			// For non-AF masters we check for collisions
			gameLocal.push.ClipPush( collisionTrace, self, PUSHFL_CLIP|PUSHFL_APPLYIMPULSE, oldOrigin, oldAxis, current.i.position, current.i.orientation );

			if (collisionTrace.fraction < 1.0f )
			{
				clipModel->Link( gameLocal.clip, self, 0, oldOrigin, oldAxis );
				current.i.position = oldOrigin;
				current.i.orientation = oldAxis;
				isBlocked = true;
				return false;
			}
		}

		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );
		current.i.linearMomentum = mass * ( ( current.i.position - oldOrigin ) / timeStep );
		current.i.angularMomentum = inertiaTensor * ( ( current.i.orientation * oldAxis.Transpose() ).ToAngularVelocity() / timeStep );

		return ( current.i.position != oldOrigin || current.i.orientation != oldAxis );
	}

	// if the body is at rest
	if ( current.atRest >= 0 || timeStep <= 0.0f ) {
		DebugDraw();
		return false;
	}

	// if putting the body to rest
	if (dropToFloor && !self->m_droppedByAI && !self->m_isFlinder)	// grayman #1330 - only go straight to the floor if an AI didn't drop it
																	// grayman #4230 - and it's not a flinder
	{
		DropToFloorAndRest();
		return true;
	}

#ifdef RB_TIMINGS
	timer_total.Start();
#endif

	// move the rigid body velocity into the frame of a pusher
//	current.i.linearMomentum -= current.pushVelocity.SubVec3( 0 ) * mass;
//	current.i.angularMomentum -= current.pushVelocity.SubVec3( 1 ) * inertiaTensor;

	clipModel->Unlink();

	next = current;
	
	// calculate next position and orientation
	Integrate( timeStep, next );

#ifdef RB_TIMINGS
	timer_collision.Start();
#endif

	// check for collisions from the current to the next state
	collided = CheckForCollisions( timeStep, next, collision );

#ifdef RB_TIMINGS
	timer_collision.Stop();
#endif

	// set the new state
	current = next;

	if ( collided )
	{
		// apply collision impulse
		if ( CollisionImpulse( collision, impulse ) )
		{
			current.atRest = gameLocal.time;
		}

		// grayman #3516
		idEntity* ent = gameLocal.entities[collision.c.entityNum];
		if (ent && ( ent != gameLocal.world ) )
		{
			self->CheckCollision(ent);
		}
	}

	// update the position of the clip model
	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );

	DebugDraw();

	if ( !noContact )
	{

#ifdef RB_TIMINGS
		timer_collision.Start();
#endif
		// get contacts
		EvaluateContacts();

#ifdef RB_TIMINGS
		timer_collision.Stop();
#endif

		// check if the body has come to rest
		if ( ( externalForce.LengthSqr() == 0.0f ) && TestIfAtRest() )
		{
			// put to rest
			Rest();
			cameToRest = true;
		}
		else
		{
			// apply contact friction
			if ( mass >= 1.0 ) // grayman #3452
			{
				ContactFriction( timeStep );
			}
			else
			{
				SmallMassContactFriction( timeStep );
			}
		}
	}

	if ( current.atRest < 0 )
	{
		ActivateContactEntities();
	}

	if ( collided )
	{
		// if the rigid body didn't come to rest or the other entity is not at rest
		ent = gameLocal.entities[collision.c.entityNum];
		if ( ent && ( !cameToRest || !ent->IsAtRest() ) )
		{
			// apply impact to other entity
			ent->ApplyImpulse( self, collision.c.id, collision.c.point, -impulse );

			if (ent->m_SetInMotionByActor.GetEntity() == NULL)
			{
				ent->m_SetInMotionByActor = self->m_SetInMotionByActor;
				ent->m_MovedByActor = self->m_MovedByActor;
			}
		}

		// greebo: Are we stuck? We still have to consider gravity and external forces
		if (collision.fraction <= 0.001f)
		{
			// Get the mass center in world coordinates
			idVec3 massCenter(current.i.position + centerOfMass * current.i.orientation);

			// Calculate the lever arms
			idVec3 arm1(externalForcePoint - massCenter);
			idVec3 arm2(externalForcePoint - collision.c.point);
			idVec3 arm1N(arm1);

			//gameRenderWorld->DebugArrow(colorCyan, massCenter, massCenter + arm1, 1, 20);
			//gameRenderWorld->DebugArrow(colorMagenta, collision.c.point, collision.c.point + arm2, 1, 20);

			float l1 = arm1N.NormalizeFast();
			float l2 = arm1N * arm2;
			//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Arm 1: %f, Arm2: %f\r", l1, l2);

			if (arm2.LengthFast() > l1)
			{
				// Apply the linear momentum caused by the external force
				current.i.linearMomentum -= externalForce*2;
				current.i.angularMomentum += (externalForcePoint - collision.c.point).Cross(externalForce);
			}
			else if (fabs(l1) > 0.01f)
			{
				float armRatio = l2/l1;
				float forceFactor = 4*armRatio*(armRatio - 1);

				forceFactor *= 15;
				idVec3 leverForceLinear = externalForce * forceFactor;

				//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("forceFactor: %f\r", forceFactor);
				//DM_LOG(LC_ENTITY, LT_INFO)LOGVECTOR("Current impulse", current.i.linearMomentum);
				//DM_LOG(LC_ENTITY, LT_INFO)LOGVECTOR("External Force", current.externalForce);
				//DM_LOG(LC_ENTITY, LT_INFO)LOGVECTOR("External Force Modified", current.externalForce*forceFactor);
				//gameRenderWorld->DebugArrow(colorMdGrey, massCenter, massCenter + leverForceLinear, 1, 20);

				// Apply the linear momentum caused by the lever force
				current.i.linearMomentum += leverForceLinear;
				current.i.angularMomentum += (externalForcePoint - collision.c.point).Cross(externalForce);
			}
		}
	}

	// move the rigid body velocity back into the world frame
//	current.i.linearMomentum += current.pushVelocity.SubVec3( 0 ) * mass;
//	current.i.angularMomentum += current.pushVelocity.SubVec3( 1 ) * inertiaTensor;
	current.pushVelocity.Zero();

	current.lastTimeStep = timeStep;

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "rigid body moved outside world bounds for entity '%s' type '%s' at (%s)",
					self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		Rest();
	}

#ifdef RB_TIMINGS
	timer_total.Stop();

	if ( rb_showTimings->integer == 1 ) {
		gameLocal.Printf( "%12s: t %1.4f cd %1.4f\n",
						self->name.c_str(),
						timer_total.Milliseconds(), timer_collision.Milliseconds() );
		lastTimerReset = 0;
	}
	else if ( rb_showTimings->integer == 2 ) {
		numRigidBodies++;
		if ( endTimeMSec > lastTimerReset ) {
			gameLocal.Printf( "rb %d: t %1.4f cd %1.4f\n",
							numRigidBodies,
							timer_total.Milliseconds(), timer_collision.Milliseconds() );
		}
	}
	if ( endTimeMSec > lastTimerReset ) {
		lastTimerReset = endTimeMSec;
		numRigidBodies = 0;
		timer_total.Clear();
		timer_collision.Clear();
	}
#endif

	if (cv_phys_show_momentum.GetBool()) 
	{
		gameRenderWorld->DebugText( idStr(current.i.linearMomentum.LengthFast()), GetAbsBounds().GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
	}

	return true; // grayman #2478
}

/*
================
idPhysics_RigidBody::UpdateTime
================
*/
void idPhysics_RigidBody::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_RigidBody::GetTime
================
*/
int idPhysics_RigidBody::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
idPhysics_RigidBody::GetImpactInfo
================
*/
void idPhysics_RigidBody::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	idVec3 linearVelocity, angularVelocity;
	idMat3 inverseWorldInertiaTensor;

	linearVelocity = inverseMass * current.i.linearMomentum;
	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;

	info->invMass = inverseMass;
	info->invInertiaTensor = inverseWorldInertiaTensor;
	info->position = point - ( current.i.position + centerOfMass * current.i.orientation );
	info->velocity = linearVelocity + angularVelocity.Cross( info->position );
}

/*
================
idPhysics_RigidBody::ApplyImpulse
================
*/
void idPhysics_RigidBody::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( noImpact )
	{
		return;
	}

	// tels: check that the impulse does not exceed the max values
	// FIXME: the comparison here is just a dummy
	if ((maxForce.x > 0) && 
	    ((fabs(impulse.x) > maxForce.x) || 
		 (fabs(impulse.y) > maxForce.y) || 
		 (fabs(impulse.z) > maxForce.z)) ) 
	{
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("impulse (%f %f %f) > maxForce (%f %f %f) for entity %s\r\r", 
			impulse.x, impulse.y, impulse.z,
			maxForce.x, maxForce.y, maxForce.z,
			self->name.c_str()
			);
		// FIXME: self needs to be replaced by whatever entity generated the impulse
		self->Killed( gameLocal.world, gameLocal.world, 0, self->GetLocalCoordinates(GetOrigin()), 0);
		return;
	}

	// greebo: Check if we have a master - if yes, propagate the impulse to it
	if ( hasMaster )
	{
		idEntity* master = self->GetBindMaster();

		assert(master != NULL); // Bind master must not be null

		master->GetPhysics()->ApplyImpulse(id, point, impulse);
	}

	current.i.linearMomentum += impulse;
	current.i.angularMomentum += ( point - ( current.i.position + centerOfMass * current.i.orientation ) ).Cross( impulse );

	Activate();
}

/*
================
idPhysics_RigidBody::AddForce
================
*/
void idPhysics_RigidBody::AddForce( const int id, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId )
{
	if ( noImpact )
	{
		return;
	}

	// add or replace existing force application
	current.forceApplications.Add(point, force, applId);

	Activate();
}

/*
================
idPhysics_RigidBody::IsAtRest
================
*/
bool idPhysics_RigidBody::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
idPhysics_RigidBody::GetRestStartTime
================
*/
int idPhysics_RigidBody::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
idPhysics_RigidBody::IsPushable
================
*/
bool idPhysics_RigidBody::IsPushable( void ) const {
	return ( !noImpact && !hasMaster );
}

/*
================
idPhysics_RigidBody::SaveState
================
*/
void idPhysics_RigidBody::SaveState( void ) {
	saved = current;
}

/*
================
idPhysics_RigidBody::RestoreState
================
*/
void idPhysics_RigidBody::RestoreState( void ) {
	current = saved;

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );

	EvaluateContacts();
}

/*
================
idPhysics::SetOrigin
================
*/
void idPhysics_RigidBody::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.i.position = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.i.position = newOrigin;
	}

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, clipModel->GetAxis() );

	Activate();
}

/*
================
idPhysics::SetAxis
================
*/
void idPhysics_RigidBody::SetAxis( const idMat3 &newAxis, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localAxis = newAxis;
	if ( hasMaster && isOrientated ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.i.orientation = newAxis * masterAxis;
	}
	else {
		current.i.orientation = newAxis;
	}

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), clipModel->GetOrigin(), current.i.orientation );

	Activate();
}

/*
================
idPhysics::Move
================
*/
void idPhysics_RigidBody::Translate( const idVec3 &translation, int id ) {

	current.localOrigin += translation;
	current.i.position += translation;

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, clipModel->GetAxis() );

	Activate();
}

/*
================
idPhysics::Rotate
================
*/
void idPhysics_RigidBody::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.i.orientation *= rotation.ToMat3();
	current.i.position *= rotation;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localAxis *= rotation.ToMat3();
		current.localOrigin = ( current.i.position - masterOrigin ) * masterAxis.Transpose();
	}
	else {
		current.localAxis = current.i.orientation;
		current.localOrigin = current.i.position;
	}

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );

	Activate();
}

/*
================
idPhysics_RigidBody::GetOrigin
================
*/
const idVec3 &idPhysics_RigidBody::GetOrigin( int id ) const {
	return current.i.position;
}

/*
================
idPhysics_RigidBody::GetAxis
================
*/
const idMat3 &idPhysics_RigidBody::GetAxis( int id ) const {
	return current.i.orientation;
}

/*
================
idPhysics_RigidBody::SetLinearVelocity
================
*/
void idPhysics_RigidBody::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.i.linearMomentum = newLinearVelocity * mass;
	Activate();
}

/*
================
idPhysics_RigidBody::SetAngularVelocity
================
*/
void idPhysics_RigidBody::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
	current.i.angularMomentum = newAngularVelocity * current.i.orientation.Transpose() * inertiaTensor * current.i.orientation;
	Activate();
}

/*
================
idPhysics_RigidBody::GetLinearVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetLinearVelocity( int id ) const {
	static idVec3 curLinearVelocity;
	curLinearVelocity = current.i.linearMomentum * inverseMass;
	return curLinearVelocity;
}

/*
================
idPhysics_RigidBody::GetAngularVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetAngularVelocity( int id ) const {
	static idVec3 curAngularVelocity;
	idMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	curAngularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
	return curAngularVelocity;
}

/*
================
idPhysics_RigidBody::SetMaxForce
================
*/
void idPhysics_RigidBody::SetMaxForce( const idVec3 &newMaxForce ) {
	maxForce = newMaxForce;
}

/*
================
idPhysics_RigidBody::SetMaxTorque
================
*/
void idPhysics_RigidBody::SetMaxTorque( const idVec3 &newMaxTorque ) {
	maxTorque = newMaxTorque;
}

/*
================
idPhysics_RigidBody::GetMaxForce
================
*/
const idVec3 &idPhysics_RigidBody::GetMaxForce( void ) const {
	return maxForce;
}

/*
================
idPhysics_RigidBody::GetMaxTorque
================
*/
const idVec3 &idPhysics_RigidBody::GetMaxTorque( void ) const {
	return maxTorque;
}

/*
================
idPhysics_RigidBody::ClipTranslation
================
*/
void idPhysics_RigidBody::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.TranslationModel( results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
											clipModel, clipModel->GetAxis(), clipMask,
											model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Translation( results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
											clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
idPhysics_RigidBody::ClipRotation
================
*/
void idPhysics_RigidBody::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.RotationModel( results, clipModel->GetOrigin(), rotation,
											clipModel, clipModel->GetAxis(), clipMask,
											model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Rotation( results, clipModel->GetOrigin(), rotation,
											clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
idPhysics_RigidBody::ClipContents
================
*/
int idPhysics_RigidBody::ClipContents( const idClipModel *model ) const {
	if ( model ) {
		return gameLocal.clip.ContentsModel( clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
									model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		return gameLocal.clip.Contents( clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, NULL );
	}
}

/*
================
idPhysics_RigidBody::DisableClip
================
*/
void idPhysics_RigidBody::DisableClip( void ) {
	clipModel->Disable();
}

/*
================
idPhysics_RigidBody::EnableClip
================
*/
void idPhysics_RigidBody::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
idPhysics_RigidBody::UnlinkClip
================
*/
void idPhysics_RigidBody::UnlinkClip( void ) {
	clipModel->Unlink();
}

/*
================
idPhysics_RigidBody::LinkClip
================
*/
void idPhysics_RigidBody::LinkClip( void ) {
	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );
}

/*
================
idPhysics_RigidBody::EvaluateContacts
================
*/
bool idPhysics_RigidBody::EvaluateContacts( void ) {
	ClearContacts();

	idVec6 dir;
	dir.SubVec3(0) = current.i.linearMomentum + current.lastTimeStep * gravityVector * mass;
	dir.SubVec3(1) = current.i.angularMomentum;
	dir.SubVec3(0).Normalize();
	dir.SubVec3(1).Normalize();
	int num;
	contactInfo_t carr[CONTACTS_MAX_NUMBER];
	num = gameLocal.clip.Contacts(
		carr, CONTACTS_MAX_NUMBER, clipModel->GetOrigin(),
		dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(), clipMask, self
	);
	contacts.SetNum( num, false );
	memcpy( contacts.Ptr(), carr, num * sizeof(carr[0]) );

	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}

/*
================
idPhysics_RigidBody::SetPushed
================
*/
void idPhysics_RigidBody::SetPushed( int deltaTime ) {
	idRotation rotation;

	rotation = ( saved.i.orientation * current.i.orientation ).ToRotation();

	// velocity with which the af is pushed
	current.pushVelocity.SubVec3(0) += ( current.i.position - saved.i.position ) / ( deltaTime * idMath::M_MS2SEC );
	current.pushVelocity.SubVec3(1) += rotation.GetVec() * -DEG2RAD( rotation.GetAngle() ) / ( deltaTime * idMath::M_MS2SEC );
}

/*
================
idPhysics_RigidBody::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity.SubVec3(0);
}

/*
================
idPhysics_RigidBody::GetPushedAngularVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetPushedAngularVelocity( const int id ) const {
	return current.pushVelocity.SubVec3(1);
}

/*
================
idPhysics_RigidBody::SetMaster
================
*/
void idPhysics_RigidBody::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.i.position - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				current.localAxis = current.i.orientation * masterAxis.Transpose();
			}
			else {
				current.localAxis = current.i.orientation;
			}
			hasMaster = true;
			isOrientated = orientated;
			ClearContacts();
		}
	}
	else {
		if ( hasMaster ) {
			hasMaster = false;
			Activate();
		}
	}
}

/*
================
idPhysics_RigidBody::WriteToSnapshot
================
*/
void idPhysics_RigidBody::WriteToSnapshot( idBitMsgDelta &msg ) const {
	idCQuat quat, localQuat;

	quat = current.i.orientation.ToCQuat();
	localQuat = current.localAxis.ToCQuat();

	msg.WriteLong( current.atRest );
	msg.WriteFloat( current.i.position[0] );
	msg.WriteFloat( current.i.position[1] );
	msg.WriteFloat( current.i.position[2] );
	msg.WriteFloat( quat.x );
	msg.WriteFloat( quat.y );
	msg.WriteFloat( quat.z );
	msg.WriteFloat( current.i.linearMomentum[0], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.linearMomentum[1], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.linearMomentum[2], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[0], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[1], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[2], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteDeltaFloat( current.i.position[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.i.position[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.i.position[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( quat.x, localQuat.x );
	msg.WriteDeltaFloat( quat.y, localQuat.y );
	msg.WriteDeltaFloat( quat.z, localQuat.z );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
/*	msg.WriteDeltaFloat( 0.0f, current.externalForce[0], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[1], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[2], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[0], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[1], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[2], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );*/
}

/*
================
idPhysics_RigidBody::ReadFromSnapshot
================
*/
void idPhysics_RigidBody::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	idCQuat quat, localQuat;

	current.atRest = msg.ReadLong();
	current.i.position[0] = msg.ReadFloat();
	current.i.position[1] = msg.ReadFloat();
	current.i.position[2] = msg.ReadFloat();
	quat.x = msg.ReadFloat();
	quat.y = msg.ReadFloat();
	quat.z = msg.ReadFloat();
	current.i.linearMomentum[0] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.linearMomentum[1] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.linearMomentum[2] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[0] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[1] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[2] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.localOrigin[0] = msg.ReadDeltaFloat( current.i.position[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.i.position[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.i.position[2] );
	localQuat.x = msg.ReadDeltaFloat( quat.x );
	localQuat.y = msg.ReadDeltaFloat( quat.y );
	localQuat.z = msg.ReadDeltaFloat( quat.z );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
/*	current.externalForce[0] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalForce[1] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalForce[2] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalTorque[0] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalTorque[1] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalTorque[2] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );*/

	current.i.orientation = quat.ToMat3();
	current.localAxis = localQuat.ToMat3();

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );
	}
}
