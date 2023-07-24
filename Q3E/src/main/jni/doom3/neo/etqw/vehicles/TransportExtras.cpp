// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../Player.h"
#include "TransportExtras.h"
#include "VehicleView.h"
#include "VehicleWeapon.h"
#include "Transport.h"
#include "VehicleIK.h"
#include "../ContentMask.h"
#include "../script/Script_Helper.h"
#include "../proficiency/StatsTracker.h"

#include "../botai/Bot.h"
#include "../botai/BotThreadData.h"

/*
===============================================================================

	sdVehicleInput

===============================================================================
*/

void sdVehicleInput::SetPlayer( idPlayer* player ) {
	data.player		= player;
	if ( data.player != NULL ) {
		data.usercmd	= gameLocal.usercmds[ player->entityNumber ];
	}

	for ( int i = 0; i < 3; i++ ) {
		data.cmdAngles[ i ] = SHORT2ANGLE( data.usercmd.angles[ i ] );
	}
}

/*
================
sdVehicleInput::GetForward
================
*/
float sdVehicleInput::GetForward( void ) const {
	return data.usercmd.forwardmove / 127.f ;
}

/*
================
sdVehicleInput::GetRight
================
*/
float sdVehicleInput::GetRight( void ) const {
	return data.usercmd.rightmove / 127.f;
}

/*
================
sdVehicleInput::GetUp
================
*/
float sdVehicleInput::GetUp( void ) const {
	return data.usercmd.upmove / 127.f;
}

/*
================
sdVehicleInput::GetYaw
================
*/
float sdVehicleInput::GetCmdYaw( void ) const {
	return idMath::AngleNormalize180( data.cmdAngles.yaw );
}

/*
================
sdVehicleInput::GetPitch
================
*/
float sdVehicleInput::GetCmdPitch( void ) const {
	return idMath::AngleNormalize180( data.cmdAngles.pitch );
}

/*
================
sdVehicleInput::GetCmdRoll
================
*/
float sdVehicleInput::GetCmdRoll( void ) const {
	return idMath::AngleNormalize180( data.cmdAngles.roll );
}

/*
===============================================================================

sdVehiclePosition

===============================================================================
*/

/*
================
sdVehiclePosition::sdVehiclePosition
================
*/
sdVehiclePosition::sdVehiclePosition( void ) {
	player				= NULL;
	transport			= NULL;
	weaponIndex			= -1;
	index				= -1;
	blockedTip			= NULL;
	maxViewOffset		= 0.f;
	viewOffsetRate		= 0.f;
	currentViewOffset	= 0.f;
	currentViewOffsetAngles = 0.f;
	statTimeSpent		= NULL;
	playerHeight		= 0.0f;
}

/*
================
sdVehiclePosition::~sdVehiclePosition
================
*/
sdVehiclePosition::~sdVehiclePosition( void ) {
	for ( int i = 0; i < views.Num(); i++ ) {
		delete views[ i ];
	}
	for ( int i = 0; i < ikSystems.Num(); i++ ) {
		delete ikSystems[ i ];
	}
}

/*
================
sdVehiclePosition::LoadData
================
*/
void sdVehiclePosition::LoadData( const idDict& dict ) {
	requirements.Load( dict, "require" );

	const char* tipName = dict.GetString( "tt_blocked" );
	if ( *tipName ) {
		blockedTip = gameLocal.declToolTipType[ tipName ];
	}

	maxViewOffset	= dict.GetFloat( "max_view_offset", "0.f" );
	viewOffsetRate	= dict.GetFloat( "view_offset_rate", "0.f" );
	if ( maxViewOffset ) {
		viewOffsetRate /= maxViewOffset;
	}

	const char* attachJointName = dict.GetString( "joint_attach" );
	if ( !*attachJointName ) {
		attachJointName = "origin";
	}

	attachAnim				= dict.GetString( "player_anim", "VehicleDefault" );
	showPlayer				= dict.GetBool( "show_player" );
	minZfrac				= dict.GetFloat( "min_z_frac", "-2.f" );
	ejectOnKilled			= dict.GetBool( "use_fallback" );
	takesDamage				= dict.GetBool( "take_damage" );
	playerHeight			= dict.GetFloat( "player_height" );
	allowWeapon				= dict.GetBool( "allow_weapon" );
	allowAdjustBodyAngles	= dict.GetBool( "adjust_body_angles" );
	resetViewOnEnter		= dict.GetBool( "reset_view_on_enter", "1" );
	damageScale				= dict.GetFloat( "damage_scale", "1" );
	playerStance			= dict.GetBool( "player_stance_crouch" ) ? PS_CROUCH : PS_NORMAL;

	const char* iconJointName = dict.GetString( "joint_icon" );
	if ( *iconJointName != '\0' ) {
		iconJoint = transport->GetAnimator()->GetJointHandle( iconJointName );
	} else {
		iconJoint = INVALID_JOINT;
	}

	const idKeyValue* kv = NULL;
	while ( kv = dict.MatchPrefix( "ability", kv ) ) {
		abilities.Add( kv->GetValue() );
	}

	attachJoint				= transport->GetAnimator()->GetJointHandle( attachJointName );
	if ( attachJoint == INVALID_JOINT ) {
		gameLocal.Warning( "sdVehiclePosition::LoadData  Joint \"%s\" does not exist in vscript %s", attachJointName, transport->GetVehicleScript()->GetName() );
	}

	cockpitName = dict.GetString( "cockpit", "" );

	const char* statName = dict.GetString( "stat_name" );
	if ( *statName ) {
		sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

		statTimeSpent = tracker.GetStat( tracker.AllocStat( va( "%s_time_spent", statName ), sdNetStatKeyValue::SVT_INT ) );
	} else {
		gameLocal.Warning( "Missing Stat Name on '%s'", name.c_str() );
	}
}

