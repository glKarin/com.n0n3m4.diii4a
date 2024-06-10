// Duke_game.cpp
//

#include "Gamelib/Game_local.h"

/*
===================
dnGameLocal::Init
===================
*/
void dnGameLocal::Init(void) {
	idGameLocal::Init();

	// Create the various jobs we need on the game side.
	clientPhysicsJob = parallelJobManager->AllocJobList(JOBLIST_GAME_CLIENTPHYSICS, JOBLIST_PRIORITY_MEDIUM, 2, 0, NULL);
	parallelJobManager->RegisterJob((jobRun_t)ClientEntityJob_t, "G_ClientPhysics");

	//gameLocal.clientPhysicsJob->AddJobA((jobRun_t)dnGameLocal::ClientEntityJob_t, nullptr);
	//gameLocal.clientPhysicsJob->Submit();

	clientSpawnCount = INITIAL_SPAWN_COUNT;
	clientSpawnedEntities.Clear();
	memset(clientEntities, 0, sizeof(clientEntities));
	memset(clientSpawnIds, -1, sizeof(clientSpawnIds));

	InitGuis();
	InitGameRender();

	Printf("3drealms game initialized.\n");
	Printf("--------------------------------------\n");
}

/*
===================
idGameLocal::IsParentalLockEnabled
===================
*/
bool dnGameLocal::IsParentalLockEnabled(void)
{
	return g_ParentalLock.GetBool();
}

/*
===================
idGameLocal::TracePoint
===================
*/
bool dnGameLocal::TracePoint(const idEntity* ent, trace_t& results, const idVec3& start, const idVec3& end, int contentMask, const idEntity* passEntity) {	
	return clip.TracePoint(results, start, end, contentMask, passEntity);
}

/*
=====================
dnGameLocal::AimHitPredict
=====================
*/
DnAimHitType_t dnGameLocal::AimHitPredict(const idVec3& origin, const idVec3& dir, const idVec3& origFxOrigin, idEntity* owner, idEntity* additionalIgnore, float range) {
	idVec3		start;
	idVec3		end;
	int			contents;
	trace_t		tr;

	idEntity* ent = nullptr;

	// Calculate the end point of the trace
	start = origin;
	end = start + (dir.ToMat3() * idVec3(idMath::ClampFloat(0, 2048, range), 0, 0));

	//gameRenderWorld->DebugLine(colorWhite, start, end, 1000);

	contents = MASK_SHOT_RENDERMODEL | CONTENTS_WATER | CONTENTS_PROJECTILE;

	TracePoint(owner, tr, start, end, contents, additionalIgnore);

	if (tr.fraction >= 1.0f || (tr.c.material && tr.c.material->GetSurfaceFlags() & SURF_NOIMPACT)) {
		return AIM_HIT_NOTHING;
	}
	ent = entities[tr.c.entityNum];

	if (ent->fl.takedamage) {
		return AIM_HIT_AI;
	}

	return AIM_HIT_NOTHING;
}

/*
=====================
dnGameLocal::HitScan
=====================
*/
idEntity* dnGameLocal::HitScan(const char* damage_type, const idVec3& origin, const idVec3& dir, const idVec3& origFxOrigin, idEntity* owner, bool noFX, float damageScale, idEntity* additionalIgnore, int	areas[2], bool spawnDebris, float range, int push)
{
	idVec3		start;
	idVec3		end;
	int			contents;
	trace_t		tr;

	idEntity* ent = nullptr;

	// Calculate the end point of the trace
	start = origin;
	end = start + (dir.ToMat3() * idVec3(idMath::ClampFloat(0, 2048, range), 0, 0));

//	gameRenderWorld->DebugLine(colorWhite, start, end, 1000);

	contents = MASK_SHOT_RENDERMODEL | CONTENTS_WATER | CONTENTS_PROJECTILE;

	TracePoint(owner, tr, start, end, contents, additionalIgnore);

	if (tr.fraction >= 1.0f || (tr.c.material && tr.c.material->GetSurfaceFlags() & SURF_NOIMPACT)) {
		return nullptr;
	}

	ent = entities[tr.c.entityNum];

	if (!gameLocal.isClient) {
		if (ent->fl.takedamage)
		{
			ent->Damage(owner, owner, dir, damage_type, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE(tr.c.id));
		}
	}
#if 1
	if (!ent->fl.takedamage)
	{
		idMat3 mat = tr.c.normal.ToAngles().ToMat3();
		idEntityFx::StartFx("fx/debris", &tr.c.point, &mat, ent, true);
	}
	else
	{
		idAngles ang = tr.c.normal.ToAngles();
		idAngles ang2 = tr.c.normal.ToAngles();
		ang.pitch += 90.0f;
		//ang2.pitch -= 90.0f;
		idMat3 mat = ang.ToMat3();
		idMat3 mat2 = ang2.ToMat3();
		idEntityFx::StartFx("fx/bloodwound", &tr.c.point, &mat, ent, true);
		idEntityFx::StartFx("fx/bloodwound", &tr.c.point, &mat2, ent, true);
	}
#endif
	return ent;
}
