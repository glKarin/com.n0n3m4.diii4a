/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __DRAWVERT_H__
#define __DRAWVERT_H__

/*
===============================================================================

	Draw Vertex.

===============================================================================
*/

class idDrawVert
{
	public:
#ifdef _SPLASHDAMAGE

#define TANGENT_BITANGENT 1

    //karin: using original layout: sizeof(idDrawVert) == 60, it causes OpenGL renderer error(wrong color)
#define _normal normal
#define _st st
	    /*
	    	The sizeof idDrawVert should be at least a multiple of 16 bytes to make the VMX128 code work.
	    	Ideally the sizeof idDrawVert will be a power of two to make indexing fast.

    idVec3			xyz;
    byte			color[4];
    idVec3			_normal;
    byte			_color2[4];
    idVec4			_tangent;		// [3] is texture polarity sign
    idVec2			_st;
    idVec2			_st2;
	    */

	    idVec3			xyz;
	    idVec2			st;
	    idVec3			normal;
#ifdef TANGENT_BITANGENT
		idVec3			tangents[2];
#else
		idVec4			_tangent;		// [3] is texture polarity sign
#endif
	    byte			color[4];
	    //byte			_color2[4];
#if 0
		idDrawVert() {
			Clear();
			color[0] = color[1] = color[2] = color[3] = 255;
		}
#endif
#else
		idVec3			xyz;
		idVec2			st;
		idVec3			normal;
		idVec3			tangents[2];
		byte			color[4];
#if 0 // was MACOS_X see comments concerning DRAWVERT_PADDED in Simd_Altivec.h 
		float			padding;
#endif
#endif
		float			operator[](const int index) const;
		float 			&operator[](const int index);

		void			Clear(void);

		void			Lerp(const idDrawVert &a, const idDrawVert &b, const float f);
		void			LerpAll(const idDrawVert &a, const idDrawVert &b, const float f);

		void			Normalize(void);

		void			SetColor(dword color);
		dword			GetColor(void) const;

#ifdef _SPLASHDAMAGE
    	const idVec3 &	GetNormal( void ) const;
	    float			GetNormalIdx( int idx ) const;
	
	    void			SetNormal( float x, float y, float z );
	    void			SetNormal( const idVec3 &n );

	    const idVec3 &	GetTangent( void ) const;
	    const idVec4 & 	GetTangentVec4( void ) const;
	    void			SetTangent( float x, float y, float z );
	    void			SetTangent( const idVec3 &t );
	
	    const idVec3 	GetBiTangent( void ) const;			// derived from normal, tangent, and tangent flag
	    void			SetBiTangent( float x, float y, float z );
	    void			SetBiTangent( const idVec3 &t );
	    void			SetBiTangent( float v ) {
			SetBiTangentSign(v);
		}
	    float			GetBiTangentSign( void ) const;
	    void			SetBiTangentSign( float sign );		// either 1.0f or -1.0f
	    float			GetSTIdx( int idx ) const;
	    void			GetST( float &s, float &t ) const;
        const idVec2 &	GetST( void ) const;
	    void			SetST( bool lowrange, const idVec2 &st );
	    void			SetST( float s, float t );
	    void			SetST( const idVec2 &st );
	    void			SetST( const idDrawVert &dv );
	    void			SetSTIdx( int i, float v );
	
	    float			GetZ( short x, short y, byte sign ) const;
	    bool			operator==( const idDrawVert& rhs ) const;
#endif
};

#ifdef _SPLASHDAMAGE
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
#if 1 //karin:
#define DRAWVERT_SIZE				60 // 64			// sizeof( idDrawVert )
#define DRAWVERT_SIZE_SHIFT			6			// log2( sizeof( idDrawVert ) )
#define DRAWVERT_XYZ_OFFSET			(0*4)		// offsetof( idDrawVert, xyz )
#define DRAWVERT_NORMAL_OFFSET		(5*4) // (4*4)		// offsetof( idDrawVert, normal )
#define DRAWVERT_TANGENT_OFFSET		(8*4)		// offsetof( idDrawVert, tangent )

assert_sizeof( idDrawVert, DRAWVERT_SIZE );
//assert_sizeof( idDrawVert, (1<<DRAWVERT_SIZE_SHIFT) );
assert_offsetof( idDrawVert, xyz, DRAWVERT_XYZ_OFFSET );
assert_offsetof( idDrawVert, _normal, DRAWVERT_NORMAL_OFFSET );
#ifdef TANGENT_BITANGENT
assert_offsetof( idDrawVert, tangents, DRAWVERT_TANGENT_OFFSET );
#else
assert_offsetof( idDrawVert, _tangent, DRAWVERT_TANGENT_OFFSET );
#endif

