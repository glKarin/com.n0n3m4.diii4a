#include "PrecompRTCW.h"
#include "RTCW_Game.h"
#include "RTCW_GoalManager.h"
#include "RTCW_NavigationFlags.h"
#include "RTCW_VoiceMacros.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "NameManager.h"
#include "ScriptManager.h"
#include "IGameManager.h"
#include "gmRTCWBinds.h"

IGame *CreateGameInstance()
{
	return new RTCW_Game;
}

int RTCW_Game::GetVersionNum() const
{
	return RTCW_VERSION_LATEST;
}

Client *RTCW_Game::CreateGameClient()
{
	return new RTCW_Client;
}

const char *RTCW_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_rtcw.dll";
#else
	return "omni-bot/omnibot_rtcw.so";
#endif
}

const char *RTCW_Game::GetGameName() const
{
	return "RTCW";
}

const char *RTCW_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "rtcw\\";
#else
	return "rtcw";
#endif
}

const char *RTCW_Game::GetNavSubfolder() const
{
#ifdef WIN32
	return "rtcw\\nav\\";
#else
	return "rtcw/nav";
#endif
}

const char *RTCW_Game::GetScriptSubfolder() const
{
#ifdef WIN32
	return "rtcw\\scripts\\";
#else
	return "rtcw/scripts";
#endif
}

eNavigatorID RTCW_Game::GetDefaultNavigator() const 
{
	return NAVID_WP; 
}

bool RTCW_Game::ReadyForDebugWindow() const 
{ 
	return InterfaceFuncs::GetGameState() == GAME_STATE_PLAYING; 
}

GoalManager *RTCW_Game::GetGoalManager()
{
	return new RTCW_GoalManager;
}

bool RTCW_Game::Init() 
{
	SetRenderOverlayType(OVERLAY_OPENGL);

	AiState::FollowPath::m_OldLadderStyle = false;

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(RTCW_Game::RTCW_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(RTCW_Game::RTCW_GetEntityClassAimOffset);
	AiState::SensoryMemory::SetEntityVisDistanceCallback(RTCW_Game::RTCW_GetEntityVisDistance);
	AiState::SensoryMemory::SetCanSensoreEntityCallback(RTCW_Game::RTCW_CanSensoreEntity);

	if(!IGame::Init())
		return false;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/rtcw_autoexec.gm", threadId);
	ScriptManager::GetInstance()->ExecuteFile("scripts/rtcw_autoexec_user.gm", threadId);

	PathPlannerWaypoint::m_BlockableMask = F_RTCW_NAV_WALL|F_RTCW_NAV_BRIDGE|F_RTCW_NAV_WATERBLOCKABLE;

	PathPlannerWaypoint::m_CallbackFlags = F_RTCW_NAV_USEPATH;

	return true;
}

void RTCW_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

void RTCW_Game::InitScriptBinds(gmMachine *_machine)
{
	// Register bot extension functions.
	gmBindRTCWBotLibrary(_machine);
}

static IntEnum RTCW_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("AXIS",RTCW_TEAM_AXIS),
	IntEnum("ALLIES",RTCW_TEAM_ALLIES),
};

void RTCW_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(RTCW_TeamEnum) / sizeof(RTCW_TeamEnum[0]);
	_ptr = RTCW_TeamEnum;	
}

