// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../framework/Licensee.h"

#include "Clip.h"
#include "../Entity.h"
#include "../WorldSpawn.h"
#include "../Player.h"
#include "../ContentMask.h"
#include "../Projectile.h"
#include "../Misc.h"

static idVec3 vec3_boxEpsilon( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );
unsigned int idClip::mailBoxID = 0;

#ifdef CLIP_DEBUG
class idClipTimer {
public:

	idClipTimer( const char* _fileName, int _lineNumber, idClip* _world, clipTimerMode_t _mode ) : lineNumber( _lineNumber ), world( _world ), mode( _mode ) {
		sprintf( fileName, "%s", _fileName );
		timer.Start();
	}

	~idClipTimer() {
		timer.Stop();
		double value = timer.Milliseconds();

		if ( collisionModelManager->GetThreadId() == MAIN_THREAD_ID ) {
			world->LogTrace( fileName, lineNumber, value, mode );
#ifdef CLIP_DEBUG_EXTREME
			world->LogTraceExtreme( fileName, lineNumber, value, mode );
#endif
		}
	}

private:
	idClip*			world;
	clipTimerMode_t	mode;
	idTimer			timer;
	char			fileName[ 512 ];
	int				lineNumber;
};

#endif // CLIP_DEBUG

/*
===============================================================

	idClip

===============================================================
*/

/*
===============
idClip::idClip
===============
*/
idClip::idClip( void ) {
	temporaryClipModel = NULL;
	thirdPersonOffsetModel = NULL;
	bigThirdPersonOffsetModel = NULL;
	leanOffsetModel = NULL;
	defaultClipModel = NULL;
	clipSectors = NULL;
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;

	deletedClipModels = NULL;

#ifdef CLIP_DEBUG_EXTREME
	isTraceLogging = false;
	logFileUpto = 0;
#endif
}

/*
===============
idClip::GetWorldBounds
===============
*/
void idClip::GetWorldBounds( idBounds& bounds, int surfaceMask, bool inclusive ) const {
	bounds.Clear();

	idBounds temp;
	for ( int i = 0; i < worldClips.Num(); i++) {
		worldClips[ i ]->GetCollisionModel()->GetBounds( temp, surfaceMask, inclusive );
		bounds += temp;
	}
}

/*
===============
idClip::CreateClipSectors
===============
*/
clipSector_t *idClip::CreateClipSectors( const int depth, const idBounds &bounds ) {
	if ( clipSectors ) {
		delete[] clipSectors;
		clipSectors = NULL;
	}

	int i;
	for( i = 0; i < 3; i++ ) {
		nodeScale[ i ] = depth / ( bounds[ 1 ][ i ] - bounds[ 0 ][ i ] );
		inverseNodeScale[ i ] = 1 / nodeScale[ i ];
		nodeOffset[ i ] = bounds.GetMins()[ i ];
	}

	clipSectors = new clipSector_t[ Square( depth ) ];
	memset( clipSectors, 0, Square( depth ) * sizeof( clipSector_t ) );
	return clipSectors;
}

/*
===============
idClip::Init
===============
*/
void idClip::Init( void ) {

	// load world model
	bool finished = false;
	int idx = -1;
	worldBounds.Clear();
	while ( !finished ) {
		idClipModel *world = NULL;
		if ( idx == -1 ) {
			idCollisionModel *mdl = collisionModelManager->LoadModel( gameLocal.GetMapName(), WORLD_MODEL_NAME );
			if ( mdl != NULL ) {
				world = new idClipModel();
				world->LoadCollisionModel( mdl );
				mdl->SetWorld( true );
			}
		} else {
			idCollisionModel *mdl = collisionModelManager->LoadModel( gameLocal.GetMapName(), va( "%s%d", WORLD_MODEL_NAME, idx ) );
			if ( mdl != NULL ) {
				world = new idClipModel();
				world->LoadCollisionModel( mdl );
				mdl->SetWorld( true );
			} else {
				finished = true;
				break;
			}
		}
		if ( world ) {
			world->SetEntity( NULL );
			world->SetId( 0 );

			if ( world->GetContents() & ( CONTENTS_BODY | CONTENTS_SLIDEMOVER | CONTENTS_CORPSE ) ) {
				gameLocal.Error( WORLD_MODEL_NAME " collision model may not use CONTENTS_BODY or CONTENTS_SLIDEMOVER or CONTENTS_CORPSE" );
			}
			worldBounds.AddBounds( world->GetBounds() );
			worldClips.Alloc() = world;
		}
		idx++;
	}

	if ( worldClips.Num() == 0 ) {
		gameLocal.Error( WORLD_MODEL_NAME " collision model not found" );
	}

	CreateClipSectors( CLIPSECTOR_WIDTH, worldBounds );
	GetClipSectorsStaticContents();

	// initialize a temporary clip model
	temporaryClipModel = new idClipModel();

	thirdPersonOffsetModel = new idClipModel( idTraceModel( idBounds( idVec3( -4.f, -4.f, -4.f ), idVec3( 4.f, 4.f, 4.f ) ) ), false );
	bigThirdPersonOffsetModel = new idClipModel( idTraceModel( idBounds( idVec3( -32.f, -32.f, -32.f ), idVec3( 32.f, 32.f, 32.f ) ) ), false );
	leanOffsetModel = new idClipModel( idTraceModel( idBounds( idVec3( -8, -8, -7 ), idVec3( 8, 8, 4 ) ) ), false );

	// initialize a default clip model
	defaultClipModel = new idClipModel( idTraceModel( idBounds( idVec3( 0.0f, 0.0f, 0.0f ) ).Expand( 8.0f ) ), false );

	// set counters to zero
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::Shutdown
===============
*/
void idClip::Shutdown( void ) {

	// free the clip sectors
	delete[] clipSectors;
	clipSectors = NULL;

	// free the world clip model
	for ( int i = 0; i < worldClips.Num(); i++ ) {
		gameLocal.clip.DeleteClipModel( worldClips[ i ] );
	}
	worldClips.Clear();

	// free the temporary clip model
	gameLocal.clip.DeleteClipModel( temporaryClipModel );
	temporaryClipModel = NULL;

	gameLocal.clip.DeleteClipModel( thirdPersonOffsetModel );
	thirdPersonOffsetModel = NULL;

	gameLocal.clip.DeleteClipModel( bigThirdPersonOffsetModel );
	bigThirdPersonOffsetModel = NULL;

	gameLocal.clip.DeleteClipModel( leanOffsetModel );
	leanOffsetModel = NULL;

	// free the default clip model
	gameLocal.clip.DeleteClipModel( defaultClipModel );
	defaultClipModel = NULL;

	clipLinkAllocator.Shutdown();

	ActuallyDeleteClipModels( true );
}

/*
====================
idClip::GetWorldBounds
====================
*/
const idBounds & idClip::GetWorldBounds( void ) const {
	return worldBounds;
}

/*
================
idClip::ClipModelsTouchingBounds
================
*/
int idClip::ClipModelsTouchingBounds( CLIP_DEBUG_PARMS_DECLARATION const idBounds &bounds, int contentMask, const idClipModel** clipModelList, int maxCount, const idEntity* passEntity, bool includeWorld ) const {
#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU const_cast< idClip* >( this ), CTM_CLIPMODELSTOUCHINGBOUNDS );
#endif // CLIP_DEBUG

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::ClipModelsTouchingBounds called from a thread other than the main thread" );
		return 0;
	}

	idBounds parms( bounds[ 0 ] - vec3_boxEpsilon, bounds[ 1 ] + vec3_boxEpsilon );

	const int MAX_CLIP_MODELS = 256;
	int numClipModels = 0;
	idClipModel* clipModels[ MAX_CLIP_MODELS ];

	int coords[ 4 ];
	CoordsForBounds( coords, parms );

	int x, y;
	for( x = coords[ 0 ]; x < coords[ 2 ]; x++ ) {
		for( y = coords[ 1 ]; y < coords[ 3 ]; y++ ) {
			clipSector_t* sector = &clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];

			for ( clipLink_t* link = sector->clipLinks; link; link = link->nextInSector ) {

				idClipModel* model = link->clipModel;
				if ( !( model->GetContents() & contentMask ) || ( model->GetEntity() == passEntity ) ) {
					continue;
				}


				int i;
				for ( i = 0; i < numClipModels; i++ ) {
					if ( clipModels[ i ] == model ) {
						break;
					}
				}
				if ( i < numClipModels ) {
					continue;
				}

				clipModels[ numClipModels ] = model;
				numClipModels++;
				if ( numClipModels >= MAX_CLIP_MODELS ) {
					gameLocal.Warning( "idClip::ClipModelsTouchingBounds MAX_CLIP_MODELS hit" );
					goto done;
				}
			}
		}
	}
done:

	int count = 0;
	for ( int i = 0; i < numClipModels; i++ ) {
		idClipModel* model = clipModels[ i ];
		
		// if the bounds really do overlap
		if ( !model->GetAbsBounds().IntersectsBounds( parms ) ) {
			continue;
		}

		if( count >= maxCount ) {
			break;
		}

		clipModelList[ count++ ] = model;
	}

	if ( includeWorld && ( passEntity == NULL || passEntity != gameLocal.world ) ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				if ( !world->GetAbsBounds().IntersectsBounds( parms ) ) {
					continue;
				}

				if( count >= maxCount ) {
					break;
				}
	
				clipModelList[ count++ ] = world;
			}
		}
	}	

	return count;
}

/*
================
idClip::EntitiesTouchingRadius
================
*/
int idClip::EntitiesTouchingRadius( const idSphere& sphere, int contentMask, idEntity **entityList, int maxCount, bool nowarning ) const {
	const idClipModel* clipModelList[ MAX_GENTITIES ];
	int i, entCount;

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::EntitiesTouchingRadius called from a thread other than the main thread" );
		return 0;
	}

	bool checks[ MAX_GENTITIES ];
	memset( checks, 0, sizeof( checks ) );

	idBounds bounds( sphere );

	int count = idClip::ClipModelsTouchingBounds( CLIP_DEBUG_PARMS bounds, contentMask, clipModelList, MAX_GENTITIES, NULL );
	entCount = 0;
	for ( i = 0; i < count; i++ ) {
		idEntity* clipEnt = clipModelList[ i ]->GetEntity();		
		if ( checks[ clipEnt->entityNumber ] ) {
			continue;
		}
		checks[ clipEnt->entityNumber ] = true;

		if ( !sphere.IntersectsBounds( clipModelList[ i ]->GetAbsBounds() ) ) {
			continue;
		}

		if ( entCount >= maxCount ) {
			if ( !nowarning ) {
				gameLocal.Warning( "idClip::EntitiesTouchingBounds: max count" );
			}
			return entCount;
		}

		entityList[ entCount ] = clipEnt;
		entCount++;
	}

	return entCount;
}

