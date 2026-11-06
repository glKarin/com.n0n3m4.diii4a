//----------------------------------------------------------------
// VehicleParts.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Projectile.h"
#include "../Effect.h"
#include "Vehicle.h"
#include "VehicleParts.h"
#include "../ai/AI_Manager.h"

/***********************************************************************

							rvVehiclePart

***********************************************************************/

ABSTRACT_DECLARATION( idClass, rvVehiclePart )
END_CLASS

/*
=====================
rvVehiclePart::Init
=====================
*/
bool rvVehiclePart::Init ( rvVehiclePosition* _position, const idDict& args, s_channelType _soundChannel ) {
	spawnArgs.Copy ( args );
	
	soundChannel	 = _soundChannel;
	position		 = _position;
	parent			 = position->GetParent();
	fl.active		 = false;
	fl.useCenterMass = false;
	
	return true;
}

/*
=====================
rvVehiclePart::Spawn
=====================
*/
void rvVehiclePart::Spawn ( void ) {
	// Get joint for orienting the part
	joint = parent->GetAnimator()->GetJointHandle ( spawnArgs.GetString ( "joint", "" ) );

	// More position information
	fl.useCenterMass = spawnArgs.GetBool ( "useCenterMass", "0" );
	spawnArgs.GetVector ( "offset", "0 0 0", localOffset );

	fl.active = false;
	
	fl.useViewAxis	= spawnArgs.GetBool( "useViewAxis", "0" );

	// Initialize the origin so we can determine which side of the vehicle we are on
	UpdateOrigin ( );
	
	// Determine if this part is on the left and/or front side of the vehicle
	idVec3 localOrigin;
	localOrigin = (worldOrigin - parent->GetPhysics()->GetCenterMass());
	fl.front = (localOrigin * parent->GetPhysics()->GetAxis()[0] < 0.0f); 
	fl.left  = (localOrigin * parent->GetPhysics()->GetAxis()[1] < 0.0f); 
}

/*
================
rvVehiclePart::UpdateOrigin
================
*/
void rvVehiclePart::UpdateOrigin ( void ) {	
	if ( joint != INVALID_JOINT ) {
		parent->GetJointWorldTransform ( joint, gameLocal.time, worldOrigin, worldAxis );
	} else {
		if ( fl.useViewAxis ) {
			worldAxis	= parent->viewAxis;
		} else {
			worldAxis   = parent->GetPhysics()->GetAxis();
		}
		worldOrigin = fl.useCenterMass ? parent->GetPhysics()->GetCenterMass() : parent->GetPhysics()->GetOrigin();
	}			
	
	worldOrigin += (localOffset * worldAxis);
	
	// Include this part in the total bounds 
	// FIXME: bounds are local
	parent->AddToBounds ( worldOrigin );
}

/*
================
rvVehiclePart::Save
================
*/
void rvVehiclePart::Save ( idSaveGame* savefile ) const {
	savefile->Write( &fl, sizeof( fl ) );

	savefile->WriteDict ( &spawnArgs );
	parent.Save ( savefile );
	savefile->WriteJoint ( joint );
	savefile->WriteInt ( soundChannel );
	savefile->WriteInt ( parent->GetPositionIndex ( position ) );
	
	savefile->WriteVec3 ( worldOrigin );
	savefile->WriteMat3 ( worldAxis );
	savefile->WriteVec3 ( localOffset );
}

/*
================
rvVehiclePart::Restore
================
*/
void rvVehiclePart::Restore	( idRestoreGame* savefile ) {
	int temp;
	
	savefile->Read( &fl, sizeof( fl ) );

	savefile->ReadDict ( &spawnArgs );
	parent.Restore ( savefile );
	savefile->ReadJoint ( joint );
	savefile->ReadInt ( soundChannel );

	savefile->ReadInt ( temp );
	position = parent->GetPosition ( temp );
	
	savefile->ReadVec3 ( worldOrigin );
	savefile->ReadMat3 ( worldAxis );
	savefile->ReadVec3 ( localOffset );
}

/***********************************************************************

							rvVehicleSound

***********************************************************************/

CLASS_DECLARATION( rvVehiclePart, rvVehicleSound )
END_CLASS

/*
=====================
rvVehicleSound::rvVehicleSound
=====================
*/
rvVehicleSound::rvVehicleSound ( void ) {
	memset( &refSound, 0, sizeof( refSound ) );
	refSound.referenceSoundHandle = -1;
	fade = false;
	autoActivate = true;
}

/*
=====================
rvVehicleSound::~rvVehicleSound
=====================
*/
rvVehicleSound::~rvVehicleSound ( void ) {
	Stop();
	soundSystem->FreeSoundEmitter( SOUNDWORLD_GAME, refSound.referenceSoundHandle, true );
	refSound.referenceSoundHandle = -1;
}

/*
=====================
rvVehiclePart::Spawn
=====================
*/
void rvVehicleSound::Spawn ( void ) {
	soundName = spawnArgs.GetString ( "snd_loop" );
	
	spawnArgs.GetVec2 ( "freqShift", "1 1", freqShift );
	spawnArgs.GetVec2 ( "volume", "0 0", volume );
	
	// Temp fix for volume
	volume[0] = idMath::dBToScale( volume[0] );
	volume[1] = idMath::dBToScale( volume[1] );

	declManager->FindSound ( soundName )->GetParms ( &refSound.parms );
}

/*
=====================
rvVehicleSound::RunPostPhysics
=====================
*/
void rvVehicleSound::RunPostPhysics ( void ) {
	Update ( );
}

/*
=====================
rvVehicleSound::Activate
=====================
*/
void rvVehicleSound::Activate ( bool active ) {
	rvVehiclePart::Activate ( active );

	if ( autoActivate ) {
		if ( active ) {
			Play ( );
		} else {
			Stop ( );
		}
	}
}

/*
=====================
rvVehicleSound::Play
=====================
*/
void rvVehicleSound::Play ( void ) {
	if ( !soundName.Length ( ) ) {
		return;
	}

	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( !emitter )  {
		refSound.referenceSoundHandle = soundSystem->AllocSoundEmitter( SOUNDWORLD_GAME );
	}

	Attenuate ( 0.0f, 0.0f );

	Update ( true );

	emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if( emitter ) {
		emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
		emitter->StartSound ( declManager->FindSound( soundName ), soundChannel, 0, 0 );
	}
}

/*
=====================
rvVehicleSound::Stop
=====================
*/
void rvVehicleSound::Stop ( void ) {
	if ( IsPlaying ( ) ) {
		idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
		if( emitter ) {
			emitter->StopSound( soundChannel );
		}
	}
}

/*
=====================
rvVehicleSound::Update
=====================
*/
void rvVehicleSound::Update ( bool force ) {
	if ( !force && !IsPlaying ( ) ) {
		return;
	}

	if ( fade && currentVolume.IsDone ( gameLocal.time ) ) {
		Stop ( );
		return;
	}

	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if( !emitter ) {
		return;
	}

	UpdateOrigin ( );

	refSound.parms.volume		    = currentVolume.GetCurrentValue ( gameLocal.time );
	refSound.parms.frequencyShift = currentFreqShift.GetCurrentValue ( gameLocal.time );
	emitter->ModifySound ( soundChannel, &refSound.parms );
	
	refSound.origin = worldOrigin;
	// bdube: please put something sensible here if you want doppler from the sound system
	refSound.velocity = vec3_origin;
	emitter->UpdateEmitter( refSound.origin, refSound.velocity, parent->entityNumber + 1, &refSound.parms );
}

/*
=====================
rvVehicleSound::Attenuate
=====================
*/
void rvVehicleSound::Attenuate ( float volumeAttenuate, float freqAttenuate ) {
	float f;

	fade = false;
	
	f = volume[0] + (volume[1]-volume[0]) * volumeAttenuate;
	currentVolume.Init ( gameLocal.time, 100, currentVolume.GetCurrentValue(gameLocal.time), f );

	f = freqShift[0] + (freqShift[1]-freqShift[0]) * freqAttenuate;
	currentFreqShift.Init ( gameLocal.time, 100, currentFreqShift.GetCurrentValue(gameLocal.time), f );
}

/*
=====================
rvVehicleSound::Fade
=====================
*/
void rvVehicleSound::Fade ( int duration, float toVolume, float toFreq ) {
	currentVolume.Init ( gameLocal.time, duration, currentVolume.GetCurrentValue(gameLocal.time), toVolume );
	currentFreqShift.Init ( gameLocal.time, duration, currentFreqShift.GetCurrentValue(gameLocal.time), toFreq );
}

/*
=====================
rvVehicleSound::Save
=====================
*/
void rvVehicleSound::Save ( idSaveGame* savefile ) const {
	savefile->WriteVec2 ( volume );
	savefile->WriteVec2 ( freqShift );
	savefile->WriteString ( soundName );
	savefile->WriteRefSound ( refSound );
	
	savefile->WriteInterpolate ( currentVolume );
	savefile->WriteInterpolate ( currentFreqShift );
	
	savefile->WriteBool ( fade );
	savefile->WriteBool ( autoActivate );
}	

/*
=====================
rvVehicleSound::Restore
=====================
*/
void rvVehicleSound::Restore ( idRestoreGame* savefile ) {
	savefile->ReadVec2 ( volume );
	savefile->ReadVec2 ( freqShift );
	savefile->ReadString ( soundName );
	savefile->ReadRefSound ( refSound );
	
	savefile->ReadInterpolate ( currentVolume );
	savefile->ReadInterpolate ( currentFreqShift );

	savefile->ReadBool ( fade );
	savefile->ReadBool ( autoActivate );
}

//----------------------------------------------------------------
//
//						rvVehicleLight
//
//----------------------------------------------------------------

CLASS_DECLARATION( rvVehiclePart, rvVehicleLight )
END_CLASS

/*
=====================
rvVehicleLight::rvVehicleLight
=====================
*/
rvVehicleLight::rvVehicleLight ( void ) {
	lightHandle = -1;
}

/*
=====================
rvVehicleLight::~rvVehicleLight
=====================
*/
rvVehicleLight::~rvVehicleLight ( void ) {
	if ( lightHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightHandle );
		lightHandle = -1;
	}
}

