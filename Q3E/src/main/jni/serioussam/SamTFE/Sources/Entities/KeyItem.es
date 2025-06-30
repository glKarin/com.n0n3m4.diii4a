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

805
%{
#include "Entities/StdH/StdH.h"
#include "Models/Items/ItemHolder/ItemHolder.h"
%}

uses "Entities/Item";

// key type 
enum KeyItemType {
  0 KIT_ANKHWOOD          "Wooden ankh",
  1 KIT_ANKHROCK          "Stone ankh",
  2 KIT_ANKHGOLD          "Gold ankh",
  3 KIT_AMONGOLD          "Gold amon",
  4 KIT_ANKHGOLDDUMMY     "Gold ankh dummy key",
  5 KIT_ELEMENTEARTH      "Element - Earth",
  6 KIT_ELEMENTWATER      "Element - Water",
  7 KIT_ELEMENTAIR        "Element - Air",
  8 KIT_ELEMENTFIRE       "Element - Fire",
  9 KIT_RAKEY             "Ra Key",
 10 KIT_MOONKEY           "Moon Key",
 12 KIT_EYEOFRA           "Eye of Ra",
 13 KIT_SCARAB            "Scarab",
 14 KIT_COBRA             "Cobra",
 15 KIT_SCARABDUMMY       "Scarab dummy",
 16 KIT_HEART             "Gold Heart",
 17 KIT_FEATHER           "Feather of Truth",
 18 KIT_SPHINX1           "Sphinx 1",
 19 KIT_SPHINX2           "Sphinx 2",
};

// event for sending through receive item
event EKey {
  enum KeyItemType kitType,
};

%{

const char *GetKeyName(enum KeyItemType kit)
{
  switch(kit) {
  case KIT_ANKHWOOD :       return TRANS("Wooden ankh"); break;
  case KIT_ANKHROCK:        return TRANS("Stone ankh"); break;
  case KIT_ANKHGOLD :
  case KIT_ANKHGOLDDUMMY :  return TRANS("Gold ankh"); break;
  case KIT_AMONGOLD :       return TRANS("Gold Amon statue"); break;
  case KIT_ELEMENTEARTH  :  return TRANS("Earth element"); break;
  case KIT_ELEMENTWATER  :  return TRANS("Water element"); break;
  case KIT_ELEMENTAIR    :  return TRANS("Air element"); break;
  case KIT_ELEMENTFIRE   :  return TRANS("Fire element"); break;
  case KIT_RAKEY         :  return TRANS("Ra key"); break;
  case KIT_MOONKEY       :  return TRANS("Moon key"); break;
  case KIT_EYEOFRA       :  return TRANS("Eye of Ra"); break;
  case KIT_SCARAB        :
  case KIT_SCARABDUMMY   :  return TRANS("Scarab"); break;
  case KIT_COBRA         :  return TRANS("Cobra"); break;
  case KIT_HEART         :  return TRANS("Gold Heart"); break;
  case KIT_FEATHER       :  return TRANS("Feather of Truth"); break;
  case KIT_SPHINX1       :
  case KIT_SPHINX2       :  return TRANS("Gold Sphinx"); break;
  default: return TRANS("unknown item"); break;
  };
}

%}

class CKeyItem : CItem {
name      "KeyItem";
thumbnail "Thumbnails\\KeyItem.tbn";
features "IsImportant";

properties:
  1 enum KeyItemType m_kitType    "Type" 'Y' = KIT_ANKHWOOD,     // key type
  3 INDEX m_iSoundComponent = 0,

components:
  0 class   CLASS_BASE        "Classes\\Item.ecl",

// ********* ANKH KEY *********
  1 model   MODEL_ANKHWOOD      "Models\\Items\\Keys\\AnkhWood\\Ankh.mdl",
  2 texture TEXTURE_ANKHWOOD    "Models\\Ages\\Egypt\\Vehicles\\BigBoat\\OldWood.tex",

  3 model   MODEL_ANKHROCK      "Models\\Items\\Keys\\AnkhStone\\Ankh.mdl",
  4 texture TEXTURE_ANKHROCK    "Models\\Items\\Keys\\AnkhStone\\Stone.tex",

  5 model   MODEL_ANKHGOLD      "Models\\Items\\Keys\\AnkhGold\\Ankh.mdl",
  6 texture TEXTURE_ANKHGOLD    "Models\\Items\\Keys\\AnkhGold\\Ankh.tex",

  7 model   MODEL_AMONGOLD      "Models\\Ages\\Egypt\\Gods\\Amon\\AmonGold.mdl",
  8 texture TEXTURE_AMONGOLD    "Models\\Ages\\Egypt\\Gods\\Amon\\AmonGold.tex",

