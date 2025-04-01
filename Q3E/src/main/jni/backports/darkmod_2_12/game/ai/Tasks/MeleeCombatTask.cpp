/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "MeleeCombatTask.h"
#include "SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& MeleeCombatTask::GetName() const
{
	static idStr _name(TASK_MELEE_COMBAT);
	return _name;
}

void MeleeCombatTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	CombatTask::Init(owner, subsystem);
	
	_bForceAttack = false;
	_bForceParry = false;
	_bInPreParryDelayState = false;
	_bInPostParryDelayState = false;
	_ParryDelayTimer = 0;
	_PreParryDelay = 0;
	_PostParryDelay = 0;
	_PrevEnemy = NULL;
	_PrevAttParried = MELEETYPE_UNBLOCKABLE;
	_PrevAttTime = 0;
	_NumAttReps = 0;

	// grayman #3331 - note if your enemy uses an unarmed melee attack. If so,
	// you can skip parries.

	idActor* enemy = _enemy.GetEntity();
	if ( enemy )
	{
		_EnemyUsesUnarmedCombat = enemy->spawnArgs.GetBool("unarmed_melee","0");
	}
}

bool MeleeCombatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Melee Combat Task performing.\r");

	idAI* ownerEnt = _owner.GetEntity();
	assert(ownerEnt != NULL);

	idActor* enemy = _enemy.GetEntity();
	if ( ( enemy == NULL ) || enemy->IsKnockedOut() || ( enemy->health <= 0 ) )
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("No enemy, terminating task!\r");
		return true; // terminate me
	}

	// Perform the task according to the current action
	EMeleeActState actState = ownerEnt->m_MeleeStatus.m_ActionState;

	switch (actState)
	{
	case MELEEACTION_READY:
		PerformReady(ownerEnt);
		break;
	case MELEEACTION_ATTACK:
		PerformAttack(ownerEnt);
		break;
	case MELEEACTION_PARRY:
		PerformParry(ownerEnt);
		break;
	default:
		PerformReady(ownerEnt);
	};
	
	return false; // not finished yet
}

void MeleeCombatTask::PerformReady(idAI* owner)
{
	// the "ready" state is where we decide whether to attack or parry
	// For now, always attack
	CMeleeStatus& ownerStatus = owner->m_MeleeStatus;

	idActor* enemy = _enemy.GetEntity();
	CMeleeStatus& enemyStatus = enemy->m_MeleeStatus;

	if( cv_melee_state_debug.GetBool() )
	{
		idStr debugText = "MeleeAction: Ready";
		gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
	}

	// TODO: Cache these rather than calc. every frame?
	int NextAttTime;
	if( ownerStatus.m_ActionResult == MELEERESULT_PAR_BLOCKED 
		|| ownerStatus.m_ActionResult == MELEERESULT_PAR_ABORTED )
	{
		// just parried, so use the riposte recovery time for the attack
		NextAttTime = ownerStatus.m_LastActTime + owner->m_MeleeCurrentRiposteRecovery;
	}
	// longer recovery if we were parried
	else if( ownerStatus.m_ActionResult == MELEERESULT_AT_PARRIED )
	{
		NextAttTime = ownerStatus.m_LastActTime + owner->m_MeleeCurrentAttackLongRecovery;
	}
	else
	{
		NextAttTime = ownerStatus.m_LastActTime + owner->m_MeleeCurrentAttackRecovery;
	}

	int NextParTime = ownerStatus.m_LastActTime + owner->m_MeleeCurrentParryRecovery;

	// ATTACK: If the timer allows us and if the enemy is in range
	if (gameLocal.time > NextAttTime 
		&& (owner->GetMemory().canHitEnemy || owner->GetMemory().willBeAbleToHitEnemy)
		&& !_bForceParry )
	{
		StartAttack(owner);
	}

	// PARRY:
	// If we can't attack and our enemy is attacking in range
	// and if the attack is still dangerous (not recovering)
	// TODO: Figure out how to switch enemies to face & parry a new one
	else if
		( 
			ownerStatus.m_bCanParry
			&& gameLocal.time > NextParTime
			&& (enemyStatus.m_ActionState == MELEEACTION_ATTACK)
			&& (enemyStatus.m_ActionPhase != MELEEPHASE_RECOVERING)
			&& owner->GetMemory().canBeHitByEnemy
			&& !_bForceAttack
			&& !_EnemyUsesUnarmedCombat // grayman #3331 - skip parries if enemy uses unarmed melee combat
		)
	{
		// check if our enemy made the same attack before
		EMeleeType AttType = enemyStatus.m_ActionType;
		if ( ( _PrevEnemy.GetEntity() == enemy ) && ( _PrevAttParried == AttType )
			&& (gameLocal.time - _PrevAttTime) < owner->m_MeleeRepAttackTime )
		{
			_NumAttReps++;
			// reset timer for next attack
			_PrevAttTime = gameLocal.time; 
		}
		else
		{
			// attack was new
			_PrevEnemy = enemy;
			_PrevAttParried = AttType;
			_PrevAttTime = gameLocal.time;
			_NumAttReps = 1;
		}

		// Counter attack if enemy is in range and chance check succeeds
		if( (owner->GetMemory().canHitEnemy || owner->GetMemory().willBeAbleToHitEnemy)
			&& gameLocal.random.RandomFloat() < owner->m_MeleeCounterAttChance )
		{
			StartAttack(owner);
		}
		else
		{
			// normal procedure, parry rather than counter attack
			StartParry(owner);
		}
	}
	else
	{
		// If we can't attack or parry, wait until we can
	}
}

