/*
_____               __  ___          __            ____        _      __
/ ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
/ (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
\___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
/___/             /_/

See Copyright Notice in gmMachine.h

*/

#include "gmConfig.h"
#include "gmVariable.h"
#include "gmThread.h"
#include "gmStringObject.h"
#include "gmUserObject.h"
#include "gmGCRoot.h"

// Init statics and constants
gmVariable gmVariable::s_null = gmVariable(GM_NULL, 0);


const char * gmVariable::AsString(gmMachine * a_machine, char * a_buffer, int a_len) const
{
	switch(m_type)
	{
	case GM_NULL :
		{
			_gmsnprintf(a_buffer, a_len, "null");
			break;
		}
	case GM_INT :
		{
			_gmsnprintf(a_buffer, a_len, "%d", m_value.m_int);
			break;
		}
	case GM_FLOAT :
		{
			_gmsnprintf(a_buffer, a_len, "%g", m_value.m_float);
			break;
		}
	case GM_STRING :
		{
			return ((gmStringObject *) GM_MOBJECT(a_machine, m_value.m_ref))->GetString();
		}
	case GM_FUNCTION:
		{
			const char *source = 0, *fileName = 0;
			if(a_machine->GetSourceCode(GetFunctionObjectSafe()->GetSourceId(), source, fileName) && fileName) {
				_gmsnprintf(a_buffer, a_len, "%s (%s)", GetFunctionObjectSafe()->GetDebugName(), fileName);
			} else {
				_gmsnprintf(a_buffer, a_len, "%s", GetFunctionObjectSafe()->GetDebugName());
			}
			break;
		}
#if(GM_USE_VECTOR3_STACK)
	case GM_VEC3:
		{
			_gmsnprintf(a_buffer, a_len, "(%.3f, %.3f, %.3f)",
				m_value.m_vec3.x,
				m_value.m_vec3.y,
				m_value.m_vec3.z);
			break;
		}
#endif
	default:
		gmAsStringCallback asStringCallback = a_machine->GetUserAsStringCallback(m_type);
		if(asStringCallback)
		{
			asStringCallback((gmUserObject *) GM_MOBJECT(a_machine, m_value.m_ref), a_buffer, a_len );
		}
		else
		{
			_gmsnprintf(a_buffer, a_len, "%s:0x%llx", a_machine->GetTypeName(m_type), m_value.m_ref); //karin: format %x -> %llx
		}
		break;
	}
	return a_buffer;
}

void gmVariable::DebugInfo(gmMachine * a_machine, gmChildInfoCallback a_cb) const
{
	const int buffSize = 256;
	char buffVar[buffSize];
	char buffVal[buffSize];

	switch(m_type)
	{
#if(GM_USE_VECTOR3_STACK)
	case GM_VEC3:
		{
			break;
		}
#endif
	case GM_STRING:
		{
			break;
		}
	case GM_TABLE:
		{
			gmTableObject *pTable = GetTableObjectSafe();
			gmTableIterator tIt;
			gmTableNode *pNode = pTable->GetFirst(tIt);
			while(pNode)
			{
				const char *pVar = pNode->m_key.AsString(a_machine, buffVar, buffSize);
				const char *pVal = pNode->m_value.AsString(a_machine, buffVal, buffSize);

				const int varId =
					pNode->m_value.IsReference() && !pNode->m_value.IsFunction() ?
					pNode->m_value.m_value.m_ref :
				0;

				a_cb(
					pVar,
					pVal,
					a_machine->GetTypeName(pNode->m_value.m_type),
					varId);

				pNode = pTable->GetNext(tIt);
			}
			break;
		}
	case GM_FUNCTION:
		{
			break;
		}
	default:
		gmDebugChildInfoCallback dbg_cb = a_machine->GetUserDebugChildInfoCallback(m_type);
		if(dbg_cb)
		{
			dbg_cb((gmUserObject*)GM_MOBJECT(a_machine, m_value.m_ref), a_machine, a_cb);
		}
	}
}

const char * gmVariable::AsStringWithType(gmMachine * a_machine, char * a_buffer, int a_len) const
{
	// Copy the type first
	_gmsnprintf(a_buffer, a_len, "%s: ", a_machine->GetTypeName(m_type));

	// Update for used portion
	int usedLen = (int)strlen(a_buffer);
	char* newBufferPos = a_buffer + usedLen;
	int newLen = a_len - usedLen;

	if(newLen > 0) //Paranoid check some string buffer remaining
	{
		const char * str = AsString(a_machine, newBufferPos, newLen);
		if(str != newBufferPos)
		{
			// copy what we can... this is used for debug purposes so it doesnt matter if we chop some off
			strncpy(newBufferPos, str, newLen);
		}
	}

	return a_buffer;
}


void gmVariable::SetUser(gmUserObject * a_object)
{
	m_type = (gmType) a_object->m_userType;
	m_value.m_ref = ((gmObject *) a_object)->GetRef();
}


void gmVariable::SetUser(gmMachine * a_machine, void * a_userPtr, int a_userType)
{
	SetUser( a_machine->AllocUserObject(a_userPtr, a_userType) );
}


void gmVariable::SetString(gmMachine * a_machine, const char * a_cString)
{
	SetString( a_machine->AllocStringObject(a_cString) );
}


const char * gmVariable::GetCStringSafe(const char *_def) const
{
	if( m_type == GM_STRING )
	{
		return ((gmStringObject *)m_value.m_ref)->GetString();
	}
	return _def;
}


void * gmVariable::GetUserSafe(int a_userType) const
{
	if( m_type == a_userType )
	{
		return ((gmUserObject *)m_value.m_ref)->m_user;
	}
	return NULL;
}

gmUserObject *gmVariable::GetUserObjectSafe(int a_userType) const
{
	if( m_type == a_userType )
	{
		return ((gmUserObject *)m_value.m_ref);
	}
	return NULL;
}

gmUserObject *gmVariable::GetUserObjectSafe() const
{
	if( m_type >= GM_USER )
	{
		return ((gmUserObject *)m_value.m_ref);
	}
	return NULL;
}

void gmVariable::Set(gmMachine *a_machine, gmGCRoot<gmFunctionObject> &a_value)
{
	if(a_value)
		SetFunction(a_value);
	else
		Nullify();
}
void gmVariable::Set(gmMachine *a_machine, gmGCRoot<gmTableObject> &a_value)
{
	if(a_value)
		SetTable(a_value);
	else
		Nullify();
}
void gmVariable::Get(gmMachine *a_machine, gmGCRoot<gmFunctionObject> &a_value)
{
	gmFunctionObject *a_v = GetFunctionObjectSafe();
	if(a_v)
		a_value.Set(a_v,a_machine);
	else
		a_value.Set(NULL,a_machine);
}
void gmVariable::Get(gmMachine *a_machine, gmGCRoot<gmTableObject> &a_value)
{
	gmTableObject *a_v = GetTableObjectSafe();
	if(a_v)
		a_value.Set(a_v,a_machine);
	else
		a_value.Set(NULL,a_machine);
}
