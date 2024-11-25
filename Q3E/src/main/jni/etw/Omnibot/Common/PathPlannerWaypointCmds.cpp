#include "PrecompCommon.h"
#include <limits>

#include "PathPlannerWaypoint.h"
#include "IGameManager.h"
#include "Waypoint.h"
#include "IGame.h"
#include "Client.h"
#include "ScriptManager.h"

using namespace std;

extern float g_fBottomWaypointOffset;
extern float g_fTopWaypointOffset;

void PathPlannerWaypoint::InitCommands()
{
	PathPlannerBase::InitCommands();

	SetEx("waypoint_add", "Adds a waypoint at the current position", 
		this, &PathPlannerWaypoint::cmdWaypointAdd);
	SetEx("waypoint_del", "Deletes a waypoint from the current position", 
		this, &PathPlannerWaypoint::cmdWaypointDelete);
	SetEx("waypoint_addx", "Adds a waypoint at the current aim position", 
		this, &PathPlannerWaypoint::cmdWaypointAddX);
	SetEx("waypoint_delx", "Deletes a waypoint from the current aim position", 
		this, &PathPlannerWaypoint::cmdWaypointDeleteX);
	SetEx("waypoint_stats", "Prints all waypoint stats", 
		this, &PathPlannerWaypoint::cmdWaypointStats);
	SetEx("waypoint_save", "Save current waypoints to disk", 
		this, &PathPlannerWaypoint::cmdWaypointSave);
	SetEx("waypoint_load", "Load last saved waypoints from disk", 
		this, &PathPlannerWaypoint::cmdWaypointLoad);
	SetEx("waypoint_autobuild", "Auto-connect waypoints", 
		this, &PathPlannerWaypoint::cmdWaypointAutoBuild);
	SetEx("waypoint_addflag", "Adds or removes navigation flags to this waypoint", 
		this, &PathPlannerWaypoint::cmdWaypointAddFlag);
	SetEx("waypoint_addflagx", "Adds or removes navigation flags to the waypoint in crosshairs", 
		this, &PathPlannerWaypoint::cmdWaypointAddFlagX);
	SetEx("waypoint_clearallflags", "Clears the flags from all or selected waypoints", 
		this, &PathPlannerWaypoint::cmdWaypointClearAllFlags);
	SetEx("waypoint_dcall", "Disconnect all waypoints", 
		this, &PathPlannerWaypoint::cmdWaypointDisconnectAll);
	SetEx("waypoint_view", "Turn on/off waypoint visibility", 
		this, &PathPlannerWaypoint::cmdWaypointView);
	SetEx("waypoint_autoflag", "Turn on/off waypoint auto flagging, not used", 
		this, &PathPlannerWaypoint::cmdWaypointAutoFlag);
	SetEx("waypoint_viewfacing", "Turn on/off rendering of the facing vector", 
		this, &PathPlannerWaypoint::cmdWaypointViewFacing);
	SetEx("waypoint_connect", "Create a path between 2 waypoints", 
		this, &PathPlannerWaypoint::cmdWaypointConnect);
	SetEx("waypoint_connectx", "Create a path between 2 waypoints in crosshairs", 
		this, &PathPlannerWaypoint::cmdWaypointConnectX);
	SetEx("waypoint_biconnect", "Create a bi-directional path between 2 waypoints", 
		this, &PathPlannerWaypoint::cmdWaypointConnect2Way);
	SetEx("waypoint_biconnectx", "Create a bi-directional path between 2 waypoints in crosshairs", 
		this, &PathPlannerWaypoint::cmdWaypointConnect2WayX);
	SetEx("waypoint_setdefaultradius", "Sets the radius for any future waypoints", 
		this, &PathPlannerWaypoint::cmdWaypointSetDefaultRadius);
	SetEx("waypoint_setradius", "Sets the radius for the closest waypoint", 
		this, &PathPlannerWaypoint::cmdWaypointSetRadius);
	SetEx("waypoint_changeradius", "Changes the radius by a specified value", 
		this, &PathPlannerWaypoint::cmdWaypointChangeRadius);
	SetEx("waypoint_setfacing", "Sets the facing for the closest waypoint", 
		this, &PathPlannerWaypoint::cmdWaypointSetFacing);
	SetEx("waypoint_info", "Prints information about the nearest waypoint", 
		this, &PathPlannerWaypoint::cmdWaypointInfo);
	SetEx("waypoint_goto", "Teleports local player to specified waypoint", 
		this, &PathPlannerWaypoint::cmdWaypointGoto);
	SetEx("waypoint_setname", "Assigns a name to closest waypoint", 
		this, &PathPlannerWaypoint::cmdWaypointSetName);
	SetEx("waypoint_setproperty", "Sets a user defined property of the closest waypoint.", 
		this, &PathPlannerWaypoint::cmdWaypointSetProperty);
	SetEx("waypoint_showproperty", "Shows all properties on the current waypoint.", 
		this, &PathPlannerWaypoint::cmdWaypointShowProperty);
	SetEx("waypoint_clearproperty", "Clears a user defined property of the closest waypoint.", 
		this, &PathPlannerWaypoint::cmdWaypointClearProperty);
	SetEx("waypoint_autoradius", "Automatically adjusts waypoint radius.", 
		this, &PathPlannerWaypoint::cmdWaypointAutoRadius);
	SetEx("waypoint_move", "Move a waypoint, keeping connections.", 
		this, &PathPlannerWaypoint::cmdWaypointMove);
	SetEx("waypoint_mirror", "Mirrors all current waypoints across a specified axis.", 
		this, &PathPlannerWaypoint::cmdWaypointMirror);
	SetEx("waypoint_deleteaxis", "Deletes all waypoints across a specified axis.", 
		this, &PathPlannerWaypoint::cmdWaypointDeleteAxis);
	SetEx("waypoint_clearcon", "Clears the connections from a waypoint.", 
		this, &PathPlannerWaypoint::cmdWaypointClearConnections);
	SetEx("waypoint_shownames", "Prints all waypoint id's and names that optionally match an expression", 
		this, &PathPlannerWaypoint::cmdWaypointGetWpNames);
	SetEx("waypoint_translate", "Translates all waypoints by a given amount.", 
		this, &PathPlannerWaypoint::cmdWaypointTranslate);
	SetEx("waypoint_color", "Customize color of various waypoints.", 
		this, &PathPlannerWaypoint::cmdWaypointColor);
	SetEx("waypoint_select", "Select all waypoints within a radius.", 
		this, &PathPlannerWaypoint::cmdSelectWaypoints);
	SetEx("waypoint_lockselected", "Selected waypoints will not be moved by waypoint_translate.", 
		this, &PathPlannerWaypoint::cmdLockSelected);
	SetEx("waypoint_unlockall", "Unlocks all waypoints.", 
		this, &PathPlannerWaypoint::cmdUnlockAll);
	SetEx("waypoint_autofeature", "Automatically waypoints jump pads, teleporters, player spawns.", 
		this, &PathPlannerWaypoint::cmdAutoBuildFeatures);
	SetEx("waypoint_boxselect", "Begin/end a box waypoint select.", 
		this, &PathPlannerWaypoint::cmdBoxSelect);
	SetEx("waypoint_boxselectroom", "Selects all waypoints in an auto created 'room'.", 
		this, &PathPlannerWaypoint::cmdBoxSelectRoom);
	SetEx("waypoint_minradius", "Clamps all waypoints minimum radius to this value", 
		this, &PathPlannerWaypoint::cmdMinRadius);
	SetEx("waypoint_maxradius", "Clamps all waypoints maximum radius to this value", 
		this, &PathPlannerWaypoint::cmdMaxRadius);
	SetEx("waypoint_slice", "Slices a connection so that it doesn't exceed a specified length.", 
		this, &PathPlannerWaypoint::cmdWaypointSlice);
	SetEx("waypoint_split", "Splits a connection to 2 parts at player position.", 
		this, &PathPlannerWaypoint::cmdWaypointSplit);
	SetEx("waypoint_unsplit", "Deletes a waypoint which has 2 connections and makes 1 connection.",
		this, &PathPlannerWaypoint::cmdWaypointUnSplit);
	SetEx("waypoint_ground", "Grounds all waypoints based on the navigation rendering offsets.",
		this, &PathPlannerWaypoint::cmdWaypointGround);
	
}

void PathPlannerWaypoint::cmdWaypointAdd(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	// get the position of the localhost
	Vector3f vPosition;
	g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vPosition);

	// Add this waypoint to the list.
	ScriptManager::GetInstance()->ExecuteStringLogged(
		(String)va("Wp.AddWaypoint( Vector3(%f, %f, %f));", 
		vPosition.x, vPosition.y, vPosition.z));
}

void PathPlannerWaypoint::cmdWaypointAddX(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	// get the position of the localhost
	Vector3f vAimPosition;
	if(Utils::GetLocalAimPoint(vAimPosition))
	{
		vAimPosition.z -= g_fBottomWaypointOffset;
		AddWaypoint(vAimPosition);
	}
}

