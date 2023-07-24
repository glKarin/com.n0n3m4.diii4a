// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "JetPack.h"
#include "TransportComponents.h"
#include "../ContentMask.h"
#include "../client/ClientEntity.h"
#include "VehicleView.h"
#include "VehicleControl.h"



#define ENABLE_JP_FLOAT_CHECKS
#if defined( ENABLE_JP_FLOAT_CHECKS )
	#undef FLOAT_CHECK_BAD
	#undef VEC_CHECK_BAD

#define FLOAT_CHECK_BAD( x ) \
	if ( FLOAT_IS_NAN( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is NAN", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_INF( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is INF", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_IND( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is IND", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_DENORMAL( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is DEN", __FILE__, __LINE__ ); \


	#define VEC_CHECK_BAD( vec ) FLOAT_CHECK_BAD( ( vec ).x ); FLOAT_CHECK_BAD( ( vec ).y ); FLOAT_CHECK_BAD( ( vec ).z );
	#define MAT_CHECK_BAD( m ) VEC_CHECK_BAD( m[ 0 ] ); VEC_CHECK_BAD( m[ 1 ] ); VEC_CHECK_BAD( m[ 2 ] );
	#define ANG_CHECK_BAD( ang ) FLOAT_CHECK_BAD( ( ang ).pitch ); FLOAT_CHECK_BAD( ( ang ).roll ); FLOAT_CHECK_BAD( ( ang ).yaw );
#else
	#define MAT_CHECK_BAD( m )
#endif


/*
===============================================================================

	sdJetPack

===============================================================================
*/

extern const idEventDef EV_GetViewAngles;

const idEventDef EV_GetChargeFraction( "getChargeFraction", 'f', DOC_TEXT( "Returns the fraction of boost charge left." ), 0, NULL );

CLASS_DECLARATION( sdTransport, sdJetPack )
	EVENT( EV_GetChargeFraction,			sdJetPack::Event_GetChargeFraction )
	EVENT( EV_GetViewAngles,				sdJetPack::Event_GetViewAngles )
END_CLASS

CLASS_DECLARATION( sdClientAnimated, sdJetPackVisuals )
END_CLASS

/*
================
sdJetPack::sdJetPack
================
*/
sdJetPack::sdJetPack( void ) {
	lastAngles.Zero();
	visuals = NULL;
	collideDamage = NULL;
	fallDamage = NULL;
}

/*
================
sdJetPack::~sdJetPack
================
*/
sdJetPack::~sdJetPack( void ) {
	positionManager.EjectAllPlayers();

	if ( visuals.IsValid() ) {
		visuals->Dispose();
		visuals = NULL;
	}
}

/*
================
sdJetPack::DoLoadVehicleScript
================
*/
void sdJetPack::DoLoadVehicleScript( void ) {
	LoadParts( VPT_SIMPLE_PART | VPT_ANTIGRAV | VPT_SCRIPTED_PART );
}

/*
================
sdJetPack::LoadParts
================
*/
void sdJetPack::LoadParts( int partTypes ) {
	ClearDriveObjects();

	int i;

	if ( partTypes & VPT_SIMPLE_PART ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_SIMPLE_PART ) {
				continue;
			}

			sdVehiclePartSimple* part = new sdVehiclePartSimple;
			part->SetIndex( driveObjects.Num() );
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_ANTIGRAV ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_ANTIGRAV ) {
				continue;
			}

			sdVehicleRigidBodyAntiGrav* part = new sdVehicleRigidBodyAntiGrav;
			part->SetIndex( driveObjects.Num() );
			part->Init( *vehicleScript->parts[ i ].part, this );
			part->SetClientParent( visuals );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_SCRIPTED_PART ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_SCRIPTED_PART ) {
				continue;
			}

			sdVehiclePartScripted* part = new sdVehiclePartScripted;
			part->SetIndex( driveObjects.Num() );
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}
}

