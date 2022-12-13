
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef MA_LaserOn("laserOn", NULL);
const idEventDef MA_LaserOff("laserOff", NULL);
const idEventDef MA_EscapePortal( "escapePortal" );
const idEventDef MA_AssignSniperFx( "<assignSniperFx>", "e" );
const idEventDef MA_KickAngle( "kickAngle", "v" );
const idEventDef MA_AssignFlashLightFx( "<assignFlashLightFx>", "e" );
const idEventDef MA_FlashLightOn( "flashLightOn" );
const idEventDef MA_FlashLightOff( "flashLightOff" );
const idEventDef MA_EnemyCanSee( "enemyCanSee", NULL, 'd' );
const idEventDef MA_PrintAction( "printAction", "s" );
const idEventDef MA_GetAlly( "getAlly", NULL, 'e' );
const idEventDef MA_TriggerDelay( "triggerDelay", "Ef" );
const idEventDef MA_CallBackup( "callBackup", "f" );
const idEventDef MA_SaySound( "saySound", "s", 'f' );
const idEventDef MA_CheckRush( "<checkRush>" );
const idEventDef MA_CheckRetreat( "<checkRetreat>" );
const idEventDef MA_SayEnemyInfo( "sayEnemyInfo", NULL, 'd' );
const idEventDef MA_SetNextVoiceTime( "setNextVoiceTime", "d" );
const idEventDef MA_GetCoverNode( "getCoverNode", NULL, 'v' );
const idEventDef MA_GetCoverPoint( "getCoverPoint", NULL, 'v' );
const idEventDef MA_OnProjectileHit( "<onProjectileHit>", "e" );
const idEventDef MA_GetSightNode( "getSightNode", NULL, 'v' );
const idEventDef MA_GetNearSightPoint( "getNearSightPoint", NULL, 'v' );
const idEventDef MA_Blocked( "<blocked>" );
const idEventDef MA_EnemyPortal( "<enemyPortal>", "e" );
const idEventDef MA_GetEnemyPortal( "getEnemyPortal", NULL, 'e' );
const idEventDef MA_EnemyVehicleDocked( "enemyVehicleDocked", NULL, 'd' );

CLASS_DECLARATION(hhMonsterAI, hhHunterSimple)
	EVENT( MA_OnProjectileLaunch,	hhHunterSimple::Event_OnProjectileLaunch )
	EVENT( MA_OnProjectileHit,		hhHunterSimple::Event_OnProjectileHit )
	EVENT( MA_LaserOn,				hhHunterSimple::Event_LaserOn)
	EVENT( MA_LaserOff,				hhHunterSimple::Event_LaserOff)
	EVENT( MA_EscapePortal,			hhHunterSimple::Event_EscapePortal )
	EVENT( MA_AssignSniperFx,		hhHunterSimple::Event_AssignSniperFx )
	EVENT( MA_KickAngle,			hhHunterSimple::Event_KickAngle )
	EVENT( MA_AssignFlashLightFx,	hhHunterSimple::Event_AssignFlashLightFx )
	EVENT( MA_FlashLightOn,			hhHunterSimple::Event_FlashLightOn )
	EVENT( MA_FlashLightOff,		hhHunterSimple::Event_FlashLightOff )
	EVENT( MA_EnemyCanSee,			hhHunterSimple::Event_EnemyCanSee )
	EVENT( MA_PrintAction,			hhHunterSimple::Event_PrintAction )
	EVENT( MA_GetAlly,				hhHunterSimple::Event_GetAlly )
	EVENT( MA_TriggerDelay,			hhHunterSimple::Event_TriggerDelay )
	EVENT( MA_CallBackup,			hhHunterSimple::Event_CallBackup )
	EVENT( MA_SaySound,				hhHunterSimple::Event_SaySound )
	EVENT( MA_CheckRush,			hhHunterSimple::Event_CheckRush )
	EVENT( MA_CheckRetreat,			hhHunterSimple::Event_CheckRetreat )
	EVENT( MA_SayEnemyInfo,			hhHunterSimple::Event_SayEnemyInfo )
	EVENT( MA_SetNextVoiceTime,		hhHunterSimple::Event_SetNextVoiceTime )
	EVENT( MA_GetCoverNode,			hhHunterSimple::Event_GetCoverNode )
	EVENT( MA_GetCoverPoint,		hhHunterSimple::Event_GetCoverPoint )
	EVENT( MA_GetSightNode,			hhHunterSimple::Event_GetSightNode )
	EVENT( MA_GetNearSightPoint,	hhHunterSimple::Event_GetNearSightPoint )
	EVENT( MA_Blocked,				hhHunterSimple::Event_Blocked )
	EVENT( MA_EnemyPortal,			hhHunterSimple::Event_EnemyPortal )
	EVENT( MA_GetEnemyPortal,		hhHunterSimple::Event_GetEnemyPortal )
	EVENT( MA_EnemyVehicleDocked,	hhHunterSimple::Event_EnemyVehicleDocked )
END_CLASS

hhHunterSimple::hhHunterSimple() {
	beamLaser = 0;
	kickSpeed = ang_zero;
	kickAngles = ang_zero;
	alternateAccuracy = false;
	bFlashLight = false;
	nodeList.Clear();
	ally.Clear();
	nextEnemyCheck = 0;
	enemyPortal = 0;
	nextBlockCheckTime = 0;
	lastMoveOrigin = vec3_zero;
	nextSpiritCheck = 0;		//HUMANHEAD jsh PCF 5/2/06 hunter combat fixes
}

void hhHunterSimple::Spawn() {
	bFlashLight = false;
	lastChargeTime = 0;
	endSpeechTime = 0;
	enemyRushCount = 0;
	enemyRetreatCount = 0;
	nextVoiceTime = 0;
	currentAction.Clear();
	currentSpeech.Clear();
	flashlightLength = spawnArgs.GetFloat( "flashlightLength", "90" );
	flashlightTime = gameLocal.time + SEC2MS( spawnArgs.GetInt( "flashlight_delay" ) );
	beamLaser = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamLaser" ) );
	initialOrigin = GetOrigin();
	if( beamLaser.IsValid() ) {
		beamLaser->Activate( false );
		idMat3 boneAxis; 
		idVec3 boneOrigin;
		GetJointWorldTransform( spawnArgs.GetString("laser_bone"), boneOrigin, boneAxis );
		beamLaser->SetOrigin( boneOrigin );
		beamLaser->BindToJoint( this, spawnArgs.GetString("laser_bone"), false );
	}
	PostEventSec( &MA_CheckRush, 0 );
	PostEventSec( &MA_CheckRetreat, 0 );
}

void hhHunterSimple::Event_PostSpawn() {
	hhMonsterAI::Event_PostSpawn();

	//look for ainodes in targets list
	for ( int i=0;i<targets.Num();i++ ) {
		if ( targets[i].IsValid() && targets[i]->IsType( hhAINode::Type ) ) {
			nodeList.Append( targets[i] );
		}
	}
	if ( nodeList.Num() <= 0 ) {
		//if targets list had none, find dynamically
		idEntity *ent;
		float distSq;
		aasPath_t	path;
		int			toAreaNum, areaNum;
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( !ent || !ent->spawnArgs.GetBool( "ainode", "0" ) ) {
				continue;
			}
			distSq = (GetOrigin() - ent->GetOrigin()).LengthSqr();
			if ( distSq > 2250000 ) { //less than 1500
				continue;
			}
			//check reachability 
			toAreaNum = PointReachableAreaNum( ent->GetOrigin() );
			areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
			if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, ent->GetOrigin() ) ) {
				continue;
			}
			nodeList.Append( ent );
		}
	}
}

void hhHunterSimple::Event_OnProjectileLaunch(hhProjectile *proj) {
	if ( !enemy.IsValid() || !proj || AI_VEHICLE ) {
		return;
	}
	if ( GetPhysics() && GetPhysics()->GetGravityNormal() != idVec3( 0,0,-1 ) ) {
		return;
	}

	float min = spawnArgs.GetFloat( "dda_dodge_min", "0.3" );
	float max = spawnArgs.GetFloat( "dda_dodge_max", "0.8" );
	float dodgeChance = 0.6f;
	
	dodgeChance = (min + (max-min)*gameLocal.GetDDAValue() );

	if ( ai_debugBrain.GetBool() ) {
		gameLocal.Printf( "%s dodge chance %f\n", GetName(), dodgeChance );
	}
	if ( proj->GetOwner() && proj->GetOwner() == enemy.GetEntity() ) {
		float reactChance = proj->spawnArgs.GetFloat( "react_chance" );
		if ( gameLocal.random.RandomFloat() < reactChance ) {
			if ( ai_debugBrain.GetBool() ) {
				gameLocal.Printf( "say %s\n", proj->spawnArgs.GetString( "react_sound" ) );
			}
			AI_ENEMY_RETREAT = true;
			PostEventSec( &MA_SaySound, gameLocal.random.RandomFloat(), proj->spawnArgs.GetString( "react_sound" ) );
		}
	}
	if ( gameLocal.random.RandomFloat() > dodgeChance ) {
		return;
	}
	if ( proj->GetOwner() && proj->GetOwner()->IsType( hhHunterSimple::Type ) ) {
		return;
	}

	//determine which side to dodge to
	const function_t *newstate = NULL;
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
	float dot = povLeft * projVel;

	if ( dot < 0 ) { 
		newstate = GetScriptFunction( "state_DodgeRight" );
	} else {
		newstate = GetScriptFunction( "state_DodgeLeft" );
	}

	if ( newstate ) {
		SetState( newstate );
		UpdateScript();
	}
}

