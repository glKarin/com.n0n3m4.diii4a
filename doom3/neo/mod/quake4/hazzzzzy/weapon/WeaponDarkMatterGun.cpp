#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"
#include "../Projectile.h"

class rvWeaponDarkMatterGun : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponDarkMatterGun );

	rvWeaponDarkMatterGun ( void );
	~rvWeaponDarkMatterGun ( void );

	virtual void			Spawn				( void );
	void					Save				( idSaveGame *savefile ) const;
	void					Restore				( idRestoreGame *savefile );
	void					PreSave					( void );
	void					PostSave				( void );

#ifdef _XENON
	virtual bool		AllowAutoAim			( void ) const { return false; }
#endif

protected:

	enum darkMatterRing_t {
		RING_OUTER,
		RING_MIDDLE,
		RING_INNER,
		RING_MAX
	};

	struct rings_s {
		idAngles		angularVelocity;
		jointHandle_t	joint;
	};
	rings_s				rings[ RING_MAX ];

	int					nextRotateTime;
	int					ringStartTime;
	int					chargeDuration;
	bool				clientReload;
	rvClientEffectPtr	coreEffect;
	rvClientEffectPtr	coreStartEffect;
	jointHandle_t		jointCore;

	void				InitRing		( darkMatterRing_t ring, const char* name );
	void				StartRings		( bool chargeUp );
	void				StopRings		( void );

private:

	stateResult_t		State_Idle		( const stateParms_t& parms );
	stateResult_t		State_Fire		( const stateParms_t& parms );
	stateResult_t		State_Reload	( const stateParms_t& parms );
	
	CLASS_STATES_PROTOTYPE ( rvWeaponDarkMatterGun );
};

CLASS_DECLARATION( rvWeapon, rvWeaponDarkMatterGun )
END_CLASS

/*
================
rvWeaponDarkMatterGun::rvWeaponDarkMatterGun
================
*/
rvWeaponDarkMatterGun::rvWeaponDarkMatterGun ( void ) {
	coreStartEffect = NULL;
	coreEffect		= NULL;
	ringStartTime	= -1;
	clientReload	= false;
}

/*
================
rvWeaponDarkMatterGun::~rvWeaponDarkMatterGun
================
*/
rvWeaponDarkMatterGun::~rvWeaponDarkMatterGun ( void ) {
	StopRings ( );
}

/*
================
rvWeaponDarkMatterGun::Spawn
================
*/
void rvWeaponDarkMatterGun::Spawn ( void ) {
	SetState ( "Raise", 0 );	
	
	InitRing ( RING_OUTER, "outer" );
	InitRing ( RING_INNER, "inner" );
	InitRing ( RING_MIDDLE, "middle" );
	
	nextRotateTime = 0;
	
	chargeDuration = SEC2MS ( spawnArgs.GetFloat ( "chargeDuration", ".5" ) );
	
	jointCore = viewModel->GetAnimator()->GetJointHandle ( spawnArgs.GetString ( "joint_core" ) );
}

/*
================
rvWeaponDarkMatterGun::Save
================
*/
void rvWeaponDarkMatterGun::Save ( idSaveGame *savefile ) const {
	for ( int i = 0; i < RING_MAX; i++ ) {
		savefile->WriteAngles ( rings[ i ].angularVelocity );
		savefile->WriteJoint ( rings[ i ].joint );
	}
	savefile->WriteInt ( nextRotateTime );
	savefile->WriteInt ( ringStartTime );
	savefile->WriteInt ( chargeDuration );
	savefile->WriteObject( coreEffect.GetEntity() );
	savefile->WriteObject( coreStartEffect.GetEntity() );
	savefile->WriteJoint ( jointCore );
}

