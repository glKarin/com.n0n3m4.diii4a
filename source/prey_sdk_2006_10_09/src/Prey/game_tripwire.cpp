#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
/**********************************************************************

hhTriggerTripwire

**********************************************************************/
CLASS_DECLARATION( idEntity, hhTriggerTripwire )
	EVENT( EV_Enable,		hhTriggerTripwire::Event_Enable )
	EVENT( EV_Disable,		hhTriggerTripwire::Event_Disable )
	EVENT( EV_Activate,		hhTriggerTripwire::Event_Activate )
	EVENT( EV_Deactivate,	hhTriggerTripwire::Event_Deactivate )
END_CLASS

/*
================
hhTriggerTripwire::Spawn
================
*/
void hhTriggerTripwire::Spawn( void ) {
	m_fMaxBeamDistance = spawnArgs.GetFloat("lengthBeam", "4096");

	m_CallFunc.ParseFunctionKeyValue( spawnArgs.GetString("call") );

	m_CallFuncRef.ParseFunctionKeyValue( spawnArgs.GetString("callRef") );
	m_CallFuncRef.InsertParm_Entity( this, 0 );

	m_CallFuncRefActivator.ParseFunctionKeyValue( spawnArgs.GetString("callRefActivator") );
	m_CallFuncRefActivator.InsertParm_Entity( this, 0 );

	m_TriggerBehavior = (triggerBehavior_t)spawnArgs.GetInt("triggerBehavior");

	GetPhysics()->GetContents( 0 );
	GetPhysics()->DisableClip();

	// Spawn the eyebeam
	m_pBeamEntity = hhBeamSystem::SpawnBeam( GetOrigin(), spawnArgs.GetString("beam") );
	if (m_pBeamEntity.IsValid()) {
		m_pBeamEntity->SetOrigin( GetOrigin() );
		m_pBeamEntity->SetAxis( GetAxis() );
		m_pBeamEntity->Bind( this, true );
	}

	GetTriggerClasses( spawnArgs );

	ToggleBeam( true );

	m_pOwner = NULL;
}

void hhTriggerTripwire::Save(idSaveGame *savefile) const {
	savefile->WriteFloat(m_fMaxBeamDistance);

	savefile->WriteStaticObject(m_CallFunc);
	savefile->WriteStaticObject(m_CallFuncRef);
	savefile->WriteStaticObject(m_CallFuncRefActivator);

	m_pBeamEntity.Save(savefile);

	savefile->WriteInt(m_TriggerClasses.Num());				// idList<idStr>
	for (int i=0; i<m_TriggerClasses.Num(); i++) {
		savefile->WriteString(m_TriggerClasses[i]);
	}

	savefile->WriteInt(m_TriggerBehavior);
	savefile->WriteObject( m_pOwner );
}

void hhTriggerTripwire::Restore( idRestoreGame *savefile ) {
	int num;

	savefile->ReadFloat(m_fMaxBeamDistance);

	savefile->ReadStaticObject(m_CallFunc);
	savefile->ReadStaticObject(m_CallFuncRef);
	savefile->ReadStaticObject(m_CallFuncRefActivator);

	m_pBeamEntity.Restore(savefile);

	m_TriggerClasses.Clear();
	savefile->ReadInt(num);									// idList<idStr>
	m_TriggerClasses.SetNum(num);
	for (int i=0; i<num; i++) {
		savefile->ReadString(m_TriggerClasses[i]);
	}

	savefile->ReadInt((int&)m_TriggerBehavior);
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_pOwner ) );
}

/*
===============
hhTriggerTripwire::~hhTriggerTripwire
===============
*/

hhTriggerTripwire::~hhTriggerTripwire() {
	SAFE_REMOVE( m_pBeamEntity );
}

/*
================
hhSecurityEyeTripwire::SetOwner
================
*/
void hhTriggerTripwire::SetOwner( idEntity* pOwner ) {
	m_pOwner = pOwner;
}

/*
===============
hhTriggerTripwire::NotifyTargets
===============
*/
void hhTriggerTripwire::NotifyTargets( idEntity* pActivator ) {
	ActivateTargets( pActivator );

	if( m_pOwner ) {
		m_pOwner->ProcessEvent( &EV_Notify, pActivator );
	}
}

/*
===============
hhTriggerTripwire::ToggleBeam
===============
*/
void hhTriggerTripwire::ToggleBeam( bool bOn ) {
	if( bOn ) {
		BecomeActive( TH_TICKER );
	}
	else {
		BecomeInactive( TH_TICKER );
	}

	if( m_pBeamEntity.IsValid() ) {
		m_pBeamEntity->Activate( bOn );
	}
}

/*
===============
hhTriggerTripwire::Ticker
===============
*/
void hhTriggerTripwire::Ticker() {
	trace_t TraceInfo;
	idEntity* pTraceTarget = NULL;
	idVec3 TraceLength = GetAxis()[0] * m_fMaxBeamDistance;
	idVec3 TraceEndPoint = GetOrigin() + TraceLength;

	gameLocal.clip.TracePoint( TraceInfo, GetOrigin(), TraceEndPoint, MASK_VISIBILITY, this );

	// Update the beam system
	if( m_pBeamEntity.IsValid() ) {
		m_pBeamEntity->SetTargetLocation( TraceInfo.endpos );
	}

	if( TraceInfo.fraction < 1.0f && TraceInfo.c.entityNum < ENTITYNUM_MAX_NORMAL ) {
		pTraceTarget = gameLocal.GetTraceEntity(TraceInfo);
		if( CheckTriggerClass(pTraceTarget) ) {
			CallFunctions( pTraceTarget );	
		
			NotifyTargets( pTraceTarget );

			if( spawnArgs.GetBool("triggerOnce") ) {
				PostEventMS( &EV_Disable, 0 );	
			}
			return;
		}
	}

	if(p_tripwireDebug.GetBool()) {
		gameRenderWorld->DebugLine(colorRed, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + TraceLength * TraceInfo.fraction);

		gameRenderWorld->DebugBox( colorBlue, idBox(GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()) );
	}
}

