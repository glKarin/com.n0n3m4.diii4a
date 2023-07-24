// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __COLOR_H__
#define __COLOR_H__

class sdColor3 {
public:	
	float			r;
	float			g;
	float			b;

					sdColor3();
	explicit		sdColor3( const float r, const float g, const float b );
	explicit		sdColor3( const idVec3& rhs );

	void 			Set( const float r, const float g, const float b );
	void			Zero();

	void			Normalize( float scaleFactor = 1.0f );

	void			ToBytes( byte* bytes ) const;
	void			FromBytes( byte* bytes );

	idVec3&			ToVec3();
	const idVec3&	ToVec3() const;


	int				GetDimension() const;

	float			operator[]( const int index ) const;
	float& 			operator[]( const int index );
	sdColor3		operator-() const;
	sdColor3		operator*( const float rhs ) const;
	sdColor3		operator/( const float rhs ) const;
	sdColor3		operator+( const sdColor3& rhs ) const;
	sdColor3		operator-( const sdColor3& rhs ) const;
	sdColor3& 		operator+=( const sdColor3& rhs );
	sdColor3& 		operator-=( const sdColor3& rhs );
	sdColor3& 		operator/=( const sdColor3& rhs );
	sdColor3& 		operator/=( const float rhs );
	sdColor3& 		operator*=( const float rhs );	

	sdColor3& 		operator=( const idVec3& rhs );

	friend sdColor3	operator*( const float a1, const sdColor3 b1 );

	bool			Compare( const sdColor3& rhs ) const;							// exact compare, no epsilon
	bool			Compare( const sdColor3& rhs, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const sdColor3& rhs ) const;						// exact compare, no epsilon
	bool			operator!=(	const sdColor3& rhs ) const;						// exact compare, no epsilon

	const float*	ToFloatPtr() const;
	float*			ToFloatPtr();
	const char*		ToString( int precision = 2 ) const;

	void			Lerp( const sdColor3& v1, const sdColor3& v2, const float l );

	// packs color floats in the range [0,1] into an integer
	static dword		PackColor( const idVec3& color );
	static void			UnpackColor( const dword color, idVec3& unpackedColor );

	static	const sdColor3 black;
	static	const sdColor3 white;
	static	const sdColor3 red;
	static	const sdColor3 green;
	static	const sdColor3 blue;
	static	const sdColor3 yellow;
	static	const sdColor3 magenta;
	static	const sdColor3 cyan;
	static	const sdColor3 orange;
	static	const sdColor3 purple;
	static	const sdColor3 pink;
	static	const sdColor3 brown;
	static	const sdColor3 ltGrey;
	static	const sdColor3 mdGrey;
	static	const sdColor3 dkGrey;

	static	const sdColor3 ltBlue;
	static	const sdColor3 dkRed;
};

ID_INLINE sdColor3::sdColor3() {
}

ID_INLINE sdColor3::sdColor3( const float r, const float g, const float b ) {
	this->r = r;
	this->g = g;
	this->b = b;
}

ID_INLINE sdColor3::sdColor3( const idVec3& rhs ) {
	this->r = rhs.x;
	this->g = rhs.y;
	this->b = rhs.z;
}

ID_INLINE void sdColor3::ToBytes( byte* bytes ) const {
	for( int i = 0; i < GetDimension(); i++ ) {
		bytes[ i ] = idMath::Ftob( (*this)[ i ] * 255.0f );
	}
}

ID_INLINE void sdColor3::FromBytes( byte* bytes ) {
	for( int i = 0; i < GetDimension(); i++ ) {
		(*this)[ i ] = bytes[ i ] / 255.0f;
	}
}

ID_INLINE void sdColor3::Set( const float r, const float g, const float b ) {
	this->r = r;
	this->g = g;
	this->b = b;
}

ID_INLINE void sdColor3::Normalize( float scaleFactor ) {
	r = idMath::ClampFloat( 0.0f, 1.0f, r * scaleFactor );
	g = idMath::ClampFloat( 0.0f, 1.0f, g * scaleFactor );
	b = idMath::ClampFloat( 0.0f, 1.0f, b * scaleFactor );
}