 10 model   MODEL_ELEMENTAIR    "Models\\Items\\Keys\\Elements\\Air.mdl",
 11 texture TEXTURE_ELEMENTAIR  "Models\\Items\\Keys\\Elements\\Air.tex",

 20 model   MODEL_ELEMENTWATER    "Models\\Items\\Keys\\Elements\\Water.mdl",
 21 texture TEXTURE_ELEMENTWATER  "Models\\Items\\Keys\\Elements\\Water.tex",

 30 model   MODEL_ELEMENTFIRE    "Models\\Items\\Keys\\Elements\\Fire.mdl",
 31 texture TEXTURE_ELEMENTFIRE  "Models\\Items\\Keys\\Elements\\Fire.tex",

 40 model   MODEL_ELEMENTEARTH    "Models\\Items\\Keys\\Elements\\Earth.mdl",
 41 texture TEXTURE_ELEMENTEARTH  "Models\\Items\\Keys\\Elements\\Texture.tex",

 50 model   MODEL_RAKEY           "Models\\Items\\Keys\\RaKey\\Key.mdl",
 51 texture TEXTURE_RAKEY         "Models\\Items\\Keys\\RaKey\\Key.tex",

 60 model   MODEL_MOONKEY         "Models\\Items\\Keys\\RaSign\\Sign.mdl",
 61 texture TEXTURE_MOONKEY       "Models\\Items\\Keys\\RaSign\\Sign.tex",

 70 model   MODEL_EYEOFRA         "Models\\Items\\Keys\\EyeOfRa\\EyeOfRa.mdl",
 71 texture TEXTURE_EYEOFRA       "Models\\Items\\Keys\\EyeOfRa\\EyeOfRa.tex",

 80 model   MODEL_SCARAB          "Models\\Items\\Keys\\Scarab\\Scarab.mdl",
 81 texture TEXTURE_SCARAB        "Models\\Items\\Keys\\Scarab\\Scarab.tex",

 90 model   MODEL_COBRA           "Models\\Items\\Keys\\Uaset\\Uaset.mdl",
 91 texture TEXTURE_COBRA         "Models\\Items\\Keys\\Uaset\\Uaset.tex",

 92 model   MODEL_FEATHER         "Models\\Items\\Keys\\Luxor\\FeatherOfTruth.mdl",
 93 texture TEXTURE_FEATHER       "Models\\Items\\Keys\\Luxor\\FeatherOfTruth.tex",
 94 model   MODEL_HEART           "Models\\Items\\Keys\\Luxor\\GoldHeart.mdl",
 95 texture TEXTURE_HEART         "Models\\Items\\Keys\\Luxor\\GoldHeart.tex",

 96 model   MODEL_SPHINXGOLD      "Models\\Items\\Keys\\GoldSphinx\\GoldSphinx.mdl",
 97 texture TEXTURE_SPHINXGOLD    "Models\\Items\\Keys\\GoldSphinx\\Sphinx.tex",

