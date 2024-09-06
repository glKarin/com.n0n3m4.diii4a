#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

class rvWeaponGauntlet : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponGauntlet );

						rvWeaponGauntlet( void );
						~rvWeaponGauntlet( void );

	virtual void		Spawn				( void );
	virtual void		CleanupWeapon		( void );
	void				Save				( idSaveGame *savefile ) const;
	void				Restore				( idRestoreGame *savefile );
	void				PreSave				( void );
	void				PostSave			( void );

protected:

	idAngles			bladeSpinFast;
	idAngles			bladeSpinSlow;
	jointHandle_t		bladeJoint;
	jointHandle_t		bladeJoint_world;
	int					bladeAccel;
	
	float				range;
	
	rvClientEffectPtr	impactEffect;
	int					impactMaterial;

	void				Attack				( void );
	void				StartBlade			( void );
	void				StopBlade			( void );

private:

	void				PlayLoopSound		( int sndType );
	int					loopSound;
	enum {
		LOOP_NONE,
		LOOP_WALL,
		LOOP_FLESH
	};

	stateResult_t		State_Raise			( const stateParms_t& parms );
	stateResult_t		State_Lower			( const stateParms_t& parms );
	stateResult_t		State_Idle			( const stateParms_t& parms );
	stateResult_t		State_Fire			( const stateParms_t& parms );
	
	CLASS_STATES_PROTOTYPE ( rvWeaponGauntlet );
};

CLASS_DECLARATION( rvWeapon, rvWeaponGauntlet )
END_CLASS

/*
================
rvWeaponGauntlet::rvWeaponGauntlet
================
*/
rvWeaponGauntlet::rvWeaponGauntlet( void ) {
	loopSound = LOOP_NONE;
}

/*
================
rvWeaponGauntlet::~rvWeaponGauntlet
================
*/
rvWeaponGauntlet::~rvWeaponGauntlet(void)
{
	if ( viewModel ) {
		StopSound( SND_CHANNEL_WEAPON, false );
	}
	if ( impactEffect ) {
		impactEffect->Stop( );
		impactEffect = NULL;
	}
	impactMaterial = -1;

}
/*
================
rvWeaponGauntlet::Spawn
================
*/
void rvWeaponGauntlet::Spawn ( void ) {
	SetState ( "Raise", 0 );
	
	bladeJoint		= viewModel->GetAnimator()->GetJointHandle ( spawnArgs.GetString ( "joint_blade", "center" ) );
	bladeJoint_world= GetWorldModel()->GetAnimator()->GetJointHandle ( spawnArgs.GetString ( "joint_blade", "center" ) );

	bladeSpinFast	= spawnArgs.GetAngles ( "blade_spinfast" );
	bladeSpinSlow	= spawnArgs.GetAngles ( "blade_spinslow" );
	bladeAccel		= SEC2MS ( spawnArgs.GetFloat ( "blade_accel", ".25" ) );
	
	range			= spawnArgs.GetFloat ( "range", "32" );

	impactMaterial = -1;
	impactEffect   = NULL;
	loopSound = LOOP_NONE;
}

/*
================
rvWeaponGauntlet::Save
================
*/
void rvWeaponGauntlet::Save ( idSaveGame *savefile ) const {
	savefile->WriteAngles ( bladeSpinFast );
	savefile->WriteAngles ( bladeSpinSlow );
	savefile->WriteJoint ( bladeJoint );
	savefile->WriteJoint ( bladeJoint_world );

	savefile->WriteInt ( bladeAccel );
	
	savefile->WriteFloat ( range );
	
	savefile->WriteObject ( impactEffect );
	savefile->WriteInt ( impactMaterial );
	savefile->WriteInt ( loopSound );
}

/*
================
rvWeaponGauntlet::Restore
================
*/
void rvWeaponGauntlet::Restore ( idRestoreGame *savefile ) {
	savefile->ReadAngles ( bladeSpinFast );
	savefile->ReadAngles ( bladeSpinSlow );
	savefile->ReadJoint ( bladeJoint );
	savefile->ReadJoint ( bladeJoint_world );

	savefile->ReadInt ( bladeAccel );
	
	savefile->ReadFloat ( range );
	
	savefile->ReadObject ( reinterpret_cast<idClass*&>( impactEffect ) );
	savefile->ReadInt ( impactMaterial );
	savefile->ReadInt ( loopSound );
}

/*
================
rvWeaponGauntlet::PreSave
================
*/
void rvWeaponGauntlet::PreSave ( void ) {

}

/*
================
rvWeaponGauntlet::PostSave
================
*/
void rvWeaponGauntlet::PostSave( void ) {
}

void rvWeaponGauntlet::PlayLoopSound( int sndType ) {
	if ( loopSound == sndType ) {
		return;
	}
	const char *loopSoundString = NULL;
	switch ( sndType ) {
		case LOOP_NONE:
		default:
			loopSoundString = "snd_spin_loop";
			break;
		case LOOP_WALL:
			loopSoundString = "snd_spin_wall";
			break;
		case LOOP_FLESH:
			loopSoundString = "snd_spin_flesh";
			break;
	}
	if ( loopSoundString ) {
		loopSound = sndType;
		StartSound( loopSoundString, SND_CHANNEL_WEAPON, 0, false, 0 );
	}
}

