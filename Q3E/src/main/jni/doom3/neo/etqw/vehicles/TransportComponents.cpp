// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "TransportComponents.h"
#include "Transport.h"
#include "Vehicle_RigidBody.h"
#include "../anim/Anim.h"
#include "Attachments.h"
#include "../ContentMask.h"
#include "../IK.h"
#include "VehicleIK.h"
#include "../../framework/CVarSystem.h"
#include "VehicleSuspension.h"
#include "VehicleControl.h"
#include "../Player.h"
#include "../client/ClientMoveable.h"
#include "../physics/Physics_JetPack.h"

#include "../../decllib/DeclSurfaceType.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../effects/TireTread.h"

const float minParticleCreationSpeed = 64.f;

idCVar g_disableTransportDebris(		"g_disableTransportDebris",			"0",	CVAR_GAME | CVAR_BOOL, "" );
idCVar g_maxTransportDebrisExtraHigh(	"g_maxTransportDebrisExtraHigh",	"8",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "The maximum number of pieces of extra high priority (really large) debris. -1 means no limit." );
idCVar g_maxTransportDebrisHigh(		"g_maxTransportDebrisHigh",			"8",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "The maximum number of pieces of high priority (large) debris. -1 means no limit." );
idCVar g_maxTransportDebrisMedium(		"g_maxTransportDebrisMedium",		"8",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "The maximum number of pieces of medium priority (middling) debris. -1 means no limit." );
idCVar g_maxTransportDebrisLow(			"g_maxTransportDebrisLow",			"8",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "The maximum number of pieces of low priority (small) debris. -1 means no limit." );
idCVar g_transportDebrisExtraHighCutoff("g_transportDebrisExtraHighCutoff",	"8192",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "Beyond this distance from the viewpoint extra high priority debris will not be spawned. -1 means no limit." );
idCVar g_transportDebrisHighCutoff(		"g_transportDebrisHighCutoff",		"4096",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "Beyond this distance from the viewpoint high priority debris will not be spawned. -1 means no limit." );
idCVar g_transportDebrisMediumCutoff(	"g_transportDebrisMediumCutoff",	"2048",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "Beyond this distance from the viewpoint medium priority debris will not be spawned. -1 means no limit." );
idCVar g_transportDebrisLowCutoff(		"g_transportDebrisLowCutoff",		"1024",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, "Beyond this distance from the viewpoint low priority debris will not be spawned. -1 means no limit." );

sdVehicleDriveObject::debrisList_t		sdVehicleDriveObject::extraHighDebris;
sdVehicleDriveObject::debrisList_t		sdVehicleDriveObject::highDebris;
sdVehicleDriveObject::debrisList_t		sdVehicleDriveObject::mediumDebris;
sdVehicleDriveObject::debrisList_t		sdVehicleDriveObject::lowDebris;

/*
===============================================================================

	sdVehicleDriveObject

===============================================================================
*/

ABSTRACT_DECLARATION( idClass, sdVehicleDriveObject )
END_CLASS

/*
================
sdVehicleDriveObject::sdVehicleDriveObject
================
*/
sdVehicleDriveObject::sdVehicleDriveObject( void ) {
	hidden			= false;
	scriptObject	= NULL;
}

/*
================
sdVehicleDriveObject::~sdVehicleDriveObject
================
*/
sdVehicleDriveObject::~sdVehicleDriveObject( void ) {
	if ( scriptObject ) {
		gameLocal.program->FreeScriptObject( scriptObject );
	}
}

/*
================
sdVehicleDriveObject::PostInit
================
*/
void sdVehicleDriveObject::PostInit( void ) {
	sdTransport* parent = GetParent();
	assert( parent );
	if ( GoesInPartList() ) {
		parent->AddActiveDriveObject( this );
	}
}

/*
================
sdVehicleDriveObject::Hide
================
*/
void sdVehicleDriveObject::Hide( void ) {
	if ( hidden ) {
		return;
	}

	hidden = true;

	sdTransport* parent = GetParent();
	assert( parent );
	if ( GoesInPartList() ) {
		parent->RemoveActiveDriveObject( this );

		if ( IsType( sdVehicleRigidBodyWheel::Type ) ) {
			parent->RemoveCriticalDrivePart();
		}
	}
}

/*
================
sdVehicleDriveObject::Show
================
*/
void sdVehicleDriveObject::Show( void ) {
	if ( !hidden ) {
		return;
	}

	hidden = false;

	sdTransport* parent = GetParent();
	assert( parent );
	if ( GoesInPartList() ) {
		parent->AddActiveDriveObject( this );

		if ( IsType( sdVehicleRigidBodyWheel::Type ) ) {
			parent->AddCriticalDrivePart();
		}
	}
}

/*
================
sdVehicleDriveObject::CanAddDebris
================
*/
bool sdVehicleDriveObject::CanAddDebris( debrisPriority_t priority, const idVec3& origin ) {
	debrisList_t* list = &highDebris;
	int limit = g_maxTransportDebrisHigh.GetInteger();
	float distanceLimit = g_transportDebrisHighCutoff.GetFloat();
	if ( priority == PRIORITY_MEDIUM ) {
		list = &mediumDebris;
		limit = g_maxTransportDebrisMedium.GetInteger();
		distanceLimit = g_transportDebrisMediumCutoff.GetFloat();
	} else if ( priority == PRIORITY_LOW ) {
		list = &lowDebris;
		limit = g_maxTransportDebrisLow.GetInteger();
		distanceLimit = g_transportDebrisLowCutoff.GetFloat();
	} else if ( priority == PRIORITY_EXTRA_HIGH ) {
		list = &extraHighDebris;
		limit = g_maxTransportDebrisExtraHigh.GetInteger();
		distanceLimit = g_transportDebrisExtraHighCutoff.GetFloat();
	}



	// clean out NULLs
	for ( int i = 0; i < list->Num(); i++ ) {
		if ( !(*list)[ i ].IsValid() ) {
			list->RemoveIndexFast( i );
			i--;
			continue;
		}
	}

	// don't let it exceed the limit
	if ( limit != -1 && list->Num() >= limit ) {
		return false;
	}

	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( distanceLimit != -1 && player != NULL ) {
		float distance = ( player->GetViewPos() - origin ).Length();
		if ( distance > distanceLimit ) {
			return false;
		}
	}

	return true;
}

/*
================
sdVehicleDriveObject::AddDebris
================
*/
void sdVehicleDriveObject::AddDebris( rvClientMoveable* debris, debrisPriority_t priority ) {
	debrisList_t* list = &highDebris;
	int limit = g_maxTransportDebrisHigh.GetInteger();
	if ( priority == PRIORITY_MEDIUM ) {
		list = &mediumDebris;
		limit = g_maxTransportDebrisMedium.GetInteger();
	} else if ( priority == PRIORITY_LOW ) {
		list = &lowDebris;
		limit = g_maxTransportDebrisLow.GetInteger();
	} else if ( priority == PRIORITY_EXTRA_HIGH ) {
		list = &extraHighDebris;
		limit = g_maxTransportDebrisExtraHigh.GetInteger();
	}

	// -1 means no limit
	if ( limit == -1 ) {
		return;
	}

	rvClientEntityPtr< rvClientMoveable >* entry = list->Alloc();
	if ( entry != NULL ) {
		entry->operator=( debris );
	}
}

/*
===============================================================================

  sdVehiclePart

===============================================================================
*/

extern const idEventDef EV_GetAngles;
extern const idEventDef EV_GetHealth;
extern const idEventDef EV_GetOrigin;

const idEventDef EV_VehiclePart_GetParent( "getParent", 'e', DOC_TEXT( "Returns the vehicle this part belongs to." ), 0, "This will never return $null$." );
const idEventDef EV_VehiclePart_GetJoint( "getJoint", 's', DOC_TEXT( "Returns the name of the joint this part is associated with." ), 0, "An empty string will be returned if the lookup fails." );

ABSTRACT_DECLARATION( sdVehicleDriveObject, sdVehiclePart )
	EVENT( EV_GetHealth,					sdVehiclePart::Event_GetHealth )
	EVENT( EV_GetOrigin,					sdVehiclePart::Event_GetOrigin )
	EVENT( EV_GetAngles,					sdVehiclePart::Event_GetAngles )	
	EVENT( EV_VehiclePart_GetParent,		sdVehiclePart::Event_GetParent )	
	EVENT( EV_VehiclePart_GetJoint,			sdVehiclePart::Event_GetJoint )
END_CLASS

/*
================
sdVehiclePart::sdVehiclePart
================
*/
sdVehiclePart::sdVehiclePart( void ) {
	bodyId			= -1;
	waterEffects	= NULL;
	scriptObject	= NULL;
	reattachTime	= 0;
}


/*
================
sdVehiclePart::AddSurface
================
*/
void sdVehiclePart::AddSurface( const char* surfaceName )  { 
	int id = GetParent()->FindSurfaceId( surfaceName );
	if ( id != -1 ) {
		surfaces.Alloc() = id;
	} else {
		gameLocal.Warning( "sdVehiclePart::AddSurface Invalid Surface '%s'", surfaceName );
	}
}

/*
================
sdVehiclePart::Detach
================
*/
void sdVehiclePart::Detach( bool createDebris, bool decay ) {
	if ( IsHidden() ) {
		return;
	}

	Hide();
	if ( !noAutoHide ) {
		HideSurfaces();
	}

	idPhysics* physics = GetParent()->GetPhysics();
	if( bodyId != -1 ) {
		oldcontents = physics->GetContents( bodyId );
		oldclipmask = physics->GetClipMask( bodyId );

		physics->SetContents( 0, bodyId );
		physics->SetClipMask( 0, bodyId );
	}

	if ( createDebris && brokenPart ) {
		if ( decay == false && physics->InWater() < 1.0f ) {
			CreateExplosionDebris();
		} else {
			CreateDecayDebris();
		}
	}
}

/*
================
sdVehiclePart::CreateExplosionDebris
================
*/
void sdVehiclePart::CreateExplosionDebris( void ) {
	if ( g_disableTransportDebris.GetBool() ) {
		return;
	}

	sdTransport* transport = GetParent();
	idPhysics*	parentPhysics = transport->GetPhysics();
	idVec3 org;
	idMat3 axis;
	GetWorldOrigin( org );
	GetWorldAxis( axis );

	// make sure we can have another part of this priority
	debrisPriority_t priority = ( debrisPriority_t )brokenPart->dict.GetInt( "priority" );
	if ( !CanAddDebris( priority, org ) ) {
		return;
	}

	if ( transport->GetMasterDestroyedPart() != NULL ) {
		parentPhysics = transport->GetMasterDestroyedPart()->GetPhysics();
	}

	const idVec3& vel = parentPhysics->GetLinearVelocity();
	const idVec3& aVel = parentPhysics->GetAngularVelocity();

	rvClientMoveable* cent = gameLocal.SpawnClientMoveable( brokenPart->GetName(), 5000, org, axis, vec3_origin, vec3_origin );
	AddDebris( cent, priority );
	if ( cent != NULL ) {
		gameLocal.PlayEffect( brokenPart->dict, colorWhite.ToVec3(), "fx_explode", NULL, cent->GetPhysics()->GetOrigin(), cent->GetPhysics()->GetAxis(), false );	

		if ( flipMaster ) {
			transport->SetMasterDestroyedPart( cent );
		}

		// 
		const idVec3& centCOM = cent->GetPhysics()->GetCenterOfMass();
		const idVec3& parentCOM = parentPhysics->GetCenterOfMass();
		const idVec3& parentOrg = parentPhysics->GetOrigin();
		const idMat3& parentAxis = parentPhysics->GetAxis();

		idVec3 radiusVector = ( centCOM*axis + org ) - ( parentCOM * parentAxis + parentOrg );
		// calculate the actual linear velocity of this point
		idVec3 myVelocity = vel + aVel.Cross( radiusVector );
		cent->GetPhysics()->SetLinearVelocity( myVelocity );
		
		//
		cent->GetPhysics()->SetContents( 0, 0 );
		cent->GetPhysics()->SetClipMask( CONTENTS_SOLID | CONTENTS_BODY, 0 );

		if ( flipPower != 0.0f ) {
			idBounds bounds = cent->GetPhysics()->GetBounds();
			// choose a random point in its bounds to apply an upwards impulse
			idVec3 size = bounds.Size();
			idVec3 point;

			// pick whether it should flip forwards or backwards based on the velocity
			if ( myVelocity * axis[ 0 ] >= 0.0f ) {
				point.x = 0.0f;
			} else {
				point.x = size.x;
			}

			point.y = size.y * gameLocal.random.RandomFloat();
			point.z = size.z * gameLocal.random.RandomFloat();
			point += bounds[ 0 ];

			point *= axis;
			point += org;

			const idVec3& gravityNormal = -cent->GetPhysics()->GetGravityNormal();
			idVec3 flipDirection = -gravityNormal;
			float flipAmount = -flipDirection * cent->GetPhysics()->GetGravity();
			if ( parentPhysics != transport->GetPhysics() ) {
				// this is a slaved part
				flipDirection = radiusVector;
				flipDirection -= ( flipDirection*gravityNormal )*gravityNormal;
				flipDirection.Normalize();
			}

			float scaleByVel = idMath::ClampFloat( 0.75f, 1.0f, myVelocity.Length() / 500.0f );

			// calculate the impulse to apply
			idVec3 impulse = scaleByVel * flipPower * MS2SEC( gameLocal.msec ) * cent->GetPhysics()->GetMass() * flipDirection * flipAmount;
			cent->GetPhysics()->ApplyImpulse( 0, point, impulse );
		}
	}
}

/*
================
sdVehiclePart::CreateDecayDebris
================
*/
void sdVehiclePart::CreateDecayDebris( void ) {
	if ( g_disableTransportDebris.GetBool() ) {
		return;
	}

	sdTransport* transport = GetParent();

	idVec3 org;
	idMat3 axis;
	GetWorldOrigin( org );
	GetWorldAxis( axis );

	// make sure we can have another part of this priority
	debrisPriority_t priority = ( debrisPriority_t )brokenPart->dict.GetInt( "priority" );
	if ( !CanAddDebris( priority, org ) ) {
		return;
	}

	idVec3 velMax = brokenPart->dict.GetVector( "decay_velocity_max", "300 0 0" );
	idVec3 aVelMax = brokenPart->dict.GetVector( "decay_angular_velocity_max", "0.5 1 2" );

	idVec3 vel, aVel;
	float random = gameLocal.random.RandomFloat();
	for( int i = 0; i < 3; i++ ) {
		// jrad - disable damage-based motion for now...
		vel[ i ] = random * ( velMax[ i ] /*+ damageAmount * -damageDirection[ i ] */ );
		aVel[ i ] = random * aVelMax[ i ];
	}

	vel *= axis;

	vel += transport->GetPhysics()->GetLinearVelocity();
	aVel += transport->GetPhysics()->GetAngularVelocity();

	vel *= transport->GetRenderEntity()->axis;
	aVel *= transport->GetRenderEntity()->axis;

	rvClientMoveable* cent = gameLocal.SpawnClientMoveable( brokenPart->GetName(), 5000, org, axis, vel, aVel, 1 );
	AddDebris( cent, priority );
	if ( cent != NULL ) {
		gameLocal.PlayEffect( brokenPart->dict, colorWhite.ToVec3(), "fx_decay", NULL, cent->GetPhysics()->GetOrigin(), cent->GetPhysics()->GetAxis(), false );	

		cent->GetPhysics()->SetContents( 0 );
		cent->GetPhysics()->SetClipMask( CONTENTS_SOLID | CONTENTS_BODY );
	}
}

/*
================
sdVehiclePart::Reattach
================
*/
void sdVehiclePart::Reattach( void ) {
	if ( !IsHidden() ) {
		return;
	}

	Show();
	ShowSurfaces();

	idPhysics* physics = GetParent()->GetPhysics();
	if( bodyId != -1 ) {
		physics->SetContents( oldcontents, bodyId );
		physics->SetClipMask( oldclipmask, bodyId );
	}

	physics->Activate();

	reattachTime = gameLocal.time;
}

/*
================
sdVehiclePart::Repair
================
*/
int sdVehiclePart::Repair( int repair ) {
	if ( health >= maxHealth ) {
		return 0;
	}

	int heal = maxHealth - health;
	if( repair > heal ) {
		repair = heal;
	}
	health += repair;

	return repair;
}

/*
================
sdVehiclePart::HideSurfaces
================
*/
void sdVehiclePart::HideSurfaces( void ) {
	bool surfacesChanged = false;
	idRenderModel* renderModel = GetParent()->GetRenderEntity()->hModel;
	for ( int i = 0; i < surfaces.Num(); i++ ) {
		GetParent()->GetRenderEntity()->hideSurfaceMask.Set( surfaces[ i ] );

		surfacesChanged = true;
	}

	// remove bound things in that bounds
	idBounds partBounds;
	idVec3 origin;
	idMat3 axis;

	GetWorldOrigin( origin );
	GetWorldAxis( axis );
	GetBounds( partBounds );
	partBounds.RotateSelf( axis );
	partBounds.TranslateSelf( origin );

	GetParent()->RemoveBinds( &partBounds, true );

	if ( GetParent()->GetModelDefHandle() != -1 ) {
		gameRenderWorld->RemoveDecals( GetParent()->GetModelDefHandle() );
	}

	if ( surfacesChanged ) {
		GetParent()->BecomeActive( TH_UPDATEVISUALS );
	}
}

/*
================
sdVehiclePart::ShowSurfaces
================
*/
void sdVehiclePart::ShowSurfaces( void ) {
	bool surfacesChanged = false;

	for ( int i = 0; i < surfaces.Num(); i++ ) {
		GetParent()->GetRenderEntity()->hideSurfaceMask.Clear( surfaces[ i ] );
		surfacesChanged = true;
	}

	if ( surfacesChanged ) {
		GetParent()->BecomeActive( TH_UPDATEVISUALS );
	}
}

/*
================
sdVehiclePart::Damage
================
*/
void sdVehiclePart::Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( IsHidden() ) {
		return;
	}

	health -= damage;
	if ( health < -1 ) {
		health = -1;
	}

	sdTransport* transport = GetParent();
	if ( health <= 0 || transport->GetHealth() <= 0 ) {
		Detach( true, false );
		OnKilled();
		transport->GetPhysics()->Activate();
	}
}

/*
================
sdVehiclePart::Decay
	This makes a part fall off, instead of blowing it off like Damage will.
================
*/
void sdVehiclePart::Decay( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( IsHidden() ) {
		return;
	}

	// destroy it instantly
	health = -1;

	sdTransport* transport = GetParent();
	//HideSurfaces(); ?!? detach calls this already
	Detach( true, true );
	OnKilled();
	transport->GetPhysics()->Activate();
}

/*
================
sdVehiclePart::~sdVehiclePart
================
*/
sdVehiclePart::~sdVehiclePart() {
	delete waterEffects;
}

/*
================
sdVehiclePart::GetBounds
================
*/
void sdVehiclePart::GetBounds( idBounds& bounds ) {
	if( !GetParent() || bodyId == -1 ) {
		bounds = partBounds;
		return;
	}

	bounds = GetParent()->GetPhysics()->GetBounds( bodyId );
}

/*
================
sdVehiclePart::GetWorldOrigin
================
*/
void sdVehiclePart::GetWorldOrigin( idVec3& vec ) {
	if( !GetParent() || bodyId == -1 ) {
		vec = GetParent()->GetPhysics()->GetOrigin();
		return;
	}

	vec = GetParent()->GetPhysics()->GetOrigin( bodyId );
}

/*
================
sdVehiclePart::GetWorldAxis
================
*/
void sdVehiclePart::GetWorldAxis( idMat3& axis ) {
	if( !GetParent() || bodyId == -1 ) {
		axis = GetParent()->GetPhysics()->GetAxis();
		return;
	}

	axis = GetParent()->GetPhysics()->GetAxis( bodyId );
}

/*
================
sdVehiclePart::Init
================
*/
void sdVehiclePart::Init( const sdDeclVehiclePart& part ) {
	name							= part.data.GetString( "name" );
	maxHealth = health				= part.data.GetInt( "health" );
	brokenPart						= gameLocal.declEntityDefType[ part.data.GetString( "def_brokenPart" ) ];
	damageInfo.damageScale			= part.data.GetFloat( "damageScale", "1.0" );
	damageInfo.collisionScale		= part.data.GetFloat( "collisionScale", "1.0" );
	damageInfo.collisionMinSpeed	= part.data.GetFloat( "collisionMinSpeed", "256.0" );
	damageInfo.collisionMaxSpeed	= part.data.GetFloat( "collisionMaxSpeed", "1024.0" );
	noAutoHide						= part.data.GetBool( "noAutoHide", "0" );
	flipPower						= part.data.GetFloat( "flip_power", "5" );
	flipMaster						= part.data.GetBool( "flip_master" );
	
	const idKeyValue* kv = NULL;
	while ( kv = part.data.MatchPrefix( "surface", kv ) ) {
		AddSurface( kv->GetValue() );
	}

	partBounds.Clear();
	if ( brokenPart ) {
		renderEntity_t renderEnt;
		gameEdit->ParseSpawnArgsToRenderEntity( brokenPart->dict, renderEnt );

		if ( renderEnt.hModel ) {
			partBounds = renderEnt.hModel->Bounds();
		}
	}
	
	waterEffects = sdWaterEffects::SetupFromSpawnArgs( part.data );
	if ( waterEffects ) {
		waterEffects->SetMaxVelocity( 300.0f );
	}
}

/*
================
sdVehiclePart::CheckWater
================
*/
void sdVehiclePart::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects ) {
		idVec3 temp;
		idMat3 temp2;
		GetWorldOrigin( temp );
		GetWorldAxis( temp2 );
		waterEffects->SetOrigin( temp );
		waterEffects->SetAxis( temp2 );
		waterEffects->SetVelocity( GetParent()->GetPhysics()->GetLinearVelocity() );
		waterEffects->CheckWater( GetParent(), waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}

/*
================
sdVehiclePart::CalcSurfaceBounds
================
*/
idBounds sdVehiclePart::CalcSurfaceBounds( jointHandle_t joint ) {
	idBounds res;
	//res[0] = idVec3(-20,-20,-20);
	//res[1] = idVec3(20,20,20);
	res.Clear();
	if ( !GetParent() ) {
		return res;
	}

	for ( int i = 0; i < surfaces.Num(); i++ ) {
		idBounds b;
		GetParent()->GetAnimator()->GetMeshBounds( joint, surfaces[ i ], gameLocal.time, b, true ); 
		res.AddBounds( b );
	}
	return res;
}

/*
================
sdVehiclePart::Event_GetHealth
================
*/
void sdVehiclePart::Event_GetHealth( void ) {
	sdProgram::ReturnFloat( health );
}

/*
================
sdVehiclePart::Event_GetOrigin
================
*/
void sdVehiclePart::Event_GetOrigin( void ) {
	idVec3 temp;
	GetWorldOrigin( temp );
	sdProgram::ReturnVector( temp );
}

/*
================
sdVehiclePart::Event_GetAngles
================
*/
void sdVehiclePart::Event_GetAngles( void ) {
	idMat3 temp;
	GetWorldAxis( temp );
	idAngles ang = temp.ToAngles();
	sdProgram::ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
sdVehiclePart::Event_GetParent
================
*/
void sdVehiclePart::Event_GetParent( void ) {
	sdProgram::ReturnEntity( GetParent() );
}

/*
================
sdVehiclePart::Event_GetJoint
================
*/
void sdVehiclePart::Event_GetJoint( void ) {
	if ( !GetParent() ) {
		sdProgram::ReturnString("");
		return;
	}
	if ( !GetParent()->GetAnimator() ) {
		sdProgram::ReturnString("");
		return;
	}

	sdProgram::ReturnString( GetParent()->GetAnimator()->GetJointName( GetJoint() ) );
}

/*
===============================================================================

sdVehiclePartSimple

===============================================================================
*/

CLASS_DECLARATION( sdVehiclePart, sdVehiclePartSimple )
END_CLASS


/*
============
sdVehiclePart::ShouldDisplayDebugInfo
============
*/
bool sdVehiclePartSimple::ShouldDisplayDebugInfo() const {
	return true;
//	idPlayer* player = gameLocal.GetLocalPlayer();
//	return ( player && parent == player->GetProxyEntity() );
}

/*
================
sdVehiclePartSimple::Init
================
*/
void sdVehiclePartSimple::Init( const sdDeclVehiclePart& part, sdTransport* _parent ) {
	parent = _parent;

	sdVehiclePart::Init( part );

	joint = _parent->GetAnimator()->GetJointHandle( part.data.GetString( "joint" ) );

	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehiclePartSimple::Init Invalid Joint Name '%s'", part.data.GetString( "joint" ) );
	}

	partBounds = CalcSurfaceBounds( joint );
}

/*
================
sdVehiclePartSimple::GetWorldAxis
================
*/
void sdVehiclePartSimple::GetWorldAxis( idMat3& axis ) {
	parent->GetWorldAxis( joint, axis );
}

/*
================
sdVehiclePartSimple::GetWorldOrigin
================
*/
void sdVehiclePartSimple::GetWorldOrigin( idVec3& vec ) {
	parent->GetWorldOrigin( joint, vec );
}

/*
================
sdVehiclePartSimple::GetWorldPhysicsAxis
================
*/
void sdVehiclePartSimple::GetWorldPhysicsAxis( idMat3& axis ) {
	GetWorldAxis( axis );

	// transform this to be relative to the physics. can't use the render info as physics input!
	const idMat3& physicsAxis = parent->GetPhysics()->GetAxis();
	const idMat3& renderAxis = parent->GetRenderEntity()->axis;

	axis = physicsAxis * renderAxis.TransposeMultiply( axis );
}

/*
================
sdVehiclePartSimple::GetWorldPhysicsOrigin
================
*/
void sdVehiclePartSimple::GetWorldPhysicsOrigin( idVec3& vec ) {
	GetWorldOrigin( vec );

	// transform this to be relative to the physics. can't use the render info as physics input!
	const idVec3& physicsOrigin = parent->GetPhysics()->GetOrigin();
	const idVec3& renderOrigin = parent->GetRenderEntity()->origin;
	const idMat3& physicsAxis = parent->GetPhysics()->GetAxis();
	const idMat3& renderAxis = parent->GetRenderEntity()->axis;

	vec = physicsAxis * renderAxis.TransposeMultiply( vec - renderOrigin ) + physicsOrigin;
}

/*
============
sdVehicleRigidBodyPart::ShouldDisplayDebugInfo
============
*/
bool sdVehicleRigidBodyPart::ShouldDisplayDebugInfo() const {
	idPlayer* player = gameLocal.GetLocalPlayer();
	return ( player && parent == player->GetProxyEntity() );
}

/*
===============================================================================

sdVehiclePartScripted

===============================================================================
*/

CLASS_DECLARATION( sdVehiclePartSimple, sdVehiclePartScripted )
END_CLASS

/*
================
sdVehiclePartScripted::Init
================
*/
void sdVehiclePartScripted::Init( const sdDeclVehiclePart& part, sdTransport* _parent ) {
	sdVehiclePartSimple::Init( part, _parent );

	onKilled		= NULL;
	onPostDamage	= NULL;

	if ( !scriptObject ) {
		const char *name = part.data.GetString( "scriptObject" );
		if ( *name ) {
			scriptObject	= gameLocal.program->AllocScriptObject( this, name );
		}
	}

	if ( scriptObject ) {
		onKilled		= scriptObject->GetFunction( "OnKilled" );
		onPostDamage	= scriptObject->GetFunction( "OnPostDamage" );
	}
}

/*
================
sdVehiclePartScripted::OnKilled
================
*/
void sdVehiclePartScripted::OnKilled( void ) {
	if ( onKilled ) {
		sdScriptHelper helper;
		scriptObject->CallNonBlockingScriptEvent( onKilled, helper );
	}
}

/*
================
sdVehiclePartScripted::Damage
================
*/
void sdVehiclePartScripted::Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision ) {
	int oldHealth = health;
	sdVehiclePartSimple::Damage( damage, inflictor, attacker, dir, collision );

	if ( onPostDamage ) {
		sdScriptHelper helper;
		helper.Push( attacker ? attacker->GetScriptObject() : NULL );
		helper.Push( oldHealth );
		helper.Push( health );
		scriptObject->CallNonBlockingScriptEvent( onPostDamage, helper );
	}
}

