#include "PrecompCommon.h"
#include "BotBaseStates.h"
#include "ScriptManager.h"
#include "ScriptGoal.h"

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////
	int ScriptGoal::gmfFinished(gmThread *a_thread)
	{
		SetFinished();
		return GM_SYS_KILL; 
	}
	int ScriptGoal::gmfGoto(gmThread *a_thread)
	{
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

		if(Goto(Vector3f(v.x,v.y,v.z), opn))
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
	int ScriptGoal::gmfGotoAsync(gmThread *a_thread)
	{
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

		bool b = Goto(Vector3f(v.x,v.y,v.z), opn);
		a_thread->Push(gmVariable(b?1:0));
		return GM_OK; 
	}
	int ScriptGoal::gmfAddAimRequest(gmThread *a_thread)
	{
		GM_CHECK_NUM_PARAMS(1);
		GM_CHECK_INT_PARAM(priority, 0);
		GM_STRING_PARAM(aimtype, 1, NULL);
		GM_VECTOR_PARAM(v, 2, 0.f, 0.f, 0.f);

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
		if(!AddScriptAimRequest(prio,eAimType,Vector3f(v.x, v.y, v.z)))
		{
			GM_EXCEPTION_MSG("Unable to add aim request. Too many!");
			return GM_EXCEPTION;
		}
		return GM_OK;
	}
	int ScriptGoal::gmfReleaseAimRequest(gmThread *a_thread)
	{
		FINDSTATE(aim, Aimer, GetClient()->GetStateRoot());
		if(aim)
			aim->ReleaseAimRequest(GetNameHash());
		return GM_OK;
	}
	int ScriptGoal::gmfAddWeaponRequest(gmThread *a_thread)
	{
		GM_CHECK_NUM_PARAMS(2);
		GM_CHECK_INT_PARAM(priority, 0);
		GM_CHECK_INT_PARAM(weaponId, 1);

		bool bSuccess = false;

		Priority::ePriority prio = (Priority::ePriority)priority;
		FINDSTATE(weapsys, WeaponSystem, GetClient()->GetStateRoot());
		if(weapsys)
			bSuccess = weapsys->AddWeaponRequest(prio, GetNameHash(), weaponId);

		if(!bSuccess)
		{
			GM_EXCEPTION_MSG("Unable to add weapon request. Too many!");
			return GM_EXCEPTION;
		}
		return GM_OK;
	}
	int ScriptGoal::gmfReleaseWeaponRequest(gmThread *a_thread)
	{
		FINDSTATE(weapsys, WeaponSystem, GetClient()->GetStateRoot());
		if(weapsys)
			weapsys->ReleaseWeaponRequest(GetNameHash());
		return GM_OK;
	}

	int ScriptGoal::gmfUpdateWeaponRequest(gmThread *a_thread)
	{
		GM_CHECK_NUM_PARAMS(1);
		GM_CHECK_INT_PARAM(weaponId, 0);

		bool bSuccess = false;

		FINDSTATE(weapsys, WeaponSystem, GetClient()->GetStateRoot());
		if(weapsys)
			bSuccess = weapsys->UpdateWeaponRequest(GetNameHash(), weaponId);

		if(!bSuccess)
		{
			GM_EXCEPTION_MSG("Unable to update weapon request. Not Found!");
			return GM_EXCEPTION;
		}
		return GM_OK;
	}

	int ScriptGoal::gmfBlockForWeaponChange(gmThread *a_thread)
	{
		GM_CHECK_INT_PARAM(weaponId, 0);

		gmVariable varSig(Utils::MakeId32((obint16)ACTION_WEAPON_CHANGE, (obint16)weaponId));

		AiState::WeaponSystem *ws = GetClient()->GetWeaponSystem();
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

	int ScriptGoal::gmfBlockForWeaponFire(gmThread *a_thread)
	{
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

	int ScriptGoal::gmfBlockForVoiceMacro(gmThread *a_thread)
	{
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

	int ScriptGoal::gmfThreadFork(gmThread *a_thread)
	{
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

		AddForkThreadId(threadId);
		a_thread->PushInt(threadId);
		return GM_OK;
	}

	int ScriptGoal::gmfSignal(gmThread *a_thread)
	{
		for(int i = 0; i < a_thread->GetNumParams(); ++i)
			Signal(a_thread->Param(i));
		return GM_OK;
	}

	//////////////////////////////////////////////////////////////////////////

	void BindScriptGoal(gmMachine *a_machine)
	{
		gmBind2::Class<ScriptGoal>("ScriptGoal",a_machine)
			.base<State>()
			.func(&ScriptGoal::gmfFinished,					"Finished")
			.func(&ScriptGoal::DidPathSucceed,				"DidPathSucceed")
			.func(&ScriptGoal::DidPathFail,					"DidPathFail")
			.func(&ScriptGoal::gmfGoto,						"Goto")
			.func(&ScriptGoal::gmfGotoAsync,				"GotoAsync")
			.func(&ScriptGoal::gmfBlockForWeaponChange,		"BlockForWeaponChange")
			.func(&ScriptGoal::gmfBlockForWeaponFire,		"BlockForWeaponFire")
			.func(&ScriptGoal::gmfBlockForVoiceMacro,		"BlockForVoiceMacro")
			.func(&ScriptGoal::gmfThreadFork,				"ForkThread")
			//.var()
			;
	}

}
