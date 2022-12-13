#ifndef __PREY_PROJECTILE_H__
#define __PREY_PROJECTILE_H__

class hhBeamSystem;

extern const idEventDef EV_SpawnDriverLocal;
extern const idEventDef EV_SpawnFxFlyLocal;

extern const idEventDef EV_Collision_Flesh;
extern const idEventDef EV_Collision_Metal;
extern const idEventDef EV_Collision_AltMetal;
extern const idEventDef EV_Collision_Wood;
extern const idEventDef EV_Collision_Stone;
extern const idEventDef EV_Collision_Glass;
extern const idEventDef EV_Collision_Liquid;
extern const idEventDef EV_Collision_Spirit;
extern const idEventDef EV_Collision_Remove;
extern const idEventDef EV_Collision_CardBoard;
extern const idEventDef EV_Collision_Tile;
extern const idEventDef EV_Collision_Forcefield;
extern const idEventDef EV_Collision_Wallwalk;
extern const idEventDef EV_Collision_Chaff;
extern const idEventDef EV_Collision_Pipe;


extern const idEventDef EV_AllowCollision_Flesh;
extern const idEventDef EV_AllowCollision_Metal;
extern const idEventDef EV_AllowCollision_AltMetal;
extern const idEventDef EV_AllowCollision_Wood;
extern const idEventDef EV_AllowCollision_Stone;
extern const idEventDef EV_AllowCollision_Glass;
extern const idEventDef EV_AllowCollision_Liquid;
extern const idEventDef EV_AllowCollision_Spirit;
extern const idEventDef EV_AllowCollision_CardBoard;
extern const idEventDef EV_AllowCollision_Tile;
extern const idEventDef EV_AllowCollision_Forcefield;
extern const idEventDef EV_AllowCollision_Wallwalk;
extern const idEventDef EV_AllowCollision_Chaff;
extern const idEventDef EV_AllowCollision_Pipe;


/***********************************************************************

  hhProjectile
	
***********************************************************************/
class hhPortal; // cjr

class hhProjectile : public idProjectile {
	CLASS_PROTOTYPE( hhProjectile );

	public:
		void					Spawn();
		virtual					~hhProjectile();
		virtual void			Create( idEntity *owner, const idVec3 &start, const idVec3 &dir );
		virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

		virtual void			Create( idEntity *owner, const idVec3 &start, const idMat3 &axis );
		virtual void			Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

		virtual void			SetOrigin( const idVec3& origin );
        virtual void			SetAxis( const idMat3& axis );

		virtual void			Think();

		virtual void			RemoveProjectile( const int removeDelay );
		virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
		virtual bool			ProcessCollisionEvent( const trace_t* collision, const idVec3& velocity );
		virtual void			Explode( const trace_t* collision, const idVec3& velocity, int removeDelay );
		virtual void			SplashDamage( const idVec3& origin, idEntity* attacker, idEntity* ignoreDamage, idEntity* ignorePush, const char* splashDamageDefName );
		virtual void			BounceSplat( const idVec3& origin, const idVec3& dir );
		virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
		virtual void			Fizzle( void );

		virtual void			SetGravity( const idVec3 &newGravity );
		virtual const idVec3&	GetGravity() const { return idEntity::GetGravity(); }

		virtual bool			GetDDACounted() const { return bDDACounted; }
		virtual void			SetDDACounted() { bDDACounted = true; }
		virtual void			SetParentProjectile( hhProjectile* in_parent );
		virtual hhProjectile*	GetParentProjectile( void );
		virtual int				GetLaunchTimestamp() const { return launchTimestamp; }

		virtual void			Portalled(idEntity *portal);
		virtual bool			AllowCollision( const trace_t &collision );

		virtual void			UpdateBalanceInfo( const trace_t* collision, const idEntity* hitEnt );

		static hhProjectile*	SpawnProjectile( const idDict* args );
		static hhProjectile*	SpawnClientProjectile( const idDict* args );

