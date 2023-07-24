// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_MISC_H__
#define __GAME_MISC_H__

#include "ScriptEntity.h"
#include "physics/Force_Spring.h"
#include "physics/Force_Field.h"
#include "physics/Physics_Parametric.h"
#include "physics/Physics_StaticMulti.h"
#include "Trigger.h"
#include "AFEntity.h"

/*
===============================================================================

idSpawnableEntity

A simple, spawnable entity with a model and no functionable ability of it's own.
For example, it can be used as a placeholder during development, for marking
locations on maps for script, or for simple placed models without any behavior
that can be bound to other entities.  Should not be subclassed.
===============================================================================
*/

class idSpawnableEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idSpawnableEntity );

	void				Spawn( void );

private:
};


/*
===============================================================================

sdModelStatic

A simple, spawnable entity with a collision model and no functionable abilityof it's own.

	NOTE: this entity is really a hack, and can go as soon as idEntity is cleaned up
	( that is, when it has no renderEntity anymore )

===============================================================================
*/

class sdModelStatic : public idEntity {
public:
	CLASS_PROTOTYPE( sdModelStatic );

	void				Spawn( void );

	static bool			InhibitSpawn( const idDict& args );

	virtual void		PostMapSpawn( void );
	virtual void		Show( void ) { }
	virtual bool		ShouldConstructScriptObjectAtSpawn() const { return false; }

private:
};

class sdDynamicSpawnPoint : public sdScriptEntity {
public:
	CLASS_PROTOTYPE( sdDynamicSpawnPoint );

						sdDynamicSpawnPoint( void );
						~sdDynamicSpawnPoint( void );
						
	void				Spawn( void );

	virtual bool		CanCollide( const idEntity* other, int traceId ) const;

private:
	sdSpawnPoint*		spawnPoint;
};

/*
===============================================================================

  Potential spawning position for players.

===============================================================================
*/

class idPlayerStart : public idEntity {
public:
	CLASS_PROTOTYPE( idPlayerStart );

						idPlayerStart( void );

	virtual void		PostMapSpawn( void );

	bool				ShouldConstructScriptObjectAtSpawn( void ) const { return false; }

	static bool			InhibitSpawn( const idDict& args );
};

/*
===============================================================================

  idForceField

===============================================================================
*/

class idForceField : public idEntity {
public:
	CLASS_PROTOTYPE( idForceField );

	void				Spawn( void );

	virtual void		Think( void );

private:
	idForce_Field		forceField;
	bool				active;

	void				Toggle( void );

	void				Event_Activate( idEntity *activator );
	void				Event_Toggle( void );
	void				Event_FindTargets( void );
	void				Event_GetMins( void );
	void				Event_GetMaxs( void );
};


/*
===============================================================================

  idAnimated

===============================================================================
*/

class idAnimated : public idAFEntity_Base {
public:
	CLASS_PROTOTYPE( idAnimated );

							idAnimated();
							~idAnimated();

	void					Spawn( void );
	virtual bool			LoadAF( void );
	bool					StartRagdoll( void );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

private:
	int						num_anims;
	int						current_anim_index;
	int						anim;
	int						blendFrames;
	jointHandle_t			soundJoint;
	idEntityPtr<idEntity>	activator;
	bool					activated;

	void					PlayNextAnim( void );

	void					Event_Activate( idEntity *activator );	
	void					Event_Start( void );
	void					Event_StartRagdoll( void );
	void					Event_AnimDone( int animIndex );
};


/*
===============================================================================

  idStaticEntity

===============================================================================
*/

class sdStaticEntityNetworkData : public sdEntityStateNetworkData {
public:
								sdStaticEntityNetworkData( void ) : physicsData( NULL ) { ; }
	virtual						~sdStaticEntityNetworkData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
};

class sdStaticEntityBroadcastData : public sdEntityStateNetworkData {
public:
								sdStaticEntityBroadcastData( void ) : physicsData( NULL ) { ; }
	virtual						~sdStaticEntityBroadcastData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
	int							hidden;
	int							forceDisableClip;
};

class idStaticEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idStaticEntity );

						idStaticEntity( void );
	virtual				~idStaticEntity( void );

	void				Spawn( void );
	virtual void		Hide( void );
	virtual void		Show( void );
	virtual void		Think( void );

	static bool			InhibitSpawn( const idDict& args );

	virtual void		PostMapSpawn( void );

	void				UpdateThinkStatus( void );

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
};


/*
===============================================================================

	sdEnvDefinition

===============================================================================
*/

