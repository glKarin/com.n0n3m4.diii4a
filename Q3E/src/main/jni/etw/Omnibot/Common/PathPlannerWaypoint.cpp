#include "PrecompCommon.h"
#include <limits>

#include "PathPlannerWaypoint.h"
#include "IGameManager.h"
#include "IGame.h"
#include "Client.h"
#include "GoalManager.h"
#include "Path.h"
#include "FileDownloader.h"
#include "physfs.h"

#include "NavigationFlags.h"

// Waypoint Serializer Implementations
#include "WaypointSerializer_V1.h"
#include "WaypointSerializer_V2.h"
#include "WaypointSerializer_V3.h"
#include "WaypointSerializer_V4.h"
#include "WaypointSerializer_V5.h"
#include "WaypointSerializer_V6.h"
#include "WaypointSerializer_V7.h"
#include "WaypointSerializer_V9.h"
// TODO: next wp version, remove F_NAV_TEAMONLY 

using namespace std;

typedef enum
{
	VIS_VERSION1,
	VIS_VERSION2,
	VIS_VERSION_LATEST,
	NUM_VIS_VERSION
} VisFile_Version;

//////////////////////////////////////////////////////////////////////////
// data drive these
float g_fTopWaypointOffset = 40.f;
float g_fBottomWaypointOffset = -24.f;
float g_fTopPathOffset = 36.f;
float g_fBottomPathOffset = 6.f;
float g_fBlockablePathOffset = 0.f;
float g_fFacingOffset = 18.f;
float g_fWaypointTextOffset = 0.f;
float g_fWaypointTextDuration = 2.f;
float g_fPathLevelOffset = 0.5f;
//////////////////////////////////////////////////////////////////////////

obColor		g_WaypointColor = COLOR::MAGENTA;
obColor		g_LinkClosedColor = obColor(20, 20, 20);
obColor		g_LinkTeleport = COLOR::YELLOW;
obColor		g_LinkColor1Way = COLOR::GREY;
obColor		g_LinkColor2Way = COLOR::GREY;
obColor		g_BlockableBlocked = COLOR::RED;
obColor		g_BlockableOpen = COLOR::GREEN;
obColor		g_AimEntity = COLOR::ORANGE;
obColor		g_RadiusIndicator = COLOR::RED;
obColor		g_SelectedWaypoint = COLOR::BLUE;
obColor		g_ShowFacingColor = COLOR::BLUE;

obColor		g_Team1 = COLOR::MAGENTA;
obColor		g_Team2 = COLOR::MAGENTA;
obColor		g_Team3 = COLOR::MAGENTA;
obColor		g_Team4 = COLOR::MAGENTA;

//////////////////////////////////////////////////////////////////////////

NavFlags PathPlannerWaypoint::m_CallbackFlags = 0;
NavFlags PathPlannerWaypoint::m_BlockableMask = 0;

//////////////////////////////////////////////////////////////////////////

PathPlannerWaypoint::PathPlannerWaypoint() 
	: PathPlannerBase()
	, m_Client					(0)
	, m_DefaultWaypointRadius	(35.0f)
	, m_SelectedWaypoint		(-1)
	, m_ConnectWp				(0)
	, m_GoodPathQueries			(0)
	, m_BadPathQueries			(0)
	, m_PathCheckCallback		(0)
	, m_WaypointMark			(0)
	, m_MovingWaypointIndex		(-1)
	, m_BoxStart				(Vector3f::ZERO)
{
	// Add all the waypoint versions along with their matching serializers
	m_WaypointSerializer[1] = WpSerializerPtr(new WaypointSerializer_V1);
	m_WaypointSerializer[2] = WpSerializerPtr(new WaypointSerializer_V2);
	m_WaypointSerializer[3] = WpSerializerPtr(new WaypointSerializer_V3);
	m_WaypointSerializer[4] = WpSerializerPtr(new WaypointSerializer_V4);
	m_WaypointSerializer[5] = WpSerializerPtr(new WaypointSerializer_V5);
	m_WaypointSerializer[6] = WpSerializerPtr(new WaypointSerializer_V6);
	m_WaypointSerializer[7] = WpSerializerPtr(new WaypointSerializer_V7);
	m_WaypointSerializer[8] = m_WaypointSerializer[7]; // same format, diff version for 0.7
	m_WaypointSerializer[9] = WpSerializerPtr(new WaypointSerializer_V9);

	LOG("Waypoint Nav System Initialized (" << m_WaypointSerializer.size() << " Serializers)");

	m_CreatingSector.m_Normal = Vector3f::UNIT_Z;
	m_CreatingSector.m_SectorBounds = AABB(Vector3f::ZERO);
}

PathPlannerWaypoint::~PathPlannerWaypoint()
{
	m_WaypointSerializer.clear();
	Shutdown();
}

int PathPlannerWaypoint::GetLatestFileVersion() const
{
	WaypointSerializers::const_reverse_iterator rIt = m_WaypointSerializer.rbegin();
	return rIt->first;
}

bool PathPlannerWaypoint::Init()
{
	m_BlockableRegulator.reset(new Regulator(2000));
	m_RadiusMarkRegulator.reset(new Regulator(200));

	InitCommands();

	// Allow the game to register a path check callback.
	IGameManager::GetInstance()->GetGame()->RegisterPathCheck(m_PathCheckCallback);

	// todo, any data structure initialization
	return true;
}

void PathPlannerWaypoint::UpdateSelectedWpRender()
{
	if(m_SelectedWaypoint != -1)
	{
		Waypoint *pWaypoint = m_WaypointList[m_SelectedWaypoint];

		//////////////////////////////////////////////////////////////////////////
		// Display a radius indicator
		Vector3f vRadiusPos = GetDisplayPosition(pWaypoint->GetPosition());
		Utils::DrawRadius(vRadiusPos, pWaypoint->GetRadius(), g_RadiusIndicator, max(0.1f, g_fWaypointTextDuration));
		//////////////////////////////////////////////////////////////////////////	

		// Build a string with the waypoint info
		GameEntity ge = Utils::GetLocalEntity();
		if(ge.IsValid())
		{
			String flagString = va("Waypoint %d, Radius %.2f, UID %d\n", 
				m_SelectedWaypoint, pWaypoint->GetRadius(), pWaypoint->GetUID()).c_str();

			if(!pWaypoint->m_WaypointName.empty())
				flagString += "Name: " + pWaypoint->m_WaypointName + "\n";

			FlagMap::const_iterator flagIt = m_WaypointFlags.begin();
			for( ; flagIt != m_WaypointFlags.end(); ++flagIt)
			{
				if(pWaypoint->IsFlagOn(flagIt->second) && flagIt->second != F_NAV_TEAMONLY)
				{
					flagString += flagIt->first;
					flagString += ", ";
				}
			}

			const PropertyMap::ValueMap &pm = pWaypoint->GetPropertyMap().GetProperties();
			PropertyMap::ValueMap::const_iterator cIt = pm.begin();
			for(; cIt != pm.end(); ++cIt)
			{
				flagString += va("\n%s %s", (*cIt).first.c_str(), (*cIt).second.c_str());
			}

			if(!flagString.empty())
			{
				Utils::PrintText(
					pWaypoint->GetPosition().AddZ(g_fWaypointTextOffset),
					COLOR::WHITE,
					g_fWaypointTextDuration,
					flagString.c_str());
			}
		}
	}
}

