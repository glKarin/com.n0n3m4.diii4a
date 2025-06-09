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

#include <Engine/StdH.h>

#include <Engine/Build.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/CRCTable.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Network/Server.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/PlayerSource.h>
#include <Engine/Network/PlayerBuffer.h>
#include <Engine/Network/PlayerTarget.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Entities/Precaching.h>
#include <Engine/Network/CommunicationInterface.h>
#include <Engine/Templates/Stock_CModelData.h>
#include <Engine/Templates/Stock_CAnimData.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CSoundData.h>
#include <Engine/Templates/Stock_CEntityClass.h>


#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Sound/SoundListener.h>
#include <Engine/Rendering/Render.h>
#include <Engine/Rendering/Render_internal.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Math/Float.h>

#include <Engine/Rendering/RenderProfile.h>
#include <Engine/Network/NetworkProfile.h>
#include <Engine/Network/LevelChange.h>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Models/ModelObject.h>
#include <Engine/Ska/ModelInstance.h>
#include <Engine/Entities/ShadingInfo.h>
#include <Engine/Entities/EntityCollision.h>
#include <Engine/Entities/LastPositions.h>

#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

#include <Engine/Templates/Stock_CAnimData.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CSoundData.h>
#include <Engine/Templates/Stock_CEntityClass.h>
#include <Engine/Templates/Stock_CModelData.h>
#include <Engine/Templates/Stock_CAnimSet.h>
#include <Engine/Templates/Stock_CMesh.h>
#include <Engine/Templates/Stock_CShader.h>
#include <Engine/Templates/Stock_CSkeleton.h>

#include <Engine/GameAgent/GameAgent.h>


// pointer to global instance of the only game object in the application
CNetworkLibrary *_pNetwork= NULL;

extern BOOL _bNeedPretouch;
__extern BOOL _bMultiPlayer = FALSE;
__extern INDEX _ctEntities = 0;
__extern INDEX _ctPredictorEntities = 0;
__extern LevelChangePhase _lphCurrent = LCP_NOCHANGE;
__extern BOOL _bTempNetwork = FALSE;  // set while using temporary second network object
extern BOOL con_bCapture;
extern CTString con_strCapture;

static FLOAT _bStartDemoRecordingNextTime = FALSE;
static FLOAT _bStopDemoRecordingNextTime = FALSE;
static INDEX dem_iRecordedNumber = 0;

// network control
__extern INDEX ser_bReportSyncOK   = FALSE;
__extern INDEX ser_bReportSyncBad  = TRUE;
__extern INDEX ser_bReportSyncLate = FALSE;
__extern INDEX ser_bReportSyncEarly = FALSE;
__extern INDEX ser_bPauseOnSyncBad = FALSE;
__extern INDEX ser_iKickOnSyncBad = 10;
__extern INDEX ser_bKickOnSyncLate = 1;
__extern INDEX ser_iRememberBehind = 3000;
__extern INDEX ser_iExtensiveSyncCheck = 0;
__extern INDEX ser_bClientsMayPause = TRUE;
__extern FLOAT ser_tmSyncCheckFrequency = 1.0f;
__extern INDEX ser_iSyncCheckBuffer = 60;
__extern INDEX ser_bEnumeration  = TRUE;
__extern INDEX ser_bPingGameAgent = TRUE;
__extern FLOAT ser_tmKeepAlive = 0.1f;
__extern FLOAT ser_tmPingUpdate = 3.0f;
__extern INDEX ser_bWaitFirstPlayer = 0;
__extern INDEX ser_iMaxAllowedBPS = 8000;
__extern CTString ser_strIPMask = "";
__extern CTString ser_strNameMask = "";
__extern INDEX ser_bInverseBanning = FALSE;
__extern CTString ser_strMOTD = "";

__extern INDEX cli_bEmulateDesync  = FALSE;
__extern INDEX cli_bDumpSync       = FALSE;
__extern INDEX cli_bDumpSyncEachTick = FALSE;
__extern INDEX cli_bAutoAdjustSettings = FALSE;
__extern FLOAT cli_tmAutoAdjustThreshold = 2.0f;
__extern INDEX cli_bPrediction = FALSE;
__extern INDEX cli_iMaxPredictionSteps = 10;
__extern INDEX cli_bPredictIfServer = FALSE;
__extern INDEX cli_bPredictLocalPlayers = TRUE;
__extern INDEX cli_bPredictRemotePlayers = FALSE;
__extern FLOAT cli_fPredictEntitiesRange = 20.0f;
__extern INDEX cli_bLerpActions = FALSE;
__extern INDEX cli_bReportPredicted = FALSE;
__extern INDEX cli_iSendBehind = 3;
__extern INDEX cli_iPredictionFlushing = 1;

__extern INDEX cli_iBufferActions = 1;
__extern INDEX cli_iMaxBPS = 4000;
__extern INDEX cli_iMinBPS = 0;

__extern INDEX net_iCompression = 1;
__extern INDEX net_bLookupHostNames = FALSE;
__extern INDEX net_bReportPackets = FALSE;
__extern INDEX net_iMaxSendRetries = 10;
__extern FLOAT net_fSendRetryWait = 0.5f;
__extern INDEX net_bReportTraffic = FALSE;
__extern INDEX net_bReportICMPErrors = FALSE;
__extern INDEX net_bReportMiscErrors = FALSE;
__extern INDEX net_bLerping       = TRUE;
__extern INDEX net_iGraphBuffer = 100;
__extern INDEX net_iExactTimer = 2;
__extern INDEX net_bDumpStreamBlocks = 0;
__extern INDEX net_bDumpConnectionInfo = 0;
__extern INDEX net_iPort = 25600;
__extern CTString net_strLocalHost = "";
__extern CTString net_strLocationCode = "";
__extern CTString net_strConnectPassword = "";
__extern CTString net_strAdminPassword = "";
__extern CTString net_strVIPPassword = "";
__extern CTString net_strObserverPassword = "";
__extern INDEX net_iVIPReserve = 0;
__extern INDEX net_iMaxObservers = 16;
__extern INDEX net_iMaxClients = 0;
__extern FLOAT net_tmConnectionTimeout = 30.0f;
__extern FLOAT net_tmProblemsTimeout = 5.0f;
__extern FLOAT net_tmDisconnectTimeout = 300.0f;  // must be higher for level changing
__extern INDEX net_bReportCRC = FALSE;
__extern FLOAT net_fDropPackets = 0.0f;
__extern FLOAT net_tmLatency = 0.0f;

__extern INDEX ent_bReportSpawnInWall = FALSE;

__extern FLOAT cmd_tmTick = 0.0f;
__extern CTString cmd_cmdOnTick = "";
__extern CTString cmd_strChatSender = "";
__extern CTString cmd_strChatMessage = "";
__extern CTString cmd_cmdOnChat = "";
__extern INDEX net_ctChatMessages = 0;  // counter for incoming chat messages

extern CPacketBufferStats _pbsSend;
extern CPacketBufferStats _pbsRecv;

__extern BOOL _bPredictionActive = FALSE;
__extern BOOL _bPlayDemoActive = FALSE;

class CGatherCRC {
public:
  BOOL bOld;
  CGatherCRC();
  ~CGatherCRC();
};

CGatherCRC::CGatherCRC() {
  bOld = CRCT_bGatherCRCs;
}
CGatherCRC::~CGatherCRC() {
  CRCT_bGatherCRCs = bOld;
}

// precache control
__extern INDEX _precache_NONE      = PRECACHE_NONE;
__extern INDEX _precache_SMART     = PRECACHE_SMART;
__extern INDEX _precache_ALL       = PRECACHE_ALL;
__extern INDEX _precache_PARANOIA  = PRECACHE_PARANOIA;
__extern INDEX gam_iPrecachePolicy = _precache_SMART;
__extern INDEX _precache_bNowPrecaching = FALSE;

__extern INDEX dbg_bBreak = FALSE;
__extern INDEX gam_bPretouch = FALSE;

__extern FLOAT phy_fCollisionCacheAhead  = 5.0f;
__extern FLOAT phy_fCollisionCacheAround = 1.5f;
__extern FLOAT cli_fPredictionFilter = 0.5f;

extern INDEX shd_bCacheAll;


// input
__extern INDEX inp_iKeyboardReadingMethod = 2;  // 0=getasynckey, 1=virtkeytrap, 2=scancodetrap
__extern INDEX inp_bAllowMouseAcceleration = TRUE;
__extern FLOAT inp_fMouseSensitivity = 1.0f;
__extern INDEX inp_bMousePrecision = FALSE;
__extern FLOAT inp_fMousePrecisionFactor = 4.0f;
__extern FLOAT inp_fMousePrecisionThreshold = 10.0f;
__extern FLOAT inp_fMousePrecisionTimeout = 0.25f;
__extern FLOAT inp_bInvertMouse = FALSE;
__extern INDEX inp_bFilterMouse = FALSE;
__extern INDEX inp_bAllowPrescan = TRUE;

__extern INDEX inp_i2ndMousePort = 0; // COM no (0=disable)
__extern FLOAT inp_f2ndMouseSensitivity = 1.0f;
__extern INDEX inp_b2ndMousePrecision = FALSE;
__extern FLOAT inp_f2ndMousePrecisionFactor = 4.0f;
__extern FLOAT inp_f2ndMousePrecisionThreshold = 10.0f;
__extern FLOAT inp_f2ndMousePrecisionTimeout = 0.25f;
__extern INDEX inp_bInvert2ndMouse = FALSE;
__extern INDEX inp_bFilter2ndMouse = FALSE;

extern INDEX inp_iMButton4Up;
extern INDEX inp_iMButton4Dn;
extern INDEX inp_iMButton5Up;
extern INDEX inp_iMButton5Dn;
extern INDEX inp_bMsgDebugger;
extern INDEX inp_ctJoysticksAllowed;
extern INDEX inp_bForceJoystickPolling;
extern INDEX inp_bAutoDisableJoysticks;

__extern INDEX wed_bUseGenericTextureReplacement = FALSE;


extern void RendererInfo(void);
extern void ClearRenderer(void);


// cache all shadowmaps now
extern void CacheShadows(void)
{
  // mute all sounds
  _pSound->Mute();
  CWorld *pwo = _pShell->GetCurrentWorld();
  if( pwo!=NULL) {
    pwo->wo_baBrushes.CacheAllShadowmaps();
    CPrintF( TRANS("All shadows recached"));
    if( shd_bCacheAll) CPrintF(".\n");
    else CPrintF( TRANS(", but not for long.\n(precache all shadows function is disabled)\n"));
  }
  // mark that we need pretouching
  _bNeedPretouch = TRUE;
}

// check if a name or IP matches a mask
extern BOOL MatchesBanMask(const CTString &strString, const CTString &strMask)
{
  CTString strRest = strMask;
  CTString strLine;

  while(strRest!="") {
    strLine = strRest;
    strLine.OnlyFirstLine();
    strRest.RemovePrefix(strLine);
    strRest.DeleteChar(0);

    if (strString.Matches(strLine)) {
      return TRUE;
    }
  }
  return FALSE;
}

extern CTString RemoveSubstring(const CTString &strFull, const CTString &strSub);

static void AddIPMask(void* pArgs)
{
  CTString strIP = *NEXTARGUMENT(CTString*);
  ser_strIPMask+= strIP+"\n";
}
static void RemIPMask(void* pArgs)
{
  CTString strIP = *NEXTARGUMENT(CTString*);
  ser_strIPMask = RemoveSubstring(ser_strIPMask, strIP+"\n");
}
static void AddNameMask(void* pArgs)
{
  CTString strName = *NEXTARGUMENT(CTString*);
  ser_strNameMask += strName+"\n";
}
static void RemNameMask(void* pArgs)
{
  CTString strName = *NEXTARGUMENT(CTString*);
  ser_strNameMask = RemoveSubstring(ser_strNameMask, strName+"\n");
}

static void StartDemoRecording(void)
{
  _bStartDemoRecordingNextTime = TRUE;
}


static void StopDemoRecording(void)
{
  _bStopDemoRecordingNextTime = TRUE;
}


