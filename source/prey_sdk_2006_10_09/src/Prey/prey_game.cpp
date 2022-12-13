#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"
//#include "../prey/win32/fxdlg.h"

extern idCVar com_forceGenericSIMD;

//HUMANHEAD: aob - needed for networking to send the least amount of bits
const int DECL_MAX_TYPES_NUM_BITS		= hhMath::BitsForInteger( DECL_MAX_TYPES );
//HUMANHEAD END

//=============================================================================
// Overridden functions
//=============================================================================

//---------------------------------------------------
//
//	hhGameLocal::Init
//
//---------------------------------------------------
void hhGameLocal::Init( void ) {

	//HUMANHEAD rww - moved into idGameLocal::Init, this should be done after the managed heap is initialized.
	//ddaManager = new hhDDAManager;	//must be before calling idGameLocal::Init(), otherwise if the map can't load, will cause a crash.
	//HUMANHEAD END

	idGameLocal::Init();

	dwWorldClipModel = NULL;
#if _HH_INLINED_PROC_CLIPMODELS
	inlinedProcClipModels.Clear(); //HUMANHEAD rww
#endif

	sunCorona = NULL; // CJR
	lastAIAlertRadius = 0;

	P_InitConsoleCommands();

	//HUMANHEAD rww - check lglcd validity and reset values
	logitechLCDEnabled = sys->LGLCD_Valid();
	logitechLCDDisplayAlt = false;
	logitechLCDButtonsLast = 0;
	logitechLCDUpdateTime = 0;
	//HUMANHEAD END
}

//---------------------------------------------------
//
//	hhGameLocal::Shutdown
//
//---------------------------------------------------
void hhGameLocal::Shutdown( void ) {

#if INGAME_DEBUGGER_ENABLED
	debugger.Shutdown();		// HUMANHEAD pdm: Shut down the debugger before idDict gets shut down
#endif

	idGameLocal::Shutdown();

	//HUMANHEAD rww - this must be done before managed heap destruction to correspond
	//delete ddaManager;
	//ddaManager = NULL;
	//HUMANHEAD END
}


//---------------------------------------------------
//
//	hhGameLocal::UnregisterEntity
//
//---------------------------------------------------
void hhGameLocal::UnregisterEntity( idEntity *ent ) {
	assert( ent );

	if ( talonTargets.Find( ent ) ) {
		talonTargets.Remove( ent );
	}

	idGameLocal::UnregisterEntity( ent );
}

//---------------------------------------------------
//
//	hhGameLocal::MapShutdown
//
//---------------------------------------------------
void hhGameLocal::MapShutdown( void ) {
	//HUMANHEAD: mdc - added support for automatically dumping stats on map switch/game exit
	if( !isMultiplayer && gamestate != GAMESTATE_NOMAP && g_dumpDDA.GetBool() ) {	//make sure we actually have a valid map to export
		GetDDA()->Export(NULL);	//export the stats
	}
	//Always clear our tracking stats and create a default node
	GetDDA()->ClearTracking();	//Clear Tracking statistics
//	GetDDA()->CreateDefaultSectionNode();	//Setup our default section-node

	idGameLocal::MapShutdown();

	sunCorona			= NULL;

	talonTargets.Clear();
}

//---------------------------------------------------
//
//	hhGameLocal::InitFromNewMap
//
//---------------------------------------------------
void hhGameLocal::InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randseed ) {
	
	//HUMANHEAD rww - throw up the prey logo if using the logitech lcd screen
	if (logitechLCDEnabled) {
		sys->LGLCD_UploadImage(NULL, -1, -1, false, true);
	}
	//HUMANHEAD END

	talonTargets.Clear(); // CJR:  Must be before idGameLocal::InitFromNewMap()

#if INGAME_DEBUGGER_ENABLED
	debugger.Reset();
#endif

	hands.Clear();

	idGameLocal::InitFromNewMap(mapName, renderWorld, soundWorld, isServer, isClient, randseed);

	// CJR: Determine if this map is a LOTA map by looking for a special entity on the map
	bIsLOTA = false;
	if ( FindEntity( "LOTA_ThisMapIsLOTA" ) ) {
		bIsLOTA = true;
	}
}

//=============================================================================
//
// hhGameLocal::CacheDictionaryMedia
//
// This is called after parsing an EntityDef and for each entity spawnArgs before
// merging the entitydef.  It could be done post-merge, but that would
// avoid the fast pre-cache check associated with each entityDef
//
// HUMANHEAD pdm: Override for doing our own precaching
//=============================================================================
void hhGameLocal::CacheDictionaryMedia( const idDict *dict ) {
/*	HUMANHEAD mdc - fix so com_makingBuild stuff will work correctly... (this was causing too early of an out)
	if ( dict == NULL ) {
		return;
	}
*/
	const idKeyValue	*kv = NULL;
	const idDecl		*decl = NULL;
	idFile				*file = NULL;
	idStr				fxName;
	idStr				buffer;

	idGameLocal::CacheDictionaryMedia( dict );

	if( dict == NULL ) {		//mdc - added early out
		return;
	}
//HUMANHEAD mdc - skip caching if we are in development and not making a build
	if( !g_precache.GetBool() && !sys_forceCache.GetBool() && !cvarSystem->GetCVarBool("com_makingBuild") ) {
		return;
	}
//HUMANHEAD END


	// TEMP: precache clip models until they become model_clip
	kv = dict->MatchPrefix( "clipmodel", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "Precaching clipmodel: %s\n", kv->GetValue().c_str() );
			renderModelManager->FindModel( kv->GetValue() );
		}
		kv = dict->MatchPrefix( "clipmodel", kv );
	}

	//HUMANHEAD bjk: removed smoke_wound_. now using fx_wound and is cached in fx_. no seperate handling needed

	kv = dict->MatchPrefix( "beam", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			if (kv->GetValue().Find(".beam") >= 0) {
				declManager->MediaPrint( "Precaching beam %s\n", kv->GetValue().c_str() );
				declManager->FindBeam( kv->GetValue() ); // Cache the beam decl
				renderModelManager->FindModel(kv->GetValue()); // Ensure the beam model is cached as well, since beams are a combination of a decl and a rendermodel
			}
			else {	// Must be a reference to a beam entity
				declManager->MediaPrint( "Precaching beam entityDef %s\n", kv->GetValue().c_str() );
				FindEntityDef( kv->GetValue().c_str(), false );
			}
		}
		kv = dict->MatchPrefix( "beam", kv );
	}
}


