// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/*
===============================================================================

  idMoveable
	
===============================================================================
*/

const idEventDef EV_BecomeNonSolid( "becomeNonSolid" );
const idEventDef EV_SetOwnerFromSpawnArgs( "<setOwnerFromSpawnArgs>" );
const idEventDef EV_IsAtRest( "isAtRest", NULL, 'd' );
const idEventDef EV_EnableDamage( "enableDamage", "f" );

CLASS_DECLARATION( idEntity, idMoveable )
	EVENT( EV_Activate,					idMoveable::Event_Activate )
	EVENT( EV_BecomeNonSolid,			idMoveable::Event_BecomeNonSolid )
	EVENT( EV_SetOwnerFromSpawnArgs,	idMoveable::Event_SetOwnerFromSpawnArgs )
	EVENT( EV_IsAtRest,					idMoveable::Event_IsAtRest )
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
	initialSpline		= NULL;
	initialSplineDir	= vec3_zero;
//	explode				= false;		// HUMANHEAD pdm: Removed expode support, use health>0 to make it get removed
	unbindOnDeath		= false;
	allowStep			= false;
	canDamage			= false;
#ifdef HUMANHEAD //jsh
	fl.networkSync		= true;
#endif
}

/*
================
idMoveable::~idMoveable
================
*/
idMoveable::~idMoveable( void ) {
	delete initialSpline;
	initialSpline = NULL;
}

/*
================
idMoveable::Spawn
================
*/
void idMoveable::Spawn( void ) {
	idTraceModel trm;
	float density, friction, bouncyness, mass;
	int clipShrink;
	idStr clipModelName;

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if ( !clipModelName[0] ) {
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	if ( !collisionModelManager->TrmFromModel( clipModelName, trm ) ) {
		gameLocal.Error( "idMoveable '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
		return;
	}

	// if the model should be shrinked
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
//	explode = spawnArgs.GetBool( "explode" );		// HUMANHEAD pdm: Removed expode support, use health>0 to make it get removed
	unbindOnDeath = spawnArgs.GetBool( "unbindondeath" );

	fxCollide = spawnArgs.GetString( "fx_collide" );
	nextCollideFxTime = 0;

	fl.takedamage = true;
	damage = spawnArgs.GetString( "def_damage", "" );
	canDamage = spawnArgs.GetBool( "damageWhenActive" ) ? false : true;
	minDamageVelocity = spawnArgs.GetFloat( "minDamageVelocity", "100" );
	maxDamageVelocity = spawnArgs.GetFloat( "maxDamageVelocity", "200" );
	nextDamageTime = 0;
	nextSoundTime = 0;

	health = spawnArgs.GetInt( "health", "0" );
// HUMANHEAD pdm: unused
//	spawnArgs.GetString( "broken", "", brokenModel );

// HUMANHEAD pdm: unused
//	if ( health ) {
//		if ( brokenModel != "" && !renderModelManager->CheckModel( brokenModel ) ) {
//			gameLocal.Error( "idMoveable '%s' at (%s): cannot load broken model '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), brokenModel.c_str() );
//		}
//	}

	// setup the physics
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( trm ), density );
	physicsObj.GetClipModel()->SetMaterial( GetRenderModelMaterial() );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction( 0.6f, 0.6f, friction );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	SetPhysics( &physicsObj );

	if ( spawnArgs.GetFloat( "mass", "10", mass ) ) {
		physicsObj.SetMass( mass );
	}

	if ( spawnArgs.GetBool( "nodrop" ) ) {
		physicsObj.PutToRest();
	} else {
		physicsObj.DropToFloor();
	}

	if ( spawnArgs.GetBool( "noimpact" ) || spawnArgs.GetBool( "notPushable" ) ) {
		physicsObj.DisableImpact();
	}

	if ( spawnArgs.GetBool( "nonsolid" ) ) {
		BecomeNonSolid();
	}

	allowStep = spawnArgs.GetBool( "allowStep", "1" );

	PostEventMS( &EV_SetOwnerFromSpawnArgs, 0 );
}

/*
================
idMoveable::Save
================
*/
void idMoveable::Save( idSaveGame *savefile ) const {

//	savefile->WriteString( brokenModel );		// HUMANHEAD pdm: unused
	savefile->WriteString( damage );
	savefile->WriteString( fxCollide );
	savefile->WriteInt( nextCollideFxTime );
	savefile->WriteFloat( minDamageVelocity );
	savefile->WriteFloat( maxDamageVelocity );
//	savefile->WriteBool( explode );				// HUMANHEAD pdm: Removed expode support, use health>0 to make it get removed
	savefile->WriteBool( unbindOnDeath );
	savefile->WriteBool( allowStep );
	savefile->WriteBool( canDamage );
	savefile->WriteInt( nextDamageTime );
	savefile->WriteInt( nextSoundTime );
	savefile->WriteInt( initialSpline != NULL ? initialSpline->GetTime( 0 ) : -1 );
	savefile->WriteVec3( initialSplineDir );

	savefile->WriteStaticObject( physicsObj );
}

/*
================
idMoveable::Restore
================
*/
void idMoveable::Restore( idRestoreGame *savefile ) {
	int initialSplineTime;

//	savefile->ReadString( brokenModel );		// HUMANHEAD pdm: unused
	savefile->ReadString( damage );
	savefile->ReadString( fxCollide );
	savefile->ReadInt( nextCollideFxTime );
	savefile->ReadFloat( minDamageVelocity );
	savefile->ReadFloat( maxDamageVelocity );
//	savefile->ReadBool( explode );				// HUMANHEAD pdm: Removed expode support, use health>0 to make it get removed
	savefile->ReadBool( unbindOnDeath );
	savefile->ReadBool( allowStep );
	savefile->ReadBool( canDamage );
	savefile->ReadInt( nextDamageTime );
	savefile->ReadInt( nextSoundTime );
	savefile->ReadInt( initialSplineTime );
	savefile->ReadVec3( initialSplineDir );

	if ( initialSplineTime != -1 ) {
		InitInitialSpline( initialSplineTime );
	} else {
		initialSpline = NULL;
	}

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );
}

