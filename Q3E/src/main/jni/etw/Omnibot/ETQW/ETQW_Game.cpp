////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETQW.h"
#include "ETQW_Game.h"
#include "ETQW_GoalManager.h"
#include "ETQW_NavigationFlags.h"
#include "ETQW_VoiceMacros.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "NameManager.h"
#include "ScriptManager.h"
#include "IGameManager.h"
#include "gmETQWBinds.h"

IGame *CreateGameInstance()
{
	return new ETQW_Game;
}

int ETQW_Game::GetVersionNum() const
{
	return ETQW_VERSION_LATEST;
}

Client *ETQW_Game::CreateGameClient()
{
	return new ETQW_Client;
}

const char *ETQW_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_etqw.dll";
#else
	return "omni-bot/omnibot_etqw.so";
#endif
}

const char *ETQW_Game::GetGameName() const
{
	return "Enemy Territory - Quake Wars";
}

const char *ETQW_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "etqw\\";
#else
	return "etqw";
#endif
}

const char *ETQW_Game::GetNavSubfolder() const
{
#ifdef WIN32
	return "etqw\\nav\\";
#else
	return "etqw/nav";
#endif
}

const char *ETQW_Game::GetScriptSubfolder() const
{
#ifdef WIN32
	return "etqw\\scripts\\";
#else
	return "etqw/scripts";
#endif
}

bool ETQW_Game::ReadyForDebugWindow() const 
{ 
	return InterfaceFuncs::GetGameState() == GAME_STATE_PLAYING; 
}

GoalManager *ETQW_Game::GetGoalManager()
{
	return new ETQW_GoalManager;
}

bool ETQW_Game::Init() 
{
	SetRenderOverlayType(OVERLAY_GAME);

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(ETQW_Game::ETQW_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(ETQW_Game::ETQW_GetEntityClassAimOffset);

	if(!IGame::Init())
		return false;

	PathPlannerWaypoint::m_CallbackFlags = F_ETQW_NAV_DISGUISE;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/etqw_autoexec.gm", threadId);	

	return true;
}

void ETQW_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

void ETQW_Game::InitScriptBinds(gmMachine *_machine)
{
	// Register bot extension functions.
	gmBindETQWBotLibrary(_machine);
}

static IntEnum ETQW_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("STROGG",ETQW_TEAM_STROGG),
	IntEnum("GDF",ETQW_TEAM_GDF),
};

void ETQW_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(ETQW_TeamEnum) / sizeof(ETQW_TeamEnum[0]);
	_ptr = ETQW_TeamEnum;	
}

static IntEnum ETQW_WeaponEnum[] =
{
	IntEnum("NONE",ETQW_WP_NONE),
	IntEnum("KNIFE", ETQW_WP_KNIFE),
	IntEnum("PISTOL", ETQW_WP_PISTOL),
	IntEnum("SMG", ETQW_WP_SMG),
	IntEnum("GRENADE", ETQW_WP_GRENADE),
	IntEnum("EMP", ETQW_WP_EMP),
	IntEnum("RPG", ETQW_WP_ROCKET),
	IntEnum("HEAVY_MG", ETQW_WP_HEAVY_MG),
	IntEnum("HEALTH", ETQW_WP_HEALTH),
	IntEnum("NEEDLE", ETQW_WP_NEEDLE),
	IntEnum("BINOCULARS", ETQW_WP_BINOCS),
	IntEnum("AIRSTRIKE", ETQW_WP_AIRCAN),
	IntEnum("SHOTGUN", ETQW_WP_SHOTGUN),
	IntEnum("PLIERS", ETQW_WP_PLIERS),
	IntEnum("SCOPED_SMG", ETQW_WP_SCOPED_SMG),
	IntEnum("NADE_SMG", ETQW_WP_NADE_SMG),
	IntEnum("SNIPERRIFLE", ETQW_WP_SNIPERRIFLE),
	IntEnum("HE_CHARGE", ETQW_WP_HE_CHARGE),
	IntEnum("LANDMINE", ETQW_WP_LANDMINE),
	IntEnum("HACK_TOOL", ETQW_WP_HACK_TOOL),
	IntEnum("AMMO_PACK", ETQW_WP_AMMO_PACK),
	IntEnum("SMOKE_NADE", ETQW_WP_SMOKE_NADE),
	IntEnum("DEPLOY_TOOL", ETQW_WP_DEPLOY_TOOL),
	IntEnum("SHIELD_GUN", ETQW_WP_SHIELD_GUN),
	IntEnum("TELEPORTER", ETQW_WP_TELEPORTER),
	IntEnum("FLYER_HIVE", ETQW_WP_FLYER_HIVE),
	IntEnum("SUPPLY_MARKER", ETQW_WP_SUPPLY_MARKER),
	IntEnum("THIRD_EYE", ETQW_WP_THIRD_EYE),
	IntEnum("PARACHUTE", ETQW_WP_PARACHUTE),
};

