// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "SoundControl.h"
#include "../Player.h"
#include "Transport.h"
#include "../../decllib/DeclSurfaceType.h"
#include "JetPack.h"

/*
===============================================================================

	sdVehicleSoundControlBase

===============================================================================
*/

sdVehicleSoundControlBase::soundControlFactory_t sdVehicleSoundControlBase::s_factory;

/*
================
sdVehicleSoundControlBase::Init
================
*/
void sdVehicleSoundControlBase::Init( sdTransport* transport ) {
	owner = transport;
}

/*
================
sdVehicleSoundControlBase::Startup
================
*/
void sdVehicleSoundControlBase::Startup( void ) {
	s_factory.RegisterType( "husky", soundControlFactory_t::Allocator< sdVehicleSoundControl_Wheeled > );
	s_factory.RegisterType( "badger", soundControlFactory_t::Allocator< sdVehicleSoundControl_Wheeled > );
	s_factory.RegisterType( "trojan", soundControlFactory_t::Allocator< sdVehicleSoundControl_Wheeled > );
	s_factory.RegisterType( "wheeled", soundControlFactory_t::Allocator< sdVehicleSoundControl_Wheeled > );
	s_factory.RegisterType( "tracked", soundControlFactory_t::Allocator< sdVehicleSoundControl_Tracked > );
	s_factory.RegisterType( "helicopter", soundControlFactory_t::Allocator< sdVehicleSoundControl_Helicopter > );
	s_factory.RegisterType( "jetpack", soundControlFactory_t::Allocator< sdVehicleSoundControl_JetPack > );
	s_factory.RegisterType( "speedboat", soundControlFactory_t::Allocator< sdVehicleSoundControl_SpeedBoat > );
}

/*
================
sdVehicleSoundControlBase::Shutdown
================
*/
void sdVehicleSoundControlBase::Shutdown( void ) {
	s_factory.Shutdown();
}

/*
================
sdVehicleSoundControlBase::InWater
================
*/
bool sdVehicleSoundControlBase::InWater( void ) const {
	return owner->GetPhysics()->InWater() > 0.0005f;
}

/*
================
sdVehicleSoundControlBase::Submerged
================
*/
bool sdVehicleSoundControlBase::Submerged( void ) const {
	return owner->GetPhysics()->InWater() > 0.5f;
}

/*
================
sdVehicleSoundControlBase::Alloc
================
*/
sdVehicleSoundControlBase* sdVehicleSoundControlBase::Alloc( const char* name ) {
	return s_factory.CreateType( name );
}


/*
===============================================================================

	sdVehicleSoundControl_Simple

===============================================================================
*/

/*
================
sdVehicleSoundControl_Simple::Init
================
*/
void sdVehicleSoundControl_Simple::Init( sdTransport* transport ) {
	sdVehicleSoundControlBase::Init( transport );

	lowPitch			= owner->spawnArgs.GetFloat( "engine_pitch_low" );
	highPitch			= owner->spawnArgs.GetFloat( "engine_pitch_high" );
	lowSpeed			= owner->spawnArgs.GetFloat( "engine_speed_low" );
	highSpeed			= owner->spawnArgs.GetFloat( "engine_speed_high" );

	maxSoundLevel		= owner->spawnArgs.GetFloat( "max_sound_level", "-5" );

	maxHornWaterLevel	= owner->spawnArgs.GetFloat( "max_horn_water_level", "0.5" );
}

/*
================
sdVehicleSoundControl_Simple::CalcSoundParms
================
*/
void sdVehicleSoundControl_Simple::CalcSoundParms( soundParms_t& parms ) const {
	parms.inWater		= InWater();
	parms.submerged		= Submerged();
	parms.speedKPH		= UPStoKPH( owner->GetPhysics()->GetLinearVelocity() * owner->GetPhysics()->GetAxis()[ 0 ] );
	parms.absSpeedKPH	= idMath::Fabs( parms.speedKPH );

	parms.newSoundLevel = maxSoundLevel - ( 1.0f / ( 0.01f * ( parms.absSpeedKPH + 0.00001f ) ) );

	float speedDiff = parms.absSpeedKPH - lowSpeed;
	if ( speedDiff < 0.0f ) {
		speedDiff = 0.0f;
	}

	if ( parms.speedKPH < 0.5f || ( parms.inWater && !owner->IsAmphibious() ) ) {
		parms.newSoundPitch = lowPitch;
	} else {
		parms.newSoundPitch = Lerp( lowPitch, highPitch, speedDiff / ( highSpeed - lowSpeed ) );
	}
}

/*
================
sdVehicleSoundControl_Simple::OnSurfaceTypeChanged
================
*/
void sdVehicleSoundControl_Simple::OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType ) {
	groundIsOffRoad = false;

	if ( surfaceType == NULL ) {
		groundSurfaceType = "";
		return;
	}

	if ( surfaceType->GetProperties().GetBool( "offroad" ) ) {
		groundSurfaceType = "_offroad";
		groundIsOffRoad = true;
	} else {
		groundSurfaceType = "_";
		groundSurfaceType += surfaceType->GetType();
	}
}

/*
================
sdVehicleSoundControl_Simple::StartHornSound
================
*/
void sdVehicleSoundControl_Simple::StartHornSound( void ) {
	if ( owner->GetPhysics()->InWater() > maxHornWaterLevel ) {
		StopHornSound();
		return;
	}

	if ( simpleSoundFlags.playingHornSound ) {
		return;
	}
	simpleSoundFlags.playingHornSound = true;

	owner->StartSound( "snd_horn_loop", SND_VEHICLE_HORN, 0, NULL );	
}

/*
================
sdVehicleSoundControl_Simple::StopHornSound
================
*/
void sdVehicleSoundControl_Simple::StopHornSound( void ) {
	if ( !simpleSoundFlags.playingHornSound ) {
		return;
	}
	simpleSoundFlags.playingHornSound = false;

	owner->StartSound( "snd_horn_stop", SND_VEHICLE_HORN, 0, NULL );
}

/*
================
sdVehicleSoundControl_Simple::StartCockpitSound
================
*/
void sdVehicleSoundControl_Simple::StartCockpitSound( void ) {
	if ( simpleSoundFlags.playingCockpitSound ) {
		return;
	}
	simpleSoundFlags.playingCockpitSound = true;

	if ( !owner->IsWeaponEMPed() ) {
		owner->StartSound( "snd_cockpit", SND_VEHICLE_INTERIOR, 0, NULL );
	} else {
		owner->StartSound( "snd_cockpit_emped", SND_VEHICLE_INTERIOR, 0, NULL );
	}
}

/*
================
sdVehicleSoundControl_Simple::StopCockpitSound
================
*/
void sdVehicleSoundControl_Simple::StopCockpitSound( void ) {
	if ( !simpleSoundFlags.playingCockpitSound ) {
		return;
	}
	simpleSoundFlags.playingCockpitSound = false;

	owner->StopSound( SND_VEHICLE_INTERIOR );
}

