/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "renderer/ModelManager.h"

#include "Fx.h"

#include "Game_local.h"
#include "Moveable.h"
#include "Projectile.h"
#include "SmokeParticles.h"

/*
===============================================================================

  idMoveable

===============================================================================
*/

const idEventDef EV_BecomeNonSolid( "becomeNonSolid" );
const idEventDef EV_SetOwnerFromSpawnArgs( "<setOwnerFromSpawnArgs>" );
const idEventDef EV_IsAtRest( "isAtRest", NULL, 'd' );
const idEventDef EV_EnableDamage( "enableDamage", "f" );
// HEXEN : Zeroth
const idEventDef EV_BecomeSolid( "MovableBecomeSolid" );
const idEventDef EV_DealDirectDamage( "directDamage", "es" );

CLASS_DECLARATION( idEntity, idMoveable )
	EVENT( EV_Activate,					idMoveable::Event_Activate )
	EVENT( EV_BecomeSolid,				idMoveable::Event_BecomeSolid )
	EVENT( EV_SetOwnerFromSpawnArgs,	idMoveable::Event_SetOwnerFromSpawnArgs )
	EVENT( EV_IsAtRest,					idMoveable::Event_IsAtRest )
	EVENT( EV_EnableDamage,				idMoveable::Event_EnableDamage )
// HEXEN : Zeroth
	EVENT( EV_BecomeNonSolid,			idMoveable::Event_BecomeNonSolid )
	EVENT( EV_DealDirectDamage,			idMoveable::Event_DirectDamage )
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
	explode				= false;
	unbindOnDeath		= false;
	allowStep			= false;
	canDamage			= false;
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
	savePersistentInfo = false;

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if ( !clipModelName[0] ) {
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	// HEXEN : Zeroth
/**	if ( !clipModelName[0] && IsType( idWood::Type ) ) {
		gameLocal.Error("idWood '%s': Please open map editor and re-save map.", name.c_str() );
	}
**/

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
	explode = spawnArgs.GetBool( "explode" );
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
	spawnArgs.GetString( "broken", "", brokenModel );
	spawnArgs.GetBool( "removeWhenBroken", "", removeWhenBroken );
	spawnArgs.GetBool( "call_script_on_broken", "", brokenScript ); // HEXEN : Zeroth

	if ( health ) {
		if ( brokenModel != "" && !renderModelManager->CheckModel( brokenModel ) ) {
			gameLocal.Error( "idMoveable '%s' at (%s): cannot load broken model '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), brokenModel.c_str() );
		}
	}

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

	if ( spawnArgs.GetBool( "nodrop" ) /**|| IsType( idWood::Type ) **/ ) { // HEXEN : Zeroth, added idWood
		physicsObj.PutToRest();
	} else {
		physicsObj.DropToFloor();
	}

	if ( spawnArgs.GetBool( "noimpact" ) || spawnArgs.GetBool( "notPushable" ) /**|| IsType( idWood::Type )**/ ) { // HEXEN : Zeroth, added idWood
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

	savefile->WriteString( brokenModel );
	savefile->WriteBool( brokenScript );
	savefile->WriteBool( removeWhenBroken );
	savefile->WriteString( damage );
	savefile->WriteString( fxCollide );
	savefile->WriteInt( nextCollideFxTime );
	savefile->WriteFloat( minDamageVelocity );
	savefile->WriteFloat( maxDamageVelocity );
	savefile->WriteBool( explode );
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

	savefile->ReadString( brokenModel );
	savefile->ReadBool( brokenScript );
	savefile->ReadBool( removeWhenBroken );
	savefile->ReadString( damage );
	savefile->ReadString( fxCollide );
	savefile->ReadInt( nextCollideFxTime );
	savefile->ReadFloat( minDamageVelocity );
	savefile->ReadFloat( maxDamageVelocity );
	savefile->ReadBool( explode );
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
	gameLocal.Printf( "restore %s\n", name.c_str() );
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

	savePersistentInfo = true;
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
			ent->Damage( this, GetPhysics()->GetClipModel()->GetOwner(), dir, damage, f, INVALID_JOINT, idVec3( collision.c.point ) );
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

	// HEXEN : Zeroth
	const idSoundShader *shader = declManager->FindSound( spawnArgs.GetString( "snd_break" ) );
	this->StartSoundShader( shader, SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );

	// HEXEN : Zeroth
	const idDeclParticle *	smokeBreak = NULL;
	int						smokeBreakTime = 0;
	const char *smokeName = spawnArgs.GetString( "smoke_break" );
	if ( *smokeName != '\0' ) {
		smokeBreak = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeBreakTime = gameLocal.time;

		// get center of bounds on moveable
		idVec3 newDir;
		newDir = GetPhysics()->GetBounds()[1] + GetPhysics()->GetBounds()[0];
		newDir *= GetPhysics()->GetClipModel()->GetAxis();
		newDir += GetPhysics()->GetOrigin();

		gameLocal.smokeParticles->EmitSmoke( smokeBreak, smokeBreakTime, gameLocal.random.CRandomFloat(), newDir, GetPhysics()->GetAxis() );
	}

	// HEXEN : Zeroth
	if ( removeWhenBroken ) {
		Hide();
		gameLocal.SetPersistentRemove( name.c_str() );

		physicsObj.PutToRest();
		CancelEvents( &EV_Explode );
		CancelEvents( &EV_Activate );

		if ( spawnArgs.GetBool( "triggerTargets" ) ) {
			ActivateTargets( this );
		}

		PostEventMS( &EV_Remove, spawnArgs.GetFloat( "fuse" ) );
	}

	// HEXEN : Zeroth
	if ( scriptThread != NULL && brokenScript ) {
		const function_t *func = GetScriptFunction( "broken" );
		if ( !func ) {
			assert( 0 );
			gameLocal.Error( "Can't find function use' in object '%s'", scriptObject.GetTypeName() );
			return;
		}

		SetState( func );
		UpdateScript();
	}

	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_debris" );
	// bool first = true;
	while ( kv ) {
		const idDict *debris_args = gameLocal.FindEntityDefDict( kv->GetValue(), false );
		if ( debris_args ) {
			idEntity *ent;
			idVec3 dir;
			idDebris *debris;
			// if ( first ) {
				dir = physicsObj.GetAxis()[1];
			//	first = false;
			// } else {
				dir.x += gameLocal.random.CRandomFloat() * 4.0f;
				dir.y += gameLocal.random.CRandomFloat() * 4.0f;
				// dir.z = gameLocal.random.RandomFloat() * 8.0f;
			// }
			dir.Normalize();

			gameLocal.SpawnEntityDef( *debris_args, &ent, false );
			if ( !ent || !ent->IsType( idDebris::Type ) ) {
				gameLocal.Error( "'projectile_debris' is not an idDebris" );
			}

			debris = static_cast<idDebris *>( ent );
			debris->randomPosInBounds = true;
			debris->randomPosEnt = this;
			debris->Create( this, physicsObj.GetOrigin(), dir.ToMat3() );
			debris->Launch();
			debris->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = ( gameLocal.time + 1500 ) * 0.001f;
			debris->UpdateVisuals();
		}
		kv = spawnArgs.MatchPrefix( "def_debris", kv );
	}

	if ( renderEntity.gui[ 0 ] ) {
		renderEntity.gui[ 0 ] = NULL;
	}

	ActivateTargets( this );

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
	physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_RENDERMODEL );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
}

/*
================
Zeroth
idMoveable::BecomeSolid
================
*/
void idMoveable::BecomeSolid( void ) {
	// set CONTENTS_RENDERMODEL so bullets still collide with the moveable
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
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
Zeroth
idMoveable::Event_BecomeSolid
================
*/
void idMoveable::Event_BecomeSolid( void ) {
	BecomeSolid();
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
================
idMoveable::Event_DirectDamage
================
*/
void idMoveable::Event_DirectDamage( idEntity *damageTarget, const char *damageDefName ) {
	DirectDamage( damageDefName, damageTarget );
}

/*
================
idMoveable::DirectDamage
================
*/
void idMoveable::DirectDamage( const char *meleeDefName, idEntity *ent ) {
	const idDict *meleeDef;
	const char *p;
	const idSoundShader *shader;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown damage def '%s' on '%s'", meleeDefName, name.c_str() );
	}

	if ( !ent->fl.takedamage ) {
		const idSoundShader *shader = declManager->FindSound(meleeDef->GetString( "snd_miss" ));
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		return;
	}

	//
	// do the damage
	//
	p = meleeDef->GetString( "snd_hit" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	idVec3	kickDir;
	meleeDef->GetVector( "kickDir", "0 0 0", kickDir );

	idVec3	globalKickDir;
	globalKickDir = kickDir; // z.todo: not proper

	ent->Damage( this, this, globalKickDir, meleeDefName, 1.0f, INVALID_JOINT, idVec3( 0, 0, 0 ) );
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
	fl.networkSync = true;
}

/*
================
idBarrel::Save
================
*/
void idBarrel::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( radius );
	savefile->WriteInt( barrelAxis );
	savefile->WriteVec3( lastOrigin );
	savefile->WriteMat3( lastAxis );
	savefile->WriteFloat( additionalRotation );
	savefile->WriteMat3( additionalAxis );
}

/*
================
idBarrel::Restore
================
*/
void idBarrel::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( radius );
	savefile->ReadInt( barrelAxis );
	savefile->ReadVec3( lastOrigin );
	savefile->ReadMat3( lastAxis );
	savefile->ReadFloat( additionalRotation );
	savefile->ReadMat3( additionalAxis );
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
	if ( thinkFlags & TH_THINK ) {
		if ( !FollowInitialSplinePath() ) {
			BecomeInactive( TH_THINK );
		}
	}

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

/*
================
idBarrel::ClientPredictionThink
================
*/
void idBarrel::ClientPredictionThink( void ) {
	Think();
}


/*
===============================================================================

idExplodingBarrel

===============================================================================
*/
const idEventDef EV_Respawn( "<respawn>" );
const idEventDef EV_TriggerTargets( "<triggertargets>" );

CLASS_DECLARATION( idBarrel, idExplodingBarrel )
	EVENT( EV_Activate,					idExplodingBarrel::Event_Activate )
	EVENT( EV_Respawn,					idExplodingBarrel::Event_Respawn )
	EVENT( EV_Explode,					idExplodingBarrel::Event_Explode )
	EVENT( EV_TriggerTargets,			idExplodingBarrel::Event_TriggerTargets )
END_CLASS

/*
================
idExplodingBarrel::idExplodingBarrel
================
*/
idExplodingBarrel::idExplodingBarrel() {
	spawnOrigin.Zero();
	spawnAxis.Zero();
	state = NORMAL;
	particleModelDefHandle = -1;
	lightDefHandle = -1;
	memset( &particleRenderEntity, 0, sizeof( particleRenderEntity ) );
	memset( &light, 0, sizeof( light ) );
	particleTime = 0;
	lightTime = 0;
	time = 0.0f;
}

/*
================
idExplodingBarrel::~idExplodingBarrel
================
*/
idExplodingBarrel::~idExplodingBarrel() {
	if ( particleModelDefHandle >= 0 ){
		gameRenderWorld->FreeEntityDef( particleModelDefHandle );
	}
	if ( lightDefHandle >= 0 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
	}
}

/*
================
idExplodingBarrel::Save
================
*/
void idExplodingBarrel::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( spawnOrigin );
	savefile->WriteMat3( spawnAxis );

	savefile->WriteInt( state );
	savefile->WriteInt( particleModelDefHandle );
	savefile->WriteInt( lightDefHandle );

	savefile->WriteRenderEntity( particleRenderEntity );
	savefile->WriteRenderLight( light );

	savefile->WriteInt( particleTime );
	savefile->WriteInt( lightTime );
	savefile->WriteFloat( time );
}

/*
================
idExplodingBarrel::Restore
================
*/
void idExplodingBarrel::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( spawnOrigin );
	savefile->ReadMat3( spawnAxis );

	savefile->ReadInt( (int &)state );
	savefile->ReadInt( (int &)particleModelDefHandle );
	savefile->ReadInt( (int &)lightDefHandle );

	savefile->ReadRenderEntity( particleRenderEntity );
	savefile->ReadRenderLight( light );

	savefile->ReadInt( particleTime );
	savefile->ReadInt( lightTime );
	savefile->ReadFloat( time );
}

