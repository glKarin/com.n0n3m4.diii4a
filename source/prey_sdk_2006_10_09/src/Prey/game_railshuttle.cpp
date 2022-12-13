#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//==========================================================================
//
//	hhRailShuttle
//
//==========================================================================

#define VCOCKPIT_PARM_POWER		4
#define VCOCKPIT_PARM_FIRE		5
#define VCOCKPIT_PARM_JUMP		6
#define VCOCKPIT_PARM_MOVE		7

const idEventDef EV_SetOrbitRadius("setOrbitRadius", "fd");
const idEventDef EV_RailShuttle_Jump("railShuttleJump");
const idEventDef EV_RailShuttle_FinalJump("railShuttleFinalJump");
const idEventDef EV_RailShuttle_IsJumping("railShuttleIsJumping", NULL, 'f');
const idEventDef EV_RailShuttle_Disengage("railShuttleDisengage");
const idEventDef EV_HideArcs("<hideArcs>");
const idEventDef EV_RestorePower("<restorePower>");
const idEventDef EV_GetCockpit( "getCockpit", "", 'e' ); //rdr

CLASS_DECLARATION(hhVehicle, hhRailShuttle)
	EVENT( EV_SetOrbitRadius,				hhRailShuttle::Event_SetOrbitRadius )
	EVENT( EV_PostSpawn,					hhRailShuttle::Event_PostSpawn )
	EVENT( EV_RailShuttle_Jump,				hhRailShuttle::Event_Jump )
	EVENT( EV_RailShuttle_FinalJump,		hhRailShuttle::Event_FinalJump )
	EVENT( EV_RailShuttle_IsJumping,		hhRailShuttle::Event_IsJumping )
	EVENT( EV_RailShuttle_Disengage,		hhRailShuttle::Event_Disengage )
	EVENT( EV_HideArcs,						hhRailShuttle::Event_HideArcs )
	EVENT( EV_RestorePower,					hhRailShuttle::Event_RestorePower )
	EVENT( EV_GetCockpit,					hhRailShuttle::Event_GetCockpit )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

void hhRailShuttle::Spawn() {
	fl.networkSync = true;

	turretPitchLimit = spawnArgs.GetFloat("turretPitchLimit");
	turretYawLimit = spawnArgs.GetFloat("turretYawLimit");

	defaultRadius = spawnArgs.GetFloat("sphereRadius", "1000");
	sphereRadius.Init(gameLocal.time, 0, defaultRadius, defaultRadius);

	sphereCenter = spawnArgs.GetVector("sphereCenter");
	sphereAngles = ang_zero;
	sphereVelocity = ang_zero;
	spherePitchMin = spawnArgs.GetFloat("spherePitchMin");
	spherePitchMax = spawnArgs.GetFloat("spherePitchMax");

	SetPhysics( NULL );		// No collision, no expensive shuttle physics
	GetPhysics()->SetContents(0);

	linearFriction = spawnArgs.GetFloat("friction_linear");
	terminalVelocity = spawnArgs.GetFloat("terminalVelocity");

	bJumping = false;
	bDisengaged = false;
	jumpStartTime = 0;
	jumpInitSpeed = spawnArgs.GetFloat("jumpInitSpeed");
	jumpAccel = spawnArgs.GetFloat("jumpAccel");
	jumpAccelTime = spawnArgs.GetFloat("jumpAccelTime");
	jumpReturnForce = spawnArgs.GetFloat("jumpReturnForce");
	jumpBounceCount = spawnArgs.GetInt("jumpBounceCount");
	jumpBounceFactor = spawnArgs.GetFloat("jumpBounceFactor");
	jumpThrustMovementScale = spawnArgs.GetFloat("jumpThrustMovementScale");
	jumpBounceMovementScale = spawnArgs.GetFloat("jumpBounceMovementScale");
	bWallMovementSoundPlaying = false;
	bAirMovementSoundPlaying = false;
	bBounced = false;

	localViewAngles = ang_zero;
	turret = NULL;
	canopy = NULL;
	leftArc = NULL;
	rightArc = NULL;

	//HUMANHEAD PCF mdl 04/29/06 - Added bUpdateViewAngles to prevent view angles from changing on load
	bUpdateViewAngles = false;

	PostEventMS(&EV_PostSpawn, 0);
}