/*
================
idClip::EntitiesTouchingBounds
================
*/
int idClip::EntitiesTouchingBounds( const idBounds &bounds, int contentMask, idEntity **entityList, int maxCount, bool nowarning ) const {
	const idClipModel *clipModelList[MAX_GENTITIES];
	int i, count, entCount;

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::EntitiesTouchingBounds called from a thread other than the main thread" );
		return 0;
	}

	bool checks[ MAX_GENTITIES ];
	memset( checks, 0, sizeof( checks ) );

	count = idClip::ClipModelsTouchingBounds( CLIP_DEBUG_PARMS bounds, contentMask, clipModelList, MAX_GENTITIES, NULL );
	entCount = 0;
	for ( i = 0; i < count; i++ ) {
		idEntity* clipEnt = clipModelList[ i ]->GetEntity();
		if ( checks[ clipEnt->entityNumber ] ) {
			continue;
		}
		checks[ clipEnt->entityNumber ] = true;

		if ( entCount >= maxCount ) {
			if ( !nowarning ) {
				gameLocal.Warning( "idClip::EntitiesTouchingBounds: max count" );
			}
			return entCount;
		}
		
		entityList[ entCount ] = clipEnt;
		entCount++;
	}

	return entCount;
}

/*
================
idClip::EntitiesTouchingBounds
================
*/
int idClip::EntitiesTouchingBounds( const idBounds &bounds, int contentMask, idEntity **entityList, int maxCount, const idTypeInfo& type, bool nowarning ) const {
	const idClipModel *clipModelList[MAX_GENTITIES];
	int i, count, entCount;

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::EntitiesTouchingBounds called from a thread other than the main thread" );
		return 0;
	}

	bool checks[ MAX_GENTITIES ];
	memset( checks, 0, sizeof( checks ) );

	count = idClip::ClipModelsTouchingBounds( CLIP_DEBUG_PARMS bounds, contentMask, clipModelList, MAX_GENTITIES, NULL );
	entCount = 0;
	for ( i = 0; i < count; i++ ) {

		idEntity* clipEnt = clipModelList[ i ]->GetEntity();
		if ( checks[ clipEnt->entityNumber ] ) {
			continue;
		}
		checks[ clipEnt->entityNumber ] = true;

		if ( !clipEnt->IsType( type ) ) {
			continue;
		}

		if ( entCount >= maxCount ) {
			if ( !nowarning ) {
				gameLocal.Warning( "idClip::EntitiesTouchingBounds: max count" );
			}
			return entCount;
		}
		entityList[ entCount ] = clipEnt;
		entCount++;
	}

	return entCount;
}

const int MAX_CLIPMODELSTOUCHINGBOUNDS = 1024;

/*
====================
idClip::DrawAreaClipSectors
====================
*/
void idClip::DrawAreaClipSectors( float range ) const {
	const idClipModel* clipModels[ MAX_CLIPMODELSTOUCHINGBOUNDS ];

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	idBounds bounds;
	bounds.GetMins() = player->GetPhysics()->GetOrigin() - idVec3( range, range, range );
	bounds.GetMaxs() = player->GetPhysics()->GetOrigin() + idVec3( range, range, range );

	int count = ClipModelsTouchingBounds( CLIP_DEBUG_PARMS bounds, MASK_ALL, clipModels, MAX_CLIPMODELSTOUCHINGBOUNDS, NULL );

	int i;
	for ( i = 0; i < count; i++ ) {
		idEntity* owner = clipModels[ i ]->GetEntity();

		const idVec3& org = clipModels[ i ]->GetOrigin();
		const idBounds& bounds = clipModels[ i ]->GetBounds();

		gameRenderWorld->DebugBounds( colorCyan, bounds, org );
		gameRenderWorld->DrawText( owner->GetClassname(), org, 0.5f, colorCyan, player->viewAngles.ToMat3(), 1 );
	}
}

/*
====================
idClip::DrawClipSectors
====================
*/
void idClip::DrawClipSectors( void ) const {
	idBounds bounds;

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	const char* filter = g_showClipSectorFilter.GetString();
	idTypeInfo* type = idClass::GetClass( filter );

	int x;
	for( x = 0; x < CLIPSECTOR_WIDTH; x++ ) {
		int y;
		for( y = 0; y < CLIPSECTOR_WIDTH; y++ ) {
			bounds[ 0 ].x = ( inverseNodeScale.x * x ) + nodeOffset.x + 1;
			bounds[ 0 ].y = ( inverseNodeScale.y * y ) + nodeOffset.y + 1;
			bounds[ 0 ].z = player->GetPhysics()->GetBounds().GetMins().z;

			bounds[ 1 ].x = ( inverseNodeScale.x * ( x + 1 ) ) + nodeOffset.x - 1;
			bounds[ 1 ].y = ( inverseNodeScale.y * ( y + 1 ) ) + nodeOffset.y - 1;
			bounds[ 1 ].z = player->GetPhysics()->GetBounds().GetMaxs().z;

			idVec3 point;
			point.x = ( bounds[ 0 ].x + bounds[ 1 ].x ) * 0.5f;
			point.y = ( bounds[ 0 ].y + bounds[ 1 ].y ) * 0.5f;
			point.z = 0.f;


			clipSector_t* sector = &clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];

			clipLink_t* link = sector->clipLinks;
			while ( link ) {
				if ( type && !link->clipModel->GetEntity()->IsType( *type ) ) {
					link = link->nextInSector;
				} else {
					break;
				}
			}

			if( link ) {
				
				gameRenderWorld->DrawText( link->clipModel->GetEntity()->GetClassname(), point, 0.5f, colorCyan, player->viewAngles.ToMat3(), 1 );
				gameRenderWorld->DebugBounds( colorMagenta, bounds );
				gameRenderWorld->DebugBounds( colorYellow, link->clipModel->GetBounds(), link->clipModel->GetOrigin() );

			} else {

//				gameRenderWorld->DrawText( sector->clipLinks->clipModel->GetEntity()->GetClassname(), point, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );

			}
		}
	}
}

/*
====================
idClip::GetClipSectorsStaticContents
====================
*/
void idClip::GetClipSectorsStaticContents( void ) {
	idBounds bounds;

	bounds[ 0 ].x = 0;
	bounds[ 0 ].y = 0;
	bounds[ 0 ].z = worldBounds.GetMins().z;

	bounds[ 1 ].x = 1 / nodeScale.x;
	bounds[ 1 ].y = 1 / nodeScale.y;
	bounds[ 1 ].z = worldBounds.GetMaxs().z;

	idTraceModel* trm = new idTraceModel( bounds );

	idVec3 org;
	org.z = 0;

	int x;
	for( x = 0; x < CLIPSECTOR_WIDTH; x++ ) {
		int y;
		for( y = 0; y < CLIPSECTOR_WIDTH; y++ ) {
			org.x = ( x / nodeScale.x ) + nodeOffset.x;
			org.y = ( y / nodeScale.y ) + nodeOffset.y;

			int contents = collisionModelManager->Contents( org, trm, mat3_identity, -1, 0, vec3_origin, mat3_default );
			clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ].contents = contents;
		}
	}

	delete trm;
}

/*
============
idClip::TraceRenderModel
============
*/
void idClip::TraceRenderModel( trace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, const idMat3 &axis, int contentMask, idClipModel *mdl ) {
	
	// avoid a crash when bot thread uses render model flag
	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		return;
	}

	// if the trace is passing through the bounds
	if ( mdl->GetAbsBounds().Expand( radius ).LineIntersection( start, end ) ) {
		modelTrace_t modelTrace;

		idClip::numRenderModelTraces++;

		// test with exact render model and modify trace_t structure accordingly
		if ( gameRenderWorld->ModelTrace( modelTrace, mdl->GetRenderEntity(), start, end, radius, (contentMask & CONTENTS_SHADOWCOLLISION) ? SURF_SHADOWCOLLISION : SURF_COLLISION ) ) {
			if ( modelTrace.fraction < trace.fraction ) {
				trace.fraction = modelTrace.fraction;
				trace.endAxis = axis;
				trace.endpos = modelTrace.point;
				trace.c.normal = modelTrace.normal;
				trace.c.dist = modelTrace.point * modelTrace.normal;
				trace.c.point = modelTrace.point;
				trace.c.type = CONTACT_TRMVERTEX;
				trace.c.modelFeature = 0;
				trace.c.trmFeature = 0;
				trace.c.contents = modelTrace.material->GetContentFlags();
				trace.c.material = modelTrace.material;
				trace.c.entityNum = mdl->GetEntityNumber();
				trace.c.surfaceType = modelTrace.surfaceType;
				trace.c.surfaceColor = modelTrace.surfaceColor;
				// NOTE: trace.c.id will be the joint number
				trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( modelTrace.jointNumber );
			}
		}
	}
}

/*
============
idClip::GetTraceClipModels
============
*/
int idClip::GetTraceClipModels( const idVec3& start, const idVec3& end, int contentMask, const idEntity* passEntity, idClipModel** clipModelList, traceMode_t traceMode ) const {
	idBounds parms;
	parms.FromPointTranslation( start, end - start );
	parms.ExpandSelf( vec3_boxEpsilon );

	const int MAX_CLIP_MODELS = MAX_GENTITIES;
	int numClipModels = 0;
	idClipModel** clipModels = clipModelList;

	int coords[ 4 ];
	CoordsForBounds( coords, parms );

	sdBounds2D bounds2d;

	idVec2 dir = end.ToVec2() - start.ToVec2();
	dir.Normalize();

	idVec2 normal( dir.y, -dir.x );

	bool mainThread = ( collisionModelManager->GetThreadId() == MAIN_THREAD_ID );	// only the main thread may use entity pointers

	Lock();

	// this must be after the Lock
	int curmb = mailBoxID++;

	for( int x = coords[ 0 ]; x < coords[ 2 ]; x++ ) {
		for( int y = coords[ 1 ]; y < coords[ 3 ]; y++ ) {

			idVec2 v[ 4 ];
			v[ 0 ].x = ( inverseNodeScale.x * x ) + nodeOffset.x;
			v[ 0 ].y = ( inverseNodeScale.y * y ) + nodeOffset.y;

			v[ 1 ].x = ( inverseNodeScale.x * ( x + 1 ) ) + nodeOffset.x;
			v[ 1 ].y = ( inverseNodeScale.y * ( y + 1 ) ) + nodeOffset.y;

			v[ 2 ].x = v[ 0 ].x;
			v[ 2 ].y = v[ 1 ].y;

			v[ 3 ].x = v[ 1 ].x;
			v[ 3 ].y = v[ 0 ].y;

			int front	= 0;
			int back	= 0;

			int i;
			for ( i = 0; i < 4; i++ ) {
				float d = ( v[ i ] - start.ToVec2() ) * normal;
				if ( d > idMath::FLT_EPSILON ) {
					front++;
					if ( back ) {
						break;
					}
				} else if ( d < -idMath::FLT_EPSILON ) {
					back++;
					if ( front ) {
						break;
					}
				} else {
					break;
				}
			}

			if ( i == 4 ) {
				continue;
			}

			clipSector_t* sector = &clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];
			for ( clipLink_t* link = sector->clipLinks; link; link = link->nextInSector ) {

				idClipModel *cm = link->clipModel;
				const idBounds &bb = cm->GetAbsBounds();
				if ( parms[1][2] < bb[0][2] ) {
					continue;
				}
				if ( parms[0][2] > bb[1][2] ) {
					continue;
				}

#if 1
				if ( cm->lastMailBox == curmb ) {
					continue;
				}
				cm->lastMailBox = curmb;
#else			
				int i;
				for ( i = 0; i < numClipModels; i++ ) {
					if ( clipModels[ i ] == link->clipModel ) {
						break;
					}
				}
				if ( i < numClipModels ) {
					continue;
				}
#endif

				clipModels[ numClipModels ] = link->clipModel;
				numClipModels++;
				if ( numClipModels >= MAX_CLIP_MODELS ) {
					goto done;
				}
			}
		}
	}
