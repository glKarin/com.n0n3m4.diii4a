#ifndef __HH_PROJECTILE_ROCKET_LAUNCHER_H
#define __HH_PROJECTILE_ROCKET_LAUNCHER_H

/***********************************************************************

  hhProjectileRocketLauncher
	
***********************************************************************/
class hhProjectileRocketLauncher : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileRocketLauncher );

	public:
		void			Spawn();
		virtual			~hhProjectileRocketLauncher();

		virtual void	Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

		virtual void	Hide();
		virtual void	Show();
		virtual void	RemoveProjectile( const int removeDelay );

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

		virtual void	ClientPredictionThink( void );

	protected:
		void			Event_SpawnModelProxyLocal();
		void			Event_SpawnFxFlyLocal( const char* defName );

		void			Event_AllowCollision_Collide( const trace_t* collision );

	protected:
		idEntityPtr<hhGenericAnimatedPart> modelProxy;
};

/***********************************************************************

  hhProjectileChaff
	
***********************************************************************/
class hhProjectileChaff : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileChaff );

	public:
		void			Spawn();

		virtual void	Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
		virtual bool	Collide( const trace_t& collision, const idVec3& velocity );

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

	protected:
		virtual void	Ticker();

		virtual int		DetermineContents();
		virtual int		DetermineClipmask();

	protected:
		void			Event_Touch( idEntity *other, trace_t *trace );

	protected:
		int				decelStart;
		int				decelEnd;

		idVec3			cachedVelocity;
};

#endif