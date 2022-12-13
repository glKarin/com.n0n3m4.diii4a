#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_RemoveAll( "removeAll", NULL );

CLASS_DECLARATION( idEntity, hhDebrisSpawner )
	EVENT( EV_Activate,					hhDebrisSpawner::Activate )
END_CLASS

/*
============
hhDebrisSpawner::hhDebrisSpawner()
============
*/
hhDebrisSpawner::hhDebrisSpawner() {
}


/*
============
hhDebrisSpawner::~hhDebrisSpawner()
============
*/
hhDebrisSpawner::~hhDebrisSpawner() {
}


/*
============
hhDebrisSpawner::Spawn()
  Spawns the entity, and all other entities associated with the mass
============
*/
void hhDebrisSpawner::Spawn() {

	// Set the defaults
	activated = false;
	sourceEntity = NULL;
	hasBounds = false;

	// Get the passed in
	spawnArgs.GetVector( "origin", "0 0 0", origin );
	spawnArgs.GetVector( "orientation", "1 0 0", orientation );
	spawnArgs.GetVector( "velocity", "0 0 0", velocity );
	spawnArgs.GetVector( "power", "50 50 50", power );
	spawnArgs.GetFloat( "duration", "0", duration );
	spawnArgs.GetBool( "multi_trigger", "0", multiActivate );
	spawnArgs.GetBool( "spawnUsingEntity", "0", useEntity );
	spawnArgs.GetBool( "fillBounds", "1", fillBounds );
	spawnArgs.GetBool( "testBounds", "1", testBounds );
	spawnArgs.GetBool( "nonsolid", "0", nonSolid );
	spawnArgs.GetBool( "useAFBounds", "0", useAFBounds ); //rww

	// Hide the model
	GetPhysics()->SetContents( 0 );
	Hide();
	bounds = GetPhysics()->GetClipModel()->GetBounds();
	GetPhysics()->DisableClip();

	// Do stuff
	power *= 20;

	// spawnWhenActivated means that 
	if ( !useEntity && !spawnArgs.GetInt( "trigger", "0" ) ) {
		Activate( NULL );
	}
}

void hhDebrisSpawner::Save(idSaveGame *savefile) const {
	savefile->WriteVec3(origin);
	savefile->WriteVec3(orientation);
	savefile->WriteVec3(velocity);
	savefile->WriteVec3(power);
	savefile->WriteBool(activated);
	savefile->WriteBool(multiActivate);
	savefile->WriteBool(hasBounds);
	savefile->WriteBounds(bounds);
	savefile->WriteFloat(duration);
	savefile->WriteBool(fillBounds);
	savefile->WriteBool(testBounds);
	savefile->WriteObject(sourceEntity);
}

void hhDebrisSpawner::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3(origin);
	savefile->ReadVec3(orientation);
	savefile->ReadVec3(velocity);
	savefile->ReadVec3(power);
	savefile->ReadBool(activated);
	savefile->ReadBool(multiActivate);
	savefile->ReadBool(hasBounds);
	savefile->ReadBounds(bounds);
	savefile->ReadFloat(duration);
	savefile->ReadBool(fillBounds);
	savefile->ReadBool(testBounds);
	savefile->ReadObject( reinterpret_cast<idClass *&>(sourceEntity) );

	spawnArgs.GetBool( "spawnUsingEntity", "0", useEntity );
	spawnArgs.GetBool( "nonsolid", "0", nonSolid );
	spawnArgs.GetBool( "useAFBounds", "0", useAFBounds );
}


/*
================
hhDebrisSpawner::Activate
  Do the actual spawning of debris, FX, etc..
================
*/
void hhDebrisSpawner::Activate( idEntity *aSourceEntity ) {

	if ( useEntity ) {
		sourceEntity = aSourceEntity;
	}
	else {				// If we don't have an entity, use our bounds instead
		hasBounds = true;
	}

	if ( !activated || multiActivate ) {
		SpawnDebris();
	
		SpawnFX();
		SpawnDecals();

		// If we have a fixed duration, remove ourselves after they are removed
		if ( ( duration > 0.0f ) && !multiActivate ) {
			PostEventSec( &EV_Remove, duration + 1.0f );
		}	
		
		activated = true;
	}	
}