/*
===============================================================================

	sdVehicleRigidBodyPartSimple

===============================================================================
*/

CLASS_DECLARATION( sdVehiclePart, sdVehicleRigidBodyPartSimple )
END_CLASS


/*
================
sdVehicleRigidBodyPartSimple::Init
================
*/
void sdVehicleRigidBodyPartSimple::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	parent = _parent;

	sdVehiclePart::Init( part );

	joint = _parent->GetAnimator()->GetJointHandle( part.data.GetString( "joint" ) );

	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehiclePartSimple::Init Invalid Joint Name '%s'", part.data.GetString( "joint" ) );
	}

	partBounds = CalcSurfaceBounds( joint );
}

/*
================
sdVehicleRigidBodyPartSimple::GetParent
================
*/
sdTransport* sdVehicleRigidBodyPartSimple::GetParent( void ) const {
	return parent;
}

/*
================
sdVehicleRigidBodyPartSimple::GetWorldOrigin
================
*/
void sdVehicleRigidBodyPartSimple::GetWorldOrigin( idVec3& vec ) {
	sdTransport* transport = GetParent();

	if( !transport ) {
		vec.Zero();
		return;
	}

	transport->GetWorldOrigin( joint, vec );

}

/*
================
sdVehicleRigidBodyPartSimple::GetWorldPhysicsOrigin
================
*/
void sdVehicleRigidBodyPartSimple::GetWorldPhysicsOrigin( idVec3& vec ) {
	GetWorldOrigin( vec );

	// transform this to be relative to the physics. can't use the render info as physics input!
	const idVec3& physicsOrigin = parent->GetPhysics()->GetOrigin();
	const idVec3& renderOrigin = parent->GetRenderEntity()->origin;
	const idMat3& physicsAxis = parent->GetPhysics()->GetAxis();
	const idMat3& renderAxis = parent->GetRenderEntity()->axis;

	vec = physicsAxis * renderAxis.TransposeMultiply( vec - renderOrigin ) + physicsOrigin;
}

/*
===============================================================================

	sdVehicleRigidBodyPart

===============================================================================
*/

CLASS_DECLARATION( sdVehiclePart, sdVehicleRigidBodyPart )
END_CLASS

/*
================
sdVehicleRigidBodyPart::Init
================
*/
void sdVehicleRigidBodyPart::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	parent = _parent;

	sdVehiclePart::Init( part );

	idClipModel* cm = NULL;

	const char* clipModelName = part.data.GetString( "cm_model" );
	if ( *clipModelName ) {
		idTraceModel trm;
		if ( !gameLocal.clip.LoadTraceModel( clipModelName, trm ) ) {
			gameLocal.Error( "sdVehicleRigidBodyPart::Init Could not convert '%s' to a trace model", clipModelName );
		}

		cm = new idClipModel( trm, false );

	} else {
		idTraceModel trm;
		
		idVec3 mins = part.data.GetVector( "mins" );
		idVec3 maxs = part.data.GetVector( "maxs" );

		idBounds bounds( mins, maxs );
		if ( bounds.GetVolume() < 0.0f ) {
			gameLocal.Warning( "sdVehicleRigidBodyPart::Init Invalid mins/maxs: Volume is negative!" );

			// fix the bounds so the volume isn't negative
			bounds.Clear();
			bounds.AddPoint( mins );
			bounds.AddPoint( maxs );
		}

		const char* typeName = part.data.GetString( "type", "box" );
		if ( !idStr::Icmp( typeName, "box" ) ) {
			trm.SetupBox( bounds );
		} else if ( !idStr::Icmp( typeName, "cylinder" ) ) {
			trm.SetupCylinder( bounds, part.data.GetInt( "sides" ), part.data.GetFloat( "angleOffset" ), part.data.GetInt( "option" ) );
		} else if ( !idStr::Icmp( typeName, "frustum" ) ) {
			trm.SetupFrustum( bounds, part.data.GetFloat( "topOffset" ) );
		} else {
			gameLocal.Error( "sdVehicleRigidBodyPart::Init Invalid Rigid Body Part Type '%s'", typeName );
		}

		cm = new idClipModel( trm, false );
	}

	sdPhysics_RigidBodyMultiple& rigidBody =  *_parent->GetRBPhysics();

	int index = rigidBody.GetNumClipModels();
	bodyId = index;

	rigidBody.SetClipModel( cm, 1.f, bodyId );
	rigidBody.SetContactFriction( bodyId, part.data.GetVector( "contactFriction" ) );
	rigidBody.SetMass( part.data.GetFloat( "mass", "1" ), bodyId );
	rigidBody.SetBodyOffset( bodyId, part.data.GetVector( "offset" ) );
	rigidBody.SetBodyBuoyancy( bodyId, part.data.GetFloat( "buoyancy", "0.01" ) );
	rigidBody.SetBodyWaterDrag( bodyId, part.data.GetFloat( "waterDrag", "0" ) );
	if ( part.data.GetBool( "noCollision" ) ) {
		rigidBody.SetClipMask( 0, bodyId );
		rigidBody.SetContents( 0, bodyId );
	} else {
		rigidBody.SetClipMask( MASK_VEHICLESOLID | CONTENTS_MONSTER, bodyId );
		rigidBody.SetContents( CONTENTS_PLAYERCLIP | CONTENTS_IKCLIP | CONTENTS_VEHICLECLIP | CONTENTS_FLYERHIVECLIP, bodyId );
	}

	joint = _parent->GetAnimator()->GetJointHandle( part.data.GetString( "joint" ) );
	partBounds = CalcSurfaceBounds( joint );
}

/*
================
sdVehicleRigidBodyPart::GetParent
================
*/
sdTransport* sdVehicleRigidBodyPart::GetParent( void ) const {
	return parent;
}

/*
================
sdVehicleRigidBodyPart::sdVehicleRigidBodyPart
================
*/
void sdVehicleRigidBodyPart::GetWorldOrigin( idVec3& vec ) {
	parent->GetRBPhysics()->GetBodyOrigin( vec, bodyId );
}

/*
============
sdVehicleRigidBodyPartSimple::ShouldDisplayDebugInfo
============
*/
bool sdVehicleRigidBodyPartSimple::ShouldDisplayDebugInfo() const {
	idPlayer* player = gameLocal.GetLocalPlayer();
	return ( player && parent == player->GetProxyEntity() );
}

/*
===============================================================================

	sdVehicleRigidBodyWheel

===============================================================================
*/

CLASS_DECLARATION( sdVehicleRigidBodyPart, sdVehicleRigidBodyWheel )
END_CLASS

/*
================
sdVehicleRigidBodyWheel::sdVehicleRigidBodyWheel
================
*/
sdVehicleRigidBodyWheel::sdVehicleRigidBodyWheel( void ) {
	frictionAxes.Identity();
	currentFriction.Zero();
	wheelModel = NULL;
	suspension = NULL;
	wheelOffset = 0;
	totalWheels = -1;
	memset( &groundTrace, 0, sizeof( groundTrace ) );

	wheelFractionMemory.SetNum( MAX_WHEEL_MEMORY );
	for ( int i = 0; i < MAX_WHEEL_MEMORY; i++ ) {
		wheelFractionMemory[ i ] = 1.0f;
	}
	currentMemoryFrame = 0;
	currentMemoryIndex = 0;

	suspensionInterface.Init( this );
}

/*
================
sdVehicleRigidBodyWheel::~sdVehicleRigidBodyWheel
================
*/
sdVehicleRigidBodyWheel::~sdVehicleRigidBodyWheel( void ) {
	gameLocal.clip.DeleteClipModel( wheelModel );
	delete suspension;
}

/*
================
sdVehicleRigidBodyWheel::Init
================
*/
void sdVehicleRigidBodyWheel::TrackWheelInit( const sdDeclVehiclePart& track, int index, sdTransport_RB* _parent ) {
	sdVehicleRigidBodyPartSimple::Init( track, _parent );

	name							= track.data.GetString( va( "wheel_joint_%i", index + 1 ) );
	health = maxHealth = -1;
	brokenPart = NULL;
	partBounds.Clear();

	joint = _parent->GetAnimator()->GetJointHandle( name.c_str() );

	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleRigidBodyWheel::TrackWheelInit Invalid Joint Name '%s'", name.c_str() );
	}

	partBounds = CalcSurfaceBounds( joint );
	CommonInit( track );

	sdVehicleSuspension_Vertical* verticalSuspension = new sdVehicleSuspension_Vertical();
	suspension = verticalSuspension;
	if ( verticalSuspension != NULL ) {
		verticalSuspension->Init( &suspensionInterface, track.data.GetString( va( "wheel_suspension_%i", index + 1 ) ) );
	}

	traceIndex = track.data.GetInt( va( "wheel_trace_index_%i", index + 1 ), "-1" );

	wheelFlags.partOfTrack = true;
}

/*
================
sdVehicleRigidBodyWheel::Init
================
*/
void sdVehicleRigidBodyWheel::Init( const sdDeclVehiclePart& wheel, sdTransport_RB* _parent ) {
	sdVehicleRigidBodyPartSimple::Init( wheel, _parent );

	CommonInit( wheel );

	const sdDeclStringMap* suspensionInfoDecl = gameLocal.declStringMapType[ wheel.data.GetString( "suspension" ) ];
	if ( suspensionInfoDecl != NULL ) {
		suspension = sdVehicleSuspension::GetSuspension( suspensionInfoDecl->GetDict().GetString( "type" ) );
		if ( suspension != NULL ) {
			suspension->Init( &suspensionInterface, suspensionInfoDecl->GetDict() );
		}
	}

	traceIndex = wheel.data.GetInt( "trace_index", "-1" );

	wheelFlags.partOfTrack = false;
}

/*
================
sdVehicleRigidBodyWheel::CommonInit
================
*/
void sdVehicleRigidBodyWheel::CommonInit( const sdDeclVehiclePart& part ) {

	if ( part.data.GetBool( "turn" ) ) {
		wheelFlags.hasSteering = true;
		wheelFlags.inverseSteering = false;
	} else if ( part.data.GetBool( "inverseturn" ) ) {
		wheelFlags.hasSteering = true;
		wheelFlags.inverseSteering = true;
	} else {
		wheelFlags.hasSteering = false;
		wheelFlags.inverseSteering = false;
	}

	wheelFlags.noPhysics			= part.data.GetBool( "noPhysics" );
	wheelFlags.hasDrive				= part.data.GetBool( "drive" );
	wheelFlags.noRotation			= part.data.GetBool( "noRotation" );
	wheelFlags.slowsOnLeft			= part.data.GetBool( "slowOnLeft" );
	wheelFlags.slowsOnRight			= part.data.GetBool( "slowOnRight" );

	angle							= 0.f;
	steerAngle						= 0.f;
	idealSteerAngle					= 0.f;
	state.moving					= false;
	state.changed					= true;
	state.steeringChanged			= false;
	state.grounded					= false;
	state.suspensionDisabled		= false;
	radius							= part.data.GetFloat( "radius" );
	rotationspeed					= 0.f;
	normalFriction					= part.data.GetVector( "contactFriction" );
	currentFriction					= normalFriction;
	steerScale						= part.data.GetFloat( "steerScale", "1" );

	suspensionInfo.velocityScale	= part.data.GetFloat( "suspensionVelocityScale", "1" );
	suspensionInfo.kCompress		= part.data.GetFloat( "suspensionKCompress" );
	suspensionInfo.upTrace			= part.data.GetFloat( "suspensionUpTrace" );
	suspensionInfo.downTrace		= part.data.GetFloat( "suspensionDownTrace" );
	suspensionInfo.totalDist		= suspensionInfo.upTrace + suspensionInfo.downTrace;
	suspensionInfo.damping			= part.data.GetFloat( "suspensionDamping" );
	suspensionInfo.base				= part.data.GetFloat( "suspensionBase" );
	suspensionInfo.range			= part.data.GetFloat( "suspensionRange" );
	suspensionInfo.maxRestVelocity	= part.data.GetFloat( "suspensionMaxRestVelocity", "5" );
	suspensionInfo.aggressiveDampening	= part.data.GetBool( "aggressiveDampening" );
	suspensionInfo.slowScale		= part.data.GetFloat( "slowScale", "0" );
	suspensionInfo.slowScaleSpeed	= part.data.GetFloat( "slowScaleSpeed", "400" );
	suspensionInfo.hardStopScale	= 1.0f / Max( 1.0f, part.data.GetFloat( "hardStopFrames", "4" ) );
	suspensionInfo.alternateSuspensionModel		= part.data.GetBool( "alternateSuspensionModel" );

	wheelSpinForceThreshold			= part.data.GetFloat( "wheelSpinForceThreshhold" );
	wheelSkidVelocityThreshold		= part.data.GetFloat( "wheelSkidVelocityThreshold", "150" );

	state.setSteering				= part.data.GetBool( "control_steering" );

	brakingForce					= part.data.GetFloat( "brakingForce", "500000" );
	handBrakeSlipScale				= part.data.GetFloat( "handBrakeSlipScale", "1" );
	maxSlip							= part.data.GetFloat( "maxSlip", "100000" );
	wheelFlags.hasHandBrake			= part.data.GetBool( "hasHandBrake" );

	traction.AssureSize( gameLocal.declSurfaceTypeType.Num() );
	int i;
	for ( i = 0; i < gameLocal.declSurfaceTypeType.Num(); i++ ) {
		traction[ i ] = part.data.GetFloat( va( "traction_%s", gameLocal.declSurfaceTypeType[ i ]->GetName() ), "1" );
	}

	static idVec3 rightWheelWinding[ 4 ] = {
		idVec3(  1.0f, 0.f, 0.0f ),
		idVec3( -1.0f, 0.f, 0.0f ),
		idVec3( -1.0f, 1.f, 0.0f ),
		idVec3(  1.0f, 1.f, 0.0f )
	};

	static idVec3 leftWheelWinding[ 4 ] = {
		idVec3(  1.0f, -1.f, 0.0f ),
		idVec3( -1.0f, -1.f, 0.0f ),
		idVec3( -1.0f, 0.f, 0.0f ),
		idVec3(  1.0f, 0.f, 0.0f )
	};

	parent->GetAnimator()->GetJointTransform( joint, gameLocal.time, baseOrg, baseAxes );
	baseOrgOffset = part.data.GetVector( "base_org_offset" );

	if ( part.data.GetBool( "localRotation" ) ) {
		rotationAxis = baseAxes[ 0 ];
	} else {
		rotationAxis.Set( 0.f, -1.f, 0.f );
	}

	wheelFlags.isLeftWheel = baseOrg[ 1 ] < 0;
	wheelFlags.isFrontWheel = baseOrg[ 0 ] > 0;

	idVec3* wheelWinding = IsLeftWheel() ? leftWheelWinding : rightWheelWinding;

	idVec3 verts[ 4 ];
	float footprint = part.data.GetFloat( "footprint" );
	if ( footprint < idMath::FLT_EPSILON ) {
		gameLocal.Error( "sdVehicleRigidBodyWheel::CommonInit \"footprint\" too small: %.6f in vscript: %s", footprint, parent->GetVehicleScript()->GetName() );
	}

	for ( i = 0; i < 4; i++ ) {
		verts[ i ] = wheelWinding[ i ] * footprint;
	}
	idTraceModel trm;
	trm.SetupPolygon( verts, 4 );
	wheelModel = new idClipModel( trm, false );

	idIK* ik = parent->GetIK();
	if ( ik && ik->IsType( sdIK_WheeledVehicle::Type ) ) {
		sdIK_WheeledVehicle* wheelIK = reinterpret_cast< sdIK_WheeledVehicle* >( ik );
		wheelIK->AddWheel( *this );
	}

	treadId = 0;

	parent->InitEffectList( dustEffects, "fx_wheeldust", numSurfaceTypesAtSpawn );
	parent->InitEffectList( spinEffects, "fx_wheelspin", numSurfaceTypesAtSpawn );
	parent->InitEffectList( skidEffects, "fx_skid", numSurfaceTypesAtSpawn );

	stroggTread = parent->spawnArgs.GetBool( "stroggTread" );
}

/*
================
sdVehicleRigidBodyWheel::GetLinearSpeed
================
*/
float sdVehicleRigidBodyWheel::GetLinearSpeed( void ) {
	idVec3 temp;
	parent->GetRBPhysics()->GetPointVelocity( groundTrace.c.point, temp );
	temp -= ( temp * groundTrace.c.normal ) * groundTrace.c.normal;
	return temp * ( frictionAxes * parent->GetRBPhysics()->GetAxis()[ 0 ] );
}

idCVar cm_drawTraces( "cm_drawTraces", "0", CVAR_GAME | CVAR_BOOL, "draw polygon and edge normals" );
idCVar g_skipVehicleFrictionFeedback( "g_skipVehicleFrictionFeedback", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "ignore the effects of surface friction" );
idCVar g_debugVehicleFrictionFeedback( "g_debugVehicleFrictionFeedback", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "show info about wheeled surface friction feedback" );
idCVar g_debugVehicleDriveForces( "g_debugVehicleDriveForces", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "show info about wheeled drive forces" );
idCVar g_debugVehicleWheelForces( "g_debugVehicleWheelForces", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "show info about wheel forces" );

idCVar g_vehicleWheelTracesPerFrame( "g_vehicleWheelTracesPerFrame", "0.5", CVAR_GAME | CVAR_FLOAT | CVAR_CHEAT, "What fraction of the wheels are updated per frame" );

struct cm_logger_t {
};

/*
================
sdVehicleRigidBodyWheel::UpdateSuspension
================
*/
void sdVehicleRigidBodyWheel::UpdateSuspension( const sdVehicleInput& input ) {
	sdPhysics_RigidBodyMultiple& physics = *parent->GetRBPhysics();

	if ( totalWheels == -1 ) {
		idIK* ik = parent->GetIK();
		if ( ik && ik->IsType( sdIK_WheeledVehicle::Type ) ) {
			sdIK_WheeledVehicle* wheelIK = reinterpret_cast< sdIK_WheeledVehicle* >( ik );
			totalWheels = wheelIK->GetNumWheels();
		}

		// auto-assign for 4 wheeled vehicles
		if ( traceIndex == -1 && totalWheels == 4 ) {
			if ( wheelFlags.isLeftWheel ) {
				if ( wheelFlags.isFrontWheel ) {
					traceIndex = 0;
				} else {
					traceIndex = 3;
				}
			}

			if ( !wheelFlags.isLeftWheel ) {
				if ( wheelFlags.isFrontWheel ) {
					traceIndex = 2;
				} else {
					traceIndex = 1;
				}
			}
		}
	}

	if( HasSteering() ) {		
		idealSteerAngle = input.GetSteerAngle();
		
		if( HasInverseSteering() ) {
			idealSteerAngle = -idealSteerAngle;
		}
		idealSteerAngle *= steerScale;
	} else {
		idealSteerAngle = 0.f;
	}

	if ( idealSteerAngle != steerAngle ) {

		steerAngle = idealSteerAngle;

		if ( state.setSteering ) {
			parent->SetSteerVisualAngle( steerAngle );
		}
		
		state.steeringChanged = true;
		idAngles::YawToMat3( -steerAngle, frictionAxes );
	}
	
	if ( !input.GetBraking() && !input.GetHandBraking() && HasDrive() ) {
		physics.Activate();
	}

	if ( !parent->GetPhysics()->IsAtRest() ) {

		idVec3 org = parent->GetPhysics()->GetOrigin();
		idMat3 axis = parent->GetPhysics()->GetAxis();
		org = org + ( ( baseOrg + baseOrgOffset ) * axis );

		idVec3 start = org + ( axis[ 2 ] * suspensionInfo.upTrace );
		idVec3 end = org - ( axis[ 2 ] * suspensionInfo.downTrace );

		bool doTraceThisFrame = false;
		if ( traceIndex != -1 && g_vehicleWheelTracesPerFrame.GetFloat() < 1.0f ) {
			// find out if this wheel is to be updated this frame
			assert( totalWheels != -1 );
			int numPerFrame = Max( 1, ( int )( g_vehicleWheelTracesPerFrame.GetFloat() * totalWheels ) );
			if ( parent->GetAORPhysicsLOD() >= 2 ) {
				numPerFrame = 1;
			}
			const int wheelToUpdate = ( gameLocal.framenum * numPerFrame );
			const int nextWheelToUpdate = wheelToUpdate + numPerFrame;
			for ( int i = wheelToUpdate; i < nextWheelToUpdate; i++ ) {
				if ( i % totalWheels == traceIndex ) {
					doTraceThisFrame = true;
					break;
				}
			}
		} else {
			doTraceThisFrame = true;
		}

		if ( !doTraceThisFrame ) {
			// check if the previous frame is in the memory bank
			int prevFrameWrap = ( gameLocal.framenum - 1 ) / MAX_WHEEL_MEMORY;
			int prevFrameIndex = ( gameLocal.framenum - 1 ) % MAX_WHEEL_MEMORY;
			int curFrameWrap = currentMemoryFrame / MAX_WHEEL_MEMORY;

			if ( prevFrameWrap < curFrameWrap - 1 ) {
				// too far in the past
				doTraceThisFrame = true;
			} else if ( prevFrameWrap == curFrameWrap - 1 ) {
				// in the previous wrap - check that the index
				// is greater than the current one
				if ( prevFrameIndex <= currentMemoryIndex ) {
					// too far in the past
					doTraceThisFrame = true;
				}
			} else {
				// in the current wrap - check that its before this one
				if ( gameLocal.framenum - 1 > currentMemoryFrame ) {
					// WTF? Future?
					doTraceThisFrame = true;
				}
			}
		}

		if ( doTraceThisFrame ) {
			memset( &groundTrace, 0, sizeof( groundTrace ) );
			if ( parent->GetAORPhysicsLOD() == 0 ) {
				physics.GetTraceCollection().Translation( CLIP_DEBUG_PARMS groundTrace, start, end, wheelModel, axis, MASK_VEHICLESOLID | CONTENTS_MONSTER );
			} else {
				// cheaper point trace
				physics.GetTraceCollection().Translation( CLIP_DEBUG_PARMS groundTrace, start, end, NULL, mat3_identity, MASK_VEHICLESOLID | CONTENTS_MONSTER );
			}

			if( g_debugVehicleWheelForces.GetBool() && ShouldDisplayDebugInfo() ) {
				gameRenderWorld->DebugArrow( colorGreen, start, groundTrace.endpos, 10 );
				gameRenderWorld->DebugArrow( colorRed, groundTrace.endpos, end, 10 );
				if ( parent->GetAORPhysicsLOD() == 0 ) {
					wheelModel->Draw( groundTrace.endpos, axis );
				}
			}

			// store the fraction in the memory bank
			currentMemoryFrame = gameLocal.framenum;
			currentMemoryIndex = gameLocal.framenum % MAX_WHEEL_MEMORY;
			wheelFractionMemory[ currentMemoryIndex ] = groundTrace.fraction;
		} else {
			// get the fraction from the previous frame
			int prevFrameIndex = ( gameLocal.framenum - 1 ) % MAX_WHEEL_MEMORY;
			float prevFraction = wheelFractionMemory[ prevFrameIndex ];

			// fake it to seem like it did a trace this frame
			idVec3 newEnd = Lerp( start, end, prevFraction );

			if ( prevFraction < 1.0f ) {
				idVec3 vel;
				physics.GetPointVelocity( newEnd, vel );
				float suspensionDelta = ( vel * axis[ 2 ] )*MS2SEC( gameLocal.msec );
				newEnd -= axis[ 2 ] * suspensionDelta * 0.5f;
			}

			// calculate the new fraction
			groundTrace.fraction = ( ( newEnd - start )*axis[ 2 ] ) / ( ( end - start )*axis[ 2 ] );
			groundTrace.endpos = newEnd;

			// store the fraction in the memory bank
			currentMemoryFrame = gameLocal.framenum;
			currentMemoryIndex = gameLocal.framenum % MAX_WHEEL_MEMORY;
			wheelFractionMemory[ currentMemoryIndex ] = groundTrace.fraction;
		}

		groundTrace.c.point = groundTrace.endpos;
		groundTrace.c.selfId = -1;

		wheelOffset = suspensionInfo.upTrace + radius - ( suspensionInfo.totalDist * groundTrace.fraction );

		state.rested = true;
		state.spinning = false;
		state.skidding = false;
		
		if ( groundTrace.fraction != 1.0f ) {
			CalcForces( suspensionForce, suspensionVelocity );

			if ( idMath::Fabs( suspensionVelocity ) > suspensionInfo.maxRestVelocity ) {
				state.rested = false;
				parent->GetPhysics()->Activate();
			} else {
				suspensionVelocity = 0.f;
			}
		}

		state.grounded = groundTrace.fraction != 1.f;

		if ( suspension ) {
			suspension->Update();
		}
	} else {
		state.rested = true;
	}

	currentFriction = normalFriction;
	UpdateFriction( input );
}

