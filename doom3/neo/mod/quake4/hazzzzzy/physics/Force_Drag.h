
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
	virtual				~idForce_Drag( void );
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
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const idPhysics *phys );

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