static void NetworkInfo(void)
{
  CPrintF("*Network library information:\n");
  CPrintF("Entities existing: %d\n", _ctEntities);
  CPrintF("Predictor entities existing: %d\n", _ctPredictorEntities);
  CPrintF("Server:\n");
  if (_pNetwork->ga_srvServer.srv_bActive) {
    CPrintF("  last processed tick: %g\n", _pNetwork->ga_srvServer.srv_tmLastProcessedTick);
    CPrintF("  last processed sequence: %d\n", _pNetwork->ga_srvServer.srv_iLastProcessedSequence);
    CPrintF("  players:\n");
    for(INDEX iplb=0; iplb<_pNetwork->ga_srvServer.srv_aplbPlayers.Count(); iplb++) {
      CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[iplb];
      if (plb.plb_Active) {
        CPrintF("    %2d(%2d):'%s'@client%2d: (%dact)\n",
          iplb, plb.plb_Index, (const char *) plb.plb_pcCharacter.GetNameForPrinting(),
          plb.plb_iClient, plb.plb_abReceived.GetCount());
      }
    }
    CPrintF("  clients:\n");
    for(INDEX iSession=0; iSession<_pNetwork->ga_srvServer.srv_assoSessions.Count(); iSession++) {
      CSessionSocket &sso = _pNetwork->ga_srvServer.srv_assoSessions[iSession];
      if (sso.sso_bActive) {
        CPrintF("  %2d:'%s'\n", iSession, (const char *) _cmiComm.Server_GetClientName(iSession)),
        CPrintF("    buffer: %dblk=%dk\n",
          sso.sso_nsBuffer.GetUsedBlocks(),
          sso.sso_nsBuffer.GetUsedMemory()/1024);
        CPrintF("    state:");
        if (sso.sso_iDisconnectedState>0) {
          CPrintF("    disconnecting");
        } else if (sso.sso_bSendStream) {
          CPrintF("    connected");
        } else {
          CPrintF("    connecting");
        }
        CPrintF("\n");
      }
    }
  } else {
    CPrintF("  not a server\n");
  }
  CPrintF("Session state:\n");
  CPrintF("  buffer: (%dblk)%dk\n",
    _pNetwork->ga_sesSessionState.ses_nsGameStream.GetUsedBlocks(),
    _pNetwork->ga_sesSessionState.ses_nsGameStream.GetUsedMemory()/1024);
  CPrintF("  last processed tick: %g\n", _pNetwork->ga_sesSessionState.ses_tmLastProcessedTick);
  CPrintF("  last processed sequence: %d\n", _pNetwork->ga_sesSessionState.ses_iLastProcessedSequence);
  CPrintF("  level change: %d\n", _pNetwork->ga_sesSessionState.ses_iLevel);
  for(INDEX iplt=0; iplt<_pNetwork->ga_sesSessionState.ses_apltPlayers.Count(); iplt++) {
    CPlayerTarget &plt = _pNetwork->ga_sesSessionState.ses_apltPlayers[iplt];
    if (plt.plt_bActive) {
      ULONG ulID = (ULONG) -1;
      if (plt.plt_penPlayerEntity!=NULL) {
        ulID = plt.plt_penPlayerEntity->en_ulID;
      }
      CPrintF("  player %2d (ID:%d): (%dact)\n", iplt, ulID, plt.plt_abPrediction.GetCount());
    }
  }


    if (TIMER_PROFILING) {
        CTString strNetProfile;
        _pfNetworkProfile.Report(strNetProfile);
        CPrintF(strNetProfile);
    }
}

static void ListPlayers(void)
{
  CPrintF("player list:\n");
  if (!_pNetwork->ga_srvServer.srv_bActive) {
    CPrintF("  <not a server>\n");
    return;
  }

  CPrintF("  client# name\n");
  CPrintF("  ----------------------\n");
  for(INDEX iplb=0; iplb<_pNetwork->ga_srvServer.srv_aplbPlayers.Count(); iplb++) {
    CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[iplb];
    if (plb.plb_Active) {
      CPrintF("     %-2d   %s\n", plb.plb_iClient, (const char *) plb.plb_pcCharacter.GetNameForPrinting());
    }
  }
  CPrintF("  ----------------------\n");
}

static void KickClient(INDEX iClient, const CTString &strReason)
{
  if (!_pNetwork->IsServer()) {
    CPrintF( TRANS("Only server can kick people!\n"));
    return;
  }
  iClient = Clamp(iClient, INDEX(0), INDEX(NET_MAXGAMECOMPUTERS));
  if (!_pNetwork->ga_srvServer.srv_assoSessions[iClient].IsActive()) {
    CPrintF(TRANSV("Client not connected!\n"));
    return;
  }
  if (iClient == 0) {
    CPrintF(TRANSV("Can't kick local client!\n"));
    return;
  }
  CPrintF( TRANS("Kicking %d with explanation '%s'...\n"), iClient, (const char *) strReason);
  _pNetwork->ga_srvServer.SendDisconnectMessage(iClient, "Admin: "+strReason);
}
static void KickClientCfunc(void* pArgs)
{
  INDEX iClient = NEXTARGUMENT(INDEX);
  CTString strReason = *NEXTARGUMENT(CTString*);
  KickClient(iClient, strReason);
}
static void KickByName(const CTString &strName, const CTString &strReason)
{
  if (!_pNetwork->IsServer()) {
    CPrintF( TRANS("Only server can kick people!\n"));
    return;
  }
  for(INDEX iplb=0; iplb<_pNetwork->ga_srvServer.srv_aplbPlayers.Count(); iplb++) {
    CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[iplb];
    if (plb.plb_Active && plb.plb_pcCharacter.GetNameForPrinting().Undecorated().Matches(strName)) {
      KickClient(plb.plb_iClient, strReason);
    }
  }
}
static void KickByNameCfunc(void* pArgs)
{
  CTString strName = *NEXTARGUMENT(CTString*);
  CTString strReason = *NEXTARGUMENT(CTString*);
  KickByName(strName, strReason);
}

static void Admin(void* pArgs)
{
  CTString strCommand = *NEXTARGUMENT(CTString*);

  CNetworkMessage nm(MSG_ADMIN_COMMAND);
  nm<<net_strAdminPassword<<strCommand;
  _pNetwork->SendToServerReliable(nm);
}

static void StockInfo(void)
{
  // find memory used by shadowmap (both cached and uploaded)
  INDEX ctCachedShadows=0, ctDynamicShadows=0, ctFlatShadows=0;
  SLONG slStaticMemory=0,  slDynamicMemory=0,  slUploadMemory=0;
  SLONG slShdBytes=0,  slSlackMemory=0,    slFlatMemory=0;
  INDEX ct256=0, ct128=0, ct64=0, ct32=0, ct16=0;
  SLONG sl256Memory=0, sl128Memory=0, sl64Memory=0, sl32Memory=0, sl16Memory=0;

  if( _pGfx!=NULL)
  {
    FLOAT fSlackRatio;
    FOREACHINLIST( CShadowMap, sm_lnInGfx, _pGfx->gl_lhCachedShadows, itsm)
    { // get polygon size in pixels (used portion of shadowmap)
      SLONG slStaticSize, slDynamicSize, slUploadSize;
      BOOL bIsFlat = itsm->GetUsedMemory( slStaticSize, slDynamicSize, slUploadSize, fSlackRatio);
      SLONG slTotalSize = slDynamicSize+slUploadSize;
      if( bIsFlat) {
        slStaticMemory += 4;
        slTotalSize    += 4;
        slFlatMemory   += slStaticSize;
        ctFlatShadows++;
      } else {
        slStaticMemory += slStaticSize;
        slTotalSize    += slStaticSize;
        if( slTotalSize>0) ctCachedShadows++;
      }
      if( slDynamicSize>0) {
        slDynamicMemory += slDynamicSize;
        ctDynamicShadows++;
      }
      slUploadMemory  += slUploadSize;
      slShdBytes  += slTotalSize + sizeof(CShadowMap);
      slSlackMemory   += (SLONG) (slTotalSize*fSlackRatio);

      if( !bIsFlat) { // by size ...
        if(      slStaticSize>128*1024) { ct256++; sl256Memory+=slTotalSize; }
        else if( slStaticSize> 64*1024) { ct128++; sl128Memory+=slTotalSize; }
        else if( slStaticSize> 32*1024) { ct64++;  sl64Memory +=slTotalSize; }
        else if( slStaticSize> 16*1024) { ct32++;  sl32Memory +=slTotalSize; }
        else if( slStaticSize> 0)       { ct16++;  sl16Memory +=slTotalSize; }
      }
    }
    // report shadowmap memory usage (if any)
    if( slShdBytes>0) {
      CPrintF( "\nCached shadowmaps:\n");
      CPrintF( "    Total: %d in %d KB with %d%% (%d KB) of slack space\n", ctCachedShadows, slShdBytes/1024, slSlackMemory*100/slShdBytes, slSlackMemory/1024);
      CPrintF( "   Static: %d KB\n", slStaticMemory/1024);
      CPrintF( "   Upload: %d KB\n", slUploadMemory/1024);
      CPrintF( "  Dynamic: %d in %d KB\n", ctDynamicShadows, slDynamicMemory/1024);
      if( ctCachedShadows<1) ctCachedShadows=1; // for percentage calc
      CPrintF( "    Flats: %d (%d%%) with %d KB saved\n", ctFlatShadows, ctFlatShadows*100/ctCachedShadows, slFlatMemory/1024);
      CPrintF("of size:\n");
      CPrintF( "    >128K: %4d in %d KB\n", ct256, sl256Memory/1024);
      CPrintF( "  128-64K: %4d in %d KB\n", ct128, sl128Memory/1024);
      CPrintF( "   64-32K: %4d in %d KB\n", ct64,  sl64Memory /1024);
      CPrintF( "   32-16K: %4d in %d KB\n", ct32,  sl32Memory /1024);
      CPrintF( "    <=16K: %4d in %d KB\n", ct16,  sl16Memory /1024);
    }
  }

  // report world stats
  INDEX ctEntities=0, ctShadowLayers=0, ctPolys=0,    ctPlanes=0,   ctEdges=0,    ctVertices=0, ctSectors=0;
  SLONG slEntBytes=0, slLyrBytes=0,     slPlyBytes=0, slPlnBytes=0, slEdgBytes=0, slVtxBytes=0, slSecBytes=0;
  SLONG slCgrBytes=0;
  CWorld *pwo = _pShell->GetCurrentWorld();

  if( pwo!=NULL)
  {
    // report count of and memory used by entities
    FOREACHINDYNAMICCONTAINER( pwo->wo_cenEntities, CEntity, iten) {
      ctEntities++;
      slEntBytes += iten->GetUsedMemory();
    }

    // report shadow layers and world geometry memory usage
    FOREACHINDYNAMICARRAY( pwo->wo_baBrushes.ba_abrBrushes, CBrush3D, itbr) // for all brush entities in the world
    {
      // skip brush without entity
      if( itbr->br_penEntity==NULL) continue;

      // for each mip
      FOREACHINLIST( CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm)
      {
        // for each sector in the brush mip
        FOREACHINDYNAMICARRAY( itbm->bm_abscSectors, CBrushSector, itbsc)
        {
          // add sector class memory usage to polygons memory
          ctSectors++;
          slSecBytes += itbsc->GetUsedMemory();

          // add each vertex and working vertex in sector
          ctVertices += itbsc->bsc_abvxVertices.Count();
          FOREACHINSTATICARRAY( itbsc->bsc_abvxVertices, CBrushVertex,   itbvx) slVtxBytes += itbvx->GetUsedMemory();
          FOREACHINSTATICARRAY( itbsc->bsc_awvxVertices, CWorkingVertex, itwvx) slVtxBytes += 32; // aligned to 32 bytes!

          // add each plane and working plane in sector
          ctPlanes += itbsc->bsc_abplPlanes.Count();
          FOREACHINSTATICARRAY( itbsc->bsc_abplPlanes, CBrushPlane,   itbpl) slPlnBytes += itbpl->GetUsedMemory();
          FOREACHINSTATICARRAY( itbsc->bsc_awplPlanes, CWorkingPlane, itwpl) slPlnBytes += sizeof(CWorkingPlane);

          // add each edge and working edge in sector
          ctEdges += itbsc->bsc_abedEdges.Count();
          FOREACHINSTATICARRAY( itbsc->bsc_abedEdges, CBrushEdge,   itbed) slEdgBytes += itbed->GetUsedMemory();
          FOREACHINSTATICARRAY( itbsc->bsc_awedEdges, CWorkingEdge, itwed) slEdgBytes += sizeof(CWorkingEdge);

          // for each polygon in sector
          ctPolys += itbsc->bsc_abpoPolygons.Count();
          FOREACHINSTATICARRAY( itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
            CBrushPolygon &bpo = *itbpo;
            slPlyBytes += bpo.GetUsedMemory();
            // count in the shadow layers (if any)
            if( bpo.bpo_smShadowMap.bsm_lhLayers.IsEmpty()) continue; // skip polygon without shadowmap
            ctShadowLayers += bpo.bpo_smShadowMap.GetShadowLayersCount();
            slLyrBytes += bpo.bpo_smShadowMap.GetUsedMemory();
          }
        }
      }
    } // add in memory used by collision grid
    extern SLONG  GetCollisionGridMemory( CCollisionGrid *pcg);
    slCgrBytes += GetCollisionGridMemory( pwo->wo_pcgCollisionGrid);
  }

  // stock info
  const DOUBLE dToMB = 1.0/1024.0/1024.0;
  const FLOAT fTexBytes = dToMB * _pTextureStock->CalculateUsedMemory();
  const FLOAT fMdlBytes = dToMB * _pModelStock->CalculateUsedMemory();
  const FLOAT fSndBytes = dToMB * _pSoundStock->CalculateUsedMemory();
  const FLOAT fMshBytes = dToMB * _pMeshStock->CalculateUsedMemory();
  const FLOAT fAstBytes = dToMB * _pAnimSetStock->CalculateUsedMemory();
  const FLOAT fShaBytes = dToMB * _pShaderStock->CalculateUsedMemory();
  const FLOAT fSkaBytes = dToMB * _pSkeletonStock->CalculateUsedMemory();

  CPrintF("\nStock information:\n");
  CPrintF("     Textures: %5d (%5.2f MB)\n", _pTextureStock->GetTotalCount(), fTexBytes);
  CPrintF("   ShadowMaps: %5d (%5.2f MB)\n", ctCachedShadows, slShdBytes*dToMB);
  CPrintF("     Entities: %5d (%5.2f MB)\n", ctEntities,      slEntBytes*dToMB);
  CPrintF("       Sounds: %5d (%5.2f MB)\n", _pSoundStock->GetTotalCount(), fSndBytes);
  CPrintF("\n");
  CPrintF("      Sectors: %5d (%5.2f MB)\n", ctSectors,  slSecBytes*dToMB);
  CPrintF("       Planes: %5d (%5.2f MB)\n", ctPlanes,   slPlnBytes*dToMB);
  CPrintF("        Edges: %5d (%5.2f MB)\n", ctEdges,    slEdgBytes*dToMB);
  CPrintF("     Polygons: %5d (%5.2f MB)\n", ctPolys,    slPlyBytes*dToMB);
  CPrintF("     Vertices: %5d (%5.2f MB)\n", ctVertices, slVtxBytes*dToMB);
  CPrintF(" ShadowLayers: %5d (%5.2f MB)\n", ctShadowLayers, slLyrBytes*dToMB);
  CPrintF("\n");
  CPrintF("       Models: %5d (%5.2f MB)\n", _pModelStock->GetTotalCount(),    fMdlBytes);
  CPrintF("       Meshes: %5d (%5.2f MB)\n", _pMeshStock->GetTotalCount(),     fMshBytes);
  CPrintF("    Skeletons: %5d (%5.2f MB)\n", _pSkeletonStock->GetTotalCount(), fSkaBytes);
  CPrintF("     AnimSets: %5d (%5.2f MB)\n", _pAnimSetStock->GetTotalCount(),  fAstBytes);
  CPrintF("      Shaders: %5d (%5.2f MB)\n", _pShaderStock->GetTotalCount(),   fShaBytes);
  CPrintF("\n");
  CPrintF("CollisionGrid: %.2f MB\n", slCgrBytes*dToMB);
  CPrintF("--------------\n");
  CPrintF("        Total: %.2f MB\n", fTexBytes+fSndBytes+fMdlBytes+fMshBytes+fSkaBytes+fAstBytes+fShaBytes
  + (slShdBytes+slEntBytes+slSecBytes+slPlnBytes+slEdgBytes+slPlyBytes+slVtxBytes+slLyrBytes+slCgrBytes)*dToMB);
  CPrintF("\n");
}


