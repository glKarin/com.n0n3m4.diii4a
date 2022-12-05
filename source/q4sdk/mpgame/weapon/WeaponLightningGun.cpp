#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"
#include "../client/ClientEffect.h"
#include "../Projectile.h"
#include "../ai/AI_Manager.h"

const int	LIGHTNINGGUN_NUM_TUBES	=	3;
const int	LIGHTNINGGUN_MAX_PATHS  =	3;

const idEventDef EV_Lightninggun_RestoreHum( "<lightninggunRestoreHum>", "" );

class rvLightningPath {
public:
	idEntityPtr<idEntity>	target;
	idVec3					origin;
	idVec3					normal;
	rvClientEffectPtr		trailEffect;
	rvClientEffectPtr		impactEffect;
	
	void					StopEffects		( void );
	void					UpdateEffects	( const idVec3& from, const idDict& dict );
	void					Save			( idSaveGame* savefile ) const;
	void					Restore			( idRestoreGame* savefile );
};

class rvWeaponLightningGun : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponLightningGun );

	rvWeaponLightningGun( void );
	~rvWeaponLightningGun( void );

	virtual void			Spawn		( void );
	virtual void			Think		( void );

	virtual void			ClientStale	( void );
	virtual void			ClientUnstale( void );

	void					PreSave		( void );
	void					PostSave	( void );

	void					Save		( idSaveGame* savefile ) const;
	void					Restore		( idRestoreGame* savefile );

	bool			NoFireWhileSwitching( void ) const { return true; }

protected:

	void					UpdateTubes	( void );

	// Tube effects
	rvClientEntityPtr<rvClientEffect>	tubeEffects[LIGHTNINGGUN_NUM_TUBES];
	idInterpolate<float>				tubeOffsets[LIGHTNINGGUN_NUM_TUBES];
	jointHandle_t						tubeJoints[LIGHTNINGGUN_NUM_TUBES];
	float								tubeMaxOffset;
	float								tubeThreshold;
	int									tubeTime;

	rvClientEntityPtr<rvClientEffect>	trailEffectView;

	int									nextCrawlTime;

	float								range;
	jointHandle_t						spireJointView;
	jointHandle_t						chestJointView;
	

	rvLightningPath						currentPath;
	
	// Chain lightning mod
	idList<rvLightningPath>				chainLightning;
	idVec3								chainLightningRange;

private:

	void				Attack					( idEntity* ent, const idVec3& dir, float power = 1.0f );

	void				UpdateChainLightning	( void );
	void				StopChainLightning		( void );
	void				UpdateEffects			( const idVec3& origin );
	void				UpdateTrailEffect		( rvClientEffectPtr& effect, const idVec3& start, const idVec3& end, bool view = false );

	stateResult_t		State_Raise				( const stateParms_t& parms );
	stateResult_t		State_Lower				( const stateParms_t& parms );
	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_Fire				( const stateParms_t& parms );

	void				Event_RestoreHum	( void );

	CLASS_STATES_PROTOTYPE ( rvWeaponLightningGun );
};

CLASS_DECLARATION( rvWeapon, rvWeaponLightningGun )
EVENT( EV_Lightninggun_RestoreHum,			rvWeaponLightningGun::Event_RestoreHum )
END_CLASS

/*
================
rvWeaponLightningGun::rvWeaponLightningGun
================
*/
rvWeaponLightningGun::rvWeaponLightningGun( void ) {
}

/*
================
rvWeaponLightningGun::~rvWeaponLightningGun
================
*/
rvWeaponLightningGun::~rvWeaponLightningGun( void ) {
	int i;
	
	if ( trailEffectView ) {
		trailEffectView->Stop( );
	}	
	currentPath.StopEffects( );
	StopChainLightning( );
	
	for ( i = 0; i < LIGHTNINGGUN_NUM_TUBES; i ++ ) {	
		if ( tubeEffects[i] ) {
			tubeEffects[i]->Stop( );
		}
	}
}

