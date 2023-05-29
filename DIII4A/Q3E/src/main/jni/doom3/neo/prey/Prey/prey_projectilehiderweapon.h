#ifndef __HH_PROJECTILE_HIDER_WEAPON_H
#define __HH_PROJECTILE_HIDER_WEAPON_H

/***********************************************************************

  hhProjectileHider
	
***********************************************************************/
class hhProjectileHider : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileHider );

	protected:
		virtual void	ApplyDamageEffect( idEntity* entHit, const trace_t& collision, const idVec3& velocity, const char* damageDefName );
};

/***********************************************************************

  hhProjectileHiderCanister
	
***********************************************************************/
class hhProjectileHiderCanister : public hhProjectileHider {
	CLASS_PROTOTYPE( hhProjectileHiderCanister );

	public:
		void			Spawn();
		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

	protected:
		virtual void	SpawnRicochetSpray( const idVec3& bounceVelocity );
		virtual void	SpawnDebris( const idVec3& collisionNormal, const idVec3& collisionDir );
		virtual void	Killed( idEntity *inflicter, idEntity *attacker, int damage, const idVec3 &dir, int location );

		void			Event_Collision_Explode( const trace_t* collision, const idVec3& velocity );
		void			Event_Collision_ExplodeChaff( const trace_t* collision, const idVec3& velocity );

	protected:
		int				numSubProjectiles;
		idDict			subProjectileDict;
		float			subSpread;
		float			subBounce;
		bool			bScatter;
};

#endif