/*
================
sdVehicleRigidBodyWheel::UpdateMotor
================
*/
void sdVehicleRigidBodyWheel::UpdateMotor( const sdVehicleInput& input, float inputMotorForce ) {
	if ( parent->IsFrozen() ) {
		state.skidding = false;
		state.spinning = false;
		state.moving = false;
		state.rested = true;
		return;
	}

	sdPhysics_RigidBodyMultiple& physics = *parent->GetRBPhysics();
	motorForce = 0.f;
	motorSpeed = 0.f;
	if ( !input.GetBraking() && !input.GetHandBraking() ) {
		if( HasDrive() ) {
			motorForce = idMath::Fabs( inputMotorForce );
			motorSpeed = GetInputSpeed( input );

			// adjust wheel velocity for better steering because there are no differentials between the wheels
			if(( steerAngle < 0.0f && SlowsOnLeft()) || 
				( steerAngle > 0.0f && SlowsOnRight() )) {
				motorSpeed *= 0.5f;
			}
		}
	}

	bool braking = input.GetBraking();
	bool handBraking = wheelFlags.hasHandBrake && input.GetHandBraking();

	if ( braking || handBraking ) {
		motorForce = brakingForce;
		motorSpeed = 0.0f;

		if ( !( handBraking && !braking ) ) {
			float scale = ( idMath::Sqrt( 1.0f - groundTrace.c.normal.z ) )*5.0f;
			motorForce *= scale + 1.5f;
		}
	}

	if ( !parent->GetPhysics()->IsAtRest() ) {
		if ( groundTrace.fraction != 1.0f ) {
			if( !( braking || handBraking ) && groundTrace.c.surfaceType ) {
				if( motorForce > 0.0f && ( motorForce / traction[ groundTrace.c.surfaceType->Index() ]  > wheelSpinForceThreshold )) {
					state.spinning = true;
				}
			}

			UpdateSkidding();
		}
	}
}

/*
================
sdVehicleRigidBodyWheel::UpdatePrePhysics
================
*/
void sdVehicleRigidBodyWheel::UpdatePrePhysics( const sdVehicleInput& input ) {
	if ( wheelFlags.noPhysics || state.suspensionDisabled ) {
		return;
	}

	UpdateSuspension( input );
	UpdateMotor( input, input.GetForce() );

	if ( !parent->GetPhysics()->IsAtRest() ) {
		if( g_debugVehicleDriveForces.GetBool() && ShouldDisplayDebugInfo() ) {
			idVec3 org = parent->GetPhysics()->GetOrigin();
			idMat3 axis = parent->GetPhysics()->GetAxis();
			org = org + ( ( baseOrg + baseOrgOffset ) * axis );

			gameRenderWorld->DrawText( va( "motor sp: %.2f\nmotor f: %.2f\n", motorSpeed, motorForce ), org, 0.25f, colorGreen, axis, 0 );
		}
	}

	if ( !state.skidding && treadId ) {
		tireTreadManager->StopSkid( treadId );
		treadId = 0;
	}
}

/*
================
sdVehicleRigidBodyWheel::UpdateFriction
================
*/
void sdVehicleRigidBodyWheel::UpdateFriction( const sdVehicleInput& input ) {
	sdPhysics_RigidBodyMultiple& physics = *parent->GetRBPhysics();

	idVec3 wheelOrigin = groundTrace.c.point;
	idMat3 worldFrictionAxes = frictionAxes * physics.GetAxis();

	// vary the lateral friction with the lateral slip
	idVec3 contactWRTground;
	physics.GetPointVelocity( wheelOrigin, contactWRTground );
	idVec3 carWRTground = physics.GetLinearVelocity();
	float lateralSlip = ( contactWRTground - carWRTground ) * worldFrictionAxes[ 1 ];


	idVec3 slidingFriction = normalFriction * 0.8f;			// assume kinetic friction is 80% of static friction

	// make it much more sensitive to slip when handbraking
	if ( wheelFlags.hasHandBrake && input.GetHandBraking() ) {
		lateralSlip *= handBrakeSlipScale;
		slidingFriction *= 0.5f;
	}

	float slideFactor = ( idMath::Fabs( lateralSlip ) ) / maxSlip;
	slideFactor = idMath::ClampFloat( 0.0f, 1.0f, slideFactor );
	currentFriction = Lerp( normalFriction, slidingFriction, slideFactor );
}

/*
================
sdVehicleRigidBodyWheel::UpdateSkidding
================
*/
void sdVehicleRigidBodyWheel::UpdateSkidding() {
	idBounds bb;
	
	GetBounds( bb );

	if ( bb.IsCleared() ) {
		return;
	}

	//
	// get info about the parent
	//
	idPhysics* physics = GetParent()->GetPhysics();
	const idVec3& origin = physics->GetOrigin();
	const idMat3& axis = physics->GetAxis();
	idMat3 axisTranspose = physics->GetAxis().Transpose();
	const idVec3& velocity = physics->GetLinearVelocity();
	const idVec3& angVel = physics->GetAngularVelocity();
	const idVec3& com = physics->GetCenterOfMass() * axis + origin;

	//
	// get info about self
	//
	idVec3 delta = ( groundTrace.c.point - com ) * axisTranspose;

	idVec3 pointVelocity = velocity + angVel.Cross( delta );
	float slipVelocity = idMath::Fabs( pointVelocity * axis[ 1 ] );

	if ( slipVelocity > wheelSkidVelocityThreshold ) {
		state.skidding = true;

		if ( gameLocal.isNewFrame ) {
			if ( treadId == 0 ) {
				treadId = tireTreadManager->StartSkid( stroggTread );
			}
			if ( treadId != 0 ) {
				idVec3 point;
				GetWorldOrigin( point );
				idMat3 wheelaxis;
				GetWorldAxis( wheelaxis );
				point += wheelaxis[2] * (bb[0][2] * 0.8f);
				if ( !tireTreadManager->AddSkidPoint( treadId, point/*groundTrace.c.point*/, pointVelocity, groundTrace.c.normal, groundTrace.c.surfaceType ) ) {
					treadId = 0;
				}
			}
		}

//		gameRenderWorld->DebugArrow( colorYellow, com, com + velocity*0.2f, 8 );
//		gameRenderWorld->DebugArrow( colorRed, com, com + delta * axis, 8 );
//		gameRenderWorld->DebugArrow( colorGreen, groundTrace.c.point, groundTrace.c.point + slipVelocity*axis[ 1 ]*0.2f, 8 );
	} else {
		if ( treadId != 0 && gameLocal.isNewFrame ) {
			tireTreadManager->StopSkid( treadId );
			treadId = 0;
		}
	}
}

/*
================
sdVehicleRigidBodyWheel::UpdatePostPhysics
================
*/
void sdVehicleRigidBodyWheel::UpdatePostPhysics( const sdVehicleInput& input ) {
	if ( !wheelFlags.noRotation ) {
		UpdateRotation( input );
	}

	if ( gameLocal.isNewFrame ) {
		UpdateParticles( input );
	}
}


idCVar g_skipVehicleTurnFeedback( "g_skipVehicleTurnFeedback", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "skip turn ducking effects on wheeled suspensions" );
idCVar g_skipVehicleAccelFeedback( "g_skipVehicleAccelFeedback", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "skip acceleration effects on wheeled suspensions" );
idCVar g_debugVehicleFeedback( "g_debugVehicleFeedback", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "show info about wheeled suspension feedback" );

/*
================
sdVehicleRigidBodyWheel::CalcForces
================
*/
void sdVehicleRigidBodyWheel::CalcForces( float& maxForce, float& velocity ) {
	sdPhysics_RigidBodyMultiple& physics = *parent->GetRBPhysics();

	idVec3 vel;
	physics.GetPointVelocity( groundTrace.endpos, vel );

	float compressionScale = idMath::ClampFloat( 0.0f, 1.0f, 1.0f - ( physics.GetLinearVelocity().Length() / suspensionInfo.slowScaleSpeed ) );
	compressionScale = 1.0f + compressionScale*suspensionInfo.slowScale;

	float springLength = groundTrace.fraction * suspensionInfo.totalDist;

	float loss = suspensionInfo.totalDist - suspensionInfo.range;
	springLength -= loss;

	float springForce = 0.0f;
	float invTimeStep = 1.0f / MS2SEC( gameLocal.msec );
	float shockVelocity = vel * physics.GetAxis()[ 2 ];

	// scale down the spring force allowed to be applied after reattaching wheels
	float raiseUpFactor = 1.0f;
	if ( reattachTime != 0 ) {
		raiseUpFactor = idMath::ClampFloat( 0.0f, 1.0f, ( gameLocal.time - reattachTime ) / 1000.0f );
	}

	if ( springLength < 0.f && raiseUpFactor >= 1.0f ) {
		// hard-stop
		velocity = Min( suspensionInfo.range * -0.1f, springLength ) * ( invTimeStep * suspensionInfo.hardStopScale );
		springForce = idMath::INFINITY * 0.5f;;
	} else {

		if ( !suspensionInfo.alternateSuspensionModel ) {
			// regular
			float compression = idMath::Sqrt( Max( 0.0f, ( suspensionInfo.range - springLength ) / suspensionInfo.range ) ) * suspensionInfo.range;
			if ( !suspensionInfo.aggressiveDampening ) {
				springForce = ( compression * compression * suspensionInfo.kCompress * compressionScale ) + suspensionInfo.base;

				float s = 1.f - ( springLength / suspensionInfo.range );
				s = s * s;

				s = ( s * suspensionInfo.velocityScale * ( 1.0f / ( 3.0f * ( compressionScale - 1.0f ) +1.0f ) ) ) + 1.f;
				float scale = s / ( float )gameLocal.msec;
				velocity = -( compression - ( shockVelocity * suspensionInfo.damping * compressionScale ) ) * scale;
			} else {
				// the "agressive dampening" version does it differently
				springForce = ( compression * compression * suspensionInfo.kCompress * compressionScale ) + suspensionInfo.base;
				springForce -= shockVelocity * suspensionInfo.damping * compressionScale;
				velocity = 0.0f;
			}

		} else {
			float compression = suspensionInfo.range - springLength;
			springForce = compression * suspensionInfo.kCompress - suspensionInfo.damping * shockVelocity;

			// velocity needs to be the velocity this should be moving at. we can predict this, roughly
			velocity = shockVelocity - MS2SEC( gameLocal.msec ) * springForce / ( parent->GetPhysics()->GetMass() / totalWheels );
			velocity /= compressionScale;

			springForce = springForce * compressionScale;
		}

		springForce *= raiseUpFactor;
	}

	if ( springForce < 0.0f ) {
		springForce = 0.0f;
	}

	if( g_debugVehicleWheelForces.GetBool() && ShouldDisplayDebugInfo() ) {
		idVec3 org = parent->GetPhysics()->GetOrigin();
		idMat3 axis = parent->GetPhysics()->GetAxis();
		org = org + ( ( baseOrg + baseOrgOffset ) * axis );
		gameRenderWorld->DrawText( va( "velocity: %.2f\nmaxForce: %.2f", velocity, maxForce ), org, 0.25f, colorGreen, axis );
	}

	maxForce = springForce;
}

/*
================
sdVehicleRigidBodyWheel::EvaluateContacts
================
*/
int sdVehicleRigidBodyWheel::EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max ) {
	if ( IsHidden() || max < 1 || state.suspensionDisabled ) {
		return 0;
	}

	if ( state.grounded ) {

		listExt[ 0 ].contactForceMax		= suspensionForce;
		listExt[ 0 ].contactForceVelocity	= suspensionVelocity;

		listExt[ 0 ].contactFriction		= currentFriction;
		listExt[ 0 ].frictionAxes			= frictionAxes;
		listExt[ 0 ].motorForce				= motorForce;

		if ( groundTrace.c.surfaceType ) {
			listExt[ 0 ].motorForce *= traction[ groundTrace.c.surfaceType->Index() ];
		}
		if ( groundTrace.fraction < 1.0f ) {
			// smoothly scale down the force applied, based on the slope of the surface
			// so shallow slopes keep much of the force, but steep slopes rapidly lose traction
			float temp = idMath::ClampFloat( 0.0f, 1.0f, groundTrace.c.normal.z );
			float normalScaleDown = -0.5f*idMath::Cos( temp*temp*temp*idMath::PI ) + 0.5f;
			if ( normalScaleDown < 0.00001f ) {
				normalScaleDown = 0.0f;
			}

			listExt[ 0 ].motorForce *= normalScaleDown;
			listExt[ 0 ].contactForceMax *= normalScaleDown;
		}

		listExt[ 0 ].motorDirection			= frictionAxes[ 0 ];
		listExt[ 0 ].motorSpeed				= motorSpeed;
		listExt[ 0 ].rested					= state.rested;
		list[ 0 ] = groundTrace.c;
		return 1;
	}
	return 0;
}

/*
================
sdVehicleRigidBodyWheel::GetBaseWorldOrg
================
*/
idVec3 sdVehicleRigidBodyWheel::GetBaseWorldOrg( void ) const {
	return parent->GetRenderEntity()->origin + ( baseOrg * parent->GetRenderEntity()->axis );
}

/*
================
sdVehicleRigidBodyWheel::GetInputSpeed
================
*/
float sdVehicleRigidBodyWheel::GetInputSpeed( const sdVehicleInput& input ) {
	return ( IsLeftWheel() ? input.GetLeftSpeed() : input.GetRightSpeed() );
}

/*
================
sdVehicleRigidBodyWheel::UpdateRotation
================
*/
void sdVehicleRigidBodyWheel::UpdateRotation( float speed ) {
	rotationspeed = speed;

	angle += 360 * ( rotationspeed * MS2SEC( gameLocal.msec ) / ( 2 * idMath::PI * radius ) );

	state.changed |= ( rotationspeed != 0 ) || ( state.steeringChanged );

	state.steeringChanged = false;

	state.moving = idMath::Fabs( speed ) > minParticleCreationSpeed;
}

/*
================
sdVehicleRigidBodyWheel::UpdateRotation
================
*/
void sdVehicleRigidBodyWheel::UpdateRotation( const sdVehicleInput& input ) {
	sdPhysics_RigidBodyMultiple& physics	= *parent->GetRBPhysics();

	float oldangle = angle;

	float s = 0.f;

	if ( state.grounded ) {
		if( !physics.IsAtRest() && !( input.GetHandBraking() && wheelFlags.hasHandBrake ) ) {
			float vel = GetLinearSpeed();
			s = vel;
			if ( groundTrace.c.surfaceType ) {
				s /= traction[ groundTrace.c.surfaceType->Index() ];
			}
			if ( fabs( s ) < 0.5f ) {
				s = 0.f;
			}
		}
	} else {
		if ( !physics.InWater() ) {
			float speed = GetInputSpeed( input );
			if ( HasDrive() && speed ) {
				s = speed;
			} else {
				float timeStep = MS2SEC( gameLocal.msec );
				float rotationSlowingFactor;
				if ( !physics.HasGroundContacts() ) {
					rotationSlowingFactor = 0.2f;
				} else 	if ( gameLocal.msec > 0 ) {
					rotationSlowingFactor = 0.2f / timeStep;
				} else {
					rotationSlowingFactor = 0.0f;
				}
				s = rotationspeed - ( MS2SEC( gameLocal.msec ) * rotationspeed * rotationSlowingFactor );
				if ( idMath::Fabs( s ) < 10.f ) {
					s = 0.f;
				}
			}
		}
	}

	UpdateRotation( s );
}

/*
================
sdVehicleRigidBodyWheel::UpdateParticles
================
*/
void sdVehicleRigidBodyWheel::UpdateParticles( const sdVehicleInput& input ) {
	sdPhysics_RigidBodyMultiple& physics = *parent->GetRBPhysics();

	int surfaceTypeIndex = -1;
	
	if ( groundTrace.fraction < 1.0f ) {
		surfaceTypeIndex = 0;
	}

	if ( groundTrace.c.surfaceType ) {
		surfaceTypeIndex = groundTrace.c.surfaceType->Index() + 1;
	}

	if ( surfaceTypeIndex != -1 ) {
		if ( state.grounded && state.moving ) {
			renderEffect_t& renderEffect = dustEffects[ surfaceTypeIndex ].GetRenderEffect();
			renderEffect.origin = groundTrace.c.point;
	
			float vel = idMath::Fabs( rotationspeed );
			renderEffect.attenuation = vel > 1000.0f ? 1.0f : 0.5f * ( 2 / Square( 1000.0f ) ) * Square( vel );		
			renderEffect.gravity = gameLocal.GetGravity();
			dustEffects[ surfaceTypeIndex ].Start( gameLocal.time );
			dustEffects[ surfaceTypeIndex ].GetNode().AddToEnd( activeEffects );
		} else {
			dustEffects[ surfaceTypeIndex ].Stop();
			dustEffects[ surfaceTypeIndex ].GetNode().Remove();
		}

		if( state.grounded && state.spinning ) {
			renderEffect_t& renderEffect = spinEffects[ surfaceTypeIndex ].GetRenderEffect();
			renderEffect.origin = groundTrace.c.point;
			renderEffect.gravity = gameLocal.GetGravity();

			spinEffects[ surfaceTypeIndex ].Start( gameLocal.time );
			spinEffects[ surfaceTypeIndex ].GetNode().AddToEnd( activeEffects );
		} else {
			spinEffects[ surfaceTypeIndex ].Stop();
			spinEffects[ surfaceTypeIndex ].GetNode().Remove();
		}

		if( state.grounded && state.skidding ) {
			renderEffect_t& renderEffect = skidEffects[ surfaceTypeIndex ].GetRenderEffect();
			renderEffect.origin = groundTrace.c.point;
			renderEffect.gravity = gameLocal.GetGravity();

			skidEffects[ surfaceTypeIndex ].Start( gameLocal.time );
			skidEffects[ surfaceTypeIndex ].GetNode().AddToEnd( activeEffects );

		} else {
			skidEffects[ surfaceTypeIndex ].Stop();
			skidEffects[ surfaceTypeIndex ].GetNode().Remove();
		}
	}

	sdEffect* effect = activeEffects.Next();
	for( ; effect != NULL; effect = effect->GetNode().Next() ) {
		effect->Update();

		// stop playing any effects for surfaces we're no longer on
		if ( ( surfaceTypeIndex == -1  ) || ( effect != &dustEffects[ surfaceTypeIndex ] 
												&& effect != &spinEffects[ surfaceTypeIndex ] 
												&& effect != &skidEffects[ surfaceTypeIndex ] ) ) {
			effect->Stop();
		}
	}	
}


/*
================
sdVehicleRigidBodyWheel::CreateDecayDebris
================
*/
void sdVehicleRigidBodyWheel::CreateDecayDebris( void ) {
	if ( g_disableTransportDebris.GetBool() ) {
		return;
	}

	sdTransport* transport = GetParent();

	idVec3 org;
	idMat3 axis;
	GetWorldOrigin( org );
	GetWorldAxis( axis );

	// make sure we can have another part of this priority
	debrisPriority_t priority = ( debrisPriority_t )brokenPart->dict.GetInt( "priority" );
	if ( !CanAddDebris( priority, org ) ) {
		return;
	}

	float velMin = brokenPart->dict.GetFloat( "decay_wheel_velocity_min", "50" );
	float velMax = brokenPart->dict.GetFloat( "decay_wheel_velocity_max", "80" );
	float avelMin = brokenPart->dict.GetFloat( "decay_wheel_angular_velocity_min", "30" );
	float avelMax = brokenPart->dict.GetFloat( "decay_wheel_angular_velocity_max", "50" );

	// get some axes of the transport
	idVec3 forward = transport->GetPhysics()->GetAxis()[0];
	idVec3 left = transport->GetPhysics()->GetAxis()[1];

	// get the difference between this and the transport's origin
	idVec3 difference = org - transport->GetPhysics()->GetOrigin();

	// vary the falling-off velocity a bit
	float velocity = (gameLocal.random.RandomFloat() * (velMax - velMin)) + velMin;
	idVec3 vel = left * velocity;

	// make an angular velocity about the forwards vector that will cause the wheels to topple over
	float angVelocity = (gameLocal.random.RandomFloat() * (avelMax - avelMin)) + avelMin;
	idVec3 aVel = forward * angVelocity;

	// use the dot product to determine what direction its in
	if (difference * left < 0.0f) {
		vel *= -1.f;
		aVel *= -1.f;
	}

	vel += transport->GetPhysics()->GetLinearVelocity();
	aVel += transport->GetPhysics()->GetAngularVelocity();

	rvClientMoveable* cent = gameLocal.SpawnClientMoveable( brokenPart->GetName(), 5000, org, axis, vel, aVel, 1 );
	if ( cent != NULL ) {
		cent->GetPhysics()->SetContents( 0 );
		cent->GetPhysics()->SetClipMask( CONTENTS_SOLID | CONTENTS_BODY );
	}
}

/*
================
sdVehicleRigidBodyWheel::CheckWater
================
*/
void sdVehicleRigidBodyWheel::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects ) {

		idVec3 mountOrg;
		idMat3 mountAxis;
		parent->GetAnimator()->GetJointTransform( joint, gameLocal.time, mountOrg, mountAxis );
		waterEffects->SetOrigin( mountOrg * parent->GetRenderEntity()->axis + parent->GetRenderEntity()->origin );
		waterEffects->SetAxis( parent->GetRenderEntity()->axis );
		waterEffects->SetMaxVelocity( 200.0f );
		waterEffects->SetVelocity( idVec3( idMath::Abs( rotationspeed ), 0.0f, 0.0f ) );
		waterEffects->CheckWater( parent, waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}

/*
================
sdVehicleRigidBodyWheel::UpdateSuspensionIK
================
*/
bool sdVehicleRigidBodyWheel::UpdateSuspensionIK( void ) {
	if ( suspension != NULL ) {
		return suspension->UpdateIKJoints( parent->GetAnimator() );
	}
	return false;
}

/*
================
sdVehicleRigidBodyWheel::ClearSuspensionIK
================
*/
void sdVehicleRigidBodyWheel::ClearSuspensionIK( void ) {
	if ( suspension != NULL ) {
		suspension->ClearIKJoints( parent->GetAnimator() );
	}
}

/*
===============================================================================

	sdVehicleTrack

===============================================================================
*/

CLASS_DECLARATION( sdVehicleDriveObject, sdVehicleTrack )
END_CLASS

/*
================
sdVehicleTrack::~sdVehicleTrack
================
*/
sdVehicleTrack::~sdVehicleTrack( void ) {
	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		delete wheels[ i ];
	}
}

/*
================
sdVehicleTrack::Init
================
*/
void sdVehicleTrack::Init( const sdDeclVehiclePart& track, sdTransport_RB* _parent ) {
	spawnPart = &track;

	parent			= _parent;
	joint			= parent->GetAnimator()->GetJointHandle( track.data.GetString( "joint" ) );
	direction		= track.data.GetVector( "direction" );
	shaderParmIndex = track.data.GetInt( "shaderParmIndex" );
	name			= track.data.GetString( "name" );

	idVec3 origin;
	parent->GetAnimator()->GetJointTransform( joint, gameLocal.time, origin );
	leftTrack = origin[ 1 ] > 0;

	parent->GetRenderEntity()->shaderParms[ shaderParmIndex ] = 0;
}

/*
================
sdVehicleTrack::GetParent
================
*/
sdTransport* sdVehicleTrack::GetParent( void ) const {
	return parent;
}

/*
================
sdVehicleTrack::GetInputSpeed
================
*/
float sdVehicleTrack::GetInputSpeed( const sdVehicleInput& input ) const {
	return ( IsLeftTrack() ? input.GetLeftSpeed() : input.GetRightSpeed() );
}

/*
================
sdVehicleTrack::UpdatePrePhysics
================
*/
void sdVehicleTrack::UpdatePrePhysics( const sdVehicleInput& input ) {
	int numOnGround = 0;
	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		wheels[ i ]->UpdateSuspension( input );
		if ( wheels[ i ]->IsGrounded() ) {
			numOnGround++;
		}
	}

	float inputMotorForce = input.GetForce();
	float maxInputMotorForce = inputMotorForce * 2.0f;
	int idealNumOnGround = wheels.Num() - 2;
	if ( numOnGround > 0 && numOnGround < idealNumOnGround ) {
		// compensate for having some wheels off the ground
		float fullForce = inputMotorForce * idealNumOnGround;
		inputMotorForce = fullForce / numOnGround;
		if ( inputMotorForce > maxInputMotorForce ) {
			inputMotorForce = maxInputMotorForce;
		}
	}

	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		wheels[ i ]->UpdateMotor( input, inputMotorForce );
	}
}

