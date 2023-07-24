// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ScriptEntity.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "script/ScriptEntityHelpers.h"
#include "structures/TeamManager.h"
#include "structures/DeployMask.h"
#include "Player.h"
#include "CommandMapInfo.h"
#include "guis/UserInterfaceLocal.h"
#include "guis/GuiSurface.h"

#include "physics/Physics_RigidBody.h"
#include "physics/Physics_SimpleRigidBody.h"
#include "physics/Physics_Simple.h"
#include "vehicles/TransportExtras.h"
#include "vehicles/Pathing.h"

#include "proficiency/StatsTracker.h"
#include "PredictionErrorDecay.h"
#include "misc/WorldToScreen.h"


idBlockAlloc< sdMotorSound, 64 > sdMotorSoundGroup::s_allocator;

/*
================
sdMotorSound::Init
================
*/
void sdMotorSound::Init( gameSoundChannel_t _channel, idEntity* _entity ) {
	channel = _channel;
	entity = _entity;

	value = 0.f;
	inrate = 2.f;
	outrate = 1.f;
	low = -60.f;
	high = 0.f;
	volume = low;
	active = false;
	shader = NULL;
}

/*
================
sdMotorSound::Update
================
*/
void sdMotorSound::Update( float desiredValue ) {
	if ( value < desiredValue ) {
		value += inrate * MS2SEC( gameLocal.msec );
		if ( value > desiredValue ) {
			value = desiredValue;
		}
	} else {
		value -= outrate * MS2SEC( gameLocal.msec );
		if ( value < desiredValue ) {
			value = desiredValue;
		}
	}

	float oldVolume = volume;
	volume = Lerp( low, high, idMath::Sqrt( value ) );

	if ( value == 0.0f ) {
		if ( active ) {
			entity->StopSound( channel );
			active = false;
		}
		return;
	}

	if ( oldVolume == volume ) {
		return;
	}

	if ( !active ) {
		entity->StartSoundShader( shader, channel, channel, 0, NULL );
		active = true;
	}
	entity->SetChannelVolume( channel, volume );
}

/*
================
sdMotorSound::Start
================
*/
void sdMotorSound::Start( const idSoundShader* _shader ) {
	shader = _shader;
}


/*
================
sdMotorSoundGroup::Alloc
================
*/
sdMotorSound* sdMotorSoundGroup::Alloc( void ) {
	gameSoundChannel_t channel = ( gameSoundChannel_t )( SND_MOTOR + sounds.Num() );
	if ( channel > SND_MOTOR_LAST ) {
		gameLocal.Error( "sdMotorSoundGroup::Alloc No Free Motor Sounds" );
	}

	sdMotorSound* sound = s_allocator.Alloc();
	sound->Init( channel, entity );
	sounds.Alloc() = sound;
	return sound;
}

/*
================
sdMotorSoundGroup::~sdMotorSoundGroup
================
*/
sdMotorSoundGroup::~sdMotorSoundGroup( void ) {
	for ( int i = 0; i < sounds.Num(); i++ ) {
		s_allocator.Free( sounds[ i ] );
	}
}



/*
================
sdScriptEntityNetworkData::~sdScriptEntityNetworkData
================
*/
sdScriptEntityNetworkData::~sdScriptEntityNetworkData( void ) {
	delete physicsData;
}

/*
================
sdScriptEntityNetworkData::MakeDefault
================
*/
void sdScriptEntityNetworkData::MakeDefault( void ) {
	scriptData.MakeDefault();

	if ( physicsData ) {
		physicsData->MakeDefault();
	}

	deltaViewAngles.Zero();
}

/*
================
sdScriptEntityNetworkData::Write
================
*/
void sdScriptEntityNetworkData::Write( idFile* file ) const {
	scriptData.Write( file );
	if ( physicsData ) {
		physicsData->Write( file );
	}

	file->WriteAngles( deltaViewAngles );
}

/*
================
sdScriptEntityNetworkData::Read
================
*/
void sdScriptEntityNetworkData::Read( idFile* file ) {
	scriptData.Read( file );
	if ( physicsData ) {
		physicsData->Read( file );
	}

	file->ReadAngles( deltaViewAngles );
}


/*
================
sdScriptEntityBroadcastData::~sdScriptEntityBroadcastData
================
*/
sdScriptEntityBroadcastData::~sdScriptEntityBroadcastData( void ) {
	delete physicsData;
}

/*
================
sdScriptEntityBroadcastData::MakeDefault
================
*/
void sdScriptEntityBroadcastData::MakeDefault( void ) {
	scriptData.MakeDefault();
	
	if ( physicsData ) {
		physicsData->MakeDefault();
	}

	team	= NULL;
	health	= 0;

	bindData.MakeDefault();
	frozen	= false;
}

/*
================
sdScriptEntityBroadcastData::Write
================
*/
void sdScriptEntityBroadcastData::Write( idFile* file ) const {
	scriptData.Write( file );
	if ( physicsData ) {
		physicsData->Write( file );
	}

	file->WriteInt( team ? team->GetIndex() : -1 );
	file->WriteInt( health );
	bindData.Write( file );
	file->WriteBool( frozen );
}

/*
================
sdScriptEntityBroadcastData::Read
================
*/
void sdScriptEntityBroadcastData::Read( idFile* file ) {
	scriptData.Read( file );
	if ( physicsData ) {
		physicsData->Read( file );
	}

	int teamIndex;
	file->ReadInt( teamIndex );
	team = teamIndex == -1 ? NULL : &sdTeamManager::GetInstance().GetTeamByIndex( teamIndex );

	file->ReadInt( health );
	bindData.Read( file );
	file->ReadBool( frozen );
}

/*
===============================================================================

	sdScriptedCrosshairInterface

===============================================================================
*/

/*
================
sdScriptedCrosshairInterface::Init
================
*/
void sdScriptedCrosshairInterface::Init( sdScriptEntity* _owner ) {
	owner		= _owner;
	function	= owner->GetScriptObject()->GetFunction( "OnUpdateCrosshairInfo" );
	targetNode.SetOwner( owner );
	gameLocal.RegisterTargetEntity( targetNode );
}

/*
================
sdScriptedCrosshairInterface::UpdateCrosshairInfo
================
*/
bool sdScriptedCrosshairInterface::UpdateCrosshairInfo( idPlayer* player ) const {
	sdScriptHelper helper;
	helper.Push( player == NULL ? NULL : player->GetScriptObject() );
	return owner->CallFloatNonBlockingScriptEvent( function, helper ) != 0.f;
}

/*
===============================================================================

	sdScriptedGuiInterface

===============================================================================
*/

/*
================
sdScriptedGuiInterface::Init
================
*/
void sdScriptedGuiInterface::Init( sdScriptEntity* _owner ) {
	owner			= _owner;
	function		= owner->GetScriptObject()->GetFunction( "OnUpdateGui" );
	messageFunction = owner->GetScriptObject()->GetFunction( "OnGuiMessage" );
}

/*
================
sdScriptedGuiInterface::WantsToThink
================
*/
bool sdScriptedGuiInterface::WantsToThink( void ) const {
	if ( function == NULL ) {
		return false;
	}

	for ( rvClientEntity* cent = owner->clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
		sdGuiSurface* guiSurface = cent->Cast< sdGuiSurface >();
		if ( guiSurface != NULL ) {
			sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( guiSurface->GetRenderable().GetGuiHandle() );
			if ( ui != NULL && ui->IsActive() ) {
				return true;
			}
		}
	}

	return true;
}

/*
================
sdScriptedGuiInterface::UpdateGui
================
*/
void sdScriptedGuiInterface::UpdateGui( void ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return;
	}

	for ( rvClientEntity* cent = owner->clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
		sdGuiSurface* guiSurface = cent->Cast< sdGuiSurface >();
		if ( guiSurface != NULL ) {
			sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( guiSurface->GetRenderable().GetGuiHandle() );
			if ( ui != NULL ) {
				sdScriptHelper helper;
				helper.Push( player->GetScriptObject() );
				owner->CallNonBlockingScriptEvent( function, helper );
				return;
			}
		}
	}
}

/*
================
sdScriptedGuiInterface::HandleGuiScriptMessage
================
*/
void sdScriptedGuiInterface::HandleGuiScriptMessage( idPlayer* player, const char* message ) {
	gameLocal.SetActionCommand( message );

	sdScriptHelper helper;
	helper.Push( player->GetScriptObject() );
	owner->CallNonBlockingScriptEvent( messageFunction, helper );
}

/*
===============================================================================

	sdScriptedNetworkInterface

===============================================================================
*/

/*
================
sdScriptedNetworkInterface::Init
================
*/
void sdScriptedNetworkInterface::Init( sdScriptEntity* _owner ) {
	owner			= _owner;
	messageFunction = owner->GetScriptObject()->GetFunction( "OnNetworkMessage" );
	eventFunction	= owner->GetScriptObject()->GetFunction( "OnNetworkEvent" );
}

/*
================
sdScriptedNetworkInterface::HandleNetworkMessage
================
*/
void sdScriptedNetworkInterface::HandleNetworkMessage( idPlayer* player, const char* message ) {
	gameLocal.SetActionCommand( message );

	sdScriptHelper helper;
	helper.Push( player->GetScriptObject() );
	owner->CallNonBlockingScriptEvent( messageFunction, helper );
}

/*
================
sdScriptedNetworkInterface::HandleNetworkEvent
================
*/
void sdScriptedNetworkInterface::HandleNetworkEvent( const char* message ) {
	gameLocal.SetActionCommand( message );

	sdScriptHelper helper;
	owner->CallNonBlockingScriptEvent( eventFunction, helper );
}

/*
===============================================================================

	sdScriptedRadarInterface

===============================================================================
*/

/*
================
sdScriptedRadarInterface::AllocJammerLayer
================
*/
int sdScriptedRadarInterface::AllocJammerLayer( sdTeamInfo* team ) {
	if ( !team ) {
		return -1;
	}

	int i;
	for ( i = 0; i < layers.Num(); i++ ) {
		if ( layers[ i ].second == NULL ) {
			break;
		}
	}
	if ( i == layers.Num() ) {
		layers.Alloc();
	}

	sdLayer& layer	= layers[ i ];
	layer.first		= team;
	layer.second	= team->AllocJammerLayer();
	return i;
}

/*
================
sdScriptedRadarInterface::AllocRadarLayer
================
*/
int sdScriptedRadarInterface::AllocRadarLayer( sdTeamInfo* team ) {
	if ( !team ) {
		return -1;
	}

	int i;
	for ( i = 0; i < layers.Num(); i++ ) {
		if ( layers[ i ].second == NULL ) {
			break;
		}
	}
	if ( i == layers.Num() ) {
		layers.Alloc();
	}

	sdLayer& layer	= layers[ i ];
	layer.first		= team;
	layer.second	= team->AllocRadarLayer();
	return i;
}

/*
================
sdScriptedRadarInterface::FreeLayer
================
*/
void sdScriptedRadarInterface::FreeLayer( int index ) {
	if ( index < 0 || index >= layers.Num() ) {
		return;
	}

	if ( !layers[ index ].second ) {
		return;
	}

	layers[ index ].first->FreeRadarLayer( layers[ index ].second );
	layers[ index ].first = NULL;
	layers[ index ].second = NULL;
}

/*
================
sdScriptedRadarInterface::FreeLayers
================
*/
void sdScriptedRadarInterface::FreeLayers( void ) {
	for ( int i = 0; i < layers.Num(); i++ ) {
		if ( !layers[ i ].second ) {
			continue;
		}

		layers[ i ].first->FreeRadarLayer( layers[ i ].second );
		layers[ i ].first = NULL;
		layers[ i ].second = NULL;
	}

	layers.Clear();
}

/*
================
sdScriptedRadarInterface::UpdatePosition
================
*/
void sdScriptedRadarInterface::UpdatePosition( const idVec3& origin, const idMat3& axes ) {
	for ( int i = 0; i < layers.Num(); i++ ) {
		if ( !layers[ i ].second ) {
			continue;
		}

		layers[ i ].second->SetOrigin( origin );
		layers[ i ].second->SetDirection( axes[ 0 ] );
	}
}

/*
===============================================================================

	sdScriptedUsableInterface

===============================================================================
*/

/*
================
sdScriptedUsableInterface::Init
================
*/
void sdScriptedUsableInterface::Init( sdScriptEntity* _owner ) {
	owner = _owner;
	xpShareFactor = 0.f;

	int count = owner->spawnArgs.GetInt( "num_positions" );
	if ( count > 0 ) {
		positions.SetNum( count );

		for ( int i = 0; i < count; i++ ) {
			const char* infoName = owner->spawnArgs.GetString( va( "str_position%i", i ) );
			const sdDeclStringMap* infoMap = gameLocal.declStringMapType[ infoName ];
			if ( !infoMap ) {
				gameLocal.Error( "sdScriptedUsableInterface::Init Invalid Info '%s'", infoName );
			}
			positions[ i ].Init( infoMap->GetDict(), owner );
		}
	}

	overlay				= gameLocal.declGUIType[ owner->spawnArgs.GetString( "gui_usable_overlay" ) ];
	onEnter				= owner->GetScriptObject()->GetFunction( "OnEnter" );
	onExit				= owner->GetScriptObject()->GetFunction( "OnExit" );	
}

/*
================
sdScriptedUsableInterface::PositionForPlayer
================
*/
sdScriptedUsableInterface::sdPosition& sdScriptedUsableInterface::PositionForPlayer( const idPlayer* player ) {
	int id = player->GetProxyPositionId(); 
	
	assert( id >= 0 && id < positions.Num() ); 
	
	return positions[ id ];
}

/*
================
sdScriptedUsableInterface::PositionForPlayer
================
*/
const sdScriptedUsableInterface::sdPosition& sdScriptedUsableInterface::PositionForPlayer( const idPlayer* player ) const {
	int id = player->GetProxyPositionId(); 
	
	assert( id >= 0 && id < positions.Num() ); 
	
	return positions[ id ];
}

/*
============
sdScriptedUsableInterface::ForceExitForAllPlayers
============
*/
void sdScriptedUsableInterface::ForceExitForAllPlayers( void ) {
	idPlayer* p = NULL;
	for ( int i = 0; i < positions.Num(); i++ ) {
		p = positions[ i ].GetPlayer();
		if ( p ) {
			OnExit( p, true );
			p->SetNoClip( false );
			p->SetGodMode( false );
			p->Damage( NULL, NULL, idVec3( 0, 0, 1 ), DAMAGE_FOR_NAME( "damage_generic" ), 999.f, NULL );
		}
	}
}

/*
================
sdScriptedUsableInterface::NumFreePositions
================
*/
int sdScriptedUsableInterface::NumFreePositions( void ) const {
	int count = 0;
	for ( int i = 0; i < positions.Num(); i++ ) {
		if ( !positions[ i ].GetPlayer() ) {
			count++;
		}
	}
	return count;
}

/*
================
sdScriptedUsableInterface::NumFreePositions
================
*/
bool sdScriptedUsableInterface::IsEmpty( void ) const {
	for ( int i = 0; i < positions.Num(); i++ ) {
		if ( positions[ i ].GetPlayer() == NULL ) {
			continue;
		}

		return false;
	}
	return true;
}

	
/*
================
sdScriptedUsableInterface::SetPositionPlayer
================
*/
void sdScriptedUsableInterface::SetPositionPlayer( idPlayer* player, int index ) {
	idMat3 playerAxes = player->GetViewAngles().ToMat3() * owner->GetPhysics()->GetAxis().Transpose();

	originalPos = player->GetPhysics()->GetOrigin();

	positions[ index ].SetPlayer( player );
	jointHandle_t attachJoint = positions[ index ].GetAttachJoint();
	idVec3 org;
	owner->GetWorldOrigin( attachJoint, org );
	player->SetOrigin( org );
	player->BindToJoint( owner, attachJoint, 0 );
	player->SetProxyEntity( owner, index );
	player->SetViewAngles( playerAxes.ToAngles() );

	const char* fsmName = positions[ index ].GetPlayerAnim();
	if ( !*fsmName ) {
		fsmName = "VehicleDefault";
	}
	player->SetAnimState( ANIMCHANNEL_LEGS, va( "Legs_%s", fsmName ), 4 );
	player->SetAnimState( ANIMCHANNEL_TORSO, va( "Torso_%s", fsmName ), 4 );

	owner->OnPlayerEntered( player, index );

	if ( onEnter ) {
		sdScriptHelper helper;
		helper.Push( player->GetScriptObject() );
		owner->CallNonBlockingScriptEvent( onEnter, helper );
	}
}

/*
================
sdScriptedUsableInterface::ForcePlacement
================
*/
void sdScriptedUsableInterface::ForcePlacement( idPlayer* other, int index, int oldIndex, bool keepCamera ) {
	assert( index >= 0 && index < positions.Num() );

	idPlayer* currentPlayer = positions[ index ].GetPlayer();
	if ( currentPlayer ) {
		OnExit( currentPlayer, true );
	}

	SetPositionPlayer( other, index );
}

/*
================
sdScriptedUsableInterface::FindPositionForPlayer
================
*/
void sdScriptedUsableInterface::FindPositionForPlayer( idPlayer* other ) {
	for ( int i = 0; i < positions.Num(); i++ ) {
		if ( positions[ i ].GetPlayer() ) {
			continue;
		}

		SetPositionPlayer( other, i );
		return;
	}
}

/*
============
sdScriptedUsableInterface::GetNumPositions
============
*/
int sdScriptedUsableInterface::GetNumPositions() const {
	return positions.Num();
}

