// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_MOVER_H__
#define __GAME_MOVER_H__

#include "Entity.h"
#include "physics/Physics_Parametric.h"
#include "physics/Physics_Linear.h"
#include "ScriptEntity.h"

//
// mover directions.  make sure to change script/doom_defs.script if you add any, or change their order
//
typedef enum {
	DIR_UP				= -1,
	DIR_DOWN			= -2,
	DIR_LEFT			= -3,
	DIR_RIGHT			= -4,
	DIR_FORWARD			= -5,
	DIR_BACK			= -6,
	DIR_REL_UP			= -7,
	DIR_REL_DOWN		= -8,
	DIR_REL_LEFT		= -9,
	DIR_REL_RIGHT		= -10,
	DIR_REL_FORWARD		= -11,
	DIR_REL_BACK		= -12
} moverDir_t;


class sdPortalState {
public:
							sdPortalState( void );
							~sdPortalState( void );

	void					Init( const idBounds& bounds );

	void					Open( void );
	void					Close( void );

	bool					IsOpen( void ) const { return open; }

	bool					IsValid( void ) const { return areaPortal != 0; }

private:
	qhandle_t				areaPortal;
	bool					open;
};

class idSplinePath : public idEntity {
public:
	CLASS_PROTOTYPE( idSplinePath );

							idSplinePath();

	void					Spawn( void );
};


struct floorInfo_s {
	idVec3					pos;
	idStr					door;
	int						floor;
};

/*
===============================================================================

  Binary movers.

===============================================================================
*/

typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

class sdMoverBinaryBroadcastState : public sdScriptEntityBroadcastData {
public:
							sdMoverBinaryBroadcastState( void ) {}

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	int						closeTime;
	bool					hidden;
	bool					toggle;
};

class idMover_Binary : public sdScriptEntity {
public:
	CLASS_PROTOTYPE( idMover_Binary );

							idMover_Binary();
							~idMover_Binary();

	void					Spawn( void );

	virtual void			Think( void );
	virtual void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damageDecl, const float damageScale, trace_t* collision, bool forceKill );

	virtual void			PreBind( void );
	virtual void			PostBind( void );

	virtual bool			StartSynced( void ) const { return true; }

	void					Enable( bool b );
	void					InitSpeed( idVec3 &mpos1, idVec3 &mpos2, float mspeed, float maccelTime, float mdecelTime );
	void					InitTime( idVec3 &mpos1, idVec3 &mpos2, float mtime, float maccelTime, float mdecelTime );
	void					GotoPosition1( void );
	void					GotoPosition2( void );
	void					Use_BinaryMover( idEntity *activator );
	idMover_Binary *		GetActivateChain( void ) const { return activateChain; }
	idMover_Binary *		GetMoveMaster( void ) const { return moveMaster; }
	void					BindTeam( idEntity *bindTo );
	idEntity *				GetActivator( void ) const;

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			OnMasterReady( void ) { ; }

	virtual void			ReachedPosition( void );

	void					OpenPortals( void );
	void					ClosePortals( bool force );

	virtual void			PostMapSpawn( void );

protected:
	void					OpenPortal( void ) { portal.Open(); }
	void					ClosePortal( void ) { portal.Close(); }

	idVec3					pos1;
	idVec3					pos2;
	moverState_t			moverState;
	idMover_Binary*			moveMaster;
	idMover_Binary*			activateChain;
	int						soundPos1;
	int						sound1to2;
	int						sound2to1;
	int						soundPos2;
	int						soundLoop;
	float					wait;
	float					damage;
	int						duration;
	int						accelTime;
	int						decelTime;
	idEntityPtr<idEntity>	activatedBy;
	bool					enabled;
	sdPhysics_Linear		physicsObj;
	sdPortalState			portal;
	bool					blocked;
	bool					portalStatus;
	bool					toggle;
	int						closeTime;

	void					MatchActivateTeam( moverState_t newstate, int time );
	void					JoinActivateTeam( idMover_Binary *master );

	void					UpdateMoverSound( moverState_t state );
	void					SetMoverState( moverState_t newstate, int time );
	moverState_t			GetMoverState( void ) const { return moverState; }

	void					Event_Use_BinaryMover( idEntity *activator );
	void					Event_MatchActivateTeam( moverState_t newstate, int time );
	void					Event_Enable( void );
	void					Event_Disable( void );
	void					Event_OpenPortal( void );
	void					Event_ClosePortal( void );
	void					Event_GetMoveMaster( void );
	void					Event_GetNextSlave( void );
	void					Event_GotoPosition1( void );
	void					Event_GotoPosition2( void );
	void					Event_SetToggle( bool toggle );

	static void				GetMovedir( float dir, idVec3 &movedir );
};

class idPlat : public idMover_Binary {
public:
	CLASS_PROTOTYPE( idPlat );

							idPlat( void );
							~idPlat( void );

	void					Spawn( void );

	virtual void			Think( void );
	virtual void			PreBind( void );
	virtual void			PostBind( void );

	virtual bool			WantsTouch( void ) const { return true; }

	virtual void			OnTeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity );
	virtual void			OnPartBlocked( idEntity *blockingEntity );
	virtual void			OnTouch( idEntity *other, const trace_t& trace );

private:
	idClipModel *			trigger;
	idVec3					localTriggerOrigin;
	idMat3					localTriggerAxis;

	void					GetLocalTriggerPosition( const idClipModel *trigger );
	void					SpawnPlatTrigger( idVec3 &pos );
};

#endif /* !__GAME_MOVER_H__ */
