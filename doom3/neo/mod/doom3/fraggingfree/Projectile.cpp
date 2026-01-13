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
#include "framework/DeclEntityDef.h"
#include "renderer/ModelManager.h"

#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"
#include "ai/AI.h"
#include "Player.h"
#include "Mover.h"
#include "SmokeParticles.h"
#include "Misc.h"
#include "Camera.h"
#include "BrittleFracture.h"
#include "Moveable.h"

#include "Projectile.h"

/*
===============================================================================

	idProjectile

===============================================================================
*/


static const int BFG_DAMAGE_FREQUENCY			= 333;
static const int BFG_SEARCH_FREQUENCY			= 100; //ff1.3 - was 333
static const float BOUNCE_SOUND_MIN_VELOCITY	= 200.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 400.0f;

const idEventDef EV_Explode( "forceExplosion", NULL ); //ff1.3: exposed to scripts - was: "<explode>"
const idEventDef EV_Fizzle( "<fizzle>", NULL );
const idEventDef EV_RadiusDamage( "<radiusdmg>", "e" );
const idEventDef EV_GetProjectileState( "getProjectileState", NULL, 'd' );

#ifdef _D3XP
const idEventDef EV_CreateProjectile( "projectileCreateProjectile", "evv" );
const idEventDef EV_LaunchProjectile( "projectileLaunchProjectile", "vvv" );
const idEventDef EV_SetGravity( "setGravity", "f" );
#endif
//ff1.3 start
const idEventDef EV_Projectile_GetOwner( "getOwner", NULL, 'e' );
//ff1.3 end

CLASS_DECLARATION( idEntity, idProjectile )
	EVENT( EV_Explode,				idProjectile::Event_Explode )
	EVENT( EV_Fizzle,				idProjectile::Event_Fizzle )
	EVENT( EV_Touch,				idProjectile::Event_Touch )
	EVENT( EV_RadiusDamage,			idProjectile::Event_RadiusDamage )
	EVENT( EV_GetProjectileState,	idProjectile::Event_GetProjectileState )
#ifdef _D3XP
	EVENT( EV_CreateProjectile,		idProjectile::Event_CreateProjectile )
	EVENT( EV_LaunchProjectile,		idProjectile::Event_LaunchProjectile )
	EVENT( EV_SetGravity,			idProjectile::Event_SetGravity )
#endif
	//ff1.3 start
	EVENT( EV_SetOwner,				idProjectile::Event_SetOwner ) //override idEntity event handler
	EVENT( EV_Projectile_GetOwner,	idProjectile::Event_GetOwner )
	//ff1.3 end
END_CLASS

/*
================
idProjectile::idProjectile
================
*/
idProjectile::idProjectile( void ) {
	owner				= NULL;
	lightDefHandle		= -1;
	thrust				= 0.0f;
	thrust_end			= 0;
	smokeFly			= NULL;
	smokeFlyTime		= 0;
	state				= SPAWNED;
	lightOffset			= vec3_zero;
	lightStartTime		= 0;
	lightEndTime		= 0;
	lightColor			= vec3_zero;
	state				= SPAWNED;
	hitCountGroupId		= 0; //ff1.3
	damagePower			= 1.0f;
	damageDef			= NULL;		// new

	memset( &projectileFlags, 0, sizeof( projectileFlags ) );
	memset( &renderLight, 0, sizeof( renderLight ) );

#ifdef _DENTONMOD
	tracerEffect = NULL;
#endif

	// note: for net_instanthit projectiles, we will force this back to false at spawn time
	fl.networkSync		= true;

	netSyncPhysics		= false;
}

/*
================
idProjectile::Spawn
================
*/
void idProjectile::Spawn( void ) {
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetContents( 0 );
	physicsObj.SetClipMask( 0 );
	physicsObj.PutToRest();
	SetPhysics( &physicsObj );
}

/*
================
idProjectile::Save
================
*/
void idProjectile::Save( idSaveGame *savefile ) const {

	owner.Save( savefile );

	projectileFlags_s flags = projectileFlags;
	LittleBitField( &flags, sizeof( flags ) );
	savefile->Write( &flags, sizeof( flags ) );

	savefile->WriteFloat( thrust );
	savefile->WriteInt( thrust_end );

	savefile->WriteRenderLight( renderLight );
	savefile->WriteInt( (int)lightDefHandle );
	savefile->WriteVec3( lightOffset );
	savefile->WriteInt( lightStartTime );
	savefile->WriteInt( lightEndTime );
	savefile->WriteVec3( lightColor );

	savefile->WriteParticle( smokeFly );
	savefile->WriteInt( smokeFlyTime );

#ifdef _D3XP
	savefile->WriteInt( originalTimeGroup );
#endif

	savefile->WriteInt( (int)state );
	savefile->WriteInt( hitCountGroupId ); //ff1.3

	savefile->WriteFloat( damagePower );

	savefile->WriteStaticObject( physicsObj );
	savefile->WriteStaticObject( thruster );
}

/*
================
idProjectile::Restore
================
*/
void idProjectile::Restore( idRestoreGame *savefile ) {

	owner.Restore( savefile );

	savefile->Read( &projectileFlags, sizeof( projectileFlags ) );
	LittleBitField( &projectileFlags, sizeof( projectileFlags ) );

	savefile->ReadFloat( thrust );
	savefile->ReadInt( thrust_end );

	savefile->ReadRenderLight( renderLight );
	savefile->ReadInt( (int &)lightDefHandle );
	savefile->ReadVec3( lightOffset );
	savefile->ReadInt( lightStartTime );
	savefile->ReadInt( lightEndTime );
	savefile->ReadVec3( lightColor );

	savefile->ReadParticle( smokeFly );
	savefile->ReadInt( smokeFlyTime );

#ifdef _D3XP
	savefile->ReadInt( originalTimeGroup );
#endif

	savefile->ReadInt( (int &)state );
	savefile->ReadInt( hitCountGroupId ); //ff1.3

	savefile->ReadFloat( damagePower );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadStaticObject( thruster );
	thruster.SetPhysics( &physicsObj );

	if ( smokeFly != NULL ) {
		idVec3 dir;
		dir = physicsObj.GetLinearVelocity();
		dir.NormalizeFast();
		gameLocal.smokeParticles->EmitSmoke( smokeFly, gameLocal.time, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

#ifdef _D3XP
	if ( lightDefHandle >= 0 ) {
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}
#endif

	//Reinitialize the damage Def--- By Clone JC Denton
	damageDef = gameLocal.FindEntityDef( spawnArgs.GetString( "def_damage" ), false );

}

//ff start
/*
================
idProjectile::Event_SetOwner
================
*/
void idProjectile::Event_SetOwner( idEntity *owner ) {
	SetOwner( owner );
	this->owner = owner; //extra line for Projectiles
}

/*
===============
idProjectile::Event_GetOwner
===============
*/
void idProjectile::Event_GetOwner( void ) {
	idThread::ReturnEntity( GetOwner() );
}
//ff end

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
idProjectile::Create
================
*/
void idProjectile::Create( idEntity *owner, const idVec3 &start, const idVec3 &dir, int hitCountGroupId ) {
	idDict		args;
	idStr		shaderName;
	idVec3		light_color;
//	idVec3		light_offset; // was delcared unnecessarily, removed by Clone JC Denton
	idVec3		tmp;
	idMat3		axis;

	Unbind();

	// align z-axis of model with the direction
	axis = dir.ToMat3();
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	physicsObj.SetOrigin( start );
	physicsObj.SetAxis( axis );

	physicsObj.GetClipModel()->SetOwner( owner );

	this->owner = owner;
	//ff1.3 start
	if ( hitCountGroupId == 0 && spawnArgs.GetBool( "hitCountGroup" ) ) {
		this->hitCountGroupId = gameLocal.time;
	} else {
		this->hitCountGroupId = hitCountGroupId;
	}
	//ff1.3 end

	memset( &renderLight, 0, sizeof( renderLight ) );
	shaderName = spawnArgs.GetString( "mtr_light_shader" );
	if ( *(const char *)shaderName ) {
		renderLight.shader = declManager->FindMaterial( shaderName, false );
		renderLight.pointLight = true;
		renderLight.noShadows = spawnArgs.GetBool( "noLightShadows" ); //ff1.3
		renderLight.lightRadius[0] =
		renderLight.lightRadius[1] =
		renderLight.lightRadius[2] = spawnArgs.GetFloat( "light_radius" );
		spawnArgs.GetVector( "light_color", "1 1 1", light_color );
		renderLight.shaderParms[0] = light_color[0];
		renderLight.shaderParms[1] = light_color[1];
		renderLight.shaderParms[2] = light_color[2];
		renderLight.shaderParms[3] = 1.0f;
	}

	spawnArgs.GetVector( "light_offset", "0 0 0", lightOffset );

	lightStartTime = 0;
	lightEndTime = 0;
	smokeFlyTime = 0;

	damagePower = 1.0f;
	damageDef = NULL;				// New
#ifdef _D3XP
	if(spawnArgs.GetBool("reset_time_offset", "0")) {
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}
#endif

	UpdateVisuals();

	state = CREATED;

	if ( spawnArgs.GetBool( "net_fullphysics" ) ) {
		netSyncPhysics = true;
	}

	//ff1.3 start
	// beam projectiles are ignored by forcefields
	if ( spawnArgs.GetBool( "rail_beam" ) ) {
		projectileFlags.ignoreForceField = true;
	}
	//ff1.3 end
}

/*
=================
idProjectile::~idProjectile
=================
*/
idProjectile::~idProjectile() {
	StopSound( SND_CHANNEL_ANY, false );
	FreeLightDef();

#ifdef _DENTONMOD
	if( tracerEffect ) {
		delete tracerEffect;
	}
#endif
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
void idProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	float			fuse;
	float			endthrust;
	idVec3			velocity;
	idAngles		angular_velocity;
	float			linear_friction;
	float			angular_friction;
	float			contact_friction;
	float			bounce;
	float			mass;
	float			speed;
	float			gravity;
	idVec3			gravVec;
	idVec3			tmp;
	idMat3			axis;
	int				contents;
	int				clipMask;
	idEntity *		ownerEnt;

	// allow characters to throw projectiles during cinematics, but not the player
	ownerEnt = owner.GetEntity();
	if ( ownerEnt && !ownerEnt->IsType( idPlayer::Type ) ) {
		cinematic = ownerEnt->cinematic;
		/*
		if ( !spawnArgs.GetBool( "cinematic", "0", cinematic ) ) { //allow proj def to override monster's cinematic
			cinematic = ownerEnt->cinematic;
		}
		*/
		//ff1.3 start
		if ( ownerEnt->IsType( idActor::Type ) && (static_cast<idActor *>(ownerEnt)->team == 0) ) {
			projectileFlags.firedByFriend = true;
		}
		//ff1.3 end
	} else { //player - ff1.1 fix: allow player proj if specified.
		cinematic = spawnArgs.GetBool( "cinematic" ); //ff1.1 - was: cinematic = false;
	}

	thrust				= spawnArgs.GetFloat( "thrust" );
	endthrust			= spawnArgs.GetFloat( "thrust_end" );

	spawnArgs.GetVector( "velocity", "0 0 0", velocity );

	speed = velocity.Length() * launchPower;

	damagePower = dmgPower;

	spawnArgs.GetAngles( "angular_velocity", "0 0 0", angular_velocity );

	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	angular_friction	= spawnArgs.GetFloat( "angular_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );
	fuse				= spawnArgs.GetFloat( "fuse" );

	//projectileFlags.detonate_on_moveables	= spawnArgs.GetBool( "detonate_on_moveables", "1" ); //ff1.3
	//projectileFlags.detonate_on_ragdoll	= spawnArgs.GetBool( "detonate_on_ragdoll" ); //ff1.3
	projectileFlags.detonate_on_world	= spawnArgs.GetBool( "detonate_on_world" );
	projectileFlags.detonate_on_actor	= spawnArgs.GetBool( "detonate_on_actor" );
	projectileFlags.randomShaderSpin	= spawnArgs.GetBool( "random_shader_spin" );
	//projectileFlags.continuousSmoke		= spawnArgs.GetBool ( "smoke_continuous" );

	if ( mass <= 0 ) {
		gameLocal.Error( "Invalid mass on '%s'\n", GetEntityDefName() );
	}

	thrust *= mass;
	thrust_end = SEC2MS( endthrust ) + gameLocal.time;

	lightStartTime = 0;
	lightEndTime = 0;

	if ( health ) {
		fl.takedamage = true;
	}

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();

	Unbind();

	// align z-axis of model with the direction
	axis = dir.ToMat3();
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	contents = 0;
	if ( spawnArgs.GetBool( "ethereal" ) ) { //ivan
		clipMask = CONTENTS_SOLID;
	} else {
		clipMask = MASK_SHOT_RENDERMODEL;
		if ( spawnArgs.GetBool( "detonate_on_trigger" ) ) {
			contents |= CONTENTS_TRIGGER;
		}
		if ( !spawnArgs.GetBool( "no_contents" ) ) {
			contents |= CONTENTS_PROJECTILE;
			clipMask |= CONTENTS_PROJECTILE;
		}
	}
/*
#ifdef _D3XP
	if ( !idStr::Cmp( this->GetEntityDefName(), "projectile_helltime_killer" ) ) {
		contents = CONTENTS_MOVEABLECLIP;
		clipMask = CONTENTS_MOVEABLECLIP;
	}
#endif
*/
	/*
	//commented out by Denton
	// don't do tracers on client, we don't know origin and direction
	if ( spawnArgs.GetBool( "tracers" ) && gameLocal.random.RandomFloat() > 0.5f ) {
		SetModel( spawnArgs.GetString( "model_tracer" ) );
		projectileFlags.isTracer = true;
	}
	}	*/
	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linear_friction, angular_friction, contact_friction );
	if ( contact_friction == 0.0f ) {
		physicsObj.NoContact();
	}
	physicsObj.SetBouncyness( bounce );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetContents( contents );
	physicsObj.SetClipMask( clipMask );
	physicsObj.SetLinearVelocity( axis[ 2 ] * speed + pushVelocity );
	physicsObj.SetAngularVelocity( angular_velocity.ToAngularVelocity() * axis );
	physicsObj.SetOrigin( start );
	physicsObj.SetAxis( axis );

	thruster.SetPosition( &physicsObj, 0, idVec3( GetPhysics()->GetBounds()[ 0 ].x, 0, 0 ) );

// Find and store the damage def only once- --- New
// place this line before checking the fuse- for beam weapons
	damageDef = gameLocal.FindEntityDef( spawnArgs.GetString( "def_damage" ), false );

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

	const idDict *damageDict = damageDef != NULL ? &damageDef->dict : &spawnArgs;
	idStr sound;

	if ( spawnArgs.GetBool( "tracers" ) ) {
		if( !damageDict->GetString( "snd_tracer", "", sound ) ){
			spawnArgs.GetString( "snd_tracer", "", sound );
		}
#ifdef _DENTONMOD
		if( spawnArgs.GetBool( "launchFromBarrel") ) {
			idStr tracerModel;
			if( spawnArgs.GetString( "beam_skin", NULL ) != NULL ) {	// See if there's a beam_skin
				tracerEffect = new dnBarrelLaunchedBeamTracer( this );
			}
			else if ( tracerEffect == NULL && spawnArgs.GetString( "model_tracer", "", tracerModel ) ){
				SetModel( tracerModel );
			}
		}
#else
		idStr tracerModel;
		if ( spawnArgs.GetString( "model_tracer", "", tracerModel ) ){
			SetModel( tracerModel );
		}
#endif
	} else {
		if( !damageDict->GetString( "snd_fly", "", sound ) ){
			spawnArgs.GetString( "snd_fly", "", sound );
		}
	}

	if ( !sound.IsEmpty() ) { //ff1.3
		StartSoundShader(declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}

	smokeFlyTime = 0;
	const char *smokeName = spawnArgs.GetString( "smoke_fly" );
	if ( *smokeName != '\0' ) {
		smokeFly = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
	}

#ifdef _D3XP
	originalTimeGroup = timeGroup;
#endif

	// used for the plasma bolts but may have other uses as well
	if ( projectileFlags.randomShaderSpin ) {
		float f = gameLocal.random.RandomFloat();
		f *= 0.5f;
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = f;
	}

	UpdateVisuals();

	state = LAUNCHED;
}

/*
================
idProjectile::Think
================
*/
void idProjectile::Think( void ) {

#ifdef _DENTONMOD
	if( state != EXPLODED ) { // update & run physics until projectile is not exploded. -Clone JC Denton
#endif
		if ( thinkFlags & TH_THINK ) {
			if ( thrust && ( gameLocal.time < thrust_end ) ) {
				// evaluate force
				thruster.SetForce( GetPhysics()->GetAxis()[ 0 ] * thrust );
				thruster.Evaluate( gameLocal.time );
			}
	}

	// run physics until projectile is not exploded. -Clone JC Denton
	RunPhysics();
#ifdef _DENTONMOD
	}

	if( tracerEffect ) {
		tracerEffect->Think();
	}
#endif
	Present();

	// add the particles
	if ( smokeFly != NULL && smokeFlyTime && !IsHidden() ) {
		idVec3 dir = -GetPhysics()->GetLinearVelocity();
		dir.Normalize();
#ifdef _D3XP
		SetTimeState ts(originalTimeGroup);
#endif
		if ( !gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup /*_D3XP*/ ) ) {
			smokeFlyTime = gameLocal.time;
		}
	}

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

	//debug
	//gameRenderWorld->DebugBounds( colorRed, GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), gameLocal.msec );
}


/*
=================
idProjectile::Collide
=================
*/
bool idProjectile::Collide( const trace_t &collision, const idVec3 &velocity ) {
	idEntity	*ent;
	idEntity	*ignore;
	const char	*damageDefName;
	idVec3		dir;
	float		push;
	float		damageScale;

	if ( state == EXPLODED || state == FIZZLED ) {
		return true;
	}

	// predict the explosion
	if ( gameLocal.isClient ) {
		if ( ClientPredictionCollide( this, damageDef!=NULL ? damageDef->dict: spawnArgs, collision, velocity, !spawnArgs.GetBool( "net_instanthit" ) ) ) {
			Explode( collision, NULL );
			return true;
		}
		return false;
	}

#ifdef _DENTONMOD
	if( tracerEffect!= NULL && tracerEffect->IsType( dnRailBeam::Type() ) ) {
		//gameLocal.Printf("%s : Create Rail\n", GetName() );
		static_cast<dnRailBeam *>( tracerEffect )->Create( collision.c.point );
	}
#endif

	// remove projectile when a 'noimpact' surface is hit
	if ( ( collision.c.material != NULL ) && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) && !spawnArgs.GetBool("rail_beam") && projectileFlags.detonate_on_world ) { //ff1.3 - allow rail_beam to hit SURF_NOIMPACT
		PostEventMS( &EV_Remove, 0 );
		common->DPrintf( "Projectile collision no impact\n" );
		return true;
	}

	// get the entity the projectile collided with
	ent = gameLocal.entities[ collision.c.entityNum ];
	if ( ent == owner.GetEntity() ) {
		assert( 0 );
		return true;
	}

	// just get rid of the projectile when it hits a player in noclip
	if ( ent->IsType( idPlayer::Type ) && static_cast<idPlayer *>( ent )->noclip ) {
		PostEventMS( &EV_Remove, 0 );
		return true;
	}

	// direction of projectile
	dir = velocity;
	dir.Normalize();

	// projectiles can apply an additional impulse next to the rigid body physics impulse
	if ( spawnArgs.GetFloat( "push", "0", push ) && push > 0.0f ) {
		ent->ApplyImpulse( this, collision.c.id, collision.c.point, push * dir );
	}

	// MP: projectiles open doors
	if ( gameLocal.isMultiplayer && ent->IsType( idDoor::Type ) && !static_cast< idDoor * >(ent)->IsOpen() && !ent->spawnArgs.GetBool( "no_touch" ) ) {
		ent->ProcessEvent( &EV_Activate , this );
	}

	if ( ent->IsType( idActor::Type ) || ( ent->IsType( idAFAttachment::Type ) && static_cast<const idAFAttachment*>(ent)->GetBody()->IsType( idActor::Type ) ) ) {
		if ( !projectileFlags.detonate_on_actor ) {
			return false;
		}
	}

	//ff1.3 start - AF having "bleed" explicitly set to 1 behave like actors
	else if ( (ent->IsType( idAFEntity_Generic::Type ) || ent->IsType( idAFEntity_WithAttachedHead::Type )) && ent->spawnArgs.GetBool( "bleed" )) {
		if ( !projectileFlags.detonate_on_actor ) { //detonate_on_ragdoll
			//gameLocal.Printf("!detonate_on_ragdoll\n");
			return false;
		}
	}
	//ff1.3 end
	else {
		if ( !projectileFlags.detonate_on_world ) {
		//if ( ! (ent->IsType( idMoveable::Type ) ? projectileFlags.detonate_on_moveables : projectileFlags.detonate_on_world) ) { //ff1.3
			if ( !StartSound( "snd_ricochet", SND_CHANNEL_ITEM, 0, true, NULL ) ) {
				float len = velocity.Length();
				if ( len > BOUNCE_SOUND_MIN_VELOCITY ) {
					SetSoundVolume( len > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( len - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) ) );
					StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, true, NULL );
				}
			}
			return false;
		}
	}

	SetOrigin( collision.endpos );
	SetAxis( collision.endAxis );

	// unlink the clip model because we no longer need it
	GetPhysics()->UnlinkClip();

	//damageDefName = spawnArgs.GetString( "def_damage" );
	if (damageDef != NULL) {
		damageDefName = damageDef->GetName();
	}
	else {
		damageDefName = NULL;
	}
	ignore = NULL;

	// if the projectile causes a damage effect
	// The Damage effects were previously applied after applying damage but we are applying before it
	// because we wanted it so in idAnimatedEntity::AddLocalDamageEffect - By Clone JCD

	if ( spawnArgs.GetBool( "impact_damage_effect" ) ) {
		// if the hit entity has a special damage effect
#ifdef _DENTONMOD_PROJECTILE_CPP
		StopSound( SND_CHANNEL_BODY, false );	// stop projectile flying sound upon impact, useful when is a looping sound.
												// FIXME: need to restart this sound when projectile is bouncing off of surfaces

		if ( (ent->IsType(idBrittleFracture::Type) || ent->IsType(idAnimatedEntity::Type) || ent->IsType(idMoveable::Type) || ent->IsType(idMoveableItem::Type)) && ent->spawnArgs.GetBool( "bleed", "1" ) ) {	// This ensures that if an entity does not have bleed key defined, it will be considered true by default
			projectileFlags.impact_fx_played = true;
			ent->AddDamageEffect( collision, velocity, damageDefName, this );
#else
		if ( ent->spawnArgs.GetBool( "bleed" ) ) {
			ent->AddDamageEffect( collision, velocity, damageDefName );
#endif

		} else {
			AddDefaultDamageEffect( collision, velocity );
		}
	}

	//ff1.3 start - combo
	//gameLocal.Printf( "hit : %s\n", ent->GetName() );

	//if the projectile hits a combo entity:
	//1) get its bindmaster, which should be another projectile.
	//2) change some spawnArgs on the other projectile so that the exposion and the damage will be different
	//3) force explosion of other proj and notify my owner (player)
	if ( ent->IsType( idComboEntity::Type ) && spawnArgs.GetBool("combo_enabled") ) {
		//gameLocal.Printf( "%s has combo_entity\n", ent->GetName() );
		idEntity *otherProj = ent->GetBindMaster();
		if ( otherProj && otherProj->IsType( idProjectile::Type )) {
			//gameLocal.Printf( "has master %s \n", otherProj->GetName() );
			otherProj->spawnArgs.Set("model_detonate",		otherProj->spawnArgs.GetString("model_combo") );
			otherProj->spawnArgs.Set("def_splash_damage",	otherProj->spawnArgs.GetString("def_combo_damage") );
			otherProj->PostEventMS( &EV_Explode, 0 );

			// if the projectile owner is a player
			if ( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) ) {
				idPlayer *player = static_cast<idPlayer *>( owner.GetEntity() );
				player->AddProjectileHits( 2, hitCountGroupId ); //1 for primary, 1 for secondary
				player->ComboCallback( this );
			}
		}
	}
	//ff1.3 end

	// if the hit entity takes damage
	if ( ent->fl.takedamage ) {
		if ( damagePower ) {
			damageScale = damagePower;
		} else {
			damageScale = 1.0f;
		}

		// if the projectile owner is a player
		if ( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) ) {

			// if the projectile hit an actor
			if ( ent->IsType( idActor::Type ) ) {
				idPlayer *player = static_cast<idPlayer *>( owner.GetEntity() );
				player->AddProjectileHits( 1, hitCountGroupId );
				damageScale *= player->PowerUpModifier( PROJECTILE_DAMAGE );
			}
		}

		if ( damageDefName && *damageDefName ) {
			//gameLocal.Printf("Collide Damaging : %s\n", ent->GetName() );
			ent->Damage( this, owner.GetEntity(), dir, damageDefName, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) );
			if ( spawnArgs.GetBool ("ignore_splash_damage", "1") ) { // Added by Clone JCD for letting projectile def decide the ignore behaviour.
				ignore = ent;
			}
		}
	}