/*
================
sdJetPack::Spawn
================
*/
void sdJetPack::Spawn( void ) {
	maxJumpCharge		= spawnArgs.GetFloat( "max_jump_charge", "100" );
	currentJumpCharge	= maxJumpCharge;
	dischargeRate		= spawnArgs.GetFloat( "rate_discharge", "10" );
	chargeRate			= spawnArgs.GetFloat( "rate_charge", "5" );

	float maxStepHeight;
	if ( !spawnArgs.GetFloat( "max_step_height", "0", maxStepHeight ) ) {
		maxStepHeight = pm_stepsize.GetFloat();
	}

	physicsObj.SetMaxStepHeight( maxStepHeight );

	lastAngles = GetPhysics()->GetAxis().ToAngles();
	lastAxis = GetPhysics()->GetAxis();

	ANG_CHECK_BAD( lastAngles );
	MAT_CHECK_BAD( lastAxis );

	physicsObj.SetSelf( this );

	//
	// Set the main clip model
	//
	idBounds bounds;
	idVec3 cmbounds = spawnArgs.GetVector( "cm_bounds" );
	bounds[ 0 ].Set( -cmbounds.x, -cmbounds.x, cmbounds.y );
	bounds[ 1 ].Set( cmbounds.x, cmbounds.x, cmbounds.z );

	idClipModel* newClip = new idClipModel( idTraceModel( bounds ), false );
	newClip->SetContents( CONTENTS_SLIDEMOVER );
	newClip->Translate( physicsObj.GetCurrentOrigin(), gameLocal.clip );
	physicsObj.SetClipModel( newClip, 1.0f );
	physicsObj.SetClipMask( MASK_PLAYERSOLID );

	//
	// Set the shot clip model
	//
	idVec3 shotmins;
	idVec3 shotmaxs;
	if ( spawnArgs.GetVector( "cm_shot_mins", "0 0 0", shotmins )
		&& spawnArgs.GetVector( "cm_shot_maxs", "0 0 0", shotmaxs ) ) {
		bounds[ 0 ] = shotmins;
		bounds[ 1 ] = shotmaxs;
	}

	idClipModel* shotClip = new idClipModel( idTraceModel( bounds ), false );
	shotClip->SetContents( CONTENTS_RENDERMODEL );
	shotClip->Translate( physicsObj.GetCurrentOrigin(), gameLocal.clip );
	physicsObj.SetClipModel( shotClip, 1.0f, 1 );

	physicsObj.SetGravity( GetPhysics()->GetGravityNormal() * spawnArgs.GetFloat( "gravity", DEFAULT_GRAVITY_STRING ) );

	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );

	// parameters specific to the jetpack
	physicsObj.SetMaxSpeed( spawnArgs.GetFloat( "max_speed", "320" ) );
	physicsObj.SetMaxBoostSpeed( spawnArgs.GetFloat( "max_boost_speed", "500" ) );
	physicsObj.SetWalkForceScale( spawnArgs.GetFloat( "walk_force_scale", "0.4" ) );
	physicsObj.SetKineticFriction( spawnArgs.GetFloat( "kinetic_friction", "10" ) );
	physicsObj.SetJumpForce( spawnArgs.GetFloat( "jump_force", "4800" ) );
	physicsObj.SetBoostForce( spawnArgs.GetFloat( "boost_force", "1200" ) );

	SetPhysics( &physicsObj );

	// create the visual model
	idDict args;
	args.Set( "classname", spawnArgs.GetString( "def_visuals" ) );
	rvClientEntity* vis = NULL;
	if ( !gameLocal.SpawnClientEntityDef( args, &vis ) && gameLocal.DoClientSideStuff() ) {
		gameLocal.Error( "sdJetPack::Spawn - couldn't spawn jetpack client entity" );
		return;
	}

	visuals = vis;
	if ( visuals ) {
		visuals->SetOrigin( GetPhysics()->GetOrigin() );
	}

	const char* damagename = spawnArgs.GetString( "dmg_collide" );
	if ( *damagename ) {
		collideDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if( !collideDamage ) {
			gameLocal.Warning( "sdJetPack::Spawn Invalid Damage Type '%s'", damagename );
		}
	}

	damagename = spawnArgs.GetString( "dmg_fall" );
	if ( *damagename ) {
		fallDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if( !fallDamage ) {
			gameLocal.Warning( "sdJetPack::Spawn Invalid Damage Type '%s'", damagename );
		}
	}

	minFallDamageSpeed = spawnArgs.GetFloat( "fall_damage_speed_min", "80" );
	maxFallDamageSpeed = spawnArgs.GetFloat( "fall_damage_speed_max", "2000" );

	nextSelfCollisionTime		= gameLocal.time + SEC2MS( 5 ); // 5s spawn invulnerability to allow it to drop to the ground
	nextCollisionTime			= 0;

	LoadVehicleScript();
}

