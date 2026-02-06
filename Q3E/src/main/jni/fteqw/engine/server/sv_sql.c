#include "quakedef.h"

//SQLITE:
//should probably be compiled with -DSQLITE_OMIT_ATTACH -DSQLITE_OMIT_LOAD_EXTENSION which would mean we don't need to override authorisation.

#ifdef SQL
#include "sv_sql.h"

#ifdef USE_MYSQL
static void (VARGS *qmysql_library_end)(void);
static int (VARGS *qmysql_library_init)(int argc, char **argv, char **groups);

static my_ulonglong (VARGS *qmysql_affected_rows)(MYSQL *mysql);
static void (VARGS *qmysql_close)(MYSQL *sock);
static void (VARGS *qmysql_data_seek)(MYSQL_RES *result, my_ulonglong offset);
static unsigned int (VARGS *qmysql_errno)(MYSQL *mysql);
static const char *(VARGS *qmysql_error)(MYSQL *mysql);
static MYSQL_FIELD *(VARGS *qmysql_fetch_field_direct)(MYSQL_RES *res, unsigned int fieldnr);
static MYSQL_ROW (VARGS *qmysql_fetch_row)(MYSQL_RES *result);
static unsigned long *(VARGS *qmysql_fetch_lengths)(MYSQL_RES *result);
static unsigned int (VARGS *qmysql_field_count)(MYSQL *mysql);
static void (VARGS *qmysql_free_result)(MYSQL_RES *result);
static const char *(VARGS *qmysql_get_client_info)(void);
static MYSQL *(VARGS *qmysql_init)(MYSQL *mysql);
static MYSQL_RES *(VARGS *qmysql_store_result)(MYSQL *mysql);
static unsigned int (VARGS *qmysql_num_fields)(MYSQL_RES *res);
static my_ulonglong (VARGS *qmysql_num_rows)(MYSQL_RES *res);
static int (VARGS *qmysql_options)(MYSQL *mysql, enum mysql_option option, const char *arg);
static int (VARGS *qmysql_query)(MYSQL *mysql, const char *q);
static MYSQL *(VARGS *qmysql_real_connect)(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag);
static unsigned long (VARGS *qmysql_real_escape_string)(MYSQL *mysql, char *to, const char *from, unsigned long length);
static void (VARGS *qmysql_thread_end)(void);
static my_bool (VARGS *qmysql_thread_init)(void);
static unsigned int (VARGS *qmysql_thread_safe)(void);

static dllfunction_t mysqlfuncs[] =
{
	{(void*)&qmysql_library_end, "mysql_server_end"}, /* written as a define alias in mysql.h */
	{(void*)&qmysql_library_init, "mysql_server_init"}, /* written as a define alias in mysql.h */
	{(void*)&qmysql_affected_rows, "mysql_affected_rows"},
	{(void*)&qmysql_close, "mysql_close"},
	{(void*)&qmysql_data_seek, "mysql_data_seek"},
	{(void*)&qmysql_errno, "mysql_errno"},
	{(void*)&qmysql_error, "mysql_error"},
	{(void*)&qmysql_fetch_field_direct, "mysql_fetch_field_direct"},
	{(void*)&qmysql_fetch_row, "mysql_fetch_row"},
	{(void*)&qmysql_fetch_lengths, "mysql_fetch_lengths"},
	{(void*)&qmysql_field_count, "mysql_field_count"},
	{(void*)&qmysql_free_result, "mysql_free_result"},
	{(void*)&qmysql_get_client_info, "mysql_get_client_info"},
	{(void*)&qmysql_init, "mysql_init"},
	{(void*)&qmysql_store_result, "mysql_store_result"},
	{(void*)&qmysql_num_fields, "mysql_num_fields"},
	{(void*)&qmysql_num_rows, "mysql_num_rows"},
	{(void*)&qmysql_options, "mysql_options"},
	{(void*)&qmysql_query, "mysql_query"},
	{(void*)&qmysql_real_connect, "mysql_real_connect"},
	{(void*)&qmysql_real_escape_string, "mysql_real_escape_string"},
	{(void*)&qmysql_thread_end, "mysql_thread_end"},
	{(void*)&qmysql_thread_init, "mysql_thread_init"},
	{(void*)&qmysql_thread_safe, "mysql_thread_safe"},
	{NULL}
};
dllhandle_t *mysqlhandle;
#endif

#ifdef USE_SQLITE
#include "sqlite3.h"
SQLITE_API int (QDECL *qsqlite3_open)(const char *zFilename, sqlite3 **ppDb);
SQLITE_API const char *(QDECL *qsqlite3_libversion)(void);
SQLITE_API int (QDECL *qsqlite3_set_authorizer)(sqlite3*, int (QDECL *xAuth)(void*,int,const char*,const char*,const char*,const char*), void *pUserData);
SQLITE_API int (QDECL *qsqlite3_enable_load_extension)(sqlite3 *db, int onoff);
SQLITE_API const char *(QDECL *qsqlite3_errmsg)(sqlite3 *db);
SQLITE_API int (QDECL *qsqlite3_close)(sqlite3 *db);

