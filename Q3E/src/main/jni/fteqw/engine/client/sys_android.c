#include <errno.h>

#include "quakedef.h"
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include "glquake.h"

#include "q3e/q3e_android.h"

#define Q3E_GAME_NAME "FTEQW"
#define Q3E_IS_INITIALIZED (host_initialized)
#define Q3E_PRINTF Sys_Printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool qboolean
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

extern void GLimp_AndroidOpenWindow(volatile ANativeWindow *win);
extern void GLimp_AndroidInit(volatile ANativeWindow *win);
extern void GLimp_AndroidQuit(void);
extern void ShutdownGame(void);

#include "q3e/q3e_android.inc"

qboolean GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

void ShutdownGame(void)
{
	if(q3e_running/* && host_initialized*/)
	{
		q3e_running = qfalse;
		NOTIFY_EXIT;
	}
}

void Q3E_KeyEvent(int state,int key,int character)
{
	IN_KeyEvent(0, state, key, character);
}

#define HARM_RELATIVE_MOUSE_MOVE 0
void Q3E_MotionEvent(float dx, float dy)
{
    IN_MouseMove(0, HARM_RELATIVE_MOUSE_MOVE, dx, dy, 0, 1);
}

void Sys_SyncState(void)
{
	//if (setState)
	{
		static int prev_state = -1;
		/* We are in game and neither console/ui is active */
		int state;

		if (Key_Dest_Has(kdm_console|kdm_message))
			state = STATE_CONSOLE;
		else if (Key_Dest_Has(kdm_menu))
			state = STATE_MENU;
		//else if (!Key_Dest_Has(~kdm_game) && cls.state == ca_disconnected)
		else
			state = STATE_GAME;

		if (state != prev_state)
		{
			setState(state);
			prev_state = state;
		}
	}
}

void IN_Analog(vec3_t moves, const float forwardspeed, const float backspeed, const float sidespeed)
{
	if (analogenabled)
	{
		if(analogy > 0.0f)
			moves[0] += forwardspeed * analogy;
		else if(analogy < 0.0f)
			moves[0] += backspeed * analogy;
		moves[1] += sidespeed * analogx;
	}
}

char *Sys_Cwd( char cwd[], size_t length )
{
	char *result = getcwd( cwd, length - 1 );
	if( result != cwd )
		return NULL;

	cwd[length-1] = 0;

	return cwd;
}

int main( int argc, char **argv );

#include <android/native_window_jni.h>

#ifndef isDedicated
#ifdef SERVERONLY
qboolean isDedicated = true;
#else
qboolean isDedicated = false;
#endif
#endif
extern int r_blockvidrestart;
static void *sys_memheap;
//static unsigned int vibrateduration;
static char sys_binarydir[MAX_OSPATH];
static char sys_basedir[MAX_OSPATH];
static char sys_basepak[MAX_OSPATH];
extern  jmp_buf 	host_abort;
extern qboolean r_forceheadless;
static qboolean r_forcevidrestart;

extern cvar_t vid_conautoscale;
void VID_Register(void);

#undef LOGI
#undef LOGW
#undef LOGE
#if 1
#define ANDROID_LOG_PRINT(a, b, ...) printf(__VA_ARGS__)
#define ANDROID_LOG_INFO 1
#define ANDROID_LOG_WARN 2
#define ANDROID_LOG_ERROR 3
#else
#include <android/log.h>
#define ANDROID_LOG_PRINT(...) __android_log_print(__VA_ARGS__)
#endif
#ifndef LOGI
#define LOGI(...) ((void)ANDROID_LOG_PRINT(ANDROID_LOG_INFO, DISTRIBUTION"Droid", __VA_ARGS__))
#endif
#ifndef LOGW
#define LOGW(...) ((void)ANDROID_LOG_PRINT(ANDROID_LOG_WARN, DISTRIBUTION"Droid", __VA_ARGS__))
#endif
#ifndef LOGE
#define LOGE(...) ((void)ANDROID_LOG_PRINT(ANDROID_LOG_ERROR, DISTRIBUTION"Droid", __VA_ARGS__))
#endif

qboolean mouseactive;
extern void Android_GrabMouseCursor(qboolean grabIt);
void IN_ActivateMouse(void)
{
	if (mouseactive)
		return;

	mouseactive = true;

	Android_GrabMouseCursor( 1 );
}

void IN_DeactivateMouse(void)
{
	if (!mouseactive)
		return;

	mouseactive = false;
	Android_GrabMouseCursor( 0 );
}



