#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef MH_AlertFriends("alertFriends", NULL, NULL);
const idEventDef MH_DropBinds("dropBinds");
const idEventDef MH_DropProjectiles("<dropProjectiles>" );

CLASS_DECLARATION(hhMonsterAI, hhMutilatedHuman)
	EVENT( MH_AlertFriends,			hhMutilatedHuman::Event_AlertFriends )
	EVENT( MH_DropBinds,			hhMutilatedHuman::Event_DropBinds )
	EVENT( MH_DropProjectiles,		hhMutilatedHuman::Event_DropProjectiles )
END_CLASS

void hhMutilatedHuman::Spawn() {
	damageFlag = 0;
}

void hhMutilatedHuman::Event_DropBinds( ) {
	idEntity *ent;
	idEntity *next;
	for( ent = teamChain; ent != NULL; ent = next ) {
		next = ent->GetTeamChain();
		if ( ent && !ent->IsType( hhProjectile::Type ) && !ent->IsType( idEntityFx::Type ) && ent->GetBindMaster() == this && ent != head.GetEntity() ) {
			ent->Unbind();
			next = teamChain;
		}
	}
}

void hhMutilatedHuman::Event_DropProjectiles() {
	idEntity *ent;
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
}

void hhMutilatedHuman::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if (!fl.takedamage) {
		return;
	}
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );

	if ( AI_LIMB_FELL ) {
		PostEventSec( &MH_DropProjectiles, 0.1f );
	}

	if ( AI_DEAD ) {
		return;
	}

	idDict args;
	idVec3 bonePos;
	idMat3 boneAxis;
	idStr debrisName;
	idStr damageGroup = GetDamageGroup( location );

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( damageDef && damageDef->GetBool( "ice" ) ) {
		return;
	}

	if ( !( damageGroup == "head" && damageDef->GetInt( "damage" ) >= spawnArgs.GetFloat( "head_falloff_damage", "70" ) ) ) {
		if ( gameLocal.random.RandomFloat() >= spawnArgs.GetFloat( "chanceLimbsWillFallOff", "0.1" ) ) { // CJR: Build in a chance that the limbs will fall off
			return;
		}
	}

	if ( damageGroup == "left_arm" && !(damageFlag & BIT(0)) ) {
		damageFlag |= BIT(0);
		debrisName = spawnArgs.GetString( "def_debris_arm" );
		if ( !debrisName.IsEmpty() ) {
			idEntity *debris = gameLocal.SpawnObject( debrisName, &args );
			if ( debris ) {
				AI_LIMB_FELL = true;
				GetJointWorldTransform( spawnArgs.GetString( "bone_arm_left" ), bonePos, boneAxis );
				debris->SetOrigin( bonePos );
				debris->GetPhysics()->SetLinearVelocity( spawnArgs.GetVector( "debris_arm_velocity" ) * GetAxis() );
				debris->GetPhysics()->SetAngularVelocity( 60 * hhUtils::RandomVector() );
				BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_left_arm_blood"), spawnArgs.GetString("bone_arm_left") );
			}
		}

	} else if ( damageGroup == "right_arm" && !(damageFlag & BIT(1)) ) {
		damageFlag |= BIT(1);
		debrisName = spawnArgs.GetString( "def_debris_arm" );
		if ( !debrisName.IsEmpty() ) {
			idEntity *debris = gameLocal.SpawnObject( debrisName, &args );
			if ( debris ) {
				AI_LIMB_FELL = true;
				GetJointWorldTransform( spawnArgs.GetString( "bone_arm_right" ), bonePos, boneAxis );
				debris->SetOrigin( bonePos );
				debris->GetPhysics()->SetLinearVelocity( spawnArgs.GetVector( "debris_arm_velocity" ) * GetAxis() );
				debris->GetPhysics()->SetAngularVelocity( 60 * hhUtils::RandomVector() );
				BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_right_arm_blood"), spawnArgs.GetString("bone_arm_right") );
			}
		}
	} else if ( damageGroup == "chest" && !(damageFlag & BIT(2)) ) {
		damageFlag |= BIT(2);
		damageFlag |= BIT(3);
		debrisName = spawnArgs.GetString( "def_debris_chest" );
		if ( !debrisName.IsEmpty() ) {
			idEntity *debris = gameLocal.SpawnObject( debrisName, &args );
			if ( debris ) {
				AI_LIMB_FELL = true;
				GetJointWorldTransform( spawnArgs.GetString( "bone_chest" ), bonePos, boneAxis );
				debris->SetOrigin( bonePos );
				debris->GetPhysics()->SetLinearVelocity( spawnArgs.GetVector( "debris_chest_velocity" ) * GetAxis() );
				debris->GetPhysics()->SetAngularVelocity( 60 * hhUtils::RandomVector() );
				BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_chest_blood"), spawnArgs.GetString("bone_chest") );
			}
		}
	} else if ( damageGroup == "head" ) {
		if ( head.IsValid() && !head->IsHidden() ) {
			debrisName = spawnArgs.GetString( "def_debris_head" );
			if ( !debrisName.IsEmpty() ) {
				idEntity *debris = gameLocal.SpawnObject( debrisName, &args );
				if ( debris ) {
					AI_LIMB_FELL = true;
					GetJointWorldTransform( spawnArgs.GetString( "bone_head" ), bonePos, boneAxis );
					debris->SetOrigin( bonePos );
					debris->GetPhysics()->SetLinearVelocity( spawnArgs.GetVector( "debris_head_velocity" ) * GetAxis() );
					debris->GetPhysics()->SetAngularVelocity( 60 * hhUtils::RandomVector() );
					BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_head_blood"), spawnArgs.GetString("bone_head") );
				}
			}
			head->Hide();
		}
	}

	idStr skinName = spawnArgs.GetString("damage_skin");
	skinName += damageFlag;
	if ( damageFlag > 0 ) {
		SetSkin( declManager->FindSkin( skinName ) );
		UpdateVisuals();
	}
}

