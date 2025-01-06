// NavFile.cpp
//


#include "Nav_local.h"

idCVar nav_dumpobj("nav_dumpobj", "0", CVAR_BOOL, "writes a obj represenation of the navmesh");

idVec3 NavConvertCoordsToDoom(idVec3 pt) {
	idVec3 v = idVec3(pt.x, pt.y, pt.z) * idAngles(0, 0, -90).ToMat3();
	return v;
}

idVec3 ConvertNavToDoomCoords(idVec3 pt) {
	idVec3 v = idVec3(pt.x, pt.y, pt.z) * idAngles(0, 0, 90).ToMat3();
	return v;
}

/*
===================
rvmNavFileLocal::rvmNavFileLocal
===================
*/
rvmNavFileLocal::rvmNavFileLocal(const char *name) {
	this->name = name;
	this->name.SetFileExtension(NAV_FILE_EXTENSION);
	m_navMesh = nullptr;
}

/*
===================
rvmNavFileLocal::~rvmNavFileLocal
===================
*/
rvmNavFileLocal::~rvmNavFileLocal() {
	if(m_navMesh != nullptr)
	{
		dtFree(m_navMesh);
		m_navMesh = nullptr;
	}
}

/*
===================
rvmNavFileLocal::GetRandomPointNearPosition
===================
*/
void rvmNavFileLocal::GetRandomPointNearPosition(idVec3 point, idVec3 &randomPoint, float radius) {
	idVec3 extents = idVec3(280, 480, 280);
	dtQueryFilter filter;
	dtPolyRef polyRef;
	idVec3 nearestPoint;

	m_navquery->findNearestPoly(NavConvertCoordsToDoom(point).ToFloatPtr(), extents.ToFloatPtr(), &filter, &polyRef, nearestPoint.ToFloatPtr());

	dtPolyRef randomPolyRef;
	m_navquery->findRandomPointAroundCircle(polyRef, point.ToFloatPtr(), radius, &filter, idMath::FRand, &randomPolyRef, randomPoint.ToFloatPtr());
	randomPoint = ConvertNavToDoomCoords(randomPoint);
}

/*
===================
rvmNavFileLocal::GetPathBetweenPoints
===================
*/
bool rvmNavFileLocal::GetPathBetweenPoints(const idVec3 p1, const idVec3 p2, idList<idVec3>& waypoints) {
	idVec3 extents = idVec3(280, 480, 280);
	
	dtQueryFilter filter;
	dtPolyRef startPolyRef, endPolyRef;
	idVec3 startNearestPt, endNearestPt;

	m_navquery->findNearestPoly(NavConvertCoordsToDoom(p1).ToFloatPtr(), extents.ToFloatPtr(), &filter, &startPolyRef, startNearestPt.ToFloatPtr());
	m_navquery->findNearestPoly(NavConvertCoordsToDoom(p2).ToFloatPtr(), extents.ToFloatPtr(), &filter, &endPolyRef, endNearestPt.ToFloatPtr());

	
	dtPolyRef path[NAV_MAX_PATHSTEPS];
	int numPaths = -1;
	m_navquery->findPath(startPolyRef, endPolyRef, NavConvertCoordsToDoom(p1).ToFloatPtr(), NavConvertCoordsToDoom(p2).ToFloatPtr(), &filter, &path[0], &numPaths, NAV_MAX_PATHSTEPS);

	if (numPaths <= 0)
		return false;

	waypoints.Clear();

	for (int i = 0; i < numPaths; i++)
	{
		idVec3 origin;
		bool ignored;

		const dtMeshTile* meshTile;
		const dtPoly* meshPoly;

		m_navMesh->getTileAndPolyByRef(path[i], &meshTile, &meshPoly);

		idBounds polyBounds;
		polyBounds.Clear();
		
		for (int i = 0; i < meshPoly->vertCount; i++)
		{
			idVec3 vertex(meshTile->verts[(meshPoly->verts[i] * 3) + 0], meshTile->verts[(meshPoly->verts[i] * 3) + 1], meshTile->verts[(meshPoly->verts[i] * 3) + 2]);
			polyBounds.AddPoint(vertex);
		}

		m_navquery->closestPointOnPoly(path[i], polyBounds.GetCenter().ToFloatPtr(), origin.ToFloatPtr(), &ignored);
		waypoints.Append(ConvertNavToDoomCoords(origin));
	}

	waypoints.Append(p2);

	return true;
}

