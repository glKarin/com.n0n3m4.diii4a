#include "PrecompCommon.h"
#include "PathPlannerRecast.h"
#include "IGameManager.h"
#include "IGame.h"
#include "GoalManager.h"
#include "NavigationFlags.h"
#include "AStarSolver.h"
#include "gmUtilityLib.h"

#include "RecastInterfaces.h"

#include <DebugDraw.h>

#include <Recast.h>
#include <RecastDebugDraw.h>

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>

#include <DetourDebugDraw.h>

#ifdef _MSC_VER
#pragma warning( disable: 4127 )	// conditional expression is constant
#endif

duDebugDraw * gDDraw = 0;

static const obuint32 NAVMESH_MAGIC = 'OMNI';
static const int NAVMESH_VERSION = 2;

static const bool DUMP_OBJS = true;

RecastBuildContext buildContext;

void calcPolyCenter(float* tc, const unsigned short* idx, int nidx, const float* verts);

//////////////////////////////////////////////////////////////////////////

PathPlannerRecast	*gRecastPlanner = 0;

struct RecastOptions_t
{
	float DrawOffset;
	float TimeShare;
	bool DrawSolidNodes;

	enum DrawMode
	{
		DRAWMODE_NAVMESH,
		DRAWMODE_NAVMESH_BVTREE,
		DRAWMODE_VOXELS,
		DRAWMODE_VOXELS_WALKABLE,
		DRAWMODE_COMPACT,
		DRAWMODE_COMPACT_DISTANCE,
		DRAWMODE_COMPACT_REGIONS,
		DRAWMODE_REGION_CONNECTIONS,
		DRAWMODE_RAW_CONTOURS,
		DRAWMODE_BOTH_CONTOURS,
		DRAWMODE_CONTOURS,
		DRAWMODE_POLYMESH,
		DRAWMODE_POLYMESH_DETAIL,
		MAX_DRAWMODE
	};

	DrawMode	RenderMode;

	bool FilterLedges;
	bool FilterLowHeight;

	bool ClimbWalls;
} RecastOptions = {};

struct RecastStats_t
{
	int		ExploredCells;
	int		BorderCells;
	double	FloodTime;

	RecastStats_t() 
		: ExploredCells(0)
		, BorderCells(0)
		, FloodTime(0.0)
	{
	}
} RecastStats;

enum FloodFillStatus
{
	Process_Init,
	Process_Flood,
	Process_FloodFinished,
	Process_BuildNavMesh,
	Process_BuildStaticMesh,
	Process_BuildTiledMesh,
	Process_FinishedNavMesh,
};

static FloodFillStatus gFloodStatus = Process_Init;

struct RecastNode
{
	Vector3f	Pos;
};

typedef std::list<RecastNode> RecastNodeList;
RecastNodeList RecastOpenList;
RecastNodeList RecastExploredList;
RecastNodeList RecastSolidList;

rcConfig				RecastCfg;

// todo: init these from game
float AgentHeight = 64.f;
float AgentRadius = 18.f;
float AgentMaxClimb = 18.f;

rcHeightfield * FloodHeightField = 0; // voxel height field

struct rcBuildData
{
	rcCompactHeightfield	* Chf;
	rcContourSet			* Contour;
	rcPolyMesh				* PolyMesh;
	rcPolyMeshDetail		* PolyMeshDetail;

	rcBuildData() : Chf(0), Contour(0), PolyMesh(0), PolyMeshDetail(0)
	{
	}
	void Alloc()
	{
		Chf = rcAllocCompactHeightfield();
		Contour = rcAllocContourSet();
		PolyMesh = rcAllocPolyMesh();
		PolyMeshDetail = rcAllocPolyMeshDetail();
	}
	~rcBuildData()
	{
		rcFreeCompactHeightfield( Chf ); Chf = NULL;
		rcFreeContourSet( Contour ); Contour = NULL;
		rcFreePolyMesh( PolyMesh ); PolyMesh = NULL;
		rcFreePolyMeshDetail( PolyMeshDetail ); PolyMeshDetail = NULL;
	}
};
typedef std::list<rcBuildData> BuildDataList;
BuildDataList			BuildData;

//typedef std::vector<dtPathLink> LinkList;
//LinkList				LinkData;

enum NavMeshType { NavMeshStatic,NavMeshTiled,NavMeshNum };
NavMeshType				CurrentNavMeshType = NavMeshStatic;

dtNavMesh * DetourNavmesh = 0;

typedef std::list<AABB> BoundsList;
BoundsList				FloodEntityBounds;

enum { MaxConnections=256 };
int				OffMeshConnectionNum = 0;
Vector3f		OffMeshConnections[MaxConnections*2] = {};
float			OffMeshConnectionRadius[MaxConnections] = {};
unsigned char	OffMeshConnectionDirection[MaxConnections] = {};
unsigned char	OffMeshConnectionArea[MaxConnections] = {};
unsigned short	OffMeshConnectionFlags[MaxConnections] = {};

//////////////////////////////////////////////////////////////////////////

struct NavObstacles
{
	GameEntity		Entity;
	AABB			EntityWorldBounds;
	/*AABB			EntityBounds;
	Matrix3f		EntityOrientation;
	Vector3f		EntityPosition;*/

	enum ObstacleState { OBS_NONE,OBS_PENDING,OBS_ADDING,OBS_ADDED,OBS_REMOVING };
	ObstacleState	State;

	void Free() { Entity.Reset(); State = OBS_NONE; }

	NavObstacles() : State(OBS_NONE) {}
};

enum { MaxObstacles = 256 };
static NavObstacles	gNavObstacles[MaxObstacles];

//////////////////////////////////////////////////////////////////////////
Vector3f ToRecast(const Vector3f &v)
{
	return Vector3f(v.x,v.z,v.y);
}
//////////////////////////////////////////////////////////////////////////

PathPlannerRecast::PathPlannerRecast()
{
	m_PlannerFlags.SetFlag(NAV_VIEW);
	gRecastPlanner = this;
}

PathPlannerRecast::~PathPlannerRecast()
{
	Shutdown();
	gRecastPlanner = NULL;
}

int PathPlannerRecast::GetLatestFileVersion() const
{
	return 1; // TODO
}

bool PathPlannerRecast::Init()
{
	RecastOptions.DrawSolidNodes = false;
	RecastOptions.DrawOffset = 4.f;
	RecastOptions.TimeShare = 0.01f;

	RecastCfg.cs = 8.f;
	RecastCfg.ch = 8.f;
	RecastCfg.walkableSlopeAngle = 45;
	RecastCfg.maxEdgeLen = 32;
	RecastCfg.maxSimplificationError = 1.f;
	RecastCfg.minRegionSize = 0;
	RecastCfg.mergeRegionSize = 0;
	RecastCfg.maxVertsPerPoly = 6;
	RecastCfg.borderSize = 0;
	RecastCfg.detailSampleDist = 6.f;
	RecastCfg.detailSampleMaxError = 8.f;

	RecastCfg.walkableHeight = (int)ceilf(AgentHeight / RecastCfg.ch);
	RecastCfg.walkableClimb = (int)floorf(AgentMaxClimb / RecastCfg.ch);
	RecastCfg.walkableRadius = (int)ceilf(AgentRadius / RecastCfg.cs);

	InitCommands();
	return true;
}

void PathPlannerRecast::Update()
{
	Prof(PathPlannerRecast);

	if(m_PlannerFlags.CheckFlag(NAV_VIEW))
	{
		for( int i = 0; i < buildContext.getLogCount(); ++i ) {
			const char * logTxt = buildContext.getLogText( i );
			EngineFuncs::ConsoleMessage(logTxt);
		}		
		buildContext.resetLog();

		// surface probe
		int contents = 0, surface = 0;
		Vector3f vAimPt, vAimNormal;
		if(Utils::GetLocalAimPoint(vAimPt, &vAimNormal, TR_MASK_FLOODFILL, &contents, &surface))
		{
			obColor clr = COLOR::WHITE;
			if(surface & SURFACE_LADDER) 
			{
				clr = COLOR::ORANGE;
			}
			Utils::DrawLine(vAimPt, vAimPt + vAimNormal * 16.f, clr, IGame::GetDeltaTimeSecs()*2.f);
		}
	}
}

void PathPlannerRecast::Shutdown()
{
	Unload();
}

bool PathPlannerRecast::Load(const String &_mapname, bool _dl)
{
	Unload();

	String waypointName	= _mapname + ".nav";
	String navPath	= String("nav/") + waypointName;
	String error;

	dtFreeNavMesh( DetourNavmesh ); 
	DetourNavmesh = NULL;

	File f;
	if(f.OpenForRead(navPath.c_str(),File::Binary))
	{
		obuint32 MagicNum = 0;
		if(!f.ReadInt32(MagicNum) || MagicNum != NAVMESH_MAGIC)
			goto errorLabel;

		obuint32 Version = 0;
		if(!f.ReadInt32(Version) || Version != NAVMESH_VERSION)
			goto errorLabel;

		obuint32 NumTiles = 0;
		if(!f.ReadInt32(NumTiles))
			goto errorLabel;

		dtNavMeshParams params;
		memset( &params, 0, sizeof( params ) );
		if(!f.Read(&params,sizeof(params), 1))
			goto errorLabel;

		DetourNavmesh = dtAllocNavMesh();
		if( !DetourNavmesh->init( &params ) )
			goto errorLabel;

		for(obuint32 t = 0; t < NumTiles; ++t)
		{
			obuint32 tileRef = 0, dataSize = 0;
			if(!f.ReadInt32( tileRef ))
				goto errorLabel;

			if(!f.ReadInt32( dataSize ))
				goto errorLabel;

			unsigned char* data = (unsigned char*)dtAlloc( dataSize, DT_ALLOC_PERM );
			if(!f.Read( data, dataSize, 1 ))
				goto errorLabel;

			DetourNavmesh->addTile(data, dataSize, DT_TILE_FREE_DATA, tileRef);
		}
		return true;
	}

errorLabel:
	dtFreeNavMesh( DetourNavmesh );
	DetourNavmesh = NULL;
	return false;
}

