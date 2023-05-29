#ifndef __HH_PROJECTILE_WRENCH_H
#define __HH_PROJECTILE_WRENCH_H

class hhProjectileWrench : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileWrench );
		void					Event_Collision_Impact_AlertAI( const trace_t* collision, const idVec3& velocity );
	protected:
		void					DamageEntityHit( const trace_t* collision, const idVec3& velocity, idEntity* entHit );
};

#endif