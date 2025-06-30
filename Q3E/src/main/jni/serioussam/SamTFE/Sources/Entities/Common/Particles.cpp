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

#include "../StdH/StdH.h"
#include "Entities/BloodSpray.h"
#include "Entities/PlayerWeapons.h"
#include "Entities/WorldSettingsController.h"
#include "Entities/BackgroundViewer.h"

static CTextureObject _toRomboidTrail;
static CTextureObject _toBombTrail;
static CTextureObject _toFirecrackerTrail;
static CTextureObject _toSpiralTrail;
static CTextureObject _toColoredStarsTrail;
static CTextureObject _toFireball01Trail;
static CTextureObject _toGrenadeTrail;
static CTextureObject _toCannonBall;
static CTextureObject _toRocketTrail;
static CTextureObject _toVerticalGradient;
static CTextureObject _toVerticalGradientAlpha;
static CTextureObject _toBlood01Trail;
static CTextureObject _toLavaTrailGradient;
static CTextureObject _toLavaTrailSmoke;
static CTextureObject _toFlamethrowerTrail;
static CTextureObject _toBoubble01;
static CTextureObject _toBoubble02;
static CTextureObject _toBoubble03;
static CTextureObject _toStar01;
static CTextureObject _toStar02;
static CTextureObject _toStar03;
static CTextureObject _toStar04;
static CTextureObject _toStar05;
static CTextureObject _toStar06;
static CTextureObject _toStar07;
static CTextureObject _toStar08;
static CTextureObject _toBlood;
static CTextureObject _toWaterfallGradient;
static CTextureObject _toGhostbusterBeam;
static CTextureObject _toLightning;
static CTextureObject _toSand;
static CTextureObject _toSandFlowGradient;
static CTextureObject _toWater;
static CTextureObject _toWaterFlowGradient;
static CTextureObject _toLava;
static CTextureObject _toLavaFlowGradient;
static CTextureObject _toBloodSprayTexture;
static CTextureObject _toFlowerSprayTexture;
static CTextureObject _toBonesSprayTexture;
static CTextureObject _toFeatherSprayTexture;
static CTextureObject _toStonesSprayTexture;
static CTextureObject _toLavaSprayTexture;
static CTextureObject _toBeastProjectileSprayTexture;
static CTextureObject _toLavaEruptingTexture;
static CTextureObject _toWoodSprayTexture;
static CTextureObject _toLavaBombTrailSmoke;
static CTextureObject _toLavaBombTrailGradient;
static CTextureObject _toElectricitySparks;
static CTextureObject _toBeastProjectileTrailTexture;
static CTextureObject _toBeastProjectileTrailGradient;
static CTextureObject _toBeastBigProjectileTrailTexture;
static CTextureObject _toBeastBigProjectileTrailGradient;
static CTextureObject _toBeastDebrisTrailGradient;
static CTextureObject _toBeastDebrisTrailTexture;
static CTextureObject _toRaindrop;
static CTextureObject _toSnowdrop;
static CTextureObject _toBulletStone;
static CTextureObject _toBulletSand;
static CTextureObject _toBulletSpark;
static CTextureObject _toBulletSmoke;
static CTextureObject _toBulletWater;
static CTextureObject _toPlayerParticles;
static CTextureObject _toWaterfallFoam;
static CTextureObject _toMetalSprayTexture;

// array for model vertices in absolute space
CStaticStackArray<FLOAT3D> avVertices;

#define CT_MAX_PARTICLES_TABLE 512

FLOAT afTimeOffsets[CT_MAX_PARTICLES_TABLE];
FLOAT afStarsPositions[CT_MAX_PARTICLES_TABLE][3];
UBYTE auStarsColors[CT_MAX_PARTICLES_TABLE][3];

void InitParticleTables(void);

// init particle effects
void InitParticles(void)
{
  try
  {
    _toRomboidTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Romboid.tex"));
    _toBombTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\WhiteBubble.tex"));
    _toFirecrackerTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\FireCracker.tex"));
    _toSpiralTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Smoke01.tex"));
    _toColoredStarsTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Star01.tex"));
    _toFireball01Trail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Fireball01.tex"));
    _toGrenadeTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Smoke02.tex"));
    _toCannonBall.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\CannonBall.tex"));
    _toRocketTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Smoke06.tex"));
    _toVerticalGradient.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\VerticalGradient.tex"));
    _toVerticalGradientAlpha.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\VerticalGradientAlpha.tex"));
    _toBlood01Trail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Blood02.tex"));
    _toLavaTrailGradient.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\LavaTrailGradient.tex"));
    _toLavaTrailSmoke.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\LavaTrailSmoke.tex"));
    _toFlamethrowerTrail.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\FlameThrower01.tex"));
    _toBoubble01.SetData_t(CTFILENAME("Models\\Items\\Particles\\Boubble01.tex"));
    _toBoubble02.SetData_t(CTFILENAME("Models\\Items\\Particles\\Boubble02.tex"));
    _toBoubble03.SetData_t(CTFILENAME("Models\\Items\\Particles\\Boubble03.tex"));
    _toStar01.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star01.tex"));
    _toStar02.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star02.tex"));
    _toStar03.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star03.tex"));
    _toStar04.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star04.tex"));
    _toStar05.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star05.tex"));
    _toStar06.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star06.tex"));
    _toStar07.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star07.tex"));
    _toStar08.SetData_t(CTFILENAME("Models\\Items\\Particles\\Star08.tex"));
    _toWaterfallGradient.SetData_t(CTFILENAME("Models\\Effects\\Heatmaps\\Waterfall08.tex"));
    _toGhostbusterBeam.SetData_t(CTFILENAME("Models\\Weapons\\GhostBuster\\Projectile\\Ray.tex"));
    _toLightning.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Lightning.tex"));
    _toSand.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Sand.tex"));
    _toSandFlowGradient.SetData_t(CTFILENAME("Models\\Effects\\Heatmaps\\SandFlow01.tex"));
    _toWater.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Water.tex"));
    _toWaterFlowGradient.SetData_t(CTFILENAME("Models\\Effects\\Heatmaps\\WaterFlow01.tex"));
    _toLava.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Lava.tex"));
    _toLavaFlowGradient.SetData_t(CTFILENAME("Models\\Effects\\Heatmaps\\LavaFlow01.tex"));
    _toBloodSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Blood03.tex"));
    _toFlowerSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Flowers.tex")); 
    _toBonesSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BonesSpill01.tex"));
    _toFeatherSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\FeatherSpill01.tex"));
    _toStonesSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\StonesSpill01.tex"));
    _toLavaSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\LavaSpill01.tex"));
    _toBeastProjectileSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BeastProjectileSpill.tex"));
    _toLavaEruptingTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\LavaErupting.tex"));
    _toWoodSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\WoodSpill01.tex"));
    _toLavaBombTrailSmoke.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\LavaBomb.tex"));
    _toLavaBombTrailGradient.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\LavaBombGradient.tex"));
    _toBeastDebrisTrailGradient.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BeastDebrisTrailGradient.tex"));
    _toBeastProjectileTrailTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BeastProjectileTrail.tex"));
    _toBeastProjectileTrailGradient.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BeastProjectileTrailGradient.tex"));
    _toBeastBigProjectileTrailTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BeastBigProjectileTrail.tex"));
    _toBeastBigProjectileTrailGradient.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BeastBigProjectileTrailGradient.tex"));
    _toBeastDebrisTrailTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BeastDebrisTrail.tex"));
    _toElectricitySparks.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\ElectricitySparks.tex"));
    _toRaindrop.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Raindrop.tex"));
    _toSnowdrop.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\Snowdrop.tex"));
    _toBulletStone.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BulletSpray.tex"));
    _toBulletWater.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BulletSprayWater.tex"));
    _toBulletSand.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BulletSpraySand.tex"));
    _toBulletSpark.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\BulletSpark.tex"));
    _toBulletSmoke.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\SmokeAnim01.tex"));
    _toPlayerParticles.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\PlayerParticles.tex"));
    _toWaterfallFoam.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\WaterfallFoam.tex"));
    _toMetalSprayTexture.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\MetalSpill.tex"));

    ((CTextureData*)_toLavaTrailGradient              .GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toLavaBombTrailGradient          .GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toBeastDebrisTrailGradient       .GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toBeastProjectileTrailGradient   .GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toBeastBigProjectileTrailGradient.GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toWaterfallGradient              .GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toSandFlowGradient               .GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toWaterFlowGradient              .GetData())->Force(TEX_STATIC);
    ((CTextureData*)_toLavaFlowGradient               .GetData())->Force(TEX_STATIC);
  }
  catch (const char *strError)
  {
    FatalError(TRANS("Unable to obtain texture: %s"), strError);
  }
  InitParticleTables();
}

// close particle effects
void CloseParticles(void)
{
  _toRomboidTrail.SetData(NULL);
  _toBombTrail.SetData(NULL);
  _toFirecrackerTrail.SetData(NULL);
  _toSpiralTrail.SetData(NULL);
  _toColoredStarsTrail.SetData(NULL);
  _toFireball01Trail.SetData(NULL);
  _toRocketTrail.SetData(NULL);
  _toGrenadeTrail.SetData(NULL);
  _toCannonBall.SetData(NULL);
  _toVerticalGradient.SetData(NULL);
  _toVerticalGradientAlpha.SetData(NULL);
  _toBlood01Trail.SetData(NULL);
  _toLavaTrailGradient.SetData(NULL);
  _toWaterfallGradient.SetData(NULL);
  _toGhostbusterBeam.SetData( NULL);
  _toLightning.SetData( NULL);
  _toLavaTrailSmoke.SetData(NULL);
  _toFlamethrowerTrail.SetData(NULL);
  _toBoubble01.SetData(NULL);
  _toBoubble02.SetData(NULL);
  _toBoubble03.SetData(NULL);
  _toStar01.SetData(NULL);
  _toStar02.SetData(NULL);
  _toStar03.SetData(NULL);
  _toStar04.SetData(NULL);
  _toStar05.SetData(NULL);
  _toStar06.SetData(NULL);
  _toStar07.SetData(NULL);
  _toStar08.SetData(NULL);
  _toSand.SetData(NULL);
  _toSandFlowGradient.SetData(NULL);
  _toWater.SetData(NULL);
  _toWaterFlowGradient.SetData(NULL);
  _toLava.SetData(NULL);
  _toLavaFlowGradient.SetData(NULL);
  _toLavaBombTrailSmoke.SetData(NULL);
  _toLavaBombTrailGradient.SetData(NULL);
  _toBloodSprayTexture.SetData(NULL);
  _toFlowerSprayTexture.SetData(NULL);
  _toBonesSprayTexture.SetData(NULL);
  _toFeatherSprayTexture.SetData(NULL);
  _toStonesSprayTexture.SetData(NULL);
  _toLavaSprayTexture.SetData(NULL);
  _toBeastProjectileSprayTexture.SetData(NULL);
  _toLavaEruptingTexture.SetData(NULL);
  _toWoodSprayTexture.SetData(NULL);
  _toElectricitySparks.SetData(NULL);
  _toBeastDebrisTrailGradient.SetData(NULL);
  _toBeastProjectileTrailTexture.SetData(NULL);
  _toBeastProjectileTrailGradient.SetData(NULL);
  _toBeastBigProjectileTrailTexture.SetData(NULL);
  _toBeastBigProjectileTrailGradient.SetData(NULL);
  _toBeastDebrisTrailTexture.SetData(NULL);
  _toRaindrop.SetData(NULL);
  _toSnowdrop.SetData(NULL);
  _toBulletStone.SetData(NULL);
  _toBulletWater.SetData(NULL);
  _toBulletSand.SetData(NULL);
  _toBulletSpark.SetData(NULL);
  _toBulletSmoke.SetData(NULL);
  _toPlayerParticles.SetData(NULL);
  _toWaterfallFoam.SetData(NULL);
  _toMetalSprayTexture.SetData(NULL);
}

void Particles_ViewerLocal(CEntity *penView)
{
  ASSERT(penView!=NULL);

  // ----------- Obtain world settings controller
  CWorldSettingsController *pwsc = NULL;
  // obtain bcg viewer
  CBackgroundViewer *penBcgViewer = (CBackgroundViewer *) penView->GetWorld()->GetBackgroundViewer();
  if( penBcgViewer != NULL)
  {
    // obtain world settings controller 
    pwsc = (CWorldSettingsController *) penBcgViewer->m_penWorldSettingsController.ep_pen;
  }

  // ***** Storm appearing effects
  // if world settings controller is valid
  if( (pwsc != NULL) && (pwsc->m_tmStormStart != -1))
  {
    FLOAT fStormFactor = pwsc->GetStormFactor();
    if( fStormFactor != 0.0f)
    {
      FLOATaabbox3D boxRainMap;
      CTextureData *ptdRainMap;
      pwsc->GetHeightMapData( ptdRainMap, boxRainMap);
      Particles_Rain( penView, 1.25f, 32, fStormFactor, ptdRainMap, boxRainMap);
    }
  }
}

// different particle effects
#define ROMBOID_TRAIL_POSITIONS 16
void Particles_RomboidTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(ROMBOID_TRAIL_POSITIONS);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  Particle_PrepareTexture(&_toRomboidTrail, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  for(INDEX iPos = 0; iPos<plp->lp_ctUsed; iPos++)
  {
    FLOAT3D vPos = plp->GetPosition(iPos);
    //FLOAT fRand = rand()/FLOAT(RAND_MAX);
    FLOAT fAngle = fSeconds*256+iPos*2.0f*PI/ROMBOID_TRAIL_POSITIONS;
    FLOAT fSin = FLOAT(sin(fAngle));
    vPos(2) += fSin*iPos/ROMBOID_TRAIL_POSITIONS;
    FLOAT fSize = (ROMBOID_TRAIL_POSITIONS-iPos)*0.5f/ROMBOID_TRAIL_POSITIONS+0.1f;
    UBYTE ub = 255-iPos*255/ROMBOID_TRAIL_POSITIONS;
    Particle_RenderSquare( vPos, fSize, fAngle, RGBToColor(255-ub,ub,255-ub)|ub);
  }
  // all done
  Particle_Flush();
}

#define BOMB_TRAIL_POSITIONS 8
#define BOMB_TRAIL_INTERPOSITIONS 4
void Particles_BombTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(BOMB_TRAIL_POSITIONS);
}
void Particles_BombTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(BOMB_TRAIL_POSITIONS);

  Particle_PrepareTexture(&_toBombTrail,  PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(plp->lp_ctUsed-1);
  for(INDEX iPos = plp->lp_ctUsed-1; iPos>=1; iPos--)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    for (INDEX iInter=0; iInter<BOMB_TRAIL_INTERPOSITIONS; iInter++) {
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/BOMB_TRAIL_INTERPOSITIONS);
      FLOAT fRand = rand()/FLOAT(RAND_MAX);
      FLOAT fAngle = fRand*2.0f*PI;
      FLOAT fSize = (BOMB_TRAIL_POSITIONS-iPos)*0.05f/BOMB_TRAIL_POSITIONS;
      UBYTE ub = 255-iPos*256/BOMB_TRAIL_POSITIONS;
      Particle_RenderSquare( vPos, fSize, fAngle, RGBToColor(ub,ub,ub)|ub);
    }
  }
  // all done
  Particle_Flush();
}

#define FIRECRACKER_TRAIL_POSITIONS 16
#define FIRECRACKER_TRAIL_INTERPOSITIONS 4
#define FIRECRACKER_TRAIL_PARTICLES (FIRECRACKER_TRAIL_INTERPOSITIONS*FIRECRACKER_TRAIL_POSITIONS)
void Particles_FirecrackerTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(FIRECRACKER_TRAIL_POSITIONS);
}
void Particles_FirecrackerTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(FIRECRACKER_TRAIL_POSITIONS);
  Particle_PrepareTexture(&_toFirecrackerTrail,  PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  if( plp->lp_ctUsed<2) return;
  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(plp->lp_ctUsed-1);
  INDEX iParticle = plp->lp_ctUsed*FIRECRACKER_TRAIL_INTERPOSITIONS;
  for(INDEX iPos = plp->lp_ctUsed-2; iPos>=0; iPos--)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    for (INDEX iInter=0; iInter<FIRECRACKER_TRAIL_INTERPOSITIONS; iInter++)
    {
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/FIRECRACKER_TRAIL_INTERPOSITIONS);
      FLOAT fSize = 
        (FIRECRACKER_TRAIL_PARTICLES-iParticle)*0.25f/FIRECRACKER_TRAIL_PARTICLES;
      UBYTE ub = 255-iParticle*255/FIRECRACKER_TRAIL_PARTICLES;
      Particle_RenderSquare( vPos, fSize/2.0f, 0, RGBToColor(ub,ub,ub)|0xFF);
      iParticle--;
    }
  }
  // all done
  Particle_Flush();
}

