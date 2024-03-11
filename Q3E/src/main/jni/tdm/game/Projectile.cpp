/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//
 
#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "ProjectileResult.h"
#include "StimResponse/StimResponseCollection.h" // grayman #2885

/*
===============================================================================

	idProjectile

===============================================================================
*/


const idEventDef EV_Explode( "<explode>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Fizzle( "<fizzle>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_RadiusDamage( "<radiusdmg>", EventArgs('e', "", ""), EV_RETURNS_VOID, "internal" );
const idEventDef EV_GetProjectileState( "getProjectileState", EventArgs(), 'd', 
	"Gets the current state of the projectile. States are defined in tdm_defs.script" );
// greebo: The launch method (takes 3 vectors as arguments)
const idEventDef EV_Launch("launch", 
	EventArgs('v', "start", "", 'v', "dir", "", 'v', "velocity", ""), EV_RETURNS_VOID, 
	"Launches the projectile from <start> in direction <dir> with the given <velocity>");
const idEventDef EV_ActivateProjectile("<activateProjectile>", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_TDM_Mine_ClearPlayerImmobilization("_EV_TDM_Mine_ClearPlayerImmobilization", 
	EventArgs('e', "", ""), EV_RETURNS_VOID, "internal"); // grayman #2478 - allows player to handle weapons again
const idEventDef EV_Mine_Replace("<mine_Replace>", EventArgs(), EV_RETURNS_VOID, "internal"); // grayman #2478

CLASS_DECLARATION( idEntity, idProjectile )
	EVENT( EV_Explode,				idProjectile::Event_Explode )
	EVENT( EV_Fizzle,				idProjectile::Event_Fizzle )
	EVENT( EV_Touch,				idProjectile::Event_Touch )
	EVENT( EV_RadiusDamage,			idProjectile::Event_RadiusDamage )
	EVENT( EV_GetProjectileState,	idProjectile::Event_GetProjectileState )
	EVENT( EV_Launch,				idProjectile::Event_Launch )
	EVENT( EV_ActivateProjectile,	idProjectile::Event_ActivateProjectile )
	EVENT( EV_TDM_Mine_ClearPlayerImmobilization, idProjectile::Event_ClearPlayerImmobilization ) // grayman #2478
	EVENT( EV_TDM_Lock_OnLockPicked, idProjectile::Event_Lock_OnLockPicked ) // grayman #2478
	EVENT( EV_Mine_Replace,	idProjectile::Event_Mine_Replace ) // grayman #2478
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
	damagePower			= 1.0f;
	nextDamageTime		= 0;
	memset( &projectileFlags, 0, sizeof( projectileFlags ) );
	memset( &renderLight, 0, sizeof( renderLight ) );

	m_Lock				= NULL;  // grayman #2478
	isMine				= false; // grayman #2478
	replaced			= false; // grayman #2908
	hasBounced			= false;
}

/*
================
idProjectile::Spawn
================
*/
void idProjectile::Spawn( void ) {
	physicsObj.SetSelf( this );

	if (!GetPhysics()->GetClipModel()->IsTraceModel())
	{
		// greebo: Clipmodel is not a trace model, try to construct it from the collision mesh
		idTraceModel traceModel;

		if ( !collisionModelManager->TrmFromModel( spawnArgs.GetString("model"), traceModel ) )
		{
			gameLocal.Error( "idProjectile '%s': cannot load tracemodel from %s", name.c_str(), spawnArgs.GetString("model") );
			return;
		}

		// Construct a new clipmodel from that loaded tracemodel
		physicsObj.SetClipModel(new idClipModel(traceModel), 1);
	}
	else
	{
		// Use the existing clipmodel, it's good enough
		physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	}
	
	physicsObj.SetContents( 0 );
	physicsObj.SetClipMask( 0 );
	physicsObj.PutToRest();
	SetPhysics( &physicsObj );
}

void idProjectile::AddObjectsToSaveGame(idSaveGame* savefile) // grayman #2478
{
	idEntity::AddObjectsToSaveGame(savefile);

	savefile->AddObject(m_Lock);
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

	savefile->WriteInt( (int)state );

	savefile->WriteFloat( damagePower );
	savefile->WriteInt( nextDamageTime );
	savefile->WriteBool(isMine);	// grayman #2478
	savefile->WriteBool(replaced);	// grayman #2908
	savefile->WriteBool(hasBounced);

	savefile->WriteStaticObject( physicsObj );
	savefile->WriteStaticObject( thruster );

	// The lock class is saved by the idSaveGame class on close, no need to handle it here
	savefile->WriteObject(m_Lock); // grayman #3353
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

	savefile->ReadInt( (int &)state );

	savefile->ReadFloat( damagePower );
	savefile->ReadInt( nextDamageTime );
	savefile->ReadBool(isMine);		// grayman #2478
	savefile->ReadBool(replaced);	// grayman #2908
	savefile->ReadBool(hasBounced);	// grayman #2908

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadStaticObject( thruster );
	thruster.SetPhysics( &physicsObj );

	if ( smokeFly != NULL ) {
		idVec3 dir;
		dir = physicsObj.GetLinearVelocity();
		dir.NormalizeFast();
		gameLocal.smokeParticles->EmitSmoke( smokeFly, gameLocal.time, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}

	// The lock class is restored by the idRestoreGame, don't handle it here
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Lock)); // grayman #3353
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
idProjectile::SetReplaced
================
*/
void idProjectile::SetReplaced() // grayman #2908
{
	this->replaced = true;
}

/*
================
idProjectile::Create
================
*/
void idProjectile::Create( idEntity *owner, const idVec3 &start, const idVec3 &dir ) {
	idDict		args;
	idStr		shaderName;
	idVec3		light_color;
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

	memset( &renderLight, 0, sizeof( renderLight ) );
	shaderName = spawnArgs.GetString( "mtr_light_shader" );
	if ( *(const char*)shaderName ) {
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
		renderLight.noShadows = spawnArgs.GetBool("light_noshadows", "0");
	}

	spawnArgs.GetVector( "light_offset", "0 0 0", lightOffset );

	lightStartTime = 0;
	lightEndTime = 0;
	smokeFlyTime = 0;

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
	delete m_Lock; // grayman #2478
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
	float			delay;
	float			gravity;
	idVec3			gravVec;
	idVec3			tmp;
	idMat3			axis;
	int				contents;
	int				clipMask;

	// allow characters to throw projectiles during cinematics, but not the player
	if ( owner.GetEntity() && !owner.GetEntity()->IsType( idPlayer::Type ) ) {
		cinematic = owner.GetEntity()->cinematic;
	} else {
		cinematic = false;
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
					
	// greebo: Fall back to default gravity if the spawnarg is missing completely (still allows "0" gravity being set)
	if (!spawnArgs.GetFloat("gravity", "0", gravity))
	{
		gravity = gameLocal.GetGravity().Length();
	}

	fuse				= spawnArgs.GetFloat( "fuse" );
	delay				= spawnArgs.GetFloat( "delay" );

	projectileFlags.detonate_on_world	= spawnArgs.GetBool( "detonate_on_world" );
	projectileFlags.detonate_on_actor	= spawnArgs.GetBool( "detonate_on_actor" );
	projectileFlags.detonate_on_water	= spawnArgs.GetBool( "detonate_on_water" ); // grayman #1104
	projectileFlags.randomShaderSpin	= spawnArgs.GetBool( "random_shader_spin" );

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
	clipMask = MASK_SHOT_RENDERMODEL;
	if ( spawnArgs.GetBool( "detonate_on_trigger" ) ) {
		contents |= CONTENTS_TRIGGER;
	}
	if ( !spawnArgs.GetBool( "no_contents" ) ) {
		contents |= CONTENTS_PROJECTILE|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP;
		clipMask |= CONTENTS_PROJECTILE;
	}

	// don't do tracers on client, we don't know origin and direction
	if ( spawnArgs.GetBool( "tracers" ) && gameLocal.random.RandomFloat() > 0.5f ) {
		SetModel( spawnArgs.GetString( "model_tracer" ) );
		projectileFlags.isTracer = true;
	}

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

	// greebo: Allow overriding of the projectile orientation via "angles" spawnarg
	idAngles angles;
	if (spawnArgs.GetAngles("angles", "0 0 0", angles))
	{
		axis = angles.ToMat3();
	}
	physicsObj.SetAxis( axis );

	// grayman #2478 - is this a mine?
	isMine =  spawnArgs.GetBool("is_mine","0");

	thruster.SetPosition( &physicsObj, 0, idVec3( GetPhysics()->GetBounds()[ 0 ].x, 0, 0 ) );

	{
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
		} 
		// greebo: Added this to allow for mines
		else if ( spawnArgs.GetBool( "no_fizzle" ) ) {
			if ( fuse < 0.0f ) {
				fuse = 0.0f;
			}
		}
		else {
			fuse -= timeSinceFire;
			if ( fuse < 0.0f ) {
				fuse = 0.0f;
			}
			PostEventSec( &EV_Fizzle, fuse );
		}
	}

	if (delay > 0.0f) {
		// Activate this projectile in <delay> seconds
		PostEventSec( &EV_ActivateProjectile, delay);
	}

	if ( projectileFlags.isTracer ) {
		StartSound( "snd_tracer", SND_CHANNEL_BODY, 0, false, NULL );
	} else {
		StartSound( "snd_fly", SND_CHANNEL_BODY, 0, false, NULL );
	}

	smokeFlyTime = 0;
	const char *smokeName = spawnArgs.GetString( "smoke_fly" );
	if ( *smokeName != '\0' ) {
		smokeFly = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeFlyTime = gameLocal.time;
	}

	// used for the plasma bolts but may have other uses as well
	if ( projectileFlags.randomShaderSpin ) {
		float f = gameLocal.random.RandomFloat();
		f *= 0.5f;
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = f;
	}

	UpdateVisuals();

	// Set this to inactive if there is a delay set
	state = (delay > 0) ? INACTIVE : LAUNCHED;
}

/*
=========================
idProjectile::IsMine - grayman #2478
=========================
*/

bool idProjectile::IsMine()
{
	return isMine;
}

/*
=========================
idProjectile::IsArmed - grayman #2906
=========================
*/

bool idProjectile::IsArmed()
{
	return ( state == LAUNCHED );
}

/*
=========================
idProjectile::AngleAdjust - grayman #2478
=========================
*/

float idProjectile::AngleAdjust(float angle)
{
	// Adjust a mine's pitch and roll angles so that it wants to
	// return to the vertical. A larger adjustment occurs when
	// the given angle is near +- 90 degrees. A smaller adjustment
	// occurs when the angle is near zero or near +- 180 degrees.
	// This keeps mines from flipping over if they land upside-down,
	// but allows them to correct when in flight. The amount of
	// adjustment varies between 0 and 5 degrees.

	float factor = 5.0f/90.0f;
	float adjust = 0;

	if (abs(angle) >= 10) // small adjustments are not interesting
	{
		if (angle < -90)
		{
			adjust = factor*angle + 10;
		}
		else if (angle < 90)
		{
			adjust = -factor*angle;
		}
		else
		{
			adjust = factor*angle - 10;
		}
	}

	return adjust;
}

/*
================
idProjectile::Think
================
*/

void idProjectile::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		if ( thrust && ( gameLocal.time < thrust_end ) ) {
			// evaluate force
			thruster.SetForce( GetPhysics()->GetAxis()[ 0 ] * thrust );
			thruster.Evaluate( gameLocal.time );
		}
	}

	if ( IsMine() )
	{
		// grayman #2478 - if this is a mine in flight, adjust its pitch and
		// roll slightly so it has a better chance of landing rightside-up.

		if ( !IsAtRest() && !(thinkFlags & TH_ARMED) )
		{
			idAngles ang = GetPhysics()->GetAxis().ToAngles();
			ang.pitch += AngleAdjust(ang.pitch);
			ang.roll += AngleAdjust(ang.roll);
			SetAngles(ang);
		}

		if ( thinkFlags & TH_ARMED ) // grayman #2478 - if armed, this is a ticking mine
		{
			// Any AI around? For AI who manage to avoid a collision with the mine, catch them here if they're close enough and moving/rotating.

			idVec3 org = GetPhysics()->GetOrigin();
			idBounds bounds = GetPhysics()->GetAbsBounds();
			float closeEnough = Square(23);

			// get all entities touching the bounds
			idClip_EntityList entityList;
			int numListedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, -1, entityList );

			for ( int i = 0 ; i < numListedEntities ; i++ )
			{
				idEntity* ent = entityList[ i ];
				if ( ent && ent->IsType(idAI::Type) )
				{
					idAI* entAI = static_cast<idAI*>(ent);
					if ( entAI->AI_FORWARD || (entAI->GetPhysics()->GetAngularVelocity().LengthSqr() > 0) )
					{
						if ( (entAI->GetPhysics()->GetOrigin() - org).LengthSqr() <= closeEnough )
						{
							MineExplode( entAI->entityNumber );
							break;
						}
					}
				}
			}
		}
	}

	/*idStr stateStr;
	switch (state)
	{
	case SPAWNED: stateStr = "SPAWNED"; break;
	case CREATED: stateStr = "CREATED"; break;
	case LAUNCHED: stateStr = "LAUNCHED"; break;
	case FIZZLED: stateStr = "FIZZLED"; break;
	case EXPLODED: stateStr = "EXPLODED"; break;
	case INACTIVE: stateStr = "INACTIVE"; break;
	};

	gameRenderWorld->DebugText(stateStr, physicsObj.GetOrigin(), 0.2f, colorRed, gameLocal.GetLocalPlayer()->viewAxis);*/

	// run physics
	RunPhysics();

	Present();

	// add the particles
	if ( smokeFly != NULL && smokeFlyTime && !IsHidden() ) {
		idVec3 dir = -GetPhysics()->GetLinearVelocity();
		dir.Normalize();
		if ( !gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3() ) ) {
			smokeFlyTime = gameLocal.time;
		}
	}

	// add the light
	if ( renderLight.lightRadius.x > 0.0f && g_projectileLights.GetBool() ) {
		renderLight.origin = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * lightOffset;
		renderLight.axis = GetPhysics()->GetAxis();
		if ( ( lightDefHandle != -1 ) ) {
			if ( lightEndTime > 0 && gameLocal.time <= lightEndTime + USERCMD_MSEC ) {
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
	idStr		SurfTypeName;

	if ( state == INACTIVE ) {
		// projectile not active, return FALSE
		return false;
	}

	if ( state == EXPLODED || state == FIZZLED ) {
		return true;
	}

	// remove projectile when a 'noimpact' surface is hit
	if ( ( collision.c.material != NULL ) && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) ) {
		PostEventMS( &EV_Remove, 0 );
		common->DPrintf( "Projectile collision no impact\n" );
		return true;
	}

	// get the entity the projectile collided with
	ent = gameLocal.entities[ collision.c.entityNum ];
	bool hitWater = ent->GetPhysics()->IsType( idPhysics_Liquid::Type ); // grayman #1104

	if ( ent )
	{
		ProcCollisionStims( ent, collision.c.id );
		
		// grayman #4009 - account for attachments to an AI
		idEntity* master = ent->GetBindMaster();
		if ( !master )
		{
			master = ent;
		}
		
		if ( master->IsType( idAI::Type ) )
		{
			idAI *alertee = static_cast<idAI *>(master);
			alertee->TactileAlert( this );
		}
	}

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
	if ( spawnArgs.GetFloat( "push", "0", push ) && ( push > 0.0f ) )
	{
		idVec3 pushDir = dir;
		if ( IsMine() ) // grayman #2478 - if this is a mine, push up
		{
			pushDir = idVec3( 0,0,1 );
		}

		// grayman #4412 - Does the struck object absorb the projectile?
		if ( !ent->spawnArgs.GetBool("absorb_projectile", "0") )
		{
			ent->ApplyImpulse(this, collision.c.id, collision.c.point, push * pushDir);
		}
	}

	// MP: projectiles open doors
	// tels: TODO 2010-06-11 need another way to find out if entity is a door (low priority, we don't have MP in TDM yet)
	//if ( gameLocal.isMultiplayer && ent->IsType( idDoor::Type ) && !static_cast< idDoor * >(ent)->IsOpen() && !ent->spawnArgs.GetBool( "no_touch" ) ) {
	//	ent->ProcessEvent( &EV_Activate , this );
	//}

	//is this a bouncing projectile?
	if ( !projectileFlags.detonate_on_actor || !projectileFlags.detonate_on_world )
	{
		//if an AF attachment was hit, look for an AI that it might be attached to
		idEntity* bounceEnt = ent;

		if( bounceEnt->IsType(idAFAttachment::Type) && static_cast<const idAFAttachment*>(ent)->GetBody()->IsType(idActor::Type) )
		{
			bounceEnt = static_cast<const idAFAttachment*>(ent)->GetBody();
		}

		//did the projectile hit an entity type that it can bounce off?
		if( bounceEnt->IsType(idActor::Type) )
		{
			if( !projectileFlags.detonate_on_actor )
			{
				Bounced( collision, velocity, bounceEnt );
				return false;
			}
		}
		else if( !projectileFlags.detonate_on_world )
		{
			Bounced( collision, velocity, bounceEnt);
			return false;
		}
	}

	SetOrigin( collision.endpos );
	SetAxis( collision.endAxis );

	// unlink the clip model because we no longer need it
	GetPhysics()->UnlinkClip();

	if ( hitWater ) // grayman #1104
	{
		damageDefName = spawnArgs.GetString( "def_damage_water" );
	}
	else
	{
		damageDefName = spawnArgs.GetString( "def_damage" );
	}

	ignore = NULL;

	bool damageInflicted = false; // grayman #2794 - whether damage was inflicted below

	// if the hit entity takes damage
	if ( ent->fl.takedamage ) 
	{
		if ( damagePower ) {
			damageScale = damagePower;
		} else {
			damageScale = 1.0f;
		}

		// scale the damage by the surface type multiplier, if any
		g_Global.GetSurfName( collision.c.material, SurfTypeName );
		SurfTypeName = "damage_mult_" + SurfTypeName;

		damageScale *= spawnArgs.GetFloat( SurfTypeName.c_str(), "1.0" ); 

		// if the projectile owner is a player
		if ( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) ) {
			// if the projectile hit an actor
			if ( ent->IsType( idActor::Type ) ) {
				idPlayer *player = static_cast<idPlayer *>( owner.GetEntity() );
				player->AddProjectileHits( 1 );
				damageScale *= player->PowerUpModifier( PROJECTILE_DAMAGE );
			}
		}

		if ( damageDefName[0] != '\0' )
		{
			// grayman #2794 - if no damage is being inflicted, then there's no need for a damage effect

			const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, true ); // grayman #3391 - don't create a default 'damageDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
			if ( !damageDef )
			{
				gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
			}

			int	damage = damageDef->GetInt( "damage" );
			damageInflicted = ( damage > 0 );

			// If a mine is unarmed, it won't take damage or explode.

			if ( damageInflicted )
			{
				if ( ent->IsType(idProjectile::Type) )
				{
					idProjectile *proj = static_cast<idProjectile*>(ent);
					if ( proj->IsMine() && !proj->IsArmed() ) // is mine armed?
					{
						damageInflicted = false;
					}
				}
			}

			if ( damageInflicted ) // grayman #2906 - only run the damage code if there's damage
			{
				ent->Damage( this, owner.GetEntity(), dir, damageDefName, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ), const_cast<trace_t *>(&collision) );
			}
			else // grayman #4009 - let AI react to getting hit, even if there's no damage
			{
				// Check if the projectile is a moss blob. If it is, the AI
				// will have no reaction to it.
				// TODO : a flying mine should cause the same reaction
				if (idStr::FindText(name.c_str(), "projectile_mossblob") < 0)
				{
					// grayman #4009 - account for attachments to an AI
					idEntity* master = ent->GetBindMaster();
					if ( !master )
					{
						master = ent;
					}

					if ( master->IsType(idAI::Type) )
					{
						// Send a signal to the current state that we've been hit by something
						static_cast<idAI*>(master)->GetMind()->GetState()->OnProjectileHit(this, owner.GetEntity(), 0);
					}
				}
			}

			ignore = ent;
		}
	}

	// grayman #3178 - allow damage decal if damage was inflicted or the projectile hit the world or hit a func_static

	if ( damageInflicted || ( ent == gameLocal.world ) || ent->IsType(idStaticEntity::Type) ) // grayman #2794
	{
		// if the projectile causes a damage effect
		if ( spawnArgs.GetBool( "impact_damage_effect" ) )
		{
			// if the hit entity has a special damage effect
			if ( ent->spawnArgs.GetBool( "bleed" ) )
			{
				ent->AddDamageEffect( collision, velocity, damageDefName );
			}
			else
			{
				AddDefaultDamageEffect( collision, velocity );
			}
		}
	}

	Explode( collision, ignore );

	return true;
}

