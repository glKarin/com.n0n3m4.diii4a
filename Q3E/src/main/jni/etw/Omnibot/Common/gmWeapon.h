#ifndef __GM_WEAPON_H__
#define __GM_WEAPON_H__

#include "gmBind.h"
#include "Weapon.h"

//class gmFireMode : public gmBind<Weapon::WeaponFireMode, gmFireMode>
//{
//public:
//	GMBIND_DECLARE_FUNCTIONS( );
//	GMBIND_DECLARE_PROPERTIES( );
//	
//	//////////////////////////////////////////////////////////////////////////
//	// Property Accessors
//	static bool setWeaponType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getWeaponType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	//static bool setMaxEquipMoveMode( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	//static bool getMaxEquipMoveMode( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setRequiresAmmo( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getRequiresAmmo( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setWaterProof( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getWaterProof( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setHasZoom( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getHasZoom( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setHasClip( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getHasClip( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//
//	static bool setStealth( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getStealth( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setInheritsVelocity( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getInheritsVelocity( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setNeedsInRange( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getNeedsInRange( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setManualDetonation( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getManualDetonation( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setPitchOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getPitchOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setMustBeOnGround( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getMustBeOnGround( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setFireOnRelease( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getFireOnRelease( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setManageHeat( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getManageHeat( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setIgnoreReload( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getIgnoreReload( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setUseMortarTrajectory( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getUseMortarTrajectory( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );	
//	static bool getRequiresTargetOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setRequiresTargetOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getRequiresShooterOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setRequiresShooterOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setSniperWeapon( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getSniperWeapon( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setChargeToIntercept( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getChargeToIntercept( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//
//	// Function Properties
//	static bool setCalculateDefaultDesirability( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setCalculateDesirability( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setCalculateAimPoint( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setPreShoot( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
//
//	//////////////////////////////////////////////////////////////////////////
//	// Functions
//	static int gmfSetDesirabilityRange(gmThread *a_thread);
//	static int gmfSetBurstRange(gmThread *a_thread);
//	static int gmfSetTargetBias(gmThread *a_thread);
//
//	//////////////////////////////////////////////////////////////////////////
//	static Weapon::WeaponFireMode *Constructor(gmThread *a_thread) { return 0; }
//	static void Destructor(Weapon::WeaponFireMode *_native) { delete _native; }
//};
//
//class gmWeapon : public gmBind<Weapon, gmWeapon>
//{
//public:
//	GMBIND_DECLARE_PROPERTIES( );
//
//	//////////////////////////////////////////////////////////////////////////
//	// Functions
//	
//	//////////////////////////////////////////////////////////////////////////
//	// Property Accessors
//	static bool getName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool setName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getPrimaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
//	static bool getSecondaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
//
//	//////////////////////////////////////////////////////////////////////////
//	static Weapon *Constructor(gmThread *a_thread) { return 0; }
//	static void Destructor(Weapon *_native) { delete _native; }
//};

#endif
