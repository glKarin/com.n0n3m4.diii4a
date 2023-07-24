// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

#include "physics/Physics_RigidBody.h"
#include "physics/Physics_SimpleRigidBody.h"
#include "physics/Physics_Parabola.h"
#include "Entity.h"
#include "proficiency/ProficiencyManager.h"
#include "effects/WaterEffects.h"

/*
===============================================================================

  idProjectile
	
===============================================================================
*/

extern const idEventDef EV_DelayedLaunch;

class sdProjectileNetworkData : public sdEntityStateNetworkData {
public:
								sdProjectileNetworkData( void ) : physicsData( NULL ) { ; }
	virtual						~sdProjectileNetworkData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
	sdScriptObjectNetworkData	scriptData;
};

class sdProjectileBroadcastData : public sdEntityStateNetworkData {
public:
								sdProjectileBroadcastData( void ) : physicsData( NULL ), team( NULL ) { ; }
	virtual						~sdProjectileBroadcastData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
	sdScriptObjectNetworkData	scriptData;

	sdTeamInfo*					team;
	int							ownerId;
	int							enemyId;
	int							launchTime;
	float						launchSpeed;
	idList< int >				owners;
	bool						hidden;
	sdBindNetworkData			bindData;
};

class idProjectile : public idEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile( void );
	virtual					~idProjectile( void );

	void					Spawn( void );

	virtual void			Create( idEntity *owner, const idVec3 &start, const idVec3 &dir );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

	virtual void			InitPhysics( void );
	virtual void			InitLaunchPhysics( float launchPower, const idVec3& origin, const idMat3& axes, const idVec3& pushVelocity );
	void					LaunchDelayed( int delay, idEntity* owner, const idVec3& org, const idVec3& dir, const idVec3& push );

	idEntity *				GetOwner( void ) const;
	virtual bool			IsOwner( idEntity* other ) const;

	virtual void			Think( void );
	virtual void					PostThink( void );
	virtual idLinkList<idEntity>*	GetPostThinkNode( void );

	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity, int bodyId );

	void					Thrust( void );
	void					UpdateTargeting( void );

	void					UpdateVisibility( void );

	virtual bool			DoRadiusPush( void ) const { return false; }

	virtual bool			CanCollide( const idEntity* other, int traceId ) const;

	virtual void			SetEnemy( idEntity* _enemy );
	virtual void			SetTarget( const idVec3& target ) { }

	bool					SetState( const sdProgram::sdFunction* newState );
	void					UpdateScript( void );

	virtual void			SetHealth( int count ) { health = count; }
	virtual void			SetMaxHealth( int count ) { maxHealth = count; }
	virtual int				GetHealth( void ) const { return health; }
	virtual int				GetMaxHealth( void ) const { return maxHealth; }

	virtual void			SetGameTeam( sdTeamInfo* _team ) { team = _team; }
	virtual sdTeamInfo*		GetGameTeam( void ) const { return team; }

	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const { return false; }

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			UpdateModelTransform( void );

	void					AddOwner( idEntity* ent ) { assert( ent ); owners.AddUnique( ent ); }

	void					Event_Hide( void );
	void					Event_Show( void );
	void					Event_Freeze( bool frozen );

	void					SetThrust( bool value );
	void					SetLaunchTime( int time );

	virtual void			OnTouch( idEntity *other, const trace_t& trace );

protected:
	idEntityPtr< idEntity >				owner;
	idList< idEntityPtr< idEntity > >	owners;
	idEntityPtr< idEntity >				enemy;

	typedef struct projectileFlags_s {
		bool				scriptHide					: 1;
		bool				frozen						: 1;
		bool				allowOwnerCollisions		: 1;
		bool				hasThrust					: 1;
		bool				thustOn						: 1;
		bool				faceVelocity				: 1;
		bool				faceVelocitySet				: 1;
	} projectileFlags_t;

	projectileFlags_t		projectileFlags;

	float					damagePower;

	int						launchTime;
	int						thrustStartTime;
	int						targetForgetTime;
	float					launchSpeed;

	float					thrustPower;

	int						health;
	int						maxHealth;

	idMat3					visualAxes;

	sdTeamInfo*				team;

	// state variables
	sdProgramThread*					baseScriptThread;
	const sdProgram::sdFunction*		scriptState;
	const sdProgram::sdFunction*		scriptIdealState;

	const sdProgram::sdFunction*		targetingFunction;

	const sdProgram::sdFunction*		onPostThink;	
	idLinkList< idEntity >				postThinkEntNode;

protected:

	void					Event_SetState( const char* stateName );
	void					Event_Fizzle( void );
	void					Event_DelayedLaunch( idEntity* owner, const idVec3 &org, idVec3 const &dir, idVec3 const &push );
	void					Event_GetDamagePower( void );
	void					Event_GetOwner( void );
	void					Event_SetOwner( idEntity* _owner );
	void					Event_GetLaunchTime( void );
	void					Event_Launch( const idVec3& velocity );
	void					Event_AddOwner( idEntity* other );
	void					Event_SetEnemy( idEntity* other );
	void					Event_IsOwner( idEntity* other );
	void					Event_GetEnemy( void );
};

/*
===============================================================================

  idProjectile_RigidBody
	
===============================================================================
*/

class idProjectile_RigidBody : public idProjectile {
public :
	CLASS_PROTOTYPE( idProjectile_RigidBody );

							idProjectile_RigidBody( void );
	virtual ~idProjectile_RigidBody( void );
							
	virtual void			Spawn( void );
	virtual void			InitPhysics( void );
	virtual void			InitLaunchPhysics( float launchPower, const idVec3& origin, const idMat3& axes, const idVec3& pushVelocity );

	virtual bool			StartSynced( void ) const { return true; }

	virtual void			CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );

protected:
	sdPhysics_SimpleRigidBody		physicsObj;
	sdWaterEffects			*waterEffects;
};

/*
===============================================================================

	sdProjectile_Parabolic

===============================================================================
*/

class sdProjectile_Parabolic : public idProjectile {
public :
	CLASS_PROTOTYPE( sdProjectile_Parabolic );

							sdProjectile_Parabolic( void );
	virtual					~sdProjectile_Parabolic( void );

	virtual void			Spawn( void );
	virtual void			InitPhysics( void );
	virtual void			InitLaunchPhysics( float launchPower, const idVec3& origin, const idMat3& axes, const idVec3& pushVelocity );

	virtual bool			StartSynced( void ) const { return true; }

	virtual void			CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );

protected:
	sdPhysics_Parabola		physicsObj;
	sdWaterEffects			*waterEffects;
};

#endif /* !__GAME_PROJECTILE_H__ */