//=============================================================================
// New functionality
//=============================================================================

//=============================================================================
//
// hhGameLocal::SpawnAppendedMapEntities
//
// HUMANHEAD pdm: Support for level appending
//=============================================================================
#if DEATHWALK_AUTOLOAD
void hhGameLocal::SpawnAppendedMapEntities() {
	if (additionalMapFile) {
		// Swap mapFile and additionalMapFile
		idMapFile *mainMapFile = mapFile;
		mapFile = additionalMapFile;

		// This routine based largely on idGameLocal::SpawnMapEntities

		// parse the key/value pairs and spawn entities for dw
		// From SpawnMapEntities();
		idMapEntity	*mapEnt;
		int			numEntities;
		idDict		args;

		numEntities = mapFile->GetNumEntities();
		if ( numEntities == 0 ) {
			Error( "...no entities" );
		}

		for ( int i = 1 ; i < numEntities ; i++ ) {	// skip worldspawn
			mapEnt = mapFile->GetEntity( i );
			args = mapEnt->epairs;

			if ( !InhibitEntitySpawn( args ) ) {
				// precache any media specified in the map entity
				gameLocal.CacheDictionaryMedia( &args );

				//HUMANHEAD rww - ok on client
				SpawnEntityDef( args, NULL, true, false, true );
			}
		}

		// Restore main mapfile
		delete additionalMapFile;
		additionalMapFile = NULL;
		mapFile = mainMapFile;


		// Since the world collision is always collision model 0, link our model in like an entity would
		dwWorldClipModel = new idClipModel("dw_worldmap");
		dwWorldClipModel->SetContents(CONTENTS_SOLID);
		dwWorldClipModel->SetEntity(world);
		dwWorldClipModel->Link(clip);
	}
}
#endif

//=============================================================================
//
// hhGameLocal::RegisterTalonTarget
//
//=============================================================================
void hhGameLocal::RegisterTalonTarget( idEntity *ent ) {
	talonTargets.Append(ent);
}

//=============================================================================
//
// hhGameLocal::SpawnClientObject
// rww - meant for spawning client ents only.
//=============================================================================
idEntity* hhGameLocal::SpawnClientObject( const char* objectName, idDict* additionalArgs ) {
	idDict	localArgs;
	idDict* args = NULL;
	idEntity* object = NULL;
	bool clientEnt = true;

	if (!gameLocal.isClient) { //if it's the server spawning a client entity we want to spawn it for listen servers but keep it local
		clientEnt = false;
	}

	if( !objectName || !objectName[0]) {
		Error( "hhGameLocal::SpawnObject: Invalid object name\n" );
	}

	args = ( additionalArgs != NULL ) ? additionalArgs : &localArgs;

	args->Set( "classname", objectName );
	if( !SpawnEntityDef( *args, &object, true, clientEnt ) ) {
		Error( "hhGameLocal::SpawnObject: Failed to spawn %s\n", objectName );
	}

	if (object) {
		object->fl.clientEntity = clientEnt;
		object->fl.networkSync = false;
	}

	return object;
}

//=============================================================================
//
// hhGameLocal::SpawnObject
//
//=============================================================================
idEntity* hhGameLocal::SpawnObject( const char* objectName, idDict* additionalArgs ) {
	idDict	localArgs;
	idDict* args = NULL;
	idEntity* object = NULL;

	//rww - are you hitting this? then the code it's coming from is doing something it shouldn't be.
	//the client should never, ever call SpawnObject.
	assert(!gameLocal.isClient);

	if( !objectName || !objectName[0]) {
		Error( "hhGameLocal::SpawnObject: Invalid object name\n" );
	}

	args = ( additionalArgs != NULL ) ? additionalArgs : &localArgs;

	args->Set( "classname", objectName );
	if( !SpawnEntityDef( *args, &object ) ) {
		Error( "hhGameLocal::SpawnObject: Failed to spawn %s\n", objectName );
	}

	return object;
}


