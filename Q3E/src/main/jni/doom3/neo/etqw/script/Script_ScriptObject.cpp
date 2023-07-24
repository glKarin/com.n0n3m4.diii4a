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
#include "Script_Helper.h"

/***********************************************************************

  idScriptObject

***********************************************************************/

/*
============
sdScriptObjectNetworkData::MakeDefault
============
*/
void sdScriptObjectNetworkData::MakeDefault( void ) {
	data.SetNum( 0, false );
}

/*
============
sdScriptObjectNetworkData::Write
============
*/
void sdScriptObjectNetworkData::Write( idFile* file ) const {
	file->WriteInt( data.Num() );
	file->Write( data.Begin(), data.Num() );
}

/*
============
sdScriptObjectNetworkData::Read
============
*/
void sdScriptObjectNetworkData::Read( idFile* file ) {
	int count;
	file->ReadInt( count );
	data.SetNum( count );

	file->Read( data.Begin(), count );
}

/*
============
idScriptObject::idScriptObject
============
*/
idScriptObject::idScriptObject( int handle, idClass* object, sdProgram* _program ) {
	typeObject			= NULL;
	program				= _program;
	_handle				= handle;
	_object				= object;
	
	for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
		networkFields[ i ].size = 0;
	}
}

/*
============
idScriptObject::~idScriptObject
============
*/
idScriptObject::~idScriptObject() {
	Free();
}

/*
============
idScriptObject::Free
============
*/
void idScriptObject::Free( void ) {
	gameLocal.program->FreeType( typeObject );
	typeObject = NULL;

	ShutdownAutoThreads();
}

/*
============
idScriptObject::SetType

Allocates an object and initializes memory.
============
*/
bool idScriptObject::SetType( const sdProgram::sdTypeInfo* newtype ) {
	typeObject = program->AllocType( typeObject, newtype );
	if ( typeObject == NULL ) {
		return false;
	}
	typeObject->SetHandle( _handle );
	return true;
}

/*
============
idScriptObject::CallEvent
============
*/
void idScriptObject::CallEvent( const char* name ) {
	const sdProgram::sdFunction* function = GetFunction( name );
	if ( function == NULL ) {
		return;
	}

	sdScriptHelper h1;
	CallNonBlockingScriptEvent( function, h1 );
}

/*
============
idScriptObject::SetType

Allocates an object and initializes memory.
============
*/
bool idScriptObject::SetType( const char *typeName ) {
	return SetType( program->FindTypeInfo( typeName ) );
}

/*
============
idScriptObject::ClearObject

Resets the memory for the script object without changing its type.
============
*/
void idScriptObject::ClearObject( void ) {
	if ( typeObject != NULL ) {
		typeObject->Clear();
	}

	for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
		networkFields[ i ].fields.Clear();
		networkFields[ i ].size = 0;
	}
}

/*
============
idScriptObject::GetTypeName
============
*/
const char* idScriptObject::GetTypeName( void ) const {
	return typeObject->GetName();
}

/*
============
idScriptObject::GetSyncFunc
============
*/
const sdProgram::sdFunction* idScriptObject::GetSyncFunc( void ) const {
	return GetFunction( "syncFields" );
}

/*
============
idScriptObject::GetPreConstructor
============
*/
const sdProgram::sdFunction* idScriptObject::GetPreConstructor( void ) const {
	return GetFunction( "preinit" );
}

/*
============
idScriptObject::GetConstructor
============
*/
const sdProgram::sdFunction* idScriptObject::GetConstructor( void ) const {
	return GetFunction( "init" );
}

/*
============
idScriptObject::GetDestructor
============
*/
const sdProgram::sdFunction* idScriptObject::GetDestructor( void ) const {
	return GetFunction( "destroy" );
}

/*
============
idScriptObject::GetFunction
============
*/
const sdProgram::sdFunction* idScriptObject::GetFunction( const char *name ) const {
	if ( !HasObject() ) {
		return NULL;
	}

	return program->FindFunction( name, typeObject );
}

/*
============
idScriptObject::GetVariable
============
*/
byte *idScriptObject::GetVariable( const char *name, etype_t etype ) const {
	if ( typeObject == NULL ) {
		return NULL;
	}

	byte* data = NULL;
	etype_t varEType = typeObject->GetVariable( name, &data );
	if ( varEType != etype ) {
		return NULL;
	}
	return data;
}

