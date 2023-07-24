// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Wakes.h"

// We need to move more than this before a new node gets added to the wake
#define NODE_DELTA 10
#define NODE_TIME 25

static idCVar g_debugWakes( "g_debugWakes", "0", CVAR_BOOL, "Debug the vehicle wakes" );

void sdWakeParms::ParseFromDict( const idDict &spawnArgs ) {

	noWake = false;
	centerMat = declHolder.declMaterialType.LocalFind( spawnArgs.GetString( "mtr_wake_center", "_default" ) );
	edgeMat = declHolder.declMaterialType.LocalFind( spawnArgs.GetString( "mtr_wake_edge", "_default" ) );
	centerWidths = spawnArgs.GetVec2( "wake_center_width", "0 0" );
	edgeWidths = spawnArgs.GetVec2( "wake_edge_width", "0 0" );
	centerScales = spawnArgs.GetVec2( "wake_center_scale", "0.0001 0.0001" );
	edgeScales = spawnArgs.GetVec2( "wake_edge_scale", "0.0001 0.0001" );
	centerTexCoord = spawnArgs.GetVec2( "wake_center_texcoord", "0 1" );
	edgeTexCoord = spawnArgs.GetVec2( "wake_edge_texcoord", "0 1" );
	maxVisDist = spawnArgs.GetInt( "maxVisDist", "2048" );

	numPoints = 0;
	idVec3 temp;
	numPoints=0;
	while ( spawnArgs.GetVector( va("wake_point_%i", numPoints ), "0 0 0", temp ) ) {
		if ( numPoints >= MAX_POINTS ) {
			common->Error( "Too many wake points" );
		}
		points[numPoints] = temp;
		numPoints++;
	}

	idVec3 mid;
	mid.Zero();
	for (int i=0; i<numPoints; i++) {
		mid += points[i];
	}
	if ( numPoints ) {
		mid /= (float)numPoints;
	}
	for (int i=0; i<numPoints; i++) {
		normals[i] = points[i] - mid;
		normals[i].Normalize();
	}

	if ( centerWidths.IsZero() && edgeWidths.IsZero() ) {
		noWake = true;
	}
}


/*****************

	Wake Layer

*****************/

int sdWakeLayer::AddToBack( sdWakeNode &node ) {
	int index = (firstNode+numNodes)%MAX_NODES;
	nodes[ index ] = node;
	numNodes++;
	return index;
}

void sdWakeLayer::PopFront() {
	firstNode = (firstNode+1)%MAX_NODES;
	numNodes--;
}

int sdWakeLayer::RemapIndex( int index ) {
	return (firstNode+index)%MAX_NODES;	
}

sdWakeNode &sdWakeLayer::GetNode( int index ) {
	return nodes[ RemapIndex( index ) ];	
}

sdWakeLayer::sdWakeLayer( void ) {
	numNodes = 0;
	firstNode = 0;

	lifeTime = 3000.0f;
	posWidth = 60.0f;
	negWidth = 0.0f;
	posCurScale = 0.0001f;
	negCurScale = 0.0001f;
	texNeg = 0.0f;
	texPos = 1.0f;

	texOfs = 0.0f;
	negScale = 1.0f;
	posScale = 1.0f;
	lastOrigin = vec3_origin;
	lastDeriv = vec3_origin;
}

void sdWakeLayer::Init( idDrawVert *triangleVerts, int firstVertex ) {
	this->triangleVerts = triangleVerts;
	this->firstVert = firstVertex;
	numNodes = 0;
	firstNode = 0;
	texOfs = 0.0f;
	negScale = 1.0f;
	posScale = 1.0f;
	lastOrigin = vec3_origin;
	lastDeriv = vec3_origin;
}