ID_INLINE void sdColor3::Zero() {
	r = g = b = 0.0f;
}

ID_INLINE int sdColor3::GetDimension() const {
	return 3;
}

ID_INLINE float sdColor3::operator[]( int index ) const {
	assert( index >= 0 && index < 3 );
	return ( &r )[ index ];
}

ID_INLINE float& sdColor3::operator[]( int index ) {
	assert( index >= 0 && index < 3 );
	return ( &r )[ index ];
}

ID_INLINE sdColor3 sdColor3::operator-() const {
	return sdColor3( -r, -g, -b );
}

ID_INLINE sdColor3 sdColor3::operator-( const sdColor3& rhs ) const {
	return sdColor3( r - rhs.r, g - rhs.g, b - rhs.b );
}

ID_INLINE sdColor3 sdColor3::operator*( const float rhs ) const {
	return sdColor3( r * rhs, g * rhs, b * rhs );
}

ID_INLINE sdColor3 sdColor3::operator/( const float rhs ) const {
	float inva = 1.0f / rhs;
	return sdColor3( r * inva, g * inva, b * inva );
}

ID_INLINE sdColor3 operator*( const float a1, const sdColor3 b1 ) {
	return sdColor3( b1.r * a1, b1.g * a1, b1.b * a1 );
}

ID_INLINE sdColor3 sdColor3::operator+( const sdColor3& rhs ) const {
	return sdColor3( r + rhs.r, g + rhs.g, b + rhs.b );
}

ID_INLINE sdColor3& sdColor3::operator+=( const sdColor3& rhs ) {
	r += rhs.r;
	g += rhs.g;
	b += rhs.b;

	return *this;
}

ID_INLINE sdColor3& sdColor3::operator/=( const sdColor3& rhs ) {
	r /= rhs.r;
	g /= rhs.g;
	b /= rhs.b;

	return *this;
}

ID_INLINE sdColor3& sdColor3::operator/=( const float rhs ) {
	float inva = 1.0f / rhs;
	r *= inva;
	g *= inva;
	b *= inva;

	return *this;
}

ID_INLINE sdColor3& sdColor3::operator-=( const sdColor3& rhs ) {
	r -= rhs.r;
	g -= rhs.g;
	b -= rhs.b;

	return *this;
}

ID_INLINE sdColor3& sdColor3::operator*=( const float rhs ) {
	r *= rhs;
	g *= rhs;
	b *= rhs;

	return *this;
}

ID_INLINE bool sdColor3::Compare( const sdColor3& rhs ) const {
	return ( ( r == rhs.r ) && ( g == rhs.g ) && ( b == rhs.b ) );
}

