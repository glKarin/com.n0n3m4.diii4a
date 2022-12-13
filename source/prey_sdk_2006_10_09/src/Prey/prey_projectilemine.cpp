#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhHarvesterMine
	
***********************************************************************/
const idEventDef EV_ApplyAttractionTowards( "applyAttractionTowards", "e" );

CLASS_DECLARATION( hhRenderEntity, hhHarvesterMine )
	EVENT( EV_Activate,					hhHarvesterMine::Event_Detonate )
	EVENT( EV_ApplyAttractionTowards,	hhHarvesterMine::Event_ApplyAttractionTowards )
	EVENT( EV_Explode,					hhHarvesterMine::Event_Explode )
END_CLASS

/*
================
hhHarvesterMine::hhHarvesterMine
================
*/
hhHarvesterMine::hhHarvesterMine() {
	proximityDetonateTrigger = NULL;
	proximityAttractionTrigger = NULL;
}

/*
================
hhHarvesterMine::~hhHarvesterMine
================
*/
hhHarvesterMine::~hhHarvesterMine() {
	SAFE_REMOVE( proximityDetonateTrigger );
	SAFE_REMOVE( proximityAttractionTrigger );
}

/*
================
hhHarvesterMine::Spawn
================
*/
void hhHarvesterMine::Spawn() {
	SpawnTriggers();
}

/*
================
hhHarvesterMine::InitPhysics
================
*/
void hhHarvesterMine::InitPhysics( const idVec3& start, const idMat3& axis, const idVec3& pushVelocity ) {
	float		speed;
	float		linear_friction;
	float		contact_friction;
	float		bounce;
	float		mass;
	float		gravity;
	idVec3		gravVec;

	speed = spawnArgs.GetVector( "velocity", "0 0 0" ).Length();

	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );

	if ( mass <= 0 ) {
		gameLocal.Error( "Invalid mass on '%s'\n", GetClassname() );
	}

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.GetClipModel()->SetOwner( DetermineClipModelOwner() );
	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linear_friction, 1.0f, contact_friction );
	
	physicsObj.SetBouncyness( bounce );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetContents( DetermineContents() );

	physicsObj.SetClipMask( MASK_SHOT_RENDERMODEL );
	physicsObj.SetLinearVelocity( axis[ 0 ] * speed + pushVelocity );

	physicsObj.SetOrigin( start );
	physicsObj.SetAxis( axis );
	SetPhysics( &physicsObj );
}

/*
================
hhHarvesterMine::SpawnTriggers
================
*/
void hhHarvesterMine::SpawnTriggers() {
	idDict dict;

	dict.SetVector( "origin", GetOrigin() );
	dict.SetMatrix( "rotation", GetAxis() );
	dict.Set( "target", name.c_str() );
	dict.SetInt( "triggerBehavior", TB_FRIENDLIES_ONLY );

	dict.SetVector( "mins", spawnArgs.GetVector("detonationMins", "-10 -10 -10") );
	dict.SetVector( "maxs", spawnArgs.GetVector("detonationMaxs", "10 10 10") );
	proximityDetonateTrigger = gameLocal.SpawnObject( spawnArgs.GetString("def_detonateTrigger"), &dict );
	proximityDetonateTrigger->Bind( this, true );

	dict.SetVector( "mins", spawnArgs.GetVector("attractionMins", "-20 -20 -20") );
	dict.SetVector( "maxs", spawnArgs.GetVector("attractionMaxs", "20 20 20") );
	dict.SetFloat( "refire", spawnArgs.GetFloat("attractionUpdateFrequency") );
	dict.Set( "eventDef", "applyAttractionTowards" );
	proximityAttractionTrigger = gameLocal.SpawnObject( spawnArgs.GetString("def_attractionTrigger"), &dict );
	proximityAttractionTrigger->Bind( this, true );
}