void sdWakeLayer::AddNode( const idVec3 &origin, const idVec3 &emitLeft, float alpha ) {

	idVec3 up( 0.0f, 0.0f, 1.0f );
	float sgn = 1.f;
	if ( !numNodes ) {
		lastOrigin = origin;
	}

	// Update derivatives ( oh these such simple fd's but they seem to work fine :D )
	idVec3 dp = origin - lastOrigin;
	if ( !numNodes ) {
		lastDeriv = dp;
	}
	idVec3 ddp = dp - lastDeriv;
	float  delta = dp.Length();

	if ( numNodes && (delta < NODE_DELTA) ) {
		return;
	}

	idVec3 leftL;
	leftL.Cross( dp, up );
	leftL.Normalize();

	// Calculate curvature (in 1 / units_of_length )
	float curv;
	float temp = dp.x*dp.x + dp.y*dp.y;
	if ( temp ) {
		float dv = idMath::Pow( temp, -(3.0f/2.0f) );
		curv = (dp.x * ddp.y - dp.y * ddp.x) * dv;
	} else {
		curv = 0.00001f;// Almost straight
	} 

	// Convert this curvature to some scale value
	// as to make the wake thinner in tight coners
	float posNewScale = (1.0f / curv) * posCurScale;
	float negNewScale = (1.0f / curv) * negCurScale;
	if ( posNewScale < 0 ) {
		posNewScale = 1.0f;
	}
	if ( negNewScale < 0 ) {
		negNewScale = 1.0f;
	}

	posNewScale = abs( posNewScale );
	negNewScale = abs( negNewScale );

	posNewScale = Min( posNewScale, 1.0f );
	posNewScale = Max( posNewScale, 0.0f );

	negNewScale = Min( negNewScale, 1.0f );
	negNewScale = Max( negNewScale, 0.0f );

	// Don't make the scale too jittery
	negScale = negScale * 0.95 + 0.05 * negNewScale;
	posScale = posScale * 0.95 + 0.05 * posNewScale;

	// Just remove the oldest don't care about fading out properly
	if ( numNodes >= MAX_NODES ) {
		PopFront();
	}

	// Update the texture coordinate "walk distance"
	texOfs += delta * 0.005f;

	// Add a new node
	sdWakeNode node;
	node.spawnTime = gameLocal.time;
	node.breakWake = false;
	node.alpha = alpha;
	int index = AddToBack( node );
	
	// Update the vertices corresponding to this node
	index = index*2 + firstVert;
	triangleVerts[index+0].xyz = origin + leftL * negWidth * posScale;
	triangleVerts[index+1].xyz = origin - leftL * negWidth * negScale;
	triangleVerts[index+0].SetST( idVec2( texOfs, texNeg ) );
	triangleVerts[index+1].SetST( idVec2( texOfs, texPos ) );
	triangleVerts[index+0].color[0] = triangleVerts[index+0].color[1] = triangleVerts[index+0].color[2] = triangleVerts[index+0].color[3] = 255;
	triangleVerts[index+1].color[0] = triangleVerts[index+1].color[1] = triangleVerts[index+1].color[2] = triangleVerts[index+1].color[3] = 255;
	triangleVerts[index+0].SetTangent( leftL );
	triangleVerts[index+1].SetTangent( leftL );
	triangleVerts[index+0].SetNormal( up );
	triangleVerts[index+1].SetNormal( up );
	triangleVerts[index+0].SetBiTangentSign( sgn );
	triangleVerts[index+1].SetBiTangentSign( sgn );

	lastOrigin = origin;
	lastDeriv = dp;
}

void sdWakeLayer::Update( srfTriangles_t *triangles ) {

	// Remove nodes that timed out (they will be at the front)
	while ( (( gameLocal.time - GetNode(0).spawnTime ) > lifeTime) && numNodes ) {
		PopFront();
	}

	// Update the index lists for the others
	triangles->bounds.Clear();
	for ( int i=1; i<numNodes; i++ ) {
		int curIndex = RemapIndex(i);

		if( nodes[curIndex].breakWake ) continue; // Don't add connecting triangles with previous...
		int lastIndex = RemapIndex(i-1);

		int curVertIndex = curIndex*2+firstVert;
		int lastVertIndex = lastIndex*2+firstVert;
		float basealpha = nodes[curIndex].alpha;

		if ( nodes[curIndex].breakWake || nodes[lastIndex].breakWake ) {
			triangles->verts[curVertIndex+0].color[3] = 0;
			triangles->verts[curVertIndex+1].color[3] = 0;
		} else { 
			byte alpha = (1.0f - ((gameLocal.time - nodes[curIndex].spawnTime) / (float)lifeTime)) * 255 * basealpha;
			triangles->verts[curVertIndex+0].color[3] = alpha;
			triangles->verts[curVertIndex+1].color[3]  = alpha;
		}
		triangles->bounds.AddPoint( triangles->verts[curVertIndex+0].xyz );
		triangles->bounds.AddPoint( triangles->verts[curVertIndex+1].xyz );

		triangles->indexes[ triangles->numIndexes+0  ] = curVertIndex+0;
		triangles->indexes[ triangles->numIndexes+1  ] = curVertIndex+1;
		triangles->indexes[ triangles->numIndexes+2  ] = lastVertIndex+1;
		triangles->indexes[ triangles->numIndexes+3  ] = curVertIndex+0;
		triangles->indexes[ triangles->numIndexes+4  ] = lastVertIndex+1;
		triangles->indexes[ triangles->numIndexes+5  ] = lastVertIndex+0;
		triangles->numIndexes += 6;
	}

	// First and last are always transparent
	if ( numNodes ) {
		triangles->verts[firstVert+RemapIndex(0)*2+0].color[3] = 0;
		triangles->verts[firstVert+RemapIndex(0)*2+1].color[3] = 0;
		triangles->verts[firstVert+RemapIndex(numNodes-1)*2+0].color[3] = 0;
		triangles->verts[firstVert+RemapIndex(numNodes-1)*2+1].color[3] = 0;
	}
}

