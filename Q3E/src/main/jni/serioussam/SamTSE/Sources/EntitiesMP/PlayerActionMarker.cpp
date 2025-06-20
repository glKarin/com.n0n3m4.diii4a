/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#line 17 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"

#include "EntitiesMP/StdH/StdH.h"

#include <EntitiesMP/PlayerActionMarker.h>
#include <EntitiesMP/PlayerActionMarker_tables.h>
void CPlayerActionMarker::SetDefaultProperties(void) {
  m_paaAction = PAA_RUN ;
  m_tmWait = 0.0f;
  m_penDoorController = NULL;
  m_penTrigger = NULL;
  m_fSpeed = 1.0f;
  m_penItem = NULL;
  CMarker::SetDefaultProperties();
}
  
#line 72 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
const CTString & CPlayerActionMarker::GetDescription(void)const {
#line 73 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
CTString strAction  = PlayerAutoAction_enum  . NameForValue  (INDEX (m_paaAction ));
#line 74 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
if(m_penTarget  == NULL ){
#line 75 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
((CTString &) m_strDescription ) . PrintF  ("%s (%s)-><none>" , (const char  *) m_strName  , (const char  *) strAction );
#line 76 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
}else {
#line 77 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
((CTString &) m_strDescription ) . PrintF  ("%s (%s)->%s" , (const char  *) m_strName  , (const char  *) strAction  , 
#line 78 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
(const char  *) m_penTarget  -> GetName  ());
#line 79 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
}
#line 80 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
return m_strDescription ;
#line 81 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
}
  
#line 84 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
BOOL CPlayerActionMarker::DropsMarker(CTFileName & fnmMarkerClass,CTString & strTargetProperty)const {
#line 85 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
fnmMarkerClass  = CTFILENAME  ("Classes\\PlayerActionMarker.ecl");
#line 86 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
strTargetProperty  = "Target";
#line 87 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
return TRUE ;
#line 88 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
}
  
#line 91 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
BOOL CPlayerActionMarker::HandleEvent(const CEntityEvent & ee) 
#line 92 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
{
#line 94 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
if(ee  . ee_slEvent  == EVENTCODE_ETrigger ){
#line 95 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
ETrigger  & eTrigger  = (ETrigger  &) ee ;
#line 97 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
if(IsDerivedFromClass  (eTrigger  . penCaused  , "Player")){
#line 99 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
EAutoAction  eAutoAction ;
#line 100 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
eAutoAction  . penFirstMarker  = this ;
#line 101 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
eTrigger  . penCaused  -> SendEvent  (eAutoAction );
#line 102 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
}
#line 103 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
return TRUE ;
#line 104 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
}
#line 105 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
return FALSE ;
#line 106 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
}
BOOL CPlayerActionMarker::
#line 110 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
Main(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CPlayerActionMarker_Main
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CPlayerActionMarker::Main expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 112 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
InitAsEditorModel  ();
#line 113 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
SetPhysicsFlags  (EPF_MODEL_IMMATERIAL );
#line 114 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
SetCollisionFlags  (ECF_IMMATERIAL );
#line 117 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
SetModel  (MODEL_MARKER );
#line 118 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
SetModelMainTexture  (TEXTURE_MARKER );
#line 120 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
m_tmWait  = ClampDn  (m_tmWait  , 0.05f);
#line 122 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
Return(STATE_CURRENT,EVoid());
#line 122 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTSE/Sources/EntitiesMP/PlayerActionMarker.es"
return TRUE; ASSERT(FALSE); return TRUE;};