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

#include "Engine/StdH.h"

#include <Engine/Base/ErrorReporting.h>
#include <Engine/World/World.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Rendering/Render.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Math/Float.h>
#include <Engine/Math/Object3D.h>
#include <Engine/Math/Projection_DOUBLE.h>
#include <Engine/Templates/Selection.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Math/Geometry.inl>

// assure that floating point precision is 53 bits
void AssureFPT_53(void)
{
  if (GetFPUPrecision()!=FPT_53BIT) {
    ASSERTALWAYS( "Floating precision must be set to 53 bits during CSG!");
    SetFPUPrecision(FPT_53BIT);
  }
}


/////////////////////////////////////////////////////////////////////
// CWorld editing operations
/*
 * Get a valid brush mip of brush for use in CSG operations.
 */
CBrushMip *CWorld::GetBrushMip(CEntity &enBrush)
{
  // the entity must be brush
  ASSERT(enBrush.en_RenderType == CEntity::RT_BRUSH ||
    enBrush.en_RenderType == CEntity::RT_FIELDBRUSH);

  // get the brush
  CBrush3D &brBrush = *enBrush.en_pbrBrush;

  // get relevant mip as if in manual mip brushing mode
  CBrushMip *pbm = brBrush.GetBrushMipByDistance(_wrpWorldRenderPrefs.GetManualMipBrushingFactor());
  return pbm;
}

/*
 * Copy selected sectors of a source brush to a 3D object.
 */
void CWorld::CopySourceBrushSectorsToObject(
  CEntity &enBrush,
  CBrushSectorSelectionForCSG &bscselSectors,
  const CPlacement3D &plSourcePlacement,
  CObject3D &obObject,
  const CPlacement3D &plTargetPlacement,
  DOUBLEaabbox3D &boxSourceAbsolute
  )
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // get the brush mip from the entity
  CBrushMip &bmBrushMip = *GetBrushMip(enBrush);

  // calculate placement of the brush in absolute space (taking relative
  // world placement and entity placement in account)
  CPlacement3D plBrush = enBrush.en_plPlacement;
  plBrush.RelativeToAbsolute(plSourcePlacement);

  // copy selected sectors of brush to object3d object
  bmBrushMip.ToObject3D(obObject, bscselSectors);

  // make a copy of the object and find its box in absolute space
  CObject3D obAbsolute;
  obAbsolute = obObject;
  CSimpleProjection3D_DOUBLE prToAbsolute;
  prToAbsolute.ObjectPlacementL() = plBrush;
  prToAbsolute.ViewerPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  prToAbsolute.Prepare();
  obAbsolute.Project(prToAbsolute);
  obAbsolute.GetBoundingBox(boxSourceAbsolute);

  // project the brush into target space
  CSimpleProjection3D_DOUBLE prSimple;
  prSimple.ObjectPlacementL() = plBrush;
  prSimple.ViewerPlacementL() = plTargetPlacement;
  prSimple.Prepare();
  obObject.Project(prSimple);
}

/*
 * Move sectors of a target brush that are affected, to a 3D object.
 */
void CWorld::MoveTargetBrushPartToObject(
  CEntity &enBrush,
  DOUBLEaabbox3D &boxAffected,
  CObject3D &obObject
  )
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  // get the brush mip from the entity
  CBrushMip &bmBrushMip = *GetBrushMip(enBrush);

  // copy those sectors of brush touching given bbox to 3D object
  CBrushSectorSelectionForCSG bscselSectors;
  bmBrushMip.SelectSectorsInRange(bscselSectors, DOUBLEtoFLOAT(boxAffected));
  bmBrushMip.ToObject3D(obObject, bscselSectors);
  bmBrushMip.DeleteSelectedSectors(bscselSectors);
  // if no sectors are moved this way
  if (obObject.ob_aoscSectors.Count()==0) {
    // move the open sector to object
    CBrushSectorSelectionForCSG bscselOpen;
    bmBrushMip.SelectOpenSector(bscselOpen);
    bmBrushMip.ToObject3D(obObject, bscselOpen);
    bmBrushMip.DeleteSelectedSectors(bscselOpen);
  }
}

/*
 * Add 3D object sectors to a brush.
 */
void CWorld::AddObjectToBrush(CObject3D &obObject, CEntity &enBrush)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_ADDOBJECTTOBRUSH);

  // get the brush mip from the entity
  CBrushMip &bmBrushMip = *GetBrushMip(enBrush);

  // return the result to the source brush
  try {
    bmBrushMip.AddFromObject3D_t(obObject);
  } catch (const char *strError) {
    FatalError("Unexpected error during CSG operation: %s", strError);
  }
  // update the bounding boxes of the brush
  bmBrushMip.UpdateBoundingBox();
  //bmBrushMip.bm_pbrBrush->CalculateBoundingBoxes();
  // find possible shadow layers near affected area
  FindShadowLayers(bmBrushMip.bm_boxBoundingBox);

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_ADDOBJECTTOBRUSH);
}

/*
 * Do some CSG operation with one brush in this world and one brush in other world.
 */
void CWorld::DoCSGOperation(
    CEntity &enThis,
    CWorld &woOther,
    CEntity &enOther,
    const CPlacement3D &plOther,
    void (CObject3D::*DoCSGOpenSector)(CObject3D &obA, CObject3D &obB),
    void (CObject3D::*DoCSGClosedSectors)(CObject3D &obA, CObject3D &obB)
    )
{
  // assure that floating point precision is 53 bits
  AssureFPT_53();

  // get relevant brush mips in each brush
  CBrushMip *pbmThis  = GetBrushMip(enThis);
  CBrushMip *pbmOther = GetBrushMip(enOther);
  if (pbmThis==NULL || pbmOther==NULL) {
    return;
  }

  // get open sector of the other brush to object
  CBrushSectorSelectionForCSG selbscOtherOpen;
  pbmOther->SelectOpenSector(selbscOtherOpen);
  CObject3D obOtherOpen;
  DOUBLEaabbox3D boxOtherOpen;
  woOther.CopySourceBrushSectorsToObject(enOther, selbscOtherOpen, plOther,
    obOtherOpen, enThis.en_plPlacement, boxOtherOpen);

  // if there is an open sector in other object
  if (obOtherOpen.ob_aoscSectors.Count()>0) {
    CObject3D obResult;
    // deportalize the open sector
    obOtherOpen.TurnPortalsToWalls();

    // if there are any sectors in this brush
    if (pbmThis->bm_abscSectors.Count()>0) {
      // move affected part of this brush to an object3d object
      CObject3D obThis;
      MoveTargetBrushPartToObject(enThis, boxOtherOpen, obThis);
      // do the open sector CSG operation on the objects
      _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_OBJECTCSG);
      (obResult.*DoCSGOpenSector)(obThis, obOtherOpen);
      _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_OBJECTCSG);

    // if there are no sectors in this brush
    } else {
      // just put the open sector directly in the result
      obResult = obOtherOpen;
    }
    // return the result back to this brush
    AddObjectToBrush(obResult, enThis);
  }

  // get closed sectors of the other brush to object
  CBrushSectorSelectionForCSG selbscOtherClosed;
  pbmOther->SelectClosedSectors(selbscOtherClosed);
  CObject3D obOtherClosed;
  DOUBLEaabbox3D boxOtherClosed;
  woOther.CopySourceBrushSectorsToObject(enOther, selbscOtherClosed, plOther,
    obOtherClosed, enThis.en_plPlacement, boxOtherClosed);

  // if there are closed sectors in other object
  if (obOtherClosed.ob_aoscSectors.Count()>0) {
    CObject3D obResult;

    // if there are any sectors in this brush
    if (pbmThis->bm_abscSectors.Count()>0) {
      // move affected part of this brush to an object3d object
      CObject3D obThis;
      MoveTargetBrushPartToObject(enThis, boxOtherClosed, obThis);
      // do the closed sectors CSG operation on the objects
      _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_OBJECTCSG);
      (obResult.*DoCSGClosedSectors)(obThis, obOtherClosed);
      _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_OBJECTCSG);

    // if there are no sectors in this brush
    } else {
      // just put the closed sectors directly in the result
      obResult = obOtherClosed;
    }
    // return the result back to this brush
    AddObjectToBrush(obResult, enThis);
  }
}