void hhHunterSimple::Event_LaserOn() {
	if ( beamLaser.IsValid() ) {
		beamLaser->Activate( true );
	}
}

void hhHunterSimple::Event_LaserOff() {
	if ( beamLaser.IsValid() ) {
		beamLaser->Activate( false );
	}
}

void hhHunterSimple::Event_KickAngle( const idAngles &ang ) {
	kickSpeed = ang;
}

void hhHunterSimple::Think() {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) {
		return;
	}

	hhMonsterAI::Think();

	trace_t trace;
	if ( flashlightTime <= 0 || gameLocal.time > flashlightTime ) {
		flashlightTime = 0;
		if ( !AI_DEAD && !IsHidden() && !InVehicle() ) {
			idMat3 boneAxis;
			idVec3 boneOrigin;
			GetJointWorldTransform( "fx_barrel", boneOrigin, boneAxis );
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugArrow( colorRed, boneOrigin, boneOrigin + boneAxis[0] * flashlightLength, 10, 1 );
			}
			gameLocal.clip.TracePoint( trace, boneOrigin, boneOrigin + boneAxis[0] * flashlightLength, MASK_OPAQUE, this );
			if ( trace.fraction < 1.0f ) {
				Event_FlashLightOff();
			} else {
				Event_FlashLightOn();
			}
		} else if ( bFlashLight ) {
			Event_FlashLightOff();
		}
	}
	if ( enemy.IsValid() && beamLaser.IsValid() && beamLaser->IsActivated() ) {
		memset(&trace, 0, sizeof(trace));

		float frametime = (gameLocal.time - gameLocal.previousTime);
		kickSpeed-=4*kickSpeed*frametime/1000;
		kickSpeed-=30*kickAngles*frametime/1000;
		kickAngles+=kickSpeed*frametime/1000;

		idAngles ang = (lastVisibleEnemyPos + enemy->EyeOffset() - idVec3( 0,0,10 ) - beamLaser->GetOrigin()).ToAngles();
		ang += kickAngles;
		idVec3 forward, up, right;
		ang.ToVectors( &forward, &right, &up );
		gameLocal.clip.TracePoint( trace, beamLaser->GetOrigin(), beamLaser->GetOrigin() + forward * 4000, MASK_SHOT_BOUNDINGBOX, this );
		if ( trace.fraction < 1.0f ) {
			idEntity *hitEnt = gameLocal.GetTraceEntity( trace );
			if ( hitEnt && hitEnt->IsType( idPlayer::Type ) ||
				( hitEnt && hitEnt->IsType( hhVehicle::Type ) ) ) { 
				//if we hit the player, set the endpoint a little farther back
				//to prevent the laser from stopping at our camera viewpoint
				idVec3 offset = (trace.endpos - beamLaser->GetOrigin()).ToNormal() * 100;
				beamLaser->SetTargetLocation( trace.endpos + offset );
			} else {
				beamLaser->SetTargetLocation( trace.endpos );
			}
		} else {
			beamLaser->SetTargetLocation( beamLaser->GetOrigin() + forward * 5000 );
		}
	}

	if ( ai_debugBrain.GetBool() && ally.IsValid() && ally->GetHealth() > 0 ) {
		gameRenderWorld->DebugArrow( colorGreen, GetOrigin(), ally->GetOrigin(), 10, 1 );
	}

	if ( health > 0 && ai_debugActions.GetBool() && currentAction.Length() ) {
		if ( AI_ENEMY_VISIBLE ) {
			gameRenderWorld->DrawText("visible", GetEyePosition() + idVec3(0.0f, 0.0f, 30.0f), 0.4f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		}
		if ( AI_ENEMY_SHOOTABLE ) {
			gameRenderWorld->DrawText("shootable", GetEyePosition() + idVec3(0.0f, 0.0f, 20.0f), 0.4f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		}
		gameRenderWorld->DrawText(va( "%s", currentAction.c_str() ), GetEyePosition() + idVec3(0.0f, 0.0f, 0.0f), 0.4f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
	}
	if ( health > 0 && ai_printSpeech.GetBool() && currentSpeech.Length() ) {
		gameRenderWorld->DrawText(va( "%s", currentSpeech.c_str() ), GetEyePosition() + idVec3(0.0f, 0.0f, 10.0f), 0.4f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		if ( endSpeechTime > 0 && gameLocal.time > endSpeechTime ) {
			currentSpeech.Clear();
			endSpeechTime = 0;
		}
	}

	if ( enemy.IsValid() ) {
		//HUMANHEAD jsh PCF 5/2/06 hunter combat fixes
		if ( nextSpiritCheck > 0 && gameLocal.time > nextSpiritCheck && enemy->IsType( hhSpiritProxy::Type ) ) {
			nextSpiritCheck = 0;
			SetEnemy( gameLocal.GetLocalPlayer() );
		}
		if ( AI_ENEMY_VISIBLE ) {
			AI_ENEMY_LAST_SEEN = MS2SEC( gameLocal.realClientTime );
		}
		if ( enemy->GetHealth() < spawnArgs.GetInt( "health_warning", "25" ) ) {
			AI_ENEMY_HEALTH_LOW = true;
		}
	}
}

idProjectile *hhHunterSimple::LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef ) {	//HUMANHEAD mdc - added desiredProjectileDef for supporting multiple projs.
	//jsh overridden to allow per-projectile accuracy
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


	if ( !projectileDef || AI_DEAD ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	if ( projectileDef->GetBool( "snipe", "0" ) ) {
		attack_accuracy = 0;
	} else {
		if ( projectileDef->GetFloat( "attack_accuracy" ) ) {
			attack_accuracy = projectileDef->GetFloat( "attack_accuracy", "7" );
		} else {
			attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
		}
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
	GetAimDir( muzzle, target, this, dir );
	ang = dir.ToAngles();

	// hunter inaccuracy is calculated instead of random spread based
	float hitchance;
	if ( !projectileDef->GetFloat( "hit_chance", "0.2", hitchance ) ) {
		hitchance = spawnArgs.GetFloat( "hit_chance" );
	}
	float hitAccuracy;
	if ( !projectileDef->GetFloat( "hit_accuracy", "0", hitAccuracy ) ) {
		hitAccuracy = spawnArgs.GetFloat( "hit_accuracy" );
	}
	float missAccuracy;
	if ( !projectileDef->GetFloat( "miss_accuracy", "0", missAccuracy ) ) {
		missAccuracy = spawnArgs.GetFloat( "miss_accuracy" );
	}
	float t = MS2SEC( gameLocal.time + entityNumber * 497 );
	alternateAccuracy = !alternateAccuracy;
	if ( gameLocal.random.RandomFloat() < hitchance ) {
		//rolled a hit so add very little inaccuracy
		ang.pitch += idMath::Fabs( idMath::Sin16( t * 5.1 ) * hitAccuracy );
		ang.yaw	+= idMath::Sin16( t * 6.7 ) * hitAccuracy;
	} else {
		float rand = gameLocal.random.RandomFloat();
		if ( alternateAccuracy ) {
			rand *= -1;
		}
		ang.pitch += rand * missAccuracy;
		if ( !alternateAccuracy ) {
			rand *= -1;
		}
		ang.yaw	+= rand * missAccuracy;
	}

	//quick hack that should be fine for now
	bool oriented = GetPhysics()->GetGravityNormal() != idVec3( 0,0,-1 ); 
	if ( !oriented && clampToAttackCone ) {
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

void hhHunterSimple::Event_EscapePortal() {
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
	//portalArgs.SetVector( "origin", GetOrigin() );
	idVec3 escapePortalOffset = spawnArgs.GetVector( "escapePortalOffset", "40 0 0" );
	portalArgs.SetVector( "origin", GetOrigin() + ( escapePortalOffset * GetAxis() ) );
	portalArgs.SetFloat( "angle", GetAxis()[0].ToYaw() - 180 );

	// Set the portal draw distance to zero (so the player cannot see through the creature portal)
	portalArgs.Set( "shaderParm5", "0" );

	// Pass along all 'portal_' keys to the portal's spawnArgs;
	hhUtils::GetKeysAndValues( spawnArgs, passPrefix, xferKeys, xferValues );
	for ( int i = 0; i < xferValues.Num(); ++i ) {
		xferKeys[ i ].StripLeadingOnce( passPrefix );
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

	// Update the camera stuff
	portal->ProcessEvent( &EV_UpdateCameraTarget );

	// Open the portal - Need to delay this, so that PostSpawn gets called/sets up the partner portal
	//? Should we always pass in the player?
	portal->PostEventSec( &EV_Activate, 0, gameLocal.GetLocalPlayer() );

	// Maybe wait for it to finish?
	
		
}

void hhHunterSimple::Event_FindReaction( const char* effect ) {
	idEntity* ent;
	hhReaction* react;
	hhReactionDesc::Effect react_effect;
	idEntity* bestEnt = NULL;
	int bestReactIndex = -1;
	float bestDistance = -1;
	float meToReact, enemyToReact;
	int bestRank = -1;

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

			float distToEnt, distToEnemy, distEnemyToEnt;
			// Check actual specifics for reaction type
			switch( react->desc->effect ) {
				case hhReactionDesc::Effect_HaveFun:
				case hhReactionDesc::Effect_Vehicle:
				case hhReactionDesc::Effect_VehicleDock:
				case hhReactionDesc::Effect_ProvideCover:
					//only use reasonably reachable objects
					if ( enemy.IsValid() ) {
						distToEnt = TravelDistance( GetOrigin(), ent->GetOrigin() );
						distToEnemy = (GetOrigin() - enemy->GetOrigin()).Length();
						distEnemyToEnt = (enemy->GetOrigin() - ent->GetOrigin()).Length();
						if ( distToEnt > 1.0 ) {
							float ratio = distToEnt / distEnemyToEnt;
							if ( ratio > 2.0f ) {
								//skip if object is farther than enemy or if enemy is closer to object than i am
								continue;
							}
						}
					}
				case hhReactionDesc::Effect_Heal:
					if ( enemy.IsValid() ) {
						distToEnt = TravelDistance( GetOrigin(), ent->GetOrigin() );
						distToEnemy = (GetOrigin() - enemy->GetOrigin()).Length();
						distEnemyToEnt = (enemy->GetOrigin() - ent->GetOrigin()).Length();
						if ( distToEnt > 1.0 ) {
							float ratio = distToEnt / distEnemyToEnt;
							if ( ratio > 2.0f ) {
								//skip if object is farther than enemy or if enemy is closer to object than i am
								continue;
							}
						}
					}
				case hhReactionDesc::Effect_Climb:
				case hhReactionDesc::Effect_Passageway:
				case hhReactionDesc::Effect_CallBackup:
					break;
				case hhReactionDesc::Effect_DamageEnemy:
					//check if enemy is close enough to matter
					if ( enemy.IsValid() && react->desc->effectRadius > 0 ) {
						enemyToReact = (enemy->GetOrigin() - react->causeEntity->GetOrigin()).LengthSqr();
						if ( enemyToReact > react->desc->effectRadius * react->desc->effectRadius ) {
							continue;
						}
					}
					//check if i'm too close
					meToReact = (GetOrigin() - react->causeEntity->GetOrigin()).LengthSqr();
					if ( meToReact < react->desc->effectRadius * react->desc->effectRadius ) {
						continue;
					}
					break;
				case hhReactionDesc::Effect_Damage:
				default:
					continue;
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

void hhHunterSimple::AnimMove( void ) {
	//overridden to set AI_BLOCKED when blockEnt is found
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
			nextMovePos = newDest;
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

	if ( ai_debugBrain.GetBool() ) {
		gameRenderWorld->DebugArrow( colorRed, physicsObj.GetOrigin(), physicsObj.GetOrigin() + delta.ToNormal() * 100, 10 );
		gameRenderWorld->DebugArrow( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + renderEntity.axis[1] * 100, 10 );
		gameRenderWorld->DebugArrow( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + GetPhysics()->GetAxis()[2] * 100, 10 );
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

	moveResult = physicsObj.GetMoveResult();
	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt ) {
			AI_BLOCKED = true;
			AI_NEXT_DIR_TIME = 0;

			if ( blockEnt->IsType( idAI::Type ) ) {
				StopMove( MOVE_STATUS_BLOCKED_BY_MONSTER );
			}
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

void hhHunterSimple::Event_AssignSniperFx( hhEntityFx* fx ) {
	sniperFx = fx;
}

void hhHunterSimple::Event_AssignFlashLightFx( hhEntityFx* fx ) {
	flashLightFx = fx;
}

bool hhHunterSimple::UpdateAnimationControllers( void ) {
	idVec3		local;
	idVec3		focusPos;
	idVec3		left;
	idVec3 		dir;
	idVec3 		orientationJointPos;
	idVec3 		localDir;
	idAngles 	newLookAng;
	idAngles	diff;
	idMat3		mat;
	idMat3		axis;
	idMat3		orientationJointAxis;
	idAFAttachment	*headEnt = head.GetEntity();
	idVec3		eyepos;
	idVec3		pos;
	int			i;
	idAngles	jointAng;
	float		orientationJointYaw;

	if ( AI_DEAD ) {
		return idActor::UpdateAnimationControllers();
	}

	if ( orientationJoint == INVALID_JOINT ) {
		orientationJointAxis = viewAxis;
		orientationJointPos = physicsObj.GetOrigin();
		orientationJointYaw = current_yaw;
	} else {
		GetJointWorldTransform( orientationJoint, gameLocal.time, orientationJointPos, orientationJointAxis );
		orientationJointYaw = orientationJointAxis[ 2 ].ToYaw();
		orientationJointAxis = idAngles( 0.0f, orientationJointYaw, 0.0f ).ToMat3();
	}

	if ( ai_debugBrain.GetBool() ) {
		gameRenderWorld->DebugArrow( colorCyan, orientationJointPos, orientationJointPos + orientationJointAxis[0] * 64.0, 10, 1 );
	}

	if ( focusJoint != INVALID_JOINT ) {
		if ( headEnt ) {
			headEnt->GetJointWorldTransform( focusJoint, gameLocal.time, eyepos, axis );
		} else {
			// JRMMERGE_GRAVAXIS - What about GetGravAxis() are we still using/needing that?
			GetJointWorldTransform( focusJoint, gameLocal.time, eyepos, axis );
		}
		eyeOffset.z = eyepos.z - physicsObj.GetOrigin().z;
	} else {
		eyepos = GetEyePosition();
	}

	if ( headEnt ) {
		CopyJointsFromBodyToHead();
	}

	// Update the IK after we've gotten all the joint positions we need, but before we set any joint positions.
	// Getting the joint positions causes the joints to be updated.  The IK gets joint positions itself (which
	// are already up to date because of getting the joints in this function) and then sets their positions, which
	// forces the heirarchy to be updated again next time we get a joint or present the model.  If IK is enabled,
	// or if we have a seperate head, we end up transforming the joints twice per frame.  Characters with no
	// head entity and no ik will only transform their joints once.  Set g_debuganim to the current entity number
	// in order to see how many times an entity transforms the joints per frame.
	idActor::UpdateAnimationControllers();

	idEntity *focusEnt = focusEntity.GetEntity();
	//HUMANHEAD jsh allow eyefocus independent from allowJointMod
	if ( ( !allowJointMod && !allowEyeFocus ) || ( gameLocal.time >= focusTime && focusTime != -1 ) ) {	
	    focusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 512.0f;
	} else if ( focusEnt == NULL ) {
		// keep looking at last position until focusTime is up
		focusPos = currentFocusPos;
	} else if ( focusEnt == enemy.GetEntity() ) {
		if ( beamLaser.IsValid() && beamLaser->IsActivated() ) {
			focusPos = beamLaser->GetTargetLocation();
		} else {
			focusPos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset - eyeVerticalOffset * enemy.GetEntity()->GetPhysics()->GetGravityNormal();
		}
	} else if ( focusEnt->IsType( idActor::Type ) ) {
		focusPos = static_cast<idActor *>( focusEnt )->GetEyePosition() - eyeVerticalOffset * focusEnt->GetPhysics()->GetGravityNormal();
	} else {
		focusPos = focusEnt->GetPhysics()->GetOrigin();
	}

	currentFocusPos = currentFocusPos + ( focusPos - currentFocusPos ) * eyeFocusRate;

	// determine yaw from origin instead of from focus joint since joint may be offset, which can cause us to bounce between two angles
	dir = focusPos - orientationJointPos;
	newLookAng.yaw = idMath::AngleNormalize180( dir.ToYaw() - orientationJointYaw );
	newLookAng.roll = 0.0f;
	newLookAng.pitch = 0.0f;
	newLookAng += lookOffset;

	// determine pitch from joint position
	dir = focusPos - eyepos;
	if ( ai_debugBrain.GetBool() ) {
		gameRenderWorld->DebugArrow( colorYellow, eyepos, eyepos + dir, 10, 1 );
	}
	dir.NormalizeFast();
	physicsObj.GetAxis().ProjectVector( dir, localDir );
	newLookAng.pitch = -idMath::AngleNormalize180( localDir.ToPitch() ) + lookOffset.pitch;
	newLookAng.roll	= 0.0f;
	diff = newLookAng - lookAng;
	
	if ( eyeAng != diff ) {
		eyeAng = diff;
		eyeAng.Clamp( eyeMin, eyeMax );
		idAngles angDelta = diff - eyeAng;
		if ( !angDelta.Compare( ang_zero, 0.1f ) ) {
			alignHeadTime = gameLocal.time;
		} else {
			alignHeadTime = gameLocal.time + ( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime;
		}
	}

	if ( idMath::Fabs( newLookAng.yaw ) < 0.1f ) {
		alignHeadTime = gameLocal.time;
	}

	if ( ( gameLocal.time >= alignHeadTime ) || ( gameLocal.time < forceAlignHeadTime ) ) {
		alignHeadTime = gameLocal.time + ( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime;
		destLookAng = newLookAng;
		destLookAng.Clamp( lookMin, lookMax );
	}

	diff = destLookAng - lookAng;
	if ( ( lookMin.pitch == -180.0f ) && ( lookMax.pitch == 180.0f ) ) {
		if ( ( diff.pitch > 180.0f ) || ( diff.pitch <= -180.0f ) ) {
			diff.pitch = 360.0f - diff.pitch;
		}
	}
	if ( ( lookMin.yaw == -180.0f ) && ( lookMax.yaw == 180.0f ) ) {
		if ( diff.yaw > 180.0f ) {
			diff.yaw -= 360.0f;
		} else if ( diff.yaw <= -180.0f ) {
			diff.yaw += 360.0f;
		}
	}
	lookAng = lookAng + diff * headFocusRate;
	lookAng.Normalize180();

	jointAng.roll = 0.0f;
	if ( allowJointMod ) {
		//quick hack. dont change yaw if in different gravity
		bool oriented = GetPhysics()->GetGravityNormal() != idVec3( 0,0,-1 ); 
		for( i = 0; i < lookJoints.Num(); i++ ) {
			if ( oriented ) {
				jointAng.yaw = 0;
			} else {
				jointAng.yaw = lookAng.yaw * lookJointAngles[ i ].yaw;
			}
			jointAng.pitch	= lookAng.pitch * lookJointAngles[ i ].pitch;
			animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
		}
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		// lean into turns
		AdjustFlyingAngles();
	}
	
	if ( headEnt ) {
		idAnimator *headAnimator = headEnt->GetAnimator();

		// HUMANHEAD pdm: Added support for look joints in head entities
		if ( allowJointMod ) {
			for( i = 0; i < headLookJoints.Num(); i++ ) {
				jointAng.pitch	= lookAng.pitch * headLookJointAngles[ i ].pitch;
				jointAng.yaw	= lookAng.yaw * headLookJointAngles[ i ].yaw;
				headAnimator->SetJointAxis( headLookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
			}
		}
		// HUMANHEAD END

		if ( allowEyeFocus ) {
			idMat3 eyeAxis = ( lookAng + eyeAng ).ToMat3(); idMat3 headTranspose = headEnt->GetPhysics()->GetAxis().Transpose();
			axis =  eyeAxis * orientationJointAxis;
			left = axis[ 1 ] * eyeHorizontalOffset;
			eyepos -= headEnt->GetPhysics()->GetOrigin();
			headAnimator->SetJointPos( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + ( axis[ 0 ] * 64.0f + left ) * headTranspose );
			headAnimator->SetJointPos( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + ( axis[ 0 ] * 64.0f - left ) * headTranspose );

			//if ( ai_debugMove.GetBool() ) {
			//	gameRenderWorld->DebugLine( colorRed, orientationJointPos, eyepos + ( axis[ 0 ] * 64.0f + left ) * headTranspose, gameLocal.msec );
			//}
		} else {
			headAnimator->ClearJoint( leftEyeJoint );
			headAnimator->ClearJoint( rightEyeJoint );
		}
	} else {
		if ( allowEyeFocus ) {
			idMat3 eyeAxis = ( lookAng + eyeAng ).ToMat3();
			axis =  eyeAxis * orientationJointAxis;
			left = axis[ 1 ] * eyeHorizontalOffset;
			eyepos += axis[ 0 ] * 64.0f - physicsObj.GetOrigin();
			animator.SetJointPos( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + left );
			animator.SetJointPos( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos - left );
		} else {
			animator.ClearJoint( leftEyeJoint );
			animator.ClearJoint( rightEyeJoint );
		}
	}

	//HUMANHEAD pdm jawflap
	hhAnimator *theAnimator;
	if (head.IsValid()) {
		theAnimator = head->GetAnimator();
	}
	else {
		theAnimator = GetAnimator();
	}
	JawFlap(theAnimator);
	//END HUMANHEAD
		
	return true;
}

void hhHunterSimple::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	//overridden to remove muzzle fx on death
	if ( AI_DEAD ) {
		AI_DAMAGE = true;
		return;
	}
	if ( enemy.IsValid() ) {
		gameLocal.AlertAI( enemy.GetEntity(), spawnArgs.GetInt( "death_alert_radius", "800" ) );
	}
	Event_FlashLightOff();
	if ( beamLaser.IsValid() ) {
		beamLaser->Unbind();
		SAFE_REMOVE( beamLaser );
	}
	hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );

	//taken from idEntity::RemoveBinds()
	//remove any lingering fx on death
	idEntity *ent;
	idEntity *next;
	for( ent = GetTeamChain(); ent != NULL; ent = next ) {
		next = ent->GetTeamChain();
		if ( ent->GetBindMaster() == this && ent->IsType( idEntityFx::Type ) ) {
			ent->Unbind();
			if( !ent->fl.noRemoveWhenUnbound ) {
				ent->PostEventMS( &EV_Remove, 0 );
				if (gameLocal.isClient) {
					ent->Hide();
				}
			}
			next = GetTeamChain();
		}
	}

}

void hhHunterSimple::Save( idSaveGame *savefile ) const {
	beamLaser.Save( savefile );
	savefile->WriteVec3( nextMovePos );
	savefile->WriteAngles( kickAngles );
	savefile->WriteAngles( kickSpeed );
	savefile->WriteFloat( flashlightLength );
	sniperFx.Save( savefile );

	ally.Save( savefile );
	muzzleFx.Save( savefile );
	flashLightFx.Save( savefile );
	savefile->WriteBool( bFlashLight );
	savefile->WriteInt( endSpeechTime );
	savefile->WriteInt( nextEnemyCheck );
	savefile->WriteFloat( lastEnemyDistance );
	savefile->WriteInt( lastChargeTime );
	savefile->WriteInt( nextVoiceTime );
	savefile->WriteVec3( initialOrigin );
	savefile->WriteInt( flashlightTime );
	enemyPortal.Save( savefile );
	savefile->WriteVec3( lastMoveOrigin );
	savefile->WriteInt( nextBlockCheckTime );
	savefile->WriteInt( nextSpiritCheck );		//HUMANHEAD jsh PCF 5/2/06 hunter combat fixes

	//HUMANHEAD jsh PCF 4/28/06 save nodelist
	savefile->WriteInt(nodeList.Num());
	for (int i = 0; i < nodeList.Num(); i++) {
		nodeList[i].Save(savefile);
	}
	//END HUMANHEAD
};

void hhHunterSimple::Restore( idRestoreGame *savefile ) {
	beamLaser.Restore( savefile );
	savefile->ReadVec3( nextMovePos );
	savefile->ReadAngles( kickAngles );
	savefile->ReadAngles( kickSpeed );
	savefile->ReadFloat( flashlightLength );
	sniperFx.Restore( savefile );

	ally.Restore( savefile );
	muzzleFx.Restore( savefile );
	flashLightFx.Restore( savefile );
	savefile->ReadBool( bFlashLight );
	savefile->ReadInt( endSpeechTime );
	savefile->ReadInt( nextEnemyCheck );
	savefile->ReadFloat( lastEnemyDistance );
	savefile->ReadInt( lastChargeTime );
	savefile->ReadInt( nextVoiceTime );
	savefile->ReadVec3( initialOrigin );
	savefile->ReadInt( flashlightTime );
	enemyPortal.Restore( savefile );
	savefile->ReadVec3( lastMoveOrigin );
	savefile->ReadInt( nextBlockCheckTime );
	savefile->ReadInt( nextSpiritCheck );		//HUMANHEAD jsh PCF 5/2/06 hunter combat fixes

	//HUMANHEAD jsh PCF 4/28/06 save nodelist
	int num;
	savefile->ReadInt(num);
	nodeList.SetNum(num);
	for (int i = 0; i < num; i++) {
		nodeList[i].Restore(savefile);
	}
	//END HUMANHEAD

	alternateAccuracy = false;
	enemyRushCount = 0;
	enemyRetreatCount = 0;
	enemyHiddenCount = 0;
	currentAction.Clear();
	currentSpeech.Clear();
};

void hhHunterSimple::Event_FlashLightOn() {
	if ( !bFlashLight && gameLocal.time > flashlightTime && spawnArgs.GetBool( "can_flashlight" ) ) {
		bFlashLight = true;
		BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_flashLight"), "fx_barrel", NULL, &MA_AssignFlashLightFx, false );
	}
}

void hhHunterSimple::Event_FlashLightOff() {
	bFlashLight = false;
	SAFE_REMOVE( flashLightFx );
}

void hhHunterSimple::PrintDebug() {
	hhMonsterAI::PrintDebug();
	gameLocal.Printf( "  Ally: %s\n", ally.IsValid() ? ally->GetName() : "none" );
	gameLocal.Printf( "  Allow Movement: %s\n", allowMove ? "yes" : "no" );
	gameLocal.Printf( "  Turn rate : %f\n", turnRate );
	if ( nodeList.Num() ) {
		gameLocal.Printf( "  Cover Points:\n" );
		aasPath_t	path; 
		int			toAreaNum, areaNum;
		for( int i=0;i<nodeList.Num();i++ ) {
			if ( nodeList[i].IsValid() ) {
				toAreaNum = PointReachableAreaNum( nodeList[i]->GetOrigin() );
				areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
				if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, nodeList[i]->GetOrigin() ) ) {
					common->Printf( "    %s: " S_COLOR_RED "NOT REACHABLE\n", nodeList[i]->GetName() );
				} else {
					common->Printf( "    %s: " S_COLOR_GREEN " REACHABLE\n", nodeList[i]->GetName() );
				}
			}
		}
	}
}

bool hhHunterSimple::TurnToward( const idVec3 &pos ) {
	idVec3 dir;
	idVec3 local_dir;
	float lengthSqr;

	if (AI_VEHICLE && InVehicle()) {
		GetVehicleInterfaceLocal()->OrientTowards( pos, 0.5 );
		return true;
	}

	dir = pos - physicsObj.GetOrigin();
#ifdef HUMANHEAD	//jsh wallwalk
	physicsObj.GetAxis().ProjectVector( dir, local_dir );
#else
	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
#endif
	local_dir.z = 0.0f;
	lengthSqr = local_dir.LengthSqr();
	if ( lengthSqr > Square( 2.0f ) || ( lengthSqr > Square( 0.1f ) && enemy.GetEntity() == NULL ) ) {
		//HUMANHEAD jsh PCF 4/29/06 changed to use enemy rather than player for directional movement
		if ( !AI_DIR_MOVEMENT || AI_PATHING || !enemy.IsValid() ) {
			AI_DIR_MOVEMENT = false;
			ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() );
			AI_DIR = HUNTER_N;
		} else {
			float toEnemy = (enemy->GetOrigin() - GetOrigin()).ToAngles().yaw;
			float toPos = (pos - GetOrigin()).ToAngles().yaw;
			if ( gameLocal.time > AI_NEXT_DIR_TIME && lengthSqr > Square( 2.0f ) ) {
				float ang = toEnemy - toPos;
				ang = idMath::AngleNormalize360( ang );
				AI_NEXT_DIR_TIME = gameLocal.time + SEC2MS(1);
				if ( ang >= 135 && ang < 225 ) {
					AI_DIR = HUNTER_S;
					ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() + 180 );
				} else if ( ang >= 45 && ang < 135 ) {
					AI_DIR = HUNTER_W;
					ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() + 90 );
				} else if ( ang >= 225 && ang < 315 ) {
					AI_DIR = HUNTER_E;
					ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() + 270 );
				} else {
					AI_DIR = HUNTER_N;
					ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() );
				}
			}
		}
	}

	bool result = FacingIdeal();
	return result;
}

void hhHunterSimple::LinkScriptVariables( void ) {
	hhMonsterAI::LinkScriptVariables();
	AI_DIR.LinkTo( scriptObject, "AI_DIR" );
	AI_DIR_MOVEMENT.LinkTo( scriptObject, "AI_DIR_MOVEMENT" );
	AI_LAST_DAMAGE_TIME.LinkTo( scriptObject, "AI_LAST_DAMAGE_TIME" );
	AI_ENEMY_RUSH.LinkTo( scriptObject, "AI_ENEMY_RUSH" );
	AI_ENEMY_RETREAT.LinkTo( scriptObject, "AI_ENEMY_RETREAT" );
	AI_ENEMY_HEALTH_LOW.LinkTo( scriptObject, "AI_ENEMY_HEALTH_LOW" );
	AI_ENEMY_RESURRECTED.LinkTo( scriptObject, "AI_ENEMY_RESURRECTED" );
	AI_ALLOW_ORDERS.LinkTo( scriptObject, "AI_ALLOW_ORDERS" );
	AI_ONGROUND.LinkTo( scriptObject, "AI_ONGROUND" );
	AI_ENEMY_SHOOTABLE.LinkTo( scriptObject, "AI_ENEMY_SHOOTABLE" );
	AI_ENEMY_LAST_SEEN.LinkTo( scriptObject, "AI_ENEMY_LAST_SEEN" );
	AI_SHOTBLOCK.LinkTo( scriptObject, "AI_SHOTBLOCK" );
	AI_NEXT_DIR_TIME.LinkTo( scriptObject, "AI_NEXT_DIR_TIME" );
	AI_BLOCKED_FAILSAFE.LinkTo( scriptObject, "AI_BLOCKED_FAILSAFE" );
	AI_KNOCKBACK.LinkTo( scriptObject, "AI_KNOCKBACK" );
}

void hhHunterSimple::Event_GetAlly() {
	if ( ally.IsValid() ) {
		idEntity *foo = ally.GetEntity();
		if ( ally->GetHealth() > 0 ) {
			idThread::ReturnEntity( ally.GetEntity() );
			return;
		} else {
			ally.Clear();
		}
	}

	idBounds bo( GetOrigin() );
	bo.ExpandSelf( 2000 );
	hhHunterSimple *entAI = NULL;
	idLocationEntity *myLoc = NULL;
	idLocationEntity *entLoc = NULL;
	float bestDistSq = 9999999;
	float distSq = 0;
	entAI = static_cast<hhHunterSimple*>(gameLocal.FindEntity( spawnArgs.GetString( "ally" ) ));
	if ( entAI && entAI->ally.IsValid() ) {
		entAI = NULL;
	}
	if ( !entAI ) {
		hhHunterSimple *hunter = NULL;
		for ( int i=0;i<hhMonsterAI::allSimpleMonsters.Num();i++ ) {
			hunter = static_cast<hhHunterSimple*>(hhMonsterAI::allSimpleMonsters[i]);
			if ( !hunter || hunter == this || hunter->health <= 0 ) {
				continue;
			}
			if ( !hunter->IsType( hhHunterSimple::Type ) || hunter->IsHidden() || hunter->ally.IsValid() ) {
				continue;
			}
			if ( !hunter->GetEnemy() || hunter->GetEnemy() != GetEnemy() ) {
				continue;
			}

			//if both locations are null, it likely means this locations haven't been set 
			//i.e. dev test map so just let the hunters to ally
			myLoc = gameLocal.LocationForPoint( GetOrigin() );
			entLoc = gameLocal.LocationForPoint( hunter->GetOrigin() );
			if ( myLoc != entLoc ) {
				continue;
			}

			distSq = (GetOrigin() - hunter->GetOrigin()).LengthSqr();
			if ( distSq > 2250000 ) { //less than 1500
				continue;
			}
			if ( distSq < bestDistSq ) {
				entAI = hunter;
				bestDistSq = distSq;
			}
		}
	}
	if ( entAI ) {
		ally.Assign( entAI );
		entAI->ally.Assign( this );
		idThread::ReturnEntity( ally.GetEntity() );
		return;
	} else {
		ally.Clear();
		idThread::ReturnEntity( NULL );
		return;
	}
}

void hhHunterSimple::Event_PrintAction( const char *action ) {
	currentAction = action;
}

void hhHunterSimple::Event_EnemyCanSee() {
	trace_t tr;
	if ( !enemy.IsValid() ) {
		idThread::ReturnInt( false );
		return;
	}

	// Check if a trace succeeds from the player to the entity
	gameLocal.clip.TracePoint( tr, enemy->GetEyePosition(), GetEyePosition(), MASK_MONSTERSOLID, enemy.GetEntity() );
	if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		idThread::ReturnInt( true );
		return;
	}
	idThread::ReturnInt( false );
}

void hhHunterSimple::Event_TriggerDelay( idEntity *ent, float delay ) {
	if ( !ent ) {
		return;
	}
	if ( delay < 0.0f ) {
		delay = 0.0f;
	}
	ent->PostEventSec( &EV_Activate, delay, this );
}

void hhHunterSimple::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( damageDef && damageDef->GetBool( "pain_knockback", "0" ) ) {
		AI_KNOCKBACK = true;
	}
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	AI_LAST_DAMAGE_TIME = MS2SEC(gameLocal.time);
}

void hhHunterSimple::Event_CallBackup( float delay ) {
	idEntity *entAI = gameLocal.FindEntity( spawnArgs.GetString( "backup" ) );
	if ( entAI && entAI->IsType( idAI::Type ) && entAI->spawnArgs.GetBool( "portal", "0" ) ) {
		if ( delay < 0.0f ) {
			delay = 0.0f;
		}
		entAI->PostEventSec( &EV_Activate, delay, this );
	}
}

bool hhHunterSimple::CanSee( idEntity *ent, bool useFov ) {
	trace_t		tr;
	idVec3		eye;
	idVec3		toPos;

	if ( ent->IsHidden() ) {
		return false;
	}

	if ( ent->IsType( idActor::Type ) ) {
		idActor *act = static_cast<idActor*>(ent);

		// If this actor is in a vehicle, look at the vehicle, not the actor
		if(act->InVehicle()) {
			ent = act->GetVehicleInterface()->GetVehicle();
		}
	}

	if ( ent->IsType( idActor::Type ) ) {
		toPos = ( ( idActor * )ent )->GetEyePosition();
	} else {
		toPos = ent->GetPhysics()->GetOrigin();
	}

	if ( useFov && !CheckFOV( toPos ) ) {
		return false;
	}

	eye = GetEyePosition();

	if ( InVehicle() ) {
		gameLocal.clip.TracePoint( tr, eye, toPos, MASK_SHOT_BOUNDINGBOX, GetVehicleInterface()->GetVehicle() ); // HUMANHEAD JRM
		if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == ent ) ) {
			return true;
		}
	} else {
		gameLocal.clip.TracePoint( tr, eye, toPos, MASK_SHOT_BOUNDINGBOX, this ); // HUMANHEAD JRM

		//if enemy is close to where the trace hit, he is shootable
		if ( tr.fraction < 1.0f && enemy.IsValid() ) {
			float hitDist = (GetEyePosition() - tr.endpos).LengthFast();
			float fullDist = (enemy->GetEyePosition() - GetEyePosition()).LengthFast();
			if ( fullDist > 0.0 ) {
				if ( hitDist / fullDist > 0.75f ) {
					AI_ENEMY_SHOOTABLE = true;
				} else {
					AI_ENEMY_SHOOTABLE = false;
				}
			}
		}

		if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == ent ) ) {
			return true;
		} else if ( bSeeThroughPortals && aas ) {
			shootTarget = NULL;
			int myArea = gameRenderWorld->PointInArea( GetOrigin() );
			int numPortals = gameRenderWorld->NumGamePortalsInArea( myArea );
			if ( numPortals > 0 ) {
				int enemyArea = gameRenderWorld->PointInArea( ent->GetOrigin() );
				for ( int i=0;i<numPortals;i++ ) {
					//if portal's destination area is the same as monster's enemy's area
					if ( gameRenderWorld->GetSoundPortal( myArea, i ).areas[0] == enemyArea ) {
						//find the portal and set it as this monster's shoottarget
						idEntity *spawnedEnt = NULL;
						for( spawnedEnt = gameLocal.spawnedEntities.Next(); spawnedEnt != NULL; spawnedEnt = spawnedEnt->spawnNode.Next() ) {
							if ( !spawnedEnt->IsType( hhPortal::Type ) ) {
								continue;
							}
			 				if ( gameRenderWorld->PointInArea( spawnedEnt->GetOrigin() ) == myArea) {
								shootTarget = spawnedEnt;
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}

void hhHunterSimple::Event_SaySound( const char *soundName ) {
	if ( AI_DEAD || ai_skipSpeech.GetBool() || gameLocal.time < nextVoiceTime ) {
		return;
	}
	//if we've got an ally, clear out this sound so that he won't say it
	if ( ally.IsValid() ) {
		ally->spawnArgs.Set( soundName, "" );
	}
	int time;
	StartSound( soundName, ( s_channelType )SND_CHANNEL_VOICE2, 0, 0, &time );
	idThread::ReturnFloat( MS2SEC( time ) );
}

void hhHunterSimple::Event_OnProjectileLand(hhProjectile *proj) {
	const function_t *newstate = NULL;
	newstate = GetScriptFunction( "state_AvoidGrenade" );
	shootTarget = proj;
	if ( newstate ) {
		SetState( newstate );
		UpdateScript();
	}
}

void hhHunterSimple::Event_CheckRush() {
	if ( enemy.IsValid() ) {
		float currentDist = (enemy->GetOrigin() - GetOrigin()).Length();
		if ( currentDist < lastEnemyDistance ) {
			//enemy is advancing
			enemyRushCount++;
			if ( enemyRushCount > spawnArgs.GetFloat( "charge_threshold", "3" ) ) {
				if ( currentDist < spawnArgs.GetFloat( "rush_distance", "400" ) ) {
					AI_ENEMY_RUSH = true;
				}
			}
		} else {
			enemyRushCount = 0;
			AI_ENEMY_RUSH = false;
		}
		lastEnemyDistance = currentDist;
	}
	PostEventSec( &MA_CheckRush, spawnArgs.GetFloat( "rush_freq", "0.5" ) );
}

void hhHunterSimple::Event_SetNextVoiceTime( int nextTime ) {
	nextVoiceTime = nextTime;
}

void hhHunterSimple::Event_CheckRetreat() {
	if ( enemy.IsValid() ) {
		float currentDist = (enemy->GetOrigin() - GetOrigin()).Length();
		if ( currentDist > lastEnemyDistance ) {
			//enemy is retreating
			enemyRetreatCount++;
			if ( enemyRetreatCount > spawnArgs.GetFloat( "retreat_threshold", "3" ) ) {
				AI_ENEMY_RETREAT = true;
			}
		} else {
			enemyRetreatCount = 0;
			AI_ENEMY_RETREAT = false;
		}
	}
	PostEventSec( &MA_CheckRetreat, spawnArgs.GetFloat( "retreat_freq", "0.5" ) );
}

void hhHunterSimple::Event_SayEnemyInfo() {
	if ( AI_DEAD || !enemy.IsValid() || !ally.IsValid() || gameLocal.time < nextVoiceTime ) {
		idThread::ReturnInt( false );
		return;
	}
	hhHunterSimple *hunterAlly = static_cast<hhHunterSimple*>(ally.GetEntity());
	if ( !hunterAlly ) {
		idThread::ReturnInt( false );
		return;
	}
	bool closerToEnemy = false;
	idVec3 meToEnemy = enemy->GetOrigin() - GetOrigin();
	idVec3 allyToEnemy = enemy->GetOrigin() - ally->GetOrigin();
	float myDistToEnemy = meToEnemy.LengthFast();
	float allyDistToEnemy = allyToEnemy.LengthFast();
	float distToAlly = (ally->GetOrigin() - GetOrigin()).LengthFast();
	if ( myDistToEnemy < allyDistToEnemy ) {
		closerToEnemy = true;
	}
	if ( !CanSee( enemy.GetEntity(), false ) ) {
		hunterAlly->SetNextVoiceTime( gameLocal.time + SEC2MS(1) );
		Event_SaySound( "snd_behind_cover" );
		idThread::ReturnInt( true );
		return;
	}
	if ( closerToEnemy ) {
		//check if enemy is underneath
		if ( allyToEnemy.z < -spawnArgs.GetFloat( "z_down_dist", "75" ) ) {
			if ( idMath::Fabs(meToEnemy.z) < spawnArgs.GetFloat( "z_equal_dist", "40" ) ) {
				Event_SaySound( "snd_down_here" );
				hunterAlly->SetNextVoiceTime( gameLocal.time + SEC2MS(1) );
				idThread::ReturnInt( true );
				return;
			}
		}
		if ( myDistToEnemy < spawnArgs.GetFloat( "over_there_dist", "400" ) ) {
			Event_SaySound( "snd_over_here" );
			hunterAlly->SetNextVoiceTime( gameLocal.time + SEC2MS(1) );
			idThread::ReturnInt( true );
			return;
		}
	} else {
		if ( myDistToEnemy > spawnArgs.GetFloat( "over_there_dist", "400" ) ) {
			Event_SaySound( "snd_over_there" );
			hunterAlly->SetNextVoiceTime( gameLocal.time + SEC2MS(1) );
			idThread::ReturnInt( true );
			return;
		}
	}
	idThread::ReturnInt( false );
}

bool hhHunterSimple::StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	const idSoundShader *shader;
	const char *sound;

	if ( length ) {
		*length = 0;
	}

	// we should ALWAYS be playing sounds from the def.
	// hardcoded sounds MUST be avoided at all times because they won't get precached.
	assert( idStr::Icmpn( soundName, "snd_", 4 ) == 0 );

	if ( !spawnArgs.GetString( soundName, "", &sound ) ) {
		return false;
	}

	if ( sound[0] == '\0' ) {
		return false;
	}

	if ( !gameLocal.isNewFrame ) {
		// don't play the sound, but don't report an error
		return true;
	}

	// HUMANHEAD nla - Check if this sound should be played every X seconds
	float minInterval;
	int   lastPlayed;
	if ( spawnArgs.GetFloat( va( "min_%s", soundName ), "0", minInterval ) && minInterval > 0 ) {
		if ( GetLastTimeSoundPlayed()->GetInt( soundName, "-1", lastPlayed ) ) {
			if ( gameLocal.time - minInterval * 1000 < lastPlayed ) {
				return( false );
			}
		}
		GetLastTimeSoundPlayed()->SetInt( soundName, gameLocal.time );
	} else if ( channel == SND_CHANNEL_VOICE2 ) {
		//only play voice over sounds once for the hunter
		if ( GetLastTimeSoundPlayed()->GetInt( soundName, "-1", lastPlayed ) ) {
			if ( lastPlayed > 0 ) {
				return( false );
			}
		}
		GetLastTimeSoundPlayed()->SetInt( soundName, gameLocal.time );		
	}
	// HUMANHEAD END

	shader = declManager->FindSound( sound );
	bool result;
	result = StartSoundShader( shader, channel, soundShaderFlags, broadcast, length );
	if ( result && shader && channel == SND_CHANNEL_VOICE2 ) {
		//print debug info for voice overs
		currentSpeech = shader->GetName();
		endSpeechTime = gameLocal.time + SEC2MS(3);
	}
	return result;
}

void hhHunterSimple::Event_GetCoverNode() {
	float			dist, bestDist;
	idEntity		*bestNode = NULL;
	idEntity		*node = NULL;
	if ( !enemy.IsValid() || !EnemyPositionValid() ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	//find the closest attack node that can see our enemy
	bestNode =  NULL;
	bestDist = 9999999.0f;
	idLocationEntity *myLocation = gameLocal.LocationForPoint( physicsObj.GetOrigin() );
	idLocationEntity *entLocation = NULL;
	for( int i = 0; i < nodeList.Num(); i++ ) {
		if ( !nodeList[i].IsValid() ) {
			continue;
		}
		if ( !nodeList[i]->IsType( hhAINode::Type ) ) {
			continue;
		}
		node = nodeList[ i ].GetEntity();
		if ( !node ) {
			continue;
		}

		//check if node is used
		if ( node->IsType( hhAINode::Type ) ) {
			hhAINode *aiNode = static_cast<hhAINode*>(node);
			if ( aiNode && aiNode->user.IsValid() && !aiNode->user.IsEqualTo( this ) ) {
				if ( !aiNode->user->IsHidden() && aiNode->user->GetHealth() > 0 ) {
					continue;
				} else { 
					aiNode->user.Clear();
				}
			}
		}

		//skip if node is farther than enemy or enemy is closer to object than i am
		float distToNode = TravelDistance( GetOrigin(), node->GetOrigin() );
		float distEnemyToNode = (enemy->GetOrigin() - node->GetOrigin()).Length();
		if ( distEnemyToNode < distToNode || distEnemyToNode < 200 ) {
			continue;
		}
		//skip if object is farther than enemy or if enemy is closer to object than i am
		if ( distToNode > 1.0 && distToNode / distEnemyToNode > 2.0f ) {
			continue;
		}

		//skip if enemy can see the node
		trace_t		tr;
		gameLocal.clip.TracePoint( tr, enemy->GetEyePosition(), node->GetOrigin(), MASK_SHOT_BOUNDINGBOX, this );
		if ( tr.fraction >= 1.0f ) {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugArrow( colorMagenta, enemy->GetEyePosition(), node->GetOrigin(), 30, 5000 );
			}
			continue;
		}

		//find closest node
		idVec3 org = node->GetPhysics()->GetOrigin();
		dist = ( physicsObj.GetOrigin() - org ).LengthSqr();
		if ( dist > bestDist ) {
			continue;
		}

		//found a node that passed all tests
		bestNode = node;
		bestDist = dist;
	}
	if ( bestNode && bestNode->IsType( hhAINode::Type ) ) {
		hhAINode *bestAINode = static_cast<hhAINode*>(bestNode);
		if ( bestAINode ) {
			bestAINode->user.Assign( this );
		}
	}
	if ( bestNode ) {
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorGreen, enemy->GetEyePosition(), bestNode->GetOrigin(), 30, 5000 );
		}
		idThread::ReturnVector( bestNode->GetOrigin() );
		return;
	}
	idThread::ReturnVector( vec3_zero );
}

void hhHunterSimple::Event_GetCoverPoint() {
	if ( !enemy.IsValid() ) {
		idThread::ReturnVector( vec3_zero );
		return;
	}
	//couldn't find a node. try sampling some nearby points
	idVec3 testPoint;
	float bestDist = 9999999.0f;
	idVec3 finalPoint = vec3_zero;
	int finalArea = -1;
	bool clipped = false;
	float yaw = (GetOrigin() - enemy->GetOrigin()).ToYaw();
	int num;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];
	idBounds bounds;
	trace_t tr;
	float distance = spawnArgs.GetFloat( "cover_range", "200" );
	float initialYaw = yaw;
	for ( int i=0;i<16;i++ ) {
		testPoint = GetOrigin() + distance * idAngles( 0, yaw, 0 ).ToForward();
		testPoint.z += 35;
		yaw += 22.5;

		if( gameLocal.clip.Contents( testPoint, GetPhysics()->GetClipModel(), viewAxis, CONTENTS_SOLID, this ) != 0 ) {
			continue;
		}

		//skip if node is farther than enemy or enemy is closer to object than i am
		float distToNode = TravelDistance( GetOrigin(), testPoint );
		float distEnemyToNode = (enemy->GetOrigin() - testPoint).Length();
		if ( distEnemyToNode < distToNode || distEnemyToNode < 200 ) {
			continue;
		}
		//skip if object is farther than enemy or if enemy is closer to object than i am
		if ( distToNode > 1.0 && distToNode / distEnemyToNode > 2.0f ) {
			continue;
		}

		//make sure it wont clip into anything at testPoint
		clipped = false;
		bounds.FromTransformedBounds( GetPhysics()->GetBounds(), testPoint, GetPhysics()->GetAxis() );
		num = gameLocal.clip.ClipModelsTouchingBounds( bounds, MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
		for ( int j = 0; j < num; j++ ) {
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
		}

		//make sure we can path there
		aasPath_t	path;
		int toAreaNum = PointReachableAreaNum( testPoint );
		int areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, testPoint ) ) {
			continue;
		}

		//make sure enemy cant see the point 
		gameLocal.clip.TracePoint( tr, enemy->GetEyePosition(), testPoint, MASK_SHOT_BOUNDINGBOX, this );
		if ( tr.fraction >= 1.0f ) {
			continue;
		}

		float distToPoint = (GetOrigin() - testPoint).Length();
		if ( distToPoint < bestDist ) {
			finalPoint = testPoint;
			bestDist = distToPoint;
			finalArea = toAreaNum;
		}
	}

	if ( finalPoint != vec3_zero ) {
		if ( aas ) {
			aas->PushPointIntoAreaNum( finalArea, finalPoint );
		}
		idThread::ReturnVector( finalPoint );
		return;
	}

	idThread::ReturnVector( vec3_zero );
}

void hhHunterSimple::Event_GetSightNode() {
	if ( !enemy.IsValid() ) {
		idThread::ReturnVector( vec3_zero );
		return;
	}

	//find a good spot to attack from
	trace_t tr;
	float distance = spawnArgs.GetFloat( "attack_range", "500" );
	idVec3 testPoint;
	idVec3 finalPoint = vec3_zero;
	bool clipped = false;
	float yaw = (GetOrigin() - enemy->GetOrigin()).ToYaw();
	float initialYaw = yaw;
	idEntity		*bestNode = NULL;
	idEntity		*node = NULL;

	//find an attack node that can see our enemy
	for( int i = 0; i < nodeList.Num(); i++ ) {
		if ( !nodeList[i].IsValid() ) {
			continue;
		}
		if ( !nodeList[i]->IsType( hhAINode::Type ) ) {
			continue;
		}
		node = nodeList[ i ].GetEntity();
		if ( !node ) {
			continue;
		}

		//check if node is used
		if ( node->IsType( hhAINode::Type ) ) {
			hhAINode *aiNode = static_cast<hhAINode*>(node);
			if ( aiNode && aiNode->user.IsValid() && !aiNode->user.IsEqualTo( this ) ) {
				if ( !aiNode->user->IsHidden() && aiNode->user->GetHealth() > 0 ) {
					continue;
				} else { 
					aiNode->user.Clear();
				}
			}
		}

		//check if we can see enemy from this node
		idVec3 eye = testPoint + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
		//HUMANHEAD jsh PCF 4/27/06 changed trace to end at enemy origin instead of enemy eye position to fix slowdown
		gameLocal.clip.TracePoint( tr, eye, enemy->GetOrigin(), MASK_SHOT_BOUNDINGBOX, enemy.GetEntity() );
		if ( tr.fraction < 1.0f || ( gameLocal.GetTraceEntity( tr ) != enemy.GetEntity() ) ) {
			continue;
		}

		//found a node that passed all tests
		bestNode = node;
	}
	if ( bestNode && bestNode->IsType( hhAINode::Type ) ) {
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorGreen, GetOrigin(), bestNode->GetOrigin(), 10, 5000 );
		}
		idThread::ReturnVector( bestNode->GetOrigin() );
		return;
	}
	idThread::ReturnVector( vec3_zero );
}

