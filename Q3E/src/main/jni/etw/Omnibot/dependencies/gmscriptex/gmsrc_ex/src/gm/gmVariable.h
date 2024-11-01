/*
_____               __  ___          __            ____        _      __
/ ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
/ (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
\___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
/___/             /_/

See Copyright Notice in gmMachine.h

*/

#ifndef _GMVARIABLE_H_
#define _GMVARIABLE_H_

#include "gmConfig.h"
#if GM_USE_INCGC
#include "gmIncGC.h"
#endif //GM_USE_INCGC

#if(GM_USE_VECTOR3_STACK)
#include "gmVec3Stack.h"
#endif

#if(GM_USE_ENTITY_STACK)
#include "gmEntity.h"
#endif

#define GM_MARK_PERSIST 0
#define GM_MARK_START 1

// fwd decls
class gmMachine;
class gmStringObject;
class gmTableObject;
class gmFunctionObject;
class gmUserObject;

template <class T>
class gmGCRoot;

/// \enum gmType
/// \brief gmType is an enum of the possible scripting types.
typedef int gmType;

enum
{
  GM_INVALID_TYPE = -1, // Represent invalid typeIds returned by a query, not stored or used otherwise.

	GM_NULL = 0, // GM_NULL must be 0 as i have relied on this in expression testing.
	GM_INT,
	GM_FLOAT,

#if(GM_USE_VECTOR3_STACK)
	//GM_BROADCAST_FLOAT,
	GM_VEC3,
#endif

#if(GM_USE_ENTITY_STACK)
	GM_ENTITY,
#endif

#if(GM_USE_ENUM_SUPPORT)
	GM_ENUM,
#endif

	GM_STRING,
	GM_TABLE,
	GM_FUNCTION,
	GM_USER,     // User types continue from here.

	GM_FORCEINT = GM_MAX_INT,
};

#if(GM_USE_ENUM_SUPPORT)
struct gmEnumData
{
	gmint32		enumType;
	gmint32		enumValue;
};
#endif

/// \struct gmVariable
/// \brief a variable is the basic type passed around on the stack, and used as storage in the symbol tables.
///        A variable is either a reference to a gmObject type, or it is a direct value such as null, int or float.
///        The gm runtime stack operates on gmVariable types.
struct gmVariable
{
	static gmVariable s_null;

	gmType m_type;
	union
	{
		gmint m_int;
		gmfloat m_float;
		gmptr m_ref;
#if(GM_USE_VECTOR3_STACK)
		gmVec3Data m_vec3;
#endif
#if(GM_USE_ENTITY_STACK)
		gmint m_enthndl;
#endif
#if(GM_USE_ENUM_SUPPORT)
		gmEnumData m_enum;
#endif
	} m_value;

	inline gmVariable()
	{
#if GMMACHINE_NULL_VAR_CTOR // Disabled by default
		Nullify();
#endif //GMMACHINE_NULL_VAR_CTOR
	}
	inline gmVariable(gmType a_type, gmptr a_ref) : m_type(a_type) { m_value.m_ref = a_ref; }

