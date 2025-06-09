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

#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/World/World.h>
#include <Engine/Math/Float.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Base/Stream.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Base/ListIterator.inl>

#include <Engine/Templates/BSP.h>
#include <Engine/Templates/BSP_internal.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>

// !!! FIXME: This confuses GCC, since CDynamicArray is a #included
// !!! FIXME:  source file, and it ends up compiling the template more than
// !!! FIXME:  once.  :(   --ryan.
#ifdef _MSC_VER
template CDynamicArray<CBrush3D>;
#endif

#ifdef _MSC_VER
extern BOOL _bPortalSectorLinksPreLoaded = FALSE;
extern BOOL _bEntitySectorLinksPreLoaded = FALSE;
#else
BOOL _bPortalSectorLinksPreLoaded = FALSE;
BOOL _bEntitySectorLinksPreLoaded = FALSE;
#endif

/*
 * Calculate bounding boxes in all brushes.
 */
void CBrushArchive::CalculateBoundingBoxes(void)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_CALCULATEBOUNDINGBOXES);
  // for each of the brushes
  FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr) {
    // if the brush has no entity
    if (itbr->br_penEntity==NULL) {
      // skip it
      continue;
    }
    // calculate its boxes
    itbr->CalculateBoundingBoxes();
  }
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_CALCULATEBOUNDINGBOXES);
}

/* Make indices for all brush elements. */
void CBrushArchive::MakeIndices(void)
{
  // NOTE: Mips and brushes don't have indices, because it is not needed yet.
  // Polygon and sector indices are needed for loading/saving of portal-sector links.

  //INDEX ctBrushes=0; // ####### not used
  //INDEX ctMips=0; ctLocals
  INDEX ctSectors=0;
  INDEX ctPolygons=0;
  // for each brush
  FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr) {
    // for each mip in the brush
    FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) {
      // for each sector in the brush mip
      FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
        // for each polygon in the sector
        FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
          itbpo->bpo_iInWorld = ctPolygons;
          ctPolygons++;
        }
        itbsc->bsc_iInWorld = ctSectors;
        ctSectors++;
      }
      //ctMips++;
    }
    //ctBrushes++;
  }

  // make arrays of pointers to sectors and polygons
  ba_apbpo.Clear();
  ba_apbpo.New(ctPolygons);
  ba_apbsc.Clear();
  ba_apbsc.New(ctSectors);
  {FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr) {
    FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) {
      FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
        ba_apbsc[itbsc->bsc_iInWorld] = itbsc;
        FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
          ba_apbpo[itbpo->bpo_iInWorld] = itbpo;
        }
      }
    }
  }}
}

#define DISTANCE_EPSILON 0.1f
/* Create links between portals and sectors on their other side. */
void CBrushArchive::LinkPortalsAndSectors(void)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_LINKPORTALSANDSECTORS);
  ASSERT(GetFPUPrecision()==FPT_53BIT);

  // for each of the zoning brushes
  FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr1) {
    if (itbr1->br_penEntity==NULL || !(itbr1->br_penEntity->en_ulFlags&ENF_ZONING)) {
      continue;
    }
    // for each mip
    FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr1->br_lhBrushMips, itbm1) {
      // for each sector in the brush mip
      FOREACHINDYNAMICARRAY(itbm1->bm_abscSectors, CBrushSector, itbsc1) {

        // for each of the zoning brushes
        FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr2) {
          if (itbr2->br_penEntity==NULL || !(itbr2->br_penEntity->en_ulFlags&ENF_ZONING)) {
            continue;
          }
          // for each mip that might have contact with the sector
          FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr2->br_lhBrushMips, itbm2) {
            if (!itbm2->bm_boxBoundingBox.HasContactWith(itbsc1->bsc_boxBoundingBox, DISTANCE_EPSILON)) {
              continue;
            }
            // for each sector in the brush mip that might have contact, except current one
            FOREACHINDYNAMICARRAY(itbm2->bm_abscSectors, CBrushSector, itbsc2) {
              if (&*itbsc1==&*itbsc2) {
                continue;
              }
              if (!itbsc2->bsc_boxBoundingBox.HasContactWith(itbsc1->bsc_boxBoundingBox, DISTANCE_EPSILON)) {
                continue;
              }
              // for all portals in this sector that might have contact
              FOREACHINSTATICARRAY(itbsc2->bsc_abpoPolygons, CBrushPolygon, itbpo2) {
                if (!(itbpo2->bpo_ulFlags&(BPOF_PORTAL|BPOF_PASSABLE))) {
                  continue;
                }
                if (!itbpo2->bpo_boxBoundingBox.HasContactWith(itbsc1->bsc_boxBoundingBox, DISTANCE_EPSILON)) {
                  continue;
                }
                // create a BSP polygon from the brush polygon
                CBrushPolygon        &brpo2 = *itbpo2;
                BSPPolygon<DOUBLE, 3> bspo2;
                brpo2.CreateBSPPolygonNonPrecise(bspo2);
                // split the polygon with the BSP of the sector
                DOUBLEbspcutter3D bcCutter(bspo2, *itbsc1->bsc_bspBSPTree.bt_pbnRoot);
                // if anything remains on the border looking outside
                if (bcCutter.bc_abedInside.Count()>0
                  ||bcCutter.bc_abedBorderInside.Count()>0
                  ||bcCutter.bc_abedBorderOutside.Count()>0) {
                  // relate the sector to the portal
                  AddRelationPair(
                    itbpo2->bpo_rsOtherSideSectors,
                    itbsc1->bsc_rdOtherSidePortals);
                }
              }
            }
          }
        }
      }
    }
  }
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_LINKPORTALSANDSECTORS);
}


