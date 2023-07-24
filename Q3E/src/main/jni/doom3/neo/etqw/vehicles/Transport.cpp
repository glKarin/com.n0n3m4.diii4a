// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Transport.h"
#include "TransportComponents.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "VehicleWeapon.h"
#include "VehicleView.h"
#include "VehicleControl.h"
#include "SoundControl.h"
#include "../ContentMask.h"
#include "../Projectile.h"
#include "../Player.h"
#include "../PlayerView.h"
#include "../../decllib/DeclSurfaceType.h"
#include "../structures/DeployMask.h"
#include "../rules/VoteManager.h"

#include "../../sys/sys_local.h"

#include "../guis/UserInterfaceLocal.h"
#include "../rules/VoteManager.h"
#include "../rules/GameRules.h"
#include "../client/ClientMoveable.h"
#include "../PredictionErrorDecay.h"

#include "../botai/BotThreadData.h"

const int	MAX_ENGINE_SOUNDS	= SND_ENGINE_LAST - SND_ENGINE + 1;
const int	MAX_ZOOM_LEVELS		= 4;
const float	MIN_DAMAGE_SPEED	= 10.f;

idCVar g_noVehicleDecay(			"g_noVehicleDecay",				"0", CVAR_BOOL | CVAR_GAME, "enables / disables vehicle decay" );
idCVar g_showVehicleCockpits(		"g_showVehicleCockpits",		"1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "enables / disables vehicle cockpits" );
idCVar g_noVehicleSpawnInvulnerability(	"g_noVehicleSpawnInvulnerability",	"0", CVAR_BOOL | CVAR_GAME | CVAR_NETWORKSYNC | CVAR_RANKLOCKED, "enables / disables vehicle invulnerability on spawn" );

/*
===============================================================================

	sdTransportUsableInterface

===============================================================================
*/

void sdTransportUsableInterface::Init( sdTransport* transport ) {
	owner				= transport;
	onActivateFunc		= transport->GetScriptObject()->GetFunction( "OnActivate" );
	onActivateHeldFunc	= transport->GetScriptObject()->GetFunction( "OnActivateHeld" );
	maxEnterDistance	= transport->spawnArgs.GetFloat( "max_enter_distance", "100" );
}

/*
================
sdTransportUsableInterface::ForcePlacement
================
*/
void sdTransportUsableInterface::ForcePlacement( idPlayer* other, int index, int oldIndex, bool keepCamera ) {
	owner->ForcePlacement( other, index, oldIndex, keepCamera );
}

/*
================
sdTransportUsableInterface::FindPositionForPlayer
================
*/
void sdTransportUsableInterface::FindPositionForPlayer( idPlayer* player ) {
	owner->FindPositionForPlayer( player );
}

/*
================
sdTransportUsableInterface::GetOverlayHandle
================
*/
const sdDeclGUI* sdTransportUsableInterface::GetOverlayGUI( void ) const {
	return owner->GetOverlayGUI();
}

/*
================
sdTransportUsableInterface::HasAbility
================
*/
bool sdTransportUsableInterface::HasAbility( qhandle_t handle, const idPlayer* player ) const {
	return owner->GetPositionManager().PositionForPlayer( player ).HasAbility( handle );
}

/*
================
sdTransportUsableInterface::GetHideHud
================
*/
bool sdTransportUsableInterface::GetHideHud( idPlayer* player ) const {
	sdVehicleView& view = owner->GetViewForPlayer( player );
	return view.GetViewParms().hideHud;
}

/*
================
sdTransportUsableInterface::GetSensitivity
================
*/
bool sdTransportUsableInterface::GetSensitivity( idPlayer* player, float& scaleX, float& scaleY ) const {
	if ( gameLocal.usercmds[ player->entityNumber ].buttons.btn.tophat ) {
		return false;
	}
	return owner->GetViewForPlayer( player ).GetSensitivity( scaleX, scaleY );
}

/*
================
sdTransportUsableInterface::GetDamageScale
================
*/
float sdTransportUsableInterface::GetDamageScale( idPlayer* player ) const {
	return owner->GetPositionManager().PositionForPlayer( player ).GetDamageScale();
}

/*
================
sdTransportUsableInterface::GetWeaponName
================
*/
const sdDeclLocStr* sdTransportUsableInterface::GetWeaponName( const idPlayer* player ) const {
	sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );

	int index = position.GetWeaponIndex();
	if( index != -1 ) {
		sdVehicleWeapon* vWeapon = owner->GetWeapon( index );
		return vWeapon->GetWeaponName();
	}
	return NULL;
}

/*
================
sdTransportUsableInterface::GetWeaponLookupName
================
*/
const char* sdTransportUsableInterface::GetWeaponLookupName( const idPlayer* player ) const {
	sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );

	int index = position.GetWeaponIndex();
	if( index != -1 ) {
		sdVehicleWeapon* vWeapon = owner->GetWeapon( index );
		return vWeapon->GetName();
	}
	return "";
}

/*
================
sdTransportUsableInterface::GetShowPlayer
================
*/
bool sdTransportUsableInterface::GetShowPlayer( idPlayer* player ) const {
	idPlayer* localViewPlayer = gameLocal.GetLocalViewPlayer();
	if ( localViewPlayer != NULL && localViewPlayer->GetProxyEntity() == owner ) {
		const sdVehicleView& localParms = owner->GetPositionManager().PositionForPlayer( localViewPlayer ).GetViewParms();

		if ( !g_showVehicleCockpits.GetBool() ) {
			if ( !localParms.GetViewParms().thirdPerson ) {
				return false;
			}
		}

		if ( !localParms.GetViewParms().showOtherPassengers ) {
			return false;
		}
	}
	return owner->GetPositionManager().PositionForPlayer( player ).GetShowPlayer();
}

/*
================
sdTransportUsableInterface::GetShowPlayerShadow
================
*/
bool sdTransportUsableInterface::GetShowPlayerShadow( idPlayer* player ) const {
	sdVehicleView& vehicleView = owner->GetViewForPlayer( player );
	return vehicleView.HasPlayerShadow();
}

/*
================
sdTransportUsableInterface::GetShowCrosshair
================
*/
bool sdTransportUsableInterface::GetShowCrosshair( idPlayer* player ) const {
	sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	const positionViewMode_t& view = position.GetViewParms().GetViewParms();
	if( view.thirdPerson && !view.showCrosshairInThirdPerson ) {
		return false;
	}

	int weaponIndex = position.GetWeaponIndex();
	if ( weaponIndex != -1 ) {
		sdVehicleWeapon* weapon = owner->GetWeapon( weaponIndex );
		bool topHat = gameLocal.usercmds[ player->entityNumber ].buttons.btn.tophat;
		assert( weapon != NULL );
		if ( weapon->GetNoTophatCrosshair() ) {
			return !topHat;
		}

		if ( topHat ) {
			idVec3 aimPosition = player->firstPersonViewOrigin + player->firstPersonViewAxis[ 0 ] * 2048.0f;
			return weapon->CanAimAt( aimPosition );
		}
	}

	return true;
}

/*
================
sdTransportUsableInterface::GetHideDecoyInfo
================
*/
bool sdTransportUsableInterface::GetHideDecoyInfo( idPlayer* player ) const {
	sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	const positionViewMode_t& view = position.GetViewParms().GetViewParms();
	return view.hideDecoyInfo;
}

/*
================
sdTransportUsableInterface::GetShowTargetingInfo
================
*/
bool sdTransportUsableInterface::GetShowTargetingInfo( idPlayer* player ) const {
	sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	const positionViewMode_t& view = position.GetViewParms().GetViewParms();
	return view.showTargetingInfo;
}

/*
================
sdTransportUsableInterface::GetPlayerStance
================
*/
playerStance_t sdTransportUsableInterface::GetPlayerStance( idPlayer* player ) const {
	sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	return position.GetPlayerStance();
}

/*
================
sdTransportUsableInterface::GetPlayerIconJoint
================
*/
jointHandle_t sdTransportUsableInterface::GetPlayerIconJoint( idPlayer* player ) const {
	sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	return position.GetIconJoint();
}

/*
================
sdTransportUsableInterface::UpdateHud
================
*/
void sdTransportUsableInterface::UpdateHud( idPlayer* player, guiHandle_t handle ) {
	owner->UpdateHud( player, handle );
}

/*
================
sdTransportUsableInterface::GetWeaponLockInfo
================
*/
const sdWeaponLockInfo* sdTransportUsableInterface::GetWeaponLockInfo( idPlayer* player ) const {
	sdVehicleWeapon* weapon = owner->GetActiveWeapon( player );
	if ( weapon != NULL ) {
		return weapon->GetLockInfo();
	}
	return NULL;
}

/*
================
sdTransportUsableInterface::NextWeapon
================
*/
void sdTransportUsableInterface::NextWeapon( idPlayer* player ) const {
	owner->NextWeapon( player );
}

/*
================
sdTransportUsableInterface::PrevWeapon
================
*/
void sdTransportUsableInterface::PrevWeapon( idPlayer* player ) const {
	owner->PrevWeapon( player );
}

/*
================
sdTransportUsableInterface::UpdateViewAngles
================
*/
void sdTransportUsableInterface::UpdateViewAngles( idPlayer* player ) {
	owner->UpdateViewAngles( player );
}

/*
================
sdTransportUsableInterface::ClampViewAngles
================
*/
void sdTransportUsableInterface::ClampViewAngles( idPlayer* player, const idAngles& oldViewAngles ) const {
	owner->ClampViewAngles( player, oldViewAngles );
}

/*
================
sdTransportUsableInterface::OnUsed
================
*/
bool sdTransportUsableInterface::OnUsed( idPlayer* player, float distance ) {
	if ( player->GetHealth() <= 0 ) {
		return false;
	}

	idEntity* proxy = player->GetProxyEntity();
	if ( proxy ) {
		assert( proxy == owner );

		if ( !gameLocal.isClient && !owner->IsTeleporting() ) {
			if ( !OnExit( player, false ) ) {
				// send the tooltip
				const char* tooltipName = owner->spawnArgs.GetString( "tt_cannot_exit" );
				const sdDeclToolTip* decl = gameLocal.declToolTipType[ tooltipName ];
				if ( decl == NULL ) {
					gameLocal.Warning( "sdTransportUsableInterface::OnUsed Invalid Tooltip" );
				} else {
					player->SendToolTip( decl );
				}
			}
		}

		return true;
	}

	if ( distance > maxEnterDistance ) {
		return false;
	}

	const sdDeclToolTip* tip = NULL;
	if ( !owner->CanUse( player, &tip ) ) {
		if ( tip ) {
			player->SpawnToolTip( tip );
		}
		return false;
	}

	if ( !gameLocal.isClient ) {
		owner->OnUsed( player );
	}

	return true;
}

/*
================
sdTransportUsableInterface::OnActivate
================
*/
bool sdTransportUsableInterface::OnActivate( idPlayer* player, float distance ) {
	if ( !onActivateFunc ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );	
	owner->GetScriptObject()->CallNonBlockingScriptEvent( onActivateFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
================
sdTransportUsableInterface::OnActivateHeld
================
*/
bool sdTransportUsableInterface::OnActivateHeld( idPlayer* player, float distance ) {
	if ( !onActivateHeldFunc ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );	
	owner->GetScriptObject()->CallNonBlockingScriptEvent( onActivateHeldFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
================
sdTransportUsableInterface::OnExit
================
*/
bool sdTransportUsableInterface::OnExit( idPlayer* player, bool force ) {
	if ( !owner->Exit( player, force ) ) {
		return false;
	}

	player->UpdateShadows();
	return true;
}

/*
================
sdTransportUsableInterface::SwapPosition
================
*/
void sdTransportUsableInterface::SwapPosition( idPlayer* player ) {
	owner->GetPositionManager().SwapPosition( player, true );
	player->UpdateShadows();
}

/*
================
sdTransportUsableInterface::SwapViewMode
================
*/
void sdTransportUsableInterface::SwapViewMode( idPlayer* player ) {
	owner->GetPositionManager().PositionForPlayer( player ).CycleCamera();
	player->UpdateShadows();
}

/*
================
sdTransportUsableInterface::SelectWeapon
================
*/
void sdTransportUsableInterface::SelectWeapon( idPlayer* player, int index ) {
	owner->SelectWeapon( player, index );
}

/*
================
sdTransportUsableInterface::BecomeActiveViewProxy
================
*/
void sdTransportUsableInterface::BecomeActiveViewProxy( idPlayer* viewPlayer ) {
	owner->SetupCockpit( viewPlayer );
}

/*
================
sdTransportUsableInterface::StopActiveViewProxy
================
*/
void sdTransportUsableInterface::StopActiveViewProxy( void ) {
	gameLocal.playerView.ClearCockpit();
	owner->SetInteriorSound( NULL );
}

/*
================
sdTransportUsableInterface::UpdateProxyView
================
*/
void sdTransportUsableInterface::UpdateProxyView( idPlayer* viewPlayer ) {
	sdVehicleView& vehicleView = owner->GetViewForPlayer( viewPlayer );

	if ( vehicleView.IsInterior() ) {
		owner->SetInteriorSound( viewPlayer );
	} else {
		owner->SetInteriorSound( NULL );
	}

	if ( !vehicleView.ShowCockpit() || !g_showVehicleCockpits.GetBool() ) {
		gameLocal.playerView.ClearCockpit();
	} else {
		if ( !gameLocal.playerView.CockpitIsValid() ) {
			owner->SetupCockpit( viewPlayer );
		}
		if ( gameLocal.playerView.CockpitIsValid() ) {
			rvClientEntityPtr< sdClientAnimated > &cockpit = gameLocal.playerView.GetCockpit();
			renderEntity_t *re = cockpit.GetEntity()->GetRenderEntity();
			if ( re ) {
				re->flags.noShadow = !vehicleView.IsCockpitShadowed();
			}
		}
	}
}

/*
================
sdTransportUsableInterface::GetFov
================
*/
float sdTransportUsableInterface::GetFov( idPlayer* player ) const {
	return owner->GetViewForPlayer( player ).GetFov();
}

/*
================
sdTransportUsableInterface::UpdateViewPos
================
*/
void sdTransportUsableInterface::UpdateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) {
	owner->GetViewForPlayer( player ).CalculateViewPos( player, origin, axis, fullUpdate );
}

/*
================
sdTransportUsableInterface::GetRequiredViewAngles
================
*/
const idAngles sdTransportUsableInterface::GetRequiredViewAngles( idPlayer* player, idVec3& target ) {
	return owner->GetViewForPlayer( player ).GetRequiredViewAngles( target );
}

/*
================
sdTransportUsableInterface::CalculateRenderView
================
*/
void sdTransportUsableInterface::CalculateRenderView( idPlayer* player, renderView_t& renderView ) {
	sdVehicleView& vehicleView = owner->GetViewForPlayer( player );

	renderView.foliageDepthHack = vehicleView.GetViewParms().foliageDepthHack;
	if ( vehicleView.GetViewParms().thirdPerson ) {
		renderView.vieworg = vehicleView.GetThirdPersonViewOrigin();
		renderView.viewaxis = vehicleView.GetThirdPersonViewAxis();
		renderView.viewID = 0;
	} else {
		// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
		// allow the right player view weapons
		if ( !owner->ShowFirstPersonPlayer() ) {
			renderView.viewID = player->entityNumber + 1;
		} else {
			renderView.viewID = 0;
		}
		renderView.vieworg	= player->firstPersonViewOrigin;
		renderView.viewaxis = player->firstPersonViewAxis;
	}

	// field of view
	gameLocal.CalcFov( player->CalcFov(), renderView.fov_x, renderView.fov_y );
}

/*
================
sdTransportUsableInterface::GetAllowPlayerMove
================
*/
bool sdTransportUsableInterface::GetAllowPlayerMove( idPlayer* player ) const {
	return owner->GetAllowPlayerMove( player );
}

/*
================
sdTransportUsableInterface::GetAllowPlayerDamage
================
*/
bool sdTransportUsableInterface::GetAllowPlayerDamage( idPlayer* player ) const {
	const sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	return position.GetTakesDamage();
}

/*
================
sdTransportUsableInterface::GetAllowPlayerDamage
================
*/
bool sdTransportUsableInterface::GetAllowPlayerWeapon( idPlayer* player ) const {
	const sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	return position.GetAllowWeapon();
}

/*
================
sdTransportUsableInterface::GetAllowAdjustBodyAngles
================
*/
bool sdTransportUsableInterface::GetAllowAdjustBodyAngles( idPlayer* player ) const {
	const sdVehiclePosition& position = owner->GetPositionManager().PositionForPlayer( player );
	return position.GetAllowAdjustBodyAngles();
}

/*
============
sdTransportUsableInterface::GetNumPositions
============
*/
int sdTransportUsableInterface::GetNumPositions() const {
	return owner->GetPositionManager().NumPositions();
}

/*
============
sdTransportUsableInterface::GetPlayerAtPosition
============
*/
idPlayer* sdTransportUsableInterface::GetPlayerAtPosition( int i ) const {
	return owner->GetPositionManager().PositionForId( i )->GetPlayer();
}

/*
============
sdTransportUsableInterface::GetPositionTitle
============
*/
const sdDeclLocStr* sdTransportUsableInterface::GetPositionTitle( int i ) const {
	return owner->GetPositionManager().PositionForId( i )->GetHudName();
}

/*
============
sdTransportUsableInterface::GetDestructionEndTime
============
*/
int sdTransportUsableInterface::GetDestructionEndTime( void ) const {
	return owner->GetDestructionEndTime();
}

/*
============
sdTransportUsableInterface::GetDirectionWarning
============
*/
bool sdTransportUsableInterface::GetDirectionWarning( void ) const {
	return owner->GetDirectionWarning();
}

/*
============
sdTransportUsableInterface::GetRouteKickDistance
============
*/
int sdTransportUsableInterface::GetRouteKickDistance( void ) const {
	return owner->GetRouteKickDistance();
}

/*
============
sdTransportUsableInterface::GetXPSharer
============
*/
idPlayer* sdTransportUsableInterface::GetXPSharer( float& shareFactor ) {
	shareFactor = 0.5f;
	return owner->GetPositionManager().FindDriver();
}

/*
===============================================================================

	sdTransport_RB

===============================================================================
*/

ABSTRACT_DECLARATION( sdTransport, sdTransport_RB )
END_CLASS

/*
================
sdTransport_RB::sdTransport_RB
================
*/
sdTransport_RB::sdTransport_RB( void ) {
}

/*
================
sdTransport_RB::~sdTransport_RB
================
*/
sdTransport_RB::~sdTransport_RB( void ) {
	positionManager.EjectAllPlayers();	
}

/*
================
sdTransport_RB::Spawn
================
*/
void sdTransport_RB::Spawn( void ) {
	// set up the scraping
	int numSurfaceTypesAtSpawn;
	InitEffectList( lightScrapeEffects, "fx_scrape_light", numSurfaceTypesAtSpawn );
	InitEffectList( mediumScrapeEffects, "fx_scrape_medium", numSurfaceTypesAtSpawn );
	InitEffectList( heavyScrapeEffects, "fx_scrape_heavy", numSurfaceTypesAtSpawn );

	lightScrapeSpeed = spawnArgs.GetFloat( "speed_scrape_light", "0" );
	mediumScrapeSpeed = spawnArgs.GetFloat( "speed_scrape_medium", "100" );
	heavyScrapeSpeed = spawnArgs.GetFloat( "speed_scrape_heavy", "200" );
}

/*
================
sdTransport_RB::DisableClip
================
*/
void sdTransport_RB::DisableClip( bool activateContacting ) {
	physicsObj.DisableClip( activateContacting );
	DisableCombat();
}

/*
================
sdTransport_RB::EnableClip
================
*/
void sdTransport_RB::EnableClip( void ) {
	if ( fl.forceDisableClip ) {
		return;
	}

	physicsObj.EnableClip();
	EnableCombat();
}

/*
================
sdTransport_RB::LoadParts
================
*/
void sdTransport_RB::LoadParts( int partTypes ) {
	ClearDriveObjects();

	int i;

	if ( partTypes & VPT_PART ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_PART ) {
				continue;
			}

			sdVehicleRigidBodyPart* part = new sdVehicleRigidBodyPart;
			part->SetIndex( driveObjects.Num() );
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

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

	if ( partTypes & VPT_ROTOR ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_ROTOR ) {
				continue;
			}

			sdVehicleRigidBodyRotor* part = new sdVehicleRigidBodyRotor;
			part->SetIndex( driveObjects.Num() );
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_WHEEL ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_WHEEL ) {
				continue;
			}

			sdVehicleRigidBodyWheel* part = new sdVehicleRigidBodyWheel;
			part->SetIndex( driveObjects.Num() );
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_MASS ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_MASS ) {
				continue;
			}

			sdDeclVehiclePart* mass = vehicleScript->parts[ i ].part;

			sdPhysics_RigidBodyMultiple& rigidBody = *GetRBPhysics();

			int index = rigidBody.GetNumClipModels();

			idTraceModel trm( idBounds( idVec3( -4.f, -4.f, -4.f ), idVec3( 4.f, 4.f, 4.f ) ) );

			rigidBody.SetClipModel( new idClipModel( trm, false ), 1.f, index );
			rigidBody.SetContactFriction( index, vec3_zero );
			rigidBody.SetMass( mass->data.GetFloat( "mass" ), index );
			rigidBody.SetBodyOffset( index, mass->data.GetVector( "origin" ) );
			rigidBody.SetBodyBuoyancy( index, mass->data.GetFloat( "buoyancy", "0" ) );
			rigidBody.SetBodyWaterDrag( index, mass->data.GetFloat( "waterDrag", "0" ) );
			rigidBody.SetClipMask( 0, index );
			rigidBody.SetContents( 0, index );
		}
	}

	if ( partTypes & VPT_THRUSTER ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_THRUSTER ) {
				continue;
			}

			sdVehicleThruster* part = new sdVehicleThruster;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_AIRBRAKE ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_AIRBRAKE ) {
				continue;
			}

			sdVehicleAirBrake* part = new sdVehicleAirBrake;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_TRACK ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_TRACK ) {
				continue;
			}

			sdVehicleTrack* part = new sdVehicleTrack;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_HOVER ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_HOVER ) {
				continue;
			}

			sdVehicleRigidBodyHoverPad* part = new sdVehicleRigidBodyHoverPad;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}	

	if ( partTypes & VPT_PSEUDO_HOVER ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_PSEUDO_HOVER ) {
				continue;
			}

			sdVehicleRigidBodyPseudoHover* part = new sdVehicleRigidBodyPseudoHover;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}	

	if ( partTypes & VPT_SUSPENSION ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_SUSPENSION ) {
				continue;
			}

			sdVehicleSuspensionPoint* part = new sdVehicleSuspensionPoint;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_VTOL ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_VTOL ) {
				continue;
			}

			sdVehicleRigidBodyVtol* part = new sdVehicleRigidBodyVtol;
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
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_DRAGPLANE ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_DRAGPLANE ) {
				continue;
			}

			sdVehicleRigidBodyDragPlane* part = new sdVehicleRigidBodyDragPlane;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}	

	if ( partTypes & VPT_RUDDER ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_RUDDER ) {
				continue;
			}

			sdVehicleRigidBodyRudder* part = new sdVehicleRigidBodyRudder;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}	

	if ( partTypes & VPT_HURTZONE ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_HURTZONE ) {
				continue;
			}

			sdVehicleRigidBodyHurtZone* part = new sdVehicleRigidBodyHurtZone;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}

	if ( partTypes & VPT_ANTIROLL ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_ANTIROLL ) {
				continue;
			}

			sdVehicleAntiRoll* part = new sdVehicleAntiRoll;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}
	if ( partTypes & VPT_ANTIPITCH ) {
		for( i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_ANTIPITCH ) {
				continue;
			}

			sdVehicleAntiPitch* part = new sdVehicleAntiPitch;
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}
}

