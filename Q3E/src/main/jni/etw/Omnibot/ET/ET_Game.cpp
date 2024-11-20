#include "PrecompET.h"
#include "ET_Game.h"
#include "ET_GoalManager.h"
#include "ET_NavigationFlags.h"
#include "ET_VoiceMacros.h"

#include "NavigationManager.h"
#include "PathPlannerWaypoint.h"
#include "NameManager.h"
#include "ScriptManager.h"
#include "IGameManager.h"
#include "gmETBinds.h"

int ET_Game::CLASSEXoffset;
bool ET_Game::IsETBlight, ET_Game::IsBastardmod, ET_Game::IsNoQuarter;
bool ET_Game::m_WatchForMines;

IGame *CreateGameInstance()
{
	return new ET_Game;
}

int ET_Game::GetVersionNum() const
{
	return ET_VERSION_LATEST;
}

bool ET_Game::CheckVersion(int _version)
{
	return _version == ET_VERSION_LATEST || _version == ET_VERSION_0_71;
}

Client *ET_Game::CreateGameClient()
{
	return new ET_Client;
}

const char *ET_Game::GetDLLName() const
{
#ifdef WIN32
	return "omni-bot\\omnibot_et.dll";
#else
	return "omni-bot/omnibot_et.so";
#endif
}

const char *ET_Game::GetGameName() const
{
	return "Enemy Territory";
}

const char *ET_Game::GetModSubFolder() const
{
#ifdef WIN32
	return "et\\";
#else
	return "et";
#endif
}

const char *ET_Game::GetNavSubfolder() const
{
#ifdef WIN32
	return "et\\nav\\";
#else
	return "et/nav";
#endif
}

const char *ET_Game::GetScriptSubfolder() const
{
#ifdef WIN32
	return "et\\scripts\\";
#else
	return "et/scripts";
#endif
}

eNavigatorID ET_Game::GetDefaultNavigator() const 
{
	//return NAVID_RECAST;
	return NAVID_WP; 
}

bool ET_Game::ReadyForDebugWindow() const 
{ 
	return InterfaceFuncs::GetGameState() == GAME_STATE_PLAYING; 
}

const char *ET_Game::IsDebugDrawSupported() const 
{ 
	if(InterfaceFuncs::GetCvar("dedicated")!=0)
		return "Can't draw waypoints on dedicated server.";
	if(strcmp(g_EngineFuncs->GetModName(), "etmain"))
		return "Only omnibot mod can draw waypoints.";

#ifdef INTERPROCESS
	bool EnableIpc = false;
	Options::GetValue("Debug Render","EnableInterProcess",EnableIpc);
	if(!EnableIpc) 
		return "Waypoints are not visible because option EnableInterProcess in file omni-bot.cfg is false.";
#endif

	if(InterfaceFuncs::GetCvar("cg_omnibotdrawing")==0) 
		return "Waypoints are not visible because cg_omnibotdrawing is \"0\".";
	return NULL;
}


GoalManager *ET_Game::GetGoalManager()
{
	return new ET_GoalManager;
}

