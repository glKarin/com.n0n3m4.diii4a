////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF.h"
#include "TF_Game.h"
#include "TF_GoalManager.h"

#include "gmTFBinds.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"
#include "NameManager.h"

Client *TF_Game::CreateGameClient()
{
	return new TF_Client;
}

GoalManager *TF_Game::GetGoalManager()
{
	return new TF_GoalManager;
}

bool TF_Game::Init() 
{
	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(TF_Game::TF_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(TF_Game::TF_GetEntityClassAimOffset);

	PathPlannerWaypoint::m_CallbackFlags = 
		F_TF_NAV_ROCKETJUMP |
		F_TF_NAV_CONCJUMP |
		F_NAV_TELEPORT |
		F_TF_NAV_DOUBLEJUMP;

	PathPlannerWaypoint::m_BlockableMask = 
		F_TF_NAV_WALL |
		F_TF_NAV_DETPACK;

	return IGame::Init();
}

void TF_Game::Shutdown()
{
}

void TF_Game::InitScriptBinds(gmMachine *_machine)
{
	LOG("Binding TF Library...");
    gmBindTFLibrary(_machine);
}

static IntEnum TF_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),	
	IntEnum("NONE",TF_TEAM_NONE),
	IntEnum("RED",TF_TEAM_RED),
	IntEnum("BLUE",TF_TEAM_BLUE),
	IntEnum("YELLOW",TF_TEAM_YELLOW),
	IntEnum("GREEN",TF_TEAM_GREEN),
};

void TF_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(TF_TeamEnum) / sizeof(TF_TeamEnum[0]);
	_ptr = TF_TeamEnum;	
}

void TF_Game::InitScriptTeams(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "SPECTATOR",		gmVariable(OB_TEAM_SPECTATOR));
	_table->Set(_machine, "NONE",			gmVariable(TF_TEAM_NONE));
	_table->Set(_machine, "RED",			gmVariable(TF_TEAM_RED));
	_table->Set(_machine, "BLUE",			gmVariable(TF_TEAM_BLUE));
	_table->Set(_machine, "YELLOW",			gmVariable(TF_TEAM_YELLOW));
	_table->Set(_machine, "GREEN",			gmVariable(TF_TEAM_GREEN));
}

static IntEnum TF_WeaponEnum[] =
{
	IntEnum("NONE",				TF_WP_NONE),
	IntEnum("AXE",				TF_WP_AXE),
	IntEnum("CROWBAR",			TF_WP_CROWBAR),
	IntEnum("KNIFE",			TF_WP_KNIFE),
	IntEnum("MEDKIT",			TF_WP_MEDKIT),
	IntEnum("SPANNER",			TF_WP_SPANNER),
	IntEnum("SHOTGUN",			TF_WP_SHOTGUN),
	IntEnum("SUPERSHOTGUN",		TF_WP_SUPERSHOTGUN),
	IntEnum("NAILGUN",			TF_WP_NAILGUN),
	IntEnum("SUPERNAILGUN",		TF_WP_SUPERNAILGUN),
	IntEnum("GRENADELAUNCHER",	TF_WP_GRENADE_LAUNCHER),
	IntEnum("RPG",				TF_WP_ROCKET_LAUNCHER),
	IntEnum("SNIPERRIFLE",		TF_WP_SNIPER_RIFLE),
	IntEnum("RAILGUN",			TF_WP_RAILGUN),
	IntEnum("FLAMETHROWER",		TF_WP_FLAMETHROWER),
	IntEnum("ASSAULTCANNON",	TF_WP_MINIGUN),
	IntEnum("AUTORIFLE",		TF_WP_AUTORIFLE),
	IntEnum("TRANQGUN",			TF_WP_DARTGUN),
	IntEnum("PIPELAUNCHER",		TF_WP_PIPELAUNCHER),
	IntEnum("NAPALMCANNON",		TF_WP_NAPALMCANNON),
	IntEnum("UMBRELLA",			TF_WP_UMBRELLA),
	IntEnum("GRENADE1",			TF_WP_GRENADE1),
	IntEnum("GRENADE2",			TF_WP_GRENADE1),
	IntEnum("GRENADE",			TF_WP_GRENADE),
	IntEnum("GRENADE_CONC",		TF_WP_GRENADE_CONC),
	IntEnum("GRENADE_EMP",		TF_WP_GRENADE_EMP),
	IntEnum("GRENADE_NAIL",		TF_WP_GRENADE_NAIL),
	IntEnum("GRENADE_MIRV",		TF_WP_GRENADE_MIRV),
	IntEnum("GRENADE_GAS",		TF_WP_GRENADE_GAS),
	IntEnum("GRENADE_CALTROPS",TF_WP_GRENADE_CALTROPS),
	IntEnum("GRENADE_NAPALM",	TF_WP_GRENADE_NAPALM),
};

