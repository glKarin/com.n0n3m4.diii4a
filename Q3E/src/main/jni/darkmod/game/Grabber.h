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

#ifndef __GRABBER_H__
#define __GRABBER_H__

#include "Entity.h"
#include "Force_Grab.h"

class idPlayer;

extern const idEventDef EV_Grabber_CheckClipList;

class CGrabbedEnt 
{
	public: 
		idEntityPtr<idEntity>	m_ent;
		int			m_clipMask;
		int			m_contents;

		bool operator==( const CGrabbedEnt &right ) const 
		{
			return right.m_ent.GetEntity() == m_ent.GetEntity();
		}
};

class CGrabber : public idEntity {
public:
		CLASS_PROTOTYPE( CGrabber );


								CGrabber( void );
		virtual					~CGrabber( void ) override;

		void					Clear( void );

		/**
		* Update() isn't just the main update loop. It's the point of entry that 
		* grabs an item that the player has frobbed. preservePosition added SteveL #4149
		**/
		void					Update( idPlayer *player, bool hold = false, bool preservePosition = false );

		void					Save( idSaveGame *savefile ) const;
		void					Restore( idRestoreGame *savefile );


		void					Spawn( void );

		idEntity *				GetSelected( void ) const { return m_dragEnt.GetEntity(); }

		idEntity *				GetEquipped( void ) const { return m_EquippedEnt.GetEntity(); }

		/**
		* Tels: Return the shouldered entity for #3282
		**/
		idEntity*				GetShouldered( void ) const;
	
		bool					IsInClipList( idEntity *ent ) const;
		bool					HasClippedEntity( void ) const;

		/**
		 * Cycles through the cliplist and removes the given entity.
		 *
		 * greebo: I added this to prevent deleted entities (after beind added to 
		 *         the inventory, for instance) from being checked and causing segfaults.
		 */
		void					RemoveFromClipList(idEntity* entity);

		/**
		* Clamp the current velocity to max velocity
		**/
		void					ClampVelocity( float maxLin, float maxAng, int idVal = 0 );

		/**
		* Increment the distance at which the held item is held.
		* Decrements the distance if the argument is false
		**/
		void					IncrementDistance( bool bIncrease );

		/**
		* Attempts to teleport an entity to the minimum hold distance and start holding it
		* Returns false and does nothing if there was not space to teleport in the entity
		* Body ID may be optionally set to specify which body to hold for AF or multi-clipmodel entities
		*
		* NOTE: Rotation of the entity to the desired orientation should be done externally
		* before calling this.
		**/
		bool					PutInHands( idEntity *ent, idMat3 axis = mat3_identity, int bodyID = 0 );
		/**
		* We can specify a different point to start the item at other than the default hold point
		**/
		bool					PutInHands( idEntity *ent, idVec3 point, idMat3 axis = mat3_identity, int bodyID = 0 );
		/**
		* And this is the internal function called by both versions of PutInHands
		**/
		bool					PutInHandsAtPoint( idEntity *ent, idVec3 point, idMat3 axis = mat3_identity, int bodyID = 0 );

		/**
		* Returns true if there is space to put an item in the player's hands at the minimum hold distance
		* Otherwise returns false.
		* Body ID may be optionally set to specify which body to hold for AF or multi-clipmodel entities
		**/
		bool					FitsInHands( idEntity *ent, idMat3 axis = mat3_identity, int bodyID = 0 );

		/**
		* Returns true if there is space to put an item at a particular point in space
		* Also checks the space between the viewPoint (player's eyes) and this point for obstructions,
		* so that the player cannot drop things to the other side of solid walls or tiny openings.
		* Body ID may be optionally set to specify which body to hold for AF or multi-clipmodel entities
		**/
		bool					FitsInWorld( idEntity *ent, idVec3 viewPoint, idVec3 point, idMat3 axis = mat3_identity, int bodyID = 0 );

		/**
		* Returns the point in world coordinates to move the entity center of mass to
		* when we are going to put this item into the player's hands.
		* Axis specifies the orientation of the object
		* bodyID optionally specifies which AF body should be held for AFs
		* (e.g., the handle of a bucket)
		**/
		idVec3					GetHoldPoint( idEntity *ent, idMat3 axis = mat3_identity, int bodyID = 0 );

