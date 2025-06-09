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

#ifdef _DIII4A //karin: main()
#include "SeriousSam_diii4a.cpp"
#else
#include "SeriousSam/StdH.h"

#ifdef PLATFORM_WIN32
#include <io.h>
#include <process.h>
#endif

// !!! FIXME: rcg01082002 Do something with these.
#ifdef PLATFORM_UNIX
  #include <Engine/Base/SDL/SDLEvents.h>
  #if !defined(PLATFORM_MACOSX) && !defined(PLATFORM_FREEBSD)
    #include <mntent.h>
  #endif
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <Engine/CurrentVersion.h>
#include <GameMP/Game.h>
#define DECL_DLL
#ifdef FIRST_ENCOUNTER
#include <Entities/Global.h>
#else
#include <EntitiesMP/Global.h>
#endif
#include "resource.h"
#include "SplashScreen.h"
#include "MainWindow.h"
#include "GLSettings.h"
#include "SeriousSam/LevelInfo.h"
#include "LCDDrawing.h"
#include "CmdLine.h"
#include "Credits.h"


CGame *_pGame = NULL;

extern FLOAT _fMenuPlayerProfileAdjuster;

extern FLOAT _fGlobalTopAdjuster;
extern FLOAT _fGlobalListAdjuster;
extern FLOAT _fGlobalTipAdjuster;
extern FLOAT _fGlobalInfoAdjuster;
extern FLOAT _fGlobalProfileAdjuster;
extern FLOAT _fGlobalOptionsAdjuster;
extern FLOAT _fGlobalModAdjuster;
extern FLOAT _fGlobalButtonAdjuster;
extern FLOAT _fGlobalProfileFOVAdjuster;
#if defined(PLATFORM_UNIX) && !defined(ANDROID) //karin: Engine build static on Android
ENGINE_API FLOAT _fWeaponFOVAdjuster;
ENGINE_API FLOAT _fPlayerFOVAdjuster;
ENGINE_API FLOAT _fArmorHeightAdjuster;
ENGINE_API FLOAT _fFragScorerHeightAdjuster;
#else
extern ENGINE_API FLOAT _fPlayerFOVAdjuster;
extern ENGINE_API FLOAT _fWeaponFOVAdjuster;
extern ENGINE_API FLOAT _fArmorHeightAdjuster;
extern ENGINE_API FLOAT _fFragScorerHeightAdjuster;
#endif

extern FLOAT _fBigStartJ;       //Position of contents below large font title
extern FLOAT _fBigSizeJ;
extern FLOAT _fMediumSizeJ;
extern FLOAT _fNoStartI;
extern FLOAT _fNoSizeI;
extern FLOAT _fNoSpaceI;
extern FLOAT _fNoUpStartJ;      //Postiion of contents without large font title
extern FLOAT _fNoDownStartJ;
extern FLOAT _fNoSizeJ;

// application state variables
__extern BOOL _bRunning = TRUE;
__extern BOOL _bQuitScreen = TRUE;
__extern BOOL bMenuActive = FALSE;
__extern BOOL bMenuRendering = FALSE;

extern BOOL _bDefiningKey;
static BOOL _bReconsiderInput = FALSE;
__extern PIX  _pixDesktopWidth = 0;    // desktop width when started (for some tests)

static INDEX sam_iMaxFPSActive   = 500;
static INDEX sam_iMaxFPSInactive = 10;
static INDEX sam_bPauseOnMinimize = TRUE; // auto-pause when window has been minimized
__extern INDEX sam_bWideScreen = FALSE;
__extern FLOAT sam_fPlayerOffset = 0.0f;

// display mode settings
__extern INDEX sam_bFullScreenActive = FALSE;
__extern INDEX sam_bBorderLessActive = FALSE;
__extern INDEX sam_iScreenSizeI = 1024;  // current size of the window
__extern INDEX sam_iScreenSizeJ = 768;  // current size of the window
__extern INDEX sam_iAspectSizeI = 16;  //
__extern INDEX sam_iAspectSizeJ = 9;  //
__extern INDEX sam_iDisplayDepth  = 0;  // 0==default, 1==16bit, 2==32bit
__extern INDEX sam_iDisplayAdapter = 0;
__extern INDEX sam_iGfxAPI = 0;                                // 0==OpenGL
__extern INDEX sam_bFirstStarted = FALSE;
__extern FLOAT sam_tmDisplayModeReport = 5.0f;
__extern INDEX sam_bShowAllLevels = FALSE;
__extern INDEX sam_bMentalActivated = FALSE;

// network settings
__extern CTString sam_strNetworkSettings = "";
// command line
__extern CTString sam_strCommandLine = "";

// 0...app started for the first time
// 1...all ok
// 2...automatic fallback
static INDEX _iDisplayModeChangeFlag = 0;
static TIME _tmDisplayModeChanged = 100.0f; // when display mode was last changed

// rendering preferences for automatic settings
__extern INDEX sam_iVideoSetup = 1;  // 0==speed, 1==normal, 2==quality, 3==custom
// automatic adjustment of audio quality
__extern BOOL sam_bAutoAdjustAudio = TRUE;

__extern INDEX sam_bAutoPlayDemos = TRUE;
static INDEX _bInAutoPlayLoop = TRUE;

// menu calling
__extern INDEX sam_bMenuSave     = FALSE;
__extern INDEX sam_bMenuLoad     = FALSE;
__extern INDEX sam_bMenuControls = FALSE;
__extern INDEX sam_bMenuHiScore  = FALSE;
__extern INDEX sam_bToggleConsole = FALSE;
__extern INDEX sam_iStartCredits = FALSE;

// for mod re-loading
__extern CTFileName _fnmModToLoad = CTString("");
__extern CTString _strModServerJoin = CTString("");
__extern CTString _strURLToVisit = CTString("");
static char _strExePath[MAX_PATH] = "";
ENGINE_API extern INDEX sys_iSysPath;

// state variables fo addon execution
// 0 - nothing
// 1 - start (invoke console)
// 2 - console invoked, waiting for one redraw
__extern INDEX _iAddonExecState = 0;
__extern CTFileName _fnmAddonToExec = CTString("");

// logo textures
static CTextureObject  _toLogoCT;
static CTextureObject  _toLogoODI;
static CTextureObject  _toLogoEAX;
__extern CTextureObject *_ptoLogoCT  = NULL;
__extern CTextureObject *_ptoLogoODI = NULL;
__extern CTextureObject *_ptoLogoEAX = NULL;

#ifdef FIRST_ENCOUNTER  // First Encounter
CTString sam_strVersion = "1.10";
CTString sam_strModName = TRANS("-   T H E  F I R S T  E N C O U N T E R   -");
#if _SE_DEMO
  CTString sam_strFirstLevel = "Levels\\KarnakDemo.wld";
#else
  CTString sam_strFirstLevel = "Levels\\01_Hatshepsut.wld";
#endif
CTString sam_strIntroLevel = "Levels\\Intro.wld";
CTString sam_strGameName = "serioussam";

CTString sam_strTechTestLevel = "Levels\\TechTest.wld";
CTString sam_strTrainingLevel = "Levels\\KarnakDemo.wld";
#else    // Second Encounter
CTString sam_strVersion = "1.10";
CTString sam_strModName = TRANS("-   T H E  S E C O N D  E N C O U N T E R   -");

CTString sam_strFirstLevel = "Levels\\LevelsMP\\1_0_InTheLastEpisode.wld";
CTString sam_strIntroLevel = "Levels\\LevelsMP\\Intro.wld";
CTString sam_strGameName = "serioussamse";

CTString sam_strTechTestLevel = "Levels\\LevelsMP\\Technology\\TechTest.wld";
CTString sam_strTrainingLevel = "Levels\\KarnakDemo.wld";
#endif

ENGINE_API extern INDEX snd_iFormat;

// main window canvas
CDrawPort *pdp;
CDrawPort *pdpNormal;
CDrawPort *pdpWideScreen;
CViewPort *pvpViewPort;
HINSTANCE _hInstance;


static void PlayDemo(void* pArgs)
{
  CTString strDemoFilename = *NEXTARGUMENT(CTString*);
  _gmMenuGameMode = GM_DEMO;
  CTFileName fnDemo = "demos\\" + strDemoFilename + ".dem";
  extern BOOL LSLoadDemo(const CTFileName &fnm);
  LSLoadDemo(fnDemo);
}

static void ApplyRenderingPreferences(void)
{
  ApplyGLSettings(TRUE);
}

