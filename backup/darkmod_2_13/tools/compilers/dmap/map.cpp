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



#include "dmap.h"

/*

  After parsing, there will be a list of entities that each has
  a list of primitives.
  
  Primitives are either brushes, triangle soups, or model references.

  Curves are tesselated to triangle soups at load time, but model
  references are 
  Brushes will have 
  
	brushes, each of which has a side definition.

*/

//
// private declarations
//

#define MAX_BUILD_SIDES		300

static	int		entityPrimitive;		// to track editor brush numbers
static	int		c_numMapPatches;
static	int		c_areaportals;

static	uEntity_t	*uEntity;

// brushes are parsed into a temporary array of sides,
// which will have duplicates removed before the final brush is allocated
static	uBrush_t	*buildBrush;


#define	NORMAL_EPSILON			0.00001f
#define	DIST_EPSILON			0.01f


/*
===========
FindFloatPlane
===========
*/
int FindFloatPlane( const idPlane &plane, bool *fixedDegeneracies ) {
	idPlane p = plane;
	bool fixed = p.FixDegeneracies( DIST_EPSILON );
	if ( fixed && fixedDegeneracies ) {
		*fixedDegeneracies = true;
	}
	return dmapGlobals.mapPlanes.FindPlane( p, NORMAL_EPSILON, DIST_EPSILON );
}

idCVar dmap_fixBrushOpacityFirstSide(
	"dmap_fixBrushOpacityFirstSide", "1", CVAR_BOOL | CVAR_SYSTEM,
	"If set to 0, then dmap ignores first side of a brush when deciding whether it is solid or not. "
	"This is a bug fixed in TDM 2.08. "
);

/*
===========
SetBrushContents

The contents on all sides of a brush should be the same
Sets contentsShader, contents, opaque
===========
*/
static void SetBrushContents( uBrush_t *b ) {
	int			contents, c2;
	side_t		*s;
	int			i;

	s = &b->sides[0];
	contents = s->material->GetContentFlags();

	b->contentShader = s->material;

	// a brush is only opaque if all sides are opaque
	b->opaque = true;

	for ( i=0 ; i<b->numsides ; i++, s++ ) {
		s = &b->sides[i];
		if ( i == 0 && !dmap_fixBrushOpacityFirstSide.GetBool() )
			continue;	//stgatilov #5129

		if ( !s->material ) {
			continue;
		}

		c2 = s->material->GetContentFlags();
		if (c2 != contents) {
			contents |= c2;
		}

		if ( s->material->Coverage() != MC_OPAQUE ) {
			b->opaque = false;
		}
	}

	if ( contents & CONTENTS_AREAPORTAL ) {
		c_areaportals++;
	}

	b->contents = contents;
}

static void CheckBrushMirrorSides( uBrush_t *b ) {
	idList<const idMaterial *> mirrorMaterials;

	for (int i = 0 ; i < buildBrush->numsides ; i++) {
		const idMaterial *mat = buildBrush->sides[i].material;
		if (!mat->HasMirrorLikeStage())
			continue;

		if (mirrorMaterials.Find(mat)) {
			common->Warning("Brush %d (entity %d) has several sides with same mirror-like material %s", buildBrush->brushnum, buildBrush->entitynum, mat->GetName());
			return;
		}
		mirrorMaterials.Append(mat);
	}
}

//============================================================================

/*
===============
FreeBuildBrush
===============
*/
static void FreeBuildBrush( void ) {
	int		i;

	for ( i = 0 ; i < buildBrush->numsides ; i++ ) {
		if ( buildBrush->sides[i].winding ) {
			delete buildBrush->sides[i].winding;
		}
	}
	buildBrush->numsides = 0;
}

