/*
_____               __  ___          __            ____        _      __
/ ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
/ (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
\___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
/___/             /_/

See Copyright Notice in gmMachine.h

*/

#ifndef _GMCALL_H_
#define _GMCALL_H_

#include "gmConfig.h"
#include "gmThread.h"
#include "gmMachine.h"

#if(GM_USE_VECTOR3_STACK)
#include "mathlib/vector.h"
#endif

/// \class gmCall
/// \brief A helper class to call script functions from C
/// Warning: Do not store any of the reference type return variables (eg. GM_SRING).  
/// As the object may be garbage collected.  Instead, copy immediately as necessary.
class gmCall
{
public:

	/// \brief Constructor
	gmCall();

	/// \brief Begin the call of a global function
	/// \param a_machine Virtual machine instance 
	/// \param a_funcName Name of function
	/// \param a_this The 'this' used by the function.
	/// \param a_delayExecuteFlag Set true if you want function thread to not execute now.
	/// \return true on sucess, false if function was not found.
	bool BeginGlobalFunction(
		gmMachine * a_machine, 
		const char * a_funcName, 
		const gmVariable& a_this = gmVariable::s_null, 
		bool a_delayExecuteFlag = false,
		gmuint8 a_priority = gmThread::None);

	/// \brief Begin the call of a global function
	/// \param a_machine Virtual machine instance 
	/// \param a_funcNameStringObj A string object that was found or created earlier, much faster than creating from c string.
	/// \param a_this The 'this' used by the function.
	/// \param a_delayExecuteFlag Set true if you want function thread to not execute now.
	/// \return true on sucess, false if function was not found.
	bool BeginGlobalFunction(
		gmMachine * a_machine, 
		gmStringObject * a_funcNameStringObj,
		const gmVariable& a_this = gmVariable::s_null, 
		bool a_delayExecuteFlag = false,
		gmuint8 a_priority = gmThread::None);

	bool BeginTableFunction(
		gmMachine * a_machine,
		const char * a_funcName, 
		const char * a_tableName,
		const gmVariable& a_this = gmVariable::s_null, 
		bool a_delayExecuteFlag = false,
		gmuint8 a_priority = gmThread::None);

	/// \brief Begin the call of a object function
	/// \param a_machine Virtual machine instance 
	/// \param a_funcName Name of function.
	/// \param a_tableObj The table on the object to look up the function.
	/// \param a_this The 'this' used by the function.
	/// \param a_delayExecuteFlag Set true if you want function thread to not execute now.
	/// \return true on sucess, false if function was not found.
	bool BeginTableFunction(
		gmMachine * a_machine, const char * a_funcName,
		gmTableObject * a_tableObj,
		const gmVariable& a_this = gmVariable::s_null, 
		bool a_delayExecuteFlag = false,
		gmuint8 a_priority = gmThread::None);

	/// \brief Begin the call of a object function
	/// \param a_machine Virtual machine instance 
	/// \param a_funcNameStringObj A string object that was found or created earlier, much faster than creating from c string.
	/// \param a_tableObj The table on the object to look up the function.
	/// \param a_this The 'this' used by the function.
	/// \param a_delayExecuteFlag Set true if you want function thread to not execute now.
	/// \return true on sucess, false if function was not found.
	bool BeginTableFunction(
		gmMachine * a_machine, 
		gmStringObject * a_funcNameStringObj, 
		gmTableObject * a_tableObj, 
		const gmVariable& a_this = gmVariable::s_null,
		bool a_delayExecuteFlag = false,
		gmuint8 a_priority = gmThread::None);

	/// \brief Begin the call of a object function
	/// \param a_funcObj A function object that was found or created earlier.
	/// \param a_tableObj The table on the object to look up the function.
	/// \param a_this The 'this' used by the function.
	/// \param a_delayExecuteFlag Set true if you want function thread to not execute now.
	/// \return true on sucess, false if function was not found.
	bool BeginFunction(
		gmMachine * a_machine, 
		gmFunctionObject * a_funcObj, 
		const gmVariable &a_thisVar = gmVariable::s_null, 
		bool a_delayExecuteFlag = false,
		gmuint8 a_priority = gmThread::None);

