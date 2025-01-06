// Nav_local.h
//

#include "../external/recast/include/Recast.h"
#include "../external/detour/include/DetourNavMesh.h"
#include "../external/detour/include/DetourNavMeshBuilder.h"
#include "../external/detour/include/DetourNavMeshQuery.h"

struct rcPolyMesh;
class dtNavMesh;
struct rcPolyMeshDetail;

#define BOT_GET_VALUE(x) botNavDecl->dict.FindKey(#x)->GetValue().ToFloat()

//
// navFileHeader_t
//
struct navFileHeader_t {
	int version;
	int mapCRC;
	int numTiles;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int dataSize;
};

//
// rvmNavFileLocal
//
class rvmNavFileLocal : public rvmNavFile {
public:
	rvmNavFileLocal(const char *name);
	~rvmNavFileLocal();

	bool LoadFromFile(void);

	virtual const char *GetName() { return name.c_str(); }
	virtual void GetRandomPointNearPosition(idVec3 point, idVec3 &randomPoint, float radius);
	virtual bool GetPathBetweenPoints(const idVec3 p1, const idVec3 p2, idList<idVec3>& waypoints);

	static void WriteNavFile(const char *name, rcPolyMesh *mesh, rcPolyMeshDetail *detailMesh, int mapCRC, const idDeclEntityDef * botNavDecl);
private:
	static void WriteNavToOBJ(const char *Name, rcPolyMeshDetail *mesh);

private:
	idStr name;
	dtNavMesh *m_navMesh;
	dtNavMeshQuery* m_navquery;
};

//
// rvmNavigationManager
//
class rvmNavigationManagerLocal : public rvmNavigationManager {
public:
	virtual void Init(void);

	virtual rvmNavFile *LoadNavFile(const char *name);
	virtual void FreeNavFile(rvmNavFile *navFile);
private:
	idList<rvmNavFileLocal *> navMeshFiles;
};

extern rvmNavigationManagerLocal navigationManagerLocal;