void TF_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(TF_WeaponEnum) / sizeof(TF_WeaponEnum[0]);
	_ptr = TF_WeaponEnum;	
}

void TF_Game::InitScriptCategories(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptCategories(_machine, _table);

	_table->Set(_machine, "BUILDABLE", gmVariable(TF_ENT_CAT_BUILDABLE));	
}

IntEnum g_TFClassMappings[] =
{
	IntEnum("NONE",				TF_CLASS_NONE),
	IntEnum("SCOUT",			TF_CLASS_SCOUT),
	IntEnum("SNIPER",			TF_CLASS_SNIPER),
	IntEnum("SOLDIER",			TF_CLASS_SOLDIER),
	IntEnum("DEMOMAN",			TF_CLASS_DEMOMAN),
	IntEnum("MEDIC",			TF_CLASS_MEDIC),
	IntEnum("HWGUY",			TF_CLASS_HWGUY),
	IntEnum("PYRO",				TF_CLASS_PYRO),
	IntEnum("SPY",				TF_CLASS_SPY),
	IntEnum("ENGINEER",			TF_CLASS_ENGINEER),
	IntEnum("CIVILIAN",			TF_CLASS_CIVILIAN),
	IntEnum("ANYPLAYER",		TF_CLASS_ANY),
	IntEnum("SENTRY",			TF_CLASSEX_SENTRY),
	IntEnum("DISPENSER",		TF_CLASSEX_DISPENSER),
	IntEnum("TELEPORTER_ENTRANCE",	TF_CLASSEX_TELEPORTER_ENTRANCE),	
	IntEnum("TELEPORTER_EXIT",		TF_CLASSEX_TELEPORTER_EXIT),
	IntEnum("DETPACK",			TF_CLASSEX_DETPACK),
	IntEnum("PIPEBOMB",			TF_CLASSEX_PIPE),
	IntEnum("GLGREN",			TF_CLASSEX_GLGRENADE),
	IntEnum("ROCKET",			TF_CLASSEX_ROCKET),
	IntEnum("NAPALM_ROCKET",	TF_CLASSEX_NAPALM),
	IntEnum("SYRINGE",			TF_CLASSEX_SYRINGE),	
	IntEnum("GRENADE",			TF_CLASSEX_GRENADE),
	IntEnum("EMP_GRENADE",		TF_CLASSEX_EMP_GRENADE),
	IntEnum("NAIL_GRENADE",		TF_CLASSEX_NAIL_GRENADE),
	IntEnum("MIRV_GRENADE",		TF_CLASSEX_MIRV_GRENADE),
	IntEnum("MIRVLET_GRENADE",	TF_CLASSEX_MIRVLET_GRENADE),
	IntEnum("NAPALM_GRENADE",	TF_CLASSEX_NAPALM_GRENADE),
	IntEnum("GAS_GRENADE",		TF_CLASSEX_GAS_GRENADE),
	IntEnum("CONC_GRENADE",		TF_CLASSEX_CONC_GRENADE),
	IntEnum("CALTROP",			TF_CLASSEX_CALTROP),
	IntEnum("TURRET",			TF_CLASSEX_TURRET),
	IntEnum("RESUPPLY",			TF_CLASSEX_RESUPPLY),	
	IntEnum("BACKPACK",			TF_CLASSEX_BACKPACK),
	IntEnum("BACKPACK_AMMO",	TF_CLASSEX_BACKPACK_AMMO),
	IntEnum("BACKPACK_HEALTH",	TF_CLASSEX_BACKPACK_HEALTH),
	IntEnum("BACKPACK_ARMOR",	TF_CLASSEX_BACKPACK_ARMOR),
	IntEnum("BACKPACK_GRENADES",TF_CLASSEX_BACKPACK_GRENADES),
	IntEnum("VEHICLE",			TF_CLASSEX_VEHICLE),
	IntEnum("VEHICLE_NODAMAGE",	TF_CLASSEX_VEHICLE_NODAMAGE),
};

const char *TF_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(g_TFClassMappings) / sizeof(g_TFClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(g_TFClassMappings[i].m_Value == _classId)
			return g_TFClassMappings[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void TF_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = TF_CLASS_ANY;

	obint32 iNumMappings = sizeof(g_TFClassMappings) / sizeof(g_TFClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, g_TFClassMappings[i].m_Key, gmVariable(g_TFClassMappings[i].m_Value));
	}
}