static void StockDump(void)
{
  try {
    CTFileStream strm;
    CTFileName fnm = CTString("Temp\\StockDump.txt");
    strm.Create_t(fnm);
    strm.PutLine_t("Animations:");
    _pAnimStock->DumpMemoryUsage_t(strm);
    strm.PutLine_t("Textures:");
    _pTextureStock->DumpMemoryUsage_t(strm);
    strm.PutLine_t("Models:");
    _pModelStock->DumpMemoryUsage_t(strm);
    strm.PutLine_t("Sounds:");
    _pSoundStock->DumpMemoryUsage_t(strm);
    strm.PutLine_t("Classes:");
    _pEntityClassStock->DumpMemoryUsage_t(strm);
    CPrintF("Dumped to '%s'\n", (const char *) CTString(fnm));
  } catch (const char *strError) {
    CPrintF("Error: %s\n", strError);
  }
}


// free all unused stocks
extern void FreeUnusedStock(void)
{
  // free all unused stocks
  _pEntityClassStock->FreeUnused();
  _pModelStock->FreeUnused();
  _pSoundStock->FreeUnused();
  _pTextureStock->FreeUnused();
  _pAnimStock->FreeUnused();
}


/*
 * This is called every TickQuantum seconds.
 */
void CNetworkTimerHandler::HandleTimer(void)
{
  ASSERT(this!=NULL);
  if (_bTempNetwork) {
    return; // this can happen during NET_MakeDefaultState_t()!
  }
  // enable stream handling during timer
#if (!defined SINGLE_THREADED)
  CTSTREAM_BEGIN {
#endif

    // do the timer loop
    _pNetwork->TimerLoop();

#if (!defined SINGLE_THREADED)
  } CTSTREAM_END;
#endif
}

/*
 * Default constructor.
 */
CNetworkLibrary::CNetworkLibrary(void) :
  ga_IsServer(FALSE),               // is not server
  ga_srvServer(*new CServer),
  ga_sesSessionState(*new CSessionState),
  ga_bDemoRec(FALSE),               // not recording demo
  ga_bDemoPlay(FALSE),              // not playing demo
  ga_bDemoPlayFinished(FALSE)      // demo not finished
{
  ga_aplsPlayers.New(NET_MAXLOCALPLAYERS);

  // default demo syncronization is real-time, with 1:1 playback speed
  ga_fDemoSyncRate = DEMOSYNC_REALTIME;
  ga_fDemoRealTimeFactor = 1.0f;
  ga_fGameRealTimeFactor = 1.0f;
  ga_pubDefaultState = NULL;
  ga_slDefaultStateSize = 0;
  memset(ga_aubDefaultProperties, 0, sizeof(ga_aubDefaultProperties));
  ga_pubCRCList = NULL;
  ga_slCRCList = 0;
  ga_ulDemoMinorVersion = _SE_BUILD_MINOR;

  ga_csNetwork.cs_iIndex = 2000;
  ga_ctTimersPending = -1;

  ga_fEnumerationProgress = 0;
  ga_bEnumerationChange = FALSE;
}

/*
 * Destructor.
 */
CNetworkLibrary::~CNetworkLibrary(void)
{
  // clear the global world
  ga_World.DeletePredictors();
  ga_World.Clear();

  // free renderer info to free pointers to entities etc.
  if (!_bTempNetwork) {
    extern void ClearRenderer(void);
    ClearRenderer();
  }

  delete &ga_sesSessionState;
  delete &ga_srvServer;
}


/*
 * Initialize game management.
 */
