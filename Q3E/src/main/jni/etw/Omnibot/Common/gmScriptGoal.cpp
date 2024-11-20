#include "PrecompCommon.h"
#include "gmScriptGoal.h"
#include "gmBotLibrary.h"

using namespace AiState;

#define CHECK_THIS_SGOAL() \
	ScriptGoal *native = gmScriptGoal::GetThisObject(a_thread); \
	if(!native) \
{ \
	GM_EXCEPTION_MSG("Script Function on NULL object"); \
	return GM_EXCEPTION; \
}

// script: ScriptGoal
//		Script Bindings for <ScriptGoal>

GMBIND_INIT_TYPE( gmScriptGoal, "Goal" );

GMBIND_FUNCTION_MAP_BEGIN( gmScriptGoal )
	GMBIND_FUNCTION( "LimitToClass", gmfLimitToClass )
	GMBIND_FUNCTION( "LimitToNoClass", gmfLimitToNoClass )
	GMBIND_FUNCTION( "LimitToTeam", gmfLimitToTeam )
	GMBIND_FUNCTION( "LimitToPowerUp", gmfLimitToPowerUp )
	GMBIND_FUNCTION( "LimitToNoPowerup", gmfLimitToNoPowerup )	
	GMBIND_FUNCTION( "LimitToEntityFlag", gmfLimitToEntityFlag )
	GMBIND_FUNCTION( "LimitToNoEntityFlag", gmfLimitToNoEntityFlag )	
	GMBIND_FUNCTION( "LimitToWeapon", gmfLimitToWeapon )
	GMBIND_FUNCTION( "LimitToRole", gmfLimitToRole )
	GMBIND_FUNCTION( "LimitTo", gmfLimitTo )

	GMBIND_FUNCTION( "LimitToNoTarget", gmfLimitToNoTarget )
	GMBIND_FUNCTION( "LimitToTargetClass", gmfLimitToTargetClass )
	GMBIND_FUNCTION( "LimitToTargetTeam", gmfLimitToTargetTeam )
	GMBIND_FUNCTION( "LimitToTargetPowerUp", gmfLimitToTargetPowerUp )
	GMBIND_FUNCTION( "LimitToTargetNoPowerUp", gmfLimitToTargetNoPowerUp )
	GMBIND_FUNCTION( "LimitToTargetEntityFlag", gmfLimitToTargetEntityFlag )	
	GMBIND_FUNCTION( "LimitToTargetNoEntityFlag", gmfLimitToTargetNoEntityFlag )
	GMBIND_FUNCTION( "LimitToTargetWeapon", gmfLimitToTargetWeapon )	

	GMBIND_FUNCTION( "IsActive", gmfIsActive )

	GMBIND_FUNCTION( "Goto", gmfGoto )
	GMBIND_FUNCTION( "GotoAsync", gmfGotoAsync )
	GMBIND_FUNCTION( "GotoRandom", gmfGotoRandom )
	GMBIND_FUNCTION( "GotoRandomAsync", gmfGotoRandomAsync )
	GMBIND_FUNCTION( "RouteTo", gmfRouteTo )
	GMBIND_FUNCTION( "Stop", gmfStop )

	GMBIND_FUNCTION( "Finished", gmfFinished )

	GMBIND_FUNCTION( "DidPathSucceed", gmfDidPathSucceed )
	GMBIND_FUNCTION( "DidPathFail", gmfDidPathFail )

	GMBIND_FUNCTION( "AddAimRequest", gmfAddAimRequest )
	GMBIND_FUNCTION( "ReleaseAimRequest", gmfReleaseAimRequest )

	GMBIND_FUNCTION( "AddWeaponRequest", gmfAddWeaponRequest )
	GMBIND_FUNCTION( "ReleaseWeaponRequest", gmfReleaseWeaponRequest )
	GMBIND_FUNCTION( "UpdateWeaponRequest", gmfUpdateWeaponRequest )

	GMBIND_FUNCTION( "BlockForWeaponChange", gmfBlockForWeaponChange )
	GMBIND_FUNCTION( "BlockForWeaponFire", gmfBlockForWeaponFire )	
	GMBIND_FUNCTION( "BlockForVoiceMacro", gmfBlockForVoiceMacro )

	GMBIND_FUNCTION( "AddFinishCriteria", gmfAddFinishCriteria )
	GMBIND_FUNCTION( "ClearFinishCriteria", gmfClearFinishCriteria )

	GMBIND_FUNCTION( "ForkThread", gmfThreadFork )
	GMBIND_FUNCTION( "KillThread", gmfThreadKill )
	GMBIND_FUNCTION( "Signal", gmfSignal )

	GMBIND_FUNCTION( "QueryGoals", gmfQueryMapGoals )	
	
	GMBIND_FUNCTION( "WatchForMapGoalsInRadius", gmfWatchForMapGoalsInRadius )	
	GMBIND_FUNCTION( "ClearWatchForMapGoalsInRadius", gmfClearWatchForMapGoalsInRadius )
	GMBIND_FUNCTION( "WatchForEntityCategory", gmfWatchForEntityCategory )	

	GMBIND_FUNCTION( "DelayGetPriority", gmfDelayGetPriority )	

	GMBIND_FUNCTION( "BlackboardDelay", gmfBlackboardDelay )
	GMBIND_FUNCTION( "BlackboardIsDelayed", gmfBlackboardIsDelayed )

	GMBIND_FUNCTION( "MarkInProgress", gmfMarkInProgress )
	GMBIND_FUNCTION( "MarkInUse", gmfMarkInUse )