/*
================
sdJetPack::WantsToThink
================
*/
bool sdJetPack::WantsToThink( void ) {
	if ( currentJumpCharge != maxJumpCharge ) {
		return true;
	}
	return sdTransport::WantsToThink();
}

/*
================
sdJetPack::Think
================
*/
void sdJetPack::Think( void ) {
	idPlayer* driver = GetPositionManager().FindDriver();

	usercmd_t cmd;
	if ( driver ) {
		cmd = gameLocal.usercmds[ driver->entityNumber ];
		lastAngles = driver->clientViewAngles;
		ANG_CHECK_BAD( lastAngles );
		physicsObj.SetPlayerInput( cmd, lastAngles, true );
	} else {
		memset( &cmd, 0, sizeof( cmd ) );
		ANG_CHECK_BAD( lastAngles );
		physicsObj.SetPlayerInput( cmd, lastAngles, true );
	}

	if ( cmd.buttons.btn.sprint && !IsTeleporting() && !IsEMPed() ) {
		currentJumpCharge -= MS2SEC( gameLocal.msec ) * dischargeRate;
		if ( currentJumpCharge < 0.f ) {
			currentJumpCharge = 0.f;
		}
		physicsObj.SetBoost( currentJumpCharge == 0.f ? 0.f : 1.0f );
	} else {
		currentJumpCharge += MS2SEC( gameLocal.msec ) * chargeRate;
		if ( currentJumpCharge > maxJumpCharge ) {
			currentJumpCharge = maxJumpCharge;
		}
		physicsObj.SetBoost( 0.0f );
	}

	if ( IsEMPed() ) {
		physicsObj.SetComeToRest( true );
	} else {
		physicsObj.SetComeToRest( false );
	}

	// update the lastAxis
	lastAngles.FixDenormals();
	lastAxis = lastAngles.ToMat3();
	lastAxis.FixDenormals();
	MAT_CHECK_BAD( lastAxis );

	sdTransport::Think();

	LinkCombat();
}

/*
================
sdJetPack::UpdateJetPackVisuals
================
*/
void sdJetPack::UpdateJetPackVisuals( void ) {
	if ( visuals ) {
		if ( !visuals->IsBound() ) {
			idMat3 axis;
			idAngles::YawToMat3( lastAngles.yaw, axis );
			MAT_CHECK_BAD( axis );

			idPlayer* driver = GetPositionManager().FindDriver();
			if ( driver ) {
				idVec3 offset( 4.0f, 0.0f, 39.0f );

				idVec3 baseOrigin = physicsObj.GetOrigin();

				sdTeleporter* teleportEnt = GetTeleportEntity();
				if ( teleportEnt != NULL ) {
					idPlayer* player = gameLocal.GetLocalViewPlayer();
					if ( player != NULL && player->GetProxyEntity() == this ) {
						idEntity* viewer = teleportEnt->GetViewEntity();
						if ( viewer != NULL ) {
							baseOrigin	= viewer->GetPhysics()->GetOrigin();
							axis	= viewer->GetPhysics()->GetAxis();
						}
					}
				}

				idVec3 posToMatchTo;
				idMat3 axisToMatchTo;
				driver->GetAnimator()->GetJointTransform( driver->GetAnimator()->GetJointHandle( "hips" ), gameLocal.time, posToMatchTo, axisToMatchTo );

				const renderEntity_t* re = driver->GetRenderEntity();
				posToMatchTo = posToMatchTo * re->axis + re->origin;		

				idVec3 posToMatch = baseOrigin + offset * axis;
				idVec3 offsetNeeded = posToMatchTo - posToMatch;

				visuals->SetOrigin( baseOrigin + offsetNeeded );
				visuals->SetAxis( axis );
			} else {
				visuals->SetAxis( axis );
				visuals->SetOrigin( physicsObj.GetOrigin() );
			}
		}
	}
}

