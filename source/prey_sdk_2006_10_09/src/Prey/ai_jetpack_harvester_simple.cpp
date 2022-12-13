#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef AI_RemoveJetFx("removeJetFx");
const idEventDef AI_CreateJetFx("createJetFx");
const idEventDef AI_LaunchMine("launchMine", "d");
const idEventDef MA_IsEnemySniping("isEnemySniping", NULL, 'd');
const idEventDef MA_NumMines("numMines", NULL, 'f');
const idEventDef MA_DodgeProjectile("<dodgeProjectile>", "e");
const idEventDef MA_AppendHoverFx( "<appendHoverFx>", "e" );

CLASS_DECLARATION( hhMonsterAI, hhJetpackHarvesterSimple )
	EVENT( AI_RemoveJetFx,				hhJetpackHarvesterSimple::Event_RemoveJetFx )
	EVENT( EV_Activate,	   				hhJetpackHarvesterSimple::Event_Activate)
	EVENT( AI_CreateJetFx,				hhJetpackHarvesterSimple::Event_CreateJetFx )
	EVENT( AI_LaunchMine,				hhJetpackHarvesterSimple::Event_LaunchMine )
	EVENT( EV_Broadcast_AppendFxToList,	hhJetpackHarvesterSimple::Event_AppendToThrusterList )
	EVENT( MA_OnProjectileLaunch,		hhJetpackHarvesterSimple::Event_OnProjectileLaunch )
	EVENT( MA_DodgeProjectile,			hhJetpackHarvesterSimple::Event_DodgeProjectile )
	EVENT( MA_IsEnemySniping,			hhJetpackHarvesterSimple::Event_IsEnemySniping )
	EVENT( MA_NumMines,					hhJetpackHarvesterSimple::Event_NumMines )
	EVENT( MA_AppendHoverFx,			hhJetpackHarvesterSimple::Event_AppendHoverFx )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

hhJetpackHarvesterSimple::~hhJetpackHarvesterSimple() {
	for ( int i=0;i<mineList.Num();i++ ) {
		if ( mineList[i].IsValid() ) {
			trace_t collision;
			memset( &collision, 0, sizeof( collision ) );
			collision.endAxis = mineList[i]->GetPhysics()->GetAxis();
			collision.endpos = mineList[i]->GetPhysics()->GetOrigin();
			collision.c.point = mineList[i]->GetPhysics()->GetOrigin();
			collision.c.normal.Set( 0, 0, 1 );
			mineList[i]->Explode( &collision, idVec3(0,0,0), 0 );
		}
	}
}

void hhJetpackHarvesterSimple::Spawn(void) {
	lastAntiProjectileAttack = gameLocal.GetTime();
	for(int i=0;i<ThrustSide_Total;i++) {		
		fxThrusters[ThrustType_Idle][i] = NULL;			
		fxThrusters[ThrustType_Hover][i] = NULL;			
	}
	if ( !spawnArgs.GetBool( "bAlwaysGib" ) && !spawnArgs.GetBool( "no_pre_death" ) ) {
		allowPreDeath = true;
	} else {
		allowPreDeath = false;
	}
	freezeDamage = false;
	specialDamage = false;
	Event_CreateJetFx();

	//give random model offset
	float offset_min = spawnArgs.GetFloat( "offset_z_min", "0" );
	float offset_max = spawnArgs.GetFloat( "offset_z_max", "0" );
	modelOffset.z = gameLocal.random.RandomFloat() * (offset_max-offset_min) + offset_min;
	walkIK.Init( this, IK_ANIM, modelOffset );	
}

void hhJetpackHarvesterSimple::Event_RemoveJetFx(void) {
	for(int i=0;i<ThrustSide_Total;i++) {
		SAFE_REMOVE(fxThrusters[ThrustType_Idle][i]);
		SAFE_REMOVE(fxThrusters[ThrustType_Hover][i]);
	}

}