/*
================
rvWeaponLightningGun::Spawn
================
*/
void rvWeaponLightningGun::Spawn( void ) {
	int i;
	
	trailEffectView = NULL;
	nextCrawlTime	= 0;

	chainLightning.Clear( );
	
	// get hitscan range for our firing
	range = weaponDef->dict.GetFloat( "range", "10000" );

	// Initialize tubes
	for ( i = 0; i < LIGHTNINGGUN_NUM_TUBES; i ++ ) {
		tubeJoints[i] = viewModel->GetAnimator()->GetJointHandle ( spawnArgs.GetString ( va("joint_tube_%d",i), "" ) );
		tubeOffsets[i].Init ( gameLocal.time, 0, 0, 0 );
	}
	
	// Cache the max ammo for the weapon and the max tube offset
	tubeMaxOffset = spawnArgs.GetFloat ( "tubeoffset" );
	tubeThreshold = owner->inventory.MaxAmmoForAmmoClass ( owner, GetAmmoNameForIndex ( ammoType ) ) / (float)LIGHTNINGGUN_NUM_TUBES;
	tubeTime	  = SEC2MS ( spawnArgs.GetFloat ( "tubeTime", ".25" ) );
		
	spireJointView = viewModel->GetAnimator ( )->GetJointHandle ( "spire_1" );	

	if( gameLocal.GetLocalPlayer())	{
		chestJointView = gameLocal.GetLocalPlayer()->GetAnimator()->GetJointHandle( spawnArgs.GetString ( "joint_hideGun_flash" ) );
	} else {
		chestJointView = spireJointView;
	}

	chainLightningRange = spawnArgs.GetVec2( "chainLightningRange", "150 300" );
	
	SetState ( "Raise", 0 );
}

/*
================
rvWeaponLightningGun::Save
================
*/
void rvWeaponLightningGun::Save	( idSaveGame* savefile ) const {
	int i;

	// Lightning Tubes
	for ( i = 0; i < LIGHTNINGGUN_NUM_TUBES; i ++ ) {
		tubeEffects[i].Save ( savefile );
		savefile->WriteInterpolate ( tubeOffsets[i] );
		savefile->WriteJoint ( tubeJoints[i] );
	}
	savefile->WriteFloat ( tubeMaxOffset );
	savefile->WriteFloat ( tubeThreshold );
	savefile->WriteInt ( tubeTime );

	// General
	trailEffectView.Save ( savefile );
	savefile->WriteInt ( nextCrawlTime );
	savefile->WriteFloat ( range );
	savefile->WriteJoint ( spireJointView );
	savefile->WriteJoint ( chestJointView );

	currentPath.Save ( savefile );
	
	// Chain Lightning mod
	savefile->WriteInt ( chainLightning.Num() );
	for ( i = 0; i < chainLightning.Num(); i ++ ) {
		chainLightning[i].Save ( savefile );
	}
	savefile->WriteVec3 ( chainLightningRange );
}

/*
================
rvWeaponLightningGun::Restore
================
*/
void rvWeaponLightningGun::Restore ( idRestoreGame* savefile ) {
	int   i;
	int   num;
	float f;
	
	// Lightning Tubes
	for ( i = 0; i < LIGHTNINGGUN_NUM_TUBES; i ++ ) {
		tubeEffects[i].Restore ( savefile );
		savefile->ReadFloat ( f );
		tubeOffsets[i].SetStartTime ( f );
		savefile->ReadFloat ( f );
		tubeOffsets[i].SetDuration ( f );
		savefile->ReadFloat ( f );
		tubeOffsets[i].SetStartValue ( f );
		savefile->ReadFloat ( f );
		tubeOffsets[i].SetEndValue ( f );
		savefile->ReadJoint ( tubeJoints[i] );
	}
	savefile->ReadFloat ( tubeMaxOffset );
	savefile->ReadFloat ( tubeThreshold );
	savefile->ReadInt ( tubeTime );
	
	// General
	trailEffectView.Restore ( savefile );

	savefile->ReadInt ( nextCrawlTime );
	savefile->ReadFloat ( range );
	savefile->ReadJoint ( spireJointView );
	savefile->ReadJoint ( chestJointView );

	currentPath.Restore ( savefile );
	
	// Chain lightning mod
	savefile->ReadInt ( num );
	chainLightning.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		chainLightning[i].Restore ( savefile );
	}
	savefile->ReadVec3 ( chainLightningRange );


}