/*
================
sdJetPack::UpdateModelTransform
================
*/
void sdJetPack::UpdateModelTransform( void ) {
	sdTeleporter* teleportEnt = teleportEntity;
	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( teleportEnt != NULL ) {
		if ( player != NULL && player->GetProxyEntity() == this ) {
			idEntity* viewer = teleportEnt->GetViewEntity();
			if ( viewer ) {
				renderEntity.axis	= viewer->GetPhysics()->GetAxis();
				renderEntity.origin	= viewer->GetPhysics()->GetOrigin();
				return;
			}
		}
	}

	idAngles::YawToMat3( lastAngles.yaw, renderEntity.axis );
	MAT_CHECK_BAD( renderEntity.axis );
	renderEntity.origin	= physicsObj.GetOrigin();

	if ( !player || player->GetProxyEntity() != this ) {
		DoPredictionErrorDecay();
	}
}

/*
================
sdJetPack::Present
================
*/
void sdJetPack::Present( void ) {
	sdTransport::Present();

	// add to interpolate list just after the driver
	idPlayer* driver = GetPositionManager().FindDriver();
	if ( !gameLocal.unlock.unlockedDraw && driver != NULL && driver->interpolateNode.InList() ) {
		interpolateNode.InsertAfter( driver->interpolateNode );
	}

	UpdateJetPackVisuals();
	if ( visuals.IsValid() ) {
		visuals->ClientUpdateView();

		renderEntity.origin = visuals->GetRenderEntity()->origin;
		renderEntity.axis = visuals->GetRenderEntity()->axis;
		lastPushedOrigin = renderEntity.origin;
		lastPushedAxis = renderEntity.axis;
	}

	// update the antigrav parts again so that their effect appears in the right place
	// Yes! This is horrible!
	for ( int i = 0; i < activeObjects.Num(); i++ ) {
		sdVehicleRigidBodyAntiGrav* antiGrav = activeObjects[ i ]->Cast< sdVehicleRigidBodyAntiGrav >();
		if ( antiGrav ) {
			antiGrav->UpdateEffect();
		}
	}
}

/*
================
sdJetPack::GetChargeFraction
================
*/
float sdJetPack::GetChargeFraction( void ) const {
	return currentJumpCharge / maxJumpCharge;
}

/*
================
sdJetPack::Event_GetChargeFraction
================
*/
void sdJetPack::Event_GetChargeFraction( void ) {
	sdProgram::ReturnFloat( GetChargeFraction() );
}

/*
================
sdJetPack::Event_GetViewAngles
================
*/
void sdJetPack::Event_GetViewAngles( void ) {
	sdProgram::ReturnVector( idVec3( lastAngles[ 0 ], lastAngles[ 1 ], lastAngles[ 2 ] ) );
}

/*
================
sdJetPack::SetAxis
================
*/
void sdJetPack::SetAxis( const idMat3& axis ) {
	idPlayer* driver = GetPositionManager().FindDriver();
	if ( driver ) {
		driver->SetAxis( axis );
	}

	lastAngles = axis.ToAngles();
	lastAxis = axis;

	ANG_CHECK_BAD( lastAngles );
	MAT_CHECK_BAD( lastAxis );

//	if ( predictionErrorDecay != NULL ) {
//		predictionErrorDecay->Reset( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
//	}
}

/*
================
sdJetPack::OnGround
================
*/
bool sdJetPack::OnGround() {
	return physicsObj.OnGround();
}

/*
================
sdJetPack::UpdateVisibility
================
*/
void sdJetPack::UpdateVisibility( void ) {
	// jetpack always has hidden main entity
	Hide();

	bool hide			= false;
	bool disableInView	= false;

	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();

	if ( viewPlayer != NULL && viewPlayer->GetProxyEntity() == this ) {
		renderEntity.flags.disableLODs = true;
	} else {
		renderEntity.flags.disableLODs = false;
	}

	if ( vehicleFlags.modelDisabled ) {
		hide = true;
	} else {
		if ( viewPlayer && viewPlayer->GetProxyEntity() == this ) {
			if ( GetViewForPlayer( viewPlayer ).GetViewParms().hideVehicle ) {
				disableInView = true;
			}
		}
	}

	if ( visuals ) {
		sdClientAnimated* animated = visuals->Cast< sdClientAnimated >();

		if ( animated ) {
			if ( hide ) {
				animated->Hide();
			} else if ( disableInView ) {
				animated->Show();

				visuals->GetRenderEntity()->suppressSurfaceInViewID	= viewPlayer->entityNumber + 1;
				visuals->GetRenderEntity()->suppressShadowInViewID	= viewPlayer->entityNumber + 1;
			} else {
				animated->Show();

				visuals->GetRenderEntity()->suppressSurfaceInViewID	= 0;
				visuals->GetRenderEntity()->suppressShadowInViewID	= 0;
			}
		}
	}
}