#else

#define DRAWVERT_SIZE				64			// sizeof( idDrawVert )
#define DRAWVERT_SIZE_SHIFT			6			// log2( sizeof( idDrawVert ) )
#define DRAWVERT_XYZ_OFFSET			(0*4)		// offsetof( idDrawVert, xyz )
#define DRAWVERT_NORMAL_OFFSET		(4*4)		// offsetof( idDrawVert, normal )
#define DRAWVERT_TANGENT_OFFSET		(8*4)		// offsetof( idDrawVert, tangent )


assert_sizeof( idDrawVert, DRAWVERT_SIZE );
assert_sizeof( idDrawVert, (1<<DRAWVERT_SIZE_SHIFT) );
assert_offsetof( idDrawVert, xyz, DRAWVERT_XYZ_OFFSET );
assert_offsetof( idDrawVert, _normal, DRAWVERT_NORMAL_OFFSET );
#ifdef TANGENT_BITANGENT
assert_offsetof( idDrawVert, tangents, DRAWVERT_TANGENT_OFFSET );
#else
assert_offsetof( idDrawVert, _tangent, DRAWVERT_TANGENT_OFFSET );
#endif

#endif

#endif

#endif

ID_INLINE float idDrawVert::operator[](const int index) const
{
	assert(index >= 0 && index < 5);
	return ((float *)(&xyz))[index];
}
ID_INLINE float	&idDrawVert::operator[](const int index)
{
	assert(index >= 0 && index < 5);
	return ((float *)(&xyz))[index];
}

ID_INLINE void idDrawVert::Clear(void)
{
#ifdef _SPLASHDAMAGE
    xyz.Zero();

	_normal.Zero();
#ifdef TANGENT_BITANGENT
	tangents[0].Zero();
	tangents[1].Zero();
#else
    _tangent.Zero();
#endif
    _st.Zero();
    //_st2.Zero();
    color[0] = color[1] = color[2] = color[3] = 0;
    //_color2[0] = _color2[1] = _color2[2] = _color2[3] = 0;
#else
	xyz.Zero();
	st.Zero();
	normal.Zero();
	tangents[0].Zero();
	tangents[1].Zero();
	color[0] = color[1] = color[2] = color[3] = 0;
#endif
}

ID_INLINE void idDrawVert::Lerp(const idDrawVert &a, const idDrawVert &b, const float f)
{
	xyz = a.xyz + f * (b.xyz - a.xyz);
	st = a.st + f * (b.st - a.st);
}

ID_INLINE void idDrawVert::LerpAll(const idDrawVert &a, const idDrawVert &b, const float f)
{
	xyz = a.xyz + f * (b.xyz - a.xyz);
	st = a.st + f * (b.st - a.st);
	normal = a.normal + f * (b.normal - a.normal);
	tangents[0] = a.tangents[0] + f * (b.tangents[0] - a.tangents[0]);
	tangents[1] = a.tangents[1] + f * (b.tangents[1] - a.tangents[1]);
	color[0] = (byte)(a.color[0] + f * (b.color[0] - a.color[0]));
	color[1] = (byte)(a.color[1] + f * (b.color[1] - a.color[1]));
	color[2] = (byte)(a.color[2] + f * (b.color[2] - a.color[2]));
	color[3] = (byte)(a.color[3] + f * (b.color[3] - a.color[3]));
}

ID_INLINE void idDrawVert::SetColor(dword color)
{
	*reinterpret_cast<dword *>(this->color) = color;
}

ID_INLINE dword idDrawVert::GetColor(void) const
{
	return *reinterpret_cast<const dword *>(this->color);
}

#ifdef _SPLASHDAMAGE

ID_INLINE bool idDrawVert::operator==( const idDrawVert& rhs ) const
{
    return ( rhs.xyz.Compare( xyz ) && ( rhs._st[0] == _st[0] && rhs._st[1] == _st[1] ) );
}

ID_INLINE const idVec3 &idDrawVert::GetNormal( void ) const
{
    return _normal;
}

ID_INLINE float		idDrawVert::GetNormalIdx( int idx ) const
{
    return _normal[idx];
}


ID_INLINE void idDrawVert::SetNormal( const idVec3 &n )
{
    _normal = n;
}

ID_INLINE void idDrawVert::SetNormal( float x, float y, float z )
{
    _normal.Set( x, y, z );
}

ID_INLINE const idVec3& idDrawVert::GetTangent( void ) const
{
#ifdef TANGENT_BITANGENT
    return tangents[0];
#else
    return _tangent.ToVec3();
#endif
}

