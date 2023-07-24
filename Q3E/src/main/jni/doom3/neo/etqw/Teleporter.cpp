// Copyright (C) 2007 Id Software, Inc.
//
#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Teleporter.h"
#include "structures/TeamManager.h"
#include "Trigger.h"
#include "Player.h"
#include "vehicles/Transport.h"
#include "vehicles/VehicleControl.h"
#include "vehicles/JetPack.h"

/*
===============================================================================

	sdTeleporter

===============================================================================
*/
const idEventDef EV_EnableTeam( "enableTeam", '\0', DOC_TEXT( "Allows the given team to use the teleporter." ), 1, "An error will be thrown if the team name is invalid.", "s", "name", "Name of the team." );
const idEventDef EV_DisableTeam( "disableTeam", '\0', DOC_TEXT( "Disables the teleporter for the given team." ), 1, "An error will be thrown if the team name is invalid.", "s", "name", "Name of the team." );
const idEventDefInternal EV_FinishTeleport( "internal_finishTeleport", "h" );

CLASS_DECLARATION( idEntity, sdTeleporter )
	EVENT( EV_EnableTeam,		sdTeleporter::Event_EnableTeam )
	EVENT( EV_DisableTeam,		sdTeleporter::Event_DisableTeam )
	EVENT( EV_FinishTeleport,	sdTeleporter::Event_FinishTeleport )
END_CLASS

/*
================
sdTeleporter::sdTeleporter
================
*/
sdTeleporter::sdTeleporter( void ) {
}

/*
================
sdTeleporter::~sdTeleporter
================
*/
sdTeleporter::~sdTeleporter( void ) {
}

/*
================
sdTeleporter::Spawn
================
*/
void sdTeleporter::Spawn( void ) {
	BecomeActive( TH_THINK );

	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

	teamInfo.SetNum( manager.GetNumTeams() );

	for ( int i = 0; i < manager.GetNumTeams(); i++ ) {
		sdTeamInfo& team = manager.GetTeamByIndex( i );

		// default to enabled
		teamInfo[ i ].enabled = spawnArgs.GetBool( va( "%s_starts_enabled", team.GetLookupName() ), "1" );
	}

	delay			= spawnArgs.GetInt( "delay" );
	exitVelocity	= spawnArgs.GetVector( "velocity_exit", "100 0 0" );

	deployReverse	= spawnArgs.GetFloat( "deploy_reverse" );
	deployLength	= spawnArgs.GetFloat( "deploy_length" );
	deployWidth		= spawnArgs.GetFloat( "deploy_width" );

	telefragDamage	= DAMAGE_FOR_NAME( spawnArgs.GetString( "dmg_telefrag" ) );

	// set up the physics
	const char* triggerModel = spawnArgs.GetString( "cm_trigger" );
	staticPhysics.SetSelf( this );
	staticPhysics.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	if ( triggerModel != NULL && *triggerModel ) {
		// the trigger for the physics
		staticPhysics.SetClipModel( new idClipModel( triggerModel ), 1.0f, 1 );
	}
	staticPhysics.SetOrigin( GetPhysics()->GetOrigin() );
	staticPhysics.SetAxis( GetPhysics()->GetAxis() );
	SetPhysics( &staticPhysics );
}

/*
================
sdTeleporter::Think
================
*/
void sdTeleporter::Think( void ) {
 	const idBounds& myBounds = GetPhysics()->GetAbsBounds();

	for ( int i = 0; i < latches.Num(); ) {
		idEntity* ent = latches[ i ];
		if ( ent == NULL ) {
			latches.RemoveIndexFast( i );
			continue;
		}

		// check if its still touching
		if ( GetPhysics()->GetNumClipModels() < 2 ) {
			continue;
		}
		idClipModel* triggerCM = GetPhysics()->GetClipModel( 1 );

		// check all the collision models of the other against our trigger model
		int numClipModels = ent->GetPhysics()->GetNumClipModels();
		int contents = 0;
		for ( int j = 0; j < numClipModels; j++ ) {
			idClipModel* otherCM = ent->GetPhysics()->GetClipModel( j );

			contents |= gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS otherCM->GetOrigin(), otherCM, otherCM->GetAxis(), -1,
													triggerCM, triggerCM->GetOrigin(), triggerCM->GetAxis() );
		}

		if ( !contents ) {
			latches.RemoveIndexFast( i );
			continue;
		}

		i++;
	}

	if ( latches.Num() == 0 ) {
		BecomeInactive( TH_THINK );
	}
}