SQLITE_API int (QDECL *qsqlite3_prepare)(sqlite3 *db, const char *zSql, int nBytes, sqlite3_stmt **ppStmt, const char **pzTail);
SQLITE_API int (QDECL *qsqlite3_column_count)(sqlite3_stmt *pStmt);
SQLITE_API const char *(QDECL *qsqlite3_column_name)(sqlite3_stmt *pStmt, int N);
SQLITE_API int (QDECL *qsqlite3_step)(sqlite3_stmt *pStmt);
SQLITE_API const unsigned char *(QDECL *qsqlite3_column_text)(sqlite3_stmt *pStmt, int i);
SQLITE_API int (QDECL *qsqlite3_column_bytes)(sqlite3_stmt*, int iCol);
SQLITE_API int (QDECL *qsqlite3_finalize)(sqlite3_stmt *pStmt);

static dllfunction_t sqlitefuncs[] =
{
	{(void*)&qsqlite3_open,						"sqlite3_open"}, /* written as a define alias in mysql.h */
	{(void*)&qsqlite3_libversion,				"sqlite3_libversion"}, /* written as a define alias in mysql.h */
	{(void*)&qsqlite3_set_authorizer,			"sqlite3_set_authorizer"},
	{(void*)&qsqlite3_enable_load_extension,	"sqlite3_enable_load_extension"},
	{(void*)&qsqlite3_errmsg,					"sqlite3_errmsg"},
	{(void*)&qsqlite3_close,					"sqlite3_close"},

	{(void*)&qsqlite3_prepare,					"sqlite3_prepare"},
	{(void*)&qsqlite3_column_count,				"sqlite3_column_count"},
	{(void*)&qsqlite3_column_name,				"sqlite3_column_name"},
	{(void*)&qsqlite3_step,						"sqlite3_step"},
	{(void*)&qsqlite3_column_text,				"sqlite3_column_text"},
	{(void*)&qsqlite3_column_bytes,				"sqlite3_column_bytes"},
	{(void*)&qsqlite3_finalize,					"sqlite3_finalize"},
	{NULL}
};
dllhandle_t *sqlitehandle;
#endif

cvar_t sql_driver = CVARF("sv_sql_driver", "", CVAR_NOUNSAFEEXPAND);
cvar_t sql_host = CVARF("sv_sql_host", "127.0.0.1", CVAR_NOUNSAFEEXPAND);
cvar_t sql_username = CVARF("sv_sql_username", "", CVAR_NOUNSAFEEXPAND);
cvar_t sql_password = CVARF("sv_sql_password", "", CVAR_NOUNSAFEEXPAND);
cvar_t sql_defaultdb = CVARF("sv_sql_defaultdb", "", CVAR_NOUNSAFEEXPAND);

void SQL_PushResult(sqlserver_t *server, queryresult_t *qres)
{
	Sys_LockMutex(server->resultlock);
	qres->next = NULL;
	if (!server->resultslast)
		server->results = server->resultslast = qres;
	else
		server->resultslast = server->resultslast->next = qres;
	Sys_UnlockMutex(server->resultlock);
}

queryresult_t *SQL_PullResult(sqlserver_t *server)
{
	queryresult_t *qres;
	Sys_LockMutex(server->resultlock);
	qres = server->results;
	if (qres)
	{
		server->results = qres->next;
		if (!server->results)
			server->resultslast = NULL;
	}
	Sys_UnlockMutex(server->resultlock);

	return qres;
}

//called by main thread
void SQL_PushRequest(sqlserver_t *server, queryrequest_t *qreq)
{
	qreq->state = SR_PENDING;
	Sys_LockConditional(server->requestcondv);
	qreq->nextqueue = NULL;
	if (!server->requestslast)
		server->requestqueue = server->requestslast = qreq;
	else
		server->requestslast = server->requestslast->nextqueue = qreq;
	Sys_UnlockConditional(server->requestcondv);
}

//called by sql worker thread
queryrequest_t *SQL_PullRequest(sqlserver_t *server, qboolean lock)
{
	queryrequest_t *qreq;
	if (lock)
		Sys_LockConditional(server->requestcondv);
	qreq = server->requestqueue;
	if (qreq)
	{
		server->requestqueue = qreq->nextqueue;
		if (!server->requestqueue)
			server->requestslast = NULL;
	}
	Sys_UnlockConditional(server->requestcondv);

	return qreq;
}

struct
{
	void *owner;
	sqlserver_t *handle;
} *sqlservers;
static int sqlservercount;
static int sqlavailable;
static int sqlinited;

#ifdef USE_SQLITE
//this is to try to sandbox sqlite so it can only edit the file its originally opened with.
int QDECL mysqlite_authorizer(void *ctx, int action, const char *detail0, const char *detail1, const char *detail2, const char *detail3)
{
	if (action == SQLITE_PRAGMA)
	{
		Sys_Printf("SQL: Rejecting pragma \"%s\"\n", detail0);
		return SQLITE_DENY;
	}
	if (action == SQLITE_ATTACH)
	{
		Sys_Printf("SQL: Rejecting attach to \"%s\"\n", detail0);
		return SQLITE_DENY;
	}
	return SQLITE_OK;
}
#endif

