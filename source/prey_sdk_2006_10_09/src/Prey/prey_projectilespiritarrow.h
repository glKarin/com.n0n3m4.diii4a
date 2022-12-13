#ifndef __HH_PROJECTILE_SPIRITARROW_H
#define __HH_PROJECTILE_SPIRITARROW_H

/***********************************************************************

  hhProjectileSpiritArrow
	
***********************************************************************/
class hhProjectileSpiritArrow: public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileSpiritArrow );

	protected:
		virtual	int	DetermineClipmask();
		void		BindToCollisionObject( const trace_t* collision );

	protected:
		void		Event_Collision_Stick( const trace_t* collision, const idVec3 &velocity );
};

#endif