
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//-----------------------------------------------------
// hhNoClipEnt
//-----------------------------------------------------
CLASS_DECLARATION(idEntity, hhNoClipEnt)
END_CLASS

void hhNoClipEnt::Spawn( void ) {
	if(GetPhysics())
		GetPhysics()->SetContents(0);	
}

//-----------------------------------------------------
// hhAINode
//-----------------------------------------------------
CLASS_DECLARATION( idEntity, hhAINode )
END_CLASS

hhAINode::hhAINode( void ) {
	user.Clear();
}

void hhAINode::Save( idSaveGame *savefile ) const {
	user.Save( savefile );
}

void hhAINode::Restore( idRestoreGame *savefile ) {
	user.Restore( savefile );
}

//-----------------------------------------------------
// hhMonsterAI
//-----------------------------------------------------

idList<idAI*> hhMonsterAI::allSimpleMonsters;

hhMonsterAI::hhMonsterAI() {
	lookOffset = ang_zero;
	shootTarget = NULL;
	bBossBar = false;
	spawnThinkFlags = 0;
	lastContactTime = gameLocal.time;
	nextSpeechTime = gameLocal.time;
	frozen = false;
	soundOnModel = false;
}

hhMonsterAI::~hhMonsterAI() {
	allSimpleMonsters.Remove(this);

	if ( bBossBar && spawnArgs.GetBool( "remove_bar_on_dissolve" ) ) {
		idPlayer *player = gameLocal.GetLocalPlayer();
		if ( player && player->hud ) {
			bBossBar = false;
			player->hud->HandleNamedEvent("HideProgressBar");
			player->hud->StateChanged(gameLocal.time);
		}
	}
}

void hhMonsterAI::Spawn() {
	AI_HAS_RANGE_ATTACK					= spawnArgs.GetBool("has_range_attack", "0");
	AI_HAS_MELEE_ATTACK					= spawnArgs.GetBool("has_melee_attack", "0");

	targetReaction.reactionIndex = -1;
	targetReaction.entity = NULL;

	allSimpleMonsters.AddUnique(this);
	bBindAxis = spawnArgs.GetBool( "bind_axis" );
	bCanFall = spawnArgs.GetBool( "can_fall", "0" );
	bSeeThroughPortals = spawnArgs.GetBool( "can_see_portals", "0" );
	bBindOrient = spawnArgs.GetBool( "bind_orient", "0" );
	hearingRange = spawnArgs.GetInt( "hearing_range", "1024" );
	bCanWallwalk = spawnArgs.GetBool( "can_wallwalk", "0" );
	fallDelay = spawnArgs.GetInt( "fall_delay" );
	bOverrideKilledByGravityZones = spawnArgs.GetBool( "overrideKilledByGravityZones" );
	bNeverTarget = spawnArgs.GetBool( "never_target", "0" );
	bNoCombat = spawnArgs.GetBool( "no_combat", "0" );
	const char *temp;
	if ( spawnArgs.GetString( "fx_custom_blood", "", &temp ) ) {
		bCustomBlood = true;
	} else {
		bCustomBlood = false;
	}

	nextSpiritProxyCheck = 0;
	nextTurnUpdate = 0;
	frozen = false;

	//handle initial rotation and sticking on wallwalk
	if ( !IsHidden() ) {	
		PostEventSec( &MA_InitialWallwalk, 0.1f );
		PostEventMS(&MA_EnemyOnSpawn, 10);	
	}
	PostEventMS(&EV_PostSpawn, 0);
	if ( spawnArgs.GetInt( "wander_radius" ) ) {
		spawnOrigin = GetOrigin();
	}

	// CJR:  Clear the DDA values used for tracking damage inflicted on the player
	totalDDADamage = 0;
}

void hhMonsterAI::Event_PostSpawn() {
	CreateHealthTriggers();

	//TODO: Move to character/girlfriend code when it exists
	// Spawn earrings for girlfriend
	idEntity *ent;
	const char *defName = spawnArgs.GetString("def_earring", NULL);
	if (defName && defName[0] && head.IsValid()) {
		const char *boneNameL = spawnArgs.GetString("earringboneL");
		const char *boneNameR = spawnArgs.GetString("earringboneR");
		idDict args;
		args.Clear();
		args.Set( "origin", GetPhysics()->GetOrigin().ToString() );

		ent = gameLocal.SpawnObject(defName, &args);
		if (ent) {
			ent->MoveToJoint(head.GetEntity(), boneNameL);
			ent->BindToJoint(head.GetEntity(), boneNameL, false);
		}

		ent = gameLocal.SpawnObject(defName, &args);
		if (ent) {
			ent->MoveToJoint(head.GetEntity(), boneNameR);
			ent->BindToJoint(head.GetEntity(), boneNameR, false);
		}
	}
}


bool hhMonsterAI::HasPathTo(const idVec3 &destPt) {
	if(!GetAAS())	// || !allowAAS)		//TODO: support the allowAAS functionality
		return FALSE;

	idVec3 fromPos = GetPhysics()->GetOrigin();	

	int currAreaNum = PointReachableAreaNum(fromPos);
	int toAreaNum   = PointReachableAreaNum(destPt);


	// Invalid #'s ? 
	if(toAreaNum == 0 || currAreaNum == 0)
		return FALSE;
	
	aasPath_t path;
	return PathToGoal(path, currAreaNum, fromPos, toAreaNum, destPt);
}

/*
==============================
hhMonsterAI::LinkScriptVariables(void)
==============================
*/
#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhMonsterAI::LinkScriptVariables(void) {

	idAI::LinkScriptVariables();

	LinkScriptVariable( AI_HAS_RANGE_ATTACK );		// required for reaction system
	LinkScriptVariable( AI_HAS_MELEE_ATTACK );		// required for reaction system
	LinkScriptVariable( AI_USING_REACTION );		// TRUE if monster is currently using reaction
	LinkScriptVariable( AI_REACTION_FAILED );		// TRUE if the last reaction attempt failed (path blocked, exclusive problem, etc)
	LinkScriptVariable( AI_REACTION_ANIM );
	LinkScriptVariable( AI_BACKWARD );
	LinkScriptVariable( AI_STRAFE_LEFT );
	LinkScriptVariable( AI_STRAFE_RIGHT );
	LinkScriptVariable( AI_UPWARD );
	LinkScriptVariable( AI_DOWNWARD );
	LinkScriptVariable( AI_SHUTTLE_DOCKED );
	LinkScriptVariable( AI_VEHICLE_ATTACK );
	LinkScriptVariable( AI_VEHICLE_ALT_ATTACK );
	LinkScriptVariable( AI_WALLWALK );
	LinkScriptVariable( AI_FALLING );
	LinkScriptVariable( AI_PATHING );
	LinkScriptVariable( AI_TURN_DIR );
	LinkScriptVariable( AI_FLY_NO_SEEK );
	LinkScriptVariable( AI_FOLLOWING_PATH );
}

