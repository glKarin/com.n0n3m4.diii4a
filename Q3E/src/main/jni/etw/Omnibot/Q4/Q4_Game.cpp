////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompQ4.h"
#include "Q4_Game.h"
#include "Q4_Client.h"
#include "Q4_GoalManager.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"
#include "gmQ4Binds.h"

IGame *CreateGameInstance()
{
	return new Q4_Game;
}

Client *Q4_Game::CreateGameClient()
{
	return new Q4_Client;
}

int Q4_Game::GetVersionNum() const
{
	return Q4_VERSION_LATEST;
}

const char *Q4_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_q4.dll";
#else
	return "omni-bot/omnibot_q4.so";
#endif
}

const char *Q4_Game::GetGameName() const
{
	return "Quake4";
}

const char *Q4_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "q4\\";
#else
	return "q4";
#endif
}

const char *Q4_Game::GetNavSubfolder() const
{
#ifdef WIN32
	return "q4\\nav\\";
#else
	return "q4/nav";
#endif
}

const char *Q4_Game::GetScriptSubfolder() const
{
#ifdef WIN32
	return "q4\\scripts\\";
#else
	return "q4/scripts";
#endif
}

eNavigatorID Q4_Game::GetDefaultNavigator() const 
{
	//return NAVID_RECAST;
	return NAVID_WP; 
}

GoalManager *Q4_Game::GetGoalManager()
{
	return new Q4_GoalManager;
}

bool Q4_Game::Init() 
{
	SetRenderOverlayType(OVERLAY_OPENGL);

#ifdef ENABLE_DEBUG_WINDOW
	extern bool RenderOverlayOpenGLUseViewer;
	RenderOverlayOpenGLUseViewer = true;
#endif

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(Q4_Game::Q4_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(Q4_Game::Q4_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/q4_autoexec.gm", threadId);

	return true;
}

void Q4_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

void Q4_Game::StartGame()
{
	//EngineFuncs::ConsoleMessage("<StartGame>");

	// cs: doing this to avoid goal registration in warmup
	if (m_GameState == GAME_STATE_PLAYING && m_LastGameState != GAME_STATE_PLAYING)
	{
		InitMapScript();
	}
}

void Q4_Game::AddBot(Msg_Addbot &_addbot, bool _createnow)
{
	IGame::AddBot(_addbot, false);
}

void Q4_Game::InitScriptBinds(gmMachine *_machine)
{
	// Register bot extension functions.
	gmBindQ4BotLibrary(_machine);
}

static IntEnum Q4_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("MARINE",Q4_TEAM_MARINE),
	IntEnum("STROGG",Q4_TEAM_STROGG),
};

void Q4_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(Q4_TeamEnum) / sizeof(Q4_TeamEnum[0]);
	_ptr = Q4_TeamEnum;	
}

static IntEnum Q4_WeaponEnum[] =
{
	IntEnum("NONE",Q4_WP_NONE),
	IntEnum("BLASTER", Q4_WP_BLASTER),
	IntEnum("MACHINEGUN", Q4_WP_MACHINEGUN),
	IntEnum("SHOTGUN", Q4_WP_SHOTGUN),
	IntEnum("NAILGUN", Q4_WP_NAILGUN),
	IntEnum("HYPERBLASTER", Q4_WP_HYPERBLASTER),
	IntEnum("RPG", Q4_WP_ROCKETLAUNCHER),
	IntEnum("LIGHTNINGGUN", Q4_WP_LIGHTNINGGUN),
	IntEnum("GRENADELAUNCHER", Q4_WP_GRENADELAUNCHER),
	IntEnum("GAUNTLET", Q4_WP_GAUNTLET),
	IntEnum("DARKMATTERGUN", Q4_WP_DMG),
	IntEnum("RAILGUN", Q4_WP_RAILGUN),
	IntEnum("NAPALMGUN", Q4_WP_NAPALMGUN),
};

void Q4_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(Q4_WeaponEnum) / sizeof(Q4_WeaponEnum[0]);
	_ptr = Q4_WeaponEnum;	
}

