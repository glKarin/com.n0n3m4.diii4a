////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF2.h"
#include "gmConfig.h"
#include "gmBot.h"
#include "gmTF2Binds.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmGameEntity.h"
#include "gmBotLibrary.h"
#include "TF2_BaseStates.h"
#include "TF2_Messages.h"

#define CHECK_THIS_BOT() \
	Client *native = gmBot::GetThisObject( a_thread ); \
	if(!native) \
	{ \
	GM_EXCEPTION_MSG("Script Function on NULL object"); \
	return GM_EXCEPTION; \
	}

// Title: TF2 Script Bindings

//////////////////////////////////////////////////////////////////////////

obBool VarToBool(const gmVariable &_var)
{
	if(_var.IsNull())
		return Invalid;
	return _var.GetIntSafe(False) ? True : False;
}

static int GM_CDECL gmfSetLoadOut(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);	
	GM_CHECK_TABLE_PARAM(tbl,0)
	using namespace AiState;

	gmTableObject *pScoutTbl = tbl->Get(a_thread->GetMachine(),"Scout").GetTableObjectSafe();
	gmTableObject *pSniperTbl = tbl->Get(a_thread->GetMachine(),"Sniper").GetTableObjectSafe();
	gmTableObject *pSoldierTbl = tbl->Get(a_thread->GetMachine(),"Soldier").GetTableObjectSafe();
	gmTableObject *pDemomanTbl = tbl->Get(a_thread->GetMachine(),"Demoman").GetTableObjectSafe();
	gmTableObject *pMedicTbl = tbl->Get(a_thread->GetMachine(),"Medic").GetTableObjectSafe();
	gmTableObject *pHwGuyTbl = tbl->Get(a_thread->GetMachine(),"HwGuy").GetTableObjectSafe();
	gmTableObject *pPyroTbl = tbl->Get(a_thread->GetMachine(),"Pyro").GetTableObjectSafe();
	gmTableObject *pEngineerTbl = tbl->Get(a_thread->GetMachine(),"Engineer").GetTableObjectSafe();
	gmTableObject *pSpyTbl = tbl->Get(a_thread->GetMachine(),"Spy").GetTableObjectSafe();

	Event_SetLoadOut_TF2 loadout = {};

	//////////////////////////////////////////////////////////////////////////
	if(pScoutTbl)
	{
	}
	if(pSniperTbl)
	{
	}
	if(pSoldierTbl)
	{
	}
	if(pDemomanTbl)
	{
	}
	if(pMedicTbl)
	{
		loadout.Medic.Blutsauger	= VarToBool(pMedicTbl->Get(a_thread->GetMachine(),"Blutsauger"));
		loadout.Medic.Kritzkrieg	= VarToBool(pMedicTbl->Get(a_thread->GetMachine(),"Kritzkrieg"));
		loadout.Medic.Ubersaw		= VarToBool(pMedicTbl->Get(a_thread->GetMachine(),"Ubersaw"));
	}
	if(pHwGuyTbl)
	{
		loadout.HwGuy.Natascha		= VarToBool(pHwGuyTbl->Get(a_thread->GetMachine(),"Natascha"));
		loadout.HwGuy.Sandvich		= VarToBool(pHwGuyTbl->Get(a_thread->GetMachine(),"Sandvich"));
		loadout.HwGuy.KGB			= VarToBool(pHwGuyTbl->Get(a_thread->GetMachine(),"KGB"));
	}
	if(pPyroTbl)
	{
		loadout.Pyro.Backburner		= VarToBool(pPyroTbl->Get(a_thread->GetMachine(),"Backburner"));
		loadout.Pyro.FlareGun		= VarToBool(pPyroTbl->Get(a_thread->GetMachine(),"FlareGun"));
		loadout.Pyro.Axtinguisher	= VarToBool(pPyroTbl->Get(a_thread->GetMachine(),"Axtinguisher"));
	}
	if(pEngineerTbl)
	{
	}
	if(pSpyTbl)
	{
	}
	//////////////////////////////////////////////////////////////////////////
	InterfaceFuncs::SetLoadOut(native->GetGameEntity(),loadout);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// package: Team Fortress Bot Library Functions
//static gmFunctionEntry s_TF2botLib[] = 
//{ 
//	/*{"LockPlayerPosition",		gmfLockPlayerPosition},
//	{"HudHint",					gmfHudHint},
//	{"HudMessage",				gmfHudTextBox},	
//	{"HudAlert",				gmfHudAlert},	
//	{"HudMenu",					gmfHudMenu},	
//	{"HudTextMsg",				gmfHudTextMsg},	*/
//};

//////////////////////////////////////////////////////////////////////////
// package: Team Fortress Bot Script Functions
static gmFunctionEntry s_TF2botTypeLib[] =
{	
	{"SetLoadOut",		gmfSetLoadOut},
};

bool gmBindTF2Library(gmMachine *_machine)
{
	// Register the bot functions.
	//_machine->RegisterLibrary(s_TF2botLib, sizeof(s_TF2botLib) / sizeof(s_TF2botLib[0]));
	//////////////////////////////////////////////////////////////////////////	
	_machine->RegisterTypeLibrary(gmBot::GetType(), s_TF2botTypeLib, sizeof(s_TF2botTypeLib) / sizeof(s_TF2botTypeLib[0]));
	return true;
}