void PathPlannerWaypoint::UpdateNavRender()
{
	Prof(UpdateNavRender);

	GameEntity ge = Utils::GetLocalEntity();
	if(!ge.IsValid()) return; 

	if(!m_CreatingSector.m_SectorBounds.IsZero())
	{
		Vector3f vAimPos;
		AABB localaabb;
		if(Utils::GetLocalAimPoint(vAimPos) && Utils::GetLocalAABB(localaabb))
		{
			Vector3f vTmp;
			localaabb.CenterBottom(vTmp);
			localaabb.UnTranslate(vTmp);
			localaabb.Translate(vAimPos);
			localaabb.m_Mins[2] += 4.f;
			localaabb.CenterPoint(vTmp);

			Vector3f axis[] =
			{
				Vector3f( 1.0f, 0.0f, 0.0f),
				Vector3f(-1.0f, 0.0f, 0.0f),
				Vector3f(0.0f,  1.0f, 0.0f),
				Vector3f(0.0f, -1.0f, 0.0f),
			};

			bool axisdone[] =
			{
				false,
				false,
				false,
				false,
			};

			obTraceResult tr;

			//Utils::OutlineAABB(localaabb, COLOR::MAGENTA, 10.f, AABB::DIR_BOTTOM);

			//Utils::DrawLine(vTmp, vTmp+Vector3f::UNIT_Z * 10.f, COLOR::GREEN, 10.f);

			// Expand each axis.
			static obuint32 iMaxCycles = 1000;
			obuint32 iNumCycles = iMaxCycles;
			bool bExpanded = true;
			while(bExpanded && (--iNumCycles)>0)
			{
				bExpanded = false;
				for(int i = 0; i < 4; ++i)
				{
					if(axisdone[i])
						continue;

					float fAxisLen = localaabb.GetAxisLength(i<2?0:1);

					AABB traceaabb = localaabb;
					traceaabb.UnTranslate(vTmp);

					Vector3f vEnd = vTmp + axis[i] * ((fAxisLen/2.f) + 8.f);
					//Utils::DrawLine(vEnd, vEnd+Vector3f::UNIT_Z * 10.f, COLOR::GREEN, 10.f);
					EngineFuncs::TraceLine(tr, vTmp, vEnd, &traceaabb, 
						TR_MASK_FLOODFILL, Utils::GetLocalGameId(), False);

					if(tr.m_Fraction == 1.f)
					{
						localaabb.Expand(vEnd);
						bExpanded = true;
					}
					else
					{
						localaabb.Expand(tr.m_Endpos);
						axisdone[i] = true;
					}
				}
			}
			m_CreatingSector.m_SectorBounds = localaabb;
			Utils::OutlineAABB(localaabb, COLOR::MAGENTA, IGame::GetDeltaTimeSecs()*1.5f, AABB::DIR_BOTTOM);
		}
	}
	//////////////////////////////////////////////////////////////////////////
	// Render box selection
	if(m_BoxStart != Vector3f::ZERO)
	{
		Vector3f vAimPos;
		if(Utils::GetLocalAimPoint(vAimPos))
		{
			boxselect.Set(m_BoxStart, vAimPos);
			Utils::OutlineAABB(boxselect, COLOR::MAGENTA, IGame::GetDeltaTimeSecs() * 2.f, AABB::DIR_BOTTOM);

			boxselect.m_Mins[2] = -4096.f;
			boxselect.m_Maxs[2] = 4096.f;
		}
	}
	else
	{
		boxselect.Set(Vector3f::ZERO);
	}
	//////////////////////////////////////////////////////////////////////////

	for(obuint32 i = 0; i < g_SectorList.size(); ++i)
	{
		Utils::OutlineAABB(g_SectorList[i].m_SectorBounds, 
			COLOR::MAGENTA, IGame::GetDeltaTimeSecs()*2.f,AABB::DIR_BOTTOM);
	}

	//////////////////////////////////////////////////////////////////////////

	// render entity connections.
	for(int i = 0; i < MaxEntityConnections; ++i)
	{
		if(EntityConnections[i].Entity.IsValid())
		{
			AABB boxi;
			EngineFuncs::EntityWorldAABB(EntityConnections[i].Entity,boxi);
			Utils::OutlineAABB(boxi,COLOR::CYAN,IGame::GetDeltaTimeSecs() * 2.f);

			for(int j = 0; j < MaxEntityConnections; ++j)
			{
				if(EntityConnections[j].Entity.IsValid())
				{
					if(EntityConnections[i].ConnectionId == EntityConnections[j].ConnectionId)
					{
						AABB boxj;
						EngineFuncs::EntityWorldAABB(EntityConnections[j].Entity,boxj);

						Vector3f vStart, vEnd;
						boxi.CenterBottom(vStart);
						boxj.CenterBottom(vEnd);

						switch(EntityConnections[i].Direction)
						{
						case CON_TWO_WAY:
							{
								boxi.CenterPoint(vStart);
								boxj.CenterPoint(vEnd);
								break;
							}
						case CON_SOURCE:
							{
								boxi.CenterTop(vStart);
								boxj.CenterPoint(vEnd);
								break;
							}
						case CON_DEST:
							{
								boxj.CenterTop(vStart);
								boxi.CenterPoint(vEnd);
								break;
							}
						}

						obColor c = COLOR::CYAN;

						Utils::DrawLine(
							vStart,
							vEnd,
							c,
							IGame::GetDeltaTimeSecs() * 2.f);
					}
				}
			}
		}
	}
}

void PathPlannerWaypoint::DrawActiveFrame()
{
	Prof(DrawActiveFrame);

	GameEntity ge = Utils::GetLocalEntity();
	if(!ge.IsValid()) return;
	Vector3f vPosition, vFacing;
	g_EngineFuncs->GetEntityEyePosition(ge, vPosition);
	g_EngineFuncs->GetEntityOrientation(ge, vFacing, 0, 0);

	for(WaypointList::const_iterator itW = m_WaypointList.begin(); itW != m_WaypointList.end(); itW++)
	{
		Waypoint *pWp = *itW;
		Vector3f vP1 = pWp->GetPosition().AddZ(g_fTopPathOffset);

		bool inFOV = Utils::InFieldOfView120(vFacing, vP1 - vPosition);
		if(inFOV)
		{
			obColor color = g_WaypointColor;
			if(pWp->IsFlagOn(F_NAV_TEAM1))
				color = g_Team1;
			else if(pWp->IsFlagOn(F_NAV_TEAM2))
				color = g_Team2;
			else if(pWp->IsFlagOn(F_NAV_TEAM3))
				color = g_Team3;
			else if(pWp->IsFlagOn(F_NAV_TEAM4))
				color = g_Team4;

			// Is it selected?
			if(std::find(m_SelectedWaypoints.begin(), m_SelectedWaypoints.end(), pWp) != m_SelectedWaypoints.end()
			  || boxselect.Contains(pWp->GetPosition()))
				color = g_SelectedWaypoint;

			g_ClientFuncs->DrawLine(
			  pWp->GetPosition().AddZ(g_fTopWaypointOffset),
			  pWp->GetPosition().AddZ(g_fBottomWaypointOffset),
			  color);

			if(m_PlannerFlags.CheckFlag(WAYPOINT_VIEW_FACING) && pWp->GetFacing() != Vector3f::ZERO)
			{
				Vector3f vStart = pWp->GetPosition().AddZ(g_fFacingOffset);
				g_ClientFuncs->DrawLine(
				  vStart,
				  vStart + pWp->GetFacing() * 32.f,
				  g_ShowFacingColor);
			}
		}

		// Add the paths
		for(Waypoint::ConnectionList::iterator it = pWp->m_Connections.begin(); it != pWp->m_Connections.end(); it++)
		{
			Vector3f vP2 = it->m_Connection->GetPosition().AddZ(g_fBottomPathOffset);
			if(inFOV || Utils::InFieldOfView120(vFacing, vP2 - vPosition))
			{
				obColor color = it->m_Connection->IsConnectedTo(pWp) ? g_LinkColor2Way : g_LinkColor1Way;
				if(it->CheckFlag(F_LNK_CLOSED))
					color = g_LinkClosedColor;
				else if(it->CheckFlag(F_LNK_TELEPORT))
					color = g_LinkTeleport;

				g_ClientFuncs->DrawLine(vP1, vP2, color);
			}
		}
	}
}

int PathPlannerWaypoint::CheckBlockable()
{
	int count = 0;
	if (m_PathCheckCallback) 
	{
		ConnectionList::iterator it = m_BlockableList.begin(), itEnd = m_BlockableList.end();
		for (; it != itEnd; ++it)
		{
			if ((*it).second->m_ConnectionFlags & F_LNK_CLOSED)
				if (m_PathCheckCallback((*it).first, (*it).second->m_Connection, false) == B_PATH_OPEN) {
					(*it).second->m_ConnectionFlags &= ~F_LNK_CLOSED;
					count++;
				}
		}
	}
	return count;
}

void PathPlannerWaypoint::Update()
{
	Prof(PathPlannerWaypoint);

	if(IsViewOn()) UpdateNavRender();

	// TODO: run any time spliced paths.
	if(m_BlockableRegulator->IsReady() && m_PathCheckCallback)
	{
		bool bDrawTests = IGameManager::GetInstance()->GetGame()->DrawBlockableTests();

		std::vector<int> changedUID;

		ConnectionList::iterator it = m_BlockableList.begin(), itEnd = m_BlockableList.end();
		for( ; it != itEnd; ++it)
		{
			BlockableStatus bl = m_PathCheckCallback((*it).first, (*it).second->m_Connection, bDrawTests);

			switch(bl)
			{
			case B_PATH_OPEN:
				if ((*it).second->m_ConnectionFlags & F_LNK_CLOSED)
				{
					(*it).second->m_ConnectionFlags &= ~F_LNK_CLOSED;
				}
				break;
			case B_PATH_CLOSED:
				if (!((*it).second->m_ConnectionFlags & F_LNK_CLOSED))
				{
					(*it).second->m_ConnectionFlags |= F_LNK_CLOSED;

					if(std::find(changedUID.begin(), changedUID.end(), (*it).second->m_Connection->GetUID()) == changedUID.end())
						changedUID.push_back((*it).second->m_Connection->GetUID());
				}
				break;
			case B_INVALID_FLAGS:
				/*DEBUG_ONLY(std::cout << "Invalid flag combination in PathCheck detected!" << std::endl);
				EngineFuncs::ConsoleError(
				"Invalid flag combination detected. Please check \"blockable\" waypoints!");*/
				break;
			}

			// Show some feedback			
			if(IsViewOn())
			{
				Utils::DrawLine(
					(*it).first->GetPosition().AddZ(g_fBlockablePathOffset),
					(*it).second->m_Connection->GetPosition().AddZ(g_fBlockablePathOffset),
					((*it).second->m_ConnectionFlags & F_LNK_CLOSED) ? g_BlockableBlocked : g_BlockableOpen,
					(float)m_BlockableRegulator->GetInterval() / 1000.f);
			}
		}

		//repath if current path goes through any blocked connection
		for(std::vector<int>::iterator it2 = changedUID.begin(); it2 != changedUID.end(); ++it2)
		{
			Event_DynamicPathsChanged m(0xFFFF /* all teams */, (*it2));
			IGameManager::GetInstance()->GetGame()->DispatchGlobalEvent(
				MessageHelper(MESSAGE_DYNAMIC_PATHS_CHANGED, &m, sizeof(m)));
		}
	}

	if(IsViewOn())
	{
		RenderFailedPaths();

		obTraceResult tr;
		Vector3f vEndPos;
		Vector3f vPlayerPos;
		Vector3f vPlayerFacing;
		GameEntity ge = Utils::GetLocalEntity();
		if(ge.IsValid())
		{
			int iPlayerGameID = g_EngineFuncs->IDFromEntity(ge);

			EngineFuncs::EntityPosition(ge, vPlayerPos);
			EngineFuncs::EntityOrientation(ge, vPlayerFacing, NULL, NULL);

			// Clamp the line to any collisions.
			vEndPos = vPlayerPos + (vPlayerFacing * 1000.0f);				
			EngineFuncs::TraceLine(tr, vPlayerPos, vEndPos, NULL, TR_MASK_SOLID, iPlayerGameID, False);
			if(tr.m_Fraction != 1.0f)
			{
				vEndPos.x = tr.m_Endpos[0];
				vEndPos.y = tr.m_Endpos[1];
				vEndPos.z = tr.m_Endpos[2];
			}

			// Outline entities that we're pointing at.
			/*if(tr.m_HitEntity.IsValid())
			{
				AABB aabb;
				if(EngineFuncs::EntityWorldAABB(tr.m_HitEntity, aabb))
				{
					Utils::OutlineAABB(aabb, g_AimEntity, 2.0f);
				}					
			}*/

			// Create a Line from the players view vector.
			float fExtent = (vEndPos - vPlayerPos).Length() * 0.5f;
			Segment3f aimSeg(vPlayerPos + vPlayerFacing * fExtent, vPlayerFacing, fExtent);

			int iBestWp = -1;
			float fClosest = Utils::FloatMax;
			for(int i = 0; i < (int)m_WaypointList.size(); ++i)
			{
				Segment3f wpSeg = m_WaypointList[i]->GetSegment();

				DistSegment3Segment3f v3ToSeg(wpSeg, aimSeg);
				float fDist = v3ToSeg.GetSquared();
				if(fDist < fClosest)
				{
					fClosest = fDist;
					iBestWp = i;
				}
			}

			if(iBestWp != -1)
			{
				m_SelectedWaypoint = iBestWp;
				UpdateSelectedWpRender();
			}
		}

		// Handle moving waypoints
		if(m_MovingWaypointIndex != -1)
		{
			Vector3f vLocalPos;
			g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos);

			if(m_MovingWaypointIndex < (int)m_WaypointList.size() && m_MovingWaypointIndex >= 0)
			{			
				m_WaypointList[m_MovingWaypointIndex]->m_Position = vLocalPos;
			}
		}
	} 
	else
	{
		// No Waypoint Selected
		m_SelectedWaypoint = -1;
	}
}

