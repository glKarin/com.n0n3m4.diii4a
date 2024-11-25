/*
_____               __  ___          __            ____        _      __
/ ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
/ (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
\___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
/___/             /_/

See Copyright Notice in gmMachine.h

*/

#include "gmCall.h"
#include "gmConfig.h"
#include "gmThread.h"
#include "gmMachine.h"

gmCall::gmCall()
	: m_machine(0)
	, m_thread(0)
	, m_returnVar(gmVariable::s_null)
	, m_paramCount(0) 
	, m_threadId(GM_INVALID_THREAD)
	, m_delayExecuteFlag(false)
#ifdef GM_DEBUG_BUILD
	, m_locked(false)
#endif //GM_DEBUG_BUILD
{
}

bool gmCall::BeginGlobalFunction(gmMachine * a_machine, 
								 const char * a_funcName, 
								 const gmVariable& a_this, 
								 bool a_delayExecuteFlag,
								 gmuint8 a_priority)
{
	GM_ASSERT(a_machine);

	gmStringObject * funcNameStringObj = a_machine->AllocPermanantStringObject(a_funcName); // Slow

	return BeginGlobalFunction(a_machine, funcNameStringObj, a_this, a_delayExecuteFlag, a_priority);
}

bool gmCall::BeginGlobalFunction(gmMachine * a_machine, 
								 gmStringObject * a_funcNameStringObj, 
								 const gmVariable& a_this, 
								 bool a_delayExecuteFlag,
								 gmuint8 a_priority)
{
	GM_ASSERT(a_machine);
	GM_ASSERT(a_funcNameStringObj);

	gmVariable lookUpVar;
	gmVariable foundFunc;

	lookUpVar.SetString(a_funcNameStringObj);
	foundFunc = a_machine->GetGlobals()->Get(lookUpVar);

	if( GM_FUNCTION == foundFunc.m_type )         // Check found variable is a function
	{
		gmFunctionObject * functionObj = (gmFunctionObject *)foundFunc.m_value.m_ref; //Func Obj from variable

		return BeginFunction(a_machine, functionObj, a_this, a_delayExecuteFlag, a_priority);
	}
	return false;
}

bool gmCall::BeginTableFunction(gmMachine * a_machine, 
								const char * a_funcName, 
								const char * a_tableName, 
								const gmVariable& a_this, 
								bool a_delayExecuteFlag,
								gmuint8 a_priority)
{
	GM_ASSERT(a_machine);
	GM_ASSERT(a_funcName);

	gmVariable tablevar = a_machine->GetGlobals()->Get(a_machine, a_tableName);
	if(tablevar.GetTableObjectSafe())
	{
		gmStringObject * funcNameStringObj = a_machine->AllocPermanantStringObject(a_funcName); // Slow

		return BeginTableFunction(
			a_machine, 
			funcNameStringObj, 
			tablevar.GetTableObjectSafe(), 
			a_this, 
			a_delayExecuteFlag,
			a_priority);
	}
	return false;
}

bool gmCall::BeginTableFunction(gmMachine * a_machine, 
								const char * a_funcName, 
								gmTableObject * a_tableObj,
								const gmVariable& a_this, 
								bool a_delayExecuteFlag,
								gmuint8 a_priority)
{
	GM_ASSERT(a_machine);
	GM_ASSERT(a_funcName);

	gmStringObject * funcNameStringObj = a_machine->AllocPermanantStringObject(a_funcName); // Slow

	return BeginTableFunction(
		a_machine, 
		funcNameStringObj, 
		a_tableObj, 
		a_this, 
		a_delayExecuteFlag, 
		a_priority);
}

bool gmCall::BeginTableFunction(gmMachine * a_machine, 
								gmStringObject * a_funcNameStringObj, 
								gmTableObject * a_tableObj, 
								const gmVariable& a_this, 
								bool a_delayExecuteFlag,
								gmuint8 a_priority)
{
	GM_ASSERT(a_machine);   
	GM_ASSERT(a_tableObj);
	GM_ASSERT(a_funcNameStringObj);

	gmVariable lookUpVar;
	gmVariable foundFunc;

	lookUpVar.SetString(a_funcNameStringObj);
	foundFunc = a_tableObj->Get(lookUpVar);

	if( GM_FUNCTION == foundFunc.m_type )         // Check found variable is a function
	{
		gmFunctionObject * functionObj = (gmFunctionObject *)foundFunc.m_value.m_ref; //Func Obj from variable
		return BeginFunction(a_machine, functionObj, a_this, a_delayExecuteFlag, a_priority);
	}
	return false;
}

bool gmCall::BeginFunction(gmMachine * a_machine, 
						   gmFunctionObject * a_funcObj, 
						   const gmVariable &a_thisVar, 
						   bool a_delayExecuteFlag,
						   gmuint8 a_priority)
{
	GM_ASSERT(a_machine);   
	GM_ASSERT(a_funcObj);

#ifdef GM_DEBUG_BUILD
	// YOU CANNOT NEST gmCall::Begin
	GM_ASSERT(m_locked == false);
	m_locked = true;
#endif //GM_DEBUG_BUILD

	Reset(a_machine);

	if( GM_FUNCTION == a_funcObj->GetType() )         // Check found variable is a function
	{
		int threadId = GM_INVALID_THREAD;
		m_thread = m_machine->CreateThread(&threadId,a_priority);     // Create thread for func to run on      
		m_thread->Push(a_thisVar);                // this
		m_thread->PushFunction(a_funcObj);        // function
		m_delayExecuteFlag = a_delayExecuteFlag;
		return true;
	}

#ifdef GM_DEBUG_BUILD
	GM_ASSERT(m_locked == true);
	m_locked = false;
#endif //GM_DEBUG_BUILD

	return false;
}