extern void ApplyVideoMode(void)
{
  StartNewMode( (GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ,  sam_iAspectSizeI, sam_iAspectSizeJ,  (enum DisplayDepth)sam_iDisplayDepth, sam_bFullScreenActive, sam_bBorderLessActive);
}

static void BenchMark(void)
{
  _pGfx->Benchmark(pvpViewPort, pdp);
}


static void QuitGame(void)
{
  _bRunning = FALSE;
  _bQuitScreen = FALSE;
}

// check if another app is already running
// !!! FIXME: rcg01042002 Actually, I've abstracted this code, but it didn't
// !!! FIXME: rcg01042002  really seem to care if there was another copy
// !!! FIXME: rcg01042002  running before anyhow. What SHOULD be done is
// !!! FIXME: rcg01042002  we should see if the lockfile exists, and if not
// !!! FIXME: rcg01042002  create it and write our process ID in it. Then, if
// !!! FIXME: rcg01042002  another copy of Serious Sam is run, it sees the
// !!! FIXME: rcg01042002  file exists, opens it for reading, gets the process
// !!! FIXME: rcg01042002  ID, and sees if that process is still running. If
// !!! FIXME: rcg01042002  so, the second copy of the game should abort.
// !!! FIXME: rcg01042002  If the process ID isn't running, recreate the file
// !!! FIXME: rcg01042002  and THEN give the warning about not shutting down
// !!! FIXME: rcg01042002  properly last time. At exit, delete the file.
// !!! FIXME: rcg01042002  This is all platform independent except for the
// !!! FIXME: rcg01042002  method of determining the current process ID and
// !!! FIXME: rcg01042002  determining if a given process ID is still running,
// !!! FIXME: rcg01042002  and those are easy abstractions.
#ifdef PLATFORM_UNIX
static CTFileName _fnmLock;
static FILE *_hLock = NULL;
static void DirectoryLockOn(void)
{
  // create lock filename
  _fnmLock = _fnmUserDir + "SeriousSam.loc";
  // try to open lock file

  if (_pFileSystem->Exists(_fnmLock))
    CPrintF(TRANSV("WARNING: SeriousSam didn't shut down properly last time!\n"));

  _hLock = fopen(_fnmLock, "w");
  if (_hLock == NULL) {
    FatalError(TRANS("Failed to create lockfile %s! (%s)"),
                     (const char *) _fnmLock, (const char *)strerror(errno));
  }
}

static void DirectoryLockOff(void)
{
  // if lock is open
  if (_hLock!=NULL) {
    fclose(_hLock);
    _hLock = NULL;
  }
  unlink(_fnmLock);
}
#else
// check if another app is already running
static HANDLE _hLock = NULL;
static CTFileName _fnmLock;
static void DirectoryLockOn(void)
{
	// create lock filename
	_fnmLock = _fnmApplicationPath + "SeriousSam.loc";
	// try to open lock file
	_hLock = CreateFileA(
		_fnmLock,
		GENERIC_WRITE,
		0/*no sharing*/,
		NULL, // pointer to security attributes
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,  // file attributes
		NULL);
	// if failed
	if (_hLock == NULL || GetLastError() != 0) {
		// report warning
		CPrintF(TRANS("WARNING: SeriousSam didn't shut down properly last time!\n"));
	}
}
static void DirectoryLockOff(void)
{
	// if lock is open
	if (_hLock != NULL) {
		// close it
		CloseHandle(_hLock);
	}
}
#endif
void End(void);

// automaticaly manage input enable/disable toggling
static BOOL _bInputEnabled = FALSE;
void UpdateInputEnabledState(void)
{
  // do nothing if window is invalid
  if( _hwndMain==NULL) return;

  // input should be enabled if application is active
  // and no menu is active and no console is active
  BOOL bShouldBeEnabled = (!IsIconic(_hwndMain) && !bMenuActive && _pGame->gm_csConsoleState==CS_OFF
                       && (_pGame->gm_csComputerState==CS_OFF || _pGame->gm_csComputerState==CS_ONINBACKGROUND))
                       || _bDefiningKey;

  // if should be turned off
  if( (!bShouldBeEnabled && _bInputEnabled) || _bReconsiderInput) {
    // disable it and remember new state
    _pInput->DisableInput();
    _bInputEnabled = FALSE;
  }
  // if should be turned on
  if( bShouldBeEnabled && !_bInputEnabled) {
    // enable it and remember new state
    _pInput->EnableInput(_hwndMain);
    _bInputEnabled = TRUE;
  }
  _bReconsiderInput = FALSE;
}


// automaticaly manage pause toggling
static void UpdatePauseState(void)
{
  BOOL bShouldPause = (_gmRunningGameMode==GM_SINGLE_PLAYER) && (bMenuActive || 
                       _pGame->gm_csConsoleState ==CS_ON || _pGame->gm_csConsoleState ==CS_TURNINGON || _pGame->gm_csConsoleState ==CS_TURNINGOFF ||
                       _pGame->gm_csComputerState==CS_ON || _pGame->gm_csComputerState==CS_TURNINGON || _pGame->gm_csComputerState==CS_TURNINGOFF);
  _pNetwork->SetLocalPause(bShouldPause);
}


// limit current frame rate if neeeded
void LimitFrameRate(void)
{
  // do not limit FPS on the Pandora, it's not powerfull enough and doesn't "iconise" games either
  #if !PLATFORM_NOT_X86
  // measure passed time for each loop
  static CTimerValue tvLast(-1.0f);
  CTimerValue tvNow   = _pTimer->GetHighPrecisionTimer();
  TIME tmCurrentDelta = (tvNow-tvLast).GetSeconds();

  // limit maximum frame rate
  sam_iMaxFPSActive   = ClampDn( (INDEX)sam_iMaxFPSActive, (INDEX)1);
  sam_iMaxFPSInactive = ClampDn( (INDEX)sam_iMaxFPSInactive, (INDEX)1);
  INDEX iMaxFPS = sam_iMaxFPSActive;
  if( IsIconic(_hwndMain)) iMaxFPS = sam_iMaxFPSInactive;
  if(_pGame->gm_CurrentSplitScreenCfg==CGame::SSC_DEDICATED) {
    iMaxFPS = ClampDn(iMaxFPS, (INDEX)60); // never go very slow if dedicated server
  }
  TIME tmWantedDelta = 1.0f / iMaxFPS;
#ifdef PLATFORM_UNIX
  if((tmCurrentDelta > 0) && (tmCurrentDelta<tmWantedDelta)) _pTimer->Sleep( (tmWantedDelta-tmCurrentDelta)*1000.0f);
#else
  if (tmCurrentDelta<tmWantedDelta) Sleep((tmWantedDelta - tmCurrentDelta)*1000.0f);
#endif
  // remember new time
  tvLast = _pTimer->GetHighPrecisionTimer();
  #endif
}

// load first demo
void StartNextDemo(void)
{
  if (!sam_bAutoPlayDemos || !_bInAutoPlayLoop) {
    _bInAutoPlayLoop = FALSE;
    return;
  }

  // skip if no demos
  if(_lhAutoDemos.IsEmpty()) {
    _bInAutoPlayLoop = FALSE;
    return;
  }

  // get first demo level and cycle the list
  CLevelInfo *pli = LIST_HEAD(_lhAutoDemos, CLevelInfo, li_lnNode);
  pli->li_lnNode.Remove();
  _lhAutoDemos.AddTail(pli->li_lnNode);

  // if intro
  if (pli->li_fnLevel==CTFileName(sam_strIntroLevel)) {
    // start intro
    _gmRunningGameMode = GM_NONE;
    _pGame->gm_aiStartLocalPlayers[0] = 0;
    _pGame->gm_aiStartLocalPlayers[1] = -1;
    _pGame->gm_aiStartLocalPlayers[2] = -1;
    _pGame->gm_aiStartLocalPlayers[3] = -1;
    _pGame->gm_strNetworkProvider = "Local";
    _pGame->gm_StartSplitScreenCfg = CGame::SSC_PLAY1;

    _pShell->SetINDEX("gam_iStartDifficulty", CSessionProperties::GD_NORMAL);
    _pShell->SetINDEX("gam_iStartMode", CSessionProperties::GM_FLYOVER);

    CUniversalSessionProperties sp;
    _pGame->SetSinglePlayerSession(sp);

    _pGame->gm_bFirstLoading = TRUE;

    if (_pGame->NewGame( sam_strIntroLevel, sam_strIntroLevel, sp)) {
      _gmRunningGameMode = GM_INTRO;
    }
  // if not intro
  } else {
    // start the demo
    _pGame->gm_StartSplitScreenCfg = CGame::SSC_OBSERVER;
    _pGame->gm_aiStartLocalPlayers[0] = -1;
    _pGame->gm_aiStartLocalPlayers[1] = -1;
    _pGame->gm_aiStartLocalPlayers[2] = -1;
    _pGame->gm_aiStartLocalPlayers[3] = -1;
    // play the demo
    _pGame->gm_strNetworkProvider = "Local";
    _gmRunningGameMode = GM_NONE;
    if( _pGame->StartDemoPlay( pli->li_fnLevel)) {
      _gmRunningGameMode = GM_DEMO;
      CON_DiscardLastLineTimes();
    }
  }

  if (_gmRunningGameMode==GM_NONE) {
    _bInAutoPlayLoop = FALSE;
  }
}

BOOL _bCDPathFound = FALSE;

BOOL FileExistsOnHD(const CTString &strFile)
{
  FILE *f = fopen(_fnmApplicationPath+strFile, "rb");
  if (f!=NULL) {
    fclose(f);
    return TRUE;
  } else {
    return FALSE;
  }
}

void TrimString(char *str)
{
  int i = strlen(str);
  if (str[i-1]=='\n' || str[i-1]=='\r') {
    str[i-1]=0;
  }
}

// run web browser and view an url
void RunBrowser(const char *strUrl)
{
#ifdef PLATFORM_WIN32
  int iResult = (int)ShellExecuteA( _hwndMain, "OPEN", strUrl, NULL, NULL, SW_SHOWMAXIMIZED);
  if (iResult<32) {
    // should report error?
    NOTHING;
  }

#else

    STUBBED("Should spawn browser here");

#endif
}

void LoadAndForceTexture(CTextureObject &to, CTextureObject *&pto, const CTFileName &fnm)
{
  try {
    to.SetData_t(fnm);
    CTextureData *ptd = (CTextureData*)to.GetData();
    ptd->Force( TEX_CONSTANT);
    ptd = ptd->td_ptdBaseTexture;
    if( ptd!=NULL) ptd->Force( TEX_CONSTANT);
    pto = &to;
  } catch (const char *pchrError) {
    (void*)pchrError;
    pto = NULL;
  }
}

static char *argv0 = NULL;

void InitializeGame(void)
{
  try {
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
  
    const char *err;
    CDynamicLoader *hGame = CDynamicLoader::GetInstance(fnmExpanded);
    if ((err = hGame->GetError()) != NULL) {
      ThrowF_t("%s", err);
    }
    CGame* (*GAME_Create)(void) = (CGame* (*)(void))hGame->FindSymbol("GAME_Create");
    if ((err = hGame->GetError()) != NULL) {
      ThrowF_t("%s", err);
    }
#else // WIN32
	#ifndef NDEBUG 
		#define GAMEDLL (_fnmApplicationExe.FileDir()+"Game"+_strModExt+"D.dll")
	#else
		#define GAMEDLL (_fnmApplicationExe.FileDir()+"Game"+_strModExt+".dll")
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
#endif
    _pGame = GAME_Create();
  } catch (char *strError) {
    FatalError("%s", strError);
  }
  // init game - this will load persistent symbols
  _pGame->Initialize(CTString("Data\\SeriousSam.gms"));
  // save executable path and sys var.
#ifdef PLATFORM_UNIX
  _pFileSystem->GetExecutablePath(_strExePath, sizeof (_strExePath)-1);
  _pFileSystem->GetExecutablePath(_strExePath, sizeof (_strExePath)-1);
#endif
}

#ifdef PLATFORM_UNIX
static void atexit_sdlquit(void) { static bool firsttime = true; if (firsttime) { firsttime = false; SDL_Quit(); } }
#endif

//#### SetAdjusters()
void SetAdjusters()
{
	if (pdp == NULL) return;
	float ratio = (float)pdp->GetWidth() / (float)pdp->GetHeight();
 	if (ratio >= 1.32f && ratio <= 1.34f) 		//4:3
 	{
		_fBigSizeJ					= 0.050f;
		_fMediumSizeJ 				= 0.03f;	//medium font row size
 		_fBigStartJ					= 0.40f;	//Position of the contents below large font title
		_fNoUpStartJ				= 0.25f;	//Postiion of contents without large font title
		_fNoDownStartJ				= 0.44f;
		_fGlobalTopAdjuster 		= 0.12f;
		_fGlobalListAdjuster 		= 0.55f;
		_fGlobalTipAdjuster 		= 0.90f;
		_fGlobalInfoAdjuster 		= 0.80f;
		_fGlobalProfileAdjuster 	= 0.75f;
		_fGlobalOptionsAdjuster 	= 0.45f;
		_fGlobalModAdjuster 		= 0.55f;
		_fGlobalButtonAdjuster  	= 1.20f;		//Menu items and buttons offset
		_fWeaponFOVAdjuster			= 1.0f;		//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.0f;		//Field of View for player
		_fGlobalProfileFOVAdjuster	= 33.0f;
		_fArmorHeightAdjuster		= 0.7f;
		_fFragScorerHeightAdjuster	= 0.75f;
        _fMenuPlayerProfileAdjuster = 0.38f;
	}else if (ratio >= 1.2f && ratio <= 1.26f) 		//5:4
 	{
		_fBigSizeJ					= 0.050f;
		_fMediumSizeJ 				= 0.03f;	//medium font row size
 		_fBigStartJ					= 0.40f;	//Position of the contents below large font title
		_fNoUpStartJ				= 0.25f;	//Postiion of contents without large font title
		_fNoDownStartJ				= 0.44f;
		_fGlobalTopAdjuster 		= 0.12f;
		_fGlobalListAdjuster 		= 0.55f;
		_fGlobalTipAdjuster 		= 0.90f;
		_fGlobalInfoAdjuster 		= 0.80f;
		_fGlobalProfileAdjuster 	= 0.75f;
		_fGlobalOptionsAdjuster 	= 0.45f;
		_fGlobalModAdjuster 		= 0.55f;
		_fGlobalButtonAdjuster  	= 1.20f;		//Menu items and buttons offset
		_fWeaponFOVAdjuster			= 1.0f;		//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.0f;		//Field of View for player
		_fGlobalProfileFOVAdjuster	= 33.0f;
		_fArmorHeightAdjuster		= 0.7f;
		_fFragScorerHeightAdjuster	= 0.75f;
        _fMenuPlayerProfileAdjuster = 0.38f;
 	}else if (ratio >= 1.76f && ratio <= 1.78f) 	//16:9
 	{
		_fBigSizeJ					= 0.060f;
		_fMediumSizeJ 				= 0.04f;	//medium font row size
 		_fBigStartJ					= 0.40f;	//Position of the contents below large font title
		_fNoUpStartJ				= 0.25f;	//Postiion of contents without large font title
		_fNoDownStartJ				= 0.44f;
		_fGlobalTopAdjuster 		= 0.15f;
		_fGlobalListAdjuster 		= 0.45f;
		_fGlobalTipAdjuster 		= 0.90f;
		_fGlobalInfoAdjuster 		= 0.80f;
		_fGlobalProfileAdjuster 	= 0.60f;
		_fGlobalOptionsAdjuster 	= 0.45f;
		_fGlobalModAdjuster 		= 0.65f;
		_fGlobalButtonAdjuster  	= 1.20f;
		_fWeaponFOVAdjuster			= 1.25f;	//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.15f;		//Field of View for player
		_fArmorHeightAdjuster		= 0.835f;
		_fFragScorerHeightAdjuster	= 1.5f;
		_fGlobalProfileFOVAdjuster	= 35.0f;
        _fMenuPlayerProfileAdjuster = 0.35f;
 	}else if (ratio >= 1.5 && ratio <= 1.70) 	//16:10
 	{
		_fBigSizeJ					= 0.050f;
		_fMediumSizeJ 				= 0.03f;	//medium font row size
 		_fBigStartJ					= 0.40f;	//Position of the contents below large font title
		_fNoUpStartJ				= 0.25f;	//Postiion of contents without large font title
		_fNoDownStartJ				= 0.44f;
		_fGlobalTopAdjuster 		= 0.15f;
		_fGlobalListAdjuster 		= 0.45f;
		_fGlobalTipAdjuster 		= 0.90f;
		_fGlobalInfoAdjuster 		= 0.80f;
		_fGlobalProfileAdjuster 	= 0.60f;
		_fGlobalOptionsAdjuster 	= 0.45f;
		_fGlobalModAdjuster 		= 0.55f;
		_fGlobalButtonAdjuster  	= 1.20f;
		_fWeaponFOVAdjuster			= 1.15f;	//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.10f;		//Field of View for player
		_fGlobalProfileFOVAdjuster	= 35.0f;
		_fArmorHeightAdjuster		= 0.78f;
		_fFragScorerHeightAdjuster	= 1.17f;
        _fMenuPlayerProfileAdjuster = 0.32f;
 	}else if (ratio >= 2.32 && ratio <= 2.34) 	//21:9
 	{
		_fBigSizeJ					= 0.076f;
		_fMediumSizeJ 				= 0.04f;	//medium font row size
 		_fBigStartJ					= 0.25f;	//Position of the contents below large font title
		_fNoUpStartJ				= 0.25f;	//Postiion of contents without large font title
		_fNoDownStartJ				= 0.25f;
		_fGlobalTopAdjuster 		= 0.19f;
		_fGlobalListAdjuster 		= 0.75f;
		_fGlobalTipAdjuster 		= 0.90f;
		_fGlobalInfoAdjuster 		= 0.90f;
		_fGlobalProfileAdjuster 	= 0.70f;
		_fGlobalOptionsAdjuster 	= 0.75f;
		_fGlobalModAdjuster 		= 1.05f;
		_fGlobalButtonAdjuster  	= 1.0f;
		_fWeaponFOVAdjuster			= 1.55f;	//Field of View for weapon
		_fPlayerFOVAdjuster			= 1.35f;		//Field of View for player
		_fGlobalProfileFOVAdjuster	= 45.0f;
		_fArmorHeightAdjuster	    = 1.0f;
		_fFragScorerHeightAdjuster	= 2.35f;
        _fMenuPlayerProfileAdjuster = 0.1f;
	}

  	ReInitializeMenus();
}
//##### SetAdjusters() end

BOOL Init( HINSTANCE hInstance, int nCmdShow, CTString strCmdLine)
{
#ifdef PLATFORM_UNIX
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1)
    FatalError("SDL_Init(VIDEO|AUDIO) failed. Reason: [%s].", SDL_GetError());
  atexit(atexit_sdlquit);
  SDL_Init(SDL_INIT_JOYSTICK);  // don't care if this fails.
#endif

#ifdef PLATFORM_PANDORA
  // enable Cortex A8 RunFast
  int v = 0;
  __asm__ __volatile__ (
    "vmrs %0, fpscr\n"
    "orr  %0, #((1<<25)|(1<<24))\n" // default NaN, flush-to-zero
    "vmsr fpscr, %0\n"
    //"vmrs %0, fpscr\n"
    : "=&r"(v));
#endif

  _hInstance = hInstance;
  ShowSplashScreen(hInstance);

  // remember desktop width
#ifdef PLATFORM_UNIX
  _pixDesktopWidth = DetermineDesktopWidth();
#else
  _pixDesktopWidth = ::GetSystemMetrics(SM_CXSCREEN);
#endif

  // prepare main window
  MainWindow_Init();
  OpenMainWindowInvisible();

  // parse command line before initializing engine
  ParseCommandLine(strCmdLine);

  // initialize engine
#ifdef PLATFORM_UNIX
  SE_InitEngine(argv0, sam_strGameName);
#else
  SE_InitEngine(sam_strGameName);
#endif
  SE_LoadDefaultFonts();
  // now print the output of command line parsing
  CPrintF("%s", (const char *) cmd_strOutput);

  // lock the directory
  DirectoryLockOn();

  // load all translation tables
  InitTranslation();
  try {
    AddTranslationTablesDir_t(CTString("Data\\Translations\\"), CTString("*.txt"));
    FinishTranslationTable();
  } catch (const char *strError) {
    FatalError("%s", strError);
  }

  // always disable all warnings when in serious sam
  _pShell->Execute( "con_bNoWarnings=1;");

  // declare shell symbols
  _pShell->DeclareSymbol("user void PlayDemo(CTString);", (void *) &PlayDemo);
  _pShell->DeclareSymbol("persistent INDEX sam_bFullScreen;",   (void *) &sam_bFullScreenActive);
  _pShell->DeclareSymbol("persistent INDEX sam_bBorderLess;",   (void *) &sam_bBorderLessActive);
  _pShell->DeclareSymbol("persistent INDEX sam_iScreenSizeI;",  (void *) &sam_iScreenSizeI);
  _pShell->DeclareSymbol("persistent INDEX sam_iScreenSizeJ;",  (void *) &sam_iScreenSizeJ);
  _pShell->DeclareSymbol("persistent INDEX sam_iAspectSizeI;",  (void *) &sam_iAspectSizeI);
  _pShell->DeclareSymbol("persistent INDEX sam_iAspectSizeJ;",  (void *) &sam_iAspectSizeJ);
  _pShell->DeclareSymbol("persistent INDEX sam_iDisplayDepth;", (void *) &sam_iDisplayDepth);
  _pShell->DeclareSymbol("persistent INDEX sam_iDisplayAdapter;", (void *) &sam_iDisplayAdapter);
  _pShell->DeclareSymbol("persistent INDEX sam_iGfxAPI;",         (void *) &sam_iGfxAPI);
  _pShell->DeclareSymbol("persistent INDEX sam_bFirstStarted;", (void *) &sam_bFirstStarted);
  _pShell->DeclareSymbol("persistent INDEX sam_bAutoAdjustAudio;", (void *) &sam_bAutoAdjustAudio);
  _pShell->DeclareSymbol("persistent user INDEX sam_bWideScreen;", (void *) &sam_bWideScreen);
  _pShell->DeclareSymbol("persistent user FLOAT sam_fPlayerOffset;", (void *) &sam_fPlayerOffset);
  _pShell->DeclareSymbol("persistent user INDEX sam_bAutoPlayDemos;", (void *) &sam_bAutoPlayDemos);
  _pShell->DeclareSymbol("persistent user INDEX sam_iMaxFPSActive;",   (void *) &sam_iMaxFPSActive);
  _pShell->DeclareSymbol("persistent user INDEX sam_iMaxFPSInactive;", (void *) &sam_iMaxFPSInactive);
  _pShell->DeclareSymbol("persistent user INDEX sam_bPauseOnMinimize;", (void *) &sam_bPauseOnMinimize);
  _pShell->DeclareSymbol("persistent user FLOAT sam_tmDisplayModeReport;",   (void *) &sam_tmDisplayModeReport);
  _pShell->DeclareSymbol("persistent user CTString sam_strNetworkSettings;", (void *) &sam_strNetworkSettings);
  _pShell->DeclareSymbol("persistent user CTString sam_strIntroLevel;",      (void *) &sam_strIntroLevel);
  _pShell->DeclareSymbol("persistent user CTString sam_strGameName;",      (void *) &sam_strGameName);
  _pShell->DeclareSymbol("user CTString sam_strVersion;",    (void *) &sam_strVersion);
  _pShell->DeclareSymbol("user CTString sam_strFirstLevel;", (void *) &sam_strFirstLevel);
  _pShell->DeclareSymbol("user CTString sam_strModName;", (void *) &sam_strModName);
  _pShell->DeclareSymbol("persistent INDEX sam_bShowAllLevels;", (void *) &sam_bShowAllLevels);
  _pShell->DeclareSymbol("persistent INDEX sam_bMentalActivated;", (void *) &sam_bMentalActivated);

  _pShell->DeclareSymbol("user void Quit(void);", (void *) &QuitGame);

  _pShell->DeclareSymbol("persistent user INDEX sam_iVideoSetup;",     (void *) &sam_iVideoSetup);
  _pShell->DeclareSymbol("user void ApplyRenderingPreferences(void);", (void *) &ApplyRenderingPreferences);
  _pShell->DeclareSymbol("user void ApplyVideoMode(void);",            (void *) &ApplyVideoMode);
  _pShell->DeclareSymbol("user void Benchmark(void);", (void *) &BenchMark);

  _pShell->DeclareSymbol("user INDEX sam_bMenuSave;",     (void *) &sam_bMenuSave);
  _pShell->DeclareSymbol("user INDEX sam_bMenuLoad;",     (void *) &sam_bMenuLoad);
  _pShell->DeclareSymbol("user INDEX sam_bMenuControls;", (void *) &sam_bMenuControls);
  _pShell->DeclareSymbol("user INDEX sam_bMenuHiScore;",  (void *) &sam_bMenuHiScore);
  _pShell->DeclareSymbol("user INDEX sam_bToggleConsole;",(void *) &sam_bToggleConsole);
  _pShell->DeclareSymbol("INDEX sam_iStartCredits;", (void *) &sam_iStartCredits);

  InitializeGame();
  _pNetwork->md_strGameID = sam_strGameName;

  _pGame->LCDInit();

  if( sam_bFirstStarted) {
    InfoMessage("%s", TRANS(
      "SeriousSam is starting for the first time.\n"
      "If you experience any problems, please consult\n"
      "ReadMe file for troubleshooting information."));
  }

  // initialize sound library
  snd_iFormat = Clamp( snd_iFormat, (INDEX)CSoundLibrary::SF_NONE, (INDEX)CSoundLibrary::SF_44100_16);
  _pSound->SetFormat( (enum CSoundLibrary::SoundFormat)snd_iFormat);

  if (sam_bAutoAdjustAudio) {
    _pShell->Execute("include \"Scripts\\Addons\\SFX-AutoAdjust.ini\"");
  }

  // execute script given on command line
  if (cmd_strScript!="") {
    CPrintF("Command line script: '%s'\n", (const char *) cmd_strScript);
    CTString strCmd;
    strCmd.PrintF("include \"%s\"", (const char *) cmd_strScript);
    _pShell->Execute(strCmd);
  }

	//#################################################################33
	//In order to fix completely fucked up menu layout (when wide screen is used)
	//and since dumb fucks from croteam defined fucking 200000 constats for each letter on the fucking screen...
	//we'll need to fix some of them for current resolution....
  	SetAdjusters();
	//#################################################################33


  // load logo textures
  LoadAndForceTexture(_toLogoCT,   _ptoLogoCT,   CTFILENAME("Textures\\Logo\\LogoCT.tex"));
  LoadAndForceTexture(_toLogoODI,  _ptoLogoODI,  CTFILENAME("Textures\\Logo\\GodGamesLogo.tex"));
  LoadAndForceTexture(_toLogoEAX,  _ptoLogoEAX,  CTFILENAME("Textures\\Logo\\LogoEAX.tex"));

  LoadStringVar(CTString("Data\\Var\\Sam_Version.var"), sam_strVersion);
  LoadStringVar(CTString("Data\\Var\\ModName.var"), sam_strModName);
  CPrintF(TRANSV("Serious Sam version: %s\n"), (const char *) sam_strVersion);
  CPrintF(TRANSV("Active mod: %s\n"), (const char *) sam_strModName);
  InitializeMenus();
  
  // if there is a mod
  if (_fnmMod!="") {
    // execute the mod startup script
    _pShell->Execute(CTString("include \"Scripts\\Mod_startup.ini\";"));
  }

  // init gl settings module
  InitGLSettings();

  // init level-info subsystem
  LoadLevelsList();
  LoadDemosList();

  // apply application mode
  StartNewMode( (GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ, sam_iAspectSizeI, sam_iAspectSizeJ, (enum DisplayDepth)sam_iDisplayDepth, sam_bFullScreenActive ,sam_bBorderLessActive);

  // set default mode reporting
  if( sam_bFirstStarted) {
    _iDisplayModeChangeFlag = 0;
    sam_bFirstStarted = FALSE;
  }
  
  HideSplashScreen();

  if (cmd_strPassword!="") {
    _pShell->SetString("net_strConnectPassword", cmd_strPassword);
  }

  // if connecting to server from command line
  if (cmd_strServer!="") {
    CTString strPort = "";
    if (cmd_iPort>0) {
      _pShell->SetINDEX("net_iPort", cmd_iPort);
      strPort.PrintF(":%d", cmd_iPort);
    }
    CPrintF(TRANSV("Command line connection: '%s%s'\n"), (const char *) cmd_strServer, (const char *) strPort);
    // go to join menu
    _pGame->gam_strJoinAddress = cmd_strServer;
    if (cmd_bQuickJoin) {
      extern void JoinNetworkGame(void);
      JoinNetworkGame();
    } else {
      StartMenus("join");
    }
  // if starting world from command line
  } else if (cmd_strWorld!="") {
    CPrintF(TRANSV("Command line world: '%s'\n"), (const char *) cmd_strWorld);
    // try to start the game with that level
    try {
      if (cmd_iGoToMarker>=0) {
        CPrintF(TRANSV("Command line marker: %d\n"), cmd_iGoToMarker);
        CTString strCommand;
        strCommand.PrintF("cht_iGoToMarker = %d;", cmd_iGoToMarker);
        _pShell->Execute(strCommand);
      }
      _pGame->gam_strCustomLevel = cmd_strWorld;
      if (cmd_bServer) {
        extern void StartNetworkGame(void);
        StartNetworkGame();
      } else {
        extern void StartSinglePlayerGame(void);
        StartSinglePlayerGame();
      }
    } catch (const char *strError) {
      CPrintF(TRANSV("Cannot start '%s': '%s'\n"), (const char *) cmd_strWorld, (const char *) strError);
    }
  // if no relevant starting at command line
  } else {
    StartNextDemo();
  }
  return TRUE;
}


void End(void)
{
  _pGame->DisableLoadingHook();
  // cleanup level-info subsystem
  ClearLevelsList();
  ClearDemosList();

  // destroy the main window and its canvas
  if (pvpViewPort!=NULL) {
    _pGfx->DestroyWindowCanvas( pvpViewPort);
    pvpViewPort = NULL;
    pdpNormal   = NULL;
  }
  CloseMainWindow();
  MainWindow_End();
  DestroyMenus();
  _pGame->End();
  _pGame->LCDEnd();
  // unlock the directory
  DirectoryLockOff();
  SE_EndEngine();

#if PLATFORM_UNIX
  SDL_Quit();
#endif
}


// print display mode info if needed
void PrintDisplayModeInfo(void)
{
  // skip if timed out
  if( _pTimer->GetRealTimeTick() > (_tmDisplayModeChanged+sam_tmDisplayModeReport)) return;

  // cache some general vars
  SLONG slDPWidth  = pdp->GetWidth();
  SLONG slDPHeight = pdp->GetHeight();
  if( pdp->IsDualHead()) slDPWidth/=2;

  CDisplayMode dm;
  dm.dm_pixSizeI = slDPWidth;
  dm.dm_pixSizeJ = slDPHeight;
  // determine proper text scale for statistics display
  FLOAT fTextScale = (FLOAT)slDPWidth/640.0f;

  // get resolution
  CTString strRes;
  extern CTString _strPreferencesDescription;
  strRes.PrintF( "%dx%dx%s", slDPWidth, slDPHeight, (const char *) _pGfx->gl_dmCurrentDisplayMode.DepthString());
  if( dm.IsDualHead())   strRes += TRANS(" DualMonitor");
  if( dm.IsWideScreen()) strRes += TRANS(" WideScreen");
       if( _pGfx->gl_eCurrentAPI==GAT_OGL) strRes += " (OpenGL)";
#ifdef PLATFORM_WIN32
#ifdef SE1_D3D
  else if( _pGfx->gl_eCurrentAPI==GAT_D3D) strRes += " (Direct3D)";
#endif // SE1_D3D
#endif // PLATFORM_WIN32


  CTString strDescr;
  strDescr.PrintF("\n%s (%s)\n", (const char *) _strPreferencesDescription, (const char *) RenderingPreferencesDescription(sam_iVideoSetup));
  strRes+=strDescr;
  // tell if application is started for the first time, or failed to set mode
  if( _iDisplayModeChangeFlag==0) {
    strRes += TRANS("Display mode set by default!");
  } else if( _iDisplayModeChangeFlag==2) {
    strRes += TRANS("Last mode set failed!");
  }

  // print it all
  pdp->SetFont( _pfdDisplayFont);
  pdp->SetTextScaling( fTextScale);
  pdp->SetTextAspect( 1.0f);
  pdp->PutText( strRes, slDPWidth*0.05f, slDPHeight*0.85f, _pGame->LCDGetColor(C_GREEN|255, "display mode"));
}

// do the main game loop and render screen
void DoGame(void)
{
  #ifdef SINGLE_THREADED
    _pTimer->HandleTimerHandlers();
  #endif

  // set flag if not in game
  if( !_pGame->gm_bGameOn) _gmRunningGameMode = GM_NONE;

  if( (_gmRunningGameMode==GM_DEMO  && _pNetwork->IsDemoPlayFinished())
    ||(_gmRunningGameMode==GM_INTRO && _pNetwork->IsGameFinished())) {
    _pGame->StopGame();
    _gmRunningGameMode = GM_NONE;

    // load next demo
    StartNextDemo();
    if (!_bInAutoPlayLoop) {
      // start menu
      StartMenus();
    }
  }

  // do the main game loop
  if( _gmRunningGameMode != GM_NONE) {
    _pGame->GameMainLoop();
  // if game is not started
  } else {
    // just handle broadcast messages
    _pNetwork->GameInactive();
  }

  if (sam_iStartCredits>0) {
    Credits_On(sam_iStartCredits);
    sam_iStartCredits = 0;
  }
  if (sam_iStartCredits<0) {
    Credits_Off();
    sam_iStartCredits = 0;
  }
  if( _gmRunningGameMode==GM_NONE) {
    Credits_Off();
    sam_iStartCredits = 0;
  }

  // redraw the view
  if( !IsIconic(_hwndMain) && pdp!=NULL && pdp->Lock())
  {
    if( _gmRunningGameMode!=GM_NONE && !bMenuActive ) {
      // handle pretouching of textures and shadowmaps
      pdp->Unlock();
      _pGame->GameRedrawView( pdp, (_pGame->gm_csConsoleState!=CS_OFF || bMenuActive)?0:GRV_SHOWEXTRAS);
      pdp->Lock();
      _pGame->ComputerRender(pdp);
      pdp->Unlock();
      CDrawPort dpScroller(pdp, TRUE);
      dpScroller.Lock();
      if (Credits_Render(&dpScroller)==0) {
        Credits_Off();
      }
      dpScroller.Unlock();
      pdp->Lock();
    } else {
      pdp->Fill( _pGame->LCDGetColor(C_dGREEN|CT_OPAQUE, "bcg fill"));
    }

    // do menu
    if( bMenuRendering) {
      // clear z-buffer
      pdp->FillZBuffer( ZBUF_BACK);
      // remember if we should render menus next tick
      bMenuRendering = DoMenu(pdp);
    }

    // print display mode info if needed
    PrintDisplayModeInfo();

    // render console
    _pGame->ConsoleRender(pdp);

    // done with all
    pdp->Unlock();

    // clear upper and lower parts of screen if in wide screen mode
    if( pdp==pdpWideScreen && pdpNormal->Lock()) {
      const PIX pixWidth  = pdpWideScreen->GetWidth();
      const PIX pixHeight = (pdpNormal->GetHeight() - pdpWideScreen->GetHeight()) /2;
      const PIX pixJOfs   = pixHeight + pdpWideScreen->GetHeight()-1;
      pdpNormal->Fill( 0, 0,       pixWidth, pixHeight, C_BLACK|CT_OPAQUE);
      pdpNormal->Fill( 0, pixJOfs, pixWidth, pixHeight, C_BLACK|CT_OPAQUE);
      pdpNormal->Unlock();
    }
    // show
    pvpViewPort->SwapBuffers();
  }
}


void TeleportPlayer(int iPosition)
{
  CTString strCommand;
  strCommand.PrintF( "cht_iGoToMarker = %d;", iPosition);
  _pShell->Execute(strCommand);
}


CTextureObject _toStarField;
static FLOAT _fLastVolume = 1.0f;
void RenderStarfield(CDrawPort *pdp, FLOAT fStrength)
{
  CTextureData *ptd = (CTextureData *)_toStarField.GetData();
  // skip if no texture
  if(ptd==NULL) return;

  PIX pixSizeI = pdp->GetWidth();
  PIX pixSizeJ = pdp->GetHeight();
  FLOAT fStretch = pixSizeI/640.0f;
  fStretch*=FLOAT(ptd->GetPixWidth())/ptd->GetWidth();

  PIXaabbox2D boxScreen(PIX2D(0,0), PIX2D(pixSizeI, pixSizeJ));
  MEXaabbox2D boxTexture(MEX2D(0, 0), MEX2D(pixSizeI/fStretch, pixSizeJ/fStretch));
  pdp->PutTexture(&_toStarField, boxScreen, boxTexture, LerpColor(C_BLACK, C_WHITE, fStrength)|CT_OPAQUE);
}


FLOAT RenderQuitScreen(CDrawPort *pdp, CViewPort *pvp)
{
  CDrawPort dpQuit(pdp, TRUE);
  CDrawPort dpWide;
  dpQuit.MakeWideScreen(&dpWide);
  // redraw the view
  if (!dpWide.Lock()) {
    return 0;
  }

  dpWide.Fill(C_BLACK|CT_OPAQUE);
  RenderStarfield(&dpWide, _fLastVolume);
  
  FLOAT fVolume = Credits_Render(&dpWide);
  _fLastVolume = fVolume;

  dpWide.Unlock();
  pvp->SwapBuffers();

  return fVolume;
}
void QuitScreenLoop(void)
{

  Credits_On(3);
  CSoundObject soMusic;
  try {
    _toStarField.SetData_t(CTFILENAME("Textures\\Background\\Night01\\Stars01.tex"));
    soMusic.Play_t(CTFILENAME("Music\\Credits.mp3"), SOF_NONGAME|SOF_MUSIC|SOF_LOOP);
  } catch (const char *strError) {
    CPrintF("%s\n", (const char *) strError);
  }
  // while it is still running
  FOREVER {
    FLOAT fVolume = RenderQuitScreen(pdp, pvpViewPort);
    if (fVolume<=0) {
      return;
    }
    // assure we can listen to non-3d sounds
    soMusic.SetVolume(fVolume, fVolume);
    _pSound->UpdateSounds();
    // while there are any messages in the message queue
    MSG msg;
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      // if it is not a keyboard or mouse message
      if(msg.message==WM_LBUTTONDOWN||
         msg.message==WM_RBUTTONDOWN||
         msg.message==WM_KEYDOWN) {
        return;
      }
    }

    //_pTimer->Sleep(5);

    #ifdef SINGLE_THREADED
      _pTimer->HandleTimerHandlers();
    #endif
  }
}

