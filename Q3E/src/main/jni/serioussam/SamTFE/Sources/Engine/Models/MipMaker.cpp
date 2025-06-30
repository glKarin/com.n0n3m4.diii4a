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

#include <Engine/Models/MipMaker.h>
#include <Engine/Math/Vector.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/ErrorReporting.h>

#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicContainer.cpp>

// if vertex removing should occure only inside surfaces
static BOOL _bPreserveSurfaces;


CMipModel::~CMipModel()
{
  mm_amsSurfaces.Clear();
  mm_ampPolygons.Clear();
  mm_amvVertices.Clear();
}

CMipVertex::CMipVertex()
{
}
CMipVertex::~CMipVertex()
{
}
void CMipVertex::Clear()
{
}

CMipPolygon::CMipPolygon()
{
  mp_pmpvFirstPolygonVertex = NULL;
}
CMipPolygon::~CMipPolygon()
{
  Clear();
}
void CMipPolygon::Clear()
{
  if (mp_pmpvFirstPolygonVertex!=NULL) {
    // delete all vertices in this polygon
    CMipPolygonVertex *pmpvCurrentInPolygon = mp_pmpvFirstPolygonVertex;
    do
    {
      CMipPolygonVertex *pvNextInPolygon = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
      delete pmpvCurrentInPolygon;
      pmpvCurrentInPolygon = pvNextInPolygon;
    }
    while( pmpvCurrentInPolygon != mp_pmpvFirstPolygonVertex);
  }
  mp_pmpvFirstPolygonVertex = NULL;
}

void CMipModel::ToObject3D( CObject3D &objDestination)
{
  // add one sector
  CObjectSector *pOS = objDestination.ob_aoscSectors.New(1);
  // add vertices to sector
  pOS->osc_aovxVertices.New( mm_amvVertices.Count());
  pOS->osc_aovxVertices.Lock();
  INDEX iVertice = 0;
  FOREACHINDYNAMICARRAY( mm_amvVertices, CMipVertex, itVertice)
  {
    FLOAT3D vRestFrame = itVertice->mv_vRestFrameCoordinate;
    pOS->osc_aovxVertices[ iVertice] = FLOATtoDOUBLE( vRestFrame);
    iVertice++;
  }

  // add mip surfaces as materials to object 3d
  pOS->osc_aomtMaterials.New( mm_amsSurfaces.Count());
  pOS->osc_aomtMaterials.Lock();
  INDEX iMaterial = 0;
  FOREACHINDYNAMICARRAY( mm_amsSurfaces, CMipSurface, itSurface)
  {
    pOS->osc_aomtMaterials[ iMaterial].omt_Name = itSurface->ms_strName;
    pOS->osc_aomtMaterials[ iMaterial].omt_Color = itSurface->ms_colColor;
    iMaterial ++;
  }


  // add polygons to object 3d
  FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    // prepare array of polygon vertex indices
    INDEX aivVertices[ 32];
    CMipPolygonVertex *pmpvPolygonVertex = itPolygon->mp_pmpvFirstPolygonVertex;
    INDEX ctPolygonVertices = 0;
    do
    {
      ASSERT( ctPolygonVertices<32);
      if( ctPolygonVertices >= 32) break;
      // add global index of vertex to list of vertex indices of polygon
      mm_amvVertices.Lock();
      aivVertices[ ctPolygonVertices] =
        mm_amvVertices.Index( pmpvPolygonVertex->mpv_pmvVertex);
      mm_amvVertices.Unlock();
      pmpvPolygonVertex = pmpvPolygonVertex->mpv_pmpvNextInPolygon;
      ctPolygonVertices ++;
    }
    while( pmpvPolygonVertex != itPolygon->mp_pmpvFirstPolygonVertex);
    // add current polygon
    pOS->CreatePolygon( ctPolygonVertices, aivVertices,
                        pOS->osc_aomtMaterials[ itPolygon->mp_iSurface], 0, FALSE);
  }
  pOS->osc_aomtMaterials.Unlock();
  pOS->osc_aovxVertices.Unlock();
}

