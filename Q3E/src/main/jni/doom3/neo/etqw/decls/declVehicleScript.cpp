// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "declVehicleScript.h"
#include "../../framework/CVarSystem.h"
#include "../proficiency/StatsTracker.h"

#include "../../decllib/declTypeHolder.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclVehicleScript

===============================================================================
*/

/*
================
sdDeclVehicleScript::sdDeclVehicleScript
================
*/
sdDeclVehicleScript::sdDeclVehicleScript( void ) {
}

/*
================
sdDeclVehicleScript::~sdDeclVehicleScript
================
*/
sdDeclVehicleScript::~sdDeclVehicleScript( void ) {
}

/*
================
sdDeclVehicleScript::FreeData
================
*/
void sdDeclVehicleScript::FreeData( void ) {
	engineSounds.DeleteContents( true );
	lights.DeleteContents( true );
	positions.DeleteContents( true );
	exits.DeleteContents( true );

	int i;
	for ( i = 0; i < parts.Num(); i++ ) {
		delete parts[ i ].part;
	}
	parts.Clear();

	cockpitInfo.Clear();
}




/*
================
sdDeclVehicleExit::SetDefault
================
*/
void sdDeclVehicleExit::SetDefault( void ) {
}

/*
================
sdDeclVehicleLight::SetDefault
================
*/
void sdDeclVehicleLight::SetDefault( void ) {
	lightInfo.group			= -1;

	lightInfo.jointName		= "";
	lightInfo.lightType		= LIGHT_NONE;
	lightInfo.maxVisDist	= 2048.f;
	lightInfo.offset.Set( 0.f, 0.f, 0.f );

	lightInfo.pointlight	= false;
	lightInfo.shader		= "lights/squarelight1";
	lightInfo.color.Set( 1.f, 1.f, 1.f );
	lightInfo.radius.Set( 300.f, 300.f, 300.f );
	lightInfo.target.Set( 1280.f, 0.f, 0.f );
	lightInfo.up.Set( 0.f, 960.f, 0.f );
	lightInfo.right.Set( 0.f, 0.f, -640.f );
	lightInfo.start			= vec3_origin;
	lightInfo.end			= lightInfo.target;
}

/*
================
sdDeclVehicleEngineSound::SetDefault
================
*/
void sdDeclVehicleEngineSound::SetDefault( void ) {
	memset( &soundInfo, 0, sizeof( soundInfo ) );

	soundInfo.lowDB = -25;
}

/*
================
sdDeclVehiclePosition::sdDeclVehiclePosition
================
*/
sdDeclVehiclePosition::sdDeclVehiclePosition( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		oldCameraMode[ i ] = -1;
	}
}

/*
================
sdDeclVehiclePosition::SetDefault
================
*/
void sdDeclVehiclePosition::SetDefault( void ) {
	positionInfo.hudname		= NULL;
	positionInfo.name			= "";
}

/*
================
sdDeclVehiclePosition::ClearView
================
*/
void sdDeclVehiclePosition::ClearView( positionViewMode_t& view ) {
	view.allowDamping		= true;
	view.hideVehicle		= false;
	view.tophatRequired		= false;
	view.followYaw			= false;
	view.followPitch		= false;
	view.hideHud			= false;
	view.thirdPerson		= false;
	view.autoCenter			= false;
	view.showCockpit		= false;
	view.isInterior			= false;
	view.showCrosshairInThirdPerson = false;
	view.hideDecoyInfo		= false;
	view.showTargetingInfo	= false;
	view.playerShadow		= false;
	view.noCockpitShadows   = false;
	view.showOtherPassengers = true;
	view.matchPrevious		= true;

	view.foliageDepthHack	= 0.f;
	view.damageScale		= 0.0f;

	view.cameraDistance		= pm_thirdPersonRange.GetFloat();
	view.cameraHeight		= pm_thirdPersonHeight.GetFloat();
	view.cameraFocus		= THIRD_PERSON_FOCUS_DISTANCE;
	view.cameraFocusHeight	= 0.0f;
	view.eyes				= "";
	view.eyePivot			= "";
	view.dampCopyFactor		= idVec3( 0.f, 1.f, 0.f );
	view.dampSpeed			= 0.25f;

	ClearClamp( view.clampPitch );
	ClearClamp( view.clampYaw );
	ClearClamp( view.clampDampedPitch );
	ClearClamp( view.clampDampedYaw );
}

