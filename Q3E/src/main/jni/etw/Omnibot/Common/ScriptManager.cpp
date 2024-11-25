#include "PrecompCommon.h"
#include "ScriptManager.h"

#include "Client.h"
#include "IGameManager.h"
#include "FileSystem.h"
#include "DebugWindow.h"

#include "Weapon.h"

// Custom script libraries
#undef GetObject //Argh Windows defines this in WINGDI.H
#include "gmDebug.h"
#include "gmSystemLibApp.h"
#include "gmMathLibrary.h"
#include "gmBotLibrary.h"
#include "gmMachineLib.h"
#include "gmHelpers.h"
#include "gmAABB.h"
#include "gmBot.h"
#include "gmWeapon.h"
#include "gmGameEntity.h"
#include "gmTargetInfo.h"
#include "gmTriggerInfo.h"
#include "gmNamesList.h"
#include "gmMatrix3.h"
#include "gmStringLib.h"
#include "gmUtilityLib.h"
#include "gmSqliteLib.h"
#include "gmTimer.h"
#include "gmDebugWindow.h"
#include "gmScriptGoal.h"
#include "gmSchemaLib.h"

// Custom Objects
#include "gmBind.h"
#include "gmbinder2.h"

//////////////////////////////////////////////////////////////////////////

bool ScriptLiveUpdate = false;

struct LiveUpdateEntry
{
	filePath		File;
	obint64			FileModTime;

	LiveUpdateEntry(const filePath &_path, obint64 _mod)
		: File(_path)
		, FileModTime(_mod)
	{
	}
};

typedef std::vector<LiveUpdateEntry> LiveUpdateList;
static LiveUpdateList g_LiveUpdate;
static int NextLiveUpdateCheck = 0;

//////////////////////////////////////////////////////////////////////////

ThreadScoper::ThreadScoper(int _id) : m_ThreadId(_id)
{
}

ThreadScoper::~ThreadScoper()
{
	Kill();
}

bool ThreadScoper::IsActive()
{
	if((m_ThreadId != GM_INVALID_THREAD) && ScriptManager::IsInstantiated())
	{
		gmThread *pTh = ScriptManager::GetInstance()->GetMachine()->GetThread(m_ThreadId);
		if(pTh != NULL && pTh->GetState() != gmThread::KILLED && pTh->GetState() != gmThread::EXCEPTION)
			return true;
	}
	return false;
}

void ThreadScoper::Kill()
{
	if((m_ThreadId != GM_INVALID_THREAD) && ScriptManager::IsInstantiated())
	{
		ScriptManager::GetInstance()->GetMachine()->KillThread(m_ThreadId);
		m_ThreadId = GM_INVALID_THREAD;
	}
}

//////////////////////////////////////////////////////////////////////////

bool g_RemoteDebuggerInitialized = false;

#if defined(ENABLE_REMOTE_DEBUGGER) && defined(GMDEBUG_SUPPORT)

#include <sfml/Network.hpp>
#pragma comment(lib,"sfml-network.lib")

sf::SocketTCP g_DebugListener;
sf::SocketTCP g_DebugClient;

const int GM_DEBUGGER_PORT = 49001;

#endif

//////////////////////////////////////////////////////////////////////////
//#if GM_LOGALLOCANDDEALLOC
//
//File destructFile;
//void LogDestruct(int _type, void *_addr)
//{
//	if(!destructFile.IsOpen())
//	{
//		FileSystem::FileDelete("user/destruct.txt");
//		destructFile.OpenForWrite("user/destruct.txt", File::Text);
//	}
//
//	if(destructFile.IsOpen())
//	{
//		const char *pTypeName = ScriptManager::GetInstance()->GetMachine()->GetTypeName((gmType)_type);
//		destructFile.WriteString(va("%x, %s", _addr, pTypeName));
//		destructFile.WriteNewLine();
//	}
//}
//
//File allocFile;
//void LogAlloc(int _type, void *_addr)
//{
//	if(!FileSystem::IsInitialized())
//		return;
//
//	if(!allocFile.IsOpen())
//	{
//		FileSystem::FileDelete("user/alloc.txt");
//		allocFile.OpenForWrite("user/alloc.txt", File::Text);
//	}
//
//	if(allocFile.IsOpen())
//	{
//		const char *pTypeName = ScriptManager::GetInstance()->GetMachine()->GetTypeName((gmType)_type);
//		allocFile.WriteString(va("%x, %s", _addr, pTypeName));
//		allocFile.WriteNewLine();
//	}
//}
//
//#endif

//////////////////////////////////////////////////////////////////////////

