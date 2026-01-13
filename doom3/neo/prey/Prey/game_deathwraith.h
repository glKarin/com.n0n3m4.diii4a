#ifndef __GAME_DEATHWRAITH_H__
#define __GAME_DEATHWRAITH_H__

class hhDeathWraith : public hhWraith {
	public:
		CLASS_PROTOTYPE( hhDeathWraith );

		void 			Spawn();
		virtual void	FlyToEnemy();
		virtual void	SetEnemy( idActor *newEnemy ) { enemy = newEnemy; }
		virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
		virtual void	Think( void );

	protected:
		virtual int		PlayAnim( const char *name, int blendFrame );
		virtual void	PlayCycle( const char *name, int blendFrame );
		virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
		virtual void	NotifyDeath( idEntity *inflictor, idEntity *attacker );
		void			SpawnEnergy(hhPlayer *player);
		void			EnterAttackState();
		void			ExitAttackState();
		virtual bool	ChargeEnemy();
		bool			CheckEnemy();
		void			HitEnemy();
		void			ApplyImpulseToEnemy();
		virtual void	FlyUp();
		virtual void	FlyMove( void );
		virtual void	EnemyDead();
		void			Event_FindEnemy( int useFOV );
		void			TeleportIn( idEntity *activator );

	protected:
		// Variables for controlling the movement of the wraith around the anchor point
		// These variables are unique for each wraith for variety
		float			circleDist;			// Distance to stay from the anchor point
		float			circleSpeed;		// Orbit speed
		float			circleHeight;		// Height above the point
		bool			circleClockwise;	// if true, orbit clockwise

		int				attackTimeMin;		// Range for the attack time
		int				attackTimeDelta;	// Range for the attack time

		idMat3			chargeAxis;

		bool			healthWraith;	// true if a health type
};

#endif