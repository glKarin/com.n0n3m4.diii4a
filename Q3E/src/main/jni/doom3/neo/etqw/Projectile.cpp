// Copyright (C) 2007 Id Software, Inc.
//
 
#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Projectile.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "Player.h"
#include "ContentMask.h"


/*
================
sdProjectileNetworkData::~sdProjectileNetworkData
================
*/
sdProjectileNetworkData::~sdProjectileNetworkData( void ) {
	delete physicsData;
}

/*
================
sdProjectileNetworkData::MakeDefault
================
*/
void sdProjectileNetworkData::MakeDefault( void ) {
	scriptData.MakeDefault();
	if ( physicsData ) {
		physicsData->MakeDefault();
	}
}

/*
================
sdProjectileNetworkData::Write
================
*/
void sdProjectileNetworkData::Write( idFile* file ) const {
	if ( physicsData ) {
		physicsData->Write( file );
	}

	scriptData.Write( file );
}

/*
================
sdProjectileNetworkData::Read
================
*/
void sdProjectileNetworkData::Read( idFile* file ) {
	if ( physicsData ) {
		physicsData->Read( file );
	}

	scriptData.Read( file );
}

/*
================
sdProjectileBroadcastData::~sdProjectileBroadcastData
================
*/
sdProjectileBroadcastData::~sdProjectileBroadcastData( void ) {
	delete physicsData;
}

/*
================
sdProjectileBroadcastData::MakeDefault
================
*/
void sdProjectileBroadcastData::MakeDefault( void ) {
	scriptData.MakeDefault();
	if ( physicsData ) {
		physicsData->MakeDefault();
	}

	team			= NULL;
	ownerId			= 0;
	enemyId			= 0;
	launchTime		= 0;
	launchSpeed		= 0.f;
	owners.Clear();
	hidden			= false;
	bindData.MakeDefault();
}

/*
================
sdProjectileBroadcastData::Write
================
*/
void sdProjectileBroadcastData::Write( idFile* file ) const {
	if ( physicsData ) {
		physicsData->Write( file );
	}

	scriptData.Write( file );

	file->WriteInt( team ? team->GetIndex() : -1 );
	file->WriteInt( ownerId );
	file->WriteInt( enemyId );
	file->WriteInt( launchTime );
	file->WriteFloat( launchSpeed );
	file->WriteInt( owners.Num() );
	for ( int i = 0; i < owners.Num(); i++ ) {
		file->WriteInt( owners[ i ] );
	}
	file->WriteBool( hidden );
	bindData.Write( file );
}

/*
================
sdProjectileBroadcastData::Read
================
*/
void sdProjectileBroadcastData::Read( idFile* file ) {
	if ( physicsData ) {
		physicsData->Read( file );
	}

	scriptData.Read( file );

	int teamIndex;
	file->ReadInt( teamIndex );
	team = teamIndex == -1 ? NULL : &sdTeamManager::GetInstance().GetTeamByIndex( teamIndex );

	file->ReadInt( ownerId );
	file->ReadInt( enemyId );
	file->ReadInt( launchTime );
	file->ReadFloat( launchSpeed );

	int count;
	file->ReadInt( count );
	owners.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		file->ReadInt( owners[ i ] );
	}
	file->ReadBool( hidden );
	bindData.Read( file );
}

/*
===============================================================================

  idProjectile
	
===============================================================================
*/

extern const idEventDef EV_SetOwner;
extern const idEventDef EV_AddOwner;
extern const idEventDef EV_Launch;
extern const idEventDef EV_SetEnemy;
extern const idEventDef EV_GetEnemy;
extern const idEventDef EV_Freeze;
extern const idEventDef EV_SetState;
extern const idEventDef EV_GetOwner;
extern const idEventDef EV_GetLaunchTime;

