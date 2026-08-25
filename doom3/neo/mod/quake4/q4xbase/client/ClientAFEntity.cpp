//----------------------------------------------------------------
// ClientAFEntity.cpp
//
// Copyright 2002-2006 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
===============================================================================

rvClientAFEntity

===============================================================================
*/

CLASS_DECLARATION( rvAnimatedClientEntity, rvClientAFEntity )
END_CLASS

static const float BOUNCE_SOUND_MIN_VELOCITY	= 80.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;

/*
================
rvClientAFEntity::rvClientAFEntity
================
*/
rvClientAFEntity::rvClientAFEntity( void ) {
	combatModel = NULL;
	combatModelContents = 0;
	nextSoundTime = 0;
	spawnOrigin.Zero();
	spawnAxis.Identity();
}


/*
================
idAFEntity_Base::~idAFEntity_Base
================
*/
rvClientAFEntity::~rvClientAFEntity( void ) {
	delete		combatModel;
	combatModel = NULL;
}

/*
================
rvClientAFEntity::Spawn
================
*/
void rvClientAFEntity::Spawn( void ) {
	spawnOrigin		= worldOrigin;
	spawnAxis		= worldAxis;
	nextSoundTime	= 0;

	//if ( !LoadAF() ) {
	//	gameLocal.Error( "Couldn't load af file on entity %d", entityNumber );
	//}

	SetCombatModel();

	//af.GetPhysics()->PutToRest();
	//if ( !spawnArgs.GetBool( "nodrop", "0" ) ) {
	//	af.GetPhysics()->Activate();
	//}
}

/*
================
rvClientAFEntity::LoadAF
================
*/
bool rvClientAFEntity::LoadAF( const char* keyname ) {
	idStr fileName;

	if ( !keyname || !*keyname ) {
		keyname = "articulatedFigure";
	}

	if ( !spawnArgs.GetString( keyname, "*unknown*", fileName ) ) {
		return false;
	}

	af.SetAnimator( GetAnimator() );
	
	idDict args = gameLocal.entities[ ENTITYNUM_CLIENT ]->spawnArgs;
	gameLocal.entities[ ENTITYNUM_CLIENT ]->spawnArgs = spawnArgs;

	if ( !af.Load( gameLocal.entities[ ENTITYNUM_CLIENT ], fileName ) ) {
		gameLocal.Error( "rvClientAFEntity::LoadAF: Couldn't load af file '%s' on client entity %d", fileName.c_str(), entityNumber );
	}
	gameLocal.entities[ ENTITYNUM_CLIENT ]->spawnArgs = args;

	af.Start();

	af.GetPhysics()->Rotate( spawnAxis.ToRotation() );
	af.GetPhysics()->Translate( spawnOrigin );

	LoadState( spawnArgs );

	af.UpdateAnimation();
	animator.CreateFrame( gameLocal.time, true );
	UpdateVisuals();

	return true;
}

/*
================
rvClientAFEntity::Think
================
*/
void rvClientAFEntity::Think( void ) {
	RunPhysics();
	UpdateAnimation();

	Present();
	LinkCombat();
}

/*
================
rvClientAFEntity::BodyForClipModelId
================
*/
int rvClientAFEntity::BodyForClipModelId( int id ) const {
	return af.BodyForClipModelId( id );
}

/*
================
rvClientAFEntity::SaveState
================
*/
void rvClientAFEntity::SaveState( idDict &args ) const {
	const idKeyValue *kv;

	// save the ragdoll pose
	af.SaveState( args );

	// save all the bind constraints
	kv = spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	while ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
		kv = spawnArgs.MatchPrefix( "bindConstraint ", kv );
	}

	// save the bind if it exists
	kv = spawnArgs.FindKey( "bind" );
	if ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
	}
	kv = spawnArgs.FindKey( "bindToJoint" );
	if ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
	}
	kv = spawnArgs.FindKey( "bindToBody" );
	if ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
	}
}

/*
================
rvClientAFEntity::LoadState
================
*/
void rvClientAFEntity::LoadState( const idDict &args ) {
	af.LoadState( args );
}