void hhHunterSimple::Event_GetNearSightPoint() {
	trace_t tr;
	float distance = spawnArgs.GetFloat( "attack_range", "500" );
	idVec3 testPoint;
	idVec3 finalPoint = vec3_zero;
	bool clipped = false;
	float yaw = 0;
	float initialYaw = 0;
	int num, i, j;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];
	idBounds bounds;
	int finalArea = -1;
	idList<idVec3> listo;

	if ( !enemy.IsValid() ) {
		idThread::ReturnVector( vec3_zero );
		return;
	}

	yaw = (GetOrigin() - enemy->GetOrigin()).ToYaw();
	initialYaw = yaw;
	//test 8 points around the monster, starting with one directly in front of it
	for ( i=0;i<32;i++ ) {
		testPoint = GetOrigin() + distance * idAngles( 0, yaw, 0 ).ToForward();
		testPoint.z += spawnArgs.GetFloat( "attack_z", "300" );
		yaw += 11.25;

		//make sure it wont clip into anything at testPoint
		clipped = false;
		bounds.FromTransformedBounds( GetPhysics()->GetBounds(), testPoint, GetPhysics()->GetAxis() );
		num = gameLocal.clip.ClipModelsTouchingBounds( bounds, MASK_MONSTERSOLID, clipModels, MAX_GENTITIES );
		for ( j = 0; j < num; j++ ) {
			cm = clipModels[ j ];
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
		}

		//make sure we can path there
		int toAreaNum = PointReachableAreaNum( testPoint );
		int areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
		aasPath_t	path;
		if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, testPoint ) ) {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugArrow( colorYellow, GetOrigin(), testPoint, 10, 5000 );
			}
			continue;
		}

		//make sure we can see enemy there
		idVec3 eye = testPoint + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
		gameLocal.clip.TracePoint( tr, eye, enemy->GetEyePosition(), MASK_SHOT_BOUNDINGBOX, enemy.GetEntity() );
		if ( tr.fraction < 1.0f ) {
			if ( ai_debugBrain.GetBool() ) {
				gameRenderWorld->DebugArrow( colorRed, eye, enemy->GetEyePosition(), 10, 5000 );
			}
			continue;
		}

		listo.Append( testPoint );
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorGreen, GetOrigin(), testPoint, 10, 5000 );
		}
		finalArea = toAreaNum;
	}
	if ( listo.Num() ) {
		finalPoint = listo[gameLocal.random.RandomInt(listo.Num())];
	}
	if ( finalPoint != vec3_zero ) {
		if ( aas && finalArea != -1 ) {
			aas->PushPointIntoAreaNum( finalArea, finalPoint );
		}
		idThread::ReturnVector( finalPoint );
		return;
	}
	idThread::ReturnVector( vec3_zero );
}