/*
===================
rvmNavFileLocal::LoadFromFile
===================
*/
bool rvmNavFileLocal::LoadFromFile(void) {
	idFileScoped file(fileSystem->OpenFileRead(name));
	navFileHeader_t header;
	
	if(file == nullptr)
	{
		common->Warning("Failed to open %s\n", name.c_str());
		return false;
	}
	
	file->Read(&header, sizeof(navFileHeader_t));
	if(header.version != NAV_FILE_VERSION) {
		common->Warning("Invalid header version\n");
		return false;
	}
	
	m_navMesh = dtAllocNavMesh();
	if (!m_navMesh)
	{
		return false;
	}

	dtNavMeshParams params;
	file->Read(&params, sizeof(dtNavMeshParams));

	dtStatus status = m_navMesh->init(&params);
	if (dtStatusFailed(status))
	{
		return false;
	}

	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		file->Read(&tileHeader, sizeof(NavMeshTileHeader));
		
		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data) break;
		memset(data, 0, tileHeader.dataSize);
		file->Read(data, tileHeader.dataSize);
		m_navMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	m_navquery = dtAllocNavMeshQuery();
	status = m_navquery->init(m_navMesh, 2048);
	if (dtStatusFailed(status))
	{
		common->FatalError("Could not init Detour navmesh query");
		return false;
	}

	return true;
}

/*
===================
rvmNavFileLocal::WriteNavToOBJ
===================
*/
void rvmNavFileLocal::WriteNavToOBJ(const char *name, rcPolyMeshDetail *mesh) {
	idStr navFileName;
	navFileName = "maps/";
	navFileName += name;
	navFileHeader_t header;

	navFileName.SetFileExtension("_navmesh.obj");

	// Open the nav file for writing.
	idFileScoped file(fileSystem->OpenFileWrite(navFileName));

	idList<idVec3> vertexes;
	for(int i = 0; i < mesh->nverts; i++)
	{
		idVec3 vertex;
		vertex.x = mesh->verts[(i * 3) + 0];
		vertex.y = mesh->verts[(i * 3) + 1];
		vertex.z = mesh->verts[(i * 3) + 2];
		vertexes.Append(vertex);
	}

	for (int i = 0; i < vertexes.Num(); i++)
	{
		file->Printf("v %f %f %f\n", vertexes[i].x, vertexes[i].y, vertexes[i].z);
	}

	file->Printf("\n");

	for (int i = 0; i < vertexes.Num(); i+=3)
		file->Printf("f %d %d %d\n", i + 1, i + 2, i + 3);
}

/*
===================
rvmNavFileLocal::WriteNavFile
===================
*/
void rvmNavFileLocal::WriteNavFile(const char *name, rcPolyMesh *mesh, rcPolyMeshDetail *detailMesh, int mapCRC, const idDeclEntityDef* botNavDecl) {
	idStr navFileName;
	navFileName = "maps/";
	navFileName += name;
	navFileHeader_t header;

	navFileName.SetFileExtension(NAV_FILE_EXTENSION);

	if(nav_dumpobj.GetBool())
	{
		WriteNavToOBJ(name, detailMesh);
	}

	// Open the nav file for writing.
	idFileScoped file(fileSystem->OpenFileWrite(navFileName));

	// Fill in the header.
	header.version = NAV_FILE_VERSION;
	header.mapCRC = mapCRC;

	// Write out the navmesh data.
	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	params.verts = mesh->verts;
	params.vertCount = mesh->nverts;
	params.polys = mesh->polys;
	params.polyAreas = mesh->areas;
	params.polyFlags = mesh->flags;
	params.polyCount = mesh->npolys;
	params.nvp = mesh->nvp;
	params.detailMeshes = detailMesh->meshes;
	params.detailVerts = detailMesh->verts;
	params.detailVertsCount = detailMesh->nverts;
	params.detailTris = detailMesh->tris;
	params.detailTriCount = detailMesh->ntris;
	//params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
	//params.offMeshConRad = m_geom->getOffMeshConnectionRads();
	//params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
	//params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
	//params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
	//params.offMeshConUserID = m_geom->getOffMeshConnectionId();
	//params.offMeshConCount = m_geom->getOffMeshConnectionCount();
	params.walkableHeight = BOT_GET_VALUE(m_agentHeight);
	params.walkableRadius = BOT_GET_VALUE(m_agentRadius);
	params.walkableClimb = BOT_GET_VALUE(m_agentMaxClimb);
	rcVcopy(params.bmin, mesh->bmin);
	rcVcopy(params.bmax, mesh->bmax);
	params.cs = BOT_GET_VALUE(m_cellSize);
	params.ch = BOT_GET_VALUE(m_cellHeight);
	params.buildBvTree = false;

	unsigned char* navData = 0;
	int navDataSize = 0;

	if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
	{
		common->FatalError("Could not build Detour navmesh.");
		return;
	}

	dtNavMesh* dtmesh = dtAllocNavMesh();
	if (!mesh)
	{
		common->FatalError("Could not create Detour navmesh");
		return;
	}

	dtStatus status;

	status = dtmesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
	if (dtStatusFailed(status))
	{
		common->FatalError("Could not init Detour navmesh");
		return;
	}

	header.numTiles = 0;
	for (int i = 0; i < dtmesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = dtmesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		header.numTiles++;
	}

	// Write out the header.
	file->Write(&header, sizeof(navFileHeader_t));

	file->Write(dtmesh->getParams(), sizeof(dtNavMeshParams));

	// Store tiles.
	for (int i = 0; i < dtmesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = dtmesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = dtmesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;
		file->Write(&tileHeader, sizeof(tileHeader));

		file->Write(tile->data, tile->dataSize);
	};
}
