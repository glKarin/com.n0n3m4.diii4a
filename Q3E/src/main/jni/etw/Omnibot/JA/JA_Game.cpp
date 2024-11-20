////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompJA.h"
#include "JA_Game.h"
#include "JA_GoalManager.h"
#include "JA_NavigationFlags.h"
#include "JA_VoiceMacros.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "NameManager.h"
#include "ScriptManager.h"
#include "IGameManager.h"
//#include "gmJABinds.h"

IGame *CreateGameInstance()
{
	return new JA_Game;
}

int JA_Game::GetVersionNum() const
{
	return JA_VERSION_LATEST;
}

Client *JA_Game::CreateGameClient()
{
	return new JA_Client;
}

const char *JA_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_ja.dll";
#else
	return "omni-bot/omnibot_ja.so";
#endif
}

const char *JA_Game::GetGameName() const
{
	return "Star Wars Jedi Knight: Jedi Academy";
}

const char *JA_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "ja\\";
#else
	return "ja";
#endif
}

const char *JA_Game::GetNavSubfolder() const
{
#ifdef WIN32
	return "ja\\nav\\";
#else
	return "ja/nav";
#endif
}

const char *JA_Game::GetScriptSubfolder() const
{
#ifdef WIN32
	return "ja\\scripts\\";
#else
	return "ja/scripts";
#endif
}

bool JA_Game::ReadyForDebugWindow() const 
{ 
	return InterfaceFuncs::GetGameState() == GAME_STATE_PLAYING; 
}

GoalManager *JA_Game::GetGoalManager()
{
	return new JA_GoalManager;
}

bool JA_Game::Init() 
{
	SetRenderOverlayType(OVERLAY_OPENGL);

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(JA_Game::JA_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(JA_Game::JA_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	PathPlannerWaypoint::m_CallbackFlags = F_JA_NAV_FORCEJUMP;
	PathPlannerWaypoint::m_BlockableMask = F_JA_NAV_WALL|F_JA_NAV_BRIDGE;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/ja_autoexec.gm", threadId);

	return true;
}

void JA_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

static IntEnum ET_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("FREE",JA_TEAM_FREE),
	IntEnum("RED",JA_TEAM_RED),
	IntEnum("BLUE",JA_TEAM_BLUE),
};

void JA_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(ET_TeamEnum) / sizeof(ET_TeamEnum[0]);
	_ptr = ET_TeamEnum;	
}

static IntEnum JA_WeaponEnum[] =
{
	IntEnum("NONE", JA_WP_NONE),
	IntEnum("STUN", JA_WP_STUN_BATON),
	IntEnum("MELEE", JA_WP_MELEE),
	IntEnum("SABER", JA_WP_SABER),
	IntEnum("PISTOL", JA_WP_BRYAR_PISTOL),
	IntEnum("BLASTER", JA_WP_BLASTER),
	IntEnum("DISRUPTOR", JA_WP_DISRUPTOR),
	IntEnum("BOWCASTER", JA_WP_BOWCASTER),
	IntEnum("REPEATER", JA_WP_REPEATER),
	IntEnum("DEMP2", JA_WP_DEMP2),
	IntEnum("FLECHETTE", JA_WP_FLECHETTE),
	IntEnum("ROCKET_LAUNCHER", JA_WP_ROCKET_LAUNCHER),
	IntEnum("THERMAL", JA_WP_THERMAL),
	IntEnum("TRIP_MINE", JA_WP_TRIP_MINE),
	IntEnum("DET_PACK", JA_WP_DET_PACK),
	IntEnum("CONCUSSION", JA_WP_CONCUSSION),
	IntEnum("OLDPISTOL", JA_WP_BRYAR_OLD),
};

void JA_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(JA_WeaponEnum) / sizeof(JA_WeaponEnum[0]);
	_ptr = JA_WeaponEnum;	
}

