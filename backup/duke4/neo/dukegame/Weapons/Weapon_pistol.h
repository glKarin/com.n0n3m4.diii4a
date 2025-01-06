// Weapon_pistol.h
//

#pragma once

class dnWeaponPistol : public rvmWeaponObject
{
public:
	CLASS_PROTOTYPE( dnWeaponPistol );

	virtual void			Init( idWeapon* weapon );

	stateResult_t			Raise( stateParms_t* parms );
	stateResult_t			Lower( stateParms_t* parms );
	stateResult_t			Idle( stateParms_t* parms );
	stateResult_t			Fire( stateParms_t* parms );
	stateResult_t			Reload( stateParms_t* parms );
private:
	float					spread;
	const idSoundShader*	fireSound;

	const idSoundShader*		snd_lowammo;
};