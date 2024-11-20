//#include "vld.h"

#include "BotExports.h"
#include "Omni-Bot.h"

//#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
BOOL APIENTRY DllMain( HANDLE , DWORD ul_reason_for_call, LPVOID )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}
#endif

OMNIBOT_API eomnibot_error ExportBotFunctionsFromDLL(Bot_EngineFuncs_t *_pBotFuncs, int _size)
{
	if(sizeof(Bot_EngineFuncs_t) == _size)
	{
		_pBotFuncs->pfnInitialize			= BotInitialise;
		_pBotFuncs->pfnUpdate				= BotUpdate;
		_pBotFuncs->pfnShutdown				= BotShutdown;
		_pBotFuncs->pfnConsoleCommand		= BotConsoleCommand;
		_pBotFuncs->pfnAddGoal				= BotAddGoal;
		_pBotFuncs->pfnSendTrigger			= BotSendTrigger;
		_pBotFuncs->pfnAddBlackboardRecord	= BotAddBBRecord;

		// New message functions.
		_pBotFuncs->pfnSendEvent			= BotSendEvent;
		_pBotFuncs->pfnSendGlobalEvent		= BotSendGlobalEvent;

		/*_pBotFuncs->pfnSubscribeToMsg		= Message_SubscribeToMsg;
		_pBotFuncs->pfnUnsubscribe			= Message_Unsubscribe;
		_pBotFuncs->pfnBeginMessage			= Message_BeginMessage;
		_pBotFuncs->pfnBeginMessageEx		= Message_BeginMessageEx;
		_pBotFuncs->pfnEndMessage			= Message_EndMessage;
		_pBotFuncs->pfnEndMessageEx			= Message_EndMessageEx;*/

		_pBotFuncs->pfnUpdateEntity			= BotUpdateEntity;
		_pBotFuncs->pfnDeleteGoal			= BotDeleteMapGoal;

		return BOT_ERROR_NONE;
	}

	if(sizeof(Bot_EngineFuncs71_t) == _size)
	{
		Bot_EngineFuncs71_t* _pBotFuncs71 = reinterpret_cast<Bot_EngineFuncs71_t*>(_pBotFuncs);
		_pBotFuncs71->pfnInitialize			= BotInitialise71;
		_pBotFuncs71->pfnUpdate				= BotUpdate;
		_pBotFuncs71->pfnShutdown				= BotShutdown;
		_pBotFuncs71->pfnConsoleCommand		= BotConsoleCommand;
		_pBotFuncs71->pfnAddGoal				= BotAddGoal71;
		_pBotFuncs71->pfnSendTrigger			= BotSendTrigger;
		_pBotFuncs71->pfnAddBlackboardRecord	= BotAddBBRecord;
		_pBotFuncs71->pfnSendEvent			= BotSendEvent71;
		_pBotFuncs71->pfnSendGlobalEvent		= BotSendGlobalEvent71;
		_pBotFuncs71->pfnUpdateEntity			= BotUpdateEntity;
		_pBotFuncs71->pfnDeleteGoal			= BotDeleteMapGoal;

		return BOT_ERROR_NONE;
	}

	return BOT_ERROR_BAD_INTERFACE;
}
