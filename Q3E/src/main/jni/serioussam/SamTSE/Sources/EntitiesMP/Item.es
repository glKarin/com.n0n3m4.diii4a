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

800
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Items/ItemHolder/ItemHolder.h"
%}

%{
// used to render certain entities only for certain players (like picked items, etc.)
extern ULONG _ulPlayerRenderingMask;
%}

class export CItem : CMovableModelEntity {
name      "Item";
thumbnail "";
features  "HasName", "HasDescription", "IsTargetable", "CanBePredictable";

properties:
  1 CTString m_strName            "Name" 'N' = "Item",
  2 CTString m_strDescription = "",

  // class properties
  5 FLOAT m_fValue = 0.0f,                                 // value
  6 FLOAT m_fRespawnTime = 0.0f,                           // default respawn time
 26 FLOAT m_fCustomRespawnTime "Respawn Time" = 0.0f,      // custom respawn time
  7 BOOL m_bRespawn "Respawn" 'R' = FALSE,    // respawn item
  8 CEntityPointer m_penTarget  "Target" 'T' COLOR(C_dGRAY|0xFF),   // target to trigger when crossed over
  9 BOOL m_bPickupOnce  "PickupOnce" 'P' = FALSE,   // can be picked by only one player, triggers only when really picked
 10 CSoundObject m_soPick,      // sound channel
 12 FLOAT m_fPickSoundLen = 0.0f,
 14 BOOL m_bDropped = FALSE,    // dropped by a player during a deathmatch game
 15 INDEX m_ulPickedMask = 0,   // mask for which players picked this item
 16 BOOL m_bFloating "Floating" 'F' = FALSE,

components:
  1 model   MODEL_ITEM      "Models\\Items\\ItemHolder\\ItemHolder.mdl",

functions:
  virtual void AdjustDifficulty(void)
  {
  }

  /* Adjust model mip factor if needed. */
  void AdjustMipFactor(FLOAT &fMipFactor)
  {
    // adjust flare glow, to decrease power with how you get closer
    CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel(ITEMHOLDER_ATTACHMENT_FLARE);
    if( pamo != NULL)
    {
      FLOAT fRatio = (Clamp( fMipFactor, 5.0f, 7.0f)-5.0f)/2.0f;
      UBYTE ubRatio = UBYTE(255*fRatio);
      COLOR colMutiply = RGBToColor(ubRatio,ubRatio,ubRatio)|CT_OPAQUE;
      pamo->amo_moModelObject.mo_colBlendColor = colMutiply;
    }

    // if never picked
    if (m_ulPickedMask==0) {
      // don't bother testing
      return;
    }

    //BOOL bFlare = TRUE;
    // if current player has already picked this item
    if (_ulPlayerRenderingMask&m_ulPickedMask) {
      // if picked items are not rendered
      extern INDEX plr_bRenderPicked;
      if (!plr_bRenderPicked) {
        // kill mip factor
        fMipFactor = UpperLimit(0.0f);
      }
      // if picked item particles are not rendered
      /*extern INDEX plr_bRenderPickedParticles;
      if (!plr_bRenderPickedParticles) {
        // kill flare 
        bFlare = FALSE; // DG: bFlare is otherwise unused!
      }*/
    }

    // implement flare on/off ?
  }

  // check whether should render particles for this item
  BOOL ShowItemParticles(void)
  {
    // if current player has already picked this item
    if (_ulPlayerRenderingMask&m_ulPickedMask) {
      // if picked item particles are not rendered
      extern INDEX plr_bRenderPickedParticles;
      if (!plr_bRenderPickedParticles) {
        // don't render
        return FALSE;
      }
    }
    // otherwise, render
    return TRUE;
  }

  // check if given player already picked this item, and mark if not
  BOOL MarkPickedBy(CEntity *pen)
  {
    if (!IsOfClass(pen, "Player")) {
      return FALSE;
    }
    INDEX iPlayer = ((CPlayerEntity*)pen)->GetMyPlayerIndex();
    BOOL bPickedAlready = (1<<iPlayer)&m_ulPickedMask;
    m_ulPickedMask |= (1<<iPlayer);
    return bPickedAlready;
  }

  // get maximum allowed range for predicting this entity
  FLOAT GetPredictionRange(void)
  {
    extern FLOAT cli_fPredictItemsRange;
    return cli_fPredictItemsRange;
  }

  /* Adjust model shading parameters if needed. */
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    // in DM, glares are off, so add some light to items
    //if( m_bRespawn)
    {
      // fixed light and ambient
      colLight   = 0x40404040;
      colAmbient = 0x60606060;
    }
    /*
    else
    {
      // fixed light and ambient
      colLight   = 0x30303030;
      colAmbient = 0x30303030;
    }
    */

    // light direction always from upper left corner relative to the object
    vLightDirection = FLOAT3D(-1,-1,-1);
    vLightDirection.Normalize();
    vLightDirection*=GetRotationMatrix();

    // no shadow
    return FALSE;
  };

/************************************************************
 *                      INITALIZATION                       *
 ************************************************************/
  void Initialize(void) {
    InitAsModel();
    SetFlags(GetFlags()|ENF_SEETHROUGH);

    if (m_bFloating) {
      SetPhysicsFlags(EPF_MODEL_FLYING);
    } else {
      SetPhysicsFlags(EPF_MODEL_SLIDING);
    }

    SetCollisionFlags(ECF_ITEM);

    // make items not slide that much
    en_fDeceleration = 60.0f;

    // set appearance
    SetModel(MODEL_ITEM);
    SetDesiredTranslation(FLOAT3D(0,0,0));  // just to add to movers
  };



