/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifdef PLATFORM_UNIX  /* rcg10072001 */
#include <signal.h>
#endif

#include "StdAfx.h"
#include <GameMP/Game.h>
#define DECL_DLL

// application state variables
BOOL _bRunning = TRUE;
static BOOL _bForceRestart = FALSE;
static BOOL _bForceNextMap = FALSE;

CTString _strSamVersion = "no version information";
INDEX ded_iMaxFPS = 100;
CTString ded_strConfig = "";
CTString ded_strLevel = "";
INDEX ded_bRestartWhenEmpty = TRUE;
FLOAT ded_tmTimeout = -1;
CGame *_pGame = NULL;
#ifdef FIRST_ENCOUNTER
CTString sam_strFirstLevel = "Levels\\KarnakDemo.wld";
CTString sam_strIntroLevel = "Levels\\Intro.wld";
CTString sam_strGameName = "serioussam";
#else
CTString sam_strFirstLevel = "Levels\\LevelsMP\\1_0_InTheLastEpisode.wld";
CTString sam_strIntroLevel = "Levels\\LevelsMP\\Intro.wld";
CTString sam_strGameName = "serioussamse";
#endif

CTimerValue _tvLastLevelEnd((__int64) -1);

// Not used; dummy declaration only needed by
// Engine/Base/ErrorReporting.o
HWND _hwndMain = NULL;

void InitializeGame(void)
{
#ifdef PLATFORM_UNIX
  #ifdef STATICALLY_LINKED
    #define fnmExpanded NULL
    CPrintF(TRANSV("Loading game library '%s'...\n"), "(statically linked)");
  #else
    CTFileName fnmDLL;
    #ifndef NDEBUG
      fnmDLL = "Bin\\Debug\\Game"+_strModExt+"D.dll";
    #else
      fnmDLL = "Bin\\Game"+_strModExt+".dll";
    #endif

    fnmDLL = CDynamicLoader::ConvertLibNameToPlatform(fnmDLL);
    CTFileName fnmExpanded;
    ExpandFilePath(EFP_READ | EFP_NOZIPS,fnmDLL,fnmExpanded);
    CPrintF(TRANSV("Loading game library '%s'...\n"), (const char *)fnmExpanded);
  #endif

  CDynamicLoader *loader = CDynamicLoader::GetInstance(fnmExpanded);
  CGame *(*GAME_Create)(void) = NULL;

  if (loader->GetError() == NULL) {
    GAME_Create = (CGame* (*)(void)) loader->FindSymbol("GAME_Create");
  }

  if (GAME_Create == NULL) {
    FatalError("%s", loader->GetError());
  } else {
    _pGame = GAME_Create();
    // init game - this will load persistent symbols
    _pGame->Initialize(CTString("Data\\DedicatedServer.gms"));
  }
#else
	try {
  #ifndef NDEBUG 
  #define GAMEDLL _fnmApplicationExe.FileDir()+"Game"+_strModExt+"D.dll"
  #else
  #define GAMEDLL _fnmApplicationExe.FileDir()+"Game"+_strModExt+".dll"
  #endif
		CTFileName fnmExpanded;
		ExpandFilePath(EFP_READ, CTString(GAMEDLL), fnmExpanded);

		CPrintF(TRANS("Loading game library '%s'...\n"), (const char *)fnmExpanded);
		HMODULE hGame = LoadLibraryA(fnmExpanded);
		if (hGame == NULL) {
			ThrowF_t("%s", GetWindowsError(GetLastError()));
		}
		CGame* (*GAME_Create)(void) = (CGame* (*)(void))GetProcAddress(hGame, "GAME_Create");
		if (GAME_Create == NULL) {
			ThrowF_t("%s", GetWindowsError(GetLastError()));
		}
		_pGame = GAME_Create();

	}
	catch (char *strError) {
		FatalError("%s", strError);
	}
	// init game - this will load persistent symbols
	_pGame->Initialize(CTString("Data\\DedicatedServer.gms"));
#endif
}

static void QuitGame(void)
{
  _bRunning = FALSE;
}

static void RestartGame(void)
{
  _bForceRestart = TRUE;
}
static void NextMap(void)
{
  _bForceNextMap = TRUE;
}


void End(void);

// limit current frame rate if neeeded

