#include "PrecompCommon.h"

#include "gmBotLibrary.h"
#include "gmBot.h"
#include "gmHelpers.h"
#include "gmGameEntity.h"
#include "gmTargetInfo.h"
#include "gmUtilityLib.h"
#include "BotBaseStates.h"

#include "BotBaseStates.h"

using namespace AiState;

#define CHECK_THIS_BOT() \
	Client *native = gmBot::GetThisObject( a_thread ); \
	if(!native) \
	{ \
		GM_EXCEPTION_MSG("Script Function on NULL object"); \
		return GM_EXCEPTION; \
	}

// script: Bot
//		Script bindings for <Client>

GMBIND_INIT_TYPE(gmBot, "Bot");

GMBIND_FUNCTION_MAP_BEGIN(gmBot)

	GMBIND_FUNCTION( "CanSnipe", gmfCanSnipe )
	GMBIND_FUNCTION( "ChangeTeam", gmfChangeTeam )
	GMBIND_FUNCTION( "ChangeClass", gmfChangeClass )
	GMBIND_FUNCTION( "ExecCommand", gmfExecCommand )
	GMBIND_FUNCTION( "FireWeapon", gmfFireWeapon )
	GMBIND_FUNCTION( "GetCurrentAmmo", gmfGetAmmo )
	GMBIND_FUNCTION( "GetClass", gmfGetClassId )	
	GMBIND_FUNCTION( "GetCurrentWeapon", gmfGetCurrentWeapon )		
	GMBIND_FUNCTION( "GetTeam", gmfGetTeam )	
	GMBIND_FUNCTION( "GetGameEntity", gmfGetGameEntity )
	GMBIND_FUNCTION( "GetGameId", gmfGetGameId )
	GMBIND_FUNCTION( "GetPosition", gmfGetPosition )
	GMBIND_FUNCTION( "GetEyePosition", gmfGetEyePosition )
	GMBIND_FUNCTION( "GetFacing", gmfGetFacing )
	GMBIND_FUNCTION( "GetSkills", gmfGetSkills )
	GMBIND_FUNCTION( "GetStat", gmfGetStat )
	GMBIND_FUNCTION( "GetVelocity", gmfGetVelocity )
	GMBIND_FUNCTION( "GetAllType", gmfGetAllType )
	GMBIND_FUNCTION( "GetAllAlly", gmfGetAllAlly )
	GMBIND_FUNCTION( "GetAllEnemy", gmfGetAllEnemy )
	GMBIND_FUNCTION( "GetNearest", gmfGetNearest)
	GMBIND_FUNCTION( "GetNearestAlly", gmfGetNearestAlly )
	GMBIND_FUNCTION( "GetNearestEnemy", gmfGetNearestEnemy )
	GMBIND_FUNCTION( "GetTarget", gmfGetTarget )
	GMBIND_FUNCTION( "ForceTarget", gmfForceTarget )
	GMBIND_FUNCTION( "GetLastTarget", gmfGetLastTarget )
	GMBIND_FUNCTION( "GetTargetInfo", gmfGetTargetInfo )
	GMBIND_FUNCTION( "IgnoreTargetForTime", gmfIgnoreTargetForTime )
	GMBIND_FUNCTION( "IgnoreTarget", gmfIgnoreTargetForTime /*gmfIgnoreTarget*/ )
	GMBIND_FUNCTION( "GetWeapon", gmfGetWeapon )
	GMBIND_FUNCTION( "GetHighLevelGoalName", gmfGetHighLevelGoalName )
	GMBIND_FUNCTION( "GetMapGoalName", gmfGetMapGoalName )
	GMBIND_FUNCTION( "SetRoles", gmfSetRoles )
	GMBIND_FUNCTION( "ClearRoles", gmfClearRoles )
	GMBIND_FUNCTION( "HasRole", gmfHasRole )

	GMBIND_FUNCTION( "GoGetAmmo", gmfSetGoal_GetAmmo )
	GMBIND_FUNCTION( "GoGetArmor", gmfSetGoal_GetArmor )
	GMBIND_FUNCTION( "GoGetHealth", gmfSetGoal_GetHealth )

	GMBIND_FUNCTION( "IsStuck", gmfIsStuck )
	GMBIND_FUNCTION( "ResetStuckTime", gmfResetStuckTime )

	GMBIND_FUNCTION( "GetMostDesiredAmmo", gmfGetMostDesiredAmmo )

	GMBIND_FUNCTION( "HasAmmo", gmfHasAmmo )
	GMBIND_FUNCTION( "HasWeapon", gmfHasWeapon )
	GMBIND_FUNCTION( "HasLineOfSightTo", gmfHasLineOfSightTo )

	GMBIND_FUNCTION( "GetBestWeapon", gmfGetBestWeapon )
	GMBIND_FUNCTION( "GetRandomWeapon", gmfGetRandomWeapon )

	GMBIND_FUNCTION( "HasPowerUp", gmfHasPowerUp )
	GMBIND_FUNCTION( "HasEntityFlag", gmfHasEntityFlagAll )
	GMBIND_FUNCTION( "HasAnyEntityFlag", gmfHasEntityFlagAny )
	GMBIND_FUNCTION( "HasAnyWeapon", gmfHasAnyWeapon )

	GMBIND_FUNCTION( "HasTarget", gmfHasTarget )

	GMBIND_FUNCTION( "InFieldOfView", gmfInFieldOfView )
	GMBIND_FUNCTION( "IsAllied", gmfGetIsAllied )

	GMBIND_FUNCTION( "MoveTowards", gmfSetMoveTo )
	GMBIND_FUNCTION( "PressButton", gmfPressButton )

	GMBIND_FUNCTION( "HoldButton", gmfHoldButton )
	GMBIND_FUNCTION( "ReleaseButton", gmfReleaseButton )
	
	GMBIND_FUNCTION( "ReloadProfile", gmfReloadProfile )
	GMBIND_FUNCTION( "Say", gmfSay )
	GMBIND_FUNCTION( "SayTeam", gmfSayTeam )
	GMBIND_FUNCTION( "SayVoice", gmfSayVoice )	
	GMBIND_FUNCTION( "SetDebugFlag", gmfSetDebugFlag )

	GMBIND_FUNCTION( "ToLocalSpace", gmfToLocalSpace )
	GMBIND_FUNCTION( "ToWorldSpace", gmfToWorldSpace )
	GMBIND_FUNCTION( "DistanceTo", gmfDistanceTo )
	GMBIND_FUNCTION( "GetNearestDestination", gmfGetNearestDestination )

	GMBIND_FUNCTION( "DumpBotTable", gmfDumpBotTable )	

	GMBIND_FUNCTION( "Enable", gmfEnable )
	GMBIND_FUNCTION( "EnableShooting", gmfSetEnableShooting )
	GMBIND_FUNCTION( "IsWeaponCharged", gmfIsWeaponCharged )

	GMBIND_FUNCTION( "GetHealthPercent", gmfGetHealthPercent )

	GMBIND_FUNCTION( "AddScriptGoal", gmfAddScriptGoal )
	GMBIND_FUNCTION( "FindState", gmfFindState )
	GMBIND_FUNCTION( "RemoveState", gmfRemoveState )
	GMBIND_FUNCTION( "SetStateEnabled", gmfSetStateEnabled )	

	GMBIND_FUNCTION( "PlaySound", gmfPlaySound )
	GMBIND_FUNCTION( "StopSound", gmfStopSound )

	GMBIND_FUNCTION( "ScriptEvent", gmfScriptEvent )
	GMBIND_FUNCTION( "ScriptMessage", gmfScriptMessage )	

	GMBIND_FUNCTION( "IsCarryingFlag", gmfIsCarryingFlag )
	GMBIND_FUNCTION( "CanGrabItem", gmfCanGrabItem )

GMBIND_FUNCTION_MAP_END()