void sdWakeLayer::Break( void ) {
	if ( !numNodes ) return; // inherently broken
	nodes[ RemapIndex(numNodes-1) ].breakWake = true;
}

int sdWakeLayer::NumNodes( void ) {
	return numNodes;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
sdWake::sdWake( void ) {
	minPoint = vec3_origin;
	maxPoint = vec3_origin;
	nextNodeTime = 0.0f;
	numPoints = 0;

	renderEntity.origin.Zero();
	renderEntity.axis.Identity();
	renderEntity.flags.noShadow = true;
	renderEntity.flags.noSelfShadow = true;
	renderEntity.flags.noDynamicInteractions = true;
	stopped = true;
}

// The hardcoded particle system will do proper cleanup...

void sdWake::Init(  const sdWakeParms &params, int ticket ) {

	minPoint = vec3_origin;
	maxPoint = vec3_origin;
	nextNodeTime = 0.0f;
	numPoints = 0;
	this->ticket = ticket;

	renderEntity.maxVisDist = params.maxVisDist;

	// Allocate surfaces
	if ( !NumSurfaces() ) {
		AddSurfaceDB( params.centerMat, ( sdWakeLayer::MAX_NODES * 2 + 2), ( sdWakeLayer::MAX_NODES * 6) );
		GetTriSurf( 0, 0 )->numVerts = GetTriSurf( 0, 0 )->numAllocedVerts;	
		GetTriSurf( 1, 0 )->numVerts = GetTriSurf( 1, 0 )->numAllocedVerts;	

		AddSurfaceDB( params.edgeMat, ( sdWakeLayer::MAX_NODES * 2 + 2)*MAX_POINTS, ( sdWakeLayer::MAX_NODES * 6)*MAX_POINTS );
		GetTriSurf( 0, 1 )->numVerts = GetTriSurf( 0, 1 )->numAllocedVerts;
		GetTriSurf( 1, 1 )->numVerts = GetTriSurf( 1, 1 )->numAllocedVerts;
	}

	triangleVerts[0].SetNum( ( sdWakeLayer::MAX_NODES * 2 + 2) );
	triangleVerts[1].SetNum( ( sdWakeLayer::MAX_NODES * 2 + 2)*MAX_POINTS );

	const_cast<modelSurface_t *>(renderEntity.hModel->Surface(0))->material = params.centerMat;
	const_cast<modelSurface_t *>(renderEntity.hModel->Surface(1))->material = params.edgeMat;
	numPoints = params.numPoints;
	for ( int i=0; i<numPoints; i++ ) {
		points[i] = params.points[i];
		normals[i] = params.normals[i];
	}

	GetTriSurf( 0, 0 )->bounds.Clear();
	GetTriSurf( 1, 0 )->bounds.Clear();
	GetTriSurf( 0, 1 )->bounds.Clear();
	GetTriSurf( 1, 1 )->bounds.Clear();

	// Edges (these are just mirrors of each other)
	for ( int i=0; i<MAX_POINTS; i++) {
		layer[i].Init( triangleVerts[1].Begin(), sdWakeLayer::MAX_NODES * 2 * i );
		layer[i].SetWidths( params.edgeWidths[0], params.edgeWidths[1] );
		layer[i].SetCurvatureScales( params.edgeScales[0], params.edgeScales[1] );
		if ( (i & 1) == 0 || 1 ) {
			layer[i].SetTexCoords( params.edgeTexCoord[0], params.edgeTexCoord[1] );
		} else {
			layer[i].SetTexCoords( params.edgeTexCoord[1], params.edgeTexCoord[0] );
		}
	}

	// Center
	layer3.Init( triangleVerts[0].Begin(), 0 );
	layer3.SetWidths( params.centerWidths[0], params.centerWidths[1] );
	layer3.SetCurvatureScales( params.centerScales[0], params.centerScales[1] );
	layer3.SetTexCoords( params.centerTexCoord[0], params.centerTexCoord[1] );	

	stopped = false;
}

static int side( float v ) {
	if ( v < 0.f ) {
		return 1;
	} else {
		return 2;
	}
}

void sdWake::Update( const idVec3 &forward, const idVec3 &origin, const idMat3 &axis ) {

	// In worldspace
	idVec3 left;
	idVec3 up( 0.0f, 0.0f, 1.0f );
	idVec3 flat = forward;
	flat.z = 0.0f;

	if ( flat.LengthSqr() < 100.f ) {
		//return;
	}


	flat.Normalize();	
	left.Cross( flat, up  );
	left.Normalize();

	// Find the extreme points of the bounding box normal to the forward direction
	float min = idMath::INFINITY;
	float max = -idMath::INFINITY;
	idVec3 newMinPoint;
	idVec3 newMaxPoint;

	int minidx = -1;
	int maxidx = -1;
	float tpvalue[ MAX_POINTS ];
	for ( int i=0; i<numPoints; i++ ) {
		float tp = ( points[i] * axis ) * left;
		tpvalue[ i ] = tp;
		if ( tp < min ) {
			min = tp;
			minidx = i;
			newMinPoint = points[i];
		}
		if ( tp > max ) {
			max = tp;
			maxidx = i;
			newMaxPoint = points[i];
		}
	}

	float avg = (min+max) * 0.5f;
	float alpha[ MAX_POINTS ];
	for (int i=0; i<numPoints; i++ ) {
		if ( tpvalue[i] < avg ) {
			alpha[i] = idMath::ClampFloat( 0.f, 1.f, 1.f-( ( tpvalue[i] - min ) / 25.f) );
		} else {
			alpha[i] = idMath::ClampFloat( 0.f, 1.f, 1.f-( ( max - tpvalue[i] ) / 25.f) );
		}
	}

	maxPoint = maxPoint * 0.95f + newMaxPoint * 0.05f;
	minPoint = minPoint * 0.95f + newMinPoint * 0.05f;

	idVec3 worldMinPoint = minPoint * axis + origin;
	idVec3 worldMaxPoint = maxPoint * axis + origin;

	worldMinPoint.z = origin.z;
	worldMaxPoint.z = origin.z;

	if ( g_debugWakes.GetBool() ) {
		gameRenderWorld->DebugSphere( idVec4( 1.0, 0.0, 0.0, 0.0 ), idSphere( worldMinPoint, 10 ) );
		gameRenderWorld->DebugSphere( idVec4( 0.0, 1.0, 0.0, 0.0 ), idSphere( worldMaxPoint, 10 ) );

		for ( int i=0; i<numPoints; i++ ) {
			idVec3 pnt = points[i];
			//pnt.z = origin.z;

			idVec3 pointWorld = pnt * axis + origin;
			pointWorld.z = origin.z;
			idVec3 normalWorld = normals[i] * axis;
			gameRenderWorld->DebugSphere( idVec4( 0.0, 0.0, 1.0, 0.0 ), idSphere( pointWorld, 10 ) );
		}
	}

	static const idVec3 offset( 0.0f, 0.0f, -1.0f );

	if ( gameLocal.time > nextNodeTime ) {
		idVec3 mid = ( worldMinPoint + worldMaxPoint ) * 0.5f;
		for (int i=0; i<numPoints; i++) {
			idVec3 pnt = points[i];
			idVec3 wp = pnt * axis + origin;
			idVec3 emit = normals[i] * axis;
			wp.z = origin.z;
			layer[i].AddNode( wp, emit, alpha[i] );
		}

		layer3.AddNode( mid + offset, axis[1], 1.f );
	}

	//Update();
}

void sdWake::Update( void ) {

	idRenderModel *prevModel = renderEntity.hModel;
	SetDoubleBufferedModel();
	memcpy( renderEntity.hModel->Surface(0)->geometry->verts, triangleVerts[0].Begin(), triangleVerts[0].Num() * sizeof(idDrawVert) );
	memcpy( renderEntity.hModel->Surface(1)->geometry->verts, triangleVerts[1].Begin(), triangleVerts[1].Num() * sizeof(idDrawVert) );

	renderEntity.hModel->FreeVertexCache();
	GetTriSurf(0)->numIndexes = 0;
	GetTriSurf(1)->numIndexes = 0;
	for (int i=0; i<numPoints; i++) {
		layer[i].Update( GetTriSurf(1) );
	}
//	GetTriSurf(1)->numIndexes = 0;
	layer3.Update( GetTriSurf(0) );
	renderEntity.bounds = GetTriSurf(0)->bounds;

	//if ( GetTriSurf(0)->numIndexes ) {
	PresentRenderEntity();
	//}

	bool allstopped = true;
	for (int i=0; i<numPoints; i++) {
		allstopped &= layer[i].NumNodes() == 0;
	}

	// IF all layers are empty mark it as stopped
	if( allstopped && layer3.NumNodes() == 0 ) {
		if ( g_debugWakes.GetBool() ) {
			common->Printf( "Wake stopped (%i)\n", gameLocal.time );
		}
		stopped = true;
		ticket = -1;
	}
}

void sdWake::Break( void ) {
	for (int i=0; i<numPoints; i++) {
		layer[i].Break();
	}
	layer3.Break();
}

void sdWake::AddPoint( const idVec3 &point ) {
	if ( numPoints >= MAX_POINTS ) return;
	points[numPoints] = point;
	numPoints++;
}

void sdWake::ClearPoints( void ) {
	numPoints = 0;
}

bool sdWake::HasStopped( void ) {
	return stopped;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int sdWakeManagerLocal::AllocateWake( const sdWakeParms &params ) {

	// If zero width, means no wake...
	if ( params.noWake ) {
		return 0;
	}

	for ( int i=0; i<MAX_WAKES; i++ ) {
		if ( wakes[i].HasStopped() ) {
			if ( g_debugWakes.GetBool() ) {
				common->Printf( "Initializing wake %i-%i (%s) (%i)\n", i, ticket, params.centerMat->GetName(), gameLocal.time );
			}		
			if ( !idStr::Cmp( "_default", params.centerMat->GetName() ) ) {
				common->Printf("defaulted\n");
			}
			wakes[i].Init( params, ticket );
			ticket++;
			return ((i+1) | ( (ticket-1) << 16));
		}
	}

	if ( g_debugWakes.GetBool() ) {
		common->Printf( "Could not allocate wake, ran out of free wakes\n" );
	}

	/*if ( numWakes >= MAX_WAKES ) {
		return -1;
	}

	wakes[numWakes].Init( params );
	numWakes++;
	return numWakes-1;*/
	
	return 0;
}

bool sdWakeManagerLocal::UpdateWake( unsigned int handle, const idVec3 &forward, const idVec3 &origin, const idMat3 &axis ) {
	if ( handle == 0 ) return true;
	int ticket  = handle >> 16;
	int index = (handle & 0xFFFF)-1;
	if ( wakes[index].GetTicket() == ticket ) {
		wakes[index].Update( forward, origin, axis );
		return true;
	} else {
		return false;
	}
}

void sdWakeManagerLocal::BreakWake( unsigned int handle ) {
	if ( handle == -1 ) return;
	int index = (handle & 0xFFFF)-1;
	wakes[index].Break();
}

void sdWakeManagerLocal::Init( void ) {
	ticket = 1;
	wakes = new sdWake[ MAX_WAKES ];
}

void sdWakeManagerLocal::Think( void ) {
	for ( int i=0; i<MAX_WAKES; i++ ) {
		if ( !wakes[i].HasStopped() ) {
			wakes[i].Update();
		}
	}
}

void sdWakeManagerLocal::Deinit( void ) {
	delete []wakes;
	wakes = NULL;
}