int sql_serverworker(void *sref)
{
	sqlserver_t *server = (sqlserver_t *)sref;
	const char *error = NULL;
	int i;
	qboolean needlock = false;
	qboolean allokay = true;
#ifdef USE_MYSQL
	int tinit = -1;
#endif

	switch(server->driver)
	{
#ifdef USE_MYSQL
	case SQLDRV_MYSQL:
		{
			my_bool reconnect = 1;
			if (!qmysql_thread_init)
				error = "MYSQL library not available";
			else if (tinit = qmysql_thread_init())
				error = "MYSQL thread init failed";
			else if (!(server->mysql = qmysql_init(NULL)))
				error = "MYSQL init failed";
			else if (qmysql_options(server->mysql, MYSQL_OPT_RECONNECT, &reconnect))
				error = "MYSQL reconnect options set failed";
			else
			{	
				int port = 0;
				char *colon;

				colon = strchr(server->connectparams[0], ':');
				if (colon)
				{
					*colon = '\0';
					port = atoi(colon + 1);
				}

				if (!(server->mysql = qmysql_real_connect(server->mysql, server->connectparams[0], server->connectparams[1], server->connectparams[2], server->connectparams[3], port, 0, 0)))
					error = "MYSQL initial connect attempt failed";

				if (colon)
					*colon = ':';
			}
		}
		break;
#endif
#ifdef USE_SQLITE
	case SQLDRV_SQLITE:
		if (qsqlite3_open(server->connectparams[3], &server->sqlite))
		{
			error = qsqlite3_errmsg(server->sqlite);
		}
		else
		{
			//disable extension loading, set up an authorizer hook.
			qsqlite3_enable_load_extension(server->sqlite, false);
			qsqlite3_set_authorizer(server->sqlite, mysqlite_authorizer, server);
		}
		break;
#endif
	default:
		error = "That driver is not enabled in this build.";
		break;
	}

	if (error)
		allokay = false;

	while (allokay)
	{	
		Sys_LockConditional(server->requestcondv);
		if (!server->requestqueue) // this is needed for thread startup and to catch any "lost" changes
			Sys_ConditionWait(server->requestcondv);
		needlock = false; // so we don't try to relock first round

		while (1)
		{
			queryrequest_t *qreq = NULL;
			queryresult_t *qres;
			int columns = -1;

			if (!(qreq = SQL_PullRequest(server, needlock)))
			{
				if (!server->active)
					allokay = false;
				break;
			}

			// pullrequest makes sure our condition is unlocked but we'll need
			// a lock next round
			needlock = true;

			switch(server->driver)
			{
			default:
				error = "Bad database driver";
				allokay = false;
				break;
#ifdef USE_MYSQL
			case SQLDRV_MYSQL:
				{
					void *res = NULL;
					int qesize = 0;
					int rows = -1;
					const char *qerror = NULL;
					// perform the query and fill out the result structure
					if (qmysql_query(server->mysql, qreq->query))
						qerror = qmysql_error(server->mysql);
					else // query succeeded
					{
						res = qmysql_store_result(server->mysql);
						if (res) // result set returned
						{
							rows = qmysql_num_rows(res);
							columns = qmysql_num_fields(res);
						}
						else if (qmysql_field_count(server->mysql) == 0) // no result set
						{
							rows = qmysql_affected_rows(server->mysql);
							if (rows < 0)
								rows = 0;
							columns = 0;
						}
						else // error
							qerror = qmysql_error(server->mysql);
				
					}

					if (qerror)
						qesize = Q_strlen(qerror);
					qres = (queryresult_t *)ZF_Malloc(sizeof(queryresult_t) + qesize);
					if (qres)
					{
						if (qerror)
							Q_strncpyz(qres->error, qerror, qesize);
						qres->result = res;
						qres->rows = rows;
						qres->columns = columns;
						qres->request = qreq;
						qres->eof = true; // store result has no more rows to read afterwards

						SQL_PushResult(server, qres);
					}
					else // we're screwed here so bomb out
					{
						server->active = false;
						error = "MALLOC ERROR! Unable to allocate query result!";
						break;
					}
				}
				break;
#endif
#ifdef USE_SQLITE
			case SQLDRV_SQLITE:
				{
					int rc;
					sqlite3_stmt *pStmt;
					const char *trailingstring;
					char *statementstring = qreq->query;
					sqliteresult_t *mat;
					int rowspace;
					int totalrows = 0;
					qboolean keeplooping = true;

//					Sys_Printf("processing %s\n", statementstring);
//					qsqlite3_mutex_enter(server->sqlite->mutex);
//					while(*statementstring)
//					{
						if (qsqlite3_prepare(server->sqlite, statementstring, -1, &pStmt, &trailingstring) == SQLITE_OK)
						{	//sql statement is valid, apparently.
							columns = qsqlite3_column_count(pStmt);

							rc = qsqlite3_step(pStmt);
							while(keeplooping)
							{
								rowspace = 65;

								qres = (queryresult_t *)ZF_Malloc(sizeof(queryresult_t) + columns * sizeof(sqliteresult_t) * rowspace);
								mat = (sqliteresult_t*)(qres + 1);
								if (qres)
								{
									qres->result = mat;
									qres->rows = 0;
									qres->columns = columns;
									qres->request = qreq;
									qres->firstrow = totalrows;
									qres->eof = false;
									qreq->nextqueue = NULL;

									//headers technically take a row.
									for (i = 0; i < columns; i++)
									{
										mat[i].ptr = strdup(qsqlite3_column_name(pStmt, i));
										mat[i].len = 0;
									}
									rowspace--;
									mat += columns;

									while(1)
									{
										if (rc == SQLITE_ROW)
										{
											if (!rowspace)
												break;

											//generate the row info
											for (i = 0; i < columns; i++)
											{
												const char *data = qsqlite3_column_text(pStmt, i);
												mat[i].len = qsqlite3_column_bytes(pStmt, i);
												mat[i].ptr = malloc(mat[i].len+1);
												memcpy(mat[i].ptr, data, mat[i].len);
												mat[i].ptr[mat[i].len] = 0;	//make sure blobs are null terminated, in case someone reads them as a string.
											}
											qres->rows++;
											totalrows++;
											rowspace--;
											mat += columns;
										}
										else if (rc == SQLITE_DONE)
										{
											//no more data to get.
											keeplooping = false;
											qres->eof = true;	//this one was the ender.
											break;
										}
										else
										{
											Sys_Printf("sqlite error code %i: %s\n", rc, statementstring);
											keeplooping = false;
											qres->eof = true;	//this one was the ender.
											if (!qres->columns)
												qres->columns = -1;
											break;
										}

										rc = qsqlite3_step(pStmt);
									}
								}
								else
									keeplooping = false;

								SQL_PushResult(server, qres);
							}
							qsqlite3_finalize(pStmt);
						}
						else
						{
							qres = (queryresult_t *)ZF_Malloc(sizeof(queryresult_t) + 18 + strlen(statementstring));
							if (qres)
							{
								strcpy(qres->error, "Bad SQL statement ");
								strcpy(qres->error+18, statementstring);
								qres->result = NULL;
								qres->rows = 0;
								qres->columns = -1;
								qres->firstrow = totalrows;
								qres->request = qreq;
								qres->eof = true;
								qreq->nextqueue = NULL;

								SQL_PushResult(server, qres);
							}
							break;
						}
//						statementstring = trailingstring;
//					}
//					qsqlite3_mutex_leave(server->sqlite->mutex);
				}
				break;
#endif
			}
		}
	}
	server->active = false;

	switch(server->driver)
	{
#ifdef USE_MYSQL
	case SQLDRV_MYSQL:
		if (server->mysql)
			qmysql_close(server->mysql);
		break;
#endif
#ifdef USE_SQLITE
	case SQLDRV_SQLITE:
		qsqlite3_close(server->sqlite);
		server->sqlite = NULL;
		break;
#endif
	default:
		break;
	}

	// if we have a server error we still need to put it on the queue
	if (error)
	{ 
		int esize = Q_strlen(error);
		queryresult_t *qres = (queryresult_t *)Z_Malloc(sizeof(queryresult_t) + esize);
		if (qres)
		{ // hopefully the qmysql_close gained us some memory otherwise we're pretty screwed
			qres->rows = qres->columns = -1;
			Q_strncpyz(qres->error, error, esize);

			SQL_PushResult(server, qres);
		}
	}

#ifdef USE_MYSQL
	if (!tinit)
		qmysql_thread_end();
#endif

	server->terminated = true;
	return 0;
}

