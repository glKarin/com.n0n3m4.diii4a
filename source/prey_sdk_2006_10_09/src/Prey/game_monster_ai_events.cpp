#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef MA_AttackMissileEx("<attackMissileEx>", "sd",NULL);
const idEventDef MA_SetMeleeRange("setMeleeRange", "f",NULL);
const idEventDef MA_FindReaction("findReaction", "s", 'E');
const idEventDef MA_UseReaction("useReaction");
const idEventDef MA_EnemyOnSide("enemyOnSide", NULL, 'f');
const idEventDef MA_HitCheck( "hitCheck", "Es", 'd' );
const idEventDef MA_CreateMonsterPortal("createMonsterPortal", NULL, 'f');
const idEventDef MA_GetShootTarget("getShootTarget", NULL,'E');
const idEventDef MA_TriggerReactEnt("triggerReactEnt");
const idEventDef MA_InitialWallwalk("<initialWallwalk>");
const idEventDef MA_GetVehicle("getVehicle",NULL,'E');
const idEventDef MA_EnemyAimingAtMe("enemyAimingAtMe",NULL,'d');
const idEventDef MA_ReachedEntity("reachedEntity","e",'d');
const idEventDef MA_EnemyOnSpawn("<enemyOnSpawn>",NULL,NULL);
const idEventDef MA_SpawnFX( "spawnFX", "s" );
const idEventDef MA_SplashDamage( "splashDamage", "s" );
const idEventDef MA_SetVehicleState( "<setVehicleState>",NULL,NULL );
const idEventDef MA_FollowPath( "followPath", "s", NULL );
const idEventDef MA_GetLastReachableEnemyPos( "getLastReachableEnemyPos", "", 'v' );
const idEventDef MA_OnProjectileLaunch("<onProjectileLaunch>", "e");
const idEventDef MA_EnemyIsA("enemyIsA", "s", 'd');
const idEventDef MA_Subtitle("subtitle", "d");
const idEventDef MA_SubtitleOff("<subtitleOff>");
const idEventDef MA_EnableHeadlook("enableHeadlook");
const idEventDef MA_DisableHeadlook("disableHeadlook");
const idEventDef MA_EnableEyelook("enableEyelook");
const idEventDef MA_DisableEyelook("disableEyelook");
const idEventDef MA_FacingEnemy("facingEnemy", "f", 'd');
const idEventDef MA_BossBar("bossBar", "d");
const idEventDef MA_FallNow("<fallNow>");
const idEventDef MA_AllowFall("allowFall", "d");	//HUMANHEAD jsh PCF 5/3/06 Changed parameter from "f" to "d"
const idEventDef MA_InPlayerFov("inPlayerFov", NULL, 'd');
const idEventDef MA_EnemyIsSpirit("<enemyIsSpirit>", "ee");
const idEventDef MA_EnemyIsPhysical("<enemyIsPhysical>", "ee");
const idEventDef MA_IsRagdoll("isRagdoll", NULL, 'd');
const idEventDef MA_MoveDone("moveDone", NULL, 'd');
const idEventDef MA_SetShootTarget("setShootTarget", "E");
const idEventDef EV_EnemyInGravityZone( "enemyInGravityZone", NULL, 'f' );
const idEventDef MA_SetLookOffset( "setLookOffset", "v" );
const idEventDef MA_SetHeadFocusRate( "setHeadFocusRate", "f" );
const idEventDef MA_FlyZip( "flyZip" );
const idEventDef MA_UseConsole( "useConsole", "e" );
const idEventDef MA_TestMeleeDef("testMeleeDef", "s",'f');
const idEventDef MA_EnemyInVehicle("enemyInVehicle", NULL, 'd');
const idEventDef MA_EnemyOnWallwalk("enemyOnWallwalk", NULL, 'd');
const idEventDef MA_AlertAI("alertAI", "ef" );
const idEventDef MA_TestAnimMoveBlocked("testAnimMoveBlocked", "s", 'e');
const idEventDef MA_InGravityZone("inGravityZone", NULL, 'd');
const idEventDef MA_StartSoundDelay( "startSoundDelay", "sddf", 'f' );
const idEventDef MA_SetTeam( "setTeam", "d" );
const idEventDef MA_GetAttackPoint ("getAttackPoint", NULL, 'v');
const idEventDef MA_HideNoDormant("hideNoDormant" );
const idEventDef MA_SoundOnModel("soundOnModel");
const idEventDef MA_ActivatePhysics("activatePhysics");
const idEventDef MA_IsVehicleDocked("isVehicleDocked");
const idEventDef MA_EnemyInSpirit( "enemyInSpirit", NULL, 'd' );
const idEventDef MA_GetAttackNode( "getAttackNode", NULL, 'v' );

