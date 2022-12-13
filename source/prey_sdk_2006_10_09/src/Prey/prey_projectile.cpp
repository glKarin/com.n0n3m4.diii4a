#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_SpawnDriverLocal( "<spawnDriverLocal>", "s" );
const idEventDef EV_SpawnFxFlyLocal( "<spawnFxFlyLocal>", "s" );

const idEventDef EV_Collision_Flesh( "<collision_flesh>", "tv", 'd' );
const idEventDef EV_Collision_Metal( "<collision_metal>", "tv", 'd' );
const idEventDef EV_Collision_AltMetal( "<collision_altmetal>", "tv", 'd' );
const idEventDef EV_Collision_Wood( "<collision_wood>", "tv", 'd' );
const idEventDef EV_Collision_Stone( "<collision_stone>", "tv", 'd' );
const idEventDef EV_Collision_Glass( "<collision_glass>", "tv", 'd' );
const idEventDef EV_Collision_Liquid( "<collision_liquid>", "tv", 'd' );
const idEventDef EV_Collision_Spirit( "<collision_spirit>", "tv", 'd' );
const idEventDef EV_Collision_Remove( "<collision_remove>", "tv", 'd' );
const idEventDef EV_Collision_CardBoard( "<collision_cardboard>", "tv", 'd' );
const idEventDef EV_Collision_Tile( "<collision_tile>", "tv", 'd' );
const idEventDef EV_Collision_Forcefield( "<collision_forcefield>", "tv", 'd' );
const idEventDef EV_Collision_Chaff( "<collision_chaff>", "tv", 'd' );
const idEventDef EV_Collision_Wallwalk( "<collision_wallwalk>", "tv", 'd' );
const idEventDef EV_Collision_Pipe( "<collision_pipe>", "tv", 'd' );


const idEventDef EV_AllowCollision_Flesh( "<collision_allow_flesh>", "t", 'd' );
const idEventDef EV_AllowCollision_Metal( "<collision_allow_metal>", "t", 'd' );
const idEventDef EV_AllowCollision_AltMetal( "<collision_allow_altmetal>", "t", 'd' );
const idEventDef EV_AllowCollision_Wood( "<collision_allow_wood>", "t", 'd' );
const idEventDef EV_AllowCollision_Stone( "<collision_allow_stone>", "t", 'd' );
const idEventDef EV_AllowCollision_Glass( "<collision_allow_glass>", "t", 'd' );
const idEventDef EV_AllowCollision_Liquid( "<collision_allow_liquid>", "t", 'd' );
const idEventDef EV_AllowCollision_Spirit( "<collision_allow_spirit>", "t", 'd' );
const idEventDef EV_AllowCollision_CardBoard( "<collision_allow_cardboard>", "t", 'd' );
const idEventDef EV_AllowCollision_Tile( "<collision_allow_tile>", "t", 'd' );
const idEventDef EV_AllowCollision_Forcefield( "<collision_allow_forcefield>", "t", 'd' );
const idEventDef EV_AllowCollision_Chaff( "<collision_allow_chaff>", "t", 'd' );
const idEventDef EV_AllowCollision_Wallwalk( "<collision_allow_wallwalk>", "t", 'd' );
const idEventDef EV_AllowCollision_Pipe( "<collision_allow_pipe>", "t", 'd' );

hhMatterEventDefPartner matterEventsCollision( "collision" );
hhMatterEventDefPartner matterEventsAllowCollision( "collision_allow" );

CLASS_DECLARATION( idProjectile, hhProjectile )
	EVENT( EV_Explode,						hhProjectile::Event_Fuse_Explode )
	EVENT( EV_SpawnDriverLocal,				hhProjectile::Event_SpawnDriverLocal )
	EVENT( EV_SpawnFxFlyLocal,				hhProjectile::Event_SpawnFxFlyLocal )

	EVENT( EV_Collision_Flesh,				hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Metal,				hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_AltMetal,			hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Wood,				hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Stone,				hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Glass,				hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Liquid,				hhProjectile::Event_Collision_DisturbLiquid )
	EVENT( EV_Collision_CardBoard,			hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Tile,				hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Forcefield,			hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Pipe,				hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Wallwalk,			hhProjectile::Event_Collision_Impact )
	EVENT( EV_Collision_Chaff,				hhProjectile::Event_Collision_Impact )

	EVENT( EV_Collision_Remove,				hhProjectile::Event_Collision_Remove )

	EVENT( EV_AllowCollision_Flesh,			hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Metal,			hhProjectile::Event_AllowCollision_CollideNoProj )
	EVENT( EV_AllowCollision_AltMetal,		hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Wood,			hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Stone,			hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Glass,			hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Liquid,		hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_CardBoard,		hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Tile,			hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Forcefield,	hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Pipe,			hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Wallwalk,		hhProjectile::Event_AllowCollision_Collide )
	EVENT( EV_AllowCollision_Spirit,		hhProjectile::Event_AllowCollision_PassThru )
	EVENT( EV_AllowCollision_Chaff,			hhProjectile::Event_AllowCollision_Collide )	//bjk: shield blocks all
END_CLASS


/*
================
hhProjectile::Spawn
================
*/
void hhProjectile::Spawn() {
	bDDACounted = false;
	parentProjectile = NULL;
	launchTimestamp = 0;

	collidedPortal = NULL; // cjr
	collideLocation = vec3_origin; // cjr
	collideVelocity = vec3_origin; // cjr

	weaponNum = -1; // cjr - default the weapon index to -1, to denote non-player weapons

	netSyncPhysics = spawnArgs.GetBool( "net_fullphysics" ); //HUMANHEAD rww
	bNoCollideWithCrawlers = spawnArgs.GetBool( "noCollideWithCrawlers", "0" );
	bProjCollide = spawnArgs.GetBool( "proj_collision", "0" );
	flyBySoundDistSq = spawnArgs.GetFloat( "flyby_dist", "0" );
	flyBySoundDistSq *= flyBySoundDistSq;
	if ( flyBySoundDistSq > 0 ) {
		bPlayFlyBySound = true;
	}
	BecomeActive( TH_TICKER );
}

/*
================
hhProjectile::~hhProjectile
================
*/
hhProjectile::~hhProjectile() {
	SAFE_REMOVE(fxFly); //HUMANHEAD rww
}

void hhProjectile::SetParentProjectile( hhProjectile* in_parent ) {
	parentProjectile.Assign( in_parent );
}

void hhProjectile::SetCollidedPortal( hhPortal *newPortal, idVec3 newLocation, idVec3 newVelocity ) {
	collidedPortal.Assign( newPortal );
	collideLocation = newLocation;
	collideVelocity = newVelocity;
	BecomeActive(TH_MISC1);

	physicsObj.PutToRest();
}

hhProjectile* hhProjectile::GetParentProjectile( void ) {
	if( parentProjectile.IsValid() ) {
		return parentProjectile.GetEntity();
	}
	return NULL;
}


