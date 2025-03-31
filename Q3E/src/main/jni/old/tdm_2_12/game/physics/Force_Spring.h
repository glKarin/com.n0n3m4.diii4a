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

#ifndef __FORCE_SPRING_H__
#define __FORCE_SPRING_H__

/*
===============================================================================

	Spring force

===============================================================================
*/

class idForce_Spring : public idForce {

public:
	CLASS_PROTOTYPE( idForce_Spring );

						idForce_Spring( void );
	virtual				~idForce_Spring( void ) override;
						// initialize the spring
	void				InitSpring( float Kstretch, float Kcompress, float damping, float restLength );
						// set the entities and positions on these entities the spring is attached to
	void				SetPosition(	idPhysics *physics1, int id1, const idVec3 &p1,
										idPhysics *physics2, int id2, const idVec3 &p2 );

public: // common force interface
	virtual void		Evaluate( int time ) override;
	virtual void		RemovePhysics( const idPhysics *phys ) override;

private:

	// spring properties
	float				Kstretch;
	float				Kcompress;
	float				damping;
	float				restLength;

	// positioning
	idPhysics *			physics1;	// first physics object
	int					id1;		// clip model id of first physics object
	idVec3				p1;			// position on clip model
	idPhysics *			physics2;	// second physics object
	int					id2;		// clip model id of second physics object
	idVec3				p2;			// position on clip model

};

#endif /* !__FORCE_SPRING_H__ */
