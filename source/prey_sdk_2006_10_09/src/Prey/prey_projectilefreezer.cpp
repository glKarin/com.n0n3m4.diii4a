#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhProjectileFreezer
	
***********************************************************************/
CLASS_DECLARATION( hhProjectile, hhProjectileFreezer )
	//EVENT( EV_Touch,		hhProjectileFreezer::Event_Touch )
	
	EVENT( EV_Collision_Flesh,			hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Metal,			hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_AltMetal,		hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Wood,			hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Stone,			hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Glass,			hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_CardBoard,		hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Tile,			hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Forcefield,		hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Pipe,			hhProjectileFreezer::Event_Collision_Bounce )
	EVENT( EV_Collision_Wallwalk,		hhProjectileFreezer::Event_Collision_Bounce )
END_CLASS

void hhProjectileFreezer::Spawn() {
	decelStart = SEC2MS( spawnArgs.GetFloat("decelStart") ) + gameLocal.GetTime();
	decelEnd = SEC2MS( spawnArgs.GetFloat("decelDuration") ) + decelStart;

	collided=false;

	BecomeActive( TH_TICKER );
}

void hhProjectileFreezer::Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	hhProjectile::Launch( start, axis, pushVelocity, timeSinceFire, launchPower, dmgPower );

	cachedVelocity = GetPhysics()->GetLinearVelocity();

	//fl.takedamage = false;
	//physicsObj.DisableImpact();
}

void hhProjectileFreezer::Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity ) {
	idEntity *entityHit = gameLocal.entities[ collision->c.entityNum ];
	if ( entityHit->IsType(idAI::Type) || entityHit->IsType(idAFEntity_Base::Type) ) {
		Event_Collision_Explode(collision, velocity);
		return;
	}

	ProcessCollision(collision, velocity);
	collided=true;
	idThread::ReturnInt( 1 );
}

bool hhProjectileFreezer::Collide( const trace_t& collision, const idVec3& velocity ) {
	if(!collided)
		return hhProjectile::Collide( collision, velocity );
	else
		return false;
}


int hhProjectileFreezer::ProcessCollision( const trace_t* collision, const idVec3& velocity ) {
	idEntity* entHit = gameLocal.entities[ collision->c.entityNum ];

	SetOrigin( collision->endpos );
	SetAxis( collision->endAxis );

	if (entHit) { //rww - may be null on client.
		DamageEntityHit( collision, velocity, entHit );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	surfTypes_t matterType = gameLocal.GetMatterType( entHit, collision->c.material, "hhProjectile::ProcessCollision" );
	return PlayImpactSound( gameLocal.FindEntityDefDict(spawnArgs.GetString("def_damage")), collision->endpos, matterType );
}

void hhProjectileFreezer::Ticker() {
	float scale = 0.0f;
	if( gameLocal.GetTime() > decelStart && gameLocal.GetTime() < decelEnd ) {
		scale = hhMath::Sin( DEG2RAD(hhMath::MidPointLerp( 0.0f, 30.0f, 90.0f, 1.0f - hhUtils::CalculateScale(gameLocal.GetTime(), decelStart, decelEnd))) );

		GetPhysics()->SetLinearVelocity( cachedVelocity * hhMath::ClampFloat(0.05f, 1.0f, scale) );
	}
}

void hhProjectileFreezer::Event_Touch( idEntity *other, trace_t *trace ) {
	//Supposed to be empty

}

void hhProjectileFreezer::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( decelStart );
	savefile->WriteInt( decelEnd );
	savefile->WriteVec3( cachedVelocity );
	savefile->WriteBool( collided );
}

void hhProjectileFreezer::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( decelStart );
	savefile->ReadInt( decelEnd );
	savefile->ReadVec3( cachedVelocity );
	savefile->ReadBool( collided );
}

void hhProjectileFreezer::ApplyDamageEffect( idEntity* hitEnt, const trace_t* collision, const idVec3& velocity, const char* damageDefName ) {
	if( hitEnt && gameLocal.random.RandomFloat() > 0.6f ) {
		hitEnt->AddDamageEffect( *collision, velocity, damageDefName, (!fl.networkSync || netSyncPhysics) );
	}
}