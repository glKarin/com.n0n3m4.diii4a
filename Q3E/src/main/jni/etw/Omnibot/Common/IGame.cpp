#include "PrecompCommon.h"
#include "NameManager.h"
#include "ScriptManager.h"
#include "NavigationManager.h"
#include "NavigationFlags.h"
#include "WeaponDatabase.h"
#include "MapGoalDatabase.h"
#include "IGame.h"
#include "BotBaseStates.h"
#include "BotGlobalStates.h"
#include "BotSteeringSystem.h"
#include "ScriptGoal.h"

BlackBoard	g_Blackboard;

obint32 IGame::m_GameMsec = 0;
obint32 IGame::m_DeltaMsec = 0;
obint32 IGame::m_StartTimeMsec = 0;
obint32 IGame::m_GameFrame = 0;
obReal IGame::m_Gravity = 0;
bool IGame::m_CheatsEnabled = false;
bool IGame::m_BotJoining = false;

IGame::GameVars IGame::m_GameVars;

GameState IGame::m_GameState		= GAME_STATE_INVALID;
GameState IGame::m_LastGameState	= GAME_STATE_INVALID;

IGame::GameVars::GameVars()
	: mPlayerHeight(100)
{
}

EntityInstance IGame::m_GameEntities[Constants::MAX_ENTITIES];
int IGame::m_MaxEntity = 0;

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<AiState::ScriptGoal> ScriptGoalPtr;
#else
typedef boost::shared_ptr<AiState::ScriptGoal> ScriptGoalPtr;
#endif
typedef std::list<ScriptGoalPtr> ScriptGoalList;
ScriptGoalList g_ScriptGoalList;

//SoundDepot g_SoundDepot;

IGame::IGame()
	: m_StateRoot(0)
	, m_NumDeletedThreads(0)
	, m_WeaponClassIdStart(0)
	, m_PlayersChanged(true)
	, m_bDrawBlockableTests	(false)
{
}

IGame::~IGame()
{
}

const char *IGame::GetVersion() const
{
	return "0.91";
}

const char *IGame::GetVersionDateTime() const
{
	return __DATE__ " " __TIME__;
}

bool IGame::CheckVersion(int _version)
{
	return _version == GetVersionNum();
}

GoalManager *IGame::GetGoalManager()
{
	return new GoalManager;
}

bool IGame::Init() 
{
	GetGameVars(m_GameVars);

	m_StartTimeMsec = g_EngineFuncs->GetGameTime();

	for(int i = 0; i < MaxDeletedThreads; ++i)
	{
		m_DeletedThreads[i] = GM_INVALID_THREAD;
	}

	for(int i = 0; i < m_MaxEntity; ++i)
	{
		m_GameEntities[i].m_Entity = GameEntity();
		m_GameEntities[i].m_EntityClass = 0;
		m_GameEntities[i].m_EntityCategory.ClearAll();
		m_GameEntities[i].m_TimeStamp = 0;
	}
	m_MaxEntity = 0;

	InitCommands();
	InitScriptSupport();

	g_WeaponDatabase.LoadWeaponDefinitions(true);
	g_MapGoalDatabase.LoadMapGoalDefinitions(true);

	// Reset the global blackboard.
	g_Blackboard.RemoveAllBBRecords(bbk_All);

	//////////////////////////////////////////////////////////////////////////
	/*m_StateRoot = new AiState::GlobalRoot;
	m_StateRoot->FixRoot();
	InitGlobalStates();
	m_StateRoot->FixRoot();
	m_StateRoot->InitializeStates();*/
	//////////////////////////////////////////////////////////////////////////
	
	return true;
}

void IGame::Shutdown()
{
	if(GameStarted())
	{
		EndGame();
		m_LastGameState = m_GameState = GAME_STATE_INVALID;
	}

	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
	{
		if(m_ClientList[i])
		{
			m_ClientList[i]->Shutdown();
			m_ClientList[i].reset();
		}
	}

	g_MapGoalDatabase.Unload();
	g_ScriptGoalList.clear();

	OB_DELETE(m_StateRoot);
}

void IGame::InitScriptSupport()
{
	LOGFUNCBLOCK;
	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

	// Bind scripts
	LOG("Initializing Game Bindings...");
	InitScriptBinds(pMachine);
	LOG("done.");

	// Register Script Constants.
	LOG("Registering Script Constants...");

	DisableGCInScope gcEn(pMachine);

	// Register Teams to Script.
	gmTableObject *pTeamTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "TEAM", gmVariable(pTeamTable));
	InitScriptTeams(pMachine, pTeamTable);

	// Register Weapons to Script.
	gmTableObject *pWeaponTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "WEAPON", gmVariable(pWeaponTable));
	InitScriptWeapons(pMachine, pWeaponTable);

	// Register Items to Script.
	gmTableObject *pItemTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "ITEM", gmVariable(pItemTable));
	InitScriptItems(pMachine, pItemTable);

	// Register Classes to Script
	gmTableObject *pClassTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "CLASS", gmVariable(pClassTable));
	InitScriptClasses(pMachine, pClassTable);

	// Register Roles to Script
	gmTableObject *pRoleTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "ROLE", gmVariable(pRoleTable));
	InitScriptRoles(pMachine, pRoleTable);

	// Register Skills to Script
	gmTableObject *pSkillTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "SKILL", gmVariable(pSkillTable));
	InitScriptSkills(pMachine, pSkillTable);

	// Register Events to Script.
	gmTableObject *pEventTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "EVENT", gmVariable(pEventTable));
	InitScriptEvents(pMachine, pEventTable);

	// Register the entity flags
	gmTableObject *pEntityFlags = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "ENTFLAG", gmVariable(pEntityFlags));
	InitScriptEntityFlags(pMachine, pEntityFlags);

	// Register the powerups.
	gmTableObject *pPowerUps = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "POWERUP", gmVariable(pPowerUps));
	InitScriptPowerups(pMachine, pPowerUps);

	// Register the entity categories
	gmTableObject *pCategoryTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "CAT", gmVariable(pCategoryTable));
	InitScriptCategories(pMachine, pCategoryTable);

	// Init the buttons table.
	gmTableObject *pButtonsTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "BTN", gmVariable(pButtonsTable));	
	InitScriptBotButtons(pMachine, pButtonsTable);

	// Init the trace masks table.
	gmTableObject *pTraceMasks = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "TRACE", gmVariable(pTraceMasks));	
	InitScriptTraceMasks(pMachine, pTraceMasks);

	// Register Voice queues.
	gmTableObject *pVoiceTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "VOICE",	gmVariable(pVoiceTable));
	InitVoiceMacros(pMachine, pVoiceTable);

	// Register Debug Flags
	gmTableObject *pDebugTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "DEBUG",	gmVariable(pDebugTable));
	InitDebugFlags(pMachine, pDebugTable);

	// Register contents flags
	gmTableObject *pContentTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "CONTENT", gmVariable(pContentTable));
	InitScriptContentFlags(pMachine, pContentTable);

	// Register surface flags
	gmTableObject *pSurfaceTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "SURFACE", gmVariable(pSurfaceTable));
	InitScriptSurfaceFlags(pMachine, pSurfaceTable);

	gmTableObject *pBlackboardTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "BB", gmVariable(pBlackboardTable));
	InitScriptBlackboardKeys(pMachine, pBlackboardTable);

	gmTableObject *pBoneTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "BONE", gmVariable(pBoneTable));
	InitBoneIds(pMachine, pBoneTable);

	gmTableObject *pBuyTable = pMachine->AllocTableObject();
	pMachine->GetGlobals()->Set(pMachine, "BUY", gmVariable(pBuyTable));
	InitScriptBuyMenu(pMachine, pBuyTable);

	// Register Profile Types
	gmTableObject *pProfileTable = pMachine->AllocTableObject();
	pProfileTable->Set(pMachine, "NONE", gmVariable(Client::PROFILE_NONE));
	pProfileTable->Set(pMachine, "CLASS", gmVariable(Client::PROFILE_CLASS));
	pProfileTable->Set(pMachine, "CUSTOM", gmVariable(Client::PROFILE_CUSTOM));
	pMachine->GetGlobals()->Set(pMachine, "PROFILE", gmVariable(pProfileTable));

	LOG("done.");
}

void IGame::InitScriptTeams(gmMachine *_machine, gmTableObject *_table)
{
	int NumElements = 0;
	const IntEnum *Enum = 0;
	GetTeamEnumeration(Enum,NumElements);

	for(int i = 0; i < NumElements; ++i)
	{
		_table->Set(_machine, Enum[i].m_Key, gmVariable(Enum[i].m_Value));
	}
}

void IGame::InitScriptWeapons(gmMachine *_machine, gmTableObject *_table)
{
	int NumElements = 0;
	const IntEnum *Enum = 0;
	GetWeaponEnumeration(Enum,NumElements);

	for(int i = 0; i < NumElements; ++i)
	{
		_table->Set(_machine, Enum[i].m_Key, gmVariable(Enum[i].m_Value));
	}
}

void IGame::InitScriptRoles(gmMachine *_machine, gmTableObject *_table)
{
	int NumElements = 0;
	const IntEnum *Enum = 0;
	GetRoleEnumeration(Enum,NumElements);

	for(int i = 0; i < NumElements; ++i)
	{
		_table->Set(_machine, Enum[i].m_Key, gmVariable(Enum[i].m_Value));
	}
}

