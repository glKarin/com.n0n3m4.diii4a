
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef MA_KeeperStartBlast("keeper_StartBlast", "e"); 
const idEventDef MA_KeeperEndBlast("keeper_EndBlast", NULL); 
const idEventDef MA_KeeperUpdateTelepathicThrow("keeper_UpdateTelepathicThrow", NULL, NULL);
const idEventDef MA_KeeperTelepathicThrow("keeper_TelepathicThrow", NULL, NULL);
const idEventDef MA_KeeperStartTeleport("keeper_StartTeleport", NULL);
const idEventDef MA_KeeperEndTeleport("keeper_EndTeleport", NULL);
const idEventDef MA_KeeperTeleportExit("keeper_TeleportExit", NULL);
const idEventDef MA_KeeperCreatePortal("keeper_createPortal", NULL);
const idEventDef MA_KeeperEnableShield("keeper_enableShield", NULL);
const idEventDef MA_KeeperDisableShield("keeper_disableShield", NULL);
const idEventDef MA_KeeperAssignShieldFx( "<assignShieldFx>", "e" );
const idEventDef MA_KeeperTrigger("keeper_Trigger", NULL, NULL);
const idEventDef MA_KeeperTeleportEnter("keeper_TeleportEnter", NULL);
const idEventDef MA_KeeperStartHeadFx("keeper_StartHeadFx");
const idEventDef MA_KeeperEndHeadFx("keeper_EndHeadFx");
const idEventDef MA_KeeperAssignHeadFx( "<assignHeadFx>", "e" );
const idEventDef MA_KeeperGetThrowEntity( "keeper_GetThrowEntity", NULL, 'e' );
const idEventDef MA_KeeperGetTriggerEntity( "keeper_GetTriggerEntity", NULL, 'e' );
const idEventDef MA_KeeperTriggerEntity( "keeper_TriggerEntity", "e" );
const idEventDef MA_KeeperThrowEntity( "keeper_ThrowEntity", "e" );
const idEventDef MA_KeeperForceDisableShield("keeper_forceDisableShield", NULL);

CLASS_DECLARATION( hhMonsterAI, hhKeeperSimple )
	EVENT( MA_KeeperStartBlast,					hhKeeperSimple::Event_StartBlast )
	EVENT( MA_KeeperEndBlast,					hhKeeperSimple::Event_EndBlast )
	EVENT( MA_KeeperUpdateTelepathicThrow,		hhKeeperSimple::Event_UpdateTelepathicThrow )
	EVENT( MA_KeeperTelepathicThrow,			hhKeeperSimple::Event_TelepathicThrow )
	EVENT( MA_KeeperStartTeleport,				hhKeeperSimple::Event_StartTeleport )
	EVENT( MA_KeeperEndTeleport,				hhKeeperSimple::Event_EndTeleport )
	EVENT( MA_KeeperTeleportExit,				hhKeeperSimple::Event_TeleportExit )
	EVENT( MA_KeeperCreatePortal,				hhKeeperSimple::Event_CreatePortal )
	EVENT( MA_KeeperEnableShield,				hhKeeperSimple::Event_EnableShield )
	EVENT( MA_KeeperDisableShield,				hhKeeperSimple::Event_DisableShield )
	EVENT( MA_KeeperAssignShieldFx,				hhKeeperSimple::Event_AssignShieldFx )
	EVENT( MA_KeeperTrigger,					hhKeeperSimple::Event_KeeperTrigger )
	EVENT( MA_KeeperTeleportEnter,				hhKeeperSimple::Event_TeleportEnter )
	EVENT( MA_KeeperStartHeadFx,				hhKeeperSimple::Event_StartHeadFx )
	EVENT( MA_KeeperEndHeadFx,					hhKeeperSimple::Event_EndHeadFx )
	EVENT( MA_KeeperAssignHeadFx,				hhKeeperSimple::Event_AssignHeadFx )
	EVENT( MA_KeeperTriggerEntity,				hhKeeperSimple::Event_TriggerEntity )
	EVENT( MA_KeeperThrowEntity,				hhKeeperSimple::Event_ThrowEntity )
	EVENT( MA_KeeperGetTriggerEntity,			hhKeeperSimple::Event_GetTriggerEntity )
	EVENT( MA_KeeperGetThrowEntity,				hhKeeperSimple::Event_GetThrowEntity )
	EVENT( MA_KeeperForceDisableShield,			hhKeeperSimple::Event_ForceDisableShield )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