void TF_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);
	
	_table->Set(_machine, "NEED_HEALTH",	gmVariable(TF_ENT_FLAG_SAVEME));
	_table->Set(_machine, "NEED_ARMOR",		gmVariable(TF_ENT_FLAG_ARMORME));
	_table->Set(_machine, "BURNING",		gmVariable(TF_ENT_FLAG_BURNING));
	_table->Set(_machine, "TRANQUED",		gmVariable(TF_ENT_FLAG_TRANQED));
	_table->Set(_machine, "INFECTED",		gmVariable(TF_ENT_FLAG_INFECTED));
	_table->Set(_machine, "GASSED",			gmVariable(TF_ENT_FLAG_GASSED));
	_table->Set(_machine, "SNIPE_AIMING",	gmVariable(ENT_FLAG_IRONSIGHT));
	_table->Set(_machine, "AC_FIRING",		gmVariable(TF_ENT_FLAG_ASSAULTFIRING));
	_table->Set(_machine, "LEGSHOT",		gmVariable(TF_ENT_FLAG_LEGSHOT));
	_table->Set(_machine, "CALTROP",		gmVariable(TF_ENT_FLAG_CALTROP));
	_table->Set(_machine, "RADIOTAGGED",	gmVariable(TF_ENT_FLAG_RADIOTAGGED));
	_table->Set(_machine, "CAN_SABOTAGE",	gmVariable(TF_ENT_FLAG_CAN_SABOTAGE));
	_table->Set(_machine, "SABOTAGED",		gmVariable(TF_ENT_FLAG_SABOTAGED));
	_table->Set(_machine, "SABOTAGING",		gmVariable(TF_ENT_FLAG_SABOTAGING));
	_table->Set(_machine, "BUILDING_SG",	gmVariable(TF_ENT_FLAG_BUILDING_SG));
	_table->Set(_machine, "BUILDING_DISP",	gmVariable(TF_ENT_FLAG_BUILDING_DISP));
	_table->Set(_machine, "BUILDING_DETP",	gmVariable(TF_ENT_FLAG_BUILDING_DETP));
	_table->Set(_machine, "BUILDINPROGRESS",gmVariable(TF_ENT_FLAG_BUILDINPROGRESS));
	_table->Set(_machine, "LEVEL2",			gmVariable(TF_ENT_FLAG_LEVEL2));
	_table->Set(_machine, "LEVEL3",			gmVariable(TF_ENT_FLAG_LEVEL3));
}

void TF_Game::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "DISGUISE_BLUE",		gmVariable(TF_PWR_DISGUISE_BLUE));
	_table->Set(_machine, "DISGUISE_RED",		gmVariable(TF_PWR_DISGUISE_RED));
	_table->Set(_machine, "DISGUISE_YELLOW",	gmVariable(TF_PWR_DISGUISE_YELLOW));
	_table->Set(_machine, "DISGUISE_GREEN",		gmVariable(TF_PWR_DISGUISE_GREEN));
	_table->Set(_machine, "DISGUISE_SCOUT",		gmVariable(TF_PWR_DISGUISE_SCOUT));
	_table->Set(_machine, "DISGUISE_SNIPER",	gmVariable(TF_PWR_DISGUISE_SNIPER));
	_table->Set(_machine, "DISGUISE_SOLDIER",	gmVariable(TF_PWR_DISGUISE_SOLDIER));
	_table->Set(_machine, "DISGUISE_DEMOMAN",	gmVariable(TF_PWR_DISGUISE_DEMOMAN));
	_table->Set(_machine, "DISGUISE_MEDIC",		gmVariable(TF_PWR_DISGUISE_MEDIC));
	_table->Set(_machine, "DISGUISE_HWGUY",		gmVariable(TF_PWR_DISGUISE_HWGUY));
	_table->Set(_machine, "DISGUISE_PYRO",		gmVariable(TF_PWR_DISGUISE_PYRO));
	_table->Set(_machine, "DISGUISE_ENGINEER",	gmVariable(TF_PWR_DISGUISE_ENGINEER));
	_table->Set(_machine, "DISGUISE_SPY",		gmVariable(TF_PWR_DISGUISE_SPY));
	_table->Set(_machine, "DISGUISE_CIVILIAN",	gmVariable(TF_PWR_DISGUISE_CIVILIAN));	
}