void hhHunterSimple::Show() {
	flashlightTime = gameLocal.time + SEC2MS( spawnArgs.GetInt( "flashlight_delay" ) );
	hhMonsterAI::Show();
}

void hhHunterSimple::Event_EnemyPortal( idEntity *ent ) {
	//HUMANHEAD jsh PCF 4/27/06 Added check for enemy and hiddenness
	if ( !enemy.IsValid() || AI_PATHING || fl.hidden ) {
		return;
	}
	enemyPortal = ent;
	SetState( GetScriptFunction( "state_PortalAttack" ) );
}

void hhHunterSimple::Event_GetEnemyPortal() {
	if ( enemyPortal.IsValid() ) {
		idThread::ReturnEntity( enemyPortal.GetEntity() );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

void hhHunterSimple::Event_OnProjectileHit(hhProjectile *proj) {
	if ( !enemy.IsValid() || !proj || AI_VEHICLE ) {
		return;
	}
	if ( ( proj->GetOrigin().ToVec2() - GetOrigin().ToVec2() ).LengthFast() < 150 ) {
		AI_SHOTBLOCK = true;
	}
}

void hhHunterSimple::BlockedFailSafe() {
	if ( move.moveCommand < NUM_NONMOVING_COMMANDS || !ai_blockedFailSafe.GetBool() || blockedRadius < 0.0f || !enemy.IsValid() || AI_PATHING ) {
		return;
	}
	if ( gameLocal.time > nextBlockCheckTime ) {
		if ( ( lastMoveOrigin - physicsObj.GetOrigin() ).Length() < blockedRadius ) {
			AI_BLOCKED_FAILSAFE = true;
		}
		lastMoveOrigin = physicsObj.GetOrigin();
		nextBlockCheckTime = gameLocal.time + blockedMoveTime;
	}
}
void hhHunterSimple::UpdateEnemyPosition( void ) {
	idActor *enemyEnt = enemy.GetEntity();
	int				enemyAreaNum;
	int				areaNum;
	aasPath_t		path;
	predictedPath_t predictedPath;
	idVec3			enemyPos;
	bool			onGround;

	if ( !enemyEnt ) {
		return;
	}

	const idVec3 &org = physicsObj.GetOrigin();

	if ( move.moveType == MOVETYPE_FLY ) {
		enemyPos = enemyEnt->GetPhysics()->GetOrigin();
		onGround = true;
	} else {
		onGround = enemyEnt->GetFloorPos( 64.0f, enemyPos );
		if ( enemyEnt->OnLadder() ) {
			onGround = false;
		}
	}

	if ( onGround ) {
		// when we don't have an AAS, we can't tell if an enemy is reachable or not,
		// so just assume that he is.
		if ( !aas ) {
			enemyAreaNum = 0;
			lastReachableEnemyPos = enemyPos;
		} else {
			enemyAreaNum = PointReachableAreaNum( enemyPos, 1.0f );
			if ( enemyAreaNum ) {
				areaNum = PointReachableAreaNum( org );
				if ( PathToGoal( path, areaNum, org, enemyAreaNum, enemyPos ) ) {
					lastReachableEnemyPos = enemyPos;
				}
			}
		}
	}

	AI_ENEMY_IN_FOV		= false;
	AI_ENEMY_VISIBLE	= false;

	if ( CanSee( enemyEnt, false ) ) {
		AI_ENEMY_VISIBLE = true;
		if ( CheckFOV( enemyEnt->GetPhysics()->GetOrigin() ) ) {
			AI_ENEMY_IN_FOV = true;
		}

		SetEnemyPosition();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorLtGrey, enemyEnt->GetPhysics()->GetBounds(), lastReachableEnemyPos, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorWhite, enemyEnt->GetPhysics()->GetBounds(), lastVisibleReachableEnemyPos, gameLocal.msec );
	}
}

//HUMANHEAD jsh PCF 4/27/06 increased the upper z zalue of the bounds to 90.0f
bool hhHunterSimple::ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const {
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
			idBounds bnds( idVec3( -16.0, -16.0f, -8.0f ), idVec3( 16.0, 16.0f, 90.0f ) );
			bnds.TranslateSelf( physicsObj.GetOrigin() );	
			if ( bnds.ContainsPoint( pos ) ) {
				return true;
			}
		}
	}
	return false;
}

