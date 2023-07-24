// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "TransportSystems.h"
#include "../Entity.h"
#include "Transport.h"
#include "../Atmosphere.h"

/*
===============================================================================

	sdTransportEngine

===============================================================================
*/

/*
================
sdTransportEngine::sdTransportEngine
================
*/
sdTransportEngine::sdTransportEngine( void ) {
	Clear();
}

/*
================
sdTransportEngine::~sdTransportEngine
================
*/
sdTransportEngine::~sdTransportEngine( void ) {
	Clear();
}

/*
================
sdTransportEngine::Init
================
*/
void sdTransportEngine::Init( idEntity* _parent, int _startIndex, int _max ) { 
	Clear();

	parent		= _parent->Cast< sdTransport >();
	if ( parent == NULL ) {
		gameLocal.Error( "sdTransportEngine::Init - _parent is not an sdTransport!" );
	}
	startIndex	= _startIndex;
	max			= _max;
}

/*
================
sdTransportEngine::AddSound
================
*/
void sdTransportEngine::AddSound( const engineSoundInfo_t& info ) {
	int num = sounds.Num();

	engineSound_t& sound = sounds.Alloc();

	sound.enabled = false;
	sound.soundFile = info.soundFile;
	sound.pitchLerp.Init( info.lowRev + info.fsStart, info.fsStop - info.fsStart, info.minFreqshift, info.maxFreqshift );
	sound.volumeLerp.Init( info.lowRev, info.leadIn, info.highRev - info.leadOut, info.leadOut, info.lowDB, info.highDB );
	sound.volumeLerp.SetOOBValue( -100 );
}

/*
================
sdTransportEngine::Update
================
*/
void sdTransportEngine::Update( bool off ) {
	const idVec3& previousVelocity = parent->GetPhysics()->GetLinearVelocity();
	state = !off;

	previousSpeed += ( MS2SEC( gameLocal.msec ) * pm_vehicleSoundLerpScale.GetFloat() * ( ( previousVelocity * parent->GetRenderEntity()->axis[ 0 ] ) - previousSpeed ) );

	if ( idMath::Fabs( previousSpeed ) < idMath::FLT_EPSILON ) {
		previousSpeed = 0.0f;
	}

	if ( !parent->GetSoundEmitter() ) {
		return;
	}

	for ( int i = 0; i < sounds.Num(); i++ ) {
		engineSound_t& info = sounds[ i ];

		if ( off || parent->IsEMPed() ) {
			if ( info.enabled ) {
				parent->StopSound( startIndex + i );
				info.enabled = false;
			}
			continue;
		}

		if ( !info.enabled ) {
			parent->StartSound( info.soundFile, startIndex + i, 0, NULL );
			info.enabled = true;
		}

		float volume		= info.volumeLerp.GetCurrentValue( previousSpeed );
		float pitchShift	= info.pitchLerp.GetCurrentValue( previousSpeed );
		if ( volume == 0.f ) {
			volume = 0.001f; // so the override actually works
		}

		const soundShaderParms_t& oldParms = parent->GetSoundEmitter()->GetChannelParms( startIndex + i );

		bool changed = false;
		if ( oldParms.volume != volume || oldParms.pitchShift != pitchShift ) {
			changed = true;
		}

		if ( changed ) {
			soundShaderParms_t parms = oldParms;

			parms.pitchShift = pitchShift;
			parms.volume = volume;
			parent->GetSoundEmitter()->ModifySound( startIndex + i, parms );
		}
	}
}

/*
================
sdTransportEngine::Clear
================
*/
void sdTransportEngine::Clear( void ) {
	for ( int i = 0; i < sounds.Num(); i++ ) {
		parent->StopSound( startIndex + i );
	}
	sounds.Clear();

	parent			= NULL;
	previousSpeed	= 0.0f;
	startIndex		= 0;
	max				= 0;
}

/*
===============================================================================

	sdVehicleLightSystem

===============================================================================
*/

/*
================
sdVehicleLightSystem::sdVehicleLightSystem
================
*/
sdVehicleLightSystem::sdVehicleLightSystem( void ) {
	parent			= NULL;
	brakeStartTime	= -1;
	lastBrakeTime	= -150;
}

/*
================
sdVehicleLightSystem::~sdVehicleLightSystem
================
*/
sdVehicleLightSystem::~sdVehicleLightSystem( void ) {
	Clear();
}

/*
================
sdVehicleLightSystem::Init
================
*/
void sdVehicleLightSystem::Init( sdTransport* _parent ) {
	parent = _parent;
	Clear();
}

/*
================
sdVehicleLightSystem::Clear
================
*/
void sdVehicleLightSystem::Clear( void ) {
	int i;
	for( i = 0; i < lights.Num(); i++ ) {
		vehicleLightData_t& light = lights[ i ];

		if( light.lightHandle != -1 ) {
			gameRenderWorld->FreeLightDef( light.lightHandle );
			light.lightHandle = -1;
		}
	}

	lights.Clear();
}

/*
================
sdVehicleLightSystem::UpdateLightPresentation
================
*/
void sdVehicleLightSystem::UpdateLightPresentation( vehicleLightData_t& light ) {
	parent->GetWorldOriginAxis( light.joint, light.renderLight.origin, light.renderLight.axis );
	light.renderLight.origin += light.offset * light.renderLight.axis;
}

