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

240
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/BackgroundViewer.h"
%}

%{

#define CTA_LEFT 1
#define CTA_RIGHT 2
#define CTA_CENTER 3

class CCreditEntry {
public:
  CTString strTitle;
  CTString strName;
  CTString strQuote;
  INDEX    iAlign;
  INDEX    iX, iY;
  FLOAT    fRelSize;
  FLOAT    fWait;
};
  
static CStaticStackArray<CCreditEntry> _acceEntries;
#define BLANK_TIME 1.0f
%}

class CCreditsHolder: CRationalEntity {
name      "CreditsHolder";
thumbnail "Thumbnails\\ScrollHolder.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:

  1 CTString m_strName "Name" 'N' = "Credits holder",
  2 CTString m_strDescription = "",
  3 CTFileName m_fnmMessage  "Scroll Text" 'T' = CTString(""),
  4 FLOAT m_fMyTimer = 0.0f,                                        // time when started
  6 FLOAT m_fMyTimerLast = 0.0f,
  5 FLOAT m_iTotalEntries = 0,
 10 BOOL m_bEnd = FALSE,  // set to TRUE to end
 15 CEntityPointer m_penEndCreditsTrigger "EndCredits trigger",

 20 BOOL m_bDataError = FALSE,

  {
    BOOL      bDataLoaded;
    CFontData _fdMedium;
  }

components:
  1 model   MODEL_HOLDER     "Models\\Editor\\MessageHolder.mdl",
  2 texture TEXTURE_HOLDER   "Models\\Editor\\MessageHolder.tex"

functions:
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("%s", (const char *) m_fnmMessage.FileName());
    return m_strDescription;
  }
  
  void CCreditsHolder(void) 
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
    if(!LoadFont())
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

      CTString strCheck;
      strm.GetLine_t(strCheck);
      strCheck.TrimSpacesRight();
      if (strCheck!="CREDITS") { return FALSE; };
      
      m_iTotalEntries = 0;
      while(!strm.AtEOF())
      {
        CTString strLine;
        CTString strArgs;
        CTString strTmp;
        CCreditEntry cceEntry;

        strm.GetLine_t(strLine);
        strm.GetLine_t(strLine);
        strLine.TrimSpacesRight();
        if (strLine=="END") {
          strm.Close();
          return TRUE;
        } else if (strLine!="ENTRY") {
          _acceEntries.PopAll();
          return FALSE;
        }
        strm.GetLine_t(strArgs);
        strArgs.ScanF("%d,%d", &cceEntry.iX, &cceEntry.iY);
        strm.GetLine_t(strArgs);
        strArgs.ScanF("%f", &cceEntry.fRelSize);
        strm.GetLine_t(strArgs);
        strArgs.TrimSpacesRight();
        if (strArgs=="CENTER") { cceEntry.iAlign = CTA_CENTER; }
        else if (strArgs=="RIGHT") { cceEntry.iAlign = CTA_RIGHT; }
        else if (TRUE) { cceEntry.iAlign = CTA_LEFT; }
        strm.GetLine_t(cceEntry.strTitle);
        strm.GetLine_t(cceEntry.strName);
        strm.GetLine_t(cceEntry.strQuote);
        strm.GetLine_t(strArgs);
        strArgs.ScanF("%f", &cceEntry.fWait);
        _acceEntries.Push() = cceEntry;
        m_iTotalEntries ++;
      }

      strm.Close();
      return TRUE;
    }
    catch ( const char *strError)
    {
      CPrintF("%s\n", (const char *)strError);
      return FALSE;
    }
  }

  BOOL LoadFont()
  {
    try 
    {
      _fdMedium.Load_t( CTFILENAME( "Fonts\\Display3-normal.fnt"));
    }
    catch ( const char *strError)
    {
      CPrintF("%s\n", (const char *)strError);
      return FALSE;
    }
    return TRUE;
  }

  // turn credits on
  BOOL Credits_On(CTFileName fnCreditsText)
  {
    _acceEntries.PopAll();
    return LoadOneFile(fnCreditsText);
  }

  // turn credits off
  void Credits_Off(void)
  {
    _acceEntries.Clear();
  }

  // render credits to given drawport
  FLOAT Credits_Render(CCreditsHolder *penThis, CDrawPort *pdp)
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
    
    //PIX pixW = 0;
    PIX pixH = 0;
    //FLOAT fResolutionScaling;
    CTString strEmpty;

    FLOAT fTime = Lerp(m_fMyTimerLast, m_fMyTimer, _pTimer->GetLerpFactor());
    CDrawPort *pdpCurr=pdp;

    pdp->Unlock();
    pdpCurr->Lock();
    
    //pixW = pdpCurr->GetWidth();
    pixH = pdpCurr->GetHeight();

    FLOAT fResFactor = pixH/480.0f;
    //fResolutionScaling = (FLOAT)pixH / 360.0f;
    pdpCurr->SetFont( _pfdDisplayFont);
    //PIX pixLineHeight = floor(20*fResolutionScaling);     

    BOOL bOver = FALSE;
  
    FLOAT fPassed = 0.0f;
    FLOAT fStart = 0.0f;
    INDEX iNextItem = 0;
    // go to active items
    for (INDEX i = 0; i<_acceEntries.Count(); i++) {
      if (_acceEntries[i].fWait!=0.0f) {
        fPassed += _acceEntries[i].fWait;
        if (fPassed>fTime) {
          break;
        } else {
          iNextItem = i+1;
          fStart = fPassed;
          if (iNextItem>=_acceEntries.Count())
          {
            bOver = TRUE;    
          }
        }
      }      
    }
