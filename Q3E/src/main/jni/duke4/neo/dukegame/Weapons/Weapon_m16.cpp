// Weapon_m16.cpp
//

#pragma hdrstop

#include "../Gamelib/Game_local.h"

CLASS_DECLARATION(rvmWeaponObject, dnWeaponM16)
END_CLASS

#define DNM16_IDLE_STARTFRAME	130
#define DNM16_IDLE_NUMFRAMES		 1

#define DNM16_FIRE_STARTFRAME     130
#define DNM16_FIRE_NUMFRAMES      10

#define DNM16_RELOAD_STARTFRAME     141
#define DNM16_RELOAD_NUMFRAMES      43


#define M16_FIRERATE			0.4
#define M16_LOWAMMO			4
#define M16_NUMPROJECTILES	1

// blend times
#define M16_IDLE_TO_LOWER	2
#define M16_IDLE_TO_FIRE		1
#define	M16_IDLE_TO_RELOAD	3
#define M16_RAISE_TO_IDLE	3
#define M16_FIRE_TO_IDLE		4
#define M16_RELOAD_TO_IDLE	40

/*
===============
dnWeaponM16::Init
===============
*/
void dnWeaponM16::Init(idWeapon* weapon)
{
	rvmWeaponObject::Init(weapon);

	next_attack = 0;
	spread = weapon->GetFloat("spread");
	snd_lowammo = FindSound("snd_lowammo");

	fireSound = declManager->FindSound("M16_fire");
}

/*
===============
dnWeaponM16::Raise
===============
*/
stateResult_t dnWeaponM16::Raise(stateParms_t* parms)
{
	enum RisingState
	{
		RISING_NOTSET = 0,
		RISING_WAIT
	};

	switch (parms->stage)
	{
	case RISING_NOTSET:
		owner->Event_PlayAnim(ANIMCHANNEL_ALL, "raise", false);
		parms->stage = RISING_WAIT;
		return SRESULT_WAIT;

	case RISING_WAIT:
		if (owner->Event_AnimDone(ANIMCHANNEL_ALL, M16_RAISE_TO_IDLE))
		{
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}


/*
===============
dnWeaponM16::Lower
===============
*/
stateResult_t dnWeaponM16::Lower(stateParms_t* parms)
{
	enum LoweringState
	{
		LOWERING_NOTSET = 0,
		LOWERING_WAIT
	};

	switch (parms->stage)
	{
	case LOWERING_NOTSET:
		owner->Event_PlayAnim(ANIMCHANNEL_ALL, "putaway", false);
		parms->stage = LOWERING_WAIT;
		return SRESULT_WAIT;

	case LOWERING_WAIT:
		if (owner->Event_AnimDone(ANIMCHANNEL_ALL, 0))
		{
			SetState("Holstered");
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}

/*
===============
dnWeaponM16::Idle
===============
*/
stateResult_t dnWeaponM16::Idle(stateParms_t* parms)
{
	enum IdleState
	{
		IDLE_NOTSET = 0,
		IDLE_WAIT
	};

	switch (parms->stage)
	{
	case IDLE_NOTSET:
		owner->Event_WeaponReady();
		owner->PlayVertexAnimation(DNM16_IDLE_STARTFRAME, DNM16_IDLE_NUMFRAMES, true);
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
dnWeaponM16::Fire
===============
*/
stateResult_t dnWeaponM16::Fire(stateParms_t* parms)
{
	int ammoClip = owner->AmmoInClip();

	enum FIRE_State
	{
		FIRE_NOTSET = 0,
		FIRE_WAIT
	};

	if (ammoClip == 0 && owner->AmmoAvailable() && parms->stage == 0)
	{
		//owner->WeaponState( WP_RELOAD, M16_IDLE_TO_RELOAD );
		owner->Reload();
		return SRESULT_DONE;
	}

	switch (parms->stage)
	{
	case FIRE_NOTSET:
		next_attack = gameLocal.realClientTime + SEC2MS(M16_FIRERATE);

		if (ammoClip == M16_LOWAMMO)
		{
			int length;
			owner->StartSoundShader(snd_lowammo, SND_CHANNEL_ITEM, 0, false, &length);
		}

		owner->Event_Attack(true, "damage_pistol", M16_NUMPROJECTILES, spread, 0);

		//owner->Event_PlayAnim( ANIMCHANNEL_ALL, "fire", false );

		owner->StartSoundShader(fireSound, SND_CHANNEL_ANY, 0, false, nullptr);
		owner->PlayVertexAnimation(DNM16_FIRE_STARTFRAME, DNM16_FIRE_NUMFRAMES, false);
		parms->stage = FIRE_WAIT;
		return SRESULT_WAIT;

	case FIRE_WAIT:
		if (owner->IsVertexAnimDone())
		{
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}


/*
===============
dnWeaponM16::Reload
===============
*/
stateResult_t dnWeaponM16::Reload(stateParms_t* parms)
{
	enum RELOAD_State
	{
		RELOAD_NOTSET = 0,
		RELOAD_WAIT
	};

	switch (parms->stage)
	{
	case RELOAD_NOTSET:
		owner->PlayVertexAnimation(DNM16_RELOAD_STARTFRAME, DNM16_RELOAD_NUMFRAMES, false);
		parms->stage = RELOAD_WAIT;
		return SRESULT_WAIT;

	case RELOAD_WAIT:
		if (owner->IsVertexAnimDone())
		{
			owner->Event_AddToClip(owner->ClipSize());
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
