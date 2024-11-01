////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompBM.h"
#include "gmConfig.h"
#include "gmBot.h"
#include "gmBMBinds.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmGameEntity.h"
#include "gmBotLibrary.h"

#define CHECK_THIS_BOT() \
	BM_Client *native = static_cast<BM_Client*>(gmBot::GetThisObject( a_thread )); \
	if(!native) \
	{ \
	GM_EXCEPTION_MSG("Script Function on NULL object"); \
	return GM_EXCEPTION; \
	}

#define VERIFY_MODULE_ID(id) if(id<0 || id >= BM_MODULE_MAX) { \
		GM_EXCEPTION_MSG("Invalid Module Id!"); \
		return GM_EXCEPTION; }

#define VERIFY_UPGRADE_ID(id) if(id<0 || id >= BM_UPGRADE_MAX) { \
	GM_EXCEPTION_MSG("Invalid Module Id!"); \
	return GM_EXCEPTION; }

// Title: BM Script Bindings
//////////////////////////////////////////////////////////////////////////

// Function: GetModuleStats
//static int GM_CDECL gmfGetModuleStats(gmThread *a_thread)
//{
//	CHECK_THIS_BOT();
//	GM_CHECK_NUM_PARAMS(1);
//	GM_CHECK_INT_PARAM(moduleType,0);
//	GM_TABLE_PARAM(statTable,1,0);
//
//	VERIFY_MODULE_ID(moduleType);
//
//	if(!statTable)
//		a_thread->GetMachine()->AllocTableObject();
//	const BM_ModuleStats &modStats = native->GetModuleStats();
//	statTable->Set(a_thread->GetMachine(),"Lvl",gmVariable(modStats.m_Module[moduleType].m_Lvl));
//	statTable->Set(a_thread->GetMachine(),"MaxLvl",gmVariable(modStats.m_Module[moduleType].m_MaxLvl));
//	statTable->Set(a_thread->GetMachine(),"UpgradeCost",gmVariable(modStats.m_Module[moduleType].m_UpgradeCost));
//	statTable->Set(a_thread->GetMachine(),"AuxDrain",gmVariable(modStats.m_Module[moduleType].m_AuxDrain));
//	statTable->Set(a_thread->GetMachine(),"Cooldown",gmVariable(modStats.m_Module[moduleType].m_Cooldown));
//	a_thread->PushTable(statTable);
//	return GM_OK;
//}
//
////////////////////////////////////////////////////////////////////////////
//
//// Function: UpgradeModule
//static int GM_CDECL gmfUpgradeModule(gmThread *a_thread)
//{
//	CHECK_THIS_BOT();
//	GM_CHECK_NUM_PARAMS(1);
//	GM_CHECK_INT_PARAM(moduleType,0);
//
//	VERIFY_MODULE_ID(moduleType);
//
//	a_thread->PushInt(InterfaceFuncs::UpgradeModule(native->GetGameEntity(),moduleType)?1:0);
//	return GM_OK;
//}
//
////////////////////////////////////////////////////////////////////////////
//
//// Function: GetWeaponUpgrade
//static int GM_CDECL gmfGetWeaponUpgrade(gmThread *a_thread)
//{
//	CHECK_THIS_BOT();
//	GM_CHECK_NUM_PARAMS(1);
//	GM_CHECK_INT_PARAM(upgradeType,0);
//	GM_TABLE_PARAM(statTable,1,0);
//
//	VERIFY_UPGRADE_ID(upgradeType);
//
//	if(!statTable)
//		a_thread->GetMachine()->AllocTableObject();
//	const BM_WeaponUpgradeStats &upStats = native->GetUpgradeStats();
//	statTable->Set(a_thread->GetMachine(),"Lvl",gmVariable(upStats.m_Upgrade[upgradeType].m_Lvl));
//	statTable->Set(a_thread->GetMachine(),"MaxLvl",gmVariable(upStats.m_Upgrade[upgradeType].m_MaxLvl));
//	statTable->Set(a_thread->GetMachine(),"UpgradeCost",gmVariable(upStats.m_Upgrade[upgradeType].m_UpgradeCost));
//	a_thread->PushTable(statTable);
//	return GM_OK;
//}
//
////////////////////////////////////////////////////////////////////////////
//
//// Function: UpgradeModule
//static int GM_CDECL gmfUpgradeWeapon(gmThread *a_thread)
//{
//	CHECK_THIS_BOT();
//	GM_CHECK_NUM_PARAMS(1);
//	GM_CHECK_INT_PARAM(upgradeType,0);
//
//	VERIFY_UPGRADE_ID(upgradeType);
//
//	a_thread->PushInt(InterfaceFuncs::UpgradeWeapon(native->GetGameEntity(),upgradeType)?1:0);
//	return GM_OK;
//}
//
////////////////////////////////////////////////////////////////////////////
//
//// Function: CanPhysPickup
//static int GM_CDECL gmfCanPhysPickup(gmThread *a_thread)
//{
//	CHECK_THIS_BOT();
//	GM_CHECK_NUM_PARAMS(1);
//	GameEntity pickup;
//	GM_CHECK_GAMEENTITY_FROM_PARAM(pickup, 0);	
//
//	a_thread->PushInt(InterfaceFuncs::CanPhysPickup(native->GetGameEntity(),pickup)?1:0);
//	return GM_OK;
//}
//
////////////////////////////////////////////////////////////////////////////
//
//// Function: GetPhysGunInfo
//static int GM_CDECL gmfPhysGunInfo(gmThread *a_thread)
//{
//	CHECK_THIS_BOT();
//	GM_CHECK_NUM_PARAMS(1);
//	GM_TABLE_PARAM(statTable,0,0);
//
//	BM_PhysGunInfo info = {};
//	if(InterfaceFuncs::GetPhysGunInfo(native->GetGameEntity(),info))
//	{
//		if(!statTable)
//			a_thread->GetMachine()->AllocTableObject();
//
//		statTable->Set(a_thread->GetMachine(),"HeldObject",gmVariable::EntityVar(info.m_HeldEntity.AsInt()));
//		//statTable->Set(a_thread->GetMachine(),"PullObject",gmVariable::EntityVar(info.m_PullingEntity.AsInt()));
//		statTable->Set(a_thread->GetMachine(),"LaunchSpeed",gmVariable(info.m_LaunchSpeed));
//		a_thread->PushTable(statTable);
//	}
//	return GM_OK;
//}
//
////////////////////////////////////////////////////////////////////////////
//
//// Function: GetChargerStatus
//static int GM_CDECL gmfGetChargerStatus(gmThread *a_thread)
//{
//	GM_CHECK_NUM_PARAMS(1);
//	GameEntity unit;
//	GM_CHECK_GAMEENTITY_FROM_PARAM(unit, 0);
//	GM_TABLE_PARAM(statTable,1,0);
//
//	BM_ChargerStatus info = {};
//	if(InterfaceFuncs::GetChargerStatus(unit,info))
//	{
//		if(!statTable)
//			a_thread->GetMachine()->AllocTableObject();
//
//		statTable->Set(a_thread->GetMachine(),"Charge",gmVariable(info.m_CurrentCharge));
//		statTable->Set(a_thread->GetMachine(),"MaxCharge",gmVariable(info.m_MaxCharge));
//		a_thread->PushTable(statTable);
//	}
//	return GM_OK;
//}
//
////////////////////////////////////////////////////////////////////////////
//// package: Modular Combat Bot Library Functions
//static gmFunctionEntry s_BMbotLib[] = 
//{ 
////	{"LockPlayerPosition",		gmfLockPlayerPosition},
////	{"HudHint",					gmfHudHint},
////	{"HudMessage",				gmfHudTextBox},	
////	{"HudAlert",				gmfHudAlert},	
////	{"HudMenu",					gmfHudMenu},	
////	{"HudTextMsg",				gmfHudTextMsg},
//
//	{"GetChargerStatus",		gmfGetChargerStatus},
//};
//
////////////////////////////////////////////////////////////////////////////
//// package: Modular Combat Bot Script Functions
//static gmFunctionEntry s_BMbotTypeLib[] =
//{
//	//{"GetPlayerStats",			gmfGetPlayerStats},
//	{"GetModuleStats",			gmfGetModuleStats},
//	{"UpgradeModule",			gmfUpgradeModule},
//
//	{"GetUpgradeInfo",			gmfGetWeaponUpgrade},
//	{"UpgradeWeapon",			gmfUpgradeWeapon},	
//
//	{"CanPhysPickup",			gmfCanPhysPickup},	
//	{"GetPhysGunInfo",			gmfPhysGunInfo},
//};
//
////////////////////////////////////////////////////////////////////////////
//
//static bool getTotalXp( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_ExperienceTotal);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getGameXp( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_ExperienceGame);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getModulePts( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_ModulePoints);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getWeaponPts( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_WeaponPoints);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getCredits( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_WeaponPoints);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getLevel( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_Level);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getMinions( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_Minions);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getMinionsMax( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_MinionsMax);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getAuxPower( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_AuxPower);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getAuxPowerMax( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_AuxPowerMax);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}
//
//static bool getAuxPowerRegen( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
//{
//	if(a_native)
//	{
//		const BM_PlayerStats stats = ((BM_Client*)a_native)->GetPlayerStats();
//		a_operands[0] = gmVariable(stats.m_AuxPowerMax);
//	}
//	else
//		a_operands[0].Nullify();
//	return true;
//}

bool gmBindBMLibrary(gmMachine *_machine)
{
	// Register the bot functions.
	//_machine->RegisterLibrary(s_BMbotLib, sizeof(s_BMbotLib) / sizeof(s_BMbotLib[0]));
	//////////////////////////////////////////////////////////////////////////	
	//_machine->RegisterTypeLibrary(gmBot::GetType(), s_BMbotTypeLib, sizeof(s_BMbotTypeLib) / sizeof(s_BMbotTypeLib[0]));
	
	return true;
}