void CNetworkLibrary::Init(const CTString &strGameID)
{
  // remember the game ID
  CMessageDispatcher::Init(strGameID);

  // add shell symbols
  _pShell->DeclareSymbol("user INDEX dbg_bBreak;", (void *)&dbg_bBreak);
  _pShell->DeclareSymbol("persistent user INDEX gam_bPretouch;", (void *)&gam_bPretouch);

  _pShell->DeclareSymbol("user INDEX dem_iRecordedNumber;",     (void *)&dem_iRecordedNumber);
  _pShell->DeclareSymbol("user void StartDemoRecording(void);", (void *)&StartDemoRecording);
  _pShell->DeclareSymbol("user void StopDemoRecording(void);",  (void *)&StopDemoRecording);
  _pShell->DeclareSymbol("user void NetworkInfo(void);",  (void *)&NetworkInfo);
  _pShell->DeclareSymbol("user void StockInfo(void);",    (void *)&StockInfo);
  _pShell->DeclareSymbol("user void StockDump(void);",    (void *)&StockDump);
  _pShell->DeclareSymbol("user void RendererInfo(void);", (void *)&RendererInfo);
  _pShell->DeclareSymbol("user void ClearRenderer(void);",   (void *)&ClearRenderer);
  _pShell->DeclareSymbol("user void CacheShadows(void);",    (void *)&CacheShadows);
  _pShell->DeclareSymbol("user void KickClient(INDEX, CTString);", (void *)&KickClientCfunc);
  _pShell->DeclareSymbol("user void KickByName(CTString, CTString);", (void *)&KickByNameCfunc);
  _pShell->DeclareSymbol("user void ListPlayers(void);", (void *)&ListPlayers);
  _pShell->DeclareSymbol("user void Admin(CTString);", (void *)&Admin);

  _pShell->DeclareSymbol("user void AddIPMask(CTString);", (void *)&AddIPMask);
  _pShell->DeclareSymbol("user void RemIPMask(CTString);", (void *)&RemIPMask);
  _pShell->DeclareSymbol("user void AddNameMask(CTString);", (void *)&AddNameMask);
  _pShell->DeclareSymbol("user void RemNameMask(CTString);", (void *)&RemNameMask);

  _pShell->DeclareSymbol("user FLOAT dem_tmTimer;",         (void *)&ga_fDemoTimer);
  _pShell->DeclareSymbol("user FLOAT dem_fSyncRate;",       (void *)&ga_fDemoSyncRate);
  _pShell->DeclareSymbol("user FLOAT dem_fRealTimeFactor;", (void *)&ga_fDemoRealTimeFactor);
  _pShell->DeclareSymbol("user FLOAT gam_fRealTimeFactor;", (void *)&ga_fGameRealTimeFactor);

  _pShell->DeclareSymbol("user const FLOAT net_tmLatency;", (void *)&net_tmLatency);
  _pShell->DeclareSymbol("user const FLOAT cmd_tmTick;", (void *)&cmd_tmTick);
  _pShell->DeclareSymbol("persistent user CTString cmd_cmdOnTick;", (void *)&cmd_cmdOnTick);
  _pShell->DeclareSymbol("user CTString cmd_strChatSender ;", (void *)&cmd_strChatSender );
  _pShell->DeclareSymbol("user CTString cmd_strChatMessage;", (void *)&cmd_strChatMessage);
  _pShell->DeclareSymbol("persistent user CTString cmd_cmdOnChat;", (void *)&cmd_cmdOnChat);

  _pShell->DeclareSymbol("user INDEX net_ctChatMessages;", (void *)&net_ctChatMessages);

  _pShell->DeclareSymbol("persistent user INDEX ent_bReportSpawnInWall;", (void *)&ent_bReportSpawnInWall);

  _pShell->DeclareSymbol("user INDEX ser_bReportSyncOK;",    (void *)&ser_bReportSyncOK);
  _pShell->DeclareSymbol("user INDEX ser_bReportSyncBad;",   (void *)&ser_bReportSyncBad);
  _pShell->DeclareSymbol("user INDEX ser_bReportSyncLate;",  (void *)&ser_bReportSyncLate);
  _pShell->DeclareSymbol("user INDEX ser_bReportSyncEarly;", (void *)&ser_bReportSyncEarly);
  _pShell->DeclareSymbol("user INDEX ser_bPauseOnSyncBad;",  (void *)&ser_bPauseOnSyncBad);
  _pShell->DeclareSymbol("user INDEX ser_iKickOnSyncBad;",   (void *)&ser_iKickOnSyncBad);
  _pShell->DeclareSymbol("user INDEX ser_bKickOnSyncLate;",  (void *)&ser_bKickOnSyncLate);
  _pShell->DeclareSymbol("persistent user FLOAT ser_tmSyncCheckFrequency;", (void *)&ser_tmSyncCheckFrequency);
  _pShell->DeclareSymbol("persistent user INDEX ser_iSyncCheckBuffer;", (void *)&ser_iSyncCheckBuffer);
  _pShell->DeclareSymbol("persistent user INDEX cli_bLerpActions;", (void *)&cli_bLerpActions);
  _pShell->DeclareSymbol("persistent user INDEX cli_bReportPredicted;", (void *)&cli_bReportPredicted);
  _pShell->DeclareSymbol("persistent user INDEX net_iExactTimer;", (void *)&net_iExactTimer);
  _pShell->DeclareSymbol("user INDEX net_bDumpStreamBlocks;",   (void *)&net_bDumpStreamBlocks);
  _pShell->DeclareSymbol("user INDEX net_bDumpConnectionInfo;", (void *)&net_bDumpConnectionInfo);
  _pShell->DeclareSymbol("user INDEX net_iPort;", (void *)&net_iPort);
  _pShell->DeclareSymbol("persistent user CTString net_strLocalHost;", (void *)&net_strLocalHost);
  _pShell->DeclareSymbol("persistent user CTString net_strLocationCode;", (void *)&net_strLocationCode);
  _pShell->DeclareSymbol("user CTString net_strVIPPassword;", (void *)&net_strVIPPassword);
  _pShell->DeclareSymbol("user CTString net_strObserverPassword;", (void *)&net_strObserverPassword);
  _pShell->DeclareSymbol("user INDEX net_iVIPReserve;", (void *)&net_iVIPReserve);
  _pShell->DeclareSymbol("user INDEX net_iMaxObservers;", (void *)&net_iMaxObservers);
  _pShell->DeclareSymbol("user INDEX net_iMaxClients;", (void *)&net_iMaxClients);
  _pShell->DeclareSymbol("user CTString net_strConnectPassword;", (void *)&net_strConnectPassword);
  _pShell->DeclareSymbol("user CTString net_strAdminPassword;", (void *)&net_strAdminPassword);
  _pShell->DeclareSymbol("user FLOAT net_tmConnectionTimeout;", (void *)&net_tmConnectionTimeout);
  _pShell->DeclareSymbol("user FLOAT net_tmProblemsTimeout;", (void *)&net_tmProblemsTimeout);
  _pShell->DeclareSymbol("user FLOAT net_tmDisconnectTimeout;", (void *)&net_tmDisconnectTimeout);
  _pShell->DeclareSymbol("user INDEX net_bReportCRC;", (void *)&net_bReportCRC);
  _pShell->DeclareSymbol("user INDEX ser_iRememberBehind;", (void *)&ser_iRememberBehind);
  _pShell->DeclareSymbol("user INDEX cli_bEmulateDesync;",  (void *)&cli_bEmulateDesync);
  _pShell->DeclareSymbol("user INDEX cli_bDumpSync;",       (void *)&cli_bDumpSync);
  _pShell->DeclareSymbol("user INDEX cli_bDumpSyncEachTick;", (void *)&cli_bDumpSyncEachTick);

  _pShell->DeclareSymbol("persistent user INDEX ser_iExtensiveSyncCheck;", (void *)&ser_iExtensiveSyncCheck);
  _pShell->DeclareSymbol("persistent user INDEX net_bLookupHostNames;",    (void *)&net_bLookupHostNames);
  _pShell->DeclareSymbol("persistent user INDEX net_iCompression ;",       (void *)&net_iCompression);
  _pShell->DeclareSymbol("persistent user INDEX net_bReportPackets;", (void *)&net_bReportPackets);
  _pShell->DeclareSymbol("persistent user INDEX net_iMaxSendRetries;", (void *)&net_iMaxSendRetries);
  _pShell->DeclareSymbol("persistent user FLOAT net_fSendRetryWait;", (void *)&net_fSendRetryWait);
  _pShell->DeclareSymbol("persistent user INDEX net_bReportTraffic;", (void *)&net_bReportTraffic);
  _pShell->DeclareSymbol("persistent user INDEX net_bReportICMPErrors;", (void *)&net_bReportICMPErrors);
  _pShell->DeclareSymbol("persistent user INDEX net_bReportMiscErrors;", (void *)&net_bReportMiscErrors);
  _pShell->DeclareSymbol("persistent user INDEX net_bLerping;",       (void *)&net_bLerping);
  _pShell->DeclareSymbol("persistent user INDEX ser_bClientsMayPause;", (void *)&ser_bClientsMayPause);
  _pShell->DeclareSymbol("persistent user INDEX ser_bEnumeration;",      (void *)&ser_bEnumeration);
  _pShell->DeclareSymbol("persistent user INDEX ser_bPingGameAgent;", (void *)&ser_bPingGameAgent);
  _pShell->DeclareSymbol("persistent user FLOAT ser_tmKeepAlive;", (void *)&ser_tmKeepAlive);
  _pShell->DeclareSymbol("persistent user FLOAT ser_tmPingUpdate;", (void *)&ser_tmPingUpdate);
  _pShell->DeclareSymbol("persistent user INDEX ser_bWaitFirstPlayer;", (void *)&ser_bWaitFirstPlayer);
  _pShell->DeclareSymbol("persistent user INDEX ser_iMaxAllowedBPS;", (void *)&ser_iMaxAllowedBPS);
  _pShell->DeclareSymbol("persistent user INDEX ser_iMaxAllowedBPS;", (void *)&ser_iMaxAllowedBPS);
  _pShell->DeclareSymbol("persistent user CTString ser_strIPMask;", (void *)&ser_strIPMask);
  _pShell->DeclareSymbol("persistent user CTString ser_strNameMask;", (void *)&ser_strNameMask);
  _pShell->DeclareSymbol("persistent user INDEX ser_bInverseBanning;", (void *)&ser_bInverseBanning);
  _pShell->DeclareSymbol("persistent user CTString ser_strMOTD;", (void *)&ser_strMOTD);

  _pShell->DeclareSymbol("persistent user INDEX cli_bAutoAdjustSettings;",   (void *)&cli_bAutoAdjustSettings);
  _pShell->DeclareSymbol("persistent user FLOAT cli_tmAutoAdjustThreshold;", (void *)&cli_tmAutoAdjustThreshold);
  _pShell->DeclareSymbol("persistent user INDEX cli_bPrediction;",           (void *)&cli_bPrediction);
  _pShell->DeclareSymbol("persistent user INDEX cli_iMaxPredictionSteps;",   (void *)&cli_iMaxPredictionSteps);
  _pShell->DeclareSymbol("persistent user INDEX cli_bPredictIfServer;",      (void *)&cli_bPredictIfServer);
  _pShell->DeclareSymbol("persistent user INDEX cli_bPredictLocalPlayers;",  (void *)&cli_bPredictLocalPlayers);
  _pShell->DeclareSymbol("persistent user INDEX cli_bPredictRemotePlayers;", (void *)&cli_bPredictRemotePlayers);
  _pShell->DeclareSymbol("persistent user FLOAT cli_fPredictEntitiesRange;", (void *)&cli_fPredictEntitiesRange);
  _pShell->DeclareSymbol("persistent user FLOAT cli_fPredictionFilter;", (void *)&cli_fPredictionFilter);
  _pShell->DeclareSymbol("persistent user INDEX cli_iSendBehind;", (void *)&cli_iSendBehind);
  _pShell->DeclareSymbol("persistent user INDEX cli_iPredictionFlushing;", (void *)&cli_iPredictionFlushing);

  _pShell->DeclareSymbol("persistent user INDEX cli_iBufferActions;",  (void *)&cli_iBufferActions);
  _pShell->DeclareSymbol("persistent user INDEX cli_iMaxBPS;",     (void *)&cli_iMaxBPS);
  _pShell->DeclareSymbol("persistent user INDEX cli_iMinBPS;",     (void *)&cli_iMinBPS);

  _pShell->DeclareSymbol("user FLOAT net_fLimitLatencySend;",   (void *)&_pbsSend.pbs_fLatencyLimit);
  _pShell->DeclareSymbol("user FLOAT net_fLimitLatencyRecv;",   (void *)&_pbsRecv.pbs_fLatencyLimit);
  _pShell->DeclareSymbol("user FLOAT net_fLatencyVariationSend;", (void *)&_pbsSend.pbs_fLatencyVariation);
  _pShell->DeclareSymbol("user FLOAT net_fLatencyVariationRecv;", (void *)&_pbsRecv.pbs_fLatencyVariation);
  _pShell->DeclareSymbol("user FLOAT net_fLimitBandwidthSend;", (void *)&_pbsSend.pbs_fBandwidthLimit);
  _pShell->DeclareSymbol("user FLOAT net_fLimitBandwidthRecv;", (void *)&_pbsRecv.pbs_fBandwidthLimit);
  _pShell->DeclareSymbol("user FLOAT net_fDropPackets;", (void *)&net_fDropPackets);

  _pShell->DeclareSymbol("persistent user INDEX net_iGraphBuffer;", (void *)&net_iGraphBuffer);

  _pShell->DeclareSymbol("user const INDEX precache_NONE;",     (void *)&_precache_NONE);
  _pShell->DeclareSymbol("user const INDEX precache_SMART;",    (void *)&_precache_SMART);
  _pShell->DeclareSymbol("user const INDEX precache_ALL;",      (void *)&_precache_ALL);
  _pShell->DeclareSymbol("user const INDEX precache_PARANOIA;", (void *)&_precache_PARANOIA);
  _pShell->DeclareSymbol("persistent user INDEX gam_iPrecachePolicy;", (void *)&gam_iPrecachePolicy);

  _pShell->DeclareSymbol("user FLOAT phy_fCollisionCacheAhead;",  (void *)&phy_fCollisionCacheAhead);
  _pShell->DeclareSymbol("user FLOAT phy_fCollisionCacheAround;", (void *)&phy_fCollisionCacheAround);

  _pShell->DeclareSymbol("persistent user INDEX inp_iKeyboardReadingMethod;",   (void *)&inp_iKeyboardReadingMethod);
  _pShell->DeclareSymbol("persistent user INDEX inp_bAllowMouseAcceleration;",  (void *)&inp_bAllowMouseAcceleration);
  _pShell->DeclareSymbol("persistent user FLOAT inp_fMouseSensitivity;",        (void *)&inp_fMouseSensitivity);
  _pShell->DeclareSymbol("persistent user INDEX inp_bMousePrecision;",          (void *)&inp_bMousePrecision);
  _pShell->DeclareSymbol("persistent user FLOAT inp_fMousePrecisionFactor;",    (void *)&inp_fMousePrecisionFactor);
  _pShell->DeclareSymbol("persistent user FLOAT inp_fMousePrecisionThreshold;", (void *)&inp_fMousePrecisionThreshold);
  _pShell->DeclareSymbol("persistent user FLOAT inp_fMousePrecisionTimeout;",   (void *)&inp_fMousePrecisionTimeout);
  _pShell->DeclareSymbol("persistent user INDEX inp_bInvertMouse;",    (void *)&inp_bInvertMouse);
  _pShell->DeclareSymbol("persistent user INDEX inp_bFilterMouse;",    (void *)&inp_bFilterMouse);
  _pShell->DeclareSymbol("persistent user INDEX inp_bAllowPrescan;",   (void *)&inp_bAllowPrescan);

  _pShell->DeclareSymbol("persistent user INDEX inp_i2ndMousePort;",   (void *)&inp_i2ndMousePort);
  _pShell->DeclareSymbol("persistent user INDEX inp_bInvert2ndMouse;", (void *)&inp_bInvert2ndMouse);
  _pShell->DeclareSymbol("persistent user INDEX inp_bFilter2ndMouse;", (void *)&inp_bFilter2ndMouse);
  _pShell->DeclareSymbol("persistent user FLOAT inp_f2ndMouseSensitivity;",        (void *)&inp_f2ndMouseSensitivity);
  _pShell->DeclareSymbol("persistent user INDEX inp_b2ndMousePrecision;",          (void *)&inp_b2ndMousePrecision);
  _pShell->DeclareSymbol("persistent user FLOAT inp_f2ndMousePrecisionFactor;",    (void *)&inp_f2ndMousePrecisionFactor);
  _pShell->DeclareSymbol("persistent user FLOAT inp_f2ndMousePrecisionThreshold;", (void *)&inp_f2ndMousePrecisionThreshold);
  _pShell->DeclareSymbol("persistent user FLOAT inp_f2ndMousePrecisionTimeout;",   (void *)&inp_f2ndMousePrecisionTimeout);

  _pShell->DeclareSymbol("persistent user INDEX inp_bMsgDebugger;",    (void *)&inp_bMsgDebugger);
  _pShell->DeclareSymbol("persistent user INDEX inp_iMButton4Up;", (void *)&inp_iMButton4Up);
  _pShell->DeclareSymbol("persistent user INDEX inp_iMButton4Dn;", (void *)&inp_iMButton4Dn);
  _pShell->DeclareSymbol("persistent user INDEX inp_iMButton5Up;", (void *)&inp_iMButton5Up);
  _pShell->DeclareSymbol("persistent user INDEX inp_iMButton5Dn;", (void *)&inp_iMButton5Dn);
  _pShell->DeclareSymbol("persistent user INDEX inp_ctJoysticksAllowed;",    (void *)&inp_ctJoysticksAllowed);
  _pShell->DeclareSymbol("persistent user INDEX inp_bForceJoystickPolling;", (void *)&inp_bForceJoystickPolling);
  _pShell->DeclareSymbol("persistent user INDEX inp_bAutoDisableJoysticks;", (void *)&inp_bAutoDisableJoysticks);

  _pShell->DeclareSymbol("persistent user INDEX wed_bUseGenericTextureReplacement;", (void *)&wed_bUseGenericTextureReplacement);

  _pShell->DeclareSymbol("persistent user CTString ga_strServer;", (void *)&ga_strServer);
  _pShell->DeclareSymbol("persistent user CTString ga_strMSLegacy;", (void *)&ga_strMSLegacy);
  _pShell->DeclareSymbol("persistent user INDEX ga_bMSLegacy;", (void *)&ga_bMSLegacy);

}

/*
 * Add the timer handler.
 */
void CNetworkLibrary::AddTimerHandler(void)
{
  ASSERT(this!=NULL);
  if (_bTempNetwork) {
    return; // this can happen during NET_MakeDefaultState_t()!
  }
  _pTimer->AddHandler(&ga_thTimerHandler);
}
/*
 * Remove the timer handler.
 */
void CNetworkLibrary::RemoveTimerHandler(void)
{
  ASSERT(this!=NULL);
  if (_bTempNetwork) {
    return; // this can happen during NET_MakeDefaultState_t()!
  }
  _pTimer->RemHandler(&ga_thTimerHandler);
}

