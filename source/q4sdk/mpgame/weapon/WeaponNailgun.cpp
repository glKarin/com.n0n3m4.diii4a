#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"
#include "../client/ClientEffect.h"
#include "../Projectile.h"

// Available drum speeds
const int NAILGUN_DRUMSPEED_STOPPED		= 0;
const int NAILGUN_DRUMSPEED_SLOW		= 1;
const int NAILGUN_DRUMSPEED_FAST		= 2;

// Spinup and spindown times
const int NAILGUN_SPINDOWN_TIME			= 1000;
const int NAILGUN_SPINUP_TIME			= 1000;

// Nailgun shader parms
const int NAILGUN_SPARM_PLAYLEADIN		= 7;

// Nailgun weapon modifications
const int NAILGUN_MOD_ROF_AMMO			= BIT(0);	// power mod combines Rate of fire and ammoo
const int NAILGUN_MOD_SEEK				= BIT(1);
//const int NAILGUN_MOD_AMMO				= BIT(2);

const int NAILGUN_SPIN_SNDCHANNEL		= SND_CHANNEL_BODY2;

class rvWeaponNailgun : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponNailgun );

	rvWeaponNailgun ( void );
	~rvWeaponNailgun ( void );

	virtual void		Spawn					( void );
	virtual void		Think					( void );
	void				Save				( idSaveGame *savefile ) const;
	void				Restore				( idRestoreGame *savefile );
	void				PreSave					( void );
	void				PostSave				( void );

protected:

	idEntityPtr<idEntity>				guideEnt;
	int									guideTime;
	int									guideStartTime;
	rvClientEntityPtr<rvClientEffect>	guideEffect;
	bool								guideLocked;
	float								guideRange;
	int									guideHoldTime;
	int									guideAquireTime;

	jointHandle_t						jointDrumView;
	jointHandle_t						jointPinsView;
	jointHandle_t						jointSteamRightView;
	jointHandle_t						jointSteamLeftView;

	jointHandle_t						jointGuideEnt;
	
	int									drumSpeed;
	int									drumSpeedIdeal;
	float								drumMultiplier;

	virtual void		OnLaunchProjectile		( idProjectile* proj );
	
private:

	void				UpdateGuideStatus		( float range = 0.0f );
	void				CancelGuide				( void );	
	bool				DrumSpin				( int speed, int blendFrames );

	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_Fire				( const stateParms_t& parms );
	stateResult_t		State_Reload			( const stateParms_t& parms );
	stateResult_t		State_Raise				( const stateParms_t& parms );
	stateResult_t		State_Lower				( const stateParms_t& parms );
	
	stateResult_t		State_DrumSpinDown		( const stateParms_t& parms );
	stateResult_t		State_DrumSpinUp		( const stateParms_t& parms );

	stateResult_t		Frame_ClaspOpen			( const stateParms_t& parms );
	stateResult_t		Frame_ClaspClose		( const stateParms_t& parms );	
	
	CLASS_STATES_PROTOTYPE ( rvWeaponNailgun );
};

CLASS_DECLARATION( rvWeapon, rvWeaponNailgun )
END_CLASS

/*
================
rvWeaponNailgun::rvWeaponNailgun
================
*/
rvWeaponNailgun::rvWeaponNailgun ( void ) {
}

/*
================
rvWeaponNailgun::~rvWeaponNailgun
================
*/
rvWeaponNailgun::~rvWeaponNailgun ( void ) {
	if ( guideEffect ) {
		guideEffect->Stop();
	}
	if( viewModel ) {
		viewModel->StopSound( NAILGUN_SPIN_SNDCHANNEL, false );
	}
}


