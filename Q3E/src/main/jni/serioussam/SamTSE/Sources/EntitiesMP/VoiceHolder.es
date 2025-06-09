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

231
%{
#include "EntitiesMP/StdH/StdH.h"
extern INDEX ent_bReportBrokenChains;
%}

class CVoiceHolder : CRationalEntity {
name      "VoiceHolder";
thumbnail "Thumbnails\\VoiceHolder.tbn";
features  "HasName", "IsTargetable";

properties:
  1 CTString m_strName          "Name" 'N' = "VoiceHolder",
  3 CTString m_strDescription = "",
  2 CTFileName m_fnmMessage  "Message" 'M' = CTString(""),
  5 BOOL m_bActive "Active" 'A' = TRUE,
  6 INDEX m_ctMaxTrigs            "Max trigs" 'X' = 1, // how many times could trig

  {
    CAutoPrecacheSound m_aps;
  }
components:
  1 model   MODEL_MARKER     "Models\\Editor\\VoiceHolder.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\VoiceHolder.tex"

functions:
  void Precache(void)
  {
    m_aps.Precache(m_fnmMessage);
  }
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("%s", (const char *) m_fnmMessage.FileName());
    return m_strDescription;
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);
    wait() {
      on (ETrigger eTrigger): {
        if (!m_bActive) {
          resume;
        }
        CEntity *penCaused = FixupCausedToPlayer(this, eTrigger.penCaused);
        EVoiceMessage eMsg;
        eMsg.fnmMessage = m_fnmMessage;
        penCaused->SendEvent(eMsg);
        // if max trig count is used for counting
        if(m_ctMaxTrigs > 0) {
          // decrease count
          m_ctMaxTrigs-=1;
          // if we trigged max times
          if( m_ctMaxTrigs <= 0) {
            // cease to exist
            Destroy();
            stop;
          }
        }
        resume;
      }
      on (EActivate): {
        m_bActive = TRUE;
        resume;
      }
      on (EDeactivate): {
        m_bActive = FALSE;
        resume;
      }
    }
    return;
  }
};

