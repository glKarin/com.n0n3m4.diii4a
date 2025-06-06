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

// Game.cpp : Defines the initialization routines for the DLL.
//

#include "StdAfx.h"
#include "GameMP/Game.h"
#if !defined(PLATFORM_FREEBSD) && !defined(ANDROID) //karin: not on Android
#include <sys/timeb.h>
#endif
#include <time.h>
#include <locale.h>

#ifdef PLATFORM_WIN32
#include <direct.h> // for _mkdir()
#include <io.h>
#endif

#include <Engine/Base/Profiling.h>
#include <Engine/Base/Statistics.h>
#include <Engine/CurrentVersion.h>
#include <Engine/World/World.h>
#include "Entities/Common/LightFixes.h"
#include "Camera.h"
#include "LCDDrawing.h"

FLOAT con_fHeightFactor = 0.5f;
FLOAT con_tmLastLines   = 5.0f;
INDEX con_bTalk = 0;
CTimerValue _tvMenuQuickSave((__int64) 0);

// used filenames
CTFileName fnmPersistentSymbols = CTString("Scripts\\PersistentSymbols.ini");
CTFileName fnmStartupScript     = CTString("Scripts\\Game_startup.ini");
CTFileName fnmConsoleHistory    = CTString("Temp\\ConsoleHistory.txt");
CTFileName fnmCommonControls    = CTString("Controls\\System\\Common.ctl");

// force dependency for player class
DECLARE_CTFILENAME( fnmPlayerClass, "Classes\\Player.ecl");

#define MAX_HIGHSCORENAME 16
#define MAX_HIGHSCORETABLESIZE ((MAX_HIGHSCORENAME+1+sizeof(INDEX)*4)*HIGHSCORE_COUNT)*2
UBYTE _aubHighScoreBuffer[MAX_HIGHSCORETABLESIZE];
UBYTE _aubHighScorePacked[MAX_HIGHSCORETABLESIZE];

// controls used for all commands not belonging to any particular player
static CControls _ctrlCommonControls;

// array for keeping all frames' times
static CStaticStackArray<TIME>  _atmFrameTimes;
static CStaticStackArray<INDEX> _actTriangles;  // world, model, particle, total


// one and only Game object
// rcg11162001 This will resolve to the main binary under Linux, so it's
//  okay. It's just another copy of the same otherwise.
#ifdef PLATFORM_UNIX
extern CGame *_pGame;
#else
CGame *_pGame = NULL;
#endif

extern "C" 
{

#ifdef PLATFORM_WIN32
#define EXPORTABLE __declspec (dllexport) 
#else
#define EXPORTABLE
#endif

EXPORTABLE CGame *GAME_Create(void)
{
  _pGame = new CGame;

  return _pGame;
}

}  // extern "C"

// Just working around a symbol reference in a shared library that isn't
//  available in SeriousSam by turning gm_ctrlControlsExtra into a pointer
//  instead of a full object. Messy; sorry!   --ryan.
CGame::CGame() : gm_ctrlControlsExtra(new CControls) {}
CGame::~CGame() { delete gm_ctrlControlsExtra; }


// recorded profiling stats
static CTimerValue _tvDemoStarted;
static CTimerValue _tvLastFrame;
static CTString _strProfile;
static BOOL  _bDumpNextTime = FALSE;
static BOOL  _bStartProfilingNextTime = FALSE;
static BOOL  _bProfiling = FALSE;
static INDEX _ctProfileRecording = 0;
static FLOAT gam_iRecordHighScore = -1.0f;

FLOAT gam_afAmmoQuantity[5]        = {2.0f,  2.0f,  1.0f, 1.0f , 2.0f };
FLOAT gam_afDamageStrength[5]      = {0.25f, 0.5f,  1.0f, 1.5f , 2.0f };
FLOAT gam_afEnemyAttackSpeed[5]    = {0.75f, 0.75f, 1.0f, 2.0f , 2.0f };
FLOAT gam_afEnemyMovementSpeed[5]  = {1.0f , 1.0f , 1.0f, 1.25f, 1.25f};
FLOAT gam_fManaTransferFactor = 0.5f;
FLOAT gam_fExtraEnemyStrength          = 0;
FLOAT gam_fExtraEnemyStrengthPerPlayer = 0;
INDEX gam_iCredits = -1;   // number of credits for respawning
FLOAT gam_tmSpawnInvulnerability = 3;
INDEX gam_iScoreLimit = 100000;
INDEX gam_iFragLimit = 20;
INDEX gam_iTimeLimit = 0;
INDEX gam_bWeaponsStay = TRUE;
INDEX gam_bAmmoStays = TRUE;
INDEX gam_bHealthArmorStays = TRUE;
INDEX gam_bAllowHealth = TRUE;
INDEX gam_bAllowArmor = TRUE;
INDEX gam_bInfiniteAmmo = FALSE;
INDEX gam_bRespawnInPlace = TRUE;
INDEX gam_bPlayEntireGame = TRUE;
INDEX gam_bFriendlyFire = FALSE;
INDEX gam_ctMaxPlayers = 8;
INDEX gam_bWaitAllPlayers = FALSE;
INDEX gam_iInitialMana = 100;
INDEX gam_bQuickLoad = FALSE;
INDEX gam_bQuickSave = FALSE;
INDEX gam_iQuickSaveSlots = 8;

INDEX gam_iQuickStartDifficulty = 1;
INDEX gam_iQuickStartMode = 0;
INDEX gam_bQuickStartMP = 0;

INDEX gam_bEnableAdvancedObserving = 0;
INDEX gam_iObserverConfig = 0;
INDEX gam_iObserverOffset = 0;

INDEX gam_iStartDifficulty = 1;
INDEX gam_iStartMode = 0;
CTString gam_strGameAgentExtras = "";

INDEX gam_iBlood = 2;     // 0=none, 1=green, 2=red, 3=hippie
INDEX gam_bGibs  = TRUE;   

INDEX gam_bUseExtraEnemies = TRUE;

static INDEX hud_iEnableStats = 1;
static FLOAT hud_fEnableFPS   = 1;
static INDEX hud_iStats    = 0;
static INDEX hud_bShowTime = FALSE;
static INDEX hud_bShowClock = FALSE;
static INDEX hud_bShowNetGraph = FALSE;
static INDEX hud_bShowResolution = FALSE;
static INDEX dem_bOSD         = FALSE;
static INDEX dem_bPlay        = FALSE;
static INDEX dem_bPlayByName  = FALSE;
static INDEX dem_bProfile     = FALSE;
static INDEX dem_iProfileRate = 5;
static CTString dem_strPostExec = "";

static INDEX ctl_iCurrentPlayerLocal = -1;
static INDEX ctl_iCurrentPlayer = -1;
static FLOAT gam_fChatSoundVolume = 0.25f;

BOOL map_bIsFirstEncounter = FALSE;
BOOL _bUserBreakEnabled = FALSE;

// Fix illuminations bug metod
// 0 - none
// 1 - fix textrure settings
// 2 - create additional lighting (better) 
INDEX gam_bFixIlluminationsMetod = 2;

//***************************************************************
//****************  Fix Textures on some levels  ****************
//***************************************************************
void _ClearLights(void)
{
  {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, pen) {
    if(IsDerivedFromClass(pen, "Light")) {
      if(((CLight&)*pen).m_strName == "fix_texture"){
        pen->Destroy();
      }
    }
  }}
}

void _CreateLights(CPlacement3D pl, FLOAT _fFallOffRange)
{
  CEntity *pen = NULL;
  pen = _pNetwork->ga_World.CreateEntity_t(pl, CTFILENAME("Classes\\Light.ecl"));
  pen->Initialize();
  ((CLight&)*pen).m_colColor = C_GRAY;
  ((CLight&)*pen).m_ltType = LT_POINT;
  ((CLight&)*pen).m_bDarkLight = TRUE;
  ((CLight&)*pen).m_rFallOffRange = _fFallOffRange;
  ((CLight&)*pen).m_strName = "fix_texture";
  pen->en_ulSpawnFlags =0xFFFFFFFF;
  pen->Reinitialize();
}

void _FixTexturesValleyOfTheKings(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 4; i++) {
    FLOAT m_fCoord1 = _fValleyOfTheKingsCoordinates[i][0];
    FLOAT m_fCoord2 = _fValleyOfTheKingsCoordinates[i][1];
    FLOAT m_fCoord3 = _fValleyOfTheKingsCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
}

void _FixTexturesDunes(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 8; i++) {
    FLOAT m_fCoord1 = _fDunesCoordinates[i][0];
    FLOAT m_fCoord2 = _fDunesCoordinates[i][1];
    FLOAT m_fCoord3 = _fDunesCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
}

void _FixTexturesSuburbs(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 21; i++) {
    FLOAT m_fCoord1 = _fSuburbsCoordinates[i][0];
    FLOAT m_fCoord2 = _fSuburbsCoordinates[i][1];
    FLOAT m_fCoord3 = _fSuburbsCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
}

void _FixTexturesMetropolis(void) 
{
  _ClearLights();
  CPlacement3D pl;
  FLOAT m_fCoord1 = _fMetropolisCoordinates[0][0];
  FLOAT m_fCoord2 = _fMetropolisCoordinates[0][1];
  FLOAT m_fCoord3 = _fMetropolisCoordinates[0][2];
  pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
  _CreateLights(pl, 8.0f);
}

void _FixTexturesAlleyOfSphinxes(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 37; i++) {
    FLOAT m_fCoord1 = _fAlleyOfSphinxesCoordinates[i][0];
    FLOAT m_fCoord2 = _fAlleyOfSphinxesCoordinates[i][1];
    FLOAT m_fCoord3 = _fAlleyOfSphinxesCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  } 
}

void _FixTexturesKarnak(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 41; i++) {
    FLOAT m_fCoord1 = _fKarnakCoordinates[i][0];
    FLOAT m_fCoord2 = _fKarnakCoordinates[i][1];
    FLOAT m_fCoord3 = _fKarnakCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
  FLOAT m_fCoord1 = _fKarnakCoordinates[41][0];
  FLOAT m_fCoord2 = _fKarnakCoordinates[41][1];
  FLOAT m_fCoord3 = _fKarnakCoordinates[41][2];
  pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
  _CreateLights(pl, 4.0f);
}

void _FixTexturesLuxor(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 51; i++) {
    FLOAT m_fCoord1 = _fLuxorCoordinates[i][0];
    FLOAT m_fCoord2 = _fLuxorCoordinates[i][1];
    FLOAT m_fCoord3 = _fLuxorCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
  FLOAT m_fCoord1 = _fLuxorCoordinates[51][0];
  FLOAT m_fCoord2 = _fLuxorCoordinates[51][1];
  FLOAT m_fCoord3 = _fLuxorCoordinates[51][2];
  pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
  _CreateLights(pl, 1.0f);
}

void _FixTexturesSacredYards(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 27; i++) {
    FLOAT m_fCoord1 = _fSacredYardsCoordinates[i][0];
    FLOAT m_fCoord2 = _fSacredYardsCoordinates[i][1];
    FLOAT m_fCoord3 = _fSacredYardsCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
}

void _FixTexturesKarnakDemo(void) 
{
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 49; i++) {
    FLOAT m_fCoord1 = _fKarnakDemoCoordinates[i][0];
    FLOAT m_fCoord2 = _fKarnakDemoCoordinates[i][1];
    FLOAT m_fCoord3 = _fKarnakDemoCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
  FLOAT m_fCoord1 = _fKarnakDemoCoordinates[49][0];
  FLOAT m_fCoord2 = _fKarnakDemoCoordinates[49][1];
  FLOAT m_fCoord3 = _fKarnakDemoCoordinates[49][2];
  pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
  _CreateLights(pl, 4.0f);
}

void _FixTexturesIntro(void) 
{ 
  _ClearLights();
  CPlacement3D pl;
  for(int i = 0; i < 8; i++) {
    FLOAT m_fCoord1 = _fIntroCoordinates[i][0];
    FLOAT m_fCoord2 = _fIntroCoordinates[i][1];
    FLOAT m_fCoord3 = _fIntroCoordinates[i][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    _CreateLights(pl, 8.0f);
  }
}
//***************************************************************
//*********************** Old metods: ***************************
//****************  Fix Textures on Obelisk  ********************
//***************************************************************
void _FixTexturesOnObelisk(CTFileName strLevelName)
{
  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for all polygons in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
          {
            CTFileName strTextureName = itbpo->bpo_abptTextures[1].bpt_toTexture.GetName().FileName();
            int _Obelisk02Light_found   = strncmp((const char *)strTextureName, (const char *) "Obelisk02Light", (size_t) 14 );
            if (_Obelisk02Light_found == 0 ){
                // Settings:
                // itbpo->bpo_abptTextures[1].bpt_toTexture.GetName().FileName()
                // itbpo->bpo_abptTextures[1].s.bpt_ubBlend
                // itbpo->bpo_abptTextures[1].s.bpt_ubFlags 
                // itbpo->bpo_abptTextures[1].s.bpt_colColor
              if ( strLevelName=="KarnakDemo" || strLevelName=="Intro" || strLevelName=="08_Suburbs"
                || strLevelName=="13_Luxor" || strLevelName=="14_SacredYards") {
                itbpo->bpo_abptTextures[1].s.bpt_colColor = (C_WHITE| 0x5F);
              } else if ( strLevelName=="04_ValleyOfTheKings" || strLevelName=="11_AlleyOfSphinxes" || strLevelName=="12_Karnak"){
                itbpo->bpo_abptTextures[1].s.bpt_colColor = (C_GRAY| 0x2F);
              }
            }
          }
        }
      }
    } // END if()
  }}
}
//***************************************************************
//**********^**  Fix Textures on Alley Of Sphinxes  *************
//***************************************************************
void _FixTexturesOnAlleyOfSphinxes(void)
{
  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for all polygons in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
          {
            CTFileName strTextureName = itbpo->bpo_abptTextures[1].bpt_toTexture.GetName().FileName();
            int _EyeOfRa_found = strncmp((const char *)strTextureName, (const char *) "EyeOfRa", (size_t) 7 );
            int _Wall12_found  = strncmp((const char *)strTextureName, (const char *) "Wall12",  (size_t) 6 );
            int _Wingy02_found = strncmp((const char *)strTextureName, (const char *) "Wingy02", (size_t) 7 );
            if (_EyeOfRa_found == 0 || _Wall12_found == 0 || _Wingy02_found == 0){
              itbpo->bpo_abptTextures[1].s.bpt_ubBlend  = BPT_BLEND_BLEND;
              itbpo->bpo_abptTextures[1].s.bpt_colColor = C_GRAY|0x80;
            }
          }
        }
      }
    } // END if()
  }}
}
//***************************************************************
//***************************************************************
//***************************************************************

// make sure that console doesn't show last lines if not playing in network
void MaybeDiscardLastLines(void)
{
  // Get Level Name and Mod Name
  CTString strLevelName = _pNetwork->ga_fnmWorld.FileName();
  CTString strModName = _pShell->GetValue("sys_strModName");
  INDEX iBugFixMetod = _pShell->GetINDEX("gam_bFixIlluminationsMetod");

  if(iBugFixMetod == 1) {
    // Fix Obelisk textures
    if ( strModName=="" ) {
      if ( strLevelName=="04_ValleyOfTheKings" || strLevelName=="11_AlleyOfSphinxes" || strLevelName=="12_Karnak" 
        || strLevelName=="13_Luxor" || strLevelName=="KarnakDemo" || strLevelName=="Intro" 
        || strLevelName=="08_Suburbs" || strLevelName=="14_SacredYards") {
        _FixTexturesOnObelisk(strLevelName);
      }
    }
    // Fix Alley Of Sphinxes textures
    if (/* strModName=="" && */ strLevelName=="11_AlleyOfSphinxes") {
      _FixTexturesOnAlleyOfSphinxes();
    }
  } else if (iBugFixMetod == 2) {
    // Fix textures
    if (/* strModName==""&& */ strLevelName=="04_ValleyOfTheKings") {
      _FixTexturesValleyOfTheKings();
    } else if (/* strModName=="" && */ strLevelName=="07_Dunes") {
      _FixTexturesDunes();
    } else if (/* strModName=="" && */ strLevelName=="08_Suburbs") {
      _FixTexturesSuburbs();
    } else if (/* strModName=="" && */ strLevelName=="10_Metropolis") {
      _FixTexturesMetropolis();
    } else if (/* strModName=="" && */ strLevelName=="11_AlleyOfSphinxes") {
      _FixTexturesAlleyOfSphinxes();
    } else if (/* strModName=="" && */ strLevelName=="12_Karnak") {
      _FixTexturesKarnak();
    } else if (/* strModName=="" && */ strLevelName=="13_Luxor") {
      _FixTexturesLuxor();
    } else if (/* strModName=="" && */ strLevelName=="14_SacredYards") {
      _FixTexturesSacredYards();
    } else if (/* strModName=="" && */ strLevelName=="KarnakDemo") {
      _FixTexturesKarnakDemo();
    } else if (/* strModName=="" && */ strLevelName=="Intro") {
      _FixTexturesIntro();
    }
  }

  // if not in network
  if (!_pNetwork->IsNetworkEnabled()) {
    // don't show last lines on screen after exiting console
    CON_DiscardLastLineTimes();
  }
}
//***************************************************************
//***************************************************************
//***************************************************************

class CEnableUserBreak {
public:
  BOOL bOld;
  CEnableUserBreak();
  ~CEnableUserBreak();
};

CEnableUserBreak::CEnableUserBreak() {
  bOld = _bUserBreakEnabled;
  _bUserBreakEnabled = TRUE;
}
CEnableUserBreak::~CEnableUserBreak() {
  _bUserBreakEnabled = bOld;
}

// wrapper function for dump and printout of extensive demo profile report
static void DumpDemoProfile(void)
{
  CTString strFragment, strAnalyzed;
  dem_iProfileRate = Clamp( dem_iProfileRate, (INDEX)0, (INDEX)60);
  strFragment = _pGame->DemoReportFragmentsProfile( dem_iProfileRate);
  strAnalyzed = _pGame->DemoReportAnalyzedProfile();
  try {
  // create file
    CTFileStream strm;
    CTString strFileName = CTString( "temp\\DemoProfile.lst");
    strm.Create_t( strFileName, CTStream::CM_TEXT);
    // dump results
    strm.FPrintF_t( strFragment);
    strm.FPrintF_t( strAnalyzed);
    // done!
    CPrintF( TRANS("Demo profile data dumped to '%s'.\n"), (const char *) strFileName);
  } 
  catch (const char *strError) {
    // something went wrong :(
    CPrintF( TRANS("Cannot dump demo profile data: %s\n"), (const char *) strError);
  }
}