/*
================
hhProjectile::SetOrigin
================
*/
void hhProjectile::SetOrigin( const idVec3& origin ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;
	idVec3 localOrigin( origin );

	if( driver.IsValid() && IsBoundTo(driver.GetEntity()) ) {
		GetMasterPosition( masterOrigin, masterAxis );
		localOrigin = (localOrigin - masterOrigin) * masterAxis.Transpose();
	}

	idProjectile::SetOrigin( localOrigin );
}

/*
================
hhProjectile::SetAxis
================
*/
void hhProjectile::SetAxis( const idMat3& axis ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;
	idMat3 localAxis( axis );

	if( driver.IsValid() && IsBoundTo(driver.GetEntity()) ) {
		GetMasterPosition( masterOrigin, masterAxis );
		localAxis *= masterAxis.Transpose();
	}

	idProjectile::SetAxis( localAxis );
}

/*
================
hhProjectile::Think
================
*/
void hhProjectile::Think( void ) {

	// HUMANHEAD: cjr - if this projectile recently struck a portal, then attempt to portal it
	if ( (thinkFlags & TH_MISC1) && collidedPortal.IsValid() ) {
		GetPhysics()->SetLinearVelocity( collideVelocity );
		collidedPortal->PortalProjectile( this, collideLocation, collideLocation + collideVelocity );
		collidedPortal = NULL;
		collideLocation = vec3_origin;
		collideVelocity = vec3_origin;
		BecomeInactive(TH_MISC1);
	}
	// HUMANHEAD END

	// run physics
	RunPhysics();

	if( thinkFlags & TH_THINK ) {
		//HUMANHEAD: aob - added thrust_start check
		if( thrust && (thrust_start <= gameLocal.GetTime() && gameLocal.GetTime() < thrust_end) ) {
			// evaluate force
			//HUMANHEAD rww - get rid of the thruster, needless projectile constructor overhead.
			//thruster.SetForce( GetAxis()[ 0 ] * thrust );
			//thruster.Evaluate( gameLocal.GetTime() );
			//replaced the logic for the thing here.
			idVec3 force = GetAxis()[ 0 ] * thrust;
			idVec3 point = physicsObj.GetCenterOfMass();
			idVec3 p = GetPhysics()->GetOrigin() + point * physics->GetAxis();
			GetPhysics()->AddForce( 0, p, force );
		}
	}

	//HUMANHEAD: aob
	if( thinkFlags & TH_TICKER ) {
		Ticker();
	}
	//HUMANHEAD END

	Present();

	if ( thinkFlags & TH_MISC2 ) {
		UpdateLight();
	}

	//HUMANHEAD jsh flyby sounds
	if( !gameLocal.isMultiplayer && bPlayFlyBySound && gameLocal.GetLocalPlayer() ) {
		if ( !owner.IsEqualTo( gameLocal.GetLocalPlayer() ) ) {
			if ( (GetOrigin() - gameLocal.GetLocalPlayer()->GetOrigin()).LengthSqr() < flyBySoundDistSq ) {
				BroadcastFxPrefixedRandom( "fx_flyby", EV_SpawnFxFlyLocal );
				StartSound( "snd_flyby", SND_CHANNEL_BODY );
				bPlayFlyBySound = false;
			}
		}
	}
}

/*
=====================
hhProjectile::PlayImpactSound

custom for projectiles. this needs to broadcast, unless we get predicted projectiles operational.
however predicted projectiles are not reliable since the client will not always collide them before
the server does, and so they will get removed with no fx. not a serious issue but somewhat bothersome
nonetheless.
=====================
*/
int hhProjectile::PlayImpactSound( const idDict* dict, const idVec3 &origin, surfTypes_t type ) {
	const char *snd = gameLocal.MatterTypeToMatterKey( "snd_impact", type );
	if( !snd || !snd[0] || !dict ) {
		return -1;
	}
		
	int length = 0;
	StartSoundShader( declManager->FindSound(dict->GetString(snd), false), SND_CHANNEL_BODY, 0, true, &length );
	return length;
}

/*
==============
hhProjectile::UpdateLight
==============
*/
void hhProjectile::UpdateLight() {
	// Attempt to remove light
	if( lightStartTime != lightEndTime && gameLocal.GetTime() >= lightEndTime ) {
		FreeLightDef();
		BecomeInactive(TH_MISC2);
	}

	if( lightDefHandle != -1 ) {
		UpdateLightPosition();
		UpdateLightFade();
		gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
	}
}

/*
==============
hhProjectile::UpdateLightPosition
==============
*/
void hhProjectile::UpdateLightPosition() {
	renderLight.origin = GetOrigin() + GetAxis() * lightOffset;
	renderLight.axis = GetAxis();
}

/*
==============
hhProjectile::UpdateLightFade
==============
*/
void hhProjectile::UpdateLightFade() {
	idVec3 color( vec3_zero );
	float frac = 0.0f;

	int time = gameLocal.GetTime();
	if( lightStartTime != lightEndTime && time < lightEndTime ) {
		frac = MS2SEC(gameLocal.GetTime() - lightStartTime) / MS2SEC(lightEndTime - lightStartTime);
		color.Lerp( lightColor, color, frac );

		for( int ix = 0; ix < 3; ++ix ) {
			renderLight.shaderParms[SHADERPARM_RED + ix] = color[ix];
		}
	} 
}

/*
==============
hhProjectile::CreateLight
==============
*/
int hhProjectile::CreateLight( const char* shaderName, const idVec3& size, const idVec3& color, const idVec3& offset, float fadeTime ) {
	int fadeDuration = 0;

	if ( size.x <= 0.0f || !g_projectileLights.GetBool() ) {
		return 0;
	}

	if( size.Compare(vec3_zero, VECTOR_EPSILON) ) {
		return 0;
	}

	SIMDProcessor->Memset( &renderLight, 0, sizeof(renderLight) );
	FreeLightDef();

	if( !shaderName || !shaderName[0] ) {
		return 0;
	}

	UpdateLightPosition();

	renderLight.shader = declManager->FindMaterial( shaderName, false );
	renderLight.pointLight = true;
	renderLight.lightRadius = size;
	renderLight.shaderParms[SHADERPARM_RED] = color[0];
	renderLight.shaderParms[SHADERPARM_GREEN] = color[1];
	renderLight.shaderParms[SHADERPARM_BLUE] = color[2];
	renderLight.shaderParms[SHADERPARM_ALPHA] = 1.0f;
	renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.GetTime() );
	renderLight.noShadows = true;		//HUMANHEAD bjk: cheaper

	fadeDuration = SEC2MS( fadeTime );

	lightOffset = offset;
	lightColor = color;
	lightStartTime = gameLocal.GetTime();
	lightEndTime = lightStartTime + fadeDuration;

	BecomeActive(TH_MISC2);

	if( lightDefHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
	} else {
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}

	return fadeDuration;
}

