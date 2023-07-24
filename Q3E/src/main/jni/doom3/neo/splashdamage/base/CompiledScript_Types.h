// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __COMPILEDSCRIPT_TYPES_H__
#define __COMPILEDSCRIPT_TYPES_H__

#include "CompiledScriptInterface.h"

class sdCompiledScriptType_Float;
class sdCompiledScriptType_String;
class sdCompiledScriptType_Boolean;

class sdCompiledScriptType_WString {
public:
	static const size_t MAX_COMPILED_STRING_LENGTH = 128;

				sdCompiledScriptType_WString( void );
				sdCompiledScriptType_WString( const wchar_t* s );
				sdCompiledScriptType_WString( const sdCompiledScriptType_Float& f );

	void		Format( const wchar_t* fmt, ... );

	operator const wchar_t*( void ) const { return value; }

	inline const byte* FromData( const byte* p ) {
		memcpy( value, p, sizeof( value ) );
		return p + sizeof( value );
	}

	inline void ToWStringData( UINT_PTR& p ) const {
		p = ( UINT_PTR )value;
	}

	inline byte* GetPointer( void ) {
		return ( byte* )&value;
	}

private:
	wchar_t		value[ MAX_COMPILED_STRING_LENGTH ];
};

class sdCompiledScriptType_String {
public:
	static const size_t MAX_COMPILED_STRING_LENGTH = 128;

				sdCompiledScriptType_String( void );
				sdCompiledScriptType_String( const char* s ) { Set( s ); }
				sdCompiledScriptType_String( const sdCompiledScriptType_Float& f );

	void		Format( const char* fmt, ... );

	const char* Get( void ) const { return value; }
	void		Set( const char* _value );

	void		Append( const char* _value );

	operator const char*( void ) const { return value; }

	void		operator=( const sdCompiledScriptType_Float& rhs );
	bool		operator!( void ) const { return value[ 0 ] == '\0'; }
	bool		operator!=( const char* _value ) const;
	bool		operator==( const char* _value ) const;

	sdCompiledScriptType_String	operator+( const char* _value ) const;
	sdCompiledScriptType_String	operator+( const sdCompiledScriptType_String& _value ) const;
	sdCompiledScriptType_String	operator+( const sdCompiledScriptType_Float& _value ) const;
	bool						operator!=( const sdCompiledScriptType_String& _value ) const;
	bool						operator==( const sdCompiledScriptType_String& _value ) const;

	inline const byte* FromData( const byte* p ) {
		memcpy( value, p, sizeof( value ) );
		return p + sizeof( value );
	}

	inline void ToStringData( UINT_PTR& p ) const {
		p = ( UINT_PTR )value;
	}

	inline byte* GetPointer( void ) {
		return ( byte* )value;
	}

private:
	char		value[ MAX_COMPILED_STRING_LENGTH ];
};

sdCompiledScriptType_String operator+( const sdCompiledScriptType_Float& a, const sdCompiledScriptType_String& b );

class sdCompiledScriptType_Boolean {
public:
				sdCompiledScriptType_Boolean( void ) { value = false; }
				sdCompiledScriptType_Boolean( bool b ) { value = b; }
				sdCompiledScriptType_Boolean( int i ) { value = i; }
				sdCompiledScriptType_Boolean( const sdCompiledScriptType_Float& f );

				operator bool( void ) const { return value != 0; }

	bool		operator!( void ) { return !value; }
	void		operator=( float d ) { value = d != 0.0; }

	bool		operator!=( const sdCompiledScriptType_Boolean& b ) const { return value != b.value; }
	void		operator=( const sdCompiledScriptType_Float& f );
	bool		operator&&( const sdCompiledScriptType_Float& f ) const;
	bool		operator==( const sdCompiledScriptType_Boolean& b ) const { return value == b.value; }
	bool		operator&&( const sdCompiledScriptType_Boolean& b ) const { return value && b.value; }
	bool		operator||( const sdCompiledScriptType_Boolean& b ) const { return value || b.value; }
	bool		operator||( const sdCompiledScriptType_Float& f ) const;
	bool		operator||( float f ) const { return value || f != 0.f; }

	void		Set( bool rhs ) { value = rhs; }
	int			Get( void ) const { return value; }

