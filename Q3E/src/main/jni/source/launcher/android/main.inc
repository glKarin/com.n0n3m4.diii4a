#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL_hints.h>
#include "tier0/dbg.h"
#include "tier0/threadtools.h"

#include "q3e/q3e_android.h"

void ShutdownGame();

#define Q3E_GAME_NAME "Source"
#define Q3E_IS_INITIALIZED (q3e_initialized)
#define Q3E_PRINTF printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool bool
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

bool q3e_initialized = false;

void GLimp_AndroidOpenWindow(volatile ANativeWindow *win)
{
	if(!win)
		return;

	ANativeWindow_acquire((ANativeWindow *)win);
	Q3E_PRINTF("[Harmattan]: ANativeWindow acquired.\n");
}

void GLimp_AndroidInit(volatile ANativeWindow *win)
{
	if(!win)
		return;

	ANativeWindow_acquire((ANativeWindow *)win);
	Q3E_PRINTF("[Harmattan]: ANativeWindow acquired.\n");
}

void GLimp_AndroidQuit(void)
{
	//ANativeWindow_release((ANativeWindow *)win);
	//win = NULL;
	Q3E_PRINTF("[Harmattan]: ANativeWindow released.\n");
}

void Q3E_MotionEvent(float dx, float dy){}
void Q3E_KeyEvent(int state,int key,int character){}

#include "q3e/q3e_android.inc"

Q3Ebool GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

Q3Ebool Q3E_IsRunning()
{
	return q3e_running;
}

void ShutdownGame() {
	if(q3e_running/* && q3e_initialized*/)
	{
		q3e_running = false;
		NOTIFY_EXIT;
	}
}

void Sys_SyncState(int state)
{
	//if (setState)
	{
		static int prev_state = -1;

		if (state != prev_state)
		{
			setState(state);
			prev_state = state;
		}
	}
}

extern void InitCrashHandler();
DLL_EXPORT int LauncherMain( int argc, char **argv ); // from launcher.cpp

/*
void SetLauncherArgs()
{
#define A(a,b) LauncherArgv[iLastArgs++] = (char*)a; \
	LauncherArgv[iLastArgs++] = (char*)b
#define D(a) LauncherArgv[iLastArgs++] = (char*)a

	static char binPath[2048];
	snprintf(binPath, sizeof binPath, "%s/hl2_linux", getenv("APP_DATA_PATH") );
	D(binPath);

	D("-nouserclip");

	char *pch;

	pch = strtok (java_args," ");
	while (pch != NULL)
	{
		LauncherArgv[iLastArgs++] = pch;
		pch = strtok (NULL, " ");
	}

	D("-fullscreen");
	D("-nosteam");
	D("-insecure");

#undef A
#undef D
}
	*/

float GetTotalMemory()
{
	int64_t mem = 0;

	char meminfo[8196] = { 0 };
	FILE *f = fopen("/proc/meminfo", "r");
	if( !f )
		return 0.f;

	size_t size = fread(meminfo, 1, sizeof(meminfo), f);
	if( !size )
		return 0.f;

	char *s = strstr(meminfo, "MemTotal:");

	if( !s ) return 0.f;

	sscanf(s+9, "%lld", &mem);
	fclose(f);

	return mem/1024/1024.f;
}

void android_property_print(const char *name)
{
	char prop[1024];

	char strValue[64];
	memset (strValue, 0, 64);
	snprintf(prop, sizeof(prop), "getprop %s", name);
	FILE *fp = NULL;
	fp = popen(prop, "r");
	if (!fp) return;

	fgets(strValue, sizeof(strValue), fp);
	pclose(fp);
	fp = NULL;

	Msg("prop %s=%s", name, strValue);
}

typedef struct Q3E_Engine_Callback_s
{
	bool (*IsRunning)();
	bool (*CheckGLInitialized)();
	void (*PullEvents)();
	void (*SyncState)(int mask);
	bool (*AnalogEvent)(float *x, float *y);
} Q3E_Engine_Callback_t;

static bool Q3E_AnalogEvent(float *x, float *y)
{
    if(analogenabled)
    {
        if(x) *x = analogx;
        if(y) *y = analogy;
    }
    return analogenabled;
}

extern void * Q3E_GetProcAddress( HMODULE hModule, const char *pName );
void Q3E_SetupEngineInteraction(HMODULE handle)
{
	if(!handle)
	    return;

    using Q3E_Engine_Interaction_f = void * (*)(const void *callback);
    Q3E_Engine_Interaction_f func = (Q3E_Engine_Interaction_f)Q3E_GetProcAddress(handle, "Q3E_Engine_Interaction");
    printf("Found engine interaction function: %p at %p\n", func, handle);
    if(func)
    {
        Q3E_Engine_Callback_t cb = {
                &Q3E_IsRunning, &GLimp_CheckGLInitialized, &Android_PollInput, &Sys_SyncState, &Q3E_AnalogEvent
        };
        func(&cb);
    }
}


/*DLL_EXPORT*/ int main( int argc, char **argv )
{
    if(!no_handle_signals)
	InitCrashHandler();

	// libraries path: apk install path
	setenv("APP_LIB_PATH", Sys_DLLDefaultPath(), 1);
	// font path: from apk/assets -> data path
	setenv("APP_DATA_PATH", Sys_GameDataDefaultPath(), 1);
	// game resource path: data path
	setenv("VALVE_GAME_PATH", Sys_GameDataDefaultPath(), 1);
	// APP_MOD_LIB

	Msg("GetTotalMemory() = %.2f \n", GetTotalMemory());

	android_property_print("ro.build.version.sdk");
	android_property_print("ro.product.device");
	android_property_print("ro.product.manufacturer");
	android_property_print("ro.product.model");
	android_property_print("ro.product.name");

	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
	DeclareCurrentThreadIsMainThread(); // Init thread propertly on Android

	return LauncherMain(argc, argv);
}

