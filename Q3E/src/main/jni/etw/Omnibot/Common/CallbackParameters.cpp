#include "PrecompCommon.h"
#include "CallbackParameters.h"

#include "gmMachine.h"
#include "gmTableObject.h"

CallbackParameters::CallbackParameters(int _messageId, gmMachine *_machine) 
	: m_MessageId(_messageId)
	, m_NumParameters(0)
	, m_Machine(_machine)
	, m_MessageName(0)
	, m_TargetState(0)
	, m_ShouldCallScript(false)
	, m_CallImmediate(false)
	, m_PropogateEvent(true)
{
	for(int i = 0; i < MaxVariables; ++i)
	{
		m_Variables[i] = gmVariable::s_null;
	}
}

void CallbackParameters::DebugName(const char *_name)
{
	m_MessageName = _name;
}

void CallbackParameters::PrintDebug()
{
#if(DEBUG_PARAMS)
	Utils::OutputDebug(kInfo, "- Message: %s %d\n", m_MessageName ? m_MessageName : "<unknown>", GetMessageId());
	for(int i = 0; i < m_NumParameters; ++i)
	{
		switch(m_Variables[i].m_type)
		{
		case GM_NULL:
			{
				EngineFuncs::ConsoleMessage(va("	+ Null: %s\n", m_DebugNames[i]));
				break;
			}
		case GM_INT:
			{
				EngineFuncs::ConsoleMessage(va("	+ Int: %s, %d\n", m_DebugNames[i], m_Variables[i].GetInt()));
				break;
			}
		case GM_FLOAT:
			{
				EngineFuncs::ConsoleMessage(va("	+ Float: %s, %f\n", m_DebugNames[i], m_Variables[i].GetFloat()));
				break;
			}
		case GM_VEC3:
			{
				float x,y,z;
				m_Variables[i].GetVector(x,y,z);
				EngineFuncs::ConsoleMessage(va("	+ Vector: %s, (%f, %f, %f)\n", m_DebugNames[i], x,y,z));
				break;
			}
		case GM_ENTITY:
			{
				EngineFuncs::ConsoleMessage(va("	+ Entity: %s, %X\n", m_DebugNames[i], m_Variables[i].GetEntity()));
				break;
			}
		case GM_STRING:
			{
				gmStringObject *pStr = m_Variables[i].GetStringObjectSafe();
				EngineFuncs::ConsoleMessage(va("	+ String: %s, \"%s\"\n", m_DebugNames[i], pStr->GetString()));
				break;
			}
		case GM_TABLE:
			{
				EngineFuncs::ConsoleMessage(va("	+ Table: %s, %X\n", m_DebugNames[i], m_Variables[i].GetTableObjectSafe()));
				break;
			}
		case GM_FUNCTION:
			{
				gmFunctionObject *pFunc = m_Variables[i].GetFunctionObjectSafe();
				EngineFuncs::ConsoleMessage(va("	+ Func: %s, %s\n", m_DebugNames[i], pFunc->GetDebugName()));
				break;
			}
		default:
			{				
				EngineFuncs::ConsoleMessage(va("	+ UserObj: %s, %X\n", m_DebugNames[i], m_Variables[i].m_value.m_ref));
			}
		}
	}
#endif
}

void CallbackParameters::CheckParameters()
{
	OBASSERT(m_Machine,"No Machine Specified!");
	OBASSERT(m_NumParameters<MaxVariables-1, "Out of Parameters!");
}

void CallbackParameters::AddNull(const char *_name)
{
	CheckParameters();
	m_Variables[m_NumParameters].Nullify();
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

void CallbackParameters::AddInt(const char *_name, int _param)
{
	CheckParameters();
	m_Variables[m_NumParameters].SetInt(_param);
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

void CallbackParameters::AddFloat(const char *_name, float _param)
{
	CheckParameters();
	m_Variables[m_NumParameters].SetFloat(_param);
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

void CallbackParameters::AddVector(const char *_name, const Vector3f &_vec)
{
	AddVector(_name, _vec.x, _vec.y, _vec.z);
}

void CallbackParameters::AddVector(const char *_name, float _x, float _y, float _z)
{
	CheckParameters();
	m_Variables[m_NumParameters].SetVector(_x, _y, _z);
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

void CallbackParameters::AddEntity(const char *_name, GameEntity _param)
{
	if(!_param.IsValid())
	{
		AddNull(_name);
		return;
	}

	CheckParameters();
	m_Variables[m_NumParameters].SetEntity(_param.AsInt());
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

void CallbackParameters::AddUserObj(const char *_name, gmUserObject *_param)
{
	CheckParameters();
	m_Variables[m_NumParameters].SetUser(_param);
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

void CallbackParameters::AddString(const char *_name, const char *_param)
{
	CheckParameters();
	m_Variables[m_NumParameters].SetString(m_Machine->AllocStringObject(_param?_param:"<unknown>"));
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

void CallbackParameters::AddTable(const char *_name, gmTableObject *_param)
{
	CheckParameters();
	m_Variables[m_NumParameters].SetTable(_param);
#if(DEBUG_PARAMS)
	m_DebugNames[m_NumParameters] = _name;
#endif
	m_NumParameters++;
}

int CallbackParameters::CallFunction(gmFunctionObject *_func,
									  const gmVariable &a_thisVar, 
									  bool a_delayExecuteFlag) const
{
	gmCall call;
	if(call.BeginFunction(m_Machine, _func, a_thisVar, a_delayExecuteFlag))
	{
		for(int i = 0; i < m_NumParameters; ++i)
			call.AddParam(m_Variables[i]);
		call.End();

#if(DEBUG_PARAMS)
		const char *pName = _func->GetDebugName();
		Utils::OutputDebug(kInfo, "	Func %s Params: %d\n", pName ? pName : "<unknown>", m_NumParameters);
#endif
	}
	return call.GetThreadId();
}