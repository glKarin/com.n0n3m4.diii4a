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

404
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/MusicHolder.h"
%}

uses "EntitiesMP/Marker";

%{
  extern void CPlayerWeapons_Precache(ULONG ulAvailable);
%}

class CPlayerMarker: CMarker {
name      "Player Marker";
thumbnail "Thumbnails\\PlayerMarker.tbn";
features "IsImportant";

properties:
  1 FLOAT m_fHealth          "Health" 'H' = 100.0f,
  2 FLOAT m_fShield          "Shield" 'S' = 0.0f,
  3 INDEX m_iGiveWeapons     "Give Weapons" 'W' = 0x1,
  4 INDEX m_iTakeWeapons     "Take Weapons"  = 0x0,
  5 CTString m_strGroup      "Group" 'G' = "",
  6 BOOL m_bQuickStart       "Quick start" 'Q' = FALSE,
  7 BOOL m_bStartInComputer  "Start in computer" 'C' = FALSE,
  8 CEntityPointer m_penMessage  "Message" 'M',
  9 FLOAT m_fMaxAmmoRatio    "Max ammo ratio" 'A' = 0.0f,
 10 FLOAT m_tmLastSpawned = -1000.0f, // to avoid telefragging in deathmatch
 11 INDEX m_iTakeAmmo        "Take Ammo"  = 0x0,
 12 BOOL m_bNoRespawnInPlace "No Respawn In Place" 'R'  = FALSE,

components:
  1 model   MODEL_MARKER     "Models\\Editor\\PlayerStart.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\PlayerStart.tex",

functions:
  void Precache(void) {
    if (m_iGiveWeapons>1) {
      CPlayerWeapons_Precache(m_iGiveWeapons);
    }
  }
  BOOL HandleEvent(const CEntityEvent &ee) {
    if (ee.ee_slEvent == EVENTCODE_ETrigger) {
      CEntity *penMainMusicHolder = _pNetwork->GetEntityWithName("MusicHolder", 0);
      if (penMainMusicHolder==NULL || !IsOfClass(penMainMusicHolder, "MusicHolder")) {
        return TRUE;
      }
      CMusicHolder *pmh = (CMusicHolder *)penMainMusicHolder;
      BOOL bNew = (pmh->m_penRespawnMarker!=this);
      pmh->m_penRespawnMarker = this;

      // if this is a new marker and we are in single player and the trigger originator is valid
      CEntity *penCaused = ((ETrigger&)ee).penCaused;
      if (bNew &&
        (GetSP()->sp_bSinglePlayer && GetSP()->sp_gmGameMode!=CSessionProperties::GM_FLYOVER)
        && IsOfClass(penCaused, "Player")) {
        // if the player wants auto-save
        CPlayerSettings *pps = (CPlayerSettings *) (((CPlayerEntity*)penCaused)->en_pcCharacter.pc_aubAppearance);
        if (pps->ps_ulFlags&PSF_AUTOSAVE) {
          // save now
          _pShell->Execute("gam_bQuickSave=1;");
        }
      }
      return TRUE;
    }

    return FALSE;
  };

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    // set name
    if (m_bQuickStart) {
      m_strName.PrintF("Player Quick Start");
    } else {
      m_strName.PrintF("Player Start - %s", (const char *) m_strGroup);
    }

    return;
  }
};