static IntEnum Q4_ClassEnum[] =
{
	IntEnum("PLAYER",				Q4_CLASS_PLAYER),
	IntEnum("ANYPLAYER",			Q4_CLASS_ANY),	
	IntEnum("GRENADE",			Q4_CLASSEX_GRENADE),
	IntEnum("ROCKET",				Q4_CLASSEX_ROCKET),
	IntEnum("NAIL",				Q4_CLASSEX_NAIL),
	IntEnum("HYPERSHOT",			Q4_CLASSEX_HYPERBLASTERSHOT),
	IntEnum("NAPALMSHOT",			Q4_CLASSEX_NAPALMSHOT),
	IntEnum("DARKMATTERSHOT",		Q4_CLASSEX_DARKMATTERSHOT),
	IntEnum("MEGAHEALTH",			Q4_CLASSEX_POWERUP_MEGAHEALTH),
	IntEnum("QUAD_DAMAGE",		Q4_CLASSEX_POWERUP_QUADDAMAGE),
	IntEnum("HASTE",				Q4_CLASSEX_POWERUP_HASTE),
	IntEnum("REGENERATION",		Q4_CLASSEX_POWERUP_REGENERATION),
	IntEnum("INVISIBILITY",		Q4_CLASSEX_POWERUP_INVISIBILITY),
	IntEnum("MARINE_FLAG",		Q4_CLASSEX_POWERUP_CTF_MARINEFLAG),
	IntEnum("STROGG_FLAG",		Q4_CLASSEX_POWERUP_CTF_STROGGFLAG),
	IntEnum("ONE_FLAG",			Q4_CLASSEX_POWERUP_CTF_ONEFLAG),
	IntEnum("AMMO_REGEN",			Q4_CLASSEX_POWERUP_AMMOREGEN),
	IntEnum("GUARD",				Q4_CLASSEX_POWERUP_GUARD),
	IntEnum("DOUBLER",			Q4_CLASSEX_POWERUP_DOUBLER),
	IntEnum("SCOUT",				Q4_CLASSEX_POWERUP_SCOUT),
	IntEnum("DEADZONE",			Q4_CLASSEX_DEADZONEPOWERUP),

	// ammo
	IntEnum("AMMO_MACHINEGUN",		Q4_CLASSEX_AMMO + Q4_WP_MACHINEGUN),
	IntEnum("AMMO_NAILGUN",			Q4_CLASSEX_AMMO + Q4_WP_NAILGUN),
	IntEnum("AMMO_RAILGUN",			Q4_CLASSEX_AMMO + Q4_WP_RAILGUN),
	IntEnum("AMMO_SHOTGUN",			Q4_CLASSEX_AMMO + Q4_WP_SHOTGUN),
	IntEnum("AMMO_HYPERBLASTER",	Q4_CLASSEX_AMMO + Q4_WP_HYPERBLASTER),
	IntEnum("AMMO_ROCKETLAUNCHER",	Q4_CLASSEX_AMMO + Q4_WP_ROCKETLAUNCHER),
	IntEnum("AMMO_GRENADELAUNCHER",	Q4_CLASSEX_AMMO + Q4_WP_GRENADELAUNCHER),
	IntEnum("AMMO_LIGHTNINGGUN",	Q4_CLASSEX_AMMO + Q4_WP_LIGHTNINGGUN),
	IntEnum("AMMO_NAPALMGUN",		Q4_CLASSEX_AMMO + Q4_WP_NAPALMGUN),
	IntEnum("AMMO_DARKMATTERGUN",	Q4_CLASSEX_AMMO + Q4_WP_DMG),
};

const char *Q4_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(Q4_ClassEnum) / sizeof(Q4_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(Q4_ClassEnum[i].m_Value == _classId)
			return Q4_ClassEnum[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void Q4_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = Q4_CLASS_ANY;

	obint32 iNumMappings = sizeof(Q4_ClassEnum) / sizeof(Q4_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, Q4_ClassEnum[i].m_Key, gmVariable(Q4_ClassEnum[i].m_Value));
	}

	InitScriptWeaponClasses(_machine,_table,Q4_CLASSEX_WEAPON);
}

void Q4_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);

	_table->Set(_machine, "IN_VEHICLE",			gmVariable(Q4_ENT_IN_VEHICLE));
	_table->Set(_machine, "FLASHLIGHT_ON",		gmVariable(Q4_ENT_FLASHLIGHT_ON));
	_table->Set(_machine, "IN_BUY_ZONE",		gmVariable(Q4_ENT_IN_BUY_ZONE));	
}

