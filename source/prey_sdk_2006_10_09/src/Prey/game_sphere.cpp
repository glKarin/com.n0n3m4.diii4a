// Game_Sphere.cpp
//
// pushable exploding sphere

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"



CLASS_DECLARATION(hhMoveable, hhSphere)
	EVENT( EV_Touch,			hhSphere::Event_Touch )
END_CLASS


hhSphere::hhSphere() {
	additionalAxis.Identity();
}

void hhSphere::Spawn(void) {

	radius = (GetPhysics()->GetBounds()[1][0] - GetPhysics()->GetBounds()[0][0]) * 0.5f;
	lastOrigin = GetPhysics()->GetOrigin();
	additionalAxis.Identity();
	CreateLight();

	fl.takedamage = false;
	BecomeActive(TH_THINK);
}

void hhSphere::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( radius );
	savefile->WriteVec3( lastOrigin );
	savefile->WriteMat3( additionalAxis );
	light.Save(savefile);
}

void hhSphere::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( radius );
	savefile->ReadVec3( lastOrigin );
	savefile->ReadMat3( additionalAxis );
	light.Restore(savefile);
}

void hhSphere::CreateLight() {
	if ( spawnArgs.GetBool("haslight") ) {
		idStr light_shader = spawnArgs.GetString("mtr_light");
		idVec3 light_color = spawnArgs.GetVector("light_color");
		idVec3 light_frustum = spawnArgs.GetVector("light_frustum");
		idVec3 light_offset = spawnArgs.GetVector("offset_light");
		idVec3 light_target = spawnArgs.GetVector("offset_lighttarget");

		idDict args;
		idVec3 lightOrigin = GetPhysics()->GetOrigin() + light_offset * GetPhysics()->GetAxis();
		light_target.Normalize();
		idMat3 lightAxis = (light_target * GetPhysics()->GetAxis()).hhToMat3();

		if ( light_shader.Length() ) {
			args.Set( "texture", light_shader );
		}
		args.SetVector( "origin", lightOrigin );
		args.Set ("angles", lightAxis.ToAngles().ToString());
		args.SetVector( "_color", light_color );
		args.SetVector( "light_target", lightAxis[0] * light_frustum.x );
		args.SetVector( "light_right", lightAxis[1] * light_frustum.y );
		args.SetVector( "light_up", lightAxis[2] * light_frustum.z );
		light = ( idLight * )gameLocal.SpawnEntityType( idLight::Type, &args );
		light->Bind(this, true);
		light->SetLightParm( 6, 1.0f );
		light->SetLightParm( SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.time) );
	}
}

void hhSphere::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	// skip idMoveable::Damage which handles damage differently
	idEntity::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
}


void hhSphere::RollThink( void ) {
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

		// if the sphere moved
		if ( movedDistanceSquared > 0.0f && movedDistanceSquared < 20.0f) {

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

void hhSphere::Think() {
	RollThink();
}

void hhSphere::ClientPredictionThink( void ) {
	RollThink();
}

bool hhSphere::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin = vec3_origin;
	axis = additionalAxis * GetPhysics()->GetAxis().Inverse();
	return true;
}

void hhSphere::Event_Touch( idEntity *other, trace_t *trace ) {
	if (spawnArgs.GetBool("walkthrough")) {
		idVec3 otherVel = other->GetPhysics()->GetLinearVelocity();
		float otherSpeed = otherVel.NormalizeFast();
		if (otherSpeed > 50.0f) { // && GetPhysics()->IsAtRest()) {
			idVec3 toSide = hhUtils::RandomSign() * other->GetAxis()[1];
			idVec3 toMoveable = GetOrigin() - other->GetOrigin();
			toMoveable.NormalizeFast();
			idVec3 newVel = ( 3*otherVel + toSide + hhUtils::RandomVector() ) * (1.0f/5.0f);
			newVel.z = 0.0f;
			newVel.NormalizeFast();
			newVel *= otherSpeed*1.5f;
			GetPhysics()->SetLinearVelocity(newVel);
		}
	}
}
