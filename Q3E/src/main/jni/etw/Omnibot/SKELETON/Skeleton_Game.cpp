////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompSkeleton.h"
#include "Skeleton_Game.h"
#include "Skeleton_Client.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"

IGame *CreateGameInstance()
{
	return new Skeleton_Game;
}

int Skeleton_Game::GetVersionNum() const
{
	return SKELETON_VERSION_LATEST;
}

Client *Skeleton_Game::CreateGameClient()
{
	return new Skeleton_Client;
}

const char *Skeleton_Game::GetDLLName() const
{
	return "omni-bot\\omnibot_skeleton.dll";
}

const char *Skeleton_Game::GetGameName() const
{
	return "Skeleton";
}

const char *Skeleton_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "skeleton\\";
#else
	return "skeleton";
#endif
}

const char *Skeleton_Game::GetNavSubfolder() const
{
	return "skeleton\\nav\\";
}

const char *Skeleton_Game::GetScriptSubfolder() const
{
	return "skeleton\\scripts\\";
}

bool Skeleton_Game::Init() 
{
	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(Skeleton_Game::Skeleton_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(Skeleton_Game::Skeleton_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/skeleton_autoexec.gm", threadId);

	return true;
}

void Skeleton_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 72.f;
}

static IntEnum Skel_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("TEAM1",SKELETON_TEAM_1),
	IntEnum("TEAM2",SKELETON_TEAM_2),
};

void Skeleton_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(Skel_TeamEnum) / sizeof(Skel_TeamEnum[0]);
	_ptr = Skel_TeamEnum;	
}

void Skeleton_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	
}

IntEnum g_SkeletonClassMappings[] =
{
	IntEnum("PLAYER",		SKELETON_CLASS_PLAYER),
	IntEnum("ANYPLAYER",	SKELETON_CLASS_ANY),
	IntEnum("GRENADE",		SKELETON_CLASSEX_GRENADE),
};

const char *Skeleton_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(g_SkeletonClassMappings) / sizeof(g_SkeletonClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(g_SkeletonClassMappings[i].m_Value == _classId)
			return g_SkeletonClassMappings[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void Skeleton_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = SKELETON_CLASS_ANY;

	obint32 iNumMappings = sizeof(g_SkeletonClassMappings) / sizeof(g_SkeletonClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, g_SkeletonClassMappings[i].m_Key, gmVariable(g_SkeletonClassMappings[i].m_Value));
	}
}

void Skeleton_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);
}

void Skeleton_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("TEAM1", SKELETON_TEAM_1);
	_planner->RegisterNavFlag("TEAM2", SKELETON_TEAM_2);
}

void Skeleton_Game::InitCommands()
{
	IGame::InitCommands();
}

const float Skeleton_Game::Skeleton_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > SKELETON_CLASS_NULL && _class < SKELETON_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

const float Skeleton_Game::Skeleton_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > SKELETON_CLASS_NULL && _class < SKELETON_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

PathPlannerWaypoint::BlockableStatus Skeleton_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	PathPlannerWaypoint::BlockableStatus res = PathPlannerWaypoint::B_INVALID_FLAGS;
	//BotTraceResult tr;

	//if(_wp1->IsFlagOn(F_FF_NAV_DETPACK) && _wp2->IsFlagOn(F_FF_NAV_DETPACK))
	//{
	//	EngineFuncs::TraceLine(tr, 
	//		(_wp1->GetPosition() + Vector3f(0,0,40)),
	//		(_wp2->GetPosition() + Vector3f(0,0,40)), 
	//		NULL, TR_MASK_SOLID, -1, True);

	//	return (tr.m_Fraction == 1.0f);
	//}
	//else
	//{
	//	DEBUG_ONLY(std::cout << "Invalid flag combination in PathCheck detected!" << std::endl);
	//}
	return res;
}

void Skeleton_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = Skeleton_PathCheck;
}
