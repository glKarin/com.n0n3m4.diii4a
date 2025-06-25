
#include <sys/stat.h>
#include <sys/types.h>
#include "client.h"

#include "q3e/q3e_android.h"

void ShutdownGame();

#define Q3E_GAME_NAME "Xash3D"
#define Q3E_IS_INITIALIZED (com_fullyInitialized)
#define Q3E_PRINTF printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool int
#define Q3E_TRUE true
#define Q3E_FALSE false
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

Q3Ebool com_fullyInitialized = false;

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

void ShutdownGame() {
	if(q3e_running/* && com_fullyInitialized*/)
	{
		q3e_running = false;
		NOTIFY_EXIT;
	}
}

void Sys_SyncState(void)
{
	//if (setState)
	{
		static int prev_state = -1;

		int state = STATE_GAME;
		switch( GameState->curstate )
		{
			case STATE_LOAD_LEVEL:
				state = STATE_LOADING;
				break;
			case STATE_LOAD_GAME:
				state = STATE_LOADING;
				break;
			case STATE_CHANGELEVEL:
				state = STATE_LOADING;
				break;
			case STATE_RUNFRAME:
				if(cls.key_dest == key_menu)
					state = STATE_MENU;
				else if(cls.key_dest == key_console)
					state = STATE_CONSOLE;
				else
					state = STATE_GAME;
				break;
			case STATE_GAME_SHUTDOWN:
				state = STATE_MENU;
				break;
		}

		if (state != prev_state)
		{
			setState(state);
			prev_state = state;
		}
	}
}

static void Q3E_CreatePath( char *path )
{
	Q3E_PRINTF("Create path: %s\n", path);
	char	*ofs, save;

	int i = 1;
	for( ofs = path + 1; *ofs; ofs++ )
	{
		if( *ofs == '/' || *ofs == '\\' )
		{
			// create the directory
			save = *ofs;
			*ofs = 0;
			_mkdir( path );
			*ofs = save;
			i = ofs - path;
		}
	}
	if(i < strlen(path) - 1)
		_mkdir( path );
}

static int Q3E_CheckParm( int argc, char **argv, const char *parm, int start )
{
	int	i;

	for( i = start; i < argc; i++ )
	{
		if( !argv[i] )
			continue;

		if( !Q_stricmp( parm, argv[i] ))
			return i;
	}
	return 0;
}

static int Q3E_GetParmFromCmdLine( int argc, char **argv, const char *parm, char *out, size_t size, int start )
{
	int	rargc = Q3E_CheckParm( argc, argv, parm, start );

	if( !rargc || !out || (rargc + 1) >= argc || !argv[rargc + 1] || !argv[rargc + 1][0] )
		return 0;

	Q_strncpy( out, argv[rargc+1], size );

	return rargc+1;
}

void Q3E_PostMain(int argc, char **argv)
{
	// add env vars
	setenv("XASH3D_GAMELIBDIR", Sys_DLLInternalPath(), 1);
	setenv("XASH3D_ENGLIBDIR", Sys_DLLDefaultPath(), 1);

	char libdir[1024];
	if(Q3E_GetParmFromCmdLine(argc, argv, "-gamelibdir", libdir, sizeof(libdir), 1))
		setenv("XASH3D_GAMELIBDIR", libdir, 1);
	if(Q3E_GetParmFromCmdLine(argc, argv, "-englibdir", libdir, sizeof(libdir), 1))
		setenv("XASH3D_ENGLIBDIR", libdir, 1);

	Q3E_PRINTF("XASH3D_GAMELIBDIR=%s\n", getenv("XASH3D_GAMELIBDIR"));
	Q3E_PRINTF("XASH3D_ENGLIBDIR=%s\n", getenv("XASH3D_ENGLIBDIR"));

	// create write game base directory
	//char path[MAX_OSPATH] = { 0 };
	//Q_snprintf(path, sizeof(path), "%s/%s", Sys_GameDataDefaultPath(), ".xash3d");
	//Q3E_CreatePath( path );
}

void IN_Analog(float *forward, float *side)
{
    if (analogenabled)
    {
        *side += analogx;
        if (analogy > 0)
            *forward += analogy;
        else if (analogy < 0)
            *forward += analogy;
    }
}
