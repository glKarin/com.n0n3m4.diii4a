#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define DEFAULT_SPORE_RESPAWN		15000

const idEventDef EV_RespawnSpore( "<respawnspore>" );

CLASS_DECLARATION( idEntity, hhHealthSpore )
	EVENT( EV_Touch,				hhHealthSpore::Event_Touch )
	EVENT( EV_RespawnSpore,			hhHealthSpore::Event_RespawnSpore )
END_CLASS

/*
================
hhHealthSpore::Spawn
================
*/
void hhHealthSpore::Spawn() {
	if ( g_wicked.GetBool() ) { // CJR:  Don't spawn health spores in wicked mode
		PostEventMS( &EV_Remove, 0 );
	}

	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	SetShaderParm( SHADERPARM_MODE, 1.0f ); // Enable the additive glow on the spore

	fl.networkSync = true;
}

/*
================
hhHealthSpore::Event_RespawnSpore
================
*/
void hhHealthSpore::Event_RespawnSpore() {
	if (gameLocal.isClient) {
		return;
	}

	GetPhysics()->SetContents( CONTENTS_TRIGGER ); //restore contents
	SetShaderParm( SHADERPARM_MODE, 1.0f ); //restore additive pass
}

void hhHealthSpore::Save(idSaveGame *savefile) const {
}

void hhHealthSpore::Restore( idRestoreGame *savefile ) {
}

void hhHealthSpore::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_MODE]);
}

void hhHealthSpore::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	renderEntity.shaderParms[SHADERPARM_MODE] = msg.ReadFloat();
	UpdateVisuals();
}

/*
================
hhHealthSpore::ApplyEffect
================
*/
void hhHealthSpore::ApplyEffect( idActor* pActor ) {
	if( pActor ) {
		int oldHealth = pActor->health;
		const char *itemHealthKey = "health";
		if (gameLocal.isMultiplayer) { //rww
			itemHealthKey = "health_mp";
		}
		pActor->Give( "health", spawnArgs.GetString(itemHealthKey) );
	}
}

/*
================
hhHealthSpore::Event_Touch
================
*/
void hhHealthSpore::Event_Touch( idEntity* pOther, trace_t* pTraceInfo ) {
	hhFxInfo fxInfo;
	idActor* pActor = NULL;

	if( pOther && pOther->IsType(idActor::Type) ) {
		pActor = static_cast<idActor*>( pOther );
		
		//rww - do not go above 100 in mp, even when maxhealth has been raised
		if (gameLocal.isMultiplayer && pActor->IsType(hhPlayer::Type) && pActor->health >= MAX_HEALTH_NORMAL_MP) {
			return;
		}

		if( !pActor->IsDamaged() || pActor->health <= 0 ) {
			return;
		}
	}
			
	fxInfo.SetNormal( GetAxis()[2] );
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_detonate", GetOrigin(), GetAxis(), &fxInfo );

	SetShaderParm( SHADERPARM_MODE, 0.0f ); // Disable the additive glow on the spore

	ApplyEffect( pActor );

	ActivateTargets( pActor );

	//rww - broadcast
	StartSound( "snd_explode", SND_CHANNEL_ANY, 0, true );

	GetPhysics()->SetContents( 0 );		//MDC - clear our contents, so we can not get re-touched.
	fl.refreshReactions = false; // JRM - no since telling AI anymore

	if (gameLocal.isMultiplayer) { //rww - respawn functionality for mp
		int respawnTime;
		if (!spawnArgs.GetInt("respawn", "0", respawnTime)) {
			respawnTime = DEFAULT_SPORE_RESPAWN;
		}
		PostEventMS(&EV_RespawnSpore, respawnTime);
	}
}