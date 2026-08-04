/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "AASFile.h"
#include "AASFile_local.h"


//===============================================================
//
//	optimize file
//
//===============================================================

/*
================
idAASFileLocal::Optimize
================
*/
void idAASFileLocal::Optimize( void ) {
	TRACE_CPU_SCOPE("OptimizeAAS")
	int i, j, k, faceNum, edgeNum, areaFirstFace, faceFirstEdge;
	aasArea_t *area;
	aasFace_t *face;
	aasEdge_t *edge;
	idReachability *reach;
	idList<int> vertexRemap;
	idList<int> edgeRemap;
	idList<int> faceRemap;
	idList<aasVertex_t> newVertices;
	idList<aasEdge_t> newEdges;
	idList<aasIndex_t> newEdgeIndex;
	idList<aasFace_t> newFaces;
	idList<aasIndex_t> newFaceIndex;

	vertexRemap.AssureSize( vertices.Num(), -1 );
	edgeRemap.AssureSize( edges.Num(), 0 );
	faceRemap.AssureSize( faces.Num(), 0 );

	newVertices.Resize( vertices.Num() );
	newEdges.Resize( edges.Num() );
	newEdges.SetNum( 1, false );
	newEdgeIndex.Resize( edgeIndex.Num() );
	newFaces.Resize( faces.Num() );
	newFaces.SetNum( 1, false );
	newFaceIndex.Resize( faceIndex.Num() );

	for ( i = 0; i < areas.Num(); i++ ) {
		area = &areas[i];

		areaFirstFace = newFaceIndex.Num();
		for ( j = 0; j < area->numFaces; j++ ) {
			faceNum = faceIndex[area->firstFace + j];
			face = &faces[ abs(faceNum) ];

			// store face
			if ( !faceRemap[ abs(faceNum) ] ) {
				faceRemap[ abs(faceNum) ] = newFaces.Num();
				newFaces.Append( *face );

				// don't store edges for faces we don't care about
				if ( !( face->flags & ( FACE_FLOOR|FACE_LADDER ) ) ) {

					newFaces[ newFaces.Num()-1 ].firstEdge = 0;
					newFaces[ newFaces.Num()-1 ].numEdges = 0;

				} else {

					// store edges
					faceFirstEdge = newEdgeIndex.Num();
					for ( k = 0; k < face->numEdges; k++ ) {
						edgeNum = edgeIndex[ face->firstEdge + k ];
						edge = &edges[ abs(edgeNum) ];

						if ( !edgeRemap[ abs(edgeNum) ] ) {
							if ( edgeNum < 0 ) {
								edgeRemap[ abs(edgeNum) ] = -newEdges.Num();
							}
							else {
								edgeRemap[ abs(edgeNum) ] = newEdges.Num();
							}

							// remap vertices if not yet remapped
							if ( vertexRemap[ edge->vertexNum[0] ] == -1 ) {
								vertexRemap[ edge->vertexNum[0] ] = newVertices.Num();
								newVertices.Append( vertices[ edge->vertexNum[0] ] );
							}
							if ( vertexRemap[ edge->vertexNum[1] ] == -1 ) {
								vertexRemap[ edge->vertexNum[1] ] = newVertices.Num();
								newVertices.Append( vertices[ edge->vertexNum[1] ] );
							}

							newEdges.Append( *edge );
							newEdges[ newEdges.Num()-1 ].vertexNum[0] = vertexRemap[ edge->vertexNum[0] ];
							newEdges[ newEdges.Num()-1 ].vertexNum[1] = vertexRemap[ edge->vertexNum[1] ];
						}

						newEdgeIndex.Append( edgeRemap[ abs(edgeNum) ] );
					}

					newFaces[ newFaces.Num()-1 ].firstEdge = faceFirstEdge;
					newFaces[ newFaces.Num()-1 ].numEdges = newEdgeIndex.Num() - faceFirstEdge;
				}
			}

			if ( faceNum < 0 ) {
				newFaceIndex.Append( -faceRemap[ abs(faceNum) ] );
			} else {
				newFaceIndex.Append( faceRemap[ abs(faceNum) ] );
			}
		}

		area->firstFace = areaFirstFace;
		area->numFaces = newFaceIndex.Num() - areaFirstFace;

		// remap the reachability edges
		for ( reach = area->reach; reach; reach = reach->next ) {
			reach->edgeNum = abs( edgeRemap[reach->edgeNum] );
		}
	}

	// store new list
	vertices = newVertices;
	edges = newEdges;
	edgeIndex = newEdgeIndex;
	faces = newFaces;
	faceIndex = newFaceIndex;
}