void hhMonsterAI::Think( void ) {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) { //HUMANHEAD rww
		return;
	}

	idVec3 oldOrigin = physicsObj.GetOrigin();
	idVec3 oldVelocity = physicsObj.GetLinearVelocity();

	if ( thinkFlags & TH_THINK ) {

		// Update vehicle guns
		if(AI_VEHICLE && InVehicle()) {
			usercmd_t cmds;
			const signed char speed = 64;	// In range [0..127]
			memset( &cmds, 0, sizeof(usercmd_t) );
			cmds.buttons |= (AI_VEHICLE_ATTACK) ? BUTTON_ATTACK : 0;
			cmds.buttons |= (AI_VEHICLE_ALT_ATTACK) ? BUTTON_ATTACK_ALT : 0;

			if(AI_FORWARD) {
				cmds.forwardmove = speed;
			} else if(AI_BACKWARD) {
				cmds.forwardmove = -speed;
			}
			
			if(AI_STRAFE_RIGHT) {
				cmds.rightmove = speed;
			} else if(AI_STRAFE_LEFT) {
				cmds.rightmove = -speed;
			}

			if(AI_UPWARD) {
				cmds.upmove = speed;
			} else if(AI_DOWNWARD) {
				cmds.upmove = -speed;
			}

			GetVehicleInterfaceLocal()->BufferPilotCmds( &cmds, NULL );
			AI_SHUTTLE_DOCKED = GetVehicleInterfaceLocal()->IsVehicleDocked();
		}		

		// clear out the enemy when he dies or is hidden
		idActor *enemyEnt = enemy.GetEntity();
		if ( enemyEnt ) {
			if ( enemyEnt->IsType( hhSpiritProxy::Type ) ) {
				idPlayer *player = gameLocal.GetLocalPlayer();
				if ( player && player->AI_DEAD ) {
					EnemyDead();
				}
			} else if ( enemyEnt->health <= 0 ) {
				EnemyDead();
			}
		}

	    //HUMANHEAD: aob - vehicle updates our viewAxis
	    if( !InVehicle() ) {
		    current_yaw += deltaViewAngles.yaw;
		    ideal_yaw = idMath::AngleNormalize180( ideal_yaw + deltaViewAngles.yaw );
		    deltaViewAngles.Zero();
		    viewAxis = idAngles( 0, current_yaw, 0 ).ToMat3();

			// Determine turn dir
			if (gameLocal.time > nextTurnUpdate) {
				AI_TURN_DIR = GetTurnDir();
				nextTurnUpdate = gameLocal.time + 250;
			}
	    }
	    //HUMANHEAD END

		if ( num_cinematics ) {
			if ( !IsHidden() && torsoAnim.AnimDone( 0 ) ) {
				PlayCinematic();
			}
			RunPhysics();
		} else if ( !allowHiddenMovement && IsHidden() ) {
			// hidden monsters
			UpdateAIScript();
		// HUMANHEAD pdm: Vehicle support
		} else if ( InVehicle() ) {
			UpdateEnemyPosition();
			UpdateAIScript();
			FlyMove();
		// HUMANHEAD END

		} else {
			// clear the ik before we do anything else so the skeleton doesn't get updated twice
			walkIK.ClearJointMods();

		    // HUMANHEAD NLA
		    physicsObj.ResetNumTouchEnt(0);
		    // HUMANHEAD END
			switch( move.moveType ) {
			case MOVETYPE_DEAD :
				// dead monsters
				UpdateAIScript();
				DeadMove();
				break;

			case MOVETYPE_FLY :
				// flying monsters
				UpdateEnemyPosition();
				UpdateAIScript();
				FlyMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_STATIC :
				// static monsters
				UpdateEnemyPosition();
				UpdateAIScript();
				StaticMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_ANIM :
				// animation based movement
				UpdateEnemyPosition();
				UpdateAIScript();
				AnimMove();
				PlayChatter();
				CheckBlink();
				break;

			case MOVETYPE_SLIDE :
				// velocity based movement
				UpdateEnemyPosition();
				UpdateAIScript();
				SlideMove();
				PlayChatter();
				CheckBlink();
				break;
			}
		    // HUMANHEAD NLA
		    ClientImpacts();
		    // HUMANHEAD END
		}

		// clear pain flag so that we recieve any damage between now and the next time we run the script
		AI_PAIN = false;
		AI_SPECIAL_DAMAGE = 0;
		AI_PUSHED = false;
	} else if ( thinkFlags & TH_PHYSICS ) {
		RunPhysics();
	}

	// HUMANHEAD jrm - need to call ticker function per aaron
	if (thinkFlags & TH_TICKER) {
		Ticker();
	}

	if ( af_push_moveables ) {
		PushWithAF();
	}

	if ( fl.hidden && allowHiddenMovement ) {
		// UpdateAnimation won't call frame commands when hidden, so call them here when we allow hidden movement
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}

	UpdateMuzzleFlash();
	UpdateAnimation();
	UpdateParticles();
	UpdateWounds();
	Present();
	UpdateDamageEffects();
	LinkCombat();

	CrashLand( oldOrigin, oldVelocity );

	if(health > 0) {
		idStr tmp;	
		if(ai_showNoAAS.GetBool() && spawnArgs.GetString("use_aas", "", tmp) && spawnArgs.GetBool("noaas_warning","1")) {			
			if(!aas) {
				gameRenderWorld->DrawText("?", this->GetEyePosition() + idVec3(0.0f, 0.0f, 12.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());				
			}
		}
	}

	if ( !AI_DEAD && bCanFall && !IsHidden() && !fl.isTractored ) {
		if ( physicsObj.HasContacts() || physicsObj.GetLinearVelocity().LengthSqr() < 4 ) {
			lastContactTime = gameLocal.time;
			if ( AI_FALLING ) {
				AI_FALLING = false;
				bCanFall = false;
				SetState( GetScriptFunction( "state_Idle" ) );
				SetWaitState( "" );
			}
		} else  if ( !AI_FALLING && !af.IsActive() && physicsObj.GetLinearVelocity().LengthSqr() > 0 ) {
			if ( gameLocal.time - lastContactTime > fallDelay ) { 
				AI_FALLING = true;
				SetState( GetScriptFunction( "state_Nothing" ) );
				Event_AnimState(ANIMCHANNEL_TORSO, "Torso_Fall", 4);
				Event_AnimState(ANIMCHANNEL_LEGS, "Legs_Fall", 4);
			}
		}
	}

	//update wallwalk
	if ( bCanWallwalk && !fl.isTractored ) {
		trace_t TraceInfo;
		gameLocal.clip.TracePoint(TraceInfo, GetOrigin(), GetOrigin() + this->GetPhysics()->GetGravityNormal() * 50, GetPhysics()->GetClipMask(), this);
		if( TraceInfo.fraction < 1.0f && gameLocal.GetMatterType(TraceInfo, NULL) == SURFTYPE_WALLWALK ) {
			SetGravity( -TraceInfo.c.normal * DEFAULT_GRAVITY );
	 		AI_WALLWALK = true;
		} else if ( AI_WALLWALK ) {
			SetGravity( idVec3(0,0,-DEFAULT_GRAVITY) );
			AI_WALLWALK = false;
		}
	} else if ( AI_WALLWALK ) {
		SetGravity( idVec3(0,0,-DEFAULT_GRAVITY) );
		AI_WALLWALK = false;
	}

	if ( bBossBar ) {
		idPlayer *player = gameLocal.GetLocalPlayer();
		if ( player && player->hud ) {
			player->hud->SetStateFloat( "progress", idMath::ClampFloat( 0.0f, 1.0f, float(health) / (float)spawnHealth ) );
			player->hud->StateChanged(gameLocal.time);
			player->hud->Redraw( gameLocal.realClientTime );
		}
	}

	if( ai_debugBrain.GetInteger() > 0 && !IsHidden() ) {
		if ( enemy.IsValid() && enemy->GetHealth() > 0 ) {
			gameRenderWorld->DebugArrow( colorWhite, GetOrigin(), enemy->GetOrigin(), 10 );
			float dist = ( GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin() ).LengthFast();
			gameRenderWorld->DrawText( va("%i", int(dist)), this->GetEyePosition() + idVec3(0.0f, 0.0f, 60.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		}
		if ( state ) {
			if ( physicsObj.GetClipMask() == 0 ) {
				gameRenderWorld->DrawText(state->Name(), GetEyePosition() + idVec3(0.0f, 0.0f, 40.0f), 0.75f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
			} else {
				gameRenderWorld->DrawText(state->Name(), GetEyePosition() + idVec3(0.0f, 0.0f, 40.0f), 0.75f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
			}
		}
		gameRenderWorld->DrawText(torsoAnim.state, this->GetEyePosition() + idVec3(0.0f, 0.0f, 20.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		gameRenderWorld->DrawText(legsAnim.state, this->GetEyePosition() + idVec3(0.0f, 0.0f, 0.0f), 0.75f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		if ( !walkIK.IsActivated() ) {
			gameRenderWorld->DrawText("ik disabled", GetEyePosition() + idVec3(0.0f, 0.0f, -20.0f), 0.75f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
		}
	}
}

void hhMonsterAI::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	//we don't want monsters pushed around by the player, projectiles, or splashDamange, so early out if they are.
	if( !af.IsActive() ) {
		if ( ent && (ent->IsType( hhProjectile::Type ) || ent->IsType( idWorldspawn::Type ) || ent->IsType( idPlayer::Type )) ) {
			return;
		}
	}

	af.GetPhysics()->UpdateTime( gameLocal.GetTime() ); 
	BecomeActive(TH_THINK);

	idAI::ApplyImpulse( ent, id, point, force );
}

//
// hhMonsterAI::CheckValidReaction
//  Helper function for using reactions.
// 
bool hhMonsterAI::CheckValidReaction() {
	hhReaction *reaction = targetReaction.GetReaction();
	if( !reaction || !reaction->causeEntity.IsValid() ) {
		return false;
	}
	return true;
}

//
// hhMonsterAI::FinishReaction
//  Called once a reaction has been used/failed.  If it failed, set the failed flag and exit. 
//	Otherwise, set any 'finish_key' we have.
//
void hhMonsterAI::FinishReaction( bool bFailed ) {
	AI_USING_REACTION = false;
	if( bFailed ) {
		AI_REACTION_FAILED = true;
	}
	else {
		hhReaction *reaction = targetReaction.GetReaction();
		if( reaction && reaction->desc->finish_key.Length() ) {
			spawnArgs.Set( reaction->desc->finish_key.c_str(), reaction->desc->finish_val.c_str() );
		}
	}

	//monster is finished with this reaction. let others use it
	if ( targetReaction.entity.IsValid() ) {
		targetReaction.entity->spawnArgs.Set( "react_inuse", "0" );
	}
}

void hhMonsterAI::Killed(idEntity *inflictor, idEntity *attacker, 
					   int damage, const idVec3 &dir, int location ) 
{				
	if ( AI_DEAD ) {
		AI_DAMAGE = true;
		return;
	}

	if ( bBossBar && spawnArgs.GetBool( "remove_bar_on_death" ) ) {
		idPlayer *player = gameLocal.GetLocalPlayer();
		if ( player && player->hud ) {
			bBossBar = false;
			player->hud->HandleNamedEvent("HideProgressBar");
			player->hud->StateChanged(gameLocal.time);
		}
	}

	HandleNoGore();

	idAI::Killed(inflictor, attacker, damage, dir, location);		

	fl.noPortal = 0; // CJR:  Set so that killed monsters never collide with portals

	SendDamageToDDA(); // CJR DDA:  Send any accumulated damage to the dda system
			
	// General non-item dropping (for monsters, souls, etc.)
	const idKeyValue *kv = NULL;
	kv = spawnArgs.MatchPrefix( "def_drops", NULL );
	while ( kv ) {

		idStr drops = kv->GetValue();			
		idDict args;
						
		idStr last5 = kv->GetKey().Right(5);
		if ( drops.Length() && idStr::Icmp( last5, "Joint" ) != 0) {

			args.Set( "classname", drops );

			// HUMANHEAD pdm: specify monster so souls can call back to remove body when picked up
			args.Set("monsterSpawnedBy", name.c_str());

			idVec3 origin;
			idMat3 axis;			
			idStr jointKey = kv->GetKey() + idStr("Joint");
			idStr jointName = spawnArgs.GetString( jointKey );
			idStr joint2JointKey = kv->GetKey() + idStr("Joint2Joint");
			idStr j2jName = spawnArgs.GetString( joint2JointKey );			
			
			idEntity *newEnt = NULL;
			gameLocal.SpawnEntityDef( args, &newEnt );
			HH_ASSERT(newEnt != NULL);

			// Spin to correct heading
			if(newEnt->IsType(hhMonsterAI::Type)) {
				hhMonsterAI *newAI = static_cast<hhMonsterAI*>(newEnt);
				newAI->current_yaw	= current_yaw;
				newAI->ideal_yaw	= ideal_yaw;
			}

			if(jointName.Length()) {
				jointHandle_t joint = GetAnimator()->GetJointHandle( jointName );
				if (!GetAnimator()->GetJointTransform( joint, gameLocal.time, origin, axis ) ) {
					gameLocal.Printf( "%s refers to invalid joint '%s' on entity '%s'\n", (const char*)jointKey.c_str(), (const char*)jointName, (const char*)name );
					origin = renderEntity.origin;
					axis = renderEntity.axis;
				}
				axis *= renderEntity.axis;
				origin = renderEntity.origin + origin * renderEntity.axis;
				newEnt->SetAxis(axis);
				newEnt->SetOrigin(origin);
			}
			else {
				
				newEnt->SetAxis(viewAxis);
				newEnt->SetOrigin(GetOrigin());
			}

		}
		
		kv = spawnArgs.MatchPrefix( "def_drops", kv );
	}
}

void hhMonsterAI::Event_Remove( void ) {    
	if( InVehicle() ) {
		GetVehicleInterface()->GetVehicle()->EjectPilot();
	}

	idAI::Event_Remove();
}

void hhMonsterAI::EnterVehicle( hhVehicle* vehicle ) {
	if (!vehicle->WillAcceptPilot(this)) {
		return;
	}
	spawnArgs.Set( "use_aas", spawnArgs.GetString( "aas_shuttle", "aasDroid" ) );
	SetAAS();
	idAI::EnterVehicle( vehicle );
	PostEventSec( &MA_SetVehicleState, 0.1f );
}

bool hhMonsterAI::TestMelee( void ) const {
	trace_t trace;
	idActor *enemyEnt = enemy.GetEntity();

	if ( !enemyEnt || !melee_range ) {
		return false;
	}

	//FIXME: make work with gravity vector
	idVec3 org = physicsObj.GetOrigin();
	const idBounds &myBounds = physicsObj.GetBounds();
	idBounds bounds;

	idBounds meleeBounds( spawnArgs.GetVector( "melee_boundmin", "0 0 0" ), spawnArgs.GetVector( "melee_boundmax", "0 0 0" ) );
	if ( meleeBounds != bounds_zero ) {
		//check custom rotated meleebound
		idBox meleeBox( meleeBounds, org, renderEntity.axis );
		idBounds enemyBounds = enemyEnt->GetPhysics()->GetBounds();
		enemyBounds.TranslateSelf( enemyEnt->GetPhysics()->GetOrigin() );
		idBox enemyBox( enemyBounds );

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugBox( colorYellow, meleeBox );
		}

		if ( !enemyBox.IntersectsBox( meleeBox ) ) {
			return false;
		}
	} else {
		// expand the bounds out by our melee range
		bounds[0][0] = -melee_range;
		bounds[0][1] = -melee_range;
		bounds[0][2] = myBounds[0][2] - 4.0f;
		bounds[1][0] = melee_range;
		bounds[1][1] = melee_range;
		bounds[1][2] = myBounds[1][2] + 4.0f;
		bounds.TranslateSelf( org );

		idVec3 enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
		idBounds enemyBounds = enemyEnt->GetPhysics()->GetBounds();
		enemyBounds.TranslateSelf( enemyOrg );

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugBounds( colorYellow, bounds, vec3_zero, gameLocal.msec );
		}

		if ( !bounds.IntersectsBounds( enemyBounds ) ) {
			return false;
		}
	}

	idVec3 start = GetEyePosition();
	idVec3 end = enemyEnt->GetEyePosition();

	gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
	if ( ( trace.fraction == 1.0f ) || ( gameLocal.GetTraceEntity( trace ) == enemyEnt ) ) {
		return true;
	}

	return false;
}

bool hhMonsterAI::MoveToPosition( const idVec3 &pos, bool enemyBlocks ) {
	if(AI_VEHICLE && InVehicle()) {
		GetVehicleInterfaceLocal()->ThrustTowards( pos, 1.0f );
		return true;
	} else {
		return idAI::MoveToPosition( pos, enemyBlocks );
	}
}

bool hhMonsterAI::TurnToward( const idVec3 &pos ) {
	if (AI_VEHICLE && InVehicle()) {
		GetVehicleInterfaceLocal()->OrientTowards( pos, 0.5 );
		return true;
	} else {
		return idAI::TurnToward( pos );
	}
}

bool hhMonsterAI::CanSee( idEntity *ent, bool useFov ) {
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

idPlayer* hhMonsterAI::GetClosestPlayer(void) {
	idEntity *closestEnt = NULL;
	float closestDist = idMath::INFINITY;	

	for(int i=0;i<gameLocal.numClients;i++) {
		idEntity *ent = gameLocal.entities[i];		
		if(ent) {
			HH_ASSERT(ent->IsType(idPlayer::Type));

			float l = (ent->GetOrigin()-GetOrigin()).Length();
			if(l < closestDist || !closestEnt) {
				closestDist = l;
				closestEnt  = ent;
			}
		}
	}

	return static_cast<idPlayer*>(closestEnt);
}

void hhMonsterAI::Show() {
	if ( spawnThinkFlags != 0 ) {
		int temp = spawnThinkFlags;
		spawnThinkFlags = 0;
		BecomeActive( temp );
	}
	idAI::Show();
	PostEventMS(&MA_EnemyOnSpawn, 10);	
	Event_InitialWallwalk();
}

bool hhMonsterAI::GetFacePosAngle( const idVec3 &pos, float &delta ) {
	float	diff;
	float	angle1;
	float	angle2;
	idVec3  sourceOrigin;
	idVec3	targetOrigin;	

	sourceOrigin = GetPhysics()->GetOrigin();
	targetOrigin = pos;//target->GetPhysics()->GetOrigin();

	angle1 = DEG2RAD( GetGravViewAxis()[0].ToYaw() ); // VIEWAXIS_TO_GETGRAVVIEWAXIS
	angle2 = hhUtils::PointToAngle( targetOrigin.x - sourceOrigin.x, targetOrigin.y - sourceOrigin.y );
	if(angle2 > angle1) {
		diff = angle2 - angle1;

		if( diff > DEG2RAD(180.0f) ) {
			delta = DEG2RAD(359.9f) - diff;
			return false;
		}
		else {
			delta = diff;
			return true;
		}
	}
	else {
		diff = angle1 - angle2;
		if( diff > DEG2RAD(180.0f) ) {
			delta = DEG2RAD(359.9f) - diff;
			return true;
		}
		else {
			delta = diff;
			return false;
		}
	}
}

void hhMonsterAI::Distracted( idActor *newEnemy ) {
	SetEnemy( newEnemy );
}

void hhMonsterAI::SetEnemy( idActor *newEnemy ) {
	idAI::SetEnemy( newEnemy );
}

idVec3 hhMonsterAI::GetTouchPos(idEntity *ent, const hhReactionDesc *desc ) {
	assert(ent != NULL);
	assert(desc != NULL);
	
	idVec3 pos = ent->GetOrigin();	

	idVec3 offset = desc->touchOffsets.GetVector("all", "0 0 0");
	if ( offset == vec3_zero ) {
		// Each monster def type has its own offset etc. 
		// touchoffset_monster_hunter	
		offset = desc->touchOffsets.GetVector(GetEntityDefName(), "0 0 0");
	}
	idStr touchDirType = desc->touchDir;

	// When touching this ent, move to it as if it were 'cover'
	if( touchDirType == idStr("cover") && enemy.IsValid() ) {
		idVec3 goalPos = ent->GetOrigin() + offset.x * (ent->GetOrigin() - GetEnemy()->GetOrigin()).ToNormal();
		goalPos.z += offset.z;
		return goalPos;
	}
	// Move directly toward this ent from our pos
	else if(touchDirType == idStr("direct")) {
		idVec3 dir = GetOrigin() - pos;
		dir.Normalize();
		offset *= dir.ToMat3();
		return pos + offset;
	}
	else if(touchDirType == idStr("object")) {
		ent->GetFloorPos( 64.f, pos );
		idVec3 dir = GetOrigin() - pos;
		dir.Normalize();
		return pos + (dir * offset.x);
	}
	else {
		offset *= ent->GetAxis();		
		return pos + offset;
	}
	

}

bool hhMonsterAI::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	origin = modelOffset;
	if ( GetBindMaster() && bindJoint != INVALID_JOINT ) {
		idMat3 masterAxis;
		idVec3 masterOrigin;
		GetMasterPosition( masterOrigin, masterAxis );
		axis = physicsObj.localAxis * masterAxis;
		origin = masterOrigin + physicsObj.GetLocalOrigin() * masterAxis;
	} else if ( ( InVehicle() || bBindOrient ) && GetBindMaster() ) {
		axis = GetBindMaster()->GetAxis();
	} else {
		axis = viewAxis;
	}
	return true;
}

void hhMonsterAI::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		if ( bBindAxis && GetBindMaster() ) {
			renderEntity.axis = GetBindMaster() ->GetAxis();
		} else {
			renderEntity.axis = axis * GetPhysics()->GetAxis();
		}
		if ( GetBindMaster() && bindJoint != INVALID_JOINT ) { 
			if ( head.IsValid() && head->GetPhysics() ) {
				head->GetPhysics()->Evaluate(gameLocal.time-gameLocal.previousTime, gameLocal.time);
			}
			renderEntity.origin = origin;
		} else {
			if ( GetBindMaster() && head.IsValid() && head->GetPhysics() ) {
				head->GetPhysics()->Evaluate(gameLocal.time-gameLocal.previousTime, gameLocal.time);
			}
			renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
		}
	} else {
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin();
	}
}

void hhMonsterAI::UpdateFromPhysics( bool moveBack ) {
	// set master delta angles for actors
	if ( GetBindMaster() ) {
		if( !InVehicle() ) {
			idAngles delta = GetDeltaViewAngles();
			if ( moveBack ) {
				delta.yaw -= physicsObj.GetMasterDeltaYaw();
			} else {
				delta.yaw += physicsObj.GetMasterDeltaYaw();
			}
		
			SetDeltaViewAngles( delta );
		} else {
			SetAxis( GetBindMaster()->GetAxis() );
		}
	}

	if ( UpdateAnimationControllers() ) {
		BecomeActive( TH_ANIMATE );
	}

	UpdateVisuals();
}

void hhMonsterAI::CreateHealthTriggers() {
	const char keyPrefix[] = "health_percent_trigger_";
	int keyPrefixLen = strlen(keyPrefix);

	const idKeyValue *kv = spawnArgs.MatchPrefix(keyPrefix, NULL);
	healthTriggers.Clear();
	while(kv) {
		hhMonsterHealthTrigger t;
		idStr k = kv->GetKey();
		idStr perct = k.Right(k.Length() - strlen(keyPrefix));
		float p = float(atoi(perct.c_str())) * 0.01f;
		t.healthThresh = int(float(health) * p);
		t.triggerEnt = gameLocal.FindEntity(kv->GetValue().c_str());
		if(!t.triggerEnt.GetEntity()) {
			gameLocal.Warning("%s specified %s key with entity \"%s\", but that entity does not exist!", name.c_str(), kv->GetKey().c_str(), kv->GetValue().c_str());
		}
		healthTriggers.Append(t);		
		kv = spawnArgs.MatchPrefix(keyPrefix, kv);
	}
}

void hhMonsterAI::UpdateHealthTriggers(int oldHealth, int currHealth) {

	// Trigger them
	for(int i=0;i<healthTriggers.Num();i++) {
		// For now, only trigger them ONCE, if scripters want something else, we can change later
		if(healthTriggers[i].triggerCount <= 0 && currHealth < healthTriggers[i].healthThresh) {
			if(healthTriggers[i].triggerEnt.GetEntity()) {
				healthTriggers[i].triggerEnt.GetEntity()->ProcessEvent(&EV_Activate, this);
				healthTriggers[i].triggerCount++;
			}
		}
	}
}

//
// Damage()
//
void hhMonsterAI::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	// Return if we don't take damage
	if ( !fl.takedamage ) {
		return;
	}

	if ( spawnArgs.GetBool( "noPlayerDamage", "0" ) ) {
		if ( attacker && attacker->IsType( idPlayer::Type ) ) {
			return;
		}
	}

	int oldHealth = health;
	idAI::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
	UpdateHealthTriggers(oldHealth, health);

	if ( bCustomBlood && inflictor ) {
		hhFxInfo fxInfo;
		fxInfo.RemoveWhenDone( true );
		BroadcastFxInfoPrefixed( "fx_custom_blood", inflictor->GetOrigin(), mat3_identity, &fxInfo );
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( AI_DEAD && damageDef ) {
		if ( damageDef->GetBool( "ice" ) && spawnArgs.GetBool( "can_freeze", "0" )  ) {
			SetSkinByName( spawnArgs.GetString( "skin_freeze" ) );

			if( af.IsLoaded() ) {
				af.GetPhysics()->SetSelfCollision(false);
				af.GetPhysics()->SetContactFrictionScale(0);
				af.GetPhysics()->SetTimeScaleRamp(0,2);
				
				for(int i=0; i < af.GetPhysics()->GetNumConstraints(); i++) {
					idAFConstraint* constraint = af.GetPhysics()->GetConstraint(i);
					switch( constraint->GetType() ) {
						case CONSTRAINT_BALLANDSOCKETJOINT:
							static_cast<idAFConstraint_BallAndSocketJoint*>(constraint)->SetFriction(200.0f);
							static_cast<idAFConstraint_BallAndSocketJoint*>(constraint)->SetNoLimit();
							break;
						case CONSTRAINT_UNIVERSALJOINT:
							static_cast<idAFConstraint_UniversalJoint*>(constraint)->SetFriction(200.0f);
							static_cast<idAFConstraint_UniversalJoint*>(constraint)->SetNoLimit();
							break;
						case CONSTRAINT_HINGE:
							static_cast<idAFConstraint_Hinge*>(constraint)->SetFriction(200.0f);
							static_cast<idAFConstraint_Hinge*>(constraint)->SetNoLimit();
							break;
					}
				}
			}

			spawnArgs.Set("fx_deatheffect", spawnArgs.GetString( "fx_ice" ));
			spawnArgs.Set("produces_splats", "0");
			if( modelDefHandle > 0 )
				gameRenderWorld->RemoveDecals( modelDefHandle );
			//SetDeformation(DEFORMTYPE_DEATHEFFECT, gameLocal.time + 5000, 12000);
			SetShaderParm(SHADERPARM_TIME_OF_DEATH, MS2SEC(gameLocal.time+100000));
			CancelEvents( &EV_Dispose );
			PostEventSec( &EV_Dispose, 5 );
		}
		if ( damageDef->GetBool( "burn" ) && !spawnArgs.GetBool( "no_burn" ) ) {
			SetSkinByName( spawnArgs.GetString( "skin_burn" ) );
			spawnArgs.Set("fx_deatheffect", spawnArgs.GetString( "fx_burn" ));
			if( modelDefHandle > 0 )
				gameRenderWorld->RemoveDecals( modelDefHandle );
			CancelEvents( &EV_Dispose );
			PostEventSec( &EV_Dispose, 0 );
		}
		if ( damageDef->GetBool( "acid" ) && spawnArgs.GetBool( "acidburn" ) ) {
			SetSkinByName( spawnArgs.GetString( "skin_acidburn" ) );
			SetDeformation(DEFORMTYPE_DEATHEFFECT, gameLocal.time + 2500, 8000);	// starttime, duration
			PostEventSec( &EV_StartSound, 1.5f, "snd_acid", SND_CHANNEL_ANY, 1 );
			spawnArgs.Set("fx_deatheffect", spawnArgs.GetString( "fx_acid" ));
			spawnArgs.Set("mtr_splat1", spawnArgs.GetString( "mtr_acidsplat" ));
			spawnArgs.Delete("mtr_splat2");
			spawnArgs.Delete("mtr_splat3");
			spawnArgs.Delete("mtr_splat4");
			spawnArgs.Set("keepDecals", "1");

			CancelEvents( &EV_Dispose );
			PostEventSec( &EV_Dispose, 1.5 );
		}
	}
}

bool hhMonsterAI::NearEnoughTouchPos( idEntity* ent, const idVec3& targetPos, idBounds& bounds ) {
	bounds.TranslateSelf( targetPos );

	idBounds bnds( idVec3(-16.f, -16.f, -8.f), idVec3(16.f, 16.f, 64.f) );
	bnds.TranslateSelf( physicsObj.GetOrigin() );
//uncomment these to show debug lines
//	gameRenderWorld->DebugBounds( colorOrange, bounds );
//	gameRenderWorld->DebugArrow( colorRed, targetPos, targetPos + idVec3(0, 0, 10), 5 );
//	gameRenderWorld->DebugBounds( colorRed, bnds );
	if( bnds.IntersectsBounds( bounds ) ) {
		return true;
	}
	return false;
}

bool hhMonsterAI::GetTouchPosBound( const hhReactionDesc *desc, idBounds & bnds ) {
	idStr minName, maxName;
	idVec3 min, max;

	minName = va("min_%s", GetEntityDefName());
	maxName = va("max_%s", GetEntityDefName());

	if( !desc->touchOffsets.GetVector(minName.c_str(), "0 0 0", min) || !desc->touchOffsets.GetVector(maxName.c_str(), "0 0 0", max) ) {
		return false;
	}
	min -= idVec3( 16.f, 16.f, 8.f );
	max += idVec3( 16.f, 16.f, 64.f );

	bnds.Clear();
	bnds[ 0 ] = min;
	bnds[ 1 ] = max;
	return true;
}

int hhMonsterAI::EvaluateReaction( const hhReaction *react ) {
// Volume/Distance
	// If a volume in specified, then use that as our spatial check
	float distSq = ( react->causeEntity->GetOrigin() - GetOrigin() ).LengthSqr();

	if( react->desc->listenerVolumes.Num() > 0 ) {
		int count = 0;
		int i;
		for( i = 0; i < react->desc->listenerVolumes.Num(); i++ ) {
			assert( react->desc->listenerVolumes[ i ]->GetPhysics() != NULL );
			assert( GetPhysics() != NULL );
			if( react->desc->listenerVolumes[ i ]->GetPhysics()->GetAbsBounds().IntersectsBounds( GetPhysics()->GetAbsBounds() ) ) {
				//MDC-TODO: Draw bounds debugging info...
				count++;	//one is good enough to continue - no need to check the rest
				break;
			}
		}
		// If we are in ZERO of the listener volumes, then we can bail because this reaction doesn't apply to us
		if( count == 0 ) {
			return 0;
		}
	}
	if ( react->desc->effectVolumes.Num() > 0 && GetEnemy() ) {
		int count = 0;
		for( int i = 0; i < react->desc->effectVolumes.Num(); i++ ) {
			assert( react->desc->effectVolumes[ i ]->GetPhysics() != NULL );
			assert( GetPhysics() != NULL );
			if( react->desc->effectVolumes[ i ]->GetPhysics()->GetAbsBounds().IntersectsBounds( GetEnemy()->GetPhysics()->GetAbsBounds() ) ) {
				count++;	//one is good enough to return false - no need to check the rest
				break;
			}
		}
		// If enemy is in ZERO of the effect volumes, then we can bail because this reaction doesn't apply to us
		if( count == 0 ) {
			return 0;
		}
	}
	if( react->desc->listenerRadius > 0.f || react->desc->listenerMinRadius > 0.f ) {
		// TOO FAR
		if( react->desc->listenerRadius > 0.f && distSq > react->desc->listenerRadius * react->desc->listenerRadius ) {
			return 0;
		}
		// TOO CLOSE
		if( react->desc->listenerMinRadius > 0.f && distSq < react->desc->listenerMinRadius * react->desc->listenerMinRadius ) {
			return 0;
		}
	}
// Path-finding
	if( react->desc->CauseRequiresPathfinding() ) {
		if( !HasPathTo( GetTouchPos(react->causeEntity.GetEntity(), react->desc ) ) ) {
			//MDC-TODO: draw path debugging
			return 0;
		}
		else {
			//MDC-TODO: draw path debugging
		}
	}
// Can-See
	if( react->desc->flags & hhReactionDesc::flagReq_CanSee ) {
		if( react->causeEntity.IsValid() ) {
			if( !CanSee(react->causeEntity.GetEntity(), TRUE) ) {
				return 0;
			}
		}
	}

	return 100;
}

int hhMonsterAI::ReactionTo( const idEntity *ent ) {
	if ( bNoCombat ) {
		return ATTACK_IGNORE;
	}
	const idActor *actor = static_cast<const idActor *>( ent );
	if( actor && actor->IsType(hhDeathProxy::Type) ) {
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
				} else {
					//HUMANHEAD jsh PCF 4/29/06 fixed logic error with spiritproxy checking
					return ATTACK_IGNORE;
				}
			} else {
				return ATTACK_ON_SIGHT;				
			}
		} else {
			return ATTACK_IGNORE;
		}
	}

	if ( ent->IsType( hhMonsterAI::Type ) ) {
		const hhMonsterAI *entAI = static_cast<const hhMonsterAI *>( ent );
		if ( entAI && entAI->bNeverTarget ) {
			return ATTACK_IGNORE;
		}
	}

	return idAI::ReactionTo( ent );
}

