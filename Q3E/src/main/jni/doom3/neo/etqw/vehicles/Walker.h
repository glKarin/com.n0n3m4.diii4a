// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLES_WALKER_H__
#define __GAME_VEHICLES_WALKER_H__

#include "../physics/Physics_Monster.h"
#include "Transport.h"
#include "VehicleIK.h"

class sdWalker : public sdTransport_AF {
public:
	CLASS_PROTOTYPE( sdWalker );

					sdWalker( void );
	virtual			~sdWalker( void );

	virtual void	Think( void );

	void			Event_GroundPound( float force, float damageScale, float shockWaveRange );

	void			Spawn( void );

	void			SetDelta( const idVec3& delta );
	void			SetCompressionScale( float scale, float length );

	virtual void	DoLoadVehicleScript( void );

	virtual void	DisableClip( bool activateContacting = true );
	virtual void	EnableClip( void );	
	virtual bool	UpdateAnimationControllers( void );

	virtual void	OnUpdateVisuals( void );

	virtual void	FreezePhysics( bool freeze ) { }

	virtual idIK*	GetIK( void ) { return &ik; }

	virtual void	SetAxis( const idMat3 &axis );
	virtual const idMat3	&GetAxis( void ) { return viewAxis; }

	void			GetMoveDelta( idVec3 &delta );

	bool			LoadIK( void );
	virtual void	LoadAF( void );

	virtual	void	UpdateModelTransform( void );

	virtual bool	Collide( const trace_t& trace, const idVec3& velocity, int bodyId );
	void			GroundPound( float force, float damageScale, float shockWaveRange );

	virtual void	OnPlayerExited( idPlayer* player, int position );

	idPhysics_Monster& GetMonsterPhysics( void ) { return physicsObj; }

	void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );

private:
	idPhysics_Monster		physicsObj;
	sdIK_Walker				ik;

	idMat3					viewAxis;

	const sdDeclDamage*		stompDamage;
	float					minStompScale;
	float					maxStompScale;
	float					stompSpeedScale;

	float					groundPoundMinSpeed;
	float					groundPoundForce;
	float					groundPoundRange;

	float					kickForce;

	int						lastForcedUpdateTime;

	int						lastIKTime;
	idVec3					lastIKPos;
};

#endif // __GAME_VEHICLES_WALKER_H__

