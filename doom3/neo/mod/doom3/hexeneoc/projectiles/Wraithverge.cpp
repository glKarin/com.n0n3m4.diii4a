
#pragma hdrstop

#include "Game_local.h"
#include "Player.h"
#include "Projectile.h"
#include "ai/AI.h"
#include "ai/AI_Golem.h"
#include "ai/AI_Veloxite.h"

#include "Wraithverge.h"

const int	PROJ_WRAITH_CHASE_RANGE	= 768;
const int	PROJ_WRAITH_SPAWN_DELAY	= 400;
const int	PROJ_WRAITH_NUM			= 2;
const int	PROJ_WRAITH_NUM_TOME	= 2;
const int	PROJ_WRAITH_CHECK_DELAY	= 300;

CLASS_DECLARATION( idProjectile, idProj_Wraith )

END_CLASS

void idProj_Wraith::Spawn( void ) {
	tome = false;
	
	if ( spawnArgs.GetBool("tome") ) { // if ( owner.GetEntity() && owner.GetEntity()->IsType( idPlayer::Type ) && ( static_cast< idPlayer * >( owner.GetEntity() )->GetPowerTome() ) ) {
		tome = true;
		spawnArgs.Set( "def_damage", "damage_wraithverge_tome" );
	}
}

void idProj_Wraith::Think( void ) {
	if ( wraithSpun == true ) {
		//gameLocal.entities[this->entityNumber] = NULL;
		PostEventSec( &EV_Explode, 0.0f );
		return;
	}

	idProjectile::Think();

	/* current projectile doesnt need spinning, turning this off
	// do spinning
	ang = forw.ToAngles();
	ang.roll = gameLocal.time * 360;
	SetAngles( ang );
	*/

	// delay until spawn new wraiths
	if ( gameLocal.time >= spinTime ) {
		if ( tome ) {
			wraithSpun = true;
			SpawnSubWraiths( PROJ_WRAITH_NUM_TOME );
		} else {
			wraithSpun = true;
			SpawnSubWraiths( PROJ_WRAITH_NUM );
		}
	}
}

void idProj_Wraith::Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	forw = dir;
	wraithInitialized=false;
	wraithSpun = false;

	spinTime = gameLocal.time + PROJ_WRAITH_SPAWN_DELAY;
    
    #if 0
    // this was used when we had an actual shadowspawn model for the projectile
	// spawn the effect around the wraith model
	spawnArgs.Set("model", spawnArgs.GetString("def_projectile"));
	spawnArgs.SetVector("origin", GetPhysics()->GetOrigin());
	spawnArgs.Set("nonsolid", "1");
	spawnArgs.Set("nocollide", "1");
	const idDict *blank = gameLocal.FindEntityDefDict("blank_item");
	effect = gameLocal.SpawnEntityType( idEntity::Type, blank );
	effect->Bind( this, 1 );
    #endif

	idProjectile::Launch( start, dir, pushVelocity, timeSinceFire, launchPower, dmgPower );
}

void idProj_Wraith::SpawnSubWraiths( int num ) {
	idProj_HomingWraith	*newWraith = NULL;
	idEntity	*ent = NULL;
	idVec3		rnd;
	idVec3		dir2;
	float		rndf=0;
	idDict		newWraithDict;

	if ( tome ) {
		newWraithDict.Set( "classname", "projectile_HomingWraith_Tome" );
	} else {
		newWraithDict.Set( "classname", "projectile_HomingWraith" );
	}

	rnd.z = 0;

	// spawn some wraiths
	for (int i=0; i < num; i++) {

		// alternate left & right
		if ( rndf > 0 ) {
			rndf = -1;
			// perpendicular vector counterclockwise
			rnd.x = -forw.y;
			rnd.y = forw.x;
		} else {
			// perpendicular vector clockwise
			rndf = 1;
			rnd.x = forw.y;
			rnd.y = -forw.x;
		}

		dir2.Set(forw.x, forw.y, 0);
		dir2.Normalize();
		float cval = gameLocal.random.RandomFloat();
		dir2 = ( dir2 * cval ) + ( rnd * ( 1 - cval ) );
		dir2.Normalize();

		newWraithDict.SetAngles( "wraithAngles", dir2.ToAngles() );
		newWraithDict.SetVector( "wraithVelocity", defVel * dir2 );
		newWraithDict.SetVector( "wraithOrigin", GetPhysics()->GetOrigin() );
		newWraithDict.SetVector( "wraithDir", dir2 );
		newWraithDict.SetBool( "wraithTome", tome );

		gameLocal.SpawnEntityDef( newWraithDict, &ent );
		if ( !ent ) {
			return;
		}
		newWraith = static_cast< idProj_HomingWraith* >( ent );
	}
}