// package: TF Script Events
//		Custom Events for Team Fortress Games. Also see <Common Script Events>
void TF_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);

	_table->Set(_machine, "GOT_ENGY_ARMOR",		gmVariable(TF_MSG_GOT_ENGY_ARMOR));
	_table->Set(_machine, "GAVE_ENGY_ARMOR",	gmVariable(TF_MSG_GAVE_ENGY_ARMOR));
	_table->Set(_machine, "GOT_MEDIC_HEALTH",	gmVariable(TF_MSG_GOT_MEDIC_HEALTH));
	_table->Set(_machine, "GAVE_MEDIC_HEALTH",	gmVariable(TF_MSG_GAVE_MEDIC_HEALTH));
	_table->Set(_machine, "GOT_DISPENSER_AMMO",	gmVariable(TF_MSG_GOT_DISPENSER_AMMO));
	_table->Set(_machine, "INFECTED",			gmVariable(TF_MSG_INFECTED));
	_table->Set(_machine, "CURED",				gmVariable(TF_MSG_CURED));
	_table->Set(_machine, "BURNED",				gmVariable(TF_MSG_BURNLEVEL));

	// General Events
	_table->Set(_machine, "CLASS_DISABLE",		gmVariable(TF_MSG_CLASS_DISABLED));
	_table->Set(_machine, "CLASS_NOTAVAILABLE", gmVariable(TF_MSG_CLASS_NOTAVAILABLE));
	_table->Set(_machine, "CLASS_CHANGELATER",	gmVariable(TF_MSG_CLASS_CHANGELATER));

	// event: BUILD_NOTONGROUND
	//		The bot must be on ground to build(currently isn't).
	_table->Set(_machine, "BUILD_NOTONGROUND",	gmVariable(TF_MSG_BUILD_MUSTBEONGROUND));	
	// event: RADAR_DETECTENEMY
	//		Radar detected enemy entity.
	//
	// Parameters:
	//		detected - entity who was detected.
	_table->Set(_machine, "RADAR_DETECTENEMY",	gmVariable(TF_MSG_RADAR_DETECT_ENEMY));
	// event: RADIOTAG_UPDATE
	//		Radio tag detected enemy entity.
	//
	// Parameters:
	//		detected - entity who was detected.
	_table->Set(_machine, "RADIOTAG_UPDATE",	gmVariable(TF_MSG_RADIOTAG_UPDATE));

	// Demo-man
	_table->Set(_machine, "DETPACK_BUILDING",	gmVariable(TF_MSG_DETPACK_BUILDING));
	_table->Set(_machine, "DETPACK_BUILT",		gmVariable(TF_MSG_DETPACK_BUILT));
	_table->Set(_machine, "DETPACK_NOAMMO",		gmVariable(TF_MSG_DETPACK_NOTENOUGHAMMO));
	_table->Set(_machine, "DETPACK_CANTBUILD",	gmVariable(TF_MSG_DETPACK_CANTBUILD));
	_table->Set(_machine, "DETPACK_ALREADYBUILT",gmVariable(TF_MSG_DETPACK_ALREADYBUILT));
	_table->Set(_machine, "DETPACK_DETONATED",	gmVariable(TF_MSG_DETPACK_DETONATED));

	_table->Set(_machine, "PIPE_PROXIMITY",		gmVariable(TF_MSG_PIPE_PROXIMITY));
	
	//TF_MSG_DETPIPES,		// The bot has detected the desire to det pipes.
	//TF_MSG_DETPIPESNOW,		// Configurable delayed message for the actual detting.
	//TF_MSG_DEMOMAN_END,

	// Medic
	_table->Set(_machine, "MEDIC_CALLED",		gmVariable(TF_MSG_CALLFORMEDIC));

	// HW-Guy
	
	// Pyro
	
	// Spy
	_table->Set(_machine, "INVALID_DISGUISE_TEAM",gmVariable(TF_MSG_CANTDISGUISE_AS_TEAM));
	_table->Set(_machine, "INVALID_DISGUISE_CLASS",gmVariable(TF_MSG_CANTDISGUISE_AS_CLASS));
	_table->Set(_machine, "DISGUISING",			gmVariable(TF_MSG_DISGUISING));
	_table->Set(_machine, "DISGUISED",			gmVariable(TF_MSG_DISGUISED));
	_table->Set(_machine, "DISGUISE_LOST",		gmVariable(TF_MSG_DISGUISE_LOST));
	_table->Set(_machine, "CANT_CLOAK",			gmVariable(TF_MSG_CANT_CLOAK));
	_table->Set(_machine, "CLOAKED",			gmVariable(TF_MSG_CLOAKED));
	_table->Set(_machine, "UNCLOAKED",			gmVariable(TF_MSG_UNCLOAKED));
	_table->Set(_machine, "SABOTAGED_SENTRY",	gmVariable(TF_MSG_SABOTAGED_SENTRY));
	_table->Set(_machine, "SABOTAGED_DISPENSER",gmVariable(TF_MSG_SABOTAGED_DISPENSER));
	
	// Engineer
	_table->Set(_machine, "SENTRY_NOAMMO",		gmVariable(TF_MSG_SENTRY_NOTENOUGHAMMO));
	_table->Set(_machine, "SENTRY_ALREADYBUILT",gmVariable(TF_MSG_SENTRY_ALREADYBUILT));
	_table->Set(_machine, "SENTRY_CANTBUILD",	gmVariable(TF_MSG_SENTRY_CANTBUILD));
	_table->Set(_machine, "SENTRY_BUILDING",	gmVariable(TF_MSG_SENTRY_BUILDING));
	_table->Set(_machine, "SENTRY_BUILT",		gmVariable(TF_MSG_SENTRY_BUILT));
	_table->Set(_machine, "SENTRY_DESTROYED",	gmVariable(TF_MSG_SENTRY_DESTROYED));
	_table->Set(_machine, "SENTRY_SPOTENEMY",	gmVariable(TF_MSG_SENTRY_SPOTENEMY));
	_table->Set(_machine, "SENTRY_AIMED",		gmVariable(TF_MSG_SENTRY_AIMED));
	_table->Set(_machine, "SENTRY_DAMAGED",		gmVariable(TF_MSG_SENTRY_DAMAGED));
	_table->Set(_machine, "SENTRY_STATS",		gmVariable(TF_MSG_SENTRY_STATS));
	_table->Set(_machine, "SENTRY_UPGRADED",	gmVariable(TF_MSG_SENTRY_UPGRADED));
	_table->Set(_machine, "SENTRY_DETONATED",	gmVariable(TF_MSG_SENTRY_DETONATED));
	_table->Set(_machine, "SENTRY_DISMANTLED",	gmVariable(TF_MSG_SENTRY_DISMANTLED));

	_table->Set(_machine, "DISPENSER_NOAMMO",	gmVariable(TF_MSG_DISPENSER_NOTENOUGHAMMO));
	_table->Set(_machine, "DISPENSER_ALREADYBUILT",gmVariable(TF_MSG_DISPENSER_ALREADYBUILT));
	_table->Set(_machine, "DISPENSER_CANTBUILD",gmVariable(TF_MSG_DISPENSER_CANTBUILD));
	_table->Set(_machine, "DISPENSER_BUILDING",	gmVariable(TF_MSG_DISPENSER_BUILDING));
	_table->Set(_machine, "DISPENSER_BUILT",	gmVariable(TF_MSG_DISPENSER_BUILT));
	_table->Set(_machine, "DISPENSER_DESTROYED",gmVariable(TF_MSG_DISPENSER_DESTROYED));
	_table->Set(_machine, "DISPENSER_ENEMYUSED",gmVariable(TF_MSG_DISPENSER_ENEMYUSED));
	_table->Set(_machine, "DISPENSER_DAMAGED",	gmVariable(TF_MSG_DISPENSER_DAMAGED));
	_table->Set(_machine, "DISPENSER_STATS",	gmVariable(TF_MSG_DISPENSER_STATS));
	_table->Set(_machine, "DISPENSER_DETONATED",gmVariable(TF_MSG_DISPENSER_DETONATED));
	_table->Set(_machine, "DISPENSER_DISMANTLED",gmVariable(TF_MSG_DISPENSER_DISMANTLED));
		
	// Civilian
	
}

