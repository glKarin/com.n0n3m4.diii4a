#include "PrecompCommon.h"
#include "Client.h"
#include "CommandReciever.h"
#include "TriggerManager.h"
#include "NameManager.h"
#include "IGameManager.h"
#include "NavigationManager.h"
#include "ScriptManager.h"
#include "IGame.h"
#include "DebugWindow.h"
#include "MapGoalDatabase.h"

#include "WeaponDatabase.h"

#include "gmBotLibrary.h"
#include "gmBot.h"
#include "gmAABB.h"
#include "gmGameEntity.h"
#include "gmMatrix3.h"

// script: BotLibrary
//		Binds various useful functionality to the scripting system.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

// function: EnableDebugWindow
//		Enables/Disables the debug render window.
//
// Parameters:
//
//		int - true/false to enable/disable
//		int - OPTIONAL - width of the debug window(default 800)
//		int - OPTIONAL - height of the debug window(default 600)
//		int - OPTIONAL - bpp of the debug window(default 32 bit)
//
// Returns:
//		none
static int GM_CDECL gmfEnableDebugWindow(gmThread *a_thread)
{
#ifdef ENABLE_DEBUG_WINDOW
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(en, 0);
	GM_INT_PARAM(width, 1, 1024);
	GM_INT_PARAM(height, 2, 768);
	GM_INT_PARAM(bpp, 3, 32);

	if(en)
		DebugWindow::Create(width, height, bpp);
	else
		DebugWindow::Destroy();
	return GM_OK;
#else
	GM_EXCEPTION_MSG("DebugWindow Not Available.");
	return GM_EXCEPTION;
#endif	
}
static int GM_CDECL gmfDumpDebugConsoleToFile(gmThread *a_thread)
{
#ifdef ENABLE_DEBUG_WINDOW
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(filename, 0);

	DW.Console.DumpConsoleToFile(filename);
	return GM_OK;
#else
	GM_EXCEPTION_MSG("DebugWindow Not Available.");
	return GM_EXCEPTION;
#endif	
}

// function: EchoToScreen
//		This function will print a message to the screen of all players.
//
// Parameters:
//
//		float - duration of the message(in seconds)
//		string - The message to print
//
// Returns:
//		None
static int GM_CDECL gmfEchoMessageToScreen(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 0);
	GM_CHECK_STRING_PARAM(msg, 1);
	//g_EngineFuncs->PrintScreenText(NULL, duration, COLOR::WHITE, msg);
	Utils::PrintText(Vector3f::ZERO, COLOR::WHITE, IGame::GetDeltaTimeSecs()*duration, msg);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: EchoError