// remove shadow layers without valid light source in all brushes
void CBrushArchive::RemoveDummyLayers(void)
{
  // for each brush
  FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr) { // for each mip
    if( itbr->br_penEntity==NULL) continue; // skip brush without entity
    FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) { // for each sector in the brush mip
      FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) { // for each polygon in the sector
        FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
          CBrushPolygon &bpo = *itbpo;
          bpo.bpo_smShadowMap.RemoveDummyLayers(); // remove shadow layers without valid light source
        }
      }
    }
  }
}


// cache all shadowmaps 
void CBrushArchive::CacheAllShadowmaps(void)
{
  // count all shadowmaps
  INDEX ctShadowMaps=0;
  {FOREACHINDYNAMICARRAY( ba_abrBrushes, CBrush3D, itbr) { // for each mip
    if( itbr->br_penEntity==NULL) continue; // skip brush without entity
    FOREACHINLIST( CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) { // for each sector in the brush mip
      FOREACHINDYNAMICARRAY( itbm->bm_abscSectors, CBrushSector, itbsc) { // for each polygon in the sector
        FOREACHINSTATICARRAY( itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
          if( !itbpo->bpo_smShadowMap.bsm_lhLayers.IsEmpty()) ctShadowMaps++; // count shadowmap if the one exist
        }
      }
    }
  }}

  try {
    SetProgressDescription( TRANS("caching shadowmaps"));
    CallProgressHook_t(0.0f);
    // for each brush
    INDEX iCurrentShadowMap=0;
    {FOREACHINDYNAMICARRAY( ba_abrBrushes, CBrush3D, itbr) { // for each mip
      if( itbr->br_penEntity==NULL) continue; // skip brush without entity
      FOREACHINLIST( CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) { // for each sector in the brush mip
        FOREACHINDYNAMICARRAY( itbm->bm_abscSectors, CBrushSector, itbsc) { // for each polygon in the sector
          FOREACHINSTATICARRAY( itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
            // cache shadowmap if the one exist
            CBrushShadowMap &bsm = itbpo->bpo_smShadowMap;
            if( bsm.bsm_lhLayers.IsEmpty()) continue;
            bsm.CheckLayersUpToDate();
            bsm.Prepare();
            bsm.SetAsCurrent();
            iCurrentShadowMap++;
            CallProgressHook_t( (FLOAT)iCurrentShadowMap/ctShadowMaps);
          }
        }
      }
    }}
    // all done
    CallProgressHook_t(1.0f);
  }
  catch (const char*) { NOTHING; }
}