	inline const byte*	FromData( const byte* p ) {
		value = ( *( int* )p ) != 0;
		return p + sizeof( int );
	}

	inline void ToHandleData( UINT_PTR& p ) const {
		( int& )p = value;
	}

	inline void ToBooleanData( UINT_PTR& p ) const {
		( int& )p = value;
	}

	inline byte* GetPointer( void ) {
		return ( byte* )&value;
	}

private:
	int			value;
};

class sdCompiledScriptType_Float {
public:
				sdCompiledScriptType_Float( void ) { value = 0.f; }
				sdCompiledScriptType_Float( float f ) { value = f; }
				sdCompiledScriptType_Float( int i ) { value = static_cast< float >( i ); }
				sdCompiledScriptType_Float( bool b ) { *this = b; }
				sdCompiledScriptType_Float( const sdCompiledScriptType_Boolean& b ) { value = static_cast< float >( b.Get() ); }

				operator bool( void ) const { return value != 0.f; }
				operator float( void ) const { return value; }

	float		ToInt( void ) const { return ( float )( ( int )value ); }

	bool		operator!( void ) const { return value == 0.f; }
	void		operator=( bool _value ) { value = _value ? 1.f : 0.f; }
	void		operator=( float _value ) { value = _value; }
	bool		operator==( float f ) const { return value == f; }
	bool		operator!=( float f ) const { return value != f; }
	float		operator+( float f ) const { return value + f; }
	float		operator*( float f ) const { return value * f; }
	float		operator/( float f ) const { return value / f; }
	float		operator-( float f ) const { return value - f; }
	bool		operator>( float f ) const { return value > f; }
	bool		operator<( float f ) const { return value < f; }
	bool		operator>=( float f ) const { return value >= f; }
	bool		operator<=( float f ) const { return value <= f; }
	float		operator-( void ) const { return -value; }
	float		operator&( float f ) const { return ( float )( ( int )value & ( int )f ); }
	float		operator|( float f ) const { return ( float )( ( int )value | ( int )f ); }
	float		operator||( float f ) { return ( float )( ( int )value || ( int )f ); }

	void		operator--( int ) { value--; }
	void		operator++( int ) { value++; }

	void		operator|=( float f ) { value = ( float )( ( int )value | ( int )f ); }
	void		operator&=( float f ) { value = ( float )( ( int )value & ( int )f ); }
	void		operator-=( float _value ) { value -= _value; }
	void		operator*=( float _value ) { value *= _value; }
	void		operator+=( float _value ) { value += _value; }

	inline float	operator&( const sdCompiledScriptType_Float& f ) const { return ( float )( ( int )value & ( int )f.value ); }
	inline float	operator|( const sdCompiledScriptType_Float& f ) const { return ( float )( ( int )value | ( int )f.value ); }
	inline void		operator/=( const sdCompiledScriptType_Float& f ) { value /= f.value; }
	inline float	operator%( const sdCompiledScriptType_Float& f ) const { return ( float )( ( int )value % ( int )f.value ); }
	inline float	operator/( const sdCompiledScriptType_Float& f ) const { return value / f.value; }
	inline float	operator*( const sdCompiledScriptType_Float& f ) const { return value * f.value; }
	inline float	operator+( const sdCompiledScriptType_Float& f ) const { return value + f.value; }
	inline float	operator-( const sdCompiledScriptType_Float& f ) const { return value - f.value; }
	inline bool		operator<( const sdCompiledScriptType_Float& f ) const { return value < f.value; }
	inline bool		operator>( const sdCompiledScriptType_Float& f ) const { return value > f.value; }
	inline bool		operator==( const sdCompiledScriptType_Float& f ) const { return value == f.value; }
	inline bool		operator!=( const sdCompiledScriptType_Float& f ) const { return value != f.value; }
	inline bool		operator>=( const sdCompiledScriptType_Float& f ) const { return value >= f.value; }
	inline bool		operator<=( const sdCompiledScriptType_Float& f ) const { return value <= f.value; }
	inline bool		operator&&( const sdCompiledScriptType_Float& f ) const { return value != 0.f && f.value != 0.f; }
	inline bool		operator&&( const sdCompiledScriptType_Boolean& b ) const { return value != 0.f && b.Get(); }
	inline bool		operator||( const sdCompiledScriptType_Boolean& b ) const { return value != 0.f || b.Get(); }
	inline void		operator=( const sdCompiledScriptType_Boolean& b ) { value = static_cast< float >( b.Get() ); }
	inline void		operator=( const sdCompiledScriptType_Float& f ) { value = f.Get(); }

