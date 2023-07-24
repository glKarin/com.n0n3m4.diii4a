// Copyright (C) 2007 Id Software, Inc.
//


#include "Precompiled.h"
#pragma hdrstop

#include "CompiledScript_Types.h"


sdCompiledScriptType_WString::sdCompiledScriptType_WString( void ) {
	value[ 0 ] = L'\0';
}

sdCompiledScriptType_WString::sdCompiledScriptType_WString( const wchar_t* s ) {
	wcsncpy( value, s, MAX_COMPILED_STRING_LENGTH - 1 );
}

sdCompiledScriptType_WString::sdCompiledScriptType_WString( const sdCompiledScriptType_Float& f ) {
	if ( f.Get() == ( float )( int )f.Get() ) {
		Format( L"%d", ( int )f.Get() );
	} else {
		Format( L"%f", f.Get() );
	}
}

void sdCompiledScriptType_WString::Format( const wchar_t* fmt, ... ) {
#ifdef _CRT_NON_CONFORMING_SWPRINTS
#error Using the new VC++ 2005 / C99 vswprintf API - incompatible with _CRT_NON_CONFORMING_SWPRINTS
#endif 
	va_list argptr;
	va_start( argptr, fmt );
	vswprintf( value, MAX_COMPILED_STRING_LENGTH, fmt, argptr );
	va_end( argptr );
}

sdCompiledScriptType_String::sdCompiledScriptType_String( void ) {
	value[ 0 ] = '\0';
}

sdCompiledScriptType_String::sdCompiledScriptType_String( const sdCompiledScriptType_Float& f ) {
	if ( f.Get() == ( float )( int )f.Get() ) {
		Format( "%d", ( int )f.Get() );
	} else {
		Format( "%f", f.Get() );
	}
}

void sdCompiledScriptType_String::Set( const char* rhs ) {
	strncpy( value, rhs, MAX_COMPILED_STRING_LENGTH - 1 );
}

sdCompiledScriptType_String	sdCompiledScriptType_String::operator+( const char* _value ) const {
	sdCompiledScriptType_String tmp( *this );
	tmp.Append( _value );
	return tmp;
}

sdCompiledScriptType_String	sdCompiledScriptType_String::operator+( const sdCompiledScriptType_String& _value ) const {
	sdCompiledScriptType_String tmp( *this );
	tmp.Append( _value.Get() );
	return tmp;
}

sdCompiledScriptType_String	sdCompiledScriptType_String::operator+( const sdCompiledScriptType_Float& _value ) const {
	sdCompiledScriptType_String tmp;
	if ( _value.Get() == ( float )( int )_value.Get() ) {
		tmp.Format( "%s%d", value, ( int )_value.Get() );
	} else {
		tmp.Format( "%s%f", value, _value.Get() );
	}
	return tmp;
}

void sdCompiledScriptType_String::operator=( const sdCompiledScriptType_Float& rhs ) {
	Format( "%f", rhs.Get() );
}

bool sdCompiledScriptType_String::operator!=( const sdCompiledScriptType_String& _value ) const {
	return strcmp( value, _value.Get() ) != 0;
}

bool sdCompiledScriptType_String::operator==( const sdCompiledScriptType_String& _value ) const {
	return strcmp( value, _value.Get() ) == 0;
}


void sdCompiledScriptType_String::Append( const char* _value ) {
	size_t s1 = strlen( value );
	strncpy( &value[ s1 ], _value, ( sizeof( value ) - s1 ) - 1 );
    value[ sizeof( value ) - 1 ] = '\0';
}

bool sdCompiledScriptType_String::operator!=( const char* _value ) const {
	return strcmp( value, _value ) != 0;
}

bool sdCompiledScriptType_String::operator==( const char* _value ) const {
	return strcmp( value, _value ) == 0;
}

void sdCompiledScriptType_String::Format( const char* fmt, ... ) {
	va_list argptr;
	va_start( argptr, fmt );
	vsnprintf( value, MAX_COMPILED_STRING_LENGTH, fmt, argptr );
	va_end( argptr );
}

sdCompiledScriptType_String operator+( const sdCompiledScriptType_Float& a, const sdCompiledScriptType_String& b ) {
	sdCompiledScriptType_String tmp;
	if ( a.Get() == ( float )( int )a.Get() ) {
		tmp.Format( "%d%s", ( int )a.Get(), b.Get() );
	} else {
		tmp.Format( "%f%s", a.Get(), b.Get() );
	}
	return tmp;
}