/*
================
rvWeaponDarkMatterGun::Restore
================
*/
void rvWeaponDarkMatterGun::Restore ( idRestoreGame *savefile ) {
	for ( int i = 0; i < RING_MAX; i++ ) {
		savefile->ReadAngles ( rings[ i ].angularVelocity );
		savefile->ReadJoint ( rings[ i ].joint );
	}
	savefile->ReadInt ( nextRotateTime );
	savefile->ReadInt ( ringStartTime );
	savefile->ReadInt ( chargeDuration );
	savefile->ReadObject( reinterpret_cast<idClass*&>( coreEffect ) );
	savefile->ReadObject( reinterpret_cast<idClass*&>( coreStartEffect ) );
	savefile->ReadJoint ( jointCore );
}

/*
================
rvWeaponDarkMatterGun::PreSave
================
*/
void rvWeaponDarkMatterGun::PreSave ( void ) {

	//disable sounds
	StopSound( SND_CHANNEL_ANY, false);

}

/*
================
rvWeaponDarkMatterGun::PostSave
================
*/
void rvWeaponDarkMatterGun::PostSave ( void ) {

	//start the ring sounds
	StartSound ( "snd_rings", SND_CHANNEL_VOICE, 0, false, NULL );		
}


/*
================
rvWeaponDarkMatterGun::InitRing
================
*/
void rvWeaponDarkMatterGun::InitRing ( darkMatterRing_t ring, const char* name ) {
	rings[ring].angularVelocity = spawnArgs.GetAngles ( va("ring_%s_velocity", name ) );
	rings[ring].joint = viewModel->GetAnimator()->GetJointHandle ( spawnArgs.GetString ( va("ring_%s_joint", name ) ) );
}

/*
================
rvWeaponDarkMatterGun::StartRings
================
*/
void rvWeaponDarkMatterGun::StartRings ( bool chargeUp ) {
	int i;
	
	if ( ringStartTime == -1 ) {	
		StartSound ( "snd_rings", SND_CHANNEL_VOICE, 0, false, NULL );		
		ringStartTime = gameLocal.time;		
	}
	
	if ( chargeUp ) {	
		coreStartEffect = viewModel->PlayEffect( "fx_core_start", jointCore );	
		for ( i = 0; i < RING_MAX; i ++ ) {
			viewModel->GetAnimator()->SetJointAngularVelocity ( rings[i].joint, rings[i].angularVelocity, gameLocal.time, chargeDuration / 2 );
		}		
	} else if ( !coreEffect ) {
		coreEffect = viewModel->PlayEffect( "fx_core", jointCore, true );	
		for ( i = 0; i < RING_MAX; i ++ ) {
			viewModel->GetAnimator()->SetJointAngularVelocity ( rings[i].joint, rings[i].angularVelocity, gameLocal.time, 0 );
		}
	}
}

/*
================
rvWeaponDarkMatterGun::StopRings
================
*/
void rvWeaponDarkMatterGun::StopRings ( void ) {
	int i;
	
	if ( !viewModel ) {
		return;
	}
	
	viewModel->StopSound ( SND_CHANNEL_VOICE, false );
	
	if ( coreEffect ) {
		coreEffect->Stop ( );
		coreEffect = NULL;
	}
	
	if ( coreStartEffect ) {
		coreStartEffect->Stop ( );
		coreStartEffect = NULL;
	}
	
	for ( i = 0; i < RING_MAX; i ++ ) {
		viewModel->GetAnimator()->ClearJoint ( rings[i].joint );
	}
	
	ringStartTime  = -1;
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponDarkMatterGun )
	STATE ( "Idle",				rvWeaponDarkMatterGun::State_Idle)
	STATE ( "Fire",				rvWeaponDarkMatterGun::State_Fire )
	STATE ( "Reload",			rvWeaponDarkMatterGun::State_Reload )
END_CLASS_STATES