/*
==============
hhProjectile::BounceSplat
==============
*/
void hhProjectile::BounceSplat( const idVec3& origin, const idVec3& dir ) {
	float size = hhMath::Lerp( spawnArgs.GetVec2("decal_bounce_size", "1.8 2.2"), gameLocal.random.RandomFloat() );
	const char* decal = spawnArgs.RandomPrefix( "mtr_bounce_shader", gameLocal.random );
	if( decal && *decal ) {
		gameLocal.ProjectDecal( origin, dir, 16.0f, true, size, decal );
	}
}


/*
================
hhProjectile::Create

HUMANHEAD: AOBMERGE - Overridden
================
*/
void hhProjectile::Create( idEntity *owner, const idVec3 &start, const idVec3 &dir ) {
	Create( owner, start, dir.ToMat3() );
}

/*
================
hhProjectile::Launch

HUMANHEAD: AOBMERGE - Overridden
================
*/
void hhProjectile::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	Launch( start, dir.ToMat3(), pushVelocity, timeSinceFire, launchPower, dmgPower );

	launchTimestamp = gameLocal.time;	//HUMANHEAD: mdc - record launch time
}

/*
================
hhProjectile::Create

HUMANHEAD: AOBMERGE - Overridden
================
*/
void hhProjectile::Create( idEntity *owner, const idVec3 &start, const idMat3 &axis ) {
	Unbind();

	SetOrigin( start );
	SetAxis( axis );

	this->owner = owner;

	CreateLight( spawnArgs.GetString("mtr_light_shader"), spawnArgs.GetVector("light_size"), spawnArgs.GetVector("light_color", "1 1 1") * spawnArgs.GetFloat("light_intensity", "1.0"), spawnArgs.GetVector("light_offset"), spawnArgs.GetFloat("light_fadetime") );

	GetPhysics()->SetContents( 0 );

	state = CREATED;
}

/*
=================
hhProjectile::Launch

HUMANHEAD: aob - made second parameter idMat3&
=================
*/
void hhProjectile::Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
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

	spawnArgs.GetVector( "velocity", "0 0 0", velocity );
	spawnArgs.GetAngles( "angular_velocity", "0 0 0", angular_velocity );

	linear_friction		= spawnArgs.GetFloat( "linear_friction" );
	angular_friction	= spawnArgs.GetFloat( "angular_friction" );
	contact_friction	= spawnArgs.GetFloat( "contact_friction" );
	bounce				= spawnArgs.GetFloat( "bounce" );
	mass				= spawnArgs.GetFloat( "mass" );
	gravity				= spawnArgs.GetFloat( "gravity" );
	fuse				= spawnArgs.GetFloat( "fuse" );

	projectileFlags.detonate_on_world	= spawnArgs.GetBool( "detonate_on_world" );
	projectileFlags.detonate_on_actor	= spawnArgs.GetBool( "detonate_on_actor" );
	projectileFlags.randomShaderSpin	= spawnArgs.GetBool( "random_shader_spin" );
	projectileFlags.isLarge				= spawnArgs.GetBool( "largeProjectile" );	// HUMANHEAD bjk

	if ( mass <= 0 ) {
		gameLocal.Error( "Invalid mass on '%s'\n", GetClassname() );
	}

	thrust = mass * spawnArgs.GetFloat( "thrust" );
	thrust_start = SEC2MS( spawnArgs.GetFloat("thrust_start") ) + gameLocal.GetTime();
	thrust_end = SEC2MS( spawnArgs.GetFloat("thrust_duration") ) + thrust_start;
	//HUMANHEAD: aob - if thrust is set then set TH_THINK
	if( hhMath::Fabs(thrust) >= VECTOR_EPSILON ) {
		BecomeActive( TH_THINK );
	}
	//HUMANHEAD END

	if ( health ) {
		fl.takedamage = true;
	}

	gravVec = gameLocal.GetGravity();
	gravVec.NormalizeFast();

	Unbind();

	int contents = DetermineContents();
	int clipMask = DetermineClipmask();

	//HUMANHEAD rww
	launchQuat = axis.ToCQuat(); //save off launch orientation
	launchPos = start; //save off launch pos
	//HUMANHEAD END

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	//HUMANHEAD: aob - added DetermineClipModelOwner so some projectiles can decide to collide with there owners
	physicsObj.GetClipModel()->SetOwner( DetermineClipModelOwner() );
	//HUMANHEAD END
	physicsObj.SetMass( mass );
	physicsObj.SetFriction( linear_friction, angular_friction, contact_friction );
	if ( contact_friction == 0.0f ) {
		physicsObj.NoContact();
	}
	physicsObj.SetBouncyness( bounce );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetContents( contents );
	physicsObj.SetClipMask( clipMask );
	physicsObj.SetLinearVelocity( axis[ 0 ] * velocity[ 0 ] + axis[ 1 ] * velocity[ 1 ] + axis[ 2 ] * velocity[ 2 ] + pushVelocity );
	/*
	if (gameLocal.isClient) {
		gameLocal.Printf("C: (%f %f %f) (%f %f %f) (%f %f %f, %f %f %f, %f %f %f)\n", velocity[0], velocity[1], velocity[2],
			pushVelocity[0], pushVelocity[1], pushVelocity[2], axis[0][0], axis[0][1], axis[0][2], axis[1][0], axis[1][1],
			axis[1][2], axis[2][0], axis[2][1], axis[2][2]);
	}
	else {
		gameLocal.Printf("S: (%f %f %f) (%f %f %f) (%f %f %f, %f %f %f, %f %f %f)\n", velocity[0], velocity[1], velocity[2],
			pushVelocity[0], pushVelocity[1], pushVelocity[2], axis[0][0], axis[0][1], axis[0][2], axis[1][0], axis[1][1],
			axis[1][2], axis[2][0], axis[2][1], axis[2][2]);
	}
	*/

	physicsObj.SetAngularVelocity( angular_velocity.ToAngularVelocity() * axis );
	physicsObj.SetOrigin( start );
	physicsObj.SetAxis( axis );
	SetPhysics( &physicsObj );

	//HUMANHEAD rww - get rid of the thruster, needless projectile constructor overhead
	//thruster.SetPosition( &physicsObj, 0, physicsObj.GetCenterOfMass() );//idVec3( GetPhysics()->GetBounds()[ 0 ].x, 0, 0 ) );

	//HUMANHEAD rww - debug projectile position and axis (for client side projectiles)
	/*
	extern void Debug_ClearDebugLines(void);
	extern void Debug_AddDebugLine(idVec3 &start, idVec3 &end, int color);

	idVec3 p = start;
	idVec3 prj;
	Debug_ClearDebugLines();
	prj = p + (axis[0]*32.0f);
	Debug_AddDebugLine(p, prj, 1);
	prj = p + (axis[1]*32.0f);
	Debug_AddDebugLine(p, prj, 2);
	prj = p + (axis[2]*32.0f);
	Debug_AddDebugLine(p, prj, 3);
	*/

	if ( !gameLocal.isClient || fl.clientEntity ) //HUMANHEAD rww - if clientEntity
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
		} else {
			fuse -= timeSinceFire;
			if ( fuse < 0.0f ) {
				fuse = 0.0f;
			}
			PostEventSec( &EV_Fizzle, fuse );
		}
	}

	StartSound( "snd_fly", SND_CHANNEL_BODY, 0, true );

	//HUMANHEAD: aob - replaces id's smoke_fly code
	// CJR:  Changed so that we can randomly choose between multiple fx_fly systems on a single projectile
	BroadcastFxPrefixedRandom( "fx_fly", EV_SpawnFxFlyLocal );
	//HUMANHEAD END

	// used for the plasma bolts but may have other uses as well
	if ( projectileFlags.randomShaderSpin ) {
		float f = gameLocal.random.RandomFloat();
		f *= 0.5f;
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = f;
	}

	// HUMANHEAD CJR: if launched by a player, set the projectile's weapon index appropriately
	if ( owner.IsValid() && owner->IsType( hhPlayer::Type ) ) {
		hhPlayer *player = static_cast<hhPlayer *>( owner.GetEntity() );
		weaponNum = player->GetCurrentWeapon();				
	} else { // Otherwise, default the weaponNum to denote a non-player weapon
		weaponNum = -1;
	}// HUMANHEAD END

	state = LAUNCHED;

	if (gameLocal.isClient) { //HUMANHEAD rww
		launchTimestamp = gameLocal.time;	//HUMANHEAD: mdc - record launch time
		return;
	}

	// Notify the AI about this launch	
	gameLocal.SendMessageAI( this, GetOrigin(), spawnArgs.GetFloat("ai_notify_launch", "0"), MA_OnProjectileLaunch );
	BroadcastEntityDef( spawnArgs.GetString("def_driver"), EV_SpawnDriverLocal );

	launchTimestamp = gameLocal.time;	//HUMANHEAD: mdc - record launch time
}