CLASS_DECLARATION(idAI, hhMonsterAI)
	EVENT( MA_AttackMissileEx,		hhMonsterAI::Event_AttackMissileEx )
	EVENT( MA_FindReaction,			hhMonsterAI::Event_FindReaction )
	EVENT( MA_UseReaction,			hhMonsterAI::Event_UseReaction )
	EVENT( MA_SetMeleeRange,		hhMonsterAI::Event_SetMeleeRange )
	EVENT( MA_EnemyOnSide,			hhMonsterAI::Event_EnemyOnSide )
	EVENT( MA_HitCheck,				hhMonsterAI::Event_HitCheck )
	EVENT( MA_CreateMonsterPortal,	hhMonsterAI::Event_CreateMonsterPortal )
	EVENT( MA_GetShootTarget,		hhMonsterAI::Event_GetShootTarget )
	EVENT( MA_TriggerReactEnt,		hhMonsterAI::Event_TriggerReactEnt )
	EVENT( MA_InitialWallwalk,		hhMonsterAI::Event_InitialWallwalk )
	EVENT( MA_GetVehicle,			hhMonsterAI::Event_GetVehicle )
	EVENT( MA_EnemyAimingAtMe,		hhMonsterAI::Event_EnemyAimingAtMe )
	EVENT( MA_ReachedEntity,		hhMonsterAI::Event_ReachedEntity )
	EVENT( MA_EnemyOnSpawn,			hhMonsterAI::Event_EnemyOnSpawn )
	EVENT( MA_SpawnFX,				hhMonsterAI::Event_SpawnFX )
	EVENT( MA_SplashDamage,			hhMonsterAI::Event_SplashDamage)
	EVENT( MA_SetVehicleState,		hhMonsterAI::Event_SetVehicleState )
	EVENT( EV_PostSpawn,			hhMonsterAI::Event_PostSpawn )
	EVENT( MA_FollowPath,			hhMonsterAI::Event_FollowPath )
	EVENT( MA_GetLastReachableEnemyPos, hhMonsterAI::Event_GetLastReachableEnemyPos )
	EVENT( MA_EnemyIsA,				hhMonsterAI::Event_EnemyIsA )
	EVENT( MA_Subtitle,				hhMonsterAI::Event_Subtitle )
	EVENT( MA_SubtitleOff,			hhMonsterAI::Event_SubtitleOff )
	EVENT( MA_EnableHeadlook,		hhMonsterAI::Event_EnableHeadlook )
	EVENT( MA_DisableHeadlook,		hhMonsterAI::Event_DisableHeadlook )
	EVENT( MA_EnableEyelook,		hhMonsterAI::Event_EnableEyelook )
	EVENT( MA_DisableEyelook,		hhMonsterAI::Event_DisableEyelook )
	EVENT( MA_FacingEnemy,			hhMonsterAI::Event_FacingEnemy )
	EVENT( MA_BossBar,				hhMonsterAI::Event_BossBar )
	EVENT( MA_FallNow,				hhMonsterAI::Event_FallNow )
	EVENT( MA_AllowFall,			hhMonsterAI::Event_AllowFall )
	EVENT( MA_InPlayerFov,			hhMonsterAI::Event_InPlayerFov )
	EVENT( MA_EnemyIsSpirit,		hhMonsterAI::Event_EnemyIsSpirit )
	EVENT( MA_EnemyIsPhysical,		hhMonsterAI::Event_EnemyIsPhysical )
	EVENT( MA_IsRagdoll,			hhMonsterAI::Event_IsRagdoll )
	EVENT( MA_MoveDone,				hhMonsterAI::Event_MoveDone )
	EVENT( MA_SetShootTarget,		hhMonsterAI::Event_SetShootTarget)
	EVENT( EV_EnemyInGravityZone,	hhMonsterAI::Event_EnemyInGravityZone )
	EVENT( MA_SetLookOffset,		hhMonsterAI::Event_SetLookOffset )
	EVENT( MA_SetHeadFocusRate,		hhMonsterAI::Event_SetHeadFocusRate )
	EVENT( MA_FlyZip,				hhMonsterAI::Event_FlyZip )
	EVENT( MA_UseConsole,			hhMonsterAI::Event_UseConsole )
	EVENT( MA_TestMeleeDef,			hhMonsterAI::Event_TestMeleeDef )
	EVENT( MA_EnemyInVehicle,		hhMonsterAI::Event_EnemyInVehicle )
	EVENT( MA_EnemyOnWallwalk,		hhMonsterAI::Event_EnemyOnWallwalk )
	EVENT( MA_AlertAI,				hhMonsterAI::Event_AlertAI )
	EVENT( MA_TestAnimMoveBlocked,	hhMonsterAI::Event_TestAnimMoveBlocked )
	EVENT( MA_InGravityZone,		hhMonsterAI::Event_InGravityZone )
	EVENT( MA_StartSoundDelay,		hhMonsterAI::Event_StartSoundDelay )
	EVENT( MA_SetTeam,				hhMonsterAI::Event_SetTeam )
	EVENT( MA_GetAttackPoint,		hhMonsterAI::Event_GetAttackPoint) 
	EVENT( MA_HideNoDormant,		hhMonsterAI::Event_HideNoDormant ) 
	EVENT( MA_SoundOnModel,			hhMonsterAI::Event_SoundOnModel )
	EVENT( MA_ActivatePhysics,		hhMonsterAI::Event_ActivatePhysics )
	EVENT( MA_IsVehicleDocked,		hhMonsterAI::Event_IsVehicleDocked )
	EVENT( MA_EnemyInSpirit,		hhMonsterAI::Event_EnemyInSpirit )
END_CLASS

void hhMonsterAI::Event_EnemyAimingAtMe() {
	if ( enemy.IsValid() ) {
		if ( enemy->GetAxis()[0] * -(enemy->GetOrigin() - GetOrigin()).ToNormal() ) {
			idThread::ReturnInt( true );
			return;
		}
	}

	idThread::ReturnInt( false );
}

//A little after Spawn() or Show(), check if monster is stuck on wallwalk.
void hhMonsterAI::Event_InitialWallwalk() {
	if ( IsHidden() ) {
		return;
	}
	idMat3 initRot = spawnArgs.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1" );
	if ( initRot == mat3_identity ) {
		return;
	}
	trace_t TraceInfo;
	gameLocal.clip.TracePoint(TraceInfo, GetOrigin(), GetOrigin() - initRot[2]*100, GetPhysics()->GetClipMask(), this);
	if( TraceInfo.fraction < 1.0f && health > 0 ) {
		if ( gameLocal.GetMatterType(TraceInfo, NULL) == SURFTYPE_WALLWALK ) {
			DisableIK();
			SetOrigin( TraceInfo.c.point + initRot * idVec3(0,0,1) );
			GetPhysics()->SetAxis( initRot );
			viewAxis = initRot;
			SetGravity( -TraceInfo.c.normal * 1066 );
			physicsObj.SetClipModelAxis();
			renderEntity.axis = initRot;

			idVec3 local_dir;
			physicsObj.GetAxis().ProjectVector( initRot[0], local_dir );
			ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() );
			current_yaw = ideal_yaw;
		}
	}
}

void hhMonsterAI::Event_EnemyOnSide() {
	if( enemy.IsValid() ) {
		idVec3 povPos, targetPos;
		povPos = enemy->GetPhysics()->GetOrigin();
		targetPos = GetPhysics()->GetOrigin();
		idVec3 povToTarget = targetPos - povPos;
		povToTarget.z = 0.f;
		povToTarget.Normalize();
		float dot = GetPhysics()->GetAxis()[ 1 ] * povToTarget;
		idThread::ReturnFloat( dot );
	}
	else {
		idThread::ReturnFloat( 0.f );
	}
}