/*
	// if the projectile causes a damage effect
	if ( spawnArgs.GetBool( "impact_damage_effect" ) ) {
		// if the hit entity has a special damage effect
		if ( ent->spawnArgs.GetBool( "bleed" ) ) {
			ent->AddDamageEffect( collision, velocity, damageDefName );
		} else {
			AddDefaultDamageEffect( collision, velocity );
		}
	}
*/
	Explode( collision, ignore );

	return true;
}

/*
=================
idProjectile::DefaultDamageEffect
=================
*/
void idProjectile::DefaultDamageEffect( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity ) {
	const char *decal, *sound, *typeName;
	surfTypes_t materialType;

	if ( collision.c.material != NULL ) {
		materialType = collision.c.material->GetSurfaceType();
	} else {
		materialType = SURFTYPE_METAL;
	}

	// get material type name
	typeName = gameLocal.sufaceTypeNames[ materialType ];

	// play impact sound
	sound = projectileDef.GetString( va( "snd_%s", typeName ) );
	if ( *sound == '\0' ) {
		sound = projectileDef.GetString( "snd_metal" );
	}
	if ( *sound == '\0' ) {
		sound = projectileDef.GetString( "snd_impact" );
	}
	if ( *sound != '\0' ) {
		soundEnt->StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}

	// project decal
	// Note that decal info is taken from projectile def, as projectDecal and projectOverlay work differently.

	decal =projectileDef.GetString( va( "mtr_wound_%s", typeName ) );

	if ( g_debugDamage.GetBool() && collision.c.material != NULL ) { // If this check is not performed game may crash at ocassions
		gameLocal.Printf("\n Collision Material Type: %s", typeName);
		gameLocal.Printf("\n File: %s", collision.c.material->GetFileName());
		gameLocal.Printf("\n Collision material: %s", collision.c.material->ImageName());
	}

	if ( *decal == '\0' ) {
		decal = projectileDef.GetString( "mtr_wound" ); // Default decal
	}
	if ( *decal != '\0' ) {
		float size;
		if ( !projectileDef.GetFloat( va( "size_wound_%s", typeName ), "6.0", size ) ) { // If Material Specific decal size not found, look for default size
			size = projectileDef.GetFloat( "size_wound", "6.0" );
		}
		if ( size > 0.0f ) {
			gameLocal.ProjectDecal( collision.c.point, -collision.c.normal, 16.0f, true, size, decal ); //ff1.3 - increased distance from 8.0f so flamegrenades project decals
		}
	}
}

/*
=================
idProjectile::AddDefaultDamageEffect
=================
*/
void idProjectile::AddDefaultDamageEffect( const trace_t &collision, const idVec3 &velocity ) {


#ifdef _DENTONMOD
	DefaultDamageEffect( this, damageDef!=NULL? damageDef->dict : spawnArgs, collision, velocity );
#else
	DefaultDamageEffect( this, spawnArgs, collision, velocity );
#endif
	if ( gameLocal.isServer && fl.networkSync ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];
		int			excludeClient;

		if ( spawnArgs.GetBool( "net_instanthit" ) ) {
			excludeClient = owner.GetEntityNum();
		} else {
			excludeClient = -1;
		}

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteFloat( collision.c.point[0] );
		msg.WriteFloat( collision.c.point[1] );
		msg.WriteFloat( collision.c.point[2] );
		msg.WriteDir( collision.c.normal, 24 );
		msg.WriteInt( ( collision.c.material != NULL ) ? gameLocal.ServerRemapDecl( -1, DECL_MATERIAL, collision.c.material->Index() ) : -1 );
		msg.WriteFloat( velocity[0], 5, 10 );
		msg.WriteFloat( velocity[1], 5, 10 );
		msg.WriteFloat( velocity[2], 5, 10 );
		ServerSendEvent( EVENT_DAMAGE_EFFECT, &msg, false, excludeClient );
	}
}

/*
================
idProjectile::Killed
================
*/
void idProjectile::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( spawnArgs.GetBool( "detonate_on_death" ) ) {
		trace_t collision;

		memset( &collision, 0, sizeof( collision ) );
		collision.endAxis = GetPhysics()->GetAxis();
		collision.endpos = GetPhysics()->GetOrigin();
		collision.c.point = GetPhysics()->GetOrigin();
		collision.c.normal.Set( 0, 0, 1 );
		Explode( collision, NULL );
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

	if ( state == EXPLODED || state == FIZZLED ) {
		return;
	}

	StopSound( SND_CHANNEL_BODY, false );
	StartSound( "snd_fizzle", SND_CHANNEL_BODY, 0, false, NULL );

	// fizzle FX
	const char *psystem = spawnArgs.GetString( "smoke_fuse" );
	if ( psystem && *psystem ) {
//FIXME:SMOKE		gameLocal.particles->SpawnParticles( GetPhysics()->GetOrigin(), vec3_origin, psystem );
	}

	// we need to work out how long the effects last and then remove them at that time
	// for example, bullets have no real effects
	if ( smokeFly && smokeFlyTime ) {
		smokeFlyTime = 0;
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
	physicsObj.PutToRest();
#ifdef _DENTONMOD
	BecomeInactive(TH_PHYSICS); // This causes the physics not to update when it's fizzled
#endif
	Hide();
	FreeLightDef();

	state = FIZZLED;

	if ( gameLocal.isClient ) {
		return;
	}

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
		gameLocal.RadiusDamage( physicsObj.GetOrigin(), this, owner.GetEntity(), ignore, this, splash_damage, damagePower, spawnArgs.GetBool( "splash_hitcount" ) ); //ff1.3 - optionally use splash for hitcount
	}

	//ff1.3 start
	const char *shockDefName = spawnArgs.GetString( "def_shockwave" );
	if ( shockDefName[0] != '\0' ) {
		const idDict *shockDef = gameLocal.FindEntityDefDict( shockDefName, false );
		if ( !shockDef ) {
			gameLocal.Warning( "shockwave \'%s\' not found", shockDefName );
		} else {
			idEntity *ent = NULL;
			gameLocal.SpawnEntityDef( *shockDef, &ent );
			ent->SetOrigin( GetPhysics()->GetOrigin() );
			ent->PostEventMS( &EV_Activate, 0, GetOwner() ); //set owner for shockwaves
		}
	}
	//ff1.3 end
}

/*
================
idProjectile::Event_RadiusDamage
================
*/
void idProjectile::Event_GetProjectileState( void ) {
	idThread::ReturnInt( state );
}

/*
================
idProjectile::Explode
================
*/
void idProjectile::Explode( const trace_t &collision, idEntity *ignore ) {
	const char *fxname, *light_shader; //, *sndExplode;
	float		light_fadetime;
	idVec3		normal;
	int			removeTime;

	if ( state == EXPLODED || state == FIZZLED ) {
		return;
	}

	// stop sound
	StopSound( SND_CHANNEL_BODY2, false );

	// play explode sound
	//ff1.3 start - disable multiple explode snd
	//damagePower is now used by rideable monsters and not by the player

	/*
	switch ( ( int ) damagePower ) {
		case 2: sndExplode = "snd_explode2"; break;
		case 3: sndExplode = "snd_explode3"; break;
		case 4: sndExplode = "snd_explode4"; break;
		default: sndExplode = "snd_explode"; break;
	}
	StartSound( sndExplode, SND_CHANNEL_BODY, 0, true, NULL );
	*/
	SetSoundVolume(0.0f); //make sure the sound volume is not overridden by snd_bounce
	StartSound( "snd_explode", SND_CHANNEL_BODY, 0, true, NULL );
	//ff1.3 end

	// we need to work out how long the effects last and then remove them at that time
	// for example, bullets have no real effects
	if ( smokeFly && smokeFlyTime ) {
		smokeFlyTime = 0;
	}

	Hide();
	FreeLightDef();

	if ( spawnArgs.GetVector( "detonation_axis", "", normal ) ) {
		GetPhysics()->SetAxis( normal.ToMat3() );
	}
	else {	//Added by Clone JCD for setting proper direction of fx.
		GetPhysics()->SetAxis( collision.c.normal.ToMat3() );
	}

  //  GetPhysics()->SetOrigin( collision.endpos + 2.0f * collision.c.normal ); // Actual effect starts a little away object.
	GetPhysics()->SetOrigin( collision.endpos + 0.5f * collision.c.normal );// By Clone JC Denton
	// default remove time
	//removeTime = spawnArgs.GetInt( "remove_time", "1500" ); //ff1.3 - moved below: default time depends on explode model

	// change the model, usually to a PRT
	fxname = NULL;
	if ( g_testParticle.GetInteger() == TEST_PARTICLE_IMPACT ) {
		fxname = g_testParticleName.GetString();
	} else if (( collision.c.material == NULL ) || !( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT )) { //ff1.3 - no detonation model on skies
		fxname = spawnArgs.GetString( "model_detonate" );
	}

	if (!projectileFlags.impact_fx_played) { // New flag added by Clone JCD,this wont play damage effects when model_detonate key is in place.
											// which is esp. useful for exploding projectiles like rockets, grenades etc.
		if ( !( fxname && *fxname ) ) {

			// fx shall be played from def from now on------- By Clone JCD
			if (damageDef != NULL) {

				int type = collision.c.material != NULL ? collision.c.material->GetSurfaceType() : SURFTYPE_METAL;
				if ( type == SURFTYPE_NONE ) {
					type = SURFTYPE_METAL;
				}

				const char *materialType = gameLocal.sufaceTypeNames[ type ];

				fxname = damageDef->dict.GetString( va( "smoke_wound_%s", materialType ) );
				if ( *fxname == '\0' ) {
					fxname = damageDef->dict.GetString( "smoke_wound" );
				}
			}
		}
	}
#ifdef _D3XP
	// If the explosion is in liquid, spawn a particle splash
	idVec3 testOrg = GetPhysics()->GetOrigin();
	int testC = gameLocal.clip.Contents( testOrg, NULL, mat3_identity, CONTENTS_WATER, this );
	if ( testC & CONTENTS_WATER ) {
		idFuncEmitter *splashEnt;
		idDict splashArgs;

		splashArgs.Set( "model", "sludgebulletimpact.prt" );
		splashArgs.Set( "start_off", "1" );
		splashEnt = static_cast<idFuncEmitter *>( gameLocal.SpawnEntityType( idFuncEmitter::Type, &splashArgs ) );

		splashEnt->GetPhysics()->SetOrigin( testOrg );
		splashEnt->PostEventMS( &EV_Activate, 0, this );
		splashEnt->PostEventMS( &EV_Remove, 1500 );

		//ff1.3 start
		/*
		was:
		// HACK - if this is a chaingun bullet, don't do the normal effect
		if ( !idStr::Cmp( spawnArgs.GetString( "def_damage" ), "damage_bullet_chaingun" ) ) {
			fxname = NULL;
		}
		*/
		//ff1.3 end
	}
#endif

	if ( fxname && *fxname ) {

#ifdef _DENTONMOD
		if( tracerEffect!= NULL && tracerEffect->IsType( dnBeamTracer::Type() ) ){ // check whether we used beam model as tracer
			memset( &renderEntity, 0, sizeof(renderEntity) );
		}
#endif
		SetModel( fxname );
		renderEntity.shaderParms[SHADERPARM_RED] =
		renderEntity.shaderParms[SHADERPARM_GREEN] =
		renderEntity.shaderParms[SHADERPARM_BLUE] =
		renderEntity.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat();
		Show();
		//ff1.3 start
		//was: removeTime = ( removeTime > 3000 ) ? removeTime : 3000;
		// default remove time with explode model
		removeTime = spawnArgs.GetInt( "remove_time", "3000" );
		//ff1.3 end
	}
	//ff1.3 start
	else {
		// default remove time without explode model
		removeTime = spawnArgs.GetInt( "remove_time", "1500" );
	}
	//gameLocal.Printf( "removeTime %d\n", removeTime );
	//ff1.3 end

	// explosion light
	light_shader = spawnArgs.GetString( "mtr_explode_light_shader" );

#ifdef CTF
	if ( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool("si_midnight") )
	{
		light_shader = "lights/midnight_grenade";
	}
#endif

	if ( *light_shader ) {
		renderLight.shader = declManager->FindMaterial( light_shader, false );
		renderLight.pointLight = true;
		renderLight.noShadows = spawnArgs.GetBool( "noLightShadows" ); //ff1.3
		renderLight.lightRadius[0] =
		renderLight.lightRadius[1] =
		renderLight.lightRadius[2] = spawnArgs.GetFloat( "explode_light_radius" );

#ifdef CTF
		// Midnight ctf
		if ( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool("si_midnight") )
		{
			renderLight.lightRadius[0] =
			renderLight.lightRadius[1] =
			renderLight.lightRadius[2] = spawnArgs.GetFloat( "explode_light_radius" ) * 2;
		}

#endif

		spawnArgs.GetVector( "explode_light_color", "1 1 1", lightColor );
		renderLight.shaderParms[SHADERPARM_RED] = lightColor.x;
		renderLight.shaderParms[SHADERPARM_GREEN] = lightColor.y;
		renderLight.shaderParms[SHADERPARM_BLUE] = lightColor.z;
		renderLight.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );

#ifdef CTF
		// Midnight ctf
		if ( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool("si_midnight") )
		{
			light_fadetime = 3.0f;
		}
		else
#endif
		light_fadetime = spawnArgs.GetFloat( "explode_light_fadetime", "0.5" );
		lightStartTime = gameLocal.time;
		lightEndTime = gameLocal.time + SEC2MS( light_fadetime );
		BecomeActive( TH_THINK );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

#ifdef _DENTONMOD
	if ( tracerEffect )
	{
		if ( tracerEffect->IsType( dnSpeedTracer::Type() ) && !static_cast<dnSpeedTracer *>(tracerEffect)->IsDead() ) {
			BecomeActive( TH_UPDATEPARTICLES );
		}
		else if( !tracerEffect->IsType( dnRailBeam::Type() ) ) {
			delete tracerEffect;
			tracerEffect = NULL;
		}
	}
#endif

	state = EXPLODED;

	if ( gameLocal.isClient ) {
		return;
	}

	// alert the ai
	gameLocal.AlertAI( owner.GetEntity(), physicsObj.GetOrigin() );

	// bind the projectile to the impact entity if necesary
	//if ( gameLocal.entities[collision.c.entityNum] && spawnArgs.GetBool( "bindOnImpact" ) ) {
	if ( collision.c.type != CONTACT_NONE && gameLocal.entities[collision.c.entityNum] && spawnArgs.GetBool( "bindOnImpact" ) ) { //ff1.3 - don't bind to player1 by mistake
		Bind( gameLocal.entities[collision.c.entityNum], true );
	}

	// splash damage
	if ( !projectileFlags.noSplashDamage ) {
		float delay = spawnArgs.GetFloat( "delay_splash" );
		if ( delay ) {
			if ( removeTime < delay * 1000 ) {
				removeTime = ( delay + 0.10 ) * 1000;
			}
			PostEventSec( &EV_RadiusDamage, delay, ignore );
		} else {
			Event_RadiusDamage( ignore );
		}
	}

	// spawn debris entities
	int fxdebris = spawnArgs.GetInt( "debris_count" );
	if ( fxdebris ) {
		int fxdebrisMin = spawnArgs.GetInt( "debris_count_min", "1" ); //ff1.3

//		const idDict *debris = gameLocal.FindEntityDefDict( "projectile_debris", false );
		const idDict *debris = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_debris"), false );
		if ( debris ) {
			int amount = fxdebrisMin + gameLocal.random.RandomInt( fxdebris - fxdebrisMin );
			for ( int i = 0; i < amount; i++ ) {
				idEntity *ent;
				idVec3 dir;
				dir.x = gameLocal.random.CRandomFloat() * 4.0f;
				dir.y = gameLocal.random.CRandomFloat() * 4.0f;
				dir.z = gameLocal.random.RandomFloat() * 8.0f;
				dir.Normalize();

				gameLocal.SpawnEntityDef( *debris, &ent, false );
				if ( !ent || !ent->IsType( idDebris::Type ) ) {
					gameLocal.Error( "'projectile_debris' is not an idDebris" );
				}

				idDebris *debris = static_cast<idDebris *>(ent);
				debris->Create( owner.GetEntity(), physicsObj.GetOrigin(), dir.ToMat3() );
				debris->Launch();
			}
		}
	}

	fxdebris = spawnArgs.GetInt( "shrapnel_count" );
	if ( fxdebris ) {
		int fxdebrisMin = spawnArgs.GetInt( "shrapnel_count_min", "1" ); //ff1.3
//		debris = gameLocal.FindEntityDefDict( "projectile_shrapnel", false );
		const idDict *debris = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_shrapnel"), false );
		if ( debris ) {
			int amount = fxdebrisMin + gameLocal.random.RandomInt( fxdebris-fxdebrisMin );
			for ( int i = 0; i < amount; i++ ) {
				idEntity *ent;
				idVec3 dir;
				dir.x = gameLocal.random.CRandomFloat() * 8.0f;
				dir.y = gameLocal.random.CRandomFloat() * 8.0f;
				dir.z = gameLocal.random.RandomFloat() * 8.0f + 8.0f;
				dir.Normalize();

				gameLocal.SpawnEntityDef( *debris, &ent, false );
				if ( !ent || !ent->IsType( idDebris::Type ) ) {
					gameLocal.Error( "'projectile_shrapnel' is not an idDebris" );
				}

				idDebris *debris = static_cast<idDebris *>(ent);
				debris->Create( owner.GetEntity(), physicsObj.GetOrigin(), dir.ToMat3() );
				debris->Launch();
			}
		}
	}

	//ff1.3 start - spawn drop entities
	DropEntities( collision );
	//ff1.3 end

	CancelEvents( &EV_Explode );
	PostEventMS( &EV_RemoveBinds, 0 ); //ff1.3
	PostEventMS( &EV_Remove, removeTime );
}

