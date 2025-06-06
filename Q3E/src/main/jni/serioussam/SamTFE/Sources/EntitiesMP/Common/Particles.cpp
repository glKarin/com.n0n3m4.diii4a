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

#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/BloodSpray.h"
#include "EntitiesMP/PlayerWeapons.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/BackgroundViewer.h"
#include "EntitiesMP/EnvironmentParticlesHolder.h"

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
static CTextureObject _toFlamethrowerTrail01;
static CTextureObject _toFlamethrowerTrail02;
static CTextureObject _toFire;
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
static CTextureObject _toTreeSprayTexture;
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
static CTextureObject _toWaterfallFoam2;
static CTextureObject _toMetalSprayTexture;
static CTextureObject _toBulletGrass;
static CTextureObject _toBulletWood;
static CTextureObject _toBulletSnow;
static CTextureObject _toAirSprayTexture;
static CTextureObject _toFlameThrowerGradient;
static CTextureObject _toFlameThrowerStartGradient;
static CTextureObject _toSpawnerProjectile;
static CTextureObject _toExplosionDebris;
static CTextureObject _toExplosionDebrisGradient;
static CTextureObject _toExplosionSpark;
static CTextureObject _toChimneySmoke;
static CTextureObject _toChimneySmokeGradient;
static CTextureObject _toWaterfallGradient2;
static CTextureObject _toAfterBurner;
static CTextureObject _toAfterBurnerHead;
static CTextureObject _toAfterBurnerGradient;
static CTextureObject _toAfterBurnerGradientBlue;
static CTextureObject _toAfterBurnerGradientMeteor;
static CTextureObject _toTwisterGradient;
static CTextureObject _toTwister;
static CTextureObject _toLarvaLaser;
static CTextureObject _toLarvaProjectileSpray;
static CTextureObject _toGrowingTwirl;
static CTextureObject _toSummonerDisappearGradient;
static CTextureObject _toSEStar01;
static CTextureObject _toSummonerStaffGradient;
static CTextureObject _toMeteorTrail;
static CTextureObject _toFireworks01Gradient;

struct FlameThrowerParticleRenderingData {
  INDEX ftprd_iFrameX;
  INDEX ftprd_iFrameY;
  FLOAT3D ftprd_vPos;
  FLOAT ftprd_fSize;
  FLOAT ftprd_fAngle;
  COLOR ftprd_colColor;
};

INDEX _ctFlameThrowerParticles=0;
//INDEX _ctFlameThrowerPipeParticles=0;
#define MAX_FLAME_PARTICLES INDEX(1024)
FlameThrowerParticleRenderingData _aftprdFlame[MAX_FLAME_PARTICLES];
//FlameThrowerParticleRenderingData _aftprdFlamePipe[MAX_FLAME_PARTICLES];

BOOL UpdateGrowthCache(CEntity *pen, CTextureData *ptdGrowthMap, FLOATaabbox3D &boxGrowthMap, CEntity *penEPH, INDEX iDrawPort);

// array for model vertices in absolute space
CStaticStackArray<FLOAT3D> avVertices;

// current player projection
extern CAnyProjection3D prPlayerProjection;

extern FLOAT gfx_fEnvParticlesRange;

#define CT_MAX_PARTICLES_TABLE 1024

FLOAT afTimeOffsets[CT_MAX_PARTICLES_TABLE];
FLOAT afStarsPositions[CT_MAX_PARTICLES_TABLE][3];
UBYTE auStarsColors[CT_MAX_PARTICLES_TABLE][3];

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

void SnapFloat( FLOAT &fDest, FLOAT fStep)
{
  // this must use floor() to get proper snapping of negative values.
  FLOAT fDiv = fDest/fStep;
  FLOAT fRound = fDiv + 0.5f;
  int iSnap = int( floor(fRound));
  FLOAT fRes = iSnap * fStep;
  fDest = fRes;
}

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
    _toFlamethrowerTrail01.SetData_t(CTFILENAME("Textures\\Effects\\Particles\\FlameThrower01.tex"));
    _toFlamethrowerTrail02.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\FlameThrower02.tex"));
    _toFire.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\Fire.tex"));
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
    _toTreeSprayTexture.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\TreeSpill01.tex"));
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
    _toBulletGrass.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\BulletSprayGrass.tex"));
    _toBulletWood.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\BulletSprayWood.tex"));
    _toBulletSnow.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\BulletSpraySnow.tex"));
    _toAirSprayTexture.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\AirSpray.tex"));
    _toFlameThrowerGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\FlameThrowerGradient.tex"));
    _toFlameThrowerStartGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\FlameThrowerStartGradient.tex"));
    _toSpawnerProjectile.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\SpawnerProjectile.tex"));
    _toExplosionDebris.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\ExplosionDebris.tex"));
    _toExplosionDebrisGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\ExplosionDebrisGradient.tex"));
    _toExplosionSpark.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\ExplosionSpark.tex"));
    _toChimneySmoke.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\ChimneySmoke.tex"));
    _toTwister.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\Twister.tex"));
    _toChimneySmokeGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\ChimneySmokeGradient.tex"));
    _toWaterfallGradient2.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\WaterfallFoamGradient.tex"));
    _toAfterBurner.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\AfterBurner.tex"));
    _toAfterBurnerHead.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\AfterBurnerHead.tex"));
    _toAfterBurnerGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\AfterBurnerGradient.tex"));
    _toAfterBurnerGradientBlue.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\AfterBurnerGradientBlue.tex"));
    _toAfterBurnerGradientMeteor.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\AfterBurnerGradientMeteor.tex"));
    _toTwisterGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\TwisterGradient.tex"));
    _toWaterfallFoam2.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\WaterfallFoam.tex"));
    _toLarvaLaser.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\LarvaLaser.tex"));
    _toLarvaProjectileSpray.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\PlasmaProjectileSpill.tex"));
    _toGrowingTwirl.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\GrowingTwirl.tex"));
    _toSummonerDisappearGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\SummonerDisappearGradient.tex"));
    _toSummonerStaffGradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\SummonerStaffGradient.tex"));
    _toFireworks01Gradient.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\Fireworks01Gradient.tex"));
    _toSEStar01.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\Star01.tex"));
    _toMeteorTrail.SetData_t(CTFILENAME("TexturesMP\\Effects\\Particles\\MeteorTrail.tex"));

    
    ((CTextureData*)_toLavaTrailGradient              .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toLavaBombTrailGradient          .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toBeastDebrisTrailGradient       .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toBeastProjectileTrailGradient   .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toBeastBigProjectileTrailGradient.GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toWaterfallGradient              .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toSandFlowGradient               .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toWaterFlowGradient              .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toLavaFlowGradient               .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toFlameThrowerGradient           .GetData())->Force(TEX_STATIC|TEX_CONSTANT);    
    ((CTextureData*)_toFlameThrowerStartGradient      .GetData())->Force(TEX_STATIC|TEX_CONSTANT);    
    ((CTextureData*)_toExplosionDebrisGradient        .GetData())->Force(TEX_STATIC|TEX_CONSTANT);    
    ((CTextureData*)_toChimneySmokeGradient           .GetData())->Force(TEX_STATIC|TEX_CONSTANT); 
    ((CTextureData*)_toWaterfallGradient2             .GetData())->Force(TEX_STATIC|TEX_CONSTANT); 
    ((CTextureData*)_toAfterBurnerGradient            .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toAfterBurnerGradientBlue        .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toAfterBurnerGradientMeteor      .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toTwisterGradient                .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toSummonerDisappearGradient      .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toSummonerStaffGradient          .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
    ((CTextureData*)_toFireworks01Gradient            .GetData())->Force(TEX_STATIC|TEX_CONSTANT);
  }
  catch ( const char *strError)
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
  _toFlamethrowerTrail01.SetData(NULL);
  _toFlamethrowerTrail02.SetData(NULL);
  _toFire.SetData(NULL);
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
  _toTreeSprayTexture.SetData(NULL);
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
  _toWaterfallFoam2.SetData(NULL);
  _toMetalSprayTexture.SetData(NULL);
  _toBulletGrass.SetData(NULL);
  _toBulletWood.SetData(NULL);
  _toBulletSnow.SetData(NULL);
  _toAirSprayTexture.SetData(NULL);
  _toFlameThrowerGradient.SetData(NULL);
  _toFlameThrowerStartGradient.SetData(NULL);
  _toSpawnerProjectile.SetData(NULL);
  _toExplosionDebris.SetData(NULL);
  _toExplosionDebrisGradient.SetData(NULL);
  _toExplosionSpark.SetData(NULL);
  _toChimneySmoke.SetData(NULL);
  _toTwister.SetData(NULL);
  _toAfterBurner.SetData(NULL);
  _toAfterBurnerHead.SetData(NULL);
  _toAfterBurnerGradient.SetData(NULL);
  _toAfterBurnerGradientBlue.SetData(NULL);
  _toAfterBurnerGradientMeteor.SetData(NULL);
  _toTwisterGradient.SetData(NULL);
  _toChimneySmokeGradient.SetData(NULL);
  _toWaterfallGradient2.SetData(NULL);
  _toLarvaLaser.SetData(NULL);
  _toLarvaProjectileSpray.SetData(NULL);
  _toGrowingTwirl.SetData(NULL);
  _toSummonerDisappearGradient.SetData(NULL);
  _toSummonerStaffGradient.SetData(NULL);
  _toFireworks01Gradient.SetData(NULL);
  _toSEStar01.SetData(NULL);
  _toMeteorTrail.SetData(NULL);
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

void SetupParticleTextureWithAddAlpha(enum ParticleTexture ptTexture)
{
  switch(ptTexture) {
  case PT_STAR01:           Particle_PrepareTexture(&_toStar01,    PBT_ADDALPHA);   break;
  case PT_STAR02:           Particle_PrepareTexture(&_toStar02,    PBT_ADDALPHA);   break;
  case PT_STAR03:           Particle_PrepareTexture(&_toStar03,    PBT_ADDALPHA);   break;
  case PT_STAR04:           Particle_PrepareTexture(&_toStar04,    PBT_ADDALPHA);   break;
  case PT_STAR05:           Particle_PrepareTexture(&_toStar05,    PBT_ADDALPHA);   break;
  case PT_STAR06:           Particle_PrepareTexture(&_toStar06,    PBT_ADDALPHA);   break;
  case PT_STAR07:           Particle_PrepareTexture(&_toStar07,    PBT_ADDALPHA);   break;
  case PT_STAR08:           Particle_PrepareTexture(&_toStar08,    PBT_ADDALPHA);   break;
  case PT_BOUBBLE01:        Particle_PrepareTexture(&_toBoubble01, PBT_ADDALPHA);   break;
  case PT_BOUBBLE02:        Particle_PrepareTexture(&_toBoubble02, PBT_ADDALPHA);   break;
  case PT_WATER01:          Particle_PrepareTexture(&_toBoubble03, PBT_BLEND); break;
  case PT_WATER02:          Particle_PrepareTexture(&_toBoubble03, PBT_BLEND); break;
  case PT_SANDFLOW:         Particle_PrepareTexture(&_toSand,      PBT_BLEND); break;
  case PT_WATERFLOW:        Particle_PrepareTexture(&_toWater,     PBT_BLEND); break;
  case PT_LAVAFLOW:         Particle_PrepareTexture(&_toLava,      PBT_BLEND); break;
  default:    ASSERT(FALSE);
  }
  Particle_SetTexturePart( 512, 512, 0, 0);
}

void Particles_ViewerLocal(CEntity *penView)
{
  ASSERT(penView!=NULL);

  // obtain world settings controller
  CWorldSettingsController *pwsc = NULL;
  CEnvironmentParticlesHolder *eph = NULL;
  // obtain bcg viewer
  CBackgroundViewer *penBcgViewer = (CBackgroundViewer *) penView->GetWorld()->GetBackgroundViewer();
  if (penBcgViewer != NULL)
  {
    // obtain world settings controller 
    pwsc = (CWorldSettingsController *) penBcgViewer->m_penWorldSettingsController.ep_pen;
    if (pwsc != NULL)
    {
      // obtain environment particles holder 
      eph = (CEnvironmentParticlesHolder *) pwsc->m_penEnvPartHolder.ep_pen;
    }
  }

  FLOATaabbox3D boxViewer;
  penView->GetBoundingBox(boxViewer);
    
  while (eph != NULL)
  {
    // calculate the bounding box inside which particles will be visible
    FLOATaabbox3D boxViewTreshold;
    boxViewTreshold = eph->m_boxHeightMap;
    boxViewTreshold += eph->GetPlacement().pl_PositionVector;
    
    FLOAT fRangeMod = Clamp(gfx_fEnvParticlesRange, 0.1f, 2.0f);
    if (eph->m_eptType == EPTH_GROWTH) {
      boxViewTreshold.Expand(eph->m_fGrowthRenderingRadius*fRangeMod+5.0f);
    }

    // shared height box variables
    FLOATaabbox3D boxTerrainMap;
    CTextureData *ptdTerrainMap;
      
    if (boxViewer.HasContactWith(boxViewTreshold)) {
      switch (eph->m_eptType) {
        
      case EPTH_GROWTH:
        // misc. growth - grass, bushes
        if( pwsc != NULL )
        {
          eph->GetHeightMapData( ptdTerrainMap, boxTerrainMap);
          Particles_Growth( penView, ptdTerrainMap, boxTerrainMap, eph, Particle_GetDrawPortID());
        }
        break;
        
      case EPTH_RAIN: {
        // rain
        FLOAT fRainFactor = eph->GetRainFactor();
        if( fRainFactor != 0.0f)
        {
          eph->GetHeightMapData( ptdTerrainMap, boxTerrainMap);
          Particles_Rain( penView, 1.25f, 32, fRainFactor, ptdTerrainMap, boxTerrainMap);
        }
        break; }
        
      case EPTH_SNOW: {
        // snow
        FLOAT fSnowFactor = eph->GetSnowFactor();
        if( fSnowFactor != 0.0f)
        {
          eph->GetHeightMapData( ptdTerrainMap, boxTerrainMap);
          Particles_Snow( penView, 2.0f, 32, fSnowFactor, ptdTerrainMap, boxTerrainMap, eph->m_tmSnowStart);
        }
        break; }
        
      case EPTH_NONE:
        break;
        
      default:
        ASSERTALWAYS("Unknown environment particle type!");
        break;
      }
    // for those EPHs that are not rendered, clear possible
    // growth caches
    } else if (eph->m_eptType==EPTH_GROWTH) {
      // delete the cache for this EPH and this DrawPort
      INDEX iDrawPort = Particle_GetDrawPortID();
      {FORDELETELIST(CGrowthCache, cgc_Node, eph->lhCache, itCache)
        if (itCache->ulID==static_cast<ULONG>(iDrawPort)) {
          itCache->acgParticles.Clear();
          itCache->cgc_Node.Remove();
          delete &itCache.Current();
          //CPrintF("removed ph %s \n", eph->GetName());          
        }
      }
    }
    
    // next environment particles holder
    eph = (CEnvironmentParticlesHolder *) eph->m_penNextHolder.ep_pen;
    if (!(IsOfClass(eph, "EnvironmentParticlesHolder"))) break;
  }

  if(_ctFlameThrowerParticles!=0)
  {
    Particle_PrepareTexture( &_toFlamethrowerTrail02, PBT_ADDALPHA);
    INDEX flt_iFramesInRaw=4;
    INDEX flt_iFramesInColumn=4;
    for( INDEX iFlame=0; iFlame<ClampUp(_ctFlameThrowerParticles, MAX_FLAME_PARTICLES); iFlame++)
    {
      FlameThrowerParticleRenderingData &ftprd=_aftprdFlame[iFlame];
      Particle_SetTexturePart( 1024/flt_iFramesInRaw, 1024/flt_iFramesInColumn, ftprd.ftprd_iFrameX, ftprd.ftprd_iFrameY);
      Particle_RenderSquare( ftprd.ftprd_vPos, ftprd.ftprd_fSize, ftprd.ftprd_fAngle, ftprd.ftprd_colColor);
    }
    _ctFlameThrowerParticles=0;
    Particle_Flush();
  }

  /*
  if(_ctFlameThrowerPipeParticles!=0)
  {
    Particle_PrepareTexture( &_toLavaTrailSmoke, PBT_ADDALPHA);
    Particle_SetTexturePart( 512, 512, 0, 0);
    for( INDEX iFlameStart=0; iFlameStart<ClampUp(_ctFlameThrowerPipeParticles, MAX_FLAME_PARTICLES); iFlameStart++)
    {
      FlameThrowerParticleRenderingData &ftprd=_aftprdFlamePipe[iFlameStart];
      Particle_RenderSquare( ftprd.ftprd_vPos, ftprd.ftprd_fSize, ftprd.ftprd_fAngle, ftprd.ftprd_colColor);
    }
    _ctFlameThrowerPipeParticles=0;
    Particle_Flush();
  }*/
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
      FLOAT fAngle = iParticle*4.0f*180.0f/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/GRENADE_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = iParticle*0.3f/iParticlesLiving+0.1f;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264f*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704f*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964f*PI))*0.05f;
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
  //FLOAT fSeconds = _pTimer->GetLerpedCurrentTick();
  
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
      FLOAT fAngle = iParticle*4.0f*180.0f/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/LAVA_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = iParticle*3.0f/iParticlesLiving+0.5f;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264f*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704f*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964f*PI))*0.05f;
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
      FLOAT fAngle = iParticle*4.0f*180.0f/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/LAVA_BOMB_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = (iParticle*1.0f/iParticlesLiving+1.0f) * fSizeMultiplier;
      fSize += afStarsPositions[iRnd][0]*0.75f*fSizeMultiplier;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264f*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704f*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964f*PI))*0.05f;
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
      FLOAT fAngle = iParticle*4.0f*180.0f/iParticlesLiving;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/BEAST_PROJECTILE_DEBRIS_TRAIL_INTERPOSITIONS);
      FLOAT fRatio = FLOAT(iParticle)/iParticlesLiving+fSeconds;
      FLOAT fSize = ((iParticle*iParticle+1.0f)/iParticlesLiving+2.0f) * fSizeMultiplier;
      vPos(2) += iParticle*1.0f/iParticlesLiving;
      vPos(1) += FLOAT(sin(fRatio*1.264f*PI))*0.05f;
      vPos(2) += FLOAT(sin(fRatio*0.704f*PI))*0.05f;
      vPos(3) += FLOAT(sin(fRatio*0.964f*PI))*0.05f;
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
    COLOR colStar = pTD->GetTexel( ClampUp((int)FloatToInt(fT*8192),8191), 0);

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
    COLOR colStar = pTD->GetTexel( ClampUp((int)FloatToInt(fT*8192),8191), 0);

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
  INDEX iPos;
  for( iPos=2; iPos<plp->lp_ctUsed; iPos++)
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
  for( iPos=2; iPos<plp->lp_ctUsed; iPos++)
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

#define TM_EXPLOSIONDEBRISLIFE1 0.75f
#define CT_EXPLOSIONDEBRIS1 128
void Particles_ExplosionDebris1(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply/*=C_WHITE|CT_OPAQUE*/)
{
  CTextureData *pTD = (CTextureData *) _toExplosionDebrisGradient.GetData();
  Particle_PrepareTexture( &_toLavaEruptingTexture, PBT_ADDALPHA);
  
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*0.5f;
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fG=5.0f;
  FLOAT fStretchSize=(vStretch(1)+vStretch(2)+vStretch(3))/3.0f;

  for( INDEX iDebris=0; iDebris<CT_EXPLOSIONDEBRIS1; iDebris++)
  {
    INDEX iRnd =(pen->en_ulID+iDebris+INDEX(tmStart*123456.23465f))%CT_MAX_PARTICLES_TABLE;
    INDEX iRnd2=(pen->en_ulID+iDebris+INDEX(tmStart*653542.129633))%CT_MAX_PARTICLES_TABLE;
    INDEX iTexture = iRnd%3;
    Particle_SetTexturePart( 512, 512, iTexture, 0);
    FLOAT3D vSpeed=FLOAT3D(afStarsPositions[iRnd][0],afStarsPositions[iRnd][1],afStarsPositions[iRnd][2]);
    FLOAT fT=tmNow-tmStart;
    FLOAT fRatio=Clamp(fT/TM_EXPLOSIONDEBRISLIFE1*(afStarsPositions[iRnd][2]+1.0f), 0.0f, 1.0f);
    FLOAT fTimeDeccelerator=Clamp(1.0f-(fT/2.0f)*(fT/2.0f), 0.5f, 1.0f);
    FLOAT fSpeed=(afStarsPositions[iRnd][0]+afStarsPositions[iRnd][1]+afStarsPositions[iRnd][2]+0.5f*3)/3.0f*40.0f;
    fSpeed*=fTimeDeccelerator;
    FLOAT3D vRel=vSpeed*fSpeed*fT-vY*fG*fT*fT;
    vRel(1)*=vStretch(1);
    vRel(2)*=vStretch(2);
    vRel(3)*=vStretch(3);
    FLOAT3D vPos=vCenter+vRel;
    
    UBYTE ubR = (UBYTE) (255);
    UBYTE ubG = (UBYTE) (240+afStarsPositions[iRnd][1]*32);
    UBYTE ubB = (UBYTE) (240+afStarsPositions[iRnd][2]*32);
    COLOR colAlpha = pTD->GetTexel(PIX(ClampUp(fRatio*1024.0f, 1023.0f)), 0);
    COLOR col= RGBToColor( ubR, ubG, ubB) | (colAlpha&0x000000FF);
    col=MulColors(col, colMultiply);

    FLOAT fSize=(0.2f+afStarsPositions[iRnd2][0]*0.2f)*fStretchSize;
    FLOAT fAngle=afStarsPositions[iRnd2][1]*720.0f*fT;
    Particle_RenderSquare( vPos, fSize, fAngle, col);
  }
  // all done
  Particle_Flush();
}
 