/*
=================================
idProjectile::AddDamageEffect - grayman #2478
=================================
*/

void idProjectile::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName )
{
	AddDefaultDamageEffect( collision, velocity );
}

/*
=================
idProjectile::DefaultDamageEffect
=================
*/

void idProjectile::DefaultDamageEffect( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity ) {
	const char *decal, *sound;
	idStr typeName;

	if ( collision.c.material != NULL ) 
	{
		g_Global.GetSurfName( collision.c.material, typeName );
	} 
	else 
	{
		typeName = gameLocal.surfaceTypeNames[ SURFTYPE_METAL ];
	}
	
	// play impact sound
	sound = projectileDef.GetString( va( "snd_%s", typeName.c_str() ) );
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
	decal = projectileDef.GetString( va( "mtr_detonate_%s", typeName.c_str() ) );
	if ( *decal == '\0' ) {
		decal = projectileDef.GetString( "mtr_detonate" );
	}

	if ( *decal != '\0' ) {
		gameLocal.ProjectDecal( collision.c.point, -collision.c.normal, 8.0f, true, projectileDef.GetFloat( "decal_size", "6.0" ), decal,
								0.0f, gameLocal.entities[collision.c.entityNum], true); // last 2 params (target entity and save=true) added #3817 for persistent decals -- SteveL
	}
}