//ff1.3 start
/*
================
idProjectile::Damage
================
*/
void idProjectile::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
					const char *damageDefName, const float damageScale, const int location ) {
	if ( attacker == GetOwner() ) {
		return; //This is important e.g. for monsters firing multiple projectiles, so they don't kill each other.
	}

	idEntity::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

/*
================
idProjectile::DropEntities
================
*/
void idProjectile::DropEntities( const trace_t &collision ) {
	const char *dropType;
	if ( spawnArgs.GetString( "def_drop", "", &dropType ) ) {
		//gameLocal.Printf("def_drop collision.c.type: %d\n", collision.c.type);

		trace_t tr;
		idVec3 spawnPos;
		float offset, dropToGroundRadius, minDistanceFromGround, traceDistance;

		//move away from collision point
		//NOTE: if collision type is CONTACT_NONE then collision was probably created manually, therefore entityNum is 0 (player1) and must be ignored
		if ( ( collision.c.material != NULL ) && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) ){ //no drop on skies
			return;
		} else if ( collision.c.type != CONTACT_NONE && gameLocal.entities[collision.c.entityNum] && gameLocal.entities[collision.c.entityNum]->IsType( idActor::Type ) ) {
			if (!spawnArgs.GetBool( "dropOnActor" )) {
				return;
			}
			spawnPos = collision.c.point;
		} else {
			if ( spawnArgs.GetFloat( "dropOffsetNormal", "0", offset ) ) {
				spawnPos = collision.c.point + offset * collision.c.normal;
			} else if ( spawnArgs.GetFloat( "dropOffsetDir", "0", offset ) ) {
				spawnPos = collision.c.point - offset * collision.endAxis[2];
			}

			//check normal
			if ( spawnArgs.GetFloat( "dropMinNormalDistance", "0", offset ) ) {
				gameLocal.clip.TracePoint( tr, spawnPos, spawnPos - offset * collision.c.normal, MASK_SOLID, this );
				if ( tr.fraction < 1.0f ) { //near the wall
					//gameLocal.Printf("dropMinNormalDistance\n");
					spawnPos += offset * (1 - tr.fraction) * collision.c.normal;
				}
			}
		}

		//check ceiling
		if ( spawnArgs.GetFloat( "dropMinCeilingDistance", "0", offset ) ) {
			gameLocal.clip.TracePoint( tr, spawnPos, spawnPos + idVec3( 0, 0, offset ), MASK_SOLID, this );
			if ( tr.fraction < 1.0f ) { //near the ceiling
				spawnPos.z -= offset * (1 - tr.fraction); //move at "dropOffset" distance from ceiling
				//gameLocal.Printf("near the ceiling\n");
			}
		}

		//check ground
		dropToGroundRadius =  spawnArgs.GetFloat( "dropToGroundRadius" );
		minDistanceFromGround =  spawnArgs.GetFloat( "dropMinGroundDistance", "1" );
		traceDistance = ( dropToGroundRadius > minDistanceFromGround ) ? dropToGroundRadius : minDistanceFromGround;
		if ( traceDistance > 0.0f ) {
			gameLocal.clip.TracePoint( tr, spawnPos, spawnPos - idVec3( 0, 0, traceDistance ), MASK_SOLID, this );
			if ( tr.fraction < 1.0f ) { //near the ground
				//gameLocal.Printf("near the ground\n");
				spawnPos.z += minDistanceFromGround - traceDistance * tr.fraction; //move at "minDistanceFromGround" from ground
			} else { //ground not found
				//gameLocal.Printf("not near the ground\n");
				if ( spawnArgs.GetBool( "dropOnlyToGround" ) ) {
					return;
				}
			}
		}

		//spawn
		const idDict *dropDef = gameLocal.FindEntityDefDict( dropType, false );
		if ( dropDef ) {
			idEntity *dropEnt;
			gameLocal.SpawnEntityDef( *dropDef, &dropEnt, false );
			if ( dropEnt ) {
				if ( dropEnt->IsType( idProjectile::Type ) ) {
					idProjectile *dropProj = static_cast<idProjectile *>(dropEnt);
					dropProj->Create( GetOwner(), spawnPos, collision.c.normal );
					dropProj->Launch( spawnPos, collision.c.normal, vec3_zero );
				} else {
					dropEnt->SetOrigin( spawnPos );
					dropEnt->SetOwner( GetOwner() );
					dropEnt->SetShaderParm(SHADERPARM_TIMEOFFSET, -MS2SEC( gameLocal.time ));
					dropEnt->SetShaderParm(SHADERPARM_DIVERSITY, gameLocal.random.CRandomFloat());
				}
			}
		}
	}
}
//ff1.3 end

/*
================
idProjectile::GetVelocity
================
*/
idVec3 idProjectile::GetVelocity( const idDict *projectile ) {
	idVec3 velocity;

	projectile->GetVector( "velocity", "0 0 0", velocity );
	return velocity;
}

/*
================
idProjectile::GetGravity
================
*/
idVec3 idProjectile::GetGravity( const idDict *projectile ) {
	float gravity;

	gravity = projectile->GetFloat( "gravity" );
	return idVec3( 0, 0, -gravity );
}

/*
================
idProjectile::Event_Explode
================
*/
void idProjectile::Event_Explode( void ) {
	trace_t collision;

	memset( &collision, 0, sizeof( collision ) );
	collision.endAxis = GetPhysics()->GetAxis();
	collision.endpos = GetPhysics()->GetOrigin();
	collision.c.point = GetPhysics()->GetOrigin();
	collision.c.normal.Set( 0, 0, 1 );
	AddDefaultDamageEffect( collision, collision.c.normal );
	Explode( collision, NULL );
}

/*
================
idProjectile::Event_Fizzle
================
*/
void idProjectile::Event_Fizzle( void ) {
	Fizzle();
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

#ifdef CTF
	// Projectiles do not collide with flags
	if ( other->IsType( idItemTeam::Type ) )
		return;
#endif

	if ( other != owner.GetEntity() ) {
		trace_t collision;

		memset( &collision, 0, sizeof( collision ) );
		collision.endAxis = GetPhysics()->GetAxis();
		collision.endpos = GetPhysics()->GetOrigin();
		collision.c.point = GetPhysics()->GetOrigin();
		collision.c.normal.Set( 0, 0, 1 );
		AddDefaultDamageEffect( collision, collision.c.normal );
		Explode( collision, NULL );
	}
}

#ifdef _D3XP
/*
================
idProjectile::CatchProjectile
================
*/
void idProjectile::CatchProjectile( idEntity* o, const char* reflectName ) {
	idEntity *prevowner = owner.GetEntity();

	owner = o;
	physicsObj.GetClipModel()->SetOwner( o );

	if ( this->IsType( idGuidedProjectile::Type ) ) {
		idGuidedProjectile *proj = static_cast<idGuidedProjectile*>(this);

		proj->SetEnemy( prevowner );
	}

	idStr s = spawnArgs.GetString( "def_damage" );
	s += reflectName;

	//ff1.3 start
    /*
    was:
    const idDict *catchDamageDef = gameLocal.FindEntityDefDict( s, false );
    if ( catchDamageDef ) {
        spawnArgs.Set( "def_damage", s );
    }
    */
    const idDeclEntityDef *catchDamageDef = gameLocal.FindEntityDef( s, false );
    if ( catchDamageDef ) {
        spawnArgs.Set( "def_damage", s );
        damageDef = catchDamageDef; //update damageDef for denton's code
    } else {
        damagePower = g_grabberProjDamageScale.GetFloat();
	}

	spawnArgs.Set( "grabbed", "1" );
	//ff1.3 end
}

/*
================
idProjectile::GetProjectileState
================
*/
int idProjectile::GetProjectileState( void ) {

	return (int)state;
}

/*
================
idProjectile::Event_CreateProjectile
================
*/
void idProjectile::Event_CreateProjectile( idEntity *owner, const idVec3 &start, const idVec3 &dir ) {
	Create(owner, start, dir);
}

/*
================
idProjectile::Event_LaunchProjectile
================
*/
void idProjectile::Event_LaunchProjectile( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity ) {
	Launch(start, dir, pushVelocity);
}

/*
================
idProjectile::Event_SetGravity
================
*/
void idProjectile::Event_SetGravity( float gravity ) {
	idVec3 gravVec;

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();
	physicsObj.SetGravity(gravVec * gravity);
}
#endif

/*
=================
idProjectile::ClientPredictionCollide
=================
*/
bool idProjectile::ClientPredictionCollide( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity, bool addDamageEffect ) {
	idEntity *ent;

	// remove projectile when a 'noimpact' surface is hit
	if ( collision.c.material && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) ) {
		return false;
	}

	// get the entity the projectile collided with
	ent = gameLocal.entities[ collision.c.entityNum ];
	if ( ent == NULL ) {
		return false;
	}

	// don't do anything if hitting a noclip player
	if ( ent->IsType( idPlayer::Type ) && static_cast<idPlayer *>( ent )->noclip ) {
		return false;
	}

	if ( ent->IsType( idActor::Type ) || ( ent->IsType( idAFAttachment::Type ) && static_cast<const idAFAttachment*>(ent)->GetBody()->IsType( idActor::Type ) ) ) {
		if ( !projectileDef.GetBool( "detonate_on_actor" ) ) {
			return false;
		}
	} else {
		if ( !projectileDef.GetBool( "detonate_on_world" ) ) {
			return false;
		}
	}

	// if the projectile causes a damage effect
	if ( addDamageEffect && projectileDef.GetBool( "impact_damage_effect" ) ) {
		// if the hit entity does not have a special damage effect
		if ( !ent->spawnArgs.GetBool( "bleed" ) ) {
			// predict damage effect
			DefaultDamageEffect( soundEnt, projectileDef, collision, velocity );
		}
	}
	return true;
}

/*
================
idProjectile::ClientPredictionThink
================
*/
void idProjectile::ClientPredictionThink( void ) {
	if ( !renderEntity.hModel ) {
		return;
	}
	Think();
}

/*
================
idProjectile::WriteToSnapshot
================
*/
void idProjectile::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( owner.GetSpawnId(), 32 );
	msg.WriteBits( state, 3 );
	msg.WriteBits( fl.hidden, 1 );
	if ( netSyncPhysics ) {
		msg.WriteBits( 1, 1 );
		physicsObj.WriteToSnapshot( msg );
	} else {
		msg.WriteBits( 0, 1 );
		const idVec3 &origin	= physicsObj.GetOrigin();
		const idVec3 &velocity	= physicsObj.GetLinearVelocity();

		msg.WriteFloat( origin.x );
		msg.WriteFloat( origin.y );
		msg.WriteFloat( origin.z );

		msg.WriteDeltaFloat( 0.0f, velocity[0], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaFloat( 0.0f, velocity[1], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaFloat( 0.0f, velocity[2], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	}
}

/*
================
idProjectile::ReadFromSnapshot
================
*/
void idProjectile::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	projectileState_t newState;

	owner.SetSpawnId( msg.ReadBits( 32 ) );
	newState = (projectileState_t) msg.ReadBits( 3 );
	if ( msg.ReadBits( 1 ) ) {
		Hide();
	} else {
		Show();
	}

	while( state != newState ) {
		switch( state ) {
			case SPAWNED: {
				Create( owner.GetEntity(), vec3_origin, idVec3( 1, 0, 0 ) );
				break;
			}
			case CREATED: {
				// the right origin and direction are required if you want bullet traces
				Launch( vec3_origin, idVec3( 1, 0, 0 ), vec3_origin );
				break;
			}
			case LAUNCHED: {
				if ( newState == FIZZLED ) {
					Fizzle();
				} else {
					trace_t collision;
					memset( &collision, 0, sizeof( collision ) );
					collision.endAxis = GetPhysics()->GetAxis();
					collision.endpos = GetPhysics()->GetOrigin();
					collision.c.point = GetPhysics()->GetOrigin();
					collision.c.normal.Set( 0, 0, 1 );
					Explode( collision, NULL );
				}
				break;
			}
			case FIZZLED:
			case EXPLODED: {
				StopSound( SND_CHANNEL_BODY2, false );
				gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );
				state = SPAWNED;
				break;
			}
		}
	}

	if ( msg.ReadBits( 1 ) ) {
		physicsObj.ReadFromSnapshot( msg );
	} else {
		idVec3 origin;
		idVec3 velocity;
		idVec3 tmp;
		idMat3 axis;

		origin.x = msg.ReadFloat();
		origin.y = msg.ReadFloat();
		origin.z = msg.ReadFloat();

		velocity.x = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
		velocity.y = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
		velocity.z = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );

		physicsObj.SetOrigin( origin );
		physicsObj.SetLinearVelocity( velocity );

		// align z-axis of model with the direction
		velocity.NormalizeFast();
		axis = velocity.ToMat3();
		tmp = axis[2];
		axis[2] = axis[0];
		axis[0] = -tmp;
		physicsObj.SetAxis( axis );
	}

	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
================
idProjectile::ClientReceiveEvent
================
*/
bool idProjectile::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	trace_t collision;
	idVec3 velocity;

	switch( event ) {
		case EVENT_DAMAGE_EFFECT: {
			memset( &collision, 0, sizeof( collision ) );
			collision.c.point[0] = msg.ReadFloat();
			collision.c.point[1] = msg.ReadFloat();
			collision.c.point[2] = msg.ReadFloat();
			collision.c.normal = msg.ReadDir( 24 );
			int index = gameLocal.ClientRemapDecl( DECL_MATERIAL, msg.ReadInt() );
			collision.c.material = ( index != -1 ) ? static_cast<const idMaterial *>( declManager->DeclByIndex( DECL_MATERIAL, index ) ) : NULL;
			velocity[0] = msg.ReadFloat( 5, 10 );
			velocity[1] = msg.ReadFloat( 5, 10 );
			velocity[2] = msg.ReadFloat( 5, 10 );
//			DefaultDamageEffect( this, spawnArgs, collision, velocity );
			DefaultDamageEffect( this, damageDef!=NULL? damageDef->dict: spawnArgs, collision, velocity ); // new
			return true;
		}
		default:
			break;
	}

	return idEntity::ClientReceiveEvent( event, time, msg );
}

/*
===============================================================================

	idGuidedProjectile

===============================================================================
*/

#ifdef _D3XP
const idEventDef EV_SetEnemy( "setEnemy", "E" );
#endif

CLASS_DECLARATION( idProjectile, idGuidedProjectile )
#ifdef _D3XP
	EVENT( EV_SetEnemy,		idGuidedProjectile::Event_SetEnemy )
#endif
END_CLASS