void PathPlannerWaypoint::cmdWaypointDelete(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	// get the position of the localhost, at his feet
	Vector3f localPos;
	g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), localPos);

	// Add this waypoint to the list.
	if(m_SelectedWaypoints.empty())
	{
		if(DeleteWaypoint(localPos))
		{
			EngineFuncs::ConsoleMessage("Waypoint Deleted.");
		}
		else
		{
			EngineFuncs::ConsoleMessage("No Waypoint in range to delete..");
		}
	}
	else
	{
		while(!m_SelectedWaypoints.empty())
			DeleteWaypoint(m_SelectedWaypoints.back());
	}	
}

void PathPlannerWaypoint::cmdWaypointDeleteX(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	// get the position of the localhost
	Vector3f vEyePosition, vFacing;
	GameEntity ge = Utils::GetLocalEntity();
	if(ge.IsValid())
	{
		g_EngineFuncs->GetEntityEyePosition(ge, vEyePosition);
		g_EngineFuncs->GetEntityOrientation(ge, vFacing, 0, 0);

		obTraceResult tr;
		g_EngineFuncs->TraceLine(tr,
			vEyePosition, 
			vEyePosition + vFacing * 1000.f,
			NULL, 
			TR_MASK_SHOT, 
			Utils::GetLocalGameId(), 
			False);

		if(tr.m_Fraction < 1.f)
		{
			Vector3f vWaypointPos(tr.m_Endpos);
			vWaypointPos.z -= g_fBottomWaypointOffset;
			DeleteWaypoint(vWaypointPos);
		}
	}
}
void PathPlannerWaypoint::cmdWaypointStats(const StringVector &_args)
{
	//if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
	//	return;

	EngineFuncs::ConsoleMessage("-= Waypoint Stats =-");
	EngineFuncs::ConsoleMessage(va("Map : %s", g_EngineFuncs->GetMapName()));
	EngineFuncs::ConsoleMessage(va("# Waypoints : %d", m_WaypointList.size()));		

	int n=0;
	for(WaypointList::iterator it = m_WaypointList.begin(); it != m_WaypointList.end(); ++it)
	{
		n+= (int) (*it)->m_Connections.size();
	}
	EngineFuncs::ConsoleMessage(va("# Connections : %d", n));
	EngineFuncs::ConsoleMessage(va("# Blockable connections : %d", m_BlockableList.size()));
	EngineFuncs::ConsoleMessage(va("A* Open List : %d", m_OpenCount));
	EngineFuncs::ConsoleMessage(va("A* Closed List : %d", m_ClosedCount));
}

void PathPlannerWaypoint::cmdWaypointSave(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	String strFile = g_EngineFuncs->GetMapName();
	OPTIONAL_STRING_PARAM(ex,1,"");
	strFile += ex;

	if(Save(strFile))
	{
		EngineFuncs::ConsoleMessage("Saved Waypoints.");
		BuildBlockableList();
		BuildSpatialDatabase();
	}
	else
		EngineFuncs::ConsoleError("ERROR Saving Waypoints.");			
}

void PathPlannerWaypoint::cmdWaypointLoad(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	String strFile = g_EngineFuncs->GetMapName();
	OPTIONAL_STRING_PARAM(ex,1,"");
	strFile += ex;

	if(Load(strFile))
	{
		EngineFuncs::ConsoleMessage("Loaded Waypoints.");
	} 
	else
		EngineFuncs::ConsoleError("ERROR Loading Waypoints.");
}

void PathPlannerWaypoint::cmdWaypointSetName(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vPos;
	if(!Utils::GetLocalPosition(vPos)) return;

	Waypoint *pWaypoint = _GetClosestWaypoint(vPos, 0, NOFILTER);
	if(!pWaypoint)
	{
		EngineFuncs::ConsoleError("nearby waypoint not found.");
		return;
	}

	// Append the rest of the tokens into the string
	String newName;
	if(_args.size() > 1)
	{
		for(int i = 1; i < (int)_args.size(); ++i)
		{
			if(!newName.empty())
				newName += " ";
			newName += _args[i];
		}
		SetWaypointName(pWaypoint, newName);
		EngineFuncs::ConsoleMessage(va("Waypoint name set to \"%s\"", newName.c_str()));
	} 
	else
	{
		EngineFuncs::ConsoleMessage("Clearing waypoint name.");
		SetWaypointName(pWaypoint, "");
	}
}

void PathPlannerWaypoint::cmdWaypointAutoRadius(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const char *strUsage[] = 
	{ 
		"waypoint_autoradius all/cur height[#] minradius[#] maxradius[#]",
		"> all or cur: autoradius all waypoints or only nearest",
		"> minradius: minimum radius to use",
		"> maxradius: maximum radius to use",
	};

	float fMinRadius = 5.0f;
	float fMaxRadius = 1000.0f;
	float fTestHeight = 0.0f;

	enum WpMode
	{
		Current_Wp,
		All_Wp,
	};

	WpMode mode = Current_Wp;

	// Parse the command arguments
	switch(_args.size())
	{
	case 5:
		{
			fMaxRadius = (float)atof(_args[4].c_str());
		}
	case 4:
		{
			float fRadius = (float)atof(_args[3].c_str());
			fMinRadius = ClampT<float>(fRadius, fMinRadius, fRadius);
		}
	case 3:
		{
			fTestHeight = (float)atof(_args[2].c_str());
		}
	case 2:
		{
			if(_args[1] == "all")
				mode = All_Wp;
			break;
		}
	default:
		PRINT_USAGE(strUsage);
		return;
	};

	EngineFuncs::ConsoleMessage(va("autoradius: %s height[%f] minradius[%f] maxradius[%f]",
		mode == All_Wp ? "all wps" : "current wp", 
		fTestHeight,
		fMinRadius, 
		fMaxRadius));

	Waypoint *pClosestWp = 0;

	if(mode == Current_Wp)
	{
		Vector3f vLocalPos;
		if(SUCCESS(g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos)))
		{
			pClosestWp = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);
		}
	}

	WaypointList::iterator it, itEnd;
	if(m_SelectedWaypoints.empty())
	{
		it = m_WaypointList.begin();
		itEnd = m_WaypointList.end();
	}
	else
	{
		it = m_SelectedWaypoints.begin();
		itEnd = m_SelectedWaypoints.end();
	}

	for( ; it != itEnd; ++it)
	{
		if(pClosestWp && pClosestWp != (*it))
			continue;

		// First get a point down from this waypoint slightly above the ground.
		Vector3f vStartPosition = (*it)->GetPosition();

		Vector3f vEndPosition = vStartPosition.AddZ(-1000);
		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStartPosition, vEndPosition, NULL, TR_MASK_SOLID, 0, False);	
		if(tr.m_Fraction < 1.0)
		{
			vStartPosition = Vector3f(tr.m_Endpos).AddZ(fTestHeight);
		}

		float fClosestHit = fMaxRadius;
		for(float fAng = 0; fAng < 360; fAng += 30.0f)
		{
			Vector3f ray = Vector3f::UNIT_Y * fClosestHit;
			Vector3f start = vStartPosition;
			Vector3f end = (*it)->GetPosition() + Quaternionf(Vector3f::UNIT_Z, fAng).Rotate(ray);

			obTraceResult tr2;
			EngineFuncs::TraceLine(tr2, start, end, NULL, TR_MASK_SOLID, 0, False);
			if(tr2.m_Fraction < 1.0f)
			{
				float fDistance = (start - end).Length() * tr2.m_Fraction;
				if(fDistance < fClosestHit)
				{
					fClosestHit = fDistance;					
				}
			}
		}

		float fNewRadius = Mathf::Max(fMaxRadius, fClosestHit);
		EngineFuncs::ConsoleMessage(va("#%d Changed Radius from %f to %f", 
			(*it)->GetUID(), 
			(*it)->GetRadius(), 
			fNewRadius));
		(*it)->SetRadius(fNewRadius);
	}
}

