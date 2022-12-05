// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "ai/AI_Manager.h"
#include "Projectile.h"
#include "spawner.h"

/*
===============================================================================

	idProjectile

===============================================================================
*/

static const float BOUNCE_SOUND_MIN_VELOCITY	= 200.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 400.0f;

const idEventDef EV_Explode( "<explode>", NULL );
const idEventDef EV_Fizzle( "<fizzle>", NULL );
const idEventDef EV_RadiusDamage( "<radiusdmg>", "E" );
const idEventDef EV_ResidualDamage ( "<residualdmg>", "E" );

CLASS_DECLARATION( idEntity, idProjectile )
	EVENT( EV_Explode,			idProjectile::Event_Explode )
	EVENT( EV_Fizzle,			idProjectile::Event_Fizzle )
	EVENT( EV_Touch,			idProjectile::Event_Touch )
	EVENT( EV_RadiusDamage,		idProjectile::Event_RadiusDamage )
	EVENT( EV_ResidualDamage,	idProjectile::Event_ResidualDamage )
END_CLASS

/*
================
idProjectile::idProjectile
================
*/
idProjectile::idProjectile( void ) {
	methodOfDeath		= -1;
	owner				= NULL;
	memset( &projectileFlags, 0, sizeof( projectileFlags ) );
	damagePower			= 1.0f;

	memset( &renderLight, 0, sizeof( renderLight ) );

	lightDefHandle		= -1;
	lightOffset.Zero();
	lightStartTime		= 0;
	lightEndTime		= 0;
	lightColor.Zero();

	visualAngles.Zero();
	angularVelocity.Zero();
	speed.Init( 0.0f, 0.0f, 0.0f, 0.0f );
	updateVelocity		= false;

	rotation.Init( 0, 0.0f, mat3_identity.ToQuat(), mat3_identity.ToQuat() );

	flyEffect			= NULL;
	flyEffectAttenuateSpeed = 0.0f;
	bounceCount			= 0;
	hitCount			= 0;
	state				= SPAWNED;
	
	fl.networkSync		= true;

	prePredictTime		= 0;

	syncPhysics			= false;

	launchTime			= 0;
	launchOrig			= vec3_origin;
	launchDir			= vec3_origin;
	launchSpeed			= 0.0f;
}

/*
================
idProjectile::Spawn
================
*/
void idProjectile::Spawn( void ) {
 	physicsObj.SetSelf( this );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
 	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
 	physicsObj.SetContents( 0 );
 	physicsObj.SetClipMask( 0 );
 	physicsObj.PutToRest();
 	SetPhysics( &physicsObj );
	prePredictTime = spawnArgs.GetInt( "predictTime", "0" );
	syncPhysics = spawnArgs.GetBool( "net_syncPhysics", "0" );

	if ( gameLocal.isClient ) {
		Hide();
	}
}

/*
================
idProjectile::Save
================
*/
void idProjectile::Save( idSaveGame *savefile ) const {
	
	savefile->WriteInt( methodOfDeath );				// cnicholson: Added unsaved var
	owner.Save( savefile );
	savefile->Write( &projectileFlags, sizeof( projectileFlags ) );
	savefile->WriteFloat( damagePower );

   	savefile->WriteRenderLight( renderLight );

   	savefile->WriteInt( ( int )lightDefHandle );
	savefile->WriteVec3( lightOffset );
 	savefile->WriteInt( lightStartTime );
 	savefile->WriteInt( lightEndTime );
 	savefile->WriteVec3( lightColor );

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteAngles( visualAngles );				// cnicholson: added unsaved var
	savefile->WriteAngles( angularVelocity );			// cnicholson: moved var

	// Save speed
	savefile->WriteBool ( updateVelocity );
	savefile->WriteFloat( speed.GetStartTime() );
	savefile->WriteFloat( speed.GetDuration() );
	savefile->WriteFloat( speed.GetStartValue() );
	savefile->WriteFloat( speed.GetEndValue() );	

	// rotation; this is a class, so it doesnt get saved here

	flyEffect.Save( savefile );							// cnicholson: added unsaved var
	savefile->WriteFloat( flyEffectAttenuateSpeed );	// cnicholson: added unsaved var
	savefile->WriteInt ( bounceCount );
	savefile->WriteInt ( hitCount );

 	savefile->WriteInt( (int)state );
}