/*
================
idExplodingBarrel::Spawn
================
*/
void idExplodingBarrel::Spawn( void ) {
	health = spawnArgs.GetInt( "health", "5" );
	fl.takedamage = true;
	spawnOrigin = GetPhysics()->GetOrigin();
	spawnAxis = GetPhysics()->GetAxis();
	state = NORMAL;
	particleModelDefHandle = -1;
	lightDefHandle = -1;
	lightTime = 0;
	particleTime = 0;
	time = spawnArgs.GetFloat( "time" );
	memset( &particleRenderEntity, 0, sizeof( particleRenderEntity ) );
	memset( &light, 0, sizeof( light ) );
}

/*
================
idExplodingBarrel::Think
================
*/
void idExplodingBarrel::Think( void ) {
	idBarrel::BarrelThink();

	if ( lightDefHandle >= 0 ){
		if ( state == BURNING ) {
			// ramp the color up over 250 ms
			float pct = (gameLocal.time - lightTime) / 250.f;
			if ( pct > 1.0f ) {
				pct = 1.0f;
			}
			light.origin = physicsObj.GetAbsBounds().GetCenter();
			light.axis = mat3_identity;
			light.shaderParms[ SHADERPARM_RED ] = pct;
			light.shaderParms[ SHADERPARM_GREEN ] = pct;
			light.shaderParms[ SHADERPARM_BLUE ] = pct;
			light.shaderParms[ SHADERPARM_ALPHA ] = pct;
			gameRenderWorld->UpdateLightDef( lightDefHandle, &light );
		} else {
			if ( gameLocal.time - lightTime > 250 ) {
				gameRenderWorld->FreeLightDef( lightDefHandle );
				lightDefHandle = -1;
			}
			return;
		}
	}

	if ( !gameLocal.isClient && state != BURNING && state != EXPLODING ) {
		BecomeInactive( TH_THINK );
		return;
	}

	if ( particleModelDefHandle >= 0 ){
		particleRenderEntity.origin = physicsObj.GetAbsBounds().GetCenter();
		particleRenderEntity.axis = mat3_identity;
		gameRenderWorld->UpdateEntityDef( particleModelDefHandle, &particleRenderEntity );
	}
}

/*
================
idExplodingBarrel::AddParticles
================
*/
void idExplodingBarrel::AddParticles( const char *name, bool burn ) {
	if ( name && *name ) {
		if ( particleModelDefHandle >= 0 ){
			gameRenderWorld->FreeEntityDef( particleModelDefHandle );
		}
		memset( &particleRenderEntity, 0, sizeof ( particleRenderEntity ) );
		const idDeclModelDef *modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name ) );
		if ( modelDef ) {
			particleRenderEntity.origin = physicsObj.GetAbsBounds().GetCenter();
			particleRenderEntity.axis = mat3_identity;
			particleRenderEntity.hModel = modelDef->ModelHandle();
			float rgb = ( burn ) ? 0.0f : 1.0f;
			particleRenderEntity.shaderParms[ SHADERPARM_RED ] = rgb;
			particleRenderEntity.shaderParms[ SHADERPARM_GREEN ] = rgb;
			particleRenderEntity.shaderParms[ SHADERPARM_BLUE ] = rgb;
			particleRenderEntity.shaderParms[ SHADERPARM_ALPHA ] = rgb;
			particleRenderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.realClientTime );
			particleRenderEntity.shaderParms[ SHADERPARM_DIVERSITY ] = ( burn ) ? 1.0f : gameLocal.random.RandomInt( 90 );
			if ( !particleRenderEntity.hModel ) {
				particleRenderEntity.hModel = renderModelManager->FindModel( name );
			}
			particleModelDefHandle = gameRenderWorld->AddEntityDef( &particleRenderEntity );
			if ( burn ) {
				BecomeActive( TH_THINK );
			}
			particleTime = gameLocal.realClientTime;
		}
	}
}

/*
================
idExplodingBarrel::AddLight
================
*/
void idExplodingBarrel::AddLight( const char *name, bool burn ) {
	if ( lightDefHandle >= 0 ){
		gameRenderWorld->FreeLightDef( lightDefHandle );
	}
	memset( &light, 0, sizeof ( light ) );
	light.axis = mat3_identity;
	light.lightRadius.x = spawnArgs.GetFloat( "light_radius" );
	light.lightRadius.y = light.lightRadius.z = light.lightRadius.x;
	light.origin = physicsObj.GetOrigin();
	light.origin.z += 128;
	light.pointLight = true;
	light.shader = declManager->FindMaterial( name );
	light.shaderParms[ SHADERPARM_RED ] = 2.0f;
	light.shaderParms[ SHADERPARM_GREEN ] = 2.0f;
	light.shaderParms[ SHADERPARM_BLUE ] = 2.0f;
	light.shaderParms[ SHADERPARM_ALPHA ] = 2.0f;
	lightDefHandle = gameRenderWorld->AddLightDef( &light );
	lightTime = gameLocal.realClientTime;
	BecomeActive( TH_THINK );
}

/*
================
idExplodingBarrel::ExplodingEffects
================
*/
void idExplodingBarrel::ExplodingEffects( void ) {
	const char *temp;

	StartSound( "snd_explode", SND_CHANNEL_ANY, 0, false, NULL );

	temp = spawnArgs.GetString( "model_damage" );
	if ( *temp != '\0' ) {
		SetModel( temp );
		Show();
	}

	temp = spawnArgs.GetString( "model_detonate" );
	if ( *temp != '\0' ) {
		AddParticles( temp, false );
	}

	temp = spawnArgs.GetString( "mtr_lightexplode" );
	if ( *temp != '\0' ) {
		AddLight( temp, false );
	}

	temp = spawnArgs.GetString( "mtr_burnmark" );
	if ( *temp != '\0' ) {
		gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), GetPhysics()->GetGravity(), 128.0f, true, 96.0f, temp );
	}
}

