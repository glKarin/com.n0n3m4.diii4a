#include "PrecompCommon.h"
#include "IGameManager.h"
#include "IGame.h"
#include "mdump.h"

#include "NavigationManager.h"
#include "GoalManager.h"
#include "TriggerManager.h"
#include "ScriptManager.h"
#include "Timer.h"
#include "Interprocess.h"
#include "DebugWindow.h"

#include "WeaponDatabase.h"
#include "FileSystem.h"
#include "FileDownloader.h"

//////////////////////////////////////////////////////////////////////////

IGameManager *IGameManager::m_Instance = 0;

IGame *CreateGameInstance();

IGameManager::IGameManager()
	: m_ScriptManager(0)
	, m_PathPlanner(0)
	, m_GoalManager(0)
	, m_Game(0)
#ifdef ENABLE_REMOTE_DEBUGGING
	, m_Remote( 0 )
#endif
{
	memset(&g_EngineFuncs, 0, sizeof(g_EngineFuncs));
}

omnibot_error IGameManager::CreateGame(IEngineInterface *_pEngineFuncs, int _version)
{
	MiniDumper::Init("Omni-bot");

	InitCommands();

	srand((unsigned int)time(NULL));

	g_EngineFuncs = _pEngineFuncs;
	m_InterfaceVersionNum = _version;

	// Attempt to create the proper instance of IGame based on the game requested.
	m_Game = CreateGameInstance();

	// Verify the version is correct.
	if(!m_Game->CheckVersion(_version))
	{
		OB_DELETE(m_Game);
		return BOT_ERROR_WRONGVERSION;
	}

	if(!FileSystem::InitFileSystem())
		return BOT_ERROR_FILESYSTEM;

	// Create log file
	int logSize = m_Game->GetLogSize();
	if(logSize > 0)
	{
		if(logSize < 2000000)
			g_Logger.FileSizeLimit() = logSize * 1024;
		fs::path logFolder = Utils::GetModFolder() / fs::path("logs");
		g_Logger.Start(
			(String)va("%s/omnibot_%s.log",
			fs::is_directory(logFolder) ? logFolder.string().c_str() : _pEngineFuncs->GetLogPath(),
			_pEngineFuncs->GetMapName()), logSize==0);
	}

	FileSystem::SetWriteDirectory(Utils::GetModFolder());

	FileSystem::MakeDirectory("user");
	FileSystem::MakeDirectory("nav");
	FileSystem::MakeDirectory("scripts");
	//FileSystem::MakeDirectory("gui");
	//FileSystem::MakeDirectory("gui/weapons");

	FileSystem::MountArchives("scripts");
	FileSystem::MountArchives("scripts/weapons");
	FileSystem::MountArchives("scripts/goals");
	FileSystem::MountArchives("user/download","nav");
	FileSystem::MountArchives("nav");
	FileSystem::MountArchives("global_scripts");
	FileSystem::MountArchives("user");

	//mount incomplete_navs to nav
	fs::path path = Utils::GetModFolder() / "incomplete_navs";
	FileSystem::Mount(path, "nav", FileSystem::MountLast);
	FileSystem::Mount(path/"priority_maps", "nav", FileSystem::MountLast);
	FileSystem::Mount(path/"with_script", "nav", FileSystem::MountLast);
	FileSystem::Mount(path/"no_script", "nav", FileSystem::MountLast);

	if(FileSystem::FileDelete("user/logged.gm"))
		EngineFuncs::ConsoleMessage("deleted user/logged.gm");

	Options::LoadConfigFile();

	//////////////////////////////////////////////////////////////////////////
	// logging options
	Options::SetValue("Log","LogInfo","true",false);
	Options::SetValue("Log","LogWarnings","true",false);
	Options::SetValue("Log","LogErrors","true",false);
	Options::SetValue("Log","LogCriticalErrors","true",false);

	g_Logger.LogMask() = 0;
	bool l = true;
	if(Options::GetValue("Log","LogInfo",l) && l)
		g_Logger.LogMask() |= Logger::LOG_INFO|Logger::LOG_INDENT|Logger::LOG_UNDENT|Logger::LOG_FLOW;
	if(Options::GetValue("Log","LogWarnings",l) && l)
		g_Logger.LogMask() |= Logger::LOG_WARN;
	if(Options::GetValue("Log","LogErrors",l) && l)
		g_Logger.LogMask() |= Logger::LOG_ERR;
	if(Options::GetValue("Log","LogCriticalErrors",l) && l)
		g_Logger.LogMask() |= Logger::LOG_CRIT;


	// Log FileSystem information
	LOGFUNCBLOCK;
	FileSystem::LogInit();

	//////////////////////////////////////////////////////////////////////////
	// Initialize the Scripting System.
	m_ScriptManager = ScriptManager::GetInstance();
	m_ScriptManager->Init();

	//////////////////////////////////////////////////////////////////////////
	// Set up global config options.
	//Options::SetValue("Settings","TickRate",20,false);

#ifdef WIN32
	Options::SetValue("Debug","DumpFileEnable",true,false);
	Options::SetValue("Debug","DumpFileDialog",true,false);
#endif

	Options::SetValue("Script","LiveUpdate",false,false);
	//Options::SetValue("Script","Debug","true",false);

#ifdef ENABLE_REMOTE_DEBUGGER
	Options::SetValue("Script","EnableRemoteDebugger","true",false);
#endif

#ifdef ENABLE_FILE_DOWNLOADER
	Options::SetValue("Downloader","Server","",false);
	Options::SetValue("Downloader","Script","",false);
	Options::SetValue("Downloader","DownloadMissingNav","true",false);
#endif

#ifdef ENABLE_REMOTE_DEBUGGING
	Options::SetValue("RemoteWindow","Enabled",0,false);
	Options::SetValue("RemoteWindow","Port",m_Remote.getPort(),false);
#endif

	//////////////////////////////////////////////////////////////////////////

#ifdef INTERPROCESS
	Options::SetValue("Debug Render", "EnableInterProcess", "true", false);
	bool EnableIpc = true;
	Options::GetValue("Debug Render","EnableInterProcess",EnableIpc);
	InterProcess::Enable(EnableIpc);
#endif

#ifdef ENABLE_REMOTE_DEBUGGING
	{
		int numConnections = 0;
		if(Options::GetValue("RemoteWindow","Enabled",numConnections)) {
			m_Remote.setMaxConnections( (unsigned short)numConnections );
		}
		int remotePort = m_Remote.getPort();
		if(Options::GetValue("RemoteWindow","Port",remotePort)) {
			m_Remote.setPort( (uint16)remotePort );
		}

		m_Remote.init( 8192, 4096, true );
	}
#endif

	// Create the requested path planner.
	if(NavigationManager::GetInstance()->CreatePathPlanner(m_Game->GetDefaultNavigator()))
	{
		m_PathPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
		m_PathPlanner->RegisterScriptFunctions(m_ScriptManager->GetMachine());
		LOG("Created Nav System : " << m_PathPlanner->GetPlannerName());
	}
	else
	{
		LOG("Unable to Create Nav System");
		return BOT_ERROR_CANTINITBOT;
	}

	// Create the goal manager.
	m_GoalManager = m_Game->GetGoalManager();
	if(m_GoalManager)
	{
		m_GoalManager->Init();
		LOG("Goal Manager Created.");
	}
	else
	{
		LOGERR("ERROR: Creating Goal Manager.");
		return BOT_ERROR_CANTINITBOT;
	}

	TriggerManager::GetInstance();

	// Initialize the game.
	if(m_Game && m_Game->Init())
	{
		LOG("Created Game Interface : " << m_Game->GetGameName());
		LOG("Game Interface : " << g_EngineFuncs->GetGameName());
		LOG("Mod Interface : " << g_EngineFuncs->GetModName());
	}
	else
	{
		LOGERR("Unable to CreateGame() : " << m_Game->GetGameName());
		return BOT_ERROR_CANTINITBOT;
	}

#ifdef ENABLE_FILE_DOWNLOADER
	FileDownloader::Init();
#endif

	// Load the waypoints for this map.
	if(m_PathPlanner->Load(String(g_EngineFuncs->GetMapName())))
	{
		EngineFuncs::ConsoleMessage("Loaded Waypoints.");
	}
	else
		EngineFuncs::ConsoleError("ERROR Loading Waypoints.");

	m_Game->LoadGoalScripts(true);

	return BOT_ERROR_NONE;
}