/*
================
sdTransport_RB::UpdateScrapeEffects
================
*/
void sdTransport_RB::UpdateScrapeEffects( void ) {
	const rbMultipleCollision_t& scrape = physicsObj.GetLastCollision();

	int surfaceTypeIndex = -1;
	if ( scrape.trace.fraction < 1.0f ) {
		surfaceTypeIndex = 0;
	}

	if ( scrape.trace.c.surfaceType ) {
		surfaceTypeIndex = scrape.trace.c.surfaceType->Index() + 1;
	}

	if ( scrape.time < gameLocal.time - 200 ) {
		surfaceTypeIndex = -1;
	}

	float hitSpeed = scrape.velocity.Length();
	if ( hitSpeed < lightScrapeSpeed ) {
		surfaceTypeIndex = -1;
	}

	idEntity* entHit = gameLocal.entities[ scrape.trace.c.entityNum ];
	if ( entHit != NULL ) {
		int contents = entHit->GetPhysics()->GetContents();
		if ( contents & CONTENTS_BODY || contents & CONTENTS_SLIDEMOVER ) {
			surfaceTypeIndex = -1;
		}
	}

	if ( surfaceTypeIndex != -1 ) {
		sdEffect* effect = NULL;

		if ( hitSpeed < mediumScrapeSpeed ) {
			effect = &lightScrapeEffects[ surfaceTypeIndex ];
			mediumScrapeEffects[ surfaceTypeIndex ].Stop();
			heavyScrapeEffects[ surfaceTypeIndex ].Stop();
		} else if ( hitSpeed < heavyScrapeSpeed ) {
			lightScrapeEffects[ surfaceTypeIndex ].Stop();
			effect = &mediumScrapeEffects[ surfaceTypeIndex ];
			heavyScrapeEffects[ surfaceTypeIndex ].Stop();
		} else {
			lightScrapeEffects[ surfaceTypeIndex ].Stop();
			mediumScrapeEffects[ surfaceTypeIndex ].Stop();
			effect = &heavyScrapeEffects[ surfaceTypeIndex ];
		}

		renderEffect_t& renderEffect = effect->GetRenderEffect();
		renderEffect.origin = scrape.trace.c.point;
		renderEffect.axis = scrape.trace.c.normal.ToMat3();
		renderEffect.gravity = gameLocal.GetGravity();

		effect->Start( gameLocal.time );
		effect->GetNode().AddToEnd( activeEffects );
	}

	sdEffect* effect = activeEffects.Next();
	for( ; effect != NULL; effect = effect->GetNode().Next() ) {
		effect->Update();

		// stop playing any effects for surfaces we're no longer on
		if ( ( surfaceTypeIndex == -1  ) || ( effect != &lightScrapeEffects[ surfaceTypeIndex ] 
											&& effect != &mediumScrapeEffects[ surfaceTypeIndex ]
											&& effect != &heavyScrapeEffects[ surfaceTypeIndex ] ) ) {
			effect->Stop();
		}
	}
}

/*
===============================================================================

	sdTransport_AF

===============================================================================
*/

ABSTRACT_DECLARATION( sdTransport, sdTransport_AF )
END_CLASS

/*
================
sdTransport_AF::~sdTransport_AF
================
*/
sdTransport_AF::~sdTransport_AF( void ) {
	positionManager.EjectAllPlayers();
}

/*
================
sdTransport_AF::UpdateAnimationControllers
================
*/
bool sdTransport_AF::UpdateAnimationControllers( void ) {
	if( !gameLocal.isNewFrame ) {
		return false;
	}

	if ( af.IsActive() ) {
		if ( af.UpdateAnimation() ) {
			return true;
		}
	}
	return false;
}

/*
================
sdTransport_AF::sdTransport_AF
================
*/
sdTransport_AF::sdTransport_AF( void ) {
}

/*
================
sdTransport_AF::DisableClip
================
*/
void sdTransport_AF::DisableClip( bool activateContacting ) {
	af.GetPhysics()->DisableClip( activateContacting );
	DisableCombat();
}

/*
================
sdTransport_AF::EnableClip
================
*/
void sdTransport_AF::EnableClip( void ) {
	if ( fl.hidden ) {
		return;
	}

	af.GetPhysics()->EnableClip();
	EnableCombat();
}

/*
================
sdTransport_AF::LoadParts
================
*/
void sdTransport_AF::LoadParts( int partTypes ) {
	ClearDriveObjects();

	if ( partTypes & VPT_SIMPLE_PART ) {
		for( int i = 0; i < vehicleScript->parts.Num(); i++ ) {
			if ( vehicleScript->parts[ i ].type != VPT_SIMPLE_PART ) {
				continue;
			}

			sdVehiclePartSimple* part = new sdVehiclePartSimple;
			part->SetIndex( driveObjects.Num() );
			part->Init( *vehicleScript->parts[ i ].part, this );

			AddDriveObject( part );
		}
	}
}

/*
===============================================================================

	sdTransport

===============================================================================
*/

/*
================
sdTransportNetworkData::~sdTransportNetworkData
================
*/
sdTransportNetworkData::~sdTransportNetworkData( void ) {
	objectStates.DeleteContents( true );
	delete controlState;
}

/*
================
sdTransportNetworkData::MakeDefault
================
*/
void sdTransportNetworkData::MakeDefault( void ) {
	sdScriptEntityNetworkData::MakeDefault();

	for ( int i = 0; i < objectStates.Num(); i++ ) {
		assert( objectStates[ i ] != NULL );
		objectStates[ i ]->MakeDefault();
	}

	if ( controlState != NULL ) {
		controlState->MakeDefault();
	}

	routeKickDistance = -1;
}

/*
================
sdTransportNetworkData::Write
================
*/
void sdTransportNetworkData::Write( idFile* file ) const {
	sdScriptEntityNetworkData::Write( file );

	for ( int i = 0; i < objectStates.Num(); i++ ) {
		assert( objectStates[ i ] != NULL );
		objectStates[ i ]->Write( file );
	}

	if ( controlState != NULL ) {
		controlState->Write( file );
	}

	file->WriteInt( routeKickDistance );
}

/*
================
sdTransportNetworkData::Read
================
*/
void sdTransportNetworkData::Read( idFile* file ) {
	sdScriptEntityNetworkData::Read( file );

	for ( int i = 0; i < objectStates.Num(); i++ ) {
		assert( objectStates[ i ] != NULL );
		objectStates[ i ]->Read( file );
	}

	if ( controlState != NULL ) {
		controlState->Read( file );
	}

	file->ReadInt( routeKickDistance );
}

/*
================
sdTransportBroadcastData::~sdTransportBroadcastData
================
*/
sdTransportBroadcastData::~sdTransportBroadcastData( void ) {
	objectStates.DeleteContents( true );
	delete controlState;
}

/*
================
sdTransportBroadcastData::MakeDefault
================
*/
void sdTransportBroadcastData::MakeDefault( void ) {
	sdScriptEntityBroadcastData::MakeDefault();

	positionWeapons.SetNum( 0 );

	teleporting				= false;
	modelDisabled			= false;
	deathThroes				= false;
	careenStartTime			= 0;
	lastRepairedPart		= -1;
	lastDamageDir			= 0;
	empTime					= 0;
	weaponEmpTime			= 0;
	weaponDisabled			= false;

	for ( int i = 0; i < objectStates.Num(); i++ ) {
		assert( objectStates[ i ] != NULL );
		objectStates[ i ]->MakeDefault();
	}

	if ( controlState != NULL ) {
		controlState->MakeDefault();
	}
}

/*
================
sdTransportBroadcastData::Write
================
*/
void sdTransportBroadcastData::Write( idFile* file ) const {
	sdScriptEntityBroadcastData::Write( file );

	file->WriteInt( positionWeapons.Num() );
	for ( int i = 0; i < positionWeapons.Num(); i++ ) {
		file->WriteInt( positionWeapons[ i ] );
	}

	file->WriteBool( modelDisabled );
	file->WriteInt( lastRepairedPart );
	file->WriteBool( deathThroes );
	file->WriteInt( careenStartTime );
	file->WriteInt( lastDamageDir );
	file->WriteInt( empTime );
	file->WriteInt( weaponEmpTime );
	file->WriteBool( weaponDisabled );

	for ( int i = 0; i < objectStates.Num(); i++ ) {
		assert( objectStates[ i ] != NULL );
		objectStates[ i ]->Write( file );
	}

	if ( controlState != NULL ) {
		controlState->Write( file );
	}
}

/*
================
sdTransportBroadcastData::Read
================
*/
void sdTransportBroadcastData::Read( idFile* file ) {
	sdScriptEntityBroadcastData::Read( file );

	int count;
	file->ReadInt( count );

	positionWeapons.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		file->ReadInt( positionWeapons[ i ] );
	}

	file->ReadBool( modelDisabled );
	file->ReadInt( lastRepairedPart );
	file->ReadBool( deathThroes );
	file->ReadInt( careenStartTime );
	file->ReadInt( lastDamageDir );
	file->ReadInt( empTime );
	file->ReadInt( weaponEmpTime );
	file->ReadBool( weaponDisabled );

	for ( int i = 0; i < objectStates.Num(); i++ ) {
		assert( objectStates[ i ] != NULL );
		objectStates[ i ]->Read( file );
	}

	if ( controlState != NULL ) {
		controlState->Read( file );
	}
}

extern const idEventDef EV_SetArmor;
extern const idEventDef EV_GetArmor;
extern const idEventDef EV_Freeze;
extern const idEventDef EV_DisableKnockback;
extern const idEventDef EV_EnableKnockback;

const idEventDefInternal EV_EMPChanged( "internal_empchanged" );
const idEventDefInternal EV_WeaponEMPChanged( "internal_weaponempchanged" );

const idEventDef EV_Transport_Repair( "repair", 'd', DOC_TEXT( "Attempts to repair the amount of damage specified, and returns how much was actually repaired." ), 1, "If the vehicle is in a state where it will not allow repairs, the result will be -1.\nOn network clients this will return 0.", "d", "count", "Amount of damage to repair." );
const idEventDef EV_Transport_GetLastRepairOrigin( "getLastRepairOrigin", 'v', DOC_TEXT( "Returns the origin of the last component that was repaired." ), 0, "If no components have been repaired, the center of the bounds of the object will be returned." );

const idEventDef EV_Transport_Input_SetPlayer( "inputSetPlayer", '\0', DOC_TEXT( "Sets the player to be used for controlling the vehicle, when using a scripted control scheme." ), 1, NULL, "E", "player", "Player to use as the controller, or $null$ for none." );
const idEventDef EV_Transport_Input_GetCollective( "inputGetCollective", 'f', DOC_TEXT( "Returns the current value of the collective in the input controls." ), 0, NULL );
const idEventDef EV_Transport_GetLandingGearDown( "getLandingGearDown", 'b', DOC_TEXT( "Returns whether or not the landing gear is down in the vehicle control system." ), 0, "If the vehicle does not have a vehicle control system, or does not support landing gear, the result will be true." );
const idEventDef EV_Transport_GetNumOccupiedPositions( "getNumOccupiedPositions", 'd', DOC_TEXT( "Returns the number of positions in the vehicle which are occupied." ), 0, NULL );
const idEventDef EV_Transport_IsEmpty( "isEmpty", 'b', DOC_TEXT( "Returns whether there are no players in the vehicle or not." ), 0, NULL );
const idEventDef EV_Transport_UpdateEngine( "updateEngine", '\0', DOC_TEXT( "Updates the engine sounds." ), 1, NULL, "b", "disabled", "Whether the engine is disabled or not." );
const idEventDef EV_Transport_SetLightsEnabled( "setLightsEnabled", '\0', DOC_TEXT( "Turns the specified light group on or off." ), 2, "If the group index is -1, this will apply to all lights.", "d", "group", "Index of the group to change.", "b", "state", "Whether the lights should be on or off." );
const idEventDef EV_Transport_Lock( "lock", '\0', DOC_TEXT( "Locks or unlocks the vehicle." ), 1, "Players cannot enter the driver seat of a locked vehicle.", "b", "state", "Whether the vehicle should be locked or not." );
const idEventDef EV_Transport_KickPlayer( "kickPlayer", '\0', DOC_TEXT( "Kicks the specified player(s) from the vehicle." ), 2, "If the index is below 0, all players will be kicked, otherwise the player in the specified index if there is one.\nThe only supported flag is EF_KILL_PLAYERS, which will kill the player(s) as well.", "d", "index", "Index of the position of the player to kick.", "d", "flags", "Additional options." );
const idEventDef EV_Transport_DisableSuspension( "disableSuspension", '\0', DOC_TEXT( "Disables or enables wheel suspension for the vehicle." ), 1, NULL, "b", "state", "Whether to disable or enable." );
const idEventDef EV_Transport_DisableTimeout( "disableTimeout", '\0', DOC_TEXT( "Disable or enables vehicle decay." ), 1, NULL, "b", "state", "Whether to disable or enable." );
const idEventDef EV_Transport_GetSteerAngle( "getSteerAngle", 'f', DOC_TEXT( "Returns the angle at which steering should be visually displayed." ), 0, NULL );

const idEventDef EV_Transport_GetPassengerNames( "getPassengerNames", 'w', DOC_TEXT( "Returns a list of the names of the players in the vehicle, suitable for display." ), 0, NULL );

const idEventDef EV_Transport_DestroyParts( "destroyParts", '\0', DOC_TEXT( "Destroys components of the specified type." ), 1, "If the mask is 0, all components will be destroyed.\nThe only supported mask value is VPT_WHEEL.", "d", "mask", "Mask of the part types to destroy." );
const idEventDef EV_Transport_DecayParts( "decayParts", '\0', DOC_TEXT( "Decays components of the specified type." ), 1, "If the mask is 0, all components will be decayed.\nThe only supported mask value is VPT_WHEEL.", "d", "mask", "Mask of the part types to decay." );
const idEventDef EV_Transport_DecayLeftWheels( "decayLeftWheels", '\0', DOC_TEXT( "Decays any wheels on the left side of the vehicle." ), 0, NULL );
const idEventDef EV_Transport_DecayRightWheels( "decayRightWheels", '\0', DOC_TEXT( "Decays any wheels on the right side of the vehicle." ), 0, NULL );
const idEventDef EV_Transport_DecayNonWheels( "decayNonWheels", '\0', DOC_TEXT( "Decays any components which are not wheels." ), 0, NULL );
const idEventDef EV_Transport_ResetDecayTime( "resetDecayTime", '\0', DOC_TEXT( "Resets the timer for decaying the vehicle." ), 0, NULL );
const idEventDef EV_Transport_HasHiddenParts( "hasHiddenParts", 'b', DOC_TEXT( "Returns whether the vehicle has any destroyed components." ), 0, NULL );

const idEventDef EV_Transport_SwapPosition( "swapPosition", '\0', DOC_TEXT( "Cycles the position of the specified player." ), 1, "Behaviour is undefined if the player is not in this vehicle.", "e", "player", "Player to cycle positions." );

const idEventDef EV_Transport_GetObject( "getObject", 'o', DOC_TEXT( "Returns the vehicle component with the given name." ), 1, "If no component with the given name can be found, or it doesn't have a script object, the result will be $null$.", "s", "name", "Name of the component to find." );
const idEventDef EV_Transport_SetDeathThroeHealth( "setDeathThroeHealth", '\0', DOC_TEXT( "Sets the health at which the vehicle will be killed." ), 1, "This is only supported for vehicles based on the air vehicle control schemes.", "d", "health", "Health level to set." );
const idEventDef EV_Transport_GetMinDisplayHealth( "getMinDisplayHealth", 'd', DOC_TEXT( "Returns the health level at which the vehicle will be killed." ), 0, NULL );

const idEventDef EV_Transport_SelectWeapon( "selectVehicleWeapon", '\0', DOC_TEXT( "Selects the vehicle weapon of the given name, for the specified player." ), 2, "If the player specified is not in this vehicle, behaviour is undefined.", "e", "player", "Player to select a weapon for.", "s", "name", "Name of the weapon to select." );
const idEventDef EV_Transport_GetSurfaceType( "getSurfaceType", 's', DOC_TEXT( "Returns the name of a $decl:surfaceType$ on which the wheels or tracks of the vehicle are sitting." ), 0, "If the vehicle has no wheels or tracks, or they are currently sitting on a surface with no $decl:surfaceType$, the result will be an empty string." );

const idEventDef EV_Transport_IsEMPed( "isEMPed", 'b', DOC_TEXT( "Returns whether the vehicle is currently EMPed or not." ), 0, NULL );
const idEventDef EV_Transport_IsWeaponEMPed( "isWeaponEMPed", 'b', DOC_TEXT( "Returns whether the vehicle's weapons are currently EMPed or not." ), 0, NULL );
const idEventDef EV_Transport_ApplyEMPDamage( "applyEMPDamage", 'b', DOC_TEXT( "Attmempts to apply EMP damage to the vehicle and its weapons, and returns whether or not it was successful." ), 2, NULL, "f", "empTime", "Length of time to disable the vehicle for in seconds.", "f", "weaponEmpTime", "Length of time in seconds to disable the vehicle's weapons for." );
const idEventDef EV_Transport_GetRemainingEMP( "getRemainingEMP", 'f', DOC_TEXT( "Returns how long in seconds the vehicle will remain disabled for, or 0 if not disabled." ), 0, NULL );

