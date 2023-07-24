// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLE_WEAPON_H__
#define __GAME_VEHICLE_WEAPON_H__

#include "../Weapon.h"
#include "../script/ScriptEntityHelpers.h"

class sdVehiclePosition;
class sdTransport;
class sdDeclStringMap;
class idWeapon;
class idPlayer;
class idAnimatedEntity;

// ===================================================================
// ===================================================================

class sdVehicleWeapon : public idClass {
public:
	ABSTRACT_PROTOTYPE( sdVehicleWeapon );

										sdVehicleWeapon( void );
	virtual								~sdVehicleWeapon( void );

	bool								Setup( sdTransport* _vehicle, const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch );
	void								SetPosition( sdVehiclePosition* _position );
	void								SetBasePosition( sdVehiclePosition* _position ) { basePosition = _position; }
	void								OnPositionPlayerChanged( void );

	idPlayer*							GetPlayer( void );
	const idDict&						GetSpawnParms( void ) const { return spawnWeaponParms->GetDict(); }

	idEntity*							GetEnemy( void );
	sdVehiclePosition*					GetPosition( void ) { return position; }
	sdVehiclePosition*					GetBasePosition( void ) { return basePosition; }
	bool								GetNoTophatCrosshair( void ) const { return noTophatCrosshair; }
	const sdDeclLocStr*					GetWeaponName( void ) const { return gunName; }

	sdProgramThread*					CreateScriptThread( const sdProgram::sdFunction* function );
	void								ConstructScriptObject( void );
	void								DeconstructScriptObject( void );

	const char*							GetName( void ) const { return name; }

	virtual void						SetTarget( const idVec3& target, bool enabled ) { ; }
	virtual bool						Spawn( const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch );
	virtual void						Update( void );

	const sdWeaponLockInfo*				GetLockInfo( void ) const { return lockInfo.IsSupported() ? &lockInfo : NULL; }

	void								UpdateScript( void );
	void								Script_SetState( const sdProgram::sdFunction* function );
	bool								SetState( void );

	void								Event_GetKey( const char* key );
	void								Event_GetFloatKey( const char* key );
	void								Event_GetVectorKey( const char* key );
	void								Event_GetVehicle( void );
	void								Event_GetPlayer( void );
	void								Event_SetState( const char* state );
	void								Event_SetTarget( const idVec3& target, bool state );
	
	bool								IsWeaponReady( void );
	void								GetWeaponOriginAxis( idVec3& org, idMat3& axis );
	jointHandle_t						GetWeaponJointHandle( void ) const { return gunJointHandle; }


	virtual idScriptObject*				GetScriptObject( void ) const { return scriptObject; }

	virtual bool						CanAimAt( const idVec3& aimPosition ) { return false; }

	bool								HasLockClamp( void ) const { return lockClampPitch.flags.enabled || lockClampYaw.flags.enabled; }
	bool								IsValidLockDirection( const idVec3& worldDirection ) const;

protected:
	sdVehiclePosition*					position;
	sdVehiclePosition*					basePosition;
	sdTransport*						vehicle;
	
	const sdDeclStringMap*				spawnWeaponParms;
	sdWeaponLockInfo					lockInfo;

	bool								scriptActive;
	bool								noTophatCrosshair;

	idScriptObject*						scriptObject;
	sdProgramThread*					scriptThread;
	const sdProgram::sdFunction*		scriptState;
	const sdProgram::sdFunction*		scriptIdealState;
	idStr								name;

	const sdProgram::sdFunction*		weaponReadyFunc;
	jointHandle_t						gunJointHandle;

	angleClamp_t						lockClampPitch;
	angleClamp_t						lockClampYaw;

	const sdDeclLocStr*					gunName;
};

// ===================================================================
// ===================================================================

const int VEHICLE_MINIGUN_FIXED_JOINT_YAW		= 0;
const int VEHICLE_MINIGUN_FIXED_JOINT_PITCH		= 1;
const int VEHICLE_MINIGUN_FIXED_NUM_JOINTS		= 2;

const int VEHICLE_MINIGUN_FIXED_IK_BASE			= 0;
const int VEHICLE_MINIGUN_FIXED_IK_YAW			= 1;
const int VEHICLE_MINIGUN_FIXED_IK_PITCH		= 2;
const int VEHICLE_MINIGUN_FIXED_IK_NUM_PATHS	= 3;

class sdVehicleWeaponFixedMinigun : public sdVehicleWeapon {
public:
	CLASS_PROTOTYPE( sdVehicleWeaponFixedMinigun );

										sdVehicleWeaponFixedMinigun( void );
	virtual								~sdVehicleWeaponFixedMinigun( void );

	virtual bool						Spawn( const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch );

	virtual void						SetTarget( const idVec3& target, bool enabled );
	virtual void						Update( void );

	virtual bool						CanAimAt( const idVec3& aimPosition );

protected:

	sdScriptedEntityHelper_Aimer		aimer;

	bool								manualTarget;
	idVec3								manualTargetPos;
};

// ===================================================================
// ===================================================================

class sdVehicleWeaponLocked : public sdVehicleWeapon {
public:
	CLASS_PROTOTYPE( sdVehicleWeaponLocked );

	virtual bool						Spawn( const sdDeclStringMap& weaponParms, const angleClamp_t& clampYaw, const angleClamp_t& clampPitch );

	virtual bool						CanAimAt( const idVec3& aimPosition );

protected:

	const static int					MAX_CANAIM_JOINTS = 8;

	idStaticList< jointHandle_t, MAX_CANAIM_JOINTS >	canAimJoints;

	bool								notReallyLocked;
	angleClamp_t						nrl_yawClamp;
	angleClamp_t						nrl_pitchClamp;
};

// ===================================================================
// ===================================================================

class sdVehicleWeaponFactory {
public:
	static sdVehicleWeapon* GetWeapon( const char* weaponType );
};

#endif // __GAME_VEHICLE_WEAPON_H__
