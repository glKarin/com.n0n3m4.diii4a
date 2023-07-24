// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "RouteConstraint.h"
#include "../structures/DeployMask.h"
#include "../Player.h"

idCVar g_drawRouteConstraints( "g_drawRouteConstraints", "0", CVAR_INTEGER | CVAR_GAME, "draws lines showing route constraints" );
idCVar g_noRouteConstraintKick( "g_noRouteConstraintKick", "0", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT | CVAR_RANKLOCKED, "enables/disables players being kicked for deviating from routes" );
idCVar g_noRouteMaskDestruction( "g_noRouteMaskDestruction", "0", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT | CVAR_RANKLOCKED, "enables/disables the mcp being destroyed when driven outside the mask" );

/*
===============================================================================

  sdRouteConstraintController

===============================================================================
*/

const idEventDefInternal EV_Link( "internal_link" );

CLASS_DECLARATION( idEntity, sdRouteConstraintController )
	EVENT( EV_Link,					sdRouteConstraintController::Event_Link )
END_CLASS


/*
================
sdRouteConstraintController::sdRouteConstraintController
================
*/
sdRouteConstraintController::sdRouteConstraintController() {
	linked = false;
	startPoint = NULL;
	endPoint = NULL;
	warningPointDeviance = 0;
	maxPointDeviance = 0;
}

/*
================
sdRouteConstraintController::~sdRouteConstraintController
================
*/
sdRouteConstraintController::~sdRouteConstraintController() {
	for ( int i = 0; i < points.Num(); i++ ) {
		sdRoutePoint::Free( points[ i ] );
	}
	points.Clear();
}

/*
================
sdRouteConstraintController::Spawn
================
*/
void sdRouteConstraintController::Spawn( void ) {
	PostEventMS( &EV_Link, 0 ); // Gordon: we want to handle this after OnPostMapSpawn

	const char* maskName;
	if ( !spawnArgs.GetString( "mask", "", &maskName ) ) {
		gameLocal.Error( "sdRouteConstraintController::Spawn No Mask Specified" );
	}
	maskHandle = gameLocal.GetDeploymentMask( maskName );
	if ( maskHandle == -1 ) {
		gameLocal.Error( "sdRouteConstraintController::Spawn Invalid Mask '%s' Specified", maskName );
	}

	warningPointDeviance = spawnArgs.GetInt( "num_points_warning", "3" );
	maxPointDeviance = spawnArgs.GetInt( "num_points_max", "6" );

	const char* directionalModelName;
	if ( !spawnArgs.GetString( "model_directional", "", &directionalModelName ) ) {
		gameLocal.Error( "sdRouteConstraintController::Spawn No Direction Arrow Specified" );
	}

	directionalModel = renderModelManager->FindModel( directionalModelName );
	directionalModelSkin = gameLocal.declSkinType[ spawnArgs.GetString( "skin_direction_model" ) ];
	directionalModelColor = spawnArgs.GetVector( "directional_model_color" );
}

/*
================
sdRouteConstraintController::Display
================
*/
void sdRouteConstraintController::Display( sdRouteConstraintTracker* tracker ) {
	idEntity* entity = tracker->GetEntity();
	const idVec3& currentPosition = entity->GetPhysics()->GetOrigin();

	SetRenderPoint( tracker, FindPoint( currentPosition ) );
}