/*
================
hhProjectile::DetermineClipModelOwner

HUMANHEAD: aob
================
*/
idEntity* hhProjectile::DetermineClipModelOwner() {
	return (spawnArgs.GetBool("collideWithOwner")) ? this : owner.GetEntity();
}

/*
================
hhProjectile::Collide
================
*/
extern const int	RB_VELOCITY_EXPONENT_BITS; //HUMANHEAD rww
extern const int	RB_VELOCITY_MANTISSA_BITS; //HUMANHEAD rww
bool hhProjectile::Collide( const trace_t& collision, const idVec3& velocity ) {
	if ( state == EXPLODED || state == FIZZLED || state == COLLIDED ) {	//HUMANHEAD bjk
		return false;
	}

	// HUMANHEAD CJR: Don't allow the collision if the projectile has recently hit a portal
	if ( collidedPortal.IsValid() ) {
		return false;
	} // HUMANHEAD END

	if ( gameLocal.isClient ) {
		//HUMANHEAD rww - our projectile stuff is pretty different from id's at this point, so i'm just making some new prediction code.
		if (state == EXPLODED || state == FIZZLED || state == COLLIDED) {	//HUMANHEAD bjk
			ProcessCollision(&collision, velocity);
			return false;
		}
		//HUMANHEAD END
	}

	//HUMANHEAD rww - commented out to allow projectiles to collide with owner after portalling
	/*
	if (gameLocal.isClient) { //HUMANHEAD rww
		idEntity* entHit = gameLocal.GetTraceEntity(collision);
		if ( entHit == owner.GetEntity() ) {
			return true;
		}
	}
	else {
		// get the entity the projectile collided with
		idEntity* entHit = gameLocal.GetTraceEntity(collision);
		if ( entHit == owner.GetEntity() ) {
			assert( 0 );
			return false;
		}
	}
	*/

	// remove projectile when a 'noimpact' surface is hit
	if ( collision.c.material && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) ) {
		common->DPrintf("removing projectile that hit noimpact surface: mat=[%s]\n", collision.c.material->GetName() );
		RemoveProjectile( 0 );
		return false;
	}

	//HUMANHEAD rww - send events for collisions on the server for pseudo-sync'd projectiles
	if (gameLocal.isServer && fl.networkSync && !netSyncPhysics) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();

		msg.WriteBits(collision.c.contents, 32);
		msg.WriteFloat(collision.c.dist);
		msg.WriteBits(collision.c.entityNum, GENTITYNUM_BITS);
		msg.WriteShort(collision.c.id);
		if (!collision.c.material) {
			msg.WriteShort(-1);
		}
		else {
			msg.WriteShort(collision.c.material->Index());
		}
		msg.WriteBits(collision.c.modelFeature, 32);
		//ensure it is normalized properly first
		idVec3 normal = collision.c.normal;
		normal.Normalize();
		msg.WriteDir(normal, 24);
		msg.WriteFloat(collision.c.point.x);
		msg.WriteFloat(collision.c.point.y);
		msg.WriteFloat(collision.c.point.z);
		msg.WriteBits(collision.c.trmFeature, 32);
		msg.WriteBits(collision.c.type, 4);
		msg.WriteFloat(collision.fraction);

		//unfortunately, this is needed for proper decal projections
		msg.WriteFloat(velocity.x, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS);
		msg.WriteFloat(velocity.y, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS);
		msg.WriteFloat(velocity.z, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS);

		msg.WriteDir(collision.endAxis[0], 24);

		ServerSendPVSEvent(EVENT_PROJECTILE_EXPLOSION, &msg, collision.endpos);
	}
	//HUMANHEAD END

	//HUMANHEAD rww - our projectile stuff is pretty different from id's at this point, so i'm just making some new prediction code.
	if (gameLocal.isClient) {
		//ProcessCollision(&collision, velocity);
		if (fl.networkSync && !netSyncPhysics) { //if this projectile is psuedo-sync'd, we will be expecting impact results from the server.
			ClientHideProjectile();
			return 0;
		}
		return ProcessCollisionEvent( &collision, velocity );
	}
	//HUMANHEAD END

	if ( !gameLocal.isMultiplayer && spawnArgs.GetString( "hit_notify" ) && owner.IsValid() && owner->IsType( hhMonsterAI::Type ) ) {
		gameLocal.SendMessageAI( this, GetOrigin(), spawnArgs.GetFloat("ai_notify_yes", "0"), MA_OnProjectileHit );
	}

	return ProcessCollisionEvent( &collision, velocity );
}

/*
================
hhProjectile::ProcessCollisionEvent
================
*/
bool hhProjectile::ProcessCollisionEvent( const trace_t* collision, const idVec3& velocity ) {
	assert( collision );

	idEntity* ent = gameLocal.entities[ collision->c.entityNum ];
	const idEventDef* eventDef = matterEventsCollision.GetPartner( ent, collision->c.material );
	assert( eventDef );

	ProcessEvent( eventDef, collision, velocity );
	return gameLocal.program.GetReturnedBool();
}