void PathPlannerWaypoint::cmdWaypointAutoBuild(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	bool bUseBBox = false, bDcAll = false;

	unsigned int iMaxConnections = std::numeric_limits<unsigned int>::max();
	float fDist = -1.0f, fLimitHeight = Utils::FloatMax;

	// Parse the command arguments
	switch(_args.size())
	{
	case 6:
		iMaxConnections = atoi(_args[5].c_str());
	case 5:
		fDist = (float)atof(_args[4].c_str());
	case 4:
		fLimitHeight = (float)atof(_args[3].c_str());
	case 3:
		bUseBBox = Utils::StringToTrue(_args[2]);
	case 2:
		EngineFuncs::ConsoleMessage("Auto Connecting Waypoints...");
		bDcAll = Utils::StringToTrue(_args[1]);
		if(bDcAll)
		{
			cmdWaypointDisconnectAll(_args);
		}
		break;
	default:
		EngineFuncs::ConsoleError("waypoint_autobuild dc[1/0] bbox[1/0] limitheight[#] limitdist[#] maxconnections[#]");
		return;
	};

	// GAME DEPENDENCY : this bounding box should depend on mod/game.
	// Magik: trying to fit this to real et values
	// TODO check if same values apply for etf, too
	float fMins[3] = {-18, -18, -35};
	float fMaxs[3] = {18, 18, 35};
	AABB bbox(fMins, fMaxs);

	int iNumConnected = 0, iNumRayCasts = 0;
	int size = (int)m_WaypointList.size();
	for(int i = 0; i < size; ++i)
	{
		for(int j = 0; j < size; ++j)
		{
			if(i==j)
				continue;

			// Are we limiting distance?
			if(fDist > 0.0f)
			{
				if((m_WaypointList[i]->GetPosition()-m_WaypointList[j]->GetPosition()).Length() > fDist)
					continue;
			}

			// Are we limiting height?
			if(fLimitHeight > 0)
			{
				float fHeightDiff = fabs((m_WaypointList[i]->GetPosition().z - m_WaypointList[j]->GetPosition().z));

				// If the difference in height is > 32 don't do anything.
				if(fHeightDiff > fLimitHeight)
					continue;
			}

			// traceline from i waypoint to j waypoint
			obTraceResult tr;

			// Use the bounding box if iUseBBox is enabled
			EngineFuncs::TraceLine(tr, 
				(m_WaypointList[i]->GetPosition().AddZ(40)),
				(m_WaypointList[j]->GetPosition().AddZ(40)), 
				(bUseBBox ? &bbox : NULL), (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);
			++iNumRayCasts;

			//Utils::DrawLine(0, (m_WaypointList[i]->GetPosition() + Vector3f(0,0,40)),
			//	tr.m_Endpos, COLOR::GREEN);

			// is it clear?
			if(tr.m_Fraction == 1.0f)
			{
				if(_ConnectWaypoints(m_WaypointList[i], m_WaypointList[j]))
					++iNumConnected;
			}
		}
	}

	// Make sure all the waypoints have have <= iMaxConnections
	for(int i = 0; i < size; ++i)
	{
		while(m_WaypointList[i]->m_Connections.size() > iMaxConnections)
		{
			Waypoint::ConnectionList::iterator longestIter = m_WaypointList[i]->m_Connections.end();
			float fLongestDist = 0;

			Waypoint::ConnectionList::iterator it = m_WaypointList[i]->m_Connections.begin();
			while(it != m_WaypointList[i]->m_Connections.end())
			{
				float fCurrentDist = (m_WaypointList[i]->GetPosition() - (*it).m_Connection->GetPosition()).Length();
				if(fCurrentDist > fLongestDist)
				{
					longestIter = it;
					fLongestDist = fCurrentDist;                    
				}
				++it;
			}

			// Did we find one longer than we wanted?
			if(longestIter != m_WaypointList[i]->m_Connections.end())
			{
				m_WaypointList[i]->m_Connections.erase(longestIter);
			}
		}
	}

	EngineFuncs::ConsoleMessage(va("Generated %d Paths, %d ray casts", iNumConnected, iNumRayCasts));

	BuildBlockableList();
}

void PathPlannerWaypoint::cmdWaypointDisconnectAll(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	EngineFuncs::ConsoleMessage("Disconnecting ALL Waypoints...");
	int size = (int)m_WaypointList.size();
	for(int i = 0; i < size; ++i)
	{
		m_WaypointList[i]->m_Connections.clear();
	}

	BuildBlockableList();
	BuildSpatialDatabase();
}

void PathPlannerWaypoint::cmdWaypointView(const StringVector &_args)
{
	if(_args.size() >= 2)
	{
		if(Utils::StringToTrue(_args[1]) || (_args[1] == "toggle" && !m_PlannerFlags.CheckFlag(NAV_VIEW)))
		{
			m_PlannerFlags.SetFlag(NAV_VIEW);
			const char * msg = IGameManager::GetInstance()->GetGame()->IsDebugDrawSupported();
			if(msg){
				EngineFuncs::ConsoleError(msg);
				return;
			}
		}
		else if(Utils::StringToFalse(_args[1]) || _args[1] == "toggle")
		{
			m_PlannerFlags.ClearFlag(NAV_VIEW);

			if(g_ClientFuncs) g_ClientFuncs->ClearView();
		}
	}
	EngineFuncs::ConsoleMessage(va("Waypoint Visible %s.", m_PlannerFlags.CheckFlag(NAV_VIEW) ? "on" : "off"));
}

void PathPlannerWaypoint::cmdWaypointAutoFlag(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_AUTODETECTFLAGS) && (_args.size() < 2 || Utils::StringToTrue(_args[1])))
	{
		m_PlannerFlags.SetFlag(NAV_AUTODETECTFLAGS);
	}
	else if(m_PlannerFlags.CheckFlag(NAV_AUTODETECTFLAGS) && (_args.size() < 2 || Utils::StringToFalse(_args[1])))
	{
		m_PlannerFlags.ClearFlag(NAV_AUTODETECTFLAGS);
	}

	EngineFuncs::ConsoleMessage(va("Waypoint Autoflag %s",
		m_PlannerFlags.CheckFlag(NAV_AUTODETECTFLAGS) ? "on." : "off."));
}

void PathPlannerWaypoint::cmdWaypointViewFacing(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(WAYPOINT_VIEW_FACING) && (_args.size() < 2 || Utils::StringToTrue(_args[1])))
	{
		m_PlannerFlags.SetFlag(WAYPOINT_VIEW_FACING);
	}
	else if(m_PlannerFlags.CheckFlag(WAYPOINT_VIEW_FACING) && (_args.size() < 2 || Utils::StringToFalse(_args[1])))
	{
		m_PlannerFlags.ClearFlag(WAYPOINT_VIEW_FACING);
	}

	EngineFuncs::ConsoleMessage(va("Waypoint Facing Visible %s",
		m_PlannerFlags.CheckFlag(WAYPOINT_VIEW_FACING) ? "on." : "off."));
}

void PathPlannerWaypoint::cmdWaypointSetProperty(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	String propertyName;
	String propertyValue;

	if(_args.size() < 3)
	{
		EngineFuncs::ConsoleError("waypoint_setproperty name value");
		return;
	}

	if(_args[2].empty())
	{
		cmdWaypointClearProperty(_args);
		return;
	}

	Vector3f vLocalPos;
	if(SUCCESS(g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos)))
	{
		Waypoint *pClosest = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);
		if(pClosest)
		{
			propertyName = _args[1];
			propertyValue = _args[2];

			std::transform(propertyName.begin(), propertyName.end(), propertyName.begin(), toLower());
			std::transform(propertyValue.begin(), propertyValue.end(), propertyValue.begin(), toLower());

			//////////////////////////////////////////////////////////////////////////
			Vector3f v;
			if(propertyValue=="<facing>" && Utils::GetLocalFacing(v))
				Utils::ConvertString(v,propertyValue);
			if(propertyValue=="<position>" && Utils::GetLocalPosition(v))
				Utils::ConvertString(v,propertyValue);
			if(propertyValue=="<aimpoint>" && Utils::GetLocalAimPoint(v))
				Utils::ConvertString(v,propertyValue);
			if(propertyValue=="<wpposition>" && Utils::GetLocalAimPoint(v))
			{
				v.z -= g_fBottomWaypointOffset;
				Utils::ConvertString(v,propertyValue);
			}
			//////////////////////////////////////////////////////////////////////////

			if(pClosest->GetPropertyMap().AddProperty(propertyName, propertyValue))
			{
				if(propertyName == "paththrough") pClosest->PostLoad();

				EngineFuncs::ConsoleMessage(va("property set: %s, %s", 
					propertyName.c_str(), propertyValue.c_str()));
			}
			return;
		}
	}
	EngineFuncs::ConsoleError("error getting waypoint or client position");
}

void PathPlannerWaypoint::cmdWaypointShowProperty(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	/*String propertyName;
	if(_args.size() >= 2)
		propertyName = _args[1];*/

	Vector3f vLocalPos;
	if(Utils::GetLocalPosition(vLocalPos))
	{
		Waypoint *pClosest = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);
		if(pClosest)
		{
			const PropertyMap::ValueMap &pm = pClosest->GetPropertyMap().GetProperties();
			PropertyMap::ValueMap::const_iterator cIt = pm.begin();
			for(; cIt != pm.end(); ++cIt)
			{
				EngineFuncs::ConsoleMessage(va("property: %s = %s", 
					(*cIt).first.c_str(), (*cIt).second.c_str()));
			}
			return;
		}
	}
	EngineFuncs::ConsoleError("error getting waypoint or client position");
}

