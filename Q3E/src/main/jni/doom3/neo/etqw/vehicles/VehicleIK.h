// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLES_VEHICLEIK_H__
#define __GAME_VEHICLES_VEHICLEIK_H__

#include "../IK.h"
#include "../script/ScriptEntityHelpers.h"

/*
===============================================================================

	IK controller for wheel based vehicle suspension

===============================================================================
*/

class sdVehicleRigidBodyWheel;
class sdTransport_RB;
class sdMotorSound;

class sdIK_WheeledVehicle : public idIK {
public:
	CLASS_PROTOTYPE( sdIK_WheeledVehicle );

							sdIK_WheeledVehicle( void );
	virtual					~sdIK_WheeledVehicle( void );

	void					AddWheel( sdVehicleRigidBodyWheel& wheel );
	void					ClearWheels( void );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	bool					Init( sdTransport_RB* self, const char *anim, const idVec3 &modelOffset );

	int						GetNumWheels( void ) const { return wheels.Num(); }

protected:
	idList< sdVehicleRigidBodyWheel* >	wheels;
	sdTransport_RB*						rbParent;
};

class sdIK_Walker : public idIK {
public:
	CLASS_PROTOTYPE( sdIK_Walker );

							sdIK_Walker( void );
	virtual					~sdIK_Walker( void );

	virtual bool			Init( idEntity *self, const char *anim, const idVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );	

	float					FindSmallest( float size1, float min2, float max2, float& sizeMin2, float& sizeMax2 );
	float					FindSmallest( float min1, float max1, float min2, float max2, float& sizeMin1, float& sizeMax1, float& sizeMin2, float& sizeMax2 );

	void					SetCompressionScale( float scale, float length );

	bool					GetStability( void ) const { return isStable; }

	virtual bool			IsInitialized( void ) const { return initialized; }

protected:
	idClipModel*			footModel;

	jointHandle_t			waistJoint;

	struct currentLegInfo_t {
		float				floorHeight;
		idVec3				jointWorldOrigin;
		idMat3				hipAxis;
		idMat3				kneeAxis;
		idMat3				ankleAxis;
		idMat3				midAxis;
	};

	struct leg_t {
		jointHandle_t		footJoint;
		jointHandle_t		ankleJoint;
		jointHandle_t		midJoint;
		jointHandle_t		kneeJoint;
		jointHandle_t		hipJoint;

		idVec3				hipForward;
		idVec3				midForward;
		idVec3				midSide;
		idVec3				kneeForward;

		float				upperLegLength;
		float				midLegLength;
		float				lowerLegLength;

		idMat3				upperLegToHipJoint;
		idMat3				midToUpperLegJoint;
		idMat3				lowerLegToKneeJoint;
		idMat3				lastAnkleAxes;

		float				oldAnkleHeight;

		currentLegInfo_t	current;
	};
	idList< leg_t >			legs;

	float					smoothing;
	float					downsmoothing;
	float					waistSmoothing;
	float					footShift;
	float					waistShift;
	float					minWaistFloorDist;
	float					minWaistAnkleDist;
	float					footUpTrace;
	float					footDownTrace;
	float					invertedLegDirOffset;
	bool					tiltWaist;
	bool					usePivot;

	bool					isStable;

	idInterpolate< float >	compressionInterpolate;

	// state
	int						pivotFoot;
	float					pivotYaw;
	idVec3					pivotPos;
	bool					oldHeightsValid;
	float					oldWaistHeight;
	idVec3					waistOffset;
};

class sdTransport;
class sdVehiclePosition;
class idPlayer;
class sdDeclStringMap;
class sdVehicleWeapon;

class sdVehicleIKSystem : public idClass {
public:
	ABSTRACT_PROTOTYPE( sdVehicleIKSystem );

	virtual					~sdVehicleIKSystem( void ) { }

	void					InitClamp( const angleClamp_t& yaw, const angleClamp_t& pitch );
	void					SetPosition( sdVehiclePosition* _position ) { position = _position; }
	virtual idPlayer*		GetPlayer( void );

	virtual bool			Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms );
	virtual void			Update( void ) = 0;

protected:
	sdTransport*				vehicle;
	angleClamp_t				clampYaw;
	angleClamp_t				clampPitch;
	sdVehiclePosition*			position;
	sdVehicleWeapon*			weapon;
};


class sdVehicleIKArms : public sdVehicleIKSystem {
public:
	CLASS_PROTOTYPE( sdVehicleIKArms );

	virtual void			Update( void );
	virtual bool			Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms );

protected:
	typedef enum armJoints_e {
		ARM_JOINT_INDEX_SHOULDER,
		ARM_JOINT_INDEX_ELBOW,
		ARM_JOINT_INDEX_WRIST,
		ARM_JOINT_INDEX_MUZZLE,
		ARM_JOINT_NUM_JOINTS
	} armJoints_t;

	sdMotorSound*			pitchSound;
	sdMotorSound*			yawSound;

	jointHandle_t			ikJoints[ ARM_JOINT_NUM_JOINTS ];
	idVec3					baseJointPositions[ ARM_JOINT_NUM_JOINTS ];
	idMat3					baseJointAxes[ ARM_JOINT_NUM_JOINTS ];
	idAngles				jointAngles;
	idMat3					oldParentAxis;

	int						pitchAxis;
	bool					requireTophat;
};

class sdVehicleSwivel : public sdVehicleIKSystem {
public:
	CLASS_PROTOTYPE( sdVehicleSwivel );

	virtual void			Update( void );
	virtual bool			Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms );

protected:
	sdMotorSound*			yawSound;

	idAngles				angles;
	idMat3					baseAxis;
	jointHandle_t			joint;
};

class sdVehicleWeaponAimer : public sdVehicleIKSystem {
public:
	CLASS_PROTOTYPE( sdVehicleWeaponAimer );

	virtual void			Update( void );
	virtual bool			Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms );

protected:
	sdScriptedEntityHelper_Aimer aimer;
};

class sdVehicleJointAimer : public sdVehicleIKSystem {
public:
	CLASS_PROTOTYPE( sdVehicleJointAimer );

	virtual void			Update( void );
	virtual bool			Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms );
	virtual idPlayer*		GetPlayer( void );

protected:
	sdMotorSound*			yawSound;
	sdMotorSound*			pitchSound;

	idAngles				angles;
	idMat3					baseAxis;
	jointHandle_t			joint;
	sdVehicleWeapon*		weapon2;
};

class sdVehicleIK_Steering : public sdVehicleIKSystem {
public:
	CLASS_PROTOTYPE( sdVehicleIK_Steering );

							sdVehicleIK_Steering( void );
	virtual					~sdVehicleIK_Steering( void );

	virtual void			Update( void );
	virtual bool			Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms );

private:
	sdPlayerArmIK			ik;
};

#endif // __GAME_VEHICLES_VEHICLEIK_H__