// property: Field Of View
//		<Field Of View> is the angle(in degrees) that the bot can 'see' in front of them.

// property: Max View Distance
//		<Max View Distance> is the maximum distance(in game units) the bot is capable of seeing something.
//		This could be tweaked lower for maps with fog or for a distance more comparable to human view distances.

// property: Reaction Time
//		<Reaction Time> is the time delay(in seconds) from when a bot first see's a target, to when
//		the bot will begin to react and target them.

// property: Memory Span
//		<Memory Span> is how long it takes(in seconds) for a bot to consider his memory of someone or something
//		'out of date' and not considered for targeting and such.

// property: Aim Persistance
//		<Aim Persistance> is how long the bot will aim in the direction of a target after the target has gone out of view.
//		This is useful for keeping the bot aiming toward the target in the event of brief obstructions of their view.
//		Expressed in seconds.

// property: Max Turn Speed
//		<Max Turn Speed> is the max speed/rotational velocity. Expressed in degrees / second.

// property: Aim Stiffness
//		Property of the Aiming calculations. Stiffness of the spring/damper model.

// property: Aim Damping
//		Property of the Aiming calculations. Damping of the spring/damper model.

// property: Aim Tolerance
//		Property of the aiming algorithm. This determines the allowable angle tolerance to the target facing
//		to be considered 'close enough' for firing weapons and such. Expressed in degrees.

GMBIND_PROPERTY_MAP_BEGIN(gmBot)
	// var: Name
	//		string - The bots current name. READ ONLY
	GMBIND_PROPERTY( "Name", getName, setName )
	// var: MemorySpan
	//		float - The bots current <Memory Span>, in seconds.
	GMBIND_PROPERTY( "MemorySpan", getMemorySpan, setMemorySpan )
	// var: AimPersistance
	//		float - The bots current <Aim Persistance>
	GMBIND_PROPERTY( "AimPersistance", getAimPersistance, setAimPersistance )
	// var: ReactionTime
	//		float - The bots current <Reaction Time>
	GMBIND_PROPERTY( "ReactionTime", getReactionTime, setReactionTime )	
	//////////////////////////////////////////////////////////////////////////
	// var: Team
	//		int - The bots current <Team>
	GMBIND_AUTOPROPERTY( "Team", GM_INT, m_Team, 0 )
	// var: FieldOfView
	//		float - The bots current <Field Of View>, in degrees.
	GMBIND_AUTOPROPERTY( "FieldOfView", GM_FLOAT, m_FieldOfView, 0 )
	// var: MaxTurnSpeed
	//		float - The bots <Max Turn Speed>. 
	GMBIND_AUTOPROPERTY( "MaxTurnSpeed", GM_FLOAT, m_MaxTurnSpeed, 0 )
	// var: AimStiffness
	//		float - The bots <Aim Stiffness>.
	GMBIND_AUTOPROPERTY( "AimStiffness", GM_FLOAT, m_AimStiffness, 0 )
	// var: AimDamping
	//		float - The bots <Aim Damping>.
	GMBIND_AUTOPROPERTY( "AimDamping", GM_FLOAT, m_AimDamping, 0 )
	// var: AimTolerance
	//		float - The bots <Aim Tolerance>.
	GMBIND_AUTOPROPERTY( "AimTolerance", GM_FLOAT, m_AimTolerance, 0 )
	// var: MaxViewDistance
	//		float - The bots current <Max View Distance>, in game units
	GMBIND_AUTOPROPERTY( "MaxViewDistance", GM_FLOAT, m_MaxViewDistance, 0 )
	// var: Health
	//		float - The bots current <Health> READ ONLY
	GMBIND_AUTOPROPERTY( "Health", GM_INT, m_HealthArmor.m_CurrentHealth, gmBot::AUTO_PROP_READONLY )
	// var: MaxHealth
	//		float - The bots current <MaxHealth> READ ONLY
	GMBIND_AUTOPROPERTY( "MaxHealth", GM_INT, m_HealthArmor.m_MaxHealth, gmBot::AUTO_PROP_READONLY )
	// var: Armor
	//		float - The bots current <Armor> READ ONLY
	GMBIND_AUTOPROPERTY( "Armor", GM_INT, m_HealthArmor.m_CurrentArmor, gmBot::AUTO_PROP_READONLY )
	// var: MaxArmor
	//		float - The bots current <MaxArmor> READ ONLY
	GMBIND_AUTOPROPERTY( "MaxArmor", GM_INT, m_HealthArmor.m_MaxArmor, gmBot::AUTO_PROP_READONLY )
GMBIND_PROPERTY_MAP_END();

// ctr/dtr

Client *gmBot::Constructor(gmThread *a_thread) 
{ 
	return NULL;
}

void gmBot::Destructor(Client *_native)
{
}

void gmBot::DebugInfo(gmUserObject *a_object, gmMachine *a_machine, gmChildInfoCallback a_infoCallback)
{
	Client *pNative = gmBot::GetNative(a_object);
	if(pNative)
	{
		a_infoCallback("Name", pNative->GetName(), a_machine->GetTypeName(GM_STRING), 0);
	}	
}

// function: GetGameEntity
//		This function gets the <GameEntity> for this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		<GameEntity> - The for this specific bot. It is usually not safe to 
//		keep this value for long. It could quickly become totally invalid, 
//		or reference a completely different entity.
int gmBot::gmfGetGameEntity(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	gmVariable v;
	v.SetEntity(native->GetGameEntity().AsInt());
	a_thread->Push(v);
	return GM_OK;
}

// function: GetGameId
//		This function gets the <GameId> for this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		<GameId> - This bots current <GameId>
int gmBot::gmfGetGameId(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	a_thread->PushInt(native->GetGameID());
	return GM_OK;
}

// function: GetPosition
//		This function gets <Vector3> position of this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		<Vector3> - Current Position
int gmBot::gmfGetPosition(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	const Vector3f &v = native->GetPosition();
	a_thread->PushVector(v.x, v.y, v.z);
	return GM_OK;
}

// function: GetEyePosition
//		This function gets <Vector3> eye position of this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		<Vector3> - Current Position
int gmBot::gmfGetEyePosition(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	const Vector3f &v = native->GetEyePosition();
	a_thread->PushVector(v.x, v.y, v.z);
	return GM_OK;
}

// function: GetFacing
//		This function gets <Vector3> facing of this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		<Vector3> - Current Position
int gmBot::gmfGetFacing(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	const Vector3f &v = native->GetFacingVector();
	a_thread->PushVector(v.x, v.y, v.z);
	return GM_OK;
}


// function: GetVelocity
//		This function gets the <Vector3> velocity for this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		<Vector3> - Current Velocity
int gmBot::gmfGetVelocity(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	const Vector3f &v = native->GetVelocity();
	a_thread->PushVector(v.x, v.y, v.z);
	return GM_OK;
}

// function: IsStuck
//		This function checks if the bot is stuck.
//
// Parameters:
//
//		float/int - stuck time threshold to check, default 0.5 seconds
//
// Returns:
//		<int> - true if stuck, false if not
int gmBot::gmfIsStuck(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_FLOAT_OR_INT_PARAM(stuckTime, 0, 0.5f);

	bool Stuck = false;

	using namespace AiState;
	FINDSTATE(fp,FollowPath,native->GetStateRoot());
	if(fp != NULL && fp->IsActive())
	{
		int iStuckTime = Utils::SecondsToMilliseconds(stuckTime);
		Stuck = native->GetStuckTime() >= iStuckTime;
	}
	a_thread->PushInt(Stuck?1:0);
	return GM_OK;
}

// function: ResetStuckTime
//		This function resets the running stuck timer.
//
// Parameters:
//
//		none
//
// Returns:
//		none
int gmBot::gmfResetStuckTime(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	native->ResetStuckTime();
	return GM_OK;
}

