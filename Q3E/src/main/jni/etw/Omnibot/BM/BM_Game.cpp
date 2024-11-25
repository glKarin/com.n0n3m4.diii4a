////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompBM.h"
#include "BM_Game.h"
#include "BM_Client.h"

#include "gmBMBinds.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "ScriptManager.h"

IGame *CreateGameInstance()
{
	return new BM_Game;
}

int BM_Game::GetVersionNum() const
{
	return BM_VERSION_LATEST;
}

Client *BM_Game::CreateGameClient()
{
	return new BM_Client;
}

const char *BM_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_bm.dll";
#else
	return "omni-bot/omnibot_bm.so";
#endif	
}

const char *BM_Game::GetGameName() const
{
	return "Modular Combat";
}

const char *BM_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "bm\\";
#else
	return "bm";
#endif
}

const char *BM_Game::GetNavSubfolder() const
{
	return "bm\\nav\\";
}

const char *BM_Game::GetScriptSubfolder() const
{
	return "bm\\scripts\\";
}

bool BM_Game::Init() 
{
	SetRenderOverlayType(OVERLAY_GAME);

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(BM_Game::BM_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(BM_Game::BM_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/bm_autoexec.gm", threadId);

	return true;
}

void BM_Game::InitScriptBinds(gmMachine *_machine)
{
	LOG("Binding BM Library...");
	gmBindBMLibrary(_machine);
}

void BM_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 72.f;
}

ClientPtr &BM_Game::GetClientFromCorrectedGameId(int _gameid) 
{
	return m_ClientList[_gameid-1]; 
}

static IntEnum BM_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("RED",BM_TEAM_RED),
	IntEnum("BLUE",BM_TEAM_BLUE),
};

void BM_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(BM_TeamEnum) / sizeof(BM_TeamEnum[0]);
	_ptr = BM_TeamEnum;	
}

static IntEnum BM_WeaponEnum[] =
{
	IntEnum("SMG",BM_WP_SMG),
	IntEnum("MINIGUN",BM_WP_MINIGUN),
	IntEnum("PLASMA",BM_WP_PLASMA),
	IntEnum("RPG",BM_WP_RPG),
	IntEnum("SMG",BM_WP_SMG),
	IntEnum("GL",BM_WP_GL),
	IntEnum("PHOENIX",BM_WP_PHOENIX),
	IntEnum("RPG",BM_WP_RPG),
	IntEnum("ION_CANNON",BM_WP_ION_CANNON),
};

void BM_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(BM_WeaponEnum) / sizeof(BM_WeaponEnum[0]);
	_ptr = BM_WeaponEnum;	
}

void BM_Game::InitScriptCategories(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptCategories(_machine, _table);
}

static IntEnum BM_ClassEnum[] =
{
	IntEnum("PLAYER",				BM_CLASS_PLAYER),
	IntEnum("ANYPLAYER",			BM_CLASS_ANY),	
	IntEnum("SMALL_HEALTH",		BM_CLASSEX_SMALL_HEALTH),	
	IntEnum("MEDIUM_HEALTH",		BM_CLASSEX_MEDIUM_HEALTH),	
	IntEnum("LARGE_HEALTH",		BM_CLASSEX_LARGE_HEALTH),	
	IntEnum("LIGHT_ARMOR",		BM_CLASSEX_LIGHT_ARMOR),	
	IntEnum("HEAVY_ARMOR",		BM_CLASSEX_HEAVY_ARMOR),
	IntEnum("BULLET_AMMO",		BM_CLASSEX_BULLET_AMMO),
	IntEnum("PLASMA_AMMO",		BM_CLASSEX_PLASMA_AMMO),
	IntEnum("ROCKET_AMMO",		BM_CLASSEX_ROCKET_AMMO),
	IntEnum("GRENADE_AMMO",		BM_CLASSEX_GRENADE_AMMO),
	IntEnum("PHOENIX_AMMO",		BM_CLASSEX_PHOENIX_AMMO),
};

const char *BM_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(BM_ClassEnum) / sizeof(BM_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(BM_ClassEnum[i].m_Value == _classId)
			return BM_ClassEnum[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void BM_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = BM_CLASS_ANY;

	obint32 iNumMappings = sizeof(BM_ClassEnum) / sizeof(BM_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, BM_ClassEnum[i].m_Key, gmVariable(BM_ClassEnum[i].m_Value));
	}

	InitScriptWeaponClasses(_machine,_table,BM_CLASSEX_WEAPON);
}

void BM_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);

	_table->Set(_machine, "PLAYER_SPREE", gmVariable(BM_EVENT_PLAYER_SPREE));
	_table->Set(_machine, "PLAYER_SPREE_END", gmVariable(BM_EVENT_PLAYER_SPREE_END));

	_table->Set(_machine, "SPREEWAR_START", gmVariable(BM_EVENT_SPREEWAR_START));
	_table->Set(_machine, "SPREEWAR_END", gmVariable(BM_EVENT_SPREEWAR_END));

	_table->Set(_machine, "LEVEL_UP", gmVariable(BM_EVENT_LEVEL_UP));
}

void BM_Game::InitScriptBotButtons(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptBotButtons(_machine, _table);

	_table->Set(_machine, "USE_POWERUP",	gmVariable(BM_BOT_BUTTON_USE_POWERUP));
}

void BM_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("RED", F_NAV_TEAM1);
	_planner->RegisterNavFlag("BLUE", F_NAV_TEAM2);
}

void BM_Game::InitCommands()
{
	IGame::InitCommands();
}

const float BM_Game::BM_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	/*switch(_class)
	{
	case BM_CLASS_PLAYER:
		{
			if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
				return 20.0f;
			return 48.0f;
		}
	}

	if(_class>=BM_CLASSEX_WEAPON && _class<BM_CLASSEX_WEAPON_LAST)
	{
		return 8.f;
	}*/
	return 0.0f;
}

const float BM_Game::BM_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	/*switch(_class)
	{
	case BM_CLASS_PLAYER:
		{
			if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
				return 20.0f;
			return 48.0f;
		}
	}*/
	return 0.0f;
}

PathPlannerWaypoint::BlockableStatus BM_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
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

void BM_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = BM_PathCheck;
}