sqlserver_t *SQL_GetServer (void *owner, int serveridx, qboolean inactives)
{
	if (serveridx < 0 || serveridx >= sqlservercount)
		return NULL;
	if (owner && sqlservers[serveridx].owner != owner)
		return NULL;
	if (!sqlservers[serveridx].handle)
		return NULL;
	if (!inactives && sqlservers[serveridx].handle->active == false)
		return NULL;
	return sqlservers[serveridx].handle;
}

queryrequest_t *SQL_GetQueryRequest (sqlserver_t *server, int queryidx)
{
	queryrequest_t *qreq;
	for (qreq = server->requests; qreq; qreq = qreq->nextreq)
	{
		if (qreq->num == queryidx)
			return qreq;
	}

	return NULL;
}

queryresult_t *SQL_GetQueryResult (sqlserver_t *server, int queryidx, int row)
{
	queryresult_t *qres;
	queryrequest_t *qreq;

	qreq = SQL_GetQueryRequest(server, queryidx);
	for (qres = qreq->results; qres; qres = qres->next)
		if (qres->request && qres->request->num == queryidx && (row >= qres->firstrow || (row == -1 && !qres->firstrow)) && row < qres->firstrow + qres->rows)
			return qres;

	return NULL;
}

static void SQL_DeallocResult(sqlserver_t *server, queryresult_t *qres)
{
	// deallocate current result
	switch(server->driver)
	{
	default:
		break;
#ifdef USE_MYSQL
	case SQLDRV_MYSQL:
		if (qres->result)
			qmysql_free_result(qres->result);
		break;
#endif
#ifdef USE_SQLITE
	case SQLDRV_SQLITE:
		if (qres->result)
		{
			sqliteresult_t *mat = qres->result;
			int i;
			for (i = 0; i < qres->columns * (qres->rows+1); i++)
				free(mat[i].ptr);
		}
		break;
#endif
	}

	Z_Free(qres);
}

static void SQL_CloseRequestResult(sqlserver_t *server, queryresult_t *qres)
{
	queryresult_t *prev, *cur;

	prev = qres->request->results;
	if (prev == qres)
	{
		qres->request->results = prev->next;
		SQL_DeallocResult(server, prev);
		return;
	}

	for (cur = prev->next; cur; prev = cur, cur = prev->next)
	{
		if (cur == qres)
		{
			prev = cur->next;
			SQL_DeallocResult(server, cur);
			return;
		}
	}
}