/*
================
sdTeleporter::Latch
================
*/
void sdTeleporter::Latch( idEntity* ent ) {
	latches.Alloc() = ent;
	BecomeActive( TH_THINK );
}

/*
================
sdTeleporter::PostMapSpawn
================
*/
void sdTeleporter::PostMapSpawn( void ) {
	storageLocation	= gameLocal.FindEntity( spawnArgs.GetString( "storage_entity" ) );
	viewLocation	= gameLocal.FindEntity( spawnArgs.GetString( "view_entity" ) );

	idEntity::PostMapSpawn();
}

/*
================
sdTeleporter::Event_EnableTeam
================
*/
void sdTeleporter::Event_EnableTeam( const char* team ){
	int teamIndex = sdTeamManager::GetInstance().GetTeam( team ).GetIndex();
	teamInfo[ teamIndex ].enabled = true;
}

/*
================
sdTeleporter::Event_DisableTeam
================
*/
void sdTeleporter::Event_DisableTeam( const char* team ) {
	int teamIndex = sdTeamManager::GetInstance().GetTeam( team ).GetIndex();
	teamInfo[ teamIndex ].enabled = false;
}

/*
================
sdTeleporter::Event_FinishTeleport
================
*/
void sdTeleporter::Event_FinishTeleport( int spawnId ) {
	FinishTeleport( gameLocal.EntityForSpawnId( spawnId ) );
}

