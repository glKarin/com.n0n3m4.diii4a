// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Script_ScriptObject.h"
#include "Script_Interface.h"

ABSTRACT_DECLARATION( idClass, sdProgramThread )
END_CLASS


/*
================
sdProgram::sdProgram
================
*/
sdProgram::sdProgram( void ) {
	allocedObjects.SetNum( allocedObjects.Max() );
	for ( int i = 0; i < allocedObjects.Num(); i++ ) {
		allocedObjects[ i ] = NULL;
	}
}

/*
================
sdProgram::GetScriptObject
================
*/
idScriptObject* sdProgram::GetScriptObject( int objectId ) {
	int objectNum = objectId & ( ( 1 << SCRIPT_OBJECTNUM_BITS ) - 1 );
	return ( allocedObjects[ objectNum ] && allocedObjects[ objectNum ]->GetHandle() == objectId ) ? allocedObjects[ objectNum ] : NULL;
}

/*
================
sdProgram::AllocScriptObject
================
*/
idScriptObject* sdProgram::AllocScriptObject( idClass* cls, const sdTypeInfo* objectType ) {
	if ( !objectType ) {
		return NULL;
	}

	int numFree = freeObjectIndicies.Num();

	if ( numFree == 0 ) {
		ListScriptObjects();
		gameLocal.Error( "sdProgram::AllocScriptObject No Free Objects" );
	}

	int freeIndex = freeObjectIndicies[ numFree - 1 ];
	freeObjectIndicies.SetNum( numFree - 1, false );

	int handle = freeIndex + ( nextScriptObjectIndex << SCRIPT_OBJECTNUM_BITS );
	nextScriptObjectIndex++;
	allocedObjects[ freeIndex ] = new idScriptObject( handle, cls, this );
	if ( !allocedObjects[ freeIndex ]->SetType( objectType ) ) {
		gameLocal.Error( "sdProgram::AllocScriptObject Invalid Object Type '%s'", objectType->GetName() );
	}
//	idScriptObject* object = GetScriptObject( allocedObjects[ freeIndex ]->GetHandle() );
//	assert( object == allocedObjects[ freeIndex ] );
	return allocedObjects[ freeIndex ];
}

/*
================
sdProgram::AllocScriptObject
================
*/
idScriptObject* sdProgram::AllocScriptObject( idClass* cls, const char* objectType ) {
	const sdTypeInfo* objectTypeDef = FindTypeInfo( objectType );
	if ( !objectTypeDef ) {
		gameLocal.Error( "sdProgram::AllocScriptObject Couldn't find typeDef for type '%s'", objectType );
	}
	return AllocScriptObject( cls, objectTypeDef );
}

/*
================
sdProgram::FreeScriptObject
================
*/
void sdProgram::FreeScriptObject( idScriptObjectPtr_t& object ) {
	if ( object == NULL ) {
		return;
	}

	int objectNum = object->GetHandle() & ( ( 1 << SCRIPT_OBJECTNUM_BITS ) - 1 );
	if ( allocedObjects[ objectNum ] != object ) {
		gameLocal.Warning( "sdProgram::FreeScriptObject Invalid Free of Object" );
//		delete object;
		return;
	}
	delete allocedObjects[ objectNum ];
	allocedObjects[ objectNum ] = NULL;
	object = NULL;
	freeObjectIndicies.Alloc() = objectNum;
}

/*
================
sdProgram::FreeScriptObjects
================
*/
void sdProgram::FreeScriptObjects( void ) {
	nextScriptObjectIndex = 1;
	allocedObjects.DeleteContents( false );
	freeObjectIndicies.SetNum( allocedObjects.Num() );
	for ( int i = 0; i < freeObjectIndicies.Num(); i++ ) {
		freeObjectIndicies[ i ] = i;
	}
}

/*
================
sdProgram::ListScriptObjects
================
*/
void sdProgram::ListScriptObjects( void ) {
	int count = 0;
	for ( int i = 0; i < MAX_SCRIPT_OBJECTS; i++ ) {
		gameLocal.Printf( "%i:\t", i );
		idScriptObject* obj = allocedObjects[ i ];
		if ( !obj ) {
			gameLocal.Printf( "Empty\n" );
			continue;
		}

		count++;
		gameLocal.Printf( "%s\n", obj->GetTypeName() );
	}
	gameLocal.Printf( "%i script objects in use\n", count );
}

/*
================
sdProgram::ReturnString
================
*/
void sdProgram::ReturnString( const char* value ) {
	gameLocal.program->ReturnStringInternal( value );
}

/*
================
sdProgram::ReturnWString
================
*/
void sdProgram::ReturnWString( const wchar_t* value ) {
	gameLocal.program->ReturnWStringInternal( value );
}

/*
================
sdProgram::ReturnFloat
================
*/
void sdProgram::ReturnFloat( float value ) {
	gameLocal.program->ReturnFloatInternal( value );
}

/*
================
sdProgram::ReturnVector
================
*/
void sdProgram::ReturnVector( const idVec3& value ) {
	gameLocal.program->ReturnVectorInternal( value );
}

/*
================
sdProgram::ReturnEntity
================
*/
void sdProgram::ReturnEntity( idEntity* value ) {
	gameLocal.program->ReturnEntityInternal( value );
}

/*
================
sdProgram::ReturnInteger
================
*/
void sdProgram::ReturnInteger( int value ) {
	gameLocal.program->ReturnFloatInternal( static_cast< float >( value ) );
}

/*
================
sdProgram::ReturnBoolean
================
*/
void sdProgram::ReturnBoolean( bool value ) {
	gameLocal.program->ReturnIntegerInternal( value ? 1 : 0 );
}

/*
================
sdProgram::ReturnObject
================
*/
void sdProgram::ReturnObject( idScriptObject* value ) {
	gameLocal.program->ReturnObjectInternal( value );
}

/*
================
sdProgram::ReturnHandle
================
*/
void sdProgram::ReturnHandle( int value ) {
	gameLocal.program->ReturnIntegerInternal( value );
}

/*
================
sdProgram::ListThreads_f
================
*/
void sdProgram::ListThreads_f( const idCmdArgs& args ) {
	if ( gameLocal.program != NULL && gameLocal.program->IsValid() ) {
		gameLocal.program->ListThreads();
	}
}