static IntEnum RTCW_WeaponEnum[] =
{
	IntEnum("NONE",RTCW_WP_NONE),
	IntEnum("KNIFE",RTCW_WP_KNIFE),
	IntEnum("LUGER",RTCW_WP_LUGER),
	IntEnum("MP40",RTCW_WP_MP40),
	IntEnum("MAUSER",RTCW_WP_MAUSER),
	IntEnum("AXIS_GRENADE",RTCW_WP_GREN_AXIS),
	IntEnum("PANZERFAUST",RTCW_WP_PANZERFAUST),
	IntEnum("VENOM",RTCW_WP_VENOM),
	IntEnum("FLAMETHROWER",RTCW_WP_FLAMETHROWER),
	IntEnum("COLT",RTCW_WP_COLT),
	IntEnum("THOMPSON",RTCW_WP_THOMPSON),
	IntEnum("GARAND",RTCW_WP_GARAND),
	IntEnum("ALLY_GRENADE",RTCW_WP_GREN_ALLIES),
	IntEnum("ROCKET",RTCW_WP_ROCKET_LAUNCHER),
	IntEnum("SNIPERRIFLE",RTCW_WP_SNIPERRIFLE),
	IntEnum("STEN",RTCW_WP_STEN),
	IntEnum("SYRINGE",RTCW_WP_SYRINGE),
	IntEnum("AMMO_PACK",RTCW_WP_AMMO_PACK),
	IntEnum("ARTY",RTCW_WP_ARTY),
	IntEnum("DYNAMITE",RTCW_WP_DYNAMITE),
	IntEnum("SNIPER",RTCW_WP_SNIPER),
	IntEnum("MEDKIT",RTCW_WP_MEDKIT),
	IntEnum("PLIERS",RTCW_WP_PLIERS),
	IntEnum("SMOKE_GRENADE",RTCW_WP_SMOKE_GRENADE),
	IntEnum("BINOCULARS",RTCW_WP_BINOCULARS),
	IntEnum("MOUNTABLE_MG42",RTCW_WP_MOUNTABLE_MG42),
	//IntEnum("FG42",RTCW_WP_FG42),
	//IntEnum("FG42_SCOPE",RTCW_WP_FG42SCOPE),
	//IntEnum("SHOTGUN",RTCW_WP_SHOTGUN),
};

void RTCW_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(RTCW_WeaponEnum) / sizeof(RTCW_WeaponEnum[0]);
	_ptr = RTCW_WeaponEnum;	
}

static IntEnum g_RTCWClassMappings[] =
{
	IntEnum("SOLDIER",			RTCW_CLASS_SOLDIER),
	IntEnum("MEDIC",			RTCW_CLASS_MEDIC),
	IntEnum("ENGINEER",			RTCW_CLASS_ENGINEER),
	IntEnum("LIEUTENANT",		RTCW_CLASS_LIEUTENANT),	
	IntEnum("ANYPLAYER",		RTCW_CLASS_ANY),	
	IntEnum("MG42MOUNT",		RTCW_CLASSEX_MG42MOUNT),
	IntEnum("DYNAMITE_ENT",		RTCW_CLASSEX_DYNAMITE),
	IntEnum("VEHICLE",			RTCW_CLASSEX_VEHICLE),
	IntEnum("BREAKABLE",		RTCW_CLASSEX_BREAKABLE),
	IntEnum("CORPSE",			RTCW_CLASSEX_CORPSE),
	IntEnum("INJUREDPLAYER",	RTCW_CLASSEX_INJUREDPLAYER),
	IntEnum("TREASURE",			RTCW_CLASSEX_TREASURE),
	IntEnum("ROCKET_ENT",		RTCW_CLASSEX_ROCKET),
	IntEnum("FLAME",			RTCW_CLASSEX_FLAMECHUNK),
	IntEnum("ARTY_ENT",			RTCW_CLASSEX_ARTY),
	IntEnum("AIRSTRIKE",		RTCW_CLASSEX_AIRSTRIKE),
	IntEnum("GRENADE",			RTCW_CLASSEX_GRENADE),
	IntEnum("BROKENCHAIR",		RTCW_CLASSEX_BROKENCHAIR),
	IntEnum("LANDMINE",			RTCW_CLASSEX_MINE),
};

const char *RTCW_Game::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(g_RTCWClassMappings) / sizeof(g_RTCWClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(g_RTCWClassMappings[i].m_Value == _classId)
			return g_RTCWClassMappings[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void RTCW_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = RTCW_CLASS_ANY;

	obint32 iNumMappings = sizeof(g_RTCWClassMappings) / sizeof(g_RTCWClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, g_RTCWClassMappings[i].m_Key, gmVariable(g_RTCWClassMappings[i].m_Value));
	}

	InitScriptWeaponClasses(_machine,_table,RTCW_CLASSEX_WEAPON);
}