ID_INLINE const idVec4& idDrawVert::GetTangentVec4( void ) const
{
#ifdef TANGENT_BITANGENT
	return *(idVec4 *)&tangents[0]; // TODO
#else
    return _tangent;
#endif
}

ID_INLINE void idDrawVert::SetTangent( float x, float y, float z )
{
#ifdef TANGENT_BITANGENT
    tangents[0].Set( x, y, z );
#else
    _tangent.ToVec3().Set( x, y, z );
#endif
}

ID_INLINE void idDrawVert::SetTangent( const idVec3 &t )
{
#ifdef TANGENT_BITANGENT
    tangents[0] = t;
#else
    _tangent.ToVec3() = t;
#endif
}

ID_INLINE const idVec3 idDrawVert::GetBiTangent( void ) const
{
#ifdef TANGENT_BITANGENT
	return tangents[1];
#else
    // derive from the normal, tangent, and bitangent direction flag
    idVec3 bitangent;

    bitangent.Cross( _normal, _tangent.ToVec3() );
    bitangent *= _tangent[3];

    return bitangent;
#endif
}

ID_INLINE void idDrawVert::SetBiTangent( float x, float y, float z )
{
#ifdef TANGENT_BITANGENT
	tangents[1].Set(x, y, z);
#else
    idVec3 bitangent;

    bitangent.Cross( _normal, _tangent.ToVec3() );
    _tangent[3] = ( bitangent.x * x + bitangent.y * y + bitangent.z * z > 0.0f ) ? 1.0f : -1.0f;
#endif
}

ID_INLINE void idDrawVert::SetBiTangent( const idVec3 &t )
{
#ifdef TANGENT_BITANGENT
	tangents[1] = t;
#else
    _tangent[3] = idVec3::BiTangentSign( _normal, _tangent.ToVec3(), t );
#endif
}

ID_INLINE float idDrawVert::GetBiTangentSign( void ) const
{
#ifdef TANGENT_BITANGENT
    return idVec3::BiTangentSign( normal, tangents[0], tangents[1] );
#else
    return _tangent.w;
#endif
}

ID_INLINE void idDrawVert::SetBiTangentSign( float sign )
{
#ifdef TANGENT_BITANGENT
	tangents[1] = normal.Cross(tangents[0]) * sign;
#if 0
	tangents[0] = normalize(tangents[0] - (tangents[0] * normal) * normal);
	tangents[1] = (tangents[1] - (tangents[1] * normal) * normal);
	tangents[1].Normalize();
#endif
#else
    _tangent[3] = sign;
#endif
}

ID_INLINE const idVec2& idDrawVert::GetST( void ) const
{
    return _st;
}

ID_INLINE void idDrawVert::GetST( float &s, float &t ) const
{
    s = _st[0];
    t = _st[1];
}

ID_INLINE float	idDrawVert::GetSTIdx( int idx ) const
{
    return _st[idx];
}


ID_INLINE void idDrawVert::SetST( float s, float t )
{
    _st[0] = s;
    _st[1] = t;
}

ID_INLINE void	idDrawVert::SetST( bool lowrange, const idVec2 &st )
{
    this->_st = st;
}

ID_INLINE void idDrawVert::SetST( const idVec2 &st )
{
    this->_st = st;
}

ID_INLINE void idDrawVert::SetST( const idDrawVert &dv )
{
    _st[0] = dv._st[0];
    _st[1] = dv._st[1];
}

ID_INLINE void idDrawVert::SetSTIdx( int idx, float v )
{
    _st[idx] = v;
}


ID_INLINE float idDrawVert::GetZ( short x, short y, byte sign ) const
{
    float v = 1.0f - ( x * x + y * y ) / ( 32767.0f * 32767.0f );
    float sqrtv = v > 0.f ? sqrtf( v ) : 0.f;
    return sqrtv * ( (float)sign - 1.f );
}


typedef struct shadowCache_s {
    idVec4						xyz;					// we use homogenous coordinate tricks
} shadowCache_t;

#define SHADOWVERT_SIZE				16			// sizeof( idDrawVert )
#define SHADOWVERT_SIZE_SHIFT		4			// log2( sizeof( idDrawVert ) )
assert_sizeof( shadowCache_t, SHADOWVERT_SIZE );
assert_sizeof( shadowCache_t, (1<<SHADOWVERT_SIZE_SHIFT) );
#endif
    //karin: for compat
// #undef _normal
// #undef _st
#endif /* !__DRAWVERT_H__ */