void LimitFrameRate(void)
{
  // measure passed time for each loop
  static CTimerValue tvLast(-1.0f);
  CTimerValue tvNow   = _pTimer->GetHighPrecisionTimer();
  TIME tmCurrentDelta = (tvNow-tvLast).GetSeconds();

  // limit maximum frame rate
  ded_iMaxFPS = ClampDn( ded_iMaxFPS, (INDEX)1);
  TIME tmWantedDelta  = 1.0f / ded_iMaxFPS;
#ifdef PLATFORM_UNIX
  if( tmCurrentDelta<tmWantedDelta)
    _pTimer->Sleep( (DWORD) ((tmWantedDelta-tmCurrentDelta)*1000.0f) );
#else
  if (tmCurrentDelta<tmWantedDelta) Sleep((tmWantedDelta - tmCurrentDelta)*1000.0f);
#endif
  // remember new time
  tvLast = _pTimer->GetHighPrecisionTimer();
}


/* rcg10072001 win32ism. */
#ifdef PLATFORM_WIN32
// break/close handler
BOOL WINAPI HandlerRoutine(
  DWORD dwCtrlType   //  control signal type
)
{
  if (dwCtrlType == CTRL_C_EVENT
  || dwCtrlType == CTRL_BREAK_EVENT
  || dwCtrlType == CTRL_CLOSE_EVENT
  || dwCtrlType == CTRL_LOGOFF_EVENT
  || dwCtrlType == CTRL_SHUTDOWN_EVENT) {
    _bRunning = FALSE;
  }
  return TRUE;
}
#endif

#ifdef PLATFORM_UNIX
void unix_signal_catcher(int signum)
{
    _bRunning = FALSE;
}
#endif


#define REFRESHTIME (0.1f)

static void LoadingHook_t(CProgressHookInfo *pphi)
{
  // measure time since last call
  static CTimerValue tvLast((__int64) 0);
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

  if (!_bRunning) {
    ThrowF_t(TRANS("User break!"));
  }
  // if not first or final update, and not enough time passed
  if (pphi->phi_fCompleted!=0 && pphi->phi_fCompleted!=1 &&
     (tvNow-tvLast).GetSeconds() < REFRESHTIME) {
    // do nothing
    return;
  }
  tvLast = tvNow;

  // print status text
  CTString strRes;
#ifdef PLATFORM_WIN32
  printf("\r                                                                      ");
  printf("\r%s : %3.0f%%\r", (const char *) pphi->phi_strDescription, pphi->phi_fCompleted*100);
#else
    // !!! FIXME: This isn't right, either...
  printf("%s : %3.0f%%\n", (const char *) pphi->phi_strDescription, pphi->phi_fCompleted*100);
#endif
}

// loading hook functions
void EnableLoadingHook(void)
{
  printf("\n");
  SetProgressHook(LoadingHook_t);
}

void DisableLoadingHook(void)
{
  SetProgressHook(NULL);
  printf("\n");
}

BOOL StartGame(CTString &strLevel)
{
  _pGame->gm_aiStartLocalPlayers[0] = -1;
  _pGame->gm_aiStartLocalPlayers[1] = -1;
  _pGame->gm_aiStartLocalPlayers[2] = -1;
  _pGame->gm_aiStartLocalPlayers[3] = -1;

  _pGame->gam_strCustomLevel = strLevel;

  _pGame->gm_strNetworkProvider = "TCP/IP Server";
  CUniversalSessionProperties sp;
  _pGame->SetMultiPlayerSession(sp);
  return _pGame->NewGame( _pGame->gam_strSessionName, strLevel, sp);
}
 
void ExecScript(const CTFileName &fnmScript)
{
  CPrintF("Executing: '%s'\n", (const char *) fnmScript);
  CTString strCmd;
  strCmd.PrintF("include \"%s\"", (const char *) fnmScript);
  _pShell->Execute(strCmd);
}


#ifdef PLATFORM_WIN32
    #define DelayBeforeExit() fgetc(stdin);
#else
    #define DelayBeforeExit()
    static void atexit_sdlquit(void) { static bool firsttime = true; if (firsttime) { firsttime = false; SDL_Quit(); } }
#endif