/*
================
hhHarvesterMine::Create
================
*/
void hhHarvesterMine::Create( idEntity *owner, const idVec3 &start, const idMat3 &axis ) {
	idStr		shaderName;
	idVec3		light_color;
	idVec3		light_offset;
	
	Unbind();

	SetOrigin( start );
	SetAxis( axis );

	this->owner = owner;

	SIMDProcessor->Memset( &renderLight, 0, sizeof( renderLight ) );
	shaderName = spawnArgs.GetString( "mtr_light_shader" );
	if ( *shaderName ) {
		renderLight.shader = declManager->FindMaterial( shaderName, false );
		renderLight.pointLight = true;
		renderLight.lightRadius = spawnArgs.GetVector( "light_size" );
		spawnArgs.GetVector( "light_color", "1 1 1", light_color );
		renderLight.shaderParms[0] = light_color[0];
		renderLight.shaderParms[1] = light_color[1];
		renderLight.shaderParms[2] = light_color[2];
		renderLight.shaderParms[3] = 1.0f;
	}

	spawnArgs.GetVector( "light_offset", "0 0 0", lightOffset );

	GetPhysics()->SetContents( 0 );

	state = CREATED;
}

/*
=================
hhHarvesterMine::Launch
=================
*/
void hhHarvesterMine::Launch( const idVec3 &start, const idMat3& axis, const idVec3 &pushVelocity ) {
	int		anim = 0;

	if ( health ) {
		fl.takedamage = true;
	}

	Unbind();

	//HUMANHEAD: aob - moved logic to helper function
	InitPhysics( start, axis, pushVelocity );
	//HUMANHEAD END

	if ( !gameLocal.isClient ) {
		PostEventSec( &EV_Explode, hhMath::hhMax(0.0f, spawnArgs.GetFloat("fuse")) );
	}

	StartSound( "snd_fly", SND_CHANNEL_BODY, 0, true, NULL );

	state = LAUNCHED;
}

/*
================
hhHarvesterMine::Killed
================
*/
void hhHarvesterMine::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	//HUMANHEAD: aob - added collision so we can get explosion to show up when killed
	trace_t collision;

	collision.fraction = 0.0f;
	collision.endpos = GetOrigin();
	collision.endAxis = GetAxis();
	collision.c.entityNum = attacker->entityNumber;
	collision.c.normal = inflictor->GetOrigin() - GetOrigin();
	collision.c.normal.Normalize();
	Explode( &collision );
}

/*
================
hhHarvesterMine::Collide
================
*/
bool hhHarvesterMine::Collide( const trace_t& collision, const idVec3& velocity ) {
	return false;//Never stop because of collision, just bounce
}

/*
================
hhHarvesterMine::DetermineContents
================
*/
int hhHarvesterMine::DetermineContents() {
	return CONTENTS_SOLID;
}

/*
================
hhHarvesterMine::DetermineClipModelOwner
================
*/
idEntity* hhHarvesterMine::DetermineClipModelOwner() {
	return (spawnArgs.GetBool("collideWithOwner")) ? this : owner.GetEntity();
}

/*
=================
hhHarvesterMine::RemoveProjectile
=================
*/
void hhHarvesterMine::RemoveProjectile( const int removeDelay ) {
	Hide();
	RemoveBinds();//Remove any fx we have because they aren't hidden
	PostEventMS( &EV_Remove, removeDelay );
}

