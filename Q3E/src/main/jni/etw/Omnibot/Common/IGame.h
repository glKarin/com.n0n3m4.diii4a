#ifndef __IGAME_H__
#define __IGAME_H__

#include "CommandReciever.h"
#include "EventReciever.h"
#include "PathPlannerWaypoint.h"
#include "Weapon.h"

class State;
class Client;
class CheckCriteria;
class Regulator;
class GoalManager;
class PathPlannerBase;
class Network;
class gmMachine;
class gmTableObject;
class gmFunctionObject;

extern BlackBoard g_Blackboard;

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<Client> ClientPtr;
typedef std::weak_ptr<Client> ClientWPtr;
#else
typedef boost::shared_ptr<Client> ClientPtr;
typedef boost::weak_ptr<Client> ClientWPtr;
#endif

//typedef MessageDepot<Event_Sound,1024> SoundDepot;
//extern SoundDepot g_SoundDepot;

struct EntityInstance
{
	GameEntity						m_Entity;
	BitFlag32						m_EntityCategory;
	int								m_EntityClass;
	int								m_TimeStamp;
};

// class: IGame
//		This class provides common functionality for various game types.
//		Specific games will derive from this class to expand and implement
//		their required functionality.
class IGame : public CommandReciever, public EventReciever
{
public:
	struct GameVars 
	{
		float	mPlayerHeight;
		GameVars();
	};
	static const GameVars &GetGameVars() { return m_GameVars; }

	virtual bool Init();
	virtual void Shutdown();
	virtual void UpdateGame();
	virtual void StartGame();
	virtual void EndGame();
	virtual void NewRound();
	virtual void StartTraining();
	virtual void RegisterNavigationFlags(PathPlannerBase *_planner);
	virtual NavFlags DeprecatedNavigationFlags() const { return F_NAV_TEAMONLY | F_NAV_SNIPE | F_NAV_HEALTH | F_NAV_ARMOR | F_NAV_AMMO | F_NAV_DEFEND | F_NAV_ATTACK | F_NAV_SCRIPT | F_NAV_ROUTEPT; }
	virtual void RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck) { }
	virtual void InitMapScript();
	virtual void GetMapScriptFile(filePath &script) { };

	virtual void ClientJoined(const Event_SystemClientConnected *_msg);
	virtual void ClientLeft(const Event_SystemClientDisConnected *_msg);

	void UpdateTime();
	static inline obint32 GetTime()				{ return m_GameMsec; };
	static inline obReal GetTimeSecs()			{ return (float)m_GameMsec / 1000.f; };
	static inline obint32 GetDeltaTime()		{ return m_DeltaMsec; };
	static inline obReal GetDeltaTimeSecs()		{ return (float)m_DeltaMsec * 0.001f; };
	static inline obint32 GetTimeSinceStart()	{ return m_GameMsec - m_StartTimeMsec; };
	static inline obint32 GetFrameNumber()		{ return m_GameFrame; }
	static inline void SetTime(obint32 _newtime){ m_GameMsec = _newtime; }
	static inline obReal GetGravity()			{ return m_Gravity; }
	static inline bool GetCheatsEnabled()		{ return m_CheatsEnabled; }
	    
	void DispatchEvent(int _dest, const MessageHelper &_message);
	void DispatchGlobalEvent(const MessageHelper &_message);

	virtual const char *GetVersion() const;
	virtual const char *GetVersionDateTime() const;
	virtual int GetVersionNum() const = 0;
	virtual bool CheckVersion(int _version);
	virtual const char *GetGameName() const = 0;
	virtual const char *GetDLLName() const = 0;
	virtual const char *GetModSubFolder() const = 0;
	virtual const char *GetNavSubfolder() const = 0;
	virtual const char *GetScriptSubfolder() const = 0;
	virtual const char *GetGameDatabaseAbbrev() const = 0;
	virtual eNavigatorID GetDefaultNavigator() const { return NAVID_RECAST; }

	virtual bool ReadyForDebugWindow() const { return true; }
	virtual const char *IsDebugDrawSupported() const { return NULL; }

	ClientPtr GetClientByGameId(int _gameId);
	ClientPtr GetClientByIndex(int _index);
	
	virtual const char *FindClassName(obint32 _classId);
	virtual int FindWeaponId(obint32 _classId);
	
	// Mod specific subclasses.
	virtual GoalManager *GetGoalManager();
	virtual Client *CreateGameClient() = 0;

	virtual void GetWeaponEnumeration(const IntEnum *&_ptr, int &num) = 0;
	virtual void GetTeamEnumeration(const IntEnum *&_ptr, int &num) = 0;
	virtual void GetRoleEnumeration(const IntEnum *&_ptr, int &num);

	virtual void AddBot(Msg_Addbot &_addbot, bool _createnow = true);
	
	virtual void CheckServerSettings(bool managePlayers = true);

	inline bool DrawBlockableTests()				{ return m_bDrawBlockableTests; }

	void LoadGoalScripts(bool _clearold);
	void ReloadGoalScripts();

	virtual ClientPtr &GetClientFromCorrectedGameId(int _gameid);

	virtual bool CreateCriteria(gmThread *_thread, CheckCriteria &_criteria, StringStr &err);

	static GameState GetGameState() { return m_GameState; }
	static GameState GetLastGameState() { return m_LastGameState; }
	static bool GameStarted() { return m_GameState != GAME_STATE_INVALID; }

	// Game Entity Stuff
	class EntityIterator
	{
	public:
		friend class IGame;		
		operator bool() const;
		void Clear();
		EntityInstance &GetEnt() { return m_Current; }
		const EntityInstance &GetEnt() const { return m_Current; }
		const int GetIndex() const { return m_Index; }
		EntityIterator() {}
	private:
		EntityInstance	m_Current;
		int				m_Index;
	};

	static bool IsEntityValid(const GameEntity &_hnl);
	static bool IterateEntity(IGame::EntityIterator &_it);
	static void UpdateEntity(EntityInstance &_ent);

	void AddDeletedThread(int threadId);
	void PropogateDeletedThreads();

	// Debug Window
	int GetDebugWindowNumClients() const;
	ClientPtr GetDebugWindowClient(int index) const;

	virtual void InitGlobalStates() {}

	virtual bool AddWeaponId(const char * weaponName, int weaponId) { return false; }
	virtual int ConvertWeaponId(int weaponId) { return weaponId; }

	virtual const char * RemoteConfigName() const { return "Omnibot"; }