bool ET_Game::Init() 
{
	SetRenderOverlayType(OVERLAY_OPENGL);

	const char *modName = g_EngineFuncs->GetModName();
	IsETBlight = !strcmp(modName, "etblight");
	IsBastardmod = !strcmp(modName, "bastardmod");
	IsNoQuarter = !strcmp(modName, "noquarter") && IGameManager::GetInstance()->GetInterfaceVersionNum() <= ET_VERSION_0_71;
	CLASSEXoffset = IsETBlight ? 2 : 0;
	m_WatchForMines = false;

	AiState::FollowPath::m_OldLadderStyle = false;

	// Set the sensory systems callback for getting aim offsets for entity types.
	AiState::SensoryMemory::SetEntityTraceOffsetCallback(ET_Game::ET_GetEntityClassTraceOffset);
	AiState::SensoryMemory::SetEntityAimOffsetCallback(ET_Game::ET_GetEntityClassAimOffset);
	AiState::SensoryMemory::SetEntityVisDistanceCallback(ET_Game::ET_GetEntityVisDistance);
	AiState::SensoryMemory::SetCanSensoreEntityCallback(ET_Game::ET_CanSensoreEntity);
	AiState::SensoryMemory::m_pfnAddSensorCategory = ET_Game::ET_AddSensorCategory;

	InitWeaponEnum();

	if(!IGame::Init())
		return false;

	PathPlannerWaypoint::m_BlockableMask = F_ET_NAV_WALL|F_ET_NAV_BRIDGE|F_ET_NAV_WATERBLOCKABLE;
	PathPlannerWaypoint::m_CallbackFlags = F_ET_NAV_DISGUISE|F_ET_NAV_USEPATH;

	// Run the games autoexec.
	int threadId;
	ScriptManager::GetInstance()->ExecuteFile("scripts/et_autoexec.gm", threadId);	
	ScriptManager::GetInstance()->ExecuteFile("scripts/et_autoexec_user.gm", threadId);

	return true;
}

void ET_Game::GetGameVars(GameVars &_gamevars)
{
	_gamevars.mPlayerHeight = 64.f;
}

void ET_Game::InitScriptBinds(gmMachine *_machine)
{
	// Register bot extension functions.
	gmBindETBotLibrary(_machine);
}

static IntEnum ET_TeamEnum[] =
{
	IntEnum("SPECTATOR",OB_TEAM_SPECTATOR),
	IntEnum("AXIS",ET_TEAM_AXIS),
	IntEnum("ALLIES",ET_TEAM_ALLIES),
};

void ET_Game::GetTeamEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(ET_TeamEnum) / sizeof(ET_TeamEnum[0]);
	_ptr = ET_TeamEnum;	
}

void ET_Game::InitScriptCategories(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptCategories(_machine,_table);
	_table->Set(_machine, "MINE",		gmVariable(ET_ENT_CAT_MINE));
}

static IntEnum ET_WeaponEnum[128] =
{
	IntEnum("NONE",ET_WP_NONE),
	IntEnum("KNIFE",ET_WP_KNIFE),
	IntEnum("BINOCULARS",ET_WP_BINOCULARS),
	IntEnum("AXIS_GRENADE",ET_WP_GREN_AXIS),
	IntEnum("ALLY_GRENADE",ET_WP_GREN_ALLIES),
	IntEnum("LUGER",ET_WP_LUGER),
	IntEnum("LUGER_AKIMBO",ET_WP_AKIMBO_LUGER),
	IntEnum("LUGER_SILENCED",ET_WP_SILENCED_LUGER),
	IntEnum("LUGER_AKIMBO_SILENCED",ET_WP_AKIMBO_SILENCED_LUGER),
	IntEnum("MP40",ET_WP_MP40),
	IntEnum("COLT",ET_WP_COLT),
	IntEnum("COLT_AKIMBO",ET_WP_AKIMBO_COLT),
	IntEnum("COLT_SILENCED",ET_WP_SILENCED_COLT),
	IntEnum("COLT_AKIMBO_SILENCED",ET_WP_AKIMBO_SILENCED_COLT),
	IntEnum("THOMPSON",ET_WP_THOMPSON),
	IntEnum("PANZERFAUST",ET_WP_PANZERFAUST),
	IntEnum("FLAMETHROWER",ET_WP_FLAMETHROWER),
	IntEnum("MORTAR",ET_WP_MORTAR),
	IntEnum("MORTAR_SET",ET_WP_MORTAR_SET),
	IntEnum("MOBILE_MG42",ET_WP_MOBILE_MG42),
	IntEnum("MOBILE_MG42_SET",ET_WP_MOBILE_MG42_SET),
	IntEnum("SYRINGE",ET_WP_SYRINGE),
	IntEnum("MEDKIT",ET_WP_MEDKIT),
	IntEnum("ADRENALINE",ET_WP_ADRENALINE),
	IntEnum("STEN",ET_WP_STEN),
	IntEnum("FG42",ET_WP_FG42),
	IntEnum("FG42_SCOPE",ET_WP_FG42_SCOPE),
	IntEnum("SATCHEL",ET_WP_SATCHEL),
	IntEnum("SATCHEL_DET",ET_WP_SATCHEL_DET),
	IntEnum("K43",ET_WP_K43),
	IntEnum("K43_SCOPE",ET_WP_K43_SCOPE),
	IntEnum("GARAND",ET_WP_GARAND),
	IntEnum("GARAND_SCOPE",ET_WP_GARAND_SCOPE),
	IntEnum("AMMO_PACK",ET_WP_AMMO_PACK),
	IntEnum("SMOKE_GRENADE",ET_WP_SMOKE_GRENADE),
	IntEnum("SMOKE_MARKER",ET_WP_SMOKE_MARKER),
	IntEnum("DYNAMITE",ET_WP_DYNAMITE),
	IntEnum("PLIERS",ET_WP_PLIERS),
	IntEnum("LANDMINE",ET_WP_LANDMINE),
	IntEnum("KAR98",ET_WP_KAR98),
	IntEnum("GPG40",ET_WP_GPG40),
	IntEnum("CARBINE",ET_WP_CARBINE),
	IntEnum("M7",ET_WP_M7),
	IntEnum("MOUNTABLE_MG42",ET_WP_MOUNTABLE_MG42),	
};

