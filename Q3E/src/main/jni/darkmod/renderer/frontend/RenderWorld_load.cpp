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

#include "renderer/tr_local.h"


/*
================
idRenderWorldLocal::FreeWorld
================
*/
void idRenderWorldLocal::FreeWorld() {
	int i;

	// this will free all the lightDefs and entityDefs
	FreeDefs();

	// free all the portals and check light/model references
	for ( auto &area: portalAreas ) {
		// there shouldn't be any remaining lightRefs or entityRefs
		if ( area.lightRefs.Num() > 0 ) {
			common->Error( "FreeWorld: unexpected remaining lightRefs" );
		}
		if ( area.entityRefs.Num() > 0 ) {
			common->Error( "FreeWorld: unexpected remaining entityRefs" );
		}
	}

	portalAreas.ClearFree();
	doublePortals.ClearFree();

	if ( areaNodes ) {
		R_StaticFree( areaNodes );
		areaNodes = NULL;
	}

	// free all the inline idRenderModels 
	for ( i = 0 ; i < localModels.Num() ; i++ ) {
		renderModelManager->RemoveModel( localModels[i] );
		delete localModels[i];
	}
	localModels.ClearFree();

	areaReferenceAllocator.Shutdown();
	interactionAllocator.Shutdown();

	mapName = "<FREED>";
}

/*
================
idRenderWorldLocal::TouchWorldModels
================
*/
void idRenderWorldLocal::TouchWorldModels( void ) {
	int i;

	for ( i = 0 ; i < localModels.Num() ; i++ ) {
		renderModelManager->CheckModel( localModels[i]->Name() );
	}
}

/*
================
idRenderWorldLocal::ParseModel
================
*/
idRenderModel *idRenderWorldLocal::ParseModel( idLexer *src ) {
	idRenderModel	*model;
	idToken			token;
	int				i, j;
	srfTriangles_t	*tri;
	modelSurface_t	surf;

	src->ExpectTokenString( "{" );

	// parse the name
	src->ExpectAnyToken( &token );

	model = renderModelManager->AllocModel();
	model->InitEmpty( token );
	TRACE_CPU_SCOPE_TEXT("Load:Model", model->Name())
	declManager->BeginModelLoad(model);

	int numSurfaces = src->ParseInt();
	if ( numSurfaces < 0 ) {
		src->Error( "R_ParseModel: bad numSurfaces" );
	}

	for ( i = 0 ; i < numSurfaces ; i++ ) {
		src->ExpectTokenString( "{" );

		src->ExpectAnyToken( &token );

		surf.material = declManager->FindMaterial( token );

		((idMaterial*)surf.material)->AddReference();

		tri = R_AllocStaticTriSurf();
		surf.geometry = tri;

		tri->numVerts = src->ParseInt();
		tri->numIndexes = src->ParseInt();

		R_AllocStaticTriSurfVerts( tri, tri->numVerts );
		for ( j = 0 ; j < tri->numVerts ; j++ ) {
			float	vec[8];

			src->Parse1DMatrix( 8, vec );

			tri->verts[j].xyz[0] = vec[0];
			tri->verts[j].xyz[1] = vec[1];
			tri->verts[j].xyz[2] = vec[2];
			tri->verts[j].st[0] = vec[3];
			tri->verts[j].st[1] = vec[4];
			tri->verts[j].normal[0] = vec[5];
			tri->verts[j].normal[1] = vec[6];
			tri->verts[j].normal[2] = vec[7];
		}

		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
		for ( j = 0 ; j < tri->numIndexes ; j++ ) {
			tri->indexes[j] = src->ParseInt();
		}
		src->ExpectTokenString( "}" );

		// add the completed surface to the model
		model->AddSurface( surf );
	}

	src->ExpectTokenString( "}" );

	model->FinishSurfaces();
	declManager->EndModelLoad(model);

	return model;
}

