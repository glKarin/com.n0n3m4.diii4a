
#pragma hdrstop

#include "Game_local.h"
#include "Player.h"
#include "Projectile.h"
#include "ai/AI.h"

#include "FireStorm.h"

const int	FIREBEAM_METEOR_DELAY	= 300; // in msec
const int	FIREBEAM_METEOR_NUM		= 8;
const int	FIREBEAM_METEOR_SPEED	= 400;
const int	FIREBEAM_METOER_DISTT	= 200; // distance between pairs in sky
const int	FIREBEAM_METOER_DISTB	= 75; // distance between pairs when reach firebeam
const int	FIREBEAM_METEOR_HEIGHT	= 200;

static const int BFG_DAMAGE_FREQUENCY			= 333;
static const float BOUNCE_SOUND_MIN_VELOCITY	= 200.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 400.0f;

const idEventDef EV_LaunchFireBeam( "LaunchFireBeam", "vvvfff" );

/*************************************

zeroth
idProj_FireBeam

*************************************/

CLASS_DECLARATION( idProjectile, idProj_FireBeam )
	EVENT( EV_LaunchFireBeam,				idProj_FireBeam::Event_Launch )
END_CLASS

void idProj_FireBeam::Spawn( void ) { }

void idProj_FireBeam::Think( void ) {
	if ( state == 2 && meteorNum < FIREBEAM_METEOR_NUM ) {
		idVec3 dir;
		const idDict *boltDef;
		idEntity *bolt;

		idVec3 forw = GetPhysics()->GetLinearVelocity();
		forw.Normalize();

		dir.z = forw.z;

		for ( int i=0; i<2; i++ ) {
			if ( i == 0 ) {
				// counter clockwise
				dir.x = -forw.y;
				dir.y = forw.x;
			} else {
				// clockwise
				dir.x = forw.y;
				dir.y = -forw.x;
			}

			boltDef = gameLocal.FindEntityDefDict( "projectile_firestorm_meteor" );
			gameLocal.SpawnEntityDef( *boltDef, &bolt );

			if ( !bolt ) {
				return;
			}

			idProjectile *blt = static_cast< idProjectile * >(bolt);

			idVec3 vec1 = GetPhysics()->GetOrigin() + dir * FIREBEAM_METOER_DISTT;

			vec1.z += FIREBEAM_METEOR_HEIGHT;
			blt->GetPhysics()->SetOrigin( vec1 );

			idVec3 vec2 = GetPhysics()->GetOrigin() + dir * FIREBEAM_METOER_DISTB;

			vec2 -= vec1;
			vec2.Normalize();

			//blt->SetAngles( vec2.ToAngles() );

			//blt->GetPhysics()->SetLinearVelocity( vec2 * FIREBEAM_METEOR_SPEED );

			blt->Launch( vec1, vec2, vec2 * FIREBEAM_METEOR_SPEED );

		}
		meteorNum++;
	}

	idProjectile::Think();
}

void idProj_FireBeam::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower);
}

void idProj_FireBeam::Event_Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );
}

/*************************************

zeroth
idProj_Incinerator

*************************************/

CLASS_DECLARATION( idProjectile, idProj_Incinerator )

END_CLASS

void idProj_Incinerator::Spawn( void ) {
	ent = NULL;
}

void idProj_Incinerator::Incinerate( void ) {
	particleEmitter_t pe;
	//idStr particleName = "firestorm_incinerator";
	idVec3 origin;
	idMat3 axis;

	// if there's no animator, just bind one flame to the entity where we impacted it
	if ( !ent->GetAnimator() ) {/*
		idEntity *flame;
		const idDict *flameDef = gameLocal.FindEntityDefDict( "firestorm_incinerator_flame" );
		gameLocal.SpawnEntityDef( *flameDef, &flame );
		flame->GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() );
		flame->Bind(ent, 0);*/

		return;
	}

	ent->onFire =  gameLocal.time + 5000;
}

bool idProj_Incinerator::Collide( const trace_t &collision, const idVec3 &velocity ) {
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
		if ( ClientPredictionCollide( this, spawnArgs, collision, velocity, !spawnArgs.GetBool( "net_instanthit" ) ) ) {
			Explode( collision, NULL );
			return true;
		}
		return false;
	}

	// remove projectile when a 'noimpact' surface is hit
	if ( ( collision.c.material != NULL ) && ( collision.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) ) {
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
	} else {
		if ( !projectileFlags.detonate_on_world ) {
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

	damageDefName = spawnArgs.GetString( "def_damage" );

	ignore = NULL;

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
				player->AddProjectileHits( 1 );
				damageScale *= player->PowerUpModifier( PROJECTILE_DAMAGE );
			}
		}

		if ( damageDefName[0] != '\0' ) {
			ent->Damage( this, owner.GetEntity(), dir, damageDefName, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ), idVec3( collision.c.point ) );
			ignore = ent;
		}
	}

	// if the projectile causes a damage effect
	if ( spawnArgs.GetBool( "impact_damage_effect" ) ) {
		// if the hit entity has a special damage effect
		if ( ent->spawnArgs.GetBool( "bleed" ) ) {
			ent->AddDamageEffect( collision, velocity, damageDefName );
		} else {
			AddDefaultDamageEffect( collision, velocity );
		}
	}

	Explode( collision, ignore );
	Incinerate();

	return true;
}

/*************************************

zeroth
idProj_IncineratorFlame

*************************************/

CLASS_DECLARATION( idEntity, idProj_IncineratorFlame )

END_CLASS

void idProj_IncineratorFlame::Spawn() {
	PostEventMS( &EV_Remove, gameLocal.random.RandomFloat() * 1000 + 4000 );
}
