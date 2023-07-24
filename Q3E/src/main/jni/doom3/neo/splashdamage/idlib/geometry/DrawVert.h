// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DRAWVERT_H__
#define __DRAWVERT_H__

/*
===============================================================================

	Draw Vertex.

===============================================================================
*/
#pragma pack( push, 1 )

class idDrawVert {
public:

/*
	The sizeof idDrawVert should be at least a multiple of 16 bytes to make the VMX128 code work.
	Ideally the sizeof idDrawVert will be a power of two to make indexing fast.
*/

#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec3			xyz;
	byte			color[4];
	short			_normal[2];
	short			_tangent[2];
	byte			_signs[4];		// 0 == normal z, 1 == tangent.z, 2 == bitangent sign, 3 == unused
	short			_st[2];
#else
	idVec3			xyz;
	byte			color[4];
	idVec3			_normal;
	byte			_color2[4];
	idVec4			_tangent;		// [3] is texture polarity sign
	idVec2			_st;
	idVec2			_st2;
#endif

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

#if defined( SD_USE_DRAWVERT_SIZE_32 )
	const idVec3	GetNormal( void ) const;
#else
	const idVec3 &	GetNormal( void ) const;
#endif
	float			GetNormalIdx( int idx ) const;

	void			SetNormal( float x, float y, float z );
	void			SetNormal( const idVec3 &n );

#if defined( SD_USE_DRAWVERT_SIZE_32 )
	const idVec3 	GetTangent( void ) const;
	const idVec4 	GetTangentVec4( void ) const;
#else
	const idVec3 &	GetTangent( void ) const;
	const idVec4 & 	GetTangentVec4( void ) const;
#endif
	void			SetTangent( float x, float y, float z );
	void			SetTangent( const idVec3 &t );

	const idVec3 	GetBiTangent( void ) const;			// derived from normal, tangent, and tangent flag
	void			SetBiTangent( float x, float y, float z );
	void			SetBiTangent( const idVec3 &t );
	float			GetBiTangentSign( void ) const;
	void			SetBiTangentSign( float sign );		// either 1.0f or -1.0f

	void			SetColor( dword color );
	dword			GetColor( void ) const;

	float			GetSTIdx( int idx ) const;
	void			GetST( float &s, float &t ) const;
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec2			GetST( void ) const;
#else
	const idVec2 &	GetST( void ) const;
#endif
	void			SetST( bool lowrange, const idVec2 &st );
	void			SetST( float s, float t );
	void			SetST( const idVec2 &st );
	void			SetST( const idDrawVert &dv );
	void			SetSTIdx( int i, float v );

	float			GetZ( short x, short y, byte sign ) const;

	void			Normalize( void );

	void			Lerp( const idDrawVert &a, const idDrawVert &b, const float f );
	void			LerpAll( const idDrawVert &a, const idDrawVert &b, const float f );

	bool			operator==( const idDrawVert& rhs ) const;
};

#pragma pack( pop )

#define ST_TO_FLOAT	1.0f / 4096.0f
#define FLOAT_TO_ST	4096.0f
#define ST_TO_FLOAT_LOWRANGE	1.0f / 32767.0f
#define FLOAT_TO_ST_LOWRANGE	32767.0f



#if defined( SD_USE_DRAWVERT_SIZE_32 )

// offsets for SIMD code
#define DRAWVERT_SIZE				32			// sizeof( idDrawVert )
#define DRAWVERT_SIZE_SHIFT			5			// log2( sizeof( idDrawVert ) )
#define DRAWVERT_XYZ_OFFSET			(0*4)		// offsetof( idDrawVert, xyz )
#define DRAWVERT_NORMAL_OFFSET		(4*4)		// offsetof( idDrawVert, normal )
#define DRAWVERT_TANGENT_OFFSET		(5*4)		// offsetof( idDrawVert, tangent )

#else

// offsets for SIMD code
#define DRAWVERT_SIZE				64			// sizeof( idDrawVert )
#define DRAWVERT_SIZE_SHIFT			6			// log2( sizeof( idDrawVert ) )
#define DRAWVERT_XYZ_OFFSET			(0*4)		// offsetof( idDrawVert, xyz )
#define DRAWVERT_NORMAL_OFFSET		(4*4)		// offsetof( idDrawVert, normal )
#define DRAWVERT_TANGENT_OFFSET		(8*4)		// offsetof( idDrawVert, tangent )