/*
// set settings to prediction-off
void AdjustPredictionOff(void)
{
  if (!cli_bAutoAdjustSettings) {
    return;
  }
  if (cli_bPrediction) {
    CPrintF("AutoAdjustment: prediction off, buffer 1\n");
  }
  cli_bPrediction = 0;
  cli_iBufferActions = 1;
}
// set settings to prediction-on
void AdjustPredictionOn(void)
{
  if (!cli_bAutoAdjustSettings) {
    return;
  }
  if (!cli_bPrediction) {
    CPrintF("AutoAdjustment: prediction on, buffer 3\n");
  }
  cli_bPrediction = 1;
  cli_iBufferActions = 3;
}

// automatically adjust network settings
void CNetworkLibrary::AutoAdjustSettings(void)
{
  // if server and not debugging prediction
  if (IsServer() && !cli_bPredictIfServer) {
    // just turn it all off
    AdjustPredictionOff();
    return;
  }

  static TIME _tmLastTimeNoPredictionSteps = -1;
  // get network lag in terms of ticks
  INDEX ctLagTicks = ga_sesSessionState.GetPredictionStepsCount()-(cli_iBufferActions-1);

  // if no significant lag
  if (ctLagTicks<=1) {
    // set settings to prediction-off
    AdjustPredictionOff();
    _tmLastTimeNoPredictionSteps = _pTimer->CurrentTick();
  // if there is lag now for some time
  } else if (_pTimer->CurrentTick()-_tmLastTimeNoPredictionSteps>=cli_tmAutoAdjustThreshold) {
    // set settings to prediction-on
    AdjustPredictionOn();
  }
}
*/

/*
 * Start a peer-to-peer game session.
 *
 * remember to keep this routine up to date with CNetworkLibrary::Read()
 */
void CNetworkLibrary::StartPeerToPeer_t(const CTString &strSessionName,
  const CTFileName &fnmWorld, ULONG ulSpawnFlags,
  INDEX ctMaxPlayers, BOOL bWaitAllPlayers,
  void *pvSessionProperties) // throw char *
{
  // mute all sounds
  _pSound->Mute();

  // go on
  CPrintF( TRANS("Starting session: '%s'\n"), (const char *) strSessionName);
  CPrintF( TRANS("  level: '%s'\n"), (const char*) fnmWorld);
  CPrintF( TRANS("  spawnflags: %08x\n"), ulSpawnFlags);
  CPrintF( TRANS("  max players: %d\n"), ctMaxPlayers);
  CPrintF( TRANS("  waiting: %d\n"), bWaitAllPlayers);

  CGatherCRC gc;

  // if starting in network
  if (_cmiComm.IsNetworkEnabled()) {
    CPrintF( TRANS("  network is on\n"));
    // start gathering CRCs
    InitCRCGather();

    // make default state data for creating deltas
    MakeDefaultState(fnmWorld, ulSpawnFlags, pvSessionProperties);
  } else {
    CPrintF( TRANS("  network is off\n"));
  }

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);
  ga_ctTimersPending = -1;    // disable timer pending

  ga_strSessionName = strSessionName;
  ga_bLocalPause = FALSE;
  ga_sesSessionState.ses_iLevel+=1;
  ga_sesSessionState.ses_ulSpawnFlags = ulSpawnFlags;
  ga_sesSessionState.ses_tmSyncCheckFrequency = ser_tmSyncCheckFrequency;
  ga_sesSessionState.ses_iExtensiveSyncCheck = ser_iExtensiveSyncCheck;

  memcpy(ga_aubProperties, pvSessionProperties, NET_MAXSESSIONPROPERTIES);

  // remember the world filename
  ga_fnmWorld = fnmWorld;
  ga_fnmNextLevel = CTString("");
  try {
    // load the world
    _pTimer->SetCurrentTick(0.0f);  // must have timer at 0 while loading
    ga_World.Load_t(fnmWorld);
    // delete all entities that don't fit given spawn flags
    ga_World.FilterEntitiesBySpawnFlags(ga_sesSessionState.ses_ulSpawnFlags);
  } catch (const char *) {
    ga_fnmWorld = CTString("");
    _cmiComm.Server_Close();
    _cmiComm.Client_Close();
    throw;
  }
  // remember the world pointer
  _pShell->SetCurrentWorld(&ga_World);

  SetProgressDescription(TRANS("starting server"));
  CallProgressHook_t(0.0f);
  // initialize server
  try {
    ga_srvServer.Start_t();
  } catch (const char *) {
    ga_World.DeletePredictors();
    ga_World.Clear();
    throw;
  }
  ga_IsServer = TRUE;
  ga_ulDemoMinorVersion = _SE_BUILD_MINOR;
  CallProgressHook_t(1.0f);

  // start the timer loop
  AddTimerHandler();

  SetProgressDescription(TRANS("starting session"));
  CallProgressHook_t(0.0f);
  // initialize session state
  try {
    ga_sesSessionState.Start_t(-1);
  } catch (const char *strError) {
    (void)strError;
    RemoveTimerHandler();
    ga_srvServer.Stop();
    ga_World.DeletePredictors();
    ga_World.Clear();
    throw;
  }
  CallProgressHook_t(1.0f);

  // remember maximum number of players
  ga_sesSessionState.ses_ctMaxPlayers = ctMaxPlayers;
  ga_sesSessionState.ses_bWaitAllPlayers = bWaitAllPlayers;

  // time speed is normal by default
  ga_sesSessionState.ses_fRealTimeFactor = 1.0f;

  // eventually cache all shadowmaps in world (memory eater!)
  if( shd_bCacheAll) ga_World.wo_baBrushes.CacheAllShadowmaps();
  // flush stale caches
  FreeUnusedStock();
  // mark that pretouching is required
  _bNeedPretouch = TRUE;

  // start timer sync anew
  ga_ctTimersPending = 0;
  FinishCRCGather();
  CPrintF( TRANS("  started.\n"));
}

/*
 * Save the game.
 */
void CNetworkLibrary::Save_t(const CTFileName &fnmGame) // throw char *
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  // must be server
  if (!ga_IsServer) {
    throw TRANS("Cannot save game - not a server!\n");
  }

  // create the file
  CTFileStream strmFile;
  strmFile.Create_t(fnmGame);

  // write game to stream
  strmFile.WriteID_t("GAME");
  ga_sesSessionState.Write_t(&strmFile);
  strmFile.WriteID_t("GEND");   // game end
}

/*
 * Load the game.
 *
 * remember to keep this routine up to date with CNetworkLibrary::StartPeerToPeer()
 */
void CNetworkLibrary::Load_t(const CTFileName &fnmGame) // throw char *
{
  // mute all sounds
  _pSound->Mute();

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);
  ga_ctTimersPending = -1;    // disable timer pending
  CGatherCRC gc;

  ga_bLocalPause = FALSE;

  // open the file
  CTFileStream strmFile;
  strmFile.Open_t(fnmGame);

  // if starting in network
  if (_cmiComm.IsNetworkEnabled()) {
    // start gathering CRCs
    InitCRCGather();
  }

  // initialize server
  ga_srvServer.Start_t();
  ga_IsServer = TRUE;
  ga_ulDemoMinorVersion = _SE_BUILD_MINOR;
  ga_fnmNextLevel = CTString("");
  memset(ga_aubProperties, 0, NET_MAXSESSIONPROPERTIES);

  // start the timer loop
  AddTimerHandler();

  strmFile.ExpectID_t("GAME");
  // read session state
  try {
    ga_sesSessionState.Start_t(-1);
    ga_sesSessionState.Read_t(&strmFile);
    // if starting in network
    if (_cmiComm.IsNetworkEnabled()) {
      // make default state data for creating deltas
      MakeDefaultState(ga_fnmWorld, ga_sesSessionState.ses_ulSpawnFlags,
        ga_aubProperties);
    }
    // players will be connected later
    ga_sesSessionState.ses_apltPlayers.Clear();
    ga_sesSessionState.ses_apltPlayers.New(NET_MAXGAMEPLAYERS);
    strmFile.ExpectID_t("GEND");   // game end
  } catch (const char *) {
    RemoveTimerHandler();
    ga_srvServer.Stop();
    ga_IsServer = FALSE;
    throw;
  }

  // set time and pause for server from the saved game
  ga_sesSessionState.ses_iLevel+=1;
  ga_srvServer.srv_tmLastProcessedTick = ga_sesSessionState.ses_tmLastProcessedTick;
  ga_srvServer.srv_iLastProcessedSequence = ga_sesSessionState.ses_iLastProcessedSequence;
  ga_srvServer.srv_bPause = ga_sesSessionState.ses_bPause;
  ga_srvServer.srv_bGameFinished = ga_sesSessionState.ses_bGameFinished;
  ga_sesSessionState.ses_tmPredictionHeadTick = ga_sesSessionState.ses_tmLastProcessedTick;
  // start sending stream to local state
  ga_srvServer.srv_assoSessions[0].sso_bSendStream = TRUE;
  ga_srvServer.srv_assoSessions[0].sso_iLastSentSequence = ga_srvServer.srv_iLastProcessedSequence;

  // eventually cache all shadowmaps in world (memory eater!)
  if( shd_bCacheAll) ga_World.wo_baBrushes.CacheAllShadowmaps();
  // flush stale caches
  FreeUnusedStock();
  // mark that pretouching is required
  _bNeedPretouch = TRUE;

  // start timer sync anew
  ga_ctTimersPending = 0;
  FinishCRCGather();
}

/*
 * Save a debugging game.
 */
void CNetworkLibrary::DebugSave(void)
{
  // try to save game
  try {
    Save_t(CTString("Save\\Debug.sav"));
  // if not successful
  } catch (const char *strError){
    FatalError("Cannot save debug game:\n%s", strError);
  }
}

/* Enumerate existing sessions. */
void CNetworkLibrary::EnumSessions(BOOL bInternet)
{
  // clear old list
  FORDELETELIST(CNetworkSession, ns_lnNode, ga_lhEnumeratedSessions, itns) {
    delete &*itns;
  }

  // make sure network is on
  if (!_cmiComm.IsNetworkEnabled()) {
    _cmiComm.PrepareForUse(/*network*/TRUE, /*client*/FALSE); // have to enumerate as server
  }

  // request enumeration
  GameAgent_EnumTrigger(bInternet);
}

/*
 * Join a running multi-player game.
 */
void CNetworkLibrary::JoinSession_t(const CNetworkSession &nsSesssion, INDEX ctLocalPlayers) // throw char *
{
  // mute all sounds
  _pSound->Mute();

  // report session addres
  CPrintF( TRANS("Joining session at: '%s'\n"), (const char *) nsSesssion.ns_strAddress);

  ga_bLocalPause = FALSE;

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);
  ga_ctTimersPending = -1;    // disable timer pending
  // start gathering CRCs
  CGatherCRC gc;
  InitCRCGather();

  // set session name and server address
  ga_strSessionName = nsSesssion.ns_strSession;
  ga_strServerAddress = nsSesssion.ns_strAddress;
  ga_fnmNextLevel = CTString("");
  ga_fnmWorld = CTString("");
  memset(ga_aubProperties, 0, NET_MAXSESSIONPROPERTIES);

  ga_IsServer = FALSE;
  ga_ulDemoMinorVersion = _SE_BUILD_MINOR;
  // start the timer loop
  AddTimerHandler();

  SetProgressDescription(TRANS("connecting"));
  CallProgressHook_t(0.0f);
  // initialize session state
  try {
    ga_sesSessionState.Start_t(ctLocalPlayers);
  } catch (const char *) {
    RemoveTimerHandler();
    throw;
  }

  // remember the world pointer
  _pShell->SetCurrentWorld(&ga_World);

  // eventually cache all shadowmaps in world (memory eater!)
  if( shd_bCacheAll) ga_World.wo_baBrushes.CacheAllShadowmaps();
  // flush stale caches
  FreeUnusedStock();
  // mark that pretouching is required
  _bNeedPretouch = TRUE;

  // run main loop to let session state process messages from server
  MainLoop();
  // start timer sync anew
  ga_ctTimersPending = 0;

  // initially auto adjust prediction on
//  AdjustPredictionOn();
  CPrintF("  joined\n");
}

/* Start playing a demo. */
void CNetworkLibrary::StartDemoPlay_t(const CTFileName &fnDemo)  // throw char *
{
  // Fix for crashing when demo playback - start
  _bPlayDemoActive = TRUE;
  // mute all sounds
  _pSound->Mute();

  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);
  ga_ctTimersPending = -1;    // disable timer pending
  ga_bLocalPause = FALSE;

  // open the file
  ga_strmDemoPlay.Open_t(fnDemo);

  // remember that playing demo
  ga_bDemoPlay = TRUE;
  ga_bDemoPlayFinished = FALSE;

  // create session name from demo name
  CTString strSessionName = CTString("Demo: ")+fnDemo;
  ga_strSessionName = strSessionName;

  ga_IsServer = FALSE;
  // start the timer loop
  AddTimerHandler();
  // initialize server
  try {
    // read initial info to stream
    ga_strmDemoPlay.ExpectID_t("DEMO");
    if (ga_strmDemoPlay.PeekID_t()==CChunkID("MVER")) {
      ga_strmDemoPlay.ExpectID_t("MVER");
      ga_strmDemoPlay>>ga_ulDemoMinorVersion;
    } else {
      ga_ulDemoMinorVersion = 2;
    }
    ga_sesSessionState.Read_t(&ga_strmDemoPlay);
  } catch (const char *) {
    RemoveTimerHandler();
    ga_strmDemoPlay.Close();
    ga_bDemoPlay = FALSE;
    throw;
  }

  // eventually cache all shadowmaps in world (memory eater!)
  if( shd_bCacheAll) ga_World.wo_baBrushes.CacheAllShadowmaps();
  // flush stale caches
  FreeUnusedStock();
  // mark that pretouching is required
  _bNeedPretouch = TRUE;

  // remember the world pointer
  _pShell->SetCurrentWorld(&ga_World);

  // demo synchronization starts at the beginning initially
  ga_fDemoTimer = 0.0f;
  ga_tvDemoTimerLastTime = _pTimer->GetHighPrecisionTimer();

  // demo sync seuqence must be initialized first time in ProcessGameStream()
  ga_sesSessionState.ses_tmLastDemoSequence = -1.0f;

  // run main loop to let server process messages from host
  MainLoop();
  // start timer sync anew
  ga_ctTimersPending = 0;
}

