#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
/**********************************************************************

hhSecurityEyeBase

**********************************************************************/

CLASS_DECLARATION( idEntity, hhSecurityEyeBase )
	EVENT( EV_Enable,					hhSecurityEyeBase::Event_Enable )
	EVENT( EV_Disable,					hhSecurityEyeBase::Event_Disable )
	EVENT( EV_Activate,					hhSecurityEyeBase::Event_Activate )
END_CLASS

void hhSecurityEyeBase::Spawn() {
	SpawnEye();
}

void hhSecurityEyeBase::Save(idSaveGame *savefile) const {
	m_pEye.Save(savefile);
}

void hhSecurityEyeBase::Restore( idRestoreGame *savefile ) {
	m_pEye.Restore(savefile);
}

void hhSecurityEyeBase::TransferArg(idDict &Args, const char *key) {
	const idKeyValue *kv = NULL;
	do {
		kv = spawnArgs.MatchPrefix(key, kv);
		if (kv) {
			Args.Set(kv->GetKey().c_str(), kv->GetValue().c_str());
		}
	} while (kv != NULL);
}

void hhSecurityEyeBase::SpawnEye() {

	idVec3 origin = GetOrigin();
	idMat3 axis = GetAxis();
	idVec3 offset = spawnArgs.GetVector("offset_eye");

	idDict Args;
	Args.SetVector( "origin", origin + offset*axis );
	Args.SetMatrix( "rotation", axis );

	TransferArg(Args, "minPitch");
	TransferArg(Args, "maxPitch");
	TransferArg(Args, "minYaw");
	TransferArg(Args, "maxYaw");
	TransferArg(Args, "startPitch");
	TransferArg(Args, "startYaw");
	TransferArg(Args, "pitchRate");
	TransferArg(Args, "yawRate");
	TransferArg(Args, "fov");
	TransferArg(Args, "pathScanRate");
	TransferArg(Args, "usePathScan");
	TransferArg(Args, "triggerOnce");
	TransferArg(Args, "lengthBeam");
	TransferArg(Args, "enabled");
	TransferArg(Args, "health");
	TransferArg(Args, "pathScanNode");
	TransferArg(Args, "call");
	TransferArg(Args, "callRef");
	TransferArg(Args, "callRefActivator");
	TransferArg(Args, "triggerBehavior");
	TransferArg(Args, "target");

	m_pEye = gameLocal.SpawnObject( spawnArgs.GetString("def_eye"), &Args );
	HH_ASSERT( m_pEye.IsValid() && m_pEye.GetEntity() );

	m_pEye->fl.noRemoveWhenUnbound = true;
	m_pEye->Bind(this, true);

	if (m_pEye->IsType(hhSecurityEye::Type)) {
		static_cast<hhSecurityEye*>(m_pEye.GetEntity())->SetBase(this);
	}
	if (spawnArgs.GetBool("nobase")) {
		Hide();
	}
}