/*
================
sdVehiclePosition::CheckRequirements
================
*/
bool sdVehiclePosition::CheckRequirements( idPlayer* player ) {
	return requirements.Check( player, transport );
}

/*
================
sdVehiclePosition::Think
================
*/
void sdVehiclePosition::Think( void ) {
	UpdateIK();
}

/*
================
sdVehiclePosition::UpdateViews
================
*/
void sdVehiclePosition::UpdateViews( sdVehicleWeapon* weapon ) {
	for ( int i = 0; i < views.Num(); i++ ) {
		views[ i ]->Update( weapon );
	}
}

/*
================
sdVehiclePosition::WeaponValid
================
*/
bool sdVehiclePosition::WeaponValid( sdVehicleWeapon* weapon ) const {
	return weapon->GetBasePosition() == this;
}

/*
================
sdVehiclePosition::GetPlayer
================
*/
idPlayer* sdVehiclePosition::GetPlayer( void ) const {
	return player;
}

/*
================
sdVehiclePosition::FindDefaultWeapon
================
*/
int sdVehiclePosition::FindDefaultWeapon( void ) const {
	if ( transport == NULL ) {
		return -1;
	}

	int i;
	for ( i = 0; i < transport->NumWeapons(); i++ ) {
		if ( WeaponValid( transport->GetWeapon( i ) ) ) {
			return i;
		}
	}
	return -1;
}

/*
================
sdVehiclePosition::SetWeaponIndex
================
*/
void sdVehiclePosition::SetWeaponIndex( int _weaponIndex ) {
	if ( weaponIndex == _weaponIndex ) {
		if ( weaponIndex == -1 || transport->GetWeapon( weaponIndex )->GetPosition() == this ) {
			return;
		}
	}

	if ( weaponIndex != -1 ) {
		if ( weaponIndex >= 0 && weaponIndex < transport->NumWeapons() ) {
			if ( transport->GetWeapon( weaponIndex )->GetPosition() == this ) {
				transport->GetWeapon( weaponIndex )->SetPosition( NULL );
			}
		}
	}
	weaponIndex = _weaponIndex;
	if ( weaponIndex != -1 ) {
		if ( weaponIndex >= 0 && weaponIndex < transport->NumWeapons() ) {
			transport->GetWeapon( weaponIndex )->SetPosition( this );
		}
	}
}

/*
================
sdVehiclePosition::FindLastWeapon
================
*/
int sdVehiclePosition::FindLastWeapon( void ) const {
	int i;
	for ( i = transport->NumWeapons() - 1; i >= 0; i-- ) {
		if ( WeaponValid( transport->GetWeapon( i ) ) ) {
			return i;
		}
	}
	return -1;
}

/*
================
sdVehiclePosition::SetPlayer
================
*/
void sdVehiclePosition::SetPlayer( idPlayer* _player ) {
	idPlayer* p = player;
	if ( p && statTimeSpent ) {
		statTimeSpent->IncreaseValue( p->entityNumber, ( int )MS2SEC( gameLocal.time - playerEnteredTime ) );
	}

	player = _player;
	playerEnteredTime = gameLocal.time;

	if ( weaponIndex >= 0 && weaponIndex < transport->NumWeapons() ) {
		transport->GetWeapon( weaponIndex )->OnPositionPlayerChanged();
	}

	if ( !gameLocal.isClient ) {
		// force us to the default weapon
		SetWeaponIndex( -1 );
	}
	transport->SortWeapons();

	if ( player != NULL ) {
		player->vehicleViewCurrentZoom = 0;
	}
}