/*
================
idExplodingBarrel::Killed
================
*/
void idExplodingBarrel::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {

	if ( IsHidden() || state == EXPLODING || state == BURNING ) {
		return;
	}

	float f = spawnArgs.GetFloat( "burn" );
	if ( f > 0.0f && state == NORMAL ) {
		state = BURNING;
		PostEventSec( &EV_Explode, f );
		StartSound( "snd_burn", SND_CHANNEL_ANY, 0, false, NULL );
		AddParticles( spawnArgs.GetString ( "model_burn", "" ), true );
		return;
	} else {
		state = EXPLODING;
		if ( gameLocal.isServer ) {
			idBitMsg	msg;
			byte		msgBuf[MAX_EVENT_PARAM_SIZE];

			msg.Init( msgBuf, sizeof( msgBuf ) );
			msg.WriteInt( gameLocal.time );
			ServerSendEvent( EVENT_EXPLODE, &msg, false, -1 );
		}
	}

	// do this before applying radius damage so the ent can trace to any damagable ents nearby
	Hide();
	physicsObj.SetContents( 0 );

	const char *splash = spawnArgs.GetString( "def_splash_damage", "damage_explosion" );
	if ( splash && *splash ) {
		gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, attacker, this, this, splash );
	}

	ExplodingEffects( );

	//FIXME: need to precache all the debris stuff here and in the projectiles
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_debris" );
	// bool first = true;
	while ( kv ) {
		const idDict *debris_args = gameLocal.FindEntityDefDict( kv->GetValue(), false );
		if ( debris_args ) {
			idEntity *ent;
			idVec3 dir;
			idDebris *debris;
			//if ( first ) {
				dir = physicsObj.GetAxis()[1];
			//	first = false;
			//} else {
				dir.x += gameLocal.random.CRandomFloat() * 4.0f;
				dir.y += gameLocal.random.CRandomFloat() * 4.0f;
				//dir.z = gameLocal.random.RandomFloat() * 8.0f;
			//}
			dir.Normalize();

			gameLocal.SpawnEntityDef( *debris_args, &ent, false );
			if ( !ent || !ent->IsType( idDebris::Type ) ) {
				gameLocal.Error( "'projectile_debris' is not an idDebris" );
			}

			debris = static_cast<idDebris *>(ent);
			debris->Create( this, physicsObj.GetOrigin(), dir.ToMat3() );
			debris->Launch();
			debris->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = ( gameLocal.time + 1500 ) * 0.001f;
			debris->UpdateVisuals();

		}
		kv = spawnArgs.MatchPrefix( "def_debris", kv );
	}

	physicsObj.PutToRest();
	CancelEvents( &EV_Explode );
	CancelEvents( &EV_Activate );

	f = spawnArgs.GetFloat( "respawn" );
	if ( f > 0.0f ) {
		PostEventSec( &EV_Respawn, f );
	} else {
		PostEventMS( &EV_Remove, 5000 );
	}

	if ( spawnArgs.GetBool( "triggerTargets" ) ) {
		ActivateTargets( this );
	}
}

/*
================
idExplodingBarrel::Damage
================
*/
void idExplodingBarrel::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
					  const char *damageDefName, const float damageScale, const int location, const idVec3 &iPoint ) {

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}
	if ( damageDef->FindKey( "radius" ) && GetPhysics()->GetContents() != 0 && GetBindMaster() == NULL ) {
		PostEventMS( &EV_Explode, 400 );
	} else {
		idEntity::Damage( inflictor, attacker, dir, damageDefName, damageScale, location, iPoint );
	}
}

/*
================
idExplodingBarrel::Event_TriggerTargets
================
*/
void idExplodingBarrel::Event_TriggerTargets() {
	ActivateTargets( this );
}

/*
================
idExplodingBarrel::Event_Explode
================
*/
void idExplodingBarrel::Event_Explode() {
	if ( state == NORMAL || state == BURNING ) {
		state = BURNEXPIRED;
		Killed( NULL, NULL, 0, vec3_zero, 0 );
	}
}

/*
================
idExplodingBarrel::Event_Respawn
================
*/
void idExplodingBarrel::Event_Respawn() {
	int i;
	int minRespawnDist = spawnArgs.GetInt( "respawn_range", "256" );
	if ( minRespawnDist ) {
		float minDist = -1;
		for ( i = 0; i < gameLocal.numClients; i++ ) {
			if ( !gameLocal.entities[ i ] || !gameLocal.entities[ i ]->IsType( idPlayer::Type ) ) {
				continue;
			}
			idVec3 v = gameLocal.entities[ i ]->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			float dist = v.Length();
			if ( minDist < 0 || dist < minDist ) {
				minDist = dist;
			}
		}
		if ( minDist < minRespawnDist ) {
			PostEventSec( &EV_Respawn, spawnArgs.GetInt( "respawn_again", "10" ) );
			return;
		}
	}
	const char *temp = spawnArgs.GetString( "model" );
	if ( temp && *temp ) {
		SetModel( temp );
	}
	health = spawnArgs.GetInt( "health", "5" );
	fl.takedamage = true;
	physicsObj.SetOrigin( spawnOrigin );
	physicsObj.SetAxis( spawnAxis );
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.DropToFloor();
	state = NORMAL;
	Show();
	UpdateVisuals();
}

/*
================
idMoveable::Event_Activate
================
*/
void idExplodingBarrel::Event_Activate( idEntity *activator ) {
	Killed( activator, activator, 0, vec3_origin, 0 );
}

/*
================
idMoveable::WriteToSnapshot
================
*/
void idExplodingBarrel::WriteToSnapshot( idBitMsgDelta &msg ) const {
	idMoveable::WriteToSnapshot( msg );
	msg.WriteBits( IsHidden(), 1 );
}

/*
================
idMoveable::ReadFromSnapshot
================
*/
void idExplodingBarrel::ReadFromSnapshot( const idBitMsgDelta &msg ) {

	idMoveable::ReadFromSnapshot( msg );
	if ( msg.ReadBits( 1 ) ) {
		Hide();
	} else {
		Show();
	}
}

/*
================
idExplodingBarrel::ClientReceiveEvent
================
*/
bool idExplodingBarrel::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_EXPLODE:
			if ( gameLocal.realClientTime - msg.ReadInt() < spawnArgs.GetInt( "explode_lapse", "1000" ) ) {
				ExplodingEffects( );
			}
			return true;
		default:
			break;
	}

	return idBarrel::ClientReceiveEvent( event, time, msg );
}


#if 0
/*********************************************************
Zeroth's idWood.
for breaking boards, rocks, etc.
*********************************************************/

