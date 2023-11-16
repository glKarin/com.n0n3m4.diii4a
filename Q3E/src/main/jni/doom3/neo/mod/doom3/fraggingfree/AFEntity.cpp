/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/geometry/JointTransform.h"
#include "renderer/ModelManager.h"

#include "gamesys/SysCvar.h"
#include "Item.h"
#include "Player.h"
#include "Fx.h"
#include "SmokeParticles.h"
#include "WorldSpawn.h"
#include "Moveable.h"

#include "AFEntity.h"

/*
===============================================================================

  idMultiModelAF

===============================================================================
*/

CLASS_DECLARATION( idEntity, idMultiModelAF )
END_CLASS

/*
================
idMultiModelAF::Spawn
================
*/
void idMultiModelAF::Spawn( void ) {
	physicsObj.SetSelf( this );
}

/*
================
idMultiModelAF::~idMultiModelAF
================
*/
idMultiModelAF::~idMultiModelAF( void ) {
	int i;

	for ( i = 0; i < modelDefHandles.Num(); i++ ) {
		if ( modelDefHandles[i] != -1 ) {
			gameRenderWorld->FreeEntityDef( modelDefHandles[i] );
			modelDefHandles[i] = -1;
		}
	}
}

/*
================
idMultiModelAF::SetModelForId
================
*/
void idMultiModelAF::SetModelForId( int id, const idStr &modelName ) {
	modelHandles.AssureSize( id+1, NULL );
	modelDefHandles.AssureSize( id+1, -1 );
	modelHandles[id] = renderModelManager->FindModel( modelName );
}

/*
================
idMultiModelAF::Present
================
*/
void idMultiModelAF::Present( void ) {
	int i;

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	for ( i = 0; i < modelHandles.Num(); i++ ) {

		if ( !modelHandles[i] ) {
			continue;
		}

		renderEntity.origin = physicsObj.GetOrigin( i );
		renderEntity.axis = physicsObj.GetAxis( i );
		renderEntity.hModel = modelHandles[i];
		renderEntity.bodyId = i;

		// add to refresh list
		if ( modelDefHandles[i] == -1 ) {
			modelDefHandles[i] = gameRenderWorld->AddEntityDef( &renderEntity );
		} else {
			gameRenderWorld->UpdateEntityDef( modelDefHandles[i], &renderEntity );
		}
	}
}

/*
================
idMultiModelAF::Think
================
*/
void idMultiModelAF::Think( void ) {
	RunPhysics();
	Present();

#ifdef _DENTONMOD
	if ( thinkFlags & TH_UPDATEWOUNDPARTICLES )
		UpdateParticles();
#endif
}


/*
===============================================================================

  idChain

===============================================================================
*/

CLASS_DECLARATION( idMultiModelAF, idChain )
END_CLASS

/*
================
idChain::BuildChain

  builds a chain hanging down from the ceiling
  the highest link is a child of the link below it etc.
  this allows an object to be attached to multiple chains while keeping a single tree structure
================
*/
void idChain::BuildChain( const idStr &name, const idVec3 &origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld ) {
	int i;
	float halfLinkLength = linkLength * 0.5f;
	idTraceModel trm;
	idClipModel *clip;
	idAFBody *body, *lastBody;
	idAFConstraint_BallAndSocketJoint *bsj;
	idAFConstraint_UniversalJoint *uj;
	idVec3 org;

	// create a trace model
	trm = idTraceModel( linkLength, linkWidth );
	trm.Translate( -trm.offset );

	org = origin - idVec3( 0, 0, halfLinkLength );

	lastBody = NULL;
	for ( i = 0; i < numLinks; i++ ) {

		// add body
		clip = new idClipModel( trm );
		clip->SetContents( CONTENTS_SOLID );
		clip->Link( gameLocal.clip, this, 0, org, mat3_identity );
		body = new idAFBody( name + idStr(i), clip, density );
		physicsObj.AddBody( body );

		// visual model for body
		SetModelForId( physicsObj.GetBodyId( body ), spawnArgs.GetString( "model" ) );

		// add constraint
		if ( bindToWorld ) {
			if ( !lastBody ) {
				uj = new idAFConstraint_UniversalJoint( name + idStr(i), body, lastBody );
				uj->SetShafts( idVec3( 0, 0, -1 ), idVec3( 0, 0, 1 ) );
				//uj->SetConeLimit( idVec3( 0, 0, -1 ), 30.0f );
				//uj->SetPyramidLimit( idVec3( 0, 0, -1 ), idVec3( 1, 0, 0 ), 90.0f, 30.0f );
			}
			else {
				uj = new idAFConstraint_UniversalJoint( name + idStr(i), lastBody, body );
				uj->SetShafts( idVec3( 0, 0, 1 ), idVec3( 0, 0, -1 ) );
				//uj->SetConeLimit( idVec3( 0, 0, 1 ), 30.0f );
			}
			uj->SetAnchor( org + idVec3( 0, 0, halfLinkLength ) );
			uj->SetFriction( 0.9f );
			physicsObj.AddConstraint( uj );
		}
		else {
			if ( lastBody ) {
				bsj = new idAFConstraint_BallAndSocketJoint( "joint" + idStr(i), lastBody, body );
				bsj->SetAnchor( org + idVec3( 0, 0, halfLinkLength ) );
				bsj->SetConeLimit( idVec3( 0, 0, 1 ), 60.0f, idVec3( 0, 0, 1 ) );
				physicsObj.AddConstraint( bsj );
			}
		}

		org[2] -= linkLength;

		lastBody = body;
	}
}

/*
================
idChain::Spawn
================
*/
void idChain::Spawn( void ) {
	int numLinks;
	float length, linkLength, linkWidth, density;
	bool drop;
	idVec3 origin;

	spawnArgs.GetBool( "drop", "0", drop );
	spawnArgs.GetInt( "links", "3", numLinks );
	spawnArgs.GetFloat( "length", idStr( numLinks * 32.0f ), length );
	spawnArgs.GetFloat( "width", "8", linkWidth );
	spawnArgs.GetFloat( "density", "0.2", density );
	linkLength = length / numLinks;
	origin = GetPhysics()->GetOrigin();

	// initialize physics
	physicsObj.SetSelf( this );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY );
	SetPhysics( &physicsObj );

	BuildChain( "link", origin, linkLength, linkWidth, density, numLinks, !drop );
}

/*
===============================================================================

  idAFAttachment

===============================================================================
*/

CLASS_DECLARATION( idAnimatedEntity, idAFAttachment )
END_CLASS

/*
=====================
idAFAttachment::idAFAttachment
=====================
*/
idAFAttachment::idAFAttachment( void ) {
	body			= NULL;
	combatModel		= NULL;
	idleAnim		= 0;
	attachJoint		= INVALID_JOINT;
}

/*
=====================
idAFAttachment::~idAFAttachment
=====================
*/
idAFAttachment::~idAFAttachment( void ) {

	StopSound( SND_CHANNEL_ANY, false );

	delete combatModel;
	combatModel = NULL;
}

/*
=====================
idAFAttachment::Spawn
=====================
*/
void idAFAttachment::Spawn( void ) {
	idleAnim = animator.GetAnim( "idle" );
}

/*
=====================
idAFAttachment::SetBody
=====================
*/
void idAFAttachment::SetBody( idEntity *bodyEnt, const char *model, jointHandle_t attachJoint ) {
	bool bleed;

	body = bodyEnt;
	this->attachJoint = attachJoint;
	SetModel( model );
	fl.takedamage = true;

	bleed = body->spawnArgs.GetBool( "bleed" );
	spawnArgs.SetBool( "bleed", bleed );
}

/*
=====================
idAFAttachment::ClearBody
=====================
*/
void idAFAttachment::ClearBody( void ) {
	body = NULL;
	attachJoint = INVALID_JOINT;
	Hide();
}

/*
=====================
idAFAttachment::GetBody
=====================
*/
idEntity *idAFAttachment::GetBody( void ) const {
	return body;
}

/*
================
idAFAttachment::Save

archive object for savegame file
================
*/
void idAFAttachment::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( body );
	savefile->WriteInt( idleAnim );
	savefile->WriteJoint( attachJoint );
}

/*
================
idAFAttachment::Restore

unarchives object from save game file
================
*/
void idAFAttachment::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( body ) );
	savefile->ReadInt( idleAnim );
	savefile->ReadJoint( attachJoint );

	SetCombatModel();
	LinkCombat();
}

/*
================
idAFAttachment::Hide
================
*/
void idAFAttachment::Hide( void ) {
	idEntity::Hide();
	UnlinkCombat();
}

/*
================
idAFAttachment::Show
================
*/
void idAFAttachment::Show( void ) {
	idEntity::Show();
	LinkCombat();
}

/*
============
idAFAttachment::Damage

Pass damage to body at the bindjoint
============
*/
void idAFAttachment::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
	const char *damageDefName, const float damageScale, const int location ) {

	if ( body ) {
		body->Damage( inflictor, attacker, dir, damageDefName, damageScale, attachJoint );
	}
}

/*
================
idAFAttachment::AddDamageEffect
================
*/
#ifdef _DENTONMOD
void idAFAttachment::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity *soundEnt ) {
#else
void idAFAttachment::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) {
#endif
	if ( body ) {
		trace_t c = collision;
		c.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint );
#ifdef _DENTONMOD
		body->AddDamageEffect( c, velocity, damageDefName, soundEnt );
#else
		body->AddDamageEffect( c, velocity, damageDefName );
#endif
	}
}

/*
================
idAFAttachment::GetImpactInfo
================
*/
void idAFAttachment::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	if ( body ) {
		body->GetImpactInfo( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, info );
	} else {
		idEntity::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
idAFAttachment::ApplyImpulse
================
*/
void idAFAttachment::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( body ) {
		body->ApplyImpulse( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, impulse );
	} else {
		idEntity::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
idAFAttachment::AddForce
================
*/
void idAFAttachment::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	if ( body ) {
		body->AddForce( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, force );
	} else {
		idEntity::AddForce( ent, id, point, force );
	}
}

/*
================
idAFAttachment::PlayIdleAnim
================
*/
void idAFAttachment::PlayIdleAnim( int blendTime ) {
	if ( idleAnim && ( idleAnim != animator.CurrentAnim( ANIMCHANNEL_ALL )->AnimNum() ) ) {
		animator.CycleAnim( ANIMCHANNEL_ALL, idleAnim, gameLocal.time, blendTime );
	}
}

/*
================
idAfAttachment::Think
================
*/
void idAFAttachment::Think( void ) {
	idAnimatedEntity::Think();
	if ( thinkFlags & TH_UPDATEPARTICLES ) {
		UpdateDamageEffects();
	}
}

/*
================
idAFAttachment::SetCombatModel
================
*/
void idAFAttachment::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( modelDefHandle );
	} else {
		combatModel = new idClipModel( modelDefHandle );
	}
	combatModel->SetOwner( body );
}

/*
================
idAFAttachment::GetCombatModel
================
*/
idClipModel *idAFAttachment::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
idAFAttachment::LinkCombat
================
*/
void idAFAttachment::LinkCombat( void ) {
	if ( fl.hidden ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
}

/*
================
idAFAttachment::UnlinkCombat
================
*/
void idAFAttachment::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}


/*
===============================================================================

  idAFEntity_Base

===============================================================================
*/

const idEventDef EV_SetConstraintPosition( "SetConstraintPosition", "sv" );

CLASS_DECLARATION( idAnimatedEntity, idAFEntity_Base )
	EVENT( EV_SetConstraintPosition,	idAFEntity_Base::Event_SetConstraintPosition )
END_CLASS

static const float BOUNCE_SOUND_MIN_VELOCITY	= 80.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;

/*
================
idAFEntity_Base::idAFEntity_Base
================
*/
idAFEntity_Base::idAFEntity_Base( void ) {
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
idAFEntity_Base::~idAFEntity_Base( void ) {
	delete combatModel;
	combatModel = NULL;
}

/*
================
idAFEntity_Base::Save
================
*/
void idAFEntity_Base::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( combatModelContents );
	savefile->WriteClipModel( combatModel );
	savefile->WriteVec3( spawnOrigin );
	savefile->WriteMat3( spawnAxis );
	savefile->WriteInt( nextSoundTime );
	af.Save( savefile );
}

/*
================
idAFEntity_Base::Restore
================
*/
void idAFEntity_Base::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( combatModelContents );
	savefile->ReadClipModel( combatModel );
	savefile->ReadVec3( spawnOrigin );
	savefile->ReadMat3( spawnAxis );
	savefile->ReadInt( nextSoundTime );
	LinkCombat();

	af.Restore( savefile );
}

//ivan start
void idAFEntity_Base::RecreateDynamicConstraints( idList<idAFConstraint *> *constraints ){
	//meant to be overridden
}
//ivan end

/*
================
idAFEntity_Base::Spawn
================
*/
void idAFEntity_Base::Spawn( void ) {
	spawnOrigin = GetPhysics()->GetOrigin();
	spawnAxis = GetPhysics()->GetAxis();
	nextSoundTime = 0;
}