done:

	Unlock();

	if ( numClipModels >= 256 && mainThread ) {
		gameLocal.Warning( "Touching a large number of collision models!" );
	}

	int count = 0;
	if ( passEntity != NULL ) {
		for ( int i = 0; i < numClipModels; i++ ) {
			idClipModel* model = clipModels[ i ];
			if ( !( model->GetContents() & contentMask ) ) {
				continue;
			}

			if ( !model->GetAbsBounds().LineIntersection( start, end ) ) {
				continue;
			}

			if ( mainThread ) {
				if ( !model->GetEntity()->CanCollide( passEntity, traceMode ) || !passEntity->CanCollide( model->GetEntity(), traceMode ) ) {
					continue;
				}
			} else {
				if ( model->GetEntity() == passEntity ) {
					continue;
				}
			}

			clipModelList[ count++ ] = model;
		}
	} else {
		for ( int i = 0; i < numClipModels; i++ ) {
			idClipModel* model = clipModels[ i ];
			if ( !( model->GetContents() & contentMask ) ) {
				continue;
			}
			
			if ( !model->GetAbsBounds().LineIntersection( start, end ) ) {
				continue;
			}

			clipModelList[ count++ ] = model;
		}
	}

	return count;
}

/*
============
idClip::GetTraceClipModels
============
*/
int idClip::GetTraceClipModels( const idBounds& bounds, int contentMask, const idEntity* passEntity, idClipModel** clipModelList, traceMode_t traceMode ) const {
	idBounds parms( bounds[ 0 ] - vec3_boxEpsilon, bounds[ 1 ] + vec3_boxEpsilon );

	const int MAX_CLIP_MODELS = MAX_GENTITIES;
	int numClipModels = 0;
	idClipModel** clipModels = clipModelList;

	int coords[ 4 ];
	CoordsForBounds( coords, parms );

	bool mainThread = ( collisionModelManager->GetThreadId() == MAIN_THREAD_ID );	// only the main thread may use entity pointers

	Lock();

	// this must be after the Lock
	int curmb = mailBoxID++;

	int x, y;
	for( x = coords[ 0 ]; x < coords[ 2 ]; x++ ) {
		for( y = coords[ 1 ]; y < coords[ 3 ]; y++ ) {
			clipSector_t* sector = &clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];

			for ( clipLink_t* link = sector->clipLinks; link; link = link->nextInSector ) {
				idClipModel *cm = link->clipModel;
				const idBounds &bb = cm->GetAbsBounds();
				if ( parms[1][2] < bb[0][2] ) {
					continue;
				}
				if ( parms[0][2] > bb[1][2] ) {
					continue;
				}

#if 1
				if ( cm->lastMailBox == curmb ) {
					continue;
				}
				cm->lastMailBox = curmb;
#else			
				int i;
				for ( i = 0; i < numClipModels; i++ ) {
					if ( clipModels[ i ] == link->clipModel ) {
						break;
					}
				}
				if ( i < numClipModels ) {
					continue;
				}
#endif

				clipModels[ numClipModels ] = link->clipModel;
				numClipModels++;
				if ( numClipModels >= MAX_CLIP_MODELS ) {
					goto done;
				}
			}
		}
	}
done:

	Unlock();

	if ( numClipModels >= 256 && mainThread ) {
		gameLocal.Warning( "Touching a large number of collision models!" );
	}

	int count = 0;
	if ( passEntity != NULL ) {
		for ( int i = 0; i < numClipModels; i++ ) {
			idClipModel* model = clipModels[ i ];
			if ( !( model->GetContents() & contentMask ) ) {
				continue;
			}

			// if the bounds really do overlap
			if ( !model->GetAbsBounds().IntersectsBounds( parms ) ) {
				continue;
			}

			if ( mainThread ) {
				if ( !model->GetEntity()->CanCollide( passEntity, traceMode ) || !passEntity->CanCollide( model->GetEntity(), traceMode ) ) {
					continue;
				}
			} else {
				if ( model->GetEntity() == passEntity ) {
					continue;
				}
			}

			clipModelList[ count++ ] = model;
		}
	} else {
		for ( int i = 0; i < numClipModels; i++ ) {
			idClipModel* model = clipModels[ i ];
			if ( !( model->GetContents() & contentMask ) ) {
				continue;
			}
			
			// if the bounds really do overlap
			if ( !model->GetAbsBounds().IntersectsBounds( parms ) ) {
				continue;
			}

			clipModelList[ count++ ] = model;
		}
	}

	return count;
}

/*
============
idClip::GetTraceClipModelsExt

This version doesn't do anything with the entity pointers, so is thread safe.
============
*/
int idClip::GetTraceClipModelsExt( const idVec3& start, const idVec3& end, int contentMask, const idEntity *passEntity1, const idEntity *passEntity2, idClipModel** clipModelList, traceMode_t traceMode ) const {
	idBounds parms;
	parms.FromPointTranslation( start, end - start );
	parms.ExpandSelf( vec3_boxEpsilon );

	const int MAX_CLIP_MODELS = MAX_GENTITIES;
	int numClipModels = 0;
	idClipModel** clipModels = clipModelList;

	int coords[ 4 ];
	CoordsForBounds( coords, parms );

	sdBounds2D bounds2d;

	idVec2 dir = end.ToVec2() - start.ToVec2();
	dir.Normalize();

	idVec2 normal( dir.y, -dir.x );

	bool mainThread = ( collisionModelManager->GetThreadId() == MAIN_THREAD_ID );	// only the main thread may use entity pointers

	Lock();

	// this must be after the Lock
	int curmb = mailBoxID++;

	for( int x = coords[ 0 ]; x < coords[ 2 ]; x++ ) {
		for( int y = coords[ 1 ]; y < coords[ 3 ]; y++ ) {

			idVec2 v[ 4 ];
			v[ 0 ].x = ( inverseNodeScale.x * x ) + nodeOffset.x;
			v[ 0 ].y = ( inverseNodeScale.y * y ) + nodeOffset.y;

			v[ 1 ].x = ( inverseNodeScale.x * ( x + 1 ) ) + nodeOffset.x;
			v[ 1 ].y = ( inverseNodeScale.y * ( y + 1 ) ) + nodeOffset.y;

			v[ 2 ].x = v[ 0 ].x;
			v[ 2 ].y = v[ 1 ].y;

			v[ 3 ].x = v[ 1 ].x;
			v[ 3 ].y = v[ 0 ].y;

			int front	= 0;
			int back	= 0;

			int i;
			for ( i = 0; i < 4; i++ ) {
				float d = ( v[ i ] - start.ToVec2() ) * normal;
				if ( d > idMath::FLT_EPSILON ) {
					front++;
					if ( back ) {
						break;
					}
				} else if ( d < -idMath::FLT_EPSILON ) {
					back++;
					if ( front ) {
						break;
					}
				} else {
					break;
				}
			}

			if ( i == 4 ) {
				continue;
			}

			clipSector_t* sector = &clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];
			for ( clipLink_t* link = sector->clipLinks; link; link = link->nextInSector ) {
				idClipModel *cm = link->clipModel;
				const idBounds &bb = cm->GetAbsBounds();
				if ( parms[1][2] < bb[0][2] ) {
					continue;
				}
				if ( parms[0][2] > bb[1][2] ) {
					continue;
				}

#if 1
				if ( cm->lastMailBox == curmb ) {
					continue;
				}
				cm->lastMailBox = curmb;

#else			
				int i;
				for ( i = 0; i < numClipModels; i++ ) {
					if ( clipModels[ i ] == link->clipModel ) {
						break;
					}
				}
				if ( i < numClipModels ) {
					continue;
				}
#endif

				clipModels[ numClipModels ] = link->clipModel;
				numClipModels++;
				if ( numClipModels >= MAX_CLIP_MODELS ) {
					goto done;
				}
			}
		}
	}
done:

	Unlock();

	if ( numClipModels >= 256 && mainThread ) {
		gameLocal.Warning( "Touching a large number of collision models!" );
	}

	int count = 0;
	if ( passEntity1 != NULL || passEntity2 != NULL ) {
		for ( int i = 0; i < numClipModels; i++ ) {
			idClipModel* model = clipModels[ i ];
			if ( !( model->GetContents() & contentMask ) ) {
				continue;
			}

			if ( !model->GetAbsBounds().LineIntersection( start, end ) ) {
				continue;
			}

			if ( ( passEntity1 != NULL && model->GetEntity() == passEntity1 ) || ( passEntity2 != NULL && model->GetEntity() == passEntity2 ) ) {
                continue;
			}

			clipModelList[ count++ ] = model;
		}
	} else {
		for ( int i = 0; i < numClipModels; i++ ) {
			idClipModel* model = clipModels[ i ];
			if ( !( model->GetContents() & contentMask ) ) {
				continue;
			}
			
			if ( !model->GetAbsBounds().LineIntersection( start, end ) ) {
				continue;
			}

			clipModelList[ count++ ] = model;
		}
	}

	return count;
}

/*
============
idClip::TestHugeTranslation
============
*/
ID_INLINE bool idClip::TestHugeTranslation( trace_t &results, const idClipModel *mdl, const idVec3 &start, const idVec3 &end, const idMat3 &trmAxis ) const {
	if ( mdl != NULL && ( end - start ).LengthSqr() > Square( CM_MAX_TRACE_DIST ) ) {

		results.fraction = 0.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		memset( &results.c, 0, sizeof( results.c ) );
		results.c.entityNum = ENTITYNUM_WORLD;
		results.c.normal.Set( 0.f, 0.f, 1.f );
		results.c.point = start;

		if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
			common->FatalError( "huge translation from a thread other than the main thread" );
		}

		if ( mdl->GetEntity() ) {
			gameLocal.Printf( "huge translation for clip model %d on entity %d '%s'\n", mdl->GetId(), mdl->GetEntityNumber(), mdl->GetEntityName() );
		} else {
			gameLocal.Printf( "huge translation for clip model %d\n", mdl->GetId() );
		}
		return true;
	}
	return false;
}

/*
============
idClip::TranslationClipModel
============
*/
ID_INLINE bool idClip::TranslationClipModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl ) {
	int i;
	trace_t trace;

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_TRANSLATION );
#endif // CLIP_DEBUG

	for ( i = 0; i < mdl->GetNumCollisionModels(); i++ ) {

		idClip::numTranslations++;
		collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
												mdl->GetCollisionModel( i ), mdl->GetOrigin(), mdl->GetAxis() );

		if ( trace.fraction < results.fraction ) {
			results = trace;
			results.c.entityNum = mdl->GetEntityNumber();
			results.c.id = mdl->GetId();
			if ( results.fraction == 0.0f ) {
				return true;
			}
		}
	}

	return false;
}

/*
============
idClip::RotationClipModel
============
*/
ID_INLINE bool idClip::RotationClipModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl ) {
	int i;
	trace_t trace;

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_ROTATION );
#endif // CLIP_DEBUG

	for ( i = 0; i < mdl->GetNumCollisionModels(); i++ ) {

		idClip::numRotations++;
		collisionModelManager->Rotation( &trace, start, rotation, trm, trmAxis, contentMask,
											mdl->GetCollisionModel( i ), mdl->GetOrigin(), mdl->GetAxis() );

		if ( trace.fraction < results.fraction ) {
			results = trace;
			results.c.entityNum = mdl->GetEntityNumber();
			results.c.id = mdl->GetId();
			if ( results.fraction == 0.0f ) {
				return true;
			}
		}
	}
	return false;
}

