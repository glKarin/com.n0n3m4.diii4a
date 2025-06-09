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

#ifndef SE_INCL_WORLD_H
#define SE_INCL_WORLD_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Synchronization.h>
#include <Engine/Base/CTString.h>
#include <Engine/Base/Lists.h>
#include <Engine/Brushes/Brush.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Math/Placement.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/DynamicContainer.h>

class CTextureTransformation;
class CTextureBlending;
class CSurfaceType;
class CContentType;
class CEnvironmentType;
class CIlluminationType;

// mirroring types for mirror and stretch
enum WorldMirrorType {
  WMT_NONE,
  WMT_X,
  WMT_Y,
  WMT_Z,
};

class ENGINE_API CWorld {
public:
// implementation:
  // current transformations for animating texture mapping
  CStaticArray<CTextureTransformation> wo_attTextureTransformations;
  // current blendings for animating texture blending
  CStaticArray<CTextureBlending> wo_atbTextureBlendings;
  // polygon surface types
  CStaticArray<CSurfaceType> wo_astSurfaceTypes;
  // sector content types
  CStaticArray<CContentType> wo_actContentTypes;
  // sector environment types
  CStaticArray<CEnvironmentType> wo_aetEnvironmentTypes;
    // illumination types (0 is not used)
  CStaticArray<CIlluminationType> wo_aitIlluminationTypes;

  CEntityClass *wo_pecWorldBaseClass;   // world base class (used for some special features)

  CBrushArchive &wo_baBrushes;    // brush archive with all brushes in the world
  CTerrainArchive &wo_taTerrains; // terrain archive with all terrains in the world
  CDynamicContainer<CEntity> wo_cenAllEntities;  // all entities including deleted but referenced ones
  CDynamicContainer<CEntity> wo_cenPredictable;  // predictable entities
  CDynamicContainer<CEntity> wo_cenWillBePredicted;  // entities that will be predicted
  CDynamicContainer<CEntity> wo_cenPredicted;  // predicted entities
  CDynamicContainer<CEntity> wo_cenPredictor;  // predictor entities

  class CCollisionGrid *wo_pcgCollisionGrid;

  COLOR wo_colBackground;                 // background color of this world
  CEntityPointer wo_penBackgroundViewer;  // viewer entity for background rendering

  CTString wo_strBackdropUp;  // upper backdrop image
  CTString wo_strBackdropFt;  // front backdrop image
  CTString wo_strBackdropRt;  // right backdrop image
  CTString wo_strBackdropObject; // 3d object that is rendered as backdrop
  FLOAT wo_fUpW, wo_fUpL, wo_fUpCX, wo_fUpCZ; // size and position of upper backdrop image
  FLOAT wo_fFtW, wo_fFtH, wo_fFtCX, wo_fFtCY; // size and position of front backdrop image
  FLOAT wo_fRtL, wo_fRtH, wo_fRtCZ, wo_fRtCY; // size and position of right backdrop image

  CPlacement3D wo_plFocus; // focus placement in WED at the moment of world saving
  FLOAT wo_fTargetDistance;// target distance in WED at the moment of world saving
  CPlacement3D wo_plThumbnailFocus; // focus placement for thumbnail in WED
  FLOAT wo_fThumbnailTargetDistance;// target distance for thumbnail in WED

  CTFileName wo_fnmFileName;  // the file that the world was loaded from
  SLONG wo_slStateDictionaryOffset;   // offset of the world state filename dictionary
  CTString wo_strName;    // name of the level to be shown to player
  ULONG wo_ulSpawnFlags;  // spawn flags telling in which game modes can the level be played
  CTString wo_strDescription; // description of the level (intro, mission, etc.)

  ULONG wo_ulNextEntityID;    // next free ID for entities
  CListHead wo_lhTimers;      // timer scheduled entities
  CListHead wo_lhMovers;        // entities that want to/have to move
  BOOL wo_bPortalLinksUpToDate; // set if portal-sector links are up to date

  /* Initialize collision grid. */
  void InitCollisionGrid(void);
  /* Destroy collision grid. */
  void DestroyCollisionGrid(void);
  /* Clear collision grid. */
  void ClearCollisionGrid(void);