/*
================
idAFEntity_Base::LoadAF
================
*/
bool idAFEntity_Base::LoadAF( void ) {
	idStr fileName;

	if ( !spawnArgs.GetString( "articulatedFigure", "*unknown*", fileName ) ) {
		return false;
	}

	af.SetAnimator( GetAnimator() );
	if ( !af.Load( this, fileName ) ) {
		gameLocal.Error( "idAFEntity_Base::LoadAF: Couldn't load af file '%s' on entity '%s'", fileName.c_str(), name.c_str() );
	}

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
idAFEntity_Base::Think
================
*/
void idAFEntity_Base::Think( void ) {
	RunPhysics();
	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
#ifdef _DENTONMOD
	if (thinkFlags & TH_UPDATEWOUNDPARTICLES)
		UpdateDamageEffects();
#endif
}

/*
================
idAFEntity_Base::BodyForClipModelId
================
*/
int idAFEntity_Base::BodyForClipModelId( int id ) const {
	return af.BodyForClipModelId( id );
}

/*
================
idAFEntity_Base::SaveState
================
*/
void idAFEntity_Base::SaveState( idDict &args ) const {
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
idAFEntity_Base::LoadState
================
*/
void idAFEntity_Base::LoadState( const idDict &args ) {
	af.LoadState( args );
}

/*
================
idAFEntity_Base::AddBindConstraints
================
*/
void idAFEntity_Base::AddBindConstraints( void ) {
	af.AddBindConstraints();
}

/*
================
idAFEntity_Base::RemoveBindConstraints
================
*/
void idAFEntity_Base::RemoveBindConstraints( void ) {
	af.RemoveBindConstraints();
}

/*
================
idAFEntity_Base::GetImpactInfo
================
*/
void idAFEntity_Base::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	if ( af.IsActive() ) {
		af.GetImpactInfo( ent, id, point, info );
	} else {
		idEntity::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
idAFEntity_Base::ApplyImpulse
================
*/
void idAFEntity_Base::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( af.IsLoaded() ) {
		af.ApplyImpulse( ent, id, point, impulse );
	}
	if ( !af.IsActive() ) {
		idEntity::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
idAFEntity_Base::AddForce
================
*/
void idAFEntity_Base::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	if ( af.IsLoaded() ) {
		af.AddForce( ent, id, point, force );
	}
	if ( !af.IsActive() ) {
		idEntity::AddForce( ent, id, point, force );
	}
}

/*
================
idAFEntity_Base::Collide
================
*/
bool idAFEntity_Base::Collide( const trace_t &collision, const idVec3 &velocity ) {
	float v, f;

	if ( af.IsActive() ) {
		v = -( velocity * collision.c.normal );
		if ( v > BOUNCE_SOUND_MIN_VELOCITY && gameLocal.time > nextSoundTime ) {
			f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
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
idAFEntity_Base::GetPhysicsToVisualTransform
================
*/
bool idAFEntity_Base::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	return idEntity::GetPhysicsToVisualTransform( origin, axis );
}

/*
================
idAFEntity_Base::UpdateAnimationControllers
================
*/
bool idAFEntity_Base::UpdateAnimationControllers( void ) {
	if ( af.IsActive() ) {
		if ( af.UpdateAnimation() ) {
			return true;
		}
	}
	return false;
}

/*
================
idAFEntity_Base::SetCombatModel
================
*/
void idAFEntity_Base::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( modelDefHandle );
	} else {
		combatModel = new idClipModel( modelDefHandle );
	}
}

/*
================
idAFEntity_Base::GetCombatModel
================
*/
idClipModel *idAFEntity_Base::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
idAFEntity_Base::SetCombatContents
================
*/
void idAFEntity_Base::SetCombatContents( bool enable ) {
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
idAFEntity_Base::LinkCombat
================
*/
void idAFEntity_Base::LinkCombat( void ) {
	if ( fl.hidden ) {
		return;
	}
	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
}

/*
================
idAFEntity_Base::UnlinkCombat
================
*/
void idAFEntity_Base::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}

/*
================
idAFEntity_Base::FreeModelDef
================
*/
void idAFEntity_Base::FreeModelDef( void ) {
	UnlinkCombat();
	idEntity::FreeModelDef();
}

/*
===============
idAFEntity_Base::ShowEditingDialog
===============
*/
void idAFEntity_Base::ShowEditingDialog( void ) {
	common->InitTool( EDITOR_AF, &spawnArgs );
}

/*
================
idAFEntity_Base::DropAFs

  The entity should have the following key/value pairs set:
	"def_drop<type>AF"		"af def"
	"drop<type>Skin"		"skin name"
  To drop multiple articulated figures the following key/value pairs can be used:
	"def_drop<type>AF*"		"af def"
  where * is an aribtrary string.
================
*/
void idAFEntity_Base::DropAFs( idEntity *ent, const char *type, idList<idEntity *> *list ) {
	const idKeyValue *kv;
	const char *skinName;
	idEntity *newEnt;
	idAFEntity_Base *af;
	idDict args;
	const idDeclSkin *skin;

	// drop the articulated figures
	kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sAF", type ), NULL );
	while ( kv ) {

		args.Set( "classname", kv->GetValue() );
		gameLocal.SpawnEntityDef( args, &newEnt );

		if ( newEnt && newEnt->IsType( idAFEntity_Base::Type ) ) {
			af = static_cast<idAFEntity_Base *>(newEnt);
			af->GetPhysics()->SetOrigin( ent->GetPhysics()->GetOrigin() );
			af->GetPhysics()->SetAxis( ent->GetPhysics()->GetAxis() );
			af->af.SetupPose( ent, gameLocal.time );
			if ( list ) {
				list->Append( af );
			}
		}

		kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sAF", type ), kv );
	}

	// change the skin to hide all the dropped articulated figures
	skinName = ent->spawnArgs.GetString( va( "skin_drop%s", type ) );
	if ( skinName[0] ) {
		skin = declManager->FindSkin( skinName );
		ent->SetSkin( skin );
	}
}

/*
================
idAFEntity_Base::Event_SetConstraintPosition
================
*/
void idAFEntity_Base::Event_SetConstraintPosition( const char *name, const idVec3 &pos ) {
	af.SetConstraintPosition( name, pos );
}

/*
===============================================================================

idAFEntity_Gibbable

===============================================================================
*/

const idEventDef EV_Gib( "gib", "s" );
const idEventDef EV_Gibbed( "<gibbed>" );

CLASS_DECLARATION( idAFEntity_Base, idAFEntity_Gibbable )
	EVENT( EV_Gib,		idAFEntity_Gibbable::Event_Gib )
	EVENT( EV_Gibbed,	idAFEntity_Base::Event_Remove )
END_CLASS


/*
================
idAFEntity_Gibbable::idAFEntity_Gibbable
================
*/
idAFEntity_Gibbable::idAFEntity_Gibbable( void ) {
	skeletonModel = NULL;
	skeletonModelDefHandle = -1;
	gibbed = false;
#ifdef _D3XP
	wasThrown = false;
#endif
	//ivan start - dmgfx
	dmgFxEntities.Clear();
	allowDmgfxs = false;
	//ivan end	
}

/*
================
idAFEntity_Gibbable::~idAFEntity_Gibbable
================
*/
idAFEntity_Gibbable::~idAFEntity_Gibbable() {
	if ( skeletonModelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( skeletonModelDefHandle );
		skeletonModelDefHandle = -1;
	}
}

/*
================
idAFEntity_Gibbable::Save
================
*/
void idAFEntity_Gibbable::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( gibbed );
	savefile->WriteBool( combatModel != NULL );
#ifdef _D3XP
	savefile->WriteBool( wasThrown );
#endif

	//ivan start - dmgfx
	int i;
	savefile->WriteBool( allowDmgfxs );
	savefile->WriteInt( dmgFxEntities.Num() );
	for( i = 0; i < dmgFxEntities.Num(); i++ ) {
		dmgFxEntities[ i ].Save( savefile );
	}
	//ivan end
}

/*
================
idAFEntity_Gibbable::Restore
================
*/
void idAFEntity_Gibbable::Restore( idRestoreGame *savefile ) {
	bool hasCombatModel;

	savefile->ReadBool( gibbed );
	savefile->ReadBool( hasCombatModel );
#ifdef _D3XP
	savefile->ReadBool( wasThrown );
#endif

	//ivan start - dmgfx
	int i, num;
	savefile->ReadBool( allowDmgfxs );

	dmgFxEntities.Clear();
	savefile->ReadInt( num );
	dmgFxEntities.SetNum( num );
	for( i = 0; i < num; i++ ) {
		dmgFxEntities[ i ].Restore( savefile );
	}
	//ivan end

	InitSkeletonModel();

	if ( hasCombatModel ) {
		SetCombatModel();
		LinkCombat();
	}
}

/*
================
idAFEntity_Gibbable::Spawn
================
*/
void idAFEntity_Gibbable::Spawn( void ) {
	InitSkeletonModel();

	gibbed = false;
#ifdef _D3XP
	wasThrown = false;
#endif

	//ivan start - dmgfx
	spawnArgs.GetBool( "allowDmgfxs", "0", allowDmgfxs );
	//ivan end
}

/*
================
idAFEntity_Gibbable::InitSkeletonModel
================
*/
void idAFEntity_Gibbable::InitSkeletonModel( void ) {
	const char *modelName;
	const idDeclModelDef *modelDef;

	skeletonModel = NULL;
	skeletonModelDefHandle = -1;

	modelName = spawnArgs.GetString( "model_gib" );

	modelDef = NULL;
	if ( modelName[0] != '\0' ) {
		modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
		if ( modelDef ) {
			skeletonModel = modelDef->ModelHandle();
		} else {
			skeletonModel = renderModelManager->FindModel( modelName );
		}
		if ( skeletonModel != NULL && renderEntity.hModel != NULL ) {
			if ( skeletonModel->NumJoints() != renderEntity.hModel->NumJoints() ) {
				gameLocal.Error( "gib model '%s' has different number of joints than model '%s'",
									skeletonModel->Name(), renderEntity.hModel->Name() );
			}
		}
	}
}

/*
================
idAFEntity_Gibbable::Present
================
*/
void idAFEntity_Gibbable::Present( void ) {
	renderEntity_t skeleton;

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		//gameLocal.Printf("entity hasn't changed\n");
		return;
	}

	// update skeleton model
	if ( gibbed && !IsHidden() && skeletonModel != NULL ) {
		skeleton = renderEntity;
		skeleton.hModel = skeletonModel;
		// add to refresh list
		if ( skeletonModelDefHandle == -1 ) {
			skeletonModelDefHandle = gameRenderWorld->AddEntityDef( &skeleton );
		} else {
			gameRenderWorld->UpdateEntityDef( skeletonModelDefHandle, &skeleton );
		}
	}

	idEntity::Present();
}

/*
============
idAFEntity_Gibbable::Killed
============
*/
void idAFEntity_Gibbable::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	//notify dmgFx
	if( health + damage > 0 ) { //killed by this damage
		if( allowDmgfxs && !gibbed && dmgFxEntities.Num() > 0 ){
			KilledDamageFxs(); //this may restart or stop the fx
		}
	}
}

/*
================
idAFEntity_Gibbable::Damage
================
*/
void idAFEntity_Gibbable::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( !fl.takedamage ) {
		return;
	}
	idAFEntity_Base::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	if ( health < -20 && spawnArgs.GetBool( "gib" ) ) {
		Gib( dir, damageDefName );
	}

	//ff1.3 start - damaging fx
	if ( allowDmgfxs && !gibbed ) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
		if ( !damageDef ) {
			gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
		}
		CheckDamageFx( damageDef, attacker ); 
	}
	//ff1.3 end
}

#ifdef _D3XP
/*
=====================
idAFEntity_Gibbable::SetThrown
=====================
*/
void idAFEntity_Gibbable::SetThrown( bool isThrown ) {

	if ( isThrown ) {
		int i, num = af.GetPhysics()->GetNumBodies();

		for ( i=0; i<num; i++ ) {
			idAFBody *body;

			body = af.GetPhysics()->GetBody( i );
			body->SetClipMask( MASK_MONSTERSOLID );
		}
	}

	wasThrown = isThrown;
}

/*
=====================
idAFEntity_Gibbable::Collide
=====================
*/
bool idAFEntity_Gibbable::Collide( const trace_t &collision, const idVec3 &velocity ) {

	if ( !gibbed && wasThrown ) {

		// Everything gibs (if possible)
		if ( spawnArgs.GetBool( "gib" ) ) {
			idEntity	*ent;

			ent = gameLocal.entities[ collision.c.entityNum ];
			if ( ent->fl.takedamage ) {
				ent->Damage( this, gameLocal.GetLocalPlayer(), collision.c.normal, "damage_thrown_ragdoll", 1.f, CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id ) );
			}

			idVec3 vel = velocity;
			vel.NormalizeFast();
			Gib( vel, "damage_gib" );
		}
	}

	return idAFEntity_Base::Collide( collision, velocity );
}
#endif

/*
=====================
idAFEntity_Gibbable::SpawnGibs
=====================
*/
void idAFEntity_Gibbable::SpawnGibs( const idVec3 &dir, const char *damageDefName ) {
	int i;
	bool gibNonSolid;
	idVec3 entityCenter, velocity;
	idList<idEntity *> list;

	assert( !gameLocal.isClient );

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	// spawn gib articulated figures
	idAFEntity_Base::DropAFs( this, "gib", &list );

	// spawn gib items
	idMoveableItem::DropItems( this, "gib", &list );

	// blow out the gibs in the given direction away from the center of the entity
	entityCenter = GetPhysics()->GetAbsBounds().GetCenter();
	gibNonSolid = damageDef->GetBool( "gibNonSolid" );
	for ( i = 0; i < list.Num(); i++ ) {
		if ( gibNonSolid ) {
			list[i]->GetPhysics()->SetContents( 0 );
			list[i]->GetPhysics()->SetClipMask( 0 );
			list[i]->GetPhysics()->UnlinkClip();
			list[i]->GetPhysics()->PutToRest();
		} else {
#ifdef _D3XP
			list[i]->GetPhysics()->SetContents( 0 );
#else
			list[i]->GetPhysics()->SetContents( CONTENTS_CORPSE );
#endif
			list[i]->GetPhysics()->SetClipMask( CONTENTS_SOLID );
			velocity = list[i]->GetPhysics()->GetAbsBounds().GetCenter() - entityCenter;
			velocity.NormalizeFast();
			velocity += ( i & 1 ) ? dir : -dir;
			list[i]->GetPhysics()->SetLinearVelocity( velocity * 75.0f );
		}
#ifdef _D3XP
		// Don't allow grabber to pick up temporary gibs
		list[i]->noGrab = true;
#endif
		list[i]->GetRenderEntity()->noShadow = true;
		list[i]->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
		list[i]->PostEventSec( &EV_Remove, 4.0f );
	}
}

/*
============
idAFEntity_Gibbable::Gib
============
*/
void idAFEntity_Gibbable::Gib( const idVec3 &dir, const char *damageDefName ) {
	// only gib once
	if ( gibbed ) {
		return;
	}

#ifdef _D3XP
	// Don't grab this ent after it's been gibbed (and now invisible!)
	noGrab = true;
#endif

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	if ( damageDef->GetBool( "gibNonSolid" ) ) {
		GetAFPhysics()->SetContents( 0 );
		GetAFPhysics()->SetClipMask( 0 );
		GetAFPhysics()->UnlinkClip();
		GetAFPhysics()->PutToRest();
	} else {
		GetAFPhysics()->SetContents( CONTENTS_CORPSE );
		GetAFPhysics()->SetClipMask( CONTENTS_SOLID );
	}

	UnlinkCombat();

	if ( g_bloodEffects.GetBool() ) {
		if ( gameLocal.time > gameLocal.GetGibTime() ) {
			gameLocal.SetGibTime( gameLocal.time + GIB_DELAY );
			SpawnGibs( dir, damageDefName );
			renderEntity.noShadow = true;
			renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
			StartSound( "snd_gibbed", SND_CHANNEL_ANY, 0, false, NULL );
			gibbed = true;
		}
	} else {
		gibbed = true;
	}

	//ivan start - turn off dmgfxs on gib
	StopDamageFxs();
	allowDmgfxs = false;
	//ivan end

	PostEventSec( &EV_Gibbed, 4.0f );
}

/*
============
idAFEntity_Gibbable::Event_Gib
============
*/
void idAFEntity_Gibbable::Event_Gib( const char *damageDefName ) {
	Gib( idVec3( 0, 0, 1 ), damageDefName );
}

//ff1.3 start - dmgfx
/*
=====================
idAFEntity_Gibbable::CheckDamageFx
=====================
*/
void idAFEntity_Gibbable::CheckDamageFx( const idDict *damageDef, idEntity *attacker ){ 
	const char *name;
	if ( damageDef->GetString( "dmgFxType", NULL, &name ) ) {
		if ( spawnArgs.GetString( va( "def_dmgfx_%s", name ), NULL, &name) && *name != '\0' ){
			//gameLocal.Printf("def_dmgfx '%s'\n", name);
			StartDamageFx( name, attacker );
		}
	}
}

/*
=====================
idAFEntity_Gibbable::StartDamageFx
=====================
*/
void idAFEntity_Gibbable::StartDamageFx( const char * defName, idEntity *attacker ){ 
	int i;
	idDamagingFx* tempFx;

	//check if the effect is already active --> restart it
	for( i = dmgFxEntities.Num() - 1; i >= 0; i-- ) {
		tempFx = dmgFxEntities[ i ].GetEntity();
		if ( !tempFx ) {
			dmgFxEntities.RemoveIndex( i ); //remove invalid ones
			continue;
		}
		if ( !strcmp( tempFx->GetEntityDefName(), defName ) ) {
			tempFx->SetAttacker( attacker );
			tempFx->Restart();
			return;
		}
	}

	//start a new one
	tempFx = idDamagingFx::StartDamagingFx( defName, attacker, this );
	if ( tempFx ) {
		idEntityPtr<idDamagingFx> &newFxPtr = dmgFxEntities.Alloc();
		newFxPtr = tempFx;
	}
}

/*
=====================
idAFEntity_Gibbable::StopDamageFxs
=====================
*/
void idAFEntity_Gibbable::StopDamageFxs( void ){ 
	int i;
	idDamagingFx* tempFx;

	for( i = dmgFxEntities.Num() - 1; i >= 0; i-- ) {
		tempFx = dmgFxEntities[ i ].GetEntity();
		if ( !tempFx ) {
			dmgFxEntities.RemoveIndex( i ); //remove invalid ones
			continue;
		}
		tempFx->FadeOutFx();
	}
}

/*
=====================
idAFEntity_Gibbable::RemoveDamageFxs
=====================
*/
void idAFEntity_Gibbable::RemoveDamageFxs( void ){ 
	int i;
	idDamagingFx* tempFx;

	for( i = dmgFxEntities.Num() - 1; i >= 0; i-- ) {
		tempFx = dmgFxEntities[ i ].GetEntity();
		if ( !tempFx ) {
			continue;
		}
		tempFx->PostEventMS( &EV_Remove, 0 );
		dmgFxEntities.RemoveIndex( i );
	}
}

/*
=====================
idAFEntity_Gibbable::KilledDamageFxs
=====================
*/
void idAFEntity_Gibbable::KilledDamageFxs( void ){ 
	int i;
	idDamagingFx* tempFx;

	for( i = dmgFxEntities.Num() - 1; i >= 0; i-- ) {
		tempFx = dmgFxEntities[ i ].GetEntity();
		if ( !tempFx ) {
			dmgFxEntities.RemoveIndex( i ); //remove invalid ones
			continue;
		}
		tempFx->VictimKilled();
	}
}
//ff1.3 end

/*
===============================================================================

  idAFEntity_Generic

===============================================================================
*/

CLASS_DECLARATION( idAFEntity_Gibbable, idAFEntity_Generic )
	EVENT( EV_Activate,			idAFEntity_Generic::Event_Activate )
END_CLASS

/*
================
idAFEntity_Generic::idAFEntity_Generic
================
*/
idAFEntity_Generic::idAFEntity_Generic( void ) {
	keepRunningPhysics = false;
}

/*
================
idAFEntity_Generic::~idAFEntity_Generic
================
*/
idAFEntity_Generic::~idAFEntity_Generic( void ) {
}

/*
================
idAFEntity_Generic::Save
================
*/
void idAFEntity_Generic::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( keepRunningPhysics );
}

/*
================
idAFEntity_Generic::Restore
================
*/
void idAFEntity_Generic::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( keepRunningPhysics );
}

/*
================
idAFEntity_Generic::Think
================
*/
void idAFEntity_Generic::Think( void ) {
	idAFEntity_Base::Think();

	if ( keepRunningPhysics ) {
		BecomeActive( TH_PHYSICS );
	}
}

/*
================
idAFEntity_Generic::Spawn
================
*/
void idAFEntity_Generic::Spawn( void ) {
	if ( !LoadAF() ) {
		gameLocal.Error( "Couldn't load af file on entity '%s'", name.c_str() );
	}

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	af.GetPhysics()->PutToRest();
	if ( !spawnArgs.GetBool( "nodrop", "0" ) ) {
		af.GetPhysics()->Activate();
	}

	fl.takedamage = true;
}