/*
=================
idProjectile::AddDefaultDamageEffect
=================
*/
void idProjectile::AddDefaultDamageEffect( const trace_t &collision, const idVec3 &velocity ) {

	DefaultDamageEffect( this, spawnArgs, collision, velocity );
}

/*
================
idProjectile::Killed
================
*/
void idProjectile::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( spawnArgs.GetBool( "detonate_on_death" ) )
	{
		trace_t collision;

		memset( &collision, 0, sizeof( collision ) );
		collision.endAxis = GetPhysics()->GetAxis();
		collision.endpos = GetPhysics()->GetOrigin();
		collision.c.point = GetPhysics()->GetOrigin();
		collision.c.normal.Set( 0, 0, 1 );
		AddDefaultDamageEffect( collision, collision.c.normal ); // grayman #3424
		Explode( collision, NULL );
		physicsObj.ClearContacts();
		physicsObj.PutToRest();
	}
	else
	{
		Fizzle();
	}
}

/*
================
idProjectile::Fizzle
================
*/
void idProjectile::Fizzle( void ) {

	if ( state == INACTIVE || state == EXPLODED || state == FIZZLED ) {
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

	Hide();
	FreeLightDef();

	state = FIZZLED;

	CancelEvents( &EV_Fizzle );
	PostEventMS( &EV_Remove, spawnArgs.GetInt( "remove_time", "1500" ) );
}

/**
* greebo: This event is being scheduled by the Launch() method, if a "delay" is set on the projectileDef
*/
void idProjectile::Event_ActivateProjectile()
{
	state = LAUNCHED;

	// grayman #2478 - arm it if it's a mine
	
	if ( IsMine() )
	{
		// The mine is now armed, so loop the armed sound.

		// grayman #2908 - determine if this mine was set by the map author or thrown by the player
		if ( !replaced )
		{
			m_SetInMotionByActor = gameLocal.GetLocalPlayer();
		}

		StartSound( "snd_mine_armed", SND_CHANNEL_BODY, 0, true, NULL );

		// Make it frobable. It won't highlight, though, for any inventory
		// item other than a lockpick, so that the player doesn't get the
		// impression that he can frob it.

		SetFrobable( true );
		m_FrobDistance = cv_frob_distance_default.GetInteger();

		// Make it locked and pickable.

		// Set up our PickableLock instance

		m_Lock = static_cast<PickableLock*>( PickableLock::CreateInstance() );
		m_Lock->SetOwner( this ); // grayman #3803 - must be done before InitFromSpawnargs()
		m_Lock->InitFromSpawnargs( spawnArgs ); // Load the spawnargs for the lock
		m_Lock->SetLocked( true );

		BecomeActive( TH_ARMED ); // guarantee continued thinking
	}
}

/*
================
idProjectile::Event_RadiusDamage
================
*/

void idProjectile::Event_RadiusDamage( idEntity *ignore ) {
	const char *splash_damage = spawnArgs.GetString( "def_splash_damage" );
	if ( splash_damage[0] != '\0' ) {
		gameLocal.RadiusDamage( physicsObj.GetOrigin(), this, owner.GetEntity(), ignore, this, splash_damage, damagePower );
	}
}

/*
================
idProjectile::Event_GetProjectileState
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
	const char *fxname, *light_shader, *sndExplode;
	idStr		SurfTypeName;
	float		light_fadetime;
	idVec3		normal, tempVel, tempAngVel;
	int			removeTime;
	bool		bActivated;

	// grayman #3424 - if a mine in an INACTIVE state (before it's armed) is
	// Killed, it should explode.

	if ( !isMine && ( state == INACTIVE || state == EXPLODED || state == FIZZLED ) )
	{
		return;
	}

	// stop sound
	StopSound( SND_CHANNEL_BODY2, false );
	StopSound( SND_CHANNEL_BODY, false );

	// DarkMod: Check material list to see if it's activated
	g_Global.GetSurfName( collision.c.material, SurfTypeName );
	if ( collision.c.material ) {
		auto matImg = collision.c.material->GetMaterialImage();
		if ( !matImg.IsEmpty() )
			gameRenderWorld->MaterialTrace( collision.c.point, collision.c.material, SurfTypeName );
	}
		
	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Weapon: Projectile surface was %s\r", SurfTypeName.c_str() );

	bActivated = TestActivated( SurfTypeName.c_str() );
// TODO: Add spawnarg option to only play explode sound and explode light on activate

	// play explode sound
	switch ( ( int ) damagePower ) {
		case 2: sndExplode = "snd_explode2"; break;
		case 3: sndExplode = "snd_explode3"; break;
		case 4: sndExplode = "snd_explode4"; break;
		default: sndExplode = "snd_explode"; break;
	}
	StartSound( sndExplode, SND_CHANNEL_BODY, 0, true, NULL );

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
	GetPhysics()->SetOrigin( collision.endpos + 5.0f * collision.c.normal );

	// default remove time
	removeTime = spawnArgs.GetInt( "remove_time", "1500" );

	// change the model, usually to a PRT
	fxname = NULL;
	if ( g_testParticle.GetInteger() == TEST_PARTICLE_IMPACT ) {
		fxname = g_testParticleName.GetString();
	} else {
		fxname = spawnArgs.GetString( "model_detonate" );
	}

	int surfaceType = collision.c.material != NULL ? collision.c.material->GetSurfaceType() : SURFTYPE_METAL;
	if ( !( fxname && *fxname ) ) {
		if ( ( surfaceType == SURFTYPE_NONE ) || ( surfaceType == SURFTYPE_METAL ) || ( surfaceType == SURFTYPE_STONE ) ) {
			fxname = spawnArgs.GetString( "model_smokespark" );
		} else if ( surfaceType == SURFTYPE_RICOCHET ) {
			fxname = spawnArgs.GetString( "model_ricochet" );
		} else {
			fxname = spawnArgs.GetString( "model_smoke" );
		}
	}

	if ( fxname && *fxname ) {
		SetModel( fxname );
		renderEntity.shaderParms[SHADERPARM_RED] = 
		renderEntity.shaderParms[SHADERPARM_GREEN] = 
		renderEntity.shaderParms[SHADERPARM_BLUE] = 
		renderEntity.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat();
		Show();
		removeTime = ( removeTime > 3000 ) ? removeTime : 3000;
	}

	// explosion light
	light_shader = spawnArgs.GetString( "mtr_explode_light_shader" );
	if ( *light_shader ) {
		renderLight.shader = declManager->FindMaterial( light_shader, false );
		renderLight.pointLight = true;
		renderLight.lightRadius[0] =
		renderLight.lightRadius[1] =
		renderLight.lightRadius[2] = spawnArgs.GetFloat( "explode_light_radius" );
		spawnArgs.GetVector( "explode_light_color", "1 1 1", lightColor );
		renderLight.shaderParms[SHADERPARM_RED] = lightColor.x;
		renderLight.shaderParms[SHADERPARM_GREEN] = lightColor.y;
		renderLight.shaderParms[SHADERPARM_BLUE] = lightColor.z;
		renderLight.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
		renderLight.noShadows = spawnArgs.GetBool("explode_light_noshadows", "0");
		light_fadetime = spawnArgs.GetFloat( "explode_light_fadetime", "0.5" );
		lightStartTime = gameLocal.time;
		lightEndTime = gameLocal.time + SEC2MS( light_fadetime );
		BecomeActive( TH_THINK );
	}

	// store the last known velocity and angular velocity for later use
	tempVel = GetPhysics()->GetLinearVelocity();
	tempAngVel = GetPhysics()->GetAngularVelocity();

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	state = EXPLODED;
	BecomeInactive( TH_ARMED ); // grayman #2478 - disable armed thinking

	//
	// bind the projectile to the impact entity if necesary
	// NOW: with special handling for the bind to AFEntity case.
	// Lloyd: Fixed binding objects to bodies: this doesn't work for animated objects, 
	//		need to bind to joints instead
	if ( gameLocal.entities[collision.c.entityNum] && spawnArgs.GetBool( "bindOnImpact" ) ) {
		idEntity *e = gameLocal.entities[ collision.c.entityNum ];

		if( e->IsType( idAFEntity_Base::Type ) ) {

//			idAFEntity_Base *af = static_cast< idAFEntity_Base * >( e );

			this->BindToJoint( e, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ), true );
		}
		else {
			Bind( gameLocal.entities[collision.c.entityNum], true );
		}
	}

	// splash damage
	if ( !projectileFlags.noSplashDamage )
	{
		float delay = spawnArgs.GetFloat( "delay_splash" );
		if ( delay )
		{
			if ( removeTime < delay * 1000 )
			{
				removeTime = static_cast<int>( delay + 0.10f ) * 1000;
			}
			PostEventSec( &EV_RadiusDamage, delay, ignore );
		}
		else
		{
			Event_RadiusDamage( ignore );
		}
	}

	// spawn debris entities
	int fxdebris = spawnArgs.GetInt( "debris_count" );
	if ( fxdebris ) {
		const idDict *debris = gameLocal.FindEntityDefDict( "projectile_debris", false );
		if ( debris ) {
			int amount = gameLocal.random.RandomInt( fxdebris );
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
		debris = gameLocal.FindEntityDefDict( "projectile_shrapnel", false );
		if ( debris ) {
			int amount = gameLocal.random.RandomInt( fxdebris );
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

	// DarkMod: Spawn projectile result entity
	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Checking projectile result:\r" );
	if ( spawnArgs.GetBool( "has_result", "0" ) )
	{
		DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Has_result set to true\r" );
		const char* resultName = "";

		// grayman #1104 - is there a different result when colliding with water?
		idEntity* ent = gameLocal.entities[collision.c.entityNum];
		if ( ent && ent->GetPhysics()->IsType( idPhysics_Liquid::Type ) )
		{
			resultName = spawnArgs.GetString("def_result_water");
		}

		if ( idStr(resultName).IsEmpty() )
		{
			resultName = spawnArgs.GetString("def_result");
		}

		const idDict *resultDef = gameLocal.FindEntityDefDict( resultName, false );
		if ( resultDef )
		{
			// grayman #1104 - some projectile results (i.e. gas) aren't allowed in water

			bool createResult = true;
			int contents = gameLocal.clip.Contents( collision.endpos, NULL, mat3_identity, -1, this );
			if (contents & MASK_WATER)
			{
				// Result wants to spawn in water. Allowed?
				createResult = spawnArgs.GetBool("allow_result_in_water","1");
			}

			if ( createResult )
			{
				idEntity *ent2;

				DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING("Result object found for projectile %s\r", name.c_str());

				gameLocal.SpawnEntityDef( *resultDef, &ent2, false );
				if ( !ent2 || !ent2->IsType( CProjectileResult::Type ) ) 
				{
					DM_LOG(LC_WEAPON, LT_ERROR)LOGSTRING("Projectile %s has a non projectile result entity in projectile_result.\r", name.c_str());
					gameLocal.Error( "'projectile_result' is not a CProjectileResult" );
				}

				DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING( "Spawned projectile result\r" );

				CProjectileResult *result = static_cast<CProjectileResult *>( ent2 );

				// Populate the data object to pass to the projectile result object
				SFinalProjData DataIn;
			
				DataIn.Owner = owner.GetEntity();
				DataIn.FinalOrigin = collision.endpos;
				DataIn.FinalAxis = GetPhysics()->GetAxis();
				DataIn.LinVelocity = tempVel;
				DataIn.AngVelocity = tempAngVel;
				// rotate the axial direction by the axis to get world direction vector
				DataIn.AxialDir = DataIn.FinalAxis * spawnArgs.GetVector( "axial_dir", "0 0 1" );
				DataIn.mass = GetPhysics()->GetMass();
				DataIn.SurfaceType = SurfTypeName;

				// Set up the projectile result with the last known results of the projectile
				result->Init( &DataIn, collision, this, bActivated );
			}
		}
	}

	CancelEvents( &EV_Explode );
	PostEventMS( &EV_Remove, removeTime );

	// grayman #2885 - turn off any visual stims this projectile is generating
	
	if ( GetStimResponseCollection()->HasStim() )
	{
		GetStimResponseCollection()->RemoveStim( ST_VISUAL );
	}
}

/*
================
idProjectile::Bounced
================
*/
void idProjectile::Bounced( const trace_t &collision, const idVec3 &velocity, idEntity *bounceEnt ) {

	//If this is the first bounce, check whether the mapper has specified new physics properties for after the first bounce
	if( !hasBounced )
	{
		hasBounced = true;

		const idKeyValue *kv = NULL;
		while( kv = spawnArgs.MatchPrefix("postbounce_", kv) )
		{
			idStr key		= kv->GetKey();
			idStr val		= kv->GetValue();
			idStr suffix	= key.Right( key.Length() - 11 );	// "postbounce_gravity" > "gravity"

			if( suffix == "gravity" )
			{
				idVec3 gravVec = gameLocal.GetGravity();
				gravVec.NormalizeFast();
				physicsObj.SetGravity( gravVec * atof(val) );
			}
			else if (suffix == "friction")
			{
				//[0]: linear, [1]: angular, [2]: contact friction
				idVec3 friction = spawnArgs.GetVector( key, "0 0 0" );
				physicsObj.SetFriction(friction[0], friction[1], friction[2]);
				if ( friction[2] == 0.0f ) {
					physicsObj.NoContact();
				}
			}
			else if (suffix == "bounce")
			{
				physicsObj.SetBouncyness( atof(val) );
			}
			else if (suffix == "mass")
			{
				physicsObj.SetMass( atof(val) );
			}
		}
	}

	//play bounce sounds if a world entity was hit and ricochet sounds are disabled
	if( !bounceEnt->IsType(idActor::Type) && !StartSound("snd_ricochet", SND_CHANNEL_ITEM, 0, true, NULL) )
	{
		float bounceSoundMinVelocity = spawnArgs.GetFloat("bounce_sound_min_velocity", "200");
		float bounceSoundMaxVelocity = spawnArgs.GetFloat("bounce_sound_max_velocity", "400");

		if ( ( velocity.Length() > bounceSoundMinVelocity ) && !spawnArgs.GetBool("no_bounce_sound", "0") ) // grayman #3331 - some projectiles should not propagate a bounce sound
		{
			/* Dragofer: this method based on SetSoundVolume is not suitable for projectiles in its current form because other sounds i.e. the explosion are also quietened
				// angua: modify the volume set in the def instead of setting a fixed value. 
				// At minimum velocity, the volume should be "min_velocity_volume_decrease" lower (in db) than the one specified in the def
				float minVelocityVolumeDecrease = abs(spawnArgs.GetFloat("min_velocity_volume_decrease", "0") );
				float f = (velocity.Length() > bounceSoundMaxVelocity) ? 0.0f : minVelocityVolumeDecrease * (idMath::Sqrt(velocity.Length() - bounceSoundMinVelocity) * (1.0f / idMath::Sqrt(bounceSoundMaxVelocity - bounceSoundMinVelocity)) - 1);

				const char* sound				= spawnArgs.GetString("snd_bounce");
				const idSoundShader* sndShader	= declManager->FindSound(sound);
				float volume					= sndShader->GetParms()->volume + f;
				SetSoundVolume(volume);
			*/

			StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, true, NULL );
		}
	}

	//apply bounce-specific damage. Modified and abbreviated version of the damage component of idMoveable::Collide. Projectile mass does not play a role.
	//world entities can also be damaged, i.e. glass or breakable containers
	idStr	damageDef = spawnArgs.GetString("def_damage_bounce", "");
	int		minDamageVelocity = spawnArgs.GetInt("min_damage_velocity", "200");
	int		maxDamageVelocity = spawnArgs.GetInt("max_damage_velocity", "600");

	if ( damageDef.Length() && ( gameLocal.time > nextDamageTime ) )
	{
		float f;
		int preHealth = bounceEnt->health;
		int location = 0;
		trace_t newCollision = collision; // grayman #2816 - in case we need to modify collision

		//calculate how much damage to apply
		if ( velocity.Length() < minDamageVelocity )
			f = 0.0f;
		else if ( velocity.Length() < maxDamageVelocity )
			f = idMath::Sqrt(( velocity.Length() - minDamageVelocity) / (maxDamageVelocity - minDamageVelocity));
		else
			f = 1.0f; // capped when v >= maxDamageVelocity

		idVec3 dir = velocity;
		dir.NormalizeFast();

		// Blame the attack on the one who launched the projectile.
		// Otherwise, assume it was put in motion by someone.

		idEntity* attacker = GetOwner();
		if ( attacker == NULL )
		{
			attacker = m_SetInMotionByActor.GetEntity();
		}

		if ( bounceEnt->IsType(idActor::Type) )
		{
			// Use a technique similar to what's used for a melee collision
			// to find a better joint (location), because when the head is
			// hit, the joint isn't identified correctly w/o it.

			location = JOINT_HANDLE_TO_CLIPMODEL_ID( newCollision.c.id );
		}

		// Apply damage 
		bounceEnt->Damage( this, attacker, dir, damageDef, f, location, const_cast<trace_t *>(&newCollision) );
		if ( bounceEnt->health < preHealth ) // only set the timer if there was damage
		{
			nextDamageTime = gameLocal.time + 1000;
		}
	}
}