		/**
		 * Restores the physics object from the drag entity after loading.
		 */
		void					SetPhysicsFromDragEntity();

		/**
		* Returns true if the item held by the grabber is stuck
		**/
		bool					ObjStuck( void );

		/**
		* Try to equip/dequip a junk item
		* Returns true if the current item was equipped or dequipped
		**/
		bool					ToggleEquip( void );
		/**
		* Daft Mugi #6316
		* Try to Use/Equip frob-highlighted item without holding it.
		* Examples: bodies, candles, lanterns, and food
		**/
		bool					EquipFrobEntity( idPlayer *player );
		/**
		* Actual functions for equipping and dequipping
		**/
		bool					Equip( void );
		bool					EquipEntity( idEntity *ent );
		bool					Dequip( void );

		/**
		* Call a use script on the currently equipped item
		**/
		void					UseEquipped( void );

		/**
		* Updates immobilizations, etc when we shoulder or unshoulder a body
		* ShoulderBody is only called after body is found to be shoulderable
		* UnShoulderBody is only called after we check if there's room
		**/
		void					ShoulderBody( idAFEntity_Base *body );
		void					UnShoulderBody( idEntity *body );

		/**
		* Stop dragging and drop the current item if there is one
		**/
		void					StopDrag( void );

		/**
		* Forget the currently dragged/equipped entity (in case it is to be removed
		* from the world.
		**/
		void					Forget( idEntity* ent );

		bool					IsInSilentMode( void ) const;

		/**
		* Used to switch between dropping a body face up and face down on each drop
		* (Toggled by idPlayer)
		**/
		bool					m_bDropBodyFaceUp;

		/**
		* Set the player associated with this grabber
		**/
		void					SetPlayer( idPlayer *player ) { m_player = player; };

		/**
		* Set to true if the grabbed entity is colliding this frame
		**/
		bool					m_bIsColliding;

		/**
		* Set to true if the grabbed entity collided in the previous frame
		**/
		bool					m_bPrevFrameCollided;

		/**
		* List of collision normals (shouldn't need this, should be able to use
		* GetContacts, but that is not working)
		**/
		idList<idVec3>			m_CollNorms;

		/**
		* Stores the max force the player can apply in [mass] * doomunits/sec^2
		* Currently only effects linear motion, not angular
		**/
		float					m_MaxForce;

protected:

		/**
		* Start grabbing an item.  Called internally.
		* 
		* If newEnt argument is NULL, it tries to grab the entity the player is frobbing
		* Otherwise it places newEnt in the hands.
		*
		* Also calls StopDrag to drop the current item before grabbing the new one, 
		* but we may need to put a time delay between so that we don't have 
		* Pauli Exclusion issues.
		*
		* preservePosition added in #4149. If true, the item won't immediately snap to 
		* one of the allowable positions calculated from m_DistanceCount. 
		**/
		void					StartDrag( idPlayer *player, idEntity *newEnt = NULL, int bodyID = 0, bool preservePosition = false );

		/**
		* Set encumbrance on the player as a function of the dragged object's mass
		* May also set jump encumbrance (no jumping)
		* Assumes m_player and m_dragEnt are already set
		**/
		void					SetDragEncumbrance( void );

		/**
		* Performs object rotation
		* Also turns the item to face the player when they yaw their view.
		**/
		void					ManipulateObject( idPlayer *player );
		
		void					AddToClipList( idEntity *ent );
		void					RemoveFromClipList( int index );

		void					Event_CheckClipList( void );

		/**
		* Throws the current item.
		* Argument is the amount of time the throw button has been held,
		* used to determine strength of the throw
		**/
		void					Throw( int HeldTime );
		
		/**
		*  returns true if the mouse is inside the dead zone
		**/
		bool					DeadMouse( void );

		/**
		* Checks for a stuck object and updates m_bObjStuck.  
		* Stuck means too far away from the drag point.
		**/
		void					CheckStuck( void );

protected:
		/**
		* Entity being dragged
		**/
		idEntityPtr<idEntity>	m_dragEnt;
		jointHandle_t			m_joint;				// joint being dragged
		int						m_id;					// id of AF body being dragged
		/**
		* Grabbed point on the entity (in entity space)
		**/
		idVec3					m_LocalEntPoint;
		/**
		* Offset between object center of mass and dragged point
		* NYI
		**/
		idVec3					m_vLocalEntOffset;
		/**
		* Offset vector between player view center and hold position
		**/
		idVec3					m_vOffset; 
		/**
		* If true, the item does not pitch up and down when the player pitches their view
		*/
		bool					m_bMaintainPitch; 

