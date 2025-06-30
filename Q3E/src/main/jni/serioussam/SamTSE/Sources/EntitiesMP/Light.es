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

200
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/ModelDestruction";
uses "EntitiesMP/AnimationChanger";
               
enum LightType {
  0 LT_POINT          "Point light",
  1 LT_AMBIENT        "Ambient light",
  2 LT_STRONG_AMBIENT "Strong ambient light",
  3 LT_DIRECTIONAL    "Directional light",
  4 LT_STRONG_POINT   "Strong point light",
};

enum LensFlareType {                
  0  LFT_NONE                         "None",
  1  LFT_STANDARD                     "Standard",
  2  LFT_STANDARD_REFLECTIONS         "Standard reflections",
  3  LFT_YELLOW_STAR_RED_RING         "Yellow star with red ring",
  4  LFT_WHITE_GLOW_STAR_RED_RING     "White glow star with red ring",
  5  LFT_WHITE_GLOW_STAR              "White glow star",
  6  LFT_WHITE_STAR_RED_RING_STREAKS  "White star with red ring and streaks",
  7  LFT_WHITE_STAR_RED_REFLECTIONS   "White star with red reflections",
  8  LFT_BLUE_STAR_BLUE_REFLECTIONS   "Blue star with blue reflections",
  9  LFT_PV_SPACE_SHIP_WINDOW_FLARE   "Pyramid valley space ship window flare",
  10 LFT_WHITE_GLOW_STAR_RED_RING_FAR "Yellow star with red ring far",
  11 LFT_WHITE_GLOW_FAR               "White glow far",
  12 LFT_WHITE_GLOW_STAR_NG           "White glow star no glare",
};

%{

void CLight_OnInitClass(void)
{
  // init lens flares effects
  InitLensFlares();
}

void CLight_OnEndClass(void)
{
  // close lens flares effects
  CloseLensFlares();
}

%}

class CLight : CEntity {
name      "Light";
thumbnail "Thumbnails\\Light.tbn";
features  "HasName", "HasDescription", "ImplementsOnInitClass", "ImplementsOnEndClass";
properties:
    2 COLOR m_colColor                    "Color" 'C' =C_GRAY,
    9 COLOR m_colAmbient                  "Directional ambient"  'E' =C_BLACK,
    1 RANGE m_rFallOffRange               "Fall-off" 'F' =8.0f features(EPROPF_HIDEINPERSPECTIVE),
    4 RANGE m_rHotSpotRange               "Hot-spot" 'H' =0.0f features(EPROPF_HIDEINPERSPECTIVE),
    7 ILLUMINATIONTYPE m_itIllumination   "Polygon illumination" 'I' =0,
    8 enum LightType m_ltType             "Type" 'Y' =LT_POINT,
   10 CTString m_strDescription = "",
   11 CTString m_strName                  "Name" 'N' = "Light",
   12 BOOL m_bDarkLight                   "Dark light" 'A' = FALSE,
   13 FLOAT m_fNearClip                   "Clip near" = 0.1f,
   14 FLOAT m_fFarClip                    "Clip far" = 0.01f,
   15 BOOL m_bSubstractSectorAmbient      "Substract sector ambient" 'S' = FALSE,
   16 BOOL m_bRenderAsSmallLight          "Render as small" 'R' = FALSE,
   17 enum LensFlareType m_lftLensFlare   "Lens flare" 'L' = LFT_NONE,
   18 BOOL m_bBackground                  "Background" 'B' = FALSE,
   19 BOOL m_bLensFlareOnly               "Lens flare only" 'O' = FALSE,
   20 CTFileName m_fnmLightAnimation      "Light animation file" = CTString(""),
   21 ANIMATION m_iLightAnimation         "Light animation" = 0,
   25 FLOAT m_tmOffsetPhase               "Light animation offset" = 0.0f,
   22 CAnimObject m_aoLightAnimation,
   24 BOOL m_bTargetable                  "Targetable" = FALSE,
   26 BOOL m_bDynamic                     "Dynamic" = FALSE,
   27 BOOL m_bDiffusion                   "Diffusion" 'D' = TRUE,
   30 CTFileName m_fnmAmbientLightAnimation      "Ambient light animation file" = CTString(""),
   31 ANIMATION m_iAmbientLightAnimation         "Ambient light animation" = 0,
   32 CAnimObject m_aoAmbientLightAnimation,
{
  CLightSource m_lsLightSource;
  CBoolDefaultFalse m_bdfInitialized; // set if already initialized once
}
components:
    1 model   MODEL_LIGHT_SOURCE         "Models\\Editor\\LightSource.mdl",
    2 texture TEXTURE_POINT_LIGHT        "Models\\Editor\\PointLight.tex",
    3 texture TEXTURE_AMBIENT_LIGHT      "Models\\Editor\\AmbientLight.tex",
    4 texture TEXTURE_REAL_AMBIENT_LIGHT "Models\\Editor\\RealAmbientLight.tex",
    5 model   MODEL_SPOT_LIGHT           "Models\\Editor\\SpotLight.mdl",
    6 texture TEXTURE_SPOT_LIGHT         "Models\\Editor\\SpotLight.tex",

functions:
  /* Get anim data for given animation property - return NULL for none. */
  CAnimData *GetAnimData(SLONG slPropertyOffset) 
  {
    if (slPropertyOffset==_offsetof(CLight, m_iLightAnimation))
    {
      return m_aoLightAnimation.GetData();
    }
    else if (slPropertyOffset==_offsetof(CLight, m_iAmbientLightAnimation)) 
    {
      return m_aoAmbientLightAnimation.GetData();
    }
    else
    {
      return CEntity::GetAnimData(slPropertyOffset);
    }
  };