static IntEnum Generic_RoleEnum[] =
{
	IntEnum("ATTACKER"			,0),
	IntEnum("DEFENDER"			,1),
	IntEnum("ROAMER"			,2),
	IntEnum("INFILTRATOR"		,3), // the constant must be same as in functions Client.IsInfiltrator and gmBot::gmfClearRoles
	IntEnum("SNIPER"			,4),
	IntEnum("AMBUSHER"			,5),
	IntEnum("TEAMCAPTAIN"		,6),
	IntEnum("FIRETEAMCAPTAIN"	,7),
	IntEnum("OFFENSECAPTAIN"	,8),
	IntEnum("DEFENSECAPTAIN"	,9),
	IntEnum("DEFENDER1"		,10),
	IntEnum("DEFENDER2"		,11),
	IntEnum("DEFENDER3"		,12),
	IntEnum("ATTACKER1"		,13),
	IntEnum("ATTACKER2"		,14),
	IntEnum("ATTACKER3"		,15),
	IntEnum("ESCORT"			,16),
};

void IGame::GetRoleEnumeration(const IntEnum *&_ptr, int &num)
{
	num = sizeof(Generic_RoleEnum) / sizeof(Generic_RoleEnum[0]);
	_ptr = Generic_RoleEnum;	
}

// package: Common Script Events
//		Common Events for Every Game.
void IGame::InitScriptEvents(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "DISCONNECTED",	gmVariable(GAME_CLIENTDISCONNECTED));

	_table->Set(_machine, "START_GAME",		gmVariable(GAME_STARTGAME));
	_table->Set(_machine, "END_GAME",		gmVariable(GAME_ENDGAME));
	_table->Set(_machine, "NEW_ROUND",		gmVariable(GAME_NEWROUND));
	_table->Set(_machine, "END_ROUND",		gmVariable(GAME_ENDROUND));	
	_table->Set(_machine, "GOAL_SUCCESS",	gmVariable(GOAL_SUCCESS));
	_table->Set(_machine, "GOAL_FAILED",	gmVariable(GOAL_FAILED));
	_table->Set(_machine, "GOAL_ABORTED",	gmVariable(GOAL_ABORTED));
	_table->Set(_machine, "PATH_SUCCESS",	gmVariable(PATH_SUCCESS));
	_table->Set(_machine, "PATH_FAILED",	gmVariable(PATH_FAILED));
	_table->Set(_machine, "AIM_SUCCESS",	gmVariable(AIM_SUCCESS));	
	_table->Set(_machine, "SCRIPTMSG",		gmVariable(MESSAGE_SCRIPTMSG));		
	_table->Set(_machine, "SPAWNED",		gmVariable(MESSAGE_SPAWN));
	_table->Set(_machine, "CHANGETEAM",		gmVariable(MESSAGE_CHANGETEAM));
	_table->Set(_machine, "CHANGECLASS",	gmVariable(MESSAGE_CHANGECLASS));
	_table->Set(_machine, "INVALID_TEAM",	gmVariable(MESSAGE_INVALIDTEAM));
	_table->Set(_machine, "INVALID_CLASS",	gmVariable(MESSAGE_INVALIDCLASS));
	_table->Set(_machine, "DEATH",			gmVariable(MESSAGE_DEATH));
	_table->Set(_machine, "HEALED",			gmVariable(MESSAGE_HEALED));
	_table->Set(_machine, "REVIVED",		gmVariable(MESSAGE_REVIVED));	
	_table->Set(_machine, "ADDWEAPON",		gmVariable(MESSAGE_ADDWEAPON));
	_table->Set(_machine, "REMOVEWEAPON",	gmVariable(MESSAGE_REMOVEWEAPON));
	_table->Set(_machine, "RESET_WEAPONS",	gmVariable(MESSAGE_RESETWEAPONS));
	_table->Set(_machine, "REFRESH_WEAPON",	gmVariable(MESSAGE_REFRESHWEAPON));	
	_table->Set(_machine, "KILLEDSOMEONE",	gmVariable(MESSAGE_KILLEDSOMEONE));
	_table->Set(_machine, "PROXIMITY_TRIGGER",gmVariable(MESSAGE_PROXIMITY_TRIGGER));
	_table->Set(_machine, "ENT_ENTER_RADIUS",gmVariable(MESSAGE_ENT_ENTER_RADIUS));
	_table->Set(_machine, "ENT_LEAVE_RADIUS",gmVariable(MESSAGE_ENT_LEAVE_RADIUS));
	_table->Set(_machine, "MG_ENTER_RADIUS",gmVariable(MESSAGE_MG_ENTER_RADIUS));
	_table->Set(_machine, "MG_LEAVE_RADIUS",gmVariable(MESSAGE_MG_LEAVE_RADIUS));
	_table->Set(_machine, "PLAYER_USE",		gmVariable(PERCEPT_FEEL_PLAYER_USE));	
	_table->Set(_machine, "FEEL_PAIN",		gmVariable(PERCEPT_FEEL_PAIN));
	_table->Set(_machine, "SENSE_ENTITY",	gmVariable(PERCEPT_SENSE_ENTITY));
	_table->Set(_machine, "WEAPON_FIRE",	gmVariable(ACTION_WEAPON_FIRE));
	_table->Set(_machine, "WEAPON_CHANGE",	gmVariable(ACTION_WEAPON_CHANGE));
	_table->Set(_machine, "GLOBAL_VOICE",	gmVariable(PERCEPT_HEAR_GLOBALVOICEMACRO));
	_table->Set(_machine, "TEAM_VOICE",		gmVariable(PERCEPT_HEAR_TEAMVOICEMACRO));
	_table->Set(_machine, "PRIVATE_VOICE",	gmVariable(PERCEPT_HEAR_PRIVATEVOICEMACRO));
	_table->Set(_machine, "GLOBAL_CHAT_MSG",gmVariable(PERCEPT_HEAR_GLOBALCHATMSG));
	_table->Set(_machine, "TEAM_CHAT_MSG",	gmVariable(PERCEPT_HEAR_TEAMCHATMSG));
	_table->Set(_machine, "PRIV_CHAT_MSG",	gmVariable(PERCEPT_HEAR_PRIVCHATMSG));
	_table->Set(_machine, "GROUP_CHAT_MSG",	gmVariable(PERCEPT_HEAR_GROUPCHATMSG));	
	_table->Set(_machine, "HEAR_SOUND",		gmVariable(PERCEPT_HEAR_SOUND));	
}

void IGame::InitScriptCategories(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "PLAYER",		gmVariable(ENT_CAT_PLAYER));
	_table->Set(_machine, "VEHICLE",	gmVariable(ENT_CAT_VEHICLE));
	_table->Set(_machine, "PROJECTILE",	gmVariable(ENT_CAT_PROJECTILE));
	_table->Set(_machine, "SHOOTABLE",	gmVariable(ENT_CAT_SHOOTABLE));
	_table->Set(_machine, "PICKUP",		gmVariable(ENT_CAT_PICKUP));
	_table->Set(_machine, "PICKUP_AMMO",gmVariable(ENT_CAT_PICKUP_AMMO));
	_table->Set(_machine, "PICKUP_WEAPON",gmVariable(ENT_CAT_PICKUP_WEAPON));
	_table->Set(_machine, "PICKUP_HEALTH",gmVariable(ENT_CAT_PICKUP_HEALTH));
	_table->Set(_machine, "PICKUP_ENERGY",gmVariable(ENT_CAT_PICKUP_ENERGY));
	_table->Set(_machine, "PICKUP_ARMOR",gmVariable(ENT_CAT_PICKUP_ARMOR));
	
	_table->Set(_machine, "TRIGGER",	gmVariable(ENT_CAT_TRIGGER));
	_table->Set(_machine, "MOVER",		gmVariable(ENT_CAT_MOVER));
	_table->Set(_machine, "AVOID",		gmVariable(ENT_CAT_AVOID));
	_table->Set(_machine, "MOUNTED_WPN",gmVariable(ENT_CAT_MOUNTEDWEAPON));	
	_table->Set(_machine, "MISC",		gmVariable(ENT_CAT_MISC));	
	_table->Set(_machine, "STATIC",		gmVariable(ENT_CAT_STATIC));
	_table->Set(_machine, "PROP",		gmVariable(ENT_CAT_PROP));	
	_table->Set(_machine, "AUTODEFENSE",gmVariable(ENT_CAT_AUTODEFENSE));
	_table->Set(_machine, "OBSTACLE",	gmVariable(ENT_CAT_OBSTACLE));
}

