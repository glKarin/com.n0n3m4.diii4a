#ifndef __HH_PROJECTILE_GASBAGPOD_H
#define __HH_PROJECTILE_GASBAGPOD_H

class hhProjectileGasbagPod : public hhProjectile {
	CLASS_PROTOTYPE(hhProjectileGasbagPod);
#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void Event_Collision_SpawnPod(const trace_t* collision, const idVec3 &velocity) {};
	void Event_Collision_Proj(const trace_t* collision, const idVec3 &velocity) {};
#else
public:
	void Spawn(void);
	virtual void Ticker(void);

protected:
	void Event_Collision_SpawnPod(const trace_t* collision, const idVec3 &velocity);
	void Event_Collision_Proj(const trace_t* collision, const idVec3 &velocity);
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif
