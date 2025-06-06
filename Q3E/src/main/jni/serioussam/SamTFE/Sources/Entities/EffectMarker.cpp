/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#line 17 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"

#include "Entities/StdH/StdH.h"
#include "Entities/Effector.h"
#include "Entities/BackgroundViewer.h"
#include "Entities/WorldSettingsController.h"

#include <Entities/EffectMarker.h>
#include <Entities/EffectMarker_tables.h>
void CEffectMarker::SetDefaultProperties(void) {
  m_emtType = EMT_NONE ;
  m_penModel = NULL;
  m_tmEffectLife = 10.0f;
  m_penModel2 = NULL;
  m_penEffector = NULL;
  m_fShakeFalloff = 250.0f;
  m_fShakeFade = 3.0f;
  m_fShakeIntensityY = 0.1f;
  m_fShakeFrequencyY = 5.0f;
  m_fShakeIntensityB = 2.5f;
  m_fShakeFrequencyB = 7.2f;
  m_fShakeIntensityZ = 0.0f;
  m_fShakeFrequencyZ = 5.0f;
  CMarker::SetDefaultProperties();
}
  
#line 69 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
BOOL CEffectMarker::IsTargetValid(SLONG slPropertyOffset,CEntity * penTarget) 
#line 70 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 71 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(penTarget  == NULL )
#line 72 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 73 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
return FALSE ;
#line 74 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 76 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(slPropertyOffset  == offsetof  (CEffectMarker  , m_penModel ) || 
#line 77 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
slPropertyOffset  == offsetof  (CEffectMarker  , m_penModel2 ))
#line 78 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 79 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
return IsOfClass  (penTarget  , "ModelHolder2");
#line 80 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 81 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
return TRUE ;
#line 82 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
  