// function: GetTeam
//		This function gets the current team # for this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		int - Current Team #
int gmBot::gmfGetTeam(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	a_thread->PushInt(native->GetTeam());
	return GM_OK;
}

// function: GetClassId
//		This function gets the class Id for this bot.
//
// Parameters:
//
//		None
//
// Returns:
//		int - This bots class Id
int gmBot::gmfGetClassId(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	a_thread->PushInt(native->GetClass());
	return GM_OK;
}

// function: ExecCommand
//		This function executes a command for the bot. Usually equivilent to console commands.
//
// Parameters:
//
//		string - string to execute for this bot
//
// Returns:
//		None
int gmBot::gmfExecCommand(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(msg, 0);

	g_EngineFuncs->BotCommand(native->GetGameID(), msg);
	return GM_OK;
}

// function: SayVoice
//		This function executes a voice macro for the bot, if supported by the current mod.
//		Voice macro id's should be exposed through the global VOICE table
//
// Parameters:
//
//		int - numeric Id for a voice macro. Use values from the global VOICE table
//
// Returns:
//		None
int gmBot::gmfSayVoice(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(voiceId, 0);

	native->SendVoiceMacro(voiceId);
	return GM_OK;
}

// function: Say
//		This function executes a chat message for this bot.
//
// Parameters:
//
//		string - the string of what you want the bot to <Say>
//
// Returns:
//		None
int gmBot::gmfSay(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	if(a_thread->GetNumParams() > 0)
	{
		const int bufferSize = 512;
		char buffer[bufferSize];

		int iMsgPos = 0;
		const int chatMsgSize = 2048;
		char chatMsg[chatMsgSize] = {0};

		// build the string
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			const char *pAsString = a_thread->Param(i).AsString(a_thread->GetMachine(), buffer, bufferSize);
			if(pAsString)
			{
				int len = (int)strlen(pAsString);
				if(chatMsgSize - iMsgPos > len)
				{
					Utils::StringCopy(&chatMsg[iMsgPos], pAsString, len);
					iMsgPos += len;
				}
			}
		}

		g_EngineFuncs->BotCommand(native->GetGameID(), va("say \"%s\"", chatMsg));
		return GM_OK;
	}

	GM_EXCEPTION_MSG("Expected 1+ parameters");
	return GM_EXCEPTION;
}

// function: SayTeam
//		This function executes a chat message for this bot, but only to the bots current team.
//
// Parameters:
//
//		string - the string of what you want the bot to <SayTeam>
//
// Returns:
//		None
int gmBot::gmfSayTeam(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	if(a_thread->GetNumParams() > 0)
	{
		const int bufferSize = 512;
		char buffer[bufferSize];

		int iMsgPos = 0;
		const int chatMsgSize = 2048;
		char chatMsg[chatMsgSize] = {0};

		// build the string
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			const char *pAsString = a_thread->Param(i).AsString(a_thread->GetMachine(), buffer, bufferSize);
			if(pAsString)
			{
				int len = (int)strlen(pAsString);
				if(chatMsgSize - iMsgPos > len)
				{
					Utils::StringCopy(&chatMsg[iMsgPos], pAsString, len);
					iMsgPos += len;
				}
			}
		}

		g_EngineFuncs->BotCommand(native->GetGameID(), va("say_team \"%s\"", chatMsg));
		return GM_OK;
	}

	GM_EXCEPTION_MSG("Expected 1+ parameters");
	return GM_EXCEPTION;
}

int gmBot::gmfSetGoal_GetAmmo(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	/*GM_CHECK_INT_PARAM(ammotype, 0);
	GM_FLOAT_PARAM(camptime, 0, 3.0f);

	if(native->GetBrain())
	{
		native->GetBrain()->ResetSubgoals("Script GetAmmo");
		Goal_GetAmmo::GoalInfo gi;
		gi.m_MaxCampTime = Utils::SecondsToMilliseconds(camptime);

		GoalPtr g(new Goal_GetAmmo(native, ammotype, gi));
		g->SignalStatus(true);
		native->GetBrain()->InsertSubgoal(g);
	}*/
	return GM_OK;
}

int gmBot::gmfSetGoal_GetArmor(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	/*GM_FLOAT_PARAM(camptime, 0, 3.0f);

	if(native->GetBrain())
	{
		native->GetBrain()->ResetSubgoals("Script GetArmor");
		Goal_GetArmor::GoalInfo gi;
		gi.m_MaxCampTime = Utils::SecondsToMilliseconds(camptime);

		GoalPtr g(new Goal_GetArmor(native, gi));
		g->SignalStatus(true);
		native->GetBrain()->InsertSubgoal(g);
	}*/
	return GM_OK;
}

int gmBot::gmfSetGoal_GetHealth(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	/*GM_FLOAT_PARAM(camptime, 0, 3.0f);

	if(native->GetBrain())
	{
		native->GetBrain()->ResetSubgoals("Script GetHealth");
		Goal_GetHealth::GoalInfo gi;
		gi.m_MaxCampTime = Utils::SecondsToMilliseconds(camptime);

		GoalPtr g(new Goal_GetHealth(native, GameEntity(), gi));
		g->SignalStatus(true);
		native->GetBrain()->InsertSubgoal(g);
	}*/
	return GM_OK;
}

// function: GetNearest
//		This function will get the nearest known ally or enemy that matches a category and class Id
//
// Parameters:
//
//		int - category of entities to search for, from the global CAT table
//		int - OPTIONAL - the specific class Id to search for, pass 0 to search for any class
//
// Returns:
//		<GameEntity> - The enemy best matching the request
//		- OR -
//		null - If no entity matches the request
static int gmfGetNearestAllyOrEnemy(gmThread *a_thread, SensoryMemory::Type _type)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(catID, 0);

	FilterClosest filter(native, _type);
	filter.AddCategory(catID);

	if (a_thread->GetNumParams() > 1)
	{
		switch (a_thread->ParamType(1))
		{
		case GM_INT:
		{
			filter.AddClass(a_thread->ParamInt(1));
			break;
		}
		case GM_TABLE:
		{
			gmTableObject *table = a_thread->ParamTable(1);
			gmTableIterator tIt;
			for (gmTableNode *pNode = table->GetFirst(tIt); pNode; pNode = table->GetNext(tIt))
			{
				if (pNode->m_value.m_type != GM_INT)
				{
					GM_EXCEPTION_MSG("expecting param 2 as table of int, got %s", a_thread->GetMachine()->GetTypeName(pNode->m_value.m_type));
					return GM_EXCEPTION;
				}
				filter.AddClass(pNode->m_value.GetInt());
			}
			break;
		}
		default:
			GM_EXCEPTION_MSG("expecting param 2 as int or table, got %s", a_thread->ParamTypeName(1));
			return GM_EXCEPTION;
		}
	}

	native->GetSensoryMemory()->QueryMemory(filter);

	GameEntity ge = filter.GetBestEntity();
	if(ge.IsValid())
	{
		gmVariable v;
		v.SetEntity(ge.AsInt());
		a_thread->Push(v);
	}
	else
		a_thread->PushNull();
	return GM_OK;
}

int gmBot::gmfGetNearest(gmThread *a_thread)
{
	return gmfGetNearestAllyOrEnemy(a_thread, SensoryMemory::EntAny);
}

int gmBot::gmfGetNearestEnemy(gmThread *a_thread)
{
	return gmfGetNearestAllyOrEnemy(a_thread, SensoryMemory::EntEnemy);
}

int gmBot::gmfGetNearestAlly(gmThread *a_thread)
{
	return gmfGetNearestAllyOrEnemy(a_thread, SensoryMemory::EntAlly);
}


