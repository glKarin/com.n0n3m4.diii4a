#include "PrecompCommon.h"
#include "gmNamesList.h"
#include "ScriptManager.h"
#include "NameManager.h"
#include "gmThread.h"

#undef GetObject

// script: NameList
//		Script bindings for <NameManager>

//////////////////////////////////////////////////////////////////////////

gmType GM_NAMESLIST = 0; //NULL;

//////////////////////////////////////////////////////////////////////////

// note: The names list is created automatically and acts as a global table.
//		Map bot names to profile scripts in any script, such as:
//		Names["SomeBotName"] = "SomeScriptName";
//		Names should only be used by 1 bot at a time.

//////////////////////////////////////////////////////////////////////////

static int GM_CDECL gmfClearNames(gmThread *a_thread)
{
	NameManager::GetInstance()->ClearNames();
	return GM_OK;
}

static int GM_CDECL gmfNameListGetInd(gmThread * a_thread, gmVariable * a_operands)
{
	GM_ASSERT(a_operands[0].m_type == GM_NAMESLIST);
	if(a_operands[1].m_type == GM_STRING)
	{
		gmStringObject* stringObj = a_operands[1].GetStringObjectSafe();

		// Return the profile that goes with this name
		if(stringObj != NULL && stringObj->GetString())
		{
			String strProfileName = NameManager::GetInstance()->GetProfileForName(stringObj->GetString());

			gmStringObject *pString = a_thread->GetMachine()->AllocStringObject(strProfileName.c_str());
			a_operands[0].SetString(pString);
			return GM_OK;
		}
	}
	a_operands[0].Nullify();
	return GM_EXCEPTION;
}

static int GM_CDECL gmfNameListSetInd(gmThread * a_thread, gmVariable * a_operands)
{
	GM_ASSERT(a_operands[0].m_type == GM_NAMESLIST);
	if(const char *pName = a_operands[1].GetCStringSafe(0))
	{
		const char *pProfileName = a_operands[2].GetCStringSafe(0);
		if(pProfileName)
		{
			if(!NameManager::GetInstance()->AddName(pName, pProfileName))
			{
				EngineFuncs::ConsoleError(va("%s : name already registered", pName));
			}
		}
		return GM_OK;
	}
	a_thread->GetMachine()->GetLog().LogEntry("expected string index");
	return GM_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////////

static gmFunctionEntry s_namesLib[] = 
{ 
	{"Clear", gmfClearNames},
};

//////////////////////////////////////////////////////////////////////////

void gmBindNamesListLib(gmMachine * a_machine)
{
	//////////////////////////////////////////////////////////////////////////
	// Register Name List type.
	GM_NAMESLIST = a_machine->CreateUserType("namelist");
	a_machine->RegisterTypeOperator(GM_NAMESLIST, O_GETIND, NULL, gmfNameListGetInd);
	a_machine->RegisterTypeOperator(GM_NAMESLIST, O_SETIND, NULL, gmfNameListSetInd);
	a_machine->RegisterUserCallbacks(GM_NAMESLIST, NULL, NULL, NULL);
	a_machine->RegisterTypeLibrary(GM_NAMESLIST, s_namesLib, sizeof(s_namesLib) / sizeof(s_namesLib[0]));
	//////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
