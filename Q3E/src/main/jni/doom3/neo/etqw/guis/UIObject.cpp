// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceManager.h"
#include "UIObject.h"
#include "UserInterfaceLocal.h"


const char sdUITemplateFunctionInstance_IdentifierObject[] = "sdUIObjectFunction";

using namespace sdProperties;

idHashMap< sdUITemplateFunction< sdUIObject >* >	sdUIObject::objectFunctions;

SD_UI_IMPLEMENT_ABSTRACT_CLASS( sdUIObject, )
SD_UI_IMPLEMENT_ABSTRACT_CLASS( sdUIObject_Drawable, sdUIObject )

/*
============
sdUIObject::sdUIObject
============
*/
sdUIObject::sdUIObject() : ui( NULL ) {
	hierarchyNode.SetOwner( this );
	scriptState.Init( this );

	scriptState.				GetProperties().RegisterProperty( "name",					name );	
	scriptState.				GetProperties().RegisterProperty( "flags",					flags );
	flags = 0.0f;

}


/*
============
sdUIObject::~sdUIObject
============
*/
sdUIObject::~sdUIObject() {
	events.Clear();
}

/*
============
sdUIObject::GetFunction
============
*/
sdUIFunctionInstance* sdUIObject::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIObject >* function = sdUIObject::FindFunction( name );
	if( function == NULL ) {		
		return NULL;
	}

	return new sdUITemplateFunctionInstance< sdUIObject, sdUITemplateFunctionInstance_IdentifierObject >( this, function );
}

/*
============
sdUIObject::FindFunction
============
*/
const sdUITemplateFunction< sdUIObject >* sdUIObject::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIObject >** ptr;
	return objectFunctions.Get( name, &ptr ) ? *ptr : NULL;
}


