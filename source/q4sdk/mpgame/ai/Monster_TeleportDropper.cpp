
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../spawner.h"
#include "../Projectile.h"
#include "AI_Manager.h"

class rvMonsterTeleportDropper : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterTeleportDropper );

	rvMonsterTeleportDropper ( void );

	void						InitSpawnArgsVariables		( void );
	void						Spawn						( void );
	void						Save						( idSaveGame *savefile ) const;
	void						Restore						( idRestoreGame *savefile );

	virtual void				Think						( void );

	virtual idProjectile*		AttackRanged				( const char* attackName, const idDict* projectileDict, jointHandle_t joint, idEntity* target, const idVec3& pushVelocity = vec3_origin );
	virtual const char*			GetIdleAnimName				( void );
	virtual void				GetDebugInfo				( debugInfoProc_t proc, void* userData );

	virtual bool				CheckAction_LeapAttack		( rvAIAction* action, int animNum );

protected:

	idEntityPtr<rvSpawner>		spawner;

	// Actions
	rvAIAction					actionDropSpawners;

	//bool						dropAtGoalOnly;

//	virtual void				OnStopMoving				( aiMoveCommand_t oldMoveCommand );
//	virtual bool				MoveToEnemy					( void );

	virtual int					FilterTactical				( int availableTactical );

	virtual bool				CheckActions				( void );

	virtual stateResult_t		State_CombatHide					( const stateParms_t& parms );

private:

	bool						leftSideBlocked;
	bool						rightSideBlocked;
	jointHandle_t				jointLeftFrontCannon;
	jointHandle_t				jointLeftRearCannon;
	jointHandle_t				jointRightFrontCannon;
	jointHandle_t				jointRightRearCannon;

	bool						leapDidAttack;

	bool						CheckAction_DropSpawners	( rvAIAction* action, int animNum );
	// Torso states
	stateResult_t				State_Torso_DropSpawners	( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterTeleportDropper );
};

CLASS_DECLARATION( idAI, rvMonsterTeleportDropper )
END_CLASS

/*
================
rvMonsterTeleportDropper::rvMonsterTeleportDropper
================
*/
rvMonsterTeleportDropper::rvMonsterTeleportDropper ( void ) {
	spawner = NULL;
	leftSideBlocked = rightSideBlocked = false;
	leapDidAttack = false;
//	dropAtGoalOnly = false;
}

void rvMonsterTeleportDropper::InitSpawnArgsVariables ( void )
{
	jointLeftFrontCannon = animator.GetJointHandle ( spawnArgs.GetString ( "joint_left_front_cannon" ) );
	jointLeftRearCannon = animator.GetJointHandle ( spawnArgs.GetString ( "joint_left_rear_cannon" ) );
	jointRightFrontCannon = animator.GetJointHandle ( spawnArgs.GetString ( "joint_right_front_cannon" ) );
	jointRightRearCannon = animator.GetJointHandle ( spawnArgs.GetString ( "joint_right_rear_cannon" ) );
}

/*
================
rvMonsterTeleportDropper::Spawn
================
*/
void rvMonsterTeleportDropper::Spawn ( void ) {
	idEntity*	ent;
	idDict		args;
	
	// Create the spawner entity
	args.Clear ( );
	args.Set ( "classname", spawnArgs.GetString ( "def_spawner" ) );
	gameLocal.SpawnEntityDef ( args, &ent );

	if ( ent ) {	
		spawner = static_cast<rvSpawner*>(ent);
		spawner->ProcessEvent ( &EV_Activate, this );
	}

	// Define actions	
	actionDropSpawners.Init ( spawnArgs, "action_dropSpawners",	"Torso_DropSpawners", 0 );		

	InitSpawnArgsVariables();
}

/*
================
rvMonsterTeleportDropper::Save
================
*/
void rvMonsterTeleportDropper::Save ( idSaveGame *savefile ) const {
	spawner.Save( savefile );

	actionDropSpawners.Save( savefile );
	savefile->WriteBool(leftSideBlocked);
	savefile->WriteBool(rightSideBlocked);
	savefile->WriteBool(leapDidAttack);
}