void TF_Game::InitScriptBotButtons(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptBotButtons(_machine, _table);

	_table->Set(_machine, "GREN1",	gmVariable(TF_BOT_BUTTON_GREN1));
	_table->Set(_machine, "GREN2",	gmVariable(TF_BOT_BUTTON_GREN2));
	
	_table->Set(_machine, "DROP_ITEM",gmVariable(TF_BOT_BUTTON_DROPITEM));
	_table->Set(_machine, "DROP_AMMO",gmVariable(TF_BOT_BUTTON_DROPAMMO));

	_table->Set(_machine, "BUILDSENTRY",gmVariable(TF_BOT_BUTTON_BUILDSENTRY));
	_table->Set(_machine, "BUILDDISPENSER",gmVariable(TF_BOT_BUTTON_BUILDDISPENSER));
	_table->Set(_machine, "BUILDDETPACK_5",gmVariable(TF_BOT_BUTTON_BUILDDETPACK_5));
	_table->Set(_machine, "BUILDDETPACK_10",gmVariable(TF_BOT_BUTTON_BUILDDETPACK_10));
	_table->Set(_machine, "BUILDDETPACK_20",gmVariable(TF_BOT_BUTTON_BUILDDETPACK_20));
	_table->Set(_machine, "BUILDDETPACK_30",gmVariable(TF_BOT_BUTTON_BUILDDETPACK_30));

	_table->Set(_machine, "AIMSENTRY",gmVariable(TF_BOT_BUTTON_AIMSENTRY));

	_table->Set(_machine, "DETSENTRY",gmVariable(TF_BOT_BUTTON_DETSENTRY));
	_table->Set(_machine, "DETDISPENSER",gmVariable(TF_BOT_BUTTON_DETDISPENSER));
	_table->Set(_machine, "DETPIPES",gmVariable(TF_BOT_BUTTON_DETPIPES));

	_table->Set(_machine, "CALLFORMEDIC",gmVariable(TF_BOT_BUTTON_CALLFORMEDIC));
	_table->Set(_machine, "CALLFORENGY",gmVariable(TF_BOT_BUTTON_CALLFORENGY));

	_table->Set(_machine, "SABOTAGE_SENTRY",gmVariable(TF_BOT_BUTTON_SABOTAGE_SENTRY));
	_table->Set(_machine, "SABOTAGE_DISPENSER",gmVariable(TF_BOT_BUTTON_SABOTAGE_DISPENSER));

	_table->Set(_machine, "CLOAK",gmVariable(TF_BOT_BUTTON_CLOAK));
	_table->Set(_machine, "SILENT_CLOAK",gmVariable(TF_BOT_BUTTON_SILENT_CLOAK));

	_table->Set(_machine, "RADAR",gmVariable(TF_BOT_BUTTON_RADAR));
}