/*
============
sdUIObject::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIObject )
void sdUIObject::InitFunctions() {
	SD_UI_FUNC_TAG( setParent, "Sets a new parent for the window." )
		SD_UI_FUNC_PARM( string, "other", "Name of the new parent." )
	SD_UI_END_FUNC_TAG
	objectFunctions.Set( "setParent",					new sdUITemplateFunction< sdUIObject >( 'v', "s",		&sdUIObject::Script_SetParent ) );	

	SD_UI_FUNC_TAG( isChild, "Checks if the window is a child (possibly indirectly a child) of the the window supplied as the first parameter." )
		SD_UI_FUNC_PARM( string, "other", "Name of the other window." )
		SD_UI_FUNC_RETURN_PARM( float, "Returns 1 if a child, otherwise 0." )
	SD_UI_END_FUNC_TAG
	objectFunctions.Set( "isChild",						new sdUITemplateFunction< sdUIObject >( 'f', "s",		&sdUIObject::Script_IsChild ) );
	
	SD_UI_FUNC_TAG( changeZOrder, "Change the drawing order for the window." )
		SD_UI_FUNC_PARM( float, "zorder", "ZO_FRONT or ZO_BACK for drawing at front or back respectively." )
	SD_UI_END_FUNC_TAG
	objectFunctions.Set( "changeZOrder",				new sdUITemplateFunction< sdUIObject >( 'v', "f",		&sdUIObject::Script_ChangeZOrder ) );	
	
	SD_UI_FUNC_TAG( postNamedEvent, "Post a named event on the window." )
		SD_UI_FUNC_PARM( string, "eventName", "Name of the named event." )
	SD_UI_END_FUNC_TAG
	objectFunctions.Set( "postNamedEvent",				new sdUITemplateFunction< sdUIObject >( 'v', "s",		&sdUIObject::Script_PostNamedEvent ) );
	
	SD_UI_FUNC_TAG( postOptionalNamedEvent, "Post a named event on the window. Will not give error if the named event does not exist." )
		SD_UI_FUNC_PARM( string, "eventName", "Name of the named event." )
	SD_UI_END_FUNC_TAG
	objectFunctions.Set( "postOptionalNamedEvent",		new sdUITemplateFunction< sdUIObject >( 'v', "s",		&sdUIObject::Script_PostOptionalNamedEvent ) );	

	SD_UI_ENUM_TAG( ZO_FRONT, "When used together with script event changeZOrder; Draw before all siblings have been drawn." )
	sdDeclGUI::AddDefine( va( "ZO_FRONT %i",		ZO_FRONT ) );

	SD_UI_ENUM_TAG( ZO_BACK, "When used together with script event changeZOrder; Draw after all siblings have been drawn." )
	sdDeclGUI::AddDefine( va( "ZO_BACK %i",			ZO_BACK ) );

	SD_UI_ENUM_TAG( OF_FIXED_LAYOUT, "Object should be immune to any potential layout changes." )
	sdDeclGUI::AddDefine( va( "OF_FIXED_LAYOUT %i", OF_FIXED_LAYOUT ) );
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUIObject::EnumerateEvents
============
*/
void sdUIObject::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if( !idStr::Icmp( name, "onCreate" ) ) {
		events.Append( sdUIEventInfo( OE_CREATE, 0 ));
		return;
	}

	if( !idStr::Icmp( name, "onNamedEvent" ) ) {
		if( flags.Empty() ) {
			gameLocal.Error( "Event 'onNamedEvent' no event specified" );
		}

		int i;
		for( i = 0; i < flags.Num(); i++ ) {
			const idToken& name = tokenCache[ flags[ i ] ];
			events.Append( sdUIEventInfo( OE_NAMED, NamedEventHandleForString( name.c_str() ) ) );
		}
		return;
	}

	if( !idStr::Icmp( name, "onCVarChanged" ) ) {
		if( flags.Empty() ) {
			gameLocal.Error( "Event 'onCVarChanged' no cvar specified" );
		}
		int i;
		for( i = 0; i < flags.Num(); i++ ) {
			const idToken& name = tokenCache[ flags[ i ] ];
			idCVar* cvar = cvarSystem->Find( name.c_str() );
			if( cvar == NULL ) {
				gameLocal.Error( "Event 'onCVarChanged' could not find cvar '%s'", name.c_str() );
				return;
			}

			int eventHandle = NamedEventHandleForString( name.c_str() );
			cvarCallback_t* callback = new cvarCallback_t( *this, *cvar, eventHandle );
			cvarCallbacks.Append( callback );

			events.Append( sdUIEventInfo( OE_CVARCHANGED, eventHandle ) );
		}
		return;
	}

	if( !idStr::Icmp( name, "onPropertyChanged" ) ) {
		if( flags.Empty() ) {
			gameLocal.Error( "Event 'onPropertyChanged' no property specified" );
		}
		int i;
		for( i = 0; i < flags.Num(); i++ ) {
			//sdProperty* prop = scriptState.GetProperty( flags[ i ] );
			// do a proper lookup, so windows can watch guis and vice-versa

			const idToken& name = tokenCache[ flags[ i ] ];
			idLexer p( sdDeclGUI::LEXER_FLAGS );
			p.LoadMemory( name.c_str(), name.Length(), "onPropertyChanged event handler" );			

			sdUserInterfaceScope* propertyScope = gameLocal.GetUserInterfaceScope( GetScope(), &p );

			idToken token;
			p.ReadToken( &token );

			sdProperty* prop = propertyScope->GetProperty( token );

			if( !prop ) {
				gameLocal.Error( "Event 'onPropertyChanged' could not find property '%s'", name.c_str() );
				return;
			}

			int eventHandle = NamedEventHandleForString( name.c_str() );
			int cbHandle = -1;
			switch( prop->GetValueType() ) {
				case PT_VEC4: 
					cbHandle = prop->value.vec4Value->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idVec4&, const idVec4& >( &sdUIObject::OnVec4PropertyChanged, this , eventHandle ) );					
					break;
				case PT_VEC3: 
					cbHandle = prop->value.vec3Value->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idVec3&, const idVec3& >( &sdUIObject::OnVec3PropertyChanged, this , eventHandle ) );					
					break;
				case PT_VEC2: 
					cbHandle = prop->value.vec2Value->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idVec2&, const idVec2& >( &sdUIObject::OnVec2PropertyChanged, this , eventHandle ) );					
					break;
				case PT_INT: 
					cbHandle = prop->value.intValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const int&, const int& >( &sdUIObject::OnIntPropertyChanged, this , eventHandle ) );
					break;
				case PT_FLOAT: 
					cbHandle = prop->value.floatValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const float&, const float& >( &sdUIObject::OnFloatPropertyChanged, this , eventHandle ) );
					break;
				case PT_STRING: 
					cbHandle = prop->value.stringValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idStr&, const idStr& >( &sdUIObject::OnStringPropertyChanged, this , eventHandle ) );
					break;
				case PT_WSTRING: 
					cbHandle = prop->value.wstringValue->AddOnChangeHandler( sdFunctions::sdBindMem1< void, int, const idWStr&, const idWStr& >( &sdUIObject::OnWStringPropertyChanged, this , eventHandle ) );
					break;
			}
			ConnectGlobalCallback( callbackHandler_t( prop, cbHandle ) );

			events.Append( sdUIEventInfo( OE_PROPCHANGED, eventHandle ) );
		}
		return;
	}
	gameLocal.Error( "'%s' unknown event '%s'", this->name.GetValue().c_str(), name );
}

