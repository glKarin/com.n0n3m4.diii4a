#ifndef __GOALMANAGER_H__
#define __GOALMANAGER_H__

#include "CommandReciever.h"

class Client;
class Waypoint;

// Title: GoalManager

enum { MapGoalVersion = 1 };

// typedef: MapGoalPtr
class MapGoal;
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<MapGoal> MapGoalPtr;
typedef std::weak_ptr<MapGoal> MapGoalWPtr;
#else
typedef boost::shared_ptr<MapGoal> MapGoalPtr;
typedef boost::weak_ptr<MapGoal> MapGoalWPtr;
#endif

// typedef: MapGoalList
//		A list of goal entities.
typedef std::vector<MapGoalPtr> MapGoalList;
typedef std::set<MapGoalPtr> MapGoalSet;

// class: GoalManager
//		The goal manager is responsible for keeping track of various goals,
//		from flags to capture points. Bots can request goals from the goal 
//		manager and the goal manager can assign goals to the bot based on
//		the needs of the game, and optionally the bot properties
class GoalManager : public CommandReciever
{
public:	
	//////////////////////////////////////////////////////////////////////////
	class Query
	{
	public:
		friend class GoalManager;

		enum SortType
		{
			SORT_BIAS,
			SORT_NONE,
			SORT_RANDOM_FULL,
			SORT_NAME
		};

		enum QueryError 
		{
			QueryOk,
			QueryBadNameExpression,
			QueryBadGroupExpression,
		};

		Query &AddType(obuint32 _type);
		Query &Team(int _team);
		Query &Bot(Client *_client);
		Query &TagName(const char *_tagname);
		Query &Sort(SortType _sort);
		Query &Expression(const char *_exp);
		Query &Group(const char *_exp);
		Query &RoleMask(obuint32 _i);
		Query &Ent(GameEntity _ent);
		Query &CheckInRadius(const Vector3f & pos, float radius);
		Query &CheckRangeProperty(bool checkRange);

		Query &NoFilters();
		Query &SkipDelayed(bool _skip);
		Query &SkipNoInProgress(bool _skip);
		Query &SkipNoInUse(bool _skip);
		Query &SkipInUse(bool _skip);

		QueryError GetError() const { return m_Error; }

		bool CheckForMatch(MapGoalPtr & mg);

		virtual void OnQueryStart();
		virtual void OnQueryFinish();
		virtual void OnMatch(MapGoalPtr & mg);

		bool GetBest(MapGoalPtr &_mg);

		Query(obuint32 _type = 0, Client *_client = 0);
		virtual ~Query() {}

		void FromTable(gmMachine *a_machine, gmTableObject *a_table);

		const char *QueryErrorString();

		MapGoalList		m_List;

		enum { MaxGoalTypes = 8 };
	private:
		int				m_NumTypes;
		obuint32		m_GoalTypeList[MaxGoalTypes];
		int				m_Team;
		BitFlag32 		m_RoleMask;
		Client *		m_Client;
		const char *	m_TagName;
		SortType		m_SortType;
		GameEntity		m_Entity;

		Vector3f		m_Position;
		float			m_Radius;

		String			m_NameExp;
		String			m_GroupExp;

		QueryError		m_Error;

		// filters
		bool			m_SkipNoInProgressSlots;
		bool			m_SkipNoInUseSlots;
		bool			m_SkipDelayed;
		bool			m_SkipInUse;
		bool			m_CheckInRadius;
		bool			m_CheckRangeProperty;
	};
	//////////////////////////////////////////////////////////////////////////
	void InitGameGoals();

	void Init();
	void Update();
	void Shutdown();
	void Reset();

	bool Load(const String &_map, ErrorObj &_err);
	bool Save(const String &_map, ErrorObj &_err);

	MapGoalPtr AddGoal(const MapGoalDef &goaldef);

	void RemoveGoalsByType(const char *_goaltype);
	void RemoveGoalByEntity(GameEntity _ent);
	
	void GetGoals(Query &_qry);
	int Iterate(const char *expression, std::function<void(MapGoal*)> action);
	MapGoalPtr GetGoal(const String &_goalname);
	MapGoalPtr GetGoal(int _serialNum);

	virtual void CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used = BitFlag64());
	void RegisterWaypointGoals(Waypoint *_wp, MapGoalDef *_def, int _num);

	const MapGoalList &GetGoalList() const { return m_MapGoalList; }
	
	static GoalManager *GetInstance();
	static void DeleteInstance();
	void UpdateGoalEntity( GameEntity oldent, GameEntity newent );
	void RemoveGoalByName( const char *_goalname );

#ifdef ENABLE_REMOTE_DEBUGGING
	virtual void Sync( RemoteLib::DataBuffer & db, bool fullSync );
#endif

	GoalManager();
	virtual ~GoalManager();
protected:

	// Commands
	void InitCommands();
	void cmdGoalShow(const StringVector &_args);
	void cmdGoalShowRoutes(const StringVector &_args);
	void cmdGoalDraw(const StringVector &_args);
	void cmdGoalDrawRoutes(const StringVector &_args);
	void cmdGoalLoad(const StringVector &_args);
	void cmdGoalSave(const StringVector &_args);
	void cmdGoalCreate(const StringVector &_args);
	void cmdGoalDelete(const StringVector &_args);
	void cmdGoalEdit(const StringVector &_args);
	void cmdGoalEditx(const StringVector &_args);
	void cmdGoalHelp(const StringVector &_args);
	void cmdGoalFinish(const StringVector &_args);
	void cmdGoalSetProperty(const StringVector &_args);
	void cmdGoalRemoveAll(const StringVector &_args);
	void cmdGoalMove(const StringVector &_args);

	enum EditMode 
	{
		EditNone,
		EditMove,
	};

	void AddGoal(MapGoalPtr newGoal);

	MapGoalPtr _GetGoalInRange(const Vector3f &_pos, float _radius, bool _onlydrawenabled);
	void _SetActiveGoal(MapGoalPtr _mg);
	void _UpdateEditModes();

	static GoalManager	*m_Instance;
private:
	MapGoalList		m_MapGoalList;

	MapGoalPtr		m_ActiveGoal;

	EditMode		m_EditMode;

	MapGoalPtr		m_HighlightedGoal;

	gmGCRoot<gmTableObject>	m_LoadedMapGoals;

	String	m_NavDir;

	void OnGoalDelete(const MapGoalPtr &_goal);
	void SwapNames();
};

#endif
