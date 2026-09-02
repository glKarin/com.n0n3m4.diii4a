/*
================
Monster_Fatguy.cpp

AI for the fat guy on the putra level
================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Projectile.h"

class rvMonsterHarvester : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterHarvester );

	rvMonsterHarvester ( void );
	
	void				InitSpawnArgsVariables			( void );
	void				Spawn							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	virtual bool		Attack							( const char* attackName, jointHandle_t joint, idEntity* target, const idVec3& pushVelocity = vec3_origin );

	virtual bool		UpdateAnimationControllers		( void );
	bool				CanTurn							( void ) const;
	virtual int			GetDamageForLocation			( int damage, int location );
	virtual	void		Damage							( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual bool		SkipImpulse						( idEntity* ent, int id );

protected:

	enum {
		WHIP_LEFT,
		WHIP_CENTER,
		WHIP_RIGHT,
		WHIP_MAX
	};

	enum {
		PART_ARM_R,
		PART_ARM_L,
		PART_LEG_FR,
		PART_LEG_FL,
		PART_LEG_BR,
		PART_LEG_BL,
		PART_TANK_R,
		PART_TANK_L,
		PARTS_MAX
	};
	idStr				partLocation[PARTS_MAX];
	idStr				partSurf[PARTS_MAX];
	idStr				partJoint[PARTS_MAX];
	int					partHealth[PARTS_MAX];
	void				DestroyPart				( int part );

	rvAIAction			actionWhipAttack;
	rvAIAction			actionSprayAttack;
	rvAIAction			actionRocketAttack;
	rvAIAction			actionGrenadeAttack;
	
	jointHandle_t			whipJoints[WHIP_MAX];
	idEntityPtr<idEntity>	whipProjectiles[WHIP_MAX];

	jointHandle_t		jointLeftMuzzle;
	jointHandle_t		jointRightMuzzle;

	virtual bool		CheckActions			( void );
	virtual int			FilterTactical			( int availableTactical );

	const char*			GetMeleeAttackAnim		( const idVec3& target );
	bool				PlayMeleeAttackAnim		( const idVec3& target, int blendFrames );
	const char*			GetRangedAttackAnim		( const idVec3& target );
	bool				PlayRangedAttackAnim	( const idVec3& target, int blendFrames );

	int					maxShots;	
	int					minShots;
	int					shots;

	int					nextTurnTime;
	int					sweepCount;

private:

	void				DropLeg					( int part );
// Custom actions
	bool				CheckAction_WhipAttack	( rvAIAction* action, int animNum );
	virtual bool		CheckAction_MeleeAttack	( rvAIAction* action, int animNum );
	virtual bool		CheckAction_RangedAttack( rvAIAction* action, int animNum );
	bool				CheckAction_SprayAttack	( rvAIAction* action, int animNum );
	bool				CheckAction_RocketAttack( rvAIAction* action, int animNum );
	bool				CheckAction_GrenadeAttack( rvAIAction* action, int animNum );

	stateResult_t		State_Killed			( const stateParms_t& parms );
	stateResult_t		State_Dead				( const stateParms_t& parms );

	// Torso States
	stateResult_t		State_Torso_WhipAttack	( const stateParms_t& parms );
	stateResult_t		State_Torso_ClawAttack	( const stateParms_t& parms );
	stateResult_t		State_Torso_RangedAttack( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnRight90	( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnLeft90	( const stateParms_t& parms );
	stateResult_t		State_Torso_SprayAttack	( const stateParms_t& parms );
	stateResult_t		State_Torso_RocketAttack( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterHarvester );
};

CLASS_DECLARATION( idAI, rvMonsterHarvester )
END_CLASS

/*
================
rvMonsterHarvester::rvMonsterHarvester
================
*/
rvMonsterHarvester::rvMonsterHarvester ( void ) {
}