const idEventDef EV_GetDamagePower( "getDamagePower", 'f', DOC_TEXT( "Returns the projectile's damage multiplier." ), 0, NULL );
const idEventDef EV_SetOwner( "setOwner", '\0', DOC_TEXT( "Sets the entity which owns the owner." ), 1, "The owner of a projectile cannot collide with it, and vice versa.", "E", "owner", "The new owner to set." );
const idEventDef EV_GetLaunchTime( "getLaunchTime", 'f', DOC_TEXT( "Returns the game time in seconds at which the projectile was launched." ), 0, "The result will be 0 if the projectile has not yet been fired." );
const idEventDef EV_Launch( "launch", '\0', DOC_TEXT( "Fires the projectile with the given world space velocity." ), 1, NULL, "v", "velocity", "The velocity to fire the projectile with." );
const idEventDef EV_AddOwner( "addOwner", '\0', DOC_TEXT( "Adds an entity to the list of owners for the projectile." ), 1, "Owners cannot collide with the projectile, or vice versa.", "e", "owner", "New owner to add." );
const idEventDef EV_SetEnemy( "setEnemy", '\0', DOC_TEXT( "Sets the target entity for this object." ), 1, NULL, "E", "target", "New target to set." );
const idEventDef EV_GetEnemy( "getEnemy", 'e', DOC_TEXT( "Returns the entity's current target." ), 0, NULL );
const idEventDef EV_IsOwner( "isOwner", 'b', DOC_TEXT( "Returns whether the given entity is an owner of this projectile." ), 1, NULL, "e", "owner", "Entity to check." );

CLASS_DECLARATION( idEntity, idProjectile )
	EVENT( EV_SetState,			idProjectile::Event_SetState )
	EVENT( EV_GetDamagePower,	idProjectile::Event_GetDamagePower )
	EVENT( EV_GetOwner,			idProjectile::Event_GetOwner )
	EVENT( EV_SetOwner,			idProjectile::Event_SetOwner )
	EVENT( EV_GetLaunchTime,	idProjectile::Event_GetLaunchTime )
	EVENT( EV_Launch,			idProjectile::Event_Launch )
	EVENT( EV_AddOwner,			idProjectile::Event_AddOwner )
	EVENT( EV_SetEnemy,			idProjectile::Event_SetEnemy )
	EVENT( EV_GetEnemy,			idProjectile::Event_GetEnemy )
	EVENT( EV_IsOwner,			idProjectile::Event_IsOwner )
	EVENT( EV_Hide,				idProjectile::Event_Hide )
	EVENT( EV_Show,				idProjectile::Event_Show )
	EVENT( EV_Freeze,			idProjectile::Event_Freeze )
END_CLASS

/*
================
idProjectile::idProjectile
================
*/
idProjectile::idProjectile( void ) {
	owner					= NULL;

	damagePower				= 1.0f;

	launchTime				= 0;
	thrustStartTime			= 0;
	targetForgetTime		= 0;
	launchSpeed				= 0.f;
	health					= 0;

	team					= NULL;

	baseScriptThread		= NULL;
	scriptIdealState		= NULL;
	scriptState				= NULL;

	projectileFlags.scriptHide				= false;
	projectileFlags.frozen					= false;
	projectileFlags.allowOwnerCollisions	= false;
	projectileFlags.hasThrust				= false;
	projectileFlags.thustOn					= false;
	projectileFlags.faceVelocity			= false;
	projectileFlags.faceVelocitySet			= false;

	postThinkEntNode.SetOwner( this );
}

/*
=================
idProjectile::~idProjectile
=================
*/
idProjectile::~idProjectile( void ) {
	
	DeconstructScriptObject();

	if ( baseScriptThread != NULL ) {
		gameLocal.program->FreeThread( baseScriptThread );
		baseScriptThread = NULL;
	}
}

