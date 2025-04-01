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



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "StimResponse/StimResponseCollection.h"

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
void idMultiModelAF::Present( void ) 
{
	int i;

	if( m_bFrobable )
	{
		UpdateFrobState();
		UpdateFrobDisplay();
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) 
	{
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
}

/*
======================
idMultiModelAF::ParseAttachments
======================
*/
void idMultiModelAF::ParseAttachments( void )
{
	return;
}

/*
======================
idMultiModelAF::ParseAttachmentsAF
======================
*/
void idMultiModelAF::ParseAttachmentsAF( void )
{
	idEntity::ParseAttachments();
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

	ParseAttachmentsAF();
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
void idAFAttachment::SetBody( idEntity *bodyEnt, const char *model, jointHandle_t attachJoint ) 
{
	bool bleed;

	body = bodyEnt;
	this->attachJoint = attachJoint;
	SetModel( model );
	fl.takedamage = true;

	bleed = body->spawnArgs.GetBool( "bleed" );
	spawnArgs.SetBool( "bleed", bleed );

	// greebo: Add the body as frob peer
	m_FrobPeers.AddUnique(bodyEnt->name);

	// ishtvan: Go through our bind children and copy the actor body info over to them
	// might end up doing a few extra calls if GetTeamChildren is broken like we think,
	// but that's okay, add extra check of direct bindmaster to prevent infinite recursion
	idList<idEntity *> children;
	GetTeamChildren( &children );
	for( int i=0; i < children.Num(); i++ )
	{
		if( !children[i]->IsType(idAFAttachment::Type) )
			continue;
		if( !children[i]->IsBoundTo( this ) )
			continue;
		else
		{
			CopyBodyTo( static_cast<idAFAttachment *>(children[i]) );
		}
	}
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
=====================
idAFAttachment::GetAttachJoint
=====================
*/
jointHandle_t idAFAttachment::GetAttachJoint( void ) const
{
	return attachJoint;
}

/**
* Return true if we can mantle this attachment, false otherwise.
**/
bool idAFAttachment::IsMantleable() const
{
	return (!body || body->IsMantleable()) && idEntity::IsMantleable();
}

/**
* idAFAttachment::BindNotify
**/
void idAFAttachment::BindNotify( idEntity *ent , const char *jointName) // grayman #3074
{
	// copy information over to a bound idAfAttachment
	if( ent->IsType(idAFAttachment::Type) )
	{
		CopyBodyTo( static_cast<idAFAttachment *>(ent) );
	}
}

void idAFAttachment::UnbindNotify( idEntity *ent )
{
	idEntity::UnbindNotify( ent );
	// remove ent from AF if it was dynamically added as an AF body
	if( body && body->IsType(idAFEntity_Base::Type) )
		static_cast<idAFEntity_Base *>(body)->RemoveAddedEnt( ent );
}

void idAFAttachment::PostUnbind( void )
{
	// no longer bound to an actor
	body = NULL;
	attachJoint = INVALID_JOINT;
}

void idAFAttachment::DropOnRagdoll( void )
{
	// some stuff copied from idAI::DropOnRagdoll
	idEntity *ent = NULL;
	int mask(0);

	// Drop TDM attachments
	for ( int i = 0 ; i < m_Attachments.Num() ; i++ )
	{
		ent = m_Attachments[i].ent.GetEntity();
		if ( !ent || !m_Attachments[i].ent.IsValid() )
			continue;
		
		// greebo: Check if we should set some attachments to nonsolid
		// this applies for instance to the builder guard's pauldrons which
		// cause twitching and self-collisions when going down
		if (ent->spawnArgs.GetBool( "drop_set_nonsolid" ))
		{
			int curContents = ent->GetPhysics()->GetContents();

			// ishtvan: Also clear the CONTENTS_CORPSE flag (maybe this was a typo in original code?)
			ent->GetPhysics()->SetContents(curContents & ~(CONTENTS_SOLID|CONTENTS_CORPSE));

			// also have to iterate thru stuff attached to this attachment
			// ishtvan: left this commentd out because I'm not sure if GetTeamChildren is bugged or not
			// don't want to accidentally set all attachments to the AI to nonsolid
			/*
			idList<idEntity *> AttChildren;
			ent->GetTeamChildren( &AttChildren );
			gameLocal.Printf("TEST: drop_set_nonsolid, Num team children = %d", AttChildren.Num() );
			for( int i=0; i < AttChildren.Num(); i++ )
			{
				idPhysics *pChildPhys = AttChildren[i]->GetPhysics();
				if( pChildPhys == NULL )
					continue;

				int childContents = pChildPhys->GetContents();
				pChildPhys->SetContents( childContents & ~(CONTENTS_SOLID|CONTENTS_CORPSE) );
			}
			*/
		}

		bool bDrop = ent->spawnArgs.GetBool( "drop_when_ragdoll" );
		
		if ( !bDrop )
		{
			continue;
		}

		bool bSetSolid = ent->spawnArgs.GetBool( "drop_add_contents_solid" );
		bool bSetCorpse = ent->spawnArgs.GetBool( "drop_add_contents_corpse" );

		// Proceed with droppage
		DetachInd( i );

		if( bSetSolid )
			mask = CONTENTS_SOLID;
		if( bSetCorpse )
			mask = mask | CONTENTS_CORPSE;

		if( mask )
			ent->GetPhysics()->SetContents( ent->GetPhysics()->GetContents() | mask );

		CheckAfterDetach( ent ); // grayman #2624 - check for frobability and whether to extinguish

		// grayman #2624 - now handled by CheckAfterDetach() above
/*
		bool bSetFrob = ent->spawnArgs.GetBool( "drop_set_frobable" );
		if( bSetFrob )
			ent->m_bFrobable = true;

		// greebo: Check if we should extinguish the attachment, like torches
		bool bExtinguish = ent->spawnArgs.GetBool("extinguish_on_drop", "0");
		if ( bExtinguish )
		{
			// Get the delay in milliseconds
			int delay = SEC2MS(ent->spawnArgs.GetInt("extinguish_on_drop_delay", "3"));
			if (delay < 0) {
				delay = 0;
			}

			// Schedule the extinguish event
			ent->PostEventMS(&EV_ExtinguishLights, delay);
		}
 */
		ent->GetPhysics()->Activate();
		ent->m_droppedByAI = true; // grayman #1330

		// grayman #3075 - set m_SetInMotionByActor here
		ent->m_SetInMotionByActor = NULL;
		ent->m_MovedByActor = NULL;
	}
}

void idAFAttachment::CopyBodyTo( idAFAttachment *other )
{
	if( body )
	{
		idStr modelName = other->spawnArgs.GetString("model","");
		other->SetBody( body, modelName.c_str(), attachJoint );
	}
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
void idAFAttachment::Hide( void ) 
{
	idEntity::Hide();

	// ishtvan: Should hide any bind children of the head (copied from idActor)
	idEntity *ent;
	idEntity *next;
	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Hide();
			if ( ent->IsType( idLight::Type ) ) {
				static_cast<idLight *>( ent )->Off();
			}
		}
	}

	UnlinkCombat();
}

/*
================
idAFAttachment::Show
================
*/
void idAFAttachment::Show( void ) 
{
	idEntity::Show();

	// ishtvan: Should show any bind children of the head (copied from idActor)
	idEntity *ent;
	idEntity *next;

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Show();
			if ( ent->IsType( idLight::Type ) ) {
				static_cast<idLight *>( ent )->On();
			}
		}
	}
	LinkCombat();
}