void ETQW_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(ETQW_WeaponEnum) / sizeof(ETQW_WeaponEnum[0]);
	_ptr = ETQW_WeaponEnum;	
}

static IntEnum ETQW_ClassEnum[] =
{
	IntEnum("SOLDIER",			ETQW_CLASS_SOLDIER),
	IntEnum("MEDIC",				ETQW_CLASS_MEDIC),
	IntEnum("ENGINEER",			ETQW_CLASS_ENGINEER),
	IntEnum("FIELDOPS",			ETQW_CLASS_FIELDOPS),	
	IntEnum("COVERTOPS",			ETQW_CLASS_COVERTOPS),
	IntEnum("ANYPLAYER",			ETQW_CLASS_ANY),	
	IntEnum("MG42MOUNT",			ETQW_CLASSEX_MG42MOUNT),
	IntEnum("DYNAMITE",			ETQW_CLASSEX_DYNAMITE),
	IntEnum("LANDMINE",			ETQW_CLASSEX_MINE),
	IntEnum("SATCHEL",			ETQW_CLASSEX_SATCHEL),
	IntEnum("SMOKEBOMB",			ETQW_CLASSEX_SMOKEBOMB),
	IntEnum("SMOKEMARKER",		ETQW_CLASSEX_SMOKEMARKER),
	IntEnum("VEHICLE",			ETQW_CLASSEX_VEHICLE),
	IntEnum("VEHICLE_HVY",		ETQW_CLASSEX_VEHICLE_HVY),
	IntEnum("VEHICLE_NODAMAGE",	ETQW_CLASSEX_VEHICLE_NODAMAGE),
	IntEnum("BREAKABLE",			ETQW_CLASSEX_BREAKABLE),
	IntEnum("CORPSE",				ETQW_CLASSEX_CORPSE),
	IntEnum("INJUREDPLAYER",		ETQW_CLASSEX_INJUREDPLAYER),
	IntEnum("TREASURE",			ETQW_CLASSEX_TREASURE),
	IntEnum("ROCKET",				ETQW_CLASSEX_ROCKET),
	IntEnum("MORTAR",				ETQW_CLASSEX_MORTAR),
	IntEnum("FLAME",				ETQW_CLASSEX_FLAMECHUNK),
	IntEnum("ARTY",				ETQW_CLASSEX_ARTY),
	IntEnum("AIRSTRIKE",			ETQW_CLASSEX_AIRSTRIKE),
	IntEnum("HEALTHCABINET",		ETQW_CLASSEX_HEALTHCABINET),
	IntEnum("AMMOCABINET",		ETQW_CLASSEX_AMMOCABINET),
	IntEnum("GRENADE",			ETQW_CLASSEX_GRENADE),
	IntEnum("M7_GRENADE",			ETQW_CLASSEX_M7_GRENADE),
	IntEnum("GPG40_GRENADE",		ETQW_CLASSEX_GPG40_GRENADE),
};

const char *ETQW_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(ETQW_ClassEnum) / sizeof(ETQW_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(ETQW_ClassEnum[i].m_Value == _classId)
			return ETQW_ClassEnum[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void ETQW_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = ETQW_CLASS_ANY;

	obint32 iNumMappings = sizeof(ETQW_ClassEnum) / sizeof(ETQW_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, ETQW_ClassEnum[i].m_Key, gmVariable(ETQW_ClassEnum[i].m_Value));
	}
}

