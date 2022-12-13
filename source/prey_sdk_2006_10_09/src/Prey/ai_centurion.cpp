#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef AI_CenturionLaunch("centurionFire", "d");			//called from anims to handle firing for centurion
const idEventDef AI_CenturionRoar("centurionRoar");					//called from level-script to cause centurion to roar
const idEventDef AI_CenturionArmChop("centurionArmChop");			//called from level-script to actually cause centurion to loose arm
const idEventDef AI_CenturionLooseArm("centurionLooseArm");			//internal: used to actually 'loose' arm
const idEventDef AI_CenturionForceFieldNotify("preForcefieldNotify");
const idEventDef AI_CenturionForceFieldToggle("forcefieldToggle", "d");
const idEventDef AI_CenturionInTunnel("playerInBox", "dE");
const idEventDef AI_CenturionReachedTunnel("reachedTunnel");
const idEventDef AI_CenturionMoveToTunnel("moveToTunnel", NULL, 'd');
const idEventDef AI_CheckForObstructions("checkForObstructions", "d", 'd');
const idEventDef AI_DestroyObstruction("destroyObstruction");
const idEventDef AI_MoveToObstruction("moveToObstruction");
const idEventDef AI_ReachedObstruction("reachedObstruction");
const idEventDef AI_CloseToObstruction("closeToObstruction", 0, 'd');
const idEventDef AI_BackhandImpulse("<backhandImpulse>", "e");
const idEventDef AI_EnemyCloseToObstruction("enemyCloseToObstruction", NULL, 'd');
const idEventDef AI_TakeDamage("takeDamage", "d");
const idEventDef AI_FindNearbyEnemy("findNearbyEnemy", "f", 'e');

CLASS_DECLARATION( hhMonsterAI, hhCenturion )
	EVENT( AI_CenturionLaunch,				hhCenturion::Event_CenturionLaunch )
	EVENT( AI_CenturionRoar,				hhCenturion::Event_ScriptedRoar )
	EVENT( AI_CenturionArmChop,				hhCenturion::Event_ScriptedArmChop )
	EVENT( AI_CenturionLooseArm,			hhCenturion::Event_CenturionLooseArm )
	EVENT( AI_CenturionInTunnel,			hhCenturion::Event_PlayerInTunnel )
	EVENT( AI_CenturionReachedTunnel,		hhCenturion::Event_ReachedTunnel )
	EVENT( AI_CenturionMoveToTunnel,		hhCenturion::Event_MoveToTunnel )
	EVENT( AI_CenturionForceFieldNotify,	hhCenturion::Event_ForceFieldNotify )
	EVENT( AI_CenturionForceFieldToggle,	hhCenturion::Event_ForceFieldToggle )
	EVENT( AI_CheckForObstructions,			hhCenturion::Event_CheckForObstruction )
	EVENT( AI_MoveToObstruction,			hhCenturion::Event_MoveToObstruction )
	EVENT( AI_DestroyObstruction,			hhCenturion::Event_DestroyObstruction )
	EVENT( AI_ReachedObstruction,			hhCenturion::Event_ReachedObstruction )
	EVENT( AI_CloseToObstruction,			hhCenturion::Event_CloseToObstruction )
	EVENT( AI_BackhandImpulse,				hhCenturion::Event_BackhandImpulse )
	EVENT( AI_EnemyCloseToObstruction,		hhCenturion::Event_EnemyCloseToObstruction )
	EVENT( AI_TakeDamage,					hhCenturion::Event_TakeDamage )
	EVENT( AI_FindNearbyEnemy,				hhCenturion::Event_FindNearbyEnemy )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

void hhCenturion::Spawn() {
	AI_CENTURION_ARM_MISSING = false;
	AI_CENTURION_REQUIRE_ROAR = false;
	AI_CENTURION_ARM_TUNNEL = false;
	AI_CENTURION_SCRIPTED_ROAR = 0;
	AI_CENTURION_FORCEFIELD_WAIT = false;
	AI_CENTURION_SCRIPTED_TUNNEL = 0;
}

void hhCenturion::Event_PostSpawn( void ) {
	hhMonsterAI::Event_PostSpawn();

	const idKeyValue *kv = spawnArgs.MatchPrefix( "aasObstacle" );
	idEntityPtr<idEntity> ent;
	while ( kv ) {
		ent = gameLocal.FindEntity( kv->GetValue() );
		if ( ent.IsValid() ) {
			obstacles.AddUnique( ent );
		}
		kv = spawnArgs.MatchPrefix( "aasObstacle", kv );
	}
}

