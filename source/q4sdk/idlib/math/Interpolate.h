
#ifndef __MATH_INTERPOLATE_H__
#define __MATH_INTERPOLATE_H__

/*
==============================================================================================

	Linear interpolation.

==============================================================================================
*/

template< class type >
class idInterpolate {
public:
						idInterpolate();
						virtual ~idInterpolate() { }

	void				Init( const float startTime, const float duration, const type &startValue, const type &endValue );
	void				SetStartTime( float time ) { this->startTime = time; }
	void				SetDuration( float duration ) { this->duration = duration; }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; }

// RAVEN BEGIN
// abahr: made virtual
	virtual type		GetCurrentValue( float time ) const;
	virtual type		GetDeltaValue( float startTime, float endTime ) const;
// RAVEN END
	bool				IsDone( float time ) const { return ( time >= startTime + duration ); }

	float				GetStartTime( void ) const { return startTime; }
	float				GetEndTime( void ) const { return startTime + duration; }
	float				GetDuration( void ) const { return duration; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

// RAVEN BEGIN
// abahr: changed to protected
protected:
// RAVEN END
	float				startTime;
	float				duration;
	type				startValue;
	type				endValue;
	mutable float		currentTime;
	mutable type		currentValue;
};

/*
====================
idInterpolate::idInterpolate
====================
*/
template< class type >
ID_INLINE idInterpolate<type>::idInterpolate() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
}

/*
====================
idInterpolate::Init
====================
*/
template< class type >
ID_INLINE void idInterpolate<type>::Init( const float startTime, const float duration, const type &startValue, const type &endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
idInterpolate::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type idInterpolate<type>::GetCurrentValue( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( deltaTime <= 0 ) {
			currentValue = startValue;
		} else if ( deltaTime >= duration ) {
			currentValue = endValue;
		} else {
			currentValue = startValue + ( endValue - startValue ) * ( (float) deltaTime / duration );
		}
	}
	return currentValue;
}

// RAVEN BEGIN
// abahr
/*
====================
idInterpolate::GetDeltaValue
====================
*/
template< class type >
ID_INLINE type idInterpolate<type>::GetDeltaValue( float startTime, float endTime ) const {
	return GetCurrentValue(endTime) - GetCurrentValue(startTime);
}

ID_INLINE float Linear( float frac ) {
	return frac;
}

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

/*
==============================================================================================

	Continuous interpolation with linear acceleration and deceleration phase.
	The velocity is continuous but the acceleration is not.

==============================================================================================
*/

template< class type >
class idInterpolateAccelDecelLinear  {
public:
						idInterpolateAccelDecelLinear();

	void				Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue );
	void				SetStartTime( float time ) { startTime = time; Invalidate(); }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; Invalidate(); }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; Invalidate(); }

	type				GetCurrentValue( float time ) const;
	type				GetCurrentSpeed( float time ) const;
	bool				IsDone( float time ) const { return ( time >= startTime + accelTime + linearTime + decelTime ); }

	float				GetStartTime( void ) const { return startTime; }
	float				GetEndTime( void ) const { return startTime + accelTime + linearTime + decelTime; }
	float				GetDuration( void ) const { return accelTime + linearTime + decelTime; }
	float				GetAcceleration( void ) const { return accelTime; }
	float				GetDeceleration( void ) const { return decelTime; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

private:
	float				startTime;
	float				accelTime;
	float				linearTime;
	float				decelTime;
	type				startValue;
	type				endValue;
	mutable idExtrapolate<type> extrapolate;

	void				Invalidate( void );
	void				SetPhase( float time ) const;
};

/*
====================
idInterpolateAccelDecelLinear::idInterpolateAccelDecelLinear
====================
*/
template< class type >
ID_INLINE idInterpolateAccelDecelLinear<type>::idInterpolateAccelDecelLinear() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
idInterpolateAccelDecelLinear::Init
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelLinear<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue ) {
	type speed;

	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if ( duration <= 0.0f ) {
		return;
	}

	if ( this->accelTime + this->decelTime > duration ) {
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	speed = ( endValue - startValue ) * ( 1000.0f / ( (float) this->linearTime + ( this->accelTime + this->decelTime ) * 0.5f ) );

	if ( this->accelTime ) {
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELLINEAR );
	} else if ( this->linearTime ) {
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR );
	} else {
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELLINEAR );
	}
}

/*
====================
idInterpolateAccelDecelLinear::Invalidate
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelLinear<type>::Invalidate( void ) {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
idInterpolateAccelDecelLinear::SetPhase
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelLinear<type>::SetPhase( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;
	if ( deltaTime < accelTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELLINEAR ) {
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELLINEAR );
		}
	} else if ( deltaTime < accelTime + linearTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR ) {
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * 0.5f ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	} else {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELLINEAR ) {
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * 0.5f ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELLINEAR );
		}
	}
}

/*
====================
idInterpolateAccelDecelLinear::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelLinear<type>::GetCurrentValue( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
idInterpolateAccelDecelLinear::GetCurrentSpeed
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelLinear<type>::GetCurrentSpeed( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}


/*
==============================================================================================

	Continuous interpolation with sinusoidal acceleration and deceleration phase.
	Both the velocity and acceleration are continuous.

==============================================================================================
*/