/*
================
sdVehicleLightSystem::UpdateLight
================
*/
void sdVehicleLightSystem::UpdateLight( vehicleLightData_t& light, const sdVehicleInput& input ) {

	renderEntity_t* renderEnt = parent->GetRenderEntity();

	if ( light.enabled && !parent->IsHidden() && !parent->IsModelDisabled() ) {
		if( light.lightType & LIGHT_BRAKELIGHT ) {

			light.on = input.GetBraking();

			if( light.on ) {
				if( brakeStartTime < 0 ) {
					brakeStartTime = gameLocal.time;
				}
				lastBrakeTime = gameLocal.time;

				parent->SetShaderParm( 6, 1.f );
			} else {
				int postBrakeDuration = gameLocal.time - lastBrakeTime;

				brakeStartTime = -1;

				parent->SetShaderParm( 6, postBrakeDuration < 150 ? ( 150 - postBrakeDuration ) / 150.f : 0.f );
			}

			light.renderLight.shaderParms[ 6 ] = renderEnt->shaderParms[ 6 ];
		}

		if( light.lightType & LIGHT_STANDARD ) {
			bool nightLightOn = NightLightIsOn();

			if( !light.on || !( light.lightType & LIGHT_BRAKELIGHT ) ) {
				light.on = nightLightOn;
			}

			parent->SetShaderParm( 5, nightLightOn ? 1.f : 0.f );
			light.renderLight.shaderParms[ 5 ] = renderEnt->shaderParms[ 5 ];
		}
	} else {
		light.on = false;

		if( light.lightType & LIGHT_BRAKELIGHT ) {
			parent->SetShaderParm( 6, 0.f );
		}

		if( light.lightType & LIGHT_STANDARD ) {
			parent->SetShaderParm( 5, 0.f );
		}
	}

	if( !light.on ) {

		if ( light.lightHandle != -1 ) {
			gameRenderWorld->FreeLightDef( light.lightHandle );
			light.lightHandle = -1;
		}
		return;

	} else if ( light.lightHandle == -1 ) {

		light.lightHandle = gameRenderWorld->AddLightDef( &light.renderLight );

	}

	if( light.lightHandle == -1 ) {
		return;
	}

	gameRenderWorld->UpdateLightDef( light.lightHandle, &light.renderLight );
}

/*
================
sdVehicleLightSystem::NightLightIsOn
================
*/
bool sdVehicleLightSystem::NightLightIsOn() const {
	const sdDeclAtmosphere* atmosphere = gameRenderWorld->GetAtmosphere();

	if ( atmosphere != NULL && atmosphere->IsNight() ) {
		return true;
	} else {
		return false;
	}
}

/*
================
sdVehicleLightSystem::AddLight
================
*/
void sdVehicleLightSystem::AddLight( const vehicleLightInfo_t& info ) {
	vehicleLightData_t& light = lights.Alloc();

	memset( &light.renderLight, 0, sizeof( light.renderLight ) );

	light.renderLight.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
	light.renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	light.renderLight.axis.Identity();

	light.renderLight.maxVisDist							= info.maxVisDist;

	light.renderLight.flags.pointLight						= info.pointlight;
	light.renderLight.material								= declHolder.declMaterialType.LocalFind( info.shader, false ); // default texture, same as for idLight
	light.renderLight.shaderParms[ SHADERPARM_RED ]			= info.color[ 0 ];
	light.renderLight.shaderParms[ SHADERPARM_GREEN ]		= info.color[ 1 ];
	light.renderLight.shaderParms[ SHADERPARM_BLUE ]		= info.color[ 2 ];
	light.renderLight.lightRadius							= info.radius;
	light.renderLight.target								= info.target;
	light.renderLight.up									= info.up;
	light.renderLight.right									= info.right;
	light.renderLight.start									= info.start;
	light.renderLight.end									= info.end;

	light.group			= info.group;
	light.joint			= parent->GetAnimator()->GetJointHandle( info.jointName );
	light.on			= false;
	light.lightType		= info.lightType;
	light.offset		= info.offset;
	light.lightHandle	= -1;
	light.enabled		= false;

	if( info.noSelfShadow ) {
		light.renderLight.lightId = LIGHTID_VEHICLE_LIGHT + parent->entityNumber;
		parent->GetRenderEntity()->suppressShadowInLightID = LIGHTID_VEHICLE_LIGHT + parent->entityNumber;
	}
}

/*
================
sdVehicleLightSystem::UpdatePresentation
================
*/
void sdVehicleLightSystem::UpdatePresentation( void ) {
	int i;
	for( i = 0; i < lights.Num(); i++ ) {
		UpdateLightPresentation( lights[ i ] );
	}		
}

/*
================
sdVehicleLightSystem::Update
================
*/
void sdVehicleLightSystem::Update( const sdVehicleInput& input ) {
	int i;
	for( i = 0; i < lights.Num(); i++ ) {
		UpdateLight( lights[ i ], input );
	}
}

/*
================
sdVehicleLightSystem::SetEnabled
================
*/
void sdVehicleLightSystem::SetEnabled( int group, bool enabled ) {
	int i;
	for ( i = 0; i < lights.Num(); i++ ) {
		if ( group == -1 || lights[ i ].group == group ) {
			lights[ i ].enabled = enabled;
		}
	}
}
