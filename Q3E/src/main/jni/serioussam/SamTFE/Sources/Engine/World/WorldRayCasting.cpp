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

#include <Engine/StdH.h>

#include <Engine/Base/Console.h>
#include <Engine/World/World.h>
#include <Engine/Rendering/Render.h>
#include <Engine/World/WorldRayCasting.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Brushes/Brush.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Models/ModelObject.h>
#include <Engine/Math/Clipping.inl>
#include <Engine/Entities/EntityCollision.h>
#include <Engine/Math/Geometry.inl>
#include <Engine/Network/Network.h>
#include <Engine/Ska/Render.h>
#include <Engine/Terrain/Terrain.h>
#include <Engine/Terrain/TerrainRayCasting.h>

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Templates/StaticStackArray.cpp>

#define EPSILON (0.1f)

class CActiveSector {
public:
  CBrushSector *as_pbsc;
  void Clear(void) {};
};

static CStaticStackArray<CActiveSector> _aas;
CListHead _lhTestedTerrains; // list of tested terrains

// calculate origin position from ray placement
static inline FLOAT3D CalculateRayOrigin(const CPlacement3D &plRay)
{
  // origin is the position from the placement
  return plRay.pl_PositionVector;
}
// calculate target position from ray placement
static inline FLOAT3D CalculateRayTarget(const CPlacement3D &plRay)
{
  // calculate direction of the ray
  FLOAT3D vDirection;
  AnglesToDirectionVector(plRay.pl_OrientationAngle, vDirection);
  // make target be from the origin in that direction
  return plRay.pl_PositionVector+vDirection;
}

/*
 * Internal construction helper.
 */
void CCastRay::Init(CEntity *penOrigin, const FLOAT3D &vOrigin, const FLOAT3D &vTarget)
{
  ClearSectorList();
  cr_penOrigin = penOrigin;
  cr_vOrigin = vOrigin;
  cr_vTarget = vTarget;
  cr_bAllowOverHit = FALSE;
  cr_pbpoIgnore = NULL;
  cr_penIgnore = NULL;

  cr_bHitPortals = FALSE;
  cr_bHitTranslucentPortals = TRUE;
  cr_ttHitModels = TT_SIMPLE;
  cr_bHitFields = FALSE;
  cr_bPhysical = FALSE;
  cr_bHitBrushes = TRUE;
  cr_bHitTerrainInvisibleTris = FALSE;
  cr_fTestR = 0;

	cr_bFindBone = TRUE;
	cr_iBoneHit	 = -1;

  cl_plRay.pl_PositionVector = vOrigin;
  DirectionVectorToAngles((vTarget-vOrigin).Normalize(), cl_plRay.pl_OrientationAngle);
}

/*
 * Constructor.
 */
CCastRay::CCastRay(CEntity *penOrigin, const CPlacement3D &plOrigin)
{
  Init(penOrigin, CalculateRayOrigin(plOrigin), CalculateRayTarget(plOrigin));
  // mark last found hit point in infinity
  cr_fHitDistance = UpperLimit(0.0f);
}
CCastRay::CCastRay(CEntity *penOrigin, const CPlacement3D &plOrigin, FLOAT fMaxTestDistance)
{
  Init(penOrigin, CalculateRayOrigin(plOrigin), CalculateRayTarget(plOrigin));
  // mark last found hit point just as far away as we wan't to test
  cr_fHitDistance = fMaxTestDistance;
}
CCastRay::CCastRay(CEntity *penOrigin, const FLOAT3D &vOrigin, const FLOAT3D &vTarget)
{
  Init(penOrigin, vOrigin, vTarget);
  // mark last found hit point just a bit behind the target
  cr_fHitDistance = (cr_vTarget-cr_vOrigin).Length() + EPSILON;
}

CCastRay::~CCastRay(void)
{
  ClearSectorList();
}

void CCastRay::ClearSectorList(void)
{
  // for each active sector
  for(INDEX ias=0; ias<_aas.Count(); ias++) {
    // mark it as inactive
    _aas[ias].as_pbsc->bsc_ulFlags&=~BSCF_RAYTESTED;
  }
  _aas.PopAll();
}

/*
 * Test if a ray hits sphere.
 */