/*
================
hhProjectile::ProcessCollision

HUMANHEAD: aob
================
*/
int hhProjectile::ProcessCollision( const trace_t* collision, const idVec3& velocity ) {
	PROFILE_SCOPE("ProjectileCollision", PROFMASK_PHYSICS|PROFMASK_COMBAT);
	idEntity* entHit = gameLocal.entities[ collision->c.entityNum ];

	SetOrigin( collision->endpos );
	SetAxis( collision->endAxis );

	if( fxFly.IsValid() ) {
		fxFly->Stop();
	}
	SAFE_REMOVE( fxFly );
	FreeLightDef();
	CancelEvents( &EV_Fizzle );

	if (entHit) { //rww - may be null on client.
		if (!gameLocal.isClient || (fl.networkSync && !netSyncPhysics)) { //don't do this on the client, unless this is a sync'd projectile
			DamageEntityHit( collision, velocity, entHit );
		}
	}

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	surfTypes_t matterType = gameLocal.GetMatterType( entHit, collision->c.material, "hhProjectile::ProcessCollision" );
	return PlayImpactSound( gameLocal.FindEntityDefDict(spawnArgs.GetString("def_damage")), collision->endpos, matterType );
}

/*
================
hhProjectile::DamageEntityHit

HUMANHEAD: aob
================
*/
void hhProjectile::DamageEntityHit( const trace_t* collision, const idVec3& velocity, idEntity* entHit ) {
	PROFILE_SCOPE("DamageEntityHit", PROFMASK_COMBAT);
	if (GERMAN_VERSION || g_nogore.GetBool()) {
		if (entHit->IsType(idActor::Type)) {
			idActor *actor = reinterpret_cast<idActor *> (entHit);
			if ( !actor->fl.takedamage || ( actor->IsActiveAF() && !actor->spawnArgs.GetBool( "not_gory", "0" ) ) ) {
				// Don't process hits on ragdolls
				return;
			}
		}
	}

	float push = 0.0f;
	float damageScale = 1.0f;
	const char *damage = NULL;
	if (gameLocal.isMultiplayer) { //rww - check for special mp damage def
		damage = spawnArgs.GetString( "def_damage_mp" );
	}
	if (!damage || !damage[0]) {
		damage = spawnArgs.GetString( "def_damage" );
	}
	hhPlayer* playerHit = (entHit->IsType(hhPlayer::Type)) ? static_cast<hhPlayer*>(entHit) : NULL;
	idAFEntity_Base* afHit = (entHit->IsType(idAFEntity_Base::Type)) ? static_cast<idAFEntity_Base*>(entHit) : NULL;

	idVec3 dir = velocity.ToNormal();

	// non-radius damage defs can also apply an additional impulse to the rigid body physics impulse
	const idDeclEntityDef *def = gameLocal.FindEntityDef( damage, false );
	if ( def ) {
		if (entHit->IsType(hhProjectile::Type)) {
			push = 0.0f; // mdl:  Don't let projectiles push each other
		} else if (afHit && afHit->IsActiveAF() ) {
			push = def->dict.GetFloat( "push_ragdoll" );
		} else {
			push = def->dict.GetFloat( "push" );
		}
	}

	if (!gameLocal.isClient) { //rww
		if( playerHit ) {
			// pdm: save collision location in case we want to project a blob there
			playerHit->playerView.SetDamageLoc( collision->endpos );
		}

		if( DamageIsValid(collision, damageScale) && entHit->fl.takedamage ) {
			UpdateBalanceInfo( collision, entHit );

			if( damage && damage[0] ) {
				idEntity *killer = owner.GetEntity();
				if (killer && killer->IsType(hhVehicle::Type)) { //rww - handle vehicle projectiles killing people
					hhVehicle *veh = static_cast<hhVehicle *>(killer);
					if (veh->GetPilot()) {
						killer = veh->GetPilot();
					}
				}

				entHit->Damage( this, killer, dir, damage, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE(collision->c.id) );

				if ( playerHit && def->dict.GetInt( "freeze_duration" ) > 0 ) {
					playerHit->Freeze( def->dict.GetInt( "freeze_duration" ) );
				}
			}
		}

		// HUMANHEAD bjk: moved to after damage so impulse can be applied to ragdoll
		if ( push > 0.0f ) {
			if (g_debugImpulse.GetBool()) {
				gameRenderWorld->DebugArrow(colorYellow, collision->c.point, collision->c.point + (push*dir), 25, 2000);
			}

			entHit->ApplyImpulse( this, collision->c.id, collision->c.point, push * dir );
		}
	}

	if ( entHit->fl.applyDamageEffects ) {
		ApplyDamageEffect( entHit, collision, velocity, damage );
	}
}

/*
================
hhProjectile::DamageIsValid

HUMANHEAD: aob
================
*/
bool hhProjectile::DamageIsValid( const trace_t* collision, float& damageScale ) {
	damageScale = DetermineDamageScale( collision );

	return true;
}

/*
================
hhProjectile::ApplyDamageEffect

HUMANHEAD: aob
================
*/
void hhProjectile::ApplyDamageEffect( idEntity* hitEnt, const trace_t* collision, const idVec3& velocity, const char* damageDefName ) {
	if( hitEnt ) {
		hitEnt->AddDamageEffect( *collision, velocity, damageDefName, (!fl.networkSync || netSyncPhysics) );
	}
}

int hhProjectile::DetermineContents() {
	return (spawnArgs.GetBool("proj_collision")) ? CONTENTS_PROJECTILE | CONTENTS_SHOOTABLE : CONTENTS_PROJECTILE;
}

int hhProjectile::DetermineClipmask() {
	return MASK_SHOT_RENDERMODEL;
}

/*
================
hhProjectile::UpdateBalanceInfo

HUMANHEAD: aob
================
*/
void hhProjectile::UpdateBalanceInfo( const trace_t* collision, const idEntity* hitEnt ) {
	hhPlayer* player = (owner.IsValid() && owner->IsType(hhPlayer::Type)) ? static_cast<hhPlayer*>(owner.GetEntity()) : NULL;

	if( hitEnt->IsType(idActor::Type) && player ) {
		player->SetLastHitTime( gameLocal.GetTime() );
		player->AddProjectileHits( 1 );
	}
}

/*
================
hhProjectile::SpawnProjectile

HUMANHEAD: aob
================
*/
hhProjectile* hhProjectile::SpawnProjectile( const idDict* args ) {//FIXME: Broadcast
	assert( args );
	assert(!gameLocal.isClient);

	idEntity* ent = NULL;

	gameLocal.SpawnEntityDef( *args, &ent );
	HH_ASSERT( ent && ent->IsType(hhProjectile::Type) );

	return static_cast<hhProjectile*>(ent);
}