/*
================
idProjectile::Restore
================
*/
void idProjectile::Restore( idRestoreGame *savefile ) {
	float	fset;
	idVec3	temp;

	savefile->ReadInt( methodOfDeath );					// cnicholson: Added unrestored var
	owner.Restore( savefile );
	savefile->Read( &projectileFlags, sizeof( projectileFlags ) );
	savefile->ReadFloat( damagePower );

	savefile->ReadRenderLight( renderLight );

	savefile->ReadInt( (int &)lightDefHandle );
	if ( lightDefHandle != -1 ) {
		//get the handle again as it's out of date after a restore!
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}
	savefile->ReadVec3( lightOffset );
 	savefile->ReadInt( lightStartTime );
 	savefile->ReadInt( lightEndTime );
 	savefile->ReadVec3( lightColor );

	// Restore the physics
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics ( &physicsObj );

	savefile->ReadAngles( visualAngles );				// cnicholson: added unrestored var
	savefile->ReadAngles( angularVelocity );			// cnicholson: moved var

	// Restore speed
	savefile->ReadBool ( updateVelocity );
	savefile->ReadFloat( fset );
	speed.SetStartTime( fset );
	savefile->ReadFloat( fset );
	speed.SetDuration( fset );
	savefile->ReadFloat( fset );
	speed.SetStartValue( fset );
	savefile->ReadFloat( fset );
	speed.SetEndValue( fset );
	
	// rotation?

	flyEffect.Restore( savefile );						// cnicholson: added unrestored var
	savefile->ReadFloat( flyEffectAttenuateSpeed );		// cnicholson: added unsaved var
	savefile->ReadInt ( bounceCount );
	savefile->ReadInt ( hitCount );

	savefile->ReadInt( (int &)state );
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
idProjectile::SetSpeed
================
*/
void idProjectile::SetSpeed( float s, int accelTime ) {
	idVec3 vel;
	vel = physicsObj.GetLinearVelocity();
	vel.Normalize();
	speed.Init( gameLocal.time, accelTime, speed.GetCurrentValue(gameLocal.time), s );

	if ( accelTime > 0 ) {
		updateVelocity = true;
	} else {
		updateVelocity = false;
	}

	// Update the velocity to match the direction we are facing and include any accelerations
	physicsObj.SetLinearVelocity( speed.GetCurrentValue( gameLocal.time ) * vel );			
}

/*
================
idProjectile::Create
================
*/
void idProjectile::Create( idEntity* _owner, const idVec3 &start, const idVec3 &dir, idEntity* ignore, idEntity* extraPassEntity ) {
	idDict		args;
	idStr		shaderName;
	idVec3		light_color;
	idVec3		light_offset;
	idVec3		tmp;
	idMat3		axis;
	
	Unbind();

	axis = dir.ToMat3();

 	physicsObj.SetOrigin( start );
 	physicsObj.SetAxis( axis );

 	physicsObj.GetClipModel()->SetOwner( ignore ? ignore : _owner );
	physicsObj.extraPassEntity = extraPassEntity;

	owner = _owner;

	memset( &renderLight, 0, sizeof( renderLight ) );
	shaderName = spawnArgs.GetString( "mtr_light_shader" );
	if ( *shaderName ) {
		renderLight.shader = declManager->FindMaterial( shaderName, false );
		renderLight.pointLight = true;
		renderLight.lightRadius[0] =
		renderLight.lightRadius[1] =
		renderLight.lightRadius[2] = spawnArgs.GetFloat( "light_radius" );
		spawnArgs.GetVector( "light_color", "1 1 1", light_color );
		renderLight.shaderParms[0] = light_color[0];
		renderLight.shaderParms[1] = light_color[1];
		renderLight.shaderParms[2] = light_color[2];
		renderLight.shaderParms[3] = 1.0f;
// RAVEN BEGIN
// dluetscher: added detail levels to render lights
		renderLight.detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// dluetscher: set the projectile lights to be no shadows
		renderLight.noShadows = cvarSystem->GetCVarInteger("com_machineSpec") < 3;
// RAVEN END
	}

	spawnArgs.GetVector( "light_offset", "0 0 0", lightOffset );

	lightStartTime = 0;
	lightEndTime = 0;

	damagePower = 1.0f;

 	UpdateVisuals();

	state = CREATED;
}

/*
=================
idProjectile::~idProjectile
=================
*/
idProjectile::~idProjectile() {
	StopSound( SND_CHANNEL_ANY, false );
	FreeLightDef();
	SetPhysics( NULL );
}

/*
=================
idProjectile::FreeLightDef
=================
*/
void idProjectile::FreeLightDef( void ) {
	if ( lightDefHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
		lightDefHandle = -1;
	}
}

/*
=================
idProjectile::Launch
=================
*/
void idProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float dmgPower ) {
	float			fuse;
	idVec3			velocity;
	float			linear_friction;
	float			angular_friction;
	float			contact_friction;
	float			bounce;
	float			mass;
	float			gravity;
	float			temp, temp2;
	idVec3			gravVec;
	idVec3			tmp;
	int				contents;
 	int				clipMask;

 	// allow characters to throw projectiles during cinematics, but not the player
 	if ( owner.GetEntity() && !owner.GetEntity()->IsType( idPlayer::GetClassType() ) ) {
 		cinematic = owner.GetEntity()->cinematic;
 	} else {
 		cinematic = false;
 	} 

	// Set the damage
	damagePower = dmgPower;

	if ( !spawnArgs.GetFloat( "speed", "0", temp ) ) {
		spawnArgs.GetVector( "velocity", "0 0 0", tmp );
		temp = tmp[0];
	} else {
		float speedRandom;
		if ( !spawnArgs.GetFloat( "speedRandom", "0", speedRandom ) ) {
			temp += gameLocal.random.CRandomFloat()*speedRandom;
		}
	}
	if ( !spawnArgs.GetFloat( "speed_end", "0", temp2 ) ) {
		temp2 = temp;
	}
	float speedDuration;
	speedDuration = SEC2MS( spawnArgs.GetFloat( "speed_duration", "0" ) );
	speed.Init( gameLocal.time, speedDuration, temp, temp2 );
	if ( speedDuration > 0 && temp != temp2 ) {
		// only support constant velocity projectiles in MP
		// ( we also assume that no MP projectiles use speedRandom )
		assert( !gameLocal.isServer );
		updateVelocity = true;
	}
	launchSpeed = temp;

	spawnArgs.GetAngles( "angular_velocity", "0 0 0", angularVelocity );

	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	angular_friction	= spawnArgs.GetFloat( "angular_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );
	fuse				= spawnArgs.GetFloat( "fuse" ) + ( spawnArgs.GetFloat( "fuse_random", "0" ) * gameLocal.random.RandomFloat() );
	bounceCount			= spawnArgs.GetInt( "bounce_count", "-1" );
	
	//spawn impact entity information
	impactEntity				= spawnArgs.GetString("def_impactEntity","");
	numImpactEntities			= spawnArgs.GetInt("numImpactEntities","0");
	ieMinPitch					= spawnArgs.GetInt("ieMinPitch","0");
	ieMaxPitch					= spawnArgs.GetInt("ieMaxPitch","0");
	ieSlicePercentage			= spawnArgs.GetFloat("ieSlicePercentage","0.0");
	
	projectileFlags.detonate_on_world	= spawnArgs.GetBool( "detonate_on_world" );
	projectileFlags.detonate_on_actor	= spawnArgs.GetBool( "detonate_on_actor" );
	projectileFlags.randomShaderSpin	= spawnArgs.GetBool( "random_shader_spin" );
	projectileFlags.detonate_on_bounce  = spawnArgs.GetBool( "detonate_on_bounce" );

	lightStartTime = 0;
	lightEndTime = 0;

	impactedEntity = 0;

	if ( health ) {
		fl.takedamage = true;
	}

// RAVEN BEGIN
// abahr:
	gravVec = ( idMath::Fabs(gravity) > VECTOR_EPSILON ) ? gameLocal.GetCurrentGravity(this) * gravity : vec3_zero;
// RAVEN END

	Unbind();

	contents = 0;
	clipMask = spawnArgs.GetBool( "clipmask_rendermodel", "1" ) ? MASK_SHOT_RENDERMODEL : MASK_SHOT_BOUNDINGBOX;
	
	// all projectiles are projectileclip
	clipMask |= CONTENTS_PROJECTILECLIP;

	if ( spawnArgs.GetBool( "clipmask_largeshot", "1" ) ) {
		clipMask |= CONTENTS_LARGESHOTCLIP;
	}

	if ( spawnArgs.GetBool( "clipmask_moveable", "0" ) ) {
		clipMask |= CONTENTS_MOVEABLECLIP;
	}
	if ( spawnArgs.GetBool( "clipmask_monsterclip", "0" ) ) {
		clipMask |= CONTENTS_MONSTERCLIP;
	}
	
	if ( spawnArgs.GetBool( "detonate_on_trigger" ) ) {
		contents |= CONTENTS_TRIGGER;
	}

 	if ( !spawnArgs.GetBool( "no_contents", "1" ) ) {
 		contents |= CONTENTS_PROJECTILE;
 	}

	clipMask |= CONTENTS_PROJECTILE;

	// don't do tracers on client, we don't know origin and direction
	if ( spawnArgs.GetBool( "tracers" ) && gameLocal.random.RandomFloat() > 0.5f ) {
		SetModel( spawnArgs.GetString( "model_tracer" ) );
		projectileFlags.isTracer = true;
	}

	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linear_friction, angular_friction, contact_friction );
	physicsObj.SetBouncyness( bounce, !projectileFlags.detonate_on_bounce );
	physicsObj.SetGravity( gravVec );
	physicsObj.SetContents( contents );
 	physicsObj.SetClipMask( clipMask | CONTENTS_WATER );
	physicsObj.SetLinearVelocity( dir * speed.GetCurrentValue(gameLocal.time) + pushVelocity );
	physicsObj.SetOrigin( start );
	physicsObj.SetAxis( dir.ToMat3() );

	if ( !gameLocal.isClient ) {
		if ( fuse <= 0 ) {
			// run physics for 1 second
			RunPhysics();
			PostEventMS( &EV_Remove, spawnArgs.GetInt( "remove_time", "1500" ) );
		} else if ( spawnArgs.GetBool( "detonate_on_fuse" ) ) {
			fuse -= timeSinceFire;
			if ( fuse < 0.0f ) {
				fuse = 0.0f;
			}
			PostEventSec( &EV_Explode, fuse );
		} else {
			fuse -= timeSinceFire;
			if ( fuse < 0.0f ) {
				fuse = 0.0f;
			}
			PostEventSec( &EV_Fizzle, fuse );
		}
	}

	idQuat q( dir.ToMat3().ToQuat() );
	rotation.Init( gameLocal.GetTime(), 0.0f, q, q );

	if ( projectileFlags.isTracer ) {
		StartSound( "snd_tracer", SND_CHANNEL_BODY, 0, false, NULL );
	} else {
		StartSound( "snd_fly", SND_CHANNEL_BODY, 0, false, NULL );
	}

	// used for the plasma bolts but may have other uses as well
	if ( projectileFlags.randomShaderSpin ) {
		float f = gameLocal.random.RandomFloat();
		f *= 0.5f;
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = f;
	}

 	UpdateVisuals();

	// Make sure these come after update visuals so the origin and axis are correct
	PlayEffect( "fx_launch", renderEntity.origin, renderEntity.axis );
	
	flyEffect = PlayEffect( "fx_fly", renderEntity.origin, renderEntity.axis, true );
	flyEffectAttenuateSpeed = spawnArgs.GetFloat( "flyEffectAttenuateSpeed", "0" );

	state = LAUNCHED;

	hitCount = 0;

	predictTime = prePredictTime;

	if ( gameLocal.isServer ) {
		// store launch information for networking
		launchTime = gameLocal.time;
		launchOrig = physicsObj.GetOrigin();
		launchDir = dir;
	}

	if ( g_perfTest_noProjectiles.GetBool() ) {
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idProjectile::Think
================
*/
void idProjectile::Think( void ) {
	// run physics
	if ( thinkFlags & TH_PHYSICS ) {

		// Update the velocity to match the changing speed
		if ( updateVelocity ) {
			idVec3 vel;
			vel = physicsObj.GetLinearVelocity ( );
			vel.Normalize ( );
			physicsObj.SetLinearVelocity ( speed.GetCurrentValue ( gameLocal.time ) * vel );			
			if ( speed.IsDone ( gameLocal.time ) ) {
				updateVelocity = false;
			}
		}
		
		RunPhysics();
		
		// If we werent at rest and are now then start the atrest fuse
		if ( physicsObj.IsAtRest( ) ) {
			float fuse = spawnArgs.GetFloat( "fuse_atrest" );
			if ( fuse > 0.0f ) {
				if ( spawnArgs.GetBool( "detonate_on_fuse" ) ) {
					CancelEvents( &EV_Explode );
					PostEventSec( &EV_Explode, fuse );
				} else {
					CancelEvents( &EV_Fizzle );
					PostEventSec( &EV_Fizzle, fuse );
				}
			}
		}

		// Stop the trail effect if the physics flag was removed
		if ( flyEffect && flyEffectAttenuateSpeed > 0.0f ) {
			if ( physicsObj.IsAtRest( ) ) {
				flyEffect->Stop( );
				flyEffect = NULL;				
			} else {
				float speed;
				speed = idMath::ClampFloat( 0, flyEffectAttenuateSpeed, physicsObj.GetLinearVelocity ( ).LengthFast ( ) );
				flyEffect->Attenuate( speed / flyEffectAttenuateSpeed );
			}
		}

		UpdateVisualAngles();
	}
		
	Present();

	// add the light
 	if ( renderLight.lightRadius.x > 0.0f && g_projectileLights.GetBool() ) {
		renderLight.origin = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * lightOffset;
		renderLight.axis = GetPhysics()->GetAxis();
		if ( ( lightDefHandle != -1 ) ) {
			if ( lightEndTime > 0 && gameLocal.time <= lightEndTime + gameLocal.GetMSec() ) {
				idVec3 color( 0, 0, 0 );
				if ( gameLocal.time < lightEndTime ) {
					float frac = ( float )( gameLocal.time - lightStartTime ) / ( float )( lightEndTime - lightStartTime );
					color.Lerp( lightColor, color, frac );
				} 
				renderLight.shaderParms[SHADERPARM_RED] = color.x;
				renderLight.shaderParms[SHADERPARM_GREEN] = color.y;
				renderLight.shaderParms[SHADERPARM_BLUE] = color.z;
			} 
			gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
		} else {
			lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
		}
	}
}

/*
=================
idProjectile::UpdateVisualAngles
=================
*/
void idProjectile::UpdateVisualAngles() {
	idVec3 linearVelocity( GetPhysics()->GetLinearVelocity() );

	if( angularVelocity.Compare(ang_zero, VECTOR_EPSILON) ) {
		rotation.Init( gameLocal.GetTime(), 0.0f, rotation.GetCurrentValue(gameLocal.GetTime()), linearVelocity.ToNormal().ToMat3().ToQuat()  );
		return;
	}

	if( physicsObj.GetNumContacts() ) {
		return;
	}

	if( !rotation.IsDone(gameLocal.GetTime()) ) {
		return;
	}

	if( linearVelocity.Length() <= BOUNCE_SOUND_MIN_VELOCITY ) {
		return;
	}

	visualAngles += angularVelocity;
	idQuat q = visualAngles.ToQuat() * linearVelocity.ToNormal().ToMat3().ToQuat();
	rotation.Init( gameLocal.GetTime(), gameLocal.GetMSec(), rotation.GetCurrentValue(gameLocal.GetTime()), q  );
}

/*
=================
idProjectile::Collide
=================
*/
bool idProjectile::Collide( const trace_t &collision, const idVec3 &velocity ) {
	bool dummy = false;
	return Collide( collision, velocity, dummy );
}

bool idProjectile::Collide( const trace_t &collision, const idVec3 &velocity, bool &hitTeleporter ) {
 	idEntity*	ent;
	idEntity*	actualHitEnt = NULL;
 	idEntity*	ignore;
 	const char*	damageDefName;
 	idVec3		dir;
 	bool		canDamage;
 	
 	hitTeleporter = false;

	if ( state == EXPLODED || state == FIZZLED ) {
		return true;
	}

	// allow projectiles to hit triggers (teleports)
	// predict this on a client
	if( collision.c.contents & CONTENTS_TRIGGER ) {
		idEntity* trigger = gameLocal.entities[ collision.c.entityNum ];
		
		if( trigger ) {
			if ( trigger->RespondsTo( EV_Touch ) || trigger->HasSignal( SIG_TOUCH ) ) {
				
				hitTeleporter = true;
				
				trace_t trace;
				
				trace.endpos = physicsObj.GetOrigin();
				trace.endAxis = physicsObj.GetAxis();

				trace.c.contents = collision.c.contents;
				trace.c.entityNum = collision.c.entityNum;
				if( trigger->GetPhysics()->GetClipModel() ) {
					trace.c.id = trigger->GetPhysics()->GetClipModel()->GetId();
				} else {
					trace.c.id = 0;
				}
				

				trigger->Signal( SIG_TOUCH );
				trigger->ProcessEvent( &EV_Touch, this, &trace );
			}
		}

		// when we hit a trigger, align our velocity to the trigger's coordinate plane
		if( gameLocal.isServer ) {
			idVec3 up( 0.0f, 0.0f, 1.0f );
			idVec3 right = collision.c.normal.Cross( up );
			idMat3 mat( collision.c.normal, right, up );

			physicsObj.SetLinearVelocity( -1.0f * (physicsObj.GetLinearVelocity() * mat.Transpose()) );
			physicsObj.SetLinearVelocity( idVec3( physicsObj.GetLinearVelocity()[ 0 ], -1.0 *physicsObj.GetLinearVelocity()[ 1 ], -1.0 * physicsObj.GetLinearVelocity()[ 2 ] ) );

			// update the projectile's launchdir and launch origin
			// this will propagate the change to the clients for prediction
			// re-launch the projectile

			idVec3 newDir = physicsObj.GetLinearVelocity();
			newDir.Normalize();
			launchTime = gameLocal.time;
			launchDir = newDir;
			physicsObj.SetOrigin( physicsObj.GetOrigin() + idVec3( 0.0f, 0.0f, 32.0f ) );
			physicsObj.SetAxis( newDir.ToMat3() );
			launchOrig = physicsObj.GetOrigin();
		}

		if( !(collision.c.contents & CONTENTS_SOLID) || hitTeleporter ) {
			return false;
		}
	}

	// don't predict collisions on projectiles that need their physics synced (e.g. grenades)
	if ( gameLocal.isClient && (!g_clientProjectileCollision.GetBool() || syncPhysics) ) {
		// optionally do not try to predict detonates, that causes projectiles disappearing
		// but most of the time works well to stop projectiles clipping through geometry onc lients
		return false;
	}

 	// remove projectile when a 'noimpact' surface is hit
 	if ( ( collision.c.material != NULL ) && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) ) {
 		PostEventMS( &EV_Remove, 0 );
		StopEffect( "fx_fly" );
		if( flyEffect)	{
			//flyEffect->Event_Remove();
		}
 		return true;
 	}
 
	// get the entity the projectile collided with
	ent = gameLocal.entities[ collision.c.entityNum ];
	if ( ent == owner.GetEntity() ) {
		// assert( 0 );		// twhitaker: this isn't necessary
		return true;
	}

 	// just get rid of the projectile when it hits a player in noclip
 	if ( ent->IsType( idPlayer::GetClassType() ) && static_cast<idPlayer *>( ent )->noclip ) {
   		PostEventMS( &EV_Remove, 0 );
  		common->DPrintf( "Projectile collision no impact\n" );
   		return true;
   	}

	// If the hit entity is bound to an actor use the actor instead
	if ( ent->GetTeamMaster( ) && ent->GetTeamMaster( )->IsType ( idActor::GetClassType() ) ) {
		actualHitEnt = ent;
		ent = ent->GetTeamMaster( );
	}

	// Can the projectile damage?  
	canDamage = ent->fl.takedamage && !(( collision.c.material != NULL ) && ( collision.c.material->GetSurfaceFlags() & SURF_NODAMAGE ));
  
 	// direction of projectile
 	dir = velocity;
 	dir.Normalize();
 
 	// projectiles can apply an additional impulse next to the rigid body physics impulse
// RAVEN BEGIN
// abahr: added call to SkipDamageImpulse changed where push comes from
	damageDefName = NULL;
	if ( collision.c.materialType ) {
		damageDefName = spawnArgs.GetString( va("def_damage_%s", collision.c.materialType->GetName()) );
	}
	if ( !damageDefName || !*damageDefName ) {
		damageDefName = spawnArgs.GetString ( "def_damage" );
	}

	if( damageDefName && damageDefName[0] ) {
		const idDict* dict = gameLocal.FindEntityDefDict( damageDefName, false );
		if ( dict ) {
 			ent->ApplyImpulse( this, collision.c.id, collision.endpos, dir, dict );
 		}
	}
// RAVEN END
 
	//Spawn any impact entities if necessary.
	SpawnImpactEntities(collision, velocity);

	//Apply any impact force if the necessary
	//ApplyImpactForce(ent, collision, dir);
 
	// MP: projectiles open doors
	if ( gameLocal.isMultiplayer && ent->IsType( idDoor::GetClassType() ) && !static_cast< idDoor * >(ent)->IsOpen() && !ent->spawnArgs.GetBool( "no_touch" ) ) {
		ent->ProcessEvent( &EV_Activate , this );
	}

	// If the projectile hits water then we need to let the projectile keep going
	if ( ent->GetPhysics()->GetContents() & CONTENTS_WATER ) {
		if ( !physicsObj.IsInWater( ) ) {
			StopEffect( "fx_fly" );
			if( flyEffect)	{
				//flyEffect->Event_Remove();
			}
		}
		// Pass through water
		return false;
	} else if ( canDamage && ent->IsType( idActor::GetClassType() ) ) {
		if ( !projectileFlags.detonate_on_actor ) {
			return false;
		}
	} else {
		bool bounce = false;
		
		// Determine if the projectile should bounce
		bounce = !physicsObj.IsInWater() && !projectileFlags.detonate_on_world && !canDamage;
		bounce = bounce && (bounceCount == -1 || bounceCount > 0);
		//assert(collision.c.material);
		if ( !bounce && collision.c.material && (collision.c.material->GetSurfaceFlags() & SURF_BOUNCE) ) {
			bounce = !projectileFlags.detonate_on_bounce;
		}
		
		if ( bounce ) {
			if ( bounceCount != -1 ) {
				bounceCount--;
			}
			
			StartSound( "snd_ricochet", SND_CHANNEL_ITEM, 0, true, NULL );

			float len = velocity.Length();
			if ( len > BOUNCE_SOUND_MIN_VELOCITY ) {
				if ( ent->IsType ( idMover::GetClassType ( ) ) ) {			
					ent->PlayEffect( 
						gameLocal.GetEffect(spawnArgs,"fx_bounce",collision.c.materialType), 
						collision.c.point, collision.c.normal.ToMat3(), 
						false, vec3_origin, true );
				} else {
					gameLocal.PlayEffect( 
						gameLocal.GetEffect(spawnArgs,"fx_bounce",collision.c.materialType), 
						collision.c.point, collision.c.normal.ToMat3(), 
						false, vec3_origin, true );
				}			
			} else {
				// FIXME: clean up
				idMat3 axis( rotation.GetCurrentValue(gameLocal.GetTime()).ToMat3() );
				axis[0].ProjectOntoPlane( collision.c.normal );
				axis[0].Normalize();
				axis[2] = collision.c.normal;
				axis[1] = axis[2].Cross( axis[0] ).ToNormal();

				rotation.Init( gameLocal.GetTime(), SEC2MS(spawnArgs.GetFloat("settle_duration")), rotation.GetCurrentValue(gameLocal.GetTime()), axis.ToQuat() );
			}
			if ( actualHitEnt
				&& actualHitEnt != ent
				&& actualHitEnt->spawnArgs.GetBool( "takeBounceDamage" ) )
			{//bleh...
 				if ( damageDefName[0] != '\0' ) {
					idVec3 dir = velocity;
					dir.Normalize();
					actualHitEnt->Damage( this, owner, dir, damageDefName, damagePower, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) );
				}
			}
			return false;		
		}
	}

	SetOrigin( collision.endpos );
//	SetAxis( collision.endAxis );

	// unlink the clip model because we no longer need it
	GetPhysics()->UnlinkClip();

	ignore = NULL;

// RAVEN BEGIN
// jshepard: Single Player- if the the player is the attacker and the victim is teammate, don't play any blood effects.
	bool willPlayDamageEffect = true;

// jnewquist: Use accessor for static class type 
		if ( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::GetClassType() ) ) {
			// if the projectile hit an ai
			if ( ent->IsType( idAI::GetClassType() ) ) {
				idPlayer *player = static_cast<idPlayer *>( owner.GetEntity() );
				player->AddProjectileHits( 1 );

// jshepard: Single Player- if the the player is the attacker and the victim is teammate, don't play any blood effects.
				idAI * ai_ent = static_cast<idAI* >(ent);
				if( ai_ent->team == player->team)	{
					willPlayDamageEffect = false;
				}
			}
		}