template< class type >
class idInterpolateAccelDecelSine  {
public:
						idInterpolateAccelDecelSine();

	void				Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue );
	void				SetStartTime( float time ) { startTime = time; Invalidate(); }
	void				SetStartValue( const type &startValue ) { this->startValue = startValue; Invalidate(); }
	void				SetEndValue( const type &endValue ) { this->endValue = endValue; Invalidate(); }

	type				GetCurrentValue( float time ) const;
	type				GetCurrentSpeed( float time ) const;
	bool				IsDone( float time ) const { return ( time >= startTime + accelTime + linearTime + decelTime ); }

	float				GetStartTime( void ) const { return startTime; }
	float				GetEndTime( void ) const { return startTime + accelTime + linearTime + decelTime; }
	float				GetDuration( void ) const { return accelTime + linearTime + decelTime; }
	float				GetAcceleration( void ) const { return accelTime; }
	float				GetDeceleration( void ) const { return decelTime; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

private:
	float				startTime;
	float				accelTime;
	float				linearTime;
	float				decelTime;
	type				startValue;
	type				endValue;
	mutable idExtrapolate<type> extrapolate;

	void				Invalidate( void );
	void				SetPhase( float time ) const;
};

/*
====================
idInterpolateAccelDecelSine::idInterpolateAccelDecelSine
====================
*/
template< class type >
ID_INLINE idInterpolateAccelDecelSine<type>::idInterpolateAccelDecelSine() {
	startTime = accelTime = linearTime = decelTime = 0;
	memset( &startValue, 0, sizeof( startValue ) );
	endValue = startValue;
}

/*
====================
idInterpolateAccelDecelSine::Init
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelSine<type>::Init( const float startTime, const float accelTime, const float decelTime, const float duration, const type &startValue, const type &endValue ) {
	type speed;

	this->startTime = startTime;
	this->accelTime = accelTime;
	this->decelTime = decelTime;
	this->startValue = startValue;
	this->endValue = endValue;

	if ( duration <= 0.0f ) {
		return;
	}

	if ( this->accelTime + this->decelTime > duration ) {
		this->accelTime = this->accelTime * duration / ( this->accelTime + this->decelTime );
		this->decelTime = duration - this->accelTime;
	}
	this->linearTime = duration - this->accelTime - this->decelTime;
	speed = ( endValue - startValue ) * ( 1000.0f / ( (float) this->linearTime + ( this->accelTime + this->decelTime ) * idMath::SQRT_1OVER2 ) );

	if ( this->accelTime ) {
		extrapolate.Init( startTime, this->accelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_ACCELSINE );
	} else if ( this->linearTime ) {
		extrapolate.Init( startTime, this->linearTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_LINEAR );
	} else {
		extrapolate.Init( startTime, this->decelTime, startValue, ( startValue - startValue ), speed, EXTRAPOLATION_DECELSINE );
	}
}

/*
====================
idInterpolateAccelDecelSine::Invalidate
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelSine<type>::Invalidate( void ) {
	extrapolate.Init( 0, 0, extrapolate.GetStartValue(), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_NONE );
}

/*
====================
idInterpolateAccelDecelSine::SetPhase
====================
*/
template< class type >
ID_INLINE void idInterpolateAccelDecelSine<type>::SetPhase( float time ) const {
	float deltaTime;

	deltaTime = time - startTime;
	if ( deltaTime < accelTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_ACCELSINE ) {
			extrapolate.Init( startTime, accelTime, startValue, extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_ACCELSINE );
		}
	} else if ( deltaTime < accelTime + linearTime ) {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_LINEAR ) {
			extrapolate.Init( startTime + accelTime, linearTime, startValue + extrapolate.GetSpeed() * ( accelTime * 0.001f * idMath::SQRT_1OVER2 ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_LINEAR );
		}
	} else {
		if ( extrapolate.GetExtrapolationType() != EXTRAPOLATION_DECELSINE ) {
			extrapolate.Init( startTime + accelTime + linearTime, decelTime, endValue - ( extrapolate.GetSpeed() * ( decelTime * 0.001f * idMath::SQRT_1OVER2 ) ), extrapolate.GetBaseSpeed(), extrapolate.GetSpeed(), EXTRAPOLATION_DECELSINE );
		}
	}
}

/*
====================
idInterpolateAccelDecelSine::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelSine<type>::GetCurrentValue( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentValue( time );
}

/*
====================
idInterpolateAccelDecelSine::GetCurrentSpeed
====================
*/
template< class type >
ID_INLINE type idInterpolateAccelDecelSine<type>::GetCurrentSpeed( float time ) const {
	SetPhase( time );
	return extrapolate.GetCurrentSpeed( time );
}

#endif /* !__MATH_INTERPOLATE_H__ */