CLASS_DECLARATION( idMoveable, idWood )
//	EVENT( EV_Touch,			idLiquid::Event_Touch )
END_CLASS
// HEXEN : Zeroth
idWood::idWood( void ) {
	geoConstructed = false;
}
// HEXEN : Zeroth
void idWood::Spawn( void ) {
	if (!geoConstructed) {
		ConstructGeo();
	}
	
	BecomeActive( TH_THINK );
}
// HEXEN : Zeroth
// SDK does not contain information that allows us to access faces of models
// like (*faces[face])[vertex], so we'll have to build that information here.
void idWood::ConstructGeo( void ) {
	int i, v, f, v2, f2;
	srfTriangles_t *surf = renderEntity.hModel->Surface(0)->geometry;
	// assign vertices to geo.faces
	for (i=0; i < (surf->numIndexes / 3); i++) {
		f = geo.faces.New();
		geo.faces[f].plane = &surf->facePlanes[i]; //z.todo: delete geta/b/c/d funcs.
		for ( v=0; v < surf->numVerts; v++ ) {
			if ( surf->facePlanes[i].Side(surf->verts[v].xyz, ON_EPSILON ) == PLANESIDE_ON ) {
				geo.faces[f].verts.Append( surf->verts[v].xyz ); //origin + winding[0].ToVec3() * axis;
			}
		}
	}
	geo.faces.Condense();
//************
//merge perpendicular faces that share 2+ verts
//************
	int same;
	for (f=0; f < geo.faces.Num(); f++) {
		int verts = geo.faces[f].verts.Num();
		for ( f2=0; f2 < geo.faces.Num(); f2++ ) {
			if ( f == f2 ) {
				continue;
			}
			// perpendicular test
			if ( geo.faces[f].plane->Normal() != geo.faces[f2].plane->Normal() ) {
				continue;
			}
			// test if all vertices on plane
			for ( v2=0; v2<geo.faces[f2].verts.Num(); v2++ ) {
				if ( geo.faces[f].plane->Side( geo.faces[f2].verts[v2], ON_EPSILON ) != PLANESIDE_ON ) {
					continue;
				}
			}
			// test if share 2 vertices
			same=0;
			for ( v2=0; v2 < geo.faces[f2].verts.Num() && same < 2; v2++ ) {
				if ( geo.faces[f].verts.Find( geo.faces[f2].verts[v2] ) ) {
					same++;
				}
			}
//			same=0;
//			for ( v=0; v < verts && same < 2; v++ ) {
//				for ( v2=0; v2 < geo.faces[f2].verts.Num() && same < 2; v2++ ) {
//					if ( geo.faces[f].verts.Find( geo.faces[f2].verts[v2] ) ) {
//						same++;
//					}
//				}
//			}
//
			if ( same >= 2 ){
				for ( v2=0; v2 < geo.faces[f2].verts.Num(); v2++ ) {
					geo.faces[f].verts.Append( geo.faces[f2].verts[v2] );
				}
				geo.faces[f2].verts.Clear();
				geo.faces.RemoveIndex(f2);
				geo.faces.Condense();
				f2--;
			}
		}
	}
//************
//delete duplicate vertices
//************
	for (f=0; f < geo.faces.Num(); f++) {
		for ( v=0; v < geo.faces[f].verts.Num(); v++ ) {
			for ( i=v+1; i < geo.faces[f].verts.Num(); i++ ) {
				if ( idVec3( geo.faces[f].verts[v] - geo.faces[f].verts[i] ).Length() < 0.00001 ) {
					geo.faces[f].verts.RemoveIndex(i);
					geo.faces[f].verts.Condense();
					i--;
				}
			}
		}
	}
	// debuging
//	for ( f=0; f < geo.faces.Num(); f++ ) {
//		gameLocal.Printf("face %i:\n", f );
//		for ( v=0; v < geo.faces[f].verts.Num(); v++ ) {
//			gameLocal.Printf("%i:%i : %f,%f,%f\n", f, v, geo.faces[f].verts[v].x, geo.faces[f].verts[v].y, geo.faces[f].verts[v].z );
//
//		}
//	}
//************
//sort vertices clockwise
//************
	for ( f=0; f < geo.faces.Num(); f++ ) {
		bool DoneSorting;
		idVec3 h,j;
		for (i=0; i < geo.faces[f].verts.Num(); i++)
		{
			DoneSorting = true;
			for (v=0; v < geo.faces[f].verts.Num()-2; v++) {
				h = geo.faces[f].verts[v+1] - geo.faces[f].verts[0];
				j = geo.faces[f].verts[v+2] - geo.faces[f].verts[0];
				j = h.Cross(j);
				j.Normalize();
				if ( j * geo.faces[f].plane->Normal() < 0.0f ) {
					DoneSorting = false;
					j = geo.faces[f].verts[v+2];
					geo.faces[f].verts[v+2] = geo.faces[f].verts[v+1];
					geo.faces[f].verts[v+1] = j;
				}
			}
			if (DoneSorting) {
			  break;
			}
		}
	}
//************
//get center of model
//***********
	geo.center.Zero();
	int verts=0;
	for ( f=0; f < geo.faces.Num(); f++ ) {
		verts += geo.faces[f].verts.Num();
		for ( v=0; v < geo.faces[f].verts.Num(); v++ ) {
			geo.center += geo.faces[f].verts[v];
		}
	}
	geo.center /= verts;
//************
//get average surface area per side
//************
	geo.area = 0;
	float a,b,c,p;
	for ( f=0; f < geo.faces.Num(); f++ ) {
		// get center of the face
		geo.faces[f].center.Zero();
		for ( v=0; v < geo.faces[f].verts.Num(); v++ ) {
			geo.faces[f].center += geo.faces[f].verts[v];
		}
		geo.faces[f].center /= geo.faces[f].verts.Num();
		// get area of each portion of face
		geo.faces[f].area = 0;
		for ( v=0; v < geo.faces[f].verts.Num() - 1; v++ ) {
			a = ( geo.faces[f].center - geo.faces[f].verts[v] ).Length();
			b = ( geo.faces[f].verts[v] - geo.faces[f].verts[v+1] ).Length();
			c = ( geo.faces[f].verts[v+1] - geo.faces[f].center ).Length();
			p = (a+b+c)/2;
			geo.faces[f].area += idMath::Sqrt( p * ( p - a ) * ( p - b ) * ( p - c ) );
		}
		gameLocal.Printf("face %i area %f\n", f, geo.faces[f].area);
		geo.area += geo.faces[f].area;
	}
	geo.avgArea = geo.area / geo.faces.Num();
	gameLocal.Printf("tot area %f\n", geo.area);
	gameLocal.Printf("areaavg %f\n", geo.avgArea);
//***********
//store adjacent sides
//***********
	// faces sharing at least one vertice with one another are adjacent.
	for ( f=0; f<geo.faces.Num(); f++ ) {
		for ( f2=f+1; f2<geo.faces.Num(); f2++ ) {
			for ( v2=0; v2<geo.faces[f2].verts.Num(); v2++) {
				if ( geo.faces[f].verts.Find( geo.faces[f2].verts[v2] ) ) {
					geo.faces[f].adjacent.Append(f2);
					geo.faces[f2].adjacent.Append(f);
					break;
				}
			}
		}
	}
	geoConstructed = true;
}
void idWood::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					 const char *damageDefName, const float damageScale, const int location, idVec3 &iPoint ) {
	if ( !fl.takedamage ) {
		return;
	}
	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}
	lastiPoint = iPoint;
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}
	int	damage = damageDef->GetInt( "damage" );
	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage ) {
		// do the damage
		health -= damage;
		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}
			Killed( inflictor, attacker, damage, dir, location );
		} else {
			Pain( inflictor, attacker, damage, dir, location );
		}
	}
}
bool idWood::CheckFloating( idWood *caller, idWood *chain[], int c ) {
	// make sure we aren't caught in a recursive loop with the other touching entities
	int numClipModels, t, i;
	idBounds bounds;
	idClipModel *cm, *clipModelList[ MAX_GENTITIES ];
	idEntity* entity;
	for ( i=0; i<c; i++ ) {
		if ( chain[i] == this ) {
			return true;
		}
	}
	if ( !this->GetPhysics()->GetClipModel() ) {
		return false;
	}
	// add self to chain
	chain[c] = this;
	c++;
	bounds.FromTransformedBounds( this->GetPhysics()->GetClipModel()->GetBounds(), this->GetPhysics()->GetClipModel()->GetOrigin(), this->GetPhysics()->GetClipModel()->GetAxis() );
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );
	// check all touchers to see if they're NOT floating
	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModelList[ i ];
		if ( !cm->IsTraceModel() ) {
			continue;
		}
		entity = clipModelList[ i ]->GetEntity();
		if ( !entity ) {
			continue;
		}
		if ( !gameLocal.clip.ContentsModel( cm->GetOrigin(), cm, cm->GetAxis(), -1, GetPhysics()->GetClipModel()->Handle(), GetPhysics()->GetClipModel()->GetOrigin(), GetPhysics()->GetClipModel()->GetAxis() ) ) {
			continue;
		}
		if ( !entity->IsType( idWood::Type ) ) {
			return false;
		}
		if ( static_cast< idWood* >( entity ) == caller ) {
			continue;
		}
		if ( static_cast< idWood* >( entity )->spawnArgs.GetBool("stopper") ) {
			return false;
		}
		if ( static_cast< idWood* >( entity )->health <= 0 ) {
			return true;
		}
		if ( ! static_cast< idWood* >( entity )->CheckFloating( this, chain, c ) ) {
			return false;
		}
	}
	// if we haven't found a non-floater, we're floating
	physicsObj.EnableImpact();
	physicsObj.SetLinearVelocity( idVec3(0,0,-20) );
	BecomeNonSolid();
	return true;
}


bool idWood::CheckFloating( idWood &caller, idWood *chain, int c ) {
	// make sure we aren't caught in a recursive loop with the other touching entities
	for ( i=0; i<c; i++ ) {
		if ( chain[i] == this ) {
			return true;
		}
	}
	int i, n;
	idEntity *	entityList[ MAX_GENTITIES ];
	n = gameLocal.clip.EntitiesTouchingBounds( physicsObj.clipModel->bounds, -1, entityList, MAX_GENTITIES );
	// if theres only one toucher, and it's the caller, we're floating
	if ( n == 1 && entityList[0] == caller ) {
		physicsObj.EnableImpact();
		physicsObj.SetLinearVelocity( idVec3(0,0,1) );
		return true;
	}
	// if no touchers.. we're floating
	if ( n == 0 ) {
		physicsObj.EnableImpact();
		physicsObj.SetLinearVelocity( idVec3(0,0,1) );
		return true;
	}
	// add self to chain
	chain[n] = this;
	c++;
	// check all touchers to see if they're NOT floating
	for ( i=0; i < n; i++ ) {
		if ( !entityList[i]->IsType( idWood::Type ) ) {
			return false;
		}
		if ( entityList[i] == caller ) {
			continue;
		}
		if ( ! static_cast< idWood* >( entityList[i] )->CheckFloating( this, chain, c ) ) {
			return false;
		}
	}
	// if we haven't found a non-floater, we're floating
	physicsObj.EnableImpact();
	physicsObj.SetLinearVelocity( idVec3(0,0,1) );
	return true;
}

