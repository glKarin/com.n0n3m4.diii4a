
#ifndef __GAME_AF_H__
#define __GAME_AF_H__


/*
===============================================================================

  Articulated figure controller.

===============================================================================
*/

typedef struct jointConversion_s {
	int						bodyId;				// id of the body
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

class idAF {
public:
							idAF( void );
							~idAF( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetAnimator( idAnimator *a ) { animator = a; }
// RAVEN BEGIN
// ddynerman: purge constraints/joints before loading a new one
	bool					Load( idEntity *ent, const char *fileName, bool purgeAF = false );
// RAVEN END
	bool					IsLoaded( void ) const { return isLoaded && self != NULL; }
	const char *			GetName( void ) const { return name.c_str(); }
	void					SetupPose( idEntity *ent, int time );
	void					ChangePose( idEntity *ent, int time );
	int						EntitiesTouchingAF( afTouch_t touchList[ MAX_GENTITIES ] ) const;
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
	void					AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	int						BodyForClipModelId( int id ) const;

	void					SaveState( idDict &args ) const;
	void					LoadState( const idDict &args );

	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

	idPhysics_AF			physicsObj;			// articulated figure physics
	bool					TestSolid( void ) const;

protected:
	idStr					name;				// name of the loaded .af file
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

};

#endif /* !__GAME_AF_H__ */
