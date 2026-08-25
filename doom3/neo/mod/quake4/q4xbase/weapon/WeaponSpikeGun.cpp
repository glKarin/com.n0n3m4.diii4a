#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

class WeaponSpikeGun : public rvWeapon {
public:

	CLASS_PROTOTYPE( WeaponSpikeGun );

	virtual void			Spawn				( void );

private:
	stateResult_t		State_Raise				( const stateParms_t& parms );
	stateResult_t		State_Lower				( const stateParms_t& parms );
	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_Fire				( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( WeaponSpikeGun );
};

CLASS_DECLARATION( rvWeapon, WeaponSpikeGun )
END_CLASS

/*
================
WeaponSpikeGun::Spawn
================
*/
void WeaponSpikeGun::Spawn ( void ) {
	SetState ( "Raise", 0 );	
}

CLASS_STATES_DECLARATION ( WeaponSpikeGun )
	STATE ( "Raise",						WeaponSpikeGun::State_Raise )
	STATE ( "Lower",						WeaponSpikeGun::State_Lower )
	STATE ( "Idle",							WeaponSpikeGun::State_Idle)
	STATE ( "Fire",							WeaponSpikeGun::State_Fire )
END_CLASS_STATES

/*
================
WeaponSpikeGun::State_Raise
================
*/
stateResult_t WeaponSpikeGun::State_Raise( const stateParms_t& parms ) {
	enum {
		RAISE_INIT,
		RAISE_WAIT,
	};
	switch ( parms.stage ) {
		case RAISE_INIT:
			SetStatus ( WP_RISING );
			PlayAnim( ANIMCHANNEL_ALL, "raise", parms.blendFrames );
			return SRESULT_STAGE(RAISE_WAIT);

		case RAISE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
WeaponSpikeGun::State_Lower
================
*/
stateResult_t WeaponSpikeGun::State_Lower ( const stateParms_t& parms ) {
	enum {
		LOWER_INIT,
		LOWER_WAIT,
		LOWER_WAITRAISE
	};
	switch ( parms.stage ) {
		case LOWER_INIT:
			SetStatus ( WP_LOWERING );
			PlayAnim( ANIMCHANNEL_ALL, "putaway", parms.blendFrames );
			return SRESULT_STAGE(LOWER_WAIT);

		case LOWER_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetStatus ( WP_HOLSTERED );
				return SRESULT_STAGE(LOWER_WAITRAISE);
			}
			return SRESULT_WAIT;

		case LOWER_WAITRAISE:
			if ( wsfl.raiseWeapon ) {
				SetState ( "Raise", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
WeaponSpikeGun::State_Idle
================
*/
stateResult_t WeaponSpikeGun::State_Idle ( const stateParms_t& parms ) {
	enum {
		IDLE_INIT,
		IDLE_WAIT,
	};
	switch ( parms.stage ) {
		case IDLE_INIT:
			SetStatus ( WP_READY );
			PlayCycle( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
			return SRESULT_STAGE ( IDLE_WAIT );

		case IDLE_WAIT:
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}

			if ( !clipSize ) {
				if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoAvailable ( ) ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}
			} else {
				if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoInClip ( ) ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
WeaponSpikeGun::State_Fire
================
*/
stateResult_t WeaponSpikeGun::State_Fire ( const stateParms_t& parms ) {
	enum {
		FIRE_INIT,
		FIRE_WAIT,
	};
	switch ( parms.stage ) {
		case FIRE_INIT:
			if ( !wsfl.attack ) {
				SetState ( "Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
			Attack ( false, 1, spread, 0, 1.0f );
			PlayAnim ( ANIMCHANNEL_ALL, "fire", 0 );

			return SRESULT_STAGE(FIRE_WAIT);

		case FIRE_WAIT:
			if ( wsfl.attack && gameLocal.time >= nextAttackTime && AmmoAvailable() && !wsfl.lowerWeapon ) {
				SetState ( "Fire", 0 );
				return SRESULT_DONE;
			}
			if ( (!wsfl.attack || !AmmoAvailable() || wsfl.lowerWeapon) && AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetState ( "Idle", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
