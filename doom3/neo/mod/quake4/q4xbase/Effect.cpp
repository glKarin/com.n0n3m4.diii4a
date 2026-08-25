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
void rvEffect::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( loop );
	savefile->WriteBool( lookAtTarget );
	savefile->WriteString( effect->GetName() );
	savefile->WriteVec3( endOrigin );
	clientEffect.Save( savefile );
}

/*
================
rvEffect::Restore
================
*/
void rvEffect::Restore( idRestoreGame *savefile ) {
	idStr	name;

	savefile->ReadBool( loop );
	savefile->ReadBool( lookAtTarget );
	savefile->ReadString( name );
	effect = declManager->FindType( DECL_EFFECT, name );
	savefile->ReadVec3( endOrigin );
	clientEffect.Restore( savefile );
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



// Awakening BEGIN
class riFireFX : public idEntity {
public:

	CLASS_PROTOTYPE( riFireFX );

					riFireFX					( void );
	void			Spawn						( void );
	void			Think						( void );
	void			ClientPredictionThink		( void );
	void			Save						( idSaveGame *savefile ) const;
	void			Restore						( idRestoreGame *savefile );

	bool			Play						( void );
	void			Stop						( bool destroyParticles = false );
	void				Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

private:
	void			Event_Activate		( idEntity *activator );
	void			Event_LookAtTarget	( void );
	void			Event_Start			( void );
	void			Event_Stop			( void );
	void			Event_Touch			( idEntity *other, trace_t *trace );

	const idDecl *	CalcCurrentEffect	(void);
	bool			UpdateStage			(void);
	void			ExecuteStage		(void);

	enum
	{
		ST_INIT = 0,
		ST_LARGE,
		ST_SMALL,
		ST_SMOKE,
	};

private:
	const idDecl						*fireLargeEffect;
	const idDecl						*fireSmallEffect;
	const idDecl						*smokeEffect;
	const idDecl						*effect;
	float								damageDelay; // milliseconds
	bool								on;
	idStr								fxtarget;
	const idDecl *						damage;
	rvClientEntityPtr<rvClientEffect>	clientEffect;

	int									stage;
	int									nextTime;
};

CLASS_DECLARATION( idEntity, riFireFX )
	EVENT( EV_Activate,				riFireFX::Event_Activate )
	EVENT( EV_LookAtTarget,			riFireFX::Event_LookAtTarget )
	EVENT( EV_Touch,				riFireFX::Event_Touch )
END_CLASS

/*
================
riFireFX::riFireFX
================
*/
riFireFX::riFireFX ( void ) {
	fl.networkSync = true;
	fl.takedamage = true;

	fireLargeEffect = NULL;
	fireSmallEffect = NULL;
	smokeEffect = NULL;
	effect = NULL;

	damageDelay = 1000;
	on = true;
	damage = NULL;

	stage = ST_INIT;
	nextTime = 0;
}

/*
================
riFireFX::Spawn
================
*/
void riFireFX::Spawn( void ) {
	const char* fx;

	fx = spawnArgs.GetString ( "fx_fireLarge", "effects/fire/gasjet_long.fx" );
	fireLargeEffect = ( const idDecl * )declManager->FindEffect( fx );
	if( fireLargeEffect->IsImplicit() ) {
		common->Warning( "Unknown large fire effect '%s' on entity '%s'", fx, GetName() );
	}
	
	fx = spawnArgs.GetString ( "fx_fireSmall", "effects/fire/gasjet.fx" );
	fireSmallEffect = ( const idDecl * )declManager->FindEffect( fx );
	if( fireSmallEffect->IsImplicit() ) {
		common->Warning( "Unknown small fire effect '%s' on entity '%s'", fx, GetName() );
	}
	
	fx = spawnArgs.GetString ( "fx_smoke", "effects/smoke/smolder1.fx" );
	smokeEffect = ( const idDecl * )declManager->FindEffect( fx );
	if( smokeEffect->IsImplicit() ) {
		common->Warning( "Unknown smoke effect '%s' on entity '%s'", fx, GetName() );
	}
	
	spawnArgs.GetFloat("damageDelay", "1000", damageDelay);
	spawnArgs.GetBool("on", "1", on);

	fx = spawnArgs.GetString ( "def_damage", "damage_painTrigger" );
	damage = ( const idDecl * )declManager->FindType( DECL_ENTITYDEF, fx );
	if( damage->IsImplicit() ) {
		common->Warning( "Unknown damage '%s' on entity '%s'", fx, GetName() );
	}

	spawnArgs.GetString( "fxtarget", "", fxtarget );

	nextTime = gameLocal.time;

	stage = ST_INIT;

	UpdateStage();

    if( on ) {
		ProcessEvent( &EV_Activate, this );
	}		
}
/*
================
riFireFX::Think
================
*/
void riFireFX::Think( void ) {
	
	if( clientEntities.IsListEmpty ( ) ) {
		BecomeInactive( TH_THINK );

		// Should the func_fx be removed now?
		if( !(gameLocal.editors & EDITOR_FX) && spawnArgs.GetBool( "remove" ) ) {		
			PostEventMS( &EV_Remove, 0 );
		} 
		
		return;
	}	
	else if( !fxtarget.IsEmpty() ) {
		// If activated and looking at its target then update the target information
		ProcessEvent( &EV_LookAtTarget );
	}	

	if (UpdateStage())
		ExecuteStage();

	UpdateVisuals();
	Present ( );
}


/*
================
riFireFX::Save
================
*/
void riFireFX::Save( idSaveGame *savefile ) const {
	savefile->WriteString( fireLargeEffect->GetName() );
	savefile->WriteString( fireSmallEffect->GetName() );
	savefile->WriteString( smokeEffect->GetName() );
	savefile->WriteFloat( damageDelay );
	savefile->WriteBool( on );
	savefile->WriteString( fxtarget );
	savefile->WriteString( damage->GetName() );
	savefile->WriteInt( nextTime );

	clientEffect.Save( savefile );
}

/*
================
riFireFX::Restore
================
*/
void riFireFX::Restore( idRestoreGame *savefile ) {
	idStr	name;

	savefile->ReadString( name );
	fireLargeEffect = (const idDecl *)declManager->FindEffect( name );
	savefile->ReadString( name );
	fireSmallEffect = (const idDecl *)declManager->FindEffect( name );
	savefile->ReadString( name );
	smokeEffect = (const idDecl *)declManager->FindEffect( name );
	savefile->ReadFloat( damageDelay );
	savefile->ReadBool( on );
	savefile->ReadString( fxtarget );
	savefile->ReadString( name );
	damage = declManager->FindType( DECL_ENTITYDEF, name );
	savefile->ReadInt( nextTime );

	clientEffect.Restore( savefile );

	stage = ST_INIT;
}

/*
=================
riFireFX::ClientPredictionThink
=================
*/
void riFireFX::ClientPredictionThink( void ) {
	if ( gameLocal.isNewFrame ) {	 
		Think ( );
	}
	RunPhysics();
	Present();
}

/*
================
riFireFX::Event_LookAtTarget

Reorients the effect entity towards its target and sets the end origin as well
================
*/
void riFireFX::Event_LookAtTarget ( void ) {
	idVec3				dir;		

	if ( /*!effect ||*/ !clientEffect || fxtarget.IsEmpty() ) {
		return;
	}

	idEntity *ent = gameLocal.FindEntity( fxtarget );
	if( ent ) {
		if( !idStr::Icmp( ent->GetEntityDefName(), "target_null" ) ) {
			dir = ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			dir.Normalize();

			clientEffect->SetEndOrigin ( ent->GetPhysics()->GetOrigin() );
			clientEffect->SetAxis ( dir.ToMat3( ) );
		}
	}
}

/*
================
riFireFX::Think
================
*/
void riFireFX::Stop( bool destroyParticles ) {
	StopEffect ( effect, destroyParticles );
}

const idDecl * riFireFX::CalcCurrentEffect(void)
{
	switch (stage)
	{
		case ST_LARGE: return fireLargeEffect;
		case ST_SMALL: return fireSmallEffect;
		case ST_SMOKE: default: return smokeEffect;
	}
}

bool riFireFX::UpdateStage(void)
{
	int st;
	int fullHealth = spawnArgs.GetInt( "health" );
	if (fullHealth > 0)
	{
		if(health > fullHealth / 2)
			st = ST_LARGE;
		else if(health > 0)
			st = ST_SMALL;
		else
			st = ST_SMOKE;
	}
	else
		st = ST_SMOKE;

	if (stage != st)
	{
		stage = st;
		return true;
	}
	else
		return false;
}

void riFireFX::ExecuteStage(void)
{
	if (stage != ST_INIT)
	{
		// update effect if changed
		const idDecl *e = CalcCurrentEffect();
		if(e != effect)
		{
			Event_Stop();
			Event_Start();
		}
		// clean clip
		if (stage == ST_SMOKE)
		{
			GetPhysics()->SetClipModel(NULL, 1.0f);
			//GetPhysics()->DisableClip();
		}
	}
}

/*
================
riFireFX::Play
================
*/
bool riFireFX::Play( void ) {
	clientEffect = PlayEffect ( effect, renderEntity.origin, renderEntity.axis, true, vec3_zero );
	if ( clientEffect ) {

		/*
		idVec4 color = vec4_one;
		color[0] = renderEntity.shaderParms[SHADERPARM_RED];
		color[1] = renderEntity.shaderParms[SHADERPARM_GREEN];
		color[2] = renderEntity.shaderParms[SHADERPARM_BLUE];
		color[3] = renderEntity.shaderParms[SHADERPARM_ALPHA];
		clientEffect->SetColor ( color );
		clientEffect->SetBrightness ( renderEntity.shaderParms[ SHADERPARM_BRIGHTNESS ] );
		*/
		clientEffect->SetAmbient( true );

		BecomeActive ( TH_THINK );
		return true;
	}
	
	return false;
}

/*
================
riFireFX::Event_Start
================
*/
void riFireFX::Event_Start ( void ) {
	effect = CalcCurrentEffect();
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
riFireFX::Event_Stop
================
*/
void riFireFX::Event_Stop ( void ) {
	if( !effect ) {
		return;
	}

	Stop( false );
	effect = NULL;
	clientEffect = NULL;
}

/*
=================
riFireFX::Event_Activate
=================
*/
void riFireFX::Event_Activate( idEntity *activator ) {
	// Stop the effect if its already playing
	if( !clientEntities.IsListEmpty ( ) ) {
		Event_Stop ( );
	} else {
		Event_Start ( );
	}

	ActivateTargets( activator );
}

/*
============
riFireFX::Damage
============
*/
void riFireFX::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( !fl.takedamage ) {
		return;
	}

	// filter damage
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Warning( "Unknown damageDef '%s' on '%s'", damageDefName, GetName() );
		return;
	}

	// If the filter isn't matched then ignore it
	if ( !damageDef->GetBool ( "filter_freeze" ) ) {
		return;
	}

	int lastHealth = health;
	idEntity::Damage ( inflictor, attacker, dir, damageDefName, damageScale, location );
	// check health changed
	if (lastHealth == health)
		return;

	// update effect stage
	if (UpdateStage())
		ExecuteStage();
}

/*
================
riFireFX::Killed
================
*/
void riFireFX::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	// update effect stage
	if (UpdateStage())
		ExecuteStage();
}

/*
================
riFireFX::Event_Touch
================
*/
void riFireFX::Event_Touch( idEntity *other, trace_t *trace ) {
	const bool playerOnly = true;

	if (!other || !on)
		return;

	if (!damage || damage->IsImplicit())
		return;

	if ((stage != ST_LARGE && stage != ST_SMALL) || health <= 0)
		return;

	// RAVEN BEGIN
	// kfuller: playeronly flag
	// jnewquist: Use accessor for static class type
	if ( playerOnly && !other->IsType( idPlayer::GetClassType() ) ) {
		return;
	}
	// RAVEN END

	if ( gameLocal.time >= nextTime ) {
		float damageScale = stage != ST_SMALL ? 0.5f : 1.0f;
		other->Damage( this, NULL, vec3_origin, damage->GetName(), damageScale, INVALID_JOINT );

		ActivateTargets( other );
		//CallScript( other );

		nextTime = gameLocal.time + /*SEC2MS*/( damageDelay );
	}
}

// Awakening END