/*
================
sdVehiclePosition::GetViewParms
================
*/
sdVehicleView& sdVehiclePosition::GetViewParms( void ) {
	int viewMode = 0;

	if ( !player ) {
		assert( false );
		gameLocal.Warning( "sdVehiclePosition::GetViewParms NULL Player" );
	} else {
		viewMode = player->GetProxyViewMode();
	}

	if ( viewMode < 0 || viewMode >= views.Num() ) {
		if ( views.Num() <= 0 ) {
			gameLocal.Error( "sdVehiclePosition::GetViewParms No Views on Position" );
		}
		return *views[ 0 ];
	}

	return *views[ viewMode ];
}

/*
================
sdVehiclePosition::GetViewParms
================
*/
const sdVehicleView& sdVehiclePosition::GetViewParms( void ) const {
	int viewMode = 0;

	if ( !player ) {
		assert( false );
		gameLocal.Warning( "sdVehiclePosition::GetViewParms NULL Player" );
	} else {
		viewMode = player->GetProxyViewMode();
	}

	if ( viewMode < 0 || viewMode >= views.Num() ) {
		if ( views.Num() <= 0 ) {
			gameLocal.Error( "sdVehiclePosition::GetViewParms No Views on Position" );
		}
		return *views[ 0 ];
	}

	return *views[ viewMode ];
}

/*
================
sdVehiclePosition::AddIKSystem
================
*/
void sdVehiclePosition::AddIKSystem( sdVehicleIKSystem* _ikSystem ) { 
	sdVehicleIKSystem** allocedIK = ikSystems.Alloc();
	if ( allocedIK == NULL ) {
		gameLocal.Error( "sdVehiclePosition::AddIKSystem number of ik systems for position exceeds MAX_POSITION_IK" );
	}

	*allocedIK = _ikSystem;
}

/*
================
sdVehiclePosition::AddView
================
*/
void sdVehiclePosition::AddView( const positionViewMode_t& parms ) {
	sdVehicleView* view = sdVehicleView::AllocView( parms.type );
	if ( view == NULL ) {
		gameLocal.Warning( "sdVehiclePosition::AddView Invalid View Type '%s'", parms.type.c_str() );
		return;
	}

	sdVehicleView** allocedView = views.Alloc();
	if ( allocedView == NULL ) {
		gameLocal.Error( "sdVehiclePosition::AddView number of views for position exceeds MAX_POSITION_VIEWS" );
	}

	*allocedView = view;
	view->Init( this, parms );
}

/*
================
sdVehiclePosition::CycleCamera
================
*/
void sdVehiclePosition::CycleCamera( void ) {	
	int viewMode = player->GetProxyViewMode();
	viewMode++;
	viewMode %= views.Num();
	player->SetProxyViewMode( viewMode, false );

	if ( player->userInfo.rememberCameraMode ) {
		const sdDeclVehicleScript* script = transport->GetVehicleScript();
		script->SetCameraMode( player->entityNumber, GetPositionId(), viewMode );
	}
}

/*
================
sdVehiclePosition::UpdatePlayerView
================
*/
void sdVehiclePosition::UpdatePlayerView( void ) {
	idPlayer* currentPlayer = player;
	if ( currentPlayer ) {
		currentPlayer->CalculateView();
	}
}

/*
================
sdVehiclePosition::PresentPlayer
================
*/
void sdVehiclePosition::PresentPlayer( void ) {
	idPlayer* currentPlayer = player;
	if ( currentPlayer ) {
		currentPlayer->UpdateVisuals();
		currentPlayer->Present();

		idWeapon* weapon = currentPlayer->GetWeapon();
		if ( weapon ) {
			weapon->PresentWeapon();
		}

//		if ( allowWeapon ) {
//			currentPlayer->AdjustBodyAngles();
//		}
	}
}

/*
================
sdVehiclePosition::UpdateIK
================
*/
void sdVehiclePosition::UpdateIK( void ) {
	if ( transport->GetInput().GetSteerAngle() > 0 ) {
		currentViewOffset += viewOffsetRate * MS2SEC( gameLocal.msec );
		if ( currentViewOffset > 1.f ) {
			currentViewOffset = 1.f;
		}
	} else if ( transport->GetInput().GetSteerAngle() < 0 ) {
		currentViewOffset -= viewOffsetRate * MS2SEC( gameLocal.msec );
		if ( currentViewOffset < -1.f ) {
			currentViewOffset = -1.f;
		}
	} else {
		currentViewOffset -= Min( idMath::Fabs( currentViewOffset ), viewOffsetRate * MS2SEC( gameLocal.msec ) ) * idMath::Sign( currentViewOffset );
	}

	if ( !gameLocal.isClient ) {
		if ( minZfrac != -2.f && transport->GetPhysics()->GetAxis()[ 2 ].z < minZfrac ) {
			transport->GetPositionManager().EjectPlayer( *this, true );
		}
	}

	currentViewOffsetAngles = ( idMath::Cos( DEG2RAD( ( currentViewOffset * 180 ) + 180 ) ) + 1 ) * 0.5f * idMath::Sign( currentViewOffset ) * maxViewOffset;
	
	for ( int i = 0; i < ikSystems.Num(); i++ ) {
		ikSystems[ i ]->Update();
	}
}