/*
=====================
rvVehicleLight::Spawn
=====================
*/
void rvVehicleLight::Spawn ( void ) {
	idVec3		color;
	const char* temp;

	memset ( &renderLight, 0, sizeof(renderLight) );

	renderLight.shader	  = declManager->FindMaterial( spawnArgs.GetString ( "mtr_light", "lights/muzzleflash" ), false );
	renderLight.pointLight  = spawnArgs.GetBool( "pointlight", "1" );

	spawnArgs.GetVector( "color", "0 0 0", color );
	renderLight.shaderParms[ SHADERPARM_RED ]		= color[0];
	renderLight.shaderParms[ SHADERPARM_GREEN ]		= color[1];
	renderLight.shaderParms[ SHADERPARM_BLUE ]		= color[2];
	renderLight.shaderParms[ SHADERPARM_TIMESCALE ] = 1.0f;

// RAVEN BEGIN
// dluetscher: added detail levels to render lights
	renderLight.detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// RAVEN END

	renderLight.lightRadius[0] = renderLight.lightRadius[1] = 
		renderLight.lightRadius[2] = (float)spawnArgs.GetInt( "radius" );

	if ( !renderLight.pointLight ) {
		renderLight.target = spawnArgs.GetVector( "target" );
		renderLight.up	   = spawnArgs.GetVector( "up" );
		renderLight.right  = spawnArgs.GetVector( "right" );
		renderLight.end	   = spawnArgs.GetVector( "target" );;
	}

	// Hide flare surface if there is one
	temp = spawnArgs.GetString ( "flaresurface", "" );
	if ( temp && *temp ) {
		parent->ProcessEvent ( &EV_HideSurface, temp );
	}

	// Sounds shader when turning light
	spawnArgs.GetString ( "snd_on", "", soundOn );
	
	// Sound shader when turning light off
	spawnArgs.GetString ( "snd_off", "", soundOff);
	
	lightOn = spawnArgs.GetBool( "start_on", "1" );
	lightHandle = -1;
}

/*
=====================
rvVehicleLight::RunPostPhysics
=====================
*/
void rvVehicleLight::RunPostPhysics ( void ) {
	if ( lightHandle == -1 || !lightOn ) {
		return;
	}
	
	UpdateLightDef ( );
}

/*
=====================
rvVehicleLight::UpdateLightDef
=====================
*/
void rvVehicleLight::UpdateLightDef ( void ) {
	UpdateOrigin ( );
	
	renderLight.origin = worldOrigin;
	renderLight.axis   = worldAxis;	
	
	if ( lightHandle == -1 ) {
		return;
	}
	gameRenderWorld->UpdateLightDef( lightHandle, &renderLight );	
}

/*
=====================
rvVehicleLight::Activate
=====================
*/
void rvVehicleLight::Activate ( bool active ) {
	rvVehiclePart::Activate ( active );
	
	if ( active && lightOn ) {
		TurnOn ( );
	} else {
		TurnOff ( );
	}
}

/*
=====================
rvVehicleLight::TurnOff
=====================
*/
void rvVehicleLight::TurnOff ( void ) {
	const char* surface;

	if ( lightHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightHandle );
		lightHandle = -1;

		// Play off sound
		if ( soundOff.Length() ) {
			parent->StartSoundShader ( declManager->FindSound ( soundOff ), soundChannel, 0, false, NULL );
		}
	}
	
	// Hide flare surface if there is one
	surface = spawnArgs.GetString ( "flaresurface", "" );
	if ( surface && *surface ) {
		parent->ProcessEvent ( &EV_HideSurface, surface );
	}
}

/*
=====================
rvVehicleLight::TurnOn
=====================
*/
void rvVehicleLight::TurnOn ( void ) {
	const char* surface;

	if ( lightHandle == -1 ) {
		lightHandle = gameRenderWorld->AddLightDef( &renderLight );
	}

	// Play off sound
	if ( soundOn.Length() ) {
		parent->StartSoundShader ( declManager->FindSound ( soundOn ), soundChannel, 0, false, NULL );
	}

	// Show flare surface if there is one
	surface = spawnArgs.GetString ( "flaresurface", "" );
	if ( surface && *surface ) {
		parent->ProcessEvent ( &EV_ShowSurface, surface );
	}
	
	UpdateLightDef ( );
}

/*
=====================
rvVehicleLight::Impulse
=====================
*/
void rvVehicleLight::Impulse ( int impulse ) {
	switch ( impulse ) {
		case IMPULSE_50:
			if ( lightOn ) {
				lightOn = false;
				TurnOff ( );
			} else {
				lightOn = true;
				TurnOn ( );
			}
			break;
	}
}

/*
=====================
rvVehicleLight::Save
=====================
*/
void rvVehicleLight::Save ( idSaveGame* savefile ) const {
	savefile->WriteRenderLight ( renderLight );
	savefile->WriteInt ( lightHandle );
	savefile->WriteBool ( lightOn );
	savefile->WriteString ( soundOn );
	savefile->WriteString ( soundOff );
}

/*
=====================
rvVehicleLight::Restore
=====================
*/
void rvVehicleLight::Restore ( idRestoreGame* savefile ) {
	savefile->ReadRenderLight ( renderLight );
	savefile->ReadInt ( lightHandle );
	if ( lightHandle != -1 ) {
		//get the handle again as it's out of date after a restore!
		lightHandle = gameRenderWorld->AddLightDef( &renderLight );
	}

	savefile->ReadBool ( lightOn );
	savefile->ReadString ( soundOn );
	savefile->ReadString ( soundOff );
}

/***********************************************************************

							rvVehicleWeapon

***********************************************************************/

CLASS_DECLARATION( rvVehiclePart, rvVehicleWeapon )
END_CLASS

#define	WEAPON_DELAY_FIRE		500

/*
=====================
rvVehicleWeapon::rvVehicleWeapon
=====================
*/
rvVehicleWeapon::rvVehicleWeapon ( void ) {
#ifdef _XENON
	bestEnemy		= 0;
#endif
	nextFireTime	= 0;
	fireDelay		= 0;
	hitScanDef		= NULL;
	projectileDef	= NULL;
	animNum			= 0;
}

/*
=====================
rvVehicleWeapon::~rvVehicleWeapon
=====================
*/
rvVehicleWeapon::~rvVehicleWeapon ( void ) {
	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	StopTargetEffect( );
}

/*
=====================
rvVehicleWeapon::Spawn
=====================
*/
void rvVehicleWeapon::Spawn ( void ) {
	int		i;
	idStr	temp;
	idVec3	color;

#ifdef _XENON
	bestEnemy = 0;
#endif

	launchFromJoint = spawnArgs.GetBool ( "launchFromJoint", "0" );
	lockScanning = spawnArgs.GetBool ( "lockScanning", "0" );
	lastLockTime = 0;
	
	if ( spawnArgs.GetString ( "def_hitscan", "", temp ) ) {
		hitScanDef = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_hitscan" ) );
	} else {
		projectileDef = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_projectile" ) );
	}
	
	fireDelay		 	= SEC2MS ( spawnArgs.GetFloat ( "firedelay" ) );
	spread			 	= spawnArgs.GetFloat ( "spread" );
	jointIndex		 	= 0;
	count			 	= spawnArgs.GetInt ( "count", "1" );
	lockRange		 	= spawnArgs.GetFloat ( "lockrange", "0" );
	ammoPerCharge	 	= spawnArgs.GetInt ( "ammopercharge", "-1" );
	chargeTime		 	= SEC2MS ( spawnArgs.GetFloat ( "chargetime" ) );
	currentAmmo	 		= ammoPerCharge;
	muzzleFlashHandle	= -1;
	
	if( spawnArgs.GetString("anim", "", temp) && *temp ) {
		animNum = parent->GetAnimator()->GetAnim( temp );
		animChannel = spawnArgs.GetInt( "animChannel" );
	}

	spawnArgs.GetVector ( "force", "0 0 0", force );

	// Vehicle guns can have multiple joints to fire from.  They will be cycled through 
	// when firing.
	joints.Append ( joint );
	for ( i = 2; ; i ++ ) {
		jointHandle_t joint2;
		joint2 = parent->GetAnimator()->GetJointHandle ( spawnArgs.GetString ( va("joint%d", i ) ) );
		if ( joint2 == INVALID_JOINT ) {
			break;
		}
		
		joints.Append ( joint2 );
	}

	// Muzzle Flash
	memset ( &muzzleFlash, 0, sizeof(muzzleFlash) );

	spawnArgs.GetVector ( "flashOffset", "0 0 0", muzzleFlashOffset );
	muzzleFlash.shader	  = declManager->FindMaterial( spawnArgs.GetString ( "mtr_flashShader", "lights/muzzleflash" ), false );
	muzzleFlash.pointLight = spawnArgs.GetBool( "flashPointLight", "1" );
// RAVEN BEGIN
// dluetscher: added detail levels to render lights
	muzzleFlash.detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// RAVEN END

	spawnArgs.GetVector( "flashColor", "0 0 0", color );
	muzzleFlash.shaderParms[ SHADERPARM_RED ]		 = color[0];
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]	 = color[1];
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]		 = color[2];
	muzzleFlash.shaderParms[ SHADERPARM_TIMESCALE ] = 1.0f;

	muzzleFlash.lightRadius[0] = muzzleFlash.lightRadius[1] = muzzleFlash.lightRadius[2] =	(float)spawnArgs.GetInt( "flashRadius" );

	if ( !muzzleFlash.pointLight ) {
		muzzleFlash.target	= spawnArgs.GetVector( "flashTarget" );
		muzzleFlash.up		= spawnArgs.GetVector( "flashUp" );
		muzzleFlash.right	= spawnArgs.GetVector( "flashRight" );
		muzzleFlash.end		= spawnArgs.GetVector( "flashTarget" );
	}
	
	shaderFire = declManager->FindSound ( spawnArgs.GetString ( "snd_fire" ), false );
	shaderReload = declManager->FindSound ( spawnArgs.GetString ( "snd_reload" ), false );

	// get the brass def
	idStr name;
	brassDict = NULL;
	if ( spawnArgs.GetString( "def_ejectBrass", "", name ) && *name ) {
		brassDict = gameLocal.FindEntityDefDict( name, false );

		if ( !brassDict ) {
			gameLocal.Warning( "Unknown brass def '%s' for weapon on vehicle '%s'", name.c_str(), position->mParent->name.c_str() );
		}
	}

	spawnArgs.GetVector ( "ejectOffset", "0 0 0", brassEjectOffset );
	brassEjectJoint = parent->GetAnimator()->GetJointHandle( spawnArgs.GetString ( "joint_view_eject", "eject" ) );
	if ( brassDict ) {
		brassEjectDelay	= SEC2MS( brassDict->GetFloat( "delay", "0.01" ) );
	}
	brassEjectNext = 0;

	targetEnt = NULL;
	targetJoint = INVALID_JOINT;
	targetPos.Zero ();

	// Zoom
	zoomFov = spawnArgs.GetInt( "zoomFov", "-1" );
	zoomGui  = uiManager->FindGui ( spawnArgs.GetString ( "gui_zoom", "" ), true );
	zoomTime = spawnArgs.GetFloat ( "zoomTime", ".15" );