IntEnum g_BaseClassMappings[] =
{
	IntEnum("SPECTATOR",		ENT_CLASS_GENERIC_SPECTATOR),
	IntEnum("PLAYERSTART",		ENT_CLASS_GENERIC_PLAYERSTART),
	IntEnum("BUTTON",			ENT_CLASS_GENERIC_BUTTON),
	IntEnum("HEALTH",			ENT_CLASS_GENERIC_HEALTH),
	IntEnum("ARMOR",			ENT_CLASS_GENERIC_ARMOR),	
	IntEnum("AMMO",				ENT_CLASS_GENERIC_AMMO),
	IntEnum("LADDER",			ENT_CLASS_GENERIC_LADDER),
	IntEnum("TELEPORTER",		ENT_CLASS_GENERIC_TELEPORTER),
	IntEnum("LIFT",				ENT_CLASS_GENERIC_LIFT),
	IntEnum("MOVER",			ENT_CLASS_GENERIC_MOVER),
	IntEnum("EXPLODING_BARREL",	ENT_CLASS_EXPLODING_BARREL),
	IntEnum("JUMPPAD",			ENT_CLASS_GENERIC_JUMPPAD),	
	IntEnum("JUMPPAD_TARGET",	ENT_CLASS_GENERIC_JUMPPAD_TARGET),
	IntEnum("GOAL",				ENT_CLASS_GENERIC_GOAL),
	IntEnum("WEAPON",			ENT_CLASS_GENERIC_WEAPON),
	IntEnum("FLAG",				ENT_CLASS_GENERIC_FLAG),
	IntEnum("CAPPOINT",			ENT_CLASS_GENERIC_FLAGCAPPOINT),

	IntEnum("PROP",				ENT_CLASS_GENERIC_PROP),
	IntEnum("PROP_EXPLODE",		ENT_CLASS_GENERIC_PROP_EXPLODE),
};

const char *IGame::FindClassName(obint32 _classId)
{
	obint32 iNumMappings = sizeof(g_BaseClassMappings) / sizeof(g_BaseClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		if(g_BaseClassMappings[i].m_Value == _classId)
			return g_BaseClassMappings[i].m_Key;
	}
	// if this is set, we should do additional checks to see if the classId represents a weapon class
	if(m_WeaponClassIdStart!=0)
	{
		// see if it's a weapon id
		int numWpns = 0;
		const IntEnum *wpnEnum = 0;
		GetWeaponEnumeration(wpnEnum,numWpns);
		for(int i = 0; i < numWpns; ++i)
		{
			if(_classId == (m_WeaponClassIdStart+wpnEnum[i].m_Value))
				return wpnEnum[i].m_Key;
		}
	}
	return 0;
}

int IGame::FindWeaponId(obint32 _classId)
{
	if(m_WeaponClassIdStart!=0)
	{
		// see if it's a weapon id
		int numWpns = 0;
		const IntEnum *wpnEnum = 0;
		GetWeaponEnumeration(wpnEnum,numWpns);

		int weaponId = _classId - m_WeaponClassIdStart;
		if(weaponId > 0 && weaponId < numWpns)
		{
			return weaponId;
		}
	}
	return 0;
}

void IGame::InitScriptClasses(gmMachine *_machine, gmTableObject *_table)
{
	// register base classes
	obint32 iNumMappings = sizeof(g_BaseClassMappings) / sizeof(g_BaseClassMappings[0]);
	for(int i = 0; i < iNumMappings; ++i)
	{
		_table->Set(_machine, g_BaseClassMappings[i].m_Key, gmVariable(g_BaseClassMappings[i].m_Value));
	}
}

void IGame::InitScriptWeaponClasses(gmMachine *_machine, gmTableObject *_table, int weaponClassId)
{
	m_WeaponClassIdStart = weaponClassId;

	int numWpns = 0;
	const IntEnum *wpnEnum = 0;
	GetWeaponEnumeration(wpnEnum,numWpns);
	for(int i = 0; i < numWpns; ++i)
	{
		OBASSERT(_table->Get(_machine,wpnEnum[i].m_Key).IsNull(),"Weapon class overwrite!");
		_table->Set(_machine, wpnEnum[i].m_Key, gmVariable(m_WeaponClassIdStart+wpnEnum[i].m_Value));
	}
}

void IGame::InitScriptSkills(gmMachine *_machine, gmTableObject *_table)
{
}

void IGame::InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "TEAM1",		gmVariable(ENT_FLAG_TEAM1));
	_table->Set(_machine, "TEAM2",		gmVariable(ENT_FLAG_TEAM2));
	_table->Set(_machine, "TEAM3",		gmVariable(ENT_FLAG_TEAM3));
	_table->Set(_machine, "TEAM4",		gmVariable(ENT_FLAG_TEAM4));
	_table->Set(_machine, "VISTEST",	gmVariable(ENT_FLAG_VISTEST));	
	_table->Set(_machine, "DISABLED",	gmVariable(ENT_FLAG_DISABLED));
	_table->Set(_machine, "PRONE",		gmVariable(ENT_FLAG_PRONED));
	_table->Set(_machine, "CROUCHED",	gmVariable(ENT_FLAG_CROUCHED));
	_table->Set(_machine, "CARRYABLE",	gmVariable(ENT_FLAG_CARRYABLE));
	_table->Set(_machine, "DEAD",		gmVariable(ENT_FLAG_DEAD));
	_table->Set(_machine, "INWATER",	gmVariable(ENT_FLAG_INWATER));
	_table->Set(_machine, "UNDERWATER",	gmVariable(ENT_FLAG_UNDERWATER));
	_table->Set(_machine, "ZOOMING",	gmVariable(ENT_FLAG_ZOOMING));
	_table->Set(_machine, "ON_LADDER",	gmVariable(ENT_FLAG_ONLADDER));	
	_table->Set(_machine, "ON_GROUND",	gmVariable(ENT_FLAG_ONGROUND));
	_table->Set(_machine, "RELOADING",	gmVariable(ENT_FLAG_RELOADING));
	_table->Set(_machine, "HUMANCONTROLLED",gmVariable(ENT_FLAG_HUMANCONTROLLED));	
	_table->Set(_machine, "IRONSIGHT",	gmVariable(ENT_FLAG_IRONSIGHT));
	_table->Set(_machine, "ON_ICE",		gmVariable(ENT_FLAG_ON_ICE));
	_table->Set(_machine, "IN_VEHICLE",	gmVariable(ENT_FLAG_INVEHICLE));
	_table->Set(_machine, "FROZEN",		gmVariable(ENT_FLAG_FROZEN));
	_table->Set(_machine, "TAUNTING",	gmVariable(ENT_FLAG_TAUNTING));
	_table->Set(_machine, "AIMING",		gmVariable(ENT_FLAG_AIMING));	
}

void IGame::InitScriptPowerups(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "INVINCIBLE",		gmVariable(PWR_INVINCIBLE));
}

void IGame::InitScriptBotButtons(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "ATTACK1",	gmVariable(BOT_BUTTON_ATTACK1));
	_table->Set(_machine, "ATTACK2",	gmVariable(BOT_BUTTON_ATTACK2));
	_table->Set(_machine, "JUMP",		gmVariable(BOT_BUTTON_JUMP));
	_table->Set(_machine, "CROUCH",		gmVariable(BOT_BUTTON_CROUCH));
	_table->Set(_machine, "PRONE",		gmVariable(BOT_BUTTON_PRONE));
	_table->Set(_machine, "WALK",		gmVariable(BOT_BUTTON_WALK));
	_table->Set(_machine, "USE",		gmVariable(BOT_BUTTON_USE));	
	_table->Set(_machine, "FORWARD",	gmVariable(BOT_BUTTON_FWD));
	_table->Set(_machine, "BACKWARD",	gmVariable(BOT_BUTTON_BACK));
	_table->Set(_machine, "STRAFE_R",	gmVariable(BOT_BUTTON_RSTRAFE));
	_table->Set(_machine, "STRAFE_L",	gmVariable(BOT_BUTTON_LSTRAFE));
	_table->Set(_machine, "RELOAD",		gmVariable(BOT_BUTTON_RELOAD));
	_table->Set(_machine, "SPRINT",		gmVariable(BOT_BUTTON_SPRINT));
	_table->Set(_machine, "DROP",		gmVariable(BOT_BUTTON_DROP));
	_table->Set(_machine, "LEAN_L",		gmVariable(BOT_BUTTON_LEANLEFT));
	_table->Set(_machine, "LEAN_R",		gmVariable(BOT_BUTTON_LEANRIGHT));
	_table->Set(_machine, "AIM",		gmVariable(BOT_BUTTON_AIM));
	_table->Set(_machine, "RESPAWN",	gmVariable(BOT_BUTTON_RESPAWN));
	_table->Set(_machine, "TAUNT",		gmVariable(BOT_BUTTON_TAUNT));	
	_table->Set(_machine, "THROW_KNIFE", gmVariable(BOT_BUTTON_THROWKNIFE));
}

void IGame::InitScriptTraceMasks(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "ALL",		gmVariable(TR_MASK_ALL));
	_table->Set(_machine, "SOLID",		gmVariable(TR_MASK_SOLID));
	_table->Set(_machine, "PLAYER",		gmVariable(TR_MASK_PLAYER));
	_table->Set(_machine, "SHOT",		gmVariable(TR_MASK_SHOT));
	_table->Set(_machine, "OPAQUE",		gmVariable(TR_MASK_OPAQUE));
	_table->Set(_machine, "WATER",		gmVariable(TR_MASK_WATER));
	_table->Set(_machine, "GRATE",		gmVariable(TR_MASK_GRATE));
	_table->Set(_machine, "PLAYERCLIP",	gmVariable(TR_MASK_PLAYERCLIP));
	_table->Set(_machine, "SMOKEBOMB",	gmVariable(TR_MASK_SMOKEBOMB));
	_table->Set(_machine, "FLOODFILL",	gmVariable(TR_MASK_FLOODFILL));
	_table->Set(_machine, "FLOODFILLENT",gmVariable(TR_MASK_FLOODFILLENT));
	_table->Set(_machine, "VISIBLE",gmVariable(TR_MASK_VISIBLE));
}

