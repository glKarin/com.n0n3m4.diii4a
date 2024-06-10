// NavMeshBuild.cpp
//


#include "../../../external/recast/include/Recast.h"
#include "../../../navigation/Nav_local.h"

idCVar nav_dumpInputGeometry("nav_dumpInputGeometry", "0", CVAR_BOOL, "dumps input geometry to recast for debugging.");

//
// WorldGeometry_t
//
struct WorldGeometry_t {
	idList<idDrawVert> vertexes;
	idList<unsigned int> indexes;
};

/*
================
AddModelToMegaGeometry
================
*/
void AddModelToMegaGeometry(WorldGeometry_t &geometry, idVec3 origin, idMat3 axis, idRenderModel *renderModel)
{
	for (int i = 0; i < renderModel->NumSurfaces(); i++)
	{
		const idModelSurface *surface = renderModel->Surface(i);

		// Don't add shadow geometry.
		if (surface->geometry->numShadowIndexesNoCaps > 0 || surface->geometry->numShadowIndexesNoFrontCaps)
			continue;

		int startIndex = geometry.vertexes.Num();
		for (int d = 0; d < surface->geometry->numIndexes; d+=3)
		{
			geometry.indexes.Append(startIndex + surface->geometry->indexes[d + 2]);
			geometry.indexes.Append(startIndex + surface->geometry->indexes[d + 1]);
			geometry.indexes.Append(startIndex + surface->geometry->indexes[d + 0]);
		}

		for (int d = 0; d < surface->geometry->numVerts; d++)
		{
			const idDrawVert *vert = &surface->geometry->verts[d];
			idDrawVert navVert;

			navVert.xyz = vert->xyz * axis + origin;
			navVert.st = vert->st;
			navVert.normal = vert->normal;
			geometry.vertexes.Append(navVert);
		}
	}
}

bool LoadWorldFile(idStr mapName, WorldGeometry_t &geometry, int &crcMAP)
{
	mapName.StripFileExtension();
	idStr procName = "maps/";
	procName += mapName;

	idRenderWorld *world = renderSystem->AllocRenderWorld();
	if (!world->InitFromMap(procName))
	{
		renderSystem->FreeRenderWorld(world);
		common->Warning("Failed to load renderworld %s\n", procName.c_str());
		return false;
	}

	//if (world->IsLegacyWorld())
	//{
	//	renderSystem->FreeRenderWorld(world);
	//	common->Warning("This map is in the legacy format, please run dmap on this map to generate world files.\n", procName.c_str());
	//	return false;
	//}

	idMapFile *mapFile = new idMapFile();
	if (!mapFile->Parse(procName + ".map")) {
		renderSystem->FreeRenderWorld(world);
		delete mapFile;
		common->Warning("Failed to load rendermap %s\n", procName.c_str());
		return true;
	}

	crcMAP = mapFile->GetGeometryCRC();

	for (int i = 0; i < world->GetNumWorldModels(); i++)
	{
		idMat3	axis;
		idVec3 origin;
		idRenderModel *model;
	
		axis.Identity();
		origin.Zero();
	
		model = world->GetWorldModel(i);
	
		AddModelToMegaGeometry(geometry, origin, axis, model);
	}

	renderSystem->FreeRenderWorld(world);

	return true;
}

/*
===================
NavDumpWorldGeometry
===================
*/
void NavDumpWorldGeometry(const char *name, WorldGeometry_t mapGeometry) {
	idStr navFileName;
	navFileName = "maps/";
	navFileName += name;
	navFileHeader_t header;

	navFileName.SetFileExtension("_navinput.obj");

	// Open the nav file for writing.
	idFileScoped file(fileSystem->OpenFileWrite(navFileName));

	for(int i = 0; i < mapGeometry.vertexes.Num(); i++)
	{
		file->Printf("v %f %f %f\n", mapGeometry.vertexes[i].xyz.x, mapGeometry.vertexes[i].xyz.z, -mapGeometry.vertexes[i].xyz.y);
	}

	for(int i = 0; i < mapGeometry.indexes.Num() / 3; i++)
	{
		file->Printf("f %d %d %d\n", mapGeometry.indexes[(i * 3) + 0] + 1, mapGeometry.indexes[(i * 3) + 1] + 1, mapGeometry.indexes[(i * 3) + 2] + 1);
	}
}