static IntEnum JKJA_ClassEnum[] =
{
	IntEnum("PLAYER",				JA_CLASS_PLAYER),
	IntEnum("ASSAULT",			JA_CLASS_ASSAULT),
	IntEnum("SCOUT",				JA_CLASS_SCOUT),
	IntEnum("TECH",				JA_CLASS_TECH),
	IntEnum("JEDI",				JA_CLASS_JEDI),
	IntEnum("DEMO",				JA_CLASS_DEMO),
	IntEnum("HW",					JA_CLASS_HW),
	IntEnum("ANYPLAYER",			JA_CLASS_ANY),	
	IntEnum("ROCKET",				JA_CLASSEX_ROCKET),
	IntEnum("MINE",				JA_CLASSEX_MINE),
	IntEnum("DETPACK",			JA_CLASSEX_DETPACK),
	IntEnum("THERMAL",			JA_CLASSEX_THERMAL),
	IntEnum("CONCUSSION",			JA_CLASSEX_CONCUSSION),
	IntEnum("BOWCASTER",			JA_CLASSEX_BOWCASTER),
	IntEnum("BOWCASTER_ALT",		JA_CLASSEX_BOWCASTER_ALT),
	IntEnum("REPEATER",			JA_CLASSEX_REPEATER),
	IntEnum("REPEATER_BLOB",		JA_CLASSEX_REPEATER_BLOB),
	IntEnum("FLECHETTE",			JA_CLASSEX_FLECHETTE),
	IntEnum("FLECHETTE_ALT",		JA_CLASSEX_FLECHETTE_ALT),
	IntEnum("BLASTER",			JA_CLASSEX_BLASTER),
	IntEnum("PISTOL",				JA_CLASSEX_PISTOL),
	IntEnum("DEMP2",				JA_CLASSEX_DEMP2),
	IntEnum("DEMP2_ALT",			JA_CLASSEX_DEMP2_ALT),
	IntEnum("TURRET_MISSILE",		JA_CLASSEX_TURRET_MISSILE),
	IntEnum("VEHMISSILE",			JA_CLASSEX_VEHMISSILE),
	IntEnum("LIGHTSABER",			JA_CLASSEX_LIGHTSABER),
	IntEnum("NPC",				JA_CLASSEX_NPC),
	IntEnum("VEHICLE",			JA_CLASSEX_VEHICLE),
	IntEnum("AUTOTURRET",			JA_CLASSEX_AUTOTURRET),
	IntEnum("BREAKABLE",			JA_CLASSEX_BREAKABLE),
	IntEnum("FORCEFIELD",			JA_CLASSEX_FORCEFIELD),
	IntEnum("SIEGEITEM",			JA_CLASSEX_SIEGEITEM),
	IntEnum("CORPSE",				JA_CLASSEX_CORPSE),
	IntEnum("WEAPON",				JA_CLASSEX_WEAPON),
	IntEnum("HOLDABLE",			JA_CLASSEX_HOLDABLE),
	IntEnum("POWERUP",			JA_CLASSEX_POWERUP),
	IntEnum("FLAGITEM",			JA_CLASSEX_FLAGITEM),
	IntEnum("HOLOCRON",			JA_CLASSEX_HOLOCRON),
};

const char *JA_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(JKJA_ClassEnum) / sizeof(JKJA_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(JKJA_ClassEnum[i].m_Value == _classId)
			return JKJA_ClassEnum[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void JA_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = JA_CLASS_ANY;

	obint32 iNumMappings = sizeof(JKJA_ClassEnum) / sizeof(JKJA_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, JKJA_ClassEnum[i].m_Key, gmVariable(JKJA_ClassEnum[i].m_Value));
	}
}

