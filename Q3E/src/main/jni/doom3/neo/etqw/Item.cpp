// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Item.h"
#include "Player.h"
#include "ContentMask.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "vehicles/Transport.h"

#include "botai/Bot.h"
#include "botai/BotThreadData.h"

/*
===============================================================================

  idItem

===============================================================================
*/

extern const idEventDef EV_GetOwner;

CLASS_DECLARATION( idEntity, idItem )
	EVENT( EV_GetOwner,			idItem::Event_GetOwner )
END_CLASS


/*
================
idItem::idItem
================
*/
idItem::idItem( void ) : pickUpTime( 0 ), team( NULL ) {
}

/*
================
idItem::~idItem
================
*/
idItem::~idItem( void ) {
//mal: see who owned this item, and let its owner know that we have been removed from the world.
	idPlayer* player = dropper->Cast< idPlayer >();

	if ( player != NULL ) {
		player->Event_SetPlayerItemState( this, true );
	}
}

/*
================
idItem::Spawn
================
*/
void idItem::Spawn( void ) {
	float tsize;
	if ( spawnArgs.GetFloat( "triggersize", "0", tsize ) ) {
		idClipModel* clipModel = new idClipModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ), true );
		GetPhysics()->SetClipModel( clipModel, 1.0f );
		clipModel->Link( gameLocal.clip );
	}

	GetPhysics()->SetContents( CONTENTS_TRIGGER );

	BecomeActive( TH_THINK );

	requirements.Load( spawnArgs, "require_pickup" );

	const char* packageName = spawnArgs.GetString( "pck_items", "" );
	if ( *packageName != '\0' ) {
		package = gameLocal.declItemPackageType[ packageName ];
		if ( package == NULL ) {
			gameLocal.Error( "idItem::Spawn '%s' With Invalid Package Set", GetClassname() );
		}
	} else {
		gameLocal.Error( "idItem::Spawn '%s' With No Package Set", GetClassname() );
	}

	onPrePickupFunction	= scriptObject->GetFunction( "OnPrePickup" );
	onPickupFunction	= scriptObject->GetFunction( "OnPickup" );

	fl.unlockInterpolate = true;
}

/*
================
idItem::GiveToPlayer
================
*/
bool idItem::GiveToPlayer( idPlayer *player ) {
	if ( !requirements.Check( player, this ) ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player ? player->GetScriptObject() : NULL );
	scriptObject->CallNonBlockingScriptEvent( onPrePickupFunction, h1 );

	return player->GivePackage( package );
}

/*
================
idItem::Pickup
================
*/
bool idItem::Pickup( idPlayer *player ) {
	
	if ( !GiveToPlayer( player ) ) {
		return false;
	}

	OnPickup( player );

	return true;
}

/*
================
idItem::Pickup
================
*/
void idItem::OnPickup( idPlayer *player ) {
	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_PICKUP );
		msg.WriteLong( gameLocal.GetSpawnId( player ) );
		msg.Send( false, sdReliableMessageClientInfoAll() );
	}

	int length = 0;
	
	// play pickup sound
	StartSound( "snd_acquire", SND_ITEM, 0, &length );

	if ( !gameLocal.isClient ) {
		PostEventMS( &EV_Remove, length + SEC2MS( 1.f ) );
	}

	// clear our contents so the object isn't picked up twice
	GetPhysics()->SetContents( 0, 0 );
	Hide();

	sdScriptHelper h1;
	h1.Push( player ? player->GetScriptObject() : NULL );
	scriptObject->CallNonBlockingScriptEvent( onPickupFunction, h1 );

	idPlayer* owner = dropper->Cast< idPlayer >();

//mal: thank the player for the goodies, only if they're close, they're not the same class as us, and they're alive.
    if ( owner ) {
		if ( owner != player && botThreadData.GetGameWorldState()->clientInfo[ owner->entityNumber ].classType != botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].classType ) { //mal: dont thank ourselves, or the same class giving us ammo/health
			botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].myHero = owner->entityNumber;
            if ( botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].isBot ) {
				if ( owner->GetHealth() > 0 && !owner->IsType( idBot::Type ) ) { //mal: dont thank the player if hes dead! Only thank humans.
                    idVec3 vec;
					vec = owner->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin();
					if ( vec.LengthSqr() < Square( 500.0f ) ) {
						botThreadData.VOChat( THANKS, player->entityNumber, false );
					}
				}
			}
		}
	}
}