// HEXEN : Zeroth
void idWood::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idEntity *impactor = NULL;
	int f, v, r, end, op;
	idVec3 ray;
	float dist;
	float oldDist = 0;
	physicsObj.EnableImpact();
	physicsObj.SetLinearVelocity( idVec3(0,0,-20) );
	BecomeNonSolid();
	idWood *	chain[ MAX_GENTITIES ];
	CheckFloating( this, chain, 0 );
	if ( unbindOnDeath ) {
		Unbind();
	}
	if ( renderEntity.gui[ 0 ] ) {
		renderEntity.gui[ 0 ] = NULL;
	}
	ActivateTargets( this );
	fl.takedamage = false;

	// spawn new log (for one half)

/****************************************************
for trying to create a new one on teh fly, or using the default model
*****************************************************/
	idDict newDebris;
	//newDebris.Copy(spawnArgs);
//	newDebris.Set("material", spawnArgs.GetString("material") );
//	idClipModel *nclip=new idClipModel( GetPhysics()->GetClipModel() );
//	newDebris.Set( "clipmodel", nclip->GetEntity()->GetName() );
//	renderEntity_t *nrend=new renderEntity_t( renderEntity );
//	newDebris.Set( "model", nrend->hModel->Name() );
	//newDebris.Delete("name");
	//newDebris.Delete("clipmodel");
	idWood *deb = static_cast< idWood * >( gameLocal.SpawnEntityType(idMoveable::Type, &newDebris) );
	idVec3 org( physicsObj.GetOrigin() );
	deb->GetPhysics()->SetOrigin( org );
	deb->GetPhysics()->SetAxis( physicsObj.GetAxis() );
	deb->GetPhysics()->SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), spawnArgs.GetFloat( "density", "0.5" ) );
	//renderEntity_t nrend( renderEntity );
	//deb->SetModel( nrend.hModel->Name() );
	//physicsObj.GetClipModel()->LoadModel( modelDefHandle );
	// err: TraceModelForClipModel: clip model 0 on wood_2 is not a trace model
//	idRenderModel *mod;
//	mod = renderModelManager->AllocModel();
//	mod->AllocSurfaceTriangles( renderEntity.hModel->Surface(0)->geometry->numVerts, renderEntity.hModel->Surface(0)->geometry->numIndexes );
//	mod->AddSurface( *renderEntity.hModel->Surface(0) );
//	mod->FinishSurfaces();
//	mod->FreeVertexCache();
	idRenderModel *mod;
	mod = renderModelManager->AllocModel();
	mod->LoadModel();
	deb->SetModel( mod->Name() );
	deb->UpdateModel();
	deb->UpdateModelTransform();
	deb->UpdateVisuals();
	srfTriangles_t *debsgeo = deb->renderEntity.hModel->Surface(0)->geometry;
	srfTriangles_t *sgeo = renderEntity.hModel->Surface(0)->geometry;
/****************************************************
	using woodbreak entity
****************************************************/
	idWood *deb = static_cast< idWood* >( gameLocal.FindEntity("woodbreak") );
	if ( !deb ) {
		return;
	}
	//srfTriangles_t *debsgeo = deb->renderEntity.hModel->Surface(0)->geometry;
	srfTriangles_t *sgeo = renderEntity.hModel->Surface(0)->geometry;
/**********************************************
	add splinters to wood
***********************************************/
	// find the smallest face (short end of the wood)
	for ( end=0, f=1; f<geo.faces.Num(); f++ ) {
		if ( geo.faces[f].area < geo.faces[end].area ) {
			end = f;
		}
	}
/*
	int j, k;
	idDrawVert vv;
	idVec3 newFace[3];
	newFace[0] = geo.faces[end].verts[0];
	newFace[1] = geo.faces[end].verts[1];
	newFace[2] = geo.faces[end].verts[1] + geo.faces[end].plane->Normal() * 10 ;
	idFixedWinding w;
	w.Clear();
	vv = sgeo->verts[ sgeo->indexes[ 0 ] ];
	w.AddPoint( newFace[0] );
	w[0].s = vv.st[0];
	w[0].t = vv.st[1];
	w.AddPoint( newFace[1] );
	w[1].s = vv.st[0];
	w[1].t = vv.st[1];
	w.AddPoint( newFace[2] );
	w[2].s = vv.st[0];
	w[2].t = vv.st[1];
*/
	/** this DEFINITELY doesnt owrk
	sgeo->numVerts += 3;
	realloc( sgeo->verts, sgeo->numVerts * sizeof( sgeo->verts ) );
	realloc( sgeo->facePlanes, ( sgeo->numVerts / 3 ) * sizeof( sgeo->facePlanes ) );
	sgeo->verts[ sgeo->numVerts - 3 ].xyz = newFace[0];
	sgeo->verts[ sgeo->numVerts - 2 ].xyz = newFace[1];
	sgeo->verts[ sgeo->numVerts - 1 ].xyz = newFace[2];
	sgeo->facePlanes[ ( sgeo->numVerts / 3 ) - 1 ] = sgeo->facePlanes[ ( sgeo->numVerts / 3 ) - 2 ];
	**/
/** // using shards... need to figure out how to use them properly.
	shard_t *shard = new shard_t;
	shard->clipModel = 	GetPhysics()->GetClipModel();
	shard->droppedTime = -1;
	shard->winding = w;
	shard->decals.Clear();
	shard->edgeHasNeighbour.AssureSize( w.GetNumPoints(), false );
	shard->neighbours.Clear();
	shard->atEdge = false;
	// setup the physics
	shard->physicsObj.SetSelf( this );
	shard->physicsObj.SetClipModel( shard->clipModel, 1 );
	shard->physicsObj.SetMass( 	GetPhysics()->GetMass() );
	shard->physicsObj.SetOrigin( 	GetPhysics()->GetOrigin() );
	shard->physicsObj.SetAxis( 	GetPhysics()->GetAxis() );
	shard->physicsObj.SetBouncyness( 1 );
	shard->physicsObj.SetFriction( 0.6f, 0.6f, 1 );
	shard->droppedTime = gameLocal.time;
	shard->physicsObj.SetContents( CONTENTS_RENDERMODEL );
	shard->physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	//shard->clipModel->SetId( shard->clipModel->GetId(); );
	BecomeActive( TH_PHYSICS );
*/
/**********************************************
	match up geo
***********************************************/
	idList <bool> oneClock;
	idList <bool> twoClock;
	idVec3 h,j;
	// store vertices in the same direction. doesnt seem to accomplish anything
	for ( f=0, r=0; r < sgeo->numVerts; r+=3, f++ ) {
		oneClock.Append(true);
		// check if side is stored clockwise
		h = sgeo->verts[r+1].xyz - sgeo->verts[r].xyz;
		j = sgeo->verts[r+2].xyz - sgeo->verts[r].xyz;
		j = h.Cross(j);
		j.Normalize();
		if ( j * sgeo->facePlanes[f].Normal() < 0.0f ) {
			oneClock[f] = false;
		}
	}
	for ( f=0, r=0; r < debsgeo->numVerts; r+=3, f++ ) {
		twoClock.Append(true);
		// check if side is stored clockwise
		h = debsgeo->verts[r+1].xyz - debsgeo->verts[r].xyz;
		j = debsgeo->verts[r+2].xyz - debsgeo->verts[r].xyz;
		j = h.Cross(j);
		j.Normalize();
		if ( j * debsgeo->facePlanes[f].Normal() < 0.0f ) {
			twoClock[f] = false;
		}
	}
	for ( f=0, r=0; r < sgeo->numVerts; r+=3, f++ ) {
		if ( oneClock[f] == twoClock[f] ) {
			debsgeo->verts[r].xyz = sgeo->verts[r].xyz;
			debsgeo->verts[r+1].xyz = sgeo->verts[r+1].xyz;
			debsgeo->verts[r+2].xyz = sgeo->verts[r+2].xyz;
		} else {
			debsgeo->verts[r].xyz = sgeo->verts[r+2].xyz;
			debsgeo->verts[r+1].xyz = sgeo->verts[r+1].xyz;
			debsgeo->verts[r+2].xyz = sgeo->verts[r].xyz;
		}
	}
	// match vertices
	for ( r=0; r < sgeo->numVerts; r++ ) {
		debsgeo->verts[r].xyz = sgeo->verts[r].xyz;
	}
	// also seems to not help
	// match faces
	for ( r=0; r < sgeo->numVerts; r+=3 ) {
		debsgeo->facePlanes[r].SetDist( sgeo->facePlanes[r].Dist() );
		debsgeo->facePlanes[r].SetNormal( sgeo->facePlanes[r].Normal() );
	}
	Hide();
	// match indexed
	for ( r=0; r <sgeo->numIndexes; r+=3 ) {
		for ( v = 0; v < 3; v++ ) {
			debsgeo->verts[ sgeo->indexes[ r + 2 - v ] ].st[0] = sgeo->verts[ sgeo->indexes[ r + 2 - v ] ].st[0];
			debsgeo->verts[ sgeo->indexes[ r + 2 - v ] ].st[1] = sgeo->verts[ sgeo->indexes[ r + 2 - v ] ].st[1];
		}
	}
	// match tangents and normal
	for ( r=0; r <sgeo->numIndexes; r+=3 ) {
		debsgeo->verts[r].tangents[0] = sgeo->verts[r].tangents[0];
		debsgeo->verts[r].tangents[1] = sgeo->verts[r].tangents[1];
		debsgeo->verts[r].normal = sgeo->verts[r].normal;
	}
	// deb->renderEntity.customShader
	// deb->renderEntity.hModel->Surface(0)->shader
	deb->renderEntity.customShader = renderEntity.hModel->Surface(0)->shader;
	deb->GetPhysics()->SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), spawnArgs.GetFloat( "density", "0.5" ), 0, true );
	deb->GetPhysics()->SetOrigin( physicsObj.GetOrigin() );
	deb->GetPhysics()->SetAxis( physicsObj.GetAxis() );
	deb->renderEntity.hModel->FreeVertexCache();
	deb->UpdateModel();
	deb->UpdateModelTransform();
	deb->UpdateVisuals();
	deb->Show();