/*
================
sdTeleporter::OnTouch
================
*/
void sdTeleporter::OnTouch( idEntity *other, const trace_t& trace ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( player != NULL && !player->IsSpectator() && player->GetHealth() <= 0 ) {
		return;
	}

	// abort if it didn't touch the trigger model
	if ( trace.c.id != 1 ) {
		return;
	}

	// abort if there is no target
	if ( targets.Num() == 0 ) {
		return;
	}

	bool teamValid = true;

	// abort if the other is on a disabled team
	sdTeamInfo* otherTeam = other->GetGameTeam();
	if ( otherTeam != NULL ) {
		if ( !teamInfo[ otherTeam->GetIndex() ].enabled ) {
			teamValid = false;

			idPlayer* otherPlayer = other->Cast< idPlayer >();
			if ( otherPlayer != NULL ) {
				if ( otherPlayer->IsDisguised() ) {
					sdTeamInfo* disguiseTeam = otherPlayer->GetDisguiseTeam();
					if ( disguiseTeam != NULL ) {
						if ( teamInfo[ disguiseTeam->GetIndex() ].enabled ) {
							teamValid = true;
						}
					}
				}
			}
		}
	}

	if ( !teamValid ) {
		return;
	}

	if ( !other->AllowTeleport() ) {
		return;
	}

	// check if this entity has already been touched
	if ( latches.FindIndex( other ) != -1 ) {
		return;
	}

	const idVec3& myPosition = GetPhysics()->GetOrigin();
	const idMat3& myAxes = GetAxis();

	// only allow the entity to teleport if its moving INTO the teleporter
	float moveDir = other->GetPhysics()->GetLinearVelocity() * myAxes[ 0 ];
	if ( moveDir > 0.0f ) {
		return;
	}


	Latch( other );

	idEntity *targetEntity = targets[ 0 ].GetEntity();

	// get the positions of the target and the latch
	idVec3 targetPosition = targetEntity->GetPhysics()->GetOrigin();
	idMat3 targetAxes = targetEntity->GetAxis();

	// find the target teleporter
	// TODO: Make it an entity key on the target entity
	sdTeleporter* targetTeleporter = NULL;
	idClass* typeInstance = Type.instances.Next();
	for ( ; typeInstance; typeInstance = typeInstance->GetInstanceNode()->Next() ) {
		sdTeleporter* testTeleporter = reinterpret_cast< sdTeleporter* >( typeInstance );
		idBounds bounds = testTeleporter->GetPhysics()->GetAbsBounds();
		if ( bounds.ContainsPoint( targetPosition ) ) {
			targetTeleporter = testTeleporter;
		}
	}

	if ( targetTeleporter != NULL ) {
		targetTeleporter->Latch( other );
	}


	// modify the position & angles if its a vehicle that should land on the ramp
	sdTransport* transportOther = other->Cast< sdTransport >();
	idPlayer* playerOther = other->Cast< idPlayer >();
	sdJetPack* jetPackOther = other->Cast< sdJetPack >();

	bool shouldTraceDown = false;
	bool shouldOrientToRamp = false;
	bool shouldOrientBackTrace = false;
	float traceDownOffset = 0.0f;
	if ( transportOther != NULL && transportOther->GetVehicleControl() != NULL ) {
		shouldTraceDown = transportOther->GetVehicleControl()->TeleportOntoRamp();
		shouldOrientToRamp = transportOther->GetVehicleControl()->TeleportOntoRampOriented();
		shouldOrientBackTrace = transportOther->GetVehicleControl()->TeleportBackTraceOriented();
		traceDownOffset = transportOther->GetVehicleControl()->TeleportOntoRampHeightOffset();
	}
	if ( playerOther != NULL || jetPackOther != NULL ) {
		shouldTraceDown = true;
	}

	if ( shouldOrientToRamp ) {
		// find the normal of the surface
		trace_t downTrace;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS downTrace, targetPosition + idVec3( 0.0f, 0.0f, 128.0f ), targetPosition - idVec3( 0.0f, 0.0f, 512.0f ), CONTENTS_MOVEABLECLIP, NULL );

		// find the new target axes using that
		idMat3 newTargetAxes;
		newTargetAxes[ 2 ] = downTrace.c.normal;
		newTargetAxes[ 1 ] = newTargetAxes[ 2 ].Cross( targetAxes[ 0 ] );
		newTargetAxes[ 1 ].Normalize();
		newTargetAxes[ 0 ] = newTargetAxes[ 1 ].Cross( newTargetAxes[ 2 ] );
		newTargetAxes[ 0 ].Normalize();
		targetAxes = newTargetAxes;
	}

	if ( shouldTraceDown ) {
		// trace down to find where to land
		trace_t downTrace;
		const idBounds& bounds = other->GetPhysics()->GetBounds();
		idMat3 traceAxis = targetAxes;
		if ( !shouldOrientToRamp ) {
			traceAxis = mat3_identity;
		}

		gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS downTrace, targetPosition + idVec3( 0.0f, 0.0f, 128.0f ), targetPosition - idVec3( 0.0f, 0.0f, 512.0f ),
									bounds, traceAxis, CONTENTS_MOVEABLECLIP, NULL );

		targetPosition = downTrace.endpos + idVec3( 0.0f, 0.0f, traceDownOffset );
	}

	// trace from front to back against the trigger to make sure the new position is outside the teleporting trigger
	if ( targetTeleporter != NULL ) {
		trace_t backTrace;
		const idBounds& bounds = other->GetPhysics()->GetBounds();
		idMat3 traceAxis = targetAxes;
		if ( !shouldOrientBackTrace ) {
			traceAxis = mat3_identity;
		}

		idClipModel* triggerCM = targetTeleporter->GetPhysics()->GetClipModel( 1 );
		const idClipModel* boundsClip = gameLocal.clip.GetTemporaryClipModel( bounds );

		idVec3 endTraceBack = targetPosition;
		idVec3 startTraceBack = endTraceBack + 1024.0f*targetAxes[ 0 ];

		// check all the collision models of the other against our trigger model
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS backTrace, startTraceBack, endTraceBack,
											boundsClip, traceAxis, -1, triggerCM, triggerCM->GetOrigin(), triggerCM->GetAxis() );

		if ( backTrace.fraction < 1.0f ) {
			// push forwards a bit
			targetPosition = backTrace.endpos + 32.0f * targetAxes[ 0 ];
		}
	}


	teleportParms_t parms;
	parms.location			= targetPosition;
	parms.orientation		= targetAxes;
	parms.linearVelocity	= exitVelocity * targetAxes;
	parms.angularVelocity	= vec3_zero;
	parms.spawnId			= gameLocal.GetSpawnId( other );

	// finally, we want to check if the entity went in backwards, in which case
	// they should come out backwards too
	const idMat3& otherAxes = other->GetAxis();
	if ( otherAxes[ 0 ] * myAxes[ 0 ] > 0.7f ) {
		parms.orientation[ 0 ] *= -1.0f;
		parms.orientation[ 1 ] *= -1.0f;
	}

	if ( delay > 0 ) {
		StartTeleport( parms );
		return;
	}

	FinishTeleport( parms );
}

/*
================
sdTeleporter::StartTeleport
================
*/
void sdTeleporter::StartTeleport( const teleportParms_t& parms ) {
	idEntity* ent = gameLocal.EntityForSpawnId( parms.spawnId );
	if ( !ent ) {
		return;
	}

	idEntity* storage = storageLocation;
	if ( storage != NULL ) {
		ent->SetOrigin( storage->GetPhysics()->GetOrigin() );
		ent->SetAxis( mat3_identity );
	}

	ent->OnTeleportStarted( this );

	teleportInfo.Alloc() = parms;

	idEventArg arg;
	arg.SetHandle( parms.spawnId );
	PostEventMS( &EV_FinishTeleport, delay, arg );
}