void ETQW_Game::InitScriptSkills(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptSkills(_machine, _table);

	_table->Set(_machine, "BATTLE_SENSE",	gmVariable(ETQW_SKILL_BATTLE_SENSE));
	_table->Set(_machine, "ENGINEERING",	gmVariable(ETQW_SKILL_ENGINEERING));
	_table->Set(_machine, "FIRST_AID",		gmVariable(ETQW_SKILL_FIRST_AID));
	_table->Set(_machine, "SIGNALS",		gmVariable(ETQW_SKILL_SIGNALS));
	_table->Set(_machine, "LIGHT_WEAPONS",	gmVariable(ETQW_SKILL_LIGHT_WEAPONS));
	_table->Set(_machine, "HEAVY_WEAPONS",	gmVariable(ETQW_SKILL_HEAVY_WEAPONS));
	_table->Set(_machine, "COVERTOPS",		gmVariable(ETQW_SKILL_COVERTOPS));
}

void ETQW_Game::InitVoiceMacros(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "PATH_CLEARED",		gmVariable(VCHAT_TEAM_PATHCLEARED));
	_table->Set(_machine, "ENEMY_WEAK",			gmVariable(VCHAT_TEAM_ENEMYWEAK));
	_table->Set(_machine, "ALL_CLEAR",			gmVariable(VCHAT_TEAM_ALLCLEAR));

	_table->Set(_machine, "INCOMING",			gmVariable(VCHAT_TEAM_INCOMING));
	_table->Set(_machine, "FIRE_IN_THE_HOLE",	gmVariable(VCHAT_TEAM_FIREINTHEHOLE));
	_table->Set(_machine, "ON_DEFENSE",			gmVariable(VCHAT_TEAM_ONDEFENSE));
	_table->Set(_machine, "ON_OFFENSE",			gmVariable(VCHAT_TEAM_ONOFFENSE));
	_table->Set(_machine, "TAKING_FIRE",		gmVariable(VCHAT_TEAM_TAKINGFIRE));
	_table->Set(_machine, "MINES_CLEARED",		gmVariable(VCHAT_TEAM_MINESCLEARED));
	_table->Set(_machine, "ENEMY_DISGUISED",	gmVariable(VCHAT_TEAM_ENEMYDISGUISED));

	_table->Set(_machine, "NEED_MEDIC",			gmVariable(VCHAT_TEAM_MEDIC));
	_table->Set(_machine, "NEED_AMMO",			gmVariable(VCHAT_TEAM_NEEDAMMO));
	_table->Set(_machine, "NEED_BACKUP",		gmVariable(VCHAT_TEAM_NEEDBACKUP));
	_table->Set(_machine, "NEED_ENGINEER",		gmVariable(VCHAT_TEAM_NEEDENGINEER));
	_table->Set(_machine, "COVER_ME",			gmVariable(VCHAT_TEAM_COVERME));
	_table->Set(_machine, "HOLD_FIRE",			gmVariable(VCHAT_TEAM_HOLDFIRE));
	_table->Set(_machine, "WHERE_TO",			gmVariable(VCHAT_TEAM_WHERETO));
	_table->Set(_machine, "NEED_OPS",			gmVariable(VCHAT_TEAM_NEEDOPS));

	_table->Set(_machine, "FOLLOW_ME",			gmVariable(VCHAT_TEAM_FOLLOWME));
	_table->Set(_machine, "LETS_GO",			gmVariable(VCHAT_TEAM_LETGO));
	_table->Set(_machine, "MOVE",				gmVariable(VCHAT_TEAM_MOVE));
	_table->Set(_machine, "CLEAR_PATH",			gmVariable(VCHAT_TEAM_CLEARPATH));
	_table->Set(_machine, "DEFEND_OBJECTIVE",	gmVariable(VCHAT_TEAM_DEFENDOBJECTIVE));
	_table->Set(_machine, "DISARM_DYNAMITE",	gmVariable(VCHAT_TEAM_DISARMDYNAMITE));
	_table->Set(_machine, "CLEAR_MINES",		gmVariable(VCHAT_TEAM_CLEARMINES));
	_table->Set(_machine, "REINFORCE_OFF",		gmVariable(VCHAT_TEAM_REINFORCE_OFF));
	_table->Set(_machine, "REINFORCE_DEF",		gmVariable(VCHAT_TEAM_REINFORCE_DEF));

	_table->Set(_machine, "AFFIRMATIVE",		gmVariable(VCHAT_TEAM_AFFIRMATIVE));
	_table->Set(_machine, "NEGATIVE",			gmVariable(VCHAT_TEAM_NEGATIVE));
	_table->Set(_machine, "THANKS",				gmVariable(VCHAT_TEAM_THANKS));
	_table->Set(_machine, "WELCOME",			gmVariable(VCHAT_TEAM_WELCOME));
	_table->Set(_machine, "SORRY",				gmVariable(VCHAT_TEAM_SORRY));
	_table->Set(_machine, "OOPS",				gmVariable(VCHAT_TEAM_OOPS));

	_table->Set(_machine, "COMMAND_ACK",		gmVariable(VCHAT_TEAM_COMMANDACKNOWLEDGED));
	_table->Set(_machine, "COMMAND_DECLINED",	gmVariable(VCHAT_TEAM_COMMANDDECLINED));
	_table->Set(_machine, "COMMAND_COMPLETED",	gmVariable(VCHAT_TEAM_COMMANDCOMPLETED));
	_table->Set(_machine, "DESTROY_PRIMARY",	gmVariable(VCHAT_TEAM_DESTROYPRIMARY));
	_table->Set(_machine, "DESTROY_SECONDARY",	gmVariable(VCHAT_TEAM_DESTROYSECONDARY));
	_table->Set(_machine, "DESTROY_CONST",		gmVariable(VCHAT_TEAM_DESTROYCONSTRUCTION));
	_table->Set(_machine, "CONST_COMMENCING",	gmVariable(VCHAT_TEAM_CONSTRUCTIONCOMMENCING));
	_table->Set(_machine, "REPAIR_VEHICLE",		gmVariable(VCHAT_TEAM_REPAIRVEHICLE));
	_table->Set(_machine, "DESTROY_VEHICLE",	gmVariable(VCHAT_TEAM_DESTROYVEHICLE));
	_table->Set(_machine, "ESCORT_VEHICLE",		gmVariable(VCHAT_TEAM_ESCORTVEHICLE));
	
	_table->Set(_machine, "IMA_SOLDIER",		gmVariable(VCHAT_IMA_SOLDIER));
	_table->Set(_machine, "IMA_MEDIC",			gmVariable(VCHAT_IMA_MEDIC));
	_table->Set(_machine, "IMA_ENGINEER",		gmVariable(VCHAT_IMA_ENGINEER));
	_table->Set(_machine, "IMA_FIELDOPS",		gmVariable(VCHAT_IMA_FIELDOPS));
	_table->Set(_machine, "IMA_COVERTOPS",		gmVariable(VCHAT_IMA_COVERTOPS));

	_table->Set(_machine, "G_AFFIRMATIVE",		gmVariable(VCHAT_GLOBAL_AFFIRMATIVE));
	_table->Set(_machine, "G_NEGATIVE",			gmVariable(VCHAT_GLOBAL_NEGATIVE));
	_table->Set(_machine, "G_ENEMY_WEAK",		gmVariable(VCHAT_GLOBAL_ENEMYWEAK));
	_table->Set(_machine, "G_HI",				gmVariable(VCHAT_GLOBAL_HI));
	_table->Set(_machine, "G_BYE",				gmVariable(VCHAT_GLOBAL_BYE));
	_table->Set(_machine, "G_GREATSHOT",		gmVariable(VCHAT_GLOBAL_GREATSHOT));
	_table->Set(_machine, "G_CHEER",			gmVariable(VCHAT_GLOBAL_CHEER));

	_table->Set(_machine, "G_THANKS",			gmVariable(VCHAT_GLOBAL_THANKS));
	_table->Set(_machine, "G_WELCOME",			gmVariable(VCHAT_GLOBAL_WELCOME));
	_table->Set(_machine, "G_OOPS",				gmVariable(VCHAT_GLOBAL_OOPS));
	_table->Set(_machine, "G_SORRY",			gmVariable(VCHAT_GLOBAL_SORRY));
	_table->Set(_machine, "G_HOLD_FIRE",		gmVariable(VCHAT_GLOBAL_HOLDFIRE));
	_table->Set(_machine, "G_GOODGAME",			gmVariable(VCHAT_GLOBAL_GOODGAME));
}