// grayman #2934

void idProjectile::AttackAction(idPlayer* player)
{
	if ( m_Lock != NULL )
	{
		m_Lock->AttackAction(player);
	}
}



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
idProjectile::Event_Launch
================
*/
void idProjectile::Event_Launch( idVec3 const &origin, idVec3 const &direction, idVec3 const &velocity ) {
	Launch(origin, direction, velocity);
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
idProjectile::MineExplode - grayman #2478
================
*/
void idProjectile::MineExplode( int entityNumber ) // entityNumber belongs to the entity that stepped on you
{
	if ( state == LAUNCHED ) // only explode if you're armed and haven't already blown up
	{
		trace_t collision;
		memset( &collision, 0, sizeof( collision ) );
		collision.endAxis = GetPhysics()->GetAxis();
		collision.endpos = GetPhysics()->GetOrigin();
		collision.c.point = collision.endpos;
		collision.c.normal.Set( 0, 0, 1 );
		collision.c.entityNum = entityNumber;
		AddDefaultDamageEffect( collision, collision.c.normal );
		Collide( collision, idVec3(0,0,0) );
	}
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
void idProjectile::Event_Touch( idEntity *other, trace_t *trace )
{
	if ( state == INACTIVE ) {
		// greebo: projectile not active yet, return FALSE
		return;
	}

	if ( IsHidden() ) {
		return;
	}

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

	// At this point it's for sure that the projectile is colliding with an entity, check the type

	// don't do anything if hitting a noclip player
	if ( ent->IsType( idPlayer::Type ) && static_cast<idPlayer *>( ent )->noclip ) {
		return false;
	}

	// Are we hitting an actor?
	if ( ent->IsType( idActor::Type ) || ( ent->IsType( idAFAttachment::Type ) && static_cast<const idAFAttachment*>(ent)->GetBody()->IsType( idActor::Type ) ) ) {
		// Yes, check if we should detonate on actors
		if ( !projectileDef.GetBool( "detonate_on_actor" ) ) {
			return false;
		}
	} else {
		// Not an actor, check if we should detonate on something else
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
	msg.WriteBits( owner.GetEntityNum(), 32 );
	msg.WriteBits( owner.GetSpawnNum(), 32 );

	msg.WriteBits( state, 3 );

	msg.WriteBits( fl.hidden, 1 );

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

/*
================
idProjectile::ReadFromSnapshot
================
*/
void idProjectile::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	projectileState_t newState;

	int entId = msg.ReadBits( 32 );
	int spnId = msg.ReadBits( 32 );
	owner.SetDirectlyWithIntegers( entId, spnId );

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
			case INACTIVE: {
				state = INACTIVE;
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
	return false;
}

/*
================
idProjectile::TestActivated
DarkMod Addition
================
*/

bool idProjectile::TestActivated( const char *typeName ) 
{
	bool		bReturnVal(false), bAssumeActive(false);
	idStr		MaterialsList;

	if( !spawnArgs.GetBool( "assume_active", "0" ) )
	{
		if( !spawnArgs.GetString( "active_surfaces", "", MaterialsList )
			|| !typeName )
		{
			// return false if the surfaces list is blank
			goto Quit;
		}
	}
	else
	{
		bAssumeActive = true;

		if( !spawnArgs.GetString( "dud_surfaces", "", MaterialsList )
			|| !typeName )
		{
			// return true if the surfaces list is blank and we assume active
			bReturnVal = true;
			goto Quit;
		}
	}

	// pad front and back with spaces for unique name searching
	MaterialsList.Insert(' ', 0 );
	MaterialsList.Append(' ');

	bReturnVal = ( MaterialsList.Find( va(" %s ", typeName) ) != -1 );
	// this XOR should cover both cases, assumed active and found dud, assumed dud found active
	bReturnVal ^= bAssumeActive;

Quit:
	return bReturnVal;
}

/*
=========================
idProjectile::CanBeUsedBy - grayman #2478
=========================
*/

bool idProjectile::CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) 
{
	if (item == NULL)
	{
		return false;
	}

	assert(item->Category() != NULL);

	// FIXME: Move this to idEntity to some sort of "usable_by_inv_category" list?
	const idStr& categoryName = item->Category()->GetName();
	if (categoryName == "#str_02389" )				// Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			return false;
		}

		// Lockpicks behave similar to keys
		return (isFrobUse) ? IsLocked() : true;
	}

	return false;
}

/*
===================
idProjectile::UseByItem - grayman #2478
===================
*/

bool idProjectile::UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	if (item == NULL)
	{
		return false;
	}

	assert(item->Category() != NULL);

	// Retrieve the entity behind that item and reject NULL entities
	idEntity* itemEntity = item->GetItemEntity();
	if (itemEntity == NULL)
	{
		return false;
	}

	// Get the name of this inventory category
	const idStr& categoryName = item->Category()->GetName();

	if (categoryName == "#str_02389" )				// Lockpicks
	{
		if (!m_Lock->IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobDoor %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks are different, we need to look at the button state
		// First we check if this item is a lockpick. It has to be of the toolclass lockpick
		// and the type must be set.
		idStr str = itemEntity->spawnArgs.GetString( "lockpick_type", "" );

		if (str.Length() == 1)
		{
			// greebo: Check if the item owner is a player, and if yes, 
			// update the immobilization flags.
			idEntity* itemOwner = item->GetOwner();

			if (itemOwner->IsType(idPlayer::Type))
			{
				idPlayer* playerOwner = static_cast<idPlayer*>(itemOwner);
				playerOwner->SetImmobilization("Lockpicking", EIM_ATTACK);

				// Schedule an event 1/3 sec. from now, to enable weapons again after this time
				CancelEvents(&EV_TDM_Mine_ClearPlayerImmobilization);
				PostEventMS(&EV_TDM_Mine_ClearPlayerImmobilization, 300, playerOwner);
			}

			// Pass the call to the lockpick routine
			return m_Lock->ProcessLockpickImpulse(impulseState, static_cast<int>(str[0]));
		}
		else
		{
			gameLocal.Warning("Wrong 'type' spawnarg for lockpicking on item %s, must be a single character.", itemEntity->name.c_str());
			return false;
		}
	}

	return false;
}

/*
======================
idProjectile::IsLocked - grayman #2478
======================
*/

bool idProjectile::IsLocked()
{
	return m_Lock->IsLocked();
}

/*
================================
idProjectile::DetonateOnWater - grayman #1104
================================
*/

bool idProjectile::DetonateOnWater()
{
	return projectileFlags.detonate_on_water;
}

/*
================================
idProjectile::SetNoSplashDamage - grayman #1104
================================
*/

void idProjectile::SetNoSplashDamage( bool setting )
{
	projectileFlags.noSplashDamage = setting;
}

/*
=============================================
idProjectile::Event_ClearPlayerImmobilization - grayman #2478
=============================================
*/

void idProjectile::Event_ClearPlayerImmobilization(idEntity* player)
{
	if (!player->IsType(idPlayer::Type))
	{
		return;
	}

	// Release the immobilization imposed on the player by Lockpicking
	static_cast<idPlayer*>(player)->SetImmobilization("Lockpicking", 0);

	// stgatilov #4968: stop lockpicking if player's frob is broken
	// note: release does not look at lockpick type, so we pass garbage
	m_Lock->ProcessLockpickImpulse(EReleased, '-');
}

/*
=====================================
idProjectile::Event_Lock_OnLockPicked - grayman #2478
=====================================
*/

void idProjectile::Event_Lock_OnLockPicked()
{
	// "Lock is picked" signal

	m_Lock->SetLocked(false);
	state = INACTIVE; // disarm the mine
	BecomeInactive( TH_ARMED ); // disable armed thinking
	StartSound( "snd_mine_disarmed", SND_CHANNEL_BODY, 0, true, NULL );
	SetFrobable(false); // can't frob this until after the disarm sound is finished
	PostEventSec( &EV_Mine_Replace, 1.0f );
}

/*
================================
idProjectile::Event_Mine_Replace - grayman #2478
================================
*/

void idProjectile::Event_Mine_Replace()
{
	// Change the projectile into a mine entity the player can put back into inventory.

	const char* resultName = spawnArgs.GetString("def_disarmed");

	const idDict *resultDef = gameLocal.FindEntityDefDict( resultName, false );
	if ( resultDef )
	{
		idEntity *newMine;
		gameLocal.SpawnEntityDef( *resultDef, &newMine, false );
		newMine->GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() );
		newMine->GetPhysics()->SetAxis( GetPhysics()->GetAxis() );
		newMine->BecomeInactive( TH_PHYSICS ); // turn off physics so the new mine doesn't sink through the floor

		SetFrobable(false);
		PostEventMS( &EV_Remove, 1 ); // Remove the projectile, which has been replaced
	}
}


