#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Effect.h"
#include "client/ClientEffect.h"

const idEventDef EV_LookAtTarget( "lookAtTarget", NULL );
const idEventDef EV_Attenuate( "attenuate", "f" );

CLASS_DECLARATION( idEntity, rvEffect )
	EVENT( EV_Activate,				rvEffect::Event_Activate )
	EVENT( EV_LookAtTarget,			rvEffect::Event_LookAtTarget )
	EVENT( EV_Earthquake,			rvEffect::Event_EarthQuake )
	EVENT( EV_Camera_Start,			rvEffect::Event_Start )
	EVENT( EV_Camera_Stop,			rvEffect::Event_Stop )
	EVENT( EV_Attenuate,			rvEffect::Event_Attenuate )
	EVENT( EV_IsActive,				rvEffect::Event_IsActive )
END_CLASS

/*
================
rvEffect::rvEffect
================
*/
rvEffect::rvEffect ( void ) {
	fl.networkSync = true;
	loop = false;
	lookAtTarget = false;
	effect = NULL;
	endOrigin.Zero();
}

/*
================
rvEffect::Spawn
================
*/
void rvEffect::Spawn( void ) {
	const char* fx;
	if ( !spawnArgs.GetString ( "fx", "", &fx ) || !*fx ) {
		if ( !( gameLocal.editors & EDITOR_FX ) ) {
			gameLocal.Warning ( "no effect file specified on effect entity '%s'", name.c_str() );
			PostEventMS ( &EV_Remove, 0 );
			return;
		}
	} else {
		effect = ( const idDecl * )declManager->FindEffect( spawnArgs.GetString ( "fx" ) );
		if( effect->IsImplicit() ) {
			common->Warning( "Unknown effect \'%s\' on entity \'%s\'", spawnArgs.GetString ( "fx" ), GetName() );
		}
	}
	
	spawnArgs.GetVector ( "endOrigin", "0 0 0", endOrigin );
	
	spawnArgs.GetBool ( "loop", "0", loop );

	// If look at target is set the effect will continually update itself to look at its target
	spawnArgs.GetBool( "lookAtTarget", "0", lookAtTarget );

	renderEntity.shaderParms[SHADERPARM_ALPHA] = spawnArgs.GetFloat ( "_alpha", "1" );
	renderEntity.shaderParms[SHADERPARM_BRIGHTNESS] = spawnArgs.GetFloat ( "_brightness", "1" );

    if( spawnArgs.GetBool( "start_on", loop ? "1" : "0" ) ) {
		ProcessEvent( &EV_Activate, this );
	}		
#if 0
	// If anyone ever gets around to a flood fill from the origin rather than the over generous PushVolumeIntoTree bounds,
	// this warning will become useful. Until then, it's a bogus warning.
	if( gameRenderWorld->PointInArea( GetPhysics()->GetOrigin() ) < 0 ) {
		common->Warning( "Effect \'%s\' out of world", name.c_str() );
	}
#endif
}

/*
================
rvEffect::Think
================
*/
void rvEffect::Think( void ) {
	
	if( clientEntities.IsListEmpty ( ) ) {
		BecomeInactive( TH_THINK );

		// Should the func_fx be removed now?
		if( !(gameLocal.editors & EDITOR_FX) && spawnArgs.GetBool( "remove" ) ) {		
			PostEventMS( &EV_Remove, 0 );
		} 
		
		return;
	}	
	else if( lookAtTarget ) {
		// If activated and looking at its target then update the target information
		ProcessEvent( &EV_LookAtTarget );
	}	

	UpdateVisuals();
	Present ( );
}

/*
================
rvEffect::Save
================
*/
void rvEffect::Save ( idSaveGame *savefile ) const {
	savefile->WriteBool ( loop );
	savefile->WriteBool ( lookAtTarget );
	savefile->WriteString ( effect->GetName() );
	savefile->WriteVec3 ( endOrigin );
	clientEffect.Save ( savefile );
}

/*
================
rvEffect::Restore
================
*/
void rvEffect::Restore ( idRestoreGame *savefile ) {
	idStr	name;

	savefile->ReadBool ( loop );
	savefile->ReadBool ( lookAtTarget );
	savefile->ReadString ( name );
	effect = declManager->FindType( DECL_EFFECT, name );
	savefile->ReadVec3 ( endOrigin );
	clientEffect.Restore ( savefile );
}

