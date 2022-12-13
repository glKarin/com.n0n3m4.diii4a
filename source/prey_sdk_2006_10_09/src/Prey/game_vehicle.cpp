#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//==========================================================================
//
//	hhVehicleThruster
//
//==========================================================================
CLASS_DECLARATION( idEntity, hhVehicleThruster )
	EVENT( EV_Broadcast_AssignFx,		hhVehicleThruster::Event_AssignFxSmoke )
END_CLASS

hhVehicleThruster::hhVehicleThruster() {
	fxSmoke = NULL;
}

void hhVehicleThruster::Spawn() {
	owner = NULL;
	GetPhysics()->SetContents(0);
	soundDistance = spawnArgs.GetFloat("soundDistance");
	bSomeThrusterActive = false;
	bSoundMaster = spawnArgs.GetBool("soundmaster");
	localVelocity = localOffset = localDirection = vec3_origin;
	fl.networkSync = true;
}

void hhVehicleThruster::Save(idSaveGame *savefile) const {
	owner.Save(savefile);
	fxSmoke.Save(savefile);
	savefile->WriteVec3(localOffset);
	savefile->WriteVec3(localDirection);
	savefile->WriteFloat(soundDistance);
	savefile->WriteBool(bSomeThrusterActive);
	savefile->WriteBool(bSoundMaster);
	savefile->WriteVec3(localVelocity);
}

void hhVehicleThruster::Restore( idRestoreGame *savefile ) {
	owner.Restore(savefile);
	fxSmoke.Restore(savefile);
	savefile->ReadVec3(localOffset);
	savefile->ReadVec3(localDirection);
	savefile->ReadFloat(soundDistance);
	savefile->ReadBool(bSomeThrusterActive);
	savefile->ReadBool(bSoundMaster);
	savefile->ReadVec3(localVelocity);
}

void hhVehicleThruster::WriteToSnapshot( idBitMsgDelta &msg ) const {
	WriteBindToSnapshot(msg);
	GetPhysics()->WriteToSnapshot(msg);

	msg.WriteBits(owner.GetSpawnId(), 32);
	msg.WriteBits(fxSmoke.GetSpawnId(), 32);

	msg.WriteFloat(localOffset.x);
	msg.WriteFloat(localOffset.y);
	msg.WriteFloat(localOffset.z);

	msg.WriteFloat(localDirection.x);
	msg.WriteFloat(localDirection.y);
	msg.WriteFloat(localDirection.z);

	msg.WriteFloat(soundDistance);

	msg.WriteBits(bSomeThrusterActive, 1);
	msg.WriteBits(bSoundMaster, 1);

	msg.WriteFloat(localVelocity.x);
	msg.WriteFloat(localVelocity.y);
	msg.WriteFloat(localVelocity.z);

	msg.WriteBits(IsHidden(), 1);
}

void hhVehicleThruster::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	ReadBindFromSnapshot(msg);
	GetPhysics()->ReadFromSnapshot(msg);

	owner.SetSpawnId(msg.ReadBits(32));
	fxSmoke.SetSpawnId(msg.ReadBits(32));

	localOffset.x = msg.ReadFloat();
	localOffset.y = msg.ReadFloat();
	localOffset.z = msg.ReadFloat();

	localDirection.x = msg.ReadFloat();
	localDirection.y = msg.ReadFloat();
	localDirection.z = msg.ReadFloat();

	soundDistance = msg.ReadFloat();

	bSomeThrusterActive = !!msg.ReadBits(1);
	bSoundMaster = !!msg.ReadBits(1);

	localVelocity.x = msg.ReadFloat();
	localVelocity.y = msg.ReadFloat();
	localVelocity.z = msg.ReadFloat();

	bool hidden = !!msg.ReadBits(1);
	if (hidden != IsHidden()) {
		if (hidden) {
			Hide();
		} else {
			Show();
		}
	}

	if (fxSmoke.IsValid() && fxSmoke.GetEntity() && fxSmoke->IsType(idEntityFx::Type)) {
		if (fxSmoke->IsHidden() != IsHidden()) {
			fxSmoke->BecomeActive( TH_THINK );
			if (IsHidden()) {
				fxSmoke->Nozzle(false);
				fxSmoke->fl.hidden = true;
			} else {
				fxSmoke->Nozzle(true);
				fxSmoke->fl.hidden = false;
			}
		}
	}
}

void hhVehicleThruster::ClientPredictionThink( void ) {
	Think();
}

void hhVehicleThruster::SetSmoker(bool bSmoker, idVec3 &offset, idVec3 &dir) {
	localOffset = offset;
	localDirection = dir;
	if ( bSmoker && (!fxSmoke.IsValid() || !fxSmoke.GetEntity()) ) {
		hhFxInfo fxInfo;
		const char *smokeName;
		smokeName = spawnArgs.GetString( "fx_smoke" );
		if (smokeName && *smokeName) {
			fxInfo.SetStart(false);
			fxInfo.RemoveWhenDone(false);
			fxInfo.SetEntity( this );
			idVec3 loc = owner->GetOrigin() + localOffset * owner->GetAxis();
			idMat3 axis = (localDirection * owner->GetAxis()).ToMat3();
			BroadcastFxInfo( smokeName, loc, axis, &fxInfo, &EV_Broadcast_AssignFx, false );
		}
	}
	else if (!bSmoker && fxSmoke.IsValid() && fxSmoke.GetEntity()) {
		fxSmoke->PostEventMS(&EV_Remove, 0);
		fxSmoke = NULL;
	}
}

void hhVehicleThruster::SetThruster(bool on) {

	if (fxSmoke.IsValid() && fxSmoke.GetEntity()) {
		fxSmoke->Nozzle( on );
	}

	on ? Show() : Hide();
}

void hhVehicleThruster::SetDying(bool bDying) {
	if (bSoundMaster) {
		SetSmoker(true, localOffset, localDirection);
	}
	else {
		SetSmoker(bDying, localOffset, localDirection);
	}
}

void hhVehicleThruster::Update( const idVec3 &vel ) {
	if (bSoundMaster) {
		localVelocity = vel;
		bool on = localVelocity != vec3_origin;
		if (on && !bSomeThrusterActive) {
			// Just became active
			StartSound("snd_thrust", SND_CHANNEL_THRUSTERS);
			bSomeThrusterActive = true;
		}
		else if (!on) {
			// Just became inactive
			StopSound(SND_CHANNEL_THRUSTERS);
			bSomeThrusterActive = false;
		}
	}

}

bool hhVehicleThruster::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	idVec3 toSound;
	idVec3 toPilot;
	if (owner.IsValid()) {
		axis.Identity();
		toSound = -localVelocity;	// Position thrust sounds in the opposite direction as thrust velocity
		toSound.Normalize();
		toPilot = (owner->GetOrigin() - GetOrigin()) + (owner->spawnArgs.GetVector("offset_pilot") + idVec3(0,0,1)*owner->spawnArgs.GetFloat("pilot_eyeHeight"))*owner->GetAxis();
		origin = toPilot + (toSound * owner->GetAxis() * soundDistance);
		return true;
	}
	return false;

}

void hhVehicleThruster::Event_AssignFxSmoke( hhEntityFx* fx ) {
	fxSmoke = fx;
}

//==========================================================================
//
//	hhPilotVehicleInterface
//
//==========================================================================
CLASS_DECLARATION( idClass, hhPilotVehicleInterface )
END_CLASS

hhPilotVehicleInterface::hhPilotVehicleInterface() {
	UnderScriptControl( false );
}

//rww - added to handle when a player is destroyed before a vehicle (only in mp i guess)
hhPilotVehicleInterface::~hhPilotVehicleInterface() {
	if (pilot.IsValid() && pilot.GetEntity()) {
		pilot->SetVehicleInterface(NULL);
	}
	if (vehicle.IsValid() && vehicle.GetEntity()) {
		vehicle->SetPilotInterface(NULL);
	}
}

void hhPilotVehicleInterface::Save(idSaveGame *savefile) const {
		pilot.Save(savefile);
		vehicle.Save(savefile);
		savefile->WriteBool(underScriptControl);
}

void hhPilotVehicleInterface::Restore( idRestoreGame *savefile ) {
		pilot.Restore(savefile);
		vehicle.Restore(savefile);
		savefile->ReadBool(underScriptControl);
}

void hhPilotVehicleInterface::RetrievePilotInput( usercmd_t& cmds, idAngles& viewAngles ) {
	if( pilot.IsValid() ) {
		pilot->GetPilotInput( cmds, viewAngles );
	}
}

void hhPilotVehicleInterface::TakeControl( hhVehicle* theVehicle, idActor* thePilot ) {
	vehicle = theVehicle;
	pilot = thePilot;

	if( theVehicle ) {
		theVehicle->AcceptPilot( this );
	}
}

void hhPilotVehicleInterface::ReleaseControl() {
	if (vehicle.IsValid()) {
		vehicle->ReleaseControl();
	}
	vehicle = NULL;
	pilot = NULL;
}

