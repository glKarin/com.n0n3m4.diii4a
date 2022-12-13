

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_UpdateTarget( "<updateTarget>" );
const idEventDef EV_DirectMoveToPosition("directMoveToPosition", "v" );
const idEventDef MA_GetCircleNode("getCircleNode", NULL, 'e' );
const idEventDef MA_SpinClouds("spinClouds", "f" );
const idEventDef MA_SetSeekScale("setSeekScale", "f" );

CLASS_DECLARATION(hhMonsterAI, hhSphereBoss)
	EVENT( EV_UpdateTarget,			hhSphereBoss::Event_UpdateTarget )
	EVENT( EV_DirectMoveToPosition, hhSphereBoss::Event_DirectMoveToPosition )
	EVENT( MA_GetCircleNode,		hhSphereBoss::Event_GetCircleNode )
	EVENT( MA_SpinClouds,			hhSphereBoss::Event_SpinClouds )
	EVENT( MA_SetSeekScale,			hhSphereBoss::Event_SetSeekScale )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

void hhSphereBoss::Spawn() {
	for ( int i=0;i<10;i++ ) {
		lastTargetPos.Append( vec3_zero );
	}
	nextShieldImpact = 0;
	lastNodeIndex = -1;
	PostEventSec( &EV_UpdateTarget, spawnArgs.GetFloat( "target_period", "0.1" ) );
}

void hhSphereBoss::Event_UpdateTarget() {
	for ( int i=0;i<10;i++ ) {
		if ( i == 9 ) {
			if ( enemy.IsValid() ) {
				lastTargetPos[i] = enemy->GetOrigin();
			} else {
				lastTargetPos[i] = lastTargetPos[i - 1];
			}
		} else {
			lastTargetPos[i] = lastTargetPos[i + 1];
		}
	}
	PostEventSec( &EV_UpdateTarget, spawnArgs.GetFloat( "target_period", "0.1" ) );
}

//overridden to allow custom accuracy/numbers per projectile def
idProjectile *hhSphereBoss::LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef ) {
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

	if ( projectileDef->GetFloat( "attack_accuracy" ) ) {
		attack_accuracy = projectileDef->GetFloat( "attack_accuracy", "7" );
	} else {
		attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	}
	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	if ( projectileDef->GetFloat( "projectile_spread" ) ) {
		projectile_spread = projectileDef->GetFloat( "projectile_spread", "0" );
	} else {
		projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	}
	if ( projectileDef->GetFloat( "num_projectiles" ) ) {
		num_projectiles = projectileDef->GetFloat( "num_projectiles", "1" );
	} else {
		num_projectiles = spawnArgs.GetFloat( "num_projectiles", "1" );
	}

	GetMuzzle( jointname, muzzle, axis );

	if ( !projectile.GetEntity() ) {
		CreateProjectile( muzzle, axis[ 0 ] );
	}

	lastProjectile = projectile.GetEntity();

	if ( target != NULL ) {
		tmp = target->GetPhysics()->GetAbsBounds().GetCenter() - muzzle;
		tmp.Normalize();
		axis = tmp.ToMat3();
	} else {
		axis = viewAxis;
	}

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
	if ( spawnArgs.GetBool( "lag_target", "0" ) ) {
		dir = (lastTargetPos[0] - muzzle).ToNormal();
	} else {
		GetAimDir( muzzle, target, this, dir );
	}
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

	TriggerWeaponEffects( muzzle,axis );

	lastAttackTime = gameLocal.time;

//HUMANHEAD mdc - added to support multiple projectiles
	projectile = NULL;
	SetCurrentProjectile( projectileDefaultDefIndex );	//set back to our default projectile to be on the safe side
//HUMANHEAD END

	return lastProjectile;
}

void hhSphereBoss::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {				
	HandleNoGore();
	if ( !AI_DEAD ) {
		AI_DEAD = true;
		state = GetScriptFunction( "state_SphereDeath" );
		SetState( state );
		SetWaitState( "" );
	}
}

void hhSphereBoss::FlyTurn( void ) {
	if ( AI_FACE_ENEMY ) {
		TurnToward( enemy->GetOrigin() );
	} else {
		if ( move.moveCommand == MOVE_FACE_ENEMY ) {
			TurnToward( lastVisibleEnemyPos );
		} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
			TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		} else if ( move.speed > 0.0f ) {
			const idVec3 &vel = physicsObj.GetLinearVelocity();
			if ( vel.ToVec2().LengthSqr() > 0.1f ) {
				TurnToward( vel.ToYaw() );
			}
		}
	}
	Turn();
}

void hhSphereBoss::Event_SetSeekScale( float new_scale ) {
	fly_seek_scale = new_scale;
}