void hhRailShuttle::Event_PostSpawn() {
	// Spawn the turret
	turret = gameLocal.SpawnObject(spawnArgs.GetString("def_turret"));
	if (turret.IsValid()) {
		turret->SetOrigin( GetOrigin() + spawnArgs.GetVector("offset_turret") * GetAxis());
		turret->SetAxis(GetAxis());
		turret->Bind(this, false);
		turret->GetPhysics()->SetContents(0);
		turret->Hide();
	}
	// Spawn the canopy
	canopy = gameLocal.SpawnObject(spawnArgs.GetString("def_turret"));
	if (canopy.IsValid()) {
		canopy->SetOrigin( GetOrigin() + spawnArgs.GetVector("offset_turret") * GetAxis());
		canopy->SetAxis(GetAxis());
		canopy->Bind(this, true);
		canopy->GetPhysics()->SetContents(0);	//temp: need to make it turn solid after player enters
		canopy->Hide();
	}

	// Spawn the arc beams
	idVec3 leftPos1 = GetOrigin() + spawnArgs.GetVector("offset_leftArcStart") * GetAxis();
	idVec3 leftPos2 = GetOrigin() + spawnArgs.GetVector("offset_leftArcEnd") * GetAxis();
	idVec3 leftDir = leftPos2 - leftPos1;
	leftDir.Normalize();
	leftArc = hhBeamSystem::SpawnBeam(leftPos1, spawnArgs.GetString("beam_arc"), mat3_identity, true);
	HH_ASSERT( leftArc.IsValid() );
	leftArc->SetOrigin(leftPos1);
	leftArc->SetAxis(leftDir.ToMat3());
	leftArc->Bind(turret.GetEntity(), true);
	leftArc->Activate( false );
	leftArc->fl.neverDormant = true;
	leftArc->ToggleBeamLength(true);
	leftArc->SetTargetLocation( leftPos2 );

	idVec3 rightPos1 = GetOrigin() + spawnArgs.GetVector("offset_rightArcStart") * GetAxis();
	idVec3 rightPos2 = GetOrigin() + spawnArgs.GetVector("offset_rightArcEnd") * GetAxis();
	idVec3 rightDir = rightPos2 - rightPos1;
	rightDir.Normalize();
	rightArc = hhBeamSystem::SpawnBeam(rightPos1, spawnArgs.GetString("beam_arc"), mat3_identity, true);
	HH_ASSERT( rightArc.IsValid() );
	rightArc->SetOrigin(rightPos1);
	rightArc->SetAxis(rightDir.ToMat3());
	rightArc->Bind(turret.GetEntity(), true);
	rightArc->Activate( false );
	rightArc->fl.neverDormant = true;
	rightArc->ToggleBeamLength(true);
	rightArc->SetTargetLocation( rightPos2 );
}