idVec3 hhPilotVehicleInterface::DeterminePilotOrigin() const {
	assert( vehicle.IsValid() );

	return vehicle->DeterminePilotOrigin();
}

idMat3 hhPilotVehicleInterface::DeterminePilotAxis() const {
	assert( vehicle.IsValid() );

	return vehicle->DeterminePilotAxis();
}

bool hhPilotVehicleInterface::ControllingVehicle() const {
	return vehicle.IsValid() && vehicle->IsVehicle();
}

hhVehicle* hhPilotVehicleInterface::GetVehicle() const {
	return vehicle.GetEntity();
}

idActor* hhPilotVehicleInterface::GetPilot() const {
	return pilot.GetEntity();
}

bool hhPilotVehicleInterface::InvalidVehicleImpulse( int impulse ) {
	switch( impulse ) {
		case IMPULSE_0:
		case IMPULSE_1:
		case IMPULSE_2:
		case IMPULSE_3:
		case IMPULSE_4:
		case IMPULSE_5:
		case IMPULSE_6:
		case IMPULSE_7:
		case IMPULSE_8:
		case IMPULSE_9:
		case IMPULSE_10:
		case IMPULSE_11:
		case IMPULSE_12:
		case IMPULSE_13:
		case IMPULSE_14:
		case IMPULSE_15:
		case IMPULSE_16:
		case IMPULSE_19:
		case IMPULSE_25:
		case IMPULSE_40:
		case IMPULSE_54:
			return true;
	}	

	return false;
}

//==========================================================================
//
//	hhAIVehicleInterface
//
//==========================================================================
CLASS_DECLARATION( hhPilotVehicleInterface, hhAIVehicleInterface )
END_CLASS

hhAIVehicleInterface::hhAIVehicleInterface(void) {
	stateFiring = false;
	stateAltFiring = false;
}

void hhAIVehicleInterface::Save(idSaveGame *savefile) const {
	savefile->WriteUsercmd(bufferedCmds);
	savefile->WriteAngles(bufferedViewAngles);
	savefile->WriteBool(stateFiring);
	savefile->WriteBool(stateAltFiring);
	savefile->WriteFloat(stateOrientSpeed);
	savefile->WriteFloat(stateThrustSpeed);
	savefile->WriteVec3(stateOrientDestination);
	savefile->WriteVec3(stateThrustDestination);

}

void hhAIVehicleInterface::Restore( idRestoreGame *savefile ) {
	savefile->ReadUsercmd(bufferedCmds);
	savefile->ReadAngles(bufferedViewAngles);
	savefile->ReadBool(stateFiring);
	savefile->ReadBool(stateAltFiring);
	savefile->ReadFloat(stateOrientSpeed);
	savefile->ReadFloat(stateThrustSpeed);
	savefile->ReadVec3(stateOrientDestination);
	savefile->ReadVec3(stateThrustDestination);
}

void hhAIVehicleInterface::TakeControl( hhVehicle* vehicle, idActor* pilot ) {
	hhPilotVehicleInterface::TakeControl( vehicle, pilot );

	ClearBufferedCmds();
	bufferedViewAngles = (pilot) ? pilot->GetAxis().ToAngles() : ang_zero;
}

void hhAIVehicleInterface::Fire(bool on) {
	stateFiring = on;
}

void hhAIVehicleInterface::AltFire(bool on) {
	stateAltFiring = on;
}

void hhAIVehicleInterface::OrientTowards( const idVec3 &loc, float speed ) {
	stateOrientDestination = loc;
	stateOrientSpeed = idMath::ClampFloat( 0.0f, 1.0f, speed );
}

void hhAIVehicleInterface::ThrustTowards( const idVec3 &loc, float speed ) {
	stateThrustDestination = loc;
	stateThrustSpeed = idMath::ClampFloat( 0.0f, 1.0f, speed );
}

void hhAIVehicleInterface::RetrievePilotInput( usercmd_t& cmds, idAngles& viewAngles ) {

	if( !ControllingVehicle() ) {
		return;
	}

	// First grab any buffered commands
	cmds = bufferedCmds;
	viewAngles = bufferedViewAngles;

	idVec3 eyePos = GetPilot()->GetEyePosition();

	// Now, Override the buffered input with any script/ai requests
	// Based on current script requests, construct a command packet
	cmds.buttons |= (stateFiring ? BUTTON_ATTACK : 0);
	cmds.buttons |= (stateAltFiring ? BUTTON_ATTACK_ALT : 0);

	// Handle thrusting
	if ( stateThrustSpeed > 0.0f ) {
		idVec3 targetDir = stateThrustDestination - eyePos;

		// Use proportional control system to ease into destination
		const float proportionalGain = 0.7f;
		float error = idMath::ClampFloat(0.0f, 1.0f, targetDir.Normalize() / 512.0f);		// error only taken into consideration when within 512 units of destination
		if (error < 1.0f) {
			error *= proportionalGain;
		}

		targetDir *= vehicle->GetAxis().Inverse();

		cmds.forwardmove = targetDir.x * 127.0f * error * stateThrustSpeed;
		cmds.rightmove = -targetDir.y * 127.0f * error * stateThrustSpeed;
		cmds.upmove = targetDir.z * 127.0f * error * stateThrustSpeed;
	}

	// Handle orienting: Using direction vectors to interpolate our orientation to our moving target orientation
	viewAngles.pitch = -idMath::AngleNormalize180( vehicle->GetAxis()[0].ToPitch() );
	viewAngles.yaw = vehicle->GetAxis()[0].ToYaw();
	viewAngles.roll = 0.0f;

	if ( stateOrientSpeed > 0.0f ) {
		idVec3 localDir;
		idAngles idealViewAngles( ang_zero );
		idVec3 targetDir = stateOrientDestination - eyePos;
		targetDir.Normalize();
		float degrees = stateOrientSpeed * 360.0f * MS2SEC(gameLocal.msec);

		vehicle->GetPhysics()->GetAxis().ProjectVector( targetDir, localDir );
		idealViewAngles.yaw = localDir.ToYaw();
		idealViewAngles.pitch = -idMath::AngleNormalize180( localDir.ToPitch() );

		idAngles deltaViewAngles = (idealViewAngles - viewAngles).Normalize180();
		deltaViewAngles.Clamp( idAngles(-degrees, -degrees, 0.0f), idAngles(degrees, degrees, 0.0f) );
		if( !deltaViewAngles.Compare(ang_zero, VECTOR_EPSILON) ) {
			viewAngles += deltaViewAngles;
			bufferedViewAngles = viewAngles;	// Save for next time
		}
	}
}

void hhAIVehicleInterface::BufferPilotCmds( const usercmd_t* cmds, const idAngles* viewAngles ) {
	if( cmds ) {
		bufferedCmds = *cmds;
	}

	if( viewAngles ) {
		bufferedViewAngles = *viewAngles;
	}
}

void hhAIVehicleInterface::ClearBufferedCmds() {
	memset( &bufferedCmds, 0, sizeof(usercmd_t) );
	stateFiring = false;
	stateAltFiring = false;
	stateOrientSpeed = 0.0f;
	stateThrustSpeed = 0.0f;
}

bool hhAIVehicleInterface::IsVehicleDocked() const {
	return vehicle.IsValid() && vehicle->IsDocked(); 
}

//==========================================================================
//
//	hhPlayerVehicleInterface
//
//==========================================================================
CLASS_DECLARATION( hhPilotVehicleInterface, hhPlayerVehicleInterface )
END_CLASS

hhPlayerVehicleInterface::hhPlayerVehicleInterface() {
	hud = NULL;
	uniqueHud = true;
	translationAlpha.Init(gameLocal.time, 0, 0.0f, 0.0f);
}

hhPlayerVehicleInterface::~hhPlayerVehicleInterface() {
}

void hhPlayerVehicleInterface::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject(weaponHandState);
	controlHand.Save(savefile);
	savefile->WriteBool(uniqueHud);
	savefile->WriteUserInterface(hud, uniqueHud);

	savefile->WriteFloat( translationAlpha.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( translationAlpha.GetDuration() );
	savefile->WriteFloat( translationAlpha.GetStartValue() );
	savefile->WriteFloat( translationAlpha.GetEndValue() );
}

void hhPlayerVehicleInterface::Restore( idRestoreGame *savefile ) {
	float set;

	savefile->ReadStaticObject(weaponHandState);
	controlHand.Restore(savefile);
	savefile->ReadBool(uniqueHud);
	savefile->ReadUserInterface(hud);

	savefile->ReadFloat( set );			// idInterpolate<float>
	translationAlpha.SetStartTime( set );
	savefile->ReadFloat( set );
	translationAlpha.SetDuration( set );
	savefile->ReadFloat( set );
	translationAlpha.SetStartValue(set);
	savefile->ReadFloat( set );
	translationAlpha.SetEndValue( set );
}