void CBrushArchive::ReadPortalSectorLinks_t( CTStream &strm)  // throw char *
{
  // links are not ok if they fail loading
  _bPortalSectorLinksPreLoaded = FALSE;

  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_READPORTALSECTORLINKS);
  // first make indices for all sectors and polygons
  MakeIndices();

  // if the chunk is not there
  if (!(strm.PeekID_t()==CChunkID("PSLS"))) {   // portal-sector links
    // do nothing;
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_READPORTALSECTORLINKS);
    return;
  }

  // read the version
  strm.ExpectID_t("PSLS");   // portal-sector links
  INDEX iVersion;
  strm>>iVersion;
  ASSERT(iVersion==1);
  // read chunk size
  SLONG slChunkSizePos = strm.GetPos_t();
  SLONG slChunkSize;
  strm>>slChunkSize;

  // repeat
  FOREVER {
    // read sector index
    INDEX iSector;
    strm>>iSector;
    // if end marker
    if (iSector==-1) {
      // stop loading
      break;
    }
    // get the sector
    CBrushSector *pbsc = ba_apbsc[iSector];
    ASSERT(pbsc->bsc_iInWorld==iSector);
    // read number of links
    INDEX ctLinks;
    strm>>ctLinks;
    // for each link
    for(INDEX iLink=0; iLink<ctLinks; iLink++) {
      // read polygon index
      INDEX iPolygon;
      strm>>iPolygon;
      CBrushPolygon *pbpo = ba_apbpo[iPolygon];
      ASSERT(pbpo->bpo_iInWorld==iPolygon);
      // relate the sector to the portal
      AddRelationPair(
        pbpo->bpo_rsOtherSideSectors,
        pbsc->bsc_rdOtherSidePortals);
    }
    pbsc->bsc_ulTempFlags|=BSCTF_PRELOADEDLINKS;
  }

  // check chunk size
  ASSERT(strm.GetPos_t()-slChunkSizePos-sizeof(INDEX)==slChunkSize);
  // check end id
  strm.ExpectID_t("PSLE");   // portal-sector links end
  // mark that links are ok
  _bPortalSectorLinksPreLoaded = TRUE;
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_READPORTALSECTORLINKS);
}

void CBrushArchive::ReadEntitySectorLinks_t( CTStream &strm)  // throw char *
{
  // links are not ok if they fail loading
  _bEntitySectorLinksPreLoaded = FALSE;

  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_READPORTALSECTORLINKS);
  // first make indices for all sectors and polygons
  MakeIndices();

  // if the chunk is not there
  if (!(strm.PeekID_t()==CChunkID("ESL2"))) {   // entity-sector links v2
    // do nothing;
    _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_READPORTALSECTORLINKS);
    return;
  }

  // read the version
  strm.ExpectID_t("ESL2");   // entity-sector links v2
  INDEX iVersion;
  strm>>iVersion;
  ASSERT(iVersion==1);
  // read chunk size
  SLONG slChunkSizePos = strm.GetPos_t();
  SLONG slChunkSize;
  strm>>slChunkSize;

  // repeat
  FOREVER {
    // read sector index
    INDEX iSector;
    strm>>iSector;
    // if end marker
    if (iSector==-1) {
      // stop loading
      break;
    }
    // get the sector
    CBrushSector *pbsc = ba_apbsc[iSector];
    ASSERT(pbsc->bsc_iInWorld==iSector);
    // read number of links
    INDEX ctLinks;
    strm>>ctLinks;
    // for each link
    for(INDEX iLink=0; iLink<ctLinks; iLink++) {
      // read entity index
      INDEX iEntity;
      strm>>iEntity;
      CEntity *pen = ba_pwoWorld->EntityFromID(iEntity);
      // relate the sector to the entity
      AddRelationPair(pbsc->bsc_rsEntities, pen->en_rdSectors);
    }
  }

  // check chunk size
  ASSERT(strm.GetPos_t()-slChunkSizePos-sizeof(INDEX)==slChunkSize);
  // check end id
  strm.ExpectID_t("ESLE");   // entity-sector links end

  // mark that links are ok
  _bEntitySectorLinksPreLoaded = TRUE;
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_READPORTALSECTORLINKS);
}