const idEventDef EV_Transport_IsWeaponDisabled( "isWeaponDisabled", 'b', DOC_TEXT( "Returns whether the vehicle's weapons are currently disabled or not." ), 0, NULL );
const idEventDef EV_Transport_SetWeaponDisabled( "setWeaponDisabled", '\0', DOC_TEXT( "Sets whether the vehicle's weapons are currently disabled or not." ), 1, NULL, "b", "disabled", "State to set to" );

const idEventDef EV_Transport_SetLockAlarmActive( "setLockAlarmActive", '\0', DOC_TEXT( "Turns the target lock alarm on or off." ), 1, NULL, "b", "state", "Whether to turn on or off." );
const idEventDef EV_Transport_SetImmobilized( "setImmobilized", '\0', DOC_TEXT( "Sets the state of the immobilized flag on the vehicle control system, which will affect engine sounds." ), 1, NULL, "b", "state", "Whether to set or clear the flag." );
const idEventDef EV_Transport_InSiegeMode( "inSiegeMode", 'b', DOC_TEXT( "Returns whether the vehicle is in siege mode or not." ), 0, "If the vehicle does not have a control system, or the control system does not support siege mode, the result will be false.\nOnly the walker and desecrator control systems support siege mode." );

const idEventDef EV_Transport_GetNumWeapons( "getNumVehicleWeapons", 'd', DOC_TEXT( "Returns the number of weapons the vehicle has." ), 0, NULL );
const idEventDef EV_Transport_GetWeapon( "getVehicleWeapon", 'o', DOC_TEXT( "Returns the script object of the weapon at the specified index." ), 1, "If the index is out of range the result will be $null$.\nVehicle weapons are of type $class:sdVehicleWeapon$.", "d", "index", "Index of the weapon to look up." );

const idEventDef EV_Transport_SetTrackerEntity( "setTrackerEntity", '\0', DOC_TEXT( "Sets the route constraint master entity for the vehicle, or $null$ to disable route contraints." ), 1, NULL, "E", "master", "Route contraint master to set." );

const idEventDef EV_Transport_IsPlayerBanned( "isPlayerBanned", 'b', DOC_TEXT( "Returns whether the specified player is currently banned from using this vehicle." ), 1, NULL, "e", "player", "The player to check." );
const idEventDef EV_Transport_BanPlayer( "banPlayer", '\0', DOC_TEXT( "Prevents the specified player from entering this vehicle for a period of time." ), 2, NULL, "e", "player", "Player to ban.", "f", "time", "Length of time in seconds to ban for." );

const idEventDef EV_Transport_ClearLastAttacker( "clearLastAttacker", '\0', DOC_TEXT( "Clears the stored value of the last attacker, so they wont be marked as responsible for killing the vehicle." ), 0, NULL );

const idEventDef EV_Transport_EnablePart( "enablePart", '\0', DOC_TEXT( "Makes the specified part active again." ), 1, NULL, "s", "name", "Name of the part to enable." );
const idEventDef EV_Transport_DisablePart( "disablePart", '\0', DOC_TEXT( "Makes the specified part inactive." ), 1, NULL, "s", "name", "Name of the part to disable." );

const idEventDef EV_Transport_DestructionTime( "destructionTime", 'f', DOC_TEXT( "Returns the game time at which the vehicle will be destroyed for going off course, or 0 if it is still on course." ), 0, NULL );
const idEventDef EV_Transport_DirectionWarning( "directionWarning", 'b', DOC_TEXT( "Returns whether the driver is currently being given a warning about driving in the wrong direction." ), 0, NULL );

const idEventDef EV_Transport_IsTeleporting( "isTeleporting", 'b', DOC_TEXT( "Returns whether the vehicle is currently teleporting or not." ), 0, NULL );

const idEventDef EV_DisableModel( "disableModel", '\0', DOC_TEXT( "Disables or enables the visual model, and the collision models of the vehicle." ), 1, NULL, "b", "state", "Whether to disable or enable." );

ABSTRACT_DECLARATION( sdScriptEntity, sdTransport )
	EVENT( EV_Transport_Repair,					sdTransport::Event_Repair )
	EVENT( EV_Transport_GetLastRepairOrigin,	sdTransport::Event_GetLastRepairOrigin )
	EVENT( EV_Transport_Input_SetPlayer,		sdTransport::Event_Input_SetPlayer )
	EVENT( EV_Transport_Input_GetCollective,	sdTransport::Event_Input_GetCollective )
	EVENT( EV_Transport_GetLandingGearDown,		sdTransport::Event_GetLandingGearDown )

	EVENT( EV_Transport_GetNumOccupiedPositions,sdTransport::Event_GetNumOccupiedPositions )
	EVENT( EV_Transport_SwapPosition,			sdTransport::Event_SwapPosition )

	EVENT( EV_Transport_IsEmpty,				sdTransport::Event_IsEmpty )
	EVENT( EV_Transport_UpdateEngine,			sdTransport::Event_UpdateEngine )
	EVENT( EV_Transport_SetLightsEnabled,		sdTransport::Event_SetLightsEnabled )
	EVENT( EV_Transport_DisableSuspension,		sdTransport::Event_DisableWheelSuspension )

	EVENT( EV_EnableKnockback,					sdTransport::Event_EnableKnockback )
	EVENT( EV_DisableKnockback,					sdTransport::Event_DisableKnockback )

	EVENT( EV_Transport_GetPassengerNames,		sdTransport::Event_GetPassengerNames )
	EVENT( EV_Freeze,							sdTransport::Event_Freeze )

	EVENT( EV_Transport_Lock,					sdTransport::Event_Lock )
	EVENT( EV_Transport_KickPlayer,				sdTransport::Event_KickPlayer )
	EVENT( EV_Transport_DisableTimeout,			sdTransport::Event_DisableTimeout )

	EVENT( EV_Transport_DestroyParts,			sdTransport::Event_DestroyParts )
	EVENT( EV_Transport_DecayParts,				sdTransport::Event_DecayParts )

	EVENT( EV_Transport_DecayLeftWheels,		sdTransport::Event_DecayLeftWheels )
	EVENT( EV_Transport_DecayRightWheels,		sdTransport::Event_DecayRightWheels )
	EVENT( EV_Transport_DecayNonWheels,			sdTransport::Event_DecayNonWheels )
	EVENT( EV_Transport_ResetDecayTime,			sdTransport::Event_ResetDecayTime )

	EVENT( EV_Transport_HasHiddenParts,			sdTransport::Event_HasHiddenParts )

	EVENT( EV_Transport_SelectWeapon,			sdTransport::Event_SelectWeapon )
	EVENT( EV_Transport_GetSurfaceType,			sdTransport::Event_GetSurfaceType )

	EVENT( EV_Transport_GetObject,				sdTransport::Event_GetObject )
	EVENT( EV_Transport_GetSteerAngle,			sdTransport::Event_GetSteerAngle )

	EVENT( EV_DisableModel,						sdTransport::Event_DisableModel )
	EVENT( EV_Transport_SetDeathThroeHealth,	sdTransport::Event_SetDeathThroeHealth )
	EVENT( EV_Transport_GetMinDisplayHealth,	sdTransport::Event_GetMinDisplayHealth )

	EVENT( EV_Transport_IsEMPed,				sdTransport::Event_IsEMPed )
	EVENT( EV_Transport_IsWeaponEMPed,			sdTransport::Event_IsWeaponEMPed )
	EVENT( EV_Transport_ApplyEMPDamage,			sdTransport::Event_ApplyEMPDamage )
	EVENT( EV_Transport_GetRemainingEMP,		sdTransport::Event_GetRemainingEMP )

	EVENT( EV_Transport_IsWeaponDisabled,		sdTransport::Event_IsWeaponDisabled )
	EVENT( EV_Transport_SetWeaponDisabled,		sdTransport::Event_SetWeaponDisabled )

	EVENT( EV_Transport_SetLockAlarmActive,		sdTransport::Event_SetLockAlarmActive )
	EVENT( EV_Transport_SetImmobilized,			sdTransport::Event_SetImmobilized )

	EVENT( EV_Transport_InSiegeMode,			sdTransport::Event_InSiegeMode )

	EVENT( EV_SetArmor,							sdTransport::Event_SetArmor )
	EVENT( EV_GetArmor,							sdTransport::Event_GetArmor )

	EVENT( EV_Transport_GetNumWeapons,			sdTransport::Event_GetNumWeapons )
	EVENT( EV_Transport_GetWeapon,				sdTransport::Event_GetWeapon )

	EVENT( EV_Transport_SetTrackerEntity,		sdTransport::Event_SetTrackerEntity )

	EVENT( EV_Transport_IsPlayerBanned,			sdTransport::Event_IsPlayerBanned )
	EVENT( EV_Transport_BanPlayer,				sdTransport::Event_BanPlayer )

	EVENT( EV_Transport_ClearLastAttacker,		sdTransport::Event_ClearLastAttacker )

	EVENT( EV_EMPChanged,						sdTransport::Event_EMPChanged )
	EVENT( EV_WeaponEMPChanged,					sdTransport::Event_WeaponEMPChanged )

	EVENT( EV_Transport_EnablePart,				sdTransport::Event_EnablePart )
	EVENT( EV_Transport_DisablePart,			sdTransport::Event_DisablePart )

	EVENT( EV_Transport_DestructionTime,		sdTransport::Event_DestructionTime )
	EVENT( EV_Transport_DirectionWarning,		sdTransport::Event_DirectionWarning )

	EVENT( EV_Transport_IsTeleporting,			sdTransport::Event_IsTeleporting )
END_CLASS

/*
================
sdTransport::sdTransport
================
*/
sdTransport::sdTransport( void ) {
	lastMovedTime				= 0;
	lastEnteredTime				= 0;
	lastOccupiedTime			= 0;
	lastOccupant				= NULL;
	vehicleScript				= NULL;
	health						= 0;
	maxHealth					= 0;
	armor						= 0.f;
	nextFlippedDamageTime		= 0;
	flippedTime					= 0;
	decayTime					= 20;
	decayDistance				= 400;
	vehicleFlags.modelDisabled	= false;
	vehicleFlags.timeoutEnabled	= true;
	vehicleFlags.locked			= false;
	vehicleFlags.disableKnockback = false;
	vehicleFlags.lockAlarmActive = false;
	vehicleFlags.disablePartStateUpdate = false;
	vehicleFlags.weaponDisabled = false;
	killPlayerDamage			= NULL;
	flippedDamage				= NULL;
	attitudeMaterial			= NULL;
	postThinkEntNode.SetOwner( this );

	damageXPScale				= 0.f;

	lastAttacker				= NULL;
	lastDamage					= NULL;

	lastTimeInPlayZone			= -1;
	vehicleFlags.inPlayZone		= true;

	waterEffects				= NULL;

	steerVisualAngle			= 0.f;
	lastSpeed					= 0.f;
	lastRepairedPart			= -1;
	surfaceType					= NULL;

	lastDamageDir				= vec3_zero;
	vehicleFlags.lastDamageDirValid = false;
	empTime						= 0;
	weaponEmpTime				= 0;

	vehicleControl				= NULL;
	vehicleSoundControl			= NULL;

	nextTeleportTime			= 0;

	playerVehicleType			= NULL_VEHICLE;
	playerVehicleTeam			= NOTEAM;
	playerVehicleFlags			= NULL_VEHICLE_FLAGS;
	enemyLockedOnUsFunc			= NULL;
	isDeployedFunc				= NULL;
	routeActionNumber			= ACTION_NULL;

	routeKickDistance			= -1;
}

/*
==============
sdTransport::SetInteriorSound
==============
*/
void sdTransport::SetInteriorSound( idPlayer* player ) {
	int newListenerId;
	if ( player != NULL ) {
		newListenerId = player->entityNumber + 1;
	} else {
		newListenerId = entityNumber + 1;
	}
	if ( refSound.listenerId != newListenerId ) {
		refSound.listenerId = newListenerId;
		UpdateSound();
	}
}

/*
==============
sdTransport::UpdatePlayZoneInfo
==============
*/
void sdTransport::UpdatePlayZoneInfo( void ) {
	if ( !scriptEntityFlags.takesOobDamage ) {
		return;
	}

	if ( GetBindMaster() != NULL ) {
		return; // Gordon: if we are bound to something, assume we are being flown in, so we shouldn't take damage
	}
	
	const sdPlayZone* playZone = gameLocal.GetPlayZone( GetPhysics()->GetOrigin(), sdPlayZone::PZF_PLAYZONE );
	if ( playZone ) {
		const sdDeployMaskInstance* deployMask = playZone->GetMask( gameLocal.GetPlayZoneMask() );
		if ( deployMask != NULL && deployMask->IsValid( GetPhysics()->GetAbsBounds() ) == DR_CLEAR ) {
			lastTimeInPlayZone	= gameLocal.time;
			vehicleFlags.inPlayZone = true;
			return;
		}
	}

	// Gordon: If we're in a map with no playzone, we don't wanna keep getting damage, etc.
	if ( lastTimeInPlayZone >= 0 && gameLocal.time - lastTimeInPlayZone > oobDamageInterval ) {
		vehicleFlags.inPlayZone = false;
		lastTimeInPlayZone = gameLocal.time;
		
		idPlayer* player = positionManager.FindDriver();
		if ( player ) {
			player->SpawnToolTip( player->GetTeam()->GetOOBToolTip() );
		}

		if ( gameLocal.isClient ) {
			return;
		}

		float dist = 0.f;
		gameLocal.ClosestPlayZone( GetPhysics()->GetOrigin(), dist, sdPlayZone::PZF_PLAYZONE );

		const float step = 512.f;
		idStr damageDeclName;

		if ( dist <= ( step * 1 ) ) {
			damageDeclName = "damage_oob_warning";
		} else if ( dist <= ( step * 2 ) ) { 
			damageDeclName = "damage_oob_1st";
		} else if ( dist <= ( step * 3 ) ) { 
			damageDeclName = "damage_oob_2nd";
		} else if ( dist <= ( step * 4 ) ) {
			damageDeclName = "damage_oob_3rd";
		} else {
			damageDeclName = "damage_oob_4th";
		}

		const sdDeclDamage* damageDecl = gameLocal.declDamageType[ damageDeclName.c_str() ];	
		if ( damageDecl == NULL ) {
			gameLocal.Warning( "sdTransport::UpdatePlayZoneInfo: couldn't find damage decl %s", damageDeclName.c_str() );
			return;
		}
		
		Damage( NULL, NULL, idVec3( 0.f, 0.f, 1.f ), damageDecl, 1.f, NULL );	
	}
}

/*
================
sdTransport::Event_GetPassengerNames
================
*/
void sdTransport::Event_GetPassengerNames( void ) {
	sdProgram::ReturnWString( GetPassengerNames() );
}

/*
================
sdTransport::Event_Freeze
================
*/
void sdTransport::Event_Freeze( bool freeze ) {
	FreezePhysics( freeze );
	scriptEntityFlags.frozen = freeze;
}

/*
================
sdTransport::Event_Lock
================
*/
void sdTransport::Event_Lock( bool _locked ) {
	vehicleFlags.locked = _locked;
}

/*
================
sdTransport::Event_KickPlayer
================
*/
void sdTransport::Event_KickPlayer( int index, int flags ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( index < 0 ) {
		positionManager.EjectAllPlayers( flags );
	} else {
		if ( index < positionManager.NumPositions() ) {
			positionManager.EjectPlayer( *positionManager.PositionForId( index ), true );
		}
	}
}

/*
================
sdTransport::~sdTransport
================
*/
sdTransport::~sdTransport( void ) {
	DeconstructScriptObject();

	positionManager.EjectAllPlayers();
	ClearDriveObjects();
	engine.Clear();
	weapons.DeleteContents( true );
	delete waterEffects;
	
	delete vehicleControl;
	vehicleControl = NULL;

	delete vehicleSoundControl;
	vehicleSoundControl = NULL;

	waterEffects = NULL;

	FreeSelectionCombatModel();
}

/*
================
sdTransport::SetDefaultMode
================
*/
void sdTransport::SetDefaultMode( sdVehiclePosition& position, idPlayer* other ) {
	int oldCameraMode = 0;
	if ( other->userInfo.rememberCameraMode ) {
		oldCameraMode = vehicleScript->GetCameraMode( other->entityNumber, position.GetPositionId() );
		if ( oldCameraMode == -1 ) {
			oldCameraMode = 0;
		}
	}

	// Setup default view modes
	other->viewAngles.Zero();
	other->clientViewAngles.Zero();
	other->SetVehicleCameraMode( oldCameraMode, position.GetResetViewOnEnter(), true );
}

/*
================
sdTransport::OnUpdateVisuals
================
*/
void sdTransport::OnUpdateVisuals( void ) {
	sdScriptEntity::OnUpdateVisuals();
	lights.UpdatePresentation();
}

/*
================
sdTransport::RemoveActiveDriveObject
================
*/
void sdTransport::RemoveActiveDriveObject( sdVehicleDriveObject* object ) {
	activeObjects.Remove( object );
	physicsObjects.Remove( object );
	SendPartState( object );
}

/*
================
sdTransport::AddActiveDriveObject
================
*/
void sdTransport::AddActiveDriveObject( sdVehicleDriveObject* object ) {
	if ( activeObjects.AddUnique( object ) == -1 ) {
		gameLocal.Error( "sdTransport::AddActiveDriveObject - activeObjects exceeds MAX_VEHICLE_PARTS" );
	}

	if ( !IsSpawning() ) {
		SendPartState( object );
	}

	if ( object->HasPhysics() ) {
		if ( physicsObjects.AddUnique( object ) == -1 ) {
			gameLocal.Error( "sdTransport::AddActiveDriveObject - physicsObjects exceeds MAX_VEHICLE_PARTS" );
		}
	}
}

/*
================
sdTransport::LoadVehicleScript
================
*/
void sdTransport::LoadVehicleScript( void ) {
	if ( !gameLocal.isClient ) {
		positionManager.EjectAllPlayers();
	}

	StopSound( SND_ANY );

	weapons.DeleteContents( true );

	ParseVehicleScript();

	DoLoadVehicleScript();

	for ( int i = 0; i < driveObjects.Num(); i++ ) {
		driveObjects[ i ]->PostInit();
	}	

	positionManager.Init( vehicleScript, this );
	SortWeapons();

	engine.Init( this, SND_ENGINE, MAX_ENGINE_SOUNDS );
	for( int i = 0; i < vehicleScript->engineSounds.Num(); i++ ) {
		engine.AddSound( vehicleScript->engineSounds[ i ]->soundInfo );
	}
	engine.Update( true );

	lights.Clear();
	lights.Init( this );
	for( int i = 0; i < vehicleScript->lights.Num(); i++ ) {
		lights.AddLight( vehicleScript->lights[ i ]->lightInfo );
	}

	if ( vehicleControl != NULL ) {
		vehicleControl->SetupComponents();
	}
}

/*
===============
sdTransport::ParseVehicleScript
===============
*/
void sdTransport::ParseVehicleScript( void ) {
	idStr vehicleScriptName = spawnArgs.GetString( "vs_vehicleScript" );
	if ( !vehicleScriptName.Length() ) {
		gameLocal.Error( "No vehicle script defined for entity '%s'", name.c_str() );
	}

	vehicleScript = gameLocal.declVehicleScriptDefType.LocalFind( vehicleScriptName, false );
	if ( !vehicleScript ) {
		gameLocal.Error( "Vehicle script '%s' for entity '%s' not found or invalid", vehicleScriptName.c_str(), name.c_str() );
	}
}

/*
===============
sdTransport::GetDriveObject
===============
*/
sdVehicleDriveObject* sdTransport::GetDriveObject( const char* objectName ) const {
	for ( int i = 0; i < driveObjects.Num(); i++ ) {
		if ( !idStr::Icmp( driveObjects[ i ]->Name(), objectName ) ) {
			return driveObjects[ i ];
		}
	}

	return NULL;
}

