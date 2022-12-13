

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_AcidBlast("acidBlast");
const idEventDef EV_AcidDrip("<acidDrip>");
const idEventDef EV_DeathCloud("<deathCloud>");
const idEventDef EV_LaunchPod("launchPod");
const idEventDef EV_NewPod("<newPod>", "e");
const idEventDef EV_SpawnBlastDebris("<spawnBlastDebris>");
const idEventDef EV_ChargeEnemy("chargeEnemy");
const idEventDef EV_GrabEnemy("grabEnemy", NULL, 'f');
const idEventDef EV_BiteEnemy("biteEnemy");
const idEventDef EV_DirectMoveToPosition("directMoveToPosition", "v");
const idEventDef EV_BindUnfroze("<bindUnfroze>", "e");
const idEventDef EV_GrabCheck("grabCheck", "", 'd');
const idEventDef EV_MoveToGrabPosition("moveToGrabPosition");
const idEventDef EV_CheckRange("checkRange", "", 'd');
const idEventDef EV_EnemyRangeZ("enemyRangeZ", "", 'f');

CLASS_DECLARATION(hhMonsterAI, hhGasbagSimple)
	EVENT(EV_AcidBlast, hhGasbagSimple::Event_AcidBlast)
	EVENT(EV_AcidDrip, hhGasbagSimple::Event_AcidDrip)
	EVENT(EV_DeathCloud, hhGasbagSimple::Event_DeathCloud)
	EVENT(EV_LaunchPod, hhGasbagSimple::Event_LaunchPod)
	EVENT(EV_NewPod, hhGasbagSimple::Event_NewPod)
	EVENT(EV_SpawnBlastDebris, hhGasbagSimple::Event_SpawnBlastDebris)
	EVENT(EV_ChargeEnemy, hhGasbagSimple::Event_ChargeEnemy)
	EVENT(EV_GrabEnemy, hhGasbagSimple::Event_GrabEnemy)
	EVENT(EV_BiteEnemy, hhGasbagSimple::Event_BiteEnemy)
	EVENT(EV_DirectMoveToPosition, hhGasbagSimple::Event_DirectMoveToPosition)
	EVENT(EV_BindUnfroze, hhGasbagSimple::Event_BindUnfroze)
	EVENT(EV_GrabCheck, hhGasbagSimple::Event_GrabCheck)
	EVENT(EV_MoveToGrabPosition, hhGasbagSimple::Event_MoveToGrabPosition)
	EVENT(EV_CheckRange, hhGasbagSimple::Event_CheckRange)
	EVENT(EV_EnemyRangeZ, hhGasbagSimple::Event_EnemyRangeZ)
END_CLASS

static const idEventDef EV_Unfreeze( "<unfreeze>" );

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
//
// ~hhGasbagSimple()
//
hhGasbagSimple::~hhGasbagSimple(void) {
	// Make sure to unbind the player if we've grabbed him to prevent deleting him
	if (enemy.IsValid() && enemy->IsBoundTo(this) && enemy->IsType(hhPlayer::Type)) {
		enemy->Unbind();
		enemy->PostEventMS(&EV_Unfreeze, 0);
	}
}

//
// Spawn()
//
void hhGasbagSimple::Spawn(void) {
	Event_SetMoveType(MOVETYPE_FLY); // So we ignore gravity zones from the beginning
	podOffset = spawnArgs.GetVector("pod_offset", "0 0 64");
	podRange = spawnArgs.GetFloat("podRange", "250");
	dripCount = 0;
	nextWoundTime = 0;

	bindController = static_cast<hhBindController *>( gameLocal.SpawnClientObject(spawnArgs.GetString("def_bindController"), NULL) );
	HH_ASSERT( bindController.IsValid() );
	bindController->fl.networkSync = false;

	float yawLimit = spawnArgs.GetFloat("yawlimit", "180");
	const char *handName = spawnArgs.GetString("def_tractorhand");
	const char *animName = spawnArgs.GetString("boundanim");

	bindController->SetRiderParameters(animName, handName, yawLimit, 0.0f);
	bindController->Bind(this, true);
	bindController->SetOrigin(spawnArgs.GetVector("grab_offset"));
	bindController->fl.neverDormant = true;
}