#endif


assert_sizeof( idDrawVert, DRAWVERT_SIZE );
assert_sizeof( idDrawVert, (1<<DRAWVERT_SIZE_SHIFT) );
assert_offsetof( idDrawVert, xyz, DRAWVERT_XYZ_OFFSET );
assert_offsetof( idDrawVert, _normal, DRAWVERT_NORMAL_OFFSET );
assert_offsetof( idDrawVert, _tangent, DRAWVERT_TANGENT_OFFSET );

ID_INLINE float idDrawVert::operator[]( const int index ) const {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}
ID_INLINE float	&idDrawVert::operator[]( const int index ) {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}

ID_INLINE bool idDrawVert::operator==( const idDrawVert& rhs ) const {
	return ( rhs.xyz.Compare( xyz ) && ( rhs._st[0] == _st[0] && rhs._st[1] == _st[1] ) );
}

ID_INLINE void idDrawVert::Clear( void ) {
	xyz.Zero();

#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_normal[0] = _normal[1] = 0;
	_tangent[0] = _tangent[1] = 0;
	_signs[0] = _signs[1] = _signs[2] = _signs[3] = 0;//128;
	_st[0] = _st[1] = 0;
	color[0] = color[1] = color[2] = color[3] = 0;
#else
	_normal.Zero();
	_tangent.Zero();
	_st.Zero();
	_st2.Zero();
	color[0] = color[1] = color[2] = color[3] = 0;
	_color2[0] = _color2[1] = _color2[2] = _color2[3] = 0;
#endif
}

#if defined( SD_USE_DRAWVERT_SIZE_32 )
ID_INLINE const idVec3 idDrawVert::GetNormal( void ) const {
	return idVec3( _normal[0] / 32767.0f, _normal[1] / 32767.0f, GetZ( _normal[0], _normal[1], _signs[0] ) );
}
#else
ID_INLINE const idVec3 &idDrawVert::GetNormal( void ) const {
	return _normal;
}
#endif

ID_INLINE float		idDrawVert::GetNormalIdx( int idx ) const
{
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	if ( idx == 2 )
	{
		return GetZ( _normal[0], _normal[1], _signs[0] );
	} else
	{
		return _normal[idx] / 32767.0f;
	}
#else
	return _normal[idx];
#endif
}


ID_INLINE void idDrawVert::SetNormal( const idVec3 &n ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_normal[0] = n.x * 32767.0f;
	_normal[1] = n.y * 32767.0f;

	_signs[0] = ( n.z > 0.0f ) ? 2 : 0;
#else
	_normal = n;
#endif
}

ID_INLINE void idDrawVert::SetNormal( float x, float y, float z ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_normal[0] = x * 32767.0f;
	_normal[1] = y * 32767.0f;

	_signs[0] = ( z > 0.0f ) ? 2 : 0;
#else
	_normal.Set( x, y, z );
#endif
}

#if defined( SD_USE_DRAWVERT_SIZE_32 )
ID_INLINE const idVec3 idDrawVert::GetTangent( void ) const {
	return idVec3( _tangent[0] / 32767.0f, _tangent[1] / 32767.0f, GetZ( _tangent[0], _tangent[1], _signs[1] ) );
}

ID_INLINE const idVec4 idDrawVert::GetTangentVec4( void ) const {
	return idVec4( _tangent[0] / 32767.0f, _tangent[1] / 32767.0f, GetZ( _tangent[0], _tangent[1], _signs[1] ), GetBiTangentSign() );
}
#else
ID_INLINE const idVec3& idDrawVert::GetTangent( void ) const {
	return _tangent.ToVec3();
}

ID_INLINE const idVec4& idDrawVert::GetTangentVec4( void ) const {
	return _tangent;
}
#endif

ID_INLINE void idDrawVert::SetTangent( float x, float y, float z ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_tangent[0] = x * 32767.0f;
	_tangent[1] = y * 32767.0f;

	_signs[1] = ( z > 0.0f ) ? 2 : 0;
#else
	_tangent.ToVec3().Set( x, y, z );
#endif
}

