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

// TODO: Make sure drag to point is not within a solid
// TODO: Detecting stuck items (distance + angular offset)
// TODO: Handling stuck items (initially stop the player's motion, then if they continue that motion, drop the item)

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"

#include "Grabber.h"

/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

const idEventDef EV_Grabber_CheckClipList( "<checkClipList>", EventArgs(), EV_RETURNS_VOID, "internal" );

// TODO: Make most of these cvars

const int CHECK_CLIP_LIST_INTERVAL =	200;

const int MOUSE_DEADZONE =				5;
// const float MOUSE_SCALE =				0.7f;
 const float MOUSE_SCALE =				0.2f;

// when you let go of an item, the velocity is clamped to this value
const float MAX_RELEASE_LINVEL =		30.0f;
// when you let go of an item, the angular velocity is clamped to this value
const float MAX_RELEASE_ANGVEL =		10.0f;
// limits how close you can hold an item to the player view point
const float MIN_HELD_DISTANCE  =		35.0f;
// granularity of the distance control
const int	DIST_GRANULARITY	=		12;

const char *SHOULDER_ANIM = "drop_body";

CLASS_DECLARATION( idEntity, CGrabber )
	EVENT( EV_Grabber_CheckClipList, 	CGrabber::Event_CheckClipList )
END_CLASS

/*
==============
CGrabber::CGrabber
==============
*/
CGrabber::CGrabber( void ) 
{
	Clear();
}

/*
==============
CGrabber::~CGrabber
==============
*/
CGrabber::~CGrabber( void ) 
{
	StopDrag();
	Clear();
}


/*
==============
CGrabber::Clear
==============
*/
void CGrabber::Clear( void ) 
{
	m_dragEnt			= NULL;
	m_player			= NULL;

	m_rotation			= idRotation(vec3_zero,vec3_zero,0.0f);
	m_rotationAxis		= 0;
	m_mousePosition		= idVec2(0.0f,0.0f);
	m_bAllowPlayerRotation = true;
	m_bAllowPlayerTranslation = true;

	m_joint			= INVALID_JOINT;
	m_id				= 0;
	m_LocalEntPoint.Zero();
	m_vLocalEntOffset.Zero();
	m_vOffset.Zero();
	m_bMaintainPitch = true;
	m_bAttackPressed = false;
	m_ThrowTimer = 0;
	m_bIsColliding = false;
	m_bPrevFrameCollided = false;
	m_CollNorms.Clear();
	
	m_bAFOffGround = false;
	m_DragUpTimer = 0;
	m_AFBodyLastZ = 0.0f;

	m_DistanceCount = 0;
	m_MinHeldDist	= 0;
	m_MaxDistCount	= DIST_GRANULARITY;
	m_LockedHeldDist = 0;
	m_bObjStuck = false;
	m_MaxForce = 0;
	m_bDropBodyFaceUp = false;

	m_EquippedEnt = NULL;
	m_bEquippedEntInWorld = false;
	m_vEquippedPosition.Zero();
	m_PreservedPosition.Zero();		// #4149
	m_StoppingPreserving = false;	// #4149
	m_EquippedEntContents = 0;
	m_EquippedEntClipMask = 0;

	while( this->HasClippedEntity() )
		this->RemoveFromClipList( 0 );

	m_clipList.Clear();
	m_silentMode = false;
}

void CGrabber::Save( idSaveGame *savefile ) const
{
	m_dragEnt.Save(savefile);
	savefile->WriteJoint(m_joint);
	savefile->WriteInt(m_id);
	savefile->WriteVec3(m_LocalEntPoint);
	savefile->WriteVec3(m_vLocalEntOffset);
	savefile->WriteVec3(m_vOffset);
	savefile->WriteBool(m_bMaintainPitch);

	m_player.Save(savefile);
	m_drag.Save(savefile);
	savefile->WriteBool(m_bAllowPlayerRotation);
	savefile->WriteBool(m_bAllowPlayerTranslation);

	// Save the three relevant values of the idRotation object
	savefile->WriteVec3(m_rotation.GetOrigin());
	savefile->WriteVec3(m_rotation.GetVec());
	savefile->WriteFloat(m_rotation.GetAngle());


	savefile->WriteInt(m_rotationAxis);
	savefile->WriteVec2(m_mousePosition);
	
	savefile->WriteInt(m_clipList.Num());
	for (int i = 0; i < m_clipList.Num(); i++)
	{
		m_clipList[i].m_ent.Save(savefile);
		savefile->WriteInt(m_clipList[i].m_clipMask);
		savefile->WriteInt(m_clipList[i].m_contents);
	}

	savefile->WriteBool(m_bAttackPressed);
	savefile->WriteInt(m_ThrowTimer);
	savefile->WriteInt(m_DragUpTimer);
	savefile->WriteFloat(m_AFBodyLastZ);
	savefile->WriteBool(m_bAFOffGround);
	savefile->WriteInt(m_DistanceCount);
	savefile->WriteInt(m_MaxDistCount);
	savefile->WriteInt(m_MinHeldDist);
	savefile->WriteVec3(m_PreservedPosition);  // #4149
	savefile->WriteBool(m_StoppingPreserving); // #4149
	savefile->WriteFloat(m_MaxForce);
	savefile->WriteInt(m_LockedHeldDist);
	savefile->WriteBool(m_bObjStuck);

	savefile->WriteBool(m_bIsColliding);
	savefile->WriteBool(m_bPrevFrameCollided);
	savefile->WriteInt( m_CollNorms.Num() );
	for (int i = 0; i < m_CollNorms.Num(); i++)
	{
		savefile->WriteVec3( m_CollNorms[i] );
	}

	m_EquippedEnt.Save(savefile);
	savefile->WriteBool(m_bEquippedEntInWorld);
	savefile->WriteVec3(m_vEquippedPosition);
	savefile->WriteInt(m_EquippedEntContents);
	savefile->WriteInt(m_EquippedEntClipMask);

	savefile->WriteBool(m_bDropBodyFaceUp);
	savefile->WriteBool(m_silentMode);
}

void CGrabber::Restore( idRestoreGame *savefile )
{
	m_dragEnt.Restore(savefile);
	savefile->ReadJoint(m_joint);
	savefile->ReadInt(m_id);
	savefile->ReadVec3(m_LocalEntPoint);
	savefile->ReadVec3(m_vLocalEntOffset);
	savefile->ReadVec3(m_vOffset);
	savefile->ReadBool(m_bMaintainPitch);

	m_player.Restore(savefile);
	m_drag.Restore(savefile);
	savefile->ReadBool(m_bAllowPlayerRotation);
	savefile->ReadBool(m_bAllowPlayerTranslation);

	// Read the three relevant values of the idRotation object
	idVec3 origin;
	idVec3 vec;
	float angle;
	savefile->ReadVec3(origin);
	savefile->ReadVec3(vec);
	savefile->ReadFloat(angle);
	m_rotation = idRotation(origin, vec, angle);

	savefile->ReadInt(m_rotationAxis);
	savefile->ReadVec2(m_mousePosition);

	int num;
	savefile->ReadInt(num);
	m_clipList.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_clipList[i].m_ent.Restore(savefile);
		savefile->ReadInt(m_clipList[i].m_clipMask);
		savefile->ReadInt(m_clipList[i].m_contents);
	}

	savefile->ReadBool(m_bAttackPressed);
	savefile->ReadInt(m_ThrowTimer);
	savefile->ReadInt(m_DragUpTimer);
	savefile->ReadFloat(m_AFBodyLastZ);
	savefile->ReadBool(m_bAFOffGround);
	savefile->ReadInt(m_DistanceCount);
	savefile->ReadInt(m_MaxDistCount);
	savefile->ReadInt(m_MinHeldDist);
	savefile->ReadVec3(m_PreservedPosition);  // #4149
	savefile->ReadBool(m_StoppingPreserving); // #4149
	savefile->ReadFloat(m_MaxForce);
	savefile->ReadInt(m_LockedHeldDist);
	savefile->ReadBool(m_bObjStuck);

	savefile->ReadBool(m_bIsColliding);
	savefile->ReadBool(m_bPrevFrameCollided);
	savefile->ReadInt(num);
	m_CollNorms.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadVec3(m_CollNorms[i]);
	}

	m_EquippedEnt.Restore(savefile);
	savefile->ReadBool(m_bEquippedEntInWorld);
	savefile->ReadVec3(m_vEquippedPosition);
	savefile->ReadInt(m_EquippedEntContents);
	savefile->ReadInt(m_EquippedEntClipMask);

	savefile->ReadBool(m_bDropBodyFaceUp);
	savefile->ReadBool(m_silentMode);
}

