#ifndef __HH_PROJECTILE_TRACKING_H
#define __HH_PROJECTILE_TRACKING_H

/***********************************************************************

  hhProjectileTracking
	
***********************************************************************/
class hhProjectileTracking: public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileTracking );

	public:
		hhProjectileTracking();
		void			Spawn();
		virtual void	Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
		virtual void	Explode( const trace_t *collision, const idVec3& velocity, int removeDelay );

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );
		void			StartTracking();
		void			SetEnemy( idEntity* newEnemy ) { enemy = newEnemy; }
	protected:
		virtual idEntity* DetermineEnemy();
		bool			EnemyIsValid( idEntity* actor ) const;
		idEntity*		WhosClosest( idEntity* possibleEnemy, idEntity* currentEnemy, float& currentEnemyDot, float& currentEnemyDist ) const;
		idVec3			DetermineEnemyPosition( const idEntity* enemy ) const;
		idVec3			DetermineEnemyDir( const idEntity* enemy ) const;
		bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	protected:
		void			Event_Hover();
		virtual void	Event_TrackTarget();
		void			Event_StartTracking();
		void			Event_StopTracking();

	protected:
		float			turnFactor;
		int				updateRate;

		idEntityPtr<idEntity> enemy;
		idAngles		angularVelocity;
		idVec3			velocity;
		float			cachedFovCos;
		float			spinAngle;
		float			turnFactorAcc;
		float			spinDelta;
};

#endif