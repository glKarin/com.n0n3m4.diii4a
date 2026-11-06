
#ifndef __FORCE_FIELD_H__
#define __FORCE_FIELD_H__

/*
===============================================================================

	Force field

===============================================================================
*/

enum forceFieldType {
	FORCEFIELD_UNIFORM,
	FORCEFIELD_EXPLOSION,
	FORCEFIELD_IMPLOSION
};

enum forceFieldApplyType {
	FORCEFIELD_APPLY_FORCE,
	FORCEFIELD_APPLY_VELOCITY,
	FORCEFIELD_APPLY_IMPULSE
};

class idForce_Field : public idForce {

public:
	CLASS_PROTOTYPE( idForce_Field );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

						idForce_Field( void );
	virtual				~idForce_Field( void );
						// uniform constant force
	void				Uniform( const idVec3 &force );
						// explosion from clip model origin
	void				Explosion( float force );
						// implosion towards clip model origin
	void				Implosion( float force );
						// add random torque
	void				RandomTorque( float force );
						// should the force field apply a force, velocity or impulse
	void				SetApplyType( const forceFieldApplyType type ) { applyType = type; }
						// make the force field only push players
	void				SetPlayerOnly( bool set ) { playerOnly = set; }
						// make the force field only push monsters
	void				SetMonsterOnly( bool set ) { monsterOnly = set; }
						// clip model describing the extents of the force field
	void				SetClipModel( idClipModel *clipModel );

// RAVEN BEGIN
// ddynerman: owner information
	void				SetOwner( idEntity* ent );
// bdube: added last apply time	
	int					GetLastApplyTime( void ) const;
// RAVEN END

public: // common force interface
	virtual void		Evaluate( int time );

private:
	// force properties
	forceFieldType		type;
	forceFieldApplyType	applyType;
	float				magnitude;
	idVec3				dir;
	float				randomTorque;
	bool				playerOnly;
	bool				monsterOnly;
	idClipModel *		clipModel;

// RAVEN BEGIN
// bdube: added last apply time	
	int					lastApplyTime;
	idEntity*			owner;
// RAVEN END
};

// RAVEN BEGIN
// bdube: added last apply time	
ID_INLINE int idForce_Field::GetLastApplyTime ( void ) const {
	return lastApplyTime;
}

// ddynerman: owner information
ID_INLINE void idForce_Field::SetOwner( idEntity* ent ) {
	owner = ent;
}
// RAVEN END

#endif /* !__FORCE_FIELD_H__ */
