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
#include "../compiler_common.h"

dmapGlobals_t	dmapGlobals;

/*
============
PrintIfVerbosityAtLeast

Added #4123. Filter console output by verbosity level. Not used for errors and warnings.
============
*/
void PrintIfVerbosityAtLeast( verbosityLevel_t vl, const char* fmt, ... )
{
	if ( vl <= dmapGlobals.verbose )
	{
		va_list argptr;
		char text[MAX_STRING_CHARS];
		va_start( argptr, fmt );
		idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );
		common->Printf( "%s", text );
	}
}

/*
============
PrintEntityHeader

Added #4123. Print entity number, class, and name, for leaks and other messages.
============
*/
void PrintEntityHeader( verbosityLevel_t vl, const uEntity_t* e )
{
	PrintIfVerbosityAtLeast( vl, "############### entity %i ###############\n", dmapGlobals.entityNum );
	const idDict* entKeys = &e->mapEntity->epairs;
	PrintIfVerbosityAtLeast( vl, "-- ( %s: %s )\n", entKeys->GetString("classname"), entKeys->GetString("name") ); 
}

/*
============
ProcessModel
============
*/
bool ProcessModel( uEntity_t *e, bool floodFill ) {
	bspface_t	*faces;
	TRACE_CPU_SCOPE_TEXT("ProcessModel", e->nameEntity)

	// build a bsp tree using all of the sides
	// of all of the structural brushes
	faces = MakeStructuralBspFaceList ( e->primitives );
	e->tree = FaceBSP( faces );

	// create portals at every leaf intersection
	// to allow flood filling
	MakeTreePortals( e->tree );

	// classify the leafs as opaque or areaportal
	FilterBrushesIntoTree( e );

	// see if the bsp is completely enclosed
	if ( floodFill && !dmapGlobals.noFlood ) {
		if ( FloodEntities( e->tree ) ) {
			// set the outside leafs to opaque
			FillOutside( e );
		} else {
			// bail out here.  If someone really wants to
			// process a map that leaks, they should use
			// -noFlood
			return false;
		}
	}

	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ClipSidesByTree( e );

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	FloodAreas( e );

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas( e );

	// now build shadow volumes for the lights and split
	// the optimize lists by the light beam trees
	// so there won't be unneeded overdraw in the static
	// case
	Prelight( e );

	// optimizing is a superset of fixing tjunctions
	if ( !dmapGlobals.noOptimize ) {
		OptimizeEntity( e );
	} else  if ( !dmapGlobals.noTJunc ) {
		FixEntityTjunctions( e );
	}

	// now fix t junctions across areas
	FixGlobalTjunctions( e );

	return true;
}

