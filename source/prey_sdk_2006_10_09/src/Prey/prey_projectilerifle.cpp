#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhProjectileRifleSniper
	
***********************************************************************/
const idEventDef EV_AttemptFinalExitEventDef( "<attemptFinalExitEventDef>" );

CLASS_DECLARATION( hhProjectile, hhProjectileRifleSniper )
	EVENT( EV_AttemptFinalExitEventDef,		hhProjectileRifleSniper::Event_AttemptFinalExitEventDef )
	EVENT( EV_AllowCollision_Flesh,			hhProjectileRifleSniper::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_Wood,			hhProjectileRifleSniper::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_Glass,			hhProjectileRifleSniper::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_Pipe,			hhProjectileRifleSniper::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_Metal,			hhProjectileRifleSniper::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_AltMetal,		hhProjectileRifleSniper::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_Tile,			hhProjectileRifleSniper::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_CardBoard,		hhProjectileRifleSniper::Event_AllowCollision_PassThru )
END_CLASS

/*
=================
hhProjectileRifleSniper::Spawn
=================
*/
void hhProjectileRifleSniper::Spawn() {
	numPassThroughs = 0.0f;
	maxPassThroughs = hhMath::hhMax<float>( 1.0f, spawnArgs.GetFloat("maxPassThroughs") );
}

/*
=================
hhProjectileRifleSniper::Event_AllowCollision_PassThru
=================
*/
void hhProjectileRifleSniper::Event_AllowCollision_PassThru( const trace_t* collision ) {
	assert( collision );

	idEntity *entityHit = gameLocal.entities[ collision->c.entityNum ];

	if (!entityHit->IsType(idAI::Type)) {
		//gameLocal.Printf("STOP: [%s]\n", entityHit->GetName());
		hhProjectile::Event_AllowCollision_Collide( collision );
		return;
	}

	if( lastDamagedEntity != entityHit ) {
		lastDamagedEntity = entityHit;

		if( numPassThroughs < maxPassThroughs ) {
			idVec3 myVel = GetPhysics()->GetLinearVelocity();
			idVec3 otherVel = entityHit->GetPhysics()->GetLinearVelocity();
			idVec3 vel = myVel - otherVel;
			DamageEntityHit( collision, vel, entityHit );

			numPassThroughs = hhMath::hhMin<float>( numPassThroughs + 1.0f, maxPassThroughs );
			if( numPassThroughs >= maxPassThroughs ) {
				ProcessEvent( &EV_AttemptFinalExitEventDef );
			}
		}
	}

	//gameLocal.Printf("PASS: [%s]\n", entityHit->GetName());
	hhProjectile::Event_AllowCollision_PassThru( collision );
}

/*
=================
hhProjectileRifleSniper::DetermineDamageScale
=================
*/
float hhProjectileRifleSniper::DetermineDamageScale( const trace_t* collision ) const {
	float scale = 1.0f - (numPassThroughs / maxPassThroughs);

	return scale;
}

/*
=================
hhProjectileRifleSniper::DamageIsValid
=================
*/
bool hhProjectileRifleSniper::DamageIsValid( const trace_t* collision, float& damageScale ) {
	if( numPassThroughs < maxPassThroughs && hhProjectile::DamageIsValid(collision, damageScale) ) {
		return true;
	}

	RemoveProjectile( 0 );
	return false;
}

void hhProjectileRifleSniper::Think() {
	hhProjectile::Think();

	// Still check for projectile outside of world, since sometimes that can make it
	// through, at which point they stop simulating, but still play sound, have fx_fly, etc.
	if( !gameLocal.clip.GetWorldBounds().ContainsPoint(GetOrigin()) ) {
		RemoveProjectile( 0 );
		BecomeInactive( TH_ALL );
	}
}

/*
=================
hhProjectileRifleSniper::Event_AttemptFinalExitEventDef
=================
*/
void hhProjectileRifleSniper::Event_AttemptFinalExitEventDef() {
	//Allow the projectile to do something behind final object hit

	CancelEvents( &EV_AttemptFinalExitEventDef );
	if( lastDamagedEntity.IsValid() && !lastDamagedEntity->GetPhysics()->GetAbsBounds().IntersectsBounds(GetPhysics()->GetAbsBounds()) ) {
		CancelEvents( &EV_Remove );
		PostEventMS( GetInvalidDamageEventDef(), 0 );
		return;
	}

	PostEventMS( &EV_AttemptFinalExitEventDef, USERCMD_MSEC );
}