/*
================
hhProjectile::SpawnClientProjectile

HUMANHEAD: rww
================
*/
hhProjectile* hhProjectile::SpawnClientProjectile( const idDict* args ) {//FIXME: Broadcast
	assert( args );

	idEntity* ent = NULL;

	gameLocal.SpawnEntityDef( *args, &ent, true, gameLocal.isClient );
	HH_ASSERT( ent && ent->IsType(hhProjectile::Type) );

	ent->fl.networkSync = false;
	ent->fl.clientEvents = true;

	return static_cast<hhProjectile*>(ent);
}

/*
================
hhProjectile::Killed
================
*/
void hhProjectile::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	//HUMANHEAD: aob - added collision so we can get explosion to show up when killed
	trace_t collision;

	if ( spawnArgs.GetBool( "detonate_on_death" ) ) {
		memset( &collision, 0, sizeof( trace_t ) );
		collision.fraction = 0.0f;
		collision.endpos = GetOrigin();
		collision.endAxis = GetAxis();
		collision.c.entityNum = attacker->entityNumber;
		collision.c.normal = inflictor->GetOrigin() - GetOrigin();
		collision.c.normal.Normalize();
		Explode( &collision, GetPhysics()->GetLinearVelocity(), 0 );
	} else {
		Fizzle();
	}
}

/*
================
hhProjectile::Fizzle
================
*/
void hhProjectile::Fizzle( void ) {
	if ( state == EXPLODED || state == FIZZLED || state == COLLIDED ) {	//HUMANHEAD bjk
		return;
	}

	int removeTime = StartSound( "snd_fizzle", SND_CHANNEL_BODY, 0, true );

	if (!gameLocal.isClient)
	{
		// fizzle FX
		hhFxInfo fxInfo;
		fxInfo.SetNormal( -GetAxis()[0] );
		fxInfo.RemoveWhenDone( true );
		BroadcastFxInfoPrefixed( "fx_fuse", GetOrigin(), GetAxis(), &fxInfo );
	}

	SAFE_REMOVE( fxFly );

	fl.takedamage = false;
	physicsObj.SetContents( 0 );
	physicsObj.GetClipModel()->Unlink();
	physicsObj.PutToRest();

	FreeLightDef();

	state = FIZZLED;

	RemoveProjectile( removeTime );
}

/*
=================
hhProjectile::RemoveProjectile
=================
*/
void hhProjectile::RemoveProjectile( const int removeDelay ) {
	Hide();
	BecomeInactive( TH_TICKER|TH_THINK );
	RemoveBinds();//Remove any fx we have because they aren't hidden
	PostEventMS( &EV_Remove, removeDelay );
}

/*
================
hhProjectile::Explode
================
*/
void hhProjectile::Explode( const trace_t* collision, const idVec3& velocity, int removeDelay ) {
	const char *fxname = NULL;
	int length = 0;

	if ( state == EXPLODED || state == FIZZLED || state == COLLIDED ) {	//HUMANHEAD bjk
		return;
	}

	if( !collision ) {
		return;
	}

	//HUMANHEAD: aob
	StartSound( "snd_explode", SND_CHANNEL_BODY, 0, true, &length );
	removeDelay = hhMath::hhMax( length, removeDelay );

	// explosion light
	//FIXME: may need to be Broadcast
	removeDelay = hhMath::hhMax( removeDelay, CreateLight(spawnArgs.GetString("mtr_explode_light_shader"), spawnArgs.GetVector("explode_light_size"), spawnArgs.GetVector("explode_light_color", "1 1 1") * spawnArgs.GetFloat("explode_light_intensity", "1.0"), spawnArgs.GetVector("explode_light_offset"), spawnArgs.GetFloat("explode_light_fadetime")) );

	//HUMANHEAD: aob - moved logic to helper function
	SpawnExplosionFx( collision );

	SpawnDebris( collision->c.normal, velocity.ToNormal() );
	//HUMANHEAD END

	state = EXPLODED;

	//HUMANHEAD rww
	if (!gameLocal.isClient) {
		idEntity *killer = owner.GetEntity();
		if (killer && killer->IsType(hhVehicle::Type)) { //rww - handle vehicle projectiles killing people
			hhVehicle *veh = static_cast<hhVehicle *>(killer);
			if (veh->GetPilot()) {
				killer = veh->GetPilot();
			}
		}

		// splash damage
		if (!gameLocal.isClient) {
			//SplashDamage( collision->endpos, killer, this, this, spawnArgs.GetString("def_splash_damage") );
			SplashDamage( GetOrigin(), killer, this, this, spawnArgs.GetString("def_splash_damage") );
		}
	}
	//HUMANHEAD END

	//HUMANHEAD: aob - moved logic to helper function
	RemoveProjectile( removeDelay );
	//HUMANHEAD END
}

/*
================
hhProjectile::SplashDamage

HUMANHEAD: aob
================
*/
void hhProjectile::SplashDamage( const idVec3& origin, idEntity* attacker, idEntity* ignoreDamage, idEntity* ignorePush, const char* splashDamageDefName ) {
	if( splashDamageDefName && splashDamageDefName[0] ) {
		gameLocal.RadiusDamage( origin, this, attacker, ignoreDamage, ignorePush, splashDamageDefName );
	}
}

/*
================
hhProjectile::SpawnExplosionFx

HUMANHEAD: aob
================
*/
void hhProjectile::SpawnExplosionFx( const trace_t* collision ) {
	idEntity*		hitEnt = NULL;
	hhFxInfo		fxInfo;
	const idDict*	dict = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_detonateFx"), false );

	if( !dict || !collision || collision->fraction >= 1.0f ) {
		return;
	}

	hitEnt = gameLocal.entities[ collision->c.entityNum ];

	fxInfo.SetNormal( collision->c.normal );
	fxInfo.SetIncomingVector( GetAxis()[0] );
	fxInfo.SetBounceVector( hhProjectile::GetBounceDirection( physicsObj.GetLinearVelocity(),  
														collision->c.normal,
														this, hitEnt
													   ) );
	fxInfo.RemoveWhenDone( true );
	surfTypes_t matterType = gameLocal.GetMatterType( hitEnt, collision->c.material, "hhProjectile::SpawnExplosionFx" );
	const char* fxKey = gameLocal.MatterTypeToMatterKey( "fx", matterType );
	BroadcastFxInfo( dict->RandomPrefix(fxKey, gameLocal.random), GetOrigin(), GetAxis(), &fxInfo, NULL, false ); //rww - no broadcast, only locally.
	//FIXME: Once BroadcastFxInfo() is gone and we are spawning the entityfx directly, mark it as fl.neverdormant here
}

