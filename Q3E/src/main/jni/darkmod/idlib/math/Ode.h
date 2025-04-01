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

#ifndef __MATH_ODE_H__
#define __MATH_ODE_H__

/*
===============================================================================

	Numerical solvers for ordinary differential equations.

===============================================================================
*/


//===============================================================
//
//	idODE
//
//===============================================================

typedef void (*deriveFunction_t)( const float t, const void *userData, const float *state, float *derivatives );

class idODE {

public:
	virtual				~idODE( void ) {}

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 ) = 0;

protected:
	int					dimension;		// dimension in floats allocated for
	deriveFunction_t	derive;			// derive function
	const void *		userData;		// client data
};

//===============================================================
//
//	idODE_Euler
//
//===============================================================

class idODE_Euler : public idODE {

public:
						idODE_Euler( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~idODE_Euler( void ) override;

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 ) override;

protected:
	float *				derivatives;	// space to store derivatives
};

//===============================================================
//
//	idODE_Midpoint
//
//===============================================================

class idODE_Midpoint : public idODE {

public:
						idODE_Midpoint( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~idODE_Midpoint( void ) override;

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 ) override;

protected:
	float *				tmpState;
	float *				derivatives;	// space to store derivatives
};

//===============================================================
//
//	idODE_RK4
//
//===============================================================

class idODE_RK4 : public idODE {

public:
						idODE_RK4( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~idODE_RK4( void ) override;

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 ) override;

protected:
	float *				tmpState;
	float *				d1;				// derivatives
	float *				d2;
	float *				d3;
	float *				d4;
};

//===============================================================
//
//	idODE_RK4Adaptive
//
//===============================================================

class idODE_RK4Adaptive : public idODE {

public:
						idODE_RK4Adaptive( const int dim, const deriveFunction_t dr, const void *ud );
	virtual				~idODE_RK4Adaptive( void ) override;

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 ) override;
	void				SetMaxError( const float err );

protected:
	float				maxError;		// maximum allowed error
	float *				tmpState;
	float *				d1;				// derivatives
	float *				d1half;
	float *				d2;
	float *				d3;
	float *				d4;
};

#endif /* !__MATH_ODE_H__ */