/*
================
sdRouteConstraintController::SetRenderPoint
================
*/
void sdRouteConstraintController::SetRenderPoint( sdRouteConstraintTracker* tracker, sdRoutePoint* newPoint ) {
	if ( tracker->GetRenderPoint() == newPoint ) {
		return;
	}
	tracker->SetRenderPoint( newPoint );

	sdRoutePoint::renderList_t& renderList = tracker->GetRenderList();
	for ( int i = 0; i < renderList.Num(); i++ ) {
		renderList[ i ].Hide();
	}

	int count = newPoint ? newPoint->GetChildren().Num() : 0;
	renderList.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		const sdRoutePoint* child = newPoint->GetChildren()[ i ];

		idVec3 middle = Lerp( newPoint->GetOrigin(), child->GetOrigin(), 0.75f );
		idVec3 direction = child->GetOrigin() - newPoint->GetOrigin();
		direction.Normalize();

		renderEntity_t& renderEnt = renderList[ i ].GetEntity();
		renderEnt.origin = middle;
		renderEnt.axis = direction.ToMat3();

		//gameRenderWorld->DebugAxis( renderEnt.origin, renderEnt.axis );

		renderEnt.hModel = directionalModel;
		renderEnt.customSkin = directionalModelSkin;

		renderEnt.shaderParms[ SHADERPARM_RED ]		= directionalModelColor.x;
		renderEnt.shaderParms[ SHADERPARM_GREEN ]	= directionalModelColor.y;
		renderEnt.shaderParms[ SHADERPARM_BLUE ]	= directionalModelColor.z;
		renderEnt.shaderParms[ SHADERPARM_ALPHA ]	= 1.f;

		renderList[ i ].Show();
	}
}

/*
================
sdRouteConstraintController::Update
================
*/
void sdRouteConstraintController::Update( sdRouteConstraintTracker* tracker ) {
	idEntity* entity = tracker->GetEntity();

	int debugValue = g_drawRouteConstraints.GetInteger();
	bool drawDebug = false;
	if ( debugValue == 1 ) {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer != NULL ) {
			if ( localPlayer->GetProxyEntity() == entity ) {
				drawDebug = true;
			}
		}
	} else if ( debugValue > 1 ) {
		drawDebug = true;
	}

	if ( drawDebug ) {
		if ( linked ) {
			for ( int i = 0; i < points.Num(); i++ ) {
				const sdRoutePoint* point = points[ i ];
				const sdRoutePoint::nodeList_t& children = point->GetChildren();
				for ( int j = 0; j < children.Num(); j++ ) {
					const sdRoutePoint* otherPoint = children[ j ];

					gameRenderWorld->DebugLine( colorRed, point->GetOrigin(), otherPoint->GetOrigin() );
				}
			}

			const sdRoutePoint* current = tracker->GetCurrentPoint();
			if ( current != NULL ) {
				gameRenderWorld->DebugBounds( colorRed, idBounds( idSphere( vec3_origin, 16 ) ), current->GetOrigin() );
			}
			const sdRoutePoint* best = tracker->GetBestPoint();
			if ( best != NULL ) {
				gameRenderWorld->DebugSphere( colorGreen, idSphere( best->GetOrigin(), 16 ) );
			}
			const sdRoutePoint* reserve = tracker->GetBestReserve();
			if ( reserve != NULL ) {
				gameRenderWorld->DebugSphere( colorBlue, idSphere( reserve->GetOrigin(), 32 ) );
			}
		}
	}

	const idVec3& currentPosition = entity->GetPhysics()->GetOrigin();

	bool positionValid = false;
	if ( !g_noRouteMaskDestruction.GetBool() ) {
		const sdPlayZone* pz = gameLocal.GetPlayZone( currentPosition, sdPlayZone::PZF_VEHICLEROUTE );
		if ( pz != NULL ) {
			const sdDeployMaskInstance* mask = pz->GetMask( maskHandle );
			if ( mask != NULL ) {
				if ( mask->IsValid( idBounds( currentPosition ) ) == DR_CLEAR ) {
					positionValid = true;
				}
			}
		}
	} else {
		positionValid = true;
	}

	// if the position isn't on the mask, don't update the node

	if ( positionValid ) {
		sdRoutePoint* newPoint = FindPoint( currentPosition );

		if ( newPoint != NULL && ( ( newPoint->GetOrigin() - currentPosition ).Length() > 8192.f ) ) {
			positionValid = false;
		} else if ( tracker->GetCurrentPoint() == NULL ) {
			// initial
			tracker->SetBestPoint( newPoint );
		} else if ( tracker->GetCurrentPoint() != newPoint ) {
			bool warning = false;
			bool kickPlayer = false;
			int kickDistance = maxPointDeviance;

			const sdRoutePoint* bestPoint = tracker->GetBestPoint();
			if ( newPoint != bestPoint ) {
				int distance = newPoint->OnChain( bestPoint );
				if ( distance != -1 ) {
					if ( !g_noRouteConstraintKick.GetBool() ) {
						// we've moved backwards
						kickDistance = maxPointDeviance - distance;
						if ( distance >= maxPointDeviance ) {
							kickPlayer = true;
						} else if ( distance >= warningPointDeviance ) {
							warning = true;
						}
					}
				} else {
					distance = bestPoint->OnChain( newPoint );
					if ( distance != -1 ) {
						// we've moved forwards, yay
						tracker->SetBestPoint( newPoint );
					} else {
						// we're on another branch.. or something
						// at worst case, this could be the start, but should never be null
						const sdRoutePoint* common = bestPoint->FindCommonParent( newPoint );
						if ( common == NULL ) {
							gameLocal.Warning( "sdRouteConstraintController::Update Failed to find common parent" );
						} else {
							distance = common->OnChain( bestPoint );
							if ( distance >= warningPointDeviance ) {
								// we've gone way off course... or are approaching a new common point
								common = bestPoint->FindCommonChild( newPoint );
								if ( common == NULL ) {
									gameLocal.Warning( "sdRouteConstraintController::Update Failed to find common child" );
								} else {
									distance = bestPoint->OnChain( common );
									if ( !g_noRouteConstraintKick.GetBool() ) {
										kickDistance = maxPointDeviance - distance;
										if ( distance >= maxPointDeviance ) {
											kickPlayer = true;
										} else if ( distance >= warningPointDeviance ) {
											warning = true;
										}
										// TODO
									}
								}
							} else {
								// short distance, so just hop over
								tracker->SetBestPoint( newPoint );
							}
						}
					}
				}
			}

			if ( kickDistance < 0 ) {
				kickDistance = 0;
			}

			tracker->SetKickPlayer( kickPlayer );
			tracker->SetKickDistance( kickDistance );
			tracker->SetPositionWarning( warning );
		}

		if ( positionValid ) {
			tracker->SetCurrentPoint( newPoint );
		}
	}

	tracker->SetMaskWarning( !positionValid );
}