void CMipModel::FromObject3D_t( CObject3D &objRestFrame, CObject3D &objMipSourceFrame)
{
  INDEX ctInvalidVertices = 0;
  CTString strInvalidVertices;
  char achrErrorVertice[ 256];
  objMipSourceFrame.ob_aoscSectors.Lock();
  objRestFrame.ob_aoscSectors.Lock();
  // lock object sectors dynamic array
  objMipSourceFrame.ob_aoscSectors[0].LockAll();
  objRestFrame.ob_aoscSectors[0].LockAll();

  CObjectSector *pOS = &objMipSourceFrame.ob_aoscSectors[0];
  // add mip surface
  mm_amsSurfaces.New( pOS->osc_aomtMaterials.Count());
  // copy material data from object 3d to mip surfaces
  INDEX iMaterial = 0;
  FOREACHINDYNAMICARRAY( mm_amsSurfaces, CMipSurface, itSurface)
  {
    itSurface->ms_strName = pOS->osc_aomtMaterials[ iMaterial].omt_Name;
    itSurface->ms_colColor = pOS->osc_aomtMaterials[ iMaterial].omt_Color;
    iMaterial ++;
  }

  // add mip vertices
  mm_amvVertices.New( pOS->osc_aovxVertices.Count());
  // copy vertice coordinates from object3d to mip vertices
  INDEX iVertice = 0;
  {FOREACHINDYNAMICARRAY( mm_amvVertices, CMipVertex, itVertice)
  {
    (FLOAT3D &)(*itVertice) = DOUBLEtoFLOAT( pOS->osc_aovxVertices[ iVertice]);
    itVertice->mv_vRestFrameCoordinate =
      DOUBLEtoFLOAT( objRestFrame.ob_aoscSectors[0].osc_aovxVertices[ iVertice]);
    // calculate bounding box of all vertices
    mm_boxBoundingBox |= *itVertice;
    iVertice++;
  }}

  // add mip polygons
  mm_ampPolygons.New( pOS->osc_aopoPolygons.Count());
  // copy polygons object 3d to mip polygons
  INDEX iPolygon = 0;
  FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    CObjectPolygon &opoPolygon = pOS->osc_aopoPolygons[ iPolygon];
    CMipPolygon &mpPolygon = itPolygon.Current();
    INDEX ctPolygonVertices = opoPolygon.opo_PolygonEdges.Count();
    // allocate polygon vertices
    CMipPolygonVertex *ppvPolygonVertices[ 32];
    INDEX iPolygonVertice;
    for( iPolygonVertice=0; iPolygonVertice<ctPolygonVertices; iPolygonVertice++)
    {
      // allocate one polygon vertex
      ppvPolygonVertices[ iPolygonVertice] = new( CMipPolygonVertex);
    }

    opoPolygon.opo_PolygonEdges.Lock();
    // for each polygon vertex in the polygon
    for( iPolygonVertice=0; iPolygonVertice<ctPolygonVertices; iPolygonVertice++)
    {
      CMipPolygonVertex *ppvPolygonVertex = ppvPolygonVertices[ iPolygonVertice];
      // get the object vertex as first vertex of the edge
      CObjectVertex *povxStart, *povxEnd;
      opoPolygon.opo_PolygonEdges[iPolygonVertice].GetVertices( povxStart, povxEnd);
      INDEX iVertexInSector = pOS->osc_aovxVertices.Index( povxStart);
      // set references to mip polygon and mip vertex
      ppvPolygonVertex->mpv_pmpPolygon = &mpPolygon;
      mm_amvVertices.Lock();
      ppvPolygonVertex->mpv_pmvVertex = &mm_amvVertices[iVertexInSector];
      mm_amvVertices.Unlock();
      // link to previous and next vertices in the mip polygon
      INDEX iNext=(iPolygonVertice+1)%ctPolygonVertices;
      ppvPolygonVertex->mpv_pmpvNextInPolygon = ppvPolygonVertices[ iNext];
    }
    opoPolygon.opo_PolygonEdges.Unlock();

    // set first polygon vertex ptr and surface index to polygon
    itPolygon->mp_pmpvFirstPolygonVertex = ppvPolygonVertices[ 0];
    itPolygon->mp_iSurface =
      pOS->osc_aomtMaterials.Index( opoPolygon.opo_Material);
    iPolygon++;
  }

	objRestFrame.ob_aoscSectors[0].UnlockAll();
  // unlock all dynamic arrays in sector
	objMipSourceFrame.ob_aoscSectors[0].UnlockAll();
  objRestFrame.ob_aoscSectors.Unlock();
  objMipSourceFrame.ob_aoscSectors.Unlock();

  if( ctInvalidVertices != 0)
  {
    sprintf( achrErrorVertice,
      "%d invalid vertices found\n-------------------------\n\n", ctInvalidVertices);
    strInvalidVertices = CTString( achrErrorVertice) + strInvalidVertices;
    strInvalidVertices.Save_t( CTFileName(CTString("Temp\\ErrorVertices.txt")));
    ThrowF_t( "%d invalid vertices found.\nUnable to create mip models.\nList of vertices "
      "that must be fixed can be found in file: \"Temp\\ErrorVertices.txt\".",
      ctInvalidVertices);
  }
}

