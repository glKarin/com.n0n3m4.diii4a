#ifndef __MAPGOAL_H__
#define __MAPGOAL_H__

#include "GoalManager.h"
#include "ScriptManager.h"
#include "PropertyBinding.h"
#include "TrackablePtr.h"
#include "gmGCRoot.h"
#include "gmbinder2.h"
class Client;

class GoalQueue;
class gmMachine;
class gmUserObject;

// class: MapGoal
//		removed from the triggermanager slots.
#ifdef ENABLE_DEBUG_WINDOW
class MapGoal : public PropertyBinding, public gcn::ActionListener
#else
class MapGoal : public PropertyBinding
#endif
{
public:
	//////////////////////////////////////////////////////////////////////////
#define PROPERTY_BOOL(name) \
public: \
	bool Get##name() const { return (m_##name); } \
	void Set##name(bool _b) { (m_##name) = _b; } \
private: \
	bool m_##name; \
public:
	//////////////////////////////////////////////////////////////////////////
#define PROPERTY_INT(name) \
public: \
	int Get##name() const { return (m_##name); } \
	void Set##name(int _b) { (m_##name) = _b; } \
private: \
	int m_##name; \
public:
	//////////////////////////////////////////////////////////////////////////
#define PROPERTY_FLOAT(name) \
public: \
	float Get##name() const { return (m_##name); } \
	void Set##name(float _b) { (m_##name) = _b; } \
private: \
	float m_##name; \
public:
	//////////////////////////////////////////////////////////////////////////
#define PROPERTY_BITFLAG64(name) \
public: \
	BitFlag64 Get##name() const { return (m_##name); } \
	void Set##name(BitFlag64 _b) { (m_##name) = _b; } \
private: \
	BitFlag64 m_##name; \
public:
//////////////////////////////////////////////////////////////////////////
#define PROPERTY_INIT(name, def) m_##name = def;
#define PROPERTY_PROPOGATE(name) m_##name =  _other->m_##name;
	//////////////////////////////////////////////////////////////////////////

	PROPERTY_BITFLAG64(DisableWithEntityFlag)
	PROPERTY_BITFLAG64(DeleteWithEntityFlag)
	PROPERTY_FLOAT(RenderHeight)
	PROPERTY_FLOAT(DefaultRenderRadius)
	PROPERTY_INT(DefaultDrawFlags)
	PROPERTY_BOOL(DeleteMe)
	PROPERTY_BOOL(DynamicPosition)
	PROPERTY_BOOL(DynamicOrientation)
	PROPERTY_BOOL(PropertiesBound)
	PROPERTY_BOOL(RemoveWithEntity)
	PROPERTY_BOOL(InterfaceGoal)
	PROPERTY_BOOL(Disabled)
	PROPERTY_BOOL(InUse)
	PROPERTY_BOOL(DisableForControllingTeam)
	PROPERTY_BOOL(DontSave)
	PROPERTY_BOOL(RenderGoal)
	PROPERTY_BOOL(RenderRoutes)
	PROPERTY_BOOL(CreateOnLoad)
	
	enum DefaultDrawFlags
	{
		DrawName,
		DrawGroup,
		DrawRole,
		DrawBounds,
		DrawRadius,
		DrawInitialAvail,
		DrawCurrentAvail,
		DrawCenterBounds,
		DrawDisabled,
		DrawSynced,
		DrawRandomUsePoint,
		DrawRangeLimit,
		DrawAll = -1,
	};

	typedef enum
	{
		TRACK_INPROGRESS,
		TRACK_INUSE,
		NUM_TRACK_CATS
	} TrackingCat;

	enum FunctionCallback 
	{
		ON_INIT,
		ON_UPDATE,
		ON_RENDER,
		ON_HELP,
		NUM_CALLBACKS
	};

	struct Route
	{
		MapGoalPtr	m_Start;
		MapGoalPtr	m_End;
		float		m_Weight;
	};
	typedef std::vector<Route> Routes;

	// function: GenerateName
	//		Generates a name for this goal, based on a name from the game
	//		or an auto generated name from the goal type
	void GenerateName(int _instance = 0, bool _skipdupecheck = false);

	// function: Init
	//		Initializes the starting state of the goal
	void InternalInitEntityState();
	bool InternalInit(gmGCRoot<gmTableObject> &_propmap, bool _newgoal);

	// function: Update
	//		Performs any updates for the map goal
	void Update();

	// function: SetAvailabilityFlags
	//		Sets the availability flags of this goal
	inline void SetAvailabilityTeams(int _flags) { m_AvailableTeams = BitFlag32(_flags); }

	inline void SetAvailabilityTeamsInit(int _flags) { m_AvailableTeamsInit = BitFlag32(_flags); }

	// function: SetAvailable
	//		Set the availability status for a certain team
	void SetAvailable(int _team, bool _available);

	// function: IsAvailable
	//		Check the availability status for a certain team
	bool IsAvailable(int _team) const;

	// function: SetAvailable
	//		Set the availability status for a certain team
	void SetAvailableInitial(int _team, bool _available);

	// function: IsAvailable
	//		Check the availability status for a certain team
	bool IsAvailableInitial(int _team) const;

	BitFlag32 GetAvailableFlags() const { return m_AvailableTeams; }

	// function: SetEntity
	//		Set the <GameEntity> that represents this goal
	inline void SetEntity(GameEntity _ent) { m_Entity = _ent; }

	// function: GetEntity
	//		Get the <GameEntity> that represents this goal
	inline const GameEntity GetEntity() const { return m_Entity; }

	// function: SetOwner
	//		Set the <GameEntity> that represents the owner
	inline void SetOwner(GameEntity _ent) { m_CurrentOwner = _ent; }

	// function: GetOwner
	//		Get the <GameEntity> that represents the owner
	inline const GameEntity GetOwner() const { return m_CurrentOwner; }

	// function: SetTagName
	//		Sets the tag name of this goal
	inline void SetTagName(const String &_name) { m_TagName = _name; }

	// function: GetTagName
	//		Gets the tag name of this goal
	inline const String &GetTagName() { return m_TagName; }

	// function: SetPosition
	//		Sets the position of this goal
	void SetPosition(const Vector3f &_pos);

	// function: GetPosition
	//		Gets the position of this goal
	const Vector3f &GetPosition();

	// function: GetPosition
	//		Gets the position of this goal
	const Vector3f &GetInterfacePosition() { return m_InterfacePosition; }

	// function: SetFacing
	//		Sets the facing of this goal
	void SetFacing(const Vector3f &_facing);

	// function: GetFacing
	//		Gets the facing of this goal
	Vector3f GetFacing();

	// function: SetMatrix
	//		Sets the matrix of this goal
	void SetMatrix(const Matrix3f &_mat);

	// function: GetMatrix
	//		Gets the matrix of this goal
	Matrix3f GetMatrix();

	// function: SetGoalBounds
	//		Sets the bounds of the goal.
	void SetGoalBounds(const AABB &_bounds);

	// function: GetGoalBounds
	//		Gets the bounds of the goal in world space.
	Box3f GetWorldBounds();

	// function: GetGoalBounds
	//		Gets the bounds of the goal.
	const AABB &GetLocalBounds() const;

	// function: SetRadius
	//		Sets the radius of this goal
	inline void SetRadius(float _rad) { m_Radius = _rad; }

	inline void SetMinRadius(float _rad) { m_MinRadius = _rad; }

	// function: GetRadius
	//		Gets the radius of this goal
	inline float GetRadius() const { return Mathf::Max(m_Radius, m_MinRadius); }

	// function: GetName
	//		Gets the name of this goal
	inline const String &GetName() const { return m_Name; }

	void SetGroupName(const String &_name) { m_GroupName = _name; }
	const String &GetGroupName() const { return m_GroupName; }

	void SetRoleMask(BitFlag32 _i) { m_RoleMask = _i; }
	BitFlag32 GetRoleMask() const { return m_RoleMask; }

	void SetDefaultPriority(obReal _f) { m_DefaultPriority = _f; }
	obReal GetDefaultPriority() const { return m_DefaultPriority; }

	obReal GetPriorityForClient(Client *_client);
	obReal GetPriorityForClass(int _teamid, int _classId);
	void ResetGoalPriorities();
	void SetPriorityForClass(int _teamid, int _classId, obReal _priority);

	// function: GetGoalType
	//		Gets the type identifier for this goal
	inline const String GetGoalType() const { return m_GoalType; }
	inline obuint32 GetGoalTypeHash() const { return m_GoalTypeHash; }
	
	// function: SetNavFlags
	//		Sets the navigation flags for this goal
	inline void SetNavFlags(NavFlags _flags) { m_NavFlags = _flags; }

	// function: GetFlags
	//		Gets the navigation flags for this goal
	inline NavFlags GetFlags() const { return m_NavFlags; } // deprecated


	inline void AddReference(int _cat, int _team)
	{
		OBASSERT(_team > 0 && _team <= Constants::MAX_TEAMS, "Invalid team");
		m_CurrentUsers[_cat][_team - 1]++;
	}
	inline void DelReference(int _cat, int _team)
	{
		OBASSERT(_team > 0 && _team <= Constants::MAX_TEAMS, "Invalid team");
		OBASSERT(m_CurrentUsers[_cat][_team - 1] > 0, "Counter got below 0!");
		m_CurrentUsers[_cat][_team - 1]--;
	}

	inline obint32 GetSlotsOpen(TrackingCat _cat, int _team)
	{
		obint32 curUsers = GetCurrentUsers(_cat, _team);
		return GetMaxUsers(_cat) - curUsers;
	}

	inline obint32 GetCurrentUsers(TrackingCat _cat, int _team)
	{
		OBASSERT(_cat < NUM_TRACK_CATS, "Invalid Tracking Category");
		OBASSERT(_team > 0 && _team <= Constants::MAX_TEAMS, "Invalid team");
		if (_cat < NUM_TRACK_CATS && _team > 0) {
			return m_CurrentUsers[_cat][_team - 1];
		}
		return 0;
	}

	inline void SetMaxUsers(TrackingCat _cat, obint32 _val)
	{
		OBASSERT(_cat < NUM_TRACK_CATS, "Invalid Tracking Category");
		if ( _cat < NUM_TRACK_CATS ) {
			m_MaxUsers[_cat] = _val;
		}
	}
	inline obint32 GetMaxUsers(TrackingCat _cat)
	{
		OBASSERT(_cat < NUM_TRACK_CATS, "Invalid Tracking Category");
		if ( _cat < NUM_TRACK_CATS ) {
			return m_MaxUsers[_cat];
		}
		return 0;
	}

	obint32 GetNumUsePoints() const { return (obint32)m_LocalUsePoints.size(); }
	void AddUsePoint(const Vector3f &_pos, bool _relative = false);
	Vector3f GetWorldUsePoint(obint32 _index = -1);
	Vec3 GetWorldUsePoint_Script(obint32 _index);
	void GetAllUsePoints(Vector3List &_pv);

	bool AddRoute_Script(const std::string &_start, const std::string &_end, float _weight);
	bool AddRoute(const MapGoalPtr &_routeStart, const MapGoalPtr &_midpt, float _weight);

	bool RouteTo(Client *_bot, DestinationVector &_dest, float _minradius = 0.f);

	Vector3f CalculateFarthestFacing();

	static void SetPersistentPriorityForClass(const String &_exp, int _team, int _class, float _priority);
	static void SetPersistentRole(const String &_exp, BitFlag32 _role);
	void CheckForPersistentProperty();
	obReal GetRolePriorityBonus() const { return m_RolePriorityBonus; };

	// function: GetSerialNum
	//		Unique id for this goal. Useful for blackboard target or other identification.
	inline obint32 GetSerialNum() const { return m_SerialNum; }

	void TriggerHandler(const TriggerInfo &) {};

	// function: GetGoalState
	//		Allows users to query a numeric state value from the goal.
	int GetGoalState() const { return m_GoalState; };

	// function: GetControllingTeam
	//		Gets the team currently controlling the goal.
	int GetControllingTeam(int) const { return 0; }

	void CheckPropertiesBound();

	void BindProperties();

	gmGCRoot<gmUserObject> GetScriptObject(gmMachine *_machine) const;

	const Routes &GetRoutes() const { return m_Routes; }

	void RenderDebug(bool _editing, bool _highlighted);
	void RenderDefault();

	void SetProperty(const String &_propname, const obUserData &_val);

	struct ClassPriority
	{
		enum { MaxTeams=4,MaxClasses=10 };
		obReal	Priorities[MaxTeams][MaxClasses];
		
		void GetPriorityText(String &_txtout, obReal roleBonus) const;
	};	

	void DrawRoute(int _color, float _duration);

	const ClassPriority &GetClassPriorities() const { return m_ClassPriority; }

	static void Bind(gmMachine *_m);

	enum GoalStateFunction
	{
		GoalStateNone,
		GoalStateFlagState,
		//GoalStateFlagHoldState,
	};

	void FromScriptTable(gmMachine *_machine, gmTableObject *_tbl, bool _caseSense = true);

	gmVariable GetProperty(const char *_name);

	bool GetProperty(const char *_name, Vector3f &_var);
	bool GetProperty(const char *_name, float &_var);
	bool GetProperty(const char *_name, bool &_var);
	bool GetProperty(const char *_name, int &_var);
	bool GetProperty(const char *_name, String &_var);
	bool GetProperty(const char *_name, Seconds &_var);

	bool SaveToTable(gmMachine *_machine, gmGCRoot<gmTableObject> &_savetable, ErrorObj &_err);
	bool LoadFromTable(gmMachine *_machine, gmGCRoot<gmTableObject> &_loadtable, ErrorObj &_err);

	MapGoalPtr GetSmartPtr();
	void SetSmartPtr(MapGoalPtr ptr);

	LimitWeapons &GetLimitWeapons() { return m_LimitWeapon; }

	void ShowHelp();

	void CopyFrom(MapGoal *_other);
	bool LoadFromFile( const filePath & _file );

	void SetProfilerZone(const String &_name);

	void CreateGuiFromBluePrint(gmMachine *a_machine, gmTableObject *a_schema);
	void HudDisplay();

	int GetRandomUsePoint() const { return m_RandomUsePoint; };

	int GetRange() const { return m_Range; };
	void SetRange(int _range) { m_Range = _range; };

#ifdef ENABLE_DEBUG_WINDOW
	// action listener
	void action(const gcn::ActionEvent& actionEvent);
#endif
	void CreateGuiFromSchema(gmMachine *a_machine, gmTableObject *a_schema);

#ifdef ENABLE_REMOTE_DEBUGGING
	void Sync( RemoteLib::DataBuffer & db, bool fullSync );

	typedef RemoteLib::SyncSnapshot<32> MapGoalSnapshot;
	MapGoalSnapshot	snapShot;
#endif

	static void SwapEntities(MapGoal *g1, MapGoal *g2);

	MapGoal(const char *_goaltype);
	~MapGoal();
private:
	String			m_GoalType;
	obuint32		m_GoalTypeHash;

	BitFlag32		m_AvailableTeams;
	BitFlag32		m_AvailableTeamsInit;
	int				m_ControllingTeam;

	BitFlag32		m_RoleMask;

	GameEntity		m_Entity;
	GameEntity		m_CurrentOwner;
	
	Vector3f		m_Position;
	Vector3f		m_InterfacePosition; //cache the auto detected position
	Matrix3f		m_Orientation;
	Vector3f		m_Euler;
	AABB			m_LocalBounds; // deprecated
	//BoundingBox	m_Bounds;

	LimitWeapons	m_LimitWeapon;

	float			m_Radius;
	float			m_MinRadius;
	float			m_MinRadiusInit;

	obint32			m_MaxUsers[NUM_TRACK_CATS];
	obint32			m_CurrentUsers[NUM_TRACK_CATS][Constants::MAX_TEAMS];

	obint32			m_SerialNum;

	NavFlags		m_NavFlags;
	
	Vector3List		m_LocalUsePoints;
	DynBitSet32		m_RelativeUsePoints;

	Routes			m_Routes;
	
	ClassPriority	m_ClassPriority;
	obReal			m_DefaultPriority;
	obReal			m_RolePriorityBonus;

	int				m_GoalStateFunction;
	int				m_GoalState;

	int				m_Version;

	bool			m_NeedsSynced;
	bool			m_OrientationValid;
	bool			m_EulerValid;

	// var: m_ScriptObject
	//		This objects script instance, so that the object can clear its script
	//		references when deleted.
	mutable gmGCRoot<gmUserObject> m_ScriptObject;

	gmGCRoot<gmFunctionObject>	m_InitNewFunc;
	gmGCRoot<gmFunctionObject>	m_UpgradeFunc;
	gmGCRoot<gmFunctionObject>	m_RenderFunc;
	gmGCRoot<gmFunctionObject>	m_SerializeFunc;
	gmGCRoot<gmFunctionObject>	m_SetPropertyFunc;
	gmGCRoot<gmFunctionObject>	m_HelpFunc;
	gmGCRoot<gmFunctionObject>	m_UpdateFunc;
	gmGCRoot<gmFunctionObject>	m_HudDisplay;
	ThreadScoper				m_ActiveThread[NUM_CALLBACKS];

	gmGCRoot<gmStringObject>	m_ExtraDebugText;

	String		m_TagName;
	String		m_Name;
	String		m_GroupName;

	int			m_RandomUsePoint; // randomly select a usepoint to use?
	int			m_Range;  // distance limited

#ifdef Prof_ENABLED
	Prof_Zone					*m_ProfZone;
#endif

	//////////////////////////////////////////////////////////////////////////
	// don't allow default ctr
	MapGoal();

	void _Init();

	// internal state updates
	void _UpdateFlagState();
	//void _UpdateFlagHoldState();
	void _CheckControllingTeam();

	// remove when all vectors converted to Vec3
	void SetPosition_Script(const Vec3 &_pos);
	void SetFacing_Script(const Vec3 &_pos);
	Vec3 GetPosition_Script();
	Vec3 GetFacing_Script();
	void SetBounds_Script(const Vec3 &_mins, const Vec3 &_maxs);
	Vec3 GetBoundsCenter_Script();

	void SetRange_Script(const int &_range);
	int GetRange_Script();

	MapGoalWPtr	m_WeakPtr;

	static bool pfnSetDotEx(gmThread * a_thread, MapGoal * a_goal, const char *a_key, gmVariable * a_operands);
	static bool pfnGetDotEx(gmThread * a_thread, MapGoal * a_goal, const char *a_key, gmVariable * a_operands);
};

typedef TrackablePtr<MapGoal, MapGoal::TRACK_INPROGRESS>	TrackInProgress;
typedef TrackablePtr<MapGoal, MapGoal::TRACK_INUSE>			TrackInUse;

struct Trackers
{
	TrackInProgress		InProgress;
	TrackInUse			InUse;

	void Reset()
	{
		InProgress.Reset();
		InUse.Reset();
	}
};

obint32 GetMapGoalSerial();

#endif