// RAVEN END

	// if the hit entity takes damage
	if ( canDamage ) {

 		if ( damageDefName[0] != '\0' ) {
			idVec3 dir = velocity;
			dir.Normalize();
// RAVEN BEGIN
// jdischler: code from the 'other' project..to ensure that if an attached head is hit, the body will use the head joint
//	otherwise damage zones for head attachments no-worky
			int hitJoint = CLIPMODEL_ID_TO_JOINT_HANDLE(collision.c.id);
			if ( ent->IsType(idActor::GetClassType()) )
			{
				idActor* entActor = static_cast<idActor*>(ent);
				if ( entActor && entActor->GetHead() && entActor->GetHead()->IsType(idAFAttachment::GetClassType()) )
				{
					idAFAttachment* headEnt = static_cast<idAFAttachment*>(entActor->GetHead());
					if ( headEnt && headEnt->entityNumber == collision.c.entityNum )
					{//hit ent's head, get the proper joint for the head
						hitJoint = entActor->GetAnimator()->GetJointHandle("head");
					}
				}
			}	
// RAVEN END
 			ent->Damage( this, owner, dir, damageDefName, damagePower, hitJoint );
			
			if( owner && owner->IsType( idPlayer::GetClassType() ) && ent->IsType( idActor::GetClassType() ) ) {
				statManager->WeaponHit( (const idActor*)(owner.GetEntity()), ent, methodOfDeath, hitCount == 0 );			
				hitCount++;
			}
		}
	}
	
	ignore = ent;

	// TODO: fixme
	ent->AddDamageEffect ( collision, velocity, damageDefName, owner );