	inline void		Set( float rhs ) { value = rhs; }
	inline float	Get( void ) const { return value; }

	inline const byte* FromData( const byte* p ) {
		value = *( float* )p;
		return p + sizeof( float );
	}

	inline void ToFloatData( UINT_PTR& p ) const {
		( float& )p = value;
	}

	inline void ToIntegerData( UINT_PTR& p ) const {
		( int& )p = static_cast< int >( value );
	}

	inline byte* GetPointer( void ) {
		return ( byte* )&value;
	}

private:
	float		value;
};

class sdCompiledScriptType_Vector {
public:
	sdCompiledScriptType_Vector( void ) {
		value[ 0 ] = 0.f;
		value[ 1 ] = 0.f;
		value[ 2 ] = 0.f;
	}

	sdCompiledScriptType_Vector( float x, float y, float z ) {
		value[ 0 ] = x;
		value[ 1 ] = y;
		value[ 2 ] = z;
	}

	sdCompiledScriptType_Vector( float* xyz ) {
		value[ 0 ] = xyz[ 0 ];
		value[ 1 ] = xyz[ 1 ];
		value[ 2 ] = xyz[ 2 ];
	}

	bool operator!=( const sdCompiledScriptType_Vector& v ) const {
		return value[ 0 ] != v.value[ 0 ] || value[ 1 ] != v.value[ 1 ] || value[ 2 ] != v.value[ 2 ];
	}

	bool operator==( const sdCompiledScriptType_Vector& v ) const {
		return value[ 0 ] == v.value[ 0 ] && value[ 1 ] == v.value[ 1 ] && value[ 2 ] == v.value[ 2 ];
	}

	sdCompiledScriptType_Vector operator-( void ) const {
		sdCompiledScriptType_Vector tmp;
		tmp.value[ 0 ] = -value[ 0 ];
		tmp.value[ 1 ] = -value[ 1 ];
		tmp.value[ 2 ] = -value[ 2 ];
		return tmp;
	}

	sdCompiledScriptType_Vector operator-( const sdCompiledScriptType_Vector& v ) const {
		sdCompiledScriptType_Vector tmp;
		tmp.value[ 0 ] = value[ 0 ] - v.value[ 0 ];
		tmp.value[ 1 ] = value[ 1 ] - v.value[ 1 ];
		tmp.value[ 2 ] = value[ 2 ] - v.value[ 2 ];
		return tmp;
	}

	sdCompiledScriptType_Vector operator+( const sdCompiledScriptType_Vector& v ) const {
		sdCompiledScriptType_Vector tmp;
		tmp.value[ 0 ] = value[ 0 ] + v.value[ 0 ];
		tmp.value[ 1 ] = value[ 1 ] + v.value[ 1 ];
		tmp.value[ 2 ] = value[ 2 ] + v.value[ 2 ];
		return tmp;
	}

	void operator-=( const sdCompiledScriptType_Vector& v ) {
		value[ 0 ] -= v.value[ 0 ];
		value[ 1 ] -= v.value[ 1 ];
		value[ 2 ] -= v.value[ 2 ];
	}

	void operator+=( const sdCompiledScriptType_Vector& v ) {
		value[ 0 ] += v.value[ 0 ];
		value[ 1 ] += v.value[ 1 ];
		value[ 2 ] += v.value[ 2 ];
	}

	void operator*=( const sdCompiledScriptType_Float& v ) {
		value[ 0 ] *= v;
		value[ 1 ] *= v;
		value[ 2 ] *= v;
	}

	void operator/=( const sdCompiledScriptType_Float& v ) {
		value[ 0 ] /= v;
		value[ 1 ] /= v;
		value[ 2 ] /= v;
	}

	float operator*( const sdCompiledScriptType_Vector& v ) const {
		return	value[ 0 ] * v.value[ 0 ] +
				value[ 1 ] * v.value[ 1 ] +
				value[ 2 ] * v.value[ 2 ];
	}