#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhCenturion::LinkScriptVariables( void ) {
	hhMonsterAI::LinkScriptVariables();
	
	LinkScriptVariable(AI_CENTURION_ARM_MISSING);
	LinkScriptVariable(AI_CENTURION_REQUIRE_ROAR);
	LinkScriptVariable(AI_CENTURION_ARM_TUNNEL);
	LinkScriptVariable(AI_CENTURION_SCRIPTED_ROAR);
	LinkScriptVariable(AI_CENTURION_SCRIPTED_TUNNEL);
	LinkScriptVariable(AI_CENTURION_FORCEFIELD_WAIT);
}


void hhCenturion::Event_ForceFieldNotify() {
	if( !armchop_Target.GetEntity() ) {
		return;		//must not be in arm-chop mode, so we don't care yet
	}
	AI_CENTURION_REQUIRE_ROAR = true;
	AI_CENTURION_FORCEFIELD_WAIT = true;
}

void hhCenturion::Event_ForceFieldToggle( int toggle ) {
	if( !armchop_Target.GetEntity() ) {
		return;
	}
	AI_CENTURION_FORCEFIELD_WAIT = false;
	if (!toggle) {
		AI_CENTURION_REQUIRE_ROAR = false;
	} else if( AI_CENTURION_ARM_TUNNEL ) {
		Event_ScriptedArmChop();
		return;
	}
}

void hhCenturion::Event_PlayerInTunnel( int toggle, idEntity* ent ) {
	if( AI_CENTURION_ARM_MISSING ) {		//don't care if the player goes back in tunnel once we already lost our arm
		return;
	}

	if( toggle ) {
		if( !ent ) {
			gameLocal.DWarning( "entity needs to be present for tunnel event" );
			return;
		}
		armchop_Target = ent;
		AI_CENTURION_SCRIPTED_TUNNEL = 1;
	}
	else {
		armchop_Target = NULL;
		AI_CENTURION_SCRIPTED_TUNNEL = 0;
	}
}

void hhCenturion::Event_MoveToTunnel() {
	if (!armchop_Target.IsValid()) {
		idThread::ReturnInt(0);
		return;
	}

	StopMove(MOVE_STATUS_DONE);
	MoveToPosition(armchop_Target->GetOrigin());
	idThread::ReturnInt(1);
}

void hhCenturion::Event_ReachedTunnel() {
	if( armchop_Target.IsValid() ) {
		idAngles faceAngles = armchop_Target->GetAxis()[0].ToAngles();
		ideal_yaw		= faceAngles.yaw;
		current_yaw		= faceAngles.yaw;
		SetAxis( armchop_Target->GetAxis() );
	}
}

void hhCenturion::Event_ScriptedRoar() {
	if ( AI_CENTURION_SCRIPTED_ROAR > 0 ) {
		gameLocal.Warning( "centurionRoar() called more than once!\n");
	} else {
		AI_CENTURION_SCRIPTED_ROAR = 1;
	}
}

void hhCenturion::Event_ScriptedArmChop() {
	const function_t* newstate = NULL;
	newstate = GetScriptFunction( "state_ScriptedArmChop" );
	if( newstate ) {
		SetState( newstate );
	}
	else {
		gameLocal.Warning( "Unable to find 'state_ScriptedArmChop' on centurion" );
	}
}

void hhCenturion::Event_CenturionLooseArm() {
	idDict		args;
	idEntity*	ent = NULL;
	idVec3		jointLoc;
	idMat3		jointAxis;

	animator.GetJointTransform( spawnArgs.GetString("arm_severjoint", ""), gameLocal.time, jointLoc, jointAxis );
	jointLoc = renderEntity.origin + jointLoc * renderEntity.axis;
	args.SetVector( "origin", jointLoc );
	args.SetMatrix( "rotation", renderEntity.axis );
	args.SetBool( "spin", 0 );
	args.SetFloat( "triggersize", 48.f );
	args.SetBool( "enablePickup", true );
	args.SetFloat( "respawn", 0.f );
	ent = gameLocal.SpawnObject( spawnArgs.GetString("def_arm_weaponclass", ""), &args );

	SetSkinByName( spawnArgs.GetString( "skin_arm_gone" ) );

	hhFxInfo fx;
	fx.SetEntity( this );
	fx.RemoveWhenDone( true );
	SpawnFxLocal( spawnArgs.GetString( "fx_armchop" ), jointLoc, mat3_identity, &fx );

	//drop any stuck arrows
	idEntity *next;
	for( ent = teamChain; ent != NULL; ent = next ) {
		next = ent->GetTeamChain();
		if ( ent && ent->IsType( hhProjectile::Type ) ) {
			ent->Unbind();
			ent->Hide();
			ent->PostEventSec( &EV_Remove, 5 );
			next = teamChain;
		}
	}

	AI_CENTURION_ARM_MISSING = true;
}