/*
================
sdVehicleTrack::UpdatePostPhysics
================
*/
void sdVehicleTrack::UpdatePostPhysics( const sdVehicleInput& input ) {
	int i;
	for ( i = 1; i < wheels.Num() - 1; i++ ) {
		wheels[ i ]->UpdatePostPhysics( input );
	}

	bool grounded = false;
	float speed;

	for ( i = 0; i < wheels.Num(); i++ ) {
		if ( wheels[ i ]->IsGrounded() ) {
			grounded = true;
			break;
		}
	}
	
	if ( grounded ) {
		idVec3 pos;
		parent->GetWorldOrigin( joint, pos );

		idVec3 velocity;
		speed = parent->GetRBPhysics()->GetPointVelocity( pos, velocity ) * ( direction * parent->GetPhysics()->GetAxis() );
	} else {
		speed = GetInputSpeed( input );
	}
	if ( idMath::Fabs( speed ) < 0.5f ) {
		speed = 0.f;
	}

	parent->GetRenderEntity()->shaderParms[ shaderParmIndex ] += speed;
	
	for ( i = 0; i < wheels.Num(); i++ ) {
		wheels[ i ]->UpdateRotation( speed );
	}
}

/*
================
sdVehicleTrack::EvaluateContacts
================
*/
int sdVehicleTrack::EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max ) {
	int num = 0;
	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		num += wheels[ i ]->EvaluateContacts( list + num, listExt + num, max - num );
	}

	return num;
}

/*
================
sdVehicleTrack::CheckWater
================
*/
void sdVehicleTrack::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		wheels[ i ]->CheckWater( waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}

/*
================
sdVehicleTrack::GetSurfaceType
================
*/
const sdDeclSurfaceType* sdVehicleTrack::GetSurfaceType( void ) const {
	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		const sdDeclSurfaceType* surface = wheels[ i ]->GetSurfaceType();
		if ( surface != NULL ) {
			return surface;
		}
	}
	return NULL;
}

/*
================
sdVehicleTrack::PostInit
================
*/
void sdVehicleTrack::PostInit( void ) {
	sdVehicleDriveObject::PostInit();

	int numWheels = spawnPart->data.GetInt( "num_true_wheels" ) + 2;
	if ( numWheels > TRACK_MAX_WHEELS ) {
		gameLocal.Error( "sdVehicleTrack::PostInit - number of wheels exceeds TRACK_MAX_WHEELS" );
	}

	wheels.SetNum( numWheels );

	// get the start wheel
	sdVehicleDriveObject* object = parent->GetDriveObject( spawnPart->data.GetString( "start_wheel" ) );
	if ( object == NULL ) {
		gameLocal.Error( "sdVehicleTrack::PostInit - no start_wheel" );
	}
	wheels[ 0 ] = reinterpret_cast< sdVehicleRigidBodyWheel* >( object );
	if ( wheels[ 0 ] == NULL ) {
		gameLocal.Error( "sdVehicleTrack::PostInit - start_wheel is not a sdVehicleRigidBodyWheel" );
	}

	// get the end wheel
	object = parent->GetDriveObject( spawnPart->data.GetString( "end_wheel" ) );
	if ( object == NULL ) {
		gameLocal.Error( "sdVehicleTrack::PostInit - no end_wheel" );
	}
	wheels[ numWheels - 1 ] = reinterpret_cast< sdVehicleRigidBodyWheel* >( object );
	if ( wheels[ numWheels - 1 ] == NULL ) {
		gameLocal.Error( "sdVehicleTrack::PostInit - end_wheel is not a sdVehicleRigidBodyWheel" );
	}

	// create the other wheels
	for ( int i = 1; i < numWheels - 1; i++ ) {
		sdVehicleRigidBodyWheel* part = new sdVehicleRigidBodyWheel;
		part->SetIndex( -1 );
		part->TrackWheelInit( *spawnPart, i, parent );
		wheels[ i ] = part;
	}

	for ( int i = 1; i < numWheels - 1; i++ ) {
		wheels[ i ]->PostInit();
	}
}

/*
================
sdVehicleTrack::UpdateSuspensionIK
================
*/
bool sdVehicleTrack::UpdateSuspensionIK( void ) {
	bool changed = false;

	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		changed |= wheels[ i ]->UpdateSuspensionIK();
	}

	return changed;
}

/*
================
sdVehicleTrack::ClearSuspensionIK
================
*/
void sdVehicleTrack::ClearSuspensionIK( void ) {
	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		wheels[ i ]->ClearSuspensionIK();
	}
}

/*
================
sdVehicleTrack::IsGrounded
================
*/
bool sdVehicleTrack::IsGrounded( void ) const {
	int numGrounded = 0;
	for ( int i = 1; i < wheels.Num() - 1; i++ ) {
		if ( wheels[ i ]->IsGrounded() ) {
			numGrounded++;
		}
	}

	return numGrounded >= 1;
}

/*
===============================================================================

	sdVehicleThruster

===============================================================================
*/

const idEventDef EV_Thruster_SetThrust( "setThrust", '\0', DOC_TEXT( "Controls the thrust multiplier used by this thruster." ), 1, NULL, "f", "scale", "Scale factor to set." );

CLASS_DECLARATION( sdVehicleDriveObject, sdVehicleThruster )
EVENT( EV_Thruster_SetThrust,				sdVehicleThruster::Event_SetThrust )
END_CLASS

/*
================
sdVehicleThruster::sdVehicleThruster
================
*/
void sdVehicleThruster::Init( const sdDeclVehiclePart& thruster, sdTransport_RB* _parent ) {
	parent			= _parent;
	direction		= thruster.data.GetVector( "direction" );
	fixedDirection	= thruster.data.GetVector( "direction_fixed" );
	origin			= thruster.data.GetVector( "origin" );
	reverseScale	= thruster.data.GetFloat( "reverse_scale", "1" );
	float force		= thruster.data.GetFloat( "force" );
	direction		*= force;
	fixedDirection	*= force;
	name			= thruster.data.GetString( "name" );
	needWater		= thruster.data.GetBool( "need_water" );

	inWater			= false;
	thrustScale		= 0.f;
}

/*
================
sdVehicleThruster::GetParent
================
*/
sdTransport* sdVehicleThruster::GetParent( void ) const {
	return parent;
}

/*
================
sdVehicleThruster::CalcPos
================
*/
void sdVehicleThruster::CalcPos( idVec3& pos ) {
	pos = parent->GetPhysics()->GetOrigin() + ( origin  * parent->GetPhysics()->GetAxis() );
}

/*
================
sdVehicleThruster::SetThrust
================
*/
void sdVehicleThruster::SetThrust( float thrust ) {
	thrustScale = thrust;
}

/*
================
sdVehicleThruster::Event_SetThrust
================
*/
void sdVehicleThruster::Event_SetThrust( float thrust ) {
	SetThrust( thrust );
}

/*
================
sdVehicleThruster::UpdatePrePhysics
================
*/
void sdVehicleThruster::UpdatePrePhysics( const sdVehicleInput& input ) {
	bool reallyInWater = inWater || parent->GetRBPhysics()->InWater();
	if ( needWater ) {
		if ( !reallyInWater ) {
			return;
		}
	} else {
		if ( reallyInWater ) {
			return;
		}
	}

	if ( thrustScale != 0.f ) {
		float height = parent->GetPhysics()->GetOrigin()[ 2 ];
		float forceScale = 1.0f;
		if ( height > gameLocal.flightCeilingLower ) {
			if ( height > gameLocal.flightCeilingUpper ) {
				forceScale = 0.f;
			} else {
				forceScale = ( 1.f - ( height - gameLocal.flightCeilingLower ) / ( gameLocal.flightCeilingUpper - gameLocal.flightCeilingLower ) );
			}
		}

		if ( thrustScale < 0.f ) {
			thrustScale *= reverseScale;
		}

		idVec3 pos;
		CalcPos( pos );

		idVec3 dir = parent->GetPhysics()->GetAxis() * ( ( direction * thrustScale ) + ( fixedDirection * idMath::Fabs( thrustScale ) ) );
		
		parent->GetPhysics()->AddForce( 0, pos, dir * forceScale );
	}
}

/*
================
sdVehicleThruster::CheckWater
================
*/
void sdVehicleThruster::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( !needWater ) {
		return;
	}

	idVec3 pos;
	CalcPos( pos );

	pos -= waterBodyOrg;
	pos *= waterBodyAxis.Transpose();

	inWater = waterBodyModel->GetBounds().ContainsPoint( pos );
}

/*
===============================================================================

	sdVehicleAirBrake

===============================================================================
*/

CLASS_DECLARATION( sdVehicleDriveObject, sdVehicleAirBrake )
END_CLASS

/*
================
sdVehicleAirBrake::sdVehicleThruster
================
*/
void sdVehicleAirBrake::Init( const sdDeclVehiclePart& airBrake, sdTransport_RB* _parent ) {
	parent			= _parent;
	enabled			= false;

	factor			= airBrake.data.GetFloat( "factor" );
	maxSpeed		= airBrake.data.GetFloat( "max_speed" );

	name			= airBrake.data.GetString( "name" );
}

/*
================
sdVehicleAirBrake::UpdatePrePhysics
================
*/
void sdVehicleAirBrake::UpdatePrePhysics( const sdVehicleInput& input ) {
	if ( !enabled ) {
		return;
	}

	const idMat3& ownerAxis = parent->GetPhysics()->GetAxis();

	idVec3 forward = ownerAxis[ 0 ];
	forward.z = 0.f;

	float speed = parent->GetPhysics()->GetLinearVelocity() * forward;
	if ( speed > 0.f ) {
		idVec3 vel = speed * forward;
		idVec3 impulse = ( -vel * factor );
		impulse.Truncate( maxSpeed );

		impulse *= parent->GetPhysics()->GetMass();

		idVec3 com = parent->GetPhysics()->GetOrigin() + ( parent->GetPhysics()->GetCenterOfMass() * ownerAxis );
		parent->GetPhysics()->ApplyImpulse( 0, com, impulse );
	}
}

/*
===============================================================================

	sdVehicleRigidBodyRotor

===============================================================================
*/

CLASS_DECLARATION( sdVehicleRigidBodyPartSimple, sdVehicleRigidBodyRotor )
END_CLASS

/*
================
sdVehicleRigidBodyRotor::sdVehicleRigidBodyRotor
================
*/
sdVehicleRigidBodyRotor::sdVehicleRigidBodyRotor( void ) {
}

/*
================
sdVehicleRigidBodyRotor::~sdVehicleRigidBodyRotor
================
*/
sdVehicleRigidBodyRotor::~sdVehicleRigidBodyRotor( void ) {
}

/*
================
sdVehicleRigidBodyRotor::Init
================
*/
void sdVehicleRigidBodyRotor::Init( const sdDeclVehiclePart& rotor, sdTransport_RB* _parent ) {
	sdVehicleRigidBodyPartSimple::Init( rotor, _parent );

	liftCoefficient				= rotor.data.GetFloat( "lift" );

	const char* typeName = rotor.data.GetString( "rotortype", "main" );
	if ( !idStr::Icmp( typeName, "main" ) ) {
		type = RT_MAIN;
	} else if ( !idStr::Icmp( typeName, "tail" ) ) {
		type = RT_TAIL;
	} else {
		gameLocal.Error( "sdVehicleRigidBodyRotor::Init Invalid Rotor Type '%s'", typeName );
	}

	maxPitchDeflect				= rotor.data.GetFloat( "maxPitchDeflect" );
	maxYawDeflect				= rotor.data.GetFloat( "maxYawDeflect" );

	advanced.cyclicBankRate		= rotor.data.GetFloat( "cyclicBankRate" ) * 0.033f;
	advanced.cyclicPitchRate	= rotor.data.GetFloat( "cyclicPitchRate" ) * 0.033f;

	oldPitch					= 0.f;
	oldYaw						= 0.f;

	int count = rotor.data.GetInt( "num_blades" );
	for ( int i = 0; i < count; i++ ) {
		rotorJoint_t& j = animJoints.Alloc();

		const char* bladeJointName = rotor.data.GetString( va( "blade%i_joint", i + 1 ) );
		j.joint = _parent->GetAnimator()->GetJointHandle( bladeJointName );
		if ( j.joint == INVALID_JOINT ) {
			gameLocal.Warning( "sdVehicleRigidBodyRotor::Init Invalid Joint '%s'", bladeJointName );
			j.jointAxes.Identity();
		} else {
			_parent->GetAnimator()->GetJointTransform( j.joint, gameLocal.time, j.jointAxes );
		}
		j.speedScale	= rotor.data.GetFloat( va( "blade%i_speedScale", i + 1 ) );
		j.isYaw			= rotor.data.GetInt( va("blade%i_yaw", i+1 ) ) != 0;
		j.angle			= 0.f;
	}

	speed			= 0;

	sideScale					= rotor.data.GetFloat( "force_side_scale", "1" );

	zOffset						= rotor.data.GetFloat( "z_offset", "0" );
}

/*
================
sdVehicleRigidBodyRotor::UpdatePrePhysics_Main
================
*/
void sdVehicleRigidBodyRotor::UpdatePrePhysics_Main( const sdVehicleInput& input ) {
	sdPhysics_RigidBodyMultiple* physics	= parent->GetRBPhysics();
	const idMat3& bodyaxis					= physics->GetAxis();
	idVec3 bodyorg							= physics->GetOrigin() + ( physics->GetMainCenterOfMass() * bodyaxis ); // + physics->GetBodyOffset( bodyId );
	bool alive								= parent->GetHealth() > 0;
	idVec3 rotorAxis						= bodyaxis[ 2 ];
	bool deathThroes						= parent->InDeathThroes();
	bool careening							= parent->IsCareening() && !deathThroes;
	const idVec3& angVelocity				= physics->GetAngularVelocity();
	const idAngles bodyangles				= bodyaxis.ToAngles();

	bool drowned = physics->InWater() > 0.5f;
	bool simpleControls = input.GetPlayer() != NULL && !input.GetPlayer()->GetUserInfo().advancedFlightControls;

	idMat3 rotateAxes;
	idAngles::YawToMat3( -( bodyangles.yaw ), rotateAxes );

	rotorAxis *= rotateAxes;

	Swap( rotorAxis[ 0 ], rotorAxis[ 2 ] );
	idAngles angles = rotorAxis.ToAngles();
	angles.pitch = idMath::AngleNormalize180( angles.pitch );
	if ( angles.pitch < 0.f ) {
		angles.pitch += maxPitchDeflect;
		if ( angles.pitch > 0.f ) {
			angles.pitch = 0.f;
		}
	} else {
		angles.pitch -= maxPitchDeflect;
		if ( angles.pitch < 0.f ) {
			angles.pitch = 0.f;
		}
	}

	angles.yaw = idMath::AngleNormalize180( angles.yaw );
	if ( angles.yaw < 0.f ) {
		angles.yaw += maxYawDeflect;
		if ( angles.yaw > 0.f ) {
			angles.yaw = 0.f;
		}
	} else {
		angles.yaw -= maxYawDeflect;
		if ( angles.yaw < 0.f ) {
			angles.yaw = 0.f;
		}
	}

	rotorAxis = angles.ToForward();
	Swap( rotorAxis[ 0 ], rotorAxis[ 2 ] );

	rotorAxis *= rotateAxes.Transpose();
	rotorAxis[ 0 ] *= sideScale;
	rotorAxis[ 0 ] = idMath::ClampFloat( -1.f, 1.f, rotorAxis[ 0 ] );
	rotorAxis[ 1 ] *= sideScale;
	rotorAxis[ 1 ] = idMath::ClampFloat( -1.f, 1.f, rotorAxis[ 1 ] );

//	gameRenderWorld->DebugLine( colorYellow, worldOrg, worldOrg + ( rotorAxis * 32 ) );

	const sdVehicleControlBase* control = parent->GetVehicleControl();

	bool deadZone = ( angles.pitch == 0 ) && ( angles.yaw == 0 ) && ( !control || physics->HasGroundContacts() );
	float deadZoneFraction = 1.0f - ( control != NULL ? control->GetDeadZoneFraction() : 0.0f );
	deadZoneFraction = idMath::ClampFloat( 0.0f, 1.0f, deadZoneFraction );
	deadZoneFraction = idMath::Sqrt( deadZoneFraction );

	if ( alive ) {
		float inputRoll = 0.0f;
		float inputPitch = 0.0f;

		if ( !deadZone ) {
			if ( input.GetPlayer() && !input.GetUserCmd().buttons.btn.tophat ) {
				inputRoll = input.GetRoll();
				inputPitch = input.GetPitch();
			}
		}

		inputRoll = idMath::ClampFloat( -2.f, 2.f, inputRoll );
		inputPitch = idMath::ClampFloat( -2.f, 2.f, inputPitch );

		advanced.cyclicBank = ( inputRoll / 180.f ) * advanced.cyclicBankRate;
		advanced.cyclicPitch = ( inputPitch / 180.f ) * advanced.cyclicPitchRate;

		if ( deathThroes ) {
			advanced.cyclicPitch = 45.0f;
		}

		if ( control && careening ) {
			advanced.cyclicBank = control->GetCareeningRollAmount();
			advanced.cyclicPitch = control->GetCareeningPitchAmount();
		}

		bodyorg += advanced.cyclicPitch * bodyaxis[ 0 ];
		bodyorg += advanced.cyclicBank * bodyaxis[ 1 ];
	}

	float frac;

	if ( control ) {
		int start	= control->GetLandingChangeTime();
		int length	= control->GetLandingChangeEndTime() - start;

		frac = length ? idMath::ClampFloat( 0.f, 1.f, ( gameLocal.time - start ) / static_cast< float >( length ) ) : 1.f;
		frac = Square( frac );
	} else {
		frac = 1.f;
	}

	if ( control && careening ) {
		frac = control->GetCareeningLiftScale();
	}

	if ( !control || control->IsLanding() && !( careening && !deathThroes ) ) {
		speed = GetTopGoalSpeed() * ( 1.f - frac );
	} else {
		speed = GetTopGoalSpeed() * frac;
	}

	float lift = 1.f;

	const float liftSpeedFrac = 0.90f;

	float speedFrac = speed / GetTopGoalSpeed();
	if ( speedFrac > liftSpeedFrac ) {
	
		lift += input.GetCollective();
		if ( control && careening ) {
			lift += control->GetCareeningCollectiveAmount();
		}
		lift *= speed * liftCoefficient;

		float height = physics->GetOrigin()[ 2 ];
		if ( height > gameLocal.flightCeilingLower ) {
			if ( height > gameLocal.flightCeilingUpper ) {
				lift = 0.f;
			} else {
				lift *= ( 1.f - ( height - gameLocal.flightCeilingLower ) / ( gameLocal.flightCeilingUpper - gameLocal.flightCeilingLower ) );
			}
		}

		if ( drowned ) {
			lift = 0.0f;
		}

		idVec3 force = rotorAxis * lift;
		physics->AddForce( bodyId, bodyorg, force );

		parent->GetRenderEntity()->shaderParms[ SHADERPARM_ALPHA ] = ( speedFrac - liftSpeedFrac ) / ( 1.f - liftSpeedFrac );
	} else {
		parent->GetRenderEntity()->shaderParms[ SHADERPARM_ALPHA ] = 0.0f;
	}

	if ( drowned ) {
		// try to resist all movement
		const idVec3& linearVelocity = physics->GetLinearVelocity();
		idVec3 stoppingAcceleration = -0.05f * linearVelocity / MS2SEC( gameLocal.msec ); 
		physics->AddForce( stoppingAcceleration * physics->GetMass( -1 ) );

		const idVec3& angularVelocity = physics->GetAngularVelocity();
		idVec3 stoppingAlpha = -0.05f * angularVelocity / MS2SEC( gameLocal.msec ); 
		physics->AddTorque( stoppingAlpha * physics->GetInertiaTensor( -1 ) );
	}
}

const float goalSpeedTail = 1000.f;
/*
================
sdVehicleRigidBodyRotor::UpdatePrePhysics_Tail
================
*/
void sdVehicleRigidBodyRotor::UpdatePrePhysics_Tail( const sdVehicleInput& input ) {
	sdPhysics_RigidBodyMultiple* physics	= parent->GetRBPhysics();
	if ( physics->InWater() > 0.5f ) {
		return;
	}


	const idMat3& bodyaxis					= physics->GetAxis();
	idVec3 bodyorg;
	idVec3 rotorAxis						= bodyaxis[ 1 ];
	const idVec3 comWorld = physics->GetOrigin() + ( physics->GetCenterOfMass() * bodyaxis );

	GetWorldPhysicsOrigin( bodyorg );
	bodyorg += bodyaxis[ 2 ] * zOffset;

	bool simpleControls = input.GetPlayer() != NULL && !input.GetPlayer()->GetUserInfo().advancedFlightControls;
	if ( !simpleControls ) {
		// advanced controls fly the vehicle in local space
		float arm = ( bodyorg - comWorld ).Length();

		rotorAxis.Set( 0.0f, 1.0f, 0.0f );
		bodyorg.Set( -arm, 0.0f, 0.0f );
	}

	const sdVehicleControlBase* control = parent->GetVehicleControl();

	float frac;
	if ( control ) {
		int start	= control->GetLandingChangeTime();
		int length	= control->GetLandingChangeEndTime() - start;
		frac = length ? idMath::ClampFloat( 0.f, 1.f, ( gameLocal.time - start ) / static_cast< float >( length ) ) : 1.f;
		frac = Square( frac );
	} else {
		frac = 1.f;
	}

	if ( !control || control->IsLanding() ) {
		speed = goalSpeedTail * ( 1.f - frac );
	} else {
		speed = goalSpeedTail * frac;
	}

	bool deathThroes = parent->InDeathThroes();
	bool careening = parent->IsCareening() && !deathThroes;
	if ( deathThroes || careening ) {
		frac = 1.0f;
		speed = goalSpeedTail;
	}

	if ( input.GetPlayer() || ( ( deathThroes || careening ) && !physics->HasGroundContacts() ) ) {
		float lift = -0.5f * input.GetYaw();
		if ( deathThroes ) {
			lift -= 1.0f;
		} else if ( control && careening ) {
			lift = control->GetCareeningYawAmount();
		}

		if( lift ) {
			lift *= speed / goalSpeedTail;

			idVec3 force = rotorAxis * lift * liftCoefficient;

			if ( !simpleControls ) {
				// use a torque to turn
				// without adverse sideways movement
				float torqueMag = force.y * bodyorg.x;
				const idMat3& itt = physics->GetInertiaTensor();

				// find the moment in the z axis
				float zMoment = idVec3( 0.0f, 0.0f, 1.0f ) * (itt * idVec3( 0.0f, 0.0f, 1.0f ));

				// calculate the torque to do a clean rotation about the vehicle's z axis
				idVec3 alpha( 0.0f, 0.0f, torqueMag / zMoment );
				idVec3 cleanTorque = itt * alpha;
				physics->AddTorque( ( cleanTorque ) * bodyaxis );
			} else {
				// calculate the effect the force WOULD have if we applied it right now
				idVec3 torque = ( bodyorg - comWorld ).Cross( force );

				const idMat3 worldIT = bodyaxis.Transpose() * physics->GetInertiaTensor() * bodyaxis;
				const idMat3 worldInvIT = worldIT.Inverse();

				idVec3 alpha = worldInvIT * torque;

				// find the alpha with only yaw (wrt world)
				float desiredAlphaMagnitude = alpha.LengthFast();
				if ( alpha.z < 0.0f ) {
					desiredAlphaMagnitude = -desiredAlphaMagnitude;
				}
				alpha.Set( 0.0f, 0.0f, desiredAlphaMagnitude );

				// calculate the torque needed
				torque = worldIT * alpha;

				// calculate the effect this will have
				idVec3 torqueAlpha = worldInvIT * torque;

				physics->AddTorque( torque );
			}
		}
	}
}

/*
================
sdVehicleRigidBodyRotor::UpdatePrePhysics
================
*/
void sdVehicleRigidBodyRotor::UpdatePrePhysics( const sdVehicleInput& input ) {
	switch( type ) {
		case RT_MAIN:
			UpdatePrePhysics_Main( input );
			break;
		case RT_TAIL:
			UpdatePrePhysics_Tail( input );
			break;
	}
}

/*
================
sdVehicleRigidBodyRotor::UpdatePostPhysics_Main
================
*/
void sdVehicleRigidBodyRotor::UpdatePostPhysics_Main( const sdVehicleInput& input ) {
	if ( speed > 0.5f ) {
		int i;
		for ( i = 0; i < animJoints.Num(); i++ ) {
			rotorJoint_t& joint = animJoints[ i ];

			joint.angle += speed * MS2SEC( gameLocal.msec ) * joint.speedScale;

			idMat3 rotation;
			idAngles::YawToMat3( joint.angle, rotation );

			parent->GetAnimator()->SetJointAxis( joint.joint, JOINTMOD_WORLD_OVERRIDE, joint.jointAxes * rotation );
		}
	}
}

/*
================
sdVehicleRigidBodyRotor::UpdatePostPhysics_Tail
================
*/
void sdVehicleRigidBodyRotor::UpdatePostPhysics_Tail( const sdVehicleInput& input ) {
	if( speed > 0.5f ) {
		int i;
		for ( i = 0; i < animJoints.Num(); i++ ) {
			rotorJoint_t& joint = animJoints[ i ];

			joint.angle += speed * MS2SEC( gameLocal.msec ) * joint.speedScale;

			idMat3 rotation;
			if ( joint.isYaw ) {
				idAngles::YawToMat3( joint.angle, rotation );
			} else {
				idAngles::PitchToMat3( joint.angle, rotation );
			}

			parent->GetAnimator()->SetJointAxis( joint.joint, JOINTMOD_WORLD_OVERRIDE, joint.jointAxes * rotation );
		}
	}
}