/*
================
idItem::ClientReceiveEvent
================
*/
bool idItem::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_PICKUP: {
			idPlayer* other = gameLocal.EntityForSpawnId( msg.ReadLong() )->Cast< idPlayer >();
			OnPickup( other );
			return true;
		}
		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
	return false;
}

/*
================
idItem::OnTouch
================
*/
void idItem::OnTouch( idEntity *other, const trace_t& trace ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( player == NULL ) {
		return;
	}

	if ( !player->IsSpectator() && player->GetHealth() <= 0 ) {
		return;
	}

	if ( gameLocal.time < pickUpTime && dropper == other ) {
		return;
	}

	botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].touchingItemTime = gameLocal.GetTime();

	idEntity* proxy = player->GetProxyEntity();
	if ( proxy != NULL ) {
		if ( !proxy->GetUsableInterface()->GetAllowPlayerDamage( player ) ) {
			return;
		}
	}

	Pickup( player );
}

/*
================
idItem::Event_GetOwner
================
*/
void idItem::Event_GetOwner( void ) {
	sdProgram::ReturnEntity( dropper );
}









 /*
 ================
 sdMoveableItemNetworkData::~sdMoveableItemNetworkData
 ================
 */
 sdMoveableItemNetworkData::~sdMoveableItemNetworkData( void ) {
	 delete physicsData;
}

/*
================
sdMoveableItemNetworkData::MakeDefault
================
*/
void sdMoveableItemNetworkData::MakeDefault( void ) {
	if ( physicsData ) {
		physicsData->MakeDefault();
	}
}

/*
================
sdMoveableItemNetworkData::Write
================
*/
void sdMoveableItemNetworkData::Write( idFile* file ) const {
	if ( physicsData ) {
		physicsData->Write( file );
	}
}

/*
================
sdMoveableItemNetworkData::Read
================
*/
void sdMoveableItemNetworkData::Read( idFile* file ) {
	if ( physicsData ) {
		physicsData->Read( file );
	}
}


/*
===============================================================================

  idMoveableItem
	
===============================================================================
*/

CLASS_DECLARATION( idItem, idMoveableItem )
END_CLASS

/*
================
idMoveableItem::idMoveableItem
================
*/
idMoveableItem::idMoveableItem() {
	trigger = NULL;
	waterEffects = NULL;
}

/*
================
idMoveableItem::~idMoveableItem
================
*/
idMoveableItem::~idMoveableItem() {
	delete waterEffects;
	gameLocal.clip.DeleteClipModel( trigger );
}