/*
================
sdDeclVehiclePosition::ClearWeapon
================
*/
void sdDeclVehiclePosition::ClearWeapon( vehicleWeaponInfo_t& weapon ) {
	weapon.name				= "";
	weapon.weaponDef		= NULL;
	weapon.weaponType		= "";

	ClearClamp( weapon.clampPitch );
	ClearClamp( weapon.clampYaw );
}

/*
================
sdDeclVehiclePosition::ClearIKSystem
================
*/
void sdDeclVehiclePosition::ClearIKSystem( vehicleIKSystemInfo_t& ikSystem ) {
	ikSystem.name		= "";
	ikSystem.ikParms.Clear();
	ikSystem.ikType		= "";

	ClearClamp( ikSystem.clampPitch );
	ClearClamp( ikSystem.clampYaw );
}


/*
================
sdDeclVehiclePosition::ClearClamp
================
*/
void sdDeclVehiclePosition::ClearClamp( angleClamp_t& clamp ) {
	memset( &clamp, 0, sizeof( clamp ) );
}

/*
================
sdDeclVehiclePosition::SetCameraMode
================
*/
void sdDeclVehiclePosition::SetCameraMode( int clientIndex, int cameraMode ) const {
	oldCameraMode[ clientIndex ] = cameraMode;
}

/*
================
sdDeclVehiclePosition::GetCameraMode
================
*/
int sdDeclVehiclePosition::GetCameraMode( int clientIndex ) const {
	return oldCameraMode[ clientIndex ];
}





/*
================
sdDeclVehiclePart::SetDefault
================
*/
void sdDeclVehiclePart::SetDefault( void ) {
}

/*
================
sdDeclVehiclePart::TouchMedia
================
*/
void sdDeclVehiclePart::TouchMedia( void ) {
	game->CacheDictionaryMedia( data );
}

/*
===============================================================================

	sdDeclVehicleScript

===============================================================================
*/

