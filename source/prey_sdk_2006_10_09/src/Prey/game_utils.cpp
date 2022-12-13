#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//////////////////////
// hhMatterEventDefPartner
//////////////////////
hhMatterEventDefPartner::hhMatterEventDefPartner( const char* eventNamePrefix ) {
	const char* event = NULL;
	const idEventDef* eventDef = NULL;
	surfTypes_t type = SURFTYPE_NONE;

	for( int ix = 0; ix < NUM_SURFACE_TYPES; ++ix ) {
		type = (surfTypes_t)ix;
		event = va( "<%s_%s>", eventNamePrefix, gameLocal.MatterTypeToMatterName(type) );
		eventDef = idEventDef::FindEvent( event );
		AddPartner( eventDef, type );
	}
}


//////////////////////
// hhUtils
//////////////////////



int hhUtils::ContentsOfBounds(const idBounds &localBounds, const idVec3 &location, const idMat3 &axis, idEntity *pass) {
	idTraceModel trm;
	trm.SetupBox(localBounds);
	idClipModel *clipModel = new idClipModel(trm);
	int contents = gameLocal.clip.Contents(location, clipModel, axis, MASK_ALL, pass);
	delete clipModel;
	return contents;
}

/*
====================
hhUtils::EntityDefWillFit
	Check whether a given entity def will fit in a location if spawned, pass the contents that would block it
====================
*/
bool hhUtils::EntityDefWillFit( const char *defName, const idVec3 &location, const idMat3 &axis, int contentMask, idEntity *passEntity ) {
	idBounds localBounds;
	idVec3 size;

	// Calculate a local bounds for object
	const idDict *dict = gameLocal.FindEntityDefDict(defName, false);
	localBounds.Zero();
	if (dict) {
		if ( dict->GetVector( "mins", NULL, localBounds[0] ) &&
			dict->GetVector( "maxs", NULL, localBounds[1] ) ) {
		} else if ( dict->GetVector( "size", NULL, size ) ) {
			localBounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
			localBounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
		} else { // Default bounds
			localBounds.Expand( 1.0f );
		}
	}

	// Determine contents of the bounds
	idTraceModel trm;
	trm.SetupBox(localBounds);
	idClipModel *clipModel = new idClipModel(trm);
	int contents = gameLocal.clip.Contents(location, clipModel, axis, contentMask, passEntity);
	delete clipModel;

/*	if (contents & CONTENTS_SOLID) {
		gameRenderWorld->DebugBounds( colorRed, localBounds, location, 5000);
	}
	else if (contents == 0) {
		gameRenderWorld->DebugBounds( colorGreen, localBounds, location, 5000);
	}
	else {
		gameRenderWorld->DebugBounds( colorYellow, localBounds, location, 5000);
	}*/

	return (contents == 0);
}


/*
====================
hhUtils::SpawnDebrisMass
====================
*/
void hhUtils::SpawnDebrisMass( const char *debrisMassEntity, 
							   const idVec3 &origin,
							   const idVec3 *orientation,
							   const idVec3 *velocity,
							   const int power,
							   bool nonsolid,
							   float *duration,
							   idEntity *entForBounds ) { //HUMANHEAD rww - added entForBounds

	idDict args;
	idVec3 powerVector;


	if ( !debrisMassEntity  || !debrisMassEntity[0] ) {
		gameLocal.Warning( "Invalid Debris Entity called (%p)", debrisMassEntity );
		return;
	}

	args.SetVector( "origin", origin );

	// Set the values to the dictionary
	if ( orientation ) {
		args.SetVector( "orientation", *orientation );
	}

	if ( power >= 0 ) {
		powerVector.x = powerVector.y = powerVector.z = power;
		args.SetVector( "power", powerVector );
	}

	if ( nonsolid ) {
		args.Set( "nonsolid", "1" );
	} 

	if ( velocity ) {
		args.SetVector( "velocity", *velocity );
	}

	if (entForBounds) { //HUMANHEAD rww
		args.SetBool("spawnUsingEntity", true);
		args.SetBool("useAFBounds", true);
	}

	// Spawn the object
	idEntity *ent = gameLocal.SpawnObject( debrisMassEntity, &args );

	if ( duration && ent->IsType( hhDebrisSpawner::Type ) ) {
		hhDebrisSpawner *dspawner = static_cast<hhDebrisSpawner *> ( ent );

		if (entForBounds) { //HUMANHEAD rww
			dspawner->Activate(entForBounds);
		}

		*duration = dspawner->GetDuration();
	}
}