/*
================
rvWeaponLightningGun::Think
================
*/
void rvWeaponLightningGun::Think ( void ) {
	trace_t	  tr;

	rvWeapon::Think();

	UpdateTubes();

	// If no longer firing or out of ammo then nothing to do in the think
	if ( !wsfl.attack || !IsReady() || !AmmoAvailable() ) {
		if ( trailEffectView ) {
			trailEffectView->Stop ( );
			trailEffectView = NULL;
		}
	
		currentPath.StopEffects();
		StopChainLightning();
		return;
	}

	// Cast a ray out to the lock range
// RAVEN BEGIN
// ddynerman: multiple clip worlds
// jshepard: allow projectile hits
	gameLocal.TracePoint(	owner, tr, 
							playerViewOrigin, 
							playerViewOrigin + playerViewAxis[0] * range, 
							(MASK_SHOT_RENDERMODEL|CONTENTS_WATER|CONTENTS_PROJECTILE), owner );
// RAVEN END
	// Calculate the direction of the lightning effect using the barrel joint of the weapon
	// and the end point of the trace
	idVec3 origin;
	idMat3 axis;
	
	//fire from chest if show gun models is off.
	if( !cvarSystem->GetCVarBool("ui_showGun"))	{
		GetGlobalJointTransform( true, chestJointView, origin, axis );
	} else {
		GetGlobalJointTransform( true, barrelJointView, origin, axis );
	}

	// Cache the target we are hitting
	currentPath.origin = tr.endpos;
	currentPath.normal = tr.c.normal;
	currentPath.target = gameLocal.entities[tr.c.entityNum];

	UpdateChainLightning();
	
	UpdateEffects( origin );
	
	MuzzleFlash();

	// Inflict damage on all targets being attacked
	if ( !gameLocal.isClient && gameLocal.time >= nextAttackTime ) {
		int    i;
		float  power = 1.0f;
		idVec3 dir;
		
		owner->inventory.UseAmmo( ammoType, ammoRequired );
		
		dir = tr.endpos - origin;
		dir.Normalize ( );
		
		nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
		Attack ( currentPath.target, dir, power );
		for ( i = 0; i < chainLightning.Num(); i ++, power *= 0.75f ) {
			Attack ( chainLightning[i].target, chainLightning[i].normal, power );
		}

		statManager->WeaponFired( owner, owner->GetCurrentWeapon(), chainLightning.Num() + 1 );
	}

	// Play the lightning crawl effect every so often when doing damage
	if ( gameLocal.time > nextCrawlTime ) {
		nextCrawlTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat ( "crawlDelay", ".3" ));
	}
}

/*
================
rvWeaponLightningGun::Attack
================
*/
void rvWeaponLightningGun::Attack ( idEntity* ent, const idVec3& dir, float power ) {
	// Double check
	if ( !ent || !ent->fl.takedamage ) {
		return;
	}

	// Start a lightning crawl effect every so often
	// we don't synchronize it, so let's not show it in multiplayer for a listen host. also fixes seeing it on the host from other instances
	if ( !gameLocal.isMultiplayer && gameLocal.time > nextCrawlTime ) {
		if ( ent->IsType( idActor::GetClassType() ) ) {
			rvClientCrawlEffect* effect;
			effect = new rvClientCrawlEffect( gameLocal.GetEffect( weaponDef->dict, "fx_crawl" ), ent, SEC2MS( spawnArgs.GetFloat ( "crawlTime", ".2" ) ) );
			effect->Play( gameLocal.time, false );
		}
	}	

// RAVEN BEGIN
// mekberg: stats
	if( owner->IsType( idPlayer::GetClassType() ) && ent->IsType( idActor::GetClassType() ) && ent != owner && !((idPlayer*)owner)->pfl.dead ) {
		statManager->WeaponHit( (idActor*)owner, ent, owner->GetCurrentWeapon() );
	}
// RAVEN END
	ent->Damage( owner, owner, dir, spawnArgs.GetString ( "def_damage" ), power * owner->PowerUpModifier( PMOD_PROJECTILE_DAMAGE ), 0 );
}

