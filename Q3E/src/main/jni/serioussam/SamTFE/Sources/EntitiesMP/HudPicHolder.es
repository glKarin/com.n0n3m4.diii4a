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

241
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/BackgroundViewer.h"
%}


%{
BOOL _bDataLoaded = FALSE;
BOOL _bDataError = FALSE;
CTextureObject _toTexture;
%}

class CHudPicHolder: CRationalEntity {
name      "HudPicHolder";
thumbnail "Thumbnails\\HudPicHolder.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:

  1 CTString m_strName "Name" 'N' = "Hud pic holder",
  2 CTString m_strDescription = "",
  3 CTFileName m_fnmPicture "Picture file" 'P' = CTString(""),
  4 FLOAT m_tmFadeInStart = 1e6,
  5 FLOAT m_tmFadeOutStart = 1e6,
  6 FLOAT m_tmFadeInLen "Fade in time" 'I' = 0.5f,
  7 FLOAT m_tmFadeOutLen "Fade out time" 'O' = 0.5f,
  8 FLOAT m_tmAutoFadeOut "Auto fade out time" 'A' = -1.0f,
  9 FLOAT m_fYRatio "Vertical position ratio" 'Y' = 0.5f,
 10 FLOAT m_fXRatio "Horizontal position ratio" 'X' = 0.5f,
 11 FLOAT m_fPictureStretch "Picture stretch" 'S' = 1.0f,

components:
  1 model   MODEL_MARKER     "Models\\Editor\\MessageHolder.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\MessageHolder.tex"

functions:
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("%s", (const char *) m_fnmPicture.FileName());
    return m_strDescription;
  }

  BOOL ReloadData(void)
  {
    _bDataError = FALSE;
    if (!Picture_On(m_fnmPicture))
    {
      Picture_Off();
      return FALSE;
    }    
    return TRUE;
  }

  BOOL LoadOneFile(const CTFileName &fnm)
  {
    if(fnm=="") { return FALSE; }
    try 
    {
      _toTexture.SetData_t(fnm);
      return TRUE;
    }
    catch ( const char *strError)
    {
      CPrintF("%s\n", (const char *)strError);
      return FALSE;
    }
  }

  // turn text on
  BOOL Picture_On(CTFileName fnPic)
  {
    return LoadOneFile(fnPic);
  }

  // turn text off
  void Picture_Off(void)
  {
    _toTexture.SetData(NULL);
  }

  // render credits to given drawport
  FLOAT HudPic_Render(CHudPicHolder *penThis, CDrawPort *pdp)
  {
    if (_bDataError) { return 0; }
    
    if (!_bDataLoaded) {
      if (!ReloadData()) {
        _bDataError = TRUE;
        return 0;
      }
      _bDataLoaded = TRUE;
      return 1;
    }
    
    FLOAT fNow=_pTimer->CurrentTick();
    if( fNow<m_tmFadeInStart) { return 0; }
    if( fNow>m_tmFadeOutStart+m_tmFadeOutLen) { return 0;}

    CDrawPort *pdpCurr=pdp;
    pdp->Unlock();
    pdpCurr->Lock();
    
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

    CTextureData *ptd=(CTextureData *)_toTexture.GetData();
    
    FLOAT fResScale = (FLOAT)pdpCurr->GetHeight() / 480.0f;
    const MEX mexTexW = ptd->GetWidth();
    const MEX mexTexH = ptd->GetHeight();
    FLOAT fPicRatioW, fPicRatioH;
    if( mexTexW > mexTexH) {
      fPicRatioW = mexTexW/mexTexH;
      fPicRatioH = 1.0f;
    } else {
      fPicRatioW = 1.0f;
      fPicRatioH = mexTexH/mexTexW;
    }
    PIX picW = (PIX) (128*m_fPictureStretch*fResScale*fPicRatioW);
    PIX picH = (PIX) (128*m_fPictureStretch*fResScale*fPicRatioH);

    FLOAT fXCenter = m_fXRatio * pdpCurr->GetWidth();
    FLOAT fYCenter = m_fYRatio * pdpCurr->GetHeight();
    PIXaabbox2D boxScr=PIXaabbox2D(
      PIX2D(fXCenter-picW/2, fYCenter-picH/2),
      PIX2D(fXCenter+picW/2, fYCenter+picH/2) );
    pdpCurr->PutTexture(&_toTexture, boxScr, C_WHITE|ubA);

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

    if( !Picture_On(m_fnmPicture))
    {
      Picture_Off();
      return;
    }
    _bDataError = FALSE;

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
          EHudPicFX etfx;
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
    Picture_Off();
    return;
  }
};