ID_INLINE void idDrawVert::SetTangent( const idVec3 &t ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_tangent[0] = t.x * 32767.0f;
	_tangent[1] = t.y * 32767.0f;

	_signs[1] = ( t.z > 0.0f ) ? 2 : 0;
#else
	_tangent.ToVec3() = t;
#endif
}

ID_INLINE const idVec3 idDrawVert::GetBiTangent( void ) const {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	// derive from the normal, tangent, and bitangent direction flag
	idVec3 normal = GetNormal();
	idVec3 tangent = GetTangent();
	idVec3 bitangent;

	bitangent.Cross( normal, tangent );
	bitangent *= ( _signs[2] - 1.f );

	return bitangent;
#else
	// derive from the normal, tangent, and bitangent direction flag
	idVec3 bitangent;

	bitangent.Cross( _normal, _tangent.ToVec3() );
	bitangent *= _tangent[3];

	return bitangent;
#endif
}

ID_INLINE void idDrawVert::SetBiTangent( float x, float y, float z ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec3 bitangent;

	bitangent.Cross( GetNormal(), GetTangent() );
	_signs[2] = ( bitangent.x * x + bitangent.y * y + bitangent.z * z > 0.0f ) ? 2 : 0;
#else
	idVec3 bitangent;

	bitangent.Cross( _normal, _tangent.ToVec3() );
	_tangent[3] = ( bitangent.x * x + bitangent.y * y + bitangent.z * z > 0.0f ) ? 1.0f : -1.0f;
#endif
}

ID_INLINE void idDrawVert::SetBiTangent( const idVec3 &t ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec3 bitangent;

	bitangent.Cross( GetNormal(), GetTangent() );
	_signs[2] = ( bitangent.x * t.x + bitangent.y * t.y + bitangent.z * t.z > 0.0f ) ? 2 : 0;
#else
	_tangent[3] = idVec3::BiTangentSign( _normal, _tangent.ToVec3(), t );
#endif
}

ID_INLINE float idDrawVert::GetBiTangentSign( void ) const {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	return (_signs[2] - 1.f);
#else
	return _tangent.w;
#endif
}

ID_INLINE void idDrawVert::SetBiTangentSign( float sign ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_signs[2] = sign > 0.f ? 2 : 0;
#else
	_tangent[3] = sign;
#endif
}

ID_INLINE void idDrawVert::SetColor( dword color ) {
	*reinterpret_cast<dword *>(this->color) = color;
}

ID_INLINE dword idDrawVert::GetColor( void ) const {
	return *reinterpret_cast<const dword *>(this->color);
}
#if defined( SD_USE_DRAWVERT_SIZE_32 )
ID_INLINE idVec2 idDrawVert::GetST( void ) const {
	return idVec2( (float)_st[0] * ST_TO_FLOAT, (float)_st[1] * ST_TO_FLOAT );
}
#else
ID_INLINE const idVec2& idDrawVert::GetST( void ) const {
	return _st;
}
#endif

#if defined( SD_USE_DRAWVERT_SIZE_32 )
ID_INLINE void idDrawVert::GetST( float &s, float &t ) const {
	s = (float)_st[0] * ST_TO_FLOAT;
	t = (float)_st[1] * ST_TO_FLOAT;
}
#else
ID_INLINE void idDrawVert::GetST( float &s, float &t ) const {
	s = _st[0];
	t = _st[1];
}
#endif

ID_INLINE float	idDrawVert::GetSTIdx( int idx ) const {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	return _st[idx] * ST_TO_FLOAT;
#else
	return _st[idx];
#endif
}


ID_INLINE void idDrawVert::SetST( float s, float t ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_st[0] = (short)(s * FLOAT_TO_ST);
	_st[1] = (short)(t * FLOAT_TO_ST);
#else
	_st[0] = s;
	_st[1] = t;

#if 0
	if ( _st[0] < -8.f || _st[0] > (32767/4096.f)) {
		assert(0);
	}
	if ( _st[1] < -8.f || _st[1] > (32767/4096.f)) {
		assert(0);
	}
#endif
#endif
}