//	wfl.zoomHideCrosshair = spawnArgs.GetBool ( "zoomHideCrosshair", "1" );
}

/*
=====================
rvVehicleWeapon::Activate
=====================
*/
void rvVehicleWeapon::Activate ( bool activate ) {
	rvVehiclePart::Activate ( activate );
	
	nextFireTime = gameLocal.time + WEAPON_DELAY_FIRE;

#ifdef _XENON
	bestEnemy = 0;
#endif
}

/*
=====================
rvVehicleWeapon::StopTargetEffect
=====================
*/
void rvVehicleWeapon::StopTargetEffect ( void ) {
	if ( targetEffect ) {
		targetEffect->Stop( );
		targetEffect = NULL;
	}
}

/*
=====================
rvVehicleWeapon::UpdateLock
=====================
*/
void rvVehicleWeapon::UpdateLock ( void ) {
	
#ifdef _XENON

	bestEnemy = 0;
	
	// Handle auto-aim if it's enabled
	if ( cvarSystem->GetCVarBool("pm_AimAssist") ) {
	
		const float testDist = 2000.0f; // huge because we're outdoors
		
		idPlayer *player = gameLocal.GetLocalPlayer();
		if ( player && position->GetDriver() == player ) {
		
			idVec3 start = position->GetEyeOrigin();
			idVec3 end = start + (position->GetEyeAxis().ToAngles().ToForward() * testDist);

			idBounds bounds( start );
			bounds.AddPoint( end );

			idClipModel *clipModelList[ MAX_GENTITIES ];
			int numClipModels = gameLocal.ClipModelsTouchingBounds( player, bounds, -1, clipModelList, MAX_GENTITIES );
			
			float bDist = testDist;
			
 			float fovX, fovY;
 			gameLocal.CalcFov( player->CalcFov( true ), fovX, fovY );
			float dNear = cvarSystem->GetCVarFloat( "r_znear" );
			float dFar = testDist;
			float size = dFar * idMath::Tan( DEG2RAD( fovY * 0.5f ) ) * float(cvarSystem->GetCVarInteger("pm_AimAssistFOV")) / 100.0f;
			
			idFrustum aimArea;
			aimArea.SetOrigin( start );
			aimArea.SetAxis( position->GetEyeAxis() );
			aimArea.SetSize( dNear, dFar, size, size );

			for ( int i = 0; i < numClipModels; i++ ) {
				
				idClipModel *clip = clipModelList[ i ];
				idEntity *ent = clip->GetEntity();

				if ( ent->IsHidden() ) {
					continue;
				}

				bool isAI = ent->IsType( idAI::GetClassType() );
				bool isFriendly = false;
				
				if ( isAI ) {
					if ( static_cast<idAI *>( ent )->team == player->team ) {
						continue;
					}
				} else {
					continue;
				}
				
				float focusLength = (ent->GetPhysics()->GetOrigin() - start).LengthFast() - ent->GetPhysics()->GetBounds().GetRadius();
			
				const idBounds &b = ent->GetPhysics()->GetAbsBounds();
				bool inside = aimArea.IntersectsBounds(b);
				
				if ( inside ) {
					float dist = b.ShortestDistance(start);
					if ( bDist > dist ) {
						bDist = dist;
						bestEnemy = (idActor *)ent;
					}
				}
			}
		}
	}
#endif
	
	if ( !position->GetDriver() || parent->health <= 0 || !lockScanning) {
		StopTargetEffect( );
	} else if ( lockScanning && position->GetDriver() && position->GetDriver()->IsType( idPlayer::GetClassType() ) ) {
		//always update locking info
		GetLockInfo( position->GetEyeOrigin(), position->GetEyeAxis() );
		idPlayer *player = gameLocal.GetLocalPlayer();
		if ( player && position->GetDriver() == player ) {
			// Update the guide effect
			if ( targetEnt && targetEnt.IsValid() && targetEnt->health > 0 && targetEnt->IsType( idActor::GetClassType() ) ) {
				idVec3 eyePos;
				if ( targetJoint != INVALID_JOINT ) {
					idMat3 jointAxis;
					targetEnt->GetAnimator()->GetJointTransform( targetJoint, gameLocal.GetTime(), eyePos, jointAxis );
					eyePos = targetEnt->GetRenderEntity()->origin + (eyePos*targetEnt->GetRenderEntity()->axis);
					if ( !targetPos.Compare( vec3_origin ) ) {
						jointAxis = jointAxis * targetEnt->GetRenderEntity()->axis;
						eyePos += jointAxis[0]*targetPos[0];
						eyePos += jointAxis[1]*targetPos[1];
						eyePos += jointAxis[2]*targetPos[2];
					}
				} else {
					eyePos = static_cast<idActor *>(targetEnt.GetEntity())->GetEyePosition();
					eyePos += targetEnt->GetPhysics()->GetAbsBounds().GetCenter ( );
					eyePos *= 0.5f;
				}
				if ( targetEffect ) {			
					targetEffect->SetOrigin ( eyePos );
					targetEffect->SetAxis ( player->firstPersonViewAxis.Transpose() );
				} else {
					targetEffect = gameLocal.PlayEffect( gameLocal.GetEffect( spawnArgs, "fx_guide" ), eyePos, player->firstPersonViewAxis.Transpose(), true, vec3_origin, false );
					if ( targetEffect ) {
						targetEffect->GetRenderEffect()->weaponDepthHackInViewID = position->GetDriver()->entityNumber + 1;
						targetEffect->GetRenderEffect()->allowSurfaceInViewID = position->GetDriver()->entityNumber + 1;
					}
				}
			} else {
				StopTargetEffect( );
			}
		}
	}
}

/*
=====================
rvVehicleWeapon::RunPostPhysics
=====================
*/
void rvVehicleWeapon::RunPostPhysics ( void ) {
	if ( !IsActive() || !parent->IsShootingEnabled ( ) ) {
		return;
	}

	if ( currentAmmo <= 0 && currentCharge.IsDone( gameLocal.GetTime() ) ) {
		currentAmmo = ammoPerCharge;
	}

	UpdateLock();

	if ( !(position->mInputCmd.buttons & BUTTON_ATTACK ) ) {
		return;
	}

	if ( gameLocal.time <= nextFireTime || !currentCharge.IsDone(gameLocal.GetTime()) ) {
		return;
	}

	// Gun animation?
	if ( animNum ) {
		parent->GetAnimator()->PlayAnim ( animChannel, animNum, gameLocal.time, 0 );
	}

	if( !spawnArgs.GetBool("launchOnFrameCommand") ) {
		if (!Fire())
		{
			return;
		}
	}

	nextFireTime = gameLocal.time + fireDelay;
}

/*
================
rvVehicleWeapon::WeaponFeedback
================
*/
void rvVehicleWeapon::WeaponFeedback( const idDict* dict ) {
	if( !dict || !GetPosition() || !GetPosition()->IsOccupied() ) {
		return;
	}

	idActor* actor = GetPosition()->GetDriver();
	if( !actor || !actor->IsType( idPlayer::GetClassType() ) ) {
		return;
	}

	//abahr: This feels like a hack.  I hate using def files for logic but it just seems the easiest way to do it 
	idPlayer* player = static_cast<idPlayer*>( actor );
	if( dict->GetInt("recoilTime") ) {
		player->playerView.WeaponFireFeedback( dict );
	}
	if( dict->GetInt("shakeTime") ) {
		player->playerView.SetShakeParms( MS2SEC(gameLocal.GetTime() + dict->GetInt("shakeTime")), dict->GetFloat("shakeMagnitude") );
	}
	EjectBrass();
}

/*
================
rvVehicleWeapon::MuzzleFlashLight
================
*/
void rvVehicleWeapon::MuzzleFlashLight( const idVec3& origin, const idMat3& axis ) {
	if ( !muzzleFlash.lightRadius[0] ) {
		return;
	}

	muzzleFlash.origin = origin;
	muzzleFlash.axis   = axis;

	// these will be different each fire
	muzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	muzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]	= parent->GetRenderEntity()->shaderParms[ SHADERPARM_DIVERSITY ];

	// the light will be removed at this time
	muzzleFlashEnd = gameLocal.time + muzzleFlashTime;

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
	} else {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
	}
}

/*
=====================
rvVehicleWeapon::UpdateCursorGUI
=====================
*/
void rvVehicleWeapon::UpdateCursorGUI ( idUserInterface* gui ) const {
	// cnicholson: I do't see a reason why this if statement is here. Its ALWAYS  false and so this block never executed,
	// thus if the player was in a vehicle, the crosshair was always the player's last held weapon, not the crosshair defined in the vehicle .def.
	// So I commented the if part out. 
	//if ( spawnArgs.GetBool( "hide_crosshair", "0" ) ) {
		gui->SetStateString ( "crossImage", spawnArgs.GetString ( "mtr_crosshair" ) );
		const idMaterial *material = declManager->FindMaterial( spawnArgs.GetString ( "mtr_crosshair" ) );
		if ( material ) {
			material->SetSort( SS_GUI );
		}			
		
		gui->SetStateString ( "crossColor", g_crosshairColor.GetString() );
		gui->SetStateInt ( "crossOffsetX", spawnArgs.GetInt ( "crosshairOffsetX", "0" ) );
		gui->SetStateInt ( "crossOffsetY", spawnArgs.GetInt ( "crosshairOffsetY", "0" ) ); 			
 		gui->StateChanged ( gameLocal.time );
	//}
}

/*
=====================
rvVehicleWeapon::Save
=====================
*/
void rvVehicleWeapon::Save ( idSaveGame* savefile ) const {
	int i;

	savefile->WriteInt ( nextFireTime );
	savefile->WriteInt ( fireDelay );
	savefile->WriteInt ( count );
	// TOSAVE: const idDict*			projectileDef;
	// TOSVAE: const idDict*			hitScanDef;

	savefile->WriteFloat ( spread );
	savefile->WriteBool ( launchFromJoint );
	savefile->WriteBool ( lockScanning );
	savefile->WriteInt ( lastLockTime );
	savefile->WriteFloat ( lockRange );
	
	savefile->WriteInt ( joints.Num() );
	for ( i = 0; i < joints.Num(); i ++ ) {
		savefile->WriteJoint ( joints[i] );
	}
	savefile->WriteInt ( jointIndex );
	
	savefile->WriteVec3 ( force );
	
	savefile->WriteInt ( ammoPerCharge );
	savefile->WriteInt ( chargeTime );
	savefile->WriteInt ( currentAmmo );
	savefile->WriteInterpolate ( currentCharge );

	savefile->WriteRenderLight ( muzzleFlash );
	savefile->WriteInt ( muzzleFlashHandle );
	savefile->WriteInt ( muzzleFlashEnd );
	savefile->WriteInt ( muzzleFlashTime );
	savefile->WriteVec3 ( muzzleFlashOffset );
	
	savefile->WriteInt ( animNum );
	savefile->WriteInt ( animChannel );

	targetEnt.Save( savefile );
	savefile->WriteJoint( targetJoint );
	savefile->WriteVec3( targetPos );
	targetEffect.Save( savefile );
	// Don't save shaderFire or shaderReload, they are setup in Restore

	savefile->WriteInt( zoomFov );
	savefile->WriteUserInterface( zoomGui, true );
	savefile->WriteFloat( zoomTime );
}