//CPrintF("start:%f, passed:%f, time:%f\n", fStart, fPassed, fTime);    
    if (!bOver) {
      while (TRUE) {
        BOOL bLast = FALSE;
        if (_acceEntries[iNextItem].fWait!=0.0f) { bLast = TRUE; };
        FLOAT fFade = CalculateRatio(fTime, fStart, fPassed-BLANK_TIME, 0.2f, 0.2f);
        pdp->SetFont( _pfdDisplayFont);
        pdp->SetTextAspect( 1.0f);

        FLOAT fTextSize01 = 1.2f*_acceEntries[iNextItem].fRelSize;
        FLOAT fTextSize02 = 2.0f*_acceEntries[iNextItem].fRelSize;
        FLOAT fTextSize03 = 0.75f*_acceEntries[iNextItem].fRelSize;
        FLOAT fTextHeight = 15.0f;
        FLOAT fSpacing01 = 1.2f;
        FLOAT fSpacing02 = 1.1f;

        if (_acceEntries[iNextItem].iAlign == CTA_CENTER) {
          FLOAT fYY = _acceEntries[iNextItem].iY*fResFactor;
          
          pdp->SetTextScaling(fTextSize01*fResFactor);
          pdp->PutTextC( _acceEntries[iNextItem].strTitle,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));
          fYY += fTextSize01*fResFactor*fTextHeight*fSpacing01;
          
          pdp->SetFont( &_fdMedium);
          pdp->SetTextScaling(fTextSize02*fResFactor);
          pdp->PutTextC( _acceEntries[iNextItem].strName,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));
          fYY += fTextSize02*fResFactor*fTextHeight*fSpacing02;

          pdp->SetTextScaling(fTextSize03*fResFactor);
          pdp->PutTextC( _acceEntries[iNextItem].strQuote,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));

        } else if (_acceEntries[iNextItem].iAlign == CTA_RIGHT) {
          FLOAT fYY = _acceEntries[iNextItem].iY*fResFactor;
          
          pdp->SetTextScaling(fTextSize01*fResFactor);
          pdp->PutTextR( _acceEntries[iNextItem].strTitle,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));
          fYY += fTextSize01*fResFactor*fTextHeight*fSpacing01;
          
          pdp->SetTextScaling(fTextSize02*fResFactor);
          pdp->PutTextR( _acceEntries[iNextItem].strName,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));
          fYY += fTextSize02*fResFactor*fTextHeight*fSpacing02;

          pdp->SetTextScaling(fTextSize03*fResFactor);
          pdp->PutTextR( _acceEntries[iNextItem].strQuote,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));

        } else if (TRUE) {
          FLOAT fYY = _acceEntries[iNextItem].iY*fResFactor;
          
          pdp->SetTextScaling(fTextSize01*fResFactor);
          pdp->PutText( _acceEntries[iNextItem].strTitle,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));
          fYY += fTextSize01*fResFactor*fTextHeight*fSpacing01;
          
          pdp->SetTextScaling(fTextSize02*fResFactor);
          pdp->PutText( _acceEntries[iNextItem].strName,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));
          fYY += fTextSize02*fResFactor*fTextHeight*fSpacing02;

          pdp->SetTextScaling(fTextSize03*fResFactor);
          pdp->PutText( _acceEntries[iNextItem].strQuote,
            _acceEntries[iNextItem].iX*fResFactor, fYY,
            C_WHITE|(INDEX)(fFade*255));

        }
        
        iNextItem++;
        if (iNextItem>=_acceEntries.Count() || bLast) { 
          bOver = TRUE;
          break;
        }
      }
    }

    pdpCurr->Unlock();
    pdp->Lock();

    if (bOver) {
      return 0;
    } else {
      return 1;
    }
  }


procedures:
  WaitScrollingToEnd()
  {
    while (!m_bEnd)
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
    SetModel(MODEL_HOLDER);
    SetModelMainTexture(TEXTURE_HOLDER);

    autowait(0.05f);

    if( !Credits_On(m_fnmMessage))
    {
      CPrintF("Error loading credits file '%s'!\n", (const char *) m_fnmMessage);
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
          ECredits ecr;
          ecr.bStart=TRUE;
          ecr.penSender=this;
          pwsc->SendEvent(ecr);
        }
        call WaitScrollingToEnd();
      }
      on (EStop eStop): 
      {
        CWorldSettingsController *pwsc = GetWSC(this);
        if( pwsc!=NULL)
        {
          ECredits ecr;
          ecr.bStart=FALSE;
          ecr.penSender=this;
          pwsc->SendEvent(ecr);
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