/*
================
sdVehicleSoundControl_Simple::OnEMPStateChanged
================
*/
void sdVehicleSoundControl_Simple::OnEMPStateChanged( void ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( !owner->IsEMPed() ) {
		if ( driver != NULL ) {
			owner->StartSound( "snd_engine_start", SND_VEHICLE_DRIVE, 0, NULL );
			owner->StartSound( "snd_engine_start_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
		}
	} else {
		if ( driver != NULL ) {
			owner->StartSound( "snd_engine_stop", SND_VEHICLE_DRIVE, 0, NULL );
			owner->StartSound( "snd_engine_stop_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
		}
	}
}

/*
================
sdVehicleSoundControl_Simple::OnWeaponEMPStateChanged
================
*/
void sdVehicleSoundControl_Simple::OnWeaponEMPStateChanged( void ) {
	StopCockpitSound();
	if ( !owner->GetPositionManager().IsEmpty() ) {
		if ( !owner->IsWeaponEMPed() ) {
			owner->StartSound( "snd_cockpit_end_emped", SND_VEHICLE_INTERIOR_POWERUP, 0, NULL );
		}
		StartCockpitSound();
	}
}

/*
===============================================================================

	sdVehicleSoundControl_CrossFade

===============================================================================
*/

/*
================
sdVehicleSoundControl_CrossFade::Init
================
*/
void sdVehicleSoundControl_CrossFade::Init( sdTransport* transport ) {
	sdVehicleSoundControl_Simple::Init( transport );

	accelSpoolTime = transport->spawnArgs.GetFloat( "engine_accel_spool_time", "0.17" );
	decelSpoolTime = transport->spawnArgs.GetFloat( "engine_decel_spool_time", "0.25" );

	idleMinSpeed = transport->spawnArgs.GetFloat( "engine_idle_min_speed", "22.0" );
	idleMaxSpeed = transport->spawnArgs.GetFloat( "engine_idle_max_speed", "30.0" );
	idleMinVol = transport->spawnArgs.GetFloat( "engine_idle_min_vol", "0.0" );
	idleMaxVol = transport->spawnArgs.GetFloat( "engine_idle_max_vol", "-15.0" );
	idlePower = transport->spawnArgs.GetFloat( "engine_idle_power", "1.0" );
	idleFadeTime = transport->spawnArgs.GetFloat( "engine_idle_fade_time", "0.066" );

	driveMinSpeed = transport->spawnArgs.GetFloat( "engine_drive_min_speed", "0.0" );
	driveMaxSpeed = transport->spawnArgs.GetFloat( "engine_drive_max_speed", "30.0" );
	driveMinVol = transport->spawnArgs.GetFloat( "engine_drive_min_vol", "-10.0" );
	driveMaxVol = transport->spawnArgs.GetFloat( "engine_drive_max_vol", "0.0" );
	drivePower = transport->spawnArgs.GetFloat( "engine_drive_power", "0.5" );
	driveFadeTime = transport->spawnArgs.GetFloat( "engine_drive_fade_time", "0.066" );

	accelPitchMultiplier = transport->spawnArgs.GetFloat( "engine_accel_pitch_mult", "3.0" );
	accelPitchOffset = transport->spawnArgs.GetFloat( "engine_accel_pitch_offset", "1.0" );

	accelMinSpeed = transport->spawnArgs.GetFloat( "engine_accel_min_speed", "0.0" );
	accelMidSpeed = transport->spawnArgs.GetFloat( "engine_accel_mid_speed", "5.0" );
	accelMaxSpeed = transport->spawnArgs.GetFloat( "engine_accel_max_speed", "30.0" );
	accelMinVol = transport->spawnArgs.GetFloat( "engine_accel_min_vol", "-10.0" );
	accelMidVol = transport->spawnArgs.GetFloat( "engine_accel_mid_vol", "5.0" );
	accelMaxVol = transport->spawnArgs.GetFloat( "engine_accel_max_vol", "-10.0" );
	accelPowerLow = transport->spawnArgs.GetFloat( "engine_accel_power_low", "0.1" );
	accelPowerHigh = transport->spawnArgs.GetFloat( "engine_accel_power_high", "3.0" );
	accelFadeTime = transport->spawnArgs.GetFloat( "engine_accel_fade_time", "0.0" );

	accelYawVolume = transport->spawnArgs.GetFloat( "engine_accel_yaw_vol", "0.0" );
	accelYawVolumeMultiplier = transport->spawnArgs.GetFloat( "engine_accel_yaw_vol_mult", "0.0" );
	accelYawPitch = transport->spawnArgs.GetFloat( "engine_accel_yaw_pitch", "0.0" );
	accelYawPitchMultiplier = transport->spawnArgs.GetFloat( "engine_accel_yaw_pitch_mult", "0.0" );

	engineSpeed = 0.0f;
	lastVolumeIncreaseValue = 0.0f;
}

/*
================
sdVehicleSoundControl_CrossFade::CalcSoundParmsAdvanced
================
*/
void sdVehicleSoundControl_CrossFade::CalcSoundParmsAdvanced( soundParmsAdvanced_t& parms ) {
	sdVehicleSoundControl_Simple::CalcSoundParms( parms.simple );
	const sdVehicleInput& input = owner->GetInput();

	if ( parms.simple.absSpeedKPH > engineSpeed ) {
		engineSpeed = Lerp( engineSpeed, parms.simple.absSpeedKPH, MS2SEC( gameLocal.msec ) / accelSpoolTime );
	} else {
		engineSpeed = Lerp( engineSpeed, parms.simple.absSpeedKPH, MS2SEC( gameLocal.msec ) / decelSpoolTime );
	}

	engineSpeed -= 1.0f;
	if ( engineSpeed < 0.0001f ) {
		engineSpeed = 0.0f;
	}

#define FADE_SOUND( speed, minSpeed, maxSpeed, minVol, maxVol, power ) ( idMath::Pow( idMath::ClampFloat( 0.0f, 1.0f, ( speed - minSpeed ) / ( maxSpeed - minSpeed ) ), power ) * ( maxVol - minVol ) + minVol )
#define FADE_SOUND_2( speed, minSpeed, midSpeed, maxSpeed, minVol, midVol, maxVol, loPower, hiPower ) ( speed < midSpeed ? FADE_SOUND( speed, minSpeed, midSpeed, minVol, midVol, loPower ) : FADE_SOUND( speed, midSpeed, maxSpeed, midVol, maxVol, hiPower ) )

	parms.idleVolume = FADE_SOUND( engineSpeed, idleMinSpeed, idleMaxSpeed, idleMinVol, idleMaxVol, idlePower );
	parms.driveVolume = FADE_SOUND( engineSpeed, driveMinSpeed, driveMaxSpeed, driveMinVol, driveMaxVol, drivePower );

	// accel sound volume & pitch depends on the status of the inputs too
	float currentAccelPitchOffset = accelPitchOffset;
	float accelEngineSpeed = engineSpeed;
	float yawVel = RAD2DEG( owner->GetPhysics()->GetAngularVelocity() * owner->GetPhysics()->GetAxis()[ 2 ] );
	if ( accelYawVolume > 0.0f ) {
		accelEngineSpeed += accelYawVolumeMultiplier * idMath::ClampFloat( 0.0f, 1.0f, idMath::Fabs( yawVel ) / accelYawVolume );
	}
	if ( accelYawPitch > 0.0f ) {
		currentAccelPitchOffset += accelYawPitchMultiplier * idMath::ClampFloat( 0.0f, 1.0f, idMath::Fabs( yawVel ) / accelYawPitch );
	}
	
	float volumeIncreaseValue = 0.0f;
	float inputVal = 0.0f;
	if ( owner->GetPhysics()->HasGroundContacts() || ( owner->GetPhysics()->InWater() && owner->IsAmphibious() ) ) {
		inputVal = input.GetForward() > 0.0f ? input.GetForward() : -input.GetForward() * 0.3f;
		if ( accelYawVolumeMultiplier > 0.0f ) {
			inputVal = Max( inputVal, idMath::Fabs( input.GetRight() ) );
		}
		float midVol = inputVal * accelMidVol;
		if ( inputVal > 0.01f ) {
			volumeIncreaseValue = 0.3f;

			// factor in some of the up-vector
			float upNess = RAD2DEG( idMath::ACos( idMath::ClampFloat( 0.0f, 1.0f, owner->GetPhysics()->GetAxis()[ 0 ].z ) ) );
			upNess = 1.0f - upNess / 90.0f;
			//accelEngineSpeed = Lerp( accelEngineSpeed, accelMidSpeed, upNess );
			volumeIncreaseValue = Max( upNess, volumeIncreaseValue );
		}
	}

	if ( volumeIncreaseValue < lastVolumeIncreaseValue ) {
		volumeIncreaseValue = Lerp( lastVolumeIncreaseValue, volumeIncreaseValue, MS2SEC( gameLocal.msec ) / 0.3f );
	}
	if ( volumeIncreaseValue < 0.0001f ) {
		volumeIncreaseValue = 0.0f;
	}
	lastVolumeIncreaseValue = volumeIncreaseValue;

	parms.accelVolume = FADE_SOUND_2( accelEngineSpeed, accelMinSpeed, accelMidSpeed, accelMaxSpeed, accelMinVol, accelMidVol * inputVal, accelMaxVol, accelPowerLow, accelPowerHigh );
	parms.accelPitch = ( parms.simple.newSoundPitch - lowPitch ) * accelPitchMultiplier + currentAccelPitchOffset;

	parms.accelVolume = Lerp( parms.accelVolume, accelMidVol, idMath::Sqrt( volumeIncreaseValue ) );

	// increase the pitch too
	parms.simple.newSoundPitch += volumeIncreaseValue*( highPitch - lowPitch );
	parms.accelPitch += volumeIncreaseValue*( highPitch - lowPitch );

	parms.simple.newSoundPitch = Min( parms.simple.newSoundPitch, highPitch );
	parms.accelPitch = Min( parms.accelPitch, highPitch * 1.5f );
}

/*
================
sdVehicleSoundControl_CrossFade::StartEngineSounds
================
*/
void sdVehicleSoundControl_CrossFade::StartEngineSounds() {
	owner->StartSound( "snd_engine_idle", SND_VEHICLE_DRIVE, 0, NULL );
	owner->StartSound( "snd_engine_drive", SND_VEHICLE_DRIVE2, 0, NULL );
	owner->StartSound( "snd_engine_hardaccel", SND_VEHICLE_DRIVE3, 0, NULL );
	owner->FadeSound( SND_VEHICLE_DRIVE, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_DRIVE2, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_DRIVE3, -60.0f, 0.0f );

	owner->StartSound( "snd_engine_idle_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
	owner->StartSound( "snd_engine_drive_interior", SND_VEHICLE_INTERIOR_DRIVE2, 0, NULL );
	owner->StartSound( "snd_engine_hardaccel_interior", SND_VEHICLE_INTERIOR_DRIVE3, 0, NULL );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE3, -60.0f, 0.0f );

	engineSpeed = 0.0f;
	lastVolumeIncreaseValue = 0.0f;
}

/*
================
sdVehicleSoundControl_CrossFade::StopEngineSounds
================
*/
void sdVehicleSoundControl_CrossFade::StopEngineSounds() {
	owner->FadeSound( SND_VEHICLE_DRIVE, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_DRIVE2, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_DRIVE3, -60.0f, 0.3f );

	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE3, -60.0f, 0.3f );

	soundKillTime = gameLocal.time + 300;
}

/*
================
sdVehicleSoundControl_CrossFade::UpdateEngineSounds
================
*/
void sdVehicleSoundControl_CrossFade::UpdateEngineSounds( soundParmsAdvanced_t& parms ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	if ( driver != NULL && !owner->IsEMPed() && ( !parms.simple.submerged || owner->IsAmphibious() ) ) {
		// set the results
		owner->SetChannelPitchShift( SND_VEHICLE_DRIVE, parms.simple.newSoundPitch );
		owner->SetChannelPitchShift( SND_VEHICLE_DRIVE2, parms.simple.newSoundPitch );
		owner->SetChannelPitchShift( SND_VEHICLE_DRIVE3, parms.accelPitch );
		owner->FadeSound( SND_VEHICLE_DRIVE, parms.idleVolume, idleFadeTime );
		owner->FadeSound( SND_VEHICLE_DRIVE2, parms.driveVolume, driveFadeTime );
		owner->FadeSound( SND_VEHICLE_DRIVE3, parms.accelVolume, accelFadeTime );

		owner->SetChannelPitchShift( SND_VEHICLE_INTERIOR_DRIVE, parms.simple.newSoundPitch );
		owner->SetChannelPitchShift( SND_VEHICLE_INTERIOR_DRIVE2, parms.simple.newSoundPitch );
		owner->SetChannelPitchShift( SND_VEHICLE_INTERIOR_DRIVE3, parms.accelPitch );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE, parms.idleVolume, idleFadeTime );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, parms.driveVolume, driveFadeTime );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE3, parms.accelVolume, accelFadeTime );

		soundKillTime = gameLocal.time + 5000;
	} else {
		engineSpeed = 0.0f;
		lastVolumeIncreaseValue = 0.0f;
	}

	// stop the sounds after an expiry has finished
	if ( gameLocal.time >= soundKillTime ) {
		owner->StopSound( SND_VEHICLE_DRIVE );
		owner->StopSound( SND_VEHICLE_DRIVE2 );
		owner->StopSound( SND_VEHICLE_DRIVE3 );
		owner->StopSound( SND_VEHICLE_INTERIOR_DRIVE );
		owner->StopSound( SND_VEHICLE_INTERIOR_DRIVE2 );
		owner->StopSound( SND_VEHICLE_INTERIOR_DRIVE3 );
	}
}

