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

#ifndef PHYSICS_LIQUID_H
#define PHYSICS_LIQUID_H

/*
===============================================================================

	Physics object for a liquid.  This class contains physics properties for a 
	given liquid.  Note: Liquid does not necessarily imply water.

	This class does very little functionally as it relies on the other physics
	classes to do the bouoyancy calculations.  It simply holds information and
	allows the other object to deal with that information however they please.

	As a side note, the difference between minSplashVelocity and 
	minWaveVelocity is that min splash is the minimum amount of velocity 
	before the liquid spawns a splash particle.  minWaveVelocity is to generate
	a wave on the surface, not a splash.  It should be lower than min splash 
	velocity.  It's the reason some things won't splash but will still cause 
	ripples in the water (especially when surfacing)
===============================================================================
*/

#ifdef MOD_WATERPHYSICS

class idPhysics_Liquid : public idPhysics_Static {
public:
	CLASS_PROTOTYPE( idPhysics_Liquid );

						idPhysics_Liquid( void );
	virtual				~idPhysics_Liquid( void ) override;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

public:
	// Creates a splash on the liquid
	virtual void		Splash( idEntity *other, float volume, impactInfo_t &info, trace_t &collision );

	// Derived information
	virtual bool		isInLiquid( const idVec3 &point ) const;
	virtual idVec3		GetDepth( const idVec3 &point ) const;
	virtual idVec3		GetPressure( const idVec3 &point ) const;

	// Physical properties
	virtual float		GetDensity() const;
	virtual void		SetDensity( float density );

	virtual float		GetViscosity() const;
	virtual void		SetViscosity( float viscosity );

	virtual const idVec3 &GetMinSplashVelocity() const;
	virtual void		SetMinSplashVelocity( const idVec3 &m );

	virtual const idVec3 &GetMinWaveVelocity() const;
	virtual void		SetMinWaveVelocity( const idVec3 &w );

private:
	// STATE
	float				density;
	float				viscosity;

	idVec3				minWaveVelocity;
	idVec3				minSplashVelocity;
};

#endif

#endif