static void ReportDemoProfile(void)
{
  CTString strFragment, strAnalyzed;
  dem_iProfileRate = Clamp( dem_iProfileRate, (INDEX)0, (INDEX)60);
  strFragment = _pGame->DemoReportFragmentsProfile( dem_iProfileRate);
  strAnalyzed = _pGame->DemoReportAnalyzedProfile();
  CPrintF("%s", (const char *) strFragment);
  CPrintF("%s", (const char *) strAnalyzed);
  CPrintF( "-\n");
}

#define MAX_SCRIPTSOUNDS 16
static CSoundObject *_apsoScriptChannels[MAX_SCRIPTSOUNDS] = {0};

static void PlayScriptSound(INDEX iChannel, const CTString &strSound, FLOAT fVolume, FLOAT fPitch, BOOL bLooping)
{
  if (iChannel<0 || iChannel>=MAX_SCRIPTSOUNDS) {
    return;
  }
  if (_apsoScriptChannels[iChannel]==NULL) {
    _apsoScriptChannels[iChannel] = new CSoundObject;
  }
  _apsoScriptChannels[iChannel]->SetVolume(fVolume, fVolume);
  _apsoScriptChannels[iChannel]->SetPitch(fPitch);
  try {
    _apsoScriptChannels[iChannel]->Play_t(strSound, SOF_NONGAME|(bLooping?SOF_LOOP:0));
  } catch (const char *strError) {
    CPrintF("%s\n", (const char *)strError);
  }
}
#if 0 // DG: unused.
static void PlayScriptSoundCfunc(void* pArgs)
{
  INDEX iChannel = NEXTARGUMENT(INDEX);
  CTString strSound = *NEXTARGUMENT(CTString*);
  FLOAT fVolume = NEXTARGUMENT(FLOAT);
  FLOAT fPitch = NEXTARGUMENT(FLOAT);
  BOOL bLooping = NEXTARGUMENT(INDEX);
  PlayScriptSound(iChannel, strSound, fVolume, fPitch, bLooping);
}
#endif // 0 (unused)
static void StopScriptSound(void* pArgs)
{
  INDEX iChannel = NEXTARGUMENT(INDEX);
  if (iChannel<0 || iChannel>=MAX_SCRIPTSOUNDS||_apsoScriptChannels[iChannel]==NULL) {
    return;
  }
  _apsoScriptChannels[iChannel]->Stop();
} 
static INDEX IsScriptSoundPlaying(INDEX iChannel)
{
  if (iChannel<0 || iChannel>=MAX_SCRIPTSOUNDS||_apsoScriptChannels[iChannel]==NULL) {
    return 0;
  }
  return _apsoScriptChannels[iChannel]->IsPlaying();
}

// Dump recorded profiling stats to file.
static void DumpProfileToFile(void)
{
  _bDumpNextTime = TRUE;
}

// Dump recorded profiling stats to console.
static void DumpProfileToConsole(void)
{
  CPutString(_strProfile);
}

// Record profiling stats.
static void RecordProfile(void)
{
  _bStartProfilingNextTime = TRUE;
}

// screen shot saving feature in console
static BOOL  bSaveScreenShot = FALSE;
static INDEX dem_iAnimFrame  = -1;
static void SaveScreenShot(void)
{
  bSaveScreenShot=TRUE;
}

static void Say(void* pArgs)
{
  CTString strText = *NEXTARGUMENT(CTString*);
  _pNetwork->SendChat(-1, -1, strText);
}
static void SayFromTo(void* pArgs)
{
  INDEX ulFrom = NEXTARGUMENT(INDEX);
  INDEX ulTo = NEXTARGUMENT(INDEX);
  CTString strText = *NEXTARGUMENT(CTString*);
  _pNetwork->SendChat(ulFrom, ulTo, strText);
}


// create name for a new screenshot
static CTFileName MakeScreenShotName(void)
{
  // create base name from the world name
  CTFileName fnmBase = CTString("ScreenShots\\")+_pNetwork->GetCurrentWorld().FileName();

  // start at counter of zero
  INDEX iShot = 0;
  // repeat forever
  FOREVER {
    // create number for the file
    CTString strNumber;
    strNumber.PrintF("_shot%04d", iShot);
    // create the full filename
    CTFileName fnmFullTGA = fnmBase+strNumber+".tga";
    CTFileName fnmFullJPG = fnmBase+strNumber+".jpg";
    // if the file doesn't exist
    if (!FileExistsForWriting(fnmFullTGA) && !FileExistsForWriting(fnmFullJPG)) {
      // that is the right filename
      return fnmFullTGA;
    }
    // if it exists, increment the number and retry
    iShot++;
  }
}


CButtonAction::CButtonAction(void)
{
  ba_iFirstKey  = KID_NONE;
  ba_iSecondKey = KID_NONE;
  ba_bFirstKeyDown = FALSE;
  ba_bSecondKeyDown = FALSE;
}

// Assignment operator.
CButtonAction &CButtonAction ::operator=(const CButtonAction &baOriginal)
{
  ba_iFirstKey                  = baOriginal.ba_iFirstKey;
  ba_iSecondKey                 = baOriginal.ba_iSecondKey;
  ba_strName                    = baOriginal.ba_strName;
  ba_strCommandLineWhenPressed  = baOriginal.ba_strCommandLineWhenPressed;
  ba_strCommandLineWhenReleased = baOriginal.ba_strCommandLineWhenReleased;
  ba_bFirstKeyDown = FALSE;
  ba_bSecondKeyDown = FALSE;

  return *this;
}

void CButtonAction::Read_t( CTStream &istrm)
{
  istrm>>ba_iFirstKey;
  istrm>>ba_iSecondKey;
  istrm>>ba_strName;
  istrm>>ba_strCommandLineWhenPressed;
  istrm>>ba_strCommandLineWhenReleased;
}

void CButtonAction::Write_t( CTStream &ostrm)
{
  ostrm<<ba_iFirstKey;
  ostrm<<ba_iSecondKey;
  ostrm<<ba_strName;
  ostrm<<ba_strCommandLineWhenPressed;
  ostrm<<ba_strCommandLineWhenReleased;
}

void CControls::DoButtonActions(void)
{
  // for all button actions
  FOREACHINLIST( CButtonAction, ba_lnNode, ctrl_lhButtonActions, itButtonAction)
  {
    // test if first button is pressed
    BOOL bFirstPressed = _pInput->GetButtonState( itButtonAction->ba_iFirstKey);
    // if it was just pressed
    if (bFirstPressed && !itButtonAction->ba_bFirstKeyDown) {
      // call pressed command
      _pShell->Execute(itButtonAction->ba_strCommandLineWhenPressed);
    // if it was just released
    } else if (!bFirstPressed && itButtonAction->ba_bFirstKeyDown) {
      // call released command
      _pShell->Execute(itButtonAction->ba_strCommandLineWhenReleased);
    }
    // remember pressed state
    itButtonAction->ba_bFirstKeyDown = bFirstPressed;

    // test if second button is pressed
    BOOL bSecondPressed = _pInput->GetButtonState( itButtonAction->ba_iSecondKey);
    // if it was just pressed
    if (bSecondPressed && !itButtonAction->ba_bSecondKeyDown) {
      // call pressed command
      _pShell->Execute(itButtonAction->ba_strCommandLineWhenPressed);
    // if it was just released
    } else if (!bSecondPressed && itButtonAction->ba_bSecondKeyDown) {
      // call released command
      _pShell->Execute(itButtonAction->ba_strCommandLineWhenReleased);
    }
    // remember pressed state
    itButtonAction->ba_bSecondKeyDown = bSecondPressed;
  }
}

// get current reading of an axis
FLOAT CControls::GetAxisValue(INDEX iAxis)
{
  CAxisAction &aa = ctrl_aaAxisActions[iAxis];

  FLOAT fReading = 0.0f;

  if (aa.aa_iAxisAction!=AXIS_NONE) {
    // get the reading
    fReading = _pInput->GetAxisValue(aa.aa_iAxisAction);

    // smooth the reading if needed
    if ( ctrl_bSmoothAxes || aa.aa_bSmooth) {
      FLOAT fSmoothed = (aa.aa_fLastReading+fReading)/2.0f;
      aa.aa_fLastReading = fReading;
      fReading = fSmoothed;
    }

    // integrate to get the absolute reading
    aa.aa_fAbsolute+=fReading;

    // get relative or absolute reading
    if (!aa.aa_bRelativeControler) {
      fReading = aa.aa_fAbsolute;
    }
  }

  // compensate for the deadzone
  if (aa.aa_fDeadZone>0) {
    FLOAT fDeadZone = aa.aa_fDeadZone/100.0f;
    if (fReading<-fDeadZone) {
      fReading = (fReading+fDeadZone)/(1-fDeadZone);
    } else if (fReading>fDeadZone) {
      fReading = (fReading-fDeadZone)/(1-fDeadZone);
    } else {
      fReading = 0.0f;
    }
  }

  // apply sensitivity and inversion
  return fReading*aa.aa_fAxisInfluence;
}

void CControls::CreateAction(const CPlayerCharacter &pc, CPlayerAction &paAction, BOOL bPreScan)
{
  // set axis-controlled moving
  paAction.pa_vTranslation(1) = -GetAxisValue( AXIS_MOVE_LR);
  paAction.pa_vTranslation(2) = GetAxisValue( AXIS_MOVE_UD);
  paAction.pa_vTranslation(3) = -GetAxisValue( AXIS_MOVE_FB);
  // set axis-controlled rotation
  paAction.pa_aRotation(1) = (ANGLE)-GetAxisValue( AXIS_TURN_LR);
  paAction.pa_aRotation(2) = (ANGLE)GetAxisValue( AXIS_TURN_UD);
  paAction.pa_aRotation(3) = (ANGLE)GetAxisValue( AXIS_TURN_BK);
  // set axis-controlled view rotation
  paAction.pa_aViewRotation(1) = (ANGLE)GetAxisValue( AXIS_LOOK_LR);
  paAction.pa_aViewRotation(2) = (ANGLE)GetAxisValue( AXIS_LOOK_UD);
  paAction.pa_aViewRotation(3) = (ANGLE)GetAxisValue( AXIS_LOOK_BK);

  // execute all button-action shell commands
  if (!bPreScan) {
    DoButtonActions();
  }
  //CPrintF("creating: prescan %d, x:%g\n", bPreScan, paAction.pa_aRotation(1));

  // make the player class create the action packet
  ctl_ComposeActionPacket(pc, paAction, bPreScan);
}

CButtonAction &CControls::AddButtonAction(void)
{
  // create a new action
  CButtonAction *pbaNew = new CButtonAction;
  // add it to end of list
  ctrl_lhButtonActions.AddTail(pbaNew->ba_lnNode);
  return *pbaNew;
}

void CControls::RemoveButtonAction( CButtonAction &baButtonAction)
{
  // remove from list
  baButtonAction.ba_lnNode.Remove();
  // free it
  delete &baButtonAction;
}

void CControls::DeleteAllButtonActions()
{
  FORDELETELIST(CButtonAction, ba_lnNode, this->ctrl_lhButtonActions, itAct) {
    delete &itAct.Current();
  }
}

// calculate some useful demo vars
static void CalcDemoProfile( INDEX ctFrames, INDEX &ctFramesNoPeaks,
            DOUBLE &dTimeSum, DOUBLE &dTimeSumNoPeaks, TIME &tmAverage, TIME &tmAverageNoPeaks,
            TIME &tmSigma, TIME &tmHighLimit, TIME &tmLowLimit, TIME &tmHighPeak, TIME &tmLowPeak,
            FLOAT &fAvgWTris, FLOAT &fAvgMTris, FLOAT &fAvgPTris, FLOAT &fAvgTTris,
            FLOAT &fAvgWTrisNoPeaks, FLOAT &fAvgMTrisNoPeaks, FLOAT &fAvgPTrisNoPeaks, FLOAT &fAvgTTrisNoPeaks)
{
  // calculate raw average
  INDEX i;
  TIME tmCurrent;
  dTimeSum = 0;
  DOUBLE dWTriSum=0, dMTriSum=0, dPTriSum=0, dTTriSum=0;
  DOUBLE dWTriSumNoPeaks=0, dMTriSumNoPeaks=0, dPTriSumNoPeaks=0, dTTriSumNoPeaks=0;
  for( i=0; i<ctFrames; i++) {
    dTimeSum += _atmFrameTimes[i];
    dWTriSum += _actTriangles[i*4 +0];  // world
    dMTriSum += _actTriangles[i*4 +1];  // model
    dPTriSum += _actTriangles[i*4 +2];  // particle
    dTTriSum += _actTriangles[i*4 +3];  // total
  }
  tmAverage = dTimeSum / ctFrames;
  fAvgWTris = dWTriSum / ctFrames;
  fAvgMTris = dMTriSum / ctFrames;
  fAvgPTris = dPTriSum / ctFrames;
  fAvgTTris = dTTriSum / ctFrames;

  // calc raw sigma and limits
  DOUBLE dSigmaSum=0;
  for( i=0; i<ctFrames; i++) {
    tmCurrent = _atmFrameTimes[i];
    TIME tmDelta = tmCurrent-tmAverage;
    dSigmaSum += tmDelta*tmDelta;
  }
  tmSigma = Sqrt(dSigmaSum/ctFrames);
  tmHighLimit = (tmAverage-tmSigma*2);
  tmLowLimit  = (tmAverage+tmSigma*2);

  // eliminate low peaks
  ctFramesNoPeaks = ctFrames;
  dTimeSumNoPeaks = dTimeSum;
  dWTriSumNoPeaks = dWTriSum;
  dMTriSumNoPeaks = dMTriSum;
  dPTriSumNoPeaks = dPTriSum;
  dTTriSumNoPeaks = dTTriSum;

  for( i=0; i<ctFrames; i++) {
    tmCurrent = _atmFrameTimes[i];
    if( tmHighLimit>tmCurrent || tmLowLimit<tmCurrent) {
      dTimeSumNoPeaks -= tmCurrent;
      dWTriSumNoPeaks -= _actTriangles[i*4 +0];
      dMTriSumNoPeaks -= _actTriangles[i*4 +1];
      dPTriSumNoPeaks -= _actTriangles[i*4 +2];
      dTTriSumNoPeaks -= _actTriangles[i*4 +3];
      ctFramesNoPeaks--;
    } 
  }

  // calculate peaks, new averages and sigma (without peaks)
  tmAverageNoPeaks = dTimeSumNoPeaks / ctFramesNoPeaks;
  fAvgWTrisNoPeaks = dWTriSumNoPeaks / ctFramesNoPeaks;
  fAvgMTrisNoPeaks = dMTriSumNoPeaks / ctFramesNoPeaks;
  fAvgPTrisNoPeaks = dPTriSumNoPeaks / ctFramesNoPeaks;
  fAvgTTrisNoPeaks = dTTriSumNoPeaks / ctFramesNoPeaks;

  dSigmaSum=0;
  tmHighPeak=99999, tmLowPeak=0;
  for( i=0; i<ctFrames; i++) {
    tmCurrent = _atmFrameTimes[i];
    if( tmHighLimit>tmCurrent || tmLowLimit<tmCurrent) continue;
    TIME tmDelta = tmCurrent-tmAverageNoPeaks;
    dSigmaSum += tmDelta*tmDelta;
    if( tmHighPeak > tmCurrent) tmHighPeak = tmCurrent;
    if( tmLowPeak  < tmCurrent) tmLowPeak  = tmCurrent;
  }
  tmSigma = Sqrt( dSigmaSum/ctFramesNoPeaks);
}


// dump demo profile to file 
CTString CGame::DemoReportFragmentsProfile( INDEX iRate)
{
  CTString strRes="";
  CTString strTmp;
  INDEX ctFrames = _atmFrameTimes.Count();

  // if report is not required
  if( dem_iProfileRate==0) {
    strRes.PrintF( TRANS("\nFragments report disabled.\n"));
    return strRes;
  }

  // if not enough frames
  if( ctFrames<20) {
    strRes.PrintF( TRANS("\nNot enough recorded frames to make fragments report.\n"));
    return strRes;
  }

  // enough frames - calc almost everything
  strRes.PrintF( TRANS("\nDemo performance results (fragment time = %d seconds):\n"), dem_iProfileRate);
  strTmp.PrintF(         "------------------------------------------------------\n\n");
  strRes += strTmp;
  DOUBLE dTimeSum, dTimeSumNoPeaks;
  INDEX  ctFramesNoPeaks;
  TIME   tmAverage, tmAverageNoPeaks;
  TIME   tmSigma, tmHighLimit, tmLowLimit, tmHighPeak, tmLowPeak;
  FLOAT  fAvgWTris, fAvgMTris, fAvgPTris, fAvgTTris;
  FLOAT  fAvgWTrisNoPeaks, fAvgMTrisNoPeaks, fAvgPTrisNoPeaks, fAvgTTrisNoPeaks;
  CalcDemoProfile( ctFrames, ctFramesNoPeaks, dTimeSum, dTimeSumNoPeaks, tmAverage, tmAverageNoPeaks,
                   tmSigma, tmHighLimit, tmLowLimit, tmHighPeak, tmLowPeak,
                   fAvgWTris, fAvgMTris, fAvgPTris, fAvgTTris,
                   fAvgWTrisNoPeaks, fAvgMTrisNoPeaks, fAvgPTrisNoPeaks, fAvgTTrisNoPeaks);
  strTmp.PrintF( TRANS("   #   average FPS     average FPS (W/O peaks)\n"));
  strRes += strTmp;
  // loop thru frames and create output of time fragmens
  dTimeSum = 0;
  dTimeSumNoPeaks = 0;
  ctFramesNoPeaks = 0;
  FLOAT fFrameCounter = 0;
  FLOAT fFrameCounterNoPeaks = 0;
  TIME  tmRate = dem_iProfileRate;
  INDEX iFragment=1;
  for( INDEX i=0; i<ctFrames; i++)
  { // get current frame time and calc sums
    TIME tmCurrent = _atmFrameTimes[i];
    dTimeSum += tmCurrent;
    fFrameCounter++;
    if( tmHighLimit<=tmCurrent && tmLowLimit>=tmCurrent) {
      dTimeSumNoPeaks += tmCurrent;
      fFrameCounterNoPeaks++;
    }
    // enough data for one time fragment
    if( dTimeSum>=tmRate) {
      FLOAT fTimeOver  = dTimeSum - tmRate;
      FLOAT fFrameOver = fTimeOver/tmCurrent;
      FLOAT fFragmentAverage = tmRate / (fFrameCounter-fFrameOver);
      FLOAT fFragmentNoPeaks = (tmRate-(dTimeSum-dTimeSumNoPeaks)) / (fFrameCounterNoPeaks-fFrameOver);
      strTmp.PrintF( "%4d    %6.1f           %6.1f", iFragment, 1.0f/fFragmentAverage, 1.0f/fFragmentNoPeaks);
      strRes += strTmp;
      INDEX iFragmentAverage10 = FloatToInt(5.0f/fFragmentAverage);
      INDEX iFragmentNoPeaks10 = FloatToInt(5.0f/fFragmentNoPeaks);
      if( iFragmentAverage10 != iFragmentNoPeaks10) strTmp.PrintF( "    !\n");
      else strTmp.PrintF( "\n");
      strRes += strTmp;
      // restart time and frames
      dTimeSum = fTimeOver;
      dTimeSumNoPeaks = fTimeOver;
      fFrameCounter = fFrameOver;
      fFrameCounterNoPeaks = fFrameOver;
      iFragment++;
    }
  }

  // all done
  return strRes;
}