/*
================
idMoveableItem::Spawn
================
*/
void idMoveableItem::Spawn( void ) {
	// create a trigger for item pickup
	float tsize;
	spawnArgs.GetFloat( "triggersize", "16.0", tsize );
	trigger = new idClipModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ), true );
	trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	trigger->SetContents( CONTENTS_TRIGGER );

	idClipModel* model = GetPhysics()->GetClipModel();
	const idTraceModel* trm = NULL;
	if ( !model->IsTraceModel() ) {
		const char* clipModelName = GetClipModelName();
		model->GetTraceModel();

		// load the trace model
		idTraceModel newTrm;
		if ( !gameLocal.clip.LoadTraceModel( clipModelName, newTrm ) ) {
			gameLocal.Error( "idMoveableItem '%s': cannot load collision model %s", name.c_str(), clipModelName );
			return;
		}

		// if the model should be shrinked
		if ( spawnArgs.GetBool( "clipshrink" ) ) {
			newTrm.Shrink( CM_CLIP_EPSILON );
		}

		trm = &newTrm;
	} else {
		trm = model->GetTraceModel();
	}

	// get rigid body properties
	float density;
	spawnArgs.GetFloat( "density", "0.5", density );
	density = idMath::ClampFloat( 0.001f, 1000.0f, density );

	float friction;
	spawnArgs.GetFloat( "friction", "0.05", friction );
	friction = idMath::ClampFloat( 0.0f, 1.0f, friction );

	float bouncyness;
	spawnArgs.GetFloat( "bouncyness", "0.6", bouncyness );
	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );

	float linearFriction;
	spawnArgs.GetFloat( "linear_friction", "0.6", linearFriction );
	linearFriction = idMath::ClampFloat( 0.0f, 1.0f, linearFriction );

	float angularFriction;
	spawnArgs.GetFloat( "angular_friction", "0.6", angularFriction );
	angularFriction = idMath::ClampFloat( 0.0f, 1.0f, angularFriction );

	float buoyancy;
	spawnArgs.GetFloat( "buoyancy", "0.6", buoyancy );
	buoyancy = idMath::ClampFloat( 0.0f, 1.0f, buoyancy );

	waterEffects = sdWaterEffects::SetupFromSpawnArgs( spawnArgs );

	// setup the physics
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( *trm, false ), density, 0 );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction( linearFriction, angularFriction, friction );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_RENDERMODEL, 0 );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP | CONTENTS_FORCEFIELD, 0 );
	physicsObj.SetBuoyancy( buoyancy );
	physicsObj.DisableImpact();
	SetPhysics( &physicsObj );
}

/*
================
idMoveableItem::Think
================
*/
void idMoveableItem::Think( void ) {

	if ( RunPhysics() ) {
		// update trigger position
		trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );
	}
	
	Present();
}

/*
================
idMoveableItem::OnPickup
================
*/
void idMoveableItem::OnPickup( idPlayer *player ) {
	trigger->SetContents( 0 );

	idItem::OnPickup( player );
}

/*
================
idMoveableItem::Collide
================
*/
bool idMoveableItem::Collide( const trace_t& collision, const idVec3& velocity, int bodyId ) {
	// don't need to do any sticking or anything here
	// any more - SimpleRigidBody takes care of it!
	return false;
}

/*
============
idMoveableItem::ApplyNetworkState
============
*/
void idMoveableItem::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	NET_GET_NEW( sdMoveableItemNetworkData );
	NET_APPLY_STATE_PHYSICS;
}

/*
============
idMoveableItem::ReadNetworkState
============
*/
void idMoveableItem::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdMoveableItemNetworkData );
	NET_READ_STATE_PHYSICS
}

/*
============
idMoveableItem::WriteNetworkState
============
*/
void idMoveableItem::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdMoveableItemNetworkData );
	NET_WRITE_STATE_PHYSICS
}

/*
============
idMoveableItem::CheckNetworkStateChanges
============
*/
bool idMoveableItem::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	NET_GET_BASE( sdMoveableItemNetworkData );
	NET_CHECK_STATE_PHYSICS

	return false;
}

/*
============
idMoveableItem::CreateNetworkStructure
============
*/
sdEntityStateNetworkData* idMoveableItem::CreateNetworkStructure( networkStateMode_t mode ) const {
	sdMoveableItemNetworkData* newData = new sdMoveableItemNetworkData();
	newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
	return newData;
}

/*
================
idMoveableItem::CanCollide
================
*/
bool idMoveableItem::CanCollide( const idEntity* other, int traceId ) const {
	if ( other->IsType( idPlayer::Type ) || other->IsType( sdTransport::Type ) || other->IsType( idMoveableItem::Type ) ) {
		return false;
	}
	return idEntity::CanCollide( other, traceId );
}

/*
================
idMoveableItem::CheckWater
================
*/
void idMoveableItem::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects ) {
		waterEffects->SetOrigin( GetPhysics()->GetOrigin() );
		waterEffects->SetAxis( GetPhysics()->GetAxis() );
		waterEffects->SetVelocity( GetPhysics()->GetLinearVelocity() );		
		waterEffects->CheckWater( this, waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}