bool PathPlannerRecast::Save(const String &_mapname)
{
	String waypointName	= _mapname + ".nav";
	String navPath	= String("nav/") + waypointName;
	
	const dtNavMesh * SaveMesh = DetourNavmesh;

	if(SaveMesh)
	{
		File f;
		if(f.OpenForWrite(navPath.c_str(),File::Binary))
		{
			f.WriteInt32(NAVMESH_MAGIC);
			f.WriteInt32(NAVMESH_VERSION);
			
			int numTiles = 0;
			for(int t = 0; t < SaveMesh->getMaxTiles(); ++t)
			{
				const dtMeshTile * tile = SaveMesh->getTile(t);
				if( !tile || !tile->header || !tile->dataSize ) {
					continue;
				}
				numTiles++;
			}
			f.WriteInt32(numTiles);

			const dtNavMeshParams * params = SaveMesh->getParams();
			f.Write( params, sizeof( dtNavMeshParams ), 1);

			// Store tiles.
			for(int t = 0; t < SaveMesh->getMaxTiles(); ++t)
			{
				const dtMeshTile* tile = SaveMesh->getTile(t);
				if( !tile || !tile->header || !tile->dataSize ) {
					continue;
				}
			
				dtTileRef tileRef = SaveMesh->getTileRef( tile );
				const int tileDataSize = tile->dataSize;

				f.WriteInt32( tileRef );
				f.WriteInt32( tileDataSize );
				f.Write( tile->data, tile->dataSize, 1 );
			}
			f.Close();
			return true;
		}
	}
	return false;
}

bool PathPlannerRecast::IsReady() const
{
	return DetourNavmesh && DetourNavmesh->getMaxTiles() > 0;
}

bool PathPlannerRecast::GetNavFlagByName(const String &_flagname, NavFlags &_flag) const
{	
	_flag = 0;
	return false;
}

Vector3f PathPlannerRecast::GetRandomDestination(Client *_client, const Vector3f &_start, const NavFlags _team)
{
	Vector3f dest = _start;
	if(DetourNavmesh && DetourNavmesh->getMaxTiles() > 0)
	{
		//DetourNavmesh->getRandomPolyPosition(rand(),dest);
	}
	return Vector3f(dest.x,dest.z,dest.y);
}

//////////////////////////////////////////////////////////////////////////

int PathPlannerRecast::PlanPathToNearest(Client *_client, const Vector3f &_start, const Vector3List &_goals, const NavFlags &_team)
{
	DestinationVector dst;
	for(obuint32 i = 0; i < _goals.size(); ++i)
		dst.push_back(Destination(_goals[i],32.f));
	return PlanPathToNearest(_client,_start,dst,_team);
}

static int NumPathPoints = 0;

enum { MaxPathPoints = 1024 };
static float StraightPath[MaxPathPoints*3];
static int StraightPathPoints = 0;
static unsigned char StraightPathFlags[MaxPathPoints];
static dtPolyRef StraightPathPolys[MaxPathPoints];

int PathPlannerRecast::PlanPathToNearest(Client *_client, const Vector3f &_start, const DestinationVector &_goals, const NavFlags &_team)
{
	//if(DetourNavmesh)
	//{
	//	if(CurrentNavMeshType == NavMeshStatic)
	//	{
	//		dtPolyRef PathPolys[MaxPathPoints];

	//		Vector3f startPos, destPos;
	//		dtQueryFilter query;
	//		Vector3f extents(512.f,512.f,512.f);
	//		const dtPolyRef startPoly = DetourNavmesh->findNearestPoly(ToRecast(_start), extents, &query, startPos);
	//		const dtPolyRef destPoly = DetourNavmesh->findNearestPoly(ToRecast(_goals[0].m_Position), extents, &query, destPos);

	//		// TODO: search for all goals
	//		NumPathPoints = DetourNavmesh->findPath(
	//			startPoly,destPoly,
	//			startPos,
	//			destPos,
	//			&query,
	//			PathPolys, MaxPathPoints);

	//		StraightPathPoints = NumPathPoints > 0 ? DetourNavmesh->findStraightPath(
	//			startPos,
	//			destPos,
	//			PathPolys,
	//			NumPathPoints,
	//			StraightPath,
	//			StraightPathFlags,
	//			StraightPathPolys,
	//			MaxPathPoints) : 0;

	//		return NumPathPoints > 0 ? 0 : -1;
	//	}
	//	else if(CurrentNavMeshType == NavMeshTiled)
	//	{
	//		//dtTilePolyRef PathPolys[MaxPathPoints];

	//		//Vector3f extents(512.f,512.f,512.f);
	//		//const dtTilePolyRef startPoly = DetourTiledNavmesh.findNearestPoly(ToRecast(_start), extents);
	//		//const dtTilePolyRef destPoly = DetourTiledNavmesh.findNearestPoly(ToRecast(_goals[0].m_Position), extents);

	//		//// TODO: search for all goals
	//		//NumPathPoints = DetourTiledNavmesh.findPath(
	//		//	startPoly,destPoly,
	//		//	ToRecast(_start),
	//		//	ToRecast(_goals[0].m_Position),
	//		//	PathPolys, MaxPathPoints);

	//		//StraightPathPoints = NumPathPoints > 0 ? DetourTiledNavmesh.findStraightPath(
	//		//	ToRecast(_start),
	//		//	ToRecast(_goals[0].m_Position),
	//		//	PathPolys,
	//		//	NumPathPoints,
	//		//	StraightPath,
	//		//	MaxPathPoints) : 0;
	//		return NumPathPoints > 0 ? 0 : -1;
	//	}
	//}
	return -1;
}

void PathPlannerRecast::PlanPathToGoal(Client *_client, const Vector3f &_start, const Vector3f &_goal, const NavFlags _team)
{
	DestinationVector dst;
	dst.push_back(Destination(_goal,32.f));
	PlanPathToNearest(_client,_start,dst,_team);
}

bool PathPlannerRecast::IsDone() const
{
	return true;
}
bool PathPlannerRecast::FoundGoal() const
{
	//return g_PathFinder.FoundGoal();
	return NumPathPoints > 0;
}

void PathPlannerRecast::Unload()
{
	FloodEntityBounds.clear();
	RecastOpenList.clear();

	RecastSolidList.clear();

	BuildData.clear();

	rcFreeHeightField( FloodHeightField );
	FloodHeightField = NULL;
	dtFreeNavMesh( DetourNavmesh );
	DetourNavmesh = NULL;
}

void PathPlannerRecast::RegisterGameGoals()
{
}

void PathPlannerRecast::GetPath(Path &_path, int _smoothiterations)
{
	for(int i = 0; i < StraightPathPoints; ++i)
	{
		_path.AddPt(
			Vector3f(
			StraightPath[i*3],
			StraightPath[i*3+2],
			StraightPath[i*3+1]),
			32.f);
	}
}

//////////////////////////////////////////////////////////////////////////

static int GetFloodTraceMask(const Vector3f &v, const AABB &voxel)
{
	/*for(BoundsList::const_iterator cIt = FloodEntityBounds.begin();
	cIt != FloodEntityBounds.end();
	++cIt)
	{
	AABB voxelWorld = voxel;
	voxelWorld.Translate(v);
	if((*cIt).Intersects(voxelWorld))
	return TR_MASK_FLOODFILLENT;
	}*/
	return TR_MASK_FLOODFILL;
}

