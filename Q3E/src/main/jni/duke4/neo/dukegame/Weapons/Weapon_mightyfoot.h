// Weapon_mightyfoot.h
//

#pragma once

class dnWeaponMightyFoot : public rvmWeaponObject
{
public:
	CLASS_PROTOTYPE(dnWeaponMightyFoot);

	virtual void			Init(idWeapon* weapon);

	stateResult_t			Raise(stateParms_t* parms);
	stateResult_t			Lower(stateParms_t* parms);
	stateResult_t			Idle(stateParms_t* parms);
	stateResult_t			Fire(stateParms_t* parms);
	stateResult_t			Reload(stateParms_t* parms);

};