// function: GetAllType
//		This function will get all entities that matche a category and class Id
//
// Parameters:
//
//		int - category of entities to search for, from the global CAT table
//		int - the specific class Id to search for, pass CLASS.ANYPLAYER to search for any player
//		table - where to put the results
//
// Returns:
//		int - count
static int gmfGetAllAllyOrEnemy(gmThread *a_thread, SensoryMemory::Type _type)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(3);
	GM_CHECK_INT_PARAM(catID, 0);
	GM_CHECK_INT_PARAM(classID, 1);
	GM_CHECK_TABLE_PARAM(table, 2);

	AiState::SensoryMemory *sensory = native->GetSensoryMemory();

	MemoryRecords list;
	list.reserve(16);

	FilterAllType filter(native, _type, list);
	filter.AddClass(classID);
	filter.AddCategory(catID);
	sensory->QueryMemory(filter);
	gmMachine *pMachine = a_thread->GetMachine();

	DisableGCInScope gcEn(pMachine);
	table->RemoveAndDeleteAll(pMachine);
	for (obuint32 i = 0; i < list.size(); ++i)
	{
		const MemoryRecord *rec = sensory->GetMemoryRecord(list[i]);

		gmVariable v;
		v.SetEntity(rec->GetEntity().AsInt());
		table->Set(pMachine, i, v);
	}
	a_thread->PushInt((gmint)list.size());
	return GM_OK;
}

int gmBot::gmfGetAllType(gmThread *a_thread)
{
	return gmfGetAllAllyOrEnemy(a_thread, SensoryMemory::EntAny);
}

int gmBot::gmfGetAllEnemy(gmThread *a_thread)
{
	return gmfGetAllAllyOrEnemy(a_thread, SensoryMemory::EntEnemy);
}

int gmBot::gmfGetAllAlly(gmThread *a_thread)
{
	return gmfGetAllAllyOrEnemy(a_thread, SensoryMemory::EntAlly);
}

// function: GetTarget
//		This function will get the bots current target.
//
// Parameters:
//
//		None
//
// Returns:
//		<GameEntity> - The current target entity
//		- OR - 
//		null - If the bot has no target
int gmBot::gmfGetTarget(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GameEntity geTarget = native->GetTargetingSystem()->GetCurrentTarget();
	if(geTarget.IsValid())
	{
		gmVariable v;
		v.SetEntity(geTarget.AsInt());
		a_thread->Push(v);
	}
	else
		a_thread->PushNull();
	return GM_OK;
}

// function: GetLastTarget
//		This function will get the bots last target
//
// Parameters:
//
//		None
//
// Returns:
//		<GameEntity> - The current target entity
//		- OR - 
//		null - If the bot has no target
int gmBot::gmfGetLastTarget(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GameEntity geTarget = native->GetTargetingSystem()->GetLastTarget();
	if(geTarget.IsValid())
	{
		gmVariable v;
		v.SetEntity(geTarget.AsInt());
		a_thread->Push(v);
	}
	else
		a_thread->PushNull();
	return GM_OK;
}

// function: ForceTarget
//		This function will set the bots target, overriding internal target selection
//
// Parameters:
//
//		None
//
// Returns:
//		<GameEntity> - The current target entity
//		- OR - 
//		null - If the bot has no target
int gmBot::gmfForceTarget(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	if(gameEnt.IsValid())
		native->GetTargetingSystem()->ForceTarget(gameEnt);
	return GM_OK;
}

// function: GetTargetInfo
//		This function will get the <TargetInfo> for the current target.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//
// Returns:
//		<TargetInfo> - The current target entity
//		- OR - 
//		null - If the bot has no target
int gmBot::gmfGetTargetInfo(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);	
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");

	const TargetInfo *pTargetInfo = native->GetSensoryMemory()->GetTargetInfo(gameEnt);
	if(pTargetInfo)
		a_thread->PushUser(pTargetInfo->GetScriptObject(a_thread->GetMachine()));
	else
		a_thread->PushNull();
	return GM_OK;
}

// function: IgnoreTargetForTime
//		This function causes the bot to ignore a specific entity for targeting for some duration of time.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		float/int - The number of seconds to ignore target
//
// Returns:
//		none
int gmBot::gmfIgnoreTargetForTime(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(2);
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");
	GM_CHECK_FLOAT_OR_INT_PARAM(ignoreTime, 1);
	
	MemoryRecord *pRecord = native->GetSensoryMemory()->GetMemoryRecord(gameEnt, true);
	if(pRecord)	
		pRecord->IgnoreAsTargetForTime(Utils::SecondsToMilliseconds(ignoreTime));
	return GM_OK;
}

// function: IgnoreTarget
//		This function causes the bot to ignore a specific entity for targeting for some duration of time.
//
// Parameters:
//
//		<GameEntity> - The entity to use
//		- OR - 
//		<int> - The gameId for the entity to use
//		<int> - - OPTIONAL - true to ignore, false to disable ignore. default true.
//
// Returns:
//		none
int gmBot::gmfIgnoreTarget(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);	
	OBASSERT(gameEnt.IsValid(), "Bad Entity");
	GM_INT_PARAM(ignoreTarget, 1, 1);

	MemoryRecord *pRecord = native->GetSensoryMemory()->GetMemoryRecord(gameEnt, true);
	if(pRecord)	
		pRecord->IgnoreAsTarget(ignoreTarget != 0);
	return GM_OK;
}

// function: GetWeapon
//		This function will get the <Weapon> for the current target.
//
// Parameters:
//
//		<int> - The weapon Id to search for
//
// Returns:
//		<Weapon> - The weapon script object
//		- OR -
//		<null> - If weapon not found.
int gmBot::gmfGetWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(weaponId, 0);

	WeaponPtr wp = native->GetWeaponSystem()->GetWeapon(weaponId, false);
	if(wp)
		a_thread->PushUser(wp->GetScriptObject(a_thread->GetMachine()));
	else
	{
		OBASSERT(weaponId==0, va("No Weapon of Type: %d", weaponId));
		a_thread->PushNull();
	}
	return GM_OK;
}