void rvMonsterHarvester::InitSpawnArgsVariables( void )
{
	whipJoints[WHIP_LEFT] = animator.GetJointHandle ( spawnArgs.GetString ( "joint_whip_left" ) );
	whipJoints[WHIP_RIGHT] = animator.GetJointHandle ( spawnArgs.GetString ( "joint_whip_right" ) );
	whipJoints[WHIP_CENTER] = animator.GetJointHandle ( spawnArgs.GetString ( "joint_whip_center" ) );

	maxShots = spawnArgs.GetInt ( "maxShots", "1" );
	minShots = spawnArgs.GetInt ( "minShots", "1" );

	for ( int part = 0; part < PARTS_MAX; part++ )
	{
		partLocation[part] = spawnArgs.GetString( va("part_%d_location",part), "" );
		partSurf[part] = spawnArgs.GetString( va("part_%d_surf",part), "" );
		partJoint[part] = spawnArgs.GetString( va("part_%d_joint",part), "" );
	}

	jointLeftMuzzle = animator.GetJointHandle ( spawnArgs.GetString ( "joint_muzzle_left_arm" ) );
	jointRightMuzzle = animator.GetJointHandle ( spawnArgs.GetString ( "joint_muzzle_right_arm" ) );
}
/*
================
rvMonsterHarvester::Spawn
================
*/
void rvMonsterHarvester::Spawn ( void ) {
	// Custom actions
	actionWhipAttack.Init ( spawnArgs, "action_whipAttack", "Torso_WhipAttack", AIACTIONF_ATTACK );
	actionSprayAttack.Init  ( spawnArgs, "action_sprayAttack",	"Torso_SprayAttack", AIACTIONF_ATTACK );
	actionRocketAttack.Init  ( spawnArgs, "action_rocketAttack",	"Torso_RocketAttack", AIACTIONF_ATTACK );
	actionGrenadeAttack.Init  ( spawnArgs, "action_grenadeAttack",	NULL, AIACTIONF_ATTACK );
	
	int i;
	for ( i = 0; i < WHIP_MAX; i ++ ) {	
		whipProjectiles[i] = NULL;
	}
	
	InitSpawnArgsVariables();
	shots	 = 0;

	for ( int part = 0; part < PARTS_MAX; part++ )
	{
		partHealth[part] = spawnArgs.GetInt( va("part_%d_health",part), "500" );
	}
}

/*
================
rvMonsterHarvester::Save
================
*/
void rvMonsterHarvester::Save ( idSaveGame *savefile ) const {
	actionWhipAttack.Save( savefile );
	actionSprayAttack.Save( savefile );
	actionRocketAttack.Save( savefile );
	actionGrenadeAttack.Save( savefile );
	
	int i;
	for ( i = 0; i < WHIP_MAX; i++ ) {
		savefile->WriteObject( whipProjectiles[i] );
	}

	savefile->WriteInt( nextTurnTime );
	savefile->WriteInt( sweepCount );
	savefile->WriteInt ( shots );

	for ( int part = 0; part < PARTS_MAX; part++ )
	{
		savefile->WriteInt( partHealth[part] );
	}
}

/*
================
rvMonsterHarvester::Restore
================
*/
void rvMonsterHarvester::Restore ( idRestoreGame *savefile ) {
	actionWhipAttack.Restore( savefile );
	actionSprayAttack.Restore( savefile );
	actionRocketAttack.Restore( savefile );
	actionGrenadeAttack.Restore( savefile );
	
	int i;
	for ( i = 0; i < WHIP_MAX; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass*&>( whipProjectiles[i] ) );
	}

	savefile->ReadInt( nextTurnTime );
	savefile->ReadInt( sweepCount );
	savefile->ReadInt ( shots );

	for ( int part = 0; part < PARTS_MAX; part++ )
	{
		savefile->ReadInt( partHealth[part] );
	}

	InitSpawnArgsVariables();
}

/*
================
rvMonsterHarvester::UpdateAnimationControllers
================
*/
bool rvMonsterHarvester::UpdateAnimationControllers ( void ) {
	idVec3 origin;
	idMat3 axis;
	idVec3 dir;
	idVec3 localDir;
	idVec3 target;

	if ( !idAI::UpdateAnimationControllers ( ) ) {
		return false;
	}

	return true;
}

/*
=====================
rvMonsterHarvester::SkipImpulse
=====================
*/
bool rvMonsterHarvester::SkipImpulse( idEntity* ent, int id ) {	
	return true;
}