void IGame::InitScriptContentFlags(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "SOLID",	gmVariable(CONT_SOLID));
	_table->Set(_machine, "WATER",	gmVariable(CONT_WATER));
	_table->Set(_machine, "SLIME",	gmVariable(CONT_SLIME));
	_table->Set(_machine, "FOG",	gmVariable(CONT_FOG));
	_table->Set(_machine, "TELEPORTER",	gmVariable(CONT_TELEPORTER));
	_table->Set(_machine, "MOVER",	gmVariable(CONT_MOVER));
	_table->Set(_machine, "TRIGGER",gmVariable(CONT_TRIGGER));
	_table->Set(_machine, "LAVA",	gmVariable(CONT_LAVA));
	_table->Set(_machine, "PLAYERCLIP",	gmVariable(CONT_PLYRCLIP));
}

void IGame::InitScriptSurfaceFlags(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "SLICK", gmVariable(SURFACE_SLICK));
	_table->Set(_machine, "LADDER", gmVariable(SURFACE_LADDER));
}

void IGame::InitScriptBlackboardKeys(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "DELAY_GOAL",	gmVariable(bbk_DelayGoal));
	_table->Set(_machine, "IS_TAKEN",	gmVariable(bbk_IsTaken));
	_table->Set(_machine, "RUN_AWAY",	gmVariable(bbk_RunAway));	
}

void IGame::InitDebugFlags(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "LOG",		gmVariable(BOT_DEBUG_LOG));
	_table->Set(_machine, "MOVEMENT",	gmVariable(BOT_DEBUG_MOVEVEC));
	_table->Set(_machine, "SCRIPT",		gmVariable(BOT_DEBUG_SCRIPT));
	_table->Set(_machine, "FPINFO", gmVariable(BOT_DEBUG_FPINFO));
	_table->Set(_machine, "PLANNER", gmVariable(BOT_DEBUG_PLANNER));
	_table->Set(_machine, "EVENTS", gmVariable(BOT_DEBUG_EVENTS));
	_table->Set(_machine, "FAILED_PATHS", gmVariable(BOT_DEBUG_LOG_FAILED_PATHS));
}

void IGame::InitBoneIds(gmMachine *_machine, gmTableObject *_table)
{
	_table->Set(_machine, "TORSO",		gmVariable(BONE_TORSO));
	_table->Set(_machine, "PELVIS",		gmVariable(BONE_PELVIS));
	_table->Set(_machine, "HEAD",		gmVariable(BONE_HEAD));
	_table->Set(_machine, "RIGHTARM",	gmVariable(BONE_RIGHTARM));
	_table->Set(_machine, "LEFTARM",	gmVariable(BONE_LEFTARM));
	_table->Set(_machine, "RIGHTHAND",	gmVariable(BONE_RIGHTHAND));
	_table->Set(_machine, "LEFTHAND",	gmVariable(BONE_LEFTHAND));
	_table->Set(_machine, "RIGHTLEG",	gmVariable(BONE_RIGHTLEG));
	_table->Set(_machine, "LEFTLEG",	gmVariable(BONE_LEFTLEG));
	_table->Set(_machine, "RIGHTFOOT",	gmVariable(BONE_RIGHTFOOT));
	_table->Set(_machine, "LEFTFOOT",	gmVariable(BONE_LEFTFOOT));
}

void IGame::RegisterNavigationFlags(PathPlannerBase *_planner)
{
	_planner->RegisterNavFlag("TEAM1",		F_NAV_TEAM1);
	_planner->RegisterNavFlag("TEAM2",		F_NAV_TEAM2);
	_planner->RegisterNavFlag("TEAM3",		F_NAV_TEAM3);
	_planner->RegisterNavFlag("TEAM4",		F_NAV_TEAM4);
	_planner->RegisterNavFlag("TEAMONLY",	F_NAV_TEAMONLY);
	_planner->RegisterNavFlag("CLOSED",		F_NAV_CLOSED);
	_planner->RegisterNavFlag("CROUCH",		F_NAV_CROUCH);
	_planner->RegisterNavFlag("DOOR",		F_NAV_DOOR);
	_planner->RegisterNavFlag("JUMP",		F_NAV_JUMP);
	_planner->RegisterNavFlag("JUMPGAP",	F_NAV_JUMPGAP);
	_planner->RegisterNavFlag("JUMPLOW",	F_NAV_JUMPLOW);
	_planner->RegisterNavFlag("CLIMB",		F_NAV_CLIMB);
	_planner->RegisterNavFlag("SNEAK",		F_NAV_SNEAK);
	_planner->RegisterNavFlag("ELEVATOR",	F_NAV_ELEVATOR);
	_planner->RegisterNavFlag("TELEPORT",	F_NAV_TELEPORT);
	_planner->RegisterNavFlag("SNIPE",		F_NAV_SNIPE);
	_planner->RegisterNavFlag("HEALTH",		F_NAV_HEALTH);
	_planner->RegisterNavFlag("ARMOR",		F_NAV_ARMOR);
	_planner->RegisterNavFlag("AMMO",		F_NAV_AMMO);
	//_planner->RegisterNavFlag("BLOCKABLE",	F_NAV_BLOCKABLE);
	_planner->RegisterNavFlag("DYNAMIC",	F_NAV_DYNAMIC);	
	_planner->RegisterNavFlag("INWATER",	F_NAV_INWATER);
	_planner->RegisterNavFlag("UNDERWATER",	F_NAV_UNDERWATER);
	//_planner->RegisterNavFlag("MOVABLE",	F_NAV_MOVABLE);	
	_planner->RegisterNavFlag("DEFEND",		F_NAV_DEFEND);	
	_planner->RegisterNavFlag("ATTACK",		F_NAV_ATTACK);	
	_planner->RegisterNavFlag("SCRIPT",		F_NAV_SCRIPT);
	_planner->RegisterNavFlag("ROUTE",		F_NAV_ROUTEPT);	
	_planner->RegisterNavFlag("INFILTRATOR", F_NAV_INFILTRATOR);
}

void IGame::UpdateGame()
{
	//Prof(GameUpdate);

	CheckGameState();

	// Check if we're dead or alive so we know what update function to call.
	if(m_StateRoot)
	{
		m_StateRoot->RootUpdate();
	}

	//obstacleManager.Update();

	// This is called often to update the state of the bots and perform their "thinking"
	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
	{
		if(m_ClientList[i])
		{
			m_ClientList[i]->Update();
		}
	}

	PropogateDeletedThreads();

	g_Blackboard.PurgeExpiredRecords();

	// Increment the game frame.
	++m_GameFrame;
}

#ifdef ENABLE_REMOTE_DEBUGGING
void IGame::UpdateSync( RemoteSnapShots & snapShots, RemoteLib::DataBuffer & db ) {
	// draw the entities registered with the system
	IGame::EntityIterator ent;
	while( IGame::IterateEntity( ent ) ) {
		SyncEntity( ent.GetEnt(), snapShots.entitySnapShots[ ent.GetIndex() ], db );
	}
}
void IGame::SyncEntity( const EntityInstance & ent, EntitySnapShot & snapShot, RemoteLib::DataBuffer & db ) {
	RemoteLib::DataBufferStatic<2048> localBuffer;
	localBuffer.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );

	EntitySnapShot newSnapShot = snapShot;

	//////////////////////////////////////////////////////////////////////////
	// check for values that have changed
	{
		Vector3f entPosition = Vector3f::ZERO,
				facingVector = Vector3f::ZERO,
				rightVector = Vector3f::ZERO,
				upVector = Vector3f::ZERO;

		Msg_HealthArmor hlth;
		EngineFuncs::EntityPosition( ent.m_Entity, entPosition );
		const int entClass = InterfaceFuncs::GetEntityClass( ent.m_Entity );
		const String entName = EngineFuncs::EntityName( ent.m_Entity );
		//Box3f worldBounds;
		AABB localBounds;
		EngineFuncs::EntityOrientation( ent.m_Entity, facingVector, rightVector, upVector );

		newSnapShot.Sync( "name", entName.c_str(), localBuffer );
		newSnapShot.Sync( "x", entPosition.x, localBuffer );
		newSnapShot.Sync( "y", entPosition.y, localBuffer );
		newSnapShot.Sync( "z", entPosition.z, localBuffer );
		newSnapShot.Sync( "classid", entClass, localBuffer );

		const float heading = Mathf::RadToDeg( facingVector.XYHeading() );
		const float pitch = Mathf::RadToDeg( facingVector.GetPitch() );

		newSnapShot.Sync( "yaw", -Mathf::RadToDeg( heading ), localBuffer );
		newSnapShot.Sync( "pitch", Mathf::RadToDeg( pitch ), localBuffer );
		newSnapShot.Sync( "teamid", InterfaceFuncs::GetEntityTeam( ent.m_Entity ), localBuffer );
		if ( InterfaceFuncs::GetHealthAndArmor( ent.m_Entity, hlth ) ) {
			newSnapShot.Sync( "health", hlth.m_CurrentHealth, localBuffer );
			newSnapShot.Sync( "maxhealth", hlth.m_MaxHealth, localBuffer );
			newSnapShot.Sync( "armor", hlth.m_CurrentArmor, localBuffer );
			newSnapShot.Sync( "maxarmor", hlth.m_MaxArmor, localBuffer );
		}

		Box3f worldBounds;
		if ( EngineFuncs::EntityWorldOBB( ent.m_Entity, worldBounds ) ) {
			newSnapShot.Sync( "entSizeX", worldBounds.Extent[0], localBuffer );
			newSnapShot.Sync( "entSizeY", worldBounds.Extent[1], localBuffer );
			newSnapShot.Sync( "entSizeZ", worldBounds.Extent[2], localBuffer );
		}

		ClientPtr bot = GetClientByIndex( ent.m_Entity.GetIndex() );
		if ( bot ) {
			bot->InternalSyncEntity( newSnapShot, localBuffer );
		}
	}
	
	const uint32 writeErrors = localBuffer.endWrite();
	assert( writeErrors == 0 );

	if ( localBuffer.getBytesWritten() > 0 && writeErrors == 0 ) {
		db.beginWrite( RemoteLib::DataBuffer::WriteModeAllOrNone );
		db.startSizeHeader();
		db.writeInt32( RemoteLib::ID_qmlComponent );
		db.writeInt32( ent.m_Entity.AsInt() );
		db.writeSmallString( "entity" );
		db.append( localBuffer );
		db.endSizeHeader();

		if ( db.endWrite() == 0 ) {
			// mark the stuff we synced as done so we don't keep spamming it
			snapShot = newSnapShot;
		}
	}
}
void IGame::InternalSyncEntity( const EntityInstance & ent, EntitySnapShot & snapShot, RemoteLib::DataBuffer & db ) {
	// for derived classes
}
#endif

