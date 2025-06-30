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

606
%{
#include "Entities/StdH/StdH.h"
#include "Entities/BackgroundViewer.h"
#include "Entities/WorldSettingsController.h"
#include "Entities/Lightning.h"
%}

class CStormController: CRationalEntity {
name      "Storm controller";
thumbnail "Thumbnails\\StormController.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:
  1 CEntityPointer m_penwsc,      // ptr to world settings controller
  2 CTString m_strName              "Name" 'N' = "Storm controller",         // class name
  3 FLOAT m_fNextLightningDelay = 0.0f,
  4 BOOL m_bStormOn = FALSE,
  5 FLOAT m_fNextLightningStrike = 0.0f,
  10 CEntityPointer m_penLightning00  "Lightning 1" 'T' COLOR(C_MAGENTA|0xFF),    // lightning
  11 CEntityPointer m_penLightning01  "Lightning 2" 'Y' COLOR(C_MAGENTA|0xFF),    // lightning
  12 CEntityPointer m_penLightning02  "Lightning 3" 'U' COLOR(C_MAGENTA|0xFF),    // lightning
  13 CEntityPointer m_penLightning03  "Lightning 4" 'I' COLOR(C_MAGENTA|0xFF),    // lightning
  14 CEntityPointer m_penLightning04  "Lightning 5" 'O' COLOR(C_MAGENTA|0xFF),    // lightning
  15 CEntityPointer m_penLightning05  "Lightning 6" 'P' COLOR(C_MAGENTA|0xFF),    // lightning
  16 CEntityPointer m_penLightning06  "Lightning 7"  COLOR(C_MAGENTA|0xFF),       // lightning
  17 CEntityPointer m_penLightning07  "Lightning 8"  COLOR(C_MAGENTA|0xFF),       // lightning
  18 CEntityPointer m_penLightning08  "Lightning 9"  COLOR(C_MAGENTA|0xFF),       // lightning
  19 CEntityPointer m_penLightning09  "Lightning 10" COLOR(C_MAGENTA|0xFF),       // lightning
  20 CEntityPointer m_penLightning10  "Lightning 11" COLOR(C_MAGENTA|0xFF),       // lightning
  21 CEntityPointer m_penLightning11  "Lightning 12" COLOR(C_MAGENTA|0xFF),       // lightning
  22 CEntityPointer m_penLightning12  "Lightning 13" COLOR(C_MAGENTA|0xFF),       // lightning
  23 CEntityPointer m_penLightning13  "Lightning 14" COLOR(C_MAGENTA|0xFF),       // lightning
  24 CEntityPointer m_penLightning14  "Lightning 15" COLOR(C_MAGENTA|0xFF),       // lightning
  25 CEntityPointer m_penLightning15  "Lightning 16" COLOR(C_MAGENTA|0xFF),       // lightning
  26 CEntityPointer m_penLightning16  "Lightning 17" COLOR(C_MAGENTA|0xFF),       // lightning
  27 CEntityPointer m_penLightning17  "Lightning 18" COLOR(C_MAGENTA|0xFF),       // lightning
  28 CEntityPointer m_penLightning18  "Lightning 19" COLOR(C_MAGENTA|0xFF),       // lightning
  29 CEntityPointer m_penLightning19  "Lightning 20" COLOR(C_MAGENTA|0xFF),       // lightning

components:
  1 model   MODEL_STORM_CONTROLLER     "Models\\Editor\\StormController.mdl",
  2 texture TEXTURE_STORM_CONTROLLER   "Models\\Editor\\StormController.tex"

functions:
  // check if one lightning target is valid 
  void CheckOneLightningTarget(CEntityPointer &pen)
  {
    if (pen!=NULL && !IsOfClass(pen, "Lightning"))
    {
      WarningMessage("Target '%s' is not of class Lightning!", (const char*)pen->GetName());
      pen=NULL;
    }
  }

  // get number of lightnings set by user
  INDEX GetLightningsCount(void) const
  {
    // note: only first N that are no NULL are used
    if (m_penLightning00==NULL) { return 0; };
    if (m_penLightning01==NULL) { return 1; };
    if (m_penLightning02==NULL) { return 2; };
    if (m_penLightning03==NULL) { return 3; };
    if (m_penLightning04==NULL) { return 4; };
    if (m_penLightning05==NULL) { return 5; };
    if (m_penLightning06==NULL) { return 6; };
    if (m_penLightning07==NULL) { return 7; };
    if (m_penLightning08==NULL) { return 8; };
    if (m_penLightning09==NULL) { return 9; };
    if (m_penLightning10==NULL) { return 10; };
    if (m_penLightning11==NULL) { return 11; };
    if (m_penLightning12==NULL) { return 12; };
    if (m_penLightning13==NULL) { return 13; };
    if (m_penLightning14==NULL) { return 14; };
    if (m_penLightning15==NULL) { return 15; };
    if (m_penLightning16==NULL) { return 16; };
    if (m_penLightning17==NULL) { return 17; };
    if (m_penLightning18==NULL) { return 18; };
    return 20;
  }

procedures:
  Storm()
  {
    // wait before first lightning
    autowait( 10.0f);
    jump StormInternal();
  }