/*
================
rvEffect::Think
================
*/
void rvEffect::Stop( bool destroyParticles ) {
	StopEffect ( effect, destroyParticles );
}

/*
================
rvEffect::Play
================
*/
bool rvEffect::Play( void ) {
	clientEffect = PlayEffect ( effect, renderEntity.origin, renderEntity.axis, loop, endOrigin );
	if ( clientEffect ) {

		idVec4 color;
		color[0] = renderEntity.shaderParms[SHADERPARM_RED];
		color[1] = renderEntity.shaderParms[SHADERPARM_GREEN];
		color[2] = renderEntity.shaderParms[SHADERPARM_BLUE];
		color[3] = renderEntity.shaderParms[SHADERPARM_ALPHA];
		clientEffect->SetColor ( color );
		clientEffect->SetBrightness ( renderEntity.shaderParms[ SHADERPARM_BRIGHTNESS ] );
		clientEffect->SetAmbient( true );

		BecomeActive ( TH_THINK );
		return true;
	}
	
	return false;
}

/*
================
rvEffect::Attenuate
================
*/
void rvEffect::Attenuate ( float attenuation ) {
	rvClientEntity* cent;
	for( cent = clientEntities.Next(); cent != NULL; cent = cent->spawnNode.Next() ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( cent->IsType ( rvClientEffect::GetClassType() ) ) {
// RAVEN END
			static_cast<rvClientEffect*>(cent)->Attenuate ( attenuation );
		}
	}			
}

/*
================
rvEffect::Restart
================
*/
void rvEffect::Restart( void ) {
	Stop( false );	
	
	if( loop )	{
		Play();
	}
}

/*
================
rvEffect::UpdateChangeableSpawnArgs
================
*/
void rvEffect::UpdateChangeableSpawnArgs( const idDict *source ) {
	const char* fx;
	const idDecl *newEffect;
	bool		newLoop;

	idEntity::UpdateChangeableSpawnArgs(source);
	if ( !source ) {
		return;
	}

	if ( source->GetString ( "fx", "", &fx ) && *fx ) {
		newEffect = ( const idDecl * )declManager->FindEffect( fx );
	} else {
		newEffect = NULL;
	}

	idVec3 color;
	source->GetVector( "_color", "1 1 1", color );
	renderEntity.shaderParms[ SHADERPARM_RED ]	 = color[0];
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = color[1];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	 = color[2];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = source->GetFloat ( "_alpha", "1" );
	renderEntity.shaderParms[ SHADERPARM_BRIGHTNESS ] = source->GetFloat ( "_brightness", "1" );
	if ( clientEffect ) {		
		clientEffect->SetColor ( idVec4(color[0],color[1],color[2],renderEntity.shaderParms[ SHADERPARM_ALPHA ]) );
		clientEffect->SetBrightness ( renderEntity.shaderParms[ SHADERPARM_BRIGHTNESS ] );
	}

	source->GetBool ( "loop", "0", newLoop );

	spawnArgs.Copy( *source );
	
	// IF the effect handle has changed or the loop status has changed then restart the effect
	if ( newEffect != effect || loop != newLoop ) {
		Stop ( false );		
	
		loop = newLoop;
		effect = newEffect;

		if ( effect ) {
			Play ( );
			BecomeActive( TH_THINK );
			UpdateVisuals();
		} else {
			BecomeInactive ( TH_THINK );
			UpdateVisuals();
		}
	}
}

/*
===============
rvEffect::ShowEditingDialog
===============
*/
void rvEffect::ShowEditingDialog( void ) {
	common->InitTool( EDITOR_FX, &spawnArgs );
}

/*
=================
rvEffect::WriteToSnapshot
=================
*/
void rvEffect::WriteToSnapshot( idBitMsgDelta &msg ) const {
	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
	idGameLocal::WriteDecl( msg, effect );
	msg.WriteBits( loop, 1 );
}

