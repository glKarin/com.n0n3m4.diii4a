// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Moveable.h"
#include "ContentMask.h"
#include "Projectile.h"
#include "Player.h"


/*
================
sdMoveableNetworkData::~sdMoveableNetworkData
================
*/
sdMoveableNetworkData::~sdMoveableNetworkData( void ) {
	delete physicsData;
}

/*
================
sdMoveableNetworkData::MakeDefault
================
*/
void sdMoveableNetworkData::MakeDefault( void ) {
	if ( physicsData ) {
		physicsData->MakeDefault();
	}
}

/*
================
sdMoveableNetworkData::Write
================
*/
void sdMoveableNetworkData::Write( idFile* file ) const {
	if ( physicsData ) {
		physicsData->Write( file );
	}
}

/*
================
sdMoveableNetworkData::Read
================
*/
void sdMoveableNetworkData::Read( idFile* file ) {
	if ( physicsData ) {
		physicsData->Read( file );
	}
}

/*
================
sdMoveableBroadcastData::~sdMoveableBroadcastData
================
*/
sdMoveableBroadcastData::~sdMoveableBroadcastData( void ) {
	delete physicsData;
}

/*
================
sdMoveableBroadcastData::MakeDefault
================
*/
void sdMoveableBroadcastData::MakeDefault( void ) {
	if ( physicsData ) {
		physicsData->MakeDefault();
	}
	hidden = false;
}

/*
================
sdMoveableBroadcastData::Write
================
*/
void sdMoveableBroadcastData::Write( idFile* file ) const {
	if ( physicsData ) {
		physicsData->Write( file );
	}

	file->WriteBool( hidden );
}

/*
================
sdMoveableBroadcastData::Read
================
*/
void sdMoveableBroadcastData::Read( idFile* file ) {
	if ( physicsData ) {
		physicsData->Read( file );
	}

	file->ReadBool( hidden );
}

/*
===============================================================================

  idMoveable
	
===============================================================================
*/

const idEventDef EV_BecomeNonSolid( "becomeNonSolid", '\0', DOC_TEXT( "Makes the object non-solid to players/vehicles, bullets will still collide however." ), 0, NULL );
const idEventDef EV_EnableDamage( "enableDamage", '\0', DOC_TEXT( "Enables/disables doing damage to objects it hits." ), 1, NULL, "b", "value", "Whether to enable/disable." );

CLASS_DECLARATION( idEntity, idMoveable )
	EVENT( EV_Activate,					idMoveable::Event_Activate )
	EVENT( EV_BecomeNonSolid,			idMoveable::Event_BecomeNonSolid )
	EVENT( EV_EnableDamage,				idMoveable::Event_EnableDamage )
END_CLASS


static const float BOUNCE_SOUND_MIN_VELOCITY	= 80.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;

/*
================
idMoveable::idMoveable
================
*/
idMoveable::idMoveable( void ) {
	minDamageVelocity	= 100.0f;
	maxDamageVelocity	= 200.0f;
	nextCollideFxTime	= 0;
	nextDamageTime		= 0;
	nextSoundTime		= 0;
	explode				= false;
	unbindOnDeath		= false;
	allowStep			= false;
	canDamage			= false;
	waterEffects		= NULL;
}

/*
================
idMoveable::~idMoveable
================
*/
idMoveable::~idMoveable( void ) {
	delete waterEffects;
}