void MeleeCombatTask::PerformAttack(idAI* owner)
{
	CMeleeStatus& ownerStatus = owner->m_MeleeStatus;
	EMeleeActPhase phase = ownerStatus.m_ActionPhase;
	
	Memory& memory = owner->GetMemory();
	
	if( phase == MELEEPHASE_PREPARING )
	{
		if( cv_melee_state_debug.GetBool() )
		{
			idStr debugText = "MeleeAction: Attack, Phase: Preparing";
			gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
		}
		// don't do anything, animation will update melee status to holding when it reaches the hold point
		// FIX: Some animations don't have a hold point and just go straight through
		idStr waitState( owner->WaitState() );
		if( waitState != "melee_action" )
		{
			// Hack: animation is done, advance the state where it will be handled properly in the next frame
			owner->Event_MeleeActionReleased();
		}
		return;
	}
	else if( phase == MELEEPHASE_HOLDING )
	{
		// TODO: Decide whether to keep holding the attack or release
		// if player is out of range but close, maybe hold the swing and charge at them for a little while?

		if( cv_melee_state_debug.GetBool() )
		{
			idStr debugText = "MeleeAction: Attack, Phase: Holding";
			gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
		}
		
		// wait some finite time before releasing (for difficulty tweaking)
		if( (gameLocal.time - ownerStatus.m_PhaseChangeTime) > owner->m_MeleeCurrentHoldTime )
		{
			owner->Event_PauseAnim( ANIMCHANNEL_TORSO, false );
			owner->Event_MeleeActionReleased();
		}
	}
	// MELEEPHASE_EXECUTING OR MELEEPHASE_RECOVERING
	else
	{
		if( cv_melee_state_debug.GetBool() )
		{
			idStr debugText = "MeleeAction: Attack, Phase: Executing/Recovering";
			gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
		}

		// check if animation is finished (script will set this when it is)
		idStr waitState( owner->WaitState() );
		if( waitState != "melee_action" )
		{
			// if attack hasn't hit anything, switch to missed at this point
			if (ownerStatus.m_ActionResult == MELEERESULT_IN_PROGRESS)
			{
				ownerStatus.m_ActionResult = MELEERESULT_AT_MISSED;
			}
			else if (ownerStatus.m_ActionResult == MELEERESULT_AT_PARRIED)
			{
				// Emit our frustration bark with a certain probability (1 out of 3)
				if (gameLocal.random.RandomFloat() > 0.7f)
				{
					EmitCombatBark(owner, "snd_combat_blocked_by_player");
				}
			}
			else if (ownerStatus.m_ActionResult == MELEERESULT_AT_HIT)
			{
				// Emit our success bark with a certain probability (1 out of 3)
				if (gameLocal.random.RandomFloat() > 0.7f)
				{
					bool hasCompany = (MS2SEC(gameLocal.time - memory.lastTimeFriendlyAISeen) <= MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK);

					EmitCombatBark(owner, hasCompany ? "snd_combat_hit_player_company" : "snd_combat_hit_player");
				}
			}

			owner->Event_MeleeActionFinished();
		}
	}
}