// RAVEN BEGIN
// jshepard: Single Player- if the the player is the attacker and the victim is teammate, don't play any effects.
// this should make sure that only explosion effects play when the player shoots his comrades. 
	if( willPlayDamageEffect || spawnArgs.GetBool( "friendly_impact") )	{
		DefaultDamageEffect( collision, velocity, damageDefName );
	}
// RAVEN END

/*
	// if the projectile causes a damage effect
	bool explodeFX = true;
 	if ( spawnArgs.GetBool( "impact_damage_effect" ) ) {
		// if the hit entity has a special damage effect
		if ( ent->spawnArgs.GetBool( "bleed" ) ) {
 			ent->AddDamageEffect( collision, velocity, damageDefName );
		} else {
 			DefaultDamageEffect( collision, velocity, damageDefName );
			explodeFX = false;
		}
	}
*/

	// don't predict explosions on clients
	if( gameLocal.isClient ) {
		return true;
	}

	Explode( &collision, false, ignore );

	return true;
}

void idProjectile::SpawnImpactEntities(const trace_t& collision, const idVec3 velocity)
{
	if( impactEntity.Length() == 0 || numImpactEntities == 0 )
		return;

	const idDict* impactEntityDict = gameLocal.FindEntityDefDict(impactEntity);
	if(impactEntityDict == NULL)
		return;

	idVec3 tempDirection;
	idVec3 direction;
	direction.Zero();

	idVec3 up = collision.c.normal;

	//Calculate the axes for that are oriented to the impact point. 
	idMat3 impactAxes;

	idVec3 right = velocity.Cross(up);
	idVec3 forward = up.Cross(right);

	right.Normalize();
	forward.Normalize();
	impactAxes[0] = forward;
	impactAxes[1] = right;
	impactAxes[2] = up;

	//Calculate the reflection vector by calculating the forward component and up component of the projectile direction
	//idVec3 reflectionVelocity = (forward * (velocity*0.33f * forward));// - (up * (velocity * up));
	idVec3 reflectionVelocity = up * 0.01f;

	//The algorithm below will launch entities at a random pitch and somewhat random yaw.
	//The yaw is calculated by dividing 360 by the number of entities to spawn. This creates
	//a distribution slice. Then using the slice percentage, this will determine how much of the
	//slice to use.
	//This creates a random,but somewhat even coverage of the circle.

	//Calculate the slice size and pick a random start position.
    int sliceSize = 360 / numImpactEntities;
	int startPosition = rvRandom::irand(0, 360);

	//Move the origin away from the collision point. This prevents the projectiles
	//from colliding with the surface.
	idVec3 origin = collision.endpos;
	origin += 10.0f * collision.c.normal;
	for(int i = 0;i < numImpactEntities; i++)
	{
		idProjectile* spawnProjectile = NULL;
		gameLocal.SpawnEntityDef(*impactEntityDict,(idEntity**)&spawnProjectile);
		if(spawnProjectile != NULL)
		{
			int pitch = rvRandom::irand(ieMinPitch, ieMaxPitch);
			int sliceMiddle = (i * sliceSize) + startPosition;
			int sliceSloppiness = (sliceSize * ieSlicePercentage) / 2;

			int yaw = rvRandom::irand(sliceMiddle - sliceSloppiness, sliceMiddle + sliceSloppiness);
			yaw = yaw % 360;

			float cosPitch = idMath::Cos(DEG2RAD(pitch));
			tempDirection.x = cosPitch * idMath::Cos(DEG2RAD(yaw));
			tempDirection.y = cosPitch * idMath::Sin(DEG2RAD(yaw));
			tempDirection.z = idMath::Sin(DEG2RAD(pitch));

			spawnProjectile->SetOwner(owner);

			//Now orient the direction to the surface world orientation.
			direction = impactAxes * tempDirection;
			spawnProjectile->Launch(origin, direction, reflectionVelocity);
		}
	}
}

/*
=================
idProjectile::DefaultDamageEffect
=================
*/
void idProjectile::DefaultDamageEffect( const trace_t &tr, const idVec3 &velocity, const char *damageDefName ) {
	idEntity* ent;
	idMat3	  axis;
	ent = gameLocal.entities[ tr.c.entityNum ];	

	// Make sure we want to play effects
	if  ( (!*spawnArgs.GetString( "def_splash_damage" ) || spawnArgs.GetBool( "bloodyImpactEffect" ))
		&& owner.GetEntity( ) && !ent->CanPlayImpactEffect( owner, ent ) ) {
		return;
	}

	// Effect axis when hitting actors is along the direction of impact because actor models are 
	// very detailed.
	if ( ent->IsType( idActor::GetClassType() ) || ent->IsType( idAFAttachment::GetClassType() ) ) {
		idVec3 dir;
		dir = velocity;
		dir.Normalize ( );
		axis = ((-dir + tr.c.normal) * 0.5f).ToMat3();
		
		// Play an actor specific impact effect?
		const idDecl *actorImpactEffect = gameLocal.GetEffect( spawnArgs, "fx_impact_actor", tr.c.materialType );
		if ( actorImpactEffect ) {
			gameLocal.PlayEffect( actorImpactEffect, tr.c.point, axis, false, vec3_origin, true );
			return;
		}
	} else {
		axis = tr.c.normal.ToMat3();
	}
	
	// Play an impact effect on the entity that got hit
	if ( ent->IsType( idMover::GetClassType ( ) ) ) {
		ent->PlayEffect( gameLocal.GetEffect( spawnArgs, "fx_impact", tr.c.materialType ), tr.c.point, axis, false, vec3_origin, true );
	} else { 
		gameLocal.PlayEffect( gameLocal.GetEffect( spawnArgs, "fx_impact", tr.c.materialType ), tr.c.point, axis, false, vec3_origin, true );
	}
}

