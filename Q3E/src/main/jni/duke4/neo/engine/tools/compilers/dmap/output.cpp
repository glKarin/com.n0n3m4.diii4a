/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#pragma hdrstop

#include "dmap.h"

//=================================================================================

idBounds Map_CalcOptimizeGroup(const optimizeGroup_t* group);

#if 0

should we try and snap values very close to 0.5, 0.25, 0.125, etc?

  do we write out normals, or just a "smooth shade" flag?
resolved: normals.  otherwise adjacent facet shaded surfaces get their
		  vertexes merged, and they would have to be split apart before drawing

  do we save out "wings" for shadow silhouette info?


#endif

static	idFile	*procFile;

#define	AREANUM_DIFFERENT	-2
/*
=============
PruneNodes_r

Any nodes that have all children with the same
area can be combined into a single leaf node

Returns the area number of all children, or
AREANUM_DIFFERENT if not the same.
=============
*/
int	PruneNodes_r( node_t *node ) {
	int		a1, a2;

	if ( node->planenum == PLANENUM_LEAF ) {
		return node->area;
	}

	a1 = PruneNodes_r( node->children[0] );
	a2 = PruneNodes_r( node->children[1] );

	if ( a1 != a2 || a1 == AREANUM_DIFFERENT ) {
		return AREANUM_DIFFERENT;
	}

	// free all the nodes below this point
	FreeTreePortals_r( node->children[0] );
	FreeTreePortals_r( node->children[1] );
	FreeTree_r( node->children[0] );
	FreeTree_r( node->children[1] );

	// change this node to a leaf
	node->planenum = PLANENUM_LEAF;
	node->area = a1;

	return a1;
}

static void WriteFloat( idFile *f, float v )
{
	if ( idMath::Fabs(v - idMath::Rint(v)) < 0.001 ) {
		f->WriteFloatString( "%i ", (int)idMath::Rint(v) );
	}
	else {
		f->WriteFloatString( "%f ", v );
	}
}

void Write1DMatrix( idFile *f, int x, float *m ) {
	int		i;

	f->WriteFloatString( "( " );

	for ( i = 0; i < x; i++ ) {
		WriteFloat( f, m[i] );
	}

	f->WriteFloatString( ") " );
}

static int CountUniqueShaders( optimizeGroup_t *groups ) {
	optimizeGroup_t		*a, *b;
	int					count;

	count = 0;

	for ( a = groups ; a ; a = a->nextGroup ) {
		if ( !a->triList ) {	// ignore groups with no tris
			continue;
		}
		for ( b = groups ; b != a ; b = b->nextGroup ) {
			if ( !b->triList ) {
				continue;
			}
			if ( a->material != b->material ) {
				continue;
			}
			if ( a->mergeGroup != b->mergeGroup ) {
				continue;
			}

// jmarshall - don't merge if this group is too far away, with forward+ it makes it so we have to deal with lights that are
// very far.
			idBounds a_bound = Map_CalcOptimizeGroup(a);
			idBounds b_bound = Map_CalcOptimizeGroup(b);

			float dist = (a_bound.GetCenter() - b_bound.GetCenter()).LengthFast();
			if (dist > 1000)
			{
				continue;
			}
// jmarshall end
			break;
		}
		if ( a == b ) {
			count++;
		}
	}

	return count;
}


/*
==============
MatchVert
==============
*/
#define	XYZ_EPSILON	0.01
#define	ST_EPSILON	0.001
#define	COSINE_EPSILON	0.999