void RTCW_Game::InitVoiceMacros(gmMachine *_machine, gmTableObject *_table)
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
	_table->Set(_machine, "IMA_LIEUTENANT",		gmVariable(VCHAT_IMA_LIEUTENANT));
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

void RTCW_Game::AddBot(Msg_Addbot &_addbot, bool _createnow)
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
		// Magik: As there's no instant team/class switching in ET, this is order dependent
		// always call pfnChangeClass() _before_ pfnChangeTeam()!
		// todo: send the weapon preferences as 3rd param
		g_EngineFuncs->ChangeClass(iGameID, cp->m_DesiredClass, NULL);
		if(!cp) return;
		g_EngineFuncs->ChangeTeam(iGameID, cp->m_DesiredTeam, NULL);
		if(!cp) return;

		cp->CheckTeamEvent();
		cp->CheckClassEvent();

		ScriptManager::GetInstance()->ExecBotCallback( m_ClientList[iGameID].get(), "SelectWeapons");
	}
}

// package: RTCW Script Events
//		Custom Events for Return to Castle Wolfenstein. Also see <Common Script Events>
void RTCW_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "DROWNING", gmVariable(RTCW_EVENT_DROWNING));
	_table->Set(_machine, "AMMO_RECIEVED", gmVariable(RTCW_EVENT_RECIEVEDAMMO));
	IGame::InitScriptEvents(_machine, _table);
}

void RTCW_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);

	_table->Set(_machine, "MNT_MG42",		gmVariable(RTCW_ENT_FLAG_MNT_MG42));
	_table->Set(_machine, "CARRYINGGOAL",	gmVariable(RTCW_ENT_FLAG_CARRYINGGOAL));	
	_table->Set(_machine, "LIMBO",			gmVariable(RTCW_ENT_FLAG_INLIMBO));
	_table->Set(_machine, "MOUNTABLE",		gmVariable(RTCW_ENT_FLAG_ISMOUNTABLE));
	_table->Set(_machine, "POISONED",		gmVariable(RTCW_ENT_FLAG_POISONED));
	_table->Set(_machine, "DISGUISED",		gmVariable(RTCW_ENT_FLAG_DISGUISED));
	_table->Set(_machine, "MOUNTED",		gmVariable(RTCW_ENT_FLAG_MOUNTED));
	_table->Set(_machine, "INJURED",		gmVariable(RTCW_ENT_FLAG_INJURED));
}

void RTCW_Game::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptPowerups(_machine, _table);

	_table->Set(_machine, "FIRE",			gmVariable(RTCW_PWR_FIRE));
	_table->Set(_machine, "ELECTRIC",		gmVariable(RTCW_PWR_ELECTRIC));
	_table->Set(_machine, "BREATHER",		gmVariable(RTCW_PWR_BREATHER));
	_table->Set(_machine, "NOFATIGUE",		gmVariable(RTCW_PWR_NOFATIGUE));
	_table->Set(_machine, "REDFLAG",		gmVariable(RTCW_PWR_REDFLAG));
	_table->Set(_machine, "BLUEFLAG",		gmVariable(RTCW_PWR_BLUEFLAG));
	_table->Set(_machine, "BALL",			gmVariable(RTCW_PWR_BALL));
}