/*
===============================================================================

	idGuidedProjectile

===============================================================================
*/

const idEventDef EV_LaunchGuided("launchGuided",
	EventArgs('v', "start", "", 'v', "dir", "", 'v', "velocity", "", 'E', "target", ""), EV_RETURNS_VOID,
	"Launches the guided projectile from <start> in direction <dir> with the given <velocity> at <target entity>. Pass $null_entity to fire an unguided projectile.");

CLASS_DECLARATION( idProjectile, idGuidedProjectile )
	EVENT( EV_LaunchGuided,				idGuidedProjectile::Event_Launch )
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
	// The lock class is restored by the idRestoreGame, don't handle it here
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

/*
=================
idGuidedProjectile::Launch
=================
*/
void idGuidedProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, float dmgPower ) {
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );

	//If an enemy isn't already provided, such as by Event_Launch, try other means to acquire an enemy
	if( !enemy.GetEntity() )
	{ 
		//check if an enemy has been set as a spawnarg on the projectile
		//otherwise get the enemy of the entity who launched the projectile, if any
		idStr str = spawnArgs.GetString("enemy", "");
		idEntity *e = gameLocal.FindEntity(str);
		if ( e ) {
			enemy = e;
		}
		else if ( owner.GetEntity() && owner.GetEntity()->IsType( idAI::Type ) ) {
			enemy = static_cast<idAI *>( owner.GetEntity() )->GetEnemy();
		}
		else if ( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) ) {
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

	const idVec3 &vel = physicsObj.GetLinearVelocity();
	angles = vel.ToAngles();
	speed = vel.Length();
	rndScale = spawnArgs.GetAngles( "random", "15 15 0" );
	turn_max = spawnArgs.GetFloat( "turn_max", "180" ) / ( float )USERCMD_HZ;
	clamp_dist = spawnArgs.GetFloat( "clamp_dist", "256" );
	burstMode = spawnArgs.GetBool( "burstMode" );
	burstDist = spawnArgs.GetFloat( "burstDist", "64" );
	burstVelocity = spawnArgs.GetFloat( "burstVelocity", "1.25" );
	unGuided = !( enemy.GetEntity() );
	UpdateVisuals();
}