SD_UI_PUSH_CLASS_TAG( sdUIObject )
const char* sdUIObject::eventNames[ OE_NUM_EVENTS ] = {
		"<NULL>",
		"<constructor>",
		SD_UI_EVENT_TAG( "onCreate",					"",						"Called on window creation" ),
		SD_UI_EVENT_TAG( "onNamedEvent",				"[Event ...]",			"Called when one of the events specified occurs" ),
		SD_UI_EVENT_TAG( "onPropertyChanged",			"[Property ...]",		"Called when one of the properties specified occurs" ),
		SD_UI_EVENT_TAG( "onCVarChanged",				"[CVar ...]",			"Called when one of the CVars' value changes" ),
};
SD_UI_POP_CLASS_TAG

/*
================
sdUIObject::PropertyTypeForName
================
*/
sdProperties::ePropertyType sdUIObject::PropertyTypeForName( const char* typeName ) {
	if( !idStr::Icmp( typeName, "string" ) ) {
		return sdProperties::PT_STRING;
	}

	if( !idStr::Icmp( typeName, "wstring" ) ) {
		return sdProperties::PT_WSTRING;
	}

	if( !idStr::Icmp( typeName, "handle" ) ) {
		return sdProperties::PT_INT;
	}

	if( !idStr::Icmp( typeName, "float" ) ) {
		return sdProperties::PT_FLOAT;
	}

	if( !idStr::Icmp( typeName, "vec2" ) ) {
		return sdProperties::PT_VEC2;
	}

	if( !idStr::Icmp( typeName, "vec3" ) || !idStr::Icmp( typeName, "vector" ) ) {
		return sdProperties::PT_VEC3;
	}

	if( !idStr::Icmp( typeName, "vec4" ) || !idStr::Icmp( typeName, "color" ) || !idStr::Icmp( typeName, "rect" ) ) {
		return sdProperties::PT_VEC4;
	}

	gameLocal.Error( "Invalid Property Type '%s'", typeName );
	return sdProperties::PT_BOOL;
}

/*
================
sdUIObject::CreateProperties
================
*/
void sdUIObject::CreateProperties( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, const idTokenCache& tokenCache ) {
	DisconnectGlobalCallbacks();

	ui = _ui;
	InitProperties();

	name = windowParms->GetName();
	name.SetReadOnly( true );

	SetupProperties( scriptState.GetProperties(), windowParms->GetProperties(), ui, tokenCache );

	// default to being owned by the desktop
	sdUIObject* desktop = ui->GetWindow( "desktop" );
	if ( desktop ) {
		SetParent( desktop );
	}
}