ID_INLINE void	idDrawVert::SetST( bool lowrange, const idVec2 &st ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	if ( lowrange ) {
		this->_st[0] = (short)(st.x * FLOAT_TO_ST_LOWRANGE);
		this->_st[1] = (short)(st.y * FLOAT_TO_ST_LOWRANGE);
	} else {
		this->_st[0] = (short)(st.x * FLOAT_TO_ST);
		this->_st[1] = (short)(st.y * FLOAT_TO_ST);
	}
#else
	this->_st = st;
#if 0
	if ( _st[0] < -8.f || _st[0] > (32767/4096.f)) {
		assert(0);
	}
	if ( _st[1] < -8.f || _st[1] > (32767/4096.f)) {
		assert(0);
	}
#endif
#endif
}

ID_INLINE void idDrawVert::SetST( const idVec2 &st ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	this->_st[0] = (short)(st.x * FLOAT_TO_ST);
	this->_st[1] = (short)(st.y * FLOAT_TO_ST);
#else
	this->_st = st;
#if 0
	if ( _st[0] < -8.f || _st[0] > (32767/4096.f)) {
		assert(0);
	}
	if ( _st[1] < -8.f || _st[1] > (32767/4096.f)) {
		assert(0);
	}
#endif
#endif
}

ID_INLINE void idDrawVert::SetST( const idDrawVert &dv ) {
	_st[0] = dv._st[0];
	_st[1] = dv._st[1];
#if 0
	if ( _st[0] < -8.f || _st[0] > (32767/4096.f)) {
		assert(0);
	}
	if ( _st[1] < -8.f || _st[1] > (32767/4096.f)) {
		assert(0);
	}
#endif
}

ID_INLINE void idDrawVert::SetSTIdx( int idx, float v ) {
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	_st[idx] = (short)(v * FLOAT_TO_ST);
#else
	_st[idx] = v;
#if 0
	if ( _st[0] < -8.f || _st[0] > (32767/4096.f)) {
		assert(0);
	}
	if ( _st[1] < -8.f || _st[1] > (32767/4096.f)) {
		assert(0);
	}
#endif
#endif
}


ID_INLINE float idDrawVert::GetZ( short x, short y, byte sign ) const {
	float v = 1.0f - ( x * x + y * y ) / ( 32767.0f * 32767.0f );
	float sqrtv = v > 0.f ? sqrtf( v ) : 0.f;
	return sqrtv * ( (float)sign - 1.f );
}

ID_INLINE void idDrawVert::Lerp( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec2 aST = a.GetST();
	idVec2 bST = b.GetST();

	SetST( aST + f * ( bST - aST ) );
#else
	_st = a._st + f * ( b._st - a._st );
#endif
}

ID_INLINE void idDrawVert::LerpAll( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
#if defined( SD_USE_DRAWVERT_SIZE_32 )
	idVec2 ST = a.GetST() + f * ( b.GetST() - a.GetST() );
	idVec3 normal = a.GetNormal() + f * ( b.GetNormal() - a.GetNormal() );
	idVec3 tangent = a.GetTangent() + f * ( b.GetTangent() - a.GetTangent() );

	SetST( ST );
	SetNormal( normal );
	SetTangent( tangent );
#else
	_st = a._st + f * ( b._st - a._st );
	_normal = a._normal + f * ( b._normal - a._normal );
	_tangent = a._tangent + f * ( b._tangent - a._tangent );
#endif
	color[0] = idMath::Ftob( a.color[0] + f * ( b.color[0] - a.color[0] ) );
	color[1] = idMath::Ftob( a.color[1] + f * ( b.color[1] - a.color[1] ) );
	color[2] = idMath::Ftob( a.color[2] + f * ( b.color[2] - a.color[2] ) );
	color[3] = idMath::Ftob( a.color[3] + f * ( b.color[3] - a.color[3] ) );
}


typedef struct shadowCache_s {
	idVec4						xyz;					// we use homogenous coordinate tricks
} shadowCache_t;

#define SHADOWVERT_SIZE				16			// sizeof( idDrawVert )
#define SHADOWVERT_SIZE_SHIFT		4			// log2( sizeof( idDrawVert ) )
assert_sizeof( shadowCache_t, SHADOWVERT_SIZE );
assert_sizeof( shadowCache_t, (1<<SHADOWVERT_SIZE_SHIFT) );


#endif /* !__DRAWVERT_H__ */