/*
============
sdScriptedUsableInterface::GetPlayerAtPosition
============
*/
idPlayer* sdScriptedUsableInterface::GetPlayerAtPosition( int i ) const {
	return positions[ i ].GetPlayer();
}

/*
============
sdScriptedUsableInterface::GetPositionTitle
============
*/
const sdDeclLocStr* sdScriptedUsableInterface::GetPositionTitle( int i ) const {
	return positions[ i ].GetWeaponName();
}

/*
================
sdScriptedUsableInterface::OnExit
================
*/
bool sdScriptedUsableInterface::OnExit( idPlayer* player, bool force ) {
	sdPosition& position = PositionForPlayer( player );
	if ( !position.GetPlayer() ) {
		// we've already been removed
		return true;
	}

	owner->OnPlayerExited( player, player->GetProxyPositionId() );

	if( onExit ) {
		sdScriptHelper helper;
		helper.Push( player->GetScriptObject() );
		owner->CallNonBlockingScriptEvent( onExit, helper );
	}

	position.SetPlayer( NULL );
	player->ClearIKJoints();
	player->Unbind();
	player->SetProxyEntity( NULL, 0 );
	player->GetPhysics()->SetOrigin( originalPos );
	player->GetPhysics()->SetLinearVelocity( vec3_zero );
	return true;
}

/*
================
sdScriptedUsableInterface::SwapPosition
================
*/
void sdScriptedUsableInterface::SwapPosition( idPlayer* player ) {
}

/*
================
sdScriptedUsableInterface::SwapViewMode
================
*/
void sdScriptedUsableInterface::SwapViewMode( idPlayer* player ) {
}

/*
================
sdScriptedUsableInterface::SelectWeapon
================
*/
void sdScriptedUsableInterface::SelectWeapon( idPlayer* player, int index ) {
}

/*
================
sdScriptedUsableInterface::GetAllowPlayerDamage
================
*/
bool sdScriptedUsableInterface::GetAllowPlayerDamage( idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetAllowPlayerDamage();
}

/*
================
sdScriptedUsableInterface::GetHideHud
================
*/
bool sdScriptedUsableInterface::GetHideHud( idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetHideHud();
}

/*
================
sdScriptedUsableInterface::GetSensitivity
================
*/
bool sdScriptedUsableInterface::GetSensitivity( idPlayer* player, float& sensX, float& sensY ) const {
	return false;
}

/*
================
sdScriptedUsableInterface::GetShowPlayer
================
*/
bool sdScriptedUsableInterface::GetShowPlayer( idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetShowPlayer();
}

/*
================
sdScriptedUsableInterface::GetFov
================
*/
float sdScriptedUsableInterface::GetFov( idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetFov();
}

/*
================
sdScriptedUsableInterface::UpdateHud
================
*/
void sdScriptedUsableInterface::UpdateHud( idPlayer* player, guiHandle_t handle ) {
}

/*
================
sdScriptedUsableInterface::UpdateViewAngles
================
*/
void sdScriptedUsableInterface::UpdateViewAngles( idPlayer* player ) {
	for( int i = 0; i < 3; i++ ) {
		player->viewAngles[ i ] = idMath::AngleNormalize180( player->cmdAngles[ i ] + player->GetDeltaViewAngles()[ i ] );
	}
}

/*
================
sdScriptedUsableInterface::UpdateViewPos
================
*/
void sdScriptedUsableInterface::UpdateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) {
	const sdPosition& position = PositionForPlayer( player );

	const idVec3& viewOrg	= owner->GetPhysics()->GetOrigin();
	const idMat3& viewAxis	= owner->GetPhysics()->GetAxis();

	// origin only takes player yaw into account
	idMat3 temp;
	idAngles::YawToMat3( player->clientViewAngles.yaw, temp );
	origin = viewOrg + temp *  ( viewAxis * position.GetFirstPersonViewOffset() );

	// axis takes full player angles into account
	idMat3 playerViewAxis = player->clientViewAngles.ToMat3();
	axis = playerViewAxis * ( viewAxis * position.GetFirstPersonViewAxis() );
}

/*
================
sdScriptedUsableInterface::GetRequiredViewAngles
================
*/
const idAngles sdScriptedUsableInterface::GetRequiredViewAngles( idPlayer* player, idVec3& target ) {
	const sdPosition& position = PositionForPlayer( player );

	idAngles result( 0.0f, 0.0f, 0.0f );

	const idVec3& viewOrg		= owner->GetPhysics()->GetOrigin();
	const idMat3& viewAxis		= owner->GetPhysics()->GetAxis();
	idAngles viewAngles			= viewAxis.ToAngles();

	const idVec3& fpOffset		= position.GetFirstPersonViewOffset();

	// HACK: use the existing angles to help refine the estimate
	idMat3 temp;
	idAngles::YawToMat3( player->clientViewAngles.yaw, temp );
	idVec3 delta = target - ( viewOrg + temp * ( viewAxis * fpOffset ) );
	idVec3 deltaDirection = delta;
	deltaDirection.Normalize();

	result = deltaDirection.ToAngles();
	result -= viewAngles;
	result.Normalize180();

	return result;
}

/*
================
sdScriptedUsableInterface::CalculateRenderView
================
*/
void sdScriptedUsableInterface::CalculateRenderView( idPlayer* player, renderView_t& renderView ) {
	sdPosition& position = PositionForPlayer( player );

	if ( position.IsThirdperson() ) {
		player->OffsetThirdPersonView( 0, position.GetThirdpersonDistance(), position.GetThirdpersonHeight(), true, renderView );
	} else {
		// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
		// allow the right player view weapons

		renderView.viewID	= player->entityNumber + 1;
		renderView.vieworg	= player->firstPersonViewOrigin;
		renderView.viewaxis = player->firstPersonViewAxis;
	}

	// field of view
	gameLocal.CalcFov( player->CalcFov(), renderView.fov_x, renderView.fov_y );
}

/*
================
sdScriptedUsableInterface::GetWeaponLockInfo
================
*/
const sdWeaponLockInfo* sdScriptedUsableInterface::GetWeaponLockInfo( idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetWeaponLockInfo();
}

/*
================
sdScriptedUsableInterface::GetHideDecoyInfo
================
*/
bool sdScriptedUsableInterface::GetHideDecoyInfo( idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetHideDecoyInfo();
}

/*
================
sdScriptedUsableInterface::GetShowTargetingInfo
================
*/
bool sdScriptedUsableInterface::GetShowTargetingInfo( idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetShowTargetingInfo();
}

/*
================
sdScriptedUsableInterface::GetPlayerStance
================
*/
playerStance_t sdScriptedUsableInterface::GetPlayerStance( idPlayer* player ) const {
	return PS_NORMAL;
}

/*
================
sdScriptedUsableInterface::GetPlayerIconJoint
================
*/
jointHandle_t sdScriptedUsableInterface::GetPlayerIconJoint( idPlayer* player ) const {
	return INVALID_JOINT;
}

/*
================
sdScriptedUsableInterface::ClampViewAngles
================
*/
void sdScriptedUsableInterface::ClampViewAngles( idPlayer* player, const idAngles& oldViewAngles ) const {
	const sdPosition& position = PositionForPlayer( player );

	idAngles inputAngles = oldViewAngles;
	idAngles outputAngles = player->viewAngles;

	const angleClamp_t& yawClamp = position.GetClampYaw();
	sdVehiclePosition::ClampAngle( outputAngles, inputAngles, yawClamp, 1 );

	const angleClamp_t& pitchClamp = position.GetClampPitch();
	sdVehiclePosition::ClampAngle( outputAngles, inputAngles, pitchClamp, 0 );

	player->viewAngles = outputAngles;
}

/*
================
sdScriptedUsableInterface::NextWeapon
================
*/
void sdScriptedUsableInterface::NextWeapon( idPlayer* player ) const {
}

/*
================
sdScriptedUsableInterface::PrevWeapon
================
*/
void sdScriptedUsableInterface::PrevWeapon( idPlayer* player ) const {
}

/*
================
sdScriptedUsableInterface::GetOverlayGUI
================
*/
const sdDeclGUI* sdScriptedUsableInterface::GetOverlayGUI( void ) const {
	return overlay;
}

/*
================
sdScriptedUsableInterface::HasAbility
================
*/
bool sdScriptedUsableInterface::HasAbility( qhandle_t handle, const idPlayer* player ) const {
	return owner->HasAbility( handle );
}

/*
================
sdScriptedUsableInterface::UpdatePlayerViews
================
*/
void sdScriptedUsableInterface::UpdatePlayerViews( void ) const {
	for ( int i = 0; i < positions.Num(); i++ ) {
		idPlayer* player = positions[ i ].GetPlayer();
		if ( !player ) {
			continue;
		}

		player->CalculateView();
	}
}

/*
================
sdScriptedUsableInterface::PresentPlayers
================
*/
void sdScriptedUsableInterface::PresentPlayers( void ) const {
	for ( int i = 0; i < positions.Num(); i++ ) {
		idPlayer* player = positions[ i ].GetPlayer();
		if ( !player ) {
			continue;
		}

		player->UpdateVisuals();
		player->Present();

		idWeapon* weapon = player->GetWeapon();
		if ( weapon ) {
			weapon->PresentWeapon();
		}
	}
}

/*
================
sdScriptedUsableInterface::GetBoundPlayer
================
*/
idPlayer* sdScriptedUsableInterface::GetBoundPlayer( int index ) const {
	if ( index < 0 || index >= positions.Num() ) {
		return NULL;
	}
	return positions[ index ].GetPlayer();
}

/*
================
sdScriptedUsableInterface::GetWeaponName
================
*/
const sdDeclLocStr* sdScriptedUsableInterface::GetWeaponName( const idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetWeaponName();
}

/*
================
sdScriptedUsableInterface::GetWeaponLookupName
================
*/
const char* sdScriptedUsableInterface::GetWeaponLookupName( const idPlayer* player ) const {
	const sdPosition& position = PositionForPlayer( player );
	return position.GetWeaponLookupName();
}

/*
================
sdScriptedUsableInterface::GetXPSharer
================
*/
idPlayer* sdScriptedUsableInterface::GetXPSharer( float& shareFactor ) {
	shareFactor = xpShareFactor;
	return xpSharer;
}

/*
================
sdScriptedUsableInterface::SetXPShareInfo
================
*/
void sdScriptedUsableInterface::SetXPShareInfo( idPlayer* player, float factor ) {
	xpSharer = player;
	xpShareFactor = factor;
}

/*
================
sdScriptedUsableInterface::SetupCockpit
================
*/
void sdScriptedUsableInterface::SetupCockpit( idPlayer* viewPlayer ) {
	const sdPosition& pos = PositionForPlayer( viewPlayer );

	const sdDeclStringMap* cockpitDecl = pos.GetCockpit();
	if ( cockpitDecl != NULL ) {
		gameLocal.playerView.SetupCockpit( cockpitDecl->GetDict(), owner );
	}
}


/*
================
sdScriptedUsableInterface::BecomeActiveViewProxy
================
*/
void sdScriptedUsableInterface::BecomeActiveViewProxy( idPlayer* viewPlayer ) {
	SetupCockpit( viewPlayer );
}

/*
================
sdScriptedUsableInterface::StopActiveViewProxy
================
*/
void sdScriptedUsableInterface::StopActiveViewProxy( void ) {
	gameLocal.playerView.ClearCockpit();
}

/*
================
sdScriptedUsableInterface::UpdateProxyView
================
*/
void sdScriptedUsableInterface::UpdateProxyView( idPlayer* viewPlayer ) {
	sdPosition& position = PositionForPlayer( viewPlayer );

	if ( !position.GetShowCockpit() ) {
		gameLocal.playerView.ClearCockpit();
	} else {
		if ( !gameLocal.playerView.CockpitIsValid() ) {
			SetupCockpit( viewPlayer );
		}
	}
}

/*
===============================================================================

	sdScriptedInteractive

===============================================================================
*/

/*
================
sdScriptedInteractiveInterface::Init
================
*/
void sdScriptedInteractiveInterface::Init( sdScriptEntity* _owner ) {
	owner			= _owner;

	onActivate		= owner->GetScriptObject()->GetFunction( "OnActivate" );
	onActivateHeld	= owner->GetScriptObject()->GetFunction( "OnActivateHeld" );
	onUsed			= owner->GetScriptObject()->GetFunction( "OnUsed" );
}