GMBIND_FUNCTION_MAP_END()

GMBIND_PROPERTY_MAP_BEGIN( gmScriptGoal )
	GMBIND_PROPERTY( "Name", getName, setName )
	GMBIND_PROPERTY( "Parent", getInsertParent, setInsertParent )
	GMBIND_PROPERTY( "InsertBefore", getInsertBefore, setInsertBefore )
	GMBIND_PROPERTY( "InsertAfter", getInsertAfter, setInsertAfter )
	GMBIND_PROPERTY( "Disable", getDisable, setDisable )
	GMBIND_PROPERTY( "AutoAdd", getAutoAdd, setAutoAdd )
	
	GMBIND_PROPERTY( "Initialize", getInitializeFunc, setInitializeFunc )
	GMBIND_PROPERTY( "OnSpawn", getSpawnFunc, setSpawnFunc )
	GMBIND_PROPERTY( "GetPriority", getPriorityFunc, setPriorityFunc )
	GMBIND_PROPERTY( "Enter", getEnterFunc, setEnterFunc )
	GMBIND_PROPERTY( "Exit", getExitFunc, setExitFunc )
	GMBIND_PROPERTY( "Update", getUpdateFunc, setUpdateFunc )
	GMBIND_PROPERTY( "OnPathThrough", getPathThroughFunc, setPathThroughFunc )
	GMBIND_PROPERTY( "AimVector", getAimVectorFunc, setAimVectorFunc )
	GMBIND_PROPERTY( "AimWeaponId", getAimWeaponIdFunc, setAimWeaponIdFunc )

	GMBIND_PROPERTY( "GetPriorityDelay", getGetPriorityDelay, setGetPriorityDelay )

	GMBIND_PROPERTY( "AutoReleaseAim", getAutoReleaseAim, setAutoReleaseAim )
	GMBIND_PROPERTY( "AutoReleaseWeapon", getAutoReleaseWeapon, setAutoReleaseWeapon )
	GMBIND_PROPERTY( "AutoReleaseTrackers", getAutoReleaseTrackers, setAutoReleaseTrackers )
	GMBIND_PROPERTY( "AutoFinishOnUnAvailable", getAutoFinishOnUnavailable, setAutoFinishOnUnavailable )
	GMBIND_PROPERTY( "AutoFinishOnNoProgressSlots", getAutoFinishOnNoProgressSlots, setAutoFinishOnNoProgressSlots )
	GMBIND_PROPERTY( "AutoFinishOnNoUseSlots", getAutoFinishOnNoUseSlots, setAutoFinishOnNoUseSlots )
	GMBIND_PROPERTY( "SkipGetPriorityWhenActive", getSkipGetPriorityWhenActive, setSkipGetPriorityWhenActive )
	
	GMBIND_PROPERTY( "AlwaysRecieveEvents", getAlwaysRecieveEvents, setAlwaysRecieveEvents )

	GMBIND_PROPERTY( "Events", getEvents, setEvents )
	GMBIND_PROPERTY( "Commands", getCommands, setCommands )

	GMBIND_PROPERTY( "DebugString", getDebugString, setDebugString )
	GMBIND_PROPERTY( "Debug", getDebug, setDebug )	

	GMBIND_PROPERTY( "Bot", getBot, NULL )
	GMBIND_PROPERTY( "MapGoal", getMapGoal, setMapGoal )

	GMBIND_PROPERTY( "Priority", getScriptPriority, setScriptPriority )
GMBIND_PROPERTY_MAP_END();

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

ScriptGoal *gmScriptGoal::Constructor(gmThread *a_thread)
{
	if(a_thread->ParamType(0) == GM_STRING)
	{
		const char *pName = a_thread->ParamString(0, NULL);
		if(pName)
			return new ScriptGoal(pName);
	}
	return 0;
}

void gmScriptGoal::Destructor(ScriptGoal *_native)
{
	OBASSERT(0,"Invalid Call");
}

void gmScriptGoal::AsStringCallback(AiState::ScriptGoal * a_object, char * a_buffer, int a_bufferLen)
{
	if(a_object)
	{
		_gmsnprintf(a_buffer, a_bufferLen, 
			"Behavior(%s, %s)",a_object->GetName().c_str(),  a_object->GetClient() ? a_object->GetClient()->GetName(true) : "");
	}
}

//////////////////////////////////////////////////////////////////////////

