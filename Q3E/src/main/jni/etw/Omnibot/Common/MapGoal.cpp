#include "PrecompCommon.h"
#include "MapGoal.h"
#include "gmbinder2_class.h"
#include "ScriptManager.h"
#include "NavigationManager.h"
#include "gmSchemaLib.h"
#include "gmBot.h"
#include "gmBotLibrary.h"

#ifdef ENABLE_DEBUG_WINDOW
#include <guichan.hpp>
#endif

static obint32 sNextSerialNum = 0;

//////////////////////////////////////////////////////////////////////////

Prof_Define(MapGoal);

//////////////////////////////////////////////////////////////////////////
#define GETMACHINE() ScriptManager::GetInstance()->GetMachine()
//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_DEBUG_WINDOW
class MgListModel : public gcn::ListModel, public gcn::DeathListener
{
public:
	//gcn::DeathListener *getDeathListener() { return this; }
	void death(const gcn::Event& event)
	{
		delete this;
	}
	int getNumberOfElements()
	{
		return mTable ? mTable->Count() : 0;
	}
	bool getColumnTitle(int column, std::string &title, int &columnwidth)
	{
		/*if(mColumns)
		{
		gmTableObject *columnTable = mColumns->Get(gmVariable(column)).GetTableObjectSafe();
		if(columnTable)
		{
		const char *columnName = columnTable->Get(GETMACHINE(),"Name").GetCStringSafe();
		if(columnName)
		title = columnName;
		return columnName || columnTable->Get(GETMACHINE(),"Width").GetIntSafe(columnwidth);
		}
		}*/
		return false;
	}
	std::string getElementAt(int i, int column)
	{
		if(mTable)
		{
			char buffer[256] = {};
			gmVariable v = mTable->Get(gmVariable(i));
			gmTableObject *subtbl = v.GetTableObjectSafe();
			if(subtbl)
			{
				gmVariable v2 = subtbl->Get(gmVariable(column));
				return v2.AsString(GETMACHINE(), buffer, 256);
			}
			else
			{
				return v.AsString(GETMACHINE(), buffer, 256);
			}
		}
		return "";
	}
	MgListModel(const gmTableObject *a_tbl)
	{
		mTable.Set(const_cast<gmTableObject*>(a_tbl),GETMACHINE());
	}
	//////////////////////////////////////////////////////////////////////////
	gmGCRoot<gmTableObject>	mTable;
};
#endif

//////////////////////////////////////////////////////////////////////////

obint32 GetMapGoalSerial()
{
	return ++sNextSerialNum;
}

MapGoal::MapGoal(const char *_goaltype)
	: m_GoalType			(_goaltype?_goaltype:"")
	, m_GoalTypeHash		(_goaltype?Utils::MakeHash32(_goaltype):0)
{
	_Init();
}

void MapGoal::CopyFrom(MapGoal *_other)
{
	_Init();

	m_GoalType = _other->m_GoalType;
	m_Radius = _other->m_Radius;
	m_MinRadius = m_MinRadiusInit = _other->m_MinRadius;
	m_DefaultPriority = _other->m_DefaultPriority;
	m_RolePriorityBonus = _other->m_RolePriorityBonus;
	m_RandomUsePoint = _other->m_RandomUsePoint;
	m_Range = _other->m_Range;

	m_AvailableTeams = _other->m_AvailableTeams;

	m_RoleMask = _other->m_RoleMask;

	for(int i = 0; i < NUM_TRACK_CATS; ++i)
		m_MaxUsers[i] = _other->m_MaxUsers[i];

	// Copy goal priorities
	for(int t = 0; t < ClassPriority::MaxTeams; ++t)
	{
		for(int c = 0; c < ClassPriority::MaxClasses; ++c)
		{
			m_ClassPriority.Priorities[t][c] = _other->m_ClassPriority.Priorities[t][c];
		}
	}

	m_GoalStateFunction = _other->m_GoalStateFunction;

	m_Version = _other->m_Version;
	m_UpgradeFunc = _other->m_UpgradeFunc;
	m_RenderFunc = _other->m_RenderFunc;
	m_SerializeFunc = _other->m_SerializeFunc;
	m_InitNewFunc = _other->m_InitNewFunc;
	m_SetPropertyFunc = _other->m_SetPropertyFunc;
	m_HelpFunc = _other->m_HelpFunc;
	m_UpdateFunc = _other->m_UpdateFunc;
	m_HudDisplay = _other->m_HudDisplay;

	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
	gmBind2::Class<MapGoal>::CloneTable(pMachine,_other->GetScriptObject(pMachine),GetScriptObject(pMachine));

#ifdef Prof_ENABLED
	m_ProfZone = _other->m_ProfZone;
#endif

	// copy select flags.
	PROPERTY_PROPOGATE(DefaultDrawFlags);
	PROPERTY_PROPOGATE(RenderHeight);
	PROPERTY_PROPOGATE(DefaultRenderRadius);
	PROPERTY_PROPOGATE(DisableWithEntityFlag);
	PROPERTY_PROPOGATE(DeleteWithEntityFlag);
	PROPERTY_PROPOGATE(DisableForControllingTeam);
	PROPERTY_PROPOGATE(DynamicPosition);
	PROPERTY_PROPOGATE(DynamicOrientation);
	PROPERTY_PROPOGATE(RemoveWithEntity);
	PROPERTY_PROPOGATE(DontSave);
	PROPERTY_PROPOGATE(CreateOnLoad);
}

void MapGoal::_Init()
{
	m_AvailableTeams.ClearAll();

	m_Position = Vector3f::ZERO;
	m_InterfacePosition = Vector3f::ZERO;
	SetMatrix(Matrix3f::IDENTITY);
	m_LocalBounds = AABB(Vector3f::ZERO, Vector3f::ZERO);
	m_Radius = 0.f;
	m_MinRadius = 0.f;
	m_NavFlags = 0;
	m_SerialNum = 0;
	m_DefaultPriority = 1.f;
	m_RolePriorityBonus = 0.f;
	m_RandomUsePoint = 0;
	m_Range = 0;

	m_SerialNum = GetMapGoalSerial();

	// By default, max out the counters.
	for(int i = 0; i < NUM_TRACK_CATS; ++i)
		m_MaxUsers[i] = 10000;

	memset(&m_CurrentUsers, 0, sizeof(m_CurrentUsers));

	ResetGoalPriorities();

	m_RoleMask = BitFlag32(0);

	m_GoalStateFunction = GoalStateNone;
	m_GoalState = 0;

	m_Version = 0;

	m_ControllingTeam = 0;

	m_NeedsSynced = false;

#ifdef Prof_ENABLED
	m_ProfZone = 0;
#endif

	//////////////////////////////////////////////////////////////////////////
	// init flags
	PROPERTY_INIT(DefaultDrawFlags,DrawAll);
	PROPERTY_INIT(DefaultRenderRadius,2048.f);
	PROPERTY_INIT(RenderHeight,IGame::GetGameVars().mPlayerHeight*0.5f);
	PROPERTY_INIT(DeleteMe,false);
	PROPERTY_INIT(DynamicPosition,false);
	PROPERTY_INIT(DynamicOrientation,false);
	PROPERTY_INIT(PropertiesBound,false);
	PROPERTY_INIT(RemoveWithEntity,true);
	PROPERTY_INIT(InterfaceGoal,false);
	PROPERTY_INIT(Disabled,false);
	PROPERTY_INIT(InUse,false);
	PROPERTY_INIT(DisableForControllingTeam,false);
	PROPERTY_INIT(DontSave,false);
	PROPERTY_INIT(RenderGoal,false);
	PROPERTY_INIT(RenderRoutes,false);
	PROPERTY_INIT(CreateOnLoad,true);
}

MapGoal::~MapGoal()
{
	//IGameManager::GetInstance()->SyncRemoteDelete( va( "MapGoal:%s", GetName().c_str() ) );

	gmBind2::Class<MapGoal>::NullifyUserObject(m_ScriptObject);
}

gmGCRoot<gmUserObject> MapGoal::GetScriptObject(gmMachine *_machine) const
{
	if(!m_ScriptObject)
		m_ScriptObject = gmBind2::Class<MapGoal>::WrapObject(_machine,const_cast<MapGoal*>(this),true);
	return m_ScriptObject;
}

void MapGoal::SetProfilerZone(const String &_name)
{
#ifdef Prof_ENABLED
	m_ProfZone = gDynamicZones.FindZone(_name.c_str());
	OBASSERT(m_ProfZone,"No Profiler Zone Available!");
#endif
}

MapGoalPtr MapGoal::GetSmartPtr()
{
	MapGoalPtr ptr = m_WeakPtr.lock();
	return ptr;
}

void MapGoal::SetSmartPtr(MapGoalPtr ptr)
{
	m_WeakPtr = ptr;
}

bool MapGoal::LoadFromFile( const filePath & _file )
{
	gmGCRoot<gmUserObject> mgref = GetScriptObject(ScriptManager::GetInstance()->GetMachine());
	gmVariable varThis(mgref);

	int iThreadId;
	if(ScriptManager::GetInstance()->ExecuteFile(_file, iThreadId, &varThis))
	{
		return true;
	}
	else
	{
		OBASSERT(0, "Error Running MapGoal Script");
	}

	return false;
}