inline static BOOL RayHitsSphere(
  const FLOAT3D &vStart,
  const FLOAT3D &vEnd,
  const FLOAT3D &vSphereCenter,
  const FLOAT fSphereRadius,
  FLOAT &fDistance)
{
  const FLOAT3D vSphereCenterToStart = vStart - vSphereCenter;
  const FLOAT3D vStartToEnd          = vEnd - vStart;
  // calculate discriminant for intersection parameters
  const FLOAT fP = ((vStartToEnd%vSphereCenterToStart)/(vStartToEnd%vStartToEnd));
  const FLOAT fQ = (((vSphereCenterToStart%vSphereCenterToStart)
    - (fSphereRadius*fSphereRadius))/(vStartToEnd%vStartToEnd));
  const FLOAT fD = fP*fP-fQ;
  // if it is less than zero
  if (fD<0) {
    // no collision will occur
    return FALSE;
  }
  // calculate intersection parameters
  const FLOAT fSqrtD = sqrt(fD);
  const FLOAT fLambda1 = -fP+fSqrtD;
  const FLOAT fLambda2 = -fP-fSqrtD;
  // use lower one
  const FLOAT fMinLambda = Min(fLambda1, fLambda2);
  // calculate distance from parameter
  fDistance = fMinLambda*vStartToEnd.Length();
  return TRUE;
}

void CCastRay::TestModelSimple(CEntity *penModel, CModelObject &mo)
{
  // get model's bounding box for current frame
  FLOATaabbox3D boxModel;
  mo.GetCurrentFrameBBox(boxModel);
  boxModel.StretchByVector(mo.mo_Stretch);
  // get center and radius of the bounding sphere in absolute space
  FLOAT fSphereRadius = boxModel.Size().Length()/2.0f;
  FLOAT3D vSphereCenter = boxModel.Center();
  vSphereCenter*=penModel->en_mRotation;
  vSphereCenter+=penModel->en_plPlacement.pl_PositionVector;

  // if the ray doesn't hit the sphere
  FLOAT fSphereHitDistance;
  if (!RayHitsSphere(cr_vOrigin, cr_vTarget,
    vSphereCenter, fSphereRadius+cr_fTestR, fSphereHitDistance) ) {
    // ignore
    return;
  }

  // if the ray hits the sphere closer than closest found hit point yet
  if (fSphereHitDistance<cr_fHitDistance && fSphereHitDistance>0.0f) {
    // set the current entity as new hit target
    cr_fHitDistance=fSphereHitDistance;
    cr_penHit = penModel;
    cr_pbscBrushSector = NULL;
    cr_pbpoBrushPolygon = NULL;
  }
}

void CCastRay::TestModelCollisionBox(CEntity *penModel)
{
  // if no collision box
  CCollisionInfo *pci = penModel->en_pciCollisionInfo;
  if (pci==NULL) {
    // don't test
    return;
  }

  // get model's collision bounding box
  FLOATaabbox3D &boxModel = pci->ci_boxCurrent;
  FLOAT fSphereRadius = boxModel.Size().Length()/2.0f;
  FLOAT3D vSphereCenter = boxModel.Center();

  // if the ray doesn't hit the sphere
  FLOAT fSphereHitDistance;
  if (!RayHitsSphere(cr_vOrigin, cr_vTarget,
    vSphereCenter, fSphereRadius+cr_fTestR, fSphereHitDistance) ) {
    // ignore
    return;
  }

  // get entity collision spheres
  CStaticArray<CMovingSphere> &ams = pci->ci_absSpheres;
  // get entity position
  const FLOAT3D &vPosition = penModel->en_plPlacement.pl_PositionVector;
  const FLOATmatrix3D &mRotation = penModel->en_mRotation;

  // for each sphere
  FOREACHINSTATICARRAY(ams, CMovingSphere, itms) {
    // project its center to absolute space
    FLOAT3D vCenter = itms->ms_vCenter*mRotation + vPosition;
    // if the ray hits the sphere closer than closest found hit point yet
    FLOAT fOneSphereHitDistance;
    if (RayHitsSphere(cr_vOrigin, cr_vTarget,
      vCenter, itms->ms_fR+cr_fTestR, fOneSphereHitDistance) &&
      fOneSphereHitDistance<cr_fHitDistance && fOneSphereHitDistance>-cr_fTestR) {
      // set the current entity as new hit target
      cr_fHitDistance=fOneSphereHitDistance;
      cr_penHit = penModel;
      cr_pbscBrushSector = NULL;
      cr_pbpoBrushPolygon = NULL;
    }
  }
}