void IGameManager::UpdateGame()
{
	InterProcess::Update();

#ifdef ENABLE_DEBUG_WINDOW
	DebugWindow::Update();
#endif

	{
		Prof(Omnibot);

		m_Game->UpdateTime();
		m_ScriptManager->Update();
		m_PathPlanner->Update();
		m_Game->UpdateGame();
		m_GoalManager->Update();
		TriggerManager::GetInstance()->Update();

		{
			Prof(Processes);

			FunctorMap::iterator it = m_UpdateMap.begin();
			for(; it != m_UpdateMap.end(); )
			{
				if((*(*it).second)() == Function_Finished)
				{
					EngineFuncs::ConsoleMessage(va("Finished Process: %s", (*it).first.c_str()));
					m_UpdateMap.erase(it++);
				}
				else
					++it;
			}
		}

#ifdef ENABLE_REMOTE_DEBUGGING
		{
			Prof(RemoteSync);
			m_Remote.updateConnections( this );
			for( int i = 0; i < m_Remote.getNumConnections(); ++i ) {
				RemoteLib::Connection * conn = m_Remote.getConnection( i );
				if ( conn->isConnected() ) {
					RemoteLib::DataBuffer & sendBuffer = conn->getSendBuffer();
					// keep alive
					if ( IGame::GetTime() > conn->getUserData() ) {
						sendBuffer.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
						sendBuffer.startSizeHeader();
						sendBuffer.writeInt32( RemoteLib::ID_ack );
						sendBuffer.endSizeHeader();
						sendBuffer.endWrite();
						conn->setUserData( IGame::GetTime() + 5000 );
					}

					if ( conn->isNewConnection() ) {
						sendBuffer.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
						sendBuffer.startSizeHeader();
						sendBuffer.writeInt32( RemoteLib::ID_configName );
						sendBuffer.writeString( m_Game->RemoteConfigName() );
						sendBuffer.endSizeHeader();
						sendBuffer.endWrite();

						m_SnapShots.Clear();
					}

					m_Game->UpdateSync( m_SnapShots, sendBuffer );
					m_PathPlanner->Sync( sendBuffer, conn->isNewConnection() );
					m_GoalManager->Sync( sendBuffer, conn->isNewConnection() );
					conn->clearNewConnection();
				}
			}
		}
#endif
	}

	Options::SaveConfigFileIfChanged();

#ifdef ENABLE_FILE_DOWNLOADER
	FileDownloader::Poll();
#endif

	EngineFuncs::FlushAsyncMessages();
}