void MapGoal::GenerateName(int _instance, bool _skipdupecheck)
{
	obint32 iNavId = g_EngineFuncs->IDFromEntity(GetEntity());
	if(m_TagName.empty())
	{
		String sNavName;
		PathPlannerBase *pBase = NavigationManager::GetInstance()->GetCurrentPathPlanner();
		pBase->GetNavInfo(GetPosition(),iNavId,m_TagName);
	}

	String gtype = GetGoalType();
	std::transform(gtype.begin(),gtype.end(),gtype.begin(),toUpper());

	if(!m_TagName.empty())
		m_Name = va("%s_%s",gtype.c_str(),m_TagName.c_str());
	else
	{
		m_Name = va("%s_%d",gtype.c_str(),iNavId);
	}
#if __cplusplus >= 201103L //karin: not replace_all algorithm in C++17, but this only replace a character
	std::replace(m_Name.begin(), m_Name.end(), ' ', '_');
#else
	boost::replace_all(m_Name, " ", "_");
#endif

	//remove invalid characters
	for(const char *s = m_Name.c_str(); *s; s++)
	{
		char c = *s;
		if(!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
		{
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
			static std::regex re("[^A-Za-z0-9_]");
			m_Name = std::regex_replace(m_Name, re, "");
#else
			static boost::regex re("[^A-Za-z0-9_]");
			m_Name = boost::regex_replace(m_Name, re, "");
#endif
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Dupe name handling, append an instance number
	if(_instance > 0)
	{
		m_Name += va("_%d",_instance);
	}

	// see if it already exists
	if(!_skipdupecheck)
	{
		MapGoalPtr exists = GoalManager::GetInstance()->GetGoal(m_Name);
		if(exists && exists.get() != this)
		{
			GenerateName(_instance+1);
		}
	}
	CheckForPersistentProperty();
}

bool MapGoal::IsAvailable(int _team) const
{
	if(GetDisabled())
		return false;
	if(GetDisableForControllingTeam() && m_ControllingTeam==_team)
		return false;
	return m_AvailableTeams.CheckFlag(_team) && !GetDeleteMe();
}

void MapGoal::SetAvailable(int _team, bool _available)
{
	if (_team == 0)
	{
	    for(int t = 1; t <= 4; ++t)
		    m_AvailableTeams.SetFlag(t,_available);
	}
	else
	{
	    m_AvailableTeams.SetFlag(_team,_available);
	}
}

bool MapGoal::IsAvailableInitial(int _team) const
{
	return m_AvailableTeamsInit.CheckFlag(_team);
}

void MapGoal::SetAvailableInitial(int _team, bool _available)
{
	if (_team == 0)
	{
	    for(int t = 1; t <= 4; ++t)
		    m_AvailableTeamsInit.SetFlag(t,_available);
	}
	else
	{
	    m_AvailableTeamsInit.SetFlag(_team,_available);
	}
}

void MapGoal::Update()
{
	Prof_Scope(MapGoal);

#ifdef Prof_ENABLED
	Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

	{
		Prof(Update);

		//////////////////////////////////////////////////////////////////////////
		if(GetEntity().IsValid())
		{
			if(GetRemoveWithEntity())
			{
				if(!IGame::IsEntityValid(GetEntity()))
				{
					SetDeleteMe(true);
					return;
				}
			}
			if(GetDeleteWithEntityFlag().AnyFlagSet())
			{
				BitFlag64 bf;
				InterfaceFuncs::GetEntityFlags(GetEntity(),bf);
				if((bf & GetDeleteWithEntityFlag()).AnyFlagSet())
				{
					SetDeleteMe(true);
					return;
				}
			}
			if(GetDisableWithEntityFlag().AnyFlagSet())
			{
				BitFlag64 bf;
				InterfaceFuncs::GetEntityFlags(GetEntity(),bf);
				if((bf & GetDisableWithEntityFlag()).AnyFlagSet())
				{
					SetDisabled(true);
				}
				else
				{
					SetDisabled(false);
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////
		_CheckControllingTeam();
		//////////////////////////////////////////////////////////////////////////
		switch(m_GoalStateFunction)
		{
		case GoalStateFlagState:
			_UpdateFlagState();
			break;
			/*case GoalStateFlagHoldState:
			_UpdateFlagHoldState();
			break;*/
		case GoalStateNone:
		default:
			break;
		}
		//////////////////////////////////////////////////////////////////////////
		if(m_UpdateFunc)
		{
			if(!m_ActiveThread[ON_UPDATE].IsActive())
			{
				gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
				gmCall call;
				gmGCRoot<gmUserObject> mgref = GetScriptObject(pMachine);
				if(call.BeginFunction(pMachine, m_UpdateFunc, gmVariable(mgref)))
				{
					if(call.End() == gmThread::EXCEPTION)
					{
					}

					m_ActiveThread[ON_UPDATE] = call.GetThreadId();
					if(call.DidReturnVariable())
						m_ActiveThread[ON_UPDATE] = 0;
				}
			}
		}
	}
}

void MapGoal::_UpdateFlagState()
{
	GameEntity owner;
	FlagState newFlagState;
	if(InterfaceFuncs::GetFlagState(GetEntity(), newFlagState, owner))
	{
		SetOwner(owner);

		if(newFlagState != m_GoalState)
		{
			const char *pFlagState = 0;
			switch(newFlagState)
			{
			case S_FLAG_NOT_A_FLAG:
				pFlagState = "deleted";
				break;
			case S_FLAG_AT_BASE:
				pFlagState = "returned";
				break;
			case S_FLAG_DROPPED:
				pFlagState = "dropped";
				break;
			case S_FLAG_CARRIED:
				pFlagState = "pickedup";
				break;
			case S_FLAG_UNAVAILABLE:
				pFlagState = "unavailable";
				break;
			case S_FLAG_UNKNOWN:
				pFlagState = "unknown";
				break;
			}

			if(pFlagState)
			{
				TriggerInfo ti;
				ti.m_Entity = GetEntity();
				ti.m_Activator = owner;
				Utils::VarArgs(ti.m_TagName, TriggerBufferSize, "Flag %s %s", GetTagName().c_str(), pFlagState);
				Utils::StringCopy(ti.m_Action, pFlagState, TriggerBufferSize);
				TriggerManager::GetInstance()->HandleTrigger(ti);
			}
			else
			{
				OBASSERT(0, "Invalid Flag State");
			}
			m_GoalState = newFlagState;
		}
	}
}

//void MapGoal::_UpdateFlagHoldState()
//{
//	if(GetEntity().IsValid())
//	{
//		const int newControllingTeam = InterfaceFuncs::GetControllingTeam(GetEntity());
//		if(newControllingTeam != m_GoalState)
//		{
//			TriggerInfo ti;
//			ti.m_Entity = GetEntity();
//			ti.m_Activator = GameEntity();
//			varArgs(ti.m_TagName,
//				TriggerBufferSize,
//				"%s to team %d",
//				GetName().c_str(),
//				newControllingTeam);
//			Utils::StringCopy(ti.m_Action, "controlling team", TriggerBufferSize);
//			TriggerManager::GetInstance()->HandleTrigger(ti);
//
//			m_GoalState = newControllingTeam;
//		}
//	}
//}

void MapGoal::_CheckControllingTeam()
{
	if(GetEntity().IsValid())
	{
		const int newControllingTeam = InterfaceFuncs::GetControllingTeam(GetEntity());
		if(newControllingTeam != m_ControllingTeam)
		{
			m_ControllingTeam = newControllingTeam;

			TriggerInfo ti;
			ti.m_Entity = GetEntity();
			ti.m_Activator = GameEntity();
			Utils::VarArgs(ti.m_TagName,
				TriggerBufferSize,
				"%s to team %d",
				GetName().c_str(),
				newControllingTeam);
			Utils::StringCopy(ti.m_Action, "controlling team", TriggerBufferSize);
			TriggerManager::GetInstance()->HandleTrigger(ti);
		}
	}
}

void MapGoal::InternalInitEntityState()
{
	// cache the values.
	if(GetEntity().IsValid())
	{
		AABB worldbounds;
		bool b1 = EngineFuncs::EntityWorldAABB(GetEntity(), worldbounds);
		bool b2 = EngineFuncs::EntityPosition(GetEntity(), m_Position);

		// cache the auto detected position
		if(b2) m_InterfacePosition = m_Position;

		worldbounds.UnTranslate(m_Position);
		if(b1) m_LocalBounds = worldbounds;

		Vector3f vFwd, vRight, vUp;
		bool b3 = EngineFuncs::EntityOrientation(GetEntity(), vFwd, vRight, vUp);
		if(b3) SetMatrix(Matrix3f(vRight, vFwd, vUp, true));

		OBASSERT(b1&&b2&&b3,"Lost Entity!");
	}

	if(m_LocalBounds.IsZero())
	{
		m_LocalBounds.Expand(5.f);
	}
}

bool MapGoal::InternalInit(gmGCRoot<gmTableObject> &_propmap, bool _newgoal)
{
	CheckPropertiesBound();

	//////////////////////////////////////////////////////////////////////////
	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

	//////////////////////////////////////////////////////////////////////////
	if(_newgoal)
	{
		if(m_InitNewFunc)
		{
			gmCall call;

			gmGCRoot<gmUserObject> mgref = GetScriptObject(pMachine);
			if(call.BeginFunction(pMachine, m_InitNewFunc, gmVariable(mgref)))
			{
				call.AddParamTable(_propmap);
				call.End();
			}
		}
	}
	else
	{
		if(m_UpgradeFunc)
		{
			for(;;)
			{
				gmCall call;

				gmGCRoot<gmUserObject> mgref = GetScriptObject(pMachine);
				if(call.BeginFunction(pMachine, m_UpgradeFunc, gmVariable(mgref)))
				{
					gmVariable gmVersionBefore = _propmap->Get(pMachine,"Version");
					call.AddParamTable(_propmap);
					const int ThreadState = call.End();
					gmVariable gmVersionAfter = _propmap->Get(pMachine,"Version");

					//meh
					_propmap->Set(pMachine,"OldType",gmVariable::s_null);

					if(!gmVersionBefore.IsInt() ||
						!gmVersionAfter.IsInt() ||
						ThreadState==gmThread::EXCEPTION)
					{
						EngineFuncs::ConsoleMessage(va("%s goal could not upgrade properly, disabling",
							GetName().c_str()));
						SetDisabled(true);
						return false;
					}
					else if(gmVersionBefore.GetInt() != gmVersionAfter.GetInt())
					{
						//EngineFuncs::ConsoleMessage("%s goal updated, version %d to %d",
						//	GetName().c_str(),gmVersionBefore.GetInt(),gmVersionAfter.GetInt());
					}
					else if(gmVersionAfter.GetInt() == m_Version)
					{
						//EngineFuncs::ConsoleMessage("%s goal up to date, version %d",
						//	GetName().c_str(),gmVersionAfter.GetInt());
						break;
					}
					else
					{
						EngineFuncs::ConsoleMessage(va("%s goal could not upgrade properly, disabling",
							GetName().c_str()));
						SetDisabled(true);
						return false;
					}
				}
			}
		}
	}
	return true;
}

void MapGoal::SetPosition(const Vector3f &_pos)
{
	m_Position = _pos;
}

const Vector3f &MapGoal::GetPosition()
{
	if(GetDynamicPosition())
	{
		bool b = EngineFuncs::EntityPosition(GetEntity(), m_Position);
		SOFTASSERTALWAYS(b,"Lost Entity for MapGoal %s!", GetName().c_str());
	}
	return m_Position;
}

void MapGoal::SetPosition_Script(const Vec3 &_pos)
{
	m_Position = Vector3f(_pos.x,_pos.y,_pos.z);
}

Vec3 MapGoal::GetPosition_Script()
{
	return Vec3(GetPosition());
}

void MapGoal::SetRange_Script(const int &_range)
{
	m_Range = _range;
}

int MapGoal::GetRange_Script()
{
	return GetRange();
}

void MapGoal::SetFacing(const Vector3f &_facing)
{
	SetMatrix(Matrix3f(_facing.Cross(Vector3f::UNIT_Z), _facing, Vector3f::UNIT_Z, true));
}

void MapGoal::SetFacing_Script(const Vec3 &_facing)
{
	Vector3f facing(_facing.x,_facing.y,_facing.z);
	SetFacing(facing);
}


Vector3f MapGoal::GetFacing()
{
	return GetMatrix().GetColumn(1);
}

Vec3 MapGoal::GetFacing_Script()
{
	return Vec3(GetFacing());
}

void MapGoal::SetMatrix(const Matrix3f &_mat)
{
	m_Orientation = _mat;
	m_OrientationValid = true;
	m_EulerValid = false;
}

Matrix3f MapGoal::GetMatrix()
{
	if(GetDynamicOrientation())
	{
		Vector3f vFwd, vRight, vUp;
		bool b = EngineFuncs::EntityOrientation(GetEntity(), vFwd, vRight, vUp);
		OBASSERT(b,"Lost Entity!");
		if(b) SetMatrix(Matrix3f(vRight, vFwd, vUp, true));
	}

	if(!m_OrientationValid){
		m_Orientation.FromEulerAnglesZXY(m_Euler.x, m_Euler.y, m_Euler.z);
		m_OrientationValid = true;
	}
	return m_Orientation;
}

void MapGoal::SetGoalBounds(const AABB &_bounds)
{
	m_LocalBounds = _bounds;
}

void MapGoal::SetBounds_Script(const Vec3 &_mins, const Vec3 &_maxs)
{
	for(int i = 0; i < 3; ++i)
	{
		m_LocalBounds.m_Mins[i] = _mins[i];
		m_LocalBounds.m_Maxs[i] = _maxs[i];
	}
}

Vec3 MapGoal::GetBoundsCenter_Script()
{
	Box3f box = GetWorldBounds();
	return Vec3(box.Center);
}

Box3f MapGoal::GetWorldBounds()
{
	Box3f obb;
	obb.Identity( 8.f );
	obb.Center = GetPosition();
	EngineFuncs::EntityOrientation(GetEntity(), obb.Axis[0], obb.Axis[1], obb.Axis[2]);
	EngineFuncs::EntityWorldOBB( GetEntity(), obb );
	return obb;
}

const AABB &MapGoal::GetLocalBounds() const
{
	return m_LocalBounds;
}

void MapGoal::AddUsePoint(const Vector3f &_pos, bool _relative)
{
	m_LocalUsePoints.resize(m_LocalUsePoints.size()+1);
	m_LocalUsePoints[m_LocalUsePoints.size()-1] = _pos;

	m_RelativeUsePoints.resize(m_LocalUsePoints.size());
	m_RelativeUsePoints.set(m_RelativeUsePoints.size()-1, _relative);
}

Vector3f MapGoal::GetWorldUsePoint(obint32 _index)
{
	if(!m_LocalUsePoints.empty())
	{
		if(_index == -1 || _index < 0 || _index >= (obint32)m_LocalUsePoints.size())
		{
			int iRand = Mathf::IntervalRandomInt(0, (int)m_LocalUsePoints.size());

			Vector3f vUsePt = m_LocalUsePoints[iRand];

			if(m_RelativeUsePoints.test(iRand))
			{
				vUsePt = GetPosition() + GetMatrix() * vUsePt;
			}
			return vUsePt;
		}

		Vector3f vUsePt = m_LocalUsePoints[_index];

		if(m_RelativeUsePoints.test(_index))
		{
			vUsePt = GetPosition() + GetMatrix() * vUsePt;
		}
		return vUsePt;
	}
	return GetPosition();
}

Vec3 MapGoal::GetWorldUsePoint_Script(obint32 _index)
{
	return Vec3(GetWorldUsePoint(_index));
}

void MapGoal::GetAllUsePoints(Vector3List &_pv)
{
	_pv.reserve(GetNumUsePoints());
	for(int i = 0; i < GetNumUsePoints(); ++i)
	{
		_pv.push_back(GetWorldUsePoint(i));
	}
}

bool MapGoal::AddRoute_Script(const std::string &_start, const std::string &_end, float _weight)
{
	MapGoalPtr mgStart = GoalManager::GetInstance()->GetGoal(_start);
	MapGoalPtr mgEnd = GoalManager::GetInstance()->GetGoal(_end);
	return AddRoute(mgStart,mgEnd,_weight);
}

bool MapGoal::AddRoute(const MapGoalPtr &_routeStart, const MapGoalPtr &_midpt, float _weight)
{
	if(_routeStart && _midpt)
	{
		// find whether this route already exists
		Routes::const_iterator cIt = m_Routes.begin(), cItEnd = m_Routes.end();
		for(; cIt != cItEnd; ++cIt)
		{
			const Route &o = (*cIt);
			if(o.m_Start == _routeStart && o.m_End == _midpt)
				return true;
		}

		Route r;
		r.m_Start = _routeStart;
		r.m_End = _midpt;
		r.m_Weight = _weight;

		m_Routes.reserve(m_Routes.size()+1);
		m_Routes.push_back(r);
		return true;
	}
	return false;
}

bool MapGoal::RouteTo(Client *_bot, DestinationVector &_dest, float _minradius)
{
	Routes routes;

	float fTolerance = _bot->GetWorldBounds().Extent[2];

	float fTotalWeight = 0.f;
	Routes::const_iterator cIt = m_Routes.begin(), cItEnd = m_Routes.end();
	for(; cIt != cItEnd; ++cIt)
	{
		const Route &r = (*cIt);

		// RouteTo can be called when a bot is not yet within ROUTE goal radius,
		// because FollowPath calculates 2D distance (that is less than 3D distance).
		// Omni-bot 0.81 compared 3D distance here and skipped ROUTE goals on stairs, hillsides etc.
		Vector3f vDist = _bot->GetPosition() - r.m_Start->GetPosition();
		if( vDist.x * vDist.x + vDist.y * vDist.y <= Mathf::Sqr(r.m_Start->GetRadius()) &&
			abs(vDist.z - fTolerance) - 2 * fTolerance <= r.m_Start->GetRadius() &&

			// Most maps have all routes enabled for all teams,
			// that's why it's better to check availability after checking radius.
			r.m_End->IsAvailable(_bot->GetTeam()) &&
			r.m_Start->IsAvailable(_bot->GetTeam()) )
		{
			routes.push_back(r);
			fTotalWeight += r.m_Weight;
		}
	}

	if(!routes.empty())
	{
		int iIndex = (int)routes.size()-1;

		float fWght = Mathf::IntervalRandom(0.f, fTotalWeight);
		for(obuint32 i = 0; i < routes.size(); ++i)
		{
			fWght -= routes[i].m_Weight;
			if(fWght <= 0.f)
			{
				iIndex = i;
				break;
			}
		}

		Destination d;
		d.m_Position = routes[iIndex].m_End->GetPosition();
		d.m_Radius = Mathf::Max(routes[iIndex].m_End->GetRadius(), _minradius);
		_dest.push_back(d);

		return true;
	}

	if(GetNumUsePoints() > 0)
	{
		if ( m_RandomUsePoint )
		{
			int iRand = Mathf::IntervalRandomInt(0, (int)GetNumUsePoints());
			Destination d;
			d.m_Position = GetWorldUsePoint(iRand);
			d.m_Radius = Mathf::Max(GetRadius(), _minradius);
			_dest.push_back(d);
		}
		else
		{
			for(obint32 i = 0; i < GetNumUsePoints(); ++i)
			{
				Destination d;
				d.m_Position = GetWorldUsePoint(i);
				d.m_Radius = Mathf::Max(GetRadius(), _minradius);
				_dest.push_back(d);
			}
		}
	}
	else
	{
		Destination d;
		d.m_Position = GetWorldUsePoint();
		d.m_Radius = Mathf::Max(GetRadius(), _minradius);
		_dest.push_back(d);
	}
	return false;
}

void MapGoal::SetProperty(const String &_propname, const obUserData &_val)
{
	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

	DisableGCInScope gcEn(pMachine);

	gmVariable var = Utils::UserDataToGmVar(pMachine,_val);

	bool Processed = false;

	StringStr err;

	if(!Processed)
	{
		if(_val.IsString())
		{
			PropertyMap pm;
			pm.AddProperty(_propname,_val.GetString());
			Processed = FromPropertyMap(pm,err);
		}
	}

	if(!Processed)
	{
		Processed = FromScriptVar(pMachine,_propname.c_str(),var,err);
	}

	if(!Processed)
	{
		if(m_SetPropertyFunc)
		{
			gmGCRoot<gmUserObject> mgref = GetScriptObject(pMachine);

			gmCall call;
			if(call.BeginFunction(pMachine, m_SetPropertyFunc, gmVariable(mgref)))
			{
				call.AddParamString(_propname.c_str());
				call.AddParam(var);
				call.End();
			}
		}
	}

	// rebuild the name
	GenerateName(0);

	if(!Processed && !err.str().empty())
		EngineFuncs::ConsoleError(va("%s",err.str().c_str()));
}

extern int NextDrawTime;

void MapGoal::RenderDebug(bool _editing, bool _highlighted)
{
	Prof_Scope(MapGoal);

#ifdef Prof_ENABLED
	Prof_Scope_Var Scope(*m_ProfZone, Prof_cache);
#endif

	{
		Prof(Render);

		if(GetRenderGoal())
		{
			if(m_RenderFunc)
			{
				if(!m_ActiveThread[ON_RENDER].IsActive())
				{
					gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

					gmGCRoot<gmUserObject> mgref = GetScriptObject(pMachine);

					gmCall call;
					if(call.BeginFunction(pMachine, m_RenderFunc, gmVariable(mgref)))
					{
						call.AddParamInt(_editing?1:0);
						call.AddParamInt(_highlighted?1:0);
						if(call.End() == gmThread::EXCEPTION)
						{
							//SetEnable(false, va("Error in Update Callback in Goal: %s", GetName().c_str()));
						}

						m_ActiveThread[ON_RENDER] = call.GetThreadId();
						if(call.DidReturnVariable())
							m_ActiveThread[ON_RENDER] = 0;
					}
				}
			}
			else
			{
				if(IGame::GetTime() > NextDrawTime)
				{
					RenderDefault();
				}
			}
		}
		else
		{
			m_ActiveThread[ON_RENDER].Reset();
		}

		//////////////////////////////////////////////////////////////////////////
		if(IGame::GetTime() > NextDrawTime)
		{
			if(GetRenderRoutes())
			{
				DrawRoute(COLOR::GREEN,2.f);
			}
		}
	}
}

void MapGoal::RenderDefault()
{
	const BitFlag32 bf(GetDefaultDrawFlags());

	Vector3f vRenderPos = GetPosition();
	obColor vRenderColor = COLOR::GREEN; // customize?

	if(bf.CheckFlag(DrawCenterBounds))
	{
		vRenderPos = GetWorldBounds().Center;
	}

	vRenderPos.z += GetRenderHeight();

	Vector3f vLocalPos, vLocalFace;
	if(!Utils::GetLocalFacing(vLocalFace) || !Utils::GetLocalPosition(vLocalPos))
		return;

	//////////////////////////////////////////////////////////////////////////
	if(GetDefaultRenderRadius() < Utils::FloatMax)
	{
		if(Length(vLocalPos,vRenderPos) > GetDefaultRenderRadius())
		{
			return;
		}
	}
	//////////////////////////////////////////////////////////////////////////

	String txtOut;

	if(bf.CheckFlag(DrawName))
	{
		txtOut += GetName();
		txtOut += "\n";
	}
	if(bf.CheckFlag(DrawGroup))
	{
		if(!GetGroupName().empty())
		{
			txtOut += " Group: ";
			txtOut += GetGroupName();
			txtOut += "\n";
		}
	}
	if(bf.CheckFlag(DrawRole))
	{
		const String strRole = Utils::BuildRoleName(GetRoleMask().GetRawFlags());
		if(strRole != "None")
		{
			txtOut += "Roles: ";
			txtOut += strRole;
			txtOut += "\n";
		}
	}
	if(bf.CheckFlag(DrawInitialAvail))
	{
		// goals created by goal_create command are always initially enabled for all teams
		if (m_AvailableTeamsInit.GetRawFlags() != 30 || !GetCreateOnLoad())
		{
			txtOut += "Initial: ";
			txtOut += Utils::GetTeamString(m_AvailableTeamsInit.GetRawFlags());
			txtOut += "\n";
		}
	}
	if(bf.CheckFlag(DrawCurrentAvail))
	{
		txtOut += "Active: ";
		txtOut += Utils::GetTeamString(m_AvailableTeams.GetRawFlags());
		txtOut += "\n";
	}
	if(bf.CheckFlag(DrawRandomUsePoint))
	{
		int rup = GetRandomUsePoint();
		if ( rup > 0 )
		{
			txtOut += "RandomUsePoint: ";
			txtOut += String(va("%i", rup));
			txtOut += "\n";
		}
	}
	if(bf.CheckFlag(DrawRangeLimit))
	{
		int range = GetRange();
		if ( range > 0 )
		{
			txtOut += "Range: ";
			txtOut += String(va("%i", range));
			txtOut += "\n";
		}
	}

	// bounds
	if(bf.CheckFlag(DrawBounds))
	{
		/*if(m_bUseOrientedBox)
		Utils::OutlineOBB(GetPosition(),GetMatrix(),m_OrientedBounds,COLOR::MAGENTA,2.f);
		else*/
		Utils::OutlineOBB(GetWorldBounds(), COLOR::ORANGE, 2.f);
	}

	// radius
	if(bf.CheckFlag(DrawRadius))
	{
		if(GetRadius() != 0.f)
			Utils::DrawRadius(GetPosition(), GetRadius(), COLOR::ORANGE, 2.f);
		else
			Utils::DrawLine(GetPosition(), GetPosition().AddZ(32), COLOR::ORANGE, 2.f);
	}

	// use pts
	for(int i = 0; i < GetNumUsePoints(); ++i)
	{
		Vector3f vUsePt = GetWorldUsePoint(i);
		Utils::DrawLine(vUsePt, vUsePt.AddZ(32), COLOR::GREEN, 2.f);
	}

	if(bf.CheckFlag(DrawDisabled))
	{
		if(GetDisabled())
		{
			txtOut += "DISABLED";
			txtOut += "\n";
			vRenderColor = COLOR::RED;
		}
	}

	if(m_ExtraDebugText)
	{
		gmStringObject *so = m_ExtraDebugText;
		const char *str = so->GetString();
		if(str)
		{
			txtOut += str;
			txtOut += "\n";
		}
	}

	Utils::PrintText(
		vRenderPos,
		vRenderColor,
		2.f,
		"%s",
		txtOut.c_str());
}

Vector3f MapGoal::CalculateFarthestFacing()
{
	const float fRayLength = 5000.0f;

	float fBestVectorLength = 0.0f;
	Vector3f vBestVector = Vector3f::UNIT_Z;

	obTraceResult tr;

	// Calculate the farthest looking vector from this point, at 5 degree intervals.
	static float fInterval = 5.0f;
	for(float fAng = fInterval; fAng <= 360.0f; fAng += fInterval)
	{
		Quaternionf quat(Vector3f::UNIT_Z, Mathf::DegToRad(fAng));
		Vector3f vVec = quat.Rotate(Vector3f::UNIT_Y*fRayLength);
		EngineFuncs::TraceLine(tr, GetPosition(), GetPosition()+vVec, NULL, TR_MASK_SHOT, -1, False);
		float fSightLength = tr.m_Fraction * fRayLength;
		if(fSightLength > fBestVectorLength)
		{
			fBestVectorLength = fSightLength;
			vBestVector = vVec;
		}
	}
	vBestVector.Normalize();
	return vBestVector;
}

void MapGoal::CheckPropertiesBound()
{
	if(!GetPropertiesBound())
	{
		BindProperties();
		SetPropertiesBound(true);
	}
}

void MapGoal::BindProperties()
{
	BindProperty("Name",m_Name,Prop::PF_READONLY);
	BindProperty("TagName",m_TagName);
	BindProperty("Group",m_GroupName);
	BindProperty("Type",m_GoalType);
	BindProperty("Entity",m_Entity);
	BindProperty("SerialNum",m_SerialNum);
	BindProperty("Priority",m_DefaultPriority);
	BindProperty("Radius",m_Radius);
	BindProperty("RandomUsePoint",m_RandomUsePoint);
	BindProperty("Range",m_Range);

	{
		int EnumSize = 0;
		const IntEnum *Enum = 0;
		IGameManager::GetInstance()->GetGame()->GetTeamEnumeration(Enum,EnumSize);
		BindProperty("Available",m_AvailableTeamsInit,0,Enum,EnumSize);
	}

	{
		int EnumSize = 0;
		const IntEnum *Enum = 0;
		IGameManager::GetInstance()->GetGame()->GetRoleEnumeration(Enum,EnumSize);
		BindProperty("Roles",m_RoleMask,0,Enum,EnumSize);
	}

	BindProperty("Position",m_Position,Prop::PF_POSITION);
	//BindProperty("Facing",m_Facing,Prop::PF_FACING);
	//BindFunction("DebugRender",this,&MapGoal::ToggleRender);
}

void MapGoal::ResetGoalPriorities()
{
	for(int t = 0; t < ClassPriority::MaxTeams; ++t)
	{
		for(int c = 0; c < ClassPriority::MaxClasses; ++c)
		{
			m_ClassPriority.Priorities[t][c] = -1.f;
		}
	}
}

void MapGoal::SetPriorityForClass(int _teamid, int _classId, obReal _priority)
{
	//////////////////////////////////////////////////////////////////////////
	if(_teamid)
		_teamid=(1<<_teamid);
	else
		_teamid = -1;

	if(_classId)
		_classId=(1<<_classId);
	else
		_classId = -1;

	//////////////////////////////////////////////////////////////////////////
	for(int t = 1; t <= ClassPriority::MaxTeams; ++t)
	{
		for(int c = 1; c < ClassPriority::MaxClasses; ++c)
		{
			if((1<<t)&_teamid && (1<<c)&_classId)
			{
				m_ClassPriority.Priorities[t-1][c-1] = _priority;
			}
		}
	}
}

obReal MapGoal::GetPriorityForClient(Client *_client)
{
	float prio = GetPriorityForClass(_client->GetTeam(),_client->GetClass());
	if(prio > 0.f && GetRoleMask().AnyFlagSet())
	{
		if((GetRoleMask() & _client->GetRoleMask()).AnyFlagSet())
		{
			prio += m_RolePriorityBonus;
		}
	}
	return prio;
}

obReal MapGoal::GetPriorityForClass(int _teamid, int _classId)
{
	if(_teamid>0 && _teamid<=ClassPriority::MaxTeams &&
		_classId>0 && _classId<ClassPriority::MaxClasses)
	{
		float classPrio = m_ClassPriority.Priorities[_teamid-1][_classId-1];
		if(classPrio != -1.f)
		{
			return classPrio;
		}
	}
	return GetDefaultPriority();
}

//////////////////////////////////////////////////////////////////////////

struct PersistentPriority
{
	String	m_Expression;
	int		m_Team;
	int		m_Class;
	float	m_Priority;
};

typedef std::vector<PersistentPriority> PersPriorityList;
PersPriorityList gPriorityList;

void MapGoal::ClassPriority::GetPriorityText(std::string &_txtout, obReal roleBonus) const
{
	//////////////////////////////////////////////////////////////////////////
	int CurrentIndex = 0;
	enum { MaxClassPriority = 32 };
	struct PrioritySummary
	{
		obint32		m_TeamId;
		obint32		m_ClassId;
		obReal		m_Priority;
	};
	PrioritySummary Summary[MaxClassPriority];

	// Search all entries to build a list of summarized priority information
	for(int t = 1; t <= ClassPriority::MaxTeams && CurrentIndex<MaxClassPriority-1; ++t)
	{
		if(!Utils::TeamExists(t))
			continue;

		for(int c = 1; c <= ClassPriority::MaxClasses && CurrentIndex<MaxClassPriority-1; ++c)
		{
			if(!Utils::ClassExists(c))
				continue;

			// class and team ids start at 1
			const obReal CurrentPriority = Priorities[t-1][c-1];

			if(CurrentPriority == -1.f)
				continue;

			bool bFound = false;

			// search for matching entry.
			for(int i = 0; i < CurrentIndex; ++i)
			{
				if(Summary[i].m_Priority == CurrentPriority && Summary[i].m_TeamId & (1<<t))
				{
					Summary[i].m_TeamId |= (1<<t);
					Summary[i].m_ClassId |= (1<<c);
					bFound = true;
				}
			}

			if(!bFound)
			{
				Summary[CurrentIndex].m_Priority = CurrentPriority;
				Summary[CurrentIndex].m_TeamId = (1<<t);
				Summary[CurrentIndex].m_ClassId = (1<<c);
				++CurrentIndex;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// print the output
	for(int i = 0; i < CurrentIndex; ++i)
	{
		if(Summary[i].m_ClassId && Summary[i].m_TeamId)
		{
			_txtout = "    "; // indent
			_txtout += Utils::GetTeamString(Summary[i].m_TeamId);
			_txtout += " ";
			_txtout += Utils::GetClassString(Summary[i].m_ClassId);
			_txtout += " ";
			obReal prior = Summary[i].m_Priority;
			if (prior > 0) prior += roleBonus;
			_txtout += va(" %.2f", prior);
			EngineFuncs::ConsoleMessage(_txtout.c_str());
		}
	}
}

void MapGoal::SetPersistentPriorityForClass(const String &_exp, int _team, int _class, float _priority)
{
	for(obuint32 i = 0; i < gPriorityList.size(); ++i)
	{
		if(_exp == gPriorityList[i].m_Expression)
		{
			gPriorityList[i].m_Team = _team;
			gPriorityList[i].m_Class = _class;
			gPriorityList[i].m_Priority = _priority;
			return;
		}
	}

	PersistentPriority pp;
	pp.m_Expression = _exp;
	pp.m_Team = _team;
	pp.m_Class = _class;
	pp.m_Priority = _priority;
	gPriorityList.push_back(pp);
}

struct PersistentRole
{
	String	m_Expression;
	BitFlag32	m_Role;
};

typedef std::vector<PersistentRole> PersRoleList;
PersRoleList gRoleList;

void MapGoal::SetPersistentRole(const String &_exp, BitFlag32 _role)
{
	for(PersRoleList::iterator it = gRoleList.begin(); it != gRoleList.end(); ++it)
	{
		if(_exp == it->m_Expression)
		{
			it->m_Role = _role;
			return;
		}
	}

	PersistentRole p;
	p.m_Expression = _exp;
	p.m_Role = _role;
	gRoleList.push_back(p);
}

void MapGoal::CheckForPersistentProperty()
{
	for(PersPriorityList::iterator it = gPriorityList.begin(); it != gPriorityList.end(); ++it)
	{
		if(Utils::RegexMatch(it->m_Expression.c_str(), GetName().c_str())) {
			SetPriorityForClass(it->m_Team, it->m_Class, it->m_Priority);
			break;
		}
	}

	for(PersRoleList::iterator it = gRoleList.begin(); it != gRoleList.end(); ++it)
	{
		if(Utils::RegexMatch(it->m_Expression.c_str(), GetName().c_str())) {
			SetRoleMask(GetRoleMask() | it->m_Role);
			break;
		}
	}
}

//void MapGoal::DrawBounds(int _color, float _duration)
//{
//	Utils::OutlineAABB(GetWorldBounds(), _color, _duration);
//}
//
//void MapGoal::DrawRadius(int _color, float _duration)
//{
//	if(GetRadius() != 0.f)
//		Utils::DrawRadius(GetPosition(), GetRadius(), _color, _duration);
//	else
//		Utils::DrawLine(GetPosition(), GetPosition().AddZ(32), _color, _duration);
//}
//
//void MapGoal::DrawUsePoints(int _color, float _duration)
//{
//	for(int i = 0; i < GetNumUsePoints(); ++i)
//	{
//		Vector3f vUsePt = GetWorldUsePoint(i);
//		Utils::DrawLine(vUsePt, vUsePt.AddZ(32), _color, _duration);
//	}
//}
void MapGoal::DrawRoute(int _color, float _duration)
{
	PathPlannerBase *planner = NavigationManager::GetInstance()->GetCurrentPathPlanner();

	Routes::const_iterator cIt = m_Routes.begin(), cItEnd = m_Routes.end();
	for(; cIt != cItEnd; ++cIt)
	{
		const Route &r = (*cIt);
		if(r.m_Start->m_AvailableTeams.AnyFlagSet() && r.m_End->m_AvailableTeams.AnyFlagSet())
		{
			planner->PlanPathToGoal(NULL,r.m_Start->GetPosition(),r.m_End->GetPosition(),0);

			Path p;
			planner->GetPath(p);
			p.DebugRender(_color,_duration);
		}
	}
}

void MapGoal::FromScriptTable(gmMachine *_machine, gmTableObject *_tbl, bool _caseSense)
{
	gmTableObject *mytbl = gmBind2::Class<MapGoal>::GetTable(GetScriptObject(_machine));
	if(mytbl)
	{
		gmTableIterator tIt;
		gmTableNode *pNode = _tbl->GetFirst(tIt);
		while(pNode)
		{
			gmTableNode * existingNode = !_caseSense ? mytbl->GetTableNode(_machine,pNode->m_key,false) : 0;
			if(existingNode)
			{
				existingNode->m_value = pNode->m_value;
			}
			else
			{
				mytbl->Set(_machine,pNode->m_key,pNode->m_value);
			}
			pNode = _tbl->GetNext(tIt);
		}
	}
}

gmVariable MapGoal::GetProperty(const char *_name)
{
	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

	gmTableObject *tbl = gmBind2::Class<MapGoal>::GetTable(GetScriptObject(pMachine));
	if(tbl)
		return pMachine->Lookup(_name,tbl);
	return gmVariable::s_null;
}

bool MapGoal::GetProperty(const char *_name, Vector3f &_var)
{
	gmVariable gmVar = GetProperty(_name);
	return gmVar.GetVector(_var);
}

bool MapGoal::GetProperty(const char *_name, float &_var)
{
	gmVariable gmVar = GetProperty(_name);
	return gmVar.GetFloatSafe(_var,0.f);
}

bool MapGoal::GetProperty(const char *_name, bool &_var)
{
	gmVariable gmVar = GetProperty(_name);
	int intvar = 0;
	if(gmVar.GetIntSafe(intvar,0))
	{
		_var = intvar != 0;
		return true;
	}
	return false;
}
bool MapGoal::GetProperty(const char *_name, int &_var)
{
	gmVariable gmVar = GetProperty(_name);
	return gmVar.GetIntSafe(_var,0);
}

bool MapGoal::GetProperty(const char *_name, String &_var)
{
	gmVariable gmVar = GetProperty(_name);
	const char *cstr = gmVar.GetCStringSafe();
	if(cstr)
		_var = cstr;

	return cstr!=0;
}
bool MapGoal::GetProperty(const char *_name, Seconds &_var)
{
	gmVariable gmVar = GetProperty(_name);
	float Secs = 0.f;
	if(gmVar.GetFloatSafe(Secs,0.f))
	{
		_var = Seconds(Secs);
		return true;
	}
	return false;
}

void MapGoal::SwapEntities(MapGoal * g1, MapGoal * g2)
{
	GameEntity e = g1->m_Entity; g1->m_Entity = g2->m_Entity; g2->m_Entity = e;
	Vector3f p = g1->m_InterfacePosition; g1->m_InterfacePosition = g2->m_InterfacePosition; g2->m_InterfacePosition = p;
}

bool MapGoal::SaveToTable(gmMachine *_machine, gmGCRoot<gmTableObject> &_savetable, ErrorObj &_err)
{
	gmGCRoot<gmTableObject> GoalTable(_machine->AllocTableObject(),_machine);

	if(m_SerializeFunc)
	{
		gmCall call;
		gmGCRoot<gmUserObject> mgref = GetScriptObject(_machine);
		if(call.BeginFunction(_machine, m_SerializeFunc, gmVariable(mgref)))
		{
			call.AddParamTable(GoalTable);
			const int ThreadState = call.End();
			if(ThreadState!=gmThread::KILLED)
			{
				_err.AddError("Error Calling Script Serialize function!");
				return false;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Standard info.
	if(m_Version!=MapGoalVersion) GoalTable->Set(_machine, "Version", gmVariable(m_Version));
	GoalTable->Set(_machine,"GoalType",GetGoalType().c_str());
	//GoalTable->Set(_machine, "Name", GetName().c_str());
	int sz = (int)(m_Name.length() - m_GoalType.length() - 1);
	if(sz <= 0 || m_Name.compare(m_GoalType.length()+1, sz, m_TagName))
		GoalTable->Set(_machine, "TagName", GetTagName().c_str());

	if(!GetGroupName().empty()) GoalTable->Set(_machine, "GroupName", GetGroupName().c_str());
	GoalTable->Set(_machine,"Position",gmVariable(m_InterfacePosition.IsZero() ? m_Position : m_InterfacePosition));
	if(m_Radius!=0.0f) GoalTable->Set(_machine, "Radius", gmVariable(m_Radius));
	if((m_MinRadius < m_MinRadiusInit && m_Radius < m_MinRadiusInit) || (m_MinRadius > m_MinRadiusInit && m_Radius < m_MinRadius))
		GoalTable->Set(_machine, "MinRadius", gmVariable(m_MinRadius));

	if(!m_CreateOnLoad) GoalTable->Set(_machine, "CreateOnLoad", gmVariable(m_CreateOnLoad));
	if(m_RandomUsePoint) GoalTable->Set(_machine,"RandomUsePoint",gmVariable(m_RandomUsePoint));
	if(m_Range) GoalTable->Set(_machine,"Range",gmVariable(m_Range));

	/*gmGCRoot<gmUserObject> userBounds = gmBind2::Class<BoundingBox>::WrapObject(_machine,&m_Bounds,true);
	GoalTable->Set(_machine,"Bounds",gmVariable(userBounds));*/

	//if(!m_EulerValid){
	//	m_Orientation.ToEulerAnglesZXY(m_Euler.x, m_Euler.y, m_Euler.z);
	//	m_EulerValid = true;
	//}
	//GoalTable->Set(_machine, "Orientation", gmVariable(m_Euler));
	//////////////////////////////////////////////////////////////////////////

	if(m_AvailableTeamsInit.GetRawFlags()!=30)
		GoalTable->Set(_machine, "TeamAvailability", gmVariable(m_AvailableTeamsInit.GetRawFlags()));

	GoalTable->Set(_machine,"Roles",gmVariable::s_null);
	if(m_RoleMask.AnyFlagSet())
	{
		gmTableObject * roleTable = _machine->AllocTableObject();

		int NumElements = 0;
		const IntEnum *Enum = 0;
		IGameManager::GetInstance()->GetGame()->GetRoleEnumeration(Enum,NumElements);
		for(int i = 0; i < 32; ++i)
		{
			if(m_RoleMask.CheckFlag(i))
			{
				for(int e = 0; e < NumElements; ++e)
				{
					if(Enum[e].m_Value == i)
					{
						roleTable->Set(_machine,roleTable->Count(),Enum[e].m_Key);
						break;
					}
				}
			}
		}
		GoalTable->Set(_machine,"Roles",gmVariable(roleTable));
	}


	//////////////////////////////////////////////////////////////////////////
	_savetable->Set(_machine,GetName().c_str(),gmVariable(GoalTable));
	return true;
}

bool MapGoal::LoadFromTable(gmMachine *_machine, gmGCRoot<gmTableObject> &_loadtable, ErrorObj &_err)
{
	gmGCRoot<gmTableObject> proptable;
	proptable.Set(_machine->AllocTableObject(),_machine);

	_loadtable->CopyTo(_machine,proptable);

	if(const char *Name = proptable->Get(_machine,"Name").GetCStringSafe(0))
		m_Name = Name;
	else
	{
		_err.AddError("Goal.Name Field Missing!");
		return false;
	}
	const char *TagName = proptable->Get(_machine, "TagName").GetCStringSafe(0);
	if(!TagName && m_Name.length() > m_GoalType.length()+1)
		TagName = m_Name.c_str() + m_GoalType.length()+1; //set TagName from Name
	if(TagName)
		m_TagName = TagName;
	else
	{
		_err.AddError("Goal.TagName Field Missing!");
		return false;
	}
	m_GroupName = proptable->Get(_machine, "GroupName").GetCStringSafe("");
	proptable->Get(_machine, "Version").GetInt(m_Version, 0);
	if(m_Version == 0) proptable->Set(_machine, "Version", gmVariable(m_Version = MapGoalVersion));
	if(!proptable->Get(_machine, "Position").GetVector(m_Position))
	{
		_err.AddError("Goal.Position Field Missing!");
		return false;
	}
	proptable->Get(_machine, "Radius").GetFloatSafe(m_Radius, 0.0f);
	proptable->Get(_machine, "MinRadius").GetFloatSafe(m_MinRadius, m_MinRadius);

	// Command /bot goal_create does not generate unique serial numbers !
	// If some goals are deleted, then newly created goals can have same serial number as existing goals.
	// Because serial numbers in the file are not unique, it would be very dangerous to load them.
	// Fortunatelly, omni-bot never loaded serial numbers from a file, because here is another bug.
	// Function GetIntSafe cannot change value of m_SerialNum, because parameter is not passed by reference (see declaration in gmVariable.h)
	//proptable->Get(_machine,"SerialNum").GetIntSafe(m_SerialNum);

	if(proptable->Get(_machine,"Orientation").GetVector(m_Euler))
	{
		m_EulerValid = true;
		m_OrientationValid = false;
	}

	int InitialTeams = 0;
	proptable->Get(_machine, "TeamAvailability").GetIntSafe(InitialTeams, 30);
	m_AvailableTeamsInit = BitFlag32(InitialTeams);
	m_AvailableTeams = m_AvailableTeamsInit;

	m_RoleMask.ClearAll();
	if(gmTableObject * roleTable = proptable->Get(_machine,"Roles").GetTableObjectSafe())
	{
		int NumElements = 0;
		const IntEnum *Enum = 0;
		IGameManager::GetInstance()->GetGame()->GetRoleEnumeration(Enum,NumElements);

		gmTableIterator tIt;
		gmTableNode * pNodeRole = roleTable->GetFirst(tIt);
		while(pNodeRole)
		{
			const char * roleName = pNodeRole->m_value.GetCStringSafe(0);
			if(roleName)
			{
				for(int e = 0; e < NumElements; ++e)
				{
					if(!Utils::StringCompareNoCase(roleName,Enum[e].m_Key))
					{
						m_RoleMask.SetFlag(Enum[e].m_Value);
					}
				}
			}
			pNodeRole = roleTable->GetNext(tIt);
		}
	}

	int CreateOnLoad;
	proptable->Get(_machine, "CreateOnLoad").GetIntSafe(CreateOnLoad, 1);
	SetCreateOnLoad(CreateOnLoad!=0);

	proptable->Get(_machine, "RandomUsePoint").GetInt(m_RandomUsePoint, 0);

	proptable->Get(_machine, "Range").GetInt(m_Range, 0);

	// clear out the properties we don't want to pass along.
	proptable->Set(_machine,"Name",gmVariable::s_null);
	proptable->Set(_machine,"TagName",gmVariable::s_null);
	proptable->Set(_machine,"GroupName",gmVariable::s_null);
	proptable->Set(_machine,"Position",gmVariable::s_null);
	proptable->Set(_machine,"Radius",gmVariable::s_null);
	proptable->Set(_machine,"MinRadius",gmVariable::s_null);
	proptable->Set(_machine,"SerialNum",gmVariable::s_null);
	proptable->Set(_machine,"GoalType",gmVariable::s_null);
	proptable->Set(_machine,"Orientation",gmVariable::s_null);
	proptable->Set(_machine,"TeamAvailability",gmVariable::s_null);
	proptable->Set(_machine,"Roles",gmVariable::s_null);

	return InternalInit(proptable,false);
}

void MapGoal::ShowHelp()
{
	if(m_HelpFunc)
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

		gmGCRoot<gmUserObject> mgref = GetScriptObject(pMachine);

		gmCall call;
		if(call.BeginFunction(pMachine, m_HelpFunc, gmVariable(mgref)))
		{
			call.End();
		}
	}
}

void MapGoal::HudDisplay()
{
#ifdef ENABLE_DEBUG_WINDOW
	if(DW.Hud.mPropertySheet)
	{
		GatherProperties(DW.Hud.mPropertySheet);

		if(m_HudDisplay)
		{
			gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

			gmGCRoot<gmUserObject> mgref = GetScriptObject(pMachine);

			gmCall call;
			if(call.BeginFunction(pMachine, m_HudDisplay, gmVariable(mgref)))
			{
				call.AddParamUser(DW.Hud.mUserObject);
				call.End();
			}
		}
	}
#endif
}

void MapGoal::CreateGuiFromSchema(gmMachine *a_machine, gmTableObject *a_schema)
{
#ifdef ENABLE_DEBUG_WINDOW
	gmTableIterator it;
	gmTableNode *pNode = a_schema->GetFirst(it);
	while(pNode)
	{
		const char *PropName = pNode->m_key.GetCStringSafe();
		if(PropName)
		{
			gmVariable thisObj(GetScriptObject(a_machine));

			gmVariable current(gmVariable::s_null);
			const gmSchema::ElementType et = gmSchema::GetElementType(a_machine,pNode->m_value);
			switch (et)
			{
				case gmSchema::EL_TABLEOF:
					{
						break;
					}
				case gmSchema::EL_ENUM:
					{
						const gmTableObject *options = gmSchema::GetEnumOptions(a_machine,pNode->m_value,thisObj,current);
						if(options)
						{
							MgListModel *lm = new MgListModel(options);
							gcn::DropDown *widget = new gcn::DropDown(lm);
							widget->getScrollArea()->setHeight(widget->getFont()->getHeight() * 4);
							widget->setActionEventId(PropName);
							widget->addActionListener(this);
							widget->addDeathListener(lm);

							DW.Hud.mPropertySheet->addProperty(PropName,widget);
						}
						break;
					}
				case gmSchema::EL_FLOATRANGE:
					{
						break;
					}
				case gmSchema::EL_INTRANGE:
					{
						break;
					}
				case gmSchema::EL_NUMRANGE:
					{
						float minrange, maxrange;
						if(gmSchema::GetNumRange(a_machine,pNode->m_value,thisObj,current,minrange,maxrange))
						{
							gcn::Slider *widget = new gcn::Slider(minrange, maxrange);
							widget->setActionEventId(PropName);
							widget->addActionListener(this);
							widget->setValue(/*current*/0.f);
							DW.Hud.mPropertySheet->addProperty(PropName,widget);
						}
						break;
					}
				case gmSchema::EL_VARTYPE:
					{
						break;
					}
				case gmSchema::EL_NONE:
				default:
					{
						break;
					}
			}
		}
		pNode = a_schema->GetNext(it);
	}
#endif
}

#ifdef ENABLE_DEBUG_WINDOW
void MapGoal::action(const gcn::ActionEvent& actionEvent)
{
}
#endif

#ifdef ENABLE_REMOTE_DEBUGGING
void MapGoal::Sync( RemoteLib::DataBuffer & db, bool fullSync ) {
	if ( fullSync || m_NeedsSynced ) {
		snapShot.Clear();
	}

	RemoteLib::DataBufferStatic<2048> localBuffer;
	localBuffer.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );

	MapGoalSnapshot newSnapShot = snapShot;

	const Box3f worldbounds = GetWorldBounds();
	const float heading = Mathf::RadToDeg( worldbounds.Axis[ 0 ].XYHeading() );
	const float pitch = Mathf::RadToDeg( worldbounds.Axis[ 0 ].GetPitch() );

	newSnapShot.Sync( "name", GetName().c_str(), localBuffer );
	newSnapShot.Sync( "tagName", GetTagName().c_str(), localBuffer );
	newSnapShot.Sync( "entityid", GetEntity().AsInt(), localBuffer );
	newSnapShot.Sync( "ownerid", GetOwner().AsInt(), localBuffer );
	newSnapShot.Sync( "x", worldbounds.Center.x, localBuffer );
	newSnapShot.Sync( "y", worldbounds.Center.y, localBuffer );
	newSnapShot.Sync( "z", worldbounds.Center.z, localBuffer );
	newSnapShot.Sync( "yaw", -Mathf::RadToDeg( heading ), localBuffer );
	newSnapShot.Sync( "pitch", Mathf::RadToDeg( pitch ), localBuffer );
	newSnapShot.Sync( "sizex", worldbounds.Extent[ 0 ], localBuffer );
	newSnapShot.Sync( "sizey", worldbounds.Extent[ 1 ], localBuffer );
	newSnapShot.Sync( "sizez", worldbounds.Extent[ 2 ], localBuffer );
	newSnapShot.Sync( "defaultPriority", GetDefaultPriority(), localBuffer );
	newSnapShot.Sync( "usersInProgress", GetCurrentUsers(MapGoal::TRACK_INPROGRESS, 1) + GetCurrentUsers(MapGoal::TRACK_INPROGRESS, 2), localBuffer);
	newSnapShot.Sync( "maxUsersInProgress", GetMaxUsers( MapGoal::TRACK_INPROGRESS ), localBuffer );
	newSnapShot.Sync( "usersInUse", GetCurrentUsers(MapGoal::TRACK_INUSE, 1) + GetCurrentUsers(MapGoal::TRACK_INUSE, 2), localBuffer);
	newSnapShot.Sync( "maxUsersInUse", GetMaxUsers( MapGoal::TRACK_INUSE ), localBuffer );
	newSnapShot.Sync( "availableTeamMask", GetAvailableFlags().GetRawFlags(), localBuffer );
	newSnapShot.Sync( "roleMask", GetRoleMask().GetRawFlags(), localBuffer );
	// todo: routes, team/class priorities, etc

	const uint32 writeErrors = localBuffer.endWrite();
	assert( writeErrors == 0 );

	if ( localBuffer.getBytesWritten() > 0 && writeErrors == 0 ) {
		db.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
		db.startSizeHeader();
		db.writeInt32( RemoteLib::ID_qmlComponent );
		db.writeInt32( GetSerialNum() );
		db.writeSmallString( "mapgoal" );
		db.append( localBuffer );
		db.endSizeHeader();

		if ( db.endWrite() == 0 ) {
			// mark the stuff we synced as done so we don't keep spamming it
			snapShot = newSnapShot;
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

static int MG_HandleMaxUsers(gmThread *a_thread, MapGoal::TrackingCat _cat)
{
	obint32 iMaxUsers = 0;

	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	switch(a_thread->GetNumParams())
	{
	case 1:
		{
			GM_CHECK_INT_PARAM(newval, 0);
			iMaxUsers = NativePtr->GetMaxUsers(_cat);
			NativePtr->SetMaxUsers(_cat, newval);
		}
	case 0:
		{
			iMaxUsers = NativePtr->GetMaxUsers(_cat);
			break;
		}
	default:
		GM_EXCEPTION_MSG("Expected 0 or 1 param.");
		return GM_EXCEPTION;
	}

	a_thread->PushInt(iMaxUsers);
	return GM_OK;
}

static int gmfMaxUsers_InProgress(gmThread *a_thread)
{
	return MG_HandleMaxUsers(a_thread, MapGoal::TRACK_INPROGRESS);
}

static int gmfMaxUsers_InUse(gmThread *a_thread)
{
	return MG_HandleMaxUsers(a_thread, MapGoal::TRACK_INUSE);
}

static int gmfGetEntity(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	if(NativePtr->GetEntity().IsValid())
		a_thread->PushEntity(NativePtr->GetEntity().AsInt());
	else
		a_thread->PushNull();
	return GM_OK;
}

static int gmfSetEntity(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread, NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	GM_CHECK_NUM_PARAMS(1);
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	NativePtr->SetEntity(gameEnt);
	return GM_OK;
}

static int gmfGetOwner(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	if(NativePtr->GetOwner().IsValid())
		a_thread->PushEntity(NativePtr->GetOwner().AsInt());
	else
		a_thread->PushNull();
	return GM_OK;
}

static int gmfAddUsePoint(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	GM_INT_PARAM(rel, 1, 0);

	NativePtr->AddUsePoint(Vector3f(v.x,v.y,v.z), rel != 0);
	return GM_OK;
}

static int gmfLimitToWeapon(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	return NativePtr->GetLimitWeapons().FromScript(a_thread);
}

static int gmfDisableWithEntityFlags(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	BitFlag64 bf;
	for(int p = 0; p < a_thread->GetNumParams(); ++p)
	{
		GM_CHECK_INT_PARAM(fl,p);
		bf.SetFlag(fl,true);

	}
	NativePtr->SetDisableWithEntityFlag(bf);
	return GM_OK;
}

static int gmfDeleteWithEntityFlags(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	BitFlag64 bf;
	for(int p = 0; p < a_thread->GetNumParams(); ++p)
	{
		GM_CHECK_INT_PARAM(fl,p);
		bf.SetFlag(fl,true);

	}
	NativePtr->SetDeleteWithEntityFlag(bf);
	return GM_OK;
}

static int gmfGetPriorityForClient(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	if(a_thread->GetNumParams()==1)
	{
		GM_CHECK_NUM_PARAMS(1);
		GM_CHECK_GMBIND_PARAM(Client*, gmBot, bot, 0);
		a_thread->PushFloat(NativePtr->GetPriorityForClient(bot));
	}
	else if(a_thread->GetNumParams()==2)
	{
		GM_CHECK_INT_PARAM(teamId,0);
		GM_CHECK_INT_PARAM(classId,1);
		a_thread->PushFloat(NativePtr->GetPriorityForClass(teamId,classId));
	}
	else
	{
		GM_EXCEPTION_MSG("expected (int,int), or (bot)");
		return GM_EXCEPTION;
	}
	return GM_OK;
}

static int gmfSetRoles(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	GM_CHECK_NUM_PARAMS(1);

	BitFlag32 rolemask = NativePtr->GetRoleMask(); // cs: preserve current mask
	for(int p = 0; p < a_thread->GetNumParams(); ++p)
	{
		GM_CHECK_INT_PARAM(r,p);
		rolemask.SetFlag(r,true);
	}
	NativePtr->SetRoleMask(rolemask);
	return GM_OK;
}

static int gmfClearRoles(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	GM_CHECK_NUM_PARAMS(1);

	BitFlag32 rolemask = NativePtr->GetRoleMask(); // cs: preserve current mask
	for(int p = 0; p < a_thread->GetNumParams(); ++p)
	{
		GM_CHECK_INT_PARAM(r,p);
		rolemask.SetFlag(r,false);
	}
	NativePtr->SetRoleMask(rolemask);
	return GM_OK;
}

static int gmfHasRole(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	GM_CHECK_NUM_PARAMS(1);
	for(int p = 0; p < a_thread->GetNumParams(); ++p)
	{
		GM_CHECK_INT_PARAM(r,p);
		if(NativePtr->GetRoleMask().CheckFlag(r))
		{
			a_thread->PushInt(1);
			return GM_OK;
		}
	}
	a_thread->PushInt(0);
	return GM_OK;
}

static int gmfSetBaseGoalType(gmThread *a_thread)
{
	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(basetype,0);

	filePath script( "scripts/mapgoals/%s", basetype );

	bool Good = false;
	try
	{
		if(NativePtr->LoadFromFile( script ) )
		{
			Good = true;
		}
	}
	catch(const std::exception &)
	{
	}

	if(!Good)
	{
		GM_EXCEPTION_MSG("Unable to set base goal type: %s", script.FileName().c_str());
		return GM_EXCEPTION;
	}

	return GM_OK;
}

static int gmfCreateGuiFromSchema(gmThread *a_thread)
{
	GM_CHECK_USER_PARAM_TYPE(gmSchema::GM_SCHEMA,0);

	MapGoal *NativePtr = 0;
	if(!gmBind2::Class<MapGoal>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal");
		return GM_EXCEPTION;
	}

	gmTableObject *Schema = static_cast<gmTableObject*>(a_thread->Param(0).GetUserSafe(gmSchema::GM_SCHEMA));
	NativePtr->CreateGuiFromSchema(a_thread->GetMachine(),Schema);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void MapGoal_AsString(MapGoal *a_var, char * a_buffer, int a_bufferSize)
{
	_gmsnprintf(a_buffer, a_bufferSize,
		"MapGoal(%s::%s)",a_var->GetGoalType().c_str(),a_var->GetName().c_str());
}

//////////////////////////////////////////////////////////////////////////

//bool MapGoal::pfnSetDotEx(gmThread * a_thread, MapGoal * a_goal, const char *a_key, gmVariable * a_operands)
//{
//	MapGoal *Mg = 0;
//	if(gmBind2::Class<MapGoal>::FromVar(a_thread,a_operands[1],Mg) && Mg)
//	{
//		MapGoalPtr mg = Mg->GetSmartPtr();
//
//		return true;
//	}
//
//	return false;
//}
//bool MapGoal::pfnGetDotEx(gmThread * a_thread, MapGoal * a_goal, const char *a_key, gmVariable * a_operands)
//{
//	return false;
//}

void MapGoal::Bind(gmMachine *_m)
{
	sNextSerialNum = 0;

	auto a = gmBind2::Global(_m,"InternalGoalState")
            .var((int)GoalStateFlagState,"FlagState")
		//.var((int)GoalStateFlagHoldState,"FlagHoldState")
		;

	gmBind2::Class<MapGoal>("MapGoal",_m)
		//.constructor()
		.asString(MapGoal_AsString)

		.func(&MapGoal::IsAvailable,	"IsAvailable","Is goal available for a given team.")
		.func(&MapGoal::SetAvailable,	"SetAvailable","Set goal available for a given team.")

		.func(&MapGoal::IsAvailableInitial,	"IsAvailableInitial","Is goal available for team initially.")
		.func(&MapGoal::SetAvailableInitial,"SetAvailableInitial","Set goal available for team initially.")

		.func(&MapGoal::GetGoalState,	"GetGoalState")
		.func(&MapGoal::GetGoalType,	"GetGoalType")
		.func(&MapGoal::GetTagName,		"GetTagName")
		.func(&MapGoal::GetName,		"GetName","Get the full name of the goal.")

		.func(&MapGoal::GetGroupName,	"GetGroupName","Get the current group this goal is assigned to.")
		.func(&MapGoal::SetGroupName,	"SetGroupName","Set the current group this goal is assigned to.")

		.func(gmfSetBaseGoalType,			"SetBaseGoalType","Clones a base goal type. Should be done first thing in goal script.")

		.func(&MapGoal::GetPosition_Script,	"GetPosition","Get Goal Position")
		.func(&MapGoal::SetPosition_Script,	"SetPosition","Set Goal Position")
		.func(&MapGoal::GetFacing_Script,	"GetFacing","Get Goal Facing")
		.func(&MapGoal::SetFacing_Script,	"SetFacing","Set Goal Position")

		.func(&MapGoal::GetDisabled,		"IsDisabled","Get whether the goal is currently disabled.")
		.func(&MapGoal::SetDisabled,		"DisableGoal","Set whether the goal is currently disabled.")

		.func(&MapGoal::GetRange_Script,	"GetRange","Get current range limit for the goal")
		.func(&MapGoal::SetRange_Script,	"SetRange","Set the current range limit for the goal")

		//.func(&MapGoal::GetWorldBounds,	"GetBounds")
		//.func(&MapGoal::GetLocalBounds,	"GetLocalBounds")
		//.func(&MapGoal::SetGoalBounds,	"SetBounds")
		.func(&MapGoal::SetBounds_Script,	"SetBounds","Set the object space bounding box of the goal.")
		.func(&MapGoal::GetBoundsCenter_Script,	"GetCenterBounds","Get the center of the bounding box.")
		//.func(&MapGoal::GetMatrix,		"GetMatrix")
		//.func(&MapGoal::SetMatrix,		"SetMatrix")

		.func(&MapGoal::GetRadius,			"GetRadius","Get the Goal Radius")
		.func(&MapGoal::SetRadius,			"SetRadius","Set the Goal Radius")

		//.func(&MapGoal::SetProperties,	"SetProperties")

		.func(gmfAddUsePoint,				"AddUsePoint","Adds a 'use' point to the goal.")
		.func(&MapGoal::GetWorldUsePoint_Script, "GetUsePoint", "Gets a use point in world space, by index.")
		.func(&MapGoal::GetNumUsePoints,	"GetNumUsePoint","Gets the number of use points currently defined.")

		.func(gmfSetRoles,					"SetRoles","Sets the roles that are allowed to use this goal.")
		.func(gmfClearRoles,				"ClearRoles","Removes the given roles from this goal.")
		.func(gmfHasRole,					"HasRole","Return true if the goal has any of the roles provided as params.")

		.func(gmfDisableWithEntityFlags,	"DisableIfEntityFlag","Sets one or more entity flags that will cause the goal to be disabled.")
		.func(gmfDeleteWithEntityFlags,		"DeleteIfEntityFlag","Sets one or more entity flags that will cause the goal to be deleted.")

		/*.func(&MapGoal::DrawBounds,			"DrawBounds")
		.func(&MapGoal::DrawRadius,			"DrawRadius")
		.func(&MapGoal::DrawUsePoints,		"DrawUsePoints")
		.func(&MapGoal::DrawRoute,			"DrawRoutes")*/

		//.func(&MapGoal::SetEnableDraw,	"SetEnableDraw")

		//.func(&MapGoal::AddRoute,			"AddRoute")

		.func(&MapGoal::SetPriorityForClass,	"SetGoalPriority","Sets the priority for a given class/team.")
		.func(gmfGetPriorityForClient,			"GetGoalPriority","Gets the priority for a given class/team.")
		.func(&MapGoal::ResetGoalPriorities,	"ResetGoalPriorities","Clears all the current priorities.")

		.func(&MapGoal::SetDeleteMe,			"SetRemoveFlag","Mark the goal for deletion.")

		.func(&MapGoal::RenderDefault,			"RenderDefault","Render the default debug options.")
		.var(&MapGoal::m_DefaultRenderRadius,	"DefaultRenderRadius","Radius in which debug options will be displayed.")
		.var(&MapGoal::m_RenderHeight,			"DefaultRenderHeight","Goal height offset where rendering will take place.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawName,"RenderDefaultName","Draw the name of the goal.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawGroup,"RenderDefaultGroup","Draw the group of the goal.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawRole,"RenderDefaultRole","Draw the roles for the goal.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawBounds,"RenderDefaultBounds","Draw the bounds of the goal.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawRadius,"RenderDefaultRadius","Draw the radius of the goal.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawInitialAvail,"RenderDefaultInitialAvailability","Draw the initial availability of the goal.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawCurrentAvail,"RenderDefaultCurrentAvailability","Draw the current availability of the goal.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawCenterBounds,"RenderDefaultAtCenterBounds","Draw debug options using the center of the bounding box.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawRandomUsePoint,"RenderRandomUsePoint","Draw whether or not the goal randomly selects a usepoint.")
		.var_bitfield(&MapGoal::m_DefaultDrawFlags,DrawRangeLimit,"RenderRangeLimit","Draw the goals current range limit if greater than 0.")

		.func(gmfMaxUsers_InProgress,			"MaxUsers_InProgress","Set the max number of 'inprogress' users that can use the goal")
		.func(gmfMaxUsers_InUse,				"MaxUsers_InUse","Set the max number of 'inuse' users that can use the goal")

		.func(gmfGetEntity,						"GetEntity","Get the entity of the goal, if any.")
		.func(gmfSetEntity, "SetEntity", "Set the entity of the goal.")
		.func(gmfGetOwner, "GetOwner", "Gets the entity owner of the goal, if any.")

		.func(gmfCreateGuiFromSchema,						"CreateGuiFromSchema", "Create Gui elements for schema properties.")

		.func(&MapGoal::AddRoute_Script,		"AddRoute","Adds a route for this goal.")

		.func(gmfLimitToWeapon,					"LimitToWeapon","Adds a list of weapons that are required from any user of the goal.")

		.var(&MapGoal::m_Version,				"Version","int","Gets the goal version.")

		.var(&MapGoal::m_GoalState,				"GoalState","int","Gets the goal state")
		.var(&MapGoal::m_GoalStateFunction,		"GoalStateFunction","enum InternalGoalState")

		.var(&MapGoal::m_InitNewFunc,			"InitNewGoal","Callback","Called on goal creation to initialize any internal variables.")
		.var(&MapGoal::m_UpgradeFunc,			"UpgradeVersion","Callback","Called to upgrade the goal to the latest used version.")
		.var(&MapGoal::m_RenderFunc,			"Render","Callback","Called when draw_goals is enabled for this goal. Used to render itself.")
		.var(&MapGoal::m_UpdateFunc,			"Update","Callback","Called every frame to update the state of the goal if needed.")
		.var(&MapGoal::m_SerializeFunc,			"SaveToTable","Callback","Called when the goals are saved to a file. Allows the goal to serialize persistent information.")
		.var(&MapGoal::m_SetPropertyFunc,		"SetProperty","Callback","Called on bot goal_setproperty x y, where x is a property name and y is a value or keyword.")
		.var(&MapGoal::m_HelpFunc,				"Help","Callback","Called on bot goal_help to print requirements and available properties for the goal.")
		.var(&MapGoal::m_HudDisplay,			"HudDisplay","Callback","Called when goal is highlighted to create gui elements for debug visualization.")

		.var(&MapGoal::m_ExtraDebugText,		"ExtraDebugText", "string","Additional debug text to render in RenderDefault function.")

		.var(&MapGoal::m_Radius,				"Radius","float","Radius of the goal.")
		.var(&MapGoal::m_MinRadius,				"MinRadius","float","Minimum allowed radius of the goal.")
		//.var(&MapGoal::m_Bounds,				"Bounds")

		.var(&MapGoal::m_GoalType,				"GoalType","string","Type of goal.")

		.var(&MapGoal::m_DefaultPriority,		"DefaultPriority","float","Default priority of the goal, if no class/team specific priorities.")
		.var(&MapGoal::m_RolePriorityBonus,		"RolePriorityBonus","float","Role priority bonus of the goal, for users matching role.")

		.var_readonly(&MapGoal::m_SerialNum,	"SerialNum","int readonly","Auto generated unique serial number of the goal.")

		// flags
		.var(&MapGoal::m_DeleteMe,				"MarkForRemoval","bool","Mark the goal for deletion.")
		.var(&MapGoal::m_DynamicPosition,		"DynamicPosition","bool","Goal should update its position from its entity.")
		.var(&MapGoal::m_DynamicOrientation,	"DynamicOrientation","bool","Goal should update its orientation from its entity.")
		.var(&MapGoal::m_RemoveWithEntity,		"RemoveWithEntity","bool","Goal should be removed if its entity is removed.")
		.var(&MapGoal::m_InUse,					"InUse","bool","Goal is inuse and should not be chosen.")
		.var(&MapGoal::m_DisableForControllingTeam,"DisableForControllingTeam","bool","Goal will be made unavailable for the team which controls it(GetOwner)")
		.var(&MapGoal::m_DontSave,				"DontSave","bool","Dont save this goal into the map goal script.")
		.var(&MapGoal::m_RenderGoal,			"RenderGoal","bool","Enable rendering for this goal.")
		.var(&MapGoal::m_RenderRoutes,			"RenderRoutes","bool","Enable rendering of the routes for this goal.")
		.var(&MapGoal::m_CreateOnLoad,			"CreateOnLoad","bool","False to not create the goal at load time, but keep the data around for when created by the interface.")

		/*.setDotEx(pfnSetDotEx)
		.getDotEx(pfnGetDotEx)*/
		;
}