/*
================
idRenderWorldLocal::ParseShadowModel
================
*/
idRenderModel *idRenderWorldLocal::ParseShadowModel( idLexer *src ) {
	idRenderModel	*model;
	idToken			token;
	int				j;
	srfTriangles_t	*tri;
	modelSurface_t	surf;

	src->ExpectTokenString( "{" );

	// parse the name
	src->ExpectAnyToken( &token );

	model = renderModelManager->AllocModel();
	model->InitEmpty( token );
	TRACE_CPU_SCOPE_TEXT("Load:Model", model->Name())
	declManager->BeginModelLoad(model);

	surf.material = tr.defaultMaterial;

	tri = R_AllocStaticTriSurf();
	surf.geometry = tri;

	tri->numVerts = src->ParseInt();
	tri->numShadowIndexesNoCaps = src->ParseInt();
	tri->numShadowIndexesNoFrontCaps = src->ParseInt();
	tri->numIndexes = src->ParseInt();
	tri->shadowCapPlaneBits = src->ParseInt();

	R_AllocStaticTriSurfShadowVerts( tri, tri->numVerts );
	tri->bounds.Clear();
	for ( j = 0 ; j < tri->numVerts ; j++ ) {
		float	vec[8];

		src->Parse1DMatrix( 3, vec );
		tri->shadowVertexes[j].xyz[0] = vec[0];
		tri->shadowVertexes[j].xyz[1] = vec[1];
		tri->shadowVertexes[j].xyz[2] = vec[2];
		tri->shadowVertexes[j].xyz[3] = 1;		// no homogenous value

		tri->bounds.AddPoint( tri->shadowVertexes[j].xyz.ToVec3() );
	}

	R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
	for ( j = 0 ; j < tri->numIndexes ; j++ ) {
		tri->indexes[j] = src->ParseInt();
	}

	// add the completed surface to the model
	model->AddSurface( surf );

	src->ExpectTokenString( "}" );

	// we do NOT do a model->FinishSurfaceces, because we don't need sil edges, planes, tangents, etc.
//	model->FinishSurfaces();
	declManager->EndModelLoad(model);

	return model;
}

/*
================
idRenderWorldLocal::SetupAreaRefs
================
*/
void idRenderWorldLocal::SetupAreaRefs() {
	int		i;

	connectedAreaNum = 0;
	for ( i = 0; i < portalAreas.Num(); i++ ) {
		portalAreas[i].areaNum = i;
		assert(portalAreas[i].entityRefs.Num() == 0);
		assert(portalAreas[i].lightRefs.Num() == 0);
	}
}

/*
================
idRenderWorldLocal::ParseInterAreaPortals
================
*/
void idRenderWorldLocal::ParseInterAreaPortals( idLexer *src ) {
	int i, j;

	src->ExpectTokenString( "{" );

	int numPortalAreas = src->ParseInt();
	if ( numPortalAreas < 0 ) {
		src->Error( "R_ParseInterAreaPortals: bad numPortalAreas" );
		return;
	}
	portalAreas.SetNum( numPortalAreas );

	// set the doubly linked lists
	SetupAreaRefs();

	auto numInterAreaPortals = src->ParseInt();
	if ( numInterAreaPortals < 0 ) {
		src->Error(  "R_ParseInterAreaPortals: bad numInterAreaPortals" );
		return;
	}

	doublePortals.SetNum( numInterAreaPortals );

	for ( i = 0 ; i < numInterAreaPortals ; i++ ) {
		int		numPoints, a1, a2;
		portal_t	*p = &doublePortals[i].portals[0];
		idWinding	*w = &p->w;

		numPoints = src->ParseInt();
		a1 = src->ParseInt();
		a2 = src->ParseInt();

		//w = new idWinding( numPoints );
		w->SetNumPoints( numPoints );
		for ( j = 0 ; j < numPoints ; j++ ) {
			src->Parse1DMatrix( 3, (*w)[j].ToFloatPtr() );
			// no texture coordinates
			(*w)[j][3] = 0;
			(*w)[j][4] = 0;
		}

		// add the portal to a1
		p->intoArea = a2;
		p->doublePortal = &doublePortals[i];
		p->w.GetPlane( p->plane );

		portalAreas[a1].areaPortals.Append(p);

		// reverse it for a2
		p++;
		p->intoArea = a1;
		p->doublePortal = &doublePortals[i];
		p->w = *w;
		p->w.ReverseSelf();
		p->w.GetPlane( p->plane );

		portalAreas[a2].areaPortals.Append(p);
	}

	src->ExpectTokenString( "}" );
}

/*
================
idRenderWorldLocal::ParseNodes
================
*/
void idRenderWorldLocal::ParseNodes( idLexer *src ) {
	int			i;

	src->ExpectTokenString( "{" );

	numAreaNodes = src->ParseInt();
	if ( numAreaNodes < 0 ) {
		src->Error( "R_ParseNodes: bad numAreaNodes" );
	}
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc( numAreaNodes * sizeof( areaNodes[0] ) );

	for ( i = 0 ; i < numAreaNodes ; i++ ) {
		areaNode_t	*node;

		node = &areaNodes[i];

		src->Parse1DMatrix( 4, node->plane.ToFloatPtr() );
		node->children[0] = src->ParseInt();
		node->children[1] = src->ParseInt();
	}

	src->ExpectTokenString( "}" );
}