		void					Save( idSaveGame *savefile ) const;
		void					Restore( idRestoreGame *savefile );

		virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
		virtual void			ClientHideProjectile(void);

		virtual const int		GetWeaponNum( void ) const { return weaponNum; } // HUMANHEAD CJR:  weapon-specific index on each projectile
		bool					ProjCollide() { return bProjCollide; }

		virtual int				ProcessCollision( const trace_t* collision, const idVec3& velocity ); //rww - made public

		void					SetCollidedPortal( hhPortal *newPortal, idVec3 newLocation, idVec3 newVelocity ); // cjr

		static idVec3			GetBounceDirection( const idVec3 &incoming_vector, 
												const idVec3 &surface_normal,
												const idEntity *incoming_entity = NULL,
												const idEntity *surface_entity = NULL );

		idCQuat					launchQuat; //rww - for saving off launch orientation
		idVec3					launchPos; //rww - for saving off launch pos

	protected:
		virtual float			DetermineDamageScale( const trace_t* collision ) const { return 1.0f; }
		virtual bool			DamageIsValid( const trace_t* collision, float& damageScale );
		virtual int				DetermineContents();
		virtual int				DetermineClipmask();

		virtual idEntity*		DetermineClipModelOwner();

		virtual int				PlayImpactSound( const idDict* dict, const idVec3 &origin, surfTypes_t type );

		virtual void			UpdateLight();
		virtual void			UpdateLightPosition();
		virtual void			UpdateLightFade();
		virtual int				CreateLight( const char* shaderName, const idVec3& size, const idVec3& color, const idVec3& offset, float fadeTime );

		virtual void			ApplyDamageEffect( idEntity* entHit, const trace_t* collision, const idVec3& velocity, const char* damageDefName );

		virtual void			SpawnExplosionFx( const trace_t* collision );
		virtual void			SpawnDebris( const idVec3& collisionNormal, const idVec3& collisionDir );

		virtual void			DamageEntityHit( const trace_t* collision, const idVec3& velocity, idEntity* entHit );

		virtual bool			ProcessAllowCollisionEvent( const trace_t* collision );

	protected:
		void					Event_Collision_Explode( const trace_t* collision, const idVec3& velocity );
		void					Event_Collision_Impact( const trace_t* collision, const idVec3& velocity );
		void					Event_Collision_DisturbLiquid( const trace_t* collision, const idVec3& velocity );
		void					Event_Collision_Remove( const trace_t* collision, const idVec3& velocity );
		void					Event_AllowCollision_CollideNoProj( const trace_t* collision );

		void					Event_AllowCollision_Collide( const trace_t* collision );
		void					Event_AllowCollision_PassThru( const trace_t* collision );

		void					Event_Fuse_Explode();

		void					Event_SpawnDriverLocal( const char* defName );
		void					Event_SpawnFxFlyLocal( const char* defName );


	protected:
		idEntityPtr<hhAnimDriven> driver;
		idEntityPtr<idEntityFx> fxFly;
		int						thrust_start;
		bool					bDDACounted;	//already counted by dda as hitting something
		idEntityPtr<hhProjectile>			parentProjectile;	//projectile that spawned me
		int						launchTimestamp;

		int						weaponNum;		// cjr - weapon index that spawned this projectile (-1) for non-player weapons

		bool					bNoCollideWithCrawlers; // mdl:  Defines whether or not we collide with crawlers/rockets
		bool					bProjCollide;

		// jsh flyby sounds
		float					flyBySoundDistSq;
		bool					bPlayFlyBySound;

		idEntityPtr<hhPortal>	collidedPortal;	// cjr:  This projectile struck a portal, so it should get portalled before thinking
		idVec3					collideLocation; // cjr: This projectile struck a portal, so it should get portalled before thinking
		idVec3					collideVelocity; // cjr: This projectile struck a portal, so it should get portalled before thinking
};

#endif