/*
=================
rvEffect::ReadFromSnapshot
=================
*/
void rvEffect::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	const idDecl *old = effect;
	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );
	
	effect = idGameLocal::ReadDecl( msg, DECL_EFFECT );
	loop = ( msg.ReadBits( 1 ) != 0 );

	if ( effect && !old ) {
		// TODO: need to account for when the effect really started
		Play();
	}
}

/*
=================
rvEffect::ClientPredictionThink
=================
*/
void rvEffect::ClientPredictionThink( void ) {
	if ( gameLocal.isNewFrame ) {	 
		Think ( );
	}
	RunPhysics();
	Present();
}

/*
================
rvEffect::Event_Start
================
*/
void rvEffect::Event_Start ( void ) {
	if( !effect || !clientEntities.IsListEmpty ( ) ) {
		return;
	}

	if( !Play() ) {
		if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
			// no effects on dedicated server
		} else {
			gameLocal.Warning( "Unable to play effect '%s'", effect->GetName() );
		}
		BecomeInactive ( TH_THINK );
	}

	ProcessEvent( &EV_LookAtTarget );
}

/*
================
rvEffect::Event_Stop
================
*/
void rvEffect::Event_Stop ( void ) {
	if( !effect ) {
		return;
	}

	Stop( false );
}

/*
=================
rvEffect::Event_Activate
=================
*/
void rvEffect::Event_Activate( idEntity *activator ) {
	// Stop the effect if its already playing
	if( !clientEntities.IsListEmpty ( ) ) {
		Event_Stop ( );
	} else {
		Event_Start ( );
	}

	ActivateTargets( activator );
}

/*
================
rvEffect::Event_LookAtTarget

Reorients the effect entity towards its target and sets the end origin as well
================
*/
void rvEffect::Event_LookAtTarget ( void ) {
	const idKeyValue	*kv;
	idVec3				dir;		

	if ( !effect || !clientEffect ) {
		return;
	}

	kv = spawnArgs.MatchPrefix( "target", NULL );
	while( kv ) {
		idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
		if( ent ) {
			if( !idStr::Icmp( ent->GetEntityDefName(), "target_null" ) ) {
				dir = ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
				dir.Normalize();
				
				clientEffect->SetEndOrigin ( ent->GetPhysics()->GetOrigin() );
				clientEffect->SetAxis ( dir.ToMat3( ) );
				return;						
			}
		}
		kv = spawnArgs.MatchPrefix( "target", kv );
	}
}

/*
================
rvEffect::Event_EarthQuake
================
*/
void rvEffect::Event_EarthQuake ( float requiresLOS ) {
	float quakeChance;

	if ( !spawnArgs.GetFloat("quakeChance", "0", quakeChance) ) {
		return;
	}
	
	if ( rvRandom::flrand(0, 1.0f) > quakeChance ) {
		// failed its activation roll
		return;
	}
	
	if ( requiresLOS ) {
		// if the player doesn't have line of sight to this fx, don't do anything
		trace_t		trace;
		idPlayer	*player = gameLocal.GetLocalPlayer();
		idVec3		viewOrigin;
		idMat3		viewAxis;

		player->GetViewPos(viewOrigin, viewAxis);
// RAVEN BEGIN
// ddynerman: multiple collision worlds
		gameLocal.TracePoint( this, trace, viewOrigin, GetPhysics()->GetOrigin(), MASK_OPAQUE, player );
// RAVEN END
		if (trace.fraction < 1.0f)
		{
			// something blocked LOS
			return;
		}
	}
	
	// activate this effect now
	ProcessEvent ( &EV_Activate, gameLocal.entities[ENTITYNUM_WORLD] );
}

/*
================
rvEffect::Event_Attenuate
================
*/
void rvEffect::Event_Attenuate( float attenuation ) {
	Attenuate( attenuation );
}

/*
================
rvEffect::Event_Attenuate
================
*/
void rvEffect::Event_IsActive( void ) {
	idThread::ReturnFloat( ( !effect || !clientEntities.IsListEmpty() ) ? 0.0f : 1.0f );
}

/*
================
rvEffect::InstanceLeave
================
*/
void rvEffect::InstanceLeave( void ) {
	idEntity::InstanceLeave();
	Stop( true );	
}

/*
================
rvEffect::InstanceJoin
================
*/
void rvEffect::InstanceJoin( void ) {
	idEntity::InstanceJoin();

	Restart();
}
