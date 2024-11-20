#include "PrecompCommon.h"
#include "PathPlannerNavMesh.h"
#include "NavigationManager.h"

#include "ScriptManager.h"

// Title: PathPlannerNavMesh Script Commands

static PathPlannerNavMesh *GetNavPlannerNM()
{
	PathPlannerBase *pPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
	if(pPlanner->GetPlannerType() == NAVID_NAVMESH)
		return static_cast<PathPlannerNavMesh*>(pPlanner);
	return 0;
}

//////////////////////////////////////////////////////////////////////////

// function: EnableView
//		Enables/Disables the nav mesh from rendering.
//
// Parameters:
//
//		<int> - true to enable/false to disable
//
// Returns:
//		none
int GM_CDECL gmfNavMeshView(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerNavMesh *pPlanner = GetNavPlannerNM();
	if(pPlanner)
		pPlanner->m_PlannerFlags.SetFlag(PathPlannerNavMesh::NAV_VIEW, Enable!=0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: EnableViewConnection
//		Enables/Disables the nav mesh connections from rendering.
//
// Parameters:
//
//		<int> - true to enable/false to disable
//
// Returns:
//		none
int GM_CDECL gmfNavMeshViewConnections(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerNavMesh *pPlanner = GetNavPlannerNM();
	if(pPlanner)
	{
		if(Enable)
			pPlanner->m_PlannerFlags.SetFlag(PathPlannerNavMesh::NAV_VIEWCONNECTIONS);
		else
			pPlanner->m_PlannerFlags.ClearFlag(PathPlannerNavMesh::NAV_VIEWCONNECTIONS);
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: EnableStep
//		Enables/Disables the nav mesh step generation.
//
// Parameters:
//
//		<int> - true to enable/false to disable
//
// Returns:
//		none
int GM_CDECL gmfNavMeshEnableStep(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerNavMesh *pPlanner = GetNavPlannerNM();
	if(pPlanner)
	{
		if(Enable)
			pPlanner->m_PlannerFlags.SetFlag(PathPlannerNavMesh::NAVMESH_STEPPROCESS);
		else
			pPlanner->m_PlannerFlags.ClearFlag(PathPlannerNavMesh::NAVMESH_STEPPROCESS);
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Step
//		Tells the nav mesh generation function to go to the next step..
//
// Parameters:
//
//		none
//
// Returns:
//		none
int GM_CDECL gmfNavMeshStep(gmThread *a_thread)
{
	PathPlannerNavMesh *pPlanner = GetNavPlannerNM();
	if(pPlanner)
		pPlanner->m_PlannerFlags.SetFlag(PathPlannerNavMesh::NAVMESH_TAKESTEP);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// package: Global Waypoint Pathplanner Functions
static gmFunctionEntry s_navmeshLib[] = 
{
	{"EnableView",			gmfNavMeshView},
	{"EnableViewConnection",gmfNavMeshViewConnections},
	
	{"EnableStep",			gmfNavMeshEnableStep},
	{"Step",				gmfNavMeshStep},	
};

void PathPlannerNavMesh::RegisterScriptFunctions(gmMachine *a_machine)
{
	a_machine->RegisterLibrary(
		s_navmeshLib, 
		sizeof(s_navmeshLib) / sizeof(s_navmeshLib[0]), 
		"Nav");
}
