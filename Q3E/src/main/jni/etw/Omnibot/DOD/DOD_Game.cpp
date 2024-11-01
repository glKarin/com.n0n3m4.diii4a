////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompDOD.h"
#include "DOD_Game.h"
#include "DOD_VoiceMacros.h"
#include "DOD_BaseStates.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"
#include "gmDODBinds.h"

IGame *CreateGameInstance()
{
	return new DOD_Game;
}

Client *DOD_Game::CreateGameClient()
{
	return new DOD_Client;
}

int DOD_Game::GetVersionNum() const
{
	return DOD_VERSION_LATEST;
}

const char *DOD_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_dod.dll";
#else
	return "omni-bot/omnibot_dod.so";
#endif
}

const char *DOD_Game::GetGameName() const
{
	return "Day Of Defeat";
}

const char *DOD_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "dod\\";
#else
	return "dod";
#endif
}

const char *DOD_Game::GetNavSubfolder() const
{
	return "dod\\nav\\";
}

const char *DOD_Game::GetScriptSubfolder() const
{
	return "dod\\scripts\\";
}
bool DOD_Game::Init() 
{
	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(DOD_Game::DOD_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(DOD_Game::DOD_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/dod_autoexec.gm", threadId);

	// Set up DOD specific data.
	using namespace AiState;

	return IGame::Init();
}

void DOD_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

void DOD_Game::InitScriptBinds(gmMachine *_machine)
{
	LOG("Binding DOD Library...");
	gmBindDODLibrary(_machine);
}

void DOD_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);
}

static IntEnum DOD_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("NONE",DOD_TEAM_NONE),
	IntEnum("AXIS",DOD_TEAM_AXIS),
	IntEnum("ALLIES",DOD_TEAM_ALLIES),
};

void DOD_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(DOD_TeamEnum) / sizeof(DOD_TeamEnum[0]);
	_ptr = DOD_TeamEnum;	
}

static IntEnum DOD_WeaponEnum[] =
{
	IntEnum("NONE",			DOD_WP_NONE),

	// shared
	IntEnum("SPADE",			DOD_WP_SPADE),
	IntEnum("AMERKNIFE",		DOD_WP_AMERKNIFE),
	IntEnum("FRAG_GER",		DOD_WP_FRAG_GER),
	IntEnum("FRAG_US",		DOD_WP_FRAG_US),

	// rifleman
	IntEnum("K98",			DOD_WP_K98),
	IntEnum("RIFLEGREN_GER",	DOD_WP_RIFLEGREN_GER),
	IntEnum("GARAND",			DOD_WP_GARAND),
	IntEnum("RIFLEGREN_US",	DOD_WP_RIFLEGREN_US),

	// assault
	IntEnum("MP40",			DOD_WP_MP40),
	IntEnum("P38",			DOD_WP_P38),
	IntEnum("SMOKE_GER",		DOD_WP_SMOKE_GER),
	IntEnum("THOMPSON",		DOD_WP_THOMPSON),
	IntEnum("COLT",			DOD_WP_COLT),
	IntEnum("SMOKE_US",		DOD_WP_SMOKE_US),

	// support
	IntEnum("MP44",			DOD_WP_MP44),
	IntEnum("BAR",			DOD_WP_BAR),

	// sniper
	IntEnum("K98S",			DOD_WP_K98S),
	IntEnum("SPRING",			DOD_WP_SPRING),

	// machinegunner
	IntEnum("MG42",			DOD_WP_MG42),
	IntEnum("30CAL",			DOD_WP_30CAL),

	// rocket
	IntEnum("PSCHRECK",		DOD_WP_PSCHRECK),
	IntEnum("C96",			DOD_WP_C96),
	IntEnum("BAZOOKA",		DOD_WP_BAZOOKA),
	IntEnum("M1CARBINE",		DOD_WP_M1CARBINE),
	
};

void DOD_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(DOD_WeaponEnum) / sizeof(DOD_WeaponEnum[0]);
	_ptr = DOD_WeaponEnum;	
}