/*
================
idProjectile::Killed
================
*/
void idProjectile::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( spawnArgs.GetBool( "detonate_on_death" ) ) {
		Explode( NULL, true );
		physicsObj.ClearContacts();
		physicsObj.PutToRest();
	} else {
		Fizzle();
	}
}

/*
================
idProjectile::Fizzle
================
*/
void idProjectile::Fizzle( void ) {

	if ( gameLocal.isClient || state == EXPLODED || state == FIZZLED ) {
		return;
	}

	StopSound( SND_CHANNEL_BODY, false );
	StartSound( "snd_fizzle", SND_CHANNEL_BODY, 0, false, NULL );
	
	gameLocal.PlayEffect( spawnArgs, "fx_fuse", GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
	physicsObj.PutToRest();

	// No more fly effects
	StopEffect( "fx_fly" );
	if( flyEffect)	{
		//flyEffect->Event_Remove();
	}

	Hide();
	FreeLightDef();

	state = FIZZLED;

 	CancelEvents( &EV_Fizzle );
	PostEventMS( &EV_Remove, spawnArgs.GetInt( "remove_time", "1500" ) );
}

/*
================
idProjectile::Event_RadiusDamage
================
*/
void idProjectile::Event_RadiusDamage( idEntity *ignore ) {
	const char *splash_damage = spawnArgs.GetString( "def_splash_damage" );
	if ( splash_damage[0] != '\0' ) {
		gameLocal.RadiusDamage( physicsObj.GetOrigin(), this, owner, ignore, this, splash_damage, damagePower, &hitCount );
	}
}

/*
================
idProjectile::Event_ResidualDamage
================
*/
void idProjectile::Event_ResidualDamage ( idEntity* ignore ) {
	const char *residual_damage = spawnArgs.GetString( "def_residual_damage" );
	if ( residual_damage[0] != '\0' ) {
		gameLocal.RadiusDamage( physicsObj.GetOrigin(), this, owner, ignore, this, residual_damage, damagePower, &hitCount );
	}

	// Keep the loop going
	PostEventSec ( &EV_ResidualDamage, spawnArgs.GetFloat ( "delay_residual" ), ignore );
}

/*
================
idProjectile::Explode
================
*/
void idProjectile::Explode( const trace_t *collision, const bool showExplodeFX, idEntity *ignore, const char *sndExplode ) {
	idVec3		normal, endpos;
	int			removeTime;

	if ( state == EXPLODED || state == FIZZLED ) {
		return;
	}

	if ( spawnArgs.GetVector( "detonation_axis", "", normal ) ) {
		GetPhysics()->SetAxis( normal.ToMat3() );
	} else {
		normal = collision ? collision->c.normal : idVec3( 0, 0, 1 );
	}
	endpos = ( collision ) ? collision->endpos : GetPhysics()->GetOrigin();

	removeTime = spawnArgs.GetInt( "remove_time", "1500" );

	// play sound
	StopSound( SND_CHANNEL_BODY, false );
	StartSound( sndExplode, SND_CHANNEL_BODY, 0, false, NULL );

	if ( showExplodeFX ) {
		idVec3 fxDir;
		if ( physicsObj.GetGravityNormal( ) != vec3_zero ) {
			fxDir = -physicsObj.GetGravityNormal( );
		} else { 
			fxDir = -physicsObj.GetLinearVelocity( );
			fxDir.Normalize( );
		}
		// FIXME: This should be done in a better way
		PlayDetonateEffect( endpos, fxDir.ToMat3() );
	}

	// Stop the fly effect without destroying particles to ensure the trail within can persist.
	StopEffect( "fx_fly" );	
	if( flyEffect)	{
		//flyEffect->Event_Remove();
	}
	
	// Stop the remaining particles
	StopAllEffects( );

	Hide();
	FreeLightDef();

	GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() + 8.0f * normal );

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	state = EXPLODED;

	if ( gameLocal.isClient ) {
		return;
	}

 	// alert the ai
 	gameLocal.AlertAI( owner.GetEntity() );

	// bind the projectile to the impact entity if necesary
	if ( collision && gameLocal.entities[collision->c.entityNum] && spawnArgs.GetBool( "bindOnImpact" ) ) {
		Bind( gameLocal.entities[collision->c.entityNum], true );
	}

	// splash damage
	removeTime = 0;
	float delay = spawnArgs.GetFloat( "delay_splash" );
	if ( delay ) {
		if ( removeTime < delay * 1000 ) {
			removeTime = ( delay + 0.10 ) * 1000;
		}
		PostEventSec( &EV_RadiusDamage, delay, ignore );
	} else {
		Event_RadiusDamage( ignore );
	}

	// Residual damage (damage over time)
	delay = SEC2MS ( spawnArgs.GetFloat ( "delay_residual" ) );
	if ( delay > 0.0f ) {
		PostEventMS ( &EV_ResidualDamage, delay, ignore );

		// Keep the projectile around until the residual damage is done		
		delay = SEC2MS ( spawnArgs.GetFloat ( "residual_time" ) );
		if ( removeTime < delay ) {
			removeTime = delay;
		}
	}
			
 	CancelEvents( &EV_Explode );
	PostEventMS( &EV_Remove, removeTime );
}

/*
================
idProjectile::GetVelocity
================
*/
idVec3 idProjectile::GetVelocity( const idDict *projectile ) {
	idVec3 velocity;	

	velocity.Zero ( );
	if ( projectile && !projectile->GetFloat ( "speed", "0", velocity.x ) ) {
		projectile->GetVector( "velocity", "0 0 0", velocity );
	}
	return velocity;
}

/*
================
idProjectile::GetGravity
================
*/
idVec3 idProjectile::GetGravity( const idDict *projectile ) {
	if ( projectile ) {
		return gameLocal.GetGravity ( ) * projectile->GetFloat( "gravity" );
	}
	return vec3_origin;
}

/*
================
idProjectile::PlayPainEffect
================
*/
void idProjectile::PlayPainEffect ( idEntity* ent, int damage, const rvDeclMatType* materialType, const idVec3& origin, const idVec3& dir )  {
	static int  damageTable[] = { 100, 50, 25, 10, 0 };
	int			index;

	// Normalize the damage value to the damage table	
	for ( index = 0; damage < damageTable[index] && damageTable[index]; index ++ );
	
	// loop until we find a pain effect, trying lower damage numbers if needed
	for ( ; damageTable[index]; index ++ ) {
		// Try the pain effect for the  current damage value and if it plays then
		// we are done
		if ( ent->PlayEffect( gameLocal.GetEffect( spawnArgs, va( "fx_pain%d", damageTable[index] ), materialType ), origin, dir.ToMat3()  ) ) {
			return;
		}
	}

	// Play the default pain effect		
	ent->PlayEffect( gameLocal.GetEffect ( spawnArgs, "fx_pain", materialType ), origin, dir.ToMat3() );
}

/*
================
idProjectile::PlayDetonateEffect
================
*/
void idProjectile::PlayDetonateEffect( const idVec3& origin, const idMat3& axis ) {
	if( physicsObj.HasGroundContacts() ) {
		if ( spawnArgs.GetBool( "detonateTestGroundMaterial" ) ) {
			trace_t tr;
			idVec3 down;
			down = GetPhysics()->GetOrigin() + GetPhysics()->GetGravityNormal()*8.0f;
			gameLocal.Translation( this, tr, GetPhysics()->GetOrigin(), down, GetPhysics()->GetClipModel(), GetPhysics()->GetClipModel()->GetAxis(), GetPhysics()->GetClipMask(), this );
			gameLocal.PlayEffect( gameLocal.GetEffect( spawnArgs, "fx_impact", tr.c.materialType ), origin, axis, false, vec3_origin, true );
		} else {
			gameLocal.PlayEffect( spawnArgs, "fx_impact", origin, axis, false, vec3_origin, true );
		}
		return;
	}

	gameLocal.PlayEffect( spawnArgs, "fx_detonate", origin, axis, false, vec3_origin, true );
}

