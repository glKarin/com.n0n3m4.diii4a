#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION(hhProjectile, hhProjectileGasbagPod)
	EVENT(EV_Collision_Flesh,				hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Metal,				hhProjectileGasbagPod::Event_Collision_Proj)
	EVENT(EV_Collision_AltMetal,			hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Wood,				hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Stone,				hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Glass,				hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Liquid,				hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_CardBoard,			hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Tile,				hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Forcefield,			hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Chaff,				hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Wallwalk,			hhProjectileGasbagPod::Event_Collision_SpawnPod)
	EVENT(EV_Collision_Pipe,				hhProjectileGasbagPod::Event_Collision_SpawnPod)

	EVENT(EV_AllowCollision_Flesh,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Metal,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_AltMetal,		hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Wood,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Stone,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Glass,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Liquid,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_CardBoard,		hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Tile,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Forcefield,		hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Pipe,			hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Wallwalk,		hhProjectileGasbagPod::Event_AllowCollision_Collide)
	EVENT(EV_AllowCollision_Spirit,			hhProjectileGasbagPod::Event_AllowCollision_PassThru)
	EVENT(EV_AllowCollision_Chaff,			hhProjectileGasbagPod::Event_AllowCollision_Collide)	//bjk: shield blocks all
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
void hhProjectileGasbagPod::Spawn(void) {
	BecomeActive(TH_TICKER);
}

void hhProjectileGasbagPod::Ticker(void) {
	renderEntity.shaderParms[SHADERPARM_ANY_DEFORM_PARM2] += .1f;
	if (renderEntity.shaderParms[SHADERPARM_ANY_DEFORM_PARM2] > 1.0f) {
		renderEntity.shaderParms[SHADERPARM_ANY_DEFORM_PARM2] = 1.0f;
		BecomeInactive(TH_TICKER);
	}
}

void hhProjectileGasbagPod::Event_Collision_SpawnPod(const trace_t* collision, const idVec3 &velocity) {
	idVec3 vel = GetPhysics()->GetLinearVelocity();
	idVec3 avel = GetPhysics()->GetAngularVelocity();

	physicsObj.PutToRest();
	physicsObj.SetContents(0);
	PostEventMS(&EV_Remove, 0);

	idDict args;
	args.Clear();
	args.Set("origin", (GetPhysics()->GetOrigin()).ToString());
	args.Set("axis", (GetPhysics()->GetAxis()).ToString());
	args.Set("nodrop", "1");

	idEntity *ent = gameLocal.SpawnObject("object_pod", &args);

	if (ent) {
		ent->GetPhysics()->SetLinearVelocity(vel);
		ent->GetPhysics()->SetAngularVelocity(avel);

		if (owner.IsValid()) {
			owner->PostEventMS(&EV_NewPod, 0, ent);
		}
	}

	idThread::ReturnInt(1);
}

void hhProjectileGasbagPod::Event_Collision_Proj(const trace_t* collision, const idVec3 &velocity) {
	idEntity *ent = gameLocal.entities[collision->c.entityNum];
	if (ent && ent->IsType(idProjectile::Type)) {
		Explode(collision, velocity, 0);
	} else {
		Event_Collision_SpawnPod(collision, velocity);
	}

	idThread::ReturnInt(0);
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build