/*
===============
sdTransport::Event_Repair
===============
*/
void sdTransport::Event_Repair( int repairCount ) {
	sdProgram::ReturnInteger( Repair( repairCount ) );
}

/*
===============
sdTransport::Event_GetLastRepairOrigin
===============
*/
void sdTransport::Event_GetLastRepairOrigin( void ) {
	if ( lastRepairedPart != -1 ) {
		idVec3 origin;
		driveObjects[ lastRepairedPart ]->GetWorldOrigin( origin );
		sdProgram::ReturnVector( origin );
	} else {
		const idBounds& bounds = GetPhysics()->GetAbsBounds();
		sdProgram::ReturnVector( bounds.GetCenter() );
	}
}

/*
===============
sdTransport::Event_Input_SetPlayer
===============
*/
void sdTransport::Event_Input_SetPlayer( idEntity* entity ) {
	if ( entity ) {
		idPlayer* player = entity->Cast< idPlayer >();
		if ( player ) {
			input.SetPlayer( player );
		}
	}
}

/*
===============
sdTransport::Event_Input_GetCollective
===============
*/
void sdTransport::Event_Input_GetCollective() {
	sdProgram::ReturnFloat( input.GetCollective() );
}

/*
===============
sdTransport::Event_GetNumOccupiedPositions
===============
*/
void sdTransport::Event_GetNumOccupiedPositions( void ) {
	int numPos = 0;
	for ( int i = positionManager.NumPositions() - 1; i >= 0; i-- ) {
		if ( positionManager.PositionForId( i )->GetPlayer() != NULL ) {
			numPos++;
		}
	}

	sdProgram::ReturnInteger( numPos );
}

/*
===============
sdTransport::Event_GetLandingGearDown
===============
*/
void sdTransport::Event_GetLandingGearDown() {
	sdProgram::ReturnBoolean( vehicleControl ? vehicleControl->GetLandingGearDown() : true );
}

/*
===============
sdTransport::Event_SwapPosition
===============
*/
void sdTransport::Event_SwapPosition( idEntity* other ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( player == NULL ) {
		gameLocal.Warning( "sdTransport::Event_SwapPosition Invalid Player" );
		return;
	}

	positionManager.SwapPosition( player, true );
}

/*
===============
sdTransport::Event_IsEmpty
===============
*/
void sdTransport::Event_IsEmpty( void ) {
	sdProgram::ReturnBoolean( positionManager.IsEmpty() );
}

/*
===============
sdTransport::Event_DisableModel
===============
*/
void sdTransport::Event_DisableModel( bool disabled ) {
	DisableModel( disabled );
}

/*
===============
sdTransport::UpdateHud
===============
*/
void sdTransport::UpdateHud( idPlayer* player, guiHandle_t handle ) {
	if ( updateHudFunc ) {
		sdScriptHelper( helper );
		helper.Push( player->GetScriptObject() );
		helper.Push( ( int )handle );
		CallNonBlockingScriptEvent( updateHudFunc, helper );
	}
}

/*
===============
sdTransport::Event_UpdateEngine
===============
*/
void sdTransport::Event_UpdateEngine( bool disabled ) {
	UpdateEngine( disabled );
}

/*
===============
sdTransport::Event_SetLightsEnabled
===============
*/
void sdTransport::Event_SetLightsEnabled( int group, bool enabled ) {
	SetLightsEnabled( group, enabled );
}

/*
===============
sdTransport::SetLightsEnabled
===============
*/
void sdTransport::SetLightsEnabled( int group, bool enabled ) {
	lights.SetEnabled( group, enabled );
}

/*
===============
sdTransport::Event_DisableWheelSuspension
===============
*/
void sdTransport::Event_DisableWheelSuspension( bool disable ) {
	DisableWheelSuspension( disable );
}

/*
===============
sdTransport::Event_DisableKnockback
===============
*/
void sdTransport::Event_DisableKnockback( void ) {
	vehicleFlags.disableKnockback = true;
}

/*
===============
sdTransport::Event_EnableKnockback
===============
*/
void sdTransport::Event_EnableKnockback( void ) {
	vehicleFlags.disableKnockback = false;
}

/*
===============
sdTransport::Event_DisableTimeout
===============
*/
void sdTransport::Event_DisableTimeout( bool disable ) {
	vehicleFlags.timeoutEnabled = !disable;
}

/*
===============
sdTransport::Event_GetSteerAngle
===============
*/
void sdTransport::Event_GetSteerAngle( void ) {
	sdProgram::ReturnFloat( GetSteerVisualAngle() );
}

/*
===============
sdTransport::Event_DestroyParts
===============
*/
void sdTransport::Event_DestroyParts( int mask ) {
	DestroyParts( mask );
}

/*
===============
sdTransport::Event_DecayParts
===============
*/
void sdTransport::Event_DecayParts( int mask ) {
	DecayParts( mask );
}

/*
===============
sdTransport::Event_DecayLeftWheels
===============
*/
void sdTransport::Event_DecayLeftWheels( void ) {
	DecayLeftWheels();
}

/*
===============
sdTransport::Event_DecayRightWheels
===============
*/
void sdTransport::Event_DecayRightWheels( void ) {
	DecayRightWheels();
}

/*
===============
sdTransport::Event_DecayNonWheels
===============
*/
void sdTransport::Event_DecayNonWheels( void ) {
	DecayNonWheels();
}

/*
===============
sdTransport::Event_ResetDecayTime
===============
*/
void sdTransport::Event_ResetDecayTime( void ) {
	if ( lastOccupiedTime ) {
		lastOccupiedTime = gameLocal.time;
	}
}

/*
===============
sdTransport::Event_HasHiddenParts
===============
*/
void sdTransport::Event_HasHiddenParts( void ) {
	sdProgram::ReturnBoolean( activeObjects.Num() != driveObjects.Num() );
}

/*
===============
sdTransport::Event_SelectWeapon
===============
*/
void sdTransport::Event_SelectWeapon( idEntity* other, const char* name ) {
	idPlayer* player = other->Cast< idPlayer >();
	if ( !player ) {
		gameLocal.Warning( "sdTransport::Event_SelectWeapon Invalid Entity Passed" );
		return;
	}

	int weaponIndex = GetWeaponIndex( name );
	if ( weaponIndex == -1 ) {
		gameLocal.Warning( "sdTransport::Event_SelectWeapon Invalid Weapon '%s'", name );
		return;
	}

	sdVehiclePosition& position = positionManager.PositionForPlayer( player );
	if ( !position.WeaponValid( weapons[ weaponIndex ] ) ) {
		return;
	}

	position.SetWeaponIndex( weaponIndex );
}

/*
===============
sdTransport::Event_GetSurfaceType
===============
*/
void sdTransport::Event_GetSurfaceType( void ) {
	sdProgram::ReturnString( surfaceType ? surfaceType->GetName() : "" );
}

/*
===============
sdTransport::Event_GetObject
===============
*/
void sdTransport::Event_GetObject( const char* objectName ) {
	sdVehicleDriveObject* obj = GetDriveObject( objectName );
	sdProgram::ReturnObject( obj != NULL ? obj->GetScriptObject() : NULL );
}

/*
===============
sdTransport::DestroyParts
	If mask is zero it deletes all unhidden parts
	If mask is nonzero it only deletes parts that match the mask
	FIXME: The mask stuff only works on wheels atm.
===============
*/
void sdTransport::DestroyParts( int mask ) {
	vehicleFlags.disablePartStateUpdate = true;

	vehicleDriveObjectList_t	originalActiveObjects = activeObjects;
	
	if ( mask == 0 ) {
		// destroy any "destroy first" objects first
		for ( int i = 0; i < originalActiveObjects.Num(); i++ ) {
			if ( originalActiveObjects[ i ]->DestroyFirst() ) {
				originalActiveObjects[ i ]->Damage( 10000.f, this, this, idVec3( 0, 0, 1 ), NULL );
			}
		}
	}

	for ( int i = 0; i < originalActiveObjects.Num(); i++ ) {
		if ( mask && !originalActiveObjects[ i ]->Mask( mask ) ) {
			continue;
		}

		originalActiveObjects[ i ]->Damage( 10000.f, this, this, idVec3( 0, 0, 1 ), NULL );
	}

	vehicleFlags.disablePartStateUpdate = false;

	SendFullPartStates( sdReliableMessageClientInfoAll() );
}

/*
===============
sdTransport::DecayParts
	If mask is zero it deletes all unhidden parts
	If mask is nonzero it only deletes parts that match the mask
	FIXME: The mask stuff only works on wheels atm.
===============
*/
void sdTransport::DecayParts( int mask ) {
	vehicleFlags.disablePartStateUpdate = true;

	vehicleDriveObjectList_t	originalActiveObjects = activeObjects;

	for ( int i = 0; i < originalActiveObjects.Num(); i++ ) {
		if ( mask && !originalActiveObjects[ i ]->Mask( mask ) ) {
			continue;
		}

		originalActiveObjects[ i ]->Decay();
	}

	vehicleFlags.disablePartStateUpdate = false;
	SendFullPartStates( sdReliableMessageClientInfoAll() );
}

/*
===============
sdTransport::DecayLeftWheels
===============
*/
void sdTransport::DecayLeftWheels( void ) {
	// only the server can use this function - everyone else gets synched through network state
	if ( gameLocal.isClient ) {
		return;
	}

	vehicleFlags.disablePartStateUpdate = true;

	vehicleDriveObjectList_t	originalActiveObjects;
	for ( int i = 0; i < activeObjects.Num(); i++ ) {
		originalActiveObjects.Append( activeObjects[ i ] );
	}

	bool changed = false;

	for ( int i = 0; i < originalActiveObjects.Num(); i++ ) {
		sdVehicleRigidBodyWheel* wheel = originalActiveObjects[ i ]->Cast< sdVehicleRigidBodyWheel >();
		if ( wheel ) {
			if ( wheel->IsRightWheel() ) {
				continue;
			}

			changed = true;
			wheel->Decay();
		}
	}

	vehicleFlags.disablePartStateUpdate = false;
	if ( changed ) {
		SendFullPartStates( sdReliableMessageClientInfoAll() );
	}
}

/*
===============
sdTransport::DecayRightWheels
===============
*/
void sdTransport::DecayRightWheels( void ) {
	// only the server can use this function - everyone else gets synced through network state
	if ( gameLocal.isClient ) {
		return;
	}

	vehicleDriveObjectList_t	originalActiveObjects;
	for ( int i = 0; i < activeObjects.Num(); i++ ) {
		originalActiveObjects.Append( activeObjects[ i ] );
	}

	vehicleFlags.disablePartStateUpdate = true;

	bool changed = false;

	for( int i = 0; i < originalActiveObjects.Num(); i++ ) {
		sdVehicleRigidBodyWheel* wheel = originalActiveObjects[ i ]->Cast< sdVehicleRigidBodyWheel >();
		if ( wheel ) {
			if ( wheel->IsLeftWheel() ) {
				continue;
			}

			changed = true;
			wheel->Decay();
		}
	}

	vehicleFlags.disablePartStateUpdate = false;
	if ( changed ) {
		SendFullPartStates( sdReliableMessageClientInfoAll() );
	}
}

/*
===============
sdTransport::DecayOther
===============
*/
void sdTransport::DecayNonWheels( void ){
	// only the server can use this function - everyone else gets synced through network state
	if ( gameLocal.isClient ) {
		return;
	}

	vehicleDriveObjectList_t	originalActiveObjects;
	for ( int i = 0; i < activeObjects.Num(); i++ ) {
		originalActiveObjects.Append( activeObjects[ i ] );
	}

	vehicleFlags.disablePartStateUpdate = true;

	bool changed = false;

	for( int i = 0; i < originalActiveObjects.Num(); i++ ) {
		if ( originalActiveObjects[ i ]->IsType( sdVehicleRigidBodyWheel::Type ) ) {
			continue;
		}

		changed = true;
		originalActiveObjects[ i ]->Decay();
	}

	vehicleFlags.disablePartStateUpdate = false;
	if ( changed ) {
		SendFullPartStates( sdReliableMessageClientInfoAll() );
	}
}

/*
===============
sdTransport::UpdateEngine
===============
*/
void sdTransport::UpdateEngine( bool disabled ) {
	engine.Update( disabled );
}

/*
===============
sdTransport::RunObjectPrePhysics
===============
*/
void sdTransport::RunObjectPrePhysics( void ) {
	const sdDeclSurfaceType* newSurfaceType = NULL;

	for( int i = 0; i < physicsObjects.Num(); i++ ) {
		physicsObjects[ i ]->UpdatePrePhysics( input );

		if ( !newSurfaceType ) {
			newSurfaceType = physicsObjects[ i ]->GetSurfaceType();
		}
	}

	if ( newSurfaceType != surfaceType ) {
		surfaceType = newSurfaceType;

		if ( surfaceTypeChangedFunction ) {
			sdScriptHelper h1;
			h1.Push( surfaceType ? surfaceType->GetName() : "" );
			scriptObject->CallNonBlockingScriptEvent( surfaceTypeChangedFunction, h1 );
		}

		if ( vehicleSoundControl != NULL ) {
			vehicleSoundControl->OnSurfaceTypeChanged( surfaceType );
		}
	}
}

/*
===============
sdTransport::RunObjectPostPhysics
===============
*/
void sdTransport::RunObjectPostPhysics( void ) {
	for( int i = 0; i < activeObjects.Num(); i++ ) {
		activeObjects[ i ]->UpdatePostPhysics( input );
	}
}

/*
===============
sdTransport::GetPassengerNames
===============
*/
const wchar_t* sdTransport::GetPassengerNames( void ) const {
	static idWStr names;
	names.Empty();

	int i;
	for( i = 0; i < positionManager.NumPositions(); i++ ) {
		const sdVehiclePosition* pos = positionManager.PositionForId( i );
		idPlayer* posplayer = pos->GetPlayer();
		if ( !posplayer ) {
			continue;
		}

		if ( !names.IsEmpty() ) {
			names += L", ";
		}

		const sdDeclLocStr* title = posplayer->GetInventory().GetClass()->GetTitle();
		if ( title != NULL ) {
			const wchar_t* text = title->GetText();
			if ( *text != L'\0' ) {
				names += text;
				names += L" ";
			}
		}

		names += va( L"%hs", posplayer->userInfo.name.c_str() );
	}

/*
	if( names.IsEmpty() ) {
		names = common->LocalizeText( "guis/game/empty" );
	}
*/

	return names.c_str();
}

/*
================
sdTransport::ClearDriveObjects
================
*/
void sdTransport::ClearDriveObjects( void ) {
	driveObjects.DeleteContents( true );
	activeObjects.Clear();
	physicsObjects.Clear();
	networkObjects.Clear();
}

/*
================
sdTransport::AddDriveObject
================
*/
void sdTransport::AddDriveObject( sdVehicleDriveObject* part ) {
	if ( driveObjects.Append( part ) == -1 ) {
		gameLocal.Error( "sdTransport::AddDriveObject - max driveObjects(%d) exceeded", MAX_VEHICLE_PARTS );
	}
	if ( part->IsNetworked() ) {
		networkObjects.Append( part );
	}
}

class sdVehicleDriverBackup_Finaliser : public sdVoteFinalizer {
public:
	sdVehicleDriverBackup_Finaliser( idPlayer* _player, sdTransport* _vehicle ) {
		player = _player;
		vehicle = _vehicle;
	}

	virtual void OnVoteCompleted( bool passed ) const {
		if ( !passed ) {
			return;
		}
		if ( !player.IsValid() || !vehicle.IsValid() ) {
			return;
		}

		if ( player->GetProxyEntity() != vehicle ) {
			return;
		}

		if ( vehicle->GetPositionManager().FindDriver() != NULL ) {
			return;
		}

		vehicle->GetPositionManager().SwapPosition( player, false );
	}

	idPlayer* GetPlayer( void ) const {
		return player;
	}

private:
	idEntityPtr< idPlayer > player;
	idEntityPtr< sdTransport > vehicle;
};

/*
================
sdTransport::CancelDriverBackupVote
================
*/
void sdTransport::CancelDriverBackupVote( idPlayer* player ) {
	if ( gameLocal.isClient ) {
		return;
	}

	sdPlayerVote* vote = sdVoteManager::GetInstance().FindVote( VI_DRIVER_BACKUP, this );
	if ( vote == NULL ) {
		return;
	}

	sdVehicleDriverBackup_Finaliser* finalizer = static_cast< sdVehicleDriverBackup_Finaliser* >( vote->GetFinalizer() );
	if ( finalizer->GetPlayer() == player ) {
		sdVoteManager::GetInstance().CancelVote( VI_DRIVER_BACKUP, this );
	}
}

/*
================
sdTransport::CancelAllDriverBackupVotes
================
*/
void sdTransport::CancelAllDriverBackupVotes() {
	if ( gameLocal.isClient ) {
		return;
	}

	for ( int i = 1; i < GetPositionManager().NumPositions(); i++ ) {
		sdVehiclePosition& pos = *GetPositionManager().PositionForId( i );

		idPlayer* other = pos.GetPlayer();

		if ( other == NULL ) {
			continue;
		}

		CancelDriverBackupVote( other );
	}
}

/*
================
sdTransport::Exit
================
*/
bool sdTransport::Exit( idPlayer* player, bool force ) {

	int index = player->GetProxyPositionId();

	sdVehiclePosition& position = positionManager.PositionForPlayer( player );
	
	idPlayer* realPlayer = position.GetPlayer();
	if ( realPlayer != player ) {
		assert( realPlayer == NULL );
		return true;
	}

	if ( !positionManager.EjectPlayer( position, force ) ) {
		// failed to eject the player
		return false;
	}

	CancelDriverBackupVote( player );
	if ( !gameLocal.isClient ) {
		if ( index == 0 ) {
			if ( vehicleFlags.tryKeepDriver ) {
				sdVehiclePosition& driverPos = *GetPositionManager().PositionForId( 0 );

				for ( int i = 1; i < GetPositionManager().NumPositions(); i++ ) {
					sdVehiclePosition& pos = *GetPositionManager().PositionForId( i );

					idPlayer* other = pos.GetPlayer();
					
					if ( !other ) {
						continue;
					}

					if ( !driverPos.CheckRequirements( other ) ) {
						continue;
					}

					// send vote
					sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
					vote->DisableFinishMessage();
					vote->MakePrivateVote( other );
					vote->Tag( VI_DRIVER_BACKUP, this );
					vote->SetText( gameLocal.declToolTipType[ "vote_vehicle_driver_backup" ] );
					vote->SetFinalizer( new sdVehicleDriverBackup_Finaliser( other, this ) );
					vote->Start();
				}
			}
		}
	}

	UpdateVisuals();
	return true;
}

const float COS_60 = 0.5f; //cos( DEG2RAD( 60 ) );

/*
================
sdTransport::IsFlipped
================
*/
bool sdTransport::IsFlipped( void ) {
	return GetPhysics()->GetAxis()[ 2 ].z < COS_60;
}

/*
================
sdTransport::CanUse
================
*/
bool sdTransport::CanUse( idPlayer* other, const sdDeclToolTip** tip ) {
	if ( gameLocal.isClient || other->GetHealth() <= 0 ) {
		return false;
	}

	sdVehiclePosition* position = positionManager.FreePosition( other, tip );
	if ( !position ) {
		return false;
	}

	if ( vehicleFlags.decaying ) {
		return false;
	}

	return true;
}

/*
================
sdTransport::OnUsed
================
*/
void sdTransport::OnUsed( idPlayer* other ) {
	const sdProgram::sdFunction* onUsed = scriptObject->GetFunction( "OnUsed" );
	if( onUsed ) {
		sdScriptHelper helper;
		helper.Push( other->GetScriptObject() );
		scriptObject->CallNonBlockingScriptEvent( onUsed, helper );
	} else {
		FindPositionForPlayer( other );
	}
}

/*
================
sdTransport::ForcePlacement
================
*/
void sdTransport::ForcePlacement( idPlayer* other, int index, int oldIndex, bool keepCamera ) {
	sdVehiclePosition* position = positionManager.PositionForId( index );
	sdVehiclePosition* oldPosition = NULL;
	if ( oldIndex > -1 ) {
		oldPosition = positionManager.PositionForId( oldIndex );
	}

	PlacePlayerInPosition( other, *position, oldPosition, keepCamera );
}

