// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_DOOR_H__
#define __GAME_DOOR_H__

#include "../Mover.h"
#include "../misc/RenderEntityBundle.h"

class sdDoorPhysicsNetworkData : public sdEntityStateNetworkData {
public:
							sdDoorPhysicsNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	float					currentPos;
	bool					closing;
};

class sdPhysics_Door : public idPhysics_Base {
public:
							sdPhysics_Door( void );
							~sdPhysics_Door( void );

	bool					IsClosed( void ) const { return currentPos == 0.f; }
	bool					IsOpen( void ) const { return currentPos == 1.f; }

	bool					IsClosing( void ) const { return destPos == 0.f && currentPos != destPos; }
	bool					IsOpening( void ) const { return destPos == 1.f && currentPos != destPos; }

	bool					Open( void );
	bool					Close( void );

	void					OpenPortals( void );
	void					ClosePortals( void );

	void					SetBodyProperties( int id, const idVec3& _pos1, const idVec3& _pos2, const idMat3& axes, bool pusher );
	void					UpdateBodyPosition( int id );
	bool					SetCurrentPos( float newPos );

	void					SetSpeed( float _speed ) { speed = _speed; }

	virtual void			SetClipModel( idClipModel *model, float density, int id, bool freeOld = true );
	virtual idClipModel*	GetClipModel( int id ) const;
	virtual int				GetNumClipModels( void ) const;
	virtual void			SetContents( int contents, int id );
	virtual int				GetContents( int id ) const;
	virtual const idBounds&	GetBounds( int id ) const;
	virtual const idBounds&	GetAbsBounds( int id ) const;
	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec );
	virtual bool			IsAtRest( void ) const;
	virtual bool			IsPushable( void ) const;
	virtual bool			EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );

	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;

	virtual const idVec3&	GetOrigin( int id = 0 ) const;
	virtual const idMat3&	GetAxis( int id = 0 ) const;

	virtual void			EnableClip( void );
	virtual void			DisableClip( bool activateContacting = true );

	virtual int				GetClipMask( int id ) const;
	virtual void			SetClipMask( int mask, int id );

private:
	struct body_t {
							body_t( void ) { clipModel = NULL; }

		idVec3				pos1;
		idVec3				pos2;
		idMat3				axes;
		idVec3				currentOrigin;
		idClipModel*		clipModel;
		int					clipMask;
		bool				pusher;
	};

	idList< body_t >		bodies;
	float					currentPos;
	float					destPos;
	float					speed;
};

struct doorSpawnInfo_t {
	idStr					name;
	idStr					group;
	renderEntity_t			renderEnt;
	idTraceModel			trm;
	idVec3					pos1;
	idVec3					pos2;
	idMat3					axes;
	bool					pusher;
};

class sdDoorNetworkData : public sdScriptEntityNetworkData {
public:
										sdDoorNetworkData( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	int									closeTime;
};

class idDoor : public sdScriptEntity {
public:
	CLASS_PROTOTYPE( idDoor );

	enum {
		EVENT_PORTALSTATE = sdScriptEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	static const int		TRIGGER_ID = 255;
	static const int		SND_TRIGGER_ID = 254;

							idDoor( void );
							~idDoor( void );

	void					Spawn( void );
	void					VectorForDir( float angle, idVec3 &vec );

	virtual void			Hide( void );
	virtual void			Show( void );

	virtual void			OnMasterReady( void );

	virtual bool			WantsTouch( void ) const { return !IsHidden(); }

	virtual void			UpdateModelTransform( void );
	virtual void			Present( void );
	virtual bool			CanCollide( const idEntity* other, int traceId ) const;

	virtual void			EnableClip( void );
	virtual void			DisableClip( bool activateContacting = true );

	virtual void			WriteDemoBaseData( idFile* file ) const;
	virtual void			ReadDemoBaseData( idFile* file );

	bool					Script_AllowOpen( idEntity* other );

	virtual void			PostMapSpawn( void );

	void					GetTraceModel( idTraceModel& trm );

	bool					IsOpen( void ) const;
	bool					IsPermanentlyOpen( void ) const;
	void					Use( idEntity *activator );
	void					Close( void );
	void					Open( void );

	virtual bool			WantsToThink( void ) const;

	void					OpenPortals( void );
	void					ClosePortals( bool force );

	virtual void			OnTeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity );
	virtual void			OnTouch( idEntity *other, const trace_t& trace );
	
	idEntity*				GetActivator( void ) const { return activator; }

	void					SpawnDoorTrigger( void );
	void					SpawnSoundTrigger( void );

	void					SpectatorTouch( idPlayer* p, const trace_t& trace );
	
	virtual void			ReachedPosition( void );
	virtual void			Think( void );

	void					SetCloseTime( int time );

	static void				OnNewMapLoad( void );
	static void				OnMapClear( void );

	virtual void			OnMoveStarted( void ) { moving = true; }
	virtual void			OnMoveFinished( void ) { moving = false; }

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg& msg );

	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;

	virtual const idVec3&	GetWayPointOrigin( void ) const { return baseOrg; }
	virtual const idBounds&	GetWayPointBounds( void ) const { return baseBounds; }
	virtual const idMat3&	GetWayPointAxis( void ) const { return baseAxis; }

	cheapDecalUsage_t		GetDecalUsage( void ) { return CDU_LOCAL; }

	virtual int				GetModelDefHandle( int id = 0 );

private:
	void					CalcPositions( idVec3& pos1, idVec3& pos2 );

	bool					toggle;
	bool					moving;
	idClipModel*			trigger;
	idClipModel*			sndTrigger;
	int						nextSndTriggerTime;
	int						normalAxisIndex;		// door faces X or Y for spectator teleports
	const sdProgram::sdFunction* checkOpenFunc;
	int						closeTime;
	int						waitTime;
	bool					reverseOnBlock;
	bool					crushOnBlock;
	bool					isSlave;

	idVec3					baseOrg;
	idMat3					baseAxis;
	idBounds				baseBounds;

	sdPortalState			portal;

	idList< sdRenderEntityBundle* > parts;

	idEntityPtr< idEntity >	activator;

	sdPhysics_Door			physicsObj;

	static idList< doorSpawnInfo_t* > s_doorInfo;

	void					CalcTriggerBounds( float size, idBounds &bounds );

	void					LinkSoundTrigger( void );
	void					LinkTrigger( void );

	void					Event_Activate( idEntity *activator );
	void					Event_Close( void );
	void					Event_Open( void );
	void					Event_IsOpen( void );
	void					Event_IsClosed( void );
	void					Event_IsOpening( void );
	void					Event_IsClosing( void );
	void					Event_OpenPortal( void );
	void					Event_ClosePortal( void );
	void					Event_ForceClosePortal( void );
	void					Event_SetSkin( const char* skinname );
	void					Event_SetToggle( bool t );
};

#endif // __GAME_DOOR_H__