void hhMonsterAI::Event_EnemyIsA( const char* testclass ) {
	idTypeInfo* type = idClass::GetClass( testclass );
	if( type ) {
		if( enemy.IsValid() ) {
			if( enemy->GetType()->IsType(*type) ) {
				idThread::ReturnInt( 1 );
				return;
			}
		}
	}
	idThread::ReturnInt( 0 );
}

void hhMonsterAI::Event_SetMeleeRange( float newRange ) {	
	melee_range = newRange;
}

//
// Event_UseReaction
//
//  Notes:
//	 + Need to support the actual various types of Causes, not just this one specific case.
//	 + Need to support blends (in/out) on animation causes
void hhMonsterAI::Event_UseReaction() {
	if( !AI_USING_REACTION ) {
		AI_USING_REACTION = true;
		AI_REACTION_FAILED = false;
		AI_REACTION_ANIM = false;
	}

	if( !CheckValidReaction() ) {
		AI_USING_REACTION = false;
		AI_REACTION_FAILED = false;
		AI_REACTION_ANIM = false;
		return;
	}

	hhReaction *reaction = targetReaction.GetReaction();
	assert(reaction); // CheckValidReaction() should ensure this is a valid pointer.
	if( reaction->desc->flags & hhReactionDesc::flag_Exclusive ) {
		if ( !reaction->exclusiveOwner.IsValid() ) {
			reaction->exclusiveOwner = this;
		} else if ( reaction->exclusiveOwner != this ) {
			AI_USING_REACTION = false;
			AI_REACTION_FAILED = false;
			AI_REACTION_ANIM = false;
			return;
		}
	}

	idVec3 tp = GetTouchPos( reaction->causeEntity.GetEntity(), reaction->desc );
	idBounds tpb;
	bool validTpb = GetTouchPosBound( reaction->desc, tpb );

	if( AI_REACTION_ANIM ) {
		if( torsoAnim.AnimDone( 0 ) || GetAnimator()->CurrentAnim( ANIMCHANNEL_TORSO )->GetEndTime() < 0.f ) {
			AI_REACTION_ANIM = false;
			Event_AnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
			Event_AnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 0 );
			ProcessEvent( &AI_EnablePain );
			FinishReaction();
		}
	}
	else {
		//play reaction sound if one exists
		if ( !ai_skipSpeech.GetBool() && gameLocal.GetTime() > nextSpeechTime ) {
			idStr speechDesc = idStr("snd_speech_");
			speechDesc += hhReactionDesc::EffectToStr( reaction->desc->effect );
			speechDesc += idStr( "_" );
			speechDesc += targetReaction.entity->spawnArgs.GetString( "speech_name" );
			if ( ai_debugBrain.GetBool() ) {
				gameLocal.Printf( "reaction speech(%s)\n", speechDesc.c_str() );
			}
			const idKeyValue *kv = spawnArgs.MatchPrefix( speechDesc );
			if( kv ) {
				StartSound(kv->GetKey(), SND_CHANNEL_VOICE, 0, true, NULL);
			}
			nextSpeechTime = gameLocal.GetTime() + int(spawnArgs.GetFloat( "speech_wait", "2.0" ) * 1000);
		}
		if( (validTpb && !NearEnoughTouchPos(reaction->causeEntity.GetEntity(), tp, tpb)) || (!validTpb && !ReachedPos(tp, move.moveCommand)) ) {
			if ( !MoveToPosition( tp, true ) ) {
				//position is unreachable so stop using this reaction
				AI_USING_REACTION = false;
				AI_REACTION_FAILED = false;
				AI_REACTION_ANIM = false;
			}
		} else {
			SlideToPosition( tp, 0.5 );
			StopMove( MOVE_STATUS_DONE );
			if( reaction->desc->flags & hhReactionDesc::flag_SnapToPoint ) {
				SetOrigin( idVec3(tp.x, tp.y, tp.z) );
			}
			//Turn to face direction specified by node
			if( reaction->causeEntity->spawnArgs.GetFloat("face_dir") ) {
				idAngles faceAngles = reaction->causeEntity->GetAxis()[0].ToAngles();
				ideal_yaw		= faceAngles.yaw;
				current_yaw		= faceAngles.yaw;
				SetAxis( reaction->causeEntity->GetAxis() );
			}
			else if( reaction->desc->flags & hhReactionDesc::flag_AnimFaceCauseDir ) {
				idAngles faceAngles = ( reaction->causeEntity->GetOrigin() - GetOrigin() ).ToAngles();
				ideal_yaw		= faceAngles.yaw;
				current_yaw	= faceAngles.yaw;
				idAngles newAngles = GetAxis().ToAngles();
				newAngles.yaw = faceAngles.yaw;
				SetAxis( newAngles.ToMat3() );
			}
			idEntity *ent = NULL;
			switch( reaction->desc->cause ) {
				case hhReactionDesc::Cause_Touch:
					FinishReaction();
					break;
				case hhReactionDesc::Cause_Use:
					ent = targetReaction.entity.GetEntity();
					if( ent && ent->IsType(hhConsole::Type)) {	
						hhConsole *cons = static_cast<hhConsole*>(ent);
						if(!cons->CanUse(this)) {
							return;
						}					
						cons->Use(this);
					}
					// Using vehicle -- just bail!
					else if(ent->IsType(hhVehicle::Type)) {
						EnterVehicle( static_cast<hhVehicle*>(ent) );
						FinishReaction();
						return;
					} else {
						gameLocal.Warning("\n AI TRIED TO USE UNUSABLE ENTITY!");
					}
					FinishReaction();
					break;
				case hhReactionDesc::Cause_PlayAnim:
					AI_REACTION_ANIM = true;
					GetAnimator()->Clear( ANIMCHANNEL_TORSO, gameLocal.GetTime(), 0 );
					GetAnimator()->Clear( ANIMCHANNEL_TORSO, gameLocal.GetTime(), 0 );
					GetAnimator()->ClearAllAnims( gameLocal.GetTime(), 0 );
					Event_AnimState( ANIMCHANNEL_TORSO, "Torso_DoNothing", 0 );
					Event_AnimState(ANIMCHANNEL_LEGS, "Legs_DoNothing", 0 );
					Event_OverrideAnim( ANIMCHANNEL_LEGS );
					Event_PlayAnim( ANIMCHANNEL_TORSO, reaction->desc->anim );
					ProcessEvent(&AI_DisablePain);
					break;
				default:
					break;
			}
		}
	}
}