/*
===============================================================================

	sdVehicleSoundControl_Wheeled

===============================================================================
*/

/*
================
sdVehicleSoundControl_Wheeled::sdVehicleSoundControl_Wheeled
================
*/
sdVehicleSoundControl_Wheeled::sdVehicleSoundControl_Wheeled( void ) {
	soundFlags.playingOffRoadSound		= false;
	soundFlags.skidSoundFadedOut		= false;
	soundFlags.playingSkidSound			= false;
	soundFlags.inWater					= false;
	soundFlags.playingDriveSound		= false;

	groundSurfaceType					= NULL;
}

/*
================
sdVehicleSoundControl_Wheeled::sdVehicleSoundControl_Wheeled
================
*/
void sdVehicleSoundControl_Wheeled::Init( sdTransport* transport ) {
	sdVehicleSoundControl_CrossFade::Init( transport );
}

/*
================
sdVehicleSoundControl_Wheeled::Update
================
*/
void sdVehicleSoundControl_Wheeled::Update( void ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	if ( driver != NULL ) {
		const usercmd_t& cmd = gameLocal.usercmds[ driver->entityNumber ];

		if ( cmd.buttons.btn.modeSwitch ) {
			StartHornSound();
		} else {
			StopHornSound();
		}
	} else {
		StopHornSound();
	}

	soundParmsAdvanced_t parms;
	CalcSoundParmsAdvanced( parms );
	UpdateEngineSounds( parms );

	if ( parms.simple.absSpeedKPH > 10.f ) {
		FadeSkidSoundIn( parms.simple.newSoundLevel );
	} else {
		FadeSkidSoundOut();
	}

	if ( parms.simple.inWater ) {
		EnterWater( parms.simple.absSpeedKPH );

		owner->SetChannelVolume( SND_VEHICLE_OFFROAD, parms.simple.newSoundLevel );
	} else {
		ExitWater( parms.simple.absSpeedKPH );

		if ( parms.simple.absSpeedKPH > 0.1f ) {
			owner->SetChannelVolume( SND_VEHICLE_OFFROAD, parms.simple.newSoundLevel );

			StartOffRoadSound( false );
		} else {
			StopOffRoadSound();
		}
	}

	const sdVehicleInput& input = owner->GetInput();

	bool wantSkid	= false;
	bool wantDrive	= false;
	bool brakesOn	= input.GetBraking() | input.GetHandBraking();

	if ( !parms.simple.inWater ) {
		if ( parms.simple.absSpeedKPH > 30.f ) {
			wantSkid = brakesOn;
		}
	}

	if ( wantSkid ) {
		StartSkidSound( false );
	} else {
		StopSkidSound();
	}

	if ( !owner->IsEMPed() && !parms.simple.submerged && driver != NULL ) {
		StartDriveSound();
	} else {
		StopDriveSound();
	}
}