/*
================
idMoveable::Spawn
================
*/
void idMoveable::Spawn( void ) {
	idTraceModel trm;
	float density, friction, bouncyness, linearFriction, angularFriction;
	int clipShrink;
	
	
	const char* clipModelName = GetClipModelName();
	if ( !gameLocal.clip.LoadTraceModel( clipModelName, trm ) ) {
		gameLocal.Error( "idMoveable '%s': cannot load collision model %s", name.c_str(), clipModelName );
		return;
	}

	// if the model should be shrunk
	clipShrink = spawnArgs.GetInt( "clipshrink" );
	if ( clipShrink != 0 ) {
		trm.Shrink( clipShrink * CM_CLIP_EPSILON );
	}

	// get rigid body properties
	spawnArgs.GetFloat( "density", "0.5", density );
	density = idMath::ClampFloat( 0.001f, 1000.0f, density );
	spawnArgs.GetFloat( "friction", "0.05", friction );
	friction = idMath::ClampFloat( 0.0f, 1.0f, friction );
	spawnArgs.GetFloat( "bouncyness", "0.6", bouncyness );
	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );
	spawnArgs.GetFloat( "linear_friction", "0.6", linearFriction );
	linearFriction = idMath::ClampFloat( 0.0f, 1.0f, linearFriction );
	spawnArgs.GetFloat( "angular_friction", "0.6", angularFriction );
	angularFriction = idMath::ClampFloat( 0.0f, 1.0f, angularFriction );
	explode = spawnArgs.GetBool( "explode" );
	unbindOnDeath = spawnArgs.GetBool( "unbindondeath" );

	nextCollideFxTime = 0;

	fl.takedamage = true;
	damageDecl = DAMAGE_FOR_NAME_UNSAFE( spawnArgs.GetString( "dmg_damage", "" ) );
	canDamage = spawnArgs.GetBool( "damageWhenActive" ) ? false : true;
	minDamageVelocity = spawnArgs.GetFloat( "minDamageVelocity", "100" );
	maxDamageVelocity = spawnArgs.GetFloat( "maxDamageVelocity", "200" );
	nextDamageTime = 0;
	nextSoundTime = 0;

	// setup the physics
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( trm, false ), density, 0 );
	physicsObj.GetClipModel( 0 )->SetMaterial( GetRenderModelMaterial() );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction( linearFriction, angularFriction, friction );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_SOLID, 0 );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_SLIDEMOVER | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP | CONTENTS_FORCEFIELD, 0 );
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "10" ), 0 );
	physicsObj.SetBuoyancy( spawnArgs.GetFloat( "buoyancy", "10" ) );
	SetPhysics( &physicsObj );

	if ( spawnArgs.GetBool( "noimpact" ) || spawnArgs.GetBool( "notPushable" ) ) {
		physicsObj.DisableImpact();
	}

	if ( spawnArgs.GetBool( "nonsolid" ) ) {
		BecomeNonSolid();
	}

	allowStep = spawnArgs.GetBool( "allowStep", "1" );

	waterEffects = sdWaterEffects::SetupFromSpawnArgs( spawnArgs );
}


/*
================
idMoveable::Hide
================
*/
void idMoveable::Hide( void ) {
	idEntity::Hide();
	physicsObj.SetContents( 0, 0 );
}

/*
================
idMoveable::Show
================
*/
void idMoveable::Show( void ) {
	idEntity::Show();
	if ( !spawnArgs.GetBool( "nonsolid" ) ) {
		physicsObj.SetContents( CONTENTS_SOLID, 0 );
	}
}

/*
=================
idMoveable::Collide
=================
*/
bool idMoveable::Collide( const trace_t &collision, const idVec3 &velocity, int bodyId ) {
	float v, f;
	idVec3 dir;
	idEntity *ent;

	v = -( velocity * collision.c.normal );
	if ( v > BOUNCE_SOUND_MIN_VELOCITY && gameLocal.time > nextSoundTime ) {
		f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
		if ( StartSound( "snd_bounce", SND_ANY, 0, NULL ) ) {
			SetSoundVolume( f );
		}
		nextSoundTime = gameLocal.time + 500;
	}

	if ( canDamage && damageDecl && gameLocal.time > nextDamageTime ) {
		ent = gameLocal.entities[ collision.c.entityNum ];
		if ( ent && v > minDamageVelocity ) {
			f = v > maxDamageVelocity ? 1.0f : idMath::Sqrt( v - minDamageVelocity ) * ( 1.0f / idMath::Sqrt( maxDamageVelocity - minDamageVelocity ) );
			dir = velocity;
			dir.NormalizeFast();
			ent->Damage( this, this, dir, damageDecl, f, NULL );
			nextDamageTime = gameLocal.time + 1000;
		}
	}

	return false;
}

/*
============
idMoveable::Killed
============
*/
void idMoveable::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	if ( unbindOnDeath ) {
		Unbind();
	}

	const char* brokenModel = spawnArgs.GetString( "model_broken" );
	if ( *brokenModel ) {
		SetModel( brokenModel );
	} else {
		if ( explode ) {
			PostEventMS( &EV_Remove, 1000 );
		}
	}

	fl.takedamage = false;
}