/*
==============
CGrabber::Spawn
==============
*/
void CGrabber::Spawn( void ) 
{
	//TODO: Change constants at the start of the file and assign them here
	//	using spawnArgs.
}

/*
==============
CGrabber::StopDrag
==============
*/
void CGrabber::StopDrag( void ) 
{
	//stgatilov #5599: allow force class to revert its temporary changes
	m_drag.SetPhysics( NULL, -1, idVec3() );

	m_bIsColliding = false;
	m_bPrevFrameCollided = false;
	m_CollNorms.Clear();
	m_bObjStuck = false;
	
	m_bAFOffGround = false;
	m_DragUpTimer = 0;
	m_AFBodyLastZ = 0.0f;

	m_DistanceCount = 0;
	m_PreservedPosition = vec3_zero; // #4149

	// grayman #2624 - I want to determine if this is a lantern that's being dropped.
	// If it is, I want to post an event to extinguish it. That event will extinguish it only if it's not
	// in a vertical position at the time. I don't see anything here that tells the held object it's
	// being let go, so I assume there's code in the object that recognizes it needs to
	// fall. That's where I'd want to post the extinguish event, but I don't know where that
	// is, so I'm going to put it here. If someone knows how to do this better, please
	// move this code where it belongs.

	// grayman #3858 - treat a dropped moveable prop torch the same way

	idEntity* draggedEntity = m_dragEnt.GetEntity();
	if ( draggedEntity != NULL )
	{
		if ( draggedEntity->spawnArgs.GetBool("is_lantern","0") || draggedEntity->spawnArgs.GetBool("is_torch","0") )
		{
			// Get the delay in milliseconds
			int delay = SEC2MS(draggedEntity->spawnArgs.GetInt("extinguish_on_drop_delay", "4"));
			if ( delay < 0 )
			{
				delay = 0;
			}

			// add a random amount for variability
			int random = SEC2MS(draggedEntity->spawnArgs.GetInt("extinguish_on_drop_delay_random", "0"));
			delay += random * gameLocal.random.RandomFloat();

			// Schedule the extinguish event
			draggedEntity->PostEventMS(&EV_ExtinguishLights, delay);
		}
	}

	m_dragEnt = NULL;

	m_drag.SetRefEnt( NULL );

	idPlayer* player = m_player.GetEntity();
	if (player != NULL)
	{
		player->m_bGrabberActive = false;
		player->SetImmobilization( "Grabber", 0 );
		player->SetHinderance( "Grabber", 1.0f, 1.0f );
	}

	// TODO: This assumes we can never equip an object and drag a second object
	/*
	if( m_EquippedEnt.GetEntity() && m_player.GetEntity() )
		Dequip();
	*/
}

/*
==============
CGrabber::Update
==============
*/
void CGrabber::Update( idPlayer *player, bool hold, bool preservePosition ) 
{
	idVec3 viewPoint(vec3_zero), origin(vec3_zero);
	idVec3 COM(vec3_zero);
	idVec3 draggedPosition(vec3_zero), targetPosition(vec3_zero);
	idMat3 viewAxis(mat3_identity), axis(mat3_identity);
	idAnimator *dragAnimator;
	renderEntity_t *renderEntity;
	float distFactor;
	bool bAttackHeld;
	idEntity *drag;
	idPhysics_Player *playerPhys;
	
	m_silentMode = false;
	if (cv_drag_new.GetBool()) {
		//stgatilov #5599: new grabber, detect if we should set silent mode
		switch (cv_drag_rigid_silentmode.GetInteger()) {
			case 1:
				if (player->m_CreepIntent)
					m_silentMode = true;
				break;
			case 2:
				if ( !(player->usercmd.buttons & BUTTON_RUN) )
					m_silentMode = true;
				break;
			case 3:
				m_silentMode = true;
		}
	}

	m_player = player;

	// if there is an entity selected, we let it go and exit
	if( !hold && m_dragEnt.GetEntity() ) 
	{
		// ClampVelocity( MAX_RELEASE_LINVEL, MAX_RELEASE_ANGVEL, m_id );

		// greebo: Clear the equipped entity reference as well, we're letting go
		m_EquippedEnt = NULL;

		StopDrag();
		
		goto Quit;
	}

	playerPhys = static_cast<idPhysics_Player *>(player->GetPhysics());
	// if the player is climbing a rope or ladder, don't let them grab things
	// greebo: Disabled this, it let things currently held by the grabber drop to the ground
	// and then reattach them after the player has finished climbing
	/*if( playerPhys->OnRope() || playerPhys->OnLadder() )
		goto Quit;*/

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity is currently selected for dragging, start grabbing the frobbed entity
	if ( !m_dragEnt.GetEntity() ) 
	{
		StartDrag( player, NULL, 0, preservePosition ); // preservePosition #4919
	}

	// if there's still not a valid ent, don't do anything
	drag = m_dragEnt.GetEntity();
	if ( !drag || !m_dragEnt.IsValid() )
	{
		goto Quit;

	}

	// Set actor info on the entity
	drag->m_SetInMotionByActor = (idActor *) player;
	drag->m_MovedByActor = (idActor *) player;

	// Check for throwing:
	bAttackHeld = player->usercmd.buttons & BUTTON_ATTACK;

	if( m_bAttackPressed && !bAttackHeld )
	{
		m_bAttackPressed = false;

		// attack button doesn't throw if the item is equipped
		if( !m_bEquippedEntInWorld )
		{
			int HeldTime = gameLocal.time - m_ThrowTimer;
			Throw( HeldTime );
			goto Quit;
		}
	}

	if( !m_bAttackPressed && bAttackHeld )
	{
		m_bAttackPressed = true;

		// start the throw timer
		m_ThrowTimer = gameLocal.time;

		// attack button uses if the item is equipped
		if( m_bEquippedEntInWorld )
			UseEquipped();
	}

	// Update the held distance

	// Lock the held distance to +/- a few increments around the current held dist
	// when collision occurs.
	// Otherwise the player would have to increment all the way back

	// TODO: This isn't really working
	// and maybe we don't want it to work since it means you couldn't
	// slide something closer/farther along a surface

	if(m_bIsColliding)
	{
		if(!m_bPrevFrameCollided)
			m_LockedHeldDist = m_DistanceCount;

		m_DistanceCount = idMath::ClampInt( (m_LockedHeldDist-2), (m_LockedHeldDist+1), m_DistanceCount );
	}

	targetPosition.x = 1.0f; // (1, 0, 0) Relative to player's view, where x (1) is the direction into the screen
	distFactor = (float) m_DistanceCount / (float) m_MaxDistCount;
	targetPosition *= m_MinHeldDist + (m_dragEnt.GetEntity()->m_FrobDistance - m_MinHeldDist) * distFactor;
	targetPosition += m_vOffset;

	if ( PreservingPosition() )			// #4149
	{
		if ( !m_StoppingPreserving )	// Player hasn't started to manipulate the object, so we keep our position
		{
			targetPosition = m_PreservedPosition;
		}
		else 								// we converge with the target position
		{
			const idVec3 pathToTarget = targetPosition - m_PreservedPosition;
			if ( pathToTarget.LengthSqr() > 0.25f )				// The numbers don't matter much. We just
			{													// want a smoother result than insta-snap
				m_PreservedPosition += pathToTarget * 0.07f;
				targetPosition = m_PreservedPosition;
			} else {
				m_PreservedPosition.Zero();						// PreservingPosition() will now return false
				m_StoppingPreserving = false;
				targetPosition = targetPosition;
			}
		}
	} 
	
	if ( m_bEquippedEntInWorld ) // equipped entities lock in position
	{
		targetPosition = m_vEquippedPosition;
	} 


	draggedPosition = viewPoint + targetPosition * viewAxis;

// ====================== AF Grounding Testing ===============================

//stgatilov #5599: this hack is only used for old grabber with AFs
if (!cv_drag_new.GetBool()) {

	// If dragging a body with a certain spawnarg set, you should only be able to pick
	// it up so far off the ground
	if( drag->IsType(idAFEntity_Base::Type) && (cv_drag_AF_free.GetBool() == false) )
	{
		idAFEntity_Base *AFPtr = (idAFEntity_Base *) drag;
		
		if( AFPtr->IsActiveAF() && AFPtr->m_bGroundWhenDragged )
		{
			// Quick fix : Do not add player's reference velocity when grounding
			m_drag.SetRefEnt( NULL );

			// Poll the critical AF bodies and see how many are off the ground
			int OnGroundCount = 0;
			for( int i=0; i<AFPtr->m_GroundBodyList.Num(); i++ )
			{
				if( AFPtr->GetAFPhysics()->HasGroundContactsAtJoint( AFPtr->m_GroundBodyList[i] ) )
					OnGroundCount++;
			}

			// check if the minimum number of these critical bodies remain on the ground
			if( OnGroundCount < AFPtr->m_GroundBodyMinNum )
			{
				m_DragUpTimer = gameLocal.time;

				// do not allow translation higher than current vertical position
				idVec3 bodyOrigin = AFPtr->GetAFPhysics()->GetOrigin( m_id );
				idVec3 UpDir = -AFPtr->GetPhysics()->GetGravityNormal();

				// If the AF just went off the ground, copy the last valid Z
				if( !m_bAFOffGround )
				{
					m_AFBodyLastZ = bodyOrigin * UpDir;
					m_bAFOffGround = true;
				}

				float deltaVert = draggedPosition * UpDir - m_AFBodyLastZ;
				if( deltaVert > 0 )
					draggedPosition -= deltaVert * UpDir;
			}
			else if( (gameLocal.time - m_DragUpTimer) < cv_drag_AF_ground_timer.GetInteger() )
			{
				idVec3 bodyOrigin = AFPtr->GetAFPhysics()->GetOrigin( m_id );
				idVec3 UpDir = -AFPtr->GetPhysics()->GetGravityNormal();
				float deltaVert = draggedPosition * UpDir - m_AFBodyLastZ;
				if( deltaVert > 0 )
				{
					float liftTimeFrac = (float)(gameLocal.time - m_DragUpTimer)/(float)cv_drag_AF_ground_timer.GetInteger();
					// try a quadratic buildup, sublinear for frac < 1
					liftTimeFrac *= liftTimeFrac;
					draggedPosition -= deltaVert * UpDir * (1.0 - liftTimeFrac);
				}

				m_bAFOffGround = false;
			}
			else
			{
				// If somehow this is not set
				m_bAFOffGround = false;
			}
			
		}
	}

}

// ===================== End AF Grounding Test ====================

	m_drag.SetDragPosition( draggedPosition );

	// Test: Drop the object if it is stuck
	// TODO: Also put absolute distance check and angle offset check in here
	CheckStuck();
	if( m_bObjStuck )
	{
		StopDrag();
		// m_EquippedEnt = NULL;
	}

	// evaluate physics
	// Note: By doing these operations in this order, we overwrite idForce_Drag angular velocity
	// calculations which is what we want so that the manipulation works properly
	m_drag.Evaluate( gameLocal.time );
	this->ManipulateObject( player );

	renderEntity = drag->GetRenderEntity();
	dragAnimator = drag->GetAnimator();

	if ( m_joint != INVALID_JOINT && renderEntity && dragAnimator ) 
	{
		dragAnimator->GetJointTransform( m_joint, gameLocal.time, draggedPosition, axis );
	}

Quit:
	m_bPrevFrameCollided = m_bIsColliding;
	
	// reset this, it gets set by physics if the object collides
	m_bIsColliding = false;
	return;
}