/*
============
hhDebrisSpawner::SpawnDebris
============
*/
void hhDebrisSpawner::SpawnDebris() {
	const idKeyValue *	kv = NULL;
	idEntity *			debris;
	idDict				args;
	idStr				debrisEntity;
	idList<idStr>		debrisEntities;
	int					numDebris;
	// For entity sources
	idBounds 			sourceBounds;	
	idVec3				sourceBoundCenter;
	idVec3				debrisOrigin;
	idBounds			searchBounds;
	idVec3				debrisVelocity;
	bool				fit;
	idBounds			afBounds; //rww
	bool				gotAFBounds = false; //rww
	idVec3				defOrigin = vec3_zero;
	idAngles			defAngles = ang_zero;

	// Optimization: if player is far away, don't spawn the chunks
	if (!gameLocal.isMultiplayer) {
		float maxDistance = spawnArgs.GetFloat("max_debris_distance");
		if (maxDistance > 0.0f) {
			float distSquared = (gameLocal.GetLocalPlayer()->GetOrigin() - origin).LengthSqr();
			if (distSquared > maxDistance*maxDistance) {
				return;
			}
		}
	}
	args.SetInt( "nodrop", 1 );
	args.SetVector( "origin", origin );
	args.SetFloat( "duration", duration );

	// Pass along variabes requested
	hhUtils::PassArgs( spawnArgs, args, "pass_" );

	if (useAFBounds && sourceEntity && sourceEntity->IsType(idActor::Type)) { //rww - try for AF bounds
		idActor *actor = static_cast<idActor *>(sourceEntity);
		if (actor->IsActiveAF()) { //they are ragdolling, so we have a chance.
			idPhysics_AF *afPhys = actor->GetAFPhysics();

			if (afPhys) { //got the af physics object, now loop through the bodies and collect an appropriate bounds.
				afBounds.Zero();

				for (int bodyCount = 0; bodyCount < afPhys->GetNumBodies(); bodyCount++) {
					idAFBody *b = afPhys->GetBody(bodyCount);
					if (b) {
						idClipModel *bodyClip = b->GetClipModel();
						if (bodyClip) { //now add the bounds of the clipModel into afBounds
							idBounds bodyBounds = bodyClip->GetBounds();
							idVec3 point = (b->GetWorldOrigin()-origin);

							bodyBounds.AddPoint(point);
							afBounds.AddBounds(bodyBounds);
							gotAFBounds = true;
						}
					}
				}
			}
		}
	}

	// Find all Debris
	while ( GetNextDebrisData( debrisEntities, numDebris, kv, defOrigin, defAngles ) ) {
		int numEntities = debrisEntities.Num();
		for ( int count = 0; count < numDebris; ++count ) {
			// Select a debris to use from the list
			debrisEntity = debrisEntities[ gameLocal.random.RandomInt( numEntities ) ];

			// If we have a bounding box to fill, use that
			if ( fillBounds && ( sourceEntity || hasBounds ) ) {
				if (gotAFBounds) { //rww
					sourceBounds = afBounds;
					//gameRenderWorld->DebugBounds(colorGreen, afBounds, origin, 1000);
				}
				else if ( sourceEntity ) {
					sourceBounds = sourceEntity->GetPhysics()->GetBounds();
				}
				else {
					sourceBounds = bounds;
				}

				fit = false;
				for ( int i = 0; i < 4; i++ ) {
					if ( i == 0 && defOrigin != vec3_zero ) {
						//first try bone origin without random offset
						debrisOrigin = defOrigin;
					} else {
						debrisOrigin = origin + hhUtils::RandomPointInBounds( sourceBounds );
					}
		
					if ( !testBounds || hhUtils::EntityDefWillFit( debrisEntity, debrisOrigin, defAngles.ToMat3(), CONTENTS_SOLID, NULL ) ) {
						fit = true;
						break; // Found a spot for the gib
					}
				}
			
				if ( !fit ) { // Gib didn't fit after 4 attempts, so don't spawn the gib
					//common->Warning( "SpawnDebris: gib didn't fit when spawned (%s)\n", debrisEntity.c_str() );
					defAngles = ang_zero;
					continue;
				}

				args.SetVector( "origin", debrisOrigin );
				if ( sourceEntity ) {
					idMat3 sourceAxis = idAngles( 0, sourceEntity->GetAxis().ToAngles().yaw, 0 ).ToMat3();
					args.SetMatrix( "rotation", defAngles.ToMat3() * sourceAxis );
				} else {
					args.SetMatrix( "rotation", defAngles.ToMat3() );
				}
			}


			if ( duration ) {
				args.SetFloat( "removeTime", duration + duration * gameLocal.random.CRandomFloat() );
			}

			// Spawn the object
			debris = gameLocal.SpawnObject( debrisEntity, &args );
			HH_ASSERT(debris != NULL); // JRM - Nick, I added this because of a crash I got. Hopefully this will catch it sooner

			// mdl:  Added this check to make sure no AFs go through the debris spawner
			if ( debris->IsType( idAFEntity_Base::Type ) ) {
				gameLocal.Warning( "hhDebrisSpawner spawned an articulated entity:  '%s'.", debrisEntity.c_str() );
			}

			if ( nonSolid ) {

				idVec3 origin;
				if ( debris->GetFloorPos( 64.0f, origin ) ) {
					debris->SetOrigin( origin ); // Start on floor, since we're not going to be moving at all
					debris->RunPhysics(); // Make sure any associated effects get updated before we turn collision off
				}

				// Turn off collision
				debris->GetPhysics()->SetContents( 0 );
				debris->GetPhysics()->SetClipMask( 0 );
				debris->GetPhysics()->UnlinkClip();
				debris->GetPhysics()->PutToRest();

			} else {
				// Add in random velocity
				idVec3 randVel( gameLocal.random.RandomInt( power.x ) - power.x / 2,
								gameLocal.random.RandomInt( power.y ) - power.y / 2,
								gameLocal.random.RandomInt( power.z ) - power.z / 2 );

				// Set linear velocity
				debrisVelocity.x = velocity.x * power.x * 0.25;
				debrisVelocity.y = velocity.y * power.y * 0.25;
				debrisVelocity.z = 0.0f;
				debris->GetPhysics()->SetLinearVelocity( debrisVelocity + randVel );

				// Add random angular velocity
				idVec3 aVel;
				aVel.x = spawnArgs.GetFloat( "ang_vel", "90.0" ) * gameLocal.random.CRandomFloat();
				aVel.y = spawnArgs.GetFloat( "ang_vel", "90.0" ) * gameLocal.random.CRandomFloat();
				aVel.z = spawnArgs.GetFloat( "ang_vel", "90.0" ) * gameLocal.random.CRandomFloat();
			
				if ( defAngles == ang_zero ) {
					debris->GetPhysics()->SetAxis( idVec3( 1, 0, 0 ).ToMat3() );
				}
				debris->GetPhysics()->SetAngularVelocity( aVel );
			}
		}
		defAngles = ang_zero;

		// Zero out the list
		debrisEntities.Clear();
	}

	return;
}


