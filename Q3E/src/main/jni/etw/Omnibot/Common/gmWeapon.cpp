#include "PrecompCommon.h"
#include "gmWeapon.h"

// class: Weapon
//		Script bindings for <Weapon>

//GMBIND_INIT_TYPE( gmWeapon, "Weapon" );
//
//GMBIND_PROPERTY_MAP_BEGIN( gmWeapon )
//	GMBIND_PROPERTY( "Name", getName, setName )
//
//	GMBIND_AUTOPROPERTY("WeaponId", GM_INT, m_WeaponID, 0);	
//	GMBIND_AUTOPROPERTY("MinUseTime", GM_FLOAT, m_MinUseTime, 0);
//
//	GMBIND_PROPERTY( "PrimaryFire", getPrimaryFire, NULL )
//	GMBIND_PROPERTY( "SecondaryFire", getSecondaryFire, NULL )
//GMBIND_PROPERTY_MAP_END();
//
//// properties: Weapon Properties
////		Name - The name of the weapon overall. <string>
////		WeaponId - The numeric id for the weapon. Must match interface. <int>
////		MinUseTime - The minimum time the bot is allowed to equip the weapon. <float>
////		PrimaryFire - Accessor to the primary fire mode. <FireMode>
////		SecondaryFire - Accessor to the secondary fire mode. <FireMode>
//
////////////////////////////////////////////////////////////////////////////
//// Property Accessors/Modifiers
//
//bool gmWeapon::getName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0] = gmVariable(a_thread->GetMachine()->AllocStringObject(a_native->GetWeaponName().c_str()));
//	return true;
//}
//
//bool gmWeapon::setName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
//	if(pStr && pStr->GetString())
//		a_native->SetWeaponName(pStr->GetString());
//	return true;
//}
//
//bool gmWeapon::getPrimaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	gmUserObject *pObj = a_native->m_FireModes[Primary].GetScriptObject(a_thread->GetMachine());
//	a_operands[0].SetUser(pObj);
//	return true;
//}
//
//bool gmWeapon::getSecondaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	gmUserObject *pObj = a_native->m_FireModes[Secondary].GetScriptObject(a_thread->GetMachine());
//	a_operands[0].SetUser(pObj);
//	return true;
//}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//// class: FireMode
////		Script bindings for <WeaponFireMode>
//
//GMBIND_INIT_TYPE( gmFireMode, "FireMode" );
//
//GMBIND_FUNCTION_MAP_BEGIN( gmFireMode )	
//	GMBIND_FUNCTION( "SetDesirabilityRange", gmfSetDesirabilityRange );
//	GMBIND_FUNCTION( "SetBurstRange", gmfSetBurstRange );
//
//	GMBIND_FUNCTION( "SetTargetBias", gmfSetTargetBias );
//	//GMBIND_FUNCTION( "SetDesirabilityCurve", gmfSetDesirabilityCurve );
//GMBIND_FUNCTION_MAP_END();
//
//GMBIND_PROPERTY_MAP_BEGIN( gmFireMode )
//	GMBIND_PROPERTY("RequiresAmmo", getRequiresAmmo, setRequiresAmmo)
//	GMBIND_PROPERTY("WaterProof", getWaterProof, setWaterProof)
//	GMBIND_PROPERTY("HasZoom", getHasZoom, setHasZoom)
//	GMBIND_PROPERTY("HasClip", getHasClip, setHasClip)
//	GMBIND_PROPERTY("Stealth", getStealth, setStealth)
//	GMBIND_PROPERTY("InheritsVelocity", getInheritsVelocity, setInheritsVelocity)	
//	GMBIND_PROPERTY("NeedsInRange", getNeedsInRange, setNeedsInRange)	
//	GMBIND_PROPERTY("ManageHeat", getManageHeat, setManageHeat)	
//	GMBIND_PROPERTY("IgnoreReload", getIgnoreReload, setIgnoreReload)	
//	GMBIND_PROPERTY("RequiresTargetOutside", getRequiresTargetOutside, setRequiresTargetOutside)
//	GMBIND_PROPERTY("RequiresShooterOutside", getRequiresShooterOutside, setRequiresShooterOutside)
//
//	GMBIND_PROPERTY("WeaponType", getWeaponType, setWeaponType)
//	//GMBIND_PROPERTY("MaxEquipMoveMode", getMaxEquipMoveMode, setMaxEquipMoveMode)
//
//	GMBIND_PROPERTY("ManualDetonation", getManualDetonation, setManualDetonation)
//	GMBIND_PROPERTY("MustBeOnGround", getMustBeOnGround, setMustBeOnGround)	
//	GMBIND_PROPERTY("FireOnRelease", getFireOnRelease, setFireOnRelease)
//	GMBIND_PROPERTY("UseMortarTrajectory", getUseMortarTrajectory, setUseMortarTrajectory)
//	GMBIND_PROPERTY("ChargeToIntercept", getChargeToIntercept, setChargeToIntercept)
//
//	GMBIND_PROPERTY("SniperWeapon", getSniperWeapon, setSniperWeapon)
//
//	GMBIND_PROPERTY("MaxAimError", getMaxAimError, setMaxAimError)
//	GMBIND_PROPERTY("AimOffset", getAimOffset, setAimOffset)
//	GMBIND_PROPERTY("PitchOffset", getPitchOffset, setPitchOffset)
//	//GMBIND_PROPERTY("MaxLeadError", getMaxLeadError, setMaxLeadError)
//
//	GMBIND_AUTOPROPERTY("ShootButton", GM_INT, m_ShootButton, 0);
//	GMBIND_AUTOPROPERTY("ZoomButton", GM_INT, m_ZoomButton, 0);
//
//	GMBIND_AUTOPROPERTY("LowAmmoThreshold", GM_INT, m_LowAmmoThreshold, 0);
//	GMBIND_AUTOPROPERTY("LowAmmoPriority", GM_FLOAT, m_LowAmmoPriority, 0);
//	GMBIND_AUTOPROPERTY("LowAmmoGetAmmoAmount", GM_INT, m_LowAmmoGetAmmoAmount, 0);
//
//	GMBIND_AUTOPROPERTY("FuseTime", GM_FLOAT, m_FuseTime, 0);
//	GMBIND_AUTOPROPERTY("ProjectileSpeed", GM_FLOAT, m_ProjectileSpeed, 0);
//	GMBIND_AUTOPROPERTY("MinRange", GM_FLOAT, m_MinRange, 0);
//	GMBIND_AUTOPROPERTY("MaxRange", GM_FLOAT, m_MaxRange, 0);
//	GMBIND_AUTOPROPERTY("MinChargeTime", GM_FLOAT, m_MinChargeTime, 0);
//	GMBIND_AUTOPROPERTY("MaxChargeTime", GM_FLOAT, m_MaxChargeTime, 0);
//	GMBIND_AUTOPROPERTY("DelayAfterFiring", GM_FLOAT, m_DelayAfterFiring, 0);
//	GMBIND_AUTOPROPERTY("ProjectileGravity", GM_FLOAT, m_ProjectileGravity, 0); // gravity multiplier?
//	GMBIND_AUTOPROPERTY("DefaultDesirability", GM_FLOAT, m_DefaultDesirability, 0);
//	GMBIND_AUTOPROPERTY("Bias", GM_FLOAT, m_WeaponBias, 0);
//
//	GMBIND_AUTOPROPERTY("MinAimAdjustmentTime", GM_FLOAT, m_MinAimAdjustmentSecs, 0);
//	GMBIND_AUTOPROPERTY("MaxAimAdjustmentTime", GM_FLOAT, m_MaxAimAdjustmentSecs, 0);
//	
//	// Functions
//	GMBIND_PROPERTY( "CalculateDefaultDesirability", NULL, setCalculateDefaultDesirability )
//	GMBIND_PROPERTY( "CalculateDesirability", NULL, setCalculateDesirability )
//	GMBIND_PROPERTY( "CalculateAimPoint", NULL, setCalculateAimPoint )
//	GMBIND_PROPERTY( "PreShoot", NULL, setPreShoot )
//
//	// Update Functions
//
//GMBIND_PROPERTY_MAP_END();
//
////////////////////////////////////////////////////////////////////////////
//// Property Accessors/Modifiers
//
//bool gmFireMode::setWeaponType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	gmStringObject *pObj = a_operands[1].GetStringObjectSafe();
//	if(pObj && pObj->GetString())
//	{
//		if(!_gmstricmp(pObj->GetString(), "melee"))
//			a_native->m_WeaponType = Weapon::Melee;
//		else if(!_gmstricmp(pObj->GetString(), "instant"))
//			a_native->m_WeaponType = Weapon::InstantHit;
//		else if(!_gmstricmp(pObj->GetString(), "projectile"))
//			a_native->m_WeaponType = Weapon::Projectile;
//		else if(!_gmstricmp(pObj->GetString(), "grenade"))
//			a_native->m_WeaponType = Weapon::Grenade;
//		else
//			Utils::OutputDebug(kError, "Invalid Weapon Type specified: %s", pObj->GetString());
//	}
//	return true;
//}
//
//bool gmFireMode::getWeaponType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	switch(a_native->m_WeaponType)
//	 {
//	 case Weapon::Melee:
//		 a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject("melee"));
//		 break;
//	 case Weapon::InstantHit:
//		 a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject("instant"));
//		 break;
//	 case Weapon::Projectile:
//		 a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject("projectile"));
//		 break;
//	 case Weapon::Grenade:
//		 a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject("grenade"));
//		 break;
//	 default:
//		 a_operands[0].Nullify();
//	 }
//	 return true;
//}
//
////bool gmFireMode::setMaxEquipMoveMode( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
////{
////	gmStringObject *pObj = a_operands[1].GetStringObjectSafe();
////	if(pObj && pObj->GetString())
////	{
////		if(!_gmstricmp(pObj->GetString(), "run"))
////			a_native->m_MaxEquipMoveMode = Weapon::Run;
////		else if(!_gmstricmp(pObj->GetString(), "walk"))
////			a_native->m_MaxEquipMoveMode = Weapon::Walk;
////		else if(!_gmstricmp(pObj->GetString(), "still"))
////			a_native->m_MaxEquipMoveMode = Weapon::Still;
////		else
////			Utils::OutputDebug(kError, "Invalid Move Mode specified: %s", pObj->GetString());
////	}
////	return true;
////}
////
////bool gmFireMode::getMaxEquipMoveMode( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
////{
////	switch(a_native->m_MaxEquipMoveMode)
////	{
////	case Weapon::Run:
////		a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject("run"));
////		break;
////	case Weapon::Walk:
////		a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject("walk"));
////		break;
////	case Weapon::Still:
////		a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject("still"));
////		break;
////	default:
////		a_operands[0].Nullify();
////	}
////	return true;
////}
//
//// properties: FireMode properties
////		WeaponType - What type of fire mode is this? <string> "melee", "projectile", "instant", "grenade"
////		AmmoType - What type of ammo does the weapon use? <int>
////		ProjectileSpeed - The speed the projectile travels. <float>
////		ProjectileGravity - Amount of gravity applied to projectile. <float>
////		MaxEquipMoveMode - The fastest we should attempt to move when equipped. <string> "run", "walk", "still"
////		RequiresAmmo - Does the weapon require ammunition to use? <bool>
////		WaterProof - Can the weapon fire when the user is underwater? <bool>
////		SplashDamage - Does the weapon produce splash damage? <bool>
////		HasZoom - Does the weapon have a zoom mode? <bool>
////		Stealth - Is this weapon preferably when the bot is attempting to remain hidden? <bool>
////		InheritsVelocity - Does the weapon projectile inherit the users velocity? <bool>
////		NeedsInRange - Should the bot attempt to stay within the Min/MaxRange properties when using? <bool>
////		MinRange - Min ranged used when <NeedsInRange> is enabled. <float>
////		MaxRange - Max ranged used when <NeedsInRange> is enabled. <float>
////		AreaEffect - Does the weapon have an area of effect? Used to reduce desirability if friendlies near target. <bool>
////		ManageHeat - Does the weapon have a heat level the bot should attempt to keep from maxing out? <bool>
////		IgnoreReload - Don't attempt to switch to this weapon to reload if ammo is low. <bool>
////		Offhand - The weapon doesn't have to be selected to be used. <bool>
////		ManualDetonation - The weapon must be detonated manually after deployment. <bool>
////		MustBeOnGround - The user must be standing on the ground to use. Can't be airborne. <bool>
////		FireOnRelease - Weapon fires when fire button is released, rather than pressed. <bool>
////		UseMortarTrajectory - Weapon should use the mortar style trajectory when aiming its projectile. <bool>
////		AimOffset - 3d offset to apply to aiming before aiming error is applied. <Vector3>
////		MaxAimError - Maximum aiming error to apply to the aim position during targeting. <Vector3>
////		ShootButton - The button to press in order to fire the weapon. <int>
////		ZoomButton - The button to press to zoom the weapons zoom mode. <int>
////		FuseTime - The time it takes the weapon to discharge when 'fired'. <int>
////		MinChargeTime - Minimum time to hold the fire button when shooting. <float>
////		MaxChargeTime - Maximum time to hold the fire button when shooting. <float>
////		DefaultDesirability - Desirability of the weapon when there is no target. <float>
////		Bias - Multiplier to use when calculating desirability for the weapon. <float>
//bool gmFireMode::setRequiresAmmo( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::RequiresAmmo, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getRequiresAmmo( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{	
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::RequiresAmmo) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setWaterProof( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::Waterproof, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getWaterProof( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::Waterproof) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setHasZoom( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::HasZoom, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getHasZoom( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::HasZoom) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setHasClip( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::HasClip, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getHasClip( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::HasClip) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setStealth( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::Stealth, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getStealth( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::Stealth) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setInheritsVelocity( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::InheritsVelocity, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getInheritsVelocity( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::InheritsVelocity) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setNeedsInRange( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::NeedsInRange, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getNeedsInRange( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::NeedsInRange) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setManualDetonation( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::ManualDetonation, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getManualDetonation( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::ManualDetonation) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setMustBeOnGround( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::MustBeOnGround, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getMustBeOnGround( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::MustBeOnGround) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setFireOnRelease( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::FireOnRelease, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getFireOnRelease( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::FireOnRelease) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setUseMortarTrajectory( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::UseMortarTrajectory, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getUseMortarTrajectory( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::UseMortarTrajectory) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setChargeToIntercept( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::ChargeToIntercept, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getChargeToIntercept( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::ChargeToIntercept) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setSniperWeapon( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::SniperWeapon, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getSniperWeapon( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::SniperWeapon) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setRequiresTargetOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::RequireTargetOutside, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getRequiresTargetOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::RequireTargetOutside) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setRequiresShooterOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::RequireShooterOutside, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getRequiresShooterOutside( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::RequireShooterOutside) ? 1 : 0);
//	return true;
//}
////////////////////////////////////////////////////////////////////////////
//
//bool gmFireMode::setManageHeat( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::ManageHeat, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getManageHeat( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::ManageHeat) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setIgnoreReload( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].IsInt())
//		a_native->SetFlag(Weapon::IgnoreReload, a_operands[1].GetInt() != 0);
//	return true;
//}
//
//bool gmFireMode::getIgnoreReload( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetInt(a_native->CheckFlag(Weapon::IgnoreReload) ? 1 : 0);
//	return true;
//}
//
//bool gmFireMode::setMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].m_type == GM_VEC3)
//	{
//		a_native->m_AimErrorMax = Vector2f(
//			a_operands[1].m_value.m_vec3.x, 
//			a_operands[1].m_value.m_vec3.y);
//	}
//	return true;
//}
//
//bool gmFireMode::getMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetVector(a_native->m_AimErrorMax.x, a_native->m_AimErrorMax.y, 0);
//	return true;
//}
//
//bool gmFireMode::setAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].m_type == GM_VEC3)
//	{
//		a_native->m_AimOffset = Vector3f(
//			a_operands[1].m_value.m_vec3.x, 
//			a_operands[1].m_value.m_vec3.y,
//			a_operands[1].m_value.m_vec3.z);
//	}
//	return true;
//}
//
//bool gmFireMode::getAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetVector(a_native->m_AimOffset.x, a_native->m_AimOffset.y, a_native->m_AimOffset.z);
//	return true;
//}
//
//bool gmFireMode::setPitchOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	float fPitchOffset = 0.f;
//	if(a_operands[1].GetFloatSafe(fPitchOffset))
//		a_native->m_PitchOffset = Mathf::DegToRad(fPitchOffset);
//	return true;
//}
//
//bool gmFireMode::getPitchOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	a_operands[0].SetFloat(a_native->m_PitchOffset);
//	return true;
//}
//
//// Function Properties
//bool gmFireMode::setCalculateDefaultDesirability( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].m_type == GM_FUNCTION)
//		a_native->m_scrCalcDefDesir.Set(a_operands[1].GetFunctionObjectSafe(), a_thread->GetMachine());
//	return true;
//}
//
//bool gmFireMode::setCalculateDesirability( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].m_type == GM_FUNCTION)
//		a_native->m_scrCalcDesir.Set(
//		a_operands[1].GetFunctionObjectSafe(), a_thread->GetMachine());
//	return true;
//}
//
//bool gmFireMode::setCalculateAimPoint( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].m_type == GM_FUNCTION)
//		a_native->m_scrCalcAimPoint.Set(
//		a_operands[1].GetFunctionObjectSafe(), a_thread->GetMachine());
//	return true;
//}
//
//bool gmFireMode::setPreShoot( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_operands[1].m_type == GM_FUNCTION)
//		a_native->m_scrPreShoot.Set(
//		a_operands[1].GetFunctionObjectSafe(), a_thread->GetMachine());
//	return true;
//}
//
//// function: SetDesirabilityRange
////		Maps a distance range to a desirability value for the weapon.
////
//// Parameters:
////
////		<float> or <int> - Minimum range
////		<float> or <int> - Maximum range
////		<float> or <int> - Desirability to use for the range provided.
////
//// Returns:
////		none
//int gmFireMode::gmfSetDesirabilityRange(gmThread *a_thread)
//{
//	GM_CHECK_NUM_PARAMS(3);
//	GM_CHECK_FLOAT_OR_INT_PARAM(windowmin, 0);
//	GM_CHECK_FLOAT_OR_INT_PARAM(windowmax, 1);
//	GM_CHECK_FLOAT_OR_INT_PARAM(desir, 2);
//	Weapon::WeaponFireMode *native = gmFireMode::GetThisObject( a_thread );
//	if(!native)
//	{
//		GM_EXCEPTION_MSG("Script Function on NULL object"); 
//		return GM_EXCEPTION;
//	}
//	native->SetDesirabilityWindow(windowmin, windowmax, desir);
//	return GM_OK;
//}
//
//// function: SetBurstRange
////		Maps a distance range to weapon burst settings.
////
//// Parameters:
////
////		<float> or <int> - Minimum range
////		<float> or <int> - Maximum range
////		<int> - Number of rounds to burst fire at the given ranges.
////		<float> or <int> - OPTIONAL - Minimum delay between bursts.
////		<float> or <int> - OPTIONAL - Maximum delay between bursts.
////
//// Returns:
////		none
//int gmFireMode::gmfSetBurstRange(gmThread *a_thread)
//{
//	GM_CHECK_NUM_PARAMS(3);
//	GM_CHECK_FLOAT_OR_INT_PARAM(windowmin, 0);
//	GM_CHECK_FLOAT_OR_INT_PARAM(windowmax, 1);
//	GM_CHECK_INT_PARAM(burstrounds, 2);
//	GM_FLOAT_OR_INT_PARAM(mindelay, 3, 1.f);
//	GM_FLOAT_OR_INT_PARAM(maxdelay, 4, 2.f);
//	Weapon::WeaponFireMode *native = gmFireMode::GetThisObject( a_thread );
//	if(!native)
//	{
//		GM_EXCEPTION_MSG("Script Function on NULL object"); 
//		return GM_EXCEPTION;
//	}
//	native->SetBurstWindow(windowmin, windowmax, burstrounds, mindelay, maxdelay);
//	return GM_OK;
//}
//
//// function: SetTargetBias
////		Sets a bias multiplier for this weapon towards certain target types..
////
//// Parameters:
////
////		<int> - Target class to set a bias for. See global CLASS table.
////		<float> or <int> - Multiplier to use for the provided class.
////
//// Returns:
////		none
//int gmFireMode::gmfSetTargetBias(gmThread *a_thread)
//{
//	Weapon::WeaponFireMode *native = gmFireMode::GetThisObject( a_thread );
//	if(!native)
//	{
//		GM_EXCEPTION_MSG("Script Function on NULL object"); 
//		return GM_EXCEPTION;
//	}
//
//	GM_CHECK_NUM_PARAMS(2);
//	GM_CHECK_INT_PARAM(targettype, 0);
//	GM_CHECK_FLOAT_OR_INT_PARAM(bias, 1);
//	native->SetTargetBias(targettype, bias);
//	return GM_OK;
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