#define SPIRAL_TRAIL_POSITIONS 16
void Particles_SpiralTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(SPIRAL_TRAIL_POSITIONS);
  
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  Particle_PrepareTexture(&_toSpiralTrail, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  for(INDEX iPos = 0; iPos<plp->lp_ctUsed; iPos++)
  {
    FLOAT3D vPos = plp->GetPosition(iPos);
    FLOAT fAngle = fSeconds*32.0f+iPos*2*PI/SPIRAL_TRAIL_POSITIONS;
    FLOAT fSin = FLOAT(sin(fAngle));
    FLOAT fCos = FLOAT(cos(fAngle));

    vPos(1) += fSin*iPos*1.0f/SPIRAL_TRAIL_POSITIONS;
    vPos(2) += fCos*iPos*1.0f/SPIRAL_TRAIL_POSITIONS;

    UBYTE ub = iPos*SPIRAL_TRAIL_POSITIONS;
    Particle_RenderSquare( vPos, 0.2f, fAngle, RGBToColor(ub,ub,ub)|ub);
  }
  // all done
  Particle_Flush();
}

static COLOR _aColors[] = {  C_WHITE, C_GRAY,
  C_RED,  C_GREEN,  C_BLUE,  C_CYAN,  C_MAGENTA,  C_YELLOW,  C_ORANGE,  C_BROWN,  C_PINK,
  C_lRED, C_lGREEN, C_lBLUE, C_lCYAN, C_lMAGENTA, C_lYELLOW, C_lORANGE, C_lBROWN, C_lPINK
};

#define COLORED_STARS_TRAIL_POSITIONS 16
void Particles_ColoredStarsTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(COLORED_STARS_TRAIL_POSITIONS);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();

  Particle_PrepareTexture(&_toColoredStarsTrail, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  for(INDEX iPos = 0; iPos<plp->lp_ctUsed; iPos++)
  {
    FLOAT3D vPos1 = plp->GetPosition(iPos);
    //FLOAT3D vPos2 = vPos1;

    FLOAT fAngle = fSeconds*64.0f+iPos*2*PI/COLORED_STARS_TRAIL_POSITIONS;
    FLOAT fSin = FLOAT(sin(fAngle));
    //FLOAT fCos = FLOAT(cos(fAngle));

    FLOAT fDeltaY = fSin/2.0f;
    vPos1(2)  += fDeltaY;
    //vPos2(2) -= fDeltaY;

    FLOAT fRand = rand()/FLOAT(RAND_MAX);
    INDEX iRandColor = INDEX(fRand*sizeof(_aColors)/sizeof(COLOR));
    COLOR colColor1 = _aColors[ iRandColor];
    Particle_RenderSquare( vPos1,  0.4f, fAngle, colColor1);
  }
  // all done
  Particle_Flush();
}

#define WHITE_LINE_TRAIL_POSITIONS 8
void Particles_WhiteLineTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(WHITE_LINE_TRAIL_POSITIONS);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  Particle_PrepareTexture(&_toSpiralTrail, PBT_ADD);
  Particle_SetTexturePart( 1, 1, 256, 256);

  FLOAT3D vOldPos = plp->GetPosition(0);
  for(INDEX iPos = 1; iPos<plp->lp_ctUsed; iPos++)
  {
    FLOAT3D vPos = plp->GetPosition(iPos);
    FLOAT fAngle = fSeconds*4.0f+iPos*PI/WHITE_LINE_TRAIL_POSITIONS;
    FLOAT fSin = FLOAT(sin(fAngle));
    FLOAT fCos = FLOAT(cos(fAngle));

    vPos(1) += fSin*iPos*1.0f/WHITE_LINE_TRAIL_POSITIONS;
    vPos(2) += fCos*iPos*1.0f/WHITE_LINE_TRAIL_POSITIONS;

    //UBYTE ub = 255-iPos*256/WHITE_LINE_TRAIL_POSITIONS;
    FLOAT fLerpFactor = FLOAT(iPos)/WHITE_LINE_TRAIL_POSITIONS;
    COLOR colColor = LerpColor( C_YELLOW, C_dRED, fLerpFactor);
    Particle_RenderLine( vPos, vOldPos, 0.05f, colColor);
    vOldPos =vPos;
  }
  // all done
  Particle_Flush();
}


#define FIREBALL01_TRAIL_POSITIONS 8
#define FIREBALL01_TRAIL_INTERPOSITIONS 4
#define FIREBALL01_TRAIL_PARTICLES (FIREBALL01_TRAIL_INTERPOSITIONS*FIREBALL01_TRAIL_POSITIONS)
void Particles_Fireball01Trail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(FIREBALL01_TRAIL_POSITIONS);
}
void Particles_Fireball01Trail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(FIREBALL01_TRAIL_POSITIONS);
  Particle_PrepareTexture(&_toFireball01Trail,  PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(plp->lp_ctUsed-1);
  INDEX iParticle = 0;
  INDEX iParticlesLiving = plp->lp_ctUsed*FIREBALL01_TRAIL_INTERPOSITIONS;
  for(INDEX iPos = plp->lp_ctUsed-2; iPos>=0; iPos--) {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    COLOR colColor;
    for (INDEX iInter=0; iInter<FIREBALL01_TRAIL_INTERPOSITIONS; iInter++)
    {
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/FIREBALL01_TRAIL_INTERPOSITIONS);
      FLOAT fSize = iParticle*0.3f/iParticlesLiving;
      UBYTE ub = UBYTE(iParticle*255/iParticlesLiving);
      colColor = RGBToColor(ub,ub,ub)|0xFF;
      Particle_RenderSquare( vPos, fSize, 0, colColor);
      iParticle++;
    }
  }
  // all done
  Particle_Flush();
}

#define GRENADE_TRAIL_POSITIONS 16
#define GRENADE_TRAIL_INTERPOSITIONS 2
void Particles_GrenadeTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(GRENADE_TRAIL_POSITIONS);
}
void Particles_GrenadeTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(GRENADE_TRAIL_POSITIONS);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  Particle_PrepareTexture(&_toGrenadeTrail, PBT_MULTIPLY);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(0);
  INDEX iParticle = 0;
  INDEX iParticlesLiving = plp->lp_ctUsed*GRENADE_TRAIL_INTERPOSITIONS;
  for(INDEX iPos = 1; iPos<plp->lp_ctUsed; iPos++)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    for (INDEX iInter=0; iInter<GRENADE_TRAIL_INTERPOSITIONS; iInter++)
    {
      FLOAT fAngle = iParticle*4.0f*180/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/GRENADE_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = iParticle*0.3f/iParticlesLiving+0.1f;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964*PI))*0.05f;
      UBYTE ub = 255-(UBYTE)((ULONG)iParticle*255/iParticlesLiving);
      Particle_RenderSquare( vPos, fSize, fAngle, RGBToColor(ub,ub,ub)|ub);
      iParticle++;
    }
  }
  // all done
  Particle_Flush();
}

#define CANNON_TRAIL_POSITIONS 12
#define CANNON_TRAIL_INTERPOSITIONS 1
void Particles_CannonBall_Prepare(CEntity *pen)
{
  pen->GetLastPositions(CANNON_TRAIL_POSITIONS);
}
void Particles_CannonBall(CEntity *pen, FLOAT fSpeedRatio)
{
  CLastPositions *plp = pen->GetLastPositions(CANNON_TRAIL_POSITIONS);
 // FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  Particle_PrepareTexture(&_toCannonBall, PBT_BLEND);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT3D vOldPos = plp->GetPosition(1);
  for( INDEX iPos=2; iPos<plp->lp_ctUsed; iPos++)
  {
    FLOAT3D vPos = plp->GetPosition(iPos);
    UBYTE ub = UBYTE((255-iPos*256/plp->lp_ctUsed)*fSpeedRatio);
    FLOAT fSize = (CANNON_TRAIL_POSITIONS-iPos)*0.04f+0.04f;
    Particle_RenderLine( vPos, vOldPos, fSize, RGBToColor(ub,ub,ub)|ub);
    vOldPos=vPos;
  }
  // all done
  Particle_Flush();
}

#define LAVA_TRAIL_POSITIONS 32
#define LAVA_TRAIL_INTERPOSITIONS 1
void Particles_LavaTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(LAVA_TRAIL_POSITIONS);
}
void Particles_LavaTrail(CEntity *pen)
{
  CLastPositions *plp = pen->GetLastPositions(LAVA_TRAIL_POSITIONS);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  CTextureData *pTD = (CTextureData *) _toLavaTrailGradient.GetData();
  Particle_PrepareTexture(&_toLavaTrailSmoke, PBT_BLEND);
  //Particle_PrepareTexture(&_toLavaTrailSmoke, PBT_MULTIPLY);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(0);
  INDEX iParticle = 0;
  INDEX iParticlesLiving = plp->lp_ctUsed*LAVA_TRAIL_INTERPOSITIONS;
  for(INDEX iPos = 1; iPos<plp->lp_ctUsed; iPos++)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    for (INDEX iInter=0; iInter<LAVA_TRAIL_INTERPOSITIONS; iInter++)
    {
      FLOAT fAngle = iParticle*4.0f*180/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/LAVA_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = iParticle*3.0f/iParticlesLiving+0.5f;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964*PI))*0.05f;
      COLOR col = pTD->GetTexel(PIX(FLOAT(iParticle)/iParticlesLiving*8*1024), 0);
      Particle_RenderSquare( vPos, fSize, fAngle, col);
      iParticle++;
    }
  }
  // all done
  Particle_Flush();
}

#define LAVA_BOMB_TRAIL_POSITIONS 16
#define LAVA_BOMB_TRAIL_INTERPOSITIONS 1
void Particles_LavaBombTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(LAVA_BOMB_TRAIL_POSITIONS);
}

void Particles_LavaBombTrail(CEntity *pen, FLOAT fSizeMultiplier)
{
  CLastPositions *plp = pen->GetLastPositions(LAVA_BOMB_TRAIL_POSITIONS);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  CTextureData *pTD = (CTextureData *) _toLavaBombTrailGradient.GetData();
  Particle_PrepareTexture(&_toLavaBombTrailSmoke, PBT_BLEND);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(0);
  INDEX iParticle = 0;
  INDEX iParticlesLiving = plp->lp_ctUsed*LAVA_BOMB_TRAIL_INTERPOSITIONS;
  for(INDEX iPos = 1; iPos<plp->lp_ctUsed; iPos++)
  {
    INDEX iRnd = ((ULONG)fSeconds+iPos)%CT_MAX_PARTICLES_TABLE;
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    if( *pvPos1 == *pvPos2) continue;
    for (INDEX iInter=0; iInter<LAVA_BOMB_TRAIL_INTERPOSITIONS; iInter++)
    {
      FLOAT fAngle = iParticle*4.0f*180/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/LAVA_BOMB_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = (iParticle*1.0f/iParticlesLiving+1.0f) * fSizeMultiplier;
      fSize += afStarsPositions[iRnd][0]*0.75f*fSizeMultiplier;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964*PI))*0.05f;
      COLOR col = pTD->GetTexel(PIX(FLOAT(iParticle)/iParticlesLiving*8*1024), 0);
      Particle_RenderSquare( vPos, fSize, fAngle, col);
      iParticle++;
    }
  }
  // all done
  Particle_Flush();
}

#define BEAST_PROJECTILE_DEBRIS_TRAIL_POSITIONS 8
#define BEAST_PROJECTILE_DEBRIS_TRAIL_INTERPOSITIONS 1
void Particles_BeastProjectileDebrisTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(BEAST_PROJECTILE_DEBRIS_TRAIL_POSITIONS);
}

void Particles_BeastProjectileDebrisTrail(CEntity *pen, FLOAT fSizeMultiplier)
{
  CLastPositions *plp = pen->GetLastPositions(BEAST_PROJECTILE_DEBRIS_TRAIL_POSITIONS);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  CTextureData *pTD = (CTextureData *) _toBeastDebrisTrailGradient.GetData();
  Particle_PrepareTexture(&_toBeastDebrisTrailTexture, PBT_BLEND);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(0);
  INDEX iParticle = 0;
  INDEX iParticlesLiving = plp->lp_ctUsed*BEAST_PROJECTILE_DEBRIS_TRAIL_INTERPOSITIONS;
  for(INDEX iPos = 1; iPos<plp->lp_ctUsed; iPos++)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    for (INDEX iInter=0; iInter<BEAST_PROJECTILE_DEBRIS_TRAIL_INTERPOSITIONS; iInter++)
    {
      FLOAT fAngle = iParticle*4.0f*180/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/BEAST_PROJECTILE_DEBRIS_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = ((iParticle*iParticle+1.0f)/iParticlesLiving+2.0f) * fSizeMultiplier;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964*PI))*0.05f;
      COLOR col = pTD->GetTexel(PIX(FLOAT(iParticle)/iParticlesLiving*8*1024), 0);
      Particle_RenderSquare( vPos, fSize, fAngle, col);
      iParticle++;
    }
  }
  // all done
  Particle_Flush();
}

#define BEAST_PROJECTILE_TRAIL_POSITIONS 32
#define BEAST_PROJECTILE_TRAIL_INTERPOSITIONS 1
void Particles_BeastProjectileTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(BEAST_PROJECTILE_TRAIL_POSITIONS);
}

#define BEAST_PROJECTILE_LINE_PARTICLES 0.4f
#define BEAST_PROJECTILE_FADE_OUT 0.3f
#define BEAST_PROJECTILE_TOTAL_TIME 0.6f
void Particles_BeastProjectileTrail( CEntity *pen, FLOAT fSize, FLOAT fHeight, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  Particle_PrepareTexture(&_toBeastProjectileTrailTexture, PBT_BLEND);
  Particle_SetTexturePart( 512, 2048, 0, 0);

  CTextureData *pTD = (CTextureData *) _toBeastProjectileTrailGradient.GetData();

  CPlacement3D pl = pen->GetLerpedPlacement();
  FLOATmatrix3D m;
  MakeRotationMatrixFast(m, pl.pl_OrientationAngle);
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( -m(1,3), -m(2,3), -m(3,3));
  FLOAT3D vZ( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pl.pl_PositionVector+vY*fHeight;

  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    FLOAT fT = fNow+afTimeOffsets[iStar];
    // apply time strech
    fT *= 1/BEAST_PROJECTILE_TOTAL_TIME;
    // get fraction part
    fT = fT-int(fT);
    //FLOAT fFade;
    //if (fT>(1.0f-BEAST_PROJECTILE_FADE_OUT)) fFade=(1-fT)*(1/BEAST_PROJECTILE_FADE_OUT);
    //else fFade=1.0f;

#define GET_POS( time) vCenter + \
      vX*(afStarsPositions[iStar][0]*time*fSize*1.5) +\
      vY*(-time*time*10.0f+(afStarsPositions[iStar][1]*2+2.0f)*1.2f*time) +\
      vZ*(afStarsPositions[iStar][2]*time*fSize*1.5);

    FLOAT3D vPos = GET_POS( fT);
    COLOR colStar = pTD->GetTexel( FloatToInt(fT*8192), 0);

    if( fT>BEAST_PROJECTILE_LINE_PARTICLES)
    {
      FLOAT fTimeOld = fT-0.25f;
      FLOAT3D vOldPos = GET_POS( fTimeOld);
      Particle_RenderLine( vOldPos, vPos, 0.4f, colStar);
    }
    else
    {
      Particle_RenderSquare( vPos, 0.5f, fT*360.0f, colStar);
    }
  }
  // all done
  Particle_Flush();
}

#define BEAST_BIG_PROJECTILE_TRAIL_POSITIONS 32
#define BEAST_BIG_PROJECTILE_TRAIL_INTERPOSITIONS 1
void Particles_BeastBigProjectileTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(BEAST_BIG_PROJECTILE_TRAIL_POSITIONS);
}

#define BIG_BEAST_PROJECTILE_LINE_PARTICLES 0.4f
#define BIG_BEAST_PROJECTILE_FADE_OUT 0.4f
#define BIG_BEAST_PROJECTILE_TOTAL_TIME 0.6f
void Particles_BeastBigProjectileTrail( CEntity *pen, FLOAT fSize, FLOAT fZOffset, FLOAT fYOffset, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  Particle_PrepareTexture(&_toBeastBigProjectileTrailTexture, PBT_BLEND);
  Particle_SetTexturePart( 512, 2048, 0, 0);

  CTextureData *pTD = (CTextureData *) _toBeastBigProjectileTrailGradient.GetData();

  CPlacement3D pl = pen->GetLerpedPlacement();
  FLOATmatrix3D m;
  MakeRotationMatrixFast(m, pl.pl_OrientationAngle);
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( -m(1,3), -m(2,3), -m(3,3));
  FLOAT3D vZ( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pl.pl_PositionVector+vY*fZOffset+vZ*fYOffset;

  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    FLOAT fT = fNow+afTimeOffsets[iStar];
    // apply time strech
    fT *= 1/BIG_BEAST_PROJECTILE_TOTAL_TIME;
    // get fraction part
    fT = fT-int(fT);
    FLOAT fFade;
    if (fT>(1.0f-BIG_BEAST_PROJECTILE_FADE_OUT)) fFade=(1-fT)*(1/BIG_BEAST_PROJECTILE_FADE_OUT);
    else fFade=1.0f;

#define GET_POS_BIG( time) vCenter + \
      vX*(afStarsPositions[iStar][0]*time*fSize*1.5) +\
      vY*(time*time*-15.0f+(afStarsPositions[iStar][1]*2+3.0f)*1.2f*time) +\
      vZ*(afStarsPositions[iStar][2]*time*fSize*1.5);

    FLOAT3D vPos = GET_POS_BIG( fT);
    COLOR colStar = pTD->GetTexel( FloatToInt(fT*8192), 0);

    if( fT>BIG_BEAST_PROJECTILE_LINE_PARTICLES)
    {
      FLOAT fTimeOld = fT-0.125f;
      FLOAT3D vOldPos = GET_POS_BIG( fTimeOld);
      Particle_RenderLine( vOldPos, vPos, 0.6f*fFade, colStar);
    }
    else
    {
      Particle_RenderSquare( vPos, 0.5, fT*360.0f, colStar);
    }
  }
  // all done
  Particle_Flush();
}

