#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#ifndef __HH_PROJECTILE_BUG_H
#define __HH_PROJECTILE_BUG_H

class hhProjectileBug: public hhProjectileTracking {
public:
	CLASS_PROTOTYPE( hhProjectileBug )
	idEntity* DetermineEnemy();
	void Event_TrackTarget();
	void Spawn();
	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );
	idVec3 DetermineEnemyPosition( const idEntity* ent ) const;
	void	Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity );
protected:
	float enemyRadius;
};

#endif
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build