bool hhMonsterAI::UpdateAnimationControllers( void ) {
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
	if ( ( !allowJointMod && !allowEyeFocus ) || ( gameLocal.time >= focusTime && focusTime != -1 ) || GetPhysics()->GetGravityNormal() != idVec3( 0,0,-1) ) {	
	    focusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 512.0f;
	} else if ( focusEnt == NULL ) {
		// keep looking at last position until focusTime is up
		focusPos = currentFocusPos;
	} else if ( focusEnt == enemy.GetEntity() ) {
		focusPos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset - eyeVerticalOffset * enemy.GetEntity()->GetPhysics()->GetGravityNormal();
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

#if 0
	gameRenderWorld->DebugLine( colorRed, orientationJointPos, focusPos, gameLocal.msec );
	gameRenderWorld->DebugLine( colorYellow, orientationJointPos, orientationJointPos + orientationJointAxis[ 0 ] * 32.0f, gameLocal.msec );
	gameRenderWorld->DebugLine( colorGreen, orientationJointPos, orientationJointPos + newLookAng.ToForward() * 48.0f, gameLocal.msec );
#endif

//JRMMERGE_GRAVAXIS: This changed to much to merge, see if you can get your monsters on planets changes back in here.  I'll leave both versions
#if OLD_CODE
	GetGravViewAxis().ProjectVector( dir, localDir ); // HUMANHEAD JRM: VIEWAXIS_TO_GETGRAVVIEWAXIS
	lookAng.yaw		= idMath::AngleNormalize180( localDir.ToYaw() );
	lookAng.pitch	= -idMath::AngleNormalize180( localDir.ToPitch() );
	lookAng.roll	= 0.0f;
#else
	// determine pitch from joint position
	dir = focusPos - eyepos;
	if ( ai_debugBrain.GetBool() ) {
		gameRenderWorld->DebugArrow( colorYellow, eyepos, eyepos + dir, 10, 1 );
	}
	dir.NormalizeFast();
	orientationJointAxis.ProjectVector( dir, localDir );
	newLookAng.pitch = -idMath::AngleNormalize180( localDir.ToPitch() ) + lookOffset.pitch;
	newLookAng.roll	= 0.0f;
#endif

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
		for( i = 0; i < lookJoints.Num(); i++ ) {
			jointAng.pitch	= 0;
			jointAng.yaw	= lookAng.yaw * lookJointAngles[ i ].yaw;
			animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );

			idMat3 pitchRot;
			hhMath::BuildRotationMatrix( DEG2RAD(-lookAng.pitch * lookJointAngles[ i ].pitch), 0, pitchRot );
			animator.GetJointLocalTransform( lookJoints[ i ], gameLocal.time, pos, mat );
			animator.SetJointAxis( lookJoints[ i ], JOINTMOD_LOCAL_OVERRIDE, pitchRot * mat );
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
			headEnt->BecomeActive( TH_ANIMATE );
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

//overridden to allow attack nodes farther away than our current enemydistance
void hhMonsterAI::Event_GetCombatNode( void ) {
	int				i;
	float			dist;
	idEntity		*targetEnt;
	idCombatNode	*node;
	float			bestDist;
	idCombatNode	*bestNode;
	idActor			*enemyEnt = enemy.GetEntity();

	if ( !targets.Num() ) {
		// no combat nodes
		idThread::ReturnEntity( NULL );
		return;
	}

	if ( !enemyEnt || !EnemyPositionValid() ) {
		// don't return a combat node if we don't have an enemy or
		// if we can see he's not in the last place we saw him
		idThread::ReturnEntity( NULL );
		return;
	}

	// find the closest attack node that can see our enemy and is closer than our enemy
	bestNode = NULL;
	const idVec3 &myPos = physicsObj.GetOrigin();
	bestDist = 9999999.0f ;
	for( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( !targetEnt || !targetEnt->IsType( idCombatNode::Type ) ) {
			continue;
		}

		node = static_cast<idCombatNode *>( targetEnt );
		if ( !node->IsDisabled() && node->EntityInView( enemyEnt, lastVisibleEnemyPos ) ) {
			idVec3 org = node->GetPhysics()->GetOrigin();
			dist = ( myPos - org ).LengthSqr();
			if ( dist < bestDist ) {
				bestNode = node;
				bestDist = dist;
			}
		}
	}

	idThread::ReturnEntity( bestNode );
}

void hhMonsterAI::FlyTurn( void ) {	//overridden for vehicle code changes
	if ( move.moveCommand == MOVE_FACE_ENEMY ) {
		TurnToward( lastVisibleEnemyPos );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else if ( move.speed > 0.0f ) {
		const idVec3 &vel = physicsObj.GetLinearVelocity();
		if ( vel.ToVec2().LengthSqr() > 0.1f ) {
			if ( InVehicle() ) {
				if ( move.goalEntity.IsValid() ) {
					TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
				} else {
					TurnToward( GetVehicleInterface()->GetVehicle()->GetPhysics()->GetLinearVelocity().ToYaw() );
				}
			} else {
				if ( vel.ToVec2().LengthSqr() > 0.1f ) {
					TurnToward( vel.ToYaw() );
				}
			}
		}
	}
	Turn();
}

//overridden to allow custom hearingRange
void hhMonsterAI::UpdateEnemyPosition( void ) {
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
	} else {
		// check if we heard any sounds in the last frame
		if ( enemyEnt == gameLocal.GetAlertEntity() ) {
			float dist = ( enemyEnt->GetPhysics()->GetOrigin() - org ).LengthSqr();
			//allow the sound's own radius to override hearingRange, if it is set
			if ( gameLocal.lastAIAlertRadius ) {
				if ( dist < Square( gameLocal.lastAIAlertRadius ) ) {
					SetEnemyPosition();
				}
			} else if ( dist < Square( hearingRange ) ) {
				SetEnemyPosition();
			}
		}
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugBounds( colorLtGrey, enemyEnt->GetPhysics()->GetBounds(), lastReachableEnemyPos, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorWhite, enemyEnt->GetPhysics()->GetBounds(), lastVisibleReachableEnemyPos, gameLocal.msec );
	}
}

void hhMonsterAI::BecameBound(hhBindController *b) {
	SetState( GetScriptFunction( "state_Nothing" ) );	
}

void hhMonsterAI::BecameUnbound(hhBindController *b) {
	if ( health > 0 ) {
		SetState( GetScriptFunction( "state_Idle" ) );
	}
}

void hhMonsterAI::CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity ) {
	const trace_t& trace = physicsObj.GetGroundTrace();
	if ( af.IsActive() || (!physicsObj.HasGroundContacts() || trace.fraction == 1.0f) && !IsBound() ) {
		return;
	}

	//aob - only check when we land on the ground
	//If we get here we can assume we currently have ground contacts
	if( physicsObj.HadGroundContacts() ) {
		return;
	}

	// if the monster wasn't going down
	if ( ( oldVelocity * -physicsObj.GetGravityNormal() ) >= 0.0f ) {
		return;
	}

	idVec3 deltaVelocity = DetermineDeltaCollisionVelocity( oldVelocity, trace );
	float delta = (IsBound()) ? deltaVelocity.Length() : deltaVelocity * physicsObj.GetGravityNormal();

	if ( delta < spawnArgs.GetFloat( "fatal_fall_velocity", "900" ) ) {
		return;	// Early out
	}
	if( trace.fraction == 1.0f ) {
		return;
	}
	Damage( NULL, NULL, oldVelocity.ToNormal(), "damage_monsterfall", 1, INVALID_JOINT );
}

hhEntityFx* hhMonsterAI::SpawnFxLocal( const char *fxName, const idVec3 &origin, const idMat3& axis, const hhFxInfo* const fxInfo, bool forceClient ) {
	//overridden to use GetJointWorldTransform to set location if binding to a bone
	idDict				fxArgs;
	hhEntityFx *		fx = NULL;

	if ( g_skipFX.GetBool() ) {
		return NULL;
	}

	if( !fxName || !fxName[0] ) {
		return NULL;
	}

	// Spawn an fx 
	fxArgs.Set( "fx", fxName );
	fxArgs.SetBool( "start", fxInfo ? fxInfo->StartIsSet() : true );
	fxArgs.SetVector( "origin", origin );
	fxArgs.SetMatrix( "rotation", axis );
	//HUMANHEAD: aob
	if( fxInfo ) {
		fxArgs.SetBool( "removeWhenDone", fxInfo->RemoveWhenDone() );
		fxArgs.SetBool( "onlyVisibleInSpirit", fxInfo->OnlyVisibleInSpirit() ); // CJR
		fxArgs.SetBool( "onlyInvisibleInSpirit", fxInfo->OnlyInvisibleInSpirit() ); // tmj
		fxArgs.SetBool( "toggle", fxInfo->Toggle() );
	}		
	//HUMANHEAD END

	//HUMANHEAD rww - use forceClient
	if (forceClient) {
		//this can happen on the "server" in the case of listen servers as well
		fx = (hhEntityFx *)gameLocal.SpawnClientObject( "func_fx", &fxArgs );
	}
	else {
		assert(!gameLocal.isClient);
		fx = (hhEntityFx *)gameLocal.SpawnObject( "func_fx", &fxArgs );
	}
	if( fxInfo ) {
		fx->SetFxInfo( *fxInfo );
	}

	if( fxInfo && fxInfo->EntityIsSet() ) {
		fx->fl.noRemoveWhenUnbound = fxInfo->NoRemoveWhenUnbound();
		if( fxInfo->BindBoneIsSet() ) {
			idVec3 bonePos;
			idMat3 boneAxis;
			GetJointWorldTransform( fxInfo->GetBindBone(), bonePos, boneAxis );
			fx->SetOrigin( bonePos );
			fx->BindToJoint( fxInfo->GetEntity(), fxInfo->GetBindBone(), true );
		} else if( fx && fx->Joint() && *fx->Joint() ) {
			fx->MoveToJoint( fxInfo->GetEntity(), fx->Joint() );
			fx->BindToJoint( fxInfo->GetEntity(), fx->Joint(), true );
		} else {
			fx->Bind( fxInfo->GetEntity(), true );
		}
	}

	fx->Show();
	
	return fx;
}

bool hhMonsterAI::AttackMelee( const char *meleeDefName ) {
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
	if ( !TestMeleeDef(meleeDefName) ) {
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

	if ( spawnArgs.GetBool( "smart_knockback", "0" ) ) {
		//do some traces to determine which dir to kick enemy
		idAngles ang = viewAxis.ToAngles();
		idVec3 forward, right, up;
		ang.ToVectors( &forward, &right, &up );
		trace_t trace;
		gameLocal.clip.TraceBounds( trace, enemyEnt->GetOrigin(), enemyEnt->GetOrigin() + -forward * 400, enemyEnt->GetPhysics()->GetBounds(), MASK_SOLID, this );
		if ( trace.fraction != 1.0f ) {
			//there's room to kick the player back
			globalKickDir = -forward;
			if ( ai_debugMove.GetBool() ) {
				gameRenderWorld->DebugArrow( colorRed, enemyEnt->GetOrigin(), enemyEnt->GetOrigin() + globalKickDir * 500, 10, 10000 );
			}
		} else {
			gameLocal.clip.TraceBounds( trace, enemyEnt->GetOrigin(), enemyEnt->GetOrigin() + right * 400, enemyEnt->GetPhysics()->GetBounds(), MASK_SOLID, this );
			if ( trace.fraction != 1.0f ) {
				globalKickDir = right;
				if ( ai_debugMove.GetBool() ) {
					gameRenderWorld->DebugArrow( colorRed, enemyEnt->GetOrigin(), enemyEnt->GetOrigin() + globalKickDir * 500, 10, 10000 );
				}
			} else {
				gameLocal.clip.TraceBounds( trace, enemyEnt->GetOrigin(), enemyEnt->GetOrigin() + right * 400, enemyEnt->GetPhysics()->GetBounds(), MASK_SOLID, this );
				if ( trace.fraction != 1.0f ) {
					globalKickDir = -right;
					if ( ai_debugMove.GetBool() ) {
						gameRenderWorld->DebugArrow( colorRed, enemyEnt->GetOrigin(), enemyEnt->GetOrigin() + globalKickDir * 500, 10, 10000 );
					}
				}
			}
		}
	}

	enemyEnt->Damage( this, this, globalKickDir, meleeDefName, 1.0f, INVALID_JOINT );

	//swap skin if we have successfully hit
	idStr hitSkin = spawnArgs.GetString( "skin_melee_hit", "" );
	if ( hitSkin.Length() ) {
		SetSkinByName( hitSkin.c_str() );
	}

	lastAttackTime = gameLocal.time;

	return true;
}

bool hhMonsterAI::TestMeleeDef( const char *meleeDefName ) const {
	//tests using "melee_boundmin" and "melee_boundmax" in the damage def
	trace_t trace;
	idActor *enemyEnt = enemy.GetEntity();
	const idDict *meleeDef;

	meleeDef = gameLocal.FindEntityDefDict( meleeDefName, false );
	if ( !meleeDef ) {
		gameLocal.Error( "Unknown melee '%s'", meleeDefName );
	}

	if ( !enemyEnt || !melee_range ) {
		return false;
	}

	//FIXME: make work with gravity vector
	idVec3 org = physicsObj.GetOrigin();
	const idBounds &myBounds = physicsObj.GetBounds();
	idBounds bounds;

	idBounds meleeBounds( meleeDef->GetVector( "melee_boundmin" ), meleeDef->GetVector( "melee_boundmax" ) );

	if ( meleeBounds != bounds_zero ) {
		//check custom rotated meleebound
		idBox meleeBox( meleeBounds, org, renderEntity.axis );
		idBounds enemyBounds = enemyEnt->GetPhysics()->GetBounds();
		enemyBounds.TranslateSelf( enemyEnt->GetPhysics()->GetOrigin() );
		idBox enemyBox( enemyBounds );

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugBox( colorYellow, meleeBox, 5000 );
		}

		if ( !enemyBox.IntersectsBox( meleeBox ) ) {
			return false;
		}
	} else {
		// expand the bounds out by our melee range
		bounds[0][0] = -melee_range;
		bounds[0][1] = -melee_range;
		bounds[0][2] = myBounds[0][2] - 4.0f;
		bounds[1][0] = melee_range;
		bounds[1][1] = melee_range;
		bounds[1][2] = myBounds[1][2] + 4.0f;
		bounds.TranslateSelf( org );

		idVec3 enemyOrg = enemyEnt->GetPhysics()->GetOrigin();
		idBounds enemyBounds = enemyEnt->GetPhysics()->GetBounds();
		enemyBounds.TranslateSelf( enemyOrg );

		if ( ai_debugMove.GetBool() ) {
			gameRenderWorld->DebugBounds( colorYellow, bounds, vec3_zero, gameLocal.msec );
		}

		if ( !bounds.IntersectsBounds( enemyBounds ) ) {
			return false;
		}
	}

	idVec3 start = GetEyePosition();
	idVec3 end = enemyEnt->GetEyePosition();

	gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
	if ( ( trace.fraction == 1.0f ) || ( gameLocal.GetTraceEntity( trace ) == enemyEnt ) ) {
		return true;
	}

	return false;
}

void hhMonsterAI::PrintDebug() {
	if ( enemy.IsValid()  ) {
		int dist = int(( GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin() ).LengthFast());
		gameLocal.Printf( "  Enemy: %s\n", enemy->GetName() );
		gameLocal.Printf( "  Distance to Enemy: %d\n", dist );
		gameLocal.Printf( "  Enemy Area: %i\n", PointReachableAreaNum(enemy->GetOrigin()) );
		gameLocal.Printf( "  Enemy Reachable: %s\n", AI_ENEMY_REACHABLE ? "yes" : "no" );
		gameLocal.Printf( "  Enemy Visible: %s\n", AI_ENEMY_VISIBLE ? "yes" : "no" );
	} else {
		gameLocal.Printf( "  Enemy: None\n" );
	}
	gameLocal.Printf( "  Current Area: %i\n", PointReachableAreaNum(GetOrigin()) );
	if ( state ) {
		gameLocal.Printf( "  State: %s\n", state->Name() );
	}
	gameLocal.Printf( "  Health: %i/%i\n", GetHealth(), spawnArgs.GetInt( "health" ) );
	gameLocal.Printf( "  Torso State: %s\n", torsoAnim.state.c_str() );
	gameLocal.Printf( "  Legs State: %s\n", legsAnim.state.c_str() );
	gameLocal.Printf( "  IK: %s\n", walkIK.IsActivated() ? "enable" : "disabled" );
	gameLocal.Printf( "  Turnrate: %f\n", turnRate );
}

bool hhMonsterAI::NewWanderDir( const idVec3 &dest ) {
	if ( spawnArgs.GetInt( "wander_radius" ) ) {
		//pick a new dest based on radius from starting origin
		idVec3 offset = hhUtils::RandomVector() * float(spawnArgs.GetInt( "wander_radius" ));
		offset.z = 0.0f;
		const idVec3 newDest = spawnOrigin + offset;
		return idAI::NewWanderDir( newDest );
	} else {
		return idAI::NewWanderDir( dest );
	}
}

/*
=====================
hhMonsterAI::Save
=====================
*/
void hhMonsterAI::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( targetReaction.reactionIndex );
	targetReaction.entity.Save( savefile);
	shootTarget.Save( savefile );
	currPassageway.Save( savefile );

	savefile->WriteBool( bCanFall );

	int i, num = healthTriggers.Num();
	savefile->WriteInt( num );
	for (i = 0; i < num; i++) {
		savefile->WriteInt( healthTriggers[i].healthThresh );
		healthTriggers[i].triggerEnt.Save( savefile );
		savefile->WriteInt( healthTriggers[i].triggerCount );
	}
	savefile->WriteInt( hearingRange );
	savefile->WriteInt( lastContactTime );
	savefile->WriteBool( bSeeThroughPortals );
	savefile->WriteBool( bBossBar );
	savefile->WriteInt( nextSpeechTime );
	savefile->WriteAngles( lookOffset );
	savefile->WriteVec3( spawnOrigin );
	savefile->WriteBool( bBindOrient );
	savefile->WriteInt( numDDADamageSamples );
	savefile->WriteFloat( totalDDADamage );
	savefile->WriteInt( spawnThinkFlags );
	savefile->WriteBool( bCanWallwalk );
	savefile->WriteBool( bOverrideKilledByGravityZones );
	savefile->WriteInt( fallDelay );
	savefile->WriteBool( soundOnModel );
	savefile->WriteBool( bBindAxis );
	savefile->WriteBool( bCustomBlood );
	savefile->WriteBool( bNoCombat );
	savefile->WriteBool( bNeverTarget );
	savefile->WriteInt( nextSpiritProxyCheck );
};