//MT only. flush a result without discarding the request. will result in gaps.
void SQL_CloseResult(sqlserver_t *server, queryresult_t *qres)
{
	if (!qres)
		return;
/*	if (qres == server->currentresult)
	{
		SQL_DeallocResult(server, server->currentresult);
		server->currentresult = NULL;
		return;
	}
*/
	// else we have a persistant query
	SQL_CloseRequestResult(server, qres);
}

//MT only. releases the request.
void SQL_CloseRequest(sqlserver_t *server, queryrequest_t *qreq, qboolean force)
{
	while(qreq->results)
	{
		SQL_CloseResult(server, qreq->results);
	}
	//if the worker thread is still active with it for whatever reason, flag it as aborted but keep it otherwise valid. actually close it later on when we get the results back.
	if (qreq->state != SR_FINISHED && qreq->state != SR_NEW && !force)
		qreq->state = SR_ABORTED;
	else
	{
		queryrequest_t *prev, *cur;

		//unlink and free it now that its complete.
		prev = server->requests;
		if (prev == qreq)
			server->requests = prev->nextreq;
		else
		{
			for (cur = prev->nextreq; cur; prev = cur, cur = prev->nextreq)
			{
				if (cur == qreq)
				{
					prev->nextreq = cur->nextreq;
					break;
				}
			}
		}
		Z_Free(qreq);
	}
}

void SQL_CloseAllRequests(sqlserver_t *server)
{
	queryresult_t *oldqres, *qres;

	//we assume lock is either held or the thread has already died. this function isn't (worker)thread safe.

	// close pending results
	qres = server->results;
	while (qres)
	{
		oldqres = qres;
		qres = qres->next;
		SQL_DeallocResult(server, oldqres);
	}

	//now terminate all the requests
	while(server->requests)
	{
		SQL_CloseRequest(server, server->requests, true);
	}

	// close server result
	if (server->serverresult)
	{
		SQL_DeallocResult(server, server->serverresult);
		server->serverresult = NULL;
	}
}

char *SQL_ReadField (sqlserver_t *server, queryresult_t *qres, int row, int col, qboolean fields, size_t *resultsize)
{
	if (resultsize)
		*resultsize = 0;
	if (!qres->result)
		return NULL;
	else
	{ // store_result query
		row -= qres->firstrow;
		if (qres->rows < row || qres->columns < col || col < 0)
			return NULL;

		if (row < 0)
		{ // fetch field name
			if (fields) // but only if we asked for them
			{
				switch(server->driver)
				{
#ifdef USE_MYSQL
				case SQLDRV_MYSQL:
					{
						MYSQL_FIELD *field = qmysql_fetch_field_direct(qres->result, col);

						if (resultsize)
							*resultsize = 0;
						if (!field)
							return NULL;
						else
							return field->name;
					}
#endif
#ifdef USE_SQLITE
				case SQLDRV_SQLITE:
					{
						sqliteresult_t *mat = qres->result;
						if (mat)
						{
							if (resultsize)
								*resultsize = mat[col].len;
							return mat[col].ptr;
						}
					}
					return NULL;
#endif
				default:
					return NULL;
				}
			}
			else
				return NULL;
		}
		else
		{ // fetch data
			switch(server->driver)
			{
#ifdef USE_MYSQL
			case SQLDRV_MYSQL:
				{
					MYSQL_ROW sqlrow;
					unsigned long *lengths;

					qmysql_data_seek(qres->result, row);
					sqlrow = qmysql_fetch_row(qres->result);
					lengths = qmysql_fetch_lengths(qres->result);
					if (resultsize)
						*resultsize = lengths?lengths[col]:0;
					if (!sqlrow || !sqlrow[col])
						return NULL;
					else
						return sqlrow[col];
				}
#endif
#ifdef USE_SQLITE
				case SQLDRV_SQLITE:
					{
						sqliteresult_t *mat = qres->result;
						col += qres->columns * (row+1);
						if (mat)
						{
							if (resultsize)
								*resultsize = mat[col].len;
							return mat[col].ptr;
						}
					}
					return NULL;
#endif
			default:
				return NULL;
			}
		}
	}
}

void SQL_CleanupServer(sqlserver_t *server)
{
	int i;
	queryrequest_t *qreq, *oldqreq;

	server->active = false; // set thread to kill itself
	if (server->requestcondv)
		Sys_ConditionBroadcast(server->requestcondv); // force condition check
	if (server->thread)
		Sys_WaitOnThread(server->thread); // wait on thread to die

	// server resource deallocation (TODO: should this be done in the thread itself?)
	if (server->requestcondv)
		Sys_DestroyConditional(server->requestcondv);
	if (server->resultlock)
		Sys_DestroyMutex(server->resultlock);
	
	// close orphaned requests
	qreq = server->requestqueue;
	while (qreq)
	{
		oldqreq = qreq;
		qreq = qreq->nextqueue;
		Z_Free(oldqreq);
	}

	SQL_CloseAllRequests(server);

	for (i = SQL_CONNECT_STRUCTPARAMS; i < SQL_CONNECT_PARAMS; i++)
		Z_Free(server->connectparams[i]);
	if (server->connectparams)
		BZ_Free(server->connectparams);

	Z_Free(server);
}

