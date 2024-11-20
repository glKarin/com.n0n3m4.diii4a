#include "PrecompCommon.h"
#include "PathPlannerRecast.h"
#include "NavigationManager.h"

#include "ScriptManager.h"

// Title: PathPlannerRecast Script Commands

static PathPlannerRecast *GetRecastPlanner()
{
	PathPlannerBase *pPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
	if(pPlanner->GetPlannerType() == NAVID_RECAST)
		return static_cast<PathPlannerRecast*>(pPlanner);
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
int GM_CDECL gmfRecastView(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerRecast *pPlanner = GetRecastPlanner();
	if(pPlanner)
		pPlanner->m_PlannerFlags.SetFlag(PathPlannerRecast::NAV_VIEW, Enable!=0);
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
int GM_CDECL gmfRecastViewConnections(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerRecast *pPlanner = GetRecastPlanner();
	if(pPlanner)
	{
		if(Enable)
			pPlanner->m_PlannerFlags.SetFlag(PathPlannerRecast::NAV_VIEWCONNECTIONS);
		else
			pPlanner->m_PlannerFlags.ClearFlag(PathPlannerRecast::NAV_VIEWCONNECTIONS);
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: AddFloodSeed
//		Adds a position to the list of locations to start flood fills at.
//
// Parameters:
//
//		<Vector3> - Position to add to the start list.
//
// Returns:
//		none
int GM_CDECL gmfRecastAddFloodSeed(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	PathPlannerRecast *pPlanner = GetRecastPlanner();
	if(pPlanner)
		pPlanner->AddFloodSeed(Vector3f(v.x,v.y,v.z));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: BuildNavMesh
//		Builds the nav mesh when the flood fill completes.
//
// Returns:
//		none
int GM_CDECL gmfRecastBuildNavMesh(gmThread *a_thread)
{
	PathPlannerRecast *pPlanner = GetRecastPlanner();
	if(pPlanner)
		pPlanner->BuildNav();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: FloodFill
//		Begins navigation generation using flood fill algorithm.
//
// Parameters:
//
//		float - Grid Radius
//		float - Step Height
//		float - Jump Height
//		float - Character Height
//
// Returns:
//		none
int GM_CDECL gmfRecastFloodFill(gmThread *a_thread)
{
	PathPlannerRecast *pPlanner = GetRecastPlanner();
	if(pPlanner)
	{
		/*GM_CHECK_FLOAT_OR_INT_PARAM(fGridRadius, 0);
		GM_CHECK_FLOAT_OR_INT_PARAM(fStepHeight, 1);
		GM_CHECK_FLOAT_OR_INT_PARAM(fJumpHeight, 2);
		GM_CHECK_FLOAT_OR_INT_PARAM(fCrouchHeight, 3);
		GM_CHECK_FLOAT_OR_INT_PARAM(fCharacterHeight, 4);	

		PathPlannerRecast::FloodFillOptions options;
		options.m_GridRadius = fGridRadius;
		options.m_CharacterStepHeight = fStepHeight;
		options.m_CharacterJumpHeight = fJumpHeight;
		options.m_CharacterCrouchHeight = fCrouchHeight;
		options.m_CharacterHeight = fCharacterHeight;*/
		pPlanner->FloodFill();
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// package: Global Waypoint Pathplanner Functions
static gmFunctionEntry s_recastLib[] = 
{
	{"EnableView",			gmfRecastView},
	{"EnableViewConnection",gmfRecastViewConnections},
	
	{"AddFloodSeed",		gmfRecastAddFloodSeed},
	{"BuildNavMesh",		gmfRecastBuildNavMesh},
	
	{"FloodFill",			gmfRecastFloodFill},
};

void PathPlannerRecast::RegisterScriptFunctions(gmMachine *a_machine)
{
	a_machine->RegisterLibrary(
		s_recastLib, 
		sizeof(s_recastLib) / sizeof(s_recastLib[0]), 
		"Nav");
}