/*
===============
FinishBrush

Produces a final brush based on the buildBrush->sides array
and links it to the current entity
===============
*/
static uBrush_t *FinishBrush( void ) {
	uBrush_t	*b;
	primitive_t	*prim;

	// create windings for sides and bounds for brush
	if ( !CreateBrushWindings( buildBrush ) ) {
		// don't keep this brush
		FreeBuildBrush();
		return NULL;
	}

	if ( buildBrush->contents & CONTENTS_AREAPORTAL ) {
		if (dmapGlobals.num_entities != 1) {
			common->Printf("Entity %i, Brush %i: areaportals only allowed in world\n"
				,  dmapGlobals.num_entities - 1, entityPrimitive);
			FreeBuildBrush();
			return NULL;
		}
	}

	// keep it
	b = CopyBrush( buildBrush );

	FreeBuildBrush();

	b->entitynum = dmapGlobals.num_entities-1;
	b->brushnum = entityPrimitive;

	b->original = b;

	prim = (primitive_t *)Mem_Alloc( sizeof( *prim ) );
	memset( prim, 0, sizeof( *prim ) );
	prim->next = uEntity->primitives;
	uEntity->primitives = prim;

	prim->brush = b;

	return b;
}

/*
================
AdjustEntityForOrigin
================
*/
#if 0
static void AdjustEntityForOrigin( uEntity_t *ent ) {
	primitive_t	*prim;
	uBrush_t	*b;
	int			i;
	side_t		*s;

	for ( prim = ent->primitives ; prim ; prim = prim->next ) {
		b = prim->brush;
		if ( !b ) {
			continue;
		}
		for ( i = 0; i < b->numsides; i++ ) {
			idPlane plane;

			s = &b->sides[i];

			plane = dmapGlobals.mapPlanes[s->planenum];
			plane[3] += plane.Normal() * ent->origin;
				
			s->planenum = FindFloatPlane( plane );

			s->texVec.v[0][3] += DotProduct( ent->origin, s->texVec.v[0] );
			s->texVec.v[1][3] += DotProduct( ent->origin, s->texVec.v[1] );

			// remove any integral shift
			s->texVec.v[0][3] -= floor( s->texVec.v[0][3] );
			s->texVec.v[1][3] -= floor( s->texVec.v[1][3] );
		}
		CreateBrushWindings(b);
	}
}
#endif

/*
=================
RemoveDuplicateBrushPlanes

Returns false if the brush has a mirrored set of planes,
meaning it encloses no volume.
Also removes planes without any normal
=================
*/
static bool RemoveDuplicateBrushPlanes( uBrush_t * b ) {
	int			i, j, k;
	side_t		*sides;

	sides = b->sides;

	for ( i = 1 ; i < b->numsides ; i++ ) {

		// check for a degenerate plane
		if ( sides[i].planenum == -1) {
			common->Printf("Entity %i, Brush %i: degenerate plane\n"
				, b->entitynum, b->brushnum);
			// remove it
			for ( k = i + 1 ; k < b->numsides ; k++ ) {
				sides[k-1] = sides[k];
			}
			b->numsides--;
			i--;
			continue;
		}

		// check for duplication and mirroring
		for ( j = 0 ; j < i ; j++ ) {
			if ( sides[i].planenum == sides[j].planenum ) {
				common->Printf("Entity %i, Brush %i: duplicate plane\n"
					, b->entitynum, b->brushnum);
				// remove the second duplicate
				for ( k = i + 1 ; k < b->numsides ; k++ ) {
					sides[k-1] = sides[k];
				}
				b->numsides--;
				i--;
				break;
			}

			if ( sides[i].planenum == (sides[j].planenum ^ 1) ) {
				// mirror plane, brush is invalid
				common->Printf("Entity %i, Brush %i: mirrored plane\n"
					, b->entitynum, b->brushnum);
				return false;
			}
		}
	}
	return true;
}