/*
================
sdScriptedInteractiveInterface::OnActivate
================
*/
bool sdScriptedInteractiveInterface::OnActivate( idPlayer* player, float distance ) {
	if ( !onActivate ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	owner->GetScriptObject()->CallNonBlockingScriptEvent( onActivate, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
================
sdScriptedInteractiveInterface::OnActivateHeld
================
*/
bool sdScriptedInteractiveInterface::OnActivateHeld( idPlayer* player, float distance ) {
	if ( !onActivateHeld ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	owner->GetScriptObject()->CallNonBlockingScriptEvent( onActivateHeld, h1 );

	return !idMath::FloatIsZero( gameLocal.program->GetReturnedFloat() );
}

/*
================
sdScriptedInteractiveInterface::OnUsed
================
*/
bool sdScriptedInteractiveInterface::OnUsed( idPlayer* player, float distance ) {
	if ( !onUsed ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	owner->GetScriptObject()->CallNonBlockingScriptEvent( onUsed, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
===============================================================================

	sdScriptedUsableInterface::sdPosition

===============================================================================
*/

/*
================
sdPosition::sdPosition
================
*/
sdScriptedUsableInterface::sdPosition::sdPosition( void ) {
	flags.thirdperson = false;
}

/*
================
sdPosition::Init
================
*/
void sdScriptedUsableInterface::sdPosition::Init( const idDict& info, sdScriptEntity* owner ) {
	flags.thirdperson			= info.GetBool( "thirdperson", "0" );
	flags.showPlayer			= info.GetBool( "show_player", "0" );
	flags.hideHud				= info.GetBool( "hide_hud", "0" );
	flags.allowMovement			= info.GetBool( "allow_movement", "0" );
	flags.takesDamage			= info.GetBool( "take_damage", "0" );
	flags.hideDecoyInfo			= info.GetBool( "hide_decoy_info", "0" );
	flags.showTargetingInfo		= info.GetBool( "show_targeting_info", "0" );

	if ( info.GetBool( "show_cockpit", "0" ) ) {
		const char* cockpitInfoString = info.GetString( "str_cockpit_info" );
		cockpitInfo				= gameLocal.declStringMapType[ cockpitInfoString ];
		if ( cockpitInfo == NULL ) {
			gameLocal.Warning( "sdScriptedUsableInterface::sdPosition::Init Invalid Cockpit Info '%s'", cockpitInfoString );
		}
	} else {
		cockpitInfo				= NULL;
	}

	thirdpersonHeight			= info.GetFloat( "thirdperson_height", "64" );
	thirdpersonDistance			= info.GetFloat( "thirdperson_distance", "256" );

	firstPersonCameraOffset.Zero();
	firstPersonCameraAxis.Identity();

	const char* jointName		= info.GetString( "joint_view" );
	if ( *jointName ) {
		jointHandle_t cameraJoint = owner->GetAnimator()->GetJointHandle( jointName );
		if ( cameraJoint == INVALID_JOINT ) {
			gameLocal.Warning( "sdScriptedUsableInterface::sdPosition::Init Invalid Camera Joint '%s'", jointName );
		} else {
			owner->GetAnimator()->GetJointTransform( cameraJoint, gameLocal.time, firstPersonCameraOffset, firstPersonCameraAxis );

			// HACK: This makes the maths to figure out the required view angles much easier
//			firstPersonCameraOffset.x = firstPersonCameraOffset.y = 0.0f;
		}
	}

	playerAnim					= info.GetString( "player_anim", "VehicleDefault" );

	fov							= info.GetFloat( "fov", "90" );

	clampYaw.flags.enabled		= info.GetVec2( "clamp_yaw", "0 0", clampYaw.extents );
	clampPitch.flags.enabled	= info.GetVec2( "clamp_pitch", "0 0", clampPitch.extents );
	clampYaw.flags.limitRate	= info.GetFloat( "clamp_yaw_rate", "0", clampYaw.rate[ 0 ] );
	clampPitch.flags.limitRate	= info.GetFloat( "clamp_pitch_rate", "0", clampPitch.rate[ 0 ] );

	attachJoint					= owner->GetAnimator()->GetJointHandle( info.GetString( "attach_joint" ) );

	weaponLookupName			= info.GetString( "weapon_name" );
	weaponName					= declHolder.FindLocStr( info.GetString( "gunName" ) );

	lockInfo.Load( info );
}

/*
================
sdPosition::GetPlayer
================
*/
idPlayer* sdScriptedUsableInterface::sdPosition::GetPlayer( void ) const {
	return boundPlayer;
}

/*
================
sdPosition::SetPlayer
================
*/
void sdScriptedUsableInterface::sdPosition::SetPlayer( idPlayer* player ) {
	boundPlayer = player;
}

/*
===============================================================================

	sdScriptEntity

===============================================================================
*/

extern const idEventDef EV_GetTeamDamageDone;
extern const idEventDef EV_SetTeamDamageDone;
extern const idEventDef EV_Freeze;
extern const idEventDef EV_SetState;

const idEventDef EV_ScriptEntity_AddHelper( "addHelper", '\0', DOC_TEXT( "Allocates a helper object to control IK on the entity." ), 1, NULL, "d", "index", "Index of the $decl:stringMap$ which contains data for the helper." );
const idEventDef EV_ScriptEntity_GetBoundPlayer( "getBoundPlayer", 'e', DOC_TEXT( "Returns the player at the specified proxy position, or $null$ if none." ), 1, "If the entity does not support being a proxy, the result will be $null$.", "d", "index", "Index of the proxy position to return." );
const idEventDef EV_ScriptEntity_RemoveBoundPlayer( "removeBoundPlayer", '\0', DOC_TEXT( "Removes the player from this proxy." ), 1, "If the specified player is not in this proxy, the behaviour is undefined.", "e", "player", "Player to remove." );

const idEventDef EV_ScriptEntity_RadarFreeLayers( "freeLayers", '\0', DOC_TEXT( "Removes all allocated radar layers for this entity." ), 0, NULL );
const idEventDef EV_ScriptEntity_AllocRadarLayer( "allocRadarLayer", 'd', DOC_TEXT( "Allocates a radar layer and returns a handle to it." ), 0, NULL );
const idEventDef EV_ScriptEntity_AllocJammerLayer( "allocJammerLayer", 'd', DOC_TEXT( "Allocates a jammer layer and returns a handle to it." ), 0, NULL );

const idEventDef EV_ScriptEntity_RadarSetLayerRange( "radarSetLayerRange", '\0', DOC_TEXT( "Sets the range of a given radar layer." ), 2, NULL, "d", "index", "Index of the layer to modify.", "f", "range", "Range to set." );
const idEventDef EV_ScriptEntity_RadarSetLayerMaxAngle( "radarSetLayerMaxAngle", '\0', DOC_TEXT( "Sets the size of the arc of a given radar layer." ), 2, NULL, "d", "index", "Index of the layer to modify.", "f", "angle", "Size of the arc in degrees." );
const idEventDef EV_ScriptEntity_RadarSetLayerMask( "radarSetLayerMask", '\0', DOC_TEXT( "Sets the maks of a given radar layer." ), 2, NULL, "d", "index", "Index of the layer to modify.", "d", "mask", "Mask to set." );

const idEventDef EV_ScriptEntity_SetRemoteViewAngles( "setRemoteViewAngles", '\0', DOC_TEXT( "Sets the angles to be used when using this entity as a remote viewing device." ), 2, NULL, "v", "angles", "Angles to set.", "e", "player", "Player using this as a remote viewing device." );
const idEventDef EV_ScriptEntity_GetRemoteViewAngles( "getRemoteViewAngles", 'v', DOC_TEXT( "Returns the view angles of this entity when it is used as a remote viewing device." ), 1, NULL, "e", "player", "Player using this as a remote viewing device." );

const idEventDef EV_ScriptEntity_PathFind( "pathFind", "svfffffb" );
const idEventDef EV_ScriptEntity_PathFindVampire( "pathFindVampire", "vvf", 'f' );
const idEventDef EV_ScriptEntity_PathLevel( "pathLevel", "fdd" );
const idEventDef EV_ScriptEntity_PathStraighten( "pathStraighten" );
const idEventDef EV_ScriptEntity_PathGetNumPoints( "pathGetNumPoints", NULL, 'f' );
const idEventDef EV_ScriptEntity_PathGetPoint( "pathGetPoint", "d", 'v' );
const idEventDef EV_ScriptEntity_PathGetLength( "pathGetLength", NULL, 'f' );
const idEventDef EV_ScriptEntity_PathGetPosition( "pathGetPosition", "f", 'v' );
const idEventDef EV_ScriptEntity_PathGetDirection( "pathGetDirection", "f", 'v' );
const idEventDef EV_ScriptEntity_PathGetAngles( "pathGetAngles", "f", 'v' );
const idEventDef EV_ScriptEntity_GetVampireBombPosition( "getVampireBombPosition", "vffff", 'f' );
const idEventDef EV_ScriptEntity_GetVampireBombAcceleration( "getVampireBombAcceleration", NULL, 'v' );
const idEventDef EV_ScriptEntity_GetVampireBombFallTime( "getVampireBombFallTime", NULL, 'f' );

const idEventDef EV_ScriptEntity_SetGroundPosition( "setGroundPosition", '\0', DOC_TEXT( "Sets the ground height to land on based on the position and the current gravity normal." ), 1, "This is only supported by physics of type $class:sdPhysics_Simple$.", "v", "position", "Point on the ground plane." );

const idEventDef EV_ScriptEntity_SetXPShareInfo( "setXPShareInfo", '\0', DOC_TEXT( "Sets up a player to gain XP whenever anyone gains XP whilst using this entity as a proxy." ), 2, NULL, "E", "player", "Player that is to share the XP.", "f", "scale", "Scale factor to apply to the original XP gain." );
const idEventDef EV_ScriptEntity_SetBoxClipModel( "setBoxClipModel", '\0', DOC_TEXT( "Changes the clip model for the entity to the given bounds, using the specified mass." ), 3, NULL, "v", "mins", "Mins of the bounds to set.", "v", "maxs", "Maxs of the bounds to set.", "f", "mass", "Mass of the new clipmodel." );

const idEventDef EV_ScriptEntity_ForceAnimUpdate( "forceAnimUpdate", '\0', DOC_TEXT( "Enables/disables forcing of animation updates, regardless of AoR state." ), 1, "This uses a ref counting system, so every call to this with true should be matched with another call with false.", "b", "state", "Whether to increase, or decrease the counter." );

const idEventDef EV_ScriptEntity_SetIKTarget( "setIKTarget", '\0', DOC_TEXT( "Sets which player a specified IK helper should apply to." ), 2, "If player is non-null, it must be a player, or an error will be thrown.", "E", "player", "Player the IK should control.", "d", "key", "Key to match against the IK helper." );

const idEventDef EV_ScriptEntity_HideInLocalView( "hideInLocalView", '\0', DOC_TEXT( "Makes the entity invisible in first person view only." ), 0, "See also $event:showInLocalView$." );
const idEventDef EV_ScriptEntity_ShowInLocalView( "showInLocalView", '\0', DOC_TEXT( "Makes the entity visible in first person view." ), 0, "See also $event:hideInLocalView$." );

const idEventDef EV_ScriptEntity_SetClipOriented( "setClipOriented", '\0', DOC_TEXT( "Sets whether the clipmodel should be oriented, or axis aligned." ), 1, "This only applies to physics of type $class:sdPhysics_SimpleRigidBody$.", "b", "state", "Whether it should be oriented or not." );

const idEventDef EV_ScriptEntity_SetIconMaterial( "setIconMaterial", "s" );
const idEventDef EV_ScriptEntity_SetIconSize( "setIconSize", "ff" );
const idEventDef EV_ScriptEntity_SetIconColorMode( "setIconColorMode", "d" );
const idEventDef EV_ScriptEntity_SetIconPosition( "setIconPosition", "d" );
const idEventDef EV_ScriptEntity_SetIconEnabled( "setIconEnabled", "b" );
const idEventDef EV_ScriptEntity_SetIconCutoff( "setIconCutoff", "f" );
const idEventDef EV_ScriptEntity_SetIconAlphaScale( "setIconAlphaScale", "f" );

const idEventDef EV_ScriptEntity_EnableCollisionPush( "enableCollisionPush" );
const idEventDef EV_ScriptEntity_DisableCollisionPush( "disableCollisionPush" );


CLASS_DECLARATION( idAnimatedEntity, sdScriptEntity )
	EVENT( EV_SetState,										sdScriptEntity::Event_SetState )
	EVENT( EV_ScriptEntity_AddHelper,						sdScriptEntity::Event_AddHelper )
	EVENT( EV_ScriptEntity_GetBoundPlayer,					sdScriptEntity::Event_GetBoundPlayer )
	EVENT( EV_ScriptEntity_RemoveBoundPlayer,				sdScriptEntity::Event_RemoveBoundPlayer )

	EVENT( EV_ScriptEntity_RadarFreeLayers,					sdScriptEntity::Event_RadarFreeLayers )
	EVENT( EV_ScriptEntity_AllocRadarLayer,					sdScriptEntity::Event_AllocRadarLayer )
	EVENT( EV_ScriptEntity_AllocJammerLayer,				sdScriptEntity::Event_AllocJammerLayer )
	EVENT( EV_ScriptEntity_RadarSetLayerRange,				sdScriptEntity::Event_RadarSetLayerRange )
	EVENT( EV_ScriptEntity_RadarSetLayerMaxAngle,			sdScriptEntity::Event_RadarSetMaxAngle )
	EVENT( EV_ScriptEntity_RadarSetLayerMask,				sdScriptEntity::Event_RadarSetMask )
	EVENT( EV_Freeze,										sdScriptEntity::Event_Freeze )

	EVENT( EV_ScriptEntity_SetRemoteViewAngles,				sdScriptEntity::Event_SetRemoteViewAngles )
	EVENT( EV_ScriptEntity_GetRemoteViewAngles,				sdScriptEntity::Event_GetRemoteViewAngles )

	EVENT( EV_ScriptEntity_PathFind,						sdScriptEntity::Event_PathFind )
	EVENT( EV_ScriptEntity_PathFindVampire,					sdScriptEntity::Event_PathFindVampire )
	EVENT( EV_ScriptEntity_PathLevel,						sdScriptEntity::Event_PathLevel )
	EVENT( EV_ScriptEntity_PathStraighten,					sdScriptEntity::Event_PathStraighten )
	EVENT( EV_ScriptEntity_PathGetNumPoints,				sdScriptEntity::Event_PathGetNumPoints )
	EVENT( EV_ScriptEntity_PathGetPoint,					sdScriptEntity::Event_PathGetPoint )
	EVENT( EV_ScriptEntity_PathGetLength,					sdScriptEntity::Event_PathGetLength )
	EVENT( EV_ScriptEntity_PathGetPosition,					sdScriptEntity::Event_PathGetPosition )
	EVENT( EV_ScriptEntity_PathGetDirection,				sdScriptEntity::Event_PathGetDirection )
	EVENT( EV_ScriptEntity_PathGetAngles,					sdScriptEntity::Event_PathGetAngles )
	EVENT( EV_ScriptEntity_GetVampireBombPosition,			sdScriptEntity::Event_GetVampireBombPosition )
	EVENT( EV_ScriptEntity_GetVampireBombAcceleration,		sdScriptEntity::Event_GetVampireBombAcceleration )
	EVENT( EV_ScriptEntity_GetVampireBombFallTime,			sdScriptEntity::Event_GetVampireBombFallTime )

	EVENT( EV_ScriptEntity_SetGroundPosition,				sdScriptEntity::Event_SetGroundPosition )

	EVENT( EV_ScriptEntity_SetXPShareInfo,					sdScriptEntity::Event_SetXPShareInfo )
	EVENT( EV_ScriptEntity_SetBoxClipModel,					sdScriptEntity::Event_SetBoxClipModel )

	EVENT( EV_GetTeamDamageDone,							sdScriptEntity::Event_GetTeamDamageDone )
	EVENT( EV_SetTeamDamageDone,							sdScriptEntity::Event_SetTeamDamageDone )

	EVENT( EV_ScriptEntity_ForceAnimUpdate,					sdScriptEntity::Event_ForceAnimUpdate )
	EVENT( EV_ScriptEntity_SetIKTarget,						sdScriptEntity::Event_SetIKTarget )

	EVENT( EV_ScriptEntity_HideInLocalView,					sdScriptEntity::Event_HideInLocalView )
	EVENT( EV_ScriptEntity_ShowInLocalView,					sdScriptEntity::Event_ShowInLocalView )

	EVENT( EV_ScriptEntity_SetClipOriented,					sdScriptEntity::Event_SetClipOriented )

	EVENT( EV_ScriptEntity_SetIconMaterial,					sdScriptEntity::Event_SetIconMaterial )
	EVENT( EV_ScriptEntity_SetIconSize,						sdScriptEntity::Event_SetIconSize )
	EVENT( EV_ScriptEntity_SetIconColorMode,				sdScriptEntity::Event_SetIconColorMode )
	EVENT( EV_ScriptEntity_SetIconPosition,					sdScriptEntity::Event_SetIconPosition )
	EVENT( EV_ScriptEntity_SetIconEnabled,					sdScriptEntity::Event_SetIconEnabled )
	EVENT( EV_ScriptEntity_SetIconCutoff,					sdScriptEntity::Event_SetIconCutoff )
	EVENT( EV_ScriptEntity_SetIconAlphaScale,				sdScriptEntity::Event_SetIconAlphaScale )

	EVENT( EV_ScriptEntity_EnableCollisionPush,				sdScriptEntity::Event_EnableCollisionPush )
	EVENT( EV_ScriptEntity_DisableCollisionPush,			sdScriptEntity::Event_DisableCollisionPush )
END_CLASS

/*
================
sdScriptEntity::sdScriptEntity
================
*/
sdScriptEntity::sdScriptEntity( void ) {
	baseScriptThread		= NULL;
	scriptIdealState		= NULL;
	scriptState				= NULL;
	team					= NULL;

	guiInterface			= NULL;
	crosshairInterface		= NULL;
	networkInterface		= NULL;
	taskInterface			= NULL;
	usableInterface			= NULL;
	interactiveInterface	= NULL;

	touchFunc				= NULL;

	physicsObj				= NULL;

	preDamageFunc			= NULL;
	postDamageFunc			= NULL;
	onHitFunc				= NULL;
	onCollideFunc			= NULL;

	isDeployedFunc			= NULL;
	isDisabledFunc			= NULL;	
	getOwnerFunc			= NULL;
	deployableType			= NULL_DEPLOYABLE;

	deployObject			= NULL;
	radiusPushScale			= 1.0f;

	waterEffects			= NULL;

	forcedAnimCounter		= 0;

	predictionErrorDecay_Origin	= NULL;
	predictionErrorDecay_Angles	= NULL;

	teamDamageDone			= 0;

	postThinkEntNode.SetOwner( this );
	iconNode.SetOwner( this );

	motorSounds.Init( this );
}

/*
================
sdScriptEntity::~sdScriptEntity
================
*/
sdScriptEntity::~sdScriptEntity( void ) {
	if ( baseScriptThread != NULL ) {
		gameLocal.program->FreeThread( baseScriptThread );
		baseScriptThread = NULL;
	}
	DeconstructScriptObject();

	// ensure any players using the interface are "ejected"
	if ( usableInterface ) {
		usableInterface->ForceExitForAllPlayers();
	}

	delete guiInterface;
	delete crosshairInterface;
	delete networkInterface;
	delete taskInterface;
	delete usableInterface;
	delete interactiveInterface;
	delete predictionErrorDecay_Origin;
	delete predictionErrorDecay_Angles;
	delete physicsObj;
	delete waterEffects;

	sdScriptedEntityHelper* helper;
	for ( helper = helpers.Next(); helper; ) {
		sdScriptedEntityHelper* next = helper->GetNode().Next();
		delete helper;
		helper = next;
	}
}

/*
================
sdScriptEntity::Spawn
================
*/
void sdScriptEntity::Spawn( void ) {
	maxHealth = health = spawnArgs.GetInt( "health" );

	if ( spawnArgs.GetBool( "option_task_interface" ) ) {
		taskInterface = new sdTaskInterface( this );
	}

	if ( spawnArgs.GetBool( "option_rigid_body_physics" ) ) {
		idVec3 gravityNormal = gameLocal.GetGravity();
		gravityNormal.Normalize();

		idPhysics_RigidBody* rbPhysics = new idPhysics_RigidBody();
		physicsObj = rbPhysics;
		physicsObj->SetSelf( this );
		physicsObj->SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
		physicsObj->SetGravity( spawnArgs.GetFloat( "gravity", DEFAULT_GRAVITY_STRING ) * gravityNormal );
		physicsObj->SetMass( spawnArgs.GetFloat( "mass", "100" ) );
		physicsObj->SetLinearVelocity( spawnArgs.GetVector( "velocity", "0 0 0" ) );
		physicsObj->SetAngularVelocity( spawnArgs.GetVector( "angular_velocity", "0 0 0" ) );
		rbPhysics->SetBuoyancy( spawnArgs.GetFloat( "bouyancy" ) );
		rbPhysics->SetBouncyness( spawnArgs.GetFloat( "bouncyness", "0.6" ) );
		rbPhysics->SetFriction( spawnArgs.GetFloat( "linear_friction" ), spawnArgs.GetFloat( "angular_friction" ), spawnArgs.GetFloat( "contact_friction" ) );
		physicsObj->SetOrigin( GetPhysics()->GetOrigin() );
		physicsObj->SetAxis( GetPhysics()->GetAxis() );
		rbPhysics->SetApplyImpulse( spawnArgs.GetBool( "apply_collision_impulse", "1" ) );

		this->SetPhysics( physicsObj );
	}	

	if ( spawnArgs.GetBool( "option_simple_rigid_body_physics" ) ) {
		idVec3 gravityNormal = gameLocal.GetGravity();
		gravityNormal.Normalize();

		sdPhysics_SimpleRigidBody* rbPhysics = new sdPhysics_SimpleRigidBody();
		physicsObj = rbPhysics;
		physicsObj->SetSelf( this );
		physicsObj->SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
		physicsObj->SetGravity( spawnArgs.GetFloat( "gravity", DEFAULT_GRAVITY_STRING ) * gravityNormal );
		physicsObj->SetMass( spawnArgs.GetFloat( "mass", "100" ) );
		physicsObj->SetLinearVelocity( spawnArgs.GetVector( "velocity", "0 0 0" ) );
		physicsObj->SetAngularVelocity( spawnArgs.GetVector( "angular_velocity", "0 0 0" ) );
		rbPhysics->SetBuoyancy( spawnArgs.GetFloat( "bouyancy" ) );
		rbPhysics->SetBouncyness( spawnArgs.GetFloat( "bouncyness", "0.6" ) );
		rbPhysics->SetFriction( spawnArgs.GetFloat( "linear_friction" ), spawnArgs.GetFloat( "angular_friction" ), spawnArgs.GetFloat( "contact_friction" ) );
		physicsObj->SetOrigin( GetPhysics()->GetOrigin() );
		physicsObj->SetAxis( GetPhysics()->GetAxis() );

		this->SetPhysics( physicsObj );
	}	

	if ( spawnArgs.GetBool( "option_simple_physics" ) ) {
		BecomeActive( TH_PHYSICS );

		idVec3 gravityNormal = gameLocal.GetGravity();
		gravityNormal.Normalize();

		physicsObj = new sdPhysics_Simple();
		physicsObj->SetSelf( this );
		physicsObj->SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
		physicsObj->SetGravity( spawnArgs.GetFloat( "gravity", DEFAULT_GRAVITY_STRING ) * gravityNormal );
		physicsObj->SetMass( spawnArgs.GetFloat( "mass", "100" ) );
		physicsObj->SetLinearVelocity( spawnArgs.GetVector( "velocity", "0 0 0" ) );
		physicsObj->SetAngularVelocity( spawnArgs.GetVector( "angular_velocity", "0 0 0" ) );
		physicsObj->SetOrigin( GetPhysics()->GetOrigin() );
		physicsObj->SetAxis( GetPhysics()->GetAxis() );

		this->SetPhysics( physicsObj );
	}

	scriptObject = gameLocal.program->AllocScriptObject( this, spawnArgs.GetString( "scriptobject", "default" ) );

	preDamageFunc	= scriptObject->GetFunction( "OnPreDamage" );
	postDamageFunc	= scriptObject->GetFunction( "OnPostDamage" );
	onHitFunc		= scriptObject->GetFunction( "OnHit" );
	onCollideFunc	= scriptObject->GetFunction( "OnCollide" );
	needsRepairFunc = scriptObject->GetFunction( "NeedsRepair" );
	overrideFunc	= scriptObject->GetFunction( "OverridePreventDeployment" );
	onPostThink		= scriptObject->GetFunction( "OnPostThink" );

	if ( onPostThink != NULL ) {
		postThinkEntNode.AddToEnd( gameLocal.postThinkEntities );
	}

	killStatSuffix	= spawnArgs.GetString( "stat_kill_suffix", "" );

	if ( !spawnArgs.GetVector( "selection_mins", "0 0 0", selectionBounds[0] ) || !spawnArgs.GetVector( "selection_maxs", "0 0 0", selectionBounds[1] ) ) {
		selectionBounds.Clear();
	}

	if ( spawnArgs.GetBool( "option_gui_interface" ) ) {
		guiInterface = new sdScriptedGuiInterface();
		guiInterface->Init( this );
	}
	if ( spawnArgs.GetBool( "option_crosshair_interface" ) ) {
		crosshairInterface = new sdScriptedCrosshairInterface();
		crosshairInterface->Init( this );
	}
	if ( spawnArgs.GetBool( "option_network_interface" ) ) {
		networkInterface = new sdScriptedNetworkInterface();
		networkInterface->Init( this );
	}
	if ( spawnArgs.GetBool( "option_usable_interface" ) ) {
		usableInterface = new sdScriptedUsableInterface();
		usableInterface->Init( this );
	}
	if ( spawnArgs.GetBool( "option_interactive_interface" ) ) {
		interactiveInterface = new sdScriptedInteractiveInterface();
		interactiveInterface->Init( this );
	}

	if ( spawnArgs.GetBool( "option_prediction_error_decay" ) ) {
		predictionErrorDecay_Origin = new sdPredictionErrorDecay_Origin();
		predictionErrorDecay_Angles = new sdPredictionErrorDecay_Angles();
		predictionErrorDecay_Origin->Init( this );
		predictionErrorDecay_Angles->Init( this );
	}

	displayIconInterface.Init( this );
	if ( spawnArgs.GetBool( "option_icon_interface" ) ) {
		gameLocal.RegisterIconEntity( iconNode );
	}

	scriptEntityFlags.noInhibitPhysics = spawnArgs.GetBool( "option_no_inhibit_physics" );

	scriptEntityFlags.allowAbilities = spawnArgs.GetBool( "option_allow_abilities" );
	fl.unlockInterpolate = spawnArgs.GetBool( "option_unlock_interpolate" );

	if ( scriptEntityFlags.allowAbilities ) {
		const idKeyValue* kv = NULL;
		while ( ( kv = spawnArgs.MatchPrefix( "ability_", kv ) ) != NULL ) {
			abilities.Add( kv->GetValue().c_str() );
		}
	}

	waterEffects = sdWaterEffects::SetupFromSpawnArgs( spawnArgs );

	BecomeActive( TH_THINK );

	if ( health > 0 ) {
		fl.takedamage = true;
	}

	if ( spawnArgs.GetBool( "option_combat_model" ) ) {
		SetCombatModel();
	}
	if ( spawnArgs.GetBool( "option_selection_combat_model" ) ) {
		SetSelectionCombatModel();
	}

	baseScriptThread = ConstructScriptObject();
	if ( baseScriptThread != NULL ) {
		BecomeActive( TH_RUNSCRIPT );
	}

	deployObject = gameLocal.declDeployableObjectType[ spawnArgs.GetString( "do_object" ) ];

	if ( deployObject != NULL ) {
		isDeployedFunc = scriptObject->GetFunction( "IsDeployed" );
		isDisabledFunc = scriptObject->GetFunction( "IsDisabled" );
		getOwnerFunc = scriptObject->GetFunction( "GetOwner" );

		int temp;
		if ( spawnArgs.GetInt( "deployable_type", "", temp ) ) {
			deployableType = temp;
		}

		float temp2;
		if ( spawnArgs.GetFloat( "range_max", "", temp2 ) ) {
			deployableRange = MetresToInches( temp2 );
		}
	}

	bool wantsTouch					= spawnArgs.GetBool( "option_wants_touch" );
	if ( wantsTouch ) {
		touchFunc = scriptObject->GetFunction( "OnTouch" );
	}
	scriptEntityFlags.writeBind			= spawnArgs.GetBool( "option_write_bind" );
	scriptEntityFlags.writeViewAngles	= spawnArgs.GetBool( "option_write_viewangles" );
	scriptEntityFlags.frozen			= false;
	scriptEntityFlags.inhibitDecals		= spawnArgs.GetBool( "option_inhibit_decals" );
	scriptEntityFlags.noCollisionPush	= spawnArgs.GetBool( "option_no_collision_push" );
	scriptEntityFlags.hasNewBoxClip		= false;

	deltaViewAngles.Zero();

	lastPathPosition = 0.0f;
	lastPathLength = 0.0f;
	lastPathPoint = 0;

	radiusPushScale = spawnArgs.GetFloat( "push_scale", "1" );

	scriptEntityFlags.takesOobDamage	= spawnArgs.GetBool( "option_take_oob_damage", "0" );
	oobDamageInterval					= spawnArgs.GetInt( "oob_damage_interval", "500" );
	lastTimeInPlayZone					= -1;

	if ( gameLocal.mapSkinPool != NULL ) {
		const char* skinKey = spawnArgs.GetString( "climate_skin_key" );
		if ( *skinKey != '\0' ) {
			const char* skinName = gameLocal.mapSkinPool->GetDict().GetString( va( "skin_%s", skinKey ) );
			if ( *skinName == '\0' ) {
				gameLocal.Warning( "sdScriptEntity::Spawn No Skin Set For '%s'", skinKey );
			} else {
				const idDeclSkin* skin = gameLocal.declSkinType[ skinName ];
				if ( skin == NULL ) {
					gameLocal.Warning( "sdScriptEntity::Spawn Skin '%s' Not Found", skinName );
				} else {
					SetSkin( skin );
				}
			}
		}
	}
}

/*
================
sdScriptEntity::ConstructScriptObject
================
*/
sdProgramThread* sdScriptEntity::ConstructScriptObject( void ) {
	if ( scriptObject == NULL ) {
		return NULL;
	}

	scriptObject->ClearObject();

	sdScriptHelper h1;
	CallNonBlockingScriptEvent( scriptObject->GetSyncFunc(), h1 );

	sdScriptHelper h2;
	CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h2 );

	const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
	return constructor ? CreateScriptThread( constructor ) : NULL;
}

/*
================
sdScriptEntity::PostMapSpawn
================
*/
void sdScriptEntity::PostMapSpawn( void ) {
	idEntity::PostMapSpawn();

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnPostMapSpawn" ), h1 );
}

/*
================
sdScriptEntity::Damage
================
*/
void sdScriptEntity::Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const sdDeclDamage* damage, const float damageScale, const trace_t* collision, bool forceKill ) {
	if ( preDamageFunc ) {
		sdScriptHelper helper;
		helper.Push( inflictor ? inflictor->GetScriptObject() : NULL );
		helper.Push( attacker ? attacker->GetScriptObject() : NULL );
		if ( !CallBooleanNonBlockingScriptEvent( preDamageFunc, helper ) ) {
			return;
		}
	}

	int oldHealth = health;

	idAnimatedEntity::Damage( inflictor, attacker, dir, damage, damageScale, collision, forceKill );

	if ( postDamageFunc && oldHealth != health ) {
		sdScriptHelper helper;
		helper.Push( attacker ? attacker->GetScriptObject() : NULL );
		helper.Push( oldHealth );
		helper.Push( health );
		CallNonBlockingScriptEvent( postDamageFunc, helper );
	}
}

/*
================
sdScriptEntity::Killed
================
*/
void sdScriptEntity::Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location, const sdDeclDamage* damageDecl ) {
	sdScriptHelper helper;
	helper.Push( inflictor ? inflictor->GetScriptObject() : NULL );
	helper.Push( attacker ? attacker->GetScriptObject() : NULL );
	helper.Push( damage );
	helper.Push( dir );
	helper.Push( location );

	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnKilled" ), helper );
}

/*
================
sdScriptEntity::UpdateRadar
================
*/
void sdScriptEntity::UpdateRadar( void ) {
	radarInterface.UpdatePosition( GetRadarOrigin(), GetRadarAxes() );
}

/*
================
sdScriptEntity::GetIsDisabled
================
*/
bool sdScriptEntity::GetIsDisabled( void ) {
	if ( isDisabledFunc == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	return CallBooleanNonBlockingScriptEvent( isDisabledFunc, h1 );
}

/*
================
sdScriptEntity::GetIsDeployed
================
*/
bool sdScriptEntity::GetIsDeployed( void ) {
	if ( isDeployedFunc == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	return CallBooleanNonBlockingScriptEvent( isDeployedFunc, h1 );
}

/*
================
sdScriptEntity::GetOwner
================
*/
idEntity* sdScriptEntity::GetOwner( void ) {
	if ( getOwnerFunc == NULL ) {
		return NULL;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( getOwnerFunc, h1 );

	idScriptObject* object = gameLocal.program->GetReturnedObject();
	if ( object == NULL ) {
		return NULL;
	}

	return object->GetClass()->Cast< idEntity >();
}

/*
================
sdScriptEntity::CreateScriptThread
================
*/
sdProgramThread* sdScriptEntity::CreateScriptThread( const sdProgram::sdFunction* state ) {
	scriptIdealState = state;
	scriptState = state;

	sdProgramThread* thread = gameLocal.program->CreateThread();
	thread->CallFunction( scriptObject, scriptState );
	thread->SetName( name.c_str() );
	thread->ManualControl();
	thread->ManualDelete();
	return thread;
}

/*
================
sdScriptEntity::Event_SetState
================
*/
void sdScriptEntity::Event_SetState( const char* stateName ) {
	const sdProgram::sdFunction* func = scriptObject->GetFunction( stateName );
	if ( !func ) {
		gameLocal.Error( "sdScriptEntity::Event_SetState Can't find function '%s' in object '%s'", stateName, scriptObject->GetTypeName() );
	}

	BecomeActive( TH_RUNSCRIPT );

	scriptIdealState = func;
	if ( baseScriptThread != NULL ) {
		baseScriptThread->DoneProcessing();
	} else {
		baseScriptThread = CreateScriptThread( scriptIdealState );
	}
}

/*
================
sdScriptEntity::Event_AddHelper
================
*/
void sdScriptEntity::Event_AddHelper( int stringMapIndex ) {
	const sdDeclStringMap* stringMap = gameLocal.declStringMapType[ stringMapIndex ];
	if ( !stringMap ) {
		return;
	}

	const char* type = stringMap->GetDict().GetString( "helper_type" );
	sdScriptedEntityHelper* helper = sdScriptedEntityHelper::AllocHelper( type );
	if ( !helper ) {
		return;
	}

	helper->Init( this, stringMap );
	BecomeActive( TH_THINK );
}

/*
================
sdScriptEntity::Event_RadarFreeLayers
================
*/
void sdScriptEntity::Event_RadarFreeLayers( void ) {
	radarInterface.FreeLayers();
}

/*
================
sdScriptEntity::Event_RadarAllocLayer
================
*/
void sdScriptEntity::Event_AllocRadarLayer( void ) {
	int index = radarInterface.AllocRadarLayer( GetGameTeam() );

	sdTeamInfo::sdRadarLayer* layer = radarInterface.GetLayer( index );
	if ( layer != NULL ) {
		layer->SetOwner( this );
	}

	UpdateRadar();
	sdProgram::ReturnInteger( index );
}

/*
================
sdScriptEntity::Event_AllocJammerLayer
================
*/
void sdScriptEntity::Event_AllocJammerLayer( void ) {
	int index = radarInterface.AllocJammerLayer( GetGameTeam() );

	sdTeamInfo::sdRadarLayer* layer = radarInterface.GetLayer( index );
	if ( layer != NULL ) {
		layer->SetOwner( this );
	}

	UpdateRadar();
	sdProgram::ReturnInteger( index );
}

/*
================
sdScriptEntity::Event_RadarSetLayerRange
================
*/
void sdScriptEntity::Event_RadarSetLayerRange( int index, float range ) {
	sdTeamInfo::sdRadarLayer* layer = radarInterface.GetLayer( index );
	if ( !layer ) {
		return;
	}

	layer->SetRange( range );
}

/*
================
sdScriptEntity::Event_RadarSetMaxAngle
================
*/
void sdScriptEntity::Event_RadarSetMaxAngle( int index, float maxAngle ) {
	sdTeamInfo::sdRadarLayer* layer = radarInterface.GetLayer( index );
	if ( !layer ) {
		return;
	}

	layer->SetMaxAngle( maxAngle );
}

/*
================
sdScriptEntity::Event_RadarSetMask
================
*/
void sdScriptEntity::Event_RadarSetMask( int index, int mask ) {
	sdTeamInfo::sdRadarLayer* layer = radarInterface.GetLayer( index );
	if ( !layer ) {
		return;
	}

	layer->SetMask( mask );
}

/*
================
sdScriptEntity::Event_Freeze
================
*/
void sdScriptEntity::Event_Freeze( bool freeze ) {
	scriptEntityFlags.frozen = freeze;
}



/*
=====================
sdScriptEntity::UpdateScript
=====================
*/
void sdScriptEntity::UpdateScript( void ) {
	if ( gameLocal.IsPaused() ) {
		return;
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for ( int i = 0; i < 20; i++ ) {
		if ( scriptIdealState != scriptState ) {
			SetState( scriptIdealState );
		}

		// don't call script until it's done waiting
		if ( baseScriptThread->IsWaiting() ) {
			return;
		}

		if ( baseScriptThread->Execute() ) {
			if ( scriptIdealState == scriptState ) {
				gameLocal.program->FreeThread( baseScriptThread );
				baseScriptThread = NULL;
				BecomeInactive( TH_RUNSCRIPT );
				return;
			}
		} else {
			if ( scriptIdealState == scriptState ) {
				return;
			}
		}
	}

	baseScriptThread->Warning( "sdScriptEntity::UpdateScript Exited Loop to Prevent Lockup" );
}

/*
=====================
sdScriptEntity::SetState
=====================
*/
bool sdScriptEntity::SetState( const sdProgram::sdFunction* newState ) {
	if ( !newState ) {
		gameLocal.Error( "sdScriptEntity::SetState NULL state" );
	}

	scriptState = newState;
	scriptIdealState = scriptState;

	baseScriptThread->CallFunction( scriptObject, scriptState );

	return true;
}

/*
=====================
sdScriptEntity::Think
=====================
*/
void sdScriptEntity::Think( void ) {
	if ( gameLocal.isNewFrame ) {
		if ( thinkFlags & TH_RUNSCRIPT ) {
			UpdateScript();
		}
	}

	// Gordon: Force physics to run if we have someone bound to us, as they will need to get their physiscs updated too
	if ( ( thinkFlags & TH_PHYSICS ) == 0 ) {
		if ( usableInterface != NULL ) {
			if ( !usableInterface->IsEmpty() ) {
				BecomeActive( TH_PHYSICS );
			}
		}
	}

	if ( thinkFlags & TH_PHYSICS ) {
		if ( !scriptEntityFlags.frozen ) {
			if ( RunPhysics() ) {
				UpdateRadar();
			}
		}
	}

	sdScriptedEntityHelper* helper;
	for ( helper = helpers.Next(); helper; ) {
		sdScriptedEntityHelper* next = helper->GetNode().Next();
		helper->Update( false );
		helper = next;
	}

	if ( gameLocal.isNewFrame ) {
		if ( thinkFlags & TH_ANIMATE ) {
			UpdateAnimation();
		}

		if ( guiInterface ) {
			guiInterface->UpdateGui();
		}

		if ( !usableInterface ) { // if we have a usable interface present will be done in postthink
			Present();
		}
	}

	UpdatePlayZoneInfo();

	if ( thinkFlags & TH_THINK && !WantsToThink() ) {
		BecomeInactive( TH_THINK );
	}
}

/*
================
sdScriptEntity::OnGuiActivated
================
*/
void sdScriptEntity::OnGuiActivated( void ) {
	if ( guiInterface != NULL ) {
		BecomeActive( TH_THINK );
	}
}

/*
================
sdScriptEntity::WantsToThink
================
*/
bool sdScriptEntity::WantsToThink( void ) const {
	if ( guiInterface && guiInterface->WantsToThink() ) {
		return true;
	}

	for ( sdScriptedEntityHelper* helper = helpers.Next(); helper != NULL; helper = helper->GetNode().Next() ) {
		if ( helper->WantsToThink() ) {
			return true;
		}
	}

	return false;
}

/*
================
sdScriptEntity::PostThink
================
*/
void sdScriptEntity::PostThink( void ) {
	if ( usableInterface != NULL ) {
		usableInterface->UpdatePlayerViews();
	}
	if( onPostThink != NULL ) {
		sdScriptHelper helper;
		CallNonBlockingScriptEvent( onPostThink, helper );
	}
	Present();
	if ( usableInterface != NULL ) {
		usableInterface->PresentPlayers();
	}

	sdScriptedEntityHelper* helper;
	for ( helper = helpers.Next(); helper; ) {
		sdScriptedEntityHelper* next = helper->GetNode().Next();
		helper->Update( true );
		helper = next;
	}
}

/*
============
sdScriptEntity::UpdateModelTransform
============
*/
void sdScriptEntity::UpdateModelTransform( void ) {
	idAnimatedEntity::UpdateModelTransform();
	DoPredictionErrorDecay();
}

/*
============
sdScriptEntity::DoPredictionErrorDecay
============
*/
void sdScriptEntity::DoPredictionErrorDecay( void ) {
	if ( gameLocal.isClient && !IsBound() ) {
		if ( predictionErrorDecay_Origin != NULL ) {
			predictionErrorDecay_Origin->Decay( renderEntity.origin );
		}
		if ( predictionErrorDecay_Angles != NULL ) {
			predictionErrorDecay_Angles->Decay( renderEntity.axis );
		}
		fl.allowPredictionErrorDecay = false;
	}
}

/*
============
sdScriptEntity::UpdatePredictionErrorDecay
============
*/
void sdScriptEntity::UpdatePredictionErrorDecay( void ) {
	if ( predictionErrorDecay_Origin != NULL ) {
		predictionErrorDecay_Origin->Update();
	}
	if ( predictionErrorDecay_Angles != NULL ) {
		predictionErrorDecay_Angles->Update();
	}
}

/*
============
sdScriptEntity::OnNewOriginRead
============
*/
void sdScriptEntity::OnNewOriginRead( const idVec3& newOrigin ) {
	if ( predictionErrorDecay_Origin != NULL ) {
		predictionErrorDecay_Origin->OnNewInfo( newOrigin );
	}
}

/*
============
sdScriptEntity::OnNewAxesRead
============
*/
void sdScriptEntity::OnNewAxesRead( const idMat3& newAxes ) {
	if ( predictionErrorDecay_Angles != NULL ) {
		predictionErrorDecay_Angles->OnNewInfo( newAxes );
	}
}

/*
============
sdScriptEntity::ResetPredictionErrorDecay
============
*/
void sdScriptEntity::ResetPredictionErrorDecay( const idVec3* origin, const idMat3* axes ) {

	idVec3 resetOrigin = GetPhysics()->GetOrigin();
	if ( origin != NULL ) {
		resetOrigin = *origin;
	}
	idMat3 resetAxes = GetPhysics()->GetAxis();
	if ( axes != NULL ) {
		resetAxes = *axes;
	}

	bool doneReset = false;
	if ( predictionErrorDecay_Origin != NULL ) {
		predictionErrorDecay_Origin->Reset( resetOrigin );
		doneReset = true;
	}
	if ( predictionErrorDecay_Angles != NULL ) {
		predictionErrorDecay_Angles->Reset( resetAxes );
		doneReset = true;
	}

	if ( doneReset && gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_RESETPREDICTIONDECAY );
		msg.WriteVector( resetOrigin );
		msg.WriteCQuat( resetAxes.ToCQuat() );
		msg.Send( false, sdReliableMessageClientInfoAll() );
	}

	interpolateHistory[ 0 ] = resetOrigin;
	interpolateHistory[ 1 ] = resetOrigin;
}

/*
=====================
sdScriptEntity::GetTeamIndex
=====================
*/
int sdScriptEntity::GetTeamIndex( void ) {
	return team ? team->GetIndex() : -1;
}

/*
=====================
sdScriptEntity::SetGameTeam
=====================
*/
void sdScriptEntity::SetGameTeam( sdTeamInfo* _team ) {
	if ( _team == team ) {
		return;
	}

	sdTeamInfo* oldTeam = team;

	team = _team;

	radarInterface.FreeLayers();

	sdScriptHelper helper;
	helper.Push( oldTeam );
	helper.Push( team );
	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnSetTeam" ), helper );
}

/*
================
sdScriptEntity::DisableClip
================
*/
void sdScriptEntity::DisableClip( bool activateContacting ) {
	idAnimatedEntity::DisableClip( activateContacting );
	DisableCombat();
}

/*
================
sdScriptEntity::EnableClip
================
*/
void sdScriptEntity::EnableClip( void ) {
	if ( fl.forceDisableClip ) {
		return;
	}
	idAnimatedEntity::EnableClip();
	EnableCombat();
}

/*
================
sdScriptEntity::OnTouch
================
*/
void sdScriptEntity::OnTouch( idEntity *other, const trace_t& trace ) {
	if ( !touchFunc ) {
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( player != NULL && !player->IsSpectator() && player->GetHealth() <= 0 ) {
		return;
	}

	sdLoggedTrace* loggedTrace = gameLocal.RegisterLoggedTrace( trace );

	sdScriptHelper helper;
	helper.Push( other->GetScriptObject() );
	helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
	CallNonBlockingScriptEvent( touchFunc, helper );

	gameLocal.FreeLoggedTrace( loggedTrace );
}

/*
================
sdScriptEntity::Event_GetBoundPlayer
================
*/
void sdScriptEntity::Event_GetBoundPlayer( int index ) {
	sdProgram::ReturnEntity( usableInterface ? usableInterface->GetBoundPlayer( index ) : NULL );
}

/*
================
sdScriptEntity::Event_RemoveBoundPlayer
================
*/
void sdScriptEntity::Event_RemoveBoundPlayer( idEntity* other ) {
	idPlayer* player = other->Cast< idPlayer >();
	if ( !player ) {
		gameLocal.Warning( "sdScriptEntity::Event_RemoveBoundPlayer Object Passed was not a Player" );
		return;
	}
	
	if ( usableInterface ) {
		usableInterface->OnExit( player, true );
	}
}

/*
================
sdScriptEntity::SetHealth
================
*/
void sdScriptEntity::SetHealth( int value ) {
	int oldHealth = health;

	health = value;

	if ( postDamageFunc ) {
		sdScriptHelper helper;
		helper.Push( static_cast< idScriptObject* >( NULL ) );
		helper.Push( oldHealth );
		helper.Push( health );
		CallNonBlockingScriptEvent( postDamageFunc, helper );
	}
}

/*
================
sdScriptEntity::SetHealth
================
*/
void sdScriptEntity::SetHealthDamaged( int count, teamAllegiance_t allegiance ) {
	// Gordon: anything not explicitly from an enemy is counted, to prevent exploits using damage from generic environment damage
	if ( allegiance != TA_ENEMY ) {
		int diff = health - count;
		if ( diff > 0 ) {
			teamDamageDone += diff;
		}
	}

	// Gordon: no callback here, as it will be handled elsewhere with the proper attacked being passed
	health = count;
}

/*
================
sdScriptEntity::UpdateCrosshairInfo
================
*/
bool sdScriptEntity::UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info ) {
	if ( !crosshairInterface ) {
		return false;
	}

	crosshairInfo = &info;

	if ( crosshairInterface->UpdateCrosshairInfo( player ) ) {
		crosshairInfo = NULL;
		return true;
	}

	crosshairInfo = NULL;
	return false;
}

/*
================
sdScriptEntity::GetPostThinkNode
================
*/
idLinkList< idEntity >* sdScriptEntity::GetPostThinkNode( void ) {
	return &postThinkEntNode;
}

/*
================
sdScriptEntity::GetRadarOrigin
================
*/
const idVec3& sdScriptEntity::GetRadarOrigin( void ) const {
	return GetPhysics()->GetOrigin();
}

/*
================
sdScriptEntity::GetRadarAxes
================
*/
const idMat3& sdScriptEntity::GetRadarAxes( void ) const {
	return GetPhysics()->GetAxis();
}

/*
================
sdScriptEntity::Hit
================
*/
void sdScriptEntity::Hit( const trace_t &collision, const idVec3 &velocity, idEntity *other ) {
	if ( onHitFunc ) {
		sdLoggedTrace* loggedTrace = gameLocal.RegisterLoggedTrace( collision );

		sdScriptHelper helper;
		helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
		helper.Push( velocity );
		helper.Push( other->GetScriptObject() );
		CallFloatNonBlockingScriptEvent( onHitFunc, helper );

		gameLocal.FreeLoggedTrace( loggedTrace );
	}
}

/*
================
sdScriptEntity::Collide
================
*/
bool sdScriptEntity::Collide( const trace_t &collision, const idVec3 &velocity, int bodyId ) {
	if ( onCollideFunc ) {
		sdLoggedTrace* loggedTrace = gameLocal.RegisterLoggedTrace( collision );

		sdScriptHelper helper;
		helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
		helper.Push( velocity );
		helper.Push( bodyId );
		bool retVal = CallFloatNonBlockingScriptEvent( onCollideFunc, helper ) != 0.0f;

		gameLocal.FreeLoggedTrace( loggedTrace );

		return retVal;
	} else {
		return false;
	}
}

/*
================
sdScriptEntity::UpdateKillStats
================
*/
void sdScriptEntity::UpdateKillStats( idPlayer* player, const sdDeclDamage* damageDecl, bool headshot ) {
	if ( killStatSuffix == "" ) {
		return;
	}

	const char* prefix = damageDecl->GetStats().name.c_str();
	if ( !*prefix ) {
		return;
	}

	if ( GetDamageXPScale() <= 0.f ) {
		return; // Gordon: No stats if the object doesn't give XP
	}

	sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

	teamAllegiance_t allegiance = GetEntityAllegiance( player );
	if ( allegiance == TA_ENEMY ) {
		tracker.GetStat( tracker.AllocStat( va( "%s_kills_%s", prefix, killStatSuffix.c_str() ), sdNetStatKeyValue::SVT_INT ) )->IncreaseValue( player->entityNumber, 1 );
		tracker.GetStat( tracker.AllocStat( va( "total_kills_%s", prefix, killStatSuffix.c_str() ), sdNetStatKeyValue::SVT_INT ) )->IncreaseValue( player->entityNumber, 1 );
	} else if ( allegiance == TA_FRIEND ) {
		tracker.GetStat( tracker.AllocStat( va( "%s_teamkills_%s", prefix, killStatSuffix.c_str() ), sdNetStatKeyValue::SVT_INT ) )->IncreaseValue( player->entityNumber, 1 );
	}
}

/*
================
sdScriptEntity::OverridePreventDeployment
================
*/
bool sdScriptEntity::OverridePreventDeployment( idPlayer* p ) {
	if ( !scriptObject || !overrideFunc ) {
		return false;
	}

	sdScriptHelper h;
	h.Push( p->scriptObject );
	return this->CallBooleanNonBlockingScriptEvent( overrideFunc, h );
}

/*
================
sdScriptEntity::OnApplyPain
================
*/
bool sdScriptEntity::OnApplyPain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	return Pain( inflictor, attacker, damage, dir, location, damageDecl );
}

/*
================
sdScriptEntity::ClientReceiveEvent
================
*/
bool sdScriptEntity::ClientReceiveEvent( int event, int time, const idBitMsg& msg ) {
	switch ( event ) {
		case EVENT_SETBOXCLIPMODEL: {
			idVec3 mins = msg.ReadVector();
			idVec3 maxs = msg.ReadVector();
			float mass = msg.ReadFloat();

			Event_SetBoxClipModel( mins, maxs, mass );
			return true;
		}
		case EVENT_RESETPREDICTIONDECAY: {
			idVec3 origin = msg.ReadVector();
			idMat3 axes = msg.ReadCQuat().ToMat3();
			ResetPredictionErrorDecay( &origin, &axes );
			SetOrigin( origin );
			SetAxis( axes );
			return true;
		}
	}

	return idAnimatedEntity::ClientReceiveEvent( event, time, msg );
}

/*
================
sdScriptEntity::ApplyNetworkState
================
*/
void sdScriptEntity::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdScriptEntityNetworkData );
		NET_APPLY_STATE_SCRIPT;
		NET_APPLY_STATE_PHYSICS;

		if ( scriptEntityFlags.writeViewAngles ) {
			deltaViewAngles = newData.deltaViewAngles;
		}
	} else if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdScriptEntityBroadcastData );
		NET_APPLY_STATE_SCRIPT;
		NET_APPLY_STATE_PHYSICS;

		if ( scriptEntityFlags.writeBind ) {
			newData.bindData.Apply( this );
		}

		int oldHealth = health;
		health = newData.health;
		scriptEntityFlags.frozen = newData.frozen;

		if ( health < oldHealth ) {
			if ( postDamageFunc ) {
				sdScriptHelper helper;
				helper.Push( static_cast< idScriptObject* >( NULL ) );
				helper.Push( oldHealth );
				helper.Push( health );
				CallNonBlockingScriptEvent( postDamageFunc, helper );
			}

			if ( oldHealth > 0 ) {
				if ( health <= 0 ) {
					if ( health < -999 ) {
						health = -999;
					}
					Killed( NULL, NULL, oldHealth - health, vec3_origin, INVALID_JOINT, NULL );
				} else {
					OnApplyPain( NULL, NULL, oldHealth - health, vec3_origin, INVALID_JOINT, NULL );
				}
			}
		} else if ( health > oldHealth ) {
			if ( postDamageFunc ) {
				sdScriptHelper helper;
				helper.Push( static_cast< idScriptObject* >( NULL ) );
				helper.Push( oldHealth );
				helper.Push( health );
				CallNonBlockingScriptEvent( postDamageFunc, helper );
			}
		}

		SetGameTeam( newData.team );
	}
}

/*
================
sdScriptEntity::ReadNetworkState
================
*/
void sdScriptEntity::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdScriptEntityNetworkData );
		NET_READ_STATE_SCRIPT;
		NET_READ_STATE_PHYSICS;

		if ( scriptEntityFlags.writeViewAngles ) {
			newData.deltaViewAngles[ 0 ]	= msg.ReadDeltaFloat( baseData.deltaViewAngles[ 0 ] );
			newData.deltaViewAngles[ 1 ]	= msg.ReadDeltaFloat( baseData.deltaViewAngles[ 1 ] );
			newData.deltaViewAngles[ 2 ]	= 0.f;
		}
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdScriptEntityBroadcastData );
		NET_READ_STATE_SCRIPT;
		NET_READ_STATE_PHYSICS;

		if ( scriptEntityFlags.writeBind ) {
			newData.bindData.Read( this, baseData.bindData, msg );
		}

		newData.health = msg.ReadDeltaShort( baseData.health );
		newData.team = sdTeamManager::GetInstance().ReadTeamFromStream( baseData.team, msg );
		newData.frozen = msg.ReadBool();

		return;
	}
}

