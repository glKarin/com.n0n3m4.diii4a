/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//well, linux or cygwin (windows with posix emulation layer), anyway...

#define _GNU_SOURCE
#include "quakedef.h"
	
#ifdef MULTITHREAD
#include <limits.h>
#include <pthread.h>
/* Thread creation calls */
typedef void *(*pfunction_t)(void *);

static pthread_t mainthread;

void Sys_ThreadsInit(void)
{
	mainthread = pthread_self();
}
qboolean Sys_IsThread(void *thread)
{
	return pthread_equal(pthread_self(), *(pthread_t*)thread);
}
qboolean Sys_IsMainThread(void)
{
	return Sys_IsThread(&mainthread);
}
void Sys_ThreadAbort(void)
{
	pthread_exit(NULL);
}

#if 1
typedef struct {
	int (*func)(void *);
	void *args;
} qthread_t;
static void *Sys_CreatedThread(void *v)
{
	qthread_t *qthread = v;
	qintptr_t r;

	r = qthread->func(qthread->args);

	return (void*)r;
}

void *Sys_CreateThread(char *name, int (*func)(void *), void *args, int priority, int stacksize)
{
	pthread_t *thread;
	qthread_t *qthread;
	pthread_attr_t attr;

	thread = (pthread_t *)malloc(sizeof(pthread_t)+sizeof(qthread_t));
	if (!thread)
		return NULL;

	qthread = (qthread_t*)(thread+1);
	qthread->func = func;
	qthread->args = args;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (stacksize != -1)
	{
		if (stacksize < PTHREAD_STACK_MIN*2)
			stacksize = PTHREAD_STACK_MIN*2;
		if (stacksize < PTHREAD_STACK_MIN+65536*16)
			stacksize = PTHREAD_STACK_MIN+65536*16;
		pthread_attr_setstacksize(&attr, stacksize);
	}
	if (pthread_create(thread, &attr, (pfunction_t)Sys_CreatedThread, qthread))
	{
		free(thread);
		thread = NULL;
	}
	pthread_attr_destroy(&attr);
#if defined(_DEBUG) && defined(__USE_GNU) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2,12)
	pthread_setname_np(*thread, name);
#endif
#endif

	return (void *)thread;
}
#else
void *Sys_CreateThread(char *name, int (*func)(void *), void *args, int priority, int stacksize)
{
	pthread_t *thread;
	pthread_attr_t attr;

	thread = (pthread_t *)malloc(sizeof(pthread_t));
	if (!thread)
		return NULL;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (stacksize < PTHREAD_STACK_MIN*2)
		stacksize = PTHREAD_STACK_MIN*2;
	pthread_attr_setstacksize(&attr, stacksize);
	if (pthread_create(thread, &attr, (pfunction_t)func, args))
	{
		free(thread);
		thread = NULL;
	}
	pthread_attr_destroy(&attr);

#if defined(_DEBUG) && defined(__USE_GNU) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2,12)
	pthread_setname_np(*thread, name);
#endif
#endif

	return (void *)thread;
}
#endif

void Sys_WaitOnThread(void *thread)
{
	int err;
	err = pthread_join(*(pthread_t *)thread, NULL);
	if (err)
		printf("pthread_join(%p) failed, error %s\n", thread, strerror(err));
		
	free(thread);
}

/* Mutex calls */
void *Sys_CreateMutex(void)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

	if (mutex && !pthread_mutex_init(mutex, NULL))
		return mutex;
	return NULL;
}

qboolean Sys_TryLockMutex(void *mutex)
{
	return !pthread_mutex_trylock(mutex);
}

qboolean Sys_LockMutex(void *mutex)
{
	return !pthread_mutex_lock(mutex);
}

qboolean Sys_UnlockMutex(void *mutex)
{
	return !pthread_mutex_unlock(mutex);
}

void Sys_DestroyMutex(void *mutex)
{
	pthread_mutex_destroy(mutex);
	free(mutex);
}

/* Conditional wait calls */
typedef struct condvar_s
{
	pthread_mutex_t *mutex;
	pthread_cond_t *cond;
} condvar_t;

void *Sys_CreateConditional(void)
{
	condvar_t *condv;
	pthread_mutex_t *mutex;
	pthread_cond_t *cond;

	condv = (condvar_t *)malloc(sizeof(condvar_t));
	if (!condv)
		return NULL;

	mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if (!mutex)
		return NULL;

	cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	if (!cond)
		return NULL;

	if (!pthread_mutex_init(mutex, NULL))
	{
		if (!pthread_cond_init(cond, NULL))
		{
			condv->cond = cond;
			condv->mutex = mutex;

			return (void *)condv;
		}
		else
			pthread_mutex_destroy(mutex);
	}

	free(cond);
	free(mutex);
	free(condv);
	return NULL;
}