/*
============
idClip::ContactsClipModel
============
*/
ID_INLINE int idClip::ContactsClipModel( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec3 *dir, const float depth,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl ) {
	int i, j, n;
	int numContacts;

	numContacts = 0;

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_CONTACTS );
#endif // CLIP_DEBUG

	for ( i = 0; i < mdl->GetNumCollisionModels(); i++ ) {

		idClip::numContacts++;
		n = collisionModelManager->Contacts( contacts + numContacts, maxContacts - numContacts,
								start, dir, depth, trm, trmAxis, contentMask,
									mdl->GetCollisionModel( i ), mdl->GetOrigin(), mdl->GetAxis() );

		for ( j = 0; j < n; j++ ) {
			contacts[numContacts].entityNum = mdl->GetEntityNumber();
			contacts[numContacts].id = mdl->GetId();
			numContacts++;
		}
		if ( numContacts >= maxContacts ) {
			return numContacts;
		}
	}
	return numContacts;
}

/*
============
idClip::ContentsClipModel
============
*/
ID_INLINE int idClip::ContentsClipModel( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl ) {
	int i;
	int contents = 0;

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_CONTENTS );
#endif // CLIP_DEBUG

	for ( i = 0; i < mdl->GetNumCollisionModels(); i++ ) {

		idClip::numContents++;
		if ( collisionModelManager->Contents( start, trm, trmAxis, contentMask, mdl->GetCollisionModel( i ), mdl->GetOrigin(), mdl->GetAxis() ) ) {
			contents |= ( mdl->GetContents() & contentMask );
		}
	}
	return contents;
}

/*
============
idClip::TranslationEntities
============
*/
bool idClip::TranslationEntities( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {
	int i, j, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;

	if ( TestHugeTranslation( results, mdl, start, end, trmAxis ) ) {
		return true;
	}

	results.fraction = 1.0f;
	results.endpos = end;
	results.endAxis = trmAxis;

	if ( mdl == NULL ) {
		radius = 0.0f;

		num = GetTraceClipModels( start, end, contentMask, passEntity, clipModelList, traceMode );
	} else {
		idBounds traceBounds;
		traceBounds.FromBoundsTranslation( mdl->GetBounds(), start, trmAxis, end - start );
		radius = mdl->GetBounds().GetRadius();

		num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, traceMode );
	}

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[ i ];

		if ( touch->IsRenderModel() ) {
			TraceRenderModel( results, start, end, radius, trmAxis, contentMask, touch );
		} else if ( mdl == NULL ) {
			if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, trmAxis, contentMask, touch ) ) {
				results.c.selfId = -1;
				return true;
			}
		} else {
			for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
				if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, mdl->GetTraceModel( j ), trmAxis, contentMask, touch ) ) {
					results.c.selfId = mdl->GetId();
					return true;
				}
			}
		}
	}

	results.c.selfId = mdl ? mdl->GetId() : -1;
	return ( results.fraction < 1.0f );
}

typedef sdPair< idEntity*, float > entFracPair_t;

int SortByEntsByFraction( const entFracPair_t* a, const entFracPair_t* b ) {
	return b->second - a->second;
}

/*
============
idClip::EntitiesForTranslation
============
*/
int idClip::EntitiesForTranslation( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start, const idVec3 &end, int contentMask, const idEntity* passEntity, idEntity** entities, int maxEntities, traceMode_t traceMode ) {
	trace_t results;
	results.endpos = end;
	results.endAxis = mat3_identity;
	memset( &results.c, 0, sizeof( results.c ) );

	bool checks[ MAX_GENTITIES ];
	memset( checks, 0, sizeof( checks ) );

	static idStaticList< entFracPair_t, MAX_GENTITIES > localEnts;
	localEnts.Clear();

	idBounds traceBounds;
	traceBounds.FromPointTranslation( start, end - start );

	idClipModel* clipModelList[ MAX_GENTITIES ];

	int numClipModels = GetTraceClipModels( start, end, contentMask, passEntity, clipModelList, traceMode );

	for ( int i = 0; i < numClipModels; i++ ) {
		idClipModel* touch = clipModelList[ i ];

		results.fraction = 1.0f;

		if ( touch->IsRenderModel() ) {
			TraceRenderModel( results, start, end, 0.f, mat3_identity, contentMask, touch );
		} else {
			TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, mat3_identity, contentMask, touch );
		}

		idEntity* ent = touch->GetEntity();
		if ( checks[ ent->entityNumber ] ) {
			for ( int j = 0; j < localEnts.Num(); j++ ) {
				if ( localEnts[ j ].first == ent ) {
					if ( results.fraction < localEnts[ j ].second ) {
						localEnts[ j ].second = results.fraction;
					}
				}
			}
		} else {
			entFracPair_t* newEnt = localEnts.Alloc();
			if ( !newEnt ) {
				break;
			}

			newEnt->first	= ent;
			newEnt->second	= results.fraction;

			checks[ ent->entityNumber ] = true;
		}
	}

	localEnts.Sort( SortByEntsByFraction );

	for ( int i = 0; i < localEnts.Num(); i++ ) {
		if ( i < maxEntities ) {
			entities[ i ] = localEnts[ i ].first;
		}
	}

	return localEnts.Num();
}

/*
============
idClip::Translation
============
*/
bool idClip::Translation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode, float forceRadius ) {
	int i, j, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;

	if ( TestHugeTranslation( results, mdl, start, end, trmAxis ) ) {
		return true;
	}

	results.fraction = 1.0f;
	results.endpos = end;
	results.endAxis = trmAxis;
	memset( &results.c, 0, sizeof( results.c ) );

	if ( mdl == NULL ) {
		traceBounds.FromPointTranslation( start, end - start );
	} else {
		traceBounds.FromBoundsTranslation( mdl->GetBounds(), start, trmAxis, end - start );
	}

	// NOTE: only comparing and not actually using the passEntity pointer to keep this code thread safe
	if ( passEntity == NULL || passEntity != gameLocal.world ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				if ( !world->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
					continue;
				}

				// test world
				if ( mdl == NULL ) {
					if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, trmAxis, contentMask, world ) ) {
						results.c.selfId = -1;
						return true;		// blocked immediately by the world
					}
				} else {
					for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
						if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, mdl->GetTraceModel( j ), trmAxis, contentMask, world ) ) {
							results.c.selfId = mdl->GetId();
							return true;		// blocked immediately by the world
						}
					}
				}
			}
		}
	}

	if ( mdl == NULL ) {
		radius = 0.0f;
		num = GetTraceClipModels( start, results.endpos, contentMask, passEntity, clipModelList, traceMode );
	} else {
		radius = mdl->GetBounds().GetRadius();
		num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, traceMode );
	}

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[ i ];

		if ( touch->IsRenderModel() ) {
			TraceRenderModel( results, start, end, radius, trmAxis, contentMask, touch );
		} else if ( mdl == NULL ) {
			if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, trmAxis, contentMask, touch ) ) {
				results.c.selfId = -1;
				return true;
			}
		} else {
			for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
				if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, mdl->GetTraceModel( j ), trmAxis, contentMask, touch ) ) {
					results.c.selfId = mdl->GetId();
					return true;
				}
			}
		}
	}

	if ( forceRadius > 0.f ) {
		if ( results.c.entityNum == ENTITYNUM_NONE || results.c.entityNum == ENTITYNUM_WORLD ) {
			for ( i = 0; i < num; i++ ) {
				touch = clipModelList[ i ];

				if ( touch->IsRenderModel() ) {
					TraceRenderModel( results, start, end, forceRadius, trmAxis, contentMask, touch );
				}
			}
		}
	}

	results.c.selfId = mdl ? mdl->GetId() : -1;
	return ( results.fraction < 1.0f );
}

/*
============
idClip::TracePointExt
============
*/
bool idClip::TracePointExt( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask, const idEntity *passEntity1, const idEntity *passEntity2, traceMode_t traceMode ) {
	int i, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;

	if ( TestHugeTranslation( results, NULL, start, end, mat3_identity ) ) {
		return true;
	}

	results.fraction = 1.0f;
	results.endpos = end;
	results.endAxis = mat3_identity;
	memset( &results.c, 0, sizeof( results.c ) );
	results.c.entityNum = -1;

	traceBounds.FromPointTranslation( start, end - start );

	// NOTE: only comparing and not actually using the passEntity pointer to keep this code thread safe
	if ( ( passEntity1 == NULL || passEntity1 != gameLocal.world ) && ( passEntity2 == NULL || passEntity2 != gameLocal.world ) ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				if ( !world->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
					continue;
				}

				// test world
				if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, mat3_identity, contentMask, world ) ) {
					results.c.selfId = -1;
					return true;		// blocked immediately by the world
				}
			}
		}
	}

	num = GetTraceClipModelsExt( start, results.endpos, contentMask, passEntity1, passEntity2, clipModelList, traceMode );

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[ i ];

		if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, mat3_identity, contentMask, touch ) ) {
			results.c.selfId = -1;
			return true;
		}
	}

	results.c.selfId = -1;
	return ( results.fraction < 1.0f );
}

/*
============
idClip::RotationInternal
============
*/
bool idClip::RotationInternal( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {
	int i, j, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;

	results.fraction = 1.0f;
	results.endpos = start;
	results.endAxis = trmAxis * rotation.ToMat3();
	rotation.RotatePoint( results.endpos );
	memset( &results.c, 0, sizeof( results.c ) );

	results.c.selfId = mdl ? mdl->GetId() : -1;

	if ( mdl == NULL ) {
		traceBounds.FromPointRotation( start, rotation );
	} else {
		traceBounds.FromBoundsRotation( mdl->GetBounds(), start, trmAxis, rotation );
	}

	// NOTE: only comparing and not actually using the passEntity pointer to keep this code thread safe
	if ( passEntity == NULL || passEntity != gameLocal.world ) {
		for (int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				if ( !world->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
					continue;
				}

				// test world
				if ( mdl == NULL ) {
					if ( RotationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, NULL, trmAxis, contentMask, world ) ) {
						results.c.selfId = mdl ? mdl->GetId() : -1;
						return true;		// blocked immediately by the world
					}
				} else {
					for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
						if ( RotationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, mdl->GetTraceModel( j ), trmAxis, contentMask, world ) ) {
							results.c.selfId = mdl ? mdl->GetId() : -1;
							return true;		// blocked immediately by the world
						}
					}
				}
			}
		}
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, traceMode );

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( touch == NULL ) {
			continue;
		}

		if ( touch->IsRenderModel() ) {
			continue;	// no rotational collision with render models
		} else if ( mdl == NULL ) {
			if ( RotationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, NULL, trmAxis, contentMask, touch ) ) {
				results.c.selfId = mdl ? mdl->GetId() : -1;
				return true;
			}
		} else {
			for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
				if ( RotationClipModel( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, mdl->GetTraceModel( j ), trmAxis, contentMask, touch ) ) {
					results.c.selfId = mdl ? mdl->GetId() : -1;
					return true;
				}
			}
		}
	}

	results.c.selfId = mdl ? mdl->GetId() : -1;
	return ( results.fraction < 1.0f );
}