/*
================
sdUIObject::Create
================
*/
void sdUIObject::SetupProperties( sdProperties::sdPropertyHandler& handler, const idList< sdDeclGUIProperty* >& properties, sdUserInterfaceLocal* ui, const idTokenCache& tokenCache ) {
	int i;
	for( i = 0; i < properties.Num(); i++ ) {
		const sdDeclGUIProperty* property = properties[ i ];
		sdUserInterfaceLocal::PushTrace( tokenCache[ property->GetName() ] );

		sdProperties::ePropertyType type = PropertyTypeForName( tokenCache[ property->GetType() ] );

		sdProperties::sdProperty* other = handler.GetProperty( tokenCache[ property->GetName() ], PT_INVALID, false );
		if( other ) {
			if( other->GetValueType() != type ) {
				gameLocal.Error( "Redefinition of Type of Existing Property Not Allowed on '%s'", tokenCache[ property->GetName() ].c_str() );
			}
			// already exists so no need to create another one
		} else {

			const idToken& name = tokenCache[ property->GetName() ];
			switch( type ) {
				case sdProperties::PT_INT: {
					handler.RegisterProperty( name.c_str(), *ui->AllocIntProperty() );
					break;
				}
				case sdProperties::PT_FLOAT: {
					handler.RegisterProperty( name.c_str(), *ui->AllocFloatProperty() );
					break;
				}
				case sdProperties::PT_VEC2: {
					handler.RegisterProperty( name.c_str(), *ui->AllocVec2Property() );
					break;
				}
				case sdProperties::PT_VEC3: {
					handler.RegisterProperty( name.c_str(), *ui->AllocVec3Property() );
					break;
				}
				case sdProperties::PT_VEC4: {
					handler.RegisterProperty( name.c_str(), *ui->AllocVec4Property() );
					break;
				}
				case sdProperties::PT_STRING: {
					handler.RegisterProperty( name.c_str(), *ui->AllocStringProperty() );
					break;
				}
				case sdProperties::PT_WSTRING: {
					handler.RegisterProperty( name.c_str(), *ui->AllocWStringProperty() );
					break;
				}
			}
		}
		sdUserInterfaceLocal::PopTrace();
	}
}

/*
================
sdUIObject::CreateTimelines
================
*/
void sdUIObject::CreateTimelines( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, idTokenCache& tokenCache ) {
	if( windowParms->GetTimelines().GetNumTimelines() > 0 ) {
		timelines.Reset( new sdUITimelineManager( *_ui, scriptState, _ui->GetScript() ) );
		timelines->CreateTimelines( windowParms->GetTimelines(), ui->GetDecl() );
		timelines->CreateProperties( windowParms->GetTimelines(), ui->GetDecl(), tokenCache );
		_ui->RegisterTimelineWindow( this );
	}
}


/*
============
sdUIObject::InitEvents
============
*/
void sdUIObject::InitEvents() {
	events.Clear();
	events.SetNumEvents( GetMaxEventTypes() );
}

/*
================
sdUIObject::CreateEvents
================
*/
void sdUIObject::CreateEvents( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, idTokenCache& tokenCache ) {
	const idList< sdDeclGUIProperty* >& windowProperties = windowParms->GetProperties();

	assert( !events.Empty() );

	namedEvents.Clear();

	sdUserInterfaceLocal::PushTrace( windowParms->GetName() );

	idList<unsigned short> constructorTokens;
	bool hasValues = sdDeclGUI::CreateConstructor( windowProperties, constructorTokens, tokenCache );

	if ( hasValues ) {
		sdUserInterfaceLocal::PushTrace( "<constructor>" );
		idLexer parser( sdDeclGUI::LEXER_FLAGS );
		parser.LoadTokenStream( constructorTokens, tokenCache, "sdUIObject::CreateEvents" );

		sdUIEventInfo constructionEvent( OE_CONSTRUCTOR, 0 );
		ui->GetScript().ParseEvent( &parser, constructionEvent, &scriptState );
		RunEvent( constructionEvent );
		sdUserInterfaceLocal::PopTrace();
	}

	idList< sdUIEventInfo > eventList;

	const idList< sdDeclGUIEvent* >& windowEvents = windowParms->GetEvents();
	for ( int i = 0; i < windowEvents.Num(); i++ ) {
		const sdDeclGUIEvent* eventInfo = windowEvents[ i ];
		sdUserInterfaceLocal::PushTrace( tokenCache[ eventInfo->GetName() ] );

		eventList.Clear();
		EnumerateEvents( tokenCache[ eventInfo->GetName() ], eventInfo->GetFlags(), eventList, tokenCache );

		int j;
		for( j = 0; j < eventList.Num(); j++ ) {
			idLexer parser( sdDeclGUI::LEXER_FLAGS );
			parser.LoadTokenStream( eventInfo->GetTokenIndices(), tokenCache, va( "windowDef '%s', event '%s'", windowParms->GetName(), tokenCache[ eventInfo->GetName() ].c_str() ) );
			//parser.LoadMemory( eventInfo->GetText(), eventInfo->GetTextLength(), va( "windowDef '%s', event '%s'", windowParms->GetName(), eventInfo->GetName()) );

			ui->GetScript().ParseEvent( &parser, eventList[ j ], &scriptState );
		}
		sdUserInterfaceLocal::PopTrace();
	}

	if( timelines.Get() != NULL ) {
		timelines->CreateEvents( windowParms->GetTimelines(), ui->GetDecl(), tokenCache );
	}
	
	sdUserInterfaceLocal::PopTrace();	
}