	/// \brief Add a parameter variable
	void AddParam(const gmVariable& a_var);

	/// \brief Add a parameter that is null
	void AddParamNull();

	/// \brief Add a parameter that is a integer
	void AddParamInt(const int a_value);

	/// \brief Add a parameter that is a float
	void AddParamFloat(const float a_value);

	/// \brief Add a parameter that is a string
	void AddParamString(const char * a_value, int a_len = -1);

	/// \brief Add a parameter that is a string (faster version since c string does not need lookup)
	void AddParamString(gmStringObject * a_value);

	/// \brief Add a parameter that is a user object.  Creates a new user object.
	/// \param a_value Pointer to user object data
	/// \param a_userType Type of user object beyond GM_USER
	void AddParamUser(void * a_value, int a_userType);

	/// \brief Add a parameter that is a user object.
	/// \param a_userObj Pushes an existing user object without creating a new one.
	void AddParamUser(gmUserObject * a_userObj);

	/// \brief Add a parameter that is a table object.
	/// \param a_tableObj Pushes an existing table object without creating a new one.
	void AddParamTable(gmTableObject * a_tableObj);

	/// \brief Make the call.  If a return value was expected, it will be set in here.
	/// \param
	int End();

	/// \brief Accesss thread created for function call.
	gmThread * GetThread() { return m_thread; }

	int GetThreadId() { return m_threadId; }

	/// \brief Returns reference to 'return' variable.  Never fails, but variable may be a 'null' variable if none was returned.
	const gmVariable& GetReturnedVariable() { return m_returnVar; }

	/// \brief Returns true if function exited and returned a variable.
	bool DidReturnVariable() { return m_returnFlag; }

	/// \brief Did function return a null?
	/// \return true if function returned null. 
	bool GetReturnedNull();

	/// \brief Get returned int
	/// \return true if function returned an int. 
	bool GetReturnedInt(int& a_value);

	/// \brief Get returned float
	/// \return true if function returned an float. 
	bool GetReturnedFloat(float& a_value);

	/// \brief Get returned string
	/// \return true if function returned an string. 
	bool GetReturnedString(const char *& a_value);

	/// \brief Get returned table
	/// \return true if function returned an string. 
	bool GetReturnedTable(gmTableObject *& a_value);

	/// \brief Get returned user
	/// \return true if function returned an user. 
	bool GetReturnedUser(void *& a_value, int a_userType);

	/// \brief Get returned user or null
	/// \return true if function returned an user or null. 
	bool GetReturnedUserOrNull(void *& a_value, int a_userType);

#if(GM_USE_VECTOR3_STACK)
	void AddParamVector(const Vec3 &vec);
	bool GetReturnedVector(Vec3 &a_value);
#endif

#if(GM_USE_ENTITY_STACK)
	void AddParamEntity(int a_value);
	bool GetReturnedEntity(int &a_enthndl);
#endif

protected:

	gmMachine * m_machine;
	gmThread * m_thread;
	gmVariable m_returnVar;
	int m_paramCount;
	int m_threadId;
	bool m_returnFlag;
	bool m_delayExecuteFlag;
#ifdef GM_DEBUG_BUILD
	bool m_locked;
#endif //GM_DEBUG_BUILD

	/// \brief Used internally to clear call information.
	GM_FORCEINLINE void Reset(gmMachine * a_machine)
	{
		GM_ASSERT(a_machine);
		m_machine = a_machine;
		m_thread = NULL;
		m_returnVar.Nullify();
		m_returnFlag = false;
		m_paramCount = 0;
		m_delayExecuteFlag = false;
	};

};


#endif // _GMCALL_H_
