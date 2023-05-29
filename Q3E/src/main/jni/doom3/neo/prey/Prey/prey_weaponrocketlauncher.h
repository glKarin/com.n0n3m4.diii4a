#ifndef __HH_WEAPON_ROCKETLAUNCHER_H
#define __HH_WEAPON_ROCKETLAUNCHER_H

/***********************************************************************

  hhRocketLauncherFireController
	
***********************************************************************/
class hhRocketLauncherFireController : public hhWeaponFireController {
	CLASS_PROTOTYPE(hhRocketLauncherFireController)

	public:
		ID_INLINE virtual void		EjectBrass();

	protected:
		ID_INLINE virtual bool		TransformBrass( const weaponJointHandle_t& handle, idVec3 &origin, idMat3 &axis );
};

/*
================
hhRocketLauncherFireController::EjectBrass
================
*/
ID_INLINE void hhRocketLauncherFireController::EjectBrass() {
	for( int ix = ejectJoints.Num() - 1; ix >= 0; --ix ) {
		hhWeaponFireController::EjectBrass();
	}
}

/*
================
hhRocketLauncherFireController::TransformBrass
================
*/
ID_INLINE bool hhRocketLauncherFireController::TransformBrass( const weaponJointHandle_t& handle, idVec3 &origin, idMat3 &axis ) {
	return self->GetJointWorldTransform( handle, origin, axis );
}

/***********************************************************************

  hhRocketLauncherAltFireController
	
***********************************************************************/
class hhRocketLauncherAltFireController : public hhRocketLauncherFireController {
	CLASS_PROTOTYPE(hhRocketLauncherAltFireController)

	protected:
		ID_INLINE virtual idMat3		DetermineProjectileAxis( const idMat3& muzzleAxis );
};

/*
================
hhRocketLauncherAltFireController::DetermineProjectileAxis
================
*/
ID_INLINE idMat3 hhRocketLauncherAltFireController::DetermineProjectileAxis( const idMat3& muzzleAxis ) {	
	float ang = spread * hhMath::Sqrt( gameLocal.random.RandomFloat() );
	float yaw = DEG2RAD(yawSpread);
	float spin = hhMath::TWO_PI * gameLocal.random.RandomFloat();
	idVec3 dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( hhMath::Sin(ang) * hhMath::Sin(spin) ) - muzzleAxis[ 1 ] * ( hhMath::Sin(ang+yaw) * hhMath::Cos(spin) );
	dir.Normalize();

	return dir.ToMat3();
}

/***********************************************************************

  hhWeaponRocketLauncher
	
***********************************************************************/
class hhWeaponRocketLauncher : public hhWeapon {
	CLASS_PROTOTYPE( hhWeaponRocketLauncher );

	protected:
		ID_INLINE virtual hhWeaponFireController* CreateFireController();
		ID_INLINE virtual hhWeaponFireController* CreateAltFireController();
};

/*
================
hhWeaponRocketLauncher::CreateFireController
================
*/
ID_INLINE hhWeaponFireController*	hhWeaponRocketLauncher::CreateFireController() {
	return new hhRocketLauncherFireController;
}

/*
================
hhWeaponRocketLauncher::CreateAltFireController
================
*/
ID_INLINE hhWeaponFireController* hhWeaponRocketLauncher::CreateAltFireController() {
	return new hhRocketLauncherAltFireController;
}

#endif