		idEntityPtr<idPlayer>	m_player;
		CForce_Grab				m_drag;

		idRotation				m_rotation;
		int						m_rotationAxis;		// 0 = none, 1 = x, 2 = y, 3 = z
		idVec2					m_mousePosition;		// mouse position when user pressed BUTTON_ZOOM

		/**
		* Allow the player to rotate objects by pressing block/manipulate, and translate
		**/
		bool					m_bAllowPlayerRotation;
		bool					m_bAllowPlayerTranslation;

		/**
		* List of entities that were grabbed and that
		* we must keep nonsolid as long as they are clipping the player
		**/
		idList<CGrabbedEnt>		m_clipList;

		/**
		* Set to true if the attack button has been pressed (used by throwing)
		**/
		bool					m_bAttackPressed;
		/**
		* Timestamp of when attack button was last pressed (used by throwing)
		**/
		int						m_ThrowTimer;

		/**
		* Time stamp in milliseconds for body drag vertical velocity clamp timer
		* (Actual time is read from cvar)
		* Velocity ramps up to normal the longer the body maintains ground contact
		* Resets when they lose ground contact.
		**/
		int						m_DragUpTimer;

		/**
		* Used to limit lifting dragged bodies off ground
		**/
		float					m_AFBodyLastZ;

		/**
		* Set to true when the body with ground checking is considered off the ground
		**/
		bool					m_bAFOffGround;

		/**
		* Int storing the distance increments for held item distance.
		* When this is equal to m_MaxDistCount, it is held at the maximum
		* distance (the frob distance).
		**/
		int						m_DistanceCount;

		/**
		* Maximum distance increments at which the item can be held.
		* Corresponds to 1.0 * the max distance.
		**/
		int						m_MaxDistCount;

		/**
		* Minimum held distance (can be unique to each entity)
		* NOTE: The maximum held distance is always the frob distance of that ent
		**/
		int						m_MinHeldDist;

		/**
		* When colliding, the held distance gets locked to +/- two increments
		* around this value.  This is to prevent the player from going way
		* past the collision point and having to increment all the way back
		* before they see any response.
		**/
		int						m_LockedHeldDist;

		
		/**		
		* A fixed, exact, floating point position for the held item, relative to the player's view. 
		* This is for items that have just been picked up. We don't start to use m_DistanceCount
		* or apply the min distance until the player actively changes the distance or starts to 
		* rotate the object. This is to avoid clunking.
		* m_StoppingPreserving is a flag that lets the object glide smoothly to the target position
		* by gradually converging m_PreservedPosition with the target central position. 
		* m_PreservedPosition.x is the distance from the eyes, so will always be > 0 when in use
		* and we can use it as a flag. SteveL #4149
		**/
		idVec3					m_PreservedPosition;
		bool					m_StoppingPreserving;
		bool					PreservingPosition() { return m_PreservedPosition.x > 0.0f; }
		
		/**
		* Set to true if the object held by the grabber is "stuck"
		* Stuck in this context means too far away from the grab point
		**/
		bool					m_bObjStuck;

		/**
		* "Equipped" object.
		* This can mean different things for different objects
		* For AI bodies, it means toggling between shouldering the AI
		* and dragging the AI.
		**/
		idEntityPtr<idEntity>	m_EquippedEnt;

		/**
		* True if the equipped ent is still dragged around the world by the Grabber
		**/
		bool					m_bEquippedEntInWorld;

		/**
		* Position of the equipped item in player view coordinates
		**/
		idVec3					m_vEquippedPosition;

		/**
		* Stored contents and clipmask of the equipped entity
		* In case we change it while equipped
		**/
		int						m_EquippedEntContents;
		int						m_EquippedEntClipMask;

		/**
		* stgatilov #5599: If silent mode is enabled, then we do not push other entities and do not make noise collision sounds.
		* The mode is enabled in new grabber (cv_drag_new) depending on cv_drag_rigid_silentmode cvar.
		**/
		bool					m_silentMode;

};


#endif /* !__GRABBER_H__ */
