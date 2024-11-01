#include "PrecompCommon.h"
#include "PathPlannerWaypoint.h"
#include "NavigationManager.h"
#include "ScriptManager.h"

// Title: PathPlannerWaypoint Script Commands
static PathPlannerWaypoint *GetWpPlanner()
{
	PathPlannerBase *pPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
	if(pPlanner->GetPlannerType() == NAVID_WP)
		return static_cast<PathPlannerWaypoint*>(pPlanner);
	return 0;
}

//////////////////////////////////////////////////////////////////////////

// function: AddWaypoint
//		Add a waypoint at a specified location.
//
// Parameters:
//
//		<Vector3> - The <Vector3> position to add waypoint.
//		<Vector3> - OPTIONAL - The <Vector3> facing for the waypoint.
//
// Returns:
//		int - Waypoint Id if successful
//		OR
//		nul if there was an error adding waypoint.
static int GM_CDECL gmfAddWaypoint(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v1,0);

	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{
		Waypoint* pWaypoint; 

		if(a_thread->GetNumParams() > 1) 
		{
			GM_CHECK_VECTOR_PARAM(v2, 1);
			pWaypoint = pWp->AddWaypoint(Vector3f(v1.x, v1.y, v1.z), Vector3f(v2.x, v2.y, v2.z));
		}
		else {
			pWaypoint = pWp->AddWaypoint(Vector3f(v1.x, v1.y, v1.z));
		}
		if(pWaypoint)
		{
			a_thread->PushInt(pWaypoint->GetUID());
			return GM_OK;
		}
	}
	a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DeleteWaypoint