void IGameManager::Shutdown()
{
#ifdef ENABLE_FILE_DOWNLOADER
	FileDownloader::Shutdown();
#endif

#ifdef ENABLE_DEBUG_WINDOW
	DebugWindow::Destroy();
#endif
#ifdef ENABLE_REMOTE_DEBUGGING
	m_Remote.shutdown();
#endif

	m_Game->Shutdown();
	g_Blackboard.RemoveAllBBRecords();

	while(!m_UpdateMap.empty())
		RemoveUpdateFunction((*m_UpdateMap.begin()).first);

	LOGFUNCBLOCK;

	// Get rid of the path planner.
	NavigationManager::DeleteInstance();
	m_PathPlanner = 0;

	// Shutdown and clean up the goal manager.
	m_GoalManager->Shutdown();
	m_GoalManager = 0;
	GoalManager::DeleteInstance();

	TriggerManager::DeleteInstance();
	NameManager::DeleteInstance();

	// Shutdown and clean up the game.
	OB_DELETE(m_Game);
	LOG("Successfully Shut down Game Interface");

	g_WeaponDatabase.Unload();

	ScriptManager::GetInstance()->Shutdown();
	ScriptManager::DeleteInstance();

	InterProcess::Shutdown();

	MiniDumper::Shutdown();

	Options::Shutdown();

	FileSystem::ShutdownFileSystem();
}

IGameManager *IGameManager::GetInstance()
{
	if(!m_Instance)
		m_Instance = new IGameManager;
	return m_Instance;
}

void IGameManager::DeleteInstance()
{
	OB_DELETE(m_Instance);
}

