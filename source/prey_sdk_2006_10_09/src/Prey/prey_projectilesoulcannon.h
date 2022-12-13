#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#ifndef __HH_PROJECTILE_SOUL_CANNON_H
#define __HH_PROJECTILE_SOUL_CANNON_H

/***********************************************************************

  hhProjectileSoulCannon
	
***********************************************************************/

class hhProjectileSoulCannon : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileSoulCannon );

	public:
		void			Spawn();
		void			Think( void );

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

	protected:
		float			maxVelocity;
		float			maxEnemyDist;
		idVec3			thrustDir;

		void			hhProjectileSoulCannon::Event_FindEnemy( void );
};

#endif
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build