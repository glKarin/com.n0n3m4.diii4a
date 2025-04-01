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

#include "precompiled.h"
#pragma hdrstop



#include "StimResponse/StimResponseCollection.h"

#ifdef MOD_WATERPHYSICS

// We do these splashes if the mass of the colliding object is less than these values.
// Anything large than MEDIUM_SPLASH does a large splash. (get it?)
const int SMALL_SPLASH		= 5;
const int MEDIUM_SPLASH		= 20;

/*
===============================================================================

	idLiquid

===============================================================================
*/

CLASS_DECLARATION( idEntity, idLiquid )
	EVENT( EV_Touch,			idLiquid::Event_Touch )
END_CLASS

idLiquid::idLiquid( void ) {
}

/*
================
idLiquid::Save
================
*/
void idLiquid::Save( idSaveGame *savefile ) const {

	int i;

   	savefile->WriteStaticObject( this->physicsObj );
	
	savefile->WriteString(smokeName.c_str());
	savefile->WriteString(soundName.c_str());

	for( i = 0; i < 3; i++ )
		savefile->WriteParticle(this->splash[i]);
	savefile->WriteParticle(this->waves);
}

/*
================
idLiquid::Restore
================
*/
void idLiquid::Restore( idRestoreGame *savefile ) {
	
	int i;

	savefile->ReadStaticObject( this->physicsObj );
	RestorePhysics( &this->physicsObj );

	savefile->ReadString(this->smokeName);
	savefile->ReadString(this->soundName);

	for( i = 0; i < 3; i++ )
		savefile->ReadParticle(this->splash[i]);
	savefile->ReadParticle(this->waves);
}

idLiquid::~idLiquid()
{
	// Traverse all spawned entities and remove ourselves as water if necessary
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (ent->GetPhysics() != NULL && ent->GetPhysics()->GetWater() == &physicsObj)
		{
			ent->GetPhysics()->SetWater(NULL, 0.0f);
		}
	}
}

/*
================
idLiquid::Spawn
================
*/
void idLiquid::Spawn() {
/*
	model = dynamic_cast<idRenderModelLiquid *>( renderEntity.hModel );
	if ( !model ) {
		gameLocal.Error( "Entity '%s' must have liquid model", name.c_str() );
	}
	model->Reset();
*/
	float liquidDensity;
	float liquidViscosity;
	float liquidFriction;
	idVec3 minSplash;
	idVec3 minWave;
	idStr temp;
	const char *splashName;

	//common->Printf("idLiquid:%s) Spawned\n",this->GetName() );

	// getters
	spawnArgs.GetFloat("density","0.001043f",liquidDensity);
	spawnArgs.GetFloat("viscosity","3.0f",liquidViscosity);
	spawnArgs.GetFloat("friction","3.0f",liquidFriction);
	spawnArgs.GetString("liquid_name","water",temp);
	spawnArgs.GetVector("minSplashVelocity","100 100 100",minSplash);
	spawnArgs.GetVector("minWaveVelocity","60 60 60",minWave);

	model = nullptr;

	// setters
	this->smokeName = "smoke_";
	this->smokeName.Append(temp);

	this->soundName = "snd_";
	this->soundName.Append(temp);

	splashName = spawnArgs.GetString("smoke_small","water_splash_small");
	this->splash[0] = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	splashName = spawnArgs.GetString("smoke_medium","water_splash");
	this->splash[1] = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	splashName = spawnArgs.GetString("smoke_large","water_splash_large");
	this->splash[2] = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	splashName = spawnArgs.GetString("smoke_waves","water_waves");
	this->waves = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE,splashName));

	// setup physics
	this->physicsObj.SetSelf(this);
	this->physicsObj.SetClipModel( new idClipModel(this->GetPhysics()->GetClipModel()), liquidDensity );
	this->physicsObj.SetOrigin(this->GetPhysics()->GetOrigin());
	this->physicsObj.SetAxis(this->GetPhysics()->GetAxis());	
	this->physicsObj.SetGravity( gameLocal.GetGravity() );
	this->physicsObj.SetContents( CONTENTS_WATER | CONTENTS_TRIGGER );
	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );

	this->physicsObj.SetDensity(liquidDensity);
	this->physicsObj.SetViscosity(liquidViscosity);
	this->physicsObj.SetMinSplashVelocity(minSplash);
	this->physicsObj.SetMinWaveVelocity(minWave);

	this->SetPhysics( &this->physicsObj );

	renderEntity.shaderParms[ 3 ]	= spawnArgs.GetFloat( "shaderParm3", "1" );
	renderEntity.shaderParms[ 4 ]	= spawnArgs.GetFloat( "shaderParm4", "0" );
	renderEntity.shaderParms[ 5 ]	= spawnArgs.GetFloat( "shaderParm5", "0.1" );
	renderEntity.shaderParms[ 6 ]	= spawnArgs.GetFloat( "shaderParm6", "1.5" );
	renderEntity.shaderParms[ 7 ]	= spawnArgs.GetFloat( "shaderParm7", "0" );
	renderEntity.shaderParms[ 8 ]	= spawnArgs.GetFloat( "shaderParm8", "0" );
	renderEntity.shaderParms[ 9 ]	= spawnArgs.GetFloat( "shaderParm9", "0" );
	renderEntity.shaderParms[ 10 ]	= spawnArgs.GetFloat( "shaderParm10", "0" );
	renderEntity.shaderParms[ 11 ]	= spawnArgs.GetFloat( "shaderParm11", "0" );

	BecomeActive( TH_THINK );
}