/*
================
sdTeleporter::FinishTeleport
================
*/
void sdTeleporter::FinishTeleport( idEntity* ent ) {
	int spawnId = gameLocal.GetSpawnId( ent );

	int i;
	for ( i = 0; i < teleportInfo.Num(); i++ ) {
		if ( teleportInfo[ i ].spawnId == spawnId ) {
			break;
		}
	}

	if ( i == teleportInfo.Num() ) {
		gameLocal.Warning( "sdTeleporter::Event_FinishTeleport Failed To Find Entry For SpawnId %d", spawnId );
		return;
	}

	teleportParms_t& parms = teleportInfo[ i ];
	FinishTeleport( parms );
	teleportInfo.RemoveIndexFast( i );
}

/*
================
sdTeleporter::GetTeleportEndPoint
================
*/
void sdTeleporter::GetTeleportEndPoint( idEntity* ent, idVec3& org, idMat3& axes ) {
	int spawnId = gameLocal.GetSpawnId( ent );

	int i;
	for ( i = 0; i < teleportInfo.Num(); i++ ) {
		if ( teleportInfo[ i ].spawnId == spawnId ) {
			break;
		}
	}

	if ( i == teleportInfo.Num() ) {
		org = vec3_origin;
		axes = mat3_identity;
		return;
	}

	org = teleportInfo[ i ].location;
	axes = teleportInfo[ i ].orientation;
}