//////////////////////////////////////////////////////////////////////////
int PathPlannerRecast::Process_FloodFill()
{
	Prof(Process_FloodFill);

	switch(gFloodStatus)
	{
	case Process_Init:
		{
			RecastStats = RecastStats_t();

			RecastSolidList.clear();
			RecastOpenList.clear();

			rcFreeHeightField( FloodHeightField );
			FloodHeightField = NULL;
			dtFreeNavMesh( DetourNavmesh );
			DetourNavmesh = NULL;

			FloodHeightField = rcAllocHeightfield();
			DetourNavmesh = dtAllocNavMesh();
			
			buildContext.enableLog( true );
			buildContext.resetLog();
			buildContext.enableTimer( true );
			buildContext.resetTimers();

			AABB mapSize;
			g_EngineFuncs->GetMapExtents(mapSize);

			// convert to recast space
			RecastCfg.bmin[0] = mapSize.m_Mins[0];
			RecastCfg.bmin[1] = -4096.f;//mapSize.m_Mins[2];
			RecastCfg.bmin[2] = mapSize.m_Mins[1];
			RecastCfg.bmax[0] = mapSize.m_Maxs[0];
			RecastCfg.bmax[1] = 4096.f;//mapSize.m_Maxs[2];
			RecastCfg.bmax[2] = mapSize.m_Maxs[1];	

			rcCalcGridSize(
				RecastCfg.bmin, 
				RecastCfg.bmax, 
				RecastCfg.cs, 
				&RecastCfg.width, 
				&RecastCfg.height);

			rcCreateHeightfield(
				&buildContext,
				*FloodHeightField, 
				RecastCfg.width, 
				RecastCfg.height, 
				RecastCfg.bmin, 
				RecastCfg.bmax, 
				RecastCfg.cs, 
				RecastCfg.ch);

			//////////////////////////////////////////////////////////////////////////
			// Look for special entity types that we might need to treat differently
			const int iMaxFeatures = 1024;
			AutoNavFeature features[iMaxFeatures] = {};
			int iNumFeatures = g_EngineFuncs->GetAutoNavFeatures(features, iMaxFeatures);
			for(int i = 0; i < iNumFeatures; ++i)
			{
				Vector3f vPos(features[i].m_Position);		
				Vector3f vFace(features[i].m_Facing);
				Vector3f vTarget(features[i].m_TargetPosition);

				AddFloodSeed(vPos);
				if(vPos != vTarget)
				{
					AddFloodSeed(vTarget);
					//pFeature->ConnectTo(pTarget);
				}

				if(features[i].m_ObstacleEntity)
				{
					AddFloodEntityBounds(features[i].m_Bounds);

					//Utils::OutlineAABB(features[i].m_Bounds,COLOR::MAGENTA,30.f);
					continue;
				}

				OffMeshConnections[OffMeshConnectionNum*2] = ToRecast(vPos);
				OffMeshConnections[OffMeshConnectionNum*2+1] = ToRecast(vTarget);
				OffMeshConnectionRadius[OffMeshConnectionNum] = AgentRadius;
				OffMeshConnectionDirection[OffMeshConnectionNum] = features[i].m_BiDirectional;
				OffMeshConnectionArea[OffMeshConnectionNum] = NAV_AREA_GROUND;
				OffMeshConnectionFlags[OffMeshConnectionNum] = 0;

				switch (features[i].m_Type)
				{
				case ENT_CLASS_GENERIC_TELEPORTER:
					OffMeshConnectionFlags[OffMeshConnectionNum] = NAV_FLAG_WALK;
					break;
				case ENT_CLASS_GENERIC_LADDER:
					OffMeshConnectionFlags[OffMeshConnectionNum] = NAV_FLAG_LADDER;
					break;
				/*case ENT_CLASS_GENERIC_LIFT:
				case ENT_CLASS_GENERIC_MOVER:
				case ENT_CLASS_GENERIC_JUMPPAD:
					break;*/
				default:
					OffMeshConnectionFlags[OffMeshConnectionNum] = 0;
					break;
				}

				if(OffMeshConnectionFlags[OffMeshConnectionNum])
				{
					++OffMeshConnectionNum;
				}
			}

			for(obuint32 i = 0; i < ladders.size(); ++i)
			{
				OffMeshConnections[OffMeshConnectionNum*2] = ToRecast(ladders[i].bottom);
				OffMeshConnections[OffMeshConnectionNum*2+1] = ToRecast(ladders[i].top);
				OffMeshConnectionRadius[OffMeshConnectionNum] = AgentRadius;
				OffMeshConnectionDirection[OffMeshConnectionNum] = 0/*features[i].m_BiDirectional*/;
				OffMeshConnectionArea[OffMeshConnectionNum] = NAV_AREA_LADDER;
				OffMeshConnectionFlags[OffMeshConnectionNum] = NAV_FLAG_LADDER;
				++OffMeshConnectionNum;
			}

			RecastStats.FloodTime = 0.0;
			gFloodStatus = Process_Flood;
			break;
		}
	case Process_Flood:
		{
			Timer tme;

			const float walkableThr = cosf(RecastCfg.walkableSlopeAngle/180.0f*(float)Mathf::PI);

			AABB voxel;
			voxel.m_Mins[0] = -(RecastCfg.cs * 0.5f);
			voxel.m_Mins[1] = -(RecastCfg.cs * 0.5f);
			voxel.m_Mins[2] = -(RecastCfg.cs * 0.5f);
			voxel.m_Maxs[0] =  (RecastCfg.cs * 0.5f);
			voxel.m_Maxs[1] =  (RecastCfg.cs * 0.5f);
			voxel.m_Maxs[2] =  (RecastCfg.cs * 0.5f);

			const float timeshare = RecastOptions.TimeShare;
			const bool infiniteTime = timeshare == 100.f;

			bool DidSomething = false;
			while(!RecastOpenList.empty())
			{
				DidSomething = true;
				const double elapsed = tme.GetElapsedSeconds();
				if(!infiniteTime && elapsed > timeshare)
				{
					RecastStats.FloodTime += elapsed;
					return Function_InProgress;
				}

				const RecastNode currentNode = RecastOpenList.front();
				RecastOpenList.pop_front();

				if(!infiniteTime)
					RecastExploredList.push_back(currentNode);
				RecastStats.ExploredCells++;

				const int TRACE_MASK = GetFloodTraceMask(currentNode.Pos,voxel);

				// find the floor
				obTraceResult trFloor;
				EngineFuncs::TraceLine(
					trFloor,
					currentNode.Pos + Vector3f::UNIT_Z * RecastCfg.ch,
					currentNode.Pos + -Vector3f::UNIT_Z * 1024.f,
					&voxel,
					TRACE_MASK,
					-1,
					False);

				if(trFloor.m_Surface & SURFACE_LADDER) {
					Utils::DrawLine(
						Vector3f(trFloor.m_Endpos),
						Vector3f(trFloor.m_Endpos)+Vector3f(trFloor.m_Normal),
						COLOR::ORANGE,
						20.f);
				}

				// didnt hit anything? skip it
				if(trFloor.m_Fraction==1.f || trFloor.m_Endpos[2] < RecastCfg.bmin[1])
					continue;

				// climb the walls
				if(trFloor.m_StartSolid)
				{
					RecastSolidList.push_back(currentNode);
					RecastStats.BorderCells++;

					if(RecastOptions.ClimbWalls)
					{
						obTraceResult trWall;
						Vector3f vWall = currentNode.Pos;

						do 
						{
							rcRasterizeVertex( &buildContext,
								ToRecast( vWall ),
								RC_NULL_AREA,
								*FloodHeightField);

							vWall.z += RecastCfg.ch;
							EngineFuncs::TraceLine(
								trFloor,
								vWall,
								vWall + -Vector3f::UNIT_Z * 1024.f,
								&voxel,
								TRACE_MASK,
								-1,
								False);

							if(!infiniteTime)
							{
								RecastNode rcn = { vWall };
								RecastExploredList.push_back(rcn);
							}
							RecastStats.ExploredCells++;

						} while(trFloor.m_StartSolid && (vWall.z - currentNode.Pos.z) <= 512.f);					

						// if we found a non solid, add it to the open list to explore further.
						if(!trFloor.m_StartSolid)
						{
							RecastNode expand;
							expand.Pos = vWall;
							RecastOpenList.push_back(expand);
						}
					}
					continue;
				}

				obuint8 area = RC_NULL_AREA;
				if(trFloor.m_Normal[2] > walkableThr)
					area |= (obuint8)RC_WALKABLE_AREA;
				//if(trFloor.m_Contents & CONT_WATER)

				const Vector3f floorPos(trFloor.m_Endpos);				
				if( !rcRasterizeVertex( &buildContext, ToRecast(floorPos), area, *FloodHeightField ) )
					continue;

				// get the ceiling height
				static bool doCeiling = false;
				if(doCeiling)
				{
					obTraceResult trCeiling;
					EngineFuncs::TraceLine(
						trCeiling,
						currentNode.Pos,
						currentNode.Pos + Vector3f::UNIT_Z * 1024.f,
						&voxel,
						TRACE_MASK,
						-1,
						False);
					if(trCeiling.m_Fraction < 1.f)
					{
						const Vector3f ceilPos(trCeiling.m_Endpos);
						rcRasterizeVertex(&buildContext,ToRecast(ceilPos),RC_NULL_AREA,*FloodHeightField);
					}
				}				

				// explore neighbors
				const Vector3f Expand[4] =
				{
					Vector3f(1.0f, 0.0f, 0.0f),
					Vector3f(0.0f, 1.0f, 0.0f),
					Vector3f(-1.0f, 0.0f, 0.0f),		
					Vector3f(0.0f, -1.0f, 0.0f),		
				};

				for(int d = 0; d < 4; ++d)
				{
					RecastNode expand;
					expand.Pos = floorPos + Expand[d] * RecastCfg.cs;
					expand.Pos.z += RecastCfg.walkableClimb;
					RecastOpenList.push_back(expand);
				}
			}
			if(DidSomething)
				RecastStats.FloodTime += tme.GetElapsedSeconds();
			gFloodStatus = Process_FloodFinished;
			break;
		}
	case Process_FloodFinished:
		{
			if(!RecastOpenList.empty())
				gFloodStatus = Process_Flood;
			break;
		}
	case Process_BuildNavMesh:
		{
			BuildData.clear();
			switch(CurrentNavMeshType)
			{
			case NavMeshStatic:
				gFloodStatus = Process_BuildStaticMesh;
				break;
			case NavMeshTiled:
				gFloodStatus = Process_BuildTiledMesh;
				break;
			default:
				break;
			}
			break;
		}
	case Process_BuildStaticMesh:
		{
			dtFreeNavMesh( DetourNavmesh );
			DetourNavmesh = NULL;
			
			DetourNavmesh = dtAllocNavMesh();

			BuildData.push_back(rcBuildData());
			rcBuildData &build = BuildData.back();
			build.Alloc();
			// TODO: rcMarkWalkableTriangles
			// TODO: rcFilterWalkableLowHeightSpans
			// TODO: rcFilterLedgeSpans
			// TODO: rcFilterWalkableBorderSpans

			if(RecastOptions.FilterLedges)
			{
				rcFilterLedgeSpans(
					&buildContext,
					RecastCfg.walkableHeight,
					RecastCfg.walkableClimb,
					*FloodHeightField);
			}

			if(RecastOptions.FilterLowHeight)
			{
				rcFilterWalkableLowHeightSpans(
					&buildContext,
					RecastCfg.walkableHeight,
					*FloodHeightField);
			}			

			rcFilterLowHangingWalkableObstacles( &buildContext, 
				RecastCfg.walkableClimb, 
				*FloodHeightField );

			rcFilterLedgeSpans( &buildContext, 
				RecastCfg.walkableHeight,
				RecastCfg.walkableClimb, 
				*FloodHeightField );

			rcFilterWalkableLowHeightSpans( &buildContext,
				RecastCfg.walkableHeight,
				*FloodHeightField );

			if ( DUMP_OBJS ) {
				const char * mapName = g_EngineFuncs->GetMapName();
				FileIO obj, mat;
				const char * objFileName = va( "%s_walkhf.obj", mapName );
				const char * matFileName = va( "%s_walkhf.mat", mapName );
				if ( obj.openForWrite( objFileName ) && mat.openForWrite( matFileName )) {
					duDebugDrawDump dmp( &obj, &mat, matFileName );
					duDebugDrawHeightfieldWalkable( &dmp, *FloodHeightField );
				}
			}

			if(!rcBuildCompactHeightfield(
				&buildContext,
				RecastCfg.walkableHeight,
				RecastCfg.walkableClimb,
				*FloodHeightField,
				*build.Chf))
			{
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			if(!rcBuildDistanceField(&buildContext,*build.Chf))
			{
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			if ( DUMP_OBJS ) {
				const char * mapName = g_EngineFuncs->GetMapName();
				FileIO obj, mat;
				const char * objFileName = va( "%s_disthf.obj", mapName );
				const char * matFileName = va( "%s_disthf.mat", mapName );
				if ( obj.openForWrite( objFileName ) && mat.openForWrite( matFileName )) {
					duDebugDrawDump dmp( &obj, &mat, matFileName );
					duDebugDrawCompactHeightfieldDistance( &dmp, *build.Chf );
				}
			}

			if(!rcBuildRegions(&buildContext,
				*build.Chf,
				RecastCfg.borderSize,
				RecastCfg.minRegionSize, 
				RecastCfg.mergeRegionSize))
			{
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			if(!rcBuildContours(&buildContext,
				*build.Chf,
				RecastCfg.maxSimplificationError, 
				RecastCfg.maxEdgeLen, 
				*build.Contour))
			{
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			if(!rcBuildPolyMesh(&buildContext,
				*build.Contour,
				RecastCfg.maxVertsPerPoly,
				*build.PolyMesh))
			{
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			if ( DUMP_OBJS ) {
				FileIO obj;			
				if ( obj.openForWrite( va( "%s_pm.obj", g_EngineFuncs->GetMapName() ) ) )
					duDumpPolyMeshToObj( *build.PolyMesh, &obj );
			}

			if(!rcBuildPolyMeshDetail(&buildContext,
				*build.PolyMesh,
				*build.Chf,
				RecastCfg.detailSampleDist,
				RecastCfg.detailSampleMaxError,
				*build.PolyMeshDetail))
			{
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			if ( DUMP_OBJS ) {
				FileIO obj;			
				if ( obj.openForWrite( va( "%s_pmd.obj", g_EngineFuncs->GetMapName() ) ) )
					duDumpPolyMeshDetailToObj( *build.PolyMeshDetail, &obj );
			}

			dtNavMeshCreateParams params;
			memset(&params, 0, sizeof(params));
			params.cs = RecastCfg.cs;
			params.ch = RecastCfg.ch;
			params.verts =  build.PolyMesh->verts;
			params.vertCount =  build.PolyMesh->nverts;
			params.polys =  build.PolyMesh->polys;
			params.polyAreas =  build.PolyMesh->areas;
			params.polyFlags =  build.PolyMesh->flags;
			params.polyCount =  build.PolyMesh->npolys;
			params.nvp =  build.PolyMesh->nvp;
			params.detailMeshes = build.PolyMeshDetail->meshes;
			params.detailVerts = build.PolyMeshDetail->verts;
			params.detailVertsCount = build.PolyMeshDetail->nverts;
			params.detailTris = build.PolyMeshDetail->tris;
			params.detailTriCount = build.PolyMeshDetail->ntris;
			params.offMeshConVerts = (float*)OffMeshConnections;
			params.offMeshConRad = OffMeshConnectionRadius;
			params.offMeshConDir = OffMeshConnectionDirection;
			params.offMeshConAreas = OffMeshConnectionArea;
			params.offMeshConFlags = OffMeshConnectionFlags;
			params.offMeshConCount = OffMeshConnectionNum;
			params.walkableHeight = ceilf(AgentHeight / params.ch);
			params.walkableClimb = floorf(AgentMaxClimb / params.ch);
			params.walkableRadius = ceilf(AgentRadius / params.cs);

			rcVcopy(params.bmin, build.PolyMesh->bmin);
			rcVcopy(params.bmax, build.PolyMesh->bmax);
			
			unsigned char* navData = 0;
			int navDataSize = 0;
			if(!dtCreateNavMeshData(&params, &navData, &navDataSize))
			{
				buildContext.log( RC_LOG_ERROR, "Could not build Detour NavMesh." );				
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			if (!DetourNavmesh->init(navData, navDataSize, DT_TILE_FREE_DATA))
			{
				delete [] navData;
				buildContext.log( RC_LOG_ERROR, "Could not init Detour NavMesh." );
				gFloodStatus = Process_FinishedNavMesh;
				break;
			}

			EngineFuncs::ConsoleMessage("Navigation Mesh Generation.");
			/*EngineFuncs::ConsoleMessage("Heightfield Memory: %s",
			Utils::FormatByteString(FloodHeightField->getMemUsed()).c_str());
			EngineFuncs::ConsoleMessage("Compact Heightfield Memory: %s",
			Utils::FormatByteString(build.Chf->getMemUsed()).c_str());*/

			EngineFuncs::ConsoleMessage("Navigation Mesh Generated.");
			EngineFuncs::ConsoleMessage(va("Size: %s",Utils::FormatByteString(navDataSize).c_str()));
			//EngineFuncs::ConsoleMessage("# Polygons: %d",DetourNavmesh.getPolyCount());
			//EngineFuncs::ConsoleMessage("# Vertices: %d",DetourNavmesh.getVertexCount());
			gFloodStatus = Process_FinishedNavMesh;
			break;
		}
	case Process_BuildTiledMesh:
		{
			//DetourNavmesh.clear();

			//static float tileSize = 256.f, portalHeight = 512.f;
			//if (!DetourNavmesh.init(RecastCfg.bmin, tileSize, portalHeight))
			//{
			//	if (rcGetLog())
			//		rcGetLog()->log(RC_LOG_ERROR, "Could not init Detour Tiled NavMesh");
			//	gFloodStatus = Process_FinishedNavMesh;
			//	break;
			//}

			//rcConfig tileCfg = RecastCfg;
			//tileCfg.borderSize += RecastCfg.walkableRadius * 2 + 2;
			//tileCfg.width = tileCfg.tileSize + tileCfg.borderSize*2;
			//tileCfg.height = tileCfg.tileSize + tileCfg.borderSize*2;

			//static int buildTiles = 5;
			//const int ts = (int)DetourTiledNavmesh.getTileSize();
			//const int tw = (int)(FloodHeightField.width * FloodHeightField.cs) / ts;
			//const int th = (int)(FloodHeightField.height * FloodHeightField.cs) / ts;

			//int numBuilt = 0;
			//for(int x = 0; x < tw; ++x)
			//{
			//	for(int y = 0; y < th; ++y)
			//	{
			//		BuildData.push_back(rcBuildData());
			//		rcBuildData &build = BuildData.back();

			//		// grab a piece of the global map
			//		float mins[3] = 
			//		{
			//			FloodHeightField.bmin[0]+x*ts,
			//			FloodHeightField.bmin[1],
			//			FloodHeightField.bmin[2]+y*ts 
			//		};
			//		float maxs[3] = 
			//		{ 
			//			mins[0] + ts,
			//			FloodHeightField.bmax[1], 
			//			mins[2] + ts 
			//		};

			//		mins[0] -= tileCfg.borderSize*tileCfg.cs;
			//		mins[2] -= tileCfg.borderSize*tileCfg.cs;
			//		maxs[0] += tileCfg.borderSize*tileCfg.cs;
			//		maxs[2] += tileCfg.borderSize*tileCfg.cs;

			//		rcHeightfield Tilehf;
			//		rcGetHeightfieldSubRegion(FloodHeightField,Tilehf,mins,maxs);

			//		if(!rcBuildCompactHeightfield(
			//			tileCfg.walkableHeight,
			//			tileCfg.walkableClimb,
			//			RC_WALKABLE,
			//			Tilehf,
			//			build.Chf))
			//		{
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}
			//		
			//		//////////////////////////////////////////////////////////////////////////
			//		
			//		if(!rcBuildDistanceField(build.Chf))
			//		{
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}

			//		if(!rcBuildRegions(build.Chf,
			//			tileCfg.walkableRadius, 
			//			tileCfg.borderSize,
			//			tileCfg.minRegionSize, 
			//			tileCfg.mergeRegionSize))
			//		{
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}

			//		if(!rcBuildContours(build.Chf,
			//			tileCfg.maxSimplificationError, 
			//			tileCfg.maxEdgeLen, 
			//			build.Contour))
			//		{
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}

			//		if(build.Contour.nconts==0)
			//		{
			//			if (rcGetLog())
			//				rcGetLog()->log(RC_LOG_WARNING, "rcBuildContours: no contours created(%d,%d).",x,y);
			//			continue;
			//		}

			//		if(!rcBuildPolyMesh(build.Contour,
			//			tileCfg.maxVertsPerPoly,
			//			build.PolyMesh))
			//		{
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}

			//		if(!rcBuildPolyMeshDetail(build.PolyMesh,
			//			build.Chf,
			//			tileCfg.detailSampleDist,
			//			tileCfg.detailSampleMaxError,
			//			build.PolyMeshDetail))
			//		{
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}
			//		//////////////////////////////////////////////////////////////////////////

			//		unsigned char *navData = 0;
			//		int navDataSize = 0;
			//		if (!dtCreateNavMeshTileData(
			//			build.PolyMesh.verts, 
			//			build.PolyMesh.nverts,
			//			build.PolyMesh.polys,
			//			build.PolyMesh.npolys,
			//			build.PolyMesh.nvp,
			//			build.PolyMeshDetail.meshes,
			//			build.PolyMeshDetail.verts,
			//			build.PolyMeshDetail.nverts,
			//			build.PolyMeshDetail.tris,
			//			build.PolyMeshDetail.ntris,
			//			build.Chf.bmin, 
			//			build.Chf.bmax,
			//			tileCfg.cs, 
			//			tileCfg.ch, 
			//			ts,
			//			tileCfg.walkableClimb, 
			//			&navData, 
			//			&navDataSize))
			//		{
			//			if (rcGetLog())
			//				rcGetLog()->log(RC_LOG_ERROR, "Could not build Detour tiles.");
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}

			//		if(!DetourTiledNavmesh.addTileAt(x,y,navData,navDataSize,true))
			//		{
			//			if (rcGetLog())
			//				rcGetLog()->log(RC_LOG_ERROR, "addTileAt failed.");
			//			gFloodStatus = Process_FinishedNavMesh;
			//			break;
			//		}

			//		++numBuilt;
			//		if(numBuilt > buildTiles)
			//		{
			//			gFloodStatus = Process_FinishedNavMesh;
			//			return Function_InProgress;
			//		}
			//	}
			//}

			gFloodStatus = Process_FinishedNavMesh;
			break;
		}
	case Process_FinishedNavMesh:
		{
			//gDDraw->ClearCache();
			gFloodStatus = Process_FloodFinished;
			break;
		}
		/*case Process_FinishedNavMesh:
		{
		gDDraw->ClearCache();

		RecastOpenList.clear();
		RecastSolidList.clear();

		gFloodStatus = Process_Init;
		return Function_Finished;
		break;
		}*/
	}
	return Function_InProgress;
}

