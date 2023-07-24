// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Script_Helper.h"
#include "Script_Program.h"
#include "Script_ScriptObject.h"
#include "../Entity.h"
#include "../structures/TeamManager.h"
#include "../../idlib/PropertiesImpl.h"

/*
===========
sdScriptHelper::sdScriptHelper
============
*/
sdScriptHelper::sdScriptHelper( void ) {
	thread = NULL;
	Clear();
}

/*
===========
sdScriptHelper::~sdScriptHelper
============
*/
sdScriptHelper::~sdScriptHelper( void ) {
	Clear();
}

/*
===========
sdScriptHelper::Clear
============
*/
void sdScriptHelper::Clear( bool free ) {
	if ( thread != NULL && free ) {
		gameLocal.program->FreeThread( thread );
	}
	thread		= NULL;
	function	= NULL;
	object		= NULL;
	size		= type_object.Size();
	args.Clear();
}

/*
===========
sdScriptHelper::Init
============
*/
bool sdScriptHelper::Init( idScriptObject* obj, const char* functionName ) {
	if ( thread != NULL ) {
		gameLocal.Warning( "sdScriptHelper::Init ScriptHelper reused without being cleared" );
		assert( false );
		return false;
	}

	if ( !obj || !functionName || !*functionName ) {
		return false;
	}

	object		= obj;

	function	= obj->GetFunction( functionName );
	if ( !function ) {
		return false;
	}

	return true;
}

/*
===========
sdScriptHelper::Init
============
*/
bool sdScriptHelper::Init( idScriptObject* obj, const sdProgram::sdFunction* _function ) {
	if ( !obj || !_function ) {
		return false;
	}

	object		= obj;
	function	= _function;

	return true;
}

/*
===========
sdScriptHelper::Done
============
*/
bool sdScriptHelper::Done( void ) const {
	return !thread || thread->IsDying();
}

/*
===========
sdScriptHelper::Push
============
*/
void sdScriptHelper::Push( int value ) {
	float fvalue = static_cast< float >( value );
	Push( fvalue );
}

/*
===========
sdScriptHelper::Push
============
*/
void sdScriptHelper::Push( const char* value ) {
	parms_t* arg	= args.Alloc();
	if ( arg == NULL ) {
		gameLocal.Error( "sdScriptHelper::Push - args.Alloc() returned NULL - number of arguments overflowed" );
		return;
	}

	arg->integer		= 0;
	arg->string		= value;

	size += MAX_STRING_LEN;
}

/*
===========
sdScriptHelper::Push
============
*/
void sdScriptHelper::Push( float value ) {
	parms_t* arg	= args.Alloc();
	if ( arg == NULL ) {
		gameLocal.Error( "sdScriptHelper::Push - args.Alloc() returned NULL - number of arguments overflowed" );
		return;
	}

	arg->integer		= *reinterpret_cast< int* >( &value );
	arg->string		= NULL;

	size += sizeof( int );
}

/*
===========
sdScriptHelper::Push
============
*/
void sdScriptHelper::Push( idScriptObject* obj ) {
	parms_t* arg	= args.Alloc();
	if ( arg == NULL ) {
		gameLocal.Error( "sdScriptHelper::Push - args.Alloc() returned NULL - number of arguments overflowed" );
		return;
	}

	arg->integer		= obj->GetHandle();
	arg->string		= NULL;

	size += sizeof( int );
}

/*
===========
sdScriptHelper::Push
============
*/
void sdScriptHelper::Push( sdTeamInfo* team ) {
	Push( team ? team->GetScriptObject() : NULL );
}

/*
===========
sdScriptHelper::Push
============
*/
void sdScriptHelper::Push( const idVec3& vec ) {
	Push( vec[ 0 ] );
	Push( vec[ 1 ] );
	Push( vec[ 2 ] );
}

/*
===========
sdScriptHelper::Push
============
*/
void sdScriptHelper::Push( const idAngles& angles ) {
	Push( angles.pitch );
	Push( angles.yaw );
	Push( angles.roll );
}


/*
===========
sdScriptHelper::Run
============
*/
void sdScriptHelper::Run( void ) {
	if ( !function ) {
		assert( false );
		return;
	}
	thread = gameLocal.program->CreateThread( *this );
	if ( thread == NULL ) {
		return;
	}
	assert( !thread->IsDying() );
#ifdef _DEBUG
	thread->SetName( "sdScriptHelper" );
#endif // _DEBUG
	thread->ManualDelete();
	thread->Execute();
}