/*
================
rvWeaponDarkMatterGun::State_Idle
================
*/
stateResult_t rvWeaponDarkMatterGun::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !AmmoAvailable ( ) ) {
				SetStatus ( WP_OUTOFAMMO );
			} else {
				SetStatus ( WP_READY );
			}

			// Auto reload?
			if ( !AmmoInClip ( ) && AmmoAvailable () && !clientReload ) {
				SetState ( "reload", 2 );
				return SRESULT_DONE;
			}
			clientReload = false;

			StartRings ( false );

			PlayCycle( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:				

			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}		

			if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoInClip ( ) ) {
				SetState ( "Fire", 0 );
				return SRESULT_DONE;
			}  

			if ( wsfl.netReload ) {
				if ( owner->entityNumber != gameLocal.localClientNum ) {
					SetState ( "Reload", 4 );
					return SRESULT_DONE;			
				} else {
					wsfl.netReload = false;
				}
			}
			
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponDarkMatterGun::State_Fire
================
*/
stateResult_t rvWeaponDarkMatterGun::State_Fire ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			StopRings ( );

			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
			Attack ( false, 1, spread, 0, 1.0f );
			PlayAnim ( ANIMCHANNEL_ALL, "fire", 0 );	
			return SRESULT_STAGE ( STAGE_WAIT );
	
		case STAGE_WAIT:		
			if ( AnimDone ( ANIMCHANNEL_ALL, 2 ) || (gameLocal.isMultiplayer && gameLocal.time >= nextAttackTime) ) {
				SetState ( "Idle", 0 );
				return SRESULT_DONE;
			}		
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponDarkMatterGun::State_Reload
================
*/
stateResult_t rvWeaponDarkMatterGun::State_Reload ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( wsfl.netReload ) {
				wsfl.netReload = false;
			} else {
				NetReload ( );
			}
			
			StartRings ( true );
			
			SetStatus ( WP_RELOAD );
			PlayAnim ( ANIMCHANNEL_ALL, "reload", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				AddToClip ( ClipSize() );
				clientReload = true;
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( gameLocal.isMultiplayer && gameLocal.time > nextAttackTime && wsfl.attack ) {
				AddToClip ( ClipSize() );
				SetStatus ( WP_READY );
				SetState ( "Fire", 4 );
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
===============================================================================

	rvDarkMatterProjectile

===============================================================================
*/

class rvDarkMatterProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( rvDarkMatterProjectile );
	
							rvDarkMatterProjectile	( void );
							~rvDarkMatterProjectile ( void );

	void					Spawn			( void );

	void					Save			( idSaveGame *savefile ) const;
	void					Restore			( idRestoreGame *savefile );

	virtual void			Think			( void );

protected:

	int					nextDamageTime;
	const idDict*		radiusDamageDef;
};

CLASS_DECLARATION( idProjectile, rvDarkMatterProjectile )
END_CLASS

/*
================
rvDarkMatterProjectile::rvDarkMatterProjectile
================
*/
rvDarkMatterProjectile::rvDarkMatterProjectile ( void ) {
	radiusDamageDef = NULL;
}

/*
================
rvDarkMatterProjectile::~rvDarkMatterProjectile
================
*/
rvDarkMatterProjectile::~rvDarkMatterProjectile ( void ) {
}

/*
================
rvDarkMatterProjectile::Spawn
================
*/
void rvDarkMatterProjectile::Spawn ( void ) {
	nextDamageTime  = 0;
	radiusDamageDef = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_radius_damage" ) );
}

/*
================
rvDarkMatterProjectile::Save
================
*/
void rvDarkMatterProjectile::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt ( nextDamageTime );
}

/*
================
rvDarkMatterProjectile::Restore
================
*/
void rvDarkMatterProjectile::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt ( nextDamageTime );
	
	radiusDamageDef = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_radius_damage" ) );
}

/*
================
rvDarkMatterProjectile::Think
================
*/
void rvDarkMatterProjectile::Think ( void ) {
	physicsObj.SetClipMask( MASK_DMGSOLID );
	idProjectile::Think ( );

	if ( gameLocal.time > nextDamageTime ) {
		gameLocal.RadiusDamage ( GetPhysics()->GetOrigin(), this, owner, owner, NULL, spawnArgs.GetString( "def_radius_damage" ), 1.0f, &hitCount );
		nextDamageTime = gameLocal.time + SEC2MS ( spawnArgs.GetFloat ( "damageRate", ".05" ) );	
	}
}


