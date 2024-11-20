////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompD3.h"
#include "D3_Game.h"
#include "D3_Client.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"
#include "gmD3Binds.h"

IGame *CreateGameInstance()
{
	return new D3_Game;
}

Client *D3_Game::CreateGameClient()
{
	return new D3_Client;
}

int D3_Game::GetVersionNum() const
{
	return D3_VERSION_LATEST;
}

const char *D3_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_d3.dll";
#else
	return "omni-bot/omnibot_d3.so";
#endif
}

const char *D3_Game::GetGameName() const
{
	return "Doom3";
}

const char *D3_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "d3\\";
#else
	return "d3";
#endif
}

const char *D3_Game::GetNavSubfolder() const
{
	return "d3\\nav\\";
}

const char *D3_Game::GetScriptSubfolder() const
{
	return "d3\\scripts\\";
}

eNavigatorID D3_Game::GetDefaultNavigator() const 
{
	//return NAVID_RECAST;
	return NAVID_WP; 
}

bool D3_Game::Init() 
{
	SetRenderOverlayType(OVERLAY_OPENGL);

#ifdef ENABLE_DEBUG_WINDOW
	extern bool RenderOverlayOpenGLUseViewer;
	RenderOverlayOpenGLUseViewer = true;
#endif

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(D3_Game::D3_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(D3_Game::D3_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/d3_autoexec.gm", threadId);

	return true;
}

void D3_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

void D3_Game::StartGame()
{
	//EngineFuncs::ConsoleMessage("<StartGame>");

	// cs: doing this to avoid goal registration in warmup
	if (m_GameState == GAME_STATE_PLAYING && m_LastGameState != GAME_STATE_PLAYING)
	{
		InitMapScript();
	}
}

void D3_Game::InitScriptBinds(gmMachine *_machine)
{
	// Register bot extension functions.
	gmBindD3BotLibrary(_machine);
}

static IntEnum D3_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("RED",D3_TEAM_RED),
	IntEnum("BLUE",D3_TEAM_BLUE),
};

void D3_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(D3_TeamEnum) / sizeof(D3_TeamEnum[0]);
	_ptr = D3_TeamEnum;	
}

static IntEnum D3_WeaponEnum[] =
{
	IntEnum("NONE",D3_WP_NONE),
	IntEnum("FLASHLIGHT",D3_WP_FLASHLIGHT),
	IntEnum("FISTS",D3_WP_FISTS),
	IntEnum("CHAINSAW",D3_WP_CHAINSAW),
	IntEnum("PISTOL",D3_WP_PISTOL),
	IntEnum("SHOTGUN",D3_WP_SHOTGUN),
	IntEnum("SHOTGUN_DBL",D3_WP_SHOTGUN_DBL),
	IntEnum("MACHINEGUN",D3_WP_MACHINEGUN),
	IntEnum("CHAINGUN",D3_WP_CHAINGUN),
	IntEnum("HANDGRENADE",D3_WP_HANDGRENADE),
	IntEnum("PLASMAGUN",D3_WP_PLASMAGUN),
	IntEnum("ROCKETLAUNCHER",D3_WP_ROCKETLAUNCHER),
	IntEnum("BFG",D3_WP_BFG),
	IntEnum("SOULCUBE",D3_WP_SOULCUBE),
	IntEnum("GRABBER",D3_WP_GRABBER),
};

void D3_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(D3_WeaponEnum) / sizeof(D3_WeaponEnum[0]);
	_ptr = D3_WeaponEnum;	
}

