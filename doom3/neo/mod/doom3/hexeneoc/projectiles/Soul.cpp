
#pragma hdrstop

#include "Game_local.h"
#include "Projectile.h"

#include "Soul.h"

const float	SOUL_SPEED	= 1000.0f;

CLASS_DECLARATION( idProjectile, idProj_Soul )

END_CLASS

void idProj_Soul::init() {
	dieTime	= 2000 + gameLocal.time;

	// spawn the effect around the wraith model
	idDict effectArgs;
	effectArgs.Set( "classname", "blank_item" );
	effectArgs.Set("model", spawnArgs.GetString("def_projectile"));
	effectArgs.SetVector("origin", GetPhysics()->GetOrigin());
	effectArgs.SetBool("nonsolid", true);
	effectArgs.SetBool("nocollide", true);
	gameLocal.SpawnEntityDef(effectArgs, &effect, true);
	effect->Bind(this, true);
	GetPhysics()->SetGravity( idVec3(0,0,0.000001f) );

	rnd = gameLocal.random.RandomInt(360);
}


void idProj_Soul::Think() {
	idProjectile::Think();

	if ( gameLocal.time >= dieTime ) {
		PostEventMS( &EV_Remove, 0.0 );
	}

   	ang.yaw = (( gameLocal.time / 1000 ) * 360) + rnd;
   	dir = ang.ToForward(); dir.Normalize();
	dir = dir + idVec3(0,0,0.66f); dir.Normalize();
   	SetAngles( ang );

	GetPhysics()->SetLinearVelocity(SOUL_SPEED * dir);
}
