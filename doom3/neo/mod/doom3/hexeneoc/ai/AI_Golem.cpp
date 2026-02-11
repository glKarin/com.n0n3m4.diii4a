
#pragma hdrstop

#include "Entity.h"
#include "Game_local.h"
#include "Moveable.h"
#include "AI.h"
#include "AI_Golem.h"

const idVec3 GOLEM_ROCKS_NO_GRAVITY				= idVec3( 0, 0, 0 );
const int GOLEM_ROCKS_MAX						= 13;
const int GOLEM_ROCKS_MIN						= 8;
const int GOLEM_ROCKS_MIN_CHECK_DELAY			= 2000;
const int GOLEM_ROCKS_MAX_DIST					= 300;
const int GOLEM_ROCKS_ATTRACT_SPEED				= 240;
const int GOLEM_RICKS_MAX_MOVE_TIME_BEFORE_BIND	= 5000;
const int GOLEM_ROCKS_BIND_DISTANCE				= 16;

const idEventDef AI_Golem_SawEnemy( "SawEnemy" );

CLASS_DECLARATION( idAI, idAI_Golem )
	EVENT( AI_Golem_SawEnemy,	idAI_Golem::Event_SawEnemy )
END_CLASS

void idAI_Golem::Spawn() {
	alive = false;
	nextFindRocks = 0;
    totRocks = 0;
    nextBoneI = 0;
    nextBone(); // start er off
	golemType = spawnArgs.GetString("golemType");
	onEnt = 0;
	onFire = -1;
}

void idAI_Golem::Save( idSaveGame *savefile ) const {
    savefile->WriteFloat( totRocks );
	savefile->WriteFloat( nextFindRocks );
	savefile->WriteInt( nextBoneI );
	savefile->WriteString( curBone );
	savefile->WriteString( golemType );
	savefile->WriteBool( alive );
	savefile->WriteInt( rocks.Num() );
	for ( int i = 0; i < rocks.Num(); i++ ) {
		savefile->WriteObject( rocks[i] );
	}
}

void idAI_Golem::Restore( idRestoreGame *savefile ) {
    savefile->ReadFloat( totRocks );
	savefile->ReadFloat( nextFindRocks );
	savefile->ReadInt( nextBoneI );
	savefile->ReadString( curBone );
	savefile->ReadString( golemType );
	savefile->ReadBool( alive );
	int i;
	savefile->ReadInt( i );
	idClass *obj;
	for ( ; i > 0; i-- ) {
		savefile->ReadObject( obj );
		rocks.Append( static_cast< idMoveable* >( obj ) );
	}
}

void idAI_Golem::findNewRocks() {
	idMoveable *rock = FindGolemRock();
	if ( rock ) {
		addRock( rock );
	}
}


void idAI_Golem::addRock( idMoveable *newRock ) {
   	newRock->BecomeNonSolid();
    newRock->spawnArgs.SetBool( "golem_owned", 1 );
	newRock->spawnArgs.SetFloat( "golem_bindTime", gameLocal.time + GOLEM_RICKS_MAX_MOVE_TIME_BEFORE_BIND );
    newRock->spawnArgs.SetBool( "golem_bound", 0 );
	newRock->spawnArgs.Set( "old_bounce", newRock->spawnArgs.GetString( "snd_bounce" ) );
    newRock->spawnArgs.Set( "snd_bounce", "" ); // no bounce sounds while on the golem, gets REAL annoying.
	newRock->spawnArgs.Set( "bone", curBone );
	newRock->health = 1000;
	newRock->GetPhysics()->DisableClip();
	rocks.Append( newRock );

	totRocks++;
	nextBone();	
}

void idAI_Golem::Think() {
	idAI::Think();

	if ( alive ) {
		evalRocks();
	}
}

void idAI_Golem::evalRocks() {
    int i;
    float length;
    idVec3 dist;
    idStr bone;
    
    if ( gameLocal.time > nextFindRocks ) {
		nextFindRocks = gameLocal.time + 2000; // find new rocks every 2 seconds
		findNewRocks();
    }
   
    for (i=0; i < rocks.Num(); i++) {
        if ( !rocks[i] ) {
			continue;
        }

        bone = rocks[i]->spawnArgs.GetString("bone");

        if ( !rocks[i]->spawnArgs.GetBool("golem_bound") ) {
			dist = GetJointPos(animator.GetJointHandle(bone)) - rocks[i]->GetPhysics()->GetOrigin();
			length = dist.Length();
		   
			// if rocks are close enough or max time movement has passed, bind them.
			if ( length < GOLEM_ROCKS_BIND_DISTANCE || gameLocal.time > rocks[i]->spawnArgs.GetFloat("golem_bindTime") ) {
				rocks[i]->GetPhysics()->SetGravity( GOLEM_ROCKS_NO_GRAVITY );
				//rocks[i]->SetOrigin( GetLocalCoordinates( GetJointPos(animator.GetJointHandle(bone)) ) );
				rocks[i]->SetOrigin( GetJointPos(animator.GetJointHandle(bone)) );
				rocks[i]->spawnArgs.Set( "golem_bound", "1");
				rocks[i]->SetAngles( idAngles(gameLocal.random.RandomInt() % 200 - 100, gameLocal.random.RandomInt() % 200 - 100, gameLocal.random.RandomInt() % 200 - 100 ) );
				rocks[i]->BindToJoint(this, bone, 1);
				rocks[i]->GetPhysics()->EnableClip();
			// else just move them
			} else {
				dist.Normalize();
				//looks shitty: rocks[i]->GetPhysics()->SetAngularVelocity( idVec3(gameLocal.random.RandomInt() % 200 - 100, gameLocal.random.RandomInt() % 200 - 100, gameLocal.random.RandomInt() % 200 - 100 ) );
				rocks[i]->GetPhysics()->SetLinearVelocity( dist * GOLEM_ROCKS_ATTRACT_SPEED );
			}

			continue;
		}

		// if a rock takes damage, the golem takes damage
		if ( rocks[i]->health < 1000 ) {
			Damage( NULL, NULL, idVec3(0,0,0), "damage_golem1", 1000 - rocks[i]->health, 0 ); // Z.TODO: this needs to be the damage def of the weapon used...somehow...
			rocks[i]->health = 1000;
		}
    }
}