/*
================
sdVehicleSoundControl_Wheeled::OnPlayerEntered
================
*/
void sdVehicleSoundControl_Wheeled::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	if ( position == 0 ) {
		if ( !owner->IsEMPed() ) {
			owner->StartSound( "snd_engine_start", SND_VEHICLE_START, 0, NULL );
		}

		StartCockpitSound();
	}
}

/*
================
sdVehicleSoundControl_Wheeled::OnPlayerExited
================
*/
void sdVehicleSoundControl_Wheeled::OnPlayerExited( idPlayer* player, int position ) {
	if ( position == 0 ) {
		StopHornSound();
		StopOffRoadSound();

		if ( !owner->IsEMPed() ) {
			owner->StartSound( "snd_engine_stop", SND_VEHICLE_STOP, 0, NULL );
		}

		StopCockpitSound();
	}
}

/*
================
sdVehicleSoundControl_Wheeled::OnEMPStateChanged
================
*/
void sdVehicleSoundControl_Wheeled::OnEMPStateChanged( void ) {
	sdVehicleSoundControl_Simple::OnEMPStateChanged();
}

/*
================
sdVehicleSoundControl_Wheeled::OnWeaponEMPStateChanged
================
*/
void sdVehicleSoundControl_Wheeled::OnWeaponEMPStateChanged( void ) {
	sdVehicleSoundControl_Simple::OnWeaponEMPStateChanged();
}

/*
================
sdVehicleSoundControl_Wheeled::StartOffRoadSound
================
*/
void sdVehicleSoundControl_Wheeled::StartOffRoadSound( bool force ) {
	if ( !force && soundFlags.playingOffRoadSound ) {
		return;
	}
	soundFlags.playingOffRoadSound = true;

	idStr snd( va( "snd_wheel%s", groundSurfaceType.c_str() ) );
	if ( !idStr::Cmp( owner->spawnArgs.GetString( snd.c_str() ), "" ) ) {
		snd = "snd_wheel";
	}

	owner->StartSound( snd.c_str(), SND_VEHICLE_OFFROAD, 0, NULL );
	owner->SetChannelVolume( SND_VEHICLE_OFFROAD, -60.0f );
}

/*
================
sdVehicleSoundControl_Wheeled::StopOffRoadSound
================
*/
void sdVehicleSoundControl_Wheeled::StopOffRoadSound( void ) {
	if ( !soundFlags.playingOffRoadSound ) {
		return;
	}
	soundFlags.playingOffRoadSound = false;

	owner->StopSound( SND_VEHICLE_OFFROAD );
}

/*
================
sdVehicleSoundControl_Wheeled::FadeSkidSoundIn
================
*/
void sdVehicleSoundControl_Wheeled::FadeSkidSoundIn( float volume ) {
	owner->SetChannelVolume( SND_VEHICLE_SKID, volume );
	soundFlags.skidSoundFadedOut = false;
}

/*
================
sdVehicleSoundControl_Wheeled::FadeSkidSoundOut
================
*/
void sdVehicleSoundControl_Wheeled::FadeSkidSoundOut( void ) {
	if ( soundFlags.skidSoundFadedOut ) {
		return;
	}
	soundFlags.skidSoundFadedOut = true;
	owner->FadeSound( SND_VEHICLE_SKID, -60.f, 0.5f );
}

/*
================
sdVehicleSoundControl_Wheeled::OnSurfaceTypeChanged
================
*/
void sdVehicleSoundControl_Wheeled::OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType ) {
	sdVehicleSoundControl_Simple::OnSurfaceTypeChanged( surfaceType );

	if ( soundFlags.playingOffRoadSound ) {
		StartOffRoadSound( true );
	}

	if ( soundFlags.playingSkidSound ) {
		StartSkidSound( true );
	}
}