void CGrabber::StartDrag( idPlayer *player, idEntity *newEnt, int bodyID, bool preservePosition ) 
{
	idVec3 viewPoint, origin, COM, COMWorld, delta2;
	idEntity *FrobEnt;
	idMat3 viewAxis, axis;
	trace_t trace;
	jointHandle_t newJoint;

	// Just in case we were called while holding another item:
	StopDrag();
	m_EquippedEnt = NULL;

	player->GetViewPos( viewPoint, viewAxis );

	// If an entity was not explictly passed in, use the frob entity
	if ( !newEnt ) 
	{
		FrobEnt = player->m_FrobEntity.GetEntity();
		if ( !FrobEnt )
		{
			return;
		}

		newEnt = FrobEnt;

		trace = player->m_FrobTrace;
		
		// If the ent was not hit directly and is an AF, we must fill in the joint and body ID
		if ( (trace.c.entityNum != FrobEnt->entityNumber) && FrobEnt->IsType(idAFEntity_Base::Type) )
		{
			static_cast<idPhysics_AF *>(FrobEnt->GetPhysics())->NearestBodyOrig( trace.c.point, &trace.c.id );
		}
	}
	else
	{
		// An entity was passed in to grab directly
		trace.c.id = bodyID;
	}

	bool isHead = false;
	if ( newEnt->GetBindMaster() ) 
	{
		// grayman #3631 - If the ent is a head, assign it an invalid joint number.
		// When the frobbed entity is change to the bindMaster body, this assignment
		// causes the grabber to manipulate the body via the head.
		idEntity* bindMaster = newEnt->GetBindMaster();
		if (bindMaster->IsType(idActor::Type))
		{
			idActor* actor = static_cast<idActor*>(bindMaster);
			idAFAttachment* head = actor->GetHead();
			if (head == newEnt)
			{
				isHead = true;
				trace.c.id = 2; // a particular invalid joint number
				newEnt = bindMaster; // move to the body
			}
		}
	
		if (!isHead)
		{
			if ( newEnt->GetBindJoint() ) 
			{
				trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( newEnt->GetBindJoint() );
			}
			else 
			{
				trace.c.id = newEnt->GetBindBody();
			}
			newEnt = bindMaster;			
		}
	}

	if ( newEnt->IsType( idAFEntity_Base::Type ) && static_cast<idAFEntity_Base *>(newEnt)->IsActiveAF() ) 
	{
		idAFEntity_Base *af = static_cast<idAFEntity_Base *>(newEnt);

		// joint being dragged
		newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
		// get the body id from the trace model id which might be a joint handle
		trace.c.id = af->BodyForClipModelId( trace.c.id );
	} 
	else if ( !newEnt->IsType( idWorldspawn::Type ) ) 
	{
		if ( trace.c.id < 0 ) 
		{
			newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
		}
		else 
		{
			newJoint = INVALID_JOINT;
		}
	}
	else 
	{
		newJoint = INVALID_JOINT;
		newEnt = NULL;
	}
	
	// If the new entity still wasn't set at this point, it was a frobbed
	// ent but was found to be invalid.

	if ( !newEnt )
	{
		return;
	}
	
	// Set up the distance and orientation and stuff

	// get the center of mass
	idPhysics *phys = newEnt->GetPhysics();
	idClipModel *clipModel;

	clipModel = phys->GetClipModel( trace.c.id ); // grayman #3631 new way (was (m_id))
	if ( clipModel && clipModel->IsTraceModel() )
	{
		float mass;
		idMat3 inertiaTensor;
		clipModel->GetMassProperties( 1.0f, mass, COM, inertiaTensor );
	}
	else 
	{
		// don't drag it if its clipmodel is not a trace model
		return;
	}

	COMWorld = phys->GetOrigin( trace.c.id ) + COM * phys->GetAxis( trace.c.id ); // grayman #3631 new way (was (m_id))

	m_dragEnt = newEnt;
	m_joint = newJoint;
	m_id = trace.c.id;

	origin = phys->GetOrigin( m_id );
	axis = phys->GetAxis( m_id );
	m_LocalEntPoint = COM;

	// find the nearest distance and set it to that
	m_MinHeldDist = int(newEnt->spawnArgs.GetFloat("hold_distance_min", "-1" ));
	if ( m_MinHeldDist < 0 )
	{
		m_MinHeldDist = int(MIN_HELD_DISTANCE);
	}
	m_vOffset = newEnt->spawnArgs.GetVector( "hold_offset" );

	delta2 = COMWorld - viewPoint;
	m_DistanceCount = int(idMath::Floor( m_MaxDistCount * (delta2.Length() - m_MinHeldDist) / (newEnt->m_FrobDistance - m_MinHeldDist ) ));
	m_DistanceCount = idMath::ClampInt( 0, m_MaxDistCount, m_DistanceCount );

	if ( preservePosition ) // #4149
	{
		// preservePosition means: Leave a picked-up item where it is until the player
		// deliberately moves it, instead of snapping it to the nearest DistanceCount.
		// Do add a tiny amount of height, so items placed exactly on a surface can't sink into it.
		m_PreservedPosition = ( delta2 + idVec3(0.0f, 0.0f, 0.1f) ) * viewAxis.Transpose();
	} else {
		m_PreservedPosition.Zero();
	}

	// prevent collision with player
	AddToClipList( newEnt );
	if ( HasClippedEntity() ) 
	{
		PostEventMS( &EV_Grabber_CheckClipList, CHECK_CLIP_LIST_INTERVAL );
	}

	// signal object manipulator to update drag position so it's relative to the objects
	// center of mass instead of its origin
	m_rotationAxis = -1;

	if ( newEnt->IsType(idAFEntity_Base::Type)
		&& static_cast<idAFEntity_Base *>(newEnt)->m_bDragAFDamping == true )
	{
		m_drag.Init( cv_drag_damping_AF.GetFloat() );
	}
	else
	{
		m_drag.Init( cv_drag_damping.GetFloat() );
	}

	m_drag.SetPhysics( phys, m_id, m_LocalEntPoint );
	m_drag.SetRefEnt( player );
	float ForceMod = newEnt->spawnArgs.GetFloat("drag_force_mod", "1.0");
	m_MaxForce = cv_drag_force_max.GetFloat() * ForceMod;

	player->m_bGrabberActive = true;

	// Apply Grabber Restrictions and Encumbrance
	SetDragEncumbrance();

	m_drag.LimitForce( cv_drag_limit_force.GetBool() );
	m_drag.ApplyDamping( true );

	return;
}