void CCastRay::TestModelFull(CEntity *penModel, CModelObject &mo)
{
  // NOTE: this contains an ugly hack to simulate good trivial rejection
  // for models that have attachments that extend far off the base entity.
  // it is used only in wed, so it should not be a big problem.

  // get model's bounding box for all frames and expand it a lot
  FLOATaabbox3D boxModel;
  mo.GetAllFramesBBox(boxModel);
  boxModel.StretchByVector(mo.mo_Stretch*5.0f);
  // get center and radius of the bounding sphere in absolute space
  FLOAT fSphereRadius = boxModel.Size().Length()/2.0f;
  FLOAT3D vSphereCenter = boxModel.Center();
  vSphereCenter*=penModel->en_mRotation;
  vSphereCenter+=penModel->en_plPlacement.pl_PositionVector;

  // if the ray doesn't hit the sphere
  FLOAT fSphereHitDistance;
  if (!RayHitsSphere(cr_vOrigin, cr_vTarget,
    vSphereCenter, fSphereRadius+cr_fTestR, fSphereHitDistance) ) {
    // ignore
    return;
  }

  FLOAT fHitDistance;
  // if the ray hits the model closer than closest found hit point yet
  if (mo.PolygonHit(cl_plRay, penModel->en_plPlacement, 0/*iCurrentMip*/,
    fHitDistance)!=NULL
    && fHitDistance<cr_fHitDistance) {
    // set the current entity as new hit target
    cr_fHitDistance=fHitDistance;
    cr_penHit = penModel;
    cr_pbscBrushSector = NULL;
    cr_pbpoBrushPolygon = NULL;
  }
}

/*
 * Test against a model entity.
 */
void CCastRay::TestModel(CEntity *penModel)
{
  // if origin is predictor, and the model is predicted
  if (cr_penOrigin!=NULL && cr_penOrigin->IsPredictor() && penModel->IsPredicted()) {
    // don't test it
    return;
  }

  // if hidden model
  if( penModel->en_ulFlags&ENF_HIDDEN)
  {
    // don't test
    return;
  }

  // get its model
  CModelObject *pmoModel;
  if (penModel->en_RenderType!=CEntity::RT_BRUSH
   && penModel->en_RenderType != CEntity::RT_FIELDBRUSH) {
    pmoModel=penModel->en_pmoModelObject;
  } else {
    // empty brushes are also tested as models
    pmoModel=_wrpWorldRenderPrefs.GetEmptyBrushModel();
  }
  // if there is no valid model
  if (pmoModel==NULL) {
    // don't test it
    return;
  }
  CModelObject &mo = *pmoModel;

  // if simple testing, or no testing (used when testing empty brushes)
  if (cr_ttHitModels==TT_SIMPLE || cr_ttHitModels==TT_NONE) {
    TestModelSimple(penModel, mo);
  // if collision box testing
  } else if (cr_ttHitModels==TT_COLLISIONBOX) {
    TestModelCollisionBox(penModel);
  // if full testing
  } else if (cr_ttHitModels==TT_FULL || cr_ttHitModels==TT_FULLSEETHROUGH) {
    TestModelFull(penModel, mo);
  // must be no other testing
  } else {
    ASSERT(FALSE);
  }
}

/*
 * Test against a ska model
 */ 
void CCastRay::TestSkaModel(CEntity *penModel)
{
  // if origin is predictor, and the model is predicted
  if (cr_penOrigin!=NULL && cr_penOrigin->IsPredictor() && penModel->IsPredicted()) {
    // don't test it
    return;
  }

  // if hidden model
  if( penModel->en_ulFlags&ENF_HIDDEN)
  {
    // don't test
    return;
  }

  CModelInstance &mi = *penModel->GetModelInstance();
  // if simple testing, or no testing (used when testing empty brushes)
  if (cr_ttHitModels==TT_SIMPLE || cr_ttHitModels==TT_NONE) {
    TestSkaModelSimple(penModel, mi);
  // if collision box testing
  } else if (cr_ttHitModels==TT_COLLISIONBOX) {
    TestModelCollisionBox(penModel);
  // if full testing
  } else if (cr_ttHitModels==TT_FULL || cr_ttHitModels==TT_FULLSEETHROUGH) {
     TestSkaModelFull(penModel, mi);
  // must be no other testing
  } else {
    ASSERT(FALSE);
  }
}