/*
================
rvWeaponNailgun::Spawn
================
*/
void rvWeaponNailgun::Spawn ( void ) {
	spawnArgs.GetFloat ( "lockRange", "1000", guideRange );
	guideHoldTime = SEC2MS ( spawnArgs.GetFloat ( "lockHoldTime", "10" ) );
	guideAquireTime = SEC2MS ( spawnArgs.GetFloat ( "lockAquireTime", ".1" ) );
	
	jointDrumView		= viewAnimator->GetJointHandle ( spawnArgs.GetString ( "joint_view_drum" ) );
	jointPinsView		= viewAnimator->GetJointHandle ( spawnArgs.GetString ( "joint_view_pins" ) );
	jointSteamRightView = viewAnimator->GetJointHandle ( spawnArgs.GetString ( "joint_view_steamRight" ) );
	jointSteamLeftView  = viewAnimator->GetJointHandle ( spawnArgs.GetString ( "joint_view_steamLeft" ) );

	jointGuideEnt		= INVALID_JOINT;
	
	drumSpeed		= NAILGUN_DRUMSPEED_STOPPED;
	drumSpeedIdeal	= drumSpeed;
	drumMultiplier	= spawnArgs.GetFloat ( "drumSpeed" );
	
	ExecuteState ( "ClaspClose" );	
	SetState ( "Raise", 0 );	
}

/*
================
rvWeaponNailgun::Save
================
*/
void rvWeaponNailgun::Save ( idSaveGame *savefile ) const {
	guideEnt.Save( savefile );
	savefile->WriteInt( guideTime );
	savefile->WriteInt( guideStartTime );
	guideEffect.Save( savefile );
	savefile->WriteBool ( guideLocked );
	savefile->WriteFloat ( guideRange );
	savefile->WriteInt ( guideHoldTime );
	savefile->WriteInt ( guideAquireTime );

	savefile->WriteJoint ( jointDrumView );
	savefile->WriteJoint ( jointPinsView );
	savefile->WriteJoint ( jointSteamRightView );
	savefile->WriteJoint ( jointSteamLeftView );
	savefile->WriteJoint ( jointGuideEnt );
	
	savefile->WriteInt ( drumSpeed );
	savefile->WriteInt ( drumSpeedIdeal );
	savefile->WriteFloat ( drumMultiplier );
}

/*
================
rvWeaponNailgun::Restore
================
*/
void rvWeaponNailgun::Restore ( idRestoreGame *savefile ) {
	guideEnt.Restore( savefile );
	savefile->ReadInt( guideTime );
	savefile->ReadInt( guideStartTime );
	guideEffect.Restore( savefile );
	savefile->ReadBool ( guideLocked );
	savefile->ReadFloat ( guideRange );
	savefile->ReadInt ( guideHoldTime );
	savefile->ReadInt ( guideAquireTime );

	savefile->ReadJoint ( jointDrumView );
	savefile->ReadJoint ( jointPinsView );
	savefile->ReadJoint ( jointSteamRightView );
	savefile->ReadJoint ( jointSteamLeftView );
	savefile->ReadJoint ( jointGuideEnt );

	
	savefile->ReadInt ( drumSpeed );
	savefile->ReadInt ( drumSpeedIdeal );
	savefile->ReadFloat ( drumMultiplier );
}

/*
================
rvWeaponNailgun::PreSave
================
*/
void rvWeaponNailgun::PreSave ( void ) {

	//disable sounds
	StopSound( SND_CHANNEL_ANY, false);

	//remove the guide gui cursor if there is one.
	if ( guideEffect ) {
		guideEffect->Stop();
		guideEffect->Event_Remove();
		guideEffect = NULL;
	}
}

/*
================
rvWeaponNailgun::PostSave
================
*/
void rvWeaponNailgun::PostSave ( void ) {

	//restore sounds-- but which one?
	if( drumSpeed == NAILGUN_DRUMSPEED_FAST )	{
		viewModel->StartSound ( "snd_spinfast", NAILGUN_SPIN_SNDCHANNEL, 0, false, NULL );		
	} else 	if( drumSpeed == NAILGUN_DRUMSPEED_SLOW )	{
		viewModel->StartSound ( "snd_spinslow", NAILGUN_SPIN_SNDCHANNEL, 0, false, NULL );		
	}

	//the guide gui effect will restore itself naturally
}

/*
================
rvWeaponNailgun::CancelGuide
================
*/
void rvWeaponNailgun::CancelGuide ( void ) {
	if ( zoomGui && guideEnt ) {
		zoomGui->HandleNamedEvent ( "lockStop" );
	}

	guideEnt = NULL;
	guideLocked = false;
	jointGuideEnt = INVALID_JOINT;
	UpdateGuideStatus ( );	
}