static int NumETweapons = 0;

void ET_Game::InitWeaponEnum()
{
	if(NumETweapons==0)
	{
		while(ET_WeaponEnum[NumETweapons].m_Key) ++NumETweapons;
	}
	m_NumWeapons = NumETweapons;
}

void ET_Game::GetWeaponEnumeration(const IntEnum *&_ptr, int &num)
{
	num = m_NumWeapons;
	_ptr = ET_WeaponEnum;	
}

bool ET_Game::AddWeaponId(const char * weaponName, int weaponId)
{
	const char * wpnName = m_ExtraWeaponNames.AddUniqueString(weaponName);

	const int arraySize = sizeof(ET_WeaponEnum) / sizeof(ET_WeaponEnum[0]);
	if(m_NumWeapons < arraySize)
	{
		ET_WeaponEnum[m_NumWeapons].m_Key = wpnName;
		ET_WeaponEnum[m_NumWeapons].m_Value = weaponId;
		m_NumWeapons++;
		return true;
	}
	return false;
}

static IntEnum ET_ClassEnum[] =
{
	IntEnum("SOLDIER",			ET_CLASS_SOLDIER),
	IntEnum("MEDIC",			ET_CLASS_MEDIC),
	IntEnum("ENGINEER",			ET_CLASS_ENGINEER),
	IntEnum("FIELDOPS",			ET_CLASS_FIELDOPS),	
	IntEnum("COVERTOPS",		ET_CLASS_COVERTOPS),
	IntEnum("ANYPLAYER",		ET_CLASS_ANY),	
	IntEnum("MG42MOUNT",		ET_CLASSEX_MG42MOUNT),
	IntEnum("DYNAMITE_ENT",		ET_CLASSEX_DYNAMITE),
	IntEnum("LANDMINE_ENT",		ET_CLASSEX_MINE),
	IntEnum("SATCHEL_ENT",		ET_CLASSEX_SATCHEL),
	IntEnum("SMOKEBOMB",		ET_CLASSEX_SMOKEBOMB),
	IntEnum("SMOKEMARKER",		ET_CLASSEX_SMOKEMARKER),
	IntEnum("VEHICLE",			ET_CLASSEX_VEHICLE),
	IntEnum("VEHICLE_HVY",		ET_CLASSEX_VEHICLE_HVY),
	IntEnum("VEHICLE_NODAMAGE",	ET_CLASSEX_VEHICLE_NODAMAGE),
	IntEnum("BREAKABLE",		ET_CLASSEX_BREAKABLE),
	IntEnum("CORPSE",			ET_CLASSEX_CORPSE),
	//IntEnum("INJUREDPLAYER",	ET_CLASSEX_INJUREDPLAYER),
	IntEnum("TRIPMINE",			ET_CLASSEX_TREASURE), // cs: to avoid breaking mod compat
	IntEnum("ROCKET",			ET_CLASSEX_ROCKET),
	IntEnum("MORTAR_ENT",		ET_CLASSEX_MORTAR),
	IntEnum("FLAME",			ET_CLASSEX_FLAMECHUNK),
	IntEnum("ARTY",				ET_CLASSEX_ARTY),
	IntEnum("AIRSTRIKE",		ET_CLASSEX_AIRSTRIKE),
	IntEnum("HEALTHCABINET",	ET_CLASSEX_HEALTHCABINET),
	IntEnum("AMMOCABINET",		ET_CLASSEX_AMMOCABINET),
	IntEnum("GRENADE",			ET_CLASSEX_GRENADE),
	IntEnum("M7_GRENADE",		ET_CLASSEX_M7_GRENADE),
	IntEnum("GPG40_GRENADE",	ET_CLASSEX_GPG40_GRENADE),
	IntEnum("BROKENCHAIR",		ET_CLASSEX_BROKENCHAIR),
};

