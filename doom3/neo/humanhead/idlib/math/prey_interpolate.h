
#ifndef __PREY_INTERPOLATE_H__
#define __PREY_INTERPOLATE_H__

//==============================================================================================
//
//	Hermite interpolation.
//
//==============================================================================================

template< class type >
class hhHermiteInterpolate {
public:
						hhHermiteInterpolate();
	void				Init( const int startTime, const int duration, const type startValue, const type endValue, float S1, float S2 );
	void				Init( const int startTime, const int duration, const type startValue, const type endValue );
	void				SetStartTime( int time ) { this->startTime = time; }
	void				SetDuration( int duration ) { this->duration = duration; }
	void				SetStartValue( const type &start ) { this->startValue = start; }
	void				SetEndValue( const type &end ) { this->endValue = end; }
	void				SetHermiteParms( float S1, float S2 ) { this->S1 = S1; this->S2 = S2; }
	type				GetCurrentValue( int time ) const;
	bool				IsDone( int time ) const { return ( time >= startTime + duration ); }
	int					GetStartTime( void ) const { return startTime; }
	int					GetDuration( void ) const { return duration; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }
	float				GetS1( void ) const { return S1; }
	float				GetS2( void ) const { return S2; }
	float				HermiteAlpha(float t) const;

private:
	float				S1;				// Slope of curve leaving start point
	float				S2;				// Slope of curve arriving at end point
	int					startTime;
	int					duration;
	type				startValue;
	type				endValue;
	mutable int			currentTime;
	mutable type		currentValue;
};

/*
====================
hhHermiteInterpolate::hhHermiteInterpolate
====================
*/
template< class type >
ID_INLINE hhHermiteInterpolate<type>::hhHermiteInterpolate() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
	S1 = S2 = 1;
}