/*
====================
hhUtils::SpawnDebrisMass
====================
*/
void hhUtils::SpawnDebrisMass( const char *debrisMassEntity, 
							   idEntity *sourceEntity,
							   const int power ) {

	idEntity *mass;
	idDict args;


	if ( !debrisMassEntity  || !debrisMassEntity[0] ) {
		gameLocal.Warning( "Invalid Debris Entity called (%p).", debrisMassEntity );
		return;
	}

	args.SetVector( "origin", sourceEntity->GetOrigin() );
	args.SetVector( "orientation", sourceEntity->GetPhysics()->GetAxis()[0] );
	args.SetVector( "velocity", sourceEntity->GetPhysics()->GetLinearVelocity() );

	if ( power >= 0 ) {
		args.SetInt( "power", power );
	}

	args.SetInt( "spawnUsingEntity" , 1 );


	// Spawn the object
	mass = gameLocal.SpawnObject( debrisMassEntity, &args );
	if ( mass ) {
		if ( mass->IsType( hhDebrisSpawner::Type ) ) {
			( ( hhDebrisSpawner * ) mass )->Activate( sourceEntity );
		}
	}
	else {
		gameLocal.Printf("Error spawning debris mass: %s\n", debrisMassEntity );
	}

}


hhProjectile *hhUtils::LaunchProjectile(idEntity *attacker,
										const char *projectile,
										const idMat3 &axis,
										const idVec3 &origin ) {

	hhProjectile *proj=NULL;
	idDict args;
	args.Clear();
	args.Set( "classname", projectile );
	args.Set( "origin", origin.ToString() );

	idEntity *ent=NULL;
	if (gameLocal.SpawnEntityDef( args, &ent ) && ent && ent->IsType(hhProjectile::Type)) {
		proj = ( hhProjectile * )ent;
		proj->Create( attacker, origin, axis );
		proj->Launch( origin, axis, vec3_origin );
	}

	return proj;
}

void hhUtils::DebugCross( const idVec4 &color, const idVec3 &start, int size, const int lifetime ) {
	static idVec3 xaxis(1.0f,0.0f,0.0f);
	static idVec3 yaxis(0.0f,1.0f,0.0f);
	static idVec3 zaxis(0.0f,0.0f,1.0f);
	gameRenderWorld->DebugLine(color, start-xaxis*size, start+xaxis*size, lifetime);
	gameRenderWorld->DebugLine(color, start-yaxis*size, start+yaxis*size, lifetime);
	gameRenderWorld->DebugLine(color, start-zaxis*size, start+zaxis*size, lifetime);
}

void hhUtils::DebugAxis( const idVec3 &origin, const idMat3 &axis, int size, const int lifetime ) {
	gameRenderWorld->DebugArrow(colorRed, origin, origin+axis[0]*size, 5, lifetime);
	gameRenderWorld->DebugArrow(colorGreen, origin, origin+axis[1]*size, 5, lifetime);
	gameRenderWorld->DebugArrow(colorBlue, origin, origin+axis[2]*size, 5, lifetime);
}


idVec3 hhUtils::RandomVector() {
	idVec3 vec;
	vec.x = gameLocal.random.CRandomFloat();
	vec.y = gameLocal.random.CRandomFloat();
	vec.z = gameLocal.random.CRandomFloat();
	return vec;
}