/*
================
sdVehicleRigidBodyRotor::UpdatePostPhysics
================
*/
void sdVehicleRigidBodyRotor::UpdatePostPhysics( const sdVehicleInput& input ) {
	switch( type ) {
		case RT_MAIN:
			UpdatePostPhysics_Main( input );
			break;
		case RT_TAIL:
			UpdatePostPhysics_Tail( input );
			break;
	}
}

/*
================
sdVehicleRigidBodyRotor::GetTopGoalSpeed
================
*/
float sdVehicleRigidBodyRotor::GetTopGoalSpeed( void ) const {	
	sdPhysics_RigidBodyMultiple* physics	= parent->GetRBPhysics();

	return ( physics->GetMainMass() * -physics->GetGravity()[ 2 ] ) / liftCoefficient;
}

/*
================
sdVehicleRigidBodyRotor::ResetCollective
================
*/
void sdVehicleRigidBodyRotor::ResetCollective( void ) {
	advanced.collective = 0.f;
}

/*
===============================================================================

	sdVehicleRigidBodyHoverPad

===============================================================================
*/

CLASS_DECLARATION( sdVehicleRigidBodyPartSimple, sdVehicleRigidBodyHoverPad )
END_CLASS

/*
================
sdVehicleRigidBodyHoverPad::sdVehicleRigidBodyHoverPad
================
*/
sdVehicleRigidBodyHoverPad::sdVehicleRigidBodyHoverPad( void ) {
}

/*
================
sdVehicleRigidBodyHoverPad::~sdVehicleRigidBodyHoverPad
================
*/
sdVehicleRigidBodyHoverPad::~sdVehicleRigidBodyHoverPad( void ) {
}

/*
================
sdVehicleRigidBodyHoverPad::Init
================
*/
void sdVehicleRigidBodyHoverPad::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehicleRigidBodyPartSimple::Init( part, _parent );

	traceDir				= part.data.GetVector( "direction", "0 0 -1" );
	traceDir.Normalize();

	minAngles				= part.data.GetAngles( "min_angles", "-10 -10 -10" );
	maxAngles				= part.data.GetAngles( "max_angles", "10 10 10" );

	maxTraceLength			= part.data.GetFloat( "distance", "64" );
	
	shaderParmIndex			= part.data.GetInt( "shaderParmIndex", "0" );
	adaptSpeed				= part.data.GetFloat( "adaption_speed", "0.005" );

	_parent->GetAnimator()->GetJointTransform( joint, gameLocal.time, baseOrg, baseAxes );
	idMat3 ident; ident.Identity();
	currentAxes = ident.ToQuat();

	// Setup the lighting effect
	renderEffect_t &effect = engineEffect.GetRenderEffect();
	effect.declEffect	= gameLocal.FindEffect( _parent->spawnArgs.GetString( "fx_hoverpad" ) );
	effect.axis			= mat3_identity;
	effect.attenuation	= 1.f;
	effect.hasEndOrigin	= true;
	effect.loop			= false;
	effect.shaderParms[SHADERPARM_RED]			= 1.0f;
	effect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	effect.shaderParms[SHADERPARM_BLUE]			= 1.0f;
	effect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	effect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

	nextBeamTime = gameLocal.time + 2000 + gameLocal.random.RandomInt( 1000 );
	beamTargetInfo = gameLocal.declTargetInfoType[ _parent->spawnArgs.GetString( "ti_lightning" ) ];

	lastVelocity.Zero();
}

idCVar g_debugVehicleHoverPads( "g_debugVehicleHoverPads", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "show info about hoverpads" );

void sdVehicleRigidBodyHoverPad::UpdatePostPhysics( const sdVehicleInput& input ) {
	
	if ( !input.GetPlayer() || parent->GetPhysics()->GetAxis()[ 2 ].z < 0.f ) {
		return;
	}

	if ( !gameLocal.DoClientSideStuff() ) {
		return;
	}

	idMat3 axis;
	idVec3 origin;
	GetWorldAxis( axis );
	GetWorldOrigin( origin );

	idMat3 ownerAxis = parent->GetPhysics()->GetAxis();
	idVec3 velocity = parent->GetPhysics()->GetLinearVelocity();
	idVec3 acceleration = ( velocity - lastVelocity ) / MS2SEC( gameLocal.msec );
	lastVelocity = velocity;

	// tilt things based on velocity & acceleration
	idVec3 forward = ownerAxis[0];
	idVec3 right = ownerAxis[1];
	idVec3 up = ownerAxis[2];

	float forwardAccel = acceleration * forward;
	float rightAccel = acceleration * right;

	float forwardVel = velocity * forward;
	float rightVel = velocity * right;

	// want to rotate the joint on its axis to look like its tilting with the speed
	idAngles target( 0.0f, 0.0f, 0.0f );
	target.pitch = 0.05f * ( forwardAccel * 0.1f + forwardVel * 0.1f );
	target.roll = -0.05f * ( rightAccel * 0.1f + rightVel * 0.1f );

	// create a rotation about the offset axis
	float rotSpeed = parent->GetPhysics()->GetAngularVelocity() * up;
	idVec3 rotAxis = baseOrg;
	rotAxis.Normalize();

	idRotation rotationRot( vec3_zero, rotAxis, rotSpeed * 15.0f );
	idMat3 rotationMat = rotationRot.ToMat3();
	idMat3 translationMat = target.ToMat3();
	idMat3 totalMat = rotationMat * translationMat;
	target = totalMat.ToAngles();
	target.Clamp( minAngles, maxAngles );

	// Lerp with the current rotations 
	// this is a semi hack, you have to do an slerp to be totally correct, but we just do a linear lerp + renormalize
	// this causes or rotation speeds to be non uniform but who cares... the rotations are still correct
	float lerp = ( gameLocal.msec * adaptSpeed );
	currentAxes = target.ToQuat() * lerp + currentAxes * ( 1.0f - lerp );
	currentAxes.Normalize();
	currentAxes.FixDenormals( 0.00001f );

	// Add the local joint transform on top of this
	idMat3 mat = baseAxes * currentAxes.ToMat3();
	parent->GetAnimator()->SetJointAxis( joint, JOINTMOD_WORLD_OVERRIDE, mat );

	// Enable disable engine shader effect on model
	if ( parent->GetEngine().GetState() ) {
		parent->GetRenderEntity()->shaderParms[shaderParmIndex] = 1.0f;
	} else {
		parent->GetRenderEntity()->shaderParms[shaderParmIndex] = 0.0f;
	}




	// Lightning bolts from engines
	//	(visuals only no physics here)
	//
	if ( gameLocal.time > nextBeamTime ) {
		idMat3 parentAxis = parent->GetRBPhysics()->GetAxis();

		idVec3 tempTraceDir = traceDir * axis;
		idVec3 end = origin + ( tempTraceDir * maxTraceLength );

		idEntity *entList[ 100 ];
		bool foundAttractor = false;
		int numFound = gameLocal.EntitiesWithinRadius( origin, 256.0f, entList, 100 );
		engineEffect.Stop(); // stop old bolt

		for ( int i = 0; i < numFound; i++ ) {

			if ( entList[ i ] != parent && beamTargetInfo->FilterEntity( entList[ i ] ) ) {
				idPhysics* entPhysics = entList[ i ]->GetPhysics();

				idVec3 end = entPhysics->GetOrigin() + entPhysics->GetBounds().GetCenter();
				engineEffect.GetRenderEffect().endOrigin = end;
				// If it is to far or going up don't bother
				idVec3 diff = end - origin;
				if ( ( diff.z < 0.f ) && ( diff.LengthSqr() < Square( 1000.f ) ) ) {
					engineEffect.Start( gameLocal.time );
					nextBeamTime = gameLocal.time + 100 + gameLocal.random.RandomInt( 250 ); //Make the lightning a bit more "explosive"
					foundAttractor = true;
					break;
				}
			}

		}
		
		if ( !foundAttractor ) {
			// Random vector in negative z cone
			idVec3 dir = gameLocal.random.RandomVectorInCone( 80.f );
			dir.z = -dir.z;
			end = origin + dir * 256.f;

			trace_t beamTrace;
			memset( &beamTrace, 0, sizeof( beamTrace ) );
			gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS beamTrace, origin, end, MASK_VEHICLESOLID, parent );

			// If it hit anything then spawn a beam towards that 
			if ( beamTrace.fraction < 1.f ) {
				engineEffect.GetRenderEffect().endOrigin = beamTrace.endpos;
				engineEffect.Start( gameLocal.time );
			}

			nextBeamTime = gameLocal.time + 1000 + gameLocal.random.RandomInt( 2500 );
		}
	}

	engineEffect.GetRenderEffect().origin = origin;
	engineEffect.GetRenderEffect().suppressSurfaceInViewID = parent->GetRenderEntity()->suppressSurfaceInViewID;
	engineEffect.GetRenderEffect().suppressLightsInViewID = parent->GetRenderEntity()->suppressSurfaceInViewID;
	engineEffect.Update();
}

/*
================
sdVehicleRigidBodyHoverPad::EvaluateContacts
================
*/
int sdVehicleRigidBodyHoverPad::EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max ) {
	return 0;
}




/*
===============================================================================

	sdVehicleSuspensionPoint

===============================================================================
*/

CLASS_DECLARATION( sdVehicleDriveObject, sdVehicleSuspensionPoint )
END_CLASS

/*
================
sdVehicleSuspensionPoint::sdVehicleSuspensionPoint
================
*/
sdVehicleSuspensionPoint::sdVehicleSuspensionPoint( void ) {
	suspension = NULL;
	offset = 0;

	suspensionInterface.Init( this );
}

/*
================
sdVehicleSuspensionPoint::~sdVehicleSuspensionPoint
================
*/
sdVehicleSuspensionPoint::~sdVehicleSuspensionPoint( void ) {
	delete suspension;
}


/*
================
sdVehicleSuspensionPoint::Init
================
*/
void sdVehicleSuspensionPoint::Init( const sdDeclVehiclePart& point, sdTransport_RB* _parent ) {
	parent = _parent;

	state.grounded	= false;
	state.rested	= true;

	suspensionInfo.velocityScale= point.data.GetFloat( "suspensionVelocityScale", "1" );
	suspensionInfo.kCompress	= point.data.GetFloat( "suspensionKCompress" );
	suspensionInfo.damping		= point.data.GetFloat( "suspensionDamping" );

	radius						= point.data.GetFloat( "radius" );

	joint = _parent->GetAnimator()->GetJointHandle( point.data.GetString( "joint" ) );
	startJoint = _parent->GetAnimator()->GetJointHandle( point.data.GetString( "startJoint" ) );

	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspensionPoint::Init Invalid Joint Name '%s'", point.data.GetString( "joint" ) );
	}
	if ( startJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehicleSuspensionPoint::Init Invalid Joint Name '%s'", point.data.GetString( "startJoint" ) );
	}

	_parent->GetAnimator()->GetJointTransform( joint, gameLocal.time, baseOrg );
	_parent->GetAnimator()->GetJointTransform( startJoint, gameLocal.time, baseStartOrg );
	suspensionInfo.totalDist = ( baseOrg - baseStartOrg ).Length();

	const sdDeclStringMap* suspenionInfo = gameLocal.declStringMapType[ point.data.GetString( "suspension" ) ];
	if ( suspenionInfo ) {
		gameLocal.CacheDictionaryMedia( suspenionInfo->GetDict() );
		suspension = sdVehicleSuspension::GetSuspension( suspenionInfo->GetDict().GetString( "type" ) );
		if ( suspension ) {
			suspension->Init( &suspensionInterface, suspenionInfo->GetDict() );
		}
	}

	frictionAxes.Identity();
	contactFriction				= point.data.GetVector( "contactFriction", "0 0 0" );
	aggressiveDampening			= point.data.GetBool( "aggressiveDampening" );

}

/*
================
sdVehicleSuspensionPoint::GetParent
================
*/
sdTransport* sdVehicleSuspensionPoint::GetParent( void ) const {
	return parent;
}

/*
================
sdVehicleSuspensionPoint::UpdatePrePhysics
================
*/
void sdVehicleSuspensionPoint::UpdatePrePhysics( const sdVehicleInput& input ) {
	idVec3 org;
	idVec3 startOrg;
	idMat3 axis = mat3_identity;

	if ( !parent->GetPhysics()->IsAtRest() ) {
		const renderEntity_t& renderEntity = *parent->GetRenderEntity();
		org = renderEntity.origin + ( baseOrg * renderEntity.axis );
		startOrg = renderEntity.origin + ( baseStartOrg * renderEntity.axis );

		idVec3 start = startOrg;
		idVec3 end = org;

		memset( &groundTrace, 0, sizeof( groundTrace ) );
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS groundTrace, start, end, MASK_VEHICLESOLID | CONTENTS_MONSTER, parent );

		groundTrace.c.selfId = -1;

		offset = radius - ( 1.0f - groundTrace.fraction ) * suspensionInfo.totalDist;

		state.rested = true;

		if ( groundTrace.fraction != 1.0f ) {
			groundTrace.c.selfId = -1;
			groundTrace.c.normal = start - end;
			groundTrace.c.normal.Normalize();

			CalcForces( suspensionForce, suspensionVelocity, -groundTrace.c.normal );
			if ( suspensionVelocity < -5.f ) {
				state.rested = false;
				parent->GetPhysics()->Activate();
			} else {
				suspensionVelocity = 0.f;
			}
		}

		state.grounded = groundTrace.fraction != 1.f;

		if ( suspension ) {
			suspension->Update();
		}

		frictionAxes = axis;
	} else {
		state.rested = true;
	}
}

/*
================
sdVehicleSuspensionPoint::EvaluateContacts
================
*/
int sdVehicleSuspensionPoint::EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max ) {
	if ( IsHidden() || max < 1 ) {
		return 0;
	}

	if ( state.grounded ) {

		listExt[ 0 ].contactForceMax		= suspensionForce;
		listExt[ 0 ].contactForceVelocity	= suspensionVelocity; 
		listExt[ 0 ].contactFriction		= contactFriction;
		listExt[ 0 ].frictionAxes			= frictionAxes;

		listExt[ 0 ].motorForce				= 0.0f;
		listExt[ 0 ].motorDirection.Zero();
		listExt[ 0 ].motorSpeed				= 0.0f;

		listExt[ 0 ].rested					= state.rested;

		list[ 0 ] = groundTrace.c;

		return 1;
	}
	return 0;
}

/*
================
sdVehicleSuspensionPoint::CalcForces
================
*/
void sdVehicleSuspensionPoint::CalcForces( float& maxForce, float& velocity, const idVec3& traceDir ) {

	idVec3 v;
	parent->GetRBPhysics()->GetPointVelocity( groundTrace.endpos, v );

	float dampingForce = suspensionInfo.damping * idMath::Fabs( v * -traceDir );
	float compression = suspensionInfo.totalDist - suspensionInfo.totalDist * groundTrace.fraction;

	state.grounded = true;

	maxForce = compression * suspensionInfo.kCompress;
	if ( groundTrace.fraction < 0.5f ) { // TODO: Make this configurable
		maxForce *= 2.f;
	}

	if ( !aggressiveDampening ) {
		velocity = dampingForce - ( compression * suspensionInfo.velocityScale );
	} else {
		velocity = 0.0f;
	}
}

/*
================
sdVehicleSuspensionPoint::UpdateSuspensionIK
================
*/
bool sdVehicleSuspensionPoint::UpdateSuspensionIK( void ) {
	if ( suspension != NULL ) {
		return suspension->UpdateIKJoints( parent->GetAnimator() );
	}
	return false;
}

/*
================
sdVehicleSuspensionPoint::ClearSuspensionIK
================
*/
void sdVehicleSuspensionPoint::ClearSuspensionIK( void ) {
	if ( suspension != NULL ) {
		suspension->ClearIKJoints( parent->GetAnimator() );
	}
}

/*
===============================================================================

sdVehicleRigidBodyVtol

===============================================================================
*/

CLASS_DECLARATION( sdVehicleRigidBodyPartSimple, sdVehicleRigidBodyVtol )
END_CLASS

/*
================
sdVehicleRigidBodyVtol::sdVehicleRigidBodyVtol
================
*/
sdVehicleRigidBodyVtol::sdVehicleRigidBodyVtol( void ) {
}

/*
================
sdVehicleRigidBodyVtol::~sdVehicleRigidBodyVtol
================
*/
sdVehicleRigidBodyVtol::~sdVehicleRigidBodyVtol( void ) {
}

/*
================
sdVehicleRigidBodyVtol::Init
================
*/
void sdVehicleRigidBodyVtol::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehicleRigidBodyPartSimple::Init( part, _parent );

	shoulderAnglesBounds	= part.data.GetVec2( "shoulderBounds", "-10 10" );
	elbowAnglesBounds	= part.data.GetVec2( "elbowBounds", "-10 10" );
	shoulderAxis		= part.data.GetInt( "shoulderAxis", "1" );	// Which major axis to rotate along
	elbowAxis			= part.data.GetInt( "elbowAxis", "0" );
	shoulderAngleScale		= part.data.GetInt( "shoulderAngleScale", "1" );
	elbowAngleScale			= part.data.GetInt( "elbowAngleScale", "1" );

	elbowJoint = _parent->GetAnimator()->GetJointHandle( part.data.GetString( "elbowJoint" ) );
	if ( elbowJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehiclePart_AF::Init '%s' can't find VTOL joint '%s'", name.c_str(), part.data.GetString( "elbowJoint" ) );
	}

	effectJoint = _parent->GetAnimator()->GetJointHandle( part.data.GetString( "effectJoint" ) );
	if ( effectJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehiclePart_AF::Init '%s' can't find VTOL joint '%s'", name.c_str(), part.data.GetString( "elbowJoint" ) );
	}

	idVec3 shoulderBaseOrg; idVec3 elbowBaseOrg;
	// Extract initial joint positions
	_parent->GetAnimator()->GetJointTransform( joint, gameLocal.time, shoulderBaseOrg, shoulderBaseAxes );
	_parent->GetAnimator()->GetJointTransform( elbowJoint, gameLocal.time, elbowBaseOrg, elbowBaseAxes );

	// Setup the engine effect
	renderEffect_t &effect = engineEffect.GetRenderEffect();
	effect.declEffect	= gameLocal.FindEffect( _parent->spawnArgs.GetString( part.data.GetString( "effect", "fx_vtol" ) ) );
	effect.axis			= mat3_identity;
	effect.attenuation	= 1.f;
	effect.hasEndOrigin	= false;
	effect.loop			= true;
	effect.shaderParms[SHADERPARM_RED]			= 1.0f;
	effect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	effect.shaderParms[SHADERPARM_BLUE]			= 1.0f;
	effect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	effect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;
	_parent->GetAnimator()->GetJointTransform( effectJoint, gameLocal.time, engineEffect.GetRenderEffect().origin, engineEffect.GetRenderEffect().axis );
	//engineEffect.Start( gameLocal.time );

	oldShoulderAngle	= 0.f;
	oldElbowAngle		= 0.f;

/*
	shaderParmIndex			= part.data.GetInt( "shaderParmIndex", "0" );
	adaptSpeed				= part.data.GetFloat( "adaption_speed", "0.005" );
	velocityInfluence		= part.data.GetFloat( "linear_velocity_influence", "0.3" );
	angVelocityInfluence	= part.data.GetFloat( "angular_velocity_influence", "-0.3" );
	noiseFreq				= part.data.GetFloat( "random_motion_freq", "0.001" );
	noisePhase				= part.data.GetFloat( "random_motion_amplitude", "3" );

	restFraction			= part.data.GetFloat( "restFraction", "0.9" );

	minAngles.Set( minAnglesValues.x, minAnglesValues.y, minAnglesValues.z );
	maxAngles.Set( maxAnglesValues.x, maxAnglesValues.y, maxAnglesValues.z );

	traceDir.Normalize();

	groundNormal = idVec3( 0.0f, 0.0f, 1.0f );
	_parent->GetAnimator()->GetJointTransform( joint, gameLocal.time, baseOrg, baseAxes );
	idMat3 ident; ident.Identity();
	currentAxes = ident.ToQuat();
	noisePhase = gameLocal.random.CRandomFloat() * 4000.0f;

	//Dir is the direction the pads should "point to" when the vehicle is rotating along it's z-axis 
	//(so it can turn on the spot (velocity == 0)) and still look cool
	//This gives directions on the rotation circle (like the arms of a swastika)
	angularDir.Cross( baseOrg, idVec3( 0, 0, 1 ) );
	angularDir.Normalize();

	// Setup the lighting effect
	renderEffect_t &effect = engineEffect.GetRenderEffect();
	effect.declEffect	= gameLocal.declEffectsType.Find( _parent->spawnArgs.GetString( "fx_hoverpad" ) );
	effect.axis			= mat3_identity;
	effect.attenuation	= 1.f;
	effect.hasEndOrigin	= true;
	effect.loop			= false;
	effect.shaderParms[SHADERPARM_RED]			= 1.0f;
	effect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	effect.shaderParms[SHADERPARM_BLUE]			= 1.0f;
	effect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	effect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

	nextBeamTime = gameLocal.time + 2000 + gameLocal.random.RandomInt( 1000 );
	beamTargetInfo = (const sdDeclTargetInfo *)gameLocal.declTargetInfoType.Find( _parent->spawnArgs.GetString( "ti_lightning" ) );*/

}

/*
================
sdVehicleRigidBodyHoverPad::UpdatePrePhysics
================
*/
void sdVehicleRigidBodyVtol::UpdatePrePhysics( const sdVehicleInput& input ) {
	// We just want to do some animation
}

void sdVehicleRigidBodyVtol::UpdatePostPhysics( const sdVehicleInput& input ) {

	const idMat3 &modelAxis = parent->GetRenderEntity()->axis;
	const idVec3 &modelOrg = parent->GetRenderEntity()->origin;
	float shoulderAngle = RAD2DEG( idMath::ACos( modelAxis[0] * idVec3(0,0,1) ) ) - 90.0f;
	float elbowAngle = RAD2DEG( idMath::ACos( modelAxis[1] * idVec3(0,0,1) ) ) - 90.0;

	// "Shoulder"
	idVec3 axis = vec3_origin;
	axis[ shoulderAxis ] = 1.0f;
	shoulderAngle *= shoulderAngleScale;
	if( shoulderAngle < shoulderAnglesBounds.x ) {
		shoulderAngle = shoulderAnglesBounds.x;
	} else if ( shoulderAngle > shoulderAnglesBounds.y ) {
		shoulderAngle = shoulderAnglesBounds.y;
	}

	if ( idMath::Fabs( oldShoulderAngle - shoulderAngle ) > 0.1f ) {
		oldShoulderAngle = shoulderAngle;

		idRotation rot( vec3_origin, axis, shoulderAngle );
		idMat3 mat = shoulderBaseAxes * rot.ToMat3();
		parent->GetAnimator()->SetJointAxis( joint, JOINTMOD_WORLD_OVERRIDE, mat );
	}

	// "Elbow"
	axis = vec3_origin;
	axis[ elbowAxis ] = 1.0f;

	elbowAngle *= elbowAngleScale;
	if( elbowAngle < elbowAnglesBounds.x ) {
		elbowAngle = elbowAnglesBounds.x;
	} else if ( elbowAngle > elbowAnglesBounds.y ) {
		elbowAngle = elbowAnglesBounds.y;
	}

	if ( idMath::Fabs( oldElbowAngle - elbowAngle ) > 0.1f ) {
		elbowAngle = elbowAngle;

		idRotation rot = idRotation( vec3_origin, axis, elbowAngle );
		parent->GetAnimator()->SetJointAxis( elbowJoint, JOINTMOD_LOCAL, rot.ToMat3() );
	}

	if ( parent->GetEngine().GetState() ) {
		idMat3 effectAxis;
		idVec3 effectOrg;
		parent->GetAnimator()->GetJointTransform( effectJoint, gameLocal.time, effectOrg, effectAxis );

		engineEffect.GetRenderEffect().axis = effectAxis * modelAxis;
		engineEffect.GetRenderEffect().origin = effectOrg * modelAxis + modelOrg;

		engineEffect.Start( gameLocal.time );
		engineEffect.Update();
	} else {
		engineEffect.Stop();	
	}
}

/*
===============================================================================

sdVehicleRigidBodyAntiGrav

===============================================================================
*/

CLASS_DECLARATION( sdVehiclePartSimple, sdVehicleRigidBodyAntiGrav )
END_CLASS

/*
================
sdVehicleRigidBodyAntiGrav::sdVehicleRigidBodyAntiGrav
================
*/
sdVehicleRigidBodyAntiGrav::sdVehicleRigidBodyAntiGrav( void ) {
	clientParent = NULL;
	oldVelocity.Zero();
}

/*
================
sdVehicleRigidBodyAntiGrav::~sdVehicleRigidBodyAntiGrav
================
*/
sdVehicleRigidBodyAntiGrav::~sdVehicleRigidBodyAntiGrav( void ) {
}