BOOL Init(int argc, char* argv[])
{
  _bDedicatedServer = TRUE;

  if (argc!=1+1 && argc!=2+1) {
    // NOTE: this cannot be translated - translations are not loaded yet
    printf("Usage: DedicatedServer <configname> [<modname>]\n"
      "This starts a server reading configs from directory 'Scripts\\Dedicated\\<configname>\\'\n");

    DelayBeforeExit();
    exit(0);
  }

  #ifdef PLATFORM_WIN32
    SetConsoleTitleA(argv[1]);
  #else
    if (SDL_Init(0) == -1) {  // just get the basics up and running, like timers. No video, audio, input.
      fprintf(stderr, "SDL_Init(0) failed! Aborting.\n");
      _exit(1);
    }
    atexit(atexit_sdlquit);
  #endif

  ded_strConfig = CTString("Scripts\\Dedicated\\")+argv[1]+"\\";

  if (argc==2+1) {
    _fnmMod = CTString("Mods\\")+argv[2]+"\\";
  }


  _strLogFile = CTString("Dedicated_")+argv[1];

  // initialize engine
#ifdef PLATFORM_UNIX
  SE_InitEngine(argv[0], sam_strGameName);
#else
  SE_InitEngine(sam_strGameName);
#endif

//  ParseCommandLine(strCmdLine);

  // load all translation tables
  InitTranslation();
  CTFileName fnmTransTable;
  try {
    fnmTransTable = CTFILENAME("Data\\Translations\\Engine.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\Game.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\Entities.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\SeriousSam.txt");
    AddTranslationTable_t(fnmTransTable);
    fnmTransTable = CTFILENAME("Data\\Translations\\Levels.txt");
    AddTranslationTable_t(fnmTransTable);

    FinishTranslationTable();
  } catch (const char *strError) {
    CTString str(fnmTransTable);
    FatalError("%s %s", (const char *) str, strError);
  }

  // always disable all warnings when in serious sam
  _pShell->Execute( "con_bNoWarnings=1;");

  // declare shell symbols
  _pShell->DeclareSymbol("persistent user INDEX ded_iMaxFPS;", (void *) &ded_iMaxFPS);
  _pShell->DeclareSymbol("user void Quit(void);", (void *) &QuitGame);
  _pShell->DeclareSymbol("user CTString ded_strLevel;", (void *) &ded_strLevel);
  _pShell->DeclareSymbol("user FLOAT ded_tmTimeout;", (void *) &ded_tmTimeout);
  _pShell->DeclareSymbol("user INDEX ded_bRestartWhenEmpty;", (void *) &ded_bRestartWhenEmpty);
  _pShell->DeclareSymbol("user void Restart(void);", (void *) &RestartGame);
  _pShell->DeclareSymbol("user void NextMap(void);", (void *) &NextMap);
  _pShell->DeclareSymbol("persistent user CTString sam_strIntroLevel;",      (void *) &sam_strIntroLevel);
  _pShell->DeclareSymbol("persistent user CTString sam_strGameName;",      (void *) &sam_strGameName);
  _pShell->DeclareSymbol("user CTString sam_strFirstLevel;", (void *) &sam_strFirstLevel);

  // init game - this will load persistent symbols
  InitializeGame();
  _pNetwork->md_strGameID = sam_strGameName;

  LoadStringVar(CTString("Data\\Var\\Sam_Version.var"), _strSamVersion);
  CPrintF(TRANSV("Serious Sam version: %s\n"), (const char *) _strSamVersion);

  #if (defined PLATFORM_WIN32)
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
  #elif (defined PLATFORM_UNIX)
    signal(SIGINT, unix_signal_catcher);
    signal(SIGHUP, unix_signal_catcher);
    signal(SIGQUIT, unix_signal_catcher);
    signal(SIGTERM, unix_signal_catcher);
  #endif

  // if there is a mod
  if (_fnmMod!="") {
    // execute the mod startup script
    _pShell->Execute(CTString("include \"Scripts\\Mod_startup.ini\";"));
  }

  return TRUE;
}
void End(void)
{

  // cleanup level-info subsystem
//  ClearDemosList();

  // end game
  _pGame->End();

  // end engine
  SE_EndEngine();
}

static INDEX iRound = 1;
static BOOL _bHadPlayers = 0;
static BOOL _bRestart = 0;
CTString strBegScript;
CTString strEndScript;