/*
================
sdJetPack::OnPlayerEntered
================
*/
void sdJetPack::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	sdTransport::OnPlayerEntered( player, position, oldPosition );

	if ( visuals ) {
		visuals->SetOrigin( vec3_origin );
	//	visuals->Bind( player, player->GetAnimator()->GetJointHandle( "Spine1" ) );
	}

	// Gordon: force an update so we don't get a flicker
	Think();
}

/*
================
sdJetPack::OnPlayerExited
================
*/
void sdJetPack::OnPlayerExited( idPlayer* player, int position ) {
	sdTransport::OnPlayerExited( player, position );

	if ( visuals ) {
		visuals->Unbind();
		
		idMat3 axis;
		idAngles::YawToMat3( lastAngles.yaw, axis );
		MAT_CHECK_BAD( axis );
		visuals->SetAxis( axis );
		visuals->SetOrigin( physicsObj.GetOrigin() );
	}

	if ( player == gameLocal.GetLocalViewPlayer() ) {
		ResetPredictionErrorDecay();
	}
}

/*
================
sdJetPack::SetTeleportEntity
================
*/
void sdJetPack::SetTeleportEntity( sdTeleporter* teleporter ) {
	idPlayer* driver = positionManager.FindDriver();
	idMat3 oldAxis;
	if ( driver != NULL ) {
		oldAxis = driver->GetAxis();
		MAT_CHECK_BAD( oldAxis );
	}

	if ( teleporter != NULL ) {
		idEntity* viewer = teleporter->GetViewEntity();
		if ( viewer != NULL ) {
			oldAxis = viewer->GetPhysics()->GetAxis();
			lastAxis = oldAxis;
			lastAngles = lastAxis.ToAngles();
			MAT_CHECK_BAD( lastAxis );
			ANG_CHECK_BAD( lastAngles );
		}
	}

	sdTransport::SetTeleportEntity( teleporter );

	if ( driver != NULL ) {
		driver->SetAxis( oldAxis );
	}
}

/*
================
sdJetPack::LinkCombat
================
*/
void sdJetPack::LinkCombat( void ) {
	renderEntity_t* re = NULL;
	if ( visuals ) {
		re = visuals->GetRenderEntity();
	} else {
		re = GetRenderEntity();
	}
}

/*
================
sdJetPack::Collide
================
*/
bool sdJetPack::Collide( const trace_t &collision, const idVec3 &velocity, int bodyId ) {
	// FIXME: This stuff should be done on collisions against us too.
	idVec3 normal = -collision.c.normal;
	float length = ( velocity * normal );
	idVec3 v = length * normal;
	idEntity* driver = positionManager.FindDriver();
	idPlayer* playerDriver = NULL;
	
	if ( driver != NULL ) {
		playerDriver = driver->Cast< idPlayer >();
		assert( playerDriver != NULL );
	}

	// do falling damage
	if ( length > 40.0f && !( collision.c.material != NULL && collision.c.material->GetSurfaceFlags() & SURF_NODAMAGE ) ) {

		const float VELOCITY_LAND_HARD = 0.7f;
		const float VELOCITY_LAND_MEDIUM = 0.4f;
		const float VELOCITY_LAND_LIGHT = 0.1f;
		const float VELOCITY_LAND_SOFT = 500.0f;

		float damageScale = ( length - minFallDamageSpeed ) / ( maxFallDamageSpeed - minFallDamageSpeed );
		damageScale = idMath::ClampFloat( -1.0f, 1.0f, damageScale );

		// player animations
		if ( playerDriver != NULL ) {
			playerDriver->AI_HARDLANDING = false;
			playerDriver->AI_SOFTLANDING = false;

			if ( damageScale > VELOCITY_LAND_HARD ) {
				playerDriver->AI_HARDLANDING = true;
				StartSound( "snd_fall_hard", SND_VEHICLE_MISC, 0, NULL );
			} else if ( damageScale > VELOCITY_LAND_MEDIUM ) {
				playerDriver->AI_HARDLANDING = true;
				StartSound( "snd_fall_medium", SND_VEHICLE_MISC, 0, NULL );
			} else if ( damageScale > VELOCITY_LAND_LIGHT ) {
				playerDriver->AI_HARDLANDING = true;
				StartSound( "snd_fall_light", SND_VEHICLE_MISC, 0, NULL );
			} else if ( length > VELOCITY_LAND_SOFT ) {
				playerDriver->AI_SOFTLANDING = true;
				StartSound( "snd_fall_soft", SND_VEHICLE_MISC, 0, NULL );
			}
		}

		if ( fallDamage != NULL && damageScale > 0.0f ) {
			Damage( NULL, NULL, idVec3( 0.0f, 0.0f, -1.0f ), fallDamage, damageScale, 0 );
			if ( driver != NULL ) {
				driver->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, -1.0f ), fallDamage, damageScale, 0 );
			}
		}
	}

	return sdTransport::Collide( collision, velocity, bodyId );
}

