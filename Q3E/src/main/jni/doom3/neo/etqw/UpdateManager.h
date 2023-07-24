// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __UPDATE_MANAGER_H__
#define __UPDATE_MANAGER_H__

#include "guis/UserInterfaceTypes.h"
#include "../framework/async/AsyncUpdates.h"

class sdUpdateManager : public sdUIPropertyHolder {
public:
	typedef sdUITemplateFunction< sdUpdateManager > uiFunction_t;
											sdUpdateManager( void );
											~sdUpdateManager( void );

	virtual sdProperties::sdProperty*		GetProperty( const char* name );
	virtual sdProperties::sdProperty*		GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler& GetProperties() { return properties; }
	virtual const char*						GetName() const { return "updateProperties"; }
	virtual const char*						FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return properties.NameForProperty( property ); }

	sdUIFunctionInstance*					GetFunction( const char* name );

	void									Update( void );
	void									Init( void );
	void									Shutdown( void );	
	void									SetAvailability( updateAvailability_t availability ) { this->availability = availability; }
	void									SetUpdateProgress( float progress ) { this->progress = progress; }
	void									SetUpdateState( updateState_t state ) { this->state = state; }
	void									SetUpdateFromServer( bool fromServer ) { this->fromServer = fromServer ? 1.f : 0.f; }

	void									Script_SetResponse( sdUIFunctionStack& stack );

											// this also clears the response to UPDATE_GUI_NONE
	guiUpdateResponse_t						GetResponse();

	void									SetUpdateMessage( const wchar_t* message ) { this->message = message; }

private:
	static void								InitFunctions();
	static void								ShutdownFunctions();
	static uiFunction_t*					FindFunction( const char* name );

private:
	static idHashMap< uiFunction_t* >		uiFunctions;
	
	sdWStringProperty						message;
	sdFloatProperty							state;
	sdFloatProperty							availability;
	sdFloatProperty							progress;
	sdFloatProperty							fromServer;

	guiUpdateResponse_t						response;


	sdProperties::sdPropertyHandler			properties;
};

#endif // __UPDATE_MANAGER_H__