void gmCall::AddParam(const gmVariable& a_var)
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

	m_thread->Push(a_var);
	++m_paramCount;
}

void gmCall::AddParamNull()
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

	m_thread->PushNull();
	++m_paramCount;
}

void gmCall::AddParamInt(const int a_value)
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

	m_thread->PushInt(a_value);
	++m_paramCount;
}

void gmCall::AddParamFloat(const float a_value)
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

	m_thread->PushFloat(a_value);
	++m_paramCount;
}

void gmCall::AddParamString(const char * a_value, int a_len)
{
	GM_ASSERT(m_machine);    
	GM_ASSERT(m_thread);

	m_thread->PushNewString(a_value, a_len);
	++m_paramCount;
}

void gmCall::AddParamString(gmStringObject * a_value)
{
	GM_ASSERT(m_machine);    
	GM_ASSERT(m_thread);

	m_thread->PushString(a_value);
	++m_paramCount;
}

void gmCall::AddParamUser(void * a_value, int a_userType)
{
	GM_ASSERT(m_machine);    
	GM_ASSERT(m_thread);

	m_thread->PushNewUser(a_value, a_userType);
	++m_paramCount;
}

void gmCall::AddParamUser(gmUserObject * a_userObj)
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

	m_thread->PushUser(a_userObj);
	++m_paramCount;
}

void gmCall::AddParamTable(gmTableObject * a_tableObj)
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

	m_thread->PushTable(a_tableObj);
	++m_paramCount;
}

#if(GM_USE_VECTOR3_STACK)
void gmCall::AddParamVector(const Vec3 &vec) 
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);
	m_thread->PushVector(ConvertVec3(vec));
	++m_paramCount;
}
bool gmCall::GetReturnedVector(Vec3 &a_value) 
{
	if (m_returnFlag && m_returnVar.GetVector(a_value.x,a_value.y,a_value.z))
		return true;
	return false;
}
#endif

#if(GM_USE_ENTITY_STACK)
void gmCall::AddParamEntity(int a_value)
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

	m_thread->PushEntity(a_value);
	++m_paramCount;
}
bool gmCall::GetReturnedEntity(int &a_enthndl)
{
	if (m_returnFlag && m_returnVar.IsEntity())
	{
		a_enthndl = m_returnVar.GetEntity();
		return true;
	}
	return false;
}
#endif

int gmCall::End()
{
	GM_ASSERT(m_machine);
	GM_ASSERT(m_thread);

#ifdef GM_DEBUG_BUILD
	// CAN ONLY CALL ::End() after a successful ::Begin
	GM_ASSERT(m_locked == true);
	m_locked = false;
#endif //GM_DEBUG_BUILD

	int state = m_thread->PushStackFrame(m_paramCount);
	if(state != gmThread::KILLED) // Can be killed immedialy if it was a C function
	{
		if(m_delayExecuteFlag)
		{
			state = m_thread->GetState();
		}
		else
		{
			state = m_thread->Sys_Execute(&m_returnVar);
		}
	}
	else
	{
		// Was a C function call, grab return var off top of stack
		m_returnVar = *(m_thread->GetTop() - 1);
		m_machine->Sys_SwitchState(m_thread, gmThread::KILLED);
	}

	// If we requested a thread Id
	if(state != gmThread::KILLED)
		m_threadId = m_thread->GetId();
	else
		m_threadId = GM_INVALID_THREAD;

	if(state == gmThread::KILLED)
	{
		m_returnFlag = true; // Function always returns something, null if not explicit.
		m_thread = NULL; // Thread has exited, no need to remember it.
	}
	return state;
}

bool gmCall::GetReturnedNull()
{
	if(DidReturnVariable() && (m_returnVar.m_type == GM_NULL))
	{
		return true;
	}
	return false;
}

bool gmCall::GetReturnedInt(int& a_value)
{
	if(DidReturnVariable() && (m_returnVar.m_type == GM_INT))
	{
		a_value = m_returnVar.m_value.m_int;
		return true;
	}
	return false;
}

bool gmCall::GetReturnedFloat(float& a_value)
{
	if(DidReturnVariable() && (m_returnVar.m_type == GM_FLOAT))
	{
		a_value = m_returnVar.m_value.m_float;
		return true;
	}
	return false;
}

bool gmCall::GetReturnedString(const char *& a_value)
{
	if(DidReturnVariable() && (m_returnVar.m_type == GM_STRING))
	{
		a_value = ((gmStringObject *)m_machine->GetGMObject(m_returnVar.m_value.m_ref))->GetString();
		return true;
	}
	return false;
}

bool gmCall::GetReturnedTable(gmTableObject *& a_value)
{
	if(DidReturnVariable() && (m_returnVar.m_type == GM_TABLE))
	{
		a_value = m_returnVar.GetTableObjectSafe();
		return true;
	}
	return false;
}

bool gmCall::GetReturnedUser(void *& a_value, int a_userType)
{
	if(DidReturnVariable() && (m_returnVar.m_type == a_userType))
	{
		a_value = (void *)m_returnVar.m_value.m_ref;
		return true;
	}
	return false;
}

bool gmCall::GetReturnedUserOrNull(void *& a_value, int a_userType)
{
	if(DidReturnVariable())
	{
		if(m_returnVar.m_type == a_userType)
		{
			a_value = (void *)m_returnVar.m_value.m_ref;
			return true;
		}
		else if(m_returnVar.m_type == GM_NULL)
		{
			a_value = (void *)NULL;
			return true;
		}
	}
	return false;
}