/*
============
ProcessModels
============
*/
bool ProcessModels( void ) {
	verbosityLevel_t	oldVerbose;
	uEntity_t			*entity;
	uint				counter = 0;  // 4123
	TRACE_CPU_SCOPE("ProcessModels")

	{	//stgatilov #4970: check for rotation-hacked entities
		idList<const char *> badRotationFuncStatics;
		for (int i = 0; i < dmapGlobals.num_entities; i++) {
			entity = &dmapGlobals.uEntities[i];
			const idDict &spawnArgs = entity->mapEntity->epairs;
			const char *name = spawnArgs.GetString("name", "{unnamed}");
			idMat3 rotation = spawnArgs.GetMatrix("rotation");
			if (!rotation.IsOrthogonal(1e-3f)) {
				if (idStr::Cmp(spawnArgs.GetString("classname"), "func_static") == 0)
					badRotationFuncStatics.Append(name);
				else
					common->Warning("Entity \"%s\" has bad rotation matrix", name);
			}
		}

		/*
		 * stgatilov #5186: Starting with 2.09, this warning is no longer necessary.
		 * Because all hack-rotated entities are fixed during map load (with proxy models).
		 * Moreover, mocking mappers with this warning is undesirable, because stock prefabs have such entities too.
		 */
#if 0
		//complain about func_static-s with bad rotation only once
		if (int k = badRotationFuncStatics.Num()) {
			idStr list;
			idRandom rnd(std::time(0));
			for (int i = 0; i < badRotationFuncStatics.Num(); i++)
				idSwap(badRotationFuncStatics[i], badRotationFuncStatics[rnd.RandomInt(i+1)]);
			for (int i = 0; i < idMath::Imin(k, 10); i++) {
				if (i) list += ", ";
				list += idStr("\"") + badRotationFuncStatics[i] + "\"";
			}
			if (k > 10)
				list += ", ...";
			common->Warning("Detected %d func_static-s with bad rotation: [%s]", k, list.c_str());
		}
#endif
	}

	// stgatilov: dmap assumes that worldspawn comes first
	// make sure mapper knows if this is violated
	bool worldspawnCorrect = false;
	if ( dmapGlobals.num_entities > 0 ) {
		const idDict &spawnargs = dmapGlobals.uEntities[0].mapEntity->epairs;
		if ( idStr::Icmp( spawnargs.GetString( "classname" ), "worldspawn" ) == 0 )
			worldspawnCorrect = true;
	}
	if ( !worldspawnCorrect )
		common->Error( "Worldspawn entity must be the first one in .map file" );

	oldVerbose = dmapGlobals.verbose;

	for ( dmapGlobals.entityNum = 0 ; dmapGlobals.entityNum < dmapGlobals.num_entities ; dmapGlobals.entityNum++ ) {

		entity = &dmapGlobals.uEntities[dmapGlobals.entityNum];
		if ( !entity->primitives ) {
			continue;
		}

		if ( dmapGlobals.entityNum == 0 ) // worldspawn
		{
			PrintEntityHeader( VL_CONCISE, entity );
		} else {
			PrintEntityHeader( VL_ORIGDEFAULT, entity );
		}

		// if we leaked, stop without any more processing
		if ( !ProcessModel( entity, (bool)(dmapGlobals.entityNum == 0 ) ) ) {
			return false;
		}

		// we usually don't want to see output for submodels unless
		// something strange is going on
		// SteveL #4123: This (pre-existing) hack allows highly verbose output for 
		// worldspawn (entity 0) without getting it for func statics too.
		if ( !dmapGlobals.verboseentities ) {
			dmapGlobals.verbose = (verbosityLevel_t)idMath::Imin( dmapGlobals.verbose, VL_ORIGDEFAULT);
		}

		++counter;
	}

	dmapGlobals.verbose = oldVerbose;
	PrintIfVerbosityAtLeast( VL_CONCISE, "%d entities containing primitives processed.\n", counter);
	return true;
}

/*
============
DmapHelp
============
*/
void DmapHelp( void ) {
	common->Printf(
		
	"Usage: dmap [options] mapfile\n"
	"Options:\n"
	"noFlood           = ignore BSP leaks\n"
	"noCurves          = don't process curves\n"
	"noCM              = don't create collision map\n"
	"noAAS             = don't create AAS files\n"
	"v                 = verbose mode (default pre TDM 2.04)"
	"v2                = very verbose mode"
	"verboseentities   = very verbose + submodel detail for entities. Requires v2"
	);
}

/*
============
ResetDmapGlobals
============
*/
void ResetDmapGlobals( void ) {
	dmapGlobals.mapFileBase[0] = '\0';
	dmapGlobals.dmapFile = NULL;
	dmapGlobals.mapPlanes.ClearFree();
	dmapGlobals.num_entities = 0;
	dmapGlobals.uEntities = NULL;
	dmapGlobals.entityNum = 0;
	dmapGlobals.mapLights.ClearFree();
	dmapGlobals.verbose = VL_CONCISE;
	dmapGlobals.glview = false;
	dmapGlobals.noOptimize = false;
	dmapGlobals.verboseentities = false;
	dmapGlobals.noCurves = false;
	dmapGlobals.fullCarve = false;
	dmapGlobals.noModelBrushes = false;
	dmapGlobals.noTJunc = false;
	dmapGlobals.nomerge = false;
	dmapGlobals.noFlood = false;
	dmapGlobals.noClipSides = false;
	dmapGlobals.noLightCarve = false;
	dmapGlobals.noShadow = false;
	dmapGlobals.shadowOptLevel = SO_NONE;
	dmapGlobals.drawBounds.Clear();
	dmapGlobals.drawflag = false;
	dmapGlobals.totalShadowTriangles = 0;
	dmapGlobals.totalShadowVerts = 0;
}


