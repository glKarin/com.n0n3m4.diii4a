#include "PrecompCommon.h"

#ifdef WIN32
#include "windows.h"
#else
// TODO: linux shit here
#endif

#include "Omni-Bot.h"

#include "IGame.h"
#include "GoalManager.h"
#include "IGameManager.h"
#include "TriggerManager.h"

IGameManager *g_GameManager = 0;

// TESTING
//#include "MemoryManager.h"
#include "FileDownloader.h"

//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName( DWORD dwThreadID, char* threadName)
{
	Sleep(10);
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

omnibot_error BotInitialise71(IEngineInterface71 *_pEngineFuncs, int _version)
{
	static IEngineInterface71wrapper interface71wrapper;
	interface71wrapper.base = _pEngineFuncs;
	return BotInitialise(&interface71wrapper, _version);
}

omnibot_error BotInitialise(IEngineInterface *_pEngineFuncs, int _version)
{
	Timer loadTime;

#ifdef WIN32
	SetThreadName ((DWORD)-1, "MainThread");
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_CHECK_CRT_DF|_CRTDBG_LEAK_CHECK_DF);
#endif

	// Create the Game Manager
	g_GameManager = IGameManager::GetInstance();
	omnibot_error result = g_GameManager->CreateGame(_pEngineFuncs, _version);

	if(result==BOT_ERROR_NONE)
	{
		EngineFuncs::ConsoleMessage(va("Omni-bot %s initialized in %.2f seconds.", g_GameManager->GetGame()->GetVersion(), loadTime.GetElapsedSeconds()));
		LOG("Bot Initialized in " << loadTime.GetElapsedSeconds() << " seconds.");
	}
	return result;
}

void BotUpdate()
{
	g_GameManager->UpdateGame();

#ifndef __linux__
	//_ASSERTE( _CrtCheckMemory( ) );
#endif
}

void BotShutdown()
{
	g_GameManager->Shutdown();
	IGameManager::DeleteInstance();
	g_Logger.Stop();
#ifndef __linux__
	//_ASSERTE( _CrtCheckMemory( ) );
#endif
}

void BotConsoleCommand(const Arguments &_args)
{
	StringVector tokList;
	for(int i = 0; i < _args.m_NumArgs; ++i)
	{
		String str = _args.m_Args[i];

		if(i==0)
		{
			std::transform(str.begin(), str.end(), str.begin(), toLower());

			if(str == "bot" || str == "ombot")
				continue;
		}

		tokList.push_back(str);
	}
	if(tokList.empty())
		tokList.push_back("help");
	CommandReciever::DispatchCommand(tokList);
}

static void FixEventId71(const MessageHelper &_message)
{
	int id=_message.GetMessageId();
	if(id>=4 && id<48) _message.m_MessageId--;
	if(id>53) _message.m_MessageId+=4;

	if(_message.GetMessageId() == GAME_ENTITYCREATED)
	{
		Event_EntityCreated *m = _message.Get<Event_EntityCreated>();
		IEngineInterface71wrapper::FixEntityCategory(m->m_EntityCategory);
		if(m->m_EntityCategory.CheckFlag(ENT_CAT_PICKUP))
		{
			if(m->m_EntityClass==ENT_CLASS_GENERIC_HEALTH)
				m->m_EntityCategory.SetFlag(ENT_CAT_PICKUP_HEALTH);
			else if(m->m_EntityClass==ENT_CLASS_GENERIC_AMMO)
				m->m_EntityCategory.SetFlag(ENT_CAT_PICKUP_AMMO);
			else if(m->m_EntityClass==ENT_CLASS_GENERIC_WEAPON)
				m->m_EntityCategory.SetFlag(ENT_CAT_PICKUP_WEAPON);
		}
	}
}

void BotSendEvent71(int _dest, const MessageHelper &_message)
{
	FixEventId71(_message);
	BotSendEvent(_dest, _message);
}