/*
================
idAFEntity_Generic::Event_Activate
================
*/
void idAFEntity_Generic::Event_Activate( idEntity *activator ) {
	float delay;
	idVec3 init_velocity, init_avelocity;

	Show();

	af.GetPhysics()->EnableImpact();
	af.GetPhysics()->Activate();

	spawnArgs.GetVector( "init_velocity", "0 0 0", init_velocity );
	spawnArgs.GetVector( "init_avelocity", "0 0 0", init_avelocity );

	delay = spawnArgs.GetFloat( "init_velocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetLinearVelocity( init_velocity );
	} else {
		PostEventSec( &EV_SetLinearVelocity, delay, init_velocity );
	}

	delay = spawnArgs.GetFloat( "init_avelocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetAngularVelocity( init_avelocity );
	} else {
		PostEventSec( &EV_SetAngularVelocity, delay, init_avelocity );
	}
}


/*
===============================================================================

  idAFEntity_WithAttachedHead

===============================================================================
*/

CLASS_DECLARATION( idAFEntity_Gibbable, idAFEntity_WithAttachedHead )
	EVENT( EV_Gib,				idAFEntity_WithAttachedHead::Event_Gib )
	EVENT( EV_Activate,			idAFEntity_WithAttachedHead::Event_Activate )
END_CLASS

/*
================
idAFEntity_WithAttachedHead::idAFEntity_WithAttachedHead
================
*/
idAFEntity_WithAttachedHead::idAFEntity_WithAttachedHead() {
	head = NULL;
}

/*
================
idAFEntity_WithAttachedHead::~idAFEntity_WithAttachedHead
================
*/
idAFEntity_WithAttachedHead::~idAFEntity_WithAttachedHead() {
	if ( head.GetEntity() ) {
		head.GetEntity()->ClearBody();
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idAFEntity_WithAttachedHead::Spawn
================
*/
void idAFEntity_WithAttachedHead::Spawn( void ) {
	SetupHead();

	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	af.GetPhysics()->PutToRest();
	if ( !spawnArgs.GetBool( "nodrop", "0" ) ) {
		af.GetPhysics()->Activate();
	}

	fl.takedamage = true;

	if ( head.GetEntity() ) {
		int anim = head.GetEntity()->GetAnimator()->GetAnim( "dead" );

		if ( anim ) {
			head.GetEntity()->GetAnimator()->SetFrame( ANIMCHANNEL_ALL, anim, 0, gameLocal.time, 0 );
		}
	}
}

/*
================
idAFEntity_WithAttachedHead::Save
================
*/
void idAFEntity_WithAttachedHead::Save( idSaveGame *savefile ) const {
	head.Save( savefile );
}

/*
================
idAFEntity_WithAttachedHead::Restore
================
*/
void idAFEntity_WithAttachedHead::Restore( idRestoreGame *savefile ) {
	head.Restore( savefile );
}

/*
================
idAFEntity_WithAttachedHead::SetupHead
================
*/
void idAFEntity_WithAttachedHead::SetupHead( void ) {
	idAFAttachment		*headEnt;
	idStr				jointName;
	const char			*headModel;
	jointHandle_t		joint;
	idVec3				origin;
	idMat3				axis;

	headModel = spawnArgs.GetString( "def_head", "" );
	if ( headModel[ 0 ] ) {
		jointName = spawnArgs.GetString( "head_joint" );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		headEnt = static_cast<idAFAttachment *>( gameLocal.SpawnEntityType( idAFAttachment::Type, NULL ) );
		headEnt->SetName( va( "%s_head", name.c_str() ) );
		headEnt->SetBody( this, headModel, joint );
		headEnt->SetCombatModel();
		head = headEnt;

#ifdef _D3XP
		idStr xSkin;
		if ( spawnArgs.GetString( "skin_head_xray", "", xSkin ) ) {
			headEnt->xraySkin = declManager->FindSkin( xSkin.c_str() );
			headEnt->UpdateModel();
		}
#endif
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;
		headEnt->SetOrigin( origin );
		headEnt->SetAxis( renderEntity.axis );
		headEnt->BindToJoint( this, joint, true );
	}
}

/*
================
idAFEntity_WithAttachedHead::Think
================
*/
void idAFEntity_WithAttachedHead::Think( void ) {
	idAFEntity_Base::Think();
}

/*
================
idAFEntity_WithAttachedHead::LinkCombat
================
*/
void idAFEntity_WithAttachedHead::LinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( fl.hidden ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->LinkCombat();
	}
}

/*
================
idAFEntity_WithAttachedHead::UnlinkCombat
================
*/
void idAFEntity_WithAttachedHead::UnlinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( combatModel ) {
		combatModel->Unlink();
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->UnlinkCombat();
	}
}

/*
================
idAFEntity_WithAttachedHead::Hide
================
*/
void idAFEntity_WithAttachedHead::Hide( void ) {
	idAFEntity_Base::Hide();
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
	UnlinkCombat();
}

/*
================
idAFEntity_WithAttachedHead::Show
================
*/
void idAFEntity_WithAttachedHead::Show( void ) {
	idAFEntity_Base::Show();
	if ( head.GetEntity() ) {
		head.GetEntity()->Show();
	}
	LinkCombat();
}

/*
================
idAFEntity_WithAttachedHead::ProjectOverlay
================
*/
void idAFEntity_WithAttachedHead::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {

	idEntity::ProjectOverlay( origin, dir, size, material );

	if ( head.GetEntity() ) {
		head.GetEntity()->ProjectOverlay( origin, dir, size, material );
	}
}

/*
============
idAFEntity_WithAttachedHead::Gib
============
*/
void idAFEntity_WithAttachedHead::Gib( const idVec3 &dir, const char *damageDefName ) {
	// only gib once
	if ( gibbed ) {
		return;
	}
	idAFEntity_Gibbable::Gib( dir, damageDefName );
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
}

/*
============
idAFEntity_WithAttachedHead::Event_Gib
============
*/
void idAFEntity_WithAttachedHead::Event_Gib( const char *damageDefName ) {
	Gib( idVec3( 0, 0, 1 ), damageDefName );
}

/*
================
idAFEntity_WithAttachedHead::Event_Activate
================
*/
void idAFEntity_WithAttachedHead::Event_Activate( idEntity *activator ) {
	float delay;
	idVec3 init_velocity, init_avelocity;

	Show();

	af.GetPhysics()->EnableImpact();
	af.GetPhysics()->Activate();

	spawnArgs.GetVector( "init_velocity", "0 0 0", init_velocity );
	spawnArgs.GetVector( "init_avelocity", "0 0 0", init_avelocity );

	delay = spawnArgs.GetFloat( "init_velocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetLinearVelocity( init_velocity );
	} else {
		PostEventSec( &EV_SetLinearVelocity, delay, init_velocity );
	}

	delay = spawnArgs.GetFloat( "init_avelocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetAngularVelocity( init_avelocity );
	} else {
		PostEventSec( &EV_SetAngularVelocity, delay, init_avelocity );
	}
}


/*
===============================================================================

  idAFEntity_Vehicle

===============================================================================
*/

//ivan start
const idEventDef EV_Vehicle_AmmoAvailable( "vehicleAmmoAvailable", NULL, 'f' );
const idEventDef EV_Vehicle_UseAmmo( "useVehicleAmmo", "d" );
//ivan end

CLASS_DECLARATION( idAFEntity_Base, idAFEntity_Vehicle )
	//ivan start
	EVENT( EV_GetDriver,				idAFEntity_Vehicle::Event_GetDriver )
	EVENT( EV_GetAimAngles,				idAFEntity_Vehicle::Event_GetAimAngles)
	
	EVENT( EV_Vehicle_AmmoAvailable,	idAFEntity_Vehicle::Event_AmmoAvailable)
	EVENT( EV_Vehicle_UseAmmo,			idAFEntity_Vehicle::Event_UseAmmo)
	//ivan end
END_CLASS

/*
================
idAFEntity_Vehicle::idAFEntity_Vehicle
================
*/
idAFEntity_Vehicle::idAFEntity_Vehicle( void ) {
	driver				= NULL;
	eyesJoint			= INVALID_JOINT;
	steeringWheelJoint	= INVALID_JOINT;
	wheelRadius			= 0.0f;
	steerAngle			= 0.0f;
	steerSpeed			= 0.0f;
	dustSmoke			= NULL;

	//ivan start
	ammo				= 0;
	vehicleVelocity		= 0.0f;
	vehicleForce		= 0.0f;
	
	thirdPersonRange	= 0.0f;
	thirdPersonHeight	= 0.0f;

	scriptThread		= NULL;		// initialized by ConstructScriptObject, which is called by idEntity::Spawn
	//state				= NULL;
	//idealState			= NULL;
	//ivan end
}

/*
=====================
idAFEntity_Vehicle::~idAFEntity_Vehicle
=====================
*/
idAFEntity_Vehicle::~idAFEntity_Vehicle( void ) {
	/*
	//already done in Entity destructor...
	DeconstructScriptObject();
	scriptObject.Free();
	*/
	ShutdownThreads();
}


/*
================
idAFEntity_Vehicle::Spawn
================
*/
void idAFEntity_Vehicle::Spawn( void ) {
	const char *eyesJointName = spawnArgs.GetString( "eyesJoint", "eyes" );
	const char *steeringWheelJointName = spawnArgs.GetString( "steeringWheelJoint", "steeringWheel" );

	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	if ( !eyesJointName[0] ) {
		gameLocal.Error( "idAFEntity_Vehicle '%s' no eyes joint specified", name.c_str() );
	}
	eyesJoint = animator.GetJointHandle( eyesJointName );
	if ( !steeringWheelJointName[0] ) {
		gameLocal.Error( "idAFEntity_Vehicle '%s' no steering wheel joint specified", name.c_str() );
	}
	steeringWheelJoint = animator.GetJointHandle( steeringWheelJointName );

	spawnArgs.GetFloat( "wheelRadius", "20", wheelRadius );
	spawnArgs.GetFloat( "steerSpeed", "5", steerSpeed );

	driver = NULL;
	steerAngle = 0.0f;

	const char *smokeName = spawnArgs.GetString( "smoke_vehicle_dust", "muzzlesmoke" );
	if ( *smokeName != '\0' ) {
		dustSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	}

	//ivan start
	spawnArgs.GetInt( "ammo", "100", ammo ); 

	//third-person camera joint
	const char *cameraJointName = spawnArgs.GetString( "cameraJoint", "origin" );
	if ( !cameraJointName[0] ) {
		gameLocal.Error( "idAFEntity_Vehicle '%s' no third-person camera joint specified", name.c_str() );
	}
	cameraJoint = animator.GetJointHandle( cameraJointName );
	
	//vehicle settings - set same default values as old CVars
	spawnArgs.GetFloat( "vehicleVelocity", "1000", vehicleVelocity);
	spawnArgs.GetFloat( "vehicleForce", "50000", vehicleForce);
	
	//camera settings
	spawnArgs.GetFloat( "thirdPersonRange", "200", thirdPersonRange);
	spawnArgs.GetFloat( "thirdPersonHeight", "106", thirdPersonHeight);


	// initialize the script variables
	LinkScriptVariables();

	//ivan end
}

/*
===============
idAFEntity_Vehicle::InitHudStats
===============
*/
void idAFEntity_Vehicle::InitHudStats( idUserInterface *hud, idUserInterface *cursor ) {
	if ( hud ) {
		//hud->SetStateInt( "sentry_health_max", spawnArgs.GetInt("health") );
		hud->HandleNamedEvent( spawnArgs.GetString("hudHealthEvent", "health_mode_buggy_green") );
		hud->HandleNamedEvent( spawnArgs.GetString("hudWeaponEvent", "setup_melee_only") );
		hud->HandleNamedEvent( "start_riding" );
		hud->HandleNamedEvent( "select_a0" ); //Vehicles does not support multiple weapons atm
	}

	if( cursor ){
		cursor->HandleNamedEvent( spawnArgs.GetString("cursorEvent", "enableSentryCursor") ); //hideCursor
	}
}

/*
===============
idAFEntity_Vehicle::UpdateHudStats
===============
*/
void idAFEntity_Vehicle::UpdateHudStats( idUserInterface *hud ) {
	if ( hud ) {
		hud->SetStateInt( "sentry_health", health );
		hud->SetStateFloat( "sentry_health_pct", health / spawnArgs.GetFloat("health") );
		hud->SetStateInt( "sentry_ammo", ammo );
	}
}

/*
================
idAFEntity_Vehicle::Use
================
*/
bool idAFEntity_Vehicle::StartDriving( idPlayer *player ) {
	idVec3 origin;
	idMat3 axis;
	idAngles angles;

	if ( driver || !player || VEHICLE_DEAD ) {
		return false;
	}
	
	driver = player;
	animator.GetJointTransform( eyesJoint, gameLocal.time, origin, axis );
	//gameLocal.Printf("GetJointTransform %d, %s\n", gameLocal.time, origin.ToString());
	origin = renderEntity.origin + origin * renderEntity.axis;
	driver->GetPhysics()->SetOrigin( origin );
	driver->BindToBody( this, 0, false ); //was true, but 'false' fixes wrong view angles after teleport

	angles = renderEntity.axis.ToAngles();
	angles.roll = 0.0f;
	driver->SetViewAngles( angles );

	af.GetPhysics()->SetComeToRest( false );
	af.GetPhysics()->Activate();

	SetState( "state_Drive" );
	return true;
}


/*
================
idAFEntity_Vehicle::Use
================
*/
void idAFEntity_Vehicle::StopDriving( void ) {
	if ( driver ) {
		driver->Unbind();
		driver = NULL;

		af.GetPhysics()->SetComeToRest( true );

		VEHICLE_FIRING = false;
		VEHICLE_FORWARD = false;

		if( !VEHICLE_DEAD ){
			SetState( "state_Idle" );
		}
		//nextUseTime = gameLocal.time + 1000;
	}
}

/*
================
idAFEntity_Vehicle::Use
================

void idAFEntity_Vehicle::Use( idPlayer *other ) {
	idVec3 origin;
	idMat3 axis;

	if ( driver ) {
		if ( driver == other ) {
			other->Unbind();
			driver = NULL;

			af.GetPhysics()->SetComeToRest( true );

			//ivan start
			VEHICLE_FIRING = false;
			VEHICLE_FORWARD = false;

			//state = GetScriptFunction( "state_Idle" );
			SetState( "state_Idle" );
			//ivan end
		}
	}
	else {
		driver = other;
		animator.GetJointTransform( eyesJoint, gameLocal.time, origin, axis );
		//gameLocal.Printf("GetJointTransform %d, %s\n", gameLocal.time, origin.ToString());
		origin = renderEntity.origin + origin * renderEntity.axis;
		driver->GetPhysics()->SetOrigin( origin );
		driver->BindToBody( this, 0, true );

		af.GetPhysics()->SetComeToRest( false );
		af.GetPhysics()->Activate();
		
		//ivan start
		//state = GetScriptFunction( "state_Drive" );
		SetState( "state_Drive" );
		//ivan end
	}
}

*/

/*
===============
idAFEntity_Vehicle::GetCameraPos
===============
*/
void idAFEntity_Vehicle::GetCameraPos( idVec3 &origin, idMat3 &axis ) {
	//we only need the world position of origin. axis is not important atm.
	//so we can simply use the code below intead of
	//GetJointWorldTransform( cameraJoint, gameLocal.time, origin, axis );
	//...which would be slower
	animator.GetJointTransform( cameraJoint, gameLocal.time, origin, axis );
	origin = renderEntity.origin + origin * renderEntity.axis;
}

/*
=====================
idActor::EyeOffset
=====================
*/
idVec3 idAFEntity_Vehicle::EyeOffset( void ) const {
	return GetPhysics()->GetGravityNormal() * -10;
}

/*
=====================
idActor::GetEyePosition
=====================
*/
idVec3 idAFEntity_Vehicle::GetEyePosition( void ) const {
	return GetPhysics()->GetOrigin() + ( GetPhysics()->GetGravityNormal() * -10 );
}

/*
=====================
idActor::GetAIAimTargets

Returns positions for the AI to aim at.
=====================
*/
void idAFEntity_Vehicle::GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos ) {
	headPos = lastSightPos + EyeOffset();
	chestPos = headPos; //( headPos + lastSightPos + GetPhysics()->GetBounds().GetCenter() ) * 0.5f;
}

/*
================
idAFEntity_Vehicle::GetSteerAngle
================
*/
float idAFEntity_Vehicle::GetSteerAngle( void ) {
	float idealSteerAngle, angleDelta;

	idealSteerAngle = driver->usercmd.rightmove * ( 30.0f / 128.0f );
	angleDelta = idealSteerAngle - steerAngle;

	if ( angleDelta > steerSpeed ) {
		steerAngle += steerSpeed;
	} else if ( angleDelta < -steerSpeed ) {
		steerAngle -= steerSpeed;
	} else {
		steerAngle = idealSteerAngle;
	}

	return steerAngle;
}

//ff start