 // ********* MISC *********
255 texture TEXTURE_FLARE       "Models\\Items\\Flares\\Flare.tex",
256 model   MODEL_FLARE         "Models\\Items\\Flares\\Flare.mdl",
257 texture TEX_REFL_GOLD01     "Models\\ReflectionTextures\\Gold01.tex",
258 texture TEX_REFL_METAL01    "Models\\ReflectionTextures\\LightMetal01.tex",
259 texture TEX_SPEC_MEDIUM     "Models\\SpecularTextures\\Medium.tex",
260 texture TEX_SPEC_STRONG     "Models\\SpecularTextures\\Strong.tex",

// ************** SOUNDS **************
301 sound   SOUND_KEY         "Sounds\\Items\\Key.wav",

functions:
  void Precache(void) {
    PrecacheSound(SOUND_KEY);
  }
  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    pes->es_strName = GetKeyName(m_kitType);
    pes->es_ctCount = 1;
    pes->es_ctAmmount = 1;
    pes->es_fValue = 1;
    pes->es_iScore = 0;//m_iScore;
    return TRUE;
  }
  // render particles
  void RenderParticles(void) {
    // no particles when not existing
    if (GetRenderType()!=CEntity::RT_MODEL) {
      return;
    }
    switch (m_kitType) {
      case KIT_ANKHWOOD:
      case KIT_ANKHROCK:
      case KIT_ANKHGOLD:
      case KIT_ANKHGOLDDUMMY:
      default:
        Particles_Stardust(this, 0.9f, 0.70f, PT_STAR08, 32);
        break;
      case KIT_AMONGOLD:
        Particles_Stardust(this, 1.6f, 1.00f, PT_STAR08, 32);
        break;
    }
  }

  // set health properties depending on type
  void SetProperties(void) {
    m_fRespawnTime = 10.0f;
    m_strDescription = GetKeyName(m_kitType);

    switch (m_kitType) {
      case KIT_ANKHWOOD:
        // set appearance
        AddItem(MODEL_ANKHWOOD, TEXTURE_ANKHWOOD, 0, 0, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;
      case KIT_ANKHROCK:
        // set appearance
        AddItem(MODEL_ANKHROCK, TEXTURE_ANKHROCK, 0, 0, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;
      case KIT_ANKHGOLD:
      case KIT_ANKHGOLDDUMMY:
        // set appearance
        AddItem(MODEL_ANKHGOLD, TEXTURE_ANKHGOLD, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_SPHINX1:
      case KIT_SPHINX2:
        // set appearance
        AddItem(MODEL_SPHINXGOLD, TEXTURE_SPHINXGOLD, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(2.0f, 2.0f, 2.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_AMONGOLD:
        // set appearance
        AddItem(MODEL_AMONGOLD, TEXTURE_AMONGOLD, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.5f,0), FLOAT3D(2,2,0.3f) );
        StretchItem(FLOAT3D(2.0f, 2.0f, 2.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_ELEMENTEARTH:
        // set appearance
        AddItem(MODEL_ELEMENTEARTH, TEXTURE_ELEMENTEARTH, TEX_REFL_METAL01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_ELEMENTAIR:
        // set appearance
        AddItem(MODEL_ELEMENTAIR, TEXTURE_ELEMENTAIR, TEX_REFL_METAL01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_ELEMENTWATER:
        // set appearance
        AddItem(MODEL_ELEMENTWATER, TEXTURE_ELEMENTWATER, TEX_REFL_METAL01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_ELEMENTFIRE:
        // set appearance
        AddItem(MODEL_ELEMENTFIRE, TEXTURE_ELEMENTFIRE, TEX_REFL_METAL01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;
  
      case KIT_RAKEY:
        // set appearance
        AddItem(MODEL_RAKEY, TEXTURE_RAKEY, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;
      case KIT_MOONKEY:
        // set appearance
        AddItem(MODEL_MOONKEY, TEXTURE_MOONKEY, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_EYEOFRA:
        // set appearance
        AddItem(MODEL_EYEOFRA, TEXTURE_EYEOFRA, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_SCARAB:
      case KIT_SCARABDUMMY:
        // set appearance
        AddItem(MODEL_SCARAB, TEXTURE_SCARAB, TEX_REFL_METAL01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_COBRA:
        // set appearance
        AddItem(MODEL_COBRA, TEXTURE_COBRA, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;

      case KIT_FEATHER:
        // set appearance
        AddItem(MODEL_FEATHER, TEXTURE_FEATHER, 0, 0, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;
      case KIT_HEART:
        // set appearance
        AddItem(MODEL_HEART, TEXTURE_HEART, TEX_REFL_GOLD01, TEX_SPEC_MEDIUM, 0);
        // add flare
        AddFlare(MODEL_FLARE, TEXTURE_FLARE, FLOAT3D(0,0.2f,0), FLOAT3D(1,1,0.3f) );
        StretchItem(FLOAT3D(1.0f, 1.0f, 1.0f));
        m_iSoundComponent = SOUND_KEY;
        break;
    }
  };

procedures:
  ItemCollected(EPass epass) : CItem::ItemCollected {
    ASSERT(epass.penOther!=NULL);

    // send key to entity
    EKey eKey;
    eKey.kitType = m_kitType;
    // if health is received
    if (epass.penOther->ReceiveItem(eKey)) {
      if(_pNetwork->IsPlayerLocal(epass.penOther)) {IFeel_PlayEffect("PU_Key");}
      // play the pickup sound
      m_soPick.Set3DParameters(50.0f, 1.0f, 1.0f, 1.0f);
      PlaySound(m_soPick, m_iSoundComponent, SOF_3D);
      m_fPickSoundLen = GetSoundLength(m_iSoundComponent);
      jump CItem::ItemReceived();
    }
    return;
  };

  Main() {
    Initialize();     // initialize base class
    StartModelAnim(ITEMHOLDER_ANIM_SMALLOSCILATION, AOF_LOOPING|AOF_NORESTART);
    ForceCollisionBoxIndexChange(ITEMHOLDER_COLLISION_BOX_SMALL);
    SetProperties();  // set properties

    jump CItem::ItemLoop();
  };
};
