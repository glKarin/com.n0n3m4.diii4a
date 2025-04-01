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

#include "precompiled.h"
#pragma hdrstop



#include "../Game_local.h"

#ifdef MOD_WATERPHYSICS

CLASS_DECLARATION( idPhysics_Static, idPhysics_Liquid )
END_CLASS

/*
===============================================================================

	idPhysics_Liquid

===============================================================================
*/

/*
================
idPhysics_Liquid::idPhysics_Liquid
================
*/
idPhysics_Liquid::idPhysics_Liquid() {

	// initializes to a water-like liquid
	this->density = 0.001043f;
	this->viscosity = 3.0f;
}

/*
================
idPhysics_Liquid::~idPhysics_Liquid
================
*/
idPhysics_Liquid::~idPhysics_Liquid() {
}

/*
================
idPhysics_Liquid::Save
================
*/
void idPhysics_Liquid::Save( idSaveGame *savefile ) const
{
	savefile->WriteFloat(this->density);
	savefile->WriteFloat(this->viscosity);
	savefile->WriteVec3(this->minSplashVelocity);
	savefile->WriteVec3(this->minWaveVelocity);
}

/*
================
idPhysics_Liquid::Restore
================
*/
void idPhysics_Liquid::Restore( idRestoreGame *savefile )
{
	savefile->ReadFloat(this->density);
	savefile->ReadFloat(this->viscosity);
	savefile->ReadVec3(this->minSplashVelocity);
	savefile->ReadVec3(this->minWaveVelocity);
}

/*
================
idPhysics_Liquid::GetDepth
	Gets the depth of a point in the liquid.  Returns -1 -1 -1 if the object is not in the liquid
================
*/
idVec3 idPhysics_Liquid::GetDepth( const idVec3 &point ) const {
	const idBounds &bounds = this->GetBounds();
	idVec3 gravityNormal = this->GetGravityNormal();
	idVec3 depth(-1.0f,-1.0f,-1.0f);

	if( !this->isInLiquid(point) )
		return depth;
	depth = (((bounds[1] + this->GetOrigin()) - point) * gravityNormal) * gravityNormal;

	return depth;
}

/*
================
idPhysics_Liquid::Splash
	Causes the liquid to splash but only if the velocity is greater than minSplashVelocity
================
*/
void idPhysics_Liquid::Splash( idEntity *other, float volume, impactInfo_t &info, trace_t &collision )
{
	int num = collision.c.entityNum; // grayman #1104
	collision.c.entityNum = other->entityNumber;
	self->Collide(collision,info.velocity);
	collision.c.entityNum = num; // grayman #1104 - restore original entityNum
}

/*
================
idPhysics_Liquid::isInLiquid
	Returns true if a point is in the liquid
================
*/
bool idPhysics_Liquid::isInLiquid( const idVec3 &point ) const {
	bool result;

	result = (gameLocal.clip.Contents(point,NULL,mat3_identity,MASK_WATER,NULL) != 0);
	return result;
}

/*
================
idPhysics_Liquid::GetPressure
	Returns the amount of pressure the liquid applies to the given point
================
*/
idVec3 idPhysics_Liquid::GetPressure( const idVec3 &point ) const {
	idVec3 pressure;
	idVec3 depth = this->GetDepth(point);
//NJM	idVec3 &depth = this->GetDepth(point);

	pressure = depth * this->density;

	return pressure;
}

/*
================
idPhysics_Liquid::GetDensity
================
*/
float idPhysics_Liquid::GetDensity() const {
	return this->density;
}

/*
================
idPhysics_Liquid::SetDensity
================
*/
void idPhysics_Liquid::SetDensity( float density ) {
	if( density > 0.0f )
		this->density = density;
}

/*
================
idPhysics_Liquid::GetViscosity
================
*/
float idPhysics_Liquid::GetViscosity() const  {
	return this->viscosity;
}

/*
================
idPhysics_Liquid::SetViscosity
================
*/
void idPhysics_Liquid::SetViscosity( float viscosity ) {
	if( viscosity >= 0.0f )	
		this->viscosity = viscosity;
}

/*
================
idPhysics_Liquid::GetMinSplashVelocity
================
*/
const idVec3 &idPhysics_Liquid::GetMinSplashVelocity() const {
	return this->minSplashVelocity;
}

/*
================
idPhysics_Liquid::SetMinSplashVelocity
================
*/
void idPhysics_Liquid::SetMinSplashVelocity( const idVec3 &m ) {
	this->minSplashVelocity  = m;
}

/*
================
idPhysics_Liquid::GetMinWaveVelocity
================
*/
const idVec3 &idPhysics_Liquid::GetMinWaveVelocity() const {
	return this->minWaveVelocity;
}

/*
================
idPhysics_Liquid::SetMinWaveVelocity
================
*/
void idPhysics_Liquid::SetMinWaveVelocity( const idVec3 &w ) {
	this->minWaveVelocity  = w;
}

#endif

