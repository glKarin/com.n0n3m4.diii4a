//**************************************************************************
//**
//** PREY_LIQUID.CPP
//**
//** Game code for Prey-specific dynamic liquid
//**
//** Currently using id's liquid code for the rendering
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//==========================================================================
//
// hhLiquid
//
//==========================================================================
const idEventDef EV_Disturb("disturb", "vff");

CLASS_DECLARATION( hhRenderEntity, hhLiquid )
	EVENT( EV_Touch,			hhLiquid::Event_Touch )
	EVENT( EV_Disturb,			hhLiquid::Event_Disturb )
END_CLASS

void hhLiquid::Spawn(void) {
	if (renderEntity.hModel) {
		renderEntity.hModel->Reset();
	}

	fl.takedamage = true;
	GetPhysics()->SetContents( CONTENTS_WATER | CONTENTS_TRIGGER | CONTENTS_RENDERMODEL );

	factor_movement = spawnArgs.GetFloat("factor_movement");
	factor_collide = spawnArgs.GetFloat("factor_collide");
}

void hhLiquid::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( factor_movement );
	savefile->WriteFloat( factor_collide );
}

void hhLiquid::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( factor_movement );
	savefile->ReadFloat( factor_collide );

	if (renderEntity.hModel) {
		renderEntity.hModel->Reset();
	}
}

void hhLiquid::Disturb( const idVec3 &point, const idBounds &bounds, const float magnitude ) {
	if (renderEntity.hModel) {
		idVec3 relativeToModel = point - GetPhysics()->GetOrigin();
		//FIXME: Take rotation of 'this' into account

#if 0
		hhUtils::DebugCross(colorRed, point, 5, 1000);
		gameLocal.Printf("Disturbance: Bounds={%.0f,%.0f,%.0f)(%.0f,%.0f,%.0f)  mag=%f\n",
			bounds[0].x, bounds[0].y, bounds[0].z,
			bounds[1].x, bounds[1].y, bounds[1].z,
			magnitude);
#endif

		// Pass in bounds in model coords
		renderEntity.hModel->IntersectBounds( bounds.Translate(relativeToModel), magnitude );
	}
}

void hhLiquid::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	idBounds bounds(idVec3(-1,-1,-1), idVec3(1,1,1));
	if (inflictor) {
		bounds = inflictor->GetPhysics()->GetBounds();
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if (damageDef) {
		float disturbance = damageDef->GetFloat("liquid_disturbance");
		float boundsfactor = damageDef->GetFloat("liquid_bounds_expand");
		Disturb(inflictor->GetPhysics()->GetOrigin(), bounds.Expand(boundsfactor), disturbance);
	}
}

void hhLiquid::Event_Touch(idEntity *other, trace_t *trace) {

	idVec3 vel = other->GetPhysics()->GetLinearVelocity();
	idVec3 pos = other->GetPhysics()->GetOrigin();
	bool bMoving = vel.LengthSqr() > 1.0f;
	if (bMoving) {
		//TODO: Make scale with velocity

		// Since deforming completely to the bounds is a little too much, average the
		// z position of the actor with the surface
		pos.z = (pos.z + GetOrigin().z) * 0.5f;
		Disturb(pos, other->GetPhysics()->GetBounds(), factor_movement);
	}
}

void hhLiquid::Event_Disturb(const idVec3 &position, float size, float magnitude) {
	idBounds bounds(idVec3(-0.5f,-0.5f,-0.5f), idVec3(0.5f,0.5f,0.5f));
	Disturb(position, bounds.Expand(size), magnitude);
}