/*
================
sdTransport::FindPositionForPlayer
================
*/
void sdTransport::FindPositionForPlayer( idPlayer* player ) {
	idPlayer* botPlayer;
	sdVehiclePosition* oldPosition = NULL;
	sdVehiclePosition* botOldPosition = NULL;

	if ( player->GetProxyEntity() == this ) {
		oldPosition = &positionManager.PositionForPlayer( player );
	}

	sdVehiclePosition* position = positionManager.FreePosition( player, NULL );

	botPlayer = position->GetPlayer();

	if ( botPlayer != NULL ) { //mal: if there was a bot in the position the human wants, move then to the next position, or boot them from the vehicle.
		botOldPosition = &positionManager.PositionForPlayer( botPlayer );

		sdVehiclePosition* botPosition = positionManager.FreePosition( botPlayer, NULL );

		if ( botPosition ) {
			PlacePlayerInPosition( botPlayer, *botPosition, botOldPosition, false );
		} else {
			positionManager.EjectPlayer( *botOldPosition, true );
		}
	}

	if ( position ) {
		PlacePlayerInPosition( player, *position, oldPosition, false );
	} else {
		assert( false );
	}
}

/*
================
sdTransport::SelectWeapon
================
*/
void sdTransport::SelectWeapon( idPlayer* player, int index ) {
	if ( !weaponSelectedFunction ) {
		return;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( index );
	scriptObject->CallNonBlockingScriptEvent( weaponSelectedFunction, h1 );
}

/*
================
sdTransport::GetWeaponIndex
================
*/
int sdTransport::GetWeaponIndex( const char* name ) {
	for ( int i = 0; i < weapons.Num(); i++ ) {
		if ( !idStr::Icmp( name, weapons[ i ]->GetName() ) ) {
			return i;
		}
	}
	return -1;
}

/*
================
sdTransport::GetWeapon
================
*/
sdVehicleWeapon* sdTransport::GetWeapon( const char* name ) {
	int index = GetWeaponIndex( name );
	return index == -1 ? NULL : weapons[ index ];
}

/*
================
sdTransport::SortWeapons
================
*/
void sdTransport::SortWeapons( void ) {
	for ( int i = 0; i < positionManager.NumPositions(); i++ ) {
		sdVehiclePosition* position = positionManager.PositionForId( i );

		int index = position->GetWeaponIndex();
		if ( index < 0 || index >= weapons.Num() || !position->WeaponValid( weapons[ index ] ) ) {
			position->SetWeaponIndex( position->FindDefaultWeapon() );
		} else {
			position->SetWeaponIndex( index );
		}
	}
}

/*
================
sdTransport::PlacePlayerInPosition
================
*/
void sdTransport::PlacePlayerInPosition( idPlayer *other, sdVehiclePosition &position, sdVehiclePosition* oldPosition, bool keepCamera ) {
	if ( position.GetPositionId() == 0 ) {
		CancelAllDriverBackupVotes();
	}
	damageXPScale = spawnArgs.GetFloat( "damage_xp_scale", "1" );

	if ( oldPosition == &position ) {
		assert( false );
		return;
	}

	if ( oldPosition != NULL ) {
		if ( oldPosition->GetPlayer() == other ) {
			positionManager.RemovePlayer( *oldPosition );
		}
	}

	idPlayer* currentPlayer = position.GetPlayer();
	if ( currentPlayer ) {
		assert( false );
		if ( currentPlayer == other ) {
			return;
		}
		positionManager.EjectPlayer( position, true );
	}

	BecomeActive( TH_THINK );

	sdTeamInfo* otherTeam = other->GetGameTeam();
	if ( GetGameTeam() != otherTeam ) {		
		SetGameTeam( otherTeam );
	}

	position.SetPlayer( other );

	BindPlayerToPosition( other, position );

	bool sameVehicle = other->GetProxyEntity() == this;

	other->SetProxyEntity( this, position.GetPositionId() );
	other->ResetPredictionErrorDecay();

	const char* fsmName = position.GetAttachAnim();
	other->SetAnimState( ANIMCHANNEL_LEGS, va( "Legs_%s", fsmName ), 4 );
	other->SetAnimState( ANIMCHANNEL_TORSO, va( "Torso_%s", fsmName ), 4 );

	int oldPositionIndex = -1;
	if ( oldPosition != NULL ) {
		oldPositionIndex = oldPosition->GetPositionId();
	}
	OnPlayerEntered( other, position.GetPositionId(), oldPositionIndex );

	if ( toolTipEnter != NULL ) {
		other->SpawnToolTip( toolTipEnter );
	}

	if ( other == gameLocal.GetLocalViewPlayer() ) {
		gameLocal.playerView.UpdateProxyView( other, true );
	}

	gameLocal.localPlayerProperties.EnteredObject( other, this );

	if ( keepCamera ) {
		int currentMode = other->GetProxyViewMode();
		if ( currentMode < 0 || currentMode >= position.GetNumViewModes() ) {
			keepCamera = false;
		}
	}
	if ( !keepCamera ) {
		SetDefaultMode( position, other );
	}

	if ( !gameLocal.isClient ) {
		if ( position.GetAllowWeapon() ) {
			if ( !other->GetInventory().IsCurrentWeaponValid() ) {
				other->GetInventory().SelectBestWeapon( false );
			}
		}
	}
}

/*
================
sdTransport::OnPlayerEntered
================
*/
void sdTransport::OnPlayerEntered( idPlayer* player, int position, int oldPosition  ) {
	sdScriptHelper helper;
	helper.Push( GetScriptObject() );
	helper.Push( position );
	helper.Push( oldPosition );
	player->CallNonBlockingScriptEvent( player->GetScriptObject()->GetFunction( "OnEnterVehicle" ), helper );

	helper.Clear();
	helper.Push( player->GetScriptObject() );
	helper.Push( position );
	CallNonBlockingScriptEvent( GetScriptObject()->GetFunction( "OnPlayerEntered" ), helper );

	if ( vehicleControl != NULL ) {
		vehicleControl->OnPlayerEntered( player, position, oldPosition );
	}
	if ( vehicleSoundControl != NULL ) {
		vehicleSoundControl->OnPlayerEntered( player, position, oldPosition );
	}

	BecomeActive( TH_PHYSICS );

	player->ownsVehicle = true; //mal: this player "owns" this vehicle - bots won't touch it as long as this player is alive.
	player->lastOwnedVehicleSpawnID = gameLocal.GetSpawnId( this );
	botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.time = gameLocal.time;
	botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].wantsVehicle = false;
}

/*
================
sdTransport::OnPlayerExited
================
*/
void sdTransport::OnPlayerExited( idPlayer* player, int position ) {
	sdScriptHelper helper;
	helper.Push( GetScriptObject() );
	helper.Push( position );
	player->CallNonBlockingScriptEvent( player->GetScriptObject()->GetFunction( "OnExitVehicle" ), helper );

	helper.Clear();
	helper.Push( player->GetScriptObject() );
	helper.Push( position );
	CallNonBlockingScriptEvent( GetScriptObject()->GetFunction( "OnPlayerExited" ), helper );

	if ( vehicleControl != NULL ) {
		vehicleControl->OnPlayerExited( player, position );
	}
	if ( vehicleSoundControl != NULL ) {
		vehicleSoundControl->OnPlayerExited( player, position );
	}

	botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.time = 0;
	player->lastOwnedVehicleTime = gameLocal.time;
}

/*
================
sdTransport::BindPlayerToPosition
================
*/
void sdTransport::BindPlayerToPosition( idPlayer *other, sdVehiclePosition &position ) {
	other->SetSuppressPredictionReset( true );

	idVec3 bindPos;
	GetWorldOrigin( position.GetAttachJoint(), bindPos );
	other->SetOrigin( bindPos );
	other->Unbind();
	other->BindToJoint( this, position.GetAttachJoint(), true );
	other->GetPhysics()->UnlinkClip();
	other->ClearIKJoints();
	other->SetSuppressPredictionReset( false );
}

/*
================
sdTransport::PartForCollisionById
================
*/
sdVehicleDriveObject* sdTransport::PartForCollisionById( const trace_t& collision, int flags ) const {
	for( int i = 0; i < activeObjects.Num(); i++ ) {
		if ( ( flags & PFC_CAN_DAMAGE ) && !activeObjects[ i ]->CanDamage() ) {
			continue;
		}

		if ( flags & PFC_SELF_COLLISION ) {
			if ( collision.c.selfId < 0 || collision.c.selfId != activeObjects[ i ]->GetBodyId() ) {
				continue;
			}
		} else {
			if ( collision.c.id < 0 || collision.c.id != activeObjects[ i ]->GetBodyId() ) {
				continue;
			}
		}

		return activeObjects[ i ];
	}

	return NULL;
}

/*
================
sdTransport::PartForCollisionByPosition
================
*/
sdVehicleDriveObject* sdTransport::PartForCollisionByPosition( const trace_t& collision, int flags ) const {
	for( int i = 0; i < activeObjects.Num(); i++ ) {
		if ( ( flags & PFC_CAN_DAMAGE ) && !activeObjects[ i ]->CanDamage() ) {
			continue;
		}

		idVec3 position = collision.c.point;

		idBounds bounds;
		activeObjects[ i ]->GetBounds( bounds );

		idMat3 axes;
		idVec3 origin;
		activeObjects[ i ]->GetWorldOrigin( origin );
		activeObjects[ i ]->GetWorldAxis( axes );

		position -= origin;
		position *= axes.Transpose();

		bounds.ExpandSelf( 1.f );

/*		if ( g_debugDamage.GetInteger() ) {
			gameRenderWorld->DebugBounds( colorBlue, bounds, origin, axes, 10000 );
		}*/

		if( !bounds.ContainsPoint( position ) ) {
			continue;
		}

		return activeObjects[ i ];
	}
	return NULL;
}

/*
================
sdTransport::PartForCollision
================
*/
sdVehicleDriveObject* sdTransport::PartForCollision( const trace_t& collision, int flags ) const {

/*	if( g_debugDamage.GetInteger() ) {
		gameRenderWorld->DebugSphere( colorRed, idSphere( collision.endpos, 4 ), 10000 );
	}*/

	sdVehicleDriveObject* object;
		
	object = PartForCollisionById( collision, flags );
	if ( object ) {
		return object;
	}

	object = PartForCollisionByPosition( collision, flags );
	if ( object ) {
		return object;
	}

	return NULL;
}

/*
================
sdTransport::Repair
================
*/
int sdTransport::Repair( int repair ) {
	if ( !vehicleFlags.inPlayZone || IsFlipped() ) {
		return -1; // Gordon: -ve = cannot repair
	}

	if ( gameLocal.isClient ) {
		return 0;
	}

	int selfrepair = repair;
	int heal = maxHealth - health;
	if( selfrepair > heal ) {
		selfrepair = heal;
	}

	SetHealth( health + selfrepair );

	lastRepairedPart = -1;

	int count = 0;
	for( int i = 0; i < driveObjects.Num(); i++ ) {
		if ( count = driveObjects[ i ]->Repair( repair ) ) {
			if ( driveObjects[ i ]->IsHidden() ) {
				driveObjects[ i ]->Reattach();
			}
			lastRepairedPart = i;
			break;
		}
	}

	return selfrepair + count;
}

/*
================
sdTransport::NeedsRepair
================
*/
bool sdTransport::NeedsRepair() {
	if ( health < maxHealth ) {
		return true;
	}
	
	return activeObjects.Num() != driveObjects.Num();
}

/*
================
sdTransport::Damage
================
*/
void sdTransport::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damage, float damageScale, const trace_t* collision, bool forceKill ) {
	if ( !fl.takedamage || health < 0 ) {
		return;
	}

	if ( attacker != this ) {
		if ( !CheckTeamDamage( inflictor, damage ) && !forceKill) {
			return;
		}
	}

	idPlayer* driver = positionManager.FindDriver();
	if ( driver && driver->GetGodMode() ) {
		return;
	}

	damageScale *= 1.f - armor;

	if ( attacker )	{ //mal: lets check who attacked us, and if its a client whos not on our team, save that info for later - unless its warmup, then get ANYONE who attacks us.
		if ( attacker != this && ( attacker->entityNumber >= 0 && attacker->entityNumber < MAX_CLIENTS ) ) {
			if ( GetEntityAllegiance( inflictor ) != TA_FRIEND || gameLocal.rules->IsWarmup() ) {

				if ( driver != NULL ) {
					botThreadData.GetGameWorldState()->clientInfo[ driver->entityNumber ].lastAttacker = attacker->entityNumber; //mal: save off who attacked us last, so we can do some tactical planning
					botThreadData.GetGameWorldState()->clientInfo[ driver->entityNumber ].lastAttackerTime = gameLocal.time;
				}

				botThreadData.GetGameWorldState()->clientInfo[ attacker->entityNumber ].lastAttackClient = entityNumber; //mal: let them keep track of who they attacked last as well.
				botThreadData.GetGameWorldState()->clientInfo[ attacker->entityNumber ].lastAttackClientTime = gameLocal.time;
			}
		}
	}

	if ( collision ) {
		if ( sdVehicleDriveObject* object = PartForCollision( *collision, PFC_CAN_DAMAGE ) ) {
			bool noScale;
			float damageCount = damage->GetDamage( this, noScale );
			if ( !noScale ) {
				damageCount *= damageScale;
				if ( attacker == this ) {
					damageCount *= 0.2f;
				}
				damageCount *= object->GetDamageInfo()->damageScale;
			}

			if ( forceKill ) {
				damageCount = health;
			}
			object->Damage( static_cast< int >( damageCount ), inflictor, attacker, dir, collision );
		}
	}

	lastDamageDir = dir;
	lastDamageDir.Normalize();

	int oldHealth = health;

	sdScriptEntity::Damage( inflictor, attacker, dir, damage, damageScale, collision, forceKill );

	vehicleControl->OnPostDamage( attacker, oldHealth, health );
}

/*
================
sdTransport::Decayed
================
*/
void sdTransport::Decayed( void ) {
	if ( vehicleFlags.decaying ) {
		return;
	}
	vehicleFlags.decaying = true;

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, sdTransport::EVENT_DECAY );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}

	sdScriptHelper helper;
	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnDecayed" ), helper );
}

/*
==============
sdTransport::WriteDemoBaseData
==============
*/
void sdTransport::WriteDemoBaseData( idFile* file ) const {
	file->WriteBool( vehicleFlags.decaying );

	for ( int i = 0; i < driveObjects.Num(); i++ ) {
		file->WriteBool( driveObjects[ i ]->IsHidden() );
	}

	file->WriteBool( vehicleFlags.routeWarning );
	file->WriteInt( routeMaskWarningEndTime );
	file->WriteInt( gameLocal.GetSpawnId( routeTracker.GetTrackerEntity() ) );
}

/*
==============
sdTransport::ReadDemoBaseData
==============
*/
void sdTransport::ReadDemoBaseData( idFile* file ) {
	bool decaying;
	file->ReadBool( decaying );
	if ( decaying ) {
		Decayed();
	}

	for ( int i = 0; i < driveObjects.Num(); i++ ) {
		sdVehicleDriveObject* part = driveObjects[ i ];

		bool state;
		file->ReadBool( state );

		if ( state ) {
			part->Detach( false, vehicleFlags.decaying );
		} else {
			part->Reattach();
		}
	}

	bool temp;
	file->ReadBool( temp );
	vehicleFlags.routeWarning = temp;
	file->ReadInt( routeMaskWarningEndTime );

	int spawnId;
	file->ReadInt( spawnId );
	routeTracker.SetTrackerEntity( gameLocal.EntityForSpawnId( spawnId ) );
}

/*
================
sdTransport::UpdateVisibility
================
*/
void sdTransport::UpdateVisibility( void ) {
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
		if ( viewPlayer != NULL && viewPlayer->GetProxyEntity() == this ) {
			if ( GetViewForPlayer( viewPlayer ).GetViewParms().hideVehicle ) {
				disableInView = true;
			}

			if ( !g_showVehicleCockpits.GetBool() ) {
				if ( !GetViewForPlayer( viewPlayer ).GetViewParms().thirdPerson ) {
					disableInView = true;
				}
			}
		}
	}

	if ( hide ) {
		Hide();
	} else if ( disableInView ) {
		Show();

		int viewID = viewPlayer->entityNumber + 1;
		if ( renderEntity.suppressSurfaceInViewID != viewID || renderEntity.suppressShadowInViewID != viewID ) {
			renderEntity.suppressSurfaceInViewID = viewID;
			renderEntity.suppressShadowInViewID = viewID;

			UpdateVisuals();
			NotifyVisChanged();
		}
	} else {
		Show();

		if ( renderEntity.suppressSurfaceInViewID != 0 || renderEntity.suppressShadowInViewID != 0 ) {
			renderEntity.suppressSurfaceInViewID = 0;
			renderEntity.suppressShadowInViewID = 0;

			UpdateVisuals();
			NotifyVisChanged();
		}
	}
}

/*
================
sdTransport::CheckFlipped
================
*/
void sdTransport::CheckFlipped( void ) {
	bool isFlipped = IsFlipped() && lastSpeed <= MIN_DAMAGE_SPEED;
	bool isEmpty = positionManager.IsEmpty();
	if ( !isEmpty && !GetPhysics()->HasGroundContacts() && ( GetPhysics()->InWater() < 0.05f ) ) {
		isFlipped = false;
	}

	// update the amount of time this vehicle has been flipped for
	if ( isFlipped ) {
		flippedTime += gameLocal.msec;
	} else {
		flippedTime -= gameLocal.msec;
		if ( flippedTime < 0 ) {
			flippedTime = 0;
		}
		return;
	}

	if ( gameLocal.time < nextFlippedDamageTime ) {
		return;
	}

	// grace period of two seconds before it will start taking flip damage
	if ( !isEmpty && flippedTime < 2000 ) {
		return;
	}

	nextFlippedDamageTime = gameLocal.time + SEC2MS( 0.5 );
	Damage( NULL, NULL, idVec3( 0.f, 0.f, 1.f ), flippedDamage, 1.f, NULL );
}

/*
================
sdTransport::WantsToThink
================
*/
bool sdTransport::WantsToThink( void ) {
	if ( !gameLocal.isNewFrame ) { // Gordon: vehicles can never go to sleep during re-prediction, as they may need IK updated later
		return true;
	}

	if ( !positionManager.IsEmpty() ) {
		return true;
	}

	if ( !gameLocal.isClient && routeTracker.IsValid() ) {
		return true;
	}

	return false;
}

/*
================
sdTransport::Think
================
*/
void sdTransport::Think( void ) {
	if ( gameLocal.isNewFrame ) {
		if ( thinkFlags & TH_RUNSCRIPT ) {
			UpdateScript();
		}
	}

	if ( vehicleControl != NULL ) {
		vehicleControl->Update();
	}
	if ( gameLocal.isNewFrame && vehicleSoundControl != NULL ) {
		// vehicle sound will generally be predicted pretty good by the only player it really matters for
		// (the driver), so don't update it during reprediction
		vehicleSoundControl->Update();
	}

	bool doPhysics = !( gameLocal.isClient && IsPhysicsInhibited() ) && !IsTeleporting() && ( thinkFlags & TH_PHYSICS );
	if ( doPhysics ) {
		if ( !gameLocal.IsPaused() ) {
			RunObjectPrePhysics();
		}
	}

	idVec3 vel = GetPhysics()->GetLinearVelocity();
	idVec3 axis = GetPhysics()->GetAxis()[ 0 ];

	lastSpeed = vel * axis;

	bool ikUpdated = false;

	if ( doPhysics ) {
		idVec3 oldOrigin = GetPhysics()->GetOrigin();
		if ( RunPhysics() ) {
			if ( waterEffects != NULL ) {
				if ( GetPhysics()->InWater() < 0.001f ) {
					waterEffects->ResetWaterState();
				}
			}
			UpdateRadar();
			if ( !oldOrigin.Compare( GetPhysics()->GetOrigin(), idMath::FLT_EPSILON ) ) {
				lastMovedTime = gameLocal.time;
			}
		}
		ikUpdated = true;
	} else {
		ikUpdated = UpdateAnimationControllers();
	}

	if ( routeTracker.IsValid() && !teleportEntity.IsValid() ) {
		if ( !gameLocal.isClient ) {
			routeTracker.Update();

			SetRouteMaskWarning( routeTracker.GetMaskWarning() || IsFlipped() || !AreTracksOnGround() || GetPhysics()->InWater() > 0.3f );
			if ( routeMaskWarningEndTime != 0 ) {
				if ( gameLocal.time > routeMaskWarningEndTime ) {
					if ( !vehicleFlags.routeWarningTimeout ) {
						vehicleFlags.routeWarningTimeout = true;

						idVec3 position;
						idAngles angles;
						routeTracker.GetDropLocation( position, angles );
						
						sdScriptHelper h1;
						h1.Push( position );
						h1.Push( angles );
						scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnRouteMaskWarningTimeout" ), h1 );
					}
				}
			}

			SetRouteWarning( routeTracker.GetPositionWarning() );
			if ( routeTracker.GetKickPlayer() ) {
				sdScriptHelper h1;
				scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnRouteKickPlayer" ), h1 );
				routeTracker.Reset();
			}

			routeKickDistance = routeTracker.GetKickDistance();
		}
        
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer != NULL && localPlayer == positionManager.FindDriver() ) {
			routeTracker.Display();
		} else {
			routeTracker.Hide();
		}
	}

	if ( gameLocal.isNewFrame ) {
		if ( doPhysics ) {
			RunObjectPostPhysics();
			UpdateScrapeEffects();
		}
		
		if ( health > 0 ) {
			bool doTouch = false;
			if ( !gameLocal.isClient ) {
				doTouch = true;
			} else {
				idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
				if ( viewPlayer != NULL ) {
					if ( viewPlayer->GetProxyEntity() == this  ) {
						doTouch = true;
					}
				}
			}
			if ( doTouch && !vehicleFlags.decaying ) {
				TouchTriggers();
			}
		}

		if ( thinkFlags & TH_ANIMATE ) {
			UpdateAnimation();
		}

		UpdateShaderParms();
	}

	if ( !ikUpdated && !WantsToThink() ) {
		BecomeInactive( TH_THINK );
	}

	if ( routeActionNumber != ACTION_NULL ) {
		botThreadData.VehicleRouteThink( false, routeActionNumber, GetPhysics()->GetOrigin() );
	}
}