int SQL_NewServer(void *owner, const char *driver, const char **paramstr)
{
	sqlserver_t *server;
	int serverref;
	int drvchoice;
	int paramsize[SQL_CONNECT_PARAMS];
	char nativepath[MAX_OSPATH];
	int i, tsize;

	//name matches
	if (Q_strcasecmp(driver, "mysql") == 0)
		drvchoice = SQLDRV_MYSQL;
	else if (Q_strcasecmp(driver, "sqlite") == 0)
		drvchoice = SQLDRV_SQLITE;
	else if (!*driver && (sqlavailable & (1u<<SQLDRV_SQLITE)))
		drvchoice = SQLDRV_SQLITE;
	else if (!*driver && (sqlavailable & (1u<<SQLDRV_MYSQL)))
		drvchoice = SQLDRV_MYSQL;
	else // invalid driver choice so we bomb out
		return -1;

	if (!SQL_Available())	//also makes sure the drivers are actually loaded.
		return -1;

	if (!(sqlavailable & (1u<<drvchoice)))
		return -1;

	if (drvchoice == SQLDRV_SQLITE)
	{
		//sqlite can be sandboxed.
		//explicitly allow 'temp' and 'memory' databases
		if (*paramstr[3] && strcmp(paramstr[3], ":memory:"))
		{
			//anything else is sandboxed into a subdir/database.
			char *qname = va("sqlite/%s.db", paramstr[3]);
			if (!FS_SystemPath(qname, FS_GAMEONLY, nativepath, sizeof(nativepath)))
				return -1;
			paramstr[3] = nativepath;
			FS_CreatePath(qname, FS_GAMEONLY);
		}
	}

	for (i = 0; i < SQL_CONNECT_PARAMS; i++)
		paramsize[i] = Q_strlen(paramstr[i]);

	// alloc or realloc sql servers array
	if (sqlservers == NULL)
	{
		serverref = 0;
		sqlservercount = 1;
		sqlservers = BZ_Malloc(sizeof(*sqlservers));
	}
	else
	{
		serverref = sqlservercount;
		sqlservercount++;
		sqlservers = BZ_Realloc(sqlservers, sizeof(*sqlservers) * sqlservercount);
	}

	// assemble server structure
	tsize = 0;
	for (i = 0; i < SQL_CONNECT_STRUCTPARAMS; i++)
		tsize += paramsize[i] + 1;	// allocate extra space for host and user only

	server = (sqlserver_t *)Z_Malloc(sizeof(sqlserver_t) + tsize);
	server->connectparams = (char **)BZ_Malloc(sizeof(char *) * SQL_CONNECT_PARAMS);

	tsize = 0;
	for (i = 0; i < SQL_CONNECT_STRUCTPARAMS; i++)
	{
		server->connectparams[i] = ((char *)(server + 1)) + tsize;
		Q_strncpyz(server->connectparams[i], paramstr[i], paramsize[i]);
		// string should be null-terminated due to Z_Malloc
		tsize += paramsize[i] + 1;
	}
	for (i = SQL_CONNECT_STRUCTPARAMS; i < SQL_CONNECT_PARAMS; i++)
	{
		server->connectparams[i] = (char *)Z_Malloc(sizeof(char) * (paramsize[i] + 1));
		Q_strncpyz(server->connectparams[i], paramstr[i], paramsize[i]);
		// string should be null-terminated due to Z_Malloc
	}

	sqlservers[serverref].owner = owner;
	sqlservers[serverref].handle = server;

	server->driver = (sqldrv_t)drvchoice;
	server->querynum = 1;
	server->active = true;
	server->requestcondv = Sys_CreateConditional();
	server->resultlock = Sys_CreateMutex();

	if (!server->requestcondv || !server->resultlock)
	{
		if (server->requestcondv)
			Sys_DestroyConditional(server->requestcondv);
		if (server->resultlock)
			Sys_DestroyMutex(server->resultlock);
		Z_Free(server);
		sqlservercount--;
		return -1;
	}

	server->thread = Sys_CreateThread("sqlworker", sql_serverworker, (void *)server, THREADP_NORMAL, 1024);
	
	if (!server->thread)
	{
		Z_Free(server);
		sqlservercount--;
		return -1;
	}

	return serverref;
}

int SQL_NewQuery(sqlserver_t *server, qboolean (*callback)(queryrequest_t *req, int firstrow, int numrows, int numcols, qboolean eof), const char *str, queryrequest_t **reqout)
{
	int qsize = Q_strlen(str)+1;
	queryrequest_t *qreq;
	int querynum;

	qreq = (queryrequest_t *)ZF_Malloc(sizeof(queryrequest_t) + qsize);
	if (qreq)
	{
		qreq->state = SR_NEW;
		querynum = qreq->num = server->querynum;
		// prevent the reference num from getting too big to prevent FP problems
		while(1)
		{
			if (++server->querynum > (1<<21))
				server->querynum = 1; 

			if (!SQL_GetQueryRequest(server, server->querynum))
				break;
		}
		
		qreq->callback = callback;
		Q_strncpyz(qreq->query, str, qsize);

		qreq->nextreq = server->requests;
		server->requests = qreq;

		SQL_PushRequest(server, qreq);
		Sys_ConditionSignal(server->requestcondv);

		if (reqout)
			*reqout = qreq;
		return querynum;
	}

	if (reqout)
		*reqout = NULL;
	return -1;
}