/*
================
rvWeaponLightningGun::UpdateChainLightning
================
*/
void rvWeaponLightningGun::UpdateChainLightning ( void ) {
	int					i;
	rvLightningPath*	parent;
	rvLightningPath*	path;
	idActor*			target;

	// Chain lightning not enabled
	if ( !chainLightningRange[0] ) {
		return;
	}

	// Need to have a primary target that is not on the same team for chain lightning to work
	path   = &currentPath;
	target = dynamic_cast<idActor*>(path->target.GetEntity());
	if ( !target || !target->health || target->team == owner->team ) {
		StopChainLightning ( );
		return;
	}
	
	currentPath.target->fl.takedamage = false;
	
	// Look through the chain lightning list and remove any paths that are no longer valid due
	// to their range or visibility
	for ( i = 0; i < chainLightning.Num(); i ++ ) {
		parent = path;
		path   = &chainLightning[i];
		target = dynamic_cast<idActor*>(path->target.GetEntity());
		
		// If the entity isnt valid anymore or is dead then remove it from the list
		if ( !target || target->health <= 0 || !target->fl.takedamage ) {
			path->StopEffects ( );
			chainLightning.RemoveIndex ( i-- );
			continue;
		}

		// Choose a destination origin the chain lightning path target	
		path->origin = (target->GetPhysics()->GetAbsBounds().GetCenter() + target->GetEyePosition()) * 0.5f;
		path->origin += target->GetPhysics()->GetGravityNormal() * (gameLocal.random.RandomFloat() * 20.0f - 10.0f);
		path->normal = path->origin - parent->origin;
		path->normal.Normalize();
		
		// Make sure the entity is still within range of its parent
		if ( (path->origin - parent->origin).LengthSqr ( ) > Square ( chainLightningRange[1] ) ) {
			path->StopEffects ( );
			chainLightning.RemoveIndex ( i-- );
			continue;
		}		
						
		// Trace to make sure we can still hit them
		trace_t tr;
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		gameLocal.TracePoint ( owner, tr, parent->origin, path->origin, MASK_SHOT_RENDERMODEL, parent->target );
// RAVEN END
		if ( tr.c.entityNum != target->entityNumber ) {
			path->StopEffects ( );
			chainLightning.RemoveIndex ( i-- );
			continue;
		}
		
		path->origin = tr.endpos;
		
		// Temporarily disable taking damage to flag this entity is used
		target->fl.takedamage = false;
	}
	
	// Start path at the end of the current path
	if ( chainLightning.Num () ) {
		path = &chainLightning[chainLightning.Num()-1];
	} else {
		path = &currentPath;
	}
	
	// Cap the number of chain lightning jumps
	while ( chainLightning.Num ( ) + 1 < LIGHTNINGGUN_MAX_PATHS ) {	
		for ( target = aiManager.GetEnemyTeam ( (aiTeam_t)owner->team ); target; target = target->teamNode.Next() ) {
			// Must be a valid entity that takes damage to chain lightning too
			if ( !target || target->health <= 0 || !target->fl.takedamage ) {
				continue;
			}
			// Must be within starting chain path range
			if ( (target->GetPhysics()->GetOrigin() - path->target->GetPhysics()->GetOrigin()).LengthSqr() > Square ( chainLightningRange[0] ) ) {
				continue;
			}
			
			// Make sure we can trace to the target from the current path
			trace_t	tr;
			idVec3	origin;
			origin = (target->GetPhysics()->GetAbsBounds().GetCenter() + target->GetEyePosition()) * 0.5f;
			origin += target->GetPhysics()->GetGravityNormal() * (gameLocal.random.RandomFloat() * 20.0f - 10.0f);
// RAVEN BEGIN
// ddynerman: multiple clip worlds
			gameLocal.TracePoint ( owner, tr, path->origin, origin, MASK_SHOT_RENDERMODEL, path->target );
// RAVEN END
			if ( tr.c.entityNum != target->entityNumber ) {
				continue;
			}
			
			path = &chainLightning.Alloc ( );
			path->target = target;
			path->normal = tr.endpos - path->origin;
			path->normal.Normalize();
			path->origin = tr.endpos;
			
			// Flag this entity to ensure it is skipped
			target->fl.takedamage = false;
			break;
		}
		// Found nothing? just break out early
		if ( !target ) {
			break;
		}
	}	
	
	// Reset the take damage flag 
	currentPath.target->fl.takedamage = true;
	for ( i = chainLightning.Num() - 1; i >= 0; i -- ) {
		chainLightning[i].target->fl.takedamage = true;
	}
}

