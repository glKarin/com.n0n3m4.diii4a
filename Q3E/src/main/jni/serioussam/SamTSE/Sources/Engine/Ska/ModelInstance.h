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

#ifndef SE_INCL_MODELINSTANCE_H
#define SE_INCL_MODELINSTANCE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Ska/Skeleton.h>
#include <Engine/Ska/AnimSet.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Math/AABBox.h>
#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/DynamicContainer.h>
#include <Engine/Graphics/Shader.h>

// nubers are same as in models.h
#define SKA_HEIGHT_EQ_WIDTH 0
#define SKA_LENGTH_EQ_WIDTH 1
#define SKA_LENGTH_EQ_HEIGHT 2

// special assert for ska 
#ifdef SKADEBUG
  #ifdef NDEBUG
    #define SKAASSERT(__ignore) ((void)0)
  #else
    #define SKAASSERT(expr) ASSERT(expr)
  #endif
#else
  #define SKAASSERT(__ignore) ((void)0)
#endif

struct ColisionBox
{
  ColisionBox() {};
  ColisionBox(FLOAT3D vMin,FLOAT3D vMax) {
    SetMin(vMin);
    SetMax(vMax);
    SetName("Default");
  };
  inline FLOAT3D &Min() {return cb_vMin;}
  inline FLOAT3D &Max() {return cb_vMax;}
  inline void SetMin(const FLOAT3D &vMin) {cb_vMin = vMin;}
  inline void SetMax(const FLOAT3D &vMax) {cb_vMax = vMax;}
  inline void SetName(CTString strName) {
    cb_strName = strName;
    cb_iBoxID  = ska_GetIDFromStringTable(cb_strName);
  }
  inline const CTString &GetName() {return cb_strName;}
  inline const INDEX GetID() {return cb_iBoxID;}
private:
  FLOAT3D  cb_vMin;
  FLOAT3D  cb_vMax;
  CTString cb_strName;
  INDEX    cb_iBoxID;
};

struct MeshInstance
{
  CMesh *mi_pMesh;
  CStaticArray<struct TextureInstance> mi_tiTextures;
};

struct TextureInstance
{
  TextureInstance &operator=(const TextureInstance &tiOther) {
    ti_iTextureID = tiOther.ti_iTextureID;
    TextureInstance &ti = (TextureInstance&)tiOther;

    CTString strTexName = ti.ti_toTexture.GetName();
    ti_toTexture.SetData_t(strTexName);
    ti.ti_toTexture.SetData(NULL);
    return *this;
  }

  INDEX GetID() {return ti_iTextureID;}
  void  SetName(CTString &strTexID) {
    ti_iTextureID = ska_GetIDFromStringTable(strTexID);
  }

public:
  CTextureObject ti_toTexture;
private:
  INDEX ti_iTextureID;
};

struct AnimQueue
{
  CStaticStackArray<struct AnimList> aq_Lists;  // Array of currently playing anim lists
};

struct AnimList
{
  FLOAT al_fStartTime;  // Time when this list was created
  FLOAT al_fFadeTime;   // Time when this list will fade in
  CStaticStackArray<struct PlayedAnim> al_PlayedAnims;  // Array of currently playing anims in this list
};

struct PlayedAnim
{
  FLOAT pa_fStartTime; // Time when this animation was started
  FLOAT pa_fSpeedMul;  // Speed multiplier
  INDEX pa_iAnimID;    // Animation id
  ULONG pa_ulFlags;    // Animation flags
  FLOAT pa_Strength;   // Animation strength
  INDEX pa_GroupID;    // Group ID
};

class ENGINE_API CModelInstance
{
public:
  CModelInstance();
  ~CModelInstance();
  CModelInstance(CModelInstance &miOther);
  void operator=(CModelInstance &miOther);

  // Add child model instance
  void AddChild(CModelInstance *pmi, INDEX iParentBoneID = -1);
  // Add new mesh to model instance
  void AddMesh_t(CTFileName fnMesh);
  // Add new skeleton to model instance
  void AddSkeleton_t(CTFileName fnSkeleton);
  // Add new animset to model instance
  void AddAnimSet_t(CTFileName fnAnimSet);
  // Add new texture to model instance
  void AddTexture_t(CTFileName fnTexture, CTString strTexID, MeshInstance *pmshi);

  // Remove child model instance
  void RemoveChild(CModelInstance *pmi);
  // Remove one texture from model instance
  void RemoveTexture(TextureInstance *ptiRemove,MeshInstance *pmshi);

  // Find mesh instance in model instance
  MeshInstance    *FindMeshInstance(INDEX iMeshID);
  // Find texture instance in all mesh instances in model instance
  TextureInstance *FindTexureInstance(INDEX iTexID);
  // Find texture instance in given mesh instance
  TextureInstance *FindTexureInstance(INDEX iTexID, MeshInstance &mshi);


  // Copy mesh instance for other model instance
  void CopyMeshInstance(CModelInstance &miOther);
  // Get child of model instance
  CModelInstance *GetChild(INDEX iChildID, BOOL bRecursive = FALSE);
  // Set parent bone of model instance
  void SetParentBone(INDEX iParentBoneID);
  // Get parent of model instance
  CModelInstance *GetParent(CModelInstance *pmiStartFrom);
  // Get first parent that does not reference its child model instance
  CModelInstance *GetFirstNonReferencedParent(CModelInstance *pmiRoot);
  // Change parent of model instance
  void ChangeParent(CModelInstance *pmiOldParent, CModelInstance *pmiNewParent);