void hhRailShuttle::Save(idSaveGame *savefile) const {
	savefile->WriteFloat( defaultRadius );

	savefile->WriteFloat( sphereRadius.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( sphereRadius.GetDuration() );
	savefile->WriteFloat( sphereRadius.GetStartValue() );
	savefile->WriteFloat( sphereRadius.GetEndValue() );

	savefile->WriteAngles( sphereAngles );
	savefile->WriteAngles( sphereVelocity );
	savefile->WriteAngles( sphereAcceleration );
	savefile->WriteVec3( sphereCenter );
	savefile->WriteFloat( spherePitchMin );
	savefile->WriteFloat( spherePitchMax );
	savefile->WriteFloat( turretYawLimit );
	savefile->WriteFloat( turretPitchLimit );
	savefile->WriteFloat( linearFriction );
	savefile->WriteFloat( terminalVelocity );
	savefile->WriteAngles( localViewAngles );
	savefile->WriteAngles( oldViewAngles );

	turret.Save( savefile );
	canopy.Save( savefile );
	leftArc.Save( savefile );
	rightArc.Save( savefile );

	savefile->WriteBool( bDisengaged );
	savefile->WriteBool( bJumping );
	savefile->WriteBool( bBounced );
	savefile->WriteInt( jumpStartTime );
	savefile->WriteInt( jumpStage );
	savefile->WriteFloat( jumpSpeed );
	savefile->WriteFloat( jumpPosition );
	savefile->WriteInt( jumpNumBounces );
	savefile->WriteFloat( jumpInitSpeed );
	savefile->WriteFloat( jumpAccel );
	savefile->WriteFloat( jumpAccelTime );
	savefile->WriteFloat( jumpReturnForce );
	savefile->WriteInt( jumpBounceCount );
	savefile->WriteFloat( jumpBounceFactor );
	savefile->WriteFloat( jumpThrustMovementScale );
	savefile->WriteFloat( jumpBounceMovementScale );
	savefile->WriteBool( bWallMovementSoundPlaying );
	savefile->WriteBool( bAirMovementSoundPlaying );
}

void hhRailShuttle::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( defaultRadius );

	float set;
	savefile->ReadFloat( set );	// idInterpolate<float>
	sphereRadius.SetStartTime( set );
	savefile->ReadFloat( set );
	sphereRadius.SetDuration( set );
	savefile->ReadFloat( set );
	sphereRadius.SetStartValue( set );
	savefile->ReadFloat( set );
	sphereRadius.SetEndValue( set );

	savefile->ReadAngles( sphereAngles );
	savefile->ReadAngles( sphereVelocity );
	savefile->ReadAngles( sphereAcceleration );
	savefile->ReadVec3( sphereCenter );
	savefile->ReadFloat( spherePitchMin );
	savefile->ReadFloat( spherePitchMax );
	savefile->ReadFloat( turretYawLimit );
	savefile->ReadFloat( turretPitchLimit );
	savefile->ReadFloat( linearFriction );
	savefile->ReadFloat( terminalVelocity );
	savefile->ReadAngles( localViewAngles );
	savefile->ReadAngles( oldViewAngles );

	turret.Restore( savefile );
	canopy.Restore( savefile );
	leftArc.Restore( savefile );
	rightArc.Restore( savefile );

	savefile->ReadBool( bDisengaged );
	savefile->ReadBool( bJumping );
	savefile->ReadBool( bBounced );
	savefile->ReadInt( jumpStartTime );
	savefile->ReadInt( jumpStage );
	savefile->ReadFloat( jumpSpeed );
	savefile->ReadFloat( jumpPosition );
	savefile->ReadInt( jumpNumBounces );
	savefile->ReadFloat( jumpInitSpeed );
	savefile->ReadFloat( jumpAccel );
	savefile->ReadFloat( jumpAccelTime );
	savefile->ReadFloat( jumpReturnForce );
	savefile->ReadInt( jumpBounceCount );
	savefile->ReadFloat( jumpBounceFactor );
	savefile->ReadFloat( jumpThrustMovementScale );
	savefile->ReadFloat( jumpBounceMovementScale );
	savefile->ReadBool( bWallMovementSoundPlaying );
	savefile->ReadBool( bAirMovementSoundPlaying );

	SetPhysics( NULL );		// No collision, no expensive shuttle physics
	GetPhysics()->SetContents(0);

	//HUMANHEAD PCF mdl 04/29/06 - Added bUpdateViewAngles to prevent view angles from changing on load
	bUpdateViewAngles = true;
}

void hhRailShuttle::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhVehicle::WriteToSnapshot(msg);
}

void hhRailShuttle::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhVehicle::ReadFromSnapshot(msg);
}

void hhRailShuttle::ClientPredictionThink( void ) {
	Think();

	UpdateVisuals();
	Present();
}

void hhRailShuttle::BecomeConsole() {
	if (turret.IsValid()) {
		turret->Hide();
	}
	if (canopy.IsValid()) {
		canopy->Hide();
	}
	if (leftArc.IsValid()) {
		leftArc->Activate( false );
	}
	if (rightArc.IsValid()) {
		rightArc->Activate( false );
	}
	hhVehicle::BecomeConsole();
}

void hhRailShuttle::BecomeVehicle() {
	if (turret.IsValid()) {
		turret->Show();
	}
	if (canopy.IsValid()) {
		canopy->Show();
	}

	hhVehicle::BecomeVehicle();
}

// Static function to determine if a pilot is suitable
bool hhRailShuttle::ValidPilot( idActor *act ) {
	if (act && act->health > 0) {
		if (act->IsType(hhPlayer::Type)) {
			hhPlayer *player = static_cast<hhPlayer*>(act);
			if( !player->IsSpiritOrDeathwalking() && !player->IsPossessed() ) {
				return true;
			}
		}
		else if (act->IsType(idAI::Type)) {
			idAI *ai = static_cast<idAI*>(act);
			if (ai->spawnArgs.GetBool("canPilotShuttle")) {
				return true;
			}
		}
	}
	return false;
}

bool hhRailShuttle::WillAcceptPilot( idActor *act ) {
	return IsConsole() && hhRailShuttle::ValidPilot( act );
}

