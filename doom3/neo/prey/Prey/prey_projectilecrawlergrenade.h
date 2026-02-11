#ifndef __HH_PROJECTILE_CRAWLER_GRENADE_H
#define __HH_PROJECTILE_CRAWLER_GRENADE_H

extern const idEventDef EV_ApplyExpandWound;

/***********************************************************************

  hhProjectileCrawlerGrenade
	
***********************************************************************/
class hhProjectileCrawlerGrenade : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileCrawlerGrenade );

	public:
		void					Spawn();
		virtual					~hhProjectileCrawlerGrenade();

		virtual void			Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

		virtual void			Hide();
		virtual void			Show();
		virtual void			RemoveProjectile( const int removeDelay );

		void					Save( idSaveGame *savefile ) const;
		void					Restore( idRestoreGame *savefile );

		//rww - networking
		virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	protected:
		virtual void			Ticker();

		virtual void			SpawnFlyFx();
		virtual void			CopyToModelProxy();
		virtual void			SpawnModelProxy();
		virtual void			InitCollisionInfo();

	protected:
		void					Event_ApplyExpandWound();

		void					Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity );
		void					Event_Collision_DisturbLiquid( const trace_t* collision, const idVec3 &velocity );
		void					EnterDyingState();
		void					EnterDeadState();
		void					CancelActivates();

	protected:
		enum States {
			StateAlive = 0,
			StateDying,
			StateDead
		} state;

		idInterpolate<float>	modelScale;
		int						inflateDuration;

		trace_t					collisionInfo;

		idEntityPtr<hhGenericAnimatedPart> modelProxy;

		bool					modelProxyCopyDone;
};

/***********************************************************************

  hhProjectileStickyCrawlerGrenade
	
***********************************************************************/
class hhProjectileStickyCrawlerGrenade : public hhProjectileCrawlerGrenade {
	CLASS_PROTOTYPE( hhProjectileStickyCrawlerGrenade );

	public:
		int				ProcessCollision( const trace_t* collision, const idVec3& velocity );
		idMat3			DetermineCollisionAxis( const idMat3& collisionAxis );
		void			Event_Activate( idEntity *pActivator );

	protected:
		virtual void	BindToCollisionObject( const trace_t* collision );
		void			Event_Collision_Stick( const trace_t* collision, const idVec3 &velocity );
		virtual void	Explode( const trace_t* collision, const idVec3& velocity, int removeDelay );

		idEntityPtr<idEntity>	proximityDetonateTrigger;
};

#endif