void RTCW_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("AXIS",			F_NAV_TEAM1);
	_planner->RegisterNavFlag("ALLIES",			F_NAV_TEAM2);
	_planner->RegisterNavFlag("WALL",			F_RTCW_NAV_WALL);
	_planner->RegisterNavFlag("BRIDGE",			F_RTCW_NAV_BRIDGE);

	_planner->RegisterNavFlag("SPRINT",			F_RTCW_NAV_SPRINT);	

	_planner->RegisterNavFlag("WATERBLOCKABLE", F_RTCW_NAV_WATERBLOCKABLE);

	_planner->RegisterNavFlag("CAPPOINT",		F_RTCW_NAV_CAPPOINT);	

	_planner->RegisterNavFlag("ARTY_SPOT",		F_RTCW_NAV_ARTSPOT);	
	_planner->RegisterNavFlag("ARTY_TARGET_S",	F_RTCW_NAV_ARTYTARGET_S);	
	_planner->RegisterNavFlag("ARTY_TARGET_D",	F_RTCW_NAV_ARTYTARGET_D);
	_planner->RegisterNavFlag("STRAFE_L",		F_RTCW_NAV_STRAFE_L);
	_planner->RegisterNavFlag("STRAFE_R",		F_RTCW_NAV_STRAFE_R);
	_planner->RegisterNavFlag("PANZER",			F_RTCW_NAV_PANZER);
	_planner->RegisterNavFlag("VENOM",			F_RTCW_NAV_VENOM);
	_planner->RegisterNavFlag("FLAME",			F_RTCW_NAV_FLAMETHROWER);
	_planner->RegisterNavFlag("UGOAL",			F_RTCW_NAV_USERGOAL);
	_planner->RegisterNavFlag("USEPATH",		F_RTCW_NAV_USEPATH);
	_planner->RegisterNavFlag("STRAFE_JUMP_L",	F_RTCW_NAV_STRAFE_JUMP_L);
	_planner->RegisterNavFlag("STRAFE_JUMP_R",	F_RTCW_NAV_STRAFE_JUMP_R);
}

NavFlags RTCW_Game::DeprecatedNavigationFlags() const
{
	return IGame::DeprecatedNavigationFlags() | F_NAV_TEAM3 | F_NAV_TEAM4
		| F_RTCW_NAV_CAPPOINT | F_RTCW_NAV_ARTSPOT | F_RTCW_NAV_ARTYTARGET_S | F_RTCW_NAV_ARTYTARGET_D
		| F_RTCW_NAV_FLAMETHROWER | F_RTCW_NAV_PANZER | F_RTCW_NAV_VENOM | F_RTCW_NAV_USERGOAL;
}

void RTCW_Game::InitCommands()
{
	IGame::InitCommands();
}

const void RTCW_Game::RTCW_GetEntityVisDistance(float &_distance, const TargetInfo &_target, const Client *_client)
{
	switch(_target.m_EntityClass)
	{
	case RTCW_CLASSEX_BREAKABLE:
		_distance = static_cast<const RTCW_Client*>(_client)->GetBreakableTargetDist();
		break;
	case ENT_CLASS_GENERIC_AMMO:
		_distance = static_cast<const RTCW_Client*>(_client)->GetAmmoEntityDist();
		break;
	case ENT_CLASS_GENERIC_HEALTH:
		_distance = static_cast<const RTCW_Client*>(_client)->GetHealthEntityDist();
		break;
	default:
		if(_target.m_EntityCategory.CheckFlag(ENT_CAT_PICKUP_WEAPON))
			_distance = static_cast<const RTCW_Client*>(_client)->GetWeaponEntityDist();
		else if(_target.m_EntityCategory.CheckFlag(ENT_CAT_PROJECTILE))
			_distance = static_cast<const RTCW_Client*>(_client)->GetProjectileEntityDist();
	}
}

const float RTCW_Game::RTCW_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > RTCW_CLASS_NULL && _class < RTCW_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 4.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 24.0f;
		return 48.0f;
	}

	switch(_class)
	{
	case RTCW_CLASSEX_DYNAMITE:	
	case RTCW_CLASSEX_CORPSE:
		return 2.0f;
	}

	return 0.0f;
}

const float RTCW_Game::RTCW_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > RTCW_CLASS_NULL && _class < RTCW_CLASS_MAX)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 2.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 16.0f;
		return 40.0f;
	}
	return 0.0f;
}

const float RTCW_Game::RTCW_GetEntityClassAvoidRadius(const int _class)
{
	switch(_class) 
	{
	case RTCW_CLASSEX_DYNAMITE:		
		return 400.0f;
		break;
	}
	return 0.0f;
}

