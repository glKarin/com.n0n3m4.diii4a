/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __GAME_IK_H__
#define __GAME_IK_H__

/*
===============================================================================

  IK base class with a simple fast two bone solver.

===============================================================================
*/

#define IK_ANIM				"ik_pose"

class idIK {
public:
							idIK( void );
	virtual					~idIK( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	bool					IsInitialized( void ) const;

	virtual bool			Init( idEntity *self, const char *anim, const idVec3 &modelOffset );
	virtual void			Evaluate( void );
	virtual void			ClearJointMods( void );

	bool					SolveTwoBones( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &dir, float len0, float len1, idVec3 &jointPos );
	float					GetBoneAxis( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &dir, idMat3 &axis );

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

							idIK_Walk( void );
	virtual					~idIK_Walk( void ) override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			Init( idEntity *self, const char *anim, const idVec3 &modelOffset ) override;
	virtual void			Evaluate( void ) override;
	virtual void			ClearJointMods( void ) override;

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

							idIK_Reach( void );
	virtual					~idIK_Reach( void ) override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			Init( idEntity *self, const char *anim, const idVec3 &modelOffset ) override;
	virtual void			Evaluate( void ) override;
	virtual void			ClearJointMods( void ) override;

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

#endif /* !__GAME_IK_H__ */