/*
================
idAFEntity_Vehicle::Damage
================
*/
void idAFEntity_Vehicle::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
	const char *damageDefName, const float damageScale, const int location ) {

	if ( !fl.takedamage || gameLocal.inCinematic || ( driver && driver->godmode ) ) {
		return;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}

	bool ignoreVehicle;
	if ( damageDef->GetBool( "ignore_vehicle", "1", ignoreVehicle ) ) {
		if ( ignoreVehicle ) {
			return;
		}
	} else if ( attacker && attacker->IsType( idActor::Type ) ) {
		//disable friendly fire unless ignore_vehicle is explicitly disabled (fix for double shotgun sec fire)
		if ( static_cast<idActor *>(attacker)->team == 0 ) {
			return; 
		}
	}

	/*
	//knockback
	idVec3		kick;
	int			knockback;

	damageDef->GetInt( "knockback", "20", knockback );

	if ( knockback != 0 && !fl.noknockback ) {
		kick = dir;
		kick.Normalize();
		kick *= g_knockback.GetFloat() * knockback / 200.0f;
		GetPhysics()->SetLinearVelocity( GetPhysics()->GetLinearVelocity() + kick );

		gameLocal.Printf("knockback\n");
		// set the timer so that the player can't cancel out the movement immediately
		//physicsObj.SetKnockBack( idMath::ClampInt( 50, 200, knockback * 2 ) );
	}
	*/

	float skillScale = 1.0f;
	if ( !gameLocal.isMultiplayer && inflictor != gameLocal.world ) {
		switch ( g_skill.GetInteger() ) {
			case 0:
				skillScale = 0.9f;
				break;
			case 2:
				skillScale = 1.1f;
				break;
			case 3:
				skillScale = 1.2f;
				break;
			default:
				break;
		}
	}

	idAFEntity_Base::Damage( inflictor, attacker, dir, damageDefName, skillScale * damageScale * damageDef->GetFloat("vehicleDamageScale", "1"), location );
}

/*
============
idAFEntity_Vehicle::Killed
============
*/
void idAFEntity_Vehicle::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( !VEHICLE_DEAD ) {
		VEHICLE_DEAD = true;
		SetState( "state_Killed" );

		if ( driver ) {
			driver->ExitVehicle();
		}
	}
}

/*
================
idAFEntity_Vehicle::Teleport
================
*/
void idAFEntity_Vehicle::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	if ( driver ) {
		idAngles viewAngles = renderEntity.axis.ToAngles();
		viewAngles.roll = 0.0f;
		driver->SetViewAngles( viewAngles );
	}

	idAFEntity_Base::Teleport(origin, angles, destination);
	GetPhysics()->SetLinearVelocity( vec3_origin );
	GetPhysics()->SetAngularVelocity( vec3_origin );
}

//ff end

//ivan start

/*
==============
idAFEntity_Vehicle::LinkScriptVariables
==============
*/
void idAFEntity_Vehicle::LinkScriptVariables( void ) {
	VEHICLE_FIRING.LinkTo(			scriptObject, "VEHICLE_FIRING" );
	VEHICLE_FORWARD.LinkTo(			scriptObject, "VEHICLE_FORWARD" );
	VEHICLE_DEAD.LinkTo(			scriptObject, "VEHICLE_DEAD" );
}

/*
================
idAFEntity_Vehicle::CallOnScriptObject
================

void idAFEntity_Vehicle::CallOnScriptObject( const char *name ) {
	const function_t *func;
	if ( scriptObject.HasObject() ) {
		func = scriptObject.GetFunction( name );
		if ( func ) {
			idThread *thread = new idThread();
			thread->CallFunction( this, func, true );
			thread->DelayedStart( 0 );
			//thread->Start(); 
		}
	}
}
*/

/*
===============
idAFEntity_Vehicle::NextWeapon
===============

void idAFEntity_Vehicle::NextWeapon( void ) {
	CallOnScriptObject( "selectNextWeapon" );
}
*/
/*
===============
idAFEntity_Vehicle::PrevWeapon
===============

void idAFEntity_Vehicle::PrevWeapon( void ) {
	CallOnScriptObject( "selectPrevWeapon" );
}
*/
/*
==================
idAFEntity_Vehicle::Event_GetDriver
==================
*/
void idAFEntity_Vehicle::Event_GetDriver( void ) {
	idThread::ReturnEntity( driver );
}

/*
==================
idAFEntity_Vehicle::Event_GetAimAngles
==================
*/
void idAFEntity_Vehicle::Event_GetAimAngles( const idVec3 &firePos ) {
	idAngles angles;
	if ( !gameLocal.inCinematic && driver && driver->renderView ) {
		renderView_t *view = driver->renderView;
		idVec3 end = view->vieworg + view->viewaxis[ 0 ] * 3000.0f;
		trace_t trace;
		gameLocal.clip.TracePoint( trace, view->vieworg, end, MASK_SOLID|CONTENTS_RENDERMODEL, this );
		if ( trace.fraction < 1.0f ) {
			angles = (trace.endpos - firePos).ToAngles();
		} else {
			angles = (end - firePos).ToAngles();
		}
	} else {
		angles = GetPhysics()->GetAxis().ToAngles();
	}
	idThread::ReturnVector( idVec3( angles[0], angles[1], angles[2] ) );
}

/*
=====================
idAFEntity_Vehicle::Event_Touch
=====================

void idAFEntity_Vehicle::Event_Touch( idEntity *other, trace_t *trace ) {
	//gameLocal.Printf("idAFEntity_Vehicle::Event_Touch %s\n", other->GetName());
	if ( !driver && nextUseTime < gameLocal.time && other->IsType( idPlayer::Type ) ){
		gameLocal.Printf("drive me\n");
		idPlayer *thePlayer = static_cast<idPlayer *>(other);
		thePlayer->EnterVehicle( this );
	}
}
*/

/*
==================
idAFEntity_Vehicle::Save
==================
*/
void idAFEntity_Vehicle::Save( idSaveGame *savefile ) const {
	//gameLocal.Printf("idAFEntity_Vehicle::Save\n");
	savefile->WriteObject(driver);
	savefile->WriteJoint(eyesJoint);
	savefile->WriteJoint(cameraJoint);
	savefile->WriteJoint(steeringWheelJoint);
	savefile->WriteFloat(wheelRadius);
	savefile->WriteFloat(steerAngle);
	savefile->WriteFloat(steerSpeed);
	savefile->WriteParticle(dustSmoke);
	savefile->WriteInt(ammo);

	savefile->WriteFloat(vehicleVelocity);
	savefile->WriteFloat(vehicleForce);
	
	savefile->WriteFloat(thirdPersonRange);
	savefile->WriteFloat(thirdPersonHeight);

	// script variables
	savefile->WriteObject( scriptThread );

	/*
	//FIXME: this is unneccesary
	idToken token;
	if ( state ) {
		idLexer src( state->Name(), idStr::Length( state->Name() ), "idAFEntity_Vehicle::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	if ( idealState ) {
		idLexer src( idealState->Name(), idStr::Length( idealState->Name() ), "idAFEntity_Vehicle::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}
	*/
}
/*
==================
idAFEntity_Vehicle::Restore
==================
*/
void idAFEntity_Vehicle::Restore( idRestoreGame *savefile ) {
	//gameLocal.Printf("idAFEntity_Vehicle::Restore\n");
	savefile->ReadObject(reinterpret_cast<idClass *&>(driver));
	savefile->ReadJoint(eyesJoint);
	savefile->ReadJoint(cameraJoint);
	savefile->ReadJoint(steeringWheelJoint);
	savefile->ReadFloat(wheelRadius);
	savefile->ReadFloat(steerAngle);
	savefile->ReadFloat(steerSpeed);
	savefile->ReadParticle(dustSmoke);
	savefile->ReadInt(ammo);

	savefile->ReadFloat(vehicleVelocity);
	savefile->ReadFloat(vehicleForce);
	
	savefile->ReadFloat(thirdPersonRange);
	savefile->ReadFloat(thirdPersonHeight);

	// script variables
	savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) );

	// Re-link idBoolFields to the scriptObject, values will be restored in scriptObject's restore
	LinkScriptVariables();

	SetCombatModel();
	LinkCombat();

	//activate Physics for a frame just to avoid GetJointTransform errors in Use function after reload
	af.GetPhysics()->Activate();

	/*
	idStr statename;
	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		state = GetScriptFunction( statename );
	}

	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		idealState = GetScriptFunction( statename );
	}
	*/
}


/***********************************************************************

	script state management

***********************************************************************/

/*
================
idAFEntity_Vehicle::ShutdownThreads
================
*/
void idAFEntity_Vehicle::ShutdownThreads( void ) {
	if ( scriptThread ) {
		scriptThread->EndThread();
		scriptThread->PostEventMS( &EV_Remove, 0 );
		delete scriptThread;
		scriptThread = NULL;
	}
}

/*
================
idAFEntity_Vehicle::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================

bool idAFEntity_Vehicle::ShouldConstructScriptObjectAtSpawn( void ) const {
	return true; //was false
}
*/

/*
================
idAFEntity_Vehicle::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idAFEntity_Vehicle::ConstructScriptObject( void ) {
	const function_t *constructor;

	// make sure we have a scriptObject
	if ( !scriptObject.HasObject() ) {
		gameLocal.Error( "No scriptobject set on '%s'.  Check the '%s' entityDef.", name.c_str(), GetEntityDefName() );
	}

	if ( !scriptThread ) {
		// create script thread
		scriptThread = new idThread();
		scriptThread->ManualDelete();
		scriptThread->ManualControl();
		scriptThread->SetThreadName( name.c_str() );
	} else {
		scriptThread->EndThread();
	}
	
	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for entity '%s'", scriptObject.GetTypeName(), name.c_str() );
	}

	// init the script object's data
	scriptObject.ClearObject();

	// just set the current function on the script.  we'll execute later.
	scriptThread->CallFunction( this, constructor, true );

	return scriptThread;
}

/*
=====================
idAFEntity_Vehicle::GetScriptFunction
=====================
*/
const function_t *idAFEntity_Vehicle::GetScriptFunction( const char *funcname ) {
	const function_t *func;

	func = scriptObject.GetFunction( funcname );
	if ( !func ) {
		scriptThread->Error( "Unknown function '%s' in '%s'", funcname, scriptObject.GetTypeName() );
	}

	return func;
}

/*
=====================
idAFEntity_Vehicle::SetState
=====================
*/
void idAFEntity_Vehicle::SetState( const function_t *newState ) {
	if ( !newState ) {
		gameLocal.Error( "idAFEntity_Vehicle::SetState: Null state" );
	}

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		gameLocal.Printf( "%d: %s: State: %s\n", gameLocal.time, name.c_str(), newState->Name() );
	}

	//state = newState;
	//idealState = state;
	scriptThread->CallFunction( this, newState, true );
}

/*
=====================
idAFEntity_Vehicle::SetState
=====================
*/
void idAFEntity_Vehicle::SetState( const char *statename ) {
	const function_t *newState;

	newState = GetScriptFunction( statename );
	SetState( newState );
}

/*
=====================
idAFEntity_Vehicle::UpdateScript
=====================
*/
void idAFEntity_Vehicle::UpdateScript( void ) {
	if ( ai_debugScript.GetInteger() == entityNumber ) {
		scriptThread->EnableDebugInfo();
	} else {
		scriptThread->DisableDebugInfo();
	}

	// don't call script until it's done waiting
	if ( !scriptThread->IsWaiting() ) {
		scriptThread->Execute();
	}
}
/*
void idAFEntity_Vehicle::UpdateScript( void ) {
	int	i;

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		scriptThread->EnableDebugInfo();
	} else {
		scriptThread->DisableDebugInfo();
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for( i = 0; i < 20; i++ ) {
		if ( idealState != state ) {
			SetState( idealState );
		}

		// don't call script until it's done waiting
		if ( scriptThread->IsWaiting() ) {
			break;
		}
        
		scriptThread->Execute();
		if ( idealState == state ) {
			break;
		}
	}

	if ( i == 20 ) {
		scriptThread->Warning( "idAFEntity_Vehicle::UpdateScript: exited loop to prevent lockup" );
	}
}
*/

/*
===============
idAFEntity_Vehicle::Event_UseAmmo
===============
*/
void idAFEntity_Vehicle::Event_UseAmmo( int amount ) {
	ammo -= amount;
	if ( ammo < 0 ) {
		ammo = 0;
	}
}

/*
===============
idAFEntity_Vehicle::Event_AmmoAvailable
===============
*/
void idAFEntity_Vehicle::Event_AmmoAvailable( void ) {
	idThread::ReturnFloat( ammo );
}
//ivan end

/*
===============================================================================

  idAFEntity_VehicleSimple

===============================================================================
*/

const idEventDef EV_Vehicle_SetTireFriction( "setTireFriction", "f" );

CLASS_DECLARATION( idAFEntity_Vehicle, idAFEntity_VehicleSimple )
	EVENT( EV_Vehicle_SetTireFriction,	idAFEntity_VehicleSimple::Event_SetTireFriction)
END_CLASS

//ivan start
static const float VEHICLE_SELF_IMPULSE_MULT_STD			= 0.1f;
static const float VEHICLE_SELF_IMPULSE_MULT_ACTOR_KILL		= 0.1f;
static const float VEHICLE_PUSH_MULT_STD					= 36.0f;
static const float VEHICLE_PUSH_MULT_ACTOR_KILL				= 366.0f;
static const float VEHICLE_MIN_PREDICTION_VELOCITY			= 10.0f;
static const float VEHICLE_MIN_DAMAGE_VELOCITY				= 100.0f;
static const float VEHICLE_MIN_SMOKE_VELOCITY				= 5.0f;
static const float VEHICLE_MAX_PREDICTION_WIDTH				= 64.0f; //100% at speed = 1000
//ivan end

/*
================
idAFEntity_VehicleSimple::idAFEntity_VehicleSimple
================
*/
idAFEntity_VehicleSimple::idAFEntity_VehicleSimple( void ) {
	int i;
	for ( i = 0; i < 4; i++ ) {
		suspension[i] = NULL;
	}

	//ivan start
	vehicleSuspensionUp			= 0.0f;
	vehicleSuspensionDown		= 0.0f;
	vehicleSuspensionKCompress	= 0.0f;
	vehicleSuspensionDamping	= 0.0f;
	vehicleTireFriction			= 0.0f;

	nextCollideFxTime	= 0;
	//ivan end
}

/*
================
idAFEntity_VehicleSimple::~idAFEntity_VehicleSimple
================
*/
idAFEntity_VehicleSimple::~idAFEntity_VehicleSimple( void ) {
	delete wheelModel;
	wheelModel = NULL;

	//all idAFConstraint_Suspension will be deleted in idPhysics_AF::~idPhysics_AF
}