int gmScriptGoal::gmfLimitToClass(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToClass().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToClass().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToNoClass(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToClass() = BitFlag32(~0);
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToClass().ClearFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTeam(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTeam().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTeam().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToPowerUp(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToPowerup().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToPowerup().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToNoPowerup(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToNoPowerup().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToNoPowerup().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToEntityFlag(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToEntFlag().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToEntFlag().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToNoEntityFlag(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToNoEntFlag().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToNoEntFlag().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToWeapon(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToWeapon().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToWeapon().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToRole(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToRole().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToRole().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitTo(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);

	if(a_thread->ParamType(0)==GM_NULL)
	{
		if(native)
			native->ClearLimitTo();
		return GM_OK;
	}

	GM_CHECK_FUNCTION_PARAM(fn,0);
	GM_FLOAT_OR_INT_PARAM(dly,1,0);
	GM_INT_PARAM(activeonly,2,0);
	if(native)
	{
		gmGCRoot<gmFunctionObject> func(fn,a_thread->GetMachine());		
		native->LimitTo(*a_thread->GetThis(), func, Utils::SecondsToMilliseconds(dly),activeonly!=0);		
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToNoTarget(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	if(native)
	{
		// special case
		native->LimitToTargetClass().SetFlag(0);
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTargetClass(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTargetClass().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTargetClass().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTargetWeapon(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTargetWeapon().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTargetWeapon().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTargetTeam(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTargetTeam().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTargetTeam().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTargetPowerUp(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTargetPowerup().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTargetPowerup().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTargetNoPowerUp(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTargetNoPowerup().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTargetNoPowerup().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTargetEntityFlag(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTargetEntFlag().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTargetEntFlag().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfLimitToTargetNoEntityFlag(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	if(native)
	{
		native->LimitToTargetNoEntFlag().ClearAll();
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
		{
			GM_CHECK_INT_PARAM(id, i);
			native->LimitToTargetNoEntFlag().SetFlag(id);
		}
	}
	return GM_OK; 
}

int gmScriptGoal::gmfFinished(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	if(native)
		native->SetFinished();
	return GM_SYS_KILL; 
}

int gmScriptGoal::gmfDidPathSucceed(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	a_thread->PushInt(native->DidPathSucceed() ? 1 : 0);
	return GM_SYS_KILL; 
}

int gmScriptGoal::gmfDidPathFail(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	a_thread->PushInt(native->DidPathFail() ? 1 : 0);
	return GM_SYS_KILL; 
}

int gmScriptGoal::gmfIsActive(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	a_thread->PushInt(native->IsActive() ? 1 : 0);
	return GM_OK; 
}

int gmScriptGoal::gmfGoto(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_FLOAT_OR_INT_PARAM(r,1,32.f);
	GM_TABLE_PARAM(Options,2,NULL);

	MoveOptions opn;
	opn.Radius = r;
	opn.ThreadId = a_thread->GetId();

	if(Options)
	{
		opn.FromTable(a_thread->GetMachine(),Options);
	}

	bool success; 

	switch (a_thread->ParamType(0))
	{
	case GM_VEC3:
	{
		Vec3 v = ConvertVec3(a_thread->Param(0).m_value.m_vec3);
		success = native->Goto(Vector3f(v.x, v.y, v.z), opn);
		break;
	}
	case GM_TABLE:
	{
		gmTableObject *typesTable = a_thread->ParamTable(0);
		Vector3List list;
		list.reserve(typesTable->Count());
		gmTableIterator tIt;
		for (gmTableNode *pNode = typesTable->GetFirst(tIt); pNode; pNode = typesTable->GetNext(tIt))
		{
			if (pNode->m_value.m_type != GM_VEC3)
			{
				GM_EXCEPTION_MSG("expecting param 1 as table of vectors, got %s", a_thread->GetMachine()->GetTypeName(pNode->m_value.m_type));
				return GM_EXCEPTION;
			}
			Vector3f v;
			pNode->m_value.GetVector(v.x, v.y, v.z);
			list.push_back(v);
		}
		success = native->Goto(list, opn);
		break;
	}
	default:
		GM_EXCEPTION_MSG("expecting param 1 as vector or table, got %s", a_thread->ParamTypeName(0));
		return GM_EXCEPTION;
	}

	if(success)
	{
		gmVariable blocks[2] = { gmVariable(PATH_SUCCESS), gmVariable(PATH_FAILED) };
		int res = a_thread->GetMachine()->Sys_Block(a_thread, 2, blocks);
		if(res == -1)
			return GM_SYS_BLOCK;
		else if(res == -2)
			return GM_SYS_YIELD;
	}
	a_thread->Push(gmVariable(PATH_FAILED));
	return GM_SYS_YIELD;
}

int gmScriptGoal::gmfGotoAsync(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_VECTOR_PARAM(v,0);
	GM_FLOAT_OR_INT_PARAM(r,1,32.f);
	GM_TABLE_PARAM(Options,2,NULL);

	MoveOptions opn;
	opn.Radius = r;
	opn.ThreadId = a_thread->GetId();

	if(Options)
	{
		opn.FromTable(a_thread->GetMachine(),Options);
	}

	bool b = native->Goto(Vector3f(v.x,v.y,v.z), opn);
	a_thread->Push(gmVariable(b?1:0));
	return GM_OK; 
}

int gmScriptGoal::gmfGotoRandom(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_FLOAT_OR_INT_PARAM(r,0,32.f);
	GM_TABLE_PARAM(Options,1,NULL);

	MoveOptions opn;
	opn.Radius = r;
	opn.ThreadId = a_thread->GetId();

	if(Options)
	{
		opn.FromTable(a_thread->GetMachine(),Options);
	}

	if(native->GotoRandom(opn))
	{
		gmVariable blocks[2] = { gmVariable(PATH_SUCCESS), gmVariable(PATH_FAILED) };
		int res = a_thread->GetMachine()->Sys_Block(a_thread, 2, blocks);
		if(res == -1)
			return GM_SYS_BLOCK;
		else if(res == -2)
			return GM_SYS_YIELD;
	}
	a_thread->Push(gmVariable(PATH_FAILED));
	return GM_SYS_YIELD;
}

int gmScriptGoal::gmfGotoRandomAsync(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_FLOAT_OR_INT_PARAM(r,0,32.f);
	GM_TABLE_PARAM(Options,1,NULL);

	MoveOptions opn;
	opn.Radius = r;
	opn.ThreadId = a_thread->GetId();

	if(Options)
	{
		opn.FromTable(a_thread->GetMachine(),Options);
	}

	bool b = native->GotoRandom(opn);
	a_thread->Push(gmVariable(b?1:0));
	return GM_OK; 
}

int gmScriptGoal::gmfRouteTo(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	MapGoal *Mg = 0;
	if(!gmBind2::Class<MapGoal>::FromParam(a_thread,0,Mg))
	{
		GM_EXCEPTION_MSG("Expected MapGoal as parameter 0, got %s",
			a_thread->GetMachine()->GetTypeName(a_thread->ParamType(0)));
		return GM_EXCEPTION;
	}
	
	GM_FLOAT_OR_INT_PARAM(r,1,32.f);
	GM_TABLE_PARAM(Options,2,NULL);

	MoveOptions opn;
	opn.Radius = r;
	opn.ThreadId = a_thread->GetId();

	if(Options)
	{
		opn.FromTable(a_thread->GetMachine(),Options);
	}

	MapGoalPtr mgp = GoalManager::GetInstance()->GetGoal(Mg->GetName());
	if(mgp && native->RouteTo(mgp, opn))
	{
		gmVariable blocks[2] = { gmVariable(PATH_SUCCESS), gmVariable(PATH_FAILED) };
		int res = a_thread->GetMachine()->Sys_Block(a_thread, 2, blocks);
		if(res == -1)
			return GM_SYS_BLOCK;
		else if(res == -2)
			return GM_SYS_YIELD;
	}
	a_thread->Push(gmVariable(PATH_FAILED));
	return GM_OK; 
}

int gmScriptGoal::gmfStop(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	native->Stop();
	return GM_OK;
}

int gmScriptGoal::gmfAddAimRequest(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(priority, 0);
	GM_STRING_PARAM(aimtype, 1, NULL);
	GM_VECTOR_PARAM(v, 2, 0.f, 0.f, 0.f);

	using namespace AiState;

	Aimer::AimType eAimType = Aimer::WorldPosition;
	if(!aimtype || !_gmstricmp(aimtype, "position"))
		eAimType = Aimer::WorldPosition;
	else if(!_gmstricmp(aimtype, "facing"))
		eAimType = Aimer::WorldFacing;
	else if(!_gmstricmp(aimtype, "movedirection"))
		eAimType = Aimer::MoveDirection;
	else
	{
		GM_EXCEPTION_MSG("Invalid Aim Type");
		return GM_EXCEPTION;
	}

	Priority::ePriority prio = (Priority::ePriority)priority;
	if(!native->AddScriptAimRequest(prio,eAimType,Vector3f(v.x, v.y, v.z)))
	{
		GM_EXCEPTION_MSG("Unable to add aim request. Too many!");
		return GM_EXCEPTION;
	}
	return GM_OK;
}

int gmScriptGoal::gmfReleaseAimRequest(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();

	using namespace AiState;
	FINDSTATE(aim, Aimer, native->GetClient()->GetStateRoot());
	if(aim)
		aim->ReleaseAimRequest(native->GetNameHash());
	return GM_OK;
}

int gmScriptGoal::gmfAddWeaponRequest(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(priority, 0);
	GM_CHECK_INT_PARAM(weaponId, 1);

	bool bSuccess = false;

	using namespace AiState;

	Priority::ePriority prio = (Priority::ePriority)priority;
	FINDSTATE(weapsys, WeaponSystem, native->GetClient()->GetStateRoot());
	if(weapsys)
		bSuccess = weapsys->AddWeaponRequest(prio, native->GetNameHash(), weaponId);

	if(!bSuccess)
	{
		GM_EXCEPTION_MSG("Unable to add weapon request. Too many!");
		return GM_EXCEPTION;
	}
	return GM_OK;
}

int gmScriptGoal::gmfReleaseWeaponRequest(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();

	using namespace AiState;
	FINDSTATE(weapsys, WeaponSystem, native->GetClient()->GetStateRoot());
	if(weapsys)
		weapsys->ReleaseWeaponRequest(native->GetNameHash());
	return GM_OK;
}

int gmScriptGoal::gmfUpdateWeaponRequest(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(weaponId, 0);

	bool bSuccess = false;

	using namespace AiState;
	FINDSTATE(weapsys, WeaponSystem, native->GetClient()->GetStateRoot());
	if(weapsys)
		bSuccess = weapsys->UpdateWeaponRequest(native->GetNameHash(), weaponId);

	if(!bSuccess)
	{
		GM_EXCEPTION_MSG("Unable to update weapon request. Not Found!");
		return GM_EXCEPTION;
	}
	return GM_OK;
}

int gmScriptGoal::gmfBlockForWeaponChange(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_INT_PARAM(weaponId, 0);

	gmVariable varSig(Utils::MakeId32((obint16)ACTION_WEAPON_CHANGE, (obint16)weaponId));

	AiState::WeaponSystem *ws = native->GetClient()->GetWeaponSystem();
	if(ws != NULL && ws->CurrentWeaponIs(weaponId))
	{
		a_thread->Push(varSig);
		return GM_OK;
	}
	
	int res = a_thread->GetMachine()->Sys_Block(a_thread, 1, &varSig);
	if(res == -1)
		return GM_SYS_BLOCK;
	else if(res == -2)
		return GM_SYS_YIELD;
	a_thread->Push(a_thread->Param(res));
	return GM_OK;
}

int gmScriptGoal::gmfBlockForWeaponFire(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_INT_PARAM(weaponId, 0);
	gmVariable varSig(Utils::MakeId32((obint16)ACTION_WEAPON_FIRE, (obint16)weaponId));
	int res = a_thread->GetMachine()->Sys_Block(a_thread, 1, &varSig);
	if(res == -1)
		return GM_SYS_BLOCK;
	else if(res == -2)
		return GM_SYS_YIELD;
	a_thread->Push(a_thread->Param(res));
	return GM_OK;
}

int gmScriptGoal::gmfBlockForVoiceMacro(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	
	int iNumMacros = 0;
	gmVariable signals[128];
	for(int i = 0; i < a_thread->GetNumParams(); ++i)
	{
		GM_CHECK_INT_PARAM(macroId,i);
		signals[iNumMacros++]= gmVariable(Utils::MakeId32((obint16)PERCEPT_HEAR_VOICEMACRO, (obint16)macroId));
	}
	
	int res = a_thread->GetMachine()->Sys_Block(a_thread, iNumMacros, signals);
	if(res == -1)
		return GM_SYS_BLOCK;
	else if(res == -2)
		return GM_SYS_YIELD;
	a_thread->Push(a_thread->Param(res));
	return GM_OK;
}

int gmScriptGoal::gmfThreadFork(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FUNCTION_PARAM(fn, 0);
	int threadId = GM_INVALID_THREAD;
	gmThread *thread = a_thread->GetMachine()->CreateThread(&threadId);
	if(thread)
	{
		thread->Push(*a_thread->GetThis());
		thread->PushFunction(fn);
		int numParameters = a_thread->GetNumParams() - 1;
		for(int i = 0; i < numParameters; ++i)
			thread->Push(a_thread->Param(i + 1));
		thread->PushStackFrame(numParameters, 0);
	}
	native->AddForkThreadId(threadId);
	a_thread->PushInt(threadId);
	return GM_OK;
}

int gmScriptGoal::gmfThreadKill(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(id,0);
	if(id == a_thread->GetId())
	{
		a_thread->PushInt(1);
		return GM_SYS_KILL;
	}
	a_thread->PushInt(native->DeleteForkThread(id)?1:0);	
	return GM_OK;
}

int gmScriptGoal::gmfSignal(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	for(int i = 0; i < a_thread->GetNumParams(); ++i)
		native->Signal(a_thread->Param(i));
	return GM_OK;
}

int gmScriptGoal::gmfDelayGetPriority(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_FLOAT_OR_INT_PARAM(dly,0);
	native->DelayGetPriority(Utils::SecondsToMilliseconds(dly));	
	return GM_OK;
}

int gmScriptGoal::gmfBlackboardDelay(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(time,0);

	int serial;
	MapGoal *Mg = 0;

	if (a_thread->GetNumParams() < 2)
	{
		MapGoal* mg = native->GetMapGoal().get();
		if(!mg)
		{
			GM_EXCEPTION_MSG("this.MapGoal is null");
			return GM_EXCEPTION;
		}
		serial = mg->GetSerialNum();
	}
	else if(gmBind2::Class<MapGoal>::FromVar(a_thread,a_thread->Param(1),Mg) && Mg)
	{
		MapGoalPtr mg = Mg->GetSmartPtr();
		if(!mg)
		{
			GM_EXCEPTION_MSG("error retrieving %s",gmBind2::Class<MapGoal>::ClassName());
			return GM_EXCEPTION;
		}
		serial = mg->GetSerialNum();
	}
	else if(a_thread->Param(1).IsInt())
	{
		serial = a_thread->Param(1).GetInt();
	}
	else
	{
		enum { BufferSize = 1024 };
		char buffer[BufferSize] = {};
		GM_EXCEPTION_MSG("expecting %s or int, got %s",
			gmBind2::Class<MapGoal>::ClassName(),
			a_thread->Param(1).AsStringWithType(a_thread->GetMachine(), buffer, BufferSize));
		return GM_EXCEPTION;
	}
	native->BlackboardDelay(time, serial);
	return GM_OK;
}

int gmScriptGoal::gmfBlackboardIsDelayed(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	
	MapGoal *Mg = 0;
	if(gmBind2::Class<MapGoal>::FromVar(a_thread,a_thread->Param(0),Mg) && Mg)
	{
		MapGoalPtr mg = Mg->GetSmartPtr();
		if(mg)
		{
			a_thread->PushInt(native->BlackboardIsDelayed(mg->GetSerialNum())?1:0);
			return GM_OK;
		}
		else
		{
			GM_EXCEPTION_MSG("error retrieving %s",gmBind2::Class<MapGoal>::ClassName());
			return GM_EXCEPTION;
		}
	}
	else if(a_thread->Param(0).IsInt())
	{
		a_thread->PushInt(native->BlackboardIsDelayed(a_thread->Param(0).GetInt())?1:0);
		return GM_OK;
	}
	
	enum { BufferSize=1024 };
	char buffer[BufferSize] = {};
	GM_EXCEPTION_MSG("expecting %s, got %s",
		gmBind2::Class<MapGoal>::ClassName(),
		a_thread->Param(0).AsStringWithType(a_thread->GetMachine(),buffer,BufferSize));
	return GM_EXCEPTION;
}

int gmScriptGoal::gmfMarkTracker(gmThread *a_thread, bool (ScriptGoal::*_func)(MapGoalPtr))
{
	CHECK_THIS_SGOAL();

	MapGoalPtr mg;
	MapGoal *Mg = 0;
	if (a_thread->GetNumParams() == 0)
	{
		mg = native->GetMapGoal();
	}
	else if(gmBind2::Class<MapGoal>::FromVar(a_thread,a_thread->Param(0),Mg) && Mg)
	{
		mg = Mg->GetSmartPtr();
		if(!mg)
		{
			GM_EXCEPTION_MSG("error retrieving %s",gmBind2::Class<MapGoal>::ClassName());
			return GM_EXCEPTION;
		}
	}
	else if(!a_thread->Param(0).IsNull())
	{
		enum { BufferSize = 1024 };
		char buffer[BufferSize] = {};
		GM_EXCEPTION_MSG("expecting %s, got %s",
			gmBind2::Class<MapGoal>::ClassName(),
			a_thread->Param(0).AsStringWithType(a_thread->GetMachine(), buffer, BufferSize));
		return GM_EXCEPTION;
	}

	a_thread->PushInt((native->*_func)(mg) ? 1 : 0);
	return GM_OK;
}

int gmScriptGoal::gmfMarkInProgress(gmThread *a_thread)
{
	return gmfMarkTracker(a_thread, &ScriptGoal::MarkInProgress);
}

int gmScriptGoal::gmfMarkInUse(gmThread *a_thread)
{
	return gmfMarkTracker(a_thread, &ScriptGoal::MarkInUse);
}


int gmScriptGoal::gmfAddFinishCriteria(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
		
	CheckCriteria c;
	StringStr err;
	if(IGameManager::GetInstance()->GetGame()->CreateCriteria(a_thread,c,err))
	{
		a_thread->PushInt(native->AddFinishCriteria(c) ? 1 : 0);
	}
	else
	{
		GM_EXCEPTION_MSG(err.str().c_str());
		return GM_EXCEPTION;
	}
	return GM_OK;
}

int gmScriptGoal::gmfClearFinishCriteria(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_INT_PARAM(persist,0,0);
	native->ClearFinishCriteria(persist!=0);
	return GM_OK;
}

int gmScriptGoal::gmfQueryMapGoals(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	Prof(QueryMapGoals);
	return GetMapGoals(a_thread, native->GetClient());
}

int gmScriptGoal::gmfWatchForMapGoalsInRadius(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();

	GameEntity ent;
	GM_CHECK_GAMEENTITY_FROM_PARAM(ent,0);
	GM_CHECK_FLOAT_OR_INT_PARAM(radius,1);
	GM_STRING_PARAM(pExpr,2,0);
	GM_TABLE_PARAM(params,3,0);

	Prof(WatchForMapGoalsInRadius);

	GoalManager::Query qry;
	qry.Bot(native->GetClient());
	qry.Expression(pExpr);

	// parse optional parameters
	if(params)
	{
		qry.FromTable(a_thread->GetMachine(),params);
	}	

	if ( qry.GetError() != GoalManager::Query::QueryOk ) {
		GM_EXCEPTION_MSG(qry.QueryErrorString());
		return GM_EXCEPTION;
	}

	native->WatchForMapGoalsInRadius(qry,ent,radius);
	a_thread->PushInt(1);
	return GM_OK;
}

int gmScriptGoal::gmfClearWatchForMapGoalsInRadius(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	native->ClearWatchForMapGoalsInRadius();
	return GM_OK;
}

int gmScriptGoal::gmfWatchForEntityCategory(gmThread *a_thread)
{
	CHECK_THIS_SGOAL();
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_TABLE_PARAM(tbl,0);

	BitFlag32 entCategory;

	// parse parameters
	gmVariable varRadius = tbl->Get(a_thread->GetMachine(),"Radius");
	if(!varRadius.IsNumber())
	{
		GM_EXCEPTION_MSG("Expected Radius");
		return GM_EXCEPTION;
	}
	gmVariable varCategory = tbl->Get(a_thread->GetMachine(),"Category");
	if(varCategory.IsInt())
	{
		entCategory.SetFlag(varCategory.GetInt());
	}
	else if(gmTableObject *catTbl = varCategory.GetTableObjectSafe())
	{
		gmTableIterator tIt;
		gmTableNode *pNode = catTbl->GetFirst(tIt);
		while(pNode)
		{
			if(pNode->m_value.IsInt())
			{
				entCategory.SetFlag(pNode->m_value.GetInt());
			}
			else
			{
				enum { BuffSize=256 };
				char buffer[BuffSize];
				const char *var = pNode->m_value.AsStringWithType(a_thread->GetMachine(),buffer,BuffSize);
				GM_EXCEPTION_MSG("expected int, got %s",var);
				return GM_EXCEPTION;
			}
			pNode = catTbl->GetNext(tIt);
		}
	}

	const int CustomTraceMask = tbl->Get(a_thread->GetMachine(),"RequireLOS").GetIntSafe(0);
	native->WatchForEntityCategory(varRadius.GetFloatSafe(),entCategory,CustomTraceMask);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// Property Accessors/Modifiers

bool gmScriptGoal::getName( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetString(a_thread->GetMachine(), a_native->GetName().c_str());
	return true;
}

bool gmScriptGoal::setName( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
	if(pStr)
	{
		a_native->SetName(pStr->GetString());
		a_native->SetFollowUserName(pStr->GetString());
		a_native->SetProfilerZone(pStr->GetString());
	}
	return true;
}

bool gmScriptGoal::getInsertParent( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetString(a_thread->GetMachine(), a_native->GetParentName().c_str());
	return true;
}

bool gmScriptGoal::setInsertParent( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
	if(pStr)
		a_native->SetParentName(pStr->GetString());
	return true;
}

bool gmScriptGoal::getInsertBefore( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetString(a_thread->GetMachine(), a_native->GetInsertBeforeName().c_str());
	return true;
}

bool gmScriptGoal::setInsertBefore( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
	if(pStr)
		a_native->SetInsertBeforeName(pStr->GetString());
	return true;
}

bool gmScriptGoal::getInsertAfter( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetString(a_thread->GetMachine(), a_native->GetInsertAfterName().c_str());
	return true;
}

bool gmScriptGoal::setInsertAfter( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
	if(pStr)
		a_native->SetInsertAfterName(pStr->GetString());
	return true;
}

bool gmScriptGoal::getDisable( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetInt(a_native->IsUserDisabled()?1:0);
	return true;
}

bool gmScriptGoal::setDisable( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_operands[1].IsInt())
		a_native->SetEnable(a_operands[1].GetInt()?0:1);
	return true;
}

bool gmScriptGoal::getAutoAdd( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetInt(a_native->IsAutoAdd()?1:0);
	return true;
}

bool gmScriptGoal::setAutoAdd( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_operands[1].IsInt())
		a_native->SetAutoAdd(a_operands[1].GetInt()!=0);
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool gmScriptGoal::getInitializeFunc( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native->GetCallback(ScriptGoal::ON_INIT))
		a_operands[0].SetFunction(a_native->GetCallback(ScriptGoal::ON_INIT));
	else 
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setInitializeFunc( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmFunctionObject *pFunc = a_operands[1].GetFunctionObjectSafe();
	if(pFunc)
		a_native->SetCallback(ScriptGoal::ON_INIT, gmGCRoot<gmFunctionObject>(pFunc, a_thread->GetMachine()));
	return true;
}

bool gmScriptGoal::getSpawnFunc( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native->GetCallback(ScriptGoal::ON_SPAWN))
		a_operands[0].SetFunction(a_native->GetCallback(ScriptGoal::ON_SPAWN));
	else 
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setSpawnFunc( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmFunctionObject *pFunc = a_operands[1].GetFunctionObjectSafe();
	if(pFunc)
		a_native->SetCallback(ScriptGoal::ON_SPAWN, gmGCRoot<gmFunctionObject>(pFunc, a_thread->GetMachine()));
	return true;
}

bool gmScriptGoal::getPriorityFunc( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native->GetCallback(ScriptGoal::ON_GETPRIORITY))
		a_operands[0].SetFunction(a_native->GetCallback(ScriptGoal::ON_GETPRIORITY));
	else 
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setPriorityFunc( ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmFunctionObject *pFunc = a_operands[1].GetFunctionObjectSafe();
	if(pFunc)
		a_native->SetCallback(ScriptGoal::ON_GETPRIORITY, gmGCRoot<gmFunctionObject>(pFunc, a_thread->GetMachine()));
	return true;
}

bool gmScriptGoal::getEnterFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_native->GetCallback(ScriptGoal::ON_ENTER))
		a_operands[0].SetFunction(a_native->GetCallback(ScriptGoal::ON_ENTER));
	else 
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setEnterFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	gmFunctionObject *pFunc = a_operands[1].GetFunctionObjectSafe();
	if(pFunc)
		a_native->SetCallback(ScriptGoal::ON_ENTER, gmGCRoot<gmFunctionObject>(pFunc, a_thread->GetMachine()));
	return true;
}

bool gmScriptGoal::getExitFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_native->GetCallback(ScriptGoal::ON_EXIT))
		a_operands[0].SetFunction(a_native->GetCallback(ScriptGoal::ON_EXIT));
	else 
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setExitFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	gmFunctionObject *pFunc = a_operands[1].GetFunctionObjectSafe();
	if(pFunc)
		a_native->SetCallback(ScriptGoal::ON_EXIT, gmGCRoot<gmFunctionObject>(pFunc, a_thread->GetMachine()));
	return true;
}

bool gmScriptGoal::getUpdateFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_native->GetCallback(ScriptGoal::ON_UPDATE))
		a_operands[0].SetFunction(a_native->GetCallback(ScriptGoal::ON_UPDATE));
	else 
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setUpdateFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	gmFunctionObject *pFunc = a_operands[1].GetFunctionObjectSafe();
	if(pFunc)
		a_native->SetCallback(ScriptGoal::ON_UPDATE, gmGCRoot<gmFunctionObject>(pFunc, a_thread->GetMachine()));
	return true;
}

bool gmScriptGoal::getPathThroughFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_native->GetCallback(ScriptGoal::ON_PATH_THROUGH))
		a_operands[0].SetFunction(a_native->GetCallback(ScriptGoal::ON_PATH_THROUGH));
	else 
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setPathThroughFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	gmFunctionObject *pFunc = a_operands[1].GetFunctionObjectSafe();
	if(pFunc)
		a_native->SetCallback(ScriptGoal::ON_PATH_THROUGH, gmGCRoot<gmFunctionObject>(pFunc, a_thread->GetMachine()));
	return true;
}

bool gmScriptGoal::getAimVectorFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetVector(a_native->GetAimVector());
	return true;
}

bool gmScriptGoal::setAimVectorFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	Vector3f v;
	if(a_operands[1].IsVector() && a_operands[1].GetVector(v.x,v.y,v.z))	
		a_native->SetAimVector(v);
	return true;
}

bool gmScriptGoal::getAimWeaponIdFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->GetAimWeaponId());
	return true;
}

bool gmScriptGoal::setGetPriorityDelay(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsNumber())	
		a_native->SetGetPriorityDelay(Utils::SecondsToMilliseconds(a_operands[1].GetFloatSafe()));
	return true;
}

bool gmScriptGoal::getGetPriorityDelay(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetFloat(Utils::MillisecondsToSeconds(a_native->GetGetPriorityDelay()));
	return true;
}

bool gmScriptGoal::setAimWeaponIdFunc(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->SetAimWeaponId(a_operands[1].GetInt());
	return true;
}

bool gmScriptGoal::getAutoReleaseAim(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->AutoReleaseAim());
	return true;
}

bool gmScriptGoal::setAutoReleaseAim(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->AutoReleaseAim(a_operands[1].GetInt()!=0);
	return true;
}

bool gmScriptGoal::getAutoReleaseWeapon(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->AutoReleaseWeapon());
	return true;
}

bool gmScriptGoal::setAutoReleaseWeapon(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->AutoReleaseWeapon(a_operands[1].GetInt()!=0);
	return true;
}

bool gmScriptGoal::getAutoReleaseTrackers(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->AutoReleaseTracker());
	return true;
}

bool gmScriptGoal::setAutoReleaseTrackers(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->AutoReleaseTracker(a_operands[1].GetInt()!=0);
	return true;
}

bool gmScriptGoal::getAutoFinishOnUnavailable(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->AutoFinishOnUnAvailable());
	return true;
}

bool gmScriptGoal::setAutoFinishOnUnavailable(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->AutoFinishOnUnAvailable(a_operands[1].GetInt()!=0);
	return true;
}

bool gmScriptGoal::getAutoFinishOnNoProgressSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->AutoFinishOnNoProgressSlots());
	return true;
}

