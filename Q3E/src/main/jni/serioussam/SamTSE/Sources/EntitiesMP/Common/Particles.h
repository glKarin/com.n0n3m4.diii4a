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

// init particle effects
void DECL_DLL InitParticles(void);
// close particle effects
void DECL_DLL CloseParticles(void);
// function for rendering local viewer particles
void DECL_DLL Particles_ViewerLocal(CEntity *penView);
// different particle effects
void DECL_DLL Particles_RomboidTrail(CEntity *pen);
void DECL_DLL Particles_RomboidTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_BombTrail(CEntity *pen);
void DECL_DLL Particles_BombTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_FirecrackerTrail(CEntity *pen);
void DECL_DLL Particles_FirecrackerTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_SpiralTrail(CEntity *pen);
void DECL_DLL Particles_SpiralTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_ColoredStarsTrail(CEntity *pen);
void DECL_DLL Particles_ColoredStarsTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_WhiteLineTrail(CEntity *pen);
void DECL_DLL Particles_WhiteLineTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_Fireball01Trail(CEntity *pen);
void DECL_DLL Particles_Fireball01Trail_Prepare(CEntity *pen);
void DECL_DLL Particles_GrenadeTrail(CEntity *pen);
void DECL_DLL Particles_GrenadeTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_CannonBall(CEntity *pen, FLOAT fSpeedRatio);
void DECL_DLL Particles_CannonBall_Prepare(CEntity *pen);
void DECL_DLL Particles_LavaTrail(CEntity *pen);
void DECL_DLL Particles_LavaTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_LavaBombTrail(CEntity *pen, FLOAT fSizeMultiplier);
void DECL_DLL Particles_LavaBombTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_RocketTrail(CEntity *pen, FLOAT fStretch);
void DECL_DLL Particles_RocketTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_ExplosionDebris1(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply=C_WHITE|CT_OPAQUE);
void DECL_DLL Particles_ExplosionDebris2(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply=C_WHITE|CT_OPAQUE);
void DECL_DLL Particles_ExplosionDebris3(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply=C_WHITE|CT_OPAQUE);
void DECL_DLL Particles_ExplosionSmoke(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch, COLOR colMultiply=C_WHITE|CT_OPAQUE);
void DECL_DLL Particles_BloodTrail(CEntity *pen);
void DECL_DLL Particles_Ghostbuster(const FLOAT3D &vSrc, const FLOAT3D &vDst, INDEX ctRays, FLOAT fSize, FLOAT fPower = 1.0f,
                           FLOAT fKneeDivider = 33.3333333f);
void DECL_DLL Particles_Burning(CEntity *pen, FLOAT fPower, FLOAT fTimeRatio);
void DECL_DLL Particles_BrushBurning(CEntity *pen, FLOAT3D vPos[], INDEX ctCount, FLOAT3D vPlane,
                                     FLOAT fPower, FLOAT fTimeRatio);
void DECL_DLL Particles_FlameThrower(const CPlacement3D &plLeader, const CPlacement3D &plFollower,
                            FLOAT3D vSpeedLeader, FLOAT3D vSpeedFollower,
                            FLOAT fLeaderLiving, FLOAT fFollowerLiving,
                            INDEX iRndSeed, BOOL bFollowerIsPipe);
void DECL_DLL Particles_FlameThrowerStart(const CPlacement3D &plPipe, FLOAT fStartTime, FLOAT fStopTime);
void DECL_DLL Particles_Twister( CEntity *pen, FLOAT fStretch, FLOAT fStartTime, FLOAT fFadeOutStartTime, FLOAT fParticleStretch);
INDEX DECL_DLL Particles_FireBreath(CEntity *pen, FLOAT3D vSource, FLOAT3D vTarget, FLOAT tmStart, FLOAT tmStop);
INDEX DECL_DLL Particles_Regeneration(CEntity *pen, FLOAT tmStart, FLOAT tmStop, FLOAT fYFactor, BOOL bDeath);