void hhSecurityEyeBase::Event_Activate(idEntity *pActivator) {
	if (m_pEye.IsValid()) {
		m_pEye->ProcessEvent(&EV_Activate, pActivator);		// Pass activate messages to eye
	}
}
void hhSecurityEyeBase::Event_Enable() {
	if (m_pEye.IsValid()) {
		m_pEye->ProcessEvent(&EV_Enable);					// Pass enable messages to eye
	}
}
void hhSecurityEyeBase::Event_Disable() {
	if (m_pEye.IsValid()) {
		m_pEye->ProcessEvent(&EV_Disable);					// Pass disable messages to eye
	}
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

/**********************************************************************

hhSecurityEye

**********************************************************************/
const idEventDef EV_Notify("notify", "e");

CLASS_DECLARATION( idEntity, hhSecurityEye )
	EVENT( EV_Enable,					hhSecurityEye::Event_Enable )
	EVENT( EV_Disable,					hhSecurityEye::Event_Disable )
	EVENT( EV_Notify,					hhSecurityEye::Event_Notify )
	EVENT( EV_Activate,					hhSecurityEye::Event_Activate )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
/*
================
hhSecurityEye::Spawn
================
*/
void hhSecurityEye::Spawn() {
	m_pBase = NULL;
	m_pTrigger = NULL;
	m_bTriggerOnce = spawnArgs.GetBool("triggerOnce");
	m_fCachedScanFovCos = idMath::Cos( DEG2RAD(spawnArgs.GetFloat("fov")) );
	m_lengthBeam = spawnArgs.GetFloat("lengthBeam", "4096");
	m_offsetTrigger = spawnArgs.GetVector("offset_trigger");

	SetupRotationParms();
	SpawnTrigger();
	m_bUsePathScan = InitPathList();

	GetPhysics()->SetContents(CONTENTS_SHOOTABLE|CONTENTS_SHOOTABLEBYARROW);
	fl.takedamage = false; // CJR:  Set to be non-destructible at 3DR's request

	m_iPVSArea = gameLocal.pvs.GetPVSArea( GetOrigin() );

	if( spawnArgs.GetBool("enabled", "1") ) {
		StartScanning();
	} else {
		EnterIdleState();
	}

	BecomeActive( TH_TICKER );
}

void hhSecurityEye::Save(idSaveGame *savefile) const {
	savefile->WriteInt( state );
	savefile->WriteObject(m_pTrigger);
	savefile->WriteBool(m_bTriggerOnce);
	savefile->WriteFloat(m_fCachedScanFovCos);
	savefile->WriteInt(m_iPVSArea);
	savefile->WriteAngles(m_StartAngles);
	savefile->WriteAngles(m_MaxLookAngles);
	savefile->WriteAngles(m_MinLookAngles);
	savefile->WriteBool(m_bPitchDirection);
	savefile->WriteBool(m_bYawDirection);
	savefile->WriteStringList(m_PathScanNodes);
	savefile->WriteInt(m_iPathScanNodeIndex);
	savefile->WriteBool(m_bUsePathScan);
	savefile->WriteFloat(m_fPathScanRate);
	savefile->WriteObject(m_pBase);
	m_Target.Save(savefile);

	savefile->WriteFloat( currentYaw.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( currentYaw.GetDuration() );
	savefile->WriteFloat( currentYaw.GetStartValue() );
	savefile->WriteFloat( currentYaw.GetEndValue() );

	savefile->WriteFloat( currentPitch.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( currentPitch.GetDuration() );
	savefile->WriteFloat( currentPitch.GetStartValue() );
	savefile->WriteFloat( currentPitch.GetEndValue() );

	savefile->WriteAngles(m_RotationRate);
	savefile->WriteFloat(m_lengthBeam);
	savefile->WriteVec3(m_offsetTrigger);

	savefile->WriteAngles(m_LookAngles);
}

void hhSecurityEye::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadInt( reinterpret_cast<int &> ( state ) );
	savefile->ReadObject(reinterpret_cast<idClass *&> (m_pTrigger));
	savefile->ReadBool(m_bTriggerOnce);
	savefile->ReadFloat(m_fCachedScanFovCos);
	savefile->ReadInt(m_iPVSArea);
	savefile->ReadAngles(m_StartAngles);
	savefile->ReadAngles(m_MaxLookAngles);
	savefile->ReadAngles(m_MinLookAngles);
	savefile->ReadBool(m_bPitchDirection);
	savefile->ReadBool(m_bYawDirection);
	savefile->ReadStringList(m_PathScanNodes);
	savefile->ReadInt(m_iPathScanNodeIndex);
	savefile->ReadBool(m_bUsePathScan);
	savefile->ReadFloat(m_fPathScanRate);
	savefile->ReadObject(reinterpret_cast<idClass *&> (m_pBase));
	m_Target.Restore(savefile);

	savefile->ReadFloat( set );	// idInterpolate<float>
	currentYaw.SetStartTime( set );
	savefile->ReadFloat( set );
	currentYaw.SetDuration( set );
	savefile->ReadFloat( set );
	currentYaw.SetStartValue( set );
	savefile->ReadFloat( set );
	currentYaw.SetEndValue( set );

	savefile->ReadFloat( set );	// idInterpolate<float>
	currentPitch.SetStartTime( set );
	savefile->ReadFloat( set );
	currentPitch.SetDuration( set );
	savefile->ReadFloat( set );
	currentPitch.SetStartValue( set );
	savefile->ReadFloat( set );
	currentPitch.SetEndValue( set );

	savefile->ReadAngles(m_RotationRate);
	savefile->ReadFloat(m_lengthBeam);
	savefile->ReadVec3(m_offsetTrigger);

	savefile->ReadAngles(m_LookAngles);
}

/*
================
hhSecurityEye::~hhSecurityEye
================
*/
hhSecurityEye::~hhSecurityEye() {
	SAFE_REMOVE( m_pTrigger );

	//Used in case you use the remove command from the console
	StopSound( SND_CHANNEL_ANY );
}

/*
=====================
hhSecurityEye::DormantBegin
=====================
*/
void hhSecurityEye::DormantBegin() {
	idEntity::DormantBegin();

	if( state == StatePathScanning || state == StateAreaScanning ) {
		DisableTrigger();
	}
}

/*
=====================
hhSecurityEye::DormantEnd
=====================
*/
void hhSecurityEye::DormantEnd() {
	idEntity::DormantEnd();

	if( state == StatePathScanning || state == StateAreaScanning ) {
		EnableTrigger();
	}
}

/*
=====================
hhSecurityEye::SetupRotationParms
=====================
*/
void hhSecurityEye::SetupRotationParms() {
	m_bPitchDirection = 1;
	m_bYawDirection = 1;
	m_StartAngles.Set( spawnArgs.GetFloat("startPitch"), spawnArgs.GetFloat("startYaw"), 0.0f );
	m_LookAngles = m_StartAngles;

	m_MaxLookAngles.Set( spawnArgs.GetFloat("maxPitch"), spawnArgs.GetFloat("maxYaw"), 0.0f );
	m_MinLookAngles.Set( spawnArgs.GetFloat("minPitch"), spawnArgs.GetFloat("minYaw"), 0.0f );
	m_RotationRate.Set ( spawnArgs.GetFloat("pitchRate"), spawnArgs.GetFloat("yawRate"), 0.0f );

	currentPitch.Init(gameLocal.time, 0, m_StartAngles.pitch, m_StartAngles.pitch);
	currentYaw.Init(gameLocal.time, 0, m_StartAngles.yaw, m_StartAngles.yaw);
}

// Turn to angles using standard rates, angles should be in -180..180 local space
void hhSecurityEye::TurnTo(idAngles &ang) {
	float curYaw, curPitch, delta;
	int duration;

	if (m_RotationRate.yaw) {
		curYaw = currentYaw.GetCurrentValue(gameLocal.time);
		delta = idMath::Fabs(curYaw - ang.yaw);
		duration = SEC2MS(delta / m_RotationRate.yaw);
		currentYaw.Init(gameLocal.time, duration, curYaw, ang.yaw);
	}

	if (m_RotationRate.pitch) {
		curPitch = currentPitch.GetCurrentValue(gameLocal.time);
		delta = idMath::Fabs(curYaw - ang.yaw);
		duration = SEC2MS(delta / m_RotationRate.pitch);
		currentPitch.Init(gameLocal.time, duration, curPitch, ang.pitch);
	}
}

void hhSecurityEye::LookAtLinear(idVec3 &pos) {
	float curYaw, curPitch, delta;
	int durationYaw, durationPitch, duration;
	idVec3 localDir;
	idVec3 dir = pos - GetOrigin();
	dir.Normalize();
	GetBaseAxis().ProjectVector(dir, localDir);

	idAngles localAngles;
	localAngles.roll = 0.0f;
	localAngles.yaw = idMath::AngleNormalize180( localDir.ToYaw() );
	localAngles.pitch = -idMath::AngleNormalize180( localDir.ToPitch() );

	curYaw = currentYaw.GetCurrentValue(gameLocal.time);
	delta = idMath::Fabs(curYaw - localAngles.yaw);
	durationYaw = SEC2MS(delta / m_fPathScanRate);

	curPitch = currentPitch.GetCurrentValue(gameLocal.time);
	delta = idMath::Fabs(curYaw - localAngles.yaw);
	durationPitch = SEC2MS(delta / m_fPathScanRate);

	// Both converge together at the same time
	duration = max(durationYaw, durationPitch);
	currentYaw.Init(gameLocal.time, duration, curYaw, localAngles.yaw);
	currentPitch.Init(gameLocal.time, duration, curPitch, localAngles.pitch);
}

// Returns base rotation, all the local angles are relative to this
idMat3 hhSecurityEye::GetBaseAxis() {
	assert(m_pBase);
	return m_pBase->GetAxis();
}

// Set rotation of eye based on local angles
void hhSecurityEye::UpdateRotation() {
	idAngles localAngles;
	localAngles.Set(currentPitch.GetCurrentValue(gameLocal.time), currentYaw.GetCurrentValue(gameLocal.time), 0.0f);
	SetAxis( localAngles.ToMat3() );
}

void hhSecurityEye::SetBase(idEntity *ent) {
	m_pBase = ent;
}


/*
=====================
hhSecurityEye::GetRenderView
=====================
*/
renderView_t* hhSecurityEye::GetRenderView() {

	renderView = idEntity::GetRenderView();

	idVec3 ViewOffset = spawnArgs.GetVector("offset_view");
	idMat3 ViewAxis = GetAxis();
	idVec3 ViewOrigin = GetOrigin() + ViewOffset*ViewAxis;

	gameLocal.CalcFov( spawnArgs.GetFloat("fov", "90"), renderView->fov_x, renderView->fov_y );		//HUMANHEAD premerge bjk
	renderView->viewaxis = ViewAxis;
	renderView->vieworg = ViewOrigin;

	return renderView;
}

/*
================
hhSecurityEye::GetTriggerArgs
================
*/
idDict* hhSecurityEye::GetTriggerArgs( idDict* pArgs ) {
	assert( pArgs );

	const idKeyValue* pKeyValue = NULL;

	pArgs->Set( "call", spawnArgs.GetString("call") );
	pArgs->Set( "callRef", spawnArgs.GetString("callRef") );
	pArgs->Set( "callRefActivator", spawnArgs.GetString("callRefActivator") );

	pArgs->Set( "triggerBehavior", spawnArgs.GetString("triggerBehavior") );
	for( int iIndex = spawnArgs.GetNumKeyVals() - 1; iIndex >= 0 ; --iIndex ) {
		pKeyValue = spawnArgs.GetKeyVal( iIndex );
		if ( !pKeyValue->GetKey().Cmpn( "trigger_class", 13 ) ) {
			pArgs->Set( pKeyValue->GetKey(), pKeyValue->GetValue() );
		} 
		else if ( !pKeyValue->GetKey().Cmpn( "target", 6 ) ) {
			pArgs->Set( pKeyValue->GetKey(), pKeyValue->GetValue() );
		}
	}

	//Tripwire args
	pArgs->SetFloat( "lengthBeam", m_lengthBeam );
	pArgs->SetBool( "rigidBeamLength", 1 );
	pArgs->Set( "beam", spawnArgs.GetString("beam") );
	//Tripwire args end

	return pArgs;
}

/*
================
hhSecurityEye::SpawnTrigger
================
*/
void hhSecurityEye::SpawnTrigger() {

	idVec3 origin = GetPhysics()->GetOrigin();
	idMat3 axis = GetPhysics()->GetAxis();

	idDict Args;
	Args.SetVector( "origin", origin + m_offsetTrigger*axis );
	Args.SetMatrix( "rotation", axis );
	m_pTrigger = static_cast<hhTriggerTripwire*>( gameLocal.SpawnObject(spawnArgs.GetString("def_trigger"), GetTriggerArgs( &Args )) );
	HH_ASSERT( m_pTrigger );

	m_pTrigger->SetOwner( this );
	m_pTrigger->Bind( this, true );
}

/*
============
hhSecurityEye::InitPathList
============
*/
bool hhSecurityEye::InitPathList() {
	const idKeyValue* pKeyValue = NULL;
	
	m_PathScanNodes.Clear();
	for( int iIndex = spawnArgs.GetNumKeyVals() - 1; iIndex >= 0; --iIndex ) {
		pKeyValue = spawnArgs.GetKeyVal( iIndex );
		if ( !pKeyValue->GetKey().Cmpn( "pathScanNode", 12 ) && pKeyValue->GetValue().Length() ) {
			m_PathScanNodes.Append( pKeyValue->GetValue() );
		}
	}

	m_iPathScanNodeIndex = 0;
	m_fPathScanRate = spawnArgs.GetFloat("pathScanRate");

	return ( m_PathScanNodes.Num() && spawnArgs.GetBool( "usePathScan" ) );
}

/*
================
hhSecurityEye::Ticker
================
*/
void hhSecurityEye::Ticker() {
	switch( state ) {
		case StateIdle:
			if (currentYaw.IsDone(gameLocal.time) && currentPitch.IsDone(gameLocal.time)) {
				BecomeInactive( TH_TICKER );
			}
			break;
		case StateTracking:
			break;

		case StateAreaScanning:
			if (currentPitch.IsDone(gameLocal.time)) {
				m_bPitchDirection ^= 1;
				if( !m_bPitchDirection ) {
					m_LookAngles.pitch = m_MinLookAngles.pitch;
				} else {
					m_LookAngles.pitch = m_MaxLookAngles.pitch;
				}
				TurnTo( m_LookAngles );
			}

			if (currentYaw.IsDone(gameLocal.time)) {
				m_bYawDirection ^= 1;
				if( !m_bYawDirection ) {
					m_LookAngles.yaw = m_MinLookAngles.yaw;
				} else {
					m_LookAngles.yaw = m_MaxLookAngles.yaw;
				}
				TurnTo( m_LookAngles );
			}
			break;

		case StatePathScanning:
			idEntity* pEntity = NULL;

			if (currentYaw.IsDone(gameLocal.time) && currentPitch.IsDone(gameLocal.time)) {
				pEntity = VerifyPathScanNode( m_iPathScanNodeIndex );
				if( pEntity ) {
					idVec3 origin = pEntity->GetPhysics()->GetOrigin();
					LookAtLinear( origin );

					m_iPathScanNodeIndex = (m_iPathScanNodeIndex + 1) % m_PathScanNodes.Num();
				}
			}	
			break;
	}

	UpdateRotation();
}

/*
============
hhSecurityEye::DetermineEntityPosition
============
*/
idVec3 hhSecurityEye::DetermineEntityPosition( idEntity* pEntity ) {
	if( pEntity && pEntity->IsType( idActor::Type ) ) {
		return static_cast<idActor*>(pEntity)->GetEyePosition();
	}

	return pEntity->GetPhysics()->GetOrigin();
}

/*
================
hhSecurityEye::CanSeeTarget
================
*/
bool hhSecurityEye::CanSeeTarget( idEntity* pEntity ) {
	float fDist;
	trace_t TraceInfo;
	idVec3 Dir;
	pvsHandle_t PVSHandle;
	idVec3 EyePos;
	idMat3 EyeAxis;
	idVec3 TraceEnd;

	PVSHandle = gameLocal.pvs.SetupCurrentPVS( m_iPVSArea );
	
	if ( !pEntity || ( pEntity->fl.notarget ) ) {
		goto CANT_SEE;
	}

	// if there is no way we can see this player
	if( !gameLocal.pvs.InCurrentPVS( PVSHandle, pEntity->GetPVSAreas(), pEntity->GetNumPVSAreas() ) ) {
		goto CANT_SEE;
	}

	TraceEnd = (pEntity->IsType(idActor::Type)) ? static_cast<idActor*>(pEntity)->GetEyePosition() : pEntity->GetPhysics()->GetOrigin();
	Dir = TraceEnd - GetPhysics()->GetOrigin();
	fDist = Dir.Normalize();

	if( fDist > m_lengthBeam ) {
		goto CANT_SEE;
	}

	EyeAxis = GetAxis();
	EyePos = GetOrigin() + m_offsetTrigger * EyeAxis;

	if( Dir * EyeAxis[0] < m_fCachedScanFovCos ) {
		goto CANT_SEE;
	}

	gameLocal.clip.TracePoint( TraceInfo, EyePos, TraceEnd, MASK_VISIBILITY, this );
	if( TraceInfo.fraction == 1.0f || TraceInfo.c.entityNum == pEntity->entityNumber ) {
		gameLocal.pvs.FreeCurrentPVS( PVSHandle );
		return true;
	}

CANT_SEE:
	gameLocal.pvs.FreeCurrentPVS( PVSHandle );

	return false;
}

/*
=====================
hhSecurityEye::EnableTrigger
=====================
*/
void hhSecurityEye::EnableTrigger() {
	if( m_pTrigger ) {
		m_pTrigger->ProcessEvent( &EV_Enable );
	}
}

/*
=====================
hhSecurityEye::DisableTrigger
=====================
*/
void hhSecurityEye::DisableTrigger() {
	if( m_pTrigger ) {
		m_pTrigger->ProcessEvent( &EV_Disable );
	}
}

/*
============
hhSecurityEye::Killed
============
*/
void hhSecurityEye::Killed( idEntity *pInflictor, idEntity *pAttacker, int iDamage, const idVec3 &Dir, int iLocation ) {
	if( state != StateDead ) {
		//Only trigger targets when active
		if( state != StateIdle ) {
			ActivateTargets( pAttacker );
		}

		EnterDeadState();
	}
}

/*
============
hhSecurityEye::StartScanning
============
*/
void hhSecurityEye::StartScanning() {
	if( !m_bUsePathScan ) {
		EnterAreaScanningState();
	} else {
		EnterPathScanningState();
	}
}

/*
============
hhSecurityEye::VerifyLookTarget
============
*/
idEntity* hhSecurityEye::VerifyPathScanNode( int& iIndex ) {
	return gameLocal.FindEntity( m_PathScanNodes[iIndex].c_str() );
}

/*
============
hhSecurityEye::Event_Enable
============
*/
void hhSecurityEye::Event_Enable() {
	switch( state ) {
		case StateIdle:
			StartScanning();
			break;
	}
}

/*
============
hhSecurityEye::Event_Disable
============
*/
void hhSecurityEye::Event_Disable() {
	switch( state ) {
		case StateTracking:
		case StatePathScanning:
		case StateAreaScanning:
			EnterIdleState();
			break;
	}
}

/*
============
hhSecurityEye::Event_Notify
============
*/
void hhSecurityEye::Event_Notify( idEntity* pActivator ) {
	//Not sure of the behavior wanted if m_bTriggerOnce is true
	if( pActivator && pActivator->IsType(idActor::Type) ) {
		const idActor* pActor = static_cast<const idActor*>( pActivator );
	}

	switch( state ) {
		case StateAreaScanning:
		case StatePathScanning:
			HH_ASSERT( pActivator );
			m_Target = pActivator;
			EnterTrackingState();
			break;
		case StateTracking:
			break;
	}
}

/*
============
hhSecurityEye::Event_Activate
============
*/
void hhSecurityEye::Event_Activate( idEntity* pActivator ) {
	switch( state ) {
		case StateIdle:
			StartScanning();
			break;
		case StatePathScanning:
		case StateAreaScanning:
		case StateTracking:
			m_Target = NULL;
			EnterIdleState();
			break;
	}
}

/*
================
hhSecurityEye::EnterDeadState
================
*/
void hhSecurityEye::EnterDeadState() {
	state = StateDead;

	hhFxInfo fxInfo;

	DisableTrigger();

	StopSound( SND_CHANNEL_ANY );

	fxInfo.SetNormal( GetAxis()[0] );
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_detonate", m_pBase->GetOrigin(), GetAxis(), &fxInfo );

	idVec3 Position = spawnArgs.GetVector("offset_debris");
	Position = GetPhysics()->GetOrigin() + Position * GetPhysics()->GetAxis();
	idVec3 Forward = GetAxis()[0];
	idVec3 Velocity = Forward*10;
	hhUtils::SpawnDebrisMass(spawnArgs.GetString("def_debrisspawner"), Position, &Forward, &Velocity, 1);

	// Cause the base to flicker when the ball is destroyed
	if( m_pBase ) {
		m_pBase->SetShaderParm( SHADERPARM_MODE, 1 ); // Activate the flicker effect
		m_pBase->SetShaderParm( SHADERPARM_TIMEOFFSET, -MS2SEC( gameLocal.time ));	
	}

	Hide();
	GetPhysics()->DisableClip();
	int length = 0;
	StartSound("snd_explode", SND_CHANNEL_ANY, 0, false, &length);
	PostEventMS( &EV_Remove, length );

	BecomeActive( TH_TICKER );
}

/*
================
hhSecurityEye::EnterIdleState
================
*/
void hhSecurityEye::EnterIdleState() {
	state = StateIdle;

	DisableTrigger();

	TurnTo( m_StartAngles );

	StopSound( SND_CHANNEL_ANY );
}

/*
================
hhSecurityEye::EnterAreaScanningState
================
*/
void hhSecurityEye::EnterAreaScanningState() {
	state = StateAreaScanning;

	m_LookAngles = m_StartAngles;
	TurnTo( m_StartAngles );

	EnableTrigger();

	StartSound("snd_move", SND_CHANNEL_ANY);

	BecomeActive( TH_TICKER );
}

/*
================
hhSecurityEye::EnterPathScanningState
================
*/
void hhSecurityEye::EnterPathScanningState() {
	state = StatePathScanning;

	HH_ASSERT( m_PathScanNodes.Num() );

	EnableTrigger();

	StartSound("snd_move", SND_CHANNEL_ANY);

	BecomeActive( TH_TICKER );
}

/*
================
hhSecurityEye::EnterTrackingState
================
*/
void hhSecurityEye::EnterTrackingState() {
	state = StateTracking;

	DisableTrigger();
	StopSound( SND_CHANNEL_ANY );

	BecomeActive( TH_TICKER );
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build