/*
==============
CGrabber::ManipulateObject
==============
*/
void CGrabber::ManipulateObject( idPlayer *player ) {
	idVec3 viewPoint;
	idMat3 viewAxis;

	player->GetViewPos( viewPoint, viewAxis );

	idEntity *ent;
	idVec3 angularVelocity;
	idPhysics *physics;
	idVec3 rotationVec(0.0, 0.0, 0.0);
	bool rotating;

	ent = m_dragEnt.GetEntity();
	if( !ent ) {
		return;
	}

	physics = ent->GetPhysics();
	if ( !physics ) {
		return;
	}

	angularVelocity = vec3_origin;

	// NOTES ON OBJECT ROTATION
	// 
	// The way the object rotation works is as follows:
	//	1) Player must be holding BUTTON_ZOOM
	//	2) if the player is holding BUTTON_RUN, rotate about the z-axis
	//	   else then if the mouse first moves along the x axis, rotate about the x-axis
	//				 else if the mouse first moves along the y axis, rotate about the y-axis
	//
	// This system may seem complicated but I found after playing with it for a few minutes
	// it's quite easy to use.  It also offers some throttling of rotation speed. (Besides, 
	// who uses the ZOOM button anyway?)
	//
	// If the player releases the ZOOM button rotation slows down.
	// To sum it all up...
	//
	// If the player holds ZOOM, make the object rotated based on mouse movement.
	if
	( 
		m_bAllowPlayerRotation 
		&& !ent->IsType( idAFEntity_Base::Type ) 
		&& player->usercmd.buttons & BUTTON_ZOOM 
	) 
	{

		float angle = 0.0f;
		rotating = true;

		// Disable player view change while rotating
		player->SetImmobilization( "Grabber", player->GetImmobilization("Grabber") | EIM_VIEW_ANGLE );
		
		if( !this->DeadMouse() || player->usercmd.jx != 0 || player->usercmd.jy != 0 ) 
		{
			float xMag, yMag;
			idVec3 xAxis, yAxis;

			xMag = player->usercmd.mx - m_mousePosition.x + player->usercmd.jx / 64.f;
			yMag = player->usercmd.my - m_mousePosition.y + player->usercmd.jy / 64.f;

			yAxis.Set( 0.0f, 1.0f, 0.0f ); // y is always pitch

			// run held => x = roll, release => x = yaw
			if( player->usercmd.buttons & BUTTON_RUN )
				xAxis.Set( -1.0f, 0.0f, 0.0f ); // roll
			else
				xAxis.Set( 0.0f, 0.0f, -1.0f ); // yaw

			rotationVec = xMag*xAxis + yMag*yAxis;
			angle = rotationVec.Normalize();

			m_mousePosition.x = player->usercmd.mx;
			m_mousePosition.y = player->usercmd.my;
		}

		angle = angle * MOUSE_SCALE;

		// Convert rotation axis from player-view coords to world coords
		idAngles viewAnglesXY = viewAxis.ToAngles();
		// ignore the change in player pitch angles
		viewAnglesXY[0] = 0;
		idMat3 viewAxisXY = viewAnglesXY.ToMat3();
		
		rotationVec = rotationVec * viewAxisXY;
		rotationVec.Normalize();

		idVec3 RotPointWorld = physics->GetOrigin( m_id ) + m_LocalEntPoint * physics->GetAxis( m_id );
		idRotation DesiredRot;
		DesiredRot.Set( RotPointWorld, rotationVec, angle );

		// TODO: Toggle visual debugging with cvar
		// gameRenderWorld->DebugLine( colorRed, RotPointWorld, (RotPointWorld + 30 * rotationVec), 1);

		// Calc. desired cumulative rotation
		m_drag.SetDragAxis( m_drag.GetDragAxis() * DesiredRot.ToMat3() );

		// #4149. Let the item now go to standard hold position, because player is rotating it
		if ( PreservingPosition() )
		{
			m_StoppingPreserving = true;
		}
	}
	else 
	{
		rotating = false;

		// Ishtvan: Enable player view change
		player->SetImmobilization( "Grabber", player->GetImmobilization("Grabber") & (~EIM_VIEW_ANGLE) );

		// reset these coordinates so that next time they press zoom the rotation will be fresh
		m_mousePosition.x = player->usercmd.mx;
		m_mousePosition.y = player->usercmd.my;

		// reset rotation information so when the next zoom is pressed we can freely rotate again
		if( m_rotationAxis )
			m_rotationAxis = 0;

		angularVelocity = vec3_zero;
		m_drag.SetDragAxis( ent->GetPhysics()->GetAxis( m_id ) );
	}


// ============== rotate object so it stays oriented with the player ===========

	if( !ent->IsType( idAFEntity_Base::Type ) && !rotating && !m_bIsColliding ) 
	{
		idVec3	normal;
		float	deltaYawAng(0), deltaPitchAng(0);
		normal = physics->GetGravityNormal();

		deltaYawAng = static_cast<idPhysics_Player *>(player->GetPhysics())->GetDeltaViewYaw();
		m_rotation.Set(m_drag.GetCenterOfMass(), normal, deltaYawAng );
		
		// rotate the object directly
		trace_t trResults;
		physics->ClipRotation( trResults, m_rotation, NULL );
		physics->Rotate( m_rotation * trResults.fraction );

		// FIXME: If a bindmaster hits a bind slave when rotated, will it stop??
		// debugging this here:
		if( trResults.fraction < 1.0f )
		{
			// gameLocal.Printf("GRABBER ROTATION TO FACE PLAYER: Grabbed object %s hit object %s\n", m_dragEnt.GetEntity()->name.c_str(), gameLocal.entities[trResults.c.entityNum]->name.c_str() );
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("GRABBER ROTATION TO FACE PLAYER: Grabbed object %s hit object %s\r", m_dragEnt.GetEntity()->name.c_str(), gameLocal.entities[trResults.c.entityNum]->name.c_str() );
		}

		if( !m_bMaintainPitch )
		{
			idVec3 dummy;
			deltaPitchAng = static_cast<idPhysics_Player *>(player->GetPhysics())->GetDeltaViewPitch();
			
			player->viewAngles.ToVectors( &dummy, &normal, &dummy );
			m_rotation.Set(m_drag.GetCenterOfMass(), normal, deltaPitchAng );
			
			// rotate the object directly
			physics->ClipRotation( trResults, m_rotation, NULL );
			physics->Rotate( m_rotation * trResults.fraction );
		}
		
		// Update the axis again since we rotated
		m_drag.SetDragAxis( ent->GetPhysics()->GetAxis( m_id ) );
	}
}