/*
=====================
rvVehicleWeapon::Restore
=====================
*/
void rvVehicleWeapon::Restore ( idRestoreGame* savefile ) {
	int		i;
	int		num;
	idStr	temp;

#ifdef _XENON
	bestEnemy = 0;
#endif

	savefile->ReadInt ( nextFireTime );
	savefile->ReadInt ( fireDelay );
	savefile->ReadInt ( count );
	savefile->ReadFloat ( spread );
	savefile->ReadBool ( launchFromJoint );
	savefile->ReadBool ( lockScanning );
	savefile->ReadInt ( lastLockTime );
	savefile->ReadFloat ( lockRange );
	
	savefile->ReadInt ( num );
	joints.Clear ( );
	joints.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		savefile->ReadJoint ( joints[i] );
	}
	savefile->ReadInt ( jointIndex );
	
	savefile->ReadVec3 ( force );
	
	savefile->ReadInt ( ammoPerCharge );
	savefile->ReadInt ( chargeTime );
	savefile->ReadInt ( currentAmmo );
	savefile->ReadInterpolate( currentCharge );

	savefile->ReadRenderLight ( muzzleFlash );
	savefile->ReadInt ( muzzleFlashHandle );
	if ( muzzleFlashHandle != -1 ) {
		//get the handle again as it's out of date after a restore!
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
	}

	savefile->ReadInt ( muzzleFlashEnd );
	savefile->ReadInt ( muzzleFlashTime );
	savefile->ReadVec3 ( muzzleFlashOffset );

	savefile->ReadInt ( animNum );	
	savefile->ReadInt ( animChannel );
	
	if ( spawnArgs.GetString ( "def_hitscan", "", temp ) ) {
		hitScanDef = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_hitscan" ) );
	} else {
		projectileDef = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_projectile" ) );
	}

	shaderFire = declManager->FindSound ( spawnArgs.GetString ( "snd_fire" ), false );
	shaderReload = declManager->FindSound ( spawnArgs.GetString ( "snd_reload" ), false );

	// Brass Def
	brassDict = gameLocal.FindEntityDefDict( spawnArgs.GetString( "def_ejectBrass" ), false );
	spawnArgs.GetVector ( "ejectOffset", "0 0 0", brassEjectOffset );
	brassEjectJoint = parent->GetAnimator()->GetJointHandle( spawnArgs.GetString ( "joint_view_eject", "eject" ) );
	if ( brassDict ) {
		brassEjectDelay	= SEC2MS( brassDict->GetFloat( "delay", "0.01" ) );
	}
	brassEjectNext = 0;

	targetEnt.Restore( savefile );
	savefile->ReadJoint( targetJoint );
	savefile->ReadVec3( targetPos );
	targetEffect.Restore( savefile );

	savefile->ReadInt( zoomFov );
	savefile->ReadUserInterface( zoomGui, &spawnArgs );
	savefile->ReadFloat( zoomTime );
}

/*
=====================
rvVehicleWeapon::Restore
=====================
*/
bool rvVehicleWeapon::Fire() {
	if ( muzzleFlashHandle != -1 && gameLocal.time >= muzzleFlashEnd ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}

//	twhitaker: moved to rvVehicleWeapon::RunPostPhysics so that the HUD would update without having to fire the gun
//	if ( currentAmmo == 0 ) {
//		currentAmmo = ammoPerCharge;
//	}

	LaunchProjectiles();

	if ( currentAmmo == -1 ) {
		currentCharge.Init ( gameLocal.time, fireDelay, 0.0, 1.0f );
	} else {
		currentAmmo--;
		if ( currentAmmo <= 0 ) {
			currentAmmo = 0;
			if ( chargeTime ) {
				parent->StartSoundShader( shaderReload, SND_CHANNEL_ANY, 0, false, NULL );
				currentCharge.Init ( gameLocal.time, chargeTime, 0.0f, 1.0f );
			}			
		}
	}
	return true;
}
 
#ifdef _XENON
/*
=====================
rvVehicleWeapon::AutoAim
=====================
*/
void rvVehicleWeapon::AutoAim( idPlayer* player, const idVec3& origin, idVec3& dir ) {
	
	if ( player ) {
	
		// If we have a enemy selected, handle aim correction
		if ( bestEnemy ) {
		
			idVec3 dif = bestEnemy->GetRenderEntity()->origin - origin;
			idVec3 muzzleDest = origin + (dir * dif.Length() );
			
			// Transform start and end into the enemy's body space
			idVec3 localStart = bestEnemy->GetRenderEntity()->axis / (origin - bestEnemy->GetRenderEntity()->origin);
			idVec3 localEnd = bestEnemy->GetRenderEntity()->axis / (muzzleDest - bestEnemy->GetRenderEntity()->origin);
			
			if ( bestEnemy->GetAnimator() ) {
				int numJoints = bestEnemy->GetAnimator()->NumJoints();
				
				if ( numJoints ) {
				
					jointHandle_t nearJoint = bestEnemy->GetAnimator()->GetNearestJoint( localStart, localEnd, gameLocal.time, 0, numJoints );
			
					// If we found a valid joint to snap to...
					if ( nearJoint > 0 ) {
						idMat3 dummy;
						bestEnemy->GetJointWorldTransform( nearJoint, gameLocal.time, muzzleDest, dummy );
						dir = muzzleDest - origin;
						dir.Normalize();
					} else {
						dir = bestEnemy->GetRenderEntity()->origin - origin;
						dir.Normalize();
					}
				}
			} else {
				dir = bestEnemy->GetRenderEntity()->origin - origin;
				dir.Normalize();
			}
			
		}
	}
}
#endif

/*
=====================
rvVehicleWeapon::LaunchHitScan
=====================
*/
void rvVehicleWeapon::LaunchHitScan( const idVec3& origin, const idVec3& _dir, const idVec3& jointOrigin ) {
	idPlayer* player = 0;
	
	idVec3 dir = _dir;
	
	// Let the AI know about the new attack
	if ( !gameLocal.isMultiplayer ) {
		if ( position->GetDriver() && position->GetDriver()->IsType( idPlayer::GetClassType() ) ) {
			player = dynamic_cast<idPlayer*>(position->GetDriver());
			if ( player ) {
				aiManager.ReactToPlayerAttack ( player, origin, dir );
			}
		}
	}
	
#ifdef _XENON

	AutoAim( player, origin, dir );

#endif
	
	gameLocal.HitScan ( *hitScanDef, origin, dir, jointOrigin, position->GetDriver(), false, 1.0f, parent );
}

/*
=====================
rvVehicleWeapon::LaunchProjectile
=====================
*/
void rvVehicleWeapon::LaunchProjectile( const idVec3& origin, const idVec3& _dir, const idVec3& pushVelocity ) {
	
	idPlayer *player = 0;
	idEntity*		ent;
	idProjectile*	projectile;

	idVec3 dir = _dir;
	
	gameLocal.SpawnEntityDef ( *projectileDef, &ent );
	if ( !ent ) {
		return;
	}
										
	if ( !gameLocal.isMultiplayer ) {
		if ( position->GetDriver() && position->GetDriver()->IsType( idPlayer::GetClassType() ) ) {
			player = dynamic_cast<idPlayer*>(position->GetDriver());
			if ( player ) {
				aiManager.ReactToPlayerAttack ( player, origin, dir );
			}
		}
	}
	
#ifdef _XENON

	AutoAim( player, origin, dir );

#endif


	projectile = ( idProjectile * )ent;
	projectile->Create( position->GetDriver(), origin, dir, parent );
	projectile->Launch( origin, dir, pushVelocity, 0.0f, 1.0f );
	
	if ( projectile->IsType ( idGuidedProjectile::GetClassType() ) ) {
		if ( spawnArgs.GetBool("guideTowardsDir") && (!targetEnt || targetJoint == INVALID_JOINT) ) {
#ifndef _XENON			
			static_cast<idGuidedProjectile*>(projectile)->GuideTo ( position->GetEyeOrigin(), position->GetEyeAxis()[0] );
#else
			if ( bestEnemy ) {
				static_cast<idGuidedProjectile*>(projectile)->GuideTo ( origin, dir );
			} else {
				static_cast<idGuidedProjectile*>(projectile)->GuideTo ( position->GetEyeOrigin(), position->GetEyeAxis()[0] );
			}
#endif
		
		} else if ( targetEnt ) {
			static_cast<idGuidedProjectile*>(projectile)->GuideTo ( targetEnt, targetJoint, targetPos );
		} else {
			static_cast<idGuidedProjectile*>(projectile)->GuideTo ( targetPos );
		}
	}
}

/*
=====================
rvVehicleWeapon::LaunchProjectiles
=====================
*/
void rvVehicleWeapon::LaunchProjectiles() {
	idVec3			jointOrigin;
	idMat3			jointAxis;
	idVec3			origin;
	idMat3			axis;

	float			spreadRad = DEG2RAD( spread );
	idVec3			dir;
	float			ang = 0.0f;
	float			spin = 0.0f;

	parent->GetJointWorldTransform ( joints[jointIndex], gameLocal.time, jointOrigin, jointAxis );
	MuzzleFlashLight ( jointOrigin + muzzleFlashOffset * jointAxis, jointAxis );

	if( launchFromJoint ) {				
		origin = jointOrigin;
		axis = jointAxis;
	} else {
		origin = position->GetEyeOrigin();
		axis = position->GetEyeAxis();
	}

	if ( !lockScanning || !targetEnt || !position->GetDriver() || !position->GetDriver()->IsType( idPlayer::GetClassType() ) ) {
		//don't do this continuously, so have to do it here
		GetLockInfo( position->GetEyeOrigin(), position->GetEyeAxis() );
	}

	parent->StartSoundShader( shaderFire, SND_CHANNEL_WEAPON, 0, false, NULL );

	parent->PlayEffect( gameLocal.GetEffect( spawnArgs, "fx_muzzleflash" ), joints[jointIndex], vec3_origin, mat3_identity );

	gameLocal.AlertAI( parent );

	for( int i = 0; i < count; i ++ ) {
		ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
		spin = DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = axis[0] + axis[ 2 ] * ( ang * idMath::Sin( spin ) ) - axis[ 1 ] * ( ang * idMath::Cos( spin ) );
		dir.Normalize();

		if ( g_debugWeapon.GetBool() ) {
			gameRenderWorld->DebugLine ( colorBlue, origin, origin + dir * 10000.0f, 10000 );		
		}

		if ( hitScanDef ) {
			LaunchHitScan( origin, dir, jointOrigin );
		} else {
			LaunchProjectile( origin, dir, parent->GetPhysics()->GetPushedLinearVelocity() );
		}
	}

	jointIndex = (jointIndex+1) % joints.Num();

	parent->GetPhysics()->ApplyImpulse ( 0, origin, force * axis );

	WeaponFeedback( &spawnArgs );
}

