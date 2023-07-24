// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PlayerBody.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../ContentMask.h"


/*
================
sdPlayerBodyNetworkData::~sdPlayerBodyNetworkData
================
*/
sdPlayerBodyNetworkData::~sdPlayerBodyNetworkData( void ) {
	delete physicsData;
}

/*
================
sdPlayerBodyNetworkData::MakeDefault
================
*/
void sdPlayerBodyNetworkData::MakeDefault( void ) {
	if ( physicsData ) {
		physicsData->MakeDefault();
	}
	scriptData.MakeDefault();
}

/*
================
sdPlayerBodyNetworkData::Write
================
*/
void sdPlayerBodyNetworkData::Write( idFile* file ) const {
	if ( physicsData ) {
		physicsData->Write( file );
	}
	scriptData.Write( file );
}

/*
================
sdPlayerBodyNetworkData::Read
================
*/
void sdPlayerBodyNetworkData::Read( idFile* file ) {
	if ( physicsData ) {
		physicsData->Read( file );
	}
	scriptData.Read( file );
}

/*
===============================================================================

	sdPlayerBodyInteractiveInterface

===============================================================================
*/

/*
==============
sdPlayerBodyInteractiveInterface::OnActivate
==============
*/
void sdPlayerBodyInteractiveInterface::Init( sdPlayerBody* owner ) {
	_owner				= owner;
	_activateFunc		= owner->GetScriptObject()->GetFunction( "OnActivate" );
	_activateHeldFunc	= owner->GetScriptObject()->GetFunction( "OnActivateHeld" );
}