qboolean Sys_LockConditional(void *condv)
{
	return !pthread_mutex_lock(((condvar_t *)condv)->mutex);
}

qboolean Sys_UnlockConditional(void *condv)
{
	return !pthread_mutex_unlock(((condvar_t *)condv)->mutex);
}

qboolean Sys_ConditionWait(void *condv)
{
	return !pthread_cond_wait(((condvar_t *)condv)->cond, ((condvar_t *)condv)->mutex);
}

qboolean Sys_ConditionSignal(void *condv)
{
	return !pthread_cond_signal(((condvar_t *)condv)->cond);
}

qboolean Sys_ConditionBroadcast(void *condv)
{
	return !pthread_cond_broadcast(((condvar_t *)condv)->cond);
}

void Sys_DestroyConditional(void *condv)
{
	condvar_t *cv = (condvar_t *)condv;

	pthread_cond_destroy(cv->cond);
	pthread_mutex_destroy(cv->mutex);
	free(cv->cond);
	free(cv->mutex);
	free(cv);
}
#endif

void Sys_Sleep (double seconds)
{
	struct timespec ts;

	ts.tv_sec = (time_t)seconds;
	seconds -= ts.tv_sec;
	ts.tv_nsec = seconds * 1000000000.0;

	nanosleep(&ts, NULL);
}




#ifdef SUBSERVERS
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

typedef struct
{
	vfsfile_t pub;

	int inpipe;
	int outpipe;
	pid_t pid;	//so we don't end up with zombie processes
} linsubserver_t;

static int QDECL Sys_MSV_ReadBytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	linsubserver_t *s = (linsubserver_t*)file;
	ssize_t avail = read(s->inpipe, buffer, bytestoread);
	if (!avail)
		return -1;	//EOF
	if (avail < 0)
	{
		int e = errno;
		if (e == EAGAIN || e == EWOULDBLOCK || e == EINTR)
			return 0;	//no data available
		else
		{
			perror("subserver read");
			return -1;	//some sort of error
		}
	}
	return avail;
}
static int QDECL Sys_MSV_WriteBytes (struct vfsfile_s *file, const void *buffer, int bytestowrite)
{
	linsubserver_t *s = (linsubserver_t*)file;
	ssize_t wrote = write(s->outpipe, buffer, bytestowrite);
	if (!wrote)
		return -1;	//EOF
	if (wrote < 0)
	{
		int e = errno;
		if (e == EAGAIN || e == EWOULDBLOCK || e == EINTR)
			return 0;	//no space available
		else
		{
			perror("subserver write");
			return -1;	//some sort of error
		}
	}
	return wrote;
}
static qboolean QDECL Sys_MSV_Close (struct vfsfile_s *file)
{
	linsubserver_t *s = (linsubserver_t*)file;

	close(s->inpipe);
	close(s->outpipe);
	s->inpipe = -1;
	s->outpipe = -1;
	waitpid(s->pid, NULL, 0);
	Z_Free(s);
	return true;
}

#ifdef SQL
#include "sv_sql.h"
#endif
#include "netinc.h"