int gmBot::gmfAddScriptGoal(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(name,0);
	
	bool bSuccess = native->AddScriptGoal(name);
	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

int gmBot::gmfFindState(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(name,0);
	bool bSuccess = native->GetStateRoot()->FindState(name)!=0;
	a_thread->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

int gmBot::gmfRemoveState(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(name,0);
	delete native->GetStateRoot()->RemoveState(name);
	return GM_OK;
}

int gmBot::gmfSetStateEnabled(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(name,0);
	GM_CHECK_INT_PARAM(en,1)
	State *pState = native->GetStateRoot()->FindState(name);
	if(pState)
		pState->SetUserDisabled(en==0);
	else
	{
		GM_EXCEPTION_MSG("State: %s not found.", name);
		return GM_EXCEPTION;
	}
	return GM_OK;
}

int gmBot::gmfPlaySound(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(name,0);
	InterfaceFuncs::PlaySound(native, name);
	return GM_OK;
}

int gmBot::gmfStopSound(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(name,0);
	InterfaceFuncs::StopSound(native, name);
	return GM_OK;
}

int gmBot::gmfGetHealthPercent(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	a_thread->PushFloat(native->GetHealthPercent());
	return GM_OK;
}

int gmBot::gmfScriptEvent(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(name,0);
	GM_STRING_PARAM(d1,1,"");
	GM_STRING_PARAM(d2,2,"");
	GM_STRING_PARAM(d3,3,"");

	Event_ScriptMessage m;
	Utils::StringCopy(m.m_MessageName,name,sizeof(m.m_MessageName));
	Utils::StringCopy(m.m_MessageData1,d1,sizeof(m.m_MessageData1));
	Utils::StringCopy(m.m_MessageData2,d2,sizeof(m.m_MessageData2));
	Utils::StringCopy(m.m_MessageData3,d3,sizeof(m.m_MessageData3));
	native->SendEvent(MessageHelper(MESSAGE_SCRIPTMSG,&m,sizeof(m)));
	return GM_OK;
}

int gmBot::gmfScriptMessage(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	//GM_CHECK_STRING_PARAM(name,0);
	
	return GM_OK;
}

// function: IsAllied
//		This function gets whether an entity is allied with this bot.
//
// Parameters:
//
//		<GameEntity> - entity to check ally status for
//		- OR -
//		int - gameId
//
// Returns:
//		bool - true if allied, false if not
int gmBot::gmfGetIsAllied(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	GameEntity gameEnt;
	GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);

	if(!gameEnt.IsValid())
	{
		GM_EXCEPTION_MSG("Invalid GameId or GameEntity");
		return GM_EXCEPTION;
	}

	a_thread->PushInt(native->IsAllied(gameEnt) ? 1 : 0);	
	return GM_OK;
}

// function: PressButton
//		This function sets makes the bot 'press' a button.
//
// Parameters:
//
//		int - button to press. Pass any number of buttons to the function as seperate parameters.
//
// Returns:
//		None
int gmBot::gmfPressButton(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	for(int i = 0; i < GM_THREAD_ARG->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(btn, i);
		native->PressButton(btn);
	}

	return GM_OK;
}

// function: HoldButton
//		This function sets makes the bot 'press' a button, and hold it for an amount of time.
//
// Parameters:
//
//		int - button to press. Pass any number of buttons to the function as seperate parameters.
//		int/float - seconds to hold the button down, must be last parameter.
//
// Returns:
//		None
int gmBot::gmfHoldButton(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_FLOAT_OR_INT_PARAM(time, a_thread->GetNumParams()-1);

	BitFlag64 buttons;
	for(int i = 0; i < a_thread->GetNumParams()-1; ++i)
	{
		GM_CHECK_INT_PARAM(btn, i);
		buttons.SetFlag(btn);
	}
	native->HoldButton(buttons, Utils::SecondsToMilliseconds(time));
	return GM_OK;
}

// function: ReleaseButton
//		Releases a button currently being held by <HoldButton>
//
// Parameters:
//
//		int - buttons to release. Pass any number of buttons to the function as seperate parameters.
//
// Returns:
//		None
int gmBot::gmfReleaseButton(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	BitFlag64 buttons;
	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(btn, i);
		buttons.SetFlag(btn);
	}
	native->ReleaseHeldButton(buttons);
	return GM_OK;
}

// function: MoveTowards
//		This function sets the bots current movement goal to a specific location. Returns true if
//
// Parameters:
//
//		<Vector3> - The 3d location to move towards. Unlike <GoTo>, this function doesn't
//				plan a path to the destination. This function simply makes the bot run toward
//				the location.
//		<float> - OPTIONAL - Distance tolerance to check, defaults to 32
//
// Returns:
//		None
int gmBot::gmfSetMoveTo(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	GM_FLOAT_OR_INT_PARAM(tolerance, 1, 32.f);
	GM_INT_PARAM(m,2,Run);
	MoveMode mm = m==Walk?Walk:Run;
	GM_THREAD_ARG->PushInt(native->MoveTo(Vector3f(v.x,v.y,v.z), tolerance, mm) ? 1 : 0);
	return GM_OK;
}

// function: ToLocalSpace
//		This function converts a world position to this bots local space.
//
// Parameters:
//
//		<Vector3> - The 3d vector to convert to local space.
//
// Returns:
//		<Vector3> - The local space coordinate
int gmBot::gmfToLocalSpace(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);

	Vector3f vl = native->ToLocalSpace(Vector3f(v.x, v.y, v.z));
	a_thread->PushVector(vl.x,vl.y,vl.z);
	return GM_OK;
}

// function: ToWorldSpace
//		This function converts a local position from this bots to world space.
//
// Parameters:
//
//		<Vector3> - The 3d vector to convert to local space.
//
// Returns:
//		<Vector3> - The local space coordinate
int gmBot::gmfToWorldSpace(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	
	Vector3f vw = native->ToWorldSpace(Vector3f(v.x, v.y, v.z));
	a_thread->PushVector(vw.x,vw.y,vw.z);
	return GM_OK;
}

// function: DistanceTo
//		Calculates the distance between the bot and another <Vector3> position..
//
// Parameters:
//
//		<Vector3> - The 3d vector to convert to calculate distance to.
//		 - OR -
//		<GameEntity> - The entity to calculate distance to.
//		- OR - 
//		<MapGoal> - The goal to calculate distance to.
//		- OR - 
//		<int> - The gameId to calculate distance to.
//		<int> - OPTIONAL - true to use eye position, false to use entity position(default false)
//
// Returns:
//		None
int gmBot::gmfDistanceTo(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_INT_PARAM(eyepos, 1, False);

	Vector3f vPosition2;
	MapGoal *Mg = 0;
	if(a_thread->ParamType(0) == GM_VEC3)
	{
		GM_CHECK_VECTOR_PARAM(v,0);
		vPosition2 = Vector3f(v.x,v.y,v.z);
	}
	else if(gmBind2::Class<MapGoal>::FromVar(a_thread, a_thread->Param(0), Mg) && Mg)
	{
		vPosition2 = Mg->GetPosition();
	}
	else
	{
		GameEntity gameEnt;
		GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
		if(!EngineFuncs::EntityPosition(gameEnt, vPosition2))
		{
			GM_EXCEPTION_MSG("Invalid Entity Provided!"); 
			return GM_EXCEPTION;
		}
	}
	Vector3f vPos = eyepos != False ? native->GetPosition() : native->GetEyePosition();
	a_thread->PushFloat((vPos - vPosition2).Length());
	return GM_OK;
}

// function: GetNearestDestination
//		Finds the nearest position from table of positions
//
// Parameters:
//
//		table of vectors
//
// Returns:
//		index of the nearest destination, or null if path not found

int gmBot::gmfGetNearestDestination(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_TABLE_PARAM(table, 0);

	DestinationVector list;
	list.reserve(table->Count());
	gmTableIterator tIt;
	gmTableNode *pNode;
	for (pNode = table->GetFirst(tIt); pNode; pNode = table->GetNext(tIt))
	{
		if (pNode->m_value.m_type != GM_VEC3)
		{
			GM_EXCEPTION_MSG("expecting param 1 as table of vectors, got %s", a_thread->GetMachine()->GetTypeName(pNode->m_value.m_type));
			return GM_EXCEPTION;
		}
		Vector3f v;
		pNode->m_value.GetVector(v.x, v.y, v.z);
		list.push_back(Destination(v, 0));
	}

	PathPlannerBase *pPathPlanner = IGameManager::GetInstance()->GetNavSystem();
	int index = pPathPlanner->PlanPathToNearest(native, native->GetPosition(), list, native->GetTeamFlag());
	if (!pPathPlanner->FoundGoal()) a_thread->PushNull();
	else
	{
		for (pNode = table->GetFirst(tIt); index > 0; pNode = table->GetNext(tIt)) index--;
		a_thread->Push(pNode->m_key);
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// Weapon System Functions

// function: ClearWeapons
//		This function clears all the weapons the bot has. In this case, clearing
//		means that the bot will remove all weapon evaluators from his weapon system.
//		It does not mean that the weapons will actually be removed from the bots game inventory
//
// Parameters:
//
//		None
//
// Returns:
//		None
int gmBot::gmfClearWeapons(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	native->GetWeaponSystem()->ClearWeapons();
	return GM_OK;
}

// function: FireWeapon
//		This function tells the bot to fire his current weapon. An alternative method of
//		firing the current weapon is <PressButton>
//
// Parameters:
//
//		None
//
// Returns:
//		None
int gmBot::gmfFireWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	native->GetWeaponSystem()->FireWeapon(Primary);
	return GM_OK;
}

// function: GetCurrentWeapon
//		This function gets the current weapon the bot is using. Since <SelectWeapon> and <SelectBestWeapon>
//		don't happen instantaneously, this function is useful for determining what weapon the bot actually
//		has equipped currently.
//
// Parameters:
//
//		None
//
// Returns:
//		int - The weapon Id of the current weapon.
int gmBot::gmfGetCurrentWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	a_thread->PushInt(native->GetWeaponSystem()->GetCurrentWeaponID());
	return GM_OK;
}