// printout extensive demo profile report
CTString CGame::DemoReportAnalyzedProfile(void)
{
  CTString strRes="";
  INDEX ctFrames = _atmFrameTimes.Count();
  // nothing kept?
  if( ctFrames<20) {
    strRes.PrintF( TRANS("\nNot enough recorded frames to analyze.\n"));
    return strRes;
  }

  // calc almost everything
  DOUBLE dTimeSum, dTimeSumNoPeaks;
  INDEX  ctFramesNoPeaks;
  TIME   tmAverage, tmAverageNoPeaks;
  TIME   tmSigma, tmHighLimit, tmLowLimit, tmHighPeak, tmLowPeak;
  FLOAT  fAvgWTris, fAvgMTris, fAvgPTris, fAvgTTris;
  FLOAT  fAvgWTrisNoPeaks, fAvgMTrisNoPeaks, fAvgPTrisNoPeaks, fAvgTTrisNoPeaks;
  CalcDemoProfile( ctFrames, ctFramesNoPeaks, dTimeSum, dTimeSumNoPeaks, tmAverage, tmAverageNoPeaks,
                   tmSigma, tmHighLimit, tmLowLimit, tmHighPeak, tmLowPeak,
                   fAvgWTris, fAvgMTris, fAvgPTris, fAvgTTris,
                   fAvgWTrisNoPeaks, fAvgMTrisNoPeaks, fAvgPTrisNoPeaks, fAvgTTrisNoPeaks);
  // calc sustains
  DOUBLE dHighSum=0, dLowSum=0;
  DOUBLE dCurrentHighSum=0, dCurrentLowSum=0;
  INDEX  ctHighFrames=0, ctLowFrames=0;
  INDEX  ctCurrentHighFrames=0, ctCurrentLowFrames=0;
  for( INDEX i=0; i<ctFrames; i++)
  { // skip low peaks
    TIME tmCurrent = _atmFrameTimes[i];
    if( tmHighLimit>tmCurrent || tmLowLimit<tmCurrent)  continue;

    // high?
    if( (tmAverageNoPeaks-tmSigma) > tmCurrent) {
      // keep high sustain
      dCurrentHighSum += tmCurrent;
      ctCurrentHighFrames++;
    } else {
      // new high sustain found?
      if( ctHighFrames < ctCurrentHighFrames) {
        ctHighFrames = ctCurrentHighFrames;
        dHighSum     = dCurrentHighSum;
      }
      // reset high sustain
      ctCurrentHighFrames = 0;
      dCurrentHighSum     = 0;
    } 
    // low?
    if( (tmAverageNoPeaks+tmSigma) < tmCurrent) {
      // keep low sustain
      dCurrentLowSum += tmCurrent;
      ctCurrentLowFrames++;
    } else {
      // new low sustain found?
      if( ctLowFrames < ctCurrentLowFrames) {
        ctLowFrames = ctCurrentLowFrames;
        dLowSum     = dCurrentLowSum;
      }
      // reset low sustain
      ctCurrentLowFrames = 0;
      dCurrentLowSum     = 0;
    } 
  }
  // and results are ...
  TIME tmHighSustained = ctHighFrames / dHighSum;
  TIME tmLowSustained  = ctLowFrames  / dLowSum; 

  // printout
  CTString strTmp;
  strTmp.PrintF( TRANS("\n%.1f KB used for demo profile:\n"), 1+ ctFrames*5*sizeof(FLOAT)/1024.0f);
  strRes += strTmp;
  strTmp.PrintF( TRANS("    Originally recorded: %d frames in %.1f seconds => %5.1f FPS average.\n"),
                 ctFrames, dTimeSum, 1.0f/tmAverage);
  strRes += strTmp;
  strTmp.PrintF( TRANS("Without excessive peaks: %d frames in %.1f seconds => %5.1f FPS average.\n"),
                 ctFramesNoPeaks, dTimeSumNoPeaks, 1.0f/tmAverageNoPeaks);
  strRes += strTmp;
  strTmp.PrintF( TRANS("       High peak: %5.1f FPS\n"), 1.0f/tmHighPeak);
  strRes += strTmp;
  strTmp.PrintF( TRANS("        Low peak: %5.1f FPS\n"), 1.0f/tmLowPeak);
  strRes += strTmp;
  // enough values recorder for high sustain?
  if( ctHighFrames > (ctFrames/1024+5)) {
    strTmp.PrintF( TRANS("  High sustained: %5.1f FPS (%d frames in %.1f seconds)\n"),
                   tmHighSustained, ctHighFrames, dHighSum);
    strRes += strTmp;
  }
  // enough values recorder for low sustain?
  if( ctLowFrames > (ctFrames/1024+5)) {
    strTmp.PrintF( TRANS("   Low sustained: %5.1f FPS (%d frames in %.1f seconds)\n"),
                   tmLowSustained,  ctLowFrames,  dLowSum);
    strRes += strTmp;
  }

  // do triangle profile output (hidden - maybe not so wise idea)
  if( dem_bProfile==217) {
    const FLOAT fAvgRTris = fAvgTTris - (fAvgWTris+fAvgMTris+fAvgPTris);
    const FLOAT fAvgRTrisNoPeaks = fAvgTTrisNoPeaks - (fAvgWTrisNoPeaks+fAvgMTrisNoPeaks+fAvgPTrisNoPeaks);
    strTmp.PrintF( TRANS("Triangles per frame (with and without excessive peaks):\n")); strRes += "\n"+strTmp;
    strTmp.PrintF( TRANS("      World: %7.1f / %.1f\n"), fAvgWTris, fAvgWTrisNoPeaks);  strRes += strTmp;
    strTmp.PrintF( TRANS("      Model: %7.1f / %.1f\n"), fAvgMTris, fAvgMTrisNoPeaks);  strRes += strTmp;
    strTmp.PrintF( TRANS("   Particle: %7.1f / %.1f\n"), fAvgPTris, fAvgPTrisNoPeaks);  strRes += strTmp;
    strTmp.PrintF( TRANS("  rest (2D): %7.1f / %.1f\n"), fAvgRTris, fAvgRTrisNoPeaks);  strRes += strTmp;
    strRes +=            "           --------------------\n";
    strTmp.PrintF( TRANS("      TOTAL: %7.1f / %.1f\n"), fAvgTTris, fAvgTTrisNoPeaks);  strRes += strTmp;
  }

  // all done
  return strRes;
}



/* This is called every TickQuantum seconds. */
void CGameTimerHandler::HandleTimer(void)
{
  // call game's timer routine
  _pGame->GameHandleTimer();
}


void CGame::GameHandleTimer(void)
{
  // if direct input is active
  if( _pInput->IsInputEnabled() && !gm_bMenuOn)
  {

    // check if any active control uses joystick
    BOOL bAnyJoy = _ctrlCommonControls.UsesJoystick();
    for( INDEX iPlayer=0; iPlayer<4; iPlayer++) {
      if( gm_lpLocalPlayers[ iPlayer].lp_pplsPlayerSource != NULL) {
        INDEX iCurrentPlayer = gm_lpLocalPlayers[ iPlayer].lp_iPlayer;
        CControls &ctrls = gm_actrlControls[ iCurrentPlayer];
        if (ctrls.UsesJoystick()) {
          bAnyJoy = TRUE;
          break;
        }
      }
    }
    _pInput->SetJoyPolling(bAnyJoy);

    // read input devices
    _pInput->GetInput(FALSE);

    // if game is currently active, and not paused
    if (gm_bGameOn && !_pNetwork->IsPaused() && !_pNetwork->GetLocalPause())
    {
      // for all possible local players
      for( INDEX iPlayer=0; iPlayer<4; iPlayer++)
      {
        // if this player exist
        if( gm_lpLocalPlayers[ iPlayer].lp_pplsPlayerSource != NULL)
        {
          // publish player index to console
          ctl_iCurrentPlayerLocal = iPlayer;
          ctl_iCurrentPlayer = gm_lpLocalPlayers[ iPlayer].lp_pplsPlayerSource->pls_Index;

          // copy its local controls to current controls
          memcpy(
            ctl_pvPlayerControls,
            gm_lpLocalPlayers[ iPlayer].lp_ubPlayerControlsState,
            ctl_slPlayerControlsSize);

          // create action for it for this tick
          CPlayerAction paAction;
          INDEX iCurrentPlayer = gm_lpLocalPlayers[ iPlayer].lp_iPlayer;
          CControls &ctrls = gm_actrlControls[ iCurrentPlayer];
          ctrls.CreateAction(gm_apcPlayers[iCurrentPlayer], paAction, FALSE);
          // set the action in the client source object
          gm_lpLocalPlayers[ iPlayer].lp_pplsPlayerSource->SetAction(paAction);

          // copy the local controls back
          memcpy(
            gm_lpLocalPlayers[ iPlayer].lp_ubPlayerControlsState,
            ctl_pvPlayerControls,
            ctl_slPlayerControlsSize);
        }
      }
      // clear player indices
      ctl_iCurrentPlayerLocal = -1;
      ctl_iCurrentPlayer = -1;
    }
    // execute all button-action shell commands for common controls
    if (gm_bGameOn) {
      _ctrlCommonControls.DoButtonActions();
    }
  }
  // if DirectInput is disabled, and game is currently active
  else if (gm_bGameOn)
  {
    // for all possible local players
    for( INDEX iPlayer=0; iPlayer<4; iPlayer++)
    { // if this player exist
      if( gm_lpLocalPlayers[iPlayer].lp_pplsPlayerSource != NULL)
      { 
        CPlayerSource &pls = *gm_lpLocalPlayers[iPlayer].lp_pplsPlayerSource;
        // create dummy action for the player for this tick
        CPlayerAction paClearAction;
        // clear actions
        paClearAction = pls.pls_paAction;
        paClearAction.pa_vTranslation  = FLOAT3D(0.0f,0.0f,0.0f);
//        paClearAction.pa_aRotation     = ANGLE3D(0,0,0);
//        paClearAction.pa_aViewRotation = ANGLE3D(0,0,0);
        paClearAction.pa_ulButtons     = 0;

        // clear the action in the client source object
        pls.SetAction(paClearAction);
      }
    }
  }
}

/*
 * Global game object (in our case Flesh) initialization function
 */