/*
================
sdScriptEntity::WriteNetworkState
================
*/
void sdScriptEntity::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdScriptEntityNetworkData );

		scriptObject->WriteNetworkState( mode, baseData.scriptData, newData.scriptData, msg );

		NET_WRITE_STATE_PHYSICS;

		if ( scriptEntityFlags.writeViewAngles ) {
			newData.deltaViewAngles[ 0 ] = deltaViewAngles[ 0 ];
			newData.deltaViewAngles[ 1 ] = deltaViewAngles[ 1 ];
			newData.deltaViewAngles[ 2 ] = 0.f;

			msg.WriteDeltaFloat( baseData.deltaViewAngles[ 0 ], newData.deltaViewAngles[ 0 ] );
			msg.WriteDeltaFloat( baseData.deltaViewAngles[ 1 ], newData.deltaViewAngles[ 1 ] );
		}
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdScriptEntityBroadcastData );

		scriptObject->WriteNetworkState( mode, baseData.scriptData, newData.scriptData, msg );

		NET_WRITE_STATE_PHYSICS;

		if ( scriptEntityFlags.writeBind ) {
			newData.bindData.Write( this, baseData.bindData, msg );
		}

		newData.health	= health;
		msg.WriteDeltaShort( baseData.health, newData.health );

		newData.team	= team;
		sdTeamManager::GetInstance().WriteTeamToStream( baseData.team, newData.team, msg );
		
		newData.frozen	= scriptEntityFlags.frozen;
		msg.WriteBool( newData.frozen );

		return;
	}
}

