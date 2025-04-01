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

#ifndef __LIQUID_H__
#define __LIQUID_H__

/*
===============================================================================

  idLiquid

	Base class for all liquid object.  The entity part of the liquid is
	responsible for spawning splashes and sounds to match.

	The physics portion is as usual, responsible for the physics.
===============================================================================
*/

#define	MOD_WATERPHYSICS_VER	"Water Physics 0.8 ^7by Lloyd"
#ifdef MOD_WATERPHYSICS

class idRenderModelLiquid;

class idLiquid : public idEntity {
public:
	CLASS_PROTOTYPE( idLiquid );

	idLiquid( void );

	virtual ~idLiquid() override;

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		Collide( const trace_t &collision, const idVec3 &velocity ) override;

private:
	void				Event_Touch( idEntity *other, trace_t *trace );

	idPhysics_Liquid	physicsObj;

	idRenderModelLiquid *model;

	const idDeclParticle *splash[3];
	const idDeclParticle *waves;

	idStr				smokeName;
	idStr				soundName;
};

#endif // MOD_WATERPHYISCS
#endif // __LIQUID_H__