void hhPlayerVehicleInterface::UpdateControlHand( const usercmd_t& cmds ) {
	if( controlHand.IsValid() ) {
		controlHand->UpdateControlDirection( idVec3(Sign(cmds.forwardmove), Sign(cmds.rightmove), Sign(cmds.upmove)) );
	}
}

void hhPlayerVehicleInterface::CreateControlHand( hhPlayer* pilot, const char* handName ) {
	//RemoveHand();
	
	if( !handName || !handName[0] ) {
		return;
	}

	weaponHandState.SetPlayer( pilot );
	weaponHandState.SetWeaponTransition( 1 );
	weaponHandState.Archive( NULL, 0, handName, 1 );

	if ( pilot->hand.IsValid() && pilot->hand->IsType( hhControlHand::Type ) ) {
		controlHand = static_cast<hhControlHand*>( pilot->hand.GetEntity() );	
	}
	else {
		controlHand = NULL;
	}

	//controlHand = static_cast<hhControlHand*>( hhControlHand::AddHand(pilot, handName) );
}

void hhPlayerVehicleInterface::RemoveHand() {
	if( controlHand.IsValid() ) {
		//controlHand->RemoveHand();
		controlHand = NULL;
	}
	weaponHandState.RestoreFromArchive();
}

void hhPlayerVehicleInterface::RetrievePilotInput( usercmd_t& cmds, idAngles& viewAngles ) {
	if( pilot.IsValid() ) {
		hhPilotVehicleInterface::RetrievePilotInput( cmds, viewAngles );
		UpdateControlHand( cmds );
	}
}

void hhPlayerVehicleInterface::TakeControl( hhVehicle* vehicle, idActor* pilot ) {
	hhPilotVehicleInterface::TakeControl( vehicle, pilot );

	if( vehicle ) {
		idStr hudName = vehicle->spawnArgs.GetString( "gui_hud" );
		if( hudName.Length() ) {
			uniqueHud = vehicle->spawnArgs.GetBool("uniqueguihud", "1");
			hud = uiManager->FindGui( hudName.c_str(), true, uniqueHud, !uniqueHud );

			bool isPilotLocal = false;
			idPlayer *localPl = gameLocal.GetLocalPlayer();
			if (localPl) {
				if (localPl->entityNumber == pilot->entityNumber || (localPl->spectating && localPl->spectator == pilot->entityNumber)) {
					isPilotLocal = true;
				}
			}
			if (isPilotLocal) {
				translationAlpha.Init( gameLocal.time, 0, 0.0f, 0.0f );
				hud->Activate( true, gameLocal.GetTime() );
				pilot->fl.clientEvents = true; //rww - hack
				pilot->PostEventMS( &EV_StartHudTranslation, 1000 );
				pilot->fl.clientEvents = false; //rww - hack
			}
		}

		if (!gameLocal.isClient) {
			CreateControlHand( static_cast<hhPlayer*>(pilot), vehicle->spawnArgs.GetString("def_hand") );
		}
	}
}

void hhPlayerVehicleInterface::ReleaseControl() {
	if( vehicle.IsValid() ) {
		hud = NULL;
		uniqueHud = true;
		if (!gameLocal.isClient) {
			RemoveHand();
		} else {
			if (controlHand.IsValid() && controlHand.GetEntity()) {
				controlHand->Hide();
			}
		}
	}

	hhPilotVehicleInterface::ReleaseControl();
}

void hhPlayerVehicleInterface::DrawHUD( idUserInterface* _hud ) {
	if( vehicle.IsValid() && vehicle->IsVehicle() && _hud ) {
		float alpha = translationAlpha.GetCurrentValue(gameLocal.GetTime());
		_hud->SetStateFloat( "translationAlpha", alpha );
		_hud->SetStateBool( "translationProject", false );

		vehicle->DrawHUD( _hud );
	}
}

void hhPlayerVehicleInterface::StartHUDTranslation() {
	translationAlpha.Init( gameLocal.time, 1000, 0.0f, 1.0f );
}


//==========================================================================
//
//	hhVehicle
//
//==========================================================================

const idEventDef EV_VehicleExplode("<vehicleexplode>", "efff");
const idEventDef EV_Vehicle_FireCannon( "fireCannon" );

const idEventDef EV_VehicleGetIn("getIn", "e");			// Script Commands
const idEventDef EV_VehicleGetOut("getOut");
const idEventDef EV_VehicleFire("fire", "d");
const idEventDef EV_VehicleAltFire("altFire", "d");
const idEventDef EV_VehicleOrientTowards("orientTowards", "vf");
const idEventDef EV_VehicleStopOrientingTOwards("stopOrientingTowards");
const idEventDef EV_VehicleThrustTowards("thrustTowards", "vf");
const idEventDef EV_VehicleStopThrustingTOwards("stopThrustingTowards");
const idEventDef EV_VehicleReleaseControl("releaseControl");
const idEventDef EV_Vehicle_EjectPilot("ejectPilot");

ABSTRACT_DECLARATION( hhRenderEntity, hhVehicle )
	EVENT( EV_Vehicle_FireCannon,		hhVehicle::Event_FireCannon )
	EVENT( EV_VehicleExplode,			hhVehicle::Event_Explode )
	EVENT( EV_VehicleGetIn,				hhVehicle::Event_GetIn )
	EVENT( EV_VehicleGetOut,			hhVehicle::Event_GetOut )
	EVENT( EV_VehicleFire,				hhVehicle::Event_Fire )
	EVENT( EV_VehicleAltFire,			hhVehicle::Event_AltFire )
	EVENT( EV_VehicleOrientTowards,		hhVehicle::Event_OrientTowards )
	EVENT( EV_VehicleThrustTowards,		hhVehicle::Event_ThrustTowards )
	EVENT( EV_VehicleReleaseControl,	hhVehicle::Event_ReleaseScriptControl )
	EVENT( EV_Vehicle_EjectPilot,		hhVehicle::Event_EjectPilot )
	EVENT( EV_ResetGravity,				hhVehicle::Event_ResetGravity )
END_CLASS

hhVehicle::~hhVehicle() {
	if (GetPilotInterface()) {
		idActor *actor = GetPilotInterface()->GetPilot();
		if (actor && actor->IsType(idActor::Type)) { //if it still has a valid pilot, eject them.
			EjectPilot();
		}
	}
}

void hhVehicle::Spawn() {
	memset( &oldCmds, 0, sizeof(usercmd_t) );
	thrusterCost = spawnArgs.GetInt( "thrusterCost" );
	currentPower = spawnArgs.GetInt( "maxPower" );
	thrustFactor = spawnArgs.GetFloat( "thrustFactor" );
	dockBoostFactor = spawnArgs.GetFloat( "dockBoostFactor" );
	if ( gameLocal.isMultiplayer ) {
		noDamage = false;
	} else {
		noDamage = spawnArgs.GetBool( "noDamage", "0" );
	}

	bDamageSelfOnCollision = spawnArgs.GetBool( "damageSelfOnCollision" );
	bDamageOtherOnCollision = spawnArgs.GetBool( "damageOtherOnCollision" );

	lastAttackTime = 0;
	fl.neverDormant = false;
	fl.refreshReactions = true;
	fireController = NULL;
	pilotInterface = NULL;
	bHeadlightOn = false;
	validThrustTime = gameLocal.time;

	bDisallowAttackUntilRelease = false;
	bDisallowAltAttackUntilRelease = false;

	InitializeAttackFuncs();	

	//rww - i see no reason why the dock should be set upon spawning the vehicle.
	//also would cause issues in mp where only one shuttle should be docked at a time.
	//startDock is set by the dock who created us
	/*
	hhDock *dock = (hhDock *)gameLocal.FindEntity(spawnArgs.GetString("startDock"));
	SetDock( dock );
	if (dock && dock->IsType(hhShuttleDock::Type)) {
		hhShuttleDock *shDock = static_cast<hhShuttleDock *>(dock);
		if (shDock->GetDockedShuttle()) { //if the dock already has a shuttle, do not dock this ship there.
			SetDock(NULL);
		}
	}
	*/

	InitPhysics();

	BecomeConsole();

	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );

	SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	SetPhysics( &physicsObj );

	if ( spawnArgs.GetInt( "nodrop" ) ) {
		physicsObj.PutToRest();
	}
	else {
		physicsObj.DropToFloor();
	}
}

