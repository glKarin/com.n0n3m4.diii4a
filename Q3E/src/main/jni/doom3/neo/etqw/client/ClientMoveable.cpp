// Copyright (C) 2007 Id Software, Inc.
//

//----------------------------------------------------------------
// ClientMoveable.cpp
//----------------------------------------------------------------

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ClientMoveable.h"
#include "../ContentMask.h"
#include "../demos/DemoManager.h"

/*
===============================================================================

rvClientMoveable

===============================================================================
*/
const idEventDefInternal CL_FadeOut( "internal_fadeOut", "d" );

static const float BOUNCE_SOUND_MIN_VELOCITY	= 100.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;
static const int   BOUNCE_SOUND_DELAY			= 200;

CLASS_DECLARATION( rvClientEntity, rvClientMoveable )
	EVENT( CL_FadeOut,			rvClientMoveable::Event_FadeOut )
END_CLASS


/*
================
rvClientMoveable::rvClientMoveable
================
*/
rvClientMoveable::rvClientMoveable ( void ) {
	memset ( &renderEntity, 0, sizeof(renderEntity) );
	entityDefHandle = -1;
	aorLayout = NULL;
}

/*
================
rvClientMoveable::~rvClientMoveable
================
*/
rvClientMoveable::~rvClientMoveable ( void ) {
	FreeEntityDef ( );
	gameEdit->DestroyRenderEntity( renderEntity );

	// Remove any trail effect if there is one
	if ( trailEffect ) {
		trailEffect->Stop ( );
	}
}

/*
================
rvClientMoveable::FreeEntityDef
================
*/
void rvClientMoveable::FreeEntityDef ( void ) {
	if ( entityDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef ( entityDefHandle );
		entityDefHandle = -1;
	}	
}

/*
================
rvClientMoveable::Spawn
================
*/
void rvClientMoveable::Spawn( const idDict* args, int _effectSet ) {
	float bouncyness = 0.5f;
	float density    = 0.5f;

	spawnArgs = args;
	assert( spawnArgs );

	effectSet = _effectSet;

	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( *spawnArgs, renderEntity );

	idTraceModel	trm;
	int				clipShrink;
	
	
	const char* clipModelName = spawnArgs->GetString( "cm_model" );
	if ( !gameLocal.clip.LoadTraceModel( clipModelName, trm ) ) {
		gameLocal.Error( "rvClientMoveable '%d': cannot load collision model %s", entityNumber, clipModelName );
		return;
	}

	// if the model should be shrunk
	clipShrink = spawnArgs->GetInt( "clipshrink" );
	if ( clipShrink != 0 ) {
		trm.Shrink( clipShrink * CM_CLIP_EPSILON );
	}

	physicsObj.SetSelf( gameLocal.entities[ENTITYNUM_CLIENT] );		
	physicsObj.SetClipModel( new idClipModel( trm, false ), spawnArgs->GetFloat ( "density", "0.5" ), 0 );
	physicsObj.SetOrigin( worldOrigin );
	physicsObj.SetAxis( worldAxis );
	physicsObj.SetBouncyness( spawnArgs->GetFloat( "bouncyness", "0.6" ) );
	physicsObj.SetBuoyancy( spawnArgs->GetFloat( "buoyancy", "0.3" ) );

	aorLayout = gameLocal.declAORType[ spawnArgs->GetString( "aor_layout", "missile" ) ];

	float friction = spawnArgs->GetFloat( "friction", "0.05" );

	physicsObj.SetFriction( 0.6f, 0.6f, friction );
	float gravity;
	if ( spawnArgs->GetFloat( "gravity", "0", gravity ) ) {
		physicsObj.SetGravity( idVec3( 0.0f, 0.0f, -gravity ) );
	} else {
		physicsObj.SetGravity( gameLocal.GetGravity() );
	}

	physicsObj.SetContents( 0, 0 );
	// TODO: These contents flags can surely be done using masks....
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_SLIDEMOVER | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP | CONTENTS_WATER | CONTENTS_FORCEFIELD, 0 );
	physicsObj.Activate();

	idVec3 center = physicsObj.GetBounds().GetCenter() * worldAxis + worldOrigin;
	const char* trailEffectName = "fx_trail";
	if ( effectSet != 0 ) {
		trailEffectName = va( "fx_trail_%i", effectSet );
	}
	trailEffect = gameLocal.PlayEffect( *spawnArgs, colorWhite.ToVec3(), trailEffectName, NULL, center, worldAxis, true );	
	trailAttenuateSpeed = spawnArgs->GetFloat ( "trailAttenuateSpeed", "200" );
	
	bounceSoundShader = gameLocal.declSoundShaderType[ spawnArgs->GetString( "snd_bounce" ) ];
	bounceSoundTime   = 0;

	firstBounce = true;
}