/*
 * Add a brush from another world to a brush in this world, other entities get copied.
 */
void CWorld::CSGAdd(CEntity &enThis, CWorld &woOther, CEntity &enOther,
                    const CPlacement3D &plOther)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();
  // do add material for open sectors and then do add rooms for closed sectors
  DoCSGOperation(enThis, woOther, enOther, plOther,
    &CObject3D::CSGAddMaterial, &CObject3D::CSGAddRooms);

  // copy all other entities
  CopyAllEntitiesExceptOne(woOther, enOther, plOther);

  // if the world doesn't have all portal-sector links updated
  if (!wo_bPortalLinksUpToDate) {
    // update the links
    wo_baBrushes.LinkPortalsAndSectors();
    wo_bPortalLinksUpToDate = TRUE;
  }

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}
/*
 * Add a brush from another world to a brush in this world, other entities get copied.
 */
void CWorld::CSGAddReverse(CEntity &enThis, CWorld &woOther, CEntity &enOther,
                    const CPlacement3D &plOther)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();
  // do add material reverse for open sectors and then do add rooms for closed sectors
  DoCSGOperation(enThis, woOther, enOther, plOther,
    &CObject3D::CSGAddMaterialReverse, &CObject3D::CSGAddRooms);

  // copy all other entities
  CopyAllEntitiesExceptOne(woOther, enOther, plOther);

  // if the world doesn't have all portal-sector links updated
  if (!wo_bPortalLinksUpToDate) {
    // update the links
    wo_baBrushes.LinkPortalsAndSectors();
    wo_bPortalLinksUpToDate = TRUE;
  }

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}

/*
 * Substract a brush from another world to a brush in this world.
 */
void CWorld::CSGRemove(CEntity &enThis, CWorld &woOther, CEntity &enOther,
  const CPlacement3D &plOther)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();
  // assure that floating point precision is 53 bits
  AssureFPT_53();

  // get relevant brush mip in other brush
  CBrushMip *pbmOther = GetBrushMip(enOther);
  if (pbmOther==NULL) {
    return;
  }

  // if other brush has more than one sector
  if (pbmOther->bm_abscSectors.Count()>1) {
    // join all sectors of the other brush together
    CBrushSectorSelection selbscOtherAll;
    pbmOther->SelectAllSectors(selbscOtherAll);
    woOther.JoinSectors(selbscOtherAll);
  }

  // do 'remove material' with joined brush
  DoCSGOperation(enThis, woOther, enOther, plOther,
    &CObject3D::CSGRemoveMaterial, &CObject3D::CSGRemoveMaterial);

  // if the world doesn't have all portal-sector links updated
  if (!wo_bPortalLinksUpToDate) {
    // update the links
    wo_baBrushes.LinkPortalsAndSectors();
    wo_bPortalLinksUpToDate = TRUE;
  }
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}

/*
 * Split one sector by a 3D object.
 */
void CWorld::SplitOneSector(CBrushSector &bscToSplit, CObject3D &obToSplitBy)
{
  // get the brush mip from sector to split
  CBrushMip *pbmMip = bscToSplit.bsc_pbmBrushMip;

  // create object to split from sector to split and destroy the sector
  CBrushSectorSelectionForCSG selbscToSplit;
  selbscToSplit.Select(bscToSplit);
  CObject3D obToSplit;
  pbmMip->ToObject3D(obToSplit, selbscToSplit);
  pbmMip->DeleteSelectedSectors(selbscToSplit);
  // copy ambient value from the sector to split to the sector to split with
  obToSplitBy.ob_aoscSectors.Lock();
  obToSplitBy.ob_aoscSectors[0].osc_colAmbient = bscToSplit.bsc_colAmbient;
  obToSplitBy.ob_aoscSectors[0].osc_colColor = bscToSplit.bsc_colColor;
  obToSplitBy.ob_aoscSectors[0].osc_ulFlags[0] = bscToSplit.bsc_ulFlags;
  obToSplitBy.ob_aoscSectors[0].osc_ulFlags[1] = bscToSplit.bsc_ulFlags2;
  obToSplitBy.ob_aoscSectors[0].osc_ulFlags[2] = bscToSplit.bsc_ulVisFlags;
  obToSplitBy.ob_aoscSectors.Unlock();

  // do 'split sectors' CSG with the objects
  CObject3D obResult;
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_OBJECTCSG);
  obResult.CSGSplitSectors(obToSplit, obToSplitBy);
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_OBJECTCSG);

  // return the result to the source brush mip
  try {
    pbmMip->AddFromObject3D_t(obResult);
  } catch (const char *strError) {
    FatalError("Unexpected error during split sectors operation: %s", strError);
  }
}

/*
 * Split selected sectors in a brush in this world with one brush in other world.
 * (other brush must have only one sector)
 */
void CWorld::SplitSectors(CEntity &enThis, CBrushSectorSelection &selbscSectorsToSplit,
  CWorld &woOther, CEntity &enOther, const CPlacement3D &plOther)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();
  // assure that floating point precision is 53 bits
  AssureFPT_53();

  // get relevant brush mip in this brush
  CBrushMip *pbmThis = GetBrushMip(enThis);
  if (pbmThis==NULL) {
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
    return;
  }

  // get relevant brush mip in other brush
  CBrushMip *pbmOther = GetBrushMip(enOther);
  if (pbmOther==NULL) {
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
    return;
  }

  /* Assure that the other brush has only one sector. */

  // if other brush has more than one sector
  if (pbmOther->bm_abscSectors.Count()>1) {
    // join all sectors of the other brush together
    CBrushSectorSelection selbscOtherAll;
    pbmOther->SelectAllSectors(selbscOtherAll);
    woOther.JoinSectors(selbscOtherAll);
  }

  /* Split selected sectors with the one sector in the other brush. */

  // get the sector of the other brush to object
  CBrushSectorSelectionForCSG selbscOther;
  pbmOther->SelectAllSectors(selbscOther);
  CObject3D obOther;
  DOUBLEaabbox3D boxOther;
  woOther.CopySourceBrushSectorsToObject(enOther, selbscOther, plOther,
    obOther, enThis.en_plPlacement, boxOther);

  // if the selection is empty
  if (selbscSectorsToSplit.Count()==0) {
    // select all sectors near the splitting tool
    pbmThis->SelectSectorsInRange(selbscSectorsToSplit, DOUBLEtoFLOAT(boxOther));
  }
  // for all sectors in the selection
  FOREACHINDYNAMICCONTAINER(selbscSectorsToSplit, CBrushSector, itbsc) {
    // split the sector using the copy of other object
    CObject3D obj(obOther);
    SplitOneSector(*itbsc, obj);
  }

  // update the bounding boxes of this brush
  pbmThis->bm_pbrBrush->CalculateBoundingBoxes();
  // find possible shadow layers near affected area
  FindShadowLayers(DOUBLEtoFLOAT(boxOther));

  /* NOTE: we must not clear the selection directly, since the sectors
     contained there are already freed and deselecting them would make an access
     violation.
   */
  // clear the selection on the container level
  selbscSectorsToSplit.CDynamicContainer<CBrushSector>::Clear();
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}