/*
================
rvMonsterTeleportDropper::Restore
================
*/
void rvMonsterTeleportDropper::Restore ( idRestoreGame *savefile ) {
	spawner.Restore( savefile );

	actionDropSpawners.Restore( savefile );
	savefile->ReadBool(leftSideBlocked);
	savefile->ReadBool(rightSideBlocked);
	savefile->ReadBool(leapDidAttack);

	InitSpawnArgsVariables();
}

/*
================
rvMonsterTeleportDropper::Think
================
*/
void rvMonsterTeleportDropper::Think ( void ) {
	idAI::Think ( );
	if ( aifl.action && actionLeapAttack.status == rvAIAction::STATUS_OK && !leapDidAttack ) {
		//in leap attack action
		if ( GetEnemy() && enemy.fl.inFov && enemy.range < 64.0f )
		{
			const idDict* attackDict;
			attackDict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_attack_leap" ), false );
			AttackMelee( "leap", attackDict );
			StartSound( "snd_leap_hit", SND_CHANNEL_BODY, 0, 0, 0 );
			leapDidAttack = true;
		}
	}
}

/*
================
rvMonsterTeleportDropper::State_CombatHide
================
*/
stateResult_t rvMonsterTeleportDropper::State_CombatHide ( const stateParms_t& parms ) {
	// Turn toward the enemy if visible but not in fov
	if ( IsEnemyVisible ( ) ) {
		if ( !move.fl.moving ) {
			TurnToward ( enemy.lastKnownPosition );
		}
	}

	if ( !(FilterTactical( combat.tacticalMaskAvailable )&AITACTICAL_HIDE_BIT) )
	{//shouldn't hide anymore
		ForceTacticalUpdate();
	}

	if ( UpdateTactical ( 5000 ) ) {
		return SRESULT_DONE_WAIT;
	}

	// Perform actions
	if ( UpdateAction ( ) ) {
		return SRESULT_WAIT;
	}	

	return SRESULT_WAIT;
}

/*
void rvMonsterTeleportDropper::OnStopMoving ( aiMoveCommand_t oldMoveCommand ) {
	idAI::OnStopMoving(oldMoveCommand);
	dropAtGoalOnly = false;
}

bool rvMonsterTeleportDropper::MoveToEnemy ( void ) {
	dropAtGoalOnly = false;
	if ( !targets.Num() )
	{
		return (idAI::MoveToEnemy());
	}
	if ( !GetEnemy() )
	{
		return false;
	}
	idEntity* goalEnt;
	idEntity* bestGoal = NULL;
	int areaNum = 0;
	aasPath_t	path;
	idVec3		pos;
	float		dist;
	float		bestDist = actionDropSpawners.maxRange;

	for( int i = 0; i < targets.Num(); i++ ) {
		goalEnt = targets[ i ].GetEntity();
		if ( !goalEnt )
		{
			continue;
		}
		pos = goalEnt->GetPhysics()->GetOrigin();
		if ( !aiManager.ValidateDestination(this,goalEnt->GetPhysics()->GetOrigin()) )
		{
			continue;
		}
		dist = GetEnemy()->DistanceTo(pos);
		if ( dist >= bestDist )
		{
			continue;
		}
		// See if it's possible to get where we want to go
		areaNum = 0;
		if ( aas ) {
			areaNum = PointReachableAreaNum( pos );
			aas->PushPointIntoAreaNum( areaNum, pos );

			if ( !PathToGoal( path, PointReachableAreaNum( physicsObj.GetOrigin() ), physicsObj.GetOrigin(), areaNum, pos ) ) {
				continue;
			}
		}
		dist = bestDist;
		bestGoal = goalEnt;
	}
	if ( bestGoal )
	{
		if ( MoveToEntity( bestGoal, bestGoal->spawnArgs.GetFloat("range","64") ) )
		{
			dropAtGoalOnly = true;
			return true;
		}
	}
	return (idAI::MoveToEnemy());
}
*/