/*
================
sdVehiclePosition::ClampAngle
================
*/
bool sdVehiclePosition::ClampAngle( idAngles& newAngles, const idAngles& oldAngles, angleClamp_t clamp, int index, float epsilon ) {
	if ( clamp.flags.limitRate ) {
		float rate = clamp.rate[ 0 ] * MS2SEC( gameLocal.msec );
		float diff = idMath::AngleDelta( newAngles[ index ], oldAngles[ index ] );

		if( diff > rate ) {
			newAngles[ index ] = idMath::AngleNormalize180( oldAngles[ index ] + rate );
		} else if ( diff < -rate ) {
			newAngles[ index ] = idMath::AngleNormalize180( oldAngles[ index ] - rate );
		}
	}

	if ( clamp.flags.enabled ) {
		float mid = ( clamp.extents[ 0 ] + clamp.extents[ 1 ] ) * 0.5f;
		float range = ( clamp.extents[ 1 ] - clamp.extents[ 0 ] ) * 0.5f;

		float temp = idMath::AngleNormalize180( newAngles[ index ] - mid );
		if ( temp < -range ) {
			temp = -range;
		} else if ( temp > range ) {
			temp = range;
		}
		newAngles[ index ] = temp + mid;
	}

	if ( epsilon <= 0.f ) {
		return newAngles[ index ] == oldAngles[ index ];
	}
	return idMath::Fabs( newAngles[ index ] - oldAngles[ index ] ) < epsilon;
}

/*
================
sdVehiclePosition::HasAbility
================
*/
bool sdVehiclePosition::HasAbility( qhandle_t handle ) const {
	return abilities.HasAbility( handle );
}

/*
===============================================================================

sdTransportPositionManager

===============================================================================
*/

/*
================
sdTransportPositionManager::sdTransportPositionManager
================
*/
sdTransportPositionManager::sdTransportPositionManager( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		bannedPlayers[ i ] = 0;
	}
}

/*
================
sdTransportPositionManager::~sdTransportPositionManager
================
*/
sdTransportPositionManager::~sdTransportPositionManager( void ) {
	ClearPositions();
}

/*
================
sdTransportPositionManager::EjectAllPlayers
================
*/
void sdTransportPositionManager::EjectAllPlayers( int flags ) {
	if ( gameLocal.isClient ) {
		return;
	}

	for( int i = 0; i < positions.Num(); i++ ) {
		idPlayer* player = positions[ i ].GetPlayer();
		EjectPlayer( positions[ i ], true );
		if ( player != NULL ) {
			const sdDeclDamage* damageDecl = transport->GetLastDamage();
			if ( flags & EF_KILL_PLAYERS ) {
				if ( ( damageDecl != NULL && damageDecl->GetForcePassengerKill() ) || !positions[ i ].GetEjectOnKilled() ) {
					player->Kill( transport->GetLastAttacker(), true, transport->GetKillPlayerDamage(), damageDecl );
				}
			}

			i = -1; // in case any players have been moved around due to the removal of this player
		}
	}
}

/*
================
sdTransportPositionManager::FreePosition
================
*/
sdVehiclePosition* sdTransportPositionManager::FreePosition( idPlayer* player, const sdDeclToolTip** tip, int startIndex ) {
	sdVehiclePosition*	blocked = NULL;
	int					blockedCount = 0;

	if ( tip ) {
		*tip = NULL;
	}

	for ( int i = startIndex; i < positions.Num(); i++ ) {
		if ( positions[ i ].GetPlayer() ) {
			if ( !positions[ i ].GetPlayer()->IsType( idBot::Type ) || player->IsType( idBot::Type ) ) {
                continue;
			}
		} //mal: bots dont count - they'll get booted out of this seat, and into another. ONLY for humans tho - they won't boot other bots.

		if ( i == 0 && transport->IsLocked() ) {
			blocked = &positions[ i ];
			blockedCount++;
			continue;
		}

		if ( !positions[ i ].CheckRequirements( player ) ) {
			blocked = &positions[ i ];
			blockedCount++;
			continue;
		}
		return &positions[ i ];
	}

	if ( blockedCount == 1 && tip ) {
		*tip = blocked->GetBlockedToolTip();
	}

	return NULL;
}


