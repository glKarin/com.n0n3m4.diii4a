#ifndef SV_SQL_H
#define SV_SQL_H

#ifdef USE_MYSQL
	#ifdef _WIN32
		#include <windows.h>
	#endif
	#include <mysql/mysql.h>
#endif
#ifdef USE_SQLITE
typedef struct
{
	char *ptr;
	int len;
} sqliteresult_t;
#endif

#define SQL_CONNECT_STRUCTPARAMS 2
#define SQL_CONNECT_PARAMS 4

typedef enum 
{
	SQLDRV_MYSQL,
	SQLDRV_SQLITE, /* NOT IN YET */
	SQLDRV_INVALID
} sqldrv_t;

typedef struct queryrequest_s
{
	int srvid;
	int num; /* query number reference */ 
	struct queryrequest_s *nextqueue; /* next request in queue */
	struct queryrequest_s *nextreq; /* next request in queue */
	struct queryresult_s *results;	/* chain of received results */
	enum
	{
		SR_NEW,
		SR_PENDING,
		SR_PARTIAL,		//
		SR_FINISHED,	//waiting for close
		SR_ABORTED		//don't notify. destroy on finish.
	} state;			//maintained by main thread. worker *may* check for aborted state as a way to quickly generate an error.
	qboolean (*callback)(struct queryrequest_s *req, int firstrow, int numrows, int numcols, qboolean eof);	/* called on main thread once complete */
	struct
	{
		qboolean persistant; /* persistant query */
		int qccallback; /* callback function reference */
		int selfent; /* self entity on call */
		float selfid; /* self entity id on call */
		int otherent; /* other entity on call */
		float otherid; /* other entity id on call */
		void *thread;
	} user;	/* sql code does not write anything in this struct */
	char query[1]; /* query to run */
} queryrequest_t;

typedef struct queryresult_s
{
	struct queryrequest_s *request; /* corresponding request */
	struct queryresult_s *next; /* next result in queue */
	int rows; /* rows contained in single result set */
	int firstrow;	/* 0 on first result block */
	int columns; /* fields */
	qboolean eof; /* end of query reached */
	void *result; /* result set from mysql */
#if 0
	char **resultset; /* stored result set from partial fetch */
#endif
	char error[1]; /* error string, "" if none */
} queryresult_t;

typedef struct sqlserver_s
{
	void *thread; /* worker thread for server */
	sqldrv_t driver; /* driver type */
#ifdef USE_MYSQL
	MYSQL *mysql; /* mysql server */
#endif
#ifdef USE_SQLITE
	struct sqlite3 *sqlite;
#endif
	volatile qboolean active; /* set to false to kill thread */
	volatile qboolean terminated; /* set by the worker to say that it won't block (for long) and can be joined */
	void *requestcondv; /* lock and conditional variable for queue read/write */
	void *resultlock; /* mutex for queue read/write */
	int querynum; /* next reference number for queries */
	queryrequest_t *requests;		/* list of pending and persistant requests */
	queryrequest_t *requestqueue; /* query requests queue */
	queryrequest_t *requestslast; /* query requests queue last link */
	queryresult_t *results; /* query results queue */
	queryresult_t *resultslast; /* query results queue last link */
	queryresult_t *serverresult; /* most recent (orphaned) server error result */
	char **connectparams; /* connect parameters (0 = host, 1 = user, 2 = pass, 3 = defaultdb) */
} sqlserver_t;

/* prototypes */
void SQL_Init(void);
void SQL_KillServers(void *owner);
void SQL_DeInit(void);

sqlserver_t *SQL_GetServer (void *owner, int serveridx, qboolean inactives);
queryrequest_t *SQL_GetQueryRequest (sqlserver_t *server, int queryidx);
queryresult_t *SQL_GetQueryResult (sqlserver_t *server, int queryidx, int row);
//void SQL_DeallocResult(sqlserver_t *server, queryresult_t *qres);
void SQL_ClosePersistantResult(sqlserver_t *server, queryresult_t *qres);
void SQL_CloseResult(sqlserver_t *server, queryresult_t *qres);
void SQL_CloseRequest(sqlserver_t *server, queryrequest_t *qres, qboolean force);
void SQL_CloseAllResults(sqlserver_t *server);
char *SQL_ReadField (sqlserver_t *server, queryresult_t *qres, int row, int col, qboolean fields, size_t *resultsize);
int SQL_NewServer(void *owner, const char *driver, const char **paramstr);
int SQL_NewQuery(sqlserver_t *server, qboolean (*callback)(queryrequest_t *req, int firstrow, int numrows, int numcols, qboolean eof), const char *str, queryrequest_t **reqout);	//callback will be called on the main thread once the result is back
void SQL_Disconnect(sqlserver_t *server);
void SQL_Escape(sqlserver_t *server, const char *src, char *dst, int dstlen);
const char *SQL_Info(sqlserver_t *server);
qboolean SQL_Available(void);
void SQL_ServerCycle (void);

extern cvar_t sql_driver;
extern cvar_t sql_host;
extern cvar_t sql_username;
extern cvar_t sql_password;
extern cvar_t sql_defaultdb;

#define SQLCVAROPTIONS "SQL Defaults"

#endif
