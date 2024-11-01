#include "PrecompCommon.h"
#include "PathPlannerFloodFill.h"
#include "NavigationManager.h"

#include "ScriptManager.h"

// Title: PathPlannerFloodFill Script Commands

static PathPlannerFloodFill *GetNavPlanner()
{
	PathPlannerBase *pPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
	if(pPlanner->GetPlannerType() == NAVID_FLOODFILL)
		return static_cast<PathPlannerFloodFill*>(pPlanner);
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
int GM_CDECL gmfFloodFillView(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
		pPlanner->m_PlannerFlags.SetFlag(PathPlannerFloodFill::NAV_VIEW, Enable!=0);
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
int GM_CDECL gmfFloodFillViewConnections(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
	{
		if(Enable)
			pPlanner->m_PlannerFlags.SetFlag(PathPlannerFloodFill::NAV_VIEWCONNECTIONS);
		else
			pPlanner->m_PlannerFlags.ClearFlag(PathPlannerFloodFill::NAV_VIEWCONNECTIONS);
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
int GM_CDECL gmfFloodFillEnableStep(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Enable, 0);
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
	{
		if(Enable)
			pPlanner->m_PlannerFlags.SetFlag(PathPlannerFloodFill::NAVMESH_STEPPROCESS);
		else
			pPlanner->m_PlannerFlags.ClearFlag(PathPlannerFloodFill::NAVMESH_STEPPROCESS);
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
int GM_CDECL gmfFloodFillStep(gmThread *a_thread)
{
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
		pPlanner->m_PlannerFlags.SetFlag(PathPlannerFloodFill::NAVMESH_TAKESTEP);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: AddFloodStart
//		Adds a position to the list of locations to start flood fills at.
//
// Parameters:
//
//		<Vector3> - Position to add to the start list.
//
// Returns:
//		none
int GM_CDECL gmfFloodFillAddFloodStart(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
		pPlanner->AddFloodStart(Vector3f(v.x,v.y,v.z));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ClearFloodStarts
//		Clear all the flood start locations.
//
// Parameters:
//
//		none
//
// Returns:
//		none
int GM_CDECL gmfFloodFillClearFloodStarts(gmThread *a_thread)
{
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
		pPlanner->ClearFloodStarts();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: LoadFloodStarts
//		Load the flood starts for the current map.
//
// Parameters:
//
//		none
//
// Returns:
//		none
int GM_CDECL gmfFloodFillLoadFloodStarts(gmThread *a_thread)
{
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
		pPlanner->LoadFloodStarts();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: SaveFloodStarts
//		Save the flood starts for the current map.
//
// Parameters:
//
//		none
//
// Returns:
//		none
int GM_CDECL gmfFloodFillSaveFloodStarts(gmThread *a_thread)
{
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
		pPlanner->SaveFloodStarts();
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
int GM_CDECL gmfFloodFillFloodFill(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(4);
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
	{
		GM_CHECK_FLOAT_OR_INT_PARAM(fGridRadius, 0);
		GM_CHECK_FLOAT_OR_INT_PARAM(fStepHeight, 1);
		GM_CHECK_FLOAT_OR_INT_PARAM(fJumpHeight, 2);
		GM_CHECK_FLOAT_OR_INT_PARAM(fCrouchHeight, 3);
		GM_CHECK_FLOAT_OR_INT_PARAM(fCharacterHeight, 4);	

		PathPlannerFloodFill::FloodFillOptions options;
		options.m_GridRadius = fGridRadius;
		options.m_CharacterStepHeight = fStepHeight;
		options.m_CharacterJumpHeight = fJumpHeight;
		options.m_CharacterCrouchHeight = fCrouchHeight;
		options.m_CharacterHeight = fCharacterHeight;		
		pPlanner->FloodFill(options);
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: TrimSectors
//		Begins navigation generation using flood fill algorithm.
//
// Parameters:
//
//		none
//
// Returns:
//		none
int GM_CDECL gmfFloodFillTrimSectors(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(trimarea, 0);
	PathPlannerFloodFill *pPlanner = GetNavPlanner();
	if(pPlanner)
		pPlanner->TrimSectors(trimarea);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// package: Global Waypoint Pathplanner Functions
static gmFunctionEntry s_floodfillLib[] = 
{
	{"EnableView",			gmfFloodFillView},
	{"EnableViewConnection",gmfFloodFillViewConnections},
	
	{"EnableStep",			gmfFloodFillEnableStep},
	{"Step",				gmfFloodFillStep},

	{"AddFloodStart",		gmfFloodFillAddFloodStart},
	{"ClearFloodStarts",	gmfFloodFillClearFloodStarts},

	{"SaveFloodStarts",		gmfFloodFillSaveFloodStarts},
	{"LoadFloodStarts",		gmfFloodFillLoadFloodStarts},

	{"TrimSectors",			gmfFloodFillTrimSectors},

	{"FloodFill",			gmfFloodFillFloodFill},
};

void PathPlannerFloodFill::RegisterScriptFunctions(gmMachine *a_machine)
{
	a_machine->RegisterLibrary(
		s_floodfillLib, 
		sizeof(s_floodfillLib) / sizeof(s_floodfillLib[0]), 
		"Nav");
}
