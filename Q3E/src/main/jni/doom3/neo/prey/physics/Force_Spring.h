// Copyright (C) 2004 Id Software, Inc.
//

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
	virtual				~idForce_Spring( void );
						// initialize the spring
	void				InitSpring( float Kstretch, float Kcompress, float damping, float restLength );
						// set the entities and positions on these entities the spring is attached to
	void				SetPosition(	idEntity *physics1, int id1, const idVec3 &p1,
										idEntity *physics2, int id2, const idVec3 &p2 ); // HUMANHEAD mdl:  Changed to taking entities instead of their physics objects

	// HUMANHEAD mdl:  Added Save/Restore
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	// HUMANHEAD END

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const idEntity *phys ); // HUMANHEAD mdl:  Changed to take entity instead of its physics object

private:

	// spring properties
	float				Kstretch;
	float				Kcompress;
	float				damping;
	float				restLength;

	// positioning
	idEntityPtr< idEntity > physics1;	// first physics object // HUMANHEAD mdl:  Changed to entity
	int					id1;		// clip model id of first physics object
	idVec3				p1;			// position on clip model
	idEntityPtr< idEntity > physics2;	// second physics object // HUMANHEAD mdl:  Changed to entity
	int					id2;		// clip model id of second physics object
	idVec3				p2;			// position on clip model

};

#endif /* !__FORCE_SPRING_H__ */
