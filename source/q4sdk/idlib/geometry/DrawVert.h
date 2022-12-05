
#ifndef __DRAWVERT_H__
#define __DRAWVERT_H__

/*
===============================================================================

	Draw Vertex.

===============================================================================
*/

class idDrawVert {
public:
	idVec3			xyz;
	byte			color[4];
	idVec3			normal;
	byte			color2[4];
	idVec3			tangents[2];
	idVec2			st;

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

	const idVec3 &	GetNormal( void ) const;
	void			SetNormal( float x, float y, float z );
	void			SetNormal( const idVec3 &n );

	const idVec3 &	GetTangent( void ) const;
	void			SetTangent( float x, float y, float z );
	void			SetTangent( const idVec3 &t );

	const idVec3 & 	GetBiTangent( void ) const;			// derived from normal, tangent, and tangent flag
	void			SetBiTangent( float x, float y, float z );
	void			SetBiTangent( const idVec3 &t );
	void			SetBiTangentSign( float sign );		// either 1.0f or -1.0f

	void			Lerp( const idDrawVert &a, const idDrawVert &b, const float f );
	void			LerpAll( const idDrawVert &a, const idDrawVert &b, const float f );

	void			Normalize( void );

	void			SetColor( dword color );
	dword			GetColor( void ) const;
};

// offsets for SIMD code
#define DRAWVERT_SIZE						64			// sizeof( idDrawVert )
#define DRAWVERT_SIZE_SHIFT					6			// log2( sizeof( idDrawVert ) )
#define DRAWVERT_XYZ_OFFSET					(0*4)		// offsetof( idDrawVert, xyz )
#define DRAWVERT_COLOR_OFFSET				(3*4)		// offsetof( idDrawVert, color )
#define DRAWVERT_NORMAL_OFFSET				(4*4)		// offsetof( idDrawVert, normal )
#define DRAWVERT_COLOR2_OFFSET				(7*4)		// offsetof( idDrawVert, color2 )
#define DRAWVERT_TANGENT0_OFFSET			(8*4)		// offsetof( idDrawVert, tangents[0] )
#define DRAWVERT_TANGENT1_OFFSET			(11*4)		// offsetof( idDrawVert, tangents[1] )
#define DRAWVERT_ST_OFFSET					(14*4)		// offsetof( idDrawVert, st )

assert_sizeof( idDrawVert,					DRAWVERT_SIZE );
assert_sizeof( idDrawVert,					(1<<DRAWVERT_SIZE_SHIFT) );
assert_offsetof( idDrawVert, xyz,			DRAWVERT_XYZ_OFFSET );
assert_offsetof( idDrawVert, color,			DRAWVERT_COLOR_OFFSET );
assert_offsetof( idDrawVert, normal,		DRAWVERT_NORMAL_OFFSET );
assert_offsetof( idDrawVert, color2,		DRAWVERT_COLOR2_OFFSET );
//assert_offsetof( idDrawVert, tangents[0],	DRAWVERT_TANGENT0_OFFSET );
//assert_offsetof( idDrawVert, tangents[1],	DRAWVERT_TANGENT1_OFFSET );
assert_offsetof( idDrawVert, st,			DRAWVERT_ST_OFFSET );

ID_INLINE float idDrawVert::operator[]( const int index ) const {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}
ID_INLINE float	&idDrawVert::operator[]( const int index ) {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}

ID_INLINE void idDrawVert::Clear( void ) {
	xyz.Zero();
	st.Zero();
	normal.Zero();
	tangents[0].Zero();
	tangents[1].Zero();
	color[0] = color[1] = color[2] = color[3] = 0;
}

ID_INLINE const idVec3 &idDrawVert::GetNormal( void ) const {
	return normal;
}

ID_INLINE void idDrawVert::SetNormal( const idVec3 &n ) {
	normal = n;
}

ID_INLINE void idDrawVert::SetNormal( float x, float y, float z ) {
	normal.Set( x, y, z );
}

ID_INLINE const idVec3 &idDrawVert::GetTangent( void ) const {
	return tangents[0];
}

ID_INLINE void idDrawVert::SetTangent( float x, float y, float z ) {
	tangents[0].Set( x, y, z );
}

ID_INLINE void idDrawVert::SetTangent( const idVec3 &t ) {
	tangents[0] = t;
}

ID_INLINE const idVec3 &idDrawVert::GetBiTangent( void ) const {
	return tangents[1];
}

ID_INLINE void idDrawVert::SetBiTangent( float x, float y, float z ) {
	tangents[1].Set( x, y, z );
}

ID_INLINE void idDrawVert::SetBiTangent( const idVec3 &t ) {
	tangents[1] = t;
}

ID_INLINE void idDrawVert::SetBiTangentSign( float sign ) {
//	tangents[0][3] = sign;
}

ID_INLINE void idDrawVert::Lerp( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );
}

ID_INLINE void idDrawVert::LerpAll( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );
	normal = a.normal + f * ( b.normal - a.normal );
	tangents[0] = a.tangents[0] + f * ( b.tangents[0] - a.tangents[0] );
	tangents[1] = a.tangents[1] + f * ( b.tangents[1] - a.tangents[1] );
	color[0] = (byte)( a.color[0] + f * ( b.color[0] - a.color[0] ) );
	color[1] = (byte)( a.color[1] + f * ( b.color[1] - a.color[1] ) );
	color[2] = (byte)( a.color[2] + f * ( b.color[2] - a.color[2] ) );
	color[3] = (byte)( a.color[3] + f * ( b.color[3] - a.color[3] ) );
}

ID_INLINE void idDrawVert::SetColor( dword color ) {
	*reinterpret_cast<dword *>(this->color) = color;
}

ID_INLINE dword idDrawVert::GetColor( void ) const {
	return *reinterpret_cast<const dword *>(this->color);
}

#endif /* !__DRAWVERT_H__ */