/*
============
idClip::MotionInternal
============
*/
bool idClip::MotionInternal( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {
	
	int i, j, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idVec3 dir, endPosition;
	idBounds traceBounds;
	float radius;
	trace_t translationalTrace, rotationalTrace, trace;
	idRotation endRotation;

	assert( rotation.GetOrigin() == start );
	assert( mdl != NULL );

	if ( TestHugeTranslation( results, mdl, start, end, trmAxis ) ) {
		return true;
	}

	if ( mdl != NULL && rotation.GetAngle() != 0.0f && rotation.GetVec() != vec3_origin ) {
		// if no translation
		if ( start == end ) {
			// pure rotation
			results.c.selfId = mdl ? mdl->GetId() : -1;
			return Rotation( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, mdl, trmAxis, contentMask, passEntity );
		}
	} else if ( start != end ) {
		// pure translation
		results.c.selfId = mdl ? mdl->GetId() : -1;
		return Translation( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, mdl, trmAxis, contentMask, passEntity );
	} else {
		// no motion
		results.fraction = 1.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		memset( &results.c, 0, sizeof( results.c ) );
		results.c.selfId = mdl ? mdl->GetId() : -1;
		return false;
	}

	translationalTrace.fraction = 1.0f;
	translationalTrace.endpos = end;
	translationalTrace.endAxis = trmAxis;
	memset( &translationalTrace.c, 0, sizeof( translationalTrace.c ) );

	// NOTE: only comparing and not actually using the passEntity pointer to keep this code thread safe
	if ( passEntity == NULL || passEntity != gameLocal.world ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				// translational collision with world
				for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
					if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU translationalTrace, start, end, mdl->GetTraceModel( j ), trmAxis, contentMask, world ) ) {
						break;
					}
				}
			}
		}
	}

	if ( translationalTrace.fraction != 0.0f ) {

		traceBounds.FromBoundsRotation( mdl->GetBounds(), start, trmAxis, rotation );
		dir = translationalTrace.endpos - start;
		for ( i = 0; i < 3; i++ ) {
			if ( dir[i] < 0.0f ) {
				traceBounds[0][i] += dir[i];
			} else {
				traceBounds[1][i] += dir[i];
			}
		}

		radius = mdl->GetBounds().GetRadius();

		num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, traceMode );

		for ( i = 0; i < num; i++ ) {
			touch = clipModelList[i];

			if ( touch == NULL ) {
				continue;
			}

			if ( touch->IsRenderModel() ) {
				TraceRenderModel( translationalTrace, start, end, radius, trmAxis, contentMask, touch );
			} else {
				for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
					if ( TranslationClipModel( CLIP_DEBUG_PARMS_PASSTHRU translationalTrace, start, end, mdl->GetTraceModel( j ), trmAxis, contentMask, touch ) ) {
						break;
					}
				}
			}
			if ( translationalTrace.fraction == 0.0f ) {
				break;
			}
		}
	} else {
		num = -1;
	}

	endPosition = translationalTrace.endpos;
	endRotation = rotation;
	endRotation.SetOrigin( endPosition );

	rotationalTrace.fraction = 1.0f;
	rotationalTrace.endpos = endPosition;
	rotationalTrace.endAxis = trmAxis * endRotation.ToMat3();
	memset( &rotationalTrace.c, 0, sizeof( rotationalTrace.c ) );

	// NOTE: only comparing and not actually using the passEntity pointer to keep this code thread safe
	if ( passEntity == NULL || passEntity != gameLocal.world ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				// rotational collision with world
				for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
					if ( RotationClipModel( CLIP_DEBUG_PARMS_PASSTHRU rotationalTrace, endPosition, endRotation, mdl->GetTraceModel( j ), trmAxis, contentMask, world ) ) {
						break;
					}
				}
			}
		}
	}

	if ( rotationalTrace.fraction != 0.0f ) {

		if ( num == -1 ) {
			traceBounds.FromBoundsRotation( mdl->GetBounds(), endPosition, trmAxis, endRotation );
			num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, traceMode );
		}

		for ( i = 0; i < num; i++ ) {
			touch = clipModelList[i];

			if ( touch == NULL ) {
				continue;
			}

			if ( touch->IsRenderModel() ) {
				continue;		// no rotational collision detection with render models
			} else {
				for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
					if ( RotationClipModel( CLIP_DEBUG_PARMS_PASSTHRU rotationalTrace, endPosition, endRotation, mdl->GetTraceModel( j ), trmAxis, contentMask, touch ) ) {
						break;
					}
				}
			}
			if ( rotationalTrace.fraction == 0.0f ) {
				break;
			}
		}
	}

	if ( rotationalTrace.fraction < 1.0f ) {
		results = rotationalTrace;
	} else {
		results = translationalTrace;
		results.endAxis = rotationalTrace.endAxis;
	}

	results.fraction = Min( translationalTrace.fraction, rotationalTrace.fraction );
	results.c.selfId = mdl ? mdl->GetId() : -1;

	return ( translationalTrace.fraction < 1.0f || rotationalTrace.fraction < 1.0f );
}

/*
============
idClip::Contacts
============
*/
int idClip::Contacts( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec3 *dir, const float depth,
					 const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {
	int i, j, num, numContacts;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;

	numContacts = 0;

	if ( mdl == NULL ) {
		traceBounds = idBounds( start ).Expand( depth );
	} else {
		traceBounds.FromTransformedBounds( mdl->GetBounds(), start, trmAxis );
		traceBounds.ExpandSelf( depth );
	}

	// NOTE: only comparing and not actually using the passEntity pointer to keep this code thread safe
	if ( passEntity == NULL || passEntity != gameLocal.world ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				if ( !world->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
					continue;
				}

				// test world
				if ( mdl == NULL ) {
					int count = ContactsClipModel( CLIP_DEBUG_PARMS_PASSTHRU contacts + numContacts, maxContacts - numContacts, start, dir, depth, NULL, trmAxis, contentMask, world );
					for ( int i = 0; i < count; i++ ) {
						contacts[ numContacts + i ].selfId = -1;
					}
					numContacts += count;
					if ( numContacts >= maxContacts ) {
						return numContacts;
					}
				} else {
					for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
						int count = ContactsClipModel( CLIP_DEBUG_PARMS_PASSTHRU contacts + numContacts, maxContacts - numContacts, start, dir, depth, mdl->GetTraceModel( j ), trmAxis, contentMask, world );
						for ( int i = 0; i < count; i++ ) {
							contacts[ numContacts + i ].selfId = mdl->GetId();
						}
						numContacts += count;
						if ( numContacts >= maxContacts ) {
							return numContacts;
						}
					}
				}
			}
		}
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, traceMode );

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( touch == NULL ) {
			continue;
		}

		if ( touch->IsRenderModel() ) {
			continue;	// no contacts with render models
		} else if ( mdl == NULL ) {
			int count = ContactsClipModel( CLIP_DEBUG_PARMS_PASSTHRU contacts + numContacts, maxContacts - numContacts, start, dir, depth, NULL, trmAxis, contentMask, touch );
			for ( int i = 0; i < count; i++ ) {
				contacts[ numContacts + i ].selfId = -1;
			}
			numContacts += count;
			if ( numContacts >= maxContacts ) {
				return numContacts;
			}
		} else {
			for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
				int count = ContactsClipModel( CLIP_DEBUG_PARMS_PASSTHRU contacts + numContacts, maxContacts - numContacts, start, dir, depth, mdl->GetTraceModel( j ), trmAxis, contentMask, touch );
				for ( int i = 0; i < count; i++ ) {
					contacts[ numContacts + i ].selfId = mdl->GetId();
				}
				numContacts += count;
				if ( numContacts >= maxContacts ) {
					return numContacts;
				}
			}
		}
	}

	return numContacts;
}

/*
============
idClip::Contents
============
*/
int idClip::Contents( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {
	int i, j, num, contents;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;

	contents = 0;

	if ( mdl == NULL ) {
		traceBounds[0] = start;
		traceBounds[1] = start;
	} else if ( trmAxis.IsRotated() ) {
		traceBounds.FromTransformedBounds( mdl->GetBounds(), start, trmAxis );
	} else {
		traceBounds[0] = mdl->GetBounds()[0] + start;
		traceBounds[1] = mdl->GetBounds()[1] + start;
	}

	// NOTE: only comparing and not actually using the passEntity pointer to keep this code thread safe
	if ( passEntity == NULL || passEntity != gameLocal.world ) {
		for (int i = 0; i < worldClips.Num(); i++) {
			idClipModel *world = worldClips[ i ];
			if ( ( world->GetContents() & contentMask ) != 0 ) {
				if ( !world->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
					continue;
				}

				// test world
				if ( mdl == NULL ) {
					contents |= ContentsClipModel( CLIP_DEBUG_PARMS_PASSTHRU start, NULL, trmAxis, contentMask, world );
				} else {
					for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
						contents |= ContentsClipModel( CLIP_DEBUG_PARMS_PASSTHRU start, mdl->GetTraceModel( j ), trmAxis, contentMask, world );
					}
				}
			}
		}
	}

	num = GetTraceClipModels( traceBounds, -1, passEntity, clipModelList, traceMode );

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( touch == NULL ) {
			continue;
		}

		// no contents test with render models
		if ( touch->IsRenderModel() ) {
			continue;
		}

		// if the entity does not have any contents we are looking for
		if ( ( touch->GetContents() & contentMask ) == 0 ) {
			continue;
		}

		// if the entity has no new contents flags
		if ( ( touch->GetContents() & contents ) == touch->GetContents() ) {
			continue;
		}

		if ( mdl == NULL ) {
			contents |= ContentsClipModel( CLIP_DEBUG_PARMS_PASSTHRU start, NULL, trmAxis, contentMask, touch );
		} else {
			for ( j = 0; j < mdl->GetNumTraceModels(); j++ ) {
				contents |= ContentsClipModel( CLIP_DEBUG_PARMS_PASSTHRU start, mdl->GetTraceModel( j ), trmAxis, contentMask, touch );
			}
		}
	}

	return contents;
}

/*
============
idClip::GetTemporaryClipModel
============
*/
const idClipModel* idClip::GetTemporaryClipModel( const idBounds& bounds ) {
	// FIXME: this creates a new idCollisionModel for each different bounds
	temporaryClipModel->LoadTraceModel( idTraceModel( bounds ), false );
	return temporaryClipModel;
}

/*
============
idClip::GetThirdPersonOffsetModel
============
*/
const idClipModel* idClip::GetThirdPersonOffsetModel( void ) {
	return thirdPersonOffsetModel;
}

/*
============
idClip::GetBigThirdPersonOffsetModel
============
*/
const idClipModel* idClip::GetBigThirdPersonOffsetModel( void ) {
	return bigThirdPersonOffsetModel;
}

/*
============
idClip::GetLeanOffsetModel
============
*/
const idClipModel* idClip::GetLeanOffsetModel( void ) {
	return leanOffsetModel;
}

/*
============
idClip::TraceBounds
============
*/
bool idClip::TraceBounds( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds, const idMat3& axis, 
										int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::TraceBounds called from a thread other than the main thread" );
		return false;
	}
	Translation( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, GetTemporaryClipModel( bounds ), axis, contentMask, passEntity, traceMode );
	return ( results.fraction < 1.0f );
}