/*
================
idProjectile::Spawn
================
*/
void idProjectile::Spawn( void ) {
	projectileFlags.allowOwnerCollisions	= spawnArgs.GetBool( "owner_collisions" );
	projectileFlags.hasThrust				= spawnArgs.GetBool( "has_thrust" );
	projectileFlags.faceVelocity			= spawnArgs.GetBool( "face_velocity" );
	
	BecomeActive( TH_THINK );

	maxHealth = health = spawnArgs.GetInt( "health" );

	thrustPower			= spawnArgs.GetFloat( "thrust_power", "7500" );

	scriptObject = gameLocal.program->AllocScriptObject( this, spawnArgs.GetString( "scriptobject", "default" ) );
	baseScriptThread = ConstructScriptObject();
	if ( baseScriptThread != NULL ) {
		baseScriptThread->ManualControl();
		baseScriptThread->ManualDelete();
	}

	targetingFunction	= scriptObject->GetFunction( "OnUpdateTargeting" );

	if ( spawnArgs.GetBool( "has_postthink" ) ) {
		onPostThink			= scriptObject->GetFunction( "OnPostThink" );
		if ( onPostThink != NULL ) {
			postThinkEntNode.AddToEnd( gameLocal.postThinkEntities );
		}
	}

	fl.unlockInterpolate = true;

	InitPhysics();
}

/*
================
idProjectile::Event_Hide
================
*/
void idProjectile::Event_Hide( void ) {
	projectileFlags.scriptHide = true;
}

/*
================
idProjectile::Event_Show
================
*/
void idProjectile::Event_Show( void ) {
	projectileFlags.scriptHide = false;
}

/*
================
idProjectile::Event_Freeze
================
*/
void idProjectile::Event_Freeze( bool frozen ) {
	projectileFlags.frozen = frozen;
}

/*
================
idProjectile::CanCollide
================
*/
bool idProjectile::CanCollide( const idEntity* other, int traceId ) const {
	if ( !projectileFlags.allowOwnerCollisions && traceId != TM_CROSSHAIR_INFO ) {
		if ( other == owner ) {
			return false;
		}
		for ( int i = 0; i < owners.Num(); i++ ) {
			if ( owners[ i ].GetEntity() == other ) {
				return false;
			}
		}
	}
	return idEntity::CanCollide( other, traceId );
}

/*
================
idProjectile::Create
================
*/
void idProjectile::Create( idEntity* _owner, const idVec3 &start, const idVec3 &dir ) {
	Unbind();

	owner = _owner;

	if ( owner ) {
		SetGameTeam( owner->GetGameTeam() );
	}
	GetPhysics()->SetOrigin( start );
	GetPhysics()->SetAxis( dir.ToMat3() );

	UpdateVisuals();
}

/*
=================
idProjectile::InitPhysics
=================
*/
void idProjectile::InitPhysics( void ) {
}

/*
=================
idProjectile::InitLaunchPhysics
=================
*/
void idProjectile::InitLaunchPhysics( float launchPower, const idVec3& origin, const idMat3& axes, const idVec3& pushVelocity ) {
}

/*
=================
idProjectile::Event_GetLaunchTime
=================
*/
void idProjectile::Event_GetLaunchTime( void ) {
	sdProgram::ReturnFloat( MS2SEC( launchTime ) ); 
}

/*
================
idProjectile::Event_Launch
================
*/
void idProjectile::Event_Launch( const idVec3& velocity ) {
	idVec3 org = GetPhysics()->GetOrigin();
	idVec3 dir = velocity;
	dir.Normalize();
	Launch( org, dir, velocity, 0, 0, 1.f );
}

/*
=================
idProjectile::Launch
=================
*/
void idProjectile::Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	damagePower = dmgPower;
	SetLaunchTime( gameLocal.time - SEC2MS( timeSinceFire ) );

	if ( health ) {
		fl.takedamage = true;
	}

	Unbind();

	idMat3 axes = dir.ToMat3();

	InitLaunchPhysics( launchPower, start, axes, pushVelocity );
	launchSpeed = GetPhysics()->GetLinearVelocity().Length();

	GetPhysics()->SetOrigin( start );
	GetPhysics()->SetAxis( axes );
	UpdateVisuals();
	Present();
	
	idAngles tracerMuzzleAngles = axes.ToAngles();
	idVec3 anglesVec( tracerMuzzleAngles.pitch, tracerMuzzleAngles.yaw, tracerMuzzleAngles.roll );

	sdScriptHelper helper;
	helper.Push( start );
	helper.Push( anglesVec );
	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnLaunch" ), helper );
}