/*
 * Join two sectors from one brush-mip together.
 */
CBrushSector *CWorld::JoinTwoSectors(CBrushSector &bscA, CBrushSector &bscB)
{
  // get the brush mip from sector A
  CBrushMip *pbmMip = bscA.bsc_pbmBrushMip;
  // the brush mip must be same for sector B
  ASSERT(pbmMip == bscB.bsc_pbmBrushMip);

  // remember sector properties of sector A
  COLOR colColor   = bscA.bsc_colColor;
  COLOR colAmbient = bscA.bsc_colAmbient;

  INDEX ctAPolygons = bscA.bsc_abpoPolygons.Count();

  // create object A from sector A and destroy sector A
  CBrushSectorSelectionForCSG selbscA;
  selbscA.Select(bscA);
  CObject3D obA;
  pbmMip->ToObject3D(obA, selbscA);
  pbmMip->DeleteSelectedSectors(selbscA);

  // create object B from sector B and destroy sector B
  CBrushSectorSelectionForCSG selbscB;
  selbscB.Select(bscB);
  CObject3D obB;
  pbmMip->ToObject3D(obB, selbscB);
  pbmMip->DeleteSelectedSectors(selbscB);

  CObject3D obResult;
  // if sector A has got at least one polygon
  if (ctAPolygons>0) {
    // do 'join sectors' CSG with objects A and B
    _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_OBJECTCSG);
    obResult.CSGJoinSectors(obA, obB);
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_OBJECTCSG);

  // if sector A has got no polygons
  // (this happens due to algorithm used in CWorld::JoinSectors())
  } else {
    // just make result be object B
    obResult = obB;
  }
  // return the result to the source brush mip
  CBrushSector *pbscResult;
  try {
    pbscResult = pbmMip->AddFromObject3D_t(obResult);
  } catch (const char *strError) {
    FatalError("Unexpected error during join sectors operation: %s", strError);
  }

  if( obResult.ob_aoscSectors.Count() == 0) return NULL;

  // set sector properties of the resulting sector to be same as in former sector A
  pbscResult->bsc_colColor   = colColor;
  pbscResult->bsc_colAmbient = colAmbient;

  // return the resulting sector (there is only one)
  return pbscResult;
}

/*
 * If two or more sectors can be joined
 */
BOOL CWorld::CanJoinSectors(CBrushSectorSelection &selbscSectorsToJoin)
{
  if (selbscSectorsToJoin.Count()<2) return FALSE;
  // get the brush mip from first sector in selection
  selbscSectorsToJoin.Lock();
  CBrushSector *pbscFirstSector = &selbscSectorsToJoin[0];
  selbscSectorsToJoin.Unlock();
  CBrushMip *pbmMip = pbscFirstSector->bsc_pbmBrushMip;

  // for all sectors in the selection
  FOREACHINDYNAMICCONTAINER(selbscSectorsToJoin, CBrushSector, itbsc) {
    // if sectors are in the same mip
    if( itbsc->bsc_pbmBrushMip != pbmMip) return FALSE;
  }
  return TRUE;
}

/*
 * Join two or more sectors from one brush-mip together.
 */
// this doesn't have to be member function of CWorld, but is here for future enhancements
void CWorld::JoinSectors(CBrushSectorSelection &selbscSectorsToJoin)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();
  // get the brush mip from first sector in selection
  selbscSectorsToJoin.Lock();
  CBrushSector *pbscFirstSector = &selbscSectorsToJoin[0];
  selbscSectorsToJoin.Unlock();
  CBrushMip *pbmMip = pbscFirstSector->bsc_pbmBrushMip;

  // create an empty result sector in the brush mip
  CBrushSector *pbscResult = pbmMip->bm_abscSectors.New(1);
  pbscResult->bsc_pbmBrushMip = pbmMip;
  // give it same properties as the first selected sector
  pbscResult->bsc_colColor = pbscFirstSector->bsc_colColor;
  pbscResult->bsc_colAmbient = pbscFirstSector->bsc_colAmbient;

  // for all sectors in the selection
  FOREACHINDYNAMICCONTAINER(selbscSectorsToJoin, CBrushSector, itbsc) {
    // join the sector with the result sector
    pbscResult = JoinTwoSectors(*pbscResult, *itbsc);
    if( pbscResult == NULL) break;
  }

  // update the bounding boxes of the brush
  pbmMip->bm_pbrBrush->CalculateBoundingBoxes();
  // find possible shadow layers near affected area
  FindShadowLayers(pbmMip->bm_boxBoundingBox);

  /* NOTE: we must not clear the selection directly, since the sectors
     contained there are already freed and deselecting them would make an access
     violation.
   */
  // clear the selection on the container level
  selbscSectorsToJoin.CDynamicContainer<CBrushSector>::Clear();
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}

/*
 * Split selected polygons in a brush in this world with one brush in other world.
 * (other brush must have only one sector.)
 */
void CWorld::SplitPolygons(CEntity &enThis, CBrushPolygonSelection &selbpoPolygonsToSplit,
  CWorld &woOther, CEntity &enOther, const CPlacement3D &plOther)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();

  // get relevant brush mip in other brush
  CBrushMip *pbmOther = GetBrushMip(enOther);
  if (pbmOther==NULL) {
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
    return;
  }

  selbpoPolygonsToSplit.Lock();
  INDEX ctSelectedPolygons = selbpoPolygonsToSplit.Count();

  // get the brush sectors from all polygons in selection
  CDynamicContainer<CBrushSector> cbscSectors;
  {for(INDEX iselbpo=0; iselbpo<ctSelectedPolygons; iselbpo++) {
    CBrushSector &bsc = *selbpoPolygonsToSplit[iselbpo].bpo_pbscSector;
    if (!cbscSectors.IsMember(&bsc)) {
      cbscSectors.Add(&bsc);
    }
  }}
  // clear the polygon selection on the container level (keep selection flags)
  selbpoPolygonsToSplit.CDynamicContainer<CBrushPolygon>::Clear();

  // get the sector of the other brush to object
  CBrushSectorSelectionForCSG selbscOther;
  pbmOther->SelectAllSectors(selbscOther);
  CObject3D obOther;
  DOUBLEaabbox3D boxOther;
  woOther.CopySourceBrushSectorsToObject(enOther, selbscOther, plOther,
    obOther, enThis.en_plPlacement, boxOther);

  // for each sector
  cbscSectors.Lock();
  INDEX ctSectors = cbscSectors.Count();
  {for(INDEX ibsc=0; ibsc<ctSectors; ibsc++) {
    CBrushSector *pbscSector = &cbscSectors[ibsc];
    CBrushMip *pbmMip = pbscSector->bsc_pbmBrushMip;
    // move the sector to 3D object
    CBrushSectorSelectionForCSG bscselSector;
    bscselSector.Select(*pbscSector);
    CObject3D obSector;
    pbmMip->ToObject3D(obSector, bscselSector);
    pbmMip->DeleteSelectedSectors(bscselSector);
    // split selected polygons in the object
    CObject3D obResult;
    obResult.CSGSplitPolygons(obSector, obOther);

    // return it back to brush mip
    try {
      pbmMip->AddFromObject3D_t(obResult);
    } catch (const char *strError) {
      FatalError("Unexpected error during split polygons operation: %s", strError);
    }

    // update the bounding boxes of the brush
    pbmMip->bm_pbrBrush->CalculateBoundingBoxes();
    // find possible shadow layers near affected area
    FindShadowLayers(pbmMip->bm_boxBoundingBox);
  }}

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}