  /* Add an entity to cell(s) in collision grid. */
  void AddEntityToCollisionGrid(CEntity *pen, const FLOATaabbox3D &boxEntity);
  /* Remove an entity from cell(s) in collision grid. */
  void RemoveEntityFromCollisionGrid(CEntity *pen, const FLOATaabbox3D &boxEntity);
  /* Move an entity inside cell(s) in collision grid. */
  void MoveEntityInCollisionGrid(CEntity *pen,
    const FLOATaabbox3D &boxOld, const FLOATaabbox3D &boxNew);
  /* Find all entities in collision grid near given box. */
  void FindEntitiesNearBox(const FLOATaabbox3D &boxNear,
    CStaticStackArray<CEntity*> &apenNearEntities);

  /* Create a new entity of given class. */
  CEntity *CreateEntity(const CPlacement3D &plPlacement, CEntityClass *pecClass);
  /* Clear all entity pointers that point to this entity. */
  void UntargetEntity(CEntity *penToUntarget);

  /* Add an entity to list of timers. */
  void AddTimer(CRationalEntity *penTimer);
  // set overdue timers to be due in current time
  void AdjustLateTimers(TIME tmCurrentTime);

  // CSG operations for editing a world
  /* Get a valid brush mip of brush for use in CSG operations. */
  CBrushMip *GetBrushMip(CEntity &enBrush);
  /* Copy selected sectors of a source brush to a 3D object. */
  void CopySourceBrushSectorsToObject(
    CEntity &enBrush,
    CBrushSectorSelectionForCSG &bscselSectors,
    const CPlacement3D &plSourcePlacement,
    CObject3D &obObject,
    const CPlacement3D &plTargetPlacement,
    DOUBLEaabbox3D &boxSourceAbsolute
    );
  /* Move sectors of a target brush that are affected, to a 3D object. */
  void MoveTargetBrushPartToObject(
    CEntity &enBrush,
    DOUBLEaabbox3D &boxAffected,
    CObject3D &obObject
    );
  /* Add 3D object sectors to a brush. */
  void AddObjectToBrush(CObject3D &obObject, CEntity &enBrush);
  /* Do some CSG operation with one brush in this world and one brush in other world. */
  void DoCSGOperation(
    CEntity &enThis,
    CWorld &woOther,
    CEntity &enOther,
    const CPlacement3D &plOther,
    void (CObject3D::*DoCSGOpenSector)(CObject3D &obA, CObject3D &obB),
    void (CObject3D::*DoCSGClosedSectors)(CObject3D &obA, CObject3D &obB)
    );

  /* Copy all entities except one from another world to this one. */
  void CopyAllEntitiesExceptOne(CWorld &woOther, CEntity &enExcepted,
    const CPlacement3D &plOtherSystem);
  /* Join two sectors from one brush-mip together. */
  CBrushSector *JoinTwoSectors(CBrushSector &bscA, CBrushSector &bscB);
  /* Split one sector by a 3D object. */
  void SplitOneSector(CBrushSector &bscToSplit, CObject3D &obToSplitBy);

  // read/write world information (description, name, flags...)
  void ReadInfo_t(CTStream *strm, BOOL bMaybeDescription); // throw char *
  void WriteInfo_t(CTStream *strm); // throw char *

  // mark all predictable entities that will be predicted using user-set criterions
  void MarkForPrediction(void);
  // unmark all predictable entities marked for prediction
  void UnmarkForPrediction(void);
  // create predictors for predictable entities that are marked for prediction
  void CreatePredictors(void);
  // delete all predictor entities
  void DeletePredictors(void);

  // get entity by its ID
  CEntity *EntityFromID(ULONG ulID);
  // triangularize selected polygons
  void TriangularizePolygons(CDynamicContainer<CBrushPolygon> &dcPolygons);
public:
// interface:
  CDynamicContainer<CEntity> wo_cenEntities;           // all entities in the world

  TIME wo_WorldGameTick;  // game tick that world is currently in

  /* Constructor. */
  CWorld(void);
  /* Destructor. */
  ~CWorld(void);
  DECLARE_NOCOPYING(CWorld);
  /* Clear all arrays. */
  void Clear(void);

  /* Lock all arrays. */
  void LockAll(void);
  /* Unlock all arrays. */
  void UnlockAll(void);