// 
// Event_FindReaction
// 
void hhMonsterAI::Event_FindReaction( const char* effect ) {
	idEntity* ent;
	hhReaction* react;
	hhReactionDesc::Effect react_effect;
	idEntity* bestEnt = NULL;
	float bestDistance = -1;
	int bestRank = -1, bestReactIndex = -1;

	react_effect = hhReactionDesc::StrToEffect( effect );

	if( react_effect == hhReactionDesc::Effect_Invalid ) {
		gameLocal.Warning( "unknown effect '%s' requested from FindReaction", effect );
		idThread::ReturnEntity( NULL );
		return;
	}

	for ( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if( !ent || ent->fl.isDormant || !ent->fl.refreshReactions ) {
			continue;
		}

		for( int j = 0; j < ent->GetNumReactions(); j++ ) {
			react = ent->GetReaction( j );
			if( !react || !react->IsActive() ) {
				continue;
			}
			if( react_effect != react->desc->effect ) {
				continue;
			}
			if( react->desc->flags & hhReactionDesc::flag_Exclusive ) { //check exclusiveness
				if ( react->exclusiveOwner.IsValid() && react->exclusiveOwner->health <= 0 ) {
					react->exclusiveOwner.Clear();
				}
				if( react->exclusiveOwner.GetEntity() && react->exclusiveOwner != this ) {
					continue;
				}
			}
			//Skip based on flag requirements
			if( (react->desc->flags & hhReactionDesc::flagReq_RangeAttack) && !AI_HAS_RANGE_ATTACK ) {
				continue;
			}
			// Skip monsters without melee attack
			if( (react->desc->flags & hhReactionDesc::flagReq_MeleeAttack) && !AI_HAS_MELEE_ATTACK ) {
				continue;
			}
			if( react->desc->flags & hhReactionDesc::flagReq_KeyValue ) {
				idStr val;
				if( spawnArgs.GetString(react->desc->key, "", val) ) {
					if( val != react->desc->keyVal ) {
						continue;
					}
				}
				else {
					continue;
				}
			}
			// Skip monsters without specific animation?
			if( react->desc->flags & hhReactionDesc::flagReq_Anim ) {
				if( !GetAnimator()->HasAnim(react->desc->anim) ) {
					continue;
				}
			}

			// Skip monsters in vehicles? 
			if( (react->desc->flags & hhReactionDesc::flagReq_NoVehicle) && InVehicle() ) {
				continue;
			}

			// Check actual specifics for reaction type
			switch( react->desc->effect ) {
				case hhReactionDesc::Effect_HaveFun:
				case hhReactionDesc::Effect_Vehicle:
				case hhReactionDesc::Effect_VehicleDock:
				case hhReactionDesc::Effect_Heal:
				case hhReactionDesc::Effect_ProvideCover:
				case hhReactionDesc::Effect_Climb:
				case hhReactionDesc::Effect_Passageway:
					break;
				case hhReactionDesc::Effect_DamageEnemy:
					if ( enemy.IsValid() && react->desc->effectRadius > 0 ) {
						float distSq = (enemy->GetOrigin() - react->causeEntity->GetOrigin()).LengthSqr();
						if ( distSq > react->desc->effectRadius * react->desc->effectRadius ) {
							continue;
						}
					}
					break;
				case hhReactionDesc::Effect_Damage:
					//jshtodo temp workaround for legacy stuff.  remove when old reaction system is removed
					if ( !ent->IsType( hhConsole::Type ) ) {
						continue;
					}
					break;
				default:
					gameLocal.Error( "effect '%s' not supported yet under simple_ai", hhReactionDesc::EffectToStr(react->desc->effect) );
			}
			// if we have actually gotten this far, do our intense calculations last..
			int rank = EvaluateReaction( react );
			if( rank == 0 ) {
				continue;
			}

// We have a valid reaction...
			float distSq = ( react->causeEntity->GetOrigin() - GetOrigin() ).LengthSqr();
			if ( bestRank == -1 || bestRank <= rank ) { // Check the reaction rank against our best rank  
				if (rank == bestRank && distSq > bestDistance) { // If they are the same rank, but the current one is farther then ignore it.
					continue;
				}
				bestEnt = ent;
				bestReactIndex = j;
				bestDistance = distSq;
				bestRank = rank;
			}
		}
	}

	if ( bestEnt ) {
		targetReaction.entity = bestEnt;
		targetReaction.reactionIndex = bestReactIndex;
		hhReaction *reaction = targetReaction.GetReaction();
		if( reaction && reaction->desc->flags & hhReactionDesc::flagReq_RangeAttack ) {
			shootTarget = bestEnt;
		} else {
			shootTarget = NULL;
		}
		idThread::ReturnEntity( targetReaction.entity.GetEntity() );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

//
// Event_AttackMissileEx()
//
void hhMonsterAI::Event_AttackMissileEx( const char *str, int boneDir ) {	
	const idDict *projDef = NULL;
	idList<idStr> parmList;

	hhUtils::SplitString( idStr(str), parmList, ' ' );

	if( animator.GetJointHandle(parmList[0].c_str()) == INVALID_JOINT ) {	
		return gameLocal.Error( "Event_AttackMissileEx: Joint '%s' not found", parmList[0].c_str() );		
	}

	if( parmList.Num() == 2 ) {
		projDef = gameLocal.FindEntityDefDict( parmList[1].c_str(), false );
	}
	
	Event_AttackMissile( parmList[0].c_str(), projDef, boneDir );
}

//HUMANHEAD jsh PCF 4/27/06 initialized proj and made sure ReturnEntity is called
void hhMonsterAI::Event_AttackMissile( const char *jointname, const idDict *projDef, int boneDir ) {
	idProjectile *proj = NULL;

	// Bonedir launch?
	if((BOOL)boneDir) {
		proj = hhProjectile::SpawnProjectile(projDef);
		if ( proj ) {
			idMat3 axis;
			idVec3 muzzle;
			GetMuzzle( jointname, muzzle, axis );
			proj->Create(this, muzzle, axis);
			proj->Launch(muzzle, axis, vec3_zero);		
		}
	}
	else {		
		if ( shootTarget.IsValid() ) {
			proj = LaunchProjectile( jointname, shootTarget.GetEntity(), true, projDef );	//HUMANHEAD mdc - pass projDef on for multiple proj support
		} else {
			proj = LaunchProjectile( jointname, enemy.GetEntity(), true, projDef );	//HUMANHEAD mdc - pass projDef on for multiple proj support
		}
	}
	idThread::ReturnEntity( proj );
}

void hhMonsterAI::Event_HitCheck( idEntity *ent, const char *animname ) {
	int		anim;
	idVec3	dir;
	idVec3	local_dir;
	idVec3	fromPos;
	idMat3	axis;
	idVec3	start;
	trace_t	tr;
	float	distance;

	if ( !AI_ENEMY_VISIBLE || !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		idThread::ReturnInt( false );
		return;
	}

	//// just do a ray test if close enough
	//if ( ent->GetPhysics()->GetAbsBounds().IntersectsBounds( physicsObj.GetAbsBounds().Expand( 16.0f ) ) ) {
	//	Event_CanHitEnemy();
	//	return;
	//}

	// calculate the world transform of the launch position
	const idVec3 &org = physicsObj.GetOrigin();
	dir = ent->GetOrigin() - org;
	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	axis = local_dir.ToMat3();
	fromPos = physicsObj.GetOrigin() + missileLaunchOffset[ anim ] * axis;

	if ( projectileClipModel == NULL ) {
		CreateProjectileClipModel();
	}

	// check if the owner bounds is bigger than the projectile bounds
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	const idBounds &projBounds = projectileClipModel->GetBounds();
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( org, viewAxis[ 0 ], distance ) ) {
			start = org + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, fromPos, projectileClipModel, mat3_identity, MASK_SHOT_RENDERMODEL, this );
	fromPos = tr.endpos;

	if ( GetAimDir( fromPos, ent, this, dir ) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

void hhMonsterAI::Event_CreateMonsterPortal(void) {
	static const char *	passPrefix = "portal_";
	const char *		portalDef;
	idEntity *			portal;
	idDict				portalArgs;
	idList<idStr>		xferKeys;
	idList<idStr>		xferValues;
	const idKeyValue *	buddyKV;


	// Find out which portal def to spawn - If none specified, then exit;
	buddyKV = spawnArgs.FindKey( "portal_buddy" );
	if ( buddyKV && buddyKV->GetValue().Length() && gameLocal.FindEntity(buddyKV->GetValue().c_str()) ) {
		// Case of a valid portal_buddy key, make a real portal
		portalDef = spawnArgs.GetString( "def_portal" );
	} else {
		portalDef = spawnArgs.GetString( "def_fakeportal" );
	}

	if ( !portalDef || !portalDef[0] ) {
		return;	
	}

	// Set the origin of the portal to us. 
	portalArgs.SetVector( "origin", GetOrigin() );

	// Pass along any angle key, if set.
	if ( spawnArgs.GetBool( "portal_face" ) && enemy.IsValid() ) {
		portalArgs.SetInt( "angle", (enemy->GetOrigin() - GetOrigin() ).ToAngles().yaw );
		ideal_yaw = ( enemy->GetOrigin() - GetOrigin() ).ToAngles().yaw;
		current_yaw = ideal_yaw;
	} else if ( spawnArgs.GetString( "angle", NULL) ) {
		portalArgs.Set( "angle", spawnArgs.GetString( "angle" ) );
	} else {
		portalArgs.Set( "rotation", spawnArgs.GetString( "rotation" ) );
	}

	// Pass along all 'portal_' keys to the portal's spawnArgs;
	hhUtils::GetKeysAndValues( spawnArgs, passPrefix, xferKeys, xferValues );
	for ( int i = 0; i < xferValues.Num(); ++i ) {
		xferKeys[ i ].StripLeadingOnce( passPrefix );
		//gameLocal.Printf( "Passing %s => %s\n", xferKeys[ i ].c_str(), xferValues[ i ].c_str() );
		portalArgs.Set( xferKeys[ i ].c_str(), xferValues[ i ].c_str() );
	}

	// Set the name of the associated game portal so it can be turned on and off
	portalArgs.Set( "gamePortalName", GetName() );

	// Spawn the portal
	portal = gameLocal.SpawnObject( portalDef, &portalArgs );
	if ( !portal ) {
		return;
	}

	// Move the portal up some pre determinted amt, since its origin is in the middle of it
	float offset = spawnArgs.GetFloat( "offset_portal", 0 );
	portal->GetPhysics()->SetOrigin( portal->GetPhysics()->GetOrigin() + (portal->GetAxis()[2] * offset) );

	// Move the portal by some offset vector
	idVec3 vectorOffset = spawnArgs.GetVector( "offset_portal_vector", "0 0 0" );
	portal->GetPhysics()->SetOrigin( portal->GetPhysics()->GetOrigin() + (portal->GetAxis() * vectorOffset ) );

	// Update the camera stuff
	portal->ProcessEvent( &EV_UpdateCameraTarget );

	// Open the portal - Need to delay this, so that PostSpawn gets called/sets up the partner portal
	//? Should we always pass in the player?
	portal->PostEventSec( &EV_Activate, 0, gameLocal.GetLocalPlayer() );
	// Maybe wait for it to finish?
	
		
}

void hhMonsterAI::Event_GetShootTarget() {
	idThread::ReturnEntity( shootTarget.GetEntity() );
}

void hhMonsterAI::Event_TriggerReactEnt(void) {
	if ( targetReaction.entity.IsValid() ) {
		targetReaction.entity->PostEventMS( &EV_Activate, 0, this );
	}
}

void hhMonsterAI::Event_GetVehicle() {
	idThread::ReturnEntity( vehicleInterface->GetVehicle() );
}

void hhMonsterAI::Event_ReachedEntity( idEntity *ent ) {
	if ( ent ) {
		idVec3 pos = ent->GetPhysics()->GetOrigin();
		if ( physicsObj.GetAbsBounds().IntersectsBounds( idBounds( pos ).Expand( 8.0f ) ) ) {
			idThread::ReturnInt( true );
			return;
		}
	}

	idThread::ReturnInt( false );
}

void hhMonsterAI::Event_EnemyOnSpawn(void)
{
    idEntity *ent = NULL;
	idStr entName;
	if(spawnArgs.GetString("enemy_on_spawn"," ", entName))
	{
		if ( entName == idStr("closest_player") || entName == idStr("players_in_view") ) {
			ent = GetClosestPlayer();
		}
		if ( !ent ) {
			ent = gameLocal.FindEntity(entName);
		}
		if( ent && ent->IsType(idActor::Type) ) {
			SetEnemy( static_cast<idActor*>(ent) );
		}		
	}
}

void hhMonsterAI::Event_SpawnFX( char *fxFile ) {

  if ( !fxFile || !fxFile[0] ) {
    return;
  }

  idVec3		offset;
  hhFxInfo		fxInfo;
	
  offset = spawnArgs.GetVector( "particle_offset", "0 0 0" );

  fxInfo.RemoveWhenDone( true );
  BroadcastFxInfo( fxFile, GetOrigin() + offset * GetAxis(), GetAxis(), &fxInfo );

}

void hhMonsterAI::Event_SplashDamage(char *damage) {
	  
	if(!damage || !damage[0])
		return;

	idStr splash_damage;	

	// Need to set takedamage to false in order to prevent infinite loop!
	//fl.takedamage = false;
	splash_damage = spawnArgs.GetString(damage);
	if ( splash_damage.Length() ) {
		gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), NULL, NULL, this, this, splash_damage );
	}	
}

void hhMonsterAI::Event_FollowPath( const char *pathName ) {
	if ( scriptObject.HasObject() ) {
		const function_t	*func;
		idThread			*thread;

		func = scriptObject.GetFunction( "follow_alternate_path" );
		if ( !func ) {
			gameLocal.Error( "Function 'follow_alternate_path' not found on entity '%s' for function call from '%s'", name.c_str(), name.c_str() );
		}
		if ( func->type->NumParameters() != 1 ) {
			gameLocal.Error( "Function 'follow_alternate_path' on entity '%s' has the wrong number of parameters for function call from '%s'", name.c_str(), name.c_str() );
		}
		if ( !scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
			gameLocal.Error( "Function 'follow_alternate_path' on entity '%s' is the wrong type for function call from '%s'", name.c_str(), name.c_str() );
		}
		spawnArgs.Set( "alt_path", pathName );
		// create a thread and call the function
		thread = new idThread();
		thread->CallFunction( this, func, true );
		thread->Start();
	}
}

void hhMonsterAI::Event_SetVehicleState() {
	const function_t *newstate = NULL;
	if ( GetVehicleInterface()->UnderScriptControl() ) {
		newstate = GetScriptFunction( "state_Nothing" );			
	}
	if ( newstate ) {
		SetState( newstate );
	}
}

void hhMonsterAI::Event_Subtitle( idList<idStr>* parmList ) {
	if ( !parmList || !parmList->Num() ) {
		return;
	}
	idPlayer *player = gameLocal.GetLocalPlayer();
	player->hud->SetStateInt("subtitlex", 0 );
	player->hud->SetStateInt("subtitley", 400 );
	player->hud->SetStateInt("subtitlecentered", true);
	player->hud->SetStateString("subtitletext", common->GetLanguageDict()->GetString( (*parmList)[0].c_str() ));
	player->hud->StateChanged(gameLocal.time);
	player->hud->HandleNamedEvent("DisplaySubtitle");

	CancelEvents(&MA_SubtitleOff);
	if ( parmList->Num() == 1 ) {
		PostEventSec(&MA_SubtitleOff, (float)atof((*parmList)[1].c_str()));
	} else {
		PostEventSec(&MA_SubtitleOff, 2.0);
	}
}

void hhMonsterAI::Event_SubtitleOff() {
	idPlayer *player = gameLocal.GetLocalPlayer();
	player->hud->HandleNamedEvent("RemoveSubtitleInstant");
}

/*
=====================
hhMonsterAI::Event_GetLastReachableEnemyPos
=====================
*/
void hhMonsterAI::Event_GetLastReachableEnemyPos() {
	idThread::ReturnVector( lastReachableEnemyPos );
}

void hhMonsterAI::Event_EnableEyelook() {
	allowEyeFocus = true;
}

void hhMonsterAI::Event_DisableEyelook() {
	allowEyeFocus = false;
}

void hhMonsterAI::Event_EnableHeadlook() {
	allowJointMod = true;
}

void hhMonsterAI::Event_DisableHeadlook() {
	allowJointMod = false;
}

void hhMonsterAI::Event_FacingEnemy( float range ) {
	if ( FacingEnemy( range ) ) {
		idThread::ReturnInt( 1 );
	} else {
		idThread::ReturnInt( 0 );
	}
}

bool hhMonsterAI::FacingEnemy( float range ) {
	if ( !enemy.IsValid() ) {
		return false;
	}

	if ( !turnRate ) {
		return true;
	}

	idVec3 dir;
	idVec3 local_dir;
	float lengthSqr;
	float local_yaw;

	dir = enemy->GetOrigin() - physicsObj.GetOrigin();
	physicsObj.GetAxis().ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	lengthSqr = local_dir.LengthSqr();
	local_yaw = idMath::AngleNormalize180( local_dir.ToYaw() );

	float diff;

	diff = idMath::AngleNormalize180( current_yaw - local_yaw );
	return ( idMath::Fabs( diff ) < range );
}

void hhMonsterAI::Event_AllowFall( int allowFall ) {
	bCanFall = allowFall != 0;
}

void hhMonsterAI::Event_BossBar( int onOff ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( !player || !player->hud ) {
		return;
	}

	if ( bBossBar ) {
		if ( !onOff ) {
			player->hud->HandleNamedEvent("HideProgressBar");
			player->hud->StateChanged(gameLocal.time);
			bBossBar = false;
		}
	} else {
		if ( onOff ) {
			player->hud->HandleNamedEvent("ShowProgressBar");
			player->hud->SetStateBool( "progressbar", true );
			bBossBar = true;
		}
	}
}

void hhMonsterAI::Event_FallNow() {
	SetGravity( DEFAULT_GRAVITY_VEC3 );
	GetPhysics()->SetLinearVelocity( idVec3( 0,0,-500 ) );
}

//taken from idEntity::Event_PlayerCanSee and removed trace
void hhMonsterAI::Event_InPlayerFov() {
	int			i;
	idEntity	*ent;
	hhPlayer	*player;
	trace_t		traceInfo;
	bool		result = false;

	// Check if this entity is in the player's PVS
	if ( gameLocal.InPlayerPVS( this ) ) {
		for ( i = 0; i < gameLocal.numClients ; i++ ) {
			ent = gameLocal.entities[ i ];

			if ( !ent || !ent->IsType( hhPlayer::Type ) ) {
				continue;
			}

			// Get the player
			player = static_cast<hhPlayer *>( ent );

			// Check if the entity is in the player's FOV, based upon the "fov" key/value
			if ( player->CheckYawFOV( this->GetOrigin() ) ) {
				result = true;
			}
		}
	}

	if ( result ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

void hhMonsterAI::Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy ) {
	HH_ASSERT( player == enemy.GetEntity() );
	//HUMANHEAD jsh PCF 4/29/06 stop looking and make enemy not visible for a frame
	Event_LookAtEntity( NULL, 0.0f );
	AI_ENEMY_VISIBLE = false;
	enemy = proxy;
}

void hhMonsterAI::Event_EnemyIsPhysical( hhPlayer *player, hhSpiritProxy *proxy ) {
	// If we weren't targetting the physical player before, and we can't see them now,
	// lose target
	//HUMANHEAD jsh PCF 4/29/06 stop looking and make enemy not visible for a frame
	Event_LookAtEntity( NULL, 0.0f );
	AI_ENEMY_VISIBLE = false;
	if ( !proxy && !CanSee( player, true ) ) {
		enemy = NULL;
	} else {
		enemy = player;
	}
}

void hhMonsterAI::Event_IsRagdoll() {
	if ( af.IsActive() ) {
		idThread::ReturnInt( true );
		return;
	}
	idThread::ReturnInt( false );
}

void hhMonsterAI::Event_MoveDone() {
	if ( AI_MOVE_DONE ) {
		idThread::ReturnInt( true );
		return;
	}
	idThread::ReturnInt( false );
}

void hhMonsterAI::Event_SetShootTarget(idEntity *ent) {
	shootTarget = ent;
}

void hhMonsterAI::Event_LookAtEntity( idEntity *ent, float duration ) {
	if ( ent == this ) {
		ent = NULL;
	}

	if ( ( ent != focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity	= ent;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	if ( duration == -1 ) {
		focusTime = -1;
	} else {
		focusTime = gameLocal.time + SEC2MS( duration );
	}
}

void hhMonsterAI::Event_LookAtEnemy( float duration ) {
	idActor *enemyEnt;

	enemyEnt = enemy.GetEntity();
	if ( ( enemyEnt != focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity	= enemyEnt;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	if ( duration == -1 ) {
		focusTime = -1;
	} else {
		focusTime = gameLocal.time + SEC2MS( duration );
	}
}

void hhMonsterAI::Event_SetLookOffset( idAngles const &ang ) {
	lookOffset = ang;
}

void hhMonsterAI::Event_SetHeadFocusRate( float rate ) {
	headFocusRate = rate;
}

void hhMonsterAI::Event_EnemyInGravityZone(void) {
	if ( !enemy.IsValid() ) {
		idThread::ReturnFloat( 1.0f );
		return;
	}
	idThread::ReturnFloat( enemy->InGravityZone() );
}

void hhMonsterAI::Event_FlyZip() {
	//if enemy is real far away, instantly teleport to a spot near him
	if ( !enemy.IsValid() || move.moveType != MOVETYPE_FLY ) {
		return;
	}

	//find a good spot to try and teleport to
	float distance = spawnArgs.GetFloat( "zip_range", "700" );
	idVec3 testPoint;
	idVec3 finalPoint = vec3_zero;
	bool clipped = false;
	float yaw = (GetOrigin() - enemy->GetOrigin()).ToYaw();
	idBounds bounds;
	idVec3 size;
	bounds.Zero();
	if ( spawnArgs.GetVector( "mins", NULL, bounds[0] ) &&
		spawnArgs.GetVector( "maxs", NULL, bounds[1] ) ) {
	} else if ( spawnArgs.GetVector( "size", NULL, size ) ) {
		bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
		bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
	} else { // Default bounds
		bounds.Expand( 1.0f );
	}	

	//test 8 points around the enemy, starting with one directly in front of it
	for ( int i=0;i<8;i++ ) {
		testPoint = enemy->GetOrigin() + distance * idAngles( 0, yaw, 0 ).ToForward();
		yaw += 45;
		if ( yaw > 360 ) {
			yaw -= 360;
		}

		int contents = hhUtils::ContentsOfBounds( bounds, testPoint, mat3_identity, this);
		if ( contents & CONTENTS_MONSTERCLIP ) {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugArrow( colorRed, GetOrigin(), testPoint, 10, 999999 );
			}
			continue;
		}

		//make sure we can path there
		int toAreaNum = PointReachableAreaNum( testPoint );
		int areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		aasPath_t	path;
		if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, testPoint ) ) {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugArrow( colorRed, GetOrigin(), testPoint, 10, 10000 );
			}
			continue;
		}

		//passed all tests use this point
		finalPoint = testPoint;
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorGreen, GetOrigin(), testPoint, 10, 10000 );
		}
		break;
	}

	if ( finalPoint != vec3_zero ) {
		//do the actual teleport
		GetPhysics()->SetOrigin( testPoint + idVec3( 0, 0, CM_CLIP_EPSILON ) );
		GetPhysics()->SetLinearVelocity( vec3_origin );
		viewAxis = (enemy->GetOrigin() - GetOrigin()).ToMat3();
		UpdateVisuals();
	}
}