void CMipModel::CheckObjectValidity(void)
{
  // for all polygons
  FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    CMipPolygon &mpMipPolygon = *itPolygon;
    CMipPolygonVertex *pvFirstInPolygon = mpMipPolygon.mp_pmpvFirstPolygonVertex;
    CMipPolygonVertex *pvCurrent = pvFirstInPolygon;
    do
    {
      ASSERT( pvCurrent->mpv_pmpPolygon == &mpMipPolygon);
      pvCurrent = pvCurrent->mpv_pmpvNextInPolygon;
    }
    while( pvCurrent != pvFirstInPolygon);
  }
}

FLOAT CMipModel::GetGoodness(CMipVertex *pmvSource, CMipVertex *pmvTarget)
{
  if( (_bPreserveSurfaces) && (pmvSource->mv_iSurface == -2) ) return -10000.0f;
  FLOAT fDistST = ( *pmvSource - *pmvTarget).Length();
  FLOAT fDistBBoxCenterT = ( mm_boxBoundingBox.Center() - *pmvTarget).Length();
  return fDistBBoxCenterT/100.0f + 1.0f/fDistST;
}

INDEX CMipModel::FindSurfacesForVertices(void)
{
  {FOREACHINDYNAMICARRAY( mm_amvVertices, CMipVertex, itVertice)
  {
    itVertice->mv_iSurface = -1;
  }}

  // for all polygons
  {FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    // for all vertices in this polygon
    CMipPolygonVertex *pmpvCurrentInPolygon = itPolygon->mp_pmpvFirstPolygonVertex;
    do
    {
      CMipVertex *pmvVertex = pmpvCurrentInPolygon->mpv_pmvVertex;
      if( pmvVertex->mv_iSurface == -1) pmvVertex->mv_iSurface = itPolygon->mp_iSurface;
      else if( pmvVertex->mv_iSurface == -2); // do nothing
      else if( pmvVertex->mv_iSurface == itPolygon->mp_iSurface);  // do nothing
      else pmvVertex->mv_iSurface = -2;
      pmpvCurrentInPolygon = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
    }
    while( pmpvCurrentInPolygon != itPolygon->mp_pmpvFirstPolygonVertex);
  }}

  // count vertices that are sourounded with only one surface
  INDEX ctVerticesWithOneSurface = 0;
  // for all vertices
  {FOREACHINDYNAMICARRAY( mm_amvVertices, CMipVertex, itVertice)
  {
    if( itVertice->mv_iSurface >= 0) ctVerticesWithOneSurface++;
  }}
  return ctVerticesWithOneSurface;
}