void IGame::ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb)
{
	switch(_message.GetMessageId())
	{
	case GAME_STARTGAME:
		{
			StartGame();
			m_LastGameState = m_GameState = GAME_STATE_WAITINGFORPLAYERS;
			break;
		}
	case GAME_ENDGAME:
		{
			EndGame();
			break;
		}
	case GAME_CLEARGOALS:
		{
			break;
		}
	case GAME_CLIENTCONNECTED:
		{
			const Event_SystemClientConnected *m = _message.Get<Event_SystemClientConnected>();
			ClientJoined(m);
			m_PlayersChanged = true;
			break;
		}
	case GAME_CLIENTDISCONNECTED:
		{
			const Event_SystemClientDisConnected *m = _message.Get<Event_SystemClientDisConnected>();
			ClientLeft(m);
			m_PlayersChanged = true;
			break;
		}
	case GAME_ENTITYCREATED:
		{
			const Event_EntityCreated *m = _message.Get<Event_EntityCreated>();
			if(m)
			{
				const int index = m->m_Entity.GetIndex();

				// special case don't replace existing entities with goal entities
				if(m_GameEntities[index].m_Entity.IsValid() && 
					m->m_EntityClass==ENT_CLASS_GENERIC_GOAL)
				{
					break;
				}

				//if(m->m_EntityCategory.CheckFlag(ENT_CAT_OBSTACLE))
				//{
				//	obstacleManager.AddObstacle( m->m_Entity );
				//}

				m_GameEntities[index].m_Entity = m->m_Entity;
				m_GameEntities[index].m_EntityClass = m->m_EntityClass;
				m_GameEntities[index].m_EntityCategory = m->m_EntityCategory;
				m_GameEntities[index].m_TimeStamp = IGame::GetTime();

				NavigationManager::GetInstance()->GetCurrentPathPlanner()->EntityCreated(m_GameEntities[index]);

#ifdef _DEBUG
				const char *pClassName = FindClassName(m->m_EntityClass);
				Utils::OutputDebug(kNormal, "Entity: %d created: %s", index, pClassName?pClassName:"<unknown>");
#endif

				// increase the upper limit if necessary.
				if(m_MaxEntity <= index)
					m_MaxEntity = index+1;
			}
			break;
		}
	case GAME_ENTITYDELETED:
		{
			const Event_EntityDeleted *m = _message.Get<Event_EntityDeleted>();
			if(m)
			{
				//obstacleManager.RemoveObstacle(m->m_Entity);

				int index = m->m_Entity.GetIndex();
				if(m_GameEntities[index].m_Entity.IsValid())
				{
#ifdef _DEBUG
					const char *pClassName = FindClassName(m_GameEntities[index].m_EntityClass);
					Utils::OutputDebug(kNormal, "Entity: %d deleted: %s", index, pClassName?pClassName:"<unknown>");
#endif

					m_GameEntities[index].m_Entity.Reset();
					m_GameEntities[index].m_EntityClass = 0;
					m_GameEntities[index].m_EntityCategory.ClearAll();
					m_GameEntities[index].m_TimeStamp = 0;

					// decrease the upper limit if necessary.
					if(m_MaxEntity == index+1)
					{
						do {
							m_MaxEntity--;
						} while(m_MaxEntity>0 && !m_GameEntities[m_MaxEntity-1].m_Entity.IsValid());
					}
				}

				GoalManager::GetInstance()->RemoveGoalByEntity(m->m_Entity);

				//////////////////////////////////////////////////////////////////////////
				PathPlannerBase *pPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
				if(pPlanner)
				{
					pPlanner->RemoveEntityConnection(m->m_Entity);
				}
			}
			break;
		}
	case GAME_NEWROUND:
		{
			NewRound();
			break;
		}
	case GAME_START_TRAINING:
		{
			StartTraining();
			break;
		}
	case GAME_GRAVITY:
		{
			const Event_SystemGravity *m = _message.Get<Event_SystemGravity>();
			m_Gravity = m->m_Gravity;
			break;
		}
	case GAME_CHEATS:
		{
			const Event_SystemCheats *m = _message.Get<Event_SystemCheats>();
			m_CheatsEnabled = m->m_Enabled==True;
			break;
		}
	case GAME_SCRIPTSIGNAL:
		{
			const Event_ScriptSignal *m = _message.Get<Event_ScriptSignal>();
			gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
			
			if(m->m_SignalName[0])
				pMachine->Signal(gmVariable(pMachine->AllocStringObject(m->m_SignalName)),GM_INVALID_THREAD,GM_INVALID_THREAD);
		}
	case GAME_SOUND:
		{
			/*const Event_Sound *m = _message.Get<Event_Sound>();
			if(m)
				g_SoundDepot.Post(*m);*/
			break;
		}
	case GAME_ADD_ENTITY_CONNECTION:
		{
			const Event_EntityConnection *m = _message.Get<Event_EntityConnection>();
			if(m)
			{
				PathPlannerBase *pPlanner = NavigationManager::GetInstance()->GetCurrentPathPlanner();
				if(pPlanner)
				{
					pPlanner->AddEntityConnection(*m);
				}
			}
			break;
		}
	}
}

void IGame::DispatchGlobalEvent(const MessageHelper &_message)
{
	// some messages we don't want to forward to clients
	switch(_message.GetMessageId())
	{
	case SYSTEM_SCRIPT_CHANGED:
		{
			const Event_SystemScriptUpdated *m = _message.Get<Event_SystemScriptUpdated>();
			g_WeaponDatabase.ReloadScript((LiveUpdateKey)m->m_ScriptKey);
			return;
		}
	case GAME_ENTITYCREATED:
	case GAME_ENTITYDELETED:
		break;
	default:
		{
			// Send this event to everyone.
			for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
			{
				if(m_ClientList[i])
					m_ClientList[i]->SendEvent(_message);
			}
			break;
		}
	}

	// Is it a game event?
	if(_message.GetMessageId() < GAME_ID_LAST && _message.GetMessageId() > GAME_ID_FIRST)
	{
		SendEvent(_message);
	}
}

void IGame::DispatchEvent(int _dest, const MessageHelper &_message)
{
	if(_dest >= 0 && _dest < Constants::MAX_PLAYERS)
	{
		// Send to someone in particular
		ClientPtr cp = GetClientFromCorrectedGameId(_dest);
		if(cp)
		{
			cp->SendEvent(_message);	
			return;
		}
		else
		{
			//OBASSERT(0, "Invalid Event Target");
		}
	}
	Utils::OutputDebug(kError, "BAD DESTINATION ID: %d FOR EVENT %d", _dest, _message.GetMessageId());
}

void IGame::ClientJoined(const Event_SystemClientConnected *_msg)
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

			g_EngineFuncs->ChangeTeam(_msg->m_GameId, cp->m_DesiredTeam, NULL);
			g_EngineFuncs->ChangeClass(_msg->m_GameId, cp->m_DesiredClass, NULL);

			cp->CheckTeamEvent();
			cp->CheckClassEvent();
		}
	}	
}

void IGame::ClientLeft(const Event_SystemClientDisConnected *_msg)
{
	Utils::OutputDebug(kInfo, "Client Left Game, ClientNum: %d", _msg->m_GameId);
	OBASSERT(_msg->m_GameId < Constants::MAX_PLAYERS && _msg->m_GameId >= 0, "Invalid Client Index!");

	ClientPtr &cp = GetClientFromCorrectedGameId(_msg->m_GameId);
	if(cp)
	{
		cp->Shutdown();
		cp.reset();
	}
}

void IGame::CheckGameState()
{
	GameState gs = InterfaceFuncs::GetGameState();
	switch(gs)
	{
	case GAME_STATE_WAITINGFORPLAYERS:
	case GAME_STATE_WARMUP:
	case GAME_STATE_WARMUP_COUNTDOWN:
	case GAME_STATE_PLAYING:
	case GAME_STATE_SUDDENDEATH:
		StartGame();
		break;		
	case GAME_STATE_INVALID:
		EndGame();
		break;
	case GAME_STATE_INTERMISSION:
	case GAME_STATE_SCOREBOARD:
	case GAME_STATE_PAUSED:
		return;
	}
	m_LastGameState = m_GameState;
	m_GameState = gs;
}