void PathPlannerWaypoint::Shutdown()
{
	// TODO: cleanup any memory.
	Unload();
}

bool PathPlannerWaypoint::IsReady() const
{
	return (!m_WaypointList.empty());
}

bool PathPlannerWaypoint::SetWaypointName(Waypoint *_wp, const String &_name)
{
	if(_wp)
	{
		_wp->SetName(_name);
		return true;
	}
	return false;
}

bool PathPlannerWaypoint::SetWaypointName(int _index, const String &_name)
{
	if(InRangeT<int>(_index, 0, (int)m_WaypointList.size()-1))
	{
		Waypoint *pWp = m_WaypointList[_index];
		return SetWaypointName(pWp, _name);
	}
	return false;
}

Waypoint *PathPlannerWaypoint::GetWaypointByName(const String &_name) const
{
	WaypointList::const_iterator cIt = m_WaypointList.begin(), cItEnd = m_WaypointList.end();
	for(; cIt != cItEnd; ++cIt)
	{
		if((*cIt)->GetName() == _name)
			return (*cIt);
	}
	return 0;
}

void PathPlannerWaypoint::GetWaypointsByName(const String &_name, WaypointList &_list) const
{
	WaypointList::const_iterator cIt = m_WaypointList.begin(), cItEnd = m_WaypointList.end();
	for (; cIt != cItEnd; ++cIt)
	{
		if ((*cIt)->GetName() == _name)
			_list.push_back(*cIt);
	}
}

void PathPlannerWaypoint::GetWaypointsByExpr(const String &_expr, WaypointList &_list) const
{
	WaypointList::const_iterator cIt = m_WaypointList.begin(), cItEnd = m_WaypointList.end();
	for(; cIt != cItEnd; ++cIt)
	{
		if((*cIt)->GetName().size()>0 && Utils::RegexMatch(_expr.c_str(), (*cIt)->GetName().c_str()))
			_list.push_back(*cIt);
	}
}

Waypoint *PathPlannerWaypoint::GetWaypointByGUID(obuint32 _uid) const
{
	WaypointList::const_iterator cIt = m_WaypointList.begin(), cItEnd = m_WaypointList.end();
	for(; cIt != cItEnd; ++cIt)
	{
		if((*cIt)->m_UID == _uid)
			return (*cIt);
	}
	return 0;
}

bool PathPlannerWaypoint::GetNavFlagByName(const String &_flagname, NavFlags &_flag) const
{
	FlagMap::const_iterator it = m_WaypointFlags.find(_flagname);
	if(it != m_WaypointFlags.end())
	{
		_flag = it->second;
		return true;
	}
	_flag = 0;
	return false;
}

int PathPlannerWaypoint::PlanPathToNearest(Client *_client, const Vector3f &_start, const Vector3List &_goals, const NavFlags &_team)
{
	DestinationVector destlist;
	for(obuint32 i = 0; i < _goals.size(); ++i)
		destlist.push_back(Destination(_goals[i],0));
	return PlanPathToNearest(_client, _start, destlist, _team);
}

int PathPlannerWaypoint::PlanPathToNearest(Client *_client, const Vector3f &_start, const DestinationVector &_goals, const NavFlags &_team)
{
	if(_goals.size() == 1 && Length(_goals[0].m_Position, _start) <= _goals[0].m_Radius)
	{
		//bot is already at goal
		m_Solution.clear();
		m_PlannerFlags.SetFlag(NAV_FOUNDGOAL);
		return 0;
	}

	m_Client = _client;
	m_GoalIndex = 0;

	//m_Start = _GetClosestLink(_start, _team);
	m_Start = ClosestLink(_GetClosestWaypoint(_start, _team, SKIP_NO_CONNECTIONS, NULL));

	if(m_Start.IsValid())
	{
		++m_WaypointMark;

		m_PlannerFlags.ClearFlag(WAYPOINT_ISDONE);
		m_PlannerFlags.ClearFlag(NAV_FOUNDGOAL);

		if(!_goals.empty())
		{
			// Mark all destinations
			for(obuint32 i = 0; i < _goals.size(); ++i)
			{
				Waypoint *pEnd = _GetClosestWaypoint(_goals[i].m_Position, _team, SKIP_NO_CONNECTIONS, NULL);	
				ClosestLink cl = ClosestLink(pEnd);

				//ClosestLink cl = _GetClosestLink(_goals[i], _team);
				if(cl.IsValid())
				{
					for(int g = 0; g < ClosestLink::NumWps; ++g)
					{
						if(cl.m_Wp[g])
						{
							cl.m_Wp[g]->m_Mark = m_WaypointMark;
							cl.m_Wp[g]->m_GoalIndex = i;
						}
					}
				}
			}

			// Clear the lists
			m_OpenList.resize(0);

			// Start the open list with this waypoint.
			for(int i = 0; i < ClosestLink::NumWps; ++i)
			{
				if(m_Start.m_Wp[i])
				{
					m_OpenList.push_back(m_Start.m_Wp[i]);
				}
			}

#ifdef ASTAR_ENABLED
			_RunAStar(_team, _goals[0].m_Position);
#else
			_RunDijkstra(_team);
#endif
			return m_GoalIndex;
		}
	}

	// None exist.
	m_PlannerFlags.SetFlag(WAYPOINT_ISDONE);
	m_PlannerFlags.ClearFlag(NAV_FOUNDGOAL);

	return m_GoalIndex;
}

void PathPlannerWaypoint::PlanPathToGoal(Client *_client, const Vector3f &_start, const Vector3f &_goal, const NavFlags _team)
{
	DestinationVector destlist;
	destlist.push_back(Destination(_goal,0));
	PlanPathToNearest(_client, _start, destlist, _team);

	if(!FoundGoal() && m_PlannerFlags.CheckFlag(NAV_SAVEFAILEDPATHS))
	{
		AddFailedPath(_start, _goal);
	}
}