/*
=====================
hhMonsterAI::Restore
=====================
*/
void hhMonsterAI::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( targetReaction.reactionIndex );
	targetReaction.entity.Restore( savefile);

	shootTarget.Restore( savefile );
	currPassageway.Restore( savefile );

	savefile->ReadBool( bCanFall );

	int i, num;
	savefile->ReadInt( num );
	healthTriggers.SetNum( num );
	for (i = 0; i < num; i++) {
		savefile->ReadInt( healthTriggers[i].healthThresh );
		healthTriggers[i].triggerEnt.Restore( savefile );
		savefile->ReadInt( healthTriggers[i].triggerCount );
	}
	savefile->ReadInt( hearingRange );
	savefile->ReadInt( lastContactTime );
	savefile->ReadBool( bSeeThroughPortals );
	savefile->ReadBool( bBossBar );
	savefile->ReadInt( nextSpeechTime );
	savefile->ReadAngles( lookOffset );
	savefile->ReadVec3( spawnOrigin );
	savefile->ReadBool( bBindOrient );
	savefile->ReadInt( numDDADamageSamples );
	savefile->ReadFloat( totalDDADamage );
	savefile->ReadInt( spawnThinkFlags );
	savefile->ReadBool( bCanWallwalk );
	savefile->ReadBool( bOverrideKilledByGravityZones );
	savefile->ReadInt( fallDelay );
	savefile->ReadBool( soundOnModel );
	savefile->ReadBool( bBindAxis );
	savefile->ReadBool( bCustomBlood );
	savefile->ReadBool( bNoCombat );
	savefile->ReadBool( bNeverTarget );
	savefile->ReadInt( nextSpiritProxyCheck );

	allSimpleMonsters.AddUnique(this);

	nextTurnUpdate = 0;

	if ( AI_VEHICLE ) {
		if ( GetVehicleInterface() ) {
			ResetClipModel();
			hhVehicle *vehicle = GetVehicleInterface()->GetVehicle();
			spawnArgs.Set( "use_aas", spawnArgs.GetString( "aas_shuttle", "aasDroid" ) );
			SetAAS();
			SetState( GetScriptFunction( "state_VehicleCombat" ) );
		}
	}
};