/*
================
rvWeaponLightningGun::UpdateEffects
================
*/
void rvWeaponLightningGun::UpdateEffects( const idVec3& origin ) {
	int					i;
	rvLightningPath*	parent;
	idVec3				dir;
	
	// Main path (world effects)
	currentPath.UpdateEffects ( origin, weaponDef->dict );
	if ( currentPath.trailEffect ) {
		currentPath.trailEffect->GetRenderEffect()->suppressSurfaceInViewID = owner->entityNumber + 1;
	}

	// In view trail effect
	dir = currentPath.origin - origin;
	dir.Normalize();
	if ( !trailEffectView ) {
		trailEffectView = gameLocal.PlayEffect ( gameLocal.GetEffect ( weaponDef->dict, "fx_trail" ), origin, dir.ToMat3(), true, currentPath.origin );		
	} else {
		trailEffectView->SetOrigin( origin );
		trailEffectView->SetAxis( dir.ToMat3() );
		trailEffectView->SetEndOrigin( currentPath.origin );
	}	
	if ( trailEffectView ) {
		trailEffectView->GetRenderEffect()->allowSurfaceInViewID = owner->entityNumber + 1;;
	}

	if ( !currentPath.target ) {
		return;
	}
	
	// Chain lightning effects
	parent = &currentPath;
	for ( i = 0; i < chainLightning.Num(); i ++ ) {
		chainLightning[i].UpdateEffects( parent->origin, weaponDef->dict );
		parent = &chainLightning[i];
	}
}

/*
================
rvWeaponLightningGun::UpdateTrailEffect
================
*/
void rvWeaponLightningGun::UpdateTrailEffect( rvClientEffectPtr& effect, const idVec3& start, const idVec3& end, bool view ) {
	idVec3 dir;
	dir = end - start;
	dir.Normalize();
	
	if ( !effect ) {
		effect = gameLocal.PlayEffect( gameLocal.GetEffect( weaponDef->dict, view ? "fx_trail" : "fx_trail_world" ), start, dir.ToMat3(), true, end );		
	} else {
		effect->SetOrigin( start );
		effect->SetAxis( dir.ToMat3() );
		effect->SetEndOrigin( end );
	}
}

/*
================
rvWeaponLightningGun::StopChainLightning
================
*/
void rvWeaponLightningGun::StopChainLightning( void ) {
	int	i;
	
	if ( !chainLightning.Num( ) ) {
		return;
	}
	
	for ( i = 0; i < chainLightning.Num(); i ++ ) {
		chainLightning[i].StopEffects( );
	}		
	
	chainLightning.Clear( );
}

/*
================
rvWeaponLightningGun::ClientStale
================
*/
void rvWeaponLightningGun::ClientStale( void ) {
	rvWeapon::ClientStale( );

	if ( trailEffectView ) {
		trailEffectView->Stop();
		trailEffectView->Event_Remove();
		trailEffectView = NULL;
	}
	
	currentPath.StopEffects( );
}