#define ROCKET_TRAIL_POSITIONS 16
#define ROCKET_TRAIL_INTERPOSITIONS 3
void Particles_RocketTrail_Prepare(CEntity *pen)
{
  pen->GetLastPositions(ROCKET_TRAIL_POSITIONS);
}
void Particles_RocketTrail(CEntity *pen, FLOAT fStretch)
{
  CLastPositions *plp = pen->GetLastPositions(ROCKET_TRAIL_POSITIONS);
  //FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
  Particle_PrepareTexture(&_toRocketTrail, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(1);
  INDEX iParticle = 0;
  INDEX iParticlesLiving = plp->lp_ctUsed*ROCKET_TRAIL_INTERPOSITIONS;
  for( INDEX iPos=2; iPos<plp->lp_ctUsed; iPos++)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    if( (*pvPos2 - *pvPos1).Length() == 0.0f)
    {
      continue;
    }
    for (INDEX iInter=0; iInter<ROCKET_TRAIL_INTERPOSITIONS; iInter++)
    {
      //FLOAT fRand = rand()/FLOAT(RAND_MAX);
      FLOAT fAngle = 0.0f;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/ROCKET_TRAIL_INTERPOSITIONS);
      FLOAT fSize = iParticle*0.5f/iParticlesLiving*fStretch+0.25f;

      UBYTE ub = 255-(UBYTE)((ULONG)iParticle*255/iParticlesLiving);
      FLOAT fLerpFactor = FLOAT(iPos)/ROCKET_TRAIL_POSITIONS;
      COLOR colColor = LerpColor( C_WHITE, C_BLACK, fLerpFactor);
      Particle_RenderSquare( vPos, fSize, fAngle, colColor|ub);
      iParticle++;
    }
  }
  Particle_Flush();

  // now render line
  Particle_PrepareTexture(&_toVerticalGradient, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);
  FLOAT3D vOldPos = plp->GetPosition(1);
  for( INDEX iPos=2; iPos<plp->lp_ctUsed; iPos++)
  {
    FLOAT3D vPos = plp->GetPosition(iPos);
    if( (vPos - vOldPos).Length() == 0.0f)
    {
      continue;
    }
    UBYTE ub = UBYTE(255-iPos*256/plp->lp_ctUsed);
    FLOAT fSize = iPos*0.01f*fStretch+0.005f;
    Particle_RenderLine( vPos, vOldPos, fSize, RGBToColor(ub,ub,ub)|ub);
    vOldPos=vPos;
  }
  // all done
  Particle_Flush();
}


#define BLOOD01_TRAIL_POSITIONS 15
void Particles_BloodTrail(CEntity *pen)
{
  // get blood type
  const INDEX iBloodType = GetSP()->sp_iBlood;
  if( iBloodType<1) return;
  COLOR col;
  if( iBloodType==3) Particle_PrepareTexture( &_toFlowerSprayTexture, PBT_BLEND);
  else               Particle_PrepareTexture( &_toBloodSprayTexture,  PBT_BLEND);

  CLastPositions *plp = pen->GetLastPositions(BLOOD01_TRAIL_POSITIONS);
  FLOAT fGA = ((CMovableEntity *)pen)->en_fGravityA;
  FLOAT3D vGDir = ((CMovableEntity *)pen)->en_vGravityDir;

  for( INDEX iPos=0; iPos<plp->lp_ctUsed; iPos++)
  {
    Particle_SetTexturePart( 256, 256, iPos%8, 0);
    FLOAT3D vPos = plp->GetPosition(iPos);
    //FLOAT fRand  = rand()/FLOAT(RAND_MAX);
    FLOAT fAngle = iPos*2.0f*PI/BLOOD01_TRAIL_POSITIONS;
    //FLOAT fSin = FLOAT(sin(fAngle));
    FLOAT fT = iPos*_pTimer->TickQuantum;
    vPos += vGDir*fGA*fT*fT/8.0f;
    FLOAT fSize = 0.2f-iPos*0.15f/BLOOD01_TRAIL_POSITIONS;
    UBYTE ub = 255-iPos*255/BLOOD01_TRAIL_POSITIONS;
         if( iBloodType==3) col = C_WHITE|ub;
    else if( iBloodType==2) col = RGBAToColor(ub,20,20,ub);
    else                    col = RGBAToColor(0,ub,0,ub);
    Particle_RenderSquare( vPos, fSize, fAngle, col);
  }
  // all done
  Particle_Flush();
}


INDEX Particles_FireBreath(CEntity *pen, FLOAT3D vSource, FLOAT3D vTarget, FLOAT tmStart, FLOAT tmStop)
{
  Particle_PrepareTexture( &_toFlamethrowerTrail, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fFlameLife = 2;
  INDEX ctFlames = 32;
  INDEX ctRendered = 0;
  FLOAT tmFlameDelta = 0.25f;
  FLOAT3D vFocus = Lerp( vSource, vTarget, 0.25f);
  for( INDEX iFlame=0; iFlame<ctFlames; iFlame++)
  {
    FLOAT tmFakeStart = tmStart+iFlame*tmFlameDelta+afStarsPositions[iFlame*2][0]*tmFlameDelta;
    FLOAT fPassedTime = fNow-tmFakeStart;
    if(fPassedTime<0.0f || fPassedTime>fFlameLife || tmFakeStart>tmStop) continue;
    // calculate fraction part
    FLOAT fT=fPassedTime/fFlameLife;
    fT=fT-INDEX(fT);
    // lerp position
    FLOAT3D vRnd = FLOAT3D( afStarsPositions[iFlame][0],afStarsPositions[iFlame][1],
      afStarsPositions[iFlame][2])*10;
    FLOAT3D vPos = Lerp( vSource, vFocus+vRnd, fT);
    FLOAT fSize = 5.0f*fT+5.0f;
    UBYTE ub = CalculateRatio( fT, 0.0f, 1.0f, 0.1f, 0.2f)*255;
    Particle_RenderSquare( vPos, fSize, fT*(1.0f+afStarsPositions[iFlame*3][1])*360.0f, RGBToColor(ub,ub,ub)|0xFF);
    ctRendered++;
  }
  // all done
  Particle_Flush();
  return ctRendered;
}

INDEX Particles_Regeneration(CEntity *pen, FLOAT tmStart, FLOAT tmStop, FLOAT fYFactor, BOOL bDeath)
{
  Particle_PrepareTexture( &_toElectricitySparks, PBT_BLEND);
  Particle_SetTexturePart( 512, 1024, 0, 0);
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fLife = 1.5;
  INDEX ctRendered = 0;
  FLOAT tmDelta = 0.001f;
  for( INDEX iVtx=0; iVtx<1024*4; iVtx++)
  {
    FLOAT tmFakeStart = tmStart+iVtx*tmDelta;
    FLOAT fPassedTime = fNow-tmFakeStart;
    if(fPassedTime<0.0f || fPassedTime>fLife || tmFakeStart>tmStop) continue;
    // calculate fraction part
    FLOAT fT=fPassedTime/fLife;
    fT=fT-INDEX(fT);

    INDEX iRnd = iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vRnd= FLOAT3D(afStarsPositions[iRnd][0],afStarsPositions[iRnd][1]+0.5f,afStarsPositions[iRnd][2]);
    vRnd(1) *= 800.0f;
    vRnd(2) *= 400.0f;
    vRnd(3) *= 800.0f;
    FLOAT3D vSource = vCenter+vRnd;
    FLOAT3D vDestination = vCenter+vRnd*0.05f;
    vDestination(2) += 40.0f*fYFactor+vRnd(2)/8.0f*fYFactor;
    FLOAT3D vPos, vPos2;
    // lerp position
    if(bDeath) {
      vPos = Lerp( vSource, vDestination, 1.0f-fT);
    } else {
      vPos = Lerp( vSource, vDestination, fT);
    }
    FLOAT fT2 = Clamp(fT-0.025f-fT*fT*0.025f, 0.0f, 1.0f);
    
    if(bDeath) {
      vPos2 = Lerp( vSource, vDestination, 1.0f-fT2);
    } else {
      vPos2 = Lerp( vSource, vDestination, fT2);
    }
    
    UBYTE ubR = 192+afStarsPositions[iRnd][0]*64;
    UBYTE ubG = 192+afStarsPositions[iRnd][1]*64;
    UBYTE ubB = 192+afStarsPositions[iRnd][2]*64;
    UBYTE ubA = CalculateRatio( fT, 0.0f, 1.0f, 0.4f, 0.01f)*255;
    COLOR colLine = RGBToColor( ubR, ubG, ubB) | ubA;
    
    FLOAT fSize = 1.0f;
    Particle_RenderLine( vPos2, vPos, fSize, colLine);
    ctRendered++;
  }

  // flush array
  avVertices.PopAll();
  // all done
  Particle_Flush();
  return ctRendered;
}

#define SECONDS_PER_PARTICLE 0.01f
void Particles_FlameThrower(const CPlacement3D &plEnd, const CPlacement3D &plStart,
                            FLOAT fEndElapsed, FLOAT fStartElapsed)
{
  Particle_PrepareTexture( &_toFlamethrowerTrail, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D &vStart = plStart.pl_PositionVector;
  const FLOAT3D &vEnd = plEnd.pl_PositionVector;
  FLOAT3D vDelta = (vEnd-vStart)/(fEndElapsed-fStartElapsed);
  FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();

  for(FLOAT fTime = ceil(fStartElapsed/SECONDS_PER_PARTICLE)*SECONDS_PER_PARTICLE;
      fTime<fEndElapsed; fTime+=SECONDS_PER_PARTICLE)
  {
    FLOAT fAngle = (fSeconds+fTime)*90.0f;
    FLOAT fSize = 1.5*fTime+0.01;
    FLOAT fRise = (fTime*fTime);
    UBYTE ub = 96;
    if( fTime>0.75f) ub = 0;
    else if( fTime>0.5f) ub = (-4*fTime+3)*96;

    ASSERT(fTime>=0.0f);
    ASSERT(fSize>=0.0f);
    FLOAT3D vPos = vStart+vDelta*(fTime-fStartElapsed)+FLOAT3D(0.0f, fRise, 0.0f);
    Particle_RenderSquare( vPos, fSize, fAngle, RGBToColor(ub,ub,ub)|0xFF);
  }
  // all done
  Particle_Flush();
}

void SetupParticleTexture(enum ParticleTexture ptTexture)
{
  switch(ptTexture) {
  case PT_STAR01:           Particle_PrepareTexture(&_toStar01,    PBT_ADD);   break;
  case PT_STAR02:           Particle_PrepareTexture(&_toStar02,    PBT_ADD);   break;
  case PT_STAR03:           Particle_PrepareTexture(&_toStar03,    PBT_ADD);   break;
  case PT_STAR04:           Particle_PrepareTexture(&_toStar04,    PBT_ADD);   break;
  case PT_STAR05:           Particle_PrepareTexture(&_toStar05,    PBT_ADD);   break;
  case PT_STAR06:           Particle_PrepareTexture(&_toStar06,    PBT_ADD);   break;
  case PT_STAR07:           Particle_PrepareTexture(&_toStar07,    PBT_ADD);   break;
  case PT_STAR08:           Particle_PrepareTexture(&_toStar08,    PBT_ADD);   break;
  case PT_BOUBBLE01:        Particle_PrepareTexture(&_toBoubble01, PBT_ADD);   break;
  case PT_BOUBBLE02:        Particle_PrepareTexture(&_toBoubble02, PBT_ADD);   break;
  case PT_WATER01:          Particle_PrepareTexture(&_toBoubble03, PBT_BLEND); break;
  case PT_WATER02:          Particle_PrepareTexture(&_toBoubble03, PBT_BLEND); break;
  case PT_SANDFLOW:         Particle_PrepareTexture(&_toSand,      PBT_BLEND); break;
  case PT_WATERFLOW:        Particle_PrepareTexture(&_toWater,     PBT_BLEND); break;
  case PT_LAVAFLOW:         Particle_PrepareTexture(&_toLava,      PBT_BLEND); break;
  default:    ASSERT(FALSE);
  }
  Particle_SetTexturePart( 512, 512, 0, 0);
}

void InitParticleTables(void)
{
  for( INDEX iStar=0; iStar<CT_MAX_PARTICLES_TABLE; iStar++)
  {
    afTimeOffsets[iStar] = rand()/FLOAT(RAND_MAX)*10;
    afStarsPositions[iStar][0] = rand()/FLOAT(RAND_MAX)-0.5f;
    afStarsPositions[iStar][1] = rand()/FLOAT(RAND_MAX)-0.5f;
    afStarsPositions[iStar][2] = rand()/FLOAT(RAND_MAX)-0.5f;
    UBYTE ubR = UBYTE(rand()/FLOAT(RAND_MAX) * 255);
    UBYTE ubG = UBYTE(rand()/FLOAT(RAND_MAX) * 255);
    UBYTE ubB = UBYTE(rand()/FLOAT(RAND_MAX) * 255);
    auStarsColors[iStar][0] = ubR;
    auStarsColors[iStar][1] = ubG;
    auStarsColors[iStar][2] = ubB;
  }
}

#define STARDUST_EXIST_TIME 0.15f
void Particles_Stardust( CEntity *pen, FLOAT fSize, FLOAT fHeight,
                        enum ParticleTexture ptTexture, INDEX ctParticles)
{
  INDEX ctOffsetSpace = 128;

  ASSERT( (ctParticles+ctOffsetSpace)<=CT_MAX_PARTICLES_TABLE);
  if( Particle_GetMipFactor()>7.0f) return;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  SetupParticleTexture( ptTexture);
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  CPlacement3D plSource = pen->GetLerpedPlacement();
  FLOAT3D vCenter = plSource.pl_PositionVector+vY*fHeight;

  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    FLOAT fT = fNow+afTimeOffsets[iStar];
    // apply time strech
    fT *= 0.3f;
    // get fraction part
    fT = fT-int(fT);
    if( fT>STARDUST_EXIST_TIME) continue;
    FLOAT fFade = -2.0f * Abs(fT*(1.0f/STARDUST_EXIST_TIME)-0.5f)+1.0f;

    INDEX iRandomFromPlacement = 
      (ULONG)(plSource.pl_PositionVector(1)+plSource.pl_PositionVector(3))&(ctOffsetSpace-1);
    INDEX iMemeber = iStar+iRandomFromPlacement;
    const FLOAT3D vPos = vCenter+FLOAT3D( afStarsPositions[iMemeber][0], 
                                          afStarsPositions[iMemeber][1],
                                          afStarsPositions[iMemeber][2])*fSize;
    COLOR colStar = RGBToColor( auStarsColors[iMemeber][0]*fFade,
                                auStarsColors[iMemeber][1]*fFade,
                                auStarsColors[iMemeber][2]*fFade);
    Particle_RenderSquare( vPos, 0.15f, 0, colStar|0xFF);
  }
  // all done
  Particle_Flush();
}

#define RISING_TOTAL_TIME 5.0f
#define RISING_EXIST_TIME 3.0f
#define RISING_FADE_IN 0.3f
#define RISING_FADE_OUT 0.3f

void Particles_Rising(CEntity *pen, FLOAT fStartTime, FLOAT fStopTime, FLOAT fStretchAll,
                      FLOAT fStretchX, FLOAT fStretchY, FLOAT fStretchZ, FLOAT fSize,
                      enum ParticleTexture ptTexture, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  if( Particle_GetMipFactor()>7.0f) return;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  SetupParticleTexture( ptTexture);
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*fStretchY;

  FLOAT fPowerFactor = Clamp((fNow - fStartTime)/5.0f,0.0f,1.0f);
  fPowerFactor *= Clamp(1+(fStopTime-fNow)/5.0f,0.0f,1.0f);
  //ctParticles = FLOAT(ctParticles) * fPowerFactor;

  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    FLOAT fT = fNow+afTimeOffsets[iStar];
    // apply time strech
    fT *= 1/RISING_TOTAL_TIME;
    // get fraction part
    fT = fT-int(fT);
    FLOAT fF = fT*(RISING_TOTAL_TIME/RISING_EXIST_TIME);
    if( fF>1) continue;
    FLOAT fFade;
    if(fF<(RISING_FADE_IN*fPowerFactor)) fFade=fF*(1/(RISING_FADE_IN*fPowerFactor));
    else if (fF>(1.0f-RISING_FADE_OUT)) fFade=(1-fF)*(1/RISING_FADE_OUT);
    else fFade=1.0f;

    FLOAT3D vPos = vCenter+FLOAT3D(
      afStarsPositions[iStar][0]*fStretchX, 
      afStarsPositions[iStar][1]*fStretchY,
      afStarsPositions[iStar][2]*fStretchZ)*fStretchAll+vY*(fF*fStretchAll*0.5f);
    vPos(1)+=sin(fF*4.0f)*0.05f*fStretchAll;
    vPos(3)+=cos(fF*4.0f)*0.05f*fStretchAll;
    
    UBYTE ub = NormFloatToByte( fFade);
    COLOR colStar = RGBToColor( ub, ub, ub>>1);
    Particle_RenderSquare( vPos, fSize*fPowerFactor, 0, colStar|(UBYTE(0xFF*fPowerFactor)));
  }
  // all done
  Particle_Flush();
}