void hhJetpackHarvesterSimple::Event_CreateJetFx(void) {
	const char *bones[ThrustSide_Total];
	bones[ThrustSide_Left]	= spawnArgs.GetString( "bone_thrust_left" );
	bones[ThrustSide_Right] = spawnArgs.GetString( "bone_thrust_right" );	

	const char *psystem = spawnArgs.GetString( "fx_thruster_idle" );
	if ( psystem && *psystem ) {
		for(int i=0;i<ThrustSide_Total;i++) {		
			hhFxInfo fxInfo;
			
			idVec3 bonePos;
			idMat3 boneAxis;
			this->GetJointWorldTransform( bones[i], bonePos, boneAxis );
			fxInfo.SetEntity( this );
			fxInfo.SetBindBone( bones[i] );
			fxInfo.Toggle();
			fxInfo.RemoveWhenDone( false );
			
			if( fxThrusters[ThrustType_Idle][i] != NULL ) {
				gameLocal.Warning("Jetpack harvester already had flamejet made.");
			}

			BroadcastFxInfo( psystem, bonePos, boneAxis, &fxInfo, &EV_Broadcast_AppendFxToList );
		}
	}

	psystem = spawnArgs.GetString( "fx_thruster_hover" );
	if ( psystem && *psystem ) {
		for(int i=0;i<ThrustSide_Total;i++) {		
			hhFxInfo fxInfo;
			idVec3 bonePos;
			idMat3 boneAxis;
			GetJointWorldTransform( bones[i], bonePos, boneAxis );
			fxInfo.SetEntity( this );
			fxInfo.SetBindBone( bones[i] );
			fxInfo.Toggle();
			fxInfo.RemoveWhenDone( false );
			
			if( fxThrusters[ThrustType_Hover][i] != NULL ) {
				gameLocal.Warning("Jetpack harvester already had flamejet made.");
			}

			BroadcastFxInfo( psystem, bonePos, boneAxis, &fxInfo, &MA_AppendHoverFx );
		}
	}
}

void hhJetpackHarvesterSimple::Event_AppendToThrusterList( hhEntityFx* fx ) {
	for( int ix = 0; ix < ThrustSide_Total; ++ix ) {
		if( fxThrusters[ThrustType_Idle][ix] != NULL ) {
			continue;
		}

		fxThrusters[ThrustType_Idle][ix] = fx;
		return;
	}
}

void hhJetpackHarvesterSimple::Event_AppendHoverFx( hhEntityFx* fx ) {
	for( int ix = 0; ix < ThrustSide_Total; ++ix ) {
		if( fxThrusters[ThrustType_Hover][ix] != NULL ) {
			continue;
		}
		fxThrusters[ThrustType_Hover][ix] = fx;
		return;
	}
}

void hhJetpackHarvesterSimple::Event_LaunchMine( const idList<idStr>* parmList ) {
	if( !parmList || parmList->Num() != 2 ) {
		return;
	}

	idVec3 jointPos;
	idMat3 jointAxis;
	const char* projectileDefName = (*parmList)[0].c_str();
	const char* jointName = (*parmList)[1].c_str();

	if( !projectileDefName || !projectileDefName[0] ) {
		return;
	}

	if( !GetJointWorldTransform(jointName, jointPos, jointAxis) ) {
		return;
	}

	hhHarvesterMine* mine = static_cast<hhHarvesterMine*>( gameLocal.SpawnObject(projectileDefName) );
	mine->Create( this, jointPos, jointAxis );
	mine->Launch( jointPos, jointAxis, physicsObj.GetPushedLinearVelocity() );
}

void hhJetpackHarvesterSimple::Hide( void ) {
	hhMonsterAI::Hide();

	for(int i=0;i<ThrustSide_Total;i++) {
		if(fxThrusters[ThrustType_Idle][i].GetEntity()) {
			fxThrusters[ThrustType_Idle][i]->Nozzle(FALSE);
		}			
		if(fxThrusters[ThrustType_Hover][i].GetEntity()) {
			fxThrusters[ThrustType_Hover][i]->Nozzle(FALSE);
		}			
	}
}

void hhJetpackHarvesterSimple::Show( void ) {
	hhMonsterAI::Show();
	
	for(int i=0;i<ThrustSide_Total;i++) {
		if(fxThrusters[ThrustType_Idle][i].GetEntity()) {
			fxThrusters[ThrustType_Idle][i]->Nozzle(TRUE);
		}			
		if(fxThrusters[ThrustType_Hover][i].GetEntity()) {
			fxThrusters[ThrustType_Hover][i]->Nozzle(FALSE);
		}			
	}
}