void CGame::InitInternal( void)
{
  gam_strCustomLevel = ""; // filename of custom level chosen
  gam_strSessionName = TRANS("Unnamed session"); // name of multiplayer network session
  gam_strJoinAddress = TRANS("serveraddress");   // join address

  gm_MenuSplitScreenCfg    = SSC_PLAY1;
  gm_StartSplitScreenCfg   = SSC_PLAY1;
  gm_CurrentSplitScreenCfg = SSC_PLAY1;
  gm_iLastSetHighScore = 0;
  gm_iSinglePlayer = 0;
  gm_iWEDSinglePlayer = 0;
  gm_bGameOn = FALSE;
  gm_bMenuOn = FALSE;
  gm_bFirstLoading = FALSE;
  gm_bProfileDemo = FALSE;
  gm_slPlayerControlsSize = 0;
  gm_pvGlobalPlayerControls = NULL;
  memset(gm_aiMenuLocalPlayers, 0, sizeof(gm_aiMenuLocalPlayers));
  memset(gm_aiStartLocalPlayers, 0, sizeof(gm_aiStartLocalPlayers));

  // first assign translated to make dependcy catcher extract the translations
  gm_astrAxisNames[AXIS_MOVE_UD] = TRANS("move u/d");      
  gm_astrAxisNames[AXIS_MOVE_LR] = TRANS("move l/r");      
  gm_astrAxisNames[AXIS_MOVE_FB] = TRANS("move f/b");      
  gm_astrAxisNames[AXIS_TURN_UD] = TRANS("look u/d");      
  gm_astrAxisNames[AXIS_TURN_LR] = TRANS("turn l/r");      
  gm_astrAxisNames[AXIS_TURN_BK] = TRANS("banking");       
  gm_astrAxisNames[AXIS_LOOK_UD] = TRANS("view u/d");      
  gm_astrAxisNames[AXIS_LOOK_LR] = TRANS("view l/r");      
  gm_astrAxisNames[AXIS_LOOK_BK] = TRANS("view banking");  
  // but we must not really use the translation for loading
  gm_astrAxisNames[AXIS_MOVE_UD] = "move u/d";     // 
  gm_astrAxisNames[AXIS_MOVE_LR] = "move l/r";     // 
  gm_astrAxisNames[AXIS_MOVE_FB] = "move f/b";     // 
  gm_astrAxisNames[AXIS_TURN_UD] = "look u/d";     // 
  gm_astrAxisNames[AXIS_TURN_LR] = "turn l/r";     // 
  gm_astrAxisNames[AXIS_TURN_BK] = "banking";      // 
  gm_astrAxisNames[AXIS_LOOK_UD] = "view u/d";     // 
  gm_astrAxisNames[AXIS_LOOK_LR] = "view l/r";     // 
  gm_astrAxisNames[AXIS_LOOK_BK] = "view banking"; // 

  gm_csConsoleState = CS_OFF;
  gm_csComputerState = CS_OFF;

  gm_bGameOn = FALSE;
  gm_bMenuOn = FALSE;
  gm_bFirstLoading = FALSE;
  gm_aiMenuLocalPlayers[0] =  0;
  gm_aiMenuLocalPlayers[1] = -1;
  gm_aiMenuLocalPlayers[2] = -1;
  gm_aiMenuLocalPlayers[3] = -1;

  gm_MenuSplitScreenCfg = SSC_PLAY1;

  LoadPlayersAndControls();

  gm_iWEDSinglePlayer = 0;
  gm_iSinglePlayer = 0;

  // Big Font
  extern INDEX cmp_bBigFont;
  _pShell->DeclareSymbol("persistent user INDEX gam_bBigComputerFont;", &cmp_bBigFont);
  // add game timer handler
  _pTimer->AddHandler(&m_gthGameTimerHandler);
  // add shell variables
  _pShell->DeclareSymbol("user void RecordProfile(void);",        (void *)&RecordProfile);
  _pShell->DeclareSymbol("user void SaveScreenShot(void);",       (void *)&SaveScreenShot);
  _pShell->DeclareSymbol("user void DumpProfileToConsole(void);", (void *)&DumpProfileToConsole);
  _pShell->DeclareSymbol("user void DumpProfileToFile(void);",    (void *)&DumpProfileToFile);
  _pShell->DeclareSymbol("user INDEX hud_iStats;", (void *)&hud_iStats);
  _pShell->DeclareSymbol("user INDEX hud_bShowResolution;", (void *)&hud_bShowResolution);
  _pShell->DeclareSymbol("persistent user INDEX hud_bShowTime;",  (void *)&hud_bShowTime);
  _pShell->DeclareSymbol("persistent user INDEX hud_bShowClock;", (void *)&hud_bShowClock);
  _pShell->DeclareSymbol("user INDEX dem_bOnScreenDisplay;", (void *)&dem_bOSD);
  _pShell->DeclareSymbol("user INDEX dem_bPlay;",            (void *)&dem_bPlay);
  _pShell->DeclareSymbol("user INDEX dem_bPlayByName;",      (void *)&dem_bPlayByName);
  _pShell->DeclareSymbol("user INDEX dem_bProfile;",         (void *)&dem_bProfile);
  _pShell->DeclareSymbol("user INDEX dem_iAnimFrame;",       (void *)&dem_iAnimFrame);
  _pShell->DeclareSymbol("user CTString dem_strPostExec;",   (void *)&dem_strPostExec);
  _pShell->DeclareSymbol("persistent user INDEX dem_iProfileRate;",  (void *)&dem_iProfileRate);
  _pShell->DeclareSymbol("persistent user INDEX hud_bShowNetGraph;", (void *)&hud_bShowNetGraph);
  _pShell->DeclareSymbol("FLOAT gam_afEnemyMovementSpeed[5];", (void *)&gam_afEnemyMovementSpeed);
  _pShell->DeclareSymbol("FLOAT gam_afEnemyAttackSpeed[5];",   (void *)&gam_afEnemyAttackSpeed);
  _pShell->DeclareSymbol("FLOAT gam_afDamageStrength[5];",     (void *)&gam_afDamageStrength);
  _pShell->DeclareSymbol("FLOAT gam_afAmmoQuantity[5];",       (void *)&gam_afAmmoQuantity);
  _pShell->DeclareSymbol("persistent user FLOAT gam_fManaTransferFactor;", (void *)&gam_fManaTransferFactor);
  _pShell->DeclareSymbol("persistent user FLOAT gam_fExtraEnemyStrength         ;", (void *)&gam_fExtraEnemyStrength          );
  _pShell->DeclareSymbol("persistent user FLOAT gam_fExtraEnemyStrengthPerPlayer;", (void *)&gam_fExtraEnemyStrengthPerPlayer );
  _pShell->DeclareSymbol("persistent user INDEX gam_iInitialMana;",        (void *)&gam_iInitialMana);
  _pShell->DeclareSymbol("persistent user INDEX gam_iScoreLimit;",  (void *)&gam_iScoreLimit);
  _pShell->DeclareSymbol("persistent user INDEX gam_iFragLimit;",   (void *)&gam_iFragLimit);
  _pShell->DeclareSymbol("persistent user INDEX gam_iTimeLimit;",   (void *)&gam_iTimeLimit);
  _pShell->DeclareSymbol("persistent user INDEX gam_ctMaxPlayers;", (void *)&gam_ctMaxPlayers);
  _pShell->DeclareSymbol("persistent user INDEX gam_bWaitAllPlayers;", (void *)&gam_bWaitAllPlayers);
  _pShell->DeclareSymbol("persistent user INDEX gam_bFriendlyFire;",   (void *)&gam_bFriendlyFire);
  _pShell->DeclareSymbol("persistent user INDEX gam_bPlayEntireGame;", (void *)&gam_bPlayEntireGame);
  _pShell->DeclareSymbol("persistent user INDEX gam_bWeaponsStay;",    (void *)&gam_bWeaponsStay);

  _pShell->DeclareSymbol("persistent user INDEX gam_bAmmoStays       ;", (void *)&gam_bAmmoStays       );
  _pShell->DeclareSymbol("persistent user INDEX gam_bHealthArmorStays;", (void *)&gam_bHealthArmorStays);
  _pShell->DeclareSymbol("persistent user INDEX gam_bAllowHealth     ;", (void *)&gam_bAllowHealth     );
  _pShell->DeclareSymbol("persistent user INDEX gam_bAllowArmor      ;", (void *)&gam_bAllowArmor      );
  _pShell->DeclareSymbol("persistent user INDEX gam_bInfiniteAmmo    ;", (void *)&gam_bInfiniteAmmo    );
  _pShell->DeclareSymbol("persistent user INDEX gam_bRespawnInPlace  ;", (void *)&gam_bRespawnInPlace  );

  _pShell->DeclareSymbol("persistent user INDEX gam_iCredits;", (void *)&gam_iCredits);
  _pShell->DeclareSymbol("persistent user FLOAT gam_tmSpawnInvulnerability;", (void *)&gam_tmSpawnInvulnerability);

  _pShell->DeclareSymbol("persistent user INDEX gam_iBlood;", (void *)&gam_iBlood);
  _pShell->DeclareSymbol("persistent user INDEX gam_bGibs;",  (void *)&gam_bGibs);

  _pShell->DeclareSymbol("persistent user INDEX gam_bUseExtraEnemies;",  (void *)&gam_bUseExtraEnemies);

  _pShell->DeclareSymbol("user INDEX gam_bQuickLoad;", (void *)&gam_bQuickLoad);
  _pShell->DeclareSymbol("user INDEX gam_bQuickSave;", (void *)&gam_bQuickSave);
  _pShell->DeclareSymbol("user INDEX gam_iQuickSaveSlots;", (void *)&gam_iQuickSaveSlots);
  _pShell->DeclareSymbol("user INDEX gam_iQuickStartDifficulty;", (void *)&gam_iQuickStartDifficulty);
  _pShell->DeclareSymbol("user INDEX gam_iQuickStartMode;",       (void *)&gam_iQuickStartMode);
  _pShell->DeclareSymbol("user INDEX gam_bQuickStartMP;",       (void *)&gam_bQuickStartMP);
  _pShell->DeclareSymbol("persistent user INDEX gam_iStartDifficulty;", (void *)&gam_iStartDifficulty);
  _pShell->DeclareSymbol("persistent user INDEX gam_iStartMode;",       (void *)&gam_iStartMode);
  _pShell->DeclareSymbol("persistent user CTString gam_strGameAgentExtras;", (void *)&gam_strGameAgentExtras);
  _pShell->DeclareSymbol("persistent user CTString gam_strCustomLevel;", (void *)&gam_strCustomLevel);
  _pShell->DeclareSymbol("persistent user CTString gam_strSessionName;", (void *)&gam_strSessionName);
  _pShell->DeclareSymbol("persistent user CTString gam_strJoinAddress;", (void *)&gam_strJoinAddress);
  _pShell->DeclareSymbol("persistent user INDEX gam_bEnableAdvancedObserving;", (void *)&gam_bEnableAdvancedObserving);
  _pShell->DeclareSymbol("user INDEX gam_iObserverConfig;", (void *)&gam_iObserverConfig);
  _pShell->DeclareSymbol("user INDEX gam_iObserverOffset;", (void *)&gam_iObserverOffset);

  _pShell->DeclareSymbol("INDEX gam_iRecordHighScore;", (void *)&gam_iRecordHighScore);

  _pShell->DeclareSymbol("persistent user FLOAT con_fHeightFactor;", (void *)&con_fHeightFactor);
  _pShell->DeclareSymbol("persistent user FLOAT con_tmLastLines;",   (void *)&con_tmLastLines);
  _pShell->DeclareSymbol("user INDEX con_bTalk;", (void *)&con_bTalk);
  _pShell->DeclareSymbol("user void ReportDemoProfile(void);", (void *)&ReportDemoProfile);
  _pShell->DeclareSymbol("user void DumpDemoProfile(void);",   (void *)&DumpDemoProfile);
  extern CTString GetGameAgentRulesInfo(void);
  extern CTString GetGameTypeName(INDEX);
  extern CTString GetGameTypeNameCfunc(void* pArgs);
  extern CTString GetCurrentGameTypeName(void);
  extern ULONG GetSpawnFlagsForGameType(INDEX);
  extern ULONG GetSpawnFlagsForGameTypeCfunc(void* pArgs);
  extern BOOL IsMenuEnabled_(const CTString &);
  extern BOOL IsMenuEnabledCfunc(void* pArgs);
  _pShell->DeclareSymbol("user CTString GetGameAgentRulesInfo(void);",   (void *)&GetGameAgentRulesInfo);
  _pShell->DeclareSymbol("user CTString GetGameTypeName(INDEX);",        (void *)&GetGameTypeNameCfunc);
  _pShell->DeclareSymbol("user CTString GetCurrentGameTypeName(void);",  (void *)&GetCurrentGameTypeName);
  _pShell->DeclareSymbol("user INDEX GetSpawnFlagsForGameType(INDEX);",  (void *)&GetSpawnFlagsForGameTypeCfunc);
  _pShell->DeclareSymbol("user INDEX IsMenuEnabled(CTString);",          (void *)&IsMenuEnabledCfunc);
  _pShell->DeclareSymbol("user void Say(CTString);",                     (void *)&Say);
  _pShell->DeclareSymbol("user void SayFromTo(INDEX, INDEX, CTString);", (void *)&SayFromTo);

  _pShell->DeclareSymbol("CTString GetGameTypeNameSS(INDEX);",           (void *)&GetGameTypeName);
  _pShell->DeclareSymbol("INDEX GetSpawnFlagsForGameTypeSS(INDEX);",     (void *)&GetSpawnFlagsForGameType);
  _pShell->DeclareSymbol("INDEX IsMenuEnabledSS(CTString);",             (void *)&IsMenuEnabled_);

  _pShell->DeclareSymbol("user const INDEX ctl_iCurrentPlayerLocal;", (void *)&ctl_iCurrentPlayerLocal);
  _pShell->DeclareSymbol("user const INDEX ctl_iCurrentPlayer;",      (void *)&ctl_iCurrentPlayer);

  _pShell->DeclareSymbol("user FLOAT gam_fChatSoundVolume;",      (void *)&gam_fChatSoundVolume);

  _pShell->DeclareSymbol("user void PlaySound(INDEX, CTString, FLOAT, FLOAT, INDEX);", (void *)&PlayScriptSound);
  _pShell->DeclareSymbol("user void StopSound(INDEX);", (void *)&StopScriptSound);
  _pShell->DeclareSymbol("user INDEX IsSoundPlaying(INDEX);", (void *)&IsScriptSoundPlaying);

  // Fix illuminations bug metod:
  // 0 - none
  // 1 - fix textrure settings fix
  // 2 - create additional lighting (better) 
  _pShell->DeclareSymbol("persistent user INDEX gam_bFixIlluminationsMetod;", (void *)&gam_bFixIlluminationsMetod);

  CAM_Init();

  // load persistent symbols
  if (!_bDedicatedServer) {
    _pShell->Execute(CTString("include \"")+fnmPersistentSymbols+"\";");
  }
  // execute the startup script
  _pShell->Execute(CTString("include \"")+fnmStartupScript+"\";");

  // check the size and pointer of player control variables that are local to each player
  if (ctl_slPlayerControlsSize <= 0
    || static_cast<ULONG>(ctl_slPlayerControlsSize) > sizeof(((CLocalPlayer*)NULL)->lp_ubPlayerControlsState)
    || ctl_pvPlayerControls == NULL) {
    FatalError(TRANS("Current player controls are invalid."));
  }

  // load common controls
  try {
    _ctrlCommonControls.Load_t(fnmCommonControls);
  } catch (const char * /*strError*/) {
    //FatalError(TRANS("Cannot load common controls: %s\n"), strError);
  }

  // init LCD textures/fonts
  LCDInit();

  // load console history
  CTString strConsole;
  try {
    strConsole.LoadKeepCRLF_t(fnmConsoleHistory);
    gam_strConsoleInputBuffer = strConsole;
  } catch (const char *strError) {
    (void)strError; // must ignore if there is no history file
  }

  // load game shell settings
  try {
    Load_t();
  } catch (const char *strError) {
    CPrintF(TRANSV("Cannot load game settings:\n%s\n  Using defaults\n"), (const char *)strError);
  }

  CON_DiscardLastLineTimes();

  // provide URL to the engine
  _strModURL = "http://www.croteam.com/mods/TheSecondEncounter";
}

// internal cleanup
void CGame::EndInternal(void)
{
  // stop game if eventually started
  StopGame();
  // remove game timer handler
  _pTimer->RemHandler( &m_gthGameTimerHandler);
  // save persistent symbols
  if (!_bDedicatedServer) {
    _pShell->StorePersistentSymbols(fnmPersistentSymbols);
  }

  LCDEnd();

  // stop and delete any playing sound
  #define MAX_SCRIPTSOUNDS 16
  for( INDEX i=0; i<MAX_SCRIPTSOUNDS; i++) {
    if( _apsoScriptChannels[i]==NULL) continue;
    _apsoScriptChannels[i]->Stop();
    delete _apsoScriptChannels[i];
  }

  // save console history
  CTString strConsole = gam_strConsoleInputBuffer;
  strConsole.TrimLeft(8192);
  try {
    strConsole.SaveKeepCRLF_t(fnmConsoleHistory);
  } catch (const char *strError) {
    WarningMessage(TRANS("Cannot save console history:\n%s"), strError);
  }
  SavePlayersAndControls();

  // save game shell settings
  try {
    Save_t();
  } catch (const char *strError) {
    WarningMessage("Cannot load game settings:\n%s\nUsing defaults!", strError);
  }
}

BOOL CGame::NewGame(const CTString &strSessionName, const CTFileName &fnWorld,
   CSessionProperties &sp)
{
  gam_iObserverConfig = 0;
  gam_iObserverOffset = 0;
  // stop eventually running game
  StopGame();

  CEnableUserBreak eub;
  if (!gm_bFirstLoading) {
    _bUserBreakEnabled = FALSE;
  }

  // try to start current network provider
  if( !StartProviderFromName()) {
    return FALSE;
    gm_bFirstLoading = FALSE;
  }

  // clear profile array and start game
  _atmFrameTimes.Clear();
  _actTriangles.Clear();
  gm_bProfileDemo = FALSE;

  // start the new session
  try {
    if( dem_bPlay) {
      gm_aiStartLocalPlayers[0] = -2;

      CTFileName fnmDemo = CTString("Temp\\Play.dem");
      if( dem_bPlayByName) {
        fnmDemo = fnWorld;
      }
      CAM_Start(fnmDemo);
      _pNetwork->StartDemoPlay_t(fnmDemo);
    } else {
      BOOL bWaitAllPlayers = sp.sp_bWaitAllPlayers && _pNetwork->IsNetworkEnabled();
      _pNetwork->StartPeerToPeer_t( strSessionName, fnWorld, 
        sp.sp_ulSpawnFlags, sp.sp_ctMaxPlayers, bWaitAllPlayers, &sp);
    }
  } catch (const char *strError) {
    gm_bFirstLoading = FALSE;
    // stop network provider
    _pNetwork->StopProvider();
    // and display error
    CPrintF(TRANSV("Cannot start game:\n%s\n"), (const char *)strError);
    return FALSE;
  }

  // setup players from given indices
  SetupLocalPlayers();

  if( !dem_bPlay && !AddPlayers()) {
    _pNetwork->StopGame();
    _pNetwork->StopProvider();
    gm_bFirstLoading = FALSE;
    return FALSE;
  }
  gm_bFirstLoading = FALSE;
  gm_bGameOn = TRUE;
  gm_CurrentSplitScreenCfg = gm_StartSplitScreenCfg;
  // clear last set highscore
  gm_iLastSetHighScore = -1;

  MaybeDiscardLastLines();
  return TRUE;
}

BOOL CGame::JoinGame(const CNetworkSession &session)
{
  CEnableUserBreak eub;
  gam_iObserverConfig = 0;
  gam_iObserverOffset = 0;

  // stop eventually running game
  StopGame();

  // try to start current network provider
  if( !StartProviderFromName()) return FALSE;

  // start the new session
  try {
    INDEX ctLocalPlayers = 0;
    if (gm_StartSplitScreenCfg>=SSC_PLAY1 && gm_StartSplitScreenCfg<=SSC_PLAY4) {
      ctLocalPlayers = (gm_StartSplitScreenCfg-SSC_PLAY1)+1;
    }
    _pNetwork->JoinSession_t(session, ctLocalPlayers);
  } catch (const char *strError) {
    // stop network provider
    _pNetwork->StopProvider();
    // and display error
    CPrintF(TRANSV("Cannot join game:\n%s\n"), (const char *)strError);
    return FALSE;
  }

  // setup players from given indices
  SetupLocalPlayers();

  if( !AddPlayers())
  {
    _pNetwork->StopGame();
    _pNetwork->StopProvider();
    return FALSE;
  }
  gm_bGameOn = TRUE;
  gm_CurrentSplitScreenCfg = gm_StartSplitScreenCfg;
  return TRUE;
}

BOOL CGame::LoadGame(const CTFileName &fnGame)
{
  gam_iObserverConfig = 0;
  gam_iObserverOffset = 0;

  // stop eventually running game
  StopGame();

  // try to start current network provider
  if( !StartProviderFromName()) return FALSE;

  // start the new session
  try {
    _pNetwork->Load_t( fnGame);
    CPrintF(TRANSV("Loaded game: %s\n"), (const char *) fnGame);
  } catch (const char *strError) {
    // stop network provider
    _pNetwork->StopProvider();
    // and display error
    CPrintF(TRANSV("Cannot load game: %s\n"), (const char *)strError);
    return FALSE;
  }

  // setup players from given indices
  SetupLocalPlayers();

  if( !AddPlayers())
  {
    _pNetwork->StopGame();
    _pNetwork->StopProvider();
    return FALSE;
  }
  gm_bGameOn = TRUE;
  gm_CurrentSplitScreenCfg = gm_StartSplitScreenCfg;
  // clear last set highscore
  gm_iLastSetHighScore = -1;

  // if it was a quicksave, and not the newest one
  if (fnGame.Matches("*\\QuickSave*") && fnGame!=GetQuickSaveName(FALSE)) {
    // mark that it should be resaved as newest
    gam_bQuickSave = TRUE;
  }

  MaybeDiscardLastLines();
  return TRUE;
}

BOOL CGame::StartDemoPlay(const CTFileName &fnDemo)
{
  CEnableUserBreak eub;

  // stop eventually running game
  StopGame();

  // try to start current network provider
  if( !StartProviderFromName()) {
    gm_bFirstLoading = FALSE;
    return FALSE;
  }

  // start the new session
  try {
    _pNetwork->StartDemoPlay_t( fnDemo);
    CPrintF(TRANSV("Started playing demo: %s\n"), (const char *) fnDemo);
  } catch (const char *strError) {
    // stop network provider
    _pNetwork->StopProvider();
    // and display error
    CPrintF(TRANSV("Cannot play demo: %s\n"), (const char *)strError);
    gm_bFirstLoading = FALSE;
    return FALSE;
  }
  gm_bFirstLoading = FALSE;

  // setup players from given indices
  gm_StartSplitScreenCfg = CGame::SSC_OBSERVER;
  SetupLocalPlayers();
  gm_bGameOn = TRUE;
  gm_CurrentSplitScreenCfg = gm_StartSplitScreenCfg;

  // prepare array for eventual profiling
  _atmFrameTimes.PopAll();
  _actTriangles.PopAll();
  gm_bProfileDemo = FALSE;
  if( dem_bProfile) gm_bProfileDemo = TRUE;
  _tvDemoStarted = _pTimer->GetHighPrecisionTimer();
  _tvLastFrame   = _tvDemoStarted;

  CTFileName fnmScript = fnDemo.NoExt()+".ini";
  if (!FileExists(fnmScript)) {
    fnmScript = CTString("Demos\\Default.ini");
  }
  CTString strCmd;
  strCmd.PrintF("include \"%s\"", (const char*)fnmScript);
  _pShell->Execute(strCmd);

  MaybeDiscardLastLines();
  // all done
  return TRUE;
}


BOOL CGame::StartDemoRec(const CTFileName &fnDemo)
{
  // save demo recording
  try {
    _pNetwork->StartDemoRec_t( fnDemo);
    CPrintF(TRANSV("Started recording demo: %s\n"), (const char *) fnDemo);
    // save a thumbnail
    SaveThumbnail(fnDemo.NoExt()+"Tbn.tex");
    return TRUE;
  } catch (const char *strError) {
    // and display error
    CPrintF(TRANSV("Cannot start recording: %s\n"), (const char *)strError);
    return FALSE;
  }
}


void CGame::StopDemoRec(void)
{
  // if no game is currently running
  if (!gm_bGameOn) return;

  _pNetwork->StopDemoRec();
  CPrintF(TRANSV("Finished recording.\n"));
}

BOOL CGame::SaveGame(const CTFileName &fnGame)
{
  // if no live players
  INDEX ctPlayers = GetPlayersCount();
  INDEX ctLivePlayers = GetLivePlayersCount();
  if (ctPlayers>0 && ctLivePlayers<=0) {
    // display error
    CPrintF(TRANSV("Won't save game when dead!\n"));
    // do not save
    return FALSE;
  }

  // save new session
  try {
    _pNetwork->Save_t( fnGame);
    CPrintF(TRANSV("Saved game: %s\n"), (const char *) fnGame);
    SaveThumbnail(fnGame.NoExt()+"Tbn.tex");
    return TRUE;
  } catch (const char *strError) {
    // and display error
    CPrintF(TRANSV("Cannot save game: %s\n"), (const char *) strError);
    return FALSE;
  }
}