/*
================
hhProjectile::SpawnDebris

HUMANHEAD: aob
================
*/
void hhProjectile::SpawnDebris( const idVec3& collisionNormal, const idVec3& collisionDir ) {
	static char		debrisKey[] = "def_debris";
	static char		countKey[] = "debris_count";
	static char		spreadKey[] = "debris_spread";

	idDebris *debris = NULL;
	idEntity *ent = NULL;
	idStr indexStr;
	int amount = 0;
	const idDict *dict = NULL;
	const idKeyValue* defKV = NULL;
	for( defKV = spawnArgs.MatchPrefix(debrisKey, NULL); defKV; defKV = spawnArgs.MatchPrefix(debrisKey, defKV) ) {

		if( !defKV->GetValue().Length() ) {
			continue;
		}
		
		dict = gameLocal.FindEntityDefDict( defKV->GetValue().c_str(), false );
		if( !dict ) {
			continue;
		}

		indexStr = defKV->GetKey();
		indexStr.Strip( debrisKey );

		if (gameLocal.isMultiplayer) { //rww - for decreasing the count for things in mp on a design basis
			amount = hhMath::Lerp( spawnArgs.GetVec2(va("%s_mp%s", countKey, indexStr.c_str())), gameLocal.random.RandomFloat() );
			if (!amount) {
				amount = hhMath::Lerp( spawnArgs.GetVec2(va("%s%s", countKey, indexStr.c_str())), gameLocal.random.RandomFloat() );
			}
		}
		else {
			amount = hhMath::Lerp( spawnArgs.GetVec2(va("%s%s", countKey, indexStr.c_str())), gameLocal.random.RandomFloat() );
		}
		for ( int i = 0; i < amount; i++ ) {
			//HUMANHEAD: aob
			idVec3 dir = hhUtils::RandomSpreadDir( collisionNormal.ToMat3(), DEG2RAD(spawnArgs.GetFloat(va("%s%s", spreadKey, indexStr.c_str()))) );
			//HUMAMHEAD END

			gameLocal.SpawnEntityDef( *dict, &ent, true, gameLocal.isClient ); //HUMANHEAD rww - make them local non-broadcast entities.
			if ( !ent || !ent->IsType( idDebris::Type ) ) {
				gameLocal.Error( "hhProjectile: 'projectile_debris' is not an idDebris" );
			}

			debris = static_cast<idDebris *>(ent);
			debris->Create( owner.GetEntity(), GetOrigin() + collisionNormal*10, dir.ToMat3() );	// HUMANHEAD bjk: displace out of surface
			debris->Launch();
			debris->fl.networkSync = false; //HUMANHEAD rww
			debris->fl.clientEvents = true; //HUMANHEAD rww
		}
	}
}