/*
=====================
rvVehicleWeapon::GetLockInfo
=====================
*/
void rvVehicleWeapon::GetLockInfo( const idVec3& eyeOrigin, const idMat3& eyeAxis ) {
	if ( lastLockTime < gameLocal.GetTime() - 2000 
		|| (targetEnt && (!targetEnt.IsValid() || targetEnt.GetEntity()->health <= 0)) ) {
		targetEnt = NULL;
		targetJoint = INVALID_JOINT;
		targetPos.Zero ();
	}
	
	if ( lockRange > 0.0f ) {
		idVec3	end;
		trace_t	tr;
		bool lockFound = false;
		idEntity* newTargetEnt = NULL;
		jointHandle_t newTargetJoint = INVALID_JOINT;

		end = eyeOrigin + eyeAxis[0] * lockRange;		
//		gameLocal.TracePoint( parent.GetEntity(), tr, eyeOrigin, end, MASK_SHOT_BOUNDINGBOX, NULL );
		gameLocal.TracePoint( parent.GetEntity(), tr, eyeOrigin, end, MASK_SHOT_RENDERMODEL, NULL );

		if ( tr.fraction < 1.0 ) {
			newTargetEnt = gameLocal.entities[ tr.c.entityNum ];
			lockFound = true;
	
			// Make sure the target is an actor and is alive
			if ( !(newTargetEnt->IsType ( idActor::GetClassType() ) || newTargetEnt->IsType ( idProjectile::GetClassType() )) || newTargetEnt->health <= 0 ) {
				newTargetEnt = NULL;
				lockFound = false;
			} else {
				//see if there's a joint to lock onto
				if ( newTargetEnt->spawnArgs.MatchPrefix ( "lockJoint" ) ) {
					jointHandle_t testJointHandle;
					idVec3 testJointOffset;
					idStr lockJointName;
					idVec3 jointOrg;
					idMat3 jointAxis;

					int lockJointNum = 1;
					float bestDist = 100.0f;
					float dist = 0.0f;

					lockJointName = newTargetEnt->spawnArgs.GetString( va("lockJoint%d",lockJointNum), NULL );
					while ( lockJointName.Length() ) {
						testJointHandle = newTargetEnt->GetAnimator()->GetJointHandle( lockJointName );
						if ( testJointHandle != INVALID_JOINT ) {
							newTargetEnt->GetAnimator()->GetJointTransform( testJointHandle, gameLocal.GetTime(), jointOrg, jointAxis );
							jointOrg = newTargetEnt->GetRenderEntity()->origin + (jointOrg*newTargetEnt->GetRenderEntity()->axis);
							jointAxis = jointAxis * newTargetEnt->GetRenderEntity()->axis;
							testJointOffset = newTargetEnt->spawnArgs.GetVector( va("lockJointOffset%d",lockJointNum), "0 0 0" );
							jointOrg += jointAxis*testJointOffset;
							dist = (jointOrg - tr.endpos).Length();
							if ( dist < bestDist ) {
								bestDist = dist;
								newTargetJoint = testJointHandle;
								targetPos = testJointOffset;
							}
						}
						lockJointName = newTargetEnt->spawnArgs.GetString( va("lockJoint%d",++lockJointNum), NULL );
					}
				}
			}
			if ( spawnArgs.GetBool("guideTowardsDir") ) {
				if ( newTargetJoint == INVALID_JOINT ) {
					newTargetEnt = NULL;
					lockFound = false;
				}
			}
		} else if ( spawnArgs.GetBool( "guideTowardsDir" ) && !targetEnt ) {
			targetPos = tr.endpos;
		}
		if ( lockFound ) {
			targetEnt = newTargetEnt;
			targetJoint = newTargetJoint;
			lastLockTime = gameLocal.GetTime();
		}

		if ( g_debugVehicle.GetInteger() == 2 ) {
			gameRenderWorld->DebugArrow ( colorGreen, eyeOrigin, end, 3, 10000 );
			gameRenderWorld->DebugArrow ( colorWhite, eyeOrigin, tr.endpos, 4, 10000 );
		}				
	}
}

/*
=====================
rvVehicleWeapon::EjectBrass
=====================
*/
void rvVehicleWeapon::EjectBrass ( void ) {
	if ( !brassDict || gameLocal.time > brassEjectNext || g_brassTime.GetFloat() <= 0.0f || gameLocal.isMultiplayer || brassEjectJoint == INVALID_JOINT || !brassDict->GetNumKeyVals() ) {
		return;
	}

	idMat3 axis;
	idVec3 origin;
	idVec3 linear_velocity;
	idVec3 angular_velocity;
	int	   brassTime;

	if ( !parent->GetJointWorldTransform( brassEjectJoint, gameLocal.time, origin, axis ) ) {
		return;
	}

	brassEjectNext += brassEjectDelay;

	// Spawn the client side moveable for the brass
	idVec3 offset;
	idMat3 playerViewAxis;
	gameLocal.GetPlayerView( offset, playerViewAxis );
	rvClientMoveable* cent = NULL;
	gameLocal.SpawnClientEntityDef( *brassDict, (rvClientEntity**)(&cent), false );

	if( !cent ) {
		return;
	}

	cent->SetOrigin ( origin + axis * brassEjectOffset );
	cent->SetAxis ( playerViewAxis );	
	cent->SetOwner ( position->GetDriver() );
	
	// Depth hack the brass to make sure it clips in front of view weapon properly
	cent->GetRenderEntity()->weaponDepthHackInViewID = position->GetDriver()->entityNumber + 1;
	
	// Clear the depth hack soon after it clears the view
	cent->PostEventMS ( &CL_ClearDepthHack, 200 );
	
	// Fade the brass out so they dont accumulate
	brassTime =(int)SEC2MS(g_brassTime.GetFloat() / 6.0f);
	cent->PostEventMS ( &CL_FadeOut, brassTime, brassTime );

	// Generate a good velocity for the brass
	idVec3 linearVelocity = brassDict->GetVector("linear_velocity").Random( brassDict->GetVector("linear_velocity_range"), gameLocal.random );
	cent->GetPhysics()->SetLinearVelocity( position->GetDriver()->GetPhysics()->GetLinearVelocity() + linearVelocity * cent->GetPhysics()->GetAxis() );
	idAngles angularVelocity = brassDict->GetAngles("angular_velocity").Random( brassDict->GetVector("angular_velocity_range"), gameLocal.random );
	cent->GetPhysics()->SetAngularVelocity( angularVelocity.ToAngularVelocity() * cent->GetPhysics()->GetAxis() );
}

/***********************************************************************

							rvVehicleTurret

***********************************************************************/

#define	TURRET_MOVESOUND_FADE	200

CLASS_DECLARATION( rvVehiclePart, rvVehicleTurret )
END_CLASS

/*
=====================
rvVehicleTurret::rvVehicleTurret
=====================
*/
rvVehicleTurret::rvVehicleTurret ( void ) {
	moveTime  = 0;
	soundPart = -1;
}

/*
=====================
rvVehicleTurret::Spawn
=====================
*/
void rvVehicleTurret::Spawn ( void ) {
	angles[0][PITCH] = spawnArgs.GetFloat ( "minpitch", "0" );
	angles[1][PITCH] = spawnArgs.GetFloat ( "maxpitch", "0" );
	axisMap[PITCH]	 = spawnArgs.GetInt ( "pitch", "-1" );
	invert[PITCH]	 = spawnArgs.GetBool ( "pitchinvert", "0" ) ? -1.0f : 1.0f;

	angles[0][ROLL]  = spawnArgs.GetFloat ( "minroll", "0" );
	angles[1][ROLL]  = spawnArgs.GetFloat ( "maxroll", "0" );
	axisMap[ROLL]	 = spawnArgs.GetInt ( "roll", "-1" );
	invert[ROLL]	 = spawnArgs.GetBool ( "rollinvert", "0" ) ? -1.0f : 1.0f;

	angles[0][YAW]   = spawnArgs.GetFloat ( "minyaw", "0" );
	angles[1][YAW]   = spawnArgs.GetFloat ( "maxyaw", "0" );
	axisMap[YAW]	 = spawnArgs.GetInt ( "yaw", "-1" );
	invert[YAW]	     = spawnArgs.GetBool ( "yawinvert", "0" ) ? -1.0f : 1.0f;
	
	turnRate = spawnArgs.GetFloat ( "turnrate", "360" );
		
	currentAngles.Zero ( );

	//the parent is *not* stuck on spawn.
	parentStuck = false;


	// Find the vehicle part for the turret sound
	if ( *spawnArgs.GetString ( "snd_loop", "" ) ) {
		soundPart = position->AddPart ( rvVehicleSound::GetClassType(), spawnArgs );
		static_cast<rvVehicleSound*>(position->GetPart(soundPart))->SetAutoActivate ( false );
	}
}

/*
=====================
rvVehicleTurret::Activate
=====================
*/
void rvVehicleTurret::Activate ( bool active ) {
	rvVehiclePart::Activate( active );

	if ( !IsActive() && soundPart >= 0 ) {
		static_cast<rvVehicleSound*>(position->GetPart(soundPart))->Stop ( );
	}

}