/*
================
rvClientMoveable::Think
================
*/
void rvClientMoveable::Think ( void ) {
	bool runPhysics = true;
	if ( gameLocal.SetupClientAoR() ) {
		int aorFlags = gameLocal.aorManager.GetFlagsForPoint( aorLayout, worldOrigin );
		if ( ( aorFlags & AOR_INHIBIT_PHYSICS ) != 0 ) {
			runPhysics = false;
		}
	}
	
	if ( runPhysics ) {
		RunPhysics();
	}

	// Special case the sound update to use the center mass since the origin may be in an odd place
	if ( refSound.referenceSound ) {
		refSound.origin = worldOrigin;
		refSound.referenceSound->UpdateEmitter( refSound.origin, refSound.listenerId, &refSound.parms );
	}
	
	renderEntity.origin = worldOrigin;
	renderEntity.axis = worldAxis;

	// Keep the trail effect following
	if ( trailEffect ) {
		float speed;
		speed = idMath::ClampFloat ( 0, trailAttenuateSpeed, physicsObj.GetLinearVelocity ( ).LengthFast ( ) );
		if ( physicsObj.IsAtRest ( ) ) {
			trailEffect->Stop ( );
			trailEffect = NULL;
		} else {
			idVec3 center = physicsObj.GetBounds().GetCenter() * worldAxis + worldOrigin;
			trailEffect->SetOrigin ( center );
			trailEffect->SetAxis ( worldAxis );
			trailEffect->Attenuate ( speed / trailAttenuateSpeed );
		}
	}

	// add to refresh list
	if ( entityDefHandle == -1 ) {
		entityDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( entityDefHandle, &renderEntity );
	}		
}

/*
================
rvClientMoveable::GetPhysics
================
*/
idPhysics* rvClientMoveable::GetPhysics ( void ) const {
	return (idPhysics*)&physicsObj;
}

/*
================
rvClientMoveable::Collide
================
*/
bool rvClientMoveable::Collide ( const trace_t &collision, const idVec3 &velocity ) {	
	if ( firstBounce ) {
		// first bounce, play effect
		const char* bounceEffectName = "fx_firstbounce";
		if ( effectSet != 0 ) {
			bounceEffectName = va( "fx_firstbounce_%i", effectSet );
		}
		const idVec3& origin = physicsObj.GetOrigin();
		idMat3 axis = ( idVec3( 0.0f, 0.0f, 1.0f ) ).ToMat3();
		gameLocal.PlayEffect( *spawnArgs, colorWhite.ToVec3(), bounceEffectName, NULL, origin, axis );
		firstBounce = false;
	}

	if ( bounceSoundShader && gameLocal.time > bounceSoundTime ) {
		float speed;
		speed = velocity.LengthFast ( );
		if ( speed > BOUNCE_SOUND_MIN_VELOCITY ) {
			StartSoundShader ( bounceSoundShader, SND_ANY, 0 );
			bounceSoundTime = BOUNCE_SOUND_DELAY;
		}
	}	
		
	return false;
}

/*
================
rvClientMoveable::Event_FadeOut
================
*/
void rvClientMoveable::Event_FadeOut ( int duration ) {
	renderEntity.flags.noShadow = true;
	renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
	PostEventMS ( &EV_Remove, duration );
}