void PathPlannerRecast::AddFloodSeed(const Vector3f &_vec)
{
	RecastNode n = { _vec };
	RecastOpenList.push_back(n);
	//EngineFuncs::ConsoleMessage("Added Flood Fill Start");
}

void PathPlannerRecast::AddFloodEntityBounds(const AABB &_bnds)
{
	FloodEntityBounds.push_back(_bnds);
	//EngineFuncs::ConsoleMessage("Added Flood Entity Bounds");
}

void PathPlannerRecast::FloodFill()
{
	if(IGameManager::GetInstance()->RemoveUpdateFunction("Recast_FloodFill"))
		return;

	gFloodStatus = Process_Init;

	FunctorPtr f(new ObjFunctor<PathPlannerRecast>(this, &PathPlannerRecast::Process_FloodFill));
	IGameManager::GetInstance()->AddUpdateFunction("Recast_FloodFill", f);
}

void PathPlannerRecast::BuildNav()
{
	gFloodStatus = Process_BuildNavMesh;
}

//bool PathPlannerRecast::DoesSectorAlreadyExist(const NavSector &_ns)
//{
//	Vector3f vAvg = Utils::AveragePoint(_ns.m_Boundary);
//
//	for(obuint32 s = 0; s < m_NavSectors.size(); ++s)
//	{
//		Vector3f vSecAvg = Utils::AveragePoint(m_NavSectors[s].m_Boundary);
//		if(Length(vAvg, vSecAvg) < Mathf::EPSILON)
//			return true;
//	}
//	return false;
//}