void SQL_Disconnect(sqlserver_t *server)
{
	server->active = false;

	// force the threads to reiterate requests and hopefully terminate
	Sys_ConditionBroadcast(server->requestcondv);
}

void SQL_Escape(sqlserver_t *server, const char *src, char *dst, int dstlen)
{
	switch (server->driver)
	{
#ifdef USE_MYSQL
	case SQLDRV_MYSQL:
		{
			int srclen = strlen(dst);
			if (srclen > (dstlen / 2 - 1))
				dst[0] = '\0';
			else
				qmysql_real_escape_string(server->mysql, dst, src, srclen);
		}
		break;
#endif
#ifdef USE_SQLITE
	case SQLDRV_SQLITE:
		{
			dstlen--;
			while (dstlen > 2 && *src)
			{
				if (*src == '\'')
				{
					*dst++ = *src;
					dstlen--;
				}
				*dst++ = *src++;
				dstlen--;
			}
			*dst = '\0';
		}
		break;
#endif
	default:
		dst[0] = '\0';
	}
}

const char *SQL_Info(sqlserver_t *server)
{
	switch (server->driver)
	{
#ifdef USE_MYSQL
	case SQLDRV_MYSQL:
		if (qmysql_get_client_info)
			return va("mysql: %s", qmysql_get_client_info());
		else
			return "ERROR: mysql library not loaded";
		break;
#endif
#ifdef USE_SQLITE
	case SQLDRV_SQLITE:
		if (qsqlite3_libversion)
			return va("sqlite: %s", qsqlite3_libversion());
		else
			return "ERROR: sqlite library not loaded";
#endif
	default:
		return "unknown";
	}
}

qboolean SQL_Available(void)
{
	if (!sqlinited)
	{
		sqlinited = true;
#ifdef USE_MYSQL
		//mysql pokes network etc. there's no sandbox. people can use quake clients to pry upon private databases.
		if (COM_CheckParm("-mysql"))
			if (SQL_MYSQLInit())
				sqlavailable |= 1u<<SQLDRV_MYSQL;
#endif
#ifdef USE_SQLITE
		//our sqlite implementation is sandboxed. we block database attachments, and restrict the master database name.
#ifdef _WIN32
		sqlitehandle = Sys_LoadLibrary("sqlite3", sqlitefuncs);
#else	
		sqlitehandle = Sys_LoadLibrary("libsqlite3.so.0", sqlitefuncs);	//at least on debian.
#endif
		if (sqlitehandle)
		{
			sqlavailable |= 1u<<SQLDRV_SQLITE;
		}
#endif
	}

	return !!sqlavailable;
}

/* SQL related commands */
void SQL_Status_f(void)
{
	int i;
	char *stat;

	SQL_Available();	//also ensures any drivers are loaded.
#ifdef USE_MYSQL
	if (!COM_CheckParm("-mysql"))
		Con_Printf("mysql: %s\n", "requires -mysql cmdline argument");
	else
		Con_Printf("mysql: %s\n", (sqlavailable&(1u<<SQLDRV_MYSQL))?"loaded":"unavailable");
#else
	Con_Printf("mysql: %s\n", "disabled at compile time");
#endif
#ifdef USE_SQLITE
	Con_Printf("sqlite: %s\n", (sqlavailable&(1u<<SQLDRV_SQLITE))?"loaded":"unavailable");
#else
	Con_Printf("sqlite: %s\n", "disabled at compile time");
#endif
	
	Con_Printf("%i connections\n", sqlservercount);
	for (i = 0; i < sqlservercount; i++)
	{
		int reqnum = 0;
		int resnum = 0;
		queryrequest_t *qreq;
		queryresult_t *qres;

		sqlserver_t *server = sqlservers[i].handle;

		if (!server)
			continue;

		Sys_LockMutex(server->resultlock);
		Sys_LockConditional(server->requestcondv);
		for (qreq = server->requests; qreq; qreq = qreq->nextreq)
			reqnum++;
		for (qres = server->results; qres; qres = qres->next)
			resnum++;

		switch(server->driver)
		{
		case SQLDRV_MYSQL:
			Con_Printf("#%i %s@%s: %s\n",
				i,
				server->connectparams[1],
				server->connectparams[0],
				server->active ? "active" : "inactive");
			break;
		case SQLDRV_SQLITE:
			Con_Printf("#%i %s: %s\n",
				i,
				server->connectparams[3],
				server->active ? "active" : "inactive");
			break;
		default:
			Con_Printf("Bad driver\n");
			break;
		}

		if (reqnum)
		{
			Con_Printf ("- %i requests\n", reqnum);
			for (qreq = server->requests; qreq; qreq = qreq->nextqueue)
			{
				switch (qreq->state)
				{
				case SR_NEW:		stat = "new";		break;
				case SR_PENDING:	stat = "pending";	break;
				case SR_PARTIAL:	stat = "partial";	break;
				case SR_FINISHED:	stat = "finished";	break;
				case SR_ABORTED:	stat = "aborted";	break;
				default:			stat = "???";		break;
				}
				Con_Printf ("  query #%i (%s): %s\n",
					qreq->num,
					stat,
					qreq->query);
				// TODO: function lookup?
			}
		}

		if (resnum)
		{
			Con_Printf ("- %i pending results\n", resnum);
			for (qres = server->results; qres; qres = qres->next)
			{
				Con_Printf ("  * %i rows, %i columns", 
					qres->rows,
					qres->columns);
				if (qres->error[0])
					Con_Printf(", error %s\n", qres->error);
				else
					Con_Printf("\n");
				// TODO: request info?
			}
		}

		if (server->serverresult)
			Con_Printf ("server result: error %s\n", server->serverresult->error);

		// TODO: list all requests, results here
		Sys_UnlockMutex(server->resultlock);
		Sys_UnlockConditional(server->requestcondv);
	}
}