#define CT_SPIRAL_PARTICLES 4
#define CT_SPIRAL_TRAIL 10
void Particles_Spiral( CEntity *pen, FLOAT fSize, FLOAT fHeight,
                      enum ParticleTexture ptTexture, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);

  FLOAT fMipFactor = Particle_GetMipFactor();
  if( fMipFactor>7.0f) return;
  fMipFactor = 2.5f-fMipFactor*0.3f;
  fMipFactor = Clamp( fMipFactor, 0.0f, 1.0f);
  INDEX ctSpiralTrail = fMipFactor*CT_SPIRAL_TRAIL;
  if( ctSpiralTrail<=0) return;
  FLOAT fTrailDelta = 0.1f/fMipFactor;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  SetupParticleTexture( ptTexture);
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*fHeight;
  
  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    FLOAT fT = fNow+afTimeOffsets[iStar];

    for( INDEX iTrail=0; iTrail<ctSpiralTrail; iTrail++) {
      FLOAT3D vPos = vCenter;
      vPos(1)+=sin((fT-iTrail*fTrailDelta)*4.0f*(afStarsPositions[iStar][0]*3.0f)+0.3f)*0.5f*fSize;
      vPos(2)+=sin((fT-iTrail*fTrailDelta)*4.0f*(afStarsPositions[iStar][1]*3.0f)+0.9f)*0.5f*fSize;
      vPos(3)+=sin((fT-iTrail*fTrailDelta)*4.0f*(afStarsPositions[iStar][2]*3.0f)+0.1f)*0.5f*fSize;
      UBYTE ub = NormFloatToByte( (FLOAT)(ctSpiralTrail-iTrail) / (FLOAT)(ctSpiralTrail));
      COLOR colStar = RGBToColor( ub, ub, ub>>1);
      Particle_RenderSquare( vPos, 0.2f, 0, colStar|0xFF);
    }
  }
  // all done
  Particle_Flush();
}

#define EMANATE_FADE_IN 0.2f
#define EMANATE_FADE_OUT 0.6f
#define EMANATE_TOTAL_TIME 1.0f
#define EMANATE_EXIST_TIME 0.5f
void Particles_Emanate( CEntity *pen, FLOAT fSize, FLOAT fHeight,
                       enum ParticleTexture ptTexture, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  if( Particle_GetMipFactor()>7.0f) return;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  SetupParticleTexture( ptTexture);
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*fHeight;

  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    FLOAT fT = fNow+afTimeOffsets[iStar];
    // apply time strech
    fT *= 1/EMANATE_TOTAL_TIME;
    // get fraction part
    fT = fT-int(fT);
    FLOAT fF = fT*(EMANATE_TOTAL_TIME/EMANATE_EXIST_TIME);
    if( fF>1) continue;
    FLOAT fFade;
    if(fF<EMANATE_FADE_IN) fFade=fF*(1/EMANATE_FADE_IN);
    else if (fF>(1.0f-EMANATE_FADE_OUT)) fFade=(1-fF)*(1/EMANATE_FADE_OUT);
    else fFade=1.0f;

    FLOAT3D vPos = vCenter+FLOAT3D(
      afStarsPositions[iStar][0], 
      afStarsPositions[iStar][1],
      afStarsPositions[iStar][2])*fSize*(fF+0.4f);
    
    UBYTE ub = NormFloatToByte( fFade);
    COLOR colStar = RGBToColor( ub, ub, ub>>1);
    Particle_RenderSquare( vPos, 0.1f, 0, colStar|0xFF);
  }
  // all done
  Particle_Flush();
}

#define WATERFALL_FOAM_FADE_IN 0.1f
#define WATERFALL_FOAM_FADE_OUT 0.4f
void Particles_WaterfallFoam(CEntity *pen, FLOAT fSizeX, FLOAT fSizeY, FLOAT fSizeZ, 
                             FLOAT fParticleSize, FLOAT fSpeed, FLOAT fSpeedY, FLOAT fLife, INDEX ctParticles)
{
  if(fLife<=0) return;

  Particle_PrepareTexture( &_toWaterfallFoam, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  for( INDEX iFoam=0; iFoam<ctParticles; iFoam++)
  {
    FLOAT fT = (fNow+afTimeOffsets[iFoam]*fLife)/(fLife*(1.0f+afStarsPositions[iFoam*2][0]*0.25f));
    // get fraction part
    fT = fT-int(fT);
    FLOAT fAppearX = (afStarsPositions[iFoam][0]+0.5f)*fSizeX;
    FLOAT fAppearZ = (afStarsPositions[iFoam][2]+0.5f)*fSizeZ;

    FLOAT fX = fAppearX + afStarsPositions[iFoam][0]*fT*fSpeed*(1.0f-fT*fT/2.0f);
    FLOAT fY = -1.0f+fSpeedY*fT;
    FLOAT fZ = fAppearZ + afStarsPositions[iFoam][2]*fT*fSpeed*(1.0f-fT*fT/2.0f);
    FLOAT3D vPos = vCenter + vX*fX + vY*fY + vZ*fZ;
    FLOAT fFade = CalculateRatio( fT, 0, 1, WATERFALL_FOAM_FADE_IN, WATERFALL_FOAM_FADE_OUT);
    FLOAT fRndRotation = afStarsPositions[iFoam*3][1];
    UBYTE ub = NormFloatToByte( fFade);
    COLOR colStar = RGBToColor( ub, ub, ub);
    Particle_RenderSquare( vPos, fParticleSize*(1.0f+afStarsPositions[iFoam][1]*0.25f), fRndRotation*300*fT, colStar|0xFF);
  }
  // all done
  Particle_Flush();
}

void Particles_EmanatePlane(CEntity *pen, FLOAT fSizeX, FLOAT fSizeY, FLOAT fSizeZ, 
                            FLOAT fParticleSize, FLOAT fAway, FLOAT fSpeed, 
                            enum ParticleTexture ptTexture, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  if( Particle_GetMipFactor()>7.0f) return;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  SetupParticleTexture( ptTexture);
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    FLOAT fT = fNow+afTimeOffsets[iStar];
    // apply time strech
    fT *= 1/(EMANATE_TOTAL_TIME*fSpeed);
    // get fraction part
    fT = fT-int(fT);
    FLOAT fF = fT*(EMANATE_TOTAL_TIME/EMANATE_EXIST_TIME);
    if( fF>1) continue;
    FLOAT fFade;
    if(fF<EMANATE_FADE_IN) fFade=fF*(1/EMANATE_FADE_IN);
    else if (fF>(1.0f-EMANATE_FADE_OUT)) fFade=(1-fF)*(1/EMANATE_FADE_OUT);
    else fFade=1.0f;

    FLOAT fX = (afStarsPositions[iStar][0]+0.5f)*fSizeX*(1+fF*fAway);
    FLOAT fY = fSizeY*fF;
    FLOAT fZ = (afStarsPositions[iStar][2]+0.5f)*fSizeZ*(1+fF*fAway);
    FLOAT3D vPos = vCenter + vX*fX + vY*fY + vZ*fZ;
    
    UBYTE ub = NormFloatToByte( fFade);
    COLOR colStar = RGBToColor( ub, ub, ub);
    Particle_RenderSquare( vPos, fParticleSize, 0, colStar|0xFF);
  }
  // all done
  Particle_Flush();
}

#define CT_FOUNTAIN_TRAIL 3
#define FOUNTAIN_FADE_IN 0.6f
#define FOUNTAIN_FADE_OUT 0.4f
#define FOUNTAIN_TOTAL_TIME 0.6f
void Particles_Fountain( CEntity *pen, FLOAT fSize, FLOAT fHeight,
                        enum ParticleTexture ptTexture, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  SetupParticleTexture( ptTexture);
  CTextureData *pTD = (CTextureData *) _toWaterfallGradient.GetData();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*fHeight;

  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    for( INDEX iTrail=0; iTrail<CT_FOUNTAIN_TRAIL; iTrail++)
    {
      FLOAT fT = fNow+afTimeOffsets[iStar]-iTrail*0.075f;
      // apply time strech
      fT *= 1/FOUNTAIN_TOTAL_TIME;
      // get fraction part
      fT = fT-int(fT);
      FLOAT fFade;
      if (fT>(1.0f-FOUNTAIN_FADE_OUT)) fFade=(1-fT)*(1/FOUNTAIN_FADE_OUT);
      else fFade=1.0f;
      fFade *= (CT_FOUNTAIN_TRAIL-iTrail)*(1.0f/CT_FOUNTAIN_TRAIL);

      FLOAT3D vPos = vCenter +
        vX*(afStarsPositions[iStar][0]*fT*fSize) +
        vY*(fT*fT*-5.0f+(afStarsPositions[iStar][1]*2+4.0f)*1.2f*fT) +
        vZ*(afStarsPositions[iStar][2]*fT*fSize);
    
      COLOR colStar = pTD->GetTexel( FloatToInt(fFade*2048), 0);
      ULONG ulA = FloatToInt( ((colStar&CT_AMASK)>>CT_ASHIFT) * fFade);
      colStar = (colStar&~CT_AMASK) | (ulA<<CT_ASHIFT);
      Particle_RenderSquare( vPos, 0.05f, 0, colStar);
    }
  }
  // all done
  Particle_Flush();
}

void Particles_DamageSmoke( CEntity *pen, FLOAT tmStarted, FLOATaabbox3D boxOwner, FLOAT fDamage)
{
  Particle_PrepareTexture( &_toBulletSmoke, PBT_BLEND);
  INDEX iRnd1 = INDEX( (tmStarted*1000.0f)+pen->en_ulID)%CT_MAX_PARTICLES_TABLE;
  Particle_SetTexturePart( 512, 512, iRnd1%3, 0);

  FLOAT fT = _pTimer->GetLerpedCurrentTick()-tmStarted;
  FLOAT fBoxSize = boxOwner.Size().Length();
  for(INDEX iSmoke=0; iSmoke<2+fDamage*2; iSmoke++)
  {
    INDEX iRnd2 = INDEX(tmStarted*12345.0f+iSmoke+fDamage*10.0f)%(CT_MAX_PARTICLES_TABLE/2);
    FLOAT fLifeTime = 2.0f+(afStarsPositions[iRnd2][0]+0.5f)*2.0f;
    FLOAT fRatio = CalculateRatio(fT, 0, fLifeTime, 0.4f, 0.6f);

    FLOAT fRndAppearX = afStarsPositions[iRnd2][0]*fBoxSize*0.125f;
    FLOAT fRndAppearZ = afStarsPositions[iRnd2][2]*fBoxSize*0.125f;
    FLOAT3D vPos = pen->GetLerpedPlacement().pl_PositionVector;
    vPos(1) += fRndAppearX; 
    vPos(3) += fRndAppearZ; 
    vPos(2) += ((afStarsPositions[iRnd2+4][1]+0.5f)*2.0f+1.5f)*fT+boxOwner.Size()(2)*0.0025f; 

    COLOR col = C_dGRAY|UBYTE(64.0f*fRatio);
    FLOAT fRotation = afStarsPositions[iRnd2+5][0]*360+fT*200.0f*afStarsPositions[iRnd2+3][0];
    FLOAT fSize = 
       0.025f*fDamage+
      (afStarsPositions[iRnd2+6][2]+0.5f)*0.075 +
      (0.15f+(afStarsPositions[iRnd2+2][1]+0.5f)*0.075f*fBoxSize)*fT;
    Particle_RenderSquare( vPos, fSize, fRotation, col);
  }

  // all done
  Particle_Flush();
}

#define RUNNING_DUST_TRAIL_POSITIONS 3*20
void Particles_RunningDust_Prepare(CEntity *pen)
{
  pen->GetLastPositions(RUNNING_DUST_TRAIL_POSITIONS);
}
void Particles_RunningDust(CEntity *pen)
{
  Particle_PrepareTexture( &_toBulletSmoke, PBT_BLEND);
  CLastPositions *plp = pen->GetLastPositions(RUNNING_DUST_TRAIL_POSITIONS);
  FLOAT3D vOldPos = plp->GetPosition(1);
  for(INDEX iPos = 2; iPos<plp->lp_ctUsed; iPos++)
  {
    FLOAT3D vPos = plp->GetPosition(iPos);
    if( (vPos-vOldPos).Length()<1.0f) continue;
    FLOAT tmStarted = _pTimer->CurrentTick()-iPos*_pTimer->TickQuantum;
    INDEX iRnd = INDEX(Abs(vPos(1)*1234.234f+vPos(2)*9834.123f+vPos(3)*543.532f+pen->en_ulID))%(CT_MAX_PARTICLES_TABLE/2);
    if( iRnd&3) continue;

    INDEX iRndTex = iRnd*324561+pen->en_ulID;
    Particle_SetTexturePart( 512, 512, iRndTex%3, 0);

    FLOAT fLifeTime = 2.8f-(afStarsPositions[iRnd][1]+0.5f)*1.0f;

    FLOAT fT = _pTimer->GetLerpedCurrentTick()-tmStarted;
    FLOAT fRatio = CalculateRatio(fT, 0, fLifeTime, 0.1f, 0.25f);

    FLOAT fRndAppearX = afStarsPositions[iRnd][0]*1.0f;
    FLOAT fRndSpeedY = (afStarsPositions[iRnd][1]+0.5f)*0.5f;
    FLOAT fRndAppearZ = afStarsPositions[iRnd][2]*1.0f;
    vPos(1) += fRndAppearX; 
    vPos(2) += (0.5f+fRndSpeedY)*fT; 
    vPos(3) += fRndAppearZ; 

    FLOAT fRndBlend = 8.0f+(afStarsPositions[iRnd*2][1]+0.5f)*64.0f;
    UBYTE ubRndH = UBYTE( (afStarsPositions[iRnd][0]+0.5f)*64);
    UBYTE ubRndS = UBYTE( (afStarsPositions[iRnd][1]+0.5f)*32);
    UBYTE ubRndV = UBYTE( 128+afStarsPositions[iRnd][0]*64.0f);
    COLOR col = HSVToColor(ubRndH,ubRndS,ubRndV)|UBYTE(fRndBlend*fRatio);
    //col=C_RED|CT_OPAQUE;
    FLOAT fRotation = afStarsPositions[iRnd+5][0]*360+fT*50.0f*afStarsPositions[iRnd+3][0];
    FLOAT fSize = 
       0.75f+(afStarsPositions[iRnd+6][2]+0.5f)*0.25 +         // static size
      (0.4f+(afStarsPositions[iRnd+2][1]+0.5f)*0.4f)*fT;    // dinamic size
    Particle_RenderSquare( vPos, fSize, fRotation, col);
    vOldPos=vPos;
  }
  // all done
  Particle_Flush();
}