/*
================
idMoveable::Hide
================
*/
void idMoveable::Hide( void ) {
	idEntity::Hide();
	physicsObj.SetContents( 0 );
}

/*
================
idMoveable::Show
================
*/
void idMoveable::Show( void ) {
	idEntity::Show();
	if ( !spawnArgs.GetBool( "nonsolid" ) ) {
		physicsObj.SetContents( CONTENTS_SOLID );
	}
}

/*
=================
idMoveable::Collide
=================
*/
bool idMoveable::Collide( const trace_t &collision, const idVec3 &velocity ) {
	float v, f;
	idVec3 dir;
	idEntity *ent;

	v = -( velocity * collision.c.normal );
	if ( v > BOUNCE_SOUND_MIN_VELOCITY && gameLocal.time > nextSoundTime ) {
		f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
		if ( StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, false, NULL ) ) {
			// don't set the volume unless there is a bounce sound as it overrides the entire channel
			// which causes footsteps on ai's to not honor their shader parms
			SetSoundVolume( f );
		}
		nextSoundTime = gameLocal.time + 500;
	}

	if ( canDamage && damage.Length() && gameLocal.time > nextDamageTime ) {
		ent = gameLocal.entities[ collision.c.entityNum ];
		if ( ent && v > minDamageVelocity ) {
			f = v > maxDamageVelocity ? 1.0f : idMath::Sqrt( v - minDamageVelocity ) * ( 1.0f / idMath::Sqrt( maxDamageVelocity - minDamageVelocity ) );
			dir = velocity;
			dir.NormalizeFast();
			ent->Damage( this, GetPhysics()->GetClipModel()->GetOwner(), dir, damage, f, INVALID_JOINT );
			nextDamageTime = gameLocal.time + 1000;
		}
	}

	if ( fxCollide.Length() && gameLocal.time > nextCollideFxTime ) {
		idEntityFx::StartFx( fxCollide, &collision.c.point, NULL, this, false );
		nextCollideFxTime = gameLocal.time + 3500;
	}

	return false;
}