/*
====================
hhHermiteInterpolate::Init
====================
*/
template< class type >
ID_INLINE void hhHermiteInterpolate<type>::Init( const int startTime, const int duration, const type startValue, const type endValue, const float S1, const float S2 ) {
	this->S1 = S1;
	this->S2 = S2;
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
hhHermiteInterpolate::Init
====================
*/
template< class type >
ID_INLINE void hhHermiteInterpolate<type>::Init( const int startTime, const int duration, const type startValue, const type endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

/*
====================
hhHermiteInterpolate::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type hhHermiteInterpolate<type>::GetCurrentValue( int time ) const {
	int deltaTime;

	deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( deltaTime <= 0 ) {
			currentValue = startValue;
		}
		else if ( deltaTime >= duration ) {
			currentValue = endValue;
		}
		else {
			currentValue = startValue + ( endValue - startValue ) * HermiteAlpha( (float) deltaTime / duration );
		}
	}
	return currentValue;
}

// Hermite()
// Hermite Interpolator
// Returns an alpha value [0..1] based on Hermite Parameters N1, N2, S1, S2 and an input alpha 't'
template< class type >
ID_INLINE float hhHermiteInterpolate<type>::HermiteAlpha(const float t) const {
	float N1 = 0.0f;
	float N2 = 1.0f;
	float tSquared = t*t;
	float tCubed = tSquared*t;
	return	(2*tCubed - 3*tSquared + 1)*N1 +
			(-2*tCubed + 3*tSquared)*N2 +
			(tCubed - 2*tSquared + t)*S1 +
			(tCubed - tSquared)*S2;
}

//==============================================================================================
//
// TCB Spline Interpolation
//
// Defines a Kochanek-Bartels spline, basically a Hermite spline with formulae to calculate the tangents
// Requires extra points at the ends, try duplicating first and last
//==============================================================================================
class hhTCBSpline {
	//TODO: Make a template like the others so it can handle something other than vec3 types
public:
					hhTCBSpline()	{	Clear();	}
	void			Clear();
	void			AddPoint(const idVec3 &point);
	void			SetControls(float tension, float continuity, float bias);
	idVec3			GetValue(float alpha);

	float			tension;		// How sharply does the curve bend?
	float			continuity;		// How rapid is the change in speed and direction?
	float			bias;			// What is the direction of the curve as it passes through the key point?
	idList<idVec3>	nodes;			// control points

protected:
	idVec3			GetNode(int i);
	idVec3			IncomingTangent(int i);
	idVec3			OutgoingTangent(int i);
};

ID_INLINE void hhTCBSpline::Clear() {
	tension = continuity = bias = 0.0f;
	nodes.Clear();
}

ID_INLINE void hhTCBSpline::AddPoint(const idVec3 &point) {
	nodes.Append(point);
}

ID_INLINE void hhTCBSpline::SetControls(float tension, float continuity, float bias) {
	this->tension = idMath::ClampFloat(0.0f, 1.0f, tension);
	this->continuity = idMath::ClampFloat(0.0f, 1.0f, continuity);
	this->bias = idMath::ClampFloat(0.0f, 1.0f, bias);
}

ID_INLINE idVec3 hhTCBSpline::GetNode(int i) {
	// Clamping has the effect of having duplicate nodes beyond the array boundaries
	int index = idMath::ClampInt(0, nodes.Num()-1, i);
	return nodes[index];
}

ID_INLINE idVec3 hhTCBSpline::IncomingTangent(int i) {
	return	((1.0f-tension)*(1.0f-continuity)*(1.0f+bias) * 0.5f) * (GetNode(i) - GetNode(i-1)) +
			((1.0f-tension)*(1.0f+continuity)*(1.0f-bias) * 0.5f) * (GetNode(i+1) - GetNode(i));
}

ID_INLINE idVec3 hhTCBSpline::OutgoingTangent(int i) {
	return	((1.0f-tension)*(1.0f+continuity)*(1.0f+bias) * 0.5f) * (GetNode(i) - GetNode(i-1)) +
			((1.0f-tension)*(1.0f-continuity)*(1.0f-bias) * 0.5f) * (GetNode(i+1) - GetNode(i));
}

ID_INLINE idVec3 hhTCBSpline::GetValue(float alpha) {
	float t = idMath::ClampFloat(0.0f, 1.0f, alpha);
	int numNodes = nodes.Num();
	int numSegments = numNodes-1;
	int startNode = t * numSegments;
	t = (t * numSegments) - startNode;		// t = alpha within this segment

	// Calculate hermite parameters
	idVec3 N1 = GetNode(startNode);
	idVec3 N2 = GetNode(startNode+1);
	idVec3 S1 = OutgoingTangent(startNode);
	idVec3 S2 = IncomingTangent(startNode+1);

	float tSquared = t*t;
	float tCubed = tSquared*t;
	return	(2*tCubed - 3*tSquared + 1)*N1 +
			(-2*tCubed + 3*tSquared)*N2 +
			(tCubed - 2*tSquared + t)*S1 +
			(tCubed - tSquared)*S2;
}


//==============================================================================================
//
//	Sawtooth interpolation.
//
//	Interpolates from startValue to endValue to startValue over duration
//==============================================================================================

template< class type >
class hhSawToothInterpolate {
public:
						hhSawToothInterpolate();
	void				Init( const int startTime, const int duration, const type startValue, const type endValue );
	void				SetStartTime( int time ) { this->startTime = time; }
	void				SetDuration( int duration ) { this->duration = duration; }
	void				SetStartValue( const type &start ) { this->startValue = start; }
	void				SetEndValue( const type &end ) { this->endValue = end; }
	type				GetCurrentValue( int time ) const;
	bool				IsDone( int time ) const { return ( time >= startTime + duration ); }
	int					GetStartTime( void ) const { return startTime; }
	int					GetDuration( void ) const { return duration; }
	const type &		GetStartValue( void ) const { return startValue; }
	const type &		GetEndValue( void ) const { return endValue; }

private:
	int					startTime;
	int					duration;
	type				startValue;
	type				endValue;
	mutable int			currentTime;
	mutable type		currentValue;
};

template< class type >
ID_INLINE hhSawToothInterpolate<type>::hhSawToothInterpolate() {
	currentTime = startTime = duration = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	startValue = endValue = currentValue;
}

template< class type >
ID_INLINE void hhSawToothInterpolate<type>::Init( const int startTime, const int duration, const type startValue, const type endValue ) {
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->endValue = endValue;
	this->currentTime = startTime - 1;
	this->currentValue = startValue;
}

template< class type >
ID_INLINE type hhSawToothInterpolate<type>::GetCurrentValue( int time ) const {
	int deltaTime;

	deltaTime = time - startTime;
	if ( time != currentTime ) {
		currentTime = time;
		if ( deltaTime <= 0 ) {
			currentValue = startValue;
		}
		else if ( deltaTime >= duration ) {
			currentValue = startValue;
		}
		else {
			float frac = ((float) deltaTime / duration );
			if (frac < 0.5f) {
				currentValue = startValue + ( endValue - startValue ) * frac * 2.0f;
			}
			else {
				currentValue = startValue + ( endValue - startValue ) * (1.0f - frac) * 2.0f;
			}
		}
	}
	return currentValue;
}


//==============================================================================================
//
//	Sine wave oscillator
//
//	Oscillates between min and max over given period
//==============================================================================================

template< class type >
class hhSinOscillator {
public:
						hhSinOscillator();
	void				Init( const int startTime, const int period, const type min, const type max );
	void				SetStartTime( int time ) { this->startTime = time; }
	void				SetPeriod( int period ) { this->period = period; }
	void				SetMinValue( const type &min ) { this->minValue = min; }
	void				SetMaxValue( const type &max ) { this->MaxValue = max; }
	type				GetCurrentValue( int time ) const;
	int					GetStartTime( void ) const { return startTime; }
	int					GetPeriod( void ) const { return period; }
	const type &		GetMinValue( void ) const { return minValue; }
	const type &		GetMaxValue( void ) const { return maxValue; }

private:
	int					startTime;
	int					period;
	type				minValue;
	type				maxValue;
	mutable int			currentTime;
	mutable type		currentValue;
};

template< class type >
ID_INLINE hhSinOscillator<type>::hhSinOscillator() {
	currentTime = startTime = period = 0;
	memset( &currentValue, 0, sizeof( currentValue ) );
	minValue = maxValue = currentValue;
}

template< class type >
ID_INLINE void hhSinOscillator<type>::Init( const int startTime, const int period, const type minValue, const type maxValue ) {
	this->startTime = startTime;
	this->period = period;
	this->minValue = minValue;
	this->maxValue = maxValue;
	this->currentTime = startTime - 1;
	this->currentValue = minValue;
}

template< class type >
ID_INLINE type hhSinOscillator<type>::GetCurrentValue( int time ) const {

	if ( time != currentTime ) {
		currentTime = time;

		float deltaTime = period == 0 ? 0.0f : ( time - startTime ) / (float) period;
		float s = (1.0f + (float) sin(deltaTime * idMath::TWO_PI)) * 0.5f;
		currentValue = minValue + s * (maxValue - minValue);
	}
	return currentValue;
}


#endif	// __PREY_INTERPOLATE_H__