/*
================
sdJetPack::ApplyNetworkState
================
*/
void sdJetPack::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {		
		NET_GET_NEW( sdJetPackNetworkData );

		currentJumpCharge = newData.currentJumpCharge;
	}

	sdTransport::ApplyNetworkState( mode, newState );
}

/*
================
sdJetPack::ReadNetworkState
================
*/
void sdJetPack::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {		
		NET_GET_STATES( sdJetPackNetworkData );

		newData.currentJumpCharge = msg.ReadDeltaFloat( baseData.currentJumpCharge );
	}

	sdTransport::ReadNetworkState( mode, baseState, newState, msg );
}

/*
================
sdJetPack::WriteNetworkState
================
*/
void sdJetPack::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {		
		NET_GET_STATES( sdJetPackNetworkData );

		newData.currentJumpCharge = currentJumpCharge;

		msg.WriteDeltaFloat( baseData.currentJumpCharge, newData.currentJumpCharge );
	}

	sdTransport::WriteNetworkState( mode, baseState, newState, msg );
}

/*
================
sdJetPack::CheckNetworkStateChanges
================
*/
bool sdJetPack::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdJetPackNetworkData );
		NET_CHECK_FIELD( currentJumpCharge, currentJumpCharge );
	}

	return sdTransport::CheckNetworkStateChanges( mode, baseState );
}

/*
================
sdJetPack::CreateTransportNetworkStructure
================
*/
sdTransportNetworkData* sdJetPack::CreateTransportNetworkStructure( void ) const {
	return new sdJetPackNetworkData();
}

/*
===============================================================================

  sdJetPackVisuals

===============================================================================
*/

/*
================
sdJetPackVisuals::UpdateRenderEntity
================
*/
bool sdJetPackVisuals::UpdateRenderEntity( renderEntity_t* renderEntity, const renderView_t* renderView, int& lastGameModifiedTime ) {
	if ( animator.CreateFrame( gameLocal.time, false ) || lastGameModifiedTime != animator.GetTransformCount() ) {
		lastGameModifiedTime = animator.GetTransformCount();
		return true;
	}

	return false;
}


/*
===============================================================================

  sdJetPackNetworkData

===============================================================================
*/

/*
================
sdJetPackNetworkData::MakeDefault
================
*/
void sdJetPackNetworkData::MakeDefault( void ) {
	sdTransportNetworkData::MakeDefault();

	currentJumpCharge = 0.0f;
}

/*
================
sdJetPackNetworkData::Write
================
*/
void sdJetPackNetworkData::Write( idFile* file ) const {
	sdTransportNetworkData::Write( file );

	file->WriteFloat( currentJumpCharge );
}

/*
================
sdJetPackNetworkData::Read
================
*/
void sdJetPackNetworkData::Read( idFile* file ) {
	sdTransportNetworkData::Read( file );

	file->ReadFloat( currentJumpCharge );
}