/*
================
sdRouteConstraintController::FindPoint
================
*/
sdRoutePoint* sdRouteConstraintController::FindPoint( const idVec3& origin ) {
	sdRoutePoint* bestPoint = points[ 0 ];
	float bestDist = ( points[ 0 ]->GetOrigin() - origin ).LengthSqr();

	for ( int i = 1; i < points.Num(); i++ ) {
		float dist = ( points[ i ]->GetOrigin() - origin ).LengthSqr();
		if ( dist >= bestDist ) {
			continue;
		}

		bestDist = dist;
		bestPoint = points[ i ];
	}

	return bestPoint;
}

/*
================
sdRouteConstraintController::FindPoint
================
*/
sdRoutePoint* sdRouteConstraintController::FindPoint( const char* name ) {
	if ( linked ) {
		gameLocal.Error( "sdRouteConstraintController::FindPoint Called after Linked" );
	}

	for ( int i = 0; i < points.Num(); i++ ) {
		if ( idStr::Icmp( points[ i ]->GetName(), name ) == 0 ) {
			return points[ i ];
		}
	}

	return NULL;
}

/*
================
sdRouteConstraintController::CheckForLoops
================
*/
void sdRouteConstraintController::CheckForLoops( idList< const sdRoutePoint* >& checkPoints, const sdRoutePoint* point ) {
	if ( checkPoints.FindIndex( point ) != -1 ) {
		gameLocal.Error( "sdRouteConstraintController::CheckForLoops Loop Found in Path" );
	}
	checkPoints.Alloc() = point;

	int currentSize = checkPoints.Num();
	for ( int i = 0; i < point->GetChildren().Num(); i++ ) {
		CheckForLoops( checkPoints, point->GetChildren()[ i ] );
		checkPoints.SetNum( currentSize, false );
	}
}

