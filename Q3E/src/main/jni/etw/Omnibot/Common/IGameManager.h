#ifndef __IGAMEMANAGER_H__
#define __IGAMEMANAGER_H__

#include "CommandReciever.h"
#include "ObjFunctor.h"

class IGame;
class GoalManager;
class ScriptManager;
class PathPlannerBase;

//////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_REMOTE_DEBUGGING

typedef RemoteLib::SyncSnapshot<32> EntitySnapShot;

struct RemoteSnapShots {
	EntitySnapShot			entitySnapShots[ Constants::MAX_ENTITIES ];

	void Clear() {
		for ( int i = 0; i < Constants::MAX_ENTITIES; ++i ) {
			entitySnapShots[ i ].Clear();
		}
	}
};
#endif
//////////////////////////////////////////////////////////////////////////

// interface: IGameManager
//		Handles most of the common functionality of the game manager. Mods should subclass
//		this in order to create their own instance of IGame from within the CreateGameFromGameID
//		function
class IGameManager : public CommandReciever
#ifdef ENABLE_REMOTE_DEBUGGING
	, public RemoteLib::ConnectionCallbacks
#endif
{
public:
	/*
		Function: CreateGame
				Initializes the bot library based on the provided game type
		Parameters:
				_pEngineFuncs - The filled in struct of functions from the game
				_version - The version number we're attempting to load.
		Returns:
				omnibot_error BOT_ERROR_NONE if successful
	*/
	omnibot_error CreateGame(IEngineInterface *_pEngineFuncs, int _version);
	
	/*
		Function: Shutdown
				Shuts down the bot library and frees memory		
		Returns:
				1 if successful, 0 if error.
	*/	
	void Shutdown();

	/*
		Function: UpdateGame
				Updates the bot library and allows the bots to perform their AI.
				This should be called at regular intervals moderately often.
	*/	
	void UpdateGame();

	inline IGame *GetGame()					{ return m_Game; } 	
	inline PathPlannerBase *GetNavSystem()	{ return m_PathPlanner; }
	inline int GetInterfaceVersionNum()	{ return m_InterfaceVersionNum; }

	bool AddUpdateFunction(const String &_name, FunctorPtr _func);
	bool RemoveUpdateFunction(const String &_name);

	void SyncRemoteDelete( int entityHandle );
	
	// Singleton Accessors
	static IGameManager *GetInstance();
	static void DeleteInstance();	
protected:

	ScriptManager						*m_ScriptManager;
	PathPlannerBase						*m_PathPlanner;
	GoalManager							*m_GoalManager;
	IGame								*m_Game;
	int	m_InterfaceVersionNum;

	FunctorMap							m_UpdateMap;

#ifdef ENABLE_REMOTE_DEBUGGING
	RemoteLib::ConnectionManagerServer	m_Remote;

	// todo: make this per connection
	RemoteSnapShots						m_SnapShots;

	void OnConnect( RemoteLib::Connection * conn );
	void OnDisConnect( RemoteLib::Connection * conn );
	void OnAcceptConnection( RemoteLib::Connection * conn );
#endif

	virtual void InitCommands();
	void cmdVersion(const StringVector &_args);
	void cmdNavSystem(const StringVector &_args);
	void cmdPrintAllFiles(const StringVector &_args);
#ifdef ENABLE_FILE_DOWNLOADER
	void cmdUpdateNavFile(const StringVector &_args);
	void cmdUpdateAllNavFiles(const StringVector &_args);
#endif
	void cmdShowProcesses(const StringVector &_args);
	void cmdStopProcess(const StringVector &_args);

	static IGameManager	*m_Instance;
	IGameManager();
	virtual ~IGameManager() {};
	IGameManager &operator=(const IGameManager&);
};

void ThreadAddMessage(const String &_msg);
void ThreadAddError(const String &_msg);

#endif