// function: GetBestWeapon
//		This function gets the weaponId of the best default weapon, or best weapon versus a target if provided.
//
// Parameters:
//
//		None
//
// Returns:
//		int - The weapon Id of the current weapon.
int gmBot::gmfGetBestWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GameEntity ent;
	GM_GAMEENTITY_FROM_PARAM(ent, 0, GameEntity())

	a_thread->PushInt(native->GetWeaponSystem()->SelectBestWeapon(ent));
	return GM_OK;
}
int gmBot::gmfGetRandomWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	const int wpnId = native->GetWeaponSystem()->SelectRandomWeapon();
	if(wpnId)
		a_thread->PushInt(wpnId);
	else
		a_thread->PushNull();
	return GM_OK;
}

// function: GetWeaponAmmo
//		This function gets the current weapon ammo for either the current weapon or
//		a specific weapon, depending on whether a parameter is passed.
//
// Parameters:
//
//		table - table to fill in with ammo info
//
// Returns:
//		table - Ammo table. Contains CurrentAmmo, MaxAmmo, CurrentClips, MaxClips
//		- OR - 
//		null - If the bot doesn't have a weapon or the requested weapon.
int gmBot::gmfGetAmmo(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_TABLE_PARAM(ammotbl,0);
	
	WeaponPtr curWeapon;
	FireMode m = InvalidFireMode;
	switch(a_thread->GetNumParams()) 
	{
	case 1:
		{
			m = Primary;
			curWeapon = native->GetWeaponSystem()->GetCurrentWeapon();
			break;
		}
	case 2:
		{
			GM_CHECK_INT_PARAM(firemode, 1);
			m = Weapon::GetFireMode(firemode);
			curWeapon = native->GetWeaponSystem()->GetCurrentWeapon();
			break;
		}
	case 3:
		{
			GM_CHECK_INT_PARAM(firemode, 1);
			GM_CHECK_INT_PARAM(weaponId, 2);
			m = Weapon::GetFireMode(firemode);
			curWeapon = native->GetWeaponSystem()->GetWeapon(weaponId);
			break;
		}
	default:
		GM_EXCEPTION_MSG("Expected 0-2(int firemode, int weaponId) parameters");
		return GM_EXCEPTION;
	}
	
	if(curWeapon && m != InvalidFireMode)
	{		
		curWeapon->UpdateAmmo(m);
		gmMachine *pMachine = a_thread->GetMachine();
		DisableGCInScope gcEn(pMachine);

		ammotbl->Set(pMachine, "CurrentAmmo", gmVariable(curWeapon->GetFireMode(m).GetCurrentAmmo()));
		ammotbl->Set(pMachine, "MaxAmmo", gmVariable(curWeapon->GetFireMode(m).GetMaxAmmo()));
		ammotbl->Set(pMachine, "CurrentClip", gmVariable(curWeapon->GetFireMode(m).GetCurrentClip()));
		ammotbl->Set(pMachine, "MaxClip", gmVariable(curWeapon->GetFireMode(m).GetMaxClip()));
		a_thread->PushInt(1);
	}
	else
	{
		a_thread->PushInt(0);
	}	
	return GM_OK;
}

// function: HasWeapon
//		This functions gets whether the bot has a specific weapon.
//
// Parameters:
//
//		int - weapon Id to check. Use values from the global WEAPON table.
//
// Returns:
//		int - true if the bot has the weapon, false if not.
int gmBot::gmfHasWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(weaponId, 0);
	
	a_thread->PushInt(native->GetWeaponSystem()->HasWeapon(weaponId) ? 1 : 0);
	return GM_OK;
}

// function: InFieldOfView
//		This functions checks whether a position is within the bots current field of view.
//
// Parameters:
//
//		<Vector3> - The 3d position to check with the bots field of view..
//		float - OPTIONAL angles to use as field of view, in degrees. Default is bots current FOV.
//
// Returns:
//		int - true if the bot has the weapon, false if not.

int gmBot::gmfInFieldOfView(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	GM_FLOAT_OR_INT_PARAM(fov, 1, native->GetFieldOfView());
	
	Vector3f toTarget = (Vector3f(v.x,v.y,v.z) - native->GetPosition());
	toTarget.Normalize();	
	a_thread->PushInt(Utils::InFieldOfView2d(native->GetFacingVector(), toTarget, fov) ? 1 : 0);
	return GM_OK;
}

// function: HasLineOfSightTo
//		This functions checks whether the bot has line of sight to a 3d position.
//		This function does not account for field of view, simply does a raycast for 
//		obstructions between the bots eye position and the provided position. To account
//		for field of view, use <InFieldOfView>. If an entity or gameId is provided as the 2nd parameter,
//		the function will return true if the raycast hits nothing on its way to the position OR
//		if it hits the entity that is passed.
//
// Parameters:
//
//		<Vector3> - The 3d position to check line of sight with.
//		<GameEntity> or int - Entity or GameId of the object to test line of sight with.
//
// Returns:
//		int - true if the bot has the weapon, false if not.
int gmBot::gmfHasLineOfSightTo(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	
	GameEntity gameEnt;
	if(a_thread->GetNumParams() == 2)
	{
		GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 1);
	}

	a_thread->PushInt(native->HasLineOfSightTo(Vector3f(v.x,v.y,v.z), gameEnt) ? 1 : 0);
	return GM_OK;
}

// function: GetMostDesiredAmmo
//		This functions gets the most desired ammo for the bot.
//
// Parameters:
//
//		table - Table to store the results in.
//
// Returns:
//		none
int gmBot::gmfGetMostDesiredAmmo(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_TABLE_PARAM(ret, 0);

	int iWeaponType = 0;
	int iGetAmmo = 1;
	obReal fDesir = native->GetWeaponSystem()->GetMostDesiredAmmo(iWeaponType,iGetAmmo);
	ret->Set(a_thread->GetMachine(), "Desire", gmVariable(fDesir));
	ret->Set(a_thread->GetMachine(), "AmmoType", gmVariable(iWeaponType));
	ret->Set(a_thread->GetMachine(), "GetAmmo", gmVariable(iGetAmmo));
	return GM_OK;
}

// function: HasAmmo
//		This functions gets whether the bot has a particular ammo type.
//
// Parameters:
//
//		int - ammo Id to check. Use values from the global AMMO table.
//		int - optional amount
//		- OR - 
//		None - uses the current weapon.
//
// Returns:
//		int - true if the bot has the ammo, false if not.
int gmBot::gmfHasAmmo(gmThread *a_thread)
{
	CHECK_THIS_BOT();

	bool bHasAmmo = false;
	if (a_thread->GetNumParams() == 1 || a_thread->GetNumParams() == 2)
	{
		GM_CHECK_INT_PARAM(ammotype, 0);
		GM_INT_PARAM(amount, 1, 0);
		bHasAmmo = native->GetWeaponSystem()->HasAmmo(ammotype, Primary, amount);
	}
	else if(a_thread->GetNumParams() == 0)
	{
		bHasAmmo = native->GetWeaponSystem()->HasAmmo(Primary);
	}
	else
	{
		// Didn't match one of the overloads
		GM_EXCEPTION_MSG("Expected 0 or 1 or 2 parameters");
		return GM_EXCEPTION;
	}
	a_thread->PushInt(bHasAmmo ? 1 : 0);
	return GM_OK;
}

// function: HasPowerUp
//		This functions gets whether the bot has a particular powerup.
//
// Parameters:
//
//		int - powerup Id to check. Use values from the global POWERUP table.
//		... any number of flags to check for.
//
// Returns:
//		int - true if the bot has the powerup, false if not.
int gmBot::gmfHasPowerUp(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(n, i);
		if(native->HasPowerup(n))
		{
			a_thread->PushInt(1);
			return GM_OK;
		}
	}
	a_thread->PushInt(0);
	return GM_OK;
}