/*
=====================
hhMonsterAI::GetTurnDir
Returns -1 if left, 0 if facing ideal, and 1 if turning right
=====================
*/
int hhMonsterAI::GetTurnDir( void ) {
	float diff;

	if ( !turnRate ) {
		return 0;
	}

	diff = idMath::AngleNormalize180( current_yaw - ideal_yaw );
	if ( idMath::Fabs( diff ) < 0.01f ) {
		// force it to be exact
		current_yaw = ideal_yaw;
		return 0;
	}

	return (diff < 0 ? -1 : 1);
}



//=============================================================================
//
// hhMonsterAI::SendDamageToDDA()
//
// Informs the player of the amount of damage inflicted upon it until this
// creature was killed.
//=============================================================================

void hhMonsterAI::SendDamageToDDA() {
	if ( gameLocal.isMultiplayer ) {
		return;
	}

	int ddaIndex = spawnArgs.GetInt( "ddaIndex", "0" );

	if ( gameLocal.GetDDA() && ddaIndex >= 0 ) { // Only include certain creatures in the DDA calculation (exclude such things as crawlers and NPCs)
		gameLocal.GetDDA()->DDA_AddDamage( ddaIndex, totalDDADamage );
		gameLocal.GetDDA()->DDA_AddSurvivalHealth( ddaIndex, gameLocal.GetLocalPlayer()->GetHealth() );
	}

	totalDDADamage = 0;
}

