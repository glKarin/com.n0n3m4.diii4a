#ifndef _KARIN_INTERPOLATE_H
#define _KARIN_INTERPOLATE_H

ID_INLINE float SinusoidalMidPoint( float frac ) {
	return idMath::Sin( DEG2RAD(idMath::MidPointLerp(0.0f, 60.0f, 90.0f, frac)) ); 
}

/*
==============================================================================================

	Spherical interpolation.

==============================================================================================
*/
typedef float (*TimeManipFunc) ( float );
class rvSphericalInterpolate : public idInterpolate<idQuat> {
public:
						rvSphericalInterpolate();
	virtual idQuat		GetCurrentValue( float time ) const;

	void				SetTimeFunction( TimeManipFunc func ) { timeFunc = func; }

protected:
	TimeManipFunc		timeFunc;
};

/*
====================
rvSphericalInterpolate::rvSphericalInterpolate
====================
*/
ID_INLINE rvSphericalInterpolate::rvSphericalInterpolate() :
	idInterpolate<idQuat>() {
	SetTimeFunction( SinusoidalMidPoint );
}

/*
====================
rvSphericalInterpolate::GetCurrentValue
====================
*/
ID_INLINE idQuat rvSphericalInterpolate::GetCurrentValue( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if( duration == 0.0f ) {
			currentValue = endValue;
		} else {
			currentValue.Slerp( startValue, endValue, timeFunc((float)deltaTime / duration) );
		}
	}
	return currentValue;
}
// RAVEN END

#endif
