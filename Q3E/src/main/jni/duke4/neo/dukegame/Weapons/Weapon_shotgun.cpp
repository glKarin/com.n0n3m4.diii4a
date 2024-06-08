// Weapon_pistol.cpp
//

#pragma hdrstop

#include "../Gamelib/Game_local.h"

CLASS_DECLARATION( rvmWeaponObject, dnWeaponShotgun )
END_CLASS

#define SHOTGUN_FIRERATE		1.333
#define SHOTGUN_LOWAMMO			2
#define SHOTGUN_RELOADRATE		2
#define	SHOTGUN_NUMPROJECTILES	2

#define DNSHOTGUN_IDLE_STARTFRAME	43
#define DNSHOTGUN_IDLE_NUMFRAMES		 1

#define DNSHOTGUN_FIRE_STARTFRAME      123
#define DNSHOTGUN_FIRE_NUMFRAMES      22

#define DNSHOTGUN_RELOAD_STARTFRAME      1
#define DNSHOTGUN_RELOAD_NUMFRAMES      42

/*
===============
dnWeaponShotgun::Init
===============
*/
void dnWeaponShotgun::Init( idWeapon* weapon )
{
	rvmWeaponObject::Init( weapon );

	next_attack = 0;
	spread = weapon->GetFloat( "spread" );
	snd_lowammo = FindSound( "snd_lowammo" );

	fireSound = declManager->FindSound("shotgun_fire");
}

/*
===============
dnWeaponShotgun::Raise
===============
*/
stateResult_t dnWeaponShotgun::Raise( stateParms_t* parms )
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
			return SRESULT_DONE;
	}

	return SRESULT_ERROR;
}


/*
===============
dnWeaponShotgun::Lower
===============
*/
stateResult_t dnWeaponShotgun::Lower( stateParms_t* parms )
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
dnWeaponShotgun::Idle
===============
*/
stateResult_t dnWeaponShotgun::Idle( stateParms_t* parms )
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
			owner->PlayVertexAnimation(DNSHOTGUN_IDLE_STARTFRAME, DNSHOTGUN_IDLE_NUMFRAMES, true);
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
dnWeaponShotgun::Fire
===============
*/
stateResult_t dnWeaponShotgun::Fire( stateParms_t* parms )
{
	int ammoClip = owner->AmmoInClip();

	enum FIRE_State
	{
		FIRE_NOTSET = 0,
		FIRE_SHOOTWAIT,
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
			next_attack = gameLocal.realClientTime + SEC2MS( SHOTGUN_FIRERATE );
			owner->StartSoundShader(fireSound, SND_CHANNEL_ANY, 0, false, nullptr);
			owner->PlayVertexAnimation(DNSHOTGUN_FIRE_STARTFRAME, DNSHOTGUN_FIRE_NUMFRAMES, false);
			parms->stage = FIRE_SHOOTWAIT;
			parms->Wait(0.05f);
			return SRESULT_WAIT;

		case FIRE_SHOOTWAIT:
			owner->Event_Attack(true, "damage_shotgun", SHOTGUN_NUMPROJECTILES, spread, 0);
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
dnWeaponShotgun::Reload
===============
*/
stateResult_t dnWeaponShotgun::Reload( stateParms_t* parms )
{
	enum RELOAD_State
	{
		RELOAD_NOTSET = 0,
		RELOAD_WAIT
	};

	switch( parms->stage )
	{
		case RELOAD_NOTSET:
			owner->PlayVertexAnimation(DNSHOTGUN_RELOAD_STARTFRAME, DNSHOTGUN_RELOAD_NUMFRAMES, false);
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