const char *ET_Game::FindClassName(obint32 _classId)
{
	if(ET_Game::CLASSEXoffset == 2) //ETBlight
	{
		if(_classId > 7 && _classId < 10000) _classId -= 2;
		else if(_classId == 6) return "SCIENTIST";
		else if(_classId == 7) return "SUPER_SOLDIER";
	}

	obint32 iNumMappings = sizeof(ET_ClassEnum) / sizeof(ET_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(ET_ClassEnum[i].m_Value == _classId)
			return ET_ClassEnum[i].m_Key;
	}
	return IGame::FindClassName(_classId);
}

void ET_Game::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptClasses(_machine, _table);

	FilterSensory::ANYPLAYERCLASS = ET_CLASS_ANY + ET_Game::CLASSEXoffset;

	obint32 iNumMappings = sizeof(ET_ClassEnum) / sizeof(ET_ClassEnum[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		int value = ET_ClassEnum[i].m_Value;
		if(value >= ET_CLASS_ANY) value += ET_Game::CLASSEXoffset;
		_table->Set(_machine, ET_ClassEnum[i].m_Key, gmVariable(value));
	}

	if(ET_Game::CLASSEXoffset == 2) //ETBlight
	{
		_table->Set(_machine, "SCIENTIST", gmVariable(6));
		_table->Set(_machine, "SUPER_SOLDIER", gmVariable(7));
	}

	InitScriptWeaponClasses(_machine,_table, ET_CLASSEX_WEAPON + ET_Game::CLASSEXoffset);
}

void ET_Game::InitScriptSkills(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptSkills(_machine, _table);

	_table->Set(_machine, "BATTLE_SENSE",	gmVariable(ET_SKILL_BATTLE_SENSE));
	_table->Set(_machine, "ENGINEERING",	gmVariable(ET_SKILL_ENGINEERING));
	_table->Set(_machine, "FIRST_AID",		gmVariable(ET_SKILL_FIRST_AID));
	_table->Set(_machine, "SIGNALS",		gmVariable(ET_SKILL_SIGNALS));
	_table->Set(_machine, "LIGHT_WEAPONS",	gmVariable(ET_SKILL_LIGHT_WEAPONS));
	_table->Set(_machine, "HEAVY_WEAPONS",	gmVariable(ET_SKILL_HEAVY_WEAPONS));
	_table->Set(_machine, "COVERTOPS",		gmVariable(ET_SKILL_COVERTOPS));
}