/*
=================
ParseBrush
=================
*/
static void ParseBrush( const idMapBrush *mapBrush, int primitiveNum ) {
	uBrush_t	*b;
	side_t		*s;
	const idMapBrushSide	*ms;
	int			i;
	bool		fixedDegeneracies = false;

	buildBrush->entitynum = dmapGlobals.num_entities-1;
	buildBrush->brushnum = entityPrimitive;
	buildBrush->numsides = mapBrush->GetNumSides();
	for ( i = 0 ; i < mapBrush->GetNumSides() ; i++ ) {
		s = &buildBrush->sides[i];
		ms = mapBrush->GetSide(i);

		memset( s, 0, sizeof( *s ) );
		s->planenum = FindFloatPlane( ms->GetPlane(), &fixedDegeneracies );
		s->material = declManager->FindMaterial( ms->GetMaterial() );
		ms->GetTextureVectors( s->texVec.v );
		// remove any integral shift, which will help with grouping
		s->texVec.v[0][3] -= floor( s->texVec.v[0][3] );
		s->texVec.v[1][3] -= floor( s->texVec.v[1][3] );
	}

	// if there are mirrored planes, the entire brush is invalid
	if ( !RemoveDuplicateBrushPlanes( buildBrush ) ) {
		return;
	}

	//stgatilov #4707: warn when brush has several sides with same "mirror" texture
	//since the engine only chooses one of them to determine mirror plane
	CheckBrushMirrorSides(buildBrush);

	// get the content for the entire brush
	SetBrushContents( buildBrush );

	b = FinishBrush();
	if ( !b ) {
		return;
	}

	if ( fixedDegeneracies && dmapGlobals.verboseentities ) {
		common->Warning( "brush %d has degenerate plane equations", primitiveNum );
	}
}

/*
================
ParseSurface
================
*/
static void ParseSurface( const idMapPatch *patch, const idSurface *surface, const idMaterial *material ) {
	int				i;
	mapTri_t		*tri;
	primitive_t		*prim;

	prim = (primitive_t *)Mem_Alloc( sizeof( *prim ) );
	memset( prim, 0, sizeof( *prim ) );
	prim->next = uEntity->primitives;
	uEntity->primitives = prim;

	for ( i = 0; i < surface->GetNumIndexes(); i += 3 ) {
		tri = AllocTri();
		tri->v[2] = (*surface)[surface->GetIndexes()[i+0]];
		tri->v[1] = (*surface)[surface->GetIndexes()[i+2]];
		tri->v[0] = (*surface)[surface->GetIndexes()[i+1]];
		tri->material = material;
		tri->next = prim->tris;
		prim->tris = tri;
	}

	// set merge groups if needed, to prevent multiple sides from being
	// merged into a single surface in the case of gui shaders, mirrors, and autosprites
	if ( material->IsDiscrete() ) {
		for ( tri = prim->tris ; tri ; tri = tri->next ) {
			tri->mergeGroup = (void *)patch;
		}
	}
}

/*
================
ParsePatch
================
*/
static void ParsePatch( const idMapPatch *patch, int primitiveNum ) {
	const idMaterial *mat;

	if ( dmapGlobals.noCurves ) {
		return;
	}

	c_numMapPatches++;

	mat = declManager->FindMaterial( patch->GetMaterial() );

	idSurface_Patch *cp = new idSurface_Patch( *patch );

	if ( patch->GetExplicitlySubdivided() ) {
		cp->SubdivideExplicit( patch->GetHorzSubdivisions(), patch->GetVertSubdivisions(), true );
	} else {
		cp->Subdivide( DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true );
	}

	ParseSurface( patch, cp, mat );

	delete cp;
}

/*
================
ProcessMapEntity
================
*/
static bool	ProcessMapEntity( idMapEntity *mapEnt ) {
	idMapPrimitive	*prim;

	uEntity = &dmapGlobals.uEntities[dmapGlobals.num_entities];
	memset( uEntity, 0, sizeof(*uEntity) );
	uEntity->mapEntity = mapEnt;
	uEntity->nameEntity = mapEnt->epairs.GetString("name");
	if (idStr::Length(uEntity->nameEntity) == 0 && dmapGlobals.num_entities == 0)
		uEntity->nameEntity = "worldspawn";
	dmapGlobals.num_entities++;
	TRACE_CPU_SCOPE_TEXT("ProcessMapEntity", uEntity->nameEntity)

	for ( entityPrimitive = 0; entityPrimitive < mapEnt->GetNumPrimitives(); entityPrimitive++ ) {
		prim = mapEnt->GetPrimitive(entityPrimitive);

		if ( prim->GetType() == idMapPrimitive::TYPE_BRUSH ) {
			ParseBrush( static_cast<idMapBrush*>(prim), entityPrimitive );
		}
		else if ( prim->GetType() == idMapPrimitive::TYPE_PATCH ) {
			ParsePatch( static_cast<idMapPatch*>(prim), entityPrimitive );
		}
	}

	// never put an origin on the world, even if the editor left one there
	if ( dmapGlobals.num_entities != 1 ) {
		uEntity->mapEntity->epairs.GetVector( "origin", "", uEntity->origin );
	}

	return true;
}

