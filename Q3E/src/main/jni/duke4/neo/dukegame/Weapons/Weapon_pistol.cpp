// Weapon_pistol.cpp
//

#pragma hdrstop

#include "../Gamelib/Game_local.h"

CLASS_DECLARATION( rvmWeaponObject, dnWeaponPistol )
END_CLASS

#define DNPISTOL_IDLE_STARTFRAME	124
#define DNPISTOL_IDLE_NUMFRAMES		 1

#define DNPISTOL_FIRE_STARTFRAME      1
#define DNPISTOL_FIRE_NUMFRAMES      11

#define DNPISTOL_RELOAD_STARTFRAME      17
#define DNPISTOL_RELOAD_NUMFRAMES      48


#define PISTOL_FIRERATE			0.4
#define PISTOL_LOWAMMO			4
#define PISTOL_NUMPROJECTILES	1

// blend times
#define PISTOL_IDLE_TO_LOWER	2
#define PISTOL_IDLE_TO_FIRE		1
#define	PISTOL_IDLE_TO_RELOAD	3
#define PISTOL_RAISE_TO_IDLE	3
#define PISTOL_FIRE_TO_IDLE		4
#define PISTOL_RELOAD_TO_IDLE	40

/*
===============
dnWeaponPistol::Init
===============
*/
void dnWeaponPistol::Init( idWeapon* weapon )
{
	rvmWeaponObject::Init( weapon );

	next_attack = 0;
	spread = weapon->GetFloat( "spread" );
	snd_lowammo = FindSound( "snd_lowammo" );

	fireSound = declManager->FindSound("pistol_fire");
}

/*
===============
dnWeaponPistol::Raise
===============
*/
stateResult_t dnWeaponPistol::Raise( stateParms_t* parms )
{
	enum RisingState
	{
		RISING_NOTSET = 0,
		RISING_WAIT
	};

	switch( parms->stage )
	{
		case RISING_NOTSET:
			owner->Event_PlayAnim( ANIMCHANNEL_ALL, "raise", false );
			parms->stage = RISING_WAIT;
			return SRESULT_WAIT;

		case RISING_WAIT:
			if( owner->Event_AnimDone( ANIMCHANNEL_ALL, PISTOL_RAISE_TO_IDLE ) )
			{
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}


/*
===============
dnWeaponPistol::Lower
===============
*/
stateResult_t dnWeaponPistol::Lower( stateParms_t* parms )
{
	enum LoweringState
	{
		LOWERING_NOTSET = 0,
		LOWERING_WAIT
	};

	switch( parms->stage )
	{
		case LOWERING_NOTSET:
			owner->Event_PlayAnim( ANIMCHANNEL_ALL, "putaway", false );
			parms->stage = LOWERING_WAIT;
			return SRESULT_WAIT;

		case LOWERING_WAIT:
			if( owner->Event_AnimDone( ANIMCHANNEL_ALL, 0 ) )
			{
				SetState( "Holstered" );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}

/*
===============
dnWeaponPistol::Idle
===============
*/
stateResult_t dnWeaponPistol::Idle( stateParms_t* parms )
{
	enum IdleState
	{
		IDLE_NOTSET = 0,
		IDLE_WAIT
	};

	switch( parms->stage )
	{
		case IDLE_NOTSET:
			owner->Event_WeaponReady();
			owner->PlayVertexAnimation(DNPISTOL_IDLE_STARTFRAME, DNPISTOL_IDLE_NUMFRAMES, true);
			parms->stage = IDLE_WAIT;
			return SRESULT_WAIT;

		case IDLE_WAIT:
			// Do nothing.
			return SRESULT_DONE;
	}

	return SRESULT_ERROR;
}

/*
===============
dnWeaponPistol::Fire
===============
*/
stateResult_t dnWeaponPistol::Fire( stateParms_t* parms )
{
	int ammoClip = owner->AmmoInClip();

	enum FIRE_State
	{
		FIRE_NOTSET = 0,
		FIRE_WAIT
	};

	if( ammoClip == 0 && owner->AmmoAvailable() && parms->stage == 0 )
	{
		//owner->WeaponState( WP_RELOAD, PISTOL_IDLE_TO_RELOAD );
		owner->Reload();
		return SRESULT_DONE;
	}

	switch( parms->stage )
	{
		case FIRE_NOTSET:
			next_attack = gameLocal.realClientTime + SEC2MS( PISTOL_FIRERATE );

			if( ammoClip == PISTOL_LOWAMMO )
			{
				int length;
				owner->StartSoundShader( snd_lowammo, SND_CHANNEL_ITEM, 0, false, &length );
			}
			
			owner->Event_Attack(true, "damage_pistol", PISTOL_NUMPROJECTILES, spread, 0);

			//owner->Event_PlayAnim( ANIMCHANNEL_ALL, "fire", false );

			owner->StartSoundShader(fireSound, SND_CHANNEL_ANY, 0, false, nullptr);
			owner->PlayVertexAnimation(DNPISTOL_FIRE_STARTFRAME, DNPISTOL_FIRE_NUMFRAMES, false);
			parms->stage = FIRE_WAIT;
			return SRESULT_WAIT;

		case FIRE_WAIT:
			if( owner->IsVertexAnimDone())
			{
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}


/*
===============
dnWeaponPistol::Reload
===============
*/
stateResult_t dnWeaponPistol::Reload( stateParms_t* parms )
{
	enum RELOAD_State
	{
		RELOAD_NOTSET = 0,
		RELOAD_WAIT
	};

	switch( parms->stage )
	{
		case RELOAD_NOTSET:
			owner->PlayVertexAnimation(DNPISTOL_RELOAD_STARTFRAME, DNPISTOL_RELOAD_NUMFRAMES, false);
			parms->stage = RELOAD_WAIT;
			return SRESULT_WAIT;

		case RELOAD_WAIT:
			if (owner->IsVertexAnimDone())
			{
				owner->Event_AddToClip( owner->ClipSize() );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