/*
===========
sdScriptHelper::GetReturnedFloat
============
*/
float sdScriptHelper::GetReturnedFloat( void ) {
	return gameLocal.program->GetReturnedFloat();
}

/*
===========
sdScriptHelper::GetReturnedInteger
============
*/
int sdScriptHelper::GetReturnedInteger( void ) {
	return gameLocal.program->GetReturnedInteger();
}

/*
===========
sdScriptHelper::GetReturnedString
============
*/
const char* sdScriptHelper::GetReturnedString( void ) {
	return gameLocal.program->GetReturnedString();
}


const idEventDef EV_CVar_GetFloatValue( "getFloatValue", 'f', DOC_TEXT( "Returns a float representation of the cvar's current value." ), 0, "If the value cannot be converted to a float, the result will be 0." );
const idEventDef EV_CVar_GetStringValue( "getStringValue", 's', DOC_TEXT( "Returns the cvar's current value." ), 0, NULL );
const idEventDef EV_CVar_GetVectorValue( "getVectorValue", 'v', DOC_TEXT( "Returns a vector representation of the cvar's current value." ), 0, "If the value cannot be converted to a vector, the result will be '0 0 0'." );
const idEventDef EV_CVar_GetIntValue( "getIntValue", 'd', DOC_TEXT( "Returns an integer representation of the cvar's current value." ), 0, "If the value cannot be converted to an integer, the result will be 0." );
const idEventDef EV_CVar_GetBoolValue( "getBoolValue", 'b', DOC_TEXT( "Returns a boolean representation of the cvar's current value." ), 0, "If the value cannot be converted to a boolean, the result will be false." );

const idEventDef EV_CVar_SetFloatValue( "setFloatValue", '\0', DOC_TEXT( "Converts the given value to a string and sets it as the cvar's value." ), 1, NULL, "f", "value", "Value to set." );
const idEventDef EV_CVar_SetStringValue( "setStringValue", '\0', DOC_TEXT( "Sets the cvar's value." ), 1, NULL, "s", "value", "Value to set." );
const idEventDef EV_CVar_SetVectorValue( "setVectorValue", '\0', DOC_TEXT( "Converts the given value to a string and sets it as the cvar's value." ), 1, NULL, "v", "value", "Value to set." );
const idEventDef EV_CVar_SetIntValue( "setIntValue", '\0', DOC_TEXT( "Converts the given value to a string and sets it as the cvar's value." ), 1, NULL, "d", "value", "Value to set." );
const idEventDef EV_CVar_SetBoolValue( "setBoolValue", '\0', DOC_TEXT( "Converts the given value to a string and sets it as the cvar's value." ), 1, NULL, "b", "value", "Value to set." );

const idEventDef EV_CVar_SetCallback( "setCallback", '\0', DOC_TEXT( "Specifies a global namespace function to be called whenever the value of this cvar changes." ), 1, "Only one callback may be active per wrapper at a time.\nSupplying a class method as the callback will have undefined behaviour.", "s", "name", "Name of the function to use as a callback." );

CLASS_DECLARATION( idClass, sdCVarWrapper )
	EVENT( EV_CVar_GetFloatValue,	sdCVarWrapper::Event_GetFloatValue )
	EVENT( EV_CVar_GetStringValue,	sdCVarWrapper::Event_GetStringValue )
	EVENT( EV_CVar_GetVectorValue,	sdCVarWrapper::Event_GetVectorValue )
	EVENT( EV_CVar_GetIntValue,		sdCVarWrapper::Event_GetIntValue )
	EVENT( EV_CVar_GetBoolValue,	sdCVarWrapper::Event_GetBoolValue )

	EVENT( EV_CVar_SetFloatValue,	sdCVarWrapper::Event_SetFloatValue )
	EVENT( EV_CVar_SetStringValue,	sdCVarWrapper::Event_SetStringValue )
	EVENT( EV_CVar_SetVectorValue,	sdCVarWrapper::Event_SetVectorValue )
	EVENT( EV_CVar_SetIntValue,		sdCVarWrapper::Event_SetIntValue )
	EVENT( EV_CVar_SetBoolValue,	sdCVarWrapper::Event_SetBoolValue )

	EVENT( EV_CVar_SetCallback,		sdCVarWrapper::Event_SetCallback )