void ETQW_Game::AddBot(Msg_Addbot &_addbot, bool _createnow)
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
		// Magik: As there's no instant team/class switching in ETQW, this is order dependent
		// always call pfnChangeClass() _before_ pfnChangeTeam()!
		// todo: send the weapon preferences as 3rd param
		g_EngineFuncs->ChangeClass(iGameID, cp->m_DesiredClass, NULL);
		g_EngineFuncs->ChangeTeam(iGameID, cp->m_DesiredTeam, NULL);

		cp->CheckTeamEvent();
		cp->CheckClassEvent();

		ScriptManager::GetInstance()->ExecBotCallback(cp.get(), "SelectWeapons");
	}
}

// package: ETQW Script Events
//		Custom Events for Enemy Territory. Also see <Common Script Events>
void ETQW_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	// event: PRETRIGGERED_MINE
	//		The bot stepped on a mine and activated it.
	_table->Set(_machine, "PRETRIGGERED_MINE", gmVariable(ETQW_EVENT_PRETRIGGER_MINE));
	// event: POSTTRIGGERED_MINE
	//		The bot stepped off a mine, thar she blows!
	_table->Set(_machine, "POSTTRIGGERED_MINE", gmVariable(ETQW_EVENT_POSTTRIGGER_MINE));
	// event: MORTAR_IMPACT
	//		The bots mortar impacted!
	_table->Set(_machine, "MORTAR_IMPACT", gmVariable(ETQW_EVENT_MORTAR_IMPACT));

	IGame::InitScriptEvents(_machine, _table);
}