/*
================
sdVehicleRigidBodyAntiGrav::Init
================
*/
void sdVehicleRigidBodyAntiGrav::Init( const sdDeclVehiclePart& part, sdTransport* _parent ) {
	sdVehiclePartSimple::Init( part, _parent );

	rotAxis	= part.data.GetInt( "rotAxis", "1" );
	tailUpAxis	= part.data.GetInt( "tailUpAxis", "0" );
	tailSideAxis	= part.data.GetInt( "tailSideAxis", "2" );

	{
		renderEffect_t &effect = engineMainEffect.GetRenderEffect();
		effect.declEffect	= gameLocal.FindEffect( _parent->spawnArgs.GetString( part.data.GetString( "effect", "fx_engine" ) ) );
		effect.axis			= mat3_identity;
		effect.attenuation	= 1.f;
		effect.hasEndOrigin	= false;
		effect.loop			= true;
		effect.shaderParms[SHADERPARM_RED]			= 1.0f;
		effect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
		effect.shaderParms[SHADERPARM_BLUE]			= 1.0f;
		effect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
		effect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;
	}
	{
		renderEffect_t &effect = engineBoostEffect.GetRenderEffect();
		effect.declEffect	= gameLocal.FindEffect( _parent->spawnArgs.GetString( part.data.GetString( "effect_boost", "fx_engine_boost" ) ) );
		effect.axis			= mat3_identity;
		effect.attenuation	= 1.f;
		effect.hasEndOrigin	= false;
		effect.loop			= true;
		effect.shaderParms[SHADERPARM_RED]			= 1.0f;
		effect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
		effect.shaderParms[SHADERPARM_BLUE]			= 1.0f;
		effect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
		effect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;
	}
	oldAngle = 0.0f;
	fanRotation = 0.f;
	targetAngle = 0.f;

	fanJointName = part.data.GetString( "fanJoint" );
	tailJointName = part.data.GetString( "tailJoint" );

	SetupJoints( _parent->GetAnimator() );

	lastGroundEffectsTime = 0;

	fanSpeedMultiplier		= parent->spawnArgs.GetFloat( "fan_speed_multiplier", "0.9" );
	fanSpeedOffset			= parent->spawnArgs.GetFloat( "fan_speed_offset", "500" );
	fanSpeedMax				= parent->spawnArgs.GetFloat( "fan_pitch_max", "800" );
	fanSpeedRampRate		= parent->spawnArgs.GetFloat( "fan_ramp_rate", "0.1" );
	lastFanSpeed			= fanSpeedOffset;
}

/*
================
sdVehicleRigidBodyAntiGrav::SetupJoints
================
*/
void sdVehicleRigidBodyAntiGrav::SetupJoints( idAnimator* targetAnimator ) {
	fanJoint = targetAnimator->GetJointHandle( fanJointName );
	if ( fanJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehiclePart_AF::Init '%s' can't find AntiGrav joint '%s'", name.c_str(), fanJointName.c_str() );
	}

	tailJoint = targetAnimator->GetJointHandle( tailJointName );
	if ( tailJoint == INVALID_JOINT ) {
		gameLocal.Error( "sdVehiclePart_AF::Init '%s' can't find AntiGrav joint '%s'", name.c_str(), tailJointName.c_str() );
	}

	idVec3 baseOrg;
	// Extract initial joint positions
	targetAnimator->GetJointTransform( joint, gameLocal.time, baseOrg, baseAxes );

	// Setup the engine effect
	targetAnimator->GetJointTransform( joint, gameLocal.time, engineMainEffect.GetRenderEffect().origin, engineMainEffect.GetRenderEffect().axis );
	targetAnimator->GetJointTransform( joint, gameLocal.time, engineBoostEffect.GetRenderEffect().origin, engineBoostEffect.GetRenderEffect().axis );
}


/*
================
sdVehicleRigidBodyAntiGrav::SetClientParent
================
*/
void sdVehicleRigidBodyAntiGrav::SetClientParent( rvClientEntity* p ) {
	clientParent = p;

	if ( p ) {
		SetupJoints( p->GetAnimator() );
	}
}

/*
================
sdVehicleRigidBodyAntiGrav::UpdatePrePhysics
================
*/
void sdVehicleRigidBodyAntiGrav::UpdatePrePhysics( const sdVehicleInput& input ) {
	// We just want to do some animation
}

/*
================
sdVehicleRigidBodyAntiGrav::UpdatePostPhysics
================
*/
void sdVehicleRigidBodyAntiGrav::UpdatePostPhysics( const sdVehicleInput& input ) {
	idPlayer* player = parent->GetPositionManager().FindDriver();
	sdPhysics_JetPack* jetpackPhysics = parent->GetPhysics()->Cast< sdPhysics_JetPack >();

	idAnimator* targetAnimator;
	idVec3 modelOrg;
	idMat3 modelAxis;

	if ( clientParent ) {
		modelOrg		= clientParent->GetOrigin();
		modelAxis		= clientParent->GetAxis();
		targetAnimator	= clientParent->GetAnimator();
	} else {
		modelOrg		= parent->GetPhysics()->GetOrigin();
		modelAxis		= parent->GetRenderEntity()->axis;
		targetAnimator	= parent->GetAnimator();
	}

	idVec3 velocity, localVelocity;// = parent->GetPhysics()->GetLinearVelocity() * modelAxis.Transpose();
	if ( jetpackPhysics ) {
		localVelocity = jetpackPhysics->GetFanForce() * modelAxis.Transpose();
	} else {
		localVelocity.Zero();
	}
	// zero out lateral velocities, as not represented in fan movement
	velocity = localVelocity;
	velocity.y = 0.f;

	// remove jittery small velocities
	if ( fabsf( velocity.x ) < 0.0001f ) {
		velocity.x = 0.f;
	}
	if ( fabsf( velocity.z ) < 0.0001f ) {
		velocity.z = 0.f;
	}

	// If we are not really going up just blend towards the forward pose
	//if ( velocity.z < 0.001f ) {
	//	velocity = idVec3( velocity.x + 100.0f, velocity.y, 0 );
	//}

	oldVelocity.FixDenormals();

	idVec3 targetVelocity = velocity * 0.9f + oldVelocity * 0.1f;

	targetVelocity.FixDenormals();

	float blendSpeed = 0.4f;
	if ( targetVelocity.Length() < 0.01f && (!jetpackPhysics || jetpackPhysics->OnGround()) ) {
		targetAngle = 0;//15;
		blendSpeed = 0.05f;
	}


	float angle =  targetAngle * blendSpeed + oldAngle * (1.f - blendSpeed);
	if ( idMath::Fabs( angle ) < idMath::FLT_EPSILON ) {
		angle = 0.0f;
	}
	idRotation rot( vec3_origin, baseAxes[rotAxis], angle );
	targetAnimator->SetJointAxis( joint, JOINTMOD_WORLD_OVERRIDE, rot.ToMat3() );
	oldAngle = angle;


	if ( !targetVelocity.Compare( oldVelocity, 0.00001f ) ) 
	{
		// Engine hubs
		idVec3 dir = targetVelocity ;
   		dir.y = 0.0f; // Ignore strafing :)
		dir.Normalize();
		targetAngle = RAD2DEG( idMath::ACos( dir * idVec3( 1, 0, 0 ) ) );

		// Tail stabilizers
		dir = targetVelocity;
		dir.Normalize();

		idVec3 angles;
		angles.Zero();
		angles[tailUpAxis] = dir.z * 2.0f;
		angles[tailSideAxis] = dir.y * 5.0f;
		targetAnimator->SetJointAxis( tailJoint, JOINTMOD_LOCAL, idAngles( angles ).ToMat3() );

		oldVelocity = targetVelocity;
	}

	if ( player ) {

		const idVec3& upAxis = parent->GetPhysics()->GetAxis()[ 2 ];
		float absSpeedKPH = ( localVelocity - ( localVelocity*upAxis )*upAxis ).Length();

		float idealNewSpeed = ( absSpeedKPH * fanSpeedMultiplier ) + fanSpeedOffset;
		if ( idealNewSpeed > fanSpeedMax ) {
			idealNewSpeed = fanSpeedMax;
		}

		float speed = Lerp( lastFanSpeed, idealNewSpeed, fanSpeedRampRate );
		lastFanSpeed = speed;

		if ( jetpackPhysics ) {
			speed += jetpackPhysics->GetBoost() * 1000.f;
		}

		// HACK - right fan spin in opposite direction
		if ( fanJointName[ 0 ] == 'r' ) {
			speed = -speed;
		}

		fanRotation += MS2SEC( gameLocal.msec ) * speed;
		idAngles rot( 0.0f, 0.0f, fanRotation );
		targetAnimator->SetJointAxis( fanJoint, JOINTMOD_LOCAL_OVERRIDE, rot.ToMat3() );
	}

	if ( !clientParent ) {
		UpdateEffect();
	}
}

/*
================
sdVehicleRigidBodyAntiGrav::UpdateEffect
================
*/
void sdVehicleRigidBodyAntiGrav::UpdateEffect() {
	idAnimator* targetAnimator;
	idVec3 modelOrg;
	idMat3 modelAxis;

	if ( clientParent ) {
		modelOrg		= clientParent->GetOrigin();
		modelAxis		= clientParent->GetAxis();
		targetAnimator	= clientParent->GetAnimator();
	} else {
		modelOrg		= parent->GetPhysics()->GetOrigin();
		modelAxis		= parent->GetRenderEntity()->axis;
		targetAnimator	= parent->GetAnimator();
	}

	if ( parent->GetPositionManager().FindDriver() ) {
		idMat3 mountAxis;
		idVec3 mountOrg;
		targetAnimator->GetJointTransform( fanJoint, gameLocal.time, mountOrg, mountAxis );

		bool boost = false;
		sdPhysics_JetPack* jetpackPhysics = parent->GetPhysics()->Cast< sdPhysics_JetPack >();
		if ( jetpackPhysics ) {
			boost = jetpackPhysics->GetBoost() > 0.5f;
		}

		engineBoostEffect.GetRenderEffect().axis = ( mountAxis * modelAxis )[ 0 ].ToMat3();
		engineBoostEffect.GetRenderEffect().origin = mountOrg * modelAxis + modelOrg;
		engineMainEffect.GetRenderEffect().axis = ( mountAxis * modelAxis )[ 0 ].ToMat3();
		engineMainEffect.GetRenderEffect().origin = mountOrg * modelAxis + modelOrg;
		engineMainEffect.Start( gameLocal.time );
		engineMainEffect.Update();

		if ( boost ) {
			engineBoostEffect.Start( gameLocal.time );
		} else {
			engineBoostEffect.Stop();	
		}
		engineBoostEffect.Update();

		if ( boost ) {
			idMat3 orient = mountAxis * modelAxis;
			idVec3 dist(-800.f, 0.f, 0.f);
			dist = orient * dist;
			idVec3 traceOrg = mountOrg * modelAxis + modelOrg;
			idVec3 traceEnd = traceOrg + dist;

			trace_t		traceObject;
			gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS traceObject, traceOrg, traceEnd, MASK_SOLID | CONTENTS_WATER | MASK_OPAQUE, parent );
			traceEnd = traceObject.endpos;
			if ( gameLocal.time >= ( lastGroundEffectsTime + 100 )
				&& traceObject.fraction < 1.0f ) {

				const char* surfaceTypeName = NULL;
				if ( traceObject.c.surfaceType ) {
					surfaceTypeName = traceObject.c.surfaceType->GetName();
				}

				parent->PlayEffect( "fx_groundeffect", colorWhite.ToVec3(), surfaceTypeName, traceObject.endpos, traceObject.c.normal.ToMat3(), 0 );
				lastGroundEffectsTime = gameLocal.time;
			}
		}
	} else {
		engineMainEffect.Stop();	
		engineBoostEffect.Stop();	
	}
}

/*
===============================================================================

	sdVehicleRigidBodyPseudoHover

===============================================================================
*/

idCVar g_debugVehiclePseudoHover( "g_debugVehiclePseudoHover", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "show info about the pseudoHover component" );
#define MASK_PSEUDOHOVERCLIP			(CONTENTS_SOLID | CONTENTS_VEHICLECLIP | CONTENTS_WATER | CONTENTS_MONSTER)

CLASS_DECLARATION( sdVehiclePart, sdVehicleRigidBodyPseudoHover )
END_CLASS

/*
================
sdVehicleRigidBodyPseudoHover::sdVehicleRigidBodyPseudoHover
================
*/
sdVehicleRigidBodyPseudoHover::sdVehicleRigidBodyPseudoHover( void ) {
}

/*
================
sdVehicleRigidBodyPseudoHover::~sdVehicleRigidBodyPseudoHover
================
*/
sdVehicleRigidBodyPseudoHover::~sdVehicleRigidBodyPseudoHover( void ) {
	gameLocal.clip.DeleteClipModel( mainClipModel );
}

/*
================
sdVehicleRigidBodyPseudoHover::Init
================
*/
#define SIGNEDPOWER( a, b ) ( idMath::Sign( a ) * idMath::Pow( idMath::Fabs( a ), b ) )

void sdVehicleRigidBodyPseudoHover::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehiclePart::Init( part );

	oldDriver = NULL;
	parent = _parent;
	grounded = false;
	parkMode = false;
	lockedPark = false;

	hoverHeight = part.data.GetFloat( "height", "60" );
	parkHeight = part.data.GetFloat( "height_park", "30" );

	repulsionSpeedCoeff = part.data.GetFloat( "repulsion_speed_coeff" );
	repulsionSpeedHeight = part.data.GetFloat( "repulsion_speed_height" );
	repulsionSpeedMax = part.data.GetFloat( "repulsion_speed_max" );

	yawCoeff = part.data.GetFloat( "yaw_coeff" );

	fwdCoeff = part.data.GetFloat( "fwd_coeff" );
	fwdSpeedDampCoeff = part.data.GetFloat( "fwd_speed_damping_coeff" );
	fwdSpeedDampPower = part.data.GetFloat( "fwd_speed_damping_pow" );
	fwdSpeedDampMax = part.data.GetFloat( "fwd_speed_damping_max" );

	frontCastPos = part.data.GetFloat( "front_cast" );
	backCastPos = part.data.GetFloat( "back_cast" );

	castOffset = part.data.GetFloat( "cast_offset" );
	maxSlope = part.data.GetFloat( "max_slope" );
	slopeDropoff = part.data.GetFloat( "slope_dropoff" );

	parkTime = part.data.GetFloat( "park_time", "0.25" );

	effectJoint = parent->GetAnimator()->GetJointHandle( parent->spawnArgs.GetString( "joint_park_fx", "origin" ) );

	lastFrictionScale = 1.0f;

	startParkTime = 0;
	endParkTime = gameLocal.time;

	evalState.surfaceNormal.Set( 0.0f, 0.0f, 1.0f );

	//
	// TWTODO: Take these from the vscript
	//
	castStarts.Append( idVec3( frontCastPos, -80.0f, castOffset ) );
	castStarts.Append( idVec3( frontCastPos, 80.0f, castOffset ) );
	castStarts.Append( idVec3( backCastPos, -80.0f, castOffset ) );
	castStarts.Append( idVec3( backCastPos, 80.0f, castOffset ) );

	castDirections.Append( idVec3( 128.0f, -64.0f, 0.0f ) );
	castDirections.Append( idVec3( 128.0f, 64.0f, 0.0f ) );
	castDirections.Append( idVec3( -64.0f, -64.0f, 0.0f ) );
	castDirections.Append( idVec3( -64.0f, 64.0f, 0.0f ) );

	mainClipModel = NULL;

	targetVelocity.Zero();
	targetQuat.Set( 1.0f, 0.0f, 0.0f, 0.0f );

	lastParkEffectTime = 0;
	lastUnparkEffectTime = 0;

	chosenParkAxis.Identity();
	chosenParkOrigin.Zero();
}

/*
================
sdVehicleRigidBodyPseudoHover::RepulsionForCast
================
*/
void sdVehicleRigidBodyPseudoHover::DoRepulsionForCast( const idVec3& start, const idVec3& end, float desiredFraction, const trace_t& trace, float& height, int numForces ) {
	idVec3 traceDir = end - start;
	float fullLength = traceDir.Normalize();

	float traceLength = trace.fraction * fullLength;
	float idealLength = desiredFraction * fullLength;

	if ( trace.c.normal.z < 0.2f ) {
		// HACK - make it pretend like a trace went straight down the full distance if it touched 
		// a too-steep slope
		const_cast< idVec3& >( trace.c.normal ) = evalState.axis[ 2 ];
		traceLength = idealLength;
	}

	//
	// Calculate the repulsion
	//
	float reachingTime = 0.5f;
	if ( trace.fraction < 0.2f ) {
		reachingTime = 0.3f;
	}
	if ( trace.fraction < 0.1f ) {
		reachingTime = 0.15f;
	}

	if ( trace.fraction < 1.0f ) {
		float delta = idealLength - traceLength;
		float futureDistMoved = evalState.linVelocity * traceDir * reachingTime * 0.5f;
		float repulseAccel = 2 * ( delta + futureDistMoved ) / ( reachingTime*reachingTime );

		idVec3 hoverAccel = -repulseAccel*traceDir - evalState.gravity;

		// TODO: Use the hover force generated to create a torque and use that to re-orient the vehicle
		//		 Will need to make the surface angle matching etc code take that into account!
		evalState.hoverForce += hoverAccel * evalState.mass / numForces;

		evalState.surfaceNormal += ( 1.0f - trace.fraction ) * trace.c.normal;
	}

	height += trace.fraction;
}

/*
================
sdVehicleRigidBodyPseudoHover::DoRepulsors
================
*/
void sdVehicleRigidBodyPseudoHover::DoRepulsors() {
	const idVec3& upVector = evalState.axis[ 2 ];

	//
	// We want to cast out a bit forwards so that we don't tend to
	// bottom out when the slope of the surface changes
	//
	idVec3 forwardLeading = evalState.linVelocity * 0.3f;
	if ( startParkTime != 0 ) {
		forwardLeading.Zero();
	} else {
		forwardLeading -= ( upVector*forwardLeading ) * upVector;
		float fwdLeadLength = forwardLeading.Length();
		if ( fwdLeadLength > 200.0f ) {
			forwardLeading = forwardLeading * ( 200.0f / fwdLeadLength );
		}
	}

	idVec3 traceDirection = -upVector;
	traceDirection.Normalize();

	evalState.hoverForce.Zero();

	float mainTraceLength = hoverHeight;
	if ( startParkTime != 0 ) {
		mainTraceLength = hoverHeight * 3.0f;
	}

	float extraTraceLength = hoverHeight * 3.0f;
	evalState.surfaceNormal.Zero();
	float realHeight = 0.0f;

	//
	// Main body
	//
	if ( g_debugVehiclePseudoHover.GetBool() ) {
		mainClipModel->Draw( evalState.origin, evalState.axis );
	}

	memset( &groundTrace, 0, sizeof( groundTrace ) );

	idVec3 start = evalState.origin + 0.3f * castOffset * upVector;
	idVec3 end = start + mainTraceLength * traceDirection;
	end += forwardLeading;

	evalState.clipLocale.Translation( CLIP_DEBUG_PARMS groundTrace, start, end, mainClipModel, evalState.axis, MASK_PSEUDOHOVERCLIP );

	if ( groundTrace.c.normal.z < 0.2f ) {
		idVec3 end = start + idVec3( 0.0f, 0.0f, -1.0f ) * extraTraceLength;
		evalState.clipLocale.TracePoint( CLIP_DEBUG_PARMS groundTrace, start, end, MASK_PSEUDOHOVERCLIP );
	}
	DoRepulsionForCast( start, end, hoverHeight / mainTraceLength, groundTrace, realHeight, 2 );
	realHeight = 0.0f;

	if ( g_debugVehiclePseudoHover.GetBool() ) {
		mainClipModel->Draw( groundTrace.endpos, evalState.axis );
	}

	//
	// Do a bunch of other position casts
	int numCasts = castStarts.Num() < castDirections.Num() ? castStarts.Num() : castDirections.Num();
	for ( int i = 0; i < numCasts; i++ ) {
		trace_t currentTrace;

		idVec3 castStart = evalState.origin + castStarts[ i ] * evalState.axis;
		idVec3 castEnd = castStart + extraTraceLength * traceDirection;
		castEnd += forwardLeading;
		castEnd += castDirections[ i ] * evalState.axis;

		evalState.clipLocale.TracePoint( CLIP_DEBUG_PARMS currentTrace, castStart, castEnd, MASK_PSEUDOHOVERCLIP );
		
		if ( currentTrace.c.normal.z < 0.2f ) {
			idVec3 castEnd = castStart + idVec3( 0.0f, 0.0f, -1.0f ) * extraTraceLength;
			evalState.clipLocale.TracePoint( CLIP_DEBUG_PARMS currentTrace, castStart, castEnd, MASK_PSEUDOHOVERCLIP );
		}

		if ( g_debugVehiclePseudoHover.GetBool() ) {
			gameRenderWorld->DebugArrow( colorRed, castStart, currentTrace.endpos, 4 );
		}
		DoRepulsionForCast( castStart, castEnd, hoverHeight / extraTraceLength, currentTrace, realHeight, numCasts + 1 );
	}

	// Calculate the estimates of the normal & height
	realHeight = realHeight * extraTraceLength / numCasts;
	float normalLength = evalState.surfaceNormal.Normalize();
	if ( normalLength < idMath::FLT_EPSILON ) {
		evalState.surfaceNormal.Set( 0.0f, 0.0f, 1.0f );
	}

	// Don't let the generated up velocity exceed 300ups
	float currentUpVel = evalState.linVelocity * evalState.axis[ 2 ];
	float upForce = evalState.hoverForce*evalState.axis[ 2 ];
	if ( idMath::Fabs( upForce ) > idMath::FLT_EPSILON ) {
		float futureVel = currentUpVel + evalState.timeStep * upForce / evalState.mass ;
		float clampedFutureVel = idMath::ClampFloat( -300.0f, 300.0f, futureVel );
		float newUpForce = evalState.mass * ( clampedFutureVel - currentUpVel ) / evalState.timeStep;
		if ( realHeight > hoverHeight && newUpForce > 0.0f ) {
			// scale the up force down to prevent it getting ridiculously high up
			float scale = Lerp( 1.0f, 0.3f, ( realHeight - hoverHeight ) / hoverHeight * 2.0f );
			scale = idMath::ClampFloat( 0.3f, 1.0f, scale );
			newUpForce *= scale;
		}
		evalState.hoverForce *= newUpForce / upForce;
	}


	// 
	// Up vector based velocity dampening
	//
	grounded = false;
	if ( realHeight < repulsionSpeedHeight ) {
		// calculate the acceleration required to put a stop to this vertical velocity
		const idVec3 estFutureVelocity = evalState.linVelocity + evalState.hoverForce*( evalState.timeStep / evalState.mass );
		const float upVelocity = ( estFutureVelocity ) * evalState.surfaceNormal;
		float coefficient = repulsionSpeedCoeff;
		if ( upVelocity > 0.0f ) {
			coefficient *= 0.5f;
		}

		const float neededUpAccel = -upVelocity / evalState.timeStep;
		float upAccel = idMath::ClampFloat( -repulsionSpeedMax, repulsionSpeedMax, neededUpAccel * coefficient );
		evalState.hoverForce += evalState.mass * upAccel*evalState.surfaceNormal;

		grounded = true;
	}
}

/*
================
sdVehicleRigidBodyPseudoHover::CalculateSurfaceAxis
================
*/
void sdVehicleRigidBodyPseudoHover::CalculateSurfaceAxis() {
	if ( evalState.surfaceNormal == vec3_origin ) {
		evalState.surfaceNormal.Set( 0.0f, 0.0f, 1.0f );
	}

	idVec3 surfaceRight = evalState.surfaceNormal.Cross( evalState.axis[ 0 ] );
	surfaceRight.Normalize();
	idVec3 surfaceForward = surfaceRight.Cross( evalState.surfaceNormal );
	surfaceForward.Normalize();

	// construct the surface matrix
	idMat3 surfaceAxis( surfaceForward, surfaceRight, evalState.surfaceNormal );

	// slerp the surface axis with the old one to add some dampening
	evalState.surfaceQuat = surfaceAxis.ToQuat();
	evalState.surfaceAxis = evalState.surfaceQuat.ToMat3();
}

/*
================
sdVehicleRigidBodyPseudoHover::CalculateDrivingForce
================
*/
void sdVehicleRigidBodyPseudoHover::CalculateDrivingForce( const sdVehicleInput& input ) {
	if ( parkMode ) {
		evalState.drivingForce.Zero();
		return;
	}

	const idVec3& upVector = evalState.axis[ 2 ];
	const idVec3& surfaceUp = evalState.surfaceAxis[ 2 ];

	// "go forwards" impetus
	idVec3 fwdAccel( input.GetForward(), -input.GetCollective(), 0.0f );
	fwdAccel.Normalize();

	idVec3 inputAccel = fwdAccel * fwdCoeff * input.GetForce();

	// HACK: these should really go in the vscript or the def file.
	float targetSpeed = 380.0f;
	if ( input.GetForce() > 1.0f ) {
		targetSpeed = 605.0f;
	}

	float currentSpeed = evalState.linVelocity * ( fwdAccel * evalState.axis );
	float maxDelta = fwdCoeff * input.GetForce();
	float speedDelta = targetSpeed - currentSpeed;
	if ( speedDelta > 0.0f ) {
		speedDelta = Lerp( speedDelta, maxDelta, ( speedDelta / targetSpeed ) * 0.15f + 0.85f );
		if ( speedDelta > maxDelta ) {
			speedDelta = maxDelta;
		}
	} else {
		speedDelta = 0.0f;
	}

	inputAccel = speedDelta * fwdAccel;

	// figure out what that means in terms of a velocity delta
	idVec3 vDelta = inputAccel * evalState.timeStep;
	// figure out what force is needed to get that sort of vDelta, considering the hovering forces
	idVec3 neededForce = evalState.surfaceAxis*inputAccel*evalState.mass - evalState.hoverForce;
	neededForce -= ( neededForce*surfaceUp )*surfaceUp;
	evalState.drivingForce = neededForce;

	//
	// Figure out if the slope is too steep, and ramp the force down
	//
	float angleBetween = idMath::ACos( idVec3( 0.0f, 0.0f, 1.0f ) * surfaceUp ) * idMath::M_RAD2DEG;
	if ( angleBetween > maxSlope) {
		// calculate the direction vectors directly up and tangential to the slope
		idVec3 slopeRight = surfaceUp.Cross( idVec3( 0.0f, 0.0f, 1.0f ) );
		idVec3 slopeForward = slopeRight.Cross( surfaceUp );

		slopeRight.Normalize();
		slopeForward.Normalize();

		// ramp down the force
		float accelScale = 1.0f - ( angleBetween - maxSlope ) * slopeDropoff;
		if ( accelScale < 0.0f ) {
			accelScale = 0.0f;
		}

		// HACK: Drop off the force quicker when boosting
		if ( input.GetForce() > 1.0f ) {
			accelScale *= accelScale;
		}

		if ( angleBetween > maxSlope * 2.0f ) {
			// way beyond the max slope, zero all force up the slope
			accelScale = 0.0f;
		}

		// remove the component up the slope
		float upSlopeComponent = evalState.drivingForce * slopeForward;
		if ( upSlopeComponent > 0.0f ) {
			evalState.drivingForce = evalState.drivingForce - slopeForward * upSlopeComponent * ( 1.f - accelScale );
		}
		float hoverUpSlopeComponent = evalState.hoverForce * slopeForward;
		if ( hoverUpSlopeComponent > 0.0f ) {
			evalState.hoverForce = evalState.hoverForce - slopeForward * hoverUpSlopeComponent * ( 1.f - accelScale );
		}

		// remove the component into the slope
		float intoSlopeComponent = evalState.drivingForce * surfaceUp;
		if ( intoSlopeComponent > 0.0f ) {
			evalState.drivingForce = evalState.drivingForce - surfaceUp * intoSlopeComponent * ( 1.f - accelScale );
		}
		float hoverIntoSlopeComponent = evalState.hoverForce * slopeForward;
		if ( hoverIntoSlopeComponent > 0.0f ) {
			evalState.hoverForce = evalState.hoverForce - slopeForward * hoverIntoSlopeComponent * ( 1.f - accelScale );
		}
	}
}