void CCastRay::TestSkaModelSimple(CEntity *penModel, CModelInstance &mi)
{
  FLOATaabbox3D boxModel;
  mi.GetCurrentColisionBox(boxModel);
  boxModel.StretchByVector(mi.mi_vStretch);
  // get center and radius of the bounding sphere in absolute space
  FLOAT fSphereRadius = boxModel.Size().Length()/2.0f;
  FLOAT3D vSphereCenter = boxModel.Center();
  vSphereCenter*=penModel->en_mRotation;
  vSphereCenter+=penModel->en_plPlacement.pl_PositionVector;

  // if the ray doesn't hit the sphere
  FLOAT fSphereHitDistance;
  if (!RayHitsSphere(cr_vOrigin, cr_vTarget,
    vSphereCenter, fSphereRadius+cr_fTestR, fSphereHitDistance) ) {
    // ignore
    return;
  }

  // if the ray hits the sphere closer than closest found hit point yet
  if (fSphereHitDistance<cr_fHitDistance && fSphereHitDistance>0.0f) {
    // set the current entity as new hit target
    cr_fHitDistance=fSphereHitDistance;
    cr_penHit = penModel;
    cr_pbscBrushSector = NULL;
    cr_pbpoBrushPolygon = NULL;
  }
}

void CCastRay::TestSkaModelFull(CEntity *penModel, CModelInstance &mi)
{
  FLOATaabbox3D boxModel;
  mi.GetAllFramesBBox(boxModel);
  boxModel.StretchByVector(mi.mi_vStretch);
  // get center and radius of the bounding sphere in absolute space
  FLOAT fSphereRadius = boxModel.Size().Length()/2.0f;
  FLOAT3D vSphereCenter = boxModel.Center();
  vSphereCenter*=penModel->en_mRotation;
  vSphereCenter+=penModel->en_plPlacement.pl_PositionVector;

  // if the ray doesn't hit the sphere
  FLOAT fSphereHitDistance;
  if (!RayHitsSphere(cr_vOrigin, cr_vTarget,
    vSphereCenter, fSphereRadius+cr_fTestR, fSphereHitDistance) ) {
    // ignore
    return;
  }

  // if the ray hits the sphere closer than closest found hit point yet
  if (fSphereHitDistance<cr_fHitDistance && fSphereHitDistance>0.0f) {
		FLOAT fTriangleHitDistance;
    // set the current entity as new hit target
//    cr_fHitDistance=fSphereHitDistance;
//    cr_penHit = penModel;
//    cr_pbscBrushSector = NULL;
//    cr_pbpoBrushPolygon = NULL;

		INDEX iBoneID = -1;
		if (cr_bFindBone) {
			fTriangleHitDistance = RM_TestRayCastHit(mi,penModel->en_mRotation,penModel->en_plPlacement.pl_PositionVector,cr_vOrigin,cr_vTarget,cr_fHitDistance,&iBoneID);
		}	else {
			fTriangleHitDistance = RM_TestRayCastHit(mi,penModel->en_mRotation,penModel->en_plPlacement.pl_PositionVector,cr_vOrigin,cr_vTarget,cr_fHitDistance,NULL);
		}

		if (fTriangleHitDistance<cr_fHitDistance && fTriangleHitDistance>0.0f) {
			// set the current entity as new hit target
			cr_fHitDistance=fTriangleHitDistance;
			cr_penHit = penModel;
			cr_pbscBrushSector = NULL;
			cr_pbpoBrushPolygon = NULL;
			
			if (cr_bFindBone) {
				cr_iBoneHit = iBoneID;
			}
		}

  }
	return;
}