void PathPlannerWaypoint::_RunDijkstra(const NavFlags _team)
{
	Prof(RunDijkstra);

	int closedCount=0, openCount=0;
	int serial = ++m_PathSerial;

	// Initialize start waypoint
	for(WaypointList::const_iterator cIt = m_OpenList.begin(); cIt != m_OpenList.end(); ++cIt)
	{
		Waypoint *wp = *cIt;
		wp->m_Parent = 0;
		wp->m_FinalCost = 0;
		wp->m_PathSerial = serial;
	}

	// Look for the path.
	while(!m_OpenList.empty())
	{
		// Grab the next best node.
		Waypoint *pCurrentNode = m_OpenList.front();
		std::pop_heap(m_OpenList.begin(), m_OpenList.end(), waypoint_comp);
		m_OpenList.pop_back();
		closedCount++;

		// Did we find the goal?
		if(pCurrentNode->m_Mark == m_WaypointMark)
		{
			m_GoalIndex = pCurrentNode->m_GoalIndex;
			m_Solution.resize(0);

			// Build the solution.
			while(pCurrentNode != NULL)
			{
				m_Solution.push_back(pCurrentNode);
				pCurrentNode = pCurrentNode->m_Parent;
			}
			m_PlannerFlags.SetFlag(NAV_FOUNDGOAL);
			break;
		}

		float fCurrentCost = pCurrentNode->m_FinalCost;

		Waypoint::ConnectionList::iterator it = pCurrentNode->m_Connections.begin();
		// Search all the connections from this waypoint
		for( ; it != pCurrentNode->m_Connections.end(); ++it)
		{
			Waypoint *pNextWp = it->m_Connection;

			// skip closed nodes
			if(pNextWp->m_FinalCost <= fCurrentCost && pNextWp->m_PathSerial == serial)
				continue;

			// Don't use waypoints we're not allowed to.
			if(pNextWp->IsFlagOn(F_NAV_TEAMONLY) && !pNextWp->IsFlagOn(_team) && _team
				&& (!pNextWp->IsFlagOn(F_NAV_INFILTRATOR) || !m_Client || !m_Client->IsInfiltrator()))
				continue;

			// take flags into account.
			if(pNextWp->IsFlagOn(F_NAV_CLOSED) || (it->m_ConnectionFlags & F_LNK_CLOSED))
				continue;

			if(m_CallbackFlags & pNextWp->GetNavigationFlags())
			{
				if( m_Client && m_Client->NavCallback(pNextWp->GetNavigationFlags(),pCurrentNode,pNextWp) == 0.f)
					continue;
			}

			// Calculate the successor cost.
			// Ignore distance to next node if this is flagged as a teleporter.
			float fSuccesorCost = pCurrentNode->m_FinalCost;
			if (!(it->m_ConnectionFlags & F_LNK_TELEPORT))
			{
				float len = (pCurrentNode->GetPosition() - pNextWp->GetPosition()).Length();
				if (pNextWp->IsFlagOn(F_NAV_PRONE)) len *= 3.8f;
				else if (pNextWp->IsFlagOn(F_NAV_CROUCH)) len *= 3.2f;
				else if (pNextWp->IsFlagOn(F_NAV_SNEAK)) len *= 2.0f;
				fSuccesorCost += len;
			}

			if(pNextWp->m_PathSerial == serial)
			{
				// Check the open list for this node to see if it's better
				if(pNextWp->m_FinalCost > fSuccesorCost)
				{
					// Set better cost
					pNextWp->m_Parent		= pCurrentNode;
					pNextWp->m_FinalCost		= fSuccesorCost;
					// Since we possibly changed cost in the middle, we need to fix the heap again.
					WaypointList::iterator openIt = std::find(m_OpenList.begin(), m_OpenList.end(), pNextWp);
					std::push_heap(m_OpenList.begin(), openIt+1, waypoint_comp);
				}
			}
			else
			{
				// New node to explore
				pNextWp->m_Parent		= pCurrentNode;
				pNextWp->m_FinalCost		= fSuccesorCost;
				pNextWp->m_PathSerial	= serial;
				HeapInsert(m_OpenList, pNextWp);
				
				int sz = (int) m_OpenList.size();
				if(sz > openCount) openCount = sz;
			}
		}
	}

	if(m_PlannerFlags.CheckFlag(NAV_FOUNDGOAL))
		++m_GoodPathQueries;
	else
		++m_BadPathQueries;

	m_OpenCount = openCount;
	m_ClosedCount = closedCount;

	m_PlannerFlags.SetFlag(WAYPOINT_ISDONE);
}

void PathPlannerWaypoint::_RunAStar(const NavFlags _team, const Vector3f &_goalPosition)
{
	Prof(RunAStar);

	int closedCount=0, openCount=0;
	int serial = ++m_PathSerial;

	// Initialize start waypoint
	for(WaypointList::const_iterator cIt = m_OpenList.begin(); cIt != m_OpenList.end(); ++cIt)
	{
		Waypoint *wp = *cIt;
		wp->m_Parent = 0;
		wp->m_GivenCost = 0;
		wp->m_HeuristicCost = (_goalPosition - wp->m_Position).Length();
		wp->m_FinalCost = wp->m_HeuristicCost/**m_HeuristicWeight*/;
		wp->m_PathSerial = serial;
	}

	// Look for the path.
	while(!m_OpenList.empty())
	{
		// Grab the next best node.
		Waypoint *pCurrentNode = m_OpenList.front();
		std::pop_heap(m_OpenList.begin(), m_OpenList.end(), waypoint_comp);
		m_OpenList.pop_back();
		closedCount++;

		// Did we find the goal?
		if(pCurrentNode->m_Mark == m_WaypointMark)
		{
			m_GoalIndex = pCurrentNode->m_GoalIndex;
			m_Solution.resize(0);

			// Build the solution.
			while(pCurrentNode != NULL)
			{
				m_Solution.push_back(pCurrentNode);
				pCurrentNode = pCurrentNode->m_Parent;
			}
			m_PlannerFlags.SetFlag(NAV_FOUNDGOAL);
			break;
		}

		float fCurrentCost = pCurrentNode->m_GivenCost;

		Waypoint::ConnectionList::iterator it = pCurrentNode->m_Connections.begin();
		// Search all the connections from this waypoint
		for( ; it != pCurrentNode->m_Connections.end(); ++it)
		{
			Waypoint *pNextWp = it->m_Connection;

			// skip closed nodes
			if(pNextWp->m_GivenCost <= fCurrentCost && pNextWp->m_PathSerial == serial)
				continue;

			// Don't use waypoints we're not allowed to.
			if(pNextWp->IsFlagOn(F_NAV_TEAMONLY) && !pNextWp->IsFlagOn(_team) && _team
				&& (!pNextWp->IsFlagOn(F_NAV_INFILTRATOR) || !m_Client || !m_Client->IsInfiltrator()))
				continue;

			// TODO: take flags into account.
			if(pNextWp->IsFlagOn(F_NAV_CLOSED) || (it->m_ConnectionFlags & F_LNK_CLOSED))
				continue;

			// If this waypoint has one of the movement cap flags, make sure the query supports it.
			if(m_CallbackFlags & pNextWp->GetNavigationFlags())
			{
				if( m_Client && m_Client->NavCallback(pNextWp->GetNavigationFlags(),pCurrentNode,pNextWp) == 0.f)
					continue;
			}

			// Calculate the successors given cost(parents given + distance from parent to me)
			// Ignore distance to next node if this is flagged as a teleporter.
			float fSuccesorCost = pCurrentNode->m_GivenCost;
			if(!(it->m_ConnectionFlags & F_LNK_TELEPORT))
				fSuccesorCost += (pCurrentNode->GetPosition() - pNextWp->GetPosition()).Length();

			if(pNextWp->m_PathSerial == serial)
			{
				// See if this would have a better cost.
				if(pNextWp->m_GivenCost > fSuccesorCost)
				{
					// Set better cost
					pNextWp->m_Parent = pCurrentNode;
					pNextWp->m_GivenCost = fSuccesorCost;
					pNextWp->m_FinalCost = fSuccesorCost + pNextWp->m_HeuristicCost/**m_HeuristicWeight*/;

					// Find node in the open list
					WaypointList::iterator openIt = std::find(m_OpenList.begin(), m_OpenList.end(), pNextWp);
					if(openIt != m_OpenList.end())
					{
					// Since we possibly changed cost in the middle, we need to fix the heap again.
						std::push_heap(m_OpenList.begin(), openIt+1, waypoint_comp);
					}
					else
					{
						// ... back into the open list
						HeapInsert(m_OpenList, pNextWp);					
						closedCount--;
					}
				}
			}
			else
			{
				// New node to explore
				pNextWp->m_Parent		= pCurrentNode;
				pNextWp->m_GivenCost		= fSuccesorCost;
				pNextWp->m_HeuristicCost	= (_goalPosition - pNextWp->m_Position).Length();
				pNextWp->m_FinalCost		= fSuccesorCost + pNextWp->m_HeuristicCost/**m_HeuristicWeight*/;
				pNextWp->m_PathSerial	= serial;
				HeapInsert(m_OpenList, pNextWp);

				int sz = (int) m_OpenList.size();
				if(sz > openCount) openCount = sz;
			}
		}
		// Have we run too long?
		/*if (m_Timer.GetElapsedSeconds() > m_TimeSliceSeconds)
		return false;*/
	}

	if(m_PlannerFlags.CheckFlag(NAV_FOUNDGOAL))
		++m_GoodPathQueries;
	else
		++m_BadPathQueries;

	m_OpenCount = openCount;
	m_ClosedCount = closedCount;

	m_PlannerFlags.SetFlag(WAYPOINT_ISDONE);
}

bool PathPlannerWaypoint::IsDone() const
{
	return m_PlannerFlags.CheckFlag(WAYPOINT_ISDONE);
}

bool PathPlannerWaypoint::FoundGoal() const
{
	return m_PlannerFlags.CheckFlag(NAV_FOUNDGOAL);
}

//bool PathPlannerWaypoint::_LoadVisFromFile(const String &_file)
//{
//	bool bGood = false;
//
//	File InFile;
//	InFile.OpenForRead(_file.c_str(), File::Binary);
//	if(InFile.IsOpen())
//	{
//		// Read the vis file header.
//		if(!InFile.Read(&m_VisFileHeader, sizeof(m_VisFileHeader)))
//			return false;
//
//		// Verify the proper version
//		if(m_VisFileHeader.m_VisVersion != VIS_VERSION_LATEST)
//			return false;
//
//		// Allocate enough bit tables.
//		m_VisTable.resize(m_VisFileHeader.m_NumWaypoints);
//
//		// If they don't match, don't bother loading the file.
//		if(!strncmp(m_VisFileHeader.m_TimeStamp, m_WaypointHeader.m_TimeStamp, sizeof(m_VisFileHeader.m_TimeStamp)))
//		{
//			for(obuint32 i = 0; i < m_VisFileHeader.m_NumWaypoints; ++i)
//			{
//				bGood = Utils::ReadDynamicBitsetFromFile(InFile, m_VisTable[i]);
//				OBASSERT(m_VisTable[i].size() == m_VisFileHeader.m_NumWaypoints, "Invalid Vis File");
//				if(!bGood)
//					break;
//			}
//		}
//		else
//		{
//			LOGWARN("Vis not loaded. Doesn't go with current waypoints.");
//			LOG((Format("Vis Header : %1% vs Wp Header %2%") % 
//				(m_VisFileHeader.m_TimeStamp[0] ? m_VisFileHeader.m_TimeStamp : "0") % 
//				(m_WaypointHeader.m_TimeStamp[0] ? m_WaypointHeader.m_TimeStamp : "0")).str());
//		}
//
//		InFile.Close();
//
//		if(bGood)
//		{
//			LOG((Format("Loaded vis from file : %1%") % _file.c_str()).str());
//		}
//		else
//		{
//			LOGERR((Format("Error Loading vis from file : %1%") % _file.c_str()).str());
//		}
//	}
//	return bGood;
//}