void TF_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("BLUE",		F_NAV_TEAM1);
	_planner->RegisterNavFlag("RED",		F_NAV_TEAM2);
	_planner->RegisterNavFlag("YELLOW",		F_NAV_TEAM3);
	_planner->RegisterNavFlag("GREEN",		F_NAV_TEAM4);
	_planner->RegisterNavFlag("SENTRY",		F_TF_NAV_SENTRY);
	_planner->RegisterNavFlag("DISPENSER",	F_TF_NAV_DISPENSER);
	_planner->RegisterNavFlag("PIPETRAP",	F_TF_NAV_PIPETRAP);
	_planner->RegisterNavFlag("DETPACK",	F_TF_NAV_DETPACK);
	_planner->RegisterNavFlag("CAPPOINT",	F_TF_NAV_CAPPOINT);	
	_planner->RegisterNavFlag("GRENADES",	F_TF_NAV_GRENADES);
	_planner->RegisterNavFlag("ROCKETJUMP",	F_TF_NAV_ROCKETJUMP);
	_planner->RegisterNavFlag("CONCJUMP",	F_TF_NAV_CONCJUMP);
	_planner->RegisterNavFlag("WALL",		F_TF_NAV_WALL);
}

const float TF_Game::TF_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if(InRangeT<int>(_class, TF_TEAM_NONE, TF_CLASS_MAX))
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 0.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 0.0f;
		return 0.0f;
	}
	switch(_class)
	{
	case TF_CLASSEX_SENTRY:
	case TF_CLASSEX_DISPENSER:
		return 32.f;
	case TF_CLASSEX_BACKPACK:
	case TF_CLASSEX_BACKPACK_AMMO:
	case TF_CLASSEX_BACKPACK_HEALTH:
	case TF_CLASSEX_BACKPACK_ARMOR:
	case TF_CLASSEX_BACKPACK_GRENADES:
		return 32.0f;
	}
	return 0.0f;
}

const float TF_Game::TF_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if(InRangeT<int>(_class, TF_TEAM_NONE, TF_CLASS_MAX))
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 0.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 0.0f;
	}
	switch(_class)
	{
	case TF_CLASSEX_SENTRY:
	case TF_CLASSEX_DISPENSER:
		return 32.f;
	case TF_CLASSEX_BACKPACK:
	case TF_CLASSEX_BACKPACK_AMMO:
	case TF_CLASSEX_BACKPACK_HEALTH:
	case TF_CLASSEX_BACKPACK_ARMOR:
	case TF_CLASSEX_BACKPACK_GRENADES:
		return 32.0f;
	}
	return 0.0f;
}