/*
============
idClip::TranslationModel
============
*/
void idClip::TranslationModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
					const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
					const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_TRANSLATION );
#endif // CLIP_DEBUG
						
	int i, j;
	trace_t trace;

	results.fraction = 1.0f;

	if ( !( model->GetContents() & contentMask ) ) {
		return;
	}

	if ( trm == NULL ) {
		for ( j = 0; j < model->GetNumCollisionModels(); j++ ) {

			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, NULL, trmAxis, contentMask, model->GetCollisionModel( j ), modelOrigin, modelAxis );

			if ( trace.fraction < results.fraction ) {
				results = trace;
				results.c.entityNum = model->GetEntityNumber();
				results.c.id = model->GetId();
				if ( results.fraction == 0.0f ) {
					return;
				}
			}
		}
	} else {
		if ( !trm->IsTraceModel() ) {
			gameLocal.Error( "idClip::TranslationModel: clip model %d of entity '%s' is not a trace model", trm->GetId(), trm->GetEntityName() );
			return;
		}
		for ( i = 0; i < trm->GetNumTraceModels(); i++ ) {
			for ( j = 0; j < model->GetNumCollisionModels(); j++ ) {

				idClip::numTranslations++;
				collisionModelManager->Translation( &trace, start, end, trm->GetTraceModel( i ), trmAxis, contentMask, model->GetCollisionModel( j ), modelOrigin, modelAxis );

				if ( trace.fraction < results.fraction ) {
					results = trace;
					results.c.entityNum = model->GetEntityNumber();
					results.c.id = model->GetId();
					if ( results.fraction == 0.0f ) {
						return;
					}
				}
			}
		}
	}
}

/*
============
idClip::RotationModelInternal
============
*/
void idClip::RotationModelInternal( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
					const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
					const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_ROTATION );
#endif // CLIP_DEBUG

	int i, j;
	trace_t trace;

	if ( !trm->IsTraceModel() ) {
		gameLocal.Error( "idClip::TranslationModel: clip model %d of entity '%s' is not a trace model", trm->GetId(), trm->GetEntityName() );
		return;
	}

	results.fraction = 1.0f;

	if ( !( model->GetContents() & contentMask ) ) {
		return;
	}

	for ( i = 0; i < trm->GetNumTraceModels(); i++ ) {
		for ( j = 0; j < model->GetNumCollisionModels(); j++ ) {

			idClip::numRotations++;
			collisionModelManager->Rotation( &trace, start, rotation, trm->GetTraceModel( i ), trmAxis, contentMask, model->GetCollisionModel( j ), modelOrigin, modelAxis );

			if ( trace.fraction < results.fraction ) {
				results = trace;
				results.c.entityNum = model->GetEntityNumber();
				results.c.id = model->GetId();
				if ( results.fraction == 0.0f ) {
					return;
				}
			}
		}
	}
}

/*
============
idClip::ContactsModel
============
*/
int idClip::ContactsModel( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec3 *dir, const float depth,
					const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
					const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_CONTACTS );
#endif // CLIP_DEBUG

	int i, j, numContacts;
	trace_t trace;

	if ( !trm->IsTraceModel() ) {
		gameLocal.Error( "idClip::TranslationModel: clip model %d of entity '%s' is not a trace model", trm->GetId(), trm->GetEntityName() );
		return 0;
	}

	if ( !( model->GetContents() & contentMask ) ) {
		return 0;
	}

	numContacts = 0;
	for ( i = 0; i < trm->GetNumTraceModels(); i++ ) {
		for ( j = 0; j < model->GetNumCollisionModels(); j++ ) {

			idClip::numContacts++;
			numContacts += collisionModelManager->Contacts( contacts + numContacts, maxContacts - numContacts,
								start, dir, depth, trm->GetTraceModel( i ), trmAxis, contentMask, model->GetCollisionModel( j ), modelOrigin, modelAxis );
		}
	}
	return numContacts;
}

/*
============
idClip::ContentsModel
============
*/
int idClip::ContentsModel( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
					const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
					const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {

#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU this, CTM_CONTENTS );
#endif // CLIP_DEBUG
						
	int i, j, contents;
	trace_t trace;

	if ( trm && !trm->IsTraceModel() ) {
		gameLocal.Error( "idClip::TranslationModel: clip model %d of entity '%s' is not a trace model", trm->GetId(), trm->GetEntityName() );
		return 0;
	}

	if ( !( model->GetContents() & contentMask ) ) {
		return 0;
	}

	contents = 0;
	if ( trm ) {
		for ( i = 0; i < trm->GetNumTraceModels(); i++ ) {
			for ( j = 0; j < model->GetNumCollisionModels(); j++ ) {
				idClip::numContents++;
				contents |= collisionModelManager->Contents( start, trm->GetTraceModel( i ), trmAxis, contentMask, model->GetCollisionModel( j ), modelOrigin, modelAxis );
			}
		}
	} else {
		for ( j = 0; j < model->GetNumCollisionModels(); j++ ) {
			idClip::numContents++;
			contents |= collisionModelManager->Contents( start, NULL, trmAxis, contentMask, model->GetCollisionModel( j ), modelOrigin, modelAxis );
		}
	}
	return contents;
}

/*
============
idClip::GetModelContactFeature
============
*/
bool idClip::GetModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, idFixedWinding &winding ) const {
	int i;
	idCollisionModel *model;
	idVec3 start, end;

	model = NULL;
	winding.Clear();

	if ( clipModel == NULL ) {
		return false;
	}
	if ( clipModel->IsRenderModel() ) {
		winding += contact.point;
		return true;
	} else {
		model = clipModel->GetCollisionModel();
	}

	// if contact with a collision model
	if ( model != NULL ) {
		switch( contact.type ) {
			case CONTACT_EDGE: {
				// the model contact feature is a collision model edge
				model->GetEdge( contact.modelFeature, start, end );
				winding += start;
				winding += end;
				break;
			}
			case CONTACT_MODELVERTEX: {
				// the model contact feature is a collision model vertex
				start = model->GetVertex( contact.modelFeature );
				winding += start;
				break;
			}
			case CONTACT_TRMVERTEX: {
				// the model contact feature is a collision model polygon
				model->GetPolygon( contact.modelFeature, winding );
				break;
			}
		}
	}

	// transform the winding to world space
	if ( clipModel ) {
		for ( i = 0; i < winding.GetNumPoints(); i++ ) {
			winding[i].ToVec3() *= clipModel->GetAxis();
			winding[i].ToVec3() += clipModel->GetOrigin();
		}
	}

	return true;
}

/*
============
idClip::PrintStatistics
============
*/
void idClip::PrintStatistics( void ) {
	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::PrintStatistics called from a thread other than the main thread" );
		return;
	}
	gameLocal.Printf( "t = %-3d, r = %-3d, m = %-3d, render = %-3d, contents = %-3d, contacts = %-3d\n",
					numTranslations, numRotations, numMotions, numRenderModelTraces, numContents, numContacts );
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
============
idClip::DrawWorld
============
*/
void idClip::DrawWorld( float radius ) {
	if ( g_showCollisionWorld.GetInteger() & 1 ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			world->Draw( vec3_origin, mat3_identity, radius );
		}
	}
	if ( g_showCollisionWorld.GetInteger() & 2 ) {
		for ( int i = 0; i < worldClips.Num(); i++ ) {
			idClipModel *world = worldClips[ i ];
			gameRenderWorld->DebugBox( colorRed, idBox( world->GetBounds() ) );
		}
	}
}

/*
============
idClip::DrawClipModels
============
*/
void idClip::DrawClipModels( const idVec3 &viewOrigin, const idMat3 &viewAxis, const float radius, const idEntity *passEntity ) {
	int				i, j, num;
	idBounds		bounds;
	const idClipModel*	clipModelList[MAX_GENTITIES];
	const idClipModel*		clipModel;

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::DrawClipModels called from a thread other than the main thread" );
		return;
	}

	bounds = idBounds( viewOrigin ).Expand( radius );

	num = idClip::ClipModelsTouchingBounds( CLIP_DEBUG_PARMS bounds, -1, clipModelList, MAX_GENTITIES, passEntity );

	for ( i = 0; i < num; i++ ) {
		clipModel = clipModelList[i];

		if ( !( clipModel->GetContents() & g_collisionModelMask.GetInteger() ) ) {
			continue;
		}
		if ( clipModel->IsRenderModel() ) {
			gameRenderWorld->DebugBounds( colorCyan, clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis() );
		} else {
			for ( j = 0; j < clipModel->GetNumCollisionModels(); j++ ) {
				collisionModelManager->DrawModel( clipModel->GetCollisionModel( j ), clipModel->GetOrigin(), clipModel->GetAxis(), viewOrigin, viewAxis, radius, 0.0f );
			}
		}
	}
}

/*
============
idClip::DrawModelContactFeature
============
*/
bool idClip::DrawModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, int lifetime ) const {
	int i;
	idMat3 axis;
	idFixedWinding winding;

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::DrawModelContactFeature called from a thread other than the main thread" );
		return false;
	}

	if ( !GetModelContactFeature( contact, clipModel, winding ) ) {
		return false;
	}

	axis = contact.normal.ToMat3();

	if ( winding.GetNumPoints() == 1 ) {
		gameRenderWorld->DebugLine( colorCyan, winding[0].ToVec3(), winding[0].ToVec3() + 2.0f * axis[0], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[1], winding[0].ToVec3() + 1.0f * axis[1], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[2], winding[0].ToVec3() + 1.0f * axis[2], lifetime );
	} else {
		for ( i = 0; i < winding.GetNumPoints(); i++ ) {
			gameRenderWorld->DebugLine( colorCyan, winding[i].ToVec3(), winding[(i+1)%winding.GetNumPoints()].ToVec3(), lifetime );
		}
	}

	axis[0] = -axis[0];
	axis[2] = -axis[2];
	gameRenderWorld->DrawText( contact.material->GetName(), winding.GetCenter() - 4.0f * axis[2], 0.1f, colorWhite, axis, 1, 5000 );

	return true;
}

/*
============
idClip::PrecacheModel
============
*/
void idClip::PrecacheModel( const char* clipModelName ) {
	if ( !*clipModelName ) {
		return;
	}

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::PrecacheModel called from a thread other than the main thread" );
		return;
	}

	idCollisionModel* model = collisionModelManager->LoadModel( gameLocal.GetMapName(), clipModelName );

	if ( model ) {
		idTraceModel trm;
		LoadTraceModel( clipModelName, trm );
	}

	collisionModelManager->FreeModel( model );
}

#if !defined( _XENON ) && !defined( MONOLITHIC )
idCVar cm_writeCompiledCollisionModels( "cm_writeCompiledCollisionModels", "0", CVAR_BOOL, "write out generated collision models to disk" );
#else
extern idCVar cm_writeCompiledCollisionModels;
#endif