void hhSphereBoss::Event_GetCircleNode() {
	if ( !enemy.IsValid() ) {
		idThread::ReturnEntity( NULL );
		return; 
	}
	idEntity *ent = NULL;
	idEntity *bestEnt = NULL;
	lastNodeIndex++;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( !ent || !ent->IsType( hhAINode::Type ) ) {
			continue;
		}
		if ( !ent->spawnArgs.GetBool( "circle_node" ) ) {
			continue;
		}
		if ( lastNodeIndex == ent->spawnArgs.GetInt( "circle_node_index" ) ) {
			bestEnt = ent;
			break;
		}
	}
	if ( !bestEnt ) {
		lastNodeIndex = 0;
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( !ent || !ent->IsType( hhAINode::Type ) ) {
				continue;
			}
			if ( !ent->spawnArgs.GetBool( "circle_node" ) ) {
				continue;
			}
			if ( lastNodeIndex == ent->spawnArgs.GetInt( "circle_node_index" ) ) {
				bestEnt = ent;
				break;
			}
		}
	}
	if ( bestEnt ) {
		idThread::ReturnEntity( bestEnt );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

void hhSphereBoss::Event_GetCombatNode() {
	idEntity *ent = NULL;
	float bestDist = 0.0f;
	float dist;
	idEntity *bestEnt = NULL;
	idList<idEntity*> list;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( !ent || !ent->spawnArgs.GetBool( "ainode" ) ) {
			continue;
		}
		dist = (ent->GetOrigin() - GetOrigin()).Length();
		if ( dist < spawnArgs.GetFloat( "node_dist", "400" ) ) {
			continue;
		}
		list.Append( ent );
	}
	if ( list.Num() > 0 ) {
		idThread::ReturnEntity( list[gameLocal.random.RandomInt(list.Num())] );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

void hhSphereBoss::Event_DirectMoveToPosition(const idVec3 &pos) {
	StopMove(MOVE_STATUS_DONE);
	DirectMoveToPosition(pos);
}

void hhSphereBoss::FlyMove( void ) {
	idVec3	goalPos;
	idVec3	oldorigin;
	idVec3	newDest;

	AI_BLOCKED = false;
	if ( ( move.moveCommand != MOVE_NONE ) && ReachedPos( move.moveDest, move.moveCommand ) ) {
		if ( AI_FACE_ENEMY ) {
			StopMove( MOVE_STATUS_DONE );
		} else {
			AI_MOVE_DONE = true;
		}
	}

	idVec3 vel = physicsObj.GetLinearVelocity();
	goalPos = move.moveDest;
	if ( ReachedPos( move.moveDest, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
	}

	if ( move.speed	) {
		FlySeekGoal( vel, goalPos );
	}
	AddFlyBob( vel );
	AdjustFlySpeed( vel );
	physicsObj.SetLinearVelocity( vel );

	// turn	
	FlyTurn();

	// run the physics for this frame
	oldorigin = physicsObj.GetOrigin();
	physicsObj.UseFlyMove( true );
	physicsObj.UseVelocityMove( false );
	physicsObj.SetDelta( vec3_zero );
	physicsObj.ForceDeltaMove( disableGravity );
	RunPhysics();

	monsterMoveResult_t	moveResult = physicsObj.GetMoveResult();
	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		} else if ( moveResult == MM_BLOCKED ) {
			move.blockTime = gameLocal.time + 500;
			AI_BLOCKED = true;
		}
	}

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 4000 );
		gameRenderWorld->DebugBounds( colorOrange, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorRed, org, org + physicsObj.GetLinearVelocity(), gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorBlue, org, goalPos, gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
		DrawRoute();
	}
}

void hhSphereBoss::AdjustFlySpeed( idVec3 &vel ) {
	float speed;

	// apply dampening
	float damp = spawnArgs.GetFloat( "fly_dampening", "0.01" );
	vel -= vel * damp * MS2SEC( gameLocal.msec );

	// gradually speed up/slow down to desired speed
	speed = vel.Normalize();
	speed += ( move.speed - speed ) * MS2SEC( gameLocal.msec );
	if ( speed < 0.0f ) {
		speed = 0.0f;
	} else if ( move.speed && ( speed > move.speed ) ) {
		speed = move.speed;
	}

	vel *= speed;
}

bool hhSphereBoss::ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const {
	if ( move.moveType == MOVETYPE_SLIDE ) {
		idBounds bnds( idVec3( -4, -4.0f, -8.0f ), idVec3( 4.0f, 4.0f, 64.0f ) );
		bnds.TranslateSelf( physicsObj.GetOrigin() );	
		if ( bnds.ContainsPoint( pos ) ) {
			return true;
		}
	} else {
		if ( ( moveCommand == MOVE_TO_ENEMY ) || ( moveCommand == MOVE_TO_ENTITY ) ) {
			if ( physicsObj.GetAbsBounds().IntersectsBounds( idBounds( pos ).Expand( 8.0f ) ) ) {
				return true;
			}
		} else {
			idBounds bnds( idVec3( -64.0, -64.0f, -64.0f ), idVec3( 64.0, 64.0f, 64.0f ) );
			bnds.TranslateSelf( physicsObj.GetOrigin() );	
			if ( bnds.ContainsPoint( pos ) ) {
				return true;
			}
		}
	}
	return false;
}