void INS_Move(void)
{
}
void INS_Commands(void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
}
void INS_Init(void)
{
	IN_ActivateMouse();
}
void INS_ReInit(void)
{
	mouseactive = false;
	IN_ActivateMouse();
}
void INS_Shutdown(void)
{
	IN_DeactivateMouse();
}
enum controllertype_e INS_GetControllerType(int id)
{
	return CONTROLLER_NONE;
}
void INS_Rumble(int joy, uint16_t amp_low, uint16_t amp_high, uint32_t duration)
{
}
void INS_RumbleTriggers(int joy, uint16_t left, uint16_t right, uint32_t duration)
{
}
void INS_SetLEDColor(int id, vec3_t color)
{
}
void INS_SetTriggerFX(int id, const void *data, size_t size)
{
}
void Sys_Vibrate(float count)
{
}

qboolean INS_KeyToLocalName(int qkey, char *buf, size_t bufsize)
{	//onscreen keyboard? erk.
	return false;
}

static void setsoftkeyboard(int flags)
{
    Android_OpenKeyboard();
}
static void showMessageAndQuit(const char *errormsg)
{
    if(errormsg)
        Android_ShowInfo(errormsg);
    Q3E_exit();
}

static qboolean FTENativeActivity_wantrelative()
{
	if (!in_windowed_mouse.ival)	//emulators etc have no grabs so we're effectively always windowed in such situations.
		return false;
	return !Key_MouseShouldBeFree();
}

int main(int argc, char **argv)
{
    char tmp[MAX_OSPATH] = { 0 };
    char *result = Sys_Cwd( tmp, sizeof( tmp ) );
    if (result)
    {
        Q_strncpyz(sys_basedir, tmp, sizeof(sys_basedir));
        if (*sys_basedir && sys_basedir[strlen(sys_basedir)-1] != '/')
            Q_strncatz(sys_basedir, "/", sizeof(sys_basedir));
    }
    else
        *sys_basedir = 0;
    if (result)
    {
        Q_strncpyz(sys_binarydir, tmp, sizeof(sys_binarydir));
        if (*sys_binarydir && sys_binarydir[strlen(sys_binarydir)-1] != '/')
            Q_strncatz(sys_binarydir, "/", sizeof(sys_binarydir));
    }
    else
        *sys_binarydir = 0;

    LOGI("FTENativeActivity_startup: basedir=%s binarydir=%s\n", sys_basedir, sys_binarydir);

	int osk = 0, wantgrabs = 0, t;
	double newtime,oldtime=Sys_DoubleTime(), time, sleeptime;
	r_forceheadless = true;
	vid.activeapp = true;

	if (!host_initialized)
	{
		static quakeparms_t parms;
		if (sys_memheap)
			free(sys_memheap);
		memset(&parms, 0, sizeof(parms));
		parms.binarydir = sys_binarydir;
		parms.basedir = sys_basedir;	/*filled in later*/
		parms.argc = argc;
		parms.argv = (const char **)argv;
#ifdef CONFIG_MANIFEST_TEXT
		parms.manifest = CONFIG_MANIFEST_TEXT;
#endif

		Sys_Printf("Starting up (apk=%s, usr=%s)\n", sys_basepak, parms.basedir);

		VID_Register();
		COM_InitArgv(parms.argc, parms.argv);
		TL_InitLanguages(sys_basedir);

		Host_Init(&parms);
		Sys_Printf("Host Inited\n");
	}
	else
		Sys_Printf("Restarting up!\n");

//	run_intent_url();
	/*if (state->savedState != NULL)
	{	//oh look, we're pretending to already be running...
		//oh.
	}*/


	//we're sufficiently done loading. let the ui thread resume.

	r_forcevidrestart = 2;
	while (q3e_running)
	{
		if (r_forcevidrestart)
		{
			r_forceheadless = r_forcevidrestart&1;
			r_forcevidrestart = 0;
		}
		//handle things if the UI thread is blocking for us (video restarts)
		GLimp_CheckGLInitialized();

		if (vid.activeapp)
		{
			// find time spent rendering last frame
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;

			sleeptime = Host_Frame(time);
			oldtime = newtime;

			if (sleeptime)
				Sys_Sleep(sleeptime);
		}
		else
			sleeptime = 0.25;

		t = FTENativeActivity_wantrelative();
		if (wantgrabs != t)
		{
			wantgrabs = t;
		}


		if (sleeptime)
			Sys_Sleep(sleeptime);
	}

	//don't permanently hold these when there's no active activity.
	//(hopefully there's no gl context active right now...)

	return 0;
}

static int secbase;