void hhVehicle::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject(physicsObj);
	savefile->WriteMat3(modelAxis);

	if( fireController ) {
		savefile->WriteBool(true);
        savefile->WriteStaticObject(*fireController);
	} else {
		savefile->WriteBool(false);
	}

	savefile->WriteUsercmd(oldCmds);

	headlight.Save(savefile);
	domelight.Save(savefile);

	savefile->WriteBool(bHeadlightOn);
	savefile->WriteInt(currentPower);
	savefile->WriteFloat(thrustFactor);
	savefile->WriteFloat(thrustMin);
	savefile->WriteFloat(thrustMax);
	savefile->WriteFloat(thrustAccel);
	savefile->WriteFloat(thrustScale);
	savefile->WriteFloat(dockBoostFactor);

	dock.Save(savefile);
	lastAttacker.Save(savefile);

	savefile->WriteInt(lastAttackTime);
	savefile->WriteInt(vehicleClipMask);
	savefile->WriteInt(vehicleContents);
	savefile->WriteBool(bDamageSelfOnCollision);
	savefile->WriteBool(bDamageOtherOnCollision);
	savefile->WriteBool(bDisallowAttackUntilRelease);
	savefile->WriteBool(bDisallowAltAttackUntilRelease);
	savefile->WriteInt(thrusterCost);
	savefile->WriteInt(validThrustTime);

	savefile->WriteEventDef(attackFunc);
	savefile->WriteEventDef(finishedAttackingFunc);
	savefile->WriteEventDef(altAttackFunc);
	savefile->WriteEventDef(finishedAltAttackingFunc);

	savefile->WriteBool(noDamage);
}

void hhVehicle::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject(physicsObj);
	RestorePhysics(&physicsObj);

	savefile->ReadMat3(modelAxis);

	bool test;
	savefile->ReadBool( test );
	if (test) {
		fireController = CreateFireController();
		savefile->ReadStaticObject(*fireController);
	} else {
		SAFE_DELETE_PTR(fireController);
	}

	savefile->ReadUsercmd(oldCmds);

	headlight.Restore(savefile);
	domelight.Restore(savefile);

	savefile->ReadBool(bHeadlightOn);
	savefile->ReadInt(currentPower);
	savefile->ReadFloat(thrustFactor);
	savefile->ReadFloat(thrustMin);
	savefile->ReadFloat(thrustMax);
	savefile->ReadFloat(thrustAccel);
	savefile->ReadFloat(thrustScale);
	savefile->ReadFloat(dockBoostFactor);

	dock.Restore(savefile);
	lastAttacker.Restore(savefile);

	savefile->ReadInt(lastAttackTime);
	savefile->ReadInt(vehicleClipMask);
	savefile->ReadInt(vehicleContents);
	savefile->ReadBool(bDamageSelfOnCollision);
	savefile->ReadBool(bDamageOtherOnCollision);
	savefile->ReadBool(bDisallowAttackUntilRelease);
	savefile->ReadBool(bDisallowAltAttackUntilRelease);
	savefile->ReadInt(thrusterCost);
	savefile->ReadInt(validThrustTime);

	savefile->ReadEventDef(attackFunc);
	savefile->ReadEventDef(finishedAttackingFunc);
	savefile->ReadEventDef(altAttackFunc);
	savefile->ReadEventDef(finishedAltAttackingFunc);

	savefile->ReadBool(noDamage);
}

const idEventDef* hhVehicle::GetAttackFunc( const char* funcName ) {
	function_t* function = NULL;

	if( !funcName || !funcName[0] ) {
		return NULL;
	}

	idTypeDef *funcType = gameLocal.program.FindType(funcName);
	if (!funcType) {
		return NULL;
	}

	function = gameLocal.program.FindFunction( funcName, funcType );
	if( !function || !function->eventdef ) {
		return NULL;
	}

	HH_ASSERT( RespondsTo(*function->eventdef) );

	return function->eventdef;
}

void hhVehicle::InitializeAttackFuncs() {
	idStr funcName;
	attackFunc = finishedAttackingFunc = altAttackFunc = finishedAltAttackingFunc = NULL;
	
	funcName = spawnArgs.GetString("attackFunc");
	if (funcName.Length()) {
		attackFunc = GetAttackFunc( funcName.c_str() );
		finishedAttackingFunc = GetAttackFunc( (funcName + "Done").c_str() );
	}
	funcName = spawnArgs.GetString("altAttackFunc");
	if (funcName.Length()) {
		altAttackFunc = GetAttackFunc( funcName.c_str() );
		finishedAltAttackingFunc = GetAttackFunc( (funcName + "Done").c_str() );
	}
}

void hhVehicle::Think() {
	usercmd_t	pilotCmds;
	idAngles	pilotViewAngles;

	if( IsVehicle() && GetPilotInterface() ) {
		GetPilotInterface()->RetrievePilotInput( pilotCmds, pilotViewAngles );
		ProcessPilotInput( &pilotCmds, &pilotViewAngles );
		GetPhysics()->SetAxis( mat3_identity );
	}

	hhRenderEntity::Think();
}

void hhVehicle::Present() {
	if (fireController) {
		fireController->UpdateMuzzleFlash();
	}

	hhRenderEntity::Present();
}

void hhVehicle::AcceptPilot( hhPilotVehicleInterface* pilotInterface ) {
	//assure that the vehicle is showing upon accepting a new pilot
	Show();

	this->pilotInterface = pilotInterface;

	fl.neverDormant = true;
	fl.takedamage = true;
	StartSound( "snd_activation", SND_CHANNEL_ANY );
	StartSound( "snd_inuse", SND_CHANNEL_MISC1 );

	CreateDomeLight();
	CreateHeadLight();
	BecomeActive( TH_TICKER );

	const idDict* infoDict = NULL;
	if ( GetPilot() && GetPilot()->IsType(idAI::Type) ) {
		infoDict = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_fireInfoAI"), false );
	} else {
		infoDict = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_fireInfo"), false );
	}
	fireController = CreateFireController();
	HH_ASSERT( fireController );
	fireController->Init( infoDict, this, GetPilot() );

	// supress model in player views, but allow it in mirrors and remote views
	if (GetPilot()->IsType(hhPlayer::Type)) {
		GetRenderEntity()->suppressSurfaceInViewID = GetPilot()->entityNumber + 1;
	}

	BecomeVehicle();
}

void hhVehicle::RestorePilot( hhPilotVehicleInterface* pilotInterface) { 
	const idDict *infoDict = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_fireInfo"), false );
	assert( infoDict );
	fireController->SetWeaponDict( infoDict );
	this->pilotInterface = pilotInterface; 
}


void hhVehicle::EjectPilot() {
	assert( IsVehicle() );

	SAFE_DELETE_PTR( fireController );

	fl.takedamage = false;
	fl.neverDormant = false;
	StopSound( SND_CHANNEL_MISC1 );
	StartSound( "snd_deactivation", SND_CHANNEL_ANY );

	FreeDomeLight();
	FreeHeadLight();
	BecomeInactive( TH_TICKER );


	if (GetPilotInterface()->GetPilot()) { //rww - make sure pilot is valid
		GetPilotInterface()->GetPilot()->ExitVehicle( this );
		pilotInterface = NULL;
	}

	RemoveVehicle();
}

idVec3 hhVehicle::DeterminePilotOrigin() const {
	return GetOrigin() + spawnArgs.GetVector("offset_pilot") * GetAxis();
}

idMat3 hhVehicle::DeterminePilotAxis() const {
	return GetAxis();
}

void hhVehicle::BecomeVehicle() {
	SetVehiclePhysics();

	SetVehicleModel();
}

bool hhVehicle::CanBecomeVehicle(idActor *pilot) {
	// Check to see if shuttle will fit
	idEntity *touch[ MAX_GENTITIES ];
	idBounds bounds, localBounds;

	idVec3 location = GetOrigin();
	localBounds[0] = spawnArgs.GetVector("mins");
	localBounds[1] = spawnArgs.GetVector("maxs");

//	idBox box(localBounds, location, GetAxis());
//	gameRenderWorld->DebugBox(colorRed, box, 8000);

	idTraceModel trm;
	trm.SetupBox( localBounds );
	idClipModel *clipModel = new idClipModel( trm );
	clipModel->Link(gameLocal.clip, this, 254, location, GetAxis());

	int pilotClipMask = pilot ? pilot->GetPhysics()->GetClipMask() : 0;
	pilotClipMask &= (~CONTENTS_HUNTERCLIP);	// Vehicles don't collide with hunterclip so they don't get hung up on dock borders
	int num = hhUtils::EntitiesTouchingClipmodel( clipModel, touch, MAX_GENTITIES, CLIPMASK_VEHICLE | pilotClipMask );
	bool blocked = false;
	for (int i=0; i<num; i++) {
		if (touch[i] && touch[i] != this && touch[i] != pilot && touch[i]->GetBindMaster() != pilot && (touch[i]->GetPhysics()->GetContents() & CLIPMASK_VEHICLE)) {
			blocked = true;
			//gameLocal.Printf("Blocked by %s\n", touch[i]->GetName());
			break;
		}
	}

	clipModel->Unlink();
	delete clipModel;

	return !blocked;
}

bool hhVehicle::IsVehicle() const {
	return fireController != NULL && GetPilotInterface() && GetPilotInterface()->GetPilot();
}

void hhVehicle::BecomeConsole() {
	SetConsolePhysics();

	if( IsDocked() ) {
		//Feels like a hack.  Shouldn't the dock do this or at least be in SetConsolePhysics
		SetAxis( dock->GetAxis() );
		GetPhysics()->SetAxis( dock->GetAxis() );
		SetOrigin( dock->GetOrigin() + dock->spawnArgs.GetVector("offset_console") * dock->GetAxis() );

		physicsObj.PutToRest();
	}

	SetConsoleModel();
}