void JA_Game::InitVoiceMacros(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "ATTACK_POSITION",	gmVariable(VCHAT_ATTACK_POSITION));
	_table->Set(_machine, "ATTACK_PRIMARY",		gmVariable(VCHAT_ATTACK_PRIMARY));
	_table->Set(_machine, "ATTACK_SECONDARY",	gmVariable(VCHAT_ATTACK_SECONDARY));

	_table->Set(_machine, "DEFEND_GUNS",		gmVariable(VCHAT_DEFEND_GUNS));
	_table->Set(_machine, "DEFEND_POSITION",	gmVariable(VCHAT_DEFEND_POSITION));
	_table->Set(_machine, "DEFEND_PRIMARY",		gmVariable(VCHAT_DEFEND_PRIMARY));
	_table->Set(_machine, "DEFEND_SECONDARY",	gmVariable(VCHAT_DEFEND_SECONDARY));

	_table->Set(_machine, "REPLY_COMING",		gmVariable(VCHAT_REPLY_COMING));
	_table->Set(_machine, "REPLY_GO",			gmVariable(VCHAT_REPLY_GO));
	_table->Set(_machine, "REPLY_NO",			gmVariable(VCHAT_REPLY_NO));
	_table->Set(_machine, "REPLY_STAY",			gmVariable(VCHAT_REPLY_STAY));
	_table->Set(_machine, "REPLY_YES",			gmVariable(VCHAT_REPLY_YES));

	_table->Set(_machine, "REQUEST_ASSISTANCE",	gmVariable(VCHAT_REQUEST_ASSISTANCE));
	_table->Set(_machine, "REQUEST_DEMO",		gmVariable(VCHAT_REQUEST_DEMO));
	_table->Set(_machine, "REQUEST_HEAVYWEAPS",	gmVariable(VCHAT_REQUEST_HEAVYWEAPS));
	_table->Set(_machine, "REQUEST_MEDIC",		gmVariable(VCHAT_REQUEST_MEDIC));
	_table->Set(_machine, "REQUEST_SUPPLIES",	gmVariable(VCHAT_REQUEST_SUPPLIES));
	_table->Set(_machine, "REQUEST_TECH",		gmVariable(VCHAT_REQUEST_TECH));

	_table->Set(_machine, "SPOT_AIR",			gmVariable(VCHAT_SPOT_AIR));
	_table->Set(_machine, "SPOT_DEFENSES",		gmVariable(VCHAT_SPOT_DEFENSES));
	_table->Set(_machine, "SPOT_EMPLACED",		gmVariable(VCHAT_SPOT_EMPLACED));
	_table->Set(_machine, "SPOT_SNIPER",		gmVariable(VCHAT_SPOT_SNIPER));
	_table->Set(_machine, "SPOT_TROOPS",		gmVariable(VCHAT_SPOT_TROOPS));

	_table->Set(_machine, "TACTICS_COVER",		gmVariable(VCHAT_TACTICS_COVER));
	_table->Set(_machine, "TACTICS_FALLBACK",	gmVariable(VCHAT_TACTICS_FALLBACK));
	_table->Set(_machine, "TACTICS_FOLLOW",		gmVariable(VCHAT_TACTICS_FOLLOW));
	_table->Set(_machine, "TACTICS_HOLD",		gmVariable(VCHAT_TACTICS_HOLD));
	_table->Set(_machine, "TACTICS_SPLITUP",	gmVariable(VCHAT_TACTICS_SPLITUP));
	_table->Set(_machine, "TACTICS_TOGETHER",	gmVariable(VCHAT_TACTICS_TOGETHER));
}

void JA_Game::AddBot(Msg_Addbot &_addbot, bool _createnow)
{
	//////////////////////////////////////////////////////////////////////////
	if(!_addbot.m_Name[0])
	{
		NamePtr nr = NameManager::GetInstance()->GetName();
		String name = nr ? nr->GetName() : Utils::FindOpenPlayerName();
		Utils::StringCopy(_addbot.m_Name, name.c_str(), sizeof(_addbot.m_Name));
	}
	//////////////////////////////////////////////////////////////////////////
	OBASSERT(GameStarted(),"Game Not Started Yet");
	if(_createnow)
		m_BotJoining = true;
	int iGameID = InterfaceFuncs::Addbot(_addbot);
	if(_createnow)
		m_BotJoining = false;
	if(iGameID != -1 && _createnow)
	{
		ClientPtr &cp = GetClientFromCorrectedGameId(iGameID);

		if(!cp)
		{
			// Initialize the appropriate slot in the list.
			cp.reset(CreateGameClient());
			cp->Init(iGameID);
		}

		cp->m_DesiredTeam = _addbot.m_Team;
		cp->m_DesiredClass = _addbot.m_Class;

		//////////////////////////////////////////////////////////////////////////
		// Script callbacks
		if(cp->m_DesiredTeam == -1)
		{
			gmVariable vteam = ScriptManager::GetInstance()->ExecBotCallback(
				cp.get(), 
				"SelectTeam");
			cp->m_DesiredTeam = vteam.IsInt() ? vteam.GetInt() : -1;
		}
		if(cp->m_DesiredClass == -1)
		{
			gmVariable vclass = ScriptManager::GetInstance()->ExecBotCallback(
				cp.get(), 
				"SelectClass");
			cp->m_DesiredClass = vclass.IsInt() ? vclass.GetInt() : -1;
		}
		//////////////////////////////////////////////////////////////////////////
		// Magik: As there's no instant team/class switching in JA, this is order dependent
		// always call pfnChangeClass() _before_ pfnChangeTeam()!
		// todo: send the weapon preferences as 3rd param
		g_EngineFuncs->ChangeClass(iGameID, cp->m_DesiredClass, NULL);
		g_EngineFuncs->ChangeTeam(iGameID, cp->m_DesiredTeam, NULL);

		cp->CheckTeamEvent();
		cp->CheckClassEvent();

		/*ScriptManager::GetInstance()->ExecBotCallback(cp.get(), "SelectWeapons");*/
	}
}