void CMipModel::JoinVertexPair( CMipVertex *pmvBestSource, CMipVertex *pmvBestTarget)
{
  // for all polygons
  {FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    // for all vertices in this polygon
    CMipPolygonVertex *pmpvCurrentInPolygon = itPolygon->mp_pmpvFirstPolygonVertex;
    do
    {
      if( pmpvCurrentInPolygon->mpv_pmvVertex == pmvBestSource)
      {
        pmpvCurrentInPolygon->mpv_pmvVertex = pmvBestTarget;
      }
      pmpvCurrentInPolygon = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
    }
    while( pmpvCurrentInPolygon != itPolygon->mp_pmpvFirstPolygonVertex);
  }}
  // delete best source vertex
  mm_amvVertices.Delete( pmvBestSource);

  // for all polygons
  {FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    // for all vertices in this polygon
    CMipPolygonVertex *pmpvCurrentInPolygon = itPolygon->mp_pmpvFirstPolygonVertex;
    do
    {
      CMipPolygonVertex *pmpvSuccesor = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
      // if current vertex and its sucessor are same mip vertex
      if( pmpvCurrentInPolygon->mpv_pmvVertex == pmpvSuccesor->mpv_pmvVertex)
      {
        // enable looping even if vertex that is first in polygon is deleted
        if( pmpvSuccesor == itPolygon->mp_pmpvFirstPolygonVertex)
        {
          itPolygon->mp_pmpvFirstPolygonVertex = pmpvSuccesor->mpv_pmpvNextInPolygon;
        }
        // relink current vertex over sucessor
        pmpvCurrentInPolygon->mpv_pmpvNextInPolygon = pmpvSuccesor->mpv_pmpvNextInPolygon;
        // delete sucessor vertex
        delete pmpvSuccesor;
      }
      pmpvCurrentInPolygon = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
    }
    while( pmpvCurrentInPolygon != itPolygon->mp_pmpvFirstPolygonVertex);
  }}

  CDynamicContainer<CMipPolygon> cPolygonsToDelete;
  // for all polygons
  {FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    CMipPolygonVertex *pmpvFirst = itPolygon->mp_pmpvFirstPolygonVertex;
    // if this is polygon with one or two vertices
    if( (pmpvFirst->mpv_pmpvNextInPolygon == pmpvFirst) ||
        (pmpvFirst->mpv_pmpvNextInPolygon->mpv_pmpvNextInPolygon == pmpvFirst) )
    {
      // add it to container for deleting
      cPolygonsToDelete.Add( &itPolygon.Current());
    }
  }}
  // delete polygons
  {FOREACHINDYNAMICCONTAINER(cPolygonsToDelete, CMipPolygon, itPolygon)
  {
    mm_ampPolygons.Delete( &itPolygon.Current());
  }}
}

void CMipModel::FindBestVertexPair( CMipVertex *&pmvBestSource, CMipVertex *&pmvBestTarget)
{
  pmvBestSource = NULL;
  pmvBestTarget = NULL;
  FLOAT fBestGoodnes = -999999.9f;
  // for all polygons
  {FOREACHINDYNAMICARRAY( mm_ampPolygons, CMipPolygon, itPolygon)
  {
    // for all vertices in this polygon
    CMipPolygonVertex *pmpvCurrentInPolygon = itPolygon->mp_pmpvFirstPolygonVertex;
    do
    {
      CMipVertex *pmvSource = pmpvCurrentInPolygon->mpv_pmvVertex;
      CMipVertex *pmvDestination = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon->mpv_pmvVertex;
      FLOAT fCurrentGoodnes = GetGoodness( pmvSource, pmvDestination);
      if( fCurrentGoodnes > fBestGoodnes)
      {
        fBestGoodnes = fCurrentGoodnes;
        pmvBestSource = pmvSource;
        pmvBestTarget = pmvDestination;
      }
      // now for inverted order
      pmvSource = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon->mpv_pmvVertex;
      pmvDestination = pmpvCurrentInPolygon->mpv_pmvVertex;
      fCurrentGoodnes = GetGoodness( pmvSource, pmvDestination);
      if( fCurrentGoodnes > fBestGoodnes)
      {
        fBestGoodnes = fCurrentGoodnes;
        pmvBestSource = pmvSource;
        pmvBestTarget = pmvDestination;
      }
      pmpvCurrentInPolygon = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
    }
    while( pmpvCurrentInPolygon != itPolygon->mp_pmpvFirstPolygonVertex);
  }}
  ASSERT( (pmvBestSource != NULL) && (pmvBestTarget != NULL) );
  ASSERT( pmvBestSource != pmvBestTarget);
}