#ifdef ENABLE_REMOTE_DEBUGGING
	void UpdateSync( RemoteSnapShots & snapShots, RemoteLib::DataBuffer & db );
	void SyncEntity( const EntityInstance & ent, EntitySnapShot & snapShot, RemoteLib::DataBuffer & db );
	virtual void InternalSyncEntity( const EntityInstance & ent, EntitySnapShot & snapShot, RemoteLib::DataBuffer & db );
#endif

	virtual inline int GetLogSize() { return 0; }

	IGame();
	virtual ~IGame();
protected:	
	ClientPtr			m_ClientList[Constants::MAX_PLAYERS];
	
	State				*m_StateRoot;

	static int					m_MaxEntity;
	static EntityInstance		m_GameEntities[Constants::MAX_ENTITIES];

	static GameState	m_GameState;
	static GameState	m_LastGameState;

	static obint32		m_GameMsec;
	static obint32		m_DeltaMsec;
	static obint32		m_StartTimeMsec;
	static obint32		m_GameFrame;
	static obReal		m_Gravity;
	static bool			m_CheatsEnabled;
	static bool			m_BotJoining;

	static GameVars		m_GameVars;

	enum { MaxDeletedThreads = 1024 };
	int			m_DeletedThreads[MaxDeletedThreads];
	int			m_NumDeletedThreads;

	virtual void GetGameVars(GameVars &_gamevars) = 0;

	void CheckGameState();

	// Script support.
	void InitScriptSupport();
	virtual void InitScriptBinds(gmMachine *_machine) {};
	virtual void InitScriptTeams(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptWeapons(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptRoles(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptClasses(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptWeaponClasses(gmMachine *_machine, gmTableObject *_table, int weaponClassId);
	virtual void InitScriptSkills(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptItems(gmMachine *_machine, gmTableObject *_table) {};
	virtual void InitScriptEvents(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptPowerups(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptCategories(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptBotButtons(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptTraceMasks(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptContentFlags(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptSurfaceFlags(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptBlackboardKeys(gmMachine *_machine, gmTableObject *_table);
	virtual void InitScriptBuyMenu(gmMachine *_machine, gmTableObject *_table) {};
	virtual void InitDebugFlags(gmMachine *_machine, gmTableObject *_table);
	virtual void InitBoneIds(gmMachine *_machine, gmTableObject *_table);

	virtual void InitVoiceMacros(gmMachine *_machine, gmTableObject *_table) {};

	int	m_WeaponClassIdStart; // start value of weapon class id range

	// Commands
	virtual void InitCommands();
	bool UnhandledCommand(const StringVector &_args);
	void cmdRevision(const StringVector &_args);
	void cmdAddbot(const StringVector &_args);
	void cmdKickbot(const StringVector &_args);	
	void cmdDebugBot(const StringVector &_args);
	void cmdKickAll(const StringVector &_args);
	void cmdBotDontShoot(const StringVector &_args);
	void cmdDumpBlackboard(const StringVector &_args);
	void cmdReloadWeaponDatabase(const StringVector &_args);
	void cmdDrawBlockableTests(const StringVector &_args);
	void cmdPrintFileSystem(const StringVector &_args);
	void cmdTraceBenchmark(const StringVector &_args);

	// Server settings
	RegulatorPtr	m_SettingLimiter;
	bool			m_PlayersChanged;

	// Misc
	bool			m_bDrawBlockableTests;

	void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
};

#endif