/*
================
idGuidedProjectile::Event_Launch
================
*/
void idGuidedProjectile::Event_Launch( idVec3 const &origin, idVec3 const &direction, idVec3 const &velocity, idEntity *target ) {
	if( target )
		enemy = target;

	Launch( origin, direction, velocity );
}

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
	sndBounce = NULL;
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
	idAngles	angular_velocity;
	float		linear_friction;
	float		angular_friction;
	float		contact_friction;
	float		bounce;
	float		mass;
	float		gravity;
	idVec3		gravVec;
	bool		randomVelocity;
	idMat3		axis;

	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	spawnArgs.GetVector( "velocity", "0 0 0", velocity );
	spawnArgs.GetAngles( "angular_velocity", "0 0 0", angular_velocity );
	
	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	angular_friction	= spawnArgs.GetFloat( "angular_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );
	fuse				= spawnArgs.GetFloat( "fuse" );
	randomVelocity		= spawnArgs.GetBool ( "random_velocity" );

	if ( mass <= 0 ) {
		gameLocal.Error( "Invalid mass on '%s'\n", GetEntityDefName() );
	}

	if ( randomVelocity ) {
		velocity.x *= gameLocal.random.RandomFloat() + 0.5f;
		velocity.y *= gameLocal.random.RandomFloat() + 0.5f;
		velocity.z *= gameLocal.random.RandomFloat() + 0.5f;
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
	physicsObj.SetLinearVelocity( axis[ 0 ] * velocity[ 0 ] + axis[ 1 ] * velocity[ 1 ] + axis[ 2 ] * velocity[ 2 ] );
	physicsObj.SetAngularVelocity( angular_velocity.ToAngularVelocity() * axis );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( axis );
	SetPhysics( &physicsObj );

	{
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
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}

	const char *sndName = spawnArgs.GetString( "snd_bounce" );
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

	if ( smokeFly && smokeFlyTime ) {
		if ( !gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() ) ) {
			smokeFlyTime = 0;
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
	if ( sndBounce != NULL ) {
		StartSoundShader( sndBounce, SND_CHANNEL_BODY, 0, false, NULL );
	}
	sndBounce = NULL;
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
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	Hide();

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
		gameLocal.smokeParticles->EmitSmoke( smokeFly, smokeFlyTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

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