idCVar dmap_compatibility(
	"dmap_compatibility", "0", CVAR_INTEGER | CVAR_SYSTEM,
	"This meta-cvar can be used to dmap old FMs.\n"
	"Without it, you'll probably have to heavily modify such a map.\n"
	"\n"
	"If you set this cvar to some TDM version, then\n"
	"  dmap will work approximately like it worked in that version of TDM.\n"
	"Version must be specified as integer without dot, e.g.: 207 or 209\n"
	"\n"
	"Implementation-wise, setting this cvar forces\n"
	"  all dmap_XXX cvars to appropriate values.\n"
);

/*
============
Dmap
============
*/
void Dmap( const idCmdArgs &args ) {
	int			i;
	int			start, end;
	char		path[1034];
	idStr		passedName;
	bool		leaked = false;
	bool		noCM = false;
	bool		noAAS = false;

	ResetDmapGlobals();

	if (int version = dmap_compatibility.GetInteger()) {
		//new in 2.08
		dmap_fixBrushOpacityFirstSide.SetBool(version >= 208);
		dmap_bspAllSidesOfVisportal.SetBool(version >= 208);
		dmap_fixVisportalOutOfBoundaryEffects.SetBool(version >= 208);
		extern idCVar cm_fixBrushContentsIgnoreLastSide;
		cm_fixBrushContentsIgnoreLastSide.SetBool(version >= 208);
		//new in 2.10
		dmap_planeHashing.SetBool(version >= 210);
		dmap_fasterPutPrimitives.SetBool(version >= 210);
		dmap_dontSplitWithFuncStaticVertices.SetBool(version >= 210);
		dmap_fixVertexSnappingTjunc.SetInteger(version >= 210 ? 2 : 0);
		dmap_fasterShareMapTriVerts.SetBool(version >= 210);
		dmap_optimizeTriangulation.SetBool(version >= 210);
		dmap_optimizeExactTjuncIntersection.SetBool(version >= 210);
		dmap_fasterAasMeltPortals.SetBool(version >= 210);
		dmap_fasterAasBrushListMerge.SetBool(version >= 210);
		dmap_pruneAasBrushesChopping.SetBool(version >= 210);
		dmap_fasterAasWaterJumpReachability.SetBool(version >= 210);
		dmap_disableCellSnappingTjunc.SetBool(version == 210);
		extern idCVar cm_buildBspForPatchEntities;
		cm_buildBspForPatchEntities.SetBool(version >= 210);
		//new in 2.11
		extern idCVar dmap_aasExpandBrushUseEdgesOnce;
		extern idCVar dmap_tjunctionsAlgorithm;
		dmap_aasExpandBrushUseEdgesOnce.SetBool(version >= 211);
		dmap_tjunctionsAlgorithm.SetBool(version >= 211);
		//new in 2.13
		dmap_outputNoSnap.SetBool(version >= 213);
	}

	if ( args.Argc() < 2 ) {
		DmapHelp();
		return;
	}

	TRACE_CPU_SCOPE_TEXT("Dmap", args.Args());
	common->Printf("---- dmap ----\n");

	dmapGlobals.fullCarve = true;
	dmapGlobals.shadowOptLevel = SO_MERGE_SURFACES;		// create shadows by merging all surfaces, but no super optimization
//	dmapGlobals.shadowOptLevel = SO_CLIP_OCCLUDERS;		// remove occluders that are completely covered
//	dmapGlobals.shadowOptLevel = SO_SIL_OPTIMIZE;
//	dmapGlobals.shadowOptLevel = SO_CULL_OCCLUDED;

	dmapGlobals.noLightCarve = true;

	for ( i = 1 ; i < args.Argc() ; i++ ) {
		const char *s;

		s = args.Argv(i);
		if ( s[0] == '-' ) {
			s++;
			if ( s[0] == '\0' ) {
				continue;
			}
		}

		if ( !idStr::Icmp( s,"glview" ) ) {
			dmapGlobals.glview = true;
		} else if ( !idStr::Icmp( s, "v" ) ) {
			common->Printf( "verbose = true (original default)\n" );
			dmapGlobals.verbose = VL_ORIGDEFAULT;
		} else if ( !idStr::Icmp( s, "v2" ) ) {
			common->Printf( "verbose = very\n" );
			dmapGlobals.verbose = VL_VERBOSE;
		} else if ( !idStr::Icmp( s, "draw" ) ) {
			if (r_glCoreProfile.GetInteger() > 0)
				common->Warning("set r_glCoreProfile 0 for draw flag!");
			else {
				common->Printf( "drawflag = true\n" );
				dmapGlobals.drawflag = true;
			}
		} else if ( !idStr::Icmp( s, "noFlood" ) ) {
			common->Printf( "noFlood = true\n" );
			dmapGlobals.noFlood = true;
		} else if ( !idStr::Icmp( s, "noLightCarve" ) ) {
			common->Printf( "noLightCarve = true\n" );
			dmapGlobals.noLightCarve = true;
		} else if ( !idStr::Icmp( s, "lightCarve" ) ) {
			common->Printf( "noLightCarve = false\n" );
			dmapGlobals.noLightCarve = false;
		} else if ( !idStr::Icmp( s, "noOpt" ) ) {
			common->Printf( "noOptimize = true\n" );
			dmapGlobals.noOptimize = true;
		} else if ( !idStr::Icmp( s, "verboseentities" ) ) {
			common->Printf( "verboseentities = true\n");
			dmapGlobals.verboseentities = true;
		} else if ( !idStr::Icmp( s, "noCurves" ) ) {
			common->Printf( "noCurves = true\n");
			dmapGlobals.noCurves = true;
		} else if ( !idStr::Icmp( s, "noModels" ) ) {
			common->Printf( "noModels = true\n" );
			dmapGlobals.noModelBrushes = true;
		} else if ( !idStr::Icmp( s, "noClipSides" ) ) {
			common->Printf( "noClipSides = true\n" );
			dmapGlobals.noClipSides = true;
		} else if ( !idStr::Icmp( s, "noCarve" ) ) {
			common->Printf( "noCarve = true\n" );
			dmapGlobals.fullCarve = false;
		} else if ( !idStr::Icmp( s, "shadowOpt" ) ) {
			dmapGlobals.shadowOptLevel = (shadowOptLevel_t)atoi( args.Argv( i+1 ) );
			common->Printf( "shadowOpt = %i\n",dmapGlobals.shadowOptLevel );
			i += 1;
		} else if ( !idStr::Icmp( s, "noTjunc" ) ) {
			// triangle optimization won't work properly without tjunction fixing
			common->Printf ("noTJunc = true\n" );
			dmapGlobals.noTJunc = true;
			dmapGlobals.noOptimize = true;
			common->Printf ("forcing noOptimize = true\n" );
		} else if ( !idStr::Icmp( s, "noCM" ) ) {
			noCM = true;
			common->Printf( "noCM = true\n" );
		} else if ( !idStr::Icmp( s, "noAAS" ) ) {
			noAAS = true;
			common->Printf( "noAAS = true\n" );
		} else if ( !idStr::Icmp( s, "editorOutput" ) ) {
#ifdef _WIN32
			com_outputMsg = true;
#endif
		} else {
			break;
		}
	}

	if ( i >= args.Argc() ) {
		common->Error( "usage: dmap [options] mapfile" );
	}

	passedName = args.Argv(i);
	FindMapFile(passedName);

    idStr stripped = passedName;
	stripped.StripFileExtension();
	idStr::Copynz( dmapGlobals.mapFileBase, stripped, sizeof(dmapGlobals.mapFileBase) );

	bool region = false;
	// if this isn't a regioned map, delete the last saved region map
	if ( passedName.Right( 4 ) != ".reg" ) {
		sprintf( path, "%s.reg", dmapGlobals.mapFileBase );
		fileSystem->RemoveFile( path, "" );
	} else {
		region = true;
	}


	passedName = stripped;

	// delete any old line leak files
	sprintf( path, "%s.lin", dmapGlobals.mapFileBase );
	fileSystem->RemoveFile( path, "" );
	// stgatilov #5129: remove all portal leak files too
	idStr mapFn = dmapGlobals.mapFileBase;
	mapFn.BackSlashesToSlashes();
	idStr dir = mapFn;
	dir.StripFilename();
	idFileList *allLinFiles = fileSystem->ListFiles(dir, "lin", false, true, "");
	for (int i = 0; i < allLinFiles->GetNumFiles(); i++) {
		idStr fn = allLinFiles->GetFile(i);
		if (fn.IcmpPrefix(mapFn) == 0 && fn.CheckExtension(".lin"))
			fileSystem->RemoveFile(fn, "");
	}
	fileSystem->FreeFileList(allLinFiles);

	//
	// start from scratch
	//
	start = Sys_Milliseconds();

	if ( !LoadDMapFile( passedName ) ) {
		return;
	}

	if ( ProcessModels() ) {
		WriteOutputFile();
		PrintIfVerbosityAtLeast( VL_CONCISE, "Dmap complete, moving on to collision world and AAS...\n");
	} else {
		leaked = true;
	}

	FreeDMapFile();

	PrintIfVerbosityAtLeast( VL_CONCISE, "%i total shadow triangles\n", dmapGlobals.totalShadowTriangles );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%i total shadow verts\n", dmapGlobals.totalShadowVerts );

	end = Sys_Milliseconds();
	PrintIfVerbosityAtLeast( VL_CONCISE, "-----------------------\n" );
	PrintIfVerbosityAtLeast( VL_CONCISE, "%5.0f seconds for dmap\n", ( end - start ) * 0.001f );

	if ( !leaked ) {

		if ( !noCM ) {
			TRACE_CPU_SCOPE("CreateCollisionMap")

			// make sure the collision model manager is not used by the game
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );

			// disconnect closes the console. reopen it. #4123
			console->Open(0.5);

			// create the collision map
			start = Sys_Milliseconds();

			collisionModelManager->LoadMap( dmapGlobals.dmapFile );
			collisionModelManager->FreeMap();

			end = Sys_Milliseconds();
			PrintIfVerbosityAtLeast( VL_CONCISE, "-------------------------------------\n" );
			PrintIfVerbosityAtLeast( VL_CONCISE, "%5.0f seconds to create collision map\n", ( end - start ) * 0.001f );
		}

		if ( !noAAS && !region ) {
			// create AAS files
			RunAAS_f( args );
		}
	}

	// free the common .map representation
	delete dmapGlobals.dmapFile;

	// clear the map plane list
	dmapGlobals.mapPlanes.ClearFree();

#ifdef _WIN32
	if ( com_outputMsg && com_hwndMsg != NULL ) {
		unsigned int msg = ::RegisterWindowMessage( DMAP_DONE );
		::PostMessage( com_hwndMsg, msg, 0, 0 );
	}
#endif
}

/*
============
Dmap_f
============
*/
void Dmap_f( const idCmdArgs &args ) {

	common->ClearWarnings( "running dmap" );

	// refresh the screen each time we print so it doesn't look
	// like it is hung
	common->SetRefreshOnPrint( true );
	Dmap( args );
	common->SetRefreshOnPrint( false );

	common->PrintWarnings();
}
