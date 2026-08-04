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

#ifndef __FORCE_DRAG_H__
#define __FORCE_DRAG_H__

/*
===============================================================================

	Drag force

===============================================================================
*/

class idForce_Drag : public idForce {

public:
	CLASS_PROTOTYPE( idForce_Drag );

						idForce_Drag( void );
	virtual				~idForce_Drag( void ) override;
						// initialize the drag force
	void				Init( float damping );
						// set physics object being dragged
	void				SetPhysics( idPhysics *physics, int id, const idVec3 &p );
						// set position to drag towards
	void				SetDragPosition( const idVec3 &pos );
						// get the position dragged towards
	const idVec3 &		GetDragPosition( void ) const;
						// get the position on the dragged physics object
	const idVec3		GetDraggedPosition( void ) const;

public: // common force interface
	virtual void		Evaluate( int time ) override;
	virtual void		RemovePhysics( const idPhysics *phys ) override;

private:

	// properties
	float				damping;

	// positioning
	idPhysics *			physics;		// physics object
	int					id;				// clip model id of physics object
	idVec3				p;				// position on clip model
	idVec3				dragPosition;	// drag towards this position
};

#endif /* !__FORCE_DRAG_H__ */