void MeleeCombatTask::PerformParry(idAI* owner)
{
	CMeleeStatus& ownerStatus = owner->m_MeleeStatus;
	CMeleeStatus& enemyStatus = _enemy.GetEntity()->m_MeleeStatus;
	EMeleeActPhase phase = ownerStatus.m_ActionPhase;
	
	if ( phase == MELEEPHASE_PREPARING )
	{
		if( cv_melee_state_debug.GetBool() )
		{
			idStr debugText = "MeleeAction: Parry, Phase: Preparing";
			gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
		}

		// wait until done with initial delay, then start the animation
		if ( _bInPreParryDelayState && ((gameLocal.time - _ParryDelayTimer) > _PreParryDelay) )
		{
			// Set the waitstate, this gets cleared by script when the anim is done
			owner->SetWaitState("melee_action");

			const char *suffix = idActor::MeleeTypeNames[ownerStatus.m_ActionType];
			// script state plays the animation and clears wait state when done
			// TODO: Why did we have 5 blend frames here?
			owner->SetAnimState(ANIMCHANNEL_TORSO, va("Torso_Parry_%s",suffix), 5);

			_bInPreParryDelayState = false;
		}

		// don't do anything, animation will update status when it reaches hold point
		return;
	}

	if ( phase == MELEEPHASE_HOLDING )
	{
		if( cv_melee_state_debug.GetBool() )
		{
			idStr debugText = "MeleeAction: Parry, Phase: Holding";
			gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
		}

		// Decide whether to keep holding the parry or to release
		bool bRelease = false;

		// If our enemy is no longer attacking, release
		if( enemyStatus.m_ActionState != MELEEACTION_ATTACK
			|| enemyStatus.m_ActionPhase == MELEEPHASE_RECOVERING )
			bRelease = true;
		// If our enemy switches attacks, stop this parry to prepare the next
		else if( ownerStatus.m_ActionType != MELEETYPE_BLOCKALL && ownerStatus.m_ActionType != enemyStatus.m_ActionType )
			bRelease = true;
		// or if enemy is holding for over some time
		else if( enemyStatus.m_ActionPhase == MELEEPHASE_HOLDING
				 && ((gameLocal.time - enemyStatus.m_PhaseChangeTime) > owner->m_MeleeCurrentParryHold) )
		{
			// also force an attack next, so we don't just go back into parry - this creates an opening
			_bForceAttack = true;
			bRelease = true;
		}
		// TODO: Check if enemy is dead or beyond some max range, then stop parrying?
		else
		{
			// debug display the countdown to release
			if( cv_melee_state_debug.GetBool() )
			{
				idStr debugText = va("Parry Waiting for: %d [ms]", (gameLocal.time - enemyStatus.m_PhaseChangeTime) );
				gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-40), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
			}
			// otherwise, keep holding the parry
			bRelease = false;
		}

		if( bRelease )
		{
			// owner->Event_PauseAnim( ANIMCHANNEL_TORSO, false );
			_bInPostParryDelayState = true;
			_ParryDelayTimer = gameLocal.time;
			owner->Event_MeleeActionReleased();
		}
	}
	// MELEEPHASE_RECOVERING
	else
	{
		// recovery has two phases: 1. Wait for post-parry delay reaction time
		// and 2. wait for recovery animation to finish


		// If just finishing up the initial delay, start the animation
		if( _bInPostParryDelayState && ((gameLocal.time - _ParryDelayTimer) > _PostParryDelay) )
		{
			owner->Event_PauseAnim( ANIMCHANNEL_TORSO, false );
			_bInPostParryDelayState = false;
		}
		// post parry delay has already finished, wait for animation
		else if( !_bInPostParryDelayState )
		{
			// check if animation is finished (script will set this when it is)
			idStr waitState( owner->WaitState() );
			if( waitState != "melee_action" )
			{
				// if nothing happened with our parry, it was aborted
				if( ownerStatus.m_ActionResult == MELEERESULT_IN_PROGRESS )
					ownerStatus.m_ActionResult = MELEERESULT_PAR_ABORTED;

				owner->Event_MeleeActionFinished();
			}
		}
		// otherwise we wait for the post parry delay

		if( cv_melee_state_debug.GetBool() )
		{
			idStr debugText = "MeleeAction: Parry, Phase: Recovering";
			gameRenderWorld->DebugText( debugText, (owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*-25), 0.20f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );
		}
	}
}

void MeleeCombatTask::StartAttack(idAI* owner)
{
	CMeleeStatus& ownerStatus = owner->m_MeleeStatus;
	CMeleeStatus& enemyStatus = _enemy.GetEntity()->m_MeleeStatus;

	_bForceAttack = false;

	// create subset of possible attacks:
	idList<EMeleeType> attacks = ownerStatus.m_attacks;

	// if our enemy is parrying a direction, attack along a different direction
	// grayman #3331 - if we have only one attack, we can't remove it from
	// the attack list, otherwise we'll just stand there
	if ( ( enemyStatus.m_ActionState == MELEEACTION_PARRY ) && ( attacks.Num() > 1 ) )
	{
		if ( enemyStatus.m_ActionType != MELEETYPE_BLOCKALL )
		{
			attacks.Remove( enemyStatus.m_ActionType );
		}
		// TODO: Shield parries need special handling
		// Either attack the shield to destroy it or wait until it's dropped or flank the parry with footwork
	}

	int numAttacks = attacks.Num();

	if (numAttacks > 0)
	{
		// choose a random attack from our possible attacks
		int i = attacks[ gameLocal.random.RandomInt(numAttacks) ];

		const char *suffix = idActor::MeleeTypeNames[i];

		// update the melee status
		owner->Event_MeleeAttackStarted( i );

		// Emit a combat bark
		EmitCombatBark(owner, "snd_combat_melee");

		// Set the waitstate, this gets cleared by 
		// the script function when the animation is done.
		owner->SetWaitState("melee_action");

		// script state plays the animation, clearing wait state when done
		// TODO: Why did we have 5 blend frames here?
		owner->SetAnimState(ANIMCHANNEL_TORSO, va("Torso_Melee_%s",suffix), 5);
	}
	else
	{
		// all of our possible attacks are currently being parried
		// TODO: Decide what to do in this case
		// Wait forever?  Attack anyway?  Attack another opponent in our FOV?
	}
}

