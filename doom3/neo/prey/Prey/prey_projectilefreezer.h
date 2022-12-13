#ifndef __HH_PROJECTILE_FREEZER_H
#define __HH_PROJECTILE_FREEZER_H

class hhProjectileFreezer : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileFreezer );

	public:
		void			Spawn();

		virtual void	Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

		virtual bool	Collide( const trace_t& collision, const idVec3& velocity );
		virtual int		ProcessCollision( const trace_t* collision, const idVec3& velocity );

	protected:
		virtual void	Ticker();

	protected:
		void			Event_Touch( idEntity *other, trace_t *trace );
		void			Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity );

		void			ApplyDamageEffect( idEntity* hitEnt, const trace_t* collision, const idVec3& velocity, const char* damageDefName );

	protected:
		int				decelStart;
		int				decelEnd;

		idVec3			cachedVelocity;
		bool			collided;
};

#endif