/*
================
idAFEntity_VehicleSimple::Spawn
================
*/
void idAFEntity_VehicleSimple::Spawn( void ) {
	static const char *wheelJointKeys[] = {
		"wheelJointFrontLeft",
		"wheelJointFrontRight",
		"wheelJointRearLeft",
		"wheelJointRearRight"
	};
	static idVec3 wheelPoly[4] = { idVec3( 2, 2, 0 ), idVec3( 2, -2, 0 ), idVec3( -2, -2, 0 ), idVec3( -2, 2, 0 ) };

	//vehicle settings - set same default values as old CVars
	spawnArgs.GetFloat( "vehicleSuspensionUp", "32", vehicleSuspensionUp);
	spawnArgs.GetFloat( "vehicleSuspensionDown", "20", vehicleSuspensionDown);
	spawnArgs.GetFloat( "vehicleSuspensionKCompress", "200", vehicleSuspensionKCompress);
	spawnArgs.GetFloat( "vehicleSuspensionDamping", "400", vehicleSuspensionDamping);
	spawnArgs.GetFloat( "vehicleTireFriction", "0.8", vehicleTireFriction);

	int i;
	idVec3 origin;
	idMat3 axis;
	idTraceModel trm;

	trm.SetupPolygon( wheelPoly, 4 );
	trm.Translate( idVec3( 0, 0, -wheelRadius ) );
	wheelModel = new idClipModel( trm );

	for ( i = 0; i < 4; i++ ) {
		const char *wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "idAFEntity_VehicleSimple '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idAFEntity_VehicleSimple '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}

		GetAnimator()->GetJointTransform( wheelJoints[i], 0, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;

		suspension[i] = new idAFConstraint_Suspension();
		//ivan start - split setup in 2 parts so we can avoid changing the position on reload
		//was: suspension[i]->Setup( va( "suspension%d", i ), af.GetPhysics()->GetBody( 0 ), origin, af.GetPhysics()->GetAxis( 0 ), wheelModel );
		suspension[i]->Setup( va( "suspension%d", i ), af.GetPhysics()->GetBody( 0 ), wheelModel );
		suspension[i]->SetPosition( origin, af.GetPhysics()->GetAxis( 0 ) );
		/*
		//was:
		suspension[i]->SetSuspension(	g_vehicleSuspensionUp.GetFloat(),
										g_vehicleSuspensionDown.GetFloat(),
										g_vehicleSuspensionKCompress.GetFloat(),
										g_vehicleSuspensionDamping.GetFloat(),
										g_vehicleTireFriction.GetFloat() );
		*/
		suspension[i]->SetSuspension(	vehicleSuspensionUp,
										vehicleSuspensionDown,
										vehicleSuspensionKCompress,
										vehicleSuspensionDamping,
										vehicleTireFriction );

		//ivan end
		af.GetPhysics()->AddConstraint( suspension[i] );
	}

	//ivan start
	fxCollide = spawnArgs.GetString( "fx_collide" );
	nextCollideFxTime = 0;

	damage = spawnArgs.GetString( "def_damage", "" );
	//ivan end

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
idAFEntity_VehicleSimple::Think
================
*/
void idAFEntity_VehicleSimple::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	idVec3 origin;
	idMat3 axis;
	idRotation wheelRotation, steerRotation;

	if ( thinkFlags & TH_THINK ) {

		if ( driver ) {
			// capture the input from the player
			velocity = vehicleVelocity; //ivan - was:  g_vehicleVelocity.GetFloat()
			if ( driver->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = idMath::Fabs( driver->usercmd.forwardmove * vehicleForce ) * (1.0f / 128.0f); //ivan - was:  g_vehicleVelocity.GetFloat()
			steerAngle = GetSteerAngle();

			//ivan start
			VEHICLE_FIRING	= ( !driver->objectiveSystemOpen && driver->usercmd.buttons & BUTTON_ATTACK );
			VEHICLE_FORWARD = ( driver->usercmd.forwardmove > 0 );

			TouchTriggers();
			//ivan end
		}

		// update the wheel motor force and steering
		for ( i = 0; i < 2; i++ ) {

			// front wheel drive
			if ( velocity != 0.0f ) {
				suspension[i]->EnableMotor( true );
			} else {
				suspension[i]->EnableMotor( false );
			}
			suspension[i]->SetMotorVelocity( velocity );
			suspension[i]->SetMotorForce( force );

			// update the wheel steering
			suspension[i]->SetSteerAngle( steerAngle );
		}

		// adjust wheel velocity for better steering because there are no differentials between the wheels
		if ( steerAngle < 0.0f ) {
			suspension[0]->SetMotorVelocity( velocity * 0.5f );
		} else if ( steerAngle > 0.0f ) {
			suspension[1]->SetMotorVelocity( velocity * 0.5f );
		}

		//ivan start - commented out
		/*
		// update suspension with latest cvar settings
		for ( i = 0; i < 4; i++ ) {
			suspension[i]->SetSuspension(	g_vehicleSuspensionUp.GetFloat(),
											g_vehicleSuspensionDown.GetFloat(),
											g_vehicleSuspensionKCompress.GetFloat(),
											g_vehicleSuspensionDamping.GetFloat(),
											g_vehicleTireFriction.GetFloat() );
		}
		*/

		UpdateScript();

		//origin = GetPhysics()->GetOrigin();
		//ivan end

		// run the physics
		RunPhysics();

		//ivan start
		//Kick Obstacles
		if ( thinkFlags & TH_PHYSICS ) {
			PredictCollisions();
		}

		int spawnSmoke = !( gameLocal.framenum & 7 );
		//gameLocal.Printf("smoke at %d: %d\n", gameLocal.framenum , spawnSmoke);
		//ivan end


		// move and rotate the wheels visually
		for ( i = 0; i < 4; i++ ) {

			idAFBody *body = af.GetPhysics()->GetBody( 0 );
			origin = suspension[i]->GetWheelOrigin();
			velocity = body->GetPointVelocity( origin ) * body->GetWorldAxis()[0];
			wheelAngles[i] += velocity * MS2SEC( gameLocal.msec ) / wheelRadius;

			// additional rotation about the wheel axis
			wheelRotation.SetAngle( RAD2DEG( wheelAngles[i] ) );
			wheelRotation.SetVec( 0, -1, 0 );

			if ( i < 2 ) {
				// rotate the wheel for steering
				steerRotation.SetAngle( steerAngle );
				steerRotation.SetVec( 0, 0, 1 );
				// set wheel rotation
				animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, wheelRotation.ToMat3() * steerRotation.ToMat3() );
			} else {
				// set wheel rotation
				animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, wheelRotation.ToMat3() );
			}

			// set wheel position for suspension
			origin = ( origin - renderEntity.origin ) * renderEntity.axis.Transpose();
			GetAnimator()->SetJointPos( wheelJoints[i], JOINTMOD_WORLD_OVERRIDE, origin );

			//ivan start - spawn dust particle effects
			if ( spawnSmoke && abs( velocity ) > VEHICLE_MIN_SMOKE_VELOCITY && suspension[i]->GetLastContactPosition() != vec3_origin ) {
				gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), suspension[i]->GetLastContactPosition(), body->GetWorldAxis(), timeGroup /* D3XP */ );
			}
			//ivan end
		}
	}

	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}

//ivan start
void idAFEntity_VehicleSimple::ApplyCollisionFeedback( const trace_t &collision, const idVec3 &velocity, bool predicted ) {
	float multiplier, speed;
	idVec3 dir;
	idVec3 forceVec;
	idVec3 delta;
	idVec2 perpendicular;
	idEntity *ent;

	ent = gameLocal.entities[ collision.c.entityNum ];
	
	//ignore prediction if too distant, unless it's an actor
	if ( predicted && ent && collision.fraction > 0.5f && !ent->IsType( idActor::Type ) ) {
		//gameLocal.Printf("ignore predicted collision with: %s\n", ent->GetName());
		return;
	}

	//get collision data
	dir = velocity;
	speed = dir.NormalizeFast();
	//v = -( velocity * collision.c.normal );

	//play sound
	if ( speed > BOUNCE_SOUND_MIN_VELOCITY &&  gameLocal.time > nextSoundTime ) {
		if ( StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, false, NULL ) ) {
			// don't set the volume unless there is a bounce sound as it overrides the entire channel
			// which causes footsteps on ai's to not honor their shader parms
			multiplier = speed > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( speed - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
			SetSoundVolume( multiplier ); //0...1
		}
		nextSoundTime = gameLocal.time + 500;
	}

	//play fx 
	if ( fxCollide.Length() && gameLocal.time > nextCollideFxTime ) {
		//gameLocal.Printf("fx pos: %s (%s)\n", collision.c.point.ToString(), predicted ? "predicted" : "");
		idEntityFx::StartFx( fxCollide, &collision.c.point, NULL, this, false );
		nextCollideFxTime = gameLocal.time + 3500;
	}
	
	//don't damage or push players and vehicles
	if ( !ent || ent->IsType( idPlayer::Type ) || ent->IsType( idAFEntity_Vehicle::Type )  ) {
		return;
	}
	/*
	if ( g_debugVehicle.GetBool() ) {
		gameLocal.Printf("collide with: %s, speed %f\n", ent->GetName(), speed);
	}
	*/

	//damage AIs and damagables
	if ( ent->IsType( idActor::Type ) || ent->IsType( idDamagable::Type ) ) {
		if ( damage.Length() && speed > VEHICLE_MIN_DAMAGE_VELOCITY && gameLocal.time > ent->spawnArgs.GetInt("nextVehicleDamage") ) {
			delta = ent->GetPhysics()->GetOrigin() - af.GetPhysics()->GetOrigin();
			delta.NormalizeFast();
			multiplier = (delta * dir) * idMath::Sqrt( speed );

			if ( g_debugVehicle.GetBool() ) {
				gameLocal.Printf("speed: %f, projection: %f, multiplier: %f\n", speed, (delta * dir), multiplier);
			}

			if ( multiplier > 10.0f ) {
				int oldhealth = ent->health; 
				ent->Damage( this, driver ? driver : (idEntity *) this, dir, damage, multiplier, INVALID_JOINT );
				ent->spawnArgs.SetInt("nextVehicleDamage", gameLocal.time + 1000);

				//push killed AI
				if ( ent->IsType( idActor::Type ) && oldhealth > 0 && ent->health <= 0 ) {
					delta = ent->GetPhysics()->GetOrigin() - collision.endpos;
					delta.NormalizeFast();
					forceVec = delta * multiplier * VEHICLE_PUSH_MULT_ACTOR_KILL; //TODO: use ent->GetPhysics()->GetMass(); ??? should be virtual...
					ent->GetPhysics()->SetLinearVelocity( forceVec );
					if ( predicted ) {
						//gameLocal.Printf("self ApplyImpulse:\n");
						ApplyImpulse( ent, 0, collision.endpos, -forceVec*VEHICLE_SELF_IMPULSE_MULT_ACTOR_KILL ); //TODO: balance
					}
				}
			}
		}
	}

	//push moveables and AFs
	else if ( ( ent->IsType( idMoveable::Type ) || (ent->IsType( idAFEntity_Base::Type ) ) ) && ent->GetPhysics()->IsPushable() ) {
		delta = ent->GetPhysics()->GetOrigin() - af.GetPhysics()->GetOrigin();
		delta.NormalizeFast();
		perpendicular.x = -delta.y;
		perpendicular.y = delta.x;
		delta.z += 0.5f;
		delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
		forceVec = delta * sqrt(speed) * VEHICLE_PUSH_MULT_STD * ent->GetPhysics()->GetMass();
		ent->ApplyImpulse( this, 0, ent->GetPhysics()->GetOrigin(), forceVec );
		if ( g_debugVehicle.GetBool() ) {
			gameLocal.Printf("push speed: %f mass: %f\n", speed, ent->GetPhysics()->GetMass());
		}
		if ( predicted ) {
			//gameLocal.Printf("self ApplyImpulse:\n");
			ApplyImpulse( ent, 0, collision.endpos, -forceVec*VEHICLE_SELF_IMPULSE_MULT_STD );
		}
	} 

}

/*
=================
idAFEntity_VehicleSimple::Collide
=================
*/
bool idAFEntity_VehicleSimple::Collide( const trace_t &collision, const idVec3 &velocity ) {
	ApplyCollisionFeedback( collision, velocity, false );
	return false;
}

/*
============
idAFEntity_VehicleSimple::KickObstacles
============
*/
void idAFEntity_VehicleSimple::PredictCollisions( void ) {
	idAFBody *body;
	idVec3 org, dest;
	trace_t	trace;

	const idVec3 &linearVel = af.GetPhysics()->GetLinearVelocity();
	
	if ( linearVel.LengthFast() < VEHICLE_MIN_PREDICTION_VELOCITY ) {
		//gameLocal.Printf("Too slow !\n");
		return;
	}

	body = af.GetPhysics()->GetBody( 0 );
	org = af.GetPhysics()->GetOrigin();
	dest = org + linearVel * 0.001f * VEHICLE_MAX_PREDICTION_WIDTH;
	
	if ( g_debugVehicle.GetBool() ) {
		gameRenderWorld->DebugArrow( colorMagenta, org, dest, 4 ); //test only
	}

	gameLocal.clip.Translation( trace, org, dest, body->GetClipModel(), body->GetClipModel()->GetAxis(), /*af.GetPhysics()->GetClipMask()*/ MASK_SOLID|CONTENTS_RENDERMODEL, this );
	if ( trace.fraction != 1.0f ) {
		ApplyCollisionFeedback( trace, linearVel, true );
	}
}

/*
============
idAFEntity_VehicleSimple::Save
============
*/
void idAFEntity_VehicleSimple::Save( idSaveGame *savefile ) const {
	//gameLocal.Printf("idAFEntity_VehicleSimple::Save\n");
	savefile->WriteFloat(vehicleSuspensionUp);
	savefile->WriteFloat(vehicleSuspensionDown);
	savefile->WriteFloat(vehicleSuspensionKCompress);
	savefile->WriteFloat(vehicleSuspensionDamping);
	savefile->WriteFloat(vehicleTireFriction);

	savefile->WriteClipModel(wheelModel);
	int wheel;
	for(wheel = 0; wheel < 4; wheel++) {
		savefile->WriteJoint(wheelJoints[wheel]);
		savefile->WriteFloat(wheelAngles[wheel]);
	}

	savefile->WriteString( damage );
	savefile->WriteString( fxCollide );
	savefile->WriteInt( nextCollideFxTime );
}

/*
============
idAFEntity_VehicleSimple::Restore
============
*/
void idAFEntity_VehicleSimple::Restore( idRestoreGame *savefile ) {
	idVec3 origin;
	idMat3 axis;
	int wheel;

	savefile->ReadFloat(vehicleSuspensionUp);
	savefile->ReadFloat(vehicleSuspensionDown);
	savefile->ReadFloat(vehicleSuspensionKCompress);
	savefile->ReadFloat(vehicleSuspensionDamping);
	savefile->ReadFloat(vehicleTireFriction);

	//gameLocal.Printf("idAFEntity_VehicleSimple::Restore\n");
	savefile->ReadClipModel(wheelModel);
	for(wheel = 0; wheel < 4; wheel++) {
		savefile->ReadJoint(wheelJoints[wheel]);
		savefile->ReadFloat(wheelAngles[wheel]);
	  
		suspension[wheel] = static_cast<idAFConstraint_Suspension *>(af.GetPhysics()->GetConstraint( wheel ));
		suspension[wheel]->Setup( va( "suspension%d", wheel ), af.GetPhysics()->GetBody( 0 ), wheelModel );
		/*
		GetAnimator()->GetJointTransform( wheelJoints[wheel], 0, origin, axis );

		origin = renderEntity.origin + origin * renderEntity.axis;
		suspension[wheel]->Setup( va( "suspension%d", wheel ), af.GetPhysics()->GetBody( 0 ), origin, af.GetPhysics()->GetAxis( 0 ), wheelModel );
		*/
	}

	savefile->ReadString( damage );
	savefile->ReadString( fxCollide );
	savefile->ReadInt( nextCollideFxTime );
}


void idAFEntity_VehicleSimple::RecreateDynamicConstraints( idList<idAFConstraint *> *constraints ){
	int i;
	for(i=0;i<4;i++) {
         idAFConstraint_Suspension *constraint = new idAFConstraint_Suspension();
         constraints->Append(constraint);
    }
	/*
	gameLocal.Printf("RecreateDynamicConstraints\n");
	if ( af.GetPhysics() != NULL ) {
		gameLocal.Printf("af.GetPhysics() ok\n");
	}
	*/
}

/*
==================
idAFEntity_VehicleSimple::Event_SetTireFriction
==================
*/
void idAFEntity_VehicleSimple::Event_SetTireFriction( float tireFriction ) {
	vehicleTireFriction = tireFriction;

	for ( int i = 0; i < 4; i++ ) {
		suspension[i]->SetSuspension(	vehicleSuspensionUp,
										vehicleSuspensionDown,
										vehicleSuspensionKCompress,
										vehicleSuspensionDamping,
										vehicleTireFriction );
	}
}
//ivan end

/*
===============================================================================

  idAFEntity_VehicleFourWheels

===============================================================================
*/

CLASS_DECLARATION( idAFEntity_Vehicle, idAFEntity_VehicleFourWheels )
END_CLASS


/*
================
idAFEntity_VehicleFourWheels::idAFEntity_VehicleFourWheels
================
*/
idAFEntity_VehicleFourWheels::idAFEntity_VehicleFourWheels( void ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		wheels[i]		= NULL;
		wheelJoints[i]	= INVALID_JOINT;
		wheelAngles[i]	= 0.0f;
	}
	steering[0]			= NULL;
	steering[1]			= NULL;
}