void MeleeCombatTask::StartParry(idAI* owner)
{
	CMeleeStatus& ownerStatus = owner->m_MeleeStatus;
	CMeleeStatus& enemyStatus = _enemy.GetEntity()->m_MeleeStatus;

	_bForceParry = false;
	
	EMeleeType AttType = enemyStatus.m_ActionType;
	EMeleeType ParType;

	// Universal (shield) parry is the best option if we can
	if( ownerStatus.m_bCanParryAll )
		ParType = MELEETYPE_BLOCKALL;
	else
		ParType = AttType; // match the attack

	//const char *suffix = idActor::MeleeTypeNames[ParType];

	// update the melee status
	owner->Event_MeleeParryStarted( ParType );

	// animation starting is postponed until after parry delay
	_bInPreParryDelayState = true;
	_ParryDelayTimer = gameLocal.time;

	// repeated attacks can have a faster parry response
	if( _NumAttReps < owner->m_MeleeNumRepAttacks )
	{
		_PreParryDelay = owner->m_MeleeCurrentPreParryDelay;
		_PostParryDelay = owner->m_MeleeCurrentPostParryDelay;
	}
	else
	{
		_PreParryDelay = owner->m_MeleeCurrentRepeatedPreParryDelay;
		_PostParryDelay = owner->m_MeleeCurrentRepeatedPostParryDelay;
	}
}

void MeleeCombatTask::OnFinish(idAI* owner)
{
	// ishtvan TODO: Will need different code for when attack is finish vs. parry?
	// TODO: Also need to figure out if we hit or miss, etc.
	CMeleeStatus& ownerStatus = owner->m_MeleeStatus;

	ownerStatus.m_ActionState = MELEEACTION_READY; // ??
	ownerStatus.m_ActionPhase = MELEEPHASE_PREPARING;

	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetWaitState("");
}

void MeleeCombatTask::Save(idSaveGame* savefile) const
{
	CombatTask::Save(savefile);

	savefile->WriteBool( _bForceAttack );
	savefile->WriteBool( _bForceParry );
	savefile->WriteBool( _bInPreParryDelayState );
	savefile->WriteBool( _bInPostParryDelayState );
	savefile->WriteInt( _ParryDelayTimer );
	savefile->WriteInt( _PreParryDelay );
	savefile->WriteInt( _PostParryDelay );
	_PrevEnemy.Save(savefile);
	savefile->WriteInt(_PrevAttParried);
	savefile->WriteInt(_PrevAttTime);
	savefile->WriteInt(_NumAttReps);
	savefile->WriteBool(_EnemyUsesUnarmedCombat); // grayman #3331
}

void MeleeCombatTask::Restore(idRestoreGame* savefile)
{
	CombatTask::Restore(savefile);

	savefile->ReadBool( _bForceAttack );
	savefile->ReadBool( _bForceParry );
	savefile->ReadBool( _bInPreParryDelayState );
	savefile->ReadBool( _bInPostParryDelayState );
	savefile->ReadInt( _ParryDelayTimer );
	savefile->ReadInt( _PreParryDelay );
	savefile->ReadInt( _PostParryDelay );
	_PrevEnemy.Restore(savefile);
	int i;
	savefile->ReadInt( i );
	_PrevAttParried = (EMeleeType) i;
	savefile->ReadInt(_PrevAttTime);
	savefile->ReadInt(_NumAttReps);
	savefile->ReadBool(_EnemyUsesUnarmedCombat); // grayman #3331
}

MeleeCombatTaskPtr MeleeCombatTask::CreateInstance()
{
	return MeleeCombatTaskPtr(new MeleeCombatTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar meleeCombatTaskRegistrar(
	TASK_MELEE_COMBAT, // Task Name
	TaskLibrary::CreateInstanceFunc(&MeleeCombatTask::CreateInstance) // Instance creation callback
);

} // namespace ai