#ifdef _POSIX_TIMERS
double Sys_DoubleTime(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
	{
		secbase = ts.tv_sec;
		return ts.tv_nsec/1000000000.0;
	}
	return (ts.tv_sec - secbase) + ts.tv_nsec/1000000000.0;
}
unsigned int Sys_Milliseconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
	{
		secbase = ts.tv_sec;
		return ts.tv_nsec/1000000;
	}
	return (ts.tv_sec - secbase)*1000 + ts.tv_nsec/1000000;
}
#else
double Sys_DoubleTime(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
			secbase = tp.tv_sec;
			return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}
unsigned int Sys_Milliseconds(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	return (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
}
#endif

void Sys_Shutdown(void)
{
	free(sys_memheap);
}
void Sys_Quit(void)
{
#ifndef SERVERONLY
	Host_Shutdown ();
#else
	SV_Shutdown();
#endif

	LOGI("%s\n", "quitting");
	showMessageAndQuit("");

	longjmp(host_abort, 1);
	exit(0);
}
void Sys_Error (const char *error, ...)
{
	va_list         argptr;
	char             string[1024];

	va_start (argptr, error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);
	COM_WorkerAbort(string);
	if (!*string)
		strcpy(string, "no error");

	LOGE("e: %s\n", string);
	showMessageAndQuit(string);

	host_initialized = false;	//don't keep calling Host_Frame, because it'll screw stuff up more. Can't trust Host_Shutdown either. :(
	vid.activeapp = false;		//make sure we don't busyloop.
	longjmp(host_abort, 1);
	exit(1);
}
void Sys_Printf (char *fmt, ...)
{
	va_list         argptr;
	char *e;

	//android doesn't do \ns properly *sigh*
	//this means we have to buffer+split it ourselves.
	//and because of lots of threads, we have to mutex it too.
	static char linebuf[2048];
	static char *endbuf = linebuf;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&lock);

	//append the new data
	va_start (argptr, fmt);
	vsnprintf (endbuf,sizeof(linebuf)-(endbuf-linebuf)-1, fmt,argptr);
	va_end (argptr);
	endbuf += strlen(endbuf);

	//split it on linebreaks
	while ((e = strchr(linebuf, '\n')))
	{
		*e = 0;
		LOGI("%s\n", linebuf);
		memmove(linebuf, e+1, endbuf-(e+1));
		linebuf[endbuf-(e+1)] = 0;
		endbuf -= (e+1)-linebuf;
	}

	pthread_mutex_unlock(&lock);
}
void Sys_Warn (char *fmt, ...)
{
	va_list         argptr;
	char             string[1024];

	va_start (argptr, fmt);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	LOGW("w: %s\n", string);
}

void Sys_CloseLibrary(dllhandle_t *lib)
{
	if (lib)
		dlclose(lib);
}
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	return dlsym(module, exportname);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	size_t i;
	dllhandle_t *h;
	char libname[MAX_OSPATH] = { 0 };
	snprintf(libname, sizeof(libname) - 1, "%s.so", name);
	h = dlopen(libname, RTLD_LAZY|RTLD_LOCAL);
	if (!h)
	{
		//Sys_Printf("try 1 dlopen('%s') error -> %s\n", libname, dlerror());
		snprintf(libname, sizeof(libname) - 1, "%s", name);
		h = dlopen(libname, RTLD_LAZY|RTLD_LOCAL);
	}
	if (!h)
	{
		//Sys_Printf("try 2 dlopen('%s') error -> %s\n", libname, dlerror());
		snprintf(libname, sizeof(libname) - 1, "lib%s.so", name);
		h = dlopen(libname, RTLD_LAZY|RTLD_LOCAL);
	}
	if (!h)
	{
		//Sys_Printf("try 3 dlopen('%s') error -> %s\n", libname, dlerror());
		Sys_Printf("dlopen('%s') error -> %s\n", name, dlerror());
		return NULL;
	}

	Sys_Printf("dlopen('%s'): %s\n", libname, name);
	if (h && funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = dlsym(h, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Con_DPrintf("Unable to find symbol \"%s\" in \"%s\"\n", funcs[i].name, name);
			Sys_CloseLibrary(h);
			h = NULL;
		}
	}
	return h;
}
char *Sys_ConsoleInput (void)
{
	return NULL;
}
void Sys_mkdir (const char *path)    //not all pre-unix systems have directories (including dos 1)
{
	mkdir(path, 0755);
}
qboolean Sys_rmdir (const char *path)
{
	if (rmdir (path) == 0)
		return true;
	if (errno == ENOENT)
		return true;
	return false;
}
qboolean Sys_remove (const char *path)
{
	return !unlink(path);
}
qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return !rename(oldfname, newfname);
}

#if _POSIX_C_SOURCE >= 200112L
	#include <sys/statvfs.h>