void CBrushArchive::WritePortalSectorLinks_t( CTStream &strm) // throw char *
{
  // first make indices for all sectors and polygons
  MakeIndices();

  // write chunk id and current version
  strm.WriteID_t("PSLS");   // portal-sector links
  strm<<INDEX(1);
  // leave room for chunk size
  SLONG slChunkSizePos = strm.GetPos_t();
  strm<<SLONG(0);

  // for each sector
  {FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr) {
    FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) {
      FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
        CBrushSector *pbsc = itbsc;
        // get number of portal links that it has
        INDEX ctLinks = pbsc->bsc_rdOtherSidePortals.Count();
        // if it has no links
        if (ctLinks==0) {
          // skip it
          continue;
        }
        // write sector index and number of links
        strm<<pbsc->bsc_iInWorld<<ctLinks;
        // for each link
        {FOREACHSRCOFDST(pbsc->bsc_rdOtherSidePortals, CBrushPolygon, bpo_rsOtherSideSectors, pbpo)
          // write the polygon index
          strm<<pbpo->bpo_iInWorld;
        ENDFOR}
      }
    }
  }}
  // write sector index -1 as end marker
  strm<<INDEX(-1);

  // write back the chunk size
  SLONG slEndPos = strm.GetPos_t();
  strm.SetPos_t(slChunkSizePos);
  strm<<SLONG(slEndPos-slChunkSizePos-sizeof(INDEX));
  strm.SetPos_t(slEndPos);

  // write end id for checking
  strm.WriteID_t("PSLE");   // portal-sector links end
}

void CBrushArchive::WriteEntitySectorLinks_t( CTStream &strm) // throw char *
{
  // first make indices for all sectors and polygons
  MakeIndices();

  // write chunk id and current version
  strm.WriteID_t("ESL2");   // entity-sector links v2
  strm<<INDEX(1);
  // leave room for chunk size
  SLONG slChunkSizePos = strm.GetPos_t();
  strm<<SLONG(0);

  // for each sector
  {FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr) {
    FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) {
      FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
        CBrushSector *pbsc = itbsc;
        // get number of entity links that it has
        INDEX ctLinks = pbsc->bsc_rsEntities.Count();
        // if it has no links
        if (ctLinks==0) {
          // skip it
          continue;
        }
        // write sector index and number of links
        strm<<pbsc->bsc_iInWorld<<ctLinks;
        // for each link
        {FOREACHDSTOFSRC(pbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
          // write the entity index
          strm<<pen->en_ulID;
        ENDFOR}
      }
    }
  }}
  // write sector index -1 as end marker
  strm<<INDEX(-1);

  // write back the chunk size
  SLONG slEndPos = strm.GetPos_t();
  strm.SetPos_t(slChunkSizePos);
  strm<<SLONG(slEndPos-slChunkSizePos-sizeof(INDEX));
  strm.SetPos_t(slEndPos);
  // write end id for checking
  strm.WriteID_t("ESLE");   // entity-sector links end
}

/*
 * Read from stream.
 */
void CBrushArchive::Read_t( CTStream *istrFile) // throw char *
{
  istrFile->ExpectID_t("BRAR");   // brush archive

  INDEX ctBrushes;
  // read number of brushes
  (*istrFile)>>ctBrushes;

  // if there are some brushes
  if (ctBrushes!=0) {
    // create that much brushes
    CBrush3D *abrBrushes = ba_abrBrushes.New(ctBrushes);
    // for each of the new brushes
    for (INDEX iBrush=0; iBrush<ctBrushes; iBrush++) {
      // read it from stream
      CallProgressHook_t(FLOAT(iBrush)/ctBrushes);
      abrBrushes[iBrush].Read_t(istrFile);
    }
  }

  // read links if possible
  ReadPortalSectorLinks_t(*istrFile);

  istrFile->ExpectID_t("EOAR");   // end of archive
}

/*
 * Write to stream.
 */
void CBrushArchive::Write_t( CTStream *ostrFile) // throw char *
{
  ostrFile->WriteID_t("BRAR");   // brush archive

  // write the number of brushes
  (*ostrFile)<<ba_abrBrushes.Count();
  // for each of the brushes
  FOREACHINDYNAMICARRAY(ba_abrBrushes, CBrush3D, itbr) {
    // write it to stream
    itbr->Write_t(ostrFile);
  }

  // write links
  WritePortalSectorLinks_t(*ostrFile);
  ostrFile->WriteID_t("EOAR");   // end of archive
}