/*
==============
CGrabber::DeadMouse
==============
*/
bool CGrabber::DeadMouse( void ) 
{
	// check mouse is in the deadzone along the x-axis or the y-axis
	if( idMath::Fabs( m_player.GetEntity()->usercmd.mx - m_mousePosition.x ) > MOUSE_DEADZONE ||
		idMath::Fabs( m_player.GetEntity()->usercmd.my - m_mousePosition.y ) > MOUSE_DEADZONE )
		return false;

	return true;
}

/*
==============
CGrabber::AddToClipList
==============
*/
void CGrabber::AddToClipList( idEntity *ent ) 
{
	if( !ent )
		return;

	CGrabbedEnt obj;
	idPhysics *phys = ent->GetPhysics();
	int clipMask = phys->GetClipMask();
	int contents = phys->GetContents();

	obj.m_ent = ent;
	obj.m_clipMask = clipMask;
	obj.m_contents = contents;

	// set the clipMask so that the player won't colide with the object but it still
	// collides with the world
	// Add can now be called on bind children that aren't solid
	// so make sure it starts out with some form of solid before adding solid contents
	bool bAddToList(false);
	if( clipMask & (CONTENTS_SOLID|CONTENTS_CORPSE) )
	{
		phys->SetClipMask( clipMask & (~MASK_PLAYERSOLID) );
		phys->SetClipMask( phys->GetClipMask() | CONTENTS_SOLID );
		bAddToList = true;
	}
	
	// Clear the solid flag to avoid player collision, 
	// but enable monsterclip for AI, rendermodel for projectiles and corpse for moveables
	// clear the opaque flag so that AI can see through carried items
	if( contents & (CONTENTS_SOLID|CONTENTS_CORPSE) )
	{
		phys->SetContents
		( 
			(contents & ~(CONTENTS_SOLID | CONTENTS_OPAQUE))
			| CONTENTS_MONSTERCLIP | CONTENTS_RENDERMODEL | CONTENTS_CORPSE
		);
		bAddToList = true;
	}

	if( bAddToList )
	{
		// gameLocal.Printf("AddToClipList: Added Entity %s\n", ent->name.c_str() );
		m_clipList.AddUnique( obj );
	}

	// add the bind children of the entity to the clip list as well
	// NOTE: We always grab the bindmaster first, avoid recursive loops
	// TODO: What about case of a ragdoll bound to something?  Grabbing won't work at all?
	if( ent->GetBindMaster() == NULL )
	{
		idList<idEntity *> BindChildren;
		ent->GetTeamChildren( &BindChildren );
		for( int i = 0; i < BindChildren.Num(); i++ )
		{
			AddToClipList( BindChildren[i] );
		}
		// gameLocal.Printf("AddToClipList: Entity %s has %d team children\n", ent->name.c_str(), BindChildren.Num() );
	}
}

/*
==============
CGrabber::RemoveFromClipList
==============
*/
void CGrabber::RemoveFromClipList( int index ) 
{
	// remove the entity and reset the clipMask
	if( index != -1)
	{
		if (m_clipList[index].m_ent.GetEntity() != NULL)
		{
			m_clipList[index].m_ent.GetEntity()->GetPhysics()->SetClipMask( m_clipList[index].m_clipMask );
			m_clipList[index].m_ent.GetEntity()->GetPhysics()->SetContents( m_clipList[index].m_contents );
		}
		m_clipList.RemoveIndex( index );
	}

	if( !this->HasClippedEntity() )
	{
		// cancel CheckClipList because the list is empty
		this->CancelEvents( &EV_Grabber_CheckClipList );
	}
}

void CGrabber::RemoveFromClipList(idEntity* entity)
{
	for (int i = 0; i < m_clipList.Num(); i++) {
		if (m_clipList[i].m_ent.GetEntity() == entity) {
			m_clipList.RemoveIndex(i);
			break;
		}
	}
	
	if( !this->HasClippedEntity() ) {
		// cancel CheckClipList because the list is empty
		this->CancelEvents( &EV_Grabber_CheckClipList );
	}
}

/*
==============
CGrabber::Event_CheckClipList
==============
*/
void CGrabber::Event_CheckClipList( void ) 
{
	idEntity *ListEnt(NULL);
	idClipModel *PlayerClip(NULL), *EntClip(NULL);
	trace_t tr;
	bool keep;
	int i;	

	// Check for any entity touching the players bounds
	// If the entity is not in our list, remove it.
	// num = gameLocal.clip.EntitiesTouchingBounds( m_player.GetEntity()->GetPhysics()->GetAbsBounds(), CONTENTS_SOLID, ent );
	for( i = 0; i < m_clipList.Num(); i++ ) 
	{
		// Check clipEntites against entities touching player
		ListEnt = m_clipList[i].m_ent.GetEntity();

		if( !ListEnt )
		{
			// not sure how this is happening, but fix it
			//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("GRABBER CLIPLIST: Removing NULL entity from cliplist\r" );
			keep = false;
		}
		// We keep an entity if it is the one we're dragging 
		else if( GetSelected() == ListEnt || (ListEnt->GetBindMaster() && GetSelected() == ListEnt->GetBindMaster()) ) 
		{
			//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("GRABBER CLIPLIST: Keeping entity %s in cliplist as it is currently selected\r", ListEnt->name.c_str() );
			keep = true;
		}
		else 
		{
			keep = false;

			// OR if it's currently touching the player
			if( m_player.GetEntity() 
				&& (EntClip = ListEnt->GetPhysics()->GetClipModel()) != NULL )
			{
				//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("GRABBER CLIPLIST: Testing entity %s for player clipping\r", ListEnt->name.c_str() );
				PlayerClip = m_player.GetEntity()->GetPhysics()->GetClipModel();
				idVec3 PlayerOrigin = PlayerClip->GetOrigin();
				
				// test if player model clips into ent model
				gameLocal.clip.TranslationModel
				( 
					tr, PlayerOrigin, PlayerOrigin,
					PlayerClip, PlayerClip->GetAxis(), CONTENTS_RENDERMODEL,
					EntClip->Handle(), EntClip->GetOrigin(), 
					EntClip->GetAxis() 
				);

				if( tr.fraction < 1.0 )
				{
					keep = true;
					//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("GRABBER CLIPLIST: Keeping entity %s in cliplist since it is still clipping player\r", ListEnt->name.c_str() );
				}
				else
				{
					//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("GRABBER CLIPLIST: Entity %s removed from cliplist since it is not selected or clipping player\r", ListEnt->name.c_str() );
				}

			}
		}

		// Note we have to decrement i otherwise we skip entities
		if( !keep ) 
		{
			this->RemoveFromClipList( i );
			i -= 1;
		}
	}

	if( this->HasClippedEntity() ) 
	{
		this->PostEventMS( &EV_Grabber_CheckClipList, CHECK_CLIP_LIST_INTERVAL );
	}
}

void CGrabber::SetPhysicsFromDragEntity() 
{
	if (m_dragEnt.GetEntity() != NULL)
	{
		m_drag.SetPhysics(m_dragEnt.GetEntity()->GetPhysics(), m_id, m_LocalEntPoint );
	}
}

/*
==============
CGrabber::IsInClipList
==============
*/
// TODO / FIXME: Will this work if we don't set the matching contents and clipmask also?
bool CGrabber::IsInClipList( idEntity *ent ) const 
{
	CGrabbedEnt obj;
	
	obj.m_ent = ent;

	// check if the entity is in the clipList
	if( m_clipList.FindIndex( obj ) == -1 ) 
	{
		return false;
	}
	return true;
}


/*
==============
CGrabber::HasClippedEntity
==============
*/
bool CGrabber::HasClippedEntity( void ) const {
	if( m_clipList.Num() > 0 ) {
		return true;
	}
	return false;
}