/*
================
hhTriggerTripwire::CallFunctions
================
*/
void hhTriggerTripwire::CallFunctions( idEntity *activator ) {
	m_CallFunc.CallFunction( spawnArgs );
	m_CallFuncRef.CallFunction( spawnArgs );

	m_CallFuncRefActivator.InsertParm_Entity( activator, 1 );//Needs to be second parm.  After self
	m_CallFuncRefActivator.CallFunction( spawnArgs );
}

/*
===============
hhTriggerTripwire::SetTriggerClasses
===============
*/
void hhTriggerTripwire::SetTriggerClasses( idList<idStr>& List ) {
	m_TriggerClasses.Clear();
	
	m_TriggerClasses.SetNum( List.Num() );

	for(int iIndex = 0; iIndex < List.Num(); ++iIndex) {
		m_TriggerClasses[iIndex] = List[iIndex];
	}
}

/*
===============
hhTriggerTripwire::CheckTriggerClass
===============
*/
bool hhTriggerTripwire::CheckTriggerClass( idEntity* pActivator ) {
	if( !pActivator ) {
		return false;
	}

	if(!m_TriggerClasses.Num() || m_TriggerBehavior == TB_ANY) {
		return true;
	}

	for(int iIndex = 0; iIndex < m_TriggerClasses.Num(); ++iIndex) {
		//Look for exact match then try for prefix match
		if( !idStr(pActivator->spawnArgs.GetString("classname")).Icmp(m_TriggerClasses[iIndex].c_str()) ||
			!idStr(pActivator->spawnArgs.GetString("classname")).IcmpPrefix(m_TriggerClasses[iIndex].c_str()) ) {
			if( pActivator && pActivator->IsType(hhPlayer::Type) ) {//Player needs client
				hhPlayer* pPlayer = static_cast<hhPlayer*>(pActivator);
				if( !pPlayer || pPlayer->noclip || (pPlayer->GetPhysics()->GetClipMask() & MASK_SPIRITPLAYER) == MASK_SPIRITPLAYER ) {
					return false;
				}
			}
			// Vehicles are added for trigger behaviors including players, so here we check if the pilot is a player and should touch triggers
			// Monsters piloting vehicles currently do not get scanned by tripwires
			if( pActivator && pActivator->IsType(hhVehicle::Type) ) {
				hhVehicle *pVehicle = static_cast<hhVehicle*>(pActivator);
				if( !pVehicle || pVehicle->IsNoClipping() || !pVehicle->GetPilot() || !pVehicle->GetPilot()->IsType(hhPlayer::Type) ) {
					return false;
				}
			}
			return true;
		}
	}

	return false;
}

/*
===============
hhTriggerTripwire::GetTriggerClasses
===============
*/
void hhTriggerTripwire::GetTriggerClasses( idDict& Args ) {
	const idKeyValue* pKeyValue = NULL;
	int iNumKeyValues = Args.GetNumKeyVals();

	m_TriggerClasses.Clear();

	switch( m_TriggerBehavior ) {
		case TB_PLAYER_ONLY:
			m_TriggerClasses.AddUnique( "player" );
			m_TriggerClasses.AddUnique( "vehicle" );
			break;

		case TB_FRIENDLIES_ONLY:
			m_TriggerClasses.AddUnique( "player" );
			m_TriggerClasses.AddUnique( "vehicle" );
			m_TriggerClasses.AddUnique( "character" );
			break;

		case TB_MONSTERS_ONLY:
			m_TriggerClasses.AddUnique( "monster" );
			break;

		case TB_PLAYER_MONSTERS_FRIENDLIES:
			m_TriggerClasses.AddUnique( "player" );
			m_TriggerClasses.AddUnique( "vehicle" );
			m_TriggerClasses.AddUnique( "monster" );
			m_TriggerClasses.AddUnique( "character" );
			break;

		case TB_SPECIFIC_ENTITIES:
			for( int iIndex = 0; iIndex < iNumKeyValues; ++iIndex ) {
				pKeyValue = Args.GetKeyVal( iIndex );
				if ( !pKeyValue->GetKey().Cmpn( "trigger_class", 13 ) ) {
					m_TriggerClasses.AddUnique( pKeyValue->GetValue() );
				}
			}
			break;

		default:
			HH_ASSERT(!"Invalid trigger behavior!\n");
			break;
	}
}

/*
================
hhTriggerTripwire::Event_Enable
================
*/
void hhTriggerTripwire::Event_Enable( void ) {
	ToggleBeam( true );
}

/*
================
hhTriggerTripwire::Event_Disable
================
*/
void hhTriggerTripwire::Event_Disable( void ) {
	ToggleBeam( false );
}

/*
================
hhTriggerTripwire::Event_Activate
================
*/
void hhTriggerTripwire::Event_Activate( idEntity *activator ) {
	ToggleBeam( true );
}

/*
================
hhTriggerTripwire::Event_Deactivate
================
*/
void hhTriggerTripwire::Event_Deactivate() {
	ToggleBeam( false );
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
