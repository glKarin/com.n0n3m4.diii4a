#include "PrecompCommon.h"
#include <limits>

#include "PathPlannerRecast.h"
#include "ScriptManager.h"
#include "IGameManager.h"
#include "Waypoint.h"
#include "IGame.h"
#include "Client.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////

void PathPlannerRecast::InitCommands()
{
	PathPlannerBase::InitCommands();

	SetEx("nav_save", "Save current navigation to disk", 
		this, &PathPlannerRecast::cmdNavSave);
	SetEx("nav_load", "Load last saved navigation from disk", 
		this, &PathPlannerRecast::cmdNavLoad);
	SetEx("nav_view", "Turn on/off navmesh visibility.", 
		this, &PathPlannerRecast::cmdNavView);
	SetEx("nav_viewconnections", "Turn on/off navmesh connection visibility.", 
		this, &PathPlannerRecast::cmdNavViewConnections);
	
	//////////////////////////////////////////////////////////////////////////
	SetEx("nav_addfloodseed", "Adds a starting node for the flood fill.", 
		this, &PathPlannerRecast::cmdAddFloodSeed);
	SetEx("nav_floodfill", "Adds a starting node for the flood fill.", 
		this, &PathPlannerRecast::cmdFloodFill);

	SetEx("nav_build", "Adds a starting node for the flood fill.", 
		this, &PathPlannerRecast::cmdBuildNav);
		//////////////////////////////////////////////////////////////////////////
	/*SetEx("nav_autofeature", "Automatically waypoints jump pads, teleporters, player spawns.", 
		this, &PathPlannerRecast::cmdAutoBuildFeatures);*/

	SetEx("nav_createladder", "creates a ladder in the navigation system.", 
		this, &PathPlannerRecast::cmdCreateLadder);
}

void PathPlannerRecast::cmdNavSave(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(Save(g_EngineFuncs->GetMapName()))
	{
		EngineFuncs::ConsoleMessage("Saved Nav.");
	}
	else
		EngineFuncs::ConsoleError("ERROR Saving Nav.");
}

void PathPlannerRecast::cmdNavLoad(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(Load(g_EngineFuncs->GetMapName()))
	{
		EngineFuncs::ConsoleMessage("Loaded Nav.");
	} 
	else
		EngineFuncs::ConsoleError("ERROR Loading Nav.");
}

void PathPlannerRecast::cmdNavView(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"nav_view enable[bool]",
		"> enable: Enable nav rendering. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	m_PlannerFlags.SetFlag(PathPlannerRecast::NAV_VIEW, bEnable!=0);
}

void PathPlannerRecast::cmdNavViewConnections(const StringVector &_args)
{
	const char *strUsage[] = 
	{ 
		"nav_viewconnections enable[bool]",
		"> enable: Enable nav connection rendering. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	m_PlannerFlags.SetFlag(PathPlannerRecast::NAV_VIEWCONNECTIONS, bEnable!=0);
}

void PathPlannerRecast::cmdAddFloodSeed(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vPosition;
	if(Utils::GetLocalAimPoint(vPosition))
	{
		AddFloodSeed(vPosition);
	}
}

void PathPlannerRecast::cmdFloodFill(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vPosition;
	if(Utils::GetLocalAimPoint(vPosition))
	{
		FloodFill();
	}

}

void PathPlannerRecast::cmdBuildNav(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	BuildNav();
}

void PathPlannerRecast::cmdCreateLadder(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	int contents = 0, surface = 0;
	Vector3f vAimPt, vAimNormal;
	if(Utils::GetLocalAimPoint(vAimPt, &vAimNormal, TR_MASK_FLOODFILL, &contents, &surface))
	{
		if(surface & SURFACE_LADDER) 
		{
			const Vector3f vStartPt = vAimPt + vAimNormal * 16.f;
			const Vector3f vSide = vAimNormal.Cross(Vector3f::UNIT_Z);

			const float StepSize = 4.f;
			const float TraceDist = 32.f;

			obTraceResult tr;
			
			// find the width
			Vector3f vDir[2] = { vSide, -vSide };
			Vector3f vEdge[2] = { vStartPt,vStartPt };
			for(int i = 0; i < 2; ++i)
			{
				for(;;)
				{
					EngineFuncs::TraceLine(tr,vEdge[i],vEdge[i]-vAimNormal*TraceDist,0,TR_MASK_FLOODFILL,0,False);
					if(tr.m_Fraction < 1.f && tr.m_Surface & SURFACE_LADDER)
					{
						vEdge[i] += vDir[i]*StepSize;
					}
					else
						break;
				}
			}

			Vector3f vBottom = (vEdge[0]+vEdge[1]) * 0.5f;
			Vector3f vTop = vBottom;

			// find the bottom			
			for(;;)
			{
				EngineFuncs::TraceLine(tr,vBottom,vBottom-vAimNormal*TraceDist,0,TR_MASK_FLOODFILL,0,False);
				if(tr.m_Fraction < 1.f && tr.m_Surface & SURFACE_LADDER)
				{
					vBottom.z -= StepSize;
				}
				else
					break;
			}

			// find the top
			for(;;)
			{
				EngineFuncs::TraceLine(tr,vTop,vTop-vAimNormal*TraceDist,0,TR_MASK_FLOODFILL,0,False);
				if(tr.m_Fraction < 1.f && tr.m_Surface & SURFACE_LADDER)
				{
					vTop.z += StepSize;
				}
				else
					break;
			}

			ladder_t newLadder;
			newLadder.normal = vAimNormal;
			newLadder.top = vTop - vAimNormal * 32.f;
			newLadder.bottom = vBottom + vAimNormal * 32.f;
			newLadder.width = (vEdge[0]-vEdge[1]).Length();
			
			bool createLadder = true;			
			for(obuint32 i = 0; i < ladders.size(); ++i)
			{
				if(ladders[i].OverLaps(newLadder))
				{
					createLadder = false;
				}
			}

			if(createLadder)
			{
				ladders.push_back(newLadder);
			}
		}
		else
		{
			EngineFuncs::ConsoleError("You must be aiming at a ladder surface to create a ladder.");
		}
	}
}