#ifdef PLATFORM_WIN32
// [Cecil] Game application is DPI-aware
static BOOL _bDPIAware = FALSE;
// Make game application be aware of the DPI scaling on Windows Vista and later
static void SetDPIAwareness(void) {
  // Load the library
  HMODULE hUser = LoadLibraryA("User32.dll");

  if (hUser == NULL) return;

  // Try to find the DPI awareness method
  typedef BOOL (*CSetAwarenessFunc)(void);
  CSetAwarenessFunc pFunc = (CSetAwarenessFunc)GetProcAddress(hUser, "SetProcessDPIAware");

  if (pFunc == NULL) return;

  // Mark game application as DPI-aware
  _bDPIAware = pFunc();
};
#endif // PLATFORM_WIN32

int SubMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  (void)hPrevInstance;

#ifdef PLATFORM_WIN32
  SetDPIAwareness();
#endif // PLATFORM_WIN32

  if( !Init( hInstance, nCmdShow, lpCmdLine )) return FALSE;

  // initialy, application is running and active, console and menu are off
  _bRunning    = TRUE;
  _bQuitScreen = TRUE;
  _pGame->gm_csConsoleState  = CS_OFF;
  _pGame->gm_csComputerState = CS_OFF;
//  bMenuActive    = FALSE;
//  bMenuRendering = FALSE;
  // while it is still running
  while( _bRunning && _fnmModToLoad=="")
  {
    // while there are any messages in the message queue
    MSG msg;
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE)) {
      // if it is not a mouse message
      if( !(msg.message>=WM_MOUSEFIRST && msg.message<=WM_MOUSELAST) ) {
        // if not system key messages
        if( !((msg.message==WM_KEYDOWN && msg.wParam==VK_F10)
            ||msg.message==WM_SYSKEYDOWN)) {
          // dispatch it
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }

      // system commands (also send by the application itself)
#ifdef PLATFORM_WIN32
      if( msg.message==WM_SYSCOMMAND)
      {
        switch( msg.wParam & ~0x0F) {
        // if should minimize
        case SC_MINIMIZE:
          if( _bWindowChanging) break;
          _bWindowChanging  = TRUE;
          _bReconsiderInput = TRUE;
          // if allowed, not already paused and only in single player game mode
          if( sam_bPauseOnMinimize && !_pNetwork->IsPaused() && _gmRunningGameMode==GM_SINGLE_PLAYER) {
            // pause game
            _pNetwork->TogglePause();
          }
          // if in full screen
          if( sam_bFullScreenActive) {
            // reset display mode and minimize window
            _pGfx->ResetDisplayMode();
            ShowWindow(_hwndMain, SW_MINIMIZE);
          // if not in full screen
          } else {
            // just minimize the window
            ShowWindow(_hwndMain, SW_MINIMIZE);
          }
          break;
        // if should restore
        case SC_RESTORE:
          if( _bWindowChanging) break;
          _bWindowChanging  = TRUE;
          _bReconsiderInput = TRUE;
          // if in full screen
          if( sam_bFullScreenActive) {
            ShowWindow(_hwndMain, SW_SHOWNORMAL);
            // set the display mode once again
            sam_bBorderLessActive = FALSE;
            StartNewMode( (GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ, sam_iAspectSizeI, sam_iAspectSizeJ, (enum DisplayDepth)sam_iDisplayDepth, sam_bFullScreenActive, sam_bBorderLessActive);
          // if not in full screen
          } else {
            // restore window
            ShowWindow(_hwndMain, SW_SHOWNORMAL);
          }
          break;
        // if should maximize
        case SC_MAXIMIZE:
          if( _bWindowChanging) break;
          _bWindowChanging  = TRUE;
          _bReconsiderInput = TRUE;
          // go to full screen
          StartNewMode( (GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ, sam_iAspectSizeI, sam_iAspectSizeJ, (enum DisplayDepth)sam_iDisplayDepth, TRUE, FALSE);
          ShowWindow( _hwndMain, SW_SHOWNORMAL);
          break;
        }
      }
#else
      STUBBED("SDL2 can handle these events");
#endif

      // toggle full-screen on alt-enter
      if( msg.message==WM_SYSKEYDOWN && msg.wParam==VK_RETURN && !IsIconic(_hwndMain)) {
        // !!! FIXME: SDL doesn't need to rebuild the GL context here to toggle fullscreen.
        //STUBBED("SDL doesn't need to rebuild the GL context here...");
        StartNewMode( (GfxAPIType)sam_iGfxAPI, sam_iDisplayAdapter, sam_iScreenSizeI, sam_iScreenSizeJ, sam_iAspectSizeI, sam_iAspectSizeJ, (enum DisplayDepth)sam_iDisplayDepth, !sam_bFullScreenActive, FALSE);
#ifdef PLATFORM_UNIX
        if (_pInput != NULL) // rcg02042003 hack for SDL vs. Win32.
          _pInput->ClearRelativeMouseMotion();
#endif
      }

      // if application should stop
      if( msg.message==WM_QUIT || msg.message==WM_CLOSE) {
        // stop running
        _bRunning = FALSE;
        _bQuitScreen = FALSE;
      }

#ifdef PLATFORM_WIN32
      // if application is deactivated or minimized
      if( (msg.message==WM_ACTIVATE && (LOWORD(msg.wParam)==WA_INACTIVE || HIWORD(msg.wParam)))
       ||  msg.message==WM_CANCELMODE
       ||  msg.message==WM_KILLFOCUS
       || (msg.message==WM_ACTIVATEAPP && !msg.wParam)) {
        // if application is running and in full screen mode
        if( !_bWindowChanging && _bRunning) {
          // minimize if in full screen 
          if( sam_bFullScreenActive) PostMessage(NULL, WM_SYSCOMMAND, SC_MINIMIZE, 0);
          // just disable input if not in full screen 
          else _pInput->DisableInput();
        }
      }
      // if application is activated or minimized
      else if( (msg.message==WM_ACTIVATE && (LOWORD(msg.wParam)==WA_ACTIVE || LOWORD(msg.wParam)==WA_CLICKACTIVE))
            ||  msg.message==WM_SETFOCUS
            || (msg.message==WM_ACTIVATEAPP && msg.wParam)) {
        // enable input back again if needed
        _bReconsiderInput = TRUE;
      }
#endif

      if (msg.message==WM_KEYDOWN && msg.wParam==VK_ESCAPE && 
        (_gmRunningGameMode==GM_DEMO || _gmRunningGameMode==GM_INTRO)) {
        _pGame->StopGame();
        _gmRunningGameMode=GM_NONE;
      }

      if (_pGame->gm_csConsoleState==CS_TALK && msg.message==WM_KEYDOWN && msg.wParam==VK_ESCAPE) {
#ifdef PLATFORM_UNIX
        if (_pInput != NULL) // rcg02042003 hack for SDL vs. Win32.
          _pInput->ClearRelativeMouseMotion();
#endif
        _pGame->gm_csConsoleState = CS_OFF;
        msg.message=WM_NULL;
      }

      BOOL bMenuForced = (_gmRunningGameMode==GM_NONE && 
        (_pGame->gm_csConsoleState==CS_OFF || _pGame->gm_csConsoleState==CS_TURNINGOFF));
      BOOL bMenuToggle = (msg.message==WM_KEYDOWN && msg.wParam==VK_ESCAPE 
        && (_pGame->gm_csComputerState==CS_OFF || _pGame->gm_csComputerState==CS_ONINBACKGROUND));
      if( !bMenuActive) {
        if( bMenuForced || bMenuToggle) {
          // if console is active
          if( _pGame->gm_csConsoleState==CS_ON || _pGame->gm_csConsoleState==CS_TURNINGON) {
            // deactivate it
            _pGame->gm_csConsoleState = CS_TURNINGOFF;
            _iAddonExecState = 0;
          }
          // delete key down message so menu would not exit because of it
          msg.message=WM_NULL;
          // start menu
          StartMenus();
        }
      } else {
        if (bMenuForced && bMenuToggle && pgmCurrentMenu->gm_pgmParentMenu == NULL) {
          // delete key down message so menu would not exit because of it
          msg.message=WM_NULL;
        }
      }

      // if neither menu nor console is running
      if (!bMenuActive && (_pGame->gm_csConsoleState==CS_OFF || _pGame->gm_csConsoleState==CS_TURNINGOFF)) {
        // if current menu is not root
        if (!IsMenusInRoot()) {
          // start current menu
          StartMenus();
        }
      }

      if (sam_bMenuSave) {
        sam_bMenuSave = FALSE;
        StartMenus("save");
      }
      if (sam_bMenuLoad) {
        sam_bMenuLoad = FALSE;
        StartMenus("load");
      }
      if (sam_bMenuControls) {
        sam_bMenuControls = FALSE;
        StartMenus("controls");
      }
      if (sam_bMenuHiScore) {
        sam_bMenuHiScore = FALSE;
        StartMenus("hiscore");
      }

      // interpret console key presses
      if (_iAddonExecState==0) {
        if (msg.message==WM_KEYDOWN) {
          _pGame->ConsoleKeyDown(msg);
          if (_pGame->gm_csConsoleState!=CS_ON) {
            _pGame->ComputerKeyDown(msg);
          }
        } else if (msg.message==WM_KEYUP) {
          // special handler for talk (not to invoke return key bind)
          if( msg.wParam==VK_RETURN && _pGame->gm_csConsoleState==CS_TALK)
          {
#ifdef PLATFORM_UNIX
            if (_pInput != NULL) // rcg02042003 hack for SDL vs. Win32.
              _pInput->ClearRelativeMouseMotion();
#endif
            _pGame->gm_csConsoleState = CS_OFF;
          }
        } else if (msg.message==WM_CHAR) {
          _pGame->ConsoleChar(msg);
        }
        if (msg.message==WM_LBUTTONDOWN
          ||msg.message==WM_RBUTTONDOWN
          ||msg.message==WM_LBUTTONDBLCLK
          ||msg.message==WM_RBUTTONDBLCLK
          ||msg.message==WM_LBUTTONUP
          ||msg.message==WM_RBUTTONUP) {
          if (_pGame->gm_csConsoleState!=CS_ON) {
            _pGame->ComputerKeyDown(msg);
          }
        }
      }
      // if menu is active and no input on
      if( bMenuActive && !_pInput->IsInputEnabled()) {
        // pass keyboard/mouse messages to menu
        if(msg.message==WM_KEYDOWN) {
          MenuOnKeyDown( msg.wParam);
        } else if (msg.message==WM_LBUTTONDOWN || msg.message==WM_LBUTTONDBLCLK) {
          MenuOnKeyDown(VK_LBUTTON);
        } else if (msg.message==WM_RBUTTONDOWN || msg.message==WM_RBUTTONDBLCLK) {
          MenuOnKeyDown(VK_RBUTTON);
        } else if (msg.message==WM_MOUSEMOVE) {
          MenuOnMouseMove(LOWORD(msg.lParam), HIWORD(msg.lParam));
#ifndef WM_MOUSEWHEEL
 #define WM_MOUSEWHEEL 0x020A
#endif
        } else if (msg.message==WM_MOUSEWHEEL) {
          SWORD swDir = SWORD(UWORD(HIWORD(msg.wParam)));
          if (swDir>0) {
            MenuOnKeyDown(11);
          } else if (swDir<0) {
            MenuOnKeyDown(10);
          }
        } else if (msg.message==WM_CHAR) {
          MenuOnChar(msg);
        }
      }

      // if toggling console
      BOOL bConsoleKey = sam_bToggleConsole || (msg.message==WM_KEYDOWN && 
            // !!! FIXME: rcg11162001 This sucks.
            // FIXME: DG: we could use SDL_SCANCODE_GRAVE ?
        #ifdef PLATFORM_UNIX
        (msg.wParam == SDLK_BACKQUOTE
        #else
        (MapVirtualKey(msg.wParam, 0)==41 // scan code for '~'
        #endif
        || msg.wParam==VK_F1 || (msg.wParam==VK_ESCAPE && _iAddonExecState==3)));
      if(bConsoleKey && !_bDefiningKey)
      {
        sam_bToggleConsole = FALSE;
        if( _iAddonExecState==3) _iAddonExecState = 0;
        // if it is up, or pulling up
        if( _pGame->gm_csConsoleState==CS_OFF || _pGame->gm_csConsoleState==CS_TURNINGOFF) {
          // start it moving down and disable menu
          _pGame->gm_csConsoleState = CS_TURNINGON;
          // stop all IFeel effects
          IFeel_StopEffect(NULL);
          if( bMenuActive) {
            StopMenus(FALSE);
          }
        // if it is down, or dropping down
        } else if( _pGame->gm_csConsoleState==CS_ON || _pGame->gm_csConsoleState==CS_TURNINGON) {
          // start it moving up
          _pGame->gm_csConsoleState = CS_TURNINGOFF;
        }
      }

      if (_pShell->GetINDEX("con_bTalk") && _pGame->gm_csConsoleState==CS_OFF) {
        _pShell->SetINDEX("con_bTalk", FALSE);
        _pGame->gm_csConsoleState = CS_TALK;
      }

      // if pause pressed
      if (msg.message==WM_KEYDOWN && msg.wParam==VK_PAUSE) {
        // toggle pause
        _pNetwork->TogglePause();
      }

#ifdef PLATFORM_WIN32
      // if command sent from external application
      if (msg.message==WM_COMMAND) {
        // if teleport player
        if (msg.wParam==1001) {
          // teleport player
          TeleportPlayer(msg.lParam);
          // restore
          PostMessage(NULL, WM_SYSCOMMAND, SC_RESTORE, 0);
        }
      }
#endif

      // if demo is playing
      if (_gmRunningGameMode==GM_DEMO ||
          _gmRunningGameMode==GM_INTRO ) {
        // check if escape is pressed
        BOOL bEscape = (msg.message==WM_KEYDOWN && msg.wParam==VK_ESCAPE);
        // check if console-invoke key is pressed
        BOOL bTilde = (msg.message==WM_KEYDOWN && 
          (msg.wParam==VK_F1 ||
            // !!! FIXME: ugly.
            #ifdef PLATFORM_UNIX
              msg.wParam == SDLK_BACKQUOTE
            #else
              MapVirtualKey(msg.wParam, 0)==41 // scan code for '~'
            #endif
          ));
        // check if any key is pressed
        BOOL bAnyKey = (
          (msg.message==WM_KEYDOWN && (msg.wParam==VK_SPACE || msg.wParam==VK_RETURN))|| 
          msg.message==WM_LBUTTONDOWN||msg.message==WM_RBUTTONDOWN);

        // if escape is pressed
        if (bEscape) {
          // stop demo
          _pGame->StopGame();
          _bInAutoPlayLoop = FALSE;
          _gmRunningGameMode = GM_NONE;
        // if any other key is pressed except console invoking
        } else if (bAnyKey && !bTilde) {
          // if not in menu or in console
          if (!bMenuActive && !bMenuRendering && _pGame->gm_csConsoleState==CS_OFF) {
            // skip to next demo
            _pGame->StopGame();
            _gmRunningGameMode = GM_NONE;
            StartNextDemo();        
          }
        }
      }

    } // loop while there are messages

    // when all messages are removed, window has surely changed
    _bWindowChanging = FALSE;

    // get real cursor position
    if( _pGame->gm_csComputerState!=CS_OFF && _pGame->gm_csComputerState!=CS_ONINBACKGROUND) {
      POINT pt;
      ::GetCursorPos(&pt);
      ::ScreenToClient(_hwndMain, &pt);
      _pGame->ComputerMouseMove(pt.x, pt.y);
    }

    // if addon is to be executed
    if (_iAddonExecState==1) {
      // print header and start console
      CPrintF(TRANSV("---- Executing addon: '%s'\n"), (const char*)_fnmAddonToExec);
      sam_bToggleConsole = TRUE;
      _iAddonExecState = 2;
    // if addon is ready for execution
    } else if (_iAddonExecState==2 && _pGame->gm_csConsoleState == CS_ON) {
      // execute it
      CTString strCmd;
      strCmd.PrintF("include \"%s\"", (const char*)_fnmAddonToExec);
      _pShell->Execute(strCmd);
      CPrintF(TRANSV("Addon done, press Escape to close console\n"));
      _iAddonExecState = 3;
    }

    // automaticaly manage input enable/disable toggling
    UpdateInputEnabledState();
    // automaticaly manage pause toggling
    UpdatePauseState();
    // notify game whether menu is active
    _pGame->gm_bMenuOn = bMenuActive;

    // do the main game loop and render screen
    DoGame();

    // limit current frame rate if neeeded
    LimitFrameRate();

  } // end of main application loop

  _pInput->DisableInput();
  _pGame->StopGame();
  
  // invoke quit screen if needed
  if( _bQuitScreen && _fnmModToLoad=="") QuitScreenLoop();

  End();
  return TRUE;
}

void CheckModReload(void)
{
  if (_fnmModToLoad!="") {
#ifndef NDEBUG
#ifdef PLATFORM_WIN32
    CTString strDebug = "Debug\\";
#else
    CTString strDebug = "Debug/";
#endif
#else
    CTString strDebug = "";
#endif

#ifdef PLATFORM_WIN32
	CTString strCommand = "SeriousSam.exe";
    CTString strPatch = _fnmApplicationPath+"Bin\\"+strDebug+strCommand;
#else
    CTString strCommand;
    if (sys_iSysPath == 1) {
      strCommand = sam_strGameName;
    } else {
      strCommand = "SeriousSam";
    }
    CTString strPatch = CTString(_strExePath) + strDebug + strCommand;
#endif
    //+mod "+_fnmModToLoad.FileName()+"\"";
    CTString strMod = _fnmModToLoad.FileName();
    const char *argv[8];
    argv[0] = (const char *)strPatch;
    argv[1] = (const char *)strCommand;
    argv[2] = (const char *)"+game";
    argv[3] = (const char *)strMod;
    argv[4] = NULL;
    argv[5] = NULL;
    argv[6] = NULL;
    argv[7] = NULL;
    if (_strModServerJoin!="") {
      argv[4] = (const char *)" +connect ";
      argv[5] = (const char *)_strModServerJoin;
      argv[6] = (const char *)" +quickjoin ";
      argv[7] = NULL;
    }
#ifdef PLATFORM_WIN32
    _execl((const char *)argv[0],(const char *)argv[1],(const char *)argv[2],(const char *)argv[3], \
       (const char *)argv[4],(const char *)argv[5],(const char *)argv[6],(const char *)argv[7]);
    MessageBoxA(0, "Error launching the Mod!\n", "Serious Sam", MB_OK|MB_ICONERROR);
#else
    execl((const char *)argv[0],(const char *)argv[1],(const char *)argv[2],(const char *)argv[3], \
       (const char *)argv[4],(const char *)argv[5],(const char *)argv[6],(const char *)argv[7], nullptr);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                         "Serious Sam",
                         "Error launching the Mod!\n",
                         NULL);
#endif
  }
}

void CheckTeaser(void)
{
#ifdef PLATFORM_WIN32
  CTFileName fnmTeaser = _fnmApplicationPath+CTString("Bin\\AfterSam.exe");
  if (fopen(fnmTeaser, "r")!=NULL) {
    Sleep(500);
    _execl(fnmTeaser, "\""+fnmTeaser+"\"", NULL);
  }
#else
    STUBBED("load teaser");
#endif
}

void CheckBrowser(void)
{
  if (_strURLToVisit!="") {
    RunBrowser(_strURLToVisit);
  }
}


int CommonMainline( HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow)
{
  int iResult;
  CTSTREAM_BEGIN {
    iResult = SubMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
  } CTSTREAM_END;
  
  CheckModReload();

  CheckTeaser();

  CheckBrowser();

  return iResult;
}



#ifdef PLATFORM_WIN32

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow)
{
  argv0 = new char[MAX_PATH];
  memset(argv0, '\0', sizeof (argv0));
  GetModuleFileNameA(NULL, argv0, MAX_PATH-1);
  const int rc = CommonMainline(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
  delete[] argv0;
  argv0 = NULL;
  return rc;
}

#else


// !!! FIXME: rcg01052002 This should really get dumped to the game's
// !!! FIXME: rcg01052002  console so it's in the log file, too.
#ifdef BETAEXPIRE
static inline void check_beta(void)
{
  bool bail = false;

  setbuf(stderr, NULL);
  fprintf(stderr, "\n\n\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");

  if ( time(NULL) > (BETAEXPIRE + 30 * 24 * 60 * 60) ) {
    fprintf(stderr,
            "Sorry, but this beta of the game has expired, and will no\n"
            " longer run. This is to prevent tech support on out-of-date\n"
            " and prerelease versions of the game. Please go to\n"
            " http://www.croteam.com/ for information on getting a release\n"
            " version that does not expire.\n");
    bail = true;
  } else {
    fprintf(stderr, "     Warning: This is a beta version of SERIOUS SAM.\n");
  }

  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "\n\n\n");

  if (bail) {
    _exit(0);
  }
} // check_beta
#endif


