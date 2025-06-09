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

#ifndef SE_INCL_EDITMODEL_H
#define SE_INCL_EDITMODEL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/CTString.h>
#include <Engine/Base/FileName.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Object3D.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Models/RenderModel.h>

#define MAX_MODELERTEXTURES		32
#define MAPPING_VERSION_WITHOUT_POLYGONS_PER_SURFACE "0001"
#define MAPPING_VERSION_WITHOUT_SOUNDS_AND_ATTACHMENTS "0002"
#define MAPPING_VERSION_WITHOUT_COLLISION "0003"
#define MAPPING_VERSION_WITHOUT_PATCHES "0004"
#define MAPPING_WITHOUT_SURFACE_COLORS "0005"
#define MAPPING_VERSION "0006"

class ENGINE_API CProgressRoutines
{
public:
  CProgressRoutines();
  void (*SetProgressMessage)( char *strMessage);      // sets message for modeler's "new progress dialog"
  void (*SetProgressRange)( INDEX iProgresSteps);     // sets range of modeler's "new progress dialog"
  void (*SetProgressState)( INDEX iCurrentStep);      // sets current modeler's "new progress dialog" state
};

ENGINE_API extern CProgressRoutines ProgresRoutines;

class ENGINE_API CTextureDataInfo
{
public:
  CListNode tdi_ListNode;
  CTextureData *tdi_TextureData;
  CTFileName tdi_FileName;
};

#define UNDO_VERTEX   0
#define UNDO_SURFACE  1

class ENGINE_API CMappingUndo
{
public:
  CListNode mu_ListNode;
  INDEX mu_Action;
  ModelTextureVertex *mu_ClosestVertex;
  FLOAT3D mu_UndoCoordinate;
  INDEX mu_iCurrentMip;
  INDEX mu_iCurrentSurface;
  FLOAT mu_Zoom;
  FLOAT3D mu_Center;
  FLOAT3D mu_HPB;
};

class ENGINE_API CAttachedModel {
public:
  BOOL am_bVisible;
  CModelObject am_moAttachedModel; // used as smart pointer (holds file name of attachment), never rendered
  CTString am_strName;
  INDEX am_iAnimation;

  CAttachedModel(void);
  ~CAttachedModel(void);
  void SetModel_t(CTFileName fnModel);
  void Read_t( CTStream *strFile); // throw char *
  void Write_t( CTStream *strFile); // throw char *
  void Clear(void); // clear the object.
};

class ENGINE_API CAttachedSound {
public:
  BOOL as_bLooping;
  BOOL as_bPlaying;
  FLOAT as_fDelay;
  CTFileName as_fnAttachedSound;

  CAttachedSound(void);
  void Read_t( CTStream *strFile); // throw char *
  void Write_t( CTStream *strFile); // throw char *
  void Clear(void) { as_fnAttachedSound = CTString("");};
};

class ENGINE_API CThumbnailSettings {
public:
  BOOL ts_bSet;
  CPlacement3D ts_plLightPlacement;
  CPlacement3D ts_plModelPlacement;
	FLOAT ts_fTargetDistance;
	FLOAT3D ts_vTarget;
	ANGLE3D ts_angViewerOrientation;
  FLOAT ts_LightDistance;
  COLOR ts_LightColor;
  COLOR ts_colAmbientColor;
	COLORREF ts_PaperColor;
	COLORREF ts_InkColor;
	BOOL ts_IsWinBcgTexture;
	CTFileName ts_WinBcgTextureName;
	CModelRenderPrefs ts_RenderPrefs;

  CThumbnailSettings( void);
  void Read_t( CTStream *strFile); // throw char *
  void Write_t( CTStream *strFile); // throw char *
};