/*
==============
CGrabber::Throw
==============
*/
void CGrabber::Throw( int HeldTime )
{
	idVec3 ImpulseVec(vec3_zero), IdentVec( 1, 0, 1), ThrowPoint(vec3_zero), TumbleVec(vec3_zero);

	idEntity *ent = m_dragEnt.GetEntity();
	ImpulseVec = m_player.GetEntity()->firstPersonViewAxis[0];
	ImpulseVec.Normalize();

	float FracPower = (float) HeldTime / (float) cv_throw_time.GetInteger();

	if( FracPower > 1.0 )
		FracPower = 1.0;

	float mass = m_dragEnt.GetEntity()->GetPhysics()->GetMass();

	// Try out a linear scaling between max and min
	float ThrowImpulse = cv_throw_impulse_min.GetFloat() * (1.0f - FracPower) + cv_throw_impulse_max.GetFloat() * FracPower;
	float VelocityLimit = cv_throw_vellimit_min.GetFloat() * (1.0f - FracPower) + cv_throw_vellimit_max.GetFloat() * FracPower;
	// Clamp to max velocity
	float ThrowVel = ThrowImpulse / (mass + 1e-10f);
	ThrowVel = idMath::ClampFloat( 0.0f, VelocityLimit, ThrowVel );
	ImpulseVec *= ThrowVel * mass;

	ClampVelocity( MAX_RELEASE_LINVEL, MAX_RELEASE_ANGVEL, m_id );

	// Only apply the impulse for throwable items
	if (ent->spawnArgs.GetBool("throwable", "1")) 
	{
		ThrowPoint = ent->GetPhysics()->GetOrigin(m_id) + ent->GetPhysics()->GetAxis(m_id) * m_LocalEntPoint;
		// tels
		// add a small random offset to ThrowPoint, so the object tumbles
		TumbleVec[0] += (gameLocal.random.RandomFloat() / 20) - 0.025; 
		TumbleVec[1] += (gameLocal.random.RandomFloat() / 20) - 0.025; 
		TumbleVec[2] += (gameLocal.random.RandomFloat() / 20) - 0.025;
		// scale the offset by the mass, so lightweight objects don't spin too much
		TumbleVec *= (mass / 2.0);
		ent->ApplyImpulse( m_player.GetEntity(), m_id, ThrowPoint + TumbleVec, ImpulseVec );
		ent->m_SetInMotionByActor = m_player.GetEntity();
		ent->m_MovedByActor =  m_player.GetEntity();
	}

	StopDrag();
	// tels: also clear the equipped entity, in case Equip() was called
	m_EquippedEnt = NULL;
}

void CGrabber::ClampVelocity(float maxLin, float maxAng, int idVal)
{
	idVec3 linear, angular;
	float  lengthLin(0), lengthAng(0), lengthLin2(0), lengthAng2(0);

	if( m_dragEnt.GetEntity() )
	{
		idEntity *ent = m_dragEnt.GetEntity();
		linear = ent->GetPhysics()->GetLinearVelocity( idVal );
		angular = ent->GetPhysics()->GetAngularVelocity( idVal );
		// only do this when we let go or throw, so can afford to do a sqrt here
		lengthLin = linear.Length();
		lengthAng = angular.Length();
		lengthLin2 = idMath::ClampFloat(0.0f, maxLin, lengthLin );
		lengthAng2 = idMath::ClampFloat(0.0f, maxAng, lengthAng );

		if( lengthLin > 0 )
			ent->GetPhysics()->SetLinearVelocity( linear * (lengthLin2/lengthLin), idVal );
		if( lengthAng > 0 )
			ent->GetPhysics()->SetAngularVelocity( angular * (lengthAng2/lengthAng), idVal );
	}
}

void CGrabber::IncrementDistance( bool bIncrease )
{
	if ( !m_dragEnt.GetEntity() || !m_bAllowPlayerTranslation )
	{ 
		return; 
	}
	
	// #4149. Let the item now go to standard hold position, because player is manipulating it
	if ( PreservingPosition() ) 
	{
		// No more overriding, the distance is changing so it'll snap to position from now on.
		m_StoppingPreserving = true; 
		// The m_DistanceCount in this situation will always be <= the original depth of the 
		// picked-up item. So if the player is pulling the item closer, don't change it the first time.
		if ( !bIncrease ) 
		{ 
			return; 
		}
	}
	
	m_DistanceCount += bIncrease ? 1 : -1;
	m_DistanceCount = idMath::ClampInt( 0, m_MaxDistCount, m_DistanceCount );
}

bool CGrabber::PutInHands(idEntity *ent, idMat3 axis, int bodyID)
{
	idVec3 COMpoint = GetHoldPoint(ent, axis, bodyID);
	return PutInHandsAtPoint(ent, COMpoint, axis, bodyID);
}

bool CGrabber::PutInHands(idEntity *ent, idVec3 point, idMat3 axis, int bodyID)
{
	return PutInHandsAtPoint(ent, point, axis, bodyID);
}

bool CGrabber::PutInHandsAtPoint(idEntity *ent, idVec3 point, idMat3 axis, int bodyID)
{
	if( !ent || !m_player.GetEntity() )
		return false;

	bool	bReturnVal = false;
	idVec3	targetCOM(vec3_zero), viewPoint;
	idMat3	viewAxis;

	m_player.GetEntity()->GetViewPos( viewPoint, viewAxis );
	targetCOM = point;

	bReturnVal = FitsInWorld( ent, viewPoint, targetCOM, axis, bodyID );
	
	if( !bReturnVal )
		return false;
	
	ent->SetAxis( axis );

	// teleport in the object, put its center of mass at the target point
	idVec3 COMLocal(vec3_zero);
	idClipModel *ClipModel = NULL;
	idPhysics *phys = ent->GetPhysics();

	ClipModel = phys->GetClipModel( bodyID );
	if( ClipModel && ClipModel->IsTraceModel() ) 
	{
		float mass;
		idMat3 inertiaTensor;
		ClipModel->GetMassProperties( 1.0f, mass, COMLocal, inertiaTensor );
	}

	idVec3 orig = targetCOM - ( phys->GetAxis( bodyID ) * COMLocal );
	
	// if this is not body 0 on an AF, need an additional offset to put desired body in hand
	// (already taken into account in the FitsInWorld clipping test)
	if( bodyID > 0 )
		orig -= phys->GetOrigin( bodyID ) - phys->GetOrigin( 0 );

	ent->SetOrigin( orig );
	// Tels: re-enable LOD here, including all possible attachments
	LodComponent::EnableLOD( ent, true ) ;
	ent->Show();

	StartDrag( m_player.GetEntity(), ent, bodyID );

	return true;
}

bool CGrabber::FitsInHands(idEntity *ent, idMat3 axis, int bodyID)
{
	if( !ent || !m_player.GetEntity() )
		return false;

	idVec3	viewPoint, holdPoint;
	idMat3	viewAxis;

	m_player.GetEntity()->GetViewPos( viewPoint, viewAxis );
	holdPoint = GetHoldPoint(ent, axis, bodyID );
	return FitsInWorld( ent, viewPoint, holdPoint, axis, bodyID );
}