/*
================
idRenderWorldLocal::CommonChildrenArea_r
================
*/
int idRenderWorldLocal::CommonChildrenArea_r( areaNode_t *node ) {
	int	nums[2];

	for ( int i = 0 ; i < 2 ; i++ ) {
		if ( node->children[i] <= 0 ) {
			nums[i] = -1 - node->children[i];
		} else {
			nums[i] = CommonChildrenArea_r( &areaNodes[ node->children[i] ] );
		}
	}

	// solid nodes will match any area
	if ( nums[0] == AREANUM_SOLID ) {
		nums[0] = nums[1];
	}
	if ( nums[1] == AREANUM_SOLID ) {
		nums[1] = nums[0];
	}

	int	common;
	if ( nums[0] == nums[1] ) {
		common = nums[0];
	} else {
		common = CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	node->commonChildrenArea = common;

	return common;
}

/*
=================
idRenderWorldLocal::ClearWorld

Sets up for a single area world
=================
*/
void idRenderWorldLocal::ClearWorld() {
	portalAreas.SetNum( 1 );

	SetupAreaRefs();

	// even though we only have a single area, create a node
	// that has both children pointing at it so we don't need to
	//
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc( sizeof( areaNodes[0] ) );
	areaNodes[0].plane[3] = 1;
	areaNodes[0].children[0] = -1;
	areaNodes[0].children[1] = -1;
}

/*
=================
idRenderWorldLocal::FreeDefs

dump all the interactions
=================
*/
void idRenderWorldLocal::FreeDefs() {
	int		i;

	generateAllInteractionsCalled = false;

	// free all lightDefs
	for ( i = 0 ; i < lightDefs.Num() ; i++ ) {
		idRenderLightLocal	*light;

		light = lightDefs[i];
		if ( light && light->world == this ) {
			FreeLightDef( i );
			lightDefs[i] = NULL;
		}
	}

	// free all entityDefs
	for ( i = 0 ; i < entityDefs.Num() ; i++ ) {
		idRenderEntityLocal	*mod;

		mod = entityDefs[i];
		if ( mod && mod->world == this ) {
			FreeEntityDef( i );
			entityDefs[i] = NULL;
		}
	}

	if (interactionAllocator.GetAllocCount() > 0) {
		common->Error("idRenderWorldLocal::FreeDefs: not all interactions removed!");
	}
}

/*
=================
idRenderWorldLocal::InitFromMap

A NULL or empty name will make a world without a map model, which
is still useful for displaying a bare model
=================
*/
bool idRenderWorldLocal::InitFromMap( const char *name ) {
	idLexer *		src;
	idToken			token;
	idStr			filename;
	idRenderModel *	lastModel;

	// if this is an empty world, initialize manually
	if ( !name || !name[0] ) {
		FreeWorld();
		mapName.Clear();
		ClearWorld();
		return true;
	}


	// load it
	filename = name;
	filename.SetFileExtension( PROC_FILE_EXT );

	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ID_TIME_T currentTimeStamp;
	fileSystem->ReadFile( filename, NULL, &currentTimeStamp );

	if ( name == mapName ) {
		if ( currentTimeStamp != FILE_NOT_FOUND_TIMESTAMP && currentTimeStamp == mapTimeStamp ) {
			common->Printf( "idRenderWorldLocal::InitFromMap: retaining existing map\n" );
			FreeDefs();
			TouchWorldModels();
			AddWorldModelEntities();
			ClearPortalStates();
			return true;
		}
		common->Printf( "idRenderWorldLocal::InitFromMap: timestamp has changed, reloading.\n" );
	}

	FreeWorld();

	src = new idLexer( filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE );
	if ( !src->IsLoaded() ) {
		common->Printf( "idRenderWorldLocal::InitFromMap: %s not found\n", filename.c_str() );
		ClearWorld();
		return false;
	}


	mapName = name;
	mapTimeStamp = currentTimeStamp;

	// if we are writing a demo, archive the load command
	if ( session->writeDemo ) {
		WriteLoadMap();
	}

	if ( !src->ReadToken( &token ) || token.Icmp( PROC_FILE_ID ) ) {
		common->Printf( "idRenderWorldLocal::InitFromMap: bad id '%s' instead of '%s'\n", token.c_str(), PROC_FILE_ID );
		delete src;
		return false;
	}

	// parse the file
	while ( 1 ) {
		if ( !src->ReadToken( &token ) ) {
			break;
		}

		if ( token == "model" ) {
			lastModel = ParseModel( src );

			// add it to the model manager list
			renderModelManager->AddModel( lastModel );

			// save it in the list to free when clearing this map
			localModels.Append( lastModel );
			continue;
		}

		if ( token == "shadowModel" ) {
			lastModel = ParseShadowModel( src );

			// add it to the model manager list
			renderModelManager->AddModel( lastModel );

			// save it in the list to free when clearing this map
			localModels.Append( lastModel );
			continue;
		}

		if ( token == "interAreaPortals" ) {
			ParseInterAreaPortals( src );
			continue;
		}

		if ( token == "nodes" ) {
			ParseNodes( src );
			continue;
		}

		src->Error( "idRenderWorldLocal::InitFromMap: bad token \"%s\"", token.c_str() );
	}

	delete src;

	// if it was a trivial map without any areas, create a single area
	if ( !portalAreas.Num() ) {
		ClearWorld();
	}

	// find the points where we can early-our of reference pushing into the BSP tree
	CommonChildrenArea_r( &areaNodes[0] );

	AddWorldModelEntities();
	ClearPortalStates();

	// done!
	return true;
}

/*
=====================
idRenderWorldLocal::ClearPortalStates
=====================
*/
void idRenderWorldLocal::ClearPortalStates() {
	int		i, j;

	// all portals start off open
	for ( i = 0 ; i < doublePortals.Num() ; i++ ) {
		doublePortals[i].blockingBits = PS_BLOCK_NONE;
	}

	// flood fill all area connections
	for ( i = 0; i < portalAreas.Num(); i++ ) {
		for ( j = 0 ; j < NUM_PORTAL_ATTRIBUTES ; j++ ) {
			connectedAreaNum++;
			FloodConnectedAreas( &portalAreas[i], j );
		}
	}
}

/*
=====================
idRenderWorldLocal::AddWorldModelEntities
=====================
*/
void idRenderWorldLocal::AddWorldModelEntities() {
	int		i;

	// add the world model for each portal area
	// we can't just call AddEntityDef, because that would place the references
	// based on the bounding box, rather than explicitly into the correct area
	for ( i = 0; i < portalAreas.Num(); i++ ) {
		idRenderEntityLocal	*def;
		int			index;

		def = new idRenderEntityLocal;

		index = AllocateEntityDefHandle();
		entityDefs[index] = def;
		def->index = index;
		def->world = this;

		def->parms.hModel = renderModelManager->FindModel( va("_area%i", i ) );
		if ( def->parms.hModel->IsDefaultModel() || !def->parms.hModel->IsStaticWorldModel() ) {
			common->Error( "idRenderWorldLocal::InitFromMap: bad area model lookup" );
		}

		idRenderModel *hModel = def->parms.hModel;

		for ( int j = 0; j < hModel->NumSurfaces(); j++ ) {
			const modelSurface_t *surf = hModel->Surface( j );

			if ( surf->material->GetSort() == SS_PORTAL_SKY ) // grayman - use SS_PORTAL_SKY 'sort' value, not the material name
//			if ( surf->shader->GetName() == idStr( "textures/smf/portal_sky" ) )
			{
				def->needsPortalSky = true;
			}
		}

		// the local and global reference bounds are the same for area models
		def->referenceBounds = def->parms.hModel->Bounds();
		def->globalReferenceBounds = def->parms.hModel->Bounds();

		def->parms.axis[0][0] = 1;
		def->parms.axis[1][1] = 1;
		def->parms.axis[2][2] = 1;

		R_AxisToModelMatrix( def->parms.axis, def->parms.origin, def->modelMatrix );

		// in case an explicit shader is used on the world, we don't
		// want it to have a 0 alpha or color
		def->parms.shaderParms[0] =
		def->parms.shaderParms[1] =
		def->parms.shaderParms[2] =
		def->parms.shaderParms[3] = 1;

		R_DeriveEntityData(def); 
		AddEntityRefToArea(def, &portalAreas[i]);
	}
}

/*
=====================
CheckAreaForPortalSky
=====================
*/
bool idRenderWorldLocal::CheckAreaForPortalSky( int areaNum ) {
	assert( areaNum >= 0 && areaNum < portalAreas.Num() );

	const idList<int> &entityIds = portalAreas[areaNum].entityRefs;
	if ( entityIds.Num() > 0 ) {
		const idRenderEntityLocal *edef = entityDefs[entityIds[0]];
		if ( const idRenderModel *model = edef->parms.hModel )
			assert( model->IsStaticWorldModel() );	// world area model always goes first
		if ( edef->needsPortalSky )
			return true;
	}

	return false;
}