/*
================
sdScriptEntity::CheckNetworkStateChanges
================
*/
bool sdScriptEntity::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdScriptEntityNetworkData );

		if ( scriptObject->CheckNetworkStateChanges( mode, baseData.scriptData ) ) {
			return true;
		}

		NET_CHECK_STATE_PHYSICS;

		if ( scriptEntityFlags.writeViewAngles ) {
			NET_CHECK_FIELD( deltaViewAngles[ 0 ], deltaViewAngles[ 0 ] );
			NET_CHECK_FIELD( deltaViewAngles[ 1 ], deltaViewAngles[ 1 ] );
		}
		return false;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdScriptEntityBroadcastData );

		if ( scriptObject->CheckNetworkStateChanges( mode, baseData.scriptData ) ) {
			return true;
		}

		NET_CHECK_STATE_PHYSICS;

		if ( baseData.health != health ) {
			return true;
		}

		if ( baseData.team != team ) {
			return true;
		}

		if ( scriptEntityFlags.writeBind && baseData.bindData.Check( this ) ) {
			return true;
		}

		if ( baseData.frozen != scriptEntityFlags.frozen ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
sdScriptEntity::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdScriptEntity::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		sdScriptEntityNetworkData* newData = new sdScriptEntityNetworkData();
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
		return newData;
	}
	if ( mode == NSM_BROADCAST ) {
		sdScriptEntityBroadcastData* newData = new sdScriptEntityBroadcastData();
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
		return newData;
	}
	return NULL;
}