/*
============
hhDebrisSpawner::SpawnFX
============
*/
void hhDebrisSpawner::SpawnFX() {
	hhFxInfo			fxInfo;
	
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx", origin, orientation.ToMat3(), &fxInfo );
}


//=============================================================================
//
// hhDebrisSpawner::SpawnDecals
//
//=============================================================================

void hhDebrisSpawner::SpawnDecals( void ) {
	const int DIR_COUNT  = 5;
	idVec3 dir[DIR_COUNT] = { ( 1, 0, 0 ), ( -1, 0, 0 ), ( 0, 0, -1 ), ( 0, 1, 0 ), ( 0, -1, 0) };

	// do blood splats
	float size = spawnArgs.GetFloat( "decal_size", "96" );

	const idKeyValue* kv = spawnArgs.MatchPrefix( "mtr_decal" );
	while( kv ) {
		if( kv->GetValue().Length() ) { 
			gameLocal.ProjectDecal( origin, dir[ gameLocal.random.RandomInt( DIR_COUNT ) ], 64.0f, true, size, kv->GetValue().c_str() );
		}
		kv = spawnArgs.MatchPrefix( "mtr_decal", kv );
	}
}

/*
============
hhDebrisSpawner::GetNextDebrisData
============
*/
bool hhDebrisSpawner::GetNextDebrisData( idList<idStr> &entityDefs, int &count, const idKeyValue * &kv, idVec3 &origin, idAngles &angle ) {
	static char		debrisDef[] = "def_debris";
	static char		debrisKey[] = "debris";
	idStr			indexStr;
	idStr			entityDefBase;
	int				numDebris, minDebris, maxDebris;
	origin = vec3_zero;

	// Loop through looking for the next valid debris key
	for ( kv = spawnArgs.MatchPrefix( debrisDef, kv );
		  kv && kv->GetValue().Length(); 
		  kv = spawnArgs.MatchPrefix( debrisDef, kv ) ) {
		indexStr = kv->GetKey();
		indexStr.Strip( debrisDef );

		// Is a valid debris base And it isn't a variation.  (ie, contains a .X postfix)
		if ( idStr::IsNumeric( indexStr ) && indexStr.Find( '.' ) < 0 ) {
			
			// Get Number of Debris
			numDebris = -1;

			if ( sourceEntity && sourceEntity->IsType( idAI::Type ) ) {
				idVec3 bonePos;
				idMat3 boneAxis;
				idStr bone = spawnArgs.GetString( va( "%s%s%s", debrisKey, "_bone", ( const char * ) indexStr ) );
				if( !bone.IsEmpty() ) {
					static_cast<idAI*>(sourceEntity)->GetJointWorldTransform( bone.c_str(), bonePos, boneAxis );
					origin = bonePos;
					angle = spawnArgs.GetAngles( va( "%s%s%s", debrisKey, "_angle", ( const char * ) indexStr ) );
				}
			}
			if ( !spawnArgs.GetInt( va( "%s%s%s", debrisKey, "_num", 
										( const char * ) indexStr ),
									"-1", numDebris ) || numDebris < 0 ) {
				// No num found, look for min and max
				if ( spawnArgs.GetInt( va( "%s%s%s", debrisKey, "_min",
										   ( const char * ) indexStr ),
									   "-1", minDebris ) && minDebris >= 0 &&
					 spawnArgs.GetInt( va( "%s%s%s", debrisKey, "_max",
										   ( const char * ) indexStr ),
									   "-1", maxDebris ) && maxDebris >= 0 ) {
					numDebris = 
						gameLocal.random.RandomInt( ( maxDebris - minDebris ) ) + minDebris;
				}	//. No min/max found

			}	//. No num found
			
			// No valid num found
			if ( numDebris < 0 ) {
				gameLocal.Warning( "ERROR: No debris num could be be found for %s%s",
								   ( const char * ) debrisDef, 
								   ( const char * ) indexStr ); 
			}
			else {		// Valid num found
				const char * 		entityDefPrefix;
				const idKeyValue *	otherDefKey = NULL;
				
				entityDefBase = kv->GetValue();
				count = numDebris;

				entityDefs.Append( entityDefBase );

				// Find the other defs that may be there.  using .X scheme
				entityDefPrefix = va( "%s.", (const char *) kv->GetKey() );

				// gameLocal.Printf( "Looking for %s\n", entityDefPrefix );

				for ( otherDefKey = spawnArgs.MatchPrefix( entityDefPrefix, otherDefKey );
					  otherDefKey && otherDefKey->GetValue().Length(); 
					  otherDefKey = spawnArgs.MatchPrefix( entityDefPrefix, otherDefKey ) ) {
					// gameLocal.Printf( "Would have added %s\n", (const char *) otherDefKey->GetValue() );
					entityDefs.Append( otherDefKey->GetValue() );
				}

				return( true );
			}

		}		//. Valid debris base
	}	//. debris loop
	

	return( false );
}