/**********************************************
	Split in half
***********************************************/
	// find the smallest face (short end of the wood)
	for ( end=0, f=1; f<geo.faces.Num(); f++ ) {
		if ( geo.faces[f].area < geo.faces[end].area ) {
			end = f;
		}
	}
	// find opposing face. the side with the center furthest from end is the opposing side (unless the wood is shaped REAL funny)
	for ( oldDist = 0, f=0; f<geo.faces.Num(); f++ ) {
		dist = idVec3( geo.faces[f].center - geo.faces[end].center ).Length();
		if ( dist > oldDist ) {
			op = f;
		}
	}
	// move shortest face half way to opposing face
	for ( v=0; v<geo.faces[end].verts.Num(); v++ ) {
		// update verts in the render entity
		for ( r=0; r < sgeo->numVerts; r++ ) {
			//sgeo->verts[r].xyz /= 2;
			if ( sgeo->verts[r].xyz != geo.faces[end].verts[v] ) {
				continue;
			}
			sgeo->verts[r].xyz += ( geo.faces[op].center - geo.faces[end].center ) / 2;
		}
		geo.faces[end].verts[v] += ( geo.faces[op].center - geo.faces[end].center ) / 2;
	}
//	// other half: move shortest face half way to opposing face
//	for ( v=0; v<deb->geo.faces[op].verts.Num(); v++ ) {
//		// update verts in the render entity
//		for ( r=0; r < debsgeo->numVerts; r++ ) {
//			if ( debsgeo->verts[r].xyz == deb->geo.faces[op].verts[v] ) {
//				debsgeo->verts[r].xyz += ( deb->geo.faces[end].center - deb->geo.faces[op].center ) / 2;
//			}
//		}
//		deb->geo.faces[op].verts[v] += ( deb->geo.faces[end].center - deb->geo.faces[op].center ) / 2;
//	}
	// store distance
	dist = idVec3(geo.faces[end].center - geo.faces[op].center).Length();
	// update center now
	geo.faces[end].center += ( geo.faces[op].center - geo.faces[end].center ) / 2;
	deb->geo.faces[op].center += ( deb->geo.faces[end].center - deb->geo.faces[op].center ) / 2;
// currently not functional
//	// resize short face and update area
//	float newArea = ( ( geo.faces[op].area - geo.faces[end].area ) / dist ) * ( dist / 2 ) + geo.faces[end].area;
//	float vertMoveDist = ( newArea / geo.faces[end].area ) / 4; // 4 sides
//	geo.faces[end].area = newArea;
//	for ( v=0; v < geo.faces[end].verts.Num(); v++ ) {
//		idVec3 vertAdd = ( geo.faces[end].verts[v] - geo.faces[end].center );
//
//		vertAdd.Normalize();
//		vertAdd *= vertMoveDist;
//
//		// update verts in the render entity
//		for ( int r=0; r < sgeo->numVerts; r++ ) {
//			if ( sgeo->verts[r].xyz == geo.faces[end].verts[v] ) {
//				sgeo->verts[r].xyz += vertAdd;
//			}
//		}
//
//		geo.faces[end].verts[v] += vertAdd;
//	}
//
//	// other half: resize short face and update area
//	newArea = ( ( deb->geo.faces[end].area - deb->geo.faces[op].area ) / dist ) * ( dist / 2 ) + deb->geo.faces[op].area;
//	vertMoveDist = ( newArea / deb->geo.faces[op].area ) / 4; // 4 sides
//	deb->geo.faces[op].area = newArea;
//	for ( v=0; v < deb->geo.faces[op].verts.Num(); v++ ) {
//		idVec3 vertAdd = ( deb->geo.faces[op].verts[v] - deb->geo.faces[op].center );
//
//		vertAdd.Normalize();
//		vertAdd *= vertMoveDist;
//
//		// update verts in the render entity
//		for ( int r=0; r < debsgeo->numVerts; r++ ) {
//			if ( debsgeo->verts[r].xyz == deb->geo.faces[op].verts[v] ) {
//				debsgeo->verts[r].xyz += vertAdd;
//			}
//		}
//
//		deb->geo.faces[op].verts[v] += vertAdd;
//	}
	renderEntity.hModel->FreeVertexCache();
	UpdateModel();
	UpdateModelTransform();
	UpdateVisuals();
	deb->renderEntity.hModel->FreeVertexCache();
	deb->UpdateModel();
	deb->UpdateModelTransform();
	deb->UpdateVisuals();
	// other side: update render model's bounds
	srfTriangles_t *suu = sgeo;
	int mins=0, maxs=0;
	idVec3 min, max;
	max.x = min.x = suu->verts[0].xyz.x;
	max.y = min.y = suu->verts[0].xyz.y;
	max.z = min.z = suu->verts[0].xyz.z;
	for ( v=1; v < suu->numVerts; v++ ) {
		if ( suu->verts[v].xyz.x < min.x ) { min.x = suu->verts[v].xyz.x; }
		if ( suu->verts[v].xyz.y < min.y ) { min.y = suu->verts[v].xyz.y; }
		if ( suu->verts[v].xyz.z < min.z ) { min.z = suu->verts[v].xyz.z; }
		if ( suu->verts[v].xyz.x > max.x ) { max.x = suu->verts[v].xyz.x; }
		if ( suu->verts[v].xyz.y > max.y ) { max.y = suu->verts[v].xyz.y; }
		if ( suu->verts[v].xyz.z > max.z ) { max.z = suu->verts[v].xyz.z; }
	}
	suu->bounds = idBounds(min, max);
	// create a new clip model based on the new bounds
	float density = spawnArgs.GetFloat( "density", "0.5" );
	physicsObj.SetClipModel( new idClipModel( idTraceModel( suu->bounds ) ), density );
	UpdateModelTransform();
	UpdateVisuals();
//	// other side: update render model's bounds
//	suu = debsgeo;
//
//	mins=0, maxs=0;
//	max.x = min.x = suu->verts[0].xyz.x;
//	max.y = min.y = suu->verts[0].xyz.y;
//	max.z = min.z = suu->verts[0].xyz.z;
//
//	for ( v=1; v < suu->numVerts; v++ ) {
//		if ( suu->verts[v].xyz.x < min.x ) { min.x = suu->verts[v].xyz.x; }
//		if ( suu->verts[v].xyz.y < min.y ) { min.y = suu->verts[v].xyz.y; }
//		if ( suu->verts[v].xyz.z < min.z ) { min.z = suu->verts[v].xyz.z; }
//		if ( suu->verts[v].xyz.x > max.x ) { max.x = suu->verts[v].xyz.x; }
//		if ( suu->verts[v].xyz.y > max.y ) { max.y = suu->verts[v].xyz.y; }
//		if ( suu->verts[v].xyz.z > max.z ) { max.z = suu->verts[v].xyz.z; }
//	}
//	suu->bounds = idBounds(min, max);
//
//	// other side: create a new clip model based on the new bounds
//	density = spawnArgs.GetFloat( "density", "0.5" );
//	deb->physicsObj.SetClipModel( new idClipModel( idTraceModel( suu->bounds ) ), density );
//
//	deb->UpdateModelTransform();
//	deb->UpdateVisuals();

}