/*
================
idProjectile::SetEnemy
================
*/
void idProjectile::SetEnemy( idEntity* newEnemy ) {
	if ( enemy == newEnemy ) {
		return;
	}

	sdScriptHelper helper;
	helper.Push( enemy ? enemy->GetScriptObject() : NULL );
	helper.Push( newEnemy ? newEnemy->GetScriptObject() : NULL );

	enemy = newEnemy;

	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnSetEnemy" ), helper );
}

/*
================
idProjectile::Event_AddOwner
================
*/
void idProjectile::Event_AddOwner( idEntity* other ) {
	AddOwner( other );
}

/*
================
idProjectile::Event_GetEnemy
================
*/
void idProjectile::Event_GetEnemy( void ) {
	sdProgram::ReturnEntity( enemy );
}

/*
================
idProjectile::Event_SetEnemy
================
*/
void idProjectile::Event_SetEnemy( idEntity* other ) {
	CancelEvents( &EV_SetEnemy );	

	SetEnemy( other );
}

/*
================
idProjectile::Event_IsOwner
================
*/
void idProjectile::Event_IsOwner( idEntity* other ) {
	sdProgram::ReturnBoolean( IsOwner( other ) );
}

/*
================
idProjectile::GetOwner
================
*/
idEntity *idProjectile::GetOwner( void ) const {
	return owner.GetEntity();
}

/*
================
idProjectile::IsOwner
================
*/
bool idProjectile::IsOwner( idEntity* other ) const {
	if ( other == owner ) {		
		return true;
	}

	for ( int i = 0; i < owners.Num(); i++ ) {
		if ( other == owners[ i ] ) {
			return true;
		}
	}

	return false;
}


/*
================
idProjectile::Thrust
================
*/
void idProjectile::Thrust( void ) {
	idVec3 vel = GetPhysics()->GetLinearVelocity();
	vel += ( thrustPower * MS2SEC( gameLocal.msec ) ) * GetPhysics()->GetAxis()[ 0 ];	// apply thrust ( adjusted to per second )
	vel.Truncate( launchSpeed );

	GetPhysics()->SetLinearVelocity( vel );
}