vfsfile_t *Sys_ForkServer(void)
{
#ifdef SERVERONLY
	extern  jmp_buf 	sys_sv_serverforked;
		
	int toslave[2];
	int tomaster[2];
	linsubserver_t *ctx;
	pid_t pid;

	//make sure we're fully synced, so that workers can't mess up
	Cvar_Set(Cvar_FindVar("worker_count"), "0");
	COM_WorkerFullSync();
#ifdef WEBCLIENT
	DL_DeThread();
#endif
#ifdef SQL
	SQL_KillServers(NULL);	//FIXME: this is bad...
#endif
	//FIXME: we should probably use posix_atfork for those.

	pipe(toslave);
	pipe(tomaster);

	//make the reads non-blocking.
	fcntl(toslave[1], F_SETFL, fcntl(toslave[1], F_GETFL, 0)|O_NONBLOCK);
	fcntl(tomaster[0], F_SETFL, fcntl(tomaster[0], F_GETFL, 0)|O_NONBLOCK);

	pid = fork();

	if (!pid)
	{	//this is the child
		dup2(toslave[0], STDIN_FILENO);
		close(toslave[1]);
		close(toslave[0]);
		dup2(tomaster[1], STDOUT_FILENO);

		SSV_SetupControlPipe(Sys_GetStdInOutStream(), false);

		FS_UnloadPackFiles();	//these handles got wiped. make sure they're all properly wiped before loading new handles.
		NET_Shutdown();			//make sure we close any of the parent's network fds ...
		FS_ReloadPackFiles();

		//jump out into the main work loop
		longjmp(sys_sv_serverforked, 1);
		exit(0);	//err...
	}
	else
	{	//this is the parent
		close(toslave[0]);
		close(tomaster[1]);
		if (pid == -1)
		{	//fork failed. make sure everything is destroyed.
			close(toslave[1]);
			close(tomaster[0]);
			return NULL;
		}

		Con_DPrintf("Forked new server node\n");
		ctx = Z_Malloc(sizeof(*ctx));
	}

	

#else
	int toslave[2];
	int tomaster[2];
	char exename[MAX_OSPATH];
	posix_spawn_file_actions_t action;
	linsubserver_t *ctx;
	char *argv[64];
	int argc = 0;
	int l;

	argv[argc++] = exename;
	argv[argc++] = "-clusterslave";
	argc += FS_GetManifestArgv(argv+argc, countof(argv)-argc-1);
	argv[argc++] = NULL;

#if 0
	strcpy(exename, "/bin/ls");
	args[1] = NULL;
#elif 0
	strcpy(exename, "/tmp/ftedbg/fteqw.sv");
#else
	l = readlink("/proc/self/exe", exename, sizeof(exename)-1);
	if (l <= 0)
		return NULL;
	exename[l] = 0;
#endif
	Con_DPrintf("Execing %s\n", exename);

	ctx = Z_Malloc(sizeof(*ctx));

	pipe(toslave);
	pipe(tomaster);

	//make the reads non-blocking.
	fcntl(toslave[1], F_SETFL, fcntl(toslave[1], F_GETFL, 0)|O_NONBLOCK);
	fcntl(tomaster[0], F_SETFL, fcntl(tomaster[0], F_GETFL, 0)|O_NONBLOCK);

	posix_spawn_file_actions_init(&action);
	posix_spawn_file_actions_addclose(&action, toslave[1]);
	posix_spawn_file_actions_addclose(&action, tomaster[0]);

	posix_spawn_file_actions_adddup2(&action, toslave[0],	STDIN_FILENO);
	posix_spawn_file_actions_adddup2(&action, tomaster[1],	STDOUT_FILENO);
//	posix_spawn_file_actions_adddup2(&action, tomaster[1],	STDERR_FILENO);

	posix_spawn_file_actions_addclose(&action, toslave[0]);
	posix_spawn_file_actions_addclose(&action, tomaster[1]);

	posix_spawn(&ctx->pid, exename, &action, NULL, argv, NULL);
#endif

	ctx->inpipe = tomaster[0];
	close(tomaster[1]);
	close(toslave[0]);
	ctx->outpipe = toslave[1];

	ctx->pub.ReadBytes = Sys_MSV_ReadBytes;
	ctx->pub.WriteBytes = Sys_MSV_WriteBytes;
	ctx->pub.Close = Sys_MSV_Close;
	return &ctx->pub;
}


static int QDECL Sys_StdoutWrite (struct vfsfile_s *file, const void *buffer, int bytestowrite)
{
	ssize_t r = write(STDOUT_FILENO, buffer, bytestowrite);
	if (r == 0 && bytestowrite)
		return -1;	//eof
	if (r < 0)
	{
		int e = errno;
		if (e == EINTR || e == EAGAIN || e == EWOULDBLOCK)
			return 0;
	}
	return r;
}
static int QDECL Sys_StdinRead (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	ssize_t r;
#if defined(__linux__) && defined(_DEBUG)
	int fl = fcntl (STDIN_FILENO, F_GETFL, 0);
	if (!(fl & O_NONBLOCK))
	{
		fcntl(STDIN_FILENO, F_SETFL, fl | O_NONBLOCK);
		Sys_Printf(CON_WARNING "stdin flags became blocking - gdb bug?\n");
	}
#endif

	r = read(STDIN_FILENO, buffer, bytestoread);
	if (r == 0 && bytestoread)
		return -1;	//eof
	if (r < 0)
	{
		int e = errno;
		if (e == EINTR || e == EAGAIN || e == EWOULDBLOCK)
			return 0;
	}
	return r;
}
qboolean QDECL Sys_StdinOutClose(vfsfile_t *fs)
{
	Sys_Error("Shutdown\n");
}
vfsfile_t *Sys_GetStdInOutStream(void)
{
	vfsfile_t *stream = Z_Malloc(sizeof(*stream));

	//make sure nothing bad is going to happen.
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0)|O_NONBLOCK);
	fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL, 0)|O_NONBLOCK);

	stream->WriteBytes = Sys_StdoutWrite;
	stream->ReadBytes = Sys_StdinRead;
	stream->Close = Sys_StdinOutClose;
	return stream;
}
#endif





