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

#include <Engine/Base/Stream.h>
#include <Engine/Base/ReplaceFile.h>
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Brushes/Brush.h>
#include <Engine/Math/Object3D.h>
#include <Engine/Math/Float.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>

static const INDEX _iSupportedVersion = 14;

static INDEX _ctPolygonsLoaded;

#define BPOV_OLD                        0
#define BPOV_WITHHYPERTEXTURES          1
#define BPOV_MULTITEXTURING             2
#define BPOV_FAKEPORTALFLAG             3
#define BPOV_TRIANGLES                  4
#define BPOV_CURRENT                    BPOV_TRIANGLES

#define BSCV_OLD                        0
#define BSCV_WITHNAME                   1
#define BSCV_WITHFLAGS2                 2
#define BSCV_WITHVISFLAGS               3
#define BSCV_CURRENT                    BSCV_WITHVISFLAGS

/*
 * Write to stream.
 */
void CBrush3D::Write_t( CTStream *postrm) // throw char *
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  (*postrm).WriteID_t("BR3D");  // 'brush 3D'
  // write the brush version
  (*postrm)<<_iSupportedVersion;

  // write number of brush mips
  (*postrm)<<br_lhBrushMips.Count();
  // for each mip
  FOREACHINLIST(CBrushMip, bm_lnInBrush, br_lhBrushMips, itbm) {
    // write the brush mip itself
    itbm->Write_t(postrm);
  }

  (*postrm).WriteID_t("BREN");  // 'brush 3D end'
}

/*
 * Read from stream.
 */
void CBrush3D::Read_t( CTStream *pistrm) // throw char *
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);
  (*pistrm).ExpectID_t("BR3D");  // 'brush 3D'
  // read the version number
  INDEX iSavedVersion;
  (*pistrm)>>iSavedVersion;

  // if the version number is the newest
  if(iSavedVersion==_iSupportedVersion) {
    // read current version
    Read_new_t(pistrm);

  // if the version number is not the newest
  } else {
    // if the version can be converted
    if (iSavedVersion==_iSupportedVersion-1) {
      // show warning
      WarningMessage(
        TRANS("The brush version was %d.\n"
        "Auto-converting to version %d."),
        iSavedVersion, _iSupportedVersion);
      // read previous version
      Read_old_t(pistrm);
    } else {
      // report error
      ThrowF_t(
        TRANS("The brush version on disk is %d.\n"
        "Current supported version is %d."),
        iSavedVersion, _iSupportedVersion);
    }
  }
}

/*
 * Read from stream -- previous version.
 */
void CBrush3D::Read_old_t( CTStream *pistrm) // throw char *
{
  _ctPolygonsLoaded = 0;
  // read number of brush mips
  INDEX ctMips;
  (*pistrm)>>ctMips;
  // for each mip
  for(INDEX iMip=0; iMip<ctMips; iMip++) {
    // create a new brush mip
    CBrushMip *pbmMip = new CBrushMip;
    // add it to list
    br_lhBrushMips.AddTail(pbmMip->bm_lnInBrush);
    // read it from stream
    pbmMip->Read_old_t(pistrm);
    // set back-pointer to the brush
    pbmMip->bm_pbrBrush = this;
  }
 (*pistrm).ExpectID_t("BREN");  // 'brush 3D end'
  _RPT1(_CRT_WARN, "Polygons in brush: %d\n", _ctPolygonsLoaded);
}

/*
 * Read from stream -- current version.
 */
void CBrush3D::Read_new_t( CTStream *pistrm) // throw char *
{
  _ctPolygonsLoaded = 0;
  // read number of brush mips
  INDEX ctMips;
  (*pistrm)>>ctMips;
  // for each mip
  for(INDEX iMip=0; iMip<ctMips; iMip++) {
    // create a new brush mip
    CBrushMip *pbmMip = new CBrushMip;
    // add it to list
    br_lhBrushMips.AddTail(pbmMip->bm_lnInBrush);
    // set back-pointer to the brush
    pbmMip->bm_pbrBrush = this;
    // read it from stream
    pbmMip->Read_new_t(pistrm);
  }
 (*pistrm).ExpectID_t("BREN");  // 'brush 3D end'
  _RPT1(_CRT_WARN, "Polygons in brush: %d\n", _ctPolygonsLoaded);
}

