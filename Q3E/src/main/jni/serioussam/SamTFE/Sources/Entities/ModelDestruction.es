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

217
%{
#include "Entities/StdH/StdH.h"
%}

uses "Entities/ModelHolder2";
uses "Entities/BasicEffects";
uses "Entities/Debris";
uses "Entities/BloodSpray";

// event sent to entities in range of model destroy
// (e.g light can turn off)
event ERangeModelDestruction {
};

// type of debris
enum DestructionDebrisType {
  1 DDT_STONE "Stone",
  2 DDT_WOOD  "Wood",
  3 DDT_PALM  "Palm",
};

class CModelDestruction : CEntity {
name      "ModelDestruction";
thumbnail "Thumbnails\\ModelDestruction.tbn";
features  "HasName", "IsTargetable", "IsImportant";

properties:
  1 CTString m_strName          "Name" 'N' = "ModelDestruction",
  2 CTString m_strDescription = "",

 10 CEntityPointer m_penModel0  "Model 0" 'M' COLOR(C_RED|0x00),
 11 CEntityPointer m_penModel1  "Model 1" COLOR(C_RED|0x00),
 12 CEntityPointer m_penModel2  "Model 2" COLOR(C_RED|0x00),
 13 CEntityPointer m_penModel3  "Model 3" COLOR(C_RED|0x00),
 14 CEntityPointer m_penModel4  "Model 4" COLOR(C_RED|0x00),

 20 FLOAT m_fHealth "Health" 'H' = 50.0f,   // health of the model pointing to this
 22 enum DestructionDebrisType m_ddtDebris "Debris" 'D' = DDT_STONE,  // type of debris
 23 INDEX m_ctDebris "Debris Count" = 3,
 24 FLOAT m_fDebrisSize "Debris Size" = 1.0f,
 25 enum EntityInfoBodyType m_eibtBodyType "Body Type" = EIBT_ROCK,
 26 enum SprayParticlesType m_sptType "Particle Type" = SPT_NONE, // type of particles

components:
  1 model   MODEL_MODELDESTRUCTION     "Models\\Editor\\ModelDestruction.mdl",
  2 texture TEXTURE_MODELDESTRUCTION   "Models\\Editor\\ModelDestruction.tex",
  3 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** WOOD PARTS **************
 10 model     MODEL_WOOD        "Models\\Effects\\Debris\\Wood01\\Wood.mdl",
 11 texture   TEXTURE_WOOD      "Models\\Effects\\Debris\\Wood01\\Wood.tex",

// ************** STONE PARTS **************
 14 model     MODEL_STONE        "Models\\Effects\\Debris\\Stone\\Stone.mdl",
 15 texture   TEXTURE_STONE      "Models\\Effects\\Debris\\Stone\\Stone.tex",

functions:
  void Precache(void) {
    PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
    switch(m_ddtDebris) {
    case DDT_STONE: { 
      PrecacheModel(MODEL_STONE);
      PrecacheTexture(TEXTURE_STONE);
                          } break;
    case DDT_WOOD:   {  
      PrecacheModel(MODEL_WOOD);
      PrecacheTexture(TEXTURE_WOOD);
                          } break;
    case DDT_PALM:   {  
      PrecacheModel(MODEL_WOOD);
      PrecacheTexture(TEXTURE_WOOD);
                          } break;
    }
  };

  const CTString &GetDescription(void) const {
    INDEX ct = GetModelsCount();
    if(ct==0) {
      ((CTString&)m_strDescription).PrintF("(%g): no more", m_fHealth);
    } else if(ct==1) {
      ((CTString&)m_strDescription).PrintF("(%g): %s", m_fHealth, (const char*)m_penModel0->GetName());
    } else if (TRUE) {
      ((CTString&)m_strDescription).PrintF("(%g): %s,...(%d)", m_fHealth, (const char*)m_penModel0->GetName(), ct);
    }
    return m_strDescription;
  }