/*
============
idScriptObject::SetSynced
============
*/
void idScriptObject::SetSynced( const char *name, bool broadcast ) {
	networkStateMode_t mode = broadcast ? NSM_BROADCAST : NSM_VISIBLE;

	if ( typeObject == NULL ) {
		gameLocal.Warning( "idScriptObject::SetSynced No ScriptObject" );
		return;
	}

	byte* data = NULL;
	etype_t etype = typeObject->GetVariable( name, &data );
	if ( data == NULL ) {
		gameLocal.Warning( "idScriptObject::SetSynced Unknown Field '%s'", name );
	}

	switch( etype ) {
		case ev_object:
		case ev_vector:
		case ev_boolean:
		case ev_float: {
			networkFieldSync_t& value = *networkFields[ mode ].fields.Alloc();
			if ( &value == NULL ) {
				gameLocal.Warning( "idScriptObject::SetSynced Max Synced Fields Hit" );
				return;
			}
			value.data		= data;
			value.type		= etype;
			value.callback	= NULL;
			value.broadcast	= broadcast;

			int size = 0;
			switch ( etype ) {
				case ev_object:
					size = sizeof( int );
					break;
				case ev_vector:
					size = sizeof( idVec3 );
					break;
				case ev_boolean:
					size = sizeof( int );
					break;
				case ev_float:
					size = sizeof( float );
					break;
			}

			networkFields[ mode ].size += size;
			return;
		}
		default:
			gameLocal.Warning( "idScriptObject::SetSynced Cannot Sync Field '%s'", name );
			return;
	}
}

/*
============
idScriptObject::SetSyncCallback
============
*/
void idScriptObject::SetSyncCallback( const char *name, const char* functionName ) {
	const sdProgram::sdFunction* function = GetFunction( functionName );
	if ( !function ) {
		gameLocal.Warning( "idScriptObject::SetSyncCallback Unknown Function '%s'", functionName );
		return;
	}

	if ( typeObject == NULL ) {
		gameLocal.Warning( "idScriptObject::SetSyncCallback No Script Object" );
		return;
	}

	byte* data = NULL;
	typeObject->GetVariable( name, &data );
	if ( data == NULL ) {
		gameLocal.Warning( "idScriptObject::SetSyncCallback Unknown Field '%s'", name );
	}

	for ( int k = 0; k < NSM_NUM_MODES; k++ ) {
		for ( int j = 0; j < networkFields[ k ].fields.Num(); j++ ) {
			if ( networkFields[ k ].fields[ j ].data == data ) {
				networkFields[ k ].fields[ j ].callback = function;
				return;
			}
		}
	}
	gameLocal.Warning( "idScriptObject::SetSyncCallback No Synced Field Named '%s' Found", name );
}

/*
================
idScriptObject::CallNonBlockingScriptEvent
================
*/
void idScriptObject::CallNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) {
	if ( !function ) {
		return;
	}

	if ( helper.Init( this, function ) ) {
		helper.Run();
		if ( !helper.Done() ) {
			gameLocal.Error( "idScriptObject::CallNonBlockingScriptEvent '%s' Cannot be Blocking", function->GetName() );
		}
	}
}

/*
================
idScriptObject::ApplyNetworkState
================
*/
void idScriptObject::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	NET_GET_NEW( sdScriptObjectNetworkData );

	newData.Reset();

	for ( int i = 0; i < networkFields[ mode ].fields.Num(); i++ ) {
		const networkFieldSync_t& value = networkFields[ mode ].fields[ i ];

		switch( value.type ) {
			case ev_object: {
				const int& newBase = newData.GetInt();

				int& e = *reinterpret_cast< int* >( value.data );
				int oldE = e;

				idEntity* ent = gameLocal.EntityForSpawnId( newBase );
				e = ent ? ent->GetScriptObject()->GetHandle() : 0;

				if ( value.callback ) {
					if ( oldE != e ) {
						gameLocal.CallFrameCommand( this, value.callback );
					}
				}
				break;
			}
			case ev_vector: {
				const idVec3& newBase = newData.GetVector();

				idVec3& v = *reinterpret_cast< idVec3* >( value.data );

				if ( newBase != v ) {
					v = newBase;
					if ( value.callback ) {
						gameLocal.CallFrameCommand( this, value.callback );
					}
				}
				break;
			}
			case ev_float: {
				const float& newBase = newData.GetFloat();

				float& f = *reinterpret_cast< float* >( value.data );

				if ( newBase != f ) {
					f = newBase;
					if ( value.callback ) {
						gameLocal.CallFrameCommand( this, value.callback );
					}
				}

				break;
			}
			case ev_boolean: {
				const int& newBase = newData.GetInt();

				bool& b = *reinterpret_cast< bool* >( value.data );

				if ( ( newBase != 0 ) != b ) {
					b = newBase != 0;
					if ( value.callback ) {
						gameLocal.CallFrameCommand( this, value.callback );
					}
				}
				break;
			}
		}
	}
}

/*
================
idScriptObject::ReadNetworkState
================
*/
void idScriptObject::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdScriptObjectNetworkData );
	SetupStateData( mode, newData );

	baseData.Reset();
	newData.Reset();

	for ( int i = 0; i < networkFields[ mode ].fields.Num(); i++ ) {
		const networkFieldSync_t& value = networkFields[ mode ].fields[ i ];

		switch( value.type ) {
			case ev_object: {
				const int& oldBase = baseData.GetInt();
				int& newBase = newData.GetInt();
				newBase = msg.ReadDeltaLong( oldBase );
				break;
			}
			case ev_vector: {
				const idVec3& oldBase = baseData.GetVector();
				idVec3& newBase = newData.GetVector();
				newBase = msg.ReadDeltaVector( oldBase );
				break;
			}
			case ev_float: {
				const float& oldBase = baseData.GetFloat();
				float& newBase = newData.GetFloat();
				newBase = msg.ReadDeltaFloat( oldBase );
				break;
			}
			case ev_boolean: {
				const int& oldBase = baseData.GetInt();
				int& newBase = newData.GetInt();
				newBase = msg.ReadBits( 1 );
				break;
			}
		}
	}
}