void PathPlannerWaypoint::cmdWaypointClearProperty(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	String propertyName;

	if(_args.size() < 2)
	{
		EngineFuncs::ConsoleError("waypoint_clearproperty name");
		return;
	}

	Vector3f vLocalPos;
	if(SUCCESS(g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos)))
	{
		Waypoint *pClosest = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);
		if(pClosest)
		{
			propertyName = _args[1];
			std::transform(propertyName.begin(), propertyName.end(), propertyName.begin(), toLower());
			pClosest->GetPropertyMap().DelProperty(propertyName);

			if(propertyName == "paththrough") pClosest->PostLoad();
			return;
		}
	}
	EngineFuncs::ConsoleError("error getting waypoint or client position");
}

void PathPlannerWaypoint::cmdWaypointSetDefaultRadius(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() >= 2)
	{
		float fWaypointRadius = (float)atof(_args[1].c_str());
		if(fWaypointRadius > 0.0f)
		{
			m_DefaultWaypointRadius = fWaypointRadius;
		}
	}
}

void PathPlannerWaypoint::cmdWaypointSetRadius(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() >= 2)
	{
		Vector3f vLocalPos;
		g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos);

		float fWaypointRadius = m_DefaultWaypointRadius;
		if(!Utils::ConvertString(_args[1], fWaypointRadius))
			if(!Utils::StringCompareNoCase(_args[1].c_str(), "default"))
				fWaypointRadius = m_DefaultWaypointRadius;
		if(fWaypointRadius > 0.0f)
		{
			if(m_SelectedWaypoints.empty())
			{
				// Look for the closest waypoint.
				Waypoint *pClosest = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);

				// Set the new radius for this waypoint
				if(pClosest)
				{
					pClosest->m_Radius = fWaypointRadius;
					EngineFuncs::ConsoleMessage(va("Waypoint %d radius changed to %f", 
						pClosest->GetUID(), fWaypointRadius));
				}
			}
			else
			{
				for(obuint32 i = 0; i < m_SelectedWaypoints.size(); ++i)
				{
					m_SelectedWaypoints[i]->m_Radius = fWaypointRadius;
					EngineFuncs::ConsoleMessage(va("Waypoint %d radius changed to %f", 
						m_SelectedWaypoints[i]->GetUID(), fWaypointRadius));
				}
			}
			m_SelectedWaypoint = -1;
		}
	}
}

void PathPlannerWaypoint::cmdWaypointChangeRadius(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	float fChangeBy = 1.0f;

	if(_args.size() >= 2)
	{
		fChangeBy = (float)atof(_args[1].c_str());
		if(fChangeBy == 0.0f)
			fChangeBy = 1.0f;
	}

	if(m_SelectedWaypoints.empty())
	{
		Vector3f vLocalPos;
		g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos);

		// Look for the closest waypoint.
		Waypoint *pClosest = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);

		// Set the new radius for this waypoint
		if(pClosest)
		{
			pClosest->m_Radius += fChangeBy;
			EngineFuncs::ConsoleMessage(va("Waypoint %d radius changed to %f", 
				pClosest->GetUID(), pClosest->m_Radius));
		}	
	}
	else
	{
		for(obuint32 i = 0; i < m_SelectedWaypoints.size(); ++i)
		{
			m_SelectedWaypoints[i]->m_Radius += fChangeBy;
			EngineFuncs::ConsoleMessage(va("Waypoint %d radius changed to %f", 
				m_SelectedWaypoints[i]->GetUID(), m_SelectedWaypoints[i]->m_Radius));
		}
	}
	m_SelectedWaypoint = -1;
}
void PathPlannerWaypoint::cmdWaypointSetFacing(const StringVector &_args)
{	
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vFacing;
	Vector3f vLocalPos;

	if(SUCCESS(g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos)) && 
		SUCCESS(g_EngineFuncs->GetEntityOrientation(Utils::GetLocalEntity(), vFacing, 0, 0)))
	{
		if(m_SelectedWaypoints.empty())
		{
			// Look for the closest waypoint.
			Waypoint *pClosest = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);

			if(pClosest)
			{
				pClosest->m_Facing = vFacing;
				EngineFuncs::ConsoleMessage(va("Waypoint %d facing changed to Vector3(%f,%f,%f)", 
					pClosest->GetUID(), vFacing[0], vFacing[1], vFacing[2]));
			}
		}
		else
		{
			for(obuint32 i = 0; i < m_SelectedWaypoints.size(); ++i)
			{
				m_SelectedWaypoints[i]->m_Facing += vFacing;
				EngineFuncs::ConsoleMessage(va("Waypoint %d facing changed to Vector3(%f,%f,%f)", 
					m_SelectedWaypoints[i]->GetUID(), vFacing[0], vFacing[1], vFacing[2]));
			}
		}
	}
}

void PathPlannerWaypoint::cmdWaypointInfo(const StringVector &_args)
{	
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vLocalPos;
	g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos);

	// Look for the closest waypoint.
	Waypoint *pClosest = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);

	// Print the info for this waypoint.
	if(pClosest)
	{
		String flagString = va("Waypoint #%d, radius %f, #connections %d\n", 
			pClosest->GetUID(), pClosest->m_Radius, (unsigned int)pClosest->m_Connections.size()).c_str();

		// Build a string with the flags
		FlagMap::const_iterator flagIt = m_WaypointFlags.begin();
		for( ; flagIt != m_WaypointFlags.end(); ++flagIt)
		{
			if(pClosest->IsFlagOn(flagIt->second))
			{
				flagString += flagIt->first;
				flagString += " ";
			}
		}
		if(!flagString.empty())
		{
			Utils::PrintText(pClosest->GetPosition(),COLOR::WHITE,2.f,flagString.c_str());
		}
	}
}

void PathPlannerWaypoint::cmdWaypointGoto(const StringVector &_args)
{	
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() != 2)
	{
		EngineFuncs::ConsoleError("Invalid Waypoint specified");
		return;
	}

	obuint32 uid;
	
	Utils::ConvertString(_args[1], uid);

	Waypoint *pTravelTo = GetWaypointByGUID(uid);

	String m_Message;

	if(!pTravelTo)
	{
		pTravelTo = GetWaypointByName(_args[1]);
		if(pTravelTo)
		{
			m_Message = ": " + pTravelTo->GetName();
		}
	}
	else
	{
		m_Message = va(" UID: %i", pTravelTo->GetUID());
	}

	if(pTravelTo)
	{
		if(!InterfaceFuncs::GotoWaypoint(m_Message.c_str(), pTravelTo->GetPosition()))
		{
			EngineFuncs::ConsoleError(va("Failed to Teleport to Waypoint %s", _args[1].c_str()));
			return;
		}
	}
	else
	{
		EngineFuncs::ConsoleError("Invalid Waypoint specified");
		return;
	}
}

void PathPlannerWaypoint::cmdWaypointMove(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vLocalPos;
	g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vLocalPos);

	if(m_MovingWaypointIndex == -1)
	{
		Waypoint *pWp = _GetClosestWaypoint(vLocalPos, 0, NOFILTER, &m_MovingWaypointIndex);
		if( pWp )
			EngineFuncs::ConsoleMessage(va("Moving waypoint : %d", pWp->GetUID()));
		else
			EngineFuncs::ConsoleMessage("waypoint_move: no waypoint found");
	}
	else
	{
		if(m_MovingWaypointIndex < (int)m_WaypointList.size() && m_MovingWaypointIndex >= 0)
		{
			EngineFuncs::ConsoleMessage(va("Placed waypoint : %d", m_WaypointList[m_MovingWaypointIndex]->GetUID()));
			m_WaypointList[m_MovingWaypointIndex]->SetPosition( vLocalPos );
			m_MovingWaypointIndex = -1;
		}
	}
}

void PathPlannerWaypoint::cmdWaypointTranslate(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() != 4)
	{
		EngineFuncs::ConsoleError("translation not specified, provide an x y z");
		return;
	}

	double dX, dY, dZ;
	if(Utils::ConvertString(_args[1], dX) && 
		Utils::ConvertString(_args[2], dY) &&
		Utils::ConvertString(_args[3], dZ))
	{
		int iNum = 0;
		WaypointList::iterator it, itEnd;

		if(m_SelectedWaypoints.empty())
		{
			iNum = (int)m_WaypointList.size();
			it = m_WaypointList.begin();
			itEnd = m_WaypointList.end();
		}
		else
		{
			iNum = (int)m_SelectedWaypoints.size();
			it = m_SelectedWaypoints.begin();
			itEnd = m_SelectedWaypoints.end();
		}
		
		for( ; it != itEnd; ++it)
		{
			Waypoint *pWp = (*it);
			if(!pWp->m_Locked)
				pWp->m_Position += Vector3f((float)dX, (float)dY, (float)dZ);
		}
		EngineFuncs::ConsoleMessage(va("translated %d waypoints by (%.2f, %.2f, %.2f)",
			iNum, dX, dY, dZ));
	}
	else
	{
		EngineFuncs::ConsoleError("invalid translation specified, provide an x y z");
	}
}

