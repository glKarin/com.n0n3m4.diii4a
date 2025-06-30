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

228
%{
#include "EntitiesMP/StdH/StdH.h"
#include <EntitiesMP/AnimationChanger.h>
%}


class CAnimationHub : CRationalEntity {
name      "AnimationHub";
thumbnail "Thumbnails\\AnimationHub.tbn";
features  "HasName", "IsTargetable";

properties:
  1 CTString m_strName          "Name" 'N' = "Animation hub",
  2 CTString m_strDescription = "",

  3 FLOAT m_tmDelayEach "Delay each" 'D' = 0.0f,

 10 CEntityPointer m_penTarget0  "Target0" 'T' COLOR(C_GREEN|0xFF),
 11 CEntityPointer m_penTarget1  "Target1"     COLOR(C_GREEN|0xFF),
 12 CEntityPointer m_penTarget2  "Target2"     COLOR(C_GREEN|0xFF),
 13 CEntityPointer m_penTarget3  "Target3"     COLOR(C_GREEN|0xFF),
 14 CEntityPointer m_penTarget4  "Target4"     COLOR(C_GREEN|0xFF),
 15 CEntityPointer m_penTarget5  "Target5"     COLOR(C_GREEN|0xFF),
 16 CEntityPointer m_penTarget6  "Target6"     COLOR(C_GREEN|0xFF),
 17 CEntityPointer m_penTarget7  "Target7"     COLOR(C_GREEN|0xFF),
 18 CEntityPointer m_penTarget8  "Target8"     COLOR(C_GREEN|0xFF),
 19 CEntityPointer m_penTarget9  "Target9"     COLOR(C_GREEN|0xFF),

 20 FLOAT m_tmDelay0 "Delay0" = 0.0f,
 21 FLOAT m_tmDelay1 "Delay1" = 0.0f,
 22 FLOAT m_tmDelay2 "Delay2" = 0.0f,
 23 FLOAT m_tmDelay3 "Delay3" = 0.0f,
 24 FLOAT m_tmDelay4 "Delay4" = 0.0f,
 25 FLOAT m_tmDelay5 "Delay5" = 0.0f,
 26 FLOAT m_tmDelay6 "Delay6" = 0.0f,
 27 FLOAT m_tmDelay7 "Delay7" = 0.0f,
 28 FLOAT m_tmDelay8 "Delay8" = 0.0f,
 29 FLOAT m_tmDelay9 "Delay9" = 0.0f,

 100 INDEX m_iModelAnim = 0,
 101 BOOL  m_bModelLoop = 0,
 102 INDEX m_iTextureAnim = 0,
 103 BOOL  m_bTextureLoop = 0,
 104 INDEX m_iLightAnim = 0,
 105 BOOL  m_bLightLoop = 0,
 106 COLOR m_colAmbient = 0,
 107 COLOR m_colDiffuse = 0,

 110 INDEX m_iCounter = 0,

components:
  1 model   MODEL_HUB     "Models\\Editor\\AnimationHub.mdl",
  2 texture TEXTURE_HUB   "Models\\Editor\\AnimationHub.tex"

functions:
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penTarget0!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s...", (const char *) m_penTarget0->GetName());
    }
    return m_strDescription;
  }

procedures:
  RelayEvents()
  {
    // for each target
    m_iCounter=0;
    while(m_iCounter<10) {
      // get delay
      FLOAT fDelay = m_tmDelayEach + (&m_tmDelay0)[m_iCounter];
      // if has delay
      if (fDelay>0) {
        // wait
        autowait(fDelay);
      }

      // get the target
      CEntity *penTarget = (&m_penTarget0)[m_iCounter];
      // if no more targets
      if (penTarget==NULL) {
        // stop
        jump WaitChange();
      }
      // sent event to it
      EChangeAnim eca;
      eca.iModelAnim   = m_iModelAnim  ;
      eca.bModelLoop   = m_bModelLoop  ;
      eca.iTextureAnim = m_iTextureAnim;
      eca.bTextureLoop = m_bTextureLoop;
      eca.iLightAnim   = m_iLightAnim  ;
      eca.bLightLoop   = m_bLightLoop  ;
      eca.colAmbient   = m_colAmbient  ;
      eca.colDiffuse   = m_colDiffuse  ;
      penTarget->SendEvent(eca);

      m_iCounter++;
    }

    jump WaitChange();
  }

  WaitChange()
  {
    // wait forever
    while(TRUE) {
      wait() {
        on (EChangeAnim eca) : {
          m_iModelAnim    = eca.iModelAnim  ;
          m_bModelLoop    = eca.bModelLoop  ;
          m_iTextureAnim  = eca.iTextureAnim;
          m_bTextureLoop  = eca.bTextureLoop;
          m_iLightAnim    = eca.iLightAnim  ;
          m_bLightLoop    = eca.bLightLoop  ;
          m_colAmbient    = eca.colAmbient  ;
          m_colDiffuse    = eca.colDiffuse  ;
          jump RelayEvents();
        }
      }
    }
  }

  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_HUB);
    SetModelMainTexture(TEXTURE_HUB);

    // check target types
    for (INDEX i=0; i<10; i++) {
      CEntityPointer &penTarget = (&m_penTarget0)[i];
      if (penTarget!=NULL && 
        !IsOfClass(penTarget, "ModelHolder2") &&
        !IsOfClass(penTarget, "Light")) {
        WarningMessage("All targets must be ModelHolder2 or Light!");
        penTarget=NULL;
      }
    }
    jump WaitChange();
  }
};