class sdEnvDefinitionEntity : public idEntity {
public:
	CLASS_PROTOTYPE( sdEnvDefinitionEntity );

						sdEnvDefinitionEntity( void ) { ; }
	void				Spawn( void );
};

/*
===============================================================================

	sdSpawnController

===============================================================================
*/

class sdSpawnController : public sdScriptEntity {
public:
	CLASS_PROTOTYPE( sdSpawnController );

										sdSpawnController( void ) { ; }
	void								Spawn( void );

	virtual sdRequirementContainer*		GetSpawnRequirements( void ) { return &spawnRequirements; }

private:
	sdRequirementContainer				spawnRequirements;
};

/*
===============================================================================

	sdLiquid

===============================================================================
*/

class sdLiquid : public idEntity {
public:
	CLASS_PROTOTYPE( sdLiquid );

										sdLiquid( void );
	void								Spawn( void );

	virtual void						GetWaterCurrent( idVec3& waterCurrent ) const { waterCurrent = current; }

private:
	idVec3								current;
};

/*
===============================================================================

	sdLODEntity

===============================================================================
*/

class sdLODEntity : public idEntity {
public:
	CLASS_PROTOTYPE( sdLODEntity );

								sdLODEntity();
	virtual						~sdLODEntity();

	void						Spawn();
	void						PostMapSpawn();

	void						AddClipModel( idClipModel* model, const idVec3& origin, const idMat3& axes );
	void						AddRenderEntity( const renderEntity_t& entity, int ID );

private:
	void						FreeModelDefs();

private:
	idList< int >				modelDefHandles;
	idList< int >				modelID;
	idPhysics_StaticMulti		physicsObj;
};

/*
===============================================================================

sdImposterEntity

===============================================================================
*/

class sdImposterEntity : public idEntity {
public:
	CLASS_PROTOTYPE( sdImposterEntity );

	sdImposterEntity();
	virtual ~sdImposterEntity();

	void						Spawn();

private:
	void						FreeModelDefs();

private:
	idList< int >				modelDefHandles;
};


/*
===============================================================================

sdJumpPad

===============================================================================
*/

class sdJumpPad : public idTrigger_Multi { 
public:
	CLASS_PROTOTYPE( sdJumpPad );

	void						Spawn( void );

	void						OnTouch( idEntity *other, const trace_t& trace );
	virtual void				PostMapSpawn( void );

protected:

	idForce_Field				forceField;
	idMat3						effectAxis;

	int							nextTriggerTime;
	int							triggerWait;
};

/*
===============================================================================

	sdInstStatic

===============================================================================
*/

class sdInstStatic : public idEntity {
public:
	CLASS_PROTOTYPE( sdInstStatic );

								sdInstStatic();
	virtual						~sdInstStatic();

	void						Spawn();

	bool						ShouldConstructScriptObjectAtSpawn( void ) const { return false; }

	void						AddClipModel( idClipModel* model, const idVec3& origin, const idMat3& axes );
	void						AddRenderEntity( const renderEntity_t& entity );

private:
	void						FreeModelDefs();

private:
	idList< int >				modelDefHandles;
	idPhysics_StaticMulti		physicsObj;
};

/*
===============================================================================

	sdEnvBound

===============================================================================
*/

class sdEnvBoundsEntity : public idEntity {
public:
	CLASS_PROTOTYPE( sdEnvBoundsEntity );

						sdEnvBoundsEntity( void ) { ; }
	void				Spawn( void );
};


/*
===============================================================================

	sdLadderEntity

===============================================================================
*/

class sdLadderEntity : public idEntity {
public:
	CLASS_PROTOTYPE( sdLadderEntity );

	void				Spawn( void );
	virtual void		Think( void );

	virtual				~sdLadderEntity( void );

	bool				IsActive( void ) const { return !fl.forceDisableClip; }
	idVec3				GetLadderNormal( void ) const;
	idClipModel*		GetLadderModel( void ) const { return ladderModel; }

private:
	idVec3				ladderNormal;
	idClipModel*		ladderModel;
};

/*
===============================================================================

	idAASObstacleEntity

	Allows turning on and off AAS Obstacles

===============================================================================
*/

class idAASObstacleEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idAASObstacleEntity );

						idAASObstacleEntity( void );

	void				Spawn( void );

	bool				IsEnabled( void ) { return enabled; }
	int					GetTeam( void ) { return team; }

private:
	void				Event_Activate( idEntity *activator );

	void				ChangeAreaState();

	bool				enabled;
	int					team;
};

#endif /* !__GAME_MISC_H__ */