idProjectile *hhJetpackHarvesterSimple::LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef ) {	//HUMANHEAD mdc - added desiredProjectileDef for supporting multiple projs.
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
	projectile_spread = desiredProjectileDef->GetFloat( "projectile_spread", "0" );
	num_projectiles = desiredProjectileDef->GetInt( "num_projectiles", "1" );
	idVec3 launch_velocity = vec3_zero;
	if ( desiredProjectileDef->GetFloat( "launch_y", "0" ) > 0.0f ) { 
		if ( gameLocal.random.RandomFloat() < 0.5 ) {
			launch_velocity.y = -desiredProjectileDef->GetFloat( "launch_y", "0" );
		} else {
			launch_velocity.y = desiredProjectileDef->GetFloat( "launch_y", "0" );
		}
		launch_velocity *= viewAxis;
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
	GetAimDir( muzzle, target, this, dir );
	ang = dir.ToAngles();

	// adjust his aim so it's not perfect.  uses sine based movement so the tracers appear less random in their spread.
	float t = MS2SEC( gameLocal.time + entityNumber * 497 );
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
		//dir = axis[ 0 ] + axis[ 2 ] * ( 0 * idMath::Sin( spin ) ) - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir = axis[ 0 ] - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir.Normalize();

		// launch the projectile
		if ( !projectile.GetEntity() ) {
			CreateProjectile( muzzle, dir );
		}
		lastProjectile = projectile.GetEntity();
		lastProjectile->Launch( muzzle, dir, launch_velocity );
		projectile = NULL;
		if ( projectileDef->GetBool( "mine", "0" ) ) {
			idEntityPtr<hhProjectile> &entityPtr = mineList.Alloc();
			entityPtr = lastProjectile;
		}
	}

	TriggerWeaponEffects( muzzle, axis );

	lastAttackTime = gameLocal.time;

//HUMANHEAD mdc - added to support multiple projectiles
	projectile = NULL;
	SetCurrentProjectile( projectileDefaultDefIndex );	//set back to our default projectile to be on the safe side
//HUMANHEAD END

	return lastProjectile;
}

void hhJetpackHarvesterSimple::AnimMove( void ) {
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

	physicsObj.UseFlyMove( false );

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
	
	delta *= spawnArgs.GetFloat( "fly_scale", "1.0" );

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

void hhJetpackHarvesterSimple::Event_NumMines() {
	//we only want valid mines, so remove dead ones
	for ( int i=0;i<mineList.Num();i++ ) {
		if ( !mineList[i].IsValid() || mineList[i].GetEntity()->health <= 0 ) {
			mineList.RemoveIndex(i);
		}
	}
	idThread::ReturnFloat( float(mineList.Num()) );
}

void hhJetpackHarvesterSimple::Event_DodgeProjectile( hhProjectile *proj ) {
	if ( !proj || !enemy.IsValid() ) {
		return;
	}
	float min = spawnArgs.GetFloat( "dda_dodge_min", "0.3" );
	float max = spawnArgs.GetFloat( "dda_dodge_max", "0.8" );

	float dodgeChance = (min + (max-min)*gameLocal.GetDDAValue() );

	if ( ai_debugBrain.GetBool() ) {
		gameLocal.Printf( "%s dodge chance %f\n", GetName(), dodgeChance );
	}
	if ( gameLocal.random.RandomFloat() > dodgeChance ) {
		return;
	}

	idVec3 fw		= viewAxis[0];
	idVec3 projFw	= proj->GetAxis()[0]; 
	if(proj->GetOwner())
		projFw = proj->GetOwner()->GetAxis()[0];
	float dot = fw * projFw;
	if(dot > -.7f)
		return;
	const function_t *newstate = NULL;

	//determine which side to dodge to
	idVec3 povPos, targetPos;
	povPos = enemy->GetPhysics()->GetOrigin();
	targetPos = GetPhysics()->GetOrigin();
	idVec3 povToTarget = targetPos - povPos;
	povToTarget.z = 0.f;
	povToTarget.Normalize();
	idVec3 povLeft, povUp;
	povToTarget.OrthogonalBasis(povLeft, povUp);
	povLeft.Normalize();
	idVec3 projVel = proj->GetPhysics()->GetLinearVelocity();
	projVel.Normalize();
	dot = povLeft * projVel;
	if ( dot < 0 ) { 
		newstate = GetScriptFunction( "state_DodgeRight" );
	} else {
		newstate = GetScriptFunction( "state_DodgeLeft" );
	}
	if ( newstate ) {
		lastAntiProjectileAttack = gameLocal.GetTime();
		SetState( newstate );
		UpdateScript();
	}	
}

void hhJetpackHarvesterSimple::Event_OnProjectileLaunch( hhProjectile *proj ) {
	if ( !proj || !spawnArgs.GetBool( "can_dodge", "1" ) || health <= 0 ) {
		return;
	}
	if ( gameLocal.GetTime() - lastAntiProjectileAttack < 1000 * int(spawnArgs.GetFloat( "dodge_freq", "2.0" )) )  {
		return;	
	}
	if( proj->GetOwner() && !(ReactionTo(proj->GetOwner()) & (ATTACK_ON_SIGHT | ATTACK_ON_DAMAGE)) ) {
		return;			// The person who launched this projectile wasn't someone to worry about
	}
	if ( !enemy.IsValid() ) {
		return;
	}
	PostEventSec( &MA_DodgeProjectile, spawnArgs.GetFloat( "dodge_delay", "0.5" ), proj );
}

bool hhJetpackHarvesterSimple::Collide( const trace_t &collision, const idVec3 &velocity ) {
	if ( af.IsActive() && !IsHidden() && !freezeDamage && !specialDamage ) {
		const char *fx = spawnArgs.GetString("fx_death");
		if ( fx && *fx ) {
			hhFxInfo fxInfo;
			fxInfo.RemoveWhenDone(true);
			BroadcastFxInfo(fx, GetOrigin(), GetAxis(), &fxInfo);
			Hide();
			PostEventMS(&EV_Remove, 1000);
		}
	}

	return false;
}

void hhJetpackHarvesterSimple::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if (!frozen) {
		// Being tractored can mess up predeath
		fl.canBeTractored = false;
	}
	HandleNoGore();
	if ( AI_DEAD ) {
		AI_PAIN = true;
		AI_DAMAGE = true;
		return;
	}

	//stop rocket fx and sounds
	StopSound( SND_CHANNEL_BODY3, false );
	for(int i=0;i<ThrustSide_Total;i++) {
		SAFE_REMOVE(fxThrusters[ThrustType_Idle][i]);
		SAFE_REMOVE(fxThrusters[ThrustType_Hover][i]);
	}

	if ( !frozen && allowPreDeath && !AI_BOUND ) {
		allowPreDeath = false;
		trace_t TraceInfo;
		gameLocal.clip.TracePoint(TraceInfo, GetOrigin(), GetOrigin() + GetPhysics()->GetGravityNormal() * 800, MASK_MONSTERSOLID, this);
		if( TraceInfo.fraction < 1.0f ) {
			if ( HasPathTo( TraceInfo.endpos ) ) {
				Event_SetMoveType( MOVETYPE_ANIM );
				health += 30;
				state = GetScriptFunction( "state_StartPreDeath" );
				SetState( state );
				SetWaitState( "" );
				return;
			}
		}
	}

	hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );

	//explode creature's mines upon death
	for ( int i=0;i<mineList.Num();i++ ) {
		if ( mineList[i].IsValid() ) {
			trace_t collision;
			memset( &collision, 0, sizeof( collision ) );
			collision.endAxis = mineList[i]->GetPhysics()->GetAxis();
			collision.endpos = mineList[i]->GetPhysics()->GetOrigin();
			collision.c.point = mineList[i]->GetPhysics()->GetOrigin();
			collision.c.normal.Set( 0, 0, 1 );
			mineList[i]->Explode( &collision, idVec3(0,0,0), 0 );
		}
	}
}