/*
 * Write to stream.
 */
void CBrushMip::Write_t( CTStream *postrm) // throw char *
{
  // write the mip factor
  postrm->WriteID_t("BRMP");
  (*postrm)<<bm_fMaxDistance;
  // write number of sectors
  (*postrm)<<bm_abscSectors.Count();
  // for each sector
  FOREACHINDYNAMICARRAY(bm_abscSectors, CBrushSector, itbsc) {
    // write it to stream
    itbsc->Write_t(postrm);
  }
}

/*
 * Read from stream -- previous version.
 */
void CBrushMip::Read_old_t( CTStream *pistrm) // throw char *
{
  // read number of sectors
  INDEX ctSectors;
  (*pistrm)>>ctSectors;
  // create that much sectors
  bm_abscSectors.New(ctSectors);
  bm_abscSectors.Lock();
  // for each sector
  for(INDEX iSector=0; iSector<ctSectors; iSector++) {
    // read it from stream
    bm_abscSectors[iSector].Read_t(pistrm);
    // set back-pointer to the brush
    bm_abscSectors[iSector].bsc_pbmBrushMip = this;
  }
  bm_abscSectors.Unlock();
}

/*
 * Read from stream -- current version.
 */
void CBrushMip::Read_new_t( CTStream *pistrm) // throw char *
{
  // read mip factor
  BOOL bWithMipDistance = FALSE;
  if (pistrm->PeekID_t()==CChunkID("BRMP")) {
    pistrm->ExpectID_t("BRMP");
    bWithMipDistance = TRUE;
  }
  (*pistrm)>>bm_fMaxDistance;
  // if old mip-factor instead max distance
  if (!bWithMipDistance) {
    // convert from factor to distance
    if (bm_fMaxDistance==100.0f) {
      bm_fMaxDistance = 1E6f;
    } else {
      FLOAT fPerspectiveRatio = 640/(2.0f*Tan(90.0f/2));
      bm_fMaxDistance = fPerspectiveRatio*pow(2, bm_fMaxDistance)/1024;
    }
  }

  // read number of sectors
  INDEX ctSectors;
  (*pistrm)>>ctSectors;
  // create that much sectors
  bm_abscSectors.New(ctSectors);
  bm_abscSectors.Lock();
  // for each sector
  for(INDEX iSector=0; iSector<ctSectors; iSector++) {
    // set back-pointer to the brush
    bm_abscSectors[iSector].bsc_pbmBrushMip = this;
    // read it from stream
    bm_abscSectors[iSector].Read_t(pistrm);
  }
  bm_abscSectors.Unlock();
}

// read/write to stream
void CBrushPolygonTexture::Read_t( CTStream &strm) // throw char *
{
  CTFileName fnmTexture;
  strm>>fnmTexture;
  SetTextureWithPossibleReplacing_t(bpt_toTexture, fnmTexture);
  // gather CRC of that texture
  if (bpt_toTexture.GetData()!=NULL) {
    bpt_toTexture.GetData()->AddToCRCTable();
  }
  strm.Read_t(&bpt_mdMapping, sizeof(bpt_mdMapping));
  strm>>s.bpt_ubScroll;
  strm>>s.bpt_ubBlend;
  strm>>s.bpt_ubFlags;
  strm>>s.bpt_ubDummy;
  strm>>s.bpt_colColor;
}
void CBrushPolygonTexture::Write_t( CTStream &strm)  // throw char *
{
  strm<<bpt_toTexture.GetName();
  strm.Write_t(&bpt_mdMapping, sizeof(bpt_mdMapping));
  strm<<s.bpt_ubScroll;
  strm<<s.bpt_ubBlend;
  strm<<s.bpt_ubFlags;
  strm<<s.bpt_ubDummy;
  strm<<s.bpt_colColor;
}

