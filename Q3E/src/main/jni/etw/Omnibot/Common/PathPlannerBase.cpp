#include "PrecompCommon.h"
#include "PathPlannerBase.h"

void PathPlannerBase::InitCommands()
{
	SetEx("nav_logfailedpath", "Saves info about failed path attempts for debugging.", 
		this, &PathPlannerBase::cmdLogFailedPaths);
	SetEx("nav_showfailedpath", "Render a failed path by its index.", 
		this, &PathPlannerBase::cmdShowFailedPaths);
	SetEx("nav_benchmarkpathfinder", "PlanPathToGoal benchmark.", 
		this, &PathPlannerBase::cmdBenchmarkPathFind);
	SetEx("nav_benchmarkgetnavpoint", "GetClosestWaypoint benchmark.", 
		this, &PathPlannerBase::cmdBenchmarkGetNavPoint);
	SetEx("nav_resaveall", "Re-save all nav files to the newest file format.", 
		this, &PathPlannerBase::cmdResaveNav);
}

void PathPlannerBase::cmdLogFailedPaths(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const char *strUsage[] = 
	{ 
		"nav_logfailedpath enable[bool]"
		"> enable: Enable failed path logging. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);

	m_PlannerFlags.SetFlag(NAV_SAVEFAILEDPATHS, bEnable);

	EngineFuncs::ConsoleMessage(va("nav_logfailedpath %s", bEnable ? "enabled" : "disabled"));
}

void PathPlannerBase::cmdShowFailedPaths(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const char *strUsage[] = 
	{ 
		"nav_showfailedpath #"
		"> #: Index of path to toggle rendering.",
		"> enable: Enable nav rendering. true/false/on/off/1/0",
		"",
	};

	if(_args.size() != 3)
	{
		PRINT_USAGE(strUsage);

		EngineFuncs::ConsoleMessage("Failed Paths");
		EngineFuncs::ConsoleMessage("------------");
		int iIndex = 0;
		FailedPathList::iterator it = m_FailedPathList.begin(), itEnd = m_FailedPathList.end();
		for(;it != itEnd; ++it)
		{
			FailedPath &fp = (*it);
			EngineFuncs::ConsoleMessage(
				va("%d: (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f) %s", 
				iIndex,
				fp.m_Start.x, fp.m_Start.y, fp.m_Start.z,
				fp.m_End.x, fp.m_End.y, fp.m_End.z,
				fp.m_Render ? "rendering" : "not rendering"));
			++iIndex;
		}
		return;
	}

	CHECK_INT_PARAM(index, 1, strUsage);
	CHECK_INT_PARAM(enable, 1, strUsage);
	if(index >= (int)m_FailedPathList.size() || index < 0)
	{
		if(!m_FailedPathList.empty())
			EngineFuncs::ConsoleMessage(va("Invalid Index, must be 0-%d", m_FailedPathList.size()));
		else
			EngineFuncs::ConsoleMessage("No failed paths to render.");
		return;
	}

	FailedPathList::iterator it = m_FailedPathList.begin();
	std::advance(it, index);
	(*it).m_Render = enable!=0;
}

void PathPlannerBase::_BenchmarkPathFinder(const StringVector &_args)
{
	EngineFuncs::ConsoleMessage("Benchmark Not Implemented!");
}
void PathPlannerBase::_BenchmarkGetNavPoint(const StringVector &_args)
{
	EngineFuncs::ConsoleMessage("Benchmark Not Implemented!");
}

void PathPlannerBase::cmdBenchmarkPathFind(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	_BenchmarkPathFinder(_args);
}

void PathPlannerBase::cmdBenchmarkGetNavPoint(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	_BenchmarkGetNavPoint(_args);
}

void PathPlannerBase::cmdResaveNav(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	DirectoryList wpFiles;
	FileSystem::FindAllFiles("nav/", wpFiles, va( ".*%s", _GetNavFileExtension().c_str() ).c_str() );
	for(obuint32 i = 0; i < wpFiles.size(); ++i)
	{
		const String &mapname = wpFiles[i].stem()
#if BOOST_FILESYSTEM_VERSION > 2
			.string()
#endif
		;
		bool bGood = false;
		if(Load(mapname))
		{
			bGood = true;
			Save(mapname);
		}

		EngineFuncs::ConsoleMessage(va("Resaving %s, %s", mapname.c_str(),bGood?"success":"failed"));
	}	

	// reload current map wps
	Load(String(g_EngineFuncs->GetMapName()));
}


void PathPlannerBase::AddFailedPath(const Vector3f &_start, const Vector3f &_end)
{
	FailedPath fp;
	fp.m_Start = _start;
	fp.m_End = _end;
	fp.m_NextRenderTime = 0;
	fp.m_Render = false;
	m_FailedPathList.push_back(fp);
	EngineFuncs::ConsoleMessage(va("Added failed path to log, view with nav_showfailedpath %d", 
		m_FailedPathList.size()));
}

bool PathPlannerBase::Load(bool _dl)
{
	return Load(g_EngineFuncs->GetMapName(),_dl);
}

void PathPlannerBase::RenderFailedPaths()
{
	Prof(RenderFailedPaths);

	if(!m_PlannerFlags.CheckFlag(NAV_SAVEFAILEDPATHS))
		return;

	FailedPathList::iterator it = m_FailedPathList.begin(), itEnd = m_FailedPathList.end();
	for(;it != itEnd; ++it)
	{
		FailedPath &fp = (*it);
		if(fp.m_Render)
		{
			if(IGame::GetTime() >= fp.m_NextRenderTime)
			{
				AABB aabb;
				Vector3f pos;
				GameEntity ge = Utils::GetLocalEntity();
				if(ge.IsValid() && 
					EngineFuncs::EntityPosition(ge, pos) && 
					EngineFuncs::EntityWorldAABB(ge, aabb))
				{
					aabb.UnTranslate(pos);

					Utils::OutlineAABB(aabb.TranslateCopy(fp.m_Start), COLOR::GREEN, 5000);
					Utils::OutlineAABB(aabb.TranslateCopy(fp.m_End), COLOR::RED, 5000);
					fp.m_NextRenderTime = IGame::GetTime() + 5000;
				}
			}
		}
	}
}