/*
================
idLiquid::Event_Touch

	This is mainly used for actors who touch the liquid, it spawns a splash
	near their feet if they're moving fast enough.
================
*/
void idLiquid::Event_Touch( idEntity *other, trace_t *trace )
{
	idPhysics_Liquid *liquid;
	idPhysics_Actor *phys;

	if ( !other->GetPhysics()->IsType(idPhysics_Actor::Type) )
	{
		return;
	}

	phys = static_cast<idPhysics_Actor *>(other->GetPhysics());
	if ( phys->GetWaterLevel() != WATERLEVEL_FEET )
	{
		return;
	}

	impactInfo_t info;
	other->GetImpactInfo(this,trace->c.id,trace->c.point,&info);
	liquid = &this->physicsObj;

	trace->c.point = info.position + other->GetPhysics()->GetOrigin();
	trace->c.entityNum = other->entityNumber;

	// stop actors from constantly splashing when they're in the water
	// (this is such a bad thing to do!!!)
	// TODO: Fixme...
	//		1) Probably the best way to fix this is put a wait timer inside the actor and have this
	//		function set/reset that timer for when the actor should spawn particles at its feet.
	//		2) Actors don't spawn particles at their feet, it's usually at the origin, for some
	//		reason info.position is (null), needs a fix so that splash position is correct
	if(	gameLocal.random.RandomFloat() > 0.5f )
	{
		return;
	}

	this->Collide(*trace,info.velocity);
}

/*
================
idLiquid::Collide
	Spawns a splash particle and attaches a sound to the colliding entity.
================
*/
bool idLiquid::Collide( const trace_t &collision, const idVec3 &velocity )
{
	idEntity *e = gameLocal.entities[collision.c.entityNum];
	idPhysics_Liquid *phys = static_cast<idPhysics_Liquid *>( this->GetPhysics() );
	const idDeclParticle *splash;
	const char *sName;
	float eMass;
	idVec3 splashSpot;
	float velSquare = velocity.LengthSqr();

	ProcCollisionStims( e, collision.c.id );

	splashSpot = collision.c.point;
		
	if ( velSquare > phys->GetMinSplashVelocity().LengthSqr() )
	{
		// grayman #4600 - The entity might be entering the liquid more than once
		// as it breaks the plane of the liquid. Wait a bit before splashing again.
		if ( gameLocal.time >= e->m_splashtime )
		{
			e->m_splashtime = gameLocal.time + 100;
			eMass = e->GetPhysics()->GetMass();

			// pick which splash particle to spawn
			// first we check the entity, if it's not defined we use
			// one defined for this liquid.
			sName = e->spawnArgs.GetString(this->smokeName.c_str());
			if ( *sName != '\0' )
			{
				// load entity particle
				splash = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, sName));
			}
			else
			{
				// load a liquid particle based on the mass of the splashing entity
				if ( eMass < SMALL_SPLASH )
				{
					splash = this->splash[0];
				}
				else if ( eMass < MEDIUM_SPLASH )
				{
					splash = this->splash[1];
				}
				else
				{
					splash = this->splash[2];
				}
			}

			// play the sound for a splash
			e->StartSound(this->soundName.c_str(), SND_CHANNEL_ANY, 0, false, NULL);

			// grayman #3413 - propagate the global sound for the splash

			idStr size = e->spawnArgs.GetString("spr_object_size");
			if ( size.IsEmpty() )
			{
				if ( eMass < SMALL_SPLASH )
				{
					size = "small";
				}
				else if ( eMass < MEDIUM_SPLASH )
				{
					size = "medium";
				}
				else
				{
					size = "large";
				}
			}
			idStr splashName = idStr("splash_") + size;
			e->PropSoundS(NULL, splashName, 0, 0);
		}
		else // grayman #4600
		{
			return true;
		}
	}
	else if ( velSquare > phys->GetMinWaveVelocity().LengthSqr() )
	{
		splash = this->waves;
	}
	else
	{
		// the object is moving to slow so we abort
		return true;
	}

	// spawn the particle
	gameLocal.smokeParticles->EmitSmoke( splash, gameLocal.time, gameLocal.random.RandomFloat(), splashSpot, collision.endAxis );
	return true;
}

#endif