/*
================
rvWeaponLightningGun::PreSave
================
*/
void rvWeaponLightningGun::PreSave( void ) {

	SetState ( "Idle", 4 );

	StopSound( SND_CHANNEL_WEAPON, 0);
	StopSound( SND_CHANNEL_BODY, 0);
	StopSound( SND_CHANNEL_BODY2, 0);
	StopSound( SND_CHANNEL_BODY3, 0);
	StopSound( SND_CHANNEL_ITEM, 0);
	StopSound( SND_CHANNEL_ANY, false );
	
	viewModel->StopSound( SND_CHANNEL_ANY, false );
	viewModel->StopSound( SND_CHANNEL_BODY, 0);

	worldModel->StopSound( SND_CHANNEL_ANY, false );
	worldModel->StopSound( SND_CHANNEL_BODY, 0);

	
	if ( trailEffectView ) {
		trailEffectView->Stop();
		trailEffectView->Event_Remove();
		trailEffectView = NULL;
	}
	for ( int i = 0; i < LIGHTNINGGUN_NUM_TUBES; i ++ ) {
		if ( tubeEffects[i] ) {
			tubeEffects[i]->Event_Remove();
		}
	}	
	currentPath.StopEffects( );

}

/*
================
rvWeaponLightningGun::PostSave
================
*/
void rvWeaponLightningGun::PostSave( void ) {
	//restore the humming
	PostEventMS( &EV_Lightninggun_RestoreHum, 10 );
}

/*
================
rvWeaponLightningGun::UpdateTubes
================
*/
void rvWeaponLightningGun::UpdateTubes( void ) {
	idAnimator* animator;
	animator = viewModel->GetAnimator ( );
	assert ( animator );

	int i;
	float ammo;
	
	ammo = AmmoAvailable ( );
	for ( i = 0; i < LIGHTNINGGUN_NUM_TUBES; i ++ ) {
		float offset;
		if ( ammo > tubeThreshold * i ) {
			offset = tubeMaxOffset;
			
			if ( !tubeEffects[i] ) {
				tubeEffects[i] = viewModel->PlayEffect ( gameLocal.GetEffect( spawnArgs, "fx_tube" ), tubeJoints[i], vec3_origin, mat3_identity, true );
				if( tubeEffects[i] ) {
					viewModel->StartSound ( "snd_tube", SND_CHANNEL_ANY, 0, false, NULL );
				}
			}
		} else {
			offset = 0;
			
			if ( tubeEffects[i] ) {
				tubeEffects[i]->Stop ( );
				tubeEffects[i] = NULL;
				viewModel->StartSound ( "snd_tube", SND_CHANNEL_ANY, 0, false, NULL );
			}
		}

		// Attenuate the tube effect to how much ammo is left in the tube
		if ( tubeEffects[i] ) {
			tubeEffects[i]->Attenuate ( (ammo - tubeThreshold * (float)i) / tubeThreshold );
		}

		if ( offset > tubeOffsets[i].GetEndValue ( ) ) {
			float current;
			current  = tubeOffsets[i].GetCurrentValue(gameLocal.time);
			tubeOffsets[i].Init ( gameLocal.time, (1.0f - (current/tubeMaxOffset)) * (float)tubeTime, current, offset );
		} else if ( offset < tubeOffsets[i].GetEndValue ( ) ) {
			float current;
			current  = tubeOffsets[i].GetCurrentValue(gameLocal.time);
			tubeOffsets[i].Init ( gameLocal.time, (current/tubeMaxOffset) * (float)tubeTime, current, offset );
		}			

		animator->SetJointPos ( tubeJoints[i], JOINTMOD_LOCAL, idVec3(tubeOffsets[i].GetCurrentValue(gameLocal.time),0,0) );
	}		
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponLightningGun )
	STATE ( "Raise",						rvWeaponLightningGun::State_Raise )
	STATE ( "Lower",						rvWeaponLightningGun::State_Lower )
	STATE ( "Idle",							rvWeaponLightningGun::State_Idle)
	STATE ( "Fire",							rvWeaponLightningGun::State_Fire )
