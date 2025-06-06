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
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Rendering/Render.h>
#include <Engine/Rendering/Render_internal.h>
#include <Engine/Templates/Selection.cpp>
#include <Engine/Models/ModelObject.h>

static PIX _pixSizeI;
static PIX _pixSizeJ;
UBYTE *_pubLassoBuffer = NULL;

// init select-on-render functionality
void InitSelectOnRender(PIX pixSizeI, PIX pixSizeJ)
{
  _pixSizeI = pixSizeI;
  _pixSizeJ = pixSizeJ;

  // if entity selecting not required
  if (_pselenSelectOnRender==NULL)
  {
    // if vertex selecting not requred
    if (_pselbvxtSelectOnRender==NULL) {
      // do nothing
      return;
    }
    // if not lasso and not alternative
    else if (_pavpixSelectLasso==NULL && !_bSelectAlternative) {
      // deselect all vertices
      _pselbvxtSelectOnRender->Clear();
    }
  }

  // if lasso selection is not on, or is invalid
  if (_pavpixSelectLasso==NULL || _pavpixSelectLasso->Count()<3
    || pixSizeI<2 || pixSizeJ<2) {
    // do nothing
    return;
  }

  SLONG slMaxOffset = pixSizeI*pixSizeJ-1;

  // allocate lasso buffer
  _pubLassoBuffer = (UBYTE*)AllocMemory(_pixSizeI*_pixSizeJ);
  // clear it
  memset(_pubLassoBuffer, 0, _pixSizeI*_pixSizeJ);

  // for each line in lasso
  INDEX ctpt = _pavpixSelectLasso->Count();
  for (INDEX ipt=0; ipt<ctpt; ipt++) {
    // get its points
    INDEX ipt0 = ipt;
    INDEX ipt1 = (ipt+1)%ctpt;
    // get their coordinates
    PIX pixI0 = (*_pavpixSelectLasso)[ipt0](1);
    PIX pixJ0 = (*_pavpixSelectLasso)[ipt0](2);
    PIX pixI1 = (*_pavpixSelectLasso)[ipt1](1);
    PIX pixJ1 = (*_pavpixSelectLasso)[ipt1](2);
    // if any of the coordinates is outside of window
    if (pixI0<0 || pixI0>=_pixSizeI || pixI1<0 || pixI1>=_pixSizeI ||
      pixJ0<0 || pixJ0>=_pixSizeJ || pixJ1<0 || pixJ1>=_pixSizeJ) {
      // skip this line
      continue;
    }

    // if line is horizontal
    if (pixJ0==pixJ1) {
      // skip it
      continue;
    // if line goes upwards
    } else if (pixJ0>pixJ1) {
      // make it go downwards
      Swap(pixI0, pixI1);
      Swap(pixJ0, pixJ1);
    }

    // calculate step
    FIX16_16 xStep = FIX16_16(FLOAT(pixI1-pixI0)/(pixJ1-pixJ0));
    // start in first row
    FIX16_16 xI = FIX16_16(pixI0);
    
    // for each row
    for(PIX pixJ = pixJ0; pixJ<pixJ1; pixJ++) {
      // get offset
      SLONG slOffset = pixJ*_pixSizeI+PIX(xI);
      // if offset is valid
      if (slOffset>=0 && slOffset<=slMaxOffset) {
        // invert the pixel in that row
        _pubLassoBuffer[slOffset] ^= 0xFF;
      }
      // step the line
      xI+=xStep;
    }
  }

  // for each row in lasso buffer
  for(PIX pixJ = 0; pixJ<_pixSizeJ; pixJ++) {
    // for each pixel in the row, except the last one
    UBYTE *pub = _pubLassoBuffer+pixJ*_pixSizeI;
    for(PIX pixI = 0; pixI<_pixSizeI-1; pixI++) {
      // xor it to the next one
      pub[1]^=pub[0];
      pub++;
    }
  }
}

// end select-on-render functionality
void EndSelectOnRender(void)
{
  // free lasso buffer
  if (_pubLassoBuffer!=NULL) {
    FreeMemory(_pubLassoBuffer);
    _pubLassoBuffer = NULL;
  }
}

// check if a vertex is influenced by current select-on-render selection
void SelectVertexOnRender(CBrushVertex &bvx, const PIX2D &vpix)
{
  // if not selecting
  if (_pselbvxtSelectOnRender==NULL) {
    // do nothing
    return;
  }

  // if the vertex is out of screen
  if (vpix(1)<0 || vpix(1)>=_pixSizeI
     || vpix(2)<0 || vpix(2)>=_pixSizeJ) {
    // do nothing
    return;
  }

  // if selecting without lasso
  if (_pubLassoBuffer==NULL) {
    // if vertex is near point
    if ((vpix-_vpixSelectNearPoint).Length()<_pixDeltaAroundVertex) {

      // if selected
      if (bvx.IsSelected(BVXF_SELECTED)) {
        // deselect it
        _pselbvxtSelectOnRender->Deselect(bvx);
      // if not selected
      } else {
        // select it
        _pselbvxtSelectOnRender->Select(bvx);
      }
    }
  // if selecting with lasso
  } else {
    // if the vertex is set in lasso buffer
    if (_pubLassoBuffer!=NULL
      &&_pubLassoBuffer[vpix(2)*_pixSizeI+vpix(1)]) {

      // if alternative
      if (_bSelectAlternative) {
        // deselect
        if (bvx.IsSelected(BVXF_SELECTED)) {
          _pselbvxtSelectOnRender->Deselect(bvx);
        }
      // if normal
      } else {
        // select
        if (!bvx.IsSelected(BVXF_SELECTED)) {
          _pselbvxtSelectOnRender->Select(bvx);
        }
      }
    }
  }
}