void IGame::StartGame()
{
	if(GameStarted())
		return;

	//EngineFuncs::ConsoleMessage("<StartGame>");
	InitMapScript();
}

void IGame::EndGame()
{
	if(!GameStarted())
		return;

	EngineFuncs::ConsoleMessage("<EndGame>");
}

void IGame::NewRound() 
{
	EngineFuncs::ConsoleMessage("<NewRound>");
}

void IGame::StartTraining()
{
	const char *pMapName = g_EngineFuncs->GetMapName();
	if(pMapName)
	{
		filePath script("scripts/%s_train.gm",pMapName);

		int iThreadId;
		if(!ScriptManager::GetInstance()->ExecuteFile(script, iThreadId))
		{
			EngineFuncs::ConsoleError(va("Error Running Training Script: %s", script.c_str()));
		}
	}
}

void IGame::InitMapScript()
{
	// Get Goals from the game.
	GoalManager::GetInstance()->Reset();

	ErrorObj err;
	const bool goalsLoaded = GoalManager::GetInstance()->Load(String(g_EngineFuncs->GetMapName()),err);
	err.PrintToConsole();

	if(!goalsLoaded)
	{
		// register nav system goals
		IGameManager::GetInstance()->GetNavSystem()->RegisterGameGoals();
	}

	GoalManager::GetInstance()->InitGameGoals();

	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
	DisableGCInScope gcEn(pMachine);

	filePath script( "nav/%s.gm", g_EngineFuncs->GetMapName() );
	GetMapScriptFile(script);

	// Load the script for the current map.
	int iThreadId = GM_INVALID_THREAD;
	if(ScriptManager::GetInstance()->ExecuteFile( script, iThreadId ))
	{
		{
			gmCall call;
			if(call.BeginGlobalFunction(pMachine, "OnMapLoad", gmVariable::s_null, true))
			{
				call.End();
			}
		}

		{
			gmCall call;
			if(call.BeginGlobalFunction(pMachine, "InitializeRoutes", gmVariable::s_null, true))
			{
				call.End();
			}
		}
	}

	// cs: moved this out so it does not depend on a map script. some autoexec scripts define it.
	{
		gmCall call;
		if(call.BeginGlobalFunction(pMachine, "PostMapLoad", gmVariable::s_null, true))
		{
			call.End();
		}
	}

	// Other initialization.
	m_SettingLimiter.reset(new Regulator(2000));
}

void IGame::UpdateTime()
{
	int iCurrentTime = g_EngineFuncs->GetGameTime();
	m_DeltaMsec = iCurrentTime - m_GameMsec;
	m_GameMsec = iCurrentTime;

	if(m_GameFrame > 0)
	{
		//initialize player info
		if(m_SettingLimiter && m_SettingLimiter->IsReady()){
			m_PlayersChanged = false;
			m_SettingLimiter->SetMsInterval(2000);
			CheckServerSettings(true); //execute ManagePlayers
		}
		else if(m_PlayersChanged){
			m_PlayersChanged = false;
			if(m_SettingLimiter) m_SettingLimiter->SetMsInterval(500);
			CheckServerSettings(false); //update the Server table
		}
	}
}

void IGame::CheckServerSettings(bool managePlayers)
{
	Prof(CheckServerSettings);

	obPlayerInfo pi;
	g_EngineFuncs->GetPlayerInfo(pi);

	//////////////////////////////////////////////////////////////////////////
	gmMachine *pM = ScriptManager::GetInstance()->GetMachine();
	DisableGCInScope gcEn(pM);

	gmVariable var = pM->GetGlobals()->Get(pM,"Server");
	gmTableObject *pServerTbl = var.GetTableObjectSafe();
	if(!pServerTbl)
	{
		pServerTbl = pM->AllocTableObject();
		pM->GetGlobals()->Set(pM,"Server",gmVariable(pServerTbl));
	}	
	gmTableObject *pTeamsTbl = pServerTbl->Get(pM,"Team").GetTableObjectSafe();
	if(!pTeamsTbl)
	{
		pTeamsTbl = pM->AllocTableObject();
		pServerTbl->Set(pM,"Team",gmVariable(pTeamsTbl));
	}

	pServerTbl->Set(pM,"NumPlayers",gmVariable(pi.GetNumPlayers()));
	pServerTbl->Set(pM,"NumPlayersNoSpec",gmVariable(pi.GetNumPlayers() - pi.GetNumPlayers(OB_TEAM_SPECTATOR)));
	pServerTbl->Set(pM,"NumBots",gmVariable(pi.GetNumPlayers(OB_TEAM_ALL,obPlayerInfo::Bot)));
	pServerTbl->Set(pM,"MaxPlayers",gmVariable(pi.GetMaxPlayers()));	
	pServerTbl->Set(pM,"AvailableTeams",gmVariable(pi.GetAvailableTeams()));

	for(int i = 0; i < obPlayerInfo::MaxTeams; ++i)
	{
		if(pi.GetAvailableTeams() & (1<<i))
		{
			gmTableObject *pTeam = pTeamsTbl->Get(i).GetTableObjectSafe();
			if(!pTeam)
			{
				pTeam = pM->AllocTableObject();
				pTeamsTbl->Set(pM, i, gmVariable(pTeam));
			}

			pTeam->Set(pM, "NumPlayers", gmVariable(pi.GetNumPlayers(i,obPlayerInfo::Both)));
			pTeam->Set(pM, "NumBots", gmVariable(pi.GetNumPlayers(i,obPlayerInfo::Bot)));
			pTeam->Set(pM, "NumHumans", gmVariable(pi.GetNumPlayers(i,obPlayerInfo::Human)));
		}
	}

	if(managePlayers && NavigationManager::GetInstance()->GetCurrentPathPlanner()->IsReady())
	{
		gmCall call;
		if(call.BeginGlobalFunction(pM,"ManagePlayers"))
		{
			call.End();
		}
	}
}

void IGame::InitCommands()
{
	SetEx("addbot", "Adds a bot to the game", 
		this, &IGame::cmdAddbot);
	SetEx("kickbot", "Removes a bot from the game", 
		this, &IGame::cmdKickbot);	
	SetEx("kickall", "Kick all bots from the game", 
		this, &IGame::cmdKickAll);
	SetEx("debugbot", "Enables debugging output on a specific bot", 
		this, &IGame::cmdDebugBot);
	SetEx("drawblocktests", "Enables drawing of blockable line tests", 
		this, &IGame::cmdDrawBlockableTests);
	SetEx("dontshoot", "Enables/disables all bot shooting ability.", 
		this, &IGame::cmdBotDontShoot);
	SetEx("show_bb", "Shows the contents of the global blackboard.", 
		this, &IGame::cmdDumpBlackboard);
	SetEx("reload_weapons", "Reloads the weapon database from script files on disc.", 
		this, &IGame::cmdReloadWeaponDatabase);
	SetEx("show_bb", "Shows the contents of the global blackboard.", 
		this, &IGame::cmdDumpBlackboard);
	SetEx("print_filesystem", "Prints files from file system.", 
		this, &IGame::cmdPrintFileSystem);
}

bool IGame::UnhandledCommand(const StringVector &_args)
{
	Prof(UnhandledCommand);
	bool handled = false;
	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
	{
		if(m_ClientList[i])
		{
			handled |= m_ClientList[i]->DistributeUnhandledCommand(_args);
		}
	}
	return handled; 
}

void IGame::cmdBotDontShoot(const StringVector &_args)
{
	if(_args.size() == 2)
	{
		obBool bDontShoot = Invalid;
		if(Utils::StringToFalse(_args[1]))
			bDontShoot = False;
		else if(Utils::StringToTrue(_args[1]))
			bDontShoot = True;

		if(bDontShoot != Invalid)
		{
			for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
			{
				if(m_ClientList[i])
				{
					m_ClientList[i]->SetUserFlag(Client::FL_SHOOTINGDISABLED, bDontShoot==True);
					
					EngineFuncs::ConsoleMessage(va("%s: shooting %s", 
						m_ClientList[i]->GetName(), 
						bDontShoot?"disabled":"enabled"));
				}
			}
		}
		return;
	}
	
	EngineFuncs::ConsoleError("Usage: bot dontshoot true/false/1/0/yes/no");
}

void IGame::cmdDumpBlackboard(const StringVector &_args)
{
	int iType = bbk_All;
	if(_args.size() > 1)
	{
		if(!Utils::ConvertString(_args[1], iType))
			return;
	}
	g_Blackboard.DumpBlackBoardContentsToGame(iType);
}