void SQL_Kill_f (void)
{
	sqlserver_t *server;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("Syntax: %s serverid\n", Cmd_Argv(0));
		return;
	}

	server = SQL_GetServer(NULL, atoi(Cmd_Argv(1)), false);
	if (server)
	{
		server->active = false;
		Sys_ConditionBroadcast(server->requestcondv);
		return;
	}
}

void SQL_Killall_f (void)
{
	SQL_KillServers(NULL);
}

void SQL_ServerCycle (void)
{
	int i;

	for (i = 0; i < sqlservercount; i++)
	{
		sqlserver_t *server = sqlservers[i].handle;
		queryresult_t *qres;
		queryrequest_t *qreq;

		if (!server)
			continue;

		while ((qres = SQL_PullResult(server)))
		{
			qreq = qres->request;
			qres->next = NULL;
			if (qreq && qreq->callback)
			{
				qboolean persist;

				//save it for later.
				qres->next = qreq->results;
				qreq->results = qres;

				if (developer.ival)
					if (*qres->error)
						Con_Printf("%s\n", qres->error);
				if (qreq->state == SR_ABORTED)
				{
					persist = false;
					if (qres->eof)
						SQL_CloseRequest(server, qreq, true);
				}
				else
				{
					if (qreq->state == SR_FINISHED)
						Sys_Error("SQL: Results after finished!\n");
					if (qreq->state == SR_PENDING)
						qreq->state = SR_PARTIAL;
					if (qres->eof)
						qreq->state = SR_FINISHED;
					persist = qreq->callback(qreq, qres->firstrow, qres->rows, qres->columns, qres->eof);
				}

				if (!persist)
				{
					SQL_CloseResult(server, qres);
					if (qreq->state == SR_FINISHED)
						SQL_CloseRequest(server, qreq, false);
				}
			}
			else // error or server-only result
			{
				if (server->serverresult)
					SQL_CloseResult(server, server->serverresult);
				server->serverresult = qres;
			}
		}

		if (server->terminated)
		{
			sqlservers[i].handle = NULL;
			SQL_CleanupServer(server);
			continue;
		}
	}
}

#ifdef USE_MYSQL
qboolean SQL_MYSQLInit(void)
{
	if ((mysqlhandle = Sys_LoadLibrary("libmysql", mysqlfuncs)))
	{
		if (qmysql_thread_safe())
		{
			if (!qmysql_library_init(0, NULL, NULL))
			{
				Con_Printf("MYSQL backend loaded\n");
				return true;
			}
			else
				Con_Printf("MYSQL library init failed!\n");
		}
		else
			Con_Printf("MYSQL client is not thread safe!\n");

		Sys_CloseLibrary(mysqlhandle);
	}
	else
	{
		Con_Printf("mysql client didn't load\n");
	}
	return false;
}
#endif

void SQL_Init(void)
{
	sqlavailable = 0;

	Cmd_AddCommand ("sqlstatus", SQL_Status_f);
	Cmd_AddCommand ("sqlkill", SQL_Kill_f);
	Cmd_AddCommand ("sqlkillall", SQL_Killall_f);

	Cvar_Register(&sql_driver, SQLCVAROPTIONS);
	Cvar_Register(&sql_host, SQLCVAROPTIONS);
	Cvar_Register(&sql_username, SQLCVAROPTIONS);
	Cvar_Register(&sql_password, SQLCVAROPTIONS);
	Cvar_Register(&sql_defaultdb, SQLCVAROPTIONS);
}

void SQL_KillServers(void *owner)
{
	int i;
	for (i = sqlservercount; i-- > 0; )
	{
		if (!owner || sqlservers[i].owner == owner)
		{
			sqlserver_t *server = sqlservers[i].handle;
			sqlservers[i].handle = NULL;
			sqlservers[i].owner = NULL;
			if (server)
				SQL_CleanupServer(server);

			if (sqlservercount == i+1)
				sqlservercount--;
		}
	}
	if (!sqlservercount)
	{
		if (sqlservers)
			Z_Free(sqlservers);
		sqlservers = NULL;
	}
}

void SQL_DeInit(void)
{
	sqlavailable = 0;

	SQL_KillServers(NULL);
	sqlinited = false;

#ifdef USE_MYSQL
	if (qmysql_library_end)
		qmysql_library_end();

	if (mysqlhandle)
		Sys_CloseLibrary(mysqlhandle);
#endif
#ifdef USE_SQLITE
	if (sqlitehandle)
		Sys_CloseLibrary(sqlitehandle);
#endif
}

#endif