// check if given vertice is selected by lasso
BOOL IsVertexInLasso( CProjection3D &prProjection, const FLOAT3D &vtx, FLOATmatrix3D *pmR, FLOAT3D &vOffset)
{
  // convert from relative to absolute space
  FLOAT3D vAbsolute = vOffset+vtx*(*pmR);
  
  FLOAT3D vtxProjected;
  prProjection.ProjectCoordinate( vAbsolute, vtxProjected);

  PIX2D vpix;
  vpix(1) = (PIX) vtxProjected(1);
  // convert coordinate into screen representation
  vpix(2) = (PIX) (_pixSizeJ-vtxProjected(2));

  // if the vertex is out of screen
  if (vpix(1)<0 || vpix(1)>=_pixSizeI
     || vpix(2)<0 || vpix(2)>=_pixSizeJ) {
    // no selecting
    return FALSE;
  }

  // if the vertex is set in lasso buffer
  if (_pubLassoBuffer!=NULL
    &&_pubLassoBuffer[vpix(2)*_pixSizeI+vpix(1)])
  {
    return TRUE;
  }
  return FALSE;
}

// check if given bounding box is selected by lasso
BOOL IsBoundingBoxInLasso( CProjection3D &prProjection, const FLOATaabbox3D &box, FLOATmatrix3D *pmR, FLOAT3D &vOffset)
{
  FLOAT3D vMin = box.Min();
  FLOAT3D vMax = box.Max();

  // test lasso influence for all of bounding box's vertices
  if(
    IsVertexInLasso( prProjection, FLOAT3D( vMin(1), vMin(2), vMin(3)), pmR, vOffset) &&
    IsVertexInLasso( prProjection, FLOAT3D( vMax(1), vMin(2), vMin(3)), pmR, vOffset) &&
    IsVertexInLasso( prProjection, FLOAT3D( vMin(1), vMax(2), vMin(3)), pmR, vOffset) &&
    IsVertexInLasso( prProjection, FLOAT3D( vMax(1), vMax(2), vMin(3)), pmR, vOffset) &&
    IsVertexInLasso( prProjection, FLOAT3D( vMin(1), vMin(2), vMax(3)), pmR, vOffset) &&
    IsVertexInLasso( prProjection, FLOAT3D( vMax(1), vMin(2), vMax(3)), pmR, vOffset) &&
    IsVertexInLasso( prProjection, FLOAT3D( vMin(1), vMax(2), vMax(3)), pmR, vOffset) &&
    IsVertexInLasso( prProjection, FLOAT3D( vMax(1), vMax(2), vMax(3)), pmR, vOffset) )
  {
    return TRUE;
  }
  return FALSE;
}

// check if all of the corners of entity's bounding box are influenced by current select-on-render selection
void SelectEntityOnRender(CProjection3D &prProjection, CEntity &en)
{
  FLOATaabbox3D bbox;

  FLOATmatrix3D mOne = FLOATmatrix3D(0.0f);
  mOne.Diagonal(1.0f);
  FLOATmatrix3D *pmR;
  FLOAT3D vOffset;

  // if this entity is model
  if (en.en_RenderType==CEntity::RT_MODEL || en.en_RenderType==CEntity::RT_EDITORMODEL)
  {
    // get bbox of current frame
    CModelObject *pmo = en.GetModelObject();
    pmo->GetCurrentFrameBBox( bbox);
    pmR = &en.en_mRotation;
    vOffset = en.GetPlacement().pl_PositionVector;
  }
  // if it is ska model
  else if(en.en_RenderType==CEntity::RT_SKAMODEL || en.en_RenderType==CEntity::RT_SKAEDITORMODEL)
  {
    en.GetModelInstance()->GetCurrentColisionBox( bbox);
    pmR = &en.en_mRotation;
    vOffset = en.GetPlacement().pl_PositionVector;
  }
  // if it is brush
  else
  {
    // get bbox of brush's first mip
    CBrush3D *pbr = en.GetBrush();
    CBrushMip *pbrmip = pbr->GetFirstMip();
    bbox = pbrmip->bm_boxBoundingBox;
    pmR = &mOne;
    vOffset = FLOAT3D( 0.0f, 0.0f, 0.0f);
  }

  if( IsBoundingBoxInLasso( prProjection, bbox, pmR, vOffset))
  {
    if( _bSelectAlternative)
    {
      // deselect
      if (en.IsSelected(ENF_SELECTED))
      {
        _pselenSelectOnRender->Deselect(en);
      }
    }
    else
    {
      // select
      if (!en.IsSelected(ENF_SELECTED))
      {
        _pselenSelectOnRender->Select(en);
      }
    }
  }
}