void PathPlannerWaypoint::cmdWaypointMirror(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() < 2)
	{
		EngineFuncs::ConsoleError("axis not specified, valid axis are x,y,z (rotation) or m (mirror)");
		return;
	}

	bool bUsePlayer = false;
	bool bAxis[4] = { false, false, false, false };
	if(_args[1].find('x') != std::string::npos)
		bAxis[0] = true;
	else if(_args[1].find('y') != std::string::npos)
		bAxis[1] = true;
	else if(_args[1].find('z') != std::string::npos)
		bAxis[2] = true;
	if(_args[1].find('m') != std::string::npos)
		bAxis[3] = true;
	if(_args.size() == 3 && _args[2].find('p') != std::string::npos)
		bUsePlayer = true;

	Vector3f vPlayerPos = Vector3f::ZERO;
	if(bUsePlayer)
		g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vPlayerPos);

	if(!bAxis[0] && !bAxis[1] && !bAxis[2] && !bAxis[3])
	{
		EngineFuncs::ConsoleError("invalid axis specified.");
		return;
	}

	EngineFuncs::ConsoleMessage(va("mirroring waypoints around %s", _args[1].c_str()));
	
	WaypointList mirroredWaypoints;

	WaypointList *waypointList = &m_WaypointList;
	if(!m_SelectedWaypoints.empty()) waypointList = &m_SelectedWaypoints;
	mirroredWaypoints.reserve(waypointList->size());

	WaypointList::iterator it, itEnd;
	itEnd = waypointList->end();
	for(it = waypointList->begin(); it != itEnd; ++it)
	{
		Waypoint *pWp = new Waypoint;
		*pWp = **it;
		pWp->AssignNewUID();

		if(bUsePlayer)
			pWp->m_Position -= vPlayerPos;

		if(pWp->IsFlagOn(F_NAV_TEAM1))
		{
			pWp->RemoveFlag(F_NAV_TEAM1);
			pWp->AddFlag(F_NAV_TEAM2);
		}
		else if(pWp->IsFlagOn(F_NAV_TEAM2))
		{
			pWp->RemoveFlag(F_NAV_TEAM2);
			pWp->AddFlag(F_NAV_TEAM1);
		}

		if(!pWp->GetName().empty())
		{
			pWp->m_WaypointName += "_mir";
		}

		// Rotate the waypoint.
		for(int i = 0; i < 3; ++i)
		{
			if(bAxis[i])
			{
				Vector3f vAxis;
				switch(i)
				{
				case 0:
					vAxis = Vector3f::UNIT_X;
					break;
				case 1:
					vAxis = Vector3f::UNIT_Y;
					break;
				case 2:
					vAxis = Vector3f::UNIT_Z;
					break;
				}
				Matrix3f mat(vAxis, Mathf::DegToRad(180.0f));
				pWp->m_Position = mat * pWp->m_Position;
				pWp->m_Facing = mat * pWp->m_Facing;
			}
		}
		// Mirror the waypoint.
		if(bAxis[3])
		{
			pWp->m_Position.y = -pWp->m_Position.y;
			pWp->m_Facing.y = -pWp->m_Facing.y;
		}
		if(bUsePlayer)
			pWp->m_Position += vPlayerPos;

		mirroredWaypoints.push_back(pWp);
	}

	// Update the connection pointers.
	{
		it = mirroredWaypoints.begin(), itEnd = mirroredWaypoints.end();
		for( ; it != itEnd; ++it)
		{
			Waypoint::ConnectionList::iterator conIt = (*it)->m_Connections.begin(), 
				conItEnd = (*it)->m_Connections.end();

			while(conIt != conItEnd)
			{
				// which waypoint does this point to in the main list.
				for(int i = 0; ; ++i)
				{
					if(i >= (int)waypointList->size())
					{
						//remove connection which is outside mirrored list
						Waypoint::ConnectionList::iterator conErase = conIt;
						++conIt;
						(*it)->m_Connections.erase(conErase);
						break;
					}
					if((*waypointList)[i] == conIt->m_Connection)
					{
						conIt->m_Connection = mirroredWaypoints[i];
						++conIt;
						break;
					}
				}
			}
		}
	}

	// Append the new waypoints to the old.
	m_WaypointList.insert(m_WaypointList.end(), mirroredWaypoints.begin(), mirroredWaypoints.end());
}

//////////////////////////////////////////////////////////////////////////
class DisconnectWaypoint
{
public:
	// Constructor initializes the value to multiply by
	DisconnectWaypoint(const Waypoint *_wp) : 
		m_Target(_wp) 
	{
	}

	// The function call for the element to be multiplied
	void operator()(Waypoint *_wp)
	{
		_wp->DisconnectFrom(m_Target);
	}
private:
	const Waypoint *m_Target;
};
//////////////////////////////////////////////////////////////////////////

void PathPlannerWaypoint::cmdWaypointDeleteAxis(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"waypoint_deleteaxis axis"
		"> axis: x, y, z, -x, -y, -z",
	};

	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() != 2)
	{
		PRINT_USAGE(strUsage);
		return;
	}

	Vector3f vAxis = Vector3f::ZERO;
	if(_args[1] == "x")
		vAxis = Vector3f::UNIT_X;
	else if(_args[1] == "y")
		vAxis = Vector3f::UNIT_Y;
	else if(_args[1] == "z")
		vAxis = Vector3f::UNIT_Z;
	else if(_args[1] == "-x")
		vAxis = -Vector3f::UNIT_X;
	else if(_args[1] == "-y")
		vAxis = -Vector3f::UNIT_Y;
	else if(_args[1] == "-z")
		vAxis = -Vector3f::UNIT_Z;

	if(vAxis == Vector3f::ZERO)
	{
		EngineFuncs::ConsoleError("invalid axis specified.");
		return;
	}

	int iDeleted = 0;
	WaypointList::iterator it = m_WaypointList.begin();
	for(; it != m_WaypointList.end(); )
	{
		bool bRemoveMe = false;
		if(vAxis.x != 0.0f)
		{
			bRemoveMe = Mathf::Sign((*it)->GetPosition().x) != Mathf::Sign(vAxis.x);
		} 
		else if(vAxis.y != 0.0f)
		{
			bRemoveMe = Mathf::Sign((*it)->GetPosition().y) != Mathf::Sign(vAxis.y);
		}
		else if(vAxis.z != 0.0f)
		{
			bRemoveMe = Mathf::Sign((*it)->GetPosition().z) != Mathf::Sign(vAxis.z);
		}

		if(bRemoveMe)
		{
			std::for_each(m_WaypointList.begin() , m_WaypointList.end() , DisconnectWaypoint(*it) );
			it = m_WaypointList.erase(it);
			++iDeleted;
		}
		else
			++it;
	}
	
	EngineFuncs::ConsoleMessage(va("deleted %d waypoints around axis: %s",iDeleted,_args[1].c_str()));
}

void PathPlannerWaypoint::cmdWaypointConnect(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	// Get the local characters position
	Vector3f myPos;
	g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), myPos);

	// Get the closest waypoint
	Waypoint *pClosestWp = _GetClosestWaypoint(myPos, 0, NOFILTER);
	if(pClosestWp != NULL && (pClosestWp->GetPosition()-myPos).Length() < 100.f)
		cmdWaypointConnect_Helper(_args, pClosestWp);
}

void PathPlannerWaypoint::cmdWaypointConnectX(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW) || (m_SelectedWaypoint == -1))
		return;

	// Get the selected waypoint
	Waypoint *pClosestWp = m_WaypointList[m_SelectedWaypoint];
	cmdWaypointConnect_Helper(_args, pClosestWp);
}

void PathPlannerWaypoint::cmdWaypointConnect_Helper(const StringVector &_args, Waypoint *_waypoint)
{
	// Make sure we're close enough before we do anything
	if(_waypoint)
	{
		if(!m_ConnectWp)
		{
			m_ConnectWp = _waypoint;
			EngineFuncs::ConsoleMessage(va("Waypoint Selected: %d", m_ConnectWp->GetUID()));
		}
		else
		{
			// See if the first waypoint is already connected to this one.
			if(_DisConnectWaypoints(m_ConnectWp, _waypoint))
			{
				EngineFuncs::ConsoleMessage(va("Waypoint Disconnected: %d-%d", 
					m_ConnectWp->GetUID(), _waypoint->GetUID()));
			}
			else if(_ConnectWaypoints(m_ConnectWp, _waypoint))
			{
				EngineFuncs::ConsoleMessage(va("Waypoint Connected: %d-%d", 
					m_ConnectWp->GetUID(), _waypoint->GetUID()));
			}

			// Clear the first selection.
			m_ConnectWp = 0;
		}
	}
}

void PathPlannerWaypoint::cmdWaypointConnect2Way(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f myPos;
	g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), myPos);
	Waypoint *pClosestWp = _GetClosestWaypoint(myPos, 0, NOFILTER);
	if(pClosestWp != NULL && (pClosestWp->GetPosition()-myPos).Length() < 100.f)
		cmdWaypointConnect2Way_Helper(_args, pClosestWp);
}