/*
================
sdDeclVehicleScript::ParseView
================
*/
bool sdDeclVehicleScript::ParseView( positionViewMode_t& view, idParser& src ) {
	idToken token;
	bool error = false;

	if( !src.ReadToken( &token ) || token.Cmp( "{" ) ) {
		return false;
	}

	while ( true ) {

		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "hideVehicle" ) ) {

			view.hideVehicle = true;

		} else if ( !token.Icmp( "followPitch" ) ) {

			view.followPitch = true;			

		} else if ( !token.Icmp( "showCrosshairInThirdPerson" ) ) {

			view.showCrosshairInThirdPerson = true;			

		} else if ( !token.Icmp( "followYaw" ) ) {

			view.followYaw = true;			

		} else if ( !token.Icmp( "tophatRequired" ) ) {

			view.tophatRequired = true;			

		} else if ( !token.Icmp( "disableDamping" ) ) {

			view.allowDamping = false;			
			
		} else if ( !token.Icmp( "autoCenter" ) ) {

			view.autoCenter = true;

		} else if ( !token.Icmp( "showCockpit" ) ) {

			view.showCockpit = true;

		} else if ( !token.Icmp( "foliageDepthHack" ) ) {

			view.foliageDepthHack = src.ParseFloat( &error );
			if( error ) {
				return false;
			}

		} else if ( !token.Icmp( "interior" ) ) {

			view.isInterior = true;

		} else if ( !token.Icmp( "thirdPerson" ) ) {

			view.thirdPerson = true;

		} else if ( !token.Icmp( "playerShadow" ) ) {

			view.playerShadow = true;

		} else if ( !token.Icmp( "noCockpitShadows" ) ) {

			view.noCockpitShadows = true;

		} else if ( !token.Icmp( "noShowOtherPlayers" ) ) {

			view.showOtherPassengers = false;

		} else if ( !token.Icmp( "noMatchPrevious" ) ) {

			view.matchPrevious = false;

		} else if ( !token.Icmp( "hideHud" ) ) {

			view.hideHud = true;

		} else if ( !token.Icmp( "hideDecoyInfo" ) ) {

			view.hideDecoyInfo = true;

		} else if ( !token.Icmp( "showTargetingInfo" ) ) {

			view.showTargetingInfo = true;

		} else if ( !token.Icmp( "cameraDistance" ) ) {
			
			view.cameraDistance = src.ParseFloat( &error );
			if( error ) {
				return false;
			}

		} else if ( !token.Icmp( "dampSpeed" ) ) {
			
			view.dampSpeed = src.ParseFloat( &error );
			if( error ) {
				return false;
			}

		} else if ( !token.Icmp( "dampCopyFactor" ) ) {
			
			if ( !src.Parse1DMatrix( 3, view.dampCopyFactor.ToFloatPtr() ) ) {
				return false;
			}

		} else if ( !token.Icmp( "cameraFocus" ) ) {

			view.cameraFocus = src.ParseFloat( &error );
			if( error ) {
				return false;
			}

		} else if ( !token.Icmp( "cameraFocusHeight" ) ) {

			view.cameraFocusHeight = src.ParseFloat( &error );
			if( error ) {
				return false;
			}
			
		} else if( !token.Icmp( "sensitivityYaw" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.sensitivityYaw = token;

		} else if( !token.Icmp( "sensitivityPitch" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.sensitivityPitch = token;

		} else if( !token.Icmp( "sensitivityYawScale" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.sensitivityYawScale = token;

		} else if( !token.Icmp( "sensitivityPitchScale" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.sensitivityPitchScale = token;

		} else if( !token.Icmp( "zoomOutSound" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.zoomOutSound = gameLocal.declSoundShaderType[ token ];

		} else if( !token.Icmp( "zoomInSound" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.zoomInSound = gameLocal.declSoundShaderType[ token ];

		} else if( !token.Icmp( "zoomTable" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.zoomTable = token;
			
		} else if( !token.Icmp( "type" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.type = token;

		} else if( !token.Icmp( "eyeJointPivot" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.eyePivot = token;

		} else if( !token.Icmp( "eyeJoint" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			view.eyes = token;

		} else if ( !token.Icmp( "cameraHeight" ) ) {

			view.cameraHeight = src.ParseFloat( &error );
			if( error ) {
				return false;
			}

		} else if( !token.Icmp( "clamp" ) ) {

			if( !src.ReadToken( &token ) ) {
				return false;
			}

			if( !token.Icmp( "yaw" ) ) {

				if( !ParseClamp( view.clampYaw, src ) ) {
					return false;
				}

			} else if ( !token.Icmp( "pitch" ) ) {

				if( !ParseClamp( view.clampPitch, src ) ) {
					return false;
				}

			} else if( !token.Icmp( "dampedYaw" ) ) {

				if( !ParseClamp( view.clampDampedYaw, src ) ) {
					return false;
				}

			} else if ( !token.Icmp( "dampedPitch" ) ) {

				if( !ParseClamp( view.clampDampedPitch, src ) ) {
					return false;
				}

			} else {
				src.Error( "sdDeclVehicleScript::ParseView Invalid Parameter for 'clamp'" );
				return false;
			}

		} else {

			src.Error( "sdDeclVehicleScript::ParseView Unknown Parameter %s", token.c_str() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclVehicleScript::ParseClamp
================
*/
bool sdDeclVehicleScript::ParseClamp( angleClamp_t& clamp, idParser& src ) {
	idToken token;

	if( !src.ReadToken( &token ) || token.Cmp( "{" ) ) {
		return false;
	}

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "min" ) ) {
			bool error;

			clamp.flags.enabled = true;
			clamp.extents[ 0 ] = src.ParseFloat( &error );

			if( error ) {
				src.Warning( "sdDeclVehicleScript::ParsePositionClamp Invalid Parms for min" );
				return false;
			}
		} else if ( !token.Icmp( "max" ) ) {
			bool error;

			clamp.flags.enabled = true;
			clamp.extents[ 1 ] = src.ParseFloat( &error );

			if( error ) {
				src.Warning( "sdDeclVehicleScript::ParsePositionClamp Invalid Parms for max" );
				return false;
			}
		} else if ( !token.Icmp( "rate" ) ) {
			bool error;

			clamp.flags.limitRate = true;

			float rate = src.ParseFloat( &error );
			clamp.rate[ 0 ] = rate;
			clamp.rate[ 1 ] = rate;

			if( error ) {
				src.Warning( "sdDeclVehicleScript::ParsePositionClamp Invalid Parms for rate" );
				return false;
			}
		} else if ( !token.Icmp( "filter" ) ) {
			bool error;

			float filter = src.ParseFloat( &error );
			clamp.filter = filter;

			if( error ) {
				src.Warning( "sdDeclVehicleScript::ParsePositionClamp Invalid Parms for filter" );
				return false;
			}
		} else if ( token.Icmp( "sound" ) == 0 ) {
			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Warning( "sdDeclVehicleScript::ParsePositionClamp Invalid Parms for sound" );
				return false;
			}

			clamp.sound = gameLocal.declSoundShaderType[ token.c_str() ];
			if ( clamp.sound == NULL ) {
				src.Warning( "sdDeclVehicleScript::ParsePositionClamp Unknown Sound '%s'", token.c_str() );
				return false;
			}
		} else {
			src.Error( "sdDeclVehicleScript::ParsePositionClamp Unknown Parameter %s", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
sdDeclVehicleScript::ParseIKSystem
================
*/
bool sdDeclVehicleScript::ParseIKSystem( vehicleIKSystemInfo_t& ikSystem, idParser& src ) {
	idToken token;

	if( !src.ReadToken( &token ) || token.Cmp( "{" ) ) {
		return false;
	}

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "name" ) ) {

			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclVehicleScript::ParseIKSystem Invalid Parameter for 'name'" );
				return false;
			}

			ikSystem.name = token;

		} else if ( !token.Icmp( "parms" ) ) {

			if ( !ikSystem.ikParms.Parse( src ) ) {
				src.Error( "sdDeclVehicleScript::ParseIKSystem Invalid IK Parms" );
				return false;
			}

			gameLocal.CacheDictionaryMedia( ikSystem.ikParms );

		} else if ( !token.Icmp( "type" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclVehicleScript::ParseWeapon Invalid Parameter for 'type'" );
				return false;
			}			

			ikSystem.ikType = token;

		} else if( !token.Icmp( "clamp" ) ) {

			if( !src.ReadToken( &token ) ) {
				return false;
			}

			if( !token.Icmp( "yaw" ) ) {

				if( !ParseClamp( ikSystem.clampYaw, src ) ) {
					return false;
				}

			} else if ( !token.Icmp( "pitch" ) ) {

				if( !ParseClamp( ikSystem.clampPitch, src ) ) {
					return false;
				}

			} else {
				src.Error( "sdDeclVehicleScript::ParseIKSystem Invalid Parameter for 'clamp'" );
				return false;
			}

		} else {

			src.Error( "sdDeclVehicleScript::ParseIKSystem Unknown Parameter %s", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
sdDeclVehicleScript::ParseWeapon
================
*/
bool sdDeclVehicleScript::ParseWeapon( vehicleWeaponInfo_t& weapon, idParser& src ) {
	idToken token;

	if( !src.ReadToken( &token ) || token.Cmp( "{" ) ) {
		return false;
	}

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if ( !token.Icmp( "name" ) ) {

			if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclVehicleScript::ParseWeapon Invalid Parameter for 'name'" );
				return false;
			}

			weapon.name = token;

		} else if ( !token.Icmp( "weapon" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclVehicleScript::ParseWeapon Invalid Parameter for 'weapon'" );
				return false;
			}			

			weapon.weaponDef = gameLocal.declStringMapType[ token ];
			if ( !weapon.weaponDef ) {
				src.Error( "sdDeclVehicleScript::ParseWeapon Invalid Parameter '%s' for 'weapon'", token.c_str() );
				return false;
			}

			game->CacheDictionaryMedia( weapon.weaponDef->GetDict() );

		} else if ( !token.Icmp( "type" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclVehicleScript::ParseWeapon Invalid Parameter for 'type'" );
				return false;
			}			

			weapon.weaponType = token;

		} else if( !token.Icmp( "clamp" ) ) {

			if( !src.ReadToken( &token ) ) {
				return false;
			}

			if( !token.Icmp( "yaw" ) ) {

				if( !ParseClamp( weapon.clampYaw, src ) ) {
					return false;
				}

			} else if ( !token.Icmp( "pitch" ) ) {

				if( !ParseClamp( weapon.clampPitch, src ) ) {
					return false;
				}

			} else {
				src.Error( "sdDeclVehicleScript::ParseWeapon Invalid Parameter for 'clamp'" );
				return false;
			}

		} else {

			src.Error( "sdDeclVehicleScript::ParseWeapon Unknown Parameter %s", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
sdDeclVehicleScript::ParsePositionToken
================
*/
bool sdDeclVehicleScript::ParsePositionToken( sdDeclVehiclePosition* position, idParser& src, idToken& token ) {

	if( !token.Icmp( "name" ) ) {
		
		if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}

		position->positionInfo.name = token;

	} else if( !token.Icmp( "data" ) ) {
		
		if ( !position->positionInfo.data.Parse( src ) ) {
			return false;
		}

		game->CacheDictionaryMedia( position->positionInfo.data );

	} else if( !token.Icmp( "hudname" ) ) {
		
		if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}

		position->positionInfo.hudname = declHolder.FindLocStr( token.c_str() );

	} else if( !token.Icmp( "weapon" ) ) {

		vehicleWeaponInfo_t& weapon = position->positionInfo.weapons.Alloc();
		position->ClearWeapon( weapon );

		if( !ParseWeapon( weapon, src ) ) {
			return false;
		}

	} else if( !token.Icmp( "ik" ) ) {

		vehicleIKSystemInfo_t& ikSystem = position->positionInfo.ikSystems.Alloc();
		position->ClearIKSystem( ikSystem );

		if( !ParseIKSystem( ikSystem, src ) ) {
			return false;
		}

	} else if( !token.Icmp( "view" ) ) {

		positionViewMode_t& view = position->positionInfo.views.Alloc();
		position->ClearView( view );

		if( !ParseView( view, src ) ) {
			return false;
		}

	} else {

		return false;

	}

	return true;
}

/*
================
sdDeclVehicleScript::ParseEngineSoundToken
================
*/
bool sdDeclVehicleScript::ParseEngineSoundToken( sdDeclVehicleEngineSound* engineSound, idParser& src, idToken& token ) {
	if( !token.Icmp( "sound" ) ) {

		if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}

		engineSound->soundInfo.soundFile = token;

	} else if( !token.Icmp( "volumeMin" ) ) {

		bool error;
		engineSound->soundInfo.lowDB = src.ParseFloat( &error );
		
		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "volumeMax" ) ) {

		bool error;
		engineSound->soundInfo.highDB = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "low" ) ) {

		bool error;
		engineSound->soundInfo.lowRev = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "high" ) ) {

		bool error;
		engineSound->soundInfo.highRev = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "fadeIn" ) ) {

		bool error;
		engineSound->soundInfo.leadIn = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "fadeOut" ) ) {

		bool error;
		engineSound->soundInfo.leadOut = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "lowFrequency" ) ) {

		bool error;
		engineSound->soundInfo.minFreqshift = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "highFrequency" ) ) {

		bool error;
		engineSound->soundInfo.maxFreqshift = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "frequencyChangeStart" ) ) {

		bool error;
		engineSound->soundInfo.fsStart = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "frequencyChangeStop" ) ) {

		bool error;
		engineSound->soundInfo.fsStop = src.ParseFloat( &error );

		if( error ) {
			return false;
		}

	} else if( !token.Icmp( "joint" ) ) {

		if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}

		engineSound->soundInfo.jointName = token;

	} else {

		return false;

	}

	return true;
}

/*
================
sdDeclVehicleScript::ParseExitToken
================
*/
bool sdDeclVehicleScript::ParseExitToken( sdDeclVehicleExit* exit, idParser& src, idToken& token ) {
	if( !token.Icmp( "joint" ) ) {

		if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
			return false;
		}
		exit->exitInfo.joint = token;

	} else {

		return false;

	}

	return true;
}