/*
================
idGuidedProjectile::idGuidedProjectile
================
*/
idGuidedProjectile::idGuidedProjectile( void ) {
	enemy			= NULL;
	speed			= 0.0f;
	turn_max		= 0.0f;
	clamp_dist		= 0.0f;
	rndScale		= ang_zero;
	rndAng			= ang_zero;
	rndUpdateTime	= 0;
	angles			= ang_zero;
	burstMode		= false;
	burstDist		= 0;
	burstVelocity	= 0.0f;
	unGuided		= false;
	//ff1.3 start
	forceFieldVelocity		= vec3_zero;
	forceFieldVelocityTime	= 0;
	//ff1.3 end
}

/*
=================
idGuidedProjectile::~idGuidedProjectile
=================
*/
idGuidedProjectile::~idGuidedProjectile( void ) {
}

/*
================
idGuidedProjectile::Spawn
================
*/
void idGuidedProjectile::Spawn( void ) {
}

/*
================
idGuidedProjectile::Save
================
*/
void idGuidedProjectile::Save( idSaveGame *savefile ) const {
	enemy.Save( savefile );
	savefile->WriteFloat( speed );
	savefile->WriteAngles( rndScale );
	savefile->WriteAngles( rndAng );
	savefile->WriteInt( rndUpdateTime );
	savefile->WriteFloat( turn_max );
	savefile->WriteFloat( clamp_dist );
	savefile->WriteAngles( angles );
	savefile->WriteBool( burstMode );
	savefile->WriteBool( unGuided );
	savefile->WriteFloat( burstDist );
	savefile->WriteFloat( burstVelocity );
	//ff1.3 start
	savefile->WriteVec3( forceFieldVelocity );
	savefile->WriteInt( forceFieldVelocityTime );
	//ff1.3 end
}

/*
================
idGuidedProjectile::Restore
================
*/
void idGuidedProjectile::Restore( idRestoreGame *savefile ) {
	enemy.Restore( savefile );
	savefile->ReadFloat( speed );
	savefile->ReadAngles( rndScale );
	savefile->ReadAngles( rndAng );
	savefile->ReadInt( rndUpdateTime );
	savefile->ReadFloat( turn_max );
	savefile->ReadFloat( clamp_dist );
	savefile->ReadAngles( angles );
	savefile->ReadBool( burstMode );
	savefile->ReadBool( unGuided );
	savefile->ReadFloat( burstDist );
	savefile->ReadFloat( burstVelocity );
	//ff1.3 start
	savefile->ReadVec3( forceFieldVelocity );
	savefile->ReadInt( forceFieldVelocityTime );
	//ff1.3 end
}


/*
================
idGuidedProjectile::GetSeekPos
================
*/
void idGuidedProjectile::GetSeekPos( idVec3 &out ) {
	idEntity *enemyEnt = enemy.GetEntity();
	if ( enemyEnt ) {
		if ( enemyEnt->IsType( idActor::Type ) ) {
			out = static_cast<idActor *>(enemyEnt)->GetEyePosition();
			out.z -= 12.0f;
		} else {
			out = enemyEnt->GetPhysics()->GetOrigin();
		}
	} else {
		out = GetPhysics()->GetOrigin() + physicsObj.GetLinearVelocity() * 2.0f;
	}
}

/*
================
idGuidedProjectile::Think
================
*/
void idGuidedProjectile::Think( void ) {
	idVec3		dir;
	idVec3		seekPos;
	idVec3		velocity;
	idVec3		nose;
	idVec3		tmp;
	idMat3		axis;
	idAngles	dirAng;
	idAngles	diff;
	float		dist;
	float		frac;
	int			i;


	if ( state == LAUNCHED && !unGuided ) {

        //ff1.3 start - missles without target goes straight
        if ( forceFieldVelocityTime >= gameLocal.time ) {
            velocity = forceFieldVelocity;
            dir = forceFieldVelocity;
            dir.Normalize();
        } else if ( enemy.GetEntity() ) {
        //ff1.3 end
            GetSeekPos( seekPos );

			if ( rndUpdateTime < gameLocal.time ) {
				rndAng[ 0 ] = rndScale[ 0 ] * gameLocal.random.CRandomFloat();
				rndAng[ 1 ] = rndScale[ 1 ] * gameLocal.random.CRandomFloat();
				rndAng[ 2 ] = rndScale[ 2 ] * gameLocal.random.CRandomFloat();
				rndUpdateTime = gameLocal.time + 200;
			}

			nose = physicsObj.GetOrigin() + 10.0f * physicsObj.GetAxis()[0];

			dir = seekPos - nose;
			dist = dir.Normalize();
			dirAng = dir.ToAngles();

			// make it more accurate as it gets closer
			frac = dist / clamp_dist;
			if ( frac > 1.0f ) {
				frac = 1.0f;
			}

			diff = dirAng - angles + rndAng * frac;

			// clamp the to the max turn rate
			diff.Normalize180();
			for( i = 0; i < 3; i++ ) {
				if ( diff[ i ] > turn_max ) {
					diff[ i ] = turn_max;
				} else if ( diff[ i ] < -turn_max ) {
					diff[ i ] = -turn_max;
				}
			}
			angles += diff;

			// make the visual model always points the dir we're traveling
			dir = angles.ToForward();
			velocity = dir * speed;

			if ( burstMode && dist < burstDist ) {
				unGuided = true;
				velocity *= burstVelocity;
			}
		//ff1.3 start
		} else { //projectiles without target go straight, but calculate the velocity anyway in case speed changes
			dir = angles.ToForward();
			velocity = dir * speed;
		}
		//ff1.3 end

		physicsObj.SetLinearVelocity( velocity );

		// align z-axis of model with the direction
		axis = dir.ToMat3();
		tmp = axis[2];
		axis[2] = axis[0];
		axis[0] = -tmp;

		GetPhysics()->SetAxis( axis );
	}

	idProjectile::Think();
}

//ff1.3 start
void idGuidedProjectile::SetForceFieldVelocity( const idVec3 &velocity ){
	forceFieldVelocity = velocity;
	forceFieldVelocityTime = gameLocal.time + 100; //keep this speed for a while even after leaving the forcefield
}
//ff1.3 end

/*
=================
idGuidedProjectile::Launch
=================
*/
void idGuidedProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, float dmgPower ) {
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );
	if ( !enemy.GetEntity() ) { //ff1.3 - avoid calculating the enemy if it was explicitly set
        if ( owner.GetEntity() ) {
            if ( owner.GetEntity()->IsType( idAI::Type ) ) {
                enemy = static_cast<idAI *>( owner.GetEntity() )->GetEnemy();
            } else if ( owner.GetEntity()->IsType( idPlayer::Type ) ) {
                trace_t tr;
                idPlayer *player = static_cast<idPlayer*>( owner.GetEntity() );
                idVec3 start = player->GetEyePosition();
                idVec3 end = start + player->viewAxis[0] * 1000.0f;
                gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL | CONTENTS_BODY, owner.GetEntity() );
                if ( tr.fraction < 1.0f ) {
                    enemy = gameLocal.GetTraceEntity( tr );
                }
                // ignore actors on the player's team
                if ( enemy.GetEntity() == NULL || !enemy.GetEntity()->IsType( idActor::Type ) || ( static_cast<idActor *>( enemy.GetEntity() )->team == player->team ) ) {
                    enemy = player->EnemyWithMostHealth();
                }
			}
		}
	}
	const idVec3 &vel = physicsObj.GetLinearVelocity();
	angles = vel.ToAngles();
	speed = vel.Length();
	rndScale = spawnArgs.GetAngles( "random", "15 15 0" );
	turn_max = spawnArgs.GetFloat( "turn_max", "180" ) / ( float )USERCMD_HZ;
	clamp_dist = spawnArgs.GetFloat( "clamp_dist", "256" );
	burstMode = spawnArgs.GetBool( "burstMode" );
	unGuided = false;
	burstDist = spawnArgs.GetFloat( "burstDist", "64" );
	burstVelocity = spawnArgs.GetFloat( "burstVelocity", "1.25" );
	UpdateVisuals();
}

#ifdef _D3XP
void idGuidedProjectile::SetEnemy( idEntity *ent ) {
	enemy = ent;
}
void idGuidedProjectile::Event_SetEnemy(idEntity *ent) {
	SetEnemy(ent);
}
#endif

//ff1.3 start
/*
===============================================================================

	idPossessionProjectile

===============================================================================
*/

CLASS_DECLARATION( idGuidedProjectile, idPossessionProjectile )
	EVENT( EV_Remove,		idPossessionProjectile::Event_Remove )
END_CLASS

/*
================
idPossessionProjectile::idPossessionProjectile
================
*/
idPossessionProjectile::idPossessionProjectile( void ) {
	startingVelocity	= vec3_zero;
	endingVelocity		= vec3_zero;
	accelTime			= 0.0f;
	launchTime			= 0;

	//cameraActive	= false;
	cameraView		= NULL;
	cameraOffset	= vec3_zero;
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
}

/*
=================
idPossessionProjectile::~idPossessionProjectile
=================
*/
idPossessionProjectile::~idPossessionProjectile( void ) {
	if ( cameraView ) {
		delete cameraView;
	}
	if ( secondModelDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( secondModelDefHandle );
		secondModelDefHandle = -1;
	}
}

/*
================
idPossessionProjectile::Spawn
================
*/
void idPossessionProjectile::Spawn( void ) {
	spawnArgs.GetVector( "camera_offset", "0 0 0", cameraOffset );

	//precache second model ?
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	const char *temp = spawnArgs.GetString( "model_two" );
	if ( temp && *temp ) {
		secondModel.hModel = renderModelManager->FindModel( temp );
		secondModel.bounds = secondModel.hModel->Bounds( &secondModel );
		secondModel.shaderParms[ SHADERPARM_RED ] =
		secondModel.shaderParms[ SHADERPARM_GREEN ] =
		secondModel.shaderParms[ SHADERPARM_BLUE ] =
		secondModel.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		secondModel.noSelfShadow = true;
		secondModel.noShadow = true;
	}
}

/*
================
idPossessionProjectile::Save
================
*/
void idPossessionProjectile::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( startingVelocity );
	savefile->WriteVec3( endingVelocity );
	savefile->WriteFloat( accelTime );
	savefile->WriteInt( launchTime );

	//savefile->WriteBool( cameraActive );
	savefile->WriteObject( cameraView );
	savefile->WriteRenderEntity( secondModel );
	savefile->WriteInt( secondModelDefHandle );
	savefile->WriteVec3( cameraOffset );
}

/*
================
idPossessionProjectile::Restore
================
*/
void idPossessionProjectile::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( startingVelocity );
	savefile->ReadVec3( endingVelocity );
	savefile->ReadFloat( accelTime );
	savefile->ReadInt( launchTime );

	//savefile->ReadBool( cameraActive );
	savefile->ReadObject( reinterpret_cast<idClass *&>( cameraView ) );
	savefile->ReadRenderEntity( secondModel );
	savefile->ReadInt( secondModelDefHandle );
	savefile->ReadVec3( cameraOffset );
}

/*
================
idPossessionProjectile::Think
================
*/
void idPossessionProjectile::Think( void ) {
	float pct;

	if ( accelTime && gameLocal.time < launchTime + accelTime * 1000 ) {
		pct = ( gameLocal.time - launchTime ) / ( accelTime * 1000 );
		speed = ( startingVelocity + ( startingVelocity + endingVelocity ) * pct ).Length();
	}

	idGuidedProjectile::Think();

	if ( state == LAUNCHED ) {
		if ( enemy.GetEntity() ) {
			const idBounds &myBounds = GetPhysics()->GetAbsBounds();
			const idBounds &entBounds =  enemy.GetEntity()->GetPhysics()->GetAbsBounds();

			if ( myBounds.IntersectsBounds( entBounds ) ) {
				//gameLocal.Printf("myBounds.IntersectsBounds( entBounds )\n");
				PostEventMS( &EV_Explode, 0 );
				StartRide( enemy.GetEntity() );
			}
		}

		if ( secondModelDefHandle >= 0 ) {
			//align second model to direction
			//NOTE: rotate the model +90 on Z axis so the soulcube faces the camera
			idMat3 secAxis = angles.ToMat3();
			idVec3 tmp = secAxis[1];
			secAxis[1] = secAxis[0];
			secAxis[0] = -tmp;
			secondModel.axis = secAxis;
			secondModel.origin = physicsObj.GetOrigin();
			gameRenderWorld->UpdateEntityDef( secondModelDefHandle, &secondModel );
		}
		if ( cameraView ) {
			UpdateCamera();
		}
	}
}

/*
================
idPossessionProjectile::GetSeekPos
================
*/
void idPossessionProjectile::GetSeekPos( idVec3 &out ) {
	idEntity *enemyEnt = enemy.GetEntity();
	if ( enemyEnt ) {
		out = enemyEnt->GetPhysics()->GetAbsBounds().GetCenter();
	} else {
		out = GetPhysics()->GetOrigin() + physicsObj.GetLinearVelocity() * 2.0f;
	}
}

/*
=================
idPossessionProjectile::Launch
=================
*/
void idPossessionProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, float dmgPower ) {
	idMat3	cameraAxis;
	idVec3	cameraPos;

	idGuidedProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );

	//accel
	startingVelocity = spawnArgs.GetVector( "startingVelocity", "15 0 0" );
	endingVelocity = spawnArgs.GetVector( "endingVelocity", "1500 0 0" );
	accelTime = spawnArgs.GetFloat( "accelTime", "5" );
	physicsObj.SetLinearVelocity( startingVelocity.Length() * physicsObj.GetAxis()[2] );
	launchTime = gameLocal.time;

	//setup camera
	EnableCamera(true);

	//create second model
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	const char *temp = spawnArgs.GetString( "model_two" );
	if ( temp && *temp ) {
		secondModel.hModel = renderModelManager->FindModel( temp );
		secondModel.bounds = secondModel.hModel->Bounds( &secondModel );
		secondModel.shaderParms[ SHADERPARM_RED ] =
		secondModel.shaderParms[ SHADERPARM_GREEN ] =
		secondModel.shaderParms[ SHADERPARM_BLUE ] =
		secondModel.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		secondModel.noSelfShadow = true;
		secondModel.noShadow = true;
		secondModel.origin = GetPhysics()->GetOrigin();
		secondModel.axis = GetPhysics()->GetAxis();
		secondModelDefHandle = gameRenderWorld->AddEntityDef( &secondModel );
	}
}

/*
================
idPossessionProjectile::EnableCamera
================
*/
void idPossessionProjectile::EnableCamera( bool on ) {
	if ( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) ) {
		idPlayer *player = static_cast<idPlayer*>( owner.GetEntity() );
		if ( on ) {
			if ( !cameraView ) {
				cameraView = static_cast<idCameraView *>( gameLocal.SpawnEntityType( idCameraView::Type, NULL ) );
			}
			UpdateCamera();
			player->SetPrivateCameraView( cameraView );
			player->EnableBloom(true);
		} else {
			if ( cameraView && player->GetPrivateCameraView() == cameraView ) {
				player->SetPrivateCameraView( NULL );
				player->EnableBloom(false);
			}
		}
	}
}

/*
=================
idPossessionProjectile::UpdateCamera
=================
*/
void idPossessionProjectile::UpdateCamera( void ){
	idMat3	cameraAxis;
	idVec3	cameraPos;

	cameraAxis = angles.ToMat3();
	cameraPos = physicsObj.GetOrigin() + cameraOffset.x * cameraAxis[0] + cameraOffset.y * cameraAxis[1] + cameraOffset.z * cameraAxis[2];
	cameraView->GetPhysics()->SetOrigin( cameraPos );
	cameraView->GetPhysics()->SetAxis( cameraAxis );
}

/*
================
idPossessionProjectile::StartRide
================
*/
void idPossessionProjectile::StartRide( idEntity* targetEnt ){
	if ( !targetEnt || !targetEnt->IsType( idAI::Type ) || targetEnt->IsType( idAI_Rideable::Type ) ){
		return;
	}

	if ( !static_cast<idAI*>( targetEnt )->CanBeRidden() ) {
		return;
	}

	if ( !owner.GetEntity() || !owner.GetEntity()->IsType( idPlayer::Type ) ){
		return;
	}

	idPlayer *player = static_cast<idPlayer*>( owner.GetEntity() );
	if ( !player->CanEnterAI() ) {
		return;
	}

	player->PossessionProjectileHitCallback( this );
	targetEnt->PostEventMS( &AI_StartRiding, 0, owner.GetEntity(), 1 );
}

/*
================
idPossessionProjectile::Explode
================
*/
void idPossessionProjectile::Explode( const trace_t &collision, idEntity *ignore ) {
	if ( secondModelDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( secondModelDefHandle );
		secondModelDefHandle = -1;
	}

	EnableCamera(false);

	idGuidedProjectile::Explode( collision, ignore );

	if ( collision.c.type != CONTACT_NONE ){
		StartRide( gameLocal.entities[collision.c.entityNum] );
	}
}

/*
================
idPossessionProjectile::Event_Remove
================
*/
void idPossessionProjectile::Event_Remove( void ) {
	EnableCamera(false);
	idGuidedProjectile::Event_Remove();
}

//ff1.3 end

/*
===============================================================================

idSoulCubeMissile

===============================================================================
*/

CLASS_DECLARATION( idGuidedProjectile, idSoulCubeMissile )
END_CLASS

/*
================
idSoulCubeMissile::Spawn( void )
================
*/
void idSoulCubeMissile::Spawn( void ) {
	startingVelocity	= vec3_zero;
	endingVelocity		= vec3_zero;
	accelTime			= 0.0f;
	launchTime			= 0;
	killPhase			= false;
	returnPhase			= false;
	smokeKillTime		= 0;
	smokeKill			= NULL;
}

/*
=================
idSoulCubeMissile::~idSoulCubeMissile
=================
*/
idSoulCubeMissile::~idSoulCubeMissile() {
}

/*
================
idSoulCubeMissile::Save
================
*/
void idSoulCubeMissile::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( startingVelocity );
	savefile->WriteVec3( endingVelocity );
	savefile->WriteFloat( accelTime );
	savefile->WriteInt( launchTime );
	savefile->WriteBool( killPhase );
	savefile->WriteBool( returnPhase );
	savefile->WriteVec3( destOrg);
	savefile->WriteInt( orbitTime );
	savefile->WriteVec3( orbitOrg );
	savefile->WriteInt( smokeKillTime );
	savefile->WriteParticle( smokeKill );
}

/*
================
idSoulCubeMissile::Restore
================
*/
void idSoulCubeMissile::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( startingVelocity );
	savefile->ReadVec3( endingVelocity );
	savefile->ReadFloat( accelTime );
	savefile->ReadInt( launchTime );
	savefile->ReadBool( killPhase );
	savefile->ReadBool( returnPhase );
	savefile->ReadVec3( destOrg);
	savefile->ReadInt( orbitTime );
	savefile->ReadVec3( orbitOrg );
	savefile->ReadInt( smokeKillTime );
	savefile->ReadParticle( smokeKill );
}

