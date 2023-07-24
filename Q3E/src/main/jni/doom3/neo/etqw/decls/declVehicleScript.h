// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLVEHICLESCRIPT_H__
#define __DECLVEHICLESCRIPT_H__

#include "../Game.h"
#include "../../decllib/declEntityDef.h"
#include "../../decllib/declTypeHolder.h"

//******************************************
// Consts
//******************************************

const int LIGHTID_VEHICLE_LIGHT = 200;

//******************************************
// ENUMS
//******************************************

typedef enum vehiclePartType_e {
	VPT_PART				= BITT< 0 >::VALUE,
	VPT_SIMPLE_PART			= BITT< 1 >::VALUE,
	VPT_WHEEL				= BITT< 2 >::VALUE,
	VPT_ROTOR				= BITT< 3 >::VALUE,
	VPT_HOVER				= BITT< 4 >::VALUE,
	VPT_MASS				= BITT< 5 >::VALUE,
	VPT_TRACK				= BITT< 6 >::VALUE,
	VPT_THRUSTER			= BITT< 7 >::VALUE,
	VPT_SUSPENSION			= BITT< 8 >::VALUE,
	VPT_VTOL				= BITT< 9 >::VALUE,
	VPT_ANTIGRAV			= BITT< 10 >::VALUE,
	VPT_SCRIPTED_PART		= BITT< 11 >::VALUE,
	VPT_PSEUDO_HOVER		= BITT< 12 >::VALUE,
	VPT_DRAGPLANE			= BITT< 13 >::VALUE,
	VPT_RUDDER				= BITT< 14 >::VALUE,
	VPT_AIRBRAKE			= BITT< 15 >::VALUE,
	VPT_HURTZONE			= BITT< 16 >::VALUE,
	VPT_ANTIROLL			= BITT< 17 >::VALUE,
	VPT_ANTIPITCH			= BITT< 18 >::VALUE,
} vehiclePartType_t;

//******************************************
// Structs
//******************************************

struct engineSoundInfo_t {
	idStr				soundFile;
	idStr				jointName;

	float				lowDB;
	float				highDB;

	float				lowRev;
	float				highRev;
	float				leadIn;
	float				leadOut;

	float				minFreqshift;
	float				maxFreqshift;
	float				fsStart;
	float				fsStop;
};

struct clipModelInfo_t {
	idStr				jointname;
	idVec3				offset;
	idVec3				mins;
	idVec3				maxs;
};

struct vehicleLightInfo_t {
	int						group;
	int						lightType;
	float					maxVisDist;
	idVec3					offset;
	idStr					jointName;
	idStr					shader;
	idVec3					color;
	idVec3					radius;
	idVec3					target;
	idVec3					up;
	idVec3					right;
	idVec3					start;
	idVec3					end;
	bool					pointlight;
	bool					noSelfShadow;
};

struct vehiclePartInfo_t{
	idDict					data;
};

struct vehicleRigidBodyRotorBladeInfo_t {
	idStr				joint;
	float				speedScale;
};

struct vehicleExitInfo_t {
	idStr				joint;
};

struct vehicleWeaponInfo_t {
	idStr					name;
	const sdDeclStringMap*	weaponDef;
	idStr					weaponType;
	angleClamp_t			clampYaw;
	angleClamp_t			clampPitch;
};

struct vehicleIKSystemInfo_t {
	idStr				name;
	idDict				ikParms;
	idStr				ikType;
	angleClamp_t		clampYaw;
	angleClamp_t		clampPitch;
};

struct positionViewMode_t {
	bool							allowDamping;
	bool							hideVehicle;
	bool							tophatRequired;
	bool							followPitch;
	bool							followYaw;
	bool							hideHud;
	bool							thirdPerson;
	bool							autoCenter;
	bool							showCockpit;
	bool							isInterior;
	bool							showCrosshairInThirdPerson;
	bool							hideDecoyInfo;
	bool							showTargetingInfo;
	bool							playerShadow;
	bool							noCockpitShadows;
	bool							showOtherPassengers;
	bool							matchPrevious;

	float							foliageDepthHack;
	float							damageScale;

	angleClamp_t					clampYaw;
	angleClamp_t					clampPitch;

	// final absolute clamps after view damping
	angleClamp_t					clampDampedYaw;
	angleClamp_t					clampDampedPitch;

	float							cameraDistance;
	float							cameraHeight;
	float							cameraFocus;
	float							cameraFocusHeight;

