////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF2.h"
#include "TF2_Game.h"
#include "TF2_Client.h"
#include "TF2_VoiceMacros.h"
#include "TF2_BaseStates.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"
#include "gmTF2Binds.h"

IGame *CreateGameInstance()
{
	return new TF2_Game;
}

Client *TF2_Game::CreateGameClient()
{
	return new TF2_Client;
}

int TF2_Game::GetVersionNum() const
{
	return TF2_VERSION_LATEST;
}

const char *TF2_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_tf2.dll";
#else
	return "omni-bot/omnibot_tf2.so";
#endif
}

const char *TF2_Game::GetGameName() const
{
	return "Team Fortress 2";
}

const char *TF2_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "tf2\\";
#else
	return "tf2";
#endif
}

const char *TF2_Game::GetNavSubfolder() const
{
	return "tf2\\nav\\";
}

const char *TF2_Game::GetScriptSubfolder() const
{
	return "tf2\\scripts\\";
}

bool TF2_Game::Init() 
{
	if(!TF_Game::Init())
		return false;

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(TF2_Game::TF2_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(TF2_Game::TF2_GetEntityClassAimOffset);

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/tf2_autoexec.gm", threadId);

	// Set up TF2 specific data.
	using namespace AiState;
	//SentryBuild::BuildEquipWeapon = TF2_WP_ENGINEER_BUILDER;
	DispenserBuild::BuildEquipWeapon = TF2_WP_ENGINEER_BUILDER;
	TF_Options::BUILD_AMMO_TYPE = TF2_WP_ENGINEER_BUILDER;
	TF_Options::BUILD_ATTEMPT_DELAY = 0; // try every frame
	TF_Options::POLL_SENTRY_STATUS = true;
	TF_Options::SENTRY_UPGRADE_WPN = TF2_WP_MELEE;
	TF_Options::SENTRY_UPGRADE_AMMO = 25;
	TF_Options::PIPE_WEAPON = TF2_WP_PIPE_LAUNCHER;
	TF_Options::PIPE_WEAPON_WATCH = TF2_WP_GRENADE_LAUNCHER;
	TF_Options::PIPE_AMMO = TF2_WP_PIPE_LAUNCHER;
	TF_Options::ROCKETJUMP_WPN = TF2_WP_ROCKET_LAUNCHER;
	TF_Options::REPAIR_ON_SABOTAGED = true;

	return true;
}

void TF2_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 72.f;
}

void TF2_Game::InitScriptBinds(gmMachine *_machine)
{
	LOG("Binding TF2 Library...");
	gmBindTF2Library(_machine);
}

void TF2_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);
}

static IntEnum TF2_WeaponEnum[] =
{
	IntEnum("NONE",			TF_WP_NONE),

	// shared
	IntEnum("MELEE",			TF2_WP_MELEE),

	// scout
	IntEnum("SCATTERGUN",		TF2_WP_SCATTERGUN),
	IntEnum("PISTOL",			TF2_WP_PISTOL),

	// sniper
	IntEnum("SNIPERRIFLE",	TF2_WP_SNIPER_RIFLE),
	IntEnum("SMG",			TF2_WP_SMG),

	// soldier
	IntEnum("RPG",			TF2_WP_ROCKET_LAUNCHER),
	IntEnum("SHOTGUN",		TF2_WP_SHOTGUN),

	// demoman
	IntEnum("GRENADELAUNCHER",TF2_WP_GRENADE_LAUNCHER),
	IntEnum("PIPELAUNCHER",	TF2_WP_PIPE_LAUNCHER),

	// hwguy
	IntEnum("MINIGUN",			TF2_WP_MINIGUN),

	// medic
	IntEnum("SYRINGE_GUN",	TF2_WP_SYRINGE_GUN),
	IntEnum("MEDIGUN",		TF2_WP_MEDIGUN),

	// pyro
	IntEnum("FLAMETHROWER",	TF2_WP_FLAMETHROWER),

	// spy
	IntEnum("REVOLVER",		TF2_WP_REVOLVER),
	IntEnum("ELECTRO_SAPPER",	TF2_WP_ELECTRO_SAPPER),

	// engineer
	IntEnum("ENGINEER_BUILD",TF2_WP_ENGINEER_BUILD),
	IntEnum("ENGINEER_DESTROY",TF2_WP_ENGINEER_DESTROY),
	IntEnum("ENGINEER_BUILDER",TF2_WP_ENGINEER_BUILDER),	
};

void TF2_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(TF2_WeaponEnum) / sizeof(TF2_WeaponEnum[0]);
	_ptr = TF2_WeaponEnum;	
}

void TF2_Game::InitVoiceMacros(gmMachine *_machine, gmTableObject *_table)
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

void TF2_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("BLUE",		F_NAV_TEAM1);
	_planner->RegisterNavFlag("RED",		F_NAV_TEAM2);
	
	_planner->RegisterNavFlag("SENTRY",		F_TF_NAV_SENTRY);
	_planner->RegisterNavFlag("DISPENSER",	F_TF_NAV_DISPENSER);
	_planner->RegisterNavFlag("PIPETRAP",	F_TF_NAV_PIPETRAP);
	_planner->RegisterNavFlag("DETPACK",	F_TF_NAV_DETPACK);
	_planner->RegisterNavFlag("CAPPOINT",	F_TF_NAV_CAPPOINT);	
	//_planner->RegisterNavFlag("GRENADES",	F_TF_NAV_GRENADES);
	_planner->RegisterNavFlag("ROCKETJUMP",	F_TF_NAV_ROCKETJUMP);
	//_planner->RegisterNavFlag("CONCJUMP",	F_TF_NAV_CONCJUMP);
	_planner->RegisterNavFlag("WALL",		F_TF_NAV_WALL);
	_planner->RegisterNavFlag("TELE_ENTER",	F_TF_NAV_TELE_ENTER);
	_planner->RegisterNavFlag("TELE_EXIT",	F_TF_NAV_TELE_EXIT);
	_planner->RegisterNavFlag("DOUBLEJUMP",	F_TF_NAV_DOUBLEJUMP);
}

void TF2_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	//_pfnPathCheck = TF2_PathCheck;
}

const float TF2_Game::TF2_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > TF_CLASS_NONE && _class < TF_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

const float TF2_Game::TF2_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > TF_CLASS_NONE && _class < TF_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

bool TF2_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	return true;
}
