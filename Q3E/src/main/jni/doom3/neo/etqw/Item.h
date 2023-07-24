// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ITEM_H__
#define __GAME_ITEM_H__

#include "proficiency/ProficiencyManager.h"
#include "physics/Physics_SimpleRigidBody.h"

class idAnimatedEntity;

/*
===============================================================================

  Items the player can pick up or use.

===============================================================================
*/

class idItem : public idEntity {
public:
	CLASS_PROTOTYPE( idItem );

							idItem();
	virtual					~idItem();

	void					Spawn( void );
	virtual bool			GiveToPlayer( idPlayer *player );
	virtual bool			Pickup( idPlayer *player );

	virtual bool			WantsTouch( void ) const { return true; }

	const sdDeclItemPackage*	GetItemPackage( void ) { return package; }

	virtual bool			StartSynced( void ) const { return true; }

	enum {
		EVENT_PICKUP = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void			OnPickup( idPlayer *player );

	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const { return true; }
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	virtual sdTeamInfo*		GetGameTeam( void ) const { return team; }
	virtual void			SetGameTeam( sdTeamInfo* _team ) { team = _team; }
	void					SetDropper( idEntity* _dropper ) { dropper = _dropper; }
	void					SetPickupTime( int time ) { pickUpTime = time; }

	virtual void			OnTouch( idEntity *other, const trace_t& trace );

	void					Event_GetOwner( void );

protected:
	const sdDeclItemPackage*	package;

private:
	int						pickUpTime;
	sdTeamInfo*				team;
	idEntityPtr< idEntity >	dropper;

	const sdProgram::sdFunction*	onPrePickupFunction;
	const sdProgram::sdFunction*	onPickupFunction;

	sdRequirementContainer	requirements;
};

class sdMoveableItemNetworkData : public sdEntityStateNetworkData {
public:
								sdMoveableItemNetworkData( void ) : physicsData( NULL ) { ; }
	virtual						~sdMoveableItemNetworkData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
};

class idMoveableItem : public idItem {
public:
	CLASS_PROTOTYPE( idMoveableItem );

							idMoveableItem();
	virtual					~idMoveableItem();

	virtual void			OnPickup( idPlayer *player );

	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			Collide( const trace_t& collision, const idVec3& velocity, int bodyId );

	virtual bool			CanCollide( const idEntity* other, int traceId ) const;

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );

private:
	sdPhysics_SimpleRigidBody	physicsObj;
	idClipModel *				trigger;
	class sdWaterEffects		*waterEffects;
};

#endif /* !__GAME_ITEM_H__ */