static IntEnum D3_ClassEnum[] =
{
	IntEnum("PLAYER",				D3_CLASS_PLAYER),
	IntEnum("ANYPLAYER",			D3_CLASS_ANY),

	IntEnum("GRENADE",			D3_CLASSEX_GRENADE),
	IntEnum("ROCKET",				D3_CLASSEX_ROCKET),
	IntEnum("PLASMA",				D3_CLASSEX_PLASMA),
	IntEnum("BFG",				D3_CLASSEX_BFG),
	IntEnum("SOULBLAST",			D3_CLASSEX_SOULBLAST),

	/*D3_WP_PISTOL,
	D3_WP_SHOTGUN,
	D3_WP_SHOTGUN_DBL,
	D3_WP_MACHINEGUN,
	D3_WP_CHAINGUN,
	D3_WP_HANDGRENADE,
	D3_WP_PLASMAGUN,
	D3_WP_ROCKETLAUNCHER,	
	D3_WP_BFG,
	D3_WP_SOULCUBE,*/

	IntEnum("AMMO_SHELLS",		D3_CLASSEX_AMMO_SHELLS),
	IntEnum("AMMO_BULLETS",		D3_CLASSEX_AMMO_BULLETS),
	IntEnum("AMMO_CLIP",		D3_CLASSEX_AMMO_CLIP),
	IntEnum("AMMO_BELT",		D3_CLASSEX_AMMO_BELT),
	IntEnum("AMMO_ROCKET",		D3_CLASSEX_AMMO_ROCKET),
	IntEnum("AMMO_CELLS",		D3_CLASSEX_AMMO_CELLS),
	IntEnum("AMMO_GRENADE",		D3_CLASSEX_AMMO_GRENADE),
	IntEnum("AMMO_BFG",			D3_CLASSEX_AMMO_BFG),

	IntEnum("POWERUP_QUADDAMAGE",	D3_CLASSEX_POWERUP_QUADDAMAGE),
	IntEnum("POWERUP_HASTE",		D3_CLASSEX_POWERUP_HASTE),
	IntEnum("POWERUP_REGENERATION",	D3_CLASSEX_POWERUP_REGENERATION),
	IntEnum("POWERUP_INVISIBILITY",	D3_CLASSEX_POWERUP_INVISIBILITY),
	IntEnum("POWERUP_MARINEFLAG",	D3_CLASSEX_POWERUP_CTF_MARINEFLAG),
	IntEnum("POWERUP_STROGGFLAG",	D3_CLASSEX_POWERUP_CTF_STROGGFLAG),
	IntEnum("POWERUP_ONEFLAG",		D3_CLASSEX_POWERUP_CTF_ONEFLAG),
	IntEnum("POWERUP_AMMOREGEN",	D3_CLASSEX_POWERUP_AMMOREGEN),
	IntEnum("POWERUP_GUARD",		D3_CLASSEX_POWERUP_GUARD),
	IntEnum("POWERUP_DOUBLER",		D3_CLASSEX_POWERUP_DOUBLER),
	IntEnum("POWERUP_SCOUT",		D3_CLASSEX_POWERUP_SCOUT),
};

const char *D3_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(D3_ClassEnum) / sizeof(D3_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(D3_ClassEnum[i].m_Value == _classId)
			return D3_ClassEnum[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void D3_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = D3_CLASS_ANY;

	obint32 iNumMappings = sizeof(D3_ClassEnum) / sizeof(D3_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, D3_ClassEnum[i].m_Key, gmVariable(D3_ClassEnum[i].m_Value));
	}

	InitScriptWeaponClasses(_machine,_table,D3_CLASSEX_WEAPON);
}

void D3_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);
	_table->Set(_machine, "IN_VEHICLE",			gmVariable(D3_ENT_IN_VEHICLE));
	_table->Set(_machine, "FLASHLIGHT_ON",		gmVariable(D3_ENT_FLASHLIGHT_ON));
}

void D3_Game::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "BERSERK",			gmVariable(D3_PWR_BERSERK));
	_table->Set(_machine, "INVISIBILITY",		gmVariable(D3_PWR_INVISIBILITY));
	_table->Set(_machine, "MEGAHEALTH",			gmVariable(D3_PWR_MEGAHEALTH));
	_table->Set(_machine, "ADRENALINE",			gmVariable(D3_PWR_ADRENALINE));
	_table->Set(_machine, "REDFLAG",			gmVariable(D3_PWR_REDFLAG));
	_table->Set(_machine, "BLUEFLAG",			gmVariable(D3_PWR_BLUEFLAG));
}

void D3_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);
}

void D3_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("RED", F_NAV_TEAM1);
	_planner->RegisterNavFlag("BLUE", F_NAV_TEAM2);
}

void D3_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	//_pfnPathCheck = D3_PathCheck;
}

const float D3_Game::D3_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > D3_CLASS_NULL && _class < D3_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	if(_class == ENT_CLASS_EXPLODING_BARREL)
		return 10.f;
	return 0.0f;
}

const float D3_Game::D3_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > D3_CLASS_NULL && _class < D3_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 48.0f;
	}
	return 0.0f;
}

bool D3_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
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