static bool MatchVert( const idDrawVert *a, const idDrawVert *b ) {
	if ( idMath::Fabs( a->xyz[0] - b->xyz[0] ) > XYZ_EPSILON ) {
		return false;
	}
	if ( idMath::Fabs( a->xyz[1] - b->xyz[1] ) > XYZ_EPSILON ) {
		return false;
	}
	if ( idMath::Fabs( a->xyz[2] - b->xyz[2] ) > XYZ_EPSILON ) {
		return false;
	}
	if ( idMath::Fabs( a->st[0] - b->st[0] ) > ST_EPSILON ) {
		return false;
	}
	if ( idMath::Fabs( a->st[1] - b->st[1] ) > ST_EPSILON ) {
		return false;
	}

	// if the normal is 0 (smoothed normals), consider it a match
	if ( a->normal[0] == 0 && a->normal[1] == 0 && a->normal[2] == 0 
		&& b->normal[0] == 0 && b->normal[1] == 0 && b->normal[2] == 0 ) {
		return true;
	}

	// otherwise do a dot-product cosine check
	if ( DotProduct( a->normal, b->normal ) < COSINE_EPSILON ) {
		return false;
	}

	return true;
}

/*
====================
ShareMapTriVerts

Converts independent triangles to shared vertex triangles
====================
*/
srfTriangles_t	*ShareMapTriVerts( const mapTri_t *tris ) {
	const mapTri_t	*step;
	int			count;
	int			i, j;
	int			numVerts;
	int			numIndexes;
	srfTriangles_t	*uTri;

	// unique the vertexes
	count = CountTriList( tris );

	uTri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( uTri, count * 3 );
	R_AllocStaticTriSurfIndexes( uTri, count * 3 );

	numVerts = 0;
	numIndexes = 0;

	for ( step = tris ; step ; step = step->next ) {
		for ( i = 0 ; i < 3 ; i++ ) {
			const idDrawVert	*dv;

			dv = &step->v[i];

			// search for a match
			for ( j = 0 ; j < numVerts ; j++ ) {
				if ( MatchVert( &uTri->verts[j], dv ) ) {
					break;
				}
			}
			if ( j == numVerts ) {
				numVerts++;
				uTri->verts[j].xyz = dv->xyz;
				uTri->verts[j].normal = dv->normal;
				uTri->verts[j].st[0] = dv->st[0];
				uTri->verts[j].st[1] = dv->st[1];
			}

			uTri->indexes[numIndexes++] = j;
		}
	}

	uTri->numVerts = numVerts;
	uTri->numIndexes = numIndexes;

	return uTri;
}

/*
==================
CleanupUTriangles
==================
*/
static void CleanupUTriangles( srfTriangles_t *tri ) {
	// perform cleanup operations

	R_RangeCheckIndexes( tri );
	R_CreateSilIndexes( tri );
//	R_RemoveDuplicatedTriangles( tri );	// this may remove valid overlapped transparent triangles
//	R_RemoveDegenerateTriangles( tri );
//	R_RemoveUnusedVerts( tri );

	R_FreeStaticTriSurfSilIndexes( tri );
}

/*
====================
WriteUTriangles

Writes text verts and indexes to procfile
====================
*/
static void WriteUTriangles( const srfTriangles_t *uTris ) {
	int			col;
	int			i;

	// emit this chain
	procFile->WriteFloatString( "/* numVerts = */ %i /* numIndexes = */ %i\n", 
		uTris->numVerts, uTris->numIndexes );

	// verts
	col = 0;
	for ( i = 0 ; i < uTris->numVerts ; i++ ) {
		float	vec[8];
		const idDrawVert *dv;

		dv = &uTris->verts[i];

		vec[0] = dv->xyz[0];
		vec[1] = dv->xyz[1];
		vec[2] = dv->xyz[2];
		vec[3] = dv->st[0];
		vec[4] = dv->st[1];
		vec[5] = dv->normal[0];
		vec[6] = dv->normal[1];
		vec[7] = dv->normal[2];
		Write1DMatrix( procFile, 8, vec );

		if ( ++col == 3 ) {
			col = 0;
			procFile->WriteFloatString( "\n" );
		}
	}
	if ( col != 0 ) {
		procFile->WriteFloatString( "\n" );
	}

	// indexes
	col = 0;
	for ( i = 0 ; i < uTris->numIndexes ; i++ ) {
		procFile->WriteFloatString( "%i ", uTris->indexes[i] );

		if ( ++col == 18 ) {
			col = 0;
			procFile->WriteFloatString( "\n" );
		}
	}
	if ( col != 0 ) {
		procFile->WriteFloatString( "\n" );
	}
}