/*
================
sdVehicleSoundControl_Wheeled::EnterWater
================
*/
void sdVehicleSoundControl_Wheeled::EnterWater( float absSpeedKPH ) {
	if ( soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = true;

	StopOffRoadSound();

	owner->StartSound( "snd_water_wake", SND_VEHICLE_OFFROAD, 0, NULL );
	owner->StartSound( absSpeedKPH > 30.f ? "snd_water_enter" : "snd_water_splash", SND_VEHICLE_MISC, 0, NULL );

	if ( owner->IsAmphibious() ) {
		owner->StartSound( "snd_engine_water_idle", SND_VEHICLE_DRIVE, 0, NULL );
		owner->StartSound( "snd_engine_water_drive", SND_VEHICLE_DRIVE2, 0, NULL );
		owner->StartSound( "snd_engine_water_hardaccel", SND_VEHICLE_DRIVE3, 0, NULL );
		owner->FadeSound( SND_VEHICLE_DRIVE, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_DRIVE2, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_DRIVE3, -60.0f, 0.0f );

		owner->StartSound( "snd_engine_water_idle_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
		owner->StartSound( "snd_engine_water_drive_interior", SND_VEHICLE_INTERIOR_DRIVE2, 0, NULL );
		owner->StartSound( "snd_engine_water_hardaccel_interior", SND_VEHICLE_INTERIOR_DRIVE3, 0, NULL );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE3, -60.0f, 0.0f );
	}
}

/*
================
sdVehicleSoundControl_Wheeled::ExitWater
================
*/
void sdVehicleSoundControl_Wheeled::ExitWater( float absSpeedKPH ) {
	if ( !soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = false;

	owner->StartSound( absSpeedKPH > 30.f ? "snd_water_exit" : "snd_water_splash", SND_VEHICLE_MISC, 0, NULL );

	if ( owner->IsAmphibious() ) {
		owner->StartSound( "snd_engine_idle", SND_VEHICLE_DRIVE, 0, NULL );
		owner->StartSound( "snd_engine_drive", SND_VEHICLE_DRIVE2, 0, NULL );
		owner->StartSound( "snd_engine_hardaccel", SND_VEHICLE_DRIVE3, 0, NULL );
		owner->FadeSound( SND_VEHICLE_DRIVE, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_DRIVE2, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_DRIVE3, -60.0f, 0.0f );

		owner->StartSound( "snd_engine_idle_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
		owner->StartSound( "snd_engine_drive_interior", SND_VEHICLE_INTERIOR_DRIVE2, 0, NULL );
		owner->StartSound( "snd_engine_hardaccel_interior", SND_VEHICLE_INTERIOR_DRIVE3, 0, NULL );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, -60.0f, 0.0f );
		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE3, -60.0f, 0.0f );
	}
}

/*
================
sdVehicleSoundControl_Wheeled::StartSkidSound
================
*/
void sdVehicleSoundControl_Wheeled::StartSkidSound( bool force ) {
	if ( !force && soundFlags.playingSkidSound ) {
		return;
	}

	if ( !soundFlags.playingSkidSound ) {
		owner->FadeSound( SND_VEHICLE_DRIVE, -30.f, 0.05f );
	}

	soundFlags.playingSkidSound = true;

	idStr snd( va( "snd_skid%s", groundSurfaceType.c_str() ) );
	if ( !idStr::Cmp( owner->spawnArgs.GetString( snd.c_str() ), "" ) ) {
		snd = "snd_skid";
	}

	owner->StartSound( snd.c_str(), SND_VEHICLE_SKID, 0, NULL );
}

/*
================
sdVehicleSoundControl_Wheeled::StopSkidSound
================
*/
void sdVehicleSoundControl_Wheeled::StopSkidSound( void ) {
	if ( !soundFlags.playingSkidSound ) {
		return;
	}
	soundFlags.playingSkidSound = false;

	owner->FadeSound( SND_VEHICLE_DRIVE, 0.f, 0.05f );
}

/*
================
sdVehicleSoundControl_Wheeled::StartDriveSound
================
*/
void sdVehicleSoundControl_Wheeled::StartDriveSound( void ) {
	if ( soundFlags.playingDriveSound ) {
		return;
	}
	soundFlags.playingDriveSound = true;

	StartEngineSounds();
}

/*
================
sdVehicleSoundControl_Wheeled::StopDriveSound
================
*/
void sdVehicleSoundControl_Wheeled::StopDriveSound( void ) {
	if ( !soundFlags.playingDriveSound ) {
		return;
	}
	soundFlags.playingDriveSound = false;
	StopEngineSounds();
}

/*
===============================================================================

	sdVehicleSoundControl_Tracked

===============================================================================
*/

/*
================
sdVehicleSoundControl_Tracked::sdVehicleSoundControl_Tracked
================
*/
sdVehicleSoundControl_Tracked::sdVehicleSoundControl_Tracked( void ) {
	soundFlags.playingOffRoadSound	= false;
	soundFlags.playingDriveSound	= false;
	soundFlags.inWater				= false;
}

/*
================
sdVehicleSoundControl_Tracked::Update
================
*/
void sdVehicleSoundControl_Tracked::Update( void ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	const sdVehicleInput& input = owner->GetInput();

	if ( driver != NULL ) {
		const usercmd_t& cmd = gameLocal.usercmds[ driver->entityNumber ];

		if ( cmd.buttons.btn.modeSwitch ) {
			StartHornSound();
		} else {
			StopHornSound();
		}
	} else {
		StopHornSound();
	}

	soundParmsAdvanced_t parms;
	CalcSoundParmsAdvanced( parms );
	UpdateEngineSounds( parms );

	if ( parms.simple.inWater ) {
		EnterWater( parms.simple.absSpeedKPH );

		owner->SetChannelVolume( SND_VEHICLE_OFFROAD, parms.simple.newSoundLevel );
	} else {
		ExitWater( parms.simple.absSpeedKPH );

		if ( parms.simple.absSpeedKPH > 1.f ) {
			StartOffRoadSound();

			float newTrackPitch = ( parms.simple.absSpeedKPH * 0.005f ) + 0.7f;
			owner->SetChannelPitchShift( SND_VEHICLE_INTERIOR_OFFROAD, newTrackPitch );
			owner->SetChannelPitchShift( SND_VEHICLE_OFFROAD, newTrackPitch );

			if ( groundIsOffRoad ) {
				owner->FadeSound( SND_VEHICLE_INTERIOR_OFFROAD, parms.simple.newSoundLevel - 10.f, 0.1f );
				owner->FadeSound( SND_VEHICLE_OFFROAD, parms.simple.newSoundLevel - 2.f, 0.1f );
			} else {
				owner->FadeSound( SND_VEHICLE_INTERIOR_OFFROAD, parms.simple.newSoundLevel - 8.f, 0.1f );
				owner->FadeSound( SND_VEHICLE_OFFROAD, parms.simple.newSoundLevel, 0.1f );
			}
		} else {
			StopOffRoadSound();
		}
	}

	if ( !owner->IsEMPed() && !parms.simple.submerged && driver != NULL ) {
		StartDriveSound();
	} else {
		StopDriveSound();
	}
}

/*
================
sdVehicleSoundControl_Tracked::OnPlayerEntered
================
*/
void sdVehicleSoundControl_Tracked::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	if ( position == 0 && !owner->IsEMPed() ) {
		owner->StartSound( "snd_engine_start", SND_VEHICLE_START, 0, NULL );
		owner->StartSound( "snd_engine_start_interior", SND_VEHICLE_INTERIOR_START, 0, NULL );
	}

	StartCockpitSound();
}

/*
================
sdVehicleSoundControl_Tracked::OnPlayerExited
================
*/
void sdVehicleSoundControl_Tracked::OnPlayerExited( idPlayer* player, int position ) {
	if ( position == 0 && !owner->IsEMPed() ) {
		owner->StartSound( "snd_engine_stop", SND_VEHICLE_STOP, 0, NULL );
		owner->StartSound( "snd_engine_stop_interior", SND_VEHICLE_INTERIOR_STOP, 0, NULL );
	}

	if ( owner->GetPositionManager().IsEmpty() ) {
		StopCockpitSound();
	}
}

/*
================
sdVehicleSoundControl_Tracked::OnSurfaceTypeChanged
================
*/
void sdVehicleSoundControl_Tracked::OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType ) {
	sdVehicleSoundControl_Simple::OnSurfaceTypeChanged( surfaceType );
}

/*
================
sdVehicleSoundControl_Tracked::OnEMPStateChanged
================
*/
void sdVehicleSoundControl_Tracked::OnEMPStateChanged( void ) {
//	sdVehicleSoundControl_Simple::OnEMPStateChanged();
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( !owner->IsEMPed() ) {
		if ( driver != NULL ) {
			owner->StartSound( "snd_engine_start", SND_VEHICLE_START, 0, NULL );
			owner->StartSound( "snd_engine_start_interior", SND_VEHICLE_INTERIOR_START, 0, NULL );
		}
	} else {
		if ( driver != NULL ) {
			owner->StartSound( "snd_engine_stop", SND_VEHICLE_STOP, 0, NULL );
			owner->StartSound( "snd_engine_stop_interior", SND_VEHICLE_INTERIOR_STOP, 0, NULL );
		}
	}
}

/*
================
sdVehicleSoundControl_Tracked::OnWeaponEMPStateChanged
================
*/
void sdVehicleSoundControl_Tracked::OnWeaponEMPStateChanged( void ) {
	sdVehicleSoundControl_Simple::OnWeaponEMPStateChanged();
}

/*
================
sdVehicleSoundControl_Tracked::Init
================
*/
void sdVehicleSoundControl_Tracked::Init( sdTransport* transport ) {
	sdVehicleSoundControl_CrossFade::Init( transport );
}

/*
================
sdVehicleSoundControl_Tracked::StartOffRoadSound
================
*/
void sdVehicleSoundControl_Tracked::StartOffRoadSound( void ) {
	if ( soundFlags.playingOffRoadSound ) {
		return;
	}
	soundFlags.playingOffRoadSound = true;

	owner->StartSound( "snd_tracks_interior", SND_VEHICLE_INTERIOR_OFFROAD, 0, NULL );
	owner->SetChannelVolume( SND_VEHICLE_INTERIOR_OFFROAD, -60.f );
	owner->StartSound( "snd_tracks", SND_VEHICLE_OFFROAD, 0, NULL );
	owner->SetChannelVolume( SND_VEHICLE_OFFROAD, -60.f );
}