void rvMonsterHarvester::DropLeg( int part )
{
	jointHandle_t joint = INVALID_JOINT;
	const char* legDef = NULL;
	switch ( part )
	{
	case PART_LEG_FR:
		joint = animator.GetJointHandle( "r_toe_front" );
		legDef = "def_leg_part1";
		break;
	case PART_LEG_FL:
		joint = animator.GetJointHandle( "l_toe_front" );
		legDef = "def_leg_part2";
		break;
	case PART_LEG_BR:
		joint = animator.GetJointHandle( "r_toe_back" );
		legDef = "def_leg_part4";
		break;
	case PART_LEG_BL:
		joint = animator.GetJointHandle( "l_toe_back" );
		legDef = "def_leg_part3";
		break;
	}
	if ( joint != INVALID_JOINT )
	{
		idEntity*	leg = gameLocal.SpawnEntityDef( spawnArgs.GetString( legDef ) );
		if ( leg )
		{
			idVec3 jointOrg;
			idMat3 jointAxis;
			animator.GetJointTransform( joint, gameLocal.GetTime(), jointOrg, jointAxis );
			jointOrg = renderEntity.origin + (jointOrg*renderEntity.axis);
			leg->GetPhysics()->SetOrigin( jointOrg );
			leg->GetPhysics()->SetAxis( jointAxis*renderEntity.axis );
			leg->PlayEffect( "fx_trail", vec3_origin, jointAxis, true, vec3_origin, true );
			if ( leg->IsType( idDamagable::GetClassType() ) )
			{//don't be destroyed for at least 5 seconds
				((idDamagable*)leg)->invincibleTime = gameLocal.GetTime() + 5000;
			}
			// push it
			if ( jointOrg.z > GetPhysics()->GetOrigin().z+20.0f )
			{//leg was blown off while in the air...
                jointHandle_t attachJoint = animator.GetJointHandle(partJoint[part].c_str());
				if ( attachJoint != INVALID_JOINT )
				{
					animator.GetJointTransform( attachJoint, gameLocal.GetTime(), jointOrg, jointAxis );
					jointOrg = renderEntity.origin + (jointOrg*renderEntity.axis);

					idVec3 impulse = leg->GetPhysics()->GetCenterMass() - jointOrg;
					impulse.z = 0;
					impulse.Normalize();
					impulse *= ((gameLocal.random.RandomFloat()*3.0f)+2.0f) * 1000;//away
					impulse.z = (gameLocal.random.CRandomFloat()*500.0f)+1000.0f;//up!
					leg->ApplyImpulse( this, 0, jointOrg, impulse );
				}
			}
		}
	}
}

void rvMonsterHarvester::DestroyPart( int part )
{
	idStr explodeFX;
	idStr trailFX;
	switch ( part )
	{
	case PART_ARM_R:
		if ( partHealth[PART_ARM_L] <= 0 )
		{
			actionRangedAttack.fl.disabled = true;
			actionSprayAttack.fl.disabled = true;
			//so we don't sit here and do nothing at medium range...?
			actionRocketAttack.minRange = actionRangedAttack.minRange;
		}
		explodeFX = "fx_destroy_part_arm";
		trailFX = "fx_destroy_part_trail_arm";
		break;
	case PART_ARM_L:
		if ( partHealth[PART_ARM_R] <= 0 )
		{
			actionRangedAttack.fl.disabled = true;
			actionSprayAttack.fl.disabled = true;
			//so we don't sit here and do nothing at medium range...?
			actionRocketAttack.minRange = actionRangedAttack.minRange;
		}
		explodeFX = "fx_destroy_part_arm";
		trailFX = "fx_destroy_part_trail_arm";
		break;
	//FIXME: spawn leg func_movables
	case PART_LEG_FR:
		DropLeg( part );
		actionMeleeAttack.fl.disabled = true;
		actionSprayAttack.fl.disabled = true;
		animPrefix = "dmg_frt";
		painAnim = "damaged";
		explodeFX = "fx_destroy_part_leg";
		trailFX = "fx_destroy_part_trail_leg";
		break;
	case PART_LEG_FL:
		DropLeg( part );
		actionMeleeAttack.fl.disabled = true;
		actionSprayAttack.fl.disabled = true;
		animPrefix = "dmg_flt";
		painAnim = "damaged";
		explodeFX = "fx_destroy_part_leg";
		trailFX = "fx_destroy_part_trail_leg";
		break;
	case PART_LEG_BR:
		DropLeg( part );
		actionMeleeAttack.fl.disabled = true;
		actionSprayAttack.fl.disabled = true;
		animPrefix = "dmg_brt";
		painAnim = "damaged";
		explodeFX = "fx_destroy_part_leg";
		trailFX = "fx_destroy_part_trail_leg";
		break;
	case PART_LEG_BL:
		DropLeg( part );
		actionMeleeAttack.fl.disabled = true;
		actionSprayAttack.fl.disabled = true;
		animPrefix = "dmg_blt";
		painAnim = "damaged";
		explodeFX = "fx_destroy_part_leg";
		trailFX = "fx_destroy_part_trail_leg";
		break;
	case PART_TANK_R:
		if ( partHealth[PART_TANK_L] <= 0 )
		{
			actionRocketAttack.fl.disabled = true;
			//so we don't sit here and do nothing at long range...?
			actionRangedAttack.maxRange = actionRocketAttack.maxRange;
		}
		explodeFX = "fx_destroy_part_tank";
		trailFX = "fx_destroy_part_trail_tank";
		break;
	case PART_TANK_L:
		if ( partHealth[PART_TANK_R] <= 0 )
		{
			actionRocketAttack.fl.disabled = true;
			//so we don't sit here and do nothing at long range...?
			actionRangedAttack.maxRange = actionRocketAttack.maxRange;
		}
		explodeFX = "fx_destroy_part_tank";
		trailFX = "fx_destroy_part_trail_tank";
		break;
	}
	HideSurface( partSurf[part].c_str() );
	PlayEffect( explodeFX, animator.GetJointHandle(partJoint[part].c_str()) );
	PlayEffect( trailFX, animator.GetJointHandle(partJoint[part].c_str()), true );
	//make sure it plays this pain
	actionTimerPain.Reset ( actionTime );
}