void IGame::cmdDebugBot(const StringVector &_args)
{
	if(_args.size() < 3)
	{
		EngineFuncs::ConsoleError("debugbot syntax: bot debugbot botname debugtype");
		EngineFuncs::ConsoleError("types: log, move, script, fpinfo, planner, events, failedpaths, aim, sensory, steer, target");
		return;
	}

	// What bot is this for?
	bool bAll = false;
	std::string botname = _args[1];
	if(botname == "all")
		bAll = true;
	bool bnotFound = false;

	for(int p = 0; p < Constants::MAX_PLAYERS; ++p)
	{
		if(m_ClientList[p])
		{
			ClientPtr bot = m_ClientList[p];
			if(botname == bot->GetName() || bAll)
			{
				for(obuint32 i = 2; i < _args.size(); ++i)
				{
					using namespace AiState;

					std::string strDebugType = _args[i];
					int flag = -1;
					if(strDebugType == "log") flag = BOT_DEBUG_LOG;
					else if(strDebugType == "move") flag = BOT_DEBUG_MOVEVEC;
					else if(strDebugType == "script") flag = BOT_DEBUG_SCRIPT;
					else if(strDebugType == "fpinfo") flag = BOT_DEBUG_FPINFO;
					else if(strDebugType == "planner") flag = BOT_DEBUG_PLANNER;
					else if(strDebugType == "events") flag = BOT_DEBUG_EVENTS;
					else if(strDebugType == "failedpaths") flag = BOT_DEBUG_LOG_FAILED_PATHS;
					else if(strDebugType == "aim") strDebugType = "Aimer";
					else if(strDebugType == "sensory") strDebugType = "SensoryMemory";
					else if(strDebugType == "steer") strDebugType = "SteeringSystem";
					else if(strDebugType == "target") strDebugType = "TargetingSystem";
					if(flag >= 0) {
						bot->EnableDebug(flag, !bot->IsDebugEnabled(flag));
					}
					else if(bot->GetStateRoot())
					{
						State *pState = bot->GetStateRoot()->FindState(strDebugType.c_str());
						if(pState)
						{
							pState->ToggleDebugDraw();
						}
						else if(!bnotFound)
						{
							bnotFound = true;
							EngineFuncs::ConsoleError(va("state or script goal %s not found", strDebugType.c_str()));
						}
					}
				}
				if(!bAll)
					return;
			}
		}
	}

	if(!bAll)
	{
		EngineFuncs::ConsoleError(va("no bot found named %s", botname.c_str()));
	}
}

void IGame::cmdKickAll(const StringVector &_args)
{
	// Kick all bots from the game.
	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
	{
		if(m_ClientList[i])
		{
			StringVector tl;
			tl.push_back("kickbot");
			tl.push_back((String)va("%i", m_ClientList[i]->GetGameID()));
			CommandReciever::DispatchCommand(tl);
		}
	}
}

void IGame::cmdAddbot(const StringVector &_args)
{
	int iTeam = -1; // -1 for autochoose in interface
	int iClass = -1; // -1 for autochoose in interface

	String profile;
	String nextName;

	// Parse the command arguments
	switch(_args.size())
	{
	case 5:
		profile = _args[4];
	case 4:
		nextName = _args[3];
	case 3:
		iClass = atoi(_args[2].c_str());
	case 2:
		iTeam = atoi(_args[1].c_str());
	case 1:
		break;
	default:
		EngineFuncs::ConsoleError("addbot team[1-2] class[1-5] name[string]");
		return;
	};

	if(nextName.empty())
	{
		NamePtr nr = NameManager::GetInstance()->GetName();
		nextName = nr ? nr->GetName() : Utils::FindOpenPlayerName();
	}

	Msg_Addbot b;
	Utils::StringCopy(b.m_Name, nextName.c_str(), sizeof(b.m_Name));
	Utils::StringCopy(b.m_Profile, profile.c_str(), sizeof(b.m_Profile));
	b.m_Team = iTeam;
	b.m_Class = iClass;
	AddBot(b, true);
}

void IGame::cmdKickbot(const StringVector &_args)
{
	// Parse the command arguments
	bool bDidSomething = false;
	for(int i = 1; i < (int)_args.size(); ++i)
	{
		Msg_Kickbot b;
		if(!Utils::ConvertString(_args[i],b.m_GameId))
			Utils::StringCopy(b.m_Name, _args[i].c_str(), Msg_Kickbot::BufferSize);
		InterfaceFuncs::Kickbot(b);
		bDidSomething = true;
	}
	
	if(!bDidSomething)
	{
		EngineFuncs::ConsoleError("kickbot [string/gameid] ...");
	}
}

void IGame::cmdDrawBlockableTests(const StringVector &_args)
{
	if(_args.size() >= 2)
	{
		if(!m_bDrawBlockableTests && Utils::StringToTrue(_args[1]))
		{
			EngineFuncs::ConsoleMessage("Draw Blockable Tests on.");
			m_bDrawBlockableTests = true;
		}
		else if(m_bDrawBlockableTests && Utils::StringToFalse(_args[1]))
		{
			EngineFuncs::ConsoleMessage("Draw Blockable Tests off.");
			m_bDrawBlockableTests = false;
		}
		else
		{
			m_bDrawBlockableTests = !m_bDrawBlockableTests;
		}
	}
}

void IGame::cmdPrintFileSystem(const StringVector &_args)
{
	String pth = "";
	String ex = ".*";

	DirectoryList dlist;
	FileSystem::FindAllFiles(pth, dlist, ex);

	EngineFuncs::ConsoleMessage("------------------------------------");
	EngineFuncs::ConsoleMessage(va("%d Files %s, in %s", dlist.size(), ex.c_str(), pth.c_str()));
	for(obuint32 i = 0; i < dlist.size(); ++i)
	{
		EngineFuncs::ConsoleMessage(dlist[i].string().c_str());		
	}
	EngineFuncs::ConsoleMessage("------------------------------------");
}

void IGame::cmdReloadWeaponDatabase(const StringVector &_args)
{
	g_WeaponDatabase.LoadWeaponDefinitions(true);
	DispatchGlobalEvent(MessageHelper(MESSAGE_REFRESHALLWEAPONS));
}

void IGame::AddBot(Msg_Addbot &_addbot, bool _createnow)
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
		g_EngineFuncs->ChangeTeam(iGameID, cp->m_DesiredTeam, NULL);
		g_EngineFuncs->ChangeClass(iGameID, cp->m_DesiredClass, NULL);

		cp->CheckTeamEvent();
		cp->CheckClassEvent();
	}
}

ClientPtr IGame::GetClientByGameId(int _gameId)
{
	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
		if(m_ClientList[i] && m_ClientList[i]->GetGameID() == _gameId)
			return ClientPtr(m_ClientList[i]);
	return ClientPtr();
}

ClientPtr IGame::GetClientByIndex(int _index)
{
	if(InRangeT<int>(_index, 0, Constants::MAX_PLAYERS))
		return m_ClientList[_index];
	return ClientPtr();
}

ClientPtr &IGame::GetClientFromCorrectedGameId(int _gameid) 
{
	return m_ClientList[_gameid]; 
}