extern float g_fBottomWaypointOffset;
void TF_Game::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	IGame::ProcessEvent(_message,_cb);
	
	PathPlannerBase *pPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
	if(pPlanner->IsViewOn() && pPlanner->IsAutoDetectFlagsOn())
	{
		switch(_message.GetMessageId())
		{
		case GAME_ENTITYCREATED:
			{
				const Event_EntityCreated *m = _message.Get<Event_EntityCreated>();
				if(m)
				{
					if(m->m_EntityClass == TF_CLASSEX_SENTRY ||
						m->m_EntityClass == TF_CLASSEX_DISPENSER ||
						m->m_EntityClass == TF_CLASSEX_DETPACK)
					{
						Vector3f vEntPosition, vEntFacing, vWpPosition, vWpFacing;
						EngineFuncs::EntityPosition(m->m_Entity, vEntPosition);
						EngineFuncs::EntityOrientation(m->m_Entity, vEntFacing, NULL, NULL);

						EngineFuncs::EntityPosition(Utils::GetLocalEntity(), vWpPosition);
						EngineFuncs::EntityOrientation(Utils::GetLocalEntity(), vWpFacing, NULL, NULL);
						
						//Utils::GetLocalEntity();
						//Utils::DrawLine(GetClient()->GetEyePosition(), _aimpos, COLOR::GREEN, 20.f);

						// Auto Nav
						if(pPlanner->GetPlannerType() == NAVID_WP)
						{
							vEntPosition.z -= g_fBottomWaypointOffset;
							//vWpPosition.z -= g_fBottomWaypointOffset;

							PathPlannerWaypoint *pWp = static_cast<PathPlannerWaypoint*>(pPlanner);
							Waypoint *pWaypoint = pWp->AddWaypoint(vWpPosition, vWpFacing, true);
							
							pWaypoint->GetPropertyMap().AddPropertyT("BuildPosition", vEntPosition);

							if(m->m_EntityClass == TF_CLASSEX_SENTRY)
							{
								Vector3f vAimPos = vEntPosition+vEntFacing*4096.f;
								pWaypoint->GetPropertyMap().AddPropertyT("AimPoint", vAimPos);
								pWaypoint->AddFlag(F_TF_NAV_SENTRY);
							}
							if(m->m_EntityClass == TF_CLASSEX_DISPENSER)
							{
								pWaypoint->AddFlag(F_TF_NAV_DISPENSER);
							}
							if(m->m_EntityClass == TF_CLASSEX_DETPACK)
							{
								pWaypoint->AddFlag(F_TF_NAV_DETPACK);
							}
							if(m->m_EntityClass == TF_CLASSEX_TELEPORTER_ENTRANCE)
							{
								pWaypoint->AddFlag(F_TF_NAV_TELE_ENTER);
							}
							if(m->m_EntityClass == TF_CLASSEX_TELEPORTER_EXIT)
							{
								pWaypoint->AddFlag(F_TF_NAV_TELE_EXIT);
							}

							ClientPtr cl(new TF_Client);
							NavFlags teamFlags = cl->GetTeamFlag(g_EngineFuncs->GetEntityTeam(m->m_Entity));
							if(teamFlags)
								pWaypoint->AddFlag(teamFlags);
						}
					}
				}
				break;
			}	
		case GAME_ENTITYDELETED:
			{
				const Event_EntityDeleted *m = _message.Get<Event_EntityDeleted>();
				if(m)
				{
					/*int index = m->m_Entity.GetIndex();
					if(m_GameEntities[index].m_Entity.IsValid())
					{
#ifdef _DEBUG
						const char *pClassName = FindClassName(m_GameEntities[index].m_EntityClass);
						Utils::OutputDebug(kNormal, "Entity: %d deleted: %s\n", index, pClassName?pClassName:"<unknown>");
#endif
						m_GameEntities[index].m_Entity.Reset();
						m_GameEntities[index].m_EntityClass = 0;
						m_GameEntities[index].m_EntityCategory.ClearAll();
					}

					GoalManager::GetInstance()->RemoveGoalByEntity(m->m_Entity);*/
				}
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
}

PathPlannerWaypoint::BlockableStatus TF_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	static bool bRender = false;
	PathPlannerWaypoint::BlockableStatus res = PathPlannerWaypoint::B_INVALID_FLAGS;

	Vector3f vStart, vEnd;

	if(/*_wp1->IsFlagOn(F_TF_NAV_WALL) &&*/ _wp2->IsFlagOn(F_TF_NAV_WALL))
	{
		static float fOffset = 0.0f;
		static Vector3f vMins(-5.f, -5.f, -5.f), vMaxs(5.f, 5.f, 5.f);
		AABB aabb(vMins, vMaxs);
		vStart = _wp1->GetPosition() + Vector3f(0, 0, fOffset);
		vEnd = _wp2->GetPosition() + Vector3f(0, 0, fOffset);

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStart, vEnd, &aabb, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);
		res = (tr.m_Fraction == 1.0f) ? PathPlannerWaypoint::B_PATH_OPEN : PathPlannerWaypoint::B_PATH_CLOSED;
	}
	else if(_wp1->IsFlagOn(F_TF_NAV_DETPACK) && _wp2->IsFlagOn(F_TF_NAV_DETPACK))
	{
		Vector3f vStart = _wp1->GetPosition() + Vector3f(0, 0, 40.0f);
		Vector3f vEnd = _wp2->GetPosition() + Vector3f(0, 0, 40.0f);

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStart, vEnd, NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);
		res = (tr.m_Fraction == 1.0f) ? PathPlannerWaypoint::B_PATH_OPEN : PathPlannerWaypoint::B_PATH_CLOSED;
	}
	return res;
}

void TF_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = TF_PathCheck;
}

//////////////////////////////////////////////////////////////////////////