/*
================
rvMonsterHarvester::CanTurn
================
*/
bool rvMonsterHarvester::CanTurn ( void ) const {
	if ( !idAI::CanTurn ( ) ) {
		return false;
	}
	return (move.anim_turn_angles != 0.0f || move.fl.moving);
}

/*
=====================
rvMonsterHarvester::GetDamageForLocation
=====================
*/
int rvMonsterHarvester::GetDamageForLocation( int damage, int location ) {
	// If the part was hit only do damage to it
	const char* dmgGroup = GetDamageGroup ( location );
	if ( dmgGroup )
	{
		for ( int part = 0; part < PARTS_MAX; part++ )
		{
			if ( idStr::Icmp ( dmgGroup, partLocation[part].c_str() ) == 0 ) 
			{
				if ( partHealth[part] > 0 )
				{
					partHealth[part] -= damage;
					painAnim = "pain";
					if ( partHealth[part] <= 0 ) 
					{
						if ( animPrefix.Length() 
							&& (part == PART_LEG_FR
								|| part == PART_LEG_FL
								|| part == PART_LEG_BR
								|| part == PART_LEG_BL ) )
						{//just blew off a leg and already had one blown off...
							DestroyPart( part );
							//we dead
							health = 0;
							return damage;
						}
						else
						{
							//FIXME: big pain?
							DestroyPart( part );
						}
					}
					else
					{
						if ( !animPrefix.Length() )
						{
							switch ( part )
							{
							case PART_LEG_FR:
								painAnim = "leg_pain_fr";
								break;
							case PART_LEG_FL:
								painAnim = "leg_pain_fl";
								break;
							case PART_LEG_BR:
								painAnim = "leg_pain_br";
								break;
							case PART_LEG_BL:
								painAnim = "leg_pain_bl";
								break;
							}
						}
					}

					if ( pain.threshold < damage && health > 0 )
					//if ( move.anim_turn_angles == 0.0f )
					{//not in the middle of a turn
						AnimTurn( 0, true );
						PerformAction ( "Torso_Pain", 2, true );	
					}
				}
				//pain.takenThisFrame = damage;
				return 0;
			}
		}
	}

	if ( health <= spawnArgs.GetInt( "death_damage_threshold" )
		&& spawnArgs.GetInt( "death_damage_threshold" ) > damage )
	{//doesn't meet the minimum damage requirements to kill us
		return 0;
	}
	 
	return idAI::GetDamageForLocation ( damage, location );
}