/*
================
idProjectile::Event_Explode
================
*/
void idProjectile::Event_Explode( void ) {
	// events are processed outside of the think loop, so set the current thinking ent appropriately
	idEntity* think = gameLocal.currentThinkingEntity;
	gameLocal.currentThinkingEntity = this;
	Explode( NULL, true );
	gameLocal.currentThinkingEntity = think;
}

/*
================
idProjectile::Event_Fizzle
================
*/
void idProjectile::Event_Fizzle( void ) {
	idEntity* think = gameLocal.currentThinkingEntity;
	gameLocal.currentThinkingEntity = this;
	Fizzle();
	gameLocal.currentThinkingEntity = think;
}

/*
================
idProjectile::Event_Touch
================
*/
void idProjectile::Event_Touch( idEntity *other, trace_t *trace ) {
	if ( IsHidden() ) {
		return;
	}

	if ( other != owner.GetEntity() ) {
		idEntity* think = gameLocal.currentThinkingEntity;
		gameLocal.currentThinkingEntity = this;

		trace_t collision;

		memset( &collision, 0, sizeof( collision ) );
		collision.c.point = GetPhysics()->GetOrigin();
		collision.c.normal.Set( 0, 0, 1 );
		DefaultDamageEffect( collision, collision.c.normal, NULL );
		Explode( NULL, true );

		gameLocal.currentThinkingEntity = think;
	}
}

/*
================
idProjectile::ClientPredictionThink
================
*/
void idProjectile::ClientPredictionThink( void ) {
	if ( !renderEntity.hModel && clientEntities.IsListEmpty() ) {
		return;
	}
	if ( !syncPhysics && state == LAUNCHED ) {
		idMat3 axis = launchDir.ToMat3();
		idVec3 origin( launchOrig );
		origin += ( ( gameLocal.time - launchTime ) / 1000.0f ) * launchSpeed * launchDir;
		physicsObj.SetAxis( axis );
		physicsObj.SetOrigin( origin );
		physicsObj.SetLinearVelocity( launchSpeed * launchDir );
	}
	Think();
}

/*
================
idProjectile::WriteToSnapshot
================
*/
void idProjectile::WriteToSnapshot( idBitMsgDelta &msg ) const {
	if ( syncPhysics ) {
		physicsObj.WriteToSnapshot( msg );
	}

	msg.WriteBits( state, 3 );
	if ( state >= LAUNCHED ) {
		// feed the client with start position, direction and time. let the client do everything else
		// this won't change during projectile life and be completely deltified away
		msg.WriteLong( launchTime );
		msg.WriteFloat( launchOrig[ 0 ] );
		msg.WriteFloat( launchOrig[ 1 ] );
		msg.WriteFloat( launchOrig[ 2 ] );
		msg.WriteDir( launchDir, 24 );
	}
}

/*
================
idProjectile::ReadFromSnapshot
================
*/
void idProjectile::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	projectileState_t newState;

	if ( syncPhysics ) {
		physicsObj.ReadFromSnapshot( msg );
	}

	newState = (projectileState_t) msg.ReadBits( 3 );
	if ( newState >= LAUNCHED ) {
		launchTime = msg.ReadLong();
		launchOrig[ 0 ] = msg.ReadFloat();
		launchOrig[ 1 ] = msg.ReadFloat();
		launchOrig[ 2 ] = msg.ReadFloat();
		launchDir = msg.ReadDir( 24 );
	}
	// we always create and launch at the same time
	// state on a client can be SPAWNED, LAUNCHED, EXPLODED
	// expect never to get a CREATED projectile, they should launch right away
	assert( state != CREATED && state != FIZZLED && newState != CREATED );
	if ( newState != state ) {
		switch ( newState ) {
		case LAUNCHED:
			Create( NULL, launchOrig, launchDir );
			Launch( launchOrig, launchDir, vec3_origin );
			Show();
			break;
		case FIZZLED:
		case EXPLODED:
			if ( state != EXPLODED ) {
				StopSound( SND_CHANNEL_BODY, false );
				StopAllEffects();
				Hide();
				FreeLightDef();
				state = EXPLODED;
			}
			break;
		}
	}

	if ( msg.HasChanged() ) {
		if( !syncPhysics && state == LAUNCHED ) {
			idMat3 axis = launchDir.ToMat3();
			idVec3 origin( launchOrig );
			origin += ( ( gameLocal.time - launchTime ) / 1000.0f ) * launchSpeed * launchDir;
			physicsObj.SetAxis( axis );
			physicsObj.SetOrigin( origin );
			physicsObj.SetLinearVelocity( launchSpeed * launchDir );
		}
	
		UpdateVisuals();
	}
}

/*
===============
idProjectile::ClientStale
===============
*/
bool idProjectile::ClientStale( void ) {
	// delete stale projectile ents. if they pop back in pvs, they will be re-spawned ( rare case anyway )
	StopAllEffects();
	return true;
}

/*
================
idProjectile::GetPhysicsToVisualTransform
================
*/
bool idProjectile::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	axis = rotation.GetCurrentValue( gameLocal.GetTime() ).ToMat3() * GetPhysics()->GetAxis().Transpose();

	origin.Zero();
	return true;
}

/*
===============================================================================

	idGuidedProjectile

===============================================================================
*/

CLASS_DECLARATION( idProjectile, idGuidedProjectile )
END_CLASS

/*
================
idGuidedProjectile::idGuidedProjectile( void )
================
*/
idGuidedProjectile::idGuidedProjectile( void ) {
	guideType = GUIDE_NONE;
	turn_max.Init ( gameLocal.time, 0, 0.0f, 0.0f );
}

/*
=================
idGuidedProjectile::~idGuidedProjectile
=================
*/
idGuidedProjectile::~idGuidedProjectile() {
}

/*
================
idGuidedProjectile::Save
================
*/
void idGuidedProjectile::Save( idSaveGame *savefile ) const {
	savefile->WriteInt ( guideType );
	guideEnt.Save( savefile );
	savefile->WriteVec3 ( guideDir );
	savefile->WriteVec3 ( guidePos );
	savefile->WriteJoint ( guideJoint );
	savefile->WriteFloat( guideMinDist ); // cnicholson: Added unsaved var

	savefile->WriteInt ( driftTime );
	savefile->WriteInt ( driftRate );
	savefile->WriteFloat ( driftRange );
	savefile->WriteFloat ( driftRadius );
	savefile->WriteFloat ( driftDiversity ); // cnicholson: Added unsaved var
	savefile->WriteFloat ( driftAngle );
	savefile->WriteFloat ( driftAngleStep );
	savefile->WriteFloat ( driftProjectRange );

	savefile->WriteFloat( turn_max.GetStartTime() );
	savefile->WriteFloat( turn_max.GetDuration() );
	savefile->WriteFloat( turn_max.GetStartValue() );
	savefile->WriteFloat( turn_max.GetEndValue() );

	savefile->WriteInt ( launchTime );
	savefile->WriteInt ( guideDelay );
	savefile->WriteInt ( driftDelay );
}

/*
================
idGuidedProjectile::Restore
================
*/
void idGuidedProjectile::Restore( idRestoreGame *savefile ) {
	float set;
	savefile->ReadInt ( guideType );
	guideEnt.Restore( savefile );
	savefile->ReadVec3 ( guideDir );
	savefile->ReadVec3 ( guidePos );
	savefile->ReadJoint ( guideJoint );
	savefile->ReadFloat( guideMinDist ); // cnicholson: Added unrestored var
	
	savefile->ReadInt ( driftTime );
	savefile->ReadInt ( driftRate );
	savefile->ReadFloat ( driftRange );
	savefile->ReadFloat ( driftRadius );
	savefile->ReadFloat ( driftDiversity ); // cnicholson: Added unrestored var
	savefile->ReadFloat ( driftAngle );
	savefile->ReadFloat ( driftAngleStep );
	savefile->ReadFloat ( driftProjectRange );
	
	savefile->ReadFloat( set );
	turn_max.SetStartTime( set );
	savefile->ReadFloat( set );
	turn_max.SetDuration( set );
	savefile->ReadFloat( set );
	turn_max.SetStartValue( set );
	savefile->ReadFloat( set );
	turn_max.SetEndValue( set );

	savefile->ReadInt ( launchTime );
	savefile->ReadInt ( guideDelay );
	savefile->ReadInt ( driftDelay );
}