/*
================
rvWeaponNailgun::UpdateGuideStatus
================
*/
void rvWeaponNailgun::UpdateGuideStatus ( float range ) {
	// Update the zoom GUI variables
	if ( zoomGui ) {
		float lockStatus;
		if ( guideEnt ) {
			if ( guideLocked ) {
				lockStatus = 1.0f;
			} else {
				lockStatus = Min((float)(gameLocal.time - guideStartTime)/(float)guideTime, 1.0f);
			}
		} else {
			lockStatus = 0.0f;
		}
		
		if ( owner == gameLocal.GetLocalPlayer( ) ) {
			zoomGui->SetStateFloat ( "lockStatus", lockStatus );		
			zoomGui->SetStateFloat ( "playerYaw", playerViewAxis.ToAngles().yaw );
		}		

		if ( guideEnt ) {		
			idVec3 diff;
			diff = guideEnt->GetPhysics()->GetOrigin ( ) - playerViewOrigin;
			diff.NormalizeFast ( );
			zoomGui->SetStateFloat ( "lockYaw", idMath::AngleDelta ( diff.ToAngles ( ).yaw, playerViewAxis.ToAngles().yaw ) );
			zoomGui->SetStateString ( "lockRange", va("%4.2f", range) );
			zoomGui->SetStateString ( "lockTime", va("%02d", (int)MS2SEC(guideStartTime + guideTime - gameLocal.time) + 1) );
		}
	}
	
	// Update the guide effect
	if ( guideEnt ) {
		idVec3 eyePos = static_cast<idActor *>(guideEnt.GetEntity())->GetEyePosition();
		if( jointGuideEnt == INVALID_JOINT )	{
			eyePos += guideEnt->GetPhysics()->GetAbsBounds().GetCenter ( );
		} else {
			idMat3	jointAxis;
			idVec3	jointLoc;
			static_cast<idAnimatedEntity *>(guideEnt.GetEntity())->GetJointWorldTransform( jointGuideEnt, gameLocal.GetTime(),jointLoc,jointAxis );
			eyePos += jointLoc;
		}
		eyePos *= 0.5f;
		if ( guideEffect ) {			
			guideEffect->SetOrigin ( eyePos );
			guideEffect->SetAxis ( playerViewAxis.Transpose() );
		} else {
			guideEffect = gameLocal.PlayEffect ( 
									gameLocal.GetEffect( spawnArgs, guideLocked ? "fx_guide" : "fx_guidestart" ), 
									eyePos, playerViewAxis.Transpose(), true, vec3_origin, false );
			if ( guideEffect ) {
				guideEffect->GetRenderEffect()->weaponDepthHackInViewID = owner->entityNumber + 1;
				guideEffect->GetRenderEffect()->allowSurfaceInViewID = owner->entityNumber + 1;
			}
		}
	} else {
		if ( guideEffect ) {
			guideEffect->Stop();
			guideEffect = NULL;
		}
	}
}