	idStr							eyes;
	idStr							eyePivot;

	idVec3							dampCopyFactor;
	float							dampSpeed;

	idStr							zoomTable;
	const idSoundShader*			zoomInSound;
	const idSoundShader*			zoomOutSound;

	idStr							type;

	idStr							sensitivityYaw;
	idStr							sensitivityPitch;
	idStr							sensitivityYawScale;
	idStr							sensitivityPitchScale;
};


struct positionInfo_t {
	idStr							name;
	const sdDeclLocStr*				hudname;
	idList< vehicleWeaponInfo_t >	weapons;
	idList< vehicleIKSystemInfo_t >	ikSystems;
	idList< positionViewMode_t >	views;
	idDict							data;
	idDict							cockpitInfo;
};

//******************************************
// Classes
//******************************************

class sdDeclVehicleLight {
public:
	vehicleLightInfo_t		lightInfo;

public:
	void					SetDefault( void );
	void					TouchMedia( void ) {
		if ( lightInfo.shader && lightInfo.shader[0] ) {
			declHolder.declMaterialType.LocalFind( lightInfo.shader );
		}
	}
};

class sdDeclVehicleEngineSound {
public:
	engineSoundInfo_t		soundInfo;

public:
	void					SetDefault( void );
};

class sdDeclVehiclePart {
public:
	idDict					data;

public:
	void					SetDefault( void );
	virtual void			TouchMedia( void );
};

class sdDeclVehicleExit {
public:
	vehicleExitInfo_t		exitInfo;

public:
	void					SetDefault( void );
};

class sdDeclVehiclePosition {
public:
	positionInfo_t			positionInfo;
	mutable int				oldCameraMode[ MAX_CLIENTS ];

public:
							sdDeclVehiclePosition( void );

	void					SetDefault( void );
	void					ClearWeapon( vehicleWeaponInfo_t& weapon );
	void					ClearIKSystem( vehicleIKSystemInfo_t& ikSystem );
	void					ClearView( positionViewMode_t& view );

	void					SetCameraMode( int clientIndex, int cameraMode ) const;
	int						GetCameraMode( int clientIndex ) const;

protected:
	void					ClearClamp( angleClamp_t& clamp );
};

typedef struct vehiclePart_s {
	sdDeclVehiclePart*		part;
	vehiclePartType_t		type;
} vehiclePart_t;

class sdDeclVehicleScript : public idDecl {
public:
											sdDeclVehicleScript( void );
	virtual									~sdDeclVehicleScript( void );

	virtual void							FreeData( void );
	virtual bool							Parse( const char *text, const int textLength );

	bool									ParseEngineSoundToken( sdDeclVehicleEngineSound* engineSound, idParser& src, idToken& token );
	bool									ParseExitToken( sdDeclVehicleExit* exit, idParser& src, idToken& token );
	bool									ParsePositionToken( sdDeclVehiclePosition* position, idParser& src, idToken& token );
	bool									ParseLightToken( sdDeclVehicleLight* light, idParser& src, idToken& token );	

	bool									ParseView( positionViewMode_t& view, idParser& src );
	bool									ParseClamp( angleClamp_t& clamp, idParser& src );
	bool									ParseWeapon( vehicleWeaponInfo_t& weapon, idParser& src );
	bool									ParseIKSystem( vehicleIKSystemInfo_t& ikSystem, idParser& src );

	template< typename T >
	bool									ParseItem( idParser& src, idList< T* >& list, bool ( sdDeclVehicleScript::*FUNC )( T* item, idParser& src, idToken& token ) );

	bool									ParsePart( idParser& src, vehiclePartType_t type );

	void									ResetCameraMode( int clientIndex ) const;
	int										GetCameraMode( int clientIndex, int positionIndex ) const;
	void									SetCameraMode( int clientIndex, int positionIndex, int cameraMode ) const;

	static void								CacheFromDict( const idDict& dict );

	void									TouchMedia( void ) const;

	const idList< sdPair< idStr, idDict > >&	GetCockpitInfo( void ) const { return cockpitInfo; }

public:
	idList< sdDeclVehicleEngineSound* >		engineSounds;
	idList< sdDeclVehicleLight* >			lights;
	idList< sdDeclVehiclePosition* >		positions;
	idList< sdDeclVehicleExit* >			exits;

	idList< vehiclePart_t >					parts;

	idList< sdPair< idStr, idDict > >		cockpitInfo;
};

#endif // __DECLVEHICLESCRIPT_H__