void hhMutilatedHuman::Event_AlertFriends() {
	hhMonsterAI *check;
	if ( !enemy.IsValid() ) {
		return;
	}

	for( int i = 0; i < targets.Num(); i++ ) {
		check = static_cast<hhMonsterAI *>(targets[ i ].GetEntity());
		if ( !check || check->IsHidden() || !check->IsType( hhMutilatedHuman::Type ) ) {
			continue;
		}
		check->SetEnemy( enemy.GetEntity() );
	}
}

void hhMutilatedHuman::AnimMove( void ) {
	//overridden to set enemy if blockedEnt can be an enemy
	idVec3				goalPos;
	idVec3				delta;
	idVec3				goalDelta;
	float				goalDist;
	monsterMoveResult_t	moveResult;
	idVec3				newDest;

	idVec3 oldorigin = physicsObj.GetOrigin();
#ifdef HUMANHEAD //jsh wallwalk
	idMat3 oldaxis = GetGravViewAxis();
#else
	idMat3 oldaxis = viewAxis;
#endif

	AI_BLOCKED = false;

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS ){ 
		move.lastMoveOrigin.Zero();
		move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;
	if ( ( move.moveCommand == MOVE_FACE_ENEMY ) && enemy.GetEntity() ) {
		TurnToward( lastVisibleEnemyPos );
		goalPos = oldorigin;
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		goalPos = oldorigin;
	} else if ( GetMovePos( goalPos ) ) {
		if ( move.moveCommand != MOVE_WANDER ) {
			CheckObstacleAvoidance( goalPos, newDest );
			TurnToward( newDest );
		} else {
			TurnToward( goalPos );
		}
	}
		
	Turn();	

	if ( move.moveCommand == MOVE_SLIDE_TO_POSITION ) {
		if ( gameLocal.time < move.startTime + move.duration ) {
			goalPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
			delta = goalPos - oldorigin;
			delta.z = 0.0f;
		} else {
			delta = move.moveDest - oldorigin;
			delta.z = 0.0f;
			StopMove( MOVE_STATUS_DONE );
		}
	} else if ( allowMove ) {
#ifdef HUMANHEAD //jsh wallwalk
		GetMoveDelta( oldaxis, GetGravViewAxis(), delta );
#else
		GetMoveDelta( oldaxis, viewAxis, delta );
#endif
	} else {
		delta.Zero();
	}

	if ( move.moveCommand == MOVE_TO_POSITION ) {
		goalDelta = move.moveDest - oldorigin;
		goalDist = goalDelta.LengthFast();
		if ( goalDist < delta.LengthFast() ) {
			delta = goalDelta;
		}
	}
	
	physicsObj.SetDelta( delta );
	physicsObj.ForceDeltaMove( disableGravity );

	RunPhysics();

	if ( ai_debugMove.GetBool() ) {
		// HUMANHEAD JRM - so we can see if grav is on or off
		if(disableGravity) {
			gameRenderWorld->DebugLine( colorRed, oldorigin, physicsObj.GetOrigin(), 5000 );
		} else {
			gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
		}
	}

	moveResult = physicsObj.GetMoveResult();
	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( idActor::Type ) && ReactionTo( blockEnt ) != ATTACK_IGNORE ) {
			SetEnemy( static_cast<idActor*>(blockEnt) );
		}
		if ( blockEnt && blockEnt->IsType( hhPod::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
			blockEnt->Damage( this, this, vec3_zero, spawnArgs.GetString( "kick_damage" ), 1.0f, INVALID_JOINT );
		}
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		}
	}

	BlockedFailSafe();

	AI_ONGROUND = physicsObj.OnGround();

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
		DrawRoute();
	}
}