bool CGrabber::FitsInWorld( idEntity *ent, idVec3 viewPoint, idVec3 point, idMat3 axis, int bodyID )
{
	int ContentsMask = CONTENTS_SOLID | CONTENTS_CORPSE | CONTENTS_RENDERMODEL;
	trace_t tr;

	// Optimization: If a solid is between viewpoint and target point, we're done
	gameLocal.clip.TracePoint( tr, viewPoint, point, ContentsMask, m_player.GetEntity() );
	if( tr.fraction < 1.0f )
		return false;

	// Proceed with the full test..
	// Momentarily show the entity if it was hidden, to update the clipmodel positions
	bool bStartedHidden = ent->IsHidden();
	ent->Show();

	idPhysics *phys = ent->GetPhysics();

	// rotate to new axis (will rotate all clipmodels around master body if it's an AF)
	// ishtvan: axis-setting disabled until AF stretching on rotation issue is resolved
	phys->SetAxis( axis );
	// translate by: the difference between the clipmodel's center of mass and the desired point
	idVec3 COMLocal(vec3_zero);
	idClipModel *ClipModel = NULL;

	ClipModel = phys->GetClipModel( bodyID );
	if( ClipModel && ClipModel->IsTraceModel() ) 
	{
		float mass;
		idMat3 inertiaTensor;
		ClipModel->GetMassProperties( 1.0f, mass, COMLocal, inertiaTensor );
	}
	idVec3 COM = ClipModel->GetOrigin() + phys->GetAxis( bodyID ) * COMLocal;
	// if we know the translation vector between current COM and target COM,
	// we may add the same translation to the origin to put the COM at target

	// we're going to try to translate the clipmodel COM from the viewpoint to the target point
	// (this is to prevent the player dropping things on the other side of walls)
	idVec3 trans1 = viewPoint - COM; // current COM to viewpoint
	idVec3 trans2 = point - COM; // current COM to target point

	// When we translate an AF, this always operates on body 0 / master body
	// therefore we must get the offset to put that particular body in the player's hands
	if( bodyID > 0 )
	{
		idVec3 offset = phys->GetOrigin(bodyID) - phys->GetOrigin(0);
		trans1 -= offset;
		trans2 -= offset;
	}

	// test each clipmodel on a translation from viewpoint-frame to target-frame
	// probably expensive, but doesn't get called often
	bool bCollided = false;

	// since we can't have two ignore entities and we want to ignore the player,
	// temporarily make the entity we're testing nonsolid (otherwise we can hit it).
	int EntContents = phys->GetContents();
	phys->SetContents(0);

	for( int i=0; i < phys->GetNumClipModels(); i++ )
	{
		ClipModel = phys->GetClipModel(i);
		if( !ClipModel || !ClipModel->IsTraceModel() )
			continue;

		idVec3 clipOrig = ClipModel->GetOrigin();

		// Uncomment for debug display of this collision test
		/*
		collisionModelManager->DrawModel
		(
			ClipModel->Handle(), clipOrig + trans2, ClipModel->GetAxis(),
			gameLocal.GetLocalPlayer()->GetEyePosition(), idMath::INFINITY 
		);
		*/

		// first try a zero-motion translation to see if we start out in solid
		gameLocal.clip.Translation
		( 
			tr, clipOrig + trans1, clipOrig + trans1,
			ClipModel, ClipModel->GetAxis(), ContentsMask,
			m_player.GetEntity()
		);
		if( tr.fraction < 1.0f )
		{
			// gameLocal.Printf("Drop trace hit entity %s\n", gameLocal.entities[tr.c.entityNum]->name.c_str() );
			bCollided = true;
		}

		// start out okay, do the real translation
		gameLocal.clip.Translation
		( 
			tr, clipOrig + trans1, clipOrig + trans2,
			ClipModel, ClipModel->GetAxis(), ContentsMask,
			m_player.GetEntity()
		);
		if( tr.fraction < 1.0f )
		{
			// gameLocal.Printf("Drop trace hit entity %s\n", gameLocal.entities[tr.c.entityNum]->name.c_str() );
			bCollided = true;
		}

		if( bCollided )
			break;
	}
	// set the entity back the way it was after the tests are done
	phys->SetContents(EntContents);

	if(bStartedHidden)
	{
		ent->Hide();
		// SetAxis leaves the clipmodel linked, which creates collisions where there should be nothing
		phys->UnlinkClip();
	}

	return !bCollided;
}

idVec3 CGrabber::GetHoldPoint( idEntity *ent, idMat3 axis, int bodyID )
{
	float	HeldDist = 0.0f;
	idVec3	targetCOM(vec3_zero);
	idVec3	forward(1.0f, 0.0f, 0.0f), viewPoint;
	idMat3	viewAxis;

	m_player.GetEntity()->GetViewPos( viewPoint, viewAxis );

	// calculate where the origin should end up based on center of mass location and orientation
	// also based on the minimum held distance
	HeldDist = ent->spawnArgs.GetFloat("hold_distance_min", "-1" );
	if( HeldDist < 0 )
		HeldDist = MIN_HELD_DISTANCE;

	targetCOM = (HeldDist * forward ) * viewAxis;
	targetCOM += viewPoint;

	return targetCOM;
}

void CGrabber::CheckStuck( void )
{
	idVec3 delta;
	float lensqr, maxsqr;
	idEntity *ent = m_dragEnt.GetEntity();
	bool bColliding(false);
	
	if( ent->IsType(idAFEntity_Base::Type) )
		bColliding = ent->GetPhysics()->GetNumContacts() > 0;
	else
		bColliding = m_bIsColliding;

	if( ent && bColliding )
	{
		delta = m_drag.GetDraggedPosition() - m_drag.GetDragPosition();
		lensqr = delta.LengthSqr();

		maxsqr = cv_drag_stuck_dist.GetFloat();
		maxsqr = maxsqr * maxsqr;

		if( lensqr > maxsqr )
			m_bObjStuck = true;
		else
			m_bObjStuck = false;
	}
}

void CGrabber::SetDragEncumbrance( void )
{
	int Immobilizations(0);
	float encumbrance(0.0f), mass(0.0f), minmass(0.0f), maxmass(0.0f);
	
	idPlayer *player = m_player.GetEntity();

	if( !player )
		goto Quit;

	// don't let the player switch weapons or items, and lower their weapon
	Immobilizations = EIM_ITEM_SELECT | EIM_WEAPON_SELECT | EIM_ATTACK | EIM_ITEM_USE;

	mass = m_dragEnt.GetEntity()->GetPhysics()->GetMass();
	// Don't jump if mass is over the limit
	// TODO: Later reduce jump height continuously with mass carried?
	if( mass > cv_drag_jump_masslimit.GetFloat() )
		Immobilizations = Immobilizations | EIM_JUMP;

	player->SetImmobilization( "Grabber", Immobilizations );

	minmass = cv_drag_encumber_minmass.GetFloat();
	maxmass = cv_drag_encumber_maxmass.GetFloat();

	if( mass < minmass )
		mass = minmass;
	else if( mass > maxmass )
		mass = maxmass;

	encumbrance = 1.0f - ((mass - minmass)/(maxmass - minmass) * ( 1.0f - cv_drag_encumber_max.GetFloat() ));
		
	// Set movement encumbrance
	player->SetHinderance( "Grabber", 1.0f, encumbrance );

Quit:
	return;
}

bool CGrabber::ToggleEquip( void )
{
	bool rc(false);

	if( m_EquippedEnt.GetEntity() )
	{
		rc = true; // always register action when trying to dequip
		Dequip();
	}
	else
		rc = Equip();

	return rc;
}

bool CGrabber::EquipFrobEntity( idPlayer *player )
{
	idEntity* frobEnt = player->m_FrobEntity.GetEntity();

	// If attachment, such as head, get its body.
	idEntity* ent = (frobEnt && frobEnt->IsType(idAFAttachment::Type))
		? static_cast<idAFAttachment*>(frobEnt)->GetBindMaster()
		: frobEnt;

	if (!(ent && ent->spawnArgs.GetBool("equippable")))
		return false;

	if (EquipEntity(ent))
	{
		if (ent->IsType(idAFEntity_Base::Type))
		{
			// If the body ent frob state is not set to 'false' after shouldering,
			// the body will be stuck in a highlighted state after dropping it
			// until the player highlights the body again. So, ensure 'false'.
			ent->SetFrobbed(false);
		}
		else
		{
			// Unless shouldering a body, make sure nothing is equipped.
			Forget(ent);
		}

		return true;
	}

	return false;
}

bool CGrabber::Equip( void )
{
	idEntity *ent = m_dragEnt.GetEntity();
	return EquipEntity(ent);
}