  // check if one model target is valid 
  void CheckOneModelTarget(CEntityPointer &pen)
  {
    if (pen!=NULL && !IsOfClass(pen, "ModelHolder2")) {
      WarningMessage("Model '%s' is not ModelHolder2!", (const char*)pen->GetName());
      pen=NULL;
    }
  }

  // get next phase in destruction
  class CModelHolder2 *GetNextPhase(void)
  {
    INDEX ct = GetModelsCount();
    // if not more models
    if (ct==0) {
      // return none
      return NULL;
    // if there are some
    } else {
      // choose by random
      return GetModel(IRnd()%ct);
    }
  }

  // get number of models set by user
  INDEX GetModelsCount(void) const
  {
    // note: only first N that are no NULL are used
    if (m_penModel0==NULL) { return 0; };
    if (m_penModel1==NULL) { return 1; };
    if (m_penModel2==NULL) { return 2; };
    if (m_penModel3==NULL) { return 3; };
    if (m_penModel4==NULL) { return 4; };
    return 5;
  }
  // get model by its index
  class CModelHolder2 *GetModel(INDEX iModel)
  {
    ASSERT(iModel<=GetModelsCount());
    iModel = Clamp(iModel, INDEX(0), GetModelsCount());
    return (CModelHolder2 *) (&m_penModel0)[iModel].ep_pen;
  }
  // spawn debris for given model
  void SpawnDebris(CModelHolder2 *penModel)
  {
    FLOATaabbox3D box;
    penModel->GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm();
    FLOAT fSize = m_fDebrisSize;
    switch(m_ddtDebris) {
    case DDT_STONE: {
      Debris_Begin(EIBT_ROCK, DPT_NONE, BET_NONE, fEntitySize, FLOAT3D(0,0,0), FLOAT3D(0,0,0), 1.0f, 0.0f);
      for(INDEX iDebris = 0; iDebris<m_ctDebris; iDebris++) {
        Debris_Spawn(penModel, this, MODEL_STONE, TEXTURE_STONE, 0, 0, 0, IRnd()%4, fSize,
          FLOAT3D(FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f, FRnd()*0.8f+0.1f));
      }
                    } break;
    case DDT_WOOD: {
      Debris_Begin(EIBT_WOOD, DPT_NONE, BET_NONE, fEntitySize, FLOAT3D(0,0,0), FLOAT3D(0,0,0), 1.0f, 0.0f);
      for(INDEX iDebris = 0; iDebris<m_ctDebris; iDebris++) {
        Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 0, fSize,
          FLOAT3D(0.5f, 0.5f, 0.5f));
      }
                    } break;
    case DDT_PALM: {
      Debris_Begin(EIBT_WOOD, DPT_NONE, BET_NONE, fEntitySize, penModel->m_vDamage*0.3f, FLOAT3D(0,0,0), 1.0f, 0.0f);
      Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 0, fSize,
        FLOAT3D(0.5f, 0.2f, 0.5f));
      Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 1, fSize,
        FLOAT3D(0.5f, 0.3f, 0.5f));
      Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 2, fSize,
        FLOAT3D(0.5f, 0.4f, 0.5f));
      Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 3, fSize,
        FLOAT3D(0.5f, 0.5f, 0.5f));
      Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 1, fSize,
        FLOAT3D(0.5f, 0.6f, 0.5f));
      Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 2, fSize,
        FLOAT3D(0.5f, 0.8f, 0.5f));
      Debris_Spawn(penModel, this, MODEL_WOOD, TEXTURE_WOOD, 0, 0, 0, 1, fSize,
        FLOAT3D(0.5f, 0.9f, 0.5f));
                    } break;
    default: {} break;
    };
  }

procedures:
  Main()
  {
    // must not allow invalid classes
    CheckOneModelTarget(m_penModel0);
    CheckOneModelTarget(m_penModel1);
    CheckOneModelTarget(m_penModel2);
    CheckOneModelTarget(m_penModel3);
    CheckOneModelTarget(m_penModel4);

    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MODELDESTRUCTION);
    SetModelMainTexture(TEXTURE_MODELDESTRUCTION);

    return;
  }
};

