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

#ifndef SE_INCL_RENDERMODEL_H
#define SE_INCL_RENDERMODEL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Placement.h>
#include <Engine/Math/Projection.h>

/*
 *	This instance of render prefs object represents global preferences
 *	used for rendering of all models and their shadows
 */
ENGINE_API extern class CModelRenderPrefs _mrpModelRenderPrefs;

/*
 *	This object is used to define how models and their shadows will be rendered
 */
class ENGINE_API CModelRenderPrefs
{
private:
	BOOL rp_BBoxFrameVisible;                       // determines visibility of frame BBox
	BOOL rp_BBoxAllVisible;                         // determines visibility of all frames BBox
  COLOR rp_InkColor;                              // ink color (wire frame)
  COLOR rp_PaperColor;                            // paper color
  ULONG rp_RenderType;								            // model's rendering type
	INDEX rp_ShadowQuality;								          // model shadow's quality (substraction to mip model index)
public:
  CModelRenderPrefs();                            // constructor sets defult values
  void SetRenderType(ULONG rtNew);      // set model rendering type
  void SetTextureType(ULONG rtNew);     // set model rendering texture type
  void SetShadingType(ULONG rtNew);     // set model shading texture type
  void SetShadowQuality(INDEX iNew);    // set shadow quality (best 0, worse -1, ...)
  void DesreaseShadowQuality(void);               // decrease shadow quality
  void IncreaseShadowQuality(void);               // increase shadow quality
  void SetWire(BOOL bWireOn);                     // set wire frame on/off
  void SetHiddenLines(BOOL bHiddenLinesOn);       // set hiden lines on/off
  BOOL BBoxFrameVisible();                        // bounding box frames visible?
  BOOL BBoxAllVisible();                          // bounding box all frames visible?
  BOOL WireOn(void);                              // returns TRUE if wire frame is on
  BOOL HiddenLines(void);                         // returns TRUE if hiden lines are visible
  void SetInkColor(COLOR clrNew);                 // set ink color
  COLOR GetInkColor();                            // get ink color
  void SetPaperColor(COLOR clrNew);               // set paper color
  COLOR GetPaperColor();                          // get paper color
  void BBoxFrameShow(BOOL bShow);                 // show bounding box frame
  void BBoxAllShow(BOOL bShow);                   // show bounding box all frames
  ULONG GetRenderType(void);                      // get model rendering type
  INDEX GetShadowQuality(void);                   // retrieves current shadow quality level
	void Read_t( CTStream *istrFile);   // throw char * // read and
	void Write_t( CTStream *ostrFile);  // throw char * // write functions
};

// texture used for simple model shadows
extern ENGINE_API CTextureObject _toSimpleModelShadow;

// begin/end model rendering to screen
extern ENGINE_API void BeginModelRenderingView(
  CAnyProjection3D &prProjection, CDrawPort *pdp);
extern ENGINE_API void EndModelRenderingView( BOOL bRestoreOrtho=TRUE);

// begin/end model rendering to shadow mask
extern ENGINE_API void BeginModelRenderingMask(
  CAnyProjection3D &prProjection, UBYTE *pubMask, SLONG slMaskWidth, SLONG slMaskHeight);
extern ENGINE_API void EndModelRenderingMask(void);

#define RMF_ATTACHMENT          (1UL<<0)    // set for attachment render models
#define RMF_FOG                 (1UL<<1)    // render in fog
#define RMF_HAZE                (1UL<<2)    // render in haze
#define RMF_SPECTATOR           (1UL<<3)    // model will not be rendered but shadows might
#define RMF_INVERTED            (1UL<<4)    // stretch is inverted
#define RMF_BBOXSET             (1UL<<5)    // bounding box has been calculated
#define RMF_INSIDE              (1UL<<6)    // doesn't need clipping to frustum
#define RMF_INMIRROR            (1UL<<7)    // doesn't need clipping to mirror/warp plane
#define RMF_WEAPON              (1UL<<8)    // TEMP: weapon model is rendering so don't use ATI's Truform!

class ENGINE_API CRenderModel {
public:
// implementation:
  CModelData *rm_pmdModelData;        // model's data
  struct ModelMipInfo *rm_pmmiMip;    // current mip
  ULONG rm_rtRenderType;              // current rendering preferences
  // lerp information
  INDEX rm_iFrame0, rm_iFrame1;       
  FLOAT rm_fRatio;
  INDEX rm_iMipLevel;
  INDEX rm_iTesselationLevel;
  union { struct ModelFrameVertex8  *rm_pFrame8_0;    // ptr to last frame
          struct ModelFrameVertex16 *rm_pFrame16_0; };
  union { struct ModelFrameVertex8  *rm_pFrame8_1;    // ptr to next frame
          struct ModelFrameVertex16 *rm_pFrame16_1; };
  FLOAT3D rm_vLightObj;   // light vector as seen from object space
  // placement of the object
  FLOAT3D rm_vObjectPosition;
  FLOATmatrix3D rm_mObjectRotation;
  // object to view placement
  FLOAT3D rm_vObjectToView;
  FLOATmatrix3D rm_mObjectToView;
  // decompression and stretch factors
  FLOAT3D rm_vStretch, rm_vOffset;
  // bounding box min/max coords in object space
  FLOAT3D rm_vObjectMinBB, rm_vObjectMaxBB;
  // flags and blend color global for this rendering
  ULONG rm_ulFlags;
  COLOR rm_colBlend;

  // set modelview matrix if not already set
  void SetModelView(void);
// interface:
public:
  CRenderModel(void);
  ~CRenderModel(void);
  // set placement of the object
  void SetObjectPlacement(const CPlacement3D &pl);
  void SetObjectPlacement(const FLOAT3D &v, const FLOATmatrix3D &m);

  FLOAT rm_fDistanceFactor;           // mip factor not including scaling or biasing
  FLOAT rm_fMipFactor;                // real mip factor 
  FLOAT3D rm_vLightDirection;         // direction shading light
  COLOR rm_colLight;                  // color of the shading light
  COLOR rm_colAmbient;                // color of the ambient
};


#endif  /* include-once check. */