END_CLASS

/*
===========
sdCVarWrapper::sdCVarWrapper
============
*/
sdCVarWrapper::sdCVarWrapper( void ) : _scriptObject( NULL ), _cvar( NULL ), _callbackFunction( NULL ) {
	_callback.Init( this );
}

/*
===========
sdCVarWrapper::Init
============
*/
void sdCVarWrapper::Init( const char* cvarName, const char* defaultValue ) {
	_cvar = cvarSystem->Find( cvarName );
	if ( !_cvar ) {
		cvarSystem->SetCVarString( cvarName, defaultValue, CVAR_GAME );

		_cvar = cvarSystem->Find( cvarName );
		if ( !_cvar ) {
			gameLocal.Error( "sdCVarWrapper::sdCVarWrapper Failed to Create CVar '%s'", defaultValue );
		}
	}
	_scriptObject = gameLocal.program->AllocScriptObject( this, "cvar" );
}

/*
===========
sdCVarWrapper::~sdCVarWrapper
============
*/
sdCVarWrapper::~sdCVarWrapper( void ) {
	if ( _callbackFunction ) {
		_cvar->UnRegisterCallback( &_callback );
	}

	gameLocal.program->FreeScriptObject( _scriptObject );
}

/*
===========
sdCVarWrapper::OnChanged
============
*/
void sdCVarWrapper::OnChanged( void ) {
	assert( _callbackFunction );
	gameLocal.CallFrameCommand( _callbackFunction );
}

/*
===========
sdCVarWrapper::Event_GetFloatValue
============
*/
void sdCVarWrapper::Event_GetFloatValue( void ) {
	sdProgram::ReturnFloat( _cvar->GetFloat() );
}

/*
===========
sdCVarWrapper::Event_GetStringValue
============
*/
void sdCVarWrapper::Event_GetStringValue( void ) {
	sdProgram::ReturnString( _cvar->GetString() );
}

/*
===========
sdCVarWrapper::Event_GetVectorValue
============
*/
void sdCVarWrapper::Event_GetVectorValue( void ) {
	idVec3 retVal;
	sdProperties::sdFromString( retVal, _cvar->GetString() );
	sdProgram::ReturnVector( retVal );
}

/*
===========
sdCVarWrapper::Event_GetIntValue
============
*/
void sdCVarWrapper::Event_GetIntValue( void ) {
	sdProgram::ReturnInteger( _cvar->GetInteger() );
}

/*
===========
sdCVarWrapper::Event_GetBoolValue
============
*/
void sdCVarWrapper::Event_GetBoolValue( void ) {
	sdProgram::ReturnBoolean( _cvar->GetBool() );
}

/*
===========
sdCVarWrapper::Event_SetBoolValue
============
*/
void sdCVarWrapper::Event_SetFloatValue( float value ) {
	_cvar->SetFloat( value );
}

/*
===========
sdCVarWrapper::Event_SetBoolValue
============
*/
void sdCVarWrapper::Event_SetStringValue( const char* value ) {
	_cvar->SetString( value );
}

/*
===========
sdCVarWrapper::Event_SetBoolValue
============
*/
void sdCVarWrapper::Event_SetIntValue( int value ) {
	_cvar->SetInteger( value );
}

/*
===========
sdCVarWrapper::Event_SetBoolValue
============
*/
void sdCVarWrapper::Event_SetBoolValue( bool value ) {
	_cvar->SetBool( value );
}

/*
===========
sdCVarWrapper::Event_SetVectorValue
============
*/
void sdCVarWrapper::Event_SetVectorValue( const idVec3& value ) {
	_cvar->SetString( value.ToString( 5 ) );
}

/*
===========
sdCVarWrapper::Event_SetCallback
============
*/
void sdCVarWrapper::Event_SetCallback( const char* callback ) {
	if ( _callbackFunction ) {
		_cvar->UnRegisterCallback( &_callback );
	}

	_callbackFunction = gameLocal.program->FindFunction( callback );
	if ( !_callbackFunction ) {
		gameLocal.Warning( "sdCVarWrapper::Event_SetCallback Failed to Find Callback '%s'", callback );
		return;
	}

	_cvar->RegisterCallback( &_callback );
}