/*
================
rvMonsterTeleportDropper::FilterTactical
================
*/
int rvMonsterTeleportDropper::FilterTactical ( int availableTactical ) {
	//do normal filter
	availableTactical = idAI::FilterTactical ( availableTactical );

	//being tethered removes these - add them back in, we don't obey tethers!  tethers are for chumps!
	availableTactical |= (AITACTICAL_MELEE_BIT|AITACTICAL_HIDE_BIT);

	// Only allow hiding while the drop spawner action isnt ready
	if ( !actionDropSpawners.timer.IsDone ( actionTime ) ) {
		availableTactical &= AITACTICAL_HIDE_BIT;
	}
	
	// Hide while the spawner is still active
	if ( spawner && (spawner->GetNumSpawnPoints ( ) || spawner->GetNumActive ( ) > 1 ) ) {
		availableTactical &= AITACTICAL_HIDE_BIT;
	}	

	if ( leftSideBlocked && rightSideBlocked )
	{//both sides are blocked
		if ( move.fl.done )
		{//not trying to move
			//get out of here!
			ForceTacticalUpdate();
			//try finding somewhere else to stand!
			availableTactical |= AITACTICAL_RANGED_BIT;
		}
	}
	return availableTactical;
}

/*
================
rvMonsterTeleportDropper::CheckAction_DropSpawners
================
*/
bool rvMonsterTeleportDropper::CheckAction_DropSpawners ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent  ) {
		return false;
	}
	if ( spawner && (spawner->GetNumSpawnPoints ( ) || spawner->GetNumActive ( ) > 1 ) ) {
		return false;
	}	
	if ( !IsEnemyRecentlyVisible ( ) ) {
		return false;
	}
	if ( animNum != -1 && !CanHitEnemyFromAnim( animNum ) ) {
		return false;
	}
	//Check to see if at least one side is open
	leftSideBlocked = rightSideBlocked = false;

	//trace against solid architecture only, don't care about other entities
	trace_t	wallTrace;
	idVec3 start, end;
	idMat3 axis;
	start = GetPhysics()->GetCenterMass();
	//NOTE: ASSUMPTION
	int mask = (MASK_SOLID|CONTENTS_LARGESHOTCLIP|CONTENTS_MOVEABLECLIP|CONTENTS_MONSTERCLIP);
	//check left
	end = start + (viewAxis[1] * 64.0f);
	gameLocal.TracePoint ( this, wallTrace, start, end, mask, this );
	if ( wallTrace.fraction < 1.0f ) {
		//left side is blocked
		leftSideBlocked = true;
	}
	//check right
	end = start - (viewAxis[1] * 64.0f);
	//trace against solid architecture only, don't care about other entities
	gameLocal.TracePoint ( this, wallTrace, start, end, mask, this );
	if ( wallTrace.fraction < 1.0f ) {
		//right side blocked
		rightSideBlocked = true;
	}
	if ( leftSideBlocked && rightSideBlocked )
	{
		return false;
	}

	/*
	if ( dropAtGoalOnly && !move.fl.done && move.moveCommand == MOVE_TO_ENTITY )
	{//FIXME: check ReachedPos?
		return false;
	}
	//fuck it, just check them all
	if ( targets.Num() )
	{
		idEntity* goalEnt;
		idVec3 pos;
		float dist;
		for( int i = 0; i < targets.Num(); i++ ) {
			goalEnt = targets[ i ].GetEntity();
			if ( !goalEnt )
			{
				continue;
			}
			pos = goalEnt->GetPhysics()->GetOrigin();
			dist = DistanceTo(pos);
			if ( dist <= goalEnt->spawnArgs.GetFloat("range","64") )
			{
				return true;
			}
		}
		return false;
	}
	*/

	return true;
}

/*
================
rvMonsterTeleportDropper::Collide
================
*/