void hhMonsterAI::Event_UseConsole( idEntity *ent ) {
	if ( !ent || !ent->IsType(hhConsole::Type) ) {
		return;
	}
	hhConsole *cons = static_cast<hhConsole*>(ent);
	if(!cons->CanUse(this)) {
		return;
	}					
	cons->Use(this);
}

void hhMonsterAI::Event_TestMeleeDef( const char *meleeDefName ) const {
	bool canMelee = TestMeleeDef( meleeDefName );

	if ( canMelee ) {
		idThread::ReturnFloat( 1.0f );
	} else {
		idThread::ReturnFloat( 0.0f );
	}
}

void hhMonsterAI::Event_EnemyInVehicle() {
	if ( enemy.IsValid() && enemy->InVehicle() ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

void hhMonsterAI::Event_HeardSound( int ignore_team ) {
	// overridden to use hearingRange instead of AI_HEARING_RANGE
	// check if we heard any sounds in the last frame
	idActor	*actor = gameLocal.GetAlertEntity();
	if ( actor && ( !ignore_team || ( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) && gameLocal.InPlayerPVS( this ) ) {
		idVec3 pos = actor->GetPhysics()->GetOrigin();
		idVec3 org = physicsObj.GetOrigin();
		float dist = ( pos - org ).LengthSqr();
		//allow the sound's own radius to override hearingRange, if it is set
		if ( gameLocal.lastAIAlertRadius ) {
			if ( dist < Square( gameLocal.lastAIAlertRadius ) ) {
				idThread::ReturnEntity( actor );
				return;
			}
		} else if ( dist < Square( hearingRange ) ) {
			idThread::ReturnEntity( actor );
			return;
		}
	}
}

void hhMonsterAI::Event_AlertAI( idEntity *ent, float radius ) {
	if ( !ent || radius <= 0 ) {
		return;
	}
	gameLocal.AlertAI( ent, radius );
}

void hhMonsterAI::Event_EnemyOnWallwalk() {
	if ( enemy.IsValid() && enemy->IsType( hhPlayer::Type ) ) {
		hhPlayer *player = static_cast<hhPlayer*>(enemy.GetEntity());
		if ( player && player->IsWallWalking() ) {
			idThread::ReturnInt( true );
			return;
		}
	}

	idThread::ReturnInt( false );
}

/*
=====================
hhMonsterAI::Event_TestAnimMoveBlocked
=====================
*/
void hhMonsterAI::Event_TestAnimMoveBlocked( const char *animname ) {
	int				anim;
	predictedPath_t path;
	idVec3			moveVec;

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		idThread::ReturnInt( false );
		return;
	}

	moveVec = animator.TotalMovementDelta( anim ) * idAngles( 0.0f, ideal_yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
	}

	HH_ASSERT( path.endEvent == 0 && !path.blockingEntity || path.blockingEntity );

	idThread::ReturnEntity( const_cast<idEntity *> ( path.blockingEntity ) );
}

void hhMonsterAI::Event_InGravityZone() {
	idThread::ReturnFloat( InGravityZone() );
}

void hhMonsterAI::Event_StartSoundDelay( const char *soundName, int channel, int netSync, float delay ) {
	if ( delay < 0 ) {
		delay = 0;
	}
	PostEventSec( &EV_StartSound, delay, soundName, channel, netSync );
}

void hhMonsterAI::Event_SetTeam( int new_team ) {
	team = new_team;
}

void hhMonsterAI::Event_HideNoDormant() {
	HideNoDormant();
}

void hhMonsterAI::Event_GetAttackPoint( void ) {
	//if enemy is real far away, instantly teleport to a spot near him
	if ( !enemy.IsValid() ) {
		idThread::ReturnVector( vec3_zero );
		return;
	}

	//find a good spot to attack from
	float distance = spawnArgs.GetFloat( "attack_range", "500" );
	idVec3 testPoint;
	idVec3 finalPoint = vec3_zero;
	bool clipped = false;
	float yaw = (GetOrigin() - enemy->GetOrigin()).ToYaw();
	int num, i, j;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];
	idBounds bounds;

	//test 8 points around the enemy, starting with one directly in front of it
	idList<idVec3> listo;
	for ( i=0;i<8;i++ ) {
		testPoint = enemy->GetOrigin() + distance * idAngles( 0, yaw, 0 ).ToForward();
		testPoint.z += spawnArgs.GetFloat( "attack_z", "300" );
		yaw += 45;
		if ( yaw > 360 ) {
			yaw -= 360;
		}
		if ( yaw == 180 ) {
			continue;
		}

		//make sure it wont clip into anything at testPoint
		clipped = false;
		bounds.FromTransformedBounds( GetPhysics()->GetBounds(), testPoint, GetPhysics()->GetAxis() );
		num = gameLocal.clip.ClipModelsTouchingBounds( bounds, MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
		for ( j = 0; j < num; j++ ) {
			cm = clipModels[ j ];
			// don't check render entities
			if ( cm->IsRenderModel() ) {
				continue;
			}
			idEntity *hit = cm->GetEntity();
			if ( ( hit == this ) || !hit->fl.takedamage ) {
				continue;
			}
			if ( physicsObj.ClipContents( cm ) ) {
				clipped = true;
				break;
			}
		}
		if ( clipped ) {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugBounds( colorRed, bounds, vec3_origin, 10000 );
			}
			continue;
		} else {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugBounds( colorGreen, bounds, vec3_origin, 10000 );
			}
		}

		//make sure we can path there
		if ( !PointReachableAreaNum( testPoint ) ) {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugArrow( colorRed, GetOrigin(), testPoint, 10, 10000 );
			}
			continue;
		} else if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorGreen, GetOrigin(), testPoint, 10, 10000 );
		}

		listo.Append( testPoint );
	}

	if ( listo.Num() ) {
		finalPoint = listo[gameLocal.random.RandomInt(listo.Num())];
	}
	if ( finalPoint != vec3_zero ) {
		idThread::ReturnVector( finalPoint );
		return;
	} else {
		idThread::ReturnVector( vec3_zero );
		return;
	}
}

void hhMonsterAI::Event_SoundOnModel() {
	soundOnModel = !soundOnModel;
}

void hhMonsterAI::Event_ActivatePhysics() {
	ActivatePhysics( this );
}

void hhMonsterAI::Event_IsVehicleDocked() {
	if ( GetVehicleInterfaceLocal()->IsVehicleDocked() ) {
		idThread::ReturnInt( true );
		return;
	}
	idThread::ReturnInt( false );
}

void hhMonsterAI::Event_EnemyInSpirit() {
	if ( enemy.IsValid() && enemy->IsType( hhPlayer::Type ) ) {
		hhPlayer *player = static_cast<hhPlayer *>( enemy.GetEntity() );
		if ( player && player->IsSpiritWalking() ) {
			idThread::ReturnInt( true );
			return;
		}
	}

	idThread::ReturnInt( false );
}

void hhMonsterAI::Event_SetNeverDormant( int enable ) {
	if ( head.IsValid() ) {
		head->fl.neverDormant = ( enable != 0 );
	}
	fl.neverDormant	= ( enable != 0 );
	dormantStart = 0;
}