void hhRailShuttle::AcceptPilot( hhPilotVehicleInterface* pilotInterface ) {
	hhVehicle::AcceptPilot(pilotInterface);

	// supress model in player views, but allow it in mirrors and remote views
	if (turret.IsValid() && GetPilot()->IsType(hhPlayer::Type)) {
		turret->GetRenderEntity()->suppressSurfaceInViewID = GetPilot()->entityNumber + 1;
	}

	oldViewAngles = GetPilot()->viewAxis.ToAngles();
}

idVec3 hhRailShuttle::GetFireOrigin() {
	return turret->GetOrigin();
}

idMat3 hhRailShuttle::GetFireAxis() {
	return turret->GetAxis();
}

void hhRailShuttle::ProcessPilotInput( const usercmd_t* cmds, const idAngles* viewAngles ) {
	float movementScale;

	if ( cmds ) {
		ProcessButtons( *cmds );

		//Check vehicle here because we could have exited the vehicle in ProcessButtons
		if( !IsVehicle() ) {
			return;
		}

		ProcessImpulses( *cmds );

		// Apply booster to allow key taps to be very low thrust
		thrustScale = idMath::ClampFloat( thrustMin, thrustMax, thrustScale * thrustAccel );

		// Apply our forces as orbit velocity
		if ((cmds->rightmove || cmds->upmove || cmds->forwardmove) && HasPower(1) && !bDisengaged) {
			sphereAcceleration.yaw = cmds->rightmove * thrustFactor * thrustScale * (60.0f * USERCMD_ONE_OVER_HZ);
			sphereAcceleration.pitch = (cmds->upmove ? cmds->upmove : cmds->forwardmove) * thrustFactor * thrustScale * (60.0f * USERCMD_ONE_OVER_HZ);
			sphereAcceleration.roll = 0;
			sphereAcceleration += -sphereVelocity * linearFriction * (60.0f * USERCMD_ONE_OVER_HZ);
			SetCockpitParm( VCOCKPIT_PARM_MOVE, -MS2SEC( gameLocal.time ) );
			if (bJumping) {
				PlayAirMovementSound();
			}
			else {
				PlayWallMovementSound();
			}
		}
		else {
			sphereAcceleration = -sphereVelocity * linearFriction * (60.0f * USERCMD_ONE_OVER_HZ);
			thrustScale = thrustMin;
			if (bJumping) {
                StopAirMovementSound();
			}
			else {
				StopWallMovementSound();
			}
		}

		sphereVelocity += sphereAcceleration * (60.0f * USERCMD_ONE_OVER_HZ);
		sphereVelocity.pitch = idMath::ClampFloat(-terminalVelocity, terminalVelocity, sphereVelocity.pitch);
		sphereVelocity.yaw = idMath::ClampFloat(-terminalVelocity, terminalVelocity, sphereVelocity.yaw);

		movementScale = bJumping ? ( bBounced ? jumpBounceMovementScale : jumpThrustMovementScale ) : 1.f;
		sphereAngles += sphereVelocity * movementScale * (60.0f * USERCMD_ONE_OVER_HZ);
		if (sphereAngles.pitch <= spherePitchMin || sphereAngles.pitch >= spherePitchMax) {
			sphereAngles.pitch = idMath::ClampFloat(spherePitchMin, spherePitchMax, sphereAngles.pitch);
			sphereVelocity.pitch = 0;
		}
		sphereAngles = sphereAngles.Normalize180();

		float radius = sphereRadius.GetCurrentValue(gameLocal.time) - UpdateJump( cmds );

		idMat3 sphereAxis = sphereAngles.ToMat3();
		SetOrigin(sphereCenter - sphereAxis[0] * radius);
		SetAxis(sphereAxis);

		memcpy( &oldCmds, cmds, sizeof(usercmd_t) );
	}

	// Apply viewAngles to turret
	if ( viewAngles ) {
		//HUMANHEAD PCF mdl 04/29/06 - Added bUpdateViewAngles to prevent view angles from changing on load
		if ( bUpdateViewAngles ) {
			// Update the old view angles after loading from a savegame
			bUpdateViewAngles = false;
		} else {
			idAngles deltaViewAngles;
			deltaViewAngles = *viewAngles - oldViewAngles;
			localViewAngles += deltaViewAngles;

			// Clamp localViewAngles
			if ( !IsNoClipping() ) {
				localViewAngles.pitch = idMath::AngleNormalize180( localViewAngles.pitch );
				localViewAngles.pitch = idMath::ClampFloat(-turretPitchLimit, turretPitchLimit, localViewAngles.pitch);
				localViewAngles.pitch = idMath::AngleNormalize180( localViewAngles.pitch );

				localViewAngles.yaw = idMath::AngleNormalize180( localViewAngles.yaw );
				localViewAngles.yaw = idMath::ClampFloat(-turretYawLimit, turretYawLimit, localViewAngles.yaw);
				localViewAngles.yaw = idMath::AngleNormalize180( localViewAngles.yaw );
			}

			idAngles turretAngles = sphereAngles + localViewAngles;
			turret->SetAxis( turretAngles.ToMat3() );
		}

		memcpy( &oldViewAngles, viewAngles, sizeof(idAngles) );
	}
}

