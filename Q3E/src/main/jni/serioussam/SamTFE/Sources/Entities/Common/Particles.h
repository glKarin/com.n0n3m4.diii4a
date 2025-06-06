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
void DECL_DLL Particles_BloodTrail(CEntity *pen);
void DECL_DLL Particles_FlameThrower(const CPlacement3D &plThis, const CPlacement3D &plOther,
                            FLOAT fThisElapsed, FLOAT fOtherElapsed);
void DECL_DLL Particles_Ghostbuster(const FLOAT3D &vSrc, const FLOAT3D &vDst, INDEX ctRays, FLOAT fSize, FLOAT fPower = 1.0f,
                           FLOAT fKneeDivider = 33.3333333f);
void DECL_DLL Particles_FlameThrower(const CPlacement3D &plThis, const CPlacement3D &plOther,
                            FLOAT fThisElapsed, FLOAT fOtherElapsed);
INDEX DECL_DLL Particles_FireBreath(CEntity *pen, FLOAT3D vSource, FLOAT3D vTarget, FLOAT tmStart, FLOAT tmStop);
INDEX DECL_DLL Particles_Regeneration(CEntity *pen, FLOAT tmStart, FLOAT tmStop, FLOAT fYFactor, BOOL bDeath);

void DECL_DLL Particles_Stardust(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Rising(CEntity *pen, FLOAT m_fActivateTime, FLOAT m_fDeactivateTime, FLOAT fStretchAll, FLOAT fStretchX, FLOAT fStretchY, FLOAT fStretchZ, FLOAT fSize,
                      enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Spiral(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Emanate(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Fountain(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_Atomic(CEntity *pen, FLOAT fSize, FLOAT fHeight, enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_BloodSpray( enum SprayParticlesType sptType, CEntity *penSpray, FLOAT3D vGDir, FLOAT fGA, FLOATaabbox3D boxOwner, FLOAT3D vSpilDirection, FLOAT tmStarted, FLOAT fDamagePower);
void DECL_DLL Particles_EmanatePlane(CEntity *pen, FLOAT fSizeX, FLOAT fSizeY, FLOAT fSizeZ, 
                            FLOAT fParticleSize, FLOAT fAway, FLOAT fSpeed, 
                            enum ParticleTexture ptTexture, INDEX ctParticles);
void DECL_DLL Particles_WaterfallFoam(CEntity *pen, FLOAT fSizeX, FLOAT fSizeY, FLOAT fSizeZ, 
                             FLOAT fParticleSize, FLOAT fSpeed, FLOAT fSpeedY, FLOAT fLife, INDEX ctParticles);
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

void DECL_DLL Particles_Rain( CEntity *pen, FLOAT fGridSize, INDEX ctGrids, FLOAT fFactor, 
                    CTextureData *ptdRainMap, FLOATaabbox3D &boxRainMap);
void DECL_DLL Particles_Snow( CEntity *pen, FLOAT fGridSize, INDEX ctGrids);
void DECL_DLL Particles_Lightning( FLOAT3D vSrc, FLOAT3D vDst, FLOAT fTimeStart);
void DECL_DLL Particles_BulletSpray(CEntity *pen,  FLOAT3D vGDir, enum EffectParticlesType eptType, FLOAT tmSpawn, FLOAT3D vDirection);
void DECL_DLL Particles_MachineBullets( CEntity *pen, TIME tmStart, TIME tmEnd, FLOAT fFrequency);
void DECL_DLL Particles_EmptyShells( CEntity *pen, ShellLaunchData *asldData);
void DECL_DLL Particles_DestroyingObelisk(CEntity *penSpray, FLOAT tmStarted);
void DECL_DLL Particles_DestroyingPylon(CEntity *penSpray, FLOAT3D vDamageDir, FLOAT tmStarted);
void DECL_DLL Particles_HitGround(CEntity *penSpray, FLOAT tmStarted, FLOAT fSizeMultiplier);
void DECL_DLL Particles_MetalParts( CEntity *pen, FLOAT tmStarted, FLOATaabbox3D boxOwner, FLOAT fDamage);
void DECL_DLL Particles_DamageSmoke( CEntity *pen, FLOAT tmStarted, FLOATaabbox3D boxOwner, FLOAT fDamage);
void DECL_DLL Particles_RunningDust_Prepare(CEntity *pen);
void DECL_DLL Particles_RunningDust(CEntity *pen);