void CreateNavMesh(idStr mapName)
{
	WorldGeometry_t mapGeometry;
	float bmin[3];
	float bmax[3];
	int mapCrc;

	common->Printf("---- CreateNavMesh ----\n");

	common->Printf("Loading Map...\n");
	if (!LoadWorldFile(mapName, mapGeometry, mapCrc))
		return;

	if(nav_dumpInputGeometry.GetBool())
	{
		common->Printf("nav_dumpInputGeometry is active, dumping input geometry\n");
		NavDumpWorldGeometry(mapName, mapGeometry);
		return;
	}

	rcConfig m_cfg;
	rcContext* m_ctx = new rcContext();

	float *verts = new float[mapGeometry.vertexes.Num() * 3];
	for (int n = 0; n < mapGeometry.vertexes.Num(); n++)
	{
		idVec3 xyz;
		xyz[0] = mapGeometry.vertexes[n].xyz[0];
		xyz[1] = mapGeometry.vertexes[n].xyz[1];
		xyz[2] = mapGeometry.vertexes[n].xyz[2];

		xyz = NavConvertCoordsToDoom(xyz);

		verts[(n * 3) + 0] = xyz.x;
		verts[(n * 3) + 1] = xyz.y;
		verts[(n * 3) + 2] = xyz.z;
	}

	int numTris = mapGeometry.indexes.Num() / 3;

	int *indexes = new int[mapGeometry.indexes.Num()];
	memcpy(indexes, mapGeometry.indexes.Ptr(), sizeof(int) * mapGeometry.indexes.Num());
	
	rcCalcBounds(verts, mapGeometry.vertexes.Num(), bmin, bmax);

	common->Printf("...RcCalcBounds (%f %f %f) (%f %f %f)\n", bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]);

	//
	// Step 1. Initialize build config.
	//

	common->Printf("1/7: Initialize build config.\n");

	// Grab the navmesh options decl and grab the options from it. 
	const idDeclEntityDef* botNavDecl = (const idDeclEntityDef * )declManager->FindType(DECL_ENTITYDEF, "navBot");

	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = BOT_GET_VALUE(m_cellSize);
	m_cfg.ch = BOT_GET_VALUE(m_cellHeight);
	m_cfg.walkableSlopeAngle = BOT_GET_VALUE(m_agentMaxSlope);
	m_cfg.walkableHeight = (int)ceilf(BOT_GET_VALUE(m_agentHeight) / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(BOT_GET_VALUE(m_agentMaxClimb) / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(BOT_GET_VALUE(m_agentRadius) / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(BOT_GET_VALUE(m_edgeMaxLen) / BOT_GET_VALUE(m_cellSize));
	m_cfg.maxSimplificationError = BOT_GET_VALUE(m_edgeMaxError);
	m_cfg.minRegionArea = (int)rcSqr(BOT_GET_VALUE(m_regionMinSize));		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(BOT_GET_VALUE(m_regionMergeSize));	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)BOT_GET_VALUE(m_vertsPerPoly);
	m_cfg.detailSampleDist = BOT_GET_VALUE(m_detailSampleDist) < 0.9f ? 0 : BOT_GET_VALUE(m_cellSize) * BOT_GET_VALUE(m_detailSampleDist);
	m_cfg.detailSampleMaxError = BOT_GET_VALUE(m_cellHeight) * BOT_GET_VALUE(m_detailSampleMaxError);

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(m_cfg.bmin, bmin);
	rcVcopy(m_cfg.bmax, bmax);
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	common->Printf("...Grid Size %dx%d\n", m_cfg.width, m_cfg.height);

	//
	// Step 2. Rasterize input polygon soup.
	//
	common->Printf("2/7: Rasterize input polygon soup..\n");
	rcHeightfield *m_solid = rcAllocHeightfield();
	if (!m_solid)
	{
		common->Warning("buildNavigation: Out of memory 'solid'.");
		return;
	}

	if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		common->Warning("buildNavigation: Could not create solid heightfield.");
		return;
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	unsigned char *m_triareas = new unsigned char[numTris];
	if (!m_triareas)
	{
		common->Warning("buildNavigation: Out of memory 'm_triareas' (%d).", numTris);
		return;
	}

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, numTris * sizeof(unsigned char));
	rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle, verts, mapGeometry.vertexes.Num(), indexes, numTris, m_triareas);
	rcRasterizeTriangles(m_ctx, verts, mapGeometry.vertexes.Num(), indexes, m_triareas, numTris, *m_solid, m_cfg.walkableClimb);

	delete[] m_triareas;
	m_triareas = 0;

	//
	// Step 3. Filter walkables surfaces.
	//
	common->Printf("3/7: Filter walkables surfaces...\n");
	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);

	// Step 4. Partition walkable surface to simple regions.
	//
	common->Printf("4/7: Partition walkable surface to simple regions...\n");
	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	rcCompactHeightfield *m_chf = rcAllocCompactHeightfield();
	if (!m_chf)
	{
		common->Warning("buildNavigation: Out of memory 'chf'.");
	}
	if (!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
	{
		common->Warning("buildNavigation: Could not build compact data.");
		return;
	}

	rcFreeHeightField(m_solid);
	m_solid = 0;

	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf))
	{
		common->Warning("buildNavigation: Could not erode.");
	}

	// Prepare for region partitioning, by calculating distance field along the walkable surface.
	if (!rcBuildDistanceField(m_ctx, *m_chf))
	{
		common->Warning("buildNavigation: Could not build distance field.");
	}

	// Partition the walkable surface into simple regions without holes.
	if (!rcBuildRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
	{
		common->Warning("buildNavigation: Could not build regions.");
	}

	rcContourSet*m_cset = rcAllocContourSet();
	if (!m_cset)
	{
		common->Warning("buildNavigation: Out of memory 'cset'.");
	}

	if (!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
	{
		common->Warning("buildNavigation: Could not create contours.");
	}

	common->Printf("m_cset->nconts=%i\n", m_cset->nconts);

	//
	// Step 6. Build polygons mesh from contours.
	//
	common->Printf("6/7: Build polygons mesh from contours..\n");
	// Build polygon navmesh from the contours.
	rcPolyMesh* m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		common->Warning("buildNavigation: Out of memory 'pmesh'.");
	}
	if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		common->Warning("buildNavigation: Could not triangulate contours.");
	}

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//
	common->Printf("7/7: Create detail mesh..\n");
	rcPolyMeshDetail* m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		common->Warning("buildNavigation: Out of memory 'pmdtl'.");
	}

	if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		common->Warning("buildNavigation: Could not build detail mesh.");
	}

	common->Printf("Writing Navigation File..\n");
	rvmNavFileLocal::WriteNavFile(mapName, m_pmesh, m_dmesh, mapCrc, botNavDecl);
}

void NavMesh_f(const idCmdArgs &args) {
	common->SetRefreshOnPrint(true);

	if (args.Argc() < 2) {
		common->Warning("Usage: navmesh <map_file>\n");
		return;
	}

	idStr mapName = args.Argv(1);
	
	CreateNavMesh(mapName);

	common->SetRefreshOnPrint(false);
}