
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
	void				SetPosition(	idPhysics *physics1, int id1, const idVec3 &p1,
										idPhysics *physics2, int id2, const idVec3 &p2 );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const idPhysics *phys );

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
