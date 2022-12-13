#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#ifndef __HH_PROJECTILE_COCOON_H
#define __HH_PROJECTILE_COCOON_H

class hhProjectileCocoon: public hhProjectileTracking {
public:
	CLASS_PROTOTYPE( hhProjectileCocoon )
	void Spawn();
	void Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
protected:
	void Event_Collision_Proj( const trace_t* collision, const idVec3 &velocity );
	void Event_TrackTarget();
	void Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity );
	void Event_AllowCollision( const trace_t* collision );
	int nextBounceTime;
};

#endif
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build