/*
================
rvWeaponGauntlet::CleanupWeapon
================
*/
void rvWeaponGauntlet::CleanupWeapon( void ) {

	if ( impactEffect ) {
		impactEffect->Stop( );
		impactEffect = NULL;
	}
	impactMaterial = -1;
	PlayLoopSound( LOOP_NONE );
}

/*
================
rvWeaponGauntlet::Attack
================
*/
void rvWeaponGauntlet::Attack ( void ) {
	trace_t		tr;
	idEntity*	ent;
	
	// Cast a ray out to the lock range
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	gameLocal.TracePoint(	owner, tr, 
							playerViewOrigin, 
							playerViewOrigin + playerViewAxis[0] * range, 
							MASK_SHOT_RENDERMODEL, owner );
// RAVEN END
	owner->WeaponFireFeedback( &weaponDef->dict );

	if ( tr.fraction >= 1.0f ) {
		if ( impactEffect ) {
			impactEffect->Stop ( );
			impactEffect = NULL;
		}
		impactMaterial = -1;
		PlayLoopSound( LOOP_NONE );
 		return;
	}
		
	// Entity we hit?
	ent = gameLocal.entities[tr.c.entityNum];

	// If the impact material changed then stop the impact effect 
	if ( (tr.c.materialType && tr.c.materialType->Index ( ) != impactMaterial) ||
		 (!tr.c.materialType && impactMaterial != -1) ) {
		if ( impactEffect ) {
			impactEffect->Stop ( );
			impactEffect = NULL;
		}
		impactMaterial = -1;
	}
	
	// In singleplayer-- the gauntlet never effects marine AI
	if( !gameLocal.isMultiplayer ) {
		idActor* actor_ent = 0;
		
		//ignore both the body and the head.
		if (ent->IsType( idActor::GetClassType()) )	{
			actor_ent = static_cast<idActor*>(ent);
		} else if (ent->IsType ( idAFAttachment::GetClassType()) )	{
			actor_ent = static_cast<idActor*>(ent->GetBindMaster());
		}
			
		if ( actor_ent && actor_ent->team == gameLocal.GetLocalPlayer()->team )	{
			PlayLoopSound( LOOP_NONE );
			return;
		}
	}

	//multiplayer-- don't gauntlet dead stuff
	if( gameLocal.isMultiplayer )	{
		idPlayer * player;
		if ( ent->IsType( idPlayer::GetClassType() )) {
			player = static_cast< idPlayer* >(ent);
			if (player->health <= 0)	{
				return;
			}
		}

	}
	
	if ( !impactEffect ) {
		impactMaterial = tr.c.materialType ? tr.c.materialType->Index() : -1;
		impactEffect = gameLocal.PlayEffect ( gameLocal.GetEffect ( spawnArgs, "fx_impact", tr.c.materialType ), tr.endpos, tr.c.normal.ToMat3(), true );
	} else {
		impactEffect->SetOrigin ( tr.endpos );
		impactEffect->SetAxis ( tr.c.normal.ToMat3() );
	}
	
	// Do damage?
	if ( gameLocal.time > nextAttackTime ) {					
		if ( ent ) {
			if ( ent->fl.takedamage ) {
				float dmgScale = 1.0f;
				dmgScale *= owner->PowerUpModifier( PMOD_MELEE_DAMAGE );
				ent->Damage ( owner, owner, playerViewAxis[0], spawnArgs.GetString ( "def_damage" ), dmgScale, 0 );
				StartSound( "snd_hit", SND_CHANNEL_ANY, 0, false, NULL );
				if ( ent->spawnArgs.GetBool( "bleed" ) ) {
					PlayLoopSound( LOOP_FLESH );
				} else {
					PlayLoopSound( LOOP_WALL );
				}
			} else {
				PlayLoopSound( LOOP_WALL );
			}
		} else {
			PlayLoopSound( LOOP_NONE );
		}
		nextAttackTime = gameLocal.time + fireRate;
	}
}

/*
================
rvWeaponGauntlet::StartBlade
================
*/
void rvWeaponGauntlet::StartBlade ( void ) {
	if ( viewModel ) {
		viewModel->GetAnimator()->SetJointAngularVelocity ( bladeJoint, bladeSpinFast, gameLocal.time, bladeAccel ); 
	}
	
	if ( GetWorldModel() ) {	
		GetWorldModel()->GetAnimator()->SetJointAngularVelocity ( bladeJoint_world, bladeSpinFast, gameLocal.time, bladeAccel ); 
	}
	
	StopSound ( SND_CHANNEL_ITEM, false );
//	StartSound ( "snd_blade_fast", SND_CHANNEL_ITEM, 0, false, NULL );
	StartSound( "snd_spin_up", SND_CHANNEL_ITEM, 0, false, 0 );
}