/**************************************************************
test code for trying to create new logs
///////////////////////////////////////////////////////////////*/


	/********************************
	remove old model and spawn two halves
	********************************/
	int f,f2,v,v2,num,lst,end;
	// find the smallest face (short end of the wood)
	for ( end=0, f=1; f<geo.faces.Num(); f++ ) {
		if ( geo.faces[f].area < geo.faces[end].area ) {
			end = f;
		}
	}
	idList< int >	adjs;
	idList< idVec3 > ver;
	idList< idVec3 > ver2;
	idList< idVec3 > ver3;
	// find all adjacent sides to end
	for ( f=0; f<geo.faces.Num(); f++ ) {
		if ( end == f ) {
			continue;
		}
		if ( geo.faces[f].adjacent.Find(end) ) {
			adjs.Append(f);
			for ( v=0; v<geo.faces[f].verts.Num(); v++ ) {
				ver.Append( geo.faces[f].verts[v] );
			}
		}
	}
	// ver2: will contain vertices of the beginning face (the lower ring of vertices on the log)
	ver2.Clear();
	for ( v=0; v<geo.faces[end].verts.Num(); v++ ) {
		ver2.Append( geo.faces[end].verts[v] );
	}
	// ver: will contain vertices from the search that weren't in the beginning face (the upper ring of vertices on the log)
	for ( v=0; v<ver.Num(); v++ ) {
		if ( geo.faces[end].verts.Find( ver[v] ) ) {
			ver.RemoveIndex(v);
			ver.Condense();
			v--;
		}
	}
	// we must have at least half of the faces in order to shorten
	while ( adjs.Num() + 1 < geo.faces.Num() / 2 ) {
		// all the while, keep a list of vertices from the current run that aren't attached to the previous.
		//ver.Clear();
		//ver.Append(geo.faces[f].verts[v] );
		// find adjacent sides to each side in our list, and store it in our list.
		lst=adjs.Num();
		for ( i=0; i<lst; i++ ) {
			for ( f=0; f<geo.faces.Num(); f++ ) {
				if ( i == f ) {
					continue;
				}
				if ( geo.faces[f].adjacent.Find(i) ) {
					adjs.Append(f);
					for ( v=0; v<geo.faces[f].verts.Num(); v++ ) {
						ver3.Append( geo.faces[f].verts[v] );
					}
				}
			}
		}
		// ver2: will contain vertices that were both in ver and ver3 (the lower ring of vertices on the log)
		ver2.Clear();
		for ( v=0; v<ver3.Num(); v++ ) {
			if ( ver.Find( ver3[v] ) ) {
				ver2.Append( ver3[v] );
			}
		}
		// ver: will contain vertices from ver3 that weren't in ver (the upper ring of vertices on the log)
		ver.Clear();
		for ( v=0; v<ver3.Num(); v++ ) {
			if ( !ver.Find( ver3[v] ) ) {
				ver.Append( ver3[v] );
			}
		}
		ver3.Clear();
	}
	// we now have enough info for log shrinkage (heh)
	// spawn new log
	idDict newDebris;
	newDebris.Copy(spawnArgs);
	newDebris.Delete("name");
	newDebris.Set("mins", "-5 -5 -5");
	newDebris.Set("maxs", "5 5 5");
	idWood *deb = static_cast< idWood * >( gameLocal.SpawnEntityType(idWood::Type, &newDebris) );
	idVec3 org( physicsObj.GetOrigin() );
	org.z += 50;
	deb->GetPhysics()->SetOrigin( org );
	deb->GetPhysics()->SetAxis( physicsObj.GetAxis() );
	srfTriangles_t *surf = deb->GetRenderEntity()->hModel->Surface(0)->geometry;
	int idx;
	idVec3 nw;
	// shorten the new log
	// z.todo: we're only ASSUMING that ver and ver2 indexes aren't in mismatched order
	for ( v=0; v < surf->numVerts; v++ ) {
		idx = ver.FindIndex( surf->verts[v].xyz );
		if ( idx != -1 ) {
/*
			// get direction fro upper ring vert to lower ring vert
			nw = ver2[idx] - ver[idx];
			// make the vector about half the length of the log
			nw.Normalize();
			nw *= idMath::Sqrt( geo.area / 2 ); // sqrt( area / 2 ) is close enough to half the length of the log
			surf->verts[v].xyz += nw;
*/
		}
	}
	// remove the old log
	// ...
	//gameLocal.Printf("adjs: %i+ 1 = %i\n", adjs.Num(),adjs.Num()+1);
	//gameLocal.Printf("faces: %i / 2 = %i\n", geo.faces.Num(), geo.faces.Num() / 2);


	/****************************************
	CRAP
	*****************************************/
/*
	idList< int > hits;
	int bestHit;
	// find surface which is closest to impact point
	// z.todo: fuck it. deal with this later.

	gameLocal.Printf("math  : %f,%f,%f - ", lastiPoint.x,lastiPoint.y,lastiPoint.z);
	gameLocal.Printf("%f,%f,%f * ", physicsObj.GetOrigin().x,physicsObj.GetOrigin().y,physicsObj.GetOrigin().z);
	gameLocal.Printf("%f,%f,%f\n", -physicsObj.GetAxis().ToAngles().yaw,-physicsObj.GetAxis().ToAngles().pitch,-physicsObj.GetAxis().ToAngles().roll);
	gameLocal.Printf("before: %f,%f,%f\n", lastiPoint.x,lastiPoint.y,lastiPoint.z);
	lastiPoint = lastiPoint - physicsObj.GetOrigin() * -physicsObj.GetAxis();
	gameLocal.Printf(" after: %f,%f,%f\n", lastiPoint.x,lastiPoint.y,lastiPoint.z);
*/

/*
	for ( i=0; i<geo.faces.Num(); i++ ) {
		// test if the point is on that side
		// whatever center is closer
		// normal no mroe than 45 degrees.
//		idVec3 rightCenter = geo.center ;//+ physicsObj.GetOrigin() * physicsObj.GetAxis();
//		idVec3 rightNorm = geo.faces[i].plane->Normal() * physicsObj.GetAxis();
//		rightNorm.Normalize();
//		ray = lastiPoint - rightCenter;
//		ray.Normalize();
/*
		gameLocal.Printf("\nTesting Face: %i\n", i );
//		gameLocal.Printf("  lastiPoint: %f,%f,%f\n", lastiPoint.x,lastiPoint.y,lastiPoint.z);
//		gameLocal.Printf("  geo.center: %f,%f,%f\n", geo.center.x,geo.center.y,geo.center.z );
//		gameLocal.Printf("  WoodCenter: %f,%f,%f\n", rightCenter.x,rightCenter.y,rightCenter.z ); // v->xyz = origin + winding[0].ToVec3() * axis;
//		gameLocal.Printf(" face i norm: %f,%f,%f\n", geo.faces[i].plane->Normal().z,geo.faces[i].plane->Normal().y,geo.faces[i].plane->Normal().z );
//		gameLocal.Printf("WFace i Norm: %f,%f,%f\n", rightNorm.z,rightNorm.y,rightNorm.z );
//		gameLocal.Printf("         Ray: %f,%f,%f\n",ray.x,ray.y,ray.z );
//		gameLocal.Printf("       Angle: %f\n", RAD2DEG( idMath::ACos( rightNorm * ray ) ) );
//		gameLocal.Printf("    physaxis: %f,%f,%f\n",physicsObj.GetAxis().ToAngles().yaw,physicsObj.GetAxis().ToAngles().pitch,physicsObj.GetAxis().ToAngles().roll );
//		gameLocal.Printf("      Origin: %f,%f,%f\n",physicsObj.GetOrigin().x,physicsObj.GetOrigin().y,physicsObj.GetOrigin().z );
		// plane(normal, distance)
		/*
		float dist = -(geo.faces[i].plane->Normal() * geo.faces[i].verts[0]);
		dist = dist + physicsObj.GetOrigin() * physicsObj.GetAxis();
		idPlane plane( geo.faces[i].plane->Normal(), dist)
		*/

		//pa = physicsObj.GetOrigin() * physicsObj.GetAxis() + 

		// make a plane that is where the face is in the world

/*
		gameLocal.Printf("origin: %f,%f,%f\n", physicsObj.GetOrigin().x, physicsObj.GetOrigin().y, physicsObj.GetOrigin().z);
		idVec3 or = physicsObj.GetOrigin() *  physicsObj.GetAxis();
		gameLocal.Printf("toaxis: %f,%f,%f\n", or.x, or.y, or.z);
		gameLocal.Printf("apoint: %f,%f,%f\n", geo.faces[i].verts[0].x, geo.faces[i].verts[0].y, geo.faces[i].verts[0].z);
		idVec3 pa = geo.faces[i].verts[0] * physicsObj.GetAxis();
		gameLocal.Printf("toaxis: %f,%f,%f\n", pa.x, pa.y, pa.z);
		pa = (physicsObj.GetOrigin() + geo.faces[i].verts[0]);
		gameLocal.Printf("added:  %f,%f,%f\n\n", pa.x, pa.y, pa.z);
		idVec3 no = geo.faces[i].plane->Normal();
		gameLocal.Printf("normal: %f,%f,%f\n\n", geo.faces[i].plane->Normal().x, geo.faces[i].plane->Normal().y, geo.faces[i].plane->Normal().z);
		no *= physicsObj.GetAxis();
		gameLocal.Printf("toaxis: %f,%f,%f\n\n", no.x, no.y, no.z);
		float dist = -( geo.faces[i].plane->Normal() * pa );
		gameLocal.Printf("dist:   %f", dist);
		idPlane plane( no, dist);
		if (plane.Side( lastiPoint, ON_EPSILON ) == PLANESIDE_FRONT ) {
//			hits.Append(i); // mark it as a hit
//		}
		if (plane.Side( lastiPoint, ON_EPSILON ) == PLANESIDE_ON ) {
//			hits.Append(i); // mark it as a hit
		}
*/
/*
		if (idMath::ACos( rightNorm * ray ) < DEG2RAD(90) && // if angle between face normal and angle of impact < 90
			geo.faces[i].plane->RayIntersection( lastiPoint, rightCenter - lastiPoint, tmpf ) // and it passes through the face
			 ) {
			gameLocal.Printf("hit\n");
			hits.Append(i); // mark it as a hit
		} */
	//}

