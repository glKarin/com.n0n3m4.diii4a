#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//==========================================================================
//
//	hhPod
//
// type of mine that can be spawned from a pod spawner
//==========================================================================

CLASS_DECLARATION(hhMine, hhPod)
END_CLASS

hhPod::hhPod() {
	bMoverThink = false;
	additionalAxis.Identity();
}

void hhPod::Spawn(void) {
	// Uses clipModel because it is a moveable, which requires it

	// Turn off collision with corpses
	int oldMask = GetPhysics()->GetClipMask();
	GetPhysics()->SetClipMask( oldMask & (~CONTENTS_CORPSE) );

	// Rolling support
	radius = (GetPhysics()->GetBounds()[1][0] - GetPhysics()->GetBounds()[0][0]) * 0.5f;
	lastOrigin = GetPhysics()->GetOrigin();
	additionalAxis.Identity();
	BecomeActive(TH_THINK);
}

void hhPod::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( radius );
	savefile->WriteVec3( lastOrigin );
	savefile->WriteMat3( additionalAxis );
	savefile->WriteBool( bMoverThink );
}

void hhPod::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( radius );
	savefile->ReadVec3( lastOrigin );
	savefile->ReadMat3( additionalAxis );
	savefile->ReadBool( bMoverThink );
}

bool hhPod::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	StartSound( "snd_pain", SND_CHANNEL_ANY );

	float scale = 1.5f - (0.5f * health / spawnArgs.GetFloat("health"));
	scale = idMath::ClampFloat(1.0f, 1.5f, scale);
	SetShaderParm(SHADERPARM_ANY_DEFORM_PARM2, scale);

	return( idEntity::Pain(inflictor, attacker, damage, dir, location) );
}

void hhPod::Release() {
	idVec3 initLinearVelocity, initAngularVelocity;
	spawnArgs.GetVector( "init_velocity", "0 0 0", initLinearVelocity );
	spawnArgs.GetVector( "init_avelocity", "0 0 0", initAngularVelocity );

	// Make the pod physics kick in
	physicsObj.EnableImpact();
	physicsObj.Activate();
	physicsObj.SetLinearVelocity( initLinearVelocity );
	physicsObj.SetAngularVelocity( initAngularVelocity );

	// Start deforming again (disabled on pod spawners)
	float parm1 = spawnArgs.GetFloat("deformParm1");
	float parm2 = spawnArgs.GetFloat("deformParm2");
	SetDeformation(DEFORMTYPE_POD, parm1, parm2);
}


bool hhPod::Collide( const trace_t &collision, const idVec3 &velocity ) {
	AttemptToPlayBounceSound( collision, velocity );

	return hhMine::Collide( collision, velocity );
}


void hhPod::RollThink( void ) {
	float movedDistance, angle;
	idVec3 curOrigin, gravityNormal, dir;

	bool wasAtRest = IsAtRest();

	RunPhysics();

	// only need to give the visual model an additional rotation if the physics were run
	if ( !wasAtRest ) {

		// current physics state
		curOrigin = GetPhysics()->GetOrigin();

		dir = curOrigin - lastOrigin;
		float movedDistanceSquared = dir.LengthSqr();

		// if the pod moved
		if ( movedDistanceSquared > 0.0f && movedDistanceSquared < 100.0f) {

			gravityNormal = GetPhysics()->GetGravityNormal();

			// movement since last frame
			movedDistance = idMath::Sqrt( movedDistanceSquared );
			dir *= 1.0f / movedDistance;

			// Get local coordinate axes
			idVec3 right = -dir.Cross(gravityNormal);

			// Rotate about it proportional to the distance moved using axis/angle
			angle = 180.0f * movedDistance / (radius*idMath::PI);
			additionalAxis *= (idRotation( vec3_origin, right, angle).ToMat3());
		}

		// save state for next think
		lastOrigin = curOrigin;
	}

	Present();
}

void hhPod::Event_HoverTo( const idVec3 &position ) {
	bMoverThink = true;
	hhMine::Event_HoverTo( position );
}

void hhPod::Event_Unhover() {
	bDetonateOnCollision = true;
	bMoverThink = false;
	hhMine::Event_Unhover();
}

void hhPod::Think() {
	if ( bMoverThink ) {
		hhMoveable::Think();
	} else {
		RollThink();
	}
}

void hhPod::ClientPredictionThink() {
	if ( bMoverThink ) {
		hhMoveable::Think();
	} else {
		RollThink();
	}
}

bool hhPod::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin = vec3_origin;
	axis = additionalAxis * GetPhysics()->GetAxis().Inverse();
	return true;
}