/*
================
sdTeleporter::FinishTeleport
================
*/
void sdTeleporter::FinishTeleport( const teleportParms_t& parms ) {
	idEntity* ent = gameLocal.EntityForSpawnId( parms.spawnId );
	if ( !ent ) {
		return;
	}

	idVec3 portLocation = parms.location;

	bool teleFragSelf = false;

	idPhysics* phys = ent->GetPhysics();
	const idClipModel* clipModels[ MAX_GENTITIES ];

	// players and jetpacks can spawn in a deploy zone around the exit point
	idPlayer* playerOther = ent->Cast< idPlayer >();
	sdJetPack* jetPackOther = ent->Cast< sdJetPack >();
	if ( playerOther != NULL || jetPackOther != NULL ) {
		// check if the target position is clear
		int clipMask = ent->GetPhysics()->GetClipMask();
		if ( gameLocal.clip.Contents( CLIP_DEBUG_PARMS parms.location + idVec3( 0.0f, 0.0f, 4.0f ), phys->GetClipModel(), mat3_identity, clipMask, this ) ) {
			// not clear - try to find an alternative
			bool foundClear = false;

			// get the positions of the target
			idEntity *targetEntity = targets[ 0 ].GetEntity();
			const idVec3& targetPosition = targetEntity->GetPhysics()->GetOrigin();
			const idMat3& targetAxes = targetEntity->GetAxis();
			idVec3 xAxis = targetAxes[ 0 ];
			idVec3 yAxis = targetAxes[ 1 ];
			xAxis.z = yAxis.z = 0.0f;
			xAxis.Normalize();
			yAxis.Normalize();

			idBounds bounds = phys->GetBounds();

			float step = bounds.Size().Length() * 0.7071f;
			float yStart = -0.5f * deployWidth;
			float yEnd = yStart + deployWidth + step * 0.5f;
			float xStart = -deployReverse;
			float xEnd = xStart + deployLength;

			trace_t trace;
			idVec3 testPosition;
			for ( float x = xStart; x <= xEnd; x += step ) {
				for ( float y = yStart; y <= yEnd; y += step ) {

					testPosition = targetPosition + y*yAxis + x*xAxis;

					gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS trace, testPosition + idVec3( 0.0f, 0.0f, 128.0f ), 
												testPosition - idVec3( 0.0f, 0.0f, 512.0f ), bounds, 
												mat3_identity, CONTENTS_MOVEABLECLIP | CONTENTS_SOLID, NULL );

					int contents = gameLocal.clip.Contents( CLIP_DEBUG_PARMS trace.endpos + idVec3( 0.0f, 0.0f, 4.0f ), phys->GetClipModel(), mat3_identity, clipMask, this );
					if ( !contents ) {
						foundClear = true;
						portLocation = trace.endpos;
						break;
//						gameRenderWorld->DebugBounds( colorGreen, phys->GetBounds(), trace.endpos, mat3_identity, 10000 );
					} else {
//						gameRenderWorld->DebugBounds( colorRed, phys->GetBounds(), trace.endpos, mat3_identity, 10000 );
					}
				}

				if ( foundClear ) {
					break;
				}
			}
			// check in between the last lot
			if ( !foundClear ) {
				for ( float x = xStart + step * 0.5f; x <= xEnd; x += step ) {
					for ( float y = yStart + step * 0.5f; y <= yEnd; y += step ) {

						testPosition = targetPosition + y*yAxis + x*xAxis;

						gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS trace, testPosition + idVec3( 0.0f, 0.0f, 128.0f ), 
													testPosition - idVec3( 0.0f, 0.0f, 512.0f ), bounds, 
													mat3_identity, CONTENTS_MOVEABLECLIP | CONTENTS_SOLID, NULL );

						int contents = gameLocal.clip.Contents( CLIP_DEBUG_PARMS trace.endpos + idVec3( 0.0f, 0.0f, 4.0f ), phys->GetClipModel(), mat3_identity, clipMask, this );
						if ( !contents ) {
							foundClear = true;
							portLocation = trace.endpos;
							break;
//							gameRenderWorld->DebugBounds( colorGreen, phys->GetBounds(), trace.endpos, mat3_identity, 10000 );
						} else {
//							gameRenderWorld->DebugBounds( colorRed, phys->GetBounds(), trace.endpos, mat3_identity, 10000 );
						}
					}

					if ( foundClear ) {
						break;
					}
				}
			}

			if ( !foundClear ) {
				teleFragSelf = true;
			}
		}
	}

	// do the moving
	ent->SetOrigin( portLocation );
	ent->SetAxis( parms.orientation );
	phys->SetLinearVelocity( parms.linearVelocity );
	phys->SetAngularVelocity( parms.angularVelocity );

	ent->OnTeleportFinished();

	if ( telefragDamage != NULL ) {
		sdTransport* transportOther = ent->Cast< sdTransport >();
		if ( transportOther != NULL ) {
			// telefrag stuff
			// see what we're touching
			idStaticList< idEntity*, MAX_GENTITIES > fragEntities;
			int numTouching = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS phys->GetAbsBounds(), phys->GetClipMask(), 
																		clipModels, MAX_GENTITIES, ent );
			for ( int i = 0; i < numTouching; i++ ) {
				const idClipModel* cm = clipModels[ i ];

				// don't check render entities
				if ( cm->IsRenderModel() ) {
					continue;
				}

				idEntity* hit = cm->GetEntity();
				if ( hit == this || hit == NULL ) {
					continue;
				}

				if ( !hit->fl.takedamage ) {
					continue;
				}

				// decide how to hurt it or us
				// some vehicles can't be teleported into, so will telefrag anything that attempts to teleport into it
				bool fragIt = true;
				sdTransport* transportHit = hit->Cast< sdTransport >();
				if ( transportHit != NULL && transportHit->GetVehicleControl() != NULL ) {
					if ( !transportHit->GetVehicleControl()->TeleportCanBeFragged() ) {
						teleFragSelf = true;
						break;
					}
				}

				fragEntities.AddUnique( hit );
			}

			if ( !teleFragSelf ) {
				idPlayer* attacker = transportOther->GetPositionManager().FindDriver();

				// telefrag everything that was to be fragged
				for ( int i = 0; i < fragEntities.Num(); i++ ) {
					fragEntities[ i ]->Damage( ent, attacker, vec3_origin, telefragDamage, 1.0f, NULL, true );
				}
			}
		}

		if ( teleFragSelf ) {
			ent->Damage( ent, ent, vec3_origin, telefragDamage, 1.0f, NULL, true );
		}
	}
}

/*
================
sdTeleporter::CancelTeleport
================
*/
void sdTeleporter::CancelTeleport( idEntity* other ) {
	int spawnId = gameLocal.GetSpawnId( other );
	for ( int i = 0; i < teleportInfo.Num(); i++ ) {
		if ( teleportInfo[ i ].spawnId != spawnId ) {
			continue;
		}

		teleportInfo.RemoveIndexFast( i );
		return;
	}
}

/*
================
sdTeleporter::GetTargetPosition
================
*/
void sdTeleporter::GetTargetPosition( idVec3& origin, idMat3& axis ) {
	idEntity *targetEntity = targets[ 0 ].GetEntity();
	origin = targetEntity->GetPhysics()->GetOrigin();
	axis = targetEntity->GetAxis();
}

