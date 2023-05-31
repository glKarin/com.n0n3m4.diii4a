#ifndef __HH_PROJECTILE_MINE_H
#define __HH_PROJECTILE_MINE_H

/***********************************************************************

  hhProjectileMine
	
***********************************************************************/
class hhHarvesterMine : public hhRenderEntity {
	CLASS_PROTOTYPE( hhHarvesterMine );

	public:
								hhHarvesterMine();
		virtual					~hhHarvesterMine();
		void					Spawn();

		virtual void			Create( idEntity *owner, const idVec3 &start, const idMat3 &axis );
		virtual void			Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity );
		virtual bool			Collide( const trace_t& collision, const idVec3& velocity );

		virtual void			SetGravity( const idVec3 &newGravity );

	protected:
		virtual void			SpawnDebris( const idVec3& collisionNormal, const idVec3& collisionDir );
		virtual void			RemoveProjectile( const int removeDelay );
		virtual idEntity*		DetermineClipModelOwner();
		virtual void			SpawnCollisionFX( const trace_t* collision, const char* fxKey );

		virtual void			Explode( const trace_t *collision );
		virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

		virtual int				DetermineContents();
		virtual void			InitPhysics( const idVec3& start, const idMat3& axis, const idVec3& pushVelocity );
		void					SpawnTriggers();

		void							Save( idSaveGame *savefile ) const;
		void							Restore( idRestoreGame *savefile );

	protected:
		void					Event_Detonate( idEntity *activator );
		void					Event_ApplyAttractionTowards( idEntity *activator );
		void					Event_Explode();

	protected:
		idEntityPtr<idEntity>	proximityDetonateTrigger;
        idEntityPtr<idEntity>	proximityAttractionTrigger;

		hhPhysics_RigidBodySimple physicsObj;

		idEntityPtr<idEntity>	owner;

		renderLight_t			renderLight;
		//qhandle_t				lightDefHandle;				// handle to renderer light def
		idVec3					lightOffset;
		int						lightStartTime;
		int						lightEndTime;
		idVec3					lightColor;

		typedef enum {
			SPAWNED = 0,
			CREATED = 1,
			LAUNCHED = 2,
			FIZZLED = 3,
			EXPLODED = 4
		} mineState_t;
		
		mineState_t		state;
};

#endif