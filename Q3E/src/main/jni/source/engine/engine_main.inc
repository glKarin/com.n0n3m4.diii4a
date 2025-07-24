
typedef struct Q3E_Engine_Callback_s
{
	bool (*IsRunning)();
	bool (*CheckGLInitialized)();
	void (*PullEvents)();
	void (*SyncState)(int mask);
	bool (*AnalogEvent)(float *x, float *y);
} Q3E_Engine_Callback_t;

static bool (*IsRunning_f)();
static bool (*CheckGLInitialized_f)();
static void (*PullEvents_f)();
static void (*SyncState_f)(int st);
static bool (*AnalogEvent_f)(float *x, float *y);

static bool analogEnabled = false;
static float analogX = 0;
static float analogY = 0;

inline bool Q3E_Engine_IsRunning()
{
    return IsRunning_f && IsRunning_f();
}

inline bool Q3E_Engine_CheckGLInitialized()
{
    return CheckGLInitialized_f && CheckGLInitialized_f();
}

inline void Q3E_Engine_PullEvents()
{
    if(PullEvents_f) PullEvents_f();
}

inline void Q3E_Engine_AnalogEvent()
{
    analogEnabled = AnalogEvent_f && AnalogEvent_f(&analogX, &analogY);
}

#include <GameUI/IGameConsole.h>
#include "vgui_baseui_interface.h"
#include "ienginetoolinternal.h"

extern IGameConsole *staticGameConsole;
inline void Q3E_Engine_SyncState()
{
// Game states
#define STATE_NONE 0
#define STATE_ACT 1 // RTCW4A-specific, keep
#define STATE_GAME (1 << 1) // map spawned
#define STATE_KICK (1 << 2) // RTCW4A-specific, keep
#define STATE_LOADING (1 << 3) // current GUI is guiLoading
#define STATE_CONSOLE (1 << 4) // fullscreen
#define STATE_MENU (1 << 5) // any menu excludes guiLoading
#define STATE_DEMO (1 << 6) // demo

    if(SyncState_f)
    {
        auto engineVGui = EngineVGui();
		int state = STATE_GAME;
		auto panel = engineVGui->GetPanel(PANEL_CLIENTDLL);
		bool inGame = cl.IsActive() && (panel && vgui::ipanel()->IsVisible(panel));
		if(!inGame)
		{
			if(staticGameConsole && staticGameConsole->IsConsoleVisible())
				state = STATE_CONSOLE;
			else if(engineVGui->IsGameUIVisible())
				state = STATE_MENU;
		}
		SyncState_f(state);
    }
}

extern "C" DECLSPEC
void * Q3E_Engine_Interaction(const void *callback)
{
    auto cb = (const Q3E_Engine_Callback_t *)callback;
    IsRunning_f = cb->IsRunning;
    CheckGLInitialized_f = cb->CheckGLInitialized;
    PullEvents_f = cb->PullEvents;
    SyncState_f = cb->SyncState;
    AnalogEvent_f = cb->AnalogEvent;
    printf("Q3E_Engine_Interactive: callback=%p, interface=%p\n", callback, nullptr);
    return nullptr;
}



typedef struct Q3E_Client_Callback_s
{
	bool (*AnalogEvent)(float *x, float *y);
} Q3E_Client_Callback_t;

static bool Q3E_Client_AnalogEvent(float *x, float *y)
{
    if(analogEnabled)
    {
        if(x) *x = analogX;
        if(y) *y = analogY;
    }
    return analogEnabled;
}

extern void * Q3E_GetProcAddress( HMODULE hModule, const char *pName );
extern CSysModule *g_ClientDLLModule;
void Q3E_Engine_SetupClientInteraction()
{
	if(!g_ClientDLLModule)
	    return;

    using Q3E_Client_Interaction_f = void * (*)(const void *callback);
    Q3E_Client_Interaction_f func = (Q3E_Client_Interaction_f)Q3E_GetProcAddress((HMODULE)g_ClientDLLModule, "Q3E_Client_Interaction");
    printf("Found client interaction function: %p at %p\n", func, g_ClientDLLModule);
    if(func)
    {
        Q3E_Client_Callback_t cb = {
            &Q3E_Client_AnalogEvent
        };
        func(&cb);
    }
}

const char * Q3E_Engine_GetGameDLLSuffix()
{
    const char *gamedll = CommandLine()->ParmValue("-sv_cl", CommandLine()->ParmValue("-game", "hl2"));
	if(true
#ifdef _SV_CL_HL2
			&& Q_stricmp(gamedll, "hl2")
#endif
#ifdef _SV_CL_CSTRIKE
	        && Q_stricmp(gamedll, "cstrike")
#endif
#ifdef _SV_CL_PORTAL
			&& Q_stricmp(gamedll, "portal")
#endif
#ifdef _SV_CL_DOD
			&& Q_stricmp(gamedll, "dod")
#endif
#ifdef _SV_CL_EPISODIC
			&& Q_stricmp(gamedll, "episodic")
#endif
#ifdef _SV_CL_HL2MP
			&& Q_stricmp(gamedll, "hl2mp")
#endif
#ifdef _SV_CL_HL1
			&& Q_stricmp(gamedll, "hl1")
#endif
#ifdef _SV_CL_HL1MP
			&& Q_stricmp(gamedll, "hl1mp")
#endif
			)
		gamedll = "hl2";
	return gamedll;
}

const char * Q3E_Engine_GetClientDLL(char *client_dll, int length)
{
    const char *customClientDll = CommandLine()->ParmValue("-clientdll", "");
	if(customClientDll && customClientDll[0])
		Q_snprintf(client_dll, length, "%s", customClientDll);
	else
	{
		const char *gamedll = Q3E_Engine_GetGameDLLSuffix();
		Q_snprintf(client_dll, length, "client_%s", gamedll);
	}
	printf("Client DLL: %s\n", client_dll);
	return client_dll;
}

const char * Q3E_Engine_GetServerDLL(char *server_dll, int length)
{
    const char *customServerDll = CommandLine()->ParmValue("-serverdll", "");
	if(customServerDll && customServerDll[0])
		Q_snprintf(server_dll, length, "%s", customServerDll);
	else
	{
		const char *gamedll = Q3E_Engine_GetGameDLLSuffix();
		Q_snprintf(server_dll, length, "server_%s", gamedll);
	}
	printf("Server DLL: %s\n", server_dll);
	return server_dll;
}