/*
 * Test if a sector selection can be joined.
 */
BOOL CWorld::CanJoinPolygons(CBrushPolygonSelection &selbpoPolygonsToJoin)
{
  // if selection has less than two polygons
  if (selbpoPolygonsToJoin.Count()<2) {
    // it cannot be joined
    return FALSE;
  }

  // get the brush sector and plane from the first polygon in selection
  selbpoPolygonsToJoin.Lock();
  CBrushPolygon *pbpoFirstPolygon = &selbpoPolygonsToJoin[0];
  selbpoPolygonsToJoin.Unlock();
  CBrushSector *pbscSector = pbpoFirstPolygon->bpo_pbscSector;
  CBrushPlane *pbplPlane = pbpoFirstPolygon->bpo_pbplPlane;

  // for each polyon in selection
  {FOREACHINDYNAMICCONTAINER(selbpoPolygonsToJoin, CBrushPolygon, itbpo) {
    // if it has different sector or plane
    if (itbpo->bpo_pbscSector!=pbscSector || itbpo->bpo_pbplPlane!=pbplPlane) {
      // it cannot be joined
      return FALSE;
    }
  }}

  // if all tests are passed, it can be joined
  return TRUE;
}

/*
 * Join two or more polygons from one brush-sector together.
 */
void CWorld::JoinPolygons(CBrushPolygonSelection &selbpoPolygonsToJoin)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();

  // do not try do this if it is not possible
  if (!CanJoinAllPossiblePolygons(selbpoPolygonsToJoin)) {
    ASSERT(FALSE);
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
    return;
  }

  selbpoPolygonsToJoin.Lock();
  INDEX ctSelectedPolygons = selbpoPolygonsToJoin.Count();

  // get the brush sectors from all polygons in selection
  CDynamicContainer<CBrushSector> cbscSectors;
  {for(INDEX iselbpo=0; iselbpo<ctSelectedPolygons; iselbpo++) {
    CBrushSector &bsc = *selbpoPolygonsToJoin[iselbpo].bpo_pbscSector;
    if (!cbscSectors.IsMember(&bsc)) {
      cbscSectors.Add(&bsc);
    }
  }}

  // repeat
  BOOL bSomeJoined;
  do {
    bSomeJoined = FALSE;

    // for each polygon in selection
    {for(INDEX iselbpo0=0; iselbpo0<ctSelectedPolygons; iselbpo0++) {
      CBrushPolygon &bpo0 = selbpoPolygonsToJoin[iselbpo0];
      // for each polygon in selection after that one
      {for(INDEX iselbpo1=iselbpo0+1; iselbpo1<ctSelectedPolygons; iselbpo1++) {
        CBrushPolygon &bpo1 = selbpoPolygonsToJoin[iselbpo1];
        // if it has no edges
        if (bpo1.bpo_abpePolygonEdges.Count()==0) {
          // skip it
          continue;
        }
        // if the two polygons can be joined
        if (bpo0.bpo_pbplPlane==bpo1.bpo_pbplPlane) {
          // move all edges of second polygon to the first one
          bpo0.MovePolygonEdges(bpo1);
          bpo1.bpo_abpePolygonEdges.Clear();
          bSomeJoined = TRUE;
        }
      }}
    }}
  // while some polygons were joined
  } while (bSomeJoined);
  // clear the polygon selection
  selbpoPolygonsToJoin.Unlock();
  selbpoPolygonsToJoin.Clear();

  // for each sector
  cbscSectors.Lock();
  INDEX ctSectors = cbscSectors.Count();
  {for(INDEX ibsc=0; ibsc<ctSectors; ibsc++) {
    CBrushSector *pbscSector = &cbscSectors[ibsc];
    CBrushMip *pbmMip = pbscSector->bsc_pbmBrushMip;
    // move the sector to 3D object
    CBrushSectorSelectionForCSG bscselSector;
    bscselSector.Select(*pbscSector);
    CObject3D obSector;
    pbmMip->ToObject3D(obSector, bscselSector);
    pbmMip->DeleteSelectedSectors(bscselSector);

    // return it back to brush mip
    try {
      pbmMip->AddFromObject3D_t(obSector);
    } catch (const char *strError) {
      FatalError("Unexpected error during join polygons operation: %s", strError);
    }

    // update the bounding boxes of the brush
    pbmMip->bm_pbrBrush->CalculateBoundingBoxes();
    // find possible shadow layers near affected area
    FindShadowLayers(pbmMip->bm_boxBoundingBox);
  }}

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}

/* Test if a polygon selection can be joined. */
BOOL CWorld::CanJoinAllPossiblePolygons(CBrushPolygonSelection &selbpoPolygonsToJoin)
{
  // if selection has less than two polygons
  if (selbpoPolygonsToJoin.Count()<2) {
    // it cannot be joined
    return FALSE;
  }
  // if all tests are passed, it can be joined
  return TRUE;
}

struct JoinedPolygon {
  INDEX jp_iTarget;
  INDEX jp_ctEdges;
};