/*
================
idSoulCubeMissile::KillTarget
================
*/
void idSoulCubeMissile::KillTarget( const idVec3 &dir ) {
	idEntity	*ownerEnt;
	const char	*smokeName;
	idActor		*act;

	ReturnToOwner();
	if ( enemy.GetEntity() && enemy.GetEntity()->IsType( idActor::Type ) ) {
		act = static_cast<idActor*>( enemy.GetEntity() );
		killPhase = true;
		orbitOrg = act->GetPhysics()->GetAbsBounds().GetCenter();
		orbitTime = gameLocal.time;
		smokeKillTime = 0;
		smokeName = spawnArgs.GetString( "smoke_kill" );
		if ( *smokeName != '\0' ) {
			smokeKill = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
			smokeKillTime = gameLocal.time;
		}
		ownerEnt = owner.GetEntity();
		if ( ( act->health > 0 ) && ownerEnt && ownerEnt->IsType( idPlayer::Type ) && ( ownerEnt->health > 0 ) && !act->spawnArgs.GetBool( "boss" ) ) {
			static_cast<idPlayer *>( ownerEnt )->GiveHealthPool( act->health );
		}
		act->Damage( this, owner.GetEntity(), dir, spawnArgs.GetString( "def_damage" ), 1.0f, INVALID_JOINT );
		act->GetAFPhysics()->SetTimeScale( 0.25 );
		StartSound( "snd_explode", SND_CHANNEL_BODY, 0, false, NULL );
	}
}

/*
================
idSoulCubeMissile::Think
================
*/
void idSoulCubeMissile::Think( void ) {
	float		pct;
	idVec3		seekPos;
	//idEntity	*ownerEnt;

	if ( state == LAUNCHED ) {
		if ( killPhase ) {
			// orbit the mob, cascading down
			if ( gameLocal.time < orbitTime + 1500 ) {
				if ( !gameLocal.smokeParticles->EmitSmoke( smokeKill, smokeKillTime, gameLocal.random.CRandomFloat(), orbitOrg, mat3_identity, timeGroup /*_D3XP*/ ) ) {
					smokeKillTime = gameLocal.time;
				}
			}
		} else {
			if ( accelTime && gameLocal.time < launchTime + accelTime * 1000 ) {
				pct = ( gameLocal.time - launchTime ) / ( accelTime * 1000 );
				speed = ( startingVelocity + ( startingVelocity + endingVelocity ) * pct ).Length();
			}
		}
		idGuidedProjectile::Think();
		GetSeekPos( seekPos );
		if ( ( seekPos - physicsObj.GetOrigin() ).Length() < 32.0f ) {
			if ( returnPhase ) {
				StopSound( SND_CHANNEL_ANY, false );
				StartSound( "snd_return", SND_CHANNEL_BODY2, 0, false, NULL );
				Hide();
				PostEventSec( &EV_Remove, 2.0f );
				/*
				//ff1.3 - removed
				ownerEnt = owner.GetEntity();
				if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
					static_cast<idPlayer *>( ownerEnt )->SetSoulCubeProjectile( NULL );
				}
				*/

				state = FIZZLED;
			} else if ( !killPhase ){
				KillTarget( physicsObj.GetAxis()[0] );
			}
		}
	}
}

/*
================
idSoulCubeMissile::GetSeekPos
================
*/
void idSoulCubeMissile::GetSeekPos( idVec3 &out ) {
	if ( returnPhase && owner.GetEntity() && owner.GetEntity()->IsType( idActor::Type ) ) {
		idActor *act = static_cast<idActor*>( owner.GetEntity() );
		out = act->GetEyePosition();
		return;
	}
	if ( destOrg != vec3_zero ) {
		out = destOrg;
		return;
	}
	idGuidedProjectile::GetSeekPos( out );
}


/*
================
idSoulCubeMissile::Event_ReturnToOwner
================
*/
void idSoulCubeMissile::ReturnToOwner() {
	speed *= 0.65f;
	killPhase = false;
	returnPhase = true;
	smokeFlyTime = 0;
}


/*
=================
idSoulCubeMissile::Launch
=================
*/
void idSoulCubeMissile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, float dmgPower ) {
	idVec3		newStart;
	idVec3		offs;
	//idEntity	*ownerEnt;

	// push it out a little
	newStart = start + dir * spawnArgs.GetFloat( "launchDist" );
	offs = spawnArgs.GetVector( "launchOffset", "0 0 -4" );
	newStart += offs;
	idGuidedProjectile::Launch( newStart, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );
	if ( enemy.GetEntity() == NULL || !enemy.GetEntity()->IsType( idActor::Type ) ) {
		destOrg = start + dir * 256.0f;
	} else {
		destOrg.Zero();
	}
	physicsObj.SetClipMask( 0 ); // never collide.. think routine will decide when to detonate
	startingVelocity = spawnArgs.GetVector( "startingVelocity", "15 0 0" );
	endingVelocity = spawnArgs.GetVector( "endingVelocity", "1500 0 0" );
	accelTime = spawnArgs.GetFloat( "accelTime", "5" );
	physicsObj.SetLinearVelocity( startingVelocity.Length() * physicsObj.GetAxis()[2] );
	launchTime = gameLocal.time;
	killPhase = false;
	UpdateVisuals();

    /*
	//ff1.3 - removed
	ownerEnt = owner.GetEntity();
	if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
		static_cast<idPlayer *>( ownerEnt )->SetSoulCubeProjectile( this );
	}
	*/

}


/*
===============================================================================

idBFGProjectile

===============================================================================
*/
const idEventDef EV_RemoveBeams( "<removeBeams>", NULL );

CLASS_DECLARATION( idProjectile, idBFGProjectile )
	EVENT( EV_RemoveBeams,		idBFGProjectile::Event_RemoveBeams )
END_CLASS


/*
=================
idBFGProjectile::idBFGProjectile
=================
*/
idBFGProjectile::idBFGProjectile() {
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	nextDamageTime = 0;
	//ff1.3 start
	damageRadius = 0.0f;
	actorDamaged = false;
	nextBeamTargetsSearchTime = 0;
	ownerTeam = 0;
	//ff1.3 end
}

/*
=================
idBFGProjectile::~idBFGProjectile
=================
*/
idBFGProjectile::~idBFGProjectile() {
	FreeBeams();

	if ( secondModelDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( secondModelDefHandle );
		secondModelDefHandle = -1;
	}
}

/*
================
idBFGProjectile::Spawn
================
*/
void idBFGProjectile::Spawn( void ) {
	beamTargets.Clear();
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	const char *temp = spawnArgs.GetString( "model_two" );
	if ( temp && *temp ) {
		secondModel.hModel = renderModelManager->FindModel( temp );
		secondModel.bounds = secondModel.hModel->Bounds( &secondModel );
		secondModel.shaderParms[ SHADERPARM_RED ] =
		secondModel.shaderParms[ SHADERPARM_GREEN ] =
		secondModel.shaderParms[ SHADERPARM_BLUE ] =
		secondModel.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		secondModel.noSelfShadow = true;
		secondModel.noShadow = true;
	}
	nextDamageTime = 0;
	damageFreq = NULL;

	//ff1.3 start
	nextBeamTargetsSearchTime = 0;
	spawnArgs.GetFloat( "damageRadius", "512", damageRadius );
	spawnArgs.GetBool( "ignoreDeadTargets", "0", ignoreDeadTargets );
	spawnArgs.GetBool( "mainDamageFirst", "0", mainDamageFirst );
	//ff1.3 end
}

/*
================
idBFGProjectile::Save
================
*/
void idBFGProjectile::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteInt( beamTargets.Num() );
	for ( i = 0; i < beamTargets.Num(); i++ ) {
		beamTargets[i].target.Save( savefile );
		savefile->WriteRenderEntity( beamTargets[i].renderEntity );
		savefile->WriteInt( beamTargets[i].modelDefHandle );
		savefile->WriteBool( beamTargets[i].alreadyDamaged ); //ff1.3
	}

	savefile->WriteRenderEntity( secondModel );
	savefile->WriteInt( secondModelDefHandle );
	savefile->WriteInt( nextDamageTime );
	savefile->WriteString( damageFreq );

	//ff1.3 start
	//savefile->WriteInt( damageFreqMs );
	savefile->WriteFloat( damageRadius );
	savefile->WriteBool( actorDamaged );
	savefile->WriteBool( mainDamageFirst );
	savefile->WriteBool( ignoreDeadTargets );
	savefile->WriteInt( nextBeamTargetsSearchTime );
	savefile->WriteInt( ownerTeam );
	//ff1.3 end
}

/*
================
idBFGProjectile::Restore
================
*/
void idBFGProjectile::Restore( idRestoreGame *savefile ) {
	int i, num;

	savefile->ReadInt( num );
	beamTargets.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		beamTargets[i].target.Restore( savefile );
		savefile->ReadRenderEntity( beamTargets[i].renderEntity );
		savefile->ReadInt( beamTargets[i].modelDefHandle );
		if ( beamTargets[i].modelDefHandle >= 0 ) {
			beamTargets[i].modelDefHandle = gameRenderWorld->AddEntityDef( &beamTargets[i].renderEntity );
		}
		savefile->ReadBool( beamTargets[i].alreadyDamaged ); //ff1.3
	}

	savefile->ReadRenderEntity( secondModel );
	savefile->ReadInt( secondModelDefHandle );
	savefile->ReadInt( nextDamageTime );
	savefile->ReadString( damageFreq );

	if ( secondModelDefHandle >= 0 ) {
		secondModelDefHandle = gameRenderWorld->AddEntityDef( &secondModel );
	}

	//ff1.3 start
	//savefile->ReadInt( damageFreqMs );
	savefile->ReadFloat( damageRadius );
	savefile->ReadBool( actorDamaged );
	savefile->ReadBool( mainDamageFirst );
	savefile->ReadBool( ignoreDeadTargets );
	savefile->ReadInt( nextBeamTargetsSearchTime );
	savefile->ReadInt( ownerTeam );
	//ff1.3 end
}

/*
=================
idBFGProjectile::FreeBeams
=================
*/
void idBFGProjectile::FreeBeams() {
	for ( int i = 0; i < beamTargets.Num(); i++ ) {
		if ( beamTargets[i].modelDefHandle >= 0 ) {
			gameRenderWorld->FreeEntityDef( beamTargets[i].modelDefHandle );
			beamTargets[i].modelDefHandle = -1;
		}
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		player->playerView.EnableBFGVision( false );
	}
}

/*
================
idBFGProjectile::UpdateBeamTarget
================
*/
bool idBFGProjectile::UpdateBeamTarget( beamTarget_t &beamTarget, bool doDamage ) {
	idEntity *targetEnt = beamTarget.target.GetEntity();

	if ( targetEnt == NULL || targetEnt->IsHidden() ) {
		//gameLocal.Printf("targetEnt == NULL || targetEnt->IsHidden()\n");
		return false;
	}

	if ( ignoreDeadTargets && targetEnt->health <= 0 ) {
		return false;
	}

	idVec3 targetCenter = targetEnt->GetPhysics()->GetAbsBounds().GetCenter();
	idVec3 distanceVec = targetCenter - GetPhysics()->GetOrigin();

	//increase radius so the beam doesn't disappear too soon after being shown at the beginning
	//also consider FindBeamTargets() uses a box, so we need to increase the sphere radius to compensate
	if( distanceVec.Normalize() > damageRadius*1.5f ){
		return false;
	}

	if ( !targetEnt->CanDamage( GetPhysics()->GetOrigin(), targetCenter ) ) {
		//gameLocal.Printf("!canDamage\n");
		return false;
	}

	beamTarget.renderEntity.origin = GetPhysics()->GetOrigin();
	beamTarget.renderEntity.shaderParms[ SHADERPARM_BEAM_END_X ] = targetCenter.x;
	beamTarget.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ] = targetCenter.y;
	beamTarget.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ] = targetCenter.z;
	beamTarget.renderEntity.shaderParms[ SHADERPARM_RED ] =
	beamTarget.renderEntity.shaderParms[ SHADERPARM_GREEN ] =
	beamTarget.renderEntity.shaderParms[ SHADERPARM_BLUE ] =
	beamTarget.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;

	//damage
	if ( doDamage ) {
		//we may need to apply the main damage instead of damageFreq, but never on player/vehicle targets
		if( mainDamageFirst && !beamTarget.alreadyDamaged
				&& !targetEnt->IsType( idPlayer::Type )
				&& !targetEnt->IsType( idAI_Rideable::Type )
				&& !targetEnt->IsType( idAFEntity_Vehicle::Type ) ){

			if( damageDef != NULL ){
				targetEnt->Damage( this, owner.GetEntity(), distanceVec, damageDef->GetName(), ( damagePower ) ? damagePower : 1.0f, INVALID_JOINT );
			}
		}else{
			if( damageFreq && *(const char *)damageFreq ){
				targetEnt->Damage( this, owner.GetEntity(), distanceVec, damageFreq, ( damagePower ) ? damagePower : 1.0f, INVALID_JOINT );
			}
		}

		if( !beamTarget.alreadyDamaged ){
			beamTarget.alreadyDamaged = true; //always let the damageFreq to start
		}

		// if player hit the first actor
		if ( !actorDamaged && targetEnt->IsType( idActor::Type ) ) {
			actorDamaged = true;  //upd hit counter only once
			idEntity* ownerEnt = owner.GetEntity();

			if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
				static_cast<idPlayer*>( ownerEnt )->AddProjectileHits( 1, hitCountGroupId );
			}
		}

			/*
		bool canDamage = true;

		if ( damageFreq && *(const char *)damageFreq && targetEnt->CanDamage( GetPhysics()->GetOrigin(), targetCenter ) ) {
			idVec3 distanceVec = targetCenter - GetPhysics()->GetOrigin();
			targetEnt->Damage( this, owner.GetEntity(), distanceVec, damageFreq, ( damagePower ) ? damagePower : 1.0f, INVALID_JOINT );
		} else {
			beamTarget.renderEntity.shaderParms[ SHADERPARM_RED ] =
			beamTarget.renderEntity.shaderParms[ SHADERPARM_GREEN ] =
			beamTarget.renderEntity.shaderParms[ SHADERPARM_BLUE ] =
			beamTarget.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 0.0f;
			canDamage = false;
		}

		//TODO add this in the new logic
		if ( targetEnt->IsType( idPlayer::Type ) ) {
			static_cast<idPlayer*>( targetEnt )->playerView.EnableBFGVision( canDamage );
		}
		*/

		/*
		if( !canDamage ){
			gameLocal.Printf("!canDamage\n");
			return false; //remove targets that cannot be hit
		}
		*/
	}

	return true;
}

/*
================
idBFGProjectile::Think
================
*/
void idBFGProjectile::Think( void ) {
	if ( state == LAUNCHED ) {

		//ff1.3 start
		//freq damage fix
		bool doDamage = false;
		if ( gameLocal.time > nextDamageTime ) {
			nextDamageTime = gameLocal.time + BFG_DAMAGE_FREQUENCY;
			doDamage = true;
		}
		//acquire new targets dynamically
		if ( gameLocal.time > nextBeamTargetsSearchTime ) {
			nextBeamTargetsSearchTime = gameLocal.time + BFG_SEARCH_FREQUENCY;
			FindBeamTargets();
		}
		//ff1.3 end

		// update beam targets
		for ( int i = 0; i < beamTargets.Num(); i++ ) {
			beamTarget_t &beamTarget = beamTargets[i];
            //idEntity *targetEnt = beamTarget.target.GetEntity();

            if( UpdateBeamTarget(beamTarget, doDamage) ){ //update beam if it's a valid target
                if ( beamTarget.modelDefHandle == -1 ) { //handle un-hidden targets too
                    beamTarget.modelDefHandle = gameRenderWorld->AddEntityDef( &beamTarget.renderEntity );
                } else {
                    gameRenderWorld->UpdateEntityDef( beamTarget.modelDefHandle, &beamTarget.renderEntity );
				}
			}else{ //hide beam
				if ( beamTarget.modelDefHandle >= 0 ) {
					gameRenderWorld->FreeEntityDef( beamTarget.modelDefHandle );
					beamTarget.modelDefHandle = -1;
				}
			}
		}

		if ( secondModelDefHandle >= 0 ) {
			secondModel.origin = GetPhysics()->GetOrigin();
			gameRenderWorld->UpdateEntityDef( secondModelDefHandle, &secondModel );
		}

		/*
		//ff1.3 - don't rotate models anymore
		idAngles ang;

		ang.pitch = ( gameLocal.time & 4095 ) * 360.0f / -4096.0f;
		ang.yaw = ang.pitch;
		ang.roll = 0.0f;
		SetAngles( ang );

		ang.pitch = ( gameLocal.time & 2047 ) * 360.0f / -2048.0f;
		ang.yaw = ang.pitch;
		ang.roll = 0.0f;
		secondModel.axis = ang.ToMat3();
		*/
		UpdateVisuals();
	}

	idProjectile::Think();
}

/*
=================
idBFGProjectile::IsBeamTarget
=================
*/
bool idBFGProjectile::IsBeamTarget(idEntity *ent){
	for ( int i = 0; i < beamTargets.Num(); i++ ) {
		if( beamTargets[i].target.GetEntity() == ent ){
			return true;
		}
	}
	return false;
}


/*
=================
idBFGProjectile::AcceptBeamTarget
=================
*/
bool idBFGProjectile::AcceptBeamTarget(idEntity *ent, idVec3& damagePoint){
	if( ent->IsType( idActor::Type) ) {
		if( ownerTeam == static_cast<idActor*>( ent )->team ){
			return false;
		}
	} else if( ent->IsType( idAFEntity_Vehicle::Type) ) {
		if( ownerTeam == 0 || !static_cast<idAFEntity_Vehicle*>( ent )->GetDriver() ){
			return false;
		}
	} else if( ent->IsType( idExplodingBarrel::Type) ) {
		//damage bomb zombies
	} else {
		return false;
	}

	if( !ent->fl.takedamage || ent->health <= 0 ){
		return false;
	}

	if ( !ent->CanDamage( GetPhysics()->GetOrigin(), damagePoint ) ) {
		return false;
	}

	return true;
}


/*
=================
idBFGProjectile::FindBeamTargets
=================
*/
void idBFGProjectile::FindBeamTargets(){

	// dmgPower * radius is the target acquisition area //ivan: it seems just 'damageRadius' to me...
	// acquisition should make sure that monsters are not dormant
	// which will cut down on hitting monsters not actively fighting
	// but saves on the traces making sure they are visible
	// damage is not applied until the projectile explodes //ivan: freq. damage now works

	idEntity *	ent;
	idEntity *	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	idBounds	bounds;
	idVec3		damagePoint;

	bounds = idBounds( GetPhysics()->GetOrigin() ).Expand( damageRadius );

	float beamWidth = spawnArgs.GetFloat( "beam_WidthFly" );
	const char *skin = spawnArgs.GetString( "skin_beam" );
	bool hadTargetsBefore = (beamTargets.Num() > 0); //note: beamtargets are never removed from this list. Related beams are hidden in case of missing target.

	//idVec3 delta( 15.0f, 15.0f, 15.0f );
	//physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, physicsObj.GetAxis().ToAngles(), delta, ang_zero );

	// get all entities touching the bounds
	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, CONTENTS_BODY|CONTENTS_PROJECTILE|CONTENTS_RENDERMODEL, entityList, MAX_GENTITIES ); //ff1.3 - CONTENTS_PROJECTILE added
	for ( int e = 0; e < numListedEntities; e++ ) {
		ent = entityList[ e ];
		assert( ent );

		if ( ent == this || ent == owner.GetEntity() || ent->IsHidden() || !ent->IsActive() ) { //ff1.3 - removed actor check
			continue;
		}

		//ff1.3 start
		//don't add the same entity twice in case of incremental updates
		if ( hadTargetsBefore && IsBeamTarget( ent ) ) {
			//gameLocal.Printf("already target %s\n", ent->GetName());
			continue;
		}
        //ff1.3 end

		if( AcceptBeamTarget(ent, damagePoint) ){
			if ( ent->IsType( idPlayer::Type ) ) {
				idPlayer *player = static_cast<idPlayer*>( ent );
				player->playerView.EnableBFGVision( true );
			}

			beamTarget_t bt;
			memset( &bt.renderEntity, 0, sizeof( renderEntity_t ) );
			bt.renderEntity.origin = GetPhysics()->GetOrigin();
			bt.renderEntity.axis = GetPhysics()->GetAxis();
			bt.renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = beamWidth;
			bt.renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat() * 0.75;
			bt.renderEntity.hModel = renderModelManager->FindModel( "_beam" );
			bt.renderEntity.callback = NULL;
			bt.renderEntity.numJoints = 0;
			bt.renderEntity.joints = NULL;
			bt.renderEntity.bounds.Clear();
			bt.renderEntity.customSkin = declManager->FindSkin( skin );
			bt.target = ent;
			bt.modelDefHandle = gameRenderWorld->AddEntityDef( &bt.renderEntity );
			bt.alreadyDamaged = false; //ff1.3
			beamTargets.Append( bt );

			//gameLocal.Printf("beamTargets.Append( %s )\n", ent->GetName());
		}
	}