void PathPlannerWaypoint::cmdWaypointConnect2WayX(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW) || (m_SelectedWaypoint == -1))
		return;

	Waypoint *pClosestWp = m_WaypointList[m_SelectedWaypoint];
	cmdWaypointConnect2Way_Helper(_args, pClosestWp);
}

void PathPlannerWaypoint::cmdWaypointConnect2Way_Helper(const StringVector &_args, Waypoint *_waypoint)
{
	// Make sure we're close enough before we do anything
	if(_waypoint)
	{
		if(!m_ConnectWp)
		{
			m_ConnectWp = _waypoint;
			EngineFuncs::ConsoleMessage(va("Waypoint Selected: %d", m_ConnectWp->GetUID()));
		}
		else
		{
			if(_DisConnectWaypoints(m_ConnectWp, _waypoint))
			{
				EngineFuncs::ConsoleMessage(va("Waypoint Disconnected: %d-%d", 
					m_ConnectWp->GetUID(), _waypoint->GetUID()));
			}
			else if(_ConnectWaypoints(m_ConnectWp, _waypoint))
			{
				EngineFuncs::ConsoleMessage(va("Waypoint Connected: %d-%d", 
					m_ConnectWp->GetUID(), _waypoint->GetUID()));
			}

			if(_DisConnectWaypoints(_waypoint, m_ConnectWp))
			{
				EngineFuncs::ConsoleMessage(va("Waypoint Disconnected: %d-%d", 
					_waypoint->GetUID(), m_ConnectWp->GetUID()));
			}
			else if(_ConnectWaypoints(_waypoint, m_ConnectWp))
			{
				EngineFuncs::ConsoleMessage(va("Waypoint Connected: %d-%d", 
					_waypoint->GetUID(), m_ConnectWp->GetUID()));
			}

			// Clear the first selection.
			m_ConnectWp = 0;
		}
	}
}

void PathPlannerWaypoint::_BenchmarkPathFinder(const StringVector &_args)
{
	EngineFuncs::ConsoleMessage("-= Waypoint PathFind Benchmark =-");

	double dTimeTaken = 0.0f;
	obint32 iNumWaypoints = (obint32)m_WaypointList.size();
	obint32 iNumPaths = iNumWaypoints * iNumWaypoints;
	Timer tme;

	tme.Reset();
	for(obint32 w1 = 0; w1 < iNumWaypoints; ++w1)
	{
		for(obint32 w2 = 0; w2 < iNumWaypoints; ++w2)
		{
			PlanPathToGoal(NULL, m_WaypointList[w1]->GetPosition(), m_WaypointList[w2]->GetPosition(), 0);
		}
	}
	dTimeTaken = tme.GetElapsedSeconds();

	EngineFuncs::ConsoleMessage(va("generated %d paths in %f seconds: %f paths/sec", 
		iNumPaths, dTimeTaken, dTimeTaken != 0.0f ? (float)iNumPaths / dTimeTaken : 0.0f));
}

void PathPlannerWaypoint::_BenchmarkGetNavPoint(const StringVector &_args)
{
	obuint32 iNumIterations = 1;
	if(_args.size() > 1)
	{
		iNumIterations = atoi(_args[1].c_str());
		if(iNumIterations <= 0)
			iNumIterations = 1;
	}

	EngineFuncs::ConsoleMessage("-= Waypoint GetNavPoint Benchmark =-");

	double dTimeTaken = 0.0f;
	obuint32 iNumWaypoints = (obuint32)m_WaypointList.size();
	Timer tme;

	obuint32 iHits = 0, iMisses = 0;
	tme.Reset();
	for(obuint32 i = 0; i < iNumIterations; ++i)
	{
		for(obuint32 w1 = 0; w1 < iNumWaypoints; ++w1)
		{
			Waypoint *pWaypoint = m_WaypointList[w1];

			Waypoint *pClosest = _GetClosestWaypoint(pWaypoint->GetPosition(), (NavFlags)0, NOFILTER);
			if(pClosest)
				++iHits;
			else
				++iMisses;
		}
	}
	
	dTimeTaken = tme.GetElapsedSeconds();

	EngineFuncs::ConsoleMessage(va("_GetClosest() %d calls, %d hits, %d misses : avg %f per second", 
		iNumWaypoints * iNumIterations, 
		iHits, 
		iMisses, 
		dTimeTaken != 0.0f ? ((float)(iNumWaypoints * iNumIterations) / dTimeTaken) : 0.0f));
}

void PathPlannerWaypoint::cmdWaypointAddFlag(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(m_SelectedWaypoints.empty())
	{
		Vector3f myPos;
		g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), myPos);
		Waypoint *pClosestWp = _GetClosestWaypoint(myPos, 0, NOFILTER);
		if(pClosestWp != NULL && (pClosestWp->GetPosition()-myPos).Length() < 100.f)
			cmdWaypointAddFlag_Helper(_args, pClosestWp);
	}
	else
	{
		for(obuint32 i = 0; i < m_SelectedWaypoints.size(); ++i)
			cmdWaypointAddFlag_Helper(_args, m_SelectedWaypoints[i]);
	}
}

void PathPlannerWaypoint::cmdWaypointAddFlagX(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW) || (m_SelectedWaypoint == -1))
		return;

	if(m_SelectedWaypoints.empty())
	{
		// Get the selected waypoint
		Waypoint *pWaypoint = m_WaypointList[m_SelectedWaypoint];

		// Make sure we're close enough before we do anything
		cmdWaypointAddFlag_Helper(_args, pWaypoint);
	}
	else
	{
		for(obuint32 i = 0; i < m_SelectedWaypoints.size(); ++i)
			cmdWaypointAddFlag_Helper(_args, m_SelectedWaypoints[i]);
	}
}

void PathPlannerWaypoint::cmdWaypointAddFlag_Helper(const StringVector &_args, Waypoint *_waypoint)
{
	// Make sure we're close enough before we do anything
	if(!_waypoint)
		return;

	NavFlags deprecatedFlags = IGameManager::GetInstance()->GetGame()->DeprecatedNavigationFlags();
	bool bPrintFlagList = true;

	// Add the flags to the waypoint.
	if(_args.size() >= 2)
	{
		// to support adding multiple flags in a single line, lets loop through all the tokens
		StringVector::const_iterator token = _args.begin();
		for(++token; token != _args.end(); ++token)
		{
			// Look for this token in the map.
			FlagMap::const_iterator it = m_WaypointFlags.find(*token);
			if(it != m_WaypointFlags.end() && (!(it->second & deprecatedFlags) || _waypoint->IsFlagOn(it->second)))
			{
				// Get the local characters position
				if(!_waypoint->IsFlagOn(it->second))
				{
					_waypoint->AddFlag(it->second);
					EngineFuncs::ConsoleMessage(va("%s Flag added to waypoint.", token->c_str()));
				} 
				else
				{
					_waypoint->RemoveFlag(it->second);
					EngineFuncs::ConsoleMessage(va("%s Flag removed from waypoint.", token->c_str()));

					//open connections if blockable flag is removed
					if ((it->second & m_BlockableMask) != 0) ClearBlockable(_waypoint);
				}

				// Team flags have a somewhat special case.
				// If no team flags are enable, make sure the teamonly flag is disable as well.
				if(!_waypoint->IsAnyFlagOn(F_NAV_TEAM_ALL))
				{
					if(_waypoint->IsFlagOn(F_NAV_TEAMONLY))
					{
						_waypoint->RemoveFlag(F_NAV_TEAMONLY);
						EngineFuncs::ConsoleMessage("Waypoint no longer team specific.");
					}
				} else
				{
					// At least one of them is on, so make sure the teamonly flag is set.
					_waypoint->AddFlag(F_NAV_TEAMONLY);
				}

				BuildBlockableList();
				BuildSpatialDatabase();
				bPrintFlagList = false;
			} 
			else
			{
				EngineFuncs::ConsoleError(va("Invalid flag: %s.", token->c_str()));
			}
		}		
	} else
	{
		EngineFuncs::ConsoleError("No Flags specified.");		
	}

	// Print out the available flags.
	if(bPrintFlagList && !m_WaypointFlags.empty())
	{
		EngineFuncs::ConsoleMessage("Waypoint Flag List.");
		FlagMap::const_iterator it = m_WaypointFlags.begin();
		for( ; it != m_WaypointFlags.end(); ++it)
			if(!(it->second & deprecatedFlags))
				EngineFuncs::ConsoleMessage(va("%s", it->first.c_str()));
	}
}