/* Join all selected polygons that can be joined. */
void CWorld::JoinAllPossiblePolygons(
  CBrushPolygonSelection &selbpoPolygonsToJoin, BOOL bPreserveTextures, INDEX iTexture)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();

  // do not try do this if it is not possible
  if (!CanJoinAllPossiblePolygons(selbpoPolygonsToJoin)) {
    ASSERT(FALSE);
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
    return;
  }

  selbpoPolygonsToJoin.Lock();
  INDEX ctSelectedPolygons = selbpoPolygonsToJoin.Count();

  // get the brush sectors from all polygons in selection
  CDynamicContainer<CBrushSector> cbscSectors;
  {for(INDEX iselbpo=0; iselbpo<ctSelectedPolygons; iselbpo++) {
    CBrushSector &bsc = *selbpoPolygonsToJoin[iselbpo].bpo_pbscSector;
    if (!cbscSectors.IsMember(&bsc)) {
      cbscSectors.Add(&bsc);
    }
  }}

  // repeat
  BOOL bSomeJoined;
  do {
    bSomeJoined = FALSE;

    // for each polygon in selection
    {for(INDEX iselbpo0=0; iselbpo0<ctSelectedPolygons; iselbpo0++) {
      CBrushPolygon &bpo0 = selbpoPolygonsToJoin[iselbpo0];
      // for each polygon in selection after that one
      {for(INDEX iselbpo1=iselbpo0+1; iselbpo1<ctSelectedPolygons; iselbpo1++) {
        CBrushPolygon &bpo1 = selbpoPolygonsToJoin[iselbpo1];
        // if it has no edges
        if (bpo1.bpo_abpePolygonEdges.Count()==0) {
          // skip it
          continue;
        }
        // if the two polygons can be joined
        if (bpo0.bpo_pbplPlane==bpo1.bpo_pbplPlane
          &&((bpo0.bpo_ulFlags&(OPOF_PORTAL|BPOF_PORTAL))==(bpo1.bpo_ulFlags&(OPOF_PORTAL|BPOF_PORTAL)))
          &&(!bPreserveTextures ||
          bpo0.bpo_abptTextures[iTexture].bpt_toTexture.GetData()
          == bpo1.bpo_abptTextures[iTexture].bpt_toTexture.GetData())
          &&bpo0.TouchesInSameSector(bpo1)) {
          // move all edges of second polygon to the first one
          bpo0.MovePolygonEdges(bpo1);
          bpo1.bpo_abpePolygonEdges.Clear();
          bSomeJoined = TRUE;
        }
      }}
    }}
  // while some polygons were joined
  } while (bSomeJoined);
  // clear the polygon selection
  selbpoPolygonsToJoin.Unlock();
  selbpoPolygonsToJoin.Clear();

  // for each sector
  cbscSectors.Lock();
  INDEX ctSectors = cbscSectors.Count();
  {for(INDEX ibsc=0; ibsc<ctSectors; ibsc++) {
    CBrushSector *pbscSector = &cbscSectors[ibsc];
    CBrushMip *pbmMip = pbscSector->bsc_pbmBrushMip;
    // move the sector to 3D object
    CBrushSectorSelectionForCSG bscselSector;
    bscselSector.Select(*pbscSector);
    CObject3D obSector;
    pbmMip->ToObject3D(obSector, bscselSector);
    pbmMip->DeleteSelectedSectors(bscselSector);

    // return it back to brush mip
    try {
      pbmMip->AddFromObject3D_t(obSector);
    } catch (const char *strError) {
      FatalError("Unexpected error during join all polygons operation: %s", strError);
    }

    // update the bounding boxes of the brush
    pbmMip->bm_pbrBrush->CalculateBoundingBoxes();
    // find possible shadow layers near affected area
    FindShadowLayers(pbmMip->bm_boxBoundingBox);
  }}

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
}

/* Copy selected sectors from one brush to a new entity in another world. */
BOOL CWorld::CanCopySectors(CBrushSectorSelection &selbscSectorsToCopy)
{
  if (selbscSectorsToCopy.Count()<1) return FALSE;
  // get the brush mip from first sector in selection
  selbscSectorsToCopy.Lock();
  CBrushSector *pbscFirstSector = &selbscSectorsToCopy[0];
  selbscSectorsToCopy.Unlock();
  CBrushMip *pbmMip = pbscFirstSector->bsc_pbmBrushMip;

  // for all sectors in the selection
  FOREACHINDYNAMICCONTAINER(selbscSectorsToCopy, CBrushSector, itbsc) {
    // if sectors are not in the same mip
    if( itbsc->bsc_pbmBrushMip != pbmMip) return FALSE;
  }
  return TRUE;
}

void CWorld::CopySectors(CBrushSectorSelection &selbscSectorsToCopy, CEntity *penTarget,
                         BOOL bWithEntities)
{
  // destination must be empty brush
  ASSERT(penTarget->IsEmptyBrush());

  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CSGTOTAL);
  _pfWorldEditingProfile.IncrementAveragingCounter();
  // do not try do this if it is not possible
  if (!CanCopySectors(selbscSectorsToCopy)) {
    ASSERT(FALSE);
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CSGTOTAL);
    return;
  }
  // assure that floating point precision is 53 bits
  AssureFPT_53();

  // get the brush mip from first sector in selection
  selbscSectorsToCopy.Lock();
  CBrushSector *pbscFirstSector = &selbscSectorsToCopy[0];
  selbscSectorsToCopy.Unlock();
  CBrushMip *pbmMip = pbscFirstSector->bsc_pbmBrushMip;
  CEntity *penSource = pbmMip->bm_pbrBrush->br_penEntity;

  // get destination mip
  CBrushMip *pbmDestination = penTarget->en_pbrBrush->GetFirstMip();
  if (pbmDestination==NULL) {
    ASSERT(FALSE);
    return;
  }

  // copy selected sectors of brush to object3d object
  CObject3D obObject;
  pbmMip->ToObject3D(obObject, selbscSectorsToCopy);
  // get bounding box of the selection
  DOUBLEaabbox3D boxSelection;
  obObject.GetBoundingBox(boxSelection);
  // find the bottom center of the selection
  FLOAT3D vCenter = DOUBLEtoFLOAT(boxSelection.Center());
  vCenter(2) = DOUBLEtoFLOAT(boxSelection.Min()(2));
  // snap the center to 1m grid
  Snap(vCenter(1), 1.0);
  Snap(vCenter(2), 1.0);
  Snap(vCenter(3), 1.0);
  CPlacement3D plCenter(vCenter, ANGLE3D(0,0,0));
  CPlacement3D plCenterInverse(-vCenter, ANGLE3D(0,0,0));
  // make projection from source entity to center of selection
  CSimpleProjection3D_DOUBLE prToCenter;
  prToCenter.ObjectPlacementL() = penSource->en_plPlacement;
  prToCenter.ViewerPlacementL() = plCenter;
  prToCenter.Prepare();
  obObject.Project(prToCenter);
  // convert the result to brush in target
  AddObjectToBrush(obObject, *penTarget);

  if( bWithEntities)
  {
    // make a container of entities to copy
    CDynamicContainer<CEntity> cenToCopy;
    // for each of the selected sectors
    {FOREACHINDYNAMICCONTAINER(selbscSectorsToCopy, CBrushSector, itbsc) {
      // for each entity in the sector
      {FOREACHDSTOFSRC(itbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
        // if it is not in container
        if (!cenToCopy.IsMember(pen)) {
          // add it to container
          cenToCopy.Add(pen);
        }
      ENDFOR}
    }}

    // copy all entities in the container to destination world
    CEntitySelection senCopied;
    penTarget->en_pwoWorld->CopyEntities(*this, cenToCopy, senCopied, plCenterInverse);
    senCopied.Clear();
  }

  // if the world doesn't have all portal-sector links updated
  if (!penTarget->en_pwoWorld->wo_bPortalLinksUpToDate) {
    // update the links
    penTarget->en_pwoWorld->wo_baBrushes.LinkPortalsAndSectors();
    penTarget->en_pwoWorld->wo_bPortalLinksUpToDate = TRUE;
  }

  // if any portals are hanging (what is probable), fix them
  GetBrushMip(*penTarget)->RemoveDummyPortals(TRUE);

  // if the world doesn't have all portal-sector links updated
  if (!penTarget->en_pwoWorld->wo_bPortalLinksUpToDate) {
    // update the links
    penTarget->en_pwoWorld->wo_baBrushes.LinkPortalsAndSectors();
    penTarget->en_pwoWorld->wo_bPortalLinksUpToDate = TRUE;
  }
}