//
// Ticker()
//
void hhGasbagSimple::Ticker(void) {
	for (int i = 0; i < podList.Num(); i++) {
		if (!podList[i].IsValid()) {
			podList.RemoveIndex(i);
			i--; // This index is gone, so don't increment
			continue;
		}
	}

	// Update the podcount
	AI_PODCOUNT = podList.Num();

	if (podList.Num() == 0) {
		BecomeInactive(TH_TICKER);
	}
}

//
// Save()
//
void hhGasbagSimple::Save(idSaveGame *savefile) const {
	savefile->WriteInt(dripCount);

	savefile->WriteInt(podList.Num());
	for (int i = 0; i < podList.Num(); i++) {
		podList[i].Save(savefile);
	}
	bindController.Save(savefile);
}

//
// Restore()
//
void hhGasbagSimple::Restore(idRestoreGame *savefile) {
	Spawn();
	savefile->ReadInt(dripCount);

	int num;
	savefile->ReadInt(num);
	podList.SetNum(num);
	for (int i = 0; i < num; i++) {
		podList[i].Restore(savefile);
	}
	bindController.Restore(savefile);

	nextWoundTime = 0;
}

//
// Event_AcidBlast()
//
void hhGasbagSimple::Event_AcidBlast(void) {
	if (health <= 0) { // Don't launch a pod if we're dead
		return;
	}

	if (!enemy.IsValid()) {
		return;
	}

	idEntity *target = enemy.GetEntity();
	idVec3 targetOrigin = target->GetOrigin();
	float len, nearest = podRange;

	// See if we have a pod near the enemy.  If we do, target it instead.
	idEntity *entList[100];
	int num = gameLocal.EntitiesWithinRadius(targetOrigin, podRange, entList, 100);
	for (int i = 0; i < num; i++) {
		if (!entList[i]->IsType(hhPod::Type)) {
			continue;
		}

		len = (entList[i]->GetOrigin() - targetOrigin).Length();

		// Always target the pod nearest to the enemy
		if (len < nearest) {
			target = entList[i];
			nearest = len;
		}
	}

	// Spawn a "muzzle flash"-like fx
	hhFxInfo fx;
	fx.SetEntity(this);
	fx.RemoveWhenDone(true);
	SpawnFxLocal(spawnArgs.GetString("fx_fire"), GetOrigin() + idVec3(0, 0, -400), enemy->GetPhysics()->GetGravityNormal().ToMat3(), &fx);

	// Spawn an acid blob in the player's general direction
	const idDict *projDef = gameLocal.FindEntityDefDict(spawnArgs.GetString("def_projectile"), false);
	LaunchProjectile(spawnArgs.GetString("acidbone", "LmbRt"), target, false, projDef); 

	PostEventMS(&EV_SpawnBlastDebris, 100 + gameLocal.random.RandomInt(100));

	dripCount = gameLocal.random.RandomInt(3) + 1;
	PostEventMS(&EV_AcidDrip, 200 + gameLocal.random.RandomInt(100)); // Drip acid after firing
}

//
// Event_SpawnBlastDebris()
//
void hhGasbagSimple::Event_SpawnBlastDebris(void) {
	idDict args;
	args.Clear();
	args.Set("origin", (GetPhysics()->GetOrigin() + spawnArgs.GetVector("blast_debris_offset", "0 0 128")).ToString());
	gameLocal.SpawnObject("debrisSpawner_gasbag_fire", &args);
}