/* Test if currently playing demo has finished. */
BOOL CNetworkLibrary::IsDemoPlayFinished(void)
{
  return ga_bDemoPlay && ga_bDemoPlayFinished;
}

/* Test if currently playing a demo. */
BOOL CNetworkLibrary::IsPlayingDemo(void)
{
  return ga_bDemoPlay;
}

/* Test if currently recording a demo. */
BOOL CNetworkLibrary::IsRecordingDemo(void)
{
  return ga_bDemoRec;
}
BOOL CNetworkLibrary::IsNetworkEnabled(void)
{
  return _cmiComm.IsNetworkEnabled();
}
// pause/unpause game
void CNetworkLibrary::TogglePause(void)
{
  ga_sesSessionState.ses_bWantPause = !ga_sesSessionState.ses_bWantPause;
}

// test if game is paused
BOOL CNetworkLibrary::IsPaused(void)
{
  ASSERT(this!=NULL);
  if (_bTempNetwork) {
    return TRUE; // this can happen during NET_MakeDefaultState_t()!
  }
  return ga_sesSessionState.ses_bPause;
}

// test if having connnection problems (not getting messages from server regulary)
BOOL CNetworkLibrary::IsConnectionStable(void)
{
  // if network is not enabled
  if (!_cmiComm.IsNetworkEnabled()) {
    // it is always stable
    return TRUE;
  }

  // check when last message was received.
  return (_pTimer->GetHighPrecisionTimer()-ga_sesSessionState.ses_tvMessageReceived).GetSeconds()<net_tmProblemsTimeout;
}
// test if completely disconnected and why
BOOL CNetworkLibrary::IsDisconnected(void)
{
  return ga_sesSessionState.ses_strDisconnected!="";
}

const CTString &CNetworkLibrary::WhyDisconnected(void)
{
  return ga_sesSessionState.ses_strDisconnected;
}

// set/get server side pause (for single player only)
void CNetworkLibrary::SetLocalPause(BOOL bPause)
{
  ga_bLocalPause = bPause;
}

BOOL CNetworkLibrary::GetLocalPause(void)
{
  ASSERT(this!=NULL);
  if (_bTempNetwork) {
    return TRUE; // this can happen during NET_MakeDefaultState_t()!
  }
  return ga_bLocalPause;
}

// get server/client name and address
void CNetworkLibrary::GetHostName(CTString &strName, CTString &strAddress)
{
  _cmiComm.GetHostName(strName, strAddress);
}

// mark that the game has finished -- called from AI
void CNetworkLibrary::SetGameFinished(void)
{
  ga_sesSessionState.ses_bGameFinished = TRUE;
  if (IsServer()) {
    ga_srvServer.srv_bGameFinished = TRUE;
  }
}
BOOL CNetworkLibrary::IsGameFinished(void)
{
  return ga_sesSessionState.ses_bGameFinished;
}

// manipulation with realtime factor for slower/faster time -- called from AI
void CNetworkLibrary::SetRealTimeFactor(FLOAT fSpeed)
{
  ga_sesSessionState.ses_fRealTimeFactor = fSpeed;
}

FLOAT CNetworkLibrary::GetRealTimeFactor(void)
{
  return ga_sesSessionState.ses_fRealTimeFactor;
}

// test if game is waiting for more players to connect
BOOL CNetworkLibrary::IsWaitingForPlayers(void)
{
  // if game mode does not include waiting for players
  if (!ga_sesSessionState.ses_bWaitAllPlayers) {
    // not waiting
    return FALSE;
  }
  // if server
  if (IsServer()) {
    // check number of players on server
    return ga_srvServer.GetPlayersCount()<ga_sesSessionState.ses_ctMaxPlayers;
  // if not server
  } else {
    // check number of players in session
    return ga_sesSessionState.GetPlayersCount()<ga_sesSessionState.ses_ctMaxPlayers;
  }
}
// test if game is waiting for server
BOOL CNetworkLibrary::IsWaitingForServer(void)
{
  return ga_sesSessionState.ses_bWaitingForServer;
}

// test if game session is currently doing prediction
BOOL CNetworkLibrary::IsPredicting(void)
{
  return ga_sesSessionState.ses_bPredicting;
}

/*
 * Stop currently running game.
 */
void CNetworkLibrary::StopGame(void)
{
  // mute all sounds
  _pSound->Mute();

  if (_bPlayDemoActive) {_bPlayDemoActive = FALSE;} // Fix for crashing when demo playback - stop

  CPrintF( TRANS("stopping game.\n"));
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);
  ga_ctTimersPending = -1;    // disable timer pending

  // stop demo recording if active
  StopDemoRec();

  // if playing demo
  if (ga_bDemoPlay) {
    // close the demo file
    ga_strmDemoPlay.Close();
    // remember that not playing demo
    ga_bDemoPlay = FALSE;
    ga_bDemoPlayFinished = FALSE;
  }

  // stop the timer loop
  RemoveTimerHandler();

  // stop session
  ga_sesSessionState.Stop();

  // stop server
  if (ga_IsServer) {
    ga_srvServer.Stop();
    ga_IsServer = FALSE;
  }
  ga_ulDemoMinorVersion = _SE_BUILD_MINOR;

  ga_strSessionName = "";

  ga_World.DeletePredictors();
  ga_World.Clear();

  // free default state if existing
  if (ga_pubDefaultState!=NULL) {
    FreeMemory(ga_pubDefaultState);
    ga_pubDefaultState = NULL;
    ga_slDefaultStateSize = 0;
    memset(ga_aubDefaultProperties, 0, sizeof(ga_aubDefaultProperties));
  }
  if (ga_pubCRCList!=NULL) {
    FreeMemory(ga_pubCRCList);
    ga_pubCRCList = NULL;
    ga_slCRCList = 0;
  }

  ga_aplsPlayers.Clear();
  ga_aplsPlayers.New(NET_MAXLOCALPLAYERS);
  // remember the world pointer
  _pShell->SetCurrentWorld(NULL);

  // rewind the timer
  _pTimer->SetCurrentTick(0.0f);
}

// initiate level change
void CNetworkLibrary::ChangeLevel(
  const CTFileName &fnmNextLevel, BOOL bRemember, INDEX iUserData)
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  ASSERT(!IsPredicting());

  // if not currently changing
  if (_lphCurrent==LCP_NOCHANGE) {
    // initiate change
    ga_fnmNextLevel = fnmNextLevel;
    ga_bNextRemember = bRemember;
    ga_iNextLevelUserData = iUserData;
    _lphCurrent = LCP_INITIATED;
  }
}

// really do the level change
void CNetworkLibrary::ChangeLevel_internal(void)
{
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  extern BOOL _bReinitEntitiesWhileCopying;
  _bReinitEntitiesWhileCopying = FALSE;

  // mute all sounds
  _pSound->Mute();

  // cancel all predictions before crossing levels
  _pNetwork->ga_World.DeletePredictors();

  // find all entities that are to cross to next level
  CEntitySelection senToCross;
  {FOREACHINDYNAMICCONTAINER(ga_World.wo_cenEntities, CEntity, iten) {
    if (iten->en_ulFlags&ENF_CROSSESLEVELS) {
      senToCross.Select(*iten);
    }
  }}

  // copy them to a temporary world
  CWorld wldTemp;
  CEntitySelection senInTemp;
  wldTemp.CopyEntities(ga_World, senToCross,
    senInTemp, CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0)));

  // remember characters for all player targets and disable them
  CPlayerCharacter apc[NET_MAXGAMEPLAYERS];
  BOOL abWasActive[NET_MAXGAMEPLAYERS];
  CPlayerAction apaActions[NET_MAXGAMEPLAYERS][2];
  {for(INDEX i=0; i<NET_MAXGAMEPLAYERS; i++) {
    CPlayerTarget &plt = ga_sesSessionState.ses_apltPlayers[i];
    abWasActive[i] = plt.IsActive();
    if (plt.IsActive()) {
      apc[i] = plt.plt_penPlayerEntity->en_pcCharacter;
      apaActions[i][0] = plt.plt_paLastAction;
      apaActions[i][1] = plt.plt_paPreLastAction;
      plt.plt_penPlayerEntity = NULL;
      plt.Deactivate();
    }
  }}

  // destroy all entities that will cross level
  ga_World.DestroyEntities(senToCross);

  // if should remember old levels
  if (ga_bNextRemember) {
    // remember current level
    ga_sesSessionState.RememberCurrentLevel(ga_fnmWorld);
  }

  CGatherCRC gc;

  // if starting in network
  if (_cmiComm.IsNetworkEnabled()) {
    // start gathering CRCs
    InitCRCGather();

    // make default state data for creating deltas
    MakeDefaultState(ga_fnmNextLevel, ga_sesSessionState.ses_ulSpawnFlags, ga_aubProperties);
  }

  // if the new level is not remembered
  if (ga_sesSessionState.FindRememberedLevel(ga_fnmNextLevel)==NULL) {
    // remember original world filename
    CTFileName fnmOldWorld = ga_fnmWorld;

    // try to
    try {
      // load the new world
      _pTimer->SetCurrentTick(0.0f);  // must have timer at 0 while loading
      ga_World.Load_t(ga_fnmNextLevel);
      // delete all entities that don't fit given spawn flags
      ga_World.FilterEntitiesBySpawnFlags(ga_sesSessionState.ses_ulSpawnFlags);
    // if failed
    } catch (const char *strError) {
      // report error
      CPrintF(TRANSV("Cannot change level:\n%s"), strError);
      // try to
      try {
        // load the old world
        ga_fnmNextLevel = fnmOldWorld;
        ga_World.Load_t(ga_fnmNextLevel);
        // delete all entities that don't fit given spawn flags
        ga_World.FilterEntitiesBySpawnFlags(ga_sesSessionState.ses_ulSpawnFlags);
      // if that fails
      } catch (const char *strError2) {
        // fatal error
        FatalError(
          TRANS("Cannot change level because:\n%s\n"
          "and cannot go back to original one because:\n%s"), strError, strError2);
        return;
      }
    }
    // remember the world filename
    ga_fnmWorld = ga_fnmNextLevel;
    // remember the world pointer
    _pShell->SetCurrentWorld(&ga_World);
  // if there is remembered level
  } else {
    // restore it
    ga_sesSessionState.RestoreOldLevel(ga_fnmNextLevel);
  }

  // set overdue timers in just loaded world to be due in current time
  ga_World.AdjustLateTimers(ga_sesSessionState.ses_tmLastProcessedTick);

  // copy entities from temporary world into new one
  CEntitySelection senCrossed;
  ga_World.CopyEntities(wldTemp, senInTemp,
    senCrossed, CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0)));

  // restore pointers to entities for all active player targets
  {for(INDEX i=0; i<NET_MAXGAMEPLAYERS; i++) {
    CPlayerTarget &plt = ga_sesSessionState.ses_apltPlayers[i];
    if (abWasActive[i]) {
      plt.Activate();
      plt.plt_paLastAction    = apaActions[i][0];
      plt.plt_paPreLastAction = apaActions[i][1];
      plt.AttachEntity(ga_World.FindEntityWithCharacter(apc[i]));
    }
  }}

  _bReinitEntitiesWhileCopying = TRUE;

  // if should not remember old levels
  if (!ga_bNextRemember) {
    // clear them all
    ga_sesSessionState.ForgetOldLevels();
  }

  // if not server
  if (!IsServer()) {
    // start waiting for server
    ga_sesSessionState.ses_bWaitingForServer = TRUE;
  // if server
  } else {
    // flush sync check buffer
    ga_srvServer.srv_ascChecks.Clear();

    // for each client
    {for( INDEX iClient=0; iClient<NET_MAXGAMECOMPUTERS; iClient++) {
      CSessionSocket &sso = ga_srvServer.srv_assoSessions[iClient];
      // reset message timer
      sso.sso_tvMessageReceived = (__int64) -1;
      // reset sync timer
      sso.sso_tmLastSyncReceived = -1.0f;
    }}
    // for each player
    {for( INDEX iPlayer=0; iPlayer<NET_MAXGAMEPLAYERS; iPlayer++) {
      CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[iPlayer];
      if (plb.plb_Active) {
        // add one dummy action
        CPlayerAction pa;
        pa.Clear();
        pa.pa_aRotation = plb.plb_paLastAction.pa_aRotation;
        pa.pa_aViewRotation = plb.plb_paLastAction.pa_aViewRotation;
        plb.plb_abReceived.AddAction(pa);
      }
    }}
  }
  ga_sesSessionState.ses_iLevel+=1;

  // flush stale caches
  FreeUnusedStock();
  // mark that pretouching is required
  _bNeedPretouch = TRUE;

  // start timer sync anew
  ga_ctTimersPending = 0;
  FinishCRCGather();
}

