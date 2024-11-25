////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompQ4.h"
#include "gmConfig.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmBot.h"
#include "gmBotLibrary.h"

#include "gmQ4Binds.h"
#include "Q4_Client.h"
#include "Q4_Config.h"

// Title: Q4 Script Bindings

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static int GM_CDECL gmfGetLocation(gmThread *a_thread)
{	
	Vector3f vPosition;
	if(a_thread->GetNumParams() == 1)
	{
		GM_CHECK_VECTOR_PARAM(v,1);
		vPosition = Vector3f(v);
	}
	else if(a_thread->GetNumParams() == 3)
	{
		GM_CHECK_FLOAT_OR_INT_PARAM(x, 0);
		GM_CHECK_FLOAT_OR_INT_PARAM(y, 1);
		GM_CHECK_FLOAT_OR_INT_PARAM(z, 2);
		vPosition.x = x;
		vPosition.y = y;
		vPosition.z = z;
	}
	else
	{
		GM_EXCEPTION_MSG("expecting vector3 or x,y,z");
		return GM_EXCEPTION;
	}

	a_thread->PushNewString(InterfaceFuncs::GetLocation(vPosition));
	return GM_OK;
}

static int GM_CDECL gmfIsBuyingAllowed(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(0);
	a_thread->PushInt(InterfaceFuncs::IsBuyingAllowed() ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static int GM_CDECL gmfGetCash(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(0);
	Client *native = gmBot::GetThisObject( a_thread );
	if(!native)
	{
		GM_EXCEPTION_MSG("Script Function on NULL object"); 
		return GM_EXCEPTION;
	}
	
	a_thread->PushFloat(InterfaceFuncs::GetPlayerCash(native->GetGameEntity()));
	return GM_OK;
}

static int GM_CDECL gmfBuy(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(itm, 0);
	Client *native = gmBot::GetThisObject( a_thread );
	if(!native)
	{
		GM_EXCEPTION_MSG("Script Function on NULL object"); 
		return GM_EXCEPTION;
	}

	a_thread->PushInt(InterfaceFuncs::BuySomething(native->GetGameEntity(), itm) ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// package: Q4 Global Functions
static gmFunctionEntry s_ExtendedBotLib[] = 
{ 
	// Function: GetLocation
	//		Gets the location name for a point.
	{"GetLocation",			gmfGetLocation},

	// Function: IsBuyingAllowed
	//		Gets the location name for a point.
	{"IsBuyingAllowed",		gmfIsBuyingAllowed},
};

static gmFunctionEntry s_ExtendedBotTypeLib[] = 
{ 
	// Function: GetCredits
	//		Gets the cash for the bot.
	{"GetCredits",			gmfGetCash},

	// Function: Buy
	//		Buys an item for the bot.
	{"Buy",					gmfBuy},
};

bool gmBindQ4BotLibrary(gmMachine *_machine)
{
	// Register the bot functions.
	_machine->RegisterLibrary(s_ExtendedBotLib, sizeof(s_ExtendedBotLib) / sizeof(s_ExtendedBotLib[0]));
	//////////////////////////////////////////////////////////////////////////	
	_machine->RegisterTypeLibrary(gmBot::GetType(), s_ExtendedBotTypeLib, sizeof(s_ExtendedBotTypeLib) / sizeof(s_ExtendedBotTypeLib[0]));
	return true;
}