hhKeeperSimple::hhKeeperSimple() {
	shieldAlpha = 0.0f;
}

hhKeeperSimple::~hhKeeperSimple() {
	idEntity *ent = NULL;
	if ( throwEntity.IsValid() ) {
		ent = throwEntity.GetEntity();
	} else if ( targetReaction.entity.IsValid() ) {
		ent = targetReaction.entity.GetEntity();
	} else {
		return;
	}

	if ( ent && ent->IsType(hhMoveable::Type)) {
		hhMoveable *throwMoveable = static_cast<hhMoveable*>(ent);	
		if ( throwMoveable ) {
			throwMoveable->Event_Unhover();
		}
	}
}

void hhKeeperSimple::Spawn( void ) {
	beamAttack = idEntityPtr<hhBeamSystem> ( hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "beamAttack" ) ) );
	if(beamAttack.IsValid()) {
		beamAttack->MoveToJoint( this, spawnArgs.GetString("bone_beamAttack"));
		beamAttack->BindToJoint( this, spawnArgs.GetString("bone_beamAttack"), false );		
		beamAttack->Activate(FALSE);
	}	
	
	beamTelepathic	= idEntityPtr<hhBeamSystem> (hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "beamTelepathic" ) ) );
	if(beamTelepathic.IsValid()) {
		beamTelepathic->MoveToJoint( this, spawnArgs.GetString("bone_beamTelepathic"));
		beamTelepathic->BindToJoint( this, spawnArgs.GetString("bone_beamTelepathic"), false );
		beamTelepathic->Activate(FALSE);
		beamTelepathic->Hide();
	}
	if ( spawnArgs.GetInt( "always_shield", "0" ) && !spawnArgs.GetBool( "hide", "0" ) ) {
		Event_EnableShield();
	}
	nextShieldImpact = gameLocal.time;
	bThrowing = false;
}

void hhKeeperSimple::Think() {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) {
		return;
	}

	idEntity *ent = NULL;
	if ( throwEntity.IsValid() ) { 
		ent = throwEntity.GetEntity();
	} else if ( targetReaction.entity.IsValid() ) {
		ent = targetReaction.entity.GetEntity();
	}

	// Draw beam from our brain to the object we are controlling
	if ( beamTelepathic.IsValid() ) {
		if( bThrowing && ent && !beamTelepathic->IsHidden() && beamTelepathic->IsActivated() ) {
			beamTelepathic->SetTargetLocation(ent->GetOrigin());
		} else {
			beamTelepathic->Activate( false );
			beamTelepathic->Hide();
		}
	}

	if ( shieldFx.IsValid() ) {
		if ( AI_SHIELD ) {
			//fade on shield
			shieldAlpha += spawnArgs.GetFloat( "shield_delta", "0.05" );
			if ( shieldAlpha > 1.0 ) {
				shieldAlpha = 1.0f;
			}
		} else {
			//fade off shield
			shieldAlpha -= spawnArgs.GetFloat( "shield_delta", "0.05" );
			if ( shieldAlpha < 0.0 ) {
				shieldAlpha = 0.0f;
			}
		}
		shieldFx->SetParticleShaderParm( SHADERPARM_TIME_OF_DEATH, shieldAlpha );
	}

	hhMonsterAI::Think();

	if ( physicsObj.HasGroundContacts() && !physicsObj.HadGroundContacts() ) {
		AI_LANDED = true;
	}
}