/*
============
idClip::LoadTraceModel
============
*/
bool idClip::LoadTraceModel( const char* clipModelName, idTraceModel& trm ) {
	idStr fileName;
	collisionModelManager->GetFullModelName( fileName, gameLocal.GetMapName(), clipModelName );

	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::LoadTraceModel called from a thread other than the main thread" );
		return false;
	}

	fileName = PREGENERATED_BASEDIR "/trm/" + fileName;

	fileName.SetFileExtension( ".trm" );

	int index = gameLocal.traceModelCache.FindTraceModel( fileName.c_str(), false );
	if ( index != -1 ) {
		trm = *gameLocal.traceModelCache.GetTraceModel( index );
		return true;
	}

	idFile* fp = fileSystem->OpenFileRead( fileName.c_str() );
	if ( !fp ) {
		if ( !collisionModelManager->TrmFromModel( gameLocal.GetMapName(), clipModelName, trm ) ) {
			return false;
		}
		index = gameLocal.traceModelCache.PrecacheTraceModel( trm, fileName );

		if ( cm_writeCompiledCollisionModels.GetBool() ) {
			fp = fileSystem->OpenFileWrite( fileName.c_str() );
			if ( fp ) {
				gameLocal.traceModelCache.Write( index, fp );

				fileSystem->CloseFile( fp );
			}
		}
		return true;
	} else {
#if defined( SD_BUFFERED_FILE_LOADS )
		fp = fileSystem->OpenBufferedFile( fp );
#endif
		gameLocal.traceModelCache.Read( trm, fp );
		fileSystem->CloseFile( fp );
		return true;
	}

	return false;
}

/*
============
idClip::AllocThread
============
*/
void idClip::AllocThread( void ) {
	Lock();
	collisionModelManager->AllocThread();
	Unlock();
}

/*
============
idClip::FreeThread
============
*/
void idClip::FreeThread( void ) {
	Lock();
	collisionModelManager->FreeThread();
	Unlock();
}

/*
============
idClip::DeleteClipModel
============
*/
void idClip::DeleteClipModel( idClipModel *clipModel ) {
	if ( clipModel != NULL ) {
		if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
			common->FatalError( "idClip::DeleteClipModel called from a thread other than the main thread" );
			return;
		}
		// unlink the clip model
		clipModel->Unlink( *this );

		// remove the clip model to the list with to be removed clip models
		Lock();
		clipModel->entity = NULL;	// make sure there's no stale entity pointer
		// FIXME: the thread count should really be a set of bit flags such that each thread can sign off on the clip model
		// setting the thread count high enough such that at least one bot think passed works for now though
		clipModel->deleteThreadCount = collisionModelManager->GetThreadCount() + bot_threadMaxFrameDelay.GetInteger();
		clipModel->nextDeleted = deletedClipModels;
		deletedClipModels = clipModel;
		Unlock();
	}
}

/*
============
idClip::ThreadDeleteClipModels
============
*/
void idClip::ThreadDeleteClipModels( void ) {
	Lock();
	idClipModel *clipModel;
	for ( clipModel = deletedClipModels; clipModel != NULL; clipModel = clipModel->nextDeleted ) {
		clipModel->deleteThreadCount--;
	}
	Unlock();
}

/*
============
idClip::ActuallyDeleteClipModels
============
*/
void idClip::ActuallyDeleteClipModels( bool force ) {
	// jrad - this can be called during static initialization, guard appropriately
	if( collisionModelManager == NULL || collisionModelManager->GetThreadId() == 0 ) {
		return;
	}
	if ( collisionModelManager->GetThreadId() != MAIN_THREAD_ID ) {
		common->FatalError( "idClip::ActuallyDeleteClipModels called from a thread other than the main thread" );
		return;
	}
	Lock();
	for ( idClipModel **clipModel = &deletedClipModels; *clipModel != NULL; ) {
		if ( (*clipModel)->deleteThreadCount <= 0 || force ) {
			idClipModel *remove = *clipModel;
			*clipModel = (*clipModel)->nextDeleted;
			delete remove;
		} else {
			clipModel = &(*clipModel)->nextDeleted;
		}
	}
	Unlock();
}

/*
============
idClip::ContentsWorld
============
*/
int idClip::ContentsWorld( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) {

	int contents = 0;
	for ( int i = 0; i < worldClips.Num(); i++ ) {
		idClipModel *world = worldClips[ i ];
		if ( ( world->GetContents() & contentMask ) != 0 ) {
			// test world
			if ( mdl == NULL ) {
				contents |= ContentsClipModel( CLIP_DEBUG_PARMS_PASSTHRU start, NULL, trmAxis, contentMask, world );
			} else {
				for ( int j = 0; j < mdl->GetNumTraceModels(); j++ ) {
					contents |= ContentsClipModel( CLIP_DEBUG_PARMS_PASSTHRU start, mdl->GetTraceModel( j ), trmAxis, contentMask, world );
				}
			}
		}
	}

	return contents;
}

/*
============
idClip::TranslationWorld
============
*/
void idClip::TranslationWorld( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, const idClipModel *trm, const idMat3 &trmAxis, int contentMask ) {
	results.fraction = 1.f;
	for ( int i = 0; i < worldClips.Num(); i++ ) {
		trace_t trace;
		TranslationModel( CLIP_DEBUG_PARMS_PASSTHRU trace, start, end, trm, trmAxis, contentMask, worldClips[ i ], vec3_origin, mat3_identity );
		if ( trace.fraction < results.fraction ) {
			results = trace;
		}
	}
}

/*
============
idClip::FindLadder
============
*/
int idClip::FindLadder( CLIP_DEBUG_PARMS_DECLARATION const idBounds& bounds, sdLadderEntity** ladderList, int maxLadders ) {
#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU const_cast< idClip* >( this ), CTM_FINDLADDER );
#endif // CLIP_DEBUG

	idBounds parms( bounds[ 0 ] - vec3_boxEpsilon, bounds[ 1 ] + vec3_boxEpsilon );

	assert( maxLadders > 0 );

	int coords[ 4 ];
	CoordsForBounds( coords, parms );

	int numLadders = 0;

	sdInstanceCollector< sdLadderEntity > ladders( true );
	for ( int i = 0; i < ladders.Num(); i++ ) {
		sdLadderEntity* ladder =ladders[ i ];
		if ( !ladder->IsActive() ) {
			continue;
		}

		const idClipModel* model = ladder->GetPhysics()->GetClipModel();

		if ( !model->IsLinked() ) {
			assert( false );
			continue;
		}

		if ( model->lastLinkCoords[ 2 ] < coords[ 0 ] ) {
			continue;
		}

		if ( model->lastLinkCoords[ 0 ] > coords[ 2 ] ) {
			continue;
		}

		if ( model->lastLinkCoords[ 3 ] < coords[ 1 ] ) {
			continue;
		}

		if ( model->lastLinkCoords[ 1 ] > coords[ 3 ] ) {
			continue;
		}

		if ( !model->GetAbsBounds().IntersectsBounds( bounds ) ) {
			continue;
		}

		ladderList[ numLadders++ ] = ladder;
		if ( numLadders == maxLadders ) {
			break;
		}
	}

	return numLadders;
}

/*
============
idClip::FindWater
============
*/
int idClip::FindWater( CLIP_DEBUG_PARMS_DECLARATION const idBounds& bounds, const idClipModel** clipModelList, int maxCount ) {
#ifdef CLIP_DEBUG
	idClipTimer timer( CLIP_DEBUG_PARMS_PASSTHRU const_cast< idClip* >( this ), CTM_FINDWATER );
#endif // CLIP_DEBUG

	idBounds parms( bounds[ 0 ] - vec3_boxEpsilon, bounds[ 1 ] + vec3_boxEpsilon );

	int coords[ 4 ];
	CoordsForBounds( coords, parms );

	sdInstanceCollector< sdLiquid > liquids( true );
	int count = 0;
	for ( int i = 0; i < liquids.Num(); i++ ) {
		sdLiquid* liquid = liquids[ i ];
		const idClipModel* model = liquid->GetPhysics()->GetClipModel();

		if ( !model->IsLinked() ) {
			assert( false );
			continue;
		}

		if ( model->lastLinkCoords[ 2 ] < coords[ 0 ] ) {
			continue;
		}

		if ( model->lastLinkCoords[ 0 ] > coords[ 2 ] ) {
			continue;
		}

		if ( model->lastLinkCoords[ 3 ] < coords[ 1 ] ) {
			continue;
		}

		if ( model->lastLinkCoords[ 1 ] > coords[ 3 ] ) {
			continue;
		}

		if ( model->GetAbsBounds().IntersectsBounds( bounds ) ) {
			clipModelList[ count++ ] = model;
			if ( count == maxCount ) {
				break;
			}
		}
	}

	return count;
}

#ifdef CLIP_DEBUG

idCVar g_enableTraceLogging( "g_enableTraceLogging", "0", CVAR_BOOL | CVAR_GAME, "" );

const char* traceTimerModeText[] = {
	"Translation",
	"Rotation",
	"Contacts",
	"Contents",
	"ClipModelsTouchingBounds",
	"FindLadder",
	"FindWater"
};

void idClip::LogTrace( const char* fileName, int lineNumber, double time, clipTimerMode_t mode ) {
	if ( !g_enableTraceLogging.GetBool() ) {
		return;
	}

	static char tempFileName[ 512 ];
	if ( !gameLocal.isNewFrame ) {
		sprintf( tempFileName, "REP %s", fileName );
	} else {
		sprintf( tempFileName, "%s", fileName );
	}

	idList< clipTimerInfo_t >* info;
	if ( !clipTimerInfo[ mode ].Get( tempFileName, &info ) ) {
		clipTimerInfo[ mode ].Set( tempFileName, idList< clipTimerInfo_t >() );
		clipTimerInfo[ mode ].Get( tempFileName, &info );
	}

	idList< clipTimerInfo_t >& infoList = *info;

	for ( int i = 0; i < infoList.Num(); i++ ) {
		if ( infoList[ i ].lineNumber == lineNumber ) {
			infoList[ i ].count++;
			infoList[ i ].time += time;
			return;
		}
	}

	clipTimerInfo_t& data = infoList.Alloc();
	data.count = 1;
	data.time = time;
	data.lineNumber = lineNumber;
}

void idClip::PrintTraceTimings( void ) {

	for ( int i = 0; i < CTM_NUM; i++ ) {
		gameLocal.Printf( "%s:\n", traceTimerModeText[ i ] );

		for ( int j = 0; j < clipTimerInfo[ i ].Num(); j++ ) {
			idList< clipTimerInfo_t >& infoList = *clipTimerInfo[ i ].GetIndex( j );

			for ( int k = 0; k < infoList.Num(); k++ ) {
				gameLocal.Printf( "File: \"%s\" Line: %d Count: %d Time: %d\n", clipTimerInfo[ i ].GetKey( j ).c_str(), infoList[ k ].lineNumber, infoList[ k ].count, ( int )infoList[ k ].time );
			}
		}

		gameLocal.Printf( "==========================\n\n", traceTimerModeText[ i ] );
	}
}

void idClip::ClearTraceTimings( void ) {
	for ( int i = 0; i < CTM_NUM; i++ ) {
		clipTimerInfo[ i ].Clear();
	}
}

#endif // CLIP_DEBUG



#ifdef CLIP_DEBUG_EXTREME

