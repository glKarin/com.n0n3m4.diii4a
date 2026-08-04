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

#ifndef __GAME_AF_H__
#define __GAME_AF_H__


/*
===============================================================================

  Articulated figure controller.

===============================================================================
*/

/* FORWRD DECLS */
class idDeclAF_Body;
class idDeclAF_Constraint;

typedef struct jointConversion_s {
	int						bodyId;				// id of the body
	idStr					bodyName;			// TDM: String name of body.  Only used on add/remove for performance
	jointHandle_t			jointHandle;		// handle of joint this body modifies
	AFJointModType_t		jointMod;			// modify joint axis, origin or both
	idVec3					jointBodyOrigin;	// origin of body relative to joint
	idMat3					jointBodyAxis;		// axis of body relative to joint
} jointConversion_t;

typedef struct afTouch_s {
	idEntity *				touchedEnt;
	idClipModel *			touchedClipModel;
	idAFBody *				touchedByBody;
} afTouch_t;
typedef idFlexList<afTouch_s, CLIPARRAY_AUTOSIZE> idClip_afTouchList;

class idAF {
public:
							idAF( void );
	virtual ~idAF( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetAnimator( idAnimator *a ) { animator = a; }
	bool					Load( idEntity *ent, const char *fileName );
	bool					IsLoaded( void ) const { return isLoaded && self != NULL; }
	const char *			GetName( void ) const { return name.c_str(); }
	void					SetupPose( idEntity *ent, int time );
	void					ChangePose( idEntity *ent, int time );
	int						EntitiesTouchingAF( idClip_afTouchList &touchList ) const;
	void					Start( void );
	void					StartFromCurrentPose( int inheritVelocityTime );
	void					Stop( void );
	void					Rest( void );
	bool					IsActive( void ) const { return isActive; }
	void					SetConstraintPosition( const char *name, const idVec3 &pos );

	idPhysics_AF *			GetPhysics( void ) { return &physicsObj; }
	const idPhysics_AF *	GetPhysics( void ) const { return &physicsObj; }
	idBounds				GetBounds( void ) const;
	bool					UpdateAnimation( void );

	void					GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) const;
	void					GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
	void					ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
	void					AddForce( idEntity *ent, int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId );
	int						BodyForClipModelId( int id ) const;
	/**
	* Find the joint associated with the given body
	**/
	jointHandle_t			JointForBody( int body );
	int						BodyForJoint( jointHandle_t joint );

	void					SaveState( idDict &args ) const;
	void					LoadState( const idDict &args );

	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

	/**
	* TDM: Allows adding a body not in the AF file, bodyNew.  Must be referenced to an existing body in the AF file, bodyExist.
	* Also needs the entity for which we are doing this.
	**/
	void AddBodyExtern
		( idAFEntity_Base *ent, idAFBody *bodyNew, 
		  idAFBody *bodyExist, AFJointModType_t mod,
		  jointHandle_t joint = INVALID_JOINT );

	/**
	* TDM: Delete an externally added body
	* NOTE: Must remove the body on the AFPhysics object BEFORE calling this.
	* Because this also refreshes the jointMod bodyID's based on the physics object.
	**/
	void DeleteBodyExtern( idAFEntity_Base *ent, const char *bodyName );

protected:
	idStr					name;				// name of the loaded .af file
	idPhysics_AF			physicsObj;			// articulated figure physics
	idEntity *				self;				// entity using the animated model
	idAnimator *			animator;			// animator on entity
	int						modifiedAnim;		// anim to modify
	idVec3					baseOrigin;			// offset of base body relative to skeletal model origin
	idMat3					baseAxis;			// axis of base body relative to skeletal model origin
	idList<jointConversion_t>jointMods;			// list with transforms from skeletal model joints to articulated figure bodies
	idList<int>				jointBody;			// table to find the nearest articulated figure body for a joint of the skeletal model
	int						poseTime;			// last time the articulated figure was transformed to reflect the current animation pose
	int						restStartTime;		// time the articulated figure came to rest
	bool					isLoaded;			// true when the articulated figure is properly loaded
	bool					isActive;			// true if the articulated figure physics is active
	bool					hasBindConstraints;	// true if the bind constraints have been added

protected:
	void					SetBase( idAFBody *body, const idJointMat *joints );
	void					AddBody( idAFBody *body, const idJointMat *joints, const char *jointName, const AFJointModType_t mod );

	bool					LoadBody( const idDeclAF_Body *fb, const idJointMat *joints );
	bool					LoadConstraint( const idDeclAF_Constraint *fc );

	bool					TestSolid( void ) const;
};

#endif /* !__GAME_AF_H__ */