/*
================
sdVehicleSoundControl_Tracked::StopOffRoadSound
================
*/
void sdVehicleSoundControl_Tracked::StopOffRoadSound( void ) {
	if ( !soundFlags.playingOffRoadSound ) {
		return;
	}
	soundFlags.playingOffRoadSound = false;

	owner->StopSound( SND_VEHICLE_INTERIOR_OFFROAD );
	owner->StopSound( SND_VEHICLE_OFFROAD );
}

/*
================
sdVehicleSoundControl_Tracked::EnterWater
================
*/
void sdVehicleSoundControl_Tracked::EnterWater( float absSpeedKPH ) {
	if ( soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = true;

	StopOffRoadSound();

	owner->StartSound( "snd_water_wake", SND_VEHICLE_OFFROAD, 0, NULL );
	owner->StartSound( absSpeedKPH > 10.f ? "snd_water_enter" : "snd_water_splash", SND_VEHICLE_MISC, 0, NULL );
}

/*
================
sdVehicleSoundControl_Tracked::ExitWater
================
*/
void sdVehicleSoundControl_Tracked::ExitWater( float absSpeedKPH ) {
	if ( !soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = false;

	owner->StartSound( absSpeedKPH > 10.f ? "snd_water_exit" : "snd_water_splash", SND_VEHICLE_MISC, 0, NULL );
}

/*
================
sdVehicleSoundControl_Tracked::StartDriveSound
================
*/
void sdVehicleSoundControl_Tracked::StartDriveSound( void ) {
	if ( soundFlags.playingDriveSound ) {
		return;
	}
	soundFlags.playingDriveSound = true;

	StartEngineSounds();
}

/*
================
sdVehicleSoundControl_Tracked::StopDriveSound
================
*/
void sdVehicleSoundControl_Tracked::StopDriveSound( void ) {
	if ( !soundFlags.playingDriveSound ) {
		return;
	}
	soundFlags.playingDriveSound = false;
	StopEngineSounds();
}

/*
===============================================================================

	sdVehicleSoundControl_Helicopter

===============================================================================
*/

/*
================
sdVehicleSoundControl_Helicopter::sdVehicleSoundControl_Helicopter
================
*/
sdVehicleSoundControl_Helicopter::sdVehicleSoundControl_Helicopter( void ) {
	soundFlags.playingRotorSound		= false;
	soundFlags.playingTurbineSound		= false;
	soundFlags.playingThrottleSound		= false;
	soundFlags.playingTailRotorSound	= false;
}

/*
================
sdVehicleSoundControl_Helicopter::Update
================
*/
void sdVehicleSoundControl_Helicopter::Update( void ) {
	soundParms_t parms;
	CalcSoundParms( parms );

	owner->SetChannelPitchShift( SND_VEHICLE_INTERIOR_IDLE, parms.newSoundPitch );
	owner->SetChannelPitchShift( SND_VEHICLE_IDLE, parms.newSoundPitch );

	const sdVehicleInput& input = owner->GetInput();

	if ( soundFlags.playingThrottleSound ) {
		float newLevel = ( input.GetCollective() * 30.f ) - 10.f;
		if ( newLevel > 1.f ) {
			newLevel = 1.f;
		}

		owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, newLevel, 0.1f );
		owner->FadeSound( SND_VEHICLE_DRIVE2, newLevel, 0.1f );
	}

	if ( input.GetRight() != 0.f ) {
		FadeTailRotorIn();
	} else {
		FadeTailRotorOut();
	}

	if ( parms.submerged ) {
		EnterWater();
	} else {
		ExitWater();
	}
}

/*
================
sdVehicleSoundControl_Helicopter::OnPlayerEntered
================
*/
void sdVehicleSoundControl_Helicopter::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	if ( position == 0 ) {
		if ( !owner->IsEMPed() ) {
			StartTurbineSound();
			StartRotorSound();
			StartThrottleSound();
		}
	}	
}

/*
================
sdVehicleSoundControl_Helicopter::OnPlayerExited
================
*/
void sdVehicleSoundControl_Helicopter::OnPlayerExited( idPlayer* player, int position ) {
	if ( position == 0 ) {
		if ( !owner->IsEMPed() ) {
			StopTurbineSound();
			StopRotorSound();
			StopThrottleSound();
		}
	}
}

/*
================
sdVehicleSoundControl_Helicopter::OnSurfaceTypeChanged
================
*/
void sdVehicleSoundControl_Helicopter::OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType ) {
	sdVehicleSoundControl_Simple::OnSurfaceTypeChanged( surfaceType );
}

/*
================
sdVehicleSoundControl_Helicopter::OnEMPStateChanged
================
*/
void sdVehicleSoundControl_Helicopter::OnEMPStateChanged( void ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( !owner->IsEMPed() ) {
		if ( driver != NULL ) {
			StartTurbineSound();
			StartRotorSound();
			StartThrottleSound();
		}
	} else {
		if ( driver != NULL ) {
			StopTurbineSound();
			StopRotorSound();
			StopThrottleSound();
		}
	}
}

/*
================
sdVehicleSoundControl_Helicopter::OnWeaponEMPStateChanged
================
*/
void sdVehicleSoundControl_Helicopter::OnWeaponEMPStateChanged( void ) {
	sdVehicleSoundControl_Simple::OnWeaponEMPStateChanged();
}

/*
================
sdVehicleSoundControl_Helicopter::Init
================
*/
void sdVehicleSoundControl_Helicopter::Init( sdTransport* transport ) {
	sdVehicleSoundControl_Simple::Init( transport );

	owner->StartSound( "snd_cockpit", SND_VEHICLE_INTERIOR, 0, NULL );
	owner->StartSound( "snd_rotor_tail", SND_VEHICLE_DRIVE3, 0, NULL );
}

/*
================
sdVehicleSoundControl_Helicopter::StartTurbineSound
================
*/
void sdVehicleSoundControl_Helicopter::StartTurbineSound( void ) {
	if ( soundFlags.playingTurbineSound ) {
		return;
	}
	soundFlags.playingTurbineSound = true;

	owner->StartSound( "snd_turbine_start_interior", SND_VEHICLE_INTERIOR_IDLE, 0, NULL );
	owner->StartSound( "snd_turbine_start", SND_VEHICLE_IDLE, 0, NULL );
}

/*
================
sdVehicleSoundControl_Helicopter::StopTurbineSound
================
*/
void sdVehicleSoundControl_Helicopter::StopTurbineSound( void ) {
	if ( !soundFlags.playingTurbineSound ) {
		return;
	}
	soundFlags.playingTurbineSound = false;

	owner->StartSound( "snd_turbine_stop_interior", SND_VEHICLE_INTERIOR_IDLE, 0, NULL );
	owner->StartSound( "snd_turbine_stop", SND_VEHICLE_IDLE, 0, NULL );
}