bool gmScriptGoal::setAutoFinishOnNoProgressSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->AutoFinishOnNoProgressSlots(a_operands[1].GetInt()!=0);
	return true;
}

bool gmScriptGoal::getAutoFinishOnNoUseSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->AutoFinishOnNoUseSlots());
	return true;
}

bool gmScriptGoal::setAutoFinishOnNoUseSlots(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->AutoFinishOnNoUseSlots(a_operands[1].GetInt()!=0);
	return true;
}

bool gmScriptGoal::getSkipGetPriorityWhenActive(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetInt(a_native->SkipGetPriorityWhenActive());
	return true;
}
bool gmScriptGoal::setSkipGetPriorityWhenActive(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())	
		a_native->SkipGetPriorityWhenActive(a_operands[1].GetInt()!=0);
	return true;
}
bool gmScriptGoal::getScriptPriority(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_operands[0].SetFloat(a_native->GetScriptPriority());
	return true;
}

bool gmScriptGoal::setScriptPriority(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsNumber())	
		a_native->SetScriptPriority(a_operands[1].GetFloatSafe());
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool gmScriptGoal::getMapGoal(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	if(a_native->GetMapGoal())
		a_operands[0].SetUser(a_native->GetMapGoal()->GetScriptObject(a_thread->GetMachine()));
	else
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setMapGoal(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsNull())
	{
		MapGoalPtr g;
		a_native->SetMapGoal(g);
		return true;
	}

	MapGoal *Mg = 0;
	if(gmBind2::Class<MapGoal>::FromVar(a_thread,a_operands[1],Mg) && Mg)
	{
		MapGoalPtr mg = Mg->GetSmartPtr();
		a_native->SetMapGoal(mg);
		return true;
	}
	return true;
}