/*
=================
hhProjectileRifleSniper::DamageEntityHit
=================
*/
void hhProjectileRifleSniper::DamageEntityHit( const trace_t* collision, const idVec3& velocity, idEntity* entHit ) {
	float push = 0.0f;
	float damageScale = 1.0f;
	const char *damage = spawnArgs.GetString( "def_damage" );
	hhPlayer* playerHit = (entHit->IsType(hhPlayer::Type)) ? static_cast<hhPlayer*>(entHit) : NULL;
	idAFEntity_Base* afHit = (entHit->IsType(idAFEntity_Base::Type)) ? static_cast<idAFEntity_Base*>(entHit) : NULL;

	idVec3 dir = velocity.ToNormal();

	// non-radius damage defs can also apply an additional impulse to the rigid body physics impulse
	const idDeclEntityDef *def = gameLocal.FindEntityDef( damage, false );
	if ( def ) {
		if (entHit->IsType(hhProjectile::Type)) {
			push = 0.0f; // mdl:  Don't let projectiles push each other
		} else if (afHit && afHit->IsActiveAF() ) {
			push = def->dict.GetFloat( "push_ragdoll" );
		} else {
			push = def->dict.GetFloat( "push" );
		}
	}

	if (!gameLocal.isClient) { //rww
		if( playerHit ) {
			// pdm: save collision location in case we want to project a blob there
			playerHit->playerView.SetDamageLoc( collision->endpos );
		}

		if ( entHit && entHit->IsType( idAI::Type ) ) {
			idAI *aiHit = static_cast<idAI*>(entHit);
			if ( aiHit && aiHit->InVehicle() ) {
				idEntity *killer = owner.GetEntity();
				hhVehicle *vehicle = aiHit->GetVehicleInterface()->GetVehicle();
				if ( vehicle ) {
					//use a different damagedef, since we cant affect the damage amount from here directly
					damage = spawnArgs.GetString( "def_pilotdamage" );
					vehicle->Damage( this, killer, dir, damage, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE(collision->c.id) );
				}
			}
		} 
		
		if( DamageIsValid(collision, damageScale) && entHit->fl.takedamage ) {
			UpdateBalanceInfo( collision, entHit );

			if( damage && damage[0] ) {
				idEntity *killer = owner.GetEntity();
				if (killer && killer->IsType(hhVehicle::Type)) { //rww - handle vehicle projectiles killing people
					hhVehicle *veh = static_cast<hhVehicle *>(killer);
					if (veh->GetPilot()) {
						killer = veh->GetPilot();
					}
				}

				entHit->Damage( this, killer, dir, damage, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE(collision->c.id) );

				if ( playerHit && def->dict.GetInt( "freeze_duration" ) > 0 ) {
					playerHit->Freeze( def->dict.GetInt( "freeze_duration" ) );
				}
			}
		}

		// HUMANHEAD bjk: moved to after damage so impulse can be applied to ragdoll
		if ( push > 0.0f ) {
			if (g_debugImpulse.GetBool()) {
				gameRenderWorld->DebugArrow(colorYellow, collision->c.point, collision->c.point + (push*dir), 25, 2000);
			}

			entHit->ApplyImpulse( this, collision->c.id, collision->c.point, push * dir );
		}
	}

	if ( entHit->fl.applyDamageEffects ) {
		ApplyDamageEffect( entHit, collision, velocity, damage );
	}
}

/*
================
hhProjectileRifleSniper::Save
================
*/
void hhProjectileRifleSniper::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( numPassThroughs );
	savefile->WriteFloat( maxPassThroughs );
	lastDamagedEntity.Save( savefile );
}

/*
================
hhProjectileRifleSniper::Restore
================
*/
void hhProjectileRifleSniper::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( numPassThroughs );
	savefile->ReadFloat( maxPassThroughs );
	lastDamagedEntity.Restore( savefile );
}