//		This function will print a message using the default error printing
//		method the game uses(Usually colored)
//
// Parameters:
//
//		string - The message to print.
//
// Returns:
//		None
static int GM_CDECL gmfEchoError(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(msg, 0);
	EngineFuncs::ConsoleError(msg);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Log
//		Write some text to the bots log file.
//
// Parameters:
//
//		string - The message to log.
//		int - OPTIONAL - 0=info, 1=warning, 2=error, 3=critical
//
// Returns:
//		None
static int GM_CDECL gmfLog(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(msg, 0);
	GM_INT_PARAM(level, 1, 0);

	switch(level){
		case 0:
			LOG(msg);
			break;
		case 1:
			LOGWARN(msg);
			break;
		case 2:
			LOGERR(msg);
			break;
		default:
			LOGCRIT(msg);
			break;
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: OnTrigger
//		This function will register a script function callback for certain triggers.
//
// Parameters:
//
//		string - The trigger name to use
//		function - The script function to be called when the trigger is recieved
//
// Returns:
//		None
static int gmfRegisterTriggerCallback(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(triggername, 0);
	GM_CHECK_FUNCTION_PARAM(callbackfunction, 1);
	if(triggername)
	{
		TriggerManager::GetInstance()->SetScriptCallback(triggername, 
			gmGCRoot<gmFunctionObject>(callbackfunction, a_thread->GetMachine()));
		LOG("Trigger Callback: " << callbackfunction->GetDebugName() << 
			" : For Function: " << triggername << " Set.");
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: RunScript
//		This function will attempt to execute a gm script file.
//
// Parameters:
//
//		string - The file name of the script to attempt to execute.
//
// Returns:
//		int - true if success, false if failure.
static int GM_CDECL gmfRunScript(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(scriptname, 0);
	
	try
	{
		if(scriptname)
		{
			gmVariable vThis = *a_thread->GetThis();

			int threadId = GM_INVALID_THREAD;
			if(ScriptManager::GetInstance()->ExecuteFile(scriptname,threadId,&vThis))
			{
				a_thread->PushInt(1);
				return GM_OK;
			}
		}
	}
	catch(const std::exception& e)
	{
		_UNUSED(e);
		OBASSERT(0, e.what());		
	}	
	a_thread->PushInt(0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: AddBot
//		This function will add a bot to the game, using optional supplied arguments.
//
// Parameters:
//
//		int - OPTIONAL - The team to add the bot to.
//		int - OPTIONAL - The class to add the bot to.
//		string - OPTIONAL - The name to give the bot.
//		string - OPTIONAL - The spawnpoint name to spawn bot at.
//		string - OPTIONAL - The model for the bot to use.
//		string - OPTIONAL - The skin for the bot to use.
//
// Returns:
//		None
static int GM_CDECL gmfAddBot(gmThread *a_thread)
{
	Msg_Addbot b;
	memset(&b,0,sizeof(b));

	if(a_thread->ParamType(0) == GM_TABLE)
	{
		gmTableObject *pTable = a_thread->ParamTable(0);
		gmVariable vName = pTable->Get(a_thread->GetMachine(), "name");
		if(vName.IsNull())
			vName = pTable->Get(a_thread->GetMachine(), "Name");
		gmVariable vTeam = pTable->Get(a_thread->GetMachine(), "team");
		if(vTeam.IsNull())
			vTeam = pTable->Get(a_thread->GetMachine(), "Team");
		gmVariable vClass = pTable->Get(a_thread->GetMachine(), "class");
		if(vClass.IsNull())
			vClass = pTable->Get(a_thread->GetMachine(), "Class");
		gmVariable vSpawnPoint = pTable->Get(a_thread->GetMachine(), "vSpawnPoint");
		if(vSpawnPoint.IsNull())
			vSpawnPoint = pTable->Get(a_thread->GetMachine(), "SpawnPoint");
		gmVariable vModel = pTable->Get(a_thread->GetMachine(), "model");
		if(vModel.IsNull())
			vModel = pTable->Get(a_thread->GetMachine(), "Model");
		gmVariable vSkin = pTable->Get(a_thread->GetMachine(), "skin");
		if(vSkin.IsNull())
			vSkin = pTable->Get(a_thread->GetMachine(), "Skin");

		b.m_Team = vTeam.IsInt() ? vTeam.GetInt() : RANDOM_TEAM_IF_NO_TEAM;
		b.m_Class = vClass.IsInt() ? vClass.GetInt() : RANDOM_CLASS_IF_NO_CLASS;
		Utils::StringCopy(b.m_Name, vName.GetCStringSafe()?vName.GetCStringSafe():"", sizeof(b.m_Name));
		Utils::StringCopy(b.m_Model, vModel.GetCStringSafe()?vModel.GetCStringSafe():"", sizeof(b.m_Model));
		Utils::StringCopy(b.m_Skin, vSkin.GetCStringSafe()?vSkin.GetCStringSafe():"", sizeof(b.m_Skin));
		Utils::StringCopy(b.m_SpawnPointName, vSpawnPoint.GetCStringSafe()?vSpawnPoint.GetCStringSafe():"", sizeof(b.m_SpawnPointName));
	}
	else
	{
		GM_INT_PARAM(iAddTeam, 0, RANDOM_TEAM);
		GM_INT_PARAM(iAddClass, 1, RANDOM_CLASS);
		GM_STRING_PARAM(pAddName, 2, "");
		GM_STRING_PARAM(spawnPoint, 3, "");
		GM_STRING_PARAM(model, 4, "");
		GM_STRING_PARAM(skin, 5, "");

		Utils::StringCopy(b.m_Name, pAddName, sizeof(b.m_Name));
		Utils::StringCopy(b.m_Model, model, sizeof(b.m_Model));
		Utils::StringCopy(b.m_Skin, skin, sizeof(b.m_Skin));
		Utils::StringCopy(b.m_SpawnPointName, spawnPoint, sizeof(b.m_SpawnPointName));
		b.m_Team = iAddTeam;
		b.m_Class = iAddClass;
	}
	IGameManager::GetInstance()->GetGame()->AddBot(b, true);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: KickBot
//		This function will kick a bot from the game.
//
// Parameters:
//
//		string or int - The name of the bot to kick or gameid
//
// Returns:
//		None
static int GM_CDECL gmfKickBot(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	String s;
	switch(a_thread->ParamType(0))
	{
	case GM_STRING:
		s = a_thread->Param(0).GetCStringSafe();
		break;
	case GM_INT:
		Utils::ConvertString(a_thread->Param(0).GetInt(),s);
		break;
	}

	StringVector tl;
	tl.push_back("kickbot");
	tl.push_back(s);
	CommandReciever::DispatchCommand(tl);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: KickBotFromTeam
//		This function will kick a bot from a team.
//
// Parameters:
//
//		int - team to kick a bot from
//
// Returns:
//		None
static int GM_CDECL gmfKickBotFromTeam(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(team,0);

	ClientPtr lastBotOnTeam;
	IGame *pGame = IGameManager::GetInstance()->GetGame();
	if(pGame)
	{
		for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
		{
			ClientPtr cp = pGame->GetClientByIndex(i);
			if(cp && cp->GetTeam() == team)
				lastBotOnTeam = cp;
		}
	}
	
	if(lastBotOnTeam)
	{
		String strGameId;
		Utils::ConvertString(lastBotOnTeam->GetGameID(), strGameId);

		StringVector tl;
		tl.push_back("kickbot");
		tl.push_back(strGameId);
		CommandReciever::DispatchCommand(tl);
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: MoveBotToAnotherTeam
//		This function will move a bot from a team to another team
//
// Parameters:
//
//		int - source team
//		int - destination team
//
// Returns:
//		None
static int GM_CDECL gmfMoveBotToAnotherTeam(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(sourceteam,0);
	GM_CHECK_INT_PARAM(destinationteam,1);

	ClientPtr lastBotOnTeam;
	IGame *pGame = IGameManager::GetInstance()->GetGame();
	if(pGame)
	{
		for(int i = 0; i < Constants::MAX_PLAYERS; ++i)
		{
			ClientPtr cp = pGame->GetClientByIndex(i);
			if(cp && cp->GetTeam() == sourceteam)
				lastBotOnTeam = cp;
		}
	}

	if(lastBotOnTeam)
	{
		lastBotOnTeam->ChangeTeam(destinationteam);
	}
	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////

// function: KickAll
//		This function will kick all bots from the game.
//
// Parameters:
//
//		None
//
// Returns:
//		None
static int GM_CDECL gmfKickAll(gmThread *a_thread)
{
	StringVector tl;
	tl.push_back("kickall");
	CommandReciever::DispatchCommand(tl);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ExecCommand
//		This function will execute a bot command as if it were a console command.
//
// Parameters:
//
//		string - The name of the bot to kick
//
// Returns:
//		None
static int GM_CDECL gmfExecCommand(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(cmd, 0);

	StringVector tokList;
	Utils::Tokenize(cmd, " ", tokList);
	CommandReciever::DispatchCommand(tokList);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: EntityKill
//		Kills a specified enemy. Requires cheats to be enabled
//
// Parameters:
//
//		<GameEntity> - The entity to kill
//		- OR - 
//		<int> - The gameId for the entity to kill
//
// Returns:
//		true if successful, false if not
static int GM_CDECL gmfEntityKill(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");
	a_thread->PushInt(InterfaceFuncs::EntityKill(gameEnt) ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: RegisterDefaultProfile
//		This function assign a default script to use for a specific classId.
//
// Parameters:
//
//		int - The class Id to set the default profile for. See the global CLASS table.
//		string - The script name to use for this classes default profile.
//
// Returns:
//		None
static int GM_CDECL gmfRegisterDefaultProfile(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(botclass, 0);
	GM_CHECK_STRING_PARAM(profilename, 1);
	if(profilename)
	{
		NameManager::GetInstance()->SetProfileForClass(botclass, profilename);
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetMapGoal
//		This function gets a reference to a map goal, by its name.
//
// Parameters:
//
//		string - The name of the map goal to get
//
// Returns:
//		<gm - mapgoal> - The map goal with the provided name.
//		- OR -
//		null - If no map goals matched the name.
static int GM_CDECL gmfGetMapGoal(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(name, 0);
	if(name)
	{
		MapGoalPtr pGoal = GoalManager::GetInstance()->GetGoal(name);
		if(pGoal)
		{			
			gmGCRoot<gmUserObject> pUser = pGoal->GetScriptObject(a_thread->GetMachine());
			OBASSERT(pUser, "Invalid Object");
			a_thread->PushUser(pUser);
		}
		else
		{
			a_thread->PushNull();
		}
	}	
	return GM_OK;
}

// function: GetMapGoals
//		This function gets a table of map goals, by its name.
//		Similar to <GetMapGoal>, though this can return more than 1
//		and supports regular expressions for the goal name.
//
// Parameters:
//
//		table	- The table to put the matching goals into.
//		team	- The team the goal should be for. 0 means any team.
//		exp		- The expression to use to match the goal names. "" to match any.
//		params
//
// Returns:
//		count of returned goals
static int GM_CDECL gmfGetMapGoals(gmThread *a_thread)
{
	return GetMapGoals(a_thread, 0);
}

int GM_CDECL GetMapGoals(gmThread *a_thread, Client *client)
{
	GM_CHECK_TABLE_PARAM(table, 0);
	table->RemoveAndDeleteAll(a_thread->GetMachine());

	GoalManager::Query qry;
	int p = 1;
	if(client) //QueryGoals
	{
		qry.Bot(client);
		qry.CheckRangeProperty(true);
	}
	else //GetGoals
	{
		qry.NoFilters();
		GM_INT_PARAM(iTeam, 1, 0);
		qry.Team(iTeam);
		p++;
	}

	if(GM_NUM_PARAMS > p)
	{
		switch(GM_THREAD_ARG->ParamType(p))
		{
		case GM_INT:
			qry.AddType(GM_THREAD_ARG->ParamInt(p));
			break;
		case GM_STRING:
			qry.Expression(GM_THREAD_ARG->ParamString(p));
			break;
		case GM_TABLE:
			{
				gmTableObject *typesTable = GM_THREAD_ARG->ParamTable(p);
				if(typesTable->Count() > GoalManager::Query::MaxGoalTypes)
				{
					GM_EXCEPTION_MSG("maximum count of goal types in query is %d, got %d", GoalManager::Query::MaxGoalTypes, typesTable->Count());
					return GM_EXCEPTION;
				}
				gmTableIterator tIt;
				for(gmTableNode *pNode = typesTable->GetFirst(tIt); pNode; pNode = typesTable->GetNext(tIt))
				{
					if(pNode->m_value.m_type != GM_INT)
					{
						GM_EXCEPTION_MSG("expecting param %d as table of int, got %s", p, GM_THREAD_ARG->GetMachine()->GetTypeName(pNode->m_value.m_type));
						return GM_EXCEPTION;
					}
					qry.AddType(pNode->m_value.GetInt());
				}
			}
			break;
		case GM_NULL:
			break;
		default:
			GM_EXCEPTION_MSG("expecting param %d as string or int or table, got %s", p, GM_THREAD_ARG->ParamTypeName(p));
			return GM_EXCEPTION;
		}
	}

	// parse optional parameters
	p++;
	GM_TABLE_PARAM(params, p, 0);
	if(params)
	{
		qry.FromTable(a_thread->GetMachine(),params);
	}	

	if ( qry.GetError() != GoalManager::Query::QueryOk ) {
		GM_EXCEPTION_MSG(qry.QueryErrorString());
		return GM_EXCEPTION;
	}

	GoalManager::GetInstance()->GetGoals(qry);
	if ( qry.GetError() != GoalManager::Query::QueryOk ) {
		GM_EXCEPTION_MSG(qry.QueryErrorString());
		return GM_EXCEPTION;
	}

	if(!qry.m_List.empty())
	{
		gmMachine *pMachine = a_thread->GetMachine();
		DisableGCInScope gcEn(pMachine);

		for(obuint32 i = 0; i < qry.m_List.size(); ++i)
		{
			gmUserObject *pUser = qry.m_List[i]->GetScriptObject(pMachine);
			OBASSERT(pUser, "Invalid Object");

			table->Set(pMachine, i, gmVariable(pUser));
		}
	}
	a_thread->PushInt((gmint)qry.m_List.size());
	return GM_OK;
}

// function: SetMapGoalProperties
//		This functions is for setting properties on 1 or more map goals.
//
// Parameters:
//
//		string	- Expression of goals to apply properties to.
//		table	- Property table.
//
// Returns:
//		none
static int GM_CDECL gmfSetMapGoalProperties(gmThread *a_thread)
{
	GM_CHECK_STRING_PARAM(expr,0);
	GM_CHECK_TABLE_PARAM(props,1);

	if(!GoalManager::GetInstance()->Iterate(expr, [=](MapGoal *g) { g->FromScriptTable(a_thread->GetMachine(),props,false); }))
		MapDebugPrint(a_thread, va("SetMapGoalProperties: goal query for %s has no results", expr));
	return GM_OK;
}

static void MapDebugPrint(gmMachine *a_machine, int threadId, const char *message)
{
	gmCall call;
	if(call.BeginTableFunction(a_machine, "MapDebugPrint", "Util"))
	{
		call.AddParamString(message);
		call.AddParamInt(2);
		if(threadId == CommandReciever::m_ConsoleCommandThreadId && threadId)
			CommandReciever::m_MapDebugPrintThreadId = call.GetThread()->GetId();
		call.End();
	}
}

void MapDebugPrint(gmThread *a_thread, const char *message)
{
	MapDebugPrint(a_thread->GetMachine(), a_thread->GetId(), message);
}

void MapDebugPrint(const char *message)
{
	MapDebugPrint(ScriptManager::GetInstance()->GetMachine(), 0, message);
}

static int SetAvailableMapGoals(gmThread *a_thread, int team, bool available, const char* expr, int ignoreErrors)
{
	int n = GoalManager::GetInstance()->Iterate(expr, [=](MapGoal *g) { g->SetAvailable(team,  available); });
	if(n==0 && !ignoreErrors)
		MapDebugPrint(a_thread, va("SetAvailableMapGoals: goal query for %s has no results", expr));
	return n;
}

// function: SetAvailableMapGoals
//		This function enables/disables map goals
//
// Parameters:
//
//		int		- The team the goal should be for. 0 means any team. See global team table.
//		int		- True to enable, false to disable.
//		string or table	- OPTIONAL - The expression to use to match the goal names.
//
// Returns:
//		int - count of goals
static int GM_CDECL gmfSetAvailableMapGoals(gmThread *a_thread)
{
	GM_CHECK_INT_PARAM(team, 0);
	GM_CHECK_INT_PARAM(enable, 1);
	GM_INT_PARAM(ignoreErrors, 3, 0);

	int size = 0;
	if(a_thread->GetNumParams() < 3)
	{
		size = SetAvailableMapGoals(a_thread, team, enable != 0, 0, 0);
	}
	else if(a_thread->ParamType(2)==GM_STRING)
	{
		size = SetAvailableMapGoals(a_thread, team, enable != 0, a_thread->ParamString(2), ignoreErrors);
	}
	else if(a_thread->ParamType(2)==GM_TABLE)
	{
		gmTableObject* tbl = a_thread->ParamTable(2);
		gmTableIterator tIt;
		for(gmTableNode *pNode = tbl->GetFirst(tIt); pNode; pNode = tbl->GetNext(tIt))
		{
			if(!pNode->m_value.IsString())
			{
				GM_EXCEPTION_MSG("expecting param 2 as table of string, got %s", a_thread->GetMachine()->GetTypeName(pNode->m_value.m_type));
				return GM_EXCEPTION;
			}
			size += SetAvailableMapGoals(a_thread, team, enable != 0, pNode->m_value.GetCStringSafe(0), ignoreErrors);
		}
	}
	else
	{
		MapDebugPrint(a_thread, "SetAvailableMapGoals: Parameter 3 must be a string or table");
	}

	a_thread->PushInt(size);
	return GM_OK;
}

		
// function: SetGoalPriority
//		This function sets the bias for a selection of goals
//
// Parameters:
//
//		string	- The expression to use to match the goal names.
//		int		- Team Id to set priority for.
//		int		- Class Id to set priority for.
//		float	- The priority to set
//		int - OPTIONAL - true for dynamic goals (FLAGRETURN, DEFUSE)
//
// Returns:
//		none
static int GM_CDECL gmfSetGoalPriorityForTeamClass(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(exp,0);
	GM_CHECK_FLOAT_OR_INT_PARAM(priority,1);
	GM_INT_PARAM(teamId,2,0);
	GM_INT_PARAM(classId,3,0);
	GM_INT_PARAM(persis,4,0);
	
	if(!GoalManager::GetInstance()->Iterate(exp, [=](MapGoal *g) { g->SetPriorityForClass(teamId, classId, priority); }) && !persis)
		MapDebugPrint(a_thread, va("SetGoalPriority: goal query for %s has no results", exp));

	if(persis) MapGoal::SetPersistentPriorityForClass(exp,teamId,classId,priority);
	return GM_OK;
}

static int GM_CDECL SetOrClearGoalRole(gmThread *a_thread, bool enable)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(exp, 0);

	int persis = 0;
	if(enable){
		GM_INT_PARAM(persis0, 2, 0);
		persis=persis0;
	}

	obint32 roleInt = 0;
	switch(a_thread->ParamType(1))
	{
		case GM_INT:
			roleInt = 1<<a_thread->ParamInt(1);
			break;
		case GM_TABLE:
		{
			gmTableObject* tbl = a_thread->ParamTable(1);
			gmTableIterator tIt;
			for(gmTableNode *pNode = tbl->GetFirst(tIt); pNode; pNode = tbl->GetNext(tIt))
			{
				if(!pNode->m_value.IsInt())
				{
					GM_EXCEPTION_MSG("expecting param 1 as table of int, got %s", a_thread->GetMachine()->GetTypeName(pNode->m_value.m_type));
					return GM_EXCEPTION;
				}
				roleInt |= 1<<pNode->m_value.GetInt();
			}
			break;
		}
		default:
			GM_EXCEPTION_MSG("expecting param 1 as int or table, got %s", a_thread->ParamTypeName(1));
			return GM_EXCEPTION;
	}
	BitFlag32 role(roleInt);

	if(!GoalManager::GetInstance()->Iterate(exp, [=](MapGoal *g){ 
			BitFlag32 oldRole = g->GetRoleMask();
			g->SetRoleMask(enable ? (oldRole | role) : (oldRole & ~role));
		}) && !persis)
		MapDebugPrint(a_thread, va("%s: goal query for %s has no results", enable ? "SetGoalRole" : "ClearGoalRole", exp));

	if(persis) MapGoal::SetPersistentRole(exp, role);
	return GM_OK;
}

// function: SetGoalRole
//		This function sets roles for goals, it preserves existing roles
//
// Parameters:
//
//		string	- The regular expression to use to match the goal names.
//		int	- roles bit mask
//		int - OPTIONAL - true for dynamic goals (DEFUSE)
//
// Returns:
//		none
static int GM_CDECL gmfSetGoalRole(gmThread *a_thread)
{
	return SetOrClearGoalRole(a_thread, true);
}

// function: ClearGoalRole
//		This function clears roles for goals
//
// Parameters:
//
//		string	- The regular expression to use to match the goal names.
//		int	- roles bit mask
//
// Returns:
//		none
static int GM_CDECL gmfClearGoalRole(gmThread *a_thread)
{
	return SetOrClearGoalRole(a_thread, false);
}


// function: SetGoalGroup
//		This function sets the group for a selection of goals
//
// Parameters:
//
//		string	- The group expression to use to match the goal names. -OR- indexed string table of goal expressions
//		string - The group name to use.
//
// Returns:
//		none
static int GM_CDECL gmfSetGoalGroup(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(2);	
	GM_CHECK_STRING_PARAM(group,1);
	
	if(a_thread->ParamType(0)==GM_TABLE)
	{
		GM_TABLE_PARAM(tbl,0,0);
		if(tbl)
		{
			gmTableIterator tIt;
			gmTableNode *pNode = tbl->GetFirst(tIt);
			while(pNode)
			{
				const char *pGoalName = pNode->m_value.GetCStringSafe(0);
				MapGoalPtr mg = pGoalName ? GoalManager::GetInstance()->GetGoal(pGoalName) : MapGoalPtr();
				if(mg)
				{
					mg->SetGroupName(group);				
				}
				pNode = tbl->GetNext(tIt);
			}
		}
	}
	else if(a_thread->ParamType(0)==GM_STRING)
	{
		GM_STRING_PARAM(exp,0,0);
		GoalManager::GetInstance()->Iterate(exp, [=](MapGoal *g){ g->SetGroupName(group); });
	}
	else
	{
		GM_EXCEPTION_MSG("expected param 0 as table or string");
		return GM_EXCEPTION;
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetGameEntityFromId
//		This function gets a <GameEntity> from a numeric game Id.
//
// Parameters:
//
//		int - The game Id to get the entity for.
//
// Returns:
//		<GameEntity> - The <GameEntity>, if it exists.
//		- OR -
//		null - If no <GameEntity> exists for that Id.
static int gmfGetGameEntityFromId(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(Id, 0);

	GameEntity ent = g_EngineFuncs->EntityFromID(Id);

	if(ent.IsValid())
	{
		gmVariable v;
		v.SetEntity(ent.AsInt());
		a_thread->Push(v);
	}
	else
		a_thread->PushNull();

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetGameIdFromEntity
//		This function gets a numeric game Id from a <GameEntity>.
//
// Parameters:
//
//		<GameEntity> - The <GameEntity> of a game entity.
//
// Returns:
//		int - Numeric game Id for the provided entity.
//		- OR -
//		null - If the entity is invalid
static int gmfGetGameIdFromEntity(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GameId Id = g_EngineFuncs->IDFromEntity(gameEnt);
	if(Id != -1)
		a_thread->PushInt(Id);
	else
		a_thread->PushNull();

	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////

// function: TraceLine
//		This function does a custom collision ray-test. Useful for line of sight tests.
//
// Parameters:
//
//		Vector3 - 3d Start position of the trace.
//		Vector3 - 3d End position of the trace.
//		int - AABB to use for trace line.
//		int - Collision masks to use for the <TraceLine>. See the global TRACE table for flags. default MASK_SHOT
//		int - GameId of an entity to ignore in the trace. Usually the source entity of the trace. default -1
//		int	- true/false to use PVS or not. Results in faster check, but doesn't return info about the collision. default false
//
// Returns:
//		table - traceline result table, with the following values. fraction, startsolid, entity, normal, end.
//				first 2 are guaranteed, others are conditional on if the ray hit something
static int gmfTraceLine(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_VECTOR_PARAM(sv,0);
	GM_CHECK_VECTOR_PARAM(ev,1);
	GM_GMBIND_PARAM(AABB*, gmAABB, bbox, 2, NULL);
	GM_INT_PARAM(iMask, 3, TR_MASK_SHOT);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 4);	
	//GM_INT_PARAM(iUser, 4, -1); 

	GM_INT_PARAM(iUsePVS, 5, false);
	
	const int iUser = gameEnt.IsValid() ? g_EngineFuncs->IDFromEntity(gameEnt) : -1;	

	obTraceResult tr;
	EngineFuncs::TraceLine(tr, Vector3f(sv.x,sv.y,sv.z), Vector3f(ev.x,ev.y,ev.z), bbox, 
		iMask, iUser, iUsePVS == False ? False : True);

	gmMachine *pMachine = a_thread->GetMachine();
	DisableGCInScope gcEn(pMachine);

	gmTableObject *pTable = pMachine->AllocTableObject();
	pTable->Set(pMachine, "fraction", gmVariable(tr.m_Fraction));
	pTable->Set(pMachine, "startsolid", gmVariable(tr.m_StartSolid ? 1 : 0));
	if(tr.m_Fraction < 1.0)
	{
		if(tr.m_HitEntity.IsValid())
		{
			gmVariable v;
			v.SetEntity(tr.m_HitEntity.AsInt());
			pTable->Set(pMachine, "entity", v);
		}

		pTable->Set(pMachine, "normal", gmVariable(tr.m_Normal[0], tr.m_Normal[1], tr.m_Normal[2]));
		pTable->Set(pMachine, "end", gmVariable(tr.m_Endpos[0], tr.m_Endpos[1], tr.m_Endpos[2]));

		pTable->Set(pMachine, "contents", gmVariable(tr.m_Contents));
		pTable->Set(pMachine, "surface", gmVariable(tr.m_Surface));
	}

	a_thread->PushTable(pTable);
	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////

// function: GroundPoint
//		This function returns a point on the ground directly below the provided point
//
// Parameters:
//
//		Vector3 - 3d position of the trace.
//		int - Collision masks to use for the <TraceLine>. See the global TRACE table for flags. default MASK_SHOT
//
// Returns:
//		table - traceline result table, with the following values. fraction, startsolid, entity, normal, end.
//				first 2 are guaranteed, others are conditional on if the ray hit something
static int gmfGroundPoint(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(pt,0);
	GM_FLOAT_OR_INT_PARAM(offset,1,0.f)
	GM_INT_PARAM(iMask, 1, TR_MASK_SHOT);

	Vector3f vPt(pt.x,pt.y,pt.z);

	obTraceResult tr;
	EngineFuncs::TraceLine(tr,vPt,vPt.AddZ(-1024.f),NULL,iMask,-1,False);
	if(tr.m_Fraction<1.f)
		vPt = tr.m_Endpos;

	vPt.z -= offset;

	a_thread->PushVector(vPt);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DrawDebugLine
//		This function will draw a colored line in the game world. Useful for debugging.
//
// Parameters:
//
//		Vector3 - 3d Start position of the line.
//		Vector3 - 3d End position of the line.
//		int - Color of the line.
//		float - duration of the line
//
// Returns:
//		None
static int gmfDrawDebugLine(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(4);
	GM_CHECK_VECTOR_PARAM(sv,0);
	GM_CHECK_VECTOR_PARAM(ev,1);
	GM_CHECK_INT_PARAM(color, 2);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 3);
	Utils::DrawLine(Vector3f(sv.x,sv.y,sv.z), Vector3f(ev.x,ev.y,ev.z), obColor(color), duration);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DrawDebugArrow
//		This function will draw a colored line in the game world. Useful for debugging.
//
// Parameters:
//
//		Vector3 - 3d Start position of the line.
//		Vector3 - 3d End position of the line.
//		int - Color of the line.
//		float - duration of the line
//
// Returns:
//		None
static int gmfDrawDebugArrow(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(4);
	GM_CHECK_VECTOR_PARAM(sv,0);
	GM_CHECK_VECTOR_PARAM(ev,1);
	GM_CHECK_INT_PARAM(color, 2);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 3);
	Utils::DrawArrow(Vector3f(sv.x,sv.y,sv.z), Vector3f(ev.x,ev.y,ev.z), obColor(color), duration);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DrawDebugAABB
//		This function will draw a colored AABB in the game world. Useful for debugging.
//
// Parameters:
//
//		AABB - AABB to draw.
//		int - Color of the AABB.
//		float - duration of the line
//
// Returns:
//		None
static int gmfDrawDebugAABB(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(3);
	if(a_thread->ParamType(0) != gmAABB::GetType())
	{
		GM_EXCEPTION_MSG("expecting param 0 as user type %d", gmAABB::GetType());
		return GM_EXCEPTION;
	}	
	GM_CHECK_GMBIND_PARAM(AABB*, gmAABB, aabb, 0);
	GM_CHECK_INT_PARAM(color, 1);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 2);

	Utils::OutlineAABB(*aabb, color, duration);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DrawDebugRadius
//		This function will draw a colored radius in the game world. Useful for debugging.
//
// Parameters:
//
//		Vector3 - center
//		float - radius
//		int - Color of the AABB.
//		float - duration of the line
//
// Returns:
//		None
static int gmfDrawDebugRadius(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(4);
	GM_CHECK_VECTOR_PARAM(v,0);
	GM_CHECK_FLOAT_OR_INT_PARAM(rad,1);
	GM_CHECK_INT_PARAM(color, 2);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 3);
	Utils::DrawRadius(Vector3f(v.x,v.y,v.z), rad, obColor(color), duration);
	return GM_OK;
}

// function: DrawDebugText3d
//		This function will Draw Text to a 3d location.
//
// Parameters:
//
//		float - duration of the message(in seconds)
//		string - The message to print
//
// Returns:
//		None
static int GM_CDECL gmfDrawDebugText3d(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(4);
	GM_CHECK_VECTOR_PARAM(v,0);
	GM_CHECK_STRING_PARAM(msg, 1);
	GM_CHECK_INT_PARAM(color, 2);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 3);
	GM_FLOAT_OR_INT_PARAM(radius, 4, 1024.f);

	if(radius != Utils::FloatMax)
	{
		Vector3f vLocalPos;
		if(Utils::GetLocalPosition(vLocalPos) && Length(vLocalPos,Vector3f(v)) >= radius)
		{
			return GM_OK;
		}
	}

	Vector3f vec(v);
	Utils::PrintText(vec,obColor(color),duration,msg);
	return GM_OK;
}

static int GM_CDECL gmfTransformAndDrawLineList(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(5);
	GM_CHECK_TABLE_PARAM(veclist,0);
	GM_CHECK_INT_PARAM(color, 1);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 2);
	GM_CHECK_VECTOR_PARAM(position,3);
	GM_CHECK_VECTOR_PARAM(euler,4);
	
	/*Matrix3f m;
	m.FromEulerAnglesZXY(euler.x,euler.y,euler.z);*/

	Quaternionf q;
	q.FromAxisAngle(Vector3f::UNIT_Z,-euler.x);

	const int PointCount = veclist->Count();
	if(PointCount > 2)
	{
		Vector3f *vecs = (Vector3f*)StackAlloc(sizeof(Vec3)*PointCount);

		int CurrentPoint = 0;
		gmTableIterator tIt;
		gmTableNode *pNode = veclist->GetFirst(tIt);
		while(pNode)
		{
			Vec3 v;
			if(!pNode->m_value.GetVector(v))
			{
				GM_EXCEPTION_MSG("Expected table of Vec3");
				return GM_EXCEPTION;
			}

			vecs[CurrentPoint++] = Vector3f(position) + q.Rotate(Vector3f(v));
			pNode = veclist->GetNext(tIt);
		}

		// draw the line list.
		for(int i = 0; i < PointCount; i+=2)
		{
			Utils::DrawLine(
				vecs[i], 
				vecs[i+1], 
				obColor(color), 
				duration);
		}
	}	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DrawTrajectory
//		This function will draw a colored line in the game world. Useful for debugging.
//
// Parameters:
//
//		Vector3 - 3d Start position of the trajectory.
//		Vector3 - 3d velocity of the trajectory.
//		float or int - interval to sample trajectory for drawing
//		float or int - max duration of the trajectory to draw
//		int - Color of the line.
//		float - duration of the line
//
// Returns:
//		None
static int gmfDrawTrajectory(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(3);
	GM_CHECK_TABLE_PARAM(tbl,0);
	GM_CHECK_INT_PARAM(color, 1);
	GM_CHECK_FLOAT_OR_INT_PARAM(duration, 2);

	Trajectory::TrajectorySim traj;
	const int Res = traj.FromTable(a_thread,tbl);
	if(Res==GM_OK)
	{
		traj.Render(obColor(color),duration);
		a_thread->PushVector(traj.m_StopPos);
	}	
	return Res;
}


//////////////////////////////////////////////////////////////////////////

// function: GetMapName
//		This function will get the map name for the currently running map.
//
// Parameters:
//
//		None
//
// Returns:
//		string - The currently running map name.
//		- OR - 
//		null - If no map is loaded, or there was an error.
static int GM_CDECL gmfGetMapName(gmThread *a_thread)
{
	const char *pMapName = g_EngineFuncs->GetMapName();
	if(pMapName)
	{
		a_thread->PushString(a_thread->GetMachine()->AllocStringObject(pMapName));
	}
	else
	{
		a_thread->PushNull();
	}

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetMapExtents
//		This function will get the map extents for the currently running map.
//
// Parameters:
//
//		None
//
// Returns:
//		<gmAABB> - Axis aligned box for the map extents.
static int GM_CDECL gmfGetMapExtents(gmThread *a_thread)
{
	GM_GMBIND_PARAM(AABB*, gmAABB, aabb, 0, NULL);
	AABB mapaabb;
	g_EngineFuncs->GetMapExtents(mapaabb);
	if(aabb)
		*aabb = mapaabb;
	else
		gmAABB::PushObject(a_thread, mapaabb);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetPointContents
//		This function will get the contents of a 3d point.
//
// Parameters:
//
//		<Vector3> - 
//
// Returns:
//		int - The point content flags for this position. 
static int GM_CDECL gmfGetPointContents(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	a_thread->PushInt(g_EngineFuncs->GetPointContents(Vector3f(v.x,v.y,v.z)));
	return GM_OK;
}

////////////////////////////////////////////////////////////////////////////

// function: GetTime
//		This function will get the time from the bot. This time isn't necessarily match time,
//		or real time, so it should only be used on relative time comparisons.
//
// Parameters:
//
//		None
//
// Returns:
//		int - The current time, in milliseconds (1000ms = 1 second)
static int GM_CDECL gmfGetTime(gmThread *a_thread)
{
	a_thread->PushFloat(IGame::GetTimeSecs());
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// function: GetEntName
//		This function gets the name of the entity. Only useful for getting the 
//		player names of certain clients. Undefined if used on non clients->
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		string - The name of this entity
//		- OR -
//		null - If there was an error or the parameter was invalid
static int gmfGetEntityName(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	// See if we can get a proper gameentity
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	const char *pName = g_EngineFuncs->GetEntityName(gameEnt);
	if(pName)
		a_thread->PushNewString(pName);
	else
		a_thread->PushNull();
	return GM_OK;
}

// function: GetEntFacing
//		This function gets the facing of this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		Vector3 - 3d facing of this entity
//		- OR -
//		null - If there was an error or the parameter was invalid
static int gmfGetEntityFacing(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	// See if we can get a proper gameentity
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	Vector3f v = Vector3f::ZERO;
	if(gameEnt.IsValid() && EngineFuncs::EntityOrientation(gameEnt, v, NULL, NULL))
		a_thread->PushVector(v.x, v.y, v.z);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntPosition
//		This function gets the position of this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		Vector3 - 3d position of this entity
//		- OR -
//		null - If there was an error or the parameter was invalid
static int gmfGetEntityPosition(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	Vector3f v = Vector3f::ZERO;
	if(gameEnt.IsValid() && EngineFuncs::EntityPosition(gameEnt, v))
		a_thread->PushVector(v.x, v.y, v.z);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntBonePosition
//		This function gets the position of a bone on this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<int> - Bone to look for, See <SkeletonBone> enum
//
// Returns:
//		Vector3 - 3d position of this entity
//		- OR -
//		null - If there was an error or the parameter was invalid
static int gmfGetEntityBonePosition(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");
	GM_CHECK_INT_PARAM(boneid, 1);

	Vector3f v = Vector3f::ZERO;
	if(gameEnt.IsValid() && EngineFuncs::EntityBonePosition(gameEnt, boneid, v))
		a_thread->PushVector(v.x, v.y, v.z);
	else
		a_thread->PushNull();
	return GM_OK;
}
// function: GetEntEyePosition
//		This function gets the eye position of this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		Vector3 - 3d eye position of this entity
//		- OR -
//		null - If there was an error or the parameter was invalid
static int gmfGetEntEyePosition(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	Vector3f v = Vector3f::ZERO;
	if(gameEnt.IsValid() && EngineFuncs::EntityEyePosition(gameEnt, v))
		a_thread->PushVector(v.x, v.y, v.z);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntVelocity
//		This function gets the velocity of this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		Vector3 - 3d velocity of this entity
//		- OR -
//		null - If there was an error or the parameter was invalid
static int gmfGetEntityVelocity(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	Vector3f v = Vector3f::ZERO;
	if(gameEnt.IsValid() && EngineFuncs::EntityVelocity(gameEnt, v))
		a_thread->PushVector(v.x, v.y, v.z);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntRotationMatrix
//		This function gets the full matrix transform for this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		Matrix3 - transform matrix of this entity
//		- OR -
//		null - If there was an error or the parameter was invalid
static int gmfGetEntRotationMatrix(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GM_GMBIND_PARAM(gmMat3Type*, gmMatrix3, mat, 1, NULL);

	Vector3f vForward, vRight, vUp;
	if(gameEnt.IsValid() && EngineFuncs::EntityOrientation(gameEnt, vForward, vRight, vUp))
	{
		if(mat)
		{
			*mat = Matrix3f(vRight, vForward, vUp, true);
			a_thread->PushInt(1);
		}
		else
			gmMatrix3::PushObject(a_thread, Matrix3f(vRight, vForward, vUp, true));
	}
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntFlags
//		This function gets the flags of this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<int> - The flag to check for
//		... - Any number of additional flags to check for
//
// Returns:
//		int - true if entity has ANY flag passed
static int gmfGetEntityFlags(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	BitFlag64 targetFlags;
	if(gameEnt.IsValid() && InterfaceFuncs::GetEntityFlags(gameEnt, targetFlags))
	{
		for(int i = 1; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(flag, i);
			if(targetFlags.CheckFlag(flag))
			{
				a_thread->PushInt(1);
				return GM_OK;
			}
		}
	}
	a_thread->PushInt(0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// function: GetEntPowerups
//		This function gets the powerups on this entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		int - true if entity has ALL powerups passed
static int gmfGetEntityPowerups(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	//bool bSuccess = false;
	BitFlag64 targetFlags;
	if(gameEnt.IsValid() && InterfaceFuncs::GetEntityPowerUps(gameEnt, targetFlags))
	{
		//bSuccess = true;
		for(int i = 1; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(flag, i);
			if(targetFlags.CheckFlag(flag))
			{
				a_thread->PushInt(1);
				return GM_OK;
			}
		}
	}
	a_thread->PushInt(0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntHealthAndArmor
//		This function gets the health and armor status of this entity. The structure
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		<table> - Health, MaxHealth, Armor, MaxArmor fields.
//		- OR -
//		null - if there was an error or the parameter was invalid
static int gmfGetEntityHealthAndArmor(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GM_CHECK_TABLE_PARAM(tbl,1);

	DisableGCInScope gcEn(a_thread->GetMachine());

	if(!tbl)
		tbl = a_thread->GetMachine()->AllocTableObject();

	Msg_HealthArmor hlthArmor;
	if(tbl != NULL && gameEnt.IsValid() && InterfaceFuncs::GetHealthAndArmor(gameEnt, hlthArmor))
	{
		tbl->Set(a_thread->GetMachine(),"Health",gmVariable(hlthArmor.m_CurrentHealth));
		tbl->Set(a_thread->GetMachine(),"MaxHealth",gmVariable(hlthArmor.m_MaxHealth));
		tbl->Set(a_thread->GetMachine(),"Armor",gmVariable(hlthArmor.m_CurrentArmor));
		tbl->Set(a_thread->GetMachine(),"MaxArmor",gmVariable(hlthArmor.m_MaxArmor));
		a_thread->PushInt(1);
	}
	else
	{
		a_thread->PushNull();
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntWorldAABB
//		This function gets the world AABB for this entity. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<AABB> - OPTIONAL - If provided, will recieve the entity aabb rather than the return value(saves memory allocations)
//
// Returns:
//		<gm - aabb> - The axis aligned bounding box for this entity.
//		- OR -
//		null - if there was an error or the parameter was invalid
//
// Returns:
//		<gm - aabb> - AABB of this entity.
//		- OR -
//		null - if there was an error or the parameter was invalid
static int gmfGetEntityAABB(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GM_GMBIND_PARAM(AABB*, gmAABB, aabb, 1, NULL);

	AABB worldaabb;
	if(gameEnt.IsValid() && EngineFuncs::EntityWorldAABB(gameEnt, worldaabb))
	{
		if(aabb)
		{
			*aabb = worldaabb;
			a_thread->PushInt(1);
		}
		else
			gmAABB::PushObject(a_thread, worldaabb);
	}
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntOwner
//		This function gets the current owner of this goal. Usually for
//		flags or carryable goals it will get who is carrying it.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		<GameEntity> of the owner of this entity
//		- OR -
//		null - If no owner.
static int gmfGetEntityOwner(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GameEntity owner = g_EngineFuncs->GetEntityOwner(gameEnt);
	if(owner.IsValid())
		a_thread->PushEntity(owner.AsInt());
	else
		a_thread->PushNull();	

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntTeam
//		This function gets the current team of this entity. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		int - team of the owner of this entity
//		- OR -
//		null - If no owner.
static int gmfGetEntityTeam(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	int iTeam = gameEnt.IsValid() ? InterfaceFuncs::GetEntityTeam(gameEnt) : 0;
	if(iTeam != 0)
		a_thread->PushInt(iTeam);
	else
		a_thread->PushNull();	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntClass
//		This function gets the current class of this entity. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		int - team of the owner of this entity
//		- OR -
//		null - If no owner.
static int gmfGetEntityClass(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	int iClass = gameEnt.IsValid() ? InterfaceFuncs::GetEntityClass(gameEnt) : 0;
	if(iClass != 0)
		a_thread->PushInt(iClass);
	else
		a_thread->PushNull();	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetClassNameFromId
//		This function gets the current class name from a class id. 
//
// Parameters:
//
//		<int> - class id
//
// Returns:
//		string - class name of id
//		- OR -
//		null - If not found.
static int gmfGetClassNameFromId(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(classId, 0);

	const char *pClassName = IGameManager::GetInstance()->GetGame()->FindClassName(classId);
	if(pClassName)
		a_thread->PushNewString(pClassName);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetWeaponIdFromClassId
//		This function gets the current weapon id from a class id. 
//
// Parameters:
//
//		<int> - class id
//
// Returns:
//		int - weapon Id(to match WEAPON table), if the classId is a weapon
//		- OR -
//		null - If not found.
static int gmfGetWeaponIdFromClassId(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(classId, 0);

	const int weaponId = IGameManager::GetInstance()->GetGame()->FindWeaponId(classId);
	if(weaponId)
		a_thread->PushInt(weaponId);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntCategory
//		This function gets the current category of this entity. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		int - category of the entity, as a bitmask.
//		- OR -
//		null - If no owner.
static int gmfGetEntCategory(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	BitFlag32 category;
	if(gameEnt.IsValid() && InterfaceFuncs::GetEntityCategory(gameEnt, category))
	{
		for(int i = 1; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(flag, i);
			if(category.CheckFlag(flag))
			{
				a_thread->PushInt(1);
				return GM_OK;
			}
		}
	}
	a_thread->PushInt(0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntEquippedWeapon
//		This function gets the current weapon of this entity. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		int - weapon id
//		- OR -
//		null - If no owner.
static int gmfGetEntEquippedWeapon(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	int iWeapon = gameEnt.IsValid() ? InterfaceFuncs::GetEquippedWeapon(gameEnt).m_WeaponId : 0;
	if (iWeapon != 0)
		a_thread->PushInt(iWeapon);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntityLocalSpace
//		This function gets the local space position of a vector. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<Vector3> - The 3d vector to convert to local space.
//
// Returns:
//		<Vector3> - The local space coordinate
static int gmfGetEntityToLocalSpace(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GM_CHECK_VECTOR_PARAM(v,1);

	Vector3f vl;
	if(Utils::ToLocalSpace(gameEnt, Vector3f(v.x,v.y,v.z), vl))
		a_thread->PushVector(vl.x,vl.y,vl.z);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntityWorldSpace
//		This function gets the world space position of a vector. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<Vector3> - The 3d vector to convert to world space.
//
// Returns:
//		<Vector3> - The world space coordinate
static int gmfGetEntityToWorldSpace(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GM_CHECK_VECTOR_PARAM(v,1);

	Vector3f vw;
	if(Utils::ToWorldSpace(gameEnt, Vector3f(v.x,v.y,v.z), vw))
		a_thread->PushVector(vw.x,vw.y,vw.z);
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntityInSphere
//		This function searches for an entity of a certain type. 
//
// Parameters:
//
//		<Vector3> - The position to search at
//		<float> - The radius to search within
//		<int> - Class Id to search for
//		<GameEntity>
//		- OR - 
//		<int> - The gameId to start from
//
// Returns:
//		<GameEntity> - The next entity within the radius of the class requested
static int gmfGetEntityInSphere(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(4);
	GM_CHECK_VECTOR_PARAM(v, 0);
	GM_CHECK_FLOAT_OR_INT_PARAM(radius, 1);
	GM_CHECK_INT_PARAM(classId, 2);

	GameEntity gameEnt;
	GM_GAMEENTITY_FROM_PARAM(gameEnt, 3, GameEntity());	

	GameEntity ent = g_EngineFuncs->FindEntityInSphere(Vector3f(v.x,v.y,v.z), radius, gameEnt, classId);
	if(ent.IsValid())
	{
		gmVariable out;
		out.SetEntity(ent.AsInt());
		a_thread->Push(out);
	}
	else
	{
		a_thread->PushNull();
	}
	return GM_OK;
}

static int gmfGetEntityByName(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(entname, 0);

	GameEntity ent = g_EngineFuncs->EntityByName(entname);
	if(ent.IsValid())
	{
		gmVariable v;
		v.SetEntity(ent.AsInt());
		a_thread->Push(v);
	}
	else
	{
		a_thread->PushNull();
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetEntityStat
//		This function gets a named stat from an entity. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<string> - The name of the stat to retrieve
//
// Returns:
//		<variable> - Resulting data
static int gmfGetEntityStat(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");
	GM_CHECK_STRING_PARAM(statname, 1);

	obUserData d = InterfaceFuncs::GetEntityStat(gameEnt, statname);
	a_thread->Push(Utils::UserDataToGmVar(a_thread->GetMachine(),d));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: EntityIsValid
//		Checks if a given entity is valid
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		bool
static int gmfEntityIsValid(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	

	bool valid = gameEnt.IsValid();

	if(valid && a_thread->ParamType(0) == GM_ENTITY && 
		g_EngineFuncs->IDFromEntity(gameEnt) == -1)
		valid = false;

	a_thread->PushInt(valid ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetTeamStat
//		This function gets a named stat from a team. 
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<string> - The name of the stat to retrieve
//
// Returns:
//		<variable> - Resulting data
static int gmfGetTeamStat(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(team, 0);
	GM_CHECK_STRING_PARAM(statname, 1);

	obUserData d = InterfaceFuncs::GetTeamStat(team, statname);
	a_thread->Push(Utils::UserDataToGmVar(a_thread->GetMachine(),d));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DistanceBetween
//		Gets the distance between 2 entities. 
//
// Parameters:
//		<Vector3> - The 3d vector to convert to calculate distance to.
//		- OR - 
//		<GameEntity> - The first entity to use
//		- OR - 
//		<MapGoal> - The first goal to use
//		- OR - 
//		<int> - The gameId for the first entity to use
//		<Vector3> - The 3d vector to convert to calculate distance to.
//		- OR - 
//		<GameEntity> - The second entity to use
//		- OR - 
//		<MapGoal> - The second goal to use
//		- OR - 
//		<int> - The gameId for the second entity to use
//
// Returns:
//		<float> - Distance between 2 entities
//		- OR -
//		null - If one or both entities were invalid.
static int gmfDistanceBetween(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	Vector3f vPosition1, vPosition2;
	MapGoal *Mg = 0;

	if(a_thread->ParamType(0) == GM_VEC3)
	{
		GM_CHECK_VECTOR_PARAM(v,0);
		vPosition1 = Vector3f(v.x,v.y,v.z);
	}
	else if(gmBind2::Class<MapGoal>::FromVar(a_thread, a_thread->Param(0), Mg) && Mg)
	{
		vPosition1 = Mg->GetPosition();
	}
	else
	{
		GameEntity gameEnt;
		GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
		if(!EngineFuncs::EntityPosition(gameEnt, vPosition1))
		{
			GM_EXCEPTION_MSG("Expected Vector3/GameEntity/GameId for param %d got %s!", 0,
				a_thread->GetMachine()->GetTypeName(a_thread->ParamType(0))); 
			return GM_EXCEPTION;
		}
	}

	if(a_thread->ParamType(1) == GM_VEC3)
	{
		GM_CHECK_VECTOR_PARAM(v,1);
		vPosition2 = Vector3f(v.x,v.y,v.z);
	}
	else if(gmBind2::Class<MapGoal>::FromVar(a_thread, a_thread->Param(1), Mg) && Mg)
	{
		vPosition2 = Mg->GetPosition();
	}
	else
	{
		GameEntity gameEnt;
		GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 1);
		if(!EngineFuncs::EntityPosition(gameEnt, vPosition2))
		{
			GM_EXCEPTION_MSG("Expected Vector3/GameEntity/GameId for param %d got %s!", 1,
				a_thread->GetMachine()->GetTypeName(a_thread->ParamType(1))); 
			return GM_EXCEPTION;
		}
	}

	a_thread->PushFloat((vPosition1 - vPosition2).Length());
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: CalcTrajectory
//		Calculate an aim vector for a trajectory. 
//
// Parameters:
//		<Vector3> - Start Position
//		<Vector3> - Target Position
//		<int> OR <float> - Projectile Speed
//		<int> OR <float> - Projectile Gravity
//
// Returns:
//		<table> - 1 or 2 aim vectors for valid trajectories
//		- OR -
//		null - If no valid trajectory exists
static int gmfCalculateTrajectory(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(4);
	GM_CHECK_VECTOR_PARAM(v1,0);
	GM_CHECK_VECTOR_PARAM(v2,1);
	GM_CHECK_FLOAT_OR_INT_PARAM(fProjectileSpeed, 2);
	GM_CHECK_FLOAT_OR_INT_PARAM(fProjectileGravity, 3);

	Trajectory::AimTrajectory traj[2];
	int t = Trajectory::Calculate(
		Vector3f(v1.x,v1.y,v1.z), 
		Vector3f(v2.x,v2.y,v2.z), 
		fProjectileSpeed, 
		IGame::GetGravity() * fProjectileGravity, 
		traj);

	if(t > 0)
	{
		gmMachine *pMachine = a_thread->GetMachine();		
		DisableGCInScope gcEn(pMachine);

		gmTableObject *pTbl = pMachine->AllocTableObject();
		for(int i = 0; i < t; ++i)
		{
			gmVariable var;
			var.SetVector((float*)traj[i].m_AimVector);
			pTbl->Set(pMachine, i, var);
		}
		a_thread->PushTable(pTbl);
	}
	else
		a_thread->PushNull();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DrawEntityAABB
//		This function draws the AABB around an entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<float/int> - OPTIONAL -  - Duration to draw the aabb, in seconds default 2 seconds.
//		<int> - OPTIONAL - Color to draw the bounding box
//
// Returns:
//		int - 1 if successful, 0 if failure
static int gmfDrawEntityAABB(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GM_FLOAT_OR_INT_PARAM(duration, 1, 2.0f);
	GM_INT_PARAM(color, 2, COLOR::WHITE.rgba());

	AABB aabb;
	if(EngineFuncs::EntityWorldAABB(gameEnt, aabb))
	{
		Utils::OutlineAABB(aabb, obColor(color), duration);
		a_thread->PushInt(1);
	}

	a_thread->PushInt(0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: gmfDrawEntityOBB
//		This function draws the AABB around an entity.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<float/int> - OPTIONAL -  - Duration to draw the aabb, in seconds default 2 seconds.
//		<int> - OPTIONAL - Color to draw the bounding box
//
// Returns:
//		int - 1 if successful, 0 if failure
static int gmfDrawEntityOBB(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	GM_FLOAT_OR_INT_PARAM(duration, 1, 2.0f);
	GM_INT_PARAM(color, 2, COLOR::WHITE.rgba());

	Box3f obb;
	if(EngineFuncs::EntityWorldOBB(gameEnt, obb))
	{
		Utils::OutlineOBB(obb, obColor(color), duration);
		a_thread->PushInt(1);
	}

	a_thread->PushInt(0);
	return GM_OK;
}

static int gmfCheckEntityBoundsIntersect(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt0;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt0, 0);	
	OBASSERT(gameEnt0.IsValid(), "Bad Entity");

	GameEntity gameEnt1;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt1, 1);	
	OBASSERT(gameEnt1.IsValid(), "Bad Entity");

	Box3f obb0, obb1;
	if(EngineFuncs::EntityWorldOBB(gameEnt0, obb0) &&
		EngineFuncs::EntityWorldOBB(gameEnt1, obb1))
	{
		IntrBox3Box3f intr(obb0,obb1);
		a_thread->PushInt( intr.Test() ? 1 : 0 );
		return GM_OK;
	}
	a_thread->PushInt(0);
	return GM_OK;
}

// function: GetGameState
//		Gets the current game state as a string, see <eGameState> and <GameStates>
//
// Parameters:
//
//		none
//
// Returns:
//		string - current game state
static int GM_CDECL gmfGetGameState(gmThread *a_thread)
{
	a_thread->PushNewString(InterfaceFuncs::GetGameState(InterfaceFuncs::GetGameState()));
	return GM_OK;
}

// function: GetGameTimeLeft
//		Gets the current game time left.
//
// Parameters:
//
//		none
//
// Returns:
//		float - seconds left in game
static int GM_CDECL gmfGetGameTimeLeft(gmThread *a_thread)
{
	a_thread->PushFloat(InterfaceFuncs::GetGameTimeLeft());
	return GM_OK;
}

// function: ExecCommandOnClient
//		This function executes a command for the entity. Usually equivilent to console commands.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<string> - command to execute for the client
//
// Returns:
//		None
static int GM_CDECL gmfExecCommandOnClient(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(2);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	GM_CHECK_STRING_PARAM(msg, 1);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");
	if(gameEnt.IsValid())
	{
		int iGameId = g_EngineFuncs->IDFromEntity(gameEnt);
		g_EngineFuncs->BotCommand(iGameId, msg);
	}
	return GM_OK;
}

// function: GetGameName
//		Gets the name of the currently running game.
// Parameters:
//
//		None
//
// Returns:
//		<string> - Name of current game.
static int GM_CDECL gmfGetGameName(gmThread *a_thread)
{	
	a_thread->PushNewString(g_EngineFuncs->GetGameName());
	return GM_OK;
}

// function: GetModName
//		Gets the name of the currently running mod.
// Parameters:
//
//		None
//
// Returns:
//		<string> - Name of current mod.
static int GM_CDECL gmfGetModName(gmThread *a_thread)
{	
	a_thread->PushNewString(g_EngineFuncs->GetModName());
	return GM_OK;
}

// function: GetModVersion
//		Gets the version of the currently running mod.
// Parameters:
//
//		None
//
// Returns:
//		<string> - Version of current mod.
static int GM_CDECL gmfGetModVersion(gmThread *a_thread)
{	
	a_thread->PushNewString(g_EngineFuncs->GetModVers());
	return GM_OK;
}

// function: ShowPaths
//		Echo the paths of the mod out. Debugging purposes.
// Parameters:
//
//		None
//
static int GM_CDECL gmfShowPaths(gmThread *a_thread)
{
	IGame *pGame = IGameManager::GetInstance()->GetGame();
	if(pGame)
	{
		EngineFuncs::ConsoleMessage(va("Game: %s", pGame->GetGameName()));
		EngineFuncs::ConsoleMessage(va("Mod Folder: %s", Utils::GetModFolder().string().c_str()));
		EngineFuncs::ConsoleMessage(va("Nav Folder: %s", Utils::GetNavFolder().string().c_str()));
		EngineFuncs::ConsoleMessage(va("Script Folder: %s", Utils::GetScriptFolder().string().c_str()));
	}
	return GM_OK;
}

// function: GetGravity
//		Gets the current gravity.
// Parameters:
//
//		None
//
// Returns:
//		<float> - Current Gravity.
static int GM_CDECL gmfGetGravity(gmThread *a_thread)
{	
	a_thread->PushFloat(IGame::GetGravity());
	return GM_OK;
}

// function: GetCheats
//		Gets the current cheat setting.
// Parameters:
//
//		None
//
// Returns:
//		<int> - true if cheats enabled, false if not.
static int GM_CDECL gmfGetCheats(gmThread *a_thread)
{	
	a_thread->PushInt(IGame::GetCheatsEnabled());
	return GM_OK;
}

// function: ServerCommand
//		Runs a server command.
// Parameters:
//
//		string - Command to execute on the server.
//
// Returns:
//		none
static int GM_CDECL gmfServerCommand(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(cmd, 0);
	InterfaceFuncs::ServerCommand(cmd);
	return GM_OK;
}

static int GM_CDECL gmfServerScriptFunction(gmThread *a_thread)
{
	GM_CHECK_STRING_PARAM(entname, 0);
	GM_CHECK_STRING_PARAM(funcname, 1);
	
	GM_STRING_PARAM(p1, 2, "");
	GM_STRING_PARAM(p2, 3, "");
	GM_STRING_PARAM(p3, 4, "");
	InterfaceFuncs::ScriptEvent(funcname, entname, p1, p2, p3);
	return GM_OK;
}

static int GM_CDECL gmfGetLocalEntity(gmThread *a_thread)
{	
	if(Utils::GetLocalEntity().IsValid())
		a_thread->PushEntity(Utils::GetLocalEntity().AsInt());
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalPosition(gmThread *a_thread)
{	
	Vector3f v;
	if(Utils::GetLocalPosition(v))
		a_thread->PushVector(v);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalGroundPosition(gmThread *a_thread)
{	
	Vector3f v;
	if(Utils::GetLocalGroundPosition(v, TR_MASK_FLOODFILL))
		a_thread->PushVector(v);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalEyePosition(gmThread *a_thread)
{	
	Vector3f v;
	if(Utils::GetLocalEyePosition(v))
		a_thread->PushVector(v);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalFacing(gmThread *a_thread)
{	
	Vector3f v;
	if(Utils::GetLocalFacing(v))
		a_thread->PushVector(v);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalAABB(gmThread *a_thread)
{	
	AABB aabb;
	if(Utils::GetLocalAABB(aabb))
		gmAABB::PushObject(a_thread, aabb);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalAimPosition(gmThread *a_thread)
{	
	GM_INT_PARAM(mask,0,TR_MASK_FLOODFILL);
	Vector3f v, n;
	if(Utils::GetLocalAimPoint(v,&n,mask))
		a_thread->PushVector(v);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalAimNormal(gmThread *a_thread)
{	
	GM_INT_PARAM(mask,0,TR_MASK_FLOODFILL);
	Vector3f v, n;
	if(Utils::GetLocalAimPoint(v,&n,mask))
		a_thread->PushVector(n);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetNearestNonSolid(gmThread *a_thread)
{	
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_VECTOR_PARAM(sv,0);
	GM_CHECK_VECTOR_PARAM(ev,0);
	GM_INT_PARAM(mask,0,TR_MASK_FLOODFILL);
	Vector3f v;
	if(Utils::GetNearestNonSolid(v,Vector3f(sv.x,sv.y,sv.z),Vector3f(ev.x,ev.y,ev.z),mask))
		a_thread->PushVector(v);
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfGetLocalCommand(gmThread *a_thread)
{
	if(CommandReciever::m_ConsoleCommandThreadId == a_thread->GetId() 
		|| CommandReciever::m_MapDebugPrintThreadId == a_thread->GetId())
		a_thread->PushNewString(CommandReciever::m_ConsoleCommand.c_str());
	else
		a_thread->PushNull();
	return GM_OK;
}

static int GM_CDECL gmfReloadGoalScripts(gmThread *a_thread)
{	
	IGameManager::GetInstance()->GetGame()->ReloadGoalScripts();
	return GM_OK;
}

static int GM_CDECL gmfAllocGoalSerialNum(gmThread *a_thread)
{	
	a_thread->PushInt(GetMapGoalSerial());
	return GM_OK;
}

static int GM_CDECL gmfDynamicPathsUpdated(gmThread *a_thread)
{	
	int iTeamMask = 0;
	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(team,i);
		iTeamMask |= 1<<team;
	}

	Event_DynamicPathsChanged m(iTeamMask);
	IGameManager::GetInstance()->GetGame()->DispatchGlobalEvent(
		MessageHelper(MESSAGE_DYNAMIC_PATHS_CHANGED,&m,sizeof(m)));
	return GM_OK;
}

static int GM_CDECL gmfOnTriggerRegion(gmThread *a_thread)
{
	if(a_thread->ParamType(0)==gmAABB::GetType() &&
		a_thread->Param(1).GetTableObjectSafe())
	{
		GM_CHECK_GMBIND_PARAM(AABB*, gmAABB, aabb, 0);
		GM_CHECK_TABLE_PARAM(tbl,1);
		
		int serial = TriggerManager::GetInstance()->AddTrigger(*aabb,a_thread->GetMachine(),tbl);
		if(serial>0)
			a_thread->PushInt(serial);
		else
			a_thread->PushNull();
		return GM_OK;
	}
	else if(a_thread->Param(0).IsVector() && 
		a_thread->Param(1).IsNumber() &&
		a_thread->Param(2).GetTableObjectSafe())
	{
		GM_CHECK_VECTOR_PARAM(v,0);
		GM_CHECK_FLOAT_OR_INT_PARAM(rad,1);
		GM_CHECK_TABLE_PARAM(tbl,2);

		int serial = TriggerManager::GetInstance()->AddTrigger(Vector3f(v.x,v.y,v.z),rad,a_thread->GetMachine(),tbl);
		if(serial>0)
			a_thread->PushInt(serial);
		else
			a_thread->PushNull();
		return GM_OK;
	}
	GM_EXCEPTION_MSG("Expected (AABB,table) or (Vector3,#,table)");
	return GM_EXCEPTION;
}

static int GM_CDECL gmfDeleteTriggerRegion(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	if(a_thread->Param(0).IsInt())
		TriggerManager::GetInstance()->DeleteTrigger(a_thread->Param(0).GetInt());
	else if(a_thread->Param(0).GetCStringSafe(NULL))
		TriggerManager::GetInstance()->DeleteTrigger(a_thread->Param(0).GetCStringSafe());
	else
	{
		GM_EXCEPTION_MSG("Expected string(name) or int(serial#)");
		return GM_EXCEPTION;
	}
	return GM_OK;
}

static int GM_CDECL gmfSendTrigger(gmThread *a_thread)
{
	GM_CHECK_TABLE_PARAM(tbl,0);

	gmMachine *pM = a_thread->GetMachine();

	TriggerInfo ti;

	{
		gmVariable vEnt = tbl->Get(pM,"Entity");
		if(vEnt.IsEntity())
			ti.m_Entity.FromInt(vEnt.GetEntity());
	}
	{
		gmVariable vActivator = tbl->Get(pM,"Activator");
		if(vActivator.IsEntity())
			ti.m_Activator.FromInt(vActivator.GetEntity());
	}
	{
		gmVariable vTagName = tbl->Get(pM,"TagName");
		if(vTagName.GetCStringSafe(0))
			Utils::StringCopy(ti.m_TagName, vTagName.GetCStringSafe(0), TriggerBufferSize);
	}
	{
		gmVariable vAction = tbl->Get(pM,"Action");
		if(vAction.GetCStringSafe(0))
			Utils::StringCopy(ti.m_Action, vAction.GetCStringSafe(0), TriggerBufferSize);
	}

	if(ti.m_Action[0] && ti.m_TagName[0])
	{
		TriggerManager::GetInstance()->HandleTrigger(ti);
		return GM_OK;
	}

	GM_EXCEPTION_MSG("No TagName or Action defined!");
	return GM_OK;
}

static int GM_CDECL gmfConfigSet(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(3);
	GM_CHECK_STRING_PARAM(section,0);
	GM_CHECK_STRING_PARAM(key,1);
	gmVariable vValue = a_thread->Param(2,gmVariable::s_null);
	GM_INT_PARAM(overwrite,3,1);
	
	enum { BufferSize = 1024 };
	char buffer[BufferSize] = {};	
	Options::SetValue(section,key,String(vValue.AsString(a_thread->GetMachine(),buffer,BufferSize)),overwrite!=0);

	return GM_OK;
}

static int GM_CDECL gmfConfigGet(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(section,0);
	GM_CHECK_STRING_PARAM(key,1);

	enum { BufferSize = 1024 };
	char buffer[BufferSize] = {};
	gmVariable vValue = a_thread->Param(2,gmVariable::s_null);
	
	//////////////////////////////////////////////////////////////////////////

	String sValue;
	if(Options::GetValue(section,key,sValue))
	{
		int IntVal = 0;
		float FloatVal = 0.f;

		if(vValue.IsInt() && Utils::ConvertString(sValue,IntVal))
			a_thread->PushInt(IntVal);
		else if(vValue.IsFloat() && Utils::ConvertString(sValue,FloatVal))
			a_thread->PushFloat(FloatVal);
		else
			a_thread->PushNewString(sValue.c_str(),(int)sValue.length());
	}
	else if(!vValue.IsNull())
	{
		Options::SetValue(section,key,String(vValue.AsString(a_thread->GetMachine(),buffer,BufferSize)));

		a_thread->Push(a_thread->Param(2));
	}
	else
		a_thread->PushNull();

	return GM_OK;
}

static int GM_CDECL gmfCreateMapGoal(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(mgtype,0);
	gmGCRoot<gmUserObject> obj = g_MapGoalDatabase.CreateMapGoalType(mgtype);
	a_thread->PushUser(obj);
	return GM_OK;
}

static int GM_CDECL gmfEntityIsOutside(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	Vector3f v = Vector3f::ZERO;
	if(gameEnt.IsValid() && EngineFuncs::EntityPosition(gameEnt, v))
	{
	    a_thread->PushInt(InterfaceFuncs::IsOutSide(v));
	}
	return GM_OK;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// function: GetWeapon
//		This function will get the given <Weapon> object.
//
// Parameters:
//
//		<int> - The weapon Id to search for
//
// Returns:
//		<Weapon> - The weapon script object
//		- OR -
//		<null> - If weapon not found.
static int GM_CDECL gmfGetWeapon(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(weaponId, 0);

	WeaponPtr wp = g_WeaponDatabase.GetWeapon(weaponId);
	if(wp)
		a_thread->PushUser(wp->GetScriptObject(a_thread->GetMachine()));
	else
	{
		OBASSERT(0, va("No Weapon of Type: %d", weaponId));
		a_thread->PushNull();
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// package: Global Bot Library Functions
static gmFunctionEntry s_botLib[] = 
{
	{"EnableDebugWindow",		gmfEnableDebugWindow},
	{"DumpConsoleToFile",		gmfDumpDebugConsoleToFile},	

	{"EchoToScreen",			gmfEchoMessageToScreen},
	{"Error",					gmfEchoError},
	{"Log",						gmfLog},
	{"RunScript",				gmfRunScript},
	{"ExecScript",				gmfRunScript},
	{"AddBot",					gmfAddBot},	
	{"KickBot",					gmfKickBot},
	{"KickBotFromTeam",			gmfKickBotFromTeam},
	{"MoveBotToAnotherTeam",	gmfMoveBotToAnotherTeam},
	
	{"KickAll",					gmfKickAll},
	{"RegisterDefaultProfile",	gmfRegisterDefaultProfile},	
	{"OnTrigger",				gmfRegisterTriggerCallback},
	{"GetGoal",					gmfGetMapGoal},
	{"GetGoals",				gmfGetMapGoals},
	
	{"SetMapGoalProperties",	gmfSetMapGoalProperties},

	{"SetAvailableMapGoals",	gmfSetAvailableMapGoals},
	{"SetGoalPriority",			gmfSetGoalPriorityForTeamClass},
	{"SetGoalGroup",			gmfSetGoalGroup},	
	{"SetGoalRole",		gmfSetGoalRole},
	{"ClearGoalRole",		gmfClearGoalRole},

	{"CreateMapGoal",			gmfCreateMapGoal},

	{"GetGameEntityFromId",		gmfGetGameEntityFromId},
	{"GetGameIdFromEntity",		gmfGetGameIdFromEntity},
	{"TraceLine",				gmfTraceLine},
	{"GroundPoint",				gmfGroundPoint},	

	{"DrawDebugLine",			gmfDrawDebugLine},
	{"DrawDebugAABB",			gmfDrawDebugAABB},	
	{"DrawLine",				gmfDrawDebugLine},
	{"DrawArrow",				gmfDrawDebugArrow},	
	{"DrawAABB",				gmfDrawDebugAABB},
	{"DrawRadius",				gmfDrawDebugRadius},
	{"DrawText3d",				gmfDrawDebugText3d},
	{"DrawTrajectory",			gmfDrawTrajectory},	
	{"TransformAndDrawLineList",gmfTransformAndDrawLineList},	

	{"DrawEntityAABB",			gmfDrawEntityAABB},	
	{"DrawEntityOBB",			gmfDrawEntityOBB},	
	{"CheckEntityBoundsIntersect",gmfCheckEntityBoundsIntersect},
	
	{"GetMapName",				gmfGetMapName},
	{"GetMapExtents",			gmfGetMapExtents},

	{"GetPointContents",		gmfGetPointContents},

	{"GetGameState",			gmfGetGameState},
	{"GetGameTimeLeft",			gmfGetGameTimeLeft},	

	{"GetTime",					gmfGetTime},
	{"ExecCommand",				gmfExecCommand},

	{"EntityKill",				gmfEntityKill},

	{"OnTriggerRegion",			gmfOnTriggerRegion},
	{"DeleteTriggerRegion",		gmfDeleteTriggerRegion},

	{"SendTrigger",				gmfSendTrigger},	

	{"ConfigSet",				gmfConfigSet},
	{"ConfigGet",				gmfConfigGet},

	{"GetWeapon",				gmfGetWeapon},

	// Unified functions that should work on GameId and GameEntity.	
	{"GetEntityName",			gmfGetEntityName},
	{"GetEntName",				gmfGetEntityName},
	{"GetEntFacing",			gmfGetEntityFacing},
	{"GetEntPosition",			gmfGetEntityPosition},	
	{"GetEntBonePosition",		gmfGetEntityBonePosition},
	{"GetEntEyePosition",		gmfGetEntEyePosition},
	{"GetEntVelocity",			gmfGetEntityVelocity},
	{"GetEntRotationMatrix",	gmfGetEntRotationMatrix},
	{"GetEntFlags",				gmfGetEntityFlags},
	{"GetEntPowerups",			gmfGetEntityPowerups},
	{"GetEntHealthAndArmor",	gmfGetEntityHealthAndArmor},
	{"GetEntWorldAABB",			gmfGetEntityAABB},
	{"GetEntOwner",				gmfGetEntityOwner},
	{"GetEntTeam",				gmfGetEntityTeam},
	{"GetEntClass",				gmfGetEntityClass},
	{"GetEntCategory",			gmfGetEntCategory},
	{"GetEntEquippedWeapon",	gmfGetEntEquippedWeapon},
	{"EntityIsValid",			gmfEntityIsValid},
	{"EntityIsOutside",			gmfEntityIsOutside},

	{"GetClassNameFromId",		gmfGetClassNameFromId},	
	{"GetWeaponIdFromClassId",	gmfGetWeaponIdFromClassId},	

	{"DistanceBetween",			gmfDistanceBetween},
	{"CalcTrajectory",			gmfCalculateTrajectory},

	{"GetEntityLocalSpace",		gmfGetEntityToLocalSpace},	
	{"GetEntityWorldSpace",		gmfGetEntityToWorldSpace},

	{"GetEntityInSphere",		gmfGetEntityInSphere},
	{"GetEntityByName",			gmfGetEntityByName},	

	{"ExecCommandOnClient",		gmfExecCommandOnClient},
	
	{"GetEntityStat",			gmfGetEntityStat},
	{"GetTeamStat",				gmfGetTeamStat},

	{"GetGravity",				gmfGetGravity},
	{"CheatsEnabled",			gmfGetCheats},
	{"ServerCommand",			gmfServerCommand},	
	{"ServerScriptFunction",	gmfServerScriptFunction},		

	{"GetGameName",				gmfGetGameName},
	{"GetModName",				gmfGetModName},
	{"GetModVersion",			gmfGetModVersion},
	{"ShowPaths",				gmfShowPaths},

	{"GetLocalEntity",			gmfGetLocalEntity},
	{"GetLocalPosition",		gmfGetLocalPosition},
	{"GetLocalGroundPosition",	gmfGetLocalGroundPosition},
	{"GetLocalEyePosition",		gmfGetLocalEyePosition},
	{"GetLocalFacing",			gmfGetLocalFacing},
	{"GetLocalAABB",			gmfGetLocalAABB},
	{"GetLocalAimPosition",		gmfGetLocalAimPosition},
	{"GetLocalAimNormal",		gmfGetLocalAimNormal},
	{"GetNearestNonSolid",		gmfGetNearestNonSolid},
	{"GetLocalCommand",			gmfGetLocalCommand},

	{"ReloadGoalScripts",		gmfReloadGoalScripts},

	{"AllocGoalSerialNum",		gmfAllocGoalSerialNum},

	{"DynamicPathsUpdated",		gmfDynamicPathsUpdated},
};

//////////////////////////////////////////////////////////////////////////

void gmBindBotLib(gmMachine * a_machine)
{
	// Register the bot functions.
	a_machine->RegisterLibrary(s_botLib, sizeof(s_botLib) / sizeof(s_botLib[0]));
}