void Q4_Game::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "MARINEFLAG",			gmVariable(Q4_PWR_MARINEFLAG));
	_table->Set(_machine, "STROGGFLAG",			gmVariable(Q4_PWR_STROGGFLAG));
	_table->Set(_machine, "ONEFLAG",			gmVariable(Q4_PWR_ONEFLAG));
	_table->Set(_machine, "QUADDAMAGE",			gmVariable(Q4_PWR_QUAD));
	_table->Set(_machine, "HASTE",				gmVariable(Q4_PWR_HASTE));
	_table->Set(_machine, "REGENERATION",		gmVariable(Q4_PWR_REGEN));
	_table->Set(_machine, "INVISIBILITY",		gmVariable(Q4_PWR_INVIS));
	_table->Set(_machine, "AMMOREGEN",			gmVariable(Q4_PWR_AMMOREGEN));
	_table->Set(_machine, "GUARD",				gmVariable(Q4_PWR_GUARD));
	_table->Set(_machine, "DOUBLER",			gmVariable(Q4_PWR_DOUBLER));
	_table->Set(_machine, "SCOUT",				gmVariable(Q4_PWR_SCOUT));
	_table->Set(_machine, "DEADZONE",			gmVariable(Q4_PWR_DEADZONE));
	_table->Set(_machine, "TEAM_AMMOREGEN",		gmVariable(Q4_PWR_TEAM_AMMO_REGEN));
	_table->Set(_machine, "TEAM_HEALTHREGEN",	gmVariable(Q4_PWR_TEAM_HEALTH_REGEN));
	_table->Set(_machine, "TEAM_DAMAGEMOD",		gmVariable(Q4_PWR_TEAM_DAMAGE_MOD));
}

void Q4_Game::InitScriptBuyMenu(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "MACHINEGUN",			gmVariable(Q4_BUY_MACHINEGUN));
	_table->Set(_machine, "SHOTGUN",			gmVariable(Q4_BUY_SHOTGUN));
	_table->Set(_machine, "NAILGUN",			gmVariable(Q4_BUY_NAILGUN));
	_table->Set(_machine, "HYPERBLASTER",		gmVariable(Q4_BUY_HYPERBLASTER));
	_table->Set(_machine, "ROCKETLAUNCHER",		gmVariable(Q4_BUY_ROCKETLAUNCHER));
	_table->Set(_machine, "LIGHTNINGGUN",		gmVariable(Q4_BUY_LIGHTNINGGUN));
	_table->Set(_machine, "GRENADELAUNCHER",	gmVariable(Q4_BUY_GRENADELAUNCHER));
	_table->Set(_machine, "RAILGUN",			gmVariable(Q4_BUY_RAILGUN));
	_table->Set(_machine, "NAPALMGUN",			gmVariable(Q4_BUY_NAPALMGUN));
	_table->Set(_machine, "ARMOR_YELLOW",		gmVariable(Q4_BUY_ARMOR_SM));
	_table->Set(_machine, "ARMOR_RED",			gmVariable(Q4_BUY_ARMOR_LG));
	_table->Set(_machine, "AMMO_REFILL",		gmVariable(Q4_BUY_AMMO_REFILL));
	_table->Set(_machine, "AMMO_REGEN",			gmVariable(Q4_BUY_AMMO_REGEN));
	_table->Set(_machine, "HEALTH_REGEN",		gmVariable(Q4_BUY_HEALTH_REGEN));
	_table->Set(_machine, "DAMAGE_BOOST",		gmVariable(Q4_BUY_DAMAGE_BOOST));
}

void Q4_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);
}

void Q4_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("MARINE", F_NAV_TEAM1);
	_planner->RegisterNavFlag("STROGG", F_NAV_TEAM2);
	_planner->RegisterNavFlag("CAPPOINT", F_Q4_NAV_CAPPOINT);
	_planner->RegisterNavFlag("DEADZONE", F_Q4_NAV_DEADZONE);
}

void Q4_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	//_pfnPathCheck = Q4_PathCheck;
}

const float Q4_Game::Q4_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > Q4_CLASS_NULL && _class < Q4_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

const float Q4_Game::Q4_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > Q4_CLASS_NULL && _class < Q4_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

bool Q4_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
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
	return false;
}
