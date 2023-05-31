#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#ifndef __HH_PROJECTILE_BOUNCE_H
#define __HH_PROJECTILE_BOUNCE_H

class hhProjectileBounce : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileBounce );
	void Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity );
};

#endif
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build