bool IGame::CreateCriteria(gmThread *_thread, CheckCriteria &_criteria, StringStr &err)
{
	_criteria = CheckCriteria();

	const int MaxArgs = 8;
	enum CriteriaArgType
	{
		kTypeNone,
		kTypeInt,
		kTypeFloat,
		kTypeNum,
		kTypeVector,
		kTypeEntity,
		kTypeMapGoal,

		kNumArgTypes
	};

	const char * ArgTypeNames[] =
	{
		"",
		"int",
		"float",
		"float or int",
		"vec3",
		"entity",
		"mapgoal",
	};
	BOOST_STATIC_ASSERT(sizeof(ArgTypeNames)/sizeof(ArgTypeNames[0]) == kNumArgTypes);

	//////////////////////////////////////////////////////////////////////////
	// Subject
	int NextArg = 0;
	if(_thread->ParamType(NextArg)==GM_ENTITY)
	{
		GameEntity e;
		e.FromInt(_thread->Param(NextArg).GetEntity());
		_criteria.m_Subject.SetEntity(e);

		NextArg++;
	}
	//////////////////////////////////////////////////////////////////////////
	
	const char *pExpression = _thread->Param(NextArg).GetCStringSafe(NULL);	
	if(pExpression)
	{
		int NumExpectedOperands = 0;
		CriteriaArgType ExpectedArgTypes[MaxArgs] = {};

		int iToken = CheckCriteria::TOK_CRITERIA;

		StringVector sv;
		Utils::Tokenize(pExpression," ",sv);
		for(obuint32 t = 0; t < sv.size(); ++t)
		{
			const obuint32 sHash = Utils::Hash32(sv[t].c_str());

			if(iToken==CheckCriteria::TOK_CRITERIA)
			{
				switch(sHash)
				{
				case 0x29b19c8a /* not */:
				case 0x8fd958a1 /* doesnot */:
					_criteria.m_Negated = true;
					break;
				case 0x84d1546a /* deleted */:
					_criteria.m_Criteria = Criteria::ON_ENTITY_DELETED;
					_criteria.m_Operator = Criteria::OP_EQUALS;
					_criteria.m_Operand[0] = obUserData(1);
					NumExpectedOperands = 0;
					++iToken;
					break;
				case 0x587bc012 /* hasentflag */:
				case 0xabcfd8b3 /* hasentflags */:
				case 0x8770b3da /* haveentflag */:
				case 0xdf6ad30b /* haveentflags */:
					_criteria.m_Criteria = Criteria::ON_ENTITY_FLAG;
					_criteria.m_Operator = Criteria::OP_EQUALS;
					ExpectedArgTypes[NumExpectedOperands++] = kTypeInt;
					++iToken;
					break;
				case 0x6b98ed8f /* health */:
					_criteria.m_Criteria = Criteria::ON_ENTITY_HEALTH;
					NumExpectedOperands = 1;
					ExpectedArgTypes[0] = kTypeNum;
					++iToken;
					break;
				case 0x7962a87c /* weaponequipped */:
					_criteria.m_Criteria = Criteria::ON_ENTITY_WEAPON;
					ExpectedArgTypes[NumExpectedOperands++] = kTypeInt; // weaponid
					++iToken;
					break;
				case 0x32741c32 /* velocity */:
					_criteria.m_Criteria = Criteria::ON_ENTITY_VELOCITY;
					ExpectedArgTypes[NumExpectedOperands++] = kTypeVector; // velocity vector
					++iToken;
					break;
				case 0x3f44af2b /* weaponcharged */:
					_criteria.m_Criteria = Criteria::ON_ENTITY_WEAPONCHARGED;
					ExpectedArgTypes[NumExpectedOperands++] = kTypeInt; // weaponid
					++iToken;
					break;
				case 0x79e3594d /* weaponhasammo */:
					_criteria.m_Criteria = Criteria::ON_ENTITY_WEAPONHASAMMO;
					ExpectedArgTypes[NumExpectedOperands++] = kTypeInt; // weaponid
					ExpectedArgTypes[NumExpectedOperands++] = kTypeInt; // ammo amount
					++iToken;
					break;
				case 0x8ad3b7b9 /* mapgoalavailable */:
					_criteria.m_Criteria = Criteria::ON_MAPGOAL_AVAILABLE;
					ExpectedArgTypes[NumExpectedOperands++] = kTypeMapGoal;
					++iToken;
					break;
				default:
					err << "invalid criteria " << sv[t].c_str() << std::endl;
					return false;
				}
			}
			else if(iToken==CheckCriteria::TOK_OPERATOR)
			{
				_criteria.ParseOperator(sHash);
				break;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Operand
		for(int i = 0; i < NumExpectedOperands; ++i)
		{
			gmVariable o = _thread->Param(2+i);

			OBASSERT(ExpectedArgTypes[i] != kTypeNone,"Invalid arg expectation");
			switch(ExpectedArgTypes[i])
			{
				case kTypeInt:
					{
						if(!o.IsInt())
							goto argError;

						_criteria.m_Operand[i] = obUserData(o.GetInt());
						break;
					}
				case kTypeFloat:
					{
						if(!o.IsFloat())
							goto argError;

						_criteria.m_Operand[i] = obUserData(o.GetFloat());
						break;
					}
				case kTypeNum:
					{
						if(!o.IsNumber())
							goto argError;

						_criteria.m_Operand[i] = obUserData(o.GetFloatSafe());
						break;
					}
				case kTypeVector:
					{
						if(!o.IsVector())
							goto argError;

						Vector3f v;
						o.GetVector(v);
						_criteria.m_Operand[i] = obUserData(v.x,v.y,v.z);
						break;
					}
				case kTypeEntity:
					{
						if(!o.IsEntity())
							goto argError;

						GameEntity e;
						e.FromInt(o.GetEntity());
						_criteria.m_Operand[i].SetEntity(e);
						break;
					}
				case kTypeMapGoal:
					{
						if(o.IsString())
						{
							const char * goalName = o.GetCStringSafe(0);
							MapGoalPtr mg = GoalManager::GetInstance()->GetGoal(goalName);
							if(mg)
							{
								_criteria.m_Operand[i] = obUserData(mg->GetSerialNum());
							}
							else
							{
								err << "unknown map goal '" << goalName << "'" << std::endl;
								return false;
							}
						}
						else
						{
							MapGoal *Mg = 0;
							if(gmBind2::Class<MapGoal>::FromVar(_thread,o,Mg) && Mg)
							{
								_criteria.m_Operand[i] = Mg->GetSerialNum();
								return true;
							}
							else
								goto argError;
						}
						break;
					}
				default:
					OBASSERT(false,"bad");
					break;
			}
			continue;
argError:
			const char * gotType = _thread->GetMachine()->GetTypeName(o.m_type);
			err << "expected " << ArgTypeNames[ExpectedArgTypes[i]] << "got " << gotType << std::endl;
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

void IGame::LoadGoalScripts(bool _clearold)
{
	LOG("Loading Script Goals");
	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

	gmTableObject *pScriptGoalTable = pMachine->GetGlobals()->Get(pMachine, "ScriptGoals").GetTableObjectSafe();
	if(_clearold || !pScriptGoalTable)
	{
		pScriptGoalTable = pMachine->AllocTableObject();
		pMachine->GetGlobals()->Set(pMachine, "ScriptGoals", gmVariable(pScriptGoalTable));
		g_ScriptGoalList.clear();
	}

	DirectoryList goalFiles;
	FileSystem::FindAllFiles("global_scripts/goals", goalFiles, "goal_.*\\.gm");
	FileSystem::FindAllFiles("scripts/goals", goalFiles, "goal_.*\\.gm");

	LOG("Loading " << goalFiles.size() << " goal scripts from: scripts/goals");
	DirectoryList::const_iterator cIt = goalFiles.begin(), cItEnd = goalFiles.end();
	for(; cIt != cItEnd; ++cIt)
	{
		ScriptGoalPtr ptr(new AiState::ScriptGoal(""));
		
		int iThreadId;
		gmUserObject *pUserObj = ptr->GetScriptObject(pMachine);
		gmVariable varThis(pUserObj);

		filePath script( (*cIt).string().c_str() );
		//LOG("Loading Goal Definition: " << script);
		if(ScriptManager::GetInstance()->ExecuteFile(script, iThreadId, &varThis) && ptr->GetName()[0])
		{
			g_ScriptGoalList.push_back(ptr);

			// add to global table.
			pScriptGoalTable->Set(pMachine, ptr->GetName().c_str(), gmVariable(pUserObj));
		}
		else
		{
			LOG("Error Running Goal Script: " << (*cIt).string());
			OBASSERT(0, "Error Running Goal Script: %s", (*cIt).string().c_str());
		}
	}
}

void IGame::ReloadGoalScripts()
{
	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
	{
		if(m_ClientList[i])
			m_ClientList[i]->GetStateRoot()->DeleteGoalScripts();
	}

	LoadGoalScripts(true);

	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
	{
		if(m_ClientList[i]){
			m_ClientList[i]->InitScriptGoals();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

int IGame::GetDebugWindowNumClients() const
{
	int num = 0;
	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
		if(m_ClientList[i])
			++num;
	return num;
}

ClientPtr IGame::GetDebugWindowClient(int index) const
{
	for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
	{
		if(m_ClientList[i])
		{
			if(index==0)
				return m_ClientList[i];
			--index;
		}
	}
	return ClientPtr();
}

//////////////////////////////////////////////////////////////////////////

IGame::EntityIterator::operator bool() const
{
	return m_Current.m_Entity.IsValid();
}

void IGame::EntityIterator::Clear()
{
	m_Current.m_Entity = GameEntity();
	m_Current.m_EntityClass = 0;
	m_Current.m_EntityCategory.ClearAll();
}

bool IGame::IsEntityValid(const GameEntity &_hnl)
{
	if(_hnl.IsValid())
	{
		const int index = _hnl.GetIndex();
		if(unsigned(index) < Constants::MAX_ENTITIES)
		{
			EntityInstance &ei = m_GameEntities[index];
			if(ei.m_Entity.IsValid())
			{
				UpdateEntity(ei);
				if(ei.m_EntityClass)
					return true;
			}
		}
	}
	return false;
}

bool IGame::IterateEntity(IGame::EntityIterator &_it)
{
	int iStartIndex = 0;
	if(_it)
		iStartIndex = _it.m_Current.m_Entity.GetIndex() + 1;

	for(int i = iStartIndex; i < m_MaxEntity; ++i)
	{
		if(m_GameEntities[i].m_Entity.IsValid())
		{
			UpdateEntity( m_GameEntities[i] );
			_it.m_Current = m_GameEntities[i];
			_it.m_Index = i;
			return true;
		}			
	}
	return false;
}

void IGame::UpdateEntity(EntityInstance &_ent)
{
	if ( _ent.m_EntityClass < FilterSensory::ANYPLAYERCLASS && _ent.m_TimeStamp < IGame::GetTime() ) {
		_ent.m_EntityClass = g_EngineFuncs->GetEntityClass( _ent.m_Entity );
		g_EngineFuncs->GetEntityCategory( _ent.m_Entity, _ent.m_EntityCategory );
		_ent.m_TimeStamp = IGame::GetTime();
	}
}

void IGame::AddDeletedThread(int threadId)
{
	// process the full buffer
	if(m_NumDeletedThreads == MaxDeletedThreads)
	{
		PropogateDeletedThreads();
	}

	// paranoid check
	if(m_NumDeletedThreads < MaxDeletedThreads)
	{
		m_DeletedThreads[m_NumDeletedThreads++] = threadId;
	}
	else
	{
		OBASSERT(m_NumDeletedThreads < MaxDeletedThreads,"out of slots");
	}
}

static bool _ThreadIdSort(int id1, int id2)
{
	if(id1==GM_INVALID_THREAD)
		return false;
	if(id2==GM_INVALID_THREAD)
		return true;
	return id1 < id2;
}

void IGame::PropogateDeletedThreads()
{
	Prof(PropogateDeletedThreads);
	if ( m_NumDeletedThreads > 0 )
	{
		std::sort(m_DeletedThreads,m_DeletedThreads+m_NumDeletedThreads,_ThreadIdSort);
		for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
		{
			if(m_ClientList[i])
			{
				m_ClientList[i]->PropogateDeletedThreads(m_DeletedThreads,m_NumDeletedThreads);
			}
		}
		m_NumDeletedThreads = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