  BOOL IsTargetable(void) const
  {
    return m_bTargetable;
  }

  BOOL IsImportant(void) const
  {
    return(m_ltType==LT_DIRECTIONAL);
  }

  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    // when someone in range is destroyed
    if (ee.ee_slEvent==EVENTCODE_ERangeModelDestruction)
    {
      // fade out completely
      m_colColor = C_BLACK;
      m_colAmbient = C_BLACK;
      CLightSource lsNew;
      SetupLightSource(lsNew);
      m_lsLightSource.SetLightSource(lsNew);
      return TRUE;
    // when animation should be changed
    }
    else if (ee.ee_slEvent==EVENTCODE_EChangeAnim)
    {
      EChangeAnim &eChange = (EChangeAnim &)ee;

      // for diffuse component of light
      m_iLightAnimation = eChange.iLightAnim;
      if (m_aoLightAnimation.GetData()!=NULL)
      {
        m_aoLightAnimation.PlayAnim(m_iLightAnimation, eChange.bLightLoop?AOF_LOOPING:0);
      }
      
      // for ambient component of light
      m_iAmbientLightAnimation = eChange.iAmbientLightAnim;
      if (m_aoAmbientLightAnimation.GetData()!=NULL)
      {
        m_aoAmbientLightAnimation.PlayAnim(m_iAmbientLightAnimation, eChange.bAmbientLightLoop?AOF_LOOPING:0);
      }

      // if neither ambient nor difuse animation is set, apply direct color change
      if( (m_aoLightAnimation.GetData()==NULL) && (m_aoAmbientLightAnimation.GetData()==NULL) )
      {
        m_colColor = eChange.colDiffuse;
        m_colAmbient = eChange.colAmbient;
        CLightSource lsNew;
        SetupLightSource(lsNew);
        m_lsLightSource.SetLightSource(lsNew);
        return TRUE;
      }
    }
    return FALSE;
  }

  // apply mirror and stretch to the entity
  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    // stretch its ranges
    m_rFallOffRange*=fStretch;
    m_rHotSpotRange*=fStretch;
    m_fNearClip    *=fStretch;
    m_fFarClip     *=fStretch;
    //(void)bMirrorX;  // no mirror for lights
  }

  /* Get static light source information. */
  CLightSource *GetLightSource(void)
  {
    // if never initialized before (happens after loading)
    if (!m_bdfInitialized.bdf_bValue) {
      // initialize now
      CLightSource lsNew;
      SetupLightSource(lsNew);
      m_lsLightSource.SetLightSourceWithNoDiscarding(lsNew);
      m_bdfInitialized.bdf_bValue = TRUE;
    }
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  }

  // prepare and transfer light source variables
  void SetupLightSource( CLightSource &lsNew)
  {
    switch( m_ltType) {
    case LT_POINT:
    case LT_STRONG_POINT:
      lsNew.ls_ulFlags = LSF_CASTSHADOWS;
      break;
    case LT_DIRECTIONAL:
      lsNew.ls_ulFlags = LSF_DIRECTIONAL|LSF_CASTSHADOWS;
      break;
    case LT_STRONG_AMBIENT:
    case LT_AMBIENT:
      lsNew.ls_ulFlags = 0;
      break;
    }
    if( m_bSubstractSectorAmbient) { lsNew.ls_ulFlags |= LSF_SUBSTRACTSECTORAMBIENT;  }
    if( m_bLensFlareOnly)          { lsNew.ls_ulFlags |= LSF_LENSFLAREONLY; }
    if( m_bDynamic)                { lsNew.ls_ulFlags |= LSF_DYNAMIC; }
    // directional light cannot be dark
    if( m_bDarkLight) {
      if( m_ltType==LT_DIRECTIONAL) {
        lsNew.ls_ulFlags &= ~LSF_DARKLIGHT;
        m_bDarkLight = FALSE;
      } else {
        lsNew.ls_ulFlags |= LSF_DARKLIGHT;
      }
    }
    // ambient and directional lights doesn't support diffusion
    if( m_bDiffusion) { 
      if( m_bDynamic || m_ltType==LT_AMBIENT || m_ltType==LT_STRONG_AMBIENT) {
        lsNew.ls_ulFlags &= ~LSF_DIFFUSION;
        m_bDiffusion = FALSE;
      } else {
        lsNew.ls_ulFlags |= LSF_DIFFUSION;
      }
    }

    lsNew.ls_rHotSpot = m_rHotSpotRange;
    lsNew.ls_rFallOff = m_rFallOffRange;
    lsNew.ls_fNearClipDistance = m_fNearClip;
    lsNew.ls_fFarClipDistance  = m_fFarClip;
    // hot spot for strong lights is 90% of light range
    if( m_ltType == LT_STRONG_AMBIENT || m_ltType == LT_STRONG_POINT) {
      lsNew.ls_rHotSpot = lsNew.ls_rFallOff*0.9f;
    }

    lsNew.ls_colColor   = m_colColor;
    lsNew.ls_colAmbient = C_BLACK; // only directional lights are allowed to have ambient component
    if( lsNew.ls_ulFlags&LSF_DIRECTIONAL) { lsNew.ls_colAmbient = m_colAmbient; }
    lsNew.ls_ubPolygonalMask = (UBYTE) m_itIllumination;

    switch(m_lftLensFlare)
    {
      case LFT_NONE:
        lsNew.ls_plftLensFlare = NULL;
        break;    
      case LFT_STANDARD:
        lsNew.ls_plftLensFlare = &_lftStandard;                   
        break;
      case LFT_STANDARD_REFLECTIONS:
        lsNew.ls_plftLensFlare = &_lftStandardReflections;        
        break;
      case LFT_YELLOW_STAR_RED_RING:
        lsNew.ls_plftLensFlare = &_lftYellowStarRedRing;          
        break;
      case LFT_WHITE_GLOW_STAR_RED_RING:
        lsNew.ls_plftLensFlare = &_lftWhiteGlowStarRedRing;       
        break;
      case LFT_WHITE_GLOW_STAR:
        lsNew.ls_plftLensFlare = &_lftWhiteGlowStar;              
        break;
      case LFT_WHITE_STAR_RED_RING_STREAKS:
        lsNew.ls_plftLensFlare = &_lftWhiteStarRedRingStreaks;    
        break;
      case LFT_WHITE_STAR_RED_REFLECTIONS:
        lsNew.ls_plftLensFlare = &_lftWhiteStarRedReflections;       
        break;
      case LFT_BLUE_STAR_BLUE_REFLECTIONS:
        lsNew.ls_plftLensFlare = &_lftBlueStarBlueReflections;       
        break;
      case LFT_PV_SPACE_SHIP_WINDOW_FLARE:
        lsNew.ls_plftLensFlare = &_lftPVSpaceShipWindowFlare;
        break;
      case LFT_WHITE_GLOW_STAR_RED_RING_FAR:
        lsNew.ls_plftLensFlare = &_lftYellowStarRedRingFar;
        break;
      case LFT_WHITE_GLOW_FAR:
        lsNew.ls_plftLensFlare = &_lftWhiteGlowFar;
        break;
      case LFT_WHITE_GLOW_STAR_NG:
        lsNew.ls_plftLensFlare = &_lftWhiteGlowStarNG;              
        break;
    }

    // --------- Setup light animations
    // diffuse
    lsNew.ls_paoLightAnimation = NULL;
    if (m_aoLightAnimation.GetData()!=NULL) {
      lsNew.ls_paoLightAnimation = &m_aoLightAnimation;
    }

    // ambient
    lsNew.ls_paoAmbientLightAnimation = NULL;
    if (m_aoAmbientLightAnimation.GetData()!=NULL) {
      lsNew.ls_paoAmbientLightAnimation = &m_aoAmbientLightAnimation;
    }
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CLight) - sizeof(CEntity) + CEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strDescription.Length();
    slUsedMemory += m_strName.Length();
    slUsedMemory += m_fnmLightAnimation.Length();
    slUsedMemory += m_fnmAmbientLightAnimation.Length();
    slUsedMemory += 2* sizeof(CAnimObject); // 2 of them
    return slUsedMemory;
  }


