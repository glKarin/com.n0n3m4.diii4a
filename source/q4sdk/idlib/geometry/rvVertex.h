//
// rvVertex.h - Describes some commonly used vertices (alternatives to the idDrawVert)
// Date: 1/10/05
// Created by: Dwight Luetscher - Raven Software
//

#ifndef __RV_VERTEX_H__
#define __RV_VERTEX_H__

//
// rvBlend4DrawVert
//
// a vertex that is used to communicate data to drawing vertices stored in vertex buffers
//
class rvBlend4DrawVert {
public:
	idVec3			xyz;
	int				blendIndex[4];			
	float			blendWeight[4];		// NOTE: the vertex stored in the actual buffer that is actually used for drawing may leave out the last weight (implied 1 - sum of other weights)
	idVec3			normal;
	idVec3			tangent;
	idVec3			binormal;
	byte			color[4];			// diffuse color, [0] red, [1] green, [2] blue, [3] alpha
	idVec2			st;
};

//
// rvSilTraceVertT
//
// a transformed vert that typically resides in system-memory and is used for operations
// like silhouette determination and trace testing
//
class rvSilTraceVertT {
public:
	idVec4			xyzw;

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

	void			Lerp( const rvSilTraceVertT &a, const rvSilTraceVertT &b, const float f );
	void			LerpAll( const rvSilTraceVertT &a, const rvSilTraceVertT &b, const float f );
};

#define SILTRACEVERT_SIZE_SHIFT			4
#define SILTRACEVERT_SIZE				(1 << SILTRACEVERT_SIZE_SHIFT)
#define SILTRACEVERT_XYZW_OFFSET		0

assert_sizeof( rvSilTraceVertT,			SILTRACEVERT_SIZE );
assert_sizeof( rvSilTraceVertT,			(1<<SILTRACEVERT_SIZE_SHIFT) );
assert_offsetof( rvSilTraceVertT, xyzw,	SILTRACEVERT_XYZW_OFFSET );

ID_INLINE float rvSilTraceVertT::operator[]( const int index ) const 
{
	assert( index >= 0 && index < 4 );
	return ((float *)(&xyzw))[index];
}

ID_INLINE float	&rvSilTraceVertT::operator[]( const int index ) 
{
	assert( index >= 0 && index < 4 );
	return ((float *)(&xyzw))[index];
}

ID_INLINE void rvSilTraceVertT::Clear( void ) 
{
	xyzw.Zero();
}

ID_INLINE void rvSilTraceVertT::Lerp( const rvSilTraceVertT &a, const rvSilTraceVertT &b, const float f ) 
{
	xyzw = a.xyzw + f * ( b.xyzw - a.xyzw );
}

ID_INLINE void rvSilTraceVertT::LerpAll( const rvSilTraceVertT &a, const rvSilTraceVertT &b, const float f ) 
{
	xyzw = a.xyzw + f * ( b.xyzw - a.xyzw );
}

#endif	// #ifndef __RV_VERTEX_H__
