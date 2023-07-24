// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLE_VEHICLE_RIGIDBODY_H__
#define __GAME_VEHICLE_VEHICLE_RIGIDBODY_H__

#include "Transport.h"
#include "VehicleIK.h"

class sdVehicle_RigidBody : public sdTransport_RB {
public:
	CLASS_PROTOTYPE( sdVehicle_RigidBody );

							sdVehicle_RigidBody( void );
							~sdVehicle_RigidBody( void );

	virtual void			DoLoadVehicleScript( void );

	void					Spawn( void );

	virtual bool			UpdateAnimationControllers( void );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity, int bodyId );
	virtual void			CollideFatal( idEntity* other );
	virtual idIK*			GetIK( void ) { return &ik; }

	virtual void			FreezePhysics( bool freeze );

	void					Event_SetDamageDealtScale( float scale );
	virtual void			SetDamageDealtScale( float scale );
	float					GetDamageDealtScale() const { return collideDamageDealtScale; }

	const sdDeclDamage*		GetCollideDamage() const { return collideDamage; }

	static void				IncRoadKillStats( idPlayer* player );

protected:
	int						nextSelfCollisionTime;
	int						nextJumpSound;
	int						nextCollisionTime;
	int						nextCollisionSound;

	const sdDeclDamage*		collideDamage;
	const sdDeclDamage*		collideFatalDamage;
	float					collideDamageDealtScale;
	float					collideDotLimit;

	sdIK_WheeledVehicle		ik;

	const sdProgram::sdFunction*		onCollisionFunc;
	const sdProgram::sdFunction*		onCollisionSideScrapeFunc;

protected:
	virtual void			HandleCollision( const trace_t &collision, const float velocity );
};

#endif // __GAME_VEHICLE_VEHICLE_RIGIDBODY_H__