/*
================
rvWeaponNailgun::Think
================
*/
void rvWeaponNailgun::Think ( void ) {
	idEntity* ent;
	trace_t	  tr;

	// Let the real weapon think first
	rvWeapon::Think ( );

	// If no guide range is set then we dont have the mod yet
	if ( !guideRange ) {
		return;
	}

	// If the zoom button isnt down then dont update the lock
	if ( !wsfl.zoom || IsReloading ( ) || IsHolstered ( ) ) {
		CancelGuide ( );
		return;
	}

	// Dont update the target if the current target is still alive, unhidden and we have already locked
	if ( guideEnt && guideEnt->health > 0 && !guideEnt->IsHidden() && guideLocked ) {
		float	range;
		range = (guideEnt->GetPhysics()->GetOrigin() - playerViewOrigin).LengthFast();
		if ( range > guideRange || gameLocal.time > guideStartTime + guideTime ) {
			CancelGuide ( );
		} else {
			UpdateGuideStatus ( range );
			return;
		}
	}

	// Cast a ray out to the lock range
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	gameLocal.TracePoint(	owner, tr, 
							playerViewOrigin, 
							playerViewOrigin + playerViewAxis[0] * guideRange, 
							MASK_SHOT_BOUNDINGBOX, owner );
// RAVEN END
	
	if ( tr.fraction >= 1.0f ) {
		CancelGuide( );
		return;
	}
	
	ent = gameLocal.entities[tr.c.entityNum];
	
	//if we're using a target nailable...
	if( ent->IsType ( rvTarget_Nailable::GetClassType()	 )	)	{
		const char* jointName = ent->spawnArgs.GetString("lock_joint");
		ent = ent->targets[ 0 ].GetEntity();
		if( !ent)	{
			CancelGuide( );
			return;
		}

		if( ent->GetAnimator() )	{
			jointGuideEnt = ent->GetAnimator()->GetJointHandle( jointName );
		}
	}


	if ( !ent->IsType ( idActor::GetClassType() ) ) {
		CancelGuide( );
		return;
	}
	if ( gameLocal.GetLocalPlayer() && static_cast<idActor *>(ent)->team == gameLocal.GetLocalPlayer()->team ) {
		CancelGuide( );
		return;
	}
	
	if ( guideEnt != ent ) {
		guideStartTime = gameLocal.time;		
		guideTime = guideAquireTime;
		guideEnt = ent;
		if ( zoomGui ) {
			zoomGui->HandleNamedEvent ( "lockStart" );
		}
	} else if ( gameLocal.time > guideStartTime + guideTime ) {
		// Stop the guide effect since it was just the guide_Start effect
		if ( guideEffect ) {
			guideEffect->Stop();
			guideEffect = NULL;
		}
		guideLocked = true;
		guideTime = guideHoldTime;
		guideStartTime = gameLocal.time;
		if ( zoomGui ) {
			zoomGui->HandleNamedEvent ( "lockAquired" );
		}
	}
	
	UpdateGuideStatus ( (ent->GetPhysics()->GetOrigin() - playerViewOrigin).LengthFast() );
}

/*
================
rvWeaponNailgun::OnLaunchProjectile
================
*/
void rvWeaponNailgun::OnLaunchProjectile ( idProjectile* proj ) {
	rvWeapon::OnLaunchProjectile(proj);

	idGuidedProjectile* guided;
	guided = dynamic_cast<idGuidedProjectile*>(proj);
	if ( guided ) {
		guided->GuideTo ( guideEnt, jointGuideEnt );
	}
}

/*
================
rvWeaponNailgun::DrumSpin

Set the drum spin speed 
================
*/
bool rvWeaponNailgun::DrumSpin ( int speed, int blendFrames ) {
	// Dont bother if the drum is already spinning at the desired speed
	if ( drumSpeedIdeal == speed ) {
		return false;
	}

	drumSpeedIdeal = speed;

	switch ( speed ) {
		case NAILGUN_DRUMSPEED_STOPPED:
			viewModel->StopSound ( NAILGUN_SPIN_SNDCHANNEL, false );
			viewAnimator->SetJointAngularVelocity ( jointDrumView, idAngles(0,0,0), gameLocal.time, 250 );
			viewAnimator->SetJointAngularVelocity ( jointPinsView, idAngles(0,0,0), gameLocal.time, 250 );
		
			// Spin the barrel down if we were spinning fast
			if ( drumSpeed == NAILGUN_DRUMSPEED_FAST ) {
				PostState ( "DrumSpinDown", blendFrames );
				return true;
			}
			break;
			
		case NAILGUN_DRUMSPEED_SLOW:
			viewAnimator->SetJointAngularVelocity ( jointDrumView, idAngles(0,0,45), gameLocal.time, NAILGUN_SPINDOWN_TIME );
			viewAnimator->SetJointAngularVelocity ( jointPinsView, idAngles(0,0,-35), gameLocal.time, NAILGUN_SPINDOWN_TIME );		
			viewModel->StopSound ( NAILGUN_SPIN_SNDCHANNEL, false );
			viewModel->StartSound ( "snd_spinslow", NAILGUN_SPIN_SNDCHANNEL, 0, false, NULL );		

			// Spin the barrel down if we were spinning fast		
			if ( drumSpeed == NAILGUN_DRUMSPEED_FAST ) {
				PostState ( "DrumSpinDown", blendFrames );
				return true;
			}
			break;
		
		case NAILGUN_DRUMSPEED_FAST:
			// Start the barrel spinning faster
			viewAnimator->SetJointAngularVelocity ( jointDrumView, idAngles(0,0,360.0f * drumMultiplier), gameLocal.time, NAILGUN_SPINUP_TIME );
			viewAnimator->SetJointAngularVelocity ( jointPinsView, idAngles(0,0,-300.0f * drumMultiplier), gameLocal.time, NAILGUN_SPINUP_TIME );

			PostState ( "DrumSpinUp", blendFrames );
			return true;
	}
	
	drumSpeed = drumSpeedIdeal;
	
	return false;
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponNailgun )
	STATE ( "Raise",						rvWeaponNailgun::State_Raise )
	STATE ( "Lower",						rvWeaponNailgun::State_Lower )
	STATE ( "Idle",							rvWeaponNailgun::State_Idle)
	STATE ( "Fire",							rvWeaponNailgun::State_Fire )
	STATE ( "Reload",						rvWeaponNailgun::State_Reload )
	STATE ( "DrumSpinDown",					rvWeaponNailgun::State_DrumSpinDown )
	STATE ( "DrumSpinUp",					rvWeaponNailgun::State_DrumSpinUp )
	
	STATE ( "ClaspOpen",					rvWeaponNailgun::Frame_ClaspOpen )
	STATE ( "ClaspClose",					rvWeaponNailgun::Frame_ClaspClose )