void DOD_Game::InitVoiceMacros(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "MEDIC",			gmVariable(VCHAT_MEDIC));
	_table->Set(_machine, "THANKS",			gmVariable(VCHAT_THANKS));
	_table->Set(_machine, "GO",				gmVariable(VCHAT_GO));
	_table->Set(_machine, "MOVEUP",			gmVariable(VCHAT_MOVEUP));
	_table->Set(_machine, "FLANK_LEFT",		gmVariable(VCHAT_FLANK_LEFT));
	_table->Set(_machine, "FLANK_RIGHT",	gmVariable(VCHAT_FLANK_RIGHT));
	_table->Set(_machine, "YES",			gmVariable(VCHAT_YES));
	_table->Set(_machine, "NO",				gmVariable(VCHAT_NO));

	_table->Set(_machine, "INCOMING",		gmVariable(VCHAT_INCOMING));
	_table->Set(_machine, "SPY",			gmVariable(VCHAT_SPY));
	_table->Set(_machine, "SENTRY_AHEAD",	gmVariable(VCHAT_SENTRY_AHEAD));
	_table->Set(_machine, "TELEPORTER_HERE",gmVariable(VCHAT_TELEPORTER_HERE));
	_table->Set(_machine, "DISPENSER_HERE",	gmVariable(VCHAT_DISPENSER_HERE));
	_table->Set(_machine, "SENTRY_HERE",	gmVariable(VCHAT_SENTRY_HERE));
	_table->Set(_machine, "ACTIVATE_UBERCHARGE",gmVariable(VCHAT_ACTIVATE_UBERCHARGE));
	_table->Set(_machine, "UBERCHARGE_READY",gmVariable(VCHAT_UBERCHARGE_READY));

	_table->Set(_machine, "HELP",			gmVariable(VCHAT_HELP));
	_table->Set(_machine, "BATTLECRY",		gmVariable(VCHAT_BATTLECRY));
	_table->Set(_machine, "CHEERS",			gmVariable(VCHAT_CHEERS));
	_table->Set(_machine, "JEERS",			gmVariable(VCHAT_JEERS));
	_table->Set(_machine, "POSITIVE",		gmVariable(VCHAT_POSITIVE));
	_table->Set(_machine, "NEGATIVE",		gmVariable(VCHAT_NEGATIVE));
	_table->Set(_machine, "NICESHOT",		gmVariable(VCHAT_NICESHOT));
	_table->Set(_machine, "GOODJOB",		gmVariable(VCHAT_GOODJOB));
}

void DOD_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("AXIS",		F_NAV_TEAM1);
	_planner->RegisterNavFlag("ALLIES",		F_NAV_TEAM2);
	
	_planner->RegisterNavFlag("CAPPOINT",	F_DOD_NAV_CAPPOINT);	
}

void DOD_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	//_pfnPathCheck = DOD_PathCheck;
}

const float DOD_Game::DOD_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > DOD_CLASS_NONE && _class < DOD_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

const float DOD_Game::DOD_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > DOD_CLASS_NONE && _class < DOD_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

bool DOD_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	return true;
}

static IntEnum g_DODClassMappings[] =
{
	IntEnum("NONE",			DOD_CLASS_NONE),
	IntEnum("RIFLEMAN",		DOD_CLASS_RIFLEMAN),
	IntEnum("ASSAULT",		DOD_CLASS_ASSAULT),
	IntEnum("SUPPORT",		DOD_CLASS_SUPPORT),
	IntEnum("SNIPER",			DOD_CLASS_SNIPER),
	IntEnum("MACHINEGUNNER",	DOD_CLASS_MACHINEGUNNER),
	IntEnum("ROCKET",			DOD_CLASS_ROCKET),
	IntEnum("ANYPLAYER",		DOD_CLASS_ANY),
};

void DOD_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = DOD_CLASS_ANY;

	obint32 iNumMappings = sizeof(g_DODClassMappings) / sizeof(g_DODClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, g_DODClassMappings[i].m_Key, gmVariable(g_DODClassMappings[i].m_Value));
	}
}

void DOD_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);
	
	_table->Set(_machine, "SNIPE_AIMING",	gmVariable(ENT_FLAG_IRONSIGHT));
}