float hhUtils::RandomSign() {
	return gameLocal.random.RandomFloat() < 0.5f ? -1.0f : 1.0f;
}

idVec3 hhUtils::RandomPointInBounds(idBounds &bounds) {
	idVec3 point;
	point.x = bounds[0].x + (bounds[1].x - bounds[0].x) * gameLocal.random.RandomFloat();
	point.y = bounds[0].y + (bounds[1].y - bounds[0].y) * gameLocal.random.RandomFloat();
	point.z = bounds[0].z + (bounds[1].z - bounds[0].z) * gameLocal.random.RandomFloat();
	return point;
}

idVec3 hhUtils::RandomPointInShell( const float innerRadius, const float outerRadius ) {
	idAngles dir( gameLocal.random.RandomFloat() * 360.0f, gameLocal.random.RandomFloat() * 360.0f, gameLocal.random.RandomFloat() * 360.0f );
	return dir.ToForward() * (innerRadius + (outerRadius - innerRadius) * gameLocal.random.RandomFloat());
}

/*
=================
hhUtils::SplitString
  Strips out the first element which is usually the original command
=================
*/
void hhUtils::SplitString( const idCmdArgs& input, idList<idStr>& pieces ) {
	int numParms = input.Argc() - 1;
	for( int ix = 0; ix < numParms; ++ix ) {
		pieces.Append( input.Argv(ix + 1) );	
	}
}

/*
=================
hhUtils::SplitString
  Takes a comma seperated string, and returns the bits between the commas
    Spaces on either side of the comma are stripped off
    The 'pieces' list is not cleared.
=================
*/
void hhUtils::SplitString( const idStr& input, idList<idStr>& pieces, const char delimiter ) {
	idStr element;
	int	endIndex = -1;
	const char groupDelimiter = '\'';
	char currentChar = '\0';
	
	for( int startIndex = 0; startIndex <= input.Length(); ++startIndex ) {
		currentChar = input[ startIndex ];
		if (currentChar) {
			if( currentChar == groupDelimiter ) {
				endIndex = input.Find( currentChar, startIndex + 1 );
				element = input.Mid( startIndex + 1, endIndex - startIndex - 1 );

				startIndex = endIndex;

				pieces.Append( element );
				element.Clear();
				continue;
			} else if( currentChar == delimiter ) {
				element += '\0';
				pieces.Append( element );
				element.Clear();
				continue;
			}

			element += currentChar;
		}
	}

	if( element.Length() ) {
		pieces.Append( element );
	}
}


/*
=================
hhUtils::GetValues
Takes a source dict, and a key base string, and returns all values for keys with that base. ie:

  "base"		"b"
  "base1"		"b1"
  "base4"		"b4"
  "base_bob"	"bob"
  
  is the dict.
  
The option 'numericOnly' restricts the keys to be either an exact match, or a numeric addition
  
  hhUtils::GetValues( dict, "base", strList, true );
  
strList would contain: "b", "b1", "b4"

while:

  hhUtils::GetValues( dict, "base", strList );
  
strList would contain: "b", "b1", "b4", "bob"

=================
*/
void hhUtils::GetValues( idDict &source, const char *keyBase, idList<idStr> &values, bool numericOnly ) {
	idList<idStr>	keys;

	GetKeysAndValues( source, keyBase, keys, values, numericOnly );
}		// hhUtils::GetValues( idDict &, const char *, idList<idStr>, [bool] )


/*
=================
hhUtils::GetKeys
Takes a source dict, and a key base string, and returns all keys with that base. ie:

  "base"		"b"
  "base1"		"b1"
  "base4"		"b4"
  "base_bob"	"bob"
  
  is the dict.
  
The option 'numericOnly' restricts the keys to be either an exact match, or a numeric addition
  
  hhUtils::GetValues( dict, "base", strList, true );
  
strList would contain: "base", "base1", "base4"

while:

  hhUtils::GetValues( dict, "base", strList );
  
strList would contain: "base", "base1", "base4", "base_bob"

=================
*/
void hhUtils::GetKeys( idDict &source, const char *keyBase, idList<idStr> &keys, bool numericOnly ) {
	idList<idStr>	values;

	GetKeysAndValues( source, keyBase, keys, values, numericOnly );
}