  // Model instance offsets from parent model
  void SetOffset(FLOAT fOffset[6]);
  void SetOffsetRot(ANGLE3D aRot);
  void SetOffsetPos(FLOAT3D vPos);
  FLOAT3D GetOffsetPos();
  ANGLE3D GetOffsetRot();

  // Stretch model instance
  void StretchModel(const FLOAT3D &vStretch);
  // Stretch model instance without attachments
  void StretchSingleModel(const FLOAT3D &vStretch);
  // Add new cloned anim state
  void NewClonedState(FLOAT fFadeTime);
  // Add new clear anim state
  void NewClearState(FLOAT fFadeTime);
  // Sets name of model instance
  void SetName(CTString strName);
  // Gets name of model instance
  const CTString &GetName();
  // Gets id of model instance
  const INDEX &GetID();

  // Add animation to last anim queue
  void AddAnimation(INDEX iAnimID, ULONG ulFlags, FLOAT fStrength, INDEX iGroupID, FLOAT fSpeedMul = 1.0f);
  // Remove all animations before last animation that has fully faded in
  void RemovePassedAnimsFromQueue(void);
  // Remove animation from anim queue
  void RemAnimation(INDEX iAnimID);
  // Remove all animations from anim queue with same ID
  void RemAnimsWithID(INDEX iGroupID);
  // Stop all animations in anim queue
  void StopAllAnimations(FLOAT fFadeTime);
  // Offset all animations in anim queue
  void OffSetAnimationQueue(TIME fOffsetTime);
  // Find animation by ID
  BOOL FindAnimationByID(int iAnimID, INDEX *piAnimSetIndex, INDEX *piAnimIndex);
  // Find first animation of all animations in ModelInstance (safety function)
  INDEX FindFirstAnimationID();
  // Get animation length
  FLOAT GetAnimLength(INDEX iAnimID);
  // Check if given animation is currently playing
  BOOL IsAnimationPlaying(INDEX iAnimID);
  // Add flags to animation playing in anim queue
  BOOL AddFlagsToPlayingAnim(INDEX iAnimID, ULONG ulFlags);

  // Model color
  COLOR &GetModelColor(void);
  void SetModelColor(COLOR colNewColor);

  BOOL HasAlpha(void);
  BOOL HasShadow(FLOAT fMipFactor);
  BOOL IsModelVisible( FLOAT fMipFactor);
  void AddSimpleShadow( const FLOAT fIntensity, const FLOATplane3D &plShadowPlane);
  void GetModelVertices( CStaticStackArray<FLOAT3D> &avVertices, FLOATmatrix3D &mRotation,FLOAT3D &vPosition, FLOAT fNormalOffset, FLOAT fDistance);

  // Colision boxes
  ColisionBox &GetColisionBox(INDEX icb);
  ColisionBox &GetCurrentColisionBox();
  void GetCurrentColisionBox(FLOATaabbox3D &paabbox);
  void GetAllFramesBBox(FLOATaabbox3D &aabbox);
  FLOAT3D GetCollisionBoxMin(INDEX iCollisionBox=0);
  FLOAT3D GetCollisionBoxMax(INDEX iCollisionBox=0);
  INDEX GetCollisionBoxDimensionEquality(INDEX iCollisionBox=0);
  INDEX GetColisionBoxIndex(INDEX iBoxID);
  // Add new colision box to model instance
  void AddColisionBox(CTString strName,FLOAT3D vMin,FLOAT3D vMax);
  // Remove colision box from model instance
  void RemoveColisionBox(INDEX iIndex);

  // Copy model instance data from other mi
  void Copy(CModelInstance &miOther);
  // Synchronize with another model (copy animations/attachments positions etc from there)
  void Synchronize(CModelInstance &miOther);
  // Clear model instance
  void Clear();
  // Flag for parser to remember source file names (used only in ska studio)
  static void EnableSrcRememberFN(BOOL bEnable);
  // Count used memory
  SLONG GetUsedMemory(void);

public:
  CSkeleton *mi_psklSkeleton;                     // pointer to skeleton object
  CStaticArray<struct MeshInstance> mi_aMeshInst; // array of mesh instances
  CStaticArray<struct ColisionBox> mi_cbAABox;    // array of colision boxes
  CDynamicContainer<class CAnimSet> mi_aAnimSet;  // array of animsets
  CDynamicContainer<class CModelInstance> mi_cmiChildren; // array of child model instances

  AnimQueue mi_aqAnims;   // current animation queue for this model instance
  QVect mi_qvOffset;      // current offset from parent model instance
  INDEX mi_iParentBoneID; // ID of parent bone in parent model instance
  INDEX mi_iCurentBBox;   // index of current colision box in colision box array
  COLOR mi_colModelColor; // color of this model instance
  FLOAT3D mi_vStretch;    // stretch of this model instance
  ColisionBox mi_cbAllFramesBBox; // all frames colision box
  CTFileName mi_fnSourceFile;     // source file name of this model instance (used only for ska studio)

private:
  INDEX mi_iModelID;      // ID of this model instance (this is ID for mi_strName)
  CTString mi_strName;    // name of this model instance
}; 

// Parse smc file in existing model instance
ENGINE_API void ParseSmcFile_t(CModelInstance &mi, const CTString &fnSmcFile);
// Create model instance and parse smc file in it
ENGINE_API CModelInstance *ParseSmcFile_t(const CTString &fnSmcFile);
// Create empty model instance 
ENGINE_API CModelInstance *CreateModelInstance(CTString strName);
// Delete model instance
ENGINE_API void DeleteModelInstance(CModelInstance *pmi);
// Calculate fading factor for animation list
ENGINE_API FLOAT CalculateFadeFactor(AnimList &alList);


#endif  /* include-once check. */