//===================================================================

/*
==============
CreateMapLight

==============
*/
static void CreateMapLight( const idMapEntity *mapEnt ) {

	// get the name for naming the shadow surfaces
	const char *name = mapEnt->epairs.GetString( "name", "" );
	TRACE_CPU_SCOPE_TEXT( "CreateMapLight", name )

	// designers can add the "noPrelight" flag to signal that
	// the lights will move around, so we don't want
	// to bother chopping up the surfaces under it or creating
	// shadow volumes
	bool dynamic = mapEnt->epairs.GetBool( "noPrelight", "0" );
	if ( dynamic ) {
		return;
	}

	mapLight_t *light = new mapLight_t;
	light->name[0] = '\0';
	light->shadowTris = NULL;

	// parse parms exactly as the game do
	// use the game's epair parsing code so
	// we can use the same renderLight generation
	gameEdit->ParseSpawnArgsToRenderLight( &mapEnt->epairs, &light->def.parms );

	R_DeriveLightData( &light->def );

	idStr::Copynz( light->name, name, sizeof( light->name ) );
	if ( !light->name[0] ) {
		common->Error( "Light at (%f,%f,%f) didn't have a name",
			light->def.parms.origin[0], light->def.parms.origin[1], light->def.parms.origin[2] );
	}
#if 0
	// use the renderer code to get the bounding planes for the light
	// based on all the parameters
	R_RenderLightFrustum( light->parms, light->frustum );
	light->lightShader = light->parms.shader;
#endif

	dmapGlobals.mapLights.Append( light );

}

/*
==============
CreateMapLights

==============
*/
static void CreateMapLights( const idMapFile *dmapFile ) {
	TRACE_CPU_SCOPE("CreateMapLights")

	for ( int i = 0 ; i < dmapFile->GetNumEntities() ; i++ ) {
		const idMapEntity *mapEnt = dmapFile->GetEntity(i);
		const char *value = mapEnt->epairs.GetString( "classname", "" );
		if ( !idStr::Icmp( value, "light" ) ) {
			CreateMapLight( mapEnt );
		}
	}

	// stgatilov #6306: deprecate shadow-casting parallel lights
	idList<idStr> deprecatedParallelLights;
	for ( const mapLight_t *light : dmapGlobals.mapLights ) {
		if ( light->def.parms.parallel ) {
			if ( !light->def.parms.noShadows && light->def.lightShader->LightCastsShadows() ) {
				if ( !light->def.parms.parallelSky ) {
					deprecatedParallelLights.Append( light->name );
				}
			}
		}
	}
	if ( deprecatedParallelLights.Num() > 0 ) {
		idStr nameList = idStr::Join( deprecatedParallelLights, ", ");
		common->Warning( "Shadow-casting and parallel lights are deprecated: %s", nameList.c_str() );
		common->Printf( "If you need global parallel light (like sun or moon), use parallelSky light instead.\n" );
		common->Printf( "If you need local parallel light, disable shadows. (see #6306)\n" );
	}
}