void Particles_MetalParts( CEntity *pen, FLOAT tmStarted, FLOATaabbox3D boxOwner, FLOAT fDamage)
{
  Particle_PrepareTexture( &_toMetalSprayTexture, PBT_BLEND);
  FLOAT fT = _pTimer->GetLerpedCurrentTick()-tmStarted;
  FLOAT fGA = 30.0f;

  FLOAT fBoxSize = boxOwner.Size().Length();
  for(INDEX iPart=0; iPart<6+fDamage*3.0f; iPart++)
  {
    INDEX iRnd = INDEX(tmStarted*12345.0f+iPart)%CT_MAX_PARTICLES_TABLE;
    FLOAT fLifeTime = 2.0f+(afStarsPositions[iRnd][0]+0.5f)*2.0f;
    FLOAT fRatio = CalculateRatio(fT, 0, fLifeTime, 0.1f, 0.1f);
    Particle_SetTexturePart( 256, 256, ((int(tmStarted*100.0f))%8+iPart)%8, 0);

    FLOAT3D vPos = pen->GetLerpedPlacement().pl_PositionVector;
    vPos(1) += afStarsPositions[iRnd][0]*fT*15;
    vPos(2) += afStarsPositions[iRnd][1]*fT*15-fGA/2.0f*fT*fT+boxOwner.Size()(2)*0.25f; 
    vPos(3) += afStarsPositions[iRnd][2]*fT*15;

    UBYTE ubRndH = UBYTE( 180+afStarsPositions[ int(iPart+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*16);
    UBYTE ubRndS = UBYTE( 12+(afStarsPositions[ int(iPart+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1])*8);
    UBYTE ubRndV = UBYTE( 192+(afStarsPositions[ int(iPart+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*64);
    //ubRndS = 0;
    ubRndV = 255;
    COLOR col = HSVToColor(ubRndH, ubRndS, ubRndV)|UBYTE(255.0f*fRatio);
    FLOAT fRotation = fT*400.0f*afStarsPositions[iRnd][0];
    FLOAT fSize = fBoxSize*0.005f+0.125f+afStarsPositions[iRnd][1]*0.025f;
    Particle_RenderSquare( vPos, fSize, fRotation, col);
  }

  // all done
  Particle_Flush();
}

#define ELECTRICITY_SPARKS_FADE_OUT_TIME 0.4f
#define ELECTRICITY_SPARKS_TOTAL_TIME 1.0f
void Particles_ElectricitySparks( CEntity *pen, FLOAT fTimeAppear, FLOAT fSize, FLOAT fHeight, INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE/2);
  FLOAT fT = _pTimer->GetLerpedCurrentTick()-fTimeAppear;

  Particle_PrepareTexture( &_toElectricitySparks, PBT_BLEND);
  Particle_SetTexturePart( 512, 1024, 0, 0);

  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*fHeight;

  for( INDEX iSpark=0; iSpark<ctParticles; iSpark++)
  {
    FLOAT fFade;
    if (fT>ELECTRICITY_SPARKS_TOTAL_TIME)
    {
      fFade=0;
    }
    else if(fT>ELECTRICITY_SPARKS_FADE_OUT_TIME)
    {
      fFade=-1.0f/(ELECTRICITY_SPARKS_TOTAL_TIME-ELECTRICITY_SPARKS_FADE_OUT_TIME)*(fT-ELECTRICITY_SPARKS_TOTAL_TIME);
    }
    else
    {
      fFade=1.0f;
    }

    FLOAT fTold = fT-0.05f;

#define SPARK_CURVE( time) \
    vCenter + \
      vX*(afStarsPositions[iSpark][0]*time*fSize*3) + \
      vY*(afStarsPositions[iSpark][1]*10.0f*time-(15.0f+afStarsPositions[iSpark*2][1]*15.0f)*time*time) + \
      vZ*(afStarsPositions[iSpark][2]*time*fSize*3);

    FLOAT3D vPosOld = SPARK_CURVE( fTold);
    FLOAT3D vPosNew = SPARK_CURVE( fT);
  
    UBYTE ubR = 224+(afStarsPositions[iSpark][2]+0.5f)*32;
    UBYTE ubG = 224+(afStarsPositions[iSpark][2]+0.5f)*32;
    UBYTE ubB = 160;
    UBYTE ubA = FloatToInt( 255 * fFade);
    COLOR colStar = RGBToColor( ubR, ubG, ubB) | ubA;
    Particle_RenderLine( vPosOld, vPosNew, 0.075f, colStar);
  }
  // all done
  Particle_Flush();
}

void Particles_LavaErupting(CEntity *pen, FLOAT fStretchAll, FLOAT fSize, 
                            FLOAT fStretchX, FLOAT fStretchY, FLOAT fStretchZ, 
                            FLOAT fActivateTime)
{
  FLOAT fT = _pTimer->GetLerpedCurrentTick()-fActivateTime;
  if( fT>10.0f) return;

  Particle_PrepareTexture( &_toLavaEruptingTexture, PBT_ADD);
  INDEX iTexture = ((ULONG)fActivateTime)%3;
  Particle_SetTexturePart( 512, 512, iTexture, 0);
  FLOAT fGA = ((CMovableEntity *)pen)->en_fGravityA;

  INDEX iRnd1 = ((ULONG)fActivateTime)%CT_MAX_PARTICLES_TABLE;
  INDEX iRnd2 = (~(ULONG)fActivateTime)%CT_MAX_PARTICLES_TABLE;

  FLOAT fRndAppearX = afStarsPositions[iRnd2][0]*fStretchAll;
  FLOAT fRndAppearZ = afStarsPositions[iRnd2][1]*fStretchAll;
  FLOAT fRndRotation = afStarsPositions[iRnd2][2];

  FLOAT3D vPos = pen->GetLerpedPlacement().pl_PositionVector;
  vPos(1) += fRndAppearX+afStarsPositions[iRnd1][0]*fT*fStretchX*10;
  vPos(2) += (fStretchY+(fStretchY*0.25f*afStarsPositions[iRnd1][1]))*fT-fGA/2.0f*fT*fT; 
  vPos(3) += fRndAppearZ+afStarsPositions[iRnd1][2]*fT*fStretchZ*10;

  Particle_RenderSquare( vPos, fSize+afStarsPositions[iRnd2][2]*fSize*0.5f, fRndRotation*300*fT, C_WHITE|CT_OPAQUE);

  // all done
  Particle_Flush();
}

#define CT_ATOMIC_TRAIL 32
void Particles_Atomic( CEntity *pen, FLOAT fSize, FLOAT fHeight,
                      enum ParticleTexture ptTexture, INDEX ctEllipses)
{
  ASSERT( ctEllipses<=CT_MAX_PARTICLES_TABLE);
  FLOAT fMipFactor = Particle_GetMipFactor();
  if( fMipFactor>7.0f) return;
  fMipFactor = 2.5f-fMipFactor*0.3f;
  fMipFactor = Clamp(fMipFactor, 0.0f ,1.0f);
  INDEX ctAtomicTrail = fMipFactor*CT_ATOMIC_TRAIL;
  if( ctAtomicTrail<=0) return;
  FLOAT fTrailDelta = 0.075f/fMipFactor;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  SetupParticleTexture( ptTexture);
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*fHeight;

  for( INDEX iEllipse=0; iEllipse<ctEllipses; iEllipse++)
  {
    FLOAT fT = fNow*4+PI*2/3*iEllipse;
    FLOAT angle1 = 2*PI*iEllipse/ctEllipses/*+fT*/;
    FLOAT angle2 = 2.0f/3.0f*PI*iEllipse/ctEllipses;
    FLOAT fSin1= sin(angle1);
    FLOAT fCos1= cos(angle1);
    FLOAT fSin2= sin(angle2);
    FLOAT fCos2= cos(angle2);
    FLOAT3D vA = vX*fSin1+vY*fCos1;
    FLOAT3D vB = vX*fSin2+vZ*fCos2;

    for( INDEX iTrail=0; iTrail<ctAtomicTrail; iTrail++)
    {
      FLOAT3D vPos = vCenter;
      vPos+=vA*(cos((fT-iTrail*fTrailDelta)/*+afStarsPositions[iEllipse][0]*/)*1.0f*fSize);
      vPos+=vB*(sin((fT-iTrail*fTrailDelta)/*+afStarsPositions[iEllipse][0]*/)*1.0f*fSize);
      UBYTE ub = NormFloatToByte( (FLOAT)(ctAtomicTrail-iTrail) / (FLOAT)(ctAtomicTrail));
      COLOR colStar = RGBToColor( ub>>3, ub>>3, ub>>2);
      Particle_RenderSquare( vPos, 0.2f, 0, colStar|0xFF);
    }
  }
  // all done
  Particle_Flush();
}

#define CT_LIGHTNINGS 8
void Particles_Ghostbuster(const FLOAT3D &vSrc, const FLOAT3D &vDst, INDEX ctRays, FLOAT fSize, FLOAT fPower,
                           FLOAT fKneeDivider/*=33.3333333f*/)
{
  Particle_PrepareTexture(&_toGhostbusterBeam, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);
  // get direction vector
  FLOAT3D vZ = vDst-vSrc;
  FLOAT fLen = vZ.Length();
  vZ.Normalize();

  // get two normal vectors
  FLOAT3D vX;
  if (Abs(vZ(2))>0.5) {
    vX = FLOAT3D(1.0f, 0.0f, 0.0f)*vZ;
  } else {
    vX = FLOAT3D(0.0f, 1.0f, 0.0f)*vZ;
  }
  FLOAT3D vY  = vZ*vX;
  const FLOAT fStep = fLen/fKneeDivider;

  for(INDEX iRay = 0; iRay<ctRays; iRay++)
  {
    FLOAT3D v0 = vSrc;
    FLOAT fT = FLOAT(iRay)/ctRays + _pTimer->GetLerpedCurrentTick()/1.5f;
    FLOAT fDT = fT-INDEX(fT);
    FLOAT fFade = 1-fDT*4.0f;

    if( fFade>1 || fFade<=0) continue;
    UBYTE ubFade = NormFloatToByte(fFade*fPower);
    COLOR colFade = RGBToColor( ubFade, ubFade, ubFade);
    for(FLOAT fPos=fStep; fPos<fLen+fStep/2; fPos+=fStep)
    {
      INDEX iOffset = ULONG(fPos*1234.5678f+iRay*103)%32;
      FLOAT3D v1 = vSrc+(vZ*fPos + vX*(0.5f*afStarsPositions[iOffset][0]*fSize) +
                                   vY*(0.5f*afStarsPositions[iOffset][1]*fSize));
      Particle_RenderLine( v0, v1, 0.125f*fSize, colFade|0xFF);
      v0 = v1;
    }
  }
  // all done
  Particle_Flush();
}

void SnapFloat( FLOAT &fDest, FLOAT fStep)
{
  // this must use floor() to get proper snapping of negative values.
  FLOAT fDiv = fDest/fStep;
  FLOAT fRound = fDiv + 0.5f;
  int iSnap = int( floor(fRound));
  FLOAT fRes = iSnap * fStep;
  fDest = fRes;
}

#define RAIN_SOURCE_HEIGHT 16.0f
#define RAIN_SPEED 16.0f
#define RAIN_DROP_TIME (RAIN_SOURCE_HEIGHT/RAIN_SPEED)

void Particles_Rain(CEntity *pen, FLOAT fGridSize, INDEX ctGrids, FLOAT fFactor,
                    CTextureData *ptdRainMap, FLOATaabbox3D &boxRainMap)
{
  FLOAT3D vPos = pen->GetLerpedPlacement().pl_PositionVector;

  vPos(1) -= fGridSize*ctGrids/2;
  vPos(3) -= fGridSize*ctGrids/2;

  SnapFloat( vPos(1), fGridSize);
  SnapFloat( vPos(2), fGridSize);
  SnapFloat( vPos(3), fGridSize);  
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  
  Particle_PrepareTexture(&_toRaindrop, PBT_BLEND);
  Particle_SetTexturePart( 512, 4096, 0, 0);

  FLOAT fMinX = boxRainMap.Min()(1);
  FLOAT fMinY = boxRainMap.Min()(2);
  FLOAT fMinZ = boxRainMap.Min()(3);
  FLOAT fSizeX = boxRainMap.Size()(1);
  FLOAT fSizeY = boxRainMap.Size()(2);
  FLOAT fSizeZ = boxRainMap.Size()(3);
  PIX pixRainMapW = 1;
  PIX pixRainMapH = 1;
  
  if( ptdRainMap != NULL)
  {
    pixRainMapW = ptdRainMap->GetPixWidth();
    pixRainMapH = ptdRainMap->GetPixHeight();
  }

  //INDEX ctDiscarded=0;
  for( INDEX iZ=0; iZ<ctGrids; iZ++)
  {
    INDEX iRndZ = (ULONG(vPos(3)+iZ)) % CT_MAX_PARTICLES_TABLE;
    FLOAT fZOrg = vPos(3) + (iZ+afStarsPositions[iRndZ][2])*fGridSize;
    for( INDEX iX=0; iX<ctGrids; iX++)
    {

      FLOAT fZ = fZOrg;
      INDEX iRndX = (ULONG(vPos(1)+iX)) % CT_MAX_PARTICLES_TABLE;
      FLOAT fX = vPos(1) + (iX+afStarsPositions[iRndX][1])*fGridSize;
      FLOAT fT0 = afStarsPositions[(INDEX(2+Abs(fX)+Abs(fZ))*262147) % CT_MAX_PARTICLES_TABLE][2];

      FLOAT fRatio = (fNow*(1+0.1f*afStarsPositions[iRndZ][2])+fT0)/RAIN_DROP_TIME;
      INDEX iRatio = int(fRatio);
      fRatio = fRatio-iRatio;
      INDEX iRnd2 = iRatio% CT_MAX_PARTICLES_TABLE;
      fX+=afStarsPositions[iRnd2][1];
      fZ+=afStarsPositions[iRnd2][2];
      // stretch to falling time
      FLOAT fY = vPos(2)+RAIN_SOURCE_HEIGHT*(1-fRatio);
      UBYTE ubR = 64+afStarsPositions[(INDEX)fT0*CT_MAX_PARTICLES_TABLE][2]*64;
      COLOR colDrop = RGBToColor(ubR, ubR, ubR)|(UBYTE(fFactor*255.0f));
      FLOAT3D vRender = FLOAT3D( fX, fY, fZ);
      FLOAT fSize = 1.75f+afStarsPositions[(INDEX)fT0*CT_MAX_PARTICLES_TABLE][1];
      if( ptdRainMap != NULL)
      {
        PIX pixX = PIX((vRender(1)-fMinX)/fSizeX*pixRainMapW);
        PIX pixZ = PIX((vRender(3)-fMinZ)/fSizeZ*pixRainMapH);

        if (pixX>=0 && pixX<pixRainMapW 
          &&pixZ>=0 && pixZ<pixRainMapH) {
          COLOR col = ptdRainMap->GetTexel( pixX, pixZ);
          FLOAT fRainMapY = fMinY+((col>>8)&0xFF)/255.0f*fSizeY;

          FLOAT fRainY = vRender(2);
          // if tested raindrop is below ceiling
          if( fRainY<=fRainMapY)
          {
            // don't render it
            continue;
          } else if (fRainY-fSize<fRainMapY) {
            fSize = fRainY-fRainMapY;
          }
        }
      }
      FLOAT3D vTarget = vRender+FLOAT3D(0.0f, -fSize, 0.0f);
      Particle_RenderLine( vRender, vTarget, 0.0125f, colDrop);
    }
  }
  // all done
  Particle_Flush();
}


#define SNOW_SOURCE_HEIGHT 16.0f
#define SNOW_SPEED 1.0f
#define SNOW_DROP_TIME (SNOW_SOURCE_HEIGHT/SNOW_SPEED)

void Particles_Snow( CEntity *pen, FLOAT fGridSize, INDEX ctGrids)
{
  FLOAT3D vPos = pen->GetLerpedPlacement().pl_PositionVector;

  vPos(1) -= fGridSize*ctGrids/2;
  vPos(3) -= fGridSize*ctGrids/2;

  SnapFloat( vPos(1), fGridSize);
  SnapFloat( vPos(2), fGridSize);
  SnapFloat( vPos(3), fGridSize);  
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  
  Particle_PrepareTexture(&_toSnowdrop, PBT_BLEND);
  Particle_SetTexturePart( 512, 512, 0, 0);

  for( INDEX iZ=0; iZ<ctGrids; iZ++)
  {
    INDEX iRndZ = (ULONG(vPos(3)+iZ)) % CT_MAX_PARTICLES_TABLE;
    FLOAT fZ = vPos(3) + (iZ+afStarsPositions[iRndZ][2])*fGridSize;
    for( INDEX iX=0; iX<ctGrids; iX++)
    {
      INDEX iRndX = (ULONG(vPos(1)+iX)) % CT_MAX_PARTICLES_TABLE;
      FLOAT fX = vPos(1) + (iX+afStarsPositions[iRndX][1])*fGridSize;
      FLOAT fT0 = afStarsPositions[(INDEX(2+Abs(fX)+Abs(fZ))*262147) % CT_MAX_PARTICLES_TABLE][2];
      FLOAT fT = (fNow*(1+0.1f*afStarsPositions[iRndZ][2])+fT0);
      fX+=afStarsPositions[int(fT)% CT_MAX_PARTICLES_TABLE][2];
      fZ+=afStarsPositions[int(fT)% CT_MAX_PARTICLES_TABLE][1];
      // get fraction part
      fT /= SNOW_DROP_TIME;
      FLOAT fFade = (fT-int(fT))*SNOW_DROP_TIME;
      // stretch to falling time
      FLOAT fY = vPos(2)+SNOW_SOURCE_HEIGHT-SNOW_SPEED*fFade;
      UBYTE ubR = 128+afStarsPositions[(INDEX)fT0*CT_MAX_PARTICLES_TABLE][2]*64;
      COLOR colDrop = RGBToColor(ubR, ubR, ubR)|CT_OPAQUE;
      FLOAT3D vRender = FLOAT3D( fX, fY, fZ);
      //FLOAT fSize = 1.75f+afStarsPositions[(INDEX)fT0*CT_MAX_PARTICLES_TABLE][1];
      Particle_RenderSquare( vRender, 0.1f, 0, colDrop);
    }
  }
  // all done
  Particle_Flush();
}

#define LIGHTNING_SPEED 2000000.0f
#define LIGHTNING_LIFE_TIME 0.4f
#define LIGHTNING_DEATH_START 0.200f
void RenderOneLightningBranch( FLOAT3D vSrc, FLOAT3D vDst, FLOAT fPath,
                              FLOAT fTimeStart, FLOAT fTimeNow, FLOAT fPower,
                              INDEX iRnd)
{
  // calculate time dependent random factor
  FLOAT fRandomDivider = 1.0f;
  FLOAT3D vZ = vDst-vSrc;
  FLOAT fLen = vZ.Length();
  FLOAT fKneeLen = fLen/10.0f;
  FLOAT3D vRenderDest;
  BOOL bRenderInProgress = TRUE;
  FLOAT fPassedTime = fTimeNow-fTimeStart;
  INDEX ctBranches=0.0f;
  INDEX ctMaxBranches = 3.0f;
  INDEX ctKnees=0.0f;
  
  FLOAT fTimeKiller = Clamp( 
    (-1.0f/(LIGHTNING_LIFE_TIME-LIGHTNING_DEATH_START)*(fPassedTime-LIGHTNING_DEATH_START)+1), 0.0f, 1.0f);

  while(bRenderInProgress)
  {
    // get direction vector
    vZ = vDst-vSrc;
    fLen = vZ.Length();
    ctKnees++;
    if( fLen < fKneeLen)
    {
      vRenderDest = vDst;
      bRenderInProgress = FALSE;
    }
    else
    {
      vZ.Normalize();
      // get two normal vectors
      FLOAT3D vX;
      if (Abs(vZ(2))>0.5)
      {
        vX = FLOAT3D(1.0f, 0.0f, 0.0f)*vZ;
      }
      else
      {
        vX = FLOAT3D(0.0f, 1.0f, 0.0f)*vZ;
      }
      // we found ortonormal vectors
      FLOAT3D vY  = vZ*vX;
    
      FLOAT fAllowRnd = 4.0f/fRandomDivider;
      fRandomDivider+=1.0f;
      vRenderDest = vSrc+
        vZ*fKneeLen +
        vX*(fAllowRnd*afStarsPositions[iRnd][0]*fKneeLen) +
        vY*(fAllowRnd*afStarsPositions[iRnd][1]*fKneeLen);
      // get new rnd index
      iRnd = (iRnd+1) % CT_MAX_PARTICLES_TABLE;
      // see if we will spawn new branch of lightning
      FLOAT fRnd = ((1-ctBranches/ctMaxBranches)*ctKnees)*afStarsPositions[iRnd][0];
      if( (fRnd < 2.0f) && (fPower>0.25f) )
      {
        ctBranches++;
        FLOAT3D vNewDirection = (vRenderDest-vSrc).Normalize();
        FLOAT3D vNewDst = vSrc + vNewDirection*fLen;
        // recurse into new branch
        RenderOneLightningBranch( vSrc, vNewDst, fPath, fTimeStart, fTimeNow, fPower/3.0f, iRnd);
      }
    }

    // calculate color
    UBYTE ubA = UBYTE(fPower*255*fTimeKiller);
    // render line
    Particle_RenderLine( vSrc, vRenderDest, fPower*2, C_WHITE|ubA);
    // add traveled path
    fPath += (vRenderDest-vSrc).Length();
    if( fPath/LIGHTNING_SPEED > fPassedTime)
    {
      bRenderInProgress = FALSE;
    }
    vSrc = vRenderDest;
  }
}

void Particles_Lightning( FLOAT3D vSrc, FLOAT3D vDst, FLOAT fTimeStart)
{
  Particle_PrepareTexture(&_toLightning, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT fTimeNow = _pTimer->GetLerpedCurrentTick();
  // get rnd index
  INDEX iRnd = (INDEX( fTimeStart*100))%CT_MAX_PARTICLES_TABLE;
  RenderOneLightningBranch( vSrc, vDst, 0, fTimeStart, fTimeNow, 1.0f, iRnd);
  
  // all done
  Particle_Flush();
}

#define CT_SANDFLOW_TRAIL 3
#define SANDFLOW_FADE_OUT 0.25f
#define SANDFLOW_TOTAL_TIME 1.0f
void Particles_SandFlow( CEntity *pen, FLOAT fStretchAll, FLOAT fSize, FLOAT fHeight, FLOAT fStartTime, FLOAT fStopTime,
                        INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  SetupParticleTexture( PT_SANDFLOW);
  CTextureData *pTD = (CTextureData *) _toSandFlowGradient.GetData();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  FLOAT fPowerFactor = Clamp((fNow - fStartTime)/2.0f,0.0f,1.0f);
  fPowerFactor *= Clamp(1+(fStopTime-fNow)/2.0f,0.0f,1.0f);
  ctParticles = FLOAT(ctParticles) * fPowerFactor;
  fHeight *= fPowerFactor;
  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    for( INDEX iTrail=0; iTrail<CT_SANDFLOW_TRAIL; iTrail++)
    {
      FLOAT fT = fNow+afTimeOffsets[iStar]/10-iTrail*0.075f;
      // apply time strech
      fT *= 1/SANDFLOW_TOTAL_TIME;
      // get fraction part
      fT = fT-int(fT);
      FLOAT fBirthTime = fNow-(fT*SANDFLOW_TOTAL_TIME);
      if( (fBirthTime<fStartTime) || (fBirthTime>fStopTime+2.0f) ) continue;
      FLOAT fFade;
      if (fT>(1.0f-SANDFLOW_FADE_OUT)) fFade=(1-fT)*(1/SANDFLOW_FADE_OUT);
      else fFade=1.0f;
      fFade *= (CT_SANDFLOW_TRAIL-iTrail)*(1.0f/CT_SANDFLOW_TRAIL);

      FLOAT3D vPos = vCenter +
        vX*(afStarsPositions[iStar][0]*fStretchAll*fPowerFactor+fHeight*fT) +
        vY*(fT*fT*-5.0f+(afStarsPositions[iStar][1]*fPowerFactor*0.1)) +
        vZ*(afStarsPositions[iStar][2]*fPowerFactor*fT*fStretchAll);
    
      COLOR colSand = pTD->GetTexel( FloatToInt(fT*2048), 0);
      ULONG ulA = FloatToInt( ((colSand&CT_AMASK)>>CT_ASHIFT) * fFade);
      colSand = (colSand&~CT_AMASK) | (ulA<<CT_ASHIFT);
      Particle_RenderSquare( vPos, fSize, 0, colSand);
    }
  }
  // all done
  Particle_Flush();
}

#define CT_WATER_FLOW_TRAIL 10
#define WATER_FLOW_FADE_OUT 0.25f
#define WATER_FLOW_TOTAL_TIME 1.0f
void Particles_WaterFlow( CEntity *pen, FLOAT fStretchAll, FLOAT fSize, FLOAT fHeight, FLOAT fStartTime, FLOAT fStopTime,
                     INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  SetupParticleTexture( PT_WATERFLOW);
  CTextureData *pTD = (CTextureData *) _toWaterFlowGradient.GetData();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  FLOAT fPowerFactor = Clamp((fNow - fStartTime)/2.0f,0.0f,1.0f);
  fPowerFactor *= Clamp(1+(fStopTime-fNow)/2.0f,0.0f,1.0f);
  ctParticles = FLOAT(ctParticles) * fPowerFactor;
  fHeight *= fPowerFactor;
  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    for( INDEX iTrail=0; iTrail<CT_WATER_FLOW_TRAIL; iTrail++)
    {
      FLOAT fT = fNow+afTimeOffsets[iStar]/10-iTrail*0.025f;
      // apply time strech
      fT *= 1/WATER_FLOW_TOTAL_TIME;
      // get fraction part
      fT = fT-int(fT);
      FLOAT fBirthTime = fNow-(fT*WATER_FLOW_TOTAL_TIME);
      if( (fBirthTime<fStartTime) || (fBirthTime>fStopTime+2.0f) ) continue;
      FLOAT fFade;
      if (fT>(1.0f-WATER_FLOW_FADE_OUT)) fFade=(1-fT)*(1/WATER_FLOW_FADE_OUT);
      else fFade=1.0f;
      fFade *= (CT_WATER_FLOW_TRAIL-iTrail)*(1.0f/CT_WATER_FLOW_TRAIL);

      FLOAT3D vPos = vCenter +
        vX*(afStarsPositions[iStar][0]*fStretchAll*fPowerFactor+fHeight*fT) +
        vY*(fT*fT*-5.0f+(afStarsPositions[iStar][1]*fPowerFactor*0.1)) +
        vZ*(afStarsPositions[iStar][2]*fPowerFactor*fT*fStretchAll);
    
      COLOR colWater = pTD->GetTexel( FloatToInt(fT*2048), 0);
      ULONG ulA = FloatToInt( ((colWater&CT_AMASK)>>CT_ASHIFT) * fFade);
      colWater = (colWater&~CT_AMASK) | (ulA<<CT_ASHIFT);
      Particle_RenderSquare( vPos, fSize, 0, colWater);
    }
  }
  // all done
  Particle_Flush();
}

#define CT_LAVA_FLOW_TRAIL 8
#define LAVA_FLOW_FADE_OUT 0.25f
#define LAVA_FLOW_TOTAL_TIME 1.25f
void Particles_LavaFlow( CEntity *pen, FLOAT fStretchAll, FLOAT fSize, FLOAT fHeight, FLOAT fStartTime, FLOAT fStopTime,
                     INDEX ctParticles)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  SetupParticleTexture( PT_LAVAFLOW);
  CTextureData *pTD = (CTextureData *) _toLavaFlowGradient.GetData();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  FLOAT fPowerFactor = Clamp((fNow - fStartTime)/2.0f,0.0f,1.0f);
  fPowerFactor *= Clamp(1+(fStopTime-fNow)/2.0f,0.0f,1.0f);
  ctParticles = FLOAT(ctParticles) * fPowerFactor;
  fHeight *= fPowerFactor;
  for( INDEX iStar=0; iStar<ctParticles; iStar++)
  {
    for( INDEX iTrail=0; iTrail<CT_LAVA_FLOW_TRAIL; iTrail++)
    {
      FLOAT fT = fNow+afTimeOffsets[iStar]/10-iTrail*0.035f;
      // apply time strech
      fT *= 1/LAVA_FLOW_TOTAL_TIME;
      // get fraction part
      fT = fT-int(fT);
      FLOAT fBirthTime = fNow-(fT*LAVA_FLOW_TOTAL_TIME);
      if( (fBirthTime<fStartTime) || (fBirthTime>fStopTime+2.0f) ) continue;
      FLOAT fFade;
      if (fT>(1.0f-LAVA_FLOW_FADE_OUT)) fFade=(1-fT)*(1/LAVA_FLOW_FADE_OUT);
      else fFade=1.0f;
      fFade *= (CT_LAVA_FLOW_TRAIL-iTrail)*(1.0f/CT_LAVA_FLOW_TRAIL);

      FLOAT3D vPos = vCenter +
        vX*(afStarsPositions[iStar][0]*fStretchAll*fPowerFactor+fHeight*fT) +
        vY*(fT*fT*-4.0f+(afStarsPositions[iStar][1]*fPowerFactor*0.1)) +
        vZ*(afStarsPositions[iStar][2]*fPowerFactor*fT*fStretchAll);
    
      COLOR colLava = pTD->GetTexel( FloatToInt(fT*2048), 0);
      ULONG ulA = FloatToInt( ((colLava&CT_AMASK)>>CT_ASHIFT) * fFade);
      colLava = (colLava&~CT_AMASK) | (ulA<<CT_ASHIFT);
      Particle_RenderSquare( vPos, fSize, 0, colLava);
    }
  }
  // all done
  Particle_Flush();
}

#define BULLET_SPRAY_FADEOUT_START 0.5f
#define BULLET_SPRAY_TOTAL_TIME 1.25f

#define BULLET_SPRAY_WATER_FADEOUT_START 0.5f
#define BULLET_SPRAY_WATER_TOTAL_TIME 1.5f

#define BULLET_SPARK_FADEOUT_START 0.05f
#define BULLET_SPARK_TOTAL_TIME 0.125f
#define BULLET_SPARK_FADEOUT_LEN (BULLET_SPARK_TOTAL_TIME-BULLET_SPARK_FADEOUT_START)

#define BULLET_SMOKE_FADEOUT_START 0.0f
#define BULLET_SMOKE_TOTAL_TIME 1.5f
#define BULLET_SMOKE_FADEOUT_LEN (BULLET_SMOKE_TOTAL_TIME-BULLET_SMOKE_FADEOUT_START)

void Particles_BulletSpray(CEntity *pen, FLOAT3D vGDir, enum EffectParticlesType eptType,
                           FLOAT tmSpawn, FLOAT3D vDirection)
{
  FLOAT3D vEntity  = pen->GetLerpedPlacement().pl_PositionVector;
  FLOAT fFadeStart = BULLET_SPRAY_FADEOUT_START;
  FLOAT fLifeTotal = BULLET_SPRAY_TOTAL_TIME;
  FLOAT fFadeLen   = fLifeTotal-fFadeStart;
  COLOR colStones = C_WHITE;

  FLOAT fMipFactor = Particle_GetMipFactor();
  FLOAT fDisappear = 1.0f;
  if( fMipFactor>8.0f) return;
  if( fMipFactor>6.0f)
  {
    fDisappear = 1.0f-(fMipFactor-6.0f)/2.0f;
  }

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fT=(fNow-tmSpawn);
  if( fT>fLifeTotal) return;
  INDEX iRnd = INDEX( (tmSpawn*1000.0f)+pen->en_ulID) &63;
  FLOAT fSizeStart;
  FLOAT fSpeedStart;
  FLOAT fConeMultiplier = 1.0f;
  COLOR colSmoke;

  switch( eptType)
  {
    case EPT_BULLET_WATER:
    {
      Particle_PrepareTexture(&_toBulletWater, PBT_BLEND);
      fSizeStart = 0.08f;
      fSpeedStart = 1.75f;
      fConeMultiplier = 0.125f;

      //FLOAT fFadeStart = BULLET_SPRAY_WATER_FADEOUT_START;
      //FLOAT fLifeTotal = BULLET_SPRAY_WATER_TOTAL_TIME;
      //FLOAT fFadeLen   = fLifeTotal-fFadeStart;

      break;
    }
    case EPT_BULLET_SAND:
    {
      colSmoke = 0xFFE8C000;
      Particle_PrepareTexture(&_toBulletSand, PBT_BLEND);
      fSizeStart = 0.15f;
      fSpeedStart = 0.75f;
      break;
    }
    case EPT_BULLET_RED_SAND:
    {
      colSmoke = 0xA0402000;
      colStones = 0x80503000;
      Particle_PrepareTexture(&_toBulletSand, PBT_BLEND);
      fSizeStart = 0.15f;
      fSpeedStart = 0.75f;
      break;
    }
    default:
    {
      colSmoke = C_WHITE;
      Particle_PrepareTexture(&_toBulletStone, PBT_BLEND);
      fSizeStart = 0.05f;
      fSpeedStart = 1.5f;
    }
  }

  FLOAT fGA = 10.0f;

  // render particles
  for( INDEX iSpray=0; iSpray<12*fDisappear; iSpray++)
  {
    Particle_SetTexturePart( 512, 512, iSpray&3, 0);

    FLOAT3D vRandomAngle = FLOAT3D(
      afStarsPositions[ iSpray+iRnd][0]*3.0f* fConeMultiplier,
      (afStarsPositions[ iSpray+iRnd][1]+1.0f)*3.0f,
      afStarsPositions[ iSpray+iRnd][2]*3.0f* fConeMultiplier);
    FLOAT fSpeedRnd = fSpeedStart+afStarsPositions[ iSpray+iRnd*2][2];
    FLOAT3D vPos = vEntity + (vDirection+vRandomAngle)*(fT*fSpeedRnd)+vGDir*(fT*fT*fGA);

    if( (eptType == EPT_BULLET_WATER) && (vPos(2) < vEntity(2)) )
    {
      continue;
    }

    FLOAT fSize = fSizeStart + afStarsPositions[ iSpray*2+iRnd*3][0]/20.0f;
    FLOAT fRotation = fT*500.0f;
    FLOAT fColorFactor = 1.0f;
    if( fT>=fFadeStart)
    {
      fColorFactor = 1-fFadeLen*(fT-fFadeStart);
    }
    UBYTE ubColor = UBYTE(CT_OPAQUE*fColorFactor);
    COLOR col = colStones|ubColor;
    Particle_RenderSquare( vPos, fSize, fRotation, col);
  }
  Particle_Flush();
  
  //---------------------------------------
  if( (fT<BULLET_SPARK_TOTAL_TIME) && (eptType != EPT_BULLET_WATER) && (eptType != EPT_BULLET_UNDER_WATER) )
  {
    // render spark lines
    Particle_PrepareTexture(&_toBulletSpark, PBT_ADD);
    for( INDEX iSpark=0; iSpark<8*fDisappear; iSpark++)
    {
      FLOAT3D vRandomAngle = FLOAT3D(
        afStarsPositions[ iSpark+iRnd][0]*0.75f,
        afStarsPositions[ iSpark+iRnd][1]*0.75f,
        afStarsPositions[ iSpark+iRnd][2]*0.75f);
      FLOAT3D vPos0 = vEntity + (vDirection+vRandomAngle)*(fT+0.00f)*12.0f;
      FLOAT3D vPos1 = vEntity + (vDirection+vRandomAngle)*(fT+0.05f)*12.0f;
      FLOAT fColorFactor = 1.0f;
      if( fT>=BULLET_SPARK_FADEOUT_START)
      {
        fColorFactor = 1-BULLET_SPARK_FADEOUT_LEN*(fT-BULLET_SPARK_FADEOUT_START);
      }
      UBYTE ubColor = UBYTE(CT_OPAQUE*fColorFactor);
      COLOR col = RGBToColor(ubColor,ubColor,ubColor)|CT_OPAQUE;
      Particle_RenderLine( vPos0, vPos1, 0.05f, col);
    }
    Particle_Flush();
  }

  //---------------------------------------
  if( (fT<BULLET_SMOKE_TOTAL_TIME) && (eptType != EPT_BULLET_WATER) && (eptType != EPT_BULLET_UNDER_WATER) )
  {
    // render smoke
    Particle_PrepareTexture( &_toBulletSmoke, PBT_BLEND);
    Particle_SetTexturePart( 512, 512, iRnd%3, 0);
    FLOAT3D vPos = vEntity - vGDir*(afStarsPositions[iRnd][0]*2.0f+1.5f)*fT;
    FLOAT fColorFactor = (BULLET_SMOKE_TOTAL_TIME-fT) / BULLET_SMOKE_TOTAL_TIME /
                         (afStarsPositions[iRnd+1][0]*2+4.0f);
    FLOAT fRotation = afStarsPositions[iRnd][1]*300*fT;
    FLOAT fSize = (afStarsPositions[iRnd][2]+0.5f)*1.0f*fT+0.25f;
    UBYTE ubAlpha = UBYTE(CT_OPAQUE*fColorFactor*fDisappear);
    Particle_RenderSquare( vPos, fSize, fRotation, colSmoke|ubAlpha);
    Particle_Flush();
  }
}

void MakeBaseFromVector(const FLOAT3D &vY, FLOAT3D &vX, FLOAT3D &vZ)
{
  // if the plane is mostly horizontal
  if (Abs(vY(2))>0.5) {
    // use cross product of +x axis and plane normal as +s axis
    vX = FLOAT3D(1.0f, 0.0f, 0.0f)*vY;
  // if the plane is mostly vertical
  } else {
    // use cross product of +y axis and plane normal as +s axis
    vX = FLOAT3D(0.0f, 1.0f, 0.0f)*vY;
  }
  // make +s axis normalized
  vX.Normalize();

  // use cross product of plane normal and +s axis as +t axis
  vZ = vX*vY;
  vZ.Normalize();
}

void Particles_EmptyShells( CEntity *pen, ShellLaunchData *asldData)
{
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fGA = ((CMovableEntity *)pen)->en_fGravityA;
  FLOAT3D vGDir = ((CMovableEntity *)pen)->en_vGravityDir;
  INDEX iRow, iColumn;

  for( INDEX iShell=0; iShell<MAX_FLYING_SHELLS; iShell++)
  {
    ShellLaunchData &sld = asldData[iShell];
    FLOAT tmLaunch = sld.sld_tmLaunch;
    Particle_PrepareTexture(&_toPlayerParticles, PBT_BLEND);

    FLOAT fT = tmNow-tmLaunch;
    switch( sld.sld_estType)
    {
      case ESL_BULLET:
      {
        FLOAT fLife = 1.5f;
        if( tmNow > (tmLaunch+fLife) ) continue;
        FLOAT fTRatio = fT/fLife;
        INDEX iFrame = INDEX( fTRatio*16*8)%16;
        iRow = iFrame/4;
        iColumn = iFrame%4;
        Particle_SetTexturePart( 256, 256, iColumn, iRow);
        FLOAT3D vPos = (sld.sld_vPos)+(sld.sld_vSpeed*fT)+(vGDir*(fT*fT*fGA/2.0f));
        Particle_RenderSquare( vPos, 0.05f, 0, C_WHITE|CT_OPAQUE);
        break;
      }
      case ESL_SHOTGUN:
      {
        FLOAT fLife = 1.5f;
        if( tmNow > (tmLaunch+fLife) ) continue;
        FLOAT fTRatio = fT/fLife;
        INDEX iFrame = INDEX( fTRatio*16*8)%16;
        iRow = 4+iFrame/4;
        iColumn = iFrame%4;
        Particle_SetTexturePart( 256, 256, iColumn, iRow);
        FLOAT3D vPos = (sld.sld_vPos)+(sld.sld_vSpeed*fT)+(vGDir*(fT*fT*fGA/2.0f));
        Particle_RenderSquare( vPos, 0.05f, 0, C_WHITE|CT_OPAQUE);
        break;
      }
      case ESL_BUBBLE:
      {
        INDEX iRnd = (INDEX(tmLaunch*1234))%CT_MAX_PARTICLES_TABLE;
        FLOAT fLife = 4.0f;
        if( tmNow > (tmLaunch+fLife) ) continue;
        Particle_SetTexturePart( 512, 512, 2, 0);
        
        FLOAT3D vX, vZ;
        MakeBaseFromVector( sld.sld_vUp, vX, vZ);

        FLOAT fZF = sin( afStarsPositions[iRnd+2][0]*PI);
        FLOAT fXF = cos( afStarsPositions[iRnd+2][0]*PI);

        FLOAT fAmpl = ClampUp( fT+afStarsPositions[iRnd+1][1]+0.5f, 2.0f)/64;
        FLOAT fFormulae =  fAmpl * sin(afStarsPositions[iRnd][1]+fT*afStarsPositions[iRnd][2]*2);

        FLOAT fColorFactor = 1.0f;
        if( fT>fLife/2)
        {
          fColorFactor = -fT+fLife;
        }
        UBYTE ubAlpha = UBYTE(CT_OPAQUE*fColorFactor);
        ubAlpha = CT_OPAQUE;
        FLOAT3D vSpeedPower = sld.sld_vSpeed/(1.0f+fT*fT);
        
        FLOAT3D vPos = sld.sld_vPos + 
          vX*fFormulae*fXF+
          vZ*fFormulae*fZF+
          sld.sld_vUp*fT/4.0f*(0.8f+afStarsPositions[iRnd][1]/8.0f)+
          vSpeedPower*fT;
        FLOAT fSize = 0.02f + afStarsPositions[iRnd+3][1]*0.01f;
        Particle_RenderSquare( vPos, fSize, 0, C_WHITE|ubAlpha);
        break;
      }
      case ESL_SHOTGUN_SMOKE:
      {
        FLOAT fLife = 1.0f;
        if( fT<fLife)
        {
          // render smoke
          INDEX iRnd = (INDEX(tmLaunch*1234))%CT_MAX_PARTICLES_TABLE;
          //FLOAT fTRatio = fT/fLife;
          INDEX iColumn = 4+INDEX( iShell)%4;
          Particle_SetTexturePart( 256, 256, iColumn, 2);

          FLOAT3D vPos = sld.sld_vPos + sld.sld_vUp*(afStarsPositions[iRnd][0]*2.0f+1.5f)*fT + sld.sld_vSpeed*fT/(1+fT*fT);
          FLOAT fColorFactor = (fLife-fT)/fLife/(afStarsPositions[iRnd+1][0]*2+4.0f);
          FLOAT fRotation = afStarsPositions[iShell][1]*300*fT;
          FLOAT fSize = 0.25f+fT;
          UBYTE ubAlpha = UBYTE(CT_OPAQUE*fColorFactor);
          COLOR colSmoke = C_WHITE;
          Particle_RenderSquare( vPos, fSize, fRotation, colSmoke|ubAlpha);
        }
        break;
      }
      case ESL_BULLET_SMOKE:
      {
        FLOAT fLife = 1.0f;
        if( fT<fLife && fT>0.0f)
        {
          // render smoke
          INDEX iRnd = (INDEX(tmLaunch*1234))%CT_MAX_PARTICLES_TABLE;
          //FLOAT fTRatio = fT/fLife;
          INDEX iColumn = 4+INDEX( iShell)%4;
          Particle_SetTexturePart( 256, 256, iColumn, 2);

          FLOAT3D vPos = sld.sld_vPos + sld.sld_vUp*(afStarsPositions[iRnd][0]/2.0f+0.5f)*fT + sld.sld_vSpeed*fT/(1+fT*fT);
          FLOAT fColorFactor = (fLife-fT)/fLife/(afStarsPositions[iRnd+1][0]*2+4.0f);
          FLOAT fRotation = afStarsPositions[iShell][1]*200*fT;
          FLOAT fSize = (0.0125f+fT/(5.0f+afStarsPositions[iRnd+1][0]*2.0f))*sld.sld_fSize;
          UBYTE ubAlpha = UBYTE(CT_OPAQUE*ClampUp(fColorFactor*sld.sld_fSize, 1.0f));
          COLOR colSmoke = C_WHITE;
          Particle_RenderSquare( vPos, fSize, fRotation, colSmoke|ubAlpha);
        }
        break;
      }
      case ESL_COLT_SMOKE:
      {
        FLOAT fLife = 1.0f;
        if( fT<fLife && fT>0.0f)
        {
          CPlayer &plr = (CPlayer&)*pen;
          CPlacement3D plPipe;
          plr.GetLerpedWeaponPosition(sld.sld_vPos, plPipe);
          FLOATmatrix3D m;
          MakeRotationMatrixFast(m, plPipe.pl_OrientationAngle);
          FLOAT3D vUp( m(1,2), m(2,2), m(3,2));

          INDEX iRnd = (INDEX(tmLaunch*1234))%CT_MAX_PARTICLES_TABLE;
          //FLOAT fTRatio = fT/fLife;
          INDEX iColumn = 4+INDEX( iShell)%4;
          Particle_SetTexturePart( 256, 256, iColumn, 2);

          FLOAT3D vPos = plPipe.pl_PositionVector+vUp*(afStarsPositions[iRnd][0]/4.0f+0.3f)*fT;
          FLOAT fColorFactor = (fLife-fT)/fLife/(afStarsPositions[iRnd+1][0]*2+4.0f);
          FLOAT fRotation = afStarsPositions[iShell][1]*500*fT;
          FLOAT fSize = 0.0025f+fT/(10.0f+(afStarsPositions[iRnd+1][0]+0.5f)*10.0f);
          UBYTE ubAlpha = UBYTE(CT_OPAQUE*fColorFactor);
          COLOR colSmoke = C_WHITE;
          Particle_RenderSquare( vPos, fSize, fRotation, colSmoke|ubAlpha);
        }
        break;
      }
    }
  }
  Particle_Flush();
}

#define FADE_IN_LENGHT 1.0f
#define FADE_OUT_LENGHT 1.5f
#define FADE_IN_START 0.0f
#define SPIRIT_SPIRAL_START 1.0f
#define FADE_OUT_START 1.75f

#define FADE_IN_END (FADE_IN_START+FADE_IN_LENGHT)
#define FADE_OUT_END (FADE_OUT_START+FADE_OUT_LENGHT)

void Particles_Death(CEntity *pen, TIME tmStart)
{
  FLOAT fMipFactor = Particle_GetMipFactor();
  BOOL bVisible = pen->en_pmoModelObject->IsModelVisible( fMipFactor);
  if( !bVisible) return;

  FLOAT fTime = _pTimer->GetLerpedCurrentTick()-tmStart;
  // don't render particles before fade in and after fadeout
  if( (fTime<FADE_IN_START) || (fTime>FADE_OUT_END)) {
    return;
  }
  FLOAT fPowerTime = pow(fTime-SPIRIT_SPIRAL_START, 2.5f);

  // fill array with absolute vertices of entity's model and its attached models
  pen->GetModelVerticesAbsolute(avVertices, 0.05f, fMipFactor); 

  // get entity position and orientation
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;
  
  SetupParticleTexture( PT_STAR07);

  // calculate color factor (for fade in/out)
  FLOAT fColorFactor = 1.0f;
  if( (fTime>=FADE_IN_START) && (fTime<=FADE_IN_END))
  {
    fColorFactor = 1/FADE_IN_LENGHT*(fTime-FADE_IN_START);
  }
  else if( (fTime>=FADE_OUT_START) && (fTime<=FADE_OUT_END))
  {
    fColorFactor = -1/FADE_OUT_LENGHT*(fTime-FADE_OUT_END);
  }

  UBYTE ubColor = UBYTE(CT_OPAQUE*fColorFactor);
  COLOR col = RGBToColor(ubColor,ubColor,ubColor)|CT_OPAQUE;

  INDEX ctVtx = avVertices.Count();
  FLOAT fSpeedFactor = 1.0f/ctVtx;

  // get corp size
  FLOATaabbox3D box;
  pen->en_pmoModelObject->GetCurrentFrameBBox(box);
  FLOAT fHeightStretch = box.Size()(2);

  FLOAT fStep = ClampDn( fMipFactor, 1.0f);
  for( FLOAT fVtx=0.0f; fVtx<ctVtx; fVtx+=fStep)
  {
    INDEX iVtx = INDEX( fVtx);
    FLOAT fF;
    if (fTime<SPIRIT_SPIRAL_START) {
      fF = 0.0f;
    } else {
      fF = fPowerTime*(1+iVtx*fSpeedFactor)*4.0f;
    }
    FLOAT fStretch = 1/ClampDn(fF*0.2f, 1.0f);
    FLOAT3D vPos = avVertices[iVtx];
    vPos-=vCenter;
    FLOAT fX = (vPos%vX)*fStretch;
    FLOAT fY = (vPos%vY)*fStretch;
    FLOAT fZ = (vPos%vZ)*fStretch;
    FLOAT fA = fF*2.0f;
    vPos = 
      vX*(fX*cos(fA)-fZ*sin(fA))+
      vZ*(fX*sin(fA)+fZ*cos(fA))+
      vY*(fY+fF*fHeightStretch*0.075f)+
      vCenter;

    Particle_RenderSquare( vPos, 0.1f*fColorFactor, 0, col);
  }

  // flush array
  avVertices.PopAll();
  // all done
  Particle_Flush();
}

#define APPEAR_IN_LENGHT 2.0f
#define APPEAR_OUT_LENGHT 5.0f
#define APPEAR_IN_START 0.0f
#define APPEAR_OUT_START 5.0f

#define APPEAR_IN_END (APPEAR_IN_START+APPEAR_IN_LENGHT)
#define APPEAR_OUT_END (APPEAR_OUT_START+APPEAR_OUT_LENGHT)

void Particles_Appearing(CEntity *pen, TIME tmStart)
{
  FLOAT fMipFactor = Particle_GetMipFactor();
  BOOL bVisible = pen->en_pmoModelObject->IsModelVisible( fMipFactor);
  if( !bVisible) return;

  FLOAT fTime = _pTimer->GetLerpedCurrentTick()-tmStart;
  // don't render particles before fade in and after fadeout
  if( (fTime<APPEAR_IN_START) || (fTime>APPEAR_OUT_END)) {
    return;
  }
  //FLOAT fPowerTime = pow(fTime-SPIRIT_SPIRAL_START, 2.5f);

  // fill array with absolute vertices of entity's model and its attached models
  pen->GetModelVerticesAbsolute(avVertices, 0.05f, fMipFactor); 

  // get entity position and orientation
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  //FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;
  
  SetupParticleTexture( PT_STAR07);

  // calculate color factor (for fade in/out)
  FLOAT fColorFactor = 1.0f;
  if( (fTime>=APPEAR_IN_START) && (fTime<=APPEAR_IN_END))
  {
    fColorFactor = 1/APPEAR_IN_LENGHT*(fTime-APPEAR_IN_START);
  }
  else if( (fTime>=APPEAR_OUT_START) && (fTime<=APPEAR_OUT_END))
  {
    fColorFactor = -1/APPEAR_OUT_LENGHT*(fTime-APPEAR_OUT_END);
  }

  UBYTE ubColor = UBYTE(CT_OPAQUE*fColorFactor);
  COLOR col = RGBToColor(ubColor,ubColor,ubColor)|CT_OPAQUE;

  INDEX ctVtx = avVertices.Count();
  //FLOAT fSpeedFactor = 1.0f/ctVtx;

  // get corp size
  FLOATaabbox3D box;
  pen->en_pmoModelObject->GetCurrentFrameBBox(box);
  //FLOAT fHeightStretch = box.Size()(2);

  FLOAT fStep = ClampDn( fMipFactor, 1.0f);
  for( FLOAT fVtx=0.0f; fVtx<ctVtx; fVtx+=fStep)
  {
    INDEX iVtx = INDEX( fVtx);
//    FLOAT fF;
//    if (fTime<SPIRIT_SPIRAL_START) {
//      fF = 0.0f;
//    } else {
//      fF = fPowerTime*(1+iVtx*fSpeedFactor)*4.0f;
//    }
//    FLOAT fStretch = 1/ClampDn(fF*0.2f, 1.0f);
    FLOAT3D vPos = avVertices[iVtx];
/*    vPos-=vCenter;
    FLOAT fX = (vPos%vX)*fStretch;
    FLOAT fY = (vPos%vY)*fStretch;
    FLOAT fZ = (vPos%vZ)*fStretch;
    FLOAT fA = fF*2.0f;
    vPos = 
      vX*(fX*cos(fA)-fZ*sin(fA))+
      vZ*(fX*sin(fA)+fZ*cos(fA))+
      vY*(fY+fF*fHeightStretch*0.075f)+
      vCenter;
  */
    Particle_RenderSquare( vPos, 0.1f*fColorFactor, 0, col);
  }

  // flush array
  avVertices.PopAll();
  // all done
  Particle_Flush();
}


#define BLOOD_SPRAYS 16
#define BLOOD_SPRAY_SPEED_MAX (3.0f*fDamagePower)
#define BLOOD_SPRAY_SPEED_MIN (1.0f*fDamagePower)
#define BLOOD_SPRAY_TOTAL_TIME (0.6f+fDamagePower/1.0f)
#define BLOOD_SPRAY_FADE_IN_END 0.1f
#define BLOOD_SPRAY_FADE_OUT_START 0.2f

// spray some blood from shot place
void Particles_BloodSpray(enum SprayParticlesType sptType, CEntity *penSpray, FLOAT3D vGDir, FLOAT fGA,
                          FLOATaabbox3D boxOwner, FLOAT3D vSpilDirection, FLOAT tmStarted, FLOAT fDamagePower)
{
  FLOAT3D vWoundPos = penSpray->GetLerpedPlacement().pl_PositionVector;
  FLOAT fBoxSize = boxOwner.Size().Length()*0.1f;
  FLOAT fEnemySizeModifier = (fBoxSize-0.2)/1.0f+1.0f;
  FLOAT fRotation = 0.0f;

  // readout blood type
  const INDEX iBloodType = GetSP()->sp_iBlood;

  // determine time difference
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  // prepare texture
  switch(sptType) {
    case SPT_BLOOD:
    case SPT_SLIME:
    {
      if( iBloodType<1) return;
      if( iBloodType==3) Particle_PrepareTexture( &_toFlowerSprayTexture, PBT_BLEND);
      else               Particle_PrepareTexture( &_toBloodSprayTexture,  PBT_BLEND);
      break;
    }
    case SPT_BONES:
    {
      Particle_PrepareTexture( &_toBonesSprayTexture, PBT_BLEND);
      break;
    }
    case SPT_FEATHER:
    {
      Particle_PrepareTexture( &_toFeatherSprayTexture, PBT_BLEND);
      fDamagePower*=2.0f;
      break;
    }
    case SPT_STONES:
    {
      Particle_PrepareTexture( &_toStonesSprayTexture, PBT_BLEND);
      fDamagePower*=3.0f;
      break;
    }
    case SPT_WOOD:
    {
      Particle_PrepareTexture( &_toWoodSprayTexture, PBT_BLEND);
      fDamagePower*=5.0f;
      break;
    }
    case SPT_SMALL_LAVA_STONES:
    {
      Particle_PrepareTexture( &_toLavaSprayTexture, PBT_BLEND);
      fDamagePower *= 0.75f;
      break;
    }
    case SPT_LAVA_STONES:
    {
      Particle_PrepareTexture( &_toLavaSprayTexture, PBT_BLEND);
      fDamagePower *=3.0f;
      break;
    }
    case SPT_BEAST_PROJECTILE_SPRAY:
    {
      Particle_PrepareTexture( &_toBeastProjectileSprayTexture, PBT_BLEND);
      fDamagePower*=3.0f;
      break;
    }
    case SPT_ELECTRICITY_SPARKS:
    {
      Particle_PrepareTexture( &_toMetalSprayTexture, PBT_BLEND);
      break;
    }
    default: ASSERT(FALSE);
      return;
    }

  FLOAT fT=(fNow-tmStarted);
  for( INDEX iSpray=0; iSpray<BLOOD_SPRAYS; iSpray++)
  {
    if( (sptType==SPT_FEATHER) && (iSpray==BLOOD_SPRAYS/2) )
    {
      Particle_Flush();
      if( iBloodType==3) Particle_PrepareTexture( &_toFlowerSprayTexture, PBT_BLEND);
      else               Particle_PrepareTexture( &_toBloodSprayTexture,  PBT_BLEND);
      fDamagePower/=2.0f;
    }

    Particle_SetTexturePart( 256, 256, ((int(tmStarted*100.0f))%8+iSpray)%8, 0);

    FLOAT fFade, fSize;
    // apply fade
    if( fT<BLOOD_SPRAY_FADE_IN_END)
    {
      fSize=fT/BLOOD_SPRAY_FADE_IN_END;
      fFade = 1.0f;
    }
    else if (fT>BLOOD_SPRAY_FADE_OUT_START)
    {
      fSize=(-1/(BLOOD_SPRAY_TOTAL_TIME-BLOOD_SPRAY_FADE_OUT_START))*(fT-BLOOD_SPRAY_TOTAL_TIME);
      fFade = fSize;
    }
    else if( fT>BLOOD_SPRAY_TOTAL_TIME)
    {
      fSize=0.0f;
      fFade =0.0f;
    }
    else
    {
      fSize=1.0f;
      fFade = fSize;
    }
    FLOAT fMipFactor = Particle_GetMipFactor();
    FLOAT fMipSizeAffector = Clamp( fMipFactor/4.0f, 0.05f, 1.0f);
    fSize *= fMipSizeAffector*fDamagePower*fEnemySizeModifier;

    FLOAT3D vRandomAngle = FLOAT3D(
      afStarsPositions[ iSpray][0]*1.75f,
      (afStarsPositions[ iSpray][1]+1.0f)*1.0f,
      afStarsPositions[ iSpray][2]*1.75f);
    FLOAT fSpeedModifier = afStarsPositions[ iSpray+BLOOD_SPRAYS][0]*0.5f;

    FLOAT fSpeed = BLOOD_SPRAY_SPEED_MIN+(BLOOD_SPRAY_TOTAL_TIME-fT)/BLOOD_SPRAY_TOTAL_TIME;
    FLOAT3D vPos = vWoundPos + (vSpilDirection+vRandomAngle)*(fT*(fSpeed+fSpeedModifier))+vGDir*(fT*fT*fGA/4.0f);
  
    UBYTE ubAlpha = UBYTE(CT_OPAQUE*fFade);
    FLOAT fSizeModifier = afStarsPositions[ int(iSpray+tmStarted*50)%CT_MAX_PARTICLES_TABLE][1]*0.5+1.0f;

    COLOR col = C_WHITE|CT_OPAQUE;
  // prepare texture
  switch(sptType) {
    case SPT_BLOOD:
    {
      UBYTE ubRndCol = UBYTE( 128+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*64);
      if( iBloodType==2) col = RGBAToColor( ubRndCol, 0, 0, ubAlpha);
      if( iBloodType==1) col = RGBAToColor( 0, ubRndCol, 0, ubAlpha);
      break;
    }
    case SPT_SLIME:
    {
      UBYTE ubRndCol = UBYTE( 128+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*64);
      if( iBloodType!=3) col = RGBAToColor(0, ubRndCol, 0, ubAlpha);
      break;
    }
    case SPT_BONES:
    {
      UBYTE ubRndH = UBYTE( 8+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*16);
      UBYTE ubRndS = UBYTE( 96+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*64);
      UBYTE ubRndV = UBYTE( 64+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*64);
      col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
      fSize/=1.5f;
      break;
    }
    case SPT_FEATHER:
    {
      if(iSpray>=BLOOD_SPRAYS/2)
      {
        UBYTE ubRndCol = UBYTE( 128+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*64);
        if( iBloodType==2) col = RGBAToColor( ubRndCol, 0, 0, ubAlpha);
        if( iBloodType==1) col = RGBAToColor( 0, ubRndCol, 0, ubAlpha);
      }
      else
      {
        UBYTE ubRndH = UBYTE( 32+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*16);
        //UBYTE ubRndS = UBYTE( 127+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*128);
        UBYTE ubRndV = UBYTE( 159+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*192);
        col = HSVToColor(ubRndH, 0, ubRndV)|ubAlpha;
        fSize/=2.0f;
        fRotation = fT*200.0f;
      }
      break;
    }
    case SPT_STONES:
    {
      UBYTE ubRndH = UBYTE( 24+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*16);
      UBYTE ubRndS = UBYTE( 32+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*64);
      UBYTE ubRndV = UBYTE( 196+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*128);
      col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
      fSize*=0.10f;
      fRotation = fT*200.0f;
      break;
    }
    case SPT_WOOD:
    {
      UBYTE ubRndH = UBYTE( 16+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*16);
      UBYTE ubRndS = UBYTE( 96+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*32);
      UBYTE ubRndV = UBYTE( 96+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*96);
      col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
      fSize*=0.15f;
      fRotation = fT*300.0f;
      break;
    }
    case SPT_LAVA_STONES:
    case SPT_SMALL_LAVA_STONES:
    {
      col = C_WHITE|ubAlpha;
      fSize/=12.0f;
      fRotation = fT*200.0f;
      break;
    }
    case SPT_BEAST_PROJECTILE_SPRAY:
    {
      col = C_WHITE|ubAlpha;
      fSize/=12.0f;
      fRotation = fT*200.0f;
      break;
    }
    case SPT_ELECTRICITY_SPARKS:
    {
      UBYTE ubRndH = UBYTE( 180+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*16);
      UBYTE ubRndS = UBYTE( 32+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*16);
      UBYTE ubRndV = UBYTE( 192+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*64);
      ubRndS = 0;
      ubRndV = 255;
      col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
      fSize/=32.0f;
      fRotation = fT*200.0f;
      break;
    }
    }
    Particle_RenderSquare( vPos, 0.25f*fSize*fSizeModifier, fRotation, col);
  }

  // all done
  Particle_Flush();
}

// spray some stones along obelisk
void Particles_DestroyingObelisk(CEntity *penSpray, FLOAT tmStarted)
{
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fT=(fNow-tmStarted);
  Particle_PrepareTexture( &_toStonesSprayTexture, PBT_BLEND);

  FLOAT fTotalTime = 10.0f;
  FLOAT fFadeOutStart = 7.5f;
  FLOAT fFadeInEnd = 1.0f;
  FLOAT3D vG = FLOAT3D(0.0f,-20.0f,0.0f);

  for( INDEX iStone=0; iStone<128; iStone++)
  {
    INDEX idx = int(iStone+tmStarted*33)%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vSpeed = FLOAT3D(
      afStarsPositions[ idx][0],
      afStarsPositions[ idx][1],
      afStarsPositions[ idx][2]);
    vSpeed(2) += 0.25f;
    vSpeed *= 50.0f;
    
    // calculate position
    FLOAT3D vPos = penSpray->GetPlacement().pl_PositionVector +
      vSpeed*fT +
      vG*fT*fT;
    vPos(2) += (afStarsPositions[ int(iStone+tmStarted*100)%CT_MAX_PARTICLES_TABLE][1]+0.5)*116.0f;

    FLOAT fFade;
    // apply fade
    if( fT<fFadeInEnd)
    {
      fFade = 1.0f;
    }
    else if (fT>fFadeOutStart)
    {
      fFade = (-1/(fTotalTime-fFadeOutStart))*(fT-fTotalTime);
    }
    else if( fT>fTotalTime)
    {
      fFade =0.0f;
    }
    else
    {
      fFade = 1.0f;
    }

    UBYTE ubRndH = UBYTE( 16+afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*8);
    UBYTE ubRndS = UBYTE( 96+(afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*64);
    UBYTE ubRndV = UBYTE( 128+(afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*64);
    UBYTE ubAlpha = UBYTE(CT_OPAQUE*fFade);
    COLOR col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
    
    FLOAT fSize=(afStarsPositions[ int(iStone+tmStarted*100)%CT_MAX_PARTICLES_TABLE][2]+1.0f)*1.5f;
    FLOAT fRotation = fT*200.0f;

    Particle_SetTexturePart( 256, 256, ((int(tmStarted*100.0f))%8+iStone)%8, 0);
    Particle_RenderSquare( vPos, fSize, fRotation, col);
  }
  // all done
  Particle_Flush();
}

// spray some stones along pylon
void Particles_DestroyingPylon(CEntity *penSpray, FLOAT3D vDamageDir, FLOAT tmStarted)
{
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fT=(fNow-tmStarted);
  Particle_PrepareTexture( &_toStonesSprayTexture, PBT_BLEND);

  FLOAT fTotalTime = 10.0f;
  FLOAT fFadeOutStart = 7.5f;
  FLOAT fFadeInEnd = 1.0f;
  FLOAT3D vG = FLOAT3D(0.0f,-20.0f,0.0f);
  const FLOATmatrix3D &m = penSpray->GetRotationMatrix();

  for( INDEX iStone=0; iStone<128; iStone++)
  {
    INDEX idx = int(iStone+tmStarted*33)%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vSpeed = vDamageDir+FLOAT3D(
      afStarsPositions[ idx][0],
      afStarsPositions[ idx][1],
      afStarsPositions[ idx][2]);
    vSpeed *= 50.0f;
    
    // calculate position
    FLOAT3D vPos = penSpray->GetPlacement().pl_PositionVector +
      vSpeed*fT*m +
      vG*fT*fT;
    FLOAT3D vOffset = FLOAT3D(0.0f,0.0f,0.0f);
    vOffset(1) = (afStarsPositions[ int(iStone+tmStarted*100)%CT_MAX_PARTICLES_TABLE][1])*32.0f;
    vOffset(2) = (afStarsPositions[ int(iStone+tmStarted*100)%CT_MAX_PARTICLES_TABLE][2]+0.5)*56.0f;
    vPos += vOffset*m;

    FLOAT fFade;
    // apply fade
    if( fT<fFadeInEnd)
    {
      fFade = 1.0f;
    }
    else if (fT>fFadeOutStart)
    {
      fFade = (-1/(fTotalTime-fFadeOutStart))*(fT-fTotalTime);
    }
    else if( fT>fTotalTime)
    {
      fFade =0.0f;
    }
    else
    {
      fFade = 1.0f;
    }

    UBYTE ubRndH = UBYTE( 16+afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*8);
    UBYTE ubRndS = UBYTE( 96+(afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*64);
    UBYTE ubRndV = UBYTE( 128+(afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*64);
    UBYTE ubAlpha = UBYTE(CT_OPAQUE*fFade);
    COLOR col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
    
    FLOAT fSize=(afStarsPositions[ int(iStone+tmStarted*100)%CT_MAX_PARTICLES_TABLE][2]+1.0f)*1.5f;
    FLOAT fRotation = fT*200.0f;

    Particle_SetTexturePart( 256, 256, ((int(tmStarted*100.0f))%8+iStone)%8, 0);
    Particle_RenderSquare( vPos, fSize, fRotation, col);
  }
  // all done
  Particle_Flush();
}

// spray some stones in the air
void Particles_HitGround(CEntity *penSpray, FLOAT tmStarted, FLOAT fSizeMultiplier)
{
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fT=(fNow-tmStarted);
  Particle_PrepareTexture( &_toStonesSprayTexture, PBT_BLEND);

  FLOAT fTotalTime = 10.0f;
  FLOAT fFadeOutStart = 7.5f;
  FLOAT fFadeInEnd = 1.0f;
  FLOAT3D vG = FLOAT3D(0.0f,-30.0f,0.0f);

  for( INDEX iStone=0; iStone<64; iStone++)
  {
    INDEX idx = int(iStone+tmStarted*33)%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vSpeed = FLOAT3D(
      afStarsPositions[ idx][0]*1.5f,
      (afStarsPositions[ idx][1]+0.5f)*3,
      afStarsPositions[ idx][2]*1.5f);
    FLOAT fSpeedMultiplier = (fSizeMultiplier-1)*(0.5f-1.0f)/(0.025f-1.0f)+1.0f;
    vSpeed *= 50.0f*fSpeedMultiplier;
    
    // calculate position
    FLOAT3D vPos = penSpray->GetPlacement().pl_PositionVector +
      vSpeed*fT +
      vG*fT*fT;

    FLOAT fFade;
    // apply fade
    if( fT<fFadeInEnd)
    {
      fFade = 1.0f;
    }
    else if (fT>fFadeOutStart)
    {
      fFade = (-1/(fTotalTime-fFadeOutStart))*(fT-fTotalTime);
    }
    else if( fT>fTotalTime)
    {
      fFade =0.0f;
    }
    else
    {
      fFade = 1.0f;
    }

    UBYTE ubRndH = UBYTE( 16+afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*8);
    UBYTE ubRndS = UBYTE( 96+(afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*64);
    UBYTE ubRndV = UBYTE( 128+(afStarsPositions[ int(iStone+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*64);
    UBYTE ubAlpha = UBYTE(CT_OPAQUE*fFade);
    COLOR col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
    
    FLOAT fSize=(afStarsPositions[ int(iStone+tmStarted*100)%CT_MAX_PARTICLES_TABLE][2]+1.0f)*4.0f*fSizeMultiplier;
    FLOAT fRotation = fT*200.0f;

    Particle_SetTexturePart( 256, 256, ((int(tmStarted*100.0f))%8+iStone)%8, 0);
    Particle_RenderSquare( vPos, fSize, fRotation, col);
  }
  // all done
  Particle_Flush();
}