/*
================
rvClientAFEntity::AddBindConstraints
================
*/
void rvClientAFEntity::AddBindConstraints( void ) {
	af.AddBindConstraints();
}

/*
================
rvClientAFEntity::RemoveBindConstraints
================
*/
void rvClientAFEntity::RemoveBindConstraints( void ) {
	af.RemoveBindConstraints();
}

/*
================
rvClientAFEntity::GetImpactInfo
================
*/
void rvClientAFEntity::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	if ( af.IsActive() ) {
		af.GetImpactInfo( ent, id, point, info );
	} else {
		rvClientEntity::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
rvClientAFEntity::ApplyImpulse
================
*/
void rvClientAFEntity::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash ) {
	if ( af.IsLoaded() ) {
		af.ApplyImpulse( ent, id, point, impulse );
	}
	if ( !af.IsActive() ) {
		rvClientEntity::ApplyImpulse( ent, id, point, impulse, splash );
	}
}

/*
================
rvClientAFEntity::AddForce
================
*/
void rvClientAFEntity::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	if ( af.IsLoaded() ) {
		af.AddForce( ent, id, point, force );
	}
	if ( !af.IsActive() ) {
		rvClientEntity::AddForce( ent, id, point, force );
	}
}

bool rvClientAFEntity::CanPlayImpactEffect ( idEntity* attacker, idEntity* target ) {
	// stubbed out
	return true;
}

/*
================
rvClientAFEntity::Collide
================
*/
bool rvClientAFEntity::Collide( const trace_t &collision, const idVec3 &velocity ) {
	float v, f;

	if ( af.IsActive() ) {
		v = -( velocity * collision.c.normal );
		if ( v > BOUNCE_SOUND_MIN_VELOCITY && gameLocal.time > nextSoundTime ) {
			// RAVEN BEGIN
			// jscott: fixed negative sqrt call
			if( v > BOUNCE_SOUND_MAX_VELOCITY ) {
				f = 1.0f;
			} else if( v <= BOUNCE_SOUND_MIN_VELOCITY ) {
				f = 0.0f;
			} else {
				f = ( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / ( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
			}
			// RAVEN END
			if ( StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, false, NULL ) ) {
				// don't set the volume unless there is a bounce sound as it overrides the entire channel
				// which causes footsteps on ai's to not honor their shader parms
				SetSoundVolume( f );
			}
			nextSoundTime = gameLocal.time + 500;
		}
	}

	return false;
}

/*
================
rvClientAFEntity::GetPhysicsToVisualTransform
================
*/
bool rvClientAFEntity::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	return rvClientModel::GetPhysicsToVisualTransform( origin, axis );
}

/*
================
rvClientAFEntity::UpdateAnimationControllers
================
*/
bool rvClientAFEntity::UpdateAnimationControllers( void ) {
	if ( af.IsActive() ) {
		if ( af.UpdateAnimation() ) {
			return true;
		}
	}
	return false;
}

/*
================
rvClientAFEntity::SetCombatModel
================
*/
void rvClientAFEntity::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( entityDefHandle );
	} else {
		RV_PUSH_HEAP_MEM(this);
		combatModel = new idClipModel( entityDefHandle );
		RV_POP_HEAP();
	}
}

/*
================
rvClientAFEntity::GetCombatModel
================
*/
idClipModel *rvClientAFEntity::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
rvClientAFEntity::SetCombatContents
================
*/
void rvClientAFEntity::SetCombatContents( bool enable ) {
	assert( combatModel );
	if ( enable && combatModelContents ) {
		assert( !combatModel->GetContents() );
		combatModel->SetContents( combatModelContents );
		combatModelContents = 0;
	} else if ( !enable && combatModel->GetContents() ) {
		assert( !combatModelContents );
		combatModelContents = combatModel->GetContents();
		combatModel->SetContents( 0 );
	}
}

/*
================
rvClientAFEntity::LinkCombat
================
*/
void rvClientAFEntity::LinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Link( gameLocal.entities[ ENTITYNUM_CLIENT ], 0, renderEntity.origin, renderEntity.axis, entityDefHandle );
	}
}

