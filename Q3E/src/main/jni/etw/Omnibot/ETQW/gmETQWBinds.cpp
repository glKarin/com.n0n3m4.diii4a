////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETQW.h"
#include "gmETQWBinds.h"

#include "gmConfig.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmBot.h"

#define CHECK_THIS_BOT() \
	Client *native = gmBot::GetThisObject( a_thread ); \
	if(!native) \
	{ \
	GM_EXCEPTION_MSG("Script Function on NULL object"); \
	return GM_EXCEPTION; \
	}

// Title: ETQW Script Bindings

//////////////////////////////////////////////////////////////////////////

// function: ChangePrimaryWeapon
//		Sets the bots primary weapon to a new weapon to use upon respawn
//
// Parameters:
//
//		int - weapon id to choose for primary weapon
//
// Returns:
//		int - true if success, false if error
static int GM_CDECL gmfBotPickPrimaryWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(weaponId, 0);

	bool bSucess = InterfaceFuncs::SelectPrimaryWeapon(native, (ETQW_Weapon)weaponId);	
	a_thread->PushInt(bSucess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ChangeSecondaryWeapon
//		Sets the bots secondary weapon to a new weapon to use upon respawn
//
// Parameters:
//
//		int - weapon id to choose for secondary weapon
//
// Returns:
//		int - true if success, false if error
static int GM_CDECL gmfBotPickSecondaryWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(weaponId, 0);

	bool bSucess = InterfaceFuncs::SelectSecondaryWeapon(native, (ETQW_Weapon)weaponId);	
	a_thread->PushInt(bSucess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetReinforceTime
//		Gets the current reinforcement time for this bots team
//
// Parameters:
//
//		none
//
// Returns:
//		int - reinforce timer
static int GM_CDECL gmfGetReinforceTime(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(0);
	
	a_thread->PushFloat(InterfaceFuncs::GetReinforceTime(native));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetCursorHint
//		Gets the current hint and hint value for the client
//
// Parameters:
//
//		table - table to store results. function sets 'type' and 'value'
//
// Returns:
//		none
static int GM_CDECL gmfGetCurrentCursorHint(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_TABLE_PARAM(hint, 0);

	int iHintType = 0, iHintValue = 0;
	InterfaceFuncs::GetCurrentCursorHint(native, iHintType, iHintValue);

	hint->Set(a_thread->GetMachine(), "type", gmVariable(iHintType));
	hint->Set(a_thread->GetMachine(), "value", gmVariable(iHintValue));

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ChangeSpawnPoint
//		Changes the bots active spawn point
//
// Parameters:
//
//		int - Spawn point to change to
//
// Returns:
//		none
static int GM_CDECL gmfChangeSpawnPoint(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(spawnpoint, 0);

	InterfaceFuncs::ChangeSpawnPoint(native, spawnpoint);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static gmFunctionEntry s_ExtendedBotTypeLib[] =
{ 
	{"ChangePrimaryWeapon",		gmfBotPickPrimaryWeapon},
	{"ChangeSecondaryWeapon",	gmfBotPickSecondaryWeapon},	
	{"GetReinforceTime",		gmfGetReinforceTime},
	{"GetCursorHint",			gmfGetCurrentCursorHint},
	{"ChangeSpawnPoint",		gmfChangeSpawnPoint},
};

void gmBindETQWBotLibrary(gmMachine *_machine)
{
	// Register the bot functions.
	//_machine->RegisterLibrary(s_ExntendedBotLib, sizeof(s_ExntendedBotLib) / sizeof(s_ExntendedBotLib[0]));
	//////////////////////////////////////////////////////////////////////////	
	_machine->RegisterTypeLibrary(gmBot::GetType(), s_ExtendedBotTypeLib, sizeof(s_ExtendedBotTypeLib) / sizeof(s_ExtendedBotTypeLib[0]));

	// Register additional bot properties

	// var: TargetBreakableDistance
	//		The distance the bot will target breakable entities. Targets beyond this range will be ignored.
	gmBot::RegisterAutoProperty("TargetBreakableDist", GM_FLOAT, offsetof(ETQW_Client, m_BreakableTargetDistance), 0);
}