void ETQW_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);

	_table->Set(_machine, "DISGUISED",		gmVariable(ETQW_ENT_FLAG_DISGUISED));
	_table->Set(_machine, "MOUNTED",		gmVariable(ETQW_ENT_FLAG_MOUNTED));
	_table->Set(_machine, "MNT_MG42",		gmVariable(ETQW_ENT_FLAG_MNT_MG42));
	_table->Set(_machine, "MNT_TANK",		gmVariable(ETQW_ENT_FLAG_MNT_TANK));
	_table->Set(_machine, "MNT_AAGUN",		gmVariable(ETQW_ENT_FLAG_MNT_AAGUN));
	_table->Set(_machine, "CARRYINGGOAL",	gmVariable(ETQW_ENT_FLAG_CARRYINGGOAL));	
	_table->Set(_machine, "LIMBO",			gmVariable(ETQW_ENT_FLAG_INLIMBO));
	_table->Set(_machine, "MOUNTABLE",		gmVariable(ETQW_ENT_FLAG_ISMOUNTABLE));
	_table->Set(_machine, "POISONED",		gmVariable(ETQW_ENT_FLAG_POISONED));
}

void ETQW_Game::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
}

void ETQW_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("AXIS",			F_NAV_TEAM1);
	_planner->RegisterNavFlag("ALLIES",			F_NAV_TEAM2);

	_planner->RegisterNavFlag("MG42",			F_ETQW_NAV_MG42SPOT);
	//_planner->RegisterNavFlag("PANZERFAUST",	F_ETQW_NAV_PANZERFSPOT);

	_planner->RegisterNavFlag("MORTAR",			F_ETQW_NAV_MORTAR);
	_planner->RegisterNavFlag("MORTAR_TARGETQW_S",F_ETQW_NAV_MORTARTARGETQW_S);
	_planner->RegisterNavFlag("MORTAR_TARGETQW_D",F_ETQW_NAV_MORTARTARGETQW_D);

	_planner->RegisterNavFlag("MINE",			F_ETQW_NAV_MINEAREA);
	//_planner->RegisterNavFlag("AIRSTRIKE",		F_ETQW_NAV_AIRSTRAREA);

	_planner->RegisterNavFlag("WALL",			F_ETQW_NAV_WALL);
	_planner->RegisterNavFlag("BRIDGE",			F_ETQW_NAV_BRIDGE);

	_planner->RegisterNavFlag("SPRINT",			F_ETQW_NAV_SPRINT);	
	_planner->RegisterNavFlag("PRONE",			F_NAV_PRONE);

	_planner->RegisterNavFlag("WATERBLOCKABLE", F_ETQW_NAV_WATERBLOCKABLE);

	_planner->RegisterNavFlag("CAPPOINT",		F_ETQW_NAV_CAPPOINT);	

	_planner->RegisterNavFlag("ARTY_SPOT",		F_ETQW_NAV_ARTSPOT);	
	_planner->RegisterNavFlag("ARTY_TARGETQW_S",	F_ETQW_NAV_ARTYTARGETQW_S);	
	_planner->RegisterNavFlag("ARTY_TARGETQW_D",	F_ETQW_NAV_ARTYTARGETQW_D);	

	_planner->RegisterNavFlag("DISGUISE",		F_ETQW_NAV_DISGUISE);	
}