int ImportModuleImpl(gmThread *a_thread, const char *a_filename, gmVariable &a_this)
{
	try
	{
		int ThreadId = GM_INVALID_THREAD;
		filePath script("%s.gm",a_filename);
		if(ScriptManager::GetInstance()->ExecuteFile(script,ThreadId,&a_this))
		{
			return GM_OK;
		}
	}
	catch(const std::exception & ex)
	{
		_UNUSED(ex);
	}

	GM_EXCEPTION_MSG("Unable to execute %s",a_filename);
	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

ScriptManager *ScriptManager::m_Instance = NULL;

ScriptManager::ScriptManager() :
	m_ScriptEngine		(0),
#ifdef _DEBUG
	m_DebugScripts		(true)
#else
	m_DebugScripts		(false)
#endif
{
}

ScriptManager::~ScriptManager()
{
	if(m_ScriptEngine)
		Shutdown();
}

ScriptManager *ScriptManager::GetInstance()
{
	if(!m_Instance)
		m_Instance = new ScriptManager;
	return m_Instance;
}

void ScriptManager::DeleteInstance()
{
	OB_DELETE(m_Instance);
}

bool ScriptManager::IsInstantiated()
{
	return m_Instance != 0;
}

void ScriptManager::Init()
{
	LOGFUNCBLOCK;
	InitCommands();
	LOG("Initializing Scripting System...");
	// Set machine callbacks
	m_ScriptEngine = new gmMachine;
	m_ScriptEngine->SetDebugMode(true);
	gmMachine::s_machineCallback = ScriptManager::ScriptSysCallback_Machine;
	gmMachine::s_printCallback = ScriptManager::ScriptSysCallback_Print;

	gmGCRootManager::Init();

	DisableGCInScope gcEn(m_ScriptEngine);

	const int MEM_USAGE_MB = 2 * 1048576;
	const int MEM_USAGE_KB = 0 * 1024;
	const int HARD_MEM_USAGE = MEM_USAGE_MB + MEM_USAGE_KB;
	m_ScriptEngine->SetDesiredByteMemoryUsageHard(HARD_MEM_USAGE);
	m_ScriptEngine->SetDesiredByteMemoryUsageSoft(HARD_MEM_USAGE * 9 / 10);

	//////////////////////////////////////////////////////////////////////////
	LOG("Hard Memory Limit: " << Utils::FormatByteString(HARD_MEM_USAGE));

	// Allocate some permanent strings for properties that will be used alot.
	m_ScriptEngine->AllocPermanantStringObject("CurrentHealth");
	m_ScriptEngine->AllocPermanantStringObject("MaxHealth");
	m_ScriptEngine->AllocPermanantStringObject("CurrentArmor");
	m_ScriptEngine->AllocPermanantStringObject("MaxArmor");

	// set the callback for executing import scripts
	gmImportExecuteFile = ImportModuleImpl;

	// Bind libraries.
	LOG("Binding Script Libraries...");
	gmBindSystemLib(m_ScriptEngine);
	LOG("+ System Library Bound.");
	gmBindMathLibrary(m_ScriptEngine);
	LOG("+ Math Library Bound.");
	gmBindStringLib(m_ScriptEngine);
	LOG("+ String Library Bound.");
	gmBindBotLib(m_ScriptEngine);
	LOG("+ Bot System Library Bound.");
	BlackBoard::Bind(m_ScriptEngine);
	LOG("+ Blackboard Library Bound.");
	gmBindNamesListLib(m_ScriptEngine);
	gmBot::Initialise(m_ScriptEngine, true);
	LOG("+ Bot Library Bound.");
	/*gmMapGoal::Initialise(m_ScriptEngine, true);
	LOG("+ MapGoal Library Bound.");*/
	gmTargetInfo::Initialise(m_ScriptEngine, true);
	LOG("+ TargetInfo Library Bound.");
	gmTriggerInfo::Initialise(m_ScriptEngine, true);
	LOG("+ TriggerInfo Library Bound.");
	gmTimer::Initialise(m_ScriptEngine, false);
	LOG("+ Timer Library Bound.");
	//BindAABB(m_ScriptEngine);
	gmAABB::Initialise(m_ScriptEngine, false);

	LOG("+ AABB Library Bound.");
	gmMatrix3::Initialise(m_ScriptEngine, false);
	LOG("+ Matrix3 Library Bound.");
	gmScriptGoal::Initialise(m_ScriptEngine, true);
	LOG("+ Script Goal Library Bound.");
	gmBindUtilityLib(m_ScriptEngine);
	LOG("+ Utility Library Bound.");
	gmSchema::BindLib(m_ScriptEngine);
	LOG("+ Schema Library Bound.");

	BindEntityStackCustom(m_ScriptEngine);

	// New Bindings
	MapGoal::Bind(m_ScriptEngine);
	LOG("+ MapGoal Library Bound.");
	Weapon::Bind(m_ScriptEngine);
	LOG("+ Weapon Library Bound.");

#ifdef ENABLE_DEBUG_WINDOW
	gmBindDebugWindowLibrary(m_ScriptEngine);
	LOG("+ Gui Library Bound.");
#endif

#if defined(ENABLE_REMOTE_DEBUGGER) && defined(GMDEBUG_SUPPORT)
	gmBindDebugLib(m_ScriptEngine);
#endif

	// Create default global tables.
	m_ScriptEngine->GetGlobals()->Set(m_ScriptEngine, "Names", gmVariable(m_ScriptEngine->AllocUserObject(NULL, GM_NAMESLIST)));
	m_ScriptEngine->GetGlobals()->Set(m_ScriptEngine, "BotTable", gmVariable(m_ScriptEngine->AllocTableObject()));
	m_ScriptEngine->GetGlobals()->Set(m_ScriptEngine, "Commands", gmVariable(m_ScriptEngine->AllocTableObject()));
	m_ScriptEngine->GetGlobals()->Set(m_ScriptEngine, "GOALS",gmVariable(m_ScriptEngine->AllocTableObject()));

	LOG("+ Name List Created");

	gmBind2::Global(m_ScriptEngine, "COLOR")
		.var(COLOR::BLACK.rgba(),		"BLACK")
		.var(COLOR::RED.rgba(),			"RED")
		.var(COLOR::GREEN.rgba(),		"GREEN")
		.var(COLOR::BLUE.rgba(),		"BLUE")
		.var(COLOR::WHITE.rgba(),		"WHITE")
		.var(COLOR::MAGENTA.rgba(),		"MAGENTA")
		.var(COLOR::LIGHT_GREY.rgba(),	"LIGHT_GREY")
		.var(COLOR::GREY.rgba(),		"GREY")
		.var(COLOR::ORANGE.rgba(),		"ORANGE")
		.var(COLOR::YELLOW.rgba(),		"YELLOW")
		.var(COLOR::CYAN.rgba(),		"CYAN")
		.var(COLOR::PINK.rgba(),		"PINK")
		.var(COLOR::BROWN.rgba(),		"BROWN")
		.var(COLOR::AQUAMARINE.rgba(),	"AQUAMARINE")
		.var(COLOR::LAVENDER.rgba(),	"LAVENDER")
		;

	gmBind2::Global(m_ScriptEngine,"MoveMode")
		.var((int)Run,"Run")
		.var((int)Walk,"Walk")
		;

	gmTableObject *pAimPrioTable = m_ScriptEngine->AllocTableObject();
	m_ScriptEngine->GetGlobals()->Set(m_ScriptEngine, "Priority", gmVariable(pAimPrioTable));
	for(int i = 0; i < Priority::NumPriority; ++i)
		pAimPrioTable->Set(m_ScriptEngine, Priority::AsString(i), gmVariable(i));

#ifdef ENABLE_REMOTE_DEBUGGER
	bool RemoteDebugger = false;
	Options::GetValue("Script","EnableRemoteDebugger",RemoteDebugger);
	EnableRemoteDebugger(RemoteDebugger);
#endif

	ScriptLiveUpdate = false;
	Options::GetValue("Script","LiveUpdate",ScriptLiveUpdate);

	LOG("done.");
}

void ScriptManager::Shutdown()
{
#if defined(ENABLE_REMOTE_DEBUGGER) && defined(GMDEBUG_SUPPORT)
	if(g_RemoteDebuggerInitialized)
	{
		CloseDebugSession();
		g_DebugListener.Close();
	}
#endif

	g_LiveUpdate.clear();

	gmGCRootManager::Get()->DestroyMachine(m_ScriptEngine);
	gmGCRootManager::Destroy();

	bool stats = false;
	Options::GetValue("Script","EndGameStats",stats);
	if(stats)
	{
		LOGFUNCBLOCK;
		ShowGMStats();
	}
	OB_DELETE(m_ScriptEngine);
	LOG("Script System Shut Down.");
}

struct ThreadStatus
{
	int m_Running;
	int m_Blocked;
	int m_Sleeping;
};

void ScriptManager::Update()
{
	Prof(ScriptManager_Update);

	{
		Prof(gmMachine_Execute);
		m_ScriptEngine->Execute((gmuint32)IGame::GetDeltaTime());
	}

	//////////////////////////////////////////////////////////////////////////
#if defined(ENABLE_REMOTE_DEBUGGER) && defined(GMDEBUG_SUPPORT)
	if(g_RemoteDebuggerInitialized)
	{
		if(!g_DebugClient.IsValid())
		{
			sf::IPAddress addr;
			sf::Socket::Status s = g_DebugListener.Accept(g_DebugClient,&addr); s;

			if(addr.IsValid())
			{
				Utils::OutputDebug(kScript, "Remote Debugger Connected: %s", addr.ToString().c_str());
				OpenDebugSession(m_ScriptEngine);
				g_DebugClient.SetBlocking(false);
			}
		}

		if(g_DebugClient.IsValid())
		{
			Prof(RemoteDebugClient);
			UpdateDebugSession();
		}
	}
#endif
	//////////////////////////////////////////////////////////////////////////
	if(ScriptLiveUpdate)
		CheckLiveUpdates();
}

gmUserObject *ScriptManager::AddBotToGlobalTable(Client *_client)
{
	gmUserObject *pUser = gmBot::WrapObject(m_ScriptEngine, _client);
	gmTableObject *pGlobalBotsTable = GetGlobalBotsTable();
	if(pGlobalBotsTable)
	{
		pGlobalBotsTable->Set(m_ScriptEngine, _client->GetGameID(), gmVariable(pUser));
	}
	return pUser;
}

void ScriptManager::RemoveFromGlobalTable(Client *_client)
{
	gmTableObject *pGlobalBotsTable = GetGlobalBotsTable();
	if(pGlobalBotsTable)
	{
		pGlobalBotsTable->Set(m_ScriptEngine, _client->GetGameID(), gmVariable::s_null);
	}
	else
	{
		EngineFuncs::ConsoleError("Bots script table lost");
	}
}

gmTableObject *ScriptManager::GetGlobalBotsTable()
{
	gmVariable botVar = m_ScriptEngine->GetGlobals()->Get(m_ScriptEngine,"BotTable");
	gmTableObject *botTable = botVar.GetTableObjectSafe();
	if(botTable)
	{
		gmTableObject *botSubTable = botVar.GetTableObjectSafe();
		return botSubTable;
	}
	else
	{
		if(IsScriptDebugEnabled())
		{
			EngineFuncs::ConsoleError("Global Bots table lost");
		}
	}
	return NULL;
}

gmTableObject *ScriptManager::GetGlobalCommandsTable()
{
	gmVariable cmdVar = m_ScriptEngine->GetGlobals()->Get(m_ScriptEngine,"Commands");
	gmTableObject *commandTable = cmdVar.GetTableObjectSafe();
	if(commandTable)
	{
		return commandTable;
	}
	else
	{
		if(IsScriptDebugEnabled())
		{
			EngineFuncs::ConsoleError("Global commands table lost");
		}
	}
	return NULL;
}

gmTableObject *ScriptManager::GetBotTable(const Client *_client)
{
	gmTableObject *pBotsTable = GetGlobalBotsTable();

	if(pBotsTable)
	{
		// Get the table for this specific bot.
		gmVariable botVar = pBotsTable->Get(gmVariable(_client->GetGameID()));

		if(botVar.m_type == gmBot::GetType())
		{
			return gmBot::GetUserTable(botVar.GetUserObjectSafe(gmBot::GetType()));
		}
		else
		{
			if(IsScriptDebugEnabled())
			{
				EngineFuncs::ConsoleError("Bot entry wrong type!");
			}
		}
	}
	return NULL;
}

gmVariable ScriptManager::ExecBotCallback(Client *_client, const char *_func)
{
	gmTableObject *pBotTable = GetBotTable(_client);
	if(pBotTable)
	{
		/*gmVariable v =*/ pBotTable->Get(m_ScriptEngine, _func);

		gmCall call;
		gmVariable vThis(_client->GetScriptObject());
		if(call.BeginTableFunction(m_ScriptEngine, _func, pBotTable, vThis))
		{
			call.End();
			return call.GetReturnedVariable();
		}
	}
	return gmVariable::s_null;
}

bool ScriptManager::ExecuteFile(const filePath &_file, int &_threadId, gmVariable *_this/* = NULL*/)
{
	//Utils::OutputDebug(kScript,"ExecuteFile: %s",_file.c_str());

	GM_ASSERT(m_ScriptEngine);
	if ( m_ScriptEngine != NULL ) {
		_threadId = GM_INVALID_THREAD;

		// Find the file
		File InFile;

		filePath localFilePath = _file;
		InFile.OpenForRead(localFilePath, File::Binary);
		if(!InFile.IsOpen())
		{
			localFilePath = filePath( "scripts/%s", _file.c_str() );
			InFile.OpenForRead(localFilePath, File::Binary);
			if(!InFile.IsOpen())
			{
				localFilePath = filePath( "global_scripts/%s", _file.c_str() );
				InFile.OpenForRead(localFilePath, File::Binary);
			}
		}

		if(InFile.IsOpen())
		{
			obuint32 fileSize = (obuint32)InFile.FileLength();
#if __cplusplus >= 201103L //karin: using std::shared_ptr<T[]> instead of boost::shared_array<T>
			compat::shared_array<char> pBuffer(new char[fileSize+1]);
#else
			boost::shared_array<char> pBuffer(new char[fileSize+1]);
#endif

			InFile.Read(pBuffer.get(), fileSize);
			pBuffer[fileSize] = 0;
			InFile.Close();

			LOG("Running script: " << _file)
			if(fileSize>0)
			{
				const char *s = pBuffer.get();
				if(s[0]=='\xEF' && s[1]=='\xBB' && s[2]=='\xBF') s+=3; //UTF-8 BOM

				int errors = m_ScriptEngine->ExecuteString(s, &_threadId, true, _file, _this);
				if(errors)
				{
					bool b = IsScriptDebugEnabled();
					SetScriptDebugEnabled(true);
					LogAnyMachineErrorMessages(m_ScriptEngine);
					SetScriptDebugEnabled(b);
				}
				else
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool ScriptManager::ExecuteString(const String &_string, gmVariable *_this/* = NULL*/, bool _now)
{
	GM_ASSERT(m_ScriptEngine);
	if ( m_ScriptEngine ) {
		int threadId = GM_INVALID_THREAD;
		int errors = m_ScriptEngine->ExecuteString(_string.c_str(), &threadId, _now, NULL, _this);
		if(errors)
		{
			LogAnyMachineErrorMessages(m_ScriptEngine);
			return false;
		}
	}
	return true;
}

bool ScriptManager::ExecuteStringLogged(const String &_string)
{
	bool b = false;
	File f;
	f.OpenForWrite("user/logged.gm", File::Text, true);
	if(f.IsOpen())
	{
		b = f.WriteString(_string) && f.WriteNewLine();
		OBASSERT(b, "Problem logging script string!");
	}
	EngineFuncs::ConsoleMessage(va("ExecString: %s", _string.c_str()));

	return (b && ExecuteString(_string));
}

void ScriptManager::LogAnyMachineErrorMessages(gmMachine *_machine)
{
	bool bFirst = true;
	const char *pMessage = 0;
	while((pMessage = _machine->GetLog().GetEntry(bFirst)))
	{
		Utils::OutputDebugBasic(kError, "Script Error:");
		Utils::OutputDebugBasic(kError, pMessage);
		LOGERR(pMessage);

#ifdef ENABLE_DEBUG_WINDOW
		String s = pMessage;
		DW.Console.AddLine(s);
#endif
		if(_machine->GetDebugMode())
		{
			EngineFuncs::ConsoleError(pMessage);
		}
	}
	_machine->GetLog().Reset();
}

void GM_CDECL ScriptManager::ScriptSysCallback_Print(gmMachine* a_machine, const char* a_string)
{
	if(a_string)
	{
#ifdef ENABLE_DEBUG_WINDOW
		DW.Console.AddLine(a_string);
#endif
		EngineFuncs::ConsoleMessage(a_string);
	}
}

bool GM_CDECL ScriptManager::ScriptSysCallback_Machine(gmMachine* a_machine, gmMachineCommand a_command, const void* a_context)
{
	const gmThread *pThread = static_cast<const gmThread*>(a_context);

	switch(a_command)
	{
	case MC_THREAD_EXCEPTION:
		{
			LogAnyMachineErrorMessages(a_machine);

			const gmVariable *pThis = pThread->GetThis();
			if(pThis){
				gmUserObject *pUserObj = pThis->GetUserObjectSafe(gmScriptGoal::GetType());
				if(pUserObj){
					AiState::ScriptGoal *pScriptGoal = gmScriptGoal::GetNative(pUserObj);
					if(pScriptGoal){
						pScriptGoal->OnException();
					}
				}
			}
			break;
		}
	case MC_THREAD_CREATE:
		{
			// Called when a thread is created.  a_context is the thread.
			Client *pClient = 0;
			const gmVariable *pThis = pThread->GetThis();
			gmUserObject *pBotUserObj = pThis ? pThis->GetUserObjectSafe(gmBot::GetType()) : 0;
			if(pBotUserObj)
			{
				// See if it's the bots goal thread that is being destroyed.
				pClient = gmBot::GetNative(pBotUserObj);
				if(pClient)
				{
					/*Event_SystemThreadCreated d = { pThread->GetId() };
					MessageHelper evt(SYSTEM_THREAD_CREATED, &d, sizeof(d));
					pClient->SendEvent(evt);*/
				}
			}

			// Debug Output
			bool bScriptDebugEnabled = ScriptManager::GetInstance()->IsScriptDebugEnabled();
			if(bScriptDebugEnabled || pClient)
			{
				const gmFunctionObject *pFn = pThread->GetFunctionObject();
				const char *pFuncName = pFn != NULL ? pFn->GetDebugName() : 0;

				const char *pSource = 0, *pFileName = 0;
				if ( pFn ) {
					a_machine->GetSourceCode(pFn->GetSourceId(), pSource, pFileName);
				}

				String errMsg = va("Thread Created <%s>: %s : %s Id: %d time: %d",
					(pClient ? pClient->GetName() : ""),
					(pFileName ? pFileName : "<unknown file>"),
					(pFuncName ? pFuncName : "<noname>"),
					pThread->GetId(),
					IGame::GetTime()).c_str();

				if(bScriptDebugEnabled)
				{
					//EngineFuncs::ConsoleMessage(errMsg.str().c_str());
					Utils::OutputDebug(kScript, errMsg.c_str());
				}
			}
			break;
		}
	case MC_THREAD_DESTROY:
		{
			/*Event_SystemThreadDestroyed d = { pThread->GetId() };
			MessageHelper evt(SYSTEM_THREAD_DESTROYED, &d, sizeof(d));
			IGameManager::GetInstance()->GetGame()->DispatchGlobalEvent(evt);*/

			IGameManager::GetInstance()->GetGame()->AddDeletedThread(pThread->GetId());
		}
	default:
		break;
	}
	return false;
}

static bool countThreadStatus(gmThread *a_thread, void *a_context)
{
	ThreadStatus *pThreadStatus = static_cast<ThreadStatus*>(a_context);
	switch(a_thread->GetState())
	{
	case gmThread::RUNNING:
	case gmThread::SYS_PENDING:
	case gmThread::SYS_YIELD:
		pThreadStatus->m_Running++;
		break;
	case gmThread::SLEEPING:
		pThreadStatus->m_Sleeping++;
		break;
	case gmThread::BLOCKED:
		pThreadStatus->m_Blocked++;
		break;
	case gmThread::KILLED:
	case gmThread::EXCEPTION:
	case gmThread::SYS_EXCEPTION:
		break;
	}
	return true;
}

void ScriptManager::ShowGMStats()
{
	if(m_ScriptEngine)
	{
		ThreadStatus st = {0,0,0};
		m_ScriptEngine->ForEachThread(countThreadStatus, &st);

		String fmtMemUsage = va("Current Memory Usage %s",Utils::FormatByteString(m_ScriptEngine->GetCurrentMemoryUsage()).c_str()).c_str();
		String fmtSoftMemLimit = va("Soft Memory Usage %s",Utils::FormatByteString(m_ScriptEngine->GetDesiredByteMemoryUsageSoft()).c_str()).c_str();
		String fmtHardMemLimit = va("Hard Memory Limit %s",Utils::FormatByteString(m_ScriptEngine->GetDesiredByteMemoryUsageHard()).c_str()).c_str();
		String fmtSysMemUsage = va("System Memory Usage %s",Utils::FormatByteString(m_ScriptEngine->GetSystemMemUsed()).c_str()).c_str();
		String fmtFullCollects = va("Full Collects %d",m_ScriptEngine->GetStatsGCNumFullCollects()).c_str();
		String fmtIncCollects = va("Inc Collects %d",m_ScriptEngine->GetStatsGCNumIncCollects()).c_str();
		String fmtGCWarnings = va("GC Warnings %d",m_ScriptEngine->GetStatsGCNumWarnings()).c_str();
		String fmtThreadInfo = va("Threads: %d, %d Running, %d Blocked, %d Sleeping",
			(st.m_Blocked+st.m_Running+st.m_Sleeping),
			st.m_Running,
			st.m_Blocked,
			st.m_Sleeping).c_str();

		EngineFuncs::ConsoleMessage("-- Script System Info --");
		EngineFuncs::ConsoleMessage(fmtMemUsage.c_str());
		EngineFuncs::ConsoleMessage(fmtSoftMemLimit.c_str());
		EngineFuncs::ConsoleMessage(fmtHardMemLimit.c_str());
		EngineFuncs::ConsoleMessage(fmtSysMemUsage.c_str());
		EngineFuncs::ConsoleMessage(fmtFullCollects.c_str());
		EngineFuncs::ConsoleMessage(fmtIncCollects.c_str());
		EngineFuncs::ConsoleMessage(fmtGCWarnings.c_str());
		EngineFuncs::ConsoleMessage(fmtThreadInfo.c_str());

		LOG(fmtMemUsage.c_str());
		LOG(fmtSoftMemLimit.c_str());
		LOG(fmtHardMemLimit.c_str());
		LOG(fmtSysMemUsage.c_str());
		LOG(fmtFullCollects.c_str());
		LOG(fmtIncCollects.c_str());
		LOG(fmtGCWarnings.c_str());
	}
	else
	{
		EngineFuncs::ConsoleError("No Script System!");
	}
}

void ScriptManager::GetAutoCompleteList(const String &_string, StringVector &_completions)
{
	//try
	//{
	//	String strCopy = _string;
	//	Utils::StringTrimCharacters(strCopy, "[]");

	//	boost::regex exp(strCopy + ".*", REGEX_OPTIONS);

	//	gmTableObject *pTable = m_ScriptEngine->GetGlobals();

	//	// Look into tables if the string contains them.
	//	String prefix, entry;
	//	obuint32 iLastDot = strCopy.find_last_of(".");
	//	if(iLastDot != strCopy.npos)
	//	{
	//		gmVariable v = m_ScriptEngine->Lookup(strCopy.substr(0,iLastDot).c_str());
	//		if(v.GetTableObjectSafe())
	//		{
	//			prefix = strCopy.substr(0,iLastDot+1);
	//			pTable = v.GetTableObjectSafe();
	//			exp = boost::regex(strCopy.substr(iLastDot+1) + ".*", REGEX_OPTIONS);
	//		}
	//	}

	//	const int bufsize = 256;
	//	char buffer[bufsize];

	//	gmTableIterator tIt;
	//	gmTableNode *pNode = pTable->GetFirst(tIt);
	//	while(pNode)
	//	{
	//		const char *pName = pNode->m_key.AsString(m_ScriptEngine, buffer, 1024);
	//		if(boost::regex_match(pName, exp))
	//		{
	//			switch(pNode->m_key.m_type)
	//			{
	//			case GM_STRING:
	//				_completions.push_back(prefix + pName);
	//				break;
	//			case GM_INT:
	//				_completions.push_back((String)va("%s[%s]", prefix.c_str(), pName));
	//				break;
	//			default:
	//				_completions.push_back(prefix + pName);
	//				break;
	//			}
	//		}
	//		pNode = pTable->GetNext(tIt);
	//	}
	//}
	//catch(const std::exception&e)
	//{
	//	e;
	//	OBASSERT(0, e.what());
	//}
}

void ScriptManager::InitCommands()
{
	Set("script_stats", "Shows scripting system memory usage/stats",
		CommandFunctorPtr(new CommandFunctorT<ScriptManager>(this, &ScriptManager::cmdScriptStats)));
	Set("script_collect", "Performs a garbage collection",
		CommandFunctorPtr(new CommandFunctorT<ScriptManager>(this, &ScriptManager::cmdScriptCollect)));
	Set("script_runfile", "Executes a specified script file",
		CommandFunctorPtr(new CommandFunctorT<ScriptManager>(this, &ScriptManager::cmdScriptRunFile)));
	Set("script_debug", "Enables/disables debug messages in the scripting system.",
		CommandFunctorPtr(new CommandFunctorT<ScriptManager>(this, &ScriptManager::cmdDebugScriptSystem)));
	Set("script_run", "Executes a string as a script snippet.",
		CommandFunctorPtr(new CommandFunctorT<ScriptManager>(this, &ScriptManager::cmdScriptExecute)));
	Set("script_docs", "Dumps a file of gm bound type info.",
		CommandFunctorPtr(new CommandFunctorT<ScriptManager>(this, &ScriptManager::cmdScriptWriteDocs)));
}

void ScriptManager::cmdScriptStats(const StringVector &_args)
{
	ShowGMStats();
}

void ScriptManager::cmdDebugScriptSystem(const StringVector &_args)
{
	if(_args.size() >= 2)
	{
		if(!m_DebugScripts && Utils::StringToTrue(_args[1]))
		{
			EngineFuncs::ConsoleMessage("Script Debug Messages On.");
			m_DebugScripts = true;
		}
		else if(m_DebugScripts && Utils::StringToFalse(_args[1]))
		{
			EngineFuncs::ConsoleMessage("Script Debug Messages Off.");
			m_DebugScripts = false;
		}
	}
}

void ScriptManager::cmdScriptCollect(const StringVector &_args)
{
	if(m_ScriptEngine)
	{
		EngineFuncs::ConsoleMessage("Before Collection");
		cmdScriptStats(_args);
		m_ScriptEngine->CollectGarbage(true);
		EngineFuncs::ConsoleMessage("After Collection");
		cmdScriptStats(_args);
	}
	else
	{
		EngineFuncs::ConsoleError("No Script System!");
	}
}

void ScriptManager::cmdScriptRunFile(const StringVector &_args)
{
	if(_args.size() >= 2)
	{
		try
		{
			int threadId;
			if(ExecuteFile(_args[1].c_str(), threadId))
				return;
		}
		catch(const std::exception & ex)
		{
			//ex;
			LOGCRIT("Filesystem Exception: "<<ex.what());
		}
	}

	EngineFuncs::ConsoleError("Error Running Script.");
}

void ScriptManager::cmdScriptExecute(const StringVector &_args)
{
	if(_args.size() >= 2)
	{
		try
		{
			String str;
			for(obuint32 i = 1; i < _args.size(); ++i)
			{
				str += " ";
				str += _args[i];
			}

			String::iterator sIt = str.begin();
			for(; sIt != str.end(); ++sIt)
			{
				if(*sIt == '\'')
					*sIt = '\"';
			}

			if(*str.rbegin() != ';')
				str.push_back(';');

			if(ExecuteString(str, NULL))
				return;
		}
		catch(const std::exception & ex)
		{
			//ex;
			LOGCRIT("Filesystem Exception: " << ex.what());
		}
	}

	EngineFuncs::ConsoleError("Error Running Script.");
}

void ScriptManager::cmdScriptWriteDocs(const StringVector &_args)
{
#if(GMBIND2_DOCUMENT_SUPPORT)
	DisableGCInScope gcEn(m_ScriptEngine);

	gmBind2::TableConstructor tc(m_ScriptEngine);
	tc.Push("Weapon");
		gmBind2::Class<Weapon>::GetPropertyTable(m_ScriptEngine,tc.Top());
	tc.Pop();
	tc.Push("FireMode");
		gmBind2::Class<Weapon::WeaponFireMode>::GetPropertyTable(m_ScriptEngine,tc.Top());
	tc.Pop();
	tc.Push("MapGoal");
		gmBind2::Class<MapGoal>::GetPropertyTable(m_ScriptEngine,tc.Top());
	tc.Pop();

#ifdef ENABLE_DEBUG_WINDOW
	gmBindDebugWindowLibraryDocs(m_ScriptEngine,tc);
#endif

	File f;
	if(f.OpenForWrite("user/docs.gm",File::Text))
		gmUtility::DumpTable(m_ScriptEngine,f,"Docs",tc.Root(),gmUtility::DUMP_RECURSE);
#endif
}

ScriptResource::ScriptResource() : m_Key(-1)
{
}
ScriptResource::~ScriptResource()
{
}
ScriptResource & ScriptResource::operator=(const ScriptResource &_rh)
{
	m_Key = _rh.m_Key;
	m_Script = _rh.m_Script;
	return *this;
}
bool ScriptResource::InitScriptSource(const filePath &_path)
{
	m_Script = _path;
	if(ScriptLiveUpdate)
		m_Key = ScriptManager::GetInstance()->RegisterLiveUpdate(_path);
	return true;
}

LiveUpdateKey ScriptManager::RegisterLiveUpdate(const filePath &_file)
{
	// check if it already exists.
	for(obuint32 i = 0; i < g_LiveUpdate.size(); ++i)
	{
		if(g_LiveUpdate[i].File == _file)
			return i;
	}

	// add it to the list.
	const LiveUpdateKey key = (int)g_LiveUpdate.size();

	LiveUpdateEntry entry(_file, FileSystem::FileModifiedTime(_file));
	g_LiveUpdate.push_back(entry);

	return key;
}

void ScriptManager::CheckLiveUpdates()
{
	if(IGame::GetTime() >= NextLiveUpdateCheck)
	{
		NextLiveUpdateCheck = IGame::GetTime() + 1000;

		for(obuint32 i = 0; i < g_LiveUpdate.size(); ++i)
		{
			LiveUpdateEntry &entry = g_LiveUpdate[i];
			const obint64 modTime = FileSystem::FileModifiedTime(entry.File);
			if(modTime > entry.FileModTime)
			{
				// send an event and update the local time.
				Event_SystemScriptUpdated d = { static_cast<obint32>(i) };
				MessageHelper evt(SYSTEM_SCRIPT_CHANGED, &d, sizeof(d));
				IGameManager::GetInstance()->GetGame()->DispatchGlobalEvent(evt);

				entry.FileModTime = modTime;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_REMOTE_DEBUGGER) && defined(GMDEBUG_SUPPORT)

struct PacketHeader {
	static const unsigned int MAGIC_NUM = 0xDEADB33F;

	const unsigned int magicNum;
	const unsigned int dataSize;
	PacketHeader( unsigned int sz ) : magicNum(MAGIC_NUM), dataSize( sz ) {}
};

String			g_RecieveBuffer;
String			g_LastCommand;

void ScriptManager::SendDebuggerMessage(const void * a_command, int a_len) {
	if(g_DebugClient.IsValid())
	{
		PacketHeader hdr( a_len );

		sf::Packet packet;
		packet.Append((const char *)&hdr, sizeof(hdr));
		packet.Append((const char *)a_command, a_len);
		if(g_DebugClient.Send(packet,false)==sf::Socket::Disconnected)
		{
			g_DebugClient.Close();
			return;
		}
		Utils::OutputDebug(kScript, "%d sent", packet.GetDataSize());
	}
}

const void * ScriptManager::PumpDebuggerMessage(int &a_len) {
	// grab all the data from the network we can
	enum { BufferSize = 4096 };
	char inBuffer[BufferSize];
	std::size_t inSize = 0;
	do
	{
		if(g_DebugClient.Receive(inBuffer,BufferSize,inSize)==sf::Socket::Done)
		{
			Utils::OutputDebug(kScript, va("%d recieved", inSize));
			if(inSize>0)
				g_RecieveBuffer.append(inBuffer, inSize);
		}
	} while ( inSize > 0 );

	if ( g_RecieveBuffer.size() > 0 ) {
		// see if we have an entire packets worth of data waiting
		const char * dataPtr = g_RecieveBuffer.c_str();
		const PacketHeader * hdr = reinterpret_cast<const PacketHeader *>(dataPtr);
		dataPtr += sizeof(PacketHeader);
		if ( hdr->magicNum != PacketHeader::MAGIC_NUM ) {
			// ERROR! Malformed Packet!
			return NULL;
		}

		if ( g_RecieveBuffer.size() >= hdr->dataSize+sizeof(PacketHeader) ) {
			a_len = hdr->dataSize;
			g_LastCommand.resize(0);
			g_LastCommand.append( dataPtr, hdr->dataSize );
			g_RecieveBuffer.erase( 0, hdr->dataSize + sizeof(PacketHeader) );
			return g_LastCommand.c_str();
		}
	}
	return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////

void ScriptManager::EnableRemoteDebugger(bool _enable)
{
#if defined(ENABLE_REMOTE_DEBUGGER) && defined(GMDEBUG_SUPPORT)
	if(_enable && !g_RemoteDebuggerInitialized)
	{
		if(g_DebugListener.Listen(GM_DEBUGGER_PORT))
		{
			g_DebugListener.SetBlocking(false);
			g_RemoteDebuggerInitialized = true;
		}
		else
		{
			//const char *pError = XPSock_TranslateErrorLong(g_ScriptDebuggerSocket.GetLastError());
			//LOGERR("Error Initializing Remote Debug Socket, disabling");
			//LOGERR(pError);
			g_RemoteDebuggerInitialized = false;
		}
	}
	else if(g_RemoteDebuggerInitialized)
	{
		// Shut it down!
		g_DebugListener.Close();
		g_RemoteDebuggerInitialized = false;
	}
#endif
}
