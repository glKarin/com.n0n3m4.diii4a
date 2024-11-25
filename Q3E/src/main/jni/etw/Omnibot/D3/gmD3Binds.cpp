////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompD3.h"
#include "gmConfig.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmBot.h"

#include "gmD3Binds.h"
#include "D3_Client.h"
#include "D3_Config.h"

// Title: D3 Script Bindings

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// package: D3 Global Functions
static gmFunctionEntry s_ExtendedBotLib[] = 
{ 
	// Function: GetLocation
	//		Gets the location name for a point.
	{"GetLocation",			gmfGetLocation},
};

bool gmBindD3BotLibrary(gmMachine *_machine)
{
	// Register the bot functions.
	_machine->RegisterLibrary(s_ExtendedBotLib, sizeof(s_ExtendedBotLib) / sizeof(s_ExtendedBotLib[0]));
	return true;
}