//////////////////////////////////////////////////////////////////////////

bool PathPlannerRecast::GetNavInfo(const Vector3f &pos,obint32 &_id,String &_name)
{

	return false;
}

void PathPlannerRecast::AddEntityConnection(const Event_EntityConnection &_conn)
{

}

void PathPlannerRecast::RemoveEntityConnection(GameEntity _ent)
{
}

Vector3f PathPlannerRecast::GetDisplayPosition(const Vector3f &_pos)
{
	return _pos;
}

void PathPlannerRecast::_BenchmarkPathFinder(const StringVector &_args)
{
	if(!DetourNavmesh)
		return;

	EngineFuncs::ConsoleMessage("-= Recast PathFind Benchmark =-");

	/*dtQueryFilter query;

	enum { MaxPolys=4096};
	dtPolyRef polys[MaxPolys];
	const float extents[3] = { 9999999.f,9999999.f,9999999.f };
	const int NumPolys = DetourNavmesh->queryPolygons(Vector3f::ZERO, extents, &query, polys, MaxPolys);
	const obint32 NumPaths = NumPolys * NumPolys;

	Timer tme;
	tme.Reset();
	for(obint32 w1 = 0; w1 < NumPolys; ++w1)
	{
		for(obint32 w2 = 0; w2 < NumPolys; ++w2)
		{
			const dtMeshTile *tile0 = DetourNavmesh->getTileByRef(polys[w1],0);
			const dtMeshTile *tile1 = DetourNavmesh->getTileByRef(polys[w2],0);
			const dtPoly * poly0 = DetourNavmesh->getPolyByRef(polys[w1]);
			const dtPoly * poly1 = DetourNavmesh->getPolyByRef(polys[w2]);

			float v0[3] = {};
			float v1[3] = {};

			calcPolyCenter(v0,poly0->verts,poly0->vertCount,tile0->header->verts);
			calcPolyCenter(v1,poly1->verts,poly1->vertCount,tile1->header->verts);

			PlanPathToGoal(NULL,
				Vector3f(v0[0],v0[2],v0[1]),
				Vector3f(v1[0],v1[2],v1[1]),
				0);
		}
	}
	double dTimeTaken = tme.GetElapsedSeconds();
	EngineFuncs::ConsoleMessage("generated %d paths in %f seconds: %f paths/sec", 
		NumPaths, dTimeTaken, dTimeTaken != 0.0f ? (float)NumPaths / dTimeTaken : 0.0f);*/
}

void PathPlannerRecast::_BenchmarkGetNavPoint(const StringVector &_args)
{
	obuint32 iNumIterations = 1;
	if(_args.size() > 1)
	{
		iNumIterations = atoi(_args[1].c_str());
		if(iNumIterations <= 0)
			iNumIterations = 1;
	}

	EngineFuncs::ConsoleMessage("-= Recast GetNavPoint Benchmark  =-");

	if(CurrentNavMeshType == NavMeshStatic)
	{
		/*double dTimeTaken = 0.0f;
		const obuint32 NumPolys = (obuint32)DetourNavmesh.getPolyCount();
		Timer tme;

		obuint32 iHits = 0, iMisses = 0;
		tme.Reset();
		for(obuint32 i = 0; i < iNumIterations; ++i)
		{
		for(obuint32 w1 = 0; w1 < NumPolys; ++w1)
		{
		const dtStatPoly * poly0 = DetourNavmesh.getPoly(w1);
		float v0[3] = {};
		DetourNavmesh.getCenter(v0,poly0);

		float extents[3] = { 512.f,512.f,512.f };
		const dtStatPolyRef polyRef = DetourNavmesh.findNearestPoly(v0,extents);
		if(polyRef)
		++iHits;
		else
		++iMisses;
		}
		}

		dTimeTaken = tme.GetElapsedSeconds();

		EngineFuncs::ConsoleMessage("findNearestPoly() %d calls, %d hits, %d misses : avg %f per second", 
		NumPolys * iNumIterations, 
		iHits, 
		iMisses, 
		dTimeTaken != 0.0f ? ((float)(NumPolys * iNumIterations) / dTimeTaken) : 0.0f);	*/
	}
	else if(CurrentNavMeshType == NavMeshTiled)
	{
		// TODO
	}
}

//////////////////////////////////////////////////////////////////////////

void PathPlannerRecast::EntityCreated(const EntityInstance &ei)
{
	if(ei.m_EntityCategory.CheckFlag(ENT_CAT_OBSTACLE))
	{
		int freeSlot = -1;

		for(int i = 0; i < MaxObstacles; ++i)
		{
			if(freeSlot == -1 && !gNavObstacles[i].Entity.IsValid())
			{
				freeSlot = i;
			}

			if(gNavObstacles[i].Entity == ei.m_Entity)
			{
				return;
			}
		}

		if(freeSlot != -1)
		{
			gNavObstacles[freeSlot].State = NavObstacles::OBS_PENDING;
			gNavObstacles[freeSlot].Entity = ei.m_Entity;
		}
		else
		{
			OBASSERT(0,"NO FREE OBSTACLE SLOTS");
		}
	}
}