/*
================
sdTransportPositionManager::SwapPosition
================
*/
void sdTransportPositionManager::SwapPosition( idPlayer* player, bool allowCycle ) {
	bool haveBotInSeat;
	int startpos = player->GetProxyPositionId();
	int i;
	int	endpos;
	sdVehiclePosition* botPosition = NULL;
	idPlayer* botPlayer;
	
	bool cycle = transport->CycleAllPositions();

	if ( cycle && allowCycle ) {
		i = ( startpos + 1 ) % positions.Num();
		endpos = startpos;
	} else {
		i = 0;
		endpos = positions.Num();
	}

	while( i != endpos ) {

		if ( ( positions[ i ].GetPlayer() == NULL || positions[ i ].GetPlayer()->IsType( idBot::Type ) && !player->IsType( idBot::Type ) ) 
			&& positions[ i ].CheckRequirements( player ) 
			&& ( i != 0 || !transport->IsLocked() ) ) {
			
			haveBotInSeat = ( positions[ i ].GetPlayer() != NULL ) ? true : false;

			if ( haveBotInSeat ) { //mal: if have a bot in this seat, boot him out of the vehicle.				
				botPlayer = positions[ i ].GetPlayer();
				botPosition = &transport->GetPositionManager().PositionForPlayer( botPlayer );
				transport->GetPositionManager().EjectPlayer( *botPosition, true );
				botThreadData.GetGameWorldState()->clientInfo[ botPlayer->entityNumber ].resetState = MAJOR_RESET_EVENT; //mal: let the bot know to reset his AI state
			}

			transport->PlacePlayerInPosition( player, positions[ i ], &positions[ startpos ], true );

			if ( haveBotInSeat ) { //mal: if had a bot, put him back in the vehicle, in the player's old seat.
				transport->PlacePlayerInPosition( botPlayer, positions[ startpos ], NULL, false );
				botThreadData.GetGameWorldState()->clientInfo[ botPlayer->entityNumber ].proxyInfo.clientChangedSeats = true;
			}

			break;
		}

		if( cycle ) {
			i = ( i + 1 ) % positions.Num();
		} else {
			i++;
		}
	}
}

/*
================
sdTransportPositionManager::PositionForId
================
*/
sdVehiclePosition* sdTransportPositionManager::PositionForId( const int positionId ) {
	return &positions[ positionId ];
}

/*
================
sdTransportPositionManager::PositionForId
================
*/
const sdVehiclePosition* sdTransportPositionManager::PositionForId( const int positionId ) const {
	return &positions[ positionId ];
}

/*
================
sdTransportPositionManager::ClearPositions
================
*/
void sdTransportPositionManager::ClearPositions( void ) {
	positions.Clear();
}

/*
================
sdTransportPositionManager::RemovePlayer
================
*/
void sdTransportPositionManager::RemovePlayer( sdVehiclePosition& position ) {
	idPlayer* player = position.GetPlayer();
	if ( !player ) {
		return;
	}

	position.SetPlayer( NULL );	

	gameLocal.localPlayerProperties.ExitingObject( player, transport );

	if ( transport->UnbindOnEject() ) {
		player->ClearIKJoints();
		player->Unbind();
	}

	transport->OnPlayerExited( player, position.GetPositionId() );

	// add the player to the recent players list
	playerExitTime[ player->entityNumber ] = gameLocal.time;
}

/*
================
sdTransportPositionManager::EjectPlayer
================
*/

class sdExitJointDistanceInfo {
public:
	float			distanceSqr;
	jointHandle_t	joint;
	idVec3			origin;
	idMat3			axis;
	
	static int SortByDistance( const sdExitJointDistanceInfo* a, const sdExitJointDistanceInfo* b ) {
		if ( a->distanceSqr > b->distanceSqr ) {
			return 1;
		} else if ( a->distanceSqr < b->distanceSqr ) {
			return -1;
		}
		return 0;
	}
};