/*
============
idMoveable::Killed
============
*/
void idMoveable::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
/*	HUMANHEAD pdm: overridden
	if ( unbindOnDeath ) {
		Unbind();
	}

	if ( brokenModel != "" ) {
		SetModel( brokenModel );
	}

	if ( explode ) {
		if ( brokenModel == "" ) {
			PostEventMS( &EV_Remove, 1000 );
		}
	}

	if ( renderEntity.gui[ 0 ] ) {
		renderEntity.gui[ 0 ] = NULL;
	}

	ActivateTargets( this );

	fl.takedamage = false;
*/
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
	physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_RENDERMODEL );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
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
idMoveable::InitInitialSpline
================
*/
void idMoveable::InitInitialSpline( int startTime ) {
	int initialSplineTime;

	initialSpline = GetSpline();
	initialSplineTime = spawnArgs.GetInt( "initialSplineTime", "300" );

	if ( initialSpline != NULL ) {
		initialSpline->MakeUniform( initialSplineTime );
		initialSpline->ShiftTime( startTime - initialSpline->GetTime( 0 ) );
		initialSplineDir = initialSpline->GetCurrentFirstDerivative( startTime );
		initialSplineDir *= physicsObj.GetAxis().Transpose();
		initialSplineDir.Normalize();
		BecomeActive( TH_THINK );
	}
}

/*
================
idMoveable::FollowInitialSplinePath
================
*/
bool idMoveable::FollowInitialSplinePath( void ) {
	if ( initialSpline != NULL ) {
		if ( gameLocal.time < initialSpline->GetTime( initialSpline->GetNumValues() - 1 ) ) {
			idVec3 splinePos = initialSpline->GetCurrentValue( gameLocal.time );
			idVec3 linearVelocity = ( splinePos - physicsObj.GetOrigin() ) * USERCMD_HZ;
			physicsObj.SetLinearVelocity( linearVelocity );

			idVec3 splineDir = initialSpline->GetCurrentFirstDerivative( gameLocal.time );
			idVec3 dir = initialSplineDir * physicsObj.GetAxis();
			idVec3 angularVelocity = dir.Cross( splineDir );
			angularVelocity.Normalize();
			angularVelocity *= idMath::ACos16( dir * splineDir / splineDir.Length() ) * USERCMD_HZ;
			physicsObj.SetAngularVelocity( angularVelocity );
			return true;
		} else {
			delete initialSpline;
			initialSpline = NULL;
		}
	}
	return false;
}

/*
================
idMoveable::Think
================
*/
void idMoveable::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		if ( !FollowInitialSplinePath() ) {
			BecomeInactive( TH_THINK );
		}
	}
	idEntity::Think();
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
		 return renderEntity.hModel->Surface( 0 )->shader;
	}
	return NULL;
}

/*
================
idMoveable::WriteToSnapshot
================
*/
void idMoveable::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
}

/*
================
idMoveable::ReadFromSnapshot
================
*/
void idMoveable::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
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

	InitInitialSpline( gameLocal.time );
}

/*
================
idMoveable::Event_SetOwnerFromSpawnArgs
================
*/
void idMoveable::Event_SetOwnerFromSpawnArgs( void ) {
	idStr owner;

	if ( spawnArgs.GetString( "owner", "", owner ) ) {
		ProcessEvent( &EV_SetOwner, gameLocal.FindEntity( owner ) );
	}
}

/*
================
idMoveable::Event_IsAtRest
================
*/
void idMoveable::Event_IsAtRest( void ) {
	idThread::ReturnInt( physicsObj.IsAtRest() );
}

/*
================
idMoveable::Event_EnableDamage
================
*/
void idMoveable::Event_EnableDamage( float enable ) {
	canDamage = ( enable != 0.0f );
}


/*
===============================================================================

  idBarrel

  HUMANHEAD pdm: removed, re-inherited from hhMoveable
	
===============================================================================
*/

/*
===============================================================================

idExplodingBarrel
  HUMANHEAD pdm: removed, unused

===============================================================================
*/

