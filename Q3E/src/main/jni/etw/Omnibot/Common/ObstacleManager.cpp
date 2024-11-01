#include "PrecompCommon.h"
#include "ObstacleManager.h"

ObstacleManager obstacleManager;

ObstacleManager::obstacle_t::obstacle_t() {
	entity.Reset();
	bounds = Box3f(Vector3f::ZERO,Vector3f::UNIT_X,Vector3f::UNIT_Y,Vector3f::UNIT_Z,0.f,0.f,0.f);
	obstacleGroup = 0;
}

void ObstacleManager::obstacle_t::UpdateObstacle() {
	EngineFuncs::EntityWorldOBB(entity,bounds);
	entClass = g_EngineFuncs->GetEntityClass(entity);
}

ObstacleManager::obstacleGroup_t::obstacleGroup_t() {
	groupNum = 0;
}

ObstacleManager::ObstacleManager()
	: numObstacles( 0 ) {
}

ObstacleManager::~ObstacleManager() {
}

void ObstacleManager::AddObstacle( GameEntity entity ) {
	if ( numObstacles < MaxObstacles ) {
		obstacles[numObstacles].entity = entity;
		obstacles[numObstacles].UpdateObstacle();		
		numObstacles++;
	}
}

void ObstacleManager::RemoveObstacle( GameEntity entity ) {
	for( int i = 0; i < numObstacles;  ) {
		if ( obstacles[i].entity == entity ) {
			RemoveObstacle( i );
		} else {
			++i;
		}
	}
}

void ObstacleManager::RemoveObstacle( int index ) {
	if ( numObstacles > 1 ) {
		obstacles[index] = obstacles[numObstacles-1];
	}
	--numObstacles;
}

void ObstacleManager::Update() {
	Prof(ObstacleManager_Update);
	for( int i = 0; i < numObstacles;  ) {
		if ( IGame::IsEntityValid( obstacles[i].entity ) ) {
			obstacles[i].UpdateObstacle();
			/*Utils::DrawLine(obstacles[i].bounds.Center,obstacles[i].bounds.Center+Vector3f::UNIT_Z*64.f,COLOR::CYAN,0.2f);
			Utils::OutlineOBB( obstacles[i].bounds, COLOR::GREEN, 0.2f );
			String name = EngineFuncs::EntityName(obstacles[i].entity,"unknown");
			Utils::PrintText(obstacles[i].bounds.Center,COLOR::GREEN,0.2f,name.c_str());*/
			++i;
		} else {
			RemoveObstacle( i );
		}
	}
}

void GetTangentPts(  const Vector3f & src, const Vector3f & pos, float radius, Vector3f pts[2] ) {
	Vector3f side = Vector3f::UNIT_Z.UnitCross( pos - src );
	pts[0] = pos + side * radius;
	pts[1] = pos - side * radius;
}

void ObstacleManager::ModifyForObstacles( Client * ai, Vector3f & nextPt ) {
	const Box3f aiBounds = ai->GetWorldBounds();

	const float aiRadius = aiBounds.Extent[0];

	static Vector3f offset = Vector3f::ZERO;
	Vector3f boxTo = nextPt + offset;
	Utils::OutlineOBB( aiBounds, COLOR::CYAN, 0.2f );

	Vector3f aiSideDir = (boxTo - aiBounds.Center).UnitCross( Vector3f::UNIT_Z );

	for( int i = 0; i < numObstacles; ++i ) {
		Box3f obsBounds = obstacles[i].bounds;
		obsBounds.Extent[0] += aiBounds.Extent[0];
		obsBounds.Extent[1] += aiBounds.Extent[1];
		obsBounds.Extent[2] += aiBounds.Extent[2];

		static float tMax = 1000.f;

		tMax = (boxTo - aiBounds.Center).Length();

		Vector3f aiVelocity = boxTo - aiBounds.Center;

		IntrBox3Box3f intr( aiBounds, obstacles[i].bounds );
		const bool hit = intr.Test( tMax, aiVelocity, Vector3f::ZERO );
		if ( hit ) {
			Utils::OutlineOBB( obstacles[i].bounds, COLOR::RED, 0.2f );

			const float avoidRadius = ai->GetAvoidRadius( obstacles[i].entClass );

			Plane3f plane(aiSideDir,aiBounds.Center);
			const Vector3f toPt = 
				obsBounds.Center + 
				aiSideDir * (plane.WhichSide( obsBounds.Center ) * avoidRadius+aiRadius);

			Utils::DrawLine(obsBounds.Center,obsBounds.Center+aiSideDir*32.f,COLOR::MAGENTA,0.2f);
			//if ( behind ) {
			//	Vector3f dir = aiBounds.Center - obsBounds.Center;
			//	nextPt = aiBounds.Center + dir * 512.f;
			//	Utils::DrawLine(aiBounds.Center,nextPt,COLOR::ORANGE,0.2f);
			//} else {
			//	Vector3f tang[2];
			//	GetTangentPts( aiBounds.Center, obsBounds.Center, avoidRadius+aiRadius, tang );

			//	//Utils::DrawLine(aiBounds.Center,tang[0],COLOR::ORANGE,0.2f);
			//	//Utils::DrawLine(aiBounds.Center,tang[1],COLOR::ORANGE,0.2f);

			//	/*const float dist0 = (tang[0] - nextPt).SquaredLength();
			//	const float dist1 = (tang[1] - nextPt).SquaredLength();
			//	const Vector3f toPt = (dist0 < dist1) ? tang[0] : tang[1];*/
			//	const Vector3f toPt = tang[0];
				Vector3f dir = toPt - aiBounds.Center;
				dir.z = 0.f;

				nextPt = aiBounds.Center + dir * 512.f;
				Utils::DrawLine(aiBounds.Center,nextPt,COLOR::ORANGE,0.2f);
			//}
			break;
		}
	}
}
