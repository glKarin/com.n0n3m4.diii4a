#ifndef __CALLBACKPARAMETERS_H__
#define __CALLBACKPARAMETERS_H__

class Client;
class gmMachine;
class gmUserObject;
class gmTableObject;
class gmFunctionObject;
#include "gmVariable.h"

#define DEBUG_PARAMS 0

class CallbackParameters
{
public:
	enum { MaxVariables = 8 };
	
	void CallScript(bool _immediate = false) { m_ShouldCallScript = true; m_CallImmediate = _immediate; }
	bool CallImmediate() const { return m_CallImmediate; }
	void DontPropogateEvent() { m_PropogateEvent = false; }
	bool ShouldCallScript() const { return m_ShouldCallScript; }
	bool ShouldPropogateEvent() const { return m_PropogateEvent; }
	void DebugName(const char *_name);
	void PrintDebug();
	void SetTargetState(obuint32 _ts) { m_TargetState = _ts; }
	obuint32 GetTargetState() const { return m_TargetState; }
	int GetMessageId() const { return m_MessageId; }
	gmMachine *GetMachine() const { return m_Machine; }

	void ResetParams() { m_NumParameters = 0; }

	void AddNull(const char *_name);
	void AddInt(const char *_name, int _param);
	void AddFloat(const char *_name, float _param);
	void AddVector(const char *_name, const Vector3f &_vec);
	void AddVector(const char *_name, float _x, float _y, float _z);
	void AddEntity(const char *_name, GameEntity _param);
	void AddUserObj(const char *_name, gmUserObject *_param);
	void AddString(const char *_name, const char *_param);
	void AddTable(const char *_name, gmTableObject *_param);

	int CallFunction(gmFunctionObject *_func,
		const gmVariable &a_thisVar = gmVariable::s_null, 
		bool a_delayExecuteFlag = false) const;

	CallbackParameters(int _messageId, gmMachine *_machine);
private:
	int			m_MessageId;
	int			m_NumParameters;
	gmMachine	*m_Machine;
	const char	*m_MessageName;
	obuint32	m_TargetState;

	gmVariable		m_Variables[MaxVariables];

#if(DEBUG_PARAMS)
	const char*		m_DebugNames[MaxVariables];
#endif

	bool		m_ShouldCallScript;
	bool		m_CallImmediate;
	bool		m_PropogateEvent;

	void CheckParameters();

	CallbackParameters();
};

#endif