/*
================
hhProjectile::SetGravity
================
*/
void hhProjectile::SetGravity( const idVec3 &newGravity ) {
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
hhProjectile::ProcessAllowCollisionEvent
================
*/
bool hhProjectile::ProcessAllowCollisionEvent( const trace_t* collision ) {
	assert( collision );

	idEntity* ent = gameLocal.entities[ collision->c.entityNum ];
	const idEventDef* eventDef = matterEventsAllowCollision.GetPartner( ent, collision->c.material );
	assert( eventDef );

	ProcessEvent( eventDef, collision );
	return gameLocal.program.GetReturnedBool();
}

//=============================================================================
//
// hhProjectile::Portalled
//
// The projectile was just portalled.  Update the fx info to the bound fly fx system
// HUMANHEAD CJR
//=============================================================================

void hhProjectile::Portalled(idEntity *portal) {
	if ( fxFly.IsValid() ) {
		hhFxInfo fxInfo;
		fxInfo.SetNormal( -GetAxis()[0] );
		fxInfo.SetEntity( this );
		fxInfo.RemoveWhenDone( false );

		fxFly->SetFxInfo( fxInfo );

		// Reset the fx system
		fxFly->Stop();
		fxFly->Start( gameLocal.time );
	}	
	if (physicsObj.GetClipModel()) { //HUMANHEAD rww - allow projectiles to collide with owner after portalling
		physicsObj.GetClipModel()->SetOwner(this);
	}
} // END HUMANHEAD

//=============================================================================
//
// hhProjectile::AllowCollision
//
// Determines if the projectile can strike a given entity.  Here, all 
// projectiles will pass-through Wraiths (other than arrows, which is handled
// in the arrow code)
//=============================================================================
bool hhProjectile::AllowCollision( const trace_t& collision ) {
	return ProcessAllowCollisionEvent( &collision );
}

/*
================
hhProjectile::Event_Fuse_Explode
================
*/
void hhProjectile::Event_Fuse_Explode() {
	trace_t collision;

	SIMDProcessor->Memset( &collision, 0, sizeof(trace_t) );
	collision.endpos = GetOrigin();
	collision.endAxis = GetAxis();
	collision.c.normal = -gameLocal.GetGravityNormal();
	Explode( &collision, GetPhysics()->GetLinearVelocity(), 0 );
}

/*
================
hhProjectile::Event_Collision_Explode
================
*/
void hhProjectile::Event_Collision_Explode( const trace_t* collision, const idVec3& velocity ) {
	Explode( collision, velocity, ProcessCollision(collision, velocity) );
	idThread::ReturnInt( 1 );
}

/*
================
hhProjectile::Event_Collision_Impact
================
*/
void hhProjectile::Event_Collision_Impact( const trace_t* collision, const idVec3& velocity ) {
	CancelEvents( &EV_Explode );
	RemoveProjectile( ProcessCollision(collision, velocity) );
	state = COLLIDED;
	idThread::ReturnInt( 1 );
}

/*
================
hhProjectile::Event_Collision_DisturbLiquid
================
*/
void hhProjectile::Event_Collision_DisturbLiquid( const trace_t* collision, const idVec3& velocity ) {
	CancelEvents( &EV_Explode );
	RemoveProjectile( ProcessCollision(collision, velocity) );
	idThread::ReturnInt( 1 );
}

/*
================
hhProjectile::Event_Collision_Remove
================
*/
void hhProjectile::Event_Collision_Remove( const trace_t* collision, const idVec3& velocity ) {
	RemoveProjectile( 0 );
	idThread::ReturnInt( 1 );
}

/*
================
hhProjectile::Event_AllowCollision_CollideNoProj
================
*/
void hhProjectile::Event_AllowCollision_CollideNoProj( const trace_t* collision ) {
	idEntity* ent = gameLocal.entities[ collision->c.entityNum ];
	if( ent->IsType( hhProjectile::Type ) ) {
		if ( static_cast<hhProjectile*>(ent)->ProjCollide() ) {
			idThread::ReturnInt( 1 );
			return;
		}
	}

	if( ent->IsType( hhProjectile::Type ) &&							// If we're colliding with another projectile
		( !bNoCollideWithCrawlers ||									//   AND we're not set to collide with crawlers
		  !( ent->IsType( hhProjectileRocketLauncher::Type ) &&			//     OR we're not a rocket launcher projectile
			 ent->IsType( hhProjectileCrawlerGrenade::Type ) ) ) ) {	//       AND we're not a crawler projectile
		idThread::ReturnInt( 0 );										// Pass through
		return;
	}

	hhProjectile::Event_AllowCollision_Collide( collision );
}

/*
================
hhProjectile::Event_AllowCollision_Collide
================
*/
void hhProjectile::Event_AllowCollision_Collide( const trace_t* collision ) {
	idThread::ReturnInt( 1 );
}

/*
================
hhProjectile::Event_AllowCollision_PassThru
================
*/
void hhProjectile::Event_AllowCollision_PassThru( const trace_t* collision ) {
	idThread::ReturnInt( 0 );
}

/*
================
hhProjectile::Event_SpawnDriverLocal

HUMANHEAD: aob
================
*/
void hhProjectile::Event_SpawnDriverLocal( const char* defName ) {
	if( !defName || !defName[ 0 ] ) {
		return;
	}
	
	driver = static_cast<hhAnimDriven*>( gameLocal.SpawnObject(defName) );
	driver->SetPassenger( this );

	//Not sure if this should be pulled out of the def_driver dict or not
	float roll = hhMath::Lerp( spawnArgs.GetVec2("driver_rollRange"), gameLocal.random.RandomFloat() );
	driver->SetAxis( idAngles( 0.0f, 0.0f, roll ).ToMat3() * driver->GetAxis() );
}

/*
================
hhProjectile::Event_SpawnFxFlyLocal

HUMANHEAD: aob
================
*/
void hhProjectile::Event_SpawnFxFlyLocal( const char* defName ) {
	if( !defName || !defName[0] ) {
		return;
	}

	SAFE_REMOVE(fxFly);
	hhFxInfo fxInfo;

	fxInfo.SetNormal( -GetAxis()[0] );
	fxInfo.SetEntity( this );
	fxInfo.RemoveWhenDone( false );
	fxFly = SpawnFxLocal( defName, GetOrigin(), GetAxis(), &fxInfo, true ); //rww - client (local) entity.
	if (fxFly.IsValid()) {
		fxFly->fl.neverDormant = true;

		//rww
		fxFly->fl.networkSync = false;
		fxFly->fl.clientEvents = true;
	}
}

//================
//hhProjectile::Save
//================
void hhProjectile::Save( idSaveGame *savefile ) const {
	driver.Save( savefile );
	fxFly.Save( savefile );
	savefile->WriteInt( thrust_start );
	savefile->WriteBool( bDDACounted );
	parentProjectile.Save( savefile );
	savefile->WriteInt( launchTimestamp );
	savefile->WriteInt( weaponNum );
	savefile->WriteBool( bPlayFlyBySound );
	savefile->WriteFloat( flyBySoundDistSq );

	collidedPortal.Save( savefile );
	savefile->WriteVec3( collideLocation );
	savefile->WriteVec3( collideVelocity );
}

//================
//hhProjectile::Restore
//================
void hhProjectile::Restore( idRestoreGame *savefile ) {
	driver.Restore( savefile );
	fxFly.Restore( savefile );
	savefile->ReadInt( thrust_start );
	savefile->ReadBool( bDDACounted );
	parentProjectile.Restore( savefile );
	savefile->ReadInt( launchTimestamp );
	savefile->ReadInt( weaponNum );
	savefile->ReadBool( bPlayFlyBySound );
	savefile->ReadFloat( flyBySoundDistSq );

	bNoCollideWithCrawlers = spawnArgs.GetBool( "noCollideWithCrawlers", "0" );
	bProjCollide = spawnArgs.GetBool( "proj_collision", "0" );

	collidedPortal.Restore( savefile );
	savefile->ReadVec3( collideLocation );
	savefile->ReadVec3( collideVelocity );
}

/*
================
hhProjectile::WriteToSnapshot
================
*/
void hhProjectile::WriteToSnapshot( idBitMsgDelta &msg ) const {
	//rww - we capture the launch orientation/pos for predicting the projectile launch, and (usually) don't sync physics at all
	if (fabsf(launchQuat.ToAngles().roll) > 0.001f) { //is it going to translate to a direction happily, or do we need a real orientation?
		msg.WriteBits(1, 1);
		msg.WriteFloat(launchQuat.x);
		msg.WriteFloat(launchQuat.y);
		msg.WriteFloat(launchQuat.z);
	}
	else {
		msg.WriteBits(0, 1);
		msg.WriteDir(launchQuat.ToMat3()[0], 24);
	}
	msg.WriteFloat(launchPos.x);
	msg.WriteFloat(launchPos.y);
	msg.WriteFloat(launchPos.z);

	idProjectile::WriteToSnapshot(msg);
}

/*
================
hhProjectile::ReadFromSnapshot
================
*/
void hhProjectile::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	//rww - we capture the launch orientation for predicting the projectile launch, and (usually) don't sync physics at all
	bool fullOrientation = !!msg.ReadBits(1);
	if (fullOrientation) {
		launchQuat.x = msg.ReadFloat();
		launchQuat.y = msg.ReadFloat();
		launchQuat.z = msg.ReadFloat();
	}
	else {
		idVec3 dir = msg.ReadDir(24);
		launchQuat = dir.ToMat3().ToCQuat();
	}
	launchPos.x = msg.ReadFloat();
	launchPos.y = msg.ReadFloat();
	launchPos.z = msg.ReadFloat();

	idProjectile::ReadFromSnapshot(msg);
}

/*
================
hhProjectile::ClientHideProjectile
================
*/
void hhProjectile::ClientHideProjectile(void) {
	Hide();
	FreeLightDef();
	BecomeInactive(TH_THINK|TH_PHYSICS);

	GetPhysics()->PutToRest();

	if (fxFly.IsValid()) {
		fxFly->Stop();
		SAFE_REMOVE( fxFly );
	}
}

/*
===============
hhProjectile::GetBounceDirection
===============
*/

idVec3 hhProjectile::GetBounceDirection( const idVec3 &incoming, 
								   const idVec3 &surface_normal,
								   const idEntity *incoming_entity,
								   const idEntity *surface_entity ) {
	idVec3 bounceDir;			   
	idVec3 tanget;
	idVec3 normal;
	float dot = 0.0f;
	float eProjectile = 1.0f;				// Elasticity constants
	float eTarget = 1.0f;
	float fProjectile = 0.0f;				// Friction constants
	float fTarget = 0.0f;	
	
	if ( incoming_entity ) {
		eProjectile = incoming_entity->spawnArgs.GetFloat("bounce", "1");
		fProjectile = incoming_entity->spawnArgs.GetFloat("contact_friction", "0");
	}
	
	if ( surface_entity ) {
		eTarget = surface_entity->spawnArgs.GetFloat("bounce", "1");
		fTarget = surface_entity->spawnArgs.GetFloat("contact_friction", "0");
	}

	dot = incoming * surface_normal;
	normal = surface_normal * dot;
	tanget = incoming - normal;

	bounceDir = tanget * (1.0f - fProjectile) * (1.0f - fTarget) -
		normal * (eProjectile * eTarget);

	//? Should this all be seperated out?  Ie, what if they don't want it normalized?
	if ( bounceDir.Length() < .0001f ) {
		bounceDir = surface_normal;
	}
	
	//HUMANHEAD: nla/aob - removed normalize to give return value more dependence on inputs.

	return( bounceDir );
}