//bool PathPlannerWaypoint::_SaveVis(const String &_file, File &_file)
//{
//
//	return true;
//}
//
//bool PathPlannerWaypoint::_LoadVis(const String &_file, File &_file)
//{
//	bool bGood = false;
//
//	fstream InFile;
//	InFile.open(_file.c_str(), ios_base::in | ios_base::binary);
//	if(InFile.is_open())
//	{
//		// Write the vis file header.
//		if(!_proxy.Read((char*)&m_VisFileHeader, sizeof(m_VisFileHeader)))
//			return false;
//
//		// Allocate enough bit tables.
//		m_VisTable.resize(m_VisFileHeader.m_NumWaypoints);
//
//		// If they don't match, don't bother loading the file.
//		if(!strncmp(m_VisFileHeader.m_TimeStamp, m_WaypointHeader.m_TimeStamp, sizeof(m_VisFileHeader.m_TimeStamp)))
//		{
//			for(obuint32 i = 0; i < m_VisFileHeader.m_NumWaypoints; ++i)
//			{
//				std::stringstream str;
//				str << m_VisTable[i] << "#";
//				_proxy.Read()
//				assert(m_VisTable[i].size() == m_VisFileHeader.m_NumWaypoints);
//				if(!bGood)
//					break;
//			}
//		}		
//		if(bGood)
//		{
//			LOG((Format("Loaded vis from file : %1%") % _file.c_str()).str());
//		}
//		else
//		{
//			LOGERR((Format("Error Loading vis from file : %1%") % _file.c_str()).str());
//		}
//	}
//	return bGood;
//}

//bool PathPlannerWaypoint::_SaveVisToFile(const String &_file)
//{
//	bool bGood = false;
//
//	File OutFile;
//	OutFile.OpenForWrite(_file.c_str(), File::Binary);
//	if(OutFile.IsOpen())
//	{
//		// Set up the vis file header.
//		m_VisFileHeader.m_VisVersion = VIS_VERSION_LATEST;
//		m_VisFileHeader.m_NumWaypoints = m_VisTable.size();
//		Utils::StringCopy(m_VisFileHeader.m_TimeStamp, m_WaypointHeader.m_TimeStamp, sizeof(m_VisFileHeader.m_TimeStamp));
//
//		// Write the vis file header.
//		OutFile.Write(&m_VisFileHeader, sizeof(m_VisFileHeader));
//
//		// Write the vis table out byte by byte. TODO: Optimize the size.
//		for(obuint32 i = 0; i < m_VisFileHeader.m_NumWaypoints; ++i)
//		{
//			bGood = Utils::WriteDynamicBitsetToFile(OutFile, m_VisTable[i]);
//			if(!bGood)
//				break;
//		}
//
//		OutFile.Close();
//
//		if(bGood)
//		{
//			LOG((Format("Saved vis to file : %1% : %2% bits") % _file.c_str() % m_VisTable.size()).str());
//		}
//	}
//	return bGood;
//}

bool PathPlannerWaypoint::Load(const String &_mapname, bool _dl)
{
	LOGFUNCBLOCK;

	if(_mapname.empty())
		return false;

	String waypointName		= _mapname + ".way";
	String visName			= _mapname + ".vis";

	String waypointPath	= String("nav/") + waypointName;
	String visPath		= String("nav/") + visName;

	bool bLoaded = false;

	// Loose files always take priority over archived files
	if(!waypointPath.empty())
	{
		LOG("Loading waypoint: " << waypointPath.c_str());
		bLoaded = LoadFromFile(waypointPath);
	}

	// Generate/load vis
	if(bLoaded)
	{
		//bool bVisLoaded = false;

		// Load vis for this map.
		//if(!visPath.empty())
		//{
		//	bVisLoaded = _LoadVisFromFile(visPath);
		//}
		//
		//if(!bVisLoaded)
		//{
		//	// todo: load from archive.
		//}

		//if(!bVisLoaded)
		//{
		//	BuildVisTable();
		//	if(_SaveVisToFile(visPath))
		//	{
		//		LOGERR((Format("Saved Vis File: %1%") % visPath.c_str()).str());
		//	}
		//	else
		//	{
		//		LOGERR((Format("Error Saving Vis File: %1%") % visPath.c_str()).str());
		//	}
		//}

		BuildBlockableList();
		BuildSpatialDatabase();
		return true;
	}

#ifdef ENABLE_FILE_DOWNLOADER
	if(_dl)
		FileDownloader::NavigationMissing(_mapname);		
#endif
	LOGERR("Waypoint " << waypointName.c_str() << " not found in file or archives.");
	return false;
}

void PathPlannerWaypoint::SetNavDir(String &navDir, const char *_file)
{
	const char* dir = PHYSFS_getRealDir(_file);
	if (dir) {
		const char* nav = strstr(dir, "incomplete_navs");
		if(nav)
		{
			navDir = nav;
			if(navDir.size() > 15 && navDir[15] == '\\') navDir[15] = '/';
		}
	}
}

bool PathPlannerWaypoint::LoadFromFile(const String &_file)
{
	Unload();

	File InFile;
	InFile.OpenForRead(_file.c_str(), File::Binary);

	m_NavDir = "";

	if(!InFile.IsOpen())
		return false;

	SetNavDir(m_NavDir, _file.c_str());

	// Read the waypoint header.
	memset(&m_WaypointHeader, 0, sizeof(m_WaypointHeader));
	InFile.Read(&m_WaypointHeader, sizeof(m_WaypointHeader));

	WaypointSerializers::iterator it = m_WaypointSerializer.find(m_WaypointHeader.m_WaypointVersion);
	if(it != m_WaypointSerializer.end())
	{
		// Call the right serializers load function
		m_WaypointList.resize(m_WaypointHeader.m_NumWaypoints);
		if(it->second->Load(InFile, m_WaypointList))
		{
			// Was it loaded with an old serializer?
			WaypointSerializers::reverse_iterator rIt = m_WaypointSerializer.rbegin();
			if(it->first < rIt->first)
			{
				LOGWARN("Loaded " << _file.c_str() << 
					" with old serializer v(" << (int)it->first << 
					"), latest v(%d)" << (int)rIt->first);
			}

			LOG(m_WaypointList.size() << " Waypoints Loaded from file " << _file.c_str());

			// delete bad wps
			for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
			{
				if(m_WaypointList[i]->IsFlagOn(F_NAV_DONTSAVE))
				{
					DeleteWaypoint(m_WaypointList[i]);
					i = 0;
				}
			}
			return true;			
		} 
		else
		{
			LOGERR(va("Unable to Load Waypoint: %s",_file.c_str()));
			Unload();
		}
	} 
	else
	{
		// No Serializer!
		LOGERR(va("No Serializer for waypoint %s : expected version %d",
			_file.c_str(),m_WaypointHeader.m_WaypointVersion));
	}
	return false;
}

bool PathPlannerWaypoint::Save(const String &_mapname)
{
	if(_mapname.empty())
		return false;

	String waypointName		= _mapname + ".way";

	String navPath	= (m_NavDir.empty() ? String("nav/") : m_NavDir + "/") + waypointName;

	// Create any directories if required.
	/*if(fs::create_directories(navPath))
	{
	LOG((Format("Creating Directory : %1%") % navPath.c_str()).str());
	}*/

	File OutFile;
	OutFile.OpenForWrite(navPath.c_str(), File::Binary, false);

	if(!OutFile.IsOpen())
		return false;

	// We need at least 1 serializer to save the waypoints out.
	// We'll use the last one in the map since it should be the latest version.
	if(!m_WaypointSerializer.empty())
	{
		WaypointSerializers::reverse_iterator rIt = m_WaypointSerializer.rbegin();

		// Write the waypoint header.
		m_WaypointHeader.m_WaypointVersion = rIt->first;
		m_WaypointHeader.m_NumWaypoints = (unsigned int)m_WaypointList.size();
		g_EngineFuncs->GetMapExtents(m_WaypointHeader.m_WorldAABB);

		// ZERO THE COMMENTS
		memset(&m_WaypointHeader.m_TimeStamp, 0, sizeof(m_WaypointHeader.m_TimeStamp));
		memset(&m_WaypointHeader.m_Comments, 0, sizeof(m_WaypointHeader.m_Comments));

		// Save the timestamp.
		time_t tme = time(NULL);
		const char *timeStamp = asctime(localtime(&tme));
		Utils::StringCopy(m_WaypointHeader.m_TimeStamp, timeStamp, sizeof(m_WaypointHeader.m_TimeStamp));

		// Write the waypoint header.
		OutFile.Write((char *)&m_WaypointHeader, sizeof(m_WaypointHeader));

		// Call the right serializers save function
		if(rIt->second->Save(OutFile, m_WaypointList))
		{
			// Successful
			LOG(m_WaypointList.size() << " Waypoints Saved to " <<
				waypointName << " using version #" << 
				rIt->first << " format");
			return true;
		} 
		else
		{
			LOGERR(va("Unable to Save Waypoint: %s",waypointName.c_str()));
		}
	} 
	else
	{
		// No Serializer!
		LOGCRIT("No Serializers!");
	}
	return true;
}

void PathPlannerWaypoint::Unload()
{
	WaypointList::iterator it = m_WaypointList.begin();
	for( ;it != m_WaypointList.end(); ++it)
		delete (*it);

	m_SelectedWaypoints.clear();
	m_WaypointList.clear();
	m_BlockableList.clear();
}