#define TM_EXPLOSIONDEBRISLIFE2 0.85f
#define CT_EXPLOSIONDEBRIS2 32
void Particles_ExplosionDebris2(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply/*=C_WHITE|CT_OPAQUE*/)
{
  Particle_PrepareTexture( &_toExplosionDebris, PBT_BLEND);
  CTextureData *pTD = (CTextureData *) _toExplosionDebrisGradient.GetData();
  
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*0.5f;
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fG=5.0f;
  FLOAT fStretchSize=(vStretch(1)+vStretch(2)+vStretch(3))/3.0f;

  for( INDEX iDebris=0; iDebris<CT_EXPLOSIONDEBRIS2; iDebris++)
  {
    INDEX iRnd =(pen->en_ulID+iDebris+INDEX(tmStart*432256.32423f))%CT_MAX_PARTICLES_TABLE;
    INDEX iRnd2=(pen->en_ulID+iDebris+INDEX(tmStart*631512.15464f))%CT_MAX_PARTICLES_TABLE;
    Particle_SetTexturePart( 256, 256, iRnd%8, 0);
    FLOAT3D vSpeed=FLOAT3D(afStarsPositions[iRnd][0],afStarsPositions[iRnd][1],afStarsPositions[iRnd][2]);
    FLOAT fT=tmNow-tmStart;
    FLOAT fRatio=Clamp(fT/TM_EXPLOSIONDEBRISLIFE2, 0.0f, 1.0f);
    FLOAT fTimeDeccelerator=Clamp(1.0f-(fT/2.0f)*(fT/2.0f), 0.5f, 1.0f);
    FLOAT fSpeed=(afStarsPositions[iRnd][0]+afStarsPositions[iRnd][1]+afStarsPositions[iRnd][2]+0.5f*3)/3.0f*60.0f;
    fSpeed*=fTimeDeccelerator;
    FLOAT3D vRel=vSpeed*fSpeed*fT-vY*fG*fT*fT;
    vRel(1)*=vStretch(1);
    vRel(2)*=vStretch(2);
    vRel(3)*=vStretch(3);    
    FLOAT3D vPos=vCenter+vRel;
    
    COLOR colAlpha = pTD->GetTexel(PIX(ClampUp(fRatio*1024.0f, 1023.0f)), 0);
    COLOR col= C_WHITE | (colAlpha&0x000000FF);
    col=MulColors(col, colMultiply);

    FLOAT fSize=(0.15f+afStarsPositions[iRnd2][0]*0.1f)*fStretchSize;
    FLOAT fAngle=afStarsPositions[iRnd2][1]*2000.0f*fT;
    Particle_RenderSquare( vPos, fSize, fAngle, col);
  }
  // all done
  Particle_Flush();
}

#define TM_EXPLOSIONDEBRISLIFE3 0.5f
#define CT_EXPLOSIONDEBRIS3 64
void Particles_ExplosionDebris3(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply/*=C_WHITE|CT_OPAQUE*/)
{
  Particle_PrepareTexture( &_toExplosionSpark, PBT_ADDALPHA);
  Particle_SetTexturePart( 1024, 1024, 0, 0);
  CTextureData *pTD = (CTextureData *) _toExplosionDebrisGradient.GetData();
  
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*0.5f;
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fG=0.0f;
  FLOAT fStretchSize=(vStretch(1)+vStretch(2)+vStretch(3))/3.0f;

  for( INDEX iDebris=0; iDebris<CT_EXPLOSIONDEBRIS3; iDebris++)
  {
    INDEX iRnd =(pen->en_ulID+iDebris+INDEX(tmStart*317309.14521f))%CT_MAX_PARTICLES_TABLE;
    INDEX iRnd2=(pen->en_ulID+iDebris+INDEX(tmStart*421852.46521f))%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vSpeed=FLOAT3D(afStarsPositions[iRnd][0],afStarsPositions[iRnd][1],afStarsPositions[iRnd][2])*1.25f;
    FLOAT fT=tmNow-tmStart;
    FLOAT fRatio=Clamp(fT/TM_EXPLOSIONDEBRISLIFE3, 0.0f, 1.0f);
    FLOAT fTimeDeccelerator=Clamp(1.0f-(fT/2.0f)*(fT/2.0f), 0.75f, 1.0f);
    FLOAT fSpeed=(afStarsPositions[iRnd][0]+afStarsPositions[iRnd][1]+afStarsPositions[iRnd][2]+0.5f*3)/3.0f*50.0f;
    fSpeed*=fTimeDeccelerator;
    FLOAT fTOld=tmNow-tmStart-0.025f-0.1f*fRatio;
    
    FLOAT3D vRel=vSpeed*fSpeed*fT-vY*fG*fT*fT;
    vRel(1)*=vStretch(1);
    vRel(2)*=vStretch(2);
    vRel(3)*=vStretch(3);
    FLOAT3D vPos=vCenter+vRel;

    FLOAT3D vRelOld=vSpeed*fSpeed*fTOld-vY*fG*fTOld*fTOld;
    vRelOld(1)*=vStretch(1);
    vRelOld(2)*=vStretch(2);
    vRelOld(3)*=vStretch(3);
    FLOAT3D vOldPos=vCenter+vRelOld;
    if( (vPos - vOldPos).Length() == 0.0f) {continue;}
    
    UBYTE ubR = (UBYTE) (255);
    UBYTE ubG = (UBYTE) (200+afStarsPositions[iRnd][1]*32);
    UBYTE ubB = (UBYTE) (150+afStarsPositions[iRnd][2]*32);
    COLOR colAlpha = pTD->GetTexel(PIX(ClampUp(fRatio*1024.0f, 1023.0f)), 0);
    COLOR col= RGBToColor( ubR, ubG, ubB) | (colAlpha&0x000000FF);
    col=MulColors(col, colMultiply);

    FLOAT fSize=(0.1f+afStarsPositions[iRnd2][0]*0.15f)*fStretchSize;
    Particle_RenderLine( vOldPos, vPos, fSize, col);
  }
  // all done
  Particle_Flush();
}

#define TM_ES_TOTAL_LIFE 4.0f
#define TM_ES_SMOKE_DELTA 0.4f
void Particles_ExplosionSmoke(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply/*=C_WHITE|CT_OPAQUE*/)
{
  Particle_PrepareTexture( &_toBulletSmoke, PBT_BLEND);
  //CTextureData *pTD = (CTextureData *) _toExplosionDebrisGradient.GetData();
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*1.25f;
  FLOAT fStretchSize=(vStretch(1)+vStretch(2)+vStretch(3))/3.0f;

  INDEX iCtRnd =(pen->en_ulID*2+INDEX(tmStart*5311234.12531))%CT_MAX_PARTICLES_TABLE;
  INDEX ctSmokes=6+INDEX((afStarsPositions[iCtRnd][0]+0.5f)*2);
  for(INDEX i=0; i<ctSmokes; i++)
  {
    FLOAT fSmokeNoRatio=1.0f-FLOAT(i)/ctSmokes;
    INDEX iRnd =(pen->en_ulID+i+INDEX(tmStart*317309.14521f))%CT_MAX_PARTICLES_TABLE;
    INDEX iRnd2 =(pen->en_ulID+iRnd+i*i+INDEX(tmStart*125187.83754))%CT_MAX_PARTICLES_TABLE;
    INDEX iRndTex = iRnd*324561+pen->en_ulID;
    Particle_SetTexturePart( 512, 512, iRndTex%3, 0);
    FLOAT fTRnd=afStarsPositions[iRnd][0];
    FLOAT tmBorn=tmStart+i*TM_ES_SMOKE_DELTA+(TM_ES_SMOKE_DELTA*fTRnd)/2.0f;
    FLOAT fT=fNow-tmBorn;
    //FLOAT fRatio=Clamp(fT/TM_ES_TOTAL_LIFE, 0.0f, 1.0f);
    if( fT>0)
    {
      FLOAT3D vSpeed=FLOAT3D(afStarsPositions[iRnd][0]*0.15f,
        afStarsPositions[iRnd][1]*0.2f+1.0f,afStarsPositions[iRnd][2]*0.15f);
      FLOAT fSpeed=1.5f+(afStarsPositions[iRnd][0]+0.5f)*0.5f+fSmokeNoRatio*1.0f;
      FLOAT3D vRel=vSpeed*fSpeed*fT;
      vRel(1)*=vStretch(1);
      vRel(2)*=vStretch(2);
      vRel(3)*=vStretch(3);
      FLOAT3D vPos=vCenter+vRel;
      FLOAT fSize=((fSmokeNoRatio+0.125f)*2.0f+(afStarsPositions[iRnd][1]+0.5f)*1.0f*fT)*fStretchSize;
      FLOAT fAngle=afStarsPositions[iRnd2][0]*360+afStarsPositions[iRnd2][1]*90.0f*fT;
      FLOAT fColorRatio=CalculateRatio(fT, 0, TM_ES_TOTAL_LIFE, 0.1f, 0.4f)*(ClampUp(0.25f+fSmokeNoRatio,1.0f));
      FLOAT fRndBlend = 64.0f+(afStarsPositions[iRnd][2]+0.5f)*32.0f;
      UBYTE ubRndH=255;
      UBYTE ubRndS=0;
      UBYTE ubRndV = UBYTE( 96.0f+afStarsPositions[iRnd][0]*64.0f);
      COLOR col = HSVToColor(ubRndH,ubRndS,ubRndV)|UBYTE(fRndBlend*fColorRatio);
      col=MulColors(col, colMultiply);
      Particle_RenderSquare( vPos, fSize, fAngle, col);
    }
  }
  // all done
  Particle_Flush();
}

#define TM_CS_TOTAL_LIFE 10.0f
void Particles_ChimneySmoke(CEntity *pen, INDEX ctCount, FLOAT fStretchAll, FLOAT fMipDisappearDistance)
{
  FLOAT fMipFactor = Particle_GetMipFactor();
  if( fMipFactor>fMipDisappearDistance) return;
  FLOAT fMipBlender=CalculateRatio(fMipFactor, 0.0f, fMipDisappearDistance, 0.0f, 0.1f);

  Particle_PrepareTexture( &_toChimneySmoke, PBT_BLEND);
  Particle_SetTexturePart( 1024, 1024, 0, 0);
  CTextureData *pTD = (CTextureData *) _toChimneySmokeGradient.GetData();
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*0.0f;
  INDEX iPosRnd=INDEX(vCenter(1)*2343.1123f+vCenter(2)*3251.16732+vCenter(3)*2761.6323f);
  INDEX iCtRnd = Abs(INDEX(pen->en_ulID+iPosRnd));
  INDEX ctSmokes=22+INDEX((afStarsPositions[iCtRnd%CT_MAX_PARTICLES_TABLE][0]+0.5f)*8);
  for(INDEX i=0; i<ctSmokes; i++)
  {
    INDEX iRnd =(pen->en_ulID+i)%CT_MAX_PARTICLES_TABLE;
    FLOAT fT = tmNow+afTimeOffsets[i];
    // apply time strech
    fT *= 1/TM_CS_TOTAL_LIFE;
    // get fraction part
    fT = fT-int(fT);
    FLOAT fSlowFactor=1.0f-fT*0.25f;
    FLOAT3D vSpeed=FLOAT3D(afStarsPositions[iRnd][0]*0.15f,
      (afStarsPositions[iRnd][1]*0.1f+0.8f)*fSlowFactor,afStarsPositions[iRnd][2]*0.15f);
    FLOAT fSpeed=25.0f+(afStarsPositions[iRnd][0]+0.5f)*2.0f;
    FLOAT3D vPos=vCenter+vSpeed*fSpeed*fT*fStretchAll;
    FLOAT fSize=(0.75f+(afStarsPositions[iRnd][1]+0.5f)*4.0f*fT)*fStretchAll;
    FLOAT fAngle=afStarsPositions[iRnd][0]*360+afStarsPositions[iRnd][1]*360.0f*fT;
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = pTD->GetTexel(PIX(ClampUp(fT*1024.0f, 1023.0f)), 0);
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f*fMipBlender);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    Particle_RenderSquare( vPos, fSize, fAngle, colCombined);
  }
  // all done
  Particle_Flush();
}