void ET_Game::InitVoiceMacros(gmMachine *_machine, gmTableObject *_table)
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

	_table->Set(_machine, "P_DEPLOYMORTAR",			gmVariable(VCHAT_PRIVATE_DEPLOYMORTAR));
	_table->Set(_machine, "P_HEALSQUAD",			gmVariable(VCHAT_PRIVATE_HEALSQUAD));
	_table->Set(_machine, "P_HEALME",				gmVariable(VCHAT_PRIVATE_HEALME));
	_table->Set(_machine, "P_REVIVETEAMMATE",		gmVariable(VCHAT_PRIVATE_REVIVETEAMMATE));
	_table->Set(_machine, "P_REVIVEME",				gmVariable(VCHAT_PRIVATE_REVIVEME));
	_table->Set(_machine, "P_DESTROYOBJECTIVE",		gmVariable(VCHAT_PRIVATE_DESTROYOBJECTIVE));
	_table->Set(_machine, "P_REPAIROBJECTIVE",		gmVariable(VCHAT_PRIVATE_REPAIROBJECTIVE));
	_table->Set(_machine, "P_CONSTRUCTOBJECTIVE",	gmVariable(VCHAT_PRIVATE_CONSTRUCTOBJECTIVE));
	_table->Set(_machine, "P_DEPLOYLANDMINES",		gmVariable(VCHAT_PRIVATE_DEPLOYLANDMINES));
	_table->Set(_machine, "P_DISARMLANDMINES",		gmVariable(VCHAT_PRIVATE_DISARMLANDMINES));
	_table->Set(_machine, "P_CALLAIRSTRIKE",		gmVariable(VCHAT_PRIVATE_CALLAIRSTRIKE));
	_table->Set(_machine, "P_CALLARTILLERY",		gmVariable(VCHAT_PRIVATE_CALLARTILLERY));
	_table->Set(_machine, "P_RESUPPLYSQUAD",		gmVariable(VCHAT_PRIVATE_RESUPPLYSQUAD));
	_table->Set(_machine, "P_RESUPPLYME",			gmVariable(VCHAT_PRIVATE_RESUPPLYME));
	_table->Set(_machine, "P_EXPLOREAREA",			gmVariable(VCHAT_PRIVATE_EXPLOREAREA));
	_table->Set(_machine, "P_CHECKFORLANDMINES",	gmVariable(VCHAT_PRIVATE_CHECKFORLANDMINES));
	_table->Set(_machine, "P_SATCHELOBJECTIVE",		gmVariable(VCHAT_PRIVATE_SATCHELOBJECTIVE));
	_table->Set(_machine, "P_INFILTRATE",			gmVariable(VCHAT_PRIVATE_INFILTRATE));
	_table->Set(_machine, "P_GOUNDERCOVER",			gmVariable(VCHAT_PRIVATE_GOUNDERCOVER));
	_table->Set(_machine, "P_PROVIDESNIPERCOVER",	gmVariable(VCHAT_PRIVATE_PROVIDESNIPERCOVER));
	_table->Set(_machine, "P_ATTACK",				gmVariable(VCHAT_PRIVATE_ATTACK));
	_table->Set(_machine, "P_FALLBACK",				gmVariable(VCHAT_PRIVATE_FALLBACK));
}

void ET_Game::AddBot(Msg_Addbot &_addbot, bool _createnow)
{
	if(_createnow && !NavigationManager::GetInstance()->GetCurrentPathPlanner()->IsReady())
	{
		EngineFuncs::ConsoleError(va("No navigation file loaded, unable to add bots."));
		return;
	}
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
		g_EngineFuncs->ChangeTeam(iGameID, cp->m_DesiredTeam, NULL);
		if (!cp) return; //some mods can kick bot in ClientUserinfoChanged
		g_EngineFuncs->ChangeClass(iGameID, cp->m_DesiredClass, NULL);		
		if (!cp) return;

		cp->CheckTeamEvent();
		cp->CheckClassEvent();

		ScriptManager::GetInstance()->ExecBotCallback(cp.get(), "SelectWeapons");
	}
}