/*
================
sdRouteConstraintController::Event_Link
================
*/
void sdRouteConstraintController::Event_Link( void ) {
	for ( int i = 0; i < points.Num(); i++ ) {
		const idStrList& targetNames = points[ i ]->GetTargetNames();

		for ( int j = 0; j < targetNames.Num(); j++ ) {
			sdRoutePoint* otherPoint = FindPoint( targetNames[ j ].c_str() );
			if ( otherPoint == NULL ) {
				gameLocal.Error( "sdRouteConstraintController::Event_Link Failed to find Target '%s'", targetNames[ j ].c_str() );
			}

			sdRoutePoint::Link( points[ i ], otherPoint );
		}
	}

	for ( int i = 0; i < points.Num(); i++ ) {
		sdRoutePoint* point = points[ i ];

		if ( point->GetParents().Num() == 0 ) {
			if ( startPoint != NULL ) {
				gameLocal.Error( "sdRouteConstraintController::Event_Link Multiple Start Points Found" );
			}
			startPoint = point;
		}

		if ( point->GetChildren().Num() == 0 ) {
			if ( endPoint != NULL ) {
				gameLocal.Error( "sdRouteConstraintController::Event_Link Multiple End Points Found" );
			}
			endPoint = point;
		}
	}

	if ( startPoint == NULL ) {
		gameLocal.Error( "sdRouteConstraintController::Event_Link No Start Point Found" );
	}

	if ( endPoint == NULL ) {
		gameLocal.Error( "sdRouteConstraintController::Event_Link No End Point Found" );
	}

	for ( int i = 0; i < points.Num(); i++ ) {
		idVec3 direction = vec3_zero;

		for ( int j = 0; j < points[ i ]->GetChildren().Num(); j++ ) {
			const sdRoutePoint* child = points[ i ]->GetChildren()[ j ];

			idVec3 temp = child->GetOrigin() - points[ i ]->GetOrigin();
			temp.Normalize();
			direction += temp;
		}

		direction.Normalize();
		
		points[ i ]->SetAngles( direction.ToAngles() );
	}

	idList< const sdRoutePoint* > checkPoints;
	CheckForLoops( checkPoints, startPoint );

	linked = true;
}

/*
================
sdRouteConstraintController::AddPoint
================
*/
void sdRouteConstraintController::AddPoint( sdRouteConstraintMarker* marker ) {
	if ( linked ) {
		gameLocal.Error( "sdRouteConstraintController::AddPoint Controller is Already Linked" );
	}
		
	sdRoutePoint* point = sdRoutePoint::Allocate();
	point->Init( marker );

	points.Alloc() = point;
}

/*
===============================================================================

  sdRoutePoint

===============================================================================
*/

idBlockAlloc< sdRoutePoint, 32 > sdRoutePoint::s_allocator;

/*
================
sdRoutePoint::Clear
================
*/
void sdRoutePoint::Clear( void ) {
	origin.Zero();
	parents.Clear();
	children.Clear();
	name.Empty();
	targetNames.Clear();
	allowAirDrop = true;
}

/*
================
sdRoutePoint::Init
================
*/
void sdRoutePoint::Init( sdRouteConstraintMarker* _marker ) {
	origin = _marker->GetPhysics()->GetOrigin();
	allowAirDrop = !_marker->spawnArgs.GetBool( "no_air_drop" );

	name = _marker->GetName();

	const idKeyValue* kv = NULL;
	while ( ( kv = _marker->spawnArgs.MatchPrefix( "target", kv ) ) != NULL ) {
		targetNames.Append( kv->GetValue() );
	}
}