obReal TF_Game::_GetDesirabilityFromTargetClass(int _grentype, int _class)
{
	switch(_grentype)
	{
	case TF_WP_GRENADE:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.4f;
			case TF_CLASS_SNIPER: return 0.4f;
			case TF_CLASS_SOLDIER: return 0.55f;
			case TF_CLASS_DEMOMAN: return 0.5f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.65f;
			case TF_CLASS_PYRO: return 0.45f;
			case TF_CLASS_SPY: return 0.5f;
			case TF_CLASS_ENGINEER: return 0.4f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.5f;
			case TF_CLASSEX_DISPENSER: return 0.5f;
			default: return 0.f;
			}
		}
	case TF_WP_GRENADE_CONC:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.4f;
			case TF_CLASS_SNIPER: return 0.55f;
			case TF_CLASS_SOLDIER: return 0.5f;
			case TF_CLASS_DEMOMAN: return 0.5f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.7f;
			case TF_CLASS_PYRO: return 0.45f;
			case TF_CLASS_SPY: return 0.5f;
			case TF_CLASS_ENGINEER: return 0.5f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.f;
			case TF_CLASSEX_DISPENSER: return 0.f;
			default: return 0.f;
			}
		}
	case TF_WP_GRENADE_EMP:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.4f;
			case TF_CLASS_SNIPER: return 0.55f;
			case TF_CLASS_SOLDIER: return 0.5f;
			case TF_CLASS_DEMOMAN: return 0.5f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.65f;
			case TF_CLASS_PYRO: return 0.55f;
			case TF_CLASS_SPY: return 0.5f;
			case TF_CLASS_ENGINEER: return 0.4f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.6f;
			case TF_CLASSEX_DISPENSER: return 0.6f;
			default: return 0.f;
			}
		}
	case TF_WP_GRENADE_NAIL:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.4f;
			case TF_CLASS_SNIPER: return 0.35f;
			case TF_CLASS_SOLDIER: return 0.55f;
			case TF_CLASS_DEMOMAN: return 0.5f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.65f;
			case TF_CLASS_PYRO: return 0.45f;
			case TF_CLASS_SPY: return 0.4f;
			case TF_CLASS_ENGINEER: return 0.4f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.6f;
			case TF_CLASSEX_DISPENSER: return 0.6f;
			default: return 0.f;
			}
		}
	case TF_WP_GRENADE_MIRV:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.25f;
			case TF_CLASS_SNIPER: return 0.35f;
			case TF_CLASS_SOLDIER: return 0.55f;
			case TF_CLASS_DEMOMAN: return 0.5f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.65f;
			case TF_CLASS_PYRO: return 0.45f;
			case TF_CLASS_SPY: return 0.4f;
			case TF_CLASS_ENGINEER: return 0.4f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.6f;
			case TF_CLASSEX_DISPENSER: return 0.6f;
			default: return 0.f;
			}
		}
	case TF_WP_GRENADE_GAS:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.2f;
			case TF_CLASS_SNIPER: return 0.4f;
			case TF_CLASS_SOLDIER: return 0.45f;
			case TF_CLASS_DEMOMAN: return 0.45f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.55f;
			case TF_CLASS_PYRO: return 0.45f;
			case TF_CLASS_SPY: return 0.5f;
			case TF_CLASS_ENGINEER: return 0.4f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.f;
			case TF_CLASSEX_DISPENSER: return 0.f;
			default: return 0.f;
			}
		}
	case TF_WP_GRENADE_CALTROPS:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.4f;
			case TF_CLASS_SNIPER: return 0.4f;
			case TF_CLASS_SOLDIER: return 0.55f;
			case TF_CLASS_DEMOMAN: return 0.5f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.65f;
			case TF_CLASS_PYRO: return 0.45f;
			case TF_CLASS_SPY: return 0.5f;
			case TF_CLASS_ENGINEER: return 0.4f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.f;
			case TF_CLASSEX_DISPENSER: return 0.f;
			default: return 0.f;
			}
		}
	case TF_WP_GRENADE_NAPALM:
		{
			switch(_class)
			{
			case TF_CLASS_SCOUT: return 0.4f;
			case TF_CLASS_SNIPER: return 0.4f;
			case TF_CLASS_SOLDIER: return 0.55f;
			case TF_CLASS_DEMOMAN: return 0.5f;
			case TF_CLASS_MEDIC: return 0.45f;
			case TF_CLASS_HWGUY: return 0.65f;
			case TF_CLASS_PYRO: return 0.45f;
			case TF_CLASS_SPY: return 0.5f;
			case TF_CLASS_ENGINEER: return 0.4f;
			case TF_CLASS_CIVILIAN: return 0.3f;
			case TF_CLASSEX_SENTRY: return 0.2f;
			case TF_CLASSEX_DISPENSER: return 0.2f;
			default: return 0.f;
			}
		}
	}
	return 0.f;
}