/*
#ifdef _D3XP
	// Major hack for end boss.  :(
	idAnimatedEntity *maledict = static_cast<idAnimatedEntity*>(gameLocal.FindEntity( "monster_boss_d3xp_maledict_1" ));

	if ( maledict ) {
		SetTimeState	ts( maledict->timeGroup );

		idVec3			realPoint;
		idMat3			temp;
		float			dist;
		jointHandle_t	bodyJoint;

		bodyJoint = maledict->GetAnimator()->GetJointHandle( "Chest1" );
		maledict->GetJointWorldTransform( bodyJoint, gameLocal.time, realPoint, temp );

		dist = idVec3( realPoint - GetPhysics()->GetOrigin() ).Length();

		if ( dist < radius ) {
			beamTarget_t bt;
			memset( &bt.renderEntity, 0, sizeof( renderEntity_t ) );
			bt.renderEntity.origin = GetPhysics()->GetOrigin();
			bt.renderEntity.axis = GetPhysics()->GetAxis();
			bt.renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = beamWidth;
			bt.renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
			bt.renderEntity.shaderParms[ SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat() * 0.75;
			bt.renderEntity.hModel = renderModelManager->FindModel( "_beam" );
			bt.renderEntity.callback = NULL;
			bt.renderEntity.numJoints = 0;
			bt.renderEntity.joints = NULL;
			bt.renderEntity.bounds.Clear();
			bt.renderEntity.customSkin = declManager->FindSkin( skin );
			bt.target = maledict;
			bt.modelDefHandle = gameRenderWorld->AddEntityDef( &bt.renderEntity );
			beamTargets.Append( bt );

			numListedEntities++;
		}
	}
#endif
*/
	//start the sound as soon as we find targets for the first time
	if( !hadTargetsBefore && beamTargets.Size() > 0 ){
		StartSound( "snd_beam", SND_CHANNEL_BODY2, 0, false, NULL );
	}
}


/*
=================
idBFGProjectile::Launch
=================
*/
void idBFGProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float power, const float dmgPower ) {
	idProjectile::Launch( start, dir, pushVelocity, 0.0f, power, dmgPower );

	//second model
	memset( &secondModel, 0, sizeof( secondModel ) );
	secondModelDefHandle = -1;
	const char *temp = spawnArgs.GetString( "model_two" );
	if ( temp && *temp ) {
		secondModel.hModel = renderModelManager->FindModel( temp );
		secondModel.bounds = secondModel.hModel->Bounds( &secondModel );
		secondModel.shaderParms[ SHADERPARM_RED ] =
		secondModel.shaderParms[ SHADERPARM_GREEN ] =
		secondModel.shaderParms[ SHADERPARM_BLUE ] =
		secondModel.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		secondModel.noSelfShadow = true;
		secondModel.noShadow = true;
		secondModel.origin = GetPhysics()->GetOrigin();
		secondModel.axis = GetPhysics()->GetAxis();
		secondModelDefHandle = gameRenderWorld->AddEntityDef( &secondModel );
	}

	damageFreq = spawnArgs.GetString( "def_damageFreq" );
	/*
	damageFreqMs = spawnArgs.GetInt( "damageFreqMs" );
	if( damageFreqMs <= 0 ){
		damageFreqMs = BFG_DAMAGE_FREQUENCY;
	}
	*/
	nextDamageTime = gameLocal.time + BFG_DAMAGE_FREQUENCY;

	//ff1.3 start
	idEntity* ownerEnt = owner.GetEntity();
	if ( ownerEnt && ownerEnt->IsType( idActor::Type ) ) {
		ownerTeam = static_cast<idActor*>( ownerEnt )->team;
	}
	//ff1.3 end

	FindBeamTargets(); //ff1.3 - code extracted to method
	nextBeamTargetsSearchTime = gameLocal.time + BFG_SEARCH_FREQUENCY; //ff1.3

	UpdateVisuals();
}

/*
================
idProjectile::Event_RemoveBeams
================
*/
void idBFGProjectile::Event_RemoveBeams() {
	FreeBeams();
	UpdateVisuals();
}

/*
================
idProjectile::Explode
================
*/
void idBFGProjectile::Explode( const trace_t &collision, idEntity *ignore ) {
	int			i;
	idVec3		dmgPoint;
	idVec3		dir;
	float		beamWidth;
	float		damageScale;
	//const char *damage; //ff1.3 - commented out
	idPlayer *	player;
	idEntity *	ownerEnt;
	idEntity *	targetEnt; //ff1.3

	ownerEnt = owner.GetEntity();
	if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
		player = static_cast< idPlayer * >( ownerEnt );
	} else {
		player = NULL;
	}

	beamWidth = spawnArgs.GetFloat( "beam_WidthExplode" );
	//damage = spawnArgs.GetString( "def_damage" ); //ff1.3 - commented out

	for ( i = 0; i < beamTargets.Num(); i++ ) {
		targetEnt = beamTargets[i].target.GetEntity();
		if ( ( targetEnt == NULL ) || ( ownerEnt == NULL ) ) {
			continue;
		}

		if ( !targetEnt->CanDamage( GetPhysics()->GetOrigin(), dmgPoint ) ) {
			continue;
		}

		beamTargets[i].renderEntity.shaderParms[SHADERPARM_BEAM_WIDTH] = beamWidth;

		// if the hit entity takes damage
		if ( damagePower ) {
			damageScale = damagePower;
		} else {
			damageScale = 1.0f;
		}

		// if the projectile owner is a player
		if ( player ) {
			// if the projectile hit an actor
			if ( !actorDamaged && targetEnt->IsType( idActor::Type ) ) {
				actorDamaged = true;
				//player->SetLastHitTime( gameLocal.time );
				player->AddProjectileHits( 1, hitCountGroupId );
				damageScale *= player->PowerUpModifier( PROJECTILE_DAMAGE );
			}
		}

		//ff1.3 start
		if ( damageDef != NULL ){
			dir = targetEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			dir.Normalize();
			targetEnt->Damage( this, ownerEnt, dir, damageDef->GetName(), damageScale, ( collision.c.id < 0 ) ? CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) : INVALID_JOINT );
		}

		/* was:
		if ( damage[0] && ( beamTargets[i].target.GetEntity()->entityNumber > gameLocal.numClients - 1 ) ) {
			dir = beamTargets[i].target.GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			dir.Normalize();
			beamTargets[i].target.GetEntity()->Damage( this, ownerEnt, dir, damage, damageScale, ( collision.c.id < 0 ) ? CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) : INVALID_JOINT );
		}
		*/
		//ff1.3 end
	}

	if ( secondModelDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( secondModelDefHandle );
		secondModelDefHandle = -1;
	}

	/* ff1.3 - splash damage enabled
	if ( ignore == NULL ) {
		projectileFlags.noSplashDamage = true;
	}
	*/

	if ( !gameLocal.isClient ) {
		if ( ignore != NULL ) {
			PostEventMS( &EV_RemoveBeams, 750 );
		} else {
			PostEventMS( &EV_RemoveBeams, 0 );
		}
	}

	return idProjectile::Explode( collision, ignore );
}


//ff1.3 start
#ifdef _FORCEPROJ
/*
===============================================================================

idForcefieldProjectile

===============================================================================
*/

CLASS_DECLARATION( idBFGProjectile, idForcefieldProjectile )
	//EVENT( EV_Touch,				idForcefieldProjectile::Event_Touch )
END_CLASS

/*
================
idForcefieldProjectile::idForcefieldProjectile
================
*/
idForcefieldProjectile::idForcefieldProjectile( void ) {
	teleportTolerance = 0.0f;
}


/*
================
idForcefieldProjectile::Spawn
================
*/
void idForcefieldProjectile::Spawn( void ) {
	//teleport
	teleportTolerance = spawnArgs.GetFloat( "teleportTolerance" );
	fxTeleport = spawnArgs.GetString( "fx_teleport" );

	//forcefield
	forceField.RandomTorque( 0.0f );
	forceField.Implosion( spawnArgs.GetFloat( "implosion", "30" ) );
	forceField.SetMagnitudeType( FORCEFIELD_MAGNITUDE_DISTANCE_INV );
	forceField.SetMagnitudeSphere( spawnArgs.GetFloat( "magnitude3dMin", "0" ), spawnArgs.GetFloat( "magnitude3dMax", "900" ) );
	forceField.SetApplyType( FORCEFIELD_APPLY_VELOCITY );
	forceField.SetOldVelocityPct( spawnArgs.GetFloat( "oldVelocityPct", "0.99" ) );
	forceField.SetOldVelocityProjPct( spawnArgs.GetFloat( "oldVelocityProjPct", "0.99" ) );
	//forceField.SetPlayerOnly( spawnArgs.GetBool( "playerOnly", "0" ) );
	//forceField.SetMonsterOnly( spawnArgs.GetBool( "monsterOnly", "0" ) );
	forceField.SetVelocityCompensationPct( spawnArgs.GetFloat( "velocityCompensationPct", "0" ) );
	forceField.UseWhitelist( true ); //work only on bfg targets
	forceField.IgnoreInactiveRagdolls( true ); //ignore AI until they are ragdolls

	// set the collision model on the forcefield
	forceField.SetClipModel( new idClipModel( idTraceModel( idBounds( vec3_origin ).Expand( damageRadius )))); //triggersize
	projectileFlags.ignoreForceField = true; //ignored by forcefields, including its own
}

/*
================
idForcefieldProjectile::Save
================
*/
void idForcefieldProjectile::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( teleportTolerance );
	savefile->WriteString( fxTeleport );
	savefile->WriteStaticObject( forceField );
}

/*
================
idForcefieldProjectile::Restore
================
*/
void idForcefieldProjectile::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( teleportTolerance );
	savefile->ReadString( fxTeleport );
	savefile->ReadStaticObject( forceField );
}

/*
================
idForcefieldProjectile::UpdateBeamTarget
================
*/
bool idForcefieldProjectile::UpdateBeamTarget( beamTarget_t &beamTarget, bool doDamage ) {
	bool isValidTarget = idBFGProjectile::UpdateBeamTarget( beamTarget, doDamage );
	if ( isValidTarget ) {
		//teleport
		idEntity *targetEnt = beamTarget.target.GetEntity();
		if ( targetEnt->IsType( idExplodingBarrel::Type ) || targetEnt->IsType( idPlayer::Type ) ) {
			return true;
		}
		idVec3 targetCenter = targetEnt->GetPhysics()->GetAbsBounds().GetCenter();
		idVec3 distanceVec = targetCenter - GetPhysics()->GetOrigin();
		if ( teleportTolerance > 0.0f && targetEnt->health <= 0 && (distanceVec.Normalize() < targetEnt->GetPhysics()->GetBounds().GetRadius()*teleportTolerance ) && fxTeleport.Length() ) {
			//gameLocal.Printf("Hide %s\n", targetEnt->GetName());
			targetEnt->Hide();
			forceField.RemoveFromWhiteList( targetEnt );
			idEntityFx::StartFx( fxTeleport, &targetCenter, NULL, this, false );
		}
	}
	return isValidTarget;
}

/*
=================
idForcefieldProjectile::AcceptBeamTarget
=================
*/
bool idForcefieldProjectile::AcceptBeamTarget(idEntity *ent, idVec3& damagePoint){
	bool accepted = idBFGProjectile::AcceptBeamTarget(ent, damagePoint);

	//Projectiles are affected by forcefield even though they are ignored by bfg beams.
	//Vehicles and ExplodingBarrels are not affected by forcefield even if they are targets of bfg beams.
	if ( (accepted && !ent->IsType( idAFEntity_Vehicle::Type ) && !ent->IsType( idExplodingBarrel::Type )) || ent->IsType( idProjectile::Type ) ) {
		if ( !forceField.IsWhiteListed( ent ) ) {
			//gameLocal.Printf("new forceField target %s\n", ent->GetName());
			forceField.AddToWhiteList( ent );
		}
	}

	return accepted;
}

/*
================
idForcefieldProjectile::Think
================
*/
void idForcefieldProjectile::Think( void ) {
	idBFGProjectile::Think();
	if ( state == LAUNCHED ) {
		// evaluate force
		forceField.SetParentLinearVelocity( GetPhysics()->GetLinearVelocity() );
		forceField.SetPosition( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
		forceField.Evaluate( gameLocal.time );
	}
}

/*
================
idForcefieldProjectile::Event_Touch
================

void idForcefieldProjectile::Event_Touch( idEntity *other, trace_t *trace ) {
	if ( other->IsType( idMoveable::Type ) || other->IsType( idMoveableItem::Type ) || other->IsType( idAFEntity_Base::Type ) ){
		gameLocal.Printf("ignore touched %s\n", other->GetName());
		return;
	}
	idBFGProjectile::Event_Touch( other, trace );
}
*/
#endif

/*
===============================================================================

idRailGunProjectile

===============================================================================
*/

static const float RAILGUN_MAX_TRACER_STEP_LENGHT = 4000.0f;
static const int RAILGUN_MAX_HIT_TARGETS = 10;

CLASS_DECLARATION( idProjectile, idRailGunProjectile )
END_CLASS


/*
=================
idRailGunProjectile::idRailGunProjectile
=================
*/
idRailGunProjectile::idRailGunProjectile() {
	launchPos = vec3_zero;
}

/*
=================
idRailGunProjectile::~idRailGunProjectile
=================
*/
idRailGunProjectile::~idRailGunProjectile() {

}

/*
================
idRailGunProjectile::Save
================
*/
void idRailGunProjectile::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( launchPos );
}

/*
================
idRailGunProjectile::Restore
================
*/
void idRailGunProjectile::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( launchPos );
}

/*
=================
idRailGunProjectile::Launch
=================
*/
void idRailGunProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	launchPos = start;
	idProjectile::Launch( start,dir,pushVelocity, timeSinceFire, launchPower, dmgPower );
}

/*
=================
idRailGunProjectile::Collide
=================
*/
bool idRailGunProjectile::Collide( const trace_t &collision, const idVec3 &velocity ) {
	FireRailTracer( launchPos, GetPhysics()->GetOrigin(), GetPhysics()->GetLinearVelocity() );

	return idProjectile::Collide(collision, velocity);
}

/*
=================
idRailGunProjectile::FireRailTracer
=================
*/
void idRailGunProjectile::FireRailTracer( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &velocity ){
	idVec3 tempStartPos, tempEndPos;
	idVec3 railDir;
	trace_t tr;
	idEntity* ent;
	int hitTargetCounter;
	bool actorDamaged;
	float push, damageScale, idealTracerLenght;
	idBounds hitbox;
	idPlayer *ownerPlayer;
	const char	*hitPrtName;
	const idDeclParticle *	hitPrt;
	int hitEntityNumbers[RAILGUN_MAX_HIT_TARGETS] = {};
	int i;

	//gameRenderWorld->DebugLine( colorBlue, startPos, endPos, 10000, true );

	//get hit particle
	hitPrtName = spawnArgs.GetString( "smoke_rail_hit" );
	if ( *hitPrtName != '\0' ) {
		hitPrt = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, hitPrtName ) );
	} else {
		hitPrt = NULL;
	}

	//damageScale
	if ( damagePower ) {
		damageScale = damagePower;
	} else {
		damageScale = 1.0f;
	}

	//owner
	ent = owner.GetEntity();
	if ( ent && ent->IsType( idPlayer::Type )) {
		ownerPlayer = static_cast<idPlayer *>( ent );
		damageScale *= ownerPlayer->PowerUpModifier( PROJECTILE_DAMAGE );
	} else {
		ownerPlayer = NULL;
	}

	//init loop vars
	hitTargetCounter = 0;
	actorDamaged = false;

	//prepare for trace
	hitbox = GetPhysics()->GetBounds();
	railDir = (endPos - startPos);
	railDir.Normalize();
	tempStartPos = startPos + railDir * 5.0f;

	//gameLocal.Printf("railDir: %s\n", railDir.ToString());
	//gameLocal.Printf("vel: %s, railDir: %s\n", GetPhysics()->GetLinearVelocity().ToString(), railDir.ToString());
	//gameLocal.Printf("damageDefName: %s, damageDef: %s\n", damageDefName, damageDef->GetName());

	while ( hitTargetCounter < RAILGUN_MAX_HIT_TARGETS ) {

		idealTracerLenght = (endPos - tempStartPos).Length();

		if ( idealTracerLenght > RAILGUN_MAX_TRACER_STEP_LENGHT ) {
			tempEndPos = tempStartPos + railDir * RAILGUN_MAX_TRACER_STEP_LENGHT;
		} else {
			tempEndPos = endPos;
		}

		//gameLocal.clip.TracePoint( tr, startPos, endPos, CONTENTS_RENDERMODEL, ent );
		gameLocal.clip.TraceBounds( tr, tempStartPos, tempEndPos, hitbox, CONTENTS_RENDERMODEL, ent );

		/*
		gameRenderWorld->DebugLine( colorBlue, tempStartPos, endPos, 10000, true );
		gameRenderWorld->DebugBounds( colorOrange,hitbox, tempStartPos, 10000 );
		*/

		if ( tr.fraction < 1.0f ) {
			hitTargetCounter++;

			//upd startPos
			tempStartPos = tr.endpos + railDir * 5.0f; //always move a bit forward
			//gameLocal.Printf("startPos: %s\n", tempStartPos.ToString() );

			//ent = gameLocal.GetTraceEntity( tr );
			ent = gameLocal.entities[ tr.c.entityNum ];
			//gameLocal.Printf("hit entity: %s\n", ent->GetName() );
			if ( !ent->IsType( idActor::Type ) && !ent->IsType( idExplodingBarrel::Type ) ){ //don't get the master for ExplodingBarrels
				idEntity* master = gameLocal.entities[ tr.c.entityNum ]->GetBindMaster();
				if ( master ) {
					ent = master;
				}
			}

			//don't hit the same entity twice
			for ( i = 0; i < hitTargetCounter; i++ ) {
				if ( ent->entityNumber == hitEntityNumbers[i] ) {
					break;
				}
			}
			if ( i < hitTargetCounter ) { //already hit
				//gameLocal.Printf("skip already hit entity: %s\n", ent->GetName() );
				continue;
			}
			hitEntityNumbers[hitTargetCounter] = ent->entityNumber;

			if ( ent == owner.GetEntity() ) {
				//gameLocal.Printf("owner hit -> ignored!\n" );
				continue;
			}

			//if ( lastHitEntity->IsType( idAI::Type ) ) {
			if ( ent->IsType( idActor::Type ) || ent->IsType( idExplodingBarrel::Type ) || ( ent->IsType( idAFAttachment::Type ) && static_cast<const idAFAttachment*>(ent)->GetBody()->IsType( idActor::Type ) ) ) {

				//gameLocal.Printf("hit entity: %s\n", ent->GetName() );

				// impulse
				if ( spawnArgs.GetFloat( "push", "0", push ) && push > 0.0f ) {
					ent->ApplyImpulse( this, tr.c.id, tr.c.point, push * railDir );
				}

				// damage effect
				if ( spawnArgs.GetBool( "impact_damage_effect" ) ) {
					// if the hit entity has a special damage effect
			#ifdef _DENTONMOD_PROJECTILE_CPP
					StopSound( SND_CHANNEL_BODY, false );

					if ( damageDef != NULL && (ent->IsType(idBrittleFracture::Type) || ent->IsType(idAnimatedEntity::Type) || ent->IsType(idMoveable::Type) || ent->IsType(idMoveableItem::Type)) && ent->spawnArgs.GetBool( "bleed", "1" ) ) {	// This ensures that if an entity does not have bleed key defined, it will be considered true by default
						projectileFlags.impact_fx_played = true;
						ent->AddDamageEffect( tr, velocity, damageDef->GetName(), this );
			#else
					if ( ent->spawnArgs.GetBool( "bleed" ) ) {
						ent->AddDamageEffect( tr, velocity, damageDefName );
			#endif

					} else {
						AddDefaultDamageEffect( tr, velocity );
					}
				}

				//add impact particle
				if ( hitPrt != NULL ) {
					gameLocal.smokeParticles->EmitSmoke( hitPrt, gameLocal.time, gameLocal.random.RandomFloat(), tr.c.point /*+ railDir * -5.0f*/, railDir.ToMat3(), timeGroup );
				}

				// if the hit entity takes damage
				if ( ent->fl.takedamage ) {

					// if player hit the first actor
					if ( ownerPlayer && !actorDamaged && ent->IsType( idActor::Type ) ) {
						ownerPlayer->AddProjectileHits( 1, hitCountGroupId );
						actorDamaged = true;  //upd hit counter only once
					}

					if ( damageDef != NULL ) {
						//gameLocal.Printf("Rail Damaging : %s\n", ent->GetName() );
						ent->Damage( this, owner.GetEntity(), railDir, damageDef->GetName(), damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE( tr.c.id ) );
					}
				}
			}
		} else if ( idealTracerLenght > RAILGUN_MAX_TRACER_STEP_LENGHT ) {
			tempStartPos = tempEndPos;
		} else {
			break; //endPos reached
		}
	}
}

