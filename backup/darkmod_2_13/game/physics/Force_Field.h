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
	virtual				~idForce_Field( void ) override;
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
						// grayman #2975 - set the player mass for reduced force effect when version == 1
	void				SetPlayerMass( float mass) { playerMass = mass; }
						// grayman #2975 - set the scaleImpulse flag ( 0 = default behavior (no scaling), 1 = scaled force behavior )
	void				SetScale( bool newScale ) { scaleImpulse = newScale; }

public: // common force interface
	virtual void		Evaluate( int time ) override;

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
	float				playerMass;    // grayman #2975
	bool				scaleImpulse; // grayman #2975
};

#endif /* !__FORCE_FIELD_H__ */