void idClip::LogTraceExtreme( const char* fileName, int lineNumber, double time, clipTimerMode_t mode ) {
	if ( !isTraceLogging ) {
		return;
	}

	char name[ 512 ];
	sprintf( name, "%s: %i", fileName, lineNumber );

	clipTimerInfoExtreme_t** infoPtr;
	clipTimerInfoExtreme_t* info;
	if ( !clipTimerInfoExtreme[ mode ].Get( name, &infoPtr ) ) {
		info = new clipTimerInfoExtreme_t;
		for ( int i = 0; i < CLIP_DEBUG_MAX_FRAMES; i++ ) {
			info->frameInfo[ i ].count = 0;
			info->frameInfo[ i ].elapsedTime = 0.0;
		}

		clipTimerInfoExtreme[ mode ].Set( name, info );
		clipTimerInfoExtreme[ mode ].Get( name, &infoPtr );
	}

	info = *infoPtr;
	info->frameInfo[ clipSampleUpto ].count++;
	info->frameInfo[ clipSampleUpto ].elapsedTime += time;
}

void idClip::StartTraceLogging( void ) {
	if ( isTraceLogging ) {
		return;
	}

	// clear the list
	for ( int mode = 0; mode < CTM_NUM; mode++ ) {
		for ( int i = 0; i < clipTimerInfoExtreme[ mode ].Num(); i++ ) {
			delete *clipTimerInfoExtreme[ mode ].GetIndex( i );
		}
		clipTimerInfoExtreme[ mode ].Clear();
	}

	isTraceLogging = true;
	clipSampleUpto = 0;
	logSubFileUpto = 0;
	logFileUpto++;

	gameLocal.DPrintf( "Started trace logging.\n" );
}

void idClip::StopTraceLogging( void ) {
	if ( !isTraceLogging ) {
		return;
	}

	isTraceLogging = false;

	DumpTraceLog();
	AssembleTraceLogs();

	gameLocal.DPrintf( "Stopped trace logging.\n" );
}

void idClip::UpdateTraceLogging( void ) {
	if ( !isTraceLogging ) {
		return;
	}

	clipSampleUpto++;
	if ( clipSampleUpto == CLIP_DEBUG_MAX_FRAMES ) {
		DumpTraceLog();
	}

	clipTimerSampleTimes[ clipSampleUpto ] = gameLocal.time;
}

void idClip::DumpTraceLog( void ) {
	for ( int i = 0; i < CTM_NUM; i++ ) {
		DumpTraceLog( i );
	}

	logSubFileUpto++;
	clipSampleUpto = 1;
}

void idClip::DumpTraceLog( int mode ) {
	const char* fileName;

	// dump a mini-log out
	fileName = va( "tr_%s_mini_time_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, logSubFileUpto );
	idFile* miniTimeLog = fileSystem->OpenFileWrite( fileName );
	if ( miniTimeLog == NULL ) {
		gameLocal.Warning( "idClip::DumpTraceLog - failed to open %s", fileName );
		return;
	}

	fileName = va( "tr_%s_mini_count_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, logSubFileUpto );
	idFile* miniCountLog = fileSystem->OpenFileWrite( fileName );
	if ( miniCountLog == NULL ) {
		gameLocal.Warning( "idClip::DumpTraceLog - failed to open %s", fileName );
		fileSystem->CloseFile( miniTimeLog );
		return;
	}

	// output number of columns
	miniTimeLog->WriteInt( clipTimerInfoExtreme[ mode ].Num() + 2 );
	miniCountLog->WriteInt( clipTimerInfoExtreme[ mode ].Num() + 2 );

	// print line of titles
	miniTimeLog->WriteString( "gameLocal.time" );
	miniCountLog->WriteString( "gameLocal.time" );
	for ( int i = 0; i < clipTimerInfoExtreme[ mode ].Num(); i++ ) {
		miniTimeLog->WriteString( va( "%s", clipTimerInfoExtreme[ mode ].GetKey( i ).c_str() ) );
		miniCountLog->WriteString( va( "%s", clipTimerInfoExtreme[ mode ].GetKey( i ).c_str() ) );
	}
	miniTimeLog->WriteString( "TOTAL" );
	miniCountLog->WriteString( "TOTAL" ); 
	
	// print data, row by row
	for ( int sample = 1; sample < clipSampleUpto; sample++ ) {
		miniTimeLog->WriteInt( clipTimerSampleTimes[ sample ] );
		miniCountLog->WriteInt( clipTimerSampleTimes[ sample ] );

		double totalTime = 0.0;
		int totalCount = 0;

		for ( int i = 0; i < clipTimerInfoExtreme[ mode ].Num(); i++ ) {
			clipTimerInfoExtreme_t* info = *clipTimerInfoExtreme[ mode ].GetIndex( i );
			miniTimeLog->WriteFloat( ( float )info->frameInfo[ sample ].elapsedTime );
			miniCountLog->WriteInt( info->frameInfo[ sample ].count );

			totalTime += info->frameInfo[ sample ].elapsedTime;
			totalCount += info->frameInfo[ sample ].count;

			info->frameInfo[ sample ].elapsedTime = 0.0;
			info->frameInfo[ sample ].count = 0;
		}
		
		miniTimeLog->WriteFloat( ( float )totalTime );
		miniCountLog->WriteInt( totalCount );
	}

	fileSystem->CloseFile( miniCountLog );
	fileSystem->CloseFile( miniTimeLog );
}

void idClip::AssembleTraceLogs( void ) {
	for ( int i = 0; i < CTM_NUM; i++ ) {
		AssembleTraceLogs( i );
	}
}

void idClip::AssembleTraceLogs( int mode ) {
	// assemble all the logs for this session into one big csv file & delete the mini-logs
	// first find all the logs & collate a list of titles
	idList< idStr > titles;
	int numLogs = 0;

	//
	// Initialize the titles list
	//
	while( 1 ) {
		idFile* miniLog = fileSystem->OpenFileRead( va( "tr_%s_mini_time_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, numLogs ) );
		idFile* miniCountLog = fileSystem->OpenFileRead( va( "tr_%s_mini_count_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, numLogs ) );
		if ( miniLog != NULL && miniCountLog != NULL ) {
			int numColumns;
			miniLog->ReadInt( numColumns );
			miniCountLog->ReadInt( numColumns );
			for ( int i = 0; i < numColumns; i++ ) {
				idStr title, title2;
				miniLog->ReadString( title );
				titles.AddUnique( title );

				miniCountLog->ReadString( title2 );
				if ( title != title2 ) {
					gameLocal.Warning( "i.dClip::AssembleTraceLogs - count & time titles do not match!" );
					return;
				}
			}
			numLogs++;
		}

		if ( miniCountLog ) {
			fileSystem->CloseFile( miniCountLog );
		}
		if ( miniLog ) {
			fileSystem->CloseFile( miniLog );
		}

		if ( miniCountLog == NULL || miniLog == NULL ) {
			break;
		}
	}

	if ( numLogs == 0 ) {
		gameLocal.Warning( "idClip::AssembleTraceLogs - no mini logs!" );
		return;
	}

	//
	// Sort the titles list, keeping gameLocal.time & TOTAL at the start
	//
	titles.RemoveFast( idStr( "gameLocal.time" ) );
	titles.RemoveFast( idStr( "TOTAL" ) );
	titles.Sort();
	titles.Insert( idStr( "TOTAL" ), 0 );
	titles.Insert( idStr( "gameLocal.time" ), 0 );

	//
	// Write the titles list
	//
	idFile* log = fileSystem->OpenFileWrite( va( "tr_%s_time_%i.csv", traceTimerModeText[ mode ], logFileUpto ) );
	if ( log == NULL ) {
		gameLocal.Warning( "idClip::AssembleTraceLogs - couldn't open file %s", va( "tr_%s_time_%i.csv", traceTimerModeText[ mode ], logFileUpto ) );
		return;
	}
	idFile* countLog = fileSystem->OpenFileWrite( va( "tr_%s_count_%i.csv", traceTimerModeText[ mode ], logFileUpto ) );
	if ( countLog == NULL ) {
		gameLocal.Warning( "idClip::AssembleTraceLogs - couldn't open file %s", va( "tr_%s_count_%i.csv", traceTimerModeText[ mode ], logFileUpto ) );
		fileSystem->CloseFile( log );
		return;
	}

	for ( int i = 0; i < titles.Num(); i++ ) {
		log->Printf( "\"%s\",", titles[ i ].c_str() );
		countLog->Printf( "\"%s\",", titles[ i ].c_str() );
	}
	log->Printf( "\n" );
	countLog->Printf( "\n" );

	//
	// Loop through the logs & write them all into the main log
	//
	idList< int > miniLogColumnToLogColumn;
	idList< int > logColumnToMiniLogColumn;
	idList< float > columnTimeValues;
	idList< int > columnCountValues;
	logColumnToMiniLogColumn.AssureSize( titles.Num() );

	for ( int logNum = 0; logNum < numLogs; logNum++ ) {
		idFile* miniLog = fileSystem->OpenFileRead( va( "tr_%s_mini_time_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, logNum ) );
		idFile* miniCountLog = fileSystem->OpenFileRead( va( "tr_%s_mini_count_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, logNum ) );
		if ( miniLog != NULL && miniCountLog != NULL ) {

			// create translation tables to & from the mini log & main log table
			int numColumns;
			miniLog->ReadInt( numColumns );
			miniCountLog->ReadInt( numColumns );
			miniLogColumnToLogColumn.AssureSize( numColumns );
			for ( int i = 0; i < titles.Num(); i++ ) {
				logColumnToMiniLogColumn[ i ] = -1;
			}
			for ( int i = 0; i < numColumns; i++ ) {
				idStr title, title2;
				miniLog->ReadString( title );
				miniCountLog->ReadString( title2 );
				miniLogColumnToLogColumn[ i ] = titles.FindIndex( title );
				logColumnToMiniLogColumn[ miniLogColumnToLogColumn[ i ] ] = i;
			}

			// go through all the samples and write them into the main file
			columnTimeValues.AssureSize( numColumns );
			columnCountValues.AssureSize( numColumns );
			while ( miniLog->Tell() < miniLog->Length() - 1 ) {
				int time;
				miniLog->ReadInt( time );
				miniCountLog->ReadInt( time );

				// read the samples
				for ( int i = 1; i < numColumns; i++ ) {
					miniLog->ReadFloat( columnTimeValues[ i ] );
					miniCountLog->ReadInt( columnCountValues[ i ] );
				}

				// write the time samples
				log->Printf( "%i,", time );
				countLog->Printf( "%i,", time );
				for ( int i = 1; i < titles.Num(); i++ ) {
					if ( logColumnToMiniLogColumn[ i ] == -1 ) {
						log->Printf( "0," );
						countLog->Printf( "0," );
					} else {
						log->Printf( "%.6f,", columnTimeValues[ logColumnToMiniLogColumn[ i ] ] );
						countLog->Printf( "%i,", columnCountValues[ logColumnToMiniLogColumn[ i ] ] );
					}
				}

				log->Printf( "\n" );
				countLog->Printf( "\n" );
			}
		}

		if ( miniLog ) {
			fileSystem->CloseFile( miniLog );
		}
		if ( miniCountLog ) {
			fileSystem->CloseFile( miniCountLog );
		}
		fileSystem->RemoveFile( va( "tr_%s_mini_count_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, logNum ) );
		fileSystem->RemoveFile( va( "tr_%s_mini_time_%i_%i.temp", traceTimerModeText[ mode ], logFileUpto, logNum ) );
	}

	fileSystem->CloseFile( log );
	fileSystem->CloseFile( countLog );
}

#endif // CLIP_DEBUG_EXTREME