/*
=====================
rvVehicleTurret::RunPostPhysics
=====================
*/
void rvVehicleTurret::RunPostPhysics ( void ) {
	int			i;
	idAngles	inputAngles;
	idAngles	lastInputAngles;
	idMat3		mat[3];
	idAngles	oldAngles;

	if ( IsActive ( ) ) {	
		for ( i = 0; i < 3; i++ ) {
			inputAngles[i] = SHORT2ANGLE( position->mInputCmd.angles[i] );
			lastInputAngles[i] = SHORT2ANGLE( position->mOldInputCmd.angles[i] );
		}
	} else {
		inputAngles.Zero ( );
		lastInputAngles.Zero ( );
	}

	oldAngles = currentAngles;

	mat[PITCH].Identity();
	mat[YAW].Identity();
	mat[ROLL].Identity();
	for ( i = 0; i < 3; i ++ ) {
		if ( axisMap[i] != -1 ) {
			float diff = (invert[i] * idMath::AngleDelta ( inputAngles[i], lastInputAngles[i] ));
			
			diff = SignZero( diff ) * idMath::ClampFloat ( 0.0f, turnRate * MS2SEC(gameLocal.GetMSec()), idMath::Fabs ( diff ) );
			if ( angles[0][i] == angles[1][i] ) {
				currentAngles[axisMap[i]] = idMath::AngleNormalize360( currentAngles[axisMap[i]] + diff );
			} else {
				currentAngles[axisMap[i]] = idMath::ClampFloat ( angles[0][i], angles[1][i], currentAngles[axisMap[i]] + diff );
			}
			
			idAngles angles;
			angles.Zero();
			angles[axisMap[i]] = currentAngles[axisMap[i]];
			mat[axisMap[i]] = angles.ToMat3();
		}
	}	

	// Update the turret moving sound
	if ( soundPart >= 0 ) {
		float speed;
		speed = idMath::Fabs( idMath::AngleDelta( oldAngles[YAW], currentAngles[YAW] ) );
		speed = Max( speed, idMath::Fabs( idMath::AngleDelta( oldAngles[PITCH], currentAngles[PITCH] ) ) );
		speed = Max( speed, idMath::Fabs( idMath::AngleDelta( oldAngles[ROLL], currentAngles[ROLL] ) ) );
		
		if ( speed ) {
			if ( !moveTime ) {			
				static_cast<rvVehicleSound*>(position->GetPart(soundPart))->Play ( );
			}
			moveTime = gameLocal.time + TURRET_MOVESOUND_FADE;
		} else if ( moveTime && gameLocal.time > moveTime ) {
			static_cast<rvVehicleSound*>(position->GetPart(soundPart))->Fade ( TURRET_MOVESOUND_FADE, 0.0f, 1.0f );
			moveTime = 0;
		}
		
		// Update the volume of the turret move sound using the current speed of the
		// turret as well as how long it has been since it has last moved.  
		if ( moveTime ) {
			float f;
			f = idMath::ClampFloat ( 0.0f, 1.0f, fabs(speed) / (turnRate * MS2SEC(gameLocal.GetMSec())) );
			static_cast<rvVehicleSound*>(position->GetPart(soundPart))->Attenuate ( f, f );
		}		
	}

	// Rotate the turret with the mouse
	parent->GetAnimator()->SetJointAxis( joint, JOINTMOD_LOCAL, mat[YAW] * mat[PITCH] * mat[ROLL] );

	if ( g_vehicleMode.GetInteger() != 0 && joint != INVALID_JOINT && parent->GetAnimator()->GetJointHandle( spawnArgs.GetString( "alignment_joint" ) ) == joint ) {
		idMat3 turretAxis;
		idVec3 temp;
		parent->GetJointWorldTransform( joint, gameLocal.GetTime(), temp, turretAxis );
		const idMat3 & vehicleAxis	= parent->GetPhysics()->GetAxis();
		int alignmentAxis			= spawnArgs.GetInt( "alignment_axis" );
		float dotRight				= vehicleAxis[1] * turretAxis[alignmentAxis];
		float dotForward			= vehicleAxis[0] * turretAxis[alignmentAxis];
		float acosForward			= idMath::ACos( dotForward );

		idAngles oldAngles			= currentAngles;
		idMat3 oldAxis				= vehicleAxis;

		if ( idMath::Fabs(1.0f - dotForward) > VECTOR_EPSILON ) {
			float sinus = idMath::Cos( dotForward );
			float power = idMath::Pow( sinus, spawnArgs.GetFloat( "alignment_power", "5" ) );
			float deg = idMath::ClampFloat( 0.0f, spawnArgs.GetFloat( "alignment_delta", "2.4" ), RAD2DEG( acosForward ) ) * power;
			idAngles delta( 0, deg * SignZero(dotRight), 0 );

			//move the tank
			parent->GetPhysics()->SetAxis( (delta).ToMat3() * vehicleAxis );
			currentAngles -= delta;
			trace_t tr;

			//trace the tank against the world.
			idBounds traceBounds;
			idVec3 raiseVector( 0, 0, 80);
			idVec3 reduceVector( 0, 0, -16);
			idVec3 parentOrigin = parent->GetPhysics()->GetOrigin();
			idVec3 parentVecForward = parent->GetPhysics()->GetAxis()[0];

			traceBounds.Zero();
			traceBounds = parent->GetPhysics()->GetAbsBounds();
			traceBounds.ExpandSelf( reduceVector );
			
			//zero the tracebounds-- abs bounds is the right size but not the right location :)
			traceBounds.TranslateSelf( traceBounds.GetCenter() * -1 );
			
			//raise the bounds up so the hover tank can hover.
			traceBounds.TranslateSelf( raiseVector );

			gameLocal.TraceBounds( parent, tr, parentOrigin, parentOrigin, traceBounds, MASK_SOLID | CONTENTS_VEHICLECLIP ,parent);

			//draw the debug turning bounds if we need to. It will be redder if there's collision.
			if ( gameDebug.IsHudActive ( DBGHUD_VEHICLE, parent ) ) {
				idVec4 vecColor(1.0f - ( tr.fraction) , 0.25f, (0.5f * tr.fraction), 1.0f);
				gameRenderWorld->DebugBounds( vecColor, traceBounds, parentOrigin, 10, true );
			}
			
			//roll it back if it's colliding.
			if( tr.fraction < 1.0f) {
				if ( gameDebug.IsHudActive ( DBGHUD_VEHICLE, parent ) ) {
					gameLocal.Warning("Turret move caused vehicle block %d distance %f", tr.c.entityNum, tr.fraction );
				}
				parent->GetPhysics()->SetAxis( oldAxis );
				currentAngles = oldAngles;

				//apply a push to the parent based on the direction of the collision? 
				parentStuck = true;
			}			
		}

	}
}
/*
=====================
rvVehicleTurret::RunPrePhysics
=====================
*/
void rvVehicleTurret::RunPrePhysics ( void ) {
	
	//in case we're stuck, apply an impulse to the parent based on the direction the turret is facing. This will help us pull out in the right direction.
	if( parentStuck )	{

		idVec3 impulseForce( 0,0,0);
		idMat3 turretAxis;
		idVec3 temp;
		int alignmentAxis = spawnArgs.GetInt( "alignment_axis" );
		parent->GetJointWorldTransform( joint, gameLocal.GetTime(), temp, turretAxis );
		impulseForce += ( turretAxis[alignmentAxis] * 50 * MS2SEC(gameLocal.msec) * parent->GetPhysics()->GetMass());
		//impulseForce += ( position->mInputCmd.forwardmove / 127.0f ) * parent->GetPhysics()->GetAxis()[0] * 25 * MS2SEC(gameLocal.msec) * parent->GetPhysics()->GetMass ();

		// Apply the impulse to the parent entity
		
		parent->GetPhysics()->ApplyImpulse( 0, parent->GetPhysics()->GetOrigin(), impulseForce);
		
		parentStuck = false;
	}


}


/*
=====================
rvVehicleTurret::Save
=====================
*/
void rvVehicleTurret::Save ( idSaveGame* savefile ) const {
	savefile->WriteBounds ( angles );
	savefile->WriteInt ( axisMap[0] );
	savefile->WriteInt ( axisMap[1] );
	savefile->WriteInt ( axisMap[2] );

	savefile->WriteFloat ( invert[0] );
	savefile->WriteFloat ( invert[1] );
	savefile->WriteFloat ( invert[2] );

	savefile->WriteAngles ( currentAngles );

	savefile->WriteInt ( moveTime );
	savefile->WriteFloat ( turnRate );

	savefile->WriteInt ( soundPart );
}

/*
=====================
rvVehicleTurret::Restore
=====================
*/
void rvVehicleTurret::Restore ( idRestoreGame* savefile ) {
	savefile->ReadBounds ( angles );
	savefile->ReadInt ( axisMap[0] );
	savefile->ReadInt ( axisMap[1] );
	savefile->ReadInt ( axisMap[2] );

	savefile->ReadFloat ( invert[0] );
	savefile->ReadFloat ( invert[1] );
	savefile->ReadFloat ( invert[2] );

	savefile->ReadAngles ( currentAngles );

	savefile->ReadInt ( moveTime );
	savefile->ReadFloat ( turnRate );

	savefile->ReadInt ( soundPart );	
}

/***********************************************************************

							rvVehicleHoverpad

***********************************************************************/

CLASS_DECLARATION( rvVehiclePart, rvVehicleHoverpad )
END_CLASS

rvVehicleHoverpad::rvVehicleHoverpad ( void ) {
	clipModel  = NULL;
	effectDust = NULL;
	soundPart  = -1;
	effectDustMaterialType = NULL;
}

rvVehicleHoverpad::~rvVehicleHoverpad ( void ) {
	if ( effectDust ) {
		effectDust->Stop ( );
		effectDust = NULL;
	}		

	delete clipModel;
}

/*
================
rvVehicleHoverpad::Spawn
================
*/
void rvVehicleHoverpad::Spawn ( void ) {
	float size;
		
	spawnArgs.GetFloat ( "size", "10", size );
	spawnArgs.GetFloat ( "height", "70", height );
	spawnArgs.GetFloat ( "dampen", "10", dampen );
	spawnArgs.GetFloat ( "forceRandom", "10", forceRandom );		
	spawnArgs.GetFloat ( "thrustForward", "0", thrustForward );
	spawnArgs.GetFloat ( "thrustLeft", "0", thrustLeft );
		
	maxRestAngle = idMath::Cos ( DEG2RAD(spawnArgs.GetFloat ( "maxRestAngle", "20" ) ) );
		
	fadeTime = SEC2MS(spawnArgs.GetFloat ( "fadetime", ".5" ));
				
	effectDust = NULL;
	atRest		= true;
	
	delete clipModel;
	clipModel = new idClipModel ( idTraceModel ( idBounds ( spawnArgs.GetVector("mins"),spawnArgs.GetVector("maxs")) ) );
	
	force = spawnArgs.GetFloat ( "force", "1066" );
	forceUpTime   = SEC2MS ( spawnArgs.GetFloat ( "forceUpTime", "1" ) );
	forceDownTime = SEC2MS ( spawnArgs.GetFloat ( "forceDownTime", "1" ) );
	forceTable = declManager->FindTable( spawnArgs.GetString ( "forceTable", "linear" ), false );

	currentForce.Init ( 0, 0, 0, 0 );				
	
	// Is a sound part specified?
	if ( *spawnArgs.GetString ( "snd_loop", "" ) ) {
		soundPart = position->AddPart ( rvVehicleSound::GetClassType(), spawnArgs );
		static_cast<rvVehicleSound*>(position->GetPart(soundPart))->SetAutoActivate ( false );
	}
	
	Activate ( false );
}

