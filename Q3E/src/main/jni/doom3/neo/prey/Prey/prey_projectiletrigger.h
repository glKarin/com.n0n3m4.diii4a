#ifndef __HH_PROJECTILE_TRIGGER_H
#define __HH_PROJECTILE_TRIGGER_H

class hhProjectileTrigger: public hhProjectile {
public:
	CLASS_PROTOTYPE( hhProjectileTrigger )
	void Event_Collision_Explode( const trace_t* collision, const idVec3& velocity );
};

#endif