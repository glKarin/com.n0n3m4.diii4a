
#ifndef __SURFACE_SWEPTSPLINE_H__
#define __SURFACE_SWEPTSPLINE_H__

/*
===============================================================================

	Swept Spline surface.

===============================================================================
*/

class idSurface_SweptSpline : public idSurface {
public:
							idSurface_SweptSpline( void );
							~idSurface_SweptSpline( void );

	void					SetSpline( idCurve_Spline<idVec4> *spline );
	void					SetSweptSpline( idCurve_Spline<idVec4> *sweptSpline );
	void					SetSweptCircle( const float radius );

	void					Tessellate( const int splineSubdivisions, const int sweptSplineSubdivisions );

	void					Clear( void );

protected:
	idCurve_Spline<idVec4> *spline;
	idCurve_Spline<idVec4> *sweptSpline;

	void					GetFrame( const idMat3 &previousFrame, const idVec3 dir, idMat3 &newFrame );
};

/*
====================
idSurface_SweptSpline::idSurface_SweptSpline
====================
*/
ID_INLINE idSurface_SweptSpline::idSurface_SweptSpline( void ) {
	spline = NULL;
	sweptSpline = NULL;
}

/*
====================
idSurface_SweptSpline::~idSurface_SweptSpline
====================
*/
ID_INLINE idSurface_SweptSpline::~idSurface_SweptSpline( void ) {
	delete spline;
	delete sweptSpline;
}

/*
====================
idSurface_SweptSpline::Clear
====================
*/
ID_INLINE void idSurface_SweptSpline::Clear( void ) {
	idSurface::Clear();
	delete spline;
	spline = NULL;
	delete sweptSpline;
	sweptSpline = NULL;
}

#endif /* !__SURFACE_SWEPTSPLINE_H__ */