// package: ET Script Events
//		Custom Events for Enemy Territory. Also see <Common Script Events>
void ET_Game::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "FIRETEAM_CHAT_MSG", gmVariable(PERCEPT_HEAR_PRIVCHATMSG));	

	_table->Set(_machine, "PRETRIGGERED_MINE", gmVariable(ET_EVENT_PRETRIGGER_MINE));
	_table->Set(_machine, "POSTTRIGGERED_MINE", gmVariable(ET_EVENT_POSTTRIGGER_MINE));
	_table->Set(_machine, "MORTAR_IMPACT", gmVariable(ET_EVENT_MORTAR_IMPACT));

	_table->Set(_machine, "FIRETEAM_CREATED", gmVariable(ET_EVENT_FIRETEAM_CREATED));
	_table->Set(_machine, "FIRETEAM_DISBANDED", gmVariable(ET_EVENT_FIRETEAM_DISBANDED));
	_table->Set(_machine, "FIRETEAM_JOINED", gmVariable(ET_EVENT_FIRETEAM_JOINED));
	_table->Set(_machine, "FIRETEAM_LEFT", gmVariable(ET_EVENT_FIRETEAM_LEFT));
	_table->Set(_machine, "FIRETEAM_INVITED", gmVariable(ET_EVENT_FIRETEAM_INVITED));
	_table->Set(_machine, "FIRETEAM_PROPOSAL", gmVariable(ET_EVENT_FIRETEAM_PROPOSAL));	
	_table->Set(_machine, "FIRETEAM_WARNED", gmVariable(ET_EVENT_FIRETEAM_WARNED));
	_table->Set(_machine, "AMMO_RECIEVED", gmVariable(ET_EVENT_RECIEVEDAMMO));

	IGame::InitScriptEvents(_machine, _table);
}

void ET_Game::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	IGame::InitScriptEntityFlags(_machine, _table);

	_table->Set(_machine, "DISGUISED",		gmVariable(ET_ENT_FLAG_DISGUISED));
	_table->Set(_machine, "MOUNTED",		gmVariable(ET_ENT_FLAG_MOUNTED));
	_table->Set(_machine, "MNT_MG42",		gmVariable(ET_ENT_FLAG_MNT_MG42));
	_table->Set(_machine, "MNT_TANK",		gmVariable(ET_ENT_FLAG_MNT_TANK));
	_table->Set(_machine, "MNT_AAGUN",		gmVariable(ET_ENT_FLAG_MNT_AAGUN));
	_table->Set(_machine, "CARRYINGGOAL",	gmVariable(ET_ENT_FLAG_CARRYINGGOAL));	
	_table->Set(_machine, "LIMBO",			gmVariable(ET_ENT_FLAG_INLIMBO));
	_table->Set(_machine, "MOUNTABLE",		gmVariable(ET_ENT_FLAG_ISMOUNTABLE));
	_table->Set(_machine, "POISONED",		gmVariable(ET_ENT_FLAG_POISONED));
}

void ET_Game::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
}

void ET_Game::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	// Should always register the default flags
	IGame::RegisterNavigationFlags(_planner);

	_planner->RegisterNavFlag("AXIS",			F_NAV_TEAM1);
	_planner->RegisterNavFlag("ALLIES",			F_NAV_TEAM2);

	_planner->RegisterNavFlag("MOBILEMG42",		F_ET_NAV_MG42SPOT);
	//_planner->RegisterNavFlag("PANZERFAUST",	F_ET_NAV_PANZERFSPOT);

	_planner->RegisterNavFlag("MOBILEMORTAR",	F_ET_NAV_MORTAR);
	_planner->RegisterNavFlag("PLANTMINE",		F_ET_NAV_MINEAREA);
	//_planner->RegisterNavFlag("AIRSTRIKE",	F_ET_NAV_AIRSTRAREA);

	_planner->RegisterNavFlag("BLOCKWALL",		F_ET_NAV_WALL);
	_planner->RegisterNavFlag("BLOCKBRIDGE",	F_ET_NAV_BRIDGE);
	_planner->RegisterNavFlag("BLOCKWATER",		F_ET_NAV_WATERBLOCKABLE);

	_planner->RegisterNavFlag("SPRINT",			F_ET_NAV_SPRINT);
	_planner->RegisterNavFlag("PRONE",			F_NAV_PRONE);
	_planner->RegisterNavFlag("CAPPOINT",		F_ET_NAV_CAPPOINT);
	_planner->RegisterNavFlag("CALLARTILLERY",	F_ET_NAV_ARTSPOT);	
	_planner->RegisterNavFlag("ARTILLERY_S",	F_ET_NAV_ARTYTARGET_S);	
	_planner->RegisterNavFlag("ARTILLERY_D",	F_ET_NAV_ARTYTARGET_D);
	_planner->RegisterNavFlag("DISGUISE",		F_ET_NAV_DISGUISE);
	_planner->RegisterNavFlag("FLAME",			F_ET_NAV_FLAMETHROWER);
	_planner->RegisterNavFlag("PANZER",			F_ET_NAV_PANZER);
	_planner->RegisterNavFlag("STRAFE_L",		F_ET_NAV_STRAFE_L);
	_planner->RegisterNavFlag("STRAFE_R",		F_ET_NAV_STRAFE_R);
	_planner->RegisterNavFlag("UGOAL",			F_ET_NAV_USERGOAL);
	_planner->RegisterNavFlag("USEPATH",		F_ET_NAV_USEPATH);
}