/*
==============
hhUtils::getKeysAndValues
==============
*/
void hhUtils::GetKeysAndValues( idDict &source, const char *keyBase, idList<idStr> &keys, idList<idStr> &values, bool numericOnly ) {
	const idKeyValue *	kv = NULL;
	idStr keyEnd;


	for ( kv = source.MatchPrefix( keyBase, kv );
		  kv && kv->GetValue().Length(); 
		  kv = source.MatchPrefix( keyBase, kv ) ) {
		keyEnd = kv->GetKey();
		keyEnd.Strip( keyBase );

		// Is a valid debris base And it isn't a variation.  (ie, contains a .X postfix)
		if ( !numericOnly  || ( idStr::IsNumeric( keyEnd ) && keyEnd.Find( '.' ) < 0 ) ) {
			//gameLocal.Printf( "Adding %s => %s\n", (const char *) kv->GetKey(), (const char *) kv->GetValue() );
			keys.Append( kv->GetKey() );		
			values.Append( kv->GetValue() );		
		}
		
	}

}

/*
=================
hhUtils::RandomSpreadDir
=================
*/
idVec3	hhUtils::RandomSpreadDir( const idMat3& baseAxis, const float spread ) {
	float ang = hhMath::Sin( spread * gameLocal.TimeBasedRandomFloat() );
	float spin = hhMath::TWO_PI * gameLocal.TimeBasedRandomFloat();
	idVec3 dir = baseAxis[ 0 ] + baseAxis[ 2 ] * ( ang * hhMath::Sin(spin) ) - baseAxis[ 1 ] * ( ang * hhMath::Cos(spin) );
	dir.Normalize();

	return dir;
}

//-----------------------------------------------------
// ProjectOntoScreen
//
// Project a world position onto screen
//-----------------------------------------------------
idVec3 hhUtils::ProjectOntoScreen(idVec3 &world, const renderView_t &renderView) {
	idVec3 pdc(-1000.0f, -1000.0f, -1.0f);

	// Convert world -> camera
	idVec3 view = ( world - renderView.vieworg ) * renderView.viewaxis.Inverse();

	// Orient from doom coords to camera coords (look down +Z)
	idVec3 cam;
	cam.x = -view[1];
	cam.y = -view[2];
	cam.z = view[0];

	if (cam.z > 0.0f) {
		// Adjust for differing FOVs
		float halfwidth = renderView.width * 0.5f;
		float halfheight = renderView.height * 0.5f;
		float f = halfwidth / tan( renderView.fov_x * 0.5f * idMath::M_DEG2RAD );
		float g = halfheight / tan( renderView.fov_y * 0.5f * idMath::M_DEG2RAD );

		// Project onto screen
		pdc[0] = (cam.x * f / cam.z) + halfwidth;
		pdc[1] = (cam.y * g / cam.z) + halfheight;
		pdc[2] = cam.z;
	}

	return pdc;	// negative Z indicates behind the view
}