void JA_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEvents(_machine, _table);
}

void JA_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);

	_table->Set(_machine, "NPC",			gmVariable(JA_ENT_FLAG_NPC));
	_table->Set(_machine, "VEH",			gmVariable(JA_ENT_FLAG_VEHICLE));
	_table->Set(_machine, "VEH_PILOTED",	gmVariable(JA_ENT_FLAG_VEHICLE_PILOTED));
	_table->Set(_machine, "JETPACK",		gmVariable(JA_ENT_FLAG_JETPACK));
	_table->Set(_machine, "CLOAKED",		gmVariable(JA_ENT_FLAG_CLOAKED));
	_table->Set(_machine, "CARRYINGFLAG",	gmVariable(JA_ENT_FLAG_CARRYINGFLAG));
	_table->Set(_machine, "SIEGEDEAD",		gmVariable(JA_ENT_FLAG_SIEGEDEAD));
	_table->Set(_machine, "YSALAMIRI",		gmVariable(JA_ENT_FLAG_YSALAMIRI));
}

void JA_Game::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "REDFLAG",			gmVariable(JA_PWR_REDFLAG));
	_table->Set(_machine, "BLUEFLAG",			gmVariable(JA_PWR_BLUEFLAG));
	_table->Set(_machine, "ONEFLAG",			gmVariable(JA_PWR_ONEFLAG));
	_table->Set(_machine, "FORCE_ENLIGHTENMENT",gmVariable(JA_PWR_FORCE_ENLIGHTENED_LIGHT));
	_table->Set(_machine, "FORCE_ENDARKENMENT",	gmVariable(JA_PWR_FORCE_ENLIGHTENED_DARK));
	_table->Set(_machine, "FORCE_BOON",			gmVariable(JA_PWR_FORCE_BOON));
	_table->Set(_machine, "YSALAMIRI",			gmVariable(JA_PWR_YSALAMIRI));
}

void JA_Game::InitScriptContentFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptContentFlags(_machine, _table);
	_table->Set(_machine, "LIGHTSABER",			gmVariable(CONT_LIGHTSABER));
}

void JA_Game::InitScriptBotButtons(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptBotButtons(_machine, _table);

	_table->Set(_machine, "FORCEPOWER",	gmVariable(BOT_BUTTON_FORCEPOWER));
	_table->Set(_machine, "FORCEGRIP",	gmVariable(BOT_BUTTON_FORCEGRIP));
	_table->Set(_machine, "FORCELIGHTNING",gmVariable(BOT_BUTTON_FORCELIGHTNING));
	_table->Set(_machine, "FORCEDRAIN",gmVariable(BOT_BUTTON_FORCEDRAIN));
}

void JA_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("RED",		F_NAV_TEAM1);
	_planner->RegisterNavFlag("BLUE",		F_NAV_TEAM2);

	_planner->RegisterNavFlag("CAPPOINT",	F_JA_NAV_CAPPOINT);
	_planner->RegisterNavFlag("WALL",		F_JA_NAV_WALL);
	_planner->RegisterNavFlag("BRIDGE",		F_JA_NAV_BRIDGE);

	_planner->RegisterNavFlag("FORCEJUMP",	F_JA_NAV_FORCEJUMP);
}

void JA_Game::InitCommands()
{
	IGame::InitCommands();
}

/*	
	bounding boxes for ja
	standing	(-15, -15, -24) x (15, 15, 40)
	crouched	(-15, -15, -24) x (15, 15, 16)
*/
const float JA_Game::JA_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > JA_CLASS_NULL && _class < JA_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 40.0f;
	}
	/*if (_class == JA_CLASSEX_NPC || _class == JA_CLASSEX_VEHICLE)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 20.0f;
		return 40.0f;
	}*/
	return 0.0f;
}