int hhMutilatedHuman::ReactionTo( const idEntity *ent ) {
	const idActor *actor = static_cast<const idActor *>( ent );
	if( actor && actor->IsType(hhDeathProxy::Type) ) {
		return ATTACK_IGNORE;
	}

	if ( ent->health <= 0 ) {
		return ATTACK_IGNORE;
	}

	if ( ent->fl.hidden ) {
		// ignore hidden entities
		return ATTACK_IGNORE;
	}

	if ( !ent->IsType( idActor::Type ) ) {
		return ATTACK_IGNORE;
	}

	actor = static_cast<const idActor *>( ent );
	if ( actor->IsType( idPlayer::Type ) && static_cast<const idPlayer *>(actor)->noclip ) {
		// ignore players in noclip mode
		return ATTACK_IGNORE;
	}

	//only attack spiritwalking players if they hurt me
	if ( ent->IsType( hhPlayer::Type ) ) {
		const hhPlayer *player = static_cast<const hhPlayer*>( ent );
		if ( nextSpiritProxyCheck == 0 && player && player->IsSpiritWalking() ) {
			nextSpiritProxyCheck = gameLocal.time + SEC2MS(2);
			return ATTACK_ON_DAMAGE;
		}
	}

	if ( ent->IsType( hhSpiritProxy::Type ) ) {
		if ( gameLocal.time > nextSpiritProxyCheck ) {
			nextSpiritProxyCheck = 0;
			//attack spiritproxy on sight if we have no enemy or if its closer than our current enemy
			if ( enemy.IsValid() && enemy->IsType( hhPlayer::Type ) ) {
				float distToEnemy = (enemy->GetOrigin() - GetOrigin()).LengthSqr();
				float distToProxy = (ent->GetOrigin() - GetOrigin()).LengthSqr();
				if ( distToProxy < distToEnemy ) {
					return ATTACK_ON_SIGHT;	
				}
			} else {
				return ATTACK_ON_SIGHT;				
			}
		} else {
			return ATTACK_IGNORE;
		}
	}

	if ( ent && ent->spawnArgs.GetBool( "never_target" ) ) {
		return ATTACK_IGNORE;
	}

	//if we're hurt, its probably because of the player, so keep him as an enemy
	if ( actor->IsType( idPlayer::Type ) && health < spawnHealth ) {
		return ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE | ATTACK_ON_SIGHT;
	}

	// actors on different teams will always fight each other
	if ( actor->team != team ) {
		if ( actor->fl.notarget ) {
			// don't attack on sight when attacker is notargeted
			if ( actor->IsType( idPlayer::Type ) ) {
				return ATTACK_ON_DAMAGE;
			} else {
				return ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE;
			}
		}
		//force players to only alert through damage
		if ( actor->IsType( idPlayer::Type ) ) {
			return ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE;
		} else if ( actor->IsType( idAI::Type ) ) {
			//only allow ai actors to otherwise alert them
			return ATTACK_ON_SIGHT | ATTACK_ON_DAMAGE | ATTACK_ON_ACTIVATE;
		}
	}

	// monsters will fight when attacked by lower ranked monsters.  rank 0 never fights back.
	if ( rank && ( actor->rank < rank ) ) {
		return ATTACK_ON_DAMAGE;
	}

	// don't fight back
	return ATTACK_IGNORE;
}

/*
=====================
hhMutilatedHuman::Save
=====================
*/
void hhMutilatedHuman::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( damageFlag );
}

/*
=====================
hhMutilatedHuman::Restore
=====================
*/
void hhMutilatedHuman::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( damageFlag );
}

#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhMutilatedHuman::LinkScriptVariables(void) {
	hhMonsterAI::LinkScriptVariables();
	LinkScriptVariable( AI_LIMB_FELL );
}