  /* Create a new entity of given class. */
  CEntity *CreateEntity_t(const CPlacement3D &plPlacement,
    const CTFileName &fnmClass); // throw char *
  /* Copy one entity from another world into this one. */
  CEntity *CopyOneEntity(CEntity &enToCopy, const CPlacement3D &plOtherSystem);
  /* Copy container of entities from another world to this one and select them. */
  void CopyEntities(CWorld &woOther, CDynamicContainer<CEntity> &cenToCopy,
    CEntitySelection &senCopied, const CPlacement3D &plOtherSystem);
  /* Copy entity in world. */
  CEntity *CopyEntityInWorld(CEntity &enOriginal, const CPlacement3D &plOtherEntity,
    BOOL bWithDescendants = TRUE);
  /* Destroy a selection of entities - reserved for use in WEd only! */
  void DestroyEntities(CEntitySelection &senToDestroy);
  /* Destroy given entity - reserved for use in WEd only! */
  void DestroyOneEntity( CEntity *penToDestroy);  
  /* Find an entity with given character. */
  CPlayerEntity *FindEntityWithCharacter(CPlayerCharacter &pcCharacter);

  /* Copy entities for prediction. */
  void CopyEntitiesToPredictors(CDynamicContainer<CEntity> &cenToCopy);

  /* Cast a ray and see what it hits. */
  void CastRay(CCastRay &crRay);
  /* Continue to cast already cast ray */
  void ContinueCast(CCastRay &crRay);
  /* Test if a movement is clipped by something and where. */
  void ClipMove(CClipMove &cmMove);

  /* Set background color for this world. */
  void SetBackgroundColor(COLOR colBackground);
  /* Get background color for this world. */
  COLOR GetBackgroundColor(void);
  /* Set background viewer entity for this world. */
  void SetBackgroundViewer(CEntity *penEntity);
  /* Get background viewer entity for this world. */
  CEntity *GetBackgroundViewer(void);

  /* Set description for this world. */
  void SetDescription(const CTString &strDescription);
  /* Get description for this world. */
  const CTString &GetDescription(void);

  // get/set name of the world
  void SetName(const CTString &strName);
  const CTString &GetName(void);
  // get/set spawn flags for the world
  void SetSpawnFlags(ULONG ulFlags);
  ULONG GetSpawnFlags(void);

  /* Save entire world (both brushes and current state). */
  void Save_t(const CTFileName &fnmWorld); // throw char *
  /* Load entire world (both brushes and current state). */
  void Load_t(const CTFileName &fnmWorld); // throw char *
  /* Reinitialize entities from their properties. (use only in WEd!) */
  void ReinitializeEntities(void);
  /* Precache data needed by entities. */
  void PrecacheEntities_t(void);  // throw char *
  // delete all entities that don't fit given spawn flags
  void FilterEntitiesBySpawnFlags(ULONG ulFlags);

  /* Read entire world (both brushes and current state). */
  void Read_t(CTStream *pistrm); // throw char *
  /* Write entire world (both brushes and current state). */
  void Write_t(CTStream *postrm); // throw char *

  /* Load just world brushes from a file with entire world information. */
  void LoadBrushes_t(const CTFileName &fnmWorld); // throw char *

  /* Update sectors after brush vertex moving */
  void UpdateSectorsAfterVertexChange( CBrushVertexSelection &selVertex);
  /* Update sectors during brush vertex moving */
  void UpdateSectorsDuringVertexChange( CBrushVertexSelection &selVertex);
  /* Triangularize polygons that contain vertices from given selection */
  void TriangularizeForVertices( CBrushVertexSelection &selVertex);
  /* Clears 'marked for use' flag on all polygons in world */
  void ClearMarkedForUseFlag(void);
  
  // CSG operations for editing worlds
  /* Add a brush from another world to a brush in this world, other entities get copied. */
  void CSGAdd(CEntity &enThis, CWorld &woOther, CEntity &enOther, const CPlacement3D &plOther);
  void CSGAddReverse(CEntity &enThis, CWorld &woOther, CEntity &enOther, const CPlacement3D &plOther);
  /* Substract a brush from another world to a brush in this world. */
  void CSGRemove(CEntity &enThis, CWorld &woOther, CEntity &enOther,
    const CPlacement3D &plOther);
  /* Split selected sectors in a brush in this world with one brush in other world.
   * (other brush must have only one sector.)
   */
  void SplitSectors(CEntity &enThis, CBrushSectorSelection &selbscSectorsToSplit,
    CWorld &woOther, CEntity &enOther, const CPlacement3D &plOther);
  /* If two or more sectors can be joined. */
  BOOL CanJoinSectors(CBrushSectorSelection &selbscSectorsToJoin);
  /* Join two or more sectors from one brush-mip together. */
  void JoinSectors(CBrushSectorSelection &selbscSectorsToJoin);