void PathPlannerRecast::EntityDeleted(const EntityInstance &ei)
{
	for(int i = 0; i < MaxObstacles; ++i)
	{
		if(gNavObstacles[i].Entity == ei.m_Entity)
		{
			gNavObstacles[i].Free();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void PathPlannerRecast::OverlayRender(RenderOverlay *overlay, const ReferencePoint &viewer)
{
	AABB voxel;
	voxel.m_Mins[0] = -(RecastCfg.cs * 0.5f);
	voxel.m_Mins[1] = -(RecastCfg.cs * 0.5f);
	voxel.m_Mins[2] = -(RecastCfg.cs * 0.5f);
	voxel.m_Maxs[0] =  (RecastCfg.cs * 0.5f);
	voxel.m_Maxs[1] =  (RecastCfg.cs * 0.5f);
	voxel.m_Maxs[2] =  (RecastCfg.cs * 0.5f);

	//////////////////////////////////////////////////////////////////////////
	// render explored nodes for visualization
	for(RecastNodeList::iterator it = RecastExploredList.begin();
		it != RecastExploredList.end();
		++it)
	{
		AABB voxelWorld = voxel;
		voxelWorld.Translate((*it).Pos);
		overlay->SetColor(COLOR::GREEN);
		overlay->DrawAABB(voxelWorld);
	}
	RecastExploredList.clear();
	//////////////////////////////////////////////////////////////////////////
	if(RecastOptions.DrawSolidNodes)
	{
		for(RecastNodeList::iterator it = RecastSolidList.begin();
			it != RecastSolidList.end();
			++it)
		{
			AABB voxelWorld = voxel;
			voxelWorld.Translate((*it).Pos);
			overlay->SetColor(COLOR::RED);
			overlay->DrawAABB(voxelWorld);
		}
	}
	//////////////////////////////////////////////////////////////////////////
	if ( m_PlannerFlags.CheckFlag(NAV_VIEW) )
	{
		overlay->PushMatrix();
		overlay->Translate(Vector3f(0,0,RecastOptions.DrawOffset));

		//gDDraw->StartFrame();

		// attempt to render from the cache, if implemented.
		//if(!gDDraw->RenderCache())
		{
			//gDDraw->StartCache();

			switch(RecastOptions.RenderMode)
			{
			case RecastOptions_t::DRAWMODE_NAVMESH:
				if ( DetourNavmesh )
					duDebugDrawNavMesh(gDDraw,*DetourNavmesh,DU_DRAWNAVMESH_OFFMESHCONS);
				break;
			case RecastOptions_t::DRAWMODE_NAVMESH_BVTREE:
				if ( DetourNavmesh )
					duDebugDrawNavMeshBVTree(gDDraw,*DetourNavmesh);
				break;
			case RecastOptions_t::DRAWMODE_VOXELS:
				if ( FloodHeightField )
					duDebugDrawHeightfieldSolid(gDDraw,*FloodHeightField);
				break;
			case RecastOptions_t::DRAWMODE_VOXELS_WALKABLE:
				if ( FloodHeightField )
					duDebugDrawHeightfieldWalkable(gDDraw,*FloodHeightField);
				break;
			case RecastOptions_t::DRAWMODE_COMPACT:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawCompactHeightfieldSolid(gDDraw,*it->Chf);
				}		
				break;
			case RecastOptions_t::DRAWMODE_COMPACT_DISTANCE:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawCompactHeightfieldDistance(gDDraw,*it->Chf);
				}
				break;
			case RecastOptions_t::DRAWMODE_COMPACT_REGIONS:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawCompactHeightfieldRegions(gDDraw,*it->Chf);
				}
				break;
			case RecastOptions_t::DRAWMODE_REGION_CONNECTIONS:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawRegionConnections(gDDraw,*it->Contour);
				}
				break;
			case RecastOptions_t::DRAWMODE_RAW_CONTOURS:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawRawContours(gDDraw,*it->Contour);
				}
				break;
			case RecastOptions_t::DRAWMODE_BOTH_CONTOURS:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawRawContours(gDDraw,*it->Contour);
					duDebugDrawContours(gDDraw,*it->Contour);
				}
				break;
			case RecastOptions_t::DRAWMODE_CONTOURS:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawContours(gDDraw,*it->Contour);
				}
				break;
			case RecastOptions_t::DRAWMODE_POLYMESH:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawPolyMesh(gDDraw,*it->PolyMesh);
				}
				break;
			case RecastOptions_t::DRAWMODE_POLYMESH_DETAIL:
				for(BuildDataList::iterator it = BuildData.begin();
					it != BuildData.end();
					++it)
				{
					duDebugDrawPolyMeshDetail(gDDraw,*it->PolyMeshDetail);
				}
				break;
			default:
				break;
			}

			/*overlay->SetColor(COLOR::MAGENTA);
			for(int i = 0; i < LinkData.size(); ++i)
			{
			overlay->DrawLine(LinkData[i].start,LinkData[i].end);
			}*/
			//gDDraw->EndCache();
		}
		overlay->PopMatrix();
	
		for(obuint32 i = 0; i < ladders.size(); ++i)
		{
			ladders[i].Render(overlay);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Highlight obstacles.
	for(int i = 0; i < MaxObstacles; ++i)
	{
		if(gNavObstacles[i].Entity.IsValid())
		{
			obColor color = COLOR::CYAN;
			switch(gNavObstacles[i].State)
			{
			case NavObstacles::OBS_NONE:
				color = COLOR::RED;
				break;
			case NavObstacles::OBS_PENDING:
				color = COLOR::YELLOW;
				break;
			case NavObstacles::OBS_ADDING:
				color = COLOR::MAGENTA;
				break;
			case NavObstacles::OBS_ADDED:
				color = COLOR::GREEN;
				break;
			case NavObstacles::OBS_REMOVING:
				color = COLOR::ORANGE;
				break;
			}

			AABB localbounds;
			EngineFuncs::EntityLocalAABB(gNavObstacles[i].Entity,localbounds);

			Vector3f pos, fwd, right, up;
			EngineFuncs::EntityPosition(gNavObstacles[i].Entity,pos);
			EngineFuncs::EntityOrientation(gNavObstacles[i].Entity,fwd, right, up);

			static float offsetZ = 0.f;
			Vector3f st = pos + up * offsetZ;

			static bool matrixmode = false;
			static bool renderbox = false;
			if(renderbox)
			{
				gOverlay->SetColor(color);
				gOverlay->DrawAABB(localbounds,pos,Matrix3f(right,fwd,up,matrixmode));
			}			

			gOverlay->SetColor(COLOR::BLUE);
			//gOverlay->DrawLine(st,st+fwd*128.f);
			gOverlay->DrawLine(st+fwd*localbounds.m_Mins[0],st+fwd*localbounds.m_Maxs[0]);
			gOverlay->SetColor(COLOR::RED);
			//gOverlay->DrawLine(st,st+right*128.f);
			gOverlay->DrawLine(st+right*localbounds.m_Mins[1],st+right*localbounds.m_Maxs[1]);
			gOverlay->SetColor(COLOR::GREEN);
			//gOverlay->DrawLine(st,st+up*128.f);
			gOverlay->DrawLine(st+up*localbounds.m_Mins[2],st+up*localbounds.m_Maxs[2]);
		}
	}
}

#ifdef ENABLE_DEBUG_WINDOW
class RecastActionListener : public gcn::ActionListener
{
	void UpdateRecastCfg()
	{
		gcn::contrib::PropertySheet *ps = static_cast<gcn::contrib::PropertySheet*>(DW.Nav.mAdjCtr->findWidgetById("prop_sheet"));
		CurrentNavMeshType = (NavMeshType)static_cast<gcn::DropDown*>(ps->findWidgetById("navtype"))->getSelected();

		RecastCfg.cs = static_cast<gcn::Slider*>(ps->findWidgetById("vox_size"))->getValue();
		RecastCfg.ch = static_cast<gcn::Slider*>(ps->findWidgetById("vox_height"))->getValue();
		RecastCfg.walkableSlopeAngle = static_cast<gcn::Slider*>(ps->findWidgetById("walk_slope"))->getValue();
		RecastCfg.maxEdgeLen = static_cast<gcn::Slider*>(ps->findWidgetById("max_edgelen"))->getValueInt();
		RecastCfg.maxSimplificationError = static_cast<gcn::Slider*>(ps->findWidgetById("simpl_err"))->getValue();
		RecastCfg.minRegionSize = static_cast<gcn::Slider*>(ps->findWidgetById("min_rgnsize"))->getValueInt();
		RecastCfg.mergeRegionSize = static_cast<gcn::Slider*>(ps->findWidgetById("merge_rgnsize"))->getValueInt();
		RecastCfg.maxVertsPerPoly = static_cast<gcn::Slider*>(ps->findWidgetById("max_polyverts"))->getValueInt();
		RecastCfg.borderSize = static_cast<gcn::Slider*>(ps->findWidgetById("bordersize"))->getValueInt();
		RecastCfg.detailSampleDist = static_cast<gcn::Slider*>(ps->findWidgetById("det_sample_dist"))->getValue();
		RecastCfg.detailSampleMaxError = static_cast<gcn::Slider*>(ps->findWidgetById("det_sample_err"))->getValue();

		AgentHeight = (static_cast<gcn::Slider*>(ps->findWidgetById("walk_height"))->getValue());
		AgentMaxClimb = (static_cast<gcn::Slider*>(ps->findWidgetById("walk_climb"))->getValue());
		AgentRadius = (static_cast<gcn::Slider*>(ps->findWidgetById("walk_radius"))->getValue());

		RecastCfg.walkableHeight = (int)ceilf(AgentHeight / RecastCfg.ch);
		RecastCfg.walkableClimb = (int)floorf(AgentMaxClimb / RecastCfg.ch);
		RecastCfg.walkableRadius = (int)ceilf(AgentRadius / RecastCfg.cs);
	}

	void action(const gcn::ActionEvent& actionEvent)
	{
		UpdateRecastCfg();
		if(actionEvent.getId()=="build_floodfill")
		{
			gRecastPlanner->FloodFill();
		}
		else if(actionEvent.getId()=="build_recast")
		{
			gRecastPlanner->BuildNav();
		}
		else if(actionEvent.getId()=="add_seed")
		{
			Vector3f vPt;
			if(Utils::GetLocalAimPoint(vPt))
				gRecastPlanner->AddFloodSeed(vPt);
		}
		else if(actionEvent.getId()=="drawOptions")
		{
			//gDDraw->ClearCache();
		}
	}
} BuildListenerRecast;

class RecastNavType : public gcn::ListModel
{
public:
	int getNumberOfElements() { return NavMeshNum; }
	std::string getElementAt(int i, int column)
	{
		switch(i)
		{
		case NavMeshStatic:
			return "NavMesh Static";
		case NavMeshTiled:
			return "NavMesh Tiled";
		default:
			break;
		}
		return "";
	}
} RecastNavTypes;

