// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DELIVERY_VEHICLE_H__
#define __DELIVERY_VEHICLE_H__

#include "../ScriptEntity.h"
#include "../effects/Effects.h"

class sdDeliveryVehicleBroadcastData : public sdScriptEntityBroadcastData {
public:
	sdDeliveryVehicleBroadcastData() {
		lastRollAccel = 0.0f;
	}

	virtual void		MakeDefault( void );

	virtual void		Write( idFile* file ) const;
	virtual void		Read( idFile* file );

	float				lastRollAccel;
};

class sdDeliveryVehicle : public sdScriptEntity {
	CLASS_PROTOTYPE( sdDeliveryVehicle );

	void				Spawn( void );
	virtual void		Think( void );
	virtual void		PostThink( void );

	// networking methods
	virtual void		ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void		ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void		WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual bool		WantsToThink( void ) const { return true; }

protected:

	enum vehicleMode_t {
		VMODE_NONE		= 0,
		VMODE_JOTUN		= 1,
		VMODE_MAGOG
	};

	enum deliveryMode_t {
		DMODE_NONE		= 0,
		DMODE_DELIVER	= 1,
		DMODE_DROP		= 2,
		DMODE_RETURN		= 3
	};

	vehicleMode_t		vehicleMode;
	deliveryMode_t		deliveryMode;
	int					modeStartTime;

	idVec3				endPoint;

	// Jotun moving
	void				Event_StartJotunDelivery( float startTime, float pathSpeed, float leadTime );
	void				Event_StartJotunReturn( float startTime, float pathSpeed, float leadTime );
	void				Jotun_Think();
	void				Jotun_DoMove( const idVec3& aheadPoint, const idVec3& aheadPointDir, const idVec3& endPoint, bool levelOut, bool leaving, float pathSpeed );

	float				lastRollAccel;
	float				pathSpeed;
	float				pathLength;
	float				leadTime;

	// Magog moving
	void				Event_StartMagogDelivery( float startTime, float pathSpeed, float leadTime, const idVec3& endPoint, float itemRotation );
	void				Event_StartMagogReturn( float startTime, float pathSpeed, float leadTime, const idVec3& endPoint );
	void				Magog_Think();
	void				Magog_DoMove( const idVec3& aheadPoint, const idVec3& aheadPointDir, const idVec3& endPoint, float itemRotation, float maxYawScale, bool orientToEnd, bool clampRoll, bool slowNearEnd, float pathSpeed );

	float				maxZVel;
	float				maxZAccel;
	float				itemRotation;
};

#endif // __DELIVERY_VEHICLE_H__