/*
================
rvMonsterHarvester::Damage
================
*/
void rvMonsterHarvester::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								  const char *damageDefName, const float damageScale, const int location ) {
	if ( attacker == this ) {
		//don't take damage from ourselves
		return;
	}
	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

/*
================
rvMonsterHarvester::CheckAction_SprayAttack
================
*/
bool rvMonsterHarvester::CheckAction_SprayAttack ( rvAIAction* action, int animNum )
{
	if ( !enemy.ent || !enemy.fl.inFov || !CheckFOV( GetEnemy()->GetEyePosition(), 20.0f ) ) {
		return false;
	}
	if ( !IsEnemyRecentlyVisible ( ) || enemy.ent->DistanceTo ( enemy.lastKnownPosition ) > 128.0f ) {
		return false;
	}
	if ( GetEnemy()->GetPhysics()->GetLinearVelocity().Compare( vec3_origin ) )
	{//not moving
		return false;
	}
	return true;
}

/*
================
rvMonsterHarvester::CheckAction_RocketAttack
================
*/
bool rvMonsterHarvester::CheckAction_RocketAttack ( rvAIAction* action, int animNum )
{
	if ( !enemy.ent ) {
		return false;
	}
	if ( !IsEnemyRecentlyVisible ( ) || enemy.ent->DistanceTo ( enemy.lastKnownPosition ) > 128.0f ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterHarvester::CheckAction_GrenadeAttack
================
*/
bool rvMonsterHarvester::CheckAction_GrenadeAttack ( rvAIAction* action, int animNum )
{
	if ( !enemy.ent || CheckFOV( GetEnemy()->GetEyePosition(), 270.0f ) ) {
		return false;
	}
	if ( !IsEnemyRecentlyVisible ( ) || enemy.ent->DistanceTo ( enemy.lastKnownPosition ) > 128.0f ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterHarvester::CheckAction_WhipAttack
================::
*/
bool rvMonsterHarvester::CheckAction_WhipAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterHarvester::CheckAction_MeleeAttack
================
*/
bool rvMonsterHarvester::CheckAction_MeleeAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
		return false;
	}
	if ( !CheckFOV ( enemy.ent->GetPhysics()->GetOrigin(), 90 ) ) {
		return false;
	}
	if ( !GetMeleeAttackAnim( enemy.ent->GetEyePosition() ) )
	{
		return false;
	}
	return true;
}

/*
================
rvMonsterHarvester::CheckAction_RangedAttack
================
*/
bool rvMonsterHarvester::CheckAction_RangedAttack ( rvAIAction* action, int animNum ) {
	return ( idAI::CheckAction_RangedAttack(action,animNum) && enemy.ent && GetRangedAttackAnim( enemy.ent->GetEyePosition() ) );
}

/*
================
rvMonsterHarvester::CheckActions
================
*/
bool rvMonsterHarvester::CheckActions ( void ) {

	// such a dirty hack... I'm not sure what is actually wrong, but somehow nextTurnTime is getting to be a rediculously high number.
	// I have some more significant bugs that really need to be solved, so for now, this will have to do.
	if ( nextTurnTime > gameLocal.time + 500 ) {
		nextTurnTime = gameLocal.time-1;
	}

	// If not moving, try turning in place
	if ( !move.fl.moving && gameLocal.time > nextTurnTime ) {
		float turnYaw = idMath::AngleNormalize180 ( move.ideal_yaw - move.current_yaw ) ;
		if ( turnYaw > lookMax[YAW] * 0.6f || (turnYaw > 0 && GetEnemy() && !enemy.fl.inFov) ) {
			PerformAction ( "Torso_TurnLeft90", 4, true );
			return true;
		} else if ( turnYaw < -lookMax[YAW] * 0.6f || (turnYaw < 0 && GetEnemy() && !enemy.fl.inFov) ) {
			PerformAction ( "Torso_TurnRight90", 4, true );
			return true;
		}
	}

 	if ( CheckPainActions ( ) ) {
 		return true;
	}

	if ( PerformAction ( &actionWhipAttack, (checkAction_t)&rvMonsterHarvester::CheckAction_WhipAttack, NULL ) ) {
		return true;
	}
	if ( PerformAction ( &actionSprayAttack,  (checkAction_t)&rvMonsterHarvester::CheckAction_SprayAttack, &actionTimerRangedAttack ) ) {
		return true;
	}
	if ( PerformAction ( &actionRocketAttack,  (checkAction_t)&rvMonsterHarvester::CheckAction_RocketAttack, &actionTimerRangedAttack ) ) {
		return true;
	}
	if ( PerformAction ( &actionGrenadeAttack,  (checkAction_t)&rvMonsterHarvester::CheckAction_GrenadeAttack, &actionTimerRangedAttack ) ) {
		return true;
	}

	return idAI::CheckActions ( );
}

/*
================
rvMonsterHarvester::FilterTactical
================
*/
int rvMonsterHarvester::FilterTactical ( int availableTactical ) {
	return availableTactical & (AITACTICAL_TURRET_BIT|AITACTICAL_RANGED_BITS|AITACTICAL_MELEE_BIT);
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterHarvester )
	STATE ( "State_Killed",			rvMonsterHarvester::State_Killed )
	STATE ( "State_Dead",			rvMonsterHarvester::State_Dead )

	STATE ( "Torso_WhipAttack",		rvMonsterHarvester::State_Torso_WhipAttack )
	STATE ( "Torso_ClawAttack",		rvMonsterHarvester::State_Torso_ClawAttack )
	STATE ( "Torso_RangedAttack",	rvMonsterHarvester::State_Torso_RangedAttack )
	STATE ( "Torso_TurnRight90",	rvMonsterHarvester::State_Torso_TurnRight90 )
	STATE ( "Torso_TurnLeft90",		rvMonsterHarvester::State_Torso_TurnLeft90 )
	STATE ( "Torso_SprayAttack",	rvMonsterHarvester::State_Torso_SprayAttack )
	STATE ( "Torso_RocketAttack",	rvMonsterHarvester::State_Torso_RocketAttack )
END_CLASS_STATES

/*
================
rvMonsterHarvester::State_Killed
================
*/
stateResult_t rvMonsterHarvester::State_Killed ( const stateParms_t& parms ) {
	DisableAnimState ( ANIMCHANNEL_LEGS );

	int numLegsLost = 0;
	for ( int i = PART_LEG_FR; i <= PART_LEG_BL; i++ ) {
		if ( partHealth[i] <= 0 ) {
			numLegsLost++;
		}
	}
	if ( numLegsLost > 1 ) {
		//dmg_death when 2 legs are blown off
		PlayAnim( ANIMCHANNEL_TORSO, "dmg_death", 0 );
	} else {
		PlayAnim( ANIMCHANNEL_TORSO, "death", 0 );
	}
	PostState ( "State_Dead" );
	return SRESULT_DONE;	
}

/*
================
rvMonsterHarvester::State_Dead
================
*/
stateResult_t rvMonsterHarvester::State_Dead ( const stateParms_t& parms ) {
	if ( !AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
		// Make sure all animation stops
		StopAnimState ( ANIMCHANNEL_TORSO );
		StopAnimState ( ANIMCHANNEL_LEGS );
		if ( head ) {
			StopAnimState ( ANIMCHANNEL_HEAD );
		}
		return SRESULT_WAIT;
	}
	return idAI::State_Dead ( parms );
}

/*
================
rvMonsterHarvester::State_Torso_WhipAttack
================
*/
stateResult_t rvMonsterHarvester::State_Torso_WhipAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_ATTACK,
		STAGE_ATTACK_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_ATTACK: {
			if ( !enemy.ent ) {
				return SRESULT_DONE;	
			}

			idEntity* ent;
			int		  i;
			
			for ( i = 0; i < WHIP_MAX; i ++ ) {				
				gameLocal.SpawnEntityDef( *gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_attack_whip" ) ), &ent, false );
				idProjectile* proj = dynamic_cast<idProjectile*>(ent);
				if ( !proj ) {
					delete ent;
					continue;
				}
				
				proj->Create ( this, vec3_origin, idVec3(0,0,1) );
				proj->Launch ( vec3_origin, idVec3(0,0,1), vec3_origin );
				
				whipProjectiles[i] = proj;
				ent->BindToJoint ( this, whipJoints[i], false );
				ent->SetOrigin ( vec3_origin );
				ent->SetAxis ( mat3_identity );
			}

			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack", parms.blendFrames );

			return SRESULT_STAGE ( STAGE_ATTACK_WAIT );
		}
			
		case STAGE_ATTACK_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				int i;
				for ( i = 0; i < WHIP_MAX; i ++ ) {	
					delete whipProjectiles[i];
					whipProjectiles[i] = NULL;
				}
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
=====================
rvMonsterHarvester::Attack
=====================
*/
bool rvMonsterHarvester::Attack ( const char* attackName, jointHandle_t joint, idEntity* target, const idVec3& pushVelocity ) {
	//NOTE: this stops spawning of projectile, but not muzzle flash... oh well...
	if ( joint == jointLeftMuzzle && partHealth[PART_ARM_L] <= 0 )
	{//can't fire from this muzzle - arm gone
		return false;
	}
	if ( joint == jointRightMuzzle && partHealth[PART_ARM_R] <= 0 )
	{//can't fire from this muzzle - arm gone
		return false;
	}
	return idAI::Attack( attackName, joint, target, pushVelocity );
}

/*
================
rvMonsterHarvester::GetMeleeAttackAnim
================
*/
const char* rvMonsterHarvester::GetMeleeAttackAnim ( const idVec3& target ) {
	idVec3 		dir;
	idVec3 		localDir;
	float  		yaw;
	const char* animName;
	
	// Get the local direction vector
	dir = target - GetPhysics()->GetOrigin();
	dir.Normalize ( );
	viewAxis.ProjectVector( dir, localDir );
	
	// Get the yaw relative to forward
	yaw = idMath::AngleNormalize180 ( localDir.ToAngles ( )[YAW] );
	
	if ( yaw < -10.0f ) {
		if ( partHealth[PART_LEG_FR] <= 0 )
		{
			return false;
		}
		animName = "attack_rleg_fw_rt";
	} else if ( yaw > 10.0f ) {
		if ( partHealth[PART_LEG_FL] <= 0 )
		{
			return false;
		}
		animName = "attack_lleg_fw_lt";
	} else{
		if ( gameLocal.random.RandomFloat() < 0.5f || partHealth[PART_LEG_FR] <= 0 )
		{
			if ( partHealth[PART_LEG_FL] <= 0 )
			{
				return false;
			}
			animName = "attack_lleg_fw";
		}
		else
		{
			if ( partHealth[PART_LEG_FR] <= 0 )
			{
				return false;
			}
			animName = "attack_rleg_fw";
		}
	}
	return animName;
}
/*
================
rvMonsterHarvester::PlayMeleeAttackAnim
================
*/
bool rvMonsterHarvester::PlayMeleeAttackAnim ( const idVec3& target, int blendFrames ) {
	const char* animName = GetMeleeAttackAnim( target );
	if ( animName )
	{
		PlayAnim ( ANIMCHANNEL_TORSO, animName, blendFrames );
		return true;
	}
	return false;
}

/*
================
rvMonsterHarvester::GetRangedAttackAnim
================
*/
const char* rvMonsterHarvester::GetRangedAttackAnim ( const idVec3& target ) {
	idVec3 		dir;
	idVec3 		localDir;
	float  		yaw;
	const char* animName = NULL;
	
	// Get the local direction vector
	dir = target - GetPhysics()->GetOrigin();
	dir.Normalize ( );
	viewAxis.ProjectVector( dir, localDir );
	
	// Get the yaw relative to forward
	yaw = idMath::AngleNormalize180 ( localDir.ToAngles ( )[YAW] );
	
	if ( yaw < -20.0f ) {
		if ( partHealth[PART_ARM_R] <= 0 )
		{
			return NULL;
		}
		animName = "fire_right";
	} else if ( yaw > 20.0f ) {
		if ( partHealth[PART_ARM_L] <= 0 )
		{
			return NULL;
		}
		animName = "fire_left";
	} else{
		animName = "fire_forward";
	}
	
	return animName;
}

/*
================
rvMonsterHarvester::PlayRangedAttackAnim
================
*/
bool rvMonsterHarvester::PlayRangedAttackAnim ( const idVec3& target, int blendFrames ) {
	const char* animName = GetRangedAttackAnim( target );
	if ( animName )
	{
		if ( !move.fl.moving )
		{
			DisableAnimState( ANIMCHANNEL_LEGS );
		}
		PlayAnim ( ANIMCHANNEL_TORSO, animName, blendFrames );
		return true;
	}
	return false;
}

/*
================
rvMonsterHarvester::State_Torso_ClawAttack
================
*/
stateResult_t rvMonsterHarvester::State_Torso_ClawAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_ATTACK,
		STAGE_ATTACK_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_ATTACK: {
			if ( !enemy.ent ) {
				return SRESULT_DONE;	
			}
		
			// Predict a bit
			if ( !PlayMeleeAttackAnim ( enemy.ent->GetEyePosition(), parms.blendFrames ) )
			{
				return SRESULT_DONE;
			}

			return SRESULT_STAGE ( STAGE_ATTACK_WAIT );
		}
			
		case STAGE_ATTACK_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
//				animator.ClearAllJoints ( );
//				leftChainOut = false;
//				rightChainOut = false;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterHarvester::State_Torso_RangedAttack
================
*/
stateResult_t rvMonsterHarvester::State_Torso_RangedAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_ATTACK,
		STAGE_ATTACK_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT: 
			if ( !enemy.ent ) {
				return SRESULT_DONE;	
			}
			shots = (minShots + gameLocal.random.RandomInt(maxShots-minShots+1)) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_ATTACK );
		
		case STAGE_ATTACK: 
			if ( !enemy.ent || !PlayRangedAttackAnim ( enemy.ent->GetEyePosition(), 0 ) )
			{
				return SRESULT_DONE;	
			}
			return SRESULT_STAGE ( STAGE_ATTACK_WAIT );
			
		case STAGE_ATTACK_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				shots--;
				if ( GetEnemy() && !enemy.fl.inFov )
				{//just stop
				}
				else if ( shots > 0 || !GetEnemy() )
				{
					return SRESULT_STAGE ( STAGE_ATTACK );
				}
//				animator.ClearAllJoints ( );
//				leftChainOut = false;
//				rightChainOut = false;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterHarvester::State_Torso_TurnRight90
================
*/
stateResult_t rvMonsterHarvester::State_Torso_TurnRight90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_90_rt", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 ) || !strstr( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "turn_90_rt" ) ) {
				AnimTurn ( 0, true );
				nextTurnTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			if ( GetEnemy() && !CheckFOV( GetEnemy()->GetEyePosition(), 270.0f ) )
			{//enemy behind me
				if ( actionGrenadeAttack.timer.IsDone(gameLocal.GetTime()) )
				{//timer okay
					//toss some nades at him
					PlayEffect( "fx_grenade_muzzleflash", animator.GetJointHandle("r_side_can_tip") );
					Attack( "grenade", animator.GetJointHandle("r_side_can_tip"), enemy.ent );
					PlayEffect( "fx_grenade_muzzleflash", animator.GetJointHandle("l_side_can_tip") );
					Attack( "grenade", animator.GetJointHandle("l_side_can_base"), enemy.ent );
					PlayEffect( "fx_grenade_muzzleflash", animator.GetJointHandle("back_can_tip") );
					Attack( "grenade", animator.GetJointHandle("back_can_base"), enemy.ent );
					actionGrenadeAttack.timer.Clear( gameLocal.GetTime() + gameLocal.random.RandomInt(1000)+500 );
				}
			}

			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterHarvester::State_Torso_TurnLeft90
================
*/
stateResult_t rvMonsterHarvester::State_Torso_TurnLeft90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_90_lt", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 ) || !strstr( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "turn_90_lt" ) ) {
				AnimTurn ( 0, true );
				nextTurnTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			if ( GetEnemy() && !CheckFOV( GetEnemy()->GetEyePosition(), 270.0f ) )
			{//enemy behind me
				if ( actionGrenadeAttack.timer.IsDone(gameLocal.GetTime()) )
				{//timer okay
					//toss some nades at him
					PlayEffect( "fx_grenade_muzzleflash", animator.GetJointHandle("r_side_can_tip") );
					Attack( "grenade", animator.GetJointHandle("r_side_can_tip"), enemy.ent );
					PlayEffect( "fx_grenade_muzzleflash", animator.GetJointHandle("l_side_can_tip") );
					Attack( "grenade", animator.GetJointHandle("l_side_can_base"), enemy.ent );
					PlayEffect( "fx_grenade_muzzleflash", animator.GetJointHandle("back_can_tip") );
					Attack( "grenade", animator.GetJointHandle("back_can_base"), enemy.ent );
					actionGrenadeAttack.timer.Clear( gameLocal.GetTime() + gameLocal.random.RandomInt(1000)+500 );
				}
			}

			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterHarvester::State_Torso_SprayAttack
================
*/
stateResult_t rvMonsterHarvester::State_Torso_SprayAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_START,
		STAGE_SWEEP,
		STAGE_END,
		STAGE_FINISH
	};
	switch ( parms.stage ) {
		case STAGE_START:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			sweepCount = 0;
			PlayAnim ( ANIMCHANNEL_TORSO, "fire_forward_spray_start", 0 );
			return SRESULT_STAGE ( STAGE_SWEEP );
		
		case STAGE_SWEEP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				sweepCount++;
				PlayAnim ( ANIMCHANNEL_TORSO, "fire_forward_spray_loop", 0 );
				return SRESULT_STAGE ( STAGE_END );
			}
			return SRESULT_WAIT;	

		case STAGE_END:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( enemy.fl.inFov && sweepCount < 3 && !gameLocal.random.RandomInt(2) )
				{
					return SRESULT_STAGE ( STAGE_SWEEP );
				}
				else
				{
					PlayAnim ( ANIMCHANNEL_TORSO, "fire_forward_spray_end", 0 );
					return SRESULT_STAGE ( STAGE_FINISH );
				}
			}
			return SRESULT_WAIT;	

		case STAGE_FINISH:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterHarvester::State_Torso_RocketAttack
================
*/
stateResult_t rvMonsterHarvester::State_Torso_RocketAttack ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_START_WAIT,
		STAGE_FIRE,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( animPrefix.Length() )
			{//don't play an anim
				return SRESULT_STAGE ( STAGE_FIRE );
			}
			PlayAnim ( ANIMCHANNEL_TORSO, "missile_fire_start", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_START_WAIT );
			
		case STAGE_START_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) || idStr::Icmp( "missile_fire_start", animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName() ) ) {
				return SRESULT_STAGE ( STAGE_FIRE );
			}
			return SRESULT_WAIT;

		case STAGE_FIRE:
			if ( partHealth[PART_TANK_L] > 0 )
			{
				PlayEffect( "fx_rocket_muzzleflash", animator.GetJointHandle("l_hopper_muzzle_flash") );
				Attack( "rocket", animator.GetJointHandle("l_hopper_muzzle_flash"), enemy.ent );
			}
			if ( partHealth[PART_TANK_R] > 0 )
			{
				PlayEffect( "fx_rocket_muzzleflash", animator.GetJointHandle("r_hopper_muzzle_flash") );
				Attack( "rocket", animator.GetJointHandle("r_hopper_muzzle_flash"), enemy.ent );
			}

			if ( animPrefix.Length() )
			{//don't play an anim
				return SRESULT_DONE;
			}

			PlayAnim ( ANIMCHANNEL_TORSO, "attack_rocket", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) || idStr::Icmp( "attack_rocket", animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName() ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}