void hhHunterSimple::Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy ) {
	HH_ASSERT( player == enemy.GetEntity() );
	//HUMANHEAD jsh PCF 4/29/06 stop looking and make enemy not visible for a frame
	Event_LookAtEntity( NULL, 0.0f );
	AI_ENEMY_VISIBLE = false;
	enemy = proxy;
	nextSpiritCheck = gameLocal.time + SEC2MS( spawnArgs.GetFloat( "spirit_check_delay", "5" ) );
}

void hhHunterSimple::Distracted( idActor *newEnemy ) {
	if ( newEnemy && newEnemy->IsType( hhTalon::Type ) ) {
		SetState( GetScriptFunction( "state_TalonCombat" ) );
	} else {
		SetState( GetScriptFunction( "state_Combat" ) );
	}

	SetEnemy( newEnemy );
}

void hhHunterSimple::Event_EnemyVehicleDocked() {
	if ( enemy.IsValid() && enemy->InVehicle() && enemy->GetVehicleInterface() ) {
		hhVehicle *vehicle = enemy->GetVehicleInterface()->GetVehicle();
		if ( vehicle && vehicle->IsDocked() ) {
			idThread::ReturnInt( true );
			return;
		}
	}
	idThread::ReturnInt( false );
}

void hhHunterSimple::Event_Blocked() {
	AI_NEXT_DIR_TIME = 0;
	StopMove( MOVE_STATUS_DONE );
}