//
// Event_AcidDrip()
//
void hhGasbagSimple::Event_AcidDrip(void) {
	assert(dripCount > 0);
	dripCount--;

	idVec3 target = GetPhysics()->GetOrigin();
	target.z -= 128;

	idVec3 rndVec = hhUtils::RandomVector() * gameLocal.random.RandomInt(20);
	const idDict *projDef = gameLocal.FindEntityDefDict(spawnArgs.GetString("def_drip_projectile", "projectile_acidspray_gasbag"), false);
	LaunchProjectileAtVec(spawnArgs.GetString("acidbone", "LmbRt"), target + rndVec, false, projDef); 

	if (dripCount > 0) {
		PostEventMS(&EV_AcidDrip, 200 + gameLocal.random.RandomInt(100)); // Fire again in 200-300ms
	}
}

//
// Event_DeathCloud()
//
void hhGasbagSimple::Event_DeathCloud(void) {
	const char *fx = spawnArgs.GetString("fx_death");
	if (!fx || !fx[0]) {
		return;
	}
	hhFxInfo fxInfo;

	fxInfo.RemoveWhenDone(true);
	BroadcastFxInfo(fx, GetOrigin(), GetAxis(), &fxInfo);
}

//
// Event_LaunchPod()
//
void hhGasbagSimple::Event_LaunchPod() {
	if (health <= 0) { // Don't launch a pod if we're dying
		return;
	}

	idVec3 target, origin, offset = vec3_origin;
	if (enemy.IsValid()) {
		// Target a random area around the enemy
		origin = enemy->GetEyePosition();
		if (gameLocal.random.RandomInt(100) > 30) { // Possibly target the enemy exactly
			offset.x = (gameLocal.random.RandomFloat() * 100.0f) - 50.0f;
			offset.y = (gameLocal.random.RandomFloat() * 100.0f) - 50.0f;
		}
	} else {
		// Target a random area beneath us
		origin = GetOrigin();
		offset.x = gameLocal.random.RandomFloat() * 10.0f;
		offset.y = gameLocal.random.RandomFloat() * 10.0f;
		offset.z = -10.0f;
	}
	target = origin + offset;

	const idDict *projDef = gameLocal.FindEntityDefDict(spawnArgs.GetString("def_pod_projectile", "projectile_gasbag_pod"), false);
	LaunchProjectileAtVec(spawnArgs.GetString("acidbone", "LmbRt"), target, false, projDef);
}

//
// Event_NewPod
//
void hhGasbagSimple::Event_NewPod(hhPod *pod) {
	HH_ASSERT(pod && pod->IsType(hhPod::Type));

	podList.Append(idEntityPtr<hhPod>(pod));

	// Update the podcount
	AI_PODCOUNT = podList.Num();

	BecomeActive(TH_TICKER);
}

//
// Killed
//
void hhGasbagSimple::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location) {
	bool wasDead = (AI_DEAD != 0);

	hhMonsterAI::Killed(inflictor, attacker, damage, dir, location);

	if (wasDead) {
		return;
	}

	CancelEvents(&EV_AcidDrip);
	CancelEvents(&EV_AcidBlast);
	CancelEvents(&EV_LaunchPod);
	dripCount = 0;

	// Release any grabbed enemy
	idEntity *rider;
	if ((rider = bindController->GetRider()) != NULL) {
		bindController->Detach();
		if (rider->IsType(hhPlayer::Type)) {
			rider->ProcessEvent(&EV_Unfreeze);
		}
	}

	int deathAnim = GetAnimator()->GetAnim("death");
	GetAnimator()->PlayAnim(ANIMCHANNEL_TORSO, deathAnim, gameLocal.time, 300.0f);
	int ttl = GetAnimator()->AnimLength(deathAnim);
	// Freeze
	physicsObj.EnableGravity(false);
	physicsObj.PutToRest();

	SetSkinByName( spawnArgs.GetString( "skin_death" ) );
	SetDeformation(DEFORMTYPE_DEATHEFFECT, gameLocal.time + 2500, 8000);	// starttime, duration
	PostEventSec( &EV_StartSound, 1.5f, "snd_acid", SND_CHANNEL_ANY, 1 );

	// Spawn gas cloud
	PostEventMS(&EV_DeathCloud, ttl);
	PostEventMS(&EV_Dispose, 0);
}