// jmarshall
/*
=================
BoundOptimizeGroup
=================
*/
idBounds Map_CalcOptimizeGroup(const optimizeGroup_t* group) {
	idBounds bounds;
	bounds.Clear();
	for (mapTri_t* tri = group->triList; tri; tri = tri->next) {
		bounds.AddPoint(tri->v[0].xyz);
		bounds.AddPoint(tri->v[1].xyz);
		bounds.AddPoint(tri->v[2].xyz);
	}

	return bounds;
}
// jmarshall

/*
=======================
GroupsAreSurfaceCompatible

Planes, texcoords, and groupLights can differ,
but the material and mergegroup must match
=======================
*/
static bool GroupsAreSurfaceCompatible( const optimizeGroup_t *a, const optimizeGroup_t *b ) {
	if ( a->material != b->material ) {
		return false;
	}
	if ( a->mergeGroup != b->mergeGroup ) {
		return false;
	}
// jmarshall - don't merge if this group is too far away, with forward+ it makes it so we have to deal with lights that are
// very far.
	if (a->noShadow != b->noShadow) {
		return false;
	}

	idBounds a_bound = Map_CalcOptimizeGroup(a);
	idBounds b_bound = Map_CalcOptimizeGroup(b);

	float dist = (a_bound.GetCenter() - b_bound.GetCenter()).LengthFast();
	if (dist > 1000)
	{
		return false;
	}
// jmarshall end

	return true;
}

// jmarshall - cleaned this up.
struct dmapSurface_t {
	srfTriangles_t* uTri;
	bool noShadow;
	idStr materialName;
};

static void CreateSrfTriangleList(int entityNum, int areaNum, idList<dmapSurface_t> &srfTriangles)
{
	mapTri_t* ambient, * copy;
	srfTriangles_t* uTri;
	uArea_t* area;
	optimizeGroup_t* group, * groupStep;
	typedef struct interactionTris_s {
		struct interactionTris_s* next;
		mapTri_t* triList;
		mapLight_t* light;
	} interactionTris_t;

	interactionTris_t* interactions, * checkInter; //, *nextInter;

	area = &dmapGlobals.uEntities[entityNum].areas[areaNum];
	

	for (group = area->groups; group; group = group->nextGroup) {
		if (group->surfaceEmited) {
			continue;
		}

		// combine all groups compatible with this one
		// usually several optimizeGroup_t can be combined into a single
		// surface, even though they couldn't be merged together to save
		// vertexes because they had different planes, texture coordinates, or lights.
		// Different mergeGroups will stay in separate surfaces.
		ambient = NULL;

		// each light that illuminates any of the groups in the surface will
		// get its own list of indexes out of the original surface
		interactions = NULL;

		for (groupStep = group; groupStep; groupStep = groupStep->nextGroup) {
			if (groupStep->surfaceEmited) {
				continue;
			}
			if (!GroupsAreSurfaceCompatible(group, groupStep)) {
				continue;
			}

			if (groupStep->triList == nullptr)
				continue;

			// copy it out to the ambient list
			copy = CopyTriList(groupStep->triList);
			ambient = MergeTriLists(ambient, copy);

			ambient->noShadow = groupStep->noShadow;
			groupStep->surfaceEmited = true;

			// duplicate it into an interaction for each groupLight
			for (int i = 0; i < groupStep->numGroupLights; i++) {
				for (checkInter = interactions; checkInter; checkInter = checkInter->next) {
					if (checkInter->light == groupStep->groupLights[i]) {
						break;
					}
				}
				if (!checkInter) {
					// create a new interaction
					checkInter = (interactionTris_t*)Mem_ClearedAlloc(sizeof(*checkInter));
					checkInter->light = groupStep->groupLights[i];
					checkInter->next = interactions;
					interactions = checkInter;
				}
				copy = CopyTriList(groupStep->triList);
				checkInter->triList = MergeTriLists(checkInter->triList, copy);
			}
		}

		if (!ambient) {
			continue;
		}

		uTri = ShareMapTriVerts(ambient);

		dmapSurface_t newSurf;
		newSurf.uTri = uTri;
		newSurf.noShadow = ambient->noShadow;
		newSurf.materialName = ambient->material->GetName();

		FreeTriList(ambient);

		srfTriangles.Append(newSurf);
	}
}