//
// hhGameLocal::RadiusDamage()
//	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
//
void hhGameLocal::RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower ) {
	float		distSquared, radiusSquared, damageScale, attackerDamageScale, attackerPushScale;
	idEntity *	ent;
	idEntity *	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	idBounds	bounds;
	idVec3 		v, damagePoint, dir;
	int			i, e, damage, radius, push;
	// HUMANHEAD AOB
	int			pushRadius;
	trace_t		result;
	// HUMANHEAD END

	const idDict *damageDef = FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}

	damageDef->GetInt( "damage", "0", damage ); // HUMANHEAD JRM - changed to ZERO not 20 default
	damageDef->GetInt( "radius", "50", radius );
	damageDef->GetInt( "push", va( "%d", damage * 100 ), push );
	damageDef->GetFloat( "attackerDamageScale", "0.5", attackerDamageScale );
	damageDef->GetFloat( "attackerPushScale", "0", attackerPushScale );

	// HUMANHEAD aob
	pushRadius = Max( 1.0f, damageDef->GetFloat("push_radius", va("%d", radius)) );
	if ( damageDef->GetBool( "nopush" ) ) {
		push = 0;
	}
	// HUMANHEAD END

	if ( radius < 1 ) {
		radius = 1;
	}
	radiusSquared = radius*radius;

	bounds = idBounds( origin ).Expand( radius );

	// get all entities touching the bounds
	numListedEntities = clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

	if ( inflictor && inflictor->IsType( idAFAttachment::Type ) ) {
		inflictor = static_cast<idAFAttachment*>(inflictor)->GetBody();
	}
	if ( attacker && attacker->IsType( idAFAttachment::Type ) ) {
		attacker = static_cast<idAFAttachment*>(attacker)->GetBody();
	}
	if ( ignoreDamage && ignoreDamage->IsType( idAFAttachment::Type ) ) {
		ignoreDamage = static_cast<idAFAttachment*>(ignoreDamage)->GetBody();
	}

	// apply damage to the entities
	if( damage > 0 )	{ // HUMANHEAD JRM - only do damage if WE HAVE damage
		for ( e = 0; e < numListedEntities; e++ ) {
			ent = entityList[ e ];
			assert( ent );

			if ( !ent->fl.takedamage ) {
				continue;
			}

			if ( ent == inflictor /*|| ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == inflictor )*/ ) {
				continue;
			}

			if ( ent == ignoreDamage /*|| ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == ignoreDamage )*/ ) {
				continue;
			}
			
			if ( ent->IsType( idAFAttachment::Type ) ) {	// bjk: no double splash damage from heads
				continue;
			}

			// don't damage a dead player
			if ( isMultiplayer && ent->entityNumber < MAX_CLIENTS && ent->IsType( idPlayer::Type ) && static_cast< idPlayer * >( ent )->health < 0 ) {
				continue;
			}
			
			// find the distance from the edge of the bounding box
			for ( i = 0; i < 3; i++ ) {
				if ( origin[ i ] < ent->GetPhysics()->GetAbsBounds()[0][ i ] ) {
					v[ i ] = ent->GetPhysics()->GetAbsBounds()[0][ i ] - origin[ i ];
				} else if ( origin[ i ] > ent->GetPhysics()->GetAbsBounds()[1][ i ] ) {
					v[ i ] = origin[ i ] - ent->GetPhysics()->GetAbsBounds()[1][ i ];
				} else {
					v[ i ] = 0;
				}
			}

			distSquared = v.LengthSqr();
			if ( distSquared >= radiusSquared ) {
				continue;
			}

			//HUMANHEAD: aob - see if radius damage is blocked.  CanDamage is used by more than just RadiusDamage
			clip.TracePoint( result, origin, (ent->GetPhysics()->GetAbsBounds()[0] + ent->GetPhysics()->GetAbsBounds()[1]) * 0.5f, CONTENTS_BLOCK_RADIUSDAMAGE, ignoreDamage );
			if( result.fraction < 1.0f && result.c.entityNum != ent->entityNumber ) {
				continue;
			}
			//HUMANHEAD END

			bool canDamage = true;
			if ( GERMAN_VERSION || g_nogore.GetBool() ) {
				if ( ent->IsType(idActor::Type) ) {
					idActor *actor = reinterpret_cast< idActor * > ( ent );
					if ( actor->IsActiveAF() && !actor->spawnArgs.GetBool( "not_gory", "0" ) ) {
						canDamage = false;
					}
				}
			}


			if ( canDamage && ent->CanDamage( origin, damagePoint ) ) {
				// push the center of mass higher than the origin so players
				// get knocked into the air more
				dir = ent->GetPhysics()->GetOrigin() - origin;
				dir[ 2 ] += 24;

				// get the damage scale
				damageScale = dmgPower * ( 1.0f - (distSquared / radiusSquared) );
				if ( ent == attacker || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == attacker ) ) {
					damageScale *= attackerDamageScale;
				}

				ent->Damage( inflictor, attacker, dir, damageDefName, damageScale, INVALID_JOINT );
			} 
		}
	} // HUMANHEAD END

	// push physics objects
	if ( push ) {
		//HUMANHEAD: aob - changed radius to pushRadius
		RadiusPush( origin, pushRadius, push * dmgPower, attacker, ignorePush, attackerPushScale, false );
	}
}

/*
==============
hhGameLocal::RadiusPush
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
==============
*/
void hhGameLocal::RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake ) {
	int i, numListedClipModels;
	idClipModel *clipModel;
	idClipModel *clipModelList[ MAX_GENTITIES ];
	idVec3 dir;
	idBounds bounds;
	modelTrace_t result;
	idEntity *ent;
	float scale;
	trace_t	trace; 	//HUMANHEAD: aob

	dir.Set( 0.0f, 0.0f, 1.0f );

	bounds = idBounds( origin ).Expand( radius );

	// get all clip models touching the bounds
	numListedClipModels = clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	if ( inflictor && inflictor->IsType( idAFAttachment::Type ) ) {
		inflictor = static_cast<const idAFAttachment*>(inflictor)->GetBody();
	}
	if ( ignore && ignore->IsType( idAFAttachment::Type ) ) {
		ignore = static_cast<const idAFAttachment*>(ignore)->GetBody();
	}

	// apply impact to all the clip models through their associated physics objects
	for ( i = 0; i < numListedClipModels; i++ ) {

		clipModel = clipModelList[i];

		// never push render models
		if ( clipModel->IsRenderModel() ) {
			continue;
		}

		ent = clipModel->GetEntity();

		// never push projectiles
		if ( ent->IsType( idProjectile::Type ) ) {
			continue;
		}

		// players use "knockback" in idPlayer::Damage
		if ( ent->IsType( idPlayer::Type ) && !quake ) {
			continue;
		}

		// don't push the ignore entity
		if ( ent == ignore || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == ignore ) ) {
			continue;
		}

		if ( gameRenderWorld->FastWorldTrace( result, origin, clipModel->GetAbsBounds().GetCenter() ) ) {
			continue;
		}

		// Don't affect ragdolls in non-gore mode
		if ( GERMAN_VERSION || g_nogore.GetBool() ) {
			if ( ent->IsType(idActor::Type) ) {
				idActor *actor = reinterpret_cast< idActor * > ( ent );
				if ( actor->IsActiveAF() && !actor->spawnArgs.GetBool( "not_gory", "0" ) ) {
					continue;
				}
			}
		}

		// scale the push for the inflictor
		if ( ent == inflictor || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == inflictor ) ) {
			scale = inflictorScale;
		} else if ( ent->IsType(hhMoveable::Type) ) {
			scale = ent->spawnArgs.GetFloat("radiusPush", "4.0");
		} else if( !ent->IsType(idActor::Type) ) {
			scale = 4.0f;
		} else {
			scale = 1.0f;
		}

		//HUMANHEAD: aob - see if radius damage is blocked
		clip.TracePoint( trace, origin, clipModel->GetEntity()->GetOrigin(), CONTENTS_BLOCK_RADIUSDAMAGE, ignore );
		if( trace.fraction < 1.0f && trace.c.entityNum != clipModel->GetEntity()->entityNumber ) {
			continue;
		}
		//HUMANHEAD END

		if ( quake ) {
			clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), scale * push * dir );
		} else {
			//RadiusPushClipModel( origin, scale * push, clipModel );

			//HUMANHEAD bjk
			idVec3 impulse = clipModel->GetAbsBounds().GetCenter() - origin;
			float dist = impulse.Normalize() / radius;
			impulse.z += 1.0f;
			dist = 0.6f - 0.3f*dist;
			impulse *= push * dist * scale;
			clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), impulse );
			//HUMANHEAD END
		}
	}
}

