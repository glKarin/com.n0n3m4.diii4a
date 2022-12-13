#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define FIRE_PREDICTION_TOLERANCE 0.16f //rww - time lag to allow prediction to handle fire and reload times

CLASS_DECLARATION( hhWeaponFireController, hhHiderWeaponAltFireController )
END_CLASS

//=============================================================================
//
// hhHiderWeaponAltFireController::GetProjectileDict
//
//=============================================================================

const idDict* hhHiderWeaponAltFireController::GetProjectileDict() const
{
	switch( self->AmmoInClip() ) {
		case 1: return gameLocal.FindEntityDefDict( dict->GetString("def_projectile1"), false );
		case 2: return gameLocal.FindEntityDefDict( dict->GetString("def_projectile2"), false );
		case 3: return gameLocal.FindEntityDefDict( dict->GetString("def_projectile3"), false );
		case 4: return gameLocal.FindEntityDefDict( dict->GetString("def_projectile4"), false );
		default: return gameLocal.FindEntityDefDict( dict->GetString("def_projectile"), false );
	}
}

CLASS_DECLARATION( hhWeapon, hhWeaponHider )
END_CLASS

hhWeaponHider::hhWeaponHider() {
	nextPredictionAttack = 0.0f;
	lastPredictionAttack = 0.0f;
	nextPredictionTimeSkip = 0;
}

void hhWeaponHider::ClientPredictionThink( void ) { //rww
	hhWeapon::ClientPredictionThink();
	if (!gameLocal.isNewFrame) {
		return;
	}
	if (owner.IsValid()) {
		if (fabsf(WEAPON_NEXTATTACK-nextPredictionAttack) > FIRE_PREDICTION_TOLERANCE) { //allow some margin of error for prediction
			if (owner->entityNumber == gameLocal.localClientNum) {
				if (nextPredictionTimeSkip < gameLocal.time) {
					if (nextPredictionTimeSkip) {
						WEAPON_NEXTATTACK = nextPredictionAttack;
						nextPredictionTimeSkip = 0;
					}
					else {
						nextPredictionTimeSkip = gameLocal.time + 300;
					}
				}
			}
			else {
				if (nextPredictionTimeSkip < gameLocal.time) { //for non-local clients, sync up if we go out of range on every new prediction frame
					WEAPON_NEXTATTACK = nextPredictionAttack;
				}
				else {
					owner->forcePredictionButtons |= BUTTON_ATTACK;
				}
			}
		}
		else if (owner->entityNumber == gameLocal.localClientNum) {
			nextPredictionTimeSkip = 0;
		}
	}
}

void hhWeaponHider::WriteToSnapshot( idBitMsgDelta &msg ) const { //rww
	hhWeapon::WriteToSnapshot(msg);

	msg.WriteFloat(WEAPON_NEXTATTACK);
}
void hhWeaponHider::ReadFromSnapshot( const idBitMsgDelta &msg ) { //rww
	hhWeapon::ReadFromSnapshot(msg);

	nextPredictionAttack = msg.ReadFloat();

	if (owner.IsValid()) {
		if (owner->entityNumber != gameLocal.localClientNum) {
			if (fabsf(WEAPON_NEXTATTACK-nextPredictionAttack) > FIRE_PREDICTION_TOLERANCE) { //ensure this client "fires" his next prediction frame
				owner->forcePredictionButtons |= BUTTON_ATTACK;
				if (lastPredictionAttack != nextPredictionAttack) {
					nextPredictionTimeSkip = gameLocal.time + 300; //give a few snapshots to straighten out for the local client
				}
			}
			else {
				owner->forcePredictionButtons &= ~BUTTON_ATTACK;
			}
		}
	}

	lastPredictionAttack = nextPredictionAttack;
}
