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

/*
 * Player entity.
 */
4
%{
#include <Engine/StdH.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/CRC.h>
%}

class export CPlayerEntity : CMovableModelEntity {
name      "PlayerEntity";
thumbnail "";
features "AbstractBaseClass";
properties:
  1 FLOAT en_tmPing = 0.0f,    // ping value in seconds (determined by derived class and distributed by the engine)
{
  CPlayerCharacter en_pcCharacter;   // character of the player
  CPlacement3D en_plViewpoint;       // placement of view point relative to the entity
  CPlacement3D en_plLastViewpoint;   // last view point (used for lerping)
}
components:
functions:
  /* Get name of this player. */
  export CTString GetPlayerName(void)
  {
    return en_pcCharacter.GetNameForPrinting();
  }
  export const CTString &GetName(void) const
  {
    return en_pcCharacter.GetName();
  }
  /* Get index of this player in the game. */
  export INDEX GetMyPlayerIndex(void)
  {
    CEntity *penMe = this;
    if (IsPredictor()) {
      penMe = GetPredicted();
    }

    INDEX iPlayer;
    for (iPlayer=0; iPlayer<GetMaxPlayers(); iPlayer++) {
      // if this is ME (this)
      if (GetPlayerEntity(iPlayer)==penMe) {
        return iPlayer;
      }
    }
    // must find my self
    return 15;  // if not found, still return a relatively logical value
  }

  /* Calculate physics for moving. */
  export void DoMoving(void)  // override from CMovableEntity
  {
    CMovableModelEntity::DoMoving();
  }

  /* Copy entity from another entity of same class. */
  export void Copy(CEntity &enOther, ULONG ulFlags)
  {
    CMovableModelEntity::Copy(enOther, ulFlags);

    CPlayerEntity *ppenOther = (CPlayerEntity *)(&enOther);
    en_pcCharacter = ppenOther->en_pcCharacter;
    en_plViewpoint = ppenOther->en_plViewpoint;
    en_plLastViewpoint = ppenOther->en_plLastViewpoint;
  }

  /* Copy entity from another entity of same class. */
  /*CPlayerEntity &operator=(CPlayerEntity &enOther)
  {
    CMovableModelEntity::operator=(enOther);
    en_pcCharacter = enOther.en_pcCharacter;
    en_plViewpoint = enOther.en_plViewpoint;
    return *this;
  }*/
  /* Read from stream. */
  export void Read_t( CTStream *istr) // throw char *
  {
    CMovableModelEntity::Read_t(istr);
    (*istr)>>en_pcCharacter>>en_plViewpoint;
    en_plLastViewpoint = en_plViewpoint;
  }
  /* Write to stream. */
  export void Write_t( CTStream *ostr) // throw char *
  {
    CMovableModelEntity::Write_t(ostr);
    (*ostr)<<en_pcCharacter<<en_plViewpoint;
  }

  // Apply the action packet to the entity movement.
  export virtual void ApplyAction(const CPlayerAction &pa, FLOAT tmLatency) {};
  // Called when player is disconnected
  export virtual void Disconnect(void) {};
  // Called when player character is changed
  export virtual void CharacterChanged(const CPlayerCharacter &pcNew) { en_pcCharacter = pcNew; };

  // provide info for GameAgent enumeration
  export virtual void GetGameAgentPlayerInfo( INDEX iPlayer, CTString &strOut) { };
  // provide info for MSLegacy enumeration
  export virtual void GetMSLegacyPlayerInf( INDEX iPlayer, CTString &strOut) { };
  
  // create a checksum value for sync-check
  export void ChecksumForSync(ULONG &ulCRC, INDEX iExtensiveSyncCheck)
  {
    CMovableModelEntity::ChecksumForSync(ulCRC, iExtensiveSyncCheck);
    CRC_AddBlock(ulCRC, en_pcCharacter.pc_aubGUID, sizeof(en_pcCharacter.pc_aubGUID));
    CRC_AddBlock(ulCRC, en_pcCharacter.pc_aubAppearance, sizeof(en_pcCharacter.pc_aubAppearance));
  }
  // dump sync data to text file
  export void DumpSync_t(CTStream &strm, INDEX iExtensiveSyncCheck)  // throw char *
  {
    CMovableModelEntity::DumpSync_t(strm, iExtensiveSyncCheck);
    strm.FPrintF_t("player: %s\n", 
      (const char *) en_pcCharacter.GetName());
    strm.FPrintF_t("GUID: ");
    {for (INDEX i=0; i<static_cast<INDEX>(sizeof(en_pcCharacter.pc_aubGUID)); i++) {
      strm.FPrintF_t("%02X", en_pcCharacter.pc_aubGUID[i]);
    }}
    strm.FPrintF_t("\n");
    strm.FPrintF_t("appearance: ");
    {for (INDEX i=0; i<MAX_PLAYERAPPEARANCE; i++) {
      strm.FPrintF_t("%02X", en_pcCharacter.pc_aubAppearance[i]);
    }}
    strm.FPrintF_t("\n");
  }
procedures:
  // must have at least one procedure per class
  Dummy() {};
};
