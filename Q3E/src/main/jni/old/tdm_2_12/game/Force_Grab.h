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

#ifndef __FORCE_GRAB_H__
#define __FORCE_GRAB_H__

#include "physics/Force.h"

/*
===============================================================================

	Grab force

===============================================================================
*/

class CForce_Grab : public idForce 
{
	public:
		CLASS_PROTOTYPE( CForce_Grab );

							CForce_Grab( void );
		virtual				~CForce_Grab( void ) override;
							// initialize the drag force
		void				Init( float damping );
							// set physics object being dragged
		void				SetPhysics( idPhysics *physics, int id, const idVec3 &p );
							// set position to drag towards
		void				SetDragPosition( const idVec3 &pos );
							// get the position dragged towards
		const idVec3 &		GetDragPosition( void ) const;
							// get the position on the dragged physics object
		/**
		* Set/get orientation to rotate toward
		**/
		void				SetDragAxis( const idMat3 &Axis );
		idMat3				GetDragAxis( void );

		idVec3				GetDraggedPosition( void ) const;
							// Gets the center of mass of the grabbed object
		idVec3				GetCenterOfMass( void ) const;
							// rotates p about the center of mass of the grabbed object
		void				Rotate( const idVec3 &vec, float angle );

		void				Save( idSaveGame *savefile ) const;
		void				Restore( idRestoreGame *savefile );

		/**
		* Toggle whether the force_grab should use a max force limit
		**/
		void				LimitForce( bool bVal );

		/**
		* Toggle whether the force_grab should apply damping or not
		**/
		void				ApplyDamping( bool bVal );

		/**
		* Set the reference entity
		**/
		void				SetRefEnt( idEntity *ent );

	public: // common force interface
		virtual void		Evaluate( int time ) override;
		virtual void		RemovePhysics( const idPhysics *phys ) override;

	protected:
		// stgatilov #5599: apply aging to frames, throw away obsolete ones, add one new frame
		void				UpdateAverageDragPosition( float dT );
		// stgatilov #5599: compute weighted moving average of drag position
		idVec3				ComputeAverageDragPosition() const;
		// stgatilov #5599: set or restore friction parameters to dragged object
		void				SetFrictionOverride(bool enabled, float linear, float angular, float contact);

		// entity to which this drag force is referenced, if any
		idEntityPtr<idEntity>	m_RefEnt;

		// if true, limit force or apply damping
		bool				m_bLimitForce;
		bool				m_bApplyDamping;

		// properties
		float				m_damping;

		// physics object properties
		idVec3				m_centerOfMass;
		idMat3				m_inertiaTensor;

		// positioning
		idPhysics *			m_physics;		// physics object
		idVec3				m_p;				// position on clip model
		int					m_id;				// clip model id of physics object
		// drag towards this position
		idVec3				m_dragPosition;
		// rotate toward this orientation
		idMat3				m_dragAxis;

		// stgatilov #5599: sorted array of (dragPosition; weight) tuples over time window
		idList<idVec4>		m_dragPositionFrames;
		// stgatilov #5599: original friction parameters of dragged object (see SetFrictionOverride)
		idList<idVec3>		m_originalFriction;
};

#endif /* !__FORCE_GRAB_H__ */