void hhCenturion::Event_CenturionLaunch( const idList<idStr>* parmList ) {
	const idDict* projDef = NULL;
	const idSoundShader* soundShader = NULL;

	// parms: joint, projectileDef, sound, fx
	if( !parmList || parmList->Num() != 5 ) {
		gameLocal.Warning( "Incorrect paramater number" );
		return;
	}
//Rbarrel_A projectile_centurion_autocannon snd_fire fx_muzzleFlash
	const char* jointName = (*parmList)[ 0 ].c_str();
	const char* projectileDefName = (*parmList)[ 1 ].c_str();
	const char* soundName = (*parmList)[ 2 ].c_str();
	const char* fxName = (*parmList)[ 3 ].c_str();
	int autoAim = atoi( (*parmList)[ 4 ].c_str() );

	if( AI_CENTURION_ARM_MISSING ) {	//skip firing if joint is on the severed arm
		if( !idStr::Icmp(jointName, spawnArgs.GetString("severed_jointA", "")) || !idStr::Icmp(jointName, spawnArgs.GetString("severed_jointB", "")) ) {
			return;
		}
	}

	projDef = gameLocal.FindEntityDefDict( projectileDefName, false );
	HH_ASSERT( !shootTarget.IsValid() );
	// If autoAim is true and we're not blending, and we're facing the enemy, auto-aim
	if ( ( autoAim == 1 || ( autoAim == 2 && gameLocal.random.RandomInt(100) < 50 ) ) && torsoAnim.animBlendFrames == 0 && FacingEnemy( 5.0f ) ) {
		AimedAttackMissile( jointName, projDef );
	} else { // Otherwise do a normal missile attack
		Event_AttackMissile( jointName, projDef, 1 );
	}

	if(idStr::Cmpn( soundName, "snd_", 4)) {
		soundShader = declManager->FindSound( soundName );
		if( soundShader->GetState() == DS_DEFAULTED ) {
			gameLocal.Warning( "Sound '%s' not found", soundName );
		}
		StartSoundShader( soundShader, SND_CHANNEL_WEAPON, 0, false, NULL );
	}
	else {
		if( !StartSound(soundName, SND_CHANNEL_WEAPON, 0, false, NULL) ) {
			gameLocal.Warning( "Framecommand 'centurionFire' on entity '%s' could not find sound '%s'", GetName(), soundName );
		}
	}
	BroadcastFxInfoAlongBone( spawnArgs.GetString(fxName), jointName );
}


void hhCenturion::Event_DestroyObstruction() {
	if( pillarEntity.IsValid() ) {
		pillarEntity->PostEventMS( &EV_Activate, 0.f, this );
	}
}