/*
================
idProjectile::UpdateTargeting
================
*/
void idProjectile::UpdateTargeting( void ) {
	if ( !targetingFunction ) {
		return;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( targetingFunction, h1 );
}

/*
================
idProjectile::SetThrust
================
*/
void idProjectile::SetThrust( bool value ) {
	if ( projectileFlags.thustOn == value ) {
		return;
	}

	projectileFlags.thustOn = value;

	if ( projectileFlags.thustOn ) {		
		scriptObject->CallEvent( "OnThrustStarted" );
	} else {
		scriptObject->CallEvent( "OnThrustStopped" );
	}
}

/*
================
idProjectile::Think
================
*/
void idProjectile::Think( void ) {

	if ( gameLocal.isNewFrame ) {
		UpdateScript();
	}

	if ( launchTime != 0 && !projectileFlags.frozen ) {
		if ( gameLocal.isNewFrame ) {
			if ( projectileFlags.faceVelocity ) {
				idVec3 vel = GetPhysics()->GetLinearVelocity();
				float length = vel.Normalize();
				if ( length > 0.75 ) {
					projectileFlags.faceVelocitySet = true;
					visualAxes = vel.ToMat3();
				}
			}
		}

		if ( projectileFlags.hasThrust && gameLocal.time >= thrustStartTime ) {
			SetThrust( true );
			
			UpdateTargeting();

			Thrust();
		}

		RunPhysics();
	}

	if ( gameLocal.isNewFrame ) {
		UpdateVisibility();
		Present();
	}
}

/*
================
idProjectile::PostThink
================
*/
void idProjectile::PostThink( void ) {
	if( onPostThink != NULL ) {
		sdScriptHelper helper;
		CallNonBlockingScriptEvent( onPostThink, helper );
	}
}

/*
================
idProjectile::GetPostThinkNode
================
*/
idLinkList< idEntity >* idProjectile::GetPostThinkNode( void ) {
	return &postThinkEntNode;
}

/*
================
idProjectile::UpdateVisibility
================
*/
void idProjectile::UpdateVisibility( void ) {
	bool hide = projectileFlags.scriptHide;	

	if ( hide ) {
		Hide();
	} else {
		Show();
	}
}

/*
=====================
idProjectile::UpdateScript
=====================
*/
void idProjectile::UpdateScript( void ) {
	if ( !baseScriptThread || gameLocal.IsPaused() ) {
		return;
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for( int i = 0; i < 20; i++ ) {
		if ( scriptIdealState != scriptState ) {
			SetState( scriptIdealState );
		}

		// don't call script until it's done waiting
		if ( baseScriptThread->IsWaiting() ) {
			return;
		}

		baseScriptThread->Execute();
		if ( scriptIdealState == scriptState ) {
			return;
		}
	}

	baseScriptThread->Warning( "idProjectile::UpdateScript Exited Loop to Prevent Lockup" );
}

/*
=====================
idProjectile::SetState
=====================
*/
bool idProjectile::SetState( const sdProgram::sdFunction* newState ) {
	if ( !newState ) {
		gameLocal.Error( "idProjectile::SetState NULL state" );
	}

	scriptState = newState;
	scriptIdealState = scriptState;

	baseScriptThread->CallFunction( scriptObject, scriptState );

	return true;
}

/*
================
idProjectile::Event_SetState
================
*/
void idProjectile::Event_SetState( const char* stateName ) {
	const sdProgram::sdFunction* func = scriptObject->GetFunction( stateName );
	if ( !func ) {
		gameLocal.Error( "idProjectile::Event_SetState Can't find function '%s' in object '%s'", stateName, scriptObject->GetTypeName() );
	}

	scriptIdealState = func;
	baseScriptThread->DoneProcessing();
}

/*
================
idProjectile::Event_GetDamagePower
================
*/
void idProjectile::Event_GetDamagePower( void ) {
	sdProgram::ReturnFloat( damagePower );
}

/*
================
idProjectile::Event_GetOwner
================
*/
void idProjectile::Event_GetOwner( void ) {
	sdProgram::ReturnEntity( owner.GetEntity() );
}

/*
================
idProjectile::Event_SetOwner
================
*/
void idProjectile::Event_SetOwner( idEntity* _owner ) {
	owner = _owner;
}

/*
================
idProjectile::Killed
================
*/
void idProjectile::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	sdScriptHelper h;
	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnKilled" ), h );
}

/*
=================
idProjectile::SetLaunchTime
=================
*/
void idProjectile::SetLaunchTime( int time ) {
	if ( launchTime == time ) {
		return;
	}

	launchTime				= time;
	thrustStartTime			= launchTime + SEC2MS( spawnArgs.GetFloat( "target_post_launch" ) );
	targetForgetTime		= thrustStartTime + SEC2MS( spawnArgs.GetFloat( "target_forget" ) );

	if ( !gameLocal.isClient ) {
		int delay = targetForgetTime - gameLocal.time;
		if ( delay > 0 ) {
			PostEventMS( &EV_SetEnemy, delay, ( idEntity* )NULL );
		}
	}

	if ( scriptObject ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnLaunchTimeChanged" ), h1 );
	}
}

/*
=================
idProjectile::Collide
=================
*/
bool idProjectile::Collide( const trace_t &collision, const idVec3& velocity, int bodyId ) {
	sdLoggedTrace* loggedTrace = gameLocal.RegisterLoggedTrace( collision );

	sdScriptHelper helper;
	helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
	helper.Push( velocity );
	helper.Push( bodyId );
	bool retVal = CallFloatNonBlockingScriptEvent( scriptObject->GetFunction( "OnCollide" ), helper ) != 0.0f;

	gameLocal.FreeLoggedTrace( loggedTrace );
	
	return retVal;
}

/*
================
idProjectile::OnTouch
================
*/
void idProjectile::OnTouch( idEntity *other, const trace_t& trace ) {
	idPlayer* player = other->Cast< idPlayer >();
	if ( player != NULL && !player->IsSpectator() && player->GetHealth() <= 0 ) {
		return;
	}

	sdLoggedTrace* loggedTrace = gameLocal.RegisterLoggedTrace( trace );

	sdScriptHelper helper;
	helper.Push( other->GetScriptObject() );
	helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnTouch" ), helper );

	if ( loggedTrace ) {
		gameLocal.FreeLoggedTrace( loggedTrace );
	}
}

/*
================
idProjectile::UpdateModelTransform
================
*/
void idProjectile::UpdateModelTransform( void ) {
	idPhysics* _physics = GetPhysics();

	if ( projectileFlags.faceVelocitySet ) {
		renderEntity.axis = visualAxes;
	} else {
		renderEntity.axis = _physics->GetAxis();
	}
	renderEntity.origin = _physics->GetOrigin();
}

/*
================
idProjectile::ApplyNetworkState
================
*/
void idProjectile::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdProjectileNetworkData );
		NET_APPLY_STATE_PHYSICS;
		NET_APPLY_STATE_SCRIPT;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdProjectileBroadcastData );
		NET_APPLY_STATE_PHYSICS;
		NET_APPLY_STATE_SCRIPT;
		
		// update state
		team							= newData.team;
		owner.SetSpawnId( newData.ownerId );
		SetEnemy( gameLocal.EntityForSpawnId( newData.enemyId ) );
		SetLaunchTime( newData.launchTime );
		launchSpeed = newData.launchSpeed;
		owners.SetNum( newData.owners.Num() );
		for ( int i = 0; i < owners.Num(); i++ ) {
			owners[ i ].SetSpawnId( newData.owners[ i ] );
		}
		if ( newData.hidden ) {
			Hide();
		} else {
			Show();
		}
		newData.bindData.Apply( this );
	}
}