bool hhVehicle::IsConsole() const {
	return !IsVehicle();
}

idVec3 hhVehicle::GetPortalPoint() {
	idVec3 offset = spawnArgs.GetVector("offset_pilot") + idVec3(0,0,1)*spawnArgs.GetFloat("pilot_eyeHeight");
	return GetOrigin() + offset * GetAxis();
}

void hhVehicle::Portalled(idEntity *portal) {
	idActor *pilot = GetPilot();
	if (pilot) {
		// Update the view angles of the player so next time we get input, it won't reset our angles
		if (pilot->IsType(hhPlayer::Type)) {
			hhPlayer* player = static_cast<hhPlayer*>( pilot );
			idVec3 origin = DeterminePilotOrigin();
			idVec3 viewDir = GetAxis()[0];

			// Don't know if all this is necessary, might be overkill, definitely need some of it though for 'shuttle through portal'
			player->Unbind();
			player->SetOrientation( origin, mat3_identity, viewDir, viewDir.ToAngles() );
			player->SetUntransformedViewAxis( mat3_identity );
			player->Bind(this, true);
		}
	}
}

void hhVehicle::ResetGravity() {
	if( IsVehicle() ) {
		physicsObj.SetGravity( spawnArgs.GetVector("activeGravity") );
	}
	else {
		physicsObj.SetGravity( spawnArgs.GetVector("inactiveGravity") );
	}
}

void hhVehicle::SetAxis( const idMat3& axis ) {
	modelAxis = axis;

	UpdateVisuals();
}

void hhVehicle::InitPhysics() {	
	physicsObj.SetSelf( this );

	physicsObj.SetFriction(
		spawnArgs.GetFloat("friction_linear"),
		spawnArgs.GetFloat("friction_angular"),
		spawnArgs.GetFloat("friction_contact") );
}

void hhVehicle::SetConsolePhysics() {
	idTraceModel trm;
	const char *clipModelName = spawnArgs.GetString( "clipmodel" );
	assert( clipModelName[0] );

	if ( !collisionModelManager->TrmFromModel( clipModelName, trm ) ) {
		gameLocal.Error( "hhVehicle '%s' at (%s): cannot load collision model %s\n",
			name.c_str(), GetPhysics()->GetOrigin().ToString(0), clipModelName );
		return;
	}

	physicsObj.SetClipModel( new idClipModel(trm), spawnArgs.GetFloat("density") );
	physicsObj.DisableImpact();
	physicsObj.SetBouncyness( 0.0f );
	physicsObj.SetContents( CONTENTS_VEHICLE );
	physicsObj.SetClipMask( CLIPMASK_VEHICLE | CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP );
	physicsObj.SetLinearVelocity( vec3_zero );
	physicsObj.SetAngularVelocity( vec3_zero );

	//Used as a cache for when noclipping
	vehicleClipMask = physicsObj.GetClipMask();
	vehicleContents = physicsObj.GetContents();

	ResetGravity();
}

void hhVehicle::SetVehiclePhysics() {
	idBounds bounds( spawnArgs.GetVector("mins"), spawnArgs.GetVector("maxs") );

	physicsObj.SetClipModel( new idClipModel(idTraceModel(bounds)), spawnArgs.GetFloat("density") );
	physicsObj.SetAxis( mat3_identity );
	physicsObj.EnableImpact();
	physicsObj.SetBouncyness( spawnArgs.GetFloat("bouncyness") );
	physicsObj.SetContents( CONTENTS_VEHICLE );

	SetThrustBooster( spawnArgs.GetFloat("thrustMin"), spawnArgs.GetFloat("thrustMax"), spawnArgs.GetFloat("thrustAccel") );

	int pilotClipMask = (GetPilotInterface() && GetPilotInterface()->GetPilot()) ? GetPilotInterface()->GetPilot()->GetPhysics()->GetClipMask() : 0;
	pilotClipMask &= (~CONTENTS_HUNTERCLIP);	// Vehicles don't collide with hunterclip so they don't get hung up on dock borders
	physicsObj.SetClipMask( CLIPMASK_VEHICLE | pilotClipMask );

	//Used as a cache for when noclipping
	vehicleClipMask = physicsObj.GetClipMask();
	vehicleContents = physicsObj.GetContents();

	ResetGravity();
}

void hhVehicle::SetConsoleModel() {
	SetModel( spawnArgs.GetString("model") );
}

void hhVehicle::SetVehicleModel() {
	SetModel( spawnArgs.GetString("model_active") );
}

float hhVehicle::CmdScale( const usercmd_t* cmd ) const {
	float scale = 0.0f;

	if( !cmd ) {
		return scale;
	}

	idVec3 abscmd(abs(cmd->forwardmove), abs(cmd->rightmove), abs(cmd->upmove));

	// Bound the cmd vector to a sphere whose radius is equal to the longest cmd axis
	int desiredLength = max(max(abscmd[0], abscmd[1]), abscmd[2]);
	if ( desiredLength != 0.0f ) {
		float currentLength = abscmd.Length();
		scale = desiredLength / currentLength;
	}

	return scale;
}

void hhVehicle::SetThrustBooster(float minBooster, float maxBooster, float accelBooster) {
	thrustMin = minBooster;
	thrustMax = maxBooster;
	thrustAccel = accelBooster;

	thrustScale = 0.1f;
}

void hhVehicle::ProcessPilotInput( const usercmd_t* cmds, const idAngles* viewAngles ) {
	idVec3 impulse;

	if( viewAngles ) {
		SetAxis( viewAngles->ToMat3() );
	}

	if( !cmds ) {
		return;
	}

	ProcessButtons( *cmds );

	//Check vehicle here because we could have exited the vehicle in ProcessButtons
	if( !IsVehicle() ) {
		return;
	}

	ProcessImpulses( *cmds );

	if ( gameLocal.time >= validThrustTime ) {
		impulse = GetAxis()[0] * cmds->forwardmove * thrustFactor;
		impulse -= GetAxis()[1] * cmds->rightmove * thrustFactor;
		impulse += GetAxis()[2] * cmds->upmove * thrustFactor;
	}
	else {
		impulse = vec3_origin;
	}

	// Apply booster to allow key taps to be very low thrust
	thrustScale = idMath::ClampFloat( thrustMin, thrustMax, thrustScale * thrustAccel );
	if( impulse.LengthSqr() >= VECTOR_EPSILON ) {
		FireThrusters( impulse * CmdScale(cmds) * thrustScale * (60.0f * USERCMD_ONE_OVER_HZ) );
	} else {
		thrustScale = thrustMin;
	}

	memcpy( &oldCmds, cmds, sizeof(usercmd_t) );
}

void hhVehicle::ProcessButtons( const usercmd_t& cmds ) {

	//This is needed in case we enter a dock while firing or using the tractor beam
	if( (cmds.buttons & BUTTON_ATTACK) && !(oldCmds.buttons & BUTTON_ATTACK) ) {
		if (IsDocked() && dock->AllowsExit() && spawnArgs.GetBool("fireToExit")) {
			if (!gameLocal.isClient) {
				EjectPilot();
			}
			return;
		}
	} else if( (cmds.buttons & BUTTON_ATTACK_ALT) && !(oldCmds.buttons & BUTTON_ATTACK_ALT) ) {
		if (IsDocked() && dock->AllowsExit() && spawnArgs.GetBool("altFireToExit")) {
			if (!gameLocal.isClient) {
				EjectPilot();
			}
			return;
		}
	}

	if (bDisallowAttackUntilRelease) {
		if (!(cmds.buttons & BUTTON_ATTACK)) {
			bDisallowAttackUntilRelease = false;
		}
		if( oldCmds.buttons & BUTTON_ATTACK ) {
			oldCmds.buttons &= ~BUTTON_ATTACK;
			if( finishedAttackingFunc ) {
				ProcessEvent( finishedAttackingFunc );
			}
		}
	}
	else {
		if( (cmds.buttons & BUTTON_ATTACK) ) {
			if( attackFunc ) {
				ProcessEvent( attackFunc );
			}
		} else if( oldCmds.buttons & BUTTON_ATTACK ) {
			if( finishedAttackingFunc ) {
				ProcessEvent( finishedAttackingFunc );
			}
		}
	}

	if (bDisallowAltAttackUntilRelease) {
		if (!(cmds.buttons & BUTTON_ATTACK_ALT)) {
			bDisallowAltAttackUntilRelease = false;
		}
		if ( oldCmds.buttons & BUTTON_ATTACK_ALT ) {
			oldCmds.buttons &= ~BUTTON_ATTACK_ALT;
			if( finishedAltAttackingFunc ) {
				ProcessEvent( finishedAltAttackingFunc );
			}
		}
	}
	else {
		if( cmds.buttons & BUTTON_ATTACK_ALT ) {
			if( altAttackFunc ) {
				ProcessEvent( altAttackFunc );
			}
		} else if( oldCmds.buttons & BUTTON_ATTACK_ALT ) {
			if( finishedAltAttackingFunc ) {
				ProcessEvent( finishedAltAttackingFunc );
			}
		}
	}
}