/*
================
idMoveable::AllowStep
================
*/
bool idMoveable::AllowStep( void ) const {
	return allowStep;
}

/*
================
idMoveable::BecomeNonSolid
================
*/
void idMoveable::BecomeNonSolid( void ) {
	// set CONTENTS_RENDERMODEL so bullets still collide with the moveable
	physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_RENDERMODEL, 0 );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP | CONTENTS_FORCEFIELD, 0 );
}

/*
================
idMoveable::EnableDamage
================
*/
void idMoveable::EnableDamage( bool enable, float duration ) {
	canDamage = enable;
	if ( duration ) {
		PostEventSec( &EV_EnableDamage, duration, ( !enable ) ? 0.0f : 1.0f );
	}
}

/*
================
idMoveable::GetRenderModelMaterial
================
*/
const idMaterial *idMoveable::GetRenderModelMaterial( void ) const {
	if ( renderEntity.customShader ) {
		return renderEntity.customShader;
	}
	if ( renderEntity.hModel && renderEntity.hModel->NumSurfaces() ) {
		 return renderEntity.hModel->Surface( 0 )->material;
	}
	return NULL;
}

/*
================
idMoveable::Event_BecomeNonSolid
================
*/
void idMoveable::Event_BecomeNonSolid( void ) {
	BecomeNonSolid();
}

/*
================
idMoveable::Event_Activate
================
*/
void idMoveable::Event_Activate( idEntity *activator ) {
	float delay;
	idVec3 init_velocity, init_avelocity;

	Show();

	if ( !spawnArgs.GetInt( "notPushable" ) ) {
        physicsObj.EnableImpact();
	}

	physicsObj.Activate();

	spawnArgs.GetVector( "init_velocity", "0 0 0", init_velocity );
	spawnArgs.GetVector( "init_avelocity", "0 0 0", init_avelocity );

	delay = spawnArgs.GetFloat( "init_velocityDelay", "0" );
	if ( delay == 0.0f ) {
		physicsObj.SetLinearVelocity( init_velocity );
	} else {
		PostEventSec( &EV_SetLinearVelocity, delay, init_velocity );
	}

	delay = spawnArgs.GetFloat( "init_avelocityDelay", "0" );
	if ( delay == 0.0f ) {
		physicsObj.SetAngularVelocity( init_avelocity );
	} else {
		PostEventSec( &EV_SetAngularVelocity, delay, init_avelocity );
	}
}

/*
================
idMoveable::Event_EnableDamage
================
*/
void idMoveable::Event_EnableDamage( bool enable ) {
	canDamage = enable;
}

/*
================
idMoveable::CheckWater
================
*/
void idMoveable::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects ) {
		waterEffects->SetOrigin( GetPhysics()->GetOrigin() );
		waterEffects->SetAxis( GetPhysics()->GetAxis() );
		waterEffects->SetVelocity( GetPhysics()->GetLinearVelocity() );
		waterEffects->CheckWater( this, waterBodyOrg, waterBodyAxis, waterBodyModel );
	}
}

/*
================
idMoveable::CheckNetworkStateChanges
================
*/
bool idMoveable::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdMoveableNetworkData );
		NET_CHECK_STATE_PHYSICS;

		return false;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdMoveableBroadcastData );
		NET_CHECK_STATE_PHYSICS;

		if ( baseData.hidden != fl.hidden ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
idMoveable::WriteNetworkState
================
*/
void idMoveable::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdMoveableNetworkData );
		NET_WRITE_STATE_PHYSICS;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdMoveableBroadcastData );
		NET_WRITE_STATE_PHYSICS;

		newData.hidden = fl.hidden;
		msg.WriteBool( newData.hidden );
	}
}

/*
================
idMoveable::ApplyNetworkState
================
*/
void idMoveable::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdMoveableNetworkData );
		NET_APPLY_STATE_PHYSICS;
	} else if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdMoveableBroadcastData );
		NET_APPLY_STATE_PHYSICS;

		if ( newData.hidden ) {
			Hide();
		} else {
			Show();
		}
	}
}

/*
================
idMoveable::ReadNetworkState
================
*/
void idMoveable::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdMoveableNetworkData );
		NET_READ_STATE_PHYSICS;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdMoveableBroadcastData );
		NET_READ_STATE_PHYSICS;

		newData.hidden = msg.ReadBool();
	}
}