/*
================
rvClientAFEntity::UnlinkCombat
================
*/
void rvClientAFEntity::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}

/*
================
rvClientAFEntity::FreeEntityDef
================
*/
void rvClientAFEntity::FreeEntityDef( void ) {
	UnlinkCombat();
	rvClientEntity::FreeEntityDef();
}

/*
================
rvClientAFEntity::GetNoPlayerImpactFX
================
*/
bool rvClientAFEntity::GetNoPlayerImpactFX( void ) {
	// stubbed out
	return false;
}


/*
===============================================================================

  idAFAttachment

===============================================================================
*/

CLASS_DECLARATION( rvClientAFEntity, rvClientAFAttachment )
END_CLASS

/*
=====================
rvClientAFAttachment::rvClientAFAttachment
=====================
*/
rvClientAFAttachment::rvClientAFAttachment( void ) {
	body			= NULL;
	combatModel		= NULL;
	idleAnim		= 0;
	damageJoint		= INVALID_JOINT;
}

/*
=====================
rvClientAFAttachment::~rvClientAFAttachment
=====================
*/
rvClientAFAttachment::~rvClientAFAttachment( void ) {
	delete combatModel;
	combatModel = NULL;
}

/*
=====================
rvClientAFAttachment::Spawn
=====================
*/
void rvClientAFAttachment::Spawn( void ) {
	idleAnim = animator.GetAnim( "idle" );
}

/*
=====================
rvClientAFAttachment::InitCopyJoints
=====================
*/
void rvClientAFAttachment::InitCopyJoints ( void ) {
	copyJoints_t		copyJoint;
	const idKeyValue*	kv;
	const char*			jointName;
	idAnimator*			bodyAnimator;

	if ( !body ) {
		return;
	}
	
	bodyAnimator = body->GetAnimator ( );

	// set up the list of joints to copy to the head
	for( kv = spawnArgs.MatchPrefix( "copy_joint", NULL ); kv != NULL; kv = spawnArgs.MatchPrefix( "copy_joint", kv ) ) {
		if ( kv->GetValue() == "" ) {
			// probably clearing out inherited key, so skip it
			continue;
		}

		if ( !body->spawnArgs.GetString ( va("copy_joint_world %s", kv->GetValue().c_str() ), kv->GetValue().c_str(), &jointName ) ) {
			copyJoint.mod = JOINTMOD_LOCAL_OVERRIDE;			
			body->spawnArgs.GetString ( va("copy_joint %s", kv->GetValue().c_str() ), kv->GetValue().c_str(), &jointName );
		} else {
			copyJoint.mod = JOINTMOD_WORLD_OVERRIDE;
		}
		
		copyJoint.from = bodyAnimator->GetJointHandle ( jointName );
		if ( copyJoint.from == INVALID_JOINT ) {
			gameLocal.Warning( "Unknown copy_joint '%s' on client entity %d", jointName, entityNumber );
			continue;
		}

		copyJoint.to = animator.GetJointHandle( kv->GetValue() );
		if ( copyJoint.to == INVALID_JOINT ) {
			gameLocal.Warning( "Unknown copy_joint '%s' on head of entity %d", kv->GetValue().c_str(), entityNumber );
			continue;
		}

		copyJoints.Append( copyJoint );
	}
}

/*
=====================
rvClientAFAttachment::CopyJointsFromBody
=====================
*/
void rvClientAFAttachment::CopyJointsFromBody ( void ) {
	MEM_SCOPED_TAG(tag,MA_ANIM);

	idAnimator*	bodyAnimator;
	int			i;
	idMat3		mat;
	idMat3		axis;
	idVec3		pos;
	
	if ( !body ) {
		return;
	}
	bodyAnimator = body->GetAnimator();

	// copy the animation from the body to the head
	for( i = 0; i < copyJoints.Num(); i++ ) {
		if ( copyJoints[ i ].mod == JOINTMOD_WORLD_OVERRIDE ) {
			mat = GetPhysics()->GetAxis().Transpose();
			body->GetJointWorldTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			pos -= GetPhysics()->GetOrigin();
			animator.SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos * mat );
			animator.SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis * mat );
		} else {
			bodyAnimator->GetJointLocalTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			animator.SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos );
			animator.SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis );
		}
	}	
}