/* Delete selected sectors. */
void CWorld::DeleteSectors(CBrushSectorSelection &selbscSectorsToDelete, BOOL bClearPortalFlags)
{
  CBrushMip *pbm = NULL;
  // for each sector in the selection
  {FOREACHINDYNAMICCONTAINER(selbscSectorsToDelete, CBrushSector, itbsc) {
    // delete it from the brush mip
    pbm = itbsc->bsc_pbmBrushMip;
    pbm->bm_abscSectors.Delete(itbsc);
  }}
  // clear the selection on the container level
  selbscSectorsToDelete.CDynamicContainer<CBrushSector>::Clear();

  if (pbm==NULL) {
    return;
  }

  // if any portals are left hanging (what is probable), fix them
  pbm->RemoveDummyPortals(bClearPortalFlags);

  // if the world doesn't have all portal-sector links updated
  CWorld *pwo = pbm->bm_pbrBrush->br_penEntity->en_pwoWorld;
  if (!pwo->wo_bPortalLinksUpToDate) {
    // update the links
    pwo->wo_baBrushes.LinkPortalsAndSectors();
    pwo->wo_bPortalLinksUpToDate = TRUE;
  }
}

void CheckOnePolygon(CBrushSector &bsc, CBrushPolygon &bpo)
{
  // NOTE: This function has no side effects, but I think "Check" means
  //       "try to access stuff and make sure it doesn't segfault", so keep it
  //       like it is even if the compiler complains about unused values?
  CBrushPlane *pbplPlane=bpo.bpo_pbplPlane;
  (void)pbplPlane; // shut up, compiler - I know this is unused, but I think it's intended like that.
  INDEX ctEdges=bpo.bpo_abpePolygonEdges.Count();
  INDEX ctVertices=bpo.bpo_apbvxTriangleVertices.Count();
  for(INDEX iEdge=0;iEdge<ctEdges;iEdge++)
  {
    CBrushPolygonEdge &edg=bpo.bpo_abpePolygonEdges[iEdge];
    CBrushEdge &be=*edg.bpe_pbedEdge;
    (void)be; // shut up, compiler
    CBrushVertex *pbvx0, *pbvx1;
    edg.GetVertices(pbvx0, pbvx1);
  }
  for(INDEX iVtx=0;iVtx<ctVertices;iVtx++)
  {
    CBrushVertex &vtx=*bpo.bpo_apbvxTriangleVertices[iVtx];
    FLOAT3D vAbs=vtx.bvx_vAbsolute;
    FLOAT3D vRel=vtx.bvx_vRelative;
    DOUBLE3D vdRel=vtx.bvx_vdPreciseRelative;
    DOUBLE3D *pvdPreciseAbsolute=vtx.bvx_pvdPreciseAbsolute;
    CBrushSector &bsc=*vtx.bvx_pbscSector;
    (void)vAbs; (void)vRel; (void)vdRel; (void)pvdPreciseAbsolute; (void)bsc; // shut up, compiler
  }
  for(INDEX ite=0;ite<bpo.bpo_aiTriangleElements.Count();ite++)
  {
    INDEX iTriangleVtx=bpo.bpo_aiTriangleElements[ite];
    CBrushSector &bsc=*bpo.bpo_pbscSector;
    (void)iTriangleVtx; (void)bsc; // ...
  }
}

void CWorld::CopyPolygonInWorld(CBrushPolygon &bpoSrc, CBrushSector &bscDst, INDEX iPol, INDEX &iEdg, INDEX &iVtx)
{
  CBrushPolygon &bpoDst=bscDst.bsc_abpoPolygons[iPol];
  CBrushPlane &plSrc=*bpoSrc.bpo_pbplPlane;
  CBrushPlane &plDst=bscDst.bsc_abplPlanes[iPol];

  plDst.bpl_plAbsolute=plSrc.bpl_plAbsolute;

  CEntity &enSrc=*bpoSrc.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  CEntity &enDst=*bscDst.bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  DOUBLEmatrix3D mdSrc=FLOATtoDOUBLE(enSrc.en_mRotation);
  DOUBLEmatrix3D mdDst=FLOATtoDOUBLE(enDst.en_mRotation);
  DOUBLE3D vdSrcOrigin=FLOATtoDOUBLE(enSrc.en_plPlacement.pl_PositionVector);
  DOUBLE3D vdDstOrigin=FLOATtoDOUBLE(enDst.en_plPlacement.pl_PositionVector);
  // calculate source absolute precise plane
  DOUBLEplane3D plSrcAbsPrecise=plSrc.bpl_pldPreciseRelative*mdSrc+vdSrcOrigin;
  plDst.bpl_plAbsolute=DOUBLEtoFLOAT(plSrcAbsPrecise);
  // calculate destination relative precise plane
  DOUBLEplane3D plDstRelPrecise=(plSrcAbsPrecise-vdDstOrigin)*!mdDst;
  plDst.bpl_pldPreciseRelative=plDstRelPrecise;
  GetMajorAxesForPlane(plDst.bpl_plAbsolute,
    plDst.bpl_iPlaneMajorAxis1,
    plDst.bpl_iPlaneMajorAxis2);
  bpoDst.bpo_pbplPlane=&plDst;

  // copy vertices
  INDEX ctEdg=bpoSrc.bpo_abpePolygonEdges.Count();
  bpoDst.bpo_abpePolygonEdges.New(ctEdg);
  for( INDEX i=0; i<ctEdg; i++)
  {
    // set vertex
    CBrushVertex *pbvx0, *pbvx1;
    CBrushPolygonEdge &edgSrc=bpoSrc.bpo_abpePolygonEdges[i];
    edgSrc.GetVertices(pbvx0, pbvx1);
    bscDst.bsc_abvxVertices[iVtx+0].SetAbsolutePosition(FLOATtoDOUBLE(pbvx0->bvx_vAbsolute));
    bscDst.bsc_abvxVertices[iVtx+1].SetAbsolutePosition(FLOATtoDOUBLE(pbvx1->bvx_vAbsolute));

    CBrushEdge &edgDst=bscDst.bsc_abedEdges[iEdg];
    edgDst.bed_pbvxVertex0=&bscDst.bsc_abvxVertices[iVtx+0];
    edgDst.bed_pbvxVertex1=&bscDst.bsc_abvxVertices[iVtx+1];

    iEdg+=1;
    iVtx+=2;

    CBrushPolygonEdge &bpe=bpoDst.bpo_abpePolygonEdges[i];
    bpe.bpe_pbedEdge=&edgDst;
    bpe.bpe_bReverse=FALSE;
  }
  bpoDst.CopyProperties(bpoSrc, TRUE);
}