/*
================
idAFEntity_VehicleFourWheels::Spawn
================
*/
void idAFEntity_VehicleFourWheels::Spawn( void ) {
	int i;
	static const char *wheelBodyKeys[] = {
		"wheelBodyFrontLeft",
		"wheelBodyFrontRight",
		"wheelBodyRearLeft",
		"wheelBodyRearRight"
	};
	static const char *wheelJointKeys[] = {
		"wheelJointFrontLeft",
		"wheelJointFrontRight",
		"wheelJointRearLeft",
		"wheelJointRearRight"
	};
	static const char *steeringHingeKeys[] = {
		"steeringHingeFrontLeft",
		"steeringHingeFrontRight",
	};

	const char *wheelBodyName, *wheelJointName, *steeringHingeName;

	for ( i = 0; i < 4; i++ ) {
		wheelBodyName = spawnArgs.GetString( wheelBodyKeys[i], "" );
		if ( !wheelBodyName[0] ) {
			gameLocal.Error( "idAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), wheelBodyKeys[i] );
		}
		wheels[i] = af.GetPhysics()->GetBody( wheelBodyName );
		if ( !wheels[i] ) {
			gameLocal.Error( "idAFEntity_VehicleFourWheels '%s' can't find wheel body '%s'", name.c_str(), wheelBodyName );
		}
		wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "idAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idAFEntity_VehicleFourWheels '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}
	}

	for ( i = 0; i < 2; i++ ) {
		steeringHingeName = spawnArgs.GetString( steeringHingeKeys[i], "" );
		if ( !steeringHingeName[0] ) {
			gameLocal.Error( "idAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), steeringHingeKeys[i] );
		}
		steering[i] = static_cast<idAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( steeringHingeName ));
		if ( !steering[i] ) {
			gameLocal.Error( "idAFEntity_VehicleFourWheels '%s': can't find steering hinge '%s'", name.c_str(), steeringHingeName );
		}
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
idAFEntity_VehicleFourWheels::Think
================
*/
void idAFEntity_VehicleFourWheels::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	idVec3 origin;
	idMat3 axis;
	idRotation rotation;

	if ( thinkFlags & TH_THINK ) {

		if ( driver ) {
			// capture the input from a player
			velocity = vehicleVelocity; //ivan: was: g_vehicleVelocity.GetFloat();
			if ( driver->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = idMath::Fabs( driver->usercmd.forwardmove * vehicleForce ) * (1.0f / 128.0f); //ivan: was g_vehicleForce.GetFloat()
			steerAngle = GetSteerAngle();
		}

		// update the wheel motor force
		for ( i = 0; i < 2; i++ ) {
			wheels[2+i]->SetContactMotorVelocity( velocity );
			wheels[2+i]->SetContactMotorForce( force );
		}

		// adjust wheel velocity for better steering because there are no differentials between the wheels
		if ( steerAngle < 0.0f ) {
			wheels[2]->SetContactMotorVelocity( velocity * 0.5f );
		}
		else if ( steerAngle > 0.0f ) {
			wheels[3]->SetContactMotorVelocity( velocity * 0.5f );
		}

		// update the wheel steering
		steering[0]->SetSteerAngle( steerAngle );
		steering[1]->SetSteerAngle( steerAngle );
		for ( i = 0; i < 2; i++ ) {
			steering[i]->SetSteerSpeed( 3.0f );
		}

		// update the steering wheel
		animator.GetJointTransform( steeringWheelJoint, gameLocal.time, origin, axis );
		rotation.SetVec( axis[2] );
		rotation.SetAngle( -steerAngle );
		animator.SetJointAxis( steeringWheelJoint, JOINTMOD_WORLD, rotation.ToMat3() );

		// run the physics
		RunPhysics();

		// rotate the wheels visually
		for ( i = 0; i < 4; i++ ) {
			if ( force == 0.0f ) {
				velocity = wheels[i]->GetLinearVelocity() * wheels[i]->GetWorldAxis()[0];
			}
			wheelAngles[i] += velocity * MS2SEC( gameLocal.msec ) / wheelRadius;
			// give the wheel joint an additional rotation about the wheel axis
			rotation.SetAngle( RAD2DEG( wheelAngles[i] ) );
			axis = af.GetPhysics()->GetAxis( 0 );
			rotation.SetVec( (wheels[i]->GetWorldAxis() * axis.Transpose())[2] );
			animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, rotation.ToMat3() );
		}

		// spawn dust particle effects
		if ( force != 0.0f && !( gameLocal.framenum & 7 ) ) {
			int numContacts;
			idAFConstraint_Contact *contacts[2];
			for ( i = 0; i < 4; i++ ) {
				numContacts = af.GetPhysics()->GetBodyContactConstraints( wheels[i]->GetClipModel()->GetId(), contacts, 2 );
				//gameLocal.Printf("numContacts: %d\n", numContacts);
				for ( int j = 0; j < numContacts; j++ ) {
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3(), timeGroup /* D3XP */ );
				}
			}
		}
	}

	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}


/*
===============================================================================

  idAFEntity_VehicleSixWheels

===============================================================================
*/

CLASS_DECLARATION( idAFEntity_Vehicle, idAFEntity_VehicleSixWheels )
END_CLASS

	/*
================
idAFEntity_VehicleSixWheels::idAFEntity_VehicleSixWheels
================
*/
idAFEntity_VehicleSixWheels::idAFEntity_VehicleSixWheels( void ) {
	int i;

	for ( i = 0; i < 6; i++ ) {
		wheels[i]		= NULL;
		wheelJoints[i]	= INVALID_JOINT;
		wheelAngles[i]	= 0.0f;
	}
	steering[0]			= NULL;
	steering[1]			= NULL;
	steering[2]			= NULL;
	steering[3]			= NULL;
}

/*
================
idAFEntity_VehicleSixWheels::Spawn
================
*/
void idAFEntity_VehicleSixWheels::Spawn( void ) {
	int i;
	static const char *wheelBodyKeys[] = {
		"wheelBodyFrontLeft",
		"wheelBodyFrontRight",
		"wheelBodyMiddleLeft",
		"wheelBodyMiddleRight",
		"wheelBodyRearLeft",
		"wheelBodyRearRight"
	};
	static const char *wheelJointKeys[] = {
		"wheelJointFrontLeft",
		"wheelJointFrontRight",
		"wheelJointMiddleLeft",
		"wheelJointMiddleRight",
		"wheelJointRearLeft",
		"wheelJointRearRight"
	};
	static const char *steeringHingeKeys[] = {
		"steeringHingeFrontLeft",
		"steeringHingeFrontRight",
		"steeringHingeRearLeft",
		"steeringHingeRearRight"
	};

	const char *wheelBodyName, *wheelJointName, *steeringHingeName;

	for ( i = 0; i < 6; i++ ) {
		wheelBodyName = spawnArgs.GetString( wheelBodyKeys[i], "" );
		if ( !wheelBodyName[0] ) {
			gameLocal.Error( "idAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), wheelBodyKeys[i] );
		}
		wheels[i] = af.GetPhysics()->GetBody( wheelBodyName );
		if ( !wheels[i] ) {
			gameLocal.Error( "idAFEntity_VehicleSixWheels '%s' can't find wheel body '%s'", name.c_str(), wheelBodyName );
		}
		wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "idAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idAFEntity_VehicleSixWheels '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}
	}

	for ( i = 0; i < 4; i++ ) {
		steeringHingeName = spawnArgs.GetString( steeringHingeKeys[i], "" );
		if ( !steeringHingeName[0] ) {
			gameLocal.Error( "idAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), steeringHingeKeys[i] );
		}
		steering[i] = static_cast<idAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( steeringHingeName ));
		if ( !steering[i] ) {
			gameLocal.Error( "idAFEntity_VehicleSixWheels '%s': can't find steering hinge '%s'", name.c_str(), steeringHingeName );
		}
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
idAFEntity_VehicleSixWheels::Think
================
*/
void idAFEntity_VehicleSixWheels::Think( void ) {
	int i;
#ifndef _D3XP
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
#endif
	idVec3 origin;
	idMat3 axis;
	idRotation rotation;

	if ( thinkFlags & TH_THINK ) {

		if ( driver ) {
			// capture the input from a player
			velocity = vehicleVelocity; //ivan - was: g_vehicleVelocity.GetFloat();
			if ( driver->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = idMath::Fabs( driver->usercmd.forwardmove * vehicleForce ) * (1.0f / 128.0f); //ivan - was: g_vehicleVelocity.GetFloat();
			steerAngle = GetSteerAngle();
		}

		// update the wheel motor force
		for ( i = 0; i < 6; i++ ) {
			wheels[i]->SetContactMotorVelocity( velocity );
			wheels[i]->SetContactMotorForce( force );
		}

		// adjust wheel velocity for better steering because there are no differentials between the wheels
		if ( steerAngle < 0.0f ) {
			for ( i = 0; i < 3; i++ ) {
				wheels[(i<<1)]->SetContactMotorVelocity( velocity * 0.5f );
			}
		}
		else if ( steerAngle > 0.0f ) {
			for ( i = 0; i < 3; i++ ) {
				wheels[1+(i<<1)]->SetContactMotorVelocity( velocity * 0.5f );
			}
		}

		// update the wheel steering
		steering[0]->SetSteerAngle( steerAngle );
		steering[1]->SetSteerAngle( steerAngle );
		steering[2]->SetSteerAngle( -steerAngle );
		steering[3]->SetSteerAngle( -steerAngle );
		for ( i = 0; i < 4; i++ ) {
			steering[i]->SetSteerSpeed( 3.0f );
		}

		// update the steering wheel
		animator.GetJointTransform( steeringWheelJoint, gameLocal.time, origin, axis );
		rotation.SetVec( axis[2] );
		rotation.SetAngle( -steerAngle );
		animator.SetJointAxis( steeringWheelJoint, JOINTMOD_WORLD, rotation.ToMat3() );

		// run the physics
		RunPhysics();

		// rotate the wheels visually
		for ( i = 0; i < 6; i++ ) {
			if ( force == 0.0f ) {
				velocity = wheels[i]->GetLinearVelocity() * wheels[i]->GetWorldAxis()[0];
			}
			wheelAngles[i] += velocity * MS2SEC( gameLocal.msec ) / wheelRadius;
			// give the wheel joint an additional rotation about the wheel axis
			rotation.SetAngle( RAD2DEG( wheelAngles[i] ) );
			axis = af.GetPhysics()->GetAxis( 0 );
			rotation.SetVec( (wheels[i]->GetWorldAxis() * axis.Transpose())[2] );
			animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, rotation.ToMat3() );
		}

		// spawn dust particle effects
		if ( force != 0.0f && !( gameLocal.framenum & 7 ) ) {
			int numContacts;
			idAFConstraint_Contact *contacts[2];
			for ( i = 0; i < 6; i++ ) {
				numContacts = af.GetPhysics()->GetBodyContactConstraints( wheels[i]->GetClipModel()->GetId(), contacts, 2 );
				for ( int j = 0; j < numContacts; j++ ) {
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3(), timeGroup /* D3XP */ );
				}
			}
		}
	}

	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}

#ifdef _D3XP
/*
===============================================================================

idAFEntity_VehicleAutomated

===============================================================================
*/
const idEventDef EV_Vehicle_setVelocity( "setVelocity", "f" );
const idEventDef EV_Vehicle_setTorque( "setTorque", "f" );
const idEventDef EV_Vehicle_setSteeringSpeed( "setSteeringSpeed", "f" );
const idEventDef EV_Vehicle_setWaypoint( "setWaypoint", "e" );

CLASS_DECLARATION( idAFEntity_VehicleSixWheels, idAFEntity_VehicleAutomated )
EVENT( EV_PostSpawn,				idAFEntity_VehicleAutomated::PostSpawn )
EVENT( EV_Vehicle_setVelocity,		idAFEntity_VehicleAutomated::Event_SetVelocity )
EVENT( EV_Vehicle_setTorque,		idAFEntity_VehicleAutomated::Event_SetTorque )
EVENT( EV_Vehicle_setSteeringSpeed,	idAFEntity_VehicleAutomated::Event_SetSteeringSpeed )
EVENT( EV_Vehicle_setWaypoint,		idAFEntity_VehicleAutomated::Event_SetWayPoint )
END_CLASS

/*
================
idAFEntity_VehicleAutomated::Spawn
================
*/
void idAFEntity_VehicleAutomated::Spawn( void ) {

	velocity = force = steerAngle = 0.f;
	currentSteering = steeringSpeed = 0.f;
	originHeight = 0.f;
	waypoint = NULL;

	spawnArgs.GetFloat( "velocity", "150", velocity );
	spawnArgs.GetFloat( "torque", "200000", force );
	spawnArgs.GetFloat( "steeringSpeed", "1", steeringSpeed );
	spawnArgs.GetFloat( "originHeight", "0", originHeight );

	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
idAFEntity_VehicleAutomated::PostSpawn
================
*/
void idAFEntity_VehicleAutomated::PostSpawn( void ) {

	if ( targets.Num() ) {
		waypoint = targets[0].GetEntity();
	}
}

/*
================
idAFEntity_VehicleAutomated::Event_SetVelocity
================
*/
void idAFEntity_VehicleAutomated::Event_SetVelocity( float _velocity ) {
	velocity = _velocity;
}

/*
================
idAFEntity_VehicleAutomated::Event_SetTorque
================
*/
void idAFEntity_VehicleAutomated::Event_SetTorque( float _torque ) {
	force = _torque;
}

/*
================
idAFEntity_VehicleAutomated::Event_SetSteeringSpeed
================
*/
void idAFEntity_VehicleAutomated::Event_SetSteeringSpeed( float _steeringSpeed ) {
	steeringSpeed = _steeringSpeed;
}

/*
================
idAFEntity_VehicleAutomated::Event_SetWayPoint
================
*/
void idAFEntity_VehicleAutomated::Event_SetWayPoint( idEntity *_waypoint ) {
	waypoint = _waypoint;
}

/*
================
idAFEntity_VehicleAutomated::Think
================
*/
#define	HIT_WAYPOINT_THRESHOLD	80.f

void idAFEntity_VehicleAutomated::Think( void ) {

	// If we don't have a waypoint, coast to a stop
	if ( !waypoint ) {
		velocity = force = steerAngle = 0.f;
		idAFEntity_VehicleSixWheels::Think();
		return;
	}

	idVec3 waypoint_origin, vehicle_origin;
	idVec3 travel_vector;
	float distance_from_waypoint;

	// Set up the vector from the vehicle origin, to the waypoint
	vehicle_origin = GetPhysics()->GetOrigin();
	vehicle_origin.z -= originHeight;

	waypoint_origin = waypoint->GetPhysics()->GetOrigin();

	travel_vector = waypoint_origin - vehicle_origin;
	distance_from_waypoint = travel_vector.Length();

	// Check if we've hit the waypoint (within a certain threshold)
	if ( distance_from_waypoint < HIT_WAYPOINT_THRESHOLD ) {
		idStr				callfunc;
		const function_t	*func;
		idThread			*thread;

		// Waypoints can call script functions
		waypoint->spawnArgs.GetString( "call", "", callfunc );
		if ( callfunc.Length() ) {
			func = gameLocal.program.FindFunction( callfunc );
			if ( func != NULL ) {
				thread = new idThread( func );
				thread->DelayedStart( 0 );
			}
		}

		// Get next waypoint
		if ( waypoint->targets.Num() ) {
			waypoint = waypoint->targets[0].GetEntity();
		} else {
			waypoint = NULL;
		}

		// We are switching waypoints, adjust steering next frame
		idAFEntity_VehicleSixWheels::Think();
		return;
	}

	idAngles vehicle_angles, travel_angles;

	// Get the angles we need to steer towards
	travel_angles = travel_vector.ToAngles().Normalize360();
	vehicle_angles = this->GetPhysics()->GetAxis().ToAngles().Normalize360();

	float	delta_yaw;

	// Get the shortest steering angle towards the travel angles
	delta_yaw = vehicle_angles.yaw - travel_angles.yaw;
	if ( idMath::Fabs( delta_yaw ) > 180.f ) {
		if ( delta_yaw > 0 ) {
			delta_yaw = delta_yaw - 360;
		} else {
			delta_yaw = delta_yaw + 360;
		}
	}

	// Maximum steering angle is 35 degrees
	delta_yaw = idMath::ClampFloat( -35.f, 35.f, delta_yaw );

	idealSteering = delta_yaw;

	// Adjust steering incrementally so it doesn't snap to the ideal angle
	if ( idMath::Fabs( (idealSteering - currentSteering) ) > steeringSpeed ) {
		if ( idealSteering > currentSteering ) {
			currentSteering += steeringSpeed;
		} else {
			currentSteering -= steeringSpeed;
		}
	} else {
		currentSteering = idealSteering;
	}

	// DEBUG
	if ( g_vehicleDebug.GetBool() ) {
		gameRenderWorld->DebugBounds( colorRed, idBounds(idVec3(-4,-4,-4),idVec3(4,4,4)), vehicle_origin );
		gameRenderWorld->DebugBounds( colorRed, idBounds(idVec3(-4,-4,-4),idVec3(4,4,4)), waypoint_origin );
		gameRenderWorld->DrawText( waypoint->name.c_str(), waypoint_origin + idVec3(0,0,16), 0.25f, colorYellow, gameLocal.GetLocalPlayer()->viewAxis );
		gameRenderWorld->DebugArrow( colorWhite, vehicle_origin, waypoint_origin, 12 );
	}

	// Set the final steerAngle for the vehicle
	steerAngle = currentSteering;

	idAFEntity_VehicleSixWheels::Think();
}
#endif

/*
===============================================================================

  idAFEntity_SteamPipe

===============================================================================
*/

CLASS_DECLARATION( idAFEntity_Base, idAFEntity_SteamPipe )
END_CLASS


/*
================
idAFEntity_SteamPipe::idAFEntity_SteamPipe
================
*/
idAFEntity_SteamPipe::idAFEntity_SteamPipe( void ) {
	steamBody			= 0;
	steamForce			= 0.0f;
	steamUpForce		= 0.0f;
	steamModelDefHandle	= -1;
	memset( &steamRenderEntity, 0, sizeof( steamRenderEntity ) );
}

/*
================
idAFEntity_SteamPipe::~idAFEntity_SteamPipe
================
*/
idAFEntity_SteamPipe::~idAFEntity_SteamPipe( void ) {
	if ( steamModelDefHandle >= 0 ){
		gameRenderWorld->FreeEntityDef( steamModelDefHandle );
	}
}

/*
================
idAFEntity_SteamPipe::Save
================
*/
void idAFEntity_SteamPipe::Save( idSaveGame *savefile ) const {
}

/*
================
idAFEntity_SteamPipe::Restore
================
*/
void idAFEntity_SteamPipe::Restore( idRestoreGame *savefile ) {
	Spawn();
}

/*
================
idAFEntity_SteamPipe::Spawn
================
*/
void idAFEntity_SteamPipe::Spawn( void ) {
	idVec3 steamDir;
	const char *steamBodyName;

	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	steamBodyName = spawnArgs.GetString( "steamBody", "" );
	steamForce = spawnArgs.GetFloat( "steamForce", "2000" );
	steamUpForce = spawnArgs.GetFloat( "steamUpForce", "10" );
	steamDir = af.GetPhysics()->GetAxis( steamBody )[2];
	steamBody = af.GetPhysics()->GetBodyId( steamBodyName );
	force.SetPosition( af.GetPhysics(), steamBody, af.GetPhysics()->GetOrigin( steamBody ) );
	force.SetForce( steamDir * -steamForce );

	InitSteamRenderEntity();

	BecomeActive( TH_THINK );
}

/*
================
idAFEntity_SteamPipe::InitSteamRenderEntity
================
*/
void idAFEntity_SteamPipe::InitSteamRenderEntity( void ) {
	const char	*temp;
	const idDeclModelDef *modelDef;

	memset( &steamRenderEntity, 0, sizeof( steamRenderEntity ) );
	steamRenderEntity.shaderParms[ SHADERPARM_RED ]		= 1.0f;
	steamRenderEntity.shaderParms[ SHADERPARM_GREEN ]	= 1.0f;
	steamRenderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
	modelDef = NULL;
	temp = spawnArgs.GetString ( "model_steam" );
	if ( *temp != '\0' ) {
		if ( !strstr( temp, "." ) ) {
			modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
			if ( modelDef ) {
				steamRenderEntity.hModel = modelDef->ModelHandle();
			}
		}

		if ( !steamRenderEntity.hModel ) {
			steamRenderEntity.hModel = renderModelManager->FindModel( temp );
		}

		if ( steamRenderEntity.hModel ) {
			steamRenderEntity.bounds = steamRenderEntity.hModel->Bounds( &steamRenderEntity );
		} else {
			steamRenderEntity.bounds.Zero();
		}
		steamRenderEntity.origin = af.GetPhysics()->GetOrigin( steamBody );
		steamRenderEntity.axis = af.GetPhysics()->GetAxis( steamBody );
		steamModelDefHandle = gameRenderWorld->AddEntityDef( &steamRenderEntity );
	}
}

/*
================
idAFEntity_SteamPipe::Think
================
*/
void idAFEntity_SteamPipe::Think( void ) {
	idVec3 steamDir;

	if ( thinkFlags & TH_THINK ) {
		steamDir.x = gameLocal.random.CRandomFloat() * steamForce;
		steamDir.y = gameLocal.random.CRandomFloat() * steamForce;
		steamDir.z = steamUpForce;
		force.SetForce( steamDir );
		force.Evaluate( gameLocal.time );
		//gameRenderWorld->DebugArrow( colorWhite, af.GetPhysics()->GetOrigin( steamBody ), af.GetPhysics()->GetOrigin( steamBody ) - 10.0f * steamDir, 4 );
	}

	if ( steamModelDefHandle >= 0 ){
		steamRenderEntity.origin = af.GetPhysics()->GetOrigin( steamBody );
		steamRenderEntity.axis = af.GetPhysics()->GetAxis( steamBody );
		gameRenderWorld->UpdateEntityDef( steamModelDefHandle, &steamRenderEntity );
	}

	idAFEntity_Base::Think();
}


/*
===============================================================================

  idAFEntity_ClawFourFingers

===============================================================================
*/

const idEventDef EV_SetFingerAngle( "setFingerAngle", "f" );
const idEventDef EV_StopFingers( "stopFingers" );

CLASS_DECLARATION( idAFEntity_Base, idAFEntity_ClawFourFingers )
	EVENT( EV_SetFingerAngle,		idAFEntity_ClawFourFingers::Event_SetFingerAngle )
	EVENT( EV_StopFingers,			idAFEntity_ClawFourFingers::Event_StopFingers )
END_CLASS

static const char *clawConstraintNames[] = {
	"claw1", "claw2", "claw3", "claw4"
};

/*
================
idAFEntity_ClawFourFingers::idAFEntity_ClawFourFingers
================
*/
idAFEntity_ClawFourFingers::idAFEntity_ClawFourFingers( void ) {
	fingers[0]	= NULL;
	fingers[1]	= NULL;
	fingers[2]	= NULL;
	fingers[3]	= NULL;
}

/*
================
idAFEntity_ClawFourFingers::Save
================
*/
void idAFEntity_ClawFourFingers::Save( idSaveGame *savefile ) const {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->Save( savefile );
	}
}

/*
================
idAFEntity_ClawFourFingers::Restore
================
*/
void idAFEntity_ClawFourFingers::Restore( idRestoreGame *savefile ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i] = static_cast<idAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( clawConstraintNames[i] ));
		fingers[i]->Restore( savefile );
	}

	SetCombatModel();
	LinkCombat();
}