END_CLASS_STATES

/*
================
rvWeaponNailgun::State_Raise

Raise the weapon
================
*/
stateResult_t rvWeaponNailgun::State_Raise ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		// Start the weapon raising
		case STAGE_INIT:
			SetStatus ( WP_RISING );
			PlayAnim( ANIMCHANNEL_LEGS, "raise", 0 );
			DrumSpin ( NAILGUN_DRUMSPEED_SLOW, 0 );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
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
rvWeaponNailgun::State_Lower

Lower the weapon
================
*/
stateResult_t rvWeaponNailgun::State_Lower ( const stateParms_t& parms ) {	
	enum {
		STAGE_INIT,
		STAGE_WAIT,
		STAGE_WAITRAISE
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			// Stop any looping sounds
			viewModel->StopSound ( NAILGUN_SPIN_SNDCHANNEL, false );
			
			SetStatus ( WP_LOWERING );
			PlayAnim ( ANIMCHANNEL_LEGS, "putaway", parms.blendFrames );
			return SRESULT_STAGE(STAGE_WAIT);
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 0 ) ) {
				SetStatus ( WP_HOLSTERED );
				return SRESULT_STAGE(STAGE_WAITRAISE);
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITRAISE:
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
rvWeaponNailgun::State_Idle

Manage the idle state of the weapon
================
*/
stateResult_t rvWeaponNailgun::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !AmmoInClip ( ) ) {
				SetStatus ( WP_OUTOFAMMO );
			} else {
				SetStatus ( WP_READY );
			}			
			// Do we need to spin the drum down?
			if ( DrumSpin ( NAILGUN_DRUMSPEED_SLOW, parms.blendFrames ) ) {
				PostState ( "Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
				
			PlayCycle( ANIMCHANNEL_LEGS, "idle", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
		
			if ( !clipSize ) {
				if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoAvailable ( ) ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}
			} else {
				if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoInClip ( ) ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}  
				if ( wsfl.attack && AutoReload() && !AmmoInClip ( ) && AmmoAvailable () ) {
					SetState ( "Reload", 4 );
					return SRESULT_DONE;			
				}
				if ( wsfl.netReload || (wsfl.reload && AmmoInClip() < ClipSize() && AmmoAvailable()>AmmoInClip()) ) {
					SetState ( "Reload", 4 );
					return SRESULT_DONE;			
				}				
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::State_Fire

Fire the weapon
================
*/
stateResult_t rvWeaponNailgun::State_Fire( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_FIRE,
		STAGE_FIREWAIT,
		STAGE_DONE,
		STAGE_SPINEMPTY,		
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !wsfl.attack ) {
				SetState ( "Idle", parms.blendFrames );				
				return SRESULT_DONE;
			}
			if ( DrumSpin ( NAILGUN_DRUMSPEED_FAST, 2 ) ) {
				PostState ( "Fire", 2 );
				return SRESULT_DONE;
			}
			nextAttackTime = gameLocal.time;
			
			return SRESULT_STAGE ( STAGE_FIRE );
			
		case STAGE_FIRE:
			if ( !wsfl.attack || wsfl.reload || wsfl.lowerWeapon || !AmmoInClip ( ) ) {
				return SRESULT_STAGE ( STAGE_DONE );
			}
			if ( mods & NAILGUN_MOD_ROF_AMMO ) {
				PlayCycle ( ANIMCHANNEL_LEGS, "fire_fast", 4 );
			} else {
				PlayCycle ( ANIMCHANNEL_LEGS, "fire_slow", 4 );
			}

			if ( wsfl.zoom ) {				
				Attack ( true, 1, spread, 0.0f, 1.0f );
				nextAttackTime = gameLocal.time + (altFireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
			} else {
				Attack ( false, 1, spread, 0.0f, 1.0f );
				nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
			}
			
			// Play the exhaust effects
			viewModel->PlayEffect ( "fx_exhaust", jointSteamRightView, false );
			viewModel->PlayEffect ( "fx_exhaust", jointSteamLeftView, false );

			viewModel->StartSound ( "snd_fire", SND_CHANNEL_WEAPON,	0, false, NULL );
			viewModel->StartSound ( "snd_fireStereo", SND_CHANNEL_ITEM, 0, false, NULL ); 
					
			return SRESULT_STAGE ( STAGE_FIREWAIT );

		case STAGE_FIREWAIT:
			if ( !wsfl.attack || wsfl.reload || wsfl.lowerWeapon || !AmmoInClip ( ) ) {
				return SRESULT_STAGE ( STAGE_DONE );
			}
			if ( gameLocal.time >= nextAttackTime ) {
				return SRESULT_STAGE ( STAGE_FIRE );
			}
			return SRESULT_WAIT;
			
		case STAGE_DONE:
			if ( clipSize && wsfl.attack && !wsfl.lowerWeapon && !wsfl.reload ) {
				PlayCycle ( ANIMCHANNEL_LEGS, "spinempty", 4 );
				return SRESULT_STAGE ( STAGE_SPINEMPTY );
			}
			DrumSpin ( NAILGUN_DRUMSPEED_SLOW, 4 );
			if ( !wsfl.attack && !AmmoInClip() && AmmoAvailable() && AutoReload ( ) && !wsfl.lowerWeapon ) {
				PostState ( "Reload", 4 );
			} else {
				PostState ( "Idle", 4 );
			}
			return SRESULT_DONE;
			
		case STAGE_SPINEMPTY:
			if ( !wsfl.attack || wsfl.reload || wsfl.lowerWeapon ) {
				return SRESULT_STAGE ( STAGE_DONE );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::State_Reload
================
*/
stateResult_t rvWeaponNailgun::State_Reload ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_RELOAD,
		STAGE_RELOADWAIT,
		STAGE_RELOADLEFT,
		STAGE_RELOADLEFTWAIT,
		STAGE_RELOADRIGHT,
		STAGE_RELOADRIGHTWAIT,
		STAGE_RELOADDONE,
		STAGE_RELOADDONEWAIT,		
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( DrumSpin ( NAILGUN_DRUMSPEED_STOPPED, parms.blendFrames ) ) {
				PostState ( "reload", parms.blendFrames );
				return SRESULT_DONE;
			}

			SetStatus ( WP_RELOAD );

			if ( wsfl.netReload ) {
				wsfl.netReload = false;
			} else {
				NetReload ( );
			}

			if ( mods & NAILGUN_MOD_ROF_AMMO ) {
				return SRESULT_STAGE( STAGE_RELOADLEFT );
			}						
			return SRESULT_STAGE( STAGE_RELOAD );
			
		case STAGE_RELOAD:
			PlayAnim( ANIMCHANNEL_LEGS, "reload", parms.blendFrames );
			return SRESULT_STAGE( STAGE_RELOADWAIT );
			
		case STAGE_RELOADWAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				AddToClip( ClipSize ( ) );
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}			
			return SRESULT_WAIT;
		
		case STAGE_RELOADLEFT:
			PlayAnim ( ANIMCHANNEL_LEGS, "reload_clip1hold", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_RELOADLEFTWAIT );
			
		case STAGE_RELOADLEFTWAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 0 ) ) {
				AddToClip ( ClipSize() / 2 );				
				return SRESULT_STAGE ( STAGE_RELOADRIGHT );
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}			
			return SRESULT_WAIT;

		case STAGE_RELOADRIGHT:
			if ( wsfl.attack || AmmoInClip() >= ClipSize() || !AmmoAvailable() ) {
				return SRESULT_STAGE ( STAGE_RELOADDONE );
			}
			PlayAnim ( ANIMCHANNEL_LEGS, "reload_clip2", 0 );
			return SRESULT_STAGE ( STAGE_RELOADRIGHTWAIT );
			
		case STAGE_RELOADRIGHTWAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				AddToClip( ClipSize() / 2 );
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}			
			return SRESULT_WAIT;
		
		case STAGE_RELOADDONE:
			PlayAnim ( ANIMCHANNEL_LEGS, "reload_clip1finish", 0 );
			return SRESULT_STAGE ( STAGE_RELOADDONEWAIT );
		
		case STAGE_RELOADDONEWAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
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
rvWeaponNailgun::State_DrumSpinUp

Spin the drum from a slow speed to a fast speed
================
*/
stateResult_t rvWeaponNailgun::State_DrumSpinUp ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			viewModel->StartSound ( "snd_spinup", NAILGUN_SPIN_SNDCHANNEL, 0, false, NULL);
			PlayAnim ( ANIMCHANNEL_LEGS, "spinup", 4 );
			return SRESULT_STAGE(STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 0 ) ) {
				viewModel->StartSound ( "snd_spinfast", NAILGUN_SPIN_SNDCHANNEL, 0, false, NULL );
				drumSpeed = drumSpeedIdeal;
				return SRESULT_DONE;
			}
			if ( !wsfl.attack ) {
				int oldSpeed = drumSpeed;
				drumSpeed = drumSpeedIdeal;
				DrumSpin ( oldSpeed, 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::State_DrumSpinDown

Spin the drum down from a faster speed
================
*/
stateResult_t rvWeaponNailgun::State_DrumSpinDown ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			viewModel->StartSound ( "snd_spindown", SND_CHANNEL_ANY, 0, false, 0 );

			// Spin down animation	
			PlayAnim( ANIMCHANNEL_LEGS, "spindown", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				drumSpeed = drumSpeedIdeal;
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				viewModel->StopSound ( NAILGUN_SPIN_SNDCHANNEL, false );
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.attack && AmmoInClip ( ) ) {
				viewModel->StopSound ( NAILGUN_SPIN_SNDCHANNEL, false );
				SetState ( "Fire", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.netReload || (wsfl.reload && AmmoAvailable() > AmmoInClip() && AmmoInClip() < ClipSize()) ) {
				SetState ( "Reload", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::Frame_ClaspOpen

Open the clasp that holds in the clips
================
*/
stateResult_t rvWeaponNailgun::Frame_ClaspOpen ( const stateParms_t& parms ) {
	PlayAnim ( ANIMCHANNEL_TORSO, "clasp_open", 0 );
	return SRESULT_OK;
}

/*
================
rvWeaponNailgun::Frame_ClaspOpen

Close the clasp that holds in the clips and make sure to use the
correct positioning depending on whether you have one or two clips
in the gun.
================
*/
stateResult_t rvWeaponNailgun::Frame_ClaspClose ( const stateParms_t& parms ) {
	if ( mods & NAILGUN_MOD_ROF_AMMO ) {
		PlayAnim( ANIMCHANNEL_TORSO, "clasp_2clip", 0 );
	} else {
		PlayAnim( ANIMCHANNEL_TORSO, "clasp_1clip", 0 );
	}
	return SRESULT_OK;
}