void PathPlannerWaypoint::cmdWaypointClearConnections(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(m_SelectedWaypoints.empty())
	{
		Vector3f myPos;
		g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), myPos);
		Waypoint *pWaypoint = _GetClosestWaypoint(myPos, 0, NOFILTER);

		if(pWaypoint)
		{
			pWaypoint->m_Connections.clear();
			EngineFuncs::ConsoleMessage(va("Waypoint %d Connections Cleared.",
				pWaypoint->GetUID()));
		}
	}
	else
	{
		for(obuint32 i = 0; i < m_SelectedWaypoints.size(); ++i)
		{
			m_SelectedWaypoints[i]->m_Connections.clear();
			EngineFuncs::ConsoleMessage(va("Waypoint %d Connections Cleared.",
				m_SelectedWaypoints[i]->GetUID()));
		}
	}

	BuildBlockableList();
}

void PathPlannerWaypoint::cmdWaypointClearAllFlags(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() >= 2)
	{
		for(unsigned int iToken = 1; iToken < _args.size(); ++iToken)
		{
			// Look for this token in the map.
			FlagMap::const_iterator flagIt = m_WaypointFlags.find(_args[iToken]);
			if(flagIt != m_WaypointFlags.end())
			{
				WaypointList::iterator it;
				WaypointList::iterator itEnd;

				if(m_SelectedWaypoints.empty())
				{
					it = m_WaypointList.begin();
					itEnd = m_WaypointList.end();
				}
				else
				{
					it = m_SelectedWaypoints.begin();
					itEnd = m_SelectedWaypoints.end();
				}

				int iNumWps = 0;
				for( ; it != itEnd; ++it)
				{
					if((*it)->IsFlagOn(flagIt->second))
					{
						++iNumWps;
						(*it)->RemoveFlag(flagIt->second);
					}
				}
				EngineFuncs::ConsoleMessage(va("Removed flag %s from %d waypoints.", 
					_args[iToken].c_str(), iNumWps));
			}
		}		
	}
	else
	{
		WaypointList::iterator it;
		WaypointList::iterator itEnd;

		if(m_SelectedWaypoints.empty())
		{
			it = m_WaypointList.begin();
			itEnd = m_WaypointList.end();
		}
		else
		{
			it = m_SelectedWaypoints.begin();
			itEnd = m_SelectedWaypoints.end();
		}

		int iNum = 0;
		for( ; it != itEnd; ++it)
		{
			++iNum;
			(*it)->ClearFlags();
		}
		EngineFuncs::ConsoleMessage(va("Cleared all flags from %d waypoints.", iNum));
	}	
}

bool _NameLT(const Waypoint *_pt1, const Waypoint *_pt2)
{
	return Utils::StringToLower(_pt1->GetName()) < Utils::StringToLower(_pt2->GetName());
}

void PathPlannerWaypoint::cmdWaypointGetWpNames(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	String exp = ".*";
	if(_args.size() < 1)
		exp = _args[0];

	WaypointList wl;

	for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
	{
		Waypoint *pWp = m_WaypointList[i];
		if(!pWp->GetName().empty() && !Utils::RegexMatch( exp.c_str(), pWp->GetName().c_str() ) )
			continue;
		wl.push_back(pWp);
	}

	std::sort(wl.begin(), wl.end(), _NameLT);

	for(obuint32 i = 0; i < wl.size(); ++i)
	{
		EngineFuncs::ConsoleMessage(va("%s : uid # %d: ", wl[i]->GetName().c_str(), wl[i]->GetUID()));
	}
}

//////////////////////////////////////////////////////////////////////////

extern obColor		g_WaypointColor;
extern obColor		g_LinkClosedColor;
extern obColor		g_LinkTeleport;
extern obColor		g_LinkColor1Way;
extern obColor		g_LinkColor2Way;
extern obColor		g_BlockableBlocked;
extern obColor		g_BlockableOpen;
extern obColor		g_AimEntity;
extern obColor		g_RadiusIndicator;
extern obColor		g_SelectedWaypoint;
extern obColor		g_ShowFacingColor;

extern obColor		g_Team1;
extern obColor		g_Team2;
extern obColor		g_Team3;
extern obColor		g_Team4;

//////////////////////////////////////////////////////////////////////////

void PathPlannerWaypoint::cmdWaypointColor(const StringVector &_args)
{
	/*if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;*/

	const char *strUsage[] = 
	{ 
		"waypoint_color type[string] red[#] green[#] blue[#]",
		"> type: one of the following",
		"    waypoint_color",
		"    waypoint_selected",
		"    link_closedcolor",
		"    link_teleport",
		"    link_1way",
		"    link_2way",
		"    blockable_blocked",
		"    blockable_open",
		"    aimentity",
		"    radius",
		"    team1",
		"    team2",
		"    team3",
		"    team4",
	};

	CHECK_NUM_PARAMS(_args, 5, strUsage);
	const String strType = _args[1];
	CHECK_INT_PARAM(iRed, 2, strUsage);
	CHECK_INT_PARAM(iGreen, 3, strUsage);
	CHECK_INT_PARAM(iBlue, 4, strUsage);
	OPTIONAL_INT_PARAM(iAlpha, 5, 255);

	obColor newColor(
		(obuint8)ClampT(iRed, 0, 255), 
		(obuint8)ClampT(iGreen, 0, 255), 
		(obuint8)ClampT(iBlue, 0, 255),
		(obuint8)ClampT(iAlpha, 0, 255));

	if(strType == "waypoint_color")
		g_WaypointColor = newColor;
	else if(strType == "waypoint_selected")
		g_SelectedWaypoint = newColor;
	else if(strType == "link_closedcolor")
		g_LinkClosedColor = newColor;
	else if(strType == "link_teleport")
		g_LinkTeleport = newColor;
	else if(strType == "link_1way")
		g_LinkColor1Way = newColor;
	else if(strType == "link_2way")
		g_LinkColor2Way = newColor;
	else if(strType == "blockable_blocked")
		g_BlockableBlocked = newColor;
	else if(strType == "blockable_open")
		g_BlockableOpen = newColor;
	else if(strType == "aimentity")
		g_AimEntity = newColor;
	else if(strType == "radius")
		g_RadiusIndicator = newColor;
	else if(strType == "facing")
		g_ShowFacingColor = newColor;
	else if(strType == "team1")
		g_Team1 = newColor;
	else if(strType == "team2")
		g_Team2 = newColor;
	else if(strType == "team3")
		g_Team3 = newColor;
	else if(strType == "team4")
		g_Team4 = newColor;
}

void PathPlannerWaypoint::cmdSelectWaypoints_Helper(const Vector3f &_pos, float _radius)
{
	m_SelectedWaypoints.clear();
	for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
	{
		if((m_WaypointList[i]->GetPosition() - _pos).Length() <= _radius)
		{
			EngineFuncs::ConsoleMessage(va("Added waypoint %d to selection.", m_WaypointList[i]->GetUID()));

			if(std::find(m_SelectedWaypoints.begin(), m_SelectedWaypoints.end(), m_WaypointList[i])==m_SelectedWaypoints.end())
				m_SelectedWaypoints.push_back(m_WaypointList[i]);
		}
	}
}

void PathPlannerWaypoint::cmdSelectWaypoints(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"waypoint_select radius[#]"
		"> radius: radius around you to select waypoints within",
	};

	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(_args.size() == 1)
	{
		m_SelectedWaypoints.clear();
		return;
	}

	CHECK_FLOAT_PARAM(fRadius, 1, strUsage);
	
	Vector3f vPos;
	if(SUCCESS(g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vPos)))
	{
		cmdSelectWaypoints_Helper(vPos, fRadius);
	}
}

void PathPlannerWaypoint::cmdLockSelected(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	for(obuint32 i = 0; i < m_SelectedWaypoints.size(); ++i)
	{
		m_SelectedWaypoints[i]->m_Locked = true;
	}
	EngineFuncs::ConsoleMessage(va("Locked %d waypoints.", m_SelectedWaypoints.size()));
}

void PathPlannerWaypoint::cmdUnlockAll(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	int iNum = 0;
	for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
	{
		if(m_WaypointList[i]->m_Locked)
		{
			m_WaypointList[i]->m_Locked = false;
			iNum++;
		}
	}
	EngineFuncs::ConsoleMessage(va("Unlocked %d waypoints.", iNum));
}

void PathPlannerWaypoint::cmdMinRadius(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"waypoint_minradius radius[#]"
		"> radius: minimum radius to clamp all waypoints to",
	};

	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	CHECK_FLOAT_PARAM(fRadius, 1, strUsage);

	int iNum = 0;
	for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
	{
		if(!m_WaypointList[i]->m_Locked && m_WaypointList[i]->GetRadius() < fRadius)
		{
			m_WaypointList[i]->SetRadius(fRadius);
			iNum++;
		}
	}
	EngineFuncs::ConsoleMessage(va("Changed Radius of %d waypoints to %f.", iNum, fRadius));
}