/*
 * Write to stream.
 */
void CBrushSector::Write_t( CTStream *postrm) // throw char *
{
  // lock the brush elements
  LockAll();

  (*postrm).WriteID_t("BSC "); // 'brush sector'
  (*postrm)<<INDEX(BSCV_CURRENT);
  // write sector name
  (*postrm)<<bsc_strName;
  // write sector color and ambient light
  (*postrm)<<bsc_colColor;
  (*postrm)<<bsc_colAmbient;
  // write sector flags
  (*postrm)<<bsc_ulFlags;
  (*postrm)<<bsc_ulFlags2;
  (*postrm)<<bsc_ulVisFlags;

  (*postrm).WriteID_t("VTXs");  // 'vertices'
  // write the number of vertices in brush
  (*postrm)<<bsc_abvxVertices.Count();
  // for each vertex
  {FOREACHINSTATICARRAY(bsc_abvxVertices, CBrushVertex, itbvx) {
    // write precise vertex coordinates
    postrm->Write_t(&itbvx->bvx_vdPreciseRelative, sizeof(DOUBLE3D));
  }}

  (*postrm).WriteID_t("PLNs");  // 'planes'
  // write the number of planes in brush
  (*postrm)<<bsc_abplPlanes.Count();
  // for each plane
  {FOREACHINSTATICARRAY(bsc_abplPlanes, CBrushPlane, itbpl) {
    // write precise plane coordinates
    postrm->Write_t(&itbpl->bpl_pldPreciseRelative, sizeof(DOUBLEplane3D));
  }}

  (*postrm).WriteID_t("EDGs");  // 'edges'
  // write the number of edges in brush
  (*postrm)<<bsc_abedEdges.Count();
  // for each edge
  {FOREACHINSTATICARRAY(bsc_abedEdges, CBrushEdge, itbed) {
    // write indices of edge vertices
    (*postrm)<<bsc_abvxVertices.Index(itbed->bed_pbvxVertex0);
    (*postrm)<<bsc_abvxVertices.Index(itbed->bed_pbvxVertex1);
  }}

  (*postrm).WriteID_t("BPOs");  // 'brush polygons'
  (*postrm)<<INDEX(BPOV_CURRENT);
  // write the number of polygons in brush
  (*postrm)<<bsc_abpoPolygons.Count();
  // for each polygon
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo) {
    CBrushPolygon &bpo = *itbpo;
    // write index of the plane
    (*postrm)<<bsc_abplPlanes.Index(bpo.bpo_pbplPlane);
    // write polygon color
    (*postrm)<<bpo.bpo_colColor;
    // write polygon flags
    (*postrm)<<bpo.bpo_ulFlags;
    // write all textures
    bpo.bpo_abptTextures[0].Write_t(*postrm);
    bpo.bpo_abptTextures[1].Write_t(*postrm);
    bpo.bpo_abptTextures[2].Write_t(*postrm);
    // write other polygon properties
    (*postrm).Write_t(&bpo.bpo_bppProperties, sizeof(bpo.bpo_bppProperties));

    // write number of polygon edges
    (*postrm)<<bpo.bpo_abpePolygonEdges.Count();
    // for each polygon edge
    {FOREACHINSTATICARRAY(bpo.bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
      // get its edge index
      INDEX iEdge = bsc_abedEdges.Index(itbpe->bpe_pbedEdge);
      // if it is reverse edge
      if (itbpe->bpe_bReverse) {
        // set highest bit in the index
        iEdge |= 0x80000000;
      }
      // write the index
      (*postrm)<<iEdge;
    }}

    // write number of triangle vertices
    (*postrm)<<bpo.bpo_apbvxTriangleVertices.Count();
    // for each triangle vertex
    {FOREACHINSTATICARRAY(bpo.bpo_apbvxTriangleVertices, CBrushVertex *, itpbvx) {
      // write its index
      (*postrm)<<bsc_abvxVertices.Index(*itpbvx);
    }}

    // write number of triangle elements
    INDEX ctElements = bpo.bpo_aiTriangleElements.Count();
    (*postrm)<<ctElements;
    // write all element indices
    if (ctElements>0) {
      (*postrm).Write_t(&bpo.bpo_aiTriangleElements[0], ctElements*sizeof(INDEX));
    }

    // write the shadow-map (if it exists)
    bpo.bpo_smShadowMap.Write_t(postrm);
    // write shadow color
    (*postrm)<<bpo.bpo_colShadow;
  }}

  // unlock the brush elements
  UnlockAll();

  // write bsp
  (*postrm).WriteID_t("BSP0");
  bsc_bspBSPTree.Write_t(*postrm);
}

