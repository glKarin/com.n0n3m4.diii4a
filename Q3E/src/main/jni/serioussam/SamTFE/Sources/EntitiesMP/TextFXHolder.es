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

239
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/BackgroundViewer.h"
%}

%{
static CStaticStackArray<CTString> _astrLines;
static CTFileName _fnLastLoaded;
%}

class CTextFXHolder: CRationalEntity {
name      "TextFXHolder";
thumbnail "Thumbnails\\TextFXHodler.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:

  1 CTString m_strName "Name" 'N' = "Text FX holder",
  2 CTString m_strDescription = "",
  3 CTFileName m_fnmMessage  "Text file" 'T' = CTString(""),
  4 FLOAT m_tmFadeInStart = 1e6,
  5 FLOAT m_tmFadeOutStart = 1e6,
  6 FLOAT m_tmFadeInLen "Fade in time" 'I' = 0.5f,
  7 FLOAT m_tmFadeOutLen "Fade out time" 'O' = 0.5f,
  8 FLOAT m_tmAutoFadeOut "Auto fade out time" 'A' = -1.0f,

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

  void CTextFXHolder(void) 
  {
    bDataLoaded = FALSE;
  }

  BOOL ReloadData(void)
  {
    m_bDataError = FALSE;
    if (!Text_On(m_fnmMessage))
    {
      Text_Off();
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
      CTString *astr = _astrLines.Push(ctLines);
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

  // turn text on
  BOOL Text_On(CTFileName fnText)
  {
    _astrLines.PopAll();
    return LoadOneFile(fnText);
  }

  // turn text off
  void Text_Off(void)
  {
    _astrLines.Clear();
  }

  // render credits to given drawport
  FLOAT TextFX_Render(CTextFXHolder *penThis, CDrawPort *pdp)
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
    
    FLOAT fNow=_pTimer->CurrentTick();
    if( fNow<m_tmFadeInStart) { return 0; }
    if( fNow>m_tmFadeOutStart+m_tmFadeOutLen) { return 0;}

    PIX pixW = 0;
    PIX pixH = 0;
    PIX pixJ = 0;
    FLOAT fResolutionScaling;
    PIX pixLineHeight;
    CTString strEmpty;

    CDrawPort *pdpCurr=pdp;
    pdp->Unlock();
    pdpCurr->Lock();
    
    pixW = pdpCurr->GetWidth();
    pixH = pdpCurr->GetHeight();
    fResolutionScaling = (FLOAT)pixH / 360.0f;
    pdpCurr->SetFont( _pfdDisplayFont);
    pixLineHeight = (PIX) floor(20*fResolutionScaling);

    INDEX ctMaxLinesOnScreen = pixH/pixLineHeight;
    INDEX ctLines=ClampUp(_astrLines.Count(), ctMaxLinesOnScreen);

    pixJ = PIX(pixH/2-ctLines/2.0f*pixLineHeight);
    for (INDEX iLine = 0; iLine<ctLines; iLine++)
    {
      CTString *pstr = &_astrLines[iLine];
      pdp->SetFont( _pfdDisplayFont);
      pdp->SetTextScaling( fResolutionScaling);
      pdp->SetTextAspect( 1.0f);
      FLOAT fRatio=1.0f;
      if( fNow>m_tmFadeOutStart)
      {
        fRatio=CalculateRatio(fNow, m_tmFadeOutStart, m_tmFadeOutStart+m_tmFadeOutLen, 0, 1);
      }
      if( fNow<m_tmFadeInStart+m_tmFadeInLen)
      {
        fRatio=CalculateRatio(fNow, m_tmFadeInStart, m_tmFadeInStart+m_tmFadeInLen, 1, 0);
      }
      UBYTE ubA=ClampUp(UBYTE(fRatio*255.0f), UBYTE(255));
      pdp->PutTextC( *pstr, pixW/2, pixJ, C_WHITE|ubA);
      pixJ+=pixLineHeight;
    }

    pdpCurr->Unlock();
    pdp->Lock();

    return 1;
  }


procedures:
  
  WaitAndFadeOut(EVoid)
  {
    autowait( m_tmAutoFadeOut);
    jump ApplyFadeOut();
  }

  ApplyFadeOut(EVoid)
  {
    m_tmFadeOutStart = _pTimer->CurrentTick();
    CWorldSettingsController *pwsc = GetWSC(this);
    if( pwsc!=NULL)
    {
      autowait(m_tmFadeOutLen);
      CWorldSettingsController *pwsc = GetWSC(this);
      ETextFX etfx;
      etfx.bStart=FALSE;
      etfx.penSender=this;
      pwsc->SendEvent(etfx);
    }
    return EReturn();
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

    if( !Text_On(m_fnmMessage))
    {
      Text_Off();
      return;
    }
    m_bDataError = FALSE;

    wait() {
      on (EBegin): 
      {
        resume;
      }
      on (EStart eStart): 
      {
        CWorldSettingsController *pwsc = GetWSC(this);
        if( pwsc!=NULL)
        {
          m_tmFadeInStart = _pTimer->CurrentTick();
          ETextFX etfx;
          etfx.bStart=TRUE;
          etfx.penSender=this;
          pwsc->SendEvent(etfx);
          if( m_tmAutoFadeOut!=-1)
          {
            call WaitAndFadeOut();
          }
        }
        resume;
      }
      on (EStop eStop): 
      {
        call ApplyFadeOut();
        resume;
      }
      on (EReturn): 
      {
        resume;
      }
    }
    Text_Off();
    return;
  }
};