//
// LaunchProjectile
//
idProjectile *hhGasbagSimple::LaunchProjectileAtVec( const char *jointname, const idVec3 &target, bool clampToAttackCone, const idDict* desiredProjectileDef ) {	//HUMANHEAD mdc - added desiredProjectileDef for supporting multiple projs.
	idVec3				muzzle;
	idVec3				dir;
	idVec3				start;
	trace_t				tr;
	idBounds			projBounds;
	float				distance;
	const idClipModel	*projClip;
	float				attack_accuracy;
	float				attack_cone;
	float				projectile_spread;
	float				diff;
	float				angle;
	float				spin;
	idAngles			ang;
	int					num_projectiles;
	int					i;
	idMat3				axis;
	idVec3				tmp;
	idProjectile		*lastProjectile;

	//HUMANHEAD mdc - added to support multiple projectiles
	if( desiredProjectileDef ) {	//try to set our projectile to the desiredProjectile
		int projIndex = FindProjectileInfo( desiredProjectileDef );
		if( projIndex >= 0 ) {
			SetCurrentProjectile( projIndex );
		}
	}
	//HUMANHEAD END


	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	num_projectiles = spawnArgs.GetInt( "num_projectiles", "1" );

	GetMuzzle( jointname, muzzle, axis );

	if ( !projectile.GetEntity() ) {
		CreateProjectile( muzzle, axis[ 0 ] );
	}

	lastProjectile = projectile.GetEntity();

	tmp = target - muzzle;
	tmp.Normalize();
	axis = tmp.ToMat3();

	// rotate it because the cone points up by default
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = lastProjectile->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( axis );

	// check if the owner bounds is bigger than the projectile bounds
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( muzzle, viewAxis[ 0 ], distance ) ) {
			start = muzzle + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, muzzle, projClip, axis, MASK_SHOT_RENDERMODEL, this );
	muzzle = tr.endpos;

	// set aiming direction	
	//GetAimDir( muzzle, target, this, dir );
	dir = target - muzzle;
	ang = dir.ToAngles();

	// adjust his aim so it's not perfect.  uses sine based movement so the tracers appear less random in their spread.
	float t = MS2SEC( gameLocal.time + entityNumber * 497 );
	ang.pitch += idMath::Sin16( t * 5.1 ) * attack_accuracy;
	ang.yaw	+= idMath::Sin16( t * 6.7 ) * attack_accuracy;

	if ( clampToAttackCone ) {
		// clamp the attack direction to be within monster's attack cone so he doesn't do
		// things like throw the missile backwards if you're behind him
		diff = idMath::AngleDelta( ang.yaw, current_yaw );
		if ( diff > attack_cone ) {
			ang.yaw = current_yaw + attack_cone;
		} else if ( diff < -attack_cone ) {
			ang.yaw = current_yaw - attack_cone;
		}
	}

	axis = ang.ToMat3();

	float spreadRad = DEG2RAD( projectile_spread );
	for( i = 0; i < num_projectiles; i++ ) {
		// spread the projectiles out
		angle = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
		spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = axis[ 0 ] + axis[ 2 ] * ( angle * idMath::Sin( spin ) ) - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir.Normalize();

		// launch the projectile
		if ( !projectile.GetEntity() ) {
			CreateProjectile( muzzle, dir );
		}
		lastProjectile = projectile.GetEntity();
		lastProjectile->Launch( muzzle, dir, vec3_origin );
		projectile = NULL;
	}

	TriggerWeaponEffects( muzzle, axis );

	lastAttackTime = gameLocal.time;