int PathPlannerWaypoint::_MarkWaypointsInRadius(const Vector3f &_pos, const NavFlags _team, int _flags)
{
	++m_WaypointMark;

	int iNumMarked = 0;

	if(IsReady())
	{
		float fWaypointDistance, fClosestDistance = Utils::FloatMax;
		Waypoint *pClosestWaypoint = 0, *pWaypoint;

		for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
		{
			pWaypoint = m_WaypointList[i];

			if(pWaypoint->IsFlagOn(F_NAV_CLOSED))
				continue;

			// Don't use waypoints we're not allowed to.
			if(_team && pWaypoint->IsFlagOn(F_NAV_TEAMONLY) && !pWaypoint->IsFlagOn(_team))
				continue;

			// Skip lone waypoints.
			if(pWaypoint->m_Connections.empty())
				continue;

			fWaypointDistance = (pWaypoint->GetPosition()-_pos).SquaredLength();

			// Mark necessary waypoints.
			if(_flags & ALL_IN_RADIUS || _flags & ALL_IN_RADIUS_LOS)
			{
				if(fWaypointDistance < pWaypoint->GetRadius())
				{
					if(_flags & ALL_IN_RADIUS_LOS)
					{
						// Do a line of sight test.
					}
					else
					{
						pWaypoint->m_Mark = m_WaypointMark;
					}
				}
			}

			if(_flags & CLOSEST_WP)
			{
				if (fWaypointDistance < fClosestDistance)
				{
					pClosestWaypoint = pWaypoint;
					fClosestDistance = fWaypointDistance;
				}
			}			
		}

		// Mark the closest waypoint if requested.
		if(_flags & CLOSEST_WP && pClosestWaypoint)
		{
			pClosestWaypoint->m_Mark = m_WaypointMark;
		}
	}

	return iNumMarked;
}

Waypoint *PathPlannerWaypoint::_GetClosestWaypoint(const Vector3f &_pos, 
												   const NavFlags _team, 
												   const int _options, 
												   int *_index) const
{
	Prof(GetClosestWaypoint);

	float fClosestDistance = Utils::FloatMax;
	Waypoint *pClosestWaypoint = 0;

	// only proceed if we got waypoints
	if (IsReady())
	{
		obuint32 size = (obuint32) m_WaypointList.size();
		for(obuint32 i = 0; i < size; ++i)
		{
			Waypoint *pWaypoint = m_WaypointList[i];

			if(pWaypoint->IsFlagOn(F_NAV_CLOSED) && !(_options & NOFILTER))
				continue;

			// Don't use waypoints we're not allowed to.
			if(pWaypoint->IsFlagOn(F_NAV_TEAMONLY) && !pWaypoint->IsFlagOn(_team) && _team)
				continue;

			// Skip lone waypoints.
			if(pWaypoint->m_Connections.empty() && (_options & SKIP_NO_CONNECTIONS))
				continue;

			Vector3f vDist = pWaypoint->GetPosition() - _pos;
			float fWaypointDistance = vDist.x * vDist.x;
			if (fWaypointDistance >= fClosestDistance) continue;
			fWaypointDistance += vDist.y * vDist.y;
			if (fWaypointDistance >= fClosestDistance) continue;
			fWaypointDistance += vDist.z * vDist.z;

			if (fWaypointDistance < fClosestDistance)
			{
				if(_index)
					*_index = i;

				pClosestWaypoint = pWaypoint;
				fClosestDistance = fWaypointDistance;
			}
		}

		// to make sure such bugs don't go unnoticed anymore
		//Utils::OutputDebug(kError, "Wp Not Found, _GetClosestWaypoint()");
	}

	return pClosestWaypoint;
}

PathPlannerWaypoint::ClosestLink PathPlannerWaypoint::_GetClosestLink(const Vector3f &_pos, const NavFlags _team) const
{
	Prof(GetClosestLink);

	float fClosestSegmentDistance = Utils::FloatMax;
	float fSegmentDistance;

	ClosestLink closest;
	closest.m_Position = _pos;

	// only proceed if we got waypoints
	if (IsReady())
	{
		Vector3f debugVec;
		Segment3f debugSegment;

		for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
		{
			Waypoint *pWaypoint = m_WaypointList[i];

			if(pWaypoint->IsFlagOn(F_NAV_CLOSED))
				continue;

			// Don't use waypoints we're not allowed to.
			if(_team && pWaypoint->IsFlagOn(F_NAV_TEAMONLY) && !pWaypoint->IsFlagOn(_team))
				continue;

			Waypoint::ConnectionList::const_iterator cIt = pWaypoint->m_Connections.begin(), 
				cItEnd = pWaypoint->m_Connections.end();
			for(; cIt != cItEnd; ++cIt)
			{
				// skip invalids
				if((*cIt).m_ConnectionFlags & F_LNK_CLOSED)
					continue;

				// Don't use waypoints we're not allowed to.
				if(_team && (*cIt).m_Connection->IsFlagOn(F_NAV_TEAMONLY) && !(*cIt).m_Connection->IsFlagOn(_team))
					continue;

				// create a line segment
				Vector3f vDir = (*cIt).m_Connection->GetPosition() - pWaypoint->GetPosition();
				float fLen = vDir.Normalize();

				float fExtent = fLen * 0.5f;
				Segment3f seg(
					pWaypoint->GetPosition() + vDir * fExtent, 
					vDir, fExtent);

				DistVector3Segment3f dtoseg(_pos, seg);
				fSegmentDistance = dtoseg.GetSquared();
				if(fSegmentDistance < fClosestSegmentDistance)
				{
					fClosestSegmentDistance = fSegmentDistance;
					closest.m_Wp[0] = (*cIt).m_Connection->IsConnectedTo(pWaypoint) ? pWaypoint : 0;
					closest.m_Wp[1] = (*cIt).m_Connection;

					debugVec = _pos;
					debugSegment = seg;
				}
			}
		}

#ifdef _DEBUG
		DistVector3Segment3f debugSeg(debugVec, debugSegment);
		Utils::DrawLine(
			debugSeg.GetSegment().GetPosEnd(), 
			debugSeg.GetSegment().GetNegEnd(), 
			COLOR::GREEN, 10.f);
		Utils::DrawLine(_pos, 
			debugSeg.GetClosestPoint0(), COLOR::CYAN, 10.f);
#endif

		// to make sure such bugs don't go unnoticed anymore
		//Utils::OutputDebug(kError, "Wp Not Found, _GetClosestWaypoint()");
	}

	return closest;
}

bool PathPlannerWaypoint::_ConnectWaypoints(Waypoint *_wp1, Waypoint *_wp2)
{
	if(_wp1 && _wp1->ConnectTo(_wp2))
	{
		if(_wp1->IsAnyFlagOn(m_BlockableMask) && _wp2->IsAnyFlagOn(m_BlockableMask))
			m_BlockableList.push_back(std::make_pair(_wp1, &_wp1->m_Connections.back()));
		return true;
	}
	return false;
}

bool PathPlannerWaypoint::_DisConnectWaypoints(Waypoint *_wp1, Waypoint *_wp2)
{
	if((_wp1 && _wp2) && (_wp1 != _wp2))
	{
		bool bRebuildBlockable = _wp1->IsAnyFlagOn(m_BlockableMask) && _wp2->IsAnyFlagOn(m_BlockableMask);

		Waypoint::ConnectionList::iterator it = _wp1->m_Connections.begin();
		for( ; it != _wp1->m_Connections.end(); ++it)
		{
			if(it->m_Connection == _wp2)
			{
				_wp1->m_Connections.erase(it);

				if(bRebuildBlockable)
				{
					BuildBlockableList();
				}

				return true;
			}
		}
	}
	return false;
}

bool waypoint_comp(const Waypoint* _wp1, const Waypoint* _wp2)
{
	return _wp1->m_FinalCost > _wp2->m_FinalCost;
}

void PathPlannerWaypoint::HeapInsert(WaypointList &_wpl, Waypoint *_wp)
{
	_wpl.push_back(_wp);
	std::push_heap(_wpl.begin(), _wpl.end(), waypoint_comp);
}

void PathPlannerWaypoint::BuildBlockableList()
{
	int iNumBlockablePaths = 0;
	m_BlockableList.resize(0);
	WaypointList::iterator it = m_WaypointList.begin(), itEnd = m_WaypointList.end();
	for( ; it != itEnd; ++it)
	{
		Waypoint * wp = (*it);

		// Only consider blockable waypoints.
		if(wp->IsAnyFlagOn(m_BlockableMask) == false)
			continue;		

		// Loop through all the connections on the waypoint.
		Waypoint::ConnectionList::iterator cIt = wp->m_Connections.begin(), 
			cEnd = wp->m_Connections.end();
		for( ; cIt != cEnd; ++cIt)
		{
			Waypoint::ConnectionInfo & ci = (*cIt);

			// connected to blockable waypoints.
			if(ci.m_Connection->IsAnyFlagOn(m_BlockableMask) == false)
				continue;

			// Add this connection to the list.
			m_BlockableList.push_back(std::make_pair(wp ,&ci));
			++iNumBlockablePaths;
		}
	}

	// maintain teleport flags
	it = m_WaypointList.begin();
	itEnd = m_WaypointList.end();
	for( ; it != itEnd; ++it)
	{
		Waypoint * wp = (*it);

		// Loop through all the connections on the waypoint.
		Waypoint::ConnectionList::iterator cIt = wp->m_Connections.begin(), 
			cEnd = wp->m_Connections.end();
		for( ; cIt != cEnd; ++cIt)
		{
			Waypoint::ConnectionInfo & ci = (*cIt);
			if(wp->IsFlagOn(F_NAV_TELEPORT) && ci.m_Connection->IsFlagOn(F_NAV_TELEPORT))
			{
				ci.SetFlag(F_LNK_TELEPORT);
			} 
			else 
			{
				ci.ClearFlag(F_LNK_TELEPORT);
			}
		}
	}

	LOG("Found " << iNumBlockablePaths << " blockable paths");
}

