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
#include <Engine/Base/CTString.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/CommunicationInterface.h>

static void (*_pLoadingHook_t)(CProgressHookInfo *pgli) = NULL;  // hook for loading/connecting
static CProgressHookInfo _phiLoadingInfo; // info passed to the hook
BOOL _bRunNetUpdates = FALSE;
static CTimerValue tvLastUpdate;
static BOOL  bTimeInitialized = FALSE;
extern FLOAT net_fSendRetryWait;

// set hook for loading/connecting
void SetProgressHook(void (*pHook)(CProgressHookInfo *pgli))
{
  _pLoadingHook_t = pHook;
}
// call loading/connecting hook
void SetProgressDescription(const CTString &strDescription)
{
  _phiLoadingInfo.phi_strDescription = strDescription;
}

#ifdef _DIII4A //karin: sync game state
extern void setSeriousState(int state);
#endif
void CallProgressHook_t(FLOAT fCompleted)
{
#ifdef _DIII4A //karin: sync game state
	setSeriousState(0); // GS_LOADING
#endif
  if (_pLoadingHook_t!=NULL) {
    _phiLoadingInfo.phi_fCompleted = fCompleted;
    _pLoadingHook_t(&_phiLoadingInfo);

 
    if (!bTimeInitialized) {
      tvLastUpdate = _pTimer->GetHighPrecisionTimer();
      bTimeInitialized = TRUE;
    }
    CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
    if ((tvNow-tvLastUpdate) > CTimerValue(net_fSendRetryWait*1.1f)) {
		  if (_pNetwork->ga_IsServer) {
        // handle server messages
        _cmiComm.Server_Update();
		  } else {
			  // handle client messages
			  _cmiComm.Client_Update();
		  }
      tvLastUpdate = _pTimer->GetHighPrecisionTimer();
    }    

  }
}

