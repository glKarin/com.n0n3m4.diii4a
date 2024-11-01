// quickly hacked by Deadsoul ( jed at deadsoul dot com ) 

#include "gmConfig.h"
#include "gmSqliteLib.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmHelpers.h"

extern "C" 
{
	#include "sqlite/sqlite3.h"
};

//#include <ctype.h>
//#include <time.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <process.h>

#undef		GetObject

static gmType s_gmSqliteType = GM_NULL;

static int GM_CDECL gmfSqlite(gmThread * a_thread)
{
	a_thread->PushNewUser(NULL, s_gmSqliteType);
	return GM_OK;
};

#if GM_USE_INCGC
static void GM_CDECL gmGCDestructSqliteUserType(gmMachine * a_machine, gmUserObject* a_object)
{
	if(a_object->m_user)
		sqlite3_close((sqlite3*)a_object->m_user);
	a_object->m_user = NULL;
}
#else //GM_USE_INCGC
static void GM_CDECL gmGCSqliteUserType(gmMachine * a_machine, gmUserObject * a_object, gmuint32 a_mark)
{
	if(a_object->m_user)
		sqlite3_close((sqlite3*)a_object->m_user);
	a_object->m_user = NULL;
}
#endif //GM_USE_INCGC

static int GM_CDECL gmfSqliteOpen(gmThread * a_thread) // path
{
	GM_CHECK_STRING_PARAM(filename, 0);

	gmUserObject * sqliteObject = a_thread->ThisUserObject();

	GM_ASSERT(sqliteObject->m_userType == s_gmSqliteType);

	if(sqliteObject->m_user)
		sqlite3_close((sqlite3*) sqliteObject->m_user);

	enum {BufferSize=1024};
	char buffer[BufferSize] = {};
	sqlite3_snprintf(BufferSize,buffer,"%s",filename);

	sqlite3 *pDb = 0;
	int status = sqlite3_open(buffer,&pDb);
	sqliteObject->m_user = (void *)pDb;

	if(status!=SQLITE_OK)
	{
		GM_EXCEPTION_MSG("sqlite error: %s", sqlite3_errmsg(pDb));
		return GM_EXCEPTION;
	}	

	if(sqliteObject->m_user)
		a_thread->PushInt(1);
	else
		a_thread->PushInt(0);
	return GM_OK;
};

static int GM_CDECL gmfSqliteClose(gmThread * a_thread)
{
	gmUserObject * sqliteObject = a_thread->ThisUserObject();
	GM_ASSERT(sqliteObject->m_userType == s_gmSqliteType);
	if(sqliteObject->m_user)
	{
		sqlite3_close((sqlite3*)sqliteObject->m_user);
	}
	sqliteObject->m_user = NULL;
	return GM_OK;
}

static int GM_CDECL gmfSqliteQuery(gmThread * a_thread)
{
	GM_CHECK_STRING_PARAM(query, 0);
	
	gmUserObject * sqliteObject = a_thread->ThisUserObject();
	GM_ASSERT(sqliteObject->m_userType == s_gmSqliteType);

	sqlite3 *pSqlite = (sqlite3*)sqliteObject->m_user;
	if(pSqlite)
	{
		int	nRows = 0;
		int	nColumns = 0;
		char *zErrMsg = 0;
		char **results = 0;

		int	nResult=sqlite3_get_table(
			pSqlite,
			query,
			&results,
			&nRows,
			&nColumns,
			&zErrMsg);
		if(nResult != SQLITE_OK)
		{
			//GM_EXCEPTION_MSG("sqlite error: %s", zErrMsg);
			//return GM_EXCEPTION;
			
			//	cs: handle scripts that get reloaded and try to CREATE existing tables
			//	if ( db.Query("CREATE TABLE files(name TEXT, filemod TEXT);") ){	
			//		db.Query("INSERT INTO files VALUES('bibble','0');");
			//	}
			a_thread->PushInt(false);
		}
		else
		{
			//	this creates a table of tables
			//	probably not the best approach but it works
			//	note only strings 

			DisableGCInScope gcEn(a_thread->GetMachine());
			if (nColumns!=0 && nRows!=0)
			{
				gmTableObject *resultTbl = a_thread->PushNewTable();
				for(int r = 0; r < nRows; ++r)
				{
					gmTableObject *pRowTbl = a_thread->GetMachine()->AllocTableObject();
					for(int c = 0; c < nColumns; ++c)
					{
						gmStringObject *pStr = a_thread->GetMachine()->AllocStringObject(results[nColumns+c+r*nColumns]);
						pRowTbl->Set(a_thread->GetMachine(),results[c],gmVariable(pStr));
					}
					resultTbl->Set(a_thread->GetMachine(),r,gmVariable(pRowTbl));
				}
			}
			sqlite3_free_table(results);
		}
	}
	return GM_OK;
};


static gmFunctionEntry s_SqliteLib[] =
{
	{"sqlite", gmfSqlite},
};
static gmFunctionEntry s_sqliteLib[] = 
{ 
  /*gm
    \lib sqlite
    \brief sqlite functions , a simple embeddable sql engine
  */
  /*gm
    \function Open
    \brief Open will open a sqllite database , creating if necessary 
    \param string filename
    \return integer value returned , 0 on error
  */
	{"Open", gmfSqliteOpen},
  /*gm
    \function Close
    \brief Close will close a sqllite database 
    \param none
    \return none
  */
	{"Close", gmfSqliteClose},
  /*gm
    \function Query
    \brief perform an sql query on the open database
    \param string sql query 
    \return a table of columns with a table of rows inside each one
	*/
	{"Query", gmfSqliteQuery},
};

void gmBindSqliteLib(gmMachine * a_machine)
{
	a_machine->RegisterLibrary(s_SqliteLib, sizeof(s_SqliteLib) / sizeof(s_SqliteLib[0]));
	// sql
	s_gmSqliteType = a_machine->CreateUserType("sqlite");
#if GM_USE_INCGC
	a_machine->RegisterUserCallbacks(s_gmSqliteType, NULL, gmGCDestructSqliteUserType);
#else //GM_USE_INCGC
	a_machine->RegisterUserCallbacks(s_gmSqliteType, NULL, gmGCSqliteUserType);
#endif //GM_USE_INCGC
	a_machine->RegisterTypeLibrary(s_gmSqliteType, s_sqliteLib, sizeof(s_sqliteLib) / sizeof(s_sqliteLib[0]));

}