/*
=================
hhUtils::GetLocalGravity
=================
*/
idVec3 hhUtils::GetLocalGravity( const idVec3& origin, const idBounds& bounds, const idVec3& defaultGravity ) {
	idEntity*			entityList[ MAX_GENTITIES ];
	idEntity*			entity = NULL;
	hhGravityZoneBase*	zone = NULL;

	int numEntities = gameLocal.clip.EntitiesTouchingBounds( bounds.Translate(origin), CONTENTS_TRIGGER, entityList, MAX_GENTITIES );
	for( int ix = 0; ix < numEntities; ++ix ) {
		entity = entityList[ ix ];

		if( !entity ) {
			continue;
		}
	
		if( !entity->IsType(hhGravityZoneBase::Type) ) {
			continue;
		}

		zone = static_cast<hhGravityZoneBase*>( entity );
		if( !zone->IsActive() || !zone->IsEnabled() ) {
			continue;
		}

		// More expensive check if non-simple zone
		if ( !zone->isSimpleBox ) {
			idTraceModel *playerTrm = new idTraceModel(bounds);
			idClipModel *playerModel = new idClipModel( *playerTrm );
			playerModel->SetContents(CONTENTS_TRIGGER);

			idClipModel *zoneModel = zone->GetPhysics()->GetClipModel();

			int contents = gameLocal.clip.ContentsModel( playerModel->GetOrigin(), playerModel, playerModel->GetAxis(), -1,
				zoneModel->Handle(), zoneModel->GetOrigin(), zoneModel->GetAxis() );

			delete playerModel;
			delete playerTrm;
			if ( !contents ) {
				continue;
			}
		}

		return zone->GetCurrentGravity( origin );
	}

	return defaultGravity;
}

//=============================================================================
//
// PointToAngle
//
// Point should be about the origin
//=============================================================================
float hhUtils::PointToAngle(float x, float y)
{
	if ( x == 0 && y == 0 ) {
		return 0;
	}

	if ( x >= 0 ) { // x >= 0
		if (y >= 0 ) {	// y >= 0
			if ( x > y ) {
				return atan( y / x );     // octant 0
			} else {
				return DEG2RAD(90) - atan( x / y );  // octant 1
			}
		}
		else { // y < 0
			y = -y;
			if ( x > y ) {
				return -atan( y / x );  // octant 8
			} else {
				return DEG2RAD(270) + atan( x / y );  // octant 7
			}
		}
	}
	else { // x < 0
		x = -x;
		if ( y >= 0 ) { // y>= 0
			if ( x > y ) {
				return DEG2RAD(180) - atan( y / x ); // octant 3
			}
			else {
				return DEG2RAD(90) + atan( x / y ); // octant 2
			}
		}
		else { // y < 0
			y = -y;
			if ( x > y ) {
				return DEG2RAD(180) + atan( y / x ); // octant 4
			} else {
				return DEG2RAD(270) - atan( x / y ); // octant 5
			}
		}
	}

	return 0;
}

float hhUtils::DetermineFinalFallVelocityMagnitude( const float totalFallDist, const float gravity ) {
	if( hhMath::Fabs(gravity) <= VECTOR_EPSILON ) {
		return 0.0f;
	}

	return gravity * hhMath::Sqrt( 2.0f * totalFallDist / gravity );
}

/*
======================
hhUtils::ChannelName2Num
returns -1 if not found
HUMANHEAD nla
======================
*/
int hhUtils::ChannelName2Num( const char *name, const idDict *entityDef ) {
	int num;

	entityDef->GetInt( va( "channel2num_%s", name ), "-1", num );
	
	return( num );
}


/*
================
hhUtils::CreateFxDefinition

//HUMANHEAD: aob - used for our wound system
================
*/
void hhUtils::CreateFxDefinition( idStr &definition, const char* smokeName, const float duration ) {

	definition.Clear();

	if( !smokeName || !smokeName[0] ) {
		return;
	}

	definition = va(
		"fx %s	// IMPLICITLY GENERATED\n"
		"{ {\n"
		"name \"%s\"\n"
		"duration %.2f\n"
		"delay 0\n"
		"restart 0\n"
		"particle \"%s.prt\"\n"
		"} }\n", smokeName, smokeName, duration, smokeName);
}