/*
================
sdTransport::UpdateDecay
================
*/
void sdTransport::UpdateDecay( void ) {
	if ( !positionManager.IsEmpty() || !vehicleFlags.timeoutEnabled || g_noVehicleDecay.GetBool() ) {
		lastOccupiedTime = gameLocal.time;

		// find and record any occupant
		for ( int i = 0; i < positionManager.NumPositions(); i++ ) {
			const sdVehiclePosition* position = positionManager.PositionForId(i);
			if ( position->GetPlayer() != NULL ) {
				lastOccupant = position->GetPlayer();
				break;
			}
		}

		vehicleFlags.showedDecayTooltip = false;
	} else {
		if ( lastOccupiedTime || health < maxHealth * 0.5f ) {
			if ( !lastOccupiedTime ) {
				lastOccupiedTime = gameLocal.time;
			}

			bool doDecay = true;
			if ( lastOccupant && !lastOccupant->IsSpectating() && ( lastOccupant->GetHealth() > 0 ) ) {
				// find how far away the previous occupant is
				float distance = lastOccupant->GetPhysics()->GetOrigin().Dist( GetPhysics()->GetOrigin() );
				if ( distance <= decayDistance ) {
					doDecay = false;
				}
			}

			if ( doDecay ) {
				// previous occupant is far enough away
				// spawn the abandon tooltip
				if ( toolTipAbandon != NULL ) {
					if ( lastOccupant && ( lastOccupant->GetHealth() > 0 && health > 0 ) && gameLocal.IsLocalPlayer( lastOccupant ) && !lastOccupant->IsSpectating() && !vehicleFlags.showedDecayTooltip ) {
						lastOccupant->SpawnToolTip( toolTipAbandon );
						vehicleFlags.showedDecayTooltip = true;
					}
				}

				if ( !gameLocal.isClient ) {
					// check if it has timed out
					if ( ( gameLocal.time - lastOccupiedTime ) > SEC2MS( decayTime ) ) {
						if ( fl.takedamage ) {
							Decayed();
						}
					}
				}
			} else {
				// not far enough away yet, restart the timer
				lastOccupiedTime = gameLocal.time;
				vehicleFlags.showedDecayTooltip = false;
			}
		}
	}
}

/*
================
sdTransport::UpdateWaterDamage
================
*/
void sdTransport::UpdateWaterDamage() {
	if ( lastWaterDamageTime > gameLocal.time - 1000 ) {
		return;
	}

	lastWaterDamageTime = gameLocal.time;

	if ( waterDamageDecl && GetPhysics()->InWater() > 0.5f ) {

			if( submergeTime == 0 ) {
				submergeTime = gameLocal.time + 3000;
			} else if( gameLocal.time > submergeTime ) {
				submergeTime = gameLocal.time + 3000;
				Damage( NULL, this, vec3_origin, waterDamageDecl, 1.0f, NULL );
			}

	} else {
		submergeTime = 0;
	}
}

/*
================
sdTransport::UpdateLockAlarm
================
*/
void sdTransport::UpdateLockAlarm() {
	if ( !gameLocal.DoClientSideStuff() || !vehicleFlags.lockAlarmActive || gameLocal.time < lockAlarmTime ) {
		return;
	}

	idPlayer* driver = GetPositionManager().FindDriver();
	if ( driver != gameLocal.GetLocalPlayer() ) {
		return;
	}

	int time;
	StartSound( "snd_target_lock_alarm", SND_VEHICLE_ALARM, 0, &time );
	lockAlarmTime = gameLocal.time + time;
}

/*
================
sdTransport::Spawn
================
*/
void sdTransport::Spawn( void ) {
	usableInterface.Init( this );

	fl.unlockInterpolate			= true;

	maxHealth = health				= spawnArgs.GetInt( "health" );
	lastEnteredTime					= 0;
	lastOccupiedTime				= 0;
	lastOccupant					= NULL;
	vehicleFlags.decaying			= false;
	vehicleFlags.showedDecayTooltip	= false;
	vehicleFlags.deathThroes		= false;
	careenStartTime					= 0;
	vehicleFlags.lockAlarmActive	= false;
	vehicleFlags.routeWarning		= false;
	vehicleFlags.routeWarningTimeout= false;
	vehicleFlags.cycleAllPositions	= spawnArgs.GetBool( "cycle_all_positions", "0" );

	decayTime				= spawnArgs.GetInt( "decay_time", "20" );
	decayDistance			= spawnArgs.GetInt( "decay_distance", "400" );

	overlayGUI				= gameLocal.declGUIType[ spawnArgs.GetString( "gui_vehicle" ) ];

	updateHudFunc			= scriptObject->GetFunction( "OnUpdateHud" );

	vehicleFlags.tryKeepDriver	= spawnArgs.GetBool( "try_keep_driver", "0" );
	
	toolTipEnter			= gameLocal.declToolTipType[ spawnArgs.GetString( "tt_enter" ) ];
	toolTipAbandon			= gameLocal.declToolTipType[ spawnArgs.GetString( "tt_abandon" ) ];
	
	const char* damageName;
	
	damageName			= spawnArgs.GetString( "dmg_kill_players" );	
	killPlayerDamage	= gameLocal.declDamageType[ damageName ];
	if ( !killPlayerDamage ) {
		gameLocal.Warning( "sdVehicle::Spawn Invalid Kill Player Damage Type '%s'", damageName );
	}

	damageName			= spawnArgs.GetString( "dmg_flipped" );
	flippedDamage		= gameLocal.declDamageType[ damageName ];
	if ( !flippedDamage ) {
		gameLocal.Warning( "sdVehicle::Spawn Invalid Flipped Damage Type '%s'", damageName );
	}	

	if ( !spawnArgs.GetVector( "selection_mins", "0 0 0", selectionBounds[0] ) || !spawnArgs.GetVector( "selection_maxs", "0 0 0", selectionBounds[1] ) ) {
		selectionBounds.Clear();
	}

	postThinkEntNode.AddToEnd( gameLocal.postThinkEntities );

	attitudeMaterial					= gameLocal.declMaterialType[ spawnArgs.GetString( "mtr_attitude_pitch" ) ];

	weaponSelectedFunction				= scriptObject->GetFunction( "OnWeaponSelected" );
	surfaceTypeChangedFunction			= scriptObject->GetFunction( "OnSurfaceTypeChanged" );

	int baseanim = animator.GetAnim( "base" );
	animator.PlayAnim( ANIMCHANNEL_ALL, baseanim, gameLocal.time, 0 );

	waterEffects = sdWaterEffects::SetupFromSpawnArgs( spawnArgs );

	lastRepairedPart = -1;

	OnInputInit();

	empTime = 0;
	weaponEmpTime = 0;
	submergeTime = 0;
	lastWaterDamageTime = 0;
	vehicleFlags.amphibious				= spawnArgs.GetBool( "amphibious" );
	vehicleFlags.weaponDisabled			= false;

	damageName			= spawnArgs.GetString( "dmg_water" );	
	waterDamageDecl		= gameLocal.declDamageType[ damageName ];
	if ( !waterDamageDecl ) {
		gameLocal.Warning( "sdVehicle::Spawn Couldn't find water Damage Type '%s'", damageName );
	}

	lockAlarmTime		= 0;

	// Gordon: FIXME: Make this use a factory
	// construct the vehicle control
	const char* controlName = spawnArgs.GetString( "vehicle_control", "script" );
	if ( !idStr::Cmp( controlName, "script" ) ) {
		vehicleControl = new sdVehicleScriptControl();
	} else if ( !idStr::Cmp( controlName, "desecrator" ) ) {
		vehicleControl = new sdDesecratorControl();
	} else if ( !idStr::Cmp( controlName, "wheeled" ) ) {
		vehicleControl = new sdWheeledVehicleControl();
	} else if ( !idStr::Cmp( controlName, "titan" ) ) {
		vehicleControl = new sdTitanControl();
	} else if ( !idStr::Cmp( controlName, "trojan" ) ) {
		vehicleControl = new sdTrojanControl();
	} else if ( !idStr::Cmp( controlName, "hog" ) ) {
		vehicleControl = new sdHogControl();
	} else if ( !idStr::Cmp( controlName, "husky" ) ) {
		vehicleControl = new sdHuskyControl();
	} else if ( !idStr::Cmp( controlName, "platypus" ) ) {
		vehicleControl = new sdPlatypusControl();
	} else if ( !idStr::Cmp( controlName, "mcp" ) ) {
		vehicleControl = new sdMCPControl();
	} else if ( !idStr::Cmp( controlName, "hornet" ) ) {
		vehicleControl = new sdHornetControl();
	} else if ( !idStr::Cmp( controlName, "hovercopter" ) ) {
		vehicleControl = new sdHovercopterControl();
	} else if ( !idStr::Cmp( controlName, "anansi" ) ) {
		vehicleControl = new sdAnansiControl();
	} else if ( !idStr::Cmp( controlName, "walker" ) ) {
		vehicleControl = new sdWalkerControl();
	}
	
	if ( vehicleControl == NULL ) {
		gameLocal.Error( "sdTransport::Spawn - vehicleControl should never be NULL!" );
	}

	vehicleControl->Init( this );
	vehicleControl->SetInput( &input );

	const char* soundControlName = spawnArgs.GetString( "sound_control", "" );
	if ( *soundControlName ) {
		vehicleSoundControl = sdVehicleSoundControlBase::Alloc( soundControlName );
		if ( vehicleSoundControl == NULL ) {
			gameLocal.Warning( "sdTransport::Spawn Invalid Sound Control ('%s') Setup on '%s'", soundControlName, GetName() );
		} else {
			vehicleSoundControl->Init( this );
		}
	}

	vehicleFlags.decals	= spawnArgs.GetBool( "decals", "1" );

	int temp;

	if ( spawnArgs.GetInt( "vehicle_num", "", temp ) ) {
		playerVehicleType = ( playerVehicleTypes_t ) temp; //mal: get the vehicle type, for quick and easy reference for the bots.
	} else {
		playerVehicleType = NULL_VEHICLE;
	}

	if ( spawnArgs.GetInt( "vehicle_team", "", temp ) ) {
		playerVehicleTeam = ( playerTeamTypes_t ) temp; //mal: get the vehicle team, for quick and easy reference for the bots.
	} else {
		playerVehicleTeam = NOTEAM;
	}

	if ( spawnArgs.GetInt( "vehicle_flags", "", temp ) ) {
		playerVehicleFlags = temp; //mal: get the vehicle flags, so the bots can understand what are the capabilities of this vehicle.
	} else {
		playerVehicleFlags = NULL_VEHICLE_FLAGS;
	}

	enemyLockedOnUsFunc = scriptObject->GetFunction( "EnemyHasLockedOnPlayer" );
	isDeployedFunc = scriptObject->GetFunction( "vIsDeployed" );

	if ( playerVehicleType == MCP ) {
		botThreadData.VehicleRouteThink( true, routeActionNumber, vec3_zero );
	}

	damagedCriticalDriveParts = 0;

	routeMaskWarningEndTime = 0;
	routeTracker.Init( this );

	BecomeActive( TH_PHYSICS );
}

/*
================
sdTransport::PostMapSpawn
================
*/
void sdTransport::PostMapSpawn( void ) {
	sdScriptEntity::PostMapSpawn();

	ResetPredictionErrorDecay();
}

/*
================
sdTransport::OnInputInit
================
*/
void sdTransport::OnInputInit( void ) {
	bindContext = NULL;

	const char* contextCvarName = spawnArgs.GetString( "control_context", "g_vehicleControlContext" );
	if ( idStr::Length( contextCvarName ) == 0 ) {
		return;
	}
	const char* contextName = cvarSystem->GetCVarString( contextCvarName );
	if ( idStr::Length( contextName ) == 0 ) {
		return;
	}

	bindContext = keyInputManager->AllocBindContext( contextName );
}

/*
================
sdTransport::OnInputShutdown
================
*/
void sdTransport::OnInputShutdown( void ) {
	bindContext = NULL;
}

/*
================
sdTransport::PrevWeapon
================
*/
void sdTransport::PrevWeapon( idPlayer* player ) {
	sdVehiclePosition& position = positionManager.PositionForPlayer( player );
	int index = PrevWeaponIndex( player, position.GetWeaponIndex() );
	if ( index == -1 ) {
		index = position.FindLastWeapon();
	}
	position.SetWeaponIndex( index );
}

/*
================
sdTransport::NextWeapon
================
*/
void sdTransport::NextWeapon( idPlayer* player ) {
	sdVehiclePosition& position = positionManager.PositionForPlayer( player );
	int index = NextWeaponIndex( player, position.GetWeaponIndex() );
	if ( index == -1 ) {
		index = position.FindDefaultWeapon();
	}
	position.SetWeaponIndex( index );
}

/*
================
sdTransport::GetActiveWeapon
================
*/
sdVehicleWeapon* sdTransport::GetActiveWeapon( idPlayer* player ) {
	sdVehiclePosition& position = positionManager.PositionForPlayer( player );
	int index = position.GetWeaponIndex();
	if ( index < 0 || index >= weapons.Num() ) {
		return NULL;
	}
	return weapons[ index ];
}

/*
================
sdTransport::DisableModel
================
*/
void sdTransport::DisableModel( bool hide ) {
	vehicleFlags.modelDisabled		= hide;

	if ( fl.forceDisableClip != hide ) {
		fl.forceDisableClip = hide;
		if ( fl.forceDisableClip ) {
			DisableClip();
		} else {
			EnableClip();
		}
	}
}

/*
================
sdTransport::ApplyNetworkState
================
*/
void sdTransport::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdTransportBroadcastData );

		vehicleFlags.modelDisabled	= newData.modelDisabled;
		vehicleFlags.deathThroes	= newData.deathThroes;
		careenStartTime				= newData.careenStartTime;

		lastRepairedPart			= newData.lastRepairedPart;

		lastDamageDir				= idBitMsg::BitsToDir( newData.lastDamageDir, 9 );
		vehicleFlags.lastDamageDirValid		= true;

		SetEMPTime( newData.empTime, newData.weaponEmpTime );

		vehicleFlags.weaponDisabled	= newData.weaponDisabled;

		for ( int i = 0; i < newData.positionWeapons.Num(); i++ ) {
			positionManager.PositionForId( i )->SetWeaponIndex( newData.positionWeapons[ i ] );
		}

		for ( int i = 0; i < newData.objectStates.Num(); i++ ) {
			assert( newData.objectStates[ i ] != NULL );
			networkObjects[ i ]->ApplyNetworkState( mode, *newData.objectStates[ i ] );
		}

		if ( newData.controlState != NULL && vehicleControl->IsNetworked() ) {
			vehicleControl->ApplyNetworkState( mode, *newData.controlState );
		}

		DisableModel( vehicleFlags.modelDisabled );
	}

	if ( mode == NSM_VISIBLE ) {		
		NET_GET_NEW( sdTransportNetworkData );

		for ( int i = 0; i < newData.objectStates.Num(); i++ ) {
			assert( newData.objectStates[ i ] != NULL );
			networkObjects[ i ]->ApplyNetworkState( mode, *newData.objectStates[ i ] );
		}

		if ( newData.controlState != NULL && vehicleControl->IsNetworked() ) {
			vehicleControl->ApplyNetworkState( mode, *newData.controlState );
		}

		routeKickDistance = newData.routeKickDistance;
	}

	sdScriptEntity::ApplyNetworkState( mode, newState );
}

/*
================
sdTransport::ResetNetworkState
================
*/
void sdTransport::ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdTransportBroadcastData );

		if ( vehicleControl->IsNetworked() ) {
			vehicleControl->ResetNetworkState( mode, *newData.controlState );
		}
	}
	if ( mode == NSM_VISIBLE ) {		
		NET_GET_NEW( sdTransportNetworkData );

		if ( vehicleControl->IsNetworked() ) {
			vehicleControl->ResetNetworkState( mode, *newData.controlState );
		}
	}

	sdScriptEntity::ResetNetworkState( mode, newState );
}

/*
================
sdTransport::OnApplyPain
================
*/
bool sdTransport::OnApplyPain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	return Pain( inflictor, attacker, damage, vehicleFlags.lastDamageDirValid ? lastDamageDir : dir, location, damageDecl );
}

/*
================
sdTransport::ReadNetworkState
================
*/
void sdTransport::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdTransportBroadcastData );

		// read state
		newData.teleporting				= msg.ReadBool();
		newData.modelDisabled			= msg.ReadBool();
		newData.deathThroes				= msg.ReadBool();
		newData.careenStartTime			= msg.ReadDeltaLong( baseData.careenStartTime );
		newData.lastRepairedPart		= msg.ReadDeltaLong( baseData.lastRepairedPart );
		newData.lastDamageDir			= msg.ReadDelta( baseData.lastDamageDir, 9 );
		newData.empTime					= msg.ReadDeltaLong( baseData.empTime );
		newData.weaponEmpTime			= msg.ReadDeltaLong( baseData.weaponEmpTime );
		newData.weaponDisabled			= msg.ReadBool();

		int net_weaponBits = idMath::BitsForInteger( NumWeapons() + 1 );

		newData.positionWeapons.SetNum( positionManager.NumPositions() );
		for ( int i = 0; i < positionManager.NumPositions(); i++ ) {
			if ( i < baseData.positionWeapons.Num() ) {
				newData.positionWeapons[ i ] = msg.ReadDelta( baseData.positionWeapons[ i ] + 1, net_weaponBits ) - 1;
			} else {
				newData.positionWeapons[ i ] = msg.ReadBits( net_weaponBits ) - 1;
			}
		}

		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			assert( baseData.objectStates[ i ] != NULL );
			assert( newData.objectStates[ i ] != NULL );
			networkObjects[ i ]->ReadNetworkState( mode, *baseData.objectStates[ i ], *newData.objectStates[ i ], msg );
		}

		if ( vehicleControl->IsNetworked() ) {
			vehicleControl->ReadNetworkState( mode, *baseData.controlState, *newData.controlState, msg );
		}
	}

	if ( mode == NSM_VISIBLE ) {		
		NET_GET_STATES( sdTransportNetworkData );

		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			assert( baseData.objectStates[ i ] != NULL );
			assert( newData.objectStates[ i ] != NULL );
			networkObjects[ i ]->ReadNetworkState( mode, *baseData.objectStates[ i ], *newData.objectStates[ i ], msg );
		}

		if ( vehicleControl->IsNetworked() ) {
			vehicleControl->ReadNetworkState( mode, *baseData.controlState, *newData.controlState, msg );
		}

		newData.routeKickDistance = msg.ReadDeltaLong( baseData.routeKickDistance );
	}
	
	sdScriptEntity::ReadNetworkState( mode, baseState, newState, msg );
}