void hhCenturion::Event_CheckForObstruction( int checkPathToPillar ) {
	bool obstacle = false;
	predictedPath_t path;
	if( enemy.IsValid() ) {
		idVec3 end = enemy->GetPhysics()->GetOrigin();
		if ( !checkPathToPillar ) {
			trace_t tr;
			idVec3 toPos, eye = GetEyePosition();

			if ( enemy->IsType( idActor::Type ) ) {
				toPos = ( ( idActor * )enemy.GetEntity() )->GetEyePosition();
			} else {
				toPos = enemy->GetPhysics()->GetOrigin();
			}

			gameLocal.clip.TracePoint( tr, eye, toPos, MASK_SHOT_BOUNDINGBOX, this );

			idEntity *traceEnt = gameLocal.GetTraceEntity( tr );
			if ( traceEnt && ( tr.fraction < 1.0f || traceEnt != enemy.GetEntity() ) ) {
				//check to see if the other object is an pillar...
				if( traceEnt->spawnArgs.GetInt("centurion_pillar", "0") == 1 ) {
					pillarEntity = traceEnt;
					obstacle = true;
				}
			}

		} else if ( pillarEntity.IsValid() ) {
			idAI::PredictPath( this, this->aas, physicsObj.GetOrigin(), enemy->GetPhysics()->GetOrigin() - physicsObj.GetOrigin(), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : (SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );
			if( path.endEvent != 0 && path.blockingEntity ) {
				//check to see if the other object is an pillar...
				if( path.blockingEntity->spawnArgs.GetInt("centurion_pillar", "0") == 1 ) {
					//check to see if we can path to the obstacle clearly
					pillarEntity = path.blockingEntity;
					//idAI::PredictPath( this, this->aas, physicsObj.GetOrigin(), pillarEntity->GetPhysics()->GetOrigin() - physicsObj.GetOrigin(), 1000, 1000, (SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA), path );
					//if( path.endEvent != 0 && path.blockingEntity == pillarEntity.GetEntity() ) {
					obstacle = true;
					//}
				}
			}
		}
	}
	idThread::ReturnInt( (int)obstacle );
}

void hhCenturion::Event_CloseToObstruction() {
	if( physicsObj.GetAbsBounds().IntersectsBounds(pillarEntity->GetPhysics()->GetAbsBounds()) ) {
		idThread::ReturnInt( 1 );
		return;
	}
	idThread::ReturnInt( 0 );
}

void hhCenturion::Event_ReachedObstruction() {
	FaceEntity( pillarEntity.GetEntity() );
}
	
void hhCenturion::Event_MoveToObstruction() {
	idVec3 temp;
	if ( aas ) {
		int toAreaNum = PointReachableAreaNum( pillarEntity.GetEntity()->GetOrigin() );
		temp = pillarEntity.GetEntity()->GetOrigin();
		aas->PushPointIntoAreaNum( toAreaNum, temp );
		MoveToPosition( temp );
	} else {
		gameLocal.Warning( "Centurion has no aas for MoveToObstruction\n" );
	}
}

void hhCenturion::Think() {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) {
		return;
	}

	hhMonsterAI::Think();

	if(ai_debugBrain.GetInteger() > 0 && state) {
		if ( enemy.IsValid() && enemy->GetHealth() > 0 ) {
			float dist = ( GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin() ).LengthFast();
			gameRenderWorld->DrawText( va("%f", dist), this->GetEyePosition() + idVec3(0.0f, 0.0f, 40.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		}
		gameRenderWorld->DrawText(state->Name(), this->GetEyePosition() + idVec3(0.0f, 0.0f, 40.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		gameRenderWorld->DrawText(torsoAnim.state, this->GetEyePosition() + idVec3(0.0f, 0.0f, 20.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		gameRenderWorld->DrawText(legsAnim.state, this->GetEyePosition() + idVec3(0.0f, 0.0f, 0.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
	}
}

bool hhCenturion::AttackMelee( const char *meleeDefName ) {
	const idDict *meleeDef;
	idActor *enemyEnt = enemy.GetEntity();
	const char *p;
	const idSoundShader *shader;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown melee '%s'", meleeDefName );
	}

	if ( !enemyEnt ) {
		p = meleeDef->GetString( "snd_miss" );
		if ( p && *p ) {
			shader = declManager->FindSound( p );
			StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		}
		return false;
	}

	// make sure the trace can actually hit the enemy
	if ( !TestMelee() ) {
		// missed
		p = meleeDef->GetString( "snd_miss" );
		if ( p && *p ) {
			shader = declManager->FindSound( p );
			StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
		}
		return false;
	}

	//
	// do the damage
	//
	p = meleeDef->GetString( "snd_hit" );
	if ( p && *p ) {
		shader = declManager->FindSound( p );
		StartSoundShader( shader, SND_CHANNEL_DAMAGE, 0, false, NULL );
	}

	idVec3	kickDir;
	meleeDef->GetVector( "kickDir", "0 0 0", kickDir );

	idVec3	globalKickDir;
	globalKickDir = ( viewAxis * physicsObj.GetGravityAxis() ) * kickDir;

	enemyEnt->Damage( this, this, globalKickDir, meleeDefName, 1.0f, INVALID_JOINT );
	if ( enemyEnt->IsActiveAF() && !enemyEnt->IsType( idPlayer::Type ) ) {
		PostEventMS( &AI_BackhandImpulse, 0, enemyEnt );
	}

	lastAttackTime = gameLocal.time;

	return true;
}

void hhCenturion::Event_BackhandImpulse( idEntity* ent ) {
	const idDict *meleeDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_impulse_damage"), false );
	if ( !meleeDef || !ent ) {
		return;
	}
	idVec3 kickDir;
	meleeDef->GetVector( "kickDir", "0 0 0", kickDir );
	idVec3 globalKickDir = ( viewAxis * physicsObj.GetGravityAxis() ) * kickDir;
	globalKickDir *= ent->spawnArgs.GetFloat( "kick_scale", "1.0" );
	ent->ApplyImpulse( this, 0, ent->GetOrigin(), meleeDef->GetFloat( "push_ragdoll" ) * globalKickDir );
}

/*
=====================
hhCenturion::Save
=====================
*/
void hhCenturion::Save( idSaveGame *savefile ) const {
	armchop_Target.Save( savefile );
	pillarEntity.Save( savefile );
}

/*
=====================
hhCenturion::Restore
=====================
*/
void hhCenturion::Restore( idRestoreGame *savefile ) {
	armchop_Target.Restore( savefile );
	pillarEntity.Restore( savefile );

	// Restore the obstacle list
	const idKeyValue *kv = spawnArgs.MatchPrefix( "aasObstacle" );
	idEntityPtr<idEntity> ent;
	while ( kv ) {
		ent = gameLocal.FindEntity( kv->GetValue() );
		if ( ent.IsValid() ) {
			obstacles.AddUnique( ent );
		}
		kv = spawnArgs.MatchPrefix( "aasObstacle", kv );
	}
}

void hhCenturion::Event_EnemyCloseToObstruction(void) {
	bool close = false;
	if ( pillarEntity.IsValid() && enemy.IsValid() ) {
		float obs_dist = ( pillarEntity->GetOrigin() - GetOrigin() ).Length();
		float dist = ( enemy->GetOrigin() - GetOrigin() ).Length();

		// If the enemy is further from me than the pillar...
		if ( dist > obs_dist) {
			dist = ( pillarEntity->GetOrigin() - enemy->GetOrigin() ).Length();
			// If the enemy is less than 400 units from the pillar, then he's 'close'
			if ( dist < 400.0f ) {
				close = true;
			}
		}
	}
	idThread::ReturnInt( (int) close );
}

void hhCenturion::AimedAttackMissile( const char *jointname, const idDict *projDef) {
	idProjectile *proj;
	idVec3 target, origin = GetOrigin();
	bool inShuttle = false;

	if ( shootTarget.IsValid() ) {
		target = shootTarget->GetOrigin();
	} else if ( enemy.IsValid() ) {
		target = enemy->GetOrigin();
		if ( enemy->IsType( idActor::Type ) ) {
			target.z += enemy->EyeHeight() / 4.0f;
		}
	} else {
		// No target?  Do the default attack
		Event_AttackMissile( jointname, projDef, 1 );
		return;
	}

	// If target is too close do a non-aimed attack
	if ( fabsf( origin.x - target.x ) < 256 &&
		   fabsf( origin.y - target.y ) < 256 ) {
		Event_AttackMissile( jointname, projDef, 1 );
		return;
	}

	idVec3 dist = origin - target;

	if ( shootTarget.IsValid() ) {
		proj = LaunchProjectile( jointname, shootTarget.GetEntity(), true, projDef );
	} else {
		proj = LaunchProjectile( jointname, enemy.GetEntity(), true, projDef );
	}
}

void hhCenturion::Event_TakeDamage(int takeDamage) {
	// Set the takedamage flag
	fl.takedamage = (takeDamage != 0);
}

void hhCenturion::Event_FindNearbyEnemy( float distance ) {
	// Search for the monster nearest to us
	idAI *nearest = NULL;
	float dist, nearDist = idMath::INFINITY;
	idAI *ai = reinterpret_cast<idAI *> (gameLocal.FindEntityOfType( idAI::Type, NULL ));
	while (ai) {
		if (ai != this) { // Don't target yourself
			dist = (ai->GetOrigin() - GetOrigin()).Length();
			if (dist < nearDist) {
				nearDist = dist;
				nearest = ai;
			}
		}
		ai = reinterpret_cast<idAI *> (gameLocal.FindEntityOfType( idAI::Type, ai ));
	}

	// If we found one near us and it's near enough, return it
	if (nearest && nearDist < distance) {
		idThread::ReturnEntity(nearest);
	} else {
		idThread::ReturnEntity(NULL);
	}
}

void hhCenturion::Event_Touch( idEntity *other, trace_t *trace ) {
	if ( (!enemy.GetEntity() || other->IsType( hhPlayer::Type )) && !other->fl.notarget && ( ReactionTo( other ) & ATTACK_ON_ACTIVATE ) ) {
		Activate( other );
		SetEnemy( static_cast<idActor *> ( other ) );
	}
	AI_PUSHED = true;
}

#endif	//HUMANHEAD jsh PCF 5/26/06: code removed for demo build