void ETQW_Game::InitCommands()
{
	IGame::InitCommands();
}

/*	
	bounding boxes for et
	standing	(-18, -18, -24) x (18, 18, 48)
	crouched	(-18, -18, -24) x (18, 18, 24)
	proned		(-18, -18, -24) x (18, 18, 16)
*/
const float ETQW_Game::ETQW_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > ETQW_CLASS_NULL && _class < ETQW_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 16.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 24.0f;
		return 48.0f;
	}

	switch(_class)
	{
	case ETQW_CLASSEX_DYNAMITE:
	case ETQW_CLASSEX_MINE:
	case ETQW_CLASSEX_SATCHEL:
	case ETQW_CLASSEX_SMOKEBOMB:		
	case ETQW_CLASSEX_CORPSE:
		return 2.0f;
	}

	return 0.0f;
}

/*	
	bounding boxes for et
	standing	(-18, -18, -24) x (18, 18, 48)
	crouched	(-18, -18, -24) x (18, 18, 24)
	proned		(-18, -18, -24) x (18, 18, 16)
*/
const float ETQW_Game::ETQW_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > ETQW_CLASS_NULL && _class < ETQW_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 8.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 16.0f;
		return 40.0f;
	}

	return 0.0f;
}

const float ETQW_Game::ETQW_GetEntityClassAvoidRadius(const int _class)
{
	switch(_class) 
	{
	case ETQW_CLASSEX_DYNAMITE:		
		return 400.0f;
	case ETQW_CLASSEX_MINE:
		return 225.0f;
	case ETQW_CLASSEX_SATCHEL:
		return 300.0f;
	case ETQW_CLASSEX_SMOKEBOMB:
		break;
	}
	return 0.0f;
}

void ETQW_Game::ClientJoined(const Event_SystemClientConnected *_msg)
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

PathPlannerWaypoint::BlockableStatus ETQW_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	static bool bRender = false;
	PathPlannerWaypoint::BlockableStatus res = PathPlannerWaypoint::B_INVALID_FLAGS;

	Vector3f vStart, vEnd;

	if(/*_wp1->IsFlagOn(F_ETQW_NAV_WALL) &&*/ _wp2->IsFlagOn(F_ETQW_NAV_WALL))
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
	else if(_wp1->IsFlagOn(F_ETQW_NAV_BRIDGE) && _wp2->IsFlagOn(F_ETQW_NAV_BRIDGE))
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
	else if(_wp2->IsFlagOn(F_ETQW_NAV_WATERBLOCKABLE))
	{
		vStart = _wp1->GetPosition();
		vEnd = vStart + Vector3f(0.0f, 0.0f, 5.0f);

		if(bRender)
		{
			Utils::DrawLine(vStart, vEnd, COLOR::ORANGE, 2.f);
		}

		int iContents = g_EngineFuncs->GetPointContents(vStart);		
		res = (iContents & CONT_WATER) ? PathPlannerWaypoint::B_PATH_CLOSED : PathPlannerWaypoint::B_PATH_OPEN;
	}
	
	if(_draw && (res != PathPlannerWaypoint::B_INVALID_FLAGS))
	{
		Utils::DrawLine(vStart, vEnd, 
			(res == PathPlannerWaypoint::B_PATH_OPEN) ? COLOR::GREEN : COLOR::RED, 2.0f);
	}

	return res;
}

void ETQW_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = ETQW_PathCheck;
}