/*
================
idMoveable::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idMoveable::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		sdMoveableNetworkData* newData = new sdMoveableNetworkData();
		
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );

		return newData;
	}

	if ( mode == NSM_BROADCAST ) {
		sdMoveableBroadcastData* newData = new sdMoveableBroadcastData();
		
		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );

		return newData;
	}

	return NULL;
}

/*
===============================================================================

  idBarrel
	
===============================================================================
*/

CLASS_DECLARATION( idMoveable, idBarrel )
END_CLASS

/*
================
idBarrel::idBarrel
================
*/
idBarrel::idBarrel() {
	radius = 1.0f;
	barrelAxis = 0;
	lastOrigin.Zero();
	lastAxis.Identity();
	additionalRotation = 0.0f;
	additionalAxis.Identity();
}

/*
================
idBarrel::BarrelThink
================
*/
void idBarrel::BarrelThink( void ) {
	bool wasAtRest, onGround;
	float movedDistance, rotatedDistance, angle;
	idVec3 curOrigin, gravityNormal, dir;
	idMat3 curAxis, axis;

	wasAtRest = IsAtRest();

	// run physics
	RunPhysics();

	// only need to give the visual model an additional rotation if the physics were run
	if ( !wasAtRest ) {

		// current physics state
		onGround = GetPhysics()->HasGroundContacts();
		curOrigin = GetPhysics()->GetOrigin();
		curAxis = GetPhysics()->GetAxis();

		// if the barrel is on the ground
		if ( onGround ) {
			gravityNormal = GetPhysics()->GetGravityNormal();

			dir = curOrigin - lastOrigin;
			dir -= gravityNormal * dir * gravityNormal;
			movedDistance = dir.LengthSqr();

			// if the barrel moved and the barrel is not aligned with the gravity direction
			if ( movedDistance > 0.0f && idMath::Fabs( gravityNormal * curAxis[barrelAxis] ) < 0.7f ) {

				// barrel movement since last think frame orthogonal to the barrel axis
				movedDistance = idMath::Sqrt( movedDistance );
				dir *= 1.0f / movedDistance;
				movedDistance = ( 1.0f - idMath::Fabs( dir * curAxis[barrelAxis] ) ) * movedDistance;

				// get rotation about barrel axis since last think frame
				angle = lastAxis[(barrelAxis+1)%3] * curAxis[(barrelAxis+1)%3];
				angle = idMath::ACos( angle );
				// distance along cylinder hull
				rotatedDistance = angle * radius;

				// if the barrel moved further than it rotated about it's axis
				if ( movedDistance > rotatedDistance ) {

					// additional rotation of the visual model to make it look
					// like the barrel rolls instead of slides
					angle = 180.0f * (movedDistance - rotatedDistance) / (radius * idMath::PI);
					if ( gravityNormal.Cross( curAxis[barrelAxis] ) * dir < 0.0f ) {
						additionalRotation += angle;
					} else {
						additionalRotation -= angle;
					}
					dir = vec3_origin;
					dir[barrelAxis] = 1.0f;
					additionalAxis = idRotation( vec3_origin, dir, additionalRotation ).ToMat3();
				}
			}
		}

		// save state for next think
		lastOrigin = curOrigin;
		lastAxis = curAxis;
	}

	Present();
}

/*
================
idBarrel::Think
================
*/
void idBarrel::Think( void ) {
	BarrelThink();
}

/*
================
idBarrel::GetPhysicsToVisualTransform
================
*/
bool idBarrel::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin = vec3_origin;
	axis = additionalAxis;
	return true;
}

/*
================
idBarrel::Spawn
================
*/
void idBarrel::Spawn( void ) {
	const idBounds &bounds = GetPhysics()->GetBounds();

	// radius of the barrel cylinder
	radius = ( bounds[1][0] - bounds[0][0] ) * 0.5f;

	// always a vertical barrel with cylinder axis parallel to the z-axis
	barrelAxis = 2;

	lastOrigin = GetPhysics()->GetOrigin();
	lastAxis = GetPhysics()->GetAxis();

	additionalRotation = 0.0f;
	additionalAxis.Identity();
}
