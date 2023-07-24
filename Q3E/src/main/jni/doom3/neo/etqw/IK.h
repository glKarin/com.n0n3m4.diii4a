// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_IK_H__
#define __GAME_IK_H__

/*
===============================================================================

  IK base class with a simple fast two bone solver.

===============================================================================
*/

#define IK_ANIM				"ik_pose"

class idPhysics;
class idAnimator;

class idIK : public idClass {
public:
	CLASS_PROTOTYPE( idIK );

							idIK( void );
	virtual					~idIK( void );

	virtual bool			IsInitialized( void ) const;
	bool					IsInhibited( void ) const;

	virtual bool			Init( idEntity *self, const char *anim, const idVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	static bool				SolveTwoBones( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &dir, float len0, float len1, idVec3 &jointPos );
	static float			GetBoneAxis( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &dir, idMat3 &axis );

	idPhysics*				GetPhysics();
	idAnimator*				GetAnimator();

protected:
	bool					initialized;
	bool					ik_activate;
	idEntity *				self;				// entity using the animated model
	idAnimator *			animator;			// animator on entity
	int						modifiedAnim;		// animation modified by the IK
	idVec3					modelOffset;
};


/*
===============================================================================

  IK controller for a walking character with an arbitrary number of legs.	

===============================================================================
*/

class idIK_Walk : public idIK {
public:
	CLASS_PROTOTYPE( idIK_Walk );

							idIK_Walk( void );
	virtual					~idIK_Walk( void );

	virtual bool			Init( idEntity *self, const char *anim, const idVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	void					EnableAll( void );
	void					DisableAll( void );
	void					EnableLeg( int num );
	void					DisableLeg( int num );

private:
	static const int		MAX_LEGS		= 8;

	idClipModel *			footModel;

	int						numLegs;
	int						enabledLegs;
	jointHandle_t			footJoints[MAX_LEGS];
	jointHandle_t			ankleJoints[MAX_LEGS];
	jointHandle_t			kneeJoints[MAX_LEGS];
	jointHandle_t			hipJoints[MAX_LEGS];
	jointHandle_t			dirJoints[MAX_LEGS];
	jointHandle_t			waistJoint;

	idVec3					hipForward[MAX_LEGS];
	idVec3					kneeForward[MAX_LEGS];

	float					upperLegLength[MAX_LEGS];
	float					lowerLegLength[MAX_LEGS];

	idMat3					upperLegToHipJoint[MAX_LEGS];
	idMat3					lowerLegToKneeJoint[MAX_LEGS];

	float					smoothing;
	float					waistSmoothing;
	float					footShift;
	float					waistShift;
	float					minWaistFloorDist;
	float					minWaistAnkleDist;
	float					footUpTrace;
	float					footDownTrace;
	bool					tiltWaist;
	bool					usePivot;

	// state
	int						pivotFoot;
	float					pivotYaw;
	idVec3					pivotPos;
	bool					oldHeightsValid;
	float					oldWaistHeight;
	float					oldAnkleHeights[MAX_LEGS];
	idVec3					waistOffset;
};


/*
===============================================================================

  IK controller for reaching a position with an arm or leg.

===============================================================================
*/

class idIK_Reach : public idIK {
public:
	CLASS_PROTOTYPE( idIK_Reach );

							idIK_Reach( void );
	virtual					~idIK_Reach( void );

	virtual bool			Init( idEntity *self, const char *anim, const idVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

private:

	static const int		MAX_ARMS	= 2;

	int						numArms;
	int						enabledArms;
	jointHandle_t			handJoints[MAX_ARMS];
	jointHandle_t			elbowJoints[MAX_ARMS];
	jointHandle_t			shoulderJoints[MAX_ARMS];
	jointHandle_t			dirJoints[MAX_ARMS];

	idVec3					shoulderForward[MAX_ARMS];
	idVec3					elbowForward[MAX_ARMS];

	float					upperArmLength[MAX_ARMS];
	float					lowerArmLength[MAX_ARMS];

	idMat3					upperArmToShoulderJoint[MAX_ARMS];
	idMat3					lowerArmToElbowJoint[MAX_ARMS];
};

/*
===============================================================================

	IK controller for aiming ( sets of ) two joints at each other

===============================================================================
*/

class sdIK_Aim : public idIK {
public:
	CLASS_PROTOTYPE( sdIK_Aim );

							sdIK_Aim( void );
	virtual					~sdIK_Aim( void );

	virtual bool			Init( idEntity *self, const char *anim, const idVec3& modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

protected:
	typedef struct jointGroup_s {
		jointHandle_t			joint1;
		jointHandle_t			joint2;

		idVec3					lastDir;
	} jointGroup_t;

	idList< jointGroup_t >		jointGroups;
};

#endif /* !__GAME_IK_H__ */