void hhVehicle::DoPlayerImpulse(int impulse) {
	if ( gameLocal.isClient && GetPilot() && GetPilot()->IsType(hhPlayer::Type) ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		hhPlayer *pl = static_cast<hhPlayer *>(GetPilot());

		assert( pl->entityNumber == gameLocal.localClientNum );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( impulse, 6 );
		pl->ClientSendEvent( hhPlayer::EVENT_IMPULSE, &msg );
	}

	switch (impulse) {
		case IMPULSE_16:
			if (!gameLocal.isClient) {
				Headlight( !bHeadlightOn );
			}
			break;
	}
}

void hhVehicle::ProcessImpulses( const usercmd_t& cmds ) {
	if( (cmds.flags & UCF_IMPULSE_SEQUENCE) == (oldCmds.flags & UCF_IMPULSE_SEQUENCE) ) {
		return;
	}

	DoPlayerImpulse(cmds.impulse); //rww - seperated into its own function because of networked impulse events
}

void hhVehicle::ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse ) {
	hhRenderEntity::ApplyImpulse( ent, id, point, impulse );
}

void hhVehicle::ApplyImpulse( const idVec3& impulse ) {
	ApplyImpulse( gameLocal.world, 0, GetOrigin() + physicsObj.GetCenterOfMass(), impulse * physicsObj.GetMass() );
}

void hhVehicle::UpdateModel( void ) {
	idVec3 origin;
	idMat3 axis;

	if ( GetPhysicsToVisualTransform(origin, axis) ) {
		//HUMANHEAD: aob
		GetRenderEntity()->axis = axis * GetAxis();
		GetRenderEntity()->origin = GetOrigin() + origin * GetRenderEntity()->axis;
		//HUMANHEAD END
	} else {
		//HUMANHEAD: aob
		GetRenderEntity()->axis = GetAxis();
		GetRenderEntity()->origin = GetOrigin();
		//HUMANHEAD END
	}

	// set to invalid number to force an update the next time the PVS areas are retrieved
	ClearPVSAreas();

	// ensure that we call Present this frame
	BecomeActive( TH_UPDATEVISUALS );
}

void hhVehicle::CreateDomeLight() {
	if (spawnArgs.GetBool("domelight")) {
		// This method is more oriented for flexibility
		idVec3 light_offset = spawnArgs.GetVector("offset_domelight");
		const char *objName = spawnArgs.GetString("def_domelight");

		idDict args;
		idVec3 lightOrigin = GetPhysics()->GetOrigin() + light_offset * GetPhysics()->GetAxis();
		args.SetVector( "origin", lightOrigin );

		domelight = (idLight *)gameLocal.SpawnObject(objName, &args);
		domelight->Bind( this, true );
		domelight->SetLightParm(SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.time));
	}
}

void hhVehicle::FreeDomeLight() {
	SAFE_REMOVE( domelight );
}

void hhVehicle::CreateHeadLight() {
	if ( spawnArgs.GetBool("headlight") ) {
		// This method is more oriented for presenting our own lights
		// Depending on the speed cost of having seperate entities for lights, we may
		// want to present our own, like the weapons
		idStr light_shader = spawnArgs.GetString("mtr_headlight");
		idVec3 light_color = spawnArgs.GetVector("headlight_color");
		idVec3 light_offset = spawnArgs.GetVector("offset_headlight");
		idVec3 light_target = spawnArgs.GetVector("offset_headlighttarget");
		idVec3 light_frustum = spawnArgs.GetVector("headlight_frustum");

		idDict args;
		idVec3 lightOrigin = GetPhysics()->GetOrigin() + light_offset * GetPhysics()->GetAxis();
		light_target.Normalize();
		idMat3 lightAxis = (light_target * GetPhysics()->GetAxis()).hhToMat3();

		if ( light_shader.Length() ) {
			args.Set( "texture", light_shader );
		}
		args.SetVector( "origin", lightOrigin );
		args.Set ("angles", lightAxis.ToAngles().ToString());
		args.SetVector( "_color", light_color );
		args.SetVector( "light_target", lightAxis[0] * light_frustum.x );
		args.SetVector( "light_right", lightAxis[1] * light_frustum.y );
		args.SetVector( "light_up", lightAxis[2] * light_frustum.z );
		headlight = ( idLight * )gameLocal.SpawnEntityTypeClient( idLight::Type, &args );

		//rww - headlight is a pure local entity
		assert(headlight.IsValid() && headlight.GetEntity());
		headlight->fl.networkSync = false;
		headlight->fl.clientEvents = true;

		headlight->Bind(this, true);

		headlight->SetLightParm( 6, 0.0f );					// fade out
		headlight->SetLightParm( SHADERPARM_TIMEOFFSET, 0 );	// Initially faded out already
	}
}

void hhVehicle::FreeHeadLight() {
	SAFE_REMOVE( headlight );
}

void hhVehicle::Headlight( bool on ) {
	bHeadlightOn = on;
	float timeOffset = -MS2SEC(gameLocal.time);

	if( headlight.IsValid() ) {
		headlight->SetLightParm(SHADERPARM_TIMEOFFSET, timeOffset);
		SetShaderParm(7, timeOffset);

		if (bHeadlightOn) {
			headlight->SetLightParm(6, 1.0f);	// Fade light in
			SetShaderParm(6, 1.0f);				// Fade light cone in
		}
		else {
			headlight->SetLightParm(6, 0.0f);	// Fade out
			SetShaderParm(6, 0.0f);				// Fade out
		}
	}
}

void hhVehicle::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if (InGodMode() || InDialogMode()) {
		return;
	}

	if (gameLocal.isMultiplayer) { //rww - don't let shuttles damage themselves or their teammates
		if (attacker && attacker->IsType(hhPlayer::Type) && GetPilot() == attacker) {
			return;
		}

		hhPlayer *playerPilot = (GetPilot() && GetPilot()->IsType( hhPlayer::Type )) ? static_cast<hhPlayer*>(GetPilot()) : NULL;
		hhPlayer *player = (attacker && attacker->IsType( hhPlayer::Type )) ? static_cast<hhPlayer*>(attacker) : NULL;
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
		if ( (gameLocal.gameType == GAME_TDM
			&& playerPilot
			&& !gameLocal.serverInfo.GetBool( "si_teamDamage" )
			&& damageDef
			&& !damageDef->GetBool( "noTeam" )
			&& player
			&& player->team == playerPilot->team) ) {
			return;
		}
	} else {
		if ( noDamage ) {
			return;
		}
		if (attacker && attacker->IsType(idAI::Type) && GetPilot() == attacker) {
			return;
		}
	}

	if (GetPilotInterface() && GetPilotInterface()->GetPilot()) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );

		if (gameLocal.isMultiplayer && IsType(hhShuttle::Type)) { //rww - in mp, let's try flickering the tractor beam on and off when hit
			hhShuttle *shtlSelf = static_cast<hhShuttle *>(this);
			if (shtlSelf->noTractorTime <= gameLocal.time && shtlSelf->TractorIsActive()) { //don't let it stay off for a long time from constant attack
				float dmg = ((float)damageDef->GetInt("damage", "100"))*damageScale;
				int dmgTime = (int)(dmg*12);
				//cap it to something reasonable on both ends
				if (dmgTime < 100) {
					dmgTime = 100;
				}
				else if (dmgTime > 1200) {
					dmgTime = 1200;
				}
				shtlSelf->noTractorTime = gameLocal.time + dmgTime;
			}
		}

		// Let playerview know about the impact direction
		if ( !damageDef ) {
			gameLocal.Warning( "Unknown damageDef '%s'", damageDefName );
			return;
		}
		idVec3 damage_from, localDamageVector;
		damage_from = dir;
		damage_from.Normalize();
		if ( GetPilotInterface()->GetPilot()->IsType(hhPlayer::Type)) {
			// Pass this on so we can get directional damage
			hhPlayer *playerPilot = static_cast<hhPlayer*>(GetPilotInterface()->GetPilot());
			playerPilot->viewAxis.ProjectVector( damage_from, localDamageVector );
			playerPilot->playerView.DamageImpulse( localDamageVector, damageDef );

			// Track last attacker for use in displaying HUD hit indicator
			if (!gameLocal.isClient) {
				playerPilot->ReportAttack(attacker);
			}

			if (gameLocal.isMultiplayer) { //rww - in MP, we want to damage the player a little from shuttle damage
				float plDmgScale = damageScale*0.2f;
				bool pilotWasTakingDamage = playerPilot->fl.takedamage;
				playerPilot->fl.takedamage = true;
				playerPilot->Damage(inflictor, attacker, dir, damageDefName, plDmgScale, INVALID_JOINT);
				playerPilot->fl.takedamage = pilotWasTakingDamage;
				if (playerPilot->health == 1) { //if the player died (or almost, kind of a hack) as a result, blow me up too
					damageDefName = "damage_suicide";
				}
			}
		}
	}
	
	// Tell HUD to show impact direction
	if( IsVehicle() ) {//AI may want to know who hit them
		lastAttacker = attacker;
		lastAttackTime = gameLocal.time;
	}
	idEntity::Damage( inflictor, attacker, dir, damageDefName, 1.0f, location );
}