void CWorld::CopyPolygonsToBrush(CBrushPolygonSelection &selPolygons, CEntity *penbr)
{
  CBrushMip *pbrmip=penbr->en_pbrBrush->GetFirstMip();
  CBrushSector *pbscDst=pbrmip->bm_abscSectors.New(1);
  pbscDst->bsc_colAmbient=C_BLACK|CT_OPAQUE;
  pbscDst->bsc_pbmBrushMip=pbrmip;

  ASSERT(pbscDst!=NULL);
  ASSERT(pbscDst->bsc_abvxVertices.Count()==0);
  ASSERT(pbscDst->bsc_abedEdges.Count()==0);
  ASSERT(pbscDst->bsc_abplPlanes.Count()==0);
  ASSERT(pbscDst->bsc_abpoPolygons.Count()==0);
  
  INDEX ctPol=selPolygons.Count();
  ASSERT(ctPol!=0);
  
  // create polygons and planes
  pbscDst->bsc_abpoPolygons.New(ctPol);
  pbscDst->bsc_abplPlanes.New(ctPol);
  // count edges
  INDEX ctEdges=0;
  {FOREACHINDYNAMICCONTAINER(selPolygons, CBrushPolygon, itbpo)
  {
    ctEdges+=itbpo->bpo_abpePolygonEdges.Count();
  }}
  ASSERT(ctEdges!=0);
  // create edges and vertices
  pbscDst->bsc_abedEdges.New(ctEdges);
  pbscDst->bsc_abvxVertices.New(ctEdges*2);

  {FOREACHINSTATICARRAY(pbscDst->bsc_abvxVertices, CBrushVertex, itbvx)
  {
    itbvx->bvx_pbscSector=pbscDst;
  }}

  {FOREACHINSTATICARRAY(pbscDst->bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    itbpo->bpo_pbscSector=pbscDst;
  }}
  
  INDEX iPol=0;
  INDEX iVtx=0;
  INDEX iEdg=0;
  {FOREACHINDYNAMICCONTAINER(selPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpoSrc=*itbpo;
    CopyPolygonInWorld( bpoSrc, *pbscDst, iPol, iEdg, iVtx);
    iPol++;
  }}

  // check polygon setup
  /*
  INDEX ctVertices=pbsc->bsc_abvxVertices.Count();
  INDEX ctEdgesControl=pbsc->bsc_abedEdges.Count();
  INDEX ctPlanes=pbsc->bsc_abplPlanes.Count();
  INDEX ctPolygons=pbsc->bsc_abpoPolygons.Count();
  
  FOREACHINLIST(CBrushMip, bm_lnInBrush, penbr->GetBrush()->br_lhBrushMips, itbm)
  {
    CBrushMip &brmip=*itbm;
    FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
    {
      CBrushSector &bsc=*itbsc;
      FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
      {
        CBrushPolygon &bpo=*itbpo;
        CheckOnePolygon(bsc, bpo);
      }
    }
  }
  */
  
  // reoptimize it
  pbscDst->bsc_pbmBrushMip->Reoptimize();
  
  // update the bounding boxes of the brush
  CBrushMip *pbrNewMip=penbr->en_pbrBrush->GetFirstMip();
  pbrNewMip->UpdateBoundingBox();
  RebuildLinks();
  // find possible shadow layers near affected area
  FindShadowLayers(pbrNewMip->bm_boxBoundingBox);
}

void CWorld::DeletePolygons(CDynamicContainer<CBrushPolygon> &dcPolygons)
{
  CDynamicContainer<CBrushSector> dcSectors;
  
  FOREACHINDYNAMICCONTAINER(dcPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpo = *itbpo;
    bpo.bpo_ulFlags|=BPOF_MARKED_FOR_USE;
    if( !dcSectors.IsMember(bpo.bpo_pbscSector))
    {
      dcSectors.Add(bpo.bpo_pbscSector);
    }
    bpo.bpo_abpePolygonEdges.Clear();
    bpo.bpo_apbvxTriangleVertices.Clear();
  }
  
  // reoptimize mips
  CDynamicContainer<CBrushMip> dcBrushMips;
  {FOREACHINDYNAMICCONTAINER(dcSectors, CBrushSector, itbsc)
  {
    CBrushSector &bsc = *itbsc;
    // reoptimize mips
    if( !dcBrushMips.IsMember(bsc.bsc_pbmBrushMip))
    {
      dcBrushMips.Add(bsc.bsc_pbmBrushMip);
    }
  }}

  {FOREACHINDYNAMICCONTAINER(dcBrushMips, CBrushMip, itbm)
  {
    itbm->Reoptimize();
    itbm->UpdateBoundingBox();
    FindShadowLayers(itbm->bm_boxBoundingBox);
  }}
  RebuildLinks();
}