//HUMANHEAD rww
void hhGameLocal::LogitechLCDUpdate(void) {
	hhPlayer *pl = (hhPlayer *)GetLocalPlayer();
	if (!pl) {
		return;
	}

	DWORD buttons;
	sys->LGLCD_ReadSoftButtons(&buttons);
	if (logitechLCDButtonsLast != buttons && buttons) {
		logitechLCDDisplayAlt = !logitechLCDDisplayAlt;
	}
	logitechLCDButtonsLast = buttons;

	if (logitechLCDUpdateTime >= gameLocal.time) { //only update the screen at 20fps
		return;
	}
	logitechLCDUpdateTime = gameLocal.time + 50;

	sys->LGLCD_DrawBegin();
		if (logitechLCDDisplayAlt) { //primary/secondary ammo
			char *ammoStr =		va("%s%s", common->GetLanguageDict()->GetString("#str_41150"), common->GetLanguageDict()->GetString("#str_41152")); //"Ammo:      N/A";
			char *ammoAltStr =	va("%s%s", common->GetLanguageDict()->GetString("#str_41151"), common->GetLanguageDict()->GetString("#str_41152")); //"Alt. Ammo: N/A";
			if (pl->weapon.IsValid()) {
				int a = pl->weapon->AmmoAvailable();
				if (a >= 0) {
					if (a > 999) {
						a = 999;
					}
					ammoStr =			va("%s%i", common->GetLanguageDict()->GetString("#str_41150"), a); //"Ammo:      %i"
				}
				if (pl->weapon->GetAmmoType() != pl->weapon->GetAltAmmoType()) {
					a = pl->weapon->AltAmmoAvailable();
					if (a >= 0) {
						if (a > 999) {
							a = 999;
						}
						ammoAltStr =	va("%s%i", common->GetLanguageDict()->GetString("#str_41151"), a); //"Alt. Ammo: %i"
					}
				}
			}

			sys->LGLCD_DrawText(ammoStr, 46, 6, true);
			sys->LGLCD_DrawText(ammoAltStr, 46, 22, true);

			//draw the weapon icon (disabled for now, since it isn't very..recognizable)
			//sys->LGLCD_DrawShape(3, 54, 0, 0, 0, 0, true);
		}
		else { //health/spirit
			int v;
			v = pl->health;
			if (v > 999) {
				v = 999;
			}
			else if (v < 0) {
				v = 0;
			}
			sys->LGLCD_DrawText(va("%s%i", common->GetLanguageDict()->GetString("#str_41153"), v), 70, 6, true); //"Health: %i"
			v = pl->GetSpiritPower();
			if (v > 999) {
				v = 999;
			}
			else if (v < 0) {
				v = 0;
			}
			sys->LGLCD_DrawText(va("%s%i", common->GetLanguageDict()->GetString("#str_41154"), v), 70, 22, true); //"Spirit: %i"

			//draw the tommy and talon
			float eyePitch;
			if (pl->InVehicle()) {
				eyePitch = 0.0f;
			}
			else {
				eyePitch = pl->GetEyeAxis()[2].ToAngles().pitch+90.0f;
				if (eyePitch < 1.0f && eyePitch > -1.0f) {
					eyePitch = 0.0f;
				}
			}
			sys->LGLCD_DrawShape(2, 42, 0, 0, 0, (int)eyePitch, true);
		}

		//draw the compass
		sys->LGLCD_DrawShape(0, 0, 0, 0, 0, 0, false); //base
		sys->LGLCD_DrawShape(1, 21, 21, 13, 1, (int)-pl->GetViewAngles().yaw, true); //line
	sys->LGLCD_DrawFinish(false);
}
//HUMANHEAD END