void CCastRay::TestTerrain(CEntity *penTerrain)
{
  // if hidden model
  if( penTerrain->en_ulFlags&ENF_HIDDEN) {
    // don't test
    return;
  }

  CTerrain *ptrTerrain = penTerrain->GetTerrain();
  FLOAT fHitDistance = TestRayCastHit(ptrTerrain,penTerrain->en_mRotation, penTerrain->en_plPlacement.pl_PositionVector,
                                      cr_vOrigin,cr_vTarget,cr_fHitDistance,cr_bHitTerrainInvisibleTris);

	if (fHitDistance<cr_fHitDistance && fHitDistance>0.0f) {
		// set the current entity as new hit target
		cr_fHitDistance=fHitDistance;
		cr_penHit = penTerrain;
		cr_pbscBrushSector = NULL;
		cr_pbpoBrushPolygon = NULL;
	}
}

/*
 * Test against a brush sector.
 */
void CCastRay::TestBrushSector(CBrushSector *pbscSector)
{
  // if entity is hidden
  if(pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_ulFlags&ENF_HIDDEN)
  {
    // don't cast ray
    return;
  }

  const CEntity *l_cr_penOrigin = cr_penOrigin;

  // for each polygon in the sector
  // FOREACHINSTATICARRAY(pbscSector->bsc_abpoPolygons, CBrushPolygon, itpoPolygon) {
  CBrushPolygon *itpoPolygon = pbscSector->bsc_abpoPolygons.sa_Array;
  int i;
  for (i = 0; i < pbscSector->bsc_abpoPolygons.sa_Count; i++, itpoPolygon++) {
    CBrushPolygon &bpoPolygon = *itpoPolygon;

    if (&bpoPolygon==cr_pbpoIgnore) {
      continue;
    }

    ULONG ulFlags = bpoPolygon.bpo_ulFlags;
    // if not testing recursively
    if (l_cr_penOrigin==NULL) {
      // if the polygon is portal
      if (ulFlags&BPOF_PORTAL) {
        // if it is translucent or selected
        if (ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT|BPOF_SELECTED)) {
          // if translucent portals should be passed through
          if (!cr_bHitTranslucentPortals) {
            // skip this polygon
            continue;
          }
        // if it is not translucent
        } else {
           // if portals should be passed through
          if (!cr_bHitPortals) {
            // skip this polygon
            continue;
          }
        }
      }
      // if polygon is detail, and detail polygons are off
      extern INDEX wld_bRenderDetailPolygons;
      if ((ulFlags&BPOF_DETAILPOLYGON) && !wld_bRenderDetailPolygons) {
        // skip this polygon
        continue;
      }
    }
#if defined __ARM_NEON__ && !defined PLATFORM_MACOSX && !defined(ANDROID) //karin: unuse asm
    // get distances of ray points from the polygon plane
    register FLOAT fDistance0 __asm__("s0") = bpoPolygon.bpo_pbplPlane->bpl_plAbsolute.PointDistance(cr_vOrigin);
    register FLOAT fDistance1 __asm__("s2") = bpoPolygon.bpo_pbplPlane->bpl_plAbsolute.PointDistance(cr_vTarget);
    FLOAT fFraction;
    int gege;
    __asm__ __volatile__ (
      "vcge.f32 d2, d0, #0\n"
      "vcge.f32 d3, d0, d1\n"
      "vand     d2, d3\n"
      "ldr      %[gege], %[nnpptr]\n" // take the cache miss, slight chance of a crash
      "pld      %[nplane]\n"
      "vmov     %[gege], d2[0]\n"
      : [gege] "=&r"(gege)
      : [nnpptr] "m"(itpoPolygon[2].bpo_pbplPlane),
        [nplane] "m"(*itpoPolygon[1].bpo_pbplPlane),
	"t"(fDistance0), "t"(fDistance1)
      : "d2", "d3"
    );

    if (gege) {
#else
    FLOAT fDistance0 = bpoPolygon.bpo_pbplPlane->bpl_plAbsolute.PointDistance(cr_vOrigin);
    FLOAT fDistance1 = bpoPolygon.bpo_pbplPlane->bpl_plAbsolute.PointDistance(cr_vTarget);
    // if the ray hits the polygon plane
    if (fDistance0>=0 && fDistance0>=fDistance1) {
      // calculate fraction of line before intersection
#endif
      FLOAT fFraction = fDistance0/((fDistance0-fDistance1) + 0.0000001f/*correction*/);
      // calculate intersection coordinate
      FLOAT3D vHitPoint = cr_vOrigin+(cr_vTarget-cr_vOrigin)*fFraction;
      // calculate intersection distance
      FLOAT fHitDistance = (vHitPoint-cr_vOrigin).Length();
      // if the hit point can not be new closest candidate
      if (fHitDistance>cr_fHitDistance) {
        // skip this polygon
        continue;
      }

      // find major axes of the polygon plane
      INDEX iMajorAxis1, iMajorAxis2;
      GetMajorAxesForPlane(itpoPolygon->bpo_pbplPlane->bpl_plAbsolute, iMajorAxis1, iMajorAxis2);

      // create an intersector
      CIntersector isIntersector(vHitPoint(iMajorAxis1), vHitPoint(iMajorAxis2));
      // for all edges in the polygon
      FOREACHINSTATICARRAY(bpoPolygon.bpo_abpePolygonEdges, CBrushPolygonEdge,
        itbpePolygonEdge) {
        // get edge vertices (edge direction is irrelevant here!)
        const FLOAT3D &vVertex0 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
        const FLOAT3D &vVertex1 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
        // pass the edge to the intersector
        isIntersector.AddEdge(
          vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
          vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
      }
      // if the polygon is intersected by the ray
      if (isIntersector.IsIntersecting()) {
        // if it is portal and testing recusively
        if ((ulFlags&cr_ulPassablePolygons) && (cr_penOrigin!=NULL)) {
          // for each sector on the other side
          {FOREACHDSTOFSRC(bpoPolygon.bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbsc)
            // add the sector
            AddSector(pbsc);
          ENDFOR}

          if( cr_bHitPortals && ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT) && !cr_bPhysical)
          {
            // remember hit coordinates
            cr_fHitDistance=fHitDistance;
            cr_penHit = pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
            cr_pbscBrushSector = pbscSector;
            cr_pbpoBrushPolygon = &bpoPolygon;
          }
        // if the ray just plainly hit it
        } else {
          // remember hit coordinates
          cr_fHitDistance=fHitDistance;
          cr_penHit = pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
          cr_pbscBrushSector = pbscSector;
          cr_pbpoBrushPolygon = &bpoPolygon;
        }
      }
    }
  }
}