class ENGINE_API CEditModel : public CSerial
{
private:
 	void NewModel(CObject3D *pO3D);									// creates new model, surface, vertice and polygon arrays
	void AddMipModel(CObject3D *pO3D);							// adds one mip model
  // loads and converts model's animation data from script file
	void LoadModelAnimationData_t( CTStream *pFile, const FLOATmatrix3D &mStretch);	// throw char *
  INDEX edm_iActiveCollisionBox;                  // collision box that is currently edited
public:
	CEditModel();																		// default contructor
	~CEditModel();																	// default destructor
  CModelData edm_md;															// edited model data
  CDynamicArray<CAttachedModel> edm_aamAttachedModels;// array of attached models
  CStaticArray<CAttachedSound> edm_aasAttachedSounds;// array of attached sounds
  CThumbnailSettings edm_tsThumbnailSettings;     // remembered parameters for taking thumbnail
  CListHead edm_WorkingSkins;	                // list of file names and texture data objects
  CListHead edm_UndoList;                         // list containing structures used for undo operation
  CListHead edm_RedoList;                         // list containing structures used for redo operation
  INDEX edm_Action;                               // type of last mapping change action (used by undo/redo)
  CTFileName edm_fnSpecularTexture;               // names of textures saved in ini file
  CTFileName edm_fnReflectionTexture;
  CTFileName edm_fnBumpTexture;
  // create empty attaching sounds
  void CreateEmptyAttachingSounds(void);
  // creates default script file
  void CreateScriptFile_t(CTFileName &fnFile);	  // throw char *
  // creates mip-model and mapping default constructios after it loads data from script
	void LoadFromScript_t(CTFileName &fnFileName); // throw char *
  // recalculate mapping for surface after some polygon has been added to surface
  void RecalculateSurfaceMapping( INDEX iMipModel, INDEX iSurface);
  // functions for extracting texture vertex links for given mip model
  void CalculateSurfaceCenterOffset( INDEX iCurrentMip, INDEX iSurface);
  void RemapTextureVerticesForSurface(INDEX iMip, INDEX iSurface, BOOL bJustCount);
  void RemapTextureVertices( INDEX iCurrentMip);
  // calculates mapping from three special 3D objects
  void CalculateUnwrappedMapping( CObject3D &o3dClosed, CObject3D &o3dOpened, CObject3D &o3dUnwrapped);
  // calculates mapping for mip models (except for main mip)
  void CalculateMappingForMips(void);
  // updates animations
  void UpdateAnimations_t(CTFileName &fnScriptName);	// throw char *
  // updates mip models configuration, looses their mapping !
	void UpdateMipModels_t(CTFileName &fnScriptName); // throw char *
  void CreateMipModels_t(CObject3D &objRestFrame, CObject3D &objMipSourceFrame,
    INDEX iVertexRemoveRate, INDEX iSurfacePreservingFactor);
	void DefaultMapping( INDEX iCurrentMip, INDEX iSurface=-1);// sets default mapping for given mip-model and surface (or all surfaces -1)
  void CalculateMapping( INDEX iCurrentMip, INDEX iSurfaceNo); // calculate mapping coordinates
  void CalculateMappingAll( INDEX iCurrentMip);
  void DrawWireSurface( CDrawPort *pDP, INDEX iCurrentMip, INDEX iCurrentSurface,
       FLOAT fMagnifyFactor, PIX offx, PIX offy, COLOR clrVisible, COLOR clrInvisible); // draws given surface in wire frame
  void DrawFilledSurface( CDrawPort *pDP, INDEX iCurrentMip, INDEX iCurrentSurface,
       FLOAT fMagnifyFactor, PIX offx, PIX offy, COLOR clrVisible, COLOR clrInvisible); // fills given surface with color
  void PrintSurfaceNumbers( CDrawPort *pDP, CFontData *pFont, INDEX iCurrentMip,
       FLOAT fMagnifyFactor, PIX offx, PIX offy, COLOR clrInk); // prints surface numbers
  void ExportSurfaceNumbersAndNames( CTFileName fnFile);
  void GetScreenSurfaceCenter( CDrawPort *pDP, INDEX iCurrentMip,
       INDEX iCurrentSurface, FLOAT fMagnifyFactor, PIX offx, PIX offy,
       PIX &XCenter, PIX &YCenter); // Calculates given surface's center point in view's window (in pixels)
  FLOATaabbox3D CalculateSurfaceBBox( INDEX iCurrentMip, // used to calculate mapping coordinate's bbox
        INDEX iSurfaceNo, FLOAT3D f3Position, FLOAT3D f3Angle, FLOAT fZoom);
  void CalculateMapping();                        // used to calculate mapping coordinates
  BOOL ChangeSurfacePositionRelative( INDEX iCurrentMip, INDEX iCurrentSurface, FLOAT fDZoom,
         FLOAT dX, FLOAT dY, PIX dTH, PIX dTV);   // if it can, sets surface position with given offseted values
  BOOL ChangeSurfacePositionAbsolute( INDEX iCurrentMip, INDEX iCurrentSurface,
         FLOAT newZoom, FLOAT3D newCenter, FLOAT3D newAngle); // if it can, sets surface position to given values
  void RememberSurfaceUndo( INDEX iCurrentMip, INDEX iCurrentSurface);   // remember undo position
  ModelTextureVertex *FindClosestMappingVertex( INDEX iCurrentMip, INDEX iCurrentSurface,
    PIX ptx, PIX pty, FLOAT fMagnifyFactor, PIX offx, PIX offy);// finds closest point and returns it
  void MoveVertex( ModelTextureVertex *pmtvClosestVertex, PIX dx, PIX dy, INDEX iCurrentMip, INDEX iCurrentSurface); // try new vertex position
  void RememberVertexUndo( ModelTextureVertex *pmtvClosestVertex);// remember coordinate change into undo buffer
  // add one texture to list of working textures
  CTextureDataInfo *AddTexture_t(const CTFileName &fnFileName, const MEX mexWidth,
        const MEX mexHeight); // throw char *
  FLOAT3D GetSurfacePos(INDEX iCurrentMip, INDEX iCurrentSurface); // Retrieves given surface's position
  FLOAT3D GetSurfaceAngles(INDEX iCurrentMip, INDEX iCurrentSurface); // Retrieves given surface's angles
  FLOAT GetSurfaceZoom(INDEX iCurrentMip, INDEX iCurrentSurface); // Retrieves given surface's zoom factor
  const char *GetSurfaceName(INDEX iCurrentMip, INDEX iCurrentSurface); // Retrieves given surface's name
  void Undo(void); // Undoes last operation
  void Redo(void); // Redoes last operation
	void RemoveTexture(char *pFileName);						// removes one of working textures in modeler app
  void MovePatchRelative( INDEX iMaskBit, MEX2D mexOffset);
  void SetPatchStretch( INDEX iMaskBit, FLOAT fNewStretch);
  BOOL EditAddPatch( CTFileName fnPatchName, MEX2D mexPos, INDEX &iMaskBit); // Adds one patch
  void EditRemovePatch( INDEX iMaskBit);            // Removes given patch
  void EditRemoveAllPatches( void);
  INDEX CountPatches(void);
  ULONG GetExistingPatchesMask(void);
  BOOL GetFirstEmptyPatchIndex( INDEX &iMaskBit);   // Finds first empty space ready to recieve new patch
  BOOL GetFirstValidPatchIndex( INDEX &iMaskBit);   // Finds first valid patch index
  void GetPreviousValidPatchIndex( INDEX &iMaskBit);// Sets previous valid patch index
  void GetNextValidPatchIndex( INDEX &iMaskBit);    // Sets next valid patch index
  void CalculatePatchesPerPolygon(void);
  INDEX GetMipCt(){ return edm_md.md_MipCt;};       // Returns number of mip models
  MEX GetWidth(){ return edm_md.md_Width;};         // Returns allowed width for model's texture
  MEX GetHeight(){ return edm_md.md_Height;};       // Returns allowed height for model's texture
  // collision box handling functions
  FLOAT3D &GetCollisionBoxMin(void);
  FLOAT3D &GetCollisionBoxMax(void);
  void AddCollisionBox(void);
  void DeleteCurrentCollisionBox(void);
  INDEX GetActiveCollisionBoxIndex(void) { return edm_iActiveCollisionBox;};
  void ActivatePreviousCollisionBox(void);
  void ActivateNextCollisionBox(void);
  void SetCollisionBox(FLOAT3D vMin, FLOAT3D vMax);
  CTString GetCollisionBoxName(INDEX iCollisionBox);
  CTString GetCollisionBoxName(void);
  void SetCollisionBoxName(CTString strNewName);
  void CorrectCollisionBoxSize(void);
  // returns HEIGHT_EQ_WIDTH, LENGTH_EQ_WIDTH or LENGTH_EQ_HEIGHT
  INDEX GetCollisionBoxDimensionEquality();
  // set new collision box equality value
  void SetCollisionBoxDimensionEquality( INDEX iNewDimEqType);
  // overloaded load function
	void Load_t( CTFileName fnFileName); // throw char *
  // overloaded save function
	void Save_t( CTFileName fnFileName); // throw char *
  // exports .h file (#define ......)
  void SaveIncludeFile_t( CTFileName fnFileName, CTString strDefinePrefix);  // throw char *

  // know how to load
	void Read_t( CTStream *istrFile); // throw char *
  // and save modeler data (i.e. vindow positions, view prefs, texture file names...)
	void Write_t( CTStream *ostrFile); // throw char *

  // load and save mapping data for whole model (iMip = -1) or just for one mip model
  void LoadMapping_t( CTFileName fnFileName, INDEX iMip = -1);
  void SaveMapping_t( CTFileName fnFileName, INDEX iMip = -1);
  // read and write settings for given mip
  void ReadMipSettings_t( CTStream *istrFile, INDEX iMip);  // throw char *
	void WriteMipSettings_t( CTStream *ostrFile, INDEX iMip);
};


#endif  /* include-once check. */