void CWorld::CreatePolygon(CBrushVertexSelection &selVtx)
{
  if(selVtx.Count()!=3) return;
  
  CBrushSector *pbsc=NULL;
  {FOREACHINDYNAMICCONTAINER(selVtx, CBrushVertex, itvtx)
  {
    CBrushVertex &bvtx=*itvtx;
    if( bvtx.bvx_pbscSector==NULL || (pbsc!=NULL && pbsc!=bvtx.bvx_pbscSector)) return;
    pbsc=bvtx.bvx_pbscSector;
  }}

  selVtx.Lock();
  CBrushVertex &bv0=selVtx[0];
  CBrushVertex &bv1=selVtx[1];
  CBrushVertex &bv2=selVtx[2];
  selVtx.Unlock();

  CBrushEdge *abpeOld=&pbsc->bsc_abedEdges[0];
  INDEX ctEdgesOld=pbsc->bsc_abedEdges.Count();
  pbsc->bsc_abedEdges.Expand(ctEdgesOld+3);
  CBrushEdge *abpeNew=&pbsc->bsc_abedEdges[0];
  
  CBrushPlane *abplOld=&pbsc->bsc_abplPlanes[0];
  INDEX ctPlanesOld=pbsc->bsc_abplPlanes.Count();
  pbsc->bsc_abplPlanes.Expand(ctPlanesOld+1);
  CBrushPlane *abplNew=&pbsc->bsc_abplPlanes[0];

  // remap edge pointers in sector's polygons
  {FOREACHINSTATICARRAY(pbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpo = *itbpo;
    for(INDEX iEdg=0; iEdg<bpo.bpo_abpePolygonEdges.Count(); iEdg++)
    {
      CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[iEdg];
      bpe.bpe_pbedEdge=(CBrushEdge*)((UBYTE*)bpe.bpe_pbedEdge+((UBYTE*)abpeNew-(UBYTE*)abpeOld));
    }
    // remap plane ptr
    bpo.bpo_pbplPlane=(CBrushPlane*)((UBYTE*)bpo.bpo_pbplPlane+((UBYTE*)abplNew-(UBYTE*)abplOld));
    // clear relations
    bpo.bpo_rsOtherSideSectors.Clear();
  }}
  
  // initialize new edges
  CBrushEdge &be0=pbsc->bsc_abedEdges[ctEdgesOld+0];
  CBrushEdge &be1=pbsc->bsc_abedEdges[ctEdgesOld+1];
  CBrushEdge &be2=pbsc->bsc_abedEdges[ctEdgesOld+2];
  be0.bed_pbvxVertex0=&bv0;
  be0.bed_pbvxVertex1=&bv1;
  be1.bed_pbvxVertex0=&bv1;
  be1.bed_pbvxVertex1=&bv2;
  be2.bed_pbvxVertex0=&bv2;
  be2.bed_pbvxVertex1=&bv0;

  // initialize new plane
  CBrushPlane &bplNew=pbsc->bsc_abplPlanes[ctPlanesOld];
  bplNew.bpl_pldPreciseRelative=DOUBLEplane3D(bv0.bvx_vdPreciseRelative,
                                              bv1.bvx_vdPreciseRelative,
                                              bv2.bvx_vdPreciseRelative);
  bplNew.bpl_plRelative=DOUBLEtoFLOAT(bplNew.bpl_pldPreciseRelative);

  // calculate plane in absolute space
  CEntity &en=*pbsc->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  DOUBLE3D vOrigin=FLOATtoDOUBLE(en.en_plPlacement.pl_PositionVector);
  DOUBLEmatrix3D m=FLOATtoDOUBLE(en.en_mRotation);
  DOUBLEplane3D plAbsPrecise=bplNew.bpl_pldPreciseRelative*m+vOrigin;
  bplNew.bpl_plAbsolute=DOUBLEtoFLOAT(plAbsPrecise);
  GetMajorAxesForPlane(bplNew.bpl_plAbsolute,
    bplNew.bpl_iPlaneMajorAxis1,
    bplNew.bpl_iPlaneMajorAxis2);

  // initialize working planes
  pbsc->bsc_awplPlanes.Delete();
  pbsc->bsc_awplPlanes.New(ctPlanesOld+1);
  for( INDEX iPlane=0; iPlane<ctPlanesOld+1; iPlane++)
  {
    pbsc->bsc_abplPlanes[iPlane].bpl_pwplWorking=&pbsc->bsc_awplPlanes[iPlane];
  }
  
  // initialize new polygon
  INDEX ctOldPolygons=pbsc->bsc_abpoPolygons.Count();
  
  CStaticArray<CBrushPolygon> abpNewPolygons;
  abpNewPolygons.New(ctOldPolygons+1);
  for(INDEX iPol=0; iPol<ctOldPolygons; iPol++)
  {
    abpNewPolygons[iPol].CopyPolygon(pbsc->bsc_abpoPolygons[iPol]);
  }

  pbsc->bsc_abpoPolygons.MoveArray(abpNewPolygons);
  CBrushPolygon &bpoNew = pbsc->bsc_abpoPolygons[ctOldPolygons];
  bpoNew.bpo_pbplPlane=&bplNew;
  bpoNew.bpo_abpePolygonEdges.New(3);
  bpoNew.bpo_abpePolygonEdges[0].bpe_pbedEdge=&be0;
  bpoNew.bpo_abpePolygonEdges[0].bpe_bReverse=FALSE;
  bpoNew.bpo_abpePolygonEdges[1].bpe_pbedEdge=&be1;
  bpoNew.bpo_abpePolygonEdges[1].bpe_bReverse=FALSE;
  bpoNew.bpo_abpePolygonEdges[2].bpe_pbedEdge=&be2;
  bpoNew.bpo_abpePolygonEdges[2].bpe_bReverse=FALSE;
  bpoNew.bpo_apbvxTriangleVertices.New(3);
  bpoNew.bpo_apbvxTriangleVertices[0]=&bv0;
  bpoNew.bpo_apbvxTriangleVertices[1]=&bv1;
  bpoNew.bpo_apbvxTriangleVertices[2]=&bv2;
  bpoNew.bpo_aiTriangleElements.New(3);
  bpoNew.bpo_aiTriangleElements[0]=0;
  bpoNew.bpo_aiTriangleElements[1]=1;
  bpoNew.bpo_aiTriangleElements[2]=2;
  bpoNew.bpo_pbscSector=pbsc;

  bpoNew.bpo_colColor=C_GRAY|CT_OPAQUE;
  bpoNew.bpo_colShadow=C_WHITE|CT_OPAQUE;

  pbsc->bsc_pbmBrushMip->bm_pbrBrush->CalculateBoundingBoxes();

  // create array of working edges
  INDEX cted = pbsc->bsc_abedEdges.Count();
  pbsc->bsc_awedEdges.Clear();
  pbsc->bsc_awedEdges.New(cted);
  // for each edge
  for (INDEX ied=0; ied<cted; ied++)
  {
    CWorkingEdge &wed = pbsc->bsc_awedEdges[ied];
    CBrushEdge   &bed = pbsc->bsc_abedEdges[ied];
    // setup its working edge
    bed.bed_pwedWorking = &wed;
    wed.wed_iwvx0 = pbsc->bsc_abvxVertices.Index(bed.bed_pbvxVertex0);
    wed.wed_iwvx1 = pbsc->bsc_abvxVertices.Index(bed.bed_pbvxVertex1);
  }
  pbsc->UpdateSector();
}

void CWorld::FlipPolygon(CBrushPolygon &bpo)
{
  CBrushSector &bsc=*bpo.bpo_pbscSector;

  DOUBLEplane3D plOldPreciseRel=bpo.bpo_pbplPlane->bpl_pldPreciseRelative;
  // create new array of planes
  CBrushPlane *abplOld=&bsc.bsc_abplPlanes[0];
  INDEX ctPlanesOld=bsc.bsc_abplPlanes.Count();
  bsc.bsc_abplPlanes.Expand(ctPlanesOld+1);
  CBrushPlane *abplNew=&bsc.bsc_abplPlanes[0];

  // remap plane ptrs
  {FOREACHINSTATICARRAY(bsc.bsc_abpoPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpo = *itbpo;
    bpo.bpo_pbplPlane=(CBrushPlane*)((UBYTE*)bpo.bpo_pbplPlane+((UBYTE*)abplNew-(UBYTE*)abplOld));
  }}
  
  // invert order of triangle vertices
  for( INDEX iVtx=0; iVtx<bpo.bpo_aiTriangleElements.Count(); iVtx+=3)
  {
    INDEX iOldFirst=bpo.bpo_aiTriangleElements[iVtx+0];
    bpo.bpo_aiTriangleElements[iVtx+0]=bpo.bpo_aiTriangleElements[iVtx+2];
    bpo.bpo_aiTriangleElements[iVtx+2]=iOldFirst;
  }
  
  // initialize new plane
  CBrushPlane &bplNew=bsc.bsc_abplPlanes[ctPlanesOld];
  // set it to flipped old plane
  bplNew.bpl_pldPreciseRelative=-plOldPreciseRel;
  bplNew.bpl_plRelative=DOUBLEtoFLOAT(bplNew.bpl_pldPreciseRelative);
  // calculate plane in absolute space
  CEntity &en=*bsc.bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  DOUBLE3D vOrigin=FLOATtoDOUBLE(en.en_plPlacement.pl_PositionVector);
  DOUBLEmatrix3D m=FLOATtoDOUBLE(en.en_mRotation);
  DOUBLEplane3D plAbsPrecise=bplNew.bpl_pldPreciseRelative*m+vOrigin;
  bplNew.bpl_plAbsolute=DOUBLEtoFLOAT(plAbsPrecise);
  GetMajorAxesForPlane(bplNew.bpl_plAbsolute,
    bplNew.bpl_iPlaneMajorAxis1,
    bplNew.bpl_iPlaneMajorAxis2);

  // set new plane ptr
  bpo.bpo_pbplPlane=&bplNew;

  // initialize working planes
  bsc.bsc_awplPlanes.Delete();
  bsc.bsc_awplPlanes.New(ctPlanesOld+1);
  for( INDEX iPlane=0; iPlane<ctPlanesOld+1; iPlane++)
  {
    bsc.bsc_abplPlanes[iPlane].bpl_pwplWorking=&bsc.bsc_awplPlanes[iPlane];
  }
  
  // flip polygon edges
  INDEX ctEdg=bpo.bpo_abpePolygonEdges.Count();
  for( INDEX i=0; i<ctEdg; i++)
  {
    CBrushPolygonEdge &edg=bpo.bpo_abpePolygonEdges[i];
    edg.bpe_bReverse=!edg.bpe_bReverse;
  }
}