void DECL_DLL Particles_Waterfall(CEntity *pen, INDEX ctCount, FLOAT fStretchAll, FLOAT fStretchX, FLOAT fStretchY,
                                  FLOAT fStretchZ, FLOAT fSize, FLOAT fMipDisappearDistance,
                                  FLOAT fParam1)
{
  FLOAT fMipFactor = Particle_GetMipFactor();
  if( fMipFactor>fMipDisappearDistance) return;
  FLOAT fMipBlender=CalculateRatio(fMipFactor, 0.0f, fMipDisappearDistance, 0.0f, 0.1f);
  
  Particle_PrepareTexture( &_toWaterfallFoam2, PBT_ADDALPHA);
  CTextureData *pTD = (CTextureData *) _toWaterfallGradient2.GetData();
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vG=vY;
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  vX=vX*fStretchX*fStretchAll;
  vY=vY*fStretchY*fStretchAll;
  vZ=vZ*fStretchZ*fStretchAll;
  FLOAT fGA = 10.0f;
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*0.0f;
  for(INDEX i=0; i<ctCount; i++)
  {
    INDEX iRnd =(pen->en_ulID+i)%CT_MAX_PARTICLES_TABLE;
    INDEX iFrame=iRnd%4;
    Particle_SetTexturePart( 256, 256, iFrame, 0);
    FLOAT fT = tmNow+afTimeOffsets[i];
    // apply time strech
    fT *= 1/fParam1;
    // get fraction part
    fT = fT-int(fT);
    //FLOAT fSlowFactor=1.0f-fT*0.25f;
    FLOAT3D vSpeed=
      vX*(afStarsPositions[iRnd][0]*0.25f)+
      vY*(afStarsPositions[iRnd][0]*0.25f)+
      -vZ*(1.5f+afStarsPositions[iRnd][0]*0.25f);
    FLOAT fSpeed=20.0f+(afStarsPositions[iRnd][0]+0.5f)*2.0f;
    FLOAT3D vPos=vCenter+vSpeed*fSpeed*fT-vG*fGA/2.0f*(fT*fParam1)*(fT*fParam1);
    FLOAT fFinalSize=(3.5f+(afStarsPositions[iRnd][1]+1.0f)*2.0f*fT)*fSize;
    FLOAT fAngle=afStarsPositions[iRnd][0]*360+afStarsPositions[iRnd][1]*360.0f*fT*fParam1/2.0f;
    if( iFrame>=2)
    {
      fAngle=0.0f;
    }
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = pTD->GetTexel(PIX(ClampUp(fT*1024.0f, 1023.0f)), 0);
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f*fMipBlender);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    //colCombined = C_WHITE|CT_OPAQUE;
    Particle_RenderSquare( vPos, fFinalSize, fAngle, colCombined);
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
  Particle_PrepareTexture( &_toFlamethrowerTrail01, PBT_ADD);
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
    UBYTE ub = (UBYTE) CalculateRatio( fT, 0.0f, 1.0f, 0.1f, 0.2f)*255;
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
    
    // DG: changed indices from 1-3 to 0-2 so they're not out of bounds
    UBYTE ubR = (UBYTE) (192+afStarsPositions[iRnd][0]*64);
    UBYTE ubG = (UBYTE) (192+afStarsPositions[iRnd][1]*64);
    UBYTE ubB = (UBYTE) (192+afStarsPositions[iRnd][2]*64);
    UBYTE ubA = (UBYTE) CalculateRatio( fT, 0.0f, 1.0f, 0.4f, 0.01f)*255;
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


#define FLAME_LIFETIME 1.0f
#define FLAME_INTERTIME 0.1f
void Particles_FlameThrower(const CPlacement3D &plLeader, const CPlacement3D &plFollower,
                            FLOAT3D vSpeedLeader, FLOAT3D vSpeedFollower,
                            FLOAT fLeaderLiving, FLOAT fFollowerLiving,
                            INDEX iRndSeed, BOOL bFollowerIsPipe)
{
  INDEX flt_iFramesInRaw=4;
  INDEX flt_iFramesInColumn=4;
  INDEX flt_iInterpolations=10;
  FLOAT flt_fSizeStart=0.075f; 
  FLOAT flt_fSizeEnd=6;

  //Particle_PrepareTexture( &_toFlamethrowerTrail02, PBT_ADDALPHA);
  CTextureData *pTD = (CTextureData *) _toFlameThrowerGradient.GetData();

  const FLOAT3D &vFollower = plFollower.pl_PositionVector;
  const FLOAT3D &vLeader = plLeader.pl_PositionVector;

  // control points
  FLOAT x1=vFollower(1);
  FLOAT y1=vFollower(2);
  FLOAT z1=vFollower(3);
  FLOAT x2=vLeader(1);
  FLOAT y2=vLeader(2);
  FLOAT z2=vLeader(3);

  // control directions (tangents)
  FLOAT dx1=vSpeedLeader(1);
  FLOAT dy1=vSpeedLeader(2);
  FLOAT dz1=vSpeedLeader(3);
  FLOAT dx2=vSpeedLeader(1);
  FLOAT dy2=vSpeedLeader(2);
  FLOAT dz2=vSpeedLeader(3);

  // calculate parameters of hermit spline
  FLOAT ft3x=dx1+dx2+2.0f*x1-2.0f*x2;
  FLOAT ft2x=-2.0f*dx1-dx2-3.0f*(x1-x2);
  FLOAT ft1x=dx1;
  FLOAT ft0x=x1;
  FLOAT ft3y=dy1+dy2+2.0f*y1-2.0f*y2;
  FLOAT ft2y=-2.0f*dy1-dy2-3.0f*(y1-y2);
  FLOAT ft1y=dy1;
  FLOAT ft0y=y1;
  FLOAT ft3z=dz1+dz2+2.0f*z1-2.0f*z2;
  FLOAT ft2z=-2.0f*dz1-dz2-3.0f*(z1-z2);
  FLOAT ft1z=dz1;
  FLOAT ft0z=z1;

  INDEX iParticle=0;
  FLOAT fLiving=fLeaderLiving;
  while(fLiving>=fFollowerLiving)
  {
    FLOAT fOlderThanLeaderTime=fLeaderLiving-fLiving;

    // set frame
    INDEX iFrame=ClampUp(INDEX(fLiving*flt_iFramesInRaw*flt_iFramesInColumn), INDEX(flt_iFramesInRaw*flt_iFramesInColumn-1));
    INDEX iFrameX=iFrame%flt_iFramesInRaw;
    INDEX iFrameY=iFrame/flt_iFramesInRaw;
    //Particle_SetTexturePart( 1024/flt_iFramesInRaw, 1024/flt_iFramesInColumn, iFrameX, iFrameY);

    // calculate time exponents
    FLOAT ft1=1.0f-fOlderThanLeaderTime/(fLeaderLiving-fFollowerLiving);
    FLOAT ft2=ft1*ft1;
    FLOAT ft3=ft1*ft1*ft1;
  
    // calculate particle position
    FLOAT fx=ft3*ft3x+ft2*ft2x+ft1*ft1x+1*ft0x;
    FLOAT fy=ft3*ft3y+ft2*ft2y+ft1*ft1y+1*ft0y;
    FLOAT fz=ft3*ft3z+ft2*ft2z+ft1*ft1z+1*ft0z;
    FLOAT3D vPos=FLOAT3D(fx,fy,fz);

    // add random position
    INDEX iRnd=(iParticle+iRndSeed)%CT_MAX_PARTICLES_TABLE;
    vPos(1) += afStarsPositions[iRnd][0]*fLiving;
    vPos(2) += afStarsPositions[iRnd][1]*fLiving + fLiving*fLiving*fLiving*2.0f;
    vPos(3) += afStarsPositions[iRnd][2]*fLiving;

    // size
    FLOAT fSize = flt_fSizeStart+fLiving*(fLiving*0.4f+(flt_fSizeEnd-flt_fSizeStart)*0.6f);
    // angle
    FLOAT fAngle = fLiving*180.0f*afStarsPositions[iParticle][0];
    // color
    COLOR col = pTD->GetTexel(PIX(Clamp(fLiving*1024.0f, 0.0f, 1023.0f)), 0);

    FlameThrowerParticleRenderingData &ftprd=_aftprdFlame[_ctFlameThrowerParticles+iParticle];
    ftprd.ftprd_iFrameX=iFrameX;
    ftprd.ftprd_iFrameY=iFrameY;
    ftprd.ftprd_vPos=vPos;
    ftprd.ftprd_fSize=fSize;
    ftprd.ftprd_fAngle=fAngle;
    ftprd.ftprd_colColor=col;

//    Particle_RenderSquare( vPos, fSize, fAngle, col);
    iParticle++;
    fLiving-=FLAME_INTERTIME/flt_iInterpolations;
  }
  // all done
//  Particle_Flush();
  _ctFlameThrowerParticles+=iParticle;
}

#define CT_FTSPARKS 64
#define CT_FTSPARK_TRAIL 1
#define FTSPARK_FADE_OUT 0.005f
#define FTSPARK_TOTAL_TIME 0.2f
#define FT_START_SPEED 1.5f
void Particles_FlameThrowerStart(const CPlacement3D &plPipe, FLOAT fStartTime, FLOAT fStopTime)
{
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  Particle_PrepareTexture( &_toLavaTrailSmoke, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);
  CTextureData *pTD = (CTextureData *) _toFlameThrowerStartGradient.GetData();

  FLOATmatrix3D m;
  MakeRotationMatrixFast(m, plPipe.pl_OrientationAngle);
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = plPipe.pl_PositionVector;

  FLOAT fPowerFactor = Clamp((fNow - fStartTime)/2.0f,0.0f,1.0f);
  fPowerFactor *= Clamp(1.0f+(fStopTime-fNow)/2.0f,0.0f,1.0f);
  INDEX ctParticles = (INDEX) (FLOAT(CT_FTSPARKS) * fPowerFactor);
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  //FLOAT fHeight = 1.0f*fPowerFactor;
  //INDEX iParticle=0;
  for( INDEX iSpark=0; iSpark<ctParticles; iSpark++)
  {
    for( INDEX iTrail=0; iTrail<CT_FTSPARK_TRAIL; iTrail++)
    {
      FLOAT fT = fNow+afTimeOffsets[iSpark]/10-iTrail*0.075f;
      // apply time strech
      fT *= 1/FTSPARK_TOTAL_TIME;
      // get fraction part
      fT = fT-int(fT);
      FLOAT fBirthTime = fNow-(fT*FTSPARK_TOTAL_TIME);
      if( (fBirthTime<fStartTime) || (fBirthTime>fStopTime+2.0f) ) continue;
// ######## fFade not used in this function
#ifndef NDEBUG
      FLOAT fFade;
      if (fT>(1.0f-FTSPARK_FADE_OUT)) fFade=(1-fT)*(1/FTSPARK_FADE_OUT);
      else fFade=1.0f;
      fFade *= (CT_FTSPARK_TRAIL-iTrail)*(1.0f/CT_FTSPARK_TRAIL);
#endif // NDEBUG

      FLOAT3D vPos = vCenter +
        vX*(afStarsPositions[iSpark][0]*0.15f*fT) +
        vY*(afStarsPositions[iSpark][1]*0.15f*fT) +
        -vZ*(FT_START_SPEED*fT);
    
      FLOAT fSize=(afStarsPositions[iSpark+16][0]+0.5f)*0.040f/*+fT*0.075f*/;
      COLOR col = pTD->GetTexel(PIX(ClampUp(fT*1024.0f, 1023.0f)), 0);
      FLOAT fAng=afStarsPositions[iSpark+8][0]*fT*360.0f;

      /*
      FlameThrowerParticleRenderingData &ftprd=_aftprdFlame[_ctFlameThrowerPipeParticles+iParticle];
      ftprd.ftprd_vPos=vPos;
      ftprd.ftprd_fSize=fSize;
      ftprd.ftprd_fAngle=fAng;
      ftprd.ftprd_colColor=col;
      iParticle++;
      */

      Particle_RenderSquare( vPos, fSize, fAng, col);
    }
  }
  //_ctFlameThrowerPipeParticles+=iParticle;
  // all done
  Particle_Flush();
}


void Particles_ShooterFlame(const CPlacement3D &plEnd, const CPlacement3D &plStart,
                            FLOAT fEndElapsed, FLOAT fStartElapsed)
{
  Particle_PrepareTexture( &_toFlamethrowerTrail02, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

  const FLOAT3D &vStart = plStart.pl_PositionVector;
  const FLOAT3D &vEnd = plEnd.pl_PositionVector;

#define SHOOTER_INTERFLAME_PARTICLES 10
  
  for (INDEX i=0; i<SHOOTER_INTERFLAME_PARTICLES; i++)
  {
    FLOAT   fBLFactor = FLOAT(i)/FLOAT(SHOOTER_INTERFLAME_PARTICLES);
    FLOAT3D vPos  = vStart+(vEnd-vStart)*fBLFactor;
    FLOAT   fTime = fStartElapsed+(fEndElapsed-fStartElapsed)*fBLFactor;
    
    INDEX iRndFact = 2*i*FloatToInt(fTime*8.0f)+2;

    // size
    FLOAT fSize = 0.05f + 1.0f*fTime;
    // angle
    FLOAT fAngle = fTime*180.0f*afStarsPositions[iRndFact][0];
    // transparency
    UBYTE ub = 255;
    if (fTime>1.0f) ub = 0;
    else if(fTime>0.6f) ub = (UBYTE) ((1.0-fTime)*(1.0f/0.4f)*255);
    // color with a bit of red tint before the burnout
    COLOR col = RGBToColor(192, 192, 192);
    if (fTime>0.95f) col = RGBToColor(192, 0, 0);
    else if (fTime>0.4f) col = RGBToColor(192, (UBYTE) ((1.0-fTime)*(1.0f/0.6f)*100+92), (UBYTE) ((1.0f-fTime)*(1.0f/0.6f)*112+80));
    
    vPos(1) += afStarsPositions[iRndFact][0]*fTime;
    vPos(2) += afStarsPositions[iRndFact][1]*fTime + 0.25f*fTime*fTime;
    vPos(3) += afStarsPositions[iRndFact][2]*fTime;

    Particle_RenderSquare( vPos, fSize, fAngle, col|ub);
  }
  // all done
  Particle_Flush();
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
    COLOR colStar = RGBToColor((UBYTE) (auStarsColors[iMemeber][0]*fFade),
                               (UBYTE) (auStarsColors[iMemeber][1]*fFade),
                               (UBYTE) (auStarsColors[iMemeber][2]*fFade));
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
  INDEX ctSpiralTrail = (INDEX) (fMipFactor*CT_SPIRAL_TRAIL);
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
                       enum ParticleTexture ptTexture, INDEX ctParticles, FLOAT fMipFactorDisappear)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  if( Particle_GetMipFactor()>fMipFactorDisappear) return;
  FLOAT fDisappearRatio=CalculateRatio(Particle_GetMipFactor(), 0, fMipFactorDisappear, 0, 0.1f);

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
    
    UBYTE ub = NormFloatToByte( fFade*fDisappearRatio);
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
                            enum ParticleTexture ptTexture, INDEX ctParticles, FLOAT fMipFactorDisappear)
{
  ASSERT( ctParticles<=CT_MAX_PARTICLES_TABLE);
  if( Particle_GetMipFactor()>fMipFactorDisappear) return;
  FLOAT fDisappearRatio=CalculateRatio(Particle_GetMipFactor(), 0, fMipFactorDisappear, 0, 0.1f);

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
    
    UBYTE ub = NormFloatToByte( fFade*fDisappearRatio);
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
    FLOAT fRotation = afStarsPositions[iRnd2+5][0]*360.0f+fT*200.0f*afStarsPositions[iRnd2+3][0];
    FLOAT fSize = 
       0.025f*fDamage+
      (afStarsPositions[iRnd2+6][2]+0.5f)*0.075f +
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

void Particles_DustFall(CEntity *pen, FLOAT tmStarted, FLOAT3D vStretch)
{
  FLOAT fMipFactor=Particle_GetMipFactor();
  fMipFactor=Clamp(fMipFactor,2.0f,6.0f);
  FLOAT fSizeRatio=0.125f+(1.0f-CalculateRatio(fMipFactor, 2.0f, 6.0f, 0.0, 1.0f))*0.875f;
  
  Particle_PrepareTexture( &_toBulletSmoke, PBT_BLEND);
  // get entity position and orientation
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;
  //FLOAT3D vG=-vY;
  //FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fT = _pTimer->GetLerpedCurrentTick()-tmStarted;
  FLOAT fStretch=vStretch.Length();

  //INDEX ctParticles=(INDEX) (4+fSizeRatio*28);
  for(INDEX iDust=0; iDust<32; iDust++)
  {
    INDEX iRnd = (pen->en_ulID*12345+iDust)%CT_MAX_PARTICLES_TABLE;
    Particle_SetTexturePart( 512, 512, iRnd%3, 0);

    FLOAT fLifeTime = 1.5f;
    FLOAT fRatio = fT/fLifeTime;
    if( fRatio>1.0f) continue;
    FLOAT fPower = CalculateRatio(fT, 0, fLifeTime, 0.1f, 0.4f);
    FLOAT fSpeed=0.351f+0.0506f*log(fRatio+0.001f);

    //FLOAT fRndAppearX = afStarsPositions[iRnd][0]*vStretch(1);
    //FLOAT fRndSpeedY = (afStarsPositions[iRnd][1]+0.5f)*0.125f*vStretch(2);
    //FLOAT fRndAppearZ = afStarsPositions[iRnd][2]*vStretch(3);
    FLOAT3D vRndDir=FLOAT3D(afStarsPositions[iRnd][0],0,afStarsPositions[iRnd][2]);
    vRndDir.Normalize();
    FLOAT fRiseTime=Max(fRatio-0.5f,0.0f);
    FLOAT3D vPos=vCenter+vRndDir*fSpeed*3*fStretch+vY*fRiseTime*0.25f;
    FLOAT fRndBlend = 8.0f+(afStarsPositions[iRnd][2]+0.5f)*64.0f;
    UBYTE ubRndH = UBYTE( (afStarsPositions[iRnd][0]+0.5f)*64);
    UBYTE ubRndS = UBYTE( (afStarsPositions[iRnd][1]+0.5f)*32);
    UBYTE ubRndV = UBYTE( 128+afStarsPositions[iRnd][2]*64.0f);
    COLOR col = HSVToColor(ubRndH,ubRndS,ubRndV)|UBYTE(fRndBlend*fPower);
    FLOAT fRotation = afStarsPositions[iRnd][0]*360+fT*360.0f*afStarsPositions[iRnd][0]*fSpeed;
    FLOAT fSize = 
       0.75f+(afStarsPositions[iRnd][2]+0.5f)*0.25f +         // static size
      (0.4f+(afStarsPositions[iRnd][1]+0.5f)*0.4f)*fT;    // dinamic size
    fSize*=fSizeRatio;
    Particle_RenderSquare( vPos, fSize*fStretch*0.2f, fRotation, col);
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
  
    UBYTE ubR = (UBYTE) (224+(afStarsPositions[iSpark][2]+0.5f)*32);
    UBYTE ubG = (UBYTE) (224+(afStarsPositions[iSpark][2]+0.5f)*32);
    UBYTE ubB = (UBYTE) (160);
    UBYTE ubA = (UBYTE) (FloatToInt( 255 * fFade));
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
  INDEX ctAtomicTrail = (INDEX) (fMipFactor*CT_ATOMIC_TRAIL);
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

void Particles_PowerUpIndicator( CEntity *pen, enum ParticleTexture ptTexture, FLOAT fSize,
              FLOAT fScale, FLOAT fHeight, INDEX ctEllipses, INDEX iTrailCount)
{
  ASSERT( ctEllipses<=CT_MAX_PARTICLES_TABLE);
  FLOAT fMipFactor = Particle_GetMipFactor();
  if( fMipFactor>7.0f) return;
  fMipFactor = 2.5f-fMipFactor*0.3f;
  fMipFactor = Clamp(fMipFactor, 0.0f ,1.0f);
  INDEX ctAtomicTrail = (INDEX) (fMipFactor*iTrailCount);
  if( ctAtomicTrail<=0) return;
  FLOAT fTrailDelta = 0.075f/fMipFactor;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  SetupParticleTexture( ptTexture);
  //const FLOATmatrix3D &m = pen->GetRotationMatrix();
  CPlacement3D pl = pen->GetLerpedPlacement();
  FLOATmatrix3D m;
  MakeRotationMatrixFast(m, pl.pl_OrientationAngle);
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pl.pl_PositionVector+vY*fHeight;

  for( INDEX iEllipse=0; iEllipse<ctEllipses; iEllipse++)
  {
    FLOAT fT = fNow*4+PI*2/3*iEllipse;
    FLOAT angle1 = 2*PI*iEllipse/ctEllipses;
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
      vPos+=vA*(cos((fT-iTrail*fTrailDelta))*1.0f*fScale);
      vPos+=vB*(sin((fT-iTrail*fTrailDelta))*1.0f*fScale);
      UBYTE ub = NormFloatToByte( (FLOAT)(ctAtomicTrail-iTrail) / (FLOAT)(ctAtomicTrail));
      COLOR colStar = RGBToColor( ub, ub, ub);
      Particle_RenderSquare( vPos, fSize, 0, colStar|0xFF);
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

// growth - one for each drawport

static int qsort_CompareGrowth(const void *pvGrowth0, const void *pvGrowth1)
{
  FLOAT ret = (((CGrowth *)pvGrowth1)->fDistanceToViewer -
      ((CGrowth *)pvGrowth0)->fDistanceToViewer) * 1000;
  return FloatToInt(ret);
}

BOOL UpdateGrowthCache(CEntity *pen, CTextureData *ptdGrowthMap, FLOATaabbox3D &boxGrowthMap, CEntity *penEPH, INDEX iDrawPort)
{

  // if there is no texture in EPH, return
  CEnvironmentParticlesHolder *eph = (CEnvironmentParticlesHolder *)&*penEPH;  
  if (eph->m_moParticleTextureHolder.mo_toTexture.GetData() == NULL) {
    return FALSE;
  }

  // don't ever render in editor
  if (_pTimer->CurrentTick()==0.0f)
  {
    return FALSE;
  }

  // calculate step
  extern FLOAT gfx_fEnvParticlesDensity;
  gfx_fEnvParticlesDensity = Clamp(gfx_fEnvParticlesDensity, 0.0f, 1.0f);
  FLOAT fStep = 1.0f;
  if (gfx_fEnvParticlesDensity<=0) {
    fStep = 0;
  } else {
    fStep = 1/gfx_fEnvParticlesDensity;
  }

  FLOAT GROWTH_RENDERING_STEP = eph->m_fGrowthRenderingStep;  

  // viewer absolute position
  FLOAT3D vPos = prPlayerProjection->pr_vViewerPosition;
  // snap viewer to grid
  FLOAT3D vSnapped = vPos;
  SnapFloat(vSnapped(1), GROWTH_RENDERING_STEP);
  SnapFloat(vSnapped(3), GROWTH_RENDERING_STEP);
  vSnapped(2) = 0.0f;

  PIX pixGrowthMapH;
  PIX pixGrowthMapW;
  
  if( ptdGrowthMap  != NULL)
  {
    pixGrowthMapW = ptdGrowthMap->GetPixWidth();
    pixGrowthMapH = ptdGrowthMap->GetPixHeight();
  } else {
    return FALSE;
  }
  
  FLOAT3D vRender;
  FLOAT texX; 
  FLOAT texY;
  FLOAT fRawHeight;
  
  FLOAT fRangeMod = Clamp(gfx_fEnvParticlesRange, 0.1f, 2.0f);
  FLOAT GROWTH_RENDERING_RADIUS_OPAQUE = (eph->m_fGrowthRenderingRadius - eph->m_fGrowthRenderingRadiusFade)*fRangeMod;
  FLOAT GROWTH_RENDERING_RADIUS_FADE = eph->m_fGrowthRenderingRadius*fRangeMod;
  BOOL  GROWTH_HIGHRES_MAP = eph->m_bGrowthHighresMap;
  
  ASSERT(GROWTH_RENDERING_RADIUS_FADE>=GROWTH_RENDERING_RADIUS_OPAQUE);
  
  FLOAT fGridStep;
  ULONG fXSpan;
  // UBYTE ubFade=0xff;
  
  fGridStep = GROWTH_RENDERING_STEP;
  fXSpan = 1234;
  
  INDEX iGridX1 = (INDEX) (GROWTH_RENDERING_RADIUS_FADE/GROWTH_RENDERING_STEP);
  INDEX iGridX0 = -iGridX1;
  INDEX iGridY1 = iGridX1;
  INDEX iGridY0 = -iGridX1;
    
  // find growth cache and check if it is initialised
  CGrowthCache *cgc = NULL;
  {FOREACHINLIST(CGrowthCache, cgc_Node, eph->lhCache, itCache)
    if (itCache->ulID==static_cast<ULONG>(iDrawPort)) cgc = itCache;
  }
  // if no cache found, create one
  if (cgc==NULL)
  {
    cgc = new(CGrowthCache);
    cgc->ulID = iDrawPort;
    cgc->iGridSide = iGridX1*2+1;
    cgc->vLastPos = vSnapped;
    eph->lhCache.AddTail(cgc->cgc_Node);
    //CPrintF("added ph %s \n", eph->GetName());
  } else {
    if (cgc->vLastPos==vSnapped && cgc->fStep==fStep) {
      cgc->vLastPos = vSnapped;
      return TRUE;
    }
    //CPrintF("need recashe! at %f\n", _pTimer->CurrentTick());
    cgc->vLastPos = vSnapped;
    cgc->fStep = fStep;
  }

  cgc->acgParticles.PopAll();
  CGrowth cgParticle;

  if (fStep<1) {
    return TRUE;
  }

  INDEX iOffI = FloatToInt(vSnapped(1)/GROWTH_RENDERING_STEP);
  INDEX iOffJ = FloatToInt(vSnapped(3)/GROWTH_RENDERING_STEP);

  FLOAT fStepSqrt = Sqrt(fStep);
  FLOAT f1oGridSizeX = 1.0f/boxGrowthMap.Size()(1);
  FLOAT f1oGridSizeZ = 1.0f/boxGrowthMap.Size()(3);
  FLOAT f1oGridStepX = 1.0f/(boxGrowthMap.Size()(1)/pixGrowthMapW);
  FLOAT f1oGridStepZ = 1.0f/(boxGrowthMap.Size()(3)/pixGrowthMapH);

  // loop through the whole particle grid
  for ( INDEX iC=iGridY0; iC<iGridY1+1; iC++ )
  {
    for ( INDEX jC=iGridX0; jC<iGridX1+1; jC++ )
    {
      double fmodi = fabs(fmod(iC+iOffI, fStepSqrt));
      double fmodj = fabs(fmod(jC+iOffJ, fStepSqrt));
      if ( fmodi>=1 || fmodj>=1) {
        continue;
      }

      // absolute positions :
      INDEX i = (INDEX) (iC*GROWTH_RENDERING_STEP + vSnapped(1));
      INDEX j = (INDEX) (jC*GROWTH_RENDERING_STEP + vSnapped(3));

            
      // apply a bit of randomness:
      UBYTE ubRndFact = (i*fXSpan+j)%CT_MAX_PARTICLES_TABLE;
            
      FLOAT iR, jR;
      iR = (FLOAT)i + fGridStep * afStarsPositions[ubRndFact][0];
      jR = (FLOAT)j + fGridStep * afStarsPositions[ubRndFact][2];
      
      // size: 
      cgParticle.fSize = Lerp(eph->m_fGrowthMinSize, eph->m_fGrowthMaxSize, 
        afStarsPositions[ubRndFact][2]+0.5f);
      
      texX = (iR-boxGrowthMap.Min()(1))*f1oGridSizeX*pixGrowthMapW;
      texY = (jR-boxGrowthMap.Min()(3))*f1oGridSizeZ*pixGrowthMapH;
      
      // particles that fall inside the boundaries
      if ((texX>0) && (texX<pixGrowthMapW) && (texY>0) && (texY<pixGrowthMapH))
      {
        // bilinear sampling of height data
        texX -= 0.5f;
        texY -= 0.5f;
        ULONG ulX1 = FloatToInt(floorf(texX));
        ULONG ulX2 = FloatToInt(ceilf(texX));  // if (ulX2>=pixGrowthMapW) ulX2=pixGrowthMapW-1;
        ULONG ulY1 = FloatToInt(floorf(texY));
        ULONG ulY2 = FloatToInt(ceilf(texY));  // if (ulY2>=pixGrowthMapH) ulY2=pixGrowthMapH-1;
        
        SLONG ulUL, ulUR, ulBL, ulBR;
        if (GROWTH_HIGHRES_MAP)
        {
          ulUL = (ptdGrowthMap->GetTexel(ulX1, ulY1)>>8)&0xFFFF; 
          ulUR = (ptdGrowthMap->GetTexel(ulX2, ulY1)>>8)&0xFFFF; 
          ulBL = (ptdGrowthMap->GetTexel(ulX1, ulY2)>>8)&0xFFFF; 
          ulBR = (ptdGrowthMap->GetTexel(ulX2, ulY2)>>8)&0xFFFF; 
        }
        else
        {
          ulUL = (ptdGrowthMap->GetTexel(ulX1, ulY1)>>8)&0xFF; 
          ulUR = (ptdGrowthMap->GetTexel(ulX2, ulY1)>>8)&0xFF; 
          ulBL = (ptdGrowthMap->GetTexel(ulX1, ulY2)>>8)&0xFF; 
          ulBR = (ptdGrowthMap->GetTexel(ulX2, ulY2)>>8)&0xFF; 
        }
        
        // bilinear formula
        FLOAT fDX = texX - ulX1;
        FLOAT fDY = texY - ulY1;
        fRawHeight = ulUL*(1-fDX)*(1-fDY) +
						         ulUR*(fDX - fDX*fDY) +
						         ulBL*(fDY - fDX*fDY) +
						         ulBR*(fDX*fDY);

        // calculate maximum slope per meter on each axis
        FLOAT fSlopeMul = 1.0f;
        if (GROWTH_HIGHRES_MAP) {
          fSlopeMul = boxGrowthMap.Size()(2)/65535.0f;
        } else {
          fSlopeMul = boxGrowthMap.Size()(2)/255.0f;
        }
        FLOAT fSlopeX = Max(Abs(ulUL-ulUR), Abs(ulBL-ulBR))*fSlopeMul;
        FLOAT fSlopeY = Max(Abs(ulUL-ulBL), Abs(ulUR-ulBR))*fSlopeMul;
        //CPrintF("%g %g\n", fSlopeX, fSlopeY);
        fSlopeX*=f1oGridStepX;
        fSlopeY*=f1oGridStepZ;
                
        // clamp to terrain height  
        FLOAT fHeight;
        if (GROWTH_HIGHRES_MAP)
        {
          fHeight = boxGrowthMap.Min()(2) + fRawHeight*boxGrowthMap.Size()(2)/65535.0f + cgParticle.fSize;
        }
        else
        {
          fHeight = boxGrowthMap.Min()(2) + fRawHeight*boxGrowthMap.Size()(2)/255.0f;
        }
        // apply sink factor
        fHeight -= eph->m_fParticlesSinkFactor*cgParticle.fSize*2.0f;
        // also sink by maximum slope
        FLOAT fSlopeSink = Max(fSlopeX, fSlopeY);
        if (fSlopeSink>1.5f) {
          continue; // if too great slope, don't render it
        }
        fHeight -= cgParticle.fSize*fSlopeSink*0.75f; // don't sink too much
        
        cgParticle.vRender = FLOAT3D (iR, fHeight, jR);
        
        ULONG ulTmp = ptdGrowthMap->GetTexel(ulX1, ulY1); 
        ULONG ulType = (((ulTmp>>24)&0xFF)*(eph->m_iGrowthMapX*eph->m_iGrowthMapY))>>8;
        cgParticle.iShapeX = ulType % eph->m_iGrowthMapX;
        cgParticle.iShapeY = ulType / eph->m_iGrowthMapX;

        cgParticle.ubShade = (ulTmp)&0xFF; 

        cgc->acgParticles.Push() = cgParticle;
      // these particles are not visible
      } else {
        cgParticle.ubShade = 0;
        cgc->acgParticles.Push() = cgParticle;
      }
    }
  }
  return TRUE;
}

void Particles_Growth(CEntity *pen, CTextureData *ptdGrowthMap, FLOATaabbox3D &boxGrowthMap, CEntity *penEPH, INDEX iDrawPort)
{

  if (!UpdateGrowthCache( pen, ptdGrowthMap, boxGrowthMap, penEPH, iDrawPort)) {
    return;
  }

  // obtain pointer to environment particles holder
  CEnvironmentParticlesHolder *eph = (CEnvironmentParticlesHolder *)&*penEPH;  
  if (eph->m_moParticleTextureHolder.mo_toTexture.GetData() == NULL) {
    return;
  }
  
  // calculate viewer position
  FLOAT3D vPos = prPlayerProjection->pr_vViewerPosition;

  FLOAT fRangeMod = Clamp(gfx_fEnvParticlesRange, 0.1f, 2.0f);
  FLOAT GROWTH_RENDERING_RADIUS_FADE = eph->m_fGrowthRenderingRadius*fRangeMod;
  FLOAT GROWTH_RENDERING_RADIUS_OPAQUE = (eph->m_fGrowthRenderingRadius - eph->m_fGrowthRenderingRadiusFade)*fRangeMod;

  // fill structures from cache
  CGrowthCache *cgc = NULL;
  {FOREACHINLIST(CGrowthCache, cgc_Node, eph->lhCache, itCache)
    if (itCache->ulID==static_cast<ULONG>(iDrawPort)) cgc = itCache;
  }
  ASSERT(cgc!=NULL);
  static CStaticStackArray<CGrowth> acgDraw;
  for ( INDEX i=0; i<cgc->acgParticles.Count(); i++ )
  {
    if (cgc->acgParticles[i].ubShade!=0)
    {
      CGrowth *cgParticle = &cgc->acgParticles[i];

      // calculate distance to viewer by projecting the particle vector onto the viewers z
      cgParticle->fDistanceToViewer = (vPos - cgParticle->vRender)%(prPlayerProjection->pr_ViewerRotationMatrix.GetRow(3));
              
      // continue only with particles that are in front of the player
      if (cgParticle->fDistanceToViewer>0.0f) {
      // calculate fade value
        FLOAT fFadeOutStrip = GROWTH_RENDERING_RADIUS_FADE - GROWTH_RENDERING_RADIUS_OPAQUE;
        
        //UBYTE ubFade = (UBYTE)(((GROWTH_RENDERING_RADIUS_FADE - cgParticle->fDistanceToViewer) / fFadeOutStrip)*255.0f);
        if ( cgParticle->fDistanceToViewer < GROWTH_RENDERING_RADIUS_OPAQUE) {
          cgParticle->ubFade = 255;
          acgDraw.Push() = *cgParticle;
        }
        else if ( cgParticle->fDistanceToViewer < GROWTH_RENDERING_RADIUS_FADE) {
          cgParticle->ubFade = (UBYTE)(((GROWTH_RENDERING_RADIUS_FADE - cgParticle->fDistanceToViewer) / fFadeOutStrip)*255.0f); 
          acgDraw.Push() = *cgParticle;          
        }
      }      
    }
  }
  
  if (acgDraw.Count()<=0)
  {
    return;
  }
  // sort particles from the farthest to the nearest
  qsort(&acgDraw[0], acgDraw.sa_UsedCount, sizeof(CGrowth), qsort_CompareGrowth);
  
  // render particles
  Particle_PrepareTexture( &(eph->m_moParticleTextureHolder.mo_toTexture), PBT_BLEND);
  for(INDEX p=0; p<acgDraw.sa_UsedCount; p++){
    INDEX iMapTileSizeX = eph->m_moParticleTextureHolder.mo_toTexture.GetWidth() / eph->m_iGrowthMapX;
    INDEX iMapTileSizeY = eph->m_moParticleTextureHolder.mo_toTexture.GetHeight() / eph->m_iGrowthMapY;
    COLOR col = RGBToColor(acgDraw[p].ubShade, acgDraw[p].ubShade, acgDraw[p].ubShade)|acgDraw[p].ubFade;
    Particle_SetTexturePart( iMapTileSizeX, iMapTileSizeY, acgDraw[p].iShapeX, acgDraw[p].iShapeY);
    /*
    Particle_RenderLine( acgDraw[p].vRender+FLOAT3D(0,acgDraw[p].fSize*2.0f,0), acgDraw[p].vRender,
      acgDraw[p].fSize, col);
      */
    //Particle_RenderSquare( acgDraw[p].vRender, acgDraw[p].fSize, 0, col);
    
    // radius
    FLOAT fR=acgDraw[p].fSize;
    /*
    FLOAT fRndA=afStarsPositions[INDEX(acgDraw[p].vRender(1)*12345.65432)%CT_MAX_PARTICLES_TABLE][0]*360.0f;
    FLOAT fdx=fR*SinFast(fRndA);
    FLOAT fdz=fR*CosFast(fRndA);

    FLOAT3D v0=acgDraw[p].vRender+FLOAT3D(-fdx,fR,-fdz);
    FLOAT3D v1=acgDraw[p].vRender+FLOAT3D(-fdx,-fR,-fdz);
    FLOAT3D v2=acgDraw[p].vRender+FLOAT3D(fdx,-fR,fdz);
    FLOAT3D v3=acgDraw[p].vRender+FLOAT3D(fdx,fR,fdz);
    Particle_RenderQuad3D(v0, v1, v2, v3, col);
    */

    // along x
    FLOAT3D v0=acgDraw[p].vRender+FLOAT3D(-fR,fR,0);
    FLOAT3D v1=acgDraw[p].vRender+FLOAT3D(-fR,-fR,0);
    FLOAT3D v2=acgDraw[p].vRender+FLOAT3D(fR,-fR,0);
    FLOAT3D v3=acgDraw[p].vRender+FLOAT3D(fR,fR,0);
    Particle_RenderQuad3D(v0, v1, v2, v3, col);

    // along y
    v0=acgDraw[p].vRender+FLOAT3D(0,fR,-fR);
    v1=acgDraw[p].vRender+FLOAT3D(0,-fR,-fR);
    v2=acgDraw[p].vRender+FLOAT3D(0,-fR,fR);
    v3=acgDraw[p].vRender+FLOAT3D(0,fR,fR);
    Particle_RenderQuad3D(v0, v1, v2, v3, col);
  }
  Particle_Flush();
  acgDraw.PopAll();  
}

/*void Particles_Growth123(CEntity *pen, CTextureData *ptdGrowthMap, FLOATaabbox3D &boxGrowthMap, CEntity *penEPH, INDEX dummy)
{
  
  CEnvironmentParticlesHolder *eph = (CEnvironmentParticlesHolder *)&*penEPH;  
  
  if (eph->m_moParticleTextureHolder.mo_toTexture.GetData() == NULL) {
    return;
  }
  
  FLOAT3D vPos = prPlayerProjection->pr_vViewerPosition;

  PIX pixGrowthMapH;
  PIX pixGrowthMapW;
  
  if( ptdGrowthMap  != NULL)
  {
    pixGrowthMapW = ptdGrowthMap->GetPixWidth();
    pixGrowthMapH = ptdGrowthMap->GetPixHeight();
  } else {
    return;
  }
  
  FLOAT3D vRender;
  FLOAT texX; 
  FLOAT texY;
  FLOAT fRawHeight;
  
  FLOAT GROWTH_RENDERING_STEP = eph->m_fGrowthRenderingStep;  
  FLOAT GROWTH_RENDERING_RADIUS_OPAQUE = eph->m_fGrowthRenderingRadius - eph->m_fGrowthRenderingRadiusFade;
  FLOAT GROWTH_RENDERING_RADIUS_FADE = eph->m_fGrowthRenderingRadius;
  BOOL  GROWTH_HIGHRES_MAP = eph->m_bGrowthHighresMap;
  
  ASSERT(GROWTH_RENDERING_RADIUS_FADE>=GROWTH_RENDERING_RADIUS_OPAQUE);
  
  FLOAT fGridStep;
  ULONG fXSpan;
  UBYTE ubFade=0xff;
  
  fGridStep = GROWTH_RENDERING_STEP;
  fXSpan = 1234;
  
  INDEX iGridX1 = GROWTH_RENDERING_RADIUS_FADE/GROWTH_RENDERING_STEP;
  INDEX iGridX0 = -iGridX1;
  INDEX iGridY1 = iGridX1;
  INDEX iGridY0 = -iGridX1;
    
  static CStaticStackArray<CGrowth> acgParticles;
  CGrowth cgParticle;
  
  // snap viewer to grid
  FLOAT3D vSnapped = vPos;
  SnapFloat(vSnapped(1), GROWTH_RENDERING_STEP);
  SnapFloat(vSnapped(3), GROWTH_RENDERING_STEP);

  // loop through the whole particle grid
  for ( INDEX iC=iGridY0; iC<iGridY1; iC++ )
  {
    for ( INDEX jC=iGridX0; jC<iGridX1; jC++ )
    {
      // absolute positions :
      INDEX i = iC*GROWTH_RENDERING_STEP + vSnapped(1);
      INDEX j = jC*GROWTH_RENDERING_STEP + vSnapped(3);
         
      // apply a bit of randomness:
      UBYTE ubRndFact = (i*fXSpan+j)%CT_MAX_PARTICLES_TABLE;
      
      FLOAT iR, jR;
      iR = (FLOAT)i + fGridStep * afStarsPositions[ubRndFact][0];
      jR = (FLOAT)j + fGridStep * afStarsPositions[ubRndFact][2];
      
      // size: 
      cgParticle.fSize = Lerp(eph->m_fGrowthMinSize, eph->m_fGrowthMaxSize, 
        afStarsPositions[ubRndFact][2]+0.5f);
      
      texX = (iR-boxGrowthMap.Min()(1))/boxGrowthMap.Size()(1)*pixGrowthMapW;
      texY = (jR-boxGrowthMap.Min()(3))/boxGrowthMap.Size()(3)*pixGrowthMapH;
      
      // particles that fall inside the boundaries
      if ((texX>0) && (texX<pixGrowthMapW) && (texY>0) && (texY<pixGrowthMapH))
      {
        // bilinear sampling of height data
        texX -= 0.5f;
        texY -= 0.5f;
        ULONG ulX1 = (ULONG) texX;          if (ulX1<0) ulX1=0;
        ULONG ulX2 = (ULONG) ceilf(texX);   if (ulX2>=pixGrowthMapW) ulX2=pixGrowthMapW-1;
        ULONG ulY1 = (ULONG) texY;          if (ulY1<0) ulY1=0;
        ULONG ulY2 = (ULONG) ceilf(texY);   if (ulY2>=pixGrowthMapH) ulY2=pixGrowthMapH-1;
        
        ULONG ulUL, ulUR, ulBL, ulBR;
        if (GROWTH_HIGHRES_MAP)
        {
          ulUL = (ptdGrowthMap->GetTexel(ulX1, ulY1)>>8)&0xFFFF; 
          ulUR = (ptdGrowthMap->GetTexel(ulX2, ulY1)>>8)&0xFFFF; 
          ulBL = (ptdGrowthMap->GetTexel(ulX1, ulY2)>>8)&0xFFFF; 
          ulBR = (ptdGrowthMap->GetTexel(ulX2, ulY2)>>8)&0xFFFF; 
        }
        else
        {
          ulUL = (ptdGrowthMap->GetTexel(ulX1, ulY1)>>8)&0xFF; 
          ulUR = (ptdGrowthMap->GetTexel(ulX2, ulY1)>>8)&0xFF; 
          ulBL = (ptdGrowthMap->GetTexel(ulX1, ulY2)>>8)&0xFF; 
          ulBR = (ptdGrowthMap->GetTexel(ulX2, ulY2)>>8)&0xFF; 
        }
        
        // bilinear formula
        FLOAT fDX = texX - ulX1;
        FLOAT fDY = texY - ulY1;
        fRawHeight = ulUL*(1-fDX)*(1-fDY) +
						         ulUR*(fDX - fDX*fDY) +
						         ulBL*(fDY - fDX*fDY) +
						         ulBR*(fDX*fDY);
                
        // clamp to terrain height  
        FLOAT fHeight;
        if (GROWTH_HIGHRES_MAP)
        {
          fHeight = boxGrowthMap.Min()(2) + fRawHeight*boxGrowthMap.Size()(2)/65535.0f + cgParticle.fSize;
        }
        else
        {
          fHeight = boxGrowthMap.Min()(2) + fRawHeight*boxGrowthMap.Size()(2)/255.0f;
        }
        // apply sink factor
        fHeight -= eph->m_fParticlesSinkFactor*cgParticle.fSize*2.0f;
        
        cgParticle.vRender = FLOAT3D (iR, fHeight, jR);
        
        ULONG ulTmp = ptdGrowthMap->GetTexel(ulX1, ulY1); 
        ULONG ulType = (((ulTmp>>24)&0xFF)*(eph->m_iGrowthMapX*eph->m_iGrowthMapY))/255;
        
        cgParticle.iShapeX = ulType % eph->m_iGrowthMapX;
        cgParticle.iShapeY = ulType / eph->m_iGrowthMapY;
      
        cgParticle.ubShade = (ulTmp)&0xFF; 
       
        // skip particles that are totaly black - used to make areas with
        // no particles on the height map
        if( cgParticle.ubShade==0) {
          continue;
        }

        // calculate distance to viewer by projecting the particle vector onto the viewers z
        cgParticle.fDistanceToViewer = (vPos - cgParticle.vRender)%(prPlayerProjection->pr_ViewerRotationMatrix.GetRow(3));
        
        // continue only with particles that are in front of the player
        if (cgParticle.fDistanceToViewer>0.0f) {
          // calculate fade value
          FLOAT fFadeOutStrip = GROWTH_RENDERING_RADIUS_FADE - GROWTH_RENDERING_RADIUS_OPAQUE;
          
          cgParticle.ubFade = (UBYTE)(((GROWTH_RENDERING_RADIUS_FADE - cgParticle.fDistanceToViewer) / fFadeOutStrip)*255.0f);
          if ( cgParticle.fDistanceToViewer < GROWTH_RENDERING_RADIUS_OPAQUE) {
            cgParticle.ubFade = 255;
            acgParticles.Push() = cgParticle;
            //acgParticlesSolid.Push() = cgParticle;
          }
          else if ( cgParticle.fDistanceToViewer < GROWTH_RENDERING_RADIUS_FADE) {
            cgParticle.ubFade = (UBYTE)(((GROWTH_RENDERING_RADIUS_FADE - cgParticle.fDistanceToViewer) / fFadeOutStrip)*255.0f); 
            acgParticles.Push() = cgParticle;
            //acgParticlesFading.Push() = cgParticle;
          }
        }
      }
    }
  }
  
  // if no particles to render (at the edge of the visible box?)
  //if (acgParticlesFading.sa_UsedCount<=0 && acgParticlesSolid.sa_UsedCount<=0) return;
  if (acgParticles.sa_UsedCount<=0) return;
  
  // sort fading particles
  //qsort(&acgParticlesFading[0], acgParticlesFading.sa_UsedCount, sizeof(CGrowth), qsort_CompareGrowth);
  qsort(&acgParticles[0], acgParticles.sa_UsedCount, sizeof(CGrowth), qsort_CompareGrowth);
  
  // render particles
  Particle_PrepareTexture( &(eph->m_moParticleTextureHolder.mo_toTexture), PBT_BLEND);
  for(INDEX p=0; p<acgParticles.sa_UsedCount; p++){
    INDEX iMapTileSizeX = eph->m_moParticleTextureHolder.mo_toTexture.GetWidth() / eph->m_iGrowthMapX;
    INDEX iMapTileSizeY = eph->m_moParticleTextureHolder.mo_toTexture.GetHeight() / eph->m_iGrowthMapY;
    COLOR col = RGBToColor(acgParticles[p].ubShade, acgParticles[p].ubShade, acgParticles[p].ubShade)|acgParticles[p].ubFade;
    Particle_SetTexturePart( iMapTileSizeX, iMapTileSizeY, acgParticles[p].iShapeX, acgParticles[p].iShapeY);
    Particle_RenderSquare( acgParticles[p].vRender, acgParticles[p].fSize, 0, col);
  }
  Particle_Flush();

  // all done
  //acgParticlesFading.PopAll();
  //acgParticlesSolid.PopAll();  
  acgParticles.PopAll();  
}*/


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
      UBYTE ubR = (UBYTE) (64+afStarsPositions[(INDEX)fT0*CT_MAX_PARTICLES_TABLE][2]*64);
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


#define SNOW_SPEED 2.0f
#define YGRID_SIZE 16.0f
#define YGRIDS_VISIBLE_ABOVE 2
#define YGRIDS_VISIBLE_BELOW 1
#define SNOW_TILE_DROP_TIME (YGRID_SIZE/SNOW_SPEED)

void Particles_Snow(CEntity *pen, FLOAT fGridSize, INDEX ctGrids, FLOAT fFactor,
                    CTextureData *ptdSnowMap, FLOATaabbox3D &boxSnowMap, FLOAT fSnowStart)
{
  FLOAT3D vPos = pen->GetLerpedPlacement().pl_PositionVector;

  vPos(1) -= fGridSize*ctGrids/2;
  vPos(3) -= fGridSize*ctGrids/2;

  SnapFloat( vPos(1), fGridSize);
  SnapFloat( vPos(2), YGRID_SIZE);
  SnapFloat( vPos(3), fGridSize);  
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT tmSnowFalling=fNow-fSnowStart;
  FLOAT fFlakePath=SNOW_SPEED*tmSnowFalling;
  FLOAT fFlakeStartPos=vPos(2)-fFlakePath;
  FLOAT fSnapFlakeStartPos=fFlakeStartPos;
  SnapFloat(fSnapFlakeStartPos, YGRID_SIZE);
  //INDEX iRndFlakeStart=INDEX(fSnapFlakeStartPos)%CT_MAX_PARTICLES_TABLE;
  FLOAT tmSnapSnowFalling = tmSnowFalling;
  SnapFloat( tmSnapSnowFalling, SNOW_TILE_DROP_TIME);
  FLOAT fTileRatio = (tmSnowFalling-tmSnapSnowFalling)/SNOW_TILE_DROP_TIME;
  
  Particle_PrepareTexture(&_toSnowdrop, PBT_BLEND);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT fMinX = boxSnowMap.Min()(1);
  FLOAT fMinY = boxSnowMap.Min()(2);
  FLOAT fMinZ = boxSnowMap.Min()(3);
  FLOAT fSizeX = boxSnowMap.Size()(1);
  FLOAT fSizeY = boxSnowMap.Size()(2);
  FLOAT fSizeZ = boxSnowMap.Size()(3);
  PIX pixSnowMapW = 1;
  PIX pixSnowMapH = 1;
  
  if( ptdSnowMap != NULL)
  {
    pixSnowMapW = ptdSnowMap->GetPixWidth();
    pixSnowMapH = ptdSnowMap->GetPixHeight();
  }

  for( INDEX iZ=0; iZ<ctGrids; iZ++)
  {
    INDEX iRndZ = (ULONG(vPos(3)+iZ*fGridSize)) % CT_MAX_PARTICLES_TABLE;
    for( INDEX iX=0; iX<ctGrids; iX++)
    {
      INDEX iRndX = (ULONG(vPos(1)+iX*fGridSize)) % CT_MAX_PARTICLES_TABLE;

      INDEX iRndXZ=(iRndZ+iRndX*37)%CT_MAX_PARTICLES_TABLE;
      FLOAT fD = afStarsPositions[iRndXZ][1]*YGRID_SIZE;
      FLOAT vYStart=vPos(2)+YGRIDS_VISIBLE_ABOVE*YGRID_SIZE+fD;

      INDEX iDanceRnd=(iRndXZ+2)%CT_MAX_PARTICLES_TABLE;
      FLOAT fDanceAngle=afStarsPositions[iDanceRnd][0]*360.0f;
      FLOAT fAmpX=afStarsPositions[iDanceRnd][1]*2.0f;
      FLOAT fAmpZ=afStarsPositions[iDanceRnd][2]*2.0f;
      FLOAT fX = vPos(1) + (iX+afStarsPositions[iRndXZ][2])*fGridSize+fAmpX*sin(fDanceAngle+fNow*3.0f);
      FLOAT fZ = vPos(3) + (iZ+afStarsPositions[iRndXZ][1])*fGridSize+fAmpZ*cos(fDanceAngle+fNow*3.0f);
      FLOAT fT0 = afStarsPositions[(INDEX(2+Abs(fX)+Abs(fZ))*262147) % CT_MAX_PARTICLES_TABLE][1];

      for( INDEX iY=0; iY<(YGRIDS_VISIBLE_ABOVE+YGRIDS_VISIBLE_BELOW); iY++)
      {
        FLOAT fY = vYStart-iY*YGRID_SIZE-fTileRatio*YGRID_SIZE;
        
        UBYTE ubR = 255;
        COLOR colDrop = RGBToColor(ubR, ubR, ubR)|(UBYTE(fFactor*255.0f));
        FLOAT fSize = 0.2f+afStarsPositions[(INDEX)fT0*CT_MAX_PARTICLES_TABLE][1]*0.1f;
        FLOAT fAngle = afStarsPositions[(iRndXZ+1)%CT_MAX_PARTICLES_TABLE][1]*fNow*360.0f;
        FLOAT3D vRender = FLOAT3D( fX, fY, fZ);

        if( ptdSnowMap != NULL)
        {
          PIX pixX = PIX((vRender(1)-fMinX)/fSizeX*pixSnowMapW);
          PIX pixZ = PIX((vRender(3)-fMinZ)/fSizeZ*pixSnowMapH);

          if (pixX>=0 && pixX<pixSnowMapW 
            &&pixZ>=0 && pixZ<pixSnowMapH) {
            COLOR col = ptdSnowMap->GetTexel( pixX, pixZ);
            FLOAT fRawHeight=(col>>8)&0xFFFF; 
            FLOAT fSnowMapY = fMinY+fRawHeight*fSizeY/65535.0f;
            FLOAT fSnowY = vRender(2);
            // if tested raindrop is below ceiling
            if( fSnowY<=fSnowMapY)
            {
              // don't render it
              continue;
            } else if (fSnowY-fSize<fSnowMapY) {
              fSize = fSnowY-fSnowMapY;
            }
          }
          else
          {
            continue;
          }
        }
        Particle_RenderSquare( vRender, fSize, fAngle, colDrop);
      }
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
  INDEX ctBranches = 0;
  INDEX ctMaxBranches = 3;
  INDEX ctKnees = 0;
  
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
  ctParticles = (INDEX) (FLOAT(ctParticles) * fPowerFactor);
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
      if (fT>(1.0f-SANDFLOW_FADE_OUT)) fFade=(1.0f-fT)*(1.0f/SANDFLOW_FADE_OUT);
      else fFade=1.0f;
      fFade *= (CT_SANDFLOW_TRAIL-iTrail)*(1.0f/CT_SANDFLOW_TRAIL);

      FLOAT3D vPos = vCenter +
        vX*(afStarsPositions[iStar][0]*fStretchAll*fPowerFactor+fHeight*fT) +
        vY*(fT*fT*-5.0f+(afStarsPositions[iStar][1]*fPowerFactor*0.1f)) +
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
  ctParticles = (INDEX) (FLOAT(ctParticles) * fPowerFactor);
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
  ctParticles = (INDEX) (FLOAT(ctParticles) * fPowerFactor);
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
    
      COLOR colLava = pTD->GetTexel( ClampUp((int)FloatToInt(fT*2048),2047), 0);
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

void Particles_BulletSpray(INDEX iRndBase, FLOAT3D vSource, FLOAT3D vGDir, enum EffectParticlesType eptType,
                           FLOAT tmSpawn, FLOAT3D vDirection, FLOAT fStretch)
{
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
  INDEX iRnd = INDEX( (tmSpawn*1000.0f)+iRndBase) &63;
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
    case EPT_BULLET_GRASS:
    {
      colSmoke = 0xFFE8C000;
      Particle_PrepareTexture(&_toBulletGrass, PBT_BLEND);
      fSizeStart = 0.15f;
      fSpeedStart = 1.75f;
      break;
    }
    case EPT_BULLET_WOOD:
    {
      colSmoke = 0xFFE8C000;
      Particle_PrepareTexture(&_toBulletWood, PBT_BLEND);
      fSizeStart = 0.15f;
      fSpeedStart = 1.25f;
      break;
    }
    case EPT_BULLET_SNOW:
    {
      colSmoke = 0xFFE8C000;
      Particle_PrepareTexture(&_toBulletSnow, PBT_BLEND);
      fSizeStart = 0.15f;
      fSpeedStart = 1.25f;
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
    FLOAT3D vPos = vSource + (vDirection+vRandomAngle)*(fT*fSpeedRnd)+vGDir*(fT*fT*fGA);

    if( (eptType == EPT_BULLET_WATER) && (vPos(2) < vSource(2)) )
    {
      continue;
    }

    FLOAT fSize = (fSizeStart + afStarsPositions[ iSpray*2+iRnd*3][0]/20.0f)*fStretch;
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
      FLOAT3D vPos0 = vSource + (vDirection+vRandomAngle)*(fT+0.00f)*12.0f;
      FLOAT3D vPos1 = vSource + (vDirection+vRandomAngle)*(fT+0.05f)*12.0f;
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
    FLOAT3D vPos = vSource - vGDir*(afStarsPositions[iRnd][0]*2.0f+1.5f)*fT;
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
  if (Abs(vY(2))>0.5f) {
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

  BOOL bVisible;
  if(pen->en_RenderType == CEntity::RT_SKAMODEL) {
    bVisible = pen->GetModelInstance()->IsModelVisible( fMipFactor);
  } else {
    bVisible = pen->en_pmoModelObject->IsModelVisible( fMipFactor);
  }
  if( !bVisible) return;

  FLOAT fTime = _pTimer->GetLerpedCurrentTick()-tmStart;
  // don't render particles before fade in and after fadeout
  if( (fTime<FADE_IN_START) || (fTime>FADE_OUT_END)) {
    return;
  }
  FLOAT fPowerTime = pow(fTime-SPIRIT_SPIRAL_START, 2.5f);

  FLOATaabbox3D box;
  if(pen->en_RenderType == CEntity::RT_SKAMODEL) {
    // fill array with absolute vertices of entity's model and its attached models
    pen->GetModelVerticesAbsolute(avVertices, 0.05f, fMipFactor); 
    // get corp size
    pen->GetModelInstance()->GetCurrentColisionBox(box);
  } else {
    // fill array with absolute vertices of entity's model and its attached models
    pen->GetModelVerticesAbsolute(avVertices, 0.05f, fMipFactor); 
    // get corp size
    pen->en_pmoModelObject->GetCurrentFrameBBox(box);
  }

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

#define MIP_FACTOR_BLEND_START 4.0f
#define MIP_FACTOR_BLEND_END 7.0f
void Particles_Burning(CEntity *pen, FLOAT fPower, FLOAT fTimeRatio)
{
  INDEX iFramesInRaw=8;
  INDEX iFramesInColumn=4;
  INDEX ctFrames=iFramesInRaw*iFramesInColumn;
  
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();

  // fill array with absolute vertices of entity's model and its attached models
  pen->GetModelVerticesAbsolute(avVertices, 0.0f, 0.0f); 

  // get entity position and orientation
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  Particle_PrepareTexture( &_toFire, PBT_ADD);

  // calculate color factor (for fade in/out)
  FLOAT fFade = fTimeRatio;
  UBYTE ubColor = UBYTE(CT_OPAQUE*fFade);
  COLOR col = RGBToColor(ubColor,ubColor,ubColor)|CT_OPAQUE;

  INDEX ctVtx = avVertices.Count();
  FLOAT fDensityFactor=1.0f-(Clamp(ctVtx, INDEX(500), INDEX(1000))-500.0f)/500.0f;

  // get corp size
  FLOATaabbox3D box;

  if(pen->en_RenderType == CEntity::RT_SKAMODEL || pen->en_RenderType == CEntity::RT_SKAEDITORMODEL) {
    pen->GetModelInstance()->GetCurrentColisionBox(box);
  } else {
    pen->GetBoundingBox(box);
  }

  FLOAT fBoxSize=box.Size().Length();
  FLOAT fBoxHeight=box.Size()(2);
  FLOAT fSizeRatio=(Clamp(fBoxSize,2.0f,12.0f)-2.0f)/10.0f;
  FLOAT fSize=0.125f+ClampDn( FLOAT(pow(box.Size()(2),1.0f/4.0f)), 1.0f)*fPower/5.0f;
  fSize+=(1.0f+fSizeRatio)*(1.0f+fSizeRatio)*0.125f;
  INDEX iVtxSteep=(INDEX) ((2+(2.0f-fSizeRatio-fDensityFactor)*6));
  if( IsOfClass(pen, "Werebull"))
  {
    iVtxSteep=2;
  }
  for( INDEX iVtx=0; iVtx<ctVtx; iVtx+=iVtxSteep)
  {
    FLOAT3D vPos = avVertices[iVtx];
    FLOAT fHighSizer=0.125f+((vPos(2)-vCenter(2))/fBoxHeight)*0.875f;
    vPos+=vY*(fSize*fHighSizer*fFade*2);
    INDEX iRnd=iVtx%CT_MAX_PARTICLES_TABLE;
    INDEX iFrame=INDEX((afStarsPositions[iRnd][0]+0.5f)*ctFrames+fNow*16.0f)%(ctFrames);

    INDEX iFrameX=iFrame%iFramesInRaw;
    INDEX iFrameY=iFrame/iFramesInRaw;
    Particle_SetTexturePart( 1024/iFramesInRaw, 1024/iFramesInColumn, iFrameX, iFrameY);
    Particle_RenderSquare( vPos, fSize*fHighSizer*fFade, 0, col, 2.0f);
  }
  avVertices.PopAll();
  Particle_Flush();
}

void Particles_Burning_Comp(CModelObject *mo, FLOAT fPower, CPlacement3D pl)
{
  INDEX iFramesInRaw=8;
  INDEX iFramesInColumn=4;
  INDEX ctFrames=iFramesInRaw*iFramesInColumn;
  
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  
  CPlacement3D plPlacement = pl;
    
  // fill array with absolute vertices of entity's model and its attached models
  FLOATmatrix3D mRotation;
  MakeRotationMatrixFast(mRotation, plPlacement.pl_OrientationAngle);
  mo->GetModelVertices( avVertices, mRotation, plPlacement.pl_PositionVector, 0.0f, 0.0f);

  // get entity position and orientation
  const FLOATmatrix3D &m = mRotation;
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = plPlacement.pl_PositionVector;

  Particle_PrepareTexture( &_toFire, PBT_ADD);

  UBYTE ubColor = UBYTE(CT_OPAQUE);
  COLOR col = RGBToColor(ubColor,ubColor,ubColor)|CT_OPAQUE;

  INDEX ctVtx = avVertices.Count();
  FLOAT fDensityFactor=1.0f-(Clamp(ctVtx, INDEX(500), INDEX(1000))-500.0f)/500.0f;

  // get corp size
  FLOATaabbox3D box;
  mo->GetAllFramesBBox(box);
  FLOAT fBoxSize=box.Size().Length();
  FLOAT fBoxHeight=box.Size()(2);
  FLOAT fSizeRatio=(Clamp(fBoxSize,2.0f,12.0f)-2.0f)/10.0f;
  //FLOAT fSize=0.125f+ClampDn( FLOAT(pow(box.Size()(2),1.0f/4.0f)), 0.1f)*fPower/5.0f;
  //fSize+=(1.0f+fSizeRatio)*(1.0f+fSizeRatio)*0.125f;
  FLOAT fSize = fPower;
  INDEX iVtxSteep=(INDEX) (2+(2.0f-fSizeRatio-fDensityFactor)*6);
  for( INDEX iVtx=0; iVtx<ctVtx; iVtx+=iVtxSteep)
  {
    FLOAT3D vPos = avVertices[iVtx];
    FLOAT fHighSizer=0.125f+((vPos(2)-vCenter(2))/fBoxHeight)*0.875f;
    vPos+=vY*(fSize*fHighSizer*2);
    INDEX iRnd=iVtx%CT_MAX_PARTICLES_TABLE;
    INDEX iFrame=INDEX((afStarsPositions[iRnd][0]+0.5f)*ctFrames+fNow*16.0f)%(ctFrames);

    INDEX iFrameX=iFrame%iFramesInRaw;
    INDEX iFrameY=iFrame/iFramesInRaw;
    Particle_SetTexturePart( 1024/iFramesInRaw, 1024/iFramesInColumn, iFrameX, iFrameY);
    Particle_RenderSquare( vPos, fSize*fHighSizer, 0, col, 2.0f);
  }
  avVertices.PopAll();
  Particle_Flush();
}

void Particles_BrushBurning(CEntity *pen, FLOAT3D vPos[], INDEX ctCount, FLOAT3D vPlane,
                            FLOAT fPower, FLOAT fTimeRatio)
{
  CEntity *penBrush = pen->GetParent();
  if (penBrush==NULL) return;

  INDEX iFramesInRaw=8;
  INDEX iFramesInColumn=4;
  INDEX ctFrames=iFramesInRaw*iFramesInColumn;
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  Particle_PrepareTexture( &_toFire, PBT_ADD);

  CPlacement3D plBrush = penBrush->GetLerpedPlacement();
  FLOAT3D vBrushPos = plBrush.pl_PositionVector;
  FLOATmatrix3D mBrushRot;
  MakeRotationMatrixFast(mBrushRot, plBrush.pl_OrientationAngle);
  
  FLOAT fFade = Clamp(fTimeRatio,0.0f, 1.0f);
  FLOAT3D vG=FLOAT3D(0,-1.0f,0);
  if( IsDerivedFromClass(pen, "MovableEntity"))
  {
    vG=((CMovableEntity *)pen)->en_vGravityDir;
  }
  FLOAT fMul=vG%vPlane;
  if( fMul>0) return;
  
  // calculate frame 3D offsets
  FLOAT3D vSlide=FLOAT3D(0,0,0);
  FLOAT3D vPerD=vG*vPlane;
  if( vPerD.Length()>0.01f)
  {
    vSlide=vPerD*vG;
    vSlide.Normalize();
  }

  INDEX iRndEn=pen->en_ulID;
  for( INDEX iFlame=0; iFlame<ctCount; iFlame++)
  {
    INDEX iRnd=(iFlame+iRndEn)%CT_MAX_PARTICLES_TABLE;
    // calculate color factor (for fade in/out)
    FLOAT fSize = 0.25f+afStarsPositions[iRnd][0]*0.3f;
    UBYTE ubR=(UBYTE) ((255.0f)*fFade);
    UBYTE ubG=(UBYTE) ((224+(afStarsPositions[iRnd][1]+0.5f)*32.0f)*fFade);
    UBYTE ubB=(UBYTE) ((224+(afStarsPositions[iRnd][2]+0.5f)*32.0f)*fFade);
    COLOR col = RGBToColor(ubR,ubG,ubB)|CT_OPAQUE;

    INDEX iFrame=INDEX((afStarsPositions[iRnd][0]+0.5f)*ctFrames+fNow*16.0f)%(ctFrames);
    INDEX iFrameX=iFrame%iFramesInRaw;
    INDEX iFrameY=iFrame/iFramesInRaw;
    Particle_SetTexturePart( 1024/iFramesInRaw, 1024/iFramesInColumn, iFrameX, iFrameY);
    Particle_RenderSquare( (vPos[iFlame]*mBrushRot)+vBrushPos
      -vG*fSize*fFade*2+vSlide*fSize*fFade, fSize*fFade, 0, col, 2.0f);
  }

  avVertices.PopAll();
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
void Particles_BloodSpray(enum SprayParticlesType sptType, FLOAT3D vSource, FLOAT3D vGDir, FLOAT fGA,
                          FLOATaabbox3D boxOwner, FLOAT3D vSpilDirection, FLOAT tmStarted, FLOAT fDamagePower,
                          COLOR colMultiply)
{
  INDEX ctSprays=BLOOD_SPRAYS;
  FLOAT fBoxSize = boxOwner.Size().Length()*0.1f;
  FLOAT fEnemySizeModifier = (fBoxSize-0.2)/1.0f+1.0f;
  FLOAT fRotation = 0.0f;

  // readout blood type
  const INDEX iBloodType = GetSP()->sp_iBlood;

  // determine time difference
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fSpeedModifier = 0;
  FLOAT fT=(fNow-tmStarted);

  // prepare texture
  switch(sptType) {
    case SPT_BLOOD:
    case SPT_SLIME:
    case SPT_GOO:
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
    case SPT_COLOREDSTONE:
    {
      Particle_PrepareTexture( &_toStonesSprayTexture, PBT_BLEND);
      fDamagePower*=2.0f;
      fGA*=2.0f;
      break;
    }
    case SPT_WOOD:
    {
      Particle_PrepareTexture( &_toWoodSprayTexture, PBT_BLEND);
      fDamagePower*=6.0f;
      fGA*=3.0f;
      break;
    }
    case SPT_TREE01:
    {
      ctSprays*=1;
      Particle_PrepareTexture( &_toWoodSprayTexture, PBT_BLEND);
      fDamagePower*=1;
      fSpeedModifier+=20;
      fRotation = fT*1000.0f;
      fGA*=4.0f;
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
    case SPT_AIRSPOUTS:
    {
      Particle_PrepareTexture( &_toAirSprayTexture, PBT_BLEND);
      break;
    }
    case SPT_PLASMA:
    {
      Particle_PrepareTexture( &_toLarvaProjectileSpray, PBT_BLEND);
      fDamagePower*=2.0f;
      break;
    }
    case SPT_NONE:
    {
      return;
    }
    default: ASSERT(FALSE);
      return;
    }

  for( INDEX iSpray=0; iSpray<ctSprays; iSpray++)
  {
    if( (sptType==SPT_FEATHER) && (iSpray==ctSprays/2) )
    {
      Particle_Flush();
      if( iBloodType==3) Particle_PrepareTexture( &_toFlowerSprayTexture, PBT_BLEND);
      else               Particle_PrepareTexture( &_toBloodSprayTexture,  PBT_BLEND);
      fDamagePower/=2.0f;
    }

    INDEX iFrame=((int(tmStarted*100.0f))%8+iSpray)%8;
    Particle_SetTexturePart( 256, 256, iFrame, 0);

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

    INDEX iRnd=(iSpray+INDEX(tmStarted*123.456))%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vRandomAngle = FLOAT3D(
      afStarsPositions[ iRnd][0]*1.75f,
      (afStarsPositions[ iRnd][1]+1.0f)*1.0f,
      afStarsPositions[ iRnd][2]*1.75f);
    FLOAT fSpilPower=vSpilDirection.Length();
    vRandomAngle=vRandomAngle.Normalize()*fSpilPower;
    fSpeedModifier+=afStarsPositions[ iSpray+ctSprays][0]*0.5f;

    FLOAT fSpeed = BLOOD_SPRAY_SPEED_MIN+(BLOOD_SPRAY_TOTAL_TIME-fT)/BLOOD_SPRAY_TOTAL_TIME;
    FLOAT3D vPos = vSource+ (vSpilDirection+vRandomAngle)*(fT*(fSpeed+fSpeedModifier))+vGDir*(fT*fT*fGA/4.0f);
  
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
    case SPT_GOO:
    {
      UBYTE ubRndCol = UBYTE( 128+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*64);
      if( iBloodType!=3) col = RGBAToColor(ubRndCol, 128, 12, ubAlpha);
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
      if(iSpray>=ctSprays/2)
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
    case SPT_COLOREDSTONE:
    {
      UBYTE ubH,ubS,ubV;
      ColorToHSV( colMultiply, ubH, ubS, ubV);
      UBYTE ubRndH = Clamp(ubH+INDEX(afStarsPositions[ int(iSpray+tmStarted*6)%CT_MAX_PARTICLES_TABLE][0]*16),INDEX(0),INDEX(255));
      UBYTE ubRndS = Clamp(ubS+INDEX(afStarsPositions[ int(iSpray+tmStarted*7)%CT_MAX_PARTICLES_TABLE][1]*64),INDEX(0),INDEX(255));
      UBYTE ubRndV = Clamp(ubV+INDEX(afStarsPositions[ int(iSpray+tmStarted*8)%CT_MAX_PARTICLES_TABLE][2]*64),INDEX(0),INDEX(255));
      col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
      fSize*=0.10f/2.0f;
      fRotation = fT*50.0f;
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
    case SPT_TREE01:
    {
      UBYTE ubRndH = UBYTE( 16+afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][0]*16);
      UBYTE ubRndS = UBYTE( 96+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][1]+0.5)*32);
      UBYTE ubRndV = UBYTE( 96+(afStarsPositions[ int(iSpray+tmStarted*10)%CT_MAX_PARTICLES_TABLE][2])*96);
      col = HSVToColor(ubRndH, ubRndS, ubRndV)|ubAlpha;
      fSize*=0.2f;
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
    case SPT_AIRSPOUTS:
    {
      col = C_WHITE|(ubAlpha>>1);
      break;
    }
    }
    Particle_RenderSquare( vPos, 0.25f*fSize*fSizeModifier, fRotation, MulColors(col,colMultiply));
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

#define CT_AFTERBURNER_SMOKES 32
#define CT_AFTERBURNER_HEAD_POSITIONS 5
#define CT_AFTERBURNER_HEAD_INTERPOSITIONS 4
void Particles_AfterBurner_Prepare(CEntity *pen)
{
  pen->GetLastPositions(CT_AFTERBURNER_SMOKES);
}

void Particles_AfterBurner(CEntity *pen, FLOAT tmSpawn, FLOAT fStretch, INDEX iGradientType)
{
  FLOAT3D vGDir = ((CMovableEntity *)pen)->en_vGravityDir;
  FLOAT fGA = ((CMovableEntity *)pen)->en_fGravityA;

  CLastPositions *plp = pen->GetLastPositions(CT_AFTERBURNER_SMOKES);
  Particle_PrepareTexture(&_toAfterBurner,  PBT_BLEND);
  CTextureData *pTD;
  switch( iGradientType)
  {
  case 0:
  default:
    pTD=(CTextureData *) _toAfterBurnerGradient.GetData();
    break;
  case 1:
    pTD=(CTextureData *) _toAfterBurnerGradientBlue.GetData();
    break;
  case 2:
    pTD=(CTextureData *) _toAfterBurnerGradientMeteor.GetData();
    break;
  }

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(plp->lp_ctUsed-1);
  
  ULONG *pcolFlare=pTD->GetRowPointer(0); // flare color
  ULONG *pcolExp=pTD->GetRowPointer(1);   // explosion color
  ULONG *pcolSmoke=pTD->GetRowPointer(2); // smoke color
  FLOAT aFlare_sol[256], aFlare_vol[256], aFlare_wol[256], aFlare_rol[256];
  FLOAT aExp_sol[256], aExp_vol[256], aExp_wol[256], aExp_rol[256];
  FLOAT aSmoke_sol[256], aSmoke_vol[256], aSmoke_rol[256];
  
  pTD->FetchRow( 4, aFlare_sol);
  pTD->FetchRow( 5, aFlare_vol);
  pTD->FetchRow( 6, aFlare_wol);
  pTD->FetchRow( 7, aFlare_rol);
  pTD->FetchRow( 8, aExp_sol);
  pTD->FetchRow( 9, aExp_vol);
  pTD->FetchRow(10, aExp_wol);
  pTD->FetchRow(11, aExp_rol);
  pTD->FetchRow(12, aSmoke_sol);
  pTD->FetchRow(13, aSmoke_vol);
  pTD->FetchRow(14, aSmoke_rol);

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();  
  for(INDEX iPos = plp->lp_ctUsed-1; iPos>=1; iPos--)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    if( *pvPos1==*pvPos2) continue;

    FLOAT fT=(iPos+_pTimer->GetLerpFactor())*_pTimer->TickQuantum;
    FLOAT fRatio=fT/(CT_AFTERBURNER_SMOKES*_pTimer->TickQuantum);
    INDEX iIndex=(INDEX) (fRatio*255);
    INDEX iRnd=(INDEX)(size_t(pvPos1)%CT_MAX_PARTICLES_TABLE);

    // smoke
    FLOAT3D vPosS = *pvPos1;
    Particle_SetTexturePart( 512, 512, 1, 0);
    FLOAT fAngleS = afStarsPositions[iRnd][1]*360.0f+fT*120.0f*afStarsPositions[iRnd][2];
    FLOAT fSizeS = (0.5f+aSmoke_sol[iIndex]*2.5f)*fStretch;
    FLOAT3D vVelocityS=FLOAT3D(afStarsPositions[iRnd][1],
                               afStarsPositions[iRnd][2],
                               afStarsPositions[iRnd][0])*5.0f;
    vPosS=vPosS+vVelocityS*fT+vGDir*fGA/2.0f*(fT*fT)/32.0f;
    Particle_RenderSquare( vPosS, fSizeS, fAngleS, ByteSwap(pcolSmoke[iIndex]));

    // explosion
    FLOAT3D vPosE = (*pvPos1+*pvPos2)/2.0f;//Lerp(*pvPos1, *pvPos2, _pTimer->GetLerpFactor());
    Particle_SetTexturePart( 512, 512, 0, 0);
    FLOAT fAngleE = afStarsPositions[iRnd][0]*360.0f;//+fT*360.0f;
    FLOAT fSizeE = (0.5f+aExp_sol[iIndex]*2.0f)*fStretch;
    FLOAT3D vVelocityE=FLOAT3D(afStarsPositions[iRnd][0], 
                               afStarsPositions[iRnd][1],
                               afStarsPositions[iRnd][2])*3.0f;
    vPosE=vPosE+vVelocityE*fT+vGDir*fGA/2.0f*(fT*fT)/32.0f;
    Particle_RenderSquare( vPosE, fSizeE, fAngleE, ByteSwap(pcolExp[iIndex]));

  }
  // all done
  Particle_Flush();

  if( IsOfClass(pen, "PyramidSpaceShip")) return;
  Particle_PrepareTexture(&_toAfterBurnerHead, PBT_ADDALPHA);
  Particle_SetTexturePart( 1024, 1024, 0, 0);
  FLOAT fColMul=1.0f;
  if( (fNow-tmSpawn)<1.0f)
  {
    fColMul=CalculateRatio(fNow-tmSpawn, 0, 1, 0.5f, 0.0f);
  }
  UBYTE ubColMul=UBYTE(CT_OPAQUE*fColMul);
  COLOR colMul=RGBAToColor(ubColMul,ubColMul,ubColMul,ubColMul);

  INDEX ctParticles=CT_AFTERBURNER_HEAD_POSITIONS;//Min(CT_AFTERBURNER_HEAD_POSITIONS,plp->lp_ctUsed-1);
  pvPos1 = &plp->GetPosition(ctParticles-1);
  for(INDEX iFlare=ctParticles-2; iFlare>=0; iFlare--)
  {
    pvPos2 = pvPos1;
    pvPos1 = &plp->GetPosition(iFlare);
    if( *pvPos1==*pvPos2) continue;

    for (INDEX iInter=CT_AFTERBURNER_HEAD_INTERPOSITIONS-1; iInter>=0; iInter--)
    {
      FLOAT fT=(iFlare+_pTimer->GetLerpFactor()+iInter*1.0f/CT_AFTERBURNER_HEAD_INTERPOSITIONS)*_pTimer->TickQuantum;
      FLOAT fRatio=fT/(ctParticles*_pTimer->TickQuantum);
      INDEX iIndex=(INDEX) (fRatio*255);
      FLOAT fSize = (aFlare_sol[iIndex]*2.0f)*fStretch;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/CT_AFTERBURNER_HEAD_INTERPOSITIONS);
      FLOAT fAngle = afStarsPositions[iInter][0]*360.0f+fRatio*360.0f;
      Particle_RenderSquare( vPos, fSize, fAngle,  MulColors( ByteSwap(pcolFlare[iIndex]), colMul));
    }
  }

  // all done
  Particle_Flush();
}

#define CT_ROCKETMOTOR_FLARES_BLEND ctCount
#define TM_ROCKETMOTOR_BLEND 1.5f
#define CT_ROCKETMOTOR_FLARES_ADD ctCount
#define TM_ROCKETMOTOR_ADD 1.0f
void Particles_RocketMotorBurning(CEntity *pen, FLOAT tmSpawn, FLOAT3D vStretch, FLOAT fStretch, FLOAT ctCount)
{
  FLOAT fMipDisappearDistance=13.0f;
  FLOAT fMipFactor = Particle_GetMipFactor();
  if( fMipFactor>fMipDisappearDistance) return;
  //FLOAT fMipBlender=CalculateRatio(fMipFactor, 0.0f, fMipDisappearDistance, 0.0f, 0.1f);

  CPlacement3D pl = pen->GetLerpedPlacement();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pl.pl_PositionVector;

  CTextureData *pTD = (CTextureData *) _toAfterBurnerGradient.GetData();

  //ULONG *pcolFlare=pTD->GetRowPointer(0); // flare color
  ULONG *pcolExp=pTD->GetRowPointer(1);   // explosion color
  ULONG *pcolSmoke=pTD->GetRowPointer(2); // smoke color
  FLOAT aFlare_sol[256], aFlare_vol[256], aFlare_wol[256], aFlare_rol[256];
  FLOAT aExp_sol[256], aExp_vol[256], aExp_wol[256], aExp_rol[256];
  FLOAT aSmoke_sol[256], aSmoke_vol[256], aSmoke_rol[256];
  
  pTD->FetchRow( 4, aFlare_sol);
  pTD->FetchRow( 5, aFlare_vol);
  pTD->FetchRow( 6, aFlare_wol);
  pTD->FetchRow( 7, aFlare_rol);
  pTD->FetchRow( 8, aExp_sol);
  pTD->FetchRow( 9, aExp_vol);
  pTD->FetchRow(10, aExp_wol);
  pTD->FetchRow(11, aExp_rol);
  pTD->FetchRow(12, aSmoke_sol);
  pTD->FetchRow(13, aSmoke_vol);
  pTD->FetchRow(14, aSmoke_rol);

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();  

  Particle_PrepareTexture(&_toAfterBurner, PBT_BLEND);
  for(INDEX iFlare=0; iFlare<CT_ROCKETMOTOR_FLARES_BLEND; iFlare++)
  {
    INDEX iRnd =(pen->en_ulID+iFlare)%CT_MAX_PARTICLES_TABLE;
    FLOAT fFireDelta=TM_ROCKETMOTOR_BLEND/CT_ROCKETMOTOR_FLARES_BLEND;
    FLOAT fT = fNow+iFlare*fFireDelta+afTimeOffsets[iFlare]*fFireDelta*0.1f;
    // apply time strech
    fT *= 1/TM_ROCKETMOTOR_BLEND;
    // get fraction part
    fT = fT-int(fT);
    FLOAT3D vSpeed=FLOAT3D(
      afStarsPositions[iRnd][0]*0.75f*vStretch(1),
      afStarsPositions[iRnd][1]*0.5f*vStretch(2),
      afStarsPositions[iRnd][2]*0.75f)*vStretch(3);
    FLOAT fPosY=(0.926+0.202*(log(fT+0.01f))+0.2f)*60.0f;
    FLOAT fSpeed=25.0f+(afStarsPositions[iRnd][0]+0.5f)*10.0f;
    FLOAT3D vPosS=vCenter+vSpeed*fSpeed*fT;
    vPosS=vPosS+vY*fPosY;
    INDEX iIndex=(INDEX) (fT*255);
    // smoke
    Particle_SetTexturePart( 512, 512, 1, 0);
    FLOAT fAngleS = afStarsPositions[iRnd][1]*360.0f+fT*120.0f*afStarsPositions[iRnd][2];
    FLOAT fSizeS = (3.0f+fT*4.5f)*fStretch;
    Particle_RenderSquare( vPosS, fSizeS, fAngleS, ByteSwap(pcolSmoke[iIndex]));

    // explosion
    Particle_SetTexturePart( 512, 512, 0, 0);
    FLOAT3D vPosE=vCenter+vSpeed*fSpeed*(fT+0.01f);
    vPosE=vPosS-vY*0.1f;
    FLOAT fAngleE = afStarsPositions[iRnd][0]*360.0f;
    FLOAT fSizeE = (2.5f+fT*4.0f)*fStretch;
    Particle_RenderSquare( vPosE, fSizeE, fAngleE, ByteSwap(pcolExp[iIndex]));
  }
  Particle_Flush();

  FLOAT fFireStretch=0.6f;
  Particle_PrepareTexture(&_toAfterBurner,  PBT_ADDALPHA);
  for(INDEX iFlame=0; iFlame<CT_ROCKETMOTOR_FLARES_ADD; iFlame++)
  {
    INDEX iRnd =(3+pen->en_ulID+iFlame)%CT_MAX_PARTICLES_TABLE;
    FLOAT fFireDelta=TM_ROCKETMOTOR_ADD/CT_ROCKETMOTOR_FLARES_ADD;
    FLOAT fT = fNow+iFlame*fFireDelta+afTimeOffsets[iFlame]*fFireDelta*0.1f;
    // apply time strech
    fT *= 1/TM_ROCKETMOTOR_ADD;
    // get fraction part
    fT = fT-int(fT);
    FLOAT3D vSpeed=FLOAT3D(afStarsPositions[iRnd][0]*0.15f,
      afStarsPositions[iRnd][1]*0.01f,afStarsPositions[iRnd][2]*0.15f);
    FLOAT fPosY=(0.926+0.202*(log(fT+0.01f)))*60.0f/1.75f;
    FLOAT fSpeed=25.0f+(afStarsPositions[iRnd][0]+0.5f)*2.0f;
    FLOAT3D vPosS=vCenter+vSpeed*fSpeed*fT;
    vPosS=vPosS+vY*fPosY;
    INDEX iIndex=(INDEX) (fT*255);
    // smoke
    Particle_SetTexturePart( 512, 512, 1, 0);
    FLOAT fAngleS = afStarsPositions[iRnd][1]*360.0f+fT*120.0f*afStarsPositions[iRnd][2];
    FLOAT fSizeS = (1.5f+aSmoke_sol[iIndex]*2.5f)*fStretch*fFireStretch;
    Particle_RenderSquare( vPosS, fSizeS, fAngleS, ByteSwap(pcolSmoke[iIndex]));

    // explosion
    Particle_SetTexturePart( 512, 512, 0, 0);
    FLOAT3D vPosE=vCenter+vSpeed*fSpeed*(fT+0.01f);
    vPosE=vPosS-vY*0.1f;
    FLOAT fAngleE = afStarsPositions[iRnd][0]*360.0f;
    FLOAT fSizeE = (1.5f+aExp_sol[iIndex]*2.0f)*fStretch*fFireStretch;
    Particle_RenderSquare( vPosE, fSizeE, fAngleE, ByteSwap(pcolExp[iIndex]));
  }
  // all done
  Particle_Flush();
}

#define SP_EXPLODE_PARTICLES 25
void Particles_SummonerProjectileExplode(CEntity *pen, FLOAT fSize, FLOAT fBeginTime, FLOAT fDuration, FLOAT fTimeAdjust)
{
  INDEX iRnd = (INDEX)(fTimeAdjust*10.0f)%(CT_MAX_PARTICLES_TABLE-SP_EXPLODE_PARTICLES);

  FLOAT fElapsed = _pTimer->GetLerpedCurrentTick() - fBeginTime;
  Particle_PrepareTexture(&_toRocketTrail, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;
  fSize *= 2.0f;

  CTextureData *pTD = (CTextureData *) _toLavaBombTrailGradient.GetData();

  for( INDEX iParticle=0; iParticle<SP_EXPLODE_PARTICLES; iParticle++)
  {
    FLOAT3D vPos;
    FLOAT fAngle = NormalizeAngle((fElapsed+afTimeOffsets[iParticle+iRnd])*5.0f);
    FLOAT fParticleSize = fSize*fElapsed;
    
    vPos = FLOAT3D(afStarsPositions[iParticle+iRnd][0], 
                   afStarsPositions[iParticle+iRnd][1],
                   afStarsPositions[iParticle+iRnd][2])*fSize + vCenter;

    vPos(1) += pow(fElapsed*afStarsPositions[iParticle+iRnd][0], 2.0f);
    vPos(2) += pow(fElapsed*(afStarsPositions[iParticle+iRnd][1]+0.5f)*4.0f, 0.5f);
    vPos(3) += pow(fElapsed*afStarsPositions[iParticle+iRnd][2], 2.0f);

    COLOR colColor;
    UBYTE ub;
    FLOAT fFadeBegin = fDuration*0.7f;

    if (fElapsed>fFadeBegin) {
      ub = (UBYTE) (255 - (fElapsed-fFadeBegin)/(fDuration-fFadeBegin)*255);
    }
    if (fElapsed>fDuration) ub = 0;

    colColor = pTD->GetTexel(PIX(fElapsed*1024), 0);
    colColor |= ub;
    
    Particle_RenderSquare( vPos, fParticleSize, fAngle, colColor);
  }

  // all done
  Particle_Flush();
}

#define SUM_EXPLODE_PARTICLES 1024
void Particles_SummonerExplode(CEntity *pen, FLOAT3D vCenter, FLOAT fArea, FLOAT fSize, FLOAT fBeginTime, FLOAT fDuration)
{
  FLOAT fElapsed = _pTimer->GetLerpedCurrentTick() - fBeginTime;
  Particle_PrepareTexture(&_toStar07, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

    
  for( INDEX iParticle=0; iParticle<SUM_EXPLODE_PARTICLES; iParticle++)
  {
    FLOAT3D vPos;
    FLOAT fAngle = auStarsColors[iParticle][0];
    
    if (afStarsPositions[iParticle][1]<0.0f) {
      vPos = FLOAT3D(-afStarsPositions[iParticle][0], 
                     -afStarsPositions[iParticle][1],
                     -afStarsPositions[iParticle][2]);
    } else {
      vPos = FLOAT3D(afStarsPositions[iParticle][0], 
                     afStarsPositions[iParticle][1],
                     afStarsPositions[iParticle][2]);
    }
    
    FLOAT fAreaModificator = afStarsPositions[iParticle][2] + 1.0f;

    vPos *= (10.0f - 10.0f/(fElapsed + 1.0f))*fArea*fAreaModificator;
    vPos(2) -=  fArea/30.0f * fElapsed * fElapsed;
    vPos += vCenter;

    COLOR colColor;
    UBYTE ub;
    FLOAT fFadeBegin = fDuration*(0.7f+afStarsPositions[iParticle][2]*0.1f);
    FLOAT fFadeEnd = fDuration*(0.9f+afStarsPositions[iParticle][1]*0.2f);


    if (fElapsed<fFadeBegin) {
      ub = 255;
    } else if (fElapsed<fFadeEnd) {
      ub = (UBYTE) (((fFadeEnd - fElapsed)/(fFadeEnd - fFadeBegin))*255);
    } else {
      ub = 0;
    }

    colColor = RGBToColor(auStarsColors[iParticle][0],
                          auStarsColors[iParticle][1],
                          auStarsColors[iParticle][2]);
    colColor |= ub;
    
    Particle_RenderSquare( vPos, fSize, fAngle, colColor);
  }

  // all done
  Particle_Flush();

}

#define TM_TWISTER_TOTAL_LIFE 10.0f
void Particles_Twister( CEntity *pen, FLOAT fStretch, FLOAT fStartTime, FLOAT fFadeOutStartTime, FLOAT fParticleStretch)
{
  FLOAT fMipFactor = Particle_GetMipFactor();
  FLOAT fMipStretch = 1.0f+ClampDn(7.0f-fMipFactor,0.0f)/2.0f;
  
  //CPrintF("fMipStretch=%g\n",fMipStretch);
  fParticleStretch/=fMipStretch;

  FLOAT fFadeOut=1.0f;
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fBirthStretcher = 1.0f;
  if( fFadeOutStartTime<fNow)
  {
    fFadeOut=CalculateRatio(fNow, fFadeOutStartTime, fFadeOutStartTime+2.0f, 0.0f, 1.0f);
  }
  else if(fNow<fStartTime+1.0f)
  {
    fBirthStretcher=CalculateRatio(fNow, fStartTime, fStartTime+1.0f, 1.0f, 0.0f);
  }
  fStretch*=fBirthStretcher;
  FLOAT fDisperseFactor=pow(2.0f-fFadeOut, 3.0f);

  Particle_PrepareTexture( &_toTwister, PBT_BLEND);
  CTextureData *pTD = (CTextureData *) _toTwisterGradient.GetData();
  FLOAT arol[256], asol[256];
  pTD->FetchRow( 1, arol);
  pTD->FetchRow( 2, asol);

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  vX=vX*fStretch*fDisperseFactor;
  vY=vY*fStretch;
  vZ=vZ*fStretch*fDisperseFactor;
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*0.0f;
  INDEX ctSmokes=128;
  for(INDEX i=0; i<ctSmokes; i++)
  {
    INDEX iRnd =(pen->en_ulID+i)%CT_MAX_PARTICLES_TABLE;
    INDEX iFrame=1+iRnd%7;
    Particle_SetTexturePart( 128, 128, iFrame, 0);
    FLOAT fT = tmNow+afTimeOffsets[i];
    // apply time strech
    fT *= 1/TM_TWISTER_TOTAL_LIFE;
    // get fraction part
    fT = fT-int(fT);
    INDEX iPos=(INDEX) (fT*255);
    //FLOAT fSlowFactor=1.0f-fT*0.25f;
    FLOAT fSpeed=25.0f+(afStarsPositions[iRnd][0]+0.5f)*2.0f;
    FLOAT fR=arol[iPos]*8.0f;
    FLOAT3D vPos=vCenter+vY*fSpeed*fT+
      vX*(fR*Sin(fT*360.0f*16.0f)/*+Sin((fNow+fT)*40.0f)*fT*8.0f*/)+
      vZ*(fR*Cos(fT*360.0f*16.0f)/*+Cos((fNow+fT)*40.0f)*fT*8.0f*/);
    FLOAT fT2=fT+0.05f*(0.5f+fT);
    FLOAT3D vPos2=vCenter+vY*fSpeed*fT2+
      vX*(fR*Sin(fT2*360.0f*16.0f)/*+Sin((fNow+fT2)*40.0f)*fT2*8.0f*/)+
      vZ*(fR*Cos(fT2*360.0f*16.0f)/*+Cos((fNow+fT2)*40.0f)*fT2*8.0f*/);
    FLOAT fSize=(1.0f+afStarsPositions[iRnd][1]*0.125f)*asol[iPos]*8.0f;
    if( iFrame>3)
    {
      fSize/=8.0f;
    }
    fSize*=fBirthStretcher;
    FLOAT fAngle=afStarsPositions[iRnd][0]*360+(1.0f+afStarsPositions[iRnd][1])*360.0f*fT*32.0f;
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = pTD->GetTexel(PIX(ClampUp(fT*1024.0f, 1023.0f)), 0);
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f*fFadeOut);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    Particle_RenderSquare( vPos, fSize*fParticleStretch, fAngle, colCombined);
    Particle_SetTexturePart( 128, 128, 0, 0);
    Particle_RenderLine( vPos, vPos2, (0.25f+2.0f*fT)*fParticleStretch, colCombined);
  }
  // all done
  Particle_Flush();
}

void Particles_ExotechLarvaLaser(CEntity *pen, FLOAT3D vSource, FLOAT3D vTarget)
{
  Particle_PrepareTexture(&_toLarvaLaser, PBT_ADD);
  Particle_SetTexturePart( 512, 512, 0, 0);
  
  COLOR colColor;
  FLOAT3D vMid;
  
  vMid = (vSource - vTarget).Normalize();
  vMid = vTarget + vMid * 2.5f;

  colColor = C_RED|0xff;
  Particle_RenderLine( vSource, vMid, 0.5f, colColor);
  Particle_RenderLine( vMid, vTarget, 0.5f, colColor);
  
  Particle_Flush();

  // begin-end flares
  Particle_PrepareTexture(&_toWater, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);
  Particle_RenderSquare( vSource, 0.5, 0.0f, colColor);
  Particle_RenderSquare( vTarget, 0.5, 0.0f, colColor);
  Particle_Flush();
  
}

void Particles_Smoke(CEntity *pen, FLOAT3D vOffset, INDEX ctCount, FLOAT fLife, FLOAT fSpread, FLOAT fStretchAll, FLOAT fYSpeed)
{

  Particle_PrepareTexture( &_toChimneySmoke, PBT_BLEND);
  Particle_SetTexturePart( 1024, 1024, 0, 0);
  CTextureData *pTD = (CTextureData *) _toChimneySmokeGradient.GetData();
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*0.0f+vOffset*m;
  INDEX iPosRnd=INDEX(vCenter(1)*2343.1123f+vCenter(2)*3251.16732+vCenter(3)*2761.6323f);
  INDEX iCtRnd =pen->en_ulID+iPosRnd;
  INDEX ctSmokes=22+INDEX((afStarsPositions[iCtRnd%CT_MAX_PARTICLES_TABLE][0]+0.5f)*8);
  for(INDEX i=0; i<ctSmokes; i++)
  {
    INDEX iRnd =(pen->en_ulID+i)%CT_MAX_PARTICLES_TABLE;
    FLOAT fT = tmNow+afTimeOffsets[i];
    // apply time strech
    fT *= 1/fLife;
    // get fraction part
    fT = fT-int(fT);
    FLOAT fSlowFactor=1.0f-fT*0.25f;
    FLOAT3D vSpeed=FLOAT3D(afStarsPositions[iRnd][0]*fSpread,
      (afStarsPositions[iRnd][1]*0.1f+0.8f)*fSlowFactor,afStarsPositions[iRnd][2]*fSpread);
    FLOAT fSpeed=25.0f+(afStarsPositions[iRnd][0]+0.5f)*fYSpeed;
    FLOAT3D vPos=vCenter+vSpeed*fSpeed*fT+vOffset*m;
    FLOAT fSize=0.25f*fStretchAll+(afStarsPositions[iRnd][1]+0.5f)*fStretchAll*fT;
    FLOAT fAngle=afStarsPositions[iRnd][0]*360+afStarsPositions[iRnd][1]*360.0f*fT;
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = pTD->GetTexel(PIX(ClampUp(fT*1024.0f, 1023.0f)), 0);
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    Particle_RenderSquare( vPos, fSize, fAngle, colCombined);
  }
  // all done
  Particle_Flush();
}

#define TM_WINDBLAST_TOTAL_LIFE 2.0f
void Particles_Windblast( CEntity *pen, FLOAT fStretch, FLOAT fFadeOutStartTime)
{
  FLOAT fFadeOut=1.0f;
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  if( fFadeOutStartTime<fNow)
  {
    fFadeOut=CalculateRatio(fNow, fFadeOutStartTime, fFadeOutStartTime+2.0f, 0.0f, 1.0f);
  }
  FLOAT fDisperseFactor=pow(2.0f-fFadeOut, 3.0f);

  Particle_PrepareTexture( &_toTwister, PBT_BLEND);
  CTextureData *pTD = (CTextureData *) _toTwisterGradient.GetData();
  FLOAT arol[256], asol[256];
  pTD->FetchRow( 1, arol);
  pTD->FetchRow( 2, asol);

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  vX=vX*fStretch*fDisperseFactor;
  vY=vY*fStretch;
  vZ=vZ*fStretch*fDisperseFactor;
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector-vX*12.0f;
  vX=vX*0.75f;
  INDEX ctSmokes=16;
  for(INDEX i=0; i<ctSmokes; i++)
  {
    INDEX iRnd =(pen->en_ulID+i)%CT_MAX_PARTICLES_TABLE;
    INDEX iFrame=1+iRnd%7;
    Particle_SetTexturePart( 128, 128, iFrame, 0);
    FLOAT fT = tmNow+afTimeOffsets[i];
    // apply time strech
    fT *= 1/TM_WINDBLAST_TOTAL_LIFE;
    // get fraction part
    fT = fT-int(fT);
    INDEX iPos=(INDEX) (fT*255);
    //FLOAT fSlowFactor=1.0f-fT*0.25f;
    FLOAT fSpeed=25.0f+(afStarsPositions[iRnd][0]+0.5f)*2.0f;
    FLOAT fR=arol[iPos]*8.0f;
    fR=3.0f;
    FLOAT3D vPos=vCenter+vX*fSpeed*fT+
      vY*(fR*Sin(fT*360.0f*16.0f)/*+Sin((fNow+fT)*40.0f)*fT*8.0f*/)+
      vZ*(fR*Cos(fT*360.0f*16.0f)/*+Cos((fNow+fT)*40.0f)*fT*8.0f*/);
    FLOAT fT2=fT+0.05f*(0.5f+fT);
    FLOAT3D vPos2=vCenter+vX*fSpeed*fT2+
      vY*(fR*Sin(fT2*360.0f*16.0f)/*+Sin((fNow+fT2)*40.0f)*fT2*8.0f*/)+
      vZ*(fR*Cos(fT2*360.0f*16.0f)/*+Cos((fNow+fT2)*40.0f)*fT2*8.0f*/);
    FLOAT fSize=3.0f;
    if( iFrame>3)
    {
      fSize/=8.0f;
    }
    FLOAT fAngle=afStarsPositions[iRnd][0]*360+(1.0f+afStarsPositions[iRnd][1])*360.0f*fT*32.0f;
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = pTD->GetTexel(PIX(ClampUp(fT*1024.0f, 1023.0f)), 0);
    /*
    colA = CT_OPAQUE;
    col=C_WHITE;
    */
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f*fFadeOut);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    Particle_RenderSquare( vPos, fSize, fAngle, colCombined);
    Particle_SetTexturePart( 128, 128, 0, 0);
    Particle_RenderLine( vPos, vPos2, 0.25f+2.0f*fT, colCombined);
  }
  // all done
  Particle_Flush();
}

#define CT_COLLECT_ENERGY_PARTICLES 128
#define CT_PROJECTILE_SPAWN_STARS 32
void Particles_CollectEnergy(CEntity *pen, FLOAT tmStart, FLOAT tmStop)
{
  Particle_PrepareTexture( &_toElectricitySparks, PBT_BLEND);
  Particle_SetTexturePart( 512, 1024, 0, 0);
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fLife = 0.5;
  //INDEX ctRendered = 0; // ##### not used in tfis func
  FLOAT tmDelta = fLife/CT_COLLECT_ENERGY_PARTICLES;
  for( INDEX iVtx=0; iVtx<CT_COLLECT_ENERGY_PARTICLES; iVtx++)
  {
    FLOAT tmFakeStart = tmStart+iVtx*tmDelta;
    FLOAT fPassedTime = fNow-tmFakeStart;
    //if(fPassedTime<0.0f || fPassedTime>fLife || tmFakeStart>tmStop) continue;
    // calculate fraction part
    FLOAT fT=fPassedTime/fLife;
    fT=fT-INDEX(fT);

    INDEX iRnd = iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vRnd= FLOAT3D(afStarsPositions[iRnd][0],afStarsPositions[iRnd][1],afStarsPositions[iRnd][2]);
    vRnd(1) *= 40.0f;
    vRnd(2) *= 40.0f;
    vRnd(3) *= 40.0f;
    FLOAT3D vSource = vCenter+vRnd;
    FLOAT3D vDestination = vCenter+vRnd*0.05f;
    FLOAT3D vPos, vPos2;
    // lerp position
    vPos = Lerp( vSource, vDestination, fT);
    FLOAT fT2 = Clamp(fT-0.125f-fT*fT*0.125f, 0.0f, 1.0f);
    vPos2 = Lerp( vSource, vDestination, fT2);

    UBYTE ubR = (UBYTE) (255);//+afStarsPositions[iRnd][1]*64;
    UBYTE ubG = (UBYTE) (128+(1.0f-fT)*128);//223+afStarsPositions[iRnd][2]*64;
    UBYTE ubB = (UBYTE) (16+afStarsPositions[iRnd][2]*32+(1.0f-fT)*64);
    UBYTE ubA = (UBYTE) (CalculateRatio( fT, 0.0f, 1.0f, 0.4f, 0.01f)*255);
    COLOR colLine = RGBToColor( ubR, ubG, ubB) | ubA;
    
    FLOAT fSize = 0.125f;
    Particle_RenderLine( vPos2, vPos, fSize, colLine);
    //ctRendered++;
  }

  // all done
  Particle_Flush();

  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));

  Particle_PrepareTexture(&_toStar03, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);
  FLOAT fStarLife=0.3f;
  FLOAT tmDelta2 = 0.025f;
  for( INDEX iStar=0; iStar<CT_PROJECTILE_SPAWN_STARS; iStar++)
  {
    FLOAT tmFakeStart = tmStart+iStar*tmDelta2;
    FLOAT fPassedTime = fNow-tmFakeStart;
    // calculate fraction part
    FLOAT fT=fPassedTime/fStarLife;
    fT=fT-INDEX(fT);
    //INDEX iRnd = iStar%CT_MAX_PARTICLES_TABLE;
    FLOAT fRadius=2;
    FLOAT3D vPos= vCenter+
      vX*Sin(fT*360.0f)*fRadius+
      vY*fT*2+
      vZ*Cos(fT*360.0f)*fRadius;
    //UBYTE ubR = (UBYTE) (255);
    //UBYTE ubG = (UBYTE) (128+(1.0f-fT)*128);
    //UBYTE ubB = (UBYTE) (16+afStarsPositions[iRnd][2]*32+(1.0f-fT)*64);
    FLOAT fFader=CalculateRatio( fT, 0.0f, 1.0f, 0.4f, 0.01f);
    FLOAT fPulser=(1.0f+(sin((fT*fT)/4.0f)))/2.0f;
    UBYTE ubA = (UBYTE) (fFader*fPulser*255);
    //COLOR colLine = RGBToColor( ubA, ubA, ubA) | CT_OPAQUE;
    FLOAT fSize = 2;
    Particle_RenderSquare( vPos, fSize, 0.0f, C_ORANGE|ubA);
    //ctRendered++;
  }

  // all done
  Particle_Flush();
}


#define SD_LIFE 4.0f
void Particles_SummonerDisappear( CEntity *pen, FLOAT tmStart)
{
  FLOAT fMipFactor = Particle_GetMipFactor();
  BOOL bVisible = pen->en_pmoModelObject->IsModelVisible( fMipFactor);
  if( !bVisible) return;

  Particle_PrepareTexture(&_toSEStar01, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

  CTextureData *pTD = (CTextureData *) _toSummonerDisappearGradient.GetData();
  FLOAT apol[256];
  ULONG *pcol=pTD->GetRowPointer(0); // flare rnd color
  ULONG *pcolAdder=pTD->GetRowPointer(1);
  pTD->FetchRow( 0, apol);

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fLiving = tmNow-tmStart;
  if( fLiving>SD_LIFE) return;
  FLOAT fRatio=fLiving*1/SD_LIFE;
  fRatio=fRatio-int(fRatio);
  INDEX iIndex=INDEX(fRatio*255.0f);
  FLOAT fG=10.0f*SD_LIFE*5.0f;
  FLOAT fSpeed=0.0f;
  FLOAT fGValue=0.0f;
  FLOAT fExplodeRatio=0.2f;
  if( fRatio>fExplodeRatio)
  {
    fSpeed=(0.351f+0.0506f*log(fRatio-fExplodeRatio))*2.0f;
    fGValue=fG/2.0f*(fRatio-fExplodeRatio)*(fRatio-fExplodeRatio);
  }
  FLOAT fOut=fSpeed*32.0f;
  // fill array with absolute vertices of entity's model and its attached models
  pen->GetModelVerticesAbsolute(avVertices, 0.05f, fMipFactor); 

  // get entity position and orientation
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;
  FLOAT3D vG=-vY;

  // calculate color factor (for fade in/out)
  FLOAT fColorFactor=CalculateRatio(tmNow, tmStart, tmStart+SD_LIFE, 0.25f, 0.1f);
  FLOAT fSizeStretcher=CalculateRatio(tmNow, tmStart, tmStart+SD_LIFE, 0.25f, 0.1f)*2.0f;
  INDEX ctVtx = avVertices.Count();
  for( INDEX iVtx=0; iVtx<ctVtx; iVtx+=1)
  {
    INDEX iRnd=iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT fRndPulseOffset=afStarsPositions[iRnd][0];
    FLOAT fRndPulseSpeed=afStarsPositions[iRnd][1]*128.0f;
    FLOAT fRndSize=afStarsPositions[iRnd][2];

    FLOAT fPulser=1.0f-(fRatio*(1.0f+(Sin(fRatio*360.0f*fRndPulseSpeed+fRndPulseOffset*360.0f)))/2.0f);
    UBYTE ubColor = UBYTE(CT_OPAQUE*fColorFactor*fPulser);
    COLOR col=(ByteSwap(pcol[iRnd%255])&0xFFFFFF00)|ubColor;
    COLOR colLighter=ByteSwap(pcolAdder[iIndex])&0xFFFFFF00;
    col=AddColors(col,colLighter);

    FLOAT3D vPos = avVertices[iVtx];
    vPos-=vCenter;
    FLOAT fX = (vPos%vX)*(1.0f+fOut);
    FLOAT fY = (vPos%vY)*(1.0f+fOut/10.0f);
    FLOAT fZ = (vPos%vZ)*(1.0f+fOut);
    vPos = vX*fX+vY*fY+vZ*fZ+vCenter+vG*fGValue;
    Particle_RenderSquare( vPos, (1.0f+fRndSize)*fSizeStretcher, 0, col);
  }

  // flush array
  avVertices.PopAll();
  // all done
  Particle_Flush();
}

void Particles_DisappearDust( CEntity *pen, FLOAT fStretch, FLOAT fStartTime)
{
}

#define TM_GROWING_SWIRL_FX_LIFE 4.0f
#define TM_GROWING_SWIRL_TOTAL_LIFE 1.0f
#define TM_SWIRL_SPARK_LAUNCH 0.05f
void Particles_GrowingSwirl( CEntity *pen, FLOAT fStretch, FLOAT fStartTime)
{
  Particle_PrepareTexture(&_toGrowingTwirl, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fFadeFX=CalculateRatio(tmNow, fStartTime, fStartTime+TM_GROWING_SWIRL_FX_LIFE, 0.1f, 0.2f);
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  vX=vX*fStretch;
  vY=vY*fStretch;
  vZ=vZ*fStretch;
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector+vY*4.0f;
  INDEX ctStars=(INDEX)(TM_GROWING_SWIRL_FX_LIFE/TM_SWIRL_SPARK_LAUNCH);
  for(INDEX i=0; i<ctStars; i++)
  {
    //INDEX iRnd =(pen->en_ulID+i)%CT_MAX_PARTICLES_TABLE;
    FLOAT fBirth = fStartTime+i*TM_SWIRL_SPARK_LAUNCH-2.0f;//+afTimeOffsets[i]*TM_SWIRL_SPARK_LAUNCH/0.25f;
    FLOAT fT = tmNow-fBirth;
    FLOAT fFade=CalculateRatio(fT, 0, TM_GROWING_SWIRL_TOTAL_LIFE, 0.1f, 0.2f);
    if( fFade==0.0f) continue;
    // apply time strech
    fT *= 1/TM_GROWING_SWIRL_TOTAL_LIFE;
    // get fraction part
    fT = fT-int(fT);
    FLOAT fR=fT*64.0f;
    FLOAT fAng=fT*600.0f;
    FLOAT3D vPos=vCenter+
      vX*(fR*Sin(fAng))+
      vZ*(fR*Cos(fAng));
    FLOAT fSize=(1.0f+fT)*4.0f;
    COLOR col = RGBToColor( 23, 112, 174);
    UBYTE ubA=UBYTE(CT_OPAQUE*fFade*fFadeFX);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    FLOAT fRot=fT*360.0f+afTimeOffsets[i]*360.0f;
    Particle_RenderSquare( vPos, fSize, fRot, colCombined);
  }
  // all done
  Particle_Flush();
}

void Particles_SniperResidue(CEntity *pen, FLOAT3D vSource, FLOAT3D vTarget)
{
  
  Particle_PrepareTexture(&_toLightning, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);
  
  COLOR colColor;
  
  FLOAT3D v1 = vSource;
  FLOAT3D v2 = vTarget;

  colColor = C_YELLOW|0xff;
  for (INDEX i=1; i<24; i++) {
    for (INDEX j=0; j<2; j++) {
      Particle_RenderLine( v1, v2, 0.05f, colColor);
    }
    v1 = v2;
    v2 = Lerp(vSource, vTarget, NormByteToFloat(i*255/24));
  }
        
  Particle_Flush();
}

void Particles_SummonerStaff(CEmiter &em)
{
  /*
  Particle_PrepareTexture(&_toSEStar01, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);
  */

  Particle_PrepareTexture(&_toStar01, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();

  CTextureData *pTD = (CTextureData *) _toSummonerStaffGradient.GetData();
  ULONG *pcol=pTD->GetRowPointer(0); // flare rnd color

  FLOAT fLerpFactor=_pTimer->GetLerpFactor();
  for(INDEX i=0; i<em.em_aepParticles.Count(); i++)
  {
    const CEmittedParticle &ep=em.em_aepParticles[i];
    if(ep.ep_tmEmitted<0) continue;
    FLOAT3D vPos=Lerp(ep.ep_vLastPos, ep.ep_vPos, fLerpFactor);
    FLOAT fRot=Lerp(ep.ep_fLastRot, ep.ep_fRot, fLerpFactor);
    INDEX iIndex=(INDEX) Clamp((tmNow-ep.ep_tmEmitted)/(ep.ep_tmLife)*255.0f,0.0f,255.0f);
    COLOR col=ByteSwap(pcol[iIndex]);
    Particle_RenderSquare( vPos, 1.0f*ep.ep_fStretch, fRot, col);
  }
  // all done
  Particle_Flush();
}

void Particles_AirElementalBlow(CEmiter &em)
{
  Particle_PrepareTexture( &_toTwister, PBT_BLEND);
  CTextureData *pTD = (CTextureData *) _toTwisterGradient.GetData();

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fLerpFactor=_pTimer->GetLerpFactor();
  for(INDEX i=0; i<em.em_aepParticles.Count(); i++)
  {
    const CEmittedParticle &ep=em.em_aepParticles[i];
    FLOAT fRatio=Clamp((tmNow-ep.ep_tmEmitted)/ep.ep_tmLife,0.0f,1.0f);
    INDEX iRnd =INDEX(ep.ep_tmEmitted*123345)%CT_MAX_PARTICLES_TABLE;
    INDEX iFrame=1+iRnd%3;
    Particle_SetTexturePart( 128, 128, iFrame, 0);

    if(ep.ep_tmEmitted<0) continue;
    FLOAT3D vPos=Lerp(ep.ep_vLastPos, ep.ep_vPos, fLerpFactor);
    FLOAT fRot=Lerp(ep.ep_fLastRot, ep.ep_fRot, fLerpFactor);
    //COLOR col=LerpColor(ep.ep_colLastColor, ep.ep_colColor, fLerpFactor);
    
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = pTD->GetTexel(PIX(ClampUp(fRatio*1024.0f, 1023.0f)), 0);
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;

    Particle_RenderSquare( vPos, (1.5f+1.5f*fRatio)*ep.ep_fStretch, fRot, colCombined);
  }
  // all done
  Particle_Flush();
}

void Particles_AirElemental(CEntity *pen, FLOAT fStretch, FLOAT fFade, FLOAT tmDeath, COLOR colMultiply)
{
  // fill array with absolute vertices of entity's model and its attached models
  pen->GetModelVerticesAbsolute(avVertices, 0.0f, 0.0f); 

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  // get entity position and orientation
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;
  FLOAT3D vG=-vY;

  FLOAT tmDying=ClampDn(tmNow-tmDeath, 0.0f);
  FLOAT fSpeed=1.0f;
  FLOAT fG=50.0f;
  FLOAT fGValue=0.0f;
  if( tmDying>0.0f)
  {
    fSpeed=(2.5f+log(2.0f*tmDying+0.22313016014842982893328047076401f))*2.0f;
    fGValue=fG*((1.0f+tmDying)*(1.0f+tmDying)-1.0f);
  }

  Particle_PrepareTexture( &_toTwister, PBT_BLEND);
  CTextureData *pTD = (CTextureData *) _toTwisterGradient.GetData();

  INDEX ctVtx = avVertices.Count();
  for( INDEX iVtx=0; iVtx<ctVtx; iVtx+=1)
  {
    INDEX iRnd =iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT fSize=(1+afStarsPositions[iRnd][0]+0.5f)*2*fStretch;
    INDEX iFrame=1+iRnd%7;
    Particle_SetTexturePart( 128, 128, iFrame, 0);
    FLOAT3D vRelPos = (avVertices[iVtx]-vCenter)*fSpeed;
    FLOAT3D vPos = vCenter+vRelPos+vG*fGValue;
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = CT_OPAQUE;
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f);
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    colCombined=MulColors(colCombined, colMultiply);
    FLOAT fRndRot=Sgn(afStarsPositions[iRnd][0])*(Abs(afStarsPositions[iRnd][1])+1.0f)*360.0f*2;
    if( iFrame>3)
    {
      fSize/=5.0f;
    }
    //fSize*=fSpeed;
    Particle_RenderSquare( vPos, fSize, tmNow*fRndRot, colCombined);
  }
  avVertices.PopAll();
  Particle_Sort();
  Particle_Flush();
}

void Particles_MeteorTrail(CEntity *pen, FLOAT fStretch, FLOAT fLength, FLOAT3D vSpeed)
{
  Particle_PrepareTexture( &_toMeteorTrail, PBT_ADD);
  Particle_SetTexturePart( 1024, 2048, 0, 0);
  //CTextureData *pTD = (CTextureData *) _toExplosionDebrisGradient.GetData();
  FLOAT3D vPos0 = pen->GetLerpedPlacement().pl_PositionVector+vSpeed*0.05f;
  FLOAT3D vPos1 = pen->GetLerpedPlacement().pl_PositionVector+vSpeed*0.05f-vSpeed*0.125f;
  Particle_RenderLine( vPos1, vPos0, 3.0f, C_WHITE|CT_OPAQUE);
  Particle_Flush();
}

#define TM_LEAVES_LIFE 5.0f
void Particles_Leaves(CEntity *penTree, FLOATaabbox3D boxSize, FLOAT3D vSource, FLOAT fDamagePower,
                      FLOAT fLaunchPower, FLOAT3D vGDir, FLOAT fGA, FLOAT tmStarted, COLOR colMax)
{
  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  Particle_PrepareTexture( &_toTreeSprayTexture, PBT_BLEND);
  FLOAT fT=(fNow-tmStarted);
  FLOAT fRatio=(fNow-tmStarted)/TM_LEAVES_LIFE;

  FLOAT3D vCenter = penTree->GetLerpedPlacement().pl_PositionVector;

  FLOAT fBoxWidth=boxSize.Size()(1);
  FLOAT fBoxHeight=boxSize.Size()(2);
  FLOAT fBoxLength=boxSize.Size()(3);

  UBYTE ubH,ubS,ubV;
  ColorToHSV( colMax, ubH, ubS, ubV);

  INDEX ctSprays=(INDEX) (128-Clamp(3.0f-fDamagePower,0.0f,3.0f)*32);
  for( INDEX iSpray=0; iSpray<ctSprays; iSpray++)
  {
    INDEX iFrame=((int(tmStarted*100.0f))%8+iSpray)%8;
    Particle_SetTexturePart( 256, 256, iFrame, 0);

    FLOAT fFade=CalculateRatio( fRatio, 0, 1, 0, 0.2f);
    INDEX iRnd1=(INDEX(iSpray+tmStarted*12356.789f))%CT_MAX_PARTICLES_TABLE;
    INDEX iRnd2=(INDEX(iSpray+tmStarted*21341.789f))%CT_MAX_PARTICLES_TABLE;
    INDEX iRnd3=(INDEX(iSpray+tmStarted*52672.789f))%CT_MAX_PARTICLES_TABLE;
    INDEX iRnd4=(INDEX(iSpray+tmStarted*83652.458f))%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vLaunchSpeed= FLOAT3D(
      afStarsPositions[iRnd1][0]*2.0f,
      (afStarsPositions[iRnd1][1]+1.0f)*3.0f,
      afStarsPositions[ iRnd1][2]*2.0f);
    vLaunchSpeed=vLaunchSpeed.Normalize()*(1+afStarsPositions[iRnd3][0]*0.25f)*fLaunchPower;
    FLOAT3D vPosRatio=FLOAT3D(
      afStarsPositions[iRnd2][0]*0.6f,
      0.6f+afStarsPositions[iRnd2][1]*0.4f,
      afStarsPositions[iRnd2][2]*0.6f);
    FLOAT3D vRelLaunchPos=FLOAT3D(
      vPosRatio(1)*fBoxWidth,
      vPosRatio(2)*fBoxHeight,
      vPosRatio(3)*fBoxLength);
    FLOAT3D vPos = vCenter+vRelLaunchPos+vLaunchSpeed*fT+vGDir*(fT*fT*fGA);
    
    FLOAT fH=Clamp(ubH*(1.0f+afStarsPositions[iRnd4][1]*0.125f), 0.0f, 255.0f);
    FLOAT fS=Clamp(ubS*(1.0f+afStarsPositions[iRnd4][2]*0.125f), 0.0f, 255.0f);
    FLOAT fV=Clamp(ubV*(1.0f-(afStarsPositions[iRnd4][2]+0.5f)*0.25f), 0.0f, 255.0f);
    COLOR colRnd=HSVToColor((UBYTE)fH,(UBYTE)fS,(UBYTE)fV);
    UBYTE ubAlpha = UBYTE(CT_OPAQUE*fFade);
    COLOR col = colRnd|ubAlpha;
    FLOAT fSize=(afStarsPositions[iRnd3][0]+1.0f)*0.5f;
    FLOAT fRotation=fT*afStarsPositions[iRnd3][1]*600.0f;
    Particle_RenderSquare( vPos, fSize, fRotation, col);
  }
  // all done
  Particle_Flush();
}

void Particles_LarvaEnergy(CEntity *pen, FLOAT3D vOffset)
{
  Particle_PrepareTexture( &_toElectricitySparks, PBT_BLEND);
  Particle_SetTexturePart( 512, 1024, 0, 0);
  FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector + vOffset;

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();
  FLOAT fLife = 0.5;
  //INDEX ctRendered = 0; // ###### not used in this func
  FLOAT tmDelta = fLife/CT_COLLECT_ENERGY_PARTICLES;
  for( INDEX iVtx=0; iVtx<CT_COLLECT_ENERGY_PARTICLES; iVtx++)
  {
    FLOAT tmFakeStart = iVtx*tmDelta;
    FLOAT fPassedTime = fNow-tmFakeStart;
    //if(fPassedTime<0.0f || fPassedTime>fLife || tmFakeStart>tmStop) continue;
    // calculate fraction part
    FLOAT fT=fPassedTime/fLife;
    fT=fT-INDEX(fT);

    INDEX iRnd = iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT3D vRnd= FLOAT3D(afStarsPositions[iRnd][0],afStarsPositions[iRnd][1],afStarsPositions[iRnd][2]);
    vRnd(1) *= 40.0f;
    vRnd(2) *= 40.0f;
    vRnd(3) *= 40.0f;
    FLOAT3D vSource = vCenter+vRnd;
    FLOAT3D vDestination = vCenter+vRnd*0.05f;
    FLOAT3D vPos, vPos2;
    // lerp position
    vPos = Lerp( vSource, vDestination, fT);
    FLOAT fT2 = Clamp(fT-0.125f-fT*fT*0.125f, 0.0f, 1.0f);
    vPos2 = Lerp( vSource, vDestination, fT2);

    UBYTE ubR = (UBYTE) (255);//+afStarsPositions[iRnd][1]*64;
    UBYTE ubG = (UBYTE) (128+(1.0f-fT)*128);//223+afStarsPositions[iRnd][2]*64;
    UBYTE ubB = (UBYTE) (16+afStarsPositions[iRnd][2]*32+(1.0f-fT)*64);
    UBYTE ubA = (UBYTE) (CalculateRatio( fT, 0.0f, 1.0f, 0.4f, 0.01f)*255);
    COLOR colLine = RGBToColor( ubR, ubG, ubB) | ubA;
    
    FLOAT fSize = 0.125f;
    Particle_RenderLine( vPos2, vPos, fSize, colLine);
    //ctRendered++;
  }

  // all done
  Particle_Flush();

  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));

  Particle_PrepareTexture(&_toStar03, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);
  FLOAT fStarLife=0.3f;
  FLOAT tmDelta2 = 0.025f;
  for( INDEX iStar=0; iStar<CT_PROJECTILE_SPAWN_STARS; iStar++)
  {
    FLOAT tmFakeStart = iStar*tmDelta2;
    FLOAT fPassedTime = fNow-tmFakeStart;
    // calculate fraction part
    FLOAT fT=fPassedTime/fStarLife;
    fT=fT-INDEX(fT);
    //INDEX iRnd = iStar%CT_MAX_PARTICLES_TABLE;
    FLOAT fRadius=2;
    FLOAT3D vPos= vCenter+
      vX*Sin(fT*360.0f)*fRadius+
      vY*fT*2+
      vZ*Cos(fT*360.0f)*fRadius;
    //UBYTE ubR = (UBYTE) (255);
    //UBYTE ubG = (UBYTE) (128+(1.0f-fT)*128);
   // UBYTE ubB = (UBYTE) (16+afStarsPositions[iRnd][2]*32+(1.0f-fT)*64);
    FLOAT fFader=CalculateRatio( fT, 0.0f, 1.0f, 0.4f, 0.01f);
    FLOAT fPulser=(1.0f+(sin((fT*fT)/4.0f)))/2.0f;
    UBYTE ubA = (UBYTE) (fFader*fPulser*255);
    //COLOR colLine = RGBToColor( ubA, ubA, ubA) | CT_OPAQUE;
    FLOAT fSize = 2;
    Particle_RenderSquare( vPos, fSize, 0.0f, C_ORANGE|ubA);
    //ctRendered++;
  }

  // all done
  Particle_Flush();
}

void Particles_AirElemental_Comp(CModelObject *mo, FLOAT fStretch, FLOAT fFade, CPlacement3D pl)
{
  
  CPlacement3D plPlacement = pl;
  
  // calculate rotation matrix
  FLOATmatrix3D mRotation;
  MakeRotationMatrixFast(mRotation, plPlacement.pl_OrientationAngle);

  mo->GetModelVertices( avVertices, mRotation, plPlacement.pl_PositionVector, 0.0f, 0.0f);

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  // get entity position and orientation
  const FLOATmatrix3D &m = mRotation;
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  FLOAT3D vCenter = plPlacement.pl_PositionVector;
  FLOAT3D vG=-vY;

  FLOAT fSpeed=1.0f;
  //FLOAT fG=50.0f;
  FLOAT fGValue=0.0f;

  Particle_PrepareTexture( &_toTwister, PBT_BLEND);
  CTextureData *pTD = (CTextureData *) _toTwisterGradient.GetData();

  INDEX ctVtx = avVertices.Count();
  for( INDEX iVtx=0; iVtx<ctVtx; iVtx+=1)
  {
    INDEX iRnd =iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT fSize=(1+afStarsPositions[iRnd][0]+0.5f)*2*fStretch;
    INDEX iFrame=1+iRnd%7;
    Particle_SetTexturePart( 128, 128, iFrame, 0);
    FLOAT3D vRelPos = (avVertices[iVtx]-vCenter)*fSpeed;
    FLOAT3D vPos = vCenter+vRelPos+vG*fGValue;
    COLOR col = pTD->GetTexel(PIX((afStarsPositions[iRnd][2]+0.5f)*1024.0f), 0);
    COLOR colA = CT_OPAQUE;
    UBYTE ubA=UBYTE((colA&0xFF)*0.75f);
    col = col&0xff000000;
    COLOR colCombined=(col&0xFFFFFF00)|ubA;
    FLOAT fRndRot=Sgn(afStarsPositions[iRnd][0])*(Abs(afStarsPositions[iRnd][1])+1.0f)*360.0f*2;
    if( iFrame>3)
    {
      fSize/=5.0f;
    }
    //fSize*=fSpeed;
    Particle_RenderSquare( vPos, fSize, tmNow*fRndRot, colCombined);
  }
  avVertices.PopAll();
  Particle_Sort();
  Particle_Flush();
}

void Particles_ModelGlow( CEntity *pen, FLOAT tmEnd, enum ParticleTexture ptTexture, FLOAT fSize, FLOAT iVtxStep, FLOAT fAnimSpd, COLOR iCol)
{
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
  
  FLOAT fMipFactor = Particle_GetMipFactor();
  BOOL bVisible = pen->en_pmoModelObject->IsModelVisible( fMipFactor);
  if( !bVisible) return;

  SetupParticleTextureWithAddAlpha( ptTexture );
  
  // fill array with absolute vertices of entity's model and its attached models
  pen->GetModelVerticesAbsolute(avVertices, fAnimSpd*(1.0f-0.5f*Sin(300.0f*tmNow)), fMipFactor); 

  // get entity position and orientation
  const FLOATmatrix3D &m = pen->GetRotationMatrix();
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  //FLOAT3D vCenter = pen->GetLerpedPlacement().pl_PositionVector;
  
  UBYTE ubCol=255;
  if((tmEnd-tmNow)<5.0f)
  {
    ubCol = FloatToInt(255.0f*(0.5f-0.5f*cos((tmEnd-tmNow)*(9.0f*3.1415927f/5.0f))));
  }
  
  INDEX ctVtx = avVertices.Count();
  for( INDEX iVtx=0; iVtx<ctVtx-1; iVtx+=(INDEX)iVtxStep)
  {
    INDEX iRnd=iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT fRndSize=afStarsPositions[iRnd][2];

    FLOAT3D vPos = avVertices[iVtx];
    Particle_RenderSquare( vPos, (1.0f+fRndSize)*fSize, 0, iCol|ubCol);    
  }

  // flush array
  avVertices.PopAll();
  // all done
  Particle_Flush();
}
void Particles_ModelGlow2( CModelObject *mo, CPlacement3D pl, FLOAT tmEnd, enum ParticleTexture ptTexture, FLOAT fSize, FLOAT iVtxStep, FLOAT fAnimSpd, COLOR iCol)
{
  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();
    
  SetupParticleTextureWithAddAlpha( ptTexture );
  
  CPlacement3D plPlacement = pl;

  // fill array with absolute vertices of entity's model and its attached models
  FLOATmatrix3D mRotation;
  MakeRotationMatrixFast(mRotation, plPlacement.pl_OrientationAngle);
  mo->GetModelVertices(avVertices, mRotation, plPlacement.pl_PositionVector, fAnimSpd*(1.0f-0.5f*Sin(300.0f*tmNow)), 0.0f); 

  // get entity position and orientation
  const FLOATmatrix3D &m = mRotation;
  FLOAT3D vX( m(1,1), m(2,1), m(3,1));
  FLOAT3D vY( m(1,2), m(2,2), m(3,2));
  FLOAT3D vZ( m(1,3), m(2,3), m(3,3));
  //FLOAT3D vCenter = plPlacement.pl_PositionVector;
  
  UBYTE ubCol=255;
  if((tmEnd-tmNow)<5.0f)
  {
    ubCol = FloatToInt(255.0f*(0.5f-0.5f*cos((tmEnd-tmNow)*(9.0f*3.1415927f/5.0f))));
  }
  
  INDEX ctVtx = avVertices.Count();
  for( INDEX iVtx=0; iVtx<ctVtx-1; iVtx+=(INDEX)iVtxStep)
  {
    INDEX iRnd=iVtx%CT_MAX_PARTICLES_TABLE;
    FLOAT fRndSize=afStarsPositions[iRnd][2];

    FLOAT3D vPos = avVertices[iVtx];
    Particle_RenderSquare( vPos, (1.0f+fRndSize)*fSize, 0, iCol|ubCol);    
  }

  // flush array
  avVertices.PopAll();
  // all done
  Particle_Flush();
}

void Particles_RunAfterBurner(CEntity *pen, FLOAT tmEnd, FLOAT fStretch, INDEX iGradientType)
{
  FLOAT3D vGDir = ((CMovableEntity *)pen)->en_vGravityDir;
  FLOAT fGA = ((CMovableEntity *)pen)->en_fGravityA;

  CLastPositions *plp = pen->GetLastPositions(CT_AFTERBURNER_SMOKES);
  Particle_PrepareTexture(&_toAfterBurner,  PBT_BLEND);
  CTextureData *pTD;
  switch( iGradientType)
  {
  case 0:
  default:
    pTD=(CTextureData *) _toAfterBurnerGradient.GetData();
    break;
  case 1:
    pTD=(CTextureData *) _toAfterBurnerGradientBlue.GetData();
    break;
  case 2:
    pTD=(CTextureData *) _toAfterBurnerGradientMeteor.GetData();
    break;
  }

  const FLOAT3D *pvPos1;
  const FLOAT3D *pvPos2 = &plp->GetPosition(plp->lp_ctUsed-1);
  
  //ULONG *pcolFlare=pTD->GetRowPointer(0); // flare color
  ULONG *pcolExp=pTD->GetRowPointer(1);   // explosion color
  ULONG *pcolSmoke=pTD->GetRowPointer(2); // smoke color
  FLOAT aFlare_sol[256], aFlare_vol[256], aFlare_wol[256], aFlare_rol[256];
  FLOAT aExp_sol[256], aExp_vol[256], aExp_wol[256], aExp_rol[256];
  FLOAT aSmoke_sol[256], aSmoke_vol[256], aSmoke_rol[256];
  
  pTD->FetchRow( 4, aFlare_sol);
  pTD->FetchRow( 5, aFlare_vol);
  pTD->FetchRow( 6, aFlare_wol);
  pTD->FetchRow( 7, aFlare_rol);
  pTD->FetchRow( 8, aExp_sol);
  pTD->FetchRow( 9, aExp_vol);
  pTD->FetchRow(10, aExp_wol);
  pTD->FetchRow(11, aExp_rol);
  pTD->FetchRow(12, aSmoke_sol);
  pTD->FetchRow(13, aSmoke_vol);
  pTD->FetchRow(14, aSmoke_rol);

  FLOAT fNow = _pTimer->GetLerpedCurrentTick();  
  FLOAT fColMul=1.0f;
  UBYTE ubColMul=255;
  if( (tmEnd-fNow)<6.0f)
  {
    fColMul = (tmEnd-fNow)/6.0f;
    ubColMul = (UBYTE) (255.0f*fColMul);
  }
  //UBYTE ubColMul=UBYTE(CT_OPAQUE*fColMul);
  //COLOR colMul=RGBAToColor(ubColMul,ubColMul,ubColMul,ubColMul);
  COLOR col;

  for(INDEX iPos = plp->lp_ctUsed-1; iPos>=1; iPos--)
  {
    pvPos1 = pvPos2;
    pvPos2 = &plp->GetPosition(iPos);
    if( *pvPos1==*pvPos2) continue;

    FLOAT fT=(iPos+_pTimer->GetLerpFactor())*_pTimer->TickQuantum;
    FLOAT fRatio=fT/(CT_AFTERBURNER_SMOKES*_pTimer->TickQuantum);
    INDEX iIndex=(INDEX) (fRatio*255);
    INDEX iRnd=(INDEX)(size_t(pvPos1)%CT_MAX_PARTICLES_TABLE);

    // smoke
    FLOAT3D vPosS = *pvPos1;
    Particle_SetTexturePart( 512, 512, 1, 0);
    FLOAT fAngleS = afStarsPositions[iRnd][1]*360.0f+fT*120.0f*afStarsPositions[iRnd][2];
    FLOAT fSizeS = (0.5f+aSmoke_sol[iIndex]*2.5f)*fStretch;
    FLOAT3D vVelocityS=FLOAT3D(afStarsPositions[iRnd][1],
                               afStarsPositions[iRnd][2],
                               afStarsPositions[iRnd][0])*5.0f;
    vPosS=vPosS+vVelocityS*fT+vGDir*fGA/2.0f*(fT*fT)/32.0f;
    col = ByteSwap(pcolSmoke[iIndex]);
    col = (col&0xffffff00)|((col&0x000000ff)*ubColMul/255);
    Particle_RenderSquare( vPosS, fSizeS, fAngleS, col);

    // explosion
    FLOAT3D vPosE = (*pvPos1+*pvPos2)/2.0f;//Lerp(*pvPos1, *pvPos2, _pTimer->GetLerpFactor());
    Particle_SetTexturePart( 512, 512, 0, 0);
    FLOAT fAngleE = afStarsPositions[iRnd][0]*360.0f;//+fT*360.0f;
    FLOAT fSizeE = (0.5f+aExp_sol[iIndex]*2.0f)*fStretch;
    FLOAT3D vVelocityE=FLOAT3D(afStarsPositions[iRnd][0], 
                               afStarsPositions[iRnd][1],
                               afStarsPositions[iRnd][2])*3.0f;
    vPosE=vPosE+vVelocityE*fT+vGDir*fGA/2.0f*(fT*fT)/32.0f;
    col = ByteSwap(pcolExp[iIndex]);
    col = (col&0xffffff00)|((col&0x000000ff)*ubColMul/255);
    Particle_RenderSquare( vPosE, fSizeE, fAngleE, col);

  }
  // all done
  Particle_Flush();

  if( IsOfClass(pen, "PyramidSpaceShip")) return;
  Particle_PrepareTexture(&_toAfterBurnerHead, PBT_ADDALPHA);
  Particle_SetTexturePart( 1024, 1024, 0, 0);
  
  INDEX ctParticles=CT_AFTERBURNER_HEAD_POSITIONS;//Min(CT_AFTERBURNER_HEAD_POSITIONS,plp->lp_ctUsed-1);
  pvPos1 = &plp->GetPosition(ctParticles-1);
  for(INDEX iFlare=ctParticles-2; iFlare>=0; iFlare--)
  {
    pvPos2 = pvPos1;
    pvPos1 = &plp->GetPosition(iFlare);
    if( *pvPos1==*pvPos2) continue;

    for (INDEX iInter=CT_AFTERBURNER_HEAD_INTERPOSITIONS-1; iInter>=0; iInter--)
    {
      FLOAT fT=(iFlare+_pTimer->GetLerpFactor()+iInter*1.0f/CT_AFTERBURNER_HEAD_INTERPOSITIONS)*_pTimer->TickQuantum;
      FLOAT fRatio=fT/(ctParticles*_pTimer->TickQuantum);
      INDEX iIndex=(INDEX) (fRatio*255);
      FLOAT fSize = (aFlare_sol[iIndex]*2.0f)*fStretch;
      FLOAT3D vPos = Lerp(*pvPos1, *pvPos2, iInter*1.0f/CT_AFTERBURNER_HEAD_INTERPOSITIONS);
      FLOAT fAngle = afStarsPositions[iInter][0]*360.0f+fRatio*360.0f;
      Particle_RenderSquare( vPos, fSize, fAngle, C_WHITE|ubColMul);
    }
  }

  // all done
  Particle_Flush();
}

void Particles_Fireworks01(CEmiter &em)
{
  Particle_PrepareTexture(&_toStar01, PBT_ADDALPHA);
  Particle_SetTexturePart( 512, 512, 0, 0);

  FLOAT tmNow = _pTimer->GetLerpedCurrentTick();

  CTextureData *pTD = (CTextureData *) _toFireworks01Gradient.GetData();
  ULONG *pcol=pTD->GetRowPointer(em.em_iGlobal);

  FLOAT fLerpFactor=_pTimer->GetLerpFactor();
  for(INDEX i=0; i<em.em_aepParticles.Count(); i++)
  {
    const CEmittedParticle &ep=em.em_aepParticles[i];
    if(ep.ep_tmEmitted<0) continue;
    FLOAT3D vPos=Lerp(ep.ep_vLastPos, ep.ep_vPos, fLerpFactor);
    FLOAT fRot=Lerp(ep.ep_fLastRot, ep.ep_fRot, fLerpFactor);
    INDEX iIndex=INDEX((tmNow-ep.ep_tmEmitted)*2.0f/(ep.ep_tmLife)*255.0f)%255;
    COLOR col=MulColors(ByteSwap(pcol[iIndex]), MulColors(ep.ep_colColor, em.em_colGlobal));
    Particle_RenderSquare( vPos, ep.ep_fStretch, fRot, col);
  }
  // all done
  Particle_Flush();
}