/*
================
idGuidedProjectile::GetGuideDir
================
*/
bool idGuidedProjectile::GetGuideDir ( idVec3 &outDir, float& outDist  ) {
	// Dont start guiding immeidately?
	if ( gameLocal.GetTime() - launchTime < guideDelay ) {
		return false;
	}
	
	switch ( guideType ) {
		case GUIDE_ENTITY:
			// If the guide entity is gone or dead then cancel the guide
			if ( !guideEnt.GetEntity ( ) || (guideEnt->fl.takedamage && guideEnt->health <= 0 ) ) {
				CancelGuide ( );
				return false;
			}
			// Use eye position for actors and center of bounds for everything else
			if ( guideJoint != INVALID_JOINT ) {
				idMat3 jointAxis;
				guideEnt->GetAnimator()->GetJointTransform( guideJoint, gameLocal.GetTime(), outDir, jointAxis );
				outDir = guideEnt->GetRenderEntity()->origin + (outDir*guideEnt->GetRenderEntity()->axis);
				if ( !guidePos.Compare( vec3_origin ) ) {
					jointAxis = jointAxis * guideEnt->GetRenderEntity()->axis;
					outDir += jointAxis[0]*guidePos[0];
					outDir += jointAxis[1]*guidePos[1];
					outDir += jointAxis[2]*guidePos[2];
				}
			} else {
				outDir = guideEnt->GetPhysics()->GetAbsBounds().GetCenter();
				if ( guideEnt->IsType( idActor::GetClassType() ) ) {
					outDir += static_cast<idActor *>(guideEnt.GetEntity())->GetEyePosition();
					outDir *= 0.5f;
				}
			}
			outDir -= physicsObj.GetOrigin();
			break;

		case GUIDE_POS:
			outDir = guidePos - physicsObj.GetOrigin();
			break;
			
		case GUIDE_DIR:
			// Project our current position on to the desired direction
			outDir = guidePos + guideDir * ((physicsObj.GetOrigin() - guidePos) * guideDir);
			
			// Seek towards a point forward along our desired direction
			outDir = (outDir + guideDir * (guideMinDist * 1.10f)) - physicsObj.GetOrigin();
			break;
			
		default:
			return false;
	}

	// Add drifting
	if ( driftRate && gameLocal.GetTime() - launchTime > driftDelay ) {		
		idMat3 axis;
		
		outDist = outDir.NormalizeFast();
		axis    = outDir.ToMat3();			
			
		if ( gameLocal.time > driftTime ) {				
			driftRadius	= driftRange + gameLocal.random.RandomFloat ( ) * driftRange * driftDiversity;
			driftTime	= gameLocal.time + driftRate;
		} else {
			driftAngle += driftAngleStep * MS2SEC ( gameLocal.msec );
			idMath::AngleNormalize360 ( driftAngle );
		}

		float angle;
		angle = DEG2RAD ( driftAngle );
		outDir = physicsObj.GetOrigin ( ) + outDir * Min( outDist, driftProjectRange );
		outDir += axis[2] * (driftRadius * idMath::Sin ( angle ));
		outDir += axis[1] * (driftRadius * idMath::Cos ( angle ));

		outDir -= physicsObj.GetOrigin();
	}

	outDist = outDir.Normalize ( );
			
	return true;
}

/*
================
idGuidedProjectile::Think
================
*/
void idGuidedProjectile::Think( void ) {

	if ( state == LAUNCHED ) {
		idVec3	dir;
		idVec3	vel;
		float	angle;
		float	maxangle;
		idMat3	axis;
		float	dist;

		// crank up to normal speed ?
		if ( guideDelay && gameLocal.GetTime() - launchTime >= guideDelay ) {
			float newSpeed = spawnArgs.GetFloat( "speed" );
			float newSpeed2;
			if ( !spawnArgs.GetFloat ( "speed_end", "0", newSpeed2 ) ) {
				newSpeed2 = newSpeed;
			}
			float newSpeedDuration;
			newSpeedDuration = SEC2MS( spawnArgs.GetFloat ( "speed_duration", "0" ) );
			speed.Init ( gameLocal.time, newSpeedDuration, newSpeed, newSpeed2 );
			guideDelay = 0;
		}

		if ( !GetGuideDir( dir, dist ) ) {
			idProjectile::Think();
			return;
		}
						
		// Direction of travel
		vel = physicsObj.GetLinearVelocity();
		vel.Normalize();

		// Calculate the angle between the current projectile direction and where we want to go					
		angle = RAD2DEG( idMath::ACos( dir * vel ) );
								
		// Make sure the angle doesnt cross our max turn radius								
		maxangle = turn_max.GetCurrentValue( gameLocal.time );
		if ( angle < -maxangle ) {
			angle = -maxangle;
		} else if ( angle > maxangle ) {
			angle = maxangle;
		}

		// Debug information
		if ( g_debugWeapon.GetBool ( ) ) {
			gameRenderWorld->DebugArrow( colorCyan, physicsObj.GetOrigin(), physicsObj.GetOrigin() + vel * 50.0f, 10.0f );
			gameRenderWorld->DebugArrow( colorMagenta, physicsObj.GetOrigin(), physicsObj.GetOrigin() + dir * 50.0f, 10.0f );
		}
					
		// Calculate the new axis by rotating the current forward vector around the cross of the forward
		// vector and the direction vector.
		vel = vel * idRotation( vec3_origin, dir.Cross ( vel ), angle );
		physicsObj.SetLinearVelocity( vel * GetSpeed ( ) );

		// If within the minium distance to the target anything over a 45 degree change will cancel the guide
		if ( guideMinDist != 0.0f && dist < guideMinDist ) {
			// Stop guiding if we have passed our target
			vel = physicsObj.GetLinearVelocity ( );
			vel.Normalize( );		
			if ( vel * dir < 0.7f ) {
				guideType = GUIDE_NONE;
			}
		}
		
		idProjectile::Think();						
	} else { 
		idProjectile::Think();
	}
}

/*
=================
idGuidedProjectile::Launch
=================
*/
void idGuidedProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, float dmgPower ) {
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, dmgPower );
	
	launchTime = gameLocal.GetTime();

	if ( owner.GetEntity() ) {
		if ( owner.GetEntity()->IsType( idAI::GetClassType() ) ) {
			GuideTo ( static_cast<idAI *>( owner.GetEntity() )->GetEnemy() );
		}
	}

	guideMinDist	= spawnArgs.GetFloat ( "min_dist", "128" );
	guideDelay		= SEC2MS(spawnArgs.GetFloat ( "delayGuide" ) + ( gameLocal.random.RandomFloat ( ) * spawnArgs.GetFloat ( "delayGuide_random")) );

	if ( guideDelay ) {
		float delaySpeed;
		if ( spawnArgs.GetFloat( "delaySpeed", "0", delaySpeed ) )	{
			float delaySpeed2;
			if ( !spawnArgs.GetFloat ( "delaySpeed_end", "0", delaySpeed2 ) ) {
				delaySpeed2 = delaySpeed;
			}
			float delaySpeedDuration;
			delaySpeedDuration = SEC2MS( spawnArgs.GetFloat ( "delaySpeed_duration", "0" ) );
			speed.Init( gameLocal.time, delaySpeedDuration, delaySpeed, delaySpeed2 );
			physicsObj.SetLinearVelocity( dir * speed.GetCurrentValue(gameLocal.time) + pushVelocity );
		}
	}

	driftTime		= 0;
	driftRate		= SEC2MS ( spawnArgs.GetFloat ( "driftRate", "0" ) );
	driftRange		= spawnArgs.GetFloat ( "driftRange" );
	driftDiversity	= spawnArgs.GetFloat ( "driftDiversity", ".5" );
	driftAngle		= gameLocal.random.RandomFloat ( ) * 360.0f;
	driftAngleStep	= spawnArgs.GetFloat ( "driftRotate" );
	driftAngleStep += gameLocal.random.CRandomFloat ( ) * (driftAngleStep * driftDiversity) * (gameLocal.random.RandomFloat()<0.5f?-1.0f:1.0f);
	driftDelay		= SEC2MS(spawnArgs.GetFloat ( "driftDelay" ));
	
	driftProjectRange = spawnArgs.GetFloat ( "driftProjectRange", "128" );

	// Turn rate can be ramped up over time
	turn_max.Init ( gameLocal.time, 
					SEC2MS(spawnArgs.GetFloat ( "turn_accel", "0" )), 
					0, 
					spawnArgs.GetFloat( "turn_max", "180" ) / ( float )gameLocal.GetMHz() );

 	UpdateVisuals();
}

/*
=================
idGuidedProjectile::Launch
=================
*/
void idGuidedProjectile::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( guideEnt.IsValid() ) {
		guideEnt->GuidedProjectileIncoming( NULL );
	}
	idProjectile::Killed( inflictor, attacker, damage, dir, location );
}

/*
===============================================================================

	rvDriftingProjectile

===============================================================================
*/

CLASS_DECLARATION( idProjectile, rvDriftingProjectile )
END_CLASS

/*
================
rvDriftingProjectile::rvDriftingProjectile( void )
================
*/
rvDriftingProjectile::rvDriftingProjectile( void ) {
}

/*
=================
rvDriftingProjectile::~rvDriftingProjectile
=================
*/
rvDriftingProjectile::~rvDriftingProjectile ( void ) {
}

/*
================
rvDriftingProjectile::Save
================
*/
void rvDriftingProjectile::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3 ( startDir );
	savefile->WriteVec3 ( startOrigin );
	savefile->WriteMat3 ( startAxis );
	savefile->WriteFloat ( startSpeed );
	savefile->WriteFloat ( driftOffsetMax );
	savefile->WriteFloat ( driftSpeedMax );
	savefile->WriteFloat ( driftTime );

	savefile->WriteInterpolate( driftSpeed );
	for( int ix = 0; ix < 2; ++ix ) {
		savefile->WriteInterpolate( driftOffset[ix] );
	}
}