/*
=====================
rvClientAFAttachment::SetBody
=====================
*/
void rvClientAFAttachment::SetBody( idAnimatedEntity *bodyEnt, const char *model, jointHandle_t _damageJoint ) {
	body = bodyEnt;
	damageJoint = _damageJoint;
	SetModel( model );

	spawnArgs.SetBool( "bleed", body->spawnArgs.GetBool( "bleed" ) );
}

/*
=====================
rvClientAFAttachment::ClearBody
=====================
*/
void rvClientAFAttachment::ClearBody( void ) {
	body = NULL;
	damageJoint = INVALID_JOINT;
	Hide();
}

/*
=====================
rvClientAFAttachment::GetBody
=====================
*/
idEntity *rvClientAFAttachment::GetBody( void ) const {
	return body;
}

/*
================
idAFAttachment::Hide
================
*/
void rvClientAFAttachment::Hide( void ) {
	UnlinkCombat();
}

/*
================
idAFAttachment::Show
================
*/
void rvClientAFAttachment::Show( void ) {
	LinkCombat();
}

/*
================
idAFAttachment::AddDamageEffect
================
*/
void rvClientAFAttachment::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	if ( body ) {
		trace_t c = collision;
		c.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( damageJoint );
		body->AddDamageEffect( c, velocity, damageDefName, inflictor );
	}
}

/*
================
rvClientAFAttachment::GetImpactInfo
================
*/
void rvClientAFAttachment::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	if ( body ) {
		body->GetImpactInfo( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( damageJoint ), point, info );
	} else {
		rvClientAFAttachment::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
rvClientAFAttachment::CanPlayImpactEffect
================
*/
bool rvClientAFAttachment::CanPlayImpactEffect ( idEntity* attacker, idEntity* target ) {
	return rvClientAFEntity::CanPlayImpactEffect( attacker, target );
}

/*
================
rvClientAFAttachment::ApplyImpulse
================
*/
void rvClientAFAttachment::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash ) {
	if ( body ) {
		body->ApplyImpulse( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( damageJoint ), point, impulse );
	} else {
		rvClientAFEntity::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
rvClientAFAttachment::AddForce
================
*/
void rvClientAFAttachment::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	if ( body ) {
		body->AddForce( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( damageJoint ), point, force );
	} else {
		rvClientAFEntity::AddForce( ent, id, point, force );
	}
}

/*
================
rvClientAFAttachment::PlayIdleAnim
================
*/
void rvClientAFAttachment::PlayIdleAnim( int channel, int blendTime ) {
	if ( idleAnim && ( idleAnim != animator.CurrentAnim( channel )->AnimNum() ) ) {
		animator.CycleAnim( channel, idleAnim, gameLocal.time, blendTime );
	}
}

/*
================
rvClientAFAttachment::Think
================
*/
void rvClientAFAttachment::Think( void ) {
	rvClientAFEntity::Think();
}

/*
================
rvClientAFAttachment::UpdateAnimationControllers
================
*/
bool rvClientAFAttachment::UpdateAnimationControllers( void ) {
	CopyJointsFromBody( );
	return rvClientAFEntity::UpdateAnimationControllers( );
}

/*
================
rvClientAFAttachment::SetCombatModel
================
*/
void rvClientAFAttachment::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( entityDefHandle );
	} else {
		RV_PUSH_HEAP_MEM(this);
		combatModel = new idClipModel( entityDefHandle );
		RV_POP_HEAP();
	}
	combatModel->SetOwner( body );
}

/*
================
rvClientAFAttachment::GetCombatModel
================
*/
idClipModel *rvClientAFAttachment::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
rvClientAFAttachment::LinkCombat
================
*/
void rvClientAFAttachment::LinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Link( gameLocal.entities[ ENTITYNUM_CLIENT ], 0, renderEntity.origin, renderEntity.axis, entityDefHandle );
	}
}

/*
================
rvClientAFAttachment::UnlinkCombat
================
*/
void rvClientAFAttachment::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}