/************************************************************
 *                   SET MODEL AND ATTACHMENT               *
 ************************************************************/
  // Add item
  void AddItem(ULONG ulIDModel, ULONG ulIDTexture,
               ULONG ulIDReflectionTexture, ULONG ulIDSpecularTexture, ULONG ulIDBumpTexture) {
    AddAttachmentToModel(this, *GetModelObject(), ITEMHOLDER_ATTACHMENT_ITEM, ulIDModel, ulIDTexture,
                         ulIDReflectionTexture, ulIDSpecularTexture, ulIDBumpTexture);
  };
  void AddItemSpecial(INDEX iAttachmentPos, ULONG ulIDModel, ULONG ulIDTexture,
               ULONG ulIDReflectionTexture, ULONG ulIDSpecularTexture, ULONG ulIDBumpTexture) {
    AddAttachmentToModel(this, *GetModelObject(), iAttachmentPos, ulIDModel, ulIDTexture,
                         ulIDReflectionTexture, ulIDSpecularTexture, ulIDBumpTexture);
  };

  // Add attachment to item
  void AddItemAttachment(INDEX iAttachment, ULONG ulIDModel, ULONG ulIDTexture,
                         ULONG ulIDReflectionTexture, ULONG ulIDSpecularTexture, ULONG ulIDBumpTexture) {
    CModelObject &mo = GetModelObject()->GetAttachmentModel(ITEMHOLDER_ATTACHMENT_ITEM)->amo_moModelObject;
    AddAttachmentToModel(this, mo, iAttachment, ulIDModel, ulIDTexture,
                         ulIDReflectionTexture, ulIDSpecularTexture, ulIDBumpTexture);
  };
  // set animation of attachment
  void SetItemAttachmentAnim(INDEX iAttachment, INDEX iAnim)
  {
    CModelObject &mo = 
      GetModelObject()->GetAttachmentModel(ITEMHOLDER_ATTACHMENT_ITEM)->amo_moModelObject.
        GetAttachmentModel(iAttachment)->amo_moModelObject;
    mo.PlayAnim(iAnim, 0);
  }

  // Add flare
  void AddFlare(ULONG ulIDModel, ULONG ulIDTexture, 
    const FLOAT3D &vPos, const FLOAT3D &vStretch)
  {
    // add flare to items if not respawn
    if( !m_bRespawn && !m_bDropped)
    {
      AddAttachmentToModel(this, *GetModelObject(), 
        ITEMHOLDER_ATTACHMENT_FLARE, ulIDModel, ulIDTexture, 0,0,0);
      CAttachmentModelObject &amo = *GetModelObject()->GetAttachmentModel(ITEMHOLDER_ATTACHMENT_FLARE);
      amo.amo_moModelObject.StretchModel(vStretch);
      amo.amo_plRelative.pl_PositionVector = vPos;
    }
  };

  // Stretch item
  void StretchItem(const FLOAT3D &vStretch) {
    CModelObject &mo = GetModelObject()->GetAttachmentModel(ITEMHOLDER_ATTACHMENT_ITEM)->amo_moModelObject;
    mo.StretchModel(vStretch);
    ModelChangeNotify();
  };



  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CItem) - sizeof(CMovableModelEntity) + CMovableModelEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strDescription.Length();
    slUsedMemory += m_strName.Length();
    slUsedMemory += 1* sizeof(CSoundObject);
    return slUsedMemory;
  }



procedures:

/************************************************************
 *          VIRTUAL PROCEDURES THAT NEED OVERRIDE           *
 ************************************************************/
  ItemCollected(EPass epass) { return; };



/************************************************************
 *                I  T  E  M    L  O  O  P                  *
 ************************************************************/

  ItemLoop(EVoid)
  {
    m_fCustomRespawnTime = ClampDn( m_fCustomRespawnTime, 0.0f);
    autowait(0.1f);

    SetPredictable(TRUE);
    AdjustDifficulty();

    wait() {
      on (EBegin) : { resume; }
      on (EPass epass) : { 
        if (!IsOfClass(epass.penOther, "Player")) {
          pass;
        }
        if (!(m_bPickupOnce||m_bRespawn)) {
          SendToTarget(m_penTarget, EET_TRIGGER, epass.penOther);
          m_penTarget = NULL;
        }
        call ItemCollected(epass); 
      }
      on (EEnd) : { stop; }
    }
    // wait for sound to end
    autowait(m_fPickSoundLen+0.5f);
    // cease to exist
    Destroy();
    return;
  };


  ItemReceived(EVoid)
  {
    // hide yourself
    SwitchToEditorModel();
    if ((m_bPickupOnce||m_bRespawn)) {
      SendToTarget(m_penTarget, EET_TRIGGER, NULL);
    }

    // respawn item
    if (m_bRespawn) {
      ASSERT(m_fRespawnTime>0.0f);

      // wait to respawn
      wait(m_fRespawnTime) {
        on (EBegin) : { resume; }
        on (ETimer) : { stop; }
        otherwise() : { resume; }
      }
      // show yourself
      SwitchToModel();
    
    // cease to exist
    } else {
      return EEnd();
    }
    return;
  };
};