void CGame::StopGame(void)
{
  // disable computer quickly
  ComputerForceOff();

  // if no game is currently running
  if (!gm_bGameOn)
  {
    // do nothing
    return;
  }
  // stop eventual camera
  CAM_Stop();
  // disable direct input
//  _pInput->DisableInput();
  // and game
  gm_bGameOn = FALSE;
  // stop the game
  _pNetwork->StopGame();
  // stop the provider
  _pNetwork->StopProvider();
  // for all four local players
  for( INDEX iPlayer=0; iPlayer<4; iPlayer++)
  {
    // mark that current player does not exist any more
    gm_lpLocalPlayers[ iPlayer].lp_bActive = FALSE;
    gm_lpLocalPlayers[ iPlayer].lp_pplsPlayerSource = NULL;
  }
}

BOOL CGame::StartProviderFromName(void)
{
  BOOL bSuccess = FALSE;
  // list to contain available network providers
  CListHead lhAvailableProviders;
  // enumerate all providers
  _pNetwork->EnumNetworkProviders( lhAvailableProviders);

  // for each provider
  FOREACHINLIST(CNetworkProvider, np_Node, lhAvailableProviders, litProviders)
  {
    // generate provider description
    CTString strProviderName = litProviders->GetDescription();
    // is this the provider we are searching for ?
    if( strProviderName == gm_strNetworkProvider)
    {
      // it is, set it as active network provider
      gm_npNetworkProvider = litProviders.Current();
      bSuccess = TRUE;
      break;
    }
  }

  // delete list of providers
  FORDELETELIST(CNetworkProvider, np_Node, lhAvailableProviders, litDelete)
  {
    delete &litDelete.Current();
  }

  // try to
  try
  {
    // start selected network provider
    _pNetwork->StartProvider_t( gm_npNetworkProvider);
  }
  // catch throwed error
  catch (const char *strError)
  {
    // if unable, report error
    CPrintF( TRANS("Can't start provider:\n%s\n"), (const char *)strError);
    bSuccess = FALSE;
  }
  return bSuccess;
}

CHighScoreEntry::CHighScoreEntry(void)
{
  hse_strPlayer = "";
  hse_gdDifficulty = (CSessionProperties::GameDifficulty)-100;
  hse_tmTime = -1.0f;
  hse_ctKills = -1;
  hse_ctScore = 0;
}
SLONG CGame::PackHighScoreTable(void)
{
  // start at the beginning of buffer
  UBYTE *pub = _aubHighScoreBuffer;
  // for each entry
  for (INDEX i=0; i<HIGHSCORE_COUNT; i++) {
#ifdef PLATFORM_UNIX
    // make its string
    char str[MAX_HIGHSCORENAME+1];
    memset(str, 0, sizeof(str));
    strncpy(str, gm_ahseHighScores[i].hse_strPlayer, MAX_HIGHSCORENAME);
    // copy the value and the string
    memcpy(pub, str, sizeof(str));
    pub += MAX_HIGHSCORENAME+1;

    INDEX ival;
    FLOAT fval;

    ival = gm_ahseHighScores[i].hse_gdDifficulty;
    BYTESWAP(ival);
    memcpy(pub, &ival, sizeof(INDEX));
    pub += sizeof(INDEX);

    fval = gm_ahseHighScores[i].hse_tmTime;
    BYTESWAP(fval);
    memcpy(pub, &fval,       sizeof(FLOAT));
    pub += sizeof(FLOAT);

    ival = gm_ahseHighScores[i].hse_ctKills;
    BYTESWAP(ival);
    memcpy(pub, &ival,      sizeof(INDEX));
    pub += sizeof(INDEX);

    ival = gm_ahseHighScores[i].hse_ctScore;
    BYTESWAP(ival);
    memcpy(pub, &ival,      sizeof(INDEX));
    pub += sizeof(INDEX);
#else
	// make its string
	char str[MAX_HIGHSCORENAME + 1];
	memset(str, 0, sizeof(str));
	strncpy(str, gm_ahseHighScores[i].hse_strPlayer, MAX_HIGHSCORENAME);
	// copy the value and the string
	memcpy(pub, str, sizeof(str));
	pub += MAX_HIGHSCORENAME + 1;
	memcpy(pub, &gm_ahseHighScores[i].hse_gdDifficulty, sizeof(INDEX));
	pub += sizeof(INDEX);
	memcpy(pub, &gm_ahseHighScores[i].hse_tmTime, sizeof(FLOAT));
	pub += sizeof(FLOAT);
	memcpy(pub, &gm_ahseHighScores[i].hse_ctKills, sizeof(INDEX));
	pub += sizeof(INDEX);
	memcpy(pub, &gm_ahseHighScores[i].hse_ctScore, sizeof(INDEX));
	pub += sizeof(INDEX);
#endif
  }
  // just copy it for now
  memcpy(_aubHighScorePacked, _aubHighScoreBuffer, MAX_HIGHSCORETABLESIZE);
  return MAX_HIGHSCORETABLESIZE;
}

void CGame::UnpackHighScoreTable(SLONG slSize)
{
  // just copy it for now
  memcpy(_aubHighScoreBuffer, _aubHighScorePacked, slSize);
  // start at the beginning of buffer
  UBYTE *pub = _aubHighScoreBuffer;
  // for each entry
  for (INDEX i=0; i<HIGHSCORE_COUNT; i++) {
#ifdef PLATFORM_UNIX
    gm_ahseHighScores[i].hse_strPlayer = (const char*)pub;
    pub += MAX_HIGHSCORENAME+1;
    memcpy(&gm_ahseHighScores[i].hse_gdDifficulty, pub, sizeof(INDEX));
    BYTESWAP(gm_ahseHighScores[i].hse_gdDifficulty);
    pub += sizeof(INDEX);
    memcpy(&gm_ahseHighScores[i].hse_tmTime      , pub, sizeof(FLOAT));
    BYTESWAP(gm_ahseHighScores[i].hse_tmTime);
    pub += sizeof(FLOAT);
    memcpy(&gm_ahseHighScores[i].hse_ctKills     , pub, sizeof(INDEX));
    BYTESWAP(gm_ahseHighScores[i].hse_ctKills);
    pub += sizeof(INDEX);
    memcpy(&gm_ahseHighScores[i].hse_ctScore     , pub, sizeof(INDEX));
    BYTESWAP(gm_ahseHighScores[i].hse_ctScore);
    pub += sizeof(INDEX);
#else
	gm_ahseHighScores[i].hse_strPlayer = (const char*)pub;
	pub += MAX_HIGHSCORENAME + 1;
	memcpy(&gm_ahseHighScores[i].hse_gdDifficulty, pub, sizeof(INDEX));
	pub += sizeof(INDEX);
	memcpy(&gm_ahseHighScores[i].hse_tmTime, pub, sizeof(FLOAT));
	pub += sizeof(FLOAT);
	memcpy(&gm_ahseHighScores[i].hse_ctKills, pub, sizeof(INDEX));
	pub += sizeof(INDEX);
	memcpy(&gm_ahseHighScores[i].hse_ctScore, pub, sizeof(INDEX));
	pub += sizeof(INDEX);
#endif
  }

  // try to
  try {
    CTFileStream strm;
    strm.Open_t(CTString("table.txt"));
    {for (INDEX i=0; i<HIGHSCORE_COUNT; i++) {
      gm_ahseHighScores[i].hse_gdDifficulty = (CSessionProperties::GameDifficulty)-100;
    }}
    {for (INDEX i=0; i<HIGHSCORE_COUNT; i++) {
      CTString strLine;
      strm.GetLine_t(strLine);
      char strName[256];
      strLine.ScanF("\"%256[^\"]\" %d %g %d %d", 
        strName, 
        &gm_ahseHighScores[i].hse_gdDifficulty,
        &gm_ahseHighScores[i].hse_tmTime,
        &gm_ahseHighScores[i].hse_ctKills,
        &gm_ahseHighScores[i].hse_ctScore);
      gm_ahseHighScores[i].hse_strPlayer = strName;
    }}
  } catch (const char *strError) {
    (void)strError;
  }

  // remember best for player hud and statistics
  plr_iHiScore = gm_ahseHighScores[0].hse_ctScore;

  // no last set
  gm_iLastSetHighScore = -1;
}

/*
 * Loads CGame from file with file name given trough SetGameSettingsSaveFileName() function
 */
void CGame::Load_t( void)
{
  ASSERT( gm_fnSaveFileName != "");
  CTFileStream strmFile;
  // open file with saved CGameObject
  strmFile.Open_t( gm_fnSaveFileName,CTStream::OM_READ);
  // read file ID
  strmFile.ExpectID_t( CChunkID( "GAME"));  // game
  // check version number
  if( !( CChunkID(GAME_SHELL_VER) == strmFile.GetID_t()) )
  {
    throw( TRANS("Invalid version of game shell."));
  }
  // load all of the class members
  strmFile>>gm_strNetworkProvider;
  strmFile>>gm_iWEDSinglePlayer;
  strmFile>>gm_iSinglePlayer;
  strmFile>>gm_aiMenuLocalPlayers[0];
  strmFile>>gm_aiMenuLocalPlayers[1];
  strmFile>>gm_aiMenuLocalPlayers[2];
  strmFile>>gm_aiMenuLocalPlayers[3];

  strmFile>>(INDEX&)gm_MenuSplitScreenCfg;

  // read high-score table
  SLONG slHSSize;
  strmFile>>slHSSize;
  strmFile.Read_t(&_aubHighScorePacked, slHSSize);
  UnpackHighScoreTable(slHSSize);
}

/*
 * Saves current state of CGame under file name given trough SetGameSettingsSaveFileName() function
 */
void CGame::Save_t( void)
{
  ASSERT( gm_fnSaveFileName != "");
  CTFileStream strmFile;
  // create file to save CGameObject
  strmFile.Create_t( gm_fnSaveFileName);
  // write file ID
  strmFile.WriteID_t( CChunkID( "GAME"));  // game shell
  // write version number
  strmFile.WriteID_t( CChunkID(GAME_SHELL_VER));

  // save all of the class members
  strmFile<<gm_strNetworkProvider;
  strmFile<<gm_iWEDSinglePlayer;
  strmFile<<gm_iSinglePlayer;
  strmFile<<gm_aiMenuLocalPlayers[0];
  strmFile<<gm_aiMenuLocalPlayers[1];
  strmFile<<gm_aiMenuLocalPlayers[2];
  strmFile<<gm_aiMenuLocalPlayers[3];

  strmFile.Write_t( &gm_MenuSplitScreenCfg, sizeof( enum SplitScreenCfg));

  // write high-score table
  SLONG slHSSize = PackHighScoreTable();
  strmFile<<slHSSize;
  strmFile.Write_t(&_aubHighScorePacked, slHSSize);
}


void LoadControls(CControls &ctrl, INDEX i)
{
  try {
    CTFileName fnm;
    fnm.PrintF("Controls\\Controls%d.ctl", i);
    ctrl.Load_t(fnm);
  } catch (const char *strError) {
    (void) strError; 
    try {
      ctrl.Load_t(CTFILENAME("Controls\\00-Default.ctl"));
    } catch (const char *strError) {
      (void) strError; 
      ctrl.SwitchToDefaults();
    }
  }
}

void LoadPlayer(CPlayerCharacter &pc, INDEX i)
{
  try {
    CTFileName fnm;
    fnm.PrintF("Players\\Player%d.plr", i);
    pc.Load_t(fnm);
  } catch (const char *strError) {
    (void) strError;
    CTString strName;
    if (i==0) {
      LoadStringVar(CTString("Data\\Var\\DefaultPlayer.var"), strName);
      strName.OnlyFirstLine();
    }
    if (strName=="") {
      strName.PrintF("Player %d", i);
    }
    pc = CPlayerCharacter(strName);
  }
}


/*
 * Loads 8 players and 8 controls
 */
void CGame::LoadPlayersAndControls( void)
{
  for (INDEX iControls=0; iControls<8; iControls++) {
    LoadControls(gm_actrlControls[iControls], iControls);
  }

  for (INDEX iPlayer=0; iPlayer<8; iPlayer++) {
    LoadPlayer(gm_apcPlayers[iPlayer], iPlayer);
  }

  SavePlayersAndControls();
}

/*
 * Saves 8 players and 8 controls
 */
void CGame::SavePlayersAndControls( void)
{
  try
  {
    // save players
    gm_apcPlayers[0].Save_t( CTString( "Players\\Player0.plr"));
    gm_apcPlayers[1].Save_t( CTString( "Players\\Player1.plr"));
    gm_apcPlayers[2].Save_t( CTString( "Players\\Player2.plr"));
    gm_apcPlayers[3].Save_t( CTString( "Players\\Player3.plr"));
    gm_apcPlayers[4].Save_t( CTString( "Players\\Player4.plr"));
    gm_apcPlayers[5].Save_t( CTString( "Players\\Player5.plr"));
    gm_apcPlayers[6].Save_t( CTString( "Players\\Player6.plr"));
    gm_apcPlayers[7].Save_t( CTString( "Players\\Player7.plr"));
    // save controls
    gm_actrlControls[0].Save_t( CTString( "Controls\\Controls0.ctl"));
    gm_actrlControls[1].Save_t( CTString( "Controls\\Controls1.ctl"));
    gm_actrlControls[2].Save_t( CTString( "Controls\\Controls2.ctl"));
    gm_actrlControls[3].Save_t( CTString( "Controls\\Controls3.ctl"));
    gm_actrlControls[4].Save_t( CTString( "Controls\\Controls4.ctl"));
    gm_actrlControls[5].Save_t( CTString( "Controls\\Controls5.ctl"));
    gm_actrlControls[6].Save_t( CTString( "Controls\\Controls6.ctl"));
    gm_actrlControls[7].Save_t( CTString( "Controls\\Controls7.ctl"));
  }
  // catch throwed error
  catch (const char *strError)
  {
    (void) strError;
  }

  // skip checking of players if game isn't on
  if( !gm_bGameOn) return;

  // for each local player
  for( INDEX i=0; i<4; i++) {
    CLocalPlayer &lp = gm_lpLocalPlayers[i];
    // if active
    if( lp.lp_bActive && lp.lp_pplsPlayerSource!=NULL && lp.lp_iPlayer>=0 && lp.lp_iPlayer<8) {
      // if its character in game is different than the one in config
      CPlayerCharacter &pcInGame = lp.lp_pplsPlayerSource->pls_pcCharacter;
      CPlayerCharacter &pcConfig = gm_apcPlayers[lp.lp_iPlayer];
      if( pcConfig.pc_strName!=pcInGame.pc_strName
       || pcConfig.pc_strTeam!=pcInGame.pc_strTeam
       || memcmp(pcConfig.pc_aubAppearance, pcInGame.pc_aubAppearance, sizeof(pcInGame.pc_aubAppearance))!=0 ) {
        // demand change in game
        lp.lp_pplsPlayerSource->ChangeCharacter(pcConfig);
      }
    }
  }
}


void CGame::SetupLocalPlayers( void)
{
  // setup local players and their controls
  gm_lpLocalPlayers[0].lp_iPlayer = gm_aiStartLocalPlayers[0];
  gm_lpLocalPlayers[1].lp_iPlayer = gm_aiStartLocalPlayers[1];
  gm_lpLocalPlayers[2].lp_iPlayer = gm_aiStartLocalPlayers[2];
  gm_lpLocalPlayers[3].lp_iPlayer = gm_aiStartLocalPlayers[3];
  if (gm_StartSplitScreenCfg < CGame::SSC_PLAY1) {
    gm_lpLocalPlayers[0].lp_iPlayer = -1;
  }
  if (gm_StartSplitScreenCfg < CGame::SSC_PLAY2) {
    gm_lpLocalPlayers[1].lp_iPlayer = -1;
  }
  if (gm_StartSplitScreenCfg < CGame::SSC_PLAY3) {
    gm_lpLocalPlayers[2].lp_iPlayer = -1;
  }
  if (gm_StartSplitScreenCfg < CGame::SSC_PLAY4) {
    gm_lpLocalPlayers[3].lp_iPlayer = -1;
  }
}

BOOL CGame::AddPlayers(void)
{
  // add local player(s) into game
  try {
    for(INDEX i=0; i<4; i++) {
      CLocalPlayer &lp = gm_lpLocalPlayers[i];
      INDEX iPlayer = lp.lp_iPlayer;
      if (iPlayer>=0) {
        ASSERT(iPlayer>=0 && iPlayer<8);
        lp.lp_pplsPlayerSource = _pNetwork->AddPlayer_t(gm_apcPlayers[iPlayer]);
        lp.lp_bActive = TRUE;
      }
    }
  } catch (const char *strError) {
    CPrintF(TRANSV("Cannot add player:\n%s\n"), (const char *)strError);
    return FALSE;
  }

  return TRUE;
}

// save thumbnail for savegame
static CTFileName _fnmThumb=CTString("");
void CGame::SaveThumbnail( const CTFileName &fnm)
{
  // request saving of thumbnail only (need drawport)
  // (saving will take place in GameRedrawView())
  _fnmThumb = fnm;
}


// timer variables
#define FRAMES_AVERAGING_MAX 20L
static  CTimerValue _tvLasts[FRAMES_AVERAGING_MAX];
static  CTimerValue _tvDelta[FRAMES_AVERAGING_MAX];
static  INDEX _iCheckNow  = 0;
static  INDEX _iCheckMax  = 0;

