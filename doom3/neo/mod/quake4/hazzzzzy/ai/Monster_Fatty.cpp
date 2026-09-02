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

typedef struct monsterFattyChain_s {
	jointHandle_t			orientationJoint;
	jointHandle_t			attackJoint;
	bool					out;
	idEntityPtr<idEntity>	projectile;
} monsterFattyChain_t;

class rvMonsterFatty : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterFatty );

	rvMonsterFatty ( void ) {}
	~rvMonsterFatty ( void );
	
	void				InitSpawnArgsVariables			( void );
	void				Spawn							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	virtual bool		UpdateAnimationControllers		( void );
	virtual bool		CanPlayImpactEffect				( idEntity* attacker, idEntity* target ) { return false; };
	virtual void		AddDamageEffect					( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );

protected:

	rvAIAction			actionWhipAttack;

	enum {
		CHAIN_LEFT,
		CHAIN_RIGHT,
		CHAIN_MAX
	};

	monsterFattyChain_t	chains[CHAIN_MAX];

	float				missHeight;		

	virtual bool		CheckActions			( void );

	void				PlayAttackAnim			( const idVec3& target, int blendFrames );
	
	void				ResetAllChains			( void );
	void				ChainIn					( int chain );
	void				ChainOut				( int chain );

private:

	// Custom actions
	bool				CheckAction_WhipAttack	( rvAIAction* action, int animNum );

	// Frame Commands
	stateResult_t		Frame_LeftChainOut		( const stateParms_t& parms );
	stateResult_t		Frame_LeftChainIn		( const stateParms_t& parms );
	
	stateResult_t		Frame_RightChainOut		( const stateParms_t& parms );
	stateResult_t		Frame_RightChainIn		( const stateParms_t& parms );

	// Torso States
	stateResult_t		State_Torso_WhipAttack	( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterFatty );
};

CLASS_DECLARATION( idAI, rvMonsterFatty )
END_CLASS

/*
================
rvMonsterFatty::~rvMonsterFatty
================
*/
rvMonsterFatty::~rvMonsterFatty ( void ) {
	ResetAllChains ( );
}

void rvMonsterFatty::InitSpawnArgsVariables ( void )
{
	chains[CHAIN_LEFT].orientationJoint		= animator.GetJointHandle ( spawnArgs.GetString ( "joint_leftChain", "chainb1" ) );
	chains[CHAIN_LEFT].attackJoint			= animator.GetJointHandle ( spawnArgs.GetString ( "joint_leftChainAttack", "hookb" ) );

	chains[CHAIN_RIGHT].orientationJoint	= animator.GetJointHandle ( spawnArgs.GetString ( "joint_rightChain", "chaina1" ) );
	chains[CHAIN_RIGHT].attackJoint			= animator.GetJointHandle ( spawnArgs.GetString ( "joint_rightChainAttack", "hooka" ) );
		
	missHeight = spawnArgs.GetFloat ( "missHeight", "72" );	
}
/*
================
rvMonsterFatty::Spawn
================
*/
void rvMonsterFatty::Spawn ( void ) {
	// Custom actions
	actionWhipAttack.Init ( spawnArgs, "action_whipAttack", "Torso_WhipAttack", AIACTIONF_ATTACK );

	// Cache joints	
	chains[CHAIN_LEFT].out					= false;
	chains[CHAIN_LEFT].projectile			= NULL;

	chains[CHAIN_RIGHT].out					= false;
	chains[CHAIN_RIGHT].projectile			= NULL;

	InitSpawnArgsVariables();
}

/*
================
rvMonsterFatty::Save
================
*/
void rvMonsterFatty::Save ( idSaveGame *savefile ) const {
	int i;
	
	actionWhipAttack.Save( savefile );

	for ( i = 0; i < CHAIN_MAX; i ++ ) {
		savefile->WriteBool( chains[i].out );
		chains[i].projectile.Save ( savefile );		
	}
}

/*
================
rvMonsterFatty::Restore
================
*/
void rvMonsterFatty::Restore ( idRestoreGame *savefile ) {
	int i;
	
	actionWhipAttack.Restore( savefile );

	for ( i = 0; i < CHAIN_MAX; i ++ ) {
		savefile->ReadBool( chains[i].out );
		chains[i].projectile.Restore ( savefile );		
	}
	
	InitSpawnArgsVariables();
}

/*
================
rvMonsterFatty::UpdateAnimationControllers
================
*/
bool rvMonsterFatty::UpdateAnimationControllers ( void ) {
	if ( !idAI::UpdateAnimationControllers ( ) ) {
		return false;
	}

	if ( enemy.ent && CheckFOV ( enemy.lastKnownPosition ) ) {
		int		i;
		idVec3	origin;
		idMat3	axis;
		idVec3	dir;
		idVec3	localDir;
		idVec3	target;

		if ( enemy.ent->IsType ( idActor::GetClassType ( ) ) ) {
			target = static_cast<idActor*>(enemy.ent.GetEntity())->GetEyePosition ( );
		} else {
			target = enemy.ent->GetPhysics()->GetOrigin ( );
		}

		if ( !IsEnemyVisible ( ) ) {
			target -= enemy.ent->GetPhysics()->GetGravityNormal() * missHeight;
		}

		for ( i = 0; i < CHAIN_MAX; i ++ ) {
			if ( !chains[i].out ) {
				continue;
			}
			
			animator.ClearJoint ( chains[i].orientationJoint );
			GetJointWorldTransform ( chains[i].orientationJoint, gameLocal.time, origin, axis );
			dir = target - origin;
			dir.Normalize ( );	
			axis.ProjectVector ( dir, localDir );
			animator.SetJointAxis ( chains[i].orientationJoint, JOINTMOD_LOCAL, localDir.ToMat3() );
		}
	}

	return true;
}