class RecastDrawOptions : public gcn::ListModel
{
public:
	int getNumberOfElements() { return RecastOptions_t::MAX_DRAWMODE; }
	std::string getElementAt(int i, int column)
	{
		switch(i)
		{
		case RecastOptions_t::DRAWMODE_NAVMESH:
			return "NavMesh";
		case RecastOptions_t::DRAWMODE_NAVMESH_BVTREE:
			return "NavMesh BV Tree";
		case RecastOptions_t::DRAWMODE_VOXELS:
			return "Voxels";
		case RecastOptions_t::DRAWMODE_VOXELS_WALKABLE:
			return "Voxels Walkable";
		case RecastOptions_t::DRAWMODE_COMPACT:
			return "Compact Heightfield";
		case RecastOptions_t::DRAWMODE_COMPACT_DISTANCE:
			return "Compact Heightfield Distance";
		case RecastOptions_t::DRAWMODE_COMPACT_REGIONS:
			return "Compact Regions";
		case RecastOptions_t::DRAWMODE_REGION_CONNECTIONS:
			return "Region Connections";
		case RecastOptions_t::DRAWMODE_RAW_CONTOURS:
			return "Contours - Raw";
		case RecastOptions_t::DRAWMODE_BOTH_CONTOURS:
			return "Contours - Both";
		case RecastOptions_t::DRAWMODE_CONTOURS:
			return "Contours";
		case RecastOptions_t::DRAWMODE_POLYMESH:
			return "PolyMesh";
		case RecastOptions_t::DRAWMODE_POLYMESH_DETAIL:
			return "PolyMesh Detail";
		default:
			break;
		}
		return "";
	}
} DrawOptionsRecast;

void PathPlannerRecast::CreateGui()
{
	DW.Nav.mAdjCtr->clear(true);
	gcn::contrib::PropertySheet *propsheet = new gcn::contrib::PropertySheet;
	propsheet->setId("prop_sheet");
	DW.Nav.mAdjCtr->add(propsheet);

	gcn::TextBox *stats = new gcn::TextBox;
	stats->setEnabled(false);
	stats->setId("stats");
	propsheet->addProperty("Stats",stats);

	gcn::DropDown *navMeshType = new gcn::DropDown(&RecastNavTypes);
	navMeshType->getScrollArea()->setDimension(navMeshType->getListBox()->getDimension());
	navMeshType->setWidth(navMeshType->getListBox()->getWidth());
	navMeshType->setId("navtype");
	propsheet->addProperty("NavMesh Type",navMeshType);	

	gcn::DropDown *drawOptions = new gcn::DropDown(&DrawOptionsRecast);
	drawOptions->getScrollArea()->setDimension(drawOptions->getListBox()->getDimension());
	drawOptions->setWidth(drawOptions->getListBox()->getWidth());
	drawOptions->setActionEventId("drawOptions");
	drawOptions->setId("drawOptions");
	drawOptions->addActionListener(&BuildListenerRecast);
	propsheet->addProperty("Render",drawOptions);

	gcn::Slider *RenderOffset = new gcn::Slider(0.f, 200.f);
	RenderOffset->setId("renderoff");
	RenderOffset->setValue(RecastOptions.DrawOffset);
	propsheet->addProperty("Render Offset",RenderOffset);

	gcn::Slider *RenderTimeShare = new gcn::Slider(0.01f, 100.f);
	RenderTimeShare->setId("timeshare");
	RenderTimeShare->setValue(RecastOptions.TimeShare);
	propsheet->addProperty("Render TimeShare",RenderTimeShare);

	gcn::Slider *VoxSize = new gcn::Slider(0.f, 64.f);
	VoxSize->setId("vox_size");
	VoxSize->setValue(RecastCfg.cs);
	propsheet->addProperty("Voxel Size",VoxSize);

	gcn::Slider *VoxHeight = new gcn::Slider(0.f, 64.f);
	VoxHeight->setId("vox_height");
	VoxHeight->setValue(RecastCfg.ch);
	propsheet->addProperty("Voxel Height",VoxHeight);

	gcn::Slider *WalkSlope = new gcn::Slider(0.f, 90.f);
	WalkSlope->setId("walk_slope");
	WalkSlope->setValue(RecastCfg.walkableSlopeAngle);
	propsheet->addProperty("Walk Slope",WalkSlope);

	gcn::Slider *WalkHeight = new gcn::Slider(0.f, 128.f);
	WalkHeight->setId("walk_height");
	WalkHeight->setValue(AgentHeight);
	propsheet->addProperty("Walk Height",WalkHeight);

	gcn::Slider *WalkClimb = new gcn::Slider(0.f, 100.f);
	WalkClimb->setId("walk_climb");
	WalkClimb->setValue(AgentMaxClimb);
	propsheet->addProperty("Walk Climb",WalkClimb);

	gcn::Slider *WalkRadius = new gcn::Slider(0.f, 100.f);
	WalkRadius->setId("walk_radius");
	WalkRadius->setValue(AgentRadius);
	propsheet->addProperty("Walk Radius",WalkRadius);

	gcn::Slider *MaxEdgeLen = new gcn::Slider(0.f, 100.f);
	MaxEdgeLen->setId("max_edgelen");
	MaxEdgeLen->setValue(RecastCfg.maxEdgeLen);
	propsheet->addProperty("Max Edge Length",MaxEdgeLen);

	gcn::Slider *MeshSimplErr = new gcn::Slider(0.f, 100.f);
	MeshSimplErr->setId("simpl_err");
	MeshSimplErr->setValue(RecastCfg.maxSimplificationError);
	propsheet->addProperty("Mesh Simpl. Err",MeshSimplErr);

	gcn::Slider *MinRgnSize = new gcn::Slider(0.f, 100.f);
	MinRgnSize->setId("min_rgnsize");
	MinRgnSize->setValue(RecastCfg.minRegionSize);
	propsheet->addProperty("Min Region Size",MinRgnSize);

	gcn::Slider *MergeRgnSize = new gcn::Slider(0.f, 100.f);
	MergeRgnSize->setId("merge_rgnsize");
	MergeRgnSize->setValue(RecastCfg.mergeRegionSize);
	propsheet->addProperty("Merge Region Size",MergeRgnSize);

	gcn::Slider *MaxPolyVerts = new gcn::Slider(0.f, (float)DT_VERTS_PER_POLYGON);
	MaxPolyVerts->setId("max_polyverts");
	MaxPolyVerts->setValue(RecastCfg.maxVertsPerPoly);
	propsheet->addProperty("Max PolyVerts",MaxPolyVerts);

	gcn::Slider *BorderSize = new gcn::Slider(0.f, 200.f);
	BorderSize->setId("bordersize");
	BorderSize->setValue(RecastCfg.borderSize);
	propsheet->addProperty("BorderSize",BorderSize);

	gcn::Slider *DetailSampleDist = new gcn::Slider(0.f, 200.f);
	DetailSampleDist->setId("det_sample_dist");
	DetailSampleDist->setValue(RecastCfg.detailSampleDist);
	propsheet->addProperty("Detail Sample Dist",DetailSampleDist);

	gcn::Slider *DetailSampleErr = new gcn::Slider(0.f, 200.f);
	DetailSampleErr->setId("det_sample_err");
	DetailSampleErr->setValue(RecastCfg.detailSampleMaxError);
	propsheet->addProperty("Detail Sample Err",DetailSampleErr);

	{
		gcn::contrib::AdjustingContainer *options = new gcn::contrib::AdjustingContainer;
		options->setId("buttons");
		options->setNumberOfColumns(3);

		gcn::Button *FFBtn = new gcn::Button("FloodFill");
		FFBtn->setActionEventId("build_floodfill");
		FFBtn->addActionListener(&BuildListenerRecast);
		options->add(FFBtn);

		gcn::Button *BuildBtn = new gcn::Button("Build");
		BuildBtn->setId("build");
		BuildBtn->setActionEventId("build_recast");
		BuildBtn->addActionListener(&BuildListenerRecast);
		options->add(BuildBtn);

		gcn::Button *SeedBtn = new gcn::Button("Add Seed");
		SeedBtn->setActionEventId("add_seed");
		SeedBtn->addActionListener(&BuildListenerRecast);
		options->add(SeedBtn);

		propsheet->addProperty("Functions",options);
	}

	{
		gcn::contrib::AdjustingContainer *options = new gcn::contrib::AdjustingContainer;
		options->setNumberOfColumns(3);

		gcn::CheckBox *cb = NULL;
		cb = new gcn::CheckBox("Filter Ledges");
		cb->setId("filter_ledges");
		options->add(cb);

		cb = new gcn::CheckBox("Filter Low Height");
		cb->setId("filter_lowheight");
		options->add(cb);

		cb = new gcn::CheckBox("Climb Walls");
		cb->setId("climb_walls");
		options->add(cb);

		cb = new gcn::CheckBox("Draw Solids");
		cb->setSelected(false);
		cb->setId("draw_solids");
		options->add(cb);

		propsheet->addProperty("Options",options);	
	}
}