/*
================
sdUIObject::AddEvent
================
*/
void sdUIObject::AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) {
	events.AddEvent( info, scriptHandle );
}

/*
================
sdUIObject::GetEvent
================
*/
sdUIEventHandle sdUIObject::GetEvent( const sdUIEventInfo& info ) const {
	return events.GetEvent( info );
}

/*
============
sdUIObject::Script_SetParent
============
*/
void sdUIObject::Script_SetParent( sdUIFunctionStack& stack ) {
	idStr parentName;
	stack.Pop( parentName );
	sdUIObject* parent = ui->GetWindow( parentName );
	if( parent == NULL ) {
		gameLocal.Error( "could not find parent named '%s'", parentName.c_str() );
		return;
	}

	if( parent == this ) {
		gameLocal.Error( "'%s' cannot parent to self", parentName.c_str() );
		return;
	}

	SetParent( parent );
}

/*
============
sdUIObject::Script_PostNamedEvent
============
*/
void sdUIObject::Script_PostNamedEvent( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );
	PostNamedEvent( name.c_str(), false );
}

/*
============
sdUIObject::Script_PostOptionalNamedEvent
============
*/
void sdUIObject::Script_PostOptionalNamedEvent( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );
	PostNamedEvent( name.c_str(), true );
}

/*
============
sdUIObject::RunNamedMethod
============
*/
bool sdUIObject::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUITemplateFunction< sdUIObject >* func = sdUIObject::FindFunction( name );
	if( !func ) {
		return false;
	}
	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}


/*
============
sdUIObject::PostNamedEvent
============
*/
bool sdUIObject::PostNamedEvent( const char* event, bool optional ) {
	assert( event != NULL );

	int index = namedEvents.FindIndex( event );
	if( index == -1 ) {
		if( optional == false ) {
			gameLocal.Error( "Could not find event '%s' in '%s' in '%s'", event, name.GetValue().c_str(), ui->GetName() );
		}
		return false;
	}

	bool retVal = RunEvent( sdUIEventInfo( OE_NAMED, index ) );
	
	if( retVal && sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() > 1 ) {
		gameLocal.Printf( "GUI '%s', window '%s', event '%s'\n", ui->GetName(), name.GetValue().c_str(), event );
	}

	return retVal;
}


/*
============
sdUIObject::RunEvent
============
*/
bool sdUIObject::RunEventHandle( const sdUIEventHandle handle ) {
	return ui->GetScript().RunEventHandle( handle, &scriptState );
}

/*
============
sdUIObject::RunEvent
============
*/
bool sdUIObject::RunEvent( const sdUIEventInfo& info ) {
	return ui->GetScript().RunEventHandle( GetEvent( info ), &scriptState );
}

/*
============
sdUIObject::NamedEventHandleForString
============
*/
int sdUIObject::NamedEventHandleForString( const char* name ) {
	int index = namedEvents.FindIndex( name );
	if( index == -1 ) {
		index = namedEvents.Append( name ) ;
	}
	return index;
}

/*
============
sdUIObject::OnStringPropertyChanged
============
*/
void sdUIObject::OnStringPropertyChanged( int event, const idStr& oldValue, const idStr& newValue ) {
	RunEvent( sdUIEventInfo( OE_PROPCHANGED, event ) );
}

/*
============
sdUIObject::OnWStringPropertyChanged
============
*/
void sdUIObject::OnWStringPropertyChanged( int event, const idWStr& oldValue, const idWStr& newValue ) {
	RunEvent( sdUIEventInfo( OE_PROPCHANGED, event ) );
}

/*
============
sdUIObject::OnIntPropertyChanged
============
*/
void sdUIObject::OnIntPropertyChanged( int event, const int oldValue, const int newValue ) {
	RunEvent( sdUIEventInfo( OE_PROPCHANGED, event ) );
}