/*
================
rvMonsterFatty::AddDamageEffect
================
*/
void rvMonsterFatty::AddDamageEffect ( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	// If there are still shields remaining then play a shield effect at the impact point
	/*
	idVec3 dir;
	dir = collision.c.point - GetPhysics()->GetCenterMass ();
	PlayEffect ( "fx_shield", collision.c.point, dir.ToMat3(), false, vec3_origin, true );
	*/
}

/*
================
rvMonsterFatty::CheckAction_WhipAttack
================
*/
bool rvMonsterFatty::CheckAction_WhipAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterFatty::CheckActions
================
*/
bool rvMonsterFatty::CheckActions ( void ) {
	if ( PerformAction ( &actionWhipAttack, (checkAction_t)&rvMonsterFatty::CheckAction_WhipAttack, NULL ) ) {
		return true;
	}
	return idAI::CheckActions ( );
}

/*
================
rvMonsterFatty::PlayAttackAnim
================
*/
void rvMonsterFatty::PlayAttackAnim ( const idVec3& target, int blendFrames ) {
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
	
	if ( yaw < -45.0f ) {
		animName = "attack4b";
	} else if ( yaw < -20.0f ) {
		animName = "attack3b";
	} else if ( yaw < -5.0f ) {
		animName = "attack5r";
	} else if ( yaw < 5.0f ) {
		animName = "attack5";
	} else if ( yaw < 20.0f ) {
		animName = "attack5l";
	} else if ( yaw < 45.0f ) {
		animName = "attack2b";
	} else{
		animName = "attack1b";
	}
	
	PlayAnim ( ANIMCHANNEL_TORSO, animName, blendFrames );
}

/*
================
rvMonsterFatty::ChainOut
================
*/
void rvMonsterFatty::ChainOut ( int chain ) {
	idEntity*		ent;
	idProjectile*	proj;
	
	gameLocal.SpawnEntityDef( *gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_attack_hook" ) ), &ent, false );
	proj = dynamic_cast<idProjectile*>(ent);
	if ( !proj ) {
		delete ent;
		return;
	}

	chains[chain].out = true;
				
	proj->Create ( this, vec3_origin, idVec3(0,0,1) );
	proj->Launch ( vec3_origin, idVec3(0,0,1), vec3_origin );

	chains[chain].projectile = proj;				
	ent->BindToJoint ( this, chains[chain].attackJoint, false );
	ent->SetOrigin ( vec3_origin );
	ent->SetAxis ( mat3_identity );	
}

/*
================
rvMonsterFatty::ChainIn
================
*/
void rvMonsterFatty::ChainIn ( int chain ) {
	chains[chain].out = false;
	if ( chains[chain].projectile ) {
		delete chains[chain].projectile;
		chains[chain].projectile = NULL;
	}
}	

/*
================
rvMonsterFatty::ResetChains
================
*/
void rvMonsterFatty::ResetAllChains ( void ) {
	int i;
	
	animator.ClearAllJoints ( );
	
	for ( i = 0; i < CHAIN_MAX; i ++ ) {
		ChainIn ( i );
	}
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterFatty )
	STATE ( "Torso_WhipAttack",		rvMonsterFatty::State_Torso_WhipAttack )

	STATE ( "Frame_LeftChainOut",		rvMonsterFatty::Frame_LeftChainOut )
	STATE ( "Frame_LeftChainIn",		rvMonsterFatty::Frame_LeftChainIn )

	STATE ( "Frame_RightChainOut",		rvMonsterFatty::Frame_RightChainOut )
	STATE ( "Frame_RightChainIn",		rvMonsterFatty::Frame_RightChainIn )
END_CLASS_STATES


/*
================
rvMonsterFatty::State_Torso_WhipAttack
================
*/
stateResult_t rvMonsterFatty::State_Torso_WhipAttack ( const stateParms_t& parms ) {
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
			PlayAttackAnim ( enemy.ent->GetEyePosition(), parms.blendFrames );

			return SRESULT_STAGE ( STAGE_ATTACK_WAIT );
		}
			
		case STAGE_ATTACK_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				ResetAllChains ( );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterFatty::Frame_LeftChainOut
================
*/
stateResult_t rvMonsterFatty::Frame_LeftChainOut ( const stateParms_t& parms ) {
	ChainOut ( CHAIN_LEFT );
	return SRESULT_OK;
}

/*
================
rvMonsterFatty::Frame_LeftChainIn
================
*/
stateResult_t rvMonsterFatty::Frame_LeftChainIn ( const stateParms_t& parms ) {
	ChainIn ( CHAIN_LEFT );
	return SRESULT_OK;
}

/*
================
rvMonsterFatty::Frame_RightChainOut
================
*/
stateResult_t rvMonsterFatty::Frame_RightChainOut ( const stateParms_t& parms ) {
	ChainOut ( CHAIN_RIGHT );
	return SRESULT_OK;
}

/*
================
rvMonsterFatty::Frame_RightChainIn
================
*/
stateResult_t rvMonsterFatty::Frame_RightChainIn ( const stateParms_t& parms ) {
	ChainIn ( CHAIN_RIGHT );
	return SRESULT_OK;
}