/*
================
LoadDMapFile
================
*/
bool LoadDMapFile( const char *filename ) {		
	primitive_t	*prim;
	idBounds	mapBounds;
	int			brushes, triSurfs;
	int			i;
	int			size;

	TRACE_CPU_SCOPE_TEXT("LoadDMapFile", filename)
	PrintIfVerbosityAtLeast( VL_CONCISE, "--- LoadDMapFile ---\n" );
	PrintIfVerbosityAtLeast( VL_CONCISE, "loading %s\n", filename ); 

	// load and parse the map file into canonical form
	dmapGlobals.dmapFile = new idMapFile();
	if ( !dmapGlobals.dmapFile->Parse(filename) ) {
		delete dmapGlobals.dmapFile;
		dmapGlobals.dmapFile = NULL;
		common->Warning( "Couldn't load map file: '%s'", filename );
		return false;
	}

	int primitivesNum = dmapGlobals.dmapFile->GetTotalPrimitivesNum();
	int capacity = idMath::Imax(1024, idMath::CeilPowerOfTwo(primitivesNum));
	dmapGlobals.mapPlanes.Init( capacity, capacity );
	dmapGlobals.mapPlanes.SetGranularity( 1024 );

	// process the canonical form into utility form
	dmapGlobals.num_entities = 0;
	c_numMapPatches = 0;
	c_areaportals = 0;

	size = dmapGlobals.dmapFile->GetNumEntities() * sizeof( dmapGlobals.uEntities[0] );
	dmapGlobals.uEntities = (uEntity_t *)Mem_Alloc( size );
	memset( dmapGlobals.uEntities, 0, size );

	// allocate a very large temporary brush for building
	// the brushes as they are loaded
	buildBrush = AllocBrush( MAX_BUILD_SIDES );

	for ( i = 0 ; i < dmapGlobals.dmapFile->GetNumEntities() ; i++ ) {
		ProcessMapEntity( dmapGlobals.dmapFile->GetEntity(i) );
	}

	CreateMapLights( dmapGlobals.dmapFile );

	brushes = 0;
	triSurfs = 0;

	mapBounds.Clear();
	for ( prim = dmapGlobals.uEntities[0].primitives ; prim ; prim = prim->next ) {
		if ( prim->brush ) {
			brushes++;
			mapBounds.AddBounds( prim->brush->bounds );
		} else if ( prim->tris ) {
			triSurfs++;
		}
	}

	PrintIfVerbosityAtLeast( VL_CONCISE, "%5i total world brushes\n", brushes );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%5i total world triSurfs\n", triSurfs );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%5i patches\n", c_numMapPatches );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%5i entities\n", dmapGlobals.num_entities );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%5i planes\n", dmapGlobals.mapPlanes.Num() );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%5i areaportals\n", c_areaportals );
	PrintIfVerbosityAtLeast( VL_CONCISE, "size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", 
		mapBounds[0][0], mapBounds[0][1], mapBounds[0][2],
		mapBounds[1][0], mapBounds[1][1], mapBounds[1][2] );

	return true;
}

/*
================
FreeOptimizeGroupList
================
*/
void FreeOptimizeGroupList( optimizeGroup_t *groups ) {
	optimizeGroup_t	*next;

	for ( ; groups ; groups = next ) {
		next = groups->nextGroup;
		FreeTriList( groups->triList );
		Mem_Free( groups );
	}
}

/*
================
FreeDMapFile
================
*/
void FreeDMapFile( void ) {
	int		i, j;

	FreeBrush( buildBrush );
	buildBrush = NULL;

	// free the entities and brushes
	for ( i = 0 ; i < dmapGlobals.num_entities ; i++ ) {
		uEntity_t	*ent;
		primitive_t	*prim, *nextPrim;

		ent = &dmapGlobals.uEntities[i];

		FreeTree( ent->tree );

		// free primitives
		for ( prim = ent->primitives ; prim ; prim = nextPrim ) {
			nextPrim = prim->next;
			if ( prim->brush ) {
				FreeBrush( prim->brush );
			}
			if ( prim->tris ) {
				FreeTriList( prim->tris );
			}
			Mem_Free( prim );
		}

		// free area surfaces
		if ( ent->areas ) {
			for ( j = 0 ; j < ent->numAreas ; j++ ) {
				uArea_t	*area;

				area = &ent->areas[j];
				FreeOptimizeGroupList( area->groups );

			}
			Mem_Free( ent->areas );
		}
	}

	Mem_Free( dmapGlobals.uEntities );

	dmapGlobals.num_entities = 0;

	// free the map lights
	for ( i = 0; i < dmapGlobals.mapLights.Num(); i++ ) {
		R_FreeLightDefDerivedData( &dmapGlobals.mapLights[i]->def );
	}
	dmapGlobals.mapLights.DeleteContents( true );
}