//
// RunFrame()
//
//	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
gameReturn_t hhGameLocal::RunFrame( const usercmd_t *clientCmds ) {
	idEntity *	ent;
	int			num;
	float		ms;
	idTimer		timer_think, timer_events, timer_singlethink;
	gameReturn_t ret;
	idPlayer	*player;
	const renderView_t *view;
	// HUMANHEAD pdm
	idTimer		timer_singledormant;
	bool		dormant;
	const float thinkalpha = 0.98f;	// filter with historical timings
	// HUMANHEAD END

	ret.sessionCommand[0] = 0;

#ifdef _DEBUG
	if ( isMultiplayer ) {
		assert( !isClient );
	}
#endif

	player = GetLocalPlayer();

	if ( !isMultiplayer && g_stopTime.GetBool() ) {
		// clear any debug lines from a previous frame
		gameRenderWorld->DebugClearLines( time + 1 );

		// set the user commands for this frame
		memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

		// Fake these categories so it still displays any that are in the history when time is stopped
		{	PROFILE_SCOPE("Misc_Think", PROFMASK_NORMAL);		}
		{	PROFILE_SCOPE("Dormant Tests", PROFMASK_NORMAL);	}
		{	PROFILE_SCOPE("Sound", PROFMASK_NORMAL);			}
		{	PROFILE_SCOPE("Animation", PROFMASK_NORMAL);		}
		{	PROFILE_SCOPE("Scripting", PROFMASK_NORMAL);		}

		if ( player ) {
			player->Think();
			if ( player->InVehicle() && player->GetVehicleInterface() ) {
				if ( player->GetVehicleInterface()->GetVehicle() ) {
					player->GetVehicleInterface()->GetVehicle()->Think();
				}
			}
		}
	} else do {
		// update the game time
		framenum++;
		previousTime = time;
		time += msec;
		realClientTime = time;
		timeRandom = time; //HUMANHEAD rww

#ifdef GAME_DLL
		// allow changing SIMD usage on the fly
		if ( com_forceGenericSIMD.IsModified() ) {
			idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );
		}
#endif

		// make sure the random number counter is used each frame so random events
		// are influenced by the player's actions
		random.RandomInt();

		if ( player ) {
			// update the renderview so that any gui videos play from the right frame
			view = player->GetRenderView();
			if ( view ) {
				gameRenderWorld->SetRenderView( view );
			}
		}

		// clear any debug lines from a previous frame
		gameRenderWorld->DebugClearLines( time );

		// clear any debug polygons from a previous frame
		gameRenderWorld->DebugClearPolygons( time );

		// set the user commands for this frame
		memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

		// free old smoke particles
		smokeParticles->FreeSmokes();

		// process events on the server
		ServerProcessEntityNetworkEventQueue();

		// update our gravity vector if needed.
		UpdateGravity();

		// create a merged pvs for all players
		SetupPlayerPVS();

		// sort the active entity list
		SortActiveEntityList();

		timer_think.Clear();
		timer_think.Start();
		PROFILE_START("Misc_Think", PROFMASK_NORMAL);	// HUMANHEAD pdm

		// HUMANHEAD pdm: This loop reworked to support debugger and dormant timings
		// let entities think
		if ( g_timeentities.GetFloat() || g_debugger.GetInteger() || g_dormanttests.GetBool()) {
			num = 0;
			for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
				if ( g_cinematic.GetBool() && inCinematic && !ent->cinematic ) {
					ent->GetPhysics()->UpdateTime( time );
					continue;
				}
				dormant = false;
				timer_singledormant.Clear();
				if (!ent->fl.neverDormant) {
					timer_singledormant.Start();
					dormant = ent->CheckDormant();
					timer_singledormant.Stop();
				}

				timer_singlethink.Clear();
				if( !dormant ) {
					timer_singlethink.Start();
					ent->Think();
					timer_singlethink.Stop();
				}

				ms = timer_singlethink.Milliseconds();
				if ( g_timeentities.GetFloat() && ms >= g_timeentities.GetFloat() ) {
					gameLocal.Printf( "%d: entity '%s': %.1f ms\n", time, ent->name.c_str(), ms );
				}
				if (g_debugger.GetInteger() || g_dormanttests.GetBool()) {
					ent->thinkMS = ent->thinkMS*thinkalpha + (1.0-thinkalpha)*ms;
				}
				if (g_dormanttests.GetBool() && !ent->fl.neverDormant) {
					float msDormant = timer_singledormant.Milliseconds();
					ent->dormantMS = ent->dormantMS*thinkalpha + (1.0f-thinkalpha)*msDormant;

					if (ent->dormantMS > ent->thinkMS && !dormant) {
						// If we're spending more time on dormant checks than on actually thinking,
						// turn dormant checks off on that class.
						Printf("%30s Dms=%6.4f Tms=%6.4f\n", ent->GetClassname(), ent->dormantMS, ent->thinkMS);
					}
				}
				num++;
			}
		} else {
			if ( inCinematic ) {
				num = 0;
				for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
					if ( g_cinematic.GetBool() && !ent->cinematic ) {
						ent->GetPhysics()->UpdateTime( time );
						continue;
					}
					// HUMANHEAD JRM
					if( !ent->CheckDormant() ) {
						ent->Think();
					}
					num++;
				}
			} else {
				num = 0;
				for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
					// HUMANHEAD JRM
					if( !ent->CheckDormant() ) {
						ent->Think();
					}
					num++;
				}
			}
		}

		// remove any entities that have stopped thinking
		if ( numEntitiesToDeactivate ) {
			idEntity *next_ent;
			int c = 0;
			for( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
				next_ent = ent->activeNode.Next();
				if ( !ent->thinkFlags ) {
					ent->activeNode.Remove();
					c++;
				}
			}
			//assert( numEntitiesToDeactivate == c );
			numEntitiesToDeactivate = 0;
		}

		PROFILE_STOP("Misc_Think", PROFMASK_NORMAL);

		timer_think.Stop();
		timer_events.Clear();
		timer_events.Start();

		// service any pending events
		idEvent::ServiceEvents();

		timer_events.Stop();

		// free the player pvs
		FreePlayerPVS();

		// do multiplayer related stuff
		if ( isMultiplayer ) {
			mpGame.Run();
		}

		// display how long it took to calculate the current game frame
		if ( g_frametime.GetBool() ) {
			Printf( "game %d: all:%.1f th:%.1f ev:%.1f %d ents \n",
				time, timer_think.Milliseconds() + timer_events.Milliseconds(),
				timer_think.Milliseconds(), timer_events.Milliseconds(), num );
		}

		// build the return value		
		ret.consistencyHash = 0;
		ret.sessionCommand[0] = 0;

		if ( !isMultiplayer && player ) {
			ret.health = player->health;
			ret.heartRate = 0;	//player->heartRate;	// HUMANHEAD pdm: not used
			ret.stamina = idMath::FtoiFast( player->stamina );
			// combat is a 0-100 value based on lastHitTime and lastDmgTime
			// each make up 50% of the time spread over 10 seconds
			ret.combat = 0;
			if ( player->lastDmgTime > 0 && time < player->lastDmgTime + 10000 ) {
				ret.combat += 50.0f * (float) ( time - player->lastDmgTime ) / 10000;
			}
			if ( player->lastHitTime > 0 && time < player->lastHitTime + 10000 ) {
				ret.combat += 50.0f * (float) ( time - player->lastHitTime ) / 10000;
			}
		}

		// see if a target_sessionCommand has forced a changelevel
		if ( sessionCommand.Length() ) {
			strncpy( ret.sessionCommand, sessionCommand, sizeof( ret.sessionCommand ) );
			break;
		}

		// make sure we don't loop forever when skipping a cinematic
		if ( skipCinematic && ( time > cinematicMaxSkipTime ) ) {
			Warning( "Exceeded maximum cinematic skip length.  Cinematic may be looping infinitely." );
			skipCinematic = false;
			break;
		}
	} while( ( inCinematic || ( time < cinematicStopTime ) ) && skipCinematic );

	ret.syncNextGameFrame = skipCinematic;
	if ( skipCinematic ) {
		soundSystem->SetMute( false );
		skipCinematic = false;		
	}

	// show any debug info for this frame
	RunDebugInfo();
	D_DrawDebugLines();

	//HUMANHEAD rww
	if (logitechLCDEnabled) {
		PROFILE_START("LogitechLCDUpdate", PROFMASK_NORMAL);
		LogitechLCDUpdate();
		PROFILE_STOP("LogitechLCDUpdate", PROFMASK_NORMAL);
	}
	//HUMANHEAD END

	return ret;
}

