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

238
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/BackgroundViewer.h"
%}

%{
#define CT_LINESONSCREEN 18 // this number must be fixed due to desinchronisation in different resolutions
static CStaticStackArray<CTString> _astrCredits;
static CTFileName _fnLastLoaded;
%}

class CScrollHolder: CRationalEntity {
name      "ScrollHolder";
thumbnail "Thumbnails\\ScrollHolder.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:

  1 CTString m_strName "Name" 'N' = "Scroll holder",
  2 CTString m_strDescription = "",
  3 CTFileName m_fnmMessage  "Scroll Text" 'T' = CTString(""),
  4 FLOAT m_fMyTimer = 0.0f,                                        // time when started
  6 FLOAT m_fMyTimerLast = 0.0f,
  5 FLOAT m_fSpeed = 1.0f,
 15 CEntityPointer m_penEndCreditsTrigger "EndScroll trigger",
 
  20 BOOL m_bDataError = FALSE,
  {
    BOOL  bDataLoaded;
  }


components:
  1 model   MODEL_MARKER     "Models\\Editor\\MessageHolder.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\MessageHolder.tex"

functions:
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("%s", (const char *) m_fnmMessage.FileName());
    return m_strDescription;
  }

  void CScrollHolder(void) 
  {
    bDataLoaded = FALSE;
  }

  BOOL ReloadData(void)
  {
    m_bDataError = FALSE;
    if (!Credits_On(m_fnmMessage))
    {
      Credits_Off();
      return FALSE;
    }    
    return TRUE;
  }

  BOOL LoadOneFile(const CTFileName &fnm)
  {
    if(fnm=="") { return FALSE; }
    try 
    {
      // open the file
      CTFileStream strm;
      strm.Open_t(fnm);

      // count number of lines
      INDEX ctLines = 0;
      while(!strm.AtEOF())
      {
        CTString strLine;
        strm.GetLine_t(strLine);
        ctLines++;
      }
      strm.SetPos_t(0);

      // allocate that much
      CTString *astr = _astrCredits.Push(ctLines);
      // load all lines
      for(INDEX iLine = 0; iLine<ctLines && !strm.AtEOF(); iLine++)
      {
        strm.GetLine_t(astr[iLine]);
      }
      strm.Close();
      return TRUE;
    }
    catch ( const char *strError)
    {
      CPrintF("%s\n", (const char *)strError);
      return FALSE;
    }
    _fnLastLoaded=fnm;
  }

  // turn credits on
  BOOL Credits_On(CTFileName fnScrollText)
  {
    _astrCredits.PopAll();
    return LoadOneFile(fnScrollText);
  }

  // turn credits off
  void Credits_Off(void)
  {
    _astrCredits.Clear();
  }

  // render credits to given drawport
  FLOAT Credits_Render(CScrollHolder *penThis, CDrawPort *pdp)
  {
    if (m_bDataError) { return 0; }
    
    if (!bDataLoaded) {
      if (!ReloadData()) {
        m_bDataError = TRUE;
        return 0;
      }
      bDataLoaded = TRUE;
      return 1;
    }
        
    PIX pixW = 0;
    PIX pixH = 0;
    PIX pixJ = 0;
    FLOAT fResolutionScaling;
    PIX pixLineHeight;
    CTString strEmpty;

    FLOAT fTime = Lerp(m_fMyTimerLast, m_fMyTimer, _pTimer->GetLerpFactor());
    CDrawPort *pdpCurr=pdp;

    pdp->Unlock();
    pdpCurr->Lock();
    
  
    pixW = pdpCurr->GetWidth();
    pixH = pdpCurr->GetHeight();
    fResolutionScaling = (FLOAT)pixH / 360.0f;
    pdpCurr->SetFont( _pfdDisplayFont);
    pixLineHeight = (PIX) floor(20*fResolutionScaling);

    const FLOAT fLinesPerSecond = penThis->m_fSpeed;
    FLOAT fOffset = fTime*fLinesPerSecond;
    INDEX ctLinesOnScreen = pixH/pixLineHeight;
    INDEX iLine1 = (INDEX) fOffset;

    pixJ = (PIX) (iLine1*pixLineHeight-fOffset*pixLineHeight);
    iLine1-=ctLinesOnScreen;

    INDEX ctLines = _astrCredits.Count();
    BOOL bOver = TRUE;

    for (INDEX i = iLine1; i<iLine1+ctLinesOnScreen+1; i++) {
      CTString *pstr = &strEmpty;
      INDEX iLine = i;
      if (iLine>=0 && iLine<ctLines) {
        pstr = &_astrCredits[iLine];
        bOver = FALSE;
      }
      pdp->SetFont( _pfdDisplayFont);
      pdp->SetTextScaling( fResolutionScaling);
      pdp->SetTextAspect( 1.0f);
      pdp->PutTextC( *pstr, pixW/2, pixJ, C_WHITE|255);
      pixJ+=pixLineHeight;
    }

    pdpCurr->Unlock();
    pdp->Lock();

    if (bOver) {
      return 0;
    } else if (ctLines-iLine1<ctLinesOnScreen) {
      return FLOAT(ctLines-iLine1)/ctLinesOnScreen;
    } else {
      return 1;
    }
  }


procedures:
  WaitScrollingToEnd()
  {
    while (m_fMyTimer<(_astrCredits.Count()+CT_LINESONSCREEN)*m_fSpeed)
    {
      autowait(_pTimer->TickQuantum);
      m_fMyTimerLast = m_fMyTimer;
      m_fMyTimer+=_pTimer->TickQuantum/_pNetwork->GetRealTimeFactor();
    }
    return EStop();
  }

  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    autowait(0.05f);
    
    if( !Credits_On(m_fnmMessage))
    {
      Credits_Off();
      return;
    }
    m_bDataError = FALSE;

    wait() {
      on (EStart eStart):
      {
        CWorldSettingsController *pwsc = GetWSC(this);
        if( pwsc!=NULL)
        {
          m_fMyTimer = 0;
          m_fMyTimerLast = 0;
          EScroll escr;
          escr.bStart=TRUE;
          escr.penSender=this;
          pwsc->SendEvent(escr);
        }
        call WaitScrollingToEnd();
      }
      on (EStop eStop): 
      {
        CWorldSettingsController *pwsc = GetWSC(this);
        if( pwsc!=NULL)
        {
          EScroll escr;
          escr.bStart=FALSE;
          escr.penSender=this;
          pwsc->SendEvent(escr);
        }
        stop;
      }
    }
    Credits_Off();
    if (m_penEndCreditsTrigger) {
      SendToTarget(m_penEndCreditsTrigger, EET_TRIGGER, FixupCausedToPlayer(this, NULL, FALSE));
    }
    return;
  }
};