procedures:
  Main()
  {
    // fall off and hot spot must be positive values
    if (m_rFallOffRange<0) {
      m_rFallOffRange = 0.0f;
    }
    if (m_rHotSpotRange<0) {
      m_rHotSpotRange = 0.0f;
    }
    // hot spot must be less or equal falloff
    if (m_rHotSpotRange>m_rFallOffRange) {
      m_rHotSpotRange = m_rFallOffRange;
    }

    // near clip must not be too small relatively to falloff 
    // (crashes on shadow rendering otherwise!)
    //if (m_fNearClip<m_rFallOffRange*1E-4f) {
    //  m_fNearClip = m_rFallOffRange*1E-4f;
    //}

    // near clip must not be too small
    if (m_fNearClip<=0.01f) {
      m_fNearClip = 0.01f;
    }
    // far clip distance must be positive values
    if (m_fFarClip<=0) {
      m_fFarClip = 0.0f;//0.01f;
    }

    // only directional lights are allowed to have ambient component
    if( m_ltType!=LT_DIRECTIONAL) {
      m_colAmbient = C_BLACK;
    }

    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set model stretch -- MUST BE DONE BEFORE SETTING MODEL!
    if( m_bRenderAsSmallLight)
    {
      GetModelObject()->mo_Stretch = FLOAT3D( 0.25f, 0.25f, 0.25f);
    }
    else
    {
      // set stretch factor of the light (directional lights don't have stretch)
      if ( m_ltType != LT_DIRECTIONAL) {
        const float LIGHT_MINSIZE=0.5f;
        FLOAT fFactor = Log2(m_rFallOffRange)*LIGHT_MINSIZE;
        if (fFactor<LIGHT_MINSIZE) {
          fFactor=LIGHT_MINSIZE;
        }
        GetModelObject()->mo_Stretch = FLOAT3D( fFactor, fFactor, fFactor);
      }
    }

    CTString strType;
    if( m_ltType == LT_POINT || m_ltType == LT_STRONG_POINT)
    {
      strType = "point";
      // set model to light source
      SetModel(MODEL_LIGHT_SOURCE);
      // set texture of point light model
      SetModelMainTexture(TEXTURE_POINT_LIGHT);
    }
    // initialize ambient light
    else if( m_ltType == LT_AMBIENT)
    {
      strType = "ambient";
      // set model to light source
      SetModel(MODEL_LIGHT_SOURCE);
      // set texture of ambient light model
      SetModelMainTexture(TEXTURE_AMBIENT_LIGHT);
    }
    // initialize real ambient light
    else if( m_ltType == LT_STRONG_AMBIENT)
    {
      strType = "ambient";
      // set model to light source
      SetModel(MODEL_LIGHT_SOURCE);
      // set texture of real ambient light model
      SetModelMainTexture(TEXTURE_REAL_AMBIENT_LIGHT);
    }
    // initialize spot light
    else if( m_ltType == LT_DIRECTIONAL)
    {
      strType = "directional";
      // set model to spot light
      SetModel(MODEL_SPOT_LIGHT);
      // set texture of spot light model
      SetModelMainTexture(TEXTURE_SPOT_LIGHT);
    }

    if( m_bDarkLight)
    {
      strType = strType+", dark";
    }

    // set diffuse light animation if available
    try {
      m_aoLightAnimation.SetData_t(m_fnmLightAnimation);
    } catch (const char *strError) {
      WarningMessage(TRANS("Cannot load '%s': %s"), (const char *) (CTString&)m_fnmLightAnimation, strError);
      m_fnmLightAnimation = "";
    }
    if (m_aoLightAnimation.GetData()!=NULL) {
      m_aoLightAnimation.PlayAnim(m_iLightAnimation, AOF_LOOPING);
      m_aoLightAnimation.OffsetPhase(m_tmOffsetPhase*m_aoLightAnimation.GetCurrentAnimLength());
    }

    // set ambient light animation if available
    try {
      m_aoAmbientLightAnimation.SetData_t(m_fnmAmbientLightAnimation);
    } catch (const char *strError) {
      WarningMessage(TRANS("Cannot load '%s': %s"), (const char *) (CTString&)m_fnmAmbientLightAnimation, strError);
      m_fnmAmbientLightAnimation = "";
    }
    if (m_aoAmbientLightAnimation.GetData()!=NULL) {
      m_aoAmbientLightAnimation.PlayAnim(m_iAmbientLightAnimation, AOF_LOOPING);
      m_aoAmbientLightAnimation.OffsetPhase(m_tmOffsetPhase*m_aoAmbientLightAnimation.GetCurrentAnimLength());
    }

    // set a new light source with light properties
    CLightSource lsNew;
    SetupLightSource(lsNew);

    // setup background rendering flag
    if (m_bBackground) {
      SetFlags(GetFlags()|ENF_BACKGROUND);
    } else {
      SetFlags(GetFlags()&~ENF_BACKGROUND);
    }

    // set up this light source from the new properties
    m_lsLightSource.ls_penEntity = this;
    if (!m_bdfInitialized.bdf_bValue) {
      m_lsLightSource.SetLightSourceWithNoDiscarding(lsNew);
      m_bdfInitialized.bdf_bValue = TRUE;
    } else {
      m_lsLightSource.SetLightSource(lsNew);
    }

    m_strDescription.PrintF("%s:%g-%g", 
      (const char *) strType,  m_rHotSpotRange, m_rFallOffRange);

    return;
  };
};