#ifdef HAVEAUTOUPDATE
#include <sys/stat.h>
#include <unistd.h>
qboolean Sys_SetUpdatedBinary(const char *newbinary)
{
	char enginebinary[MAX_OSPATH];
	char tmpbinary[MAX_OSPATH];
//	char enginebinarybackup[MAX_OSPATH+4];
//	size_t len;
	int i;
	struct stat src, dst;

	//update blocked via commandline. just in case.
	if (COM_CheckParm("-noupdate") || COM_CheckParm("--noupdate") || COM_CheckParm("-noautoupdate") || COM_CheckParm("--noautoupdate"))
		return false;

	//get the binary name
	i = readlink("/proc/self/exe", enginebinary, sizeof(enginebinary)-1);
	if (i <= 0)
		return false;
	enginebinary[i] = 0;

	//generate the temp name
	/*memcpy(enginebinarybackup, enginebinary, sizeof(enginebinary));
	len = strlen(enginebinarybackup);
	if (len > 4 && !strcasecmp(enginebinarybackup+len-4, ".bin"))
		len -= 4;	//swap its extension over, if we can.
	strcpy(enginebinarybackup+len, ".bak");*/

	//copy over file permissions (don't ignore the user)
	if (stat(enginebinary, &dst)<0)
		dst.st_mode = 0777;
	if (stat(newbinary, &src)<0)
		src.st_mode = 0777;

//	if (src.st_dev != dst.st_dev)
	{	//oops, its on a different filesystem. create a copy
		Q_snprintfz(tmpbinary, sizeof(tmpbinary), "%s.new", enginebinary);
		if (!FS_Copy(newbinary, tmpbinary, FS_SYSTEM, FS_SYSTEM))
			return false;
		newbinary = tmpbinary;
	}
	chmod(newbinary, dst.st_mode|S_IXUSR);	//but make sure its executable, just in case...

	//overwrite the name we were started through. this is supposed to be atomic.
	if (rename(newbinary, enginebinary) < 0)
	{
		Con_Printf("Failed to overwrite %s with %s\n", enginebinary, newbinary);
		return false;	//failed
	}
	return true;	//succeeded.
}
qboolean Sys_EngineMayUpdate(void)
{
	char enginebinary[MAX_OSPATH];
	int len;

	//update blocked via commandline
	if (COM_CheckParm("-noupdate") || COM_CheckParm("--noupdate") || COM_CheckParm("-noautoupdate") || COM_CheckParm("--noautoupdate"))
		return false;

	//check that we can actually do it.
	len = readlink("/proc/self/exe", enginebinary, sizeof(enginebinary)-1);
	if (len <= 0)
		return false;

	//if we can't get a revision number from our cflags then don't allow updates (unless forced on).
	if (!COM_CheckParm("-allowupdate"))
	{
		char *s;
		if (revision_number(true)<=0)
			return false;

		//if there's 3 consecutive digits or digit.digit then assume the user is doing their own versioning, and disallow engine updates (unless they use the -allowupdate arg).
		//(that or they ran one of our older builds directly)
		for (s=COM_SkipPath(enginebinary); *s; s++)
		{
			if ( s[0] >= '0' && s[0] <= '9')
			if ((s[1] >= '0' && s[1] <= '9') || s[1] == '.' || s[1] == '_' || s[1] == '-')
			if ( s[2] >= '0' && s[2] <= '9')
				return false;
		}
	}

	enginebinary[len] = 0;
	if (access(enginebinary, R_OK|W_OK|X_OK) < 0)
		return false;	//can't write it. don't try downloading updates.
	*COM_SkipPath(enginebinary) = 0;
	if (access(enginebinary, R_OK|W_OK) < 0)
		return false;	//can't write to the containing directory. this does not bode well for moves/overwrites.


	return true;
}
#endif