void idAI_Golem::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, idVec3 &iPoint ) {
	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = (idEntity *) gameLocal.world;
	}
	if ( !attacker ) {
		attacker = (idEntity *) gameLocal.world;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" ) * damageScale;
	damage = GetDamageForLocation( damage, location );

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage > 0 ) {
		health -= damage;
		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}
			gameLocal.Printf( "BLOWUP!\n" );
			BlowUp();
			Killed( inflictor, attacker, damage, dir, location );
		} else {
			Pain( inflictor, attacker, damage, dir, location );
		}
	} else {
		// don't accumulate knockback
		if ( af.IsLoaded() ) {
			// clear impacts
			af.Rest();

			// physics is turned off by calling af.Rest()
			BecomeActive( TH_PHYSICS );
		}
	}
}

void idAI_Golem::BlowUp( void ) {
	//spawnArgs.SetVector( "origin", GetPhysics()->GetOrigin() );
	//spawnArgs.Set( "classname", "proj_golemsoul" );
	//gameLocal.SpawnEntityDef( spawnArgs );
	//soul->GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() );

	// rocks get removed from head to toe.
    for (int i=rocks.Num(); i >= 0; i--) {
        if ( !rocks[i] ) {
			rocks[i]->GetPhysics()->SetGravity( gameLocal.GetGravity() );
			rocks[i]->spawnArgs.Set( "golem_owned", 0 );
			rocks[i]->spawnArgs.Set( "golem_bound", 0 );
			rocks[i]->spawnArgs.Set( "golem_bindTime", 0 );
			rocks[i]->spawnArgs.Set( "snd_bounce", rocks[i]->spawnArgs.GetString( "old_bounce" ) ); //Z.TODO
			rocks[i]->Unbind();
			rocks[i]->BecomeSolid();
			idVec3 dx;
			dx.x = gameLocal.random.RandomInt() % 200 - 100;
			dx.y = gameLocal.random.RandomInt() % 200 - 100;
			dx.z = ( gameLocal.random.RandomInt() % 100 ) * 3;
			dx.Normalize();
			rocks[i]->GetPhysics()->SetLinearVelocity( dx * 600 );
			dx.x = gameLocal.random.RandomInt() % 200 - 100;
			dx.y = gameLocal.random.RandomInt() % 200 - 100;
			dx.z = gameLocal.random.RandomInt() % 200 - 100;
			dx.Normalize();
			rocks[i]->GetPhysics()->EnableClip();
			rocks[i]->GetPhysics()->SetAngularVelocity( dx * 100 );
			rocks.RemoveIndex( i );
		}
    }
}

void idAI_Golem::nextBone() {

	// one rock per joint, from toe to head.
	switch ( nextBoneI ) {
		case 0: curBone = "lfoot"; break;
		case 1: curBone = "rfoot"; break;
		case 2: curBone = "lknee"; break;
		case 3: curBone = "rknee"; break;
		case 4: curBone = "pelvis"; break;
		case 5: curBone = "spineup"; break;
		case 6: curBone = "lhand"; break;
		case 7: curBone = "rhand"; break;
		case 8: curBone = "lloarm"; break;
		case 9: curBone = "rloarm"; break;
		case 10: curBone = "luparm"; break;
		case 11: curBone = "ruparm"; break;
		case 12: curBone = "head"; break;
		default: curBone = ""; break;
	}

	nextBoneI++;
}

void idAI_Golem::Event_SawEnemy() {
	alive = true;
}

const char * idAI_Golem::GetGolemType( void ) const {
	return golemType.c_str();
}

// HEXEN : Zeroth
idMoveable *idAI_Golem::FindGolemRock( void ) {
	int hash, i;
	idMoveable *target = NULL;

	hash = gameLocal.entypeHash.GenerateKey( idMoveable::Type.classname, true );

	for ( i = gameLocal.entypeHash.First( hash ); i != -1; i = gameLocal.entypeHash.Next( i ) ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( idMoveable::Type ) ) {
			target = static_cast< idMoveable* >( gameLocal.entities[i] );

			if ( target->IsHidden() ) {
				continue;
			}

			if ( target->spawnArgs.GetInt( "golem_owned" ) == 1 ) {
				continue;
			}
/* smaller golemd haven't been implemented yet
			if ( !target->spawnArgs.GetInt( "forLargeGolem" ) ) {
				continue;
			}
*/
			if ( GetGolemType() != target->spawnArgs.GetString( "rockType" ) ) {
				continue;
			}

			// trace from golem to rock
			trace_t trace;	
			idEntity *traceEnt = NULL;
			gameLocal.clip.TraceBounds( trace, 
				GetJointPos( animator.GetJointHandle( curBone ) ), 
				target->GetPhysics()->GetOrigin(), 
				idBounds( GOLEM_TRACE_MINS, GOLEM_TRACE_MAXS ), 
				MASK_MONSTERSOLID, 
				this );

			if ( trace.fraction < 1.0f ) {
				traceEnt = gameLocal.entities[ trace.c.entityNum ];
			}

			// make sure rock isn't through a wall or something.
			if ( !traceEnt || traceEnt != target ) {
				continue;
			}

			return target;
		}
	}

	return NULL;
}
