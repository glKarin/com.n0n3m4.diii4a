// Copyright (C) 2004 Id Software, Inc.
//

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
	idVec2			st;
	idVec3			normal;
	idVec3			tangents[2];
	byte			color[4];
#if 0 // was MACOS_X see comments concerning DRAWVERT_PADDED in Simd_Altivec.h 
	float			padding;
#endif
	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

	void			Lerp( const idDrawVert &a, const idDrawVert &b, const float f );
	void			LerpAll( const idDrawVert &a, const idDrawVert &b, const float f );

	void			Normalize( void );

	void			SetColor( dword color );
	dword			GetColor( void ) const;
};

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