const bool RTCW_Game::RTCW_CanSensoreEntity(const EntityInstance &_ent)
{
	if( (((1<<ENT_CAT_PICKUP_HEALTH)|(1<<ENT_CAT_PICKUP_AMMO)|(1<<ENT_CAT_PICKUP_WEAPON)|(1<<ENT_CAT_PROJECTILE)|(1<<ENT_CAT_SHOOTABLE))
		& _ent.m_EntityCategory.GetRawFlags()) == 0)
		return false;

	int c =_ent.m_EntityClass;
	return c<RTCW_CLASS_MAX || (c!=RTCW_CLASSEX_ARTY && c!=RTCW_CLASSEX_FLAMECHUNK && c!=RTCW_CLASSEX_ROCKET);
}

void RTCW_Game::ClientJoined(const Event_SystemClientConnected *_msg)
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
			if(!cp) return;
			g_EngineFuncs->ChangeTeam(_msg->m_GameId, cp->m_DesiredTeam, NULL);
			if(!cp) return;

			cp->CheckTeamEvent();
			cp->CheckClassEvent();
		}
	}	
}

PathPlannerWaypoint::BlockableStatus RTCW_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	static bool bRender = false;
	PathPlannerWaypoint::BlockableStatus res = PathPlannerWaypoint::B_INVALID_FLAGS;
	Vector3f vStart, vEnd;

	if(/*_wp1->IsFlagOn(F_ET_NAV_WALL) &&*/ _wp2->IsFlagOn(F_RTCW_NAV_WALL))
	{
		static float fOffset = 25.0f;
		static Vector3f vMins(-5.f, -5.f, -5.f), vMaxs(5.f, 5.f, 5.f);
		AABB aabb(vMins, vMaxs);
		vStart = _wp1->GetPosition().AddZ(fOffset);
		vEnd = _wp2->GetPosition().AddZ(fOffset);
		
		if(bRender)
		{
			Utils::DrawLine(vStart, vEnd, COLOR::ORANGE, 2.f);
		}

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStart, vEnd, &aabb, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);
		res = (tr.m_Fraction == 1.0f) ? PathPlannerWaypoint::B_PATH_OPEN : PathPlannerWaypoint::B_PATH_CLOSED;
	}

	if(res != PathPlannerWaypoint::B_PATH_CLOSED && _wp1->IsFlagOn(F_RTCW_NAV_BRIDGE) && _wp2->IsFlagOn(F_RTCW_NAV_BRIDGE))
	{
		vStart = _wp1->GetPosition() + (_wp2->GetPosition() - _wp1->GetPosition()) * 0.5;
		vEnd = vStart.AddZ(-48);

		if(bRender)
		{
			Utils::DrawLine(vStart, vEnd, COLOR::ORANGE, 2.f);
		}

		obTraceResult tr;
		EngineFuncs::TraceLine(tr, vStart, vEnd, NULL, (TR_MASK_SOLID | TR_MASK_PLAYERCLIP), -1, True);
		res = (tr.m_Fraction == 1.0f) ? PathPlannerWaypoint::B_PATH_CLOSED : PathPlannerWaypoint::B_PATH_OPEN;
	}

	if(res != PathPlannerWaypoint::B_PATH_CLOSED && _wp2->IsFlagOn(F_RTCW_NAV_WATERBLOCKABLE))
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

void RTCW_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = RTCW_PathCheck;
}

void RTCW_Game::GetMapScriptFile(filePath &script)
{
	const char * mapName = g_EngineFuncs->GetMapName();
	if ( InterfaceFuncs::GetCvar("g_deathmatch") != 0 )
		script = filePath( "nav/%s_dm.gm", mapName );
	else if( InterfaceFuncs::GetGameType() == 7 )
		script = filePath( "nav/%s_cp.gm", mapName );
}


int RTCW_Game::GetLogSize()
{
	return InterfaceFuncs::GetCvar("omnibot_logsize");
}