/*************************************************************************

Homing Wraith

*************************************************************************/

CLASS_DECLARATION( idProjectile, idProj_HomingWraith )

END_CLASS

void idProj_HomingWraith::Spawn( void ) {
	GetPhysics()->SetLinearVelocity(  spawnArgs.GetVector( "wraithVelocity" ) );
	GetPhysics()->SetGravity( idVec3(0, 0, 0.000001f ) ); // grav can't be zero.  //z.todo: try this in def
	GetPhysics()->SetOrigin(  spawnArgs.GetVector( "wraithOrigin" ) );

	SetAngles( spawnArgs.GetAngles( "wraithAngles" ) );

	target = NULL;
	tome = spawnArgs.GetBool( "wraithTome" );
	dir = spawnArgs.GetVector( "wraithDir" );
	defVel = spawnArgs.GetFloat("velocity");
	dieTime	= SEC2MS( spawnArgs.GetFloat("fuse") ) + gameLocal.time; // not working???

	nextSearch = 0;
	hits = 0;
}

void idProj_HomingWraith::Think() {

	if ( state == EXPLODED ) {
		return;
	}

	if ( target ) {
		// move to the head of the enemy, not the origin (feet)
		targOrigin = target->GetPhysics()->GetOrigin();
		targOrigin.z = targOrigin.z + ( targSize.z );
		
		dir = targOrigin -GetPhysics()->GetOrigin();
		
		if ( dir.Length() < 10 ) { // target distance
			//z.todo startSound("snd_explode", SND_CHANNEL_ANY, false );
//			target->GetPhysics()->SetLinearVelocity(target->GetPhysics()->GetLinearVelocity() + dir * 150 ); // push

			//radiusDamage(target->GetPhysics()->GetOrigin()(), this, this, this, "damage_wraithverge", 1); // damage //Z.TODO: make player the attacker
			DirectDamage( spawnArgs.GetString("def_damage"), target );
			target->spawnArgs.Set("wraithChased", "0");

			//get off this target, keep flying for a bit, and find a new one
			target = NULL;
			if ( tome ) {
				nextSearch = gameLocal.time + ( PROJ_WRAITH_CHECK_DELAY / 3 );
			} else {
				nextSearch = gameLocal.time + ( PROJ_WRAITH_CHECK_DELAY );
			}
		}
		
		dir.Normalize();
	} else {
		if ( gameLocal.time > dieTime ) {
			//gameLocal.entities[this->entityNumber] = NULL;
			PostEventSec( &EV_Explode, 0.0f );
		}

		if ( gameLocal.time > nextSearch ) {
			findTarget();
			nextSearch = gameLocal.time + 500 + 250 * gameLocal.random.RandomFloat(); // we don't want to search every frame, that will get real expensive.

			if ( target ) {
				targSize = target->spawnArgs.GetVector("size");
			}
		}
	}
	
	GetPhysics()->SetLinearVelocity(defVel * dir); // chase target, apply velocity here, this way we maintain speed each frameall the time (it likes to slow down)

	SetAngles( dir.ToAngles() );
	
	idProjectile::Think();

}