void hhMonsterAI::FlyMove( void ) {
	idVec3	goalPos;
	idVec3	oldorigin;
	idVec3	newDest;

	AI_BLOCKED = false;
	if ( ( move.moveCommand != MOVE_NONE ) && ReachedPos( move.moveDest, move.moveCommand ) ) {
		StopMove( MOVE_STATUS_DONE );
	}

	if ( move.moveCommand != MOVE_TO_POSITION_DIRECT ) {
		idVec3 vel = physicsObj.GetLinearVelocity();

		if ( GetMovePos( goalPos ) ) {
			CheckObstacleAvoidance( goalPos, newDest );
			goalPos = newDest;
		}

		if ( !AI_FLY_NO_SEEK && move.speed ) {
			FlySeekGoal( vel, goalPos );
		}

		// add in bobbing
		AddFlyBob( vel );

		if ( enemy.GetEntity() && ( move.moveCommand != MOVE_TO_POSITION ) ) {
			AdjustFlyHeight( vel, goalPos );
		}

		AdjustFlySpeed( vel );

		physicsObj.SetLinearVelocity( vel );
	}

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

void hhMonsterAI::Activate( idEntity *activator ) {
	if ( spawnThinkFlags != 0 ) {
		int temp = spawnThinkFlags;
		spawnThinkFlags = 0;
		BecomeActive( temp );
	}
	idAI::Activate( activator );
}

void hhMonsterAI::Hide() {
	if ( spawnThinkFlags == 0 && ai_hideSkipThink.GetBool() ) {
		if ( ( spawnArgs.GetBool( "hide" ) || spawnArgs.GetBool( "portal" ) ) )  {
			spawnThinkFlags = thinkFlags;
			thinkFlags = 0;
		}
	}
	idAI::Hide();
}

void hhMonsterAI::HideNoDormant() {
	idAI::Hide();
}

void hhMonsterAI::BecomeActive( int flags ) {
	if ( spawnThinkFlags != 0 ) {
		spawnThinkFlags |= flags;
		return;
	}
	idAI::BecomeActive( flags );
}

void hhMonsterAI::HandleNoGore(void) {
	if (GERMAN_VERSION || g_nogore.GetBool()) {
		fl.takedamage = false;
		fl.canBeTractored = false;
		GetPhysics()->SetContents(0);
		float time = spawnArgs.GetFloat("nogore_dispose_time", "1");
		if (time > 0.0f) {
			//PostEventSec(&EV_Dispose, time);
		}
	}
}

bool hhMonsterAI::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	if ( soundOnModel ) {
		idVec3 boneOrigin;
		idMat3 boneAxis;
		jointHandle_t bone = animator.GetJointHandle( spawnArgs.GetString( "sound_joint", "b1" ) );
		if ( bone != INVALID_JOINT && animator.GetJointTransform( bone, gameLocal.time, boneOrigin, boneAxis ) ) {
			origin = boneOrigin;
			axis = boneAxis * viewAxis;
			return true;
		}
	}
	return idAI::GetPhysicsToSoundTransform( origin, axis );
}

void hhMonsterAI::AddLocalMatterWound( jointHandle_t jointNum, const idVec3 &localOrigin, const idVec3 &localNormal, const idVec3 &localDir, int damageDefIndex, const idMaterial *collisionMaterial ) {
	if ( bCustomBlood ) {
		return;
	}

	const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, damageDefIndex ) );
	if ( def == NULL ) {
		return;
	}

	surfTypes_t matterType = gameLocal.GetMatterType( this, collisionMaterial, "idEntity::AddLocalMatterWound" );

	GetWoundManager()->AddWounds( def, matterType, jointNum, localOrigin, localNormal, localDir );

	if ( head.IsValid() ) {
		head->AddLocalMatterWound( jointNum, localOrigin, localNormal, localDir, damageDefIndex, collisionMaterial );
	}
	//TODO: Grab head projection code from below and put into ApplyImpactMark()
}