/*
==============
sdPlayerBodyInteractiveInterface::OnActivate
==============
*/
bool sdPlayerBodyInteractiveInterface::OnActivate( idPlayer* player, float distance ) {
	if ( !_activateFunc ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	_owner->GetScriptObject()->CallNonBlockingScriptEvent( _activateFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
==============
sdPlayerBodyInteractiveInterface::OnActivateHeld
==============
*/
bool sdPlayerBodyInteractiveInterface::OnActivateHeld( idPlayer* player, float distance ) {
	if ( !_activateHeldFunc ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	_owner->GetScriptObject()->CallNonBlockingScriptEvent( _activateHeldFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
===============================================================================

	sdPlayerBody

===============================================================================
*/

extern const idEventDef EV_GetOwner;
extern const idEventDef EV_GetRenderViewAngles;

CLASS_DECLARATION( idAnimatedEntity, sdPlayerBody )
	EVENT( EV_GetOwner,				sdPlayerBody::Event_GetOwner )
	EVENT( EV_GetRenderViewAngles,	sdPlayerBody::Event_GetRenderViewAngles )
END_CLASS

/*
==============
sdPlayerBody::sdPlayerBody
==============
*/
sdPlayerBody::sdPlayerBody( void ) {
	client				= NULL;
	team				= NULL;
	updateCrosshairFunc	= NULL;
	rank				= NULL;
	creationTime		= gameLocal.time;
	viewYaw				= 0.f;
	viewAxis.Identity();

	isSpawnHostableFunc			= NULL;
	isSpawnHostFunc				= NULL;
	hasNoUniformFunc			= NULL;
	prePlayerFullyKilledFunc	= NULL;
}

/*
==============
sdPlayerBody::CanCollide
==============
*/
bool sdPlayerBody::CanCollide( const idEntity* other, int traceId ) const {
	if ( traceId == TM_THIRDPERSON_OFFSET ) {
		return false;
	}
	return idEntity::CanCollide( other, traceId );
}

/*
==============
sdPlayerBody::Spawn
==============
*/
void sdPlayerBody::Spawn( void ) {
	scriptObject = gameLocal.program->AllocScriptObject( this, "dead_body" );
	ConstructScriptObject();

	interactiveInterface.Init( this );

	updateCrosshairFunc = scriptObject->GetFunction( "OnUpdateCrosshairInfo" );

	isSpawnHostableFunc = scriptObject->GetFunction( "IsSpawnHostable" );
	isSpawnHostFunc = scriptObject->GetFunction( "IsSpawnHost" );
	hasNoUniformFunc = scriptObject->GetFunction( "HasNoUniform" );
	prePlayerFullyKilledFunc = scriptObject->GetFunction( "PrePlayerFullyKilled" );

	animator.RemoveOriginOffset( true );

	if ( !gameLocal.isClient ) {
		sdInstanceCollector< sdPlayerBody > bodies( false );
		for ( int i = 0; i < bodies.Num(); ) {
			if ( bodies[ i ]->creationTime == 0 ) {
				bodies.GetList().RemoveIndexFast( i );
			} else {
				i++;
			}
		}

		if ( bodies.Num() > MAX_PLAYERBODIES ) {
			bodies.GetList().Sort( SortByTime );

			bodies[ 0 ]->PostEventMS( &EV_Remove, 0 );
			bodies[ 0 ]->creationTime = 0;
		}
	}

	idBounds bounds;
	idPhysics_Player::CalcNormalBounds( bounds );
	idTraceModel trm( bounds );

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( trm, true ), 1.0f );
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	physicsObj.SetClipMask( MASK_PLAYERSOLID );
	physicsObj.DisableImpact();
	
	idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
	gravity *= g_gravity.GetFloat();		
	physicsObj.SetGravity( gravity );
	physicsObj.SetMaxStepHeight( 0 );
	physicsObj.SetContents( 0 );	

	SetPhysics( &physicsObj );

	BecomeActive( TH_THINK );	
}

/*
==============
sdPlayerBody::SetupBody
==============
*/
void sdPlayerBody::SetupBody( void ) {
	if ( playerClass.GetClass() == NULL ) {
		return;
	}

	const idDeclModelDef* model = playerClass.GetClass()->GetModel();
	const idDict& info			= playerClass.GetClass()->GetModelData();

	SetModel( model->GetName() );
	SetSkin( sdInventory::SkinForClass( playerClass.GetClass() ) );
	renderEntity.maxVisDist		= 5144;
	renderEntity.flags.noShadow = !g_showPlayerShadow.GetBool();

	SetCombatModel();
}

/*
==============
sdPlayerBody::Init
==============
*/
void sdPlayerBody::Init( idPlayer* _client, const sdPlayerClassSetup* _playerClass, sdTeamInfo* _team ) {
	assert( _playerClass );
	assert( _team );

	client			= _client;
	playerClass.SetClass( _playerClass->GetClass() );
	playerClass.SetOptions( _playerClass->GetOptions() );
	team			= _team;
	viewYaw			= client->viewAxis.ToAngles().yaw;
	idAngles::YawToMat3( viewYaw, viewAxis );

	rank			= client->GetProficiencyTable().GetRank();
	rating			= client->GetRating();

	SetupBody();

	GetPhysics()->SetOrigin( client->GetPhysics()->GetOrigin() );
	GetPhysics()->SetAxis( client->GetPhysics()->GetAxis() );

	idAnimBlend* torosBlend = client->GetAnimator()->CurrentAnim( ANIMCHANNEL_TORSO );
	torsoAnimNum = torosBlend->AnimNum();
	torsoAnimStartTime = torosBlend->GetStartTime();
	GetAnimator()->PlayAnim( ANIMCHANNEL_TORSO, torsoAnimNum, torsoAnimStartTime, 0 );

	idAnimBlend* legsBlend = client->GetAnimator()->CurrentAnim( ANIMCHANNEL_LEGS );
	legsAnimNum = legsBlend->AnimNum();
	legsAnimStartTime = legsBlend->GetStartTime();
	GetAnimator()->PlayAnim( ANIMCHANNEL_LEGS, legsAnimNum, legsAnimStartTime, 0 );

	animator.CreateFrame( gameLocal.time, true );

	// ensure that the combat model has the correct bounds 
	// fixes the case where people tapping out made spawn hosting & possess/disguise difficult
	animator.GetBounds( gameLocal.time, renderEntity.bounds, true );

	// ao: onfullykilled must not have been called on player
	if ( client.IsValid() ) {
		sdScriptHelper h1;
		h1.Push( client->GetScriptObject() );
		CallNonBlockingScriptEvent( prePlayerFullyKilledFunc, h1 );
	}

	UpdateVisuals();
	Present();

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_INIT );
		msg.WriteLong( client.GetSpawnId() );

		// write the class info
		msg.WriteLong( playerClass.GetClass()->Index() );

		sdTeamManager::GetInstance().WriteTeamToStream( team, msg );
		msg.WriteLong( torsoAnimNum );
		msg.WriteLong( torsoAnimStartTime );
		msg.WriteLong( legsAnimNum );
		msg.WriteLong( legsAnimStartTime );
		msg.WriteUShort( ANGLE2SHORT( viewYaw ) );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}

	if ( client->GetPhysics()->GetLinearVelocity().LengthSqr() < idMath::FLT_EPSILON && client->GetPhysics()->GetNumContacts() > 0 ) {
		GetPhysics()->PutToRest();
	}
}

/*
================
sdPlayerBody::ReadDemoBaseData
================
*/
void sdPlayerBody::ReadDemoBaseData( idFile* file ) {
	idEntity::ReadDemoBaseData( file );

	bool temp;
	file->ReadBool( temp );
	if ( temp ) {
		int clientSpawnId;
		file->ReadInt( clientSpawnId );

		int rankIndex;
		file->ReadInt( rankIndex );

		int ratingIndex;
		file->ReadInt( ratingIndex );

		int classIndex;
		file->ReadInt( classIndex );

		int numOptions;
		idList< int > classOptions;
		file->ReadInt( numOptions );
		classOptions.SetNum( numOptions );
		for ( int i = 0; i < numOptions; i++ ) {
			file->ReadInt( classOptions[ i ] );
		}

		sdTeamInfo* teamInfo = sdTeamManager::GetInstance().ReadTeamFromStream( file );

		int tAnimNum;
		file->ReadInt( tAnimNum );

		int tAnimStartTime;
		file->ReadInt( tAnimStartTime );

		int lAnimNum;
		file->ReadInt( lAnimNum );

		int lAnimStartTime;
		file->ReadInt( lAnimStartTime );

		float newViewYaw;
		file->ReadFloat( newViewYaw );

		Init( clientSpawnId, rankIndex, ratingIndex, classIndex, classOptions, teamInfo, tAnimNum, tAnimStartTime, lAnimNum, lAnimStartTime, newViewYaw );
	}
}

/*
================
sdPlayerBody::WriteDemoBaseData
================
*/
void sdPlayerBody::WriteDemoBaseData( idFile* file ) const {
	idEntity::WriteDemoBaseData( file );

	if ( playerClass.GetClass() == NULL ) {
		// not inited yet
		file->WriteBool( false );
	} else {
		file->WriteBool( true );

		file->WriteInt( client.GetSpawnId() );

		file->WriteInt( rank == NULL ? -1 : rank->Index() );
		file->WriteInt( rating == NULL ? -1 : rating->Index() );

		file->WriteInt( playerClass.GetClass()->Index() );

		const idList< int >& options = playerClass.GetOptions();

		file->WriteInt( options.Num() );
		for ( int i = 0; i < options.Num(); i++ ) {
			file->WriteInt( options[ i ] );
		}

		sdTeamManager::GetInstance().WriteTeamToStream( team, file );
		file->WriteInt( torsoAnimNum );
		file->WriteInt( torsoAnimStartTime );
		file->WriteInt( legsAnimNum );
		file->WriteInt( legsAnimStartTime );
		file->WriteFloat( viewYaw );
	}
}

/*
================
sdPlayerBody::GetPhysicsToVisualTransform
================
*/
bool sdPlayerBody::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin = vec3_origin;
	axis = viewAxis;
	return true;
}

/*
================
sdPlayerBody::UpdateCrosshairInfo
================
*/
bool sdPlayerBody::UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info ) {
	if ( !updateCrosshairFunc ) {
		return false;
	}

	crosshairInfo = &info;

	sdScriptHelper h1;
	h1.Push( player == NULL ? NULL : player->GetScriptObject() );
	scriptObject->CallNonBlockingScriptEvent( updateCrosshairFunc, h1 );

	crosshairInfo = NULL;

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
================
sdPlayerBody::Init
================
*/
void sdPlayerBody::Init( int clientSpawnId, int rankIndex, int ratingIndex, int classIndex, const idList< int >& classOptions, sdTeamInfo* teamInfo, int tAnimNum, int tAnimStartTime,
						int lAnimNum, int lAnimStartTime, float newViewYaw ) {
	client.ForceSpawnId( clientSpawnId );

	rank = gameLocal.declRankType.SafeIndex( rankIndex );
	rating = gameLocal.declRatingType.SafeIndex( ratingIndex );

	playerClass.SetClass( gameLocal.declPlayerClassType[ classIndex ] );
	for ( int i = 0; i < playerClass.GetOptions().Num(); i++ ) {
		playerClass.SetOption( i, classOptions[ i ] );
	}

	team = teamInfo;

	SetupBody();

	torsoAnimNum = tAnimNum;
	torsoAnimStartTime = tAnimStartTime;

	legsAnimNum = lAnimNum;
	legsAnimStartTime = lAnimStartTime;

	viewYaw = newViewYaw;
	idAngles::YawToMat3( viewYaw, viewAxis );

	GetAnimator()->PlayAnim( ANIMCHANNEL_TORSO, torsoAnimNum, torsoAnimStartTime, 0 );
	GetAnimator()->PlayAnim( ANIMCHANNEL_LEGS, legsAnimNum, legsAnimStartTime, 0 );

	animator.CreateFrame( gameLocal.time, true );

	// ensure that the combat model has the correct bounds 
	// fixes the case where people tapping out made spawn hosting & possess/disguise difficult
	animator.GetBounds( gameLocal.time, renderEntity.bounds, true );

	UpdateVisuals();
	Present();
}

/*
================
sdPlayerBody::GetDecalUsage
================
*/
cheapDecalUsage_t sdPlayerBody::GetDecalUsage( void ) {
	return CDU_INHIBIT;
}

/*
================
sdPlayerBody::ClientReceiveEvent
================
*/
bool sdPlayerBody::ClientReceiveEvent( int event, int time, const idBitMsg& msg ) {
	switch ( event ) {
		case EVENT_INIT: {
			int clientSpawnId = msg.ReadLong();
			int classIndex = msg.ReadLong();
			idList< int > classOptions;

			const sdDeclPlayerClass* cls = gameLocal.declPlayerClassType[ classIndex ];
			if ( cls != NULL ) {
				for ( int i = 0; i < cls->GetNumOptions(); i++ ) {
					classOptions.Alloc() = 0;
				}
			}

			sdTeamInfo* teamInfo = sdTeamManager::GetInstance().ReadTeamFromStream( msg );

			int tAnimNum = msg.ReadLong();
			int tAnimStartTime = msg.ReadLong();

			int lAnimNum = msg.ReadLong();
			int lAnimStartTime = msg.ReadLong();

			unsigned short newViewYaw = msg.ReadUShort();

			Init( clientSpawnId, -1, -1, classIndex, classOptions, teamInfo, tAnimNum, tAnimStartTime, lAnimNum, lAnimStartTime, SHORT2ANGLE( newViewYaw ) );

			return true;
		}
	}
	return false;
}

/*
================
sdPlayerBody::Event_GetOwner
================
*/
void sdPlayerBody::Event_GetOwner( void ) {
	sdProgram::ReturnEntity( client );
}

/*
================
sdPlayerBody::Event_GetRenderViewAngles
================
*/
void sdPlayerBody::Event_GetRenderViewAngles( void ) {
	idAngles renderAngles = viewAxis.ToAngles();
	sdProgram::ReturnVector( idVec3( renderAngles[ 0 ], renderAngles[ 1 ], renderAngles[ 2 ] ) );
}

/*
================
sdPlayerBody::SortByTime
================
*/
int sdPlayerBody::SortByTime( sdPlayerBody* bodyA, sdPlayerBody* bodyB ) {
	return bodyA->creationTime - bodyB->creationTime;
}

/*
============
sdPlayerBody::ApplyNetworkState
============
*/
void sdPlayerBody::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	NET_GET_NEW( sdPlayerBodyNetworkData );
	NET_APPLY_STATE_PHYSICS
	NET_APPLY_STATE_SCRIPT
}

/*
============
sdPlayerBody::ReadNetworkState
============
*/
void sdPlayerBody::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdPlayerBodyNetworkData );
	NET_READ_STATE_PHYSICS
	NET_READ_STATE_SCRIPT
}

/*
============
sdPlayerBody::WriteNetworkState
============
*/
void sdPlayerBody::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdPlayerBodyNetworkData );
	NET_WRITE_STATE_PHYSICS
	NET_WRITE_STATE_SCRIPT
}

/*
============
sdPlayerBody::CheckNetworkStateChanges
============
*/
bool sdPlayerBody::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	NET_GET_BASE( sdPlayerBodyNetworkData );
	NET_CHECK_STATE_PHYSICS
	NET_CHECK_STATE_SCRIPT

	return false;
}

/*
============
sdPlayerBody::CreateNetworkStructure
============
*/
sdEntityStateNetworkData* sdPlayerBody::CreateNetworkStructure( networkStateMode_t mode ) const {
	sdPlayerBodyNetworkData* newData = new sdPlayerBodyNetworkData();
	newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
	return newData;
}
