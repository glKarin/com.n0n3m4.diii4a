#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#ifndef __HH_PROJECTILE_BUGTRIGGER_H
#define __HH_PROJECTILE_BUGTRIGGER_H

class hhProjectileBugTrigger: public hhProjectile {
public:
	CLASS_PROTOTYPE( hhProjectileBugTrigger )
	void Event_Collision_Impact( const trace_t* collision, const idVec3& velocity );
	void Event_Touch( idEntity *other, trace_t *trace );
};

#endif
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build