/*
===============================================================================

idPainkillerProjectile

===============================================================================
*/

static const int PK_DAMAGE_FREQUENCY			= 150;

static const float PK_PUSH_FORCE				= 35.0f;
static const float PK_PUSH_MULT_RAGDOLL			= 10.0f;
static const float PK_PUSH_MIN_VELOCITY_Z		= 500.0f;
static const float PK_PUSH_NEAR_RADIUS			= 300.0f;

const idEventDef EV_PK_PushEntity( "<pkPushEntity>", "ev" );

CLASS_DECLARATION( idProjectile, idPainkillerProjectile )
	EVENT( EV_PK_PushEntity,		idPainkillerProjectile::Event_PushEntity )

END_CLASS


/*
=================
idPainkillerProjectile::idPainkillerProjectile
=================
*/
idPainkillerProjectile::idPainkillerProjectile() {
	memset( &beam, 0, sizeof( beam ) );
	beam.modelDefHandle = -1;

	returnPhase		= false;
	returned		= false;
	speed			= 0.0f;
	nextDamageTime	= 0;
	smokeBeamTime	= 0;

	damageBeamDef	= NULL;
}

/*
=================
idPainkillerProjectile::~idPainkillerProjectile
=================
*/
idPainkillerProjectile::~idPainkillerProjectile() {
	FreeBeam();
}

/*
================
idPainkillerProjectile::Spawn
================
*/
void idPainkillerProjectile::Spawn( void ) {
	const char	*hitPrtName;

	memset( &beam, 0, sizeof( beam ) );
	beam.modelDefHandle = -1;

	//get hit particle
	hitPrtName = spawnArgs.GetString( "smoke_beam_hit" );
	if ( *hitPrtName != '\0' ) {
		smokeDamage = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, hitPrtName ) );
	} else {
		smokeDamage = NULL;
	}
}

/*
================
idPainkillerProjectile::Save
================
*/
void idPainkillerProjectile::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( speed );
	savefile->WriteBool( returnPhase );
	savefile->WriteBool( returned );
	savefile->WriteInt( nextDamageTime );
	savefile->WriteInt( smokeBeamTime );

	savefile->WriteRenderEntity( beam.renderEntity );
	savefile->WriteInt( beam.modelDefHandle );
}

/*
================
idPainkillerProjectile::Restore
================
*/
void idPainkillerProjectile::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( speed );
	savefile->ReadBool( returnPhase );
	savefile->ReadBool( returned );
	savefile->ReadInt( nextDamageTime );
	savefile->ReadInt( smokeBeamTime );

	savefile->ReadRenderEntity( beam.renderEntity );
	savefile->ReadInt( beam.modelDefHandle );

	if ( beam.modelDefHandle >= 0 ) {
		beam.modelDefHandle = gameRenderWorld->AddEntityDef( &beam.renderEntity );
	}

	//Reinitialize the damage Def
	damageBeamDef = gameLocal.FindEntityDef( spawnArgs.GetString( "def_damage_beam" ), false );
}

/*
=================
idPainkillerProjectile::FreeBeam
=================
*/
void idPainkillerProjectile::FreeBeam() {
	if ( beam.modelDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( beam.modelDefHandle );
		beam.modelDefHandle = -1;
	}
}

/*
================
idPainkillerProjectile::StartReturnPhase
================
*/
void idPainkillerProjectile::StartReturnPhase( void ) {
	//gameLocal.Printf("StartReturnPhase\n");
	if ( !returnPhase && !returned ) {
		StartSound( "snd_return", SND_CHANNEL_WEAPON, 0, false, NULL );
		StartSound( "snd_fly", SND_CHANNEL_BODY2, 0, false, NULL ); //same channel as snd_idle

		fl.takedamage = false;
		physicsObj.SetContents( 0 );
		physicsObj.SetClipMask( 0 );
		physicsObj.Activate();
		returnPhase = true;

		if ( smokeFly ) {
			smokeFlyTime = gameLocal.time;
		}
	}
}

/*
================
idPainkillerProjectile::Think
================
*/
void idPainkillerProjectile::Think( void ) {
	idVec3		dir;
	idVec3		muzzlePos;
	idVec3		velocity;
	idVec3		tmp;
	idVec3		org;
	idMat3		axis;
	float		dist, frac;
	idEntity	*ownerEnt;
	idEntity	*hitEnt;
	trace_t		tr;
	bool		beamEnabled;

	if ( state >= LAUNCHED && !returned ){
		ownerEnt = owner.GetEntity();
		if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
			static_cast<idPlayer*>( ownerEnt )->GetPainKillerBeamData( muzzlePos, beamEnabled );
			org = physicsObj.GetOrigin();
			dir = org - muzzlePos;
			dist = dir.NormalizeFast();

			//update direction
			if ( returnPhase ) {
				if ( dist < 8.0f ) { //complete return phase
					//gameLocal.Printf("complete return phase\n");
					StopSound( SND_CHANNEL_ANY, false );
					//StartSound( "snd_return", SND_CHANNEL_BODY, 0, false, NULL );
					Hide();
					FreeLightDef();
					renderLight.lightRadius = vec3_zero; //prevent light from being re-created by idProjectile::Think()
					physicsObj.PutToRest();
					static_cast<idPlayer*>( ownerEnt )->PainKillerReturnedCallback( state == EXPLODED );
					returnPhase = false;
					beamEnabled = false;
					returned = true;
					PostEventSec( &EV_Remove, 0 );
				} else { //move back to owner
					//slow down at the end
					frac = dist / 64.0f;
					if ( frac > 1.0f ) {
						frac = 1.0f;
					} else if ( frac < 0.5f ) {
						frac = 0.5f;
					}

					velocity = -dir * speed * frac;
					physicsObj.SetLinearVelocity( velocity );

					// align z-axis of model with the direction
					axis = (-dir).ToMat3();
					tmp = axis[2];
					axis[2] = axis[0];
					axis[0] = -tmp;

					physicsObj.SetAxis( axis );
					if ( state == EXPLODED ) {
						RunPhysics(); //idProjectile won't run Physics if EXPLODED
					}
				}
			}

			//enabled?
			if ( beamEnabled && gameLocal.clip.TracePoint( tr, muzzlePos - dir * 20, org, MASK_SOLID, this ) ) { //move a bit away from walls
				//gameLocal.Printf("enabled? hit entity: %s\n", gameLocal.GetTraceEntity( tr )->GetName() );
				beamEnabled = false;
			}

			//beam damage and impact prt
			if ( beamEnabled ) {
				int hitTargets = 0;

				if ( !returnPhase ) {
					bool doDamage = ( nextDamageTime < gameLocal.time );
					if ( doDamage ) {
						nextDamageTime = gameLocal.time + PK_DAMAGE_FREQUENCY;
					}
					hitEnt = ownerEnt;
					tmp = muzzlePos;

					while ( hitTargets < 10 ) {
						gameLocal.clip.TracePoint( tr, tmp, org, MASK_SHOT_RENDERMODEL, hitEnt );

						if ( tr.fraction < 1.0f ) {
							hitTargets++;
							hitEnt = gameLocal.entities[tr.c.entityNum]; //don't use master entity. Zombie bomb would not work.

							if ( hitEnt == ownerEnt ) {
								continue;
							}

							//add impact particle
							if ( smokeDamage != NULL && !gameLocal.smokeParticles->EmitSmoke( smokeDamage, smokeBeamTime, gameLocal.random.RandomFloat(), tr.c.point - dir * 2.0f, dir.ToMat3(), timeGroup ) ) {
								smokeBeamTime = gameLocal.time;
							}

							//damage
							if ( doDamage && damageBeamDef != NULL ) {

						#ifdef _DENTONMOD_PROJECTILE_CPP
								if ( (hitEnt->IsType(idBrittleFracture::Type) || hitEnt->IsType(idAnimatedEntity::Type) || hitEnt->IsType(idMoveable::Type) || hitEnt->IsType(idMoveableItem::Type)) && hitEnt->spawnArgs.GetBool( "bleed", "1" ) ) {	// This ensures that if an entity does not have bleed key defined, it will be considered true by default
									//gameLocal.Printf("damage1 entity: %s\n", hitEnt->GetName() );
									//projectileFlags.impact_fx_played = true;
									hitEnt->AddDamageEffect( tr, dir /*velocity*/, damageBeamDef->GetName(), this );
						#else
								if ( hitEnt->spawnArgs.GetBool( "bleed" ) ) {
									hitEnt->AddDamageEffect( tr, velocity, damageDefName );
						#endif

								} else {
									//gameLocal.Printf("damage2 entity: %s\n", hitEnt->GetName() );
									AddDefaultDamageEffect( tr, dir /*velocity*/ );
								}

								// if the hit entity takes damage
								if ( hitEnt->fl.takedamage ) {
									hitEnt->Damage( this, owner.GetEntity(), dir, damageBeamDef->GetName(), 1.0f /*damageScale*/, CLIPMODEL_ID_TO_JOINT_HANDLE( tr.c.id ) );
								}
							}

							//upd startPos
							tmp = tr.endpos + dir * 2.0f;

						} else {
							break; //endPos reached
						}
					} //end while
				}

				//update beam
				beam.renderEntity.origin = GetPhysics()->GetOrigin();
				beam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_X ] = muzzlePos.x;
				beam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ] = muzzlePos.y;
				beam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ] = muzzlePos.z;
				beam.renderEntity.weaponDepthHack = !hitTargets && gameLocal.clip.TracePoint( tr, static_cast<idPlayer*>( ownerEnt )->firstPersonViewOrigin, muzzlePos, MASK_SOLID, this ); //hack useful only near walls

				if ( beam.modelDefHandle >= 0 ) {
					gameRenderWorld->UpdateEntityDef( beam.modelDefHandle, &beam.renderEntity );
				} else { //was hidden
					beam.modelDefHandle = gameRenderWorld->AddEntityDef( &beam.renderEntity );
					StartSound( "snd_beam", SND_CHANNEL_ITEM, 0, false, NULL );
				}
			} else if ( beam.modelDefHandle >= 0 ) {
				gameRenderWorld->FreeEntityDef( beam.modelDefHandle );
				beam.modelDefHandle = -1;
				StopSound( SND_CHANNEL_ITEM, false );
			}

		} else { //no owner
			PostEventMS( &EV_Remove, 0 );
		}
	}

	idProjectile::Think();
}

/*
=================
idPainkillerProjectile::Launch
=================
*/
void idPainkillerProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float power, const float dmgPower ) {
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, power, dmgPower );

	const idVec3 &vel = physicsObj.GetLinearVelocity();
	speed = vel.Length();

	idEntity *ownerEnt = owner.GetEntity();
	if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
		static_cast<idPlayer *>( ownerEnt )->SetPainKillerProjectile( this );
	}

	damageBeamDef = gameLocal.FindEntityDef( spawnArgs.GetString( "def_damage_beam" ), false );

	//beam
	float beamParm4 = spawnArgs.GetFloat( "beam_parm4" );
	float beamWidth = spawnArgs.GetFloat( "beam_width" );
	idVec3 beamColor = spawnArgs.GetVector( "beam_color" );
	const char *skin = spawnArgs.GetString( "skin_beam" );
	memset( &beam.renderEntity, 0, sizeof( renderEntity_t ) );
	beam.renderEntity.origin = GetPhysics()->GetOrigin();
	beam.renderEntity.axis = GetPhysics()->GetAxis();
	beam.renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = beamWidth;
	beam.renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = beamParm4;
	beam.renderEntity.shaderParms[ SHADERPARM_RED ] = beamColor.x;
	beam.renderEntity.shaderParms[ SHADERPARM_GREEN ] = beamColor.y;
	beam.renderEntity.shaderParms[ SHADERPARM_BLUE ] = beamColor.z;
	beam.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
	beam.renderEntity.shaderParms[ SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat() * 0.75;
	beam.renderEntity.hModel = renderModelManager->FindModel( "_beam" );
	beam.renderEntity.callback = NULL;
	beam.renderEntity.numJoints = 0;
	beam.renderEntity.joints = NULL;
	beam.renderEntity.bounds.Clear();
	beam.renderEntity.weaponDepthHack = true;
	beam.renderEntity.customSkin = declManager->FindSkin( skin );
	beam.modelDefHandle = gameRenderWorld->AddEntityDef( &beam.renderEntity );

	StartSound( "snd_beam", SND_CHANNEL_ITEM, 0, false, NULL );
	StartSound( "snd_fly", SND_CHANNEL_BODY, 0, false, NULL );

	UpdateVisuals();
	BecomeActive( TH_THINK ); //must update the beam
}

/*
================
idPainkillerProjectile::Event_PushEntity
================
*/
void idPainkillerProjectile::Event_PushEntity( idEntity *ent, const idVec3 &hitPos ) {
	PushEntity(ent, hitPos);
}

/*
================
idPainkillerProjectile::PushEntity
================
*/
void idPainkillerProjectile::PushEntity( idEntity *ent, const idVec3 &hitPos ) {
	//gameLocal.Printf("PushEntityTowardPlayer\n");
	float distance;
	float multiplier;
	idVec3 dir;
	idVec3 forceVec;
	idVec3 delta;

	if ( ent && owner.GetEntity() ) {
		if ( ent->IsType( idMoveable::Type ) || ent->IsType( idAFEntity_Vehicle::Type ) ) {
			multiplier = 1.0f;
		} else {
			multiplier = PK_PUSH_MULT_RAGDOLL;
		}

		delta = owner.GetEntity()->GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin();
		distance = delta.NormalizeFast();
		delta.z += 0.75f;
		forceVec = delta * sqrt(distance) * PK_PUSH_FORCE * multiplier /* * fftest1.GetFloat()  * ent->GetPhysics()->GetMass() */;

		//extra Z force for near objects
		if ( distance < PK_PUSH_NEAR_RADIUS ) {
			//gameLocal.Printf("near distance fix\n");
			forceVec.x *= (distance/PK_PUSH_NEAR_RADIUS);
			forceVec.y *= (distance/PK_PUSH_NEAR_RADIUS);
			forceVec.z *= (PK_PUSH_NEAR_RADIUS - distance) * 0.015;
		}

		//min Z force
		if ( forceVec.z < PK_PUSH_MIN_VELOCITY_Z * multiplier ) {
			//gameLocal.Printf("min Z fix\n"); //0.6
			forceVec.z = PK_PUSH_MIN_VELOCITY_Z * multiplier;
		}

		//ent->ApplyImpulse( this, 0, hitPos, forceVec ); //hitPos
		ent->GetPhysics()->SetLinearVelocity( forceVec );
		//gameLocal.Printf("push distance: %f, vec: %f  %f  %f, mass: %f\n", distance, forceVec.x, forceVec.y, forceVec.z, ent->GetPhysics()->GetMass());
	}
}

/*
================
idPainkillerProjectile::SchedulePush
================
*/
void idPainkillerProjectile::SchedulePush( idEntity* ent, idVec3 pos ) {
	if ( ent->GetBindMaster() && ShouldPush(ent->GetBindMaster()) ) {
		PostEventMS( &EV_PK_PushEntity, gameLocal.msec, ent->GetBindMaster(), pos );
	} else if ( ShouldPush(ent) ) {
		PostEventMS( &EV_PK_PushEntity, gameLocal.msec, ent, pos );
	}
}

/*
================
idPainkillerProjectile::ShouldPush
================
*/
bool idPainkillerProjectile::ShouldPush( idEntity* ent ) {
	return ( ent->IsType( idMoveable::Type ) || ( ent->IsType( idAFEntity_Base::Type ) && static_cast<idAFEntity_Base *>(ent)->IsActiveAF() ) );
}

/*
================
idPainkillerProjectile::ShouldReturnOn
================
*/
bool idPainkillerProjectile::ShouldReturnOn( idEntity* ent ) {
	return ( ent->IsType( idMoveable::Type ) || ( ent->IsType( idAFEntity_Base::Type )  || ent->IsType( idDamagable::Type ) ) );
}