/* Add a sector if needed. */
inline void CCastRay::AddSector(CBrushSector *pbsc)
{
  // if not already active and in first mip of its brush
  if ( pbsc->bsc_pbmBrushMip->IsFirstMip()
    &&!(pbsc->bsc_ulFlags&BSCF_RAYTESTED)) {
    // add it to active sectors
    _aas.Push().as_pbsc = pbsc;
    pbsc->bsc_ulFlags|=BSCF_RAYTESTED;
  }
}
/* Add all sectors of a brush. */
void CCastRay::AddAllSectorsOfBrush(CBrush3D *pbr)
{
  // get relevant mip as if in manual mip brushing mode
  CBrushMip *pbmMip = pbr->GetBrushMipByDistance(
    _wrpWorldRenderPrefs.GetManualMipBrushingFactor());

  // if it has no brush mip for that mip factor
  if (pbmMip==NULL) {
    // skip it
    return;
  }
  // for each sector in the brush mip
  FOREACHINDYNAMICARRAY(pbmMip->bm_abscSectors, CBrushSector, itbsc) {
    // add the sector
    AddSector(itbsc);
  }
}

/* Add all sectors around given entity. */
void CCastRay::AddSectorsAroundEntity(CEntity *pen)
{
  // for each zoning sector that this entity is in
  {FOREACHSRCOFDST(pen->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
    // if part of zoning brush
    if (pbsc->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->GetRenderType()!=CEntity::RT_BRUSH) {
      // skip it
      continue;
    }
    // add the sector
    AddSector(pbsc);
  ENDFOR}
}

/* Test entire world against ray. */
void CCastRay::TestWholeWorld(CWorld *pwoWorld)
{
  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(pwoWorld->wo_cenEntities, CEntity, itenInWorld) {
    // if it is the origin of the ray
    if (itenInWorld==cr_penOrigin || itenInWorld==cr_penIgnore) {
      // skip it
      continue;
    }

    // if it is a brush and testing against brushes is disabled
    if( (itenInWorld->en_RenderType == CEntity::RT_BRUSH ||
         itenInWorld->en_RenderType == CEntity::RT_FIELDBRUSH) && 
         !cr_bHitBrushes) {
      // skip it
      continue;
    }

    // if it is a model and testing against models is enabled
    if(((itenInWorld->en_RenderType == CEntity::RT_MODEL
      ||(itenInWorld->en_RenderType == CEntity::RT_EDITORMODEL
         && _wrpWorldRenderPrefs.IsEditorModelsOn()))
      && cr_ttHitModels != TT_NONE)
    //  and if cast type is TT_FULL_SEETROUGH then model is not
    //  ENF_SEETROUGH
      && !((cr_ttHitModels == TT_FULLSEETHROUGH || cr_ttHitModels == TT_COLLISIONBOX) &&
           (itenInWorld->en_ulFlags&ENF_SEETHROUGH))) {
      // test it against the model entity
      TestModel(itenInWorld);
    // if it is a ska model
    } else if(((itenInWorld->en_RenderType == CEntity::RT_SKAMODEL
      ||(itenInWorld->en_RenderType == CEntity::RT_SKAEDITORMODEL
         && _wrpWorldRenderPrefs.IsEditorModelsOn()))
      && cr_ttHitModels != TT_NONE)
    //  and if cast type is TT_FULL_SEETROUGH then model is not
    //  ENF_SEETROUGH
      && !((cr_ttHitModels == TT_FULLSEETHROUGH || cr_ttHitModels == TT_COLLISIONBOX) &&
           (itenInWorld->en_ulFlags&ENF_SEETHROUGH))) {
      TestSkaModel(itenInWorld);
    } else if (itenInWorld->en_RenderType == CEntity::RT_TERRAIN) {
      TestTerrain(itenInWorld);
    // if it is a brush
    } else if (itenInWorld->en_RenderType == CEntity::RT_BRUSH ||
      (itenInWorld->en_RenderType == CEntity::RT_FIELDBRUSH
      &&_wrpWorldRenderPrefs.IsFieldBrushesOn() && cr_bHitFields)) {
      // get its brush
      CBrush3D &brBrush = *itenInWorld->en_pbrBrush;

      // get relevant mip as if in manual mip brushing mode
      CBrushMip *pbmMip = brBrush.GetBrushMipByDistance(
        _wrpWorldRenderPrefs.GetManualMipBrushingFactor());

      // if it has no brush mip for that mip factor
      if (pbmMip==NULL) {
        // skip it
        continue;
      }

      // if it has zero sectors
      if (pbmMip->bm_abscSectors.Count()==0){
        // test it against the model entity
        TestModel(itenInWorld);

      // if it has some sectors
      } else {
        // for each sector in the brush mip
        FOREACHINDYNAMICARRAY(pbmMip->bm_abscSectors, CBrushSector, itbsc) {
          // if the sector is not hidden
          if (!(itbsc->bsc_ulFlags & BSCF_HIDDEN)) {
            // test the ray against the sector
            TestBrushSector(itbsc);
          }
        }
      }
    }
  }}
}

