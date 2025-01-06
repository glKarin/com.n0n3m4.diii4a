// Nav_public.h
//

#define NAV_FILE_VERSION				2
#define NAV_FILE_EXTENSION				".nav"
#define NAV_MAX_PATHSTEPS				512

//
// rvmNavFile
//
class rvmNavFile {
public:
	// Returns the name of this navmesh.
	virtual const char *GetName() = 0;

	// Finds a random walkable point near point.
	virtual void GetRandomPointNearPosition(idVec3 point, idVec3 &randomPoint, float radius) = 0;

	// Finds the path between two points.
	virtual bool GetPathBetweenPoints(const idVec3 p1, const idVec3 p2, idList<idVec3>& waypoints) = 0;
};

//
// rvmNavigationManager
//
class rvmNavigationManager {
public:
	// Init the nav manager.
	virtual void Init(void) = 0;

	// Loads in a navigation file.
	virtual rvmNavFile *LoadNavFile(const char *name) = 0;

	// Frees the navigation file.
	virtual void FreeNavFile(rvmNavFile *navFile) = 0;
private:
	
};

idVec3 NavConvertCoordsToDoom(idVec3 pt);

extern rvmNavigationManager *navigationManager;