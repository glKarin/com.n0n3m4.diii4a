// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __TELEPORTER_H__
#define __TELEPORTER_H__

#include "physics/Physics_StaticMulti.h"

class sdTeleporter : public idEntity {
public:
	CLASS_PROTOTYPE( sdTeleporter );

						sdTeleporter( void );
						~sdTeleporter( void );

	void				Spawn( void );
	virtual void		Think( void );

	virtual bool		StartSynced( void ) const { return true; }

	virtual bool		WantsTouch( void ) const { return true; }
	virtual void		OnTouch( idEntity *other, const trace_t& trace );

	void				Event_EnableTeam( const char* team );
	void				Event_DisableTeam( const char* team );
	void				Event_FinishTeleport( int spawnId );

	void				Latch( idEntity* ent );

	void				CancelTeleport( idEntity* ent );
	void				FinishTeleport( idEntity* ent );

	virtual void		PostMapSpawn( void );

	idEntity*			GetViewEntity( void ) { return viewLocation; }

	void				GetTargetPosition( idVec3& origin, idMat3& axis );

	void				GetTeleportEndPoint( idEntity* ent, idVec3& org, idMat3& axes );

protected:
	idList< idEntityPtr< idEntity > > latches;

	struct teamInfo_t {
		bool				enabled;
	};

	struct teleportParms_t {
		idVec3				location;
		idMat3				orientation;

		idVec3				linearVelocity;
		idVec3				angularVelocity;

		int					spawnId;
	};

	void				StartTeleport( const teleportParms_t& parms );
	void				FinishTeleport( const teleportParms_t& parms );

	idPhysics_StaticMulti		staticPhysics;

	idList< teamInfo_t >		teamInfo;
	idList< teleportParms_t >	teleportInfo;

	idEntityPtr< idEntity >		storageLocation;
	idEntityPtr< idEntity >		viewLocation;
	int							delay;

	idVec3						exitVelocity;

	float						deployReverse;
	float						deployLength;
	float						deployWidth;

	const sdDeclDamage*			telefragDamage;
};

#endif