/*
================
hhUtils::CalculateSoundVolume
	Calculate a linear volume from a velocity magnitude and min/max range
================
*/
float hhUtils::CalculateSoundVolume( const float value, const float min, const float max ) {
	float linear;
	float decibels;

	if( min == max ) {
		return 0.0f;
	}

	linear = hhMath::ClampFloat( 0.0f, 1.0f, (value - min) / (max - min) );
	linear = (linear * 0.95f) + 0.05f;	// Make lower end -30dB not -60dB

	decibels = hhMath::Scale2dB(linear);
	decibels -= 3;
	linear = hhMath::dB2Scale(decibels);

	return linear;
}

/*
================
hhUtils::CalculateScale
================
*/
float hhUtils::CalculateScale( const float value, const float min, const float max ) {
	if( min == max ) {
		return 0.0f;
	}

	return hhMath::ClampFloat( 0.0f, 1.0f, (value - min) / (max - min) );
}

// Find all entities overlapping this clipmodel
// Pass a contents to limit the entities found to those matching that contents
int hhUtils::EntitiesTouchingClipmodel( idClipModel *clipModel, idEntity **entityList, int maxCount, int contents ) {
	int				i, j, numClipModels, numEntities;
	idClipModel *	clipModels[ MAX_GENTITIES ];
	idClipModel *	cm;
	idEntity *		ent;

	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipModel->GetAbsBounds(), contents, clipModels, MAX_GENTITIES );
	numEntities = 0;

	// For each entity in bounds of our clipModel
	//	test if entity overlaps the clipModel
	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModels[ i ];

		if (cm == clipModel) {
			continue;
		}

		ent = cm->GetEntity();

		if (!ent || !ent->GetPhysics()->GetClipModel()->IsTraceModel()) {
			// Can only test versus trace models
			continue;
		}

		// if the entity is not yet in the list
		for ( j = 0; j < numEntities; j++ ) {
			if ( entityList[j] == ent ) {
				break;
			}
		}
		if ( j < numEntities ) {
			continue;
		}

		// Test if entity clipmodel overlaps our clipmodel
		if ( !ent->GetPhysics()->ClipContents( clipModel ) ) {
			continue;
		}

		// add entity to the list
		if ( numEntities >= maxCount ) {
			gameLocal.Warning( "hhUtils::EntitiesTouchingClipmodel: max count" );
			break;
		}
		entityList[numEntities++] = ent;

	}

	return numEntities;
}

idBounds hhUtils::ScaleBounds( const idBounds& bounds, float scale ) {
	idBounds localBounds;

	if( scale <= 0.0 || scale == 1.0f ) {
		return bounds;
	}

	//Put bounds at origin so our scale is done correctly
	localBounds = bounds.Translate( -bounds.GetCenter() );

	for( int ix = 0; ix < 2; ++ix ) {
		localBounds[ix] *= scale;
	}

	return localBounds;
}

idVec3 hhUtils::DetermineOppositePointOnBounds( const idVec3& start, const idVec3& dir, const idBounds& bounds ) {
	float scale = 0.0f;
	float radius = bounds.GetRadius();

	if( !bounds.RayIntersection(start, dir, scale) ) {
		return start;
	}

	if( !bounds.RayIntersection(start + dir * radius, -dir, scale) ) {
		return start;
	}
	
	return start + dir * (radius - scale);
}

/*
=============
hhUtils::PassArgs
  Take any args that begin with prefix, and pass them into dict
=============
*/

void hhUtils::PassArgs( const idDict &source, idDict &dest, const char *passPrefix ) {
	const idKeyValue *	kv = NULL;
	idStr			indexStr;

	// Loop through looking for the next valid key to pass
	for ( kv = source.MatchPrefix( passPrefix, kv );
		  kv && kv->GetValue().Length(); 
		  kv = source.MatchPrefix( passPrefix, kv ) ) {
		indexStr = kv->GetKey();
		indexStr.Strip( passPrefix );

//		gameLocal.Printf( "Passing %s => %s\n", 
//					(const char *) indexStr,
//					(const char *) kv->GetValue() );

		dest.Set( indexStr, kv->GetValue() );
	}
}