/*
================
sdVehicleSoundControl_Helicopter::StartRotorSound
================
*/
void sdVehicleSoundControl_Helicopter::StartRotorSound( void ) {
	if ( soundFlags.playingRotorSound ) {
		return;
	}
	soundFlags.playingRotorSound = true;

	owner->StartSound( "snd_rotor_start_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
	owner->StartSound( "snd_rotor_start", SND_VEHICLE_DRIVE, 0, NULL );
}

/*
================
sdVehicleSoundControl_Helicopter::StopRotorSound
================
*/
void sdVehicleSoundControl_Helicopter::StopRotorSound( void ) {
	if ( !soundFlags.playingRotorSound ) {
		return;
	}
	soundFlags.playingRotorSound = false;

	owner->StartSound( "snd_rotor_stop_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
	owner->StartSound( "snd_rotor_stop", SND_VEHICLE_DRIVE, 0, NULL );
}

/*
================
sdVehicleSoundControl_Helicopter::StartThrottleSound
================
*/
void sdVehicleSoundControl_Helicopter::StartThrottleSound( void ) {
	if ( soundFlags.playingThrottleSound ) {
		return;
	}
	soundFlags.playingThrottleSound = true;

	owner->StartSound( "snd_rotor_throttle_interior", SND_VEHICLE_INTERIOR_DRIVE2, 0, NULL );
	owner->StartSound( "snd_rotor_throttle", SND_VEHICLE_DRIVE2, 0, NULL );
}

/*
================
sdVehicleSoundControl_Helicopter::StopThrottleSound
================
*/
void sdVehicleSoundControl_Helicopter::StopThrottleSound( void ) {
	if ( !soundFlags.playingThrottleSound ) {
		return;
	}
	soundFlags.playingThrottleSound = false;

	owner->StopSound( SND_VEHICLE_INTERIOR_DRIVE2 );
	owner->StopSound( SND_VEHICLE_DRIVE2 );
}

/*
================
sdVehicleSoundControl_Helicopter::FadeTailRotorIn
================
*/
void sdVehicleSoundControl_Helicopter::FadeTailRotorIn( void ) {
	if ( soundFlags.playingTailRotorSound ) {
		return;
	}
	soundFlags.playingTailRotorSound = true;

	owner->FadeSound( SND_VEHICLE_DRIVE3, 0.f, 0.5f );
}

/*
================
sdVehicleSoundControl_Helicopter::FadeTailRotorOut
================
*/
void sdVehicleSoundControl_Helicopter::FadeTailRotorOut( void ) {
	if ( !soundFlags.playingTailRotorSound ) {
		return;
	}
	soundFlags.playingTailRotorSound = false;

	owner->FadeSound( SND_VEHICLE_DRIVE3, -60.f, 1.f );
}

/*
================
sdVehicleSoundControl_Helicopter::EnterWater
================
*/
void sdVehicleSoundControl_Helicopter::EnterWater( void ) {
	if ( soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = true;

	owner->StartSound( "snd_water_enter", SND_VEHICLE_MISC, 0, NULL );
}

/*
================
sdVehicleSoundControl_Helicopter::ExitWater
================
*/
void sdVehicleSoundControl_Helicopter::ExitWater( void ) {
	if ( !soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = false;
}



/*
===============================================================================

	sdVehicleSoundControl_JetPack

===============================================================================
*/

/*
================
sdVehicleSoundControl_JetPack::sdVehicleSoundControl_JetPack
================
*/
sdVehicleSoundControl_JetPack::sdVehicleSoundControl_JetPack( void ) {
	nextJetStartSoundTime		= 0;

	soundFlags.playingJetSound	= false;
	soundFlags.playingIdleSound	= false;
}

/*
================
sdVehicleSoundControl_JetPack::Update
================
*/
void sdVehicleSoundControl_JetPack::Update( void ) {
	const idVec3& upAxis = owner->GetAxis()[ 2 ];
	const idVec3& velocity = owner->GetPhysics()->GetLinearVelocity();
	float absSpeedKPH = ( velocity - ( velocity*upAxis )*upAxis ).Length();

	if ( soundFlags.playingIdleSound ) {
		float idealNewPitch = ( absSpeedKPH * soundSpeedMultiplier ) + soundSpeedOffset;
		if ( idealNewPitch > soundPitchMax ) {
			idealNewPitch = soundPitchMax;
		}

		float newPitch = Lerp( lastSoundPitch, idealNewPitch, soundRampRate );
		owner->SetChannelPitchShift( SND_VEHICLE_IDLE, newPitch );
		lastSoundPitch = newPitch;
	}

	const sdVehicleInput& input = owner->GetInput();
	
	bool playJetSound = false;

	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( driver && !owner->IsEMPed() ) {
		if ( gameLocal.usercmds[ driver->entityNumber ].buttons.btn.sprint ) {
			if ( jetPack->GetChargeFraction() > 0.f ) {
				playJetSound = true;
			}
		}
	}

	if ( playJetSound ) {
		StartJetSound();
	} else {
		StopJetSound();
	}

	// play jumped sound
	if ( jetPack->GetJetPackPhysics()->HasJumped() ) {
		owner->StartSound( "snd_jump", SND_VEHICLE_JUMP, 0, NULL );
	}
}

/*
================
sdVehicleSoundControl_JetPack::OnPlayerEntered
================
*/
void sdVehicleSoundControl_JetPack::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	if ( position == 0 && !owner->IsEMPed() ) {
		StartIdleSound();
	}
}

/*
================
sdVehicleSoundControl_JetPack::OnPlayerExited
================
*/
void sdVehicleSoundControl_JetPack::OnPlayerExited( idPlayer* player, int position ) {
	if ( position == 0 && !owner->IsEMPed() ) {
		StopIdleSound();
	}
}

/*
================
sdVehicleSoundControl_JetPack::OnSurfaceTypeChanged
================
*/
void sdVehicleSoundControl_JetPack::OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType ) {
}

/*
================
sdVehicleSoundControl_JetPack::OnEMPStateChanged
================
*/
void sdVehicleSoundControl_JetPack::OnEMPStateChanged( void ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( !owner->IsEMPed() ) {
		if ( driver != NULL ) {
			StartIdleSound();
		}
	} else {
		if ( driver != NULL ) {
			StopIdleSound();
		}
	}
}

/*
================
sdVehicleSoundControl_JetPack::OnWeaponEMPStateChanged
================
*/
void sdVehicleSoundControl_JetPack::OnWeaponEMPStateChanged( void ) {
}

/*
================
sdVehicleSoundControl_JetPack::Init
================
*/
void sdVehicleSoundControl_JetPack::Init( sdTransport* transport ) {
	sdVehicleSoundControlBase::Init( transport );

	jetPack = transport->Cast< sdJetPack >();
	if ( jetPack == NULL ) {
		gameLocal.Error( "sdVehicleSoundControl_JetPack::Init Tried To Attach To a Non JetPack Entity" );
	}

	soundSpeedMultiplier	= transport->spawnArgs.GetFloat( "sound_speed_multiplier", "0.005" );
	soundSpeedOffset		= transport->spawnArgs.GetFloat( "sound_speed_offset", "1" );
	soundPitchMax			= transport->spawnArgs.GetFloat( "sound_pitch_max", "2.5" );
	soundRampRate			= transport->spawnArgs.GetFloat( "sound_ramp_rate", "0.1" );

	lastSoundPitch			= soundSpeedOffset;
}

/*
================
sdVehicleSoundControl_JetPack::StartJetSound
================
*/
void sdVehicleSoundControl_JetPack::StartJetSound( void ) {
	if ( soundFlags.playingJetSound ) {
		return;
	}
	soundFlags.playingJetSound = true;
	
	if ( !owner->IsEMPed() ) {
		owner->StartSound( "snd_jet", SND_VEHICLE_DRIVE, 0, NULL );

		if ( nextJetStartSoundTime < gameLocal.time ) {
			owner->StartSound( "snd_jet_start", SND_VEHICLE_DRIVE2, 0, NULL );
			nextJetStartSoundTime = gameLocal.time + 950;

			owner->FadeSound( SND_VEHICLE_DRIVE2, -15.0f, 0.0f );
			owner->FadeSound( SND_VEHICLE_DRIVE2, 0.0f, 0.05f );

			owner->FadeSound( SND_VEHICLE_DRIVE, -30.0f, 0.0f );
			owner->FadeSound( SND_VEHICLE_DRIVE, 0.0f, 0.5f );
		}
	}
}

/*
================
sdVehicleSoundControl_JetPack::StopJetSound
================
*/
void sdVehicleSoundControl_JetPack::StopJetSound( void ) {
	if ( !soundFlags.playingJetSound ) {
		return;
	}
	soundFlags.playingJetSound = false;
	
	owner->FadeSound( SND_VEHICLE_DRIVE, -60.0f, 2.0f );
	owner->FadeSound( SND_VEHICLE_DRIVE2, -60.0f, 2.0f );
}

/*
================
sdVehicleSoundControl_JetPack::StartIdleSound
================
*/
void sdVehicleSoundControl_JetPack::StartIdleSound( void ) {
	if ( soundFlags.playingIdleSound ) {
		return;
	}
	soundFlags.playingIdleSound = true;

	if ( !owner->IsEMPed() ) {
		owner->StartSound( "snd_idle", SND_VEHICLE_IDLE, 0, NULL );
	}
}

/*
================
sdVehicleSoundControl_JetPack::StopIdleSound
================
*/
void sdVehicleSoundControl_JetPack::StopIdleSound( void ) {
	if ( !soundFlags.playingIdleSound ) {
		return;
	}
	soundFlags.playingIdleSound = false;

	owner->StopSound( SND_VEHICLE_IDLE );
}





/*
===============================================================================

	sdVehicleSoundControl_SpeedBoat

===============================================================================
*/

/*
================
sdVehicleSoundControl_SpeedBoat::sdVehicleSoundControl_SpeedBoat
================
*/
sdVehicleSoundControl_SpeedBoat::sdVehicleSoundControl_SpeedBoat( void ) {
	soundFlags.playingDriveSound	= false;
	soundFlags.inWater				= false;
}

/*
================
sdVehicleSoundControl_SpeedBoat::Update
================
*/
void sdVehicleSoundControl_SpeedBoat::Update( void ) {
	soundParmsAdvanced_t parms;
	CalcSoundParmsAdvanced( parms );
	UpdateEngineSounds( parms );

	const sdVehicleInput& input = owner->GetInput();

	if ( !owner->IsEMPed() && !parms.simple.submerged && input.GetPlayer() != NULL ) {
		StartDriveSound();
	} else {
		StopDriveSound();
	}

	if ( parms.simple.inWater ) {
		EnterWater();

		owner->SetChannelVolume( SND_VEHICLE_DRIVE4, parms.simple.newSoundLevel );
	} else {
		ExitWater();
	}
}

/*
================
sdVehicleSoundControl_SpeedBoat::OnPlayerEntered
================
*/
void sdVehicleSoundControl_SpeedBoat::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	if ( position == 0 && !owner->IsEMPed() ) {
		owner->StartSound( "snd_engine_start", SND_VEHICLE_START, 0, NULL );
	}
}

/*
================
sdVehicleSoundControl_SpeedBoat::OnPlayerExited
================
*/
void sdVehicleSoundControl_SpeedBoat::OnPlayerExited( idPlayer* player, int position ) {
	if ( position == 0 && !owner->IsEMPed() ) {
		owner->StartSound( "snd_engine_stop", SND_VEHICLE_STOP, 0, NULL );
	}
}

/*
================
sdVehicleSoundControl_SpeedBoat::OnSurfaceTypeChanged
================
*/
void sdVehicleSoundControl_SpeedBoat::OnSurfaceTypeChanged( const sdDeclSurfaceType* surfaceType ) {
	sdVehicleSoundControl_CrossFade::OnSurfaceTypeChanged( surfaceType );
}

/*
================
sdVehicleSoundControl_SpeedBoat::OnEMPStateChanged
================
*/
void sdVehicleSoundControl_SpeedBoat::OnEMPStateChanged( void ) {
	sdVehicleSoundControl_CrossFade::OnEMPStateChanged();
}

/*
================
sdVehicleSoundControl_SpeedBoat::OnWeaponEMPStateChanged
================
*/
void sdVehicleSoundControl_SpeedBoat::OnWeaponEMPStateChanged( void ) {
	sdVehicleSoundControl_CrossFade::OnEMPStateChanged();
}

/*
================
sdVehicleSoundControl_SpeedBoat::Init
================
*/
void sdVehicleSoundControl_SpeedBoat::Init( sdTransport* transport ) {
	sdVehicleSoundControl_CrossFade::Init( transport );
}

/*
================
sdVehicleSoundControl_SpeedBoat::StartDriveSound
================
*/
void sdVehicleSoundControl_SpeedBoat::StartDriveSound( void ) {
	if ( soundFlags.playingDriveSound ) {
		return;
	}
	soundFlags.playingDriveSound = true;

	owner->StartSound( "snd_engine_water_idle", SND_VEHICLE_DRIVE, 0, NULL );
	owner->StartSound( "snd_engine_water_drive", SND_VEHICLE_DRIVE2, 0, NULL );
	owner->StartSound( "snd_engine_water_hardaccel", SND_VEHICLE_DRIVE3, 0, NULL );
	owner->FadeSound( SND_VEHICLE_DRIVE, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_DRIVE2, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_DRIVE3, -60.0f, 0.0f );

	owner->StartSound( "snd_engine_water_idle_interior", SND_VEHICLE_INTERIOR_DRIVE, 0, NULL );
	owner->StartSound( "snd_engine_water_drive_interior", SND_VEHICLE_INTERIOR_DRIVE2, 0, NULL );
	owner->StartSound( "snd_engine_water_hardaccel_interior", SND_VEHICLE_INTERIOR_DRIVE3, 0, NULL );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, -60.0f, 0.0f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE3, -60.0f, 0.0f );


	engineSpeed = 0.0f;
	lastVolumeIncreaseValue = 0.0f;
}

