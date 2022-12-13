
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idForce, hhForce_Converge )
END_CLASS


hhForce_Converge::hhForce_Converge( void ) {
	restoreFactor = 1.0f;
	restoreForceSlack = 1.0f;
	restoreTime = 0.1f;
	ent = NULL;
	physics = NULL;
	axisEnt = NULL;
	bShuttle = false;
}

hhForce_Converge::~hhForce_Converge( void ) {
}

void hhForce_Converge::Save(idSaveGame *savefile) const {
	ent.Save(savefile);
	axisEnt.Save(savefile);
	savefile->WriteVec3( target );
	savefile->WriteVec3( offset );
	savefile->WriteInt( bodyID );
	savefile->WriteFloat( restoreTime );
	savefile->WriteFloat( restoreFactor );
	savefile->WriteFloat( restoreForceSlack );
	savefile->WriteBool( bShuttle );
}

void hhForce_Converge::Restore( idRestoreGame *savefile ) {
	ent.Restore(savefile);
	axisEnt.Restore(savefile);
	savefile->ReadVec3( target );
	savefile->ReadVec3( offset );
	savefile->ReadInt( bodyID );
	savefile->ReadFloat( restoreTime );
	savefile->ReadFloat( restoreFactor );
	savefile->ReadFloat( restoreForceSlack );
	savefile->ReadBool( bShuttle );

	physics = NULL; // HUMANHEAD mdl:  Updated each frame
}

void hhForce_Converge::Evaluate( int time ) {

	// Get the physics of ent each frame, in case their physics object was changed.
	physics = ent.IsValid() ? ent->GetPhysics() : NULL;

	// Apply convergent force
	if (physics) {
		idVec3 p = physics->GetOrigin( bodyID ) + offset * physics->GetAxis( bodyID );
		idVec3 x = p - target;

		if (axisEnt.IsValid()) {
			idBounds bounds = ent->GetPhysics()->GetBounds();
			idVec3 rightPt = ent->GetOrigin() + (ent->GetAxis()[0] * bounds[0].x);
			idVec3 axisRightPt = axisEnt->GetOrigin() + (axisEnt->GetAxis()[1] * bounds[1].x);

			idVec3 leftPt = ent->GetOrigin() + (ent->GetAxis()[0] * bounds[1].x);
			idVec3 axisLeftPt = axisEnt->GetOrigin() + (axisEnt->GetAxis()[1] * bounds[0].x);

			//gameRenderWorld->DebugLine(idVec4(1, 0, 0, 1), rightPt, axisRightPt, 100);

			p = rightPt;
			x = p - axisRightPt;

			x *= restoreForceSlack; //allow a linear buildup based on the distance of the target point

			idVec3 v = physics->GetLinearVelocity();
			float m = physics->GetMass();
			float b = idMath::Sqrt(4*restoreFactor*m);
			idVec3 force = -restoreFactor*x - b*v;			// use with addforce, k=200000
			physics->AddForce(bodyID, p, force * 0.5f );				// Great to show mass variations

			//gameRenderWorld->DebugLine(idVec4(1, 0, 0, 1), leftPt, axisLeftPt, 100);

			p = leftPt;
			x = p - axisLeftPt;

			x *= restoreForceSlack; //allow a linear buildup based on the distance of the target point

			force = axisLeftPt - leftPt;
			force *= m * 4;
			physics->AddForce(bodyID, p, force );				// Great to show mass variations
			return;
		}

		x *= restoreForceSlack; //allow a linear buildup based on the distance of the target point

		if (ent->IsType(idAFEntity_Base::Type)) {	// Actors use velocity method since their masses are unrealistic, ragdolls need it too
			if (!ent->fl.tooHeavyForTractor) {
				// Velocity method: cover distance in fixed time
				idVec3 velocity = -x / restoreTime;					// Use with SetLinearVelocity()
				if (ent->IsType(idActor::Type)) { //if a player is affected by this force..
					//..do not allow him to build up velocity into his ground plane
					if (ent->GetPhysics()->IsType(idPhysics_Player::Type) && ent->GetPhysics()->HasGroundContacts()) {
						velocity.x *= -ent->GetPhysics()->GetGravityNormal().x;
						velocity.y *= -ent->GetPhysics()->GetGravityNormal().y;
						velocity.z *= -ent->GetPhysics()->GetGravityNormal().z;
					}
				}
				physics->SetLinearVelocity(velocity, bodyID);
			}
			else {
				// Entities that are too heavy get no force, just give feedback force
			}
		} else {
			float m = physics->GetMass();
			if (bShuttle && m < 1500.0f) { // Different equation for picking up low mass objects with the shuttle
				idVec3 v = physics->GetLinearVelocity();
				float springFactor = restoreFactor*0.0002f;
				float dampFactor = idMath::Sqrt(springFactor);
				idVec3 force = (-x * springFactor * m) - (v * dampFactor * m);
				physics->AddForce(bodyID, p, force );
			} else {	// Critically Damped spring:  m*a = -k*x - b*v ;  b = 2 * sqrt(m*k)
				idVec3 v = physics->GetLinearVelocity();
				float m = physics->GetMass();
				float b = idMath::Sqrt(4.0f*restoreFactor*m);
				idVec3 force = -restoreFactor*x - b*v;			// use with addforce, k=200000
				physics->AddForce(bodyID, p, force );				// Great to show mass variations
			}
		}
/*		else {	// Impulse method
			idVec3 v = physics->GetLinearVelocity();
			float b = idMath::Sqrt(4*restoreFactor);
			impulse = -restoreFactor*x - b*v;							// Use with applyimpulse, k=50000
			physics->ApplyImpulse(bodyID, p, impulse );		// Worked for all but some moveables (k=50000)
		}
		else {	// Mass scaled force method, applies varying force to keep constant velocity
			idVec3 v = physics->GetLinearVelocity();
			float m = physics->GetMass();
			float b = idMath::Sqrt(4*restoreFactor);
			impulse = (-restoreFactor*x - b*v) * m;						// use with addforce, k=500
			physics->AddForce(bodyID, p, impulse );			// Works for all but ragdolls		 (k=500)
		}
		}*/
	}
}

void hhForce_Converge::RemovePhysics( const idPhysics *phys ) {
	// physics of ent is stored each evaluate so we can compare pointers directly, rather than querying
	// ent about it's physics.  This is done because typically, physics objects are reported as removed
	// during deconstruction of the entities.
	if (ent.IsValid() && phys == physics) {
		SetEntity(NULL);
	}
}

void hhForce_Converge::SetRestoreTime( float time ) {
	restoreTime = idMath::ClampFloat(0.02f, time, time);
}

void hhForce_Converge::SetTarget(idVec3 &newTarget) {
	target = newTarget;
}

void hhForce_Converge::SetEntity(idEntity *entity, int id, const idVec3 &point) {
	ent = entity;
	bodyID = id;
	offset = point;
	physics = NULL;
}

void hhForce_Converge::SetAxisEntity(idEntity *entity) {
	axisEnt = entity;
}