/*
============
idAFAttachment::Damage

Pass damage to body at the bindjoint
============
*/
void idAFAttachment::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
	const char *damageDefName, const float damageScale, const int location, trace_t *tr ) 
{
	trace_t traceCopy;
	trace_t *pTrace = NULL;

	if( tr )
	{
		traceCopy = *tr;
		pTrace = &traceCopy;

		//TDM Fix: Propagate the trace.
		// Also, some things like KO check the endpoint of the trace rather than the "location" for the joint hit
		// So change this in the trace to the attach joint on the body we're attached to.
		traceCopy.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint );
	}

	if ( body ) 
	{
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("AF Attachment %s passing along damage to actor %s at attachjoint %d \r", name.c_str(), body->name.c_str(), (int) attachJoint );
		body->Damage( inflictor, attacker, dir, damageDefName, damageScale, attachJoint, pTrace );
	}
}

/*
================
idAFAttachment::AddDamageEffect
================
*/
void idAFAttachment::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) {
	if ( body ) {
		trace_t c = collision;
		c.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint );
		body->AddDamageEffect( c, velocity, damageDefName );
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
void idAFAttachment::AddForce( idEntity *ent, int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId ) {
	if ( body ) {
		body->AddForce( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, force, applId );
	} else {
		idEntity::AddForce( ent, bodyId, point, force, applId );
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

	if (m_StimResponseColl->HasResponse())
	{
		combatModel->SetContents( combatModel->GetContents() | CONTENTS_RESPONSE );
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

idEntity* idAFAttachment::GetResponseEntity()
{
	return body;
}

/*
===============================================================================

  idAFEntity_Base

===============================================================================
*/

const idEventDef EV_SetConstraintPosition( "SetConstraintPosition", EventArgs('s', "constraintName", "", 'v', "position", ""), EV_RETURNS_VOID, 
		"Moves the constraint with the given name that binds this entity to another entity." );
	
const idEventDef EV_GetLinearVelocityB( "getLinearVelocityB", EventArgs('d', "id", ""), 'v', 
		"Get the linear velocitiy of a particular body\n" \
		"Returns (0,0,0) if the body ID is invalid.");

const idEventDef EV_GetAngularVelocityB( "getAngularVelocityB", EventArgs('d', "id", ""), 'v', 
		"Get the angular velocitiy of a particular body\n" \
		"Returns (0,0,0) if the body ID is invalid.");

const idEventDef EV_SetLinearVelocityB( "setLinearVelocityB", EventArgs('v', "velocity", "", 'd', "id", "" ), EV_RETURNS_VOID, 
		"Set the linear velocity of a particular body");

const idEventDef EV_SetAngularVelocityB( "setAngularVelocityB", EventArgs('v', "velocity", "", 'd', "id", "" ), EV_RETURNS_VOID, 
		"Set the angular velocity of a particular body");

const idEventDef EV_GetNumBodies( "getNumBodies", EventArgs(), 'd', 
		"Returns the number of bodies in the AF. If the AF physics pointer is NULL, it returns 0.");

const idEventDef EV_RestoreAddedEnts( "restoreAddedEnts", EventArgs(), EV_RETURNS_VOID, "");

CLASS_DECLARATION( idAnimatedEntity, idAFEntity_Base )
	EVENT( EV_SetConstraintPosition,	idAFEntity_Base::Event_SetConstraintPosition )
	EVENT( EV_GetLinearVelocityB,		idAFEntity_Base::Event_GetLinearVelocityB )
	EVENT( EV_GetAngularVelocityB,		idAFEntity_Base::Event_GetAngularVelocityB )
	EVENT( EV_SetLinearVelocityB,		idAFEntity_Base::Event_SetLinearVelocityB )
	EVENT( EV_SetAngularVelocityB,		idAFEntity_Base::Event_SetAngularVelocityB )
	EVENT( EV_GetNumBodies,				idAFEntity_Base::Event_GetNumBodies )
	EVENT( EV_RestoreAddedEnts,			idAFEntity_Base::RestoreAddedEnts )

END_CLASS

static const float BOUNCE_SOUND_MIN_VELOCITY	= 80.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;

/*
================
idAFEntity_Base::idAFEntity_Base
================
*/
idAFEntity_Base::idAFEntity_Base( void ) 
{
	combatModel = NULL;
	combatModelContents = 0;
	nextSoundTime = NO_PROP_SOUND; // grayman #4609
	spawnOrigin.Zero();
	spawnAxis.Identity();
	m_bGroundWhenDragged = false;
	m_GroundBodyMinNum = 0;
	m_bDragAFDamping = false;
	m_bCollideWithTeam = true;
	m_bAFPushMoveables = false;
	m_AddedEnts.Clear();
}

/*
================
idAFEntity_Base::~idAFEntity_Base
================
*/
idAFEntity_Base::~idAFEntity_Base( void ) 
{
	delete combatModel;
	combatModel = NULL;
	m_GroundBodyList.Clear();
	m_AddedEnts.Clear();
}

/*
================
idAFEntity_Base::Save
================
*/
void idAFEntity_Base::Save( idSaveGame *savefile ) const 
{
	savefile->WriteInt( combatModelContents );
	savefile->WriteClipModel( combatModel );
	savefile->WriteVec3( spawnOrigin );
	savefile->WriteMat3( spawnAxis );
	savefile->WriteInt( nextSoundTime );

	savefile->WriteBool( m_bGroundWhenDragged );
	savefile->WriteInt( m_GroundBodyList.Num() );
	for( int i = 0; i < m_GroundBodyList.Num(); i++ )
		savefile->WriteInt( m_GroundBodyList[i] );
	savefile->WriteInt( m_GroundBodyMinNum );
	savefile->WriteBool( m_bDragAFDamping );
	savefile->WriteBool( m_bCollideWithTeam );
	savefile->WriteBool( m_bAFPushMoveables );

	savefile->WriteInt( m_AddedEnts.Num() );
	for( int j = 0; j < m_AddedEnts.Num(); j++ )
	{
		m_AddedEnts[j].ent.Save( savefile );
		savefile->WriteString( m_AddedEnts[j].bodyName );
		savefile->WriteString( m_AddedEnts[j].AddedToBody );
		savefile->WriteInt( m_AddedEnts[j].entContents );
		savefile->WriteInt( m_AddedEnts[j].entClipMask );
		// Check the body contents and clipmask just before saving, instead of putting in
		// the variables bodyContents and bodyClipMask. These are only used on restoring. 
		// go to hell, const correctness!
		idAFEntity_Base *thisNoConst = const_cast<idAFEntity_Base *>(this);
		idAFBody *body = thisNoConst->GetAFPhysics()->GetBody( m_AddedEnts[j].bodyName.c_str() );
		savefile->WriteInt( body->GetClipModel()->GetContents() );
		savefile->WriteInt( body->GetClipMask() );
	}

	af.Save( savefile );
}

/*
================
idAFEntity_Base::Restore
================
*/
void idAFEntity_Base::Restore( idRestoreGame *savefile ) 
{
	savefile->ReadInt( combatModelContents );
	savefile->ReadClipModel( combatModel );
	savefile->ReadVec3( spawnOrigin );
	savefile->ReadMat3( spawnAxis );
	savefile->ReadInt( nextSoundTime );
	LinkCombat();

	savefile->ReadBool( m_bGroundWhenDragged );

	int GroundBodyListNum;
	savefile->ReadInt( GroundBodyListNum );
	m_GroundBodyList.SetNum( GroundBodyListNum );
	for( int i = 0; i < GroundBodyListNum; i++ )
		savefile->ReadInt( m_GroundBodyList[i] );

	savefile->ReadInt( m_GroundBodyMinNum );
	savefile->ReadBool( m_bDragAFDamping );
	savefile->ReadBool( m_bCollideWithTeam );
	savefile->ReadBool( m_bAFPushMoveables );

	int AddedEntsNum;
	savefile->ReadInt( AddedEntsNum );
	m_AddedEnts.SetNum( AddedEntsNum );
	for( int j = 0; j < AddedEntsNum; j++ )
	{
		m_AddedEnts[j].ent.Restore( savefile );
		savefile->ReadString( m_AddedEnts[j].bodyName );
		savefile->ReadString( m_AddedEnts[j].AddedToBody );
		savefile->ReadInt( m_AddedEnts[j].entContents );
		savefile->ReadInt( m_AddedEnts[j].entClipMask );
		savefile->ReadInt( m_AddedEnts[j].bodyContents );
		savefile->ReadInt( m_AddedEnts[j].bodyClipMask );
	}

	af.Restore( savefile );
	if( m_bAFPushMoveables )
	{
		af.SetupPose( this, gameLocal.time );
		af.GetPhysics()->EnableClip();
	}

	// Schedule any added entities for re-adding when they have spawned
	PostEventMS( &EV_RestoreAddedEnts, 0 );
}

/*
================
idAFEntity_Base::Spawn
================
*/
void idAFEntity_Base::Spawn( void ) 
{
	spawnOrigin = GetPhysics()->GetOrigin();
	spawnAxis = GetPhysics()->GetAxis();
	nextSoundTime = NO_PROP_SOUND; // grayman #4609
	m_bGroundWhenDragged = spawnArgs.GetBool( "ground_when_dragged", "0" );
	m_GroundBodyMinNum = spawnArgs.GetInt( "ground_min_number", "0" );
	m_bDragAFDamping = spawnArgs.GetBool( "drag_af_damping", "0" );
	m_bCollideWithTeam = spawnArgs.GetBool( "af_collide_with_team", "1" ); // true by default
	m_bAFPushMoveables = spawnArgs.GetBool( "af_push_moveables", "0" );
}

/*
================
idAFEntity_Base::LoadAF
================
*/
bool idAFEntity_Base::LoadAF( void ) 
{
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

	SetUpGroundingVars();

	if( m_bAFPushMoveables )
	{
		af.SetupPose( this, gameLocal.time );
		af.GetPhysics()->EnableClip();
	}

	return true;
}

/*
================
idAFEntity_Base::Think
================
*/
void idAFEntity_Base::Think( void ) 
{
	if( !IsHidden() )
	{
		RunPhysics();
		UpdateAnimation();
	}
	if ( thinkFlags & TH_UPDATEVISUALS ) 
	{
		Present();
		LinkCombat();
		if ( needsDecalRestore ) // #3817
		{
			ReapplyDecals();
			needsDecalRestore = false;
		}
	}
// TDM: Anim/AF physics mods, generalized behavior that originally was just on AI

	// Update the AF bodies for the anim if we are set to do that
	if ( m_bAFPushMoveables && af.IsLoaded()
		&& !IsType(idAI::Type) && !IsHidden()
		&& animator.FrameHasChanged( gameLocal.time ) ) 
	{
		af.ChangePose( this, gameLocal.time );

		// copied from idAI::PushWithAF
		idClip_afTouchList touchList;
		idClip_EntityList pushed_ents;
		idEntity *ent;
		idVec3 vel( vec3_origin );
		int i, j;

		int num = af.EntitiesTouchingAF( touchList );
		for( i = 0; i < num; i++ ) 
		{
			// skip projectiles
			if ( touchList[ i ].touchedEnt->IsType( idProjectile::Type ) )
				continue;

			// make sure we havent pushed this entity already.  this avoids causing double damage
			for( j = 0; j < pushed_ents.Num(); j++ ) 
			{
				if ( pushed_ents[ j ] == touchList[ i ].touchedEnt )
					break;
			}
			if ( j >= pushed_ents.Num() ) 
			{
				ent = touchList[ i ].touchedEnt;
				pushed_ents.AddGrow(ent);
				vel = ent->GetPhysics()->GetAbsBounds().GetCenter() - touchList[ i ].touchedByBody->GetWorldOrigin();
				vel.Normalize();
				ent->ApplyImpulse( this, touchList[i].touchedClipModel->GetId(), ent->GetPhysics()->GetOrigin(), cv_ai_bumpobject_impulse.GetFloat() * vel );
			}
		}
	}
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
void idAFEntity_Base::AddForce( idEntity *ent, int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId ) {
	if ( af.IsLoaded() ) {
		af.AddForce( ent, bodyId, point, force, applId );
	}
	if ( !af.IsActive() ) {
		idEntity::AddForce( ent, bodyId, point, force, applId );
	}
}

/*
================
idAFEntity_Base::Collide
================
*/
bool idAFEntity_Base::Collide( const trace_t &collision, const idVec3 &velocity ) {
	float v, f;

	idEntity *e = gameLocal.entities[collision.c.entityNum];

	if(e)
	{
		ProcCollisionStims( e, collision.c.id );
		if( e->IsType( idAI::Type ) )
		{
			idAI *alertee = static_cast<idAI *>(e);
			alertee->TactileAlert( this );
		}
	}

	if ( af.IsActive() ) 
	{
		v = -( velocity * collision.c.normal );
		if ( ( nextSoundTime != NO_PROP_SOUND ) && // grayman #4609
			 ( gameLocal.time >= nextSoundTime ) &&
			 ( v > BOUNCE_SOUND_MIN_VELOCITY ) &&
			 !spawnArgs.GetBool("no_bounce_sound", "0") ) // grayman #3331 - some objects shouldn't propagate a bounce sound
		{
			f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : idMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / idMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
			// tels: #2953: support snd_bounce_material (like snd_bounce_carpet) here, too
			idStr sndNameLocal;
			const idMaterial *material = collision.c.material;
			idStr surfaceName;
			if (material != NULL)
			{
				// Prepend the snd_bounce_ prefix to check for a surface-specific sound
				surfaceName = g_Global.GetSurfName(material);
				idStr sndNameWithSurface = idStr("snd_bounce_") + surfaceName;

				if (spawnArgs.FindKey(sndNameWithSurface) != NULL)
				{
					sndNameLocal = sndNameWithSurface;
				}
				else
				{
					sndNameLocal = "snd_bounce";
				}
			}

			const char* sound = spawnArgs.GetString(sndNameLocal);
			const idSoundShader* sndShader = declManager->FindSound(sound);

			// don't set the volume unless there is a bounce sound as it overrides the entire channel
			// which causes footsteps on ai's to not honor their shader parms
			if (sndShader)
			{
				float volume = sndShader->GetParms()->volume + f;

				SetSoundVolume(volume);

				// greebo: We don't use StartSound() here, we want to do the sound propagation call manually
				StartSoundShader(sndShader, SND_CHANNEL_ANY, 0, false, NULL);

				// Propagate a suspicious sound, using the "group" convention (soft, hard, small, med, etc.)
				idStr sndPropName = GetSoundPropNameForMaterial(surfaceName);
				PropSoundS( NULL, sndPropName, f , 0); // grayman #3355
				nextSoundTime = gameLocal.time + 500;
			}
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
bool idAFEntity_Base::SetCombatContents( bool enable ) {
	assert( combatModel );
	bool oldIsEnabled = (combatModel->GetContents() != 0);
	if ( enable && combatModelContents ) {
		assert( !combatModel->GetContents() );
		combatModel->SetContents( combatModelContents );
		combatModelContents = 0;
	} else if ( !enable && combatModel->GetContents() ) {
		assert( !combatModelContents );
		combatModelContents = combatModel->GetContents();
		combatModel->SetContents( 0 );
	}
	return oldIsEnabled;
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
================
idAFEntity_Base::SetModel
================
*/
void idAFEntity_Base::SetModel( const char *modelname ) {
	bool hadModelDef = ( GetAnimator()->ModelDef() != NULL );

	idAnimatedEntity::SetModel(modelname);

	if ( hadModelDef && !GetAnimator()->ModelDef() ) {
		//stgatilov #5845: can only switch AF model with compatible AF model
		//because we don't (and cannot) change the body set in af.physicsObj
		common->Error("idAFEntity_Base::SetModel: model %s is not modelDef", modelname);
	}
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

void idAFEntity_Base::Event_SetLinearVelocityB( idVec3 &NewVelocity, int id )
{
	GetPhysics()->SetLinearVelocity( NewVelocity, id );
}

void idAFEntity_Base::Event_SetAngularVelocityB( idVec3 &NewVelocity, int id )
{
	GetPhysics()->SetAngularVelocity( NewVelocity, id );
}

void idAFEntity_Base::Event_GetLinearVelocityB( int id )
{
	idThread::ReturnVector( GetPhysics()->GetLinearVelocity( id ) );
}

void idAFEntity_Base::Event_GetAngularVelocityB( int id )
{
	idThread::ReturnVector( GetPhysics()->GetAngularVelocity( id ) );
}

void idAFEntity_Base::Event_GetNumBodies( void )
{
	idThread::ReturnInt( static_cast<idPhysics_AF *>( GetPhysics() )->GetNumBodies() );
}

void idAFEntity_Base::SetUpGroundingVars( void )
{
	if( m_bGroundWhenDragged && af.IsLoaded() )
	{
		idLexer	src;
		idToken	token;
		idStr tempStr = spawnArgs.GetString( "ground_critical_bodies", "" );
		
		m_GroundBodyList.Clear();
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Parsing critical ground bodies list %s\r", tempStr.c_str());
		if( tempStr.Length() )
		{
			src.LoadMemory( tempStr.c_str(), tempStr.Length(), "" );
			src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES );
			
			while( src.ReadToken( &token ) )
				m_GroundBodyList.Append( GetAFPhysics()->GetBodyId(token.c_str()) );
			
			src.FreeSource();
		}
	}
}

bool idAFEntity_Base::CollidesWithTeam( void )
{
	return m_bCollideWithTeam;
}

void idAFEntity_Base::AddEntByJoint( idEntity *ent, jointHandle_t joint )
{
	int bodID(0);
	
	if( af.IsLoaded() )
	{
		bodID = BodyForClipModelId( JOINT_HANDLE_TO_CLIPMODEL_ID( joint ) );
		AddEntByBody( ent, bodID, joint );
	}
}

void idAFEntity_Base::AddEntByBody( idEntity *ent, int bodID, jointHandle_t joint )
{
	float EntMass(0.0), AFMass(0.0), MassOut(0.0), density(0.0);
	idVec3 COM(vec3_zero), orig(vec3_zero);
	idMat3 inertiaTensor, axis;
	idClipModel *EntClip(NULL), *NewClip(NULL);
	int newBodID(0);
	SAddedEnt Entry;
	idStr AddName;

	if( !af.IsLoaded() ) return;

	//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("AddEntByBody: Called, ent %s, body %d\r", ent->name.c_str(), bodID );


	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AddEntByBody: Entity origin: %s \r", orig.ToString() );
	
	EntClip = ent->GetPhysics()->GetClipModel();
	axis = EntClip->GetAxis();
	orig = EntClip->GetOrigin();

	NewClip = new idClipModel(EntClip);

	// Propagate CONTENTS_CORPSE from AF to new clipmodel
	if( GetAFPhysics()->GetContents() & CONTENTS_CORPSE )
		NewClip->SetContents( (NewClip->GetContents() & (~CONTENTS_SOLID)) | CONTENTS_CORPSE | CONTENTS_RENDERMODEL );
	
	// EntMass = ent->GetPhysics()->GetMass();
	// FIX: Large masses aren't working, the AFs are not quite that flexible that you can put on a huge mass
	// Or it could be the occasional small negative elements in the inertia tensor.
	// In any case for now, we set the masses to 1 so they only collide, don't pull on the AF
	EntMass = 1.0f;
	if ( EntMass <= 0.0f || FLOAT_IS_NAN( EntMass ) ) 
	{
		EntMass = 1.0f;
	}
	AFMass = GetAFPhysics()->GetMass();
	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AddEntByBody: Retrieved masses. AF mass: %f , Ent mass: %f \r", AFMass, EntMass );

	// Trick: Use a test density of 1.0 here, then divide the actual mass by output mass to get actual density
	NewClip->GetMassProperties( 1.0f, MassOut, COM, inertiaTensor );

	// AF bodies want to have their origin at the center of mass
	NewClip->TranslateOrigin( -COM );
	orig += COM * axis;

	// DEBUG:
//	idVec3 COMNew;
//	NewClip->GetMassProperties( 1.0f, MassOut, COMNew, inertiaTensor );
//	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING("AF Bind: New Clip COM: %s \r", COMNew.ToString() );
//	DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING("AF Bind: Modified origin: %s \r", orig.ToString() );

	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AddEntByBody: Linking clipmodel copy... \r" );
	// FIXME: Do we really want to set id 0 here?  Won't this conflict?
	NewClip->Link( gameLocal.clip, this, 0, orig, axis );
	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AddEntByBody: Clipmodel linked.\r");

	// Add the mass in the AF Structure
	density = idMath::Fabs( EntMass / MassOut );
	GetAFPhysics()->SetMass( AFMass + EntMass );
	
	AddName = ent->name + idStr(gameLocal.time);

	idAFBody *bodyExist = GetAFPhysics()->GetBody(bodID);
	idAFBody *body = new idAFBody( AddName, NewClip, density );
	body->SetClipMask( bodyExist->GetClipMask() );
	body->SetSelfCollision( false );
	body->SetRerouteEnt( ent );
	body->SetWorldOrigin( orig );
	body->SetWorldAxis( axis );
	newBodID = GetAFPhysics()->AddBody( body );
	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AddEntByBody: Body added to physics_AF with id %d.\r", newBodID);
	
	// ishtvan: Don't add the constraint while animating, wait until full ragdoll mode
	if( af.IsActive() )
	{
		idAFConstraint_Fixed *cf = new idAFConstraint_Fixed( AddName, body, bodyExist );
		GetAFPhysics()->AddConstraint( cf );
	}

	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AddEntByBody: Constraint added between new body %s and original body %s.\r", body->GetName().c_str(), bodyExist->GetName().c_str());

	// Now add body to AF object, for updating with idAF::ChangePos and the like
	// We use AF_JOINTMOD_NONE since this new AF shouldn't actually stretch joints on the model when it moves
	af.AddBodyExtern( this, body, bodyExist, AF_JOINTMOD_NONE, joint );

	// Add to list
	Entry.ent = ent;
	Entry.AddedToBody = bodyExist->GetName();
	Entry.bodyName = AddName;
	Entry.bodyContents = body->GetClipModel()->GetContents();
	Entry.bodyClipMask = body->GetClipMask();
	Entry.entContents = EntClip->GetContents();
	Entry.entClipMask = ent->GetPhysics()->GetClipMask();

	m_AddedEnts.Append( Entry );

	// Disable the entity's clipmodel
	// leave CONTENTS_RESPONSE and CONTENTS_FROBABLE alone
	int SetContents = 0;
	if( (EntClip->GetContents() & CONTENTS_RESPONSE) != 0 )
		SetContents = CONTENTS_RESPONSE;
	
	if( (EntClip->GetContents() & CONTENTS_FROBABLE) != 0
		// Temporary fix: CONTENTS_FROBABLE is not currently set on all frobables
		|| ent->m_bFrobable )
		SetContents = SetContents | CONTENTS_FROBABLE;

	EntClip->SetContents( SetContents );
	ent->GetPhysics()->SetClipMask( 0 );

	// Make sure the AF activates as soon as this is done
	if( af.IsActive() )
		GetAFPhysics()->Activate();
		
	//DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("AddEntByBody: Done.\r");
}

/*
================
idAFEntity_Base::RemoveAddedEnt
================
*/
void idAFEntity_Base::RemoveAddedEnt( idEntity *ent )
{
	bool bRemoved = false;
	idStr bodyName;

	for( int i=m_AddedEnts.Num() - 1; i >= 0; i-- )
	{
		if(ent && (m_AddedEnts[i].ent.GetEntity() == ent))
		{
			bodyName = m_AddedEnts[i].bodyName;
			GetAFPhysics()->DeleteBody( bodyName.c_str() );
			af.DeleteBodyExtern( this, bodyName.c_str() );
			
			ent->GetPhysics()->SetContents( m_AddedEnts[i].entContents );
			ent->GetPhysics()->SetClipMask( m_AddedEnts[i].entClipMask );
			m_AddedEnts.RemoveIndex(i);
			// Added ent AF bodies have a mass of 1 for now
			// GetAFPhysics()->SetMass( GetPhysics()->GetMass() - ent->GetPhysics()->GetMass() );
			GetAFPhysics()->SetMass( GetPhysics()->GetMass() - 1.0f );

			bRemoved = true;
		}
	}

	if( bRemoved && IsActiveAF() )
		ActivatePhysics( this );
}

jointHandle_t idAFEntity_Base::JointForBody( int body )
{
	return af.JointForBody( body );
}
	
int	idAFEntity_Base::BodyForJoint( jointHandle_t joint )
{
	return af.BodyForJoint( joint );
}

idAFBody *idAFEntity_Base::AFBodyForEnt( idEntity *ent )
{
	idAFBody *returnBody = NULL;
	for( int i=0; i < m_AddedEnts.Num(); i++ )
	{
		if( m_AddedEnts[i].ent.GetEntity() == ent )
		{
			idStr bodyName = m_AddedEnts[i].bodyName;
			returnBody = GetAFPhysics()->GetBody( bodyName.c_str() );
			break;
		}
	}

	return returnBody;
}

void idAFEntity_Base::RestoreAddedEnts( void )
{
	// This must be called after all entities are loaded
	int TempContents, TempClipMask, bodyID;
	idEntity *ent;
	idList<SAddedEnt>	OldAdded;
	idStr temp;
	
	OldAdded = m_AddedEnts;
	m_AddedEnts.Clear();

	for(int i=0; i<OldAdded.Num(); i++)
	{
		// First, we reset all the original clipmodel contents because AddEntByBody will read it and store it
		TempContents = OldAdded[i].entContents;
		TempClipMask = OldAdded[i].entClipMask;
		ent = OldAdded[i].ent.GetEntity();
		ent->GetPhysics()->SetContents( TempContents );
		ent->GetPhysics()->SetClipMask( TempClipMask );

		bodyID = GetAFPhysics()->GetBodyId( OldAdded[i].AddedToBody );

		// Add the body again
		AddEntByBody( ent, bodyID );

		// restore the saved contents and clipmask of the AF body just added
		idAFBody *body = AFBodyForEnt( ent );
		if( body )
		{
			TempContents = OldAdded[i].bodyContents;
			TempClipMask = OldAdded[i].bodyClipMask;
			body->GetClipModel()->SetContents( TempContents );
			body->SetClipMask( TempClipMask );
		}
	}
}

void idAFEntity_Base::UpdateAddedEntConstraints( void )
{
	for( int i=0; i < m_AddedEnts.Num(); i++ )
	{
		idStr AddName = m_AddedEnts[i].bodyName;
		idAFBody *body = GetAFPhysics()->GetBody( AddName.c_str() );
		idAFBody *bodyExist = GetAFPhysics()->GetBody( m_AddedEnts[i].AddedToBody );

		// get the AF bodies in their most recent position (just before ragdoll start)
		af.ChangePose( this, gameLocal.time );
		// add a constraint for the current position
		idAFConstraint_Fixed *cf = new idAFConstraint_Fixed( AddName, body, bodyExist );
		GetAFPhysics()->AddConstraint( cf );
	}
}

void idAFEntity_Base::UnbindNotify( idEntity *ent )
{
	idEntity::UnbindNotify( ent );
	// remove ent from AF if it was dynamically added as an AF body
	RemoveAddedEnt( ent );
}

void idAFEntity_Base::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, trace_t *tr )
{
	idEntity *reroute = NULL;
	idAFBody *StruckBody = NULL;
	int bodID;
	
	if( tr )
	{
		bodID = BodyForClipModelId( tr->c.id );
		StruckBody = GetAFPhysics()->GetBody( bodID );
		
		if( StruckBody != NULL )
			reroute = StruckBody->GetRerouteEnt();
	}
	
	// check for reroute entity on the AF body and damage this instead
	if ( reroute != NULL )
	{
		reroute->Damage( inflictor, attacker, dir, damageDefName, damageScale, location, tr );
	}
	else
	{
		idEntity::Damage( inflictor, attacker, dir, damageDefName, damageScale, location, tr );
	}
}

/*
======================
idAFEntity_Base::ParseAttachments
======================
*/
void idAFEntity_Base::ParseAttachments( void )
{
	return;
}

/*
======================
idAFEntity_Base::ParseAttachmentsAF
======================
*/
void idAFEntity_Base::ParseAttachmentsAF( void )
{
	// FIX: Preserve initial frame facing when posing the AF prior to attaching
	// otherwise relationships between new AF bodies and AF bodies they're attached to
	// get screwed up (since attached entities are attached when the AFEntity is still facing 0,0,0)
	idMat3 tempAxis = renderEntity.axis;
	idMat3 tempAxis2 = GetPhysics()->GetAxis();
	renderEntity.axis = mat3_identity;
	SetAxis(mat3_identity);

	af.SetupPose(this, gameLocal.time);
	idEntity::ParseAttachments();

	renderEntity.axis = tempAxis;
	SetAxis(tempAxis2);
}

void idAFEntity_Base::ReAttachToPos
	( const char *AttName, const char *PosName  )
{
	int ind = GetAttachmentIndex( AttName );
	if (ind == -1 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("ReAttachToPos called with invalid attachment name %s on entity %s\r", AttName, name.c_str());
		return;
	}

	idEntity* ent = GetAttachment( ind );

	if( !ent )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("ReAttachToPos called with invalid attached entity on entity %s\r", AttName, name.c_str());
		return;
	}
 
	// retain the AF body contents (don't want to accidentally re-enable them if clip disabled)
	idAFBody *body = NULL;
	int bodyContents = 0, bodyClipMask = 0;
	bool bStoredAFBodyInfo = false;
	if( (body = static_cast<idAFEntity_Base *>(this)->AFBodyForEnt( ent )) != NULL )
	{
		bodyContents = body->GetClipModel()->GetContents();
		bodyClipMask = body->GetClipMask();
		bStoredAFBodyInfo = true;
	}

	idEntity::ReAttachToPos( AttName, PosName );

	// copy over the old AF body contents
	if( (body = static_cast<idAFEntity_Base *>(this)->AFBodyForEnt( ent )) != NULL
		&& bStoredAFBodyInfo )
	{
		body->GetClipModel()->SetContents( bodyContents );
		body->SetClipMask( bodyClipMask );
	}
}

void idAFEntity_Base::ReAttachToCoordsOfJoint
	( const char *AttName, idStr jointName, 
		idVec3 offset, idAngles angles  )
{
	idEntity *ent(NULL);
	CAttachInfo *attachment = GetAttachInfo( AttName );

	if( !attachment )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("ReAttachToPos called with invalid attachment name %s on entity %s\r", AttName, name.c_str());
		return;
	}
	
	ent = attachment->ent.GetEntity();
	if( !attachment->ent.IsValid() || !ent )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("ReAttachToPos called with invalid attached entity on entity %s\r", name.c_str());
		return;
	}

	// retain the AF body contents (don't want to accidentally re-enable them if clip disabled)
	idAFBody *body = NULL;
	int bodyContents = 0, bodyClipMask = 0;
	bool bStoredAFBodyInfo = false;
	if( (body = static_cast<idAFEntity_Base *>(this)->AFBodyForEnt( ent )) != NULL )
	{
		bodyContents = body->GetClipModel()->GetContents();
		bodyClipMask = body->GetClipMask();
		bStoredAFBodyInfo = true;
	}

	idAnimatedEntity::ReAttachToCoordsOfJoint( AttName, jointName, offset, angles );

	// copy over the old AF body contents
	if( (body = static_cast<idAFEntity_Base *>(this)->AFBodyForEnt( ent )) != NULL
		&& bStoredAFBodyInfo )
	{
		body->GetClipModel()->SetContents( bodyContents );
		body->SetClipMask( bodyClipMask );
	}
}

/*
===============================================================================

idAFEntity_Gibbable

===============================================================================
*/

const idEventDef EV_Gib( "gib", EventArgs('s', "damageDefName", ""), EV_RETURNS_VOID, "" );
const idEventDef EV_Gibbed( "<gibbed>", EventArgs(), EV_RETURNS_VOID, "internal");

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
================
idAFEntity_Gibbable::Damage
================
*/
void idAFEntity_Gibbable::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, trace_t *tr ) 
{
	idAFEntity_Base::Damage( inflictor, attacker, dir, damageDefName, damageScale, location, tr );	
	
	if ( !fl.takedamage ) 
	{
		return;
	}
	
	if ( health < -20 && spawnArgs.GetBool( "gib" ) ) {
		Gib( dir, damageDefName );
	}
}

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

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, true ); // grayman #3391 - don't create a default 'damageDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( !damageDef )
	{
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
			list[i]->GetPhysics()->SetContents( CONTENTS_CORPSE );
			list[i]->GetPhysics()->SetClipMask( CONTENTS_SOLID );
			velocity = list[i]->GetPhysics()->GetAbsBounds().GetCenter() - entityCenter;
			velocity.NormalizeFast();
			velocity += ( i & 1 ) ? dir : -dir;
			list[i]->GetPhysics()->SetLinearVelocity( velocity * 75.0f );
		}
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

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, true ); // grayman #3391 - don't create a default 'damageDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( !damageDef )
	{
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

	if ( keepRunningPhysics && !IsHidden() ) 
	{
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

	ParseAttachmentsAF();

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

	// greebo: Reactivate the animation flag, just in case
	// This hopefully helps rope arrows to not stick out straight in the air
	BecomeActive(TH_ANIMATE);
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
void idAFEntity_WithAttachedHead::Spawn( void ) 
{
	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	SetupHead();
	ParseAttachmentsAF();

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
void idAFEntity_WithAttachedHead::SetupHead()
{
	idStr headModelDefName = spawnArgs.GetString( "def_head", "" );

	idVec3 modelOffset = spawnArgs.GetVector("offsetModel", "0 0 0");
	idVec3 HeadModelOffset = spawnArgs.GetVector("offsetHeadModel", "0 0 0");

	if ( !headModelDefName.IsEmpty() ) 
	{
		// We look if the head model is defined as a key to have a specific offset.
		// If that is not the case, then we use the default value, if it exists, 
		// otherwise there is no offset at all.
		if (spawnArgs.FindKey(headModelDefName) != NULL)
		{
			HeadModelOffset = spawnArgs.GetVector(headModelDefName, "0 0 0");
		}

		idStr jointName = spawnArgs.GetString( "head_joint" );
		jointHandle_t joint = animator.GetJointHandle( jointName.c_str() );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		// Setup the default spawnargs for all heads
		idDict args;

		const idDeclEntityDef* def = gameLocal.FindEntityDef(headModelDefName, false);

		if (def == NULL)
		{
			gameLocal.Warning("Could not find head entityDef %s!", headModelDefName.c_str());

			// Try to fallback on the default head entityDef
			def = gameLocal.FindEntityDef(TDM_HEAD_ENTITYDEF, false);
		}

		if (def != NULL)
		{
			// Make a copy of the default spawnargs
			args = def->dict;
		}
		else
		{
			gameLocal.Warning("Could not find head entityDef %s or %s!", headModelDefName.c_str(), TDM_HEAD_ENTITYDEF);
		}

		// Spawn the head entity
		idEntity* ent = gameLocal.SpawnEntityType(idAFAttachment::Type, &args);
		idAFAttachment* headEnt = static_cast<idAFAttachment*>(ent);

		// Retrieve the actual model from the head entityDef
		idStr headModel = args.GetString("model");
		if (headModel.IsEmpty())
		{
			gameLocal.Warning("No 'model' spawnarg on head entityDef: %s", headModelDefName.c_str());
		}

		headEnt->SetName( va( "%s_head", name.c_str() ) );
		headEnt->SetBody( this, headModel, joint );
		headEnt->SetCombatModel();
		head = headEnt;

		idVec3				origin;
		idMat3				axis;
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + ( origin + modelOffset + HeadModelOffset ) * renderEntity.axis;
		headEnt->SetOrigin( origin );
		headEnt->SetAxis( renderEntity.axis );
		headEnt->BindToJoint( this, joint, true );

		// greebo: Setup the frob-peer relationship between head and body
		m_FrobPeers.AddUnique(headEnt->name);
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
void idAFEntity_WithAttachedHead::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material, bool save ) {

	idEntity::ProjectOverlay( origin, dir, size, material, save );

	if ( head.GetEntity() ) {
		head.GetEntity()->ProjectOverlay( origin, dir, size, material, save );
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

CLASS_DECLARATION( idAFEntity_Base, idAFEntity_Vehicle )
END_CLASS

/*
================
idAFEntity_Vehicle::idAFEntity_Vehicle
================
*/
idAFEntity_Vehicle::idAFEntity_Vehicle( void ) {
	player				= NULL;
	eyesJoint			= INVALID_JOINT;
	steeringWheelJoint	= INVALID_JOINT;
	wheelRadius			= 0.0f;
	steerAngle			= 0.0f;
	steerSpeed			= 0.0f;
	dustSmoke			= NULL;
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

	ParseAttachmentsAF();

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

	player = NULL;
	steerAngle = 0.0f;

	const char *smokeName = spawnArgs.GetString( "smoke_vehicle_dust", "muzzlesmoke" );
	if ( *smokeName != '\0' ) {
		dustSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	}
}

/*
================
idAFEntity_Vehicle::Use
================
*/
void idAFEntity_Vehicle::Use( idPlayer *other ) {
	idVec3 origin;
	idMat3 axis;

	if ( player ) {
		if ( player == other ) {
			other->Unbind();
			player = NULL;

			af.GetPhysics()->SetComeToRest( true );
		}
	}
	else {
		player = other;
		animator.GetJointTransform( eyesJoint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;
		player->GetPhysics()->SetOrigin( origin );
		player->BindToBody( this, 0, true );

		af.GetPhysics()->SetComeToRest( false );
		af.GetPhysics()->Activate();
	}
}

/*
================
idAFEntity_Vehicle::GetSteerAngle
================
*/
float idAFEntity_Vehicle::GetSteerAngle( void ) {
	float idealSteerAngle, angleDelta;

	idealSteerAngle = player->usercmd.rightmove * ( 30.0f / 128.0f );
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


/*
===============================================================================

  idAFEntity_VehicleSimple

===============================================================================
*/

CLASS_DECLARATION( idAFEntity_Vehicle, idAFEntity_VehicleSimple )
END_CLASS

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
}

/*
================
idAFEntity_VehicleSimple::~idAFEntity_VehicleSimple
================
*/
idAFEntity_VehicleSimple::~idAFEntity_VehicleSimple( void ) {
	delete wheelModel;
	wheelModel = NULL;
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
		suspension[i]->Setup( va( "suspension%d", i ), af.GetPhysics()->GetBody( 0 ), origin, af.GetPhysics()->GetAxis( 0 ), wheelModel );
		suspension[i]->SetSuspension(	g_vehicleSuspensionUp.GetFloat(),
										g_vehicleSuspensionDown.GetFloat(),
										g_vehicleSuspensionKCompress.GetFloat(),
										g_vehicleSuspensionDamping.GetFloat(),
										g_vehicleTireFriction.GetFloat() );

		af.GetPhysics()->AddConstraint( suspension[i] );
	}

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
	idRotation wheelRotation, steerRotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = g_vehicleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = idMath::Fabs( player->usercmd.forwardmove * g_vehicleForce.GetFloat() ) * (1.0f / 128.0f);
			steerAngle = GetSteerAngle();
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

		// update suspension with latest cvar settings
		for ( i = 0; i < 4; i++ ) {
			suspension[i]->SetSuspension(	g_vehicleSuspensionUp.GetFloat(),
											g_vehicleSuspensionDown.GetFloat(),
											g_vehicleSuspensionKCompress.GetFloat(),
											g_vehicleSuspensionDamping.GetFloat(),
											g_vehicleTireFriction.GetFloat() );
		}

		// run the physics
		RunPhysics();

		// move and rotate the wheels visually
		for ( i = 0; i < 4; i++ ) {
			idAFBody *body = af.GetPhysics()->GetBody( 0 );

			origin = suspension[i]->GetWheelOrigin();
			velocity = body->GetPointVelocity( origin ) * body->GetWorldAxis()[0];
			wheelAngles[i] += velocity * MS2SEC( USERCMD_MSEC ) / wheelRadius;

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
		}
/*
		// spawn dust particle effects
		if ( force != 0.0f && !( gameLocal.framenum & 7 ) ) {
			int numContacts;
			idAFConstraint_Contact *contacts[2];
			for ( i = 0; i < 4; i++ ) {
				numContacts = af.GetPhysics()->GetBodyContactConstraints( wheels[i]->GetClipModel()->GetId(), contacts, 2 );
				for ( int j = 0; j < numContacts; j++ ) {
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3() );
				}
			}
		}
*/
	}

	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}


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

		if ( player ) {
			// capture the input from a player
			velocity = g_vehicleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = idMath::Fabs( player->usercmd.forwardmove * g_vehicleForce.GetFloat() ) * (1.0f / 128.0f);
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
			wheelAngles[i] += velocity * MS2SEC( USERCMD_MSEC ) / wheelRadius;
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
				for ( int j = 0; j < numContacts; j++ ) {
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3() );
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
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	idVec3 origin;
	idMat3 axis;
	idRotation rotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = g_vehicleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = idMath::Fabs( player->usercmd.forwardmove * g_vehicleForce.GetFloat() ) * (1.0f / 128.0f);
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
			wheelAngles[i] += velocity * MS2SEC( USERCMD_MSEC ) / wheelRadius;
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
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3() );
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

	ParseAttachmentsAF();

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
	//stgatilov: make it frobable in TDM world
	args.Set( "frobable", "1" );
	args.Set( "grabable", "1" );
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
	int i(0), jointNum(0);
	const idDeclAF *af(NULL);
	const idDeclAF_Body *fb(NULL);
	renderEntity_t ent;
	idVec3 origin, *bodyOrigin(NULL), *newBodyOrigin(NULL), *modifiedOrigin(NULL);
	idMat3 axis, *bodyAxis(NULL), *newBodyAxis(NULL), *modifiedAxis(NULL);
	declAFJointMod_t *jointMod(NULL);
	idAngles angles;
	const idDict *defArgs(NULL);
	const idKeyValue *arg(NULL);
	idStr name;
	jointTransformData_t data;
	const char *classname(NULL), *afName(NULL), *modelName(NULL);
	idRenderModel *md5(NULL);
	const idDeclModelDef *modelDef(NULL);
	const idMD5Anim *MD5anim(NULL);
	const idMD5Joint *MD5joint(NULL);
	const idMD5Joint *MD5joints(NULL);
	int numMD5joints;
	idJointMat *originalJoints(NULL);
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