/*
================
rvVehicleHoverpad::Activate
================
*/
void rvVehicleHoverpad::Activate ( bool active ) {
	rvVehiclePart::Activate ( active );

	if ( active ) {
		currentForce.Init ( gameLocal.time, forceUpTime, currentForce.GetCurrentValue( gameLocal.time ), force );
		atRest	= false;
	} else {
		currentForce.Init ( gameLocal.time, forceDownTime, currentForce.GetCurrentValue( gameLocal.time ), 0 );
	}	
}

/*
================
rvVehicleHoverpad::RunPrePhysics

Called before RunPhysics.  Since impact velocities are altered by calls to ApplyImpulse
we need to cache them to ensure other hoverpads are not altering the data.  The start 
position of the hoverpad is also cached here for efficiency.
================
*/
void rvVehicleHoverpad::RunPrePhysics ( void ) {	
	impactInfo_t info;

	UpdateOrigin ( );

	// Cache velocity at start point
	parent->GetPhysics()->GetImpactInfo( 0, worldOrigin, &info );
	velocity = info.velocity;
}

/*
================
rvVehicleHoverpad::RunPhysics
================
*/
void rvVehicleHoverpad::RunPhysics ( void ) {
	idVec3	end;
	idMat3	axis;
	trace_t	tr;
	idVec3  impulseForce;
	idVec3	dampingForce;
	float	hoverForce;
	float   curlength;		

	// Disable pad?
	if ( !IsActive() && !atRest ) {
		if ( parent->IsAtRest ( ) || currentForce.GetCurrentValue ( gameLocal.time ) <= 0.0f || parent->IsFlipped( ) || parent->IsStalled() ) {
			if ( soundPart >= 0 ) {
				float fadeVolume;
				fadeVolume = idMath::dBToScale ( spawnArgs.GetFloat ( "fadevolume", "0" ) );
				static_cast<rvVehicleSound*>(position->GetPart(soundPart))->Fade ( fadeTime, fadeVolume, spawnArgs.GetFloat ( "fadefreqshift", "0.1" ) );
			}

			if ( effectDust ) {
				effectDust->Stop ( );
				effectDust = NULL;			
			}
			
			atRest = true;
		}
	}

	// If the vehicle isnt activate then kill the effect on it and dont think
	if ( atRest ) {						
		return;
	}

	// Prepare for trace			
	axis[0] = -parent->GetPhysics()->GetAxis()[2];
	axis[1] = parent->GetPhysics()->GetAxis()[0];	
	axis[2] = parent->GetPhysics()->GetAxis()[1];

	// Always point the hover jets towards gravity
	end  = worldOrigin + (parent->GetPhysics()->GetGravityNormal ( ) * height);	

	// Determine how far away the hoverpad is from the ground
	gameLocal.Translation ( parent.GetEntity(), tr, worldOrigin, end, clipModel, axis, MASK_SOLID|CONTENTS_VEHICLECLIP, parent );
	if ( tr.fraction >= 1.0f && tr.endpos == end ) {	
		UpdateDustEffect ( worldOrigin, axis, 0.0f, NULL );
		return;
	}
		
	// Determine spring properties from trace
	tr.c.point = worldOrigin + (end-worldOrigin) * (tr.fraction + 0.001f);
	curlength  = height - (worldOrigin - tr.c.point).Length ( );		

	// Dampening
	dampingForce = tr.c.point - worldOrigin;
	dampingForce = ( dampen * ( (velocity * dampingForce) / (dampingForce * dampingForce) ) ) * dampingForce;		

	// Random swap forces
	float rnd = 0.0f;
	if ( !position->mInputCmd.forwardmove && !position->mInputCmd.rightmove ) {
		float angle = DEG2RAD( ( (float)( gameLocal.time % 5000 ) / 5000.0f ) * 360.f );
		if ( fl.left ) {
			angle += 90;
		}
		if ( fl.front ) {
			angle += 180;
		}
		rnd = forceRandom * idMath::Sin( angle );
	}

	// Determine how much force should be applied at the center of the hover spring		
	hoverForce = (currentForce.GetCurrentValue(gameLocal.time) + rnd) * parent->GetPhysics()->GetMass () * MS2SEC(gameLocal.GetMSec());

	// If the slope under the hoverpad is < 30 degress then use the gravity vector to allow the hovertank to
	// stop on slopes, otherwise use the vehicles axis which will cause it to slide down steep slopes 
	float f = (-parent->GetPhysics()->GetGravityNormal ( ) * tr.c.normal);
	if ( f < maxRestAngle || (thrustForward && position->mInputCmd.forwardmove != 0 ) ) {
		impulseForce = (forceTable->TableLookup ( curlength / height ) * hoverForce) * -axis[0] - dampingForce;	
	} else {
		impulseForce = (forceTable->TableLookup ( curlength / height ) * hoverForce) * -parent->GetPhysics()->GetGravityNormal ( ) - dampingForce;	
	}

	// No thrust if movement is disabled
	if ( parent->IsMovementEnabled ( ) ) {
		idMat3 axis;
		idVec3 offset;
		jointHandle_t axisJoint = joint;
		if ( axisJoint == INVALID_JOINT ) {
			axisJoint = parent->GetAnimator()->GetJointHandle( "gun_pivot" );
		}
		parent->GetJointWorldTransform( axisJoint, gameLocal.time, offset, axis );

		// Add forward/backward thrust
		if ( thrustForward ) {
			if ( g_vehicleMode.GetInteger() == 2 ) {
				impulseForce += ( position->mInputCmd.forwardmove / 127.0f ) * axis[0] * thrustForward * MS2SEC(gameLocal.msec) * parent->GetPhysics()->GetMass ();
			} else {
				impulseForce += ( position->mInputCmd.forwardmove / 127.0f ) * parent->GetPhysics()->GetAxis()[0] * thrustForward * MS2SEC(gameLocal.msec) * parent->GetPhysics()->GetMass ();
			}
		}
 
		// Add right/left thrust.  The front pads will add force to the right if the right button is pressed and the rear will do the opposite.  If moving backward the directions are flipped again
		if ( thrustLeft ) {
			if ( !parent->IsStrafing() && g_vehicleMode.GetInteger() != 0 ) {
				impulseForce += ( position->mInputCmd.rightmove / 127.0f ) * -axis[2] * thrustLeft * MS2SEC(gameLocal.msec) * parent->GetPhysics()->GetMass ();
			} else {
				impulseForce += ( position->mInputCmd.rightmove / 127.0f ) * (position->mInputCmd.forwardmove<0?-1:1) * parent->GetPhysics()->GetAxis()[1] * thrustLeft * MS2SEC(gameLocal.msec) * parent->GetPhysics()->GetMass () * (fl.front ? 1 : -1);
			}
		}
	}
	
	// Apply the impulse to the parent entity
	parent->GetPhysics()->ApplyImpulse( 0, worldOrigin, impulseForce);
		
	// Update the position and intensity of the dust effect
	UpdateDustEffect ( tr.c.point, tr.c.normal.ToMat3( ), (curlength / height), tr.c.materialType );

	// Debug information
	if ( gameDebug.IsHudActive ( DBGHUD_VEHICLE, parent ) ) {
		idVec3 offset;
		offset = worldOrigin - parent->GetPhysics()->GetOrigin();
		offset *= parent->GetPhysics()->GetAxis().Transpose();		
		gameDebug.AppendList ( "hover", va("%c%c %d\t%0.1f\t%0.1f\t%d\t", (offset[0]>0?'F':'B'),(offset[1]<0?'R':'L'), (int)impulseForce.Length(), curlength, dampingForce.Length(), (int)(velocity*axis[0] ) ) );		
	}		
	
	// Draw debug information
	if ( g_debugVehicle.GetInteger() == 2 ) {
		collisionModelManager->DrawModel ( clipModel->GetCollisionModel(), tr.c.point, axis, vec3_origin, mat3_identity, 0.0f ); 
		gameRenderWorld->DebugCircle ( colorGreen, tr.c.point, axis[0], 5, 10 );				
		gameRenderWorld->DebugLine ( colorMagenta, worldOrigin, worldOrigin + idVec3(0,0,-height));
		gameRenderWorld->DebugLine ( colorWhite, worldOrigin, end );
		gameRenderWorld->DebugCircle ( colorBlue, worldOrigin, axis[0], 5, 10 );
		gameRenderWorld->DebugCircle ( colorBlue, end, axis[0], 5, 10 );

		if ( thrustForward && position->mInputCmd.forwardmove != 0 ) {
			gameRenderWorld->DebugArrow( colorCyan, worldOrigin, worldOrigin + parent->GetPhysics()->GetAxis()[1] * 50.0f * (fl.front ? 1 : -1) * ( position->mInputCmd.rightmove / 127.0f ), 10);
		}

		if ( thrustLeft && position->mInputCmd.rightmove != 0 ) {
			gameRenderWorld->DebugArrow( colorCyan, worldOrigin, worldOrigin + parent->GetPhysics()->GetAxis()[0] * 50.0f * ( position->mInputCmd.forwardmove / 127.0f ), 10);
		}
	}
}

/*
================
rvVehicleHoverpad::UpdateDustEffect
================
*/
void rvVehicleHoverpad::UpdateDustEffect ( const idVec3& origin, const idMat3& axis, float attenuation, const rvDeclMatType* mtype ) {
	if ( !effectDust || (mtype && effectDustMaterialType != mtype) ) {	
		if ( effectDust ) {
			effectDust->Stop ( );
			effectDust = NULL;
		}		

		effectDust = gameLocal.PlayEffect ( gameLocal.GetEffect ( spawnArgs, "fx_dust", mtype ), origin, axis, true );
		effectDustMaterialType = mtype;
	}

	if ( effectDust ) {	
		effectDust->Attenuate ( attenuation );
		effectDust->SetOrigin ( origin );
		effectDust->SetAxis ( axis );
	}

	// If there is a sound playing update it as well	
	if ( soundPart >= 0 ) {
		if ( static_cast<rvVehicleSound*>(position->GetPart(soundPart))->IsPlaying ( ) ) {
			static_cast<rvVehicleSound*>(position->GetPart(soundPart))->Attenuate ( attenuation, attenuation );
		} else {
			static_cast<rvVehicleSound*>(position->GetPart(soundPart))->Play ( );
		}
	}
}