//HUMANHEAD mdc - added to support multiple projectiles
	projectile = NULL;
	SetCurrentProjectile( projectileDefaultDefIndex );	//set back to our default projectile to be on the safe side
//HUMANHEAD END

	return lastProjectile;
}

//
// LaunchProjectile
//
idProjectile *hhGasbagSimple::LaunchProjectileAtVec( const idVec3 &startOrigin, const idMat3 &startAxis, const idVec3 &target, bool clampToAttackCone, const idDict* desiredProjectileDef ) {	//HUMANHEAD mdc - added desiredProjectileDef for supporting multiple projs.
	idVec3				muzzle;
	idVec3				dir;
	idVec3				start;
	trace_t				tr;
	idBounds			projBounds;
	float				distance;
	const idClipModel	*projClip;
	//float				attack_accuracy;
	//float				attack_cone;
	float				projectile_spread;
	float				angle;
	float				spin;
	idAngles			ang;
	int					num_projectiles;
	int					i;
	idMat3				axis;
	idVec3				tmp;
	idProjectile		*lastProjectile;

	//HUMANHEAD mdc - added to support multiple projectiles
	if( desiredProjectileDef ) {	//try to set our projectile to the desiredProjectile
		int projIndex = FindProjectileInfo( desiredProjectileDef );
		if( projIndex >= 0 ) {
			SetCurrentProjectile( projIndex );
		}
	}
	//HUMANHEAD END


	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	//attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	//attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	num_projectiles = spawnArgs.GetInt( "num_projectiles", "1" );

	muzzle = startOrigin;
	axis = startAxis;

	if ( !projectile.GetEntity() ) {
		CreateProjectile( muzzle, axis[ 0 ] );
	}

	lastProjectile = projectile.GetEntity();

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = lastProjectile->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( axis );

	// check if the owner bounds is bigger than the projectile bounds
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( muzzle, viewAxis[ 0 ], distance ) ) {
			start = muzzle + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, muzzle, projClip, axis, MASK_SHOT_RENDERMODEL, this );
	muzzle = tr.endpos;

	float spreadRad = DEG2RAD( projectile_spread );
	for( i = 0; i < num_projectiles; i++ ) {
		// spread the projectiles out
		angle = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
		spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = axis[ 0 ] + axis[ 2 ] * ( angle * idMath::Sin( spin ) ) - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir.Normalize();

		// launch the projectile
		if ( !projectile.GetEntity() ) {
			CreateProjectile( muzzle, dir );
		}
		lastProjectile = projectile.GetEntity();
		lastProjectile->Launch( muzzle, dir, vec3_origin );
		projectile = NULL;
	}

	TriggerWeaponEffects( muzzle, axis );

	lastAttackTime = gameLocal.time;

//HUMANHEAD mdc - added to support multiple projectiles
	projectile = NULL;
	SetCurrentProjectile( projectileDefaultDefIndex );	//set back to our default projectile to be on the safe side
//HUMANHEAD END

	return lastProjectile;
}

#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhGasbagSimple::LinkScriptVariables(void) {
	hhMonsterAI::LinkScriptVariables();
	
	LinkScriptVariable(AI_PODCOUNT);
	LinkScriptVariable(AI_CHARGEDONE);
	LinkScriptVariable(AI_SWOOP);
	LinkScriptVariable(AI_DODGEDAMAGE);
}

//
// Event_GrabEnemy()
//
void hhGasbagSimple::Event_GrabEnemy(void) {
	idThread::ReturnFloat( GrabEnemy() );
}

//
// GrabEnemy()
//
bool hhGasbagSimple::GrabEnemy(void) {
	if (!enemy.IsValid() || enemy->IsBound() || bindController->GetRider()) {
		return false;
	}

	// Don't grab Talon
	if (enemy->IsType(hhTalon::Type)) {
		return false;
	}

	if (enemy->IsType(hhPlayer::Type)) {
		hhPlayer *player = reinterpret_cast<hhPlayer *> (enemy.GetEntity());
		if (player->IsSpiritOrDeathwalking() || player->InGravityZone()) { // Refuse to grab spirit players, or players affected by a gravity zone
			return false;
		}
		player->Freeze();
	}

	bindController->Attach(enemy.GetEntity());
	enemy->SetOrigin(vec3_origin);
	return true;
}