/*
================
sdDeclVehicleScript::ParseLightToken
================
*/
bool sdDeclVehicleScript::ParseLightToken( sdDeclVehicleLight* light, idParser& src, idToken& token ) {
	
	if( !token.Icmp( "group" ) ) {

		light->lightInfo.group = src.ParseInt();

	} else if( !token.Icmp( "joint" ) ) {

		if( !src.ReadToken( &token ) ) {
			src.Warning( "missing parameter for 'joint' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.jointName = token;

	} else if ( !token.Icmp( "lightType" ) ) {

		if( !src.ReadToken( &token ) ) {
			src.Warning( "missing parameter for 'lightType' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		if ( !token.Icmp( "standard" ) ) {

			light->lightInfo.lightType |= LIGHT_STANDARD;

		} else if ( !token.Icmp( "brake" ) ) {

			light->lightInfo.lightType |= LIGHT_BRAKELIGHT;

		}

	} else if ( !token.Icmp( "shader" ) ) {

		if( !src.ReadToken( &token ) ) {
			src.Warning( "missing parameter for 'shader' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.shader = token;

	} else if ( !token.Icmp( "color" ) ) {

		if( !src.Parse1DMatrix( 3, light->lightInfo.color.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'color' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

	} else if ( !token.Icmp( "radius" ) ) {

		idVec3 radius;

		if( !src.Parse1DMatrix( 3, radius.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'radius' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.radius = radius;

	} else if ( !token.Icmp( "target" ) ) {

		idVec3 target;

		if( !src.Parse1DMatrix( 3, target.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'target' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.target = target;

	} else if ( !token.Icmp( "up" ) ) {

		idVec3 up;

		if( !src.Parse1DMatrix( 3, up.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'up' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.up = up;

	} else if ( !token.Icmp( "right" ) ) {
		idVec3 right;

		if( !src.Parse1DMatrix( 3, right.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'right' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.right = right;

	} else if ( !token.Icmp( "start" ) ) {

		idVec3 start;

		if( !src.Parse1DMatrix( 3, start.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'start' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.start = start;

	} else if( !token.Icmp( "end" ) ) {

		idVec3 end;

		if( !src.Parse1DMatrix( 3, end.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'end' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.end = end;

	} else if( !token.Icmp( "pointLight" ) ) {

		light->lightInfo.pointlight = true;

	} else if( !token.Icmp( "maxVisDist" ) ) {

		light->lightInfo.maxVisDist = src.ParseFloat();

	} else if( !token.Icmp( "noSelfShadow" ) ) {

		light->lightInfo.noSelfShadow = true;

	} else if( !token.Icmp( "offset" ) ) {

		idVec3 offset;

		if( !src.Parse1DMatrix( 3, offset.ToFloatPtr() ) ) {
			src.Warning( "missing or bad parameter for 'offset' keyword in vehicle script '%s'", base->GetName() );
			return false;
		}

		light->lightInfo.offset = offset;

	} else {

		return false;
	}

	return true;
}

/*
================
sdDeclVehicleScript::ParseItem
================
*/

template< typename T >
bool sdDeclVehicleScript::ParseItem( idParser& src, idList< T* >& list, bool ( sdDeclVehicleScript::*FUNC )( T* item, idParser& src, idToken& token ) ) {
	idToken token;
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	T* item = new T;
	list.Alloc() = item;
	item->SetDefault();

	while( src.ReadToken( &token ) ) {
		if( !token.Cmp( "}" ) ) {
			break;
		}

		if( ( *this.*FUNC )( item, src, token ) ) {
			continue;
		} else {
			return false;
		}
	}

	return true;
}

/*
================
sdDeclVehicleScript::ParsePart
================
*/

bool sdDeclVehicleScript::ParsePart( idParser& src, vehiclePartType_t type ) {
	vehiclePart_t& part = parts.Alloc();
	part.part = new sdDeclVehiclePart();
	part.part->SetDefault();
	part.type = type;

	bool retVal = part.part->data.Parse( src );
	if( retVal ) {
		game->CacheDictionaryMedia( part.part->data );
	}

	return retVal;
}

typedef struct vehiclePartId_s {
	const char*			name;
	vehiclePartType_t	type;
} vehiclePartId_t;

vehiclePartId_t partIds[] = {
	{ "part",			VPT_PART },
	{ "simplePart",		VPT_SIMPLE_PART },
	{ "scriptedPart",	VPT_SCRIPTED_PART },
	{ "wheel",			VPT_WHEEL },
	{ "rotor",			VPT_ROTOR },
	{ "hover",			VPT_HOVER },
	{ "mass",			VPT_MASS },
	{ "track",			VPT_TRACK },
	{ "thruster",		VPT_THRUSTER },
	{ "suspension",		VPT_SUSPENSION },
	{ "vtol",			VPT_VTOL },
	{ "antigrav",		VPT_ANTIGRAV },
	{ "pseudoHover",	VPT_PSEUDO_HOVER },
	{ "dragPlane",		VPT_DRAGPLANE },
	{ "rudder",			VPT_RUDDER },
	{ "airBrake",		VPT_AIRBRAKE },
	{ "hurtZone",		VPT_HURTZONE },
	{ "antiRoll",		VPT_ANTIROLL },
	{ "antiPitch",		VPT_ANTIPITCH },
};

int numPartIds = _arraycount( partIds );

/*
================
sdDeclVehicleScript::Parse
================
*/
bool sdDeclVehicleScript::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	while( src.ReadToken( &token ) ) {
		if( !token.Icmp( "exitDef" ) ) {

			if( !ParseItem< sdDeclVehicleExit >( src, exits, &sdDeclVehicleScript::ParseExitToken ) ) {
				return false;
			}
			continue;

		} else if( !token.Icmp( "positionDef" ) ) {

			if( !ParseItem< sdDeclVehiclePosition >( src, positions, &sdDeclVehicleScript::ParsePositionToken ) ) {
				return false;
			}
			continue;

		} else if( !token.Icmp( "engineSoundDef" ) ) {

			if( !ParseItem< sdDeclVehicleEngineSound >( src, engineSounds, &sdDeclVehicleScript::ParseEngineSoundToken ) ) {
				return false;
			}
			continue;

		} else if( !token.Icmp( "lightDef" ) ) {

			if( !ParseItem< sdDeclVehicleLight >( src, lights, &sdDeclVehicleScript::ParseLightToken ) ) {
				return false;
			}
			continue;

		} else if ( !token.Icmp( "cockpit" ) ) {

			if ( !src.ReadToken( &token ) ) {
				return false;
			}

			sdPair< idStr, idDict >& info = cockpitInfo.Alloc();

			info.first = token;
			
			if ( !info.second.Parse( src ) ) {
				return false;
			}

			game->CacheDictionaryMedia( info.second );
			continue;

		}

		int i;
		for ( i = 0; i < numPartIds; i++ ) {
			vehiclePartId_t& partId = partIds[ i ];

			if ( !token.Icmp( partId.name ) ) {
				if ( !ParsePart( src, partId.type ) ) {
					src.Error( "sdDeclVehicleScript::Parse Error Parsing Part of Type '%s'", partId.name );
					return false;
				}
				break;
			}
		}
		if ( i != numPartIds ) {
			continue;
		}

		if( token == "}" ) {

			break;

		} else {

			src.Error( "sdDeclVehicleScript::Parse Unknown Keyword %s", token.c_str() );
			return false;

		}
	}

	TouchMedia();

	return true;
}

/*
================
sdDeclVehicleScript::CacheFromDict
================
*/
void sdDeclVehicleScript::CacheFromDict( const idDict& dict ) {
	const idKeyValue *kv;

	kv = NULL;
	while( kv = dict.MatchPrefix( "vs", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declVehicleScriptDefType[ kv->GetValue() ];
		}
	}
}

/*
================
sdDeclVehicleScript::Parse
================
*/
void sdDeclVehicleScript::TouchMedia( void ) const {
	int i;

	for ( i = 0; i < lights.Num(); i++ ) {
		lights[i]->TouchMedia();
	}

	for ( i = 0; i < parts.Num(); i++ ) {
		parts[ i ].part->TouchMedia();
	}
}

/*
================
sdDeclVehicleScript::ResetCameraMode
================
*/
void sdDeclVehicleScript::ResetCameraMode( int clientIndex ) const {
	for ( int i = 0; i < positions.Num(); i++ ) {
		SetCameraMode( clientIndex, i, -1 );
	}
}

/*
================
sdDeclVehicleScript::GetCameraMode
================
*/
int sdDeclVehicleScript::GetCameraMode( int clientIndex, int positionIndex ) const {
	if ( positionIndex < 0 || positionIndex >= positions.Num() ) {
		assert( false );
		return -1;
	}

	return positions[ positionIndex ]->GetCameraMode( clientIndex );
}

/*
================
sdDeclVehicleScript::SetCameraMode
================
*/
void sdDeclVehicleScript::SetCameraMode( int clientIndex, int positionIndex, int cameraMode ) const {
	if ( positionIndex < 0 || positionIndex >= positions.Num() ) {
		assert( false );
		return;
	}

	positions[ positionIndex ]->SetCameraMode( clientIndex, cameraMode );
}