// !!! FIXME: rcg01102002 This should really get dumped to the game's
// !!! FIXME: rcg01102002  console so it's in the log file, too.
#ifdef PROFILING_ENABLED
static inline void warn_profiling(void)
{
  setbuf(stderr, NULL);
  fprintf(stderr, "\n\n\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "      Warning: Profiling is enabled in this binary!\n");
  fprintf(stderr, "         DO NOT SHIP A BINARY WITH THIS ENABLED!\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "*********************************************************\n");
  fprintf(stderr, "\n\n\n");
} // check_beta
#endif



int main(int argc, char **argv)
{
  #ifdef BETAEXPIRE
    // !!! FIXME: This is Unix-centric (at least, non-win32) if put in main().
    check_beta();
  #endif

  #ifdef PROFILING_ENABLED
    // !!! FIXME: This is Unix-centric (at least, non-win32) if put in main().
    warn_profiling();
  #endif

  argv0 = argv[0];

  CTString cmdLine;
  for (int i = 1; i < argc; i++) {
    cmdLine += " \"";
    cmdLine += argv[i];
    cmdLine += "\"";
  }

  return(CommonMainline(NULL, NULL, (char *) ((const char *) cmdLine), 0));
}

#endif


// try to start a new display mode
BOOL TryToSetDisplayMode( enum GfxAPIType eGfxAPI, INDEX iAdapter, PIX pixSizeI, PIX pixSizeJ,
                          PIX aspSizeI, PIX aspSizeJ, enum DisplayDepth eColorDepth, BOOL bFullScreenMode, BOOL bBorderLessMode)
{
  CDisplayMode dmTmp;
  dmTmp.dm_ddDepth = eColorDepth;
  CPrintF( TRANS("  Starting display mode: %dx%dx%s (%s, %s)\n"),
           pixSizeI, pixSizeJ, (const char *) dmTmp.DepthString(),
           bFullScreenMode ? TRANS("fullscreen") : TRANS("window"),
           bBorderLessMode ? TRANS("borderless") : TRANS("normal"));

  // mark to start ignoring window size/position messages until settled down
  _bWindowChanging = TRUE;
  
  // destroy canvas if existing
  _pGame->DisableLoadingHook();
  if( pvpViewPort!=NULL) {
    _pGfx->DestroyWindowCanvas( pvpViewPort);
    pvpViewPort = NULL;
    pdpNormal = NULL;
  }

  // close the application window
  CloseMainWindow();

  // try to set new display mode
  BOOL bSuccess;
   
  // TODO: enable full screen
  if( bFullScreenMode) {
#ifdef SE1_D3D
    if( eGfxAPI==GAT_D3D) OpenMainWindowFullScreen( pixSizeI, pixSizeJ);
#endif // SE1_D3D
    bSuccess = _pGfx->SetDisplayMode( eGfxAPI, iAdapter, pixSizeI, pixSizeJ, eColorDepth);
    if( bSuccess && eGfxAPI==GAT_OGL) OpenMainWindowFullScreen( pixSizeI, pixSizeJ);
  } else {
#ifdef SE1_D3D
    if( eGfxAPI==GAT_D3D) OpenMainWindowNormal( pixSizeI, pixSizeJ);
#endif // SE1_D3D
    bSuccess = _pGfx->ResetDisplayMode( eGfxAPI);
    if( bSuccess && eGfxAPI==GAT_OGL) OpenMainWindowNormal( pixSizeI, pixSizeJ);
#ifdef SE1_D3D
    if( bSuccess && eGfxAPI==GAT_D3D) ResetMainWindowNormal();
#endif // SE1_D3D
  }

  // if new mode was set
  if( bSuccess) {
    // create canvas
    ASSERT( pvpViewPort==NULL);
    ASSERT( pdpNormal==NULL);
    _pGfx->CreateWindowCanvas( _hwndMain, &pvpViewPort, &pdpNormal);

    // erase context of both buffers (for the sake of wide-screen)
    pdp = pdpNormal;
    if( pdp!=NULL && pdp->Lock()) {
      pdp->Fill(C_BLACK|CT_OPAQUE);
      pdp->Unlock();
      pvpViewPort->SwapBuffers();
      pdp->Lock();
      pdp->Fill(C_BLACK|CT_OPAQUE);
      pdp->Unlock();
      pvpViewPort->SwapBuffers();
    }

    // lets try some wide screen screaming :)
    const PIX pixYBegAdj = pdp->GetHeight() * 21/24;
    const PIX pixYEndAdj = pdp->GetHeight() * 3/24;
    const PIX pixXEnd    = pdp->GetWidth();
    pdpWideScreen = new CDrawPort( pdp, PIXaabbox2D( PIX2D(0,pixYBegAdj), PIX2D(pixXEnd, pixYEndAdj)));
    pdpWideScreen->dp_fWideAdjustment = 9.0f / 12.0f;
    if( sam_bWideScreen) pdp = pdpWideScreen;

    // initial screen fill and swap, just to get context running
    BOOL bSuccess = FALSE;
    if( pdp!=NULL && pdp->Lock()) {
      pdp->Fill(_pGame->LCDGetColor(C_dGREEN|CT_OPAQUE, "bcg fill"));
      pdp->Unlock();
      pvpViewPort->SwapBuffers();
      bSuccess = TRUE;
    }
    _pGame->EnableLoadingHook(pdp);

    // if the mode is not working, or is not accelerated
    if( !bSuccess || !_pGfx->IsCurrentModeAccelerated())
    { // report error
      CPrintF( TRANS("This mode does not support hardware acceleration.\n"));
      // destroy canvas if existing
      if( pvpViewPort!=NULL) {
        _pGame->DisableLoadingHook();
        _pGfx->DestroyWindowCanvas( pvpViewPort);
        pvpViewPort = NULL;
        pdpNormal = NULL;
      }
      // close the application window
      CloseMainWindow();
      // report failure
      return FALSE;
    }

    // remember new settings
    sam_bFullScreenActive = bFullScreenMode;
    sam_iScreenSizeI = pixSizeI;
    sam_iScreenSizeJ = pixSizeJ;
	sam_iAspectSizeI = aspSizeI;
	sam_iAspectSizeJ = aspSizeJ;
    sam_iDisplayDepth = eColorDepth;
    sam_iDisplayAdapter = iAdapter;
    sam_iGfxAPI = eGfxAPI;

    // report success
    return TRUE;
  // if couldn't set new mode
  } else {
    // close the application window
    CloseMainWindow();
    // report failure
    return FALSE;
  }
}


// list of possible display modes for recovery 
const INDEX aDefaultModes[][3] =
{ // color, API, adapter
  { DD_DEFAULT, GAT_OGL, 0},
  { DD_16BIT,   GAT_OGL, 0},
  { DD_16BIT,   GAT_OGL, 1}, // 3dfx Voodoo2
#ifdef SE1_D3D
  { DD_DEFAULT, GAT_D3D, 0},
  { DD_16BIT,   GAT_D3D, 0},
  { DD_16BIT,   GAT_D3D, 1},
#endif // SE1_D3D
};
const INDEX ctDefaultModes = ARRAYCOUNT(aDefaultModes);

// start new display mode
void StartNewMode( enum GfxAPIType eGfxAPI, INDEX iAdapter, PIX pixSizeI, PIX pixSizeJ, PIX aspSizeI, PIX aspSizeJ, enum DisplayDepth eColorDepth, BOOL bFullScreenMode, BOOL bBorderLessMode)
{
  CPrintF( TRANS("\n* START NEW DISPLAY MODE ...\n"));

  _pShell->Execute("gam_bEnableAdvancedObserving = 1;");
  // try to set the mode
  BOOL bSuccess = TryToSetDisplayMode( eGfxAPI, iAdapter, pixSizeI, pixSizeJ, aspSizeI, aspSizeJ, eColorDepth, bFullScreenMode, bBorderLessMode);

  // if failed
  if( !bSuccess)
  {
    // report failure and reset to default resolution
    _iDisplayModeChangeFlag = 2;  // failure
    CPrintF( TRANS("Requested display mode could not be set!\n"));
    pixSizeI = 640;
    pixSizeJ = 480;
    aspSizeI = 4;
    aspSizeJ = 3;
    bFullScreenMode = FALSE;
    // try to revert to one of recovery modes
    for( INDEX iMode=0; iMode<ctDefaultModes; iMode++) {
      eColorDepth = (DisplayDepth)aDefaultModes[iMode][0];
      eGfxAPI     = (GfxAPIType)  aDefaultModes[iMode][1];
      iAdapter    =               aDefaultModes[iMode][2];
      // set sam_iGfxAPI for SDL_CreateWindow
      sam_iGfxAPI = eGfxAPI;
      CPrintF(TRANSV("\nTrying recovery mode %d...\n"), iMode);
      bSuccess = TryToSetDisplayMode( eGfxAPI, iAdapter, pixSizeI, pixSizeJ, aspSizeI, aspSizeJ, eColorDepth, bFullScreenMode, bBorderLessMode);
      if( bSuccess) break;
    }
    // if all failed
    if( !bSuccess) {
      FatalError(TRANS(
        "Cannot set display mode!\n"
        "Serious Sam was unable to find display mode with hardware acceleration.\n"
        "Make sure you install proper drivers for your video card as recommended\n"
        "in documentation and set your desktop to 16 bit (65536 colors).\n"
        "Please see ReadMe file for troubleshooting information.\n"));
    }

  // if succeeded
  } else {
    _iDisplayModeChangeFlag = 1;  // all ok
  }

	//#################################################################33
	//In order to fix completely fucked up menu layout (when wide screen is used)
	//and since dumb fucks from croteam defined fucking 200000 constats for each letter on the fucking screen...
	//we'll need to fix some of them for current resolution....
  	SetAdjusters();
	//#################################################################33

  // apply 3D-acc settings
  ApplyGLSettings(FALSE);

  // remember time of mode setting
  _tmDisplayModeChanged = _pTimer->GetRealTimeTick();
}
#endif
