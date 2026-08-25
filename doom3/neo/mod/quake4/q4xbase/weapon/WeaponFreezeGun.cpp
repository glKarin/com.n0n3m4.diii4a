#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

class rvWeaponFreezeGun : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponFreezeGun );

	virtual void			Spawn				( void );

private:
	stateResult_t		State_Raise		( const stateParms_t& parms );
	stateResult_t		State_Lower		( const stateParms_t& parms );
	stateResult_t		State_Idle		( const stateParms_t& parms );
	stateResult_t		State_Fire		( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvWeaponFreezeGun );
};

CLASS_DECLARATION( rvWeapon, rvWeaponFreezeGun )
END_CLASS

/*
================
rvWeaponFreezeGun::Spawn
================
*/
void rvWeaponFreezeGun::Spawn ( void ) {
	SetState ( "Raise", 0 );	
}

CLASS_STATES_DECLARATION ( rvWeaponFreezeGun )
	STATE ( "Raise",						rvWeaponFreezeGun::State_Raise )
	STATE ( "Lower",						rvWeaponFreezeGun::State_Lower )
	STATE ( "Idle",							rvWeaponFreezeGun::State_Idle)
	STATE ( "Fire",							rvWeaponFreezeGun::State_Fire )
END_CLASS_STATES

/*
================
rvWeaponFreezeGun::State_Raise

Raise the weapon
================
*/
stateResult_t rvWeaponFreezeGun::State_Raise ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		// Start the weapon raising
		case STAGE_INIT:
			SetStatus ( WP_RISING );
			PlayAnim( ANIMCHANNEL_ALL, "raise", 0 );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
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
rvWeaponFreezeGun::State_Lower

Lower the weapon
================
*/
stateResult_t rvWeaponFreezeGun::State_Lower ( const stateParms_t& parms ) {	
	enum {
		STAGE_INIT,
		STAGE_WAIT,
		STAGE_WAITRAISE
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			SetStatus ( WP_LOWERING );
			PlayAnim ( ANIMCHANNEL_ALL, "putaway", parms.blendFrames );
			return SRESULT_STAGE(STAGE_WAIT);
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetStatus ( WP_HOLSTERED );
				return SRESULT_STAGE(STAGE_WAITRAISE);
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITRAISE:
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
rvWeaponFreezeGun::State_Idle

Manage the idle state of the weapon
================
*/
stateResult_t rvWeaponFreezeGun::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !AmmoAvailable ( ) ) {
				SetStatus ( WP_OUTOFAMMO );
			} else {
				SetStatus ( WP_READY );
			}			
				
			PlayCycle( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}

			if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoAvailable ( ) ) {
				SetState ( "Fire", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponFreezeGun::State_Fire

Fire the weapon
================
*/
stateResult_t rvWeaponFreezeGun::State_Fire( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !wsfl.attack ) {
				SetState ( "Idle", parms.blendFrames );				
				return SRESULT_DONE;
			}
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
			Attack ( false, 1, spread, 0, 1.0f );
			PlayAnim ( ANIMCHANNEL_ALL, "fire", 0 );	
			
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:		
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