	explicit inline gmVariable(int a_val) : m_type(GM_INT) { m_value.m_int = a_val; }
	explicit inline gmVariable(float a_val) : m_type(GM_FLOAT) { m_value.m_float = a_val; }
	explicit inline gmVariable(gmStringObject * a_string) { SetString(a_string); }
	explicit inline gmVariable(gmTableObject * a_table) { SetTable(a_table); }
	explicit inline gmVariable(gmFunctionObject * a_func) { SetFunction(a_func); }
	explicit inline gmVariable(gmUserObject * a_user) { SetUser(a_user); }

#if(GM_USE_VECTOR3_STACK)
	explicit inline gmVariable(const gmVec3Data &a_val)
		: m_type(GM_VEC3)
	{
		SetVector(a_val);
	}
	explicit inline gmVariable(float a_x,float a_y,float a_z)
		: m_type(GM_VEC3)
	{
		SetVector(a_x,a_y,a_z);
	}
	explicit inline gmVariable(const float *a_v)
		: m_type(GM_VEC3)
	{
		SetVector(a_v);
	}
	inline bool IsVector() const
	{
		return m_type==GM_VEC3;
	}
	void SetVector(const gmVec3Data &a_val)
	{
		m_type = GM_VEC3;
		m_value.m_vec3 = a_val;
	}
	void SetVector(float a_x, float a_y, float a_z)
	{
		m_type = GM_VEC3;
		m_value.m_vec3.x = a_x;
		m_value.m_vec3.y = a_y;
		m_value.m_vec3.z = a_z;
	}
	bool GetVector(float &a_x, float &a_y, float &a_z) const
	{
		if(IsVector())
		{
			a_x = m_value.m_vec3.x;
			a_y = m_value.m_vec3.y;
			a_z = m_value.m_vec3.z;
			return true;
		}
		return false;
	}
	void SetVector(const float *a_value)
	{
		SetVector(a_value[0],a_value[1],a_value[2]);
	}
	bool GetVector(float *a_value) const
	{
		return GetVector(a_value[0],a_value[1],a_value[2]);
	}
#endif

#if(GM_USE_ENTITY_STACK)
	inline bool IsEntity() const { return m_type == GM_ENTITY; }
	inline void SetEntity(int _hndl) { m_type = GM_ENTITY; m_value.m_enthndl = _hndl; }
	inline int GetEntity() const { return m_value.m_enthndl; }
	static gmVariable EntityVar(int _hndl) { gmVariable v; v.SetEntity(_hndl); return v; }
#endif

#if(GM_USE_ENUM_SUPPORT)
	inline bool IsEnum() const { return m_type == GM_ENUM; }
	inline void SetEnum(int enumType, int enumValue)
	{
		m_value.m_enum.enumType = enumType;
		m_value.m_enum.enumValue = enumValue;
	}
	inline int GetEnumType() const { return m_value.m_enum.enumType; }
	inline int GetEnumValue() const { return m_value.m_enum.enumValue; }
#endif

	inline void SetInt(int a_value) { m_type = GM_INT; m_value.m_int = a_value; }
	inline void SetFloat(float a_value) { m_type = GM_FLOAT; m_value.m_float = a_value; }
	inline void SetString(gmStringObject * a_string);
	void SetString(gmMachine * a_machine, const char * a_cString);
	inline void SetTable(gmTableObject * a_table);
	inline void SetFunction(gmFunctionObject * a_function);
	void SetUser(gmUserObject * a_object);
	void SetUser(gmMachine * a_machine, void * a_userPtr, int a_userType);

	inline void Nullify() { m_type = GM_NULL; m_value.m_int = 0; }
	inline bool IsNull() const { return m_type == GM_NULL; }
	inline bool IsReference() const { return m_type >= GM_STRING; }
	inline bool IsInt() const { return m_type == GM_INT; }
	inline bool IsFloat() const { return m_type == GM_FLOAT; }
	inline bool IsNumber() const { return IsInt() || IsFloat(); }
	inline bool IsString() const { return m_type == GM_STRING; }
	inline bool IsFunction() const { return m_type == GM_FUNCTION; }

	// GetInt and GetFloat are not protected. User should verify the type before calling this.
	inline int GetInt() const  { return m_value.m_int; }
	inline bool GetInt(int &_i, int _default);
	inline int GetInt(int _default);

	inline float GetFloat() const { return m_value.m_float; }
	inline bool GetFloat(float &_i, float _default);

	/// \brief AsString will get this gm variable as a string if possible.  AsString is used for the gm "print" and system.Exec function bindings.
	/// \param a_buffer is a buffer you must provide for the function to write into.  this buffer needs only be 256 characters long as it is stated that
	///        user types should give an AsString conversion < 256 characters.  If the type has a longer string type, it may return an interal string
	/// \param a_len must be >= 256
	/// \return a_buffer or internal variable string
	const char * AsString(gmMachine * a_machine, char * a_buffer, int a_len) const;

	/// \brief AsStringWithType will get the gm variable as a string with type name in front of value.  AsStringWithType is used for debugging, and may cut
	///        the end off some string types.
	/// \sa AsString
	/// \return a_buffer always
	const char * AsStringWithType(gmMachine * a_machine, char * a_buffer, int a_len) const;

	void DebugInfo(gmMachine * a_machine, gmChildInfoCallback a_cb) const;