void hhVehicle::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if (gameLocal.isClient) {
		return;
	}
	if( gameLocal.isMultiplayer || health < spawnArgs.GetInt("gibhealth") ) { //rww - always explode in mp
		CancelEvents(&EV_VehicleExplode);
		PostEventMS(&EV_VehicleExplode, 0, attacker, dir.x, dir.y, dir.z);
	}
	else if( health + damage > 0 ) {	// Only do this the first death
		ConsumePower(currentPower);

		if( domelight.IsValid() ) {
			domelight->SetLightParm(7, 1.0f);
		}

		StopSound( SND_CHANNEL_DYING, true );
		StartSound( "snd_dying", SND_CHANNEL_DYING, 0, true );

		// control release during the damage pipe was causing a crash when damaged by splash damage
		// the clip model is invalidated, then radius damage tries to apply to it.
		// Should be okay to switch back if "killed" is made into an event
		CancelEvents( &EV_VehicleExplode );
		PostEventSec( &EV_VehicleExplode, spawnArgs.GetFloat("explodedelay"), attacker, dir.x, dir.y, dir.z );
	}
}

bool hhVehicle::Collide( const trace_t &collision, const idVec3 &velocity ) {
	static const float minCollisionVelocity = 40.0f;
	static const float maxCollisionVelocity = 650.0f;

	// Velocity in normal direction
	float len = velocity * -collision.c.normal;

	if ( len > minCollisionVelocity && collision.c.material && !(collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT) ) {
		if (StartSound( "snd_bounce", SND_CHANNEL_BODY3 )) {
			// Change volume only after we know the sound played
			float volume = hhUtils::CalculateSoundVolume( len, minCollisionVelocity, maxCollisionVelocity );
			HH_SetSoundVolume(volume, SND_CHANNEL_BODY3);
		}

		if (len > 200.0f) {
			// Damage other
			idEntity *other = gameLocal.GetTraceEntity(collision);
			if (other && bDamageOtherOnCollision) {
				idEntity *collideAttacker = this;

				if (GetPilotInterface() && GetPilotInterface()->GetPilot()) { //use pilot as attacker if we have one
					collideAttacker = GetPilotInterface()->GetPilot();
				}
				float damageScale = hhUtils::CalculateScale(len, 200, 600);
				damageScale = (damageScale * 4.0f) + 1.0f;							// damageScale in range [1..5]
				damageScale *= other->spawnArgs.GetFloat("vehicledamagescale");		// for fine tuning

				// NOTE: Assuming damage_vehicleCollision is set to 20!
				if (20 * damageScale >= other->health) {
					other->Damage(this, collideAttacker, vec3_origin, "damage_vehicleCollision_gib", damageScale, 0);
				}
				else {
					other->Damage(this, collideAttacker, vec3_origin, "damage_vehicleCollision", damageScale, 0);
				}
			}

			// Damage self
			if (bDamageSelfOnCollision) {
				Damage(other, other, collision.c.normal, "damage_vehicleCollision", 1.0f, 0);
			}
		}
	}

	return false;
}

bool hhVehicle::IsNoClipping() const {
	return GetPilotInterface() && GetPilotInterface()->GetPilot() && GetPilotInterface()->GetPilot()->IsType(hhPlayer::Type) && static_cast<hhPlayer*>(GetPilotInterface()->GetPilot())->noclip;
}

bool hhVehicle::InGodMode() const {
	return GetPilotInterface() && GetPilotInterface()->GetPilot() && GetPilotInterface()->GetPilot()->IsType(hhPlayer::Type) && static_cast<hhPlayer*>(GetPilotInterface()->GetPilot())->godmode;
}

bool hhVehicle::InDialogMode() const {
	return GetPilotInterface() && GetPilotInterface()->GetPilot() && GetPilotInterface()->GetPilot()->IsType(hhPlayer::Type) && static_cast<hhPlayer*>(GetPilotInterface()->GetPilot())->InDialogDamageMode();
}

void hhVehicle::GiveHealth( int amount ) {
	health = idMath::ClampInt( 0, spawnHealth, health + amount );
}

void hhVehicle::GivePower( int amount ) {
	currentPower = idMath::ClampInt( 0, spawnArgs.GetInt("maxPower"), currentPower + amount );
}

bool hhVehicle::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;
	if( !src->ReadToken(&token) || token == ";" ) {
		return false;
	}
	if( !token.Icmp("controlvehicle") ) {
		if (IsHidden()) { //rww - was possible to get back in a vehicle once it had been hidden for removal
			return true;
		}
		if( entityGui->IsType(idActor::Type) ) {
			static_cast<idActor*>( entityGui )->EnterVehicle( this );
		}
		return true;
	}
	return false;
}

void hhVehicle::DrawHUD( idUserInterface* _hud ) {
	if( _hud ) {
		//rww - transfer necessary hud variables from the player hud to the vehicle hud
		if (GetPilotInterface() && GetPilotInterface()->GetPilot() && GetPilotInterface()->GetPilot()->IsType(hhPlayer::Type)) {
            hhPlayer *plPilot = static_cast<hhPlayer *>(GetPilotInterface()->GetPilot());
			if (plPilot->hud) {
				//aiming
				_hud->SetStateFloat("aim_R", plPilot->hud->GetStateFloat("aim_R", ""));
				_hud->SetStateFloat("aim_G", plPilot->hud->GetStateFloat("aim_G", ""));
				_hud->SetStateFloat("aim_B", plPilot->hud->GetStateFloat("aim_B", ""));
				_hud->SetStateString("aim_text", plPilot->hud->GetStateString("aim_text", ""));
				//set in UpdateHud:
				/*
				_hud->SetStateString("playername", plPilot->hud->GetStateString("playername", ""));
				_hud->SetStateFloat("team_R", plPilot->hud->GetStateFloat("team_R", ""));
				_hud->SetStateFloat("team_G", plPilot->hud->GetStateFloat("team_G", ""));
				_hud->SetStateFloat("team_B", plPilot->hud->GetStateFloat("team_B", ""));
				_hud->SetStateBool("ismultiplayer", plPilot->hud->GetStateBool("ismultiplayer", ""));
				*/

				//set in UpdateHud:
				/*
				//top 4
				for (int i = 0; i < 4; i++) {
					_hud->SetStateString( va( "player%i", i+1 ), plPilot->hud->GetStateString( va( "player%i", i+1 ), ""));
					_hud->SetStateString( va( "player%i_portrait", i+1 ), plPilot->hud->GetStateString(va( "player%i_portrait", i+1 ), ""));
					_hud->SetStateString( va( "player%i_score", i+1 ), plPilot->hud->GetStateString( va( "player%i_score", i+1 ), ""));
					_hud->SetStateString( va( "rank%i", i+1 ), plPilot->hud->GetStateString( va( "rank%i", i+1 ), ""));
				}
				_hud->SetStateString("rank_self", plPilot->hud->GetStateString("rank_self", ""));
				*/
			}
		}
		_hud->SetStateBool( "dying", health <= 0 );
		_hud->SetStateFloat( "healthfraction", ((float)health)/(float)spawnHealth );
		_hud->SetStateFloat( "powerfraction", (currentPower)/spawnArgs.GetFloat("maxPower") );
		idAngles angles = GetAxis().ToAngles();
		_hud->SetStateFloat( "pitch", angles.pitch );
		_hud->SetStateFloat( "yaw", angles.yaw );
		_hud->SetStateFloat( "roll", angles.roll );
		_hud->Redraw( gameLocal.realClientTime );
	}
}

void hhVehicle::PerformDeathAction(int deathAction, idActor *savedPilot, idEntity *attacker, idVec3 &dir) {
	switch(deathAction) {
		case 0:		// Drop pilot
			break;
		case 1:		// Kill pilot
			savedPilot->Damage(this, attacker, dir, spawnArgs.GetString("def_killpilotdamage"), 1.0f, 0);
			break;
	}
}