	sdCompiledScriptType_Vector operator*( const sdCompiledScriptType_Float& f ) const {
		sdCompiledScriptType_Vector tmp;
		tmp.value[ 0 ] = value[ 0 ] * f.Get();
		tmp.value[ 1 ] = value[ 1 ] * f.Get();
		tmp.value[ 2 ] = value[ 2 ] * f.Get();
		return tmp;
	}

	sdCompiledScriptType_Vector operator*( float f ) const {
		sdCompiledScriptType_Vector tmp;
		tmp.value[ 0 ] = value[ 0 ] * f;
		tmp.value[ 1 ] = value[ 1 ] * f;
		tmp.value[ 2 ] = value[ 2 ] * f;
		return tmp;
	}

	sdCompiledScriptType_Float& GetX( void ) { return value[ 0 ]; }
	sdCompiledScriptType_Float& GetY( void ) { return value[ 1 ]; }
	sdCompiledScriptType_Float& GetZ( void ) { return value[ 2 ]; }

	const sdCompiledScriptType_Float& GetX( void ) const { return value[ 0 ]; }
	const sdCompiledScriptType_Float& GetY( void ) const { return value[ 1 ]; }
	const sdCompiledScriptType_Float& GetZ( void ) const { return value[ 2 ]; }

	operator float*( void ) const { return ( float* )value; }

	inline const byte* FromData( const byte* p ) {
		p = value[ 0 ].FromData( p );
		p = value[ 1 ].FromData( p );
		return value[ 2 ].FromData( p );
	}

	inline void ToVectorData( UINT_PTR& p ) const {
		p = ( UINT_PTR )value;
	}

	inline byte* GetPointer( void ) {
		return ( byte* )value;
	}

private:
	sdCompiledScriptType_Float		value[ 3 ];
};

inline sdCompiledScriptType_Boolean::sdCompiledScriptType_Boolean( const sdCompiledScriptType_Float& f ) {
	value = f.Get() != 0.f;
}

inline bool	sdCompiledScriptType_Boolean::operator&&( const sdCompiledScriptType_Float& f ) const {
	return value && f.Get() != 0.f;
}

inline bool sdCompiledScriptType_Boolean::operator||( const sdCompiledScriptType_Float& f ) const {
	return value || f.Get() != 0.f;
}


inline void	sdCompiledScriptType_Boolean::operator=( const sdCompiledScriptType_Float& f ) {
	*this = f.Get();
}

class sdCompiledScript_Class;

template< typename T >
class sdCompiledScriptType_Object {
public:
					sdCompiledScriptType_Object( void ) { handle = 0; }
					sdCompiledScriptType_Object( sdCompiledScript_ClassBase* _value ) { SetObject( _value ); }

					template< typename OT >
	inline			sdCompiledScriptType_Object( sdCompiledScriptType_Object< OT >& _value ) { handle = _value.GetHandle(); }

	inline bool		operator!( void ) { return GetObject() == NULL; }
	inline 			operator T*( void ) { return GetObject(); }
	inline T*		operator->( void ) { return GetObject(); }
	inline T*		GetObject( void ) { sdCompiledScript_ClassBase* temp = compilerInterface->GetObject( handle ); return temp->Cast< T >(); }
	inline void		SetObject( sdCompiledScript_ClassBase* _value ) { handle = _value ? _value->__S_GetHandle() : 0; }

	inline const byte* FromData( const byte* p ) {
		handle = *( int* )p;
		return p + sizeof( int );
	}

	inline void ToObjectData( UINT_PTR& p ) const {
		p = ( UINT_PTR )compilerInterface->GetScriptObject( handle );
	}

	inline bool ToEntityData( UINT_PTR& p ) const {
		p = ( UINT_PTR )compilerInterface->GetEntity( handle );
		return p != NULL;
	}

	inline int		GetHandle( void ) { return handle; }

	inline byte*	GetPointer( void ) {
		return ( byte* )&handle;
	}

private:
	int				handle;
};

inline sdCompiledScriptType_Vector operator*( const sdCompiledScriptType_Float& a, const sdCompiledScriptType_Vector& b ) {
	return sdCompiledScriptType_Vector( b.GetX().Get() * a.Get(), b.GetY().Get() * a.Get(), b.GetZ().Get() * a.Get() );
}

#endif // __COMPILEDSCRIPT_TYPES_H__
