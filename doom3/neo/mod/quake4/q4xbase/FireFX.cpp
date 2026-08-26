#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Effect.h"
#include "client/ClientEffect.h"


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
			// clear targets
			RemoveTargets(true);
			fl.takedamage = false;
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
	if ( forwardDamageEnt.IsValid() ) {
		forwardDamageEnt->Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
		return;
	}

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

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	float power;
	switch (stage)
	{
		case ST_LARGE: power = 2.0f; break;
		case ST_SMALL: power = 5.0f; break;
		case ST_SMOKE: power = 10.0f; break;
		default: power = 1.0f; break;
	}

	int	damage = damageDef->GetInt( "damage" );
	damage = idMath::Ftoi((float)damage * damageScale * power);

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage ) {
		// do the damage
		//jshepard: this is kinda important, no?
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