#line 85 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
BOOL CEffectMarker::HandleEvent(const CEntityEvent & ee) 
#line 86 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 87 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(ee  . ee_slEvent  == EVENTCODE_ETrigger )
#line 88 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 89 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
switch(m_emtType )
#line 90 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 91 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_SHAKE_IT_BABY : 
#line 92 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 94 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CWorldSettingsController  * pwsc  = NULL ;
#line 96 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CBackgroundViewer  * penBcgViewer  = (CBackgroundViewer  *) GetWorld  () -> GetBackgroundViewer  ();
#line 97 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(penBcgViewer  != NULL )
#line 98 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 99 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  = (CWorldSettingsController  *) penBcgViewer  -> m_penWorldSettingsController  . ep_pen ;
#line 100 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_tmShakeStarted  = _pTimer  -> CurrentTick  ();
#line 101 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_vShakePos  = GetPlacement  () . pl_PositionVector ;
#line 102 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_fShakeFalloff  = m_fShakeFalloff ;
#line 103 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_fShakeFade  = m_fShakeFade ;
#line 104 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_fShakeIntensityZ  = m_fShakeIntensityZ ;
#line 105 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_tmShakeFrequencyZ  = m_fShakeFrequencyZ ;
#line 106 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_fShakeIntensityY  = m_fShakeIntensityY ;
#line 107 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_tmShakeFrequencyY  = m_fShakeFrequencyY ;
#line 108 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_fShakeIntensityB  = m_fShakeIntensityB ;
#line 109 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
pwsc  -> m_tmShakeFrequencyB  = m_fShakeFrequencyB ;
#line 110 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 111 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 112 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 113 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_HIDE_ENTITY : 
#line 114 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 115 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penTarget  != NULL )
#line 116 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 117 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penTarget  -> SetFlags  (m_penTarget  -> GetFlags  () | ENF_HIDDEN );
#line 118 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 119 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 120 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 121 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_SHOW_ENTITY : 
#line 122 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 123 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penTarget  != NULL )
#line 124 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 125 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penTarget  -> SetFlags  (m_penTarget  -> GetFlags  () & ~ ENF_HIDDEN );
#line 126 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 127 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 128 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 129 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_PLAYER_APPEAR : 
#line 130 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penModel  != NULL  && IsOfClass  (m_penModel  , "ModelHolder2"))
#line 131 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 132 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CModelObject * pmo  = m_penModel  -> GetModelObject  ();
#line 133 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(pmo  != NULL )
#line 134 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 136 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CPlacement3D plFX  = m_penModel  -> GetPlacement  ();
#line 137 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CEntity  * penFX  = CreateEntity  (plFX  , CLASS_EFFECTOR );
#line 138 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
ESpawnEffector  eSpawnFX ;
#line 139 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . tmLifeTime  = m_tmEffectLife ;
#line 140 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . eetType  = ET_PORTAL_LIGHTNING ;
#line 141 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . penModel  = m_penModel ;
#line 142 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
penFX  -> Initialize  (eSpawnFX );
#line 143 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 144 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 145 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 146 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_APPEARING_BIG_BLUE_FLARE : 
#line 147 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 149 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CPlacement3D plFX  = GetPlacement  ();
#line 150 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CEntity  * penFX  = CreateEntity  (plFX  , CLASS_EFFECTOR );
#line 151 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
ESpawnEffector  eSpawnFX ;
#line 152 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . tmLifeTime  = m_tmEffectLife ;
#line 153 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . fSize  = 1.0f;
#line 154 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . eetType  = ET_SIZING_BIG_BLUE_FLARE ;
#line 155 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
penFX  -> Initialize  (eSpawnFX );
#line 156 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 157 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 158 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_BLEND_MODELS : 
#line 159 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penModel  != NULL  && IsOfClass  (m_penModel  , "ModelHolder2") && 
#line 160 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penModel2  != NULL  && IsOfClass  (m_penModel2  , "ModelHolder2"))
#line 161 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 162 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penEffector  == NULL )
#line 163 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 164 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CModelObject * pmo1  = m_penModel  -> GetModelObject  ();
#line 165 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CModelObject * pmo2  = m_penModel2  -> GetModelObject  ();
#line 166 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(pmo1  != NULL  && pmo2  != NULL )
#line 167 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 169 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CPlacement3D plFX  = m_penModel  -> GetPlacement  ();
#line 170 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CEntity  * penFX  = CreateEntity  (plFX  , CLASS_EFFECTOR );
#line 171 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
ESpawnEffector  eSpawnFX ;
#line 172 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . tmLifeTime  = m_tmEffectLife ;
#line 173 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . eetType  = ET_MORPH_MODELS ;
#line 174 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . penModel  = m_penModel ;
#line 175 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . penModel2  = m_penModel2 ;
#line 176 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
penFX  -> Initialize  (eSpawnFX );
#line 177 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  = penFX ;
#line 178 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 179 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 180 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
else 
#line 181 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 182 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  -> SendEvent  (ETrigger  ());
#line 183 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 184 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 185 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 186 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_DISAPPEAR_MODEL : 
#line 187 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penModel  != NULL  && IsOfClass  (m_penModel  , "ModelHolder2"))
#line 188 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 189 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penEffector  == NULL )
#line 190 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 191 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CModelObject * pmo  = m_penModel  -> GetModelObject  ();
#line 192 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(pmo  != NULL )
#line 193 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 195 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CPlacement3D plFX  = m_penModel  -> GetPlacement  ();
#line 196 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CEntity  * penFX  = CreateEntity  (plFX  , CLASS_EFFECTOR );
#line 197 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
ESpawnEffector  eSpawnFX ;
#line 198 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . tmLifeTime  = m_tmEffectLife ;
#line 199 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . eetType  = ET_DISAPPEAR_MODEL ;
#line 200 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . penModel  = m_penModel ;
#line 201 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
penFX  -> Initialize  (eSpawnFX );
#line 202 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  = penFX ;
#line 203 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 204 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 205 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
else 
#line 206 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 207 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  -> SendEvent  (ETrigger  ());
#line 208 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 209 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 210 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 211 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_APPEAR_MODEL : 
#line 212 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penModel  != NULL  && IsOfClass  (m_penModel  , "ModelHolder2"))
#line 213 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 214 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penEffector  == NULL )
#line 215 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 216 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CModelObject * pmo  = m_penModel  -> GetModelObject  ();
#line 217 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(pmo  != NULL )
#line 218 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 220 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CPlacement3D plFX  = m_penModel  -> GetPlacement  ();
#line 221 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CEntity  * penFX  = CreateEntity  (plFX  , CLASS_EFFECTOR );
#line 222 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
ESpawnEffector  eSpawnFX ;
#line 223 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . tmLifeTime  = m_tmEffectLife ;
#line 224 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . eetType  = ET_APPEAR_MODEL ;
#line 225 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . penModel  = m_penModel ;
#line 226 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
penFX  -> Initialize  (eSpawnFX );
#line 227 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  = penFX ;
#line 228 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 229 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 230 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
else 
#line 231 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 232 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  -> SendEvent  (ETrigger  ());
#line 233 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 234 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 235 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 236 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 237 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 238 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
else if(ee  . ee_slEvent  == EVENTCODE_EActivate )
#line 239 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 240 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
switch(m_emtType )
#line 241 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 242 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_APPEAR_DISAPPEAR : 
#line 243 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penModel  != NULL  && IsOfClass  (m_penModel  , "ModelHolder2"))
#line 244 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 245 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CModelObject * pmo  = m_penModel  -> GetModelObject  ();
#line 246 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(pmo  != NULL )
#line 247 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 249 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CPlacement3D plFX  = m_penModel  -> GetPlacement  ();
#line 250 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CEntity  * penFX  = CreateEntity  (plFX  , CLASS_EFFECTOR );
#line 251 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
ESpawnEffector  eSpawnFX ;
#line 252 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . tmLifeTime  = m_tmEffectLife ;
#line 253 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . eetType  = ET_APPEAR_MODEL_NOW ;
#line 254 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . penModel  = m_penModel ;
#line 255 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
penFX  -> Initialize  (eSpawnFX );
#line 256 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  = penFX ;
#line 257 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 258 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 259 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 260 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 261 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 262 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
else if(ee  . ee_slEvent  == EVENTCODE_EDeactivate )
#line 263 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 264 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
switch(m_emtType )
#line 265 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 266 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
case EMT_APPEAR_DISAPPEAR : 
#line 267 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(m_penModel  != NULL  && IsOfClass  (m_penModel  , "ModelHolder2"))
#line 268 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 269 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CModelObject * pmo  = m_penModel  -> GetModelObject  ();
#line 270 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
if(pmo  != NULL )
#line 271 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
{
#line 273 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CPlacement3D plFX  = m_penModel  -> GetPlacement  ();
#line 274 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
CEntity  * penFX  = CreateEntity  (plFX  , CLASS_EFFECTOR );
#line 275 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
ESpawnEffector  eSpawnFX ;
#line 276 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . tmLifeTime  = m_tmEffectLife ;
#line 277 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . eetType  = ET_DISAPPEAR_MODEL_NOW ;
#line 278 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
eSpawnFX  . penModel  = m_penModel ;
#line 279 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
penFX  -> Initialize  (eSpawnFX );
#line 280 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  = penFX ;
#line 281 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 282 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 283 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
break ;
#line 284 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 285 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
#line 286 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
return FALSE ;
#line 287 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
}
BOOL CEffectMarker::
#line 291 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
Main(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CEffectMarker_Main
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CEffectMarker::Main expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 294 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
InitAsEditorModel  ();
#line 295 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
SetPhysicsFlags  (EPF_MODEL_IMMATERIAL );
#line 296 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
SetCollisionFlags  (ECF_IMMATERIAL );
#line 299 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
SetModel  (MODEL_MARKER );
#line 300 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
SetModelMainTexture  (TEXTURE_MARKER );
#line 302 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
m_penEffector  = NULL ;
#line 303 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
Return(STATE_CURRENT,EVoid());
#line 303 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/EffectMarker.es"
return TRUE; ASSERT(FALSE); return TRUE;};