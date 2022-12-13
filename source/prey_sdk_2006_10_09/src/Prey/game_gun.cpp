
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//-----------------------------------------------------------------------
//
// hhGun
//
//-----------------------------------------------------------------------
const idEventDef EV_SetEnemy("setenemy", "e");
const idEventDef EV_FireGunAt("fireat", "e");

CLASS_DECLARATION(idEntity, hhGun)
	EVENT( EV_Activate,			hhGun::Event_Activate )
	EVENT( EV_SetEnemy,			hhGun::Event_SetEnemy )
	EVENT( EV_FireGunAt,		hhGun::Event_FireAt )
END_CLASS


float AngleBetweenVectors(const idVec3 &v1, const idVec3 &v2) {
	float dot = (v1 * v2) / ( v1.Length() * v2.Length() );
	return RAD2DEG(idMath::ACos(dot));
}

void hhGun::Spawn(void) {
	coneAngle = spawnArgs.GetFloat("coneAngle");
	burstCount = spawnArgs.GetInt("bursts");
	burstRate = ( int )( spawnArgs.GetFloat( "burstRate" ) * 1000.0f );
	fireRate = ( int )( spawnArgs.GetFloat( "fireRate" ) * 1000.0f );
	fireDeviation = ( int )( spawnArgs.GetFloat( "fireDeviation" ) * 1000.0f );
	targetOffset = spawnArgs.GetVector("targetOffset");
	targetRadius = spawnArgs.GetFloat("targetRadius");
	nextFireTime = 0;
	nextBurstTime = 0;
	nextEnemyTime = 0;
	firing = false;
	enemyRate = 200;

	GetPhysics()->SetContents(CONTENTS_SOLID);
	fl.takedamage = true;

	SetEnemy(NULL);
	if (spawnArgs.GetBool("enabled")) {
		BecomeActive(TH_THINK);
	}
}

void hhGun::Save(idSaveGame *savefile) const {
	enemy.Save(savefile);
	savefile->WriteInt( enemyRate );
	savefile->WriteInt( nextEnemyTime );
	savefile->WriteVec3( targetOffset );
	savefile->WriteFloat( targetRadius );
	savefile->WriteInt( fireRate );
	savefile->WriteInt( fireDeviation );
	savefile->WriteInt( nextFireTime );
	savefile->WriteInt( burstRate );
	savefile->WriteInt( burstCount );
	savefile->WriteInt( nextBurstTime );
	savefile->WriteInt( curBurst );
	savefile->WriteBool( firing );
	savefile->WriteFloat( coneAngle );
}

void hhGun::Restore( idRestoreGame *savefile ) {
	enemy.Restore(savefile);
	savefile->ReadInt( enemyRate );
	savefile->ReadInt( nextEnemyTime );
	savefile->ReadVec3( targetOffset );
	savefile->ReadFloat( targetRadius );
	savefile->ReadInt( fireRate );
	savefile->ReadInt( fireDeviation );
	savefile->ReadInt( nextFireTime );
	savefile->ReadInt( burstRate );
	savefile->ReadInt( burstCount );
	savefile->ReadInt( nextBurstTime );
	savefile->ReadInt( curBurst );
	savefile->ReadBool( firing );
	savefile->ReadFloat( coneAngle );
}

void hhGun::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location) {
	SetEnemy(NULL);
	fl.takedamage = false;
	BecomeInactive(TH_THINK);

	const char *killedModel = spawnArgs.GetString("model_killed", NULL);
	if (killedModel) {
		SetModel(killedModel);
		UpdateVisuals();
	}
	else {
		GetPhysics()->SetContents(0);
	}

	// Spawn gibs
	if (spawnArgs.FindKey("def_debrisspawner")) {
		hhUtils::SpawnDebrisMass(spawnArgs.GetString("def_debrisspawner"), this );
	}

	StartSound( "snd_explode", SND_CHANNEL_ANY );

	ActivateTargets( attacker );
}

void hhGun::SetEnemy(idEntity *ent) {
	if (ent && ent->IsType(hhPlayer::Type)) {
		hhPlayer *player = static_cast<hhPlayer *>(ent);
		enemy = player->InVehicle() ? player->GetVehicleInterface()->GetVehicle() : ent;
	}
	else {
		enemy = ent;
	}
}

void hhGun::FindEnemy() {
	if (enemy.IsValid() && enemy->GetHealth() > 0) {
		return;
	}
	if ( !gameLocal.InPlayerPVS( this ) ) {
		return;
	}

	idEntity *bestEnt = NULL;
	float bestDistSqr = idMath::INFINITY;
	float radiusSqr = targetRadius*targetRadius;
	idVec3 origin = GetPhysics()->GetOrigin();
	idMat3 axis = GetPhysics()->GetAxis();
	for ( int i = 0; i < MAX_CLIENTS ; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];

		if (ent) {
			idVec3 toEnt = ent->GetPhysics()->GetOrigin() + targetOffset - origin;
			float distSqr = toEnt.LengthSqr();
			if (distSqr < bestDistSqr && distSqr < radiusSqr &&
					AngleBetweenVectors(toEnt, axis[0]) < coneAngle) {
				bestDistSqr = distSqr;
				bestEnt = ent;
			}
		}
	}

	SetEnemy(bestEnt);
}

