// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_GUIS_USERINTERFACEBINDER_H__
#define __GAME_GUIS_USERINTERFACEBINDER_H__

#include "UserInterfaceTypes.h"

extern const char sdUITemplateFunctionInstance_IdentifierBinder[];

/*
============
sdUIBinder
============
*/
SD_UI_PROPERTY_TAG(
alias = "binder";
)
class sdUIBinder :
	public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUIBinder )

	typedef enum bindEvent_e {
		BE_BIND_COMPLETE = WE_NUM_EVENTS + 1,
		BE_NUM_EVENTS,
	} bindEvent_t;

							sdUIBinder();
	virtual					~sdUIBinder() { DisconnectGlobalCallbacks();}

	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	virtual const char*		GetScopeClassName() const { return "sdUIBinder"; }

	virtual bool			PostEvent( const sdSysEvent* event );
	virtual void			EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );	
	virtual int				GetMaxEventTypes( void ) const { return BE_NUM_EVENTS; }
	virtual const char*		GetEventNameForType( int event ) const { return ( event < ( WE_NUM_EVENTS + 1 )) ? sdUIWindow::GetEventNameForType( event ): eventNames[ event - WE_NUM_EVENTS - 1 ]; }

	static void										InitFunctions( void );
	static void										ShutdownFunctions( void ) { binderFunctions.DeleteContents(); }
	static const sdUITemplateFunction< sdUIBinder >*FindFunction( const char* name );

	void					Script_ApplyBinding( sdUIFunctionStack& stack );
	void					Script_UnbindBinding( sdUIFunctionStack& stack );

private:
	SD_UI_PROPERTY_TAG(
	title				= "Object/Bind Command";
	desc				= "Command to bind.";
	editor				= "edit";
	datatype			= "wstring";
	)
	sdWStringProperty		bindCommand;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Object/Bind Command";
	desc				= "Command to bind.";
	editor				= "edit";
	datatype			= "wstring";
	)
	sdWStringProperty	bindMessage;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Object/Bind Index";
	desc				= "Index to rebind";
	editor				= "edit";
	datatype			= "string";
	)
	sdFloatProperty		bindIndex;
	// ===========================================

	static const char* const eventNames[ BE_NUM_EVENTS - WE_NUM_EVENTS ];
	static idHashMap< sdUITemplateFunction< sdUIBinder >* >	binderFunctions;

	idKey* currentKey;
};


#endif // ! __GAME_GUIS_USERINTERFACEBINDER_H__