/*
================
sdVehicleSoundControl_SpeedBoat::StopDriveSound
================
*/
void sdVehicleSoundControl_SpeedBoat::StopDriveSound( void ) {
	if ( !soundFlags.playingDriveSound ) {
		return;
	}

	soundFlags.playingDriveSound = false;

	owner->FadeSound( SND_VEHICLE_DRIVE, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_DRIVE2, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_DRIVE3, -60.0f, 0.3f );

	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE2, -60.0f, 0.3f );
	owner->FadeSound( SND_VEHICLE_INTERIOR_DRIVE3, -60.0f, 0.3f );

	soundKillTime = gameLocal.time + 300;
}

/*
================
sdVehicleSoundControl_SpeedBoat::EnterWater
================
*/
void sdVehicleSoundControl_SpeedBoat::EnterWater( void ) {
	if ( soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = true;

	owner->StartSound( "snd_water_splash", SND_VEHICLE_MISC, 0, NULL );
	owner->StartSound( "snd_water_wake", SND_VEHICLE_DRIVE4, 0, NULL );
}

/*
================
sdVehicleSoundControl_SpeedBoat::ExitWater
================
*/
void sdVehicleSoundControl_SpeedBoat::ExitWater( void ) {
	if ( !soundFlags.inWater ) {
		return;
	}
	soundFlags.inWater = false;

	owner->StartSound( "snd_water_splash", SND_VEHICLE_MISC, 0, NULL );
	owner->StopSound( SND_VEHICLE_DRIVE4 );
}