//	if ( hits.Num() < 1 ) {
//		return; // this should never happen
//	}

//	gameLocal.Printf("hits: %i\n", hits.Num() );

/*
	// find the closest face-center to impact
	bestHit = hits[0];
	for ( i=1; i<hits.Num(); i++ ) {
		if ( ( geo.faces[ hits[i] ].center - lastiPoint ).Length() < ( geo.faces[ bestHit ].center - lastiPoint ).Length() ) {
			bestHit = hits[i];
		}
	}
*/
//	i=0;

	// if hit on the long side, break it
	//if ( geo.faces[i].area > geo.avgArea * (2/3) ) {
//		gameLocal.Printf("Break, side %i, area %f\n", i, geo.faces[i].area);

//		const idDeclParticle *splashParticleTiny = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, "water_body_splash_tiny" ) );
//		gameLocal.smokeParticles->EmitSmoke( splashParticleTiny, gameLocal.time, gameLocal.random.CRandomFloat(), lastiPoint, idAngles( 0, gameLocal.random.CRandomFloat() * 360, 0 ).ToMat3() );

		///	spawn two halves, touching where hit occured. same texture. do some math to figure out shape.
		///	spawn some shards with somewhat random vertices so they're shaped differently
		///	spawn some shards attached to board halves where hit occured (for splints)
		///	spawn some brittle fracture
		///	remove board
/*	} else {
		// just spawn some chipped debris
		gameLocal.Printf("Cip, side %i, area %f\n", i, geo.faces[i].area);
		///	if hit on the short side less than half the length of an adjacent side)
		///	spawn largest portion. same texture. do some math to figure out shape.
		///	spawn some brittle fracture
	}
*/


	// * amount of debris dependant upon size
	// * health dependant upon size



/*************************************************************************
Test code for trying to create clip models on the fly
*************************************************************************/

/**/ // remove clip model
//physicsObj.DisableClip();
//delete physicsObj.clipModel;
//physicsObj.clipModel = NULL;
//physicsObj.DisableImpact();


/** // create a new clip model
	//FreeModelDef();
	//UpdateVisuals();
	physicsObj.UnlinkClip();
	idTraceModel trm;
	float density, friction, bouncyness, mass;
	int clipShrink;
	idStr renderModelName = spawnArgs.GetString( "model" );
	// create clipModel from renderModel
	if ( !collisionModelManager->TrmFromModel( renderModelName, trm ) ) {
		gameLocal.Error( "idMoveable '%s': cannot load collision model %s", name.c_str(), renderModelName.c_str() );
		return;
	}
	//trm.bounds = suu->bounds = idBounds(min, max);
	// get rigid body properties
	spawnArgs.GetFloat( "density", "0.5", density );
	density = idMath::ClampFloat( 0.001f, 1000.0f, density );
//	spawnArgs.GetFloat( "friction", "0.05", friction );
//	friction = idMath::ClampFloat( 0.0f, 1.0f, friction );
//	spawnArgs.GetFloat( "bouncyness", "0.6", bouncyness );
//	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );
//	damage = spawnArgs.GetString( "def_damage", "" );
//	canDamage
//	minDamageVelocity = spawnArgs.GetFloat( "minDamageVelocity", "300" );	// _D3XP
//	maxDamageVelocity = spawnArgs.GetFloat( "maxDamageVelocity", "700" );	// _D3XP
	// setup the physics
	//physicsObj.clipModel
//	physicsObj.SetSelf( this );
	// ??? physicsObj.GetClipModel()->CheckModel( const char *name );
	//physicsObj.GetClipModel()->ClearTraceModelCache();
	//physicsObj.GetClipModel()->TraceModelCacheSize();
	//nooope physicsObj.SetClipModel( new idClipModel( trm ), density, 0, true );
	//physicsObj.SetClipModel( new idClipModel( modelDefHandle ), density, 0, true ); //nooope
	// noooope
	//int hnd = physicsObj.GetClipModel()->GetRenderModelHandle();
	//    physicsObj.SetClipModel( new idClipModel( hnd ), density, 0, true );
	/* nope
	idRenderModel *ass( renderEntity.hModel );
	renderEntity_t re(renderEntity);
	re.hModel = ass;
	physicsObj.SetClipModel( new idClipModel( gameRenderWorld->AddEntityDef( &re ) ), density, 0, true );
	*/
/*
	physicsObj.SetClipModel( new idClipModel( gameRenderWorld->AddEntityDef( &re ) ), density, 0, true );
	physicsObj.GetClipModel()->SetMaterial( GetRenderModelMaterial() );
	physicsObj.LinkClip();
*/

//	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
//	physicsObj.SetAxis( GetPhysics()->GetAxis() );
//	physicsObj.SetBouncyness( bouncyness );
//	physicsObj.SetFriction( 0.6f, 0.6f, friction );
//	physicsObj.SetGravity( gameLocal.GetGravity() );
//	physicsObj.SetContents( CONTENTS_SOLID );
//	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
//	SetPhysics( &physicsObj );

//		physicsObj.SetMass( mass );
//		physicsObj.DropToFloor();
//		physicsObj.DisableImpact();
//		BecomeNonSolid();
//	allowStep = spawnArgs.GetBool( "allowStep", "1" );

/** // update things again
	//UpdateModel();
	UpdateModelTransform();
	UpdateVisuals();
	//renderEntity.forceUpdate; // ?
**/


/*/ // method 2: remove surfaces, add new ones, 
	asds
	// allocate new surface ( contains all faces of model, init with previous model )
	modelSurface_t sur( *renderEntity.hModel->Surface(0) );
	// update the bounds
	// ...
	// resize the new surfaces
	for ( v=0; v < sur.geometry->numVerts; v++ ) {
		sur.geometry->verts[v].xyz /= 2;
	}
	// disappear the render model
//	renderEntity.hModel->PurgeModel();
//	srfTriangles_t tri;
//	for ( int i=0, vrt=0; i < sur.geometry->numVerts / 3; i++, vrt++ ) {
		//tri.vert
	//	sur.geometry->nums
	//	sur.geometry->facePlanes[
	//	renderEntity.hModel->FreeSurfaceTriangles( &tri );
/////		renderEntity.hModel->FreeSurfaceTriangles( renderEntity.hModel->Surface(0)->geometry );
//	}
// assign new surfaces
/// AllocSurfaceTriangles
//	renderEntity.hModel->AddSurface( sur );
/// FinishSurfaces
//	renderEntity.hModel->AddSurface( sur ); // crashed the game...
	//renderEntity.hModel->FinishSurfaces();
 // create the new clip model
	//idTraceModel nt( sur.geometry->bounds );
	physicsObj.GetClipModel()->LoadModel( sur.geometry->bounds;
/**/


/*************/
// this is how to create a renderEntity/renderModel
// cant create renderModel with static models
/*
	idRenderModel			*newmodel;
	renderEntity_t			ent;
	memset( &ent, 0, sizeof( ent ) );
	ent.bounds.Clear();
	ent.suppressSurfaceInViewID = 0;
	ent.numJoints = renderEntity.hModel->NumJoints();
	ent.joints = ( idJointMat * )Mem_Alloc16( ent.numJoints * sizeof( *ent.joints ) );
	newmodel = renderEntity.hModel->InstantiateDynamicModel( &ent, NULL, NULL );
	Mem_Free16( ent.joints );
	ent.joints = NULL;
	return;
	// newmodel;
/**************/


/*
	modelSurface_t sur( *renderEntity.hModel->Surface(0) );
	for ( v=0; v < sur.geometry->numVerts; v++ ) {
		sur.geometry->verts[v].xyz /= 2;
	}
	renderEntity.hModel->FreeSurfaceTriangles();
	//PurgeModel();
	renderEntity.hModel->AddSurface( sur );
	//renderEntity.hModel->FinishSurfaces();
	idTraceModel nt( renderEntity.hModel->Bounds() );
	physicsObj.GetClipModel()->LoadModel( nt );
	//renderEntity.hModel->InitEmpty( "something" );
	UpdateModel();
	UpdateModelTransform();
	UpdateVisuals();
	//renderEntity.forceUpdate;


	//physicsObj.GetClipModel()->LoadModel( modelDefHandle );
	// err: TraceModelForClipModel: clip model 0 on wood_2 is not a trace model

	//renderEntity.hModel = renderModelManager->AllocModel();
	//renderEntity.hModel->InitEmpty( brittleFracture_SnapshotName );
*/
}
#endif