/*
float hhRailShuttle::UpdateJump() {
	float jumpDist;

	jumpDist = 0;
	if( bJumping ) {
		float elapsedTime = MS2SEC( gameLocal.time - jumpStartTime );
		float jumpDuration = spawnArgs.GetFloat( "jumpDuration" );
		if( elapsedTime < jumpDuration ) {
			jumpDist = sin( elapsedTime * idMath::PI / jumpDuration ) * spawnArgs.GetFloat( "jumpDistance" );
		}
		else {
			bJumping = false;
		}
	}
	return jumpDist;
}
*/

float hhRailShuttle::UpdateJump( const usercmd_t* cmds ) {
	float elapsedTime;

	if( !bJumping) {
		return 0;
	}

	elapsedTime = MS2SEC( gameLocal.time - jumpStartTime );
	if( jumpStage == 1 ) {
		jumpSpeed += jumpAccel * (60.0f * USERCMD_ONE_OVER_HZ);
		if( elapsedTime > jumpAccelTime ) {
			jumpStage = 2;
		}
	}
	jumpSpeed -= jumpReturnForce * (60.0f * USERCMD_ONE_OVER_HZ);
	jumpPosition += jumpSpeed * (60.0f * USERCMD_ONE_OVER_HZ);
	if( jumpPosition < 0 ) {
		jumpPosition = 0;
		if( ++jumpNumBounces > jumpBounceCount ) {
			bJumping = false;
			StopAirMovementSound();
			StartSound( "snd_jumpland", SND_CHANNEL_ANY );
		}
		else {
			bBounced = true;
			jumpSpeed = -jumpSpeed * jumpBounceFactor;
			StartSound( "snd_jumpbounce", SND_CHANNEL_ANY );
			if( (cmds->buttons & BUTTON_ATTACK_ALT) && sphereRadius.IsDone(gameLocal.time) && HasPower(1) && !bDisengaged ) {
				Jump();
			}
		}
	}
	SetCockpitParm( VCOCKPIT_PARM_JUMP, jumpPosition );
	return jumpPosition;
}

void hhRailShuttle::PlayWallMovementSound() {
	if (!bWallMovementSoundPlaying) {
		StartSound("snd_wallmovestart", SND_CHANNEL_ANY, 0, true);
		StartSound("snd_wallmove", SND_CHANNEL_MISC5, 0, true);
		bWallMovementSoundPlaying = true;
	}
}

void hhRailShuttle::StopWallMovementSound() {
	if (bWallMovementSoundPlaying) {
		StopSound(SND_CHANNEL_MISC5, true);
		if (!bJumping) {
			StartSound("snd_wallmovestop", SND_CHANNEL_ANY, 0, true);
		}
		bWallMovementSoundPlaying = false;
	}
}

void hhRailShuttle::PlayAirMovementSound() {
	if (!bAirMovementSoundPlaying) {
		StartSound("snd_thrust", SND_CHANNEL_THRUSTERS, 0, true);
		bAirMovementSoundPlaying = true;
	}
}

void hhRailShuttle::StopAirMovementSound() {
	if (bAirMovementSoundPlaying) {
		StopSound(SND_CHANNEL_THRUSTERS, true);
		bAirMovementSoundPlaying = false;
	}
}