/*	
	bounding boxes for ja
	standing	(-15, -15, -24) x (15, 15, 40)
	crouched	(-15, -15, -24) x (15, 15, 16)
*/
const float JA_Game::JA_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > JA_CLASS_NULL && _class < JA_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 12.8f;
		return 32.0f;
	}
	/*if (_class == JA_CLASSEX_NPC || _class == JA_CLASSEX_VEHICLE)
	{
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 12.8f;
		return 32.0f;
	}*/
	return 0.0f;
}

const float JA_Game::JA_GetEntityClassAvoidRadius(const int _class)
{
	switch(_class) 
	{
	case JA_CLASSEX_MINE:
		return 310.0f;
	case JA_CLASSEX_DETPACK:		
		return 300.0f;
	case JA_CLASSEX_THERMAL:
		return 180.0f;
	}
	return 0.0f;
}

void JA_Game::ClientJoined(const Event_SystemClientConnected *_msg)
{
	Utils::OutputDebug(kInfo, "Client Joined Game, IsBot: %d, ClientNum: %d", _msg->m_IsBot, _msg->m_GameId);
	if(_msg->m_IsBot && !m_BotJoining)
	{
		CheckGameState();
		OBASSERT(GameStarted(),"Game Not Started Yet");
		OBASSERT(_msg->m_GameId < Constants::MAX_PLAYERS && _msg->m_GameId >= 0, "Invalid Client Index!");
		// If a bot isn't created by now, it has probably been a map change,
		// and the game has re-added the clients itself.
		ClientPtr &cp = GetClientFromCorrectedGameId(_msg->m_GameId);
		if(!cp)
		{
			// Initialize the appropriate slot in the list.
			cp.reset(CreateGameClient());
			cp->Init(_msg->m_GameId);

			cp->m_DesiredTeam = _msg->m_DesiredTeam;
			cp->m_DesiredClass = _msg->m_DesiredClass;

			g_EngineFuncs->ChangeClass(_msg->m_GameId, cp->m_DesiredClass, NULL);
			g_EngineFuncs->ChangeTeam(_msg->m_GameId, cp->m_DesiredTeam, NULL);

			cp->CheckTeamEvent();
			cp->CheckClassEvent();
		}
	}	
}

PathPlannerWaypoint::BlockableStatus JA_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	static bool bRender = false;
	PathPlannerWaypoint::BlockableStatus res = PathPlannerWaypoint::B_INVALID_FLAGS;

	Vector3f vStart, vEnd;

	if(/*_wp1->IsFlagOn(F_JA_NAV_WALL) &&*/ _wp2->IsFlagOn(F_JA_NAV_WALL))
	{
		static float fOffset = 25.0f;
		static Vector3f vMins(-5.f, -5.f, -5.f), vMaxs(5.f, 5.f, 5.f);
		AABB aabb(vMins, vMaxs);
		vStart = _wp1->GetPosition() + Vector3f(0, 0, fOffset);
		vEnd = _wp2->GetPosition() + Vector3f(0, 0, fOffset);
		
		if(bRender)
		{
			Utils::DrawLine(vStart, vEnd, COLOR::ORANGE, 2.f);
		}

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStart, vEnd, &aabb, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);
		res = (tr.m_Fraction == 1.0f) ? PathPlannerWaypoint::B_PATH_OPEN : PathPlannerWaypoint::B_PATH_CLOSED;
	}
	else if(_wp1->IsFlagOn(F_JA_NAV_BRIDGE) && _wp2->IsFlagOn(F_JA_NAV_BRIDGE))
	{
		vStart = _wp1->GetPosition() + (_wp2->GetPosition() - _wp1->GetPosition()) * 0.5;
		vEnd = vStart +  Vector3f(0,0,-48);

		if(bRender)
		{
			Utils::DrawLine(vStart, vEnd, COLOR::ORANGE, 2.f);
		}

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStart, vEnd, NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);
		res = (tr.m_Fraction == 1.0f) ? PathPlannerWaypoint::B_PATH_CLOSED : PathPlannerWaypoint::B_PATH_OPEN;
	}
	
	if(_draw && (res != PathPlannerWaypoint::B_INVALID_FLAGS))
	{
		Utils::DrawLine(vStart, vEnd, 
			(res == PathPlannerWaypoint::B_PATH_OPEN) ? COLOR::GREEN : COLOR::RED, 2.0f);
	}

	return res;
}

void JA_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = JA_PathCheck;
}
