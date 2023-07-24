// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UpdateManager.h"
#include "../framework/async/AsyncUpdates.h"
#include "guis/UserInterfaceLocal.h"

idHashMap< sdUpdateManager::uiFunction_t* > sdUpdateManager::uiFunctions;

/*
===============================================================================

	sdUpdateManager

===============================================================================
*/

/*
============
sdUpdateManager::sdUpdateManager
============
*/
sdUpdateManager::sdUpdateManager( void ) {
}

/*
============
sdUpdateManager::~sdUpdateManager
============
*/
sdUpdateManager::~sdUpdateManager( void ) {
}

/*
============
sdUpdateManager::GetProperty
============
*/
sdProperties::sdProperty* sdUpdateManager::GetProperty( const char* name ) {
	return properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
============
sdUpdateManager::GetProperty
============
*/
sdProperties::sdProperty* sdUpdateManager::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop && prop->GetValueType() != type && type != sdProperties::PT_INVALID ) {
		gameLocal.Error( "sdUpdateManager::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
============
sdUpdateManager::Init
============
*/
void sdUpdateManager::Init( void ) {
	properties.RegisterProperty( "availability",			availability );
	properties.RegisterProperty( "progress",				progress );
	properties.RegisterProperty( "state",					state );
	properties.RegisterProperty( "message",					message );
	properties.RegisterProperty( "fromserver",				fromServer );

	sdDeclGUI::AddDefine( va( "UPDATE_AVAIL_UNKNOWN %i",			UPDATE_AVAIL_UNKNOWN ) );
	sdDeclGUI::AddDefine( va( "UPDATE_AVAIL_NOREPLY %i",			UPDATE_AVAIL_NOREPLY ) );
	sdDeclGUI::AddDefine( va( "UPDATE_AVAIL_NONE %i",				UPDATE_AVAIL_NONE ) );
	sdDeclGUI::AddDefine( va( "UPDATE_AVAIL_WEB %i",				UPDATE_AVAIL_WEB ) );
	sdDeclGUI::AddDefine( va( "UPDATE_AVAIL_WEB_REQUIRED %i",		UPDATE_AVAIL_WEB_REQUIRED ) );
	sdDeclGUI::AddDefine( va( "UPDATE_AVAIL_SOFT %i",				UPDATE_AVAIL_SOFT ) );
	sdDeclGUI::AddDefine( va( "UPDATE_AVAIL_REQUIRED %i",			UPDATE_AVAIL_REQUIRED ) );

	sdDeclGUI::AddDefine( va( "UPDATE_GUI_NONE %i",					UPDATE_GUI_NONE ) );
	sdDeclGUI::AddDefine( va( "UPDATE_GUI_DOWNLOAD %i",				UPDATE_GUI_DOWNLOAD ) );
	sdDeclGUI::AddDefine( va( "UPDATE_GUI_WEBSITE %i",				UPDATE_GUI_WEBSITE ) );
	sdDeclGUI::AddDefine( va( "UPDATE_GUI_LATER %i",				UPDATE_GUI_LATER ) );
	sdDeclGUI::AddDefine( va( "UPDATE_GUI_YES %i",					UPDATE_GUI_YES ) );
	sdDeclGUI::AddDefine( va( "UPDATE_GUI_NO %i",					UPDATE_GUI_NO ) );
	sdDeclGUI::AddDefine( va( "UPDATE_GUI_CANCEL %i",				UPDATE_GUI_CANCEL ) );

	sdDeclGUI::AddDefine( va( "UPDATE_IDLE %i", 					UPDATE_IDLE ) );	
	sdDeclGUI::AddDefine( va( "UPDATE_WAITING %i", 					UPDATE_WAITING ) );			
//	sdDeclGUI::AddDefine( va( "UPDATE_PROCESS_UPDATE %i", 			UPDATE_PROCESS_UPDATE ) );	
//	sdDeclGUI::AddDefine( va( "UPDATE_INITIATE_DOWNLOAD %i",		UPDATE_INITIATE_DOWNLOAD ) );
	sdDeclGUI::AddDefine( va( "UPDATE_REMINDING %i", 				UPDATE_REMINDING ) );
//	sdDeclGUI::AddDefine( va( "UPDATE_PROMPTING_SETUP %i", 			UPDATE_PROMPTING_SETUP ) );	
	sdDeclGUI::AddDefine( va( "UPDATE_PROMPTING %i", 				UPDATE_PROMPTING ) );
	sdDeclGUI::AddDefine( va( "UPDATE_DOWNLOADING %i", 				UPDATE_DOWNLOADING ) );
	sdDeclGUI::AddDefine( va( "UPDATE_DOWNLOAD_FAILED %i", 			UPDATE_DOWNLOAD_FAILED ) );
	sdDeclGUI::AddDefine( va( "UPDATE_PROMPT_DL_FAILED %i",			UPDATE_PROMPT_DL_FAILED ) );
//	sdDeclGUI::AddDefine( va( "UPDATE_DOWNLOAD_DONE %i", 			UPDATE_DOWNLOAD_DONE ) );
//	sdDeclGUI::AddDefine( va( "UPDATE_EXECUTE %i", 					UPDATE_EXECUTE ) );
//	sdDeclGUI::AddDefine( va( "UPDATE_REMIND_READY %i", 			UPDATE_REMIND_READY ) );
	sdDeclGUI::AddDefine( va( "UPDATE_PROMPT_READY %i",				UPDATE_PROMPT_READY ) );

	// Initialize functions
	InitFunctions();
}


/*
============
sdUpdateManager::Shutdown
============
*/
void sdUpdateManager::Shutdown( void ) {
	properties.Clear();
	ShutdownFunctions();
}

/*
============
sdUpdateManager::Update
============
*/
void sdUpdateManager::Update( void ) {
	
}

/*
================
sdUpdateManager::InitFunctions
================
*/

#define ALLOC_FUNC( name, returntype, parms, function ) uiFunctions.Set( name, new uiFunction_t( returntype, parms, function ) )
void sdUpdateManager::InitFunctions() {
	ALLOC_FUNC( "setResponse",				'v', "f",		&sdUpdateManager::Script_SetResponse );
}

#undef ALLOC_FUNC

/*
================
sdUpdateManager::ShutdownFunctions
================
*/
void sdUpdateManager::ShutdownFunctions() {
	uiFunctions.DeleteContents();
}

/*
================
sdUpdateManager::FindFunction
================
*/
sdUpdateManager::uiFunction_t* sdUpdateManager::FindFunction( const char* name ) {
	uiFunction_t** ptr;
	return uiFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUpdateManager::GetFunction
============
*/
sdUIFunctionInstance* sdUpdateManager::GetFunction( const char* name ) {
	uiFunction_t* function = FindFunction( name );
	if ( function == NULL ) {
		return NULL;
	}

	return new sdUITemplateFunctionInstance< sdUpdateManager, sdUITemplateFunctionInstance_Identifier >( this, function );
}

/*
============
sdUpdateManager::Script_SetResponse
============
*/
void sdUpdateManager::Script_SetResponse( sdUIFunctionStack& stack ) {
	int iResponse;
	stack.Pop( iResponse );

	response = UPDATE_GUI_NONE;
	if( !sdIntToContinuousEnum( iResponse, UPDATE_GUI_MIN, UPDATE_GUI_MAX, response )) {
		gameLocal.Warning( "sdUpdateManager::SetResponse: invalid value '%i'", iResponse );
	}
}

/*
============
sdUpdateManager::GetResponse
============
*/
guiUpdateResponse_t sdUpdateManager::GetResponse() {
	guiUpdateResponse_t toReturn = response;
	response = UPDATE_GUI_NONE;
	return toReturn;
}