/*
================
idAFEntity_ClawFourFingers::Spawn
================
*/
void idAFEntity_ClawFourFingers::Spawn( void ) {
	int i;

	LoadAF();

	SetCombatModel();

	af.GetPhysics()->LockWorldConstraints( true );
	af.GetPhysics()->SetForcePushable( true );
	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	for ( i = 0; i < 4; i++ ) {
		fingers[i] = static_cast<idAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( clawConstraintNames[i] ));
		if ( !fingers[i] ) {
			gameLocal.Error( "idClaw_FourFingers '%s': can't find claw constraint '%s'", name.c_str(), clawConstraintNames[i] );
		}
	}
}

/*
================
idAFEntity_ClawFourFingers::Event_SetFingerAngle
================
*/
void idAFEntity_ClawFourFingers::Event_SetFingerAngle( float angle ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->SetSteerAngle( angle );
		fingers[i]->SetSteerSpeed( 0.5f );
	}
	af.GetPhysics()->Activate();
}

/*
================
idAFEntity_ClawFourFingers::Event_StopFingers
================
*/
void idAFEntity_ClawFourFingers::Event_StopFingers( void ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->SetSteerAngle( fingers[i]->GetAngle() );
	}
}


/*
===============================================================================

  editor support routines

===============================================================================
*/


/*
================
idGameEdit::AF_SpawnEntity
================
*/
bool idGameEdit::AF_SpawnEntity( const char *fileName ) {
	idDict args;
	idPlayer *player;
	idAFEntity_Generic *ent;
	const idDeclAF *af;
	idVec3 org;
	float yaw;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return false;
	}

	af = static_cast<const idDeclAF *>( declManager->FindType( DECL_AF, fileName ) );
	if ( !af ) {
		return false;
	}

	yaw = player->viewAngles.yaw;
	args.Set( "angle", va( "%f", yaw + 180 ) );
	org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 );
	args.Set( "origin", org.ToString() );
	args.Set( "spawnclass", "idAFEntity_Generic" );
	if ( af->model[0] ) {
		args.Set( "model", af->model.c_str() );
	} else {
		args.Set( "model", fileName );
	}
	if ( af->skin[0] ) {
		args.Set( "skin", af->skin.c_str() );
	}
	args.Set( "articulatedFigure", fileName );
	args.Set( "nodrop", "1" );
	ent = static_cast<idAFEntity_Generic *>(gameLocal.SpawnEntityType( idAFEntity_Generic::Type, &args));

	// always update this entity
	ent->BecomeActive( TH_THINK );
	ent->KeepRunningPhysics();
	ent->fl.forcePhysicsUpdate = true;

	player->dragEntity.SetSelected( ent );

	return true;
}

/*
================
idGameEdit::AF_UpdateEntities
================
*/
void idGameEdit::AF_UpdateEntities( const char *fileName ) {
	idEntity *ent;
	idAFEntity_Base *af;
	idStr name;

	name = fileName;
	name.StripFileExtension();

	// reload any idAFEntity_Generic which uses the given articulated figure file
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( idAFEntity_Base::Type ) ) {
			af = static_cast<idAFEntity_Base *>(ent);
			if ( name.Icmp( af->GetAFName() ) == 0 ) {
				af->LoadAF();
				af->GetAFPhysics()->PutToRest();
			}
		}
	}
}

/*
================
idGameEdit::AF_UndoChanges
================
*/
void idGameEdit::AF_UndoChanges( void ) {
	int i, c;
	idEntity *ent;
	idAFEntity_Base *af;
	idDeclAF *decl;

	c = declManager->GetNumDecls( DECL_AF );
	for ( i = 0; i < c; i++ ) {
		decl = static_cast<idDeclAF *>( const_cast<idDecl *>( declManager->DeclByIndex( DECL_AF, i, false ) ) );
		if ( !decl->modified ) {
			continue;
		}

		decl->Invalidate();
		declManager->FindType( DECL_AF, decl->GetName() );

		// reload all AF entities using the file
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( ent->IsType( idAFEntity_Base::Type ) ) {
				af = static_cast<idAFEntity_Base *>(ent);
				if ( idStr::Icmp( decl->GetName(), af->GetAFName() ) == 0 ) {
					af->LoadAF();
				}
			}
		}
	}
}

/*
================
GetJointTransform
================
*/
typedef struct {
	renderEntity_t *ent;
	const idMD5Joint *joints;
} jointTransformData_t;

static bool GetJointTransform( void *model, const idJointMat *frame, const char *jointName, idVec3 &origin, idMat3 &axis ) {
	int i;
	jointTransformData_t *data = reinterpret_cast<jointTransformData_t *>(model);

	for ( i = 0; i < data->ent->numJoints; i++ ) {
		if ( data->joints[i].name.Icmp( jointName ) == 0 ) {
			break;
		}
	}
	if ( i >= data->ent->numJoints ) {
		return false;
	}
	origin = frame[i].ToVec3();
	axis = frame[i].ToMat3();
	return true;
}

/*
================
GetArgString
================
*/
static const char *GetArgString( const idDict &args, const idDict *defArgs, const char *key ) {
	const char *s;

	s = args.GetString( key );
	if ( !s[0] && defArgs ) {
		s = defArgs->GetString( key );
	}
	return s;
}

/*
================
idGameEdit::AF_CreateMesh
================
*/
idRenderModel *idGameEdit::AF_CreateMesh( const idDict &args, idVec3 &meshOrigin, idMat3 &meshAxis, bool &poseIsSet ) {
	int i, jointNum;
	const idDeclAF *af;
	const idDeclAF_Body *fb;
	renderEntity_t ent;
	idVec3 origin, *bodyOrigin, *newBodyOrigin, *modifiedOrigin;
	idMat3 axis, *bodyAxis, *newBodyAxis, *modifiedAxis;
	declAFJointMod_t *jointMod;
	idAngles angles;
	const idDict *defArgs;
	const idKeyValue *arg;
	idStr name;
	jointTransformData_t data;
	const char *classname, *afName, *modelName;
	idRenderModel *md5;
	const idDeclModelDef *modelDef;
	const idMD5Anim *MD5anim;
	const idMD5Joint *MD5joint;
	const idMD5Joint *MD5joints;
	int numMD5joints;
	idJointMat *originalJoints;
	int parentNum;

	poseIsSet = false;
	meshOrigin.Zero();
	meshAxis.Identity();

	classname = args.GetString( "classname" );
	defArgs = gameLocal.FindEntityDefDict( classname );

	// get the articulated figure
	afName = GetArgString( args, defArgs, "articulatedFigure" );
	af = static_cast<const idDeclAF *>( declManager->FindType( DECL_AF, afName ) );
	if ( !af ) {
		return NULL;
	}

	// get the md5 model
	modelName = GetArgString( args, defArgs, "model" );
	modelDef = static_cast< const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
	if ( !modelDef ) {
		return NULL;
	}

	// make sure model hasn't been purged
	if ( modelDef->ModelHandle() && !modelDef->ModelHandle()->IsLoaded() ) {
		modelDef->ModelHandle()->LoadModel();
	}

	// get the md5
	md5 = modelDef->ModelHandle();
	if ( !md5 || md5->IsDefaultModel() ) {
		return NULL;
	}

	// get the articulated figure pose anim
	int animNum = modelDef->GetAnim( "af_pose" );
	if ( !animNum ) {
		return NULL;
	}
	const idAnim *anim = modelDef->GetAnim( animNum );
	if ( !anim ) {
		return NULL;
	}
	MD5anim = anim->MD5Anim( 0 );
	MD5joints = md5->GetJoints();
	numMD5joints = md5->NumJoints();

	// setup a render entity
	memset( &ent, 0, sizeof( ent ) );
	ent.customSkin = modelDef->GetSkin();
	ent.bounds.Clear();
	ent.numJoints = numMD5joints;
	ent.joints = ( idJointMat * )_alloca16( ent.numJoints * sizeof( *ent.joints ) );

	// create animation from of the af_pose
	ANIM_CreateAnimFrame( md5, MD5anim, ent.numJoints, ent.joints, 1, modelDef->GetVisualOffset(), false );

	// buffers to store the initial origin and axis for each body
	bodyOrigin = (idVec3 *) _alloca16( af->bodies.Num() * sizeof( idVec3 ) );
	bodyAxis = (idMat3 *) _alloca16( af->bodies.Num() * sizeof( idMat3 ) );
	newBodyOrigin = (idVec3 *) _alloca16( af->bodies.Num() * sizeof( idVec3 ) );
	newBodyAxis = (idMat3 *) _alloca16( af->bodies.Num() * sizeof( idMat3 ) );

	// finish the AF positions
	data.ent = &ent;
	data.joints = MD5joints;
	af->Finish( GetJointTransform, ent.joints, &data );

	// get the initial origin and axis for each AF body
	for ( i = 0; i < af->bodies.Num(); i++ ) {
		fb = af->bodies[i];

		if ( fb->modelType == TRM_BONE ) {
			// axis of bone trace model
			axis[2] = fb->v2.ToVec3() - fb->v1.ToVec3();
			axis[2].Normalize();
			axis[2].NormalVectors( axis[0], axis[1] );
			axis[1] = -axis[1];
		} else {
			axis = fb->angles.ToMat3();
		}

		newBodyOrigin[i] = bodyOrigin[i] = fb->origin.ToVec3();
		newBodyAxis[i] = bodyAxis[i] = axis;
	}

	// get any new body transforms stored in the key/value pairs
	for ( arg = args.MatchPrefix( "body ", NULL ); arg; arg = args.MatchPrefix( "body ", arg ) ) {
		name = arg->GetKey();
		name.Strip( "body " );
		for ( i = 0; i < af->bodies.Num(); i++ ) {
			fb = af->bodies[i];
			if ( fb->name.Icmp( name ) == 0 ) {
				break;
			}
		}
		if ( i >= af->bodies.Num() ) {
			continue;
		}
		sscanf( arg->GetValue(), "%f %f %f %f %f %f", &origin.x, &origin.y, &origin.z, &angles.pitch, &angles.yaw, &angles.roll );

		if ( fb->jointName.Icmp( "origin" ) == 0 ) {
			meshAxis = bodyAxis[i].Transpose() * angles.ToMat3();
			meshOrigin = origin - bodyOrigin[i] * meshAxis;
			poseIsSet = true;
		} else {
			newBodyOrigin[i] = origin;
			newBodyAxis[i] = angles.ToMat3();
		}
	}

	// save the original joints
	originalJoints = ( idJointMat * )_alloca16( numMD5joints * sizeof( originalJoints[0] ) );
	memcpy( originalJoints, ent.joints, numMD5joints * sizeof( originalJoints[0] ) );

	// buffer to store the joint mods
	jointMod = (declAFJointMod_t *) _alloca16( numMD5joints * sizeof( declAFJointMod_t ) );
	memset( jointMod, -1, numMD5joints * sizeof( declAFJointMod_t ) );
	modifiedOrigin = (idVec3 *) _alloca16( numMD5joints * sizeof( idVec3 ) );
	memset( modifiedOrigin, 0, numMD5joints * sizeof( idVec3 ) );
	modifiedAxis = (idMat3 *) _alloca16( numMD5joints * sizeof( idMat3 ) );
	memset( modifiedAxis, 0, numMD5joints * sizeof( idMat3 ) );

	// get all the joint modifications
	for ( i = 0; i < af->bodies.Num(); i++ ) {
		fb = af->bodies[i];

		if ( fb->jointName.Icmp( "origin" ) == 0 ) {
			continue;
		}

		for ( jointNum = 0; jointNum < numMD5joints; jointNum++ ) {
			if ( MD5joints[jointNum].name.Icmp( fb->jointName ) == 0 ) {
				break;
			}
		}

		if ( jointNum >= 0 && jointNum < ent.numJoints ) {
			jointMod[ jointNum ] = fb->jointMod;
			modifiedAxis[ jointNum ] = ( bodyAxis[i] * originalJoints[jointNum].ToMat3().Transpose() ).Transpose() * ( newBodyAxis[i] * meshAxis.Transpose() );
			// FIXME: calculate correct modifiedOrigin
			modifiedOrigin[ jointNum ] = originalJoints[ jointNum ].ToVec3();
		}
	}

	// apply joint modifications to the skeleton
	MD5joint = MD5joints + 1;
	for( i = 1; i < numMD5joints; i++, MD5joint++ ) {

		parentNum = MD5joint->parent - MD5joints;
		idMat3 parentAxis = originalJoints[ parentNum ].ToMat3();
		idMat3 localm = originalJoints[i].ToMat3() * parentAxis.Transpose();
		idVec3 localt = ( originalJoints[i].ToVec3() - originalJoints[ parentNum ].ToVec3() ) * parentAxis.Transpose();

		switch( jointMod[i] ) {
			case DECLAF_JOINTMOD_ORIGIN: {
				ent.joints[ i ].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[ i ].SetTranslation( modifiedOrigin[ i ] );
				break;
			}
			case DECLAF_JOINTMOD_AXIS: {
				ent.joints[ i ].SetRotation( modifiedAxis[ i ] );
				ent.joints[ i ].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
			case DECLAF_JOINTMOD_BOTH: {
				ent.joints[ i ].SetRotation( modifiedAxis[ i ] );
				ent.joints[ i ].SetTranslation( modifiedOrigin[ i ] );
				break;
			}
			default: {
				ent.joints[ i ].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[ i ].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
		}
	}

	// instantiate a mesh using the joint information from the render entity
	return md5->InstantiateDynamicModel( &ent, NULL, NULL );
}

#ifdef _D3XP

/*
===============================================================================
idHarvestable
===============================================================================
*/

const idEventDef EV_Harvest_SpawnHarvestTrigger( "<spawnHarvestTrigger>", NULL );

CLASS_DECLARATION( idEntity, idHarvestable )
EVENT( EV_Harvest_SpawnHarvestTrigger,	idHarvestable::Event_SpawnHarvestTrigger )
EVENT( EV_Touch,						idHarvestable::Event_Touch )
END_CLASS

idHarvestable::idHarvestable() {
	trigger = NULL;
	parentEnt = NULL;
}

idHarvestable::~idHarvestable() {
	if ( trigger ) {
		delete trigger;
		trigger = NULL;
	}
}

void idHarvestable::Spawn() {

	startTime = 0;

	spawnArgs.GetFloat( "triggersize", "120", triggersize );
	spawnArgs.GetFloat( "give_delay", "3", giveDelay);
	giveDelay *= 1000;
	given = false;

	removeDelay = spawnArgs.GetFloat( "remove_delay") * 1000.0f;

	fxFollowPlayer = spawnArgs.GetBool("fx_follow_player", "1");
	fxOrient = spawnArgs.GetString("fx_orient");


}

void idHarvestable::Init(idEntity* parent) {

	assert(parent);

	parentEnt = parent;

	GetPhysics()->SetOrigin( parent->GetPhysics()->GetOrigin() );
	this->Bind(parent, true);

	//Set the skin of the entity to the harvest skin
	idStr skin = parent->spawnArgs.GetString("skin_harvest", "");
	if(skin.Length()) {
		parent->SetSkin(declManager->FindSkin(skin.c_str()));
	}

	idEntity* head = NULL;
	if(parent->IsType(idActor::Type)) {
		idActor* withHead = (idActor*)parent;
		head = withHead->GetHeadEntity();
	}
	if(parent->IsType(idAFEntity_WithAttachedHead::Type)) {
		idAFEntity_WithAttachedHead* withHead = (idAFEntity_WithAttachedHead*)parent;
		head = withHead->head.GetEntity();
	}
	if(head) {
		idStr headskin = parent->spawnArgs.GetString("skin_harvest_head", "");
		if(headskin.Length()) {
			head->SetSkin(declManager->FindSkin(headskin.c_str()));
		}
	}

	idStr sound = parent->spawnArgs.GetString("harvest_sound");
	if(sound.Length() > 0) {
		 parent->StartSound( sound.c_str(), SND_CHANNEL_ANY, 0, false, NULL);
	}


	PostEventMS( &EV_Harvest_SpawnHarvestTrigger, 0 );
}

void idHarvestable::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( triggersize );
	savefile->WriteClipModel( trigger );
	savefile->WriteFloat( giveDelay );
	savefile->WriteFloat( removeDelay );
	savefile->WriteBool( given );

	player.Save( savefile );
	savefile->WriteInt( startTime );

	savefile->WriteBool( fxFollowPlayer );
	fx.Save( savefile );
	savefile->WriteString( fxOrient );

	parentEnt.Save(savefile);
}

void idHarvestable::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( triggersize );
	savefile->ReadClipModel( trigger );
	savefile->ReadFloat( giveDelay );
	savefile->ReadFloat( removeDelay );
	savefile->ReadBool( given );

	player.Restore( savefile );
	savefile->ReadInt( startTime );

	savefile->ReadBool( fxFollowPlayer );
	fx.Restore( savefile );
	savefile->ReadString( fxOrient );

	parentEnt.Restore(savefile);
}