void idProj_HomingWraith::DirectDamage( const char *meleeDefName, idEntity *ent ) {
	const idDict *meleeDef;
	const char *p;
	const idSoundShader *shader;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown damage def '%s' on '%s'", meleeDefName, name.c_str() );
	}

	if ( !ent->fl.takedamage ) {
		const idSoundShader *shader = declManager->FindSound(meleeDef->GetString( "snd_miss" ));
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		return;
	}

	//
	// do the damage
	//
	p = meleeDef->GetString( "snd_hit" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	ent->Damage( this, this, idVec3(0,0,0), meleeDefName, 1.0f, INVALID_JOINT, idVec3(0,0,0) );

	hits++;

	if ( tome && hits >= 4) {
		target->spawnArgs.Set("wraithChased", "0");
		Explode( owner.GetEntity() );
	}
}

void idProj_HomingWraith::Explode( idEntity *ignore ) {
	trace_t collision;

	memset( &collision, 0, sizeof( collision ) );
	collision.endAxis = GetPhysics()->GetAxis();
	collision.endpos = GetPhysics()->GetOrigin();
	collision.c.point = GetPhysics()->GetOrigin();
	collision.c.normal.Set( 0, 0, 1 );
	AddDefaultDamageEffect( collision, collision.c.normal );
	idProjectile::Explode( collision, ignore );
}

void idProj_HomingWraith::findTarget() {
	int hash, i, j;
	const char *cname=NULL;

	for ( j=0; j<3; j++ ) {
		switch (j) {
			case 0:
				cname = idAI::Type.classname;
				break;
			case 1:
				cname = idAI_Golem::Type.classname;
				break;
			case 2:
				cname = idAI_Veloxite::Type.classname;
				break;/*
			case 3:
				cname = idAI_Shadowspawn::Type.classname;
				break;*/
			default:
				cname = idAI::Type.classname;
				break;
		}

		hash = gameLocal.entypeHash.GenerateKey( cname, true );

		for ( i = gameLocal.entypeHash.First( hash ); i != -1; i = gameLocal.entypeHash.Next( i ) ) {
			if ( gameLocal.entities[i] && !strcmp(gameLocal.entities[i]->GetClassname(), cname ) ) {
				target = static_cast< idAI* >( gameLocal.entities[i] );

				if ( target->IsHidden() ) {
					continue;
				}

				if ( target->health <= 0 ) {
					continue;
				}

				if ( target->spawnArgs.GetFloat("wraithChased") ) {
					continue;
				}

				// check distance
				if ( idVec3( gameLocal.entities[i]->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).Length() > 768 /* wraith chase range */ ) {
					continue;
				}

				target->spawnArgs.Set("wraithChased", "1");
				return;
			}
		}
	}
	target = NULL;
}

#if 0
/*************************************************************************

Pulling Wraith

*************************************************************************/

// pulling wraith was a neat idea, but wraiths come out of the wraithverge. let's stick to the story here...
void proj_PullingWraith::Think() {
	startSound("snd_split", SND_CHANNEL_ANY, false );

	findTarget();
	
	if (! (!target)) {
		targSize = target->spawnArgs.GetVector("size");
		
		SetOrigin(target->GetPhysics()->GetOrigin() - idVec3(0, 0, 125);
		
		while (1) {
			// move to the head of the enemy, not the origin (feet)
			targOrigin = target->GetPhysics()->GetOrigin();
			targOrigin.z = targOrigin.z + ( targSize.z );
			
			dir = targOrigin -GetPhysics()->GetOrigin();
			targetDist = dir.Length();
			
			if (targetDist < 10) { // drag enemy down
				startSound("snd_explode", SND_CHANNEL_ANY, false );
				
				//target->bind(this);
				
				dieTime = gameLocal.time + 2;
				defVel = defVel * 0.5;
				
				target->becomeRagdoll();
				while (gameLocal.time < dieTime) {
					target->GetPhysics()->SetOrigin()(GetPhysics()->GetOrigin());
					GetPhysics()->SetLinearVelocity(defVel * idVec3(0, 0, -1) );
					waitFrame();
				}
				
				target->Set("wraithChased", "0");
				target->remove();
				
				break;
			}
			
			dir = sys.vecNormalize(dir);				
			GetPhysics()->SetLinearVelocity(defVel * dir); // chase target
		GetPhysics()->SetAngles(sys.VecToAngles(dir));
		
			waitFrame();
		}
	}
}
#endif