/*
================
idProjectile::ReadNetworkState
================
*/
void idProjectile::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdProjectileNetworkData );
		NET_READ_STATE_PHYSICS;
		NET_READ_STATE_SCRIPT;

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdProjectileBroadcastData );
		NET_READ_STATE_PHYSICS;
		NET_READ_STATE_SCRIPT;

		// read new state
		newData.team			= sdTeamManager::GetInstance().ReadTeamFromStream( baseData.team, msg );
		newData.ownerId			= msg.ReadDeltaLong( baseData.ownerId );
		newData.enemyId			= msg.ReadDeltaLong( baseData.enemyId );
		newData.launchTime		= msg.ReadDeltaLong( baseData.launchTime );
		newData.launchSpeed		= msg.ReadDeltaFloat( baseData.launchSpeed );
		int count				= msg.ReadDeltaLong( baseData.owners.Num() );
		newData.owners.SetNum( count );
		for ( int i = 0; i < newData.owners.Num(); i++ ) {
			if ( i < baseData.owners.Num() ) {
				newData.owners[ i ] = msg.ReadDeltaLong( baseData.owners[ i ] );
			} else {
				newData.owners[ i ] = msg.ReadLong();
			}
		}
		newData.hidden			= msg.ReadBool();
		newData.bindData.Read( this, baseData.bindData, msg );

		return;
	}
}