END_CLASS_STATES

/*
================
rvWeaponLightningGun::State_Raise
================
*/
stateResult_t rvWeaponLightningGun::State_Raise( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		// Start the weapon raising
		case STAGE_INIT:
			SetStatus( WP_RISING );
			viewModel->SetShaderParm( 6, 1 );
			PlayAnim( ANIMCHANNEL_ALL, "raise", parms.blendFrames );
			return SRESULT_STAGE( STAGE_WAIT );
		
		// Wait for the weapon to finish raising and allow it to be cancelled by
		// lowering the weapon 
		case STAGE_WAIT:
			if ( AnimDone( ANIMCHANNEL_ALL, 4 ) ) {
				SetState( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				StopSound( SND_CHANNEL_BODY3, false );
				SetState( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponLightningGun::State_Lower
================
*/
stateResult_t rvWeaponLightningGun::State_Lower( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
		STAGE_WAITRAISE
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			SetStatus( WP_LOWERING );
			PlayAnim( ANIMCHANNEL_ALL, "putaway", parms.blendFrames );
			return SRESULT_STAGE(STAGE_WAIT);
		
		case STAGE_WAIT:
			if ( AnimDone( ANIMCHANNEL_ALL, 0 ) ) {
				SetStatus( WP_HOLSTERED );
				return SRESULT_STAGE(STAGE_WAITRAISE);
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITRAISE:
			if ( wsfl.raiseWeapon ) {
				SetState( "Raise", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponLightningGun::State_Idle
================
*/
stateResult_t rvWeaponLightningGun::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			SetStatus( WP_READY );
			PlayCycle( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
			StopSound( SND_CHANNEL_BODY3, false );
			StartSound( "snd_idle_hum", SND_CHANNEL_BODY3, 0, false, NULL );

			return SRESULT_STAGE( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( wsfl.lowerWeapon ) {
				StopSound( SND_CHANNEL_BODY3, false );
				SetState( "Lower", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.attack && gameLocal.time > nextAttackTime && AmmoAvailable ( ) ) {
				SetState( "Fire", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponLightningGun::State_Fire
================
*/
stateResult_t rvWeaponLightningGun::State_Fire( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_ATTACKLOOP,
		STAGE_DONE,
		STAGE_DONEWAIT
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			StartSound( "snd_fire", SND_CHANNEL_WEAPON, 0, false, NULL );
			StartSound( "snd_fire_stereo", SND_CHANNEL_ITEM, 0, false, NULL );
			StartSound( "snd_fire_loop", SND_CHANNEL_BODY2, 0, false, NULL );
			
			viewModel->SetShaderParm( 6, 0 );

			viewModel->PlayEffect( "fx_spire", spireJointView, true );
			viewModel->PlayEffect( "fx_flash", barrelJointView, true );

			if ( worldModel && flashJointWorld != INVALID_JOINT ) {
  				worldModel->PlayEffect( gameLocal.GetEffect( weaponDef->dict,"fx_flash_world"), flashJointWorld, vec3_origin, mat3_identity, true );
  			}

			PlayAnim( ANIMCHANNEL_ALL, "shoot_start", parms.blendFrames );
			return SRESULT_STAGE( STAGE_ATTACKLOOP );
		
		case STAGE_ATTACKLOOP:
			if ( !wsfl.attack || wsfl.lowerWeapon || !AmmoAvailable ( ) ) {
				return SRESULT_STAGE ( STAGE_DONE );
			}
			if ( AnimDone( ANIMCHANNEL_ALL, 0 ) ) {
				PlayCycle( ANIMCHANNEL_ALL, "shoot_loop", 0 );
				if ( !gameLocal.isMultiplayer
					&& owner == gameLocal.GetLocalPlayer() ) {
					owner->playerView.SetShakeParms( MS2SEC(gameLocal.GetTime() + 500), 2.0f );
				}
			}
			return SRESULT_WAIT;
						
		case STAGE_DONE:
			StopSound( SND_CHANNEL_BODY2, false );

			viewModel->StopEffect( "fx_spire" );
			viewModel->StopEffect( "fx_flash" );
 			if ( worldModel ) {
  				worldModel->StopEffect( gameLocal.GetEffect( weaponDef->dict, "fx_flash_world" ) );
  			}
			viewModel->SetShaderParm( 6, 1 );

			PlayAnim( ANIMCHANNEL_ALL, "shoot_end", 0 );
			return SRESULT_STAGE( STAGE_DONEWAIT );
			
		case STAGE_DONEWAIT:
			if ( AnimDone( ANIMCHANNEL_ALL, 4 ) ) {
				SetState( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( !wsfl.lowerWeapon && wsfl.attack && AmmoAvailable ( ) ) {
				SetState( "Fire", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvWeaponLightningGun::Event_RestoreHum
================
*/
void rvWeaponLightningGun::Event_RestoreHum ( void ) {
	StopSound( SND_CHANNEL_BODY3, false );
	StartSound( "snd_idle_hum", SND_CHANNEL_BODY3, 0, false, NULL );
}

/*
================
rvWeaponLightningGun::ClientUnStale
================
*/
void rvWeaponLightningGun::ClientUnstale( void ) {
	Event_RestoreHum();
}

/*
===============================================================================

	rvLightningPath

===============================================================================
*/

/*
================
rvLightningPath::StopEffects
================
*/
void rvLightningPath::StopEffects( void ) {
	if ( trailEffect ) {
		trailEffect->Stop( );
		trailEffect->Event_Remove( );
		trailEffect = NULL;
	}
	if ( impactEffect ) {
		impactEffect->Stop( );
		impactEffect->PostEventMS( &EV_Remove, 1000 );
		impactEffect = NULL;
	}
}

/*
================
rvLightningPath::UpdateEffects
================
*/
void rvLightningPath::UpdateEffects ( const idVec3& from, const idDict& dict ) {
	idVec3 dir;
	dir = origin - from;
	dir.Normalize();
	
	// Trail effect
	if ( !trailEffect ) {
		trailEffect = gameLocal.PlayEffect ( gameLocal.GetEffect ( dict, "fx_trail_world" ), from, dir.ToMat3(), true, origin );		
	} else {
		trailEffect->SetOrigin ( from );
		trailEffect->SetAxis ( dir.ToMat3() );
		trailEffect->SetEndOrigin ( origin );
	}
	
	// Impact effect
	if ( !target || target.GetEntityNum ( ) == ENTITYNUM_NONE ) {
		if ( impactEffect ) {
			impactEffect->Stop ( );
			impactEffect->PostEventMS( &EV_Remove, 1000 );
			impactEffect = NULL;
		}
	} else { 
		if ( !impactEffect ) {
			impactEffect = gameLocal.PlayEffect ( gameLocal.GetEffect ( dict, "fx_impact" ), origin, normal.ToMat3(), true );
		} else {
			impactEffect->SetOrigin ( origin );
			impactEffect->SetAxis ( normal.ToMat3 ( ) );
		}
	}		
}

/*
================
rvLightningPath::Save
================
*/
void rvLightningPath::Save( idSaveGame* savefile ) const {
	target.Save( savefile );
	savefile->WriteVec3( origin );
	savefile->WriteVec3( normal );
	trailEffect.Save( savefile );
	impactEffect.Save( savefile );
}

/*
================
rvLightningPath::Restore
================
*/
void rvLightningPath::Restore( idRestoreGame* savefile ) {
	target.Restore( savefile );
	savefile->ReadVec3( origin );
	savefile->ReadVec3( normal );
	trailEffect.Restore( savefile );
	impactEffect.Restore( savefile );
}