idMat3 hhGun::GetAimAxis() {
#if 0
	// Fast approximation
	const idDict *projectileDef = declManager->FindEntityDef( spawnArgs.GetString("def_projectile") );
	float projSpeed	= idProjectile::GetVelocity( projectileDef ).Length();
	idVec3 firePos = GetPhysics()->GetOrigin();
	idVec3 enemyPos = enemy->GetPhysics()->GetOrigin() + targetOffset;
	idVec3 enemyVel	= enemy->GetPhysics()->GetLinearVelocity();
	idVec3 toEnemy	= enemyPos - firePos;
	float enemyDist	= toEnemy.Length();
	float projTime	= enemyDist / projSpeed;
	idVec3 predictedEnemyPos = enemyPos + enemyVel * projTime;
	idVec3 aim = predictedEnemyPos - firePos;
	aim.Normalize();
	return aim.hhToMat3();
#else
/*	Projectile prediction:

Let:
	Pe(t) = position of enemy at time t		Ve = velocity of enemy (known)
	Pm(t) = position of missile at time t	Vm = velocity of missile (magnitude known)
	Pf    = position firing from

(1)		Pe(t)  = Pe(0) + Ve * t		Enemy position function

(2)		Aim(t) = Pe(t) - Pf			Aim function is vector from firing point to Pe(t)

		|Aim(t)|					distance / distance/sec -> sec
(3)		-------- = t
		  |Vm|

(4)		d = Pe(0) - Pf				delta vector from firing point to initial enemy position

(5)		s = |Vm|

	  Substituting and simplifying yields a quadratic function in terms of t, Ve, d, & s:
	  At² + Bt + C = 0
*/
	const idDict *projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_projectile") );
	if ( !projectileDef ) {
		gameLocal.Error( "Unknown def_projectile:  %s\n", spawnArgs.GetString("def_projectile") );
	}
	float projSpeed	= idProjectile::GetVelocity( projectileDef ).Length();
	idVec3 firePos = GetPhysics()->GetOrigin();
	idVec3 enemyPos = enemy->GetOrigin() + idVec3(0,0,32);
	idVec3 enemyVel	= enemy->GetPhysics()->GetLinearVelocity();
	idVec3 d = enemyPos - firePos;
	idVec3 v = enemyVel;
	float  s = projSpeed;

	float a = v.x*v.x + v.y*v.y + v.z*v.x - s*s;	// t2 term
	float b = 2 * (d.x*v.x + d.y*v.y + d.z*v.z);	// t term
	float c = d.x*d.x + d.y*d.y + d.z*d.z;

	// Use quadratic formula to solve for t
	float t1 = (-b + sqrt(b*b - 4*a*c) ) / (2*a);
	float t2 = (-b - sqrt(b*b - 4*a*c) ) / (2*a);
	float projTime = t1 > 0 ? t1 : t2;

	idVec3 predictedEnemyPos = enemyPos + enemyVel * projTime;
	idVec3 aim = predictedEnemyPos - firePos;
	aim.Normalize();
	return aim.hhToMat3();
#endif
}

bool hhGun::ValidEnemy() {
	return (enemy.IsValid()) ? enemy->GetHealth() > 0 : false;
}

void hhGun::Fire(idMat3 &axis) {
	if (health > 0) {
		hhUtils::LaunchProjectile(this, spawnArgs.GetString("def_projectile"), axis, GetOrigin());
		StartSound( "snd_fire", SND_CHANNEL_ANY );
	}
}

void hhGun::Think(void) {
	if (thinkFlags & TH_THINK) {

		// Temp: for placement
		if (spawnArgs.GetBool("showCone")) {
			float radius = targetRadius * tan(DEG2RAD(coneAngle));
			gameRenderWorld->DebugCone(colorGreen,
				GetPhysics()->GetOrigin(),
				GetPhysics()->GetAxis()[0] * targetRadius,
				0, radius);
		}

		if (!ValidEnemy() && gameLocal.time >= nextEnemyTime ) {
			FindEnemy();
			nextEnemyTime = gameLocal.time + enemyRate;
		}

		if (ValidEnemy()) {
			if (!firing && gameLocal.time >= nextFireTime ) {
				firing = true;
				curBurst = burstCount;
				nextFireTime = gameLocal.time + fireRate + gameLocal.random.CRandomFloat()*fireDeviation;
				nextBurstTime = gameLocal.time;
			}

			if (firing && gameLocal.time >= nextBurstTime ) {
				// Aim
				idMat3 aimAxis = GetAimAxis();

				// Burst if enemy is in cone
				if ( AngleBetweenVectors(aimAxis[0], GetPhysics()->GetAxis()[0]) < coneAngle ) {
					Fire(aimAxis);
					if (--curBurst <= 0) {
						firing = false;
					}
				}
				else {
					firing = false;
					SetEnemy(NULL);
				}

				nextBurstTime = gameLocal.time + burstRate;
			}
		}
	}
	idEntity::Think();
}

void hhGun::Event_Activate(idEntity *activator) {
	if (targets.Num() && targets[0].IsValid()) {
		Event_FireAt(targets[0].GetEntity());
		return;
	}

	if (thinkFlags & TH_THINK) {
		BecomeInactive(TH_THINK);
	}
	else {
		BecomeActive(TH_THINK);
	}
}

void hhGun::Event_SetEnemy(idEntity *newEnemy) {
	SetEnemy(newEnemy);
}

void hhGun::Event_FireAt(idEntity *victim) {
	idVec3 dir = victim->GetOrigin() - GetOrigin();
	dir.Normalize();
	Fire(dir.ToMat3());
}