/*
=====================
rvVehicleHoverpad::Save
=====================
*/
void rvVehicleHoverpad::Save ( idSaveGame* savefile ) const {
	savefile->WriteFloat ( height );
	savefile->WriteFloat ( dampen );
	savefile->WriteBool ( atRest );

	savefile->WriteBounds ( clipModel->GetBounds ( ) );

	savefile->WriteInterpolate ( currentForce );
	savefile->WriteFloat ( force );
	savefile->WriteTable ( forceTable );
	savefile->WriteFloat ( forceRandom );
	
	savefile->WriteFloat ( fadeTime );
	savefile->WriteVec3 ( velocity );

	savefile->WriteFloat ( thrustForward );
	savefile->WriteFloat ( thrustLeft );
	
	savefile->WriteFloat ( maxRestAngle );

	savefile->WriteInt ( soundPart );

	savefile->WriteInt ( forceUpTime );
	savefile->WriteInt ( forceDownTime );

	effectDust.Save ( savefile );
	savefile->WriteMaterialType ( effectDustMaterialType );
}

/*
=====================
rvVehicleHoverpad::Restore
=====================
*/
void rvVehicleHoverpad::Restore ( idRestoreGame* savefile ) {
	idBounds bounds;

	savefile->ReadFloat ( height );
	savefile->ReadFloat ( dampen );
	savefile->ReadBool ( atRest );

	delete clipModel;
	savefile->ReadBounds ( bounds );
	clipModel = new idClipModel ( idTraceModel ( bounds ) );
	
	savefile->ReadInterpolate ( currentForce );
	savefile->ReadFloat ( force );
	savefile->ReadTable ( forceTable );
	savefile->ReadFloat ( forceRandom );

	savefile->ReadFloat ( fadeTime );
	savefile->ReadVec3 ( velocity );

	savefile->ReadFloat ( thrustForward );
	savefile->ReadFloat ( thrustLeft );
	
	savefile->ReadFloat ( maxRestAngle );

	savefile->ReadInt ( soundPart );
	
	savefile->ReadInt ( forceUpTime );
	savefile->ReadInt ( forceDownTime );

	effectDust.Restore ( savefile );
	savefile->ReadMaterialType ( effectDustMaterialType );
}

/***********************************************************************

							rvVehicleThruster

***********************************************************************/

CLASS_DECLARATION( rvVehiclePart, rvVehicleThruster )
END_CLASS

rvVehicleThruster::rvVehicleThruster ( void ) {
}

rvVehicleThruster::~rvVehicleThruster ( void ) {
}

/*
=====================
rvVehicleThruster::Save
=====================
*/
void rvVehicleThruster::Save ( idSaveGame* savefile ) const {
	savefile->WriteFloat ( force );
	savefile->WriteInt ( forceAxis );
	savefile->WriteInt ( key );
}

/*
=====================
rvVehicleThruster::Restore
=====================
*/
void rvVehicleThruster::Restore ( idRestoreGame* savefile ) {
	savefile->ReadFloat ( force );
	savefile->ReadInt ( forceAxis );
	savefile->ReadInt ( (int&)key );
}

/*
================
rvVehicleThruster::Spawn
================
*/
void rvVehicleThruster::Spawn ( void ) {
	idStr keyName;

	force		= spawnArgs.GetFloat ( "force", "0" );
	forceAxis	= spawnArgs.GetInt ( "forceAxis", "0" );
	
	// Determine which key controls the thruster
	spawnArgs.GetString ( "key", "", keyName );
	if ( !keyName.Icmp ( "forward" ) ) 	{
		key = KEY_FORWARD;
	} else if ( !keyName.Icmp ( "right" ) ) {
		key = KEY_RIGHT;
	} else if ( !keyName.Icmp ( "up" ) ) {
		key = KEY_UP;
	}			
}

/*
================
rvVehicleThruster::RunPhysics
================
*/
void rvVehicleThruster::RunPhysics ( void ) {
	float mult;
	
	if ( !IsActive ( ) ) {
		return;
	}
	
	// Determine the force multiplier from the key being pressed
	mult = 0.0f;
	switch ( key )	{
		case KEY_FORWARD:
			mult = idMath::ClampFloat ( -1.0f, 1.0f, position->mInputCmd.forwardmove );
			break;

		case KEY_RIGHT:
			mult = idMath::ClampFloat ( -1.0f, 1.0f, position->mInputCmd.rightmove );
			break;

		case KEY_UP:
			mult = idMath::ClampFloat ( -1.0f, 1.0f, position->mInputCmd.upmove );
			break;
	}
	
	// No multiplier, no move
	if ( mult == 0.0f ) {
		return;
	}			
	
	UpdateOrigin ( );
	
	// Apply the force
	parent->GetPhysics()->ApplyImpulse ( 0, worldOrigin, worldAxis[forceAxis] * force * mult );
	
	// Debug Information
	if ( g_debugVehicle.GetInteger() == 2 ) {
		gameRenderWorld->DebugBounds ( colorCyan, idBounds(idVec3(-4,-4,-4), idVec3(4,4,4) ), worldOrigin );
		gameRenderWorld->DebugArrow ( colorCyan, worldOrigin, worldOrigin + worldAxis[forceAxis] * 100.0f * mult * (force < 0 ? -1 : 1), 10 );	
	}
}

/***********************************************************************

						rvVehicleUserAnimated

***********************************************************************/

CLASS_DECLARATION( rvVehiclePart, rvVehicleUserAnimated )
	EVENT( EV_PostSpawn, rvVehicleUserAnimated::Event_PostSpawn )
END_CLASS

rvVehicleUserAnimated::rvVehicleUserAnimated ( void ) {
}

/*
=====================
rvVehicleUserAnimated::Save
=====================
*/
void rvVehicleUserAnimated::Save ( idSaveGame* savefile ) const {
}

/*
=====================
rvVehicleUserAnimated::Restore
=====================
*/
void rvVehicleUserAnimated::Restore ( idRestoreGame* savefile ) {
	Event_PostSpawn();
}

/*
================
rvVehicleUserAnimated::Spawn
================
*/
void rvVehicleUserAnimated::Spawn ( void ) {
	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
rvVehicleUserAnimated::Event_PostSpawn
================
*/
void rvVehicleUserAnimated::Event_PostSpawn ( void ) {
	InitAnim( "forward",	anims[ VUAA_Forward	] );
	InitAnim( "strafe",		anims[ VUAA_Strafe	] );
	InitAnim( "crouch",		anims[ VUAA_Crouch	] );
	InitAnim( "attack",		anims[ VUAA_Attack	] );

	InitFunc( "forward",	funcs[ VUAF_Forward	] );
	InitFunc( "strafe",		funcs[ VUAF_Strafe	] );
	InitFunc( "crouch",		funcs[ VUAF_Crouch	] );
	InitFunc( "attack",		funcs[ VUAF_Attack	] );
}


/*
================
rvVehicleUserAnimated::RunPrePhysics
================
*/
void rvVehicleUserAnimated::RunPrePhysics ( void ) {
	if ( !IsActive ( ) ) {
		return;
	}

	rvVehicle				*vehicle	= position->GetParent();
	const rvVehiclePosition	*position	= vehicle->GetPosition( 0 );
	const usercmd_t &		input		= position->mInputCmd;

	if ( position->IsOccupied() ) {
		LateralMove( input.forwardmove, VUAA_Forward,	VUAF_Forward );
		LateralMove( input.rightmove,	VUAA_Strafe,	VUAF_Strafe );
		LateralMove( input.upmove,		VUAA_Crouch,	VUAF_Crouch	);

		// cheat and use LateralMove for the attack
		LateralMove( input.buttons & BUTTON_ATTACK, VUAA_Attack, VUAF_Attack );
	}
}

/*
================
rvVehicleUserAnimated::InitAnim
================
*/
void rvVehicleUserAnimated::InitAnim( const char * action, rvUserAnimatedAnim_t & anim ) {
	idStr prefix( action );
	rvVehicle * vehicle = position->GetParent();

	anim.index		= vehicle->GetAnimator()->GetAnim( spawnArgs.GetString( prefix + "_anim" ) );
	anim.rate		= spawnArgs.GetFloat( prefix + "_rate", "1" );
	anim.frame		= spawnArgs.GetInt( prefix + "_frame" ) * anim.rate;
	anim.channel	= spawnArgs.GetInt( prefix + "_channel" );
	anim.loop		= spawnArgs.GetBool( prefix + "_loop" );
	anim.length		= vehicle->GetAnimator()->NumFrames( anim.index );

	// NOTE: I may want to add the suffix "_play" in the InitAnim() function
	// this would specify that the animation should be played rather than stepped through
	// which could be useful especially for the attack command
}

/*
================
rvVehicleUserAnimated::InitFunc
================
*/
void rvVehicleUserAnimated::InitFunc( const char * action, rvScriptFuncUtility & func ) {
	idStr temp = idStr( action ) + "_script";

	if ( spawnArgs.GetString( temp, NULL, temp ) ) {
		func.Init( temp );
	}
}

/*
================
rvVehicleUserAnimated::SetFrame
================
*/
void rvVehicleUserAnimated::SetFrame( const rvUserAnimatedAnim_t & anim ) {
	float lerp				= anim.frame - int( anim.frame );
	frameBlend_t frameBlend	= { 0, anim.frame, anim.frame + 1.0f, 1.0f - lerp, lerp };
	position->GetParent()->GetAnimator()->SetFrame( anim.channel, anim.index, frameBlend );
}

/*
================
rvVehicleUserAnimated::LateralMove
================
*/
void rvVehicleUserAnimated::LateralMove( signed char input, int anim, int func ) {
	if ( !input ) {
		return;
	}

	rvUserAnimatedAnim_t & theAnim = anims[ anim ];

	if ( theAnim.index ) {

		float sign		= ( ( input > 0 ) ? 1.0f : -1.0f );
		theAnim.frame	+= theAnim.rate * sign;

		if ( theAnim.frame < 1 ) {
			theAnim.frame = ( ( theAnim.loop ) ? theAnim.length : 1 );
		} else if ( theAnim.frame > theAnim.length ) {
			theAnim.frame = ( ( theAnim.loop ) ? 1 : theAnim.length );
		}

		SetFrame( theAnim );
	}

	if ( funcs[ func ].Valid( ) )
		funcs[ func ].CallFunc( &spawnArgs );
}