/*
================
sdVehicleRigidBodyPseudoHover::CalculateFrictionForce
================
*/
void sdVehicleRigidBodyPseudoHover::CalculateFrictionForce( const sdVehicleInput& input ) {
	const idVec3& upVector = evalState.axis[ 2 ];
	const idVec3& surfaceUp = evalState.surfaceAxis[ 2 ];

	// calculate the future velocity that will be created by the forces that are applied
	idVec3 futureVelocity = evalState.linVelocity 
							+ ( evalState.hoverForce + evalState.drivingForce ) * evalState.timeStep / evalState.mass
							/*+ evalState.gravity * evalState.timeStep*/;

	// project it to the plane
	futureVelocity -= ( futureVelocity * surfaceUp ) * surfaceUp;
	float futureSpeed = futureVelocity.Normalize();

	// apply friction to it
	float futureVelDamp = -fwdSpeedDampCoeff * SIGNEDPOWER( futureSpeed, fwdSpeedDampPower );
	futureVelDamp = idMath::ClampFloat( -fwdSpeedDampMax, fwdSpeedDampMax, futureVelDamp );
	if ( fabs( input.GetForce() ) > 0.0f ) {
		futureVelDamp /= input.GetForce();
	} else {
		futureVelDamp = 0.0f;
	}

	// if the speed is quite small or the player is giving no input 
	// then increase the magnitude of the friction
	// TODO: Move these tuning parameters to vscript defined thingies
	float frictionScaleCutoff = 128.0f;
	float frictionScaleMax = 15.0f;
	float frictionScalePower = 0.5f; 
	float frictionScale = 0.8f;

	float parkTimeRemaining = parkTime;
	if ( startParkTime != 0 ) {
		parkTimeRemaining = parkTime - MS2SEC( gameLocal.time - startParkTime );
	} else if ( endParkTime != 0 ) {
		parkTimeRemaining = MS2SEC( gameLocal.time - endParkTime );
	}
	parkTimeRemaining /= parkTime;
	parkTimeRemaining = idMath::ClampFloat( 0.0f, 1.0f, parkTimeRemaining );
	float parkScale = parkTimeRemaining;

	if ( parkTimeRemaining < 1.0f || ( input.GetForward() == 0.0f && input.GetCollective() == 0.0f ) ) {
		if ( futureSpeed < frictionScaleCutoff ) {
			frictionScale = ( frictionScaleCutoff - futureSpeed ) / frictionScaleCutoff;
			frictionScale = idMath::Pow( frictionScale, frictionScalePower );
			frictionScale = frictionScale * frictionScaleMax + 1.0f;
		} else {
			// its going too fast to be acted on by the really strong power
			// hack a different friction that will slow it down to the friction cutoff over time
			lastFrictionScale = 1.0f;
			// calculate the acceleration needed to nullify the velocity
			futureVelDamp = -( futureSpeed / evalState.timeStep ) / 5.0f;
		}
	} else {
		// the player is applying input, so snap to zero friction scale
		lastFrictionScale = 1.0f;
	}

	// blend the friction scale with the last frame's one
	frictionScale = lastFrictionScale * 0.7f + frictionScale * 0.3f;
	frictionScale *= parkScale;
	lastFrictionScale = frictionScale;

	// apply the friction
	evalState.frictionForce = futureVelDamp * evalState.mass * frictionScale * futureVelocity;
}

/*
================
sdVehicleRigidBodyPseudoHover::CalculateTilting
================
*/
void sdVehicleRigidBodyPseudoHover::CalculateTilting( const sdVehicleInput& input ) {
	evalState.surfaceMatchingQuat = evalState.surfaceQuat;

	if ( startParkTime == 0 ) {
		idVec3 inputMove( -input.GetForward(), -( input.GetCollective() + 2.0f*input.GetSteerAngle() ) * 0.5f, 0.0f );
		inputMove *= 2.5f * input.GetForce();

		// modify the surface quat using the movement keys
		idRotation pitchRotation( vec3_origin, evalState.axis[ 1 ], inputMove.x );
		idRotation yawRotation( vec3_origin, evalState.axis[ 0 ], inputMove.y );
		evalState.surfaceMatchingQuat *= pitchRotation.ToQuat();
		evalState.surfaceMatchingQuat *= yawRotation.ToQuat();
	}

	targetQuat = evalState.surfaceMatchingQuat;
}

/*
================
sdVehicleRigidBodyPseudoHover::CalculateYaw
================
*/
void sdVehicleRigidBodyPseudoHover::CalculateYaw( const sdVehicleInput& input ) {
	// calculate the desired angular velocity for the yaw
	float yawVel = -yawCoeff * input.GetSteerAngle() * input.GetForce() * 0.04f;
	if ( input.GetForward() < 0.0f ) {
		yawVel = -yawVel;
	}
	evalState.steeringAngVel = evalState.axis[ 2 ] * yawVel;

	// ramp the steering based on park mode
	float parkTimeRemaining = 1.0f;
	if ( startParkTime != 0 ) {
		parkTimeRemaining = parkTime - MS2SEC( gameLocal.time - startParkTime );
	} else if ( endParkTime != 0 ) {
		parkTimeRemaining = MS2SEC( gameLocal.time - endParkTime );
	}
	parkTimeRemaining /= parkTime;
	parkTimeRemaining = idMath::ClampFloat( 0.0f, 1.0f, parkTimeRemaining );
	evalState.steeringAngVel *= parkTimeRemaining;


	float rotation = RAD2DEG( -yawVel * parkTimeRemaining * evalState.timeStep * 3.0f );
	idRotation rot( vec3_origin, evalState.axis[ 2 ], rotation );
	idQuat rotQuat = rot.ToQuat();
	targetQuat = targetQuat * rotQuat;
}

/*
================
sdVehicleRigidBodyPseudoHover::ChooseParkPosition
================
*/
void sdVehicleRigidBodyPseudoHover::ChooseParkPosition() {

	// The method here: - Cast down from all the corners of the bounds, & the main bounds
	//					- Use those to estimate an angle to park at
	//					- Cast the bounds down using the average normal to find the ground
	//					- Find a position offset above the ground to park at

	grounded = false;

	// do the downward casts
	float midHeight = ( mainBounds[ 0 ].z + mainBounds[ 1 ].z ) * 0.5f;
	idVec3 castOffset = midHeight * evalState.axis[ 2 ];

	const idVec3& upVector = evalState.axis[ 2 ];
	idVec3 traceVector( 0.0f, 0.0f, -1024.0f );

	idVec3 start = evalState.origin;
	idVec3 end = start + traceVector;
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	evalState.clipLocale.Translation( CLIP_DEBUG_PARMS groundTrace, start, end, mainClipModel, evalState.axis, MASK_PSEUDOHOVERCLIP );
	grounded |= groundTrace.fraction < 1.0f;

	idVec3 mainBodyEndPoint = groundTrace.endpos;
	idVec3 mainBodyNormal = groundTrace.c.normal;

	if ( g_debugVehiclePseudoHover.GetBool() ) {
		gameRenderWorld->DebugBounds( colorGreen, mainBounds, evalState.origin, evalState.axis, 10000 );
	}

	// front
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	start = evalState.origin + mainBounds[ 1 ].x * evalState.axis[ 0 ] + castOffset;
	end = start + traceVector;
	evalState.clipLocale.TracePoint( CLIP_DEBUG_PARMS groundTrace, start, end , MASK_PSEUDOHOVERCLIP );
	grounded |= groundTrace.fraction < 1.0f;
	
	idVec3 frontEndPoint = groundTrace.endpos;
	idVec3 frontNormal = groundTrace.c.normal;

	if ( g_debugVehiclePseudoHover.GetBool() ) {
		gameRenderWorld->DebugArrow( colorGreen, start, frontEndPoint, 4, 10000 );
	}

	// back
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	start = evalState.origin + mainBounds[ 0 ].x * evalState.axis[ 0 ] + castOffset;
	end = start + traceVector;
	evalState.clipLocale.TracePoint( CLIP_DEBUG_PARMS groundTrace, start, end , MASK_PSEUDOHOVERCLIP );
	grounded |= groundTrace.fraction < 1.0f;
	
	idVec3 backEndPoint = groundTrace.endpos;
	idVec3 backNormal = groundTrace.c.normal;

	if ( g_debugVehiclePseudoHover.GetBool() ) {
		gameRenderWorld->DebugArrow( colorGreen, start, backEndPoint, 4, 10000 );
	}

	// left
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	start = evalState.origin + mainBounds[ 1 ].y * evalState.axis[ 1 ] + castOffset;
	end = start + traceVector;
	evalState.clipLocale.TracePoint( CLIP_DEBUG_PARMS groundTrace, start, end , MASK_PSEUDOHOVERCLIP );
	grounded |= groundTrace.fraction < 1.0f;
	
	idVec3 leftEndPoint = groundTrace.endpos;
	idVec3 leftNormal = groundTrace.c.normal;

	if ( g_debugVehiclePseudoHover.GetBool() ) {
		gameRenderWorld->DebugArrow( colorGreen, start, leftEndPoint, 4, 10000 );
	}

	// right
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	start = evalState.origin + mainBounds[ 0 ].y * evalState.axis[ 1 ] + castOffset;
	end = start + traceVector;
	evalState.clipLocale.TracePoint( CLIP_DEBUG_PARMS groundTrace, start, end , MASK_PSEUDOHOVERCLIP );
	grounded |= groundTrace.fraction < 1.0f;
	
	idVec3 rightEndPoint = groundTrace.endpos;
	idVec3 rightNormal = groundTrace.c.normal;

	if ( g_debugVehiclePseudoHover.GetBool() ) {
		gameRenderWorld->DebugArrow( colorGreen, start, rightEndPoint, 4, 10000 );
	}

	if ( !grounded ) {
		evalState.surfaceNormal = vec3_origin;
		return;
	}

	//
	// Use the info gleaned to estimate a surface normal
	//
	
	// Calculate the pitch angle of the surface
	idVec3 frontBack = frontEndPoint - backEndPoint;
	idVec3 frontBackDir = frontBack;
	frontBackDir.Normalize();
	float pitchAngle = idMath::ACos( idVec3( 0.0f, 0.0f, 1.0f ) * frontBackDir ) * idMath::M_RAD2DEG - 90.0f;

	// Calculate the roll angle of the surface
	idVec3 leftRight = rightEndPoint - leftEndPoint;
	idVec3 leftRightDir = leftRight;
	leftRightDir.Normalize();
	float rollAngle = idMath::ACos( idVec3( 0.0f, 0.0f, 1.0f ) * leftRightDir ) * idMath::M_RAD2DEG - 90.0f;

	float yawAngle = evalState.axis.ToAngles().yaw;

	idAngles newAngles( pitchAngle, yawAngle, rollAngle );
	idMat3 chosenAxis = newAngles.ToMat3();
	chosenParkAxis = chosenAxis;

	// now cast the main body down with that normal as its 
	start = evalState.origin;
	end = start - 1024.0f * chosenParkAxis[ 2 ];
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	evalState.clipLocale.Translation( CLIP_DEBUG_PARMS groundTrace, start, end, mainClipModel, chosenAxis, MASK_PSEUDOHOVERCLIP );

	chosenParkOrigin = groundTrace.endpos + chosenParkAxis[ 2 ] * parkHeight;

	foundPark = true;

	if ( g_debugVehiclePseudoHover.GetBool() ) {
		gameRenderWorld->DebugArrow( colorRed, chosenParkOrigin, chosenParkOrigin + chosenAxis[ 0 ] * 128.0f, 4, 10000 );
		gameRenderWorld->DebugArrow( colorGreen, chosenParkOrigin, chosenParkOrigin + chosenAxis[ 1 ] * 128.0f, 4, 10000 );
		gameRenderWorld->DebugArrow( colorBlue, chosenParkOrigin, chosenParkOrigin + chosenAxis[ 2 ] * 128.0f, 4, 10000 );
	}
}

/*
================
sdVehicleRigidBodyPseudoHover::DoParkRepulsors
================
*/
void sdVehicleRigidBodyPseudoHover::DoParkRepulsors() {
	//
	// Park mode hovering
	//
	float timeRemaining = parkTime - MS2SEC( gameLocal.time - startParkTime );
	if ( timeRemaining < evalState.timeStep ) {
		timeRemaining = evalState.timeStep;
	}

	if ( !foundPark ) {
		chosenParkAxis = evalState.axis;
		chosenParkOrigin = evalState.origin;
	}

	evalState.surfaceNormal = chosenParkAxis[ 2 ];
	idVec3 distToMove = chosenParkOrigin - evalState.origin;

	assert( chosenParkAxis[ 2 ] != vec3_origin );
	if ( chosenParkAxis[ 2 ] == vec3_origin ) {
		return;
	}

	float distToMoveLength = distToMove.Length();

	if ( distToMoveLength < 25.0f ) {
		lockedPark = true;
	}

	if ( distToMoveLength > 512.0f ) {
		lockedPark = false;
	}

	idVec3 neededAcceleration;
	if ( distToMoveLength < 10.0f ) {
		// close enough! just figure out how to cancel out our existing velocity
		neededAcceleration = -0.2f * evalState.linVelocity / evalState.timeStep;
	} else {

		// figure out a velocity to reach that point in the time remaining
		idVec3 neededVelocity = distToMove / timeRemaining;
		float neededVelocityLength = neededVelocity.Length();
		if ( neededVelocityLength > 100.0f ) {
			neededVelocity *= 100.0f / neededVelocityLength;
		}

		// figure out what acceleration is needed to get to that velocity in the next frame
		idVec3 vDelta = neededVelocity - evalState.linVelocity;
 		neededAcceleration = vDelta / evalState.timeStep;
	}

	// make sure nothing ridiculous is happening with the acceleration
	float accelLength = neededAcceleration.Length();
	if ( accelLength > 500.0f ) {
		neededAcceleration *= 500.0f / accelLength;
	}

	neededAcceleration -= evalState.gravity;
	evalState.hoverForce = neededAcceleration * evalState.mass;
}

/*
================
sdVehicleRigidBodyPseudoHover::UpdatePrePhysics
================
*/
void sdVehicleRigidBodyPseudoHover::UpdatePrePhysics( const sdVehicleInput& input ) {
	evalState.physics = parent->GetPhysics()->Cast< sdPhysics_RigidBodyMultiple >();
	if ( evalState.physics == NULL ) {
		return;
	}

	if ( mainClipModel == NULL ) {
		mainBounds.Clear();
		for ( int i = 0; i < evalState.physics->GetNumClipModels(); i++ ) {
			int contents = evalState.physics->GetContents( i );
			if ( contents && contents != MASK_HURTZONE ) {
				idBounds bounds = evalState.physics->GetBounds( i );
				bounds.TranslateSelf( evalState.physics->GetBodyOffset( i ) );
				mainBounds.AddBounds( bounds );
			}
		}

		mainClipModel = new idClipModel( idTraceModel( mainBounds ), false );
	}

	evalState.timeStep = MS2SEC( gameLocal.msec );
	evalState.driver = parent->GetPositionManager().FindDriver();

	// get the set of clip models to trace against
	idBounds localeBounds = mainBounds;
	localeBounds[ 0 ].z -= hoverHeight;
	evalState.clipLocale.Update( localeBounds, evalState.physics->GetOrigin(), evalState.physics->GetAxis(), 
								evalState.physics->GetLinearVelocity(), evalState.physics->GetAngularVelocity(), 
								MASK_PSEUDOHOVERCLIP, parent );
	evalState.clipLocale.RemoveEntitiesOfCollection( "vehicles" );
	evalState.clipLocale.RemoveEntitiesOfCollection( "deployables" );

	// see if its too steep to go into siege mode
	bool wantsHandBrake = input.GetHandBraking();
	if ( wantsHandBrake ) {
		float angleBetween = idMath::ACos( idVec3( 0.0f, 0.0f, 1.0f ) * evalState.surfaceNormal ) * idMath::M_RAD2DEG;
		if ( angleBetween > maxSlope ) {
			wantsHandBrake = false;
			parent->GetVehicleControl()->CancelSiegeMode();
		}
	}

	if ( parkMode && ( !wantsHandBrake || !grounded ) && !parent->IsEMPed() ) {
		parkMode = false;
		foundPark = false;
		lockedPark = false;
		startParkTime = 0;
		endParkTime = gameLocal.time;

		if ( gameLocal.time - lastUnparkEffectTime > 500 ) {
			parent->PlayEffect( "fx_park_disengage", colorWhite.ToVec3(), NULL, effectJoint );
			if ( oldDriver != NULL ) {
				parent->StartSound( "snd_park_disengage", SND_VEHICLE_MISC, SND_VEHICLE_MISC, 0, NULL );
			}

			lastUnparkEffectTime = gameLocal.time;
		}
	} else if ( ( parent->IsEMPed() || wantsHandBrake ) && !parkMode && grounded ) {
		parkMode = true;
		foundPark = false;
		lockedPark = false;
		startParkTime = gameLocal.time;
		lastParkUpdateTime = 0;
		endParkTime = 0;

		if ( gameLocal.time - lastParkEffectTime > 500 ) {
			parent->PlayEffect( "fx_park_engage", colorWhite.ToVec3(), NULL, effectJoint );
			if ( evalState.driver != NULL ) {
				parent->StartSound( "snd_park_engage", SND_VEHICLE_MISC, SND_VEHICLE_MISC, 0, NULL );
			}

			lastParkEffectTime = gameLocal.time;
		}
	}

	oldDriver = evalState.driver;

	// inform the vehicle control if we've finished parking or not
	sdVehicleControlBase* control = parent->GetVehicleControl();
	if ( control != NULL ) {
		control->SetSiegeMode( lockedPark );
	}

	//
	// Harvest data
	//
	evalState.origin = evalState.physics->GetOrigin();
	evalState.axis = evalState.physics->GetAxis();
	evalState.mass = evalState.physics->GetMass( -1 );
	evalState.gravity = evalState.physics->GetGravity();

	evalState.linVelocity = evalState.physics->GetLinearVelocity() + evalState.gravity * evalState.timeStep;
	evalState.angVelocity = evalState.physics->GetAngularVelocity();

	evalState.inertiaTensor = evalState.physics->GetInertiaTensor();

	//
	// Handle park updating
	//
	if ( !gameLocal.isClient ) {
		// HACK: seek new parking places when in contact with an MCP
		if ( parkMode && lockedPark ) {
			for ( int i = 0; i < evalState.physics->GetNumContacts(); i++ ) {
				const contactInfo_t& contact = evalState.physics->GetContact( i );
				idEntity* contactEnt = gameLocal.entities[ contact.entityNum ];
				if ( contactEnt != NULL && contactEnt->Cast< sdTransport >() != NULL ) {
					if ( !contactEnt->IsCollisionPushable() ) {
						lockedPark = false;
						foundPark = false;
					}
				}
			}
		}

		// try picking a place to park
		if ( parkMode && ( !foundPark || gameLocal.time > lastParkUpdateTime + 750 ) && !lockedPark ) {
			foundPark = false;
			ChooseParkPosition();
			lastParkUpdateTime = gameLocal.time;
		}
	} else {
		// clients do a simplified form of what the server does, just so they can
		// predict the initial park location and start the process - otherwise they'll
		// continually override what the server tells them to do
		if ( parkMode && !foundPark && gameLocal.isNewFrame ) {
			ChooseParkPosition();
		}
	}

	//
	// Hovering
	//
	if ( !parkMode ) {
		DoRepulsors();
	} else if ( !lockedPark ) {
		DoParkRepulsors();
	}

	if ( !lockedPark ) {
		CalculateSurfaceAxis();
		CalculateDrivingForce( input );
		CalculateFrictionForce( input );
		CalculateTilting( input );
		CalculateYaw( input );

		idVec3 force = evalState.drivingForce + evalState.frictionForce + evalState.hoverForce;
		targetVelocity = evalState.linVelocity + evalState.timeStep * force / evalState.mass;

		evalState.physics->Activate();
	}
}

/*
================
sdVehicleRigidBodyPseudoHover::UpdatePostPhysics
================
*/
void sdVehicleRigidBodyPseudoHover::UpdatePostPhysics( const sdVehicleInput& input ) {
}

/*
================
sdVehicleRigidBodyPseudoHover::AddCustomConstraints
================
*/
const float CONTACT_LCP_EPSILON		= 1e-8f;

int sdVehicleRigidBodyPseudoHover::AddCustomConstraints( constraintInfo_t* list, int max ) {

	if ( !grounded ) {
		return 0;
	}

	idPhysics* parentPhysics = parent->GetPhysics();

	idVec3 targVel = vec3_origin;
	idVec3 targAngVel = vec3_origin;

	idQuat toQuat;
	float angVelDamp = 0.05f;

	if ( !lockedPark ) {
		targVel = targetVelocity;
		toQuat = targetQuat;
	} else {
		idVec3 delta = 0.2f * ( chosenParkOrigin - parentPhysics->GetOrigin() );
		delta /= MS2SEC( gameLocal.msec );
		delta *= 0.5f;
		targVel = delta;
		toQuat = chosenParkAxis.ToQuat();
		angVelDamp = 0.1f;
	}

	idQuat fromQuat = parentPhysics->GetAxis().ToQuat();
	idQuat diffQuat = toQuat.Inverse() * fromQuat;
	targAngVel = ( diffQuat.ToAngularVelocity() - parentPhysics->GetAngularVelocity() * angVelDamp ) / MS2SEC( gameLocal.msec );

	//
	// LINEAR VELOCITY
	//

	idVec3 comWorld = parentPhysics->GetCenterOfMass() * parentPhysics->GetAxis() + parentPhysics->GetOrigin();

	// add the position matching constraints
	constraintInfo_t& vx = list[ 0 ];
	constraintInfo_t& vy = list[ 1 ];
	constraintInfo_t& vz = list[ 2 ];

	vx.j.SubVec3( 0 ).Set( 1.0f, 0.0f, 0.0f );
	vy.j.SubVec3( 0 ).Set( 0.0f, 1.0f, 0.0f );
	vz.j.SubVec3( 0 ).Set( 0.0f, 0.0f, 1.0f );
	vx.j.SubVec3( 1 ).Zero();
	vy.j.SubVec3( 1 ).Zero();
	vz.j.SubVec3( 1 ).Zero();
	vx.boxIndex = vy.boxIndex = vz.boxIndex = -1;
	vx.error = vy.error = vz.error = CONTACT_LCP_EPSILON;
	vx.pos = vy.pos = vz.pos = comWorld;
	vx.lm = vy.lm = vz.lm = 0.0f;

	vx.c = -targVel.x;
	vy.c = -targVel.y;
	vz.c = -targVel.z;

	// calculate the force needed to make it in one frame
	idVec3 force = ( targVel - parentPhysics->GetLinearVelocity() ) / MS2SEC( gameLocal.msec );

	// limit the max acceleration
	float forceLength = force.Normalize();
	if ( forceLength > 4000.0f ) {
		forceLength = 4000.0f;
	}
	force = force * forceLength;
	force -= parentPhysics->GetGravity();
	force *= parentPhysics->GetMass();

	if ( force.x < 0.0f ) {
		vx.lo = force.x;
		vx.hi = 0.0f;
	} else {
		vx.hi = force.x;
		vx.lo = 0.0f;
	}
	if ( force.y < 0.0f ) {
		vy.lo = force.y;
		vy.hi = 0.0f;
	} else {
		vy.hi = force.y;
		vy.lo = 0.0f;
	}
	if ( force.z < 0.0f ) {
		vz.lo = force.z;
		vz.hi = 0.0f;
	} else {
		vz.hi = force.z;
		vz.lo = 0.0f;
	}

	//
	// ANGULAR VELOCITY
	//

	constraintInfo_t& wx = list[ 3 ];
	constraintInfo_t& wy = list[ 4 ];
	constraintInfo_t& wz = list[ 5 ];

	wx.j.SubVec3( 0 ).Zero();
	wy.j.SubVec3( 0 ).Zero();
	wz.j.SubVec3( 0 ).Zero();
	wx.j.SubVec3( 1 ).Set( 1.0f, 0.0f, 0.0f );
	wy.j.SubVec3( 1 ).Set( 0.0f, 1.0f, 0.0f );
	wz.j.SubVec3( 1 ).Set( 0.0f, 0.0f, 1.0f );
	wx.boxIndex = wy.boxIndex = wz.boxIndex = -1;
	wx.error = wy.error = wz.error = CONTACT_LCP_EPSILON;
	wx.pos = wy.pos = wz.pos = comWorld;
	wx.lm = wy.lm = wz.lm = 0.0f;

	wx.c = -targAngVel.x;
	wy.c = -targAngVel.y;
	wz.c = -targAngVel.z;

	// calculate the alpha needed to achieve the desired angular movements
	idVec3 alpha = ( targAngVel - parentPhysics->GetAngularVelocity() ) / MS2SEC( gameLocal.msec );
	// limit the max acceleration
	float alphaLength = alpha.Normalize();
	if ( alphaLength > 400.0f ) {
		alphaLength = 400.0f;
	}
	alpha = alpha * alphaLength;
	alpha *= parentPhysics->GetInertiaTensor();

	if ( alpha.x < 0.0f ) {
		wx.lo = alpha.x;
		wx.hi = 0.0f;
	} else {
		wx.hi = alpha.x;
		wx.lo = 0.0f;
	}
	if ( alpha.y < 0.0f ) {
		wy.lo = alpha.y;
		wy.hi = 0.0f;
	} else {
		wy.hi = alpha.y;
		wy.lo = 0.0f;
	}
	if ( alpha.z < 0.0f ) {
		wz.lo = alpha.z;
		wz.hi = 0.0f;
	} else {
		wz.hi = alpha.z;
		wz.lo = 0.0f;
	}

	return 6;
}