ID_INLINE bool sdColor3::Compare( const sdColor3& rhs, const float epsilon ) const {
	if ( idMath::Fabs( r - rhs.r ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( g - rhs.g ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( b - rhs.b ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool sdColor3::operator==( const sdColor3& rhs ) const {
	return Compare( rhs );
}

ID_INLINE bool sdColor3::operator!=( const sdColor3& rhs ) const {
	return !Compare( rhs );
}

ID_INLINE const float* sdColor3::ToFloatPtr() const {
	return &r;
}

ID_INLINE float* sdColor3::ToFloatPtr() {
	return &r;
}

ID_INLINE sdColor3& sdColor3::operator=( const idVec3& rhs ) {
	r = rhs.x;
	g = rhs.y;
	b = rhs.z;
	return *this;
}

ID_INLINE const idVec3&	sdColor3::ToVec3() const {
	return *reinterpret_cast< const idVec3 * >( this );
}

ID_INLINE idVec3&	sdColor3::ToVec3() {
	return *reinterpret_cast< idVec3 * >( this );
}





class sdColor4 {
public:	
	float			r;
	float			g;
	float			b;
	float			a;

					sdColor4();
	explicit		sdColor4( const float r, const float g, const float b, const float a );
	explicit		sdColor4( const idVec4& rhs );

	void 			Set( const float r, const float g, const float b, const float a );
	void			Zero();

	void			Normalize( float scaleFactor = 1.0f );

	int				GetDimension() const;

	void			ToBytes( byte* bytes ) const;
	void			FromBytes( byte* bytes );

	idVec4&			ToVec4();
	const idVec4&	ToVec4() const;

	float			operator[]( const int index ) const;
	float& 			operator[]( const int index );
	sdColor4		operator-() const;
	sdColor4		operator*( const float rhs ) const;
	sdColor4		operator/( const float rhs ) const;
	sdColor4		operator+( const sdColor4& rhs ) const;
	sdColor4		operator-( const sdColor4& rhs ) const;
	sdColor4& 		operator+=( const sdColor4& rhs );
	sdColor4& 		operator-=( const sdColor4& rhs );
	sdColor4& 		operator/=( const sdColor4& rhs );
	sdColor4& 		operator/=( const float rhs );
	sdColor4& 		operator*=( const float rhs );	

	sdColor4& 		operator=( const idVec4& rhs );

	friend sdColor4	operator*( const float a1, const sdColor4 b1 );

	bool			Compare( const sdColor4& rhs ) const;							// exact compare, no epsilon
	bool			Compare( const sdColor4& rhs, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const sdColor4& rhs ) const;						// exact compare, no epsilon
	bool			operator!=(	const sdColor4& rhs ) const;						// exact compare, no epsilon

	const float*	ToFloatPtr() const;
	float*			ToFloatPtr();
	const char*		ToString( int precision = 2 ) const;

	void			Lerp( const sdColor4& v1, const sdColor4& v2, const float l );

	static dword	PackColor( const idVec3& color, float alpha );
	static dword	PackColor( const idVec4& color );
	static void		UnpackColor( const dword color, idVec4& unpackedColor );

	const sdColor3&	ToColor3() const;
	sdColor3&		ToColor3();

	static	const sdColor4 black;
	static	const sdColor4 white;
	static	const sdColor4 red;
	static	const sdColor4 green;
	static	const sdColor4 blue;
	static	const sdColor4 yellow;
	static	const sdColor4 magenta;
	static	const sdColor4 cyan;
	static	const sdColor4 orange;
	static	const sdColor4 purple;
	static	const sdColor4 pink;
	static	const sdColor4 brown;
	static	const sdColor4 ltGrey;
	static	const sdColor4 mdGrey;
	static	const sdColor4 dkGrey;

	static	const sdColor4 ltBlue;
	static	const sdColor4 dkRed;
};

ID_INLINE sdColor4::sdColor4() {
}

ID_INLINE sdColor4::sdColor4( const float r, const float g, const float b, const float a ) {
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

ID_INLINE sdColor4::sdColor4( const idVec4& rhs ) {
	this->r = rhs.x;
	this->g = rhs.y;
	this->b = rhs.z;
	this->a = rhs.w;
}

ID_INLINE void sdColor4::ToBytes( byte* bytes ) const {
	for( int i = 0; i < GetDimension(); i++ ) {
		bytes[ i ] = idMath::Ftob( (*this)[ i ] * 255.0f );
	}
}

ID_INLINE void sdColor4::FromBytes( byte* bytes ) {
	for( int i = 0; i < GetDimension(); i++ ) {
		(*this)[ i ] = bytes[ i ] / 255.0f;
	}
}

ID_INLINE void sdColor4::Set( const float r, const float g, const float b, const float a ) {
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

ID_INLINE void sdColor4::Normalize( float scaleFactor ) {
	r = idMath::ClampFloat( 0.0f, 1.0f, r * scaleFactor );
	g = idMath::ClampFloat( 0.0f, 1.0f, g * scaleFactor );
	b = idMath::ClampFloat( 0.0f, 1.0f, b * scaleFactor );
	a = idMath::ClampFloat( 0.0f, 1.0f, a * scaleFactor );
}

ID_INLINE void sdColor4::Zero() {
	r = g = b = a = 0.0f;
}

ID_INLINE int sdColor4::GetDimension() const {
	return 4;
}

ID_INLINE float sdColor4::operator[]( int index ) const {
	assert( index >= 0 && index < 4 );
	return ( &r )[ index ];
}

ID_INLINE float& sdColor4::operator[]( int index ) {
	assert( index >= 0 && index < 4 );
	return ( &r )[ index ];
}

ID_INLINE sdColor4 sdColor4::operator-() const {
	return sdColor4( -r, -g, -b, -a );
}

ID_INLINE sdColor4 sdColor4::operator-( const sdColor4& rhs ) const {
	return sdColor4( r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a );
}

ID_INLINE sdColor4 sdColor4::operator*( const float rhs ) const {
	return sdColor4( r * rhs, g * rhs, b * rhs, a * rhs );
}

ID_INLINE sdColor4 sdColor4::operator/( const float rhs ) const {
	float inva = 1.0f / rhs;
	return sdColor4( r * inva, g * inva, b * inva, a * inva );
}

ID_INLINE sdColor4 operator*( const float a1, const sdColor4 b1 ) {
	return sdColor4( b1.r * a1, b1.g * a1, b1.b * a1, b1.a * a1 );
}

ID_INLINE sdColor4 sdColor4::operator+( const sdColor4& rhs ) const {
	return sdColor4( r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a );
}

ID_INLINE sdColor4& sdColor4::operator+=( const sdColor4& rhs ) {
	r += rhs.r;
	g += rhs.g;
	b += rhs.b;
	a += rhs.a;

	return *this;
}

ID_INLINE sdColor4& sdColor4::operator/=( const sdColor4& rhs ) {
	r /= rhs.r;
	g /= rhs.g;
	b /= rhs.b;
	a /= rhs.a;

	return *this;
}

ID_INLINE sdColor4& sdColor4::operator/=( const float rhs ) {
	float inva = 1.0f / rhs;
	r *= inva;
	g *= inva;
	b *= inva;
	a *= inva;

	return *this;
}

ID_INLINE sdColor4& sdColor4::operator-=( const sdColor4& rhs ) {
	r -= rhs.r;
	g -= rhs.g;
	b -= rhs.b;
	a -= rhs.a;

	return *this;
}

ID_INLINE sdColor4& sdColor4::operator*=( const float rhs ) {
	r *= rhs;
	g *= rhs;
	b *= rhs;
	a *= rhs;

	return *this;
}

ID_INLINE bool sdColor4::Compare( const sdColor4& rhs ) const {
	return ( ( r == rhs.r ) && ( g == rhs.g ) && ( b == rhs.b ) && a == rhs.a );
}

ID_INLINE bool sdColor4::Compare( const sdColor4& rhs, const float epsilon ) const {
	if ( idMath::Fabs( r - rhs.r ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( g - rhs.g ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( b - rhs.b ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( a - rhs.a ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool sdColor4::operator==( const sdColor4& rhs ) const {
	return Compare( rhs );
}

ID_INLINE bool sdColor4::operator!=( const sdColor4& rhs ) const {
	return !Compare( rhs );
}

ID_INLINE const float* sdColor4::ToFloatPtr() const {
	return &r;
}

ID_INLINE float* sdColor4::ToFloatPtr() {
	return &r;
}

ID_INLINE sdColor4& sdColor4::operator=( const idVec4& rhs ) {
	r = rhs.x;
	g = rhs.y;
	b = rhs.z;
	a = rhs.w;
	return *this;
}

ID_INLINE const sdColor3 &sdColor4::ToColor3() const {
	return *reinterpret_cast< const sdColor3 * >( this );
}

ID_INLINE sdColor3 &sdColor4::ToColor3() {
	return *reinterpret_cast< sdColor3 * >( this );
}

ID_INLINE const idVec4&	sdColor4::ToVec4() const {
	return *reinterpret_cast< const idVec4 * >( this );
}

ID_INLINE idVec4&	sdColor4::ToVec4() {
	return *reinterpret_cast< idVec4 * >( this );
}


#endif // __COLOR_H__