//	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
bool hhGameLocal::Draw( int clientNum ) {
	//HUMANHEAD rww - dedicated server update
	if (clientNum == -1) {
		gameSoundWorld->PlaceListener( vec3_origin, mat3_identity, -1, gameLocal.time, "Undefined" );
		return false;
	}
	//HUMANHEAD END

	if ( isMultiplayer ) {
		return mpGame.Draw( clientNum );
	}

	//HUMANHEAD: aob - changed idPlayer to hhPlayer
	hhPlayer *player = static_cast<hhPlayer *>(entities[ clientNum ]);

	if ( !player ) {
		return false;
	}

	// render the scene
	// HUMANHEAD pdm: added case of vehicle determining the player hud
	if (player->InVehicle()) {
		player->playerView.RenderPlayerView( player->GetVehicleInterfaceLocal()->GetHUD() );
	}
	else {
		player->playerView.RenderPlayerView( player->hud );
	}
	// HUMANHEAD END

#if INGAME_DEBUGGER_ENABLED					// HUMANHEAD pdm
	debugger.UpdateDebugger();
#endif

	return true;
}


float hhGameLocal::GetTimeScale() const {
	return hhMath::ClampFloat( VECTOR_EPSILON, 10.0f, cvarSystem->GetCVarFloat("timescale") );
}

//
// SimpleMonstersWithinRadius()
// TODO: Use this exclusively and remove MonstersWithinRadius when old AI is removed
int hhGameLocal::SimpleMonstersWithinRadius( const idVec3 org, float radius, idAI **monstList, int maxCount ) const {
	idBounds bo( org );
	int entCount = 0;

	bo.ExpandSelf( radius );
	for(int i=0;i<hhMonsterAI::allSimpleMonsters.Num();i++) {
		if ( hhMonsterAI::allSimpleMonsters[i]->GetPhysics()->GetAbsBounds().IntersectsBounds( bo ) ) {
			monstList[entCount++] = hhMonsterAI::allSimpleMonsters[i];
		}
	}

	return entCount;
}

//
// SpawnMapEntities()
//
void hhGameLocal::SpawnMapEntities( void ) {
	if ( reactionHandler ) {
		delete reactionHandler;
	}
	reactionHandler	= new hhReactionHandler;
	idGameLocal::SpawnMapEntities();
}

/*
==============
hhGameLocal::SendMessageAI
==============
*/
void hhGameLocal::SendMessageAI( const idEntity* entity, const idVec3& origin, float radius, const idEventDef& message ) {
	idAI *simplemonsters[ MAX_GENTITIES ];

	if( radius < VECTOR_EPSILON || isClient ) {
		return;
	}
	for( int i = gameLocal.SimpleMonstersWithinRadius(origin, radius, simplemonsters) - 1; i >= 0; --i ) {
		if( simplemonsters[i]->GetHealth() > 0 && simplemonsters[i]->IsActive() && !simplemonsters[i]->IsHidden() ) {
			simplemonsters[i]->ProcessEvent( &message, entity );
		}
	}
}

/*
==============
hhGameLocal::MatterTypeToMatterName
==============
*/
const char* hhGameLocal::MatterTypeToMatterName( surfTypes_t type ) const {
	return sufaceTypeNames[ type ];
}

/*
==============
hhGameLocal::MatterTypeToMatterKey
==============
*/
const char* hhGameLocal::MatterTypeToMatterKey( const char* prefix, surfTypes_t type ) const {
	return va( "%s_%s", prefix, MatterTypeToMatterName(type) );
}

/*
==============
hhGameLocal::MatterNameToMatterType
==============
*/
surfTypes_t hhGameLocal::MatterNameToMatterType( const char* name ) const {
	surfTypes_t type = SURFTYPE_NONE;

	for( int ix = 0; ix < MAX_SURFACE_TYPES; ++ix ) {
		type = (surfTypes_t)ix;
		if( idStr::Icmp(name, MatterTypeToMatterName(type)) ) {
			continue;
		}

		return type;
	}

	return SURFTYPE_NONE;
}

/*
==============
hhGameLocal::GetMatterType
==============
*/
surfTypes_t hhGameLocal::GetMatterType( const trace_t& trace, const char* descriptor ) const {
	return GetMatterType( gameLocal.GetTraceEntity(trace), trace.c.material, descriptor );
}

/*
==============
hhGameLocal::GetMatterType
==============
*/
surfTypes_t hhGameLocal::GetMatterType( const idEntity *ent, const idMaterial *material, const char* descriptor ) const {
	surfTypes_t type = SURFTYPE_NONE;
	const char *matterName = NULL;
	const idMaterial *remapped = material;

	// If the entityDef has a matter specified, always use it
	// Otherwise, use the materials matter.  If none, default to metal.
	if( ent && ent->spawnArgs.GetString("matter", NULL, &matterName ) ) {
		type = MatterNameToMatterType( matterName );
	}
	else {
		if (ent && ent->GetSkin()) {
			remapped = ent->GetSkin()->RemapShaderBySkin(material);
		}
		type = (remapped != NULL) ? remapped->GetSurfaceType() : SURFTYPE_METAL;
	}

	// OBS: Will never happen
	if( !type ) {
#if 0
		Warning("No matter for hit surface");
		Warning(" Entity: %s", ent ? ent->name.c_str() : "none");
		Warning(" Class: %s", ent ? ent->GetClassname() : "none");
		Warning(" Material: %s", material ? material->GetName() : "none");
#endif
		type = SURFTYPE_METAL;
	}

	if( g_debugMatter.GetInteger() > 0 && descriptor && descriptor[0] ) {
		Printf("%s: [%s] ent=[%s] mat=[%s]\n",
			descriptor,
			MatterTypeToMatterName(type),
			ent ? ent->GetName() : "none",
			material ? material->GetName() : "none");
	}

	return type;
}

