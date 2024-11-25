////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompWOLF.h"
#include "WOLF_Game.h"
#include "WOLF_Client.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"

IGame *CreateGameInstance()
{
	return new WOLF_Game;
}

int WOLF_Game::GetVersionNum() const
{
	return WOLF_VERSION_CURRENT;
}

Client *WOLF_Game::CreateGameClient()
{
	return new WOLF_Client;
}

const char *WOLF_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_wolf.dll";
#else
	return "omni-bot/omnibot_wolf.so";
#endif
	
}

const char *WOLF_Game::GetGameName() const
{
	return "WOLF";
}

const char *WOLF_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "WOLF\\";
#else
	return "WOLF";
#endif
}

const char *WOLF_Game::GetNavSubfolder() const
{
	return "WOLF\\nav\\";
}

const char *WOLF_Game::GetScriptSubfolder() const
{
	return "WOLF\\scripts\\";
}

bool WOLF_Game::Init() 
{
	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(WOLF_Game::WOLF_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(WOLF_Game::WOLF_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/WOLF_autoexec.gm", threadId);

	return true;
}

void WOLF_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 72.f;
}

static IntEnum WOLF_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("ALLIES",WOLF_ALLIES),
	IntEnum("AXIS",WOLF_AXIS),
};

void WOLF_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(WOLF_TeamEnum) / sizeof(WOLF_TeamEnum[0]);
	_ptr = WOLF_TeamEnum;	
}

static IntEnum WOLF_WeaponEnum[] =
{
	IntEnum("FLAMETHROWER",WOLF_WP_FLAMETHROWER),
	IntEnum("PANZERSCHRECK",WOLF_WP_PANZERSCHRECK),
	IntEnum("PARTICLECANNON",WOLF_WP_PARTICLECANNON),
	IntEnum("TESLACANNON",WOLF_WP_TESLACANNON),
	IntEnum("MP40",WOLF_WP_MP40),
	IntEnum("KAR98",WOLF_WP_KAR98),
	IntEnum("KAR98",WOLF_WP_MP43),
};

void WOLF_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(WOLF_WeaponEnum) / sizeof(WOLF_WeaponEnum[0]);
	_ptr = WOLF_WeaponEnum;	
}

static IntEnum g_WOLFClassMappings[] =
{
	IntEnum("ENGINEER",			WOLF_CLASS_ENGINEER),
	IntEnum("MEDIC",			WOLF_CLASS_MEDIC),
	IntEnum("SOLDIER",			WOLF_CLASS_SOLDIER),
	IntEnum("ANYPLAYER",		WOLF_CLASS_ANY),	
};

const char *WOLF_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(g_WOLFClassMappings) / sizeof(g_WOLFClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(g_WOLFClassMappings[i].m_Value == _classId)
			return g_WOLFClassMappings[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void WOLF_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = WOLF_CLASS_ANY;

	obint32 iNumMappings = sizeof(g_WOLFClassMappings) / sizeof(g_WOLFClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, g_WOLFClassMappings[i].m_Key, gmVariable(g_WOLFClassMappings[i].m_Value));
	}
}

void WOLF_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);
}

void WOLF_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("TEAM1", F_NAV_TEAM1);
	_planner->RegisterNavFlag("TEAM2", F_NAV_TEAM2);
}

void WOLF_Game::InitCommands()
{
	IGame::InitCommands();
}

const float WOLF_Game::WOLF_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > WOLF_CLASS_NULL && _class < WOLF_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

const float WOLF_Game::WOLF_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > WOLF_CLASS_NULL && _class < WOLF_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

PathPlannerWaypoint::BlockableStatus WOLF_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
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

void WOLF_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = WOLF_PathCheck;
}