//
// Event_BiteEnemy()
//
void hhGasbagSimple::Event_BiteEnemy(void) {
	if (!enemy.IsValid() || bindController->GetRider() != enemy.GetEntity()) {
		return;
	}

	if (enemy->IsType(hhSpiritProxy::Type)) {
		float diff = gameLocal.GetTime() - reinterpret_cast<hhSpiritProxy *> (enemy.GetEntity())->GetActivationTime();
		if (diff < 1150.0f) {
			 // Don't allow the spirit proxy to ignore damage
			scriptThread->WaitMS(diff + 250.0f);
			CancelEvents(&EV_BiteEnemy);
			PostEventMS(&EV_BiteEnemy, diff + 100.0f);
			return;
		}
	}

	// Direct damage
	bindController->Detach();
	enemy->Damage(this, this, vec3_origin, spawnArgs.GetString("def_damage_bite"), 1.0f, INVALID_JOINT);
}

//
// Damage()
//
void hhGasbagSimple::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location) {
	int curHealth = health;
	idEntity *enemyEnt = enemy.GetEntity();
	hhMonsterAI::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);

	if (health < curHealth) {
		if (gameLocal.time > nextWoundTime && location != INVALID_JOINT) {
			hhFxInfo fx;
			fx.SetEntity(this);
			fx.RemoveWhenDone(true);
			SpawnFxLocal(spawnArgs.GetString("fx_damage"), inflictor->GetOrigin(), dir.ToMat3(), &fx);

			nextWoundTime = gameLocal.time + 250;
		}

		// Only release enemy if we actually took damage
		idEntity *rider;
		if ((rider = bindController->GetRider()) != NULL) {
			bindController->Detach();
			if (rider->IsType(hhPlayer::Type)) {
				rider->PostEventMS(&EV_Unfreeze, 0);
			}
		}

		AI_DODGEDAMAGE = AI_DODGEDAMAGE + (curHealth - health);
	}
}

//
// Event_DirectMoveToPosition()
//
void hhGasbagSimple::Event_DirectMoveToPosition(const idVec3 &pos) {
	StopMove(MOVE_STATUS_DONE);
	DirectMoveToPosition(pos);
}

//
// Event_ChargeEnemy
//
void hhGasbagSimple::Event_ChargeEnemy(void) {
	AI_CHARGEDONE = true;
	StopMove(MOVE_STATUS_DEST_NOT_FOUND);
	if (!enemy.IsValid() || enemy->IsBound()) {
		return;
	}

	idVec3 enemyOrg;

	fly_offset = 0.0f;

	// position destination so that we're in the enemy's view
	enemyOrg = enemy->GetEyePosition();
	enemyOrg -= enemy->GetPhysics()->GetGravityNormal() * fly_offset;

	DirectMoveToPosition(enemyOrg);
	HH_ASSERT(move.bEnemyBlocks == false);
}

void hhGasbagSimple::Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy ) {
	hhMonsterAI::Event_EnemyIsSpirit(player, proxy);
	if (bindController->GetRider() == player) {
		bindController->Detach();
		player->Unfreeze();
		GrabEnemy();
	}
}

void hhGasbagSimple::Event_EnemyIsPhysical( hhPlayer *player, hhSpiritProxy *proxy ) {
	hhMonsterAI::Event_EnemyIsPhysical(player, proxy);
	if (bindController->GetRider() == player) {
		player->Freeze();
	}
}