void idHarvestable::SetParent(idEntity* parent) {
	parentEnt = parent;
}

void idHarvestable::Think() {

	idEntity* parent = parentEnt.GetEntity();
	if(!parent) {
		return;
	}

	//Update the orientation of the box
	if(trigger && parent && !parent->GetPhysics()->IsAtRest()) {
		trigger->Link( gameLocal.clip, this, 0, parent->GetPhysics()->GetOrigin(), parent->GetPhysics()->GetAxis());
	}

	if(startTime && gameLocal.slow.time - startTime > giveDelay && ! given) {
		idPlayer *thePlayer = player.GetEntity();

		thePlayer->Give(spawnArgs.GetString("give_item"), spawnArgs.GetString("give_value"));
		thePlayer->harvest_lock = false;
		given = true;
	}

	if(startTime && gameLocal.slow.time - startTime > removeDelay) {
		parent->PostEventMS( &EV_Remove, 0 );
		PostEventMS( &EV_Remove, 0 );
	}

	if(fxFollowPlayer) {
		idEntityFx* fxEnt = fx.GetEntity();

		if(fxEnt) {
			idMat3 orientAxisLocal;
			if(GetFxOrientationAxis(orientAxisLocal)) {
				//gameRenderWorld->DebugAxis(fxEnt->GetPhysics()->GetOrigin(), orientAxisLocal);
				fxEnt->GetPhysics()->SetAxis(orientAxisLocal);
			}
		}
	}
}

/*
================
idAFEntity_Harvest::Gib
Called when the parent object has been gibbed.
================
*/
void idHarvestable::Gib() {
	//Stop any looping sound that was playing
	idEntity* parent = parentEnt.GetEntity();
	if(parent) {
		idStr sound = parent->spawnArgs.GetString("harvest_sound");
		if(sound.Length() > 0) {
			parent->StopSound(SND_CHANNEL_ANY, false);
		}
	}
}

/*
================
idAFEntity_Harvest::BeginBurn
================
*/
void idHarvestable::BeginBurn() {

	idEntity* parent = parentEnt.GetEntity();
	if(!parent) {
		return;
	}

	if(!spawnArgs.GetBool("burn")) {
		return;
	}


	//Switch Skins if the parent would like us to.
	idStr skin = parent->spawnArgs.GetString("skin_harvest_burn", "");
	if(skin.Length()) {
		parent->SetSkin(declManager->FindSkin(skin.c_str()));
	}
	parent->GetRenderEntity()->noShadow = true;
	parent->SetShaderParm( SHADERPARM_TIME_OF_DEATH, gameLocal.slow.time * 0.001f );

	idEntity* head = NULL;
	if(parent->IsType(idActor::Type)) {
		idActor* withHead = (idActor*)parent;
		head = withHead->GetHeadEntity();
	}
	if(parent->IsType(idAFEntity_WithAttachedHead::Type)) {
		idAFEntity_WithAttachedHead* withHead = (idAFEntity_WithAttachedHead*)parent;
		head = withHead->head.GetEntity();
	}
	if(head) {
		idStr headskin = parent->spawnArgs.GetString("skin_harvest_burn_head", "");
		if(headskin.Length()) {
			head->SetSkin(declManager->FindSkin(headskin.c_str()));
		}

		head->GetRenderEntity()->noShadow = true;
		head->SetShaderParm( SHADERPARM_TIME_OF_DEATH, gameLocal.slow.time * 0.001f );
	}



}

/*
================
idAFEntity_Harvest::BeginFX
================
*/
void idHarvestable::BeginFX() {
	if(strlen(spawnArgs.GetString("fx")) <= 0) {
		return;
	}

	idMat3* orientAxis = NULL;
	idMat3 orientAxisLocal;

	if(GetFxOrientationAxis(orientAxisLocal)) {
		orientAxis = &orientAxisLocal;
	}
	fx = idEntityFx::StartFx( spawnArgs.GetString("fx"), NULL, orientAxis, this, spawnArgs.GetBool("fx_bind") );
}

/*
================
idAFEntity_Harvest::CalcTriggerBounds
================
*/
void idHarvestable::CalcTriggerBounds( float size, idBounds &bounds ) {

	idEntity* parent = parentEnt.GetEntity();
	if(!parent) {
		return;
	}

	//Simple trigger bounds is the absolute bounds of the AF plus a defined size
	bounds = parent->GetPhysics()->GetAbsBounds();
	bounds.ExpandSelf(size);
	bounds[0] -= parent->GetPhysics()->GetOrigin();
	bounds[1] -= parent->GetPhysics()->GetOrigin();
}

bool idHarvestable::GetFxOrientationAxis(idMat3& mat) {

	idEntity* parent = parentEnt.GetEntity();
	if(!parent) {
		return false;
	}

	idPlayer *thePlayer = player.GetEntity();

	if(!fxOrient.Icmp("up")) {
		//Orient up
		idVec3 grav = parent->GetPhysics()->GetGravityNormal()*-1;
		idVec3 left, up;

		grav.OrthogonalBasis(left, up);
		idMat3 temp(left.x, left.y, left.z, up.x, up.y, up.z, grav.x, grav.y, grav.z);
		mat = temp;

		return true;

	} else if(!fxOrient.Icmp("weapon")) {
		//Orient the fx towards the muzzle of the weapon
		jointHandle_t	joint;
		idVec3	joint_origin;
		idMat3	joint_axis;

		joint = thePlayer->weapon.GetEntity()->GetAnimator()->GetJointHandle( spawnArgs.GetString("fx_weapon_joint") );
		if ( joint != INVALID_JOINT ) {
			thePlayer->weapon.GetEntity()->GetJointWorldTransform( joint, gameLocal.slow.time, joint_origin, joint_axis );
		} else {
			joint_origin = thePlayer->GetPhysics()->GetOrigin();
		}

		idVec3 toPlayer = joint_origin-parent->GetPhysics()->GetOrigin();
		toPlayer.NormalizeFast();

		idVec3 left, up;
		toPlayer.OrthogonalBasis(left, up);
		idMat3 temp(left.x, left.y, left.z, up.x, up.y, up.z, toPlayer.x, toPlayer.y, toPlayer.z);
		mat = temp;

		return true;

	} else if(!fxOrient.Icmp("player")) {
		//Orient the fx towards the eye of the player
		idVec3 eye = thePlayer->GetEyePosition();
		idVec3 toPlayer = eye-parent->GetPhysics()->GetOrigin();

		toPlayer.Normalize();

		idVec3 left, up;
		up.Set(0, 1, 0);
		left = toPlayer.Cross(up);
		up = left.Cross(toPlayer);


		//common->Printf("%.2f %.2f %.2f - %.2f %.2f %.2f - %.2f %.2f %.2f\n", toPlayer.x, toPlayer.y, toPlayer.z, left.x, left.y, left.z, up.x, up.y, up.z );

		idMat3 temp(left.x, left.y, left.z, up.x, up.y, up.z, toPlayer.x, toPlayer.y, toPlayer.z);

		mat = temp;

		return true;
	}

	//Returning false indicates that the orientation is not used;
	return false;
}

/*
================
idAFEntity_Harvest::Event_SpawnHarvestTrigger
================
*/
void idHarvestable::Event_SpawnHarvestTrigger( void ) {
	idBounds		bounds;

	idEntity* parent = parentEnt.GetEntity();
	if(!parent) {
		return;
	}

	CalcTriggerBounds( triggersize, bounds );

	// create a trigger clip model
	trigger = new idClipModel( idTraceModel( bounds ) );
	trigger->Link( gameLocal.clip, this, 255, parent->GetPhysics()->GetOrigin(), mat3_identity);
	trigger->SetContents( CONTENTS_TRIGGER );

	startTime = 0;
}

/*
================
idAFEntity_Harvest::Event_Touch
================
*/
void idHarvestable::Event_Touch( idEntity *other, trace_t *trace ) {

	idEntity* parent = parentEnt.GetEntity();
	if(!parent) {
		return;
	}
	if(parent->IsType(idAFEntity_Gibbable::Type)) {
		idAFEntity_Gibbable* gibParent = (idAFEntity_Gibbable*)parent;
		if(gibParent->IsGibbed())
			return;
	}


	if(!startTime && other && other->IsType(idPlayer::Type)) {
		idPlayer *thePlayer = static_cast<idPlayer *>(other);

		if(thePlayer->harvest_lock) {
			//Don't harvest if the player is in mid harvest
			return;
		}

		player = thePlayer;

		bool okToGive = true;
		idStr requiredWeapons = spawnArgs.GetString("required_weapons");

		if(requiredWeapons.Length() > 0) {
			idStr playerWeap = thePlayer->GetCurrentWeapon();
			if(playerWeap.Length() == 0 || requiredWeapons.Find(playerWeap, false) == -1) {
				okToGive = false;
			}
		}

		if(okToGive) {
			if(thePlayer->CanGive(spawnArgs.GetString("give_item"), spawnArgs.GetString("give_value"))) {

				startTime = gameLocal.slow.time;

				//Lock the player from harvesting to prevent multiple harvests when only one is needed
				thePlayer->harvest_lock = true;

				idWeapon* weap = (idWeapon*)thePlayer->weapon.GetEntity();
				if(weap) {
					//weap->PostEventMS(&EV_Weapon_State, 0, "Charge", 8);
					weap->ProcessEvent(&EV_Weapon_State, "Charge", 8);
				}

				BeginBurn();
				BeginFX();

				//Stop any looping sound that was playing
				idStr sound = parent->spawnArgs.GetString("harvest_sound");
				if(sound.Length() > 0) {
					parent->StopSound(SND_CHANNEL_ANY, false);
				}

				//Make the parent object non-solid
				parent->GetPhysics()->SetContents( 0 );
				parent->GetPhysics()->GetClipModel()->Unlink();

				//Turn of the trigger so it doesn't process twice
				trigger->SetContents( 0 );
			}
		}
	}
}


/*
===============================================================================

idAFEntity_Harvest

===============================================================================
*/

const idEventDef EV_Harvest_SpawnHarvestEntity( "<spawnHarvestEntity>", NULL );

CLASS_DECLARATION( idAFEntity_WithAttachedHead, idAFEntity_Harvest )
EVENT( EV_Harvest_SpawnHarvestEntity,	idAFEntity_Harvest::Event_SpawnHarvestEntity )
END_CLASS

/*
================
idAFEntity_Harvest::idAFEntity_Harvest
================
*/
idAFEntity_Harvest::idAFEntity_Harvest() {
	harvestEnt = NULL;
}

/*
================
idAFEntity_Harvest::~idAFEntity_Harvest
================
*/
idAFEntity_Harvest::~idAFEntity_Harvest() {

	if ( harvestEnt.GetEntity() ) {
		harvestEnt.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}

}

/*
================
idAFEntity_Harvest::Save
================
*/
void idAFEntity_Harvest::Save( idSaveGame *savefile ) const {
	harvestEnt.Save(savefile);
}

/*
================
idAFEntity_Harvest::Restore
================
*/
void idAFEntity_Harvest::Restore( idRestoreGame *savefile ) {
	harvestEnt.Restore(savefile);
	//if(harvestEnt.GetEntity()) {
	//	harvestEnt.GetEntity()->SetParent(this);
	//}
}

/*
================
idAFEntity_Harvest::Spawn
================
*/
void idAFEntity_Harvest::Spawn( void ) {

	PostEventMS( &EV_Harvest_SpawnHarvestEntity, 0 );
}

/*
================
idAFEntity_Harvest::Think
================
*/
void idAFEntity_Harvest::Think( void ) {

	idAFEntity_WithAttachedHead::Think();

}

void idAFEntity_Harvest::Event_SpawnHarvestEntity( void ) {

	const idDict *harvestDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_harvest_type"), false );
	if ( harvestDef ) {
		idEntity *temp;
		gameLocal.SpawnEntityDef( *harvestDef, &temp, false );
		harvestEnt = static_cast<idHarvestable *>(temp);
	}

	if(harvestEnt.GetEntity()) {
		//Let the harvest entity set itself up
		harvestEnt.GetEntity()->Init(this);
		harvestEnt.GetEntity()->BecomeActive( TH_THINK );
	}
}

void idAFEntity_Harvest::Gib( const idVec3 &dir, const char *damageDefName ) {
	if(harvestEnt.GetEntity()) {
		//Let the harvest ent know that we gibbed
		harvestEnt.GetEntity()->Gib();
	}
	idAFEntity_WithAttachedHead::Gib(dir, damageDefName);
}

#endif