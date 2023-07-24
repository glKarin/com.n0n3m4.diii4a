// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DeployRequest.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../structures/DeployMask.h"

/*
===============================================================================

	sdDeployRequest

===============================================================================
*/

/*
==============
sdDeployRequest::sdDeployRequest
==============
*/
sdDeployRequest::sdDeployRequest( idFile* file ) : 
	object( NULL ), owner( NULL ), renderEntityHandle( -1 ), position( vec3_origin ), rotation( 0.f ), team( NULL ), callTime( -1 ) {

	file->ReadFloat( rotation );
	
	int ownerSpawnId;
	file->ReadInt( ownerSpawnId );
	owner.ForceSpawnId( ownerSpawnId );

	int objectIndex;
	file->ReadInt( objectIndex );
	object = gameLocal.declDeployableObjectType[ objectIndex ];

	file->ReadVec3( position );

	int teamIndex;
	file->ReadInt( teamIndex );
	team = teamIndex == -1 ? NULL : &sdTeamManager::GetInstance().GetTeamByIndex( teamIndex );

	if ( !team ) {
		gameLocal.Error( "sdDeployRequest::sdDeployRequest Player With No Team Requested a Deployable" );
	}

	SetupModel();
}

/*
==============
sdDeployRequest::sdDeployRequest
==============
*/
sdDeployRequest::sdDeployRequest( const sdDeclDeployableObject* _object, idPlayer* _owner, const idVec3& _position, float _rotation, sdTeamInfo* _team, int delayMS ) : 
	object( _object ), owner( _owner ), renderEntityHandle( -1 ), position( _position ), rotation( _rotation ), team( _team ), callTime( -1 ) {

	if ( !team ) {
		gameLocal.Error( "sdDeployRequest::sdDeployRequest Player With No Team Requested a Deployable" );
	}

	if ( !gameLocal.isClient ) {
		callTime = gameLocal.time + object->GetWaitTime() + delayMS;
	}

	SetupModel();
}

/*
==============
sdDeployRequest::~sdDeployRequest
==============
*/
sdDeployRequest::~sdDeployRequest( void ) {
	Hide();
	gameEdit->DestroyRenderEntity( renderEntity );
}

/*
==============
sdDeployRequest::SetupModel
==============
*/
void sdDeployRequest::SetupModel( void ) {
	if ( object->GetPlacementInfo() ) {
		gameEdit->ParseSpawnArgsToRenderEntity( object->GetPlacementInfo()->GetDict(), renderEntity );
	} else {
		memset( &renderEntity, 0, sizeof( renderEntity ) );
	}
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = gameLocal.time;

/*	sdDeployMask* mask = gameLocal.GetDeploymentMask( object->GetDeploymentMask() );
	if ( !mask ) {
		gameLocal.Error( "sdDeployRequest::SetupModel Missing Deploy Mask" );
	}*/

	bounds.Clear();
	bounds.AddPoint( position );
	bounds.ExpandSelf( object->GetObjectSize() );
//	mask->ExpandToGrid( bounds );
}

/*
==============
sdDeployRequest::Show
==============
*/
void sdDeployRequest::UpdateRenderEntity( renderEntity_t& renderEntity, const idVec4& color, const idVec3& position ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= color[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= color[ 2 ];

	renderEntity.axis.Identity();
	renderEntity.origin = position;
	renderEntity.origin[ 2 ] += 32.f;
}

/*
==============
sdDeployRequest::Show
==============
*/
void sdDeployRequest::Show( void ) {
	if ( renderEntityHandle < 0 && renderEntity.hModel ) {
		renderEntityHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}
}

/*
==============
sdDeployRequest::Show
==============
*/
void sdDeployRequest::Hide( void ) {
	if ( gameRenderWorld && renderEntityHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( renderEntityHandle );
		renderEntityHandle = -1;
	}
}

/*
==============
sdDeployRequest::CheckBlock
==============
*/
bool sdDeployRequest::CheckBlock( const idBounds& _bounds ) {
	return bounds.IntersectsBounds( _bounds );
}

/*
==============
sdDeployRequest::Show
==============
*/
bool sdDeployRequest::Update( idPlayer* player ) {
	bool show = false;

	if ( player ) {
		sdTeamInfo* localTeam = player->GetGameTeam();
		show = localTeam && *localTeam == *team;

		if ( show ) {
			Show();

			if ( renderEntityHandle != -1 ) {
				UpdateRenderEntity( renderEntity, colorLtGrey, position );
				idAngles::YawToMat3( rotation, renderEntity.axis );
				gameRenderWorld->UpdateEntityDef( renderEntityHandle, &renderEntity );
			}
		} else {
			Hide();
		}
	}

	if ( callTime != -1 && gameLocal.time > callTime ) {
		if ( !CallForDropOff() ) {
			return false;
		}
		callTime = -1;
	}

	return true;
}

/*
==============
sdDeployRequest::CallForDropOff
==============
*/
bool sdDeployRequest::CallForDropOff( void ) {
	if ( !owner.IsValid() ) {
		gameLocal.Warning( "sdDeployRequest::CallForDropOff Owner No Longer Exists" );
		return false;
	}

	const idDeclEntityDef* deployableInfo = object->GetCarrierDef();

	idEntity* other = NULL;
	if ( !gameLocal.SpawnEntityDef( deployableInfo->dict, true, &other ) ) {
		gameLocal.Error( "sdDeployRequest::sdDeployRequest Could not Spawn Deployable Transport" );
	}

	other->SetGameTeam( team );

	sdScriptHelper helper;
	helper.Push( object->GetEntityDef()->Index() );
	helper.Push( owner->entityNumber );
	helper.Push( position );
	helper.Push( rotation );
	other->CallNonBlockingScriptEvent( other->scriptObject->GetFunction( "OnSetDeploymentParms" ), helper );

	return true;
}

/*
==============
sdDeployRequest::WriteCreateEvent
==============
*/
void sdDeployRequest::WriteCreateEvent( int index, const sdReliableMessageClientInfoBase& info ) const {
	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_CREATEDEPLOYREQUEST );
	msg.WriteLong( index );
	msg.WriteFloat( rotation );
	msg.WriteLong( owner.GetSpawnId() );
	msg.WriteBits( object->Index() + 1, gameLocal.GetNumDeployObjectBits() );
	msg.WriteVector( position );
	sdTeamManager::GetInstance().WriteTeamToStream( team, msg );
	msg.Send( info );
}

/*
==============
sdDeployRequest::Write
==============
*/
void sdDeployRequest::Write( idFile* file ) const {
	file->WriteFloat( rotation );
	file->WriteInt( owner.GetSpawnId() );
	file->WriteInt( object->Index() );
	file->WriteVec3( position );
	file->WriteInt( team ? team->GetIndex() : -1 );
}

/*
==============
sdDeployRequest::WriteDestroyEvent
==============
*/
void sdDeployRequest::WriteDestroyEvent( int index ) const {
	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_DELETEDEPLOYREQUEST );
	msg.WriteLong( index );
	msg.Send( sdReliableMessageClientInfoAll() );
}
