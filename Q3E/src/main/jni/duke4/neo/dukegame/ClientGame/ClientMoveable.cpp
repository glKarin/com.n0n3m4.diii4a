//----------------------------------------------------------------
// ClientMoveable.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../Gamelib/Game_local.h"

/*
===============================================================================

rvClientMoveable

===============================================================================
*/
const idEventDef CL_FadeOut( "<fadeOut>", "d" );
const idEventDef CL_ClearDepthHack ( "<clearDepthHack>" );

static const float BOUNCE_SOUND_MIN_VELOCITY	= 100.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;
static const int   BOUNCE_SOUND_DELAY			= 200;

CLASS_DECLARATION( rvClientEntity, rvClientMoveable )
	EVENT( CL_FadeOut,			rvClientMoveable::Event_FadeOut )
	EVENT( CL_ClearDepthHack,	rvClientMoveable::Event_ClearDepthHack )
END_CLASS


/*
================
rvClientMoveable::rvClientMoveable
================
*/
rvClientMoveable::rvClientMoveable ( void ) {
	memset ( &renderEntity, 0, sizeof(renderEntity) );
	entityDefHandle = -1;
	scale.Init( 0, 0, 1.0f, 1.0f );
}

/*
================
rvClientMoveable::~rvClientMoveable
================
*/
rvClientMoveable::~rvClientMoveable ( void ) {	
	gameLocal.clientGamePhysicsMutex.Lock();
		FreeEntityDef();
		gameLocal.UnregisterClientEntity(this);
		gameLocal.clientEntityThreadWork.Remove(this);
	gameLocal.clientGamePhysicsMutex.Unlock();

	// Remove any trail effect if there is one
	//if ( trailEffect ) {
	//	trailEffect->Stop ( );
	//}
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

idVec3 simpleTri[3] =
{
	idVec3( -1.0, -1.0, 0.0 ),
	idVec3( 0.0, 2.0, 0.0 ),
	idVec3( 2.0, 0.0, 0.0 )
};

/*
================
rvClientMoveable::Spawn
================
*/
void rvClientMoveable::Spawn ( void ) {
	// parse static models the same way the editor display does
	gameLocal.ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );

	//idTraceModel	trm;
	int				clipShrink;
	idStr			clipModelName;

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if ( !clipModelName.Length ()  ) {
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	//if ( clipModelName == SIMPLE_TRI_NAME ) {
	//	trm.SetupPolygon( simpleTri, 3 );
	//} else {
	//	clipModelName.BackSlashesToSlashes();
	//
	//	if ( !collisionModelManager->TrmFromModel( gameLocal.GetMapName(), trm ) ) {
	//		gameLocal.Error( "rvClientMoveable '%d': cannot load collision model %s", entityNumber, clipModelName.c_str() );
	//		return;
	//	}
	//}
	//
	//// if the model should be shrunk
	//clipShrink = spawnArgs.GetInt( "clipshrink" );
	//if ( clipShrink != 0 ) {
	//	trm.Shrink( clipShrink * CM_CLIP_EPSILON );
	//}

	idRenderModel* model = renderModelManager->FindModel(clipModelName);

	physicsObj.SetSelf ( gameLocal.entities[ENTITYNUM_CLIENT] );		
	physicsObj.SetClipModel ( new idClipModel(model->Bounds()), spawnArgs.GetFloat ( "density", "0.5" ), entityNumber );

	physicsObj.SetOrigin( GetOrigin() );
	physicsObj.SetAxis( GetAxis() );
	physicsObj.SetBouncyness( spawnArgs.GetFloat( "bouncyness", "0.6" ) );
	physicsObj.SetFriction( spawnArgs.GetFloat("linear_friction", "0.6"), spawnArgs.GetFloat( "angular_friction", "0.6"), spawnArgs.GetFloat("friction", "0.05") );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( 0 );
	// abahr: changed to MASK_SHOT_RENDERMODEL because brass was getting pinched between the player and the wall in some cases
	//			may want to try something cheaper.
	physicsObj.SetClipMask( CONTENTS_OPAQUE ); // MASK_SHOT_RENDERMODEL | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP | CONTENTS_WATER );
	physicsObj.Activate ( );

	//trailEffect = gameLocal.PlayEffect ( spawnArgs, "fx_trail", physicsObj.GetCenterMass(), GetAxis(), true );	
	trailAttenuateSpeed = spawnArgs.GetFloat ( "trailAttenuateSpeed", "200" );
	
	bounceSoundShader = declManager->FindSound ( spawnArgs.GetString ( "snd_bounce" ), false );
	bounceSoundTime   = 0;
	mPlayBounceSoundOnce = spawnArgs.GetBool("bounce_sound_once");
	mHasBounced = false;
	
	scale.Init( gameLocal.GetTime(), SEC2MS(spawnArgs.GetFloat("scale_reset_duration", "0.2")), Max(VECTOR_EPSILON, spawnArgs.GetFloat("scale", "1.0f")), 1.0f );
}

/*
================
rvClientMoveable::SetOrigin
================
*/
void rvClientMoveable::SetOrigin( const idVec3& origin ) {
	rvClientEntity::SetOrigin( origin );
	physicsObj.SetOrigin( origin );
}

/*
================
rvClientMoveable::SetAxis
================
*/
void rvClientMoveable::SetAxis( const idMat3& axis ) {
	rvClientEntity::SetAxis( axis );
	physicsObj.SetAxis( axis );
}

/*
================
rvClientMoveable::SetOwner
================
*/
void rvClientMoveable::SetOwner( idEntity* owner ) {
	physicsObj.GetClipModel()->SetOwner( owner );
	physicsObj.GetClipModel()->SetEntity( owner );
}

/*
================
rvClientMoveable::Think
================
*/
void rvClientMoveable::Think ( void ) {
	if( bindMaster && (bindMaster->GetRenderEntity()->hModel && bindMaster->GetModelDefHandle() == -1) ) {
		return;
	}

	RunPhysics ( );	

	// Special case the sound update to use the center mass since the origin may be in an odd place
	//idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	//if ( emitter ) {
	//	refSound.origin = worldOrigin;
	//	refSound.velocity = worldVelocity;
	//	emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
	//}

	renderEntity.origin = worldOrigin;
	renderEntity.axis = worldAxis * scale.GetCurrentValue( gameLocal.GetTime() );

	//gameRenderWorld->DebugBox(colorWhite,idBox(renderEntity.origin, idVec3(10, 10, 10), mat3_identity));

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
	if (mPlayBounceSoundOnce && mHasBounced)
	{
		return false;
	}
	if ( bounceSoundShader && gameLocal.time > bounceSoundTime ) {
		float speed;
		speed = velocity.LengthFast ( );
		if ( speed > BOUNCE_SOUND_MIN_VELOCITY ) {
			StartSoundShader ( bounceSoundShader, SND_CHANNEL_BODY, 0 );
			bounceSoundTime = BOUNCE_SOUND_DELAY;
			mHasBounced = true;
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
	renderEntity.noShadow = true;
	renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
	PostEventMS ( &EV_Remove, duration );	
}

/*
================
rvClientMoveable::Event_ClearDepthHack
================
*/
void rvClientMoveable::Event_ClearDepthHack ( void ) {
	renderEntity.weaponDepthHack = true;
}

/*
================
rvClientMoveable::SpawnClientMoveables
================
*/
void rvClientMoveable::SpawnClientMoveables( idEntity* ent, const char *type, idList<rvClientMoveable *>* list ) {
	const idKeyValue *kv;
	idVec3 origin;
	idMat3 axis;
	
	if( list == NULL || type == NULL ) {
		return;
	}

	// drop all items
	kv = ent->spawnArgs.MatchPrefix( va( "def_%s", type ), NULL );
	while ( kv ) {
		origin = ent->GetPhysics()->GetOrigin();
		axis = ent->GetPhysics()->GetAxis();

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if( ent->IsType( idAnimatedEntity::Type ) ) {
// RAVEN END
			idAnimatedEntity* animEnt = static_cast<idAnimatedEntity*>(ent);
			jointHandle_t clientMoveableJoint;
	
			const char* clientMoveableJointName = ent->spawnArgs.GetString( va( "%s_joint", kv->GetKey().c_str() + 4 ) );
		
			// use a joint if specified
			if ( idStr::Icmp( clientMoveableJointName, "") ) {
				clientMoveableJoint = animEnt->GetAnimator()->GetJointHandle( clientMoveableJointName );

				if ( !animEnt->GetJointWorldTransform( clientMoveableJoint, gameLocal.time, origin, axis ) ) {
					gameLocal.Warning( "%s refers to invalid joint '%s' on entity '%s'\n", va( "%s_joint", kv->GetKey().c_str() + 4 ), clientMoveableJointName, ent->name.c_str() );
					origin = ent->GetPhysics()->GetOrigin();
					axis = ent->GetPhysics()->GetAxis();
				}
			} 
		} 

		// spawn the entity
		const idDict* entityDef = gameLocal.FindEntityDefDict ( kv->GetValue().c_str(), false );
	
		if ( entityDef == NULL ) {
			gameLocal.Warning( "%s refers to invalid entity def '%s' on entity '%s'\n", kv->GetKey().c_str(), kv->GetValue().c_str(), ent->name.c_str() );
			break;
		}
	
		rvClientMoveable* newModel = NULL;
		// force spawnclass to rvClientMoveable
		gameLocal.SpawnClientEntityDef( *entityDef, (rvClientEntity**)(&newModel), false, "rvClientMoveable" );

		if( !newModel ) {
			gameLocal.Warning( "error spawning client moveable (invalid entity def '%s' on entity '%s')\n", kv->GetValue().c_str(), ent->name.c_str() );
			break;
		}
		newModel->SetOrigin ( origin );
		newModel->SetAxis( axis );

		list->Append( newModel );
		kv = ent->spawnArgs.MatchPrefix( va( "def_%s", type ), kv );
	}
}