void hhRailShuttle::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if( !HasPower(1) || bDisengaged ) {
		return;
	}
	if (!InGodMode()) {
		if (idStr::Icmp(damageDefName, "damage_railbeam") == 0) {
			ConsumePower(currentPower);
			PostEventMS(&EV_RestorePower, SEC2MS(spawnArgs.GetFloat("powerOutageDuration")));
			StartSound( "snd_damagestart", SND_CHANNEL_ANY );
			StartSound( "snd_damageloop", SND_CHANNEL_MISC3, 0, true );

			idEntity *ent = gameLocal.FindEntity( spawnArgs.GetString( "triggerDamage" ) );
			if( ent ) {
				ent->PostEventMS( &EV_Activate, 0, inflictor );
			}

			SetCockpitParm( VCOCKPIT_PARM_POWER, -MS2SEC( gameLocal.time ) );
		}
	}
	//rww - don't want this thing to ever take real damage
	//hhVehicle::Damage( inflictor, attacker, dir, damageDefName, 1.0f, location );
}

void hhRailShuttle::Event_SetOrbitRadius(float radius, int lerpTime) {
	defaultRadius = radius;
	float currentRadius = sphereRadius.GetCurrentValue(gameLocal.time);
	sphereRadius.Init(gameLocal.time, lerpTime, currentRadius, defaultRadius);
	bJumping = false;	// Cancel any jumping
}

void hhRailShuttle::Event_Jump() {
	if (!bJumping && sphereRadius.IsDone(gameLocal.time) && HasPower(1) && !bDisengaged) {
		Jump();
	}
}

void hhRailShuttle::Event_FinalJump() {
	jumpInitSpeed = spawnArgs.GetFloat("endingInitSpeed");
	jumpAccel = spawnArgs.GetFloat("endingAccel");
	jumpAccelTime = spawnArgs.GetFloat("endingAccelTime");
	jumpReturnForce = spawnArgs.GetFloat("endingReturnForce");
	Jump();
}

void hhRailShuttle::Event_IsJumping() {
	idThread::ReturnFloat(bJumping);
}

void hhRailShuttle::Event_Disengage() {
	bDisengaged = true;
}

void hhRailShuttle::Jump() {
	bJumping = true;
	bBounced = false;
	jumpStartTime = gameLocal.time;
	jumpStage = 1;
	jumpSpeed = jumpInitSpeed;
	jumpPosition = 0;
	jumpNumBounces = 0;
	StopWallMovementSound();
	if( !bDisengaged ) {
		StartSound( "snd_jump", SND_CHANNEL_MISC2 );
	}
}

void hhRailShuttle::Event_FireCannon() {
	if( HasPower(1) && !bDisengaged && fireController && fireController->LaunchProjectiles(vec3_zero) ) {
		StartSound( "snd_cannon", SND_CHANNEL_ANY );
		fireController->MuzzleFlash();
		fireController->WeaponFeedback();
		SetCockpitParm( VCOCKPIT_PARM_FIRE, -MS2SEC( gameLocal.time ) );

		idEntity *ent = gameLocal.FindEntity( spawnArgs.GetString( "triggerFire" ) );
		if( ent ) {
			ent->PostEventMS( &EV_Activate, 0, this );
		}

		leftArc->Activate( true );
		rightArc->Activate( true );

		CancelEvents(&EV_HideArcs);
		PostEventMS(&EV_HideArcs, spawnArgs.GetFloat("arcDuration"));
	}
}

void hhRailShuttle::Event_HideArcs() {
	leftArc->Activate( false );
	rightArc->Activate( false );
}

void hhRailShuttle::Event_RestorePower() {
	StopSound( SND_CHANNEL_MISC3, true );
	StartSound( "snd_damagestop", SND_CHANNEL_ANY );
	GivePower(100);
}

void hhRailShuttle::SetCockpitParm( int parmNumber, float value ) {
	if( GetPilotInterface() && GetPilotInterface()->IsType( hhPlayerVehicleInterface::Type ) ) {
		hhControlHand *hand = static_cast<hhPlayerVehicleInterface*>(GetPilotInterface())->GetHandEntity();
		if( hand ) {
			hand->SetShaderParm( parmNumber, value );
		}
	}
}

void hhRailShuttle::Event_GetCockpit() {
	if( GetPilotInterface() && GetPilotInterface()->IsType( hhPlayerVehicleInterface::Type ) ) {
		hhControlHand *hand = static_cast<hhPlayerVehicleInterface*>(GetPilotInterface())->GetHandEntity();
		if( hand ) {
			idThread::ReturnEntity( hand );
		} else {
			idThread::ReturnEntity( NULL );
		}
	}
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build