/*
================
idProjectile::WriteNetworkState
================
*/
void idProjectile::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdProjectileNetworkData );
		NET_WRITE_STATE_PHYSICS;
		NET_WRITE_STATE_SCRIPT;

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdProjectileBroadcastData );
		NET_WRITE_STATE_PHYSICS;
		NET_WRITE_STATE_SCRIPT;

		// update state
		newData.team			= team;
		newData.ownerId			= owner.GetSpawnId();
		newData.enemyId			= enemy.GetSpawnId();
		newData.launchTime		= launchTime;
		newData.launchSpeed		= launchSpeed;
		newData.owners.SetNum( owners.Num() );
		for ( int i = 0; i < owners.Num(); i++ ) {
			newData.owners[ i ] = owners[ i ].GetSpawnId();
		}
		newData.hidden			= fl.hidden;

		// write new state
		sdTeamManager::GetInstance().WriteTeamToStream( baseData.team, newData.team, msg );
		msg.WriteDeltaLong( baseData.ownerId, newData.ownerId );
		msg.WriteDeltaLong( baseData.enemyId, newData.enemyId );
		msg.WriteDeltaLong( baseData.launchTime, newData.launchTime );
		msg.WriteDeltaFloat( baseData.launchSpeed, newData.launchSpeed );
		msg.WriteDeltaLong( baseData.owners.Num(), newData.owners.Num() );
		for ( int i = 0; i < newData.owners.Num(); i++ ) {
			if ( i < baseData.owners.Num() ) {
				msg.WriteDeltaLong( baseData.owners[ i ], newData.owners[ i ] );
			} else {
				msg.WriteLong( newData.owners[ i ] );
			}
		}
		msg.WriteBool( newData.hidden );
		newData.bindData.Write( this, baseData.bindData, msg );

		return;
	}
}

/*
================
idProjectile::CheckNetworkStateChanges
================
*/
bool idProjectile::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {	
		NET_GET_BASE( sdProjectileNetworkData );
		NET_CHECK_STATE_PHYSICS;
		NET_CHECK_STATE_SCRIPT;

		return false;
	}

	if ( mode == NSM_BROADCAST ) {	
		NET_GET_BASE( sdProjectileBroadcastData );
		NET_CHECK_STATE_PHYSICS;
		NET_CHECK_STATE_SCRIPT;

		if ( baseData.team != team ) {
			return true;
		}

		if ( baseData.ownerId != owner.GetSpawnId() ) {
			return true;
		}

		if ( baseData.enemyId != enemy.GetSpawnId() ) {
			return true;
		}		

		if ( baseData.launchTime != launchTime ) {
			return true;
		}

		if ( baseData.launchSpeed != launchSpeed ) {
			return true;
		}

		if ( baseData.owners.Num() != owners.Num() ) {
			return true;
		}

		int i;
		for ( i = 0; i < owners.Num(); i++ ) {
			if ( baseData.owners[ i ] != owners[ i ].GetSpawnId() ) {
				return true;
			}
		}

		if ( baseData.hidden != fl.hidden ) {
			return true;
		}

		if ( baseData.bindData.Check( this ) ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
idProjectile::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idProjectile::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		sdProjectileNetworkData* newData = new sdProjectileNetworkData();
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
		return newData;
	}
	if ( mode == NSM_BROADCAST ) {
		sdProjectileBroadcastData* newData = new sdProjectileBroadcastData();
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );
		return newData;
	}
	return NULL;
}

/*
===============================================================================

	idProjectile_RigidBody

===============================================================================
*/

CLASS_DECLARATION( idProjectile, idProjectile_RigidBody )
END_CLASS

/*
================
idProjectile_RigidBody::idProjectile_RigidBody
================
*/
idProjectile_RigidBody::idProjectile_RigidBody( void ) {
	waterEffects = NULL;
}

/*
================
idProjectile_RigidBody::~idProjectile_RigidBody
================
*/
idProjectile_RigidBody::~idProjectile_RigidBody( void ) {
	delete waterEffects;
}

/*
================
idProjectile_RigidBody::~idProjectile_RigidBody
================
*/
void idProjectile_RigidBody::Spawn( void ) {
	waterEffects = sdWaterEffects::SetupFromSpawnArgs( spawnArgs );
}