void hhVehicle::Explode( idEntity *attacker, idVec3 dir ) {
	if (gameLocal.isClient) {
		return;
	}
	if (!GetPilotInterface()) {
		return;
	}

	idEntityPtr<idActor> savedPilot = GetPilotInterface()->GetPilot();

	EjectPilot();

	if (savedPilot.IsValid()) {
		int deathAction = savedPilot->IsType(hhPlayer::Type) ? spawnArgs.GetInt("DeathActionPlayer") : spawnArgs.GetInt("DeathActionAI");
		PerformDeathAction(deathAction, savedPilot.GetEntity(), attacker, dir);
	}

	const char *deathExplosionDef = spawnArgs.GetString("def_deathexplosion", "");
	if (deathExplosionDef && deathExplosionDef[0]) {
		gameLocal.RadiusDamage(GetOrigin(), this, this, this, this, deathExplosionDef);
	}

	// Explode into gibs
	idVec3 vel = dir * 300;
	hhUtils::SpawnDebrisMass(spawnArgs.GetString("def_debrisspawner"),
		GetPhysics()->GetOrigin(), NULL, &vel, 1);
	StopSound(SND_CHANNEL_DYING, true);
	StartSound("snd_death", SND_CHANNEL_ANY);
}

bool hhVehicle::HasPower( int amount ) const {
	return currentPower >= amount;
}

bool hhVehicle::ConsumePower( int amount ) {
	if( InGodMode() ) {
		return true;
	}

	if (currentPower >= amount) {
		currentPower -= amount;
		return true;
	}

	currentPower = 0;
	return false;
}

void hhVehicle::RemoveVehicle() {
	//rww - stop sounds too
	StopSound(SND_CHANNEL_THRUSTERS, true);
	StopSound(SND_CHANNEL_MISC1, true);
	StopSound(SND_CHANNEL_DYING, true);

	Hide();
	GetPhysics()->SetContents( 0 );
	GetPhysics()->SetClipMask( 0 );
	PostEventMS( &EV_Remove, 3000 );			// Give anything targetting it a chance to retarget
}

void hhVehicle::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
	assert(currentPower < (1<<20));
	msg.WriteBits(currentPower, 20);
	assert(health < (1<<12));
	msg.WriteBits(health, 12);
	msg.WriteBits(bHeadlightOn, 1);

	idCQuat modelQuat = modelAxis.ToCQuat();
	msg.WriteFloat(modelQuat.x);
	msg.WriteFloat(modelQuat.y);
	msg.WriteFloat(modelQuat.z);

	msg.WriteBits(renderEntity.suppressSurfaceInViewID, GENTITYNUM_BITS);

	/*
	msg.WriteBits(currentPower, 32);
	msg.WriteFloat(thrustFactor);
	msg.WriteFloat(thrustMin);
	msg.WriteFloat(thrustMax);
	msg.WriteFloat(thrustAccel);
	msg.WriteFloat(thrustScale);
	msg.WriteFloat(dockBoostFactor);
	*/

	msg.WriteBits(IsNoClipping() ? 0 : vehicleClipMask, 32);
	msg.WriteBits(IsNoClipping() ? 0 : vehicleContents, 32);
	msg.WriteBits(IsHidden(), 1);

	msg.WriteBits(dock.GetSpawnId(), 32);
	msg.WriteBits(domelight.GetSpawnId(), 32);
	WriteBindToSnapshot( msg );

	if (fireController) { //fire controller can be null at this point.
		msg.WriteBits(fireController->barrelOffsets.GetCurrentIndex(), 8);
	}
	else {
		msg.WriteBits(0, 8);
	}
}

void hhVehicle::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
	currentPower = msg.ReadBits(20);
	health = msg.ReadBits(12);
	bool headlightOn = !!msg.ReadBits(1);
	if (headlightOn != bHeadlightOn) {
		Headlight(headlightOn);
	}

	idCQuat modelQuat;
	modelQuat.x = msg.ReadFloat();
	modelQuat.y = msg.ReadFloat();
	modelQuat.z = msg.ReadFloat();
	modelAxis = modelQuat.ToMat3();

	renderEntity.suppressSurfaceInViewID = msg.ReadBits(GENTITYNUM_BITS);

	/*
	currentPower = msg.ReadBits(32);
	thrustFactor = msg.ReadFloat();
	thrustMin = msg.ReadFloat();
	thrustMax = msg.ReadFloat();
	thrustAccel = msg.ReadFloat();
	thrustScale = msg.ReadFloat();
	dockBoostFactor = msg.ReadFloat();
	*/

	int newVehicleClipMask = msg.ReadBits(32);
	if (newVehicleClipMask != vehicleClipMask) {
		physicsObj.SetClipMask(newVehicleClipMask);
		vehicleClipMask = newVehicleClipMask;
	}
	int newVehicleContents = msg.ReadBits(32);
	if (newVehicleContents != vehicleContents) {
		physicsObj.SetContents(newVehicleContents);
		vehicleContents = newVehicleContents;
	}

	bool hidden = !!msg.ReadBits(1);
	if (hidden != IsHidden()) {
		if (hidden) {
			Hide();
		} else {
			Show();
		}
	}

	int spawnId;

	spawnId = msg.ReadBits(32); //rwwFIXME why is 0 check needed? something checking the container strangely?
	if (!spawnId) {
		dock = NULL;
	}
	else {
		dock.SetSpawnId(spawnId);
	}
	spawnId = msg.ReadBits(32);
	if (!spawnId) {
		domelight = NULL;
	}
	else {
		if (domelight.SetSpawnId(spawnId)) {
			domelight->Bind( this, true );
			domelight->SetLightParm(SHADERPARM_TIMEOFFSET, -MS2SEC(gameLocal.time));
		}
	}
	ReadBindFromSnapshot( msg );

	int barrelIndex = msg.ReadBits(8);
	if (fireController) {
		fireController->barrelOffsets.SetCurrentIndex(barrelIndex);
	}
}

void hhVehicle::ClientPredictionThink( void ) {
	Think();
}

bool hhVehicle::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch ( event ) {
		case EVENT_EJECT_PILOT:
			EjectPilot();
			return true;
		default:
			return hhRenderEntity::ClientReceiveEvent( event, time, msg );
	}
}

void hhVehicle::Event_Explode( idEntity *attacker, float dx, float dy, float dz ) {
	if (InDialogMode()) {
		// Death not allowed, repost
		CancelEvents(&EV_VehicleExplode);
		PostEventMS(&EV_VehicleExplode, 1000, attacker, dx, dy, dz);
		return;
	}

	idVec3 dir(dx, dy, dz);
	Explode(attacker, dir);
}

void hhVehicle::Event_ResetGravity() {
	ResetGravity();
}

void hhVehicle::Event_FireCannon() {
	if( fireController && fireController->LaunchProjectiles(vec3_zero) ) {
		StartSound( "snd_cannon", SND_CHANNEL_ANY );
		fireController->MuzzleFlash();
		if (GetPilot() && GetPilot()->IsType(hhPlayer::Type)) {
			// Only players get weapon fire feedback, so monster shuttles aren't pushed back into docks
			fireController->WeaponFeedback();
		}
	}
}

// Script control interfaces
void hhVehicle::Event_GetIn( idEntity *ent ) {
	if (ent->IsType(idActor::Type)) {
		static_cast<idActor*>( ent )->EnterVehicle( this );
		GetPilotInterface()->UnderScriptControl( true );
	}
}

void hhVehicle::Event_GetOut() {
	if (GetPilotInterface()) {
		GetPilotInterface()->UnderScriptControl( true );
		EjectPilot();
	}
}

void hhVehicle::Event_Fire( bool start ) {
	if( GetPilotInterface() ) {
		GetPilotInterface()->UnderScriptControl( true );
		GetPilotInterface()->Fire( start );
	}
}

void hhVehicle::Event_AltFire( bool start ) {
	if( GetPilotInterface() ) {
		GetPilotInterface()->UnderScriptControl( true );
		GetPilotInterface()->AltFire( start );
	}
}

void hhVehicle::Event_OrientTowards( idVec3 &point, float speed ) {
	if( GetPilotInterface() ) {
		GetPilotInterface()->UnderScriptControl( true );
		GetPilotInterface()->OrientTowards( point, speed );	// passed as percentage of max
	}
}

void hhVehicle::Event_StopOrientingTowards() {
	if( GetPilotInterface() ) {
		GetPilotInterface()->UnderScriptControl( true );
		GetPilotInterface()->OrientTowards( vec3_origin, 0.0f );
	}
}

void hhVehicle::Event_ThrustTowards( idVec3 &point, float speed ) {
	idVec3 pt = point;
	if( GetPilotInterface() ) {
		GetPilotInterface()->UnderScriptControl( true );
		GetPilotInterface()->ThrustTowards( pt, speed );	// passed as percentage of max
	}
}

void hhVehicle::Event_StopThrustingTowards() {
	if( GetPilotInterface() ) {
		GetPilotInterface()->UnderScriptControl( true );
		GetPilotInterface()->ThrustTowards( vec3_origin, 0.0f );
	}
}

void hhVehicle::Event_ReleaseScriptControl() {
	if( GetPilotInterface() ) {
		GetPilotInterface()->UnderScriptControl( false );
		GetPilotInterface()->ClearBufferedCmds();
	}
}

void hhVehicle::Event_EjectPilot() {
	EjectPilot();
}