void hhGameLocal::AlertAI( idEntity *ent ) {
	if ( ent && ent->IsType( idActor::Type ) ) {
		// alert them for the next frame
		lastAIAlertTime = time + msec;
		lastAIAlertEntity = static_cast<idActor *>( ent );
		lastAIAlertRadius = 0;			//no radius required by default
	}
}

void hhGameLocal::AlertAI( idEntity *ent, float radius ) {
	if ( ent && ent->IsType( idActor::Type ) ) {
		// alert them for the next frame
		lastAIAlertTime = time + msec;
		lastAIAlertEntity = static_cast<idActor *>( ent );
		lastAIAlertRadius = radius;		//radius of effect
	}
}

//================
//hhGameLocal::Save
//================
void hhGameLocal::Save( idSaveGame *savefile ) const {
	int i, num = talonTargets.Num();
	savefile->WriteInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->WriteObject( talonTargets[i] );
	}

	reactionHandler->Save( savefile );
	savefile->WriteClipModel( dwWorldClipModel );
	//HUMANHEAD rww
#if _HH_INLINED_PROC_CLIPMODELS
	savefile->WriteInt(inlinedProcClipModels.Num());
	for (i = 0; i < inlinedProcClipModels.Num(); i++) {
		savefile->WriteClipModel(inlinedProcClipModels[i]);
	}
#endif
	//HUMANHEAD END
	savefile->WriteVec3( gravityNormal );
	savefile->WriteObject( sunCorona );
	ddaManager->Save( savefile );
	savefile->WriteBool( bIsLOTA );
	savefile->WriteFloat( lastAIAlertRadius );

	num = staticClipModels.Num();
	savefile->WriteInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->WriteClipModel( staticClipModels[i] );
	}

	num = staticRenderEntities.Num();
	savefile->WriteInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->WriteRenderEntity( *staticRenderEntities[i] );
	}

	savefile->WriteInt( hands.Num() );
	for ( i = 0; i < hands.Num(); i++ ) {
		hands[i].Save( savefile );
	}
}

//================
//hhGameLocal::Restore
//================
void hhGameLocal::Restore( idRestoreGame *savefile ) {
	idEntity *ent;
	int i, num;
	idClipModel *model;
	renderEntity_t *renderEnt;
	savefile->ReadInt( num );
	talonTargets.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&> ( ent ) );
		talonTargets[i] = ent;
	}

	reactionHandler	= new hhReactionHandler;
	reactionHandler->Restore( savefile );
	savefile->ReadClipModel( dwWorldClipModel );
	//HUMANHEAD rww
#if _HH_INLINED_PROC_CLIPMODELS
	int numInlinedProcClipModels;
	savefile->ReadInt(numInlinedProcClipModels);
	for (i = 0; i < numInlinedProcClipModels; i++) {
		savefile->ReadClipModel(inlinedProcClipModels[i]);
	}
#endif
	//HUMANHEAD END
	savefile->ReadVec3( gravityNormal );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( sunCorona ) );
	ddaManager->Restore ( savefile );
	savefile->ReadBool( bIsLOTA );
	savefile->ReadFloat( lastAIAlertRadius );

	savefile->ReadInt( num );
	staticClipModels.DeleteContents( false );
	staticClipModels.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadClipModel( model );
		staticClipModels[i] = model;
	}

	savefile->ReadInt( num );
	staticRenderEntities.DeleteContents( false );
	staticRenderEntities.SetNum( num );
	for( i = 0; i < num; i++ ) {
		renderEnt = new renderEntity_t;
		savefile->ReadRenderEntity( *renderEnt );
		gameRenderWorld->AddEntityDef( renderEnt );
		staticRenderEntities[i] = renderEnt;
	}

	savefile->ReadInt( num );
	hands.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		hands[i].Restore( savefile );
	}
}

bool hhGameLocal::InhibitEntitySpawn( idDict &spawnArgs ) {
	const char *modelName = spawnArgs.GetString( "model" );
	int inlineEnt = spawnArgs.GetInt( "inline" );

	if( idStr::Icmp( spawnArgs.GetString( "classname", NULL ), "func_static" ) == 0 &&	// Only deal with func_static objects
        !spawnArgs.GetBool( "neverInline", "0" ) &&										// and we're not flagged neverInline
		( inlineEnt != 0 ||																// and we're flagged explicit inline
		  world->spawnArgs.GetBool( "inlineAllStatics" ) ) ) {							//     or we're inlining all statics

#if _HH_INLINED_PROC_CLIPMODELS
		if (inlineEnt != 3) { //HUMANHEAD rww
#endif
			// Handle explicit clip models
			idClipModel *model = NULL;
			const char *temp = spawnArgs.GetString( "clipmodel", NULL );

			if( temp ) {
				if ( idClipModel::CheckModel( temp ) ) {
					model = new idClipModel( temp );
				}
			}

			if( model || !spawnArgs.GetBool( "noclipmodel", "0" ) ) {
				// Get the origin and axis of the object
				idMat3	axis;
				// get the rotation matrix in either full form, or single angle form
				if ( !spawnArgs.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) ) {
					float angle = spawnArgs.GetFloat( "angle" );
					if ( angle != 0.0f ) {
						axis = idAngles( 0.0f, angle, 0.0f ).ToMat3();
					} else {
						axis.Identity();
					}
				}		

				idVec3	origin = spawnArgs.GetVector( "origin" );

				// Create a clip model for the static object and position it correctly
				if( !model ) {
					model = new idClipModel( spawnArgs.GetString( "model" ) );
				}
				if( spawnArgs.GetBool( "bulletsOnly", "0" ) ) {
					model->SetContents( CONTENTS_SHOOTABLE|CONTENTS_SHOOTABLEBYARROW );
				} else if( spawnArgs.GetBool( "solid", "1" ) ) {
					model->SetContents( CONTENTS_SOLID );
				} else {
					model->SetContents( 0 );
				}
				model->SetPosition( origin, axis );
				model->SetEntity( world );
				model->Link( clip );
				staticClipModels.Append( model );
			}
#if _HH_INLINED_PROC_CLIPMODELS
		}
#endif

		if ( inlineEnt == 1 ) { // This means we must hand the model, too
			renderEntity_t *renderEnt = new renderEntity_t;
			gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, renderEnt );
			HH_ASSERT( renderEnt->hModel && !renderEnt->callback && renderEnt->shaderParms[ SHADERPARM_ANY_DEFORM ] == DEFORMTYPE_NONE ); // Inlined statics can't have a callback
			renderEnt->entityNum = 0; // WorldSpawn
			gameRenderWorld->AddEntityDef( renderEnt );
			staticRenderEntities.Append( renderEnt );
		}

		// Don't spawn an entity for an inlined static
		return true;
	} else {
		return idGameLocal::InhibitEntitySpawn( spawnArgs );
	}
}