void DECL_DLL Particles_Stardust(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Rising(CEntity *pen, FLOAT m_fActivateTime, FLOAT m_fDeactivateTime, FLOAT fStretchAll, FLOAT fStretchX, FLOAT fStretchY, FLOAT fStretchZ, FLOAT fSize,
                      enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Spiral(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Emanate(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles, FLOAT fMipFactorDisappear);
void DECL_DLL Particles_Fountain(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Atomic(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_PowerUpIndicator( CEntity *pen, enum ParticleTexture ptTexture, FLOAT fSize,
              FLOAT fScale, FLOAT fHeight, INDEX ctEllipses, INDEX iTrailCount);
void DECL_DLL Particles_BloodSpray( enum SprayParticlesType sptType, FLOAT3D vPos, FLOAT3D vGDir, FLOAT fGA, FLOATaabbox3D boxOwner, FLOAT3D vSpilDirection,
                                   FLOAT tmStarted, FLOAT fDamagePower, COLOR colMultiply=C_WHITE|CT_OPAQUE);
void DECL_DLL Particles_EmanatePlane(CEntity *pen, FLOAT fSizeX, FLOAT fSizeY, FLOAT fSizeZ, 
                            FLOAT fParticleSize, FLOAT fAway, FLOAT fSpeed, 
                            enum ParticleTexture ptTexture, INDEX ctParticles, FLOAT fMipFactorDisappear);
void DECL_DLL Particles_WaterfallFoam(CEntity *pen, FLOAT fSizeX, FLOAT fSizeY, FLOAT fSizeZ, 
                             FLOAT fParticleSize, FLOAT fSpeed, FLOAT fSpeedY, FLOAT fLife, INDEX ctParticles);
void DECL_DLL Particles_ChimneySmoke(CEntity *pen, INDEX ctCount, FLOAT fStretchAll, FLOAT fMipDisappearDistance);
void DECL_DLL Particles_Waterfall(CEntity *pen, INDEX ctCount, FLOAT fStretchAll, FLOAT fStretchX, FLOAT fStretchY,
                                  FLOAT fStretchZ, FLOAT fSize, FLOAT fMipDisappearDistance, FLOAT fParam1);
void DECL_DLL Particles_SandFlow(CEntity *pen, FLOAT fStretchAll, FLOAT fSize, FLOAT fHeight, FLOAT fBirthTime, FLOAT fDeathTime,
                        INDEX ctParticles);
void DECL_DLL Particles_WaterFlow( CEntity *pen, FLOAT fStretchAll, FLOAT fSize, FLOAT fHeight, FLOAT fStartTime, FLOAT fStopTime,
                     INDEX ctParticles);
void DECL_DLL Particles_LavaFlow( CEntity *pen, FLOAT fStretchAll, FLOAT fSize, FLOAT fHeight, FLOAT fStartTime, FLOAT fStopTime,
                     INDEX ctParticles);
void DECL_DLL Particles_Death(CEntity *pen, TIME tmStart);
void DECL_DLL Particles_Appearing(CEntity *pen, TIME tmStart);
void DECL_DLL Particles_ElectricitySparks( CEntity *pen, FLOAT fTimeAppear, FLOAT fSize, FLOAT fHeight, INDEX ctParticles);
void DECL_DLL Particles_LavaErupting(CEntity *pen, FLOAT fStretchAll, FLOAT fSize, 
                            FLOAT fStretchX, FLOAT fStretchY, FLOAT fStretchZ, 
                            FLOAT fActivateTime);
void DECL_DLL Particles_BeastProjectileTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_BeastProjectileTrail( CEntity *pen, FLOAT fSize, FLOAT fHeight, INDEX ctParticles);
void DECL_DLL Particles_BeastBigProjectileTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_BeastBigProjectileTrail( CEntity *pen, FLOAT fSize, FLOAT fZOffset, FLOAT fYOffset, INDEX ctParticles);
void DECL_DLL Particles_BeastProjectileDebrisTrail_Prepare(CEntity *pen);
void DECL_DLL Particles_BeastProjectileDebrisTrail(CEntity *pen, FLOAT fSizeMultiplier);
void DECL_DLL Particles_AfterBurner(CEntity *pen, FLOAT m_tmSpawn, FLOAT fStretch, INDEX iGradientType=0);
void DECL_DLL Particles_AfterBurner_Prepare(CEntity *pen);
void DECL_DLL Particles_RocketMotorBurning(CEntity *pen, FLOAT tmSpawn, FLOAT3D vStretch, FLOAT fStretch, FLOAT ctCount);

//void DECL_DLL Particles_Growth(CEntity *pen, CTextureData *ptdGrowthMap, FLOATaabbox3D &boxGrowthMap, CEntity *penEPH);
class CGrowth {
public:
  FLOAT3D vRender;
  FLOAT   fDistanceToViewer;
  FLOAT   fSize;
  INDEX   iShapeX;
  INDEX   iShapeY;
  UBYTE   ubShade;
  UBYTE   ubFade;
};

class CGrowthCache {
public:
  ULONG   ulID;
  FLOAT3D vLastPos;
  INDEX   iGridSide;
  FLOAT   fStep;
  CListNode cgc_Node;
  CStaticStackArray<CGrowth> acgParticles;
};

void DECL_DLL Particles_Growth(CEntity *pen, CTextureData *ptdGrowthMap, FLOATaabbox3D &boxGrowthMap, CEntity *penEPH, INDEX iDrawPort);

void DECL_DLL Particles_Rain( CEntity *pen, FLOAT fGridSize, INDEX ctGrids, FLOAT fFactor, 
                    CTextureData *ptdRainMap, FLOATaabbox3D &boxRainMap);
void DECL_DLL Particles_Snow( CEntity *pen, FLOAT fGridSize, INDEX ctGrids, FLOAT fPower,
                    CTextureData *ptdRainMap, FLOATaabbox3D &boxRainMap, FLOAT fSnowStart);
void DECL_DLL Particles_Lightning( FLOAT3D vSrc, FLOAT3D vDst, FLOAT fTimeStart);
void DECL_DLL Particles_BulletSpray(INDEX iRndBase,  FLOAT3D vSource, FLOAT3D vGDir, enum EffectParticlesType eptType, FLOAT tmSpawn,
                                    FLOAT3D vDirection, FLOAT fStretch);
void DECL_DLL Particles_MachineBullets( CEntity *pen, TIME tmStart, TIME tmEnd, FLOAT fFrequency);
void DECL_DLL Particles_EmptyShells( CEntity *pen, ShellLaunchData *asldData);
void DECL_DLL Particles_DestroyingObelisk(CEntity *penSpray, FLOAT tmStarted);
void DECL_DLL Particles_DestroyingPylon(CEntity *penSpray, FLOAT3D vDamageDir, FLOAT tmStarted);
void DECL_DLL Particles_HitGround(CEntity *penSpray, FLOAT tmStarted, FLOAT fSizeMultiplier);
void DECL_DLL Particles_MetalParts( CEntity *pen, FLOAT tmStarted, FLOATaabbox3D boxOwner, FLOAT fDamage);
void DECL_DLL Particles_DamageSmoke( CEntity *pen, FLOAT tmStarted, FLOATaabbox3D boxOwner, FLOAT fDamage);
void DECL_DLL Particles_RunningDust_Prepare(CEntity *pen);
void DECL_DLL Particles_RunningDust(CEntity *pen);
void DECL_DLL Particles_ShooterFlame(const CPlacement3D &plEnd, const CPlacement3D &plStart,
                            FLOAT fEndElapsed, FLOAT fStartElapsed);
void DECL_DLL Particles_SummonerProjectileFly(CEntity *pen, FLOAT fSize, FLOAT fTimeAdjust);
void DECL_DLL Particles_SummonerProjectileExplode(CEntity *pen, FLOAT fSize, FLOAT fBeginTime, FLOAT fDuration, FLOAT fTimeAdjust);
void DECL_DLL Particles_SummonerExplode(CEntity *pen, FLOAT3D vCenter, FLOAT fArea, FLOAT fSize, FLOAT fBeginTime, FLOAT fDuration);
void DECL_DLL Particles_ExotechLarvaLaser(CEntity *pen, FLOAT3D vSource, FLOAT3D vTarget);
void DECL_DLL Particles_Smoke(CEntity *pen, FLOAT3D vOffset, INDEX ctCount, FLOAT fLife, FLOAT fSpread, FLOAT fStretchAll, FLOAT fYSpeed);
void DECL_DLL Particles_Windblast( CEntity *pen, FLOAT fStretch, FLOAT fFadeOutStartTime);
void DECL_DLL Particles_CollectEnergy(CEntity *pen, FLOAT tmStart, FLOAT tmStop);
void DECL_DLL Particles_SniperResidue(CEntity *pen, FLOAT3D vSource, FLOAT3D vTarget); 
void DECL_DLL Particles_GrowingSwirl( CEntity *pen, FLOAT fStretch, FLOAT fStartTime);
void DECL_DLL Particles_SummonerDisappear( CEntity *pen, FLOAT tmStart);
void DECL_DLL Particles_DisappearDust( CEntity *pen, FLOAT fStretch, FLOAT fStartTime);
void DECL_DLL Particles_DustFall(CEntity *pen, FLOAT tmStart, FLOAT3D vStretch);
void DECL_DLL Particles_AirElementalBlow(class CEmiter &em);
void DECL_DLL Particles_SummonerStaff(class CEmiter &em);
void DECL_DLL Particles_AirElemental(CEntity *pen, FLOAT fStretch, FLOAT fFade, FLOAT tmDeath, COLOR colMultiply);
void DECL_DLL Particles_MeteorTrail(CEntity *pen, FLOAT fStretch, FLOAT fLength, FLOAT3D vSpeed);
void DECL_DLL Particles_Leaves(CEntity *penTree, FLOATaabbox3D boxSize, FLOAT3D vSource, FLOAT fDamagePower,
                               FLOAT fLaunchPower, FLOAT3D vGDir, FLOAT fGA, FLOAT tmStarted, COLOR colMaxColor);
void DECL_DLL Particles_LarvaEnergy(CEntity *pen, FLOAT3D vOffset);
void DECL_DLL Particles_AirElemental_Comp(CModelObject *mo, FLOAT fStretch, FLOAT fFade, CPlacement3D pl);
void DECL_DLL Particles_Burning_Comp(CModelObject *mo, FLOAT fPower, CPlacement3D pl);
void DECL_DLL Particles_ModelGlow( CEntity *pen, FLOAT tmEnd, enum ParticleTexture ptTexture, FLOAT fSize, FLOAT iVtxStep, FLOAT fAnimSpd, COLOR iCol);
void DECL_DLL Particles_RunAfterBurner(CEntity *pen, FLOAT tmEnd, FLOAT fStretch, INDEX iGradientType);
void DECL_DLL Particles_Fireworks01(CEmiter &em);
void DECL_DLL Particles_ModelGlow2( CModelObject *mo,  CPlacement3D pl, FLOAT tmEnd, enum ParticleTexture ptTexture, FLOAT fSize, FLOAT iVtxStep, FLOAT fAnimSpd, COLOR iCol);
void DECL_DLL Particles_RunAfterBurner(CEntity *pen, FLOAT tmEnd, FLOAT fStretch, INDEX iGradientType);