/*
=================
idProjectile_RigidBody::InitPhysics
=================
*/
void idProjectile_RigidBody::InitPhysics( void ) {
	float linearFriction		= spawnArgs.GetFloat( "linear_friction" );
	float angularFriction		= spawnArgs.GetFloat( "angular_friction" );
	float contactFriction		= spawnArgs.GetFloat( "contact_friction" );
	float bounce				= spawnArgs.GetFloat( "bounce" );
	float gravity				= spawnArgs.GetFloat( "gravity" );
	float mass					= spawnArgs.GetFloat( "mass" );
	float buoyancy				= spawnArgs.GetFloat( "buoyancy" );

	if ( mass < idMath::FLT_EPSILON ) {
		gameLocal.Error( "Invalid mass on '%s'", GetEntityDefName() );
	}

	idVec3 gravVec = gameLocal.GetGravity();
	gravVec.Normalize();

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetContents( 0 );
	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linearFriction, angularFriction, contactFriction );
	if ( contactFriction == 0.f ) {
		physicsObj.NoContact();
	}
	physicsObj.SetBouncyness( bounce );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetBuoyancy( buoyancy );

	SetPhysics( &physicsObj );
}

/*
=================
idProjectile_RigidBody::InitLaunchPhysics
=================
*/
void idProjectile_RigidBody::InitLaunchPhysics( float launchPower, const idVec3& origin, const idMat3& axes, const idVec3& pushVelocity ) {
	idVec3 velocity				= spawnArgs.GetVector( "velocity", "0 0 0" );
	idVec3 angular_velocity		= spawnArgs.GetVector( "angular_velocity", "0 0 0" );

	float speed					= velocity.Length() + launchPower;

	physicsObj.SetLinearVelocity( ( axes[ 0 ] * speed ) + pushVelocity );
	physicsObj.SetAngularVelocity( DEG2RAD( angular_velocity ) * axes );
}

/*
================
idProjectile_RigidBody::CheckWater
================
*/
void idProjectile_RigidBody::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects ) {
		waterEffects->SetOrigin( GetPhysics()->GetOrigin() );
		waterEffects->SetAxis( GetPhysics()->GetAxis() );
		waterEffects->SetVelocity( GetPhysics()->GetLinearVelocity() );		
		waterEffects->CheckWater( this, waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}

/*
===============================================================================

sdProjectile_Parabolic

===============================================================================
*/

CLASS_DECLARATION( idProjectile, sdProjectile_Parabolic )
END_CLASS

/*
================
sdProjectile_Parabolic::sdProjectile_Parabolic
================
*/
sdProjectile_Parabolic::sdProjectile_Parabolic( void ) : waterEffects(NULL) {
}

/*
==============
sdProjectile_Parabolic::~sdProjectile_Parabolic
==============
*/
sdProjectile_Parabolic::~sdProjectile_Parabolic( ) {
	delete waterEffects;
}

/*
==============
sdProjectile_Parabolic::Spawn
==============
*/
void sdProjectile_Parabolic::Spawn( void ) {
	waterEffects = sdWaterEffects::SetupFromSpawnArgs( spawnArgs );
}

/*
=================
sdProjectile_Parabolic::InitPhysics
=================
*/
void sdProjectile_Parabolic::InitPhysics( void ) {
	float gravity				= spawnArgs.GetFloat( "gravity" );

	idVec3 gravVec = gameLocal.GetGravity();
	gravVec.Normalize();

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ) );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetContents( 0 );

	SetPhysics( &physicsObj );
}

/*
=================
sdProjectile_Parabolic::InitLaunchPhysics
=================
*/
void sdProjectile_Parabolic::InitLaunchPhysics( float launchPower, const idVec3& origin, const idMat3& axes, const idVec3& pushVelocity ) {
	idVec3 velocity				= spawnArgs.GetVector( "velocity", "0 0 0" );
	float speed					= velocity.Length() + launchPower;
	velocity					= ( axes[ 0 ] * speed ) + pushVelocity;

	physicsObj.Init( origin, velocity, vec3_zero, axes, gameLocal.time, -1 );
}

/*
==============
sdProjectile_Parabolic::CheckWater
==============
*/
void sdProjectile_Parabolic::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects ) {
		waterEffects->SetOrigin( GetPhysics()->GetOrigin() );
		waterEffects->SetAxis( GetPhysics()->GetAxis() );
		waterEffects->SetVelocity( GetPhysics()->GetLinearVelocity() );		
		waterEffects->CheckWater( this, waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}