// print resolution, frame rate or extensive profiling, and elapsed time
static void PrintStats( CDrawPort *pdpDrawPort)
{
  // cache some general vars
  SLONG slDPWidth  = pdpDrawPort->GetWidth();
  SLONG slDPHeight = pdpDrawPort->GetHeight();
  
  // determine proper text scale for statistics display
  FLOAT fTextScale = (FLOAT)slDPWidth/640.0f;
  
  // display resolution info (if needed)
  if( hud_bShowResolution) {
    CTString strRes;
    strRes.PrintF( "%dx%dx%s", slDPWidth, slDPHeight, (const char *) _pGfx->gl_dmCurrentDisplayMode.DepthString());
    pdpDrawPort->SetFont( _pfdDisplayFont);
    pdpDrawPort->SetTextScaling( fTextScale);
    pdpDrawPort->SetTextAspect( 1.0f);
    pdpDrawPort->PutTextC( strRes, slDPWidth*0.5f, slDPHeight*0.15f, C_WHITE|255);
  }

  // if required, print elapsed playing time
  if( hud_bShowTime) {
    // set font, spacing and scale
    pdpDrawPort->SetFont( _pfdDisplayFont);
    pdpDrawPort->SetTextScaling( fTextScale);
    pdpDrawPort->SetTextAspect( 1.0f);
    // calculate elapsed time
    CTimerValue tvNow = _pTimer->CurrentTick();
    ULONG ulTime = (ULONG)tvNow.GetSeconds();
    // printout elapsed time
    CTString strTime;
    if( ulTime >= (60*60)) {
      // print hours
      strTime.PrintF( "%02d:%02d:%02d", ulTime/(60*60), (ulTime/60)%60, ulTime%60);
    } else {
      // skip hours
      strTime.PrintF( "%2d:%02d", ulTime/60, ulTime%60);
    }
    pdpDrawPort->PutTextC( strTime, slDPWidth*0.5f, slDPHeight*0.06f, C_WHITE|CT_OPAQUE);
  }

  // if required, print real time
  if( hud_bShowClock) {
    // set font, spacing and scale
    pdpDrawPort->SetFont( _pfdConsoleFont);
    pdpDrawPort->SetTextScaling(1.0f);
    pdpDrawPort->SetTextAspect( 1.0f);
    // determine time
    struct tm *newtime;
    time_t long_time;
    time(&long_time);
    newtime = localtime(&long_time);
    // printout
    CTString strTime;
    strTime.PrintF( "%2d:%02d", newtime->tm_hour, newtime->tm_min);
    pdpDrawPort->PutTextR( strTime, slDPWidth-3, 2, C_lYELLOW|CT_OPAQUE);
  }

  // if required, draw netgraph
  if( hud_bShowNetGraph)
  { // determine and clamp size
    INDEX ctLines = _pNetwork->ga_angeNetGraph.Count();
    ctLines = ClampUp( ctLines, (INDEX)(slDPHeight*0.7f));
    FLOAT f192oLines = 192.0f / (FLOAT)ctLines;
    const FLOAT fMaxWidth = slDPWidth  *0.1f;
    const PIX pixJ = (PIX) (slDPHeight *0.9f);
    // draw canvas
    #ifdef FIRST_ENCOUNTER  // First Encounter
    pdpDrawPort->Fill(       slDPWidth-1-fMaxWidth, pixJ-ctLines+1, fMaxWidth,   ctLines,   C_vdGREEN|64);    
    #else // Second Encounter
    pdpDrawPort->Fill(       slDPWidth-1-fMaxWidth, pixJ-ctLines+1, fMaxWidth,   ctLines,   SE_COL_BLUE_DARK_HV|64);    
    #endif
    // pdpDrawPort->Fill(       slDPWidth-1-fMaxWidth, pixJ-ctLines+1, fMaxWidth,   ctLines,   SE_COL_BLUE_DARK_HV|64);
    pdpDrawPort->DrawBorder( slDPWidth-2-fMaxWidth, pixJ-ctLines,   fMaxWidth+2, ctLines+2, C_WHITE  |192);
    // draw graph
    for( INDEX i=0; i<ctLines; i++) {
      FLOAT fValue  = _pNetwork->ga_angeNetGraph[i].nge_fLatency;
      enum NetGraphEntryType nge = _pNetwork->ga_angeNetGraph[i].nge_ngetType;
      FLOAT fWidth = Clamp( fValue, 0.0f, 1.0f)*fMaxWidth;
      COLOR colLine = C_GREEN;                 
           if( nge==NGET_ACTION)            colLine = C_GREEN;  // normal action (default)
      else if( nge==NGET_MISSING)           colLine = C_RED;    // missing sequence
      else if( nge==NGET_NONACTION)         colLine = C_WHITE;  // non-action sequence
      else if( nge==NGET_REPLICATEDACTION)  colLine = C_BLUE;   // duplicated action
      else if( nge==NGET_SKIPPEDACTION)     colLine = C_YELLOW; // skipped action
      else                 colLine = C_BLACK;  // unknown ???
      ULONG ulAlpha = FloatToInt( ((FLOAT)ctLines-(i*0.3333f)) *f192oLines);
      pdpDrawPort->DrawLine( slDPWidth-2, pixJ-i, slDPWidth-2-fWidth, pixJ-i, colLine|ulAlpha);
    }
  }

  // if stats aren't required
  hud_iStats = Clamp( hud_iStats, (INDEX)0, (INDEX)2);
  if( hud_iStats==0 || (hud_iEnableStats==0 && hud_fEnableFPS==0)) {
    // display nothing
    _iCheckNow = 0;
    _iCheckMax = 0;
#ifdef PLATFORM_UNIX
    STAT_Enable(FALSE);
#endif
    return;
  }

  // calculate FPS
  FLOAT fFPS = 0.0f;
  _iCheckMax++;
  if( _iCheckMax >= FRAMES_AVERAGING_MAX) {
    for( INDEX i=0; i<FRAMES_AVERAGING_MAX; i++) fFPS += _tvDelta[i].GetSeconds();
    fFPS = FRAMES_AVERAGING_MAX*FRAMES_AVERAGING_MAX / fFPS;
    _iCheckMax = FRAMES_AVERAGING_MAX;
  }

  // determine newest time
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  _tvDelta[_iCheckNow] = tvNow - _tvLasts[_iCheckNow];
  _tvLasts[_iCheckNow] = tvNow;
  _iCheckNow = (_iCheckNow+1) % FRAMES_AVERAGING_MAX;

  // set display interface (proportional) font
  pdpDrawPort->SetFont( _pfdDisplayFont);
  pdpDrawPort->SetTextAspect( 1.0f);
  pdpDrawPort->SetTextScaling( fTextScale);

		// display colored FPS 
		COLOR colFPS = C_RED;
		if( fFPS >= 20) colFPS = C_GREEN;
		if( fFPS >= 60) colFPS = C_WHITE;

		// prepare FPS string for printing
		CTString strFPS = "?";
				 if( fFPS >= 20)   strFPS.PrintF( "%4.1f", fFPS);
		else if( fFPS >= 0.1f) strFPS.PrintF( "%4.1f", fFPS);

		// printout FPS number
        if(!_pShell->GetINDEX("hud_bLegacyHUD")) {
		  pdpDrawPort->PutTextR( strFPS, slDPWidth*0.643f, slDPHeight*0.97f, colFPS|245);
        } else {
          if( hud_fEnableFPS) pdpDrawPort->PutTextC( strFPS, slDPWidth*0.75f, slDPHeight*0.005f, colFPS|192);
        }
		// set display interface (proportional) font
        if(!_pShell->GetINDEX("hud_bLegacyHUD")) {
		  pdpDrawPort->SetFont( _pfdDisplayFont);
		  pdpDrawPort->SetTextAspect( 1.0f);
		  pdpDrawPort->SetTextScaling( 0.6f*fTextScale);
		  strFPS.PrintF("FPS\n");
		  pdpDrawPort->PutTextR( strFPS, slDPWidth*0.67f, slDPHeight*0.979f, colFPS|245);
       }

  // if in extensive stats mode
  if( hud_iStats==2 && hud_iEnableStats)
  { // display extensive statistics
    CTString strReport;
#ifdef PLATFORM_UNIX
	STAT_Enable(TRUE);
#endif
    STAT_Report(strReport);
    STAT_Reset();

    // adjust and set font
    pdpDrawPort->SetFont( _pfdConsoleFont);
    pdpDrawPort->SetTextScaling( 1.0f);
    pdpDrawPort->SetTextLineSpacing( -1);

    // put filter
    pdpDrawPort->Fill( 0,0, 128,slDPHeight, C_BLACK|128, C_BLACK|0, C_BLACK|128, C_BLACK|0);
    // printout statistics
    strFPS.PrintF( " frame =%3.0f ms\n---------------\n", 1000.0f/fFPS);
    pdpDrawPort->PutText( strFPS,    0, 40, C_WHITE|CT_OPAQUE);
    pdpDrawPort->PutText( strReport, 4, 65, C_GREEN|CT_OPAQUE);
  }
  else {
#ifdef PLATFORM_UNIX
	  STAT_Enable(FALSE);
#endif 
  }
}


// max possible drawports
CDrawPort adpDrawPorts[7];
// and ptrs to them
CDrawPort *apdpDrawPorts[7];

INDEX iFirstObserver = 0;

static void MakeSplitDrawports(enum CGame::SplitScreenCfg ssc, INDEX iCount, CDrawPort *pdp)
{
  apdpDrawPorts[0] = NULL;
  apdpDrawPorts[1] = NULL;
  apdpDrawPorts[2] = NULL;
  apdpDrawPorts[3] = NULL;
  apdpDrawPorts[4] = NULL;
  apdpDrawPorts[5] = NULL;
  apdpDrawPorts[6] = NULL;

  // if observer
  if (ssc==CGame::SSC_OBSERVER) {
    // must have at least one screen
    iCount = Clamp(iCount, (INDEX)1, (INDEX)4);
    // starting at first drawport
    iFirstObserver = 0;
  }

  // if one player or observer with one screen
  if (ssc==CGame::SSC_PLAY1 || (ssc==CGame::SSC_OBSERVER && iCount==1)) {
    // the only drawport covers entire screen
    adpDrawPorts[0] = CDrawPort( pdp, 0.0, 0.0, 1.0, 1.0);
    apdpDrawPorts[0] = &adpDrawPorts[0];
  // if two players or observer with two screens
  } else if (ssc==CGame::SSC_PLAY2 || (ssc==CGame::SSC_OBSERVER && iCount==2)) {
    // if the drawport is not dualhead
    if (!pdp->IsDualHead()) {
      // need two drawports for filling the empty spaces left and right
      CDrawPort dpL( pdp, 0.0, 0.0, 0.2, 1.0f);
      CDrawPort dpR( pdp, 0.8, 0.0, 0.2, 1.0f);
      dpL.Lock();  dpL.Fill(C_BLACK|CT_OPAQUE);  dpL.Unlock();
      dpR.Lock();  dpR.Fill(C_BLACK|CT_OPAQUE);  dpR.Unlock();
      // first of two draw ports covers upper half of the screen
       adpDrawPorts[0] = CDrawPort( pdp, 0.1666, 0.0, 0.6668, 0.5);
      apdpDrawPorts[0] = &adpDrawPorts[0];
      // second draw port covers lower half of the screen
       adpDrawPorts[1] = CDrawPort( pdp, 0.1666, 0.5, 0.6668, 0.5);
      apdpDrawPorts[1] = &adpDrawPorts[1];
    // if the drawport is dualhead
    } else {
      // first of two draw ports covers left half of the screen
       adpDrawPorts[0] = CDrawPort( pdp, 0.0, 0.0, 0.5, 1.0);
      apdpDrawPorts[0] = &adpDrawPorts[0];
      // second draw port covers right half of the screen
       adpDrawPorts[1] = CDrawPort( pdp, 0.5, 0.0, 0.5, 1.0);
      apdpDrawPorts[1] = &adpDrawPorts[1];
    }
  // if three players or observer with three screens
  } else if (ssc==CGame::SSC_PLAY3 || (ssc==CGame::SSC_OBSERVER && iCount==3)) {
    // if the drawport is not dualhead
    if (!pdp->IsDualHead()) {
      // need two drawports for filling the empty spaces left and right
      CDrawPort dpL( pdp, 0.0, 0.0, 0.2, 0.5f);
      CDrawPort dpR( pdp, 0.8, 0.0, 0.2, 0.5f);
      dpL.Lock();  dpL.Fill(C_BLACK|CT_OPAQUE);  dpL.Unlock();
      dpR.Lock();  dpR.Fill(C_BLACK|CT_OPAQUE);  dpR.Unlock();
      // first of three draw ports covers center upper half of the screen
       adpDrawPorts[0] = CDrawPort( pdp, 0.1666, 0.0, 0.6667, 0.5);
      apdpDrawPorts[0] = &adpDrawPorts[0];
      // second draw port covers lower-left part of the screen
       adpDrawPorts[1] = CDrawPort( pdp, 0.0, 0.5, 0.5, 0.5);
      apdpDrawPorts[1] = &adpDrawPorts[1];
      // third draw port covers lower-right part of the screen
       adpDrawPorts[2] = CDrawPort( pdp, 0.5, 0.5, 0.5, 0.5);
      apdpDrawPorts[2] = &adpDrawPorts[2];
    // if the drawport is dualhead
    } else {
      // first player uses entire left part
       adpDrawPorts[0] = CDrawPort( pdp, 0.0, 0.0, 0.5, 1.0);
      apdpDrawPorts[0] = &adpDrawPorts[0];
      // make right DH part
      CDrawPort dpDHR( pdp, 0.5, 0.0, 0.5, 1.0);
      // need two drawports for filling the empty spaces left and right on the right DH part
      CDrawPort dpL( &dpDHR, 0.0, 0.0, 0.2, 1.0f);
      CDrawPort dpR( &dpDHR, 0.8, 0.0, 0.2, 1.0f);
      dpL.Lock();  dpL.Fill(C_BLACK|CT_OPAQUE);  dpL.Unlock();
      dpR.Lock();  dpR.Fill(C_BLACK|CT_OPAQUE);  dpR.Unlock();
      // second draw port covers upper half of the right screen
       adpDrawPorts[1] = CDrawPort( &dpDHR, 0.1666, 0.0, 0.6667, 0.5);
      apdpDrawPorts[1] = &adpDrawPorts[1];
      // third draw port covers lower half of the right screen
       adpDrawPorts[2] = CDrawPort( &dpDHR, 0.1666, 0.5, 0.6667, 0.5);
      apdpDrawPorts[2] = &adpDrawPorts[2];
    }
  // if four players or observer with four screens
  } else if (ssc==CGame::SSC_PLAY4 || (ssc==CGame::SSC_OBSERVER && iCount==4)) {
    // if the drawport is not dualhead
    if (!pdp->IsDualHead()) {
      // first of four draw ports covers upper-left part of the screen
       adpDrawPorts[0] = CDrawPort( pdp, 0.0, 0.0, 0.5, 0.5);
      apdpDrawPorts[0] = &adpDrawPorts[0];
      // second draw port covers upper-right part of the screen
       adpDrawPorts[1] = CDrawPort( pdp, 0.5, 0.0, 0.5, 0.5);
      apdpDrawPorts[1] = &adpDrawPorts[1];
      // third draw port covers lower-left part of the screen
       adpDrawPorts[2] = CDrawPort( pdp, 0.0, 0.5, 0.5, 0.5);
      apdpDrawPorts[2] = &adpDrawPorts[2];
      // fourth draw port covers lower-right part of the screen
       adpDrawPorts[3] = CDrawPort( pdp, 0.5, 0.5, 0.5, 0.5);
      apdpDrawPorts[3] = &adpDrawPorts[3];
    // if the drawport is dualhead
    } else {
      // make the DH parts
      CDrawPort dpDHL( pdp, 0.0, 0.0, 0.5, 1.0);
      CDrawPort dpDHR( pdp, 0.5, 0.0, 0.5, 1.0);
      // on the left part
      {
        // need two drawports for filling the empty spaces left and right
        CDrawPort dpL( &dpDHL, 0.0, 0.0, 0.2, 1.0f);
        CDrawPort dpR( &dpDHL, 0.8, 0.0, 0.2, 1.0f);
        dpL.Lock();  dpL.Fill(C_BLACK|CT_OPAQUE);  dpL.Unlock();
        dpR.Lock();  dpR.Fill(C_BLACK|CT_OPAQUE);  dpR.Unlock();
        // first of two draw ports covers upper half of the screen
         adpDrawPorts[0] = CDrawPort( &dpDHL, 0.1666, 0.0, 0.6667, 0.5);
        apdpDrawPorts[0] = &adpDrawPorts[0];
        // second draw port covers lower half of the screen
         adpDrawPorts[1] = CDrawPort( &dpDHL, 0.1666, 0.5, 0.6667, 0.5);
        apdpDrawPorts[1] = &adpDrawPorts[1];
      }
      // on the right part
      {
        // need two drawports for filling the empty spaces left and right
        CDrawPort dpL( &dpDHR, 0.0, 0.0, 0.2, 1.0f);
        CDrawPort dpR( &dpDHR, 0.8, 0.0, 0.2, 1.0f);
        dpL.Lock();  dpL.Fill(C_BLACK|CT_OPAQUE);  dpL.Unlock();
        dpR.Lock();  dpR.Fill(C_BLACK|CT_OPAQUE);  dpR.Unlock();
        // first of two draw ports covers upper half of the screen
         adpDrawPorts[2] = CDrawPort( &dpDHR, 0.1666, 0.0, 0.6667, 0.5);
        apdpDrawPorts[2] = &adpDrawPorts[2];
        // second draw port covers lower half of the screen
         adpDrawPorts[3] = CDrawPort( &dpDHR, 0.1666, 0.5, 0.6667, 0.5);
        apdpDrawPorts[3] = &adpDrawPorts[3];
      }
    }
  }

  // if observer
  if (ssc==CGame::SSC_OBSERVER) {
    // observing starts at first drawport
    iFirstObserver = 0;
  // if not observer
  } else {
    // observing starts after all players
    iFirstObserver = INDEX(ssc)+1;
  }

  // if not observer and using more than one screen
  if (ssc!=CGame::SSC_OBSERVER && iCount>=1) {
    // create extra small screens
    #define FREE (1/16.0)
    #define FULL (4/16.0)
    if (iCount==1) {
       adpDrawPorts[iFirstObserver+0] = CDrawPort( pdp, 1.0-FREE-FULL, FREE, FULL, FULL);
      apdpDrawPorts[iFirstObserver+0] = &adpDrawPorts[iFirstObserver+0];
    } else if (iCount==2) {
       adpDrawPorts[iFirstObserver+0] = CDrawPort( pdp, 1.0-FREE-FULL, FREE+0*(FULL+FREE), FULL, FULL);
      apdpDrawPorts[iFirstObserver+0] = &adpDrawPorts[iFirstObserver+0];
       adpDrawPorts[iFirstObserver+1] = CDrawPort( pdp, 1.0-FREE-FULL, FREE+1*(FULL+FREE), FULL, FULL);
      apdpDrawPorts[iFirstObserver+1] = &adpDrawPorts[iFirstObserver+1];
    } else if (iCount==3) {
       adpDrawPorts[iFirstObserver+0] = CDrawPort( pdp, 1.0-FREE-FULL, FREE+0*(FULL+FREE), FULL, FULL);
      apdpDrawPorts[iFirstObserver+0] = &adpDrawPorts[iFirstObserver+0];
       adpDrawPorts[iFirstObserver+1] = CDrawPort( pdp, 1.0-FREE-FULL, FREE+1*(FULL+FREE), FULL, FULL);
      apdpDrawPorts[iFirstObserver+1] = &adpDrawPorts[iFirstObserver+1];
       adpDrawPorts[iFirstObserver+2] = CDrawPort( pdp, 1.0-FREE-FULL, FREE+2*(FULL+FREE), FULL, FULL);
      apdpDrawPorts[iFirstObserver+2] = &adpDrawPorts[iFirstObserver+2];
    }
  }
}