/*
====================
WriteOutputSurfaces
====================
*/
static void WriteOutputSurfaces( int entityNum, int areaNum ) {
	idList<dmapSurface_t> srfTriangles;
	idMapEntity* entity;

	entity = dmapGlobals.uEntities[entityNum].mapEntity;

	CreateSrfTriangleList(entityNum, areaNum, srfTriangles);

	if ( entityNum == 0 ) {
		procFile->WriteFloatString( "model { /* name = */ \"_area%i\" /* numSurfaces = */ %i\n\n", 
			areaNum, srfTriangles.Num() );
	} else {
		const char *name;

		entity->epairs.GetString( "name", "", &name );
		if ( !name[0] ) {
			common->Error( "Entity %i has surfaces, but no name key", entityNum );
		}
		procFile->WriteFloatString( "model { /* name = */ \"%s\" /* numSurfaces = */ %i\n\n", 
			name, srfTriangles.Num());
	}

	for (int i = 0; i < srfTriangles.Num(); i++)
	{
		procFile->WriteFloatString("/* surface %i */ { ", i);
		procFile->WriteFloatString("\"%s\" /* noShadow */ %d ", srfTriangles[i].materialName.c_str(), srfTriangles[i].noShadow);

		CleanupUTriangles(srfTriangles[i].uTri);
		WriteUTriangles(srfTriangles[i].uTri);
		R_FreeStaticTriSurf(srfTriangles[i].uTri);

		procFile->WriteFloatString("}\n\n");
	}

	procFile->WriteFloatString( "}\n\n" );
}
// jmarshall end

/*
===============
WriteNode_r

===============
*/
static void WriteNode_r( node_t *node ) {
	int		child[2];
	int		i;
	idPlane	*plane;

	if ( node->planenum == PLANENUM_LEAF ) {
		// we shouldn't get here unless the entire world
		// was a single leaf
		procFile->WriteFloatString( "/* node 0 */ ( 0 0 0 0 ) -1 -1\n" );
		return;
	}

	for ( i = 0 ; i < 2 ; i++ ) {
		if ( node->children[i]->planenum == PLANENUM_LEAF ) {
			child[i] = -1 - node->children[i]->area;
		} else {
			child[i] = node->children[i]->nodeNumber;
		}
	}

	plane = &dmapGlobals.mapPlanes[node->planenum];

	procFile->WriteFloatString( "/* node %i */ ", node->nodeNumber  );
	Write1DMatrix( procFile, 4, plane->ToFloatPtr() );
	procFile->WriteFloatString( "%i %i\n", child[0], child[1] );

	if ( child[0] > 0 ) {
		WriteNode_r( node->children[0] );
	}
	if ( child[1] > 0 ) {
		WriteNode_r( node->children[1] );
	}
}

static int NumberNodes_r( node_t *node, int nextNumber ) {
	if ( node->planenum == PLANENUM_LEAF ) {
		return nextNumber;
	}
	node->nodeNumber = nextNumber;
	nextNumber++;
	nextNumber = NumberNodes_r( node->children[0], nextNumber );
	nextNumber = NumberNodes_r( node->children[1], nextNumber );

	return nextNumber;
}