/*
================
sdRoutePoint::Link
================
*/
void sdRoutePoint::Link( sdRoutePoint* parent, sdRoutePoint* child ) {
	parent->AddChild( child );
	child->AddParent( parent );
}

/*
================
sdRoutePoint::AddParent
================
*/
void sdRoutePoint::AddParent( sdRoutePoint* point ) {
	const sdRoutePoint** parent = parents.Alloc();
	if ( parent == NULL ) {
		gameLocal.Error( "sdRoutePoint::AddParent '%s' Has too many parent nodes", name.c_str() );
	}

	*parent = point;
}

/*
================
sdRoutePoint::AddChild
================
*/
void sdRoutePoint::AddChild( sdRoutePoint* point ) {
	const sdRoutePoint** child = children.Alloc();
	if ( child == NULL ) {
		gameLocal.Error( "sdRoutePoint::AddChild '%s' Has too many child nodes", name.c_str() );
	}

	*child = point;
}

/*
================
sdRoutePoint::OnChain
================
*/
int sdRoutePoint::OnChain( const sdRoutePoint* chainEnd ) const {
	int distance = -1;
	for ( int i = 0; i < chainEnd->parents.Num(); i++ ) {
		const sdRoutePoint* parent = chainEnd->parents[ i ];
		if ( parent == this ) {
			return 1;
		}
		int temp = OnChain( parent );
		if ( temp != -1 ) {
			temp++;
			if ( distance == -1 || temp < distance ) {
				distance = temp;
			}
		}
	}

	return distance;
}

/*
================
sdRoutePoint::FindCommonParent
================
*/
const sdRoutePoint* sdRoutePoint::FindCommonParent( const sdRoutePoint* other ) const {
	for ( int i = 0; i < parents.Num(); i++ ) {
		const sdRoutePoint* out = FindCommonParent( other, parents[ i ] );
		if ( out != NULL ) {
			return out;
		}
	}

	return NULL;
}

/*
================
sdRoutePoint::FindCommonChild
================
*/
const sdRoutePoint* sdRoutePoint::FindCommonChild( const sdRoutePoint* other ) const {
	for ( int i = 0; i < children.Num(); i++ ) {
		const sdRoutePoint* out = FindCommonChild( other, children[ i ] );
		if ( out != NULL ) {
			return out;
		}
	}

	return NULL;
}

/*
================
sdRoutePoint::FindCommonParent
================
*/
const sdRoutePoint* sdRoutePoint::FindCommonParent( const sdRoutePoint* other, const sdRoutePoint* node ) const {
	if ( node->children.Num() > 1 ) {
		int distance = node->OnChain( other );
		if ( distance != -1 ) {
			return node;
		}
	}

	for ( int i = 0; i < node->parents.Num(); i++ ) {
		const sdRoutePoint* out = FindCommonParent( other, node->parents[ i ] );
		if ( out != NULL ) {
			return out;
		}
	}

	return NULL;
}

/*
================
sdRoutePoint::FindCommonChild
================
*/
const sdRoutePoint* sdRoutePoint::FindCommonChild( const sdRoutePoint* other, const sdRoutePoint* node ) const {
	if ( node->parents.Num() > 1 ) {
		int distance = other->OnChain( node );
		if ( distance != -1 ) {
			return node;
		}
	}

	for ( int i = 0; i < node->children.Num(); i++ ) {
		const sdRoutePoint* out = FindCommonChild( other, node->children[ i ] );
		if ( out != NULL ) {
			return out;
		}
	}

	return NULL;
}