NavFlags ET_Game::DeprecatedNavigationFlags() const
{
	return IGame::DeprecatedNavigationFlags() | F_NAV_TEAM3 | F_NAV_TEAM4 | F_ET_NAV_MG42SPOT | F_ET_NAV_MORTAR 
		| F_ET_NAV_MINEAREA | F_ET_NAV_CAPPOINT | F_ET_NAV_ARTSPOT | F_ET_NAV_ARTYTARGET_S | F_ET_NAV_ARTYTARGET_D 
		| F_ET_NAV_FLAMETHROWER | F_ET_NAV_PANZER | F_ET_NAV_USERGOAL;
}

void ET_Game::InitCommands()
{
	IGame::InitCommands();
}

const void ET_Game::ET_GetEntityVisDistance(float &_distance, const TargetInfo &_target, const Client *_client)
{
	switch(_target.m_EntityClass)
	{
	case ENT_CLASS_GENERIC_AMMO:
		_distance = 2000.0f;
		break;
	case ENT_CLASS_GENERIC_HEALTH:
		_distance = 1000.0f;
		break;
	default:
		if(_target.m_EntityClass - ET_Game::CLASSEXoffset == ET_CLASSEX_BREAKABLE)
			_distance = static_cast<const ET_Client*>(_client)->GetBreakableTargetDist();
		else if(_target.m_EntityCategory.CheckFlag(ENT_CAT_PICKUP_WEAPON))
			_distance = 1500.0f;
		else if(_target.m_EntityCategory.CheckFlag(ENT_CAT_PROJECTILE))
			_distance = 500.0f;
	}
}

/*	
	bounding boxes for et
	standing	(-18, -18, -24) x (18, 18, 48)
	crouched	(-18, -18, -24) x (18, 18, 24)
	proned		(-18, -18, -24) x (18, 18, 16)
*/
const float ET_Game::ET_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > ET_CLASS_NULL && _class < FilterSensory::ANYPLAYERCLASS)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 16.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 24.0f;
		return 48.0f;
	}

	switch(_class - ET_Game::CLASSEXoffset)
	{
	case ET_CLASSEX_DYNAMITE:
	case ET_CLASSEX_MINE:
	case ET_CLASSEX_SATCHEL:
	case ET_CLASSEX_SMOKEBOMB:		
	case ET_CLASSEX_CORPSE:
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
const float ET_Game::ET_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags)
{
	if (_class > ET_CLASS_NULL && _class < FilterSensory::ANYPLAYERCLASS)
	{
		if (_entflags.CheckFlag(ENT_FLAG_PRONED))
			return 8.0f;
		if (_entflags.CheckFlag(ENT_FLAG_CROUCHED))
			return 16.0f;
		return 40.0f;
	}

	return 0.0f;
}