/* Test active sectors recusively. */
void CCastRay::TestThroughSectors(void)
{
  // for each active sector (sectors are added during iteration!)
  for(INDEX ias=0; ias<_aas.Count(); ias++) {
    CBrushSector *pbsc = _aas[ias].as_pbsc;
    // test the ray against the sector
    TestBrushSector(pbsc);
    // for each entity in the sector
    {FOREACHDSTOFSRC(pbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
      // if it is the origin of the ray
      if (pen==cr_penOrigin || pen==cr_penIgnore) {
        // skip it
        continue;
      }
      // if it is a model and testing against models is enabled
      if(((pen->en_RenderType == CEntity::RT_MODEL
        ||(pen->en_RenderType == CEntity::RT_EDITORMODEL
           && _wrpWorldRenderPrefs.IsEditorModelsOn()))
        && cr_ttHitModels != TT_NONE)
      //  and if cast type is TT_FULL_SEETROUGH then model is not
      //  ENF_SEETROUGH
        && !((cr_ttHitModels == TT_FULLSEETHROUGH || cr_ttHitModels == TT_COLLISIONBOX) &&
             (pen->en_ulFlags&ENF_SEETHROUGH))) {
        // test it against the model entity
        TestModel(pen);
      // if is is a ska model
      } else if(((pen->en_RenderType == CEntity::RT_SKAMODEL
        ||(pen->en_RenderType == CEntity::RT_SKAEDITORMODEL
           && _wrpWorldRenderPrefs.IsEditorModelsOn()))
        && cr_ttHitModels != TT_NONE)
      //  and if cast type is TT_FULL_SEETROUGH then model is not
      //  ENF_SEETROUGH
        && !((cr_ttHitModels == TT_FULLSEETHROUGH || cr_ttHitModels == TT_COLLISIONBOX) &&
             (pen->en_ulFlags&ENF_SEETHROUGH))) {
        // test it against the ska model entity
        TestSkaModel(pen);
      // if it is a terrain
      } else if( pen->en_RenderType == CEntity::RT_TERRAIN) {
        CTerrain *ptrTerrain = pen->GetTerrain();
        ASSERT(ptrTerrain!=NULL);
        // if terrain hasn't allready been tested
        if(!ptrTerrain->tr_lnInActiveTerrains.IsLinked()) {
          // test it now and add it to list of tested terrains
          TestTerrain(pen);
          _lhTestedTerrains.AddTail(ptrTerrain->tr_lnInActiveTerrains);
        }
      // if it is a non-hidden brush
      } else if ( (pen->en_RenderType == CEntity::RT_BRUSH) &&
                  !(pen->en_ulFlags&ENF_HIDDEN) ) {
        // get its brush
        CBrush3D &brBrush = *pen->en_pbrBrush;
        // add all sectors in the brush
        AddAllSectorsOfBrush(&brBrush);
      }
    ENDFOR}
  }

  // for all tested terrains
  {FORDELETELIST(CTerrain, tr_lnInActiveTerrains, _lhTestedTerrains, ittr) {
    // remove it from list
    ittr->tr_lnInActiveTerrains.Remove();
  }}
  ASSERT(_lhTestedTerrains.IsEmpty());
}

/*
 * Do the ray casting.
 */
void CCastRay::Cast(CWorld *pwoWorld)
{
  // setup stat timers
  const BOOL bMainLoopTimer = _sfStats.CheckTimer(CStatForm::STI_MAINLOOP);
  if( bMainLoopTimer) _sfStats.StopTimer(CStatForm::STI_MAINLOOP);
  _sfStats.StartTimer(CStatForm::STI_RAYCAST);

  // initially no polygon is found
  cr_pbpoBrushPolygon= NULL;
  cr_pbscBrushSector = NULL;
  cr_penHit = NULL;
  if (cr_bPhysical) {
    cr_ulPassablePolygons = BPOF_PASSABLE|BPOF_SHOOTTHRU;
  } else {
    cr_ulPassablePolygons = BPOF_PORTAL|BPOF_OCCLUDER;
  }

  // if origin entity is given
  if (cr_penOrigin!=NULL) {
    // if not continuing
    if (_aas.Count()==0) {
      // add all sectors around it
      AddSectorsAroundEntity(cr_penOrigin);
    }
    // test all sectors recursively
    TestThroughSectors();
  // if there is no origin entity
  } else {
    // test entire world against ray
    TestWholeWorld(pwoWorld);
  }

	// calculate the hit point from the hit distance
  cr_vHit = cr_vOrigin + (cr_vTarget-cr_vOrigin).Normalize()*cr_fHitDistance;

  // done with timing
  _sfStats.StopTimer(CStatForm::STI_RAYCAST);
  if( bMainLoopTimer) _sfStats.StartTimer(CStatForm::STI_MAINLOOP);
}


/*
 * Continue cast.
 */
void CCastRay::ContinueCast(CWorld *pwoWorld)
{
  cr_pbpoIgnore = cr_pbpoBrushPolygon;
  if (cr_penHit->GetRenderType()==CEntity::RT_MODEL) {
    cr_penIgnore = cr_penHit;
  }

  cr_vOrigin = cr_vHit;
  cl_plRay.pl_PositionVector = cr_vOrigin;
  cr_fHitDistance = (cr_vTarget-cr_vOrigin).Length() + EPSILON;
  Cast(pwoWorld);
}

/////////////////////////////////////////////////////////////////////
/*
 * Cast a ray and see what it hits.
 */
void CWorld::CastRay(CCastRay &crRay)
{
  crRay.Cast(this);
}
/*
 * Continue to cast already cast ray
 */
void CWorld::ContinueCast(CCastRay &crRay)
{
  crRay.ContinueCast(this);
}