void PathPlannerRecast::UpdateGui()
{
	if(!DW.Nav.mAdjCtr)
		return;

	gcn::contrib::PropertySheet *ps = static_cast<gcn::contrib::PropertySheet*>(DW.Nav.mAdjCtr->findWidgetById("prop_sheet"));
	gcn::TextBox *stats = static_cast<gcn::TextBox*>(ps->findWidgetById("stats"));
	if(stats)
	{
		String str = va(
			"Explored Cells: %d\nBorder Cells: %d\nCells/Sec: %.2f\nOpen Nodes: %d\n",
			RecastStats.ExploredCells,
			RecastStats.BorderCells,
			(double)RecastStats.ExploredCells/(RecastStats.FloodTime+Mathf::EPSILON),
			RecastOpenList.size()
			);
		//str += va("Memory (Hf): %s\n",Utils::FormatByteString(FloodHeightField.getMemUsed()).c_str());

		/*int chfMem = 0;
		for(BuildDataList::iterator it = BuildData.begin();
		it != BuildData.end();
		++it)
		{
		chfMem += it->Chf.getMemUsed();
		}
		str += va("Memory (CHf): %s\n",Utils::FormatByteString(chfMem).c_str());

		if(CurrentNavMeshType == NavMeshStatic)
		{
		str += va("NavMesh Size: %s\n",Utils::FormatByteString(DetourNavmesh->getMemUsed()).c_str());
		}*/
		stats->setText(str);
	}

	gcn::Button *buildBtn = static_cast<gcn::Button*>(ps->findWidgetById("build"));
	if(buildBtn)
	{
		buildBtn->setEnabled(gFloodStatus == Process_FloodFinished);
	}

	// rendering stuff.
	RecastOptions_t::DrawMode OldRenderMode = RecastOptions.RenderMode;
	RecastOptions.RenderMode = 
		(RecastOptions_t::DrawMode)static_cast<gcn::DropDown*>(ps->findWidgetById("drawOptions"))->getSelected();
	if(OldRenderMode != RecastOptions.RenderMode)
	{
		//gDDraw->ClearCache();
	}

	RecastOptions.FilterLedges =
		static_cast<gcn::CheckBox*>(ps->findWidgetById("filter_ledges"))->isSelected();
	RecastOptions.FilterLowHeight =
		static_cast<gcn::CheckBox*>(ps->findWidgetById("filter_lowheight"))->isSelected();

	RecastOptions.ClimbWalls =
		static_cast<gcn::CheckBox*>(ps->findWidgetById("climb_walls"))->isSelected();
	RecastOptions.DrawSolidNodes =
		static_cast<gcn::CheckBox*>(ps->findWidgetById("draw_solids"))->isSelected();

	RecastOptions.DrawOffset = static_cast<gcn::Slider*>(ps->findWidgetById("renderoff"))->getValue();
	RecastOptions.TimeShare = static_cast<gcn::Slider*>(ps->findWidgetById("timeshare"))->getValue();
}

Vector3f FromWorld(const AABB &_world, const Vector3f &_wp);

void PathPlannerRecast::RenderToMapViewPort(gcn::Widget *widget, gcn::Graphics *graphics)
{
	using namespace gcn;

	Color colLinks(255,127,0);
	Color colBot(0,0,255);
	Color colBotPath(255,0,255);

#ifdef USE_TILED_NAVMESH
	// TODO
#else
	//// find the nav extents
	//AABB navextents;
	//for (int i = 0; i < DetourNavmesh.getPolyCount(); ++i)
	//{
	//	const dtStatPoly* p = DetourNavmesh.getPoly(i);
	//	unsigned short vi[3];
	//	for (int j = 2; j < (int)p->nv; ++j)
	//	{
	//		vi[0] = p->v[0];
	//		vi[1] = p->v[j-1];
	//		vi[2] = p->v[j];
	//		for (int k = 0; k < 3; ++k)
	//		{
	//			const float* v = DetourNavmesh.getVertex(vi[k]);

	//			Vector3f vec(v[0], v[2], v[1]);
	//			if(i==0 && j==2)
	//				navextents.Set(vec);
	//			else
	//				navextents.Expand(vec);
	//		}
	//	}
	//}
	//const int MapWidth = widget->getWidth();
	//const int MapHeight = widget->getHeight();
	//const float fDesiredAspectRatio = (float)MapWidth / (float)MapHeight;

	//float fCurrentAspect = navextents.GetAxisLength(0) / navextents.GetAxisLength(1);
	//if(fCurrentAspect < fDesiredAspectRatio)
	//{
	//	float fDesiredHeight = navextents.GetAxisLength(0) * fDesiredAspectRatio/fCurrentAspect;
	//	navextents.ExpandAxis(0, (fDesiredHeight-navextents.GetAxisLength(0)) * 0.5f);
	//}
	//else if(fCurrentAspect > fDesiredAspectRatio)
	//{
	//	float fDesiredHeight = navextents.GetAxisLength(1) * fCurrentAspect/fDesiredAspectRatio;
	//	navextents.ExpandAxis(1, (fDesiredHeight-navextents.GetAxisLength(1)) * 0.5f);
	//}

	//// Expand a small edge buffer
	//navextents.ExpandAxis(0, 256.f);
	//navextents.ExpandAxis(1, 256.f);

	//gcn::Color col = gcn::Color(0,196,255,64);
	//
	//for (int i = 0; i < DetourNavmesh.getPolyCount(); ++i)
	//{
	//	const dtStatPoly* p = DetourNavmesh.getPoly(i);

	//	int NumTriangles = 0;
	//	gcn::Graphics::Triangle Triangles[64];

	//	for (int j = 2; j < (int)p->nv; ++j)
	//	{
	//		Vector3f v0 = Vector3f(DetourNavmesh.getVertex(p->v[0]));
	//		Vector3f v1 = Vector3f(DetourNavmesh.getVertex(p->v[j-1]));
	//		Vector3f v2 = Vector3f(DetourNavmesh.getVertex(p->v[j]));
	//		Vector2f vP0 = FromWorld(navextents, ToRecast(v0)).As2d();
	//		Vector2f vP1 = FromWorld(navextents, ToRecast(v1)).As2d(); 
	//		Vector2f vP2 = FromWorld(navextents, ToRecast(v2)).As2d();
	//		//graphics->drawLine((int)vP1.x, (int)vP1.y, (int)vP2.x, (int)vP2.y);

	//		Triangles[NumTriangles].v0 = gcn::Graphics::Point((int)vP0.x, (int)vP0.y, col);
	//		Triangles[NumTriangles].v1 = gcn::Graphics::Point((int)vP1.x, (int)vP1.y, col);
	//		Triangles[NumTriangles].v2 = gcn::Graphics::Point((int)vP2.x, (int)vP2.y, col);
	//		NumTriangles++;
	//	}
	//	graphics->drawPolygon(Triangles,NumTriangles);
	//}

	//graphics->setColor(colLinks);
	//for (int i = 0; i < DetourNavmesh.getPolyCount(); ++i)
	//{
	//	const dtStatPoly* p = DetourNavmesh.getPoly(i);
	//	for(int l = 0; l < DT_STAT_LINKS_PER_POLYGON; ++l)
	//	{
	//		const dtStatPolyLink *link = DetourNavmesh.getLinkByRef(p->links[l]);
	//		if(link)
	//		{
	//			Vector2f linkStart = FromWorld(navextents, ToRecast(link->start)).As2d();
	//			Vector2f linkEnd = FromWorld(navextents, ToRecast(link->end)).As2d();
	//			graphics->drawLine((int)linkStart.x, (int)linkStart.y, (int)linkEnd.x, (int)linkEnd.y);
	//		}
	//	}
	//}


	//for(int i = 0; i < IGame::MAX_PLAYERS; ++i)
	//{
	//	ClientPtr c = IGameManager::GetInstance()->GetGame()->GetClientByIndex(i);
	//	if(c)
	//	{
	//		Vector3f vViewPortPos = FromWorld(navextents, c->GetPosition());
	//		Vector3f vViewPortFace = FromWorld(navextents, c->GetPosition() + c->GetFacingVector() * 50.f);

	//		graphics->setColor(colBot);
	//		graphics->drawLine((int)vViewPortPos.x, (int)vViewPortPos.y, (int)vViewPortFace.x, (int)vViewPortFace.y);

	//		//DrawBuffer::Add(graphics, Graphics::Line((int)vViewPortPos.x, (int)vViewPortPos.y, (int)vViewPortFace.x, (int)vViewPortFace.y, colBot));

	//		/*if(selectedPlayer && selectedPlayer==c)
	//			drawCircle(graphics, vViewPortPos, 3.f+Mathf::Sin(IGame::GetTimeSecs()*3.f), colSelectedBot);
	//		else
	//			drawCircle(graphics, vViewPortPos, 2, colBot);*/

	//		//if(m_DrawBotPaths->value())
	//		{
	//			using namespace AiState;
	//			FINDSTATE(follow, FollowPath, c->GetStateRoot());
	//			if(follow)
	//			{
	//				Vector3f vLastPosition = c->GetPosition();

	//				Path::PathPoint ppt;
	//				Path pth = follow->GetCurrentPath();

	//				while(pth.GetCurrentPt(ppt) && !pth.IsEndOfPath())
	//				{
	//					Vector3f vP1 = FromWorld(navextents, vLastPosition);
	//					Vector3f vP2 = FromWorld(navextents, ppt.m_Pt);

	//					graphics->setColor(colBotPath);
	//					graphics->drawLine((int)vP1.x, (int)vP1.y, (int)vP2.x, (int)vP2.y);

	//					//drawCircle(graphics, FromWorld(navextents, ppt.m_Pt), ppt.m_Radius*fRadiusScaler, colBotPath);

	//					vLastPosition =  ppt.m_Pt;
	//					pth.NextPt();
	//				}
	//			}
	//		}
	//	}
	//}
#endif
}
#endif

bool PathPlannerRecast::ladder_t::OverLaps(const ladder_t & other) const
{
	AABB myBounds(bottom);
	myBounds.Expand(top);
	myBounds.Expand(width);

	AABB otherBounds(other.bottom);
	otherBounds.Expand(other.top);
	otherBounds.Expand(other.width);

	return myBounds.Intersects(otherBounds);
}

void PathPlannerRecast::ladder_t::Render(RenderOverlay *overlay) const
{
	Vector3f midPt = top.MidPoint(bottom);
	Vector3f side = Normalize(top-bottom).Cross(normal);

	overlay->SetColor(COLOR::GREEN);
	overlay->DrawLine(top,bottom);	
	overlay->DrawLine(midPt+side*width*0.5f,midPt-side*width*0.5f);
}