/*
 * Read from stream.
 */
void CBrushSector::Read_t( CTStream *pistrm) // throw char *
{
  // lock the brush elements
  LockAll();

  INDEX iBSCVersion=BSCV_OLD;
  if ((*pistrm).PeekID_t()==CChunkID("BSC ")) {
    (*pistrm).ExpectID_t("BSC "); // 'brush sector'
    (*pistrm)>>iBSCVersion;
    if (iBSCVersion<BSCV_OLD) {
      ThrowF_t(TRANS("Brush sector version too old (%d)."), iBSCVersion);
    }
  }

  // read sector name
  if (iBSCVersion>=BSCV_WITHNAME) {
    (*pistrm)>>bsc_strName;
  }

  // read sector color and ambient light
  (*pistrm)>>bsc_colColor;
  (*pistrm)>>bsc_colAmbient;
  // read sector flags
  (*pistrm)>>bsc_ulFlags;
  if (iBSCVersion>=BSCV_WITHFLAGS2) {
    (*pistrm)>>bsc_ulFlags2;
  }
  if (iBSCVersion>=BSCV_WITHVISFLAGS) {
    (*pistrm)>>bsc_ulVisFlags;
  }
  // clear sector flags for selection
  bsc_ulFlags &= ~(BSCF_SELECTED | BSCF_SELECTEDFORCSG);
  // no temp flags initially
  bsc_ulTempFlags = 0;

  (*pistrm).ExpectID_t("VTXs");  // 'vertices'
  // read the number of vertices in brush
  INDEX ctVertices;
  (*pistrm)>>ctVertices;
  // create that much vertices
  bsc_abvxVertices.New(ctVertices);
  bsc_awvxVertices.New(ctVertices);
  // for each vertex
  {FOREACHINSTATICARRAY(bsc_abvxVertices, CBrushVertex, itbvx) {
    // read precise vertex coordinates
   pistrm->Read_t(&itbvx->bvx_vdPreciseRelative, sizeof(DOUBLE3D));
    // remember sector pointer
    itbvx->bvx_pbscSector = this;
  }}

  (*pistrm).ExpectID_t("PLNs");  // 'planes'
  // read the number of planes in brush
  INDEX ctPlanes;
  (*pistrm)>>ctPlanes;
  // create that much planes
  bsc_abplPlanes.New(ctPlanes);
  bsc_awplPlanes.New(ctPlanes);
  // for each plane
  {FOREACHINSTATICARRAY(bsc_abplPlanes, CBrushPlane, itbpl) {
    // read precise plane coordinates
    pistrm->Read_t(&itbpl->bpl_pldPreciseRelative, sizeof(DOUBLEplane3D));
  }}

  (*pistrm).ExpectID_t("EDGs");  // 'edges'
  // read the number of edges in brush
  INDEX ctEdges;
  (*pistrm)>>ctEdges;
  // create that much edges
  bsc_abedEdges.New(ctEdges);
  bsc_awedEdges.New(ctEdges);
  // for all edges in object
  {for(INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    CBrushEdge &bed = bsc_abedEdges[iEdge];
    CWorkingEdge &wed = bsc_awedEdges[iEdge];
    // read indices of edge vertices
    INDEX iVertex0;
    (*pistrm)>>iVertex0;
    INDEX iVertex1;
    (*pistrm)>>iVertex1;
    // set vertex pointers
    bed.bed_pbvxVertex0 = &bsc_abvxVertices[iVertex0];
    bed.bed_pbvxVertex1 = &bsc_abvxVertices[iVertex1];
    // set the working edge
    bed.bed_pwedWorking = &wed;
    wed.wed_iwvx0 = iVertex0;
    wed.wed_iwvx1 = iVertex1;
  }}

  INDEX iBPOVersion;
  (*pistrm).ExpectID_t("BPOs");  // 'brush polygons'
  (*pistrm)>>iBPOVersion;
  if (iBPOVersion<BPOV_WITHHYPERTEXTURES) {
    ThrowF_t(TRANS("Brush polygon version too old (%d)."), iBPOVersion);
  }

  // read the number of polygons in brush
  INDEX ctPolygons;
  (*pistrm)>>ctPolygons;
  _ctPolygonsLoaded += ctPolygons;
  // create that much polygons
  bsc_abpoPolygons.New(ctPolygons);
  // for each polygon
  {FOREACHINSTATICARRAY(bsc_abpoPolygons, CBrushPolygon, itbpo) {
    CBrushPolygon &bpo = *itbpo;
    // read index of the plane
    INDEX iPlane;
    (*pistrm)>>iPlane;
    // set plane pointer
    bpo.bpo_pbplPlane = &bsc_abplPlanes[iPlane];

    if (iBPOVersion>=BPOV_MULTITEXTURING) {
      // read polygon color
      (*pistrm)>>bpo.bpo_colColor;
      // read polygon flags
      (*pistrm)>>bpo.bpo_ulFlags;
      // read all textures
      bpo.bpo_abptTextures[0].Read_t(*pistrm);
      bpo.bpo_abptTextures[1].Read_t(*pistrm);
      bpo.bpo_abptTextures[2].Read_t(*pistrm);

      // read other polygon properties
      (*pistrm).Read_t(&bpo.bpo_bppProperties, sizeof(bpo.bpo_bppProperties));

    } else {
      // read textures
      CTFileName fnmTexture;
      (*pistrm)>>fnmTexture;
      SetTextureWithPossibleReplacing_t(bpo.bpo_abptTextures[0].bpt_toTexture, fnmTexture);
      CTFileName fnmHyperTexture;
      (*pistrm)>>fnmHyperTexture;
      SetTextureWithPossibleReplacing_t(bpo.bpo_abptTextures[1].bpt_toTexture, fnmHyperTexture);
      // read polygon color
      (*pistrm)>>bpo.bpo_colColor;
      // read polygon flags
      (*pistrm)>>bpo.bpo_ulFlags;
      // read texture mapping
      bpo.bpo_mdShadow.ReadOld_t(*pistrm);
      // read other polygon properties
      (*pistrm).Read_t(&bpo.bpo_bppProperties, sizeof(bpo.bpo_bppProperties));

      // adjust polygon and texture properties
      bpo.bpo_abptTextures[0].bpt_mdMapping = bpo.bpo_mdShadow;
      bpo.bpo_abptTextures[0].s.bpt_ubBlend = BPT_BLEND_OPAQUE;
      bpo.bpo_abptTextures[0].s.bpt_colColor = C_WHITE|CT_OPAQUE;

      bpo.bpo_abptTextures[1].bpt_mdMapping = bpo.bpo_mdShadow;
      bpo.bpo_abptTextures[1].s.bpt_ubBlend = BPT_BLEND_SHADE;
      bpo.bpo_abptTextures[1].s.bpt_colColor = C_WHITE|CT_OPAQUE;

      bpo.bpo_abptTextures[2].bpt_mdMapping = bpo.bpo_mdShadow;
      bpo.bpo_abptTextures[2].s.bpt_ubBlend = BPT_BLEND_SHADE;
      bpo.bpo_abptTextures[2].s.bpt_colColor = C_WHITE|CT_OPAQUE;

      bpo.bpo_bppProperties.bpp_ubShadowBlend = BPT_BLEND_SHADE;

      if (bpo.bpo_ulFlags&BPOF_PORTAL) {
        bpo.bpo_abptTextures[0].s.bpt_ubBlend = BPT_BLEND_BLEND;
        bpo.bpo_abptTextures[1].s.bpt_ubBlend = BPT_BLEND_ADD;
        bpo.bpo_bppProperties.bpp_ubShadowBlend = BPT_BLEND_ADD;
      }
    }
    // if version before fake protal flag
    if(iBPOVersion<BPOV_FAKEPORTALFLAG) {
      // convert portal flag to fake
      if (bpo.bpo_ulFlags&OPOF_PORTAL) {
        bpo.bpo_ulFlags|=BPOF_PORTAL;
      } else {
        bpo.bpo_ulFlags&=~BPOF_PORTAL;
      }
    }

    // set sector pointer
    bpo.bpo_pbscSector = this;
    // clear polygon flags for selection
    bpo.bpo_ulFlags &= ~(BPOF_SELECTED | BPOF_SELECTEDFORCSG);

    // read number of polygon edges
    INDEX ctPolygonEdges;
    (*pistrm)>>ctPolygonEdges;
    // create that much polygons edges
    bpo.bpo_abpePolygonEdges.New(ctPolygonEdges);
    // for each polygon edge
    {FOREACHINSTATICARRAY(bpo.bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
      // read its edge index
      INDEX iEdge;
      (*pistrm)>>iEdge;
      // if the highest bit is set
      if (iEdge & 0x80000000) {
        // mark that it is reverse edge
        itbpe->bpe_bReverse = TRUE;
        // clear the highest bit
        iEdge &= ~0x80000000;
      } else {
        // mark that it is not reverse edge
        itbpe->bpe_bReverse = FALSE;
      }
      // set edge pointer
      itbpe->bpe_pbedEdge = &bsc_abedEdges[iEdge];
    }}

    // if triangles are saved
    if (iBPOVersion>=BPOV_TRIANGLES) {
      // read number of triangle vertices
      INDEX ctVertices;
      (*pistrm)>>ctVertices;
      // allocate them
      bpo.bpo_apbvxTriangleVertices.New(ctVertices);
      // for each triangle vertex
      {FOREACHINSTATICARRAY(bpo.bpo_apbvxTriangleVertices, CBrushVertex *, itpbvx) {
        // read its index
        INDEX ivx;
        (*pistrm)>>ivx;
        *itpbvx = &bsc_abvxVertices[ivx];
      }}

      // read number of triangle elements
      INDEX ctElements;
      (*pistrm)>>ctElements;
      // allocate them
      bpo.bpo_aiTriangleElements.New(ctElements);
      // read all element indices
      if (ctElements>0) {
        (*pistrm).Read_t(&bpo.bpo_aiTriangleElements[0], ctElements*sizeof(INDEX));
      }
    }

    // read the shadow-map (if it exists)
    bpo.bpo_smShadowMap.Read_t(pistrm);
    if (iBPOVersion>=BPOV_MULTITEXTURING) {
      // read shadow color
      (*pistrm)>>bpo.bpo_colShadow;
    } else {
      // read shadow animation object index
      UBYTE ubDummy;
      (*pistrm)>>ubDummy;
      bpo.bpo_colShadow = C_WHITE|CT_OPAQUE;
    }
  }}

  // unlock the brush elements
  UnlockAll();

  // calculate the volume of the sector
  CalculateVolume();

  bsc_ulTempFlags&=~BSCTF_PRELOADEDBSP;
  // if there is current version of bsp saved
  if ((*pistrm).PeekID_t()==CChunkID("BSP0")) {
    _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_READBSP);
    (*pistrm).ExpectID_t("BSP0");
    // read it
    bsc_bspBSPTree.Read_t(*pistrm);
    // if read ok
    if (bsc_bspBSPTree.bt_abnNodes.Count()>0) {
      // mark that tree doesn't have to be recalculated
      bsc_ulTempFlags|=BSCTF_PRELOADEDBSP;
    }
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_READBSP);
  }
}