/*
================
rvMonsterTeleportDropper::CheckAction_LeapAttack
================
*/
bool rvMonsterTeleportDropper::CheckAction_LeapAttack ( rvAIAction* action, int animNum ) {
	if ( combat.tacticalCurrent == AITACTICAL_HIDE )
	{
		if ( !move.fl.done 
			&& !move.fl.blocked )
		{//still running away
			return false;
		}
		//done running, okay to attack if in range
	}
	else
	{
		if ( !leftSideBlocked || !rightSideBlocked )
		{//clear to shoot on sides
			return false;
		}
	}

	if ( idAI::CheckAction_LeapAttack( action, animNum ) )
	{
		leapDidAttack = false;
		return true;
	}
	return false;
}

/*
================
rvMonsterTeleportDropper::CheckActions
================
*/
bool rvMonsterTeleportDropper::CheckActions ( void ) {
	if ( PerformAction ( &actionDropSpawners, (checkAction_t)&rvMonsterTeleportDropper::CheckAction_DropSpawners ) ) {
		return true;
	}
	if ( CheckPainActions ( ) ) {
		return true;
	}
	if ( PerformAction ( &actionLeapAttack,  (checkAction_t)&rvMonsterTeleportDropper::CheckAction_LeapAttack ) ) {
		return true;
	}

	return false;
}

/*
================
rvMonsterTeleportDropper::AttackMissileExt
================
*/
idProjectile* rvMonsterTeleportDropper::AttackRanged ( const char* attackName, const idDict* attackDict, jointHandle_t joint, idEntity* target, const idVec3& pushVelocity ) {
	idProjectile* proj;
	
	// Launch the projectile
	if ( leftSideBlocked || rightSideBlocked )
	{
		if ( idStr::Icmp( attackName, "dropSpawner" ) == 0 )
		{
			if ( leftSideBlocked )
			{
				if ( joint == jointLeftFrontCannon
					|| joint == jointLeftRearCannon )
				{
					return NULL;
				}
			}
			else if ( rightSideBlocked )
			{
				if ( joint == jointRightFrontCannon
					|| joint == jointRightRearCannon )
				{
					return NULL;
				}
			}
		}
	}
	proj = idAI::AttackRanged ( attackName, attackDict, joint, target, pushVelocity );

	if ( !proj ) {
		return NULL;
	}
	
	// If it was a spawner projectile set the spawer
	if ( proj->IsType ( rvSpawnerProjectile::GetClassType() ) ) {
		static_cast<rvSpawnerProjectile*>(proj)->SetSpawner ( spawner );
	}
	
	return proj;
}

/*
================
rvMonsterTeleportDropper::GetIdleAnimName
================
*/
const char* rvMonsterTeleportDropper::GetIdleAnimName ( void ) {
	if ( combat.tacticalCurrent == AITACTICAL_HIDE ) {
		return "idle_hide";
	}
	return idAI::GetIdleAnimName ( );
}

/*
================
rvMonsterTeleportDropper::GetDebugInfo
================
*/
void rvMonsterTeleportDropper::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "rvMonsterTeleportDropper", "action_dropSpawners",		aiActionStatusString[actionDropSpawners.status], userData );
	//proc ( "rvMonsterTeleportDropper", "dropAtGoalOnly",		dropAtGoalOnly?"true":"false", userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterTeleportDropper )
	STATE ( "State_CombatHide",			rvMonsterTeleportDropper::State_CombatHide )
	STATE ( "Torso_DropSpawners",		rvMonsterTeleportDropper::State_Torso_DropSpawners )
END_CLASS_STATES

/*
================
rvMonsterTeleportDropper::State_Torso_DropSpawners
================
*/
stateResult_t rvMonsterTeleportDropper::State_Torso_DropSpawners ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:			
			PlayAnim ( ANIMCHANNEL_TORSO, "drop_spawners", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				ForceTacticalUpdate ( );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;			
	}
	
	return SRESULT_DONE;
}