#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhSphereBoss::LinkScriptVariables() {
	hhMonsterAI::LinkScriptVariables();
	LinkScriptVariable( AI_FACE_ENEMY );
	LinkScriptVariable( AI_CAN_DAMAGE );
}

void hhSphereBoss::Event_SpinClouds( float shouldSpin ) {
	if ( shouldSpin == 0.0f ) {
		const idKeyValue *kv = spawnArgs.MatchPrefix( "cloud" );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent ) {
				ent->SetShaderParm( spawnArgs.GetInt( "spin_parm", "4" ), ent->spawnArgs.GetFloat( "shaderparm4", "0" ) );
			}
			kv = spawnArgs.MatchPrefix( "cloud", kv );
		}
	} else {
		const idKeyValue *kv = spawnArgs.MatchPrefix( "cloud" );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent ) {
				ent->SetShaderParm( spawnArgs.GetInt( "spin_parm", "4" ), spawnArgs.GetFloat( "spin_value", "0.5" ) );
			}
			kv = spawnArgs.MatchPrefix( "cloud", kv );
		}
	}
}

void hhSphereBoss::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	//overridden to allow custom wound code
	if ( !AI_CAN_DAMAGE && inflictor && spawnArgs.MatchPrefix("mineDamage") ) {
		hhFxInfo fxInfo;
		fxInfo.RemoveWhenDone( true );
		if ( gameLocal.time >= nextShieldImpact ) {
			nextShieldImpact = gameLocal.time + int(spawnArgs.GetFloat( "shield_impact_freq", "0.4" ) * 1000);
			idVec3 offset;
			if ( attacker && inflictor ) {
				offset = (attacker->GetOrigin() - inflictor->GetOrigin()).ToNormal() * spawnArgs.GetFloat( "shield_impact_offset", "150" );
				const char *defName = spawnArgs.GetString( "fx_shield_impact" );
				idEntityFx *impactFx = SpawnFxLocal( defName, inflictor->GetOrigin() + offset, mat3_identity, &fxInfo, gameLocal.isClient );
				if ( impactFx ) {
					impactFx->Bind( this, true );
				}
			}
		}
		StartSound( "snd_shield_impact", SND_CHANNEL_BODY );
		if ( inflictor && inflictor->IsType( idProjectile::Type ) ) {
			inflictor->PostEventMS( &EV_Remove, 0 );
		}
	} else {
		hhFxInfo fxInfo;
		fxInfo.RemoveWhenDone( true );
		if ( gameLocal.time >= nextShieldImpact ) {
			nextShieldImpact = gameLocal.time + int(spawnArgs.GetFloat( "shield_impact_freq", "0.4" ) * 1000);
			idVec3 offset;
			if ( attacker && inflictor ) {
				offset = (attacker->GetOrigin() - inflictor->GetOrigin()).ToNormal() * spawnArgs.GetFloat( "shield_impact_offset", "150" );
				BroadcastFxInfoPrefixed( "fx_pain_impact", inflictor->GetOrigin() + offset, mat3_identity, &fxInfo );
			}
		}
	}

	bool mine_damage = false;
	if ( !AI_CAN_DAMAGE && spawnArgs.MatchPrefix("mineDamage") != NULL ) {
		const idKeyValue *kv = spawnArgs.MatchPrefix("mineDamage");
		while( kv && kv->GetValue().Length() ) {
			if ( !kv->GetValue().Icmp(damageDefName) ) {
				mine_damage = true;
				break;
			}
			kv = spawnArgs.MatchPrefix("mineDamage", kv);
		}
		if (!mine_damage) {
			return;
		}
	}

	if ( !mine_damage ) {
		hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	}

	if ( !AI_CAN_DAMAGE && health > 0 ) {
		SetState( GetScriptFunction( "state_Pain" ) );
		SetWaitState( "" );
	}
}

/*
=====================
hhSphereBoss::Save
=====================
*/
void hhSphereBoss::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( lastTargetPos.Num() );
	savefile->WriteInt( nextShieldImpact );
	for ( int i=0;i<lastTargetPos.Num(); i++) {
		savefile->WriteVec3( lastTargetPos[i] );
	}
}

/*
=====================
hhSphereBoss::Restore
=====================
*/
void hhSphereBoss::Restore( idRestoreGame *savefile ) {
	int num = 0;
	savefile->ReadInt( num );
	savefile->ReadInt( nextShieldImpact );
	lastTargetPos.SetNum( num );
	for ( int i=0;i<num; i++) {
		savefile->ReadVec3( lastTargetPos[i] );
	}
}

void hhSphereBoss::AddLocalMatterWound( jointHandle_t jointNum, const idVec3 &localOrigin, const idVec3 &localNormal, const idVec3 &localDir, int damageDefIndex, const idMaterial *collisionMaterial ) {
	//overridden to remove woundmanager hook and use custom wound code is Damage()
	if ( AI_CAN_DAMAGE  ) {
		return hhMonsterAI::AddLocalMatterWound( jointNum, localOrigin, localNormal, localDir, damageDefIndex, collisionMaterial );
	}
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build