/*
====================
WriteOutputNodes
====================
*/
static void WriteOutputNodes( node_t *node ) {
	int		numNodes;

	// prune unneeded nodes and count
	PruneNodes_r( node );
	numNodes = NumberNodes_r( node, 0 );

	// output
	procFile->WriteFloatString( "nodes { /* numNodes = */ %i\n\n", numNodes );
	procFile->WriteFloatString( "/* node format is: ( planeVector ) positiveChild negativeChild */\n" );
	procFile->WriteFloatString( "/* a child number of 0 is an opaque, solid area */\n" );
	procFile->WriteFloatString( "/* negative child numbers are areas: (-1-child) */\n" );

	WriteNode_r( node );

	procFile->WriteFloatString( "}\n\n" );
}

/*
====================
WriteOutputPortals
====================
*/
static void WriteOutputPortals( uEntity_t *e ) {
	int			i, j;
	interAreaPortal_t	*iap;
	idWinding			*w;

	procFile->WriteFloatString( "interAreaPortals { /* numAreas = */ %i /* numIAP = */ %i\n\n", 
		e->numAreas, numInterAreaPortals );
	procFile->WriteFloatString( "/* interAreaPortal format is: numPoints positiveSideArea negativeSideArea ( point) ... */\n" );
	for ( i = 0 ; i < numInterAreaPortals ; i++ ) {
		iap = &interAreaPortals[i];
		w = iap->side->winding;
		procFile->WriteFloatString("/* iap %i */ %i %i %i ", i, w->GetNumPoints(), iap->area0, iap->area1 );
		for ( j = 0 ; j < w->GetNumPoints() ; j++ ) {
			Write1DMatrix( procFile, 3, (*w)[j].ToFloatPtr() );
		}
		procFile->WriteFloatString("\n" );
	}

	procFile->WriteFloatString( "}\n\n" );
}


/*
====================
WriteOutputEntity
====================
*/
static void WriteOutputEntity( int entityNum ) {
	int		i;
	uEntity_t *e;

	e = &dmapGlobals.uEntities[entityNum];

	if ( entityNum != 0 ) {
		// entities may have enclosed, empty areas that we don't need to write out
		if ( e->numAreas > 1 ) {
			e->numAreas = 1;
		}
	}

	for ( i = 0 ; i < e->numAreas ; i++ ) {
		WriteOutputSurfaces( entityNum, i );
	}

	// we will completely skip the portals and nodes if it is a single area
	if ( entityNum == 0 && e->numAreas > 1 ) {
		// output the area portals
		WriteOutputPortals( e );

		// output the nodes
		WriteOutputNodes( e->tree->headnode );
	}
}


/*
====================
WriteOutputFile
====================
*/
void WriteOutputFile( void ) {
	int				i;
	uEntity_t		*entity;
	idStr			qpath;

	// write the file
	common->Printf( "----- WriteOutputFile -----\n" );

	sprintf( qpath, "%s." PROC_FILE_EXT, dmapGlobals.mapFileBase );

	common->Printf( "writing %s\n", qpath.c_str() );
	// _D3XP used fs_cdpath
	procFile = fileSystem->OpenFileWrite( qpath, "fs_devpath" );
	if ( !procFile ) {
		common->Error( "Error opening %s", qpath.c_str() );
	}

	procFile->WriteFloatString( "%s\n\n", PROC_FILE_ID );
// jmarshall
	procFile->WriteFloatString("worldInfo {\n");
	if (dmapGlobals.portal_sky_location != vec3_zero)
	{
		procFile->WriteFloatString("\tportalSkyCamera %f %f %f\n", dmapGlobals.portal_sky_location.x, dmapGlobals.portal_sky_location.y, dmapGlobals.portal_sky_location.z);
	}
	procFile->WriteFloatString("}\n\n");
// jmarshall end

	// write the entity models and information, writing entities first
	//for ( i=dmapGlobals.num_entities - 1 ; i >= 0 ; i-- ) {
	//	entity = &dmapGlobals.uEntities[i];
	//
	//	if ( !entity->primitives ) {
	//		continue;
	//	}
	//
	//	
	//}

	// Write out the world only
	WriteOutputEntity(0);

	fileSystem->CloseFile( procFile );
}
