/*
 * This file is generated by Entity Class Compiler, (c) CroTeam 1997-98
 */

#line 17 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"

#include "Entities/StdH/StdH.h"

#include <Entities/ShipMarker.h>
#include <Entities/ShipMarker_tables.h>
void CShipMarker::SetDefaultProperties(void) {
  m_bHarbor = FALSE ;
  m_fSpeed = -1.0f;
  m_fRotation = -1.0f;
  m_fAcceleration = 10.0f;
  m_fRockingV = -1.0f;
  m_fRockingA = -1.0f;
  m_tmRockingChange = 3.0f;
  CMarker::SetDefaultProperties();
}
  
#line 42 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
BOOL CShipMarker::DropsMarker(CTFileName & fnmMarkerClass,CTString & strTargetProperty)const {
#line 43 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
fnmMarkerClass  = CTFILENAME  ("Classes\\ShipMarker.ecl");
#line 44 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
strTargetProperty  = "Target";
#line 45 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
return TRUE ;
#line 46 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
}
BOOL CShipMarker::
#line 48 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
Main(const CEntityEvent &__eeInput) {
#undef STATE_CURRENT
#define STATE_CURRENT STATE_CShipMarker_Main
  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_EVoid, "CShipMarker::Main expects 'EVoid' as input!");  const EVoid &e = (const EVoid &)__eeInput;
#line 50 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
InitAsEditorModel  ();
#line 51 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
SetPhysicsFlags  (EPF_MODEL_IMMATERIAL );
#line 52 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
SetCollisionFlags  (ECF_IMMATERIAL );
#line 55 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
SetModel  (MODEL_MARKER );
#line 56 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
SetModelMainTexture  (TEXTURE_MARKER );
#line 58 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
Return(STATE_CURRENT,EVoid());
#line 58 "/data/data/com.termux/files/home/doom3/SeriousSamClassic-1.10.7/SamTFE/Sources/Entities/ShipMarker.es"
return TRUE; ASSERT(FALSE); return TRUE;};