void IGameManager::InitCommands()
{
	Set("version", "Prints out the bot version number.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdVersion)));
	Set("revision", "Prints out the bot version number.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdVersion)));
	Set("stopprocess", "Stops a process by its name.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdStopProcess)));
	Set("showprocesses", "Shows current proccesses.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdShowProcesses)));
	Set("navsystem", "Creates a navigation system of a specified type.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdNavSystem)));
	Set("printfs", "Prints the whole file system.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdPrintAllFiles)));

#ifdef ENABLE_FILE_DOWNLOADER
	Set("update_nav", "Checks the remote waypoint database for updated navigation.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdUpdateNavFile)));
	Set("update_all_nav", "Attempts to download all nav files from the database, including updating existing files.",
		CommandFunctorPtr(new CommandFunctorT<IGameManager>(this, &IGameManager::cmdUpdateAllNavFiles)));
#endif
}

void IGameManager::cmdVersion(const StringVector &_args)
{
	if(m_Game)
	{
#ifdef _DEBUG
		EngineFuncs::ConsoleMessage(va("Omni-Bot DEBUG Build : %s", m_Game->GetVersionDateTime()));
#else
		EngineFuncs::ConsoleMessage(va("Omni-Bot : %s", m_Game->GetVersionDateTime()));
#endif
		EngineFuncs::ConsoleMessage(va("Version : %s", m_Game->GetVersion()));
		EngineFuncs::ConsoleMessage(va("Interface # : %d", m_InterfaceVersionNum));
	}
}

void IGameManager::cmdShowProcesses(const StringVector &_args)
{
	EngineFuncs::ConsoleMessage(va("# Processes: %d!", m_UpdateMap.size()));
	FunctorMap::iterator it = m_UpdateMap.begin(), itEnd = m_UpdateMap.end();
	for(; it != itEnd; ++it)
	{
		EngineFuncs::ConsoleMessage(va("Process: %s!", (*it).first.c_str()));
	}
}

void IGameManager::cmdStopProcess(const StringVector &_args)
{
	if(!_args.empty())
	{
		RemoveUpdateFunction(_args[1]);
	}
}

void IGameManager::cmdNavSystem(const StringVector &_args)
{
	const char *strUsage[] =
	{
		"navsystem type[wp, navmesh, flood]",
		"> type: the type of navigation system to use",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);

	NavigatorID navId = NAVID_WP;
	if(_args[1] == "wp")
		navId = NAVID_WP;
	else if(_args[1] == "navmesh")
		navId = NAVID_NAVMESH;
	else if(_args[1] == "flood")
		navId = NAVID_FLOODFILL;
	else if(_args[1] == "recast")
		navId = NAVID_RECAST;
	else
	{
		PRINT_USAGE(strUsage);
		return;
	}

	if(!m_PathPlanner || m_PathPlanner->GetPlannerType() != navId)
	{
		NavigationManager::GetInstance()->DeletePathPlanner();

		// Create the requested path planner.
		if(NavigationManager::GetInstance()->CreatePathPlanner(navId))
		{
			m_PathPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
			m_PathPlanner->RegisterScriptFunctions(m_ScriptManager->GetMachine());
			EngineFuncs::ConsoleMessage("Navigation System created.");
		}
		else
		{
			EngineFuncs::ConsoleError("Unable to create Navigation System.");
		}
	}
}

void IGameManager::cmdPrintAllFiles(const StringVector &_args)
{
	FileSystem::EnumerateFiles("");
}

#ifdef ENABLE_FILE_DOWNLOADER
void IGameManager::cmdUpdateNavFile(const StringVector &_args)
{
	const char *strUsage[] =
	{
		"update_nav mapname",
		"> mapname: the name of the map to update",
	};

	CHECK_NUM_PARAMS(_args, 1, strUsage);
	OPTIONAL_STRING_PARAM(mapname, 1, g_EngineFuncs->GetMapName());

	FileDownloader::UpdateWaypoints(mapname);
}

void IGameManager::cmdUpdateAllNavFiles(const StringVector &_args)
{
	OPTIONAL_BOOL_PARAM(newstufftoo, 1, false);
	FileDownloader::UpdateAllWaypoints(newstufftoo);
}
#endif

//////////////////////////////////////////////////////////////////////////

bool IGameManager::AddUpdateFunction(const String &_name, FunctorPtr _func)
{
	if(m_UpdateMap.find(_name) != m_UpdateMap.end())
	{
		EngineFuncs::ConsoleError("That process is already running!");
		return false;
	}
	EngineFuncs::ConsoleMessage(va("Process %s has been started! ", _name.c_str()));
	m_UpdateMap.insert(std::make_pair(_name, _func));
	return true;
}

bool IGameManager::RemoveUpdateFunction(const String &_name)
{
	FunctorMap::iterator it = m_UpdateMap.find(_name);
	if(it != m_UpdateMap.end())
	{
		EngineFuncs::ConsoleMessage(va("Process %s has been stopped! ", _name.c_str()));
		m_UpdateMap.erase(_name.c_str());
		return true;
	}
	return false;
}

void IGameManager::SyncRemoteDelete( int entityHandle )
{
#ifdef ENABLE_REMOTE_DEBUGGING
	if ( m_Remote.getNumConnections() > 0 ) {
		enum { BufferSize = 128 };
		char buffer[BufferSize] = {};
		RemoteLib::DataBuffer db( buffer, BufferSize );

		db.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
		db.startSizeHeader();
		db.writeInt32( RemoteLib::ID_delete );
		db.writeInt32( entityHandle );
		db.endSizeHeader();
		db.endWrite();
		m_Remote.sendToAll( db );
	}
#endif
}

#ifdef ENABLE_REMOTE_DEBUGGING
void IGameManager::OnConnect( RemoteLib::Connection * conn ) {
	EngineFuncs::ConsoleMessage( va( "Connecting to %s", conn->getIp() ) );
}
void IGameManager::OnDisConnect( RemoteLib::Connection * conn ) {
	EngineFuncs::ConsoleMessage( va( "Remote disconnected %s", conn->getIp() ) );
}
void IGameManager::OnAcceptConnection( RemoteLib::Connection * conn ) {
	EngineFuncs::ConsoleMessage( va( "Remote connected to %s", conn->getIp() ) );
}
#endif // ENABLE_REMOTE_DEBUGGING