/*
============
sdUIObject::OnFloatPropertyChanged
============
*/
void sdUIObject::OnFloatPropertyChanged( int event, const float oldValue, const float newValue ) {
	RunEvent( sdUIEventInfo( OE_PROPCHANGED, event ) );
}

/*
============
sdUIObject::OnVec4PropertyChanged
============
*/
void sdUIObject::OnVec4PropertyChanged( int event, const idVec4& oldValue, const idVec4& newValue ) {
	RunEvent( sdUIEventInfo( OE_PROPCHANGED, event ) );
}

/*
============
sdUIObject::OnVec3PropertyChanged
============
*/
void sdUIObject::OnVec3PropertyChanged( int event, const idVec3& oldValue, const idVec3& newValue ) {
	RunEvent( sdUIEventInfo( OE_PROPCHANGED, event ) );
}

/*
============
sdUIObject::OnVec2PropertyChanged
============
*/
void sdUIObject::OnVec2PropertyChanged( int event, const idVec2& oldValue, const idVec2& newValue ) {
	RunEvent( sdUIEventInfo( OE_PROPCHANGED, event ) );
}

/*
============
sdUIObject::SetParent
============
*/
void sdUIObject::SetParent( sdUIObject* parent ) {
	if( this == parent || parent == GetNode().GetParent() ) {
		return;
	}

	sdUIObject* object = parent->GetNode().GetChild();
	if( !object ) {
		// insert first child
		GetNode().ParentTo( parent->GetNode() );
	} else {
		// append after the last child
		while( true ) {
			if( object->GetNode().GetSibling() ) {
				object = object->GetNode().GetSibling();
			} else {
				break;
			}
		}
		assert( object );
		GetNode().MakeSiblingAfter( object->GetNode() );
	}
}


/*
============
sdUIObject::Script_ChangeZOrder
============
*/
void sdUIObject::Script_ChangeZOrder( sdUIFunctionStack& stack ) {
	float value;
	stack.Pop( value );

	sdUIObject* object = NULL;
	sdUIObject* parent = GetNode().GetParent();
	sdUIObject* last = NULL;
	
	if( parent == NULL ) {
		return;
	}
    
	switch( idMath::Ftoi( value )) {
		case ZO_FRONT:
			GetNode().ParentTo( parent->GetNode() );
			break;
		case ZO_BACK:
			object = parent->GetNode().GetChild();
			while( object != NULL ) {
				if( object == this ) {
					object = object->GetNode().GetSibling();
					continue;
				}
				if( object->GetNode().GetSibling() ) {
					last = object;
					object = object->GetNode().GetSibling();
				} else {
					break;
				}				
			}

			if( object == this || object == NULL ) {
				object = last;
			}
			if( object == NULL ) {
				GetNode().ParentTo( parent->GetNode() );
				return;
			}
			GetNode().MakeSiblingAfter( object->GetNode() );
			break;
	}
}


bool sdUITypeInfo::initialized = false;

/*
============
sdUITypeInfo::GetTypeList
============
*/
sdUITypeInfo::typeList_t& sdUITypeInfo::GetTypeList() {
	static typeList_t types;
	return types;
}


/*
============
sdUITypeInfo::GetTypeRoot
============
*/
sdUITypeInfo::Ptr sdUITypeInfo::GetTypeRoot( Ptr root ) {
	static Ptr		typeRoot;
	if( root ) {
		typeRoot = root;
	}
	return typeRoot;
}


/*
============
sdUITypeInfo::Init
============
*/
void sdUITypeInfo::Init() {
	if( initialized ) {
		return;
	}

	typeList_t::ConstIterator iter = GetTypeList().Begin();

	common->DPrintf( "Initializing %i classes...\n", GetTypeList().Num() );

	while( iter != GetTypeList().End()) {
		Ptr type = *iter;
		if( idStr::Length( type->superName ) == 0 ) {
			if( GetTypeRoot() ) {
				throw idException( "Cannot have more than one root class" );
			}
			GetTypeRoot( type );
		} else {
			Ptr superClass = FindType( type->superName );
			type->node.ParentTo( superClass->node );
		}
		//		common->Printf( "Initializing %s -> %s\n", type->name.c_str(), type->superName.c_str() );
		++iter;
	}
	if( GetTypeRoot() ) {
		size_t id = 0;
		NumberTypes( GetTypeRoot()->node, id );
	}

	common->DPrintf( "Initialized %i classes with a root of '%s'\n", GetTypeList().Num(), GetTypeRoot() ? GetTypeRoot()->GetName() : "<empty root>" );

	initialized = true;
}