bool gmScriptGoal::getAlwaysRecieveEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	a_operands[0].SetInt(a_native->AlwaysRecieveEvents()?1:0);
	return true;
}

bool gmScriptGoal::setAlwaysRecieveEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	if(a_operands[1].IsInt())
		a_native->SetAlwaysRecieveEvents(a_operands[1].GetInt()!=0);
	return true;
}

bool gmScriptGoal::getEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	a_operands[0].SetTable(a_native->m_EventTable);
	return true;
}

bool gmScriptGoal::setEvents(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	gmTableObject *pTbl = a_operands[1].GetTableObjectSafe();
	if(pTbl)
		a_native->m_EventTable.Set(pTbl, a_thread->GetMachine());
	return true;
}

bool gmScriptGoal::getCommands(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	a_operands[0].SetTable(a_native->m_CommandTable);
	return true;
}

bool gmScriptGoal::setCommands(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	gmTableObject *pTbl = a_operands[1].GetTableObjectSafe();
	if(pTbl)
		a_native->m_CommandTable.Set(pTbl, a_thread->GetMachine());
	return true;
}

bool gmScriptGoal::getDebugString(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	if(a_native->DebugString())
		a_operands[0].SetString(a_native->DebugString());
	else
		a_operands[0].Nullify();
	return true;
}

bool gmScriptGoal::setDebugString(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
	if(pStr)
		a_native->DebugString().Set(pStr, a_thread->GetMachine());
	return true;
}

bool gmScriptGoal::getDebug(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	a_operands[0].SetInt(a_native->DebugDrawingEnabled());
	return true;
}

bool gmScriptGoal::setDebug(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{
	a_native->DebugDraw(a_operands[1].GetInt(0)!=0);
	return true;
}

bool gmScriptGoal::getBot(AiState::ScriptGoal *a_native, gmThread *a_thread, gmVariable *a_operands)
{	
	a_operands[0].SetUser(a_native->GetClient()->GetScriptObject());
	return true;
}

//////////////////////////////////////////////////////////////////////////
