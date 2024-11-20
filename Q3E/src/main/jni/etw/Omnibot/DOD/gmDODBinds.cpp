////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompDOD.h"
#include "gmConfig.h"
#include "gmBot.h"
#include "gmDODBinds.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmGameEntity.h"
#include "gmBotLibrary.h"
#include "DOD_BaseStates.h"
#include "DOD_Messages.h"

#define CHECK_THIS_BOT() \
	Client *native = gmBot::GetThisObject( a_thread ); \
	if(!native) \
	{ \
	GM_EXCEPTION_MSG("Script Function on NULL object"); \
	return GM_EXCEPTION; \
	}

// Title: DOD Script Bindings

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

	gmTableObject *pRiflemanTbl = tbl->Get(a_thread->GetMachine(),"Rifleman").GetTableObjectSafe();
	gmTableObject *pAssaultTbl = tbl->Get(a_thread->GetMachine(),"Assault").GetTableObjectSafe();
	gmTableObject *pSupportTbl = tbl->Get(a_thread->GetMachine(),"Support").GetTableObjectSafe();
	gmTableObject *pSniperTbl = tbl->Get(a_thread->GetMachine(),"Sniper").GetTableObjectSafe();
	gmTableObject *pMachineGunnerTbl = tbl->Get(a_thread->GetMachine(),"MachineGunner").GetTableObjectSafe();
	gmTableObject *pRocket = tbl->Get(a_thread->GetMachine(),"Rocket").GetTableObjectSafe();

	//////////////////////////////////////////////////////////////////////////
	if(pRiflemanTbl)
	{
	}
	if(pAssaultTbl)
	{
	}
	if(pSupportTbl)
	{
	}
	if(pSniperTbl)
	{
	}
	if(pMachineGunnerTbl)
	{
	}
	if(pRocket)
	{
	}
	//////////////////////////////////////////////////////////////////////////
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// package: Day of Defeat Bot Script Functions
static gmFunctionEntry s_DODbotTypeLib[] =
{	
	{"SetLoadOut",		gmfSetLoadOut},
};

bool gmBindDODLibrary(gmMachine *_machine)
{
	// Register the bot functions.
	//_machine->RegisterLibrary(s_DODbotLib, sizeof(s_DODbotLib) / sizeof(s_DODbotLib[0]));
	//////////////////////////////////////////////////////////////////////////	
	_machine->RegisterTypeLibrary(gmBot::GetType(), s_DODbotTypeLib, sizeof(s_DODbotTypeLib) / sizeof(s_DODbotTypeLib[0]));
	return true;
}