void hhKeeperSimple::Event_StartBlast( idEntity *ent) {
	if( beamAttack.IsValid() ) {
		hhFxInfo fxInfo;
		fxInfo.SetEntity( ent );
		fxInfo.RemoveWhenDone( true );	
		BroadcastFxInfo( spawnArgs.GetString("fx_portalstart"), ent->GetOrigin() + (spawnArgs.GetVector( "portal_start_offset" ) * ent->GetAxis()), ent->GetAxis(), &fxInfo );
		beamAttack->SetTargetLocation( ent->GetOrigin() + ent->GetAxis() * spawnArgs.GetVector( "telepathic_beam_offset" ) );
		beamAttack->Activate(TRUE);
	}
}

void hhKeeperSimple::Event_EndBlast() {
	if( beamAttack.IsValid() ) {
		beamAttack->Activate(FALSE);
	}
}

void hhKeeperSimple::Event_UpdateTelepathicThrow() {
	idEntity *ent = NULL;
	if ( throwEntity.IsValid() ) { 
		ent = throwEntity.GetEntity();
	} else if ( targetReaction.entity.IsValid() ) {
		ent = targetReaction.entity.GetEntity();
	} else {
		return;
	}
	if ( !ent || !ent->IsType(hhMoveable::Type) ) {
		return;
	}
	bThrowing = true;
	if ( beamTelepathic.IsValid() ) {
		if ( beamTelepathic->IsHidden() ) {
			beamTelepathic->Show();
		}
		beamTelepathic->Activate( true );
	}

	hhMoveable *throwMoveable = static_cast<hhMoveable*>(ent);
	idVec3 hoverToPos;

	if(GetEnemy()) {
		UpdateEnemyPosition();
		idVec3 toEnemy = GetLastEnemyPos() - GetOrigin();
		float dist = toEnemy.Normalize();
		hoverToPos = GetOrigin() + toEnemy * dist * 0.25f;
		hoverToPos.z = GetOrigin().z + EyeHeight() + 200.0f;
	}
	else {
		 hoverToPos = throwMoveable->GetOrigin() + idVec3(0.0f, 0.0f, 128.0f);
	}
	throwMoveable->AllowImpact( true );
	throwMoveable->Event_HoverTo(hoverToPos);
	throwMoveable->GetPhysics()->SetAngularVelocity(idVec3(10.0f, 10.0f, 10.0f));
	// HUMANHEAD mdl:  Beef up the damage on movables thrown by the keeper
	throwMoveable->SetDamageDef( spawnArgs.GetString( "moveableDamageDef", "damage_movable20" ) );
	PostEventMS(&MA_KeeperUpdateTelepathicThrow, 100);
}

void hhKeeperSimple::Event_TelepathicThrow() {
	idEntity *ent = NULL;
	bThrowing = false;
	if ( beamTelepathic.IsValid() ) {
		beamTelepathic->Hide();
	}
	if ( throwEntity.IsValid() ) { 
		ent = throwEntity.GetEntity();
	} else if ( targetReaction.entity.IsValid() ) {
		ent = targetReaction.entity.GetEntity();
	} else {
		return;
	}
	
	// Start the object hovering
	if ( !ent || !ent->IsType(hhMoveable::Type) ) {
		return;
	}

	targetReaction.entity.Clear();
	CancelEvents( &MA_KeeperUpdateTelepathicThrow );
	hhMoveable *throwMoveable = static_cast<hhMoveable*>(ent);	
	if ( throwMoveable && throwMoveable->IsType( hhMoveable::Type ) ) {
		throwMoveable->Event_Unhover();
	}

	// Throw it at an enemy if we have one
	if(GetEnemy()) {
		idVec3 toEnemy = GetEnemy()->GetAimPosition() - throwMoveable->GetOrigin();
		toEnemy.Normalize();
		throwMoveable->GetPhysics()->SetLinearVelocity(toEnemy * spawnArgs.GetFloat("telepathic_throw_velocity", "2000"));
	}
}

void hhKeeperSimple::Event_TeleportExit() {
	const function_t *newstate = NULL;
	newstate = GetScriptFunction( "state_TeleportExit" );
	if ( newstate ) {
		SetState( newstate );
		UpdateScript();
	}
}