/*
================
sdTransport::WriteNetworkState
================
*/
void sdTransport::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdTransportBroadcastData );

		// update state
		newData.positionWeapons.SetNum( positionManager.NumPositions() );
		for ( int i = 0; i < positionManager.NumPositions(); i++ ) {
			newData.positionWeapons[ i ] = positionManager.PositionForId( i )->GetWeaponIndex();
		}

		newData.modelDisabled			= vehicleFlags.modelDisabled;
		newData.deathThroes				= vehicleFlags.deathThroes;
		newData.careenStartTime			= careenStartTime;
		newData.lastRepairedPart		= lastRepairedPart;
		newData.lastDamageDir			= idBitMsg::DirToBits( lastDamageDir, 9 );
		newData.empTime					= empTime;
		newData.weaponEmpTime			= weaponEmpTime;
		newData.weaponDisabled			= vehicleFlags.weaponDisabled;
	
		// write state
		msg.WriteBool( newData.teleporting );
		msg.WriteBool( newData.modelDisabled );
		msg.WriteBool( newData.deathThroes );
		msg.WriteDeltaLong( baseData.careenStartTime, newData.careenStartTime );
		msg.WriteDeltaLong( baseData.lastRepairedPart, newData.lastRepairedPart );
		msg.WriteDelta( baseData.lastDamageDir, newData.lastDamageDir, 9 );
		msg.WriteDeltaLong( baseData.empTime, newData.empTime );
		msg.WriteDeltaLong( baseData.weaponEmpTime, newData.weaponEmpTime );
		msg.WriteBool( newData.weaponDisabled );

		int net_weaponBits = idMath::BitsForInteger( NumWeapons() + 1 );

		for ( int i = 0; i < positionManager.NumPositions(); i++ ) {
			if ( i < baseData.positionWeapons.Num() ) {
				msg.WriteDelta( baseData.positionWeapons[ i ] + 1, newData.positionWeapons[ i ] + 1, net_weaponBits );
			} else {
				msg.WriteBits( newData.positionWeapons[ i ] + 1, net_weaponBits );
			}
		}

		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			assert( baseData.objectStates[ i ] != NULL );
			assert( newData.objectStates[ i ] != NULL );
			networkObjects[ i ]->WriteNetworkState( mode, *baseData.objectStates[ i ], *newData.objectStates[ i ], msg );
		}

		if ( vehicleControl->IsNetworked() ) {
			vehicleControl->WriteNetworkState( mode, *baseData.controlState, *newData.controlState, msg );
		}
	}

	if ( mode == NSM_VISIBLE ) {		
		NET_GET_STATES( sdTransportNetworkData );

		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			assert( baseData.objectStates[ i ] != NULL );
			assert( newData.objectStates[ i ] != NULL );
			networkObjects[ i ]->WriteNetworkState( mode, *baseData.objectStates[ i ], *newData.objectStates[ i ], msg );
		}

		if ( vehicleControl->IsNetworked() ) {
			vehicleControl->WriteNetworkState( mode, *baseData.controlState, *newData.controlState, msg );
		}

		newData.routeKickDistance = routeKickDistance;

		msg.WriteDeltaLong( baseData.routeKickDistance, newData.routeKickDistance );
	}

	sdScriptEntity::WriteNetworkState( mode, baseState, newState, msg );
}

/*
================
sdTransport::CheckNetworkStateChanges
================
*/
bool sdTransport::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdTransportBroadcastData );

		if ( vehicleFlags.deathThroes != baseData.deathThroes ) {
			return true;
		}

		if ( vehicleFlags.modelDisabled != baseData.modelDisabled ) {
			return true;
		}

		if ( vehicleFlags.weaponDisabled != baseData.weaponDisabled ) {
			return true;
		}

		NET_CHECK_FIELD( careenStartTime, careenStartTime );
		NET_CHECK_FIELD( lastRepairedPart, lastRepairedPart );
		NET_CHECK_FIELD( lastDamageDir, idBitMsg::DirToBits( lastDamageDir, 9 ) );
		NET_CHECK_FIELD( empTime, empTime );
		NET_CHECK_FIELD( weaponEmpTime, weaponEmpTime );

		if ( baseData.positionWeapons.Num() != positionManager.NumPositions() ) {
			return true;
		}

		for ( int i = 0; i < positionManager.NumPositions(); i++ ) {
			if ( baseData.positionWeapons[ i ] != positionManager.PositionForId( i )->GetWeaponIndex() ) {
				return true;
			}
		}

		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			assert( baseData.objectStates[ i ] != NULL );
			if ( networkObjects[ i ]->CheckNetworkStateChanges( mode, *baseData.objectStates[ i ] ) ) {
				return true;
			}
		}

		if ( vehicleControl->IsNetworked() ) {
			if ( vehicleControl->CheckNetworkStateChanges( mode, *baseData.controlState ) ) {
				return true;
			}
		}
	}

	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdTransportNetworkData );

		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			assert( baseData.objectStates[ i ] != NULL );
			if ( networkObjects[ i ]->CheckNetworkStateChanges( mode, *baseData.objectStates[ i ] ) ) {
				return true;
			}
		}

		if ( vehicleControl->IsNetworked() ) {
			if ( vehicleControl->CheckNetworkStateChanges( mode, *baseData.controlState ) ) {
				return true;
			}
		}

		NET_CHECK_FIELD( routeKickDistance, routeKickDistance );
	}

	if ( sdScriptEntity::CheckNetworkStateChanges( mode, baseState ) ) {
		return true;
	}

	return false;
}

/*
================
sdTransport::CreateTransportNetworkStructure
================
*/
sdTransportNetworkData* sdTransport::CreateTransportNetworkStructure( void ) const {
	return new sdTransportNetworkData();
}

/*
================
sdTransport::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdTransport::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		sdTransportBroadcastData* newData = new sdTransportBroadcastData();
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			newData->objectStates.Append( networkObjects[ i ]->CreateNetworkStructure( mode ) );
		}
		if ( vehicleControl->IsNetworked() ) {
			newData->controlState = vehicleControl->CreateNetworkStructure( mode );
		}
		return newData;
	}

	if ( mode == NSM_VISIBLE ) {
		sdTransportNetworkData* newData = CreateTransportNetworkStructure();
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
		for ( int i = 0; i < networkObjects.Num(); i++ ) {
			newData->objectStates.Append( networkObjects[ i ]->CreateNetworkStructure( mode ) );
		}
		if ( vehicleControl->IsNetworked() ) {
			newData->controlState = vehicleControl->CreateNetworkStructure( mode );
		}
		return newData;
	}

	return sdScriptEntity::CreateNetworkStructure( mode );
}

/*
================
sdTransport::SendFullPartStates
================
*/
void sdTransport::SendFullPartStates( const sdReliableMessageClientInfoBase& target ) const {
	// send the state of all the parts
	sdEntityBroadcastEvent msg( this, sdTransport::EVENT_PARTSTATE );
	msg.WriteBits( 0, BitsForPartIndex() );
	msg.WriteBool( vehicleFlags.decaying ); // Gordon: only include this for a full state, i'll send it on its own when the vehicle actually decays

	for ( int i = 0; i < driveObjects.Num(); i++ ) {
		msg.WriteBool( driveObjects[ i ]->IsHidden() );
	}
	msg.Send( false, target );
}

/*
================
sdTransport::WriteInitialReliableMessages
================
*/
void sdTransport::WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const {
	SendFullPartStates( target );
}

/*
================
sdTransport::SendPartState
================
*/
void sdTransport::SendPartState( sdVehicleDriveObject* object ) const {
	if ( !gameLocal.isServer || vehicleFlags.disablePartStateUpdate ) {
		return;
	}

	int partNum = driveObjects.FindIndex( object );
	assert( partNum >= 0 && partNum < driveObjects.Num() );

	sdEntityBroadcastEvent msg( this, sdTransport::EVENT_PARTSTATE );
	msg.WriteBits( partNum + 1, BitsForPartIndex() );
	msg.WriteBool( driveObjects[ partNum ]->IsHidden() );
	msg.Send( false, sdReliableMessageClientInfoAll() );
}

/*
================
sdTransport::UpdateViewAngles
================
*/
void sdTransport::UpdateViewAngles( idPlayer* player ) {
	usercmd_t& cmd = gameLocal.usercmds[ player->entityNumber ];
	bool changed = cmd.buttons.btn.attack;

	bool canChange = true;
	sdVehicleView& view = GetViewForPlayer( player );
	if ( view.GetViewParms().tophatRequired && !cmd.buttons.btn.tophat ) {
		canChange = false;
	} else if ( teleportEntity.IsValid() ) {
		canChange = false;
	}

	if ( canChange ) {
		for( int i = 0; i < 3; i++ ) {
			player->viewAngles[ i ] = idMath::AngleNormalize180( player->cmdAngles[ i ] + player->GetDeltaViewAngles()[ i ] );

			if ( idMath::Fabs( idMath::AngleDelta( player->lastVehicleViewAngles[ i ], player->cmdAngles[ i ] ) ) > 3 ) {
				changed = true;
			}
		}
	}

	if ( changed ) {
		player->lastVehicleViewAnglesChange = gameLocal.time;
		player->lastVehicleViewAngles = player->cmdAngles;
	}

	if ( !cmd.buttons.btn.tophat ) {
		bool reset = false;
		float returnSpeed = 0.5f;
		if ( view.GetViewParms().tophatRequired ) {
			reset = true;
			returnSpeed = 2.0f;
		} else if ( view.AutoCenter() && ( gameLocal.time - player->lastVehicleViewAnglesChange > SEC2MS( 2.5f ) ) ) {
			reset = true;
		}

		if ( reset ) {
			for( int i = 0; i < 3; i++ ) {
				player->viewAngles[ i ] -= ( player->viewAngles[ i ] * returnSpeed ) * MS2SEC( gameLocal.msec );
			}
			player->viewAngles.FixDenormals();
		}
	}
}

/*
================
sdTransport::ClampViewAngles
================
*/
void sdTransport::ClampViewAngles( idPlayer* player, const idAngles& oldAngles ) {
	sdVehiclePosition& position = positionManager.PositionForPlayer( player );
	if ( position.GetWeaponIndex() != -1 && IsWeaponEMPed() ) {
		player->viewAngles = oldAngles;
	} else {
		sdVehicleView& view = position.GetViewParms();
		view.ClampViewAngles( player->viewAngles, oldAngles );
	}
}

/*
================
sdTransport::EvaluateContacts
================
*/
int sdTransport::EvaluateContacts( contactInfo_t* list, contactInfoExt_t* extList, int max ) {
	int count = 0;
	int i;
	for ( i = 0; i < driveObjects.Num() && ( count < max ); i++ ) {
		count += driveObjects[ i ]->EvaluateContacts( &list[ count ], &extList[ count ], max - count );
	}
	return count;
}

/*
================
sdTransport::AddCustomConstraints
================
*/
int sdTransport::AddCustomConstraints( constraintInfo_t* list, int max ) {
	int count = 0;
	int i;
	for ( i = 0; i < driveObjects.Num() && ( count < max ); i++ ) {
		count += driveObjects[ i ]->AddCustomConstraints( &list[ count ], max - count );
	}
	return count;
}

/*
================
sdTransport::NextWeaponIndex
================
*/
int sdTransport::NextWeaponIndex( idPlayer* player, int index ) {
	sdVehiclePosition& position = positionManager.PositionForPlayer( player );

	while ( true ) {
		index++;

		if ( index < 0 || index >= weapons.Num() ) {
			return -1;
		}

		if ( position.WeaponValid( weapons[ index ] ) ) {
			return index;
		}
	}
}

/*
================
sdTransport::PrevWeaponIndex
================
*/
int sdTransport::PrevWeaponIndex( idPlayer* player, int index ) {
	sdVehiclePosition& position = positionManager.PositionForPlayer( player );

	while ( true ) {
		index--;

		if ( index < 0 || index >= weapons.Num() ) {
			return -1;
		}

		if ( position.WeaponValid( weapons[ index ] ) ) {
			return index;
		}
	}
}

/*
================
sdTransport::GetViewForPlayer
================
*/
sdVehicleView& sdTransport::GetViewForPlayer( const idPlayer* player ) {
	return positionManager.PositionForPlayer( player ).GetViewParms();
}

/*
================
sdTransport::PostThink
================
*/
void sdTransport::PostThink( void ) {
	UpdateDecay();
	UpdateLockAlarm();

	if ( !vehicleFlags.amphibious ) {
		UpdateWaterDamage();
	}

	UpdatePlayZoneInfo();

	CheckFlipped();

	lights.Update( input );

	if ( guiInterface ) {
		guiInterface->UpdateGui();
	}

	positionManager.UpdatePlayerViews();
	positionManager.Think();
	usableInterface.OnPostThink();

	for ( int i = 0; i < weapons.Num(); i++ ) {
		weapons[ i ]->Update();
	}

	positionManager.PresentPlayers();

	if ( ( predictionErrorDecay_Origin != NULL && predictionErrorDecay_Origin->NeedsUpdate() ) 
		|| ( predictionErrorDecay_Angles != NULL && predictionErrorDecay_Angles->NeedsUpdate() ) ) {
		fl.allowPredictionErrorDecay = true;
		UpdateVisuals();
	}

	UpdateVisibility();
	Present();
}

/*
================
sdTransport::GetPostThinkNode
================
*/
idLinkList< idEntity >* sdTransport::GetPostThinkNode( void ) {
	return &postThinkEntNode;
}

/*
================
sdTransport::UpdateViews
================
*/
void sdTransport::UpdateViews( sdVehicleWeapon* weapon ) {
	int i;
	for ( i = 0; i < positionManager.NumPositions(); i++ ) {
		positionManager.PositionForId( i )->UpdateViews( weapon );
	}
}


/*
===================
sdTransport::ReloadVehicleScripts
===================
*/
void sdTransport::ReloadVehicleScripts( idDecl* decl ) {
	idEntity *ent;

	if ( gameLocal.GetLocalPlayer() && !gameLocal.CheatsOk( false ) ) {
		return;
	}

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if( ent->IsType( sdTransport::Type ) ) {
			sdTransport* transport = reinterpret_cast< sdTransport* >( ent );
			if( transport->GetVehicleScript() != decl ) {
				continue;
			}

			transport->GetPositionManager().OnVehicleScriptReloaded();
			transport->LoadVehicleScript();
		}
	}
}

/*
===================
sdTransport::DisableWheelSuspension
===================
*/
void sdTransport::DisableWheelSuspension( bool disable ) {
	int i;
	for ( i = 0; i < driveObjects.Num(); i++ ) {
		sdVehicleRigidBodyWheel* wheel = driveObjects[ i ]->Cast< sdVehicleRigidBodyWheel >();
		if ( wheel == NULL ) {
			continue;
		}

		wheel->DisableSuspension( disable );
	}
}

/*
===================
sdTransport::AreTracksOnGround
===================
*/
bool sdTransport::AreTracksOnGround( void ) const {
	bool onGround = true;
	for ( int i = 0; i < driveObjects.Num(); i++ ) {
		sdVehicleTrack* track = driveObjects[ i ]->Cast< sdVehicleTrack >();
		if ( track == NULL ) {
			continue;
		}
		
		onGround = onGround && track->IsGrounded();
	}

	return onGround;
}

/*
===============
sdTransport::AreWheelsOnGround
===============
*/
float sdTransport::AreWheelsOnGround( void ) const {
	int numWheels = 0;
	int numGrounded = 0;
	int numTracks = 0;
	int numGroundedTracks = 0;
	for( int i = 0; i < driveObjects.Num(); i++ ) {
		sdVehicleRigidBodyWheel* wheel = driveObjects[ i ]->Cast< sdVehicleRigidBodyWheel >();
		if ( wheel ) {
			if ( wheel->IsGrounded() ) {
				numGrounded++;
			}
			numWheels++;
		} else {
			// check tracks
			sdVehicleTrack* track = driveObjects[ i ]->Cast< sdVehicleTrack >();
			if ( track ) {
				if ( track->IsGrounded() ) {
					numGroundedTracks++;
				}
				numTracks++;
			}
		}
	}

	if ( numTracks > 0 ) {
		return numGroundedTracks / ( float )numTracks;
	}

	if ( numWheels > 0 ) {
		return numGrounded / ( float )numWheels;
	}

	return 1.0f;
}

/*
============
sdTransport::InitEffectList
============
*/
void sdTransport::InitEffectList( vehicleEffectList_t& list, const char* effectName, int& numSurfaceTypes ) {
	idStr effectValue;
	numSurfaceTypes = gameLocal.declSurfaceTypeType.Num() + 1;
	if ( ( int )list.Size() < numSurfaceTypes ) {
		gameLocal.Error( "sdTransport::InitEffectList - numSurfaceTypes > MAX_VEHICLE_EFFECTS" );
	}

	list.SetNum( numSurfaceTypes );

	idStr defaultValue;
	defaultValue = spawnArgs.GetString( va( "%s_default", effectName ), "" );
	if ( defaultValue.Length() != 0 ) {
		list[ 0 ].Init( defaultValue );
		list[ 0 ].GetRenderEffect().loop = true;
	}

	for( int i = 1; i < numSurfaceTypes; i++ ) {
		const sdDeclSurfaceType* surfaceType = gameLocal.declSurfaceTypeType.SafeIndex( i - 1 );
		effectValue = spawnArgs.GetString( va( "%s_%s", effectName, surfaceType->GetName()), "" );

		if ( effectValue.Length() == 0 ) {
			effectValue = defaultValue;
		}

		if ( effectValue.Length() != 0 ) {
			list[ i ].Init( effectValue );
			list[ i ].GetRenderEffect().loop = true;		
		}		
	}
}	

/*
============
sdTransport::CheckWater
============
*/
void sdTransport::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	bool showWake = true;
	idPlayer *p = gameLocal.GetLocalPlayer();
	if ( p != NULL ) {
		idEntity* proxy = p->GetProxyEntity();
		if ( proxy == this ) {
			sdVehicleView& vehicleView = GetViewForPlayer( p );
			showWake = vehicleView.ShowCockpit() || (!vehicleView.GetViewParms().hideVehicle);
		}
	}

	int i;
	for ( i = 0; i < driveObjects.Num(); i++ ) {
		driveObjects[ i ]->CheckWater( waterBodyOrg, waterBodyAxis, waterBodyModel );
	}

	if ( waterEffects ) {
		waterEffects->SetOrigin( GetPhysics()->GetOrigin() );
		waterEffects->SetAxis( GetPhysics()->GetAxis() );
		waterEffects->SetVelocity( GetPhysics()->GetLinearVelocity() );
		waterEffects->CheckWater( this, waterBodyOrg, waterBodyAxis, waterBodyModel, showWake );
	}
}

/*
============
sdTransport::SetupCockpit
============
*/
void sdTransport::SetupCockpit( idPlayer* viewPlayer ) {
	const sdVehiclePosition& pos = positionManager.PositionForPlayer( viewPlayer );

	const idDict* cockpitInfo = NULL;
	const char* cockpitName = pos.GetCockpit();
	if ( *cockpitName ) {
		const idList< sdPair< idStr, idDict > >& info = vehicleScript->GetCockpitInfo();

		for ( int i = 0; i < info.Num(); i++ ) {
			if ( info[ i ].first.Icmp( cockpitName ) ) {
				continue;
			}

			cockpitInfo = &info[ i ].second;
			break;
		}
	}

	if ( cockpitInfo ) {
		gameLocal.playerView.SetupCockpit( *cockpitInfo, this );
	}
}

/*
============
sdTransport::Pain
============
*/
bool sdTransport::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	if ( !damageDecl || !damageDecl->GetNoPain() ) {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer && localPlayer->GetProxyEntity() == this ) {
			idVec3 dirNormalized = -dir;
			dirNormalized.Normalize();	

			if ( damageDecl == NULL || !damageDecl->GetNoDirection() ) {
				localPlayer->AddDamageEvent( gameLocal.time, dirNormalized.ToAngles().Normalize180().yaw, damage, true );
			} else {
				localPlayer->AddDamageEvent( gameLocal.time, 0, damage, false );
				localPlayer->AddDamageEvent( gameLocal.time, 90, damage, false );
				localPlayer->AddDamageEvent( gameLocal.time, 180, damage, false  );
				localPlayer->AddDamageEvent( gameLocal.time, 270, damage, false  );
			}
		}
	}

	return true;
}