/*
================
sdScriptEntity::ResetNetworkState
================
*/
void sdScriptEntity::ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdScriptEntityBroadcastData );

		if ( newData.physicsData ) {
			GetPhysics()->ResetNetworkState( mode, *newData.physicsData );
		}

		return;
	}
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdScriptEntityNetworkData );

		if ( newData.physicsData ) {
			GetPhysics()->ResetNetworkState( mode, *newData.physicsData );
		}

		return;
	}

}

/*
============
sdScriptEntity::OnSnapshotHitch
============
*/
void sdScriptEntity::OnSnapshotHitch() {
	ResetPredictionErrorDecay();
}

/*
================
sdScriptEntity::NeedsRepair
================
*/
bool sdScriptEntity::NeedsRepair() {
	if ( needsRepairFunc ) {
		sdScriptHelper helper;
		return CallBooleanNonBlockingScriptEvent( needsRepairFunc, helper );
	}

	return false;
}

/*
================
sdScriptEntity::Event_SetViewAngles
================
*/
void sdScriptEntity::Event_SetRemoteViewAngles( const idVec3& angles, idEntity* refPlayer ) {
	const idPlayer* player = refPlayer->Cast< idPlayer >();
	if ( player ) {
		SetRemoteViewAngles( idAngles( angles ), player );
	}
}

/*
================
sdScriptEntity::Event_GetViewAngles
================
*/
void sdScriptEntity::Event_GetRemoteViewAngles( idEntity* refPlayer ) {
	const idPlayer* player = refPlayer->Cast< idPlayer >();
	if ( player ) {
		idAngles angles = GetRemoteViewAngles( player );
		sdProgram::ReturnVector( idVec3( angles[ 0 ], angles[ 1 ], angles[ 2 ] ) );
		return;
	}
	sdProgram::ReturnVector( idVec3( 0.0f, 0.0f, 0.0f ) );
}

/*
================
sdScriptEntity::SetViewAngles
================
*/
void sdScriptEntity::SetRemoteViewAngles( const idAngles& angles, const idPlayer* refPlayer  ) {
	for( int i = 0; i < 3; i++ ) {
		deltaViewAngles[ i ] = angles[ i ] - SHORT2ANGLE( refPlayer->usercmd.angles[ i ] );
	}
}

/*
================
sdScriptEntity::GetViewAngles
================
*/
idAngles sdScriptEntity::GetRemoteViewAngles( const idPlayer* refPlayer ) {
	idAngles refAngles;
	for( int i = 0; i < 3; i++ ) {
		refAngles[ i ] = idMath::AngleNormalize180( deltaViewAngles[ i ] + SHORT2ANGLE( refPlayer->usercmd.angles[ i ] ) );
	}

	return refAngles;
}

/*
================
sdScriptEntity::Event_PathFind
================
*/
void sdScriptEntity::Event_PathFind( const char* pathName, const idVec3& target, float startTime, float direction, float cornerX, float cornerY, float heightOffset, bool append ) {
	const sdPlayZone* pz = gameLocal.GetPlayZone( target, sdPlayZone::PZF_PATH );
	if ( pz == NULL ) {
		gameLocal.Warning( "sdScriptEntity::Event_PathFind No Valid Playzone at Target" );
		return;
	}

	sdVehiclePathGrid* vehiclePath = pz->GetPath( pathName );
	if ( !vehiclePath ) {
		gameLocal.Warning( "sdScriptEntity::Event_PathFind Invalid Path '%s'", pathName );
		return;
	}

	int oldPathPointsNum = pathPoints.Num();
	pathPoints.AssureSize( MAX_SCRIPTENTITY_PATHPOINTS );
	if ( !append ) {
		pathPoints.Clear();
		oldPathPointsNum = 0;
	}

	static idStaticList< idVec3, MAX_SCRIPTENTITY_PATHPOINTS > tempList;
	tempList.Clear();
	vehiclePath->SetupPathPoints( target, tempList, ( int )( startTime * 1000 ), cornerX, cornerY );

	if ( tempList.Num() > MAX_SCRIPTENTITY_PATHPOINTS ) {
		gameLocal.Error( "sdScriptEntity::Event_PathFind() - too many points in path" );
	}

	pathPoints.SetNum( oldPathPointsNum + tempList.Num() );

	if ( direction < 0.0f ) {
		int j = oldPathPointsNum;
		int i = tempList.Num() - 1;
		if ( append ) {
			// check that the new point isn't the same as the old point, ignore it if it is
			idVec3 diff = tempList[ tempList.Num() - 1 ] - pathPoints[ oldPathPointsNum - 1 ].origin;
			diff.z = 0.0f;
			if ( diff.LengthSqr() < idMath::FLT_EPSILON ) {
				i--;
				// shrink the pathpoints by one
				pathPoints.SetNum( pathPoints.Num() - 1, false );
			}
		}
		for ( ; i >= 0; i-- ) {
			pathPoints[ j ].origin = tempList[ i ];
			pathPoints[ j ].origin.z += heightOffset * 2.0f;
			pathPoints[ j ].axis.Identity();
			j++;
		}
	} else {
		int j = oldPathPointsNum;
		int i = 0;
		if ( append ) {
			// check that the new point isn't the same as the old point, ignore it if it is
			idVec3 diff = tempList[ 0 ] - pathPoints[ oldPathPointsNum - 1 ].origin;
			diff.z = 0.0f;
			if ( diff.LengthSqr() < idMath::FLT_EPSILON ) {
				i++;
				// shrink the pathpoints by one
				pathPoints.SetNum( pathPoints.Num() - 1, false );
			}
		}
		for ( ; i < tempList.Num(); i++ ) {
			pathPoints[ j ].origin = tempList[ i ];
			pathPoints[ j ].origin.z += heightOffset;
			pathPoints[ j ].axis.Identity();
			j++;
		}
	}

	// reset the searching
	lastPathPosition = 0.0f;
	lastPathLength = 0.0f;
	lastPathPoint = 0;
}

/*
================
sdScriptEntity::HasAbility
================
*/
bool sdScriptEntity::HasAbility( qhandle_t handle ) const {
	return abilities.HasAbility( handle );
}

/*
================
sdScriptEntity::Event_PathFindVampire
================
*/

// TWTODO: Allow the script to specify this stuff
#define VAMPIRE_RUN_WIDTH			512.0f
#define VAMPIRE_RUN_HEIGHT			512.0f
#define VAMPIRE_RUN_END_LENGTH		50000.0f

#define VAMPIRE_ENTRY_ANGLE_MIN		10.0f
#define VAMPIRE_ENTRY_ANGLE_MAX		88.0f
#define VAMPIRE_ENTRY_ANGLE_STEPS	8
#define VAMPIRE_ENTRY_PATH_POINTS	8

#define VAMPIRE_EXIT_ANGLE_MIN		20.0f
#define VAMPIRE_EXIT_ANGLE_MAX		88.0f
#define VAMPIRE_EXIT_ANGLE_STEPS	8
#define VAMPIRE_EXIT_PATH_POINTS	8

void sdScriptEntity::Event_PathFindVampire( const idVec3& runStart, const idVec3& runEnd, float runHeightOffset ) {
	pathPoints.AssureSize( MAX_SCRIPTENTITY_PATHPOINTS );
	pathPoints.Clear();

	idVec3 runDirection = runEnd - runStart;
	runDirection.z = 0.0f;
	float runLength = runDirection.Normalize();

	float runZ = runStart.z;
	sdProgram::ReturnFloat( runZ );

	// find some clear space to do the flying in
	const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( runStart, sdPlayZone::PZF_HEIGHTMAP );
	bool gotHeightMap = false;
	if ( playZoneHeight != NULL ) {
		const sdHeightMapInstance& heightMap = playZoneHeight->GetHeightMap();
		if ( heightMap.IsValid() ) {
			gotHeightMap = true;

			idVec3 runSide = runDirection.Cross( idVec3( 0.0f, 0.0f, 1.0f ) );
			float runWidth = VAMPIRE_RUN_WIDTH;
			idVec3 runWidthOffset = runSide * runWidth * 0.5f;

			idVec3 midRun = ( runStart + runEnd ) * 0.5f;
			idVec3 trueRunStart, trueRunEnd;
			trueRunStart = midRun - runLength * 0.5f * runDirection;
			trueRunEnd = trueRunStart + runLength * runDirection;

			// find the run height
			float heightL = heightMap.GetHeight( trueRunStart + runWidthOffset, trueRunEnd + runWidthOffset ) + runHeightOffset;
			float heightR = heightMap.GetHeight( trueRunStart - runWidthOffset, trueRunEnd - runWidthOffset ) + runHeightOffset;
			if ( heightR > heightL ) {
				midRun.z = trueRunStart.z = trueRunEnd.z = heightR;
			} else {
				midRun.z = trueRunStart.z = trueRunEnd.z = heightL;
			}

			runZ = midRun.z;

			float deltaL = VAMPIRE_RUN_END_LENGTH;
			idVec3 pathOffset = deltaL * 0.1f * runSide;


			//
			// Create the entry path
			//
			{
				float entryAngleStep = DEG2RAD( VAMPIRE_ENTRY_ANGLE_MAX - VAMPIRE_ENTRY_ANGLE_MIN ) / VAMPIRE_ENTRY_ANGLE_STEPS;
				float entryAngle = DEG2RAD( VAMPIRE_ENTRY_ANGLE_MIN );
				for ( int i = 0; i < VAMPIRE_ENTRY_ANGLE_STEPS; i++, entryAngle += entryAngleStep ) {
					float deltaZ = deltaL * idMath::Tan( entryAngle );
					idVec3 endPoint = trueRunStart - deltaL * runDirection + pathOffset;
					endPoint.z += deltaZ;
				
					idVec3 traceEnd;
					if ( heightMap.TracePoint( trueRunStart + runWidthOffset, endPoint + runWidthOffset, traceEnd, VAMPIRE_RUN_HEIGHT ) == 1.0f ) {
						if ( heightMap.TracePoint( trueRunStart - runWidthOffset, endPoint - runWidthOffset, traceEnd, VAMPIRE_RUN_HEIGHT ) == 1.0f ) {
							break;
						}
					}
				}

				// calculate the radius & center of the circle described by these two lines
				float deltaZ = deltaL * idMath::Tan( entryAngle );
				idVec3 endPoint = trueRunStart - deltaL * runDirection + pathOffset;
				endPoint.z += deltaZ;

				// find the axis mutually perpendicular to the two lines (A & B)
				idVec3 Adirection = -runDirection;
				idVec3 Bdirection = endPoint - trueRunStart;
				Bdirection.Normalize();

				idVec3 mutualPerp = Bdirection.Cross( Adirection );
				if ( mutualPerp.Normalize() < idMath::FLT_EPSILON ) {
					// uh-oh!
					assert( false );
					return;
				}

				// find the perpendiculars to A & B (the radii)
				idVec3 Aperp = Adirection.Cross( mutualPerp );
				idVec3 Bperp = Bdirection.Cross( mutualPerp );


				// calculate the TRUE angle
				entryAngle = idMath::ACos( Adirection * Bdirection );

				// find the length of the chord between the intersections of the tangents & radii
				float chordLength = 0.5f * runLength * idMath::Sin( idMath::PI - entryAngle ) / idMath::Sin( entryAngle / 2.0f );
				// now find the radius
				float radius = chordLength * idMath::Sin( ( idMath::PI - entryAngle )*0.5f ) / idMath::Sin( entryAngle );

				// use that to find the center
				idVec3 center = midRun + Aperp * radius;

				// generate path points
				pathPoint_t& pointEntry = pathPoints.Alloc();
				pointEntry.origin = endPoint;
				float step = entryAngle / VAMPIRE_ENTRY_PATH_POINTS;
				for ( float angle = -entryAngle; angle < step * 0.5f; angle += step ) {
					float trueAngle = idMath::PI + idMath::HALF_PI - angle;
					float deltaL = radius * idMath::Cos( trueAngle );
					float deltaZ = radius * ( idMath::Sin( trueAngle ) + 1.0f );

					idVec3 point = deltaL * Adirection + deltaZ * Aperp;
					point += midRun;

					pathPoint_t& pointEntry = pathPoints.Alloc();
					pointEntry.origin = point;

					idMat3 pointAxis;
					pointAxis[ 2 ] = center - point;
					pointAxis[ 2 ].Normalize();
					pointAxis[ 1 ] = -mutualPerp;
					pointAxis[ 0 ] = pointAxis[ 1 ].Cross( pointAxis[ 2 ] );
					pointAxis[ 0 ].Normalize();
					pointEntry.axis = pointAxis;
				}
			}


			pathOffset *= 5.0f;

			//
			// Create the exit path
			//
			{
				float exitAngleStep = DEG2RAD( VAMPIRE_EXIT_ANGLE_MAX - VAMPIRE_EXIT_ANGLE_MIN ) / VAMPIRE_EXIT_ANGLE_STEPS;
				float exitAngle = DEG2RAD( VAMPIRE_EXIT_ANGLE_MIN );
				for ( int i = 0; i < VAMPIRE_EXIT_ANGLE_STEPS; i++, exitAngle += exitAngleStep ) {
					float deltaZ = deltaL * idMath::Tan( exitAngle );
					idVec3 endPoint = trueRunEnd + deltaL * runDirection + pathOffset;
					endPoint.z += deltaZ;
					
					idVec3 traceEnd;
					if ( heightMap.TracePoint( trueRunEnd + runWidthOffset, endPoint + runWidthOffset, traceEnd, VAMPIRE_RUN_HEIGHT ) == 1.0f ) {
						if ( heightMap.TracePoint( trueRunEnd - runWidthOffset, endPoint - runWidthOffset, traceEnd, VAMPIRE_RUN_HEIGHT ) == 1.0f ) {
							break;
						}
					}
				}

				// calculate the radius & center of the circle described by these two lines
				float deltaZ = deltaL * idMath::Tan( exitAngle );
				idVec3 endPoint = trueRunEnd + deltaL * runDirection + pathOffset;
				endPoint.z += deltaZ;


				// find the exis mutually perpendicular to the two lines (A & B)
				idVec3 Adirection = runDirection;
				idVec3 Bdirection = endPoint - trueRunEnd;
				Bdirection.Normalize();

				idVec3 mutualPerp = Bdirection.Cross( Adirection );
				if ( mutualPerp.Normalize() < idMath::FLT_EPSILON ) {
					// uh-oh!
					assert( false );
					return;
				}

				// find the perpendiculars to A & B (the radii)
				idVec3 Aperp = Adirection.Cross( mutualPerp );
				idVec3 Bperp = Bdirection.Cross( mutualPerp );

				// calculate the TRUE angle
				exitAngle = idMath::ACos( Adirection * Bdirection );

				// find the length of the chord between the intersections of the tangents & radii
				float chordLength = 0.5f * runLength * idMath::Sin( idMath::PI - exitAngle ) / idMath::Sin( exitAngle / 2.0f );
				// now find the radius
				float radius = chordLength * idMath::Sin( ( idMath::PI - exitAngle )*0.5f ) / idMath::Sin( exitAngle );

				// use that to find the center
				idVec3 center = midRun + Aperp * radius;

				// generate path points
				float step = exitAngle / VAMPIRE_ENTRY_PATH_POINTS;
				for ( float angle = step; angle <= exitAngle; angle += step ) {
					float trueAngle = idMath::PI + idMath::HALF_PI - angle;
					float deltaL = -radius * idMath::Cos( trueAngle );
					float deltaZ = radius * ( idMath::Sin( trueAngle ) + 1.0f );

					idVec3 point = deltaL * Adirection + deltaZ * Aperp;
					point += midRun;

					pathPoint_t& pointEntry = pathPoints.Alloc();
					pointEntry.origin = point;

					idMat3 pointAxis;
					pointAxis[ 2 ] = center - point;
					pointAxis[ 2 ].Normalize();
					pointAxis[ 1 ] = mutualPerp;
					pointAxis[ 0 ] = pointAxis[ 1 ].Cross( pointAxis[ 2 ] );
					pointAxis[ 0 ].Normalize();
					pointEntry.axis = pointAxis;
				}
				pathPoint_t& pointEntry = pathPoints.Alloc();
				pointEntry.origin = endPoint;
			}
		}
	} 

	if ( !gotHeightMap ) {
		pathPoint_t& startPointEntry = pathPoints.Alloc();
		pathPoint_t& endPointEntry = pathPoints.Alloc();

		startPointEntry.origin = runStart - runDirection * 50000.0f;
		endPointEntry.origin = runEnd + runDirection * 50000.0f;
	}

	if ( pathPoints.Num() < 2 ) {
		// uh-oh! This should never ever happen!
		assert( false );
		return;
	}

	// trim the start and end points to fit inside the world bounds
	{
		const idBounds& worldBounds = gameLocal.clip.GetWorldBounds();
		const idVec3& mins = worldBounds.GetMins();
		const idVec3& maxs = worldBounds.GetMaxs();

		idVec3 startPoint = pathPoints[ 0 ].origin;
		idVec3 startDirection = pathPoints[ 1 ].origin - startPoint;
		startDirection.Normalize();
		idVec3 endPoint = pathPoints[ pathPoints.Num() - 1 ].origin;
		idVec3 endDirection = endPoint - pathPoints[ pathPoints.Num() - 2 ].origin;
		endDirection.Normalize();
	
		// trim ends
		for ( int i = 0; i < 3; i++ ) {
			if ( startPoint[ i ] < mins[ i ] ) {
				startPoint += ( ( mins[ i ] - startPoint[ i ] ) / startDirection[ i ] ) * startDirection;
			}
			if ( startPoint[ i ] > maxs[ i ] ) {
				startPoint += ( ( maxs[ i ] - startPoint[ i ] ) / startDirection[ i ] ) * startDirection;
			}

			if ( endPoint[ i ] < mins[ i ] ) {
				endPoint += ( ( mins[ i ] - endPoint[ i ] ) / endDirection[ i ] ) * endDirection;
			}
			if ( endPoint[ i ] > maxs[ i ] ) {
				endPoint += ( ( maxs[ i ] - endPoint[ i ] ) / endDirection[ i ] ) * endDirection;
			}
		}

		pathPoints[ 0 ].origin = startPoint;
		pathPoints[ pathPoints.Num() - 1 ].origin = endPoint;

		// end orientations
		pathPoints[ 0 ].axis = startDirection.ToMat3();
		pathPoints[ pathPoints.Num() - 1 ].axis = endDirection.ToMat3();
	}

	sdProgram::ReturnFloat( runZ );
}