//		Delete a waypoint from a specified location.
//
// Parameters:
//
//		<Vector3> - The <Vector3> position to delete waypoint from.
//		- OR -
//		<int> - Waypoint Guid to delete
//
// Returns:
//		int - true if successful, false if not
static int GM_CDECL gmfDeleteWaypoint(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	bool bSuccess = false;
	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{
		if(a_thread->ParamType(0) == GM_INT)
		{
			GM_CHECK_INT_PARAM(wpid,0);
			Waypoint *w = pWp->GetWaypointByGUID(wpid);
			if(w){
				pWp->DeleteWaypoint(w);
				bSuccess = true;
			}
		}
		else if(a_thread->ParamType(GM_VEC3))
		{
			GM_CHECK_VECTOR_PARAM(v,0);
			bSuccess = pWp->DeleteWaypoint(Vector3f(v.x,v.y,v.z));
		}
	}
	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

void SetWaypointDataInTable(gmMachine *_machine, gmTableObject *_table, const Waypoint *_waypoint)
{
	DisableGCInScope gcEn(_machine);

	_table->Set(_machine, "position", gmVariable(_waypoint->GetPosition().x, _waypoint->GetPosition().y, _waypoint->GetPosition().z));
	_table->Set(_machine, "facing", gmVariable(_waypoint->GetFacing().x, _waypoint->GetFacing().y, _waypoint->GetFacing().z));
	_table->Set(_machine, "guid", gmVariable(_waypoint->GetUID()));
	_table->Set(_machine, "radius", gmVariable(_waypoint->GetRadius()));
	if (!_waypoint->GetName().empty()) _table->Set(_machine, "name", _waypoint->GetName().c_str());

	gmTableObject *pFlagTable = _machine->AllocTableObject();

	_table->Set(_machine, "flags", gmVariable(pFlagTable));

	// add all flags this waypoint has.
	//////////////////////////////////////////////////////////////////////////
	{
		const PathPlannerWaypoint::FlagMap &flagMap =  GetWpPlanner()->GetNavFlagMap();
		PathPlannerWaypoint::FlagMap::const_iterator it = flagMap.begin(),
			itEnd = flagMap.end();
		for(;it != itEnd; ++it)
		{
			if(_waypoint->IsFlagOn(it->second))
				pFlagTable->Set(_machine, it->first.c_str(), gmVariable(1));
		}
	}
	//////////////////////////////////////////////////////////////////////////
	{
		gmTableObject *pPropertyTable = _machine->AllocTableObject();
		_table->Set(_machine, "property", gmVariable(pPropertyTable));
		const PropertyMap &pm = _waypoint->GetPropertyMap();
		for(PropertyMap::ValueMap::const_iterator it = pm.GetProperties().begin();
			it != pm.GetProperties().end();
			++it)
		{
			pPropertyTable->Set(_machine, it->first.c_str(), it->second.c_str());
		}
	}
	//////////////////////////////////////////////////////////////////////////
}

// function: GetWaypointByName
//		Gets a waypoint information by its name.
//
// Parameters:
//
//		<string> - The name of the waypoint to get the info of.
//		<table> - An empty table, the function will fill in position, facing, guid.
//
// Returns:
//		int - true if successful, false if not
static int GM_CDECL gmfGetWaypointByName(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(name, 0);
	GM_CHECK_TABLE_PARAM(params, 1);
	
	gmMachine *pMachine = a_thread->GetMachine();
	DisableGCInScope gcEn(pMachine);

	bool bSuccess = false;
	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{		
		Waypoint *pWaypoint = pWp->GetWaypointByName(name);
		if(pWaypoint)
		{
			SetWaypointDataInTable(pMachine, params, pWaypoint);
			bSuccess = true;
		}
	}

	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetWaypointByGUID
//		Gets a waypoint information by its guid.
//
// Parameters:
//
//		<int> - Guid of a waypoint.
//		<table> - An empty table, the function will fill in position, facing, guid.
//
// Returns:
//		int - true if successful, false if not
static int GM_CDECL gmfGetWaypointByUID(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(guid, 0);
	GM_CHECK_TABLE_PARAM(params, 1);

	gmMachine *pMachine = a_thread->GetMachine();
	DisableGCInScope gcEn(pMachine);

	bool bSuccess = false;
	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{		
		Waypoint *pWaypoint = pWp->GetWaypointByGUID(guid);
		if(pWaypoint)
		{
			SetWaypointDataInTable(pMachine, params, pWaypoint);
			bSuccess = true;
		}
	}
	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ConnectWaypoints
//		Connects two waypoints.
//
// Parameters:
//
//		<int> - Guid of a waypoint.
//		<int> - Guid of another waypoint.
//
// Returns:
//		int - true if successful, false if not
static int GM_CDECL gmfConnectWaypoints(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(guid1, 0);
	GM_CHECK_INT_PARAM(guid2, 1);

	PathPlannerWaypoint *pWp = GetWpPlanner();
	bool bSuccess = pWp && pWp->_ConnectWaypoints(pWp->GetWaypointByGUID(guid1), pWp->GetWaypointByGUID(guid2));
	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DisconnectWaypoints
//		Disconnects two waypoints.
//
// Parameters:
//
//		<int> - Guid of a waypoint.
//		<int> - Guid of another waypoint.
//
// Returns:
//		int - true if successful, false if not
static int GM_CDECL gmfDisconnectWaypoints(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(guid1, 0);
	GM_CHECK_INT_PARAM(guid2, 1);

	bool bSuccess = false;
	PathPlannerWaypoint* pWp = GetWpPlanner();
	if (pWp)
	{
		Waypoint* pWaypoint1 = pWp->GetWaypointByGUID(guid1);
		Waypoint* pWaypoint2 = pWp->GetWaypointByGUID(guid2);
		if (pWaypoint1 != NULL && pWaypoint2 != NULL)
		{
			pWaypoint1->DisconnectFrom(pWaypoint2);
			bSuccess = true;
		}
	}

	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: SetRadius
//		Sets a wapoint radius by guid.
//
// Parameters:
//
//		<int> - Guid of a waypoint.
//		<int> - Radius.
//
// Returns:
//		int - true if successful, false if not
static int GM_CDECL gmfSetRadius(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(guid1, 0);
	GM_CHECK_FLOAT_PARAM(radius, 1);

	bool bSuccess = false;
	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{		
		Waypoint *pWaypoint1 = pWp->GetWaypointByGUID(guid1);
		if(pWaypoint1)
		{
			pWaypoint1->SetRadius(radius);
			bSuccess = true;
		}
	}

	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: SetWaypointFlag
//		Sets the flag on a waypoint by name.
//
// Parameters:
//
//		<int> - Guid of a waypoint.
//		- OR -
//		<string> - Name of the waypoint.
//		<string> - The name of the flag to set.
//		- OR -
//		<table> - table of flags.
//		<int> - True to set, false to clear.
//
// Returns:
//		None
static int GM_CDECL gmfSetWaypointFlag(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(3);

	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(!pWp)
	{
		GM_EXCEPTION_MSG("Wrong Path Planner");
		return GM_EXCEPTION;
	}

	PathPlannerWaypoint::WaypointList list;
	switch(a_thread->ParamType(0))
	{
		case GM_INT:
		{
			Waypoint *pWaypoint = pWp->GetWaypointByGUID(a_thread->ParamInt(0));
			if(pWaypoint) list.push_back(pWaypoint);
			break;
		}
		case GM_STRING:
		{
			const char *name=a_thread->ParamString(0);
			pWp->GetWaypointsByName(name, list);
			if(list.empty()) pWp->GetWaypointsByExpr(name, list);
			break;
		}
	}
	if(list.size()==0)
	{
		GM_EXCEPTION_MSG("Invalid Waypoint specified in param 0");
		return GM_EXCEPTION;
	}

	NavFlags flag=0;
	switch(a_thread->ParamType(1))
	{
		case GM_STRING:
		{
			const char *flagname = a_thread->ParamString(1);
			if(!pWp->GetNavFlagByName(flagname, flag))
			{
				GM_EXCEPTION_MSG("Invalid navigation flag %s", flagname);
				return GM_EXCEPTION;
			}
			break;
		}
		case GM_TABLE:
		{
			gmTableObject* tbl = a_thread->ParamTable(1);
			gmTableIterator tIt;
			for(gmTableNode *pNode = tbl->GetFirst(tIt); pNode; pNode = tbl->GetNext(tIt))
			{
				const char *flagname = pNode->m_value.GetCStringSafe();
				NavFlags flag1;
				if(!pWp->GetNavFlagByName(flagname, flag1))
				{
					GM_EXCEPTION_MSG("Invalid navigation flag %s", flagname);
					return GM_EXCEPTION;
				}
				flag|=flag1;
			}
			break;
		}
		default:
			GM_EXCEPTION_MSG("expecting param 1 as string or table");
			return GM_EXCEPTION;
	}

	GM_CHECK_INT_PARAM(enable, 2);

	PathPlannerWaypoint::WaypointList::const_iterator cIt = list.begin(), cItEnd = list.end();
	for(; cIt != cItEnd; ++cIt)
	{
		if(enable)
			(*cIt)->AddFlag(flag);
		else
		{
			(*cIt)->RemoveFlag(flag);
			if(flag & PathPlannerWaypoint::m_BlockableMask) pWp->ClearBlockable(*cIt);
		}

		if(!(*cIt)->IsAnyFlagOn(F_NAV_TEAM_ALL))
		{
			(*cIt)->RemoveFlag(F_NAV_TEAMONLY);
		}
		else
		{
			// At least one of them is on, so make sure the teamonly flag is set.
			(*cIt)->AddFlag(F_NAV_TEAMONLY);
		}
	}

	if(flag & PathPlannerWaypoint::m_BlockableMask)
		pWp->BuildBlockableList();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: SetWaypointProperty
//		Sets the waypoint property.
//
// Parameters:
//
//		<int> - Guid of a waypoint.
//		- OR -
//		<string> - Name of the waypoint.
//		<string> - The name of the property to set.
//		<string> - Property value to set.
//
// Returns:
//		None
static int GM_CDECL gmfSetWaypointProperty(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(3);

	Waypoint *pWaypoint = 0;
	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{
		if(a_thread->ParamType(0) == GM_INT)
		{
			GM_CHECK_INT_PARAM(guid, 0);
			pWaypoint = pWp->GetWaypointByGUID(guid);
		} 
		else if(a_thread->ParamType(0) == GM_STRING)
		{
			GM_CHECK_STRING_PARAM(name, 0);
			pWaypoint = pWp->GetWaypointByName(name);
		}		
	}
	else
	{
		GM_EXCEPTION_MSG("Wrong Path Planner");
		return GM_EXCEPTION;
	}

	if(!pWaypoint)
	{
		GM_EXCEPTION_MSG("Invalid Waypoint specified in param 0");
		return GM_EXCEPTION;
	}

	GM_CHECK_STRING_PARAM(propname, 1);
	GM_CHECK_STRING_PARAM(propvalue, 2);

	if(*propvalue != 0 && propvalue)
		pWaypoint->GetPropertyMap().AddProperty(propname,propvalue);
	else
		pWaypoint->GetPropertyMap().DelProperty(propname);

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

extern float g_fTopWaypointOffset;
extern float g_fBottomWaypointOffset;
extern float g_fWaypointPathOffset;
extern float g_fTopPathOffset;
extern float g_fBottomPathOffset;
extern float g_fBlockablePathOffset;
extern float g_fFacingOffset;
extern float g_fWaypointTextOffset;
extern float g_fWaypointTextDuration;
extern float g_fPathLevelOffset;

//////////////////////////////////////////////////////////////////////////

// function: WaypointColor
//		Customize the color of various types of waypoints, paths, etc...
//		See waypoint_color command for help.
//
// Parameters:
//
//		<string> - The name of the category to set a color for.
//		<int> - The color to use for this type.
//
// Returns:
//		None
static int GM_CDECL gmfWaypointColor(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);	
	GM_CHECK_STRING_PARAM(wptype, 0);
	GM_CHECK_INT_PARAM(c, 1);	

	obColor color(c);

	StringVector v;
	v.push_back("waypoint_color");
	v.push_back(wptype);
	v.push_back((String)va("%d", color.r()));
	v.push_back((String)va("%d", color.g()));
	v.push_back((String)va("%d", color.b()));
	v.push_back((String)va("%d", color.a()));
	CommandReciever::DispatchCommand(v);
	return GM_OK;
}

static int GM_CDECL gmfGetAllWaypoints(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);	
	GM_CHECK_TABLE_PARAM(tbl, 0);

	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{
		const PathPlannerWaypoint::WaypointList &wl = pWp->GetWaypointList();
		PathPlannerWaypoint::WaypointList::const_iterator wIt = wl.begin(), wItEnd = wl.end();

		obint32 iNum = 0;
		for(; wIt != wItEnd; ++wIt)
		{
			const Waypoint *pWaypoint = (*wIt);
			gmTableObject *pWpTable = a_thread->GetMachine()->AllocTableObject();
			tbl->Set(a_thread->GetMachine(), iNum++, gmVariable(pWpTable));
			SetWaypointDataInTable(a_thread->GetMachine(),pWpTable,pWaypoint);
		}
	}
	return GM_OK;
}

static int GM_CDECL gmfGetAllSelectedWaypoints(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);	
	GM_CHECK_TABLE_PARAM(tbl, 0);	

	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{
		const PathPlannerWaypoint::WaypointList &wl = pWp->GetSelectedWaypointList();
		PathPlannerWaypoint::WaypointList::const_iterator wIt = wl.begin(), wItEnd = wl.end();

		obint32 iNum = 0;
		for(; wIt != wItEnd; ++wIt)
		{
			const Waypoint *pWaypoint = (*wIt);
			gmTableObject *pWpTable = a_thread->GetMachine()->AllocTableObject();
			tbl->Set(a_thread->GetMachine(), iNum++, gmVariable(pWpTable));
			SetWaypointDataInTable(a_thread->GetMachine(),pWpTable,pWaypoint);
		}
	}
	return GM_OK;
}

static int GM_CDECL gmfGetClosestWaypoint(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(pos, 0);
	GM_INT_PARAM(team, 1, 0);
	GM_INT_PARAM(options, 2, 1);

	PathPlannerWaypoint *pWp = GetWpPlanner();
	if(pWp)
	{
		const Waypoint *pWaypoint = pWp->_GetClosestWaypoint(Vector3f(pos), (NavFlags)team, options);
		if(pWaypoint)
		{
			gmTableObject *pWpTable = a_thread->GetMachine()->AllocTableObject();
			SetWaypointDataInTable(a_thread->GetMachine(), pWpTable, pWaypoint);
			a_thread->PushTable(pWpTable);
			return GM_OK;
		}
	}
	a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfCheckBlockable(gmThread* a_thread)
{
	PathPlannerWaypoint* pWp = GetWpPlanner();
	if (pWp) a_thread->PushInt(pWp->CheckBlockable());
	else a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfWaypointSave(gmThread *a_thread)
{
	GM_STRING_PARAM(wpname,0,0);	

	PathPlannerWaypoint *obj = NULL;
	if(gmBind2::GetThisGMType<PathPlannerWaypoint>(a_thread, obj)==GM_EXCEPTION)
		return GM_EXCEPTION;

	String strFile = wpname ? wpname : g_EngineFuncs->GetMapName();
	a_thread->PushInt(obj->Save(strFile)?1:0);
	return GM_OK;
}

static int GM_CDECL gmfWaypointLoad(gmThread *a_thread)
{
	GM_STRING_PARAM(wpname,0,0);	

	PathPlannerWaypoint *obj = NULL;
	if(gmBind2::GetThisGMType<PathPlannerWaypoint>(a_thread, obj)==GM_EXCEPTION)
		return GM_EXCEPTION;

	String strFile = wpname ? wpname : g_EngineFuncs->GetMapName();
	a_thread->PushInt(obj->Load(strFile)?1:0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

void PathPlannerWaypoint::RegisterScriptFunctions(gmMachine *a_machine)
{
	gmBind2::Class<PathPlannerWaypoint>("_Wp", a_machine)
		.func(gmfAddWaypoint, "AddWaypoint")
		.func(gmfDeleteWaypoint, "DeleteWaypoint")
		.func(gmfGetWaypointByName, "GetWaypointByName")
		.func(gmfGetWaypointByUID, "GetWaypointByGUID")
		.func(gmfConnectWaypoints, "Connect")
		.func(gmfDisconnectWaypoints, "Disconnect")
		.func(gmfSetRadius, "SetRadius")
		.func(gmfSetWaypointFlag, "SetWaypointFlag")
		.func(gmfSetWaypointProperty, "SetWaypointProperty")
		.func(gmfWaypointColor, "WaypointColor")
		.func(gmfGetAllWaypoints, "GetAllWaypoints")
		.func(gmfGetAllSelectedWaypoints, "GetAllSelectedWaypoints")
		.func(gmfGetClosestWaypoint, "GetClosestWaypoint")
		.func(gmfCheckBlockable, "CheckBlockable")
		.func(gmfWaypointSave,"Save")
		.func(gmfWaypointLoad,"Load")
		.func((bool (PathPlannerWaypoint::*)())&PathPlannerWaypoint::IsViewOn,"IsWaypointViewOn")
		.func((bool (PathPlannerWaypoint::*)(int, const String &))&PathPlannerWaypoint::SetWaypointName,"SetWaypointName")
		.var(&g_fTopWaypointOffset,"TopWaypointOffset")
		.var(&g_fBottomWaypointOffset,"BottomWaypointOffset")
		.var(&g_fTopPathOffset,"TopPathOffset")
		.var(&g_fBottomPathOffset,"BottomPathOffset")
		.var(&g_fBlockablePathOffset,"BlockablePathOffset")
		.var(&g_fFacingOffset,"FacingOffset")
		.var(&g_fWaypointTextOffset,"TextOffset")
		.var(&g_fWaypointTextDuration,"TextDuration")
		.var(&g_fPathLevelOffset,"PathLevelOffset")
		;

	m_WpRef = gmBind2::Class<PathPlannerWaypoint>::WrapObject(a_machine,this);
	a_machine->GetGlobals()->Set(a_machine,"Wp",gmVariable(m_WpRef)); //cs: changed back to Wp from Nav
}