/* Start recording a demo. */
void CNetworkLibrary::StartDemoRec_t(const CTFileName &fnDemo) // throw char *
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  // if already recording
  if (ga_bDemoRec) {
    // error
    throw TRANS("Already recording a demo!");
  }

  // create the file
  ga_strmDemoRec.Create_t(fnDemo);

  // write initial info to stream
  ga_strmDemoRec.WriteID_t("DEMO");
  ga_strmDemoRec.WriteID_t("MVER");
  ga_strmDemoRec<<ULONG(_SE_BUILD_MINOR);
  ga_sesSessionState.Write_t(&ga_strmDemoRec);

  // remember that recording demo
  ga_bDemoRec = TRUE;
}

/* Stop recording a demo. */
void CNetworkLibrary::StopDemoRec(void)
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  // if not recording
  if (!ga_bDemoRec) {
    // do nothing
    return;
  }
  // write terminal info to the stream
  ga_strmDemoRec.WriteID_t("DEND");   // game end
  // close the file
  ga_strmDemoRec.Close();
  // remember that not recording demo
  ga_bDemoRec = FALSE;
}

// split the rcon response string into lines and send one by one to the client
static void SendAdminResponse(ULONG ulAdr, UWORD uwPort, ULONG ulCode, const CTString &strResponse)
{
  CTString str = strResponse;
  INDEX iLineCt = 0;

  while (str!="") {
    CTString strLine = str;
    strLine.OnlyFirstLine();
    str.RemovePrefix(strLine);
    str.DeleteChar(0);
    if (strLine.Length()>0) {
      CNetworkMessage nm(MSG_EXTRA);
      nm<<CTString(0, "log %u %d %s\n", ulCode, iLineCt++, (const char *) strLine);
      _pNetwork->SendBroadcast(nm, ulAdr, uwPort);
    }
  }
}

/*
 * Main loop.
 */
void CNetworkLibrary::MainLoop(void)
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  // update network state variable (to control usage of some cvars that cannot be altered in mulit-player mode)
  _bMultiPlayer = (_pNetwork->ga_sesSessionState.GetPlayersCount() > 1);

  // if should change world
  if (_lphCurrent==LCP_SIGNALLED) {
    // really do the level change here
    ChangeLevel_internal();
    _lphCurrent=LCP_CHANGED;
  }

  if (_bStartDemoRecordingNextTime) {
    _bStartDemoRecordingNextTime = 0.0f;
    if (!ga_bDemoRec) {
      try {
        CTString strName;
        strName.PrintF("Temp\\Recorded%02d.dem", (INDEX)dem_iRecordedNumber);
        StartDemoRec_t(strName);
        dem_iRecordedNumber+=1;
      } catch (const char *strError) {
        CPrintF(TRANSV("Demo recording error: %s\n"), strError);
      }
    }
  }
  if (_bStopDemoRecordingNextTime) {
    _bStopDemoRecordingNextTime = 0.0f;
    if (ga_bDemoRec) {
      StopDemoRec();
    }
  }

  _sfStats.StartTimer(CStatForm::STI_MAINLOOP);
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_MAINLOOP);

  // handle messages for session state
  if (!ga_bDemoPlay) {
    if (_cmiComm.Client_Update() == FALSE) {
      ga_sesSessionState.Stop();
      return;
    }
    ga_sesSessionState.SessionStateLoop();
    if (_cmiComm.Client_Update() == FALSE) {
      ga_sesSessionState.Stop();
      return;
    }
  }
  // if this is server computer
  if (ga_IsServer) {
    // handle server messages
    _cmiComm.Server_Update();
  }

  // let server process game stream
  TIME tmBefore = _pTimer->GetRealTimeTick();
  _pTimer->SetLerp(0.0f);


/*
  // automatically adjust network settings
  if (cli_bAutoAdjustSettings) {
    AutoAdjustSettings();
  }
  */

  // determine whether to use prediction
  BOOL bUsePrediction;  
  // Fix for crashing when demo playback
  if (_bPlayDemoActive) {
    bUsePrediction = FALSE; 
  } else {
    bUsePrediction = cli_bPrediction && (cli_bPredictIfServer || !IsServer());
  }
  _bPredictionActive = bUsePrediction;    // memeber this for other misc code
  // mark all predictable entities that will be predicted using user-set criterions
  if (bUsePrediction) {
    ga_World.MarkForPrediction();
  }
  // process the game stream coming from the server
  ga_sesSessionState.ProcessGameStream();
  // flush actions that don't need to be predicted any more
  ga_sesSessionState.FlushProcessedPredictions();
  // process additional prediction steps
  if (bUsePrediction) {
    // mark all new predictable entities that might have been spawned
    ga_World.UnmarkForPrediction();
    ga_World.MarkForPrediction();
    ga_sesSessionState.ProcessPrediction();
    // unmark all predictable entities marked for prediction
    ga_World.UnmarkForPrediction();
  }

  ga_sesSessionState.ses_tmLastUpdated = _pTimer->GetRealTimeTick();

  TIME tmAfter = _pTimer->GetRealTimeTick();
  ga_sesSessionState.ses_bKeepingUpWithTime = (tmAfter-tmBefore)<=_pTimer->TickQuantum*2.01f;

  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  // set the lerping factor for current frame
  if (!ga_bDemoPlay) {
    ga_sesSessionState.SetLerpFactor(tvNow);
  } else {
    ga_sesSessionState.SetLerpFactor(CTimerValue(ga_fDemoTimer));
  }

  // if playing a demo
  if (ga_bDemoPlay) {
    // if synchronizing by real time
    if (ga_fDemoSyncRate==DEMOSYNC_REALTIME) {
      // if server is keeping up
      if (ga_sesSessionState.ses_bKeepingUpWithTime) {
        // add passed time with slow/fast factor
        ga_fDemoTimer += FLOAT((tvNow-ga_tvDemoTimerLastTime).GetSeconds())
          *ga_fDemoRealTimeFactor*ga_sesSessionState.ses_fRealTimeFactor;
      }
    // if synchronizing is stopped
    } else if (ga_fDemoSyncRate==DEMOSYNC_STOP) {
      // don't step
      NOTHING;
    // if synchronizing by given steps
    } else {
      // just add the step
      ga_fDemoTimer += 1.0f/ga_fDemoSyncRate;
    }
  }
  // remember the demo timer
  ga_tvDemoTimerLastTime = tvNow;

  // if network
  if (_cmiComm.IsNetworkEnabled()) {

    // do services for gameagent querying
    GameAgent_ServerUpdate();

//    _cmiComm.Broadcast_Update();

    // repeat
    FOREVER {
      CNetworkMessage nmReceived;

//      _cmiComm.Broadcast_Update();
      ULONG ulFrom;
      UWORD uwPort;
      BOOL bHasMsg = ReceiveBroadcast(nmReceived, ulFrom, uwPort);
      // if there are no more messages
      if (!bHasMsg) {
        // finish
        break;
      }

      // if this message is not valid rcon message
      if (nmReceived.GetType()!=MSG_EXTRA) {
        // skip it
        continue;
      }
      // get the string from the message
      CTString strMsg;
      nmReceived>>strMsg;

      // if this is server
      if (IsServer()) {
        // accept requests
        if (!strMsg.RemovePrefix("rcmd ")) {
          continue;
        }
        ULONG ulCode;
        char strPass[80];
        char strCmd[256];
        strMsg.ScanF("%u \"%80[^\"]\"%256[^\n]", &ulCode, strPass, strCmd);
        CTString strAdr = AddressToString(ulFrom);

        if (net_strAdminPassword=="" || net_strAdminPassword!=strPass) {
          CPrintF(TRANSV("Server: Client '%s', Wrong password for remote administration.\n"), (const char*)strAdr);
          continue;
        }

        CPrintF(TRANSV("Server: Client '%s', Admin cmd: %s\n"), (const char*)strAdr, strCmd);

        con_bCapture = TRUE;
        con_strCapture = "";
        _pShell->Execute(CTString(strCmd)+";");

        CTString strResponse = CTString(">")+strCmd+"\n"+con_strCapture;
        SendAdminResponse(ulFrom, uwPort, ulCode, strResponse);
        con_bCapture = FALSE;
        con_strCapture = "";
      }
    }
  }

  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_MAINLOOP);
  _sfStats.StopTimer(CStatForm::STI_MAINLOOP);
}

// make actions packet for local players and send to server
void CNetworkLibrary::SendActionsToServer(void)
{
  // make the packet
  CNetworkMessage nmAction(MSG_ACTION);

  // for all local players on this machine
  for(INDEX ipls=0; ipls<ga_aplsPlayers.Count(); ipls++) {
    CPlayerSource &pls = ga_aplsPlayers[ipls];
    // create action packet if the player exists
    pls.WriteActionPacket(nmAction);
  }
  // send the packet
  SendToServer(nmAction);
}

/*
 * Client loop.
 */
void CNetworkLibrary::TimerLoop(void)
{
  ASSERT(this!=NULL);
  if (_bTempNetwork) {
    return; // this can happen during NET_MakeDefaultState_t()!
  }
  _pfNetworkProfile.StartTimer(CNetworkProfile::PTI_TIMERLOOP);

  // count number of timer interrupts that happened
  if (ga_ctTimersPending>=0) {
    ga_ctTimersPending++;
  }

  // if can synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, FALSE);
  // initially, no timer functions needed
  INDEX ct = 0;
  // if timer exactness level is full
  if (net_iExactTimer==2) {
    // lock network mutex
    slNetwork.Lock();
    // execute exactly one
    ct = 1;
  // if timer exactness level is partial
  } else if (net_iExactTimer==1) {
    // if network mutex can be locked
    if (slNetwork.TryToLock()) {
      // execute all pending
      ct = ga_ctTimersPending;
    }
  // if timer exactness level is low
  } else if (net_iExactTimer==0) {
    // if network mutex can be locked
    if (slNetwork.TryToLock()) {
      // execute exactly one
      ct = 1;
    }
  }
  // for each pending interrupt
  while(ct) {
    ct--;
    ga_ctTimersPending--;
    // if not disconnected
//    if (!IsDisconnected()) {
    if (_cmiComm.cci_bClientInitialized) {
      // make actions packet for all local players and send to server
      SendActionsToServer();
      _cmiComm.Client_Update();
    }

    // if this is server computer
    if (ga_IsServer) {
      // handle server messages
      _cmiComm.Server_Update();
      ga_srvServer.ServerLoop();
      _cmiComm.Server_Update();
    }
  }

  _pfNetworkProfile.StopTimer(CNetworkProfile::PTI_TIMERLOOP);
}

/* Get player entity for a given local player. */
CEntity *CNetworkLibrary::GetLocalPlayerEntity(CPlayerSource *ppls)
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  // get the index of the player target in game state
  INDEX iPlayerTarget = ppls->pls_Index;
  // if player is not added
  if (iPlayerTarget<0) {
    // no entity
    return NULL;
  // if player is added
  } else {
    // get the entity from player target
    CPlayerTarget &plt = ga_sesSessionState.ses_apltPlayers[iPlayerTarget];
    CPlayerEntity *pen = plt.plt_penPlayerEntity;
    return pen;
  }
}

/* Get player entity for a given player by name. */
CEntity *CNetworkLibrary::GetPlayerEntityByName(const CTString &strName)
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  // for each player in game
  CStaticArray<CPlayerTarget> &aplt = ga_sesSessionState.ses_apltPlayers;
  for(INDEX iplt = 0; iplt<aplt.Count(); iplt++) {
    // if it is active and has that name
    if (aplt[iplt].IsActive()
      &&aplt[iplt].plt_penPlayerEntity->en_pcCharacter.GetName()==strName) {
      // return it
      return aplt[iplt].plt_penPlayerEntity;
    }
  }
  // else not found
  return NULL;
}

/* Get number of entities with given name. */
INDEX CNetworkLibrary::GetNumberOfEntitiesWithName(const CTString &strName)
{
  INDEX ctEntities = 0;
  {FOREACHINDYNAMICCONTAINER(ga_World.wo_cenEntities, CEntity, iten) {
    if (iten->GetName()==strName) {
      ctEntities++;
    }
  }}
  return ctEntities;
}