/*
================
sdScriptEntity::LevelPathSegment
================
*/
void sdScriptEntity::LevelPathSegment( float maxSlope, int startPoint, int endPoint, bool reverse ) {
	int iDelta = reverse ? -1 : 1;

	// go through the path and level it out
	for ( int i = startPoint; i != endPoint; i += iDelta ) {
		if ( !reverse && i == endPoint - 1 ) {
			break;
		}

		idVec3& point = pathPoints[ i ].origin;
		idVec3& nextPoint = pathPoints[ i + iDelta ].origin;
		idVec3 segment = nextPoint - point;
		float segmentHeight = segment.z;
		segment.z = 0.0f;
		float segmentLength = segment.Length();
		
		// Gordon: TEMP: Tristan needs to merge the points or something, he says!
		if ( segmentLength == 0.f ) {
			nextPoint.z = point.z - segmentLength * maxSlope;
		} else {
			float slope = segmentHeight / segmentLength;
			if ( slope < -maxSlope ) {
				// goes down too fast - pull the next point up
				nextPoint.z = point.z - segmentLength * maxSlope;
			} else if ( slope > maxSlope ) { 
				// goes up too fast - pull the current point up and re-level that side
				point.z = nextPoint.z - segmentLength * maxSlope;
				if ( i != startPoint ) {
					LevelPathSegment( maxSlope, i, startPoint, !reverse );
				}
			}
		}
	}
}

/*
================
sdScriptEntity::Event_PathLevel
================
*/
void sdScriptEntity::Event_PathLevel( float maxSlopeAngle, int startIndex, int endIndex ) {
	float maxSlope = idMath::Fabs( idMath::Tan( RAD2DEG( maxSlopeAngle ) ) );

	int numPathPoints = pathPoints.Num();

	if ( numPathPoints == 0 ) {
		return;
	}

	if ( endIndex < startIndex ) {
		Swap( startIndex, endIndex );
	}

	if ( endIndex < 0 ) { 
		endIndex = numPathPoints - 1;
	}

	startIndex = idMath::ClampInt( 0, numPathPoints - 1, startIndex );
	endIndex = idMath::ClampInt( 0, numPathPoints - 1, endIndex );

	if ( startIndex == endIndex ) {
		return;
	}

	float pathHeight = pathPoints[ endIndex ].origin.z - pathPoints[ startIndex ].origin.z;
	
	if ( pathHeight > 2000.0f ) {
		LevelPathSegment( maxSlope, endIndex, startIndex, true );
	} else {
		LevelPathSegment( maxSlope, startIndex, endIndex, false );
	}
}

/*
================
sdScriptEntity::Event_PathStraighten
================
*/
void sdScriptEntity::Event_PathStraighten() {
	if ( pathPoints.Num() <= 3 ) {
		return;
	}

/*	// draw the path
	idVec3 lastPoint = pathPoints[ 0 ].origin;
	for ( int i = 1; i < pathPoints.Num(); i++ ) {
		gameRenderWorld->DebugLine( colorRed, lastPoint, pathPoints[ i ].origin, 10000 );
		lastPoint = pathPoints[ i ].origin;
	}*/
	

	// take samples of 3 points, and make the middle point closer to being a smooth path
	for ( int j = 0; j < 2; j++ ) {
		idVec3 startDirection = pathPoints[ j + 1 ].origin - pathPoints[ j ].origin;
		startDirection.Normalize();
		for ( int i = j; i < pathPoints.Num() - 3; i += 2 ) {
			const idVec3& startPoint = pathPoints[ i ].origin;
			idVec3& midPoint = pathPoints[ i + 1 ].origin;
			const idVec3& endPoint = pathPoints[ i + 2 ].origin;
			const idVec3& afterEndPoint = pathPoints[ i + 3 ].origin;
			idVec3 endDirection = afterEndPoint - endPoint;
			endDirection.Normalize();

			// cubic spline
			idVec3 x0 = startPoint;
			idVec3 x1 = endPoint;
			float distBetween = ( x0 - x1 ).Length();
			idVec3 dx0 = startDirection * distBetween * 0.25f;
			idVec3 dx1 = endDirection * distBetween * 0.25f;
			
			// calculate coefficients
			idVec3 D = x0;
			idVec3 C = dx0;
			idVec3 B = 3*x1 - dx1 - 2*C - 3*D;
			idVec3 A = x1 - B - C - D;

			// calculate the new midpoint
			float oldZ = midPoint.z;
			midPoint = A*(0.5f*0.5f*0.5f) + B*(0.5f*0.5f) + C*0.5f + D;
			if ( midPoint.z < oldZ ) {
				midPoint.z = oldZ;
			}

			startDirection = endDirection;
		}
	}

/*	// draw the path
	lastPoint = pathPoints[ 0 ].origin;
	for ( int i = 1; i < pathPoints.Num(); i++ ) {
		gameRenderWorld->DebugLine( colorGreen, lastPoint, pathPoints[ i ].origin, 10000 );
		lastPoint = pathPoints[ i ].origin;
	}*/
}

/*
================
sdScriptEntity::PathGetPoint
================
*/
const idVec3& sdScriptEntity::PathGetPoint( int index ) {
	if ( index < 0 || index >= pathPoints.Num() ) {
		return vec3_origin;
	}

	return pathPoints[ index ].origin;
}

/*
================
sdScriptEntity::PathGetLength
================
*/
float sdScriptEntity::PathGetLength( void ) {
	// Brute force algorithms FTW!
	float length = 0.0f;
	for ( int i = 1; i < pathPoints.Num(); i++ ) {
		idVec3 p1 = pathPoints[ i - 1 ].origin;
		idVec3 p2 = pathPoints[ i ].origin;

		length += ( p1 - p2 ).Length();
	}

	return length;
}

/*
================
sdScriptEntity::PathGetPosition
================
*/
void sdScriptEntity::PathGetPosition( float position, idVec3& result ) {
	if ( lastPathPosition > position ) {
		// last search was past this point
		// re-do the search from the start
		lastPathLength = 0.0f;
		lastPathPoint = 0;
	}

	lastPathPosition = position;

	// Brute force algorithms FTW!
	float length = lastPathLength;
	int numPoints = pathPoints.Num();

	if ( lastPathPoint == numPoints - 1 ) {
		// beyond the last point
		result = pathPoints[ lastPathPoint ].origin;
		return;
	}

	for ( int i = lastPathPoint + 1; i < numPoints; i++ ) {
		float oldLength = length;

		idVec3 p1 = pathPoints[ i - 1 ].origin;
		idVec3 p2 = pathPoints[ i ].origin;

		float distance = ( p1 - p2 ).Length(); 
		length += distance;

		// record the information about the point (so searches can be continued)
		lastPathLength = oldLength;
		lastPathPoint = i - 1;

		if ( length > position ) {
			// this is the right segment
			float blendFactor = ( position - oldLength ) / distance;
			result = Lerp( p1, p2, blendFactor );
			return;
		}

		if ( i == numPoints - 1 ) {
			result = p2;

			// record that the last one was right on the end
			lastPathPoint = numPoints - 1;
			return;
		}
	}

	result = vec3_origin;
}

/*
================
sdScriptEntity::Event_GetVampireBombPosition
================
*/
void sdScriptEntity::Event_GetVampireBombPosition( const idVec3& targetPos, float gravity, float pathSpeed, float stepDist, float pathLength ) {
	float bestPosition = -1.0f;
	float bestAcceleration = idMath::INFINITY;

	//
	// Go through the path and find when to launch a bomb to hit the target
	//
	idVec3 accel( 0.0f, 0.0f, gravity );
	for ( float position = 0.0f; position < pathLength; position += stepDist ) {
		idVec3 origin;
		idVec3 velocity;
		PathGetPosition( position, origin );
		PathGetDirection( position, velocity );
		velocity *= pathSpeed;

		// find the time to hit the target
		float zRoots[ 2 ];
		int numZRoots = idPolynomial::GetRoots2( accel.z * 0.5f, velocity.z, origin.z - targetPos.z, zRoots );

		// find the overall acceleration needed
		for ( int i = 0; i < numZRoots; i++ ) {
			if ( zRoots[ i ] < idMath::FLT_EPSILON ) {
				continue;
			}

			idVec3 accelerationNeeded = 2.0f * ( targetPos - origin - velocity * zRoots[ i ] );
			accelerationNeeded /= Square( zRoots[ i ] ); 

			// see if its the smallest acceleration needed yet (most natural looking path)
			float accel = accelerationNeeded.LengthSqr();
			if ( accel < bestAcceleration ) {
				bestAcceleration = accel;
				cachedBombAccel = accelerationNeeded;
				cachedBombFallTime = zRoots[ i ];
				bestPosition = position;
			}
		}
	}

	sdProgram::ReturnFloat( bestPosition );
}

/*
================
sdScriptEntity::Event_GetVampireBombAcceleration
================
*/
void sdScriptEntity::Event_GetVampireBombAcceleration( void ) {
	sdProgram::ReturnVector( cachedBombAccel );
}

/*
================
sdScriptEntity::Event_GetVampireBombFallTime
================
*/
void sdScriptEntity::Event_GetVampireBombFallTime( void ) {
	sdProgram::ReturnFloat( cachedBombFallTime );
}

/*
================
sdScriptEntity::PathGetDirection
================
*/
void sdScriptEntity::PathGetDirection( float position, idVec3& result ) {
	// find the path point that this position lies on
	if ( lastPathPosition != position ) {
		idVec3 temp;
		PathGetPosition( position, temp );
	}

	if ( lastPathPoint == pathPoints.Num() - 1 ) {
		// beyond the last point
		result = vec3_origin;
		return;
	}

	idVec3 p1 = pathPoints[ lastPathPoint ].origin;
	idVec3 p2 = pathPoints[ lastPathPoint + 1 ].origin;
	idVec3 delta = p2 - p1;
	delta.Normalize();

	result = delta;
}

/*
================
sdScriptEntity::PathGetAxis
================
*/
void sdScriptEntity::PathGetAxes( float position, idMat3& result ) {
	if ( lastPathPosition > position ) {
		// last search was past this point
		// re-do the search from the start
		lastPathLength = 0.0f;
		lastPathPoint = 0;
	}

	lastPathPosition = position;

	// Brute force algorithms FTW!
	float length = lastPathLength;
	int numPoints = pathPoints.Num();

	if ( lastPathPoint == numPoints - 1 ) {
		// beyond the last point
		result = pathPoints[ lastPathPoint ].axis;
		return;
	}

	for ( int i = lastPathPoint + 1; i < numPoints; i++ ) {
		float oldLength = length;

		idVec3 p1 = pathPoints[ i - 1 ].origin;
		idVec3 p2 = pathPoints[ i ].origin;

		float distance = ( p1 - p2 ).Length(); 
		length += distance;

		// record the information about the point (so searches can be continued)
		lastPathLength = oldLength;
		lastPathPoint = i - 1;

		if ( length > position ) {
			// this is the right segment
			float blendFactor = ( position - oldLength ) / distance;
			idQuat q1 = pathPoints[ i - 1 ].axis.ToQuat();
			idQuat q2 = pathPoints[ i ].axis.ToQuat();
			idQuat qi;
			qi.Slerp( q1, q2, blendFactor );
			result = qi.ToMat3();
			return;
		}

		if ( i == numPoints - 1 ) {
			result = pathPoints[ i ].axis;

			// record that the last one was right on the end
			lastPathPoint = numPoints - 1;
			return;
		}
	}

	result.Identity();
}

/*
================
sdScriptEntity::Event_PathGetNumPoints
================
*/
void sdScriptEntity::Event_PathGetNumPoints( void ) {
	sdProgram::ReturnInteger( PathGetNumPoints() );
}

/*
================
sdScriptEntity::Event_PathGetPoint
================
*/
void sdScriptEntity::Event_PathGetPoint( int index ) {
	sdProgram::ReturnVector( PathGetPoint( index ) );
}

/*
================
sdScriptEntity::Event_PathGetLength
================
*/
void sdScriptEntity::Event_PathGetLength() {
	sdProgram::ReturnFloat( PathGetLength() );
}

/*
================
sdScriptEntity::Event_PathGetPosition
================
*/
void sdScriptEntity::Event_PathGetPosition( float position ) {
	idVec3 temp;
	PathGetPosition( position, temp );
	sdProgram::ReturnVector( temp );
}

/*
================
sdScriptEntity::Event_PathGetDirection
================
*/
void sdScriptEntity::Event_PathGetDirection( float position ) {
	idVec3 temp;
	PathGetDirection( position, temp );
	sdProgram::ReturnVector( temp );
}

/*
================
sdScriptEntity::Event_PathGetAngles
================
*/
void sdScriptEntity::Event_PathGetAngles( float position ) {
	idMat3 temp;
	PathGetAxes( position, temp );
	idAngles tempAngles = temp.ToAngles();
	sdProgram::ReturnVector( idVec3( tempAngles[ 0 ], tempAngles[ 1 ], tempAngles[ 2 ] ) );
}

/*
================
sdScriptEntity::Event_SetGroundPosition
================
*/
void sdScriptEntity::Event_SetGroundPosition( const idVec3& position ) {
	sdPhysics_Simple* simplePhysics = GetPhysics()->Cast< sdPhysics_Simple >();
	if ( simplePhysics ) {
		simplePhysics->SetGroundPosition( position );
	}
}

/*
================
sdScriptEntity::Event_SetXPShareInfo
================
*/
void sdScriptEntity::Event_SetXPShareInfo( idEntity* entity, float scale ) {
	idPlayer* other = entity->Cast< idPlayer >();
	if ( entity != NULL && other == NULL ) {
		gameLocal.Warning( "sdScriptEntity::Event_SetXPShareInfo Invalid Entity Passed" );
		return;
	}

	if ( usableInterface != NULL ) {
		usableInterface->SetXPShareInfo( other, scale );
	}
}

/*
============
sdScriptEntity::GetDecalUsage
============
*/
cheapDecalUsage_t sdScriptEntity::GetDecalUsage( void ) {
	if ( !scriptEntityFlags.inhibitDecals ) {
		return idEntity::GetDecalUsage();
	} else {
		return CDU_INHIBIT;
	}
}

/*
================
sdScriptEntity::GetImpactInfo
================
*/
void sdScriptEntity::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	GetPhysics()->GetImpactInfo( id, point, info );

	if ( scriptEntityFlags.noCollisionPush ) {
		info->invMass = 0.0f;
		info->invInertiaTensor.Zero();
	}
}

/*
================
sdScriptEntity::GetImpactInfo
================
*/
bool sdScriptEntity::IsCollisionPushable( void ) { 
	return !scriptEntityFlags.noCollisionPush; 
}

/*
================
idEntity::Event_EnableCollisionPush
================
*/
void sdScriptEntity::Event_EnableCollisionPush( void ) {
	scriptEntityFlags.noCollisionPush = false;
}

/*
================
idEntity::Event_DisableCollisionPush
================
*/
void sdScriptEntity::Event_DisableCollisionPush( void ) {
	scriptEntityFlags.noCollisionPush = true;
}