/*
================
rvWeaponGauntlet::StopBlade
================
*/
void rvWeaponGauntlet::StopBlade ( void ) {
	if ( viewModel ) {
		viewModel->GetAnimator()->SetJointAngularVelocity ( bladeJoint, bladeSpinSlow, gameLocal.time, bladeAccel ); 
	}
	
	if ( GetWorldModel() ) {
		GetWorldModel()->GetAnimator()->SetJointAngularVelocity ( bladeJoint_world, bladeSpinSlow, gameLocal.time, bladeAccel ); 
	}
	
	StopSound ( SND_CHANNEL_WEAPON, false );
//	StartSound ( "snd_blade_slow", SND_CHANNEL_ITEM, 0, false, NULL );
	
	if ( impactEffect ) {
		impactEffect->Stop ( );
		impactEffect = NULL;
	}
	impactMaterial = -1;
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponGauntlet )
	STATE ( "Raise",		rvWeaponGauntlet::State_Raise )
	STATE ( "Lower",		rvWeaponGauntlet::State_Lower )
	STATE ( "Idle",			rvWeaponGauntlet::State_Idle)
	STATE ( "Fire",			rvWeaponGauntlet::State_Fire )
END_CLASS_STATES

/*
================
rvWeaponGauntlet::State_Raise
================
*/
stateResult_t rvWeaponGauntlet::State_Raise( const stateParms_t& parms ) {
	enum {
		RAISE_INIT,
		RAISE_WAIT,
	};	
	switch ( parms.stage ) {
		case RAISE_INIT:			
			SetStatus ( WP_RISING );
			PlayAnim( ANIMCHANNEL_ALL, "raise", parms.blendFrames );
			return SRESULT_STAGE(RAISE_WAIT);
			
		case RAISE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;	
}

/*
================
rvWeaponGauntlet::State_Lower
================
*/
stateResult_t rvWeaponGauntlet::State_Lower ( const stateParms_t& parms ) {
	enum {
		LOWER_INIT,
		LOWER_WAIT,
		LOWER_WAITRAISE
	};	
	switch ( parms.stage ) {
		case LOWER_INIT:
			SetStatus ( WP_LOWERING );
			PlayAnim( ANIMCHANNEL_ALL, "lower", parms.blendFrames );
			return SRESULT_STAGE(LOWER_WAIT);
			
		case LOWER_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetStatus ( WP_HOLSTERED );
				return SRESULT_STAGE(LOWER_WAITRAISE);
			}
			return SRESULT_WAIT;
	
		case LOWER_WAITRAISE:
			if ( wsfl.raiseWeapon ) {
				SetState ( "Raise", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponGauntlet::State_Idle
================
*/
stateResult_t rvWeaponGauntlet::State_Idle ( const stateParms_t& parms ) {	
	enum {
		IDLE_INIT,
		IDLE_WAIT,
	};	
	switch ( parms.stage ) {
		case IDLE_INIT:			
			SetStatus ( WP_READY );
			StopBlade ( );
			PlayCycle( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
			return SRESULT_STAGE ( IDLE_WAIT );
			
		case IDLE_WAIT:
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.attack ) {
				SetState ( "Fire", 2 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponGauntlet::State_Fire
================
*/
stateResult_t rvWeaponGauntlet::State_Fire ( const stateParms_t& parms ) {
	enum {
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_LOOP,
		STAGE_LOOP_WAIT,
		STAGE_END,
		STAGE_END_WAIT
	};	
	switch ( parms.stage ) {
		case STAGE_START:	
			PlayAnim ( ANIMCHANNEL_ALL, "attack_start", parms.blendFrames );
			StartBlade ( );
			loopSound = LOOP_NONE;
			return SRESULT_STAGE(STAGE_START_WAIT);
		
		case STAGE_START_WAIT:
			if ( !wsfl.attack ) {
				return SRESULT_STAGE ( STAGE_END );
			}
			if ( AnimDone ( ANIMCHANNEL_ALL, parms.blendFrames ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
			
		case STAGE_LOOP:
			PlayCycle ( ANIMCHANNEL_ALL, "attack_loop", parms.blendFrames );
			StartSound( "snd_spin_loop", SND_CHANNEL_WEAPON, 0, false, 0 );
			return SRESULT_STAGE(STAGE_LOOP_WAIT);
			
		case STAGE_LOOP_WAIT:
			if ( !wsfl.attack || wsfl.lowerWeapon ) {
				return SRESULT_STAGE ( STAGE_END );
			}
			Attack ( );
			return SRESULT_WAIT;
		
		case STAGE_END:
			PlayAnim ( ANIMCHANNEL_ALL, "attack_end", parms.blendFrames );
			StopBlade ( );
			StartSound( "snd_spin_down", SND_CHANNEL_WEAPON, 0, false, 0 );
			return SRESULT_STAGE ( STAGE_END_WAIT );
		
		case STAGE_END_WAIT:
			if ( wsfl.attack || AnimDone ( ANIMCHANNEL_ALL, parms.blendFrames ) ) {
				PostState ( "Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}			
	return SRESULT_ERROR;
}
