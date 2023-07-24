// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_MISC_PLAYERBODY_H__
#define __GAME_MISC_PLAYERBODY_H__

#include "../AFEntity.h"
#include "../interfaces/UsableInterface.h"
#include "../physics/Physics_Monster.h"
#include "../Player.h"

class sdPlayerBody;

const int MAX_PLAYERBODIES = 32;

class sdPlayerBodyInteractiveInterface : public sdInteractiveInterface {
public:
										sdPlayerBodyInteractiveInterface( void ) { _owner = NULL; _activateFunc = NULL; }

	void								Init( sdPlayerBody* owner );

	virtual bool						OnActivate( idPlayer* player, float distance );
	virtual bool						OnActivateHeld( idPlayer* player, float distance );
	virtual bool						OnUsed( idPlayer* player, float distance ) { return false; }

private:
	sdPlayerBody*						_owner;
	const sdProgram::sdFunction*		_activateFunc;
	const sdProgram::sdFunction*		_activateHeldFunc;
};

class sdPlayerBodyNetworkData : public sdEntityStateNetworkData {
public:
								sdPlayerBodyNetworkData( void ) : physicsData( NULL ) { ; }
	virtual						~sdPlayerBodyNetworkData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
	sdScriptObjectNetworkData	scriptData;
};

class sdPlayerBody : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( sdPlayerBody );

								sdPlayerBody( void );

	void						Spawn( void );

	void						Init( idPlayer* _client, const sdPlayerClassSetup* _playerClass, sdTeamInfo* _team );
	void						Init( int clientSpawnId, int rankIndex, int ratingIndex, int classIndex, const idList< int >& classOptions, sdTeamInfo* teamInfo, int tAnimNum, int tAnimStartTime, int lAnimNum, int lAnimStartTime, float newViewYaw );

	virtual bool				GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );

	virtual bool				UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info );
	virtual bool				ShouldConstructScriptObjectAtSpawn( void ) const { return false; }
	virtual bool				ClientReceiveEvent( int event, int time, const idBitMsg& msg );
	virtual bool				StartSynced( void ) const { return true; }

	virtual sdInteractiveInterface* GetInteractiveInterface( void ) { return &interactiveInterface; }

	virtual bool				CanCollide( const idEntity* other, int traceId ) const;

	idPlayer*					GetClient( void ) const { return client; }
	const sdDeclRank*			GetRank( void ) const { return rank; }
	const sdDeclRating*			GetRating( void ) const { return rating; }
	const sdPlayerClassSetup*	GetPlayerClassSetup( void ) const { return &playerClass; }
	const sdDeclPlayerClass*	GetPlayerClass( void ) const { return playerClass.GetClass(); }
	const idList< int >&		GetClassOptions( void ) const { return playerClass.GetOptions(); }

	virtual void				WriteDemoBaseData( idFile* file ) const;
	virtual void				ReadDemoBaseData( idFile* file );

	sdTeamInfo*					GetGameTeam( void ) const { return team; }

	static int					SortByTime( sdPlayerBody* bodyA, sdPlayerBody* bodyB );

	virtual cheapDecalUsage_t	GetDecalUsage( void );

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	void						Event_GetOwner( void );
	void						Event_GetRenderViewAngles( void );

	enum {
		EVENT_INIT				= idAFEntity_Base::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	int							GetCreationTime( void ) const { return creationTime; }

	const sdProgram::sdFunction*		isSpawnHostableFunc;
	const sdProgram::sdFunction*		isSpawnHostFunc;
	const sdProgram::sdFunction*		hasNoUniformFunc;
	const sdProgram::sdFunction*		prePlayerFullyKilledFunc;

private:
	void								SetupBody( void );

	idPhysics_Monster					physicsObj;

	idEntityPtr< idPlayer >				client;
	sdPlayerClassSetup					playerClass;
	sdTeamInfo*							team;
	const sdDeclRank*					rank;
	const sdDeclRating*					rating;

	float								viewYaw;
	idMat3								viewAxis;

	int									creationTime;

	int									torsoAnimNum;
	int									torsoAnimStartTime;

	int									legsAnimNum;
	int									legsAnimStartTime;

	const sdProgram::sdFunction*		updateCrosshairFunc;

	sdPlayerBodyInteractiveInterface	interactiveInterface;
};

#endif // __GAME_MISC_PLAYERBODY_H__