void hhKeeperSimple::Event_TeleportEnter() {
	const function_t *newstate = NULL;
	newstate = GetScriptFunction( "state_TeleportEnter" );
	if ( newstate ) {
		SetState( newstate );
		UpdateScript();
	}
}

void hhKeeperSimple::Event_StartTeleport() {
	Hide();
}

void hhKeeperSimple::Event_EndTeleport() {
	idVec3 newOrig;
	idEntity *ent;
	idList<idEntity*> nodeList; 
	float bestScore = -1;
	float bestNodeIndex = -1;

	// Find the best position to teleport to
	// Find where we want to teleport to
	idStr randomNode = spawnArgs.RandomPrefix( "teleport_info", gameLocal.random );
	ent = gameLocal.FindEntity( randomNode );
	if ( ent ) {
		newOrig = ent->GetOrigin();
	} else {
		newOrig = GetOrigin();
		gameLocal.Warning( "%s doesn't have any teleport infos", name.c_str() );
	}

	//teleport and face enemy
	SetOrigin( newOrig );
	if( GetEnemy() ) {
		TurnToward(GetEnemy()->GetOrigin());
        current_yaw = ideal_yaw;		
		Turn();
	}	
}

float hhKeeperSimple::GetTeleportDestRating(const idVec3 &pos) {
	// Make sure this spot doesn't contain something else 
	idEntity *entList[MAX_GENTITIES];
	idBounds myNewBnds = this->GetPhysics()->GetBounds();
	myNewBnds = myNewBnds.Translate(pos);
	myNewBnds.ExpandSelf(24.0f);
	int numHit = gameLocal.clip.EntitiesTouchingBounds(myNewBnds, -1, entList, MAX_GENTITIES);
	if(numHit > 0) {
		for(int i=0;i<numHit;i++) {
			if(entList[i] && !entList[i]->IsHidden() && (entList[i]->IsType(hhMoveable::Type) || entList[i]->IsType(idActor::Type))) {
				return -1;
			}
		}		
	}

	float w = gameLocal.random.RandomFloat();

	return idMath::ClampFloat(-1.0f, 1.0f, w);
}

#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhKeeperSimple::LinkScriptVariables() {
	hhMonsterAI::LinkScriptVariables();
	LinkScriptVariable( AI_LANDED );
	LinkScriptVariable( AI_SHIELD );
}

void hhKeeperSimple::Event_CreatePortal (void) {
	static const char *	passPrefix = "portal_";
	const char *	portalDef;
	idEntity *		portal;
	idDict			portalArgs;
	idList<idStr>	xferKeys;
	idList<idStr>	xferValues;

	const idKeyValue *buddyKV = spawnArgs.FindKey( "portal_buddy" );
	if ( buddyKV && buddyKV->GetValue().Length() && gameLocal.FindEntity(buddyKV->GetValue().c_str()) ) {
		// Case of a valid portal_buddy key, make a real portal
		portalDef = spawnArgs.GetString( "def_portal" );
	} else if ( spawnArgs.FindKey( "portal_cameraTarget" ) ) { // The monster portal has a camera target, so use a real portal instead of the fake portal
		portalDef = spawnArgs.GetString( "def_portal" );
	} else {
		portalDef = spawnArgs.GetString( "def_fakeportal" );
	}

	if ( !portalDef || !portalDef[0] ) {
		return;	
	}

	// Set the origin of the portal to us. 
	portalArgs.SetVector( "origin", GetOrigin() );
	portalArgs.Set( "rotation", spawnArgs.GetString( "rotation" ) );
	portalArgs.Set( "remove_on_close", "1" );	

	// Pass along all 'portal_' keys to the portal's spawnArgs;
	hhUtils::GetKeysAndValues( spawnArgs, passPrefix, xferKeys, xferValues );
	for ( int i = 0; i < xferValues.Num(); ++i ) {
		xferKeys[ i ].StripLeadingOnce( passPrefix );
		portalArgs.Set( xferKeys[ i ].c_str(), xferValues[ i ].c_str() );
	}

	// Spawn the portal
	portal = gameLocal.SpawnObject( portalDef, &portalArgs );
	if ( !portal ) {
		return;
	}

	// Move the portal up some pre determinted amt, since its origin is in the middle of it
	trace_t tr;	
	gameLocal.clip.TracePoint(tr, GetOrigin(), GetOrigin() + idVec3(0,0,-200), MASK_MONSTERSOLID, this);	
	if ( tr.fraction < 1.0f ) {
		portal->GetPhysics()->SetOrigin( tr.c.point + idVec3( 0,0,2 ) );
	} else {
		float offset = spawnArgs.GetFloat( "offset_portal", 0 );
		portal->GetPhysics()->SetOrigin( portal->GetPhysics()->GetOrigin() + (portal->GetAxis()[2] * offset) );
	}

	// Update the camera stuff
	portal->ProcessEvent( &EV_UpdateCameraTarget );

	// Open the portal - Need to delay this, so that PostSpawn gets called/sets up the partner portal
	//? Should we always pass in the player?
	portal->PostEventSec( &EV_Activate, 0, gameLocal.GetLocalPlayer() );
}