void PathPlannerWaypoint::ClearBlockable(Waypoint *_waypoint)
{
	if (!_waypoint->IsAnyFlagOn(m_BlockableMask))
	{
		ConnectionList::iterator bIt = m_BlockableList.begin();
		for (; bIt != m_BlockableList.end(); ++bIt)
		{
			if (bIt->first == _waypoint || bIt->second->m_Connection == _waypoint)
				bIt->second->ClearFlag(F_LNK_CLOSED);
		}
	}
}

//void PathPlannerWaypoint::BuildVisTable()
//{
//	m_VisTable.clear();
//	m_VisTable.resize(m_WaypointList.size());
//
//	const obuint32 iNumTests = m_WaypointList.size() * m_WaypointList.size();
//	float fTarget = 0.1f;
//
//	obTraceResult tr;
//	for(obuint32 i = 0; i < m_VisTable.size(); ++i)
//	{
//		m_VisTable[i].resize(m_WaypointList.size());
//
//		for(obuint32 j = 0; j < m_WaypointList.size(); ++j)
//		{
//			if(i == j)
//			{
//				// Can always see itself.
//				m_VisTable[i].set(j, true);
//			}
//			else
//			{
//				// traceline from i waypoint to j waypoint
//				EngineFuncs::TraceLine(tr, 
//					(m_WaypointList[i]->GetPosition() + Vector3f(0,0,40)),
//					(m_WaypointList[j]->GetPosition() + Vector3f(0,0,40)), NULL, 
//					(TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);			
//
//				// is it clear?
//				if(tr.m_Fraction == 1.0f)
//				{
//					m_VisTable[i].set(j, true);
//				}
//			}
//
//			obuint32 iCurrentTest = i * m_WaypointList.size() + j;
//			float fPc = (float)(iCurrentTest / iNumTests);
//			if(fPc >= fTarget)
//			{
//				EngineFuncs::ConsoleMessage("Building Vis Table: %d / %d : %f", 
//					iCurrentTest, iNumTests, fTarget * 10.f);
//				fTarget += 0.1f;
//			}
//		}
//	}
//
//	LOG((Format("Vis table built: %1%") % (m_VisTable.size() * m_VisTable.size())).str());
//}

void PathPlannerWaypoint::BuildSpatialDatabase()
{
#ifndef __arm__
	//LOG("Generating Spacial Database.");
	Vector3f center;
	m_WaypointHeader.m_WorldAABB.CenterPoint(center);
	LOG("World Extents: Center: " << center << " Size: " << 
		m_WaypointHeader.m_WorldAABB.GetAxisLength(0) << "x " <<
		m_WaypointHeader.m_WorldAABB.GetAxisLength(1) << "y " <<
		m_WaypointHeader.m_WorldAABB.GetAxisLength(2) << "z");

	// TODO: generate spacial db
#endif
}

// Creation methods
Waypoint *PathPlannerWaypoint::AddWaypoint(const Vector3f &_pos, const Vector3f &_facing, bool _blockdupe /*= false*/)
{
	if(_blockdupe)
	{
		Waypoint *pWp = _GetClosestWaypoint(_pos,0,NOFILTER);
		if(pWp != NULL && Length(pWp->GetPosition(), _pos) <= Mathf::EPSILON)
			return pWp;
	}

	Waypoint *pNewWaypoint = new Waypoint(_pos, m_DefaultWaypointRadius);
	pNewWaypoint->m_Facing = _facing;
	pNewWaypoint->AssignNewUID();
	EngineFuncs::ConsoleMessage("Adding Waypoint..");

	// Auto detect if this waypoint is under water.
	int iContents = g_EngineFuncs->GetPointContents(_pos);	
	if(iContents & CONT_WATER)
	{
		pNewWaypoint->AddFlag(F_NAV_UNDERWATER);
		EngineFuncs::ConsoleMessage("Waypoint Under water, Auto Flagging");
	}

	BitFlag64 entFlags;
	if(InterfaceFuncs::GetEntityFlags(g_EngineFuncs->EntityFromID(Utils::GetLocalGameId()), entFlags))
	{
		if(entFlags.CheckFlag(ENT_FLAG_PRONED))
			pNewWaypoint->AddFlag(F_NAV_PRONE);
		else if(entFlags.CheckFlag(ENT_FLAG_CROUCHED))
			pNewWaypoint->AddFlag(F_NAV_CROUCH);

		if(entFlags.CheckFlag(ENT_FLAG_INWATER))
			pNewWaypoint->AddFlag(F_NAV_INWATER);
	}

	m_WaypointList.push_back(pNewWaypoint);
	return pNewWaypoint;
}

bool PathPlannerWaypoint::DeleteWaypoint(const Vector3f &_pos)
{
	// Delete a waypoint close to this location.
	Waypoint *pDeleteMe = _GetClosestWaypoint(_pos, 0, NOFILTER);

	if(pDeleteMe && (pDeleteMe->GetPosition() - _pos).Length() < 100.0f)
	{
		DeleteWaypoint(pDeleteMe);
		return true;
	}
	return false;
}

void PathPlannerWaypoint::DeleteWaypoint(Waypoint *pDeleteMe)
{
	// Clear the connect point if we're going to delete it.
	if(m_ConnectWp == pDeleteMe)
		m_ConnectWp = 0;

	// Remove it from the selected waypoint list.
	WaypointList::iterator newEnd = std::remove(m_SelectedWaypoints.begin(), m_SelectedWaypoints.end(), pDeleteMe);
	if(newEnd != m_SelectedWaypoints.end())
		m_SelectedWaypoints.erase(newEnd, m_SelectedWaypoints.end());

	// Remove from the blockable list.
	ConnectionList::iterator bIt = m_BlockableList.begin();
	while(bIt != m_BlockableList.end())
	{
		if((bIt->first == pDeleteMe) || (bIt->second->m_Connection == pDeleteMe))
			bIt = m_BlockableList.erase(bIt);
		else
			++bIt;
	}

	// Delete it from the list.
	int index = 0;
	WaypointList::iterator it = m_WaypointList.begin();
	for(; it != m_WaypointList.end(); ++it, ++index)
	{
		if((*it) == pDeleteMe)
		{
			delete (*it);
			m_WaypointList.erase(it);
			break;
		}
	}

	// Clear the moving waypoint if we delete.
	if(m_MovingWaypointIndex == index)
		m_MovingWaypointIndex = -1;
	else if(m_MovingWaypointIndex > index)
		m_MovingWaypointIndex--;

	// Eliminate all connections to this waypoint.
	it = m_WaypointList.begin();
	while(it != m_WaypointList.end())
	{
		(*it)->DisconnectFrom(pDeleteMe);
		++it;
	}
}

void PathPlannerWaypoint::RegisterNavFlag(const String &_name, const NavFlags &_bits)
{
	LOG("Registered Waypoint Flag: " << _name);

	String flagName = _name;
	std::transform(flagName.begin(), flagName.end(), flagName.begin(), toLower());

	// Error check.
	OBASSERT(m_WaypointFlags.find(flagName) == m_WaypointFlags.end(), "Flag Already Exists!");
	m_WaypointFlags.insert(std::make_pair(flagName, _bits));
}

Vector3f PathPlannerWaypoint::GetRandomDestination(Client *_client, const Vector3f &_start, const NavFlags _team)
{
	Vector3f dest = _start;

	WaypointList reachableWps;
	_FindAllReachable(_client, _start, _team, reachableWps);
	
	if(!reachableWps.empty())
	{
		int ix = rand() % (int)reachableWps.size();
		const float offset = g_fBottomWaypointOffset + (g_fTopWaypointOffset-g_fBottomWaypointOffset) * g_fPathLevelOffset;
		dest = reachableWps[ix]->GetPosition().AddZ(offset);
	}
	return dest;
}

void PathPlannerWaypoint::RegisterGameGoals()
{
	GoalManager *pGm = GoalManager::GetInstance();
	for(int i = 0; i < (int)m_WaypointList.size(); ++i)
	{
		pGm->CheckWaypointForGoal(m_WaypointList[i]);
	}
}

void PathPlannerWaypoint::GetPath(Path &_path, int _smoothiterations)
{
	const float offset = g_fBottomWaypointOffset + (g_fTopWaypointOffset-g_fBottomWaypointOffset) * g_fPathLevelOffset;
	
	bool bFirst = true;
	while(!m_Solution.empty())
	{
		// Center the waypoint position according to offsets.
		Waypoint *pWp = m_Solution.back();
		Vector3f vWpPos = pWp->GetPosition().AddZ(offset);

		if(bFirst)
		{
			bFirst = false;

			if(m_Solution.size() > 1)
			{
				if(m_Client && !pWp->OnPathThrough())
				{
					Waypoint *pWp2 = m_Solution[m_Solution.size()-2];
					Vector3f vNextWpPos = pWp2->GetPosition().AddZ(offset);
					Vector3f vClosest;
					float t = Utils::ClosestPtOnLine(vWpPos, vNextWpPos, m_Client->GetPosition(), vClosest);
					if(t > 0.f) //bot is near connection between the first and second waypoint
					{
						_path.AddPt(vClosest, pWp->GetRadius() + t * (pWp2->GetRadius() - pWp->GetRadius()))
							.Flags(pWp->GetNavigationFlags());

						m_Solution.pop_back();
						continue;
					}
				}
			}
		}

		_path.AddPt(vWpPos, pWp->GetRadius())
			.Flags(pWp->GetNavigationFlags())
			.OnPathThrough(pWp->OnPathThrough())
			.NavId(pWp->GetUID())
			.OnPathThroughParam(pWp->OnPathThroughParam());

		m_Solution.pop_back();
	}
}