/*
============
sdUITypeInfo::Shutdown
============
*/
void sdUITypeInfo::Shutdown() {

	initialized = false;
}


/*
============
sdUITypeInfo::ListClasses
============
*/
void sdUITypeInfo::ListClasses() {
	if( !GetTypeRoot() ) {
		return;
	}

	ListClasses( GetTypeRoot()->GetNode() );
}



/*
============
sdUITypeInfo::ListClasses
============
*/
void sdUITypeInfo::ListClasses( node_t& root ) {
	sdUITypeInfo* child = root.GetChild();
	sdUITypeInfo::node_t* walker = child ? &child->node : NULL;

	while( walker ) {
		ListClasses( *walker );
		child = walker->GetSibling();
		walker = child ? &child->node : NULL;
	}
}


/*
============
sdUITypeInfo::NumberTypes
============
*/
void sdUITypeInfo::NumberTypes( node_t& root, size_t& id ) {
	root.Owner()->typeBegin = id++;

	sdUITypeInfo* child = root.GetChild();
	sdUITypeInfo::node_t* walker = child ? &child->node : NULL;

	while( walker ) {
		NumberTypes( *walker, id );
		child = walker->GetSibling();
		walker = child ? &child->node : NULL;
	}
	root.Owner()->typeEnd = id - 1;
}


/*
============
sdUITypeInfo::FindType
============
*/
sdUITypeInfo::Ptr sdUITypeInfo::FindType( const char* typeName, bool allowNotFound ) {
	typeList_t::ConstIterator iter = GetTypeList().Begin();

	while( iter != GetTypeList().End()) {
		if( idStr::Cmp( typeName, (*iter)->GetName() ) == 0 ) {
			break;
		}
		++iter;
	}
	if( iter == GetTypeList().End() ) {
		if( !allowNotFound ) {
			throw idException( va( "Unknown type %s", typeName ));
		}
		return Ptr();
	}
	return *iter;
}

/*
============
sdUIObject::OnCVarChanged
============
*/
void sdUIObject::OnCVarChanged( idCVar& cvar, int id ) {
	bool result = RunEvent( sdUIEventInfo( OE_CVARCHANGED, id ) );
	if( result && sdUserInterfaceLocal::g_debugGUIEvents.GetInteger() ) {
		gameLocal.Printf( "%s: OnCVarChanged\n", name.GetValue().c_str() );
	}
}

/*
============
sdUIObject::ApplyLayout
============
*/
void sdUIObject::ApplyLayout() {
	sdUIObject* child = GetNode().GetChild();
	while( child != NULL ) {
		child->ApplyLayout();
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIObject::MakeLayoutDirty_r
============
*/
void sdUIObject::MakeLayoutDirty_r( sdUIObject* parent ) {	
	sdUIObject* child = parent->GetNode().GetChild();
	while( child != NULL ) {
		child->MakeLayoutDirty();

		MakeLayoutDirty_r( child );
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIObject::OnLanguageInit_r
============
*/
void sdUIObject::OnLanguageInit_r( sdUIObject* parent ) {
	sdUIObject* child = parent->GetNode().GetChild();
	while ( child != NULL ) {
		child->OnLanguageInit();

		OnLanguageInit_r( child );
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIObject::OnLanguageShutdown_r
============
*/
void sdUIObject::OnLanguageShutdown_r( sdUIObject* parent ) {
	sdUIObject* child = parent->GetNode().GetChild();
	while ( child != NULL ) {
		child->OnLanguageShutdown();

		OnLanguageShutdown_r( child );
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUIObject::Script_IsChild
============
*/
void sdUIObject::Script_IsChild( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );
	sdUIObject* child = ui->GetWindow( name.c_str() );
	
	bool isChild = false;
	if( child != NULL ) {
		sdUIObject* parent = child->GetNode().GetParent();
		while( parent != NULL ) {
			if( parent == this ) {
				isChild = true;
				break;
			}
			parent = parent->GetNode().GetParent();
		}
	}
	stack.Push( isChild );
}