  /* Split selected polygons in a brush in this world with one brush in other world.
   * (other brush must have only one sector.)
   */
  void SplitPolygons(CEntity &enThis, CBrushPolygonSelection &selbpoPolygonsToSplit,
    CWorld &woOther, CEntity &enOther, const CPlacement3D &plOther);
  /* Join two or more polygons from one brush-sector together. */
  void JoinPolygons(CBrushPolygonSelection &selbpoPolygonsToJoin);
  /* Test if a polygon selection can be joined. */
  BOOL CanJoinPolygons(CBrushPolygonSelection &selbpoPolygonsToJoin);
  /* Join all selected polygons that can be joined. */
  void JoinAllPossiblePolygons(CBrushPolygonSelection &selbpoPolygonsToJoin,
    BOOL bPreserveTextures, INDEX iTexture);
  /* Test if a polygon selection can be joined. */
  BOOL CanJoinAllPossiblePolygons(CBrushPolygonSelection &selbpoPolygonsToJoin);

  /* Copy selected sectors from one brush to a new entity in another world. */
  BOOL CanCopySectors(CBrushSectorSelection &selbscSectorsToCopy);
  void CopySectors(CBrushSectorSelection &selbscSectorsToCopy, CEntity *penTarget, BOOL bWithEntities);
  /* Delete selected sectors. */
  void DeleteSectors(CBrushSectorSelection &selbscSectorsToDelete, BOOL bClearPortalFlags);
  void CopyPolygonInWorld(CBrushPolygon &bpoSrc, CBrushSector &bsc, INDEX iPol, INDEX &iEdg, INDEX &iVtx);
  void CopyPolygonsToBrush(CBrushPolygonSelection &selPolygons, CEntity *penbr);
  void DeletePolygons(CDynamicContainer<CBrushPolygon> &dcPolygons);
  void CreatePolygon(CBrushVertexSelection &selVtx);
  void FlipPolygon(CBrushPolygon &bpo);
 
  // mirror and stretch another world into this one
  void MirrorAndStretch(CWorld &woOriginal, FLOAT fStretch, enum WorldMirrorType wmt);

  // hide/show functions
  /* Hide entities contained in given selection. */
  void HideSelectedEntities(CEntitySelection &selenEntitiesToHide);
  /* Hide all unselected entities. */
  void HideUnselectedEntities(void);
  /* Show all entities. */
  void ShowAllEntities(void);
  /* Hide sectors contained in given selection. */
  void HideSelectedSectors(CBrushSectorSelection &selbscSectorsToHide);
  /* Hide all unselected sectors. */
  void HideUnselectedSectors(void);
  /* Show all sectors. */
  void ShowAllSectors(void);

  /* Selecting by texture in selected sectors */
  void SelectByTextureInSelectedSectors( CTFileName fnTexture, CBrushPolygonSelection &selbpoSimilar, INDEX iTexture);
  /* Selecting by texture in world */
  void SelectByTextureInWorld( CTFileName fnTexture, CBrushPolygonSelection &selbpoSimilar, INDEX iTexture);

  // shadow manipulation functions
  /* Recalculate all shadow maps that are not valid or of smaller precision. */
  void CalculateDirectionalShadows(void);
  void CalculateNonDirectionalShadows(void);
  /* Find all shadow layers near a certain position. */
  void FindShadowLayers(const FLOATaabbox3D &boxNear, BOOL bSelectedOnly=FALSE, BOOL bDirectional = TRUE);
  /* Discard shadows on all brush polygons in the world. */
  void DiscardAllShadows(void);

  // create links between zoning brush sectors and non-zoning entities in sectors
  void LinkEntitiesToSectors(void);

  // rebuild all links in world
  void RebuildLinks(void);

  /* Read world brushes from stream. */
  void ReadBrushes_t( CTStream *istr);   // throw char *
  /* Write world brushes to stream. */
  void WriteBrushes_t( CTStream *ostr);  // throw char *

  /* Read current world state from stream. */
  void ReadState_t( CTStream *istr);   // throw char *
  void ReadState_veryold_t( CTStream *istr);   // throw char *
  void ReadState_old_t( CTStream *istr);   // throw char *
  void ReadState_new_t( CTStream *istr);   // throw char *
  /* Write current world state to stream. */
  void WriteState_t( CTStream *ostr, BOOL bImportDictionary = FALSE);  // throw char *
};


#endif  /* include-once check. */