// function: IsCarryingFlag
//		This functions gets whether the bot is carrying a flag.
//
// Parameters:
//
//		string - optional mapgoal name (ignored in ET and RTCW)
//
// Returns:
//		int - true if the bot has the flag, false if not.
int gmBot::gmfIsCarryingFlag(gmThread *a_thread)
{
	CHECK_THIS_BOT();

	MapGoalPtr pGoal;
	if (a_thread->GetNumParams() > 0)
	{
		GM_CHECK_STRING_PARAM(name, 0);
		if (name)
		{
			pGoal = GoalManager::GetInstance()->GetGoal(name);
			if (!pGoal) MapDebugPrint(a_thread, va("IsCarryingFlag: goal %s not found", name));
		}
	}
	a_thread->PushInt(native->DoesBotHaveFlag(pGoal) ? 1 : 0);
	return GM_OK;
}


// function: CanGrabItem
//		This function gets whether the bot can grab an entity.
//
// Parameters:
//
//		GameEntity
//		OR
//		string - goal name (FLAG or FLAG_dropped)
//
// Returns:
//		int - true if the bot can grab the entity, false if not.
int gmBot::gmfCanGrabItem(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	bool result = false;
	if (a_thread->ParamType(0) == GM_STRING)
	{
		GM_CHECK_STRING_PARAM(name, 0);
		MapGoalPtr pGoal = GoalManager::GetInstance()->GetGoal(name);
		if (!pGoal)
		{
			MapDebugPrint(a_thread, va("CanGrabItem: goal %s not found", name));
		}
		else result = native->IsFlagGrabbable(pGoal);
	}
	else
	{
		GameEntity gameEnt;
		GM_CHECK_GAMEENTITY_FROM_PARAM(gameEnt, 0);
		OBASSERT(gameEnt.IsValid(), "Bad Entity");
		result = native->IsItemGrabbable(gameEnt);
	}
	a_thread->PushInt(result ? 1 : 0);
	return GM_OK;
}


// function: HasEntityFlag
//		This functions gets whether the bot has a particular entity flag.
//
// Parameters:
//
//		int - powerup Id to check. Use values from the global ENTFLAG table.
//		... any number of flags to check for.
//
// Returns:
//		int - true if the bot has the powerup, false if not.
int gmBot::gmfHasEntityFlagAll(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(n, i);
		if(!native->HasEntityFlag(n))
		{
			a_thread->PushInt(0);
			return GM_OK;
		}
	}
	a_thread->PushInt(1);
	return GM_OK;
}

// function: HasEntityFlagAny
//		This functions gets whether the bot has a particular entity flag.
//
// Parameters:
//
//		int - powerup Id to check. Use values from the global ENTFLAG table.
//		... any number of flags to check for.
//
// Returns:
//		int - true if the bot has the powerup, false if not.
int gmBot::gmfHasEntityFlagAny(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(n, i);
		if(native->HasEntityFlag(n))
		{
			a_thread->PushInt(1);
			return GM_OK;
		}
	}
	a_thread->PushInt(0);
	return GM_OK;
}

// function: HasAnyWeapon
//		This functions gets whether the bot has any weapon from a table of provided weapon ids.
//
// Parameters:
//
//		table - table of weapon ids to check for. 
//		table - optional table to provide additional parameters
//		table - optional table to fill in with a list of all owned weapons from the input table
//
// Returns:
//		int - weapon Id of the first weapon in the list that the bot has
int gmBot::gmfHasAnyWeapon(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_TABLE_PARAM(wpns,0);
	GM_TABLE_PARAM(params,1,0);
	GM_TABLE_PARAM(weaponlist,2,0);

	FINDSTATE(ws,WeaponSystem,native->GetStateRoot());

	//////////////////////////////////////////////////////////////////////////
	bool CheckAmmo = true;
	bool CheckCharged = true;
	if(params)
	{
		gmVariable chkAmmo = params->Get(a_thread->GetMachine(),"CheckAmmo");
		if(chkAmmo.IsInt())
			CheckAmmo = chkAmmo.GetInt()!=0;

		gmVariable chkCharged = params->Get(a_thread->GetMachine(),"CheckCharged");
		if(chkCharged.IsInt())
			CheckCharged = chkCharged.GetInt()!=0;
	}
	//////////////////////////////////////////////////////////////////////////
	int WeaponIdFirstFound = 0;

	gmTableIterator it;
	gmTableNode *pNode = wpns->GetFirst(it);
	while(pNode)
	{
		if(pNode->m_value.IsInt())
		{
			const int WeaponId = pNode->m_key.GetInt();
			if(ws != NULL && ws->HasWeapon(WeaponId))
			{
				bool WeaponReady = true;
				if(CheckAmmo && !ws->HasAmmo(WeaponId))
					WeaponReady = false;
				if(CheckCharged && !InterfaceFuncs::IsWeaponCharged(native,WeaponId))
					WeaponReady = false;
				if(WeaponReady)
				{
					if(WeaponIdFirstFound==0)
						WeaponIdFirstFound = WeaponId;

					if(weaponlist)
					{
						weaponlist->Set(a_thread->GetMachine(),WeaponId,gmVariable(1));
					}

					else 
						break;
				}
			}
		}
		pNode = wpns->GetNext(it);
	}

	a_thread->PushInt(WeaponIdFirstFound);
	return GM_OK;
}



// function: HasTarget
//		This functions gets whether the bot has a target.
//
// Parameters:
//
//		none
//
// Returns:
//		int - true if the bot has the target, false if not.
int gmBot::gmfHasTarget(gmThread *a_thread)
{
	CHECK_THIS_BOT();

	FINDSTATE(ts,TargetingSystem,native->GetStateRoot());
	a_thread->PushInt(ts != NULL && ts->HasTarget() ? 1 : 0);
	return GM_OK;
}

// function: ChangeTeam
//		This functions tells the bot to change to a different team.
//
// Parameters:
//
//		int - team to change to.
//
// Returns:
//		None
int gmBot::gmfChangeTeam(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(teamId, 0);

	native->ChangeTeam( teamId );
	return GM_OK;
}

// function: ChangeClass
//		This functions tells the bot to change to a different class.
//
// Parameters:
//
//		int - team to change to.
//
// Returns:
//		None
int gmBot::gmfChangeClass(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(classId, 0);

	native->ChangeClass( classId );
	return GM_OK;
}

// function: SetDebugFlag
//		This functions sets the status of a debug flag for this bot. See the global DEBUG table.
//
// Parameters:
//
//		int - flag to set.
//		int - 1 for on, 0 for off.
//
// Returns:
//		None
int gmBot::gmfSetDebugFlag(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(flag, 0);
	GM_CHECK_INT_PARAM(val, 1);

	native->EnableDebug(flag, val != 0);
	return GM_OK;
}

// function: EnableShooting
//		This functions sets the status of a debug flag for this bot. See the global DEBUG table.
//
// Parameters:
//
//		int - flag to set.
//
// Returns:
//		None
int gmBot::gmfSetEnableShooting(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(enable, 0);

	native->SetUserFlag(Client::FL_SHOOTINGDISABLED, enable == 0);
	return GM_OK;
}

// function: ReloadProfile
//		This functions re-initializes the goals, weapons, and items with the properties held in script.
//
// Parameters:
//
//		None
//
// Returns:
//		None
int gmBot::gmfReloadProfile(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	native->LoadProfile(Client::PROFILE_CUSTOM);
	return GM_OK;
}