/*
================
idPainkillerProjectile::Explode
================
*/
void idPainkillerProjectile::Explode( const trace_t &collision, idEntity *ignore ) {

	if ( state == EXPLODED || state == FIZZLED ) {
		return;
	}

	// play explode sound
	StartSound( "snd_explode", SND_CHANNEL_BODY, 0, true, NULL );

	// we need to work out how long the effects last and then remove them at that time
	// for example, bullets have no real effects
	if ( smokeFly && smokeFlyTime ) {
		smokeFlyTime = 0;
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );

	state = EXPLODED;

	if ( gameLocal.isClient ) {
		return;
	}

	// alert the ai
	gameLocal.AlertAI( owner.GetEntity(), physicsObj.GetOrigin() );

	if ( collision.c.type != CONTACT_NONE && gameLocal.entities[collision.c.entityNum] ) {
		idEntity* contactEnt = gameLocal.entities[collision.c.entityNum];
		SchedulePush( contactEnt, collision.endpos ); //give AI ragdoll time to start

		if ( ShouldReturnOn( contactEnt ) || ( contactEnt->GetBindMaster() && ShouldReturnOn( contactEnt->GetBindMaster() ) ) ) {
			StartReturnPhase();
		} else {
			physicsObj.PutToRest();
			StartSound( "snd_idle", SND_CHANNEL_BODY2, 0, true, NULL ); //same channel as snd_fly for return phase
		}
	}

	CancelEvents( &EV_Explode );
}

/*
================
idPainkillerProjectile::Fizzle
================
*/
void idPainkillerProjectile::Fizzle( void ) {

	if ( state == EXPLODED || state == FIZZLED ) {
		return;
	}

	StopSound( SND_CHANNEL_BODY, false );
	StartSound( "snd_fizzle", SND_CHANNEL_BODY, 0, false, NULL );

	fl.takedamage = false;
	physicsObj.SetContents( 0 );

	state = FIZZLED;

	if ( gameLocal.isClient ) {
		return;
	}

	StartReturnPhase();

	CancelEvents( &EV_Fizzle );
}


/*
===============================================================================

idRemoteGrenadeProjectile

===============================================================================
*/

CLASS_DECLARATION( idProjectile, idRemoteGrenadeProjectile )
END_CLASS

/*
=================
idRemoteGrenadeProjectile::idRemoteGrenadeProjectile
=================
*/
idRemoteGrenadeProjectile::idRemoteGrenadeProjectile() {
	allowPickupTime = 0;
	allowGrabTime = 0;
}

/*
================
idRemoteGrenadeProjectile::Save
================
*/
void idRemoteGrenadeProjectile::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( allowPickupTime );
	savefile->WriteInt( allowGrabTime );
}

/*
================
idRemoteGrenadeProjectile::Restore
================
*/
void idRemoteGrenadeProjectile::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( allowPickupTime );
	savefile->ReadInt( allowGrabTime );
}

/*
================
idRemoteGrenadeProjectile::Think
================
*/
void idRemoteGrenadeProjectile::Think( void ) {
	if ( state == LAUNCHED ){
		if ( allowGrabTime > 0 && allowGrabTime < gameLocal.time ) {
			allowGrabTime = 0;
			SetOwner(NULL); //this is about grabber/clip and does not change the "owner" variable of the projectile
		}
		if ( allowPickupTime > 0 && allowPickupTime < gameLocal.time && owner.GetEntity() ) {
			idEntity *ownerEnt = owner.GetEntity();

			//const idBounds myBounds = idBounds( GetPhysics()->GetOrigin() ).Expand( 32 );
			const idBounds &myBounds = GetPhysics()->GetAbsBounds();
			const idBounds &entBounds = ownerEnt->GetPhysics()->GetAbsBounds();

			if ( myBounds.IntersectsBounds( entBounds ) ) {
				if ( ownerEnt->IsType( idPlayer::Type ) && static_cast<idPlayer *>( ownerEnt )->Give( "pw_remote_grenade", "1" ) ){
					allowPickupTime = 0;
					PostEventMS( &EV_Remove, 0 );
				}
			}
		}
	}
	idProjectile::Think();
}

/*
================
idRemoteGrenadeProjectile::Explode
================
*/
void idRemoteGrenadeProjectile::Explode( const trace_t &collision, idEntity *ignore ) {
	idProjectile::Explode( collision, ignore );

	idEntity *ownerEnt = owner.GetEntity();
	if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
		static_cast<idPlayer *>( ownerEnt )->SetRemoteGrenadeProjectile( NULL );
	}
}

/*
=================
idRemoteGrenadeProjectile::Launch
=================
*/
void idRemoteGrenadeProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float power, const float dmgPower ) {
	allowGrabTime = gameLocal.time + 1000;
	allowPickupTime = gameLocal.time + 1000;
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, power, dmgPower );

	idEntity *ownerEnt = owner.GetEntity();
	if ( ownerEnt && ownerEnt->IsType( idPlayer::Type ) ) {
		static_cast<idPlayer *>( ownerEnt )->SetRemoteGrenadeProjectile( this );
	}
	BecomeActive( TH_THINK ); //must check pickup
}

/*
================
idRemoteGrenadeProjectile::CatchProjectile
================
*/
void idRemoteGrenadeProjectile::CatchProjectile( idEntity* o, const char* reflectName ) {
	idProjectile::CatchProjectile(o, reflectName);
	SetOwner(NULL); //allow multiple catching
}



/*
===============================================================================

idVagaryProjectile

===============================================================================
*/

CLASS_DECLARATION( idGuidedProjectile, idVagaryProjectile )
END_CLASS


/*
=================
idVagaryProjectile::idVagaryProjectile
=================
*/
idVagaryProjectile::idVagaryProjectile() {

}

/*
================
idVagaryProjectile::Think
================
*/
void idVagaryProjectile::Think( void ) {
	idGuidedProjectile::Think();

	if ( state == LAUNCHED && !GetOwner() ){
		PostEventMS( &EV_Explode, 0 );
	}
}

/*
=================
idVagaryProjectile::Collide
=================
*/
bool idVagaryProjectile::Collide( const trace_t &collision, const idVec3 &velocity ) {
	if ( state == EXPLODED || state == FIZZLED ) {
		return true;
	}
	bool result = idGuidedProjectile::Collide( collision, velocity );
	if ( result && state == EXPLODED ) {
		PullTarget( collision );
	}
	return result;
}

/*
=================
idVagaryProjectile::PullTarget
=================
*/
void idVagaryProjectile::PullTarget( const trace_t &collision ) {
	idEntity	*targetEnt;
	idEntity	*ownerEnt;

	targetEnt = gameLocal.entities[ collision.c.entityNum ];
	ownerEnt = GetOwner();

	if ( targetEnt && ownerEnt ) {
		ownerEnt->spawnArgs.SetBool("enemy_hit", true); //used by Vagary script
		idVec3 dir = ownerEnt->GetPhysics()->GetOrigin() - targetEnt->GetPhysics()->GetOrigin();
		float power = idMath::Sqrt( dir.Length() ) / 2.0f;
		targetEnt->GetPhysics()->SetLinearVelocity( dir + idVec3(0.0f, 0.0f, 55.0f) * power );
	}
}

//ff1.3 end

/*
===============================================================================

	idDebris

===============================================================================
*/

CLASS_DECLARATION( idEntity, idDebris )
EVENT( EV_Explode,			idDebris::Event_Explode )
EVENT( EV_Fizzle,			idDebris::Event_Fizzle )
END_CLASS

/*
================
idDebris::Spawn
================
*/
void idDebris::Spawn( void ) {
	owner = NULL;
	smokeFly = NULL;
	smokeFlyTime = 0;
	nextSoundTime = 0;		// BY Clone JCD
	soundTimeDifference = 0; //
}

/*
================
idDebris::Create
================
*/
void idDebris::Create( idEntity *owner, const idVec3 &start, const idMat3 &axis ) {
	Unbind();
	GetPhysics()->SetOrigin( start );
	GetPhysics()->SetAxis( axis );
	GetPhysics()->SetContents( 0 );
	this->owner = owner;
	smokeFly = NULL;
	smokeFlyTime = 0;
	nextSoundTime = 0;		// BY Clone JCD
	soundTimeDifference = 0; //
	sndBounce = NULL;
#ifdef _D3XP
	noGrab = true;
#endif
	UpdateVisuals();
}

/*
=================
idDebris::idDebris
=================
*/
idDebris::idDebris( void ) {
	owner = NULL;
	smokeFly = NULL;
	smokeFlyTime = 0;
	sndBounce = NULL;
	nextSoundTime = 0;		// BY Clone JCD
	soundTimeDifference = 0; //
}

/*
=================
idDebris::~idDebris
=================
*/
idDebris::~idDebris( void ) {
}

/*
=================
idDebris::Save
=================
*/
void idDebris::Save( idSaveGame *savefile ) const {
	owner.Save( savefile );

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteParticle( smokeFly );
	savefile->WriteInt( smokeFlyTime );
 //   savefile->WriteInt( nextSoundTime );		// No need to store this value, BY Clone JCD
	savefile->WriteInt( soundTimeDifference ); //
	savefile->WriteSoundShader( sndBounce );
}

/*
=================
idDebris::Restore
=================
*/
void idDebris::Restore( idRestoreGame *savefile ) {
	owner.Restore( savefile );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadParticle( smokeFly );
	savefile->ReadInt( smokeFlyTime );
    //savefile->ReadInt( nextSoundTime );		// No need to store this value, BY Clone JCD
	savefile->ReadInt( soundTimeDifference ); //
	savefile->ReadSoundShader( sndBounce );
}

/*
=================
idDebris::Launch
=================
*/
void idDebris::Launch( void ) {
	float		fuse;
	idVec3		velocity;
#ifdef _DENTONMOD
	idVec3		angular_velocity_vect;
#else
	idAngles	angular_velocity;
#endif
	float		linear_friction;
	float		angular_friction;
	float		contact_friction;
	float		bounce;
	float		mass;
	float		gravity;
	idVec3		gravVec;
	bool		randomVelocity;
	idMat3		axis;

//ff1.3 start - fix slow debris of fast projectiles
#ifdef _D3XP
	SetTimeState ts( timeGroup );
#endif
//ff1.3 end

	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	spawnArgs.GetVector( "velocity", "0 0 0", velocity );
#ifdef _DENTONMOD
	angular_velocity_vect = spawnArgs.GetAngles( "angular_velocity", "0 0 0").ToAngularVelocity();
#else
	spawnArgs.GetAngles( "angular_velocity", "0 0 0", angular_velocity );
#endif
	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	angular_friction	= spawnArgs.GetFloat( "angular_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );
	fuse				= spawnArgs.GetFloat( "fuse" );
	randomVelocity		= spawnArgs.GetBool ( "random_velocity" );
	continuousSmoke		= spawnArgs.GetBool ( "smoke_continuous" );
	if ( mass <= 0 ) {
		gameLocal.Error( "Invalid mass on '%s'\n", GetEntityDefName() );
	}

	if ( randomVelocity ) {
#ifdef _DENTONMOD
		float rand = spawnArgs.GetFloat("linear_velocity_rand", "0.35");

		// sets velocity randomly between ((1-rand)*100)% and ((1+rand)*100)%
		// e.g.1: if rand = 0.2, velocity will be randomly set between 80% and 120%
		// e.g.2: if rand = 0.3, velocity will be randomly set between 70% and 130%
		// and so on.
		velocity.x *= gameLocal.random.RandomFloat()*rand*2.0 + 1.0 -  rand;
		velocity.y *= gameLocal.random.RandomFloat()*rand*2.0 + 1.0 -  rand;
		velocity.z *= gameLocal.random.RandomFloat()*rand*2.0 + 1.0 -  rand;

		// do not perform following calculations unless there's key in decl that says so.
		if( spawnArgs.GetFloat( "angular_velocity_rand", "0.0", rand) && rand > 0.0f ) {
			angular_velocity_vect.x *= gameLocal.random.RandomFloat()*rand*2.0 + 1.0 -  rand;
			angular_velocity_vect.y *= gameLocal.random.RandomFloat()*rand*2.0 + 1.0 -  rand;
			angular_velocity_vect.z *= gameLocal.random.RandomFloat()*rand*2.0 + 1.0 -  rand;
		}
#else
		velocity.x *= gameLocal.random.RandomFloat() + 0.5f;
		velocity.y *= gameLocal.random.RandomFloat() + 0.5f;
		velocity.z *= gameLocal.random.RandomFloat() + 0.5f;
#endif
	}

	if ( health ) {
		fl.takedamage = true;
	}

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();
	axis = GetPhysics()->GetAxis();

	Unbind();

	physicsObj.SetSelf( this );

	// check if a clip model is set
	const char *clipModelName;
	idTraceModel trm;

	//ff1.3 - use box unless clipmodel is explicitly set. This fixes warnings and errors for complex models.
	
	/*was:
	spawnArgs.GetString( "clipmodel", "", &clipModelName );
	if ( !clipModelName[0] ) {
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	// load the trace model
	if ( !collisionModelManager->TrmFromModel( clipModelName, trm ) ) {
		// default to a box
		physicsObj.SetClipBox( renderEntity.bounds, 1.0f );
	} else {
		physicsObj.SetClipModel( new idClipModel( trm ), 1.0f );
	}
	*/
	if ( spawnArgs.GetString( "clipmodel", "", &clipModelName ) && clipModelName[0] && collisionModelManager->TrmFromModel( clipModelName, trm ) ) {
		physicsObj.SetClipModel( new idClipModel( trm ), 1.0f );
	} else {
		// default to a box
		physicsObj.SetClipBox( renderEntity.bounds, 1.0f );
	}
	//ff1.3 end

	physicsObj.GetClipModel()->SetOwner( owner.GetEntity() );
	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linear_friction, angular_friction, contact_friction );
	if ( contact_friction == 0.0f ) {
		physicsObj.NoContact();
	}
	physicsObj.SetBouncyness( bounce );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetContents( 0 );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
#ifdef _DENTONMOD
	// Make sure that the linear velocity is added with
	// owner's linear velocity for more accurate physics simulation.
	idEntity *ownerEnt = owner.GetEntity();
	if( ownerEnt != NULL ) {
		physicsObj.SetLinearVelocity( (axis[ 0 ] * velocity[ 0 ] + axis[ 1 ] * velocity[ 1 ] + axis[ 2 ] * velocity[ 2 ]) + ownerEnt->GetPhysics()->GetLinearVelocity());
	}
	else {
		physicsObj.SetLinearVelocity( axis[ 0 ] * velocity[ 0 ] + axis[ 1 ] * velocity[ 1 ] + axis[ 2 ] * velocity[ 2 ] );
	}
	physicsObj.SetAngularVelocity( angular_velocity_vect * axis );
#else
	physicsObj.SetLinearVelocity( axis[ 0 ] * velocity[ 0 ] + axis[ 1 ] * velocity[ 1 ] + axis[ 2 ] * velocity[ 2 ] );
	physicsObj.SetAngularVelocity( angular_velocity.ToAngularVelocity() * axis );
#endif

	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( axis );
	SetPhysics( &physicsObj );

	if ( !gameLocal.isClient ) {
		if ( fuse <= 0 ) {
			// run physics for 1 second
			RunPhysics();
			PostEventMS( &EV_Remove, 0 );
		} else if ( spawnArgs.GetBool( "detonate_on_fuse" ) ) {
			if ( fuse < 0.0f ) {
				fuse = 0.0f;
			}
			RunPhysics();
			PostEventSec( &EV_Explode, fuse );
		} else {
			if ( fuse < 0.0f ) {
				fuse = 0.0f;
			}
			PostEventSec( &EV_Fizzle, fuse );
		}
	}

	StartSound( "snd_fly", SND_CHANNEL_BODY, 0, false, NULL );

	smokeFly = NULL;
	smokeFlyTime = 0;
	const char *smokeName = spawnArgs.GetString( "smoke_fly" );
	if ( *smokeName != '\0' ) {
		smokeFly = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

	const char *sndName = spawnArgs.GetString( "snd_bounce" );

	nextSoundTime = 0;		// BY Clone JCD
	soundTimeDifference = spawnArgs.GetInt ( "next_sound_time" ); //

	 if ( *sndName != '\0' ) {
		sndBounce = declManager->FindSound( sndName );
	}

	UpdateVisuals();
}

/*
================
idDebris::Think
================
*/
void idDebris::Think( void ) {

	// run physics
	RunPhysics();
	Present();

	if ( smokeFly && smokeFlyTime && !IsHidden()) { // Emit particles only when visible
		idVec3 dir = -GetPhysics()->GetLinearVelocity();
		dir.Normalize();
		if ( !gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup /*_D3XP*/  ) ) {

			if( continuousSmoke ) {
				smokeFlyTime = gameLocal.time; // Emit particles continuously - Clone JC Denton
			}
			else {
				smokeFlyTime = 0;
			}
		}
	}
}

/*
================
idDebris::Killed
================
*/
void idDebris::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( spawnArgs.GetBool( "detonate_on_death" ) ) {
		Explode();
	} else {
		Fizzle();
	}
}

/*
=================
idDebris::Collide
=================
*/
bool idDebris::Collide( const trace_t &collision, const idVec3 &velocity ) {

	if (sndBounce != NULL ){
		if ( !soundTimeDifference ) {
			StartSoundShader( sndBounce, SND_CHANNEL_BODY, 0, false, NULL );
			sndBounce = NULL;
			return false;
		}

		if ( gameLocal.time > nextSoundTime ){

			float v = -( velocity * collision.c.normal );

			if ( v > BOUNCE_SOUND_MIN_VELOCITY ) {
				float f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
				if ( StartSoundShader( sndBounce, SND_CHANNEL_BODY, 0, false, NULL ) )
					SetSoundVolume( f );
			}
			else {
				float f = ( 0.5f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
				if ( StartSoundShader( sndBounce, SND_CHANNEL_BODY, 0, false, NULL ) )
					SetSoundVolume( f );
				sndBounce = NULL;
				return false;
			}
			nextSoundTime = gameLocal.time + soundTimeDifference;
		}
	}
	//sndBounce = NULL;
	return false;
}


/*
================
idDebris::Fizzle
================
*/
void idDebris::Fizzle( void ) {
	if ( IsHidden() ) {
		// already exploded
		return;
	}

	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_fizzle", SND_CHANNEL_BODY, 0, false, NULL );

	// fizzle FX
	const char *smokeName = spawnArgs.GetString( "smoke_fuse" );
	if ( *smokeName != '\0' ) {
		smokeFly = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();
#ifdef _DENTONMOD
	BecomeInactive(TH_PHYSICS); // This causes the physics not to update after explosion
#endif
	Hide();

	if ( gameLocal.isClient ) {
		return;
	}

	CancelEvents( &EV_Fizzle );
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idDebris::Explode
================
*/
void idDebris::Explode( void ) {
	if ( IsHidden() ) {
		// already exploded
		return;
	}

	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_explode", SND_CHANNEL_BODY, 0, false, NULL );

	Hide();

	// these must not be "live forever" particle systems
	smokeFly = NULL;
	smokeFlyTime = 0;
	const char *smokeName = spawnArgs.GetString( "smoke_detonate" );
	if ( *smokeName != '\0' ) {
		smokeFly = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();
#ifdef _DENTONMOD
	BecomeInactive(TH_PHYSICS); // This causes the physics not to update after explosion
#endif
	CancelEvents( &EV_Explode );
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idDebris::Event_Explode
================
*/
void idDebris::Event_Explode( void ) {
	Explode();
}

/*
================
idDebris::Event_Fizzle
================
*/
void idDebris::Event_Fizzle( void ) {
	Fizzle();
}
