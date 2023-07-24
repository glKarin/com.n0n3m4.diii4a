// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceTypes.h"

const char sdUIFunctionStack_Identifier[] = "sdUIFunctionStack";

sdUIFunctionStack::allocator_t sdUIFunctionStack::allocator;

/*
===============================================================================

	sdUIFunctionStack

===============================================================================
*/

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( int& value ) {
	float temp;
	Pop( temp );
	value = idMath::Ftoi( temp );
}

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( bool& value ) {
	float temp;
	Pop( temp );
	value = temp != 0.0f;
}

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( float& value ) {
	Top( value );
	floatStack.SetNum( floatStack.Num() - 1 );
}

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( idVec2& value ) {
	Top( value );
	floatStack.SetNum( floatStack.Num() - 2 );
}

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( idVec3& value ) {
	Top( value );
	floatStack.SetNum( floatStack.Num() - 3 );
}

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( idVec4& value ) {
	Top( value );
	floatStack.SetNum( floatStack.Num() - 4 );
}

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( idStr& value ) {
	Top( value );
	stringStack.SetNum( stringStack.Num() - 1 );
}

/*
================
sdUIFunctionStack::Pop
================
*/
void sdUIFunctionStack::Pop( idWStr& value ) {
	Top( value );
	wstringStack.SetNum( wstringStack.Num() - 1 );
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( int& value ) const {
	float temp;
	Top( temp );
	value = temp;
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( bool& value ) const {
	float temp;
	Top( temp );
	value = temp != 0.0f;
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( float& value ) const {
	if ( floatStack.Num() < 1 ) {
		gameLocal.Error( "%s: Float Stack Underflow", identifier.c_str() );
	}
	int index = floatStack.Num() - 1;
	value = floatStack[ index + 0 ];
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( idVec2& value ) const {
	if ( floatStack.Num() < 2 ) {
		gameLocal.Error( "%s: Vec2 Stack Underflow", identifier.c_str() );
	}
	int index = floatStack.Num() - 2;
	value[ 0 ] = floatStack[ index + 1 ];
	value[ 1 ] = floatStack[ index + 0 ];
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( idVec3& value ) const {
	if ( floatStack.Num() < 3 ) {
		gameLocal.Error( "%s: Vec3 Stack Underflow", identifier.c_str() );
	}
	int index = floatStack.Num() - 3;
	value[ 0 ] = floatStack[ index + 2 ];
	value[ 1 ] = floatStack[ index + 1 ];
	value[ 2 ] = floatStack[ index + 0 ];
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( idVec4& value ) const {
	if ( floatStack.Num() < 4 ) {
		gameLocal.Error( "%s: Vec4 Stack Underflow", identifier.c_str() );
	}
	int index = floatStack.Num() - 4;
	value[ 0 ] = floatStack[ index + 3 ];
	value[ 1 ] = floatStack[ index + 2 ];
	value[ 2 ] = floatStack[ index + 1 ];
	value[ 3 ] = floatStack[ index + 0 ];
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( idStr& value ) const {
	if ( stringStack.Num() < 1 ) {
		gameLocal.Error( "%s String Stack Underflow", identifier.c_str() );
	}
	int index = stringStack.Num() - 1;
	value = stringStack[ index + 0 ];
}

/*
================
sdUIFunctionStack::Top
================
*/
void sdUIFunctionStack::Top( idWStr& value ) const {
	if ( wstringStack.Num() < 1 ) {
		gameLocal.Error( "%s: WString Stack Underflow", identifier.c_str() );
	}
	int index = wstringStack.Num() - 1;
	value = wstringStack[ index + 0 ];
}

/*
============
sdUIEventTable::sdUIEventTable
============
*/
sdUIEventTable::sdUIEventTable() {
}

/*
================
sdUIEventTable::AddEvent
================
*/
void sdUIEventTable::AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) {	
	if( !info.eventType.IsValid() || info.eventType >= events.Num() ) {
		gameLocal.Warning( "sdUIEventTable::AddEvent: event type '%i' out of range", (int)info.eventType );
		assert( 0 );
		return;
	}

	eventList_t& list = events[ info.eventType ];

	int count = info.parameter + 1;
	if ( count > list.events.Num() ) {
		int oldNum = list.events.Num();
		list.events.SetNum( count, false );
	}

	list.events[ info.parameter ] = scriptHandle;
}

/*
================
sdUIEventTable::GetEvent
================
*/
sdUIEventHandle sdUIEventTable::GetEvent( const sdUIEventInfo& info ) const {	
	if( !info.eventType.IsValid() || info.eventType >= events.Num() ) {
		assert( 0 );
		gameLocal.Warning( "sdUIEventTable::GetEvent: event type '%i' out of range", (int)info.eventType );
		return sdUIEventHandle();
	}

	const eventList_t& list = events[ info.eventType ];
	if ( info.parameter >= list.events.Num() ) {
		return sdUIEventHandle();
	}
	return list.events[ info.parameter ];
}

/*
============
sdUIEventTable::Clear
============
*/
void sdUIEventTable::Clear() {	
	events.Clear();
}
