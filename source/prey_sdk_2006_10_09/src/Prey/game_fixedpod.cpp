// Game_FixedPod.cpp
//
// non-moving exploding pod

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_ExplodedBy("<explodedby>", "e");
const idEventDef EV_SpawnShrapnel("<spawnDebris>");

CLASS_DECLARATION(idEntity, hhFixedPod)
	EVENT( EV_Activate,	   			hhFixedPod::Event_Trigger )
	EVENT( EV_ExplodeDamage,		hhFixedPod::Event_ExplodeDamage )
	EVENT( EV_ExplodedBy,			hhFixedPod::Event_ExplodedBy )
	EVENT( EV_SpawnShrapnel,		hhFixedPod::Event_SpawnDebris )
	EVENT( EV_Broadcast_AssignFx,	hhFixedPod::Event_AssignFx )
END_CLASS


hhFixedPod::hhFixedPod() {
	fx = NULL;
}

//==========================================================================
//
// hhFixedPod::Spawn
//
//==========================================================================
void hhFixedPod::Spawn(void) {

	fl.takedamage = true;

	// setup the clipModel
	GetPhysics()->SetContents( CONTENTS_SOLID );

	// Spawn the energy beam
	hhBeamSystem *beam = hhBeamSystem::SpawnBeam( GetOrigin(), spawnArgs.GetString("beam") );
	if( beam ) {
		beam->SetOrigin(GetPhysics()->GetOrigin() - GetPhysics()->GetAxis()[2]*28);
		beam->SetAxis(GetPhysics()->GetAxis());
		if (IsBound() || spawnArgs.GetBool("force_bind")) {
			beam->Bind(this, false);	// Only bind if we will be moving
		}
		beam->SetTargetLocation(beam->GetPhysics()->GetOrigin() + GetPhysics()->GetAxis()[2] * 56);
		// Bound entities automatically removed upon destruction
	}

	hhFxInfo fxInfo;
	if (IsBound() || spawnArgs.GetBool("force_bind")) {
		fxInfo.SetEntity( this );	// Only bind if we will be moving
	}
	fxInfo.RemoveWhenDone( false );
	fxInfo.NoRemoveWhenUnbound( true );
	BroadcastFxInfo( spawnArgs.GetString("fx_energybeam"), GetOrigin(), GetAxis(), &fxInfo, &EV_Broadcast_AssignFx );

	StartSound( "snd_idle", SND_CHANNEL_IDLE, 0, true, NULL );	
}

//==========================================================================
//
// hhFixedPod::Event_AssignFxSmoke
//
//==========================================================================
void hhFixedPod::Event_AssignFx( hhEntityFx* fx ) {
	this->fx = fx;
}

void hhFixedPod::Save(idSaveGame *savefile) const {
	fx.Save(savefile);
}

void hhFixedPod::Restore( idRestoreGame *savefile ) {
	fx.Restore(savefile);
}

//==========================================================================
//
// hhFixedPod::Killed
//
//==========================================================================
void hhFixedPod::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	fl.takedamage = false;

	// Activate targets
	ActivateTargets( attacker );

	GetPhysics()->SetContents( 0 );

	// Need to post an event because already in physics code now, can't nest a projectile spawn from within physics code
	// currently, because rigid body physics ::Evaluate is not reentrant friendly (has a static timer)
	PostEventMS( &EV_ExplodedBy, 0, attacker );
}


//==========================================================================
///
// hhFixedPod::Explode
//
//==========================================================================
void hhFixedPod::Explode( idEntity *attacker ) {
	hhFxInfo fxInfo;

	if (fx.IsValid()) {
		fx->Hide();
		fx->PostEventMS(&EV_Remove, 0);
	}

	// Spawn explosion
	StopSound( SND_CHANNEL_IDLE, true );
	StartSound( "snd_explode", SND_CHANNEL_ANY, 0, true, NULL );

	fxInfo.SetNormal( idVec3(0.0f, 0.0f, 1.0f) );
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_detonate", GetOrigin(), GetAxis(), &fxInfo );
	
	Hide();

	PostEventMS( &EV_SpawnShrapnel, 10 );
	PostEventMS( &EV_ExplodeDamage, 250, attacker );	// NOTE:  This even MUST occur before the remove event
	PostEventMS( &EV_Remove, 500 );						// Remove after a small delay to allow sound commands to execute
}

//==========================================================================
//
// hhFixedPod::SpawnDebris
//
//==========================================================================
void hhFixedPod::SpawnDebris() {
	idEntity* ent = NULL;
	idVec3 launchDir;
	int amount = 0;
	idDebris* debris = NULL;
	const idDict *dict = NULL;

	int numShrapnel = spawnArgs.GetInt( "debris_count" );
	if( !numShrapnel ) {
		return;
	}
			
	for( const idKeyValue* kv = spawnArgs.MatchPrefix("def_debris", NULL); kv; kv = spawnArgs.MatchPrefix("def_debris", kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}

		dict = gameLocal.FindEntityDefDict( kv->GetValue().c_str(), false );
		if( !dict ) {
			continue;
		}

		amount = hhMath::hhMax( 1, gameLocal.random.RandomInt(numShrapnel) );
		for ( int i = 0; i < amount; i++ ) {
			launchDir = hhUtils::RandomVector();
			launchDir.Normalize();
	
			gameLocal.SpawnEntityDef( *dict, &ent );
			if ( !ent || !ent->IsType( idDebris::Type ) ) {
				gameLocal.Error( "'%s' is not an idDebris", kv->GetValue().c_str() );
			}

			debris = static_cast<idDebris *>(ent);
			debris->Create( this, GetOrigin(), launchDir.ToMat3() );
			debris->Launch();
		}
	}
}

//==========================================================================
//
// hhFixedPod::Event_ExplodeDamage
//
// Applies the radius damage slightly after the actual explosion, so that
// when one explodes it will cascade and explode other fixed pods.
//==========================================================================

void hhFixedPod::Event_ExplodeDamage( idEntity *attacker ) {
	gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, attacker, this, this, spawnArgs.GetString("def_explodedamage") );
}

//==========================================================================
//
// hhFixedPod::Event_ExplodedBy
//
//==========================================================================
void hhFixedPod::Event_ExplodedBy( idEntity *activator ) {
	Explode(activator);
}

//==========================================================================
//
// hhFixedPod::Event_Trigger
//
//==========================================================================

void hhFixedPod::Event_Trigger( idEntity *activator ) {
	Explode(activator);
}

//==========================================================================
//
// hhFixedPod::Event_SpawnDebris
//
//==========================================================================

void hhFixedPod::Event_SpawnDebris() {
	SpawnDebris();
}