bool CGrabber::EquipEntity( idEntity *ent )
{
	idStr str;

	if( !ent || !ent->spawnArgs.GetBool("equippable") )
		return false;

	//gameLocal.Printf("Equip called\n");

	// tels: Execute a potential equip script
    if(ent->spawnArgs.GetString("equip_action_script", "", str))
	{
		// Call the script
        idThread* thread = CallScriptFunctionArgs(str.c_str(), true, 0, "e", ent);
		if ( thread != NULL )
		{
			// Run the thread at once, the script result might be needed below.
			thread->Execute();
		}
	}

	// ishtvan: Test general "equip in world system"
	if( ent->spawnArgs.GetBool("equip_in_world") )
	{
		gameLocal.Printf("Equipping item in world\n");

		m_vEquippedPosition = ent->spawnArgs.GetVector("equip_position");
		
		// rotate initially to the desired equipped axis (relative to player)
		idAngles EqAngles = ent->spawnArgs.GetAngles("equip_angles");
		idMat3 EqAxis = EqAngles.ToMat3();
		idVec3 dummy;
		idMat3 viewAxis;
		m_player.GetEntity()->GetViewPos( dummy, viewAxis );
		ent->SetAxis(EqAxis * viewAxis);

		if( ent->spawnArgs.GetBool("equip_draw_on_top") )
			ent->GetRenderEntity()->weaponDepthHack = true;
		if( ent->spawnArgs.GetBool("equip_nonsolid") )
		{
			m_EquippedEntContents = ent->GetPhysics()->GetContents();
			ent->GetPhysics()->SetContents( m_EquippedEntContents & ~(CONTENTS_SOLID | CONTENTS_CORPSE | CONTENTS_RENDERMODEL) );
			m_EquippedEntClipMask = ent->GetPhysics()->GetClipMask();
			ent->GetPhysics()->SetClipMask( m_EquippedEntClipMask & ~(CONTENTS_SOLID | CONTENTS_CORPSE | CONTENTS_RENDERMODEL) );

			// don't limit the force, item always sticks with player when turning
			m_drag.LimitForce( false );
			m_drag.ApplyDamping( false );
		}
		// item is still dragged, but don't allow player controls to rotate or translate
		m_bAllowPlayerRotation = false;
		m_bAllowPlayerTranslation = false;
		m_bMaintainPitch = false;

		m_bEquippedEntInWorld = true;
	}

	// Specific case of shouldering a body
	if (ent->IsType(idAFEntity_Base::Type))
	{
		// greebo: Only shoulderable AF should get "equipped"
		if (ent->spawnArgs.GetBool("shoulderable"))
		{
			{
				idPlayer *player = m_player.GetEntity();
				idPhysics_Player* playerPhysics = player->GetPlayerPhysics();
				if (playerPhysics && playerPhysics->IsMidAir())
				{
					// Cannot shoulder midair
					player->StartSound("snd_drop_item_failed", SND_CHANNEL_ITEM, 0, false, NULL);
					DM_LOG(LC_INVENTORY, LT_INFO)LOGSTRING("Cannot shoulder midair!\r");
					return false;
				}
			}

			m_EquippedEnt = ent;

			StopDrag();

			ShoulderBody(static_cast<idAFEntity_Base*>(ent));

			// greebo: Clear the drag entity, otherwise frobbing interferes
			m_dragEnt = NULL;

			return true; // equipped
		}

		// non-shoulderable AF entity, did not equip this one
		return false; 
	}
	else
	{
		// Non-AF entity, mark it as equipped
		m_EquippedEnt = ent;
		return true;
	}
}

bool CGrabber::Dequip( void )
{
	bool bDequipped(false);
	bool bRunScript(false); // grayman #2624
	idStr str;
	idEntity *ent = m_EquippedEnt.GetEntity();

	if ( !ent )
	{
		return false;
	}

	//gameLocal.Printf("Dequip called\n");

	idPlayer *player = m_player.GetEntity();

	// inventory items go back in inventory on dequipping

	if ( player->AddToInventory(ent) )
	{
		StopDrag();
		bDequipped = true;
	}
	// grayman #2624 - special case for lantern, which does not
	// behave like other non-inventory items. In its case, "dequip"
	// doesn't mean drop, it means toggle the light
	else if ( ent->spawnArgs.GetBool("is_lantern","0") )
	{
		bRunScript = true;
	}
	// junk items and shouldered bodies dequip back to the hands
	// test if item is successfully dequipped
	else if ( player->DropToHands(ent) )
	{
		ent->m_SetInMotionByActor = (idActor *) player; // grayman #3394
		// put in hands (should already be dragged?)
		bDequipped = true;
	}
	else
	{
		// Not dequipped yet
		player->StartSound( "snd_drop_item_failed", SND_CHANNEL_ITEM, 0, false, NULL );
	}

	// tels: Execute a potential dequip script
	if ( ( bDequipped || bRunScript ) && ent->spawnArgs.GetString("dequip_action_script", "", str))
	{ 
		// Call the script
	    idThread* thread = CallScriptFunctionArgs(str.c_str(), true, 0, "e", ent);
		if ( thread != NULL )
		{
			// Run the thread at once, the script result might be needed below.
			thread->Execute();
		}
	}

	// ishtvan: Test general "equip in world system"
	if ( bDequipped && ent->spawnArgs.GetBool("equip_in_world") )
	{
		if ( ent->spawnArgs.GetBool("equip_draw_on_top") )
		{
			ent->GetRenderEntity()->weaponDepthHack = false;
		}

		if ( ent->spawnArgs.GetBool("equip_nonsolid") )
		{
			ent->GetPhysics()->SetContents( m_EquippedEntContents );
			ent->GetPhysics()->SetClipMask( m_EquippedEntClipMask );

			// limit force again if set to do so
			m_drag.LimitForce( cv_drag_limit_force.GetBool() );
			m_drag.ApplyDamping( true );
		}
		// allow player controls to rotate or translate
		m_bAllowPlayerRotation = true;
		m_bAllowPlayerTranslation = true;
		m_bMaintainPitch = true;

		m_bEquippedEntInWorld = false;
	}

	if ( bDequipped )
	{
		// call unshoulderbody if dequipping a body
		if ( ent->IsType(idAFEntity_Base::Type) && ent->spawnArgs.GetBool("shoulderable") )
		{
			// greebo: Pass the entity along, the m_EquippedEnt reference might already be set to NULL at this point
			UnShoulderBody(ent);
		}

		m_EquippedEnt = NULL;
		
		// failsafe, doesn't seem to be reset properly
		m_bEquippedEntInWorld = false;
		m_bMaintainPitch = true;
	}

	return bDequipped;
}

void CGrabber::UseEquipped( void )
{
	idStr str;
	idEntity *ent = GetEquipped();
	
	if ( !ent )
	{
		return;
	}

	gameLocal.Printf("Use equipped called\n");

    if (ent->spawnArgs.GetString("equipped_use_script", "", str))
	{ 
		gameLocal.Printf("Use equipped found script\n");
		// Call the script
        idThread* thread = CallScriptFunctionArgs(str.c_str(), true, 0, "e", ent);
		if (thread != NULL)
		{
			// Run the thread at once, the script result might be needed below.
			thread->Execute();
		}
	}
}

void CGrabber::ShoulderBody( idAFEntity_Base *body )
{
	idPlayer *player = m_player.GetEntity();

	// greebo: Emit the callback to the owner
	player->OnStartShoulderingBody(body);

	// hide the body for now
	//idEntity *ent = m_EquippedEnt.GetEntity();

	body->Unbind();
	body->GetPhysics()->PutToRest();
	body->GetPhysics()->UnlinkClip();
	// Tels: fix #2826 by stopping LOD temporarily on shouldered bodies, including any attachements
	LodComponent::DisableLOD( body, true );
	body->Hide();

	// greebo: prevent this entity from stimming while shouldered
	gameLocal.UnlinkStimEntity(body);

	// Load the animation frame that will put the body in the shouldered pose
	// TODO: Doesn't seem to work currently
	idAnimator *animator = body->GetAnimator();
	int animNum;
	if( (animNum = animator->GetAnim( SHOULDER_ANIM )) != 0 )
	{
		gameLocal.Printf("Found drop_body animation\n");
		animator->ClearAFPose();
		animator->ClearAllAnims(gameLocal.time,0);
		animator->SetFrame(ANIMCHANNEL_ALL, animNum, 0, 0, 0);

		// TODO: Call af.SetPose to move the bodies to match the anim?
	}
}

void CGrabber::UnShoulderBody( idEntity *body )
{
	idPlayer *player = m_player.GetEntity();

	// greebo: Emit the callback to the owner
	player->OnStopShoulderingBody(body);

	// Tels: #2826: Enable LOD again, including attachements
	LodComponent::EnableLOD( body, true );

	// Allow the body to stim again after dropping it
	gameLocal.LinkStimEntity(body);

	// toggle face up/down
	m_bDropBodyFaceUp = !m_bDropBodyFaceUp;
}

void CGrabber::Forget( idEntity* ent )
{
	// The entity in the grabber got removed, so clear everything

	// TODO: This assumes we can never have something equipped and also
	// grab something else, will have to revisit if we change that

    if ( m_dragEnt.GetEntity() == ent || m_EquippedEnt.GetEntity() == ent )
	{
		StopDrag();

		if ( m_EquippedEnt.GetEntity() == ent )
		{
			// Tels: Fix #2430
			m_EquippedEnt = NULL;
		}

		// try to remove from grabber clip list
		if( ent->IsType(idEntity::Type) )
			RemoveFromClipList( ent );
	}
}

/*
Tels: Return the shouldered entity for #3282
*/
idEntity* CGrabber::GetShouldered( void ) const
{
	idEntity* ent = m_EquippedEnt.GetEntity();

	// we can only shoulder AF entities, so if the equipped entity is one of these, it is shouldered
	return (ent && ent->spawnArgs.GetBool("shoulderable") && ent->IsType(idAFEntity_Base::Type)) ? ent : NULL;
}
	
bool CGrabber::IsInSilentMode( void ) const
{
	return m_silentMode;
}