void CMipModel::RemoveUnusedVertices(void)
{
  // if there are no vertices
  if (mm_amvVertices.Count()==0) {
    // do nothing
    return;
  }

  // clear all vertex tags
  {FOREACHINDYNAMICARRAY(mm_amvVertices, CMipVertex, itmvtx) {
    itmvtx->mv_bUsed = FALSE;
  }}

  // mark all vertices that are used by some polygon
  {FOREACHINDYNAMICARRAY(mm_ampPolygons, CMipPolygon, itpo) {
    CMipPolygonVertex *pmpvCurrentInPolygon = itpo->mp_pmpvFirstPolygonVertex;
    do
    {
      pmpvCurrentInPolygon->mpv_pmvVertex->mv_bUsed = TRUE;
      pmpvCurrentInPolygon = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
    }
    while( pmpvCurrentInPolygon != itpo->mp_pmpvFirstPolygonVertex);
  }}

  // find number of used vertices
  INDEX ctUsedVertices = 0;
  {FOREACHINDYNAMICARRAY(mm_amvVertices, CMipVertex, itmvtx) {
    if (itmvtx->mv_bUsed) {
      ctUsedVertices++;
    }
  }}

  // create a new array with as much vertices as we have counted in last pass
  CDynamicArray<CMipVertex> amvxNew;
  CMipVertex *pmvxUsed = amvxNew.New(ctUsedVertices);

  // for each vertex
  {FOREACHINDYNAMICARRAY(mm_amvVertices, CMipVertex, itmvtx) {
    // if it is used
    if (itmvtx->mv_bUsed) {
      // copy it to new array
      *pmvxUsed = itmvtx.Current();
      // set its remap pointer into new array
      itmvtx->mv_pmvxRemap = pmvxUsed;
      pmvxUsed++;
    // if it is not used
    } else {
      // clear its remap pointer (for debugging)
      #ifndef NDEBUG
      itmvtx->mv_pmvxRemap = NULL;
      #endif
    }
  }}

  // for each polygon
  {FOREACHINDYNAMICARRAY(mm_ampPolygons, CMipPolygon, itpo) {
    // for each polygon vertex in polygon
    CMipPolygonVertex *pmpvCurrentInPolygon = itpo->mp_pmpvFirstPolygonVertex;
    do
    {
      // remap pointer to vertex
      pmpvCurrentInPolygon->mpv_pmvVertex = pmpvCurrentInPolygon->mpv_pmvVertex->mv_pmvxRemap;
      pmpvCurrentInPolygon = pmpvCurrentInPolygon->mpv_pmpvNextInPolygon;
    }
    while( pmpvCurrentInPolygon != itpo->mp_pmpvFirstPolygonVertex);
  }}

  // use new array of vertices instead of the old one
  mm_amvVertices.Clear();
  mm_amvVertices.MoveArray(amvxNew);
}

BOOL CMipModel::CreateMipModel_t(INDEX ctVerticesToRemove, INDEX iSurfacePreservingFactor)
{
  if( ctVerticesToRemove>mm_amvVertices.Count()) return FALSE;

  for( INDEX ctRemoved = 0; ctRemoved<ctVerticesToRemove; ctRemoved++)
  {
    INDEX ctVerticesWithOneSurface = FindSurfacesForVertices();

    // setup flag for preserving surfaces
    _bPreserveSurfaces = TRUE;
    if( (ctVerticesWithOneSurface == 0) ||
        (( ((FLOAT)ctVerticesWithOneSurface) / mm_amvVertices.Count())*100 <=
        (100-iSurfacePreservingFactor)) )
    {
      _bPreserveSurfaces = FALSE;
    }

    CMipVertex *pmvBestSource, *pmvBestTarget;
    FindBestVertexPair( pmvBestSource, pmvBestTarget);
    JoinVertexPair( pmvBestSource, pmvBestTarget);
    RemoveUnusedVertices();
    if( mm_amvVertices.Count() == 0) return FALSE;
  }
  return TRUE;
}