/*
================
hhHarvesterMine::Explode
================
*/
void hhHarvesterMine::Explode( const trace_t *collision ) {
	const char *light_shader;
	float		light_fadetime;
	int			removeDelay;
	trace_t		collisionInfo;

	if ( state == EXPLODED || state == FIZZLED ) {
		return;
	}

	//HUMANHEAD: aob
	if( collision && collision->fraction < 1.0f ) {
		memcpy( &collisionInfo, collision, sizeof(trace_t) );
	} else {
		collisionInfo.fraction = 0.0f;
		collisionInfo.endpos = GetOrigin();
		collisionInfo.endAxis = GetAxis();
		collisionInfo.c.entityNum = gameLocal.world->entityNumber;
		collisionInfo.c.normal = idVec3(0.0f,0.0f,1.0f);
	}

	removeDelay = spawnArgs.GetFloat( "remove_time", "200" );
	//HUMANHEAD END

	// play sound
	//HUMANHEAD: aob - in case the sound length is longer than removeTime
	int length = 0;
	StartSound( "snd_explode", SND_CHANNEL_BODY, 0, true, &length );
	removeDelay = hhMath::hhMax( length, removeDelay );
	//HUMANHEAD END

	FreeLightDef();

	// explosion light
	light_shader = spawnArgs.GetString( "mtr_explode_light_shader" );
	if ( *light_shader ) {
		renderLight.shader = declManager->FindMaterial( light_shader, false );
		renderLight.pointLight = true;
		renderLight.lightRadius = spawnArgs.GetVector( "explode_light_size" );
		spawnArgs.GetVector( "explode_light_color", "1 1 1", lightColor );
		renderLight.shaderParms[SHADERPARM_RED] = lightColor[0];
		renderLight.shaderParms[SHADERPARM_GREEN] = lightColor[1];
		renderLight.shaderParms[SHADERPARM_BLUE] = lightColor[2];
		renderLight.shaderParms[SHADERPARM_ALPHA] = 1.0f;
		renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
		light_fadetime = spawnArgs.GetFloat( "explode_light_fadetime" );
		lightStartTime = gameLocal.time;
		lightEndTime = gameLocal.time + SEC2MS( light_fadetime );
		BecomeActive( TH_THINK );
	}

	if( !gameLocal.isClient ) {
		SpawnCollisionFX( &collisionInfo, "fx_detonate" );
		SpawnDebris( collisionInfo.c.normal, physicsObj.GetLinearVelocity().ToMat3()[0] );
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	state = EXPLODED;

	if ( gameLocal.isClient ) {
		return;
	}

	// splash damage
	idStr splash_damage = spawnArgs.GetString( "def_splash_damage" );
	if ( splash_damage.Length() ) {
		gameLocal.RadiusDamage( collisionInfo.endpos, this, owner.GetEntity(), this, this, splash_damage );
	}

	//HUMANHEAD: aob - moved logic to helper function
	RemoveProjectile( removeDelay );
	//HUMANHEAD END
}

/*
================
hhHarvesterMine::SpawnCollisionFX
================
*/
void hhHarvesterMine::SpawnCollisionFX( const trace_t* collision, const char* fxKey ) {
	hhFxInfo	fxInfo;

	if( !collision || collision->fraction >= 1.0f ) {
		return;
	}

	fxInfo.SetNormal( collision->c.normal );
	fxInfo.RemoveWhenDone( true );

	BroadcastFxInfoPrefixedRandom( fxKey, GetOrigin(), GetAxis(), &fxInfo ); 
}

/*
================
hhHarvesterMine::SpawnDebris
================
*/
void hhHarvesterMine::SpawnDebris( const idVec3& collisionNormal, const idVec3& collisionDir ) {
	int fxdebris = spawnArgs.GetInt( "debris_count" );
	if( !fxdebris ) {
		return;
	}

	idDebris *debris = NULL;
	idEntity *ent = NULL;
	int amount = 0;
	const idDict *dict = NULL;
	for( const idKeyValue* kv = spawnArgs.MatchPrefix("def_debris", NULL); kv; kv = spawnArgs.MatchPrefix("def_debris", kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}
		
		dict = gameLocal.FindEntityDefDict( kv->GetValue().c_str(), false );
		if( !dict ) {
			continue;
		}

		amount = gameLocal.random.RandomInt( fxdebris );
		for ( int i = 0; i < amount; i++ ) {
			//HUMANHEAD: aob
			idVec3 dir = hhUtils::RandomSpreadDir( collisionNormal.ToMat3(), DEG2RAD(spawnArgs.GetFloat("spread_debris", "10")) );
			//HUMAMHEAD END

			gameLocal.SpawnEntityDef( *dict, &ent );
			if ( !ent || !ent->IsType( idDebris::Type ) ) {
				gameLocal.Error( "hhProjectile: 'projectile_debris' is not an idDebris" );
			}

			debris = static_cast<idDebris *>(ent);
			debris->Create( owner.GetEntity(), GetOrigin(), dir.ToMat3() );
			debris->Launch();
		}
	}
}

/*
================
hhHarvesterMine::SetGravity
================
*/
void hhHarvesterMine::SetGravity( const idVec3 &newGravity ) {
	float relativeMagnitude = spawnArgs.GetFloat( "gravity" );
	idVec3 newGravityVector( vec3_zero );

	if( GetGravity().Compare(newGravity, VECTOR_EPSILON) ) {
		return;
	}

	if( relativeMagnitude > 0.0f ) {
		newGravityVector = newGravity;
		relativeMagnitude *= newGravityVector.Normalize() / gameLocal.GetGravity().Length();
		newGravityVector *= relativeMagnitude;
	}
	
	GetPhysics()->SetGravity( newGravityVector );
}

/*
================
hhHarvesterMine::Event_Explode
================
*/
void hhHarvesterMine::Event_Explode( void ) {
	trace_t collision;

	SIMDProcessor->Memset( &collision, 0, sizeof(trace_t) );
	collision.endpos = GetOrigin();
	collision.endAxis = GetAxis();
	collision.c.entityNum = ENTITYNUM_WORLD;
	collision.c.normal = idVec3(0.0f, 0.0f, 1.0f);
	Explode( &collision );
}

/*
================
hhHarvesterMine::Event_Detonate
================
*/
void hhHarvesterMine::Event_Detonate( idEntity *activator ) {
	trace_t collision;

	//Monsters and other harvesters are culled out by the trigger behavior
	if( owner != activator ) {
		SIMDProcessor->Memset( &collision, 0, sizeof(trace_t) );
		collision.endpos = GetOrigin();
		collision.endAxis = GetAxis();
		if(!activator) {
			collision.c.entityNum	= ENTITYNUM_WORLD;
			collision.c.normal		= idVec3(0.0f, 0.0f, 1.0f);
		} else {
			collision.c.entityNum	= activator->entityNumber;
			collision.c.normal		= GetOrigin() - activator->GetOrigin();
			collision.c.normal.Normalize();
		}		
		
		Explode( &collision );
	}
}

/*
================
hhHarvesterMine::Event_ApplyAttractionTowards
================
*/
void hhHarvesterMine::Event_ApplyAttractionTowards( idEntity *activator ) {
	if( !activator || owner == activator ) {
		return;
	}

	idVec3 dirToTarget = ((activator->IsType(idActor::Type)) ? static_cast<idActor*>(activator)->GetEyePosition() : activator->GetOrigin()) - GetOrigin();
	dirToTarget.Normalize();

	ApplyImpulse( this, 0, GetOrigin(), dirToTarget * spawnArgs.GetFloat("attractionMagnitude") );
}

/*
================
hhHarvesterMine::Save
================
*/
void hhHarvesterMine::Save( idSaveGame *savefile ) const {
	proximityDetonateTrigger.Save( savefile );
	proximityAttractionTrigger.Save( savefile );

	savefile->WriteStaticObject( physicsObj );

	owner.Save( savefile );

	savefile->WriteRenderLight( renderLight );
	savefile->WriteVec3( lightOffset );
	savefile->WriteInt( lightStartTime );
	savefile->WriteInt( lightEndTime );
	savefile->WriteVec3( lightColor );
	savefile->WriteInt( state );
}

/*
================
hhHarvesterMine::Restore
================
*/
void hhHarvesterMine::Restore( idRestoreGame *savefile ) {
	proximityDetonateTrigger.Restore( savefile );
	proximityAttractionTrigger.Restore( savefile );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	owner.Restore( savefile );

	savefile->ReadRenderLight( renderLight );
	savefile->ReadVec3( lightOffset );
	savefile->ReadInt( lightStartTime );
	savefile->ReadInt( lightEndTime );
	savefile->ReadVec3( lightColor );
	savefile->ReadInt( reinterpret_cast<int &> ( state ) );
}