// function: Enable
//		This functions enables/disables the bot thinking.
//
// Parameters:
//
//		bool - true to enable, false to disable
//
// Returns:
//		None
int gmBot::gmfEnable(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(en, 0);

	native->SetUserFlag(Client::FL_DISABLED, en == 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: IsWeaponCharged
//		Gets whether a weapon is charged
//
// Parameters:
//
//		int - weapon id to check if charged
//		int - OPTIONAL, fire mode, 0 - primary, 1 = secondary
//
// Returns:
//		int - true if charged, false if not
int gmBot::gmfIsWeaponCharged(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(weaponId, 0);
	GM_INT_PARAM(fireMode, 1, Primary);

	a_thread->PushInt(InterfaceFuncs::IsWeaponCharged(native, weaponId, Weapon::GetFireMode(fireMode)) ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: DumpBotTable
//		Dumps the bots table to a file.
//
// Parameters:
//
//		string - filename to dump to
//
// Returns:
//		int - bot version number
int gmBot::gmfDumpBotTable(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(filename, 0);

	gmTableObject *pTbl = gmBot::GetThisTable(a_thread);
	if(pTbl)
	{
		char strBuffer[1024] = {};
		sprintf(strBuffer, "user/%s", filename);

		File outFile;
		outFile.OpenForWrite(strBuffer, File::Text);

		const int BUF_SIZE = 512;
		char buffer[BUF_SIZE] = {0};
		gmUtility::DumpTableInfo(a_thread->GetMachine(), 
			gmUtility::DUMP_ALL, pTbl, buffer, BUF_SIZE, 0, outFile);

		outFile.Close();
	}

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: CanSnipe
//		Checks the bot sniping capability. Playerclass is covert ops and has sniper weapon.
//
// Parameters:
//
//		void
//
// Returns:
//		none
int gmBot::gmfCanSnipe(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	a_thread->PushInt(native->CanBotSnipe() ? 1 : 0);
	return GM_OK;
}


//////////////////////////////////////////////////////////////////////////

// function: GetSkills
//		Get the skills for the bot
//
// Parameters:
//
//		table - table to fill skill info in
//
// Returns:
//		int - true if successful, false is failure
int gmBot::gmfGetSkills(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_TABLE_PARAM(skilltable, 0);
	a_thread->PushInt(native->GetSkills(a_thread->GetMachine(), skilltable)?1:0);
	return GM_OK;
}

int gmBot::gmfGetStat(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_STRING_PARAM(statname, 0);

	obUserData d = InterfaceFuncs::GetEntityStat(native->GetGameEntity(), statname);
	a_thread->Push(Utils::UserDataToGmVar(a_thread->GetMachine(),d));
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

int gmBot::gmfGetHighLevelGoalName(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	using namespace AiState;
	FINDSTATE(hl,HighLevel,native->GetStateRoot());
	if(hl != NULL && hl->GetActiveState())
		a_thread->PushNewString(hl->GetActiveState()->GetName().c_str());
	else
		a_thread->PushNull();

	return GM_OK;
}

int gmBot::gmfGetMapGoalName(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	using namespace AiState;
	FINDSTATE(hl,HighLevel,native->GetStateRoot());
	if(hl != NULL)
	{
		State *state = hl->GetActiveState();
		if(state)
		{
			MapGoal *g = state->GetMapGoalPtr();
			if(g)
			{
				a_thread->PushNewString(g->GetName().c_str());
				return GM_OK;
			}
		}
	}
	a_thread->PushNull();
	return GM_OK;
}

int gmBot::gmfSetRoles(gmThread *a_thread)
{
	CHECK_THIS_BOT();

	GM_CHECK_NUM_PARAMS(1);

	BitFlag32 rolemask = native->GetRoleMask(); // cs: preserve current mask
	for(int p = 0; p < a_thread->GetNumParams(); ++p)
	{
		GM_CHECK_INT_PARAM(r,p);
		rolemask.SetFlag(r,true);
	}
	native->SetRoleMask(rolemask);
	return GM_OK;
}

int gmBot::gmfClearRoles(gmThread *a_thread)
{
	CHECK_THIS_BOT();

	GM_CHECK_NUM_PARAMS(1);

	BitFlag32 rolemask = native->GetRoleMask(); // cs: preserve current mask
	bool lostDisguise = false;
	for(int p = 0; p < a_thread->GetNumParams(); ++p)
	{
		GM_CHECK_INT_PARAM(r,p);
		if(r==3 && native->IsInfiltrator() && !native->HasEntityFlag(ENT_FLAG_DEAD)) lostDisguise = true;
		rolemask.SetFlag(r,false);
	}
	native->SetRoleMask(rolemask);

	if(lostDisguise)
	{
		Event_DynamicPathsChanged m(0xFFFF /* all teams */, 0, F_NAV_INFILTRATOR | (F_NAV_NEXT<<16) /*F_ET_NAV_DISGUISE*/);
		native->SendEvent(MessageHelper(MESSAGE_DYNAMIC_PATHS_CHANGED,&m,sizeof(m)), Utils::MakeHash32("FollowPath"));
	}
	return GM_OK;
}

int gmBot::gmfHasRole(gmThread *a_thread)
{
	CHECK_THIS_BOT();
	GM_CHECK_NUM_PARAMS(1);

	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(n, i);
		if(native->GetRoleMask().CheckFlag(n))
		{
			a_thread->PushInt(1);
			return GM_OK;
		}
	}
	a_thread->PushInt(0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// Property Accessors/Modifiers

bool gmBot::getName( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native)
		a_operands[0] = gmVariable(a_thread->GetMachine()->AllocStringObject(a_native->GetName()));
	else
		a_operands[0].Nullify();
	return true;
}

bool gmBot::setName( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
	if(a_native && pStr != NULL && pStr->GetString())
	{
		InterfaceFuncs::ChangeName(a_native, pStr->GetString());
	}
	return true;
}

bool gmBot::getMemorySpan( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native && a_native->GetSensoryMemory())
        a_operands[0].SetFloat((float)a_native->GetSensoryMemory()->GetMemorySpan() / 1000.0f);
	else
		a_operands[0].Nullify();
	return true;
}

bool gmBot::setMemorySpan( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native && a_native->GetSensoryMemory())
	{
		float fMemorySpan = 0.0f;
		if(gmGetFloatOrIntParamAsFloat(a_operands[1], fMemorySpan))
		{
			int iMilliseconds = Utils::SecondsToMilliseconds(fMemorySpan);
			a_native->GetSensoryMemory()->SetMemorySpan(OB_MAX(0, iMilliseconds));
			return true;
		}
		return false;
	}
	return true;
}

bool gmBot::getAimPersistance( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native && a_native->GetWeaponSystem())
		a_operands[0].SetFloat((float)a_native->GetWeaponSystem()->GetAimPersistance() / 1000.0f);
	else
		a_operands[0].Nullify();
	return true;
}

bool gmBot::setAimPersistance( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native && a_native->GetWeaponSystem())
	{
		float fAimPersistance = 0.0f;
		if(gmGetFloatOrIntParamAsFloat(a_operands[1], fAimPersistance))
		{
			int iMilliseconds = Utils::SecondsToMilliseconds(fAimPersistance);
			a_native->GetWeaponSystem()->SetAimPersistance(OB_MAX(0, iMilliseconds));
			return true;
		}
		return false;
	}
	return false;
}

bool gmBot::getReactionTime( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native && a_native->GetWeaponSystem())
		a_operands[0].SetFloat((float)a_native->GetWeaponSystem()->GetReactionTime() / 1000.0f);
	else
		a_operands[0].Nullify();
	return true;
}

bool gmBot::setReactionTime( Client *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native && a_native->GetWeaponSystem())
	{
		float fReactionTime = 0.0f;
		if(gmGetFloatOrIntParamAsFloat(a_operands[1], fReactionTime))
		{
			int iMilliseconds = Utils::SecondsToMilliseconds(fReactionTime);
			a_native->GetWeaponSystem()->SetReactionTime(OB_MAX(0, iMilliseconds));
			return true;
		}
		return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// Operators