const float ET_Game::ET_GetEntityClassAvoidRadius(const int _class)
{
	switch(_class - ET_Game::CLASSEXoffset) 
	{
	case ET_CLASSEX_DYNAMITE:		
		return 400.0f;
	case ET_CLASSEX_MINE:
		return 225.0f;
	case ET_CLASSEX_SATCHEL:
		return 300.0f;
	case ET_CLASSEX_SMOKEBOMB:
		break;
	}
	return 0.0f;
}

void ET_Game::ET_AddSensorCategory(BitFlag32 category)
{
	if(category.CheckFlag(ET_ENT_CAT_MINE)) m_WatchForMines = true;
}

const bool ET_Game::ET_CanSensoreEntity(const EntityInstance &_ent)
{
	if((((1<<ENT_CAT_PICKUP_HEALTH)|(1<<ENT_CAT_PICKUP_AMMO)|(1<<ENT_CAT_PICKUP_WEAPON)|(1<<ENT_CAT_PROJECTILE)|(1<<ENT_CAT_SHOOTABLE)|(1<<ET_ENT_CAT_MINE))
		& _ent.m_EntityCategory.GetRawFlags()) == 0)
		return false;

	int c =_ent.m_EntityClass - ET_Game::CLASSEXoffset;
	return c<ET_CLASS_ANY || (c!=ET_CLASSEX_GPG40_GRENADE && c!=ET_CLASSEX_M7_GRENADE && 
		c!=ET_CLASSEX_ARTY && c!=ET_CLASSEX_SMOKEBOMB && c!=ET_CLASSEX_FLAMECHUNK && c!=ET_CLASSEX_ROCKET &&
		(c!=ET_CLASSEX_MINE || m_WatchForMines));
}

void ET_Game::ClientJoined(const Event_SystemClientConnected *_msg)
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
			if(!cp) return; //some mods can kick bot in ClientUserinfoChanged
			g_EngineFuncs->ChangeTeam(_msg->m_GameId, cp->m_DesiredTeam, NULL);
			if(!cp) return;

			cp->CheckTeamEvent();
			cp->CheckClassEvent();
		}
	}	
}

PathPlannerWaypoint::BlockableStatus ET_PathCheck(const Waypoint* _wp1, const Waypoint* _wp2, bool _draw)
{
	static bool bRender = false;
	PathPlannerWaypoint::BlockableStatus res = PathPlannerWaypoint::B_INVALID_FLAGS;

	Vector3f vStart, vEnd;

	if(/*_wp1->IsFlagOn(F_ET_NAV_WALL) &&*/ _wp2->IsFlagOn(F_ET_NAV_WALL))
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

	if(res != PathPlannerWaypoint::B_PATH_CLOSED && _wp1->IsFlagOn(F_ET_NAV_BRIDGE) && _wp2->IsFlagOn(F_ET_NAV_BRIDGE))
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

	if(res != PathPlannerWaypoint::B_PATH_CLOSED && _wp2->IsFlagOn(F_ET_NAV_WATERBLOCKABLE))
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

void ET_Game::RegisterPathCheck(PathPlannerWaypoint::pfbWpPathCheck &_pfnPathCheck)
{
	_pfnPathCheck = ET_PathCheck;
}

int ET_Game::GetLogSize()
{
	return InterfaceFuncs::GetCvar("omnibot_logsize");
}

int ET_Game::ConvertWeaponId(int weaponId)
{
	if(IsNoQuarter)
	{
		switch(weaponId) {
			case 88: //MOBILE_BROWNING
				weaponId=ET_WP_MOBILE_MG42;
				break;
			case 89: //MOBILE_BROWNING_SET
				weaponId=ET_WP_MOBILE_MG42_SET;
				break;
			case 92: //GRANATWERFER
				weaponId=ET_WP_MORTAR;
				break;
			case 93:; //GRANATWERFER_SET
				weaponId=ET_WP_MORTAR_SET;
				break;
			case 94: //KNIFE_KABAR
				weaponId=ET_WP_KNIFE;
				break;
		}
	}
	return weaponId;
}