/*
============
sdTransport::SetTrackerEntity
============
*/
void sdTransport::SetTrackerEntity( idEntity* trackerEntity ) {
	if ( gameLocal.isServer ) {
		// Gordon: these are non-networked entities ( or should be ), so the spawnId wont always match => just use entityNum
		sdEntityBroadcastEvent msg( this, EVENT_ROUTETRACKERENTITY );
		msg.WriteLong( trackerEntity ? trackerEntity->entityNumber : -1 );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}

	routeTracker.SetTrackerEntity( trackerEntity );
	if ( routeTracker.IsValid() ) {
		BecomeActive( TH_THINK );
	}
}


/*
============
sdTransport::Event_GetMinDisplayHealth
============
*/
void sdTransport::Event_GetMinDisplayHealth() {
	if ( vehicleControl != NULL ) {
		sdProgram::ReturnInteger( vehicleControl->GetMinDisplayHealth() );
		return;
	}
	sdProgram::ReturnInteger( 0 );
}

/*
============
sdTransport::Event_SetDeathThroeHealth
============
*/
void sdTransport::Event_SetDeathThroeHealth( int amount ) {
	if ( vehicleControl != NULL ) {
		vehicleControl->SetDeathThroeHealth( amount );
	}
}

/*
============
sdTransport::SetDeathThroes
============
*/
void sdTransport::SetDeathThroes( bool enabled ) {
	vehicleFlags.deathThroes = enabled;
}

/*
============
sdTransport::SetCareening
============
*/
void sdTransport::SetCareening( int startTime ) {
	careenStartTime = startTime;
}

/*
============
sdTransport::IsEMPed
============
*/
bool sdTransport::IsEMPed( void ) {
	if ( empTime > gameLocal.time ) {
		return true;
	}

	return false;
}

/*
============
sdTransport::IsWeaponEMPed
============
*/
bool sdTransport::IsWeaponEMPed( void ) {
	if ( weaponEmpTime > gameLocal.time ) {
		return true;
	}

	return false;
}

/*
============
sdTransport::SetEMPTime
============
*/
void sdTransport::SetEMPTime( int newTime, int newWeaponTime ) {
	if ( empTime != newTime ) {
		bool wasEMPed = IsEMPed();
		empTime = newTime;

		CancelEvents( &EV_EMPChanged );
		if ( empTime > gameLocal.time ) {
			PostEventMS( &EV_EMPChanged, empTime - gameLocal.time );
		}

		if ( !wasEMPed ) {
			OnEMPStateChanged();
		}
	}

	if ( weaponEmpTime != newWeaponTime ) {
		bool wasWeaponEMPed = IsWeaponEMPed();
		weaponEmpTime = newWeaponTime;

		CancelEvents( &EV_WeaponEMPChanged );
		if ( weaponEmpTime > gameLocal.time ) {
			PostEventMS( &EV_WeaponEMPChanged, weaponEmpTime - gameLocal.time );
		}

		if ( !wasWeaponEMPed ) {
			OnWeaponEMPStateChanged();
		}
	}
}

/*
============
sdTransport::OnEMPStateChanged
============
*/
void sdTransport::OnEMPStateChanged( void ) {
	if ( vehicleControl != NULL ) {
		vehicleControl->OnEMPStateChanged();
	}
	if ( vehicleSoundControl != NULL ) {
		vehicleSoundControl->OnEMPStateChanged();
	}

/*	if ( IsEMPed() ) {
		gameLocal.Printf( "EMP Started\n" );
	} else {
		gameLocal.Printf( "EMP Stopped\n" );
	}*/
}

/*
============
sdTransport::OnWeaponEMPStateChanged
============
*/
void sdTransport::OnWeaponEMPStateChanged( void ) {
	if ( vehicleControl != NULL ) {
		vehicleControl->OnWeaponEMPStateChanged();
	}
	if ( vehicleSoundControl != NULL ) {
		vehicleSoundControl->OnWeaponEMPStateChanged();
	}
	
/*	if ( IsWeaponEMPed() ) {
		gameLocal.Printf( "EMP Weapon Started\n" );
	} else {
		gameLocal.Printf( "EMP Weapon Stopped\n" );
	}*/
}

/*
============
sdTransport::GetRemainingEMP
============
*/
float sdTransport::GetRemainingEMP( void ) {
	if ( gameLocal.time > empTime ) {
		return 0.0f;
	}
	return MS2SEC( empTime - gameLocal.time );
}

/*
============
sdTransport::GetRemainingWeaponEMP
============
*/
float sdTransport::GetRemainingWeaponEMP( void ) {
	if ( gameLocal.time > weaponEmpTime ) {
		return 0.0f;
	}
	return MS2SEC( weaponEmpTime - gameLocal.time );
}

/*
============
sdTransport::Event_IsEMPed
============
*/
void sdTransport::Event_IsEMPed( void ) {
	sdProgram::ReturnBoolean( IsEMPed() );
}

/*
============
sdTransport::Event_IsWeaponEMPed
============
*/
void sdTransport::Event_IsWeaponEMPed( void ) {
	sdProgram::ReturnBoolean( IsWeaponEMPed() );
}

/*
============
sdTransport::Event_ApplyEMPDamage
============
*/
void sdTransport::Event_ApplyEMPDamage( float time, float weaponTime ) {
	int newTime = gameLocal.time + SEC2MS( time );
	int newWeaponTime = gameLocal.time + SEC2MS( weaponTime );
	if ( newTime > empTime || newWeaponTime > weaponEmpTime ) {
		SetEMPTime( newTime, newWeaponTime );

		Event_ResetDecayTime();
		
		sdProgram::ReturnBoolean( true );
	} else {
		sdProgram::ReturnBoolean( false );
	}
}

/*
============
sdTransport::Event_GetRemainingEMP
============
*/
void sdTransport::Event_GetRemainingEMP( void ) {
	sdProgram::ReturnFloat( GetRemainingEMP() );
}

/*
============
sdTransport::IsWeaponDisabled
============
*/
bool sdTransport::IsWeaponDisabled( void ) const {
	return vehicleFlags.weaponDisabled;
}

/*
============
sdTransport::Event_IsWeaponDisabled
============
*/
void sdTransport::Event_IsWeaponDisabled( void ) {
	sdProgram::ReturnBoolean( IsWeaponDisabled() );
}

/*
============
sdTransport::Event_SetWeaponDisabled
============
*/
void sdTransport::Event_SetWeaponDisabled( bool disabled ) {
	vehicleFlags.weaponDisabled = disabled;
}


/*
============
sdTransport::Event_SetLockAlarmActive
============
*/
void sdTransport::Event_SetLockAlarmActive( bool active ) {
	vehicleFlags.lockAlarmActive = active;
}

/*
============
sdTransport::Event_SetImmobilized
============
*/
void sdTransport::Event_SetImmobilized( bool immobile ) {
	if ( vehicleControl != NULL ) {
		vehicleControl->SetImmobilized( immobile );
	}
}

/*
============
sdTransport::Event_InSiegeMode
============
*/
void sdTransport::Event_InSiegeMode() {
	sdProgram::ReturnBoolean( vehicleControl && vehicleControl->InSiegeMode() );
}

/*
============
sdTransport::Event_SetArmor
============
*/
void sdTransport::Event_SetArmor( float _armor ) {
	armor = idMath::ClampFloat( 0.f, 1.f, _armor );
}

/*
============
sdTransport::Event_GetArmor
============
*/
void sdTransport::Event_GetArmor( void ) {
	sdProgram::ReturnFloat( armor );
}

/*
============
sdTransport::Event_GetNumWeapons
============
*/
void sdTransport::Event_GetNumWeapons( void ) {
	sdProgram::ReturnInteger( NumWeapons() );
}

/*
============
sdTransport::Event_GetWeapon
============
*/
void sdTransport::Event_GetWeapon( int index ) {
	if ( index < 0 || index >= NumWeapons() ) {
		sdProgram::ReturnObject( NULL );
		return;
	}
	sdVehicleWeapon* weapon = GetWeapon( index );
	sdProgram::ReturnObject( weapon->GetScriptObject() );
}

/*
============
sdTransport::Event_SetTrackerEntity
============
*/
void sdTransport::Event_SetTrackerEntity( idEntity* trackerEntity ) {
	if ( gameLocal.isClient ) {
		return;
	}
	SetTrackerEntity( trackerEntity );
}

/*
============
sdTransport::Event_IsPlayerBanned
============
*/
void sdTransport::Event_IsPlayerBanned( idEntity* entity ) {
	idPlayer* player = entity->Cast< idPlayer >();
	if ( player == NULL ) {
		gameLocal.Warning( "sdTransport::Event_IsPlayerBanned Entity is not a Player" );
		sdProgram::ReturnBoolean( true );
		return;
	}

	sdProgram::ReturnBoolean( positionManager.IsPlayedBanned( player->entityNumber ) );
}

/*
============
sdTransport::Event_BanPlayer
============
*/
void sdTransport::Event_BanPlayer( idEntity* entity, float time ) {
	idPlayer* player = entity->Cast< idPlayer >();
	if ( player == NULL ) {
		gameLocal.Warning( "sdTransport::Event_BanPlayer Entity is not a Player" );
		return;
	}

	positionManager.BanPlayer( player->entityNumber, SEC2MS( time ) );
}

/*
============
sdTransport::Event_BanPlayer
============
*/
void sdTransport::Event_ClearLastAttacker() {
	SetLastAttacker( NULL, NULL );
}

/*
============
sdTransport::Event_EMPChanged
============
*/
void sdTransport::Event_EMPChanged( void ) {
	OnEMPStateChanged();
}

/*
============
sdTransport::Event_WeaponEMPChanged
============
*/
void sdTransport::Event_WeaponEMPChanged( void ) {
	OnWeaponEMPStateChanged();
}


/*
============
sdTransport::ClientReceiveEvent
============
*/
bool sdTransport::ClientReceiveEvent( int event, int time, const idBitMsg& msg ) {
	switch ( event ) {
		case EVENT_CONTROLMESSAGE:
			if ( vehicleControl != NULL ) {
				vehicleControl->OnNetworkEvent( time, msg );
			}
			return true;

		case EVENT_SETTELEPORTER: {
			int spawnId = msg.ReadLong();
			SetTeleportEntity( gameLocal.EntityForSpawnId( spawnId )->Cast< sdTeleporter >() );
			return true;
		}
		
		case EVENT_PARTSTATE: {
			int partNum = msg.ReadBits( BitsForPartIndex() ) - 1;
			if ( partNum < -1 || partNum >= driveObjects.Num() ) {
				gameLocal.Warning( "sdTransport::ClientReceiveEvent - Received EVENT_PARTSTATE for part out of range!" );
				return true;
			}

			int loopStart = partNum;
			int loopEnd = partNum + 1;
			if ( partNum == -1 ) {
				// -1 means to do all of em
				loopStart = 0;
				loopEnd = driveObjects.Num();

				bool decaying = msg.ReadBool();
				if ( decaying ) {
					Decayed();
				}
			}

			for ( int i = loopStart; i < loopEnd; i++ ) {
				sdVehicleDriveObject* part = driveObjects[ i ];
				bool hidden = msg.ReadBool();
				if ( hidden ) {
					part->Detach( true, vehicleFlags.decaying );
				} else {
					part->Reattach();
				}
			}
			return true;
		}
		case EVENT_DECAY: {
			Decayed();
			return true;
		}
		case EVENT_ROUTEWARNING: {
			SetRouteWarning( msg.ReadBool() );
			return true;
		}
		case EVENT_ROUTEMASKWARNING: {
			routeMaskWarningEndTime = msg.ReadLong();
			return true;
		}
		case EVENT_ROUTETRACKERENTITY: {
			// Gordon: special case using entityNumber here
			int entityNumber = msg.ReadLong();
			if ( entityNumber == -1 ) {
				SetTrackerEntity( NULL );
			} else {
				SetTrackerEntity( gameLocal.entities[ entityNumber ] );
			}
			return true;
		}
	}

	return sdScriptEntity::ClientReceiveEvent( event, time, msg );
}

/*
============
sdTransport::GetDecalUsage
============
*/
cheapDecalUsage_t sdTransport::GetDecalUsage( void ) {
	if ( vehicleFlags.decals ) {
		return CDU_LOCAL;
	} else {
		return CDU_INHIBIT;
	}
}

/*
================
sdTransport::UpdateModelTransform
================
*/
void sdTransport::UpdateModelTransform( void ) {
	sdTeleporter* teleportEnt = teleportEntity;
	if ( teleportEnt != NULL ) {
		idPlayer* player = gameLocal.GetLocalViewPlayer();
		if ( player != NULL && player->GetProxyEntity() == this ) {
			idEntity* viewer = teleportEnt->GetViewEntity();
			if ( viewer != NULL ) {
				renderEntity.axis	= viewer->GetPhysics()->GetAxis();
				renderEntity.origin	= viewer->GetPhysics()->GetOrigin();
				return;
			}
		}
	}

	renderEntity.axis	= GetPhysics()->GetAxis();
	renderEntity.origin	= GetPhysics()->GetOrigin();

	DoPredictionErrorDecay();
}

/*
================
sdTransport::OnTeleportStarted
================
*/
void sdTransport::OnTeleportStarted( sdTeleporter* teleporter ) {
	SetTeleportEntity( teleporter );

	GetPhysics()->SetLinearVelocity( vec3_zero );
}

/*
================
sdTransport::OnTeleportFinished
================
*/
void sdTransport::OnTeleportFinished( void ) {
	SetTeleportEntity( NULL );

	nextTeleportTime = gameLocal.time; // + SEC2MS( 2.f );
}

/*
================
sdTransport::SetTeleportEntity
================
*/
void sdTransport::SetTeleportEntity( sdTeleporter* teleporter ) {
	teleportEntity = teleporter;

	if ( scriptObject != NULL ) {
		idScriptObject* teleporterScript = teleporter == NULL ? NULL : teleporter->GetScriptObject();

		sdScriptHelper h1;
		h1.Push( teleporterScript );
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnTeleportEntityChanged" ), h1 );
	}

	// clear any lock-ons
	if ( !gameLocal.isClient ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* p = gameLocal.GetClient( i );
			if ( p == NULL ) {
				continue;
			}
			if ( p->targetEntity == this ) {
				p->SetTargetEntity( NULL );
			}
			if ( p->GetProxyEntity() == this ) {
				p->UpdateDeltaViewAngles( idAngles( 0.f, 0.f, 0.f ) );
			}
		}
	}

	// reset the damped things
	if ( vehicleControl != NULL ) {
		vehicleControl->OnTeleport();
	}
	for ( int i = 0; i < positionManager.NumPositions(); i++ ) {
		sdVehiclePosition* position = positionManager.PositionForId( i );
		if ( position->GetPlayer() != NULL ) {
			position->GetViewParms().OnTeleport();
		}
	}
	if ( !gameLocal.isClient ) {
		ResetPredictionErrorDecay();
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SETTELEPORTER );
		msg.WriteLong( gameLocal.GetSpawnId( teleporter ) );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdTransport::PlayHitBeep
================
*/
int sdTransport::PlayHitBeep( idPlayer* player, bool headshot ) const {
	int duration = 0;
	if( GetEntityAllegiance( player ) == TA_ENEMY ) {
		player->StartSound( "snd_hit_vehicle_feedback", SND_ANY, SSF_PRIVATE_SOUND, &duration );
	} else {
		player->StartSound( "snd_hit_vehicle_friendly_feedback", SND_ANY, SSF_PRIVATE_SOUND, &duration );
	}
	return duration;
}

/*
================
sdTransport::OnKeyMove
================
*/
bool sdTransport::OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	if ( vehicleControl != NULL ) {
		return vehicleControl->OnKeyMove( forward, right, up, cmd );
	} else {
		return false;
	}
}

/*
================
sdTransport::OnControllerMove
================
*/
void sdTransport::OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
										   const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	if ( vehicleControl != NULL ) {
		return vehicleControl->OnControllerMove( doGameCallback, numControllers, controllerNumbers, controllerAxis, viewAngles, cmd );
	}
}

/*
================
sdTransport::SetMasterDestroyedPart
================
*/
void sdTransport::SetMasterDestroyedPart( rvClientMoveable* part ) { 
	masterDestroyedPart = part; 
}

/*
================
sdTransport::GetMasterDestroyedPart
================
*/
rvClientMoveable* sdTransport::GetMasterDestroyedPart( void ) { 
	return masterDestroyedPart; 
}

/*
================
sdTransport::GetMinDisplayHealth
================
*/
int sdTransport::GetMinDisplayHealth( void ) const {
	if ( vehicleControl ) {
		return vehicleControl->GetMinDisplayHealth();
	}
	return 0;
}

/*
================
sdTransport::UpdateShaderParms
================
*/
void sdTransport::UpdateShaderParms( void ) {
	if ( positionManager.IsEmpty() ) {
		lastEnteredTime = gameLocal.time;
	}
	SetShaderParm( 8, lastEnteredTime != 0 ? MS2SEC( gameLocal.time - lastEnteredTime ) : 0.f );
}

/*
================
sdTransport::SetRouteWarning
================
*/
void sdTransport::SetRouteWarning( bool value ) {
	if ( vehicleFlags.routeWarning == value ) {
		return;
	}
	vehicleFlags.routeWarning = value;

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_ROUTEWARNING );
		msg.WriteBool( value );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdTransport::SetRouteMaskWarning
================
*/
void sdTransport::SetRouteMaskWarning( bool value ) {
	if ( value ) {
		if ( routeMaskWarningEndTime != 0 ) {
			return;
		}
		routeMaskWarningEndTime = gameLocal.time + SEC2MS( 30.f );
	} else {
		if ( routeMaskWarningEndTime == 0 ) {
			return;
		}
		routeMaskWarningEndTime = 0;
		vehicleFlags.routeWarningTimeout = false;
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_ROUTEMASKWARNING );
		msg.WriteLong( routeMaskWarningEndTime );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}
}

/*
===============
sdTransport::HasLockonDanger
===============
*/
bool sdTransport::HasLockonDanger( void ) {
	if ( !enemyLockedOnUsFunc ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( enemyLockedOnUsFunc, h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
===============
sdTransport::IsDeployed
===============
*/
bool sdTransport::IsDeployed( void ) {
	if ( !isDeployedFunc ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( isDeployedFunc, h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
===============
sdTransport::Event_DisablePart
===============
*/
void sdTransport::Event_DisablePart( const char* name ) {
	sdVehicleDriveObject* object = GetDriveObject( name );
	if ( object != NULL ) {
		object->Detach( false, false );
	}
}

/*
===============
sdTransport::Event_EnablePart
===============
*/
void sdTransport::Event_EnablePart( const char* name ) {
	sdVehicleDriveObject* object = GetDriveObject( name );
	if ( object != NULL ) {
		object->Reattach();
	}
}

/*
===============
sdTransport::Event_DestructionTime
===============
*/
void sdTransport::Event_DestructionTime( void ) {
	sdProgram::ReturnFloat( MS2SEC( routeMaskWarningEndTime ) );
}

/*
===============
sdTransport::Event_DirectionWarning
===============
*/
void sdTransport::Event_DirectionWarning( void ) {
	sdProgram::ReturnBoolean( vehicleFlags.routeWarning );
}

/*
===============
sdTransport::Event_IsTeleporting
===============
*/
void sdTransport::Event_IsTeleporting( void ) {
	sdProgram::ReturnBoolean( IsTeleporting() );
}

/*
===============
sdTransport::SetOrigin
===============
*/
void sdTransport::SetOrigin( const idVec3 &org ) {
	GetPhysics()->SetOrigin( org );
	ResetPredictionErrorDecay();
	UpdateVisuals();
	Present();
}

/*
================
sdTransport::SetAxis
================
*/
void sdTransport::SetAxis( const idMat3 &axis ) {
	GetPhysics()->SetAxis( axis );
	ResetPredictionErrorDecay();
	UpdateVisuals();
	Present();
}

/*
================
sdTransport::OnPhysicsRested
================
*/
void sdTransport::OnPhysicsRested( void ) {
	if ( !positionManager.IsEmpty() || gameLocal.IsPaused() ) {
		return;
	}
	idEntity::OnPhysicsRested();
}