/*
================
sdVehicleRigidBodyPseudoHover::CreateNetworkStructure
================
*/
sdEntityStateNetworkData*	sdVehicleRigidBodyPseudoHover::CreateNetworkStructure( networkStateMode_t mode ) const { 
	if ( mode == NSM_BROADCAST ) {
		return new sdPseudoHoverBroadcastData;
	}

	if ( mode == NSM_VISIBLE ) {
		return new sdPseudoHoverNetworkData;
	}

	return NULL;
}

/*
================
sdVehicleRigidBodyPseudoHover::CheckNetworkStateChanges
================
*/
bool sdVehicleRigidBodyPseudoHover::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdPseudoHoverBroadcastData );

		NET_CHECK_FIELD( parkMode, parkMode );
		NET_CHECK_FIELD( foundPark, foundPark );
		NET_CHECK_FIELD( lockedPark, lockedPark );
		NET_CHECK_FIELD( startParkTime, startParkTime );
		NET_CHECK_FIELD( endParkTime, endParkTime );
		NET_CHECK_FIELD( lastParkUpdateTime, lastParkUpdateTime );
		NET_CHECK_FIELD( chosenParkOrigin, chosenParkOrigin );
		NET_CHECK_FIELD( chosenParkAxis, chosenParkAxis );

		return false;
	}

	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdPseudoHoverNetworkData );

		NET_CHECK_FIELD( lastFrictionScale, lastFrictionScale );

		return false;
	}

	return false;
}

/*
================
sdVehicleRigidBodyPseudoHover::WriteNetworkState
================
*/
void sdVehicleRigidBodyPseudoHover::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPseudoHoverBroadcastData );

		newData.parkMode = parkMode;
		newData.foundPark = foundPark;
		newData.lockedPark = lockedPark;
		newData.startParkTime = startParkTime;
		newData.endParkTime = endParkTime;
		newData.lastParkUpdateTime = lastParkUpdateTime;
		newData.chosenParkOrigin = chosenParkOrigin;
		newData.chosenParkAxis = chosenParkAxis;

		msg.WriteBool( newData.parkMode );
		msg.WriteBool( newData.foundPark );
		msg.WriteBool( newData.lockedPark );
		msg.WriteDeltaLong( baseData.startParkTime, newData.startParkTime );
		msg.WriteDeltaLong( baseData.endParkTime, newData.endParkTime );
		msg.WriteDeltaLong( baseData.lastParkUpdateTime, newData.lastParkUpdateTime );
		msg.WriteDeltaVector( baseData.chosenParkOrigin, newData.chosenParkOrigin );
		msg.WriteDeltaCQuat( baseData.chosenParkAxis.ToCQuat(), newData.chosenParkAxis.ToCQuat() );

		return;
	}

	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdPseudoHoverNetworkData );

		newData.lastFrictionScale = lastFrictionScale;

		msg.WriteDeltaFloat( baseData.lastFrictionScale, newData.lastFrictionScale );

		return;
	}
}

/*
================
sdVehicleRigidBodyPseudoHover::ReadNetworkState
================
*/
void sdVehicleRigidBodyPseudoHover::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdPseudoHoverNetworkData );

		newData.lastFrictionScale	= msg.ReadDeltaFloat( baseData.lastFrictionScale );		

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPseudoHoverBroadcastData );

		newData.parkMode			= msg.ReadBool();
		newData.foundPark			= msg.ReadBool();
		newData.lockedPark			= msg.ReadBool();
		newData.startParkTime		= msg.ReadDeltaLong( baseData.startParkTime );
		newData.endParkTime			= msg.ReadDeltaLong( baseData.endParkTime );
		newData.lastParkUpdateTime	= msg.ReadDeltaLong( baseData.lastParkUpdateTime );
		newData.chosenParkOrigin	= msg.ReadDeltaVector( baseData.chosenParkOrigin );

		idCQuat readQuat			= msg.ReadDeltaCQuat( baseData.chosenParkAxis.ToCQuat() );
		newData.chosenParkAxis		= readQuat.ToMat3();
		return;
	}
}

/*
================
sdVehicleRigidBodyPseudoHover::ApplyNetworkState
================
*/
void sdVehicleRigidBodyPseudoHover::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdPseudoHoverNetworkData );

		lastFrictionScale = newData.lastFrictionScale;

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPseudoHoverBroadcastData );

		parkMode = newData.parkMode;
		foundPark = newData.foundPark;
		lockedPark = newData.lockedPark;
		startParkTime = newData.startParkTime;
		endParkTime = newData.endParkTime;
		lastParkUpdateTime = newData.lastParkUpdateTime;
		chosenParkOrigin = newData.chosenParkOrigin;
		chosenParkAxis = newData.chosenParkAxis;

		return;
	}
}

/*
================
sdPseudoHoverNetworkData::MakeDefault
================
*/
void sdPseudoHoverNetworkData::MakeDefault( void ) {
	lastFrictionScale = 1.0f;
}

/*
================
sdPseudoHoverNetworkData::Write
================
*/
void sdPseudoHoverNetworkData::Write( idFile* file ) const {
	file->WriteFloat( lastFrictionScale );
}

/*
================
sdPseudoHoverNetworkData::Read
================
*/
void sdPseudoHoverNetworkData::Read( idFile* file ) {
	file->ReadFloat( lastFrictionScale );
}

/*
================
sdPseudoHoverBroadcastData::MakeDefault
================
*/
void sdPseudoHoverBroadcastData::MakeDefault( void ) {
	parkMode = false;
	foundPark = false;
	lockedPark = false;
	startParkTime = 0;
	endParkTime = gameLocal.time;
	lastParkUpdateTime = 0;
	chosenParkOrigin.Zero();
	chosenParkAxis.Identity();
}

/*
================
sdPseudoHoverBroadcastData::Write
================
*/
void sdPseudoHoverBroadcastData::Write( idFile* file ) const {
	file->WriteBool( parkMode );
	file->WriteBool( foundPark );
	file->WriteBool( lockedPark );
	file->WriteInt( startParkTime );
	file->WriteInt( endParkTime );
	file->WriteInt( lastParkUpdateTime );
	file->WriteVec3( chosenParkOrigin );
	file->WriteMat3( chosenParkAxis );
}

/*
================
sdPseudoHoverBroadcastData::Read
================
*/
void sdPseudoHoverBroadcastData::Read( idFile* file ) {
	file->ReadBool( parkMode );
	file->ReadBool( foundPark );
	file->ReadBool( lockedPark );
	file->ReadInt( startParkTime );
	file->ReadInt( endParkTime );
	file->ReadInt( lastParkUpdateTime );
	file->ReadVec3( chosenParkOrigin );
	file->ReadMat3( chosenParkAxis );
}

/*
===============================================================================

	sdVehicleRigidBodyDragPlane

===============================================================================
*/
CLASS_DECLARATION( sdVehiclePart, sdVehicleRigidBodyDragPlane )
END_CLASS

/*
================
sdVehicleRigidBodyDragPlane::sdVehicleRigidBodyDragPlane
================
*/
sdVehicleRigidBodyDragPlane::sdVehicleRigidBodyDragPlane( void ) {
}

/*
================
sdVehicleRigidBodyDragPlane::~sdVehicleRigidBodyDragPlane
================
*/
sdVehicleRigidBodyDragPlane::~sdVehicleRigidBodyDragPlane( void ) {
}

/*
================
sdVehicleRigidBodyDragPlane::Init
================
*/
void sdVehicleRigidBodyDragPlane::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehiclePart::Init( part );

	parent = _parent;

	coefficient = part.data.GetFloat( "coefficient" );
	maxForce = part.data.GetFloat( "max_force" );
	minForce = part.data.GetFloat( "min_force" );
	origin = part.data.GetVector( "origin" );
	normal = part.data.GetVector( "normal" );
	normal.Normalize();
	doubleSided = part.data.GetBool( "double_sided" );
	useAngleScale = part.data.GetBool( "use_angle_scale" );
}

/*
================
sdVehicleRigidBodyDragPlane::UpdatePrePhysics
================
*/
void sdVehicleRigidBodyDragPlane::UpdatePrePhysics( const sdVehicleInput& input ) {
	idPhysics* physics = parent->GetPhysics();

	idVec3 parentOrigin	= physics->GetOrigin();
	idVec3 velocity		= physics->GetLinearVelocity();
	idMat3 axis			= physics->GetAxis();

	idVec3 worldNormal	= normal * axis;
	idVec3 worldOrigin	= parentOrigin + ( origin * axis );
	idVec3 dragForce	= vec3_zero;

	// calculate the component of the velocity in the normal direction
	float normalVel = velocity * worldNormal;

	// only velocity INTO the surface is considered relevant
	if ( !doubleSided && normalVel < 0.0f ) {
		return;
	}

	float angleScale = 1.0f;
	if ( useAngleScale ) {
		// simulate the plane only being in contact for some of the time (ie, boat rising out of the water)
		idAngles angles = axis.ToAngles();
		angleScale = 7.5f - fabs( angles.pitch );
		angleScale = idMath::ClampFloat( 0.0f, 7.5f, angleScale ) / 7.5f;

		const idVec3& gravity = physics->GetGravityNormal();
		if ( axis[ 2 ] * gravity < -0.7f ) {
			idVec3 temp = velocity - ( ( velocity * gravity ) * gravity );
			if ( temp.LengthSqr() > Square( 10.f ) ) {
				// do a HORRIBLE HACK to keep the rear end of the boat out of the water
				const idBounds& bounds = parent->GetPhysics()->GetBounds();
				float rearDist = bounds[ 0 ].x;

				idVec3 rear( rearDist, 0.0f, 24.0f );
				idVec3 worldRear = parentOrigin + rear * axis;

				// check if it is in the water
				int cont = gameLocal.clip.Contents( CLIP_DEBUG_PARMS worldRear, NULL, mat3_identity, CONTENTS_WATER, parent );
				
				if ( cont ) {
					// find the top of the water

					// TWTODO (Post-E3): Calculate this without using a trace!
					trace_t trace;
					idVec3 traceStart = worldRear;
					traceStart.z = parentOrigin.z + 24.0f;
					gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, traceStart, worldRear, CONTENTS_WATER, parent );
					idVec3 waterSurfacePoint = trace.endpos;
					// End TWTODO

					//gameRenderWorld->DebugArrow( colorRed, worldRear, waterSurfacePoint, 2.0f );
					float dropDist = trace.endpos.z - worldRear.z;

					// figure out what velocity it needs to push itself out of the water enough
					float timeStep = MS2SEC( gameLocal.msec );
					float pushOutVel = 0.5f * dropDist / timeStep;
					if ( pushOutVel > velocity.z ) {
						// ok so its not scaling it by the force, but it just wants a slight nudge up
						float accelToPushOut = 0.5f * ( pushOutVel - velocity.z ) / timeStep;
						parent->GetPhysics()->AddForce( 0, worldRear, idVec3( 0.0f, 0.0f, accelToPushOut ) );
					}
				}
			}
		}
	}

	// calculate the amount of drag caused by this
	float drag = coefficient * normalVel * normalVel;
	if ( drag > maxForce ) {
		drag = maxForce;
	} else if ( drag <= minForce ) {
		return;
	}

	// calculate the drag force
	dragForce = -drag * angleScale * worldNormal;

	// apply the drag force
	parent->GetPhysics()->AddForce( 0, worldOrigin, dragForce );
	
//	gameRenderWorld->DebugLine( colorGreen, worldOrigin, worldOrigin + dragForce * 0.0001f );
}

/*
================
sdVehicleRigidBodyDragPlane::UpdatePostPhysics
================
*/
void sdVehicleRigidBodyDragPlane::UpdatePostPhysics( const sdVehicleInput& input ) {
}


/*
===============================================================================

	sdVehicleRigidBodyRudder

===============================================================================
*/
CLASS_DECLARATION( sdVehicleRigidBodyDragPlane, sdVehicleRigidBodyRudder )
END_CLASS

/*
================
sdVehicleRigidBodyRudder::Init
================
*/
void sdVehicleRigidBodyRudder::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehicleRigidBodyDragPlane::Init( part, _parent );
}

/*
================
sdVehicleRigidBodyRudder::UpdatePrePhysics
================
*/
void sdVehicleRigidBodyRudder::UpdatePrePhysics( const sdVehicleInput& input ) {
	float oldCoefficient = coefficient;
	idVec3 oldOrigin = origin;

	float right = input.GetRight();
	if ( right != 0.f ) {
		origin.y += right * -600.0f;
		coefficient *= fabs( right );
		normal.x = 1.0f;
		normal.y = 0.0f;
		normal.z = 0.0f;

		sdVehicleRigidBodyDragPlane::UpdatePrePhysics( input );
	}

	coefficient = oldCoefficient;
	origin = oldOrigin;
}

/*
===============================================================================

	sdVehicleRigidBodyHurtZone

===============================================================================
*/
idCVar g_debugVehicleHurtZones( "g_debugVehicleHurtZones", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "show info about the hurtZone component" );

CLASS_DECLARATION( sdVehiclePart, sdVehicleRigidBodyHurtZone )
END_CLASS

/*
================
sdVehicleRigidBodyHurtZone::Init
================
*/
void sdVehicleRigidBodyHurtZone::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehicleRigidBodyPart::Init( part, _parent );

	maxHealth = health = -1.0f;

	sdPhysics_RigidBodyMultiple& rigidBody =  *_parent->GetRBPhysics();
	rigidBody.SetContactFriction( bodyId, vec3_origin );
	rigidBody.SetMass( 1.0f, bodyId );
	rigidBody.SetBodyBuoyancy( bodyId, 0.0f );
	rigidBody.SetClipMask( MASK_HURTZONE, bodyId );
	rigidBody.SetContents( MASK_HURTZONE, bodyId );
}


/*
===============================================================================

	sdVehicleAntiRoll

===============================================================================
*/
CLASS_DECLARATION( sdVehiclePart, sdVehicleAntiRoll )
END_CLASS

/*
================
sdVehicleAntiRoll::sdVehicleAntiRoll
================
*/
sdVehicleAntiRoll::sdVehicleAntiRoll( void ) {
}

/*
================
sdVehicleAntiRoll::~sdVehicleAntiRoll
================
*/
sdVehicleAntiRoll::~sdVehicleAntiRoll( void ) {
}

/*
================
sdVehicleAntiRoll::Init
================
*/
void sdVehicleAntiRoll::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehiclePart::Init( part );

	parent = _parent;

	currentStrength		= 0.0f;
	active				= false;
	startAngle			= part.data.GetFloat( "angle_start", "15" );
	endAngle			= part.data.GetFloat( "angle_end", "45" );
	strength			= part.data.GetFloat( "strength", "1" );
	needsGround			= part.data.GetBool( "needs_ground", "1" );

	if ( endAngle <= startAngle + 0.1f ) {
		endAngle = startAngle + 0.1f;
	}
}

/*
================
sdVehicleAntiRoll::UpdatePrePhysics
================
*/
void sdVehicleAntiRoll::UpdatePrePhysics( const sdVehicleInput& input ) {
	assert( parent != NULL );
	active = false;

	if ( parent->GetPositionManager().FindDriver() == NULL ) {
		return;
	}

	// use the ground contacts & water level to figure out if we should apply
	// cos flipping whilst in the air is kinda cool! ;)
	sdPhysics_RigidBodyMultiple* parentPhysics = parent->GetRBPhysics();
	if ( !needsGround || parentPhysics->HasGroundContacts() || parentPhysics->InWater() ) {
		idAngles myAngles = parentPhysics->GetAxis().ToAngles();
		myAngles.Normalize180();
		idAngles levelAngles( myAngles.pitch, myAngles.yaw, 0.0f );
		idMat3 levelAxis = levelAngles.ToMat3();

		// find the angle from the vertical
		idVec3 currentUp = parentPhysics->GetAxis()[ 2 ];
		float angleBetween = idMath::Fabs( idMath::ACos( levelAxis[ 2 ] * currentUp ) ) * idMath::M_RAD2DEG;

		if ( angleBetween < startAngle ) {
			return;
		}
		active = true;

		if ( angleBetween > endAngle ) {
			angleBetween = endAngle;
		}

		currentStrength = strength * ( angleBetween - startAngle ) / ( endAngle - startAngle );
	}
}

/*
================
sdVehicleAntiRoll::UpdatePostPhysics
================
*/
void sdVehicleAntiRoll::UpdatePostPhysics( const sdVehicleInput& input ) {
}

/*
================
sdVehicleAntiRoll::AddCustomConstraints
================
*/
int sdVehicleAntiRoll::AddCustomConstraints( constraintInfo_t* list, int max ) {
	if ( !active ) {
		return 0;
	}
	
	//
	// LIMIT ROLLING
	//  unidirectional rotation constraint away from current direction we exceed allowed in
	//
	idPhysics* parentPhysics = parent->GetPhysics();
	const idMat3& axis = parentPhysics->GetAxis();
	idVec3 comWorld = parentPhysics->GetCenterOfMass() * axis + parentPhysics->GetOrigin();

	idAngles angles = axis.ToAngles();
	angles.Normalize180();

	// figure out the velocity needed to get back to the allowed roll range
	idAngles clampedAngles = angles;
	clampedAngles.roll = idMath::ClampFloat( -endAngle, endAngle, clampedAngles.roll );

	float angVelDamp = 0.05f;
	idAngles diffAngles = clampedAngles - angles;
	diffAngles.Normalize180();

	// clamp the diff angles so that it doesn't try to achieve something totally insane
	diffAngles.roll = idMath::ClampFloat( -5.0f, 5.0f, diffAngles.roll );

	
	float rollVelocity = parentPhysics->GetAngularVelocity() * axis[ 0 ];
	float neededVelocity = DEG2RAD( diffAngles.roll / MS2SEC( gameLocal.msec ) );
	float targetRollVel = neededVelocity - rollVelocity * angVelDamp;

	// figure out angular impulse needed
	float rollImpulse = currentStrength * ( targetRollVel - rollVelocity ) / MS2SEC( gameLocal.msec );
	rollImpulse *= ( parentPhysics->GetInertiaTensor() * axis[ 0 ] ) * axis[ 0 ];

	if ( idMath::Fabs( rollImpulse ) < idMath::FLT_EPSILON ) {
		return 0;
	}

	// set up the constraint
	constraintInfo_t& wx = list[ 0 ];

	wx.j.SubVec3( 0 ).Zero();
	wx.j.SubVec3( 1 ) = parentPhysics->GetAxis()[ 0 ];
	wx.boxIndex = -1;
	wx.error = CONTACT_LCP_EPSILON;
	wx.pos = comWorld;
	wx.lm = 0.0f;

	wx.c = -targetRollVel;
	
	if ( rollImpulse < 0.0f ) {
		wx.lo = rollImpulse;
		wx.hi = 0.0f;
	} else {
		wx.hi = rollImpulse;
		wx.lo = 0.0f;
	}

	return 1;
}



/*
===============================================================================

	sdVehicleAntiPitch

===============================================================================
*/
CLASS_DECLARATION( sdVehiclePart, sdVehicleAntiPitch )
END_CLASS

/*
================
sdVehicleAntiPitch::sdVehicleAntiPitch
================
*/
sdVehicleAntiPitch::sdVehicleAntiPitch( void ) {
}

/*
================
sdVehicleAntiPitch::~sdVehicleAntiPitch
================
*/
sdVehicleAntiPitch::~sdVehicleAntiPitch( void ) {
}

/*
================
sdVehicleAntiPitch::Init
================
*/
void sdVehicleAntiPitch::Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent ) {
	sdVehiclePart::Init( part );

	parent = _parent;

	currentStrength		= 0.0f;
	active				= false;
	startAngle			= part.data.GetFloat( "angle_start", "15" );
	endAngle			= part.data.GetFloat( "angle_end", "45" );
	strength			= part.data.GetFloat( "strength", "1" );
	needsGround			= part.data.GetBool( "needs_ground", "1" );

	if ( endAngle <= startAngle + 0.1f ) {
		endAngle = startAngle + 0.1f;
	}
}

/*
================
sdVehicleAntiPitch::UpdatePrePhysics
================
*/
void sdVehicleAntiPitch::UpdatePrePhysics( const sdVehicleInput& input ) {
	assert( parent != NULL );
	active = false;

	if ( parent->GetPositionManager().FindDriver() == NULL ) {
		return;
	}

	// use the ground contacts & water level to figure out if we should apply
	// cos flipping whilst in the air is kinda cool! ;)
	sdPhysics_RigidBodyMultiple* parentPhysics = parent->GetRBPhysics();
	if ( !needsGround || parentPhysics->HasGroundContacts() || parentPhysics->InWater() ) {
		idAngles myAngles = parentPhysics->GetAxis().ToAngles();
		myAngles.Normalize180();
		idAngles levelAngles( 0.0f, myAngles.yaw, myAngles.roll );
		idMat3 levelAxis = levelAngles.ToMat3();

		// find the angle from the vertical
		idVec3 currentUp = parentPhysics->GetAxis()[ 2 ];
		float angleBetween = idMath::Fabs( idMath::ACos( levelAxis[ 2 ] * currentUp ) ) * idMath::M_RAD2DEG;

		if ( angleBetween < startAngle ) {
			return;
		}
		active = true;

		if ( angleBetween > endAngle ) {
			angleBetween = endAngle;
		}

		currentStrength = strength * ( angleBetween - startAngle ) / ( endAngle - startAngle );
	}
}

/*
================
sdVehicleAntiPitch::UpdatePostPhysics
================
*/
void sdVehicleAntiPitch::UpdatePostPhysics( const sdVehicleInput& input ) {
}

/*
================
sdVehicleAntiPitch::AddCustomConstraints
================
*/
int sdVehicleAntiPitch::AddCustomConstraints( constraintInfo_t* list, int max ) {
	if ( !active ) {
		return 0;
	}
	
	//
	// LIMIT PITCHING
	//  unidirectional rotation constraint away from current direction we exceed allowed in
	//
	idPhysics* parentPhysics = parent->GetPhysics();
	const idMat3& axis = parentPhysics->GetAxis();
	idVec3 comWorld = parentPhysics->GetCenterOfMass() * axis + parentPhysics->GetOrigin();

	idAngles angles = axis.ToAngles();
	angles.Normalize180();

	// figure out the velocity needed to get back to the allowed roll range
	idAngles clampedAngles = angles;
	clampedAngles.pitch = idMath::ClampFloat( -endAngle, endAngle, clampedAngles.pitch );

	float angVelDamp = 0.05f;
	idAngles diffAngles = clampedAngles - angles;
	diffAngles.Normalize180();

	// clamp the diff angles so that it doesn't try to achieve something totally insane
	diffAngles.pitch = idMath::ClampFloat( -5.0f, 5.0f, diffAngles.pitch );

	
	float pitchVelocity = parentPhysics->GetAngularVelocity() * axis[ 1 ];
	float neededVelocity = DEG2RAD( diffAngles.pitch / MS2SEC( gameLocal.msec ) );
	float targetPitchVel = neededVelocity - pitchVelocity * angVelDamp;

	// figure out angular impulse needed
	float pitchImpulse = currentStrength * ( targetPitchVel - pitchVelocity ) / MS2SEC( gameLocal.msec );
	pitchImpulse *= ( parentPhysics->GetInertiaTensor() * axis[ 1 ] ) * axis[ 1 ];

	if ( idMath::Fabs( pitchImpulse ) < idMath::FLT_EPSILON ) {
		return 0;
	}

	// set up the constraint
	constraintInfo_t& wx = list[ 0 ];

	wx.j.SubVec3( 0 ).Zero();
	wx.j.SubVec3( 1 ) = parentPhysics->GetAxis()[ 1 ];
	wx.boxIndex = -1;
	wx.error = CONTACT_LCP_EPSILON;
	wx.pos = comWorld;
	wx.lm = 0.0f;

	wx.c = -targetPitchVel;
	
	if ( pitchImpulse < 0.0f ) {
		wx.lo = pitchImpulse;
		wx.hi = 0.0f;
	} else {
		wx.hi = pitchImpulse;
		wx.lo = 0.0f;
	}

	return 1;
}