void hhJetpackHarvesterSimple::Event_IsEnemySniping(void) {
	idThread::ReturnInt( false );
}

/*
=====================
hhJetpackHarvesterSimple::Save
=====================
*/
void hhJetpackHarvesterSimple::Save( idSaveGame *savefile ) const {
	int i;
	for ( i = 0; i < ThrustSide_Total; i++ ) {
		for ( int j = 0; j < ThrustType_Total; j++ ) {
			fxThrusters[i][j].Save( savefile );
		}
	}

	savefile->WriteInt( lastAntiProjectileAttack );
	savefile->WriteBool( allowPreDeath );

	int num = mineList.Num();
	savefile->WriteInt( num );
	savefile->WriteBool( specialDamage );
	for ( i = 0; i < num; i++ ) {
		mineList[i].Save( savefile );
	}
}

/*
=====================
hhJetpackHarvesterSimple::Restore
=====================
*/
void hhJetpackHarvesterSimple::Restore( idRestoreGame *savefile ) {
	int i;
	for ( i = 0; i < ThrustSide_Total; i++ ) {
		for ( int j = 0; j < ThrustType_Total; j++ ) {
			fxThrusters[i][j].Restore( savefile );
		}
	}

	savefile->ReadInt( lastAntiProjectileAttack );
	savefile->ReadBool( allowPreDeath );

	int num;
	savefile->ReadInt( num );
	savefile->ReadBool( specialDamage );
	mineList.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		mineList[i].Restore( savefile );
	}
}

void hhJetpackHarvesterSimple::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !AI_DEAD ) {
		if ( damageDef && damageDef->GetBool( "ice" ) && spawnArgs.GetBool( "can_freeze", "0" ) ) {
			freezeDamage = true;
			allowPreDeath = false;
		} else {
			freezeDamage = false;
		}
		if ( damageDef && damageDef->GetBool( "no_special_death" ) ) {
			specialDamage = true;
			allowPreDeath = false;
		}
	}
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build