obuint32 PathPlannerWaypoint::SelectWaypoints(const AABB &_box)
{
	obuint32 iNumSelected = 0;
	for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
	{
		if(_box.Contains(m_WaypointList[i]->GetPosition()))
		{
			if(std::find(m_SelectedWaypoints.begin(), m_SelectedWaypoints.end(), m_WaypointList[i])==m_SelectedWaypoints.end())
			{
				++iNumSelected;
				m_SelectedWaypoints.push_back(m_WaypointList[i]);
			}
		}
	}
	return iNumSelected;
}

bool PathPlannerWaypoint::GroundPosition(Vector3f &out, const Vector3f &p, bool offsetforwp)
{
	obTraceResult tr;
	EngineFuncs::TraceLine(tr,p.AddZ(32),p.AddZ(-2048),NULL,TR_MASK_FLOODFILL,-1,False);
	out = Vector3f(tr.m_Endpos).AddZ(offsetforwp ? -g_fBottomWaypointOffset : 0);
	return tr.m_Fraction < 1.f && !tr.m_StartSolid;
}

void PathPlannerWaypoint::SliceLink(Waypoint *wp0, Waypoint *wp1, float _maxlen)
{
	const float mid = (g_fTopWaypointOffset + g_fBottomWaypointOffset)/2;
	Vector3f wp0mid = wp0->GetPosition().AddZ(mid);
	Vector3f wp1mid = wp1->GetPosition().AddZ(mid);

	float fLinkLen = Length(wp0mid, wp1mid);

	if(fLinkLen <= _maxlen)
		return;

	Vector3f vNormal = Normalize(wp1mid - wp0mid);

	const bool con0to1 = wp0->IsConnectedTo(wp1);
	const bool con1to0 = wp1->IsConnectedTo(wp0);
	if(con0to1)
		wp0->DisconnectFrom(wp1);
	if(con1to0)
		wp1->DisconnectFrom(wp0);

	const int iNumLinks = Mathf::Round(fLinkLen / _maxlen);
	const float fInterval = fLinkLen / (float)iNumLinks;

	WaypointList wpl;
	wpl.push_back(wp0);
	for(float f = fInterval; f < fLinkLen-fInterval; f+=fInterval)
	{
		Vector3f p;
		if(GroundPosition(p, wp0mid + vNormal * f))
		{
			Waypoint *wp = AddWaypoint(p,Vector3f::ZERO,false);			
			wpl.push_back(wp);
		}
	}
	wpl.push_back(wp1);

	if(con0to1)
	{
		for(int i = 0; i < (int)wpl.size()-1; ++i)
		{
			wpl[i]->ConnectTo(wpl[i+1]);
		}
	}

	if(con1to0)
	{
		for(int i = (int)wpl.size()-1; i > 0; --i)
		{
			wpl[i]->ConnectTo(wpl[i-1]);
		}
	}
}

bool PathPlannerWaypoint::GetNavInfo(const Vector3f &pos,obint32 &_id,String &_name)
{
	Waypoint *pWp = _GetClosestWaypoint(pos,0,0,0);
	if(pWp)
	{
		_id = pWp->GetUID();
		_name = pWp->GetName();
		return true;
	}
	return false;
}

void PathPlannerWaypoint::AddEntityConnection(const Event_EntityConnection &_conn)
{
	int iFreeIndex = -1;
	for(int i = 0; i < MaxEntityConnections; ++i)
	{
		if(iFreeIndex == -1 && !EntityConnections[i].Entity.IsValid())
			iFreeIndex = i;

		if(EntityConnections[i].Entity == _conn.m_Entity)
		{
			iFreeIndex = i;
			break;
		}
	}

	Vector3f vPos;
	if(iFreeIndex != -1 && EngineFuncs::EntityPosition(_conn.m_Entity,vPos))
	{
		EntityConnections[iFreeIndex].Entity = _conn.m_Entity;
		EntityConnections[iFreeIndex].ConnectionId = _conn.m_ConnectionId;
		EntityConnections[iFreeIndex].Direction = _conn.m_ConnectionDir;

		EntityConnections[iFreeIndex].Wp = AddWaypoint(vPos, Vector3f::ZERO, false);
		EntityConnections[iFreeIndex].Wp->SetRadius(_conn.m_Radius);
		EntityConnections[iFreeIndex].Wp->SetEntity(_conn.m_Entity);
		EntityConnections[iFreeIndex].Wp->AddFlag(F_NAV_DONTSAVE);
		if(_conn.m_Teleport)
			EntityConnections[iFreeIndex].Wp->AddFlag(F_NAV_TELEPORT);

		// add team flag
		for(int i = 1; i <= 4; ++i)
		{
			if(_conn.m_Team.CheckFlag(i))
				EntityConnections[iFreeIndex].Wp->AddFlag(F_NAV_TEAM1 << i);
		}		

		Waypoint *pWp = _GetClosestWaypoint(vPos,0,SKIP_NO_CONNECTIONS,0);
		if(pWp)
		{
			// create 2 way connection to splice the new wp into the mesh
			pWp->ConnectTo(EntityConnections[iFreeIndex].Wp);
			EntityConnections[iFreeIndex].Wp->ConnectTo(pWp);
		}

		// see if the other connections exist.
		for(int i = 0; i < MaxEntityConnections; ++i)
		{
			if(iFreeIndex == i)
				continue;

			if(EntityConnections[iFreeIndex].ConnectionId == EntityConnections[i].ConnectionId)
			{
				obuint32 iFlags = 0;
				if(_conn.m_Teleport)
					iFlags |= F_LNK_TELEPORT;

				if(EntityConnections[iFreeIndex].Direction == CON_TWO_WAY ||
					EntityConnections[i].Direction == CON_TWO_WAY)
				{
					EntityConnections[iFreeIndex].Wp->ConnectTo(EntityConnections[i].Wp,iFlags);
					EntityConnections[i].Wp->ConnectTo(EntityConnections[iFreeIndex].Wp,iFlags);
				}
				else if(EntityConnections[iFreeIndex].Direction == CON_SOURCE ||
					EntityConnections[i].Direction == CON_DEST)
				{
					EntityConnections[iFreeIndex].Wp->ConnectTo(EntityConnections[i].Wp,iFlags);
				}
				else if(EntityConnections[iFreeIndex].Direction == CON_DEST ||
					EntityConnections[i].Direction == CON_SOURCE)
				{
					EntityConnections[i].Wp->ConnectTo(EntityConnections[iFreeIndex].Wp,iFlags);
				}
			}
		}
	}
}

void PathPlannerWaypoint::RemoveEntityConnection(GameEntity _ent)
{
	for(int i = 0; i < MaxEntityConnections; ++i)
	{
		if(EntityConnections[i].Entity == _ent)
		{
			EntityConnections[i].Entity.Reset();

			if(EntityConnections[i].Wp)
			{
				DeleteWaypoint(EntityConnections[i].Wp);
				EntityConnections[i].Wp = 0;
			}
		}
	}
}

Vector3f PathPlannerWaypoint::GetDisplayPosition(const Vector3f &_pos)
{
	return _pos.AddZ((g_fTopWaypointOffset + g_fBottomWaypointOffset)/2);
}

void PathPlannerWaypoint::_FindAllReachable(Client *_client, const Vector3f &_pos, const NavFlags &_team, WaypointList & reachable) {
	ClosestLink startLink = ClosestLink(_GetClosestWaypoint(_pos, _client->GetTeam(), SKIP_NO_CONNECTIONS, NULL));
	WaypointList openList;
	if(startLink.IsValid()) 
	{
		int serial = ++m_PathSerial;
		for(int i = 0; i < ClosestLink::NumWps; ++i)
		{
			if(startLink.m_Wp[i])
			{
				openList.push_back(startLink.m_Wp[i]);
				startLink.m_Wp[i]->m_PathSerial = serial;
			}
		}
		while(!openList.empty()) {
			Waypoint * pWp = openList.back();
			openList.pop_back();

			reachable.push_back(pWp);
			
			// loop through all connections
			Waypoint::ConnectionList::iterator it = pWp->m_Connections.begin();
			for( ; it != pWp->m_Connections.end(); ++it)
			{
				Waypoint * pNextWp = it->m_Connection;
				if(pNextWp->m_PathSerial==serial)
					continue;

				// Don't use waypoints we're not allowed to.
				if(_team && pNextWp->IsFlagOn(F_NAV_TEAMONLY) && !pNextWp->IsFlagOn(_team)
					&& (!pNextWp->IsFlagOn(F_NAV_INFILTRATOR) || !_client || !_client->IsInfiltrator()))
					continue;

				// TODO: take flags into account.
				if(pNextWp->IsFlagOn(F_NAV_CLOSED) || (it->m_ConnectionFlags & F_LNK_CLOSED))
					continue;

				if(m_CallbackFlags & pNextWp->GetNavigationFlags())
				{
					if( _client && _client->NavCallback(pNextWp->GetNavigationFlags(),pWp,pNextWp) == 0.f)
						continue;
				}

				openList.push_back(pNextWp);
				pNextWp->m_PathSerial = serial;
			}
		}
	}
	
}

void PathPlannerWaypoint::EntityCreated(const EntityInstance &ei)
{
	if(ei.m_EntityCategory.CheckFlag(ENT_CAT_OBSTACLE))
	{		
	}
}

void PathPlannerWaypoint::EntityDeleted(const EntityInstance &ei)
{
}

#ifdef ENABLE_REMOTE_DEBUGGING
void PathPlannerWaypoint::Sync( RemoteLib::DataBuffer & db, bool fullSync ) { 
	for(obuint32 i = 0; i < m_WaypointList.size(); ++i) {
		Waypoint *wp = m_WaypointList[i];
		wp->Sync( db, fullSync );
	}
}
#endif