void hhKeeperSimple::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );
	if ( beamAttack.IsValid() ) {
		SAFE_REMOVE( beamAttack );
	}
	if( beamTelepathic.IsValid() ) {
		SAFE_REMOVE( beamTelepathic );
	}
	hhMoveable *throwMoveable = static_cast<hhMoveable*>(targetReaction.entity.GetEntity());	
	if ( throwMoveable && throwMoveable->IsType( hhMoveable::Type ) ) {
		targetReaction.entity.Clear();
		throwMoveable->Event_Unhover();
	}
	if ( renderEntity.customSkin == NULL ) {
		SetSkinByName( "skins/monsters/keeper_nolegs" );
	}
}

void hhKeeperSimple::Event_EnableShield(void) {
	if ( spawnArgs.GetInt( "never_shield", "0" ) ) {
		return;
	}
	fl.applyDamageEffects = false;
	AI_SHIELD = true;
	if ( shieldFx.IsValid() ) {
		SAFE_REMOVE( shieldFx );
	}
	hhFxInfo fxInfo;
	fxInfo.SetEntity( this );
	fxInfo.SetBindBone( "head" );
	fxInfo.RemoveWhenDone( true );	
	BroadcastFxInfo( spawnArgs.GetString("fx_shield"), GetOrigin(), mat3_identity, &fxInfo, &MA_KeeperAssignShieldFx );				
}

void hhKeeperSimple::Event_ForceDisableShield(void) {
	fl.applyDamageEffects = true;
	AI_SHIELD = false;
}

void hhKeeperSimple::Event_DisableShield(void) {
	if ( spawnArgs.GetInt( "always_shield", "0" ) ) {
		return;
	}
	fl.applyDamageEffects = true;
	AI_SHIELD = false;
}

void hhKeeperSimple::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int loc ) {
	if ( AI_SHIELD ) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
		if ( damageDef && damageDef->GetBool( "spirit_damage" ) ) {
			hhMonsterAI::Damage(inflictor, attacker, dir, damageDefName, damageScale, loc);
		}
		if ( inflictor ) {
			hhFxInfo fxInfo;
			fxInfo.RemoveWhenDone( true );
			if ( gameLocal.time >= nextShieldImpact ) {
				nextShieldImpact = gameLocal.time + int(spawnArgs.GetFloat( "shield_impact_freq", "0.4" ) * 1000);
				BroadcastFxInfoPrefixed( "fx_shield_impact", inflictor->GetOrigin() + GetAxis()*idVec3(50,0,0), mat3_identity, &fxInfo );
			}
			StartSound( "snd_shield_impact", SND_CHANNEL_BODY );
			if ( inflictor->IsType( idProjectile::Type ) ) {
				inflictor->PostEventMS( &EV_Remove, 0 );
			}
		}
	} else {
		hhMonsterAI::Damage(inflictor, attacker, dir, damageDefName, damageScale, loc);
	}
}