void RoundBegin(void)
{
  // repeat generate script names
  FOREVER {
    strBegScript.PrintF("%s%d_begin.ini", (const char *) ded_strConfig, iRound);
    strEndScript.PrintF("%s%d_end.ini",   (const char *) ded_strConfig, iRound);
    // if start script exists
    if (FileExists(strBegScript)) {
      // stop searching
      break;

    // if start script doesn't exist
    } else {
      // if this is first round
      if (iRound==1) {
        // error
        CPrintF(TRANSV("No scripts present!\n"));
        _bRunning = FALSE;
        return;
      }
      // try again with first round
      iRound = 1;
    }
  }
  
  // run start script
  ExecScript(strBegScript);

  // start the level specified there
  if (ded_strLevel=="") {
    CPrintF(TRANSV("ERROR: No next level specified!\n"));
    _bRunning = FALSE;
  } else {
    EnableLoadingHook();
    StartGame(ded_strLevel);
    _bHadPlayers = 0;
    _bRestart = 0;
    DisableLoadingHook();
    _tvLastLevelEnd = CTimerValue((__int64) -1);
    CPrintF(TRANSV("\nALL OK: Dedicated server is now running!\n"));
    CPrintF(TRANSV("Use Ctrl+C to shutdown the server.\n"));
    CPrintF(TRANSV("DO NOT use the 'Close' button, it might leave the port hanging!\n\n"));
  }
}

void ForceNextMap(void)
{
  EnableLoadingHook();
  StartGame(ded_strLevel);
  _bHadPlayers = 0;
  _bRestart = 0;
  DisableLoadingHook();
  _tvLastLevelEnd = CTimerValue((__int64) -1);
}

void RoundEnd(void)
{
  CPrintF("end of round---------------------------\n");

  ExecScript(strEndScript);
  iRound++;
}

// do the main game loop and render screen
void DoGame(void)
{
  #ifdef SINGLE_THREADED
    _pTimer->HandleTimerHandlers();
  #endif

  // do the main game loop
  if( _pGame->gm_bGameOn) {
    _pGame->GameMainLoop();

    // if any player is connected
    if (_pGame->GetPlayersCount()) {
      if (!_bHadPlayers) {
        // unpause server
        if (_pNetwork->IsPaused()) {
          _pNetwork->TogglePause();
        }
      }
      // remember that
      _bHadPlayers = 1;
    // if no player is connected, 
    } else {
      // if was before
      if (_bHadPlayers) {
        // make it restart
        _bRestart = TRUE;
      // if never had any player yet
      } else {
        // keep the server paused
        if (!_pNetwork->IsPaused()) {
          _pNetwork->TogglePause();
        }
      }
    }

  // if game is not started
  } else {
    // just handle broadcast messages
    _pNetwork->GameInactive();
  }

  // limit current frame rate if needed
  LimitFrameRate();
}

int SubMain(int argc, char* argv[])
{

  // initialize
  if( !Init(argc, argv)) {
    End();
    return -1;
  }

  // initialy, application is running
  _bRunning = TRUE;

  // execute dedicated server startup script
  ExecScript(CTFILENAME("Scripts\\Dedicated_startup.ini"));
  // execute startup script for this config
  ExecScript(ded_strConfig+"init.ini");
  // start first round
  RoundBegin();

  // while it is still running
  while( _bRunning)
  {
    // do the main game loop
    DoGame();

    // if game is finished
    if (_pNetwork->IsGameFinished()) {
      // if not yet remembered end of level
      if (_tvLastLevelEnd.tv_llValue<0) {
        // remember end of level
        _tvLastLevelEnd = _pTimer->GetHighPrecisionTimer();
        // finish this round
        RoundEnd();
      // if already remembered
      } else {
        // if time is out
        if ((_pTimer->GetHighPrecisionTimer()-_tvLastLevelEnd).GetSeconds()>ded_tmTimeout) {
          // start next round
          RoundBegin();
        }
      }
    }

    if (_bRestart||_bForceRestart) {
      if (ded_bRestartWhenEmpty||_bForceRestart) {
        _bForceRestart = FALSE;
        _bRestart = FALSE;
        RoundEnd();
        CPrintF(TRANSV("\nNOTE: Restarting server!\n\n"));
        RoundBegin();
      } else {
        _bRestart = FALSE;
        _bHadPlayers = FALSE;
      }
    }
    if (_bForceNextMap) {
      ForceNextMap();
      _bForceNextMap = FALSE;
    }

  } // end of main application loop

  _pGame->StopGame();

  End();

  return 0;
}


int main(int argc, char* argv[])
{
  int iResult;
  CTSTREAM_BEGIN {
    iResult = SubMain(argc, argv);
  } CTSTREAM_END;

  return iResult;
}