#endif
qboolean Sys_GetFreeDiskSpace(const char *path, quint64_t *freespace)
{
#if _POSIX_C_SOURCE >= 200112L
	//posix 2001
	struct statvfs inf;
	if(0==statvfs(path, &inf))
	{
		*freespace = inf.f_bsize*(quint64_t)inf.f_bavail;
		return true;
	}
#endif
	return false;
}

void Sys_SendKeyEvents(void)
{
	Sys_SyncState();
	Android_PollInput();
}
char *Sys_URIScheme_NeedsRegistering(void)
{	//android does its mime/etc registrations via android xml junk. dynamically registering stuff isn't supported, so pretend that its already registered to avoid annoying prompts.
	return NULL;
}
void Sys_Init(void)
{
}

qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	*width = screen_width;
	*height = screen_height;
	*bpp = gl_format == 0x0565 ? 16 : 24;
	*refreshrate = refresh_rate;
	return false;
}
qboolean Sys_RandomBytes(qbyte *string, int len)
{
	qboolean res = false;
	int fd = open("/dev/urandom", 0);
	if (fd >= 0)
	{
		res = (read(fd, string, len) == len);
		close(fd);
	}

	return res;
}

void Sys_ServerActivity(void)
{
	/*FIXME: flash window*/
}

#ifdef WEBCLIENT
qboolean Sys_RunInstaller(void)
{       //not implemented
	return false;
}
#endif

#ifndef MULTITHREAD
void Sys_Sleep (double seconds)
{
	struct timespec ts;

	ts.tv_sec = (time_t)seconds;
	seconds -= ts.tv_sec;
	ts.tv_nsec = seconds * 1000000000.0;

	nanosleep(&ts, NULL);
}
#endif
qboolean Sys_InitTerminal(void)
{
	/*switching to dedicated mode, show text window*/
	return false;
}
void Sys_CloseTerminal(void)
{
}

#define SYS_CLIPBOARD_SIZE  256
static char clipboard_buffer[SYS_CLIPBOARD_SIZE] = {0};
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, const char *utf8), void *ctx)
{
	char *text = Android_GetClipboardData();
	if(!text)
		clipboard_buffer[0] = '\0';
	else
	{
		Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
		free(text);
	}
	callback(ctx, clipboard_buffer);
}
void Sys_SaveClipboard(clipboardtype_t cbt, const char *text)
{
	if(!text)
	{
		clipboard_buffer[0] = '\0';
		return;
	}
 	Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
	Android_SetClipboardData(text);
}

int Sys_EnumerateFiles (const char *gpath, const char *match, int (*func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *), void *parm, searchpathfuncs_t *spath)
{
	DIR *dir;
	char apath[MAX_OSPATH];
	char file[MAX_OSPATH];
	char truepath[MAX_OSPATH];
	char *s;
	struct dirent *ent;
	struct stat st;

	//printf("path = %s\n", gpath);
	//printf("match = %s\n", match);

	if (!gpath)
		gpath = "";
	*apath = '\0';

	Q_strncpyz(apath, match, sizeof(apath));
	for (s = apath+strlen(apath)-1; s >= apath; s--)
	{
		if (*s == '/')
		{
			s[1] = '\0';
			match += s - apath+1;
			break;
		}
	}
	if (s < apath)  //didn't find a '/' 
		*apath = '\0'; 

	Q_snprintfz(truepath, sizeof(truepath), "%s/%s", gpath, apath); 


	//printf("truepath = %s\n", truepath); 
	//printf("gamepath = %s\n", gpath); 
	//printf("apppath = %s\n", apath); 
	//printf("match = %s\n", match); 
	dir = opendir(truepath); 
	if (!dir) 
	{ 
		Con_DPrintf("Failed to open dir %s\n", truepath); 
		return true; 
	} 
	do 
	{ 
		ent = readdir(dir); 
		if (!ent) 
			break; 
		if (*ent->d_name != '.') 
		{ 
			if (wildcmp(match, ent->d_name)) 
			{ 
				Q_snprintfz(file, sizeof(file), "%s/%s", truepath, ent->d_name); 

				if (stat(file, &st) == 0) 
				{ 
					Q_snprintfz(file, sizeof(file), "%s%s%s", apath, ent->d_name, S_ISDIR(st.st_mode)?"/":""); 

					if (!func(file, st.st_size, st.st_mtime, parm, spath)) 
					{ 
						closedir(dir); 
						return false; 
					} 
				} 
				else 
					printf("Stat failed for \"%s\"\n", file); 
			} 
		} 
	} while(1); 
	closedir(dir); 

	return true; 
}