bool sdTransportPositionManager::EjectPlayer( sdVehiclePosition& position, bool force ) {
	idPlayer* player = position.GetPlayer();
	if ( !player ) {
		return true;
	}

	//
	// Find a position to eject to
	//
	bool foundOrg = false;
	idVec3 selectedOrg = player->GetPhysics()->GetOrigin();
	idMat3 selectedAxes = player->GetPhysics()->GetAxis();

	if ( transport->UnbindOnEject() ) {
		if( !gameLocal.isClient ) {
			player->DisableClip( false );

			sdTeleporter* teleportEnt = transport->GetTeleportEntity();
			if ( teleportEnt != NULL ) {
				teleportEnt->GetTeleportEndPoint( transport, selectedOrg, selectedAxes );
				selectedOrg.z += 64.f;
				foundOrg = true;
			} else {
				// prioritize exit joints by the nearest 
				idStaticList< sdExitJointDistanceInfo, MAX_EXIT_JOINTS > sortedExitJoints;
				sortedExitJoints.SetNum( exitJoints.Num() );
				idVec3 traceFromPoint;
				transport->GetWorldOrigin( position.GetAttachJoint(), traceFromPoint );
				for( int i = 0; i < exitJoints.Num(); i++ ) {
					sortedExitJoints[ i ].joint = exitJoints[ i ];
					transport->GetWorldOriginAxis( exitJoints[ i ], sortedExitJoints[ i ].origin, sortedExitJoints[ i ].axis );
					sortedExitJoints[ i ].distanceSqr = ( traceFromPoint - sortedExitJoints[ i ].origin ).LengthSqr();
				}

				sortedExitJoints.Sort( sdExitJointDistanceInfo::SortByDistance );

				// choose a point to do the cast-to-exit-point from - if we just use the origin it could
				// potentially be in all sorts of wacky positions depending how the vehicle is built
				// this enures the the point casted from is inside the vehicle
				traceFromPoint = transport->GetPhysics()->GetAxis().TransposeMultiply( traceFromPoint - transport->GetPhysics()->GetOrigin() );
				const idBounds& transportBounds = transport->GetPhysics()->GetBounds();
				traceFromPoint.z = ( transportBounds[ 0 ].z + transportBounds[ 1 ].z ) * 0.5f;
				traceFromPoint = traceFromPoint * transport->GetPhysics()->GetAxis() + transport->GetPhysics()->GetOrigin();

				// default position to get out is inside the vehicle
				selectedOrg = traceFromPoint;
				selectedAxes;

				const idClipModel* playerClip = player->GetPlayerPhysics().GetNormalClipModel();

				for ( int i = 0; i < sortedExitJoints.Num(); i++ ) {
					idVec3 org = sortedExitJoints[ i ].origin;
					idMat3 axes = sortedExitJoints[ i ].axis;

					if ( gameRenderWorld->PointInArea( org ) == -1 ) {
						// outside the map, so no go
						continue;
					}

					// check that the point is clear
					int contents = gameLocal.clip.Contents( CLIP_DEBUG_PARMS org, playerClip, mat3_identity, MASK_PLAYERSOLID, NULL );
					if( !contents ) {
						// check that theres nothing in between the vehicle and the exit point
						trace_t trace;
						if( !gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, traceFromPoint, org, MASK_PLAYERSOLID, transport ) ) {
							selectedOrg = org;
							selectedAxes = axes;
							foundOrg = true;
							break;
						}
					}
				}

				if( !foundOrg ) {
					// Search all 8 positions around every exit joint, should find at least one.
					for ( int i = 0; i < sortedExitJoints.Num(); i++ ) {
						idVec3 orgBase = sortedExitJoints[ i ].origin;
						idMat3 axes = sortedExitJoints[ i ].axis;
						const int size = playerClip->GetBounds().GetSize().x;
						const int spacing = 8;

						for ( int j = -1; j < 2 && !foundOrg; j++ ) {
							for ( int k = -1; k < 2 && !foundOrg; k++ ) {
								if ( j == 0 && k == 0 ) {
									continue;
								}

								idVec3 org = orgBase + idVec3( j * size + j * spacing, k * size + k * spacing, 0.0f );

								if ( gameRenderWorld->PointInArea( org ) == -1 ) {
									// outside the map, so no go
									continue;
								}

								// check that the point is clear
								int contents = gameLocal.clip.Contents( CLIP_DEBUG_PARMS org, playerClip, mat3_identity, MASK_PLAYERSOLID, NULL );
								if( !contents ) {
									// check that theres nothing in between the vehicle and the exit point
									trace_t trace;
									if( !gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, traceFromPoint, org, MASK_PLAYERSOLID, transport ) ) {
										selectedOrg = org;
										selectedAxes = axes;
										foundOrg = true;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if ( !gameLocal.isClient ) {
		if ( !foundOrg ) {
			if ( !force ) {
				return false;
			} else {
				gameLocal.Warning( "sdTransportPositionManager::EjectPlayer No Valid Eject Position Found" );
			}
		}
	}



	//
	// Actually eject
	//

	player->SetSuppressPredictionReset( true );
	RemovePlayer( position );

	if ( transport->UnbindOnEject() ) {
		// copy the velocity over
		idVec3 v = transport->GetPhysics()->GetLinearVelocity();
		for ( int i = 0; i < 3; i++ ) {
			if ( FLOAT_IS_NAN( v[ i ] ) ) {
				v[ i ] = 0.f;
			}
		}
		v.FixDenormals();

		player->GetPhysics()->SetLinearVelocity( v );

		// set the position
		if ( foundOrg ) {
			idAngles temp;
			temp = selectedAxes.ToAngles();
			temp.roll = 0.0f;
			if ( temp.pitch < -10.0f ) {
				temp.pitch = -10.0f;
			}
			player->SetViewAngles( temp );
			player->SetOrigin( selectedOrg );
		}
		player->EnableClip();
	}


	player->SetProxyEntity( NULL, 0 );
	// this forces the reset message to be re-sent
	player->SetSuppressPredictionReset( false );
	player->ResetPredictionErrorDecay();

	return true;
}

/*
================
sdTransportPositionManager::FindDriver
================
*/
idPlayer* sdTransportPositionManager::FindDriver( void ) {
	if ( positions.Num() ) {
		return positions[ 0 ].GetPlayer();
	}
	return NULL;
}

/*
================
sdTransportPositionManager::Think
================
*/
void sdTransportPositionManager::Think( void ) {
	for ( int i = 0; i < positions.Num(); i++ ) {
		positions[ i ].Think();
	}
}

/*
================
sdTransportPositionManager::UpdatePlayerViews
================
*/
void sdTransportPositionManager::UpdatePlayerViews( void ) {
	for ( int i = 0; i < positions.Num(); i++ ) {
		positions[ i ].UpdatePlayerView();
	}
}

/*
================
sdTransportPositionManager::PresentPlayers
================
*/
void sdTransportPositionManager::PresentPlayers( void ) {
	for ( int i = 0; i < positions.Num(); i++ ) {
		positions[ i ].PresentPlayer();
	}
}

/*
================
sdTransportPositionManager::BanPlayer
================
*/
void sdTransportPositionManager::BanPlayer( int clientNum, int length ) {
	int newBanTime = gameLocal.time + length;
	if ( newBanTime > bannedPlayers[ clientNum ] ) {
		bannedPlayers[ clientNum ] = newBanTime;
	}
}

/*
================
sdTransportPositionManager::ResetBan
================
*/
void sdTransportPositionManager::ResetBan( int clientNum ) {
	bannedPlayers[ clientNum ] = 0;
}

/*
================
sdTransportPositionManager::Init
================
*/
void sdTransportPositionManager::Init( const sdDeclVehicleScript* vehicleScript, sdTransport* other ) {
	transport = other;

	idAnimator& animator = *other->GetAnimator();

	if ( !gameLocal.isClient ) {
		EjectAllPlayers();
	}
	ClearPositions();

	playerExitTime.SetNum( MAX_CLIENTS );
	for ( int i = 0; i < playerExitTime.Num(); i++ ) {
		playerExitTime[ i ] = 0;
	}

	for( int i = 0; i < vehicleScript->positions.Num(); i++ ) {
		positionInfo_t& filePosition = vehicleScript->positions[ i ]->positionInfo;

		// FIXME: Gordon: Uh, the position should do this itself innit.

		sdVehiclePosition* positionSlot = positions.Alloc();
		if ( positionSlot == NULL ) {
			gameLocal.Error( "sdTransportPositionManager::Init - Number of positions exceeds MAX_POSITIONS" );
		}
		sdVehiclePosition& position = *positionSlot;


		position.SetPositionId( i );
		position.SetHudName( filePosition.hudname );
		position.SetName( filePosition.name );
		position.SetTransport( other );
		position.LoadData( filePosition.data );

		int j;

		for ( j = 0; j < filePosition.views.Num(); j++ ) {
			position.AddView( filePosition.views[ j ] );
		}

		for( j = 0; j < filePosition.weapons.Num(); j++ ) {
			sdVehicleWeapon* weapon = sdVehicleWeaponFactory::GetWeapon( filePosition.weapons[ j ].weaponType );
			weapon->SetPosition( NULL );
			weapon->SetBasePosition( &position );
			weapon->Setup( other, *filePosition.weapons[ j ].weaponDef, filePosition.weapons[ j ].clampYaw, filePosition.weapons[ j ].clampPitch );
			other->AddWeapon( weapon );
		}

		for( j = 0; j < filePosition.ikSystems.Num(); j++ ) {
			const char* ikSystemTypeName = filePosition.ikSystems[ j ].ikType.c_str();
			idTypeInfo* ikType = idClass::GetClass( ikSystemTypeName );
			if ( !ikType ) {
				gameLocal.Error( "sdTransportPositionManager: Invalid ikType '%s'", ikSystemTypeName );
			}
			if ( !ikType->IsType( sdVehicleIKSystem::Type ) ) {
				gameLocal.Error( "sdTransportPositionManager: ikType '%s' Is Not Of Type sdVehicleIKSystem", ikSystemTypeName );
			}

			sdVehicleIKSystem* ikSystem = reinterpret_cast< sdVehicleIKSystem* >( ikType->CreateInstance() );
			ikSystem->SetPosition( &position );
			ikSystem->Setup( other, filePosition.ikSystems[ j ].clampYaw, filePosition.ikSystems[ j ].clampPitch, filePosition.ikSystems[ j ].ikParms );

			position.AddIKSystem( ikSystem );
		}
	}

	for( int i = 0; i < vehicleScript->exits.Num(); i++ ) {
		jointHandle_t joint = animator.GetJointHandle( vehicleScript->exits[ i ]->exitInfo.joint );
		if( joint == INVALID_JOINT ) {
			gameLocal.Warning( "sdTransportPositionManager::Init Invalid Joint Name '%s' For Vehicle '%s'", vehicleScript->exits[ i ]->exitInfo.joint.c_str(), other->name.c_str() );
			continue;
		}

		jointHandle_t* jointSlot = exitJoints.Alloc();
		if ( jointSlot == NULL ) {
			gameLocal.Error( "sdTransportPositionManager::Init - Number of exit joints exceeds MAX_EXIT_JOINTS" );
		}

		*jointSlot = joint;
	}
}

/*
================
sdTransportPositionManager::Init
================
*/
sdVehiclePosition* sdTransportPositionManager::FindDriverPosition( void ) {
	if ( positions.Num() ) {
		return &positions[ 0 ];
	}

	return NULL;
}

/*
================
sdTransportPositionManager::PositionForPlayer
================
*/
sdVehiclePosition& sdTransportPositionManager::PositionForPlayer( const idPlayer* player ) {
	return positions[ player->GetProxyPositionId() ];
}

/*
================
sdTransportPositionManager::PositionForPlayer
================
*/
const sdVehiclePosition& sdTransportPositionManager::PositionForPlayer( const idPlayer* player ) const {
	return positions[ player->GetProxyPositionId() ];
}

/*
================
sdTransportPositionManager::PositionNameForPlayer
================
*/
const wchar_t* sdTransportPositionManager::PositionNameForPlayer( idPlayer* player ) {
	sdVehiclePosition& position = PositionForPlayer( player );
	return position.GetHudName()->GetText();
}

/*
================
sdTransportPositionManager::IsEmpty
================
*/
bool sdTransportPositionManager::IsEmpty( void ) {
	for( int i = 0; i < positions.Num(); i++ ) {
		if( positions[ i ].GetPlayer() ) {
			return false;
		}
	}
	return true;
}

/*
================
sdTransportPositionManager::HasFreePosition

Just a general check if this vehicle has any open seats at all.
================
*/
bool sdTransportPositionManager::HasFreePosition( void ) {

	bool hasSeat = false;
	int i;

	for ( i = 0; i < positions.Num(); i++ ) {
		if ( positions[ i ].GetPlayer() ) {
			continue;
		}
		
		hasSeat = true;
		break;
	}

	return hasSeat;
}

/*
================
sdTransportPositionManager::OnVehicleScriptReloaded
================
*/
void sdTransportPositionManager::OnVehicleScriptReloaded() {
	for ( int i = 0; i < positions.Num(); i++ ) {
		positions[ i ].OnVehicleScriptReloaded();
	}
	positions.Clear();
	exitJoints.Clear();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		bannedPlayers[ i ] = 0;
	}
}

/*
================
sdVehiclePosition::OnVehicleScriptReloaded
================
*/
void sdVehiclePosition::OnVehicleScriptReloaded() {
	if ( player ) {
		player->SetProxyEntity( NULL, 0 );
	}

	player				= NULL;
	transport			= NULL;
	weaponIndex			= -1;
	index				= -1;
	blockedTip			= NULL;
	maxViewOffset		= 0.f;
	viewOffsetRate		= 0.f;
	currentViewOffset	= 0.f;
	currentViewOffsetAngles = 0.f;
	statTimeSpent		= NULL;
	playerEnteredTime	= 0;
	hudname				= NULL;
	name				= "";
	attachJoint			= INVALID_JOINT;
	attachAnim			= "";
	cockpitName			= "";
	abilities.Clear();
	requirements.Clear();

	for ( int i = 0; i < views.Num(); i++ ) {
		delete views[ i ];
	}
	views.Clear();
	for ( int i = 0; i < ikSystems.Num(); i++ ) {
		delete ikSystems[ i ];
	}
	ikSystems.Clear();
}
