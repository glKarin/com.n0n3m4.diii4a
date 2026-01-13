
#ifndef __FORCE_H__
#define __FORCE_H__

/*
===============================================================================

	Force base class

	A force object applies a force to a physics object.

===============================================================================
*/

class idEntity;
class idPhysics;

class idForce : public idClass {

public:
	CLASS_PROTOTYPE( idForce );

						idForce( void );
	virtual				~idForce( void );
	static void			DeletePhysics( const idPhysics *phys );
	static void			ClearForceList( void );

public: // common force interface
						// evalulate the force up to the given time
	virtual void		Evaluate( int time );
						// removes any pointers to the physics object
	virtual void		RemovePhysics( const idPhysics *phys );

private:

	static idList<idForce*> forceList;
};

#endif /* !__FORCE_H__ */