/* Get n-th entity with given name. */
CEntity *CNetworkLibrary::GetEntityWithName(const CTString &strName, INDEX iEntityWithThatName)
{
  INDEX ctEntities = 0;
  CEntity *pen = NULL;
  {FOREACHINDYNAMICCONTAINER(ga_World.wo_cenEntities, CEntity, iten) {
    if (iten->GetName()==strName) {
      pen = iten;
      if (ctEntities==iEntityWithThatName) {
        break;
      }
      ctEntities++;
    }
  }}
  return pen;
}
/* Test if a given player is local to this computer. */
BOOL CNetworkLibrary::IsPlayerLocal(CEntity *pen)
{
  return GetPlayerSource(pen)!=NULL;
}
// get player source for a given player if it is local to this computer
CPlayerSource *CNetworkLibrary::GetPlayerSource(CEntity *pen)
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  // for all local player on this machine
  {FOREACHINSTATICARRAY(ga_aplsPlayers, CPlayerSource, itpls) {
    // get the index of the player target in game state
    INDEX iPlayerTarget = itpls->pls_Index;
    // if player is added
    if (iPlayerTarget>=0) {
      // get the player target
      CPlayerTarget &plt = ga_sesSessionState.ses_apltPlayers[iPlayerTarget];
      // if it is that one
      if (plt.plt_penPlayerEntity == pen) {
        // return it
        return itpls;
      }
    }
  }}

  // if not found, it is not local
  return NULL;
}

// get game time in currently running game
TIME CNetworkLibrary::GetGameTime(void)
{
  return ga_sesSessionState.ses_tmLastProcessedTick;
}

/*
 * Add a new client to game.
 */
CPlayerSource *CNetworkLibrary::AddPlayer_t(CPlayerCharacter &pcCharacter)  // throw char *
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);
  CPrintF( TRANS("Adding player: '%s'\n"), (const char *) pcCharacter.GetNameForPrinting());

  // for all local clients on this machine
  FOREACHINSTATICARRAY(ga_aplsPlayers, CPlayerSource, itcls) {
    // if client is not active
    if (!itcls->IsActive()) {
      // activate it
      itcls->Start_t(pcCharacter);
      CPrintF( TRANS("  done.\n"));
      return &itcls.Current();
    }
  }
  // number of local clients is limited with NET_MAXLOCALCLIENTS
  ASSERTALWAYS("Adding too much local clients!");
  throw TRANS("Cannot add more local clients");
  return NULL;
}

/* Get session properties for current game. */
void *CNetworkLibrary::GetSessionProperties(void)
{
  // synchronize access to network
  CTSingleLock slNetwork(&ga_csNetwork, TRUE);

  return ga_aubProperties;
}

/* Send chat message from some players to some other players. */
void CNetworkLibrary::SendChat(ULONG ulFrom, ULONG ulTo, const CTString &strMessage)
{
  // if the string is too long and we're not server
  if (strlen(strMessage)>256 && !_pNetwork->IsServer()) {
    // refuse it
    return;
  }

  // just make the message and send it to server
  CNetworkMessage nm(MSG_CHAT_IN);
  nm<<ulFrom;
  nm<<ulTo;
  nm<<strMessage;
  SendToServer(nm);
}

// save current version of engine
void CNetworkLibrary::WriteVersion_t(CTStream &strm)
{
  strm.WriteID_t("BUIV"); // build version
  strm<<INDEX(_SE_BUILD_MAJOR);
}
// load version of engine saved in file and check against current
void CNetworkLibrary::CheckVersion_t(CTStream &strm, BOOL bAllowReinit, BOOL &bNeedsReinit)
{
  // if not saved
  if (strm.PeekID_t()!=CChunkID("BUIV")) { // build version
    // behave as if everything is ok (for old versions)
    bNeedsReinit = FALSE;
    return;
  }
  strm.ExpectID_t("BUIV"); // build version
  // read the saved one
  INDEX iSaved;
  strm>>iSaved;
  // get current one
  INDEX iCurrent = _SE_BUILD_MAJOR;
  // if current version is an internal build
  if (iCurrent==0) {
    // it is never forced to reinit
    bNeedsReinit = FALSE;
    return;
  }

  // if current version is older than the saved one
  if (iCurrent<iSaved) {
    // it cannot be reinitialized
    ThrowF_t(TRANS("File '%s' was saved by a newer version of engine, it cannot be loaded"),
      (const char *) strm.GetDescription());
    return;
  }

  // if current version is same as the saved one
  if (iCurrent==iSaved) {
    // all ok
    bNeedsReinit = FALSE;
    return;
  }

  // if current version is newer than the saved one
  if (iCurrent>iSaved) {
    // it should be reinitialized
    bNeedsReinit = TRUE;

    // if it may not be reinitialized
    if (!bAllowReinit) {
      ThrowF_t(TRANS("File '%s' was saved by an older version of engine, it cannot be loaded"),
        (const char *) strm.GetDescription());
    }
    return;
  }
  // this should not happen
  ASSERT(FALSE);
  bNeedsReinit = FALSE;
  return;
}

// add a value to the netgraph
void CNetworkLibrary::AddNetGraphValue(enum NetGraphEntryType nget, FLOAT fLatency)
{
  net_iGraphBuffer = Clamp(net_iGraphBuffer, INDEX(20), INDEX(1000));

  // make sure the netgraph has wanted number of values
  if (ga_angeNetGraph.Count()!=net_iGraphBuffer) {
    ga_angeNetGraph.Clear();
    ga_angeNetGraph.New(net_iGraphBuffer);
    memset(&ga_angeNetGraph[0], 0, ga_angeNetGraph.Count()*sizeof(ga_angeNetGraph[0]));
  }

  // scroll the values in the netgraph by one value
  memmove(&ga_angeNetGraph[1], &ga_angeNetGraph[0], (ga_angeNetGraph.Count()-1)*sizeof(ga_angeNetGraph[0]));

  // add the new value
  ga_angeNetGraph[0].nge_ngetType = nget;
  ga_angeNetGraph[0].nge_fLatency = fLatency;
}

// make default state for a network game
extern void NET_MakeDefaultState_t(
  const CTFileName &fnmWorld, ULONG ulSpawnFlags, void *pvSessionProperties,
  CTStream &strmState) // throw char *
{
  // mute all sounds
  _pSound->Mute();

  // first off - mark that we are in the special state
  _bTempNetwork = TRUE;

  // make sure that current network object gets locked
  CTSingleLock slNetwork(&_pNetwork->ga_csNetwork, TRUE);

  // remember original network pointer and clear it
  CNetworkLibrary *pnlOld = _pNetwork;
  _pNetwork = NULL;

  // try to
  try {
    // create new network object
    CNetworkLibrary *pNewNet = new CNetworkLibrary;
    // it must have new mutex index since both will be locked
    pNewNet->ga_csNetwork.cs_iIndex = 2001;

    // lock the new network access
    CTSingleLock slNetwork(&pNewNet->ga_csNetwork, TRUE);
    pNewNet->ga_ctTimersPending = -1;    // disable timer pending
    // only after locking it, we may allow the new pointer to be remembered
    // otherwise, the other thread can jump in between
    _pNetwork = pNewNet;

    // remember settings
    _pNetwork->ga_sesSessionState.ses_ulSpawnFlags = ulSpawnFlags;
    _pNetwork->ga_sesSessionState.ses_tmSyncCheckFrequency = 10.0f;
    _pNetwork->ga_sesSessionState.ses_iExtensiveSyncCheck = 0;
    memcpy(_pNetwork->ga_aubProperties, pvSessionProperties, NET_MAXSESSIONPROPERTIES);
    _pNetwork->ga_fnmWorld = fnmWorld;
    _pNetwork->ga_fnmNextLevel = CTString("");

    try {
      // load the world
      _pTimer->SetCurrentTick(0.0f);  // must have timer at 0 while loading
      _pNetwork->ga_World.Load_t(fnmWorld);
      // delete all entities that don't fit given spawn flags
      _pNetwork->ga_World.FilterEntitiesBySpawnFlags(_pNetwork->ga_sesSessionState.ses_ulSpawnFlags);
    } catch (const char *) {
      throw;
    }
    // remember the world filename
    _pNetwork->ga_fnmWorld = fnmWorld;
    _pNetwork->ga_fnmNextLevel = CTString("");
    // remember the world pointer
    _pShell->SetCurrentWorld(&_pNetwork->ga_World);

    // reset random number generator
    _pNetwork->ga_sesSessionState.ResetRND();
    // flush stale caches
    FreeUnusedStock();

    // warmup the world
    _pNetwork->ga_sesSessionState.WarmUpWorld();

    // save the session state to the stream
    _pNetwork->ga_sesSessionState.Write_t(&strmState);

  // if any error
  } catch (const char *) {
    // restore original network pointer
    CNetworkLibrary *pnlTemp = _pNetwork;
    _pNetwork = pnlOld;
    if (pnlTemp!=NULL) {
      delete pnlTemp;
    }
    _bTempNetwork = FALSE;
    // fail
    throw;
  }

  // restore original network pointer
  CNetworkLibrary *pnlTemp = _pNetwork;
  _pNetwork = pnlOld;
  delete pnlTemp;
  _bTempNetwork = FALSE;
}

// handle broadcast messages (server enumeration)
void CNetworkLibrary::GameInactive(void)
{
  GameAgent_EnumUpdate();

  // if no network
  if (!_cmiComm.IsNetworkEnabled()) {
    // do not handle
    return;
  }

//  _cmiComm.Broadcast_Update();

  // repeat
  FOREVER {
    CNetworkMessage nmReceived;

//  _cmiComm.Broadcast_Update();
    ULONG ulFrom;
    UWORD uwPort;
    BOOL bHasMsg = ReceiveBroadcast(nmReceived, ulFrom, uwPort);
    // if there are no more messages
    if (!bHasMsg) {
      // finish
      break;
    }

    /* This is handled by GameAgent. 

    // if requesting enumeration and this is server and enumeration is allowed
    if (nmReceived.GetType()==MSG_REQ_ENUMSERVERS
      && IsServer()
      && (ser_bEnumeration && ga_sesSessionState.ses_ctMaxPlayers>1)) {
      // create response
      CNetworkMessage nmEnum(MSG_SERVERINFO);
      nmEnum<<ga_strSessionName;
      nmEnum<<ga_World.wo_strName;
      nmEnum<<ga_srvServer.GetPlayersCount();
      nmEnum<<ga_sesSessionState.ses_ctMaxPlayers;

      // send it
      SendBroadcast(nmEnum, ulFrom, uwPort);
    // if receiving enumeration
    } else if (nmReceived.GetType()==MSG_SERVERINFO) {
      // create a new session
      CNetworkSession &ns = *new CNetworkSession;
      ga_lhEnumeratedSessions.AddTail(ns.ns_lnNode);
      // read it
      nmReceived>>ns.ns_strSession;
      nmReceived>>ns.ns_strWorld;
      nmReceived>>ns.ns_ctPlayers;
      nmReceived>>ns.ns_ctMaxPlayers;
      ns.ns_strAddress = AddressToString(ulFrom);
    }*/
  }
}

void CNetworkLibrary::InitCRCGather(void)
{
  CRCT_ResetActiveList();
  CRCT_bGatherCRCs = TRUE;
  CRCT_AddFile_t(CTString("Classes\\Player.ecl"));
}

// finish gathering of file CRCs to CRC table (call for server only!)
void CNetworkLibrary::FinishCRCGather(void)
{
  try {
    // make the list
    CTMemoryStream strmCRC;
    CRCT_MakeFileList_t(strmCRC);

    // remember it
    strmCRC.SetPos_t(0);
    ga_slCRCList = strmCRC.GetStreamSize();
    ga_pubCRCList = (UBYTE*)AllocMemory(ga_slCRCList);
    strmCRC.Read_t(ga_pubCRCList, ga_slCRCList);

    // remember its CRC
    strmCRC.SetPos_t(0);
    ga_ulCRC = CRCT_MakeCRCForFiles_t(strmCRC);

  } catch (const char *strError) {
    CPrintF(TRANSV("Warning, cannot get CRCs: %s\n"), strError);
  }
}

// make default state data for creating deltas
void CNetworkLibrary::MakeDefaultState(const CTFileName &fnmWorld,
  ULONG ulSpawnFlags, void *pvSessionProperties)
{
  // prepare file or memory stream for state
  CTFileStream strmStateFile; CTMemoryStream strmStateMem;
  CTStream *pstrmState;
  extern INDEX net_bDumpConnectionInfo;
  if (net_bDumpConnectionInfo) {
    strmStateFile.Create_t(CTString("Temp\\DefaultState.bin"));
    pstrmState = &strmStateFile;
  } else {
    pstrmState = &strmStateMem;
  }
  // make default state for a network game
  NET_MakeDefaultState_t(fnmWorld, ulSpawnFlags, pvSessionProperties, *pstrmState);
  pstrmState->SetPos_t(0);

  ga_slDefaultStateSize = pstrmState->GetStreamSize();
  ga_pubDefaultState = (UBYTE*)AllocMemory(ga_slDefaultStateSize);
  pstrmState->Read_t(ga_pubDefaultState, ga_slDefaultStateSize);
  memcpy(ga_aubDefaultProperties, pvSessionProperties, sizeof(ga_aubDefaultProperties));
}