/*
=========================
sdScriptEntity::UpdatePlayzoneInfo
=========================
*/
void sdScriptEntity::UpdatePlayZoneInfo( void ) {
	if ( !scriptEntityFlags.takesOobDamage ) {
		return;
	}
	
	const sdPlayZone* playZone = gameLocal.GetPlayZone( GetPhysics()->GetOrigin(), sdPlayZone::PZF_PLAYZONE );
	if ( playZone != NULL ) {
		const sdDeployMaskInstance* deployMask = playZone->GetMask( gameLocal.GetPlayZoneMask() );
		if ( deployMask == NULL || deployMask->IsValid( GetPhysics()->GetAbsBounds() ) == DR_CLEAR ) {
			// we're in the playzone, don't do anything
			lastTimeInPlayZone = gameLocal.time;
			return;
		}
	}

	if ( gameLocal.isClient ) {
		return;
	}

	if ( gameLocal.time - lastTimeInPlayZone >= oobDamageInterval ) {
		lastTimeInPlayZone = gameLocal.time;

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
			gameLocal.Warning( "sdScriptEntity::UpdatePlayZoneInfo: couldn't find damage decl %s", damageDeclName.c_str() );
			return;
		}

		Damage( NULL, NULL, idVec3( 0.f, 0.f, 1.f ), damageDecl, 1.f, NULL );	
	}
}

/*
=========================
sdScriptEntity::Event_SetBoxClipModel
=========================
*/
void sdScriptEntity::Event_SetBoxClipModel( const idVec3& mins, const idVec3& maxs, float mass ) {
	if ( physicsObj == NULL ) {
		// this only does anything for entities with one of the optional physics modes
		return;
	}

	// contents are stored by the clipmodel, so need to keep those the same
	int contents = physicsObj->GetContents();
	int clipMask = physicsObj->GetClipMask();

	// velocity & angular velocity are stored as momentum, so depend on the mass & inertia tensor
	// so we need to keep them the same!
	idVec3 velocity = physicsObj->GetLinearVelocity();
	idVec3 angularVelocity = physicsObj->GetAngularVelocity();

	idClipModel* newModel = new idClipModel( idTraceModel( idBounds( mins, maxs ) ), false );
	physicsObj->SetClipModel( newModel, 1.0f );
	physicsObj->SetMass( mass );
	physicsObj->SetContents( contents );
	physicsObj->SetClipMask( clipMask );
	physicsObj->SetLinearVelocity( velocity );
	physicsObj->SetAngularVelocity( angularVelocity );
	physicsObj->LinkClip();

	if ( !gameLocal.isClient ) {	
		// throw a warning if this clip model change has left us embedded in something
		if ( gameLocal.clip.Contents( CLIP_DEBUG_PARMS_SCRIPT physicsObj->GetOrigin(), newModel, physicsObj->GetAxis(), contents | clipMask, this ) ) {
			gameLocal.Warning( "sdScriptEntity::Event_SetBoxClipModel - This operation made the entity %s clip into something else!", GetName() );
		}
	}

	// send this using an event to make sure clients all get synced up properly
	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SETBOXCLIPMODEL );
		msg.WriteVector( mins );
		msg.WriteVector( maxs );
		msg.WriteFloat( mass );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}

	scriptEntityFlags.hasNewBoxClip = true;
}

/*
===============
sdScriptEntity::WriteDemoBaseData
==============
*/
void sdScriptEntity::WriteDemoBaseData( idFile* file ) const {
	idAnimatedEntity::WriteDemoBaseData( file );

	file->WriteBool( scriptEntityFlags.hasNewBoxClip );
	if ( scriptEntityFlags.hasNewBoxClip ) {
		const idBounds& bounds = physicsObj->GetClipModel()->GetBounds();
		file->WriteVec3( bounds.GetMins() );
		file->WriteVec3( bounds.GetMaxs() );
		file->WriteFloat( physicsObj->GetMass() );
	}
}

/*
===============
sdScriptEntity::ReadDemoBaseData
==============
*/
void sdScriptEntity::ReadDemoBaseData( idFile* file ) {
	idAnimatedEntity::ReadDemoBaseData( file );

	bool hasNewBoxClip;
	file->ReadBool( hasNewBoxClip );
	if ( hasNewBoxClip ) {
		idVec3 mins, maxs;
		float mass;
		file->ReadVec3( mins );
		file->ReadVec3( maxs );
		file->ReadFloat( mass );

		Event_SetBoxClipModel( mins, maxs, mass );
	}
}

/*
===============
sdScriptEntity::Event_GetTeamDamageDone
==============
*/
void sdScriptEntity::Event_GetTeamDamageDone( void ) {
	sdProgram::ReturnInteger( teamDamageDone );
}

/*
===============
sdScriptEntity::Event_SetTeamDamageDone
==============
*/
void sdScriptEntity::Event_SetTeamDamageDone( int value ) {
	if ( value < 0 ) {
		value = 0;
	}
	teamDamageDone = value;
}

/*
===============
sdScriptEntity::Event_ForceAnimUpdate
==============
*/
void sdScriptEntity::Event_ForceAnimUpdate( bool value ) {
	if ( value ) {
		IncForcedAnimUpdate();
	} else {
		DecForcedAnimUpdate();
	}
}

/*
===============
sdScriptEntity::Event_SetIKTarget
==============
*/
void sdScriptEntity::Event_SetIKTarget( idEntity* other, int index ) {
	idPlayer* player = NULL;
	if ( other != NULL ) {
		player = other->Cast< idPlayer >();
		if ( player == NULL ) {
			gameLocal.Error( "sdScriptEntity::Event_SetIKTarget Invalid Player" );
		}
	}

	for ( sdScriptedEntityHelper* helper = helpers.Next(); helper != NULL; helper = helper->GetNode().Next() ) {
		helper->SetIKTarget( player, index );
	}
}

/*
===============
sdScriptEntity::Event_HideInLocalView
==============
*/
void sdScriptEntity::Event_HideInLocalView( void ) {
	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( player != NULL ) {
		renderEntity.suppressSurfaceInViewID = player->entityNumber + 1;
		renderEntity.suppressShadowInViewID = player->entityNumber + 1;

		UpdateVisuals();
		NotifyVisChanged();
	}
}

/*
===============
sdScriptEntity::Event_ShowInLocalView
==============
*/
void sdScriptEntity::Event_ShowInLocalView( void ) {
	renderEntity.suppressSurfaceInViewID = 0;
	renderEntity.suppressShadowInViewID = 0;

	UpdateVisuals();
	NotifyVisChanged();
}

/*
===============
sdScriptEntity::Event_SetClipOriented
==============
*/
void sdScriptEntity::Event_SetClipOriented( bool oriented ) {
	sdPhysics_SimpleRigidBody* simplePhysics = GetPhysics()->Cast< sdPhysics_SimpleRigidBody >();
	if ( simplePhysics != NULL ) {
		simplePhysics->SetClipOriented( oriented );
	}
}

/*
================
sdScriptEntity_Projectile::CheckWater
================
*/
void sdScriptEntity::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects ) {
		waterEffects->SetOrigin( GetPhysics()->GetOrigin() );
		waterEffects->SetAxis( GetPhysics()->GetAxis() );
		waterEffects->SetVelocity( GetPhysics()->GetLinearVelocity() );
		waterEffects->CheckWater( this, waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}

/*
================
sdScriptEntity::OnMouseMove
================
*/
void sdScriptEntity::OnMouseMove( idPlayer* player, const idAngles& baseAngles, idAngles& angleDelta ) {
	// HACK: Limit the angles so that it'll never wrap around! :)
	angleDelta.yaw = idMath::ClampFloat( -179.0f, 179.0f, angleDelta.yaw );
	angleDelta.pitch = idMath::ClampFloat( -179.0f, 179.0f, angleDelta.pitch );
}


/*
================
sdScriptEntity::Event_SetIconMaterial
================
*/
void sdScriptEntity::Event_SetIconMaterial( const char* materialName ) {
	const idMaterial* material = gameLocal.declMaterialType[ spawnArgs.GetString( materialName ) ];
	displayIconInterface.SetIconMaterial( material );
}

/*
================
sdScriptEntity::Event_SetIconSize
================
*/
void sdScriptEntity::Event_SetIconSize( float width, float height ) {
	displayIconInterface.SetIconSize( idVec2( width, height ) );
}

/*
================
sdScriptEntity::Event_SetIconColorMode
================
*/
void sdScriptEntity::Event_SetIconColorMode( int mode ) {
	displayIconInterface.SetIconColorMode( ( sdScriptEntityDisplayIconInterface::iconColorMode_t )mode );
}

/*
================
sdScriptEntity::Event_SetIconPosition
================
*/
void sdScriptEntity::Event_SetIconPosition( int pos ) {
	displayIconInterface.SetIconPosition( ( sdScriptEntityDisplayIconInterface::iconPosition_t )pos );
}

/*
================
sdScriptEntity::Event_SetIconEnabled
================
*/
void sdScriptEntity::Event_SetIconEnabled( bool enabled ) {
	displayIconInterface.SetIconEnabled( enabled );
}

/*
================
sdScriptEntity::Event_SetIconCutoff
================
*/
void sdScriptEntity::Event_SetIconCutoff( float cutoff ) {
	displayIconInterface.SetIconCutoff( cutoff );
}

/*
================
sdScriptEntity::Event_SetIconAlphaScale
================
*/
void sdScriptEntity::Event_SetIconAlphaScale( float scale ) {
	displayIconInterface.SetIconAlphaScale( scale );
}


/*
===============================================================================

sdScriptEntityDisplayIconInterface

===============================================================================
*/

/*
================
sdScriptEntityDisplayIconInterface::Init
================
*/
void sdScriptEntityDisplayIconInterface::Init( sdScriptEntity* _owner ) { 
	owner = _owner; 
	hasIconResult = false; 
	enabled = false; 
	cutoffDistance = -1.0f;
	colorMode = EI_NONE;
	position = EI_CENTER;
	alphaScale = 1.0f;
}

/*
================
sdScriptEntityDisplayIconInterface::HasIcon
================
*/
bool sdScriptEntityDisplayIconInterface::HasIcon( idPlayer* viewPlayer, sdWorldToScreenConverter& converter ) {
	assert( owner != NULL );

	// evaluate all the information about if the icon is visible etc
	hasIconResult = false;

	// is it enabled?
	if ( !enabled ) {
		return false;
	}
	
	// is it visible?
	if ( !owner->IsVisibleOcclusionTest() ) {
		return false;
	}

	const idVec3& ownerOrigin = owner->GetLastPushedOrigin();
	const idMat3& ownerAxis = owner->GetLastPushedAxis();
	idBounds ownerBounds = owner->GetPhysics()->GetBounds();

	// calculate the origin of the icon in world space
	idVec3 origin = ownerOrigin + ( ownerBounds.GetCenter() * ownerAxis );

	// is it in front of the player?
	idVec3 delta = origin - viewPlayer->renderView.vieworg;
	if ( delta * viewPlayer->renderView.viewaxis[ 0 ] <= 0.0f ) {
		return false;
	}

	// is it too far away?
	storedInfo.distance = delta.Length();
	if ( cutoffDistance > 0.0f && storedInfo.distance > cutoffDistance ) {
		return false;
	}

	// calculate the final position
	if ( position == EI_ABOVE ) {
		idBounds transformedBounds = ownerBounds;
		transformedBounds.RotateSelf( ownerAxis );

		float verticalOffset = transformedBounds.GetSize().z * 0.5f;
		origin.z += verticalOffset;
		if ( verticalOffset > 64.0f ) {
			origin.z += 64.0f;
		} else {
			origin.z += verticalOffset;
		}
	}

	converter.Transform( origin, storedInfo.origin );
	storedInfo.origin.y += storedInfo.size.y * 0.5f;

	// find the color
	storedInfo.color.Set( 1.0f, 1.0f, 1.0f, 1.0f );
	if ( colorMode == EI_TEAM ) {
		teamAllegiance_t allegiance = owner->GetEntityAllegiance( viewPlayer );
		if ( allegiance != TA_NEUTRAL ) {
			storedInfo.color = idEntity::GetColorForAllegiance( allegiance );
		}
	}

	storedInfo.color.w *= alphaScale;


	hasIconResult = true;	
	return hasIconResult;
}

/*
================
sdScriptEntityDisplayIconInterface::GetEntityDisplayIconInfo
================
*/
bool sdScriptEntityDisplayIconInterface::GetEntityDisplayIconInfo( idPlayer* viewPlayer, sdWorldToScreenConverter& converter, sdEntityDisplayIconInfo& iconInfo ) {
	if ( hasIconResult ) {
		iconInfo = storedInfo;
	}
	return hasIconResult;
}

/*
===============================================================================

sdScriptEntity_Projectile

===============================================================================
*/
extern const idEventDef EV_GetLaunchTime;
extern const idEventDef EV_GetOwner;

CLASS_DECLARATION( sdScriptEntity, sdScriptEntity_Projectile )
	EVENT( EV_GetLaunchTime,		sdScriptEntity_Projectile::Event_GetLaunchTime )
	EVENT( EV_GetOwner,				sdScriptEntity_Projectile::Event_GetOwner )
END_CLASS

/*
============
sdScriptEntity_Projectile::Create
============
*/
void sdScriptEntity_Projectile::Create( idEntity* owner, const idVec3& start, const idVec3& dir ) {
	Unbind();

	this->owner = owner;
	SetGameTeam( owner->GetGameTeam() );

	idPhysics* physics = GetPhysics();
	idMat3 axis = dir.ToMat3();
	physics->SetOrigin( start );
	physics->SetAxis( axis );	
	physics->SetLinearVelocity( spawnArgs.GetVector( "velocity", "0 0 0" ) * axis );
	UpdateVisuals();

	launchTime = gameLocal.time;
}

/*
=================
sdScriptEntity_Projectile::Event_GetLaunchTime
=================
*/
void sdScriptEntity_Projectile::Event_GetLaunchTime( void ) {
	sdProgram::ReturnFloat( MS2SEC( launchTime ) ); 
}

/*
=================
sdScriptEntity_Projectile::Event_GetOwner
=================
*/
void sdScriptEntity_Projectile::Event_GetOwner( void ) {
	sdProgram::ReturnEntity( owner );
}


/*
================
sdScriptEntity_Projectile::CanCollide
================
*/
bool sdScriptEntity_Projectile::CanCollide( const idEntity* other, int traceId ) const {
	if( other == GetOwner() && traceId != TM_CROSSHAIR_INFO ) {
		return false;
	}

	return sdScriptEntity::CanCollide( other, traceId );
}

/*
================
sdScriptEntity_Projectile::ApplyNetworkState
================
*/
void sdScriptEntity_Projectile::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	sdScriptEntity::ApplyNetworkState( mode, newState );

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdScriptEntity_ProjectileBroadcastData );

		launchTime = newData.launchTime;
		owner.SetSpawnId( newData.owner );
		return;
	}
}

/*
================
sdScriptEntity_Projectile::ReadNetworkState
================
*/
void sdScriptEntity_Projectile::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, 
															sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	sdScriptEntity::ReadNetworkState( mode, baseState, newState, msg );
	
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdScriptEntity_ProjectileBroadcastData );

		newData.launchTime = msg.ReadDeltaLong( baseData.launchTime );
		newData.owner = msg.ReadDeltaLong( baseData.owner );
		return;
	}
}

/*
================
sdScriptEntity_Projectile::WriteNetworkState
================
*/
void sdScriptEntity_Projectile::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, 
																sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	sdScriptEntity::WriteNetworkState( mode, baseState, newState, msg );

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdScriptEntity_ProjectileBroadcastData );

		newData.launchTime = launchTime;
		newData.owner = owner.GetSpawnId();

		msg.WriteDeltaLong( baseData.launchTime, newData.launchTime );
		msg.WriteDeltaLong( baseData.owner, newData.owner );
		return;
	}
}

/*
================
sdScriptEntity_Projectile::CheckNetworkStateChanges
================
*/
bool sdScriptEntity_Projectile::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( sdScriptEntity::CheckNetworkStateChanges( mode, baseState ) ) {
		return true;
	}


	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdScriptEntity_ProjectileBroadcastData );

		NET_CHECK_FIELD( launchTime, launchTime );
		NET_CHECK_FIELD( owner, owner.GetSpawnId() );
		return false;
	}
	return false;
}

/*
================
sdScriptEntity_Projectile::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdScriptEntity_Projectile::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		sdScriptEntityBroadcastData* newData = new sdScriptEntity_ProjectileBroadcastData();
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
		return newData;
	}
	return sdScriptEntity::CreateNetworkStructure( mode );
}

/*
================
sdScriptEntity_ProjectileBroadcastData::MakeDefault
================
*/
void sdScriptEntity_ProjectileBroadcastData::MakeDefault( void ) {
	sdScriptEntityBroadcastData::MakeDefault();

	launchTime = 0;
	owner = 0;
}

/*
================
sdScriptEntity_ProjectileBroadcastData::Write
================
*/
void sdScriptEntity_ProjectileBroadcastData::Write( idFile* file ) const {
	sdScriptEntityBroadcastData::Write( file );

	file->WriteInt( launchTime );
	file->WriteInt( owner );
}

/*
================
sdScriptEntity_ProjectileBroadcastData::Read
================
*/
void sdScriptEntity_ProjectileBroadcastData::Read( idFile* file ) {
	sdScriptEntityBroadcastData::Read( file );

	file->ReadInt( launchTime );
	file->ReadInt( owner );
}
