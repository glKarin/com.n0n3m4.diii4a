// weapon_fist.cpp
//

#include "../Gamelib/Game_local.h"

CLASS_DECLARATION(rvmWeaponObject, dnWeaponMightyFoot)
END_CLASS

#define DNFOOT_FIRE_STARTFRAME     30
#define DNFOOT_FIRE_NUMFRAMES      17


/*
================
dnWeaponMightyFoot::Init
================
*/
void dnWeaponMightyFoot::Init(idWeapon* weapon)
{
	rvmWeaponObject::Init(weapon);
}

/*
================
dnWeaponMightyFoot::Raise
================
*/
stateResult_t dnWeaponMightyFoot::Raise(stateParms_t* parms)
{
	return SRESULT_DONE;
}

/*
================
dnWeaponMightyFoot::Lower
================
*/
stateResult_t dnWeaponMightyFoot::Lower(stateParms_t* parms)
{
	SetState("Holstered");
	return SRESULT_DONE;
}

/*
================
dnWeaponMightyFoot::Idle
================
*/
stateResult_t dnWeaponMightyFoot::Idle(stateParms_t* parms)
{
	enum IdleState
	{
		IDLE_NOTSET = 0,
		IDLE_WAIT
	};

	switch (parms->stage)
	{
	case IDLE_NOTSET:
		//owner->Event_PlayCycle(ANIMCHANNEL_ALL, "idle");
		owner->PlayVertexAnimation(0, 1, true); // Mighty foot doesn't have a idle.
		parms->stage = IDLE_WAIT;
		return SRESULT_WAIT;

	case IDLE_WAIT:
		// Do nothing.
		return SRESULT_DONE;
	}

	return SRESULT_ERROR;
}

/*
================
dnWeaponMightyFoot::Fire
================
*/
stateResult_t dnWeaponMightyFoot::Fire(stateParms_t* parms)
{
	enum FIRE_State
	{
		FIRE_NOTSET = 0,
		FIRE_MELEE,
		FIRE_WAIT
	};

	switch (parms->stage)
	{
	case FIRE_NOTSET:
		owner->PlayVertexAnimation(DNFOOT_FIRE_STARTFRAME, DNFOOT_FIRE_NUMFRAMES, false);
		parms->stage = FIRE_MELEE;
		parms->Wait(0.1f);
		return SRESULT_WAIT;

	case FIRE_MELEE:
		owner->Event_Melee();
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
================
dnWeaponMightyFoot::Reload
================
*/
stateResult_t dnWeaponMightyFoot::Reload(stateParms_t* parms)
{
	return SRESULT_DONE;
}