void hhKeeperSimple::Event_AssignShieldFx( hhEntityFx* fx ) {
	shieldFx = fx;
}

void hhKeeperSimple::Event_StartHeadFx() {
	hhFxInfo fxInfo;
	fxInfo.SetEntity( this );
	fxInfo.SetBindBone( spawnArgs.GetString( "bone_telepathic_fx", "head" ) );
	fxInfo.RemoveWhenDone( true );	
	BroadcastFxInfo( spawnArgs.GetString("fx_telepathic"), GetOrigin(), mat3_identity, &fxInfo, &MA_KeeperAssignHeadFx );
}

void hhKeeperSimple::Event_EndHeadFx() {
	SAFE_REMOVE( headFx );
}

void hhKeeperSimple::Event_AssignHeadFx( hhEntityFx* fx ) {
	headFx = fx;
}

void hhKeeperSimple::Event_KeeperTrigger() {
	if ( targetReaction.entity.IsValid() ) {
		targetReaction.entity->ProcessEvent( &EV_Activate, this );
		hhReaction *reaction = targetReaction.GetReaction();
		if (reaction) {
			reaction->active = false;
		}
	}
}

idProjectile *hhKeeperSimple::LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef ) {	//HUMANHEAD mdc - added desiredProjectileDef for supporting multiple projs.
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
		//dir = axis[ 0 ] + axis[ 2 ] * ( 0 * idMath::Sin( spin ) ) - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir = axis[ 0 ] - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
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

void hhKeeperSimple::Event_GetTriggerEntity() {
	idThread::ReturnEntity( triggerEntity.GetEntity() );
}

void hhKeeperSimple::Event_TriggerEntity( idEntity *ent ) {
	if ( !ent ) {
		return;
	}
	triggerEntity = ent;
	const function_t *newstate = NULL;
	newstate = GetScriptFunction( "state_TriggerEntity" );
	if ( newstate ) {
		SetState( newstate );
		UpdateScript();
	}
}

void hhKeeperSimple::Event_GetThrowEntity() {
	idThread::ReturnEntity( throwEntity.GetEntity() );
}

void hhKeeperSimple::Event_ThrowEntity( idEntity *ent ) {
	if ( !ent || !ent->IsType( idMoveable::Type ) ) {
		return;
	}
	throwEntity = ent;
	const function_t *newstate = NULL;
	newstate = GetScriptFunction( "state_ThrowEntity" );
	if ( newstate ) {
		SetState( newstate );
		UpdateScript();
	}
}

void hhKeeperSimple::Hide() {
	Event_DisableShield();
	hhMonsterAI::Hide();
}

void hhKeeperSimple::Show() {
	hhMonsterAI::Show();
	if ( spawnArgs.GetInt( "always_shield", "0" ) ) {
		Event_EnableShield();
	}
}

void hhKeeperSimple::HideNoDormant() {
	//overridden to hide shield fx
	if ( shieldFx.IsValid() ) {
		shieldFx->Hide();
	}
	idAI::Hide();
}

/*
=====================
hhKeeperSimple::Save
=====================
*/
void hhKeeperSimple::Save( idSaveGame *savefile ) const {
	beamTelepathic.Save( savefile );
	beamAttack.Save( savefile );
	shieldFx.Save( savefile );
	headFx.Save( savefile );
	triggerEntity.Save( savefile );
	throwEntity.Save( savefile );
	savefile->WriteInt( nextShieldImpact );
	savefile->WriteBool( bThrowing );
}

/*
=====================
hhKeeperSimple::Restore
=====================
*/
void hhKeeperSimple::Restore( idRestoreGame *savefile ) {
	beamTelepathic.Restore( savefile );
	beamAttack.Restore( savefile );
	shieldFx.Restore( savefile );
	headFx.Restore( savefile );
	triggerEntity.Restore( savefile );
	throwEntity.Restore( savefile );
	savefile->ReadInt( nextShieldImpact );
	savefile->ReadBool( bThrowing );
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build