/*
================
idScriptObject::WriteNetworkState
================
*/
void idScriptObject::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdScriptObjectNetworkData );
	SetupStateData( mode, newData );

	baseData.Reset();
	newData.Reset();

	for ( int i = 0; i < networkFields[ mode ].fields.Num(); i++ ) {
		const networkFieldSync_t& value = networkFields[ mode ].fields[ i ];

		switch( value.type ) {
			case ev_object: {
				idScriptObject* object = gameLocal.program->GetScriptObject( *reinterpret_cast< int* >( value.data ) );
				int id = 0;
				if ( object ) {
					idEntity* ent = object->GetClass()->Cast< idEntity >();
					id = gameLocal.GetSpawnId( ent );
				}
				
				const int& oldBase = baseData.GetInt();
				int& newBase = newData.GetInt();

				newBase = id;

				msg.WriteDeltaLong( oldBase, newBase );

				break;
			}
			case ev_vector: {
				const idVec3& oldBase = baseData.GetVector();
				idVec3& newBase = newData.GetVector();

				newBase = *reinterpret_cast< idVec3* >( value.data );

				msg.WriteDeltaVector( oldBase, newBase );
				break;
			}
			case ev_float: {
				const float& oldBase = baseData.GetFloat();
				float& newBase = newData.GetFloat();

				newBase = *reinterpret_cast< float* >( value.data );

				msg.WriteDeltaFloat( oldBase, newBase );
				break;
			}
			case ev_boolean: {
				const int& oldBase = baseData.GetInt();
				int& newBase = newData.GetInt();

				newBase = *reinterpret_cast< int* >( value.data ) != 0;

				msg.WriteBits( newBase, 1 );
				break;
			}
		}
	}
}

/*
================
idScriptObject::CheckNetworkStateChanges
================
*/
bool idScriptObject::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	NET_GET_BASE( sdScriptObjectNetworkData );

	if ( baseData.data.Num() != networkFields[ mode ].size ) {
		return true;
	}

	baseData.Reset();

	for ( int i = 0; i < networkFields[ mode ].fields.Num(); i++ ) {
		const networkFieldSync_t& value = networkFields[ mode ].fields[ i ];

		switch( value.type ) {
			case ev_object: {
				const int& intRef = baseData.GetInt();

				idScriptObject* object = gameLocal.program->GetScriptObject( *reinterpret_cast< int* >( value.data ) );
				int id = 0;
				if ( object ) {
					idEntity* ent = object->GetClass()->Cast< idEntity >();
					id = gameLocal.GetSpawnId( ent );
				}
				if ( intRef != id ) {
					return true;
				}
				break;
			}
			case ev_vector: {
				const idVec3& vec3Ref = baseData.GetVector();

				idVec3* temp = reinterpret_cast< idVec3* >( value.data );
				if ( vec3Ref != *temp ) {
					return true;
				}
				break;
			}
			default:
			case ev_float: {
				const float& floatRef = baseData.GetFloat();

				float* temp = reinterpret_cast< float* >( value.data );
				if ( floatRef != *temp ) {
					return true;
				}
				break;
			}
			case ev_boolean: {
				const int& intRef = baseData.GetInt();

				int* temp = reinterpret_cast< int* >( value.data );
				if ( intRef != *temp ) {
					return true;
				}
				break;
			}
		}
	}

	return false;
}

/*
================
idScriptObject::SetupStateData
================
*/
void idScriptObject::SetupStateData( networkStateMode_t mode, sdScriptObjectNetworkData& stateData ) const {
	if ( stateData.data.Num() != networkFields[ mode ].size ) {
		stateData.data.SetNum( networkFields[ mode ].size );
		for ( int i = 0; i < networkFields[ mode ].size; i++ ) {
			stateData.data[ i ] = 0;
		}
	}
}

/*
================
idScriptObject::ShutdownAutoThreads
================
*/
void idScriptObject::ShutdownAutoThreads( void ) {
	sdProgramThread* nextThread;
	for ( sdProgramThread* thread = autoThreads.Next(); thread != NULL; thread = nextThread ) {
		nextThread = thread->GetAutoNode().Next();

		if ( thread->IsDying() ) {
			continue;
		}

		if ( thread == gameLocal.program->GetCurrentThread() ) {
			gameLocal.Warning( "idScriptObject::ShutdownAutoThreads Killing A Scriptobject Within its own Auto Thread" );
			continue;
		}

		thread->EndThread();
	}
}

int Script_GetSpawnId( idEntity* entity ) {
	idScriptObject* object = entity->GetScriptObject();
	return object ? object->GetHandle() : 0;
}

idEntity* Script_EntityForSpawnId( int handle ) {
	idScriptObject* object = gameLocal.program->GetScriptObject( handle );
	return object ? object->GetClass()->Cast< idEntity >() : NULL;
}