	/// Return int/float or zero
	inline int GetIntSafe(int _default = 0) const;
	bool GetIntSafe(int &_i, int _default) const;
	inline bool GetBoolSafe() const;
	inline bool GetBoolSafe(bool &_b, bool _default) const;
	/// Return float/int or zero
	inline float GetFloatSafe() const;
	inline bool GetFloatSafe(float &_f, float _default = 0.f) const;
	/// Return string object or null
	inline gmStringObject* GetStringObjectSafe() const;
	/// Return table object or null
	inline gmTableObject* GetTableObjectSafe() const;
	/// Return function object or null
	inline gmFunctionObject* GetFunctionObjectSafe() const;
	/// Return c string or empty string
	const char* GetCStringSafe(const char *_def = "") const;
	/// Return user type ptr or null
	void* GetUserSafe(int a_userType) const;
	gmUserObject *GetUserObjectSafe(int a_userType) const;
	gmUserObject *GetUserObjectSafe() const;

	static inline gmuint Hash(const gmVariable &a_key)
	{
		gmuint hash = (gmuint) a_key.m_value.m_ref;
		if(a_key.IsReference())
		{
			hash >>= 2; // Reduce pointer address aliasing
		}
		return hash;
	}

	static inline int Compare(const gmVariable &a_keyA, const gmVariable &a_keyB)
	{
		if(a_keyA.m_type < a_keyB.m_type) return -1;
		if(a_keyA.m_type > a_keyB.m_type) return 1;
		if(a_keyA.m_value.m_int < a_keyB.m_value.m_int) return -1;
		if(a_keyA.m_value.m_int > a_keyB.m_value.m_int) return 1;
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	// for script binding helpers
	void Set(gmMachine *a_machine, bool a_value) { SetInt(a_value?1:0); }
	void Set(gmMachine *a_machine, int a_value) { SetInt(a_value); }
	void Set(gmMachine *a_machine, float a_value) { SetFloat(a_value); }
	void Set(gmMachine *a_machine, gmStringObject *a_value) { SetString(a_value); }
	void Set(gmMachine *a_machine, gmTableObject *a_value) { SetTable(a_value); }
	void Set(gmMachine *a_machine, gmFunctionObject *a_value) { SetFunction(a_value); }
	void Set(gmMachine *a_machine, gmUserObject *a_value) { SetUser(a_value); }
	void Set(gmMachine *a_machine, gmGCRoot<gmFunctionObject> &a_value);
	void Set(gmMachine *a_machine, gmGCRoot<gmTableObject> &a_value);

#if(GM_USE_VECTOR3_STACK)
	void Set(gmMachine *a_machine, Vec3 a_value)
	{
		SetVector(a_value.x,a_value.y,a_value.z);
	}
#endif

	void Get(gmMachine *a_machine, bool &a_value)
	{
		a_value = GetIntSafe()!=0;
	}
	void Get(gmMachine *a_machine, int &a_value)
	{
		a_value = GetIntSafe();
	}
	void Get(gmMachine *a_machine, float &a_value)
	{
		a_value = GetFloatSafe();
	}
	void Get(gmMachine *a_machine, gmStringObject *&a_value)
	{
		a_value = GetStringObjectSafe();
	}
	void Get(gmMachine *a_machine, gmTableObject *&a_value)
	{
		a_value = GetTableObjectSafe();
	}
	void Get(gmMachine *a_machine, gmFunctionObject * a_value)
	{
		a_value = GetFunctionObjectSafe();
	}
	void Get(gmMachine *a_machine, gmUserObject *&a_value)
	{
		a_value = GetUserObjectSafe();
	}
#if(GM_USE_VECTOR3_STACK)
	void Get(gmMachine *a_machine, Vec3 &a_value)
	{
		GetVector(a_value.x,a_value.y,a_value.z);
	}
#endif
	void Get(gmMachine *a_machine, gmGCRoot<gmFunctionObject> &a_value);
	void Get(gmMachine *a_machine, gmGCRoot<gmTableObject> &a_value);

};



/// \class gmObject
/// \brief gmObject is the base class for gm reference types.  All gmObject types are allocated from the gmMachine
class gmObject
#if GM_USE_INCGC
	: public gmGCObjBase
#endif //GM_USE_INCGC
{
public:

	inline gmptr GetRef() const { return (gmptr) this; }
	virtual int GetType() const = 0;

#if !GM_USE_INCGC
	/// \brief Only call Mark() if NeedsMark() returns true.
	virtual void Mark(gmMachine * a_machine, gmuint32 a_mark) { if(m_mark != GM_MARK_PERSIST) m_mark = a_mark; }
	inline bool NeedsMark(gmuint32 a_mark) { return ((m_mark != GM_MARK_PERSIST) && (m_mark != a_mark)); }
	virtual void Destruct(gmMachine * a_machine) = 0;

protected:

	gmuint32 m_mark;

private:

	gmObject * m_sysNext;
#endif //!GM_USE_INCGC

protected:

	/// \brief Non-public constructor.  Create via gmMachine.
	inline gmObject()
#if !GM_USE_INCGC
		: m_mark(GM_MARK_START)
#endif //!GM_USE_INCGC
	{}
	friend class gmMachine;
};


//
// INLINE IMPLEMENTATION
//


inline void gmVariable::SetString(gmStringObject * a_string)
{
	m_type = GM_STRING;
	m_value.m_ref = ((gmObject *) a_string)->GetRef();
}

inline void gmVariable::SetTable(gmTableObject * a_table)
{
	m_type = GM_TABLE;
	m_value.m_ref = ((gmObject *) a_table)->GetRef();
}

inline void gmVariable::SetFunction(gmFunctionObject * a_function)
{
	m_type = GM_FUNCTION;
	m_value.m_ref = ((gmObject *) a_function)->GetRef();
}

inline bool gmVariable::GetInt(int &_i, int _default)
{
	_i = IsInt() ? GetInt() : _default;
	return IsInt();
}

inline bool gmVariable::GetFloat(float &_i, float _default)
{
	_i = IsFloat() ? GetFloat() : _default;
	return IsFloat();
}

inline int gmVariable::GetInt(int _default)
{
	return IsInt() ? GetInt() : _default;
}

inline int gmVariable::GetIntSafe(int _default) const
{
	if( GM_INT == m_type )
	{
		return m_value.m_int;
	}
	else if( GM_FLOAT == m_type )
	{
		return (int)m_value.m_float;
	}
	return _default;
}

inline bool gmVariable::GetIntSafe(int &_i, int _default) const
{
	_i = _default;
	if( GM_INT == m_type )
	{
		_i = m_value.m_int;
		return true;
	}
	else if( GM_FLOAT == m_type )
	{
		_i = (int)m_value.m_float;
		return true;
	}
	return false;
}

inline bool gmVariable::GetBoolSafe() const
{
	if( GM_INT == m_type )
	{
		return m_value.m_int != 0;
	}
	return false;
}

inline bool gmVariable::GetBoolSafe(bool &_b, bool _default) const
{
	_b = _default;
	if( GM_INT == m_type )
	{
		_b = m_value.m_int != 0;
		return true;
	}
	return false;
}

inline float gmVariable::GetFloatSafe() const
{
	if( GM_FLOAT == m_type )
	{
		return m_value.m_float;
	}
	else if( GM_INT == m_type )
	{
		return (float)m_value.m_int;
	}
	return 0.0f;
}

inline bool gmVariable::GetFloatSafe(float &_f, float _default) const
{
	_f = _default;
	if( GM_FLOAT == m_type )
	{
		_f = m_value.m_float;
		return true;
	}
	else if( GM_INT == m_type )
	{
		_f = (float)m_value.m_int;
		return true;
	}
	return false;
}

inline gmStringObject * gmVariable::GetStringObjectSafe() const
{
	if( GM_STRING == m_type )
	{
		return (gmStringObject *)m_value.m_ref;
	}
	return NULL;
}


inline gmTableObject * gmVariable::GetTableObjectSafe() const
{
	if( GM_TABLE == m_type )
	{
		return (gmTableObject *)m_value.m_ref;
	}
	return NULL;
}

inline gmFunctionObject * gmVariable::GetFunctionObjectSafe() const
{
	if( GM_FUNCTION == m_type )
	{
		return (gmFunctionObject *)m_value.m_ref;
	}
	return NULL;
}


#endif // _GMVARIABLE_H_