//
// Event_BindUnfroze
//
void hhGasbagSimple::Event_BindUnfroze(idEntity *unfrozenBind) {
	if (!enemy.IsValid() || unfrozenBind != enemy.GetEntity() || bindController->GetRider() != enemy.GetEntity()) {
		gameLocal.Warning("Gasbag received BindUnfroze event but enemy is not bound to it.\n");
		return;
	}

	bindController->Detach();
}

//
// FlyTurn
//
void hhGasbagSimple::FlyTurn(void) {
	if (!AI_SWOOP && (AI_ENEMY_VISIBLE || move.moveCommand == MOVE_FACE_ENEMY)) {
		TurnToward( lastVisibleEnemyPos );
	} else if ((move.moveCommand == MOVE_FACE_ENTITY) && move.goalEntity.GetEntity()) {
		TurnToward(move.goalEntity.GetEntity()->GetPhysics()->GetOrigin());
	} else if (move.speed > 0.1f) {
		const idVec3 &vel = physicsObj.GetLinearVelocity();
		if (vel.ToVec2().LengthSqr() > 0.1f) {
			TurnToward(vel.ToYaw());
		}
	}
	Turn();
}

//
// Event_GrabCheck()
//
void hhGasbagSimple::Event_GrabCheck(void) {
	idThread::ReturnInt(enemy.IsValid() && enemy->IsBoundTo(this));
}


void hhGasbagSimple::Event_MoveToGrabPosition(void) {
	StopMove(MOVE_STATUS_DEST_NOT_FOUND);
	if (!enemy.IsValid() || enemy->IsBound()) {
		return;
	}

	fly_offset = 0.0f;

	// position destination so that we're right on the enemy when we grab him
	DirectMoveToPosition(enemy->GetOrigin() - spawnArgs.GetVector( "grab_offset" ));
	HH_ASSERT(move.bEnemyBlocks == false);
}

void hhGasbagSimple::SetEnemy(idActor *newEnemy) {
	idEntity *rider;
	if ((rider = bindController->GetRider())) { // This is usually caused by Talon
		if (rider->IsType(hhPlayer::Type)) {
			rider->PostEventMS(&EV_Unfreeze, 0);
		}
		bindController->Detach();
	}

	hhMonsterAI::SetEnemy(newEnemy);
}

void hhGasbagSimple::Event_CheckRange(void) {
	if (!enemy.IsValid()) {
		idThread::ReturnInt(0);
		return;
	}

	int retval;
	float dist = ( enemy->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).Length();

	if (enemy->IsType(hhPlayer::Type)) {
		float min = spawnArgs.GetFloat( "dda_range_min" );
		float max = spawnArgs.GetFloat( "dda_range_max" );
		float player_dist = min + (max - min) * gameLocal.GetDDAValue();

		retval = (int) (dist <= player_dist);
	} else {
		retval = (int) (dist <= 100.0f);
	}

	idThread::ReturnInt(retval);
}

void hhGasbagSimple::Gib( const idVec3 &dir, const char *damageDefName ) {
	// Bypass idActor::Gib()
	idAFEntity_Gibbable::Gib( dir, damageDefName );
}

int hhGasbagSimple::ReactionTo( const idEntity *ent ) {
	//gasbags skip the spirit stuff in hhMonsterAI::ReactionTo()
	if ( bNoCombat ) {
		return ATTACK_IGNORE;
	}
	const idActor *actor = static_cast<const idActor *>( ent );
	if( actor && actor->IsType(hhDeathProxy::Type) ) {
		return ATTACK_IGNORE;
	}
	if ( ent->IsType( hhMonsterAI::Type ) ) {
		const hhMonsterAI *entAI = static_cast<const hhMonsterAI *>( ent );
		if ( entAI && entAI->bNeverTarget ) {
			return ATTACK_IGNORE;
		}
	}

	return idAI::ReactionTo( ent );
}

void hhGasbagSimple::Event_EnemyRangeZ( void ) {
	idThread::ReturnFloat( lastVisibleEnemyPos.z - GetOrigin().z );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build