void PathPlannerWaypoint::cmdMaxRadius(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"waypoint_maxradius radius[#]"
		"> radius: maximum radius to clamp all waypoints to",
	};

	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	CHECK_FLOAT_PARAM(fRadius, 1, strUsage);

	int iNum = 0;
	for(obuint32 i = 0; i < m_WaypointList.size(); ++i)
	{
		if(!m_WaypointList[i]->m_Locked && m_WaypointList[i]->GetRadius() > fRadius)
		{
			m_WaypointList[i]->SetRadius(fRadius);
			iNum++;
		}
	}
	EngineFuncs::ConsoleMessage(va("Changed Radius of %d waypoints to %f.", iNum, fRadius));
}

void PathPlannerWaypoint::cmdAutoBuildFeatures(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const int iMaxFeatures = 1024;
	AutoNavFeature features[iMaxFeatures];
	int iNumFeatures = g_EngineFuncs->GetAutoNavFeatures(features, iMaxFeatures);
	for(int i = 0; i < iNumFeatures; ++i)
	{
		const float fTime = 30.f;

		Vector3f vPos(features[i].m_Position);		
		Vector3f vFace(features[i].m_Facing);
		Vector3f vTarget(features[i].m_TargetPosition);

		// adjust for waypoint offset
		vPos.z -= g_fBottomWaypointOffset;
		vTarget.z -= g_fBottomWaypointOffset;

		// Adjust for waypoint offsets
		if(!features[i].m_Bounds.IsZero())
		{
			features[i].m_Bounds.CenterBottom(vPos);
			vPos.z -= g_fBottomWaypointOffset;
		}
		if(!features[i].m_TargetBounds.IsZero())
		{
			features[i].m_TargetBounds.CenterBottom(vTarget);
			vTarget.z -= g_fBottomWaypointOffset;
		}

		Waypoint *pFeature = AddWaypoint(vPos, vFace, true);
		if(vPos != vTarget)
		{
			Waypoint *pTarget = AddWaypoint(vTarget, Vector3f::ZERO, true);
			pFeature->ConnectTo(pTarget);
		}
		
		//////////////////////////////////////////////////////////////////////////
		Utils::DrawLine(vPos, vPos.AddZ(32), COLOR::GREEN, fTime);
		if(vPos != vTarget)
		{
			Utils::DrawLine(vPos, vTarget, COLOR::MAGENTA, fTime);
			Utils::DrawLine(vTarget, vTarget.AddZ(32), COLOR::RED, fTime);
		}
		if(!features[i].m_Bounds.IsZero())
			Utils::OutlineAABB(features[i].m_Bounds, COLOR::GREEN, fTime);
		if(!features[i].m_TargetBounds.IsZero())
			Utils::OutlineAABB(features[i].m_TargetBounds, COLOR::ORANGE, fTime);
		//////////////////////////////////////////////////////////////////////////
	}
	EngineFuncs::ConsoleMessage(va("Found %d nav features.", iNumFeatures));
}

void PathPlannerWaypoint::cmdBoxSelect(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vAimPos;
	if(Utils::GetLocalAimPoint(vAimPos))
	{
		if(m_BoxStart == Vector3f::ZERO)
		{
			m_BoxStart = vAimPos;
			EngineFuncs::ConsoleMessage("Started Box Select.");
		}
		else
		{
			boxselect.Set(m_BoxStart, vAimPos);
			Utils::OutlineAABB(boxselect, COLOR::MAGENTA, 2.f,AABB::DIR_BOTTOM);

			boxselect.m_Mins[2] = -4096.f;
			boxselect.m_Maxs[2] = 4096.f;

			obuint32 iNumSelected = SelectWaypoints(boxselect);
			m_BoxStart = Vector3f::ZERO;
			EngineFuncs::ConsoleMessage(va("Selected %d waypoints.", iNumSelected));
		}
	}
}

void PathPlannerWaypoint::cmdBoxSelectRoom(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vAimPos;
	if(m_CreatingSector.m_SectorBounds.IsZero() && Utils::GetLocalAimPoint(vAimPos))
	{
		m_CreatingSector.m_SectorBounds.Set(vAimPos);
		EngineFuncs::ConsoleMessage("Started Sector.");
	}
	else
	{
		g_SectorList.push_back(m_CreatingSector);

		Utils::OutlineAABB(m_CreatingSector.m_SectorBounds, COLOR::GREEN, 10.f, AABB::DIR_BOTTOM);

		/*obuint32 iNumSelected = SelectWaypoints(boxselect);
		EngineFuncs::ConsoleMessage("Selected %d waypoints.", iNumSelected);*/

		m_CreatingSector.m_SectorBounds.Set(Vector3f::ZERO);
		EngineFuncs::ConsoleMessage("End Sector.");
	}
}

void PathPlannerWaypoint::cmdWaypointSlice(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const char *strUsage[] = 
	{ 
		"waypoint_slice maxsegmentlength[#]",
		"> maxsegmentlength: max length allowed in slices",
	};

	CHECK_FLOAT_PARAM(fMaxLen, 1, strUsage);

	Vector3f p;
	if(Utils::GetLocalPosition(p))
	{
		ClosestLink l = _GetClosestLink(p,F_NAV_TEAM1|F_NAV_TEAM2|F_NAV_TEAM3|F_NAV_TEAM4);
		if(l.m_Wp[0] && l.m_Wp[1])
		{
			SliceLink(l.m_Wp[0], l.m_Wp[1], fMaxLen);
			return;
		}
	}
	PRINT_USAGE(strUsage);
}

void PathPlannerWaypoint::cmdWaypointSplit(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f p;
	if(!Utils::GetLocalPosition(p))
		return;

	ClosestLink l = _GetClosestLink(p,F_NAV_TEAM1|F_NAV_TEAM2|F_NAV_TEAM3|F_NAV_TEAM4);
	Waypoint *wp0 = l.m_Wp[0]; 
	Waypoint *wp1 = l.m_Wp[1];
	if(!wp0 || !wp1)
	{
		EngineFuncs::ConsoleError("You must stand at a connection between waypoints.");
		return;
	}
	
	const float fMid = (g_fTopWaypointOffset + g_fBottomWaypointOffset) / 2;
	Vector3f p0 = wp0->GetPosition().AddZ(fMid); 
	Vector3f p1 = wp1->GetPosition().AddZ(fMid); 
	
	// find point on connection which is near player
	Vector3f d = p1 - p0;
	p = (p-p1).Dot(d) / d.SquaredLength() * d + p1;

	const float minDist = 20;
	if(Length(p, p0) < minDist || Length(p, p1) < minDist)
	{
		EngineFuncs::ConsoleError("You are too close to a waypoint.");
		return;
	}

	Vector3f g;
	if(GroundPosition(g, p)) p = g;

	Waypoint *wp = AddWaypoint(p,Vector3f::ZERO);			

	if(wp0->IsConnectedTo(wp1)){
		wp0->DisconnectFrom(wp1);
		wp0->ConnectTo(wp);
		wp->ConnectTo(wp1);
	}
	if(wp1->IsConnectedTo(wp0)){
		wp1->DisconnectFrom(wp0);
		wp1->ConnectTo(wp);
		wp->ConnectTo(wp0);
	}

 	if(wp0->IsAnyFlagOn(m_BlockableMask) && wp1->IsAnyFlagOn(m_BlockableMask)) 
		BuildBlockableList();
}

void PathPlannerWaypoint::cmdWaypointUnSplit(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vLocalPos;
	if(!Utils::GetLocalPosition(vLocalPos))
		return;
	Waypoint *wP = _GetClosestWaypoint(vLocalPos, 0, NOFILTER);
	if(!wP || wP->m_Connections.size() != 2)
	{
		EngineFuncs::ConsoleError("The closest waypoint does not have 2 connections.");
		return;
	}

	Waypoint::ConnectionList::iterator it = wP->m_Connections.begin();
	Waypoint *wp0 = it->m_Connection;
	Waypoint *wp1 = (++it)->m_Connection;
	DeleteWaypoint(wP);
	wp0->ConnectTo(wp1);
	wp1->ConnectTo(wp0);

	if(wp0->IsAnyFlagOn(m_BlockableMask) && wp1->IsAnyFlagOn(m_BlockableMask))
		BuildBlockableList();
}

void PathPlannerWaypoint::cmdWaypointGround(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	/*const char *strUsage[] = 
	{ 
		"waypoint_ground",
	};*/

	WaypointList::iterator it = m_WaypointList.begin();
	for(; it != m_WaypointList.end(); ++it)
	{
		Waypoint *w = (*it);

		NavFlags NO_GROUND_FLAGS = F_NAV_ELEVATOR|F_NAV_CLIMB;

		if(w->GetNavigationFlags() & NO_GROUND_FLAGS)
			continue;

		Vector3f wpmid = w->GetPosition().AddZ((g_fTopWaypointOffset + g_fBottomWaypointOffset)/2);

		Vector3f np;
		if(GroundPosition(np,wpmid,true))
		{
			w->m_Position = np;
		}
	}
	//PRINT_USAGE(strUsage);
}

//////////////////////////////////////////////////////////////////////////