/*
===============================================================================

  sdRouteConstraintMarker

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdRouteConstraintMarker )
END_CLASS

/*
================
sdRouteConstraintMarker::PostMapSpawn
================
*/
void sdRouteConstraintMarker::PostMapSpawn( void ) {
	const char* parentName;
	if ( !spawnArgs.GetString( "parent", "", &parentName ) ) {
		gameLocal.Error( "sdRouteConstraintMarker::PostMapSpawn No 'parent' key specified" );
	}
	idEntity* parent = gameLocal.FindEntity( parentName );
	if ( parent == NULL ) {
		gameLocal.Error( "sdRouteConstraintMarker::PostMapSpawn Parent '%s' not found", parentName );
	}

	sdRouteConstraintController* controller = parent->Cast< sdRouteConstraintController >();
	if ( controller == NULL ) {
		gameLocal.Error( "sdRouteConstraintMarker::PostMapSpawn Parent is not of type 'sdRouteConstraintController'" );
	}

	controller->AddPoint( this );

	PostEventMS( &EV_Remove, 0 );
}



/*
===============================================================================

  sdRouteConstraintTracker

===============================================================================
*/

/*
================
sdRouteConstraintTracker::sdRouteConstraintTracker
================
*/
sdRouteConstraintTracker::sdRouteConstraintTracker( void ) {
	currentPoint = NULL;
	bestPoint = NULL;
	bestPointReserve = NULL;
	renderPoint = NULL;
	positionWarning = false;
	maskWarning  = false;
	kickPlayer = false;
	kickDistance = -1;
}

/*
================
sdRouteConstraintTracker::Init
================
*/
void sdRouteConstraintTracker::Init( idEntity* _entity ) {
	entity = _entity;
}

/*
================
sdRouteConstraintTracker::Update
================
*/
void sdRouteConstraintTracker::Update( void ) {
	if ( !controller.IsValid() ) {
		return;
	}
	controller->Update( this );
}

/*
================
sdRouteConstraintTracker::Display
================
*/
void sdRouteConstraintTracker::Display( void ) {
	if ( !controller.IsValid() ) {
		return;
	}
	controller->Display( this );
}

/*
================
sdRouteConstraintTracker::SetReserve
================
*/
void sdRouteConstraintTracker::SetReserve( const sdRoutePoint* point ) {
	if ( point != NULL && point->GetAllowAirDrop() ) {
		bestPointReserve = point;
	}
}

/*
================
sdRouteConstraintTracker::GetDropLocation
================
*/
void sdRouteConstraintTracker::GetDropLocation( idVec3& position, idAngles& angles ) const {
	if ( bestPointReserve != NULL ) {
		position = bestPointReserve->GetOrigin();
		angles = bestPointReserve->GetAngles();
		return;
	}

	if ( controller.IsValid() ) {
		const sdRoutePoint* point = controller->GetStartPoint();
		if ( point != NULL ) {
			position = point->GetOrigin();
			angles = point->GetAngles();
			return;
		}
	}

	gameLocal.Error( "sdRouteConstraintTracker::GetDropLocation No valid location!!??!??" );
}

/*
================
sdRouteConstraintTracker::SetTrackerEntity
================
*/
void sdRouteConstraintTracker::SetTrackerEntity( idEntity* entity ) {
	currentPoint		= NULL;
	bestPoint			= NULL;
	bestPointReserve	= NULL;
	renderPoint			= NULL;

	for ( int i = 0; i < markers.Num(); i++ ) {
		markers[ i ].Hide();
	}
	markers.SetNum( 0 );

	positionWarning = false;
	maskWarning = false;
	kickPlayer = false;
	kickDistance = -1;

	if ( entity == NULL ) {
		controller = NULL;
		return;
	}

	controller = entity->Cast< sdRouteConstraintController >();
	if ( !controller.IsValid() ) {
		gameLocal.Error( "sdRouteConstraintTracker::SetTrackerEntity Invalid Entity Passed" );
	}
}

/*
================
sdRouteConstraintTracker::Hide
================
*/
void sdRouteConstraintTracker::Hide( void ) {
	if ( !controller.IsValid() ) {
		return;
	}
	controller->SetRenderPoint( this, NULL );
}