/*
================
rvDriftingProjectile::Restore
================
*/
void rvDriftingProjectile::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3 ( startDir );
	savefile->ReadVec3 ( startOrigin );
	savefile->ReadMat3 ( startAxis );
	savefile->ReadFloat ( startSpeed );
	savefile->ReadFloat ( driftOffsetMax );
	savefile->ReadFloat ( driftSpeedMax );
	savefile->ReadFloat ( driftTime );

	savefile->ReadInterpolate( driftSpeed );
	for( int ix = 0; ix < 2; ++ix ) {
		savefile->ReadInterpolate( driftOffset[ix] );
	}
}

/*
================
rvDriftingProjectile::Think
================
*/
void rvDriftingProjectile::Think( void ) {
	idVec3 diff;
	idVec3 origin;
	idVec3 oldOrigin;
	idVec3 dir;
	float  dist;
	
	oldOrigin = GetPhysics()->GetOrigin ( );
	
	diff   = oldOrigin - startOrigin;
	dist   = diff.Length ( );
	origin = startOrigin + startDir * dist;
	
	if ( driftSpeed.IsDone ( gameLocal.time ) ) {
		driftSpeed.Init ( gameLocal.time, driftTime / 4.0f, driftTime / 4.0f, driftTime + gameLocal.random.RandomFloat() * driftTime,
						  driftSpeed.GetCurrentValue ( gameLocal.time ),
						  gameLocal.random.RandomFloat() * driftSpeedMax * (driftSpeed.GetCurrentValue ( gameLocal.time )<0?1:-1) );
	}
	
	if ( driftOffset[0].IsDone ( gameLocal.time ) ) {
		driftOffset[0].Init ( gameLocal.time, driftTime / 4.0f, driftTime / 4.0f, driftTime + gameLocal.random.RandomFloat() * driftTime, 
						driftOffset[0].GetCurrentValue ( gameLocal.time ),
						gameLocal.random.RandomFloat() * driftOffsetMax * (driftOffset[0].GetCurrentValue ( gameLocal.time )<0?1:-1) );
	}

	if ( driftOffset[1].IsDone ( gameLocal.time ) ) {
		driftOffset[1].Init ( gameLocal.time, driftTime / 4.0f, driftTime / 4.0f, driftTime + gameLocal.random.RandomFloat()*driftTime, 
						driftOffset[1].GetCurrentValue ( gameLocal.time ),
						gameLocal.random.RandomFloat() * driftOffsetMax * (driftOffset[1].GetCurrentValue ( gameLocal.time )<0?1:-1) );
	}

	origin += startAxis[1] * driftOffset[0].GetCurrentValue ( gameLocal.time );
	origin += startAxis[2] * driftOffset[1].GetCurrentValue ( gameLocal.time );
	
	GetPhysics ( )->SetOrigin ( origin );
	GetPhysics ( )->SetLinearVelocity ( startDir * (startSpeed + driftSpeed.GetCurrentValue ( gameLocal.time )) );
	
	idProjectile::Think();

	// Now orient the projectile using the old origin
	dir = GetPhysics()->GetOrigin() - oldOrigin;
	dir.Normalize ( );	
	GetPhysics ( )->SetAxis ( dir.ToMat3 ( ) );
}

/*
=================
rvDriftingProjectile::Launch
=================
*/
void rvDriftingProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, float dmgPower) {
	startDir	= dir;	
	startOrigin = start;
	startAxis   = dir.ToMat3();

	driftOffsetMax	= spawnArgs.GetFloat ( "driftOffset", "50" );
	driftSpeedMax	= spawnArgs.GetFloat ( "driftSpeed", "50" );
	driftTime	= SEC2MS ( spawnArgs.GetFloat ( "driftTime", ".5" ) );

	idProjectile::Launch ( start, dir, pushVelocity, timeSinceFire, dmgPower );

	startSpeed = GetPhysics()->GetLinearVelocity().Length ( );
}

/*
=================
rvDriftingProjectile::Launch
=================
*/
void rvDriftingProjectile::UpdateVisualAngles ( void ) {
	rotation.Init( gameLocal.GetTime(), 0.0f, rotation.GetCurrentValue(gameLocal.GetTime()), GetPhysics()->GetAxis().ToQuat()  );
}

/*
===============================================================================

	rvSpawnerProjectile

===============================================================================
*/

CLASS_DECLARATION( idProjectile, rvSpawnerProjectile )
	EVENT( EV_PostSpawn,		rvSpawnerProjectile::Event_PostSpawn )
END_CLASS

/*
================
rvSpawnerProjectile::rvSpawnerProjectile( void )
================
*/
rvSpawnerProjectile::rvSpawnerProjectile( void ) {
	spawnState = STATE_NONE;
}

/*
=================
rvSpawnerProjectile::~rvSpawnerProjectile
=================
*/
rvSpawnerProjectile::~rvSpawnerProjectile ( void ) {
	if ( spawnState == STATE_ADDED && spawner ) {
		spawner->RemoveSpawnPoint ( this );
	}
}

/*
=================
rvSpawnerProjectile::SetSpawner
=================
*/
void rvSpawnerProjectile::Spawn ( void ) {
	if ( *spawnArgs.GetString ( "spawner" ) ) {
		PostEventMS ( &EV_PostSpawn, 0 );
	}
}

/*
=================
rvSpawnerProjectile::SetSpawner
=================
*/
void rvSpawnerProjectile::SetSpawner ( rvSpawner* _spawner ) {
	spawner = _spawner;
}

/*
=================
rvSpawnerProjectile::Think
=================
*/
void rvSpawnerProjectile::Think ( void ) {
	idProjectile::Think ( );
	
	if ( physicsObj.IsAtRest ( ) ) {
		if ( spawnState == STATE_NONE && spawner ) {
			spawner->AddSpawnPoint ( this );
			spawnState = STATE_ADDED;
		}
	}
}

/*
=================
rvSpawnerProjectile::Event_PostSpawn
=================
*/
void rvSpawnerProjectile::Event_PostSpawn ( void ) {
	const char* temp;
	temp = spawnArgs.GetString ( "spawner" );
	if ( temp && *temp ) {
		idEntity* ent;
		ent = gameLocal.FindEntity ( temp );
		if ( !ent ) {
			gameLocal.Warning ( "spawner entity ('%s') not found for rvSpawnerProjectile '%s'", temp, GetName ( ) ); 
		} else if ( !ent->IsType ( rvSpawner::GetClassType() ) )  {
			gameLocal.Warning ( "spawner entity ('%s') is not of type rvSpawner for rvSpawnerProjectile '%s'", temp, GetName ( ) ); 
		} else {
			SetSpawner ( static_cast<rvSpawner*>(ent) );
		}
	}
}


/*
===============================================================================

	rvMIRVProjectile

===============================================================================
*/
idEventDef		EV_LaunchWarheads( "launchWarheads" );

CLASS_DECLARATION( idProjectile, rvMIRVProjectile )
	EVENT( EV_LaunchWarheads,		rvMIRVProjectile::Event_LaunchWarheads )
END_CLASS

/*
================
rvMIRVProjectile::rvMIRVProjectile( void )
================
*/
rvMIRVProjectile::rvMIRVProjectile( void ) {

}

/*
=================
rvMIRVProjectile::~rvMIRVProjectile
=================
*/
rvMIRVProjectile::~rvMIRVProjectile ( void ) {
	
}

/*
================
void rvMIRVProjectile::Spawn( void ) 
================
*/
void rvMIRVProjectile::Spawn( void ) {
	
	float launchDelay = spawnArgs.GetFloat("warhead_fuse", "0");
	//post event for warhead launch
	PostEventSec( &EV_LaunchWarheads, launchDelay );
}

/*
================
void rvMIRVProjectile::Event_LaunchWarheads( void )
================
*/
void rvMIRVProjectile::Event_LaunchWarheads( void ) {

	const char* warhead;
	int count;
	
	warhead = spawnArgs.GetString("def_warhead","");
	count = spawnArgs.GetFloat("warhead_count","0");

	if( warhead && warhead[0] )	{
		
		//start launching!
		float angle = (360.0f / count);
		idMat3 normalMat = this->GetPhysics()->GetAxis( );
		idVec3 normal = normalMat[0];
		idVec3 axis = normalMat[1];

		float t;
		idMat3 axisMat;
		idProjectile * warheadEntity;
		
		//hey how 'bout that.
		normal.Normalize();

		for( t = 0; t< count; t++)	{
			
			//rotate axis around normal by angle degrees.
			axisMat = axis.ToMat3();
			axisMat.RotateArbitrary( normal, angle * t);
			warheadEntity =  static_cast<idProjectile*>(gameLocal.SpawnEntityDef( warhead));
			warheadEntity->Create( this->GetOwner(), this->GetPhysics()->GetOrigin(), axisMat[0], this );
			warheadEntity->Launch( this->GetPhysics()->GetOrigin(), axisMat[0], axisMat[0] );

		}
		//foom!
		gameLocal.PlayEffect( spawnArgs, "fx_impact", GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
		PostEventSec( &EV_Explode, 0 );
	}



}
// RAVEN END