  StormInternal()
  {
    m_fNextLightningDelay = 5.0f;
    
    while( m_bStormOn && _pTimer->CurrentTick() < ((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmStormEnd)
    {
      m_fNextLightningStrike = _pTimer->CurrentTick() + 1.0f + FRnd()*3.0f + m_fNextLightningDelay;
      while( _pTimer->CurrentTick()<m_fNextLightningStrike &&
             _pTimer->CurrentTick()<((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmStormEnd &&
             m_bStormOn)
      {
        // wait until next lightning
        wait(_pTimer->TickQuantum)
        {
          on (EBegin) :
          {
            resume;
          }
          on (EDeactivate) :
          {
            m_fNextLightningStrike+=1.0f;
            resume;
          }
          on (ETimer) : { stop; }
        }
      }
      // decrease lightning delay
      m_fNextLightningDelay = ClampDn(m_fNextLightningDelay-0.75f, 1.0f);
      // choose random lightning
      INDEX ctLightnings = GetLightningsCount();
      // if there are some lightnings
      if (ctLightnings!=0)
      {
        // choose by random
        CLightning *penLightning = (CLightning *) (&m_penLightning00)[IRnd()%ctLightnings].ep_pen;
        SendToTarget(penLightning, EET_TRIGGER);
      }
    }
    return EReturn();
  }

  Main(EVoid)
  {
    // check lightning targets
    CheckOneLightningTarget( m_penLightning00);
    CheckOneLightningTarget( m_penLightning01);
    CheckOneLightningTarget( m_penLightning02);
    CheckOneLightningTarget( m_penLightning03);
    CheckOneLightningTarget( m_penLightning04);
    CheckOneLightningTarget( m_penLightning05);
    CheckOneLightningTarget( m_penLightning06);
    CheckOneLightningTarget( m_penLightning07);
    CheckOneLightningTarget( m_penLightning08);
    CheckOneLightningTarget( m_penLightning09);
    CheckOneLightningTarget( m_penLightning10);
    CheckOneLightningTarget( m_penLightning11);
    CheckOneLightningTarget( m_penLightning12);
    CheckOneLightningTarget( m_penLightning13);
    CheckOneLightningTarget( m_penLightning14);
    CheckOneLightningTarget( m_penLightning15);
    CheckOneLightningTarget( m_penLightning16);
    CheckOneLightningTarget( m_penLightning17);
    CheckOneLightningTarget( m_penLightning18);
    CheckOneLightningTarget( m_penLightning19);

    // set appearance
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_STORM_CONTROLLER);
    SetModelMainTexture(TEXTURE_STORM_CONTROLLER);
    
    // spawn in world editor
    autowait(0.1f);

    // obtain bcg viewer entity
    CBackgroundViewer *penBcgViewer = (CBackgroundViewer *) GetWorld()->GetBackgroundViewer();
    if( penBcgViewer == NULL)
    {
      // don't do anything
      return;
    }

    // obtain world settings controller 
    m_penwsc = penBcgViewer->m_penWorldSettingsController;
    if( m_penwsc == NULL)
    {
      // don't do anything
      return;
    }
    
    // must be world settings controller entity
    if (!IsOfClass(m_penwsc, "WorldSettingsController"))
    {
      // don't do anything
      return;
    }

    m_bStormOn = FALSE;
    while (TRUE)
    {
      wait()
      {
        on (EEnvironmentStart eEnvironmentStart) :
        {
          TIME tmNow = _pTimer->CurrentTick();
          ((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmStormStart = tmNow-10.0f;
          // force storm end in far future
          ((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmStormEnd = 1e6;
          // set delay between lightnings
          m_bStormOn = TRUE;
          call StormInternal();
          resume;
        }
        on (EStart eStart) :
        {
          if( !m_bStormOn)
          {
            TIME tmNow = _pTimer->CurrentTick();
            // remember current time as storm start time in world settings controller
            ((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmStormStart = tmNow;
            // force storm end in far future
            ((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmStormEnd = 1e6;
            // set delay between lightnings
            m_bStormOn = TRUE;
            call Storm();
          }
          resume;
        }
        on (EStop eStop) :
        {
          if( m_bStormOn)
          {
            TIME tmNow = _pTimer->CurrentTick();
            m_bStormOn = FALSE;
            // remember current time as storm end time in world settings controller
            ((CWorldSettingsController *) m_penwsc.ep_pen)->m_tmStormEnd = tmNow;
          }
          resume;
        }
        otherwise() :
        {
          resume;
        };
      };
    }
  }
};