void BotSendGlobalEvent71(const MessageHelper &_message)
{
	FixEventId71(_message);
	BotSendGlobalEvent(_message);
}

void BotSendEvent(int _dest, const MessageHelper &_message)
{
	IGameManager::GetInstance()->GetGame()->DispatchEvent(_dest, _message);
}

void BotSendGlobalEvent(const MessageHelper &_message)
{
	IGameManager::GetInstance()->GetGame()->DispatchGlobalEvent(_message);
}

void BotAddGoal71(const MapGoalDef71 &goaldef071)
{
	static const char* goalTypes[]={"build", "plant", "defuse", "revive", "mover", "mountmg42", 0,
		0, 0, "plant", 0, 0, 0, 0, "healthcab", "ammocab", "checkpoint", "explode"};

	MapGoalDef goaldef;
	if(goaldef071.m_GoalType==8) goaldef.Props.SetString("Type", "flag");
	if(goaldef071.m_GoalType==11) goaldef.Props.SetString("Type", "flagreturn");
	else if(goaldef071.m_GoalType>1000 && goaldef071.m_GoalType<1019 && goalTypes[goaldef071.m_GoalType-1001])
		goaldef.Props.SetString("Type", goalTypes[goaldef071.m_GoalType-1001]);

	goaldef.Props.SetEntity("Entity", goaldef071.m_Entity);
	goaldef.Props.SetInt("Team", goaldef071.m_Team);
	goaldef.Props.SetString("TagName", goaldef071.m_TagName);
	goaldef.Props.SetInt("InterfaceGoal", 1);
	BotAddGoal(goaldef);

	if(goaldef071.m_GoalType==1006){
		goaldef.Props.SetString("Type", "repairmg42");
		BotAddGoal(goaldef);
	}
}

void BotAddGoal(const MapGoalDef &goaldef)
{
	GameEntity Entity;
	if(goaldef.Props.GetEntity("Entity",Entity) && Entity.IsValid())
	{
		Event_EntityCreated d;
		d.m_Entity = Entity;
		d.m_EntityClass = ENT_CLASS_GENERIC_GOAL;
		d.m_EntityCategory.SetFlag(ENT_CAT_INTERNAL);
		BotSendGlobalEvent(MessageHelper(GAME_ENTITYCREATED, &d, sizeof(d)));
	}

	MapGoalPtr goal = GoalManager::GetInstance()->AddGoal(goaldef);

	//send "dropped" trigger when FLAGRETURN goal is created
	if(goal && goal->GetGoalTypeHash()==0xa06840e5)
	{
		TriggerInfo ti;
		const char *TagName;
		if(goaldef.Props.GetString("TagName", TagName))
		{
			int len = (int)strlen(TagName) - 8; //trim suffix
			if(len>0){
				sprintf(ti.m_TagName, "Flag dropped %.*s!", OB_MIN(len, TriggerBufferSize-15), TagName);
				goaldef.Props.GetEntity("Entity", ti.m_Entity);
				strcpy(ti.m_Action, "dropped");
				TriggerManager::GetInstance()->HandleTrigger(ti);
			}
		}
	}
}

void BotSendTrigger(const TriggerInfo &_triggerInfo)
{
	TriggerManager::GetInstance()->HandleTrigger(_triggerInfo);
}

void BotAddBBRecord(BlackBoard_Key _type, int _posterID, int _targetID, obUserData *_data)
{
}

void BotUpdateEntity( GameEntity oldent, GameEntity newent )
{
	GoalManager::GetInstance()->UpdateGoalEntity( oldent, newent );
}

void BotDeleteMapGoal( const char *goalname )
{
	GoalManager::GetInstance()->RemoveGoalByName( goalname );
}

void Omnibot_Load_PrintErr(char const *) {}
void Omnibot_Load_PrintMsg(char const *) {}

//////////////////////////////////////////////////////////////////////////
