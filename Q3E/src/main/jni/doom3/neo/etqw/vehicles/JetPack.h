// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_VEHICLES_JETPACK_H__
#define __GAME_VEHICLES_JETPACK_H__

#include "Transport.h"
#include "../physics/Physics_JetPack.h"
#include "../client/ClientEntity.h"

class sdJetPackVisuals : public sdClientAnimated {
public:
	CLASS_PROTOTYPE( sdJetPackVisuals );

							sdJetPackVisuals( void ) {};
							virtual ~sdJetPackVisuals( void ) {};

	virtual bool			UpdateRenderEntity( renderEntity_t* renderEntity, const renderView_t* renderView, int& lastGameModifiedTime );

protected:
};

class sdJetPackNetworkData : public sdTransportNetworkData {
public:
										sdJetPackNetworkData( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	float								currentJumpCharge;
};

class sdJetPack : public sdTransport {
public:
	CLASS_PROTOTYPE( sdJetPack );

							sdJetPack( void );
							~sdJetPack( void );

	void					Spawn( void );

	void					Event_GetChargeFraction( void );
	void					Event_GetViewAngles( void );

	virtual void			DoLoadVehicleScript( void );
	virtual void			FreezePhysics( bool freeze ) { }
	virtual void			LoadParts( int partTypes );

	virtual void			Think( void );
	virtual void			UpdateModelTransform( void );
	virtual void			UpdateJetPackVisuals( void );

	virtual bool			WantsToThink( void );

	virtual void			Present( void );
	virtual void			UpdateVisibility( void );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity, int bodyId );

	virtual void			SetAxis( const idMat3 &axis );
	virtual const idMat3 &	GetAxis( void ) { return lastAxis; }

	virtual const char*		GetDefaultSurfaceType( void ) const { return "metal"; }

	bool					OnGround();

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );

	virtual void			LinkCombat( void );

	float					GetChargeFraction( void ) const;

	virtual void			SetTeleportEntity( sdTeleporter* teleporter );

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdTransportNetworkData*		CreateTransportNetworkStructure( void ) const;

	sdPhysics_JetPack*		GetJetPackPhysics() { return &physicsObj; }

private:
	sdPhysics_JetPack		physicsObj;
	idAngles				lastAngles;
	idMat3					lastAxis;

	const sdDeclDamage*		collideDamage;
	int						nextSelfCollisionTime;
	int						nextCollisionTime;

	const sdDeclDamage*		fallDamage;
	float					minFallDamageSpeed;
	float					maxFallDamageSpeed;

	float					maxJumpCharge;
	float					currentJumpCharge;
	float					dischargeRate;
	float					chargeRate;

	float					moveSpeed;
	float					flySpeed;

	rvClientEntityPtr< rvClientEntity >	visuals;
};

#endif // __GAME_VEHICLES_JETPACK_H__