//HUMANHEAD rww
#if _HH_INLINED_PROC_CLIPMODELS
void hhGameLocal::CreateInlinedProcClip(idEntity *clipOwner) {
	assert(world);
	assert(inlinedProcClipModels.Num() == 0); //should never have existing models when this is called
	int n = collisionModelManager->GetNumInlinedProcClipModels();
	if (n <= 0) { //nothing to work with, leave
		return;
	}

	for (int i = 0; i < n; i++) {
		idClipModel *chunk = new idClipModel(va("%s%i", PROC_CLIPMODEL_STRING_PRFX, i));
		chunk->SetContents(CONTENTS_SOLID);
		chunk->SetEntity(clipOwner);
		chunk->Link(clip);
		inlinedProcClipModels.Append(chunk);
	}
}
#endif
//HUMANHEAD END

// Finds entities matching a specified type.  If last is NULL it starts at the beginning of the list.
// If last is valid, starts looking at that entities position in the list
idEntity *hhGameLocal::FindEntityOfType(const idTypeInfo &type, idEntity *last) {
	idEntity *ent;
	if (last) {
		ent = last->activeNode.Next();
	}
	else {
		ent = activeEntities.Next();
	}

	for( ; ent != NULL; ent = ent->activeNode.Next() ) {
		if (ent->IsType(type)) {
			return ent;
		}
	}
	return NULL;
}

float hhGameLocal::GetDDAValue( void ) {
	if ( !ddaManager ) {
		return 0.5;
	} else {
		return ddaManager->GetDifficulty();
	}	
}


void hhGameLocal::ClearStaticData( void ) {
	delete dwWorldClipModel; //rww - this needs to be done as well, dw clipmodel expects world as owner.
	dwWorldClipModel = NULL;

#if _HH_INLINED_PROC_CLIPMODELS
	inlinedProcClipModels.DeleteContents(true); //HUMANHEAD rww
#endif

	staticClipModels.DeleteContents( true ); // Clear any inlined static clip models
	staticRenderEntities.DeleteContents( true ); // Clear any inlined static render entities
}

float hhGameLocal::TimeBasedRandomFloat(void) {
	if (gameLocal.isMultiplayer) { //rand based on time step
		timeRandom = (1103515245 * timeRandom + 12345)%(1<<31);
		return (timeRandom)/( float )( (1<<31) + 1 );
	}
	else { //give sp complete randomness
		return random.RandomFloat();
	}
}

bool hhGameLocal::PlayerIsDeathwalking(void) {
	HH_ASSERT( !isMultiplayer );
	hhPlayer *player = static_cast<hhPlayer *> (GetLocalPlayer());
	HH_ASSERT( player );
	return player->IsDeathWalking();
}


// Abstraction to hide the ugly code for text exchange between engine and game
void hhGameLocal::GetTip(const char *binding, idStr &keyMaterialString, idStr &keyString, bool &isWide) {

	char keyMaterial[256];	// No passing idStr between game and engine
	char key[256];			// No passing idStr between game and engine
	keyMaterial[0] = '\0';
	key[0] = '\0';
	if (binding) {
		common->MaterialKeyForBinding(binding, keyMaterial, key, isWide);
		keyMaterialString = keyMaterial;
		keyString = key;
	}
}

/*
	hhGameLocal::SetTip

		gui			gui to display tip
		binding		key binding to display associated key
		tip			textual tip to display
		top			optional top image
		mtr			optional material, overrides binding image
		prefix		optional prefix for gui state keys
*/
bool hhGameLocal::SetTip(idUserInterface* gui, const char *binding, const char *tip, const char *topMaterial, const char *overrideMaterial, const char *prefix) {
	assert(gui);

	idStr keyMaterial, key;
	bool keywide = false;
	if ( !spawnArgs.GetString("mtr_override", "", keyMaterial) ) {
		if (binding) {
			GetTip(binding, keyMaterial, key, keywide);
		}
	}

	const char *translated = common->GetLanguageDict()->GetString(tip);

	if (prefix != NULL) {
		gui->SetStateBool( va("%s_keywide", prefix), keywide );
		gui->SetStateString( va("%s_tip", prefix), translated ? translated : "" );
		gui->SetStateString( va("%s_key", prefix), key.c_str() );
		gui->SetStateString( va("%s_keyMaterial", prefix), keyMaterial.c_str() );
		gui->SetStateString( va("%s_topMaterial", prefix), topMaterial ? topMaterial : "" );
	}
	else {
		gui->SetStateBool( "keywide", keywide );
		gui->SetStateString( "tip", translated ? translated : "" );
		gui->SetStateString( "key", key.c_str() );
		gui->SetStateString( "keyMaterial", keyMaterial.c_str() );
		gui->SetStateString( "topMaterial", topMaterial ? topMaterial : "" );
	}

	return (keyMaterial.Length() > 0);
}