// this is used to make sure that the thumbnail is never saved with an empty screen
static BOOL _bPlayerViewRendered = FALSE;

// redraw all game views (for case of split-screens and such)
void CGame::GameRedrawView( CDrawPort *pdpDrawPort, ULONG ulFlags)
{
  // if thumbnail saving has been required
  if( _fnmThumb!="")
  { // reset the need for saving thumbnail
    CTFileName fnm = _fnmThumb;
    _fnmThumb = CTString("");
    // render one game view to a small cloned drawport
    //PIX pixSizeJ = pdpDrawPort->GetHeight();
    PIXaabbox2D boxThumb( PIX2D(0,0), PIX2D(128,128));
    CDrawPort dpThumb( pdpDrawPort, boxThumb);
    _bPlayerViewRendered = FALSE;
    GameRedrawView( &dpThumb, 0);
    if (_bPlayerViewRendered) {
      // grab screenshot
      CImageInfo iiThumb;
      CTextureData tdThumb;
      dpThumb.GrabScreen( iiThumb);
      // try to save thumbnail
      try {
        CTFileStream strmThumb;
        tdThumb.Create_t( &iiThumb, 128, MAX_MEX_LOG2, FALSE);
        strmThumb.Create_t(fnm);
        tdThumb.Write_t( &strmThumb);
        strmThumb.Close();
      } catch (const char *strError) {
        // report an error to console, if failed
        CPrintF( "%s\n",(const char *) strError);
      }
    } else {
      _fnmThumb = fnm;
    }
  }

  if( ulFlags) {
    // pretouch memory if required (only if in game mode, not screengrabbing or profiling!)
    SE_PretouchIfNeeded();
  }

  // if game is started and computer isn't on
  //BOOL bClientJoined = FALSE;
  if( gm_bGameOn && (_pGame->gm_csComputerState==CS_OFF || pdpDrawPort->IsDualHead()) 
    && gm_CurrentSplitScreenCfg!=SSC_DEDICATED )
  {

    INDEX ctObservers = Clamp(gam_iObserverConfig, (INDEX)0, (INDEX)4);
    INDEX iObserverOffset = ClampDn(gam_iObserverOffset, (INDEX)0);
    if (gm_CurrentSplitScreenCfg==SSC_OBSERVER) {
      ctObservers = ClampDn(ctObservers, (INDEX)1);
    }
    if (gm_CurrentSplitScreenCfg!=SSC_OBSERVER) {
      if (!gam_bEnableAdvancedObserving || !GetSP()->sp_bCooperative) {
        ctObservers = 0;
      }
    }
    MakeSplitDrawports(gm_CurrentSplitScreenCfg, ctObservers, pdpDrawPort);

    // get number of local players
    INDEX ctLocals = 0;
    {for (INDEX i=0; i<4; i++) {
      if (gm_lpLocalPlayers[i].lp_pplsPlayerSource!=NULL) {
        ctLocals++;
      }
    }}

    CEntity *apenViewers[7];
    apenViewers[0] = NULL;
    apenViewers[1] = NULL;
    apenViewers[2] = NULL;
    apenViewers[3] = NULL;
    apenViewers[4] = NULL;
    apenViewers[5] = NULL;
    apenViewers[6] = NULL;
    INDEX ctViewers = 0;

    // check if input is enabled
    BOOL bDoPrescan = _pInput->IsInputEnabled() &&
      !_pNetwork->IsPaused() && !_pNetwork->GetLocalPause() &&
      _pShell->GetINDEX("inp_bAllowPrescan");
    // prescan input
    if (bDoPrescan) {
      _pInput->GetInput(TRUE);
    }
    // timer must not occur during prescanning
    { 
#if defined(PLATFORM_UNIX) && !defined(SINGLE_THREADED)
      //#warning "This seems to cause Race Condition, so disabled"
#else
      CTSingleLock csTimer(&_pTimer->tm_csHooks, TRUE);
#endif
    // for each local player
    for( INDEX i=0; i<4; i++) {
      // if local player
      CPlayerSource *ppls = gm_lpLocalPlayers[i].lp_pplsPlayerSource;
      if( ppls!=NULL) {
        // get local player entity
        apenViewers[ctViewers++] = _pNetwork->GetLocalPlayerEntity(ppls);
        // precreate action for it for this tick
        if (bDoPrescan) {
          // copy its local controls to current controls
          memcpy(
            ctl_pvPlayerControls,
            gm_lpLocalPlayers[i].lp_ubPlayerControlsState,
            ctl_slPlayerControlsSize);

          // do prescanning
          CPlayerAction paPreAction;
          INDEX iCurrentPlayer = gm_lpLocalPlayers[i].lp_iPlayer;
          CControls &ctrls = gm_actrlControls[ iCurrentPlayer];
          ctrls.CreateAction(gm_apcPlayers[iCurrentPlayer], paPreAction, TRUE);

          // copy the local controls back
          memcpy(
            gm_lpLocalPlayers[i].lp_ubPlayerControlsState,
            ctl_pvPlayerControls,
            ctl_slPlayerControlsSize);
        }
      }
    }}

    // fill in all players that are not local
    INDEX ctNonlocals = 0;
    CEntity *apenNonlocals[16];
    memset(apenNonlocals, 0, sizeof(apenNonlocals));
    {for (INDEX i=0; i<16; i++) {
      CEntity *pen = CEntity::GetPlayerEntity(i);
      if (pen!=NULL && !_pNetwork->IsPlayerLocal(pen)) {
        apenNonlocals[ctNonlocals++] = pen;
      }
    }}

    // if there are any non-local players
    if (ctNonlocals>0) {
      // for each observer
      {for (INDEX i=0; i<ctObservers; i++) {
        // get the given player with given offset that is not local
        INDEX iPlayer = (i+iObserverOffset)%ctNonlocals;
        apenViewers[ctViewers++] = apenNonlocals[iPlayer];
      }}
    }

    // for each view
    BOOL bHadViewers = FALSE;
    {for (INDEX i=0; i<ctViewers; i++) {
      CDrawPort *pdp = apdpDrawPorts[i];
      if (pdp!=NULL && pdp->Lock()) {

        // if there is a viewer
        if (apenViewers[i]!=NULL) {
          bHadViewers = TRUE;
          // if predicted
          if (apenViewers[i]->IsPredicted()) {
            // use predictor instead
            apenViewers[i] = apenViewers[i]->GetPredictor();
          }

          if (!CAM_IsOn()) {
            _bPlayerViewRendered = TRUE;
            // render it
            apenViewers[i]->RenderGameView(pdp, (void*)((size_t)ulFlags));
          } else {
            CAM_Render(apenViewers[i], pdp);
          }
        } else {
          pdp->Fill( C_BLACK|CT_OPAQUE);
        }
        pdp->Unlock();
      }
    }}
    if (!bHadViewers) {
      pdpDrawPort->Lock();
      pdpDrawPort->Fill( C_BLACK|CT_OPAQUE);
      pdpDrawPort->Unlock();
    }

    // create drawport for messages (left on DH)
    CDrawPort dpMsg(pdpDrawPort, TRUE);
    if ((ulFlags&GRV_SHOWEXTRAS) && dpMsg.Lock())
    {
      // print pause indicators
      CTString strIndicator;
      if (_pNetwork->IsDisconnected()) {
        strIndicator.PrintF(TRANSV("Disconnected: %s\nPress F9 to reconnect"), (const char *)_pNetwork->WhyDisconnected());
      } else if (_pNetwork->IsWaitingForPlayers()) {
        strIndicator = TRANS("Waiting for all players to connect");
      } else if (_pNetwork->IsWaitingForServer()) {
        strIndicator = TRANS("Waiting for server to continue");
      } else if (!_pNetwork->IsConnectionStable()) {
        strIndicator = TRANS("Trying to stabilize connection...");
      } else if (_pNetwork->IsGameFinished()) {
        strIndicator = TRANS("Game finished");
      } else if (_pNetwork->IsPaused() || _pNetwork->GetLocalPause()) {
        strIndicator = TRANS("Paused");
      } else if (_tvMenuQuickSave.tv_llValue!=0 && 
        (_pTimer->GetHighPrecisionTimer()-_tvMenuQuickSave).GetSeconds()<3) {
        strIndicator = TRANS("Use F6 for QuickSave during game!");
      } else if (_pNetwork->ga_sesSessionState.ses_strMOTD!="") {
        CTString strMotd = _pNetwork->ga_sesSessionState.ses_strMOTD;
        static CTString strLastMotd = "";
        static CTimerValue tvLastMotd((__int64) 0);
        if (strLastMotd!=strMotd) {
          tvLastMotd = _pTimer->GetHighPrecisionTimer();
          strLastMotd = strMotd;
        }
        if (tvLastMotd.tv_llValue!=((__int64) 0) && (_pTimer->GetHighPrecisionTimer()-tvLastMotd).GetSeconds()<3) {
          strIndicator = strMotd;
        }
      }

      if (strIndicator!="") {
        // setup font
        dpMsg.SetFont( _pfdDisplayFont);
        dpMsg.SetTextAspect( 1.0f);
        dpMsg.PutTextCXY( strIndicator, 
        dpMsg.GetWidth()*0.5f,
        #ifdef FIRST_ENCOUNTER  // First Encounter
        dpMsg.GetHeight()*0.4f, C_GREEN|192);    
        #else // Second Encounter
        dpMsg.GetHeight()*0.4f, SE_COL_BLUEGREEN_LT|192);    
        #endif
      }
      // print recording indicator
      if (_pNetwork->IsRecordingDemo()) {
        // setup font
        dpMsg.SetFont( _pfdDisplayFont);
        dpMsg.SetTextScaling( 1.0f);
        dpMsg.SetTextAspect( 1.0f);
        dpMsg.PutText( TRANS("Recording"), 
        dpMsg.GetWidth()*0.1f, 
        dpMsg.GetHeight()*0.1f, C_CYAN|192);
      }

      // print some statistics
      PrintStats( &dpMsg);
  
      // print last few lines from console to top of screen
      if (_pGame->gm_csConsoleState==CS_OFF) ConsolePrintLastLines( &dpMsg);

      // print demo OSD
      if( dem_bOSD) {
        CTString strMessage;
        // print the message
        strMessage.PrintF("%.2fs", _pNetwork->ga_fDemoTimer);
        dpMsg.SetFont( _pfdDisplayFont);
        dpMsg.SetTextAspect( 1.0f);
        dpMsg.PutText( strMessage, 20, 20);
      }
      dpMsg.Unlock();
    }

    // keep frames' time if required
    if( gm_bProfileDemo)
    {
      CTimerValue tvThisFrame = _pTimer->GetHighPrecisionTimer();
      // if demo has been finished
      if( _pNetwork->IsDemoPlayFinished()) {
        // end profile
        gm_bProfileDemo = FALSE;
        CPrintF( DemoReportAnalyzedProfile());
        CPrintF( "-\n");
      } else {
        // determine frame time delta
        TIME tmDelta = (tvThisFrame - _tvLastFrame).GetSeconds();
        _tvLastFrame =  tvThisFrame;
        _atmFrameTimes.Push() = tmDelta;  // add new frame time stamp
        INDEX *piTriangles = _actTriangles.Push(4); // and polygons count
        piTriangles[0] = _pGfx->gl_ctWorldTriangles;
        piTriangles[1] = _pGfx->gl_ctModelTriangles;
        piTriangles[2] = _pGfx->gl_ctParticleTriangles;
        piTriangles[3] = _pGfx->gl_ctTotalTriangles;
      }
    }
    
    // execute cvar after demoplay
    if( _pNetwork->IsDemoPlayFinished() && dem_strPostExec!="") _pShell->Execute(dem_strPostExec);
  }

  // if no game is active
  else
  {
    // clear background
    if( pdpDrawPort->Lock()) {
        #ifdef FIRST_ENCOUNTER  // First Encounter
  	    pdpDrawPort->Fill( C_dGREEN|CT_OPAQUE);   
        #else // Second Encounter
  	    pdpDrawPort->Fill( SE_COL_BLUE_DARK|CT_OPAQUE);   
        #endif
      pdpDrawPort->FillZBuffer( ZBUF_BACK);
      pdpDrawPort->Unlock();
    }
  }

  // check for new chat message
  static INDEX ctChatMessages=0;
  INDEX ctNewChatMessages = _pShell->GetINDEX("net_ctChatMessages");
  if (ctNewChatMessages!=ctChatMessages) {
    ctChatMessages=ctNewChatMessages;
    PlayScriptSound(MAX_SCRIPTSOUNDS-1, CTFILENAME("Sounds\\Menu\\Chat.wav"), 4.0f*gam_fChatSoundVolume, 1.0f, FALSE);
  }

  // update sounds and forbid probing
  _pSound->UpdateSounds();
  _pGfx->gl_bAllowProbing = FALSE;

  if( bSaveScreenShot || dem_iAnimFrame>=0)
  {
    // make the screen shot directory if it doesn't already exist
    bSaveScreenShot = FALSE;
    CTFileName fnmExpanded;
    ExpandFilePath(EFP_WRITE, CTString("ScreenShots"), fnmExpanded);

    // rcg01052002 This is done in Stream.cpp, now. --ryan.
    //_mkdir(fnmExpanded);

    // create a name for screenshot
    CTFileName fnmScreenShot;
    if( dem_iAnimFrame<0) {
      fnmScreenShot = MakeScreenShotName();
    } else {
      // create number for the file
      CTString strNumber;
      strNumber.PrintF("%05d", (INDEX)dem_iAnimFrame);
      fnmScreenShot = CTString("ScreenShots\\Anim_")+strNumber+".tga";
      dem_iAnimFrame+=1;
    }
    // grab screen creating image info
    CImageInfo iiImageInfo;
    if( pdpDrawPort->Lock()) {
      pdpDrawPort->GrabScreen( iiImageInfo, 1);
      pdpDrawPort->Unlock();
    }
    // try to
    try {
      // save screen shot as TGA
      iiImageInfo.SaveTGA_t( fnmScreenShot);
      if( dem_iAnimFrame<0) CPrintF( TRANS("screen shot: %s\n"), (const char *) (CTString&)fnmScreenShot);
    }
    // if failed
    catch (const char *strError) {
      // report error
      CPrintF( TRANS("Cannot save screenshot:\n%s\n"), (const char *) strError);
    }
  }
}


void CGame::RecordHighScore(void)
{
  // if game is not running
  if (!gm_bGameOn) {
    // do nothing
    return;
  }
  // find that player
  INDEX ipl= Clamp(int(gam_iRecordHighScore), 0, NET_MAXGAMEPLAYERS);
  CPlayer *penpl = (CPlayer *)&*CEntity::GetPlayerEntity(ipl);
  if (penpl==NULL) {
    //CPrintF( TRANS("Warning: cannot record score for player %d!\n"), ipl);
    return;
  }

  // get its score
  INDEX ctScore = penpl->m_psGameStats.ps_iScore;

  // find entry with lower score
  INDEX iLess;
  for(iLess=0; iLess<HIGHSCORE_COUNT; iLess++) {
    if (gm_ahseHighScores[iLess].hse_ctScore<ctScore) {
      break;
    }
  }
  // if none
  if (iLess>=HIGHSCORE_COUNT) {
    // do nothing more
    return;
  }

  // move all lower entries one place down, dropping the last one
  for(INDEX i=HIGHSCORE_COUNT-1; i>iLess; i--) {
    gm_ahseHighScores[i] = gm_ahseHighScores[i-1];
  }

  // remember new one
  gm_ahseHighScores[iLess].hse_ctScore = ctScore;
  gm_ahseHighScores[iLess].hse_strPlayer = penpl->GetPlayerName();
  gm_ahseHighScores[iLess].hse_gdDifficulty = GetSP()->sp_gdGameDifficulty;
  if (GetSP()->sp_bMental) {
    (INDEX&)gm_ahseHighScores[iLess].hse_gdDifficulty = CSessionProperties::GD_EXTREME+1;
  }
  gm_ahseHighScores[iLess].hse_tmTime = _pTimer->CurrentTick();
  gm_ahseHighScores[iLess].hse_ctKills = penpl->m_psGameStats.ps_iKills;

  // remember best for player hud and statistics
  plr_iHiScore = gm_ahseHighScores[0].hse_ctScore;

  // remember last set
  gm_iLastSetHighScore = iLess;
}

INDEX CGame::GetLivePlayersCount(void)
{
  INDEX ctLive = 0;

  for (INDEX ipl=0; ipl<NET_MAXGAMEPLAYERS; ipl++) {
    CEntity *penpl = CEntity::GetPlayerEntity(ipl);
    if (penpl!=NULL && (penpl->GetFlags()&ENF_ALIVE)) {
      ctLive++;
    }
  }

  return ctLive;
}

INDEX CGame::GetPlayersCount(void)
{
  INDEX ctPlayers = 0;

  for (INDEX ipl=0; ipl<NET_MAXGAMEPLAYERS; ipl++) {
    CEntity *penpl = CEntity::GetPlayerEntity(ipl);
    if (penpl!=NULL) {
      ctPlayers++;
    }
  }

  return ctPlayers;
}

// get default description for a game (for save games/demos)
CTString CGame::GetDefaultGameDescription(BOOL bWithInfo)
{
  CTString strDescription;

  struct tm *newtime;
  time_t long_time;
  time(&long_time);
  newtime = localtime(&long_time);

  setlocale(LC_ALL, "");
  CTString strTimeline;
  char achTimeLine[256]; 
  strftime( achTimeLine, sizeof(achTimeLine)-1, "%x %H:%M", newtime);
  strTimeline = achTimeLine;
  setlocale(LC_ALL, "C");

  strDescription.PrintF( "%s - %s", (const char *) TranslateConst(_pNetwork->ga_World.GetName(), 0), (const char *) strTimeline);

  if (bWithInfo) {
    CPlayer *penPlayer = (CPlayer *)&*CEntity::GetPlayerEntity(0);
    CTString strStats;
    if (penPlayer!=NULL) {
      penPlayer->GetStats(strStats, CST_SHORT, 40);
    }
    strDescription += "\n"+strStats;
  }

  return strDescription;
}

