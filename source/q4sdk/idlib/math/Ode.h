
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
	virtual				~idODE_Euler( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

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
	virtual				~idODE_Midpoint( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

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
	virtual				~idODE_RK4( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );

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
	virtual				~idODE_RK4Adaptive( void );

	virtual float		Evaluate( const float *state, float *newState, float t0, float t1 );
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