struct QuickSave {
  CListNode qs_lnNode;
  CTFileName qs_fnm;
  INDEX qs_iNumber;
};

int qsort_CompareQuickSaves_FileUp( const void *elem1, const void *elem2)
{
  const QuickSave &qs1 = **(QuickSave **)elem1;
  const QuickSave &qs2 = **(QuickSave **)elem2;
  return strcmp(qs1.qs_fnm, qs2.qs_fnm);
}

// delete extra quicksaves and find the next free number
INDEX FixQuicksaveDir(const CTFileName &fnmDir, INDEX ctMax)
{
  // list the directory
  CDynamicStackArray<CTFileName> afnmDir;
  MakeDirList(afnmDir, fnmDir, CTString("*.sav"), 0);

  CListHead lh;
  INDEX iMaxNo = -1;
  
  // for each file in the directory
  for (INDEX i=0; i<afnmDir.Count(); i++) {
    CTFileName fnmName = afnmDir[i];

    // parse it 
    INDEX iFile = -1;
    fnmName.FileName().ScanF("QuickSave%d", &iFile);
    // if it can be parsed
    if (iFile>=0) {
      // create new info for that file
      QuickSave *pqs = new QuickSave;
      pqs->qs_fnm = fnmName;
      pqs->qs_iNumber = iFile;
      if (iFile>iMaxNo) {
        iMaxNo = iFile;
      }
      // add it to list
      lh.AddTail(pqs->qs_lnNode);
    }
  }

  // sort the list
#ifdef _MSC_VER
#ifndef _offsetof
#define _offsetof offsetof
#endif
#endif
  lh.Sort(qsort_CompareQuickSaves_FileUp, _offsetof(QuickSave, qs_lnNode));
  INDEX ctCount = lh.Count();

  // delete all old ones while number of files is too large
  FORDELETELIST(QuickSave, qs_lnNode, lh, itqs) {
    if (ctCount>ctMax) {
      RemoveFile(itqs->qs_fnm);
      RemoveFile(itqs->qs_fnm.NoExt()+"Tbn.tex");
      RemoveFile(itqs->qs_fnm.NoExt()+".des");
      ctCount--;
    }
    delete &*itqs;
  }

  return iMaxNo;
}

CTFileName CGame::GetQuickSaveName(BOOL bSave)
{
  // find out base directory
  CTFileName fnmDir;
  if (GetSP()->sp_ctMaxPlayers==1) {
    INDEX iPlayer = gm_iSinglePlayer;
    if (GetSP()->sp_bQuickTest) {
      iPlayer = gm_iWEDSinglePlayer;
    }
    fnmDir.PrintF("SaveGame\\Player%d\\Quick\\", iPlayer);
  } else {
    if (_pNetwork->IsNetworkEnabled()) {
      fnmDir = CTString("SaveGame\\Network\\Quick\\");
    } else {
      fnmDir = CTString("SaveGame\\SplitScreen\\Quick\\");
    }
  }
  // load last saved number 
  INDEX iLast = FixQuicksaveDir(fnmDir, bSave ? gam_iQuickSaveSlots-1 : gam_iQuickSaveSlots );
  if (bSave) {
    iLast++;
  }

  // add save name to that
  CTFileName fnmName;
  fnmName.PrintF("QuickSave%06d.sav", iLast);
  return fnmDir+fnmName;
}


void CGame::GameMainLoop(void)
{
  if (gam_bQuickSave && GetSP()->sp_gmGameMode!=CSessionProperties::GM_FLYOVER) {
    if (gam_bQuickSave==2) {
      _tvMenuQuickSave = _pTimer->GetHighPrecisionTimer();
    }
    gam_bQuickSave = FALSE;
    CTFileName fnm = GetQuickSaveName(TRUE);
    CTString strDes = GetDefaultGameDescription(TRUE);
    SaveGame(fnm);
    SaveStringVar(fnm.NoExt()+".des", strDes);
  }
  // if quickload invoked
  if (gam_bQuickLoad && GetSP()->sp_gmGameMode!=CSessionProperties::GM_FLYOVER) {
    gam_bQuickLoad = FALSE;
    // if no game active, or this computer is server
    if (!gm_bGameOn || _pNetwork->IsServer()) {
      // do a quickload
      LoadGame(GetQuickSaveName(FALSE));
    // otherwise
    } else {
      // rejoin current section
      JoinGame(CNetworkSession(gam_strJoinAddress));
    }
  }
  if (gam_iRecordHighScore>=0) {
    RecordHighScore();
    gam_iRecordHighScore = -1.0f;
  }
  // if server was restarted
  if (gm_bGameOn && !_pNetwork->IsServer() && _pNetwork->IsGameFinished() && _pNetwork->IsDisconnected()) {
    // automatically reconnect
    JoinGame(CNetworkSession(gam_strJoinAddress));
  }

  if (_bStartProfilingNextTime) {
    _bStartProfilingNextTime = FALSE;
    _bProfiling = TRUE;
    _ctProfileRecording = 50;
    // reset the profiles
    _pfRenderProfile.Reset();
    _pfGfxProfile.Reset();
    _pfModelProfile.Reset();
    _pfSoundProfile.Reset();
    _pfNetworkProfile.Reset();
    _pfPhysicsProfile.Reset();
  } else if (_bProfiling) {
    _ctProfileRecording--;
    if (_ctProfileRecording<=0) {
      _bProfiling = FALSE;
      _bDumpNextTime = TRUE;
      _strProfile = "===========================================================\n";
      /* Render profile */
      CTString strRenderReport;
      _pfRenderProfile.Report(strRenderReport);
      _strProfile+=strRenderReport;
      _pfRenderProfile.Reset();

      /* Model profile */
      CTString strModelReport;
      _pfModelProfile.Report(strModelReport);
      _strProfile+=strModelReport;
      _pfModelProfile.Reset();

      /* Gfx profile */
      CTString strGfxReport;
      _pfGfxProfile.Report(strGfxReport);
      _strProfile+=strGfxReport;
      _pfGfxProfile.Reset();

      /* Sound profile */
      CTString strSoundReport;
      _pfSoundProfile.Report(strSoundReport);
      _strProfile+=strSoundReport;
      _pfSoundProfile.Reset();

      /* Network profile */
      CTString strNetworkReport;
      _pfNetworkProfile.Report(strNetworkReport);
      _strProfile+=strNetworkReport;
      _pfNetworkProfile.Reset();

      /* Physics profile */
      CTString strPhysicsReport;
      _pfPhysicsProfile.Report(strPhysicsReport);
      _strProfile+=strPhysicsReport;
      _pfPhysicsProfile.Reset();

      CPrintF( TRANS("Profiling done.\n"));
    }
  }

  if (_bDumpNextTime) {
    _bDumpNextTime = FALSE;
    try {
      // create a file for profile
      CTFileStream strmProfile;
      strmProfile.Create_t(CTString("Game.profile"));
      strmProfile.Write_t(_strProfile, strlen(_strProfile));
    } catch (const char *strError) {
      CPutString(strError);
    }
  }

  // if game is started
  if (gm_bGameOn) {
    // do main loop procesing
    _pNetwork->MainLoop();
  }
}

/*************************************************************
 *         S E C O N D   E N C O U N T E R   M E N U         *
 *************************************************************/

static CTextureObject _toPointer;
static CTextureObject _toBcgClouds;
static CTextureObject _toBcgGrid;
#ifndef FIRST_ENCOUNTER
static CTextureObject _toBackdrop;
static CTextureObject _toSamU;
static CTextureObject _toSamD;
static CTextureObject _toLeftU;
static CTextureObject _toLeftD;
#endif

static PIXaabbox2D _boxScreen_SE;
static PIX _pixSizeI_SE;
static PIX _pixSizeJ_SE;
CDrawPort *_pdp_SE = NULL;
static FLOAT _tmNow_SE;
static ULONG _ulA_SE;
static BOOL  _bPopup;

void TiledTextureSE( PIXaabbox2D &_boxScreen, FLOAT fStretch, const MEX2D &vScreen, MEXaabbox2D &boxTexture)
{
  PIX pixW = _boxScreen.Size()(1);
  PIX pixH = _boxScreen.Size()(2);
  boxTexture = MEXaabbox2D(MEX2D(0, 0), MEX2D(pixW/fStretch, pixH/fStretch));
  boxTexture+=vScreen;
}

////

void CGame::LCDInit(void)
{
  ::_LCDInit();
}
void CGame::LCDEnd(void)
{
  ::_LCDEnd();
}
void CGame::LCDPrepare(FLOAT fFade)
{
  ::_LCDPrepare(fFade);
}
void CGame::LCDSetDrawport(CDrawPort *pdp)
{
  ::_LCDSetDrawport(pdp);
}
void CGame::LCDDrawBox(PIX pixUL, PIX pixDR, const PIXaabbox2D &box, COLOR col)
{
  ::_LCDDrawBox(pixUL, pixDR, box, col);
}
void CGame::LCDScreenBox(COLOR col)
{
  ::_LCDScreenBox(col);
}
void CGame::LCDScreenBoxOpenLeft(COLOR col)
{
  ::_LCDScreenBoxOpenLeft(col);
}
void CGame::LCDScreenBoxOpenRight(COLOR col)
{
  ::_LCDScreenBoxOpenRight(col);
}
void CGame::LCDRenderClouds1(void)
{

  #ifdef FIRST_ENCOUNTER 
  //LCDRenderCloudsForComp();
  //LCDRenderCompGrid();
  ::_LCDRenderClouds1();
  #else
  _pdp_SE->PutTexture(&_toBackdrop, _boxScreen_SE, C_WHITE|255);

  if (!_bPopup) {

    PIXaabbox2D box;
        
    // right character - Sam
    INDEX iSize = 170;
    INDEX iYU = 120;
    INDEX iYM = iYU + iSize;
    INDEX iYB = iYM + iSize;
    INDEX iXL = 420;
    INDEX iXR = (INDEX) (iXL + iSize*_pdp_SE->dp_fWideAdjustment);
    
    box = PIXaabbox2D( PIX2D( iXL*_pdp_SE->GetWidth()/640, iYU*_pdp_SE->GetHeight()/480) ,
                       PIX2D( iXR*_pdp_SE->GetWidth()/640, iYM*_pdp_SE->GetHeight()/480));

    _pdp_SE->PutTexture(&_toSamU, box, SE_COL_BLUE_NEUTRAL|255);
    box = PIXaabbox2D( PIX2D( iXL*_pdp_SE->GetWidth()/640, iYM*_pdp_SE->GetHeight()/480) ,
                       PIX2D( iXR*_pdp_SE->GetWidth()/640, iYB*_pdp_SE->GetHeight()/480));
    _pdp_SE->PutTexture(&_toSamD, box, SE_COL_BLUE_NEUTRAL|255);

    iSize = 120;
    iYU = 0;
    iYM = iYU + iSize;
    iYB = iYM + iSize;
    iXL = -20;
    iXR = iXL + iSize;
    box = PIXaabbox2D( PIX2D( iXL*_pdp_SE->GetWidth()/640, iYU*_pdp_SE->GetWidth()/640) ,
                       PIX2D( iXR*_pdp_SE->GetWidth()/640, iYM*_pdp_SE->GetWidth()/640));
    _pdp_SE->PutTexture(&_toLeftU, box, SE_COL_BLUE_NEUTRAL|200);   
    box = PIXaabbox2D( PIX2D( iXL*_pdp_SE->GetWidth()/640, iYM*_pdp_SE->GetWidth()/640) ,
                       PIX2D( iXR*_pdp_SE->GetWidth()/640, iYB*_pdp_SE->GetWidth()/640));
    _pdp_SE->PutTexture(&_toLeftD, box, SE_COL_BLUE_NEUTRAL|200);   

    iYU = iYB;
    iYM = iYU + iSize;
    iYB = iYM + iSize;
    iXL = -20;
    iXR = iXL + iSize;
    box = PIXaabbox2D( PIX2D( iXL*_pdp_SE->GetWidth()/640, iYU*_pdp_SE->GetWidth()/640) ,
                       PIX2D( iXR*_pdp_SE->GetWidth()/640, iYM*_pdp_SE->GetWidth()/640));
    _pdp_SE->PutTexture(&_toLeftU, box, SE_COL_BLUE_NEUTRAL|200);

    box = PIXaabbox2D( PIX2D( iXL*_pdp_SE->GetWidth()/640, iYM*_pdp_SE->GetWidth()/640) ,
                       PIX2D( iXR*_pdp_SE->GetWidth()/640, iYB*_pdp_SE->GetWidth()/640));
    _pdp_SE->PutTexture(&_toLeftD, box, SE_COL_BLUE_NEUTRAL|200);
  }

  MEXaabbox2D boxBcgClouds1;
  TiledTextureSE(_boxScreen_SE, 1.2f*_pdp_SE->GetWidth()/640.0f, 
    MEX2D(sin(_tmNow_SE*0.5f)*35,sin(_tmNow_SE*0.7f+1)*21),   boxBcgClouds1);
  _pdp_SE->PutTexture(&_toBcgClouds, _boxScreen_SE, boxBcgClouds1, C_BLACK|_ulA_SE>>2);
  TiledTextureSE(_boxScreen_SE, 0.7f*_pdp_SE->GetWidth()/640.0f, 
    MEX2D(sin(_tmNow_SE*0.6f+1)*32,sin(_tmNow_SE*0.8f)*25),   boxBcgClouds1);
  _pdp_SE->PutTexture(&_toBcgClouds, _boxScreen_SE, boxBcgClouds1, C_BLACK|_ulA_SE>>2);
  #endif
}
void CGame::LCDRenderCloudsForComp(void)
{
  MEXaabbox2D boxBcgClouds1;
  TiledTextureSE(_boxScreen_SE, 1.856f*_pdp_SE->GetWidth()/640.0f, 
    MEX2D(sin(_tmNow_SE*0.5f)*35,sin(_tmNow_SE*0.7f)*21),   boxBcgClouds1);
    #ifdef FIRST_ENCOUNTER  // First Encounter
  _pdp_SE->PutTexture(&_toBcgClouds, _boxScreen_SE, boxBcgClouds1, SE_COL_GREEN_NEUTRAL|_ulA_SE>>2);
    #else // Second Encounter
  _pdp_SE->PutTexture(&_toBcgClouds, _boxScreen_SE, boxBcgClouds1, SE_COL_BLUE_NEUTRAL|_ulA_SE>>2);
    #endif
  TiledTextureSE(_boxScreen_SE, 1.323f*_pdp_SE->GetWidth()/640.0f, 
    MEX2D(sin(_tmNow_SE*0.6f)*31,sin(_tmNow_SE*0.8f)*25),   boxBcgClouds1);
    #ifdef FIRST_ENCOUNTER  // First Encounter
  _pdp_SE->PutTexture(&_toBcgClouds, _boxScreen_SE, boxBcgClouds1, SE_COL_GREEN_NEUTRAL|_ulA_SE>>2);
    #else // Second Encounter
  _pdp_SE->PutTexture(&_toBcgClouds, _boxScreen_SE, boxBcgClouds1, SE_COL_BLUE_NEUTRAL|_ulA_SE>>2);
    #endif
}
void CGame::LCDRenderClouds2(void)
{
  ::_LCDRenderClouds2();
}
void CGame::LCDRenderGrid(void)
{
  ::_LCDRenderGrid();
}
void CGame::LCDDrawPointer(PIX pixI, PIX pixJ)
{
  ::_LCDDrawPointer(pixI, pixJ);
}
COLOR CGame::LCDGetColor(COLOR colDefault, const char *strName)
{

  #ifdef FIRST_ENCOUNTER  // First Encounter
  if (!strcmp(strName, "thumbnail border")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "no thumbnail")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "popup box")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "tool tip")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "unselected")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "selected")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "disabled selected")) {
    colDefault = SE_COL_GREEN_NEUTRAL |255;
  } else if (!strcmp(strName, "disabled unselected")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "label")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "title")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "editing")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "hilited")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "hilited rectangle")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "edit fill")) {
    colDefault = SE_COL_GREEN_NEUTRAL|75;
  } else if (!strcmp(strName, "editing cursor")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "model box")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "hiscore header")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "hiscore data")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "hiscore last set")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "slider box")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "file info")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "display mode")) {
    colDefault = SE_COL_GREEN_LIGHT|255;
  } else if (!strcmp(strName, "bcg fill")) {
    colDefault = SE_COL_GREEN_DARK_LT|255;
  }
  #else // Second Encounter
  if (!strcmp(strName, "thumbnail border")) {
    colDefault = SE_COL_BLUE_NEUTRAL|255;
  } else if (!strcmp(strName, "no thumbnail")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "popup box")) {
    colDefault = SE_COL_BLUE_NEUTRAL|255;
  } else if (!strcmp(strName, "tool tip")) {
    colDefault = SE_COL_ORANGE_LIGHT|255;
  } else if (!strcmp(strName, "unselected")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "selected")) {
    colDefault = SE_COL_ORANGE_LIGHT|255;
  } else if (!strcmp(strName, "disabled selected")) {
    colDefault = SE_COL_ORANGE_DARK_LT |255;
  } else if (!strcmp(strName, "disabled unselected")) {
    colDefault = SE_COL_ORANGE_DARK|255;
  } else if (!strcmp(strName, "label")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "title")) {
    colDefault = C_WHITE|255;
  } else if (!strcmp(strName, "editing")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "hilited")) {
    colDefault = SE_COL_ORANGE_LIGHT|255;
  } else if (!strcmp(strName, "hilited rectangle")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "edit fill")) {
    colDefault = SE_COL_BLUE_DARK_LT|75;
  } else if (!strcmp(strName, "editing cursor")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "model box")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "hiscore header")) {
    colDefault = SE_COL_ORANGE_LIGHT|255;
  } else if (!strcmp(strName, "hiscore data")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "hiscore last set")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "slider box")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "file info")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "display mode")) {
    colDefault = SE_COL_ORANGE_NEUTRAL|255;
  } else if (!strcmp(strName, "bcg fill")) {
    colDefault = SE_COL_BLUE_DARK|255;
  }
  #endif
  return ::_LCDGetColor(colDefault, strName);
}
COLOR CGame::LCDFadedColor(COLOR col)
{
  return ::_LCDFadedColor(col);
}
COLOR CGame::LCDBlinkingColor(COLOR col0, COLOR col1)
{
  return ::_LCDBlinkingColor(col0, col1);
}


// menu interface functions
void CGame::MenuPreRenderMenu(const char *strMenuName)
{
}
void CGame::MenuPostRenderMenu(const char *strMenuName)
{
}
