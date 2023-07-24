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
#include "UserInterfaceLocal.h"
#include "UIObjectScriptState.h"
#include "UIObject.h"

using namespace sdProperties;


/*
===============================================================================

	sdUIWindowState

===============================================================================
*/

/*
================
sdUIWindowState::sdUIWindowState
================
*/
sdUIWindowState::sdUIWindowState( void ) : window( NULL ) {
}

/*
================
sdUIWindowState::~sdUIWindowState
================
*/
sdUIWindowState::~sdUIWindowState( void ) {
}

/*
================
sdUIWindowState::Init
================
*/
void sdUIWindowState::Init( sdUIObject* _window ) {
	window = _window;
}

/*
============
sdUIWindowState::GetUI
============
*/
sdUserInterfaceLocal* sdUIWindowState::GetUI() {
	return window->GetUI();
}

/*
============
sdUIWindow::GetName
============
*/
const char*	sdUIWindowState::GetName() const {
	return window->GetName();
}


/*
============
sdUIWindowState::GetScopeClassName
============
*/
const char*	sdUIWindowState::GetScopeClassName() const {
	return window->GetScopeClassName();
}

/*
================
sdUIWindowState::GetSubScope
================
*/
sdUserInterfaceScope* sdUIWindowState::GetSubScope( const char* name ) {
	if( !idStr::Icmp( name, "gui" ) ) {
		return &window->GetUI()->GetState();
	}

	if( !idStr::Icmp( name, "timeline" ) ) {
		return window->GetTimelineManager();
	}

	return NULL;
}

/*
================
sdUIWindowState::GetProperty
================
*/
sdProperties::sdProperty* sdUIWindowState::GetProperty( const char* name ) {
	return properties.GetProperty( name, PT_INVALID, false );
}

/*
============
sdUIWindowState::GetProperty
============
*/
sdProperties::sdProperty* sdUIWindowState::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, type, false );
	return prop;
}

/*
================
sdUIWindowState::GetFunction
================
*/
sdUIFunctionInstance* sdUIWindowState::GetFunction( const char* name ) {
	return window->GetFunction( name );
}


/*
================
sdUIWindowState::GetEvaluator
================
*/
sdUIEvaluatorTypeBase* sdUIWindowState::GetEvaluator( const char* name ) {
	return GetUI()->GetState().GetEvaluator( name );
}

/*
================
sdUIWindowState::IndexForProperty
================
*/
int sdUIWindowState::IndexForProperty( sdProperties::sdProperty* property ) {
	return window->GetUI()->GetState().IndexForProperty( property );
}

/*
================
sdUIWindowState::FindPropertyNameByKey
================
*/
const char* sdUIWindowState::FindPropertyNameByKey( int index, sdUserInterfaceScope*& scope ) {
	return window->GetUI()->GetState().FindPropertyNameByKey( index, scope );
}

/*
================
sdUIWindowState::FindPropertyName
================
*/
const char* sdUIWindowState::FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) {
	scope = this;
	return properties.NameForProperty( property );
}

/*
================
sdUIWindowState::SetPropertyExpression
================
*/
void sdUIWindowState::SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression ) {
	window->GetUI()->GetState().SetPropertyExpression( propertyKey, propertyIndex, expression );
}

/*
================
sdUIWindowState::SetPropertyExpression
================
*/
void sdUIWindowState::ClearPropertyExpression( int propertyKey, int propertyIndex ) {
	window->GetUI()->GetState().ClearPropertyExpression( propertyKey, propertyIndex );
}

/*
================
sdUIWindowState::RunFunction
================
*/
void sdUIWindowState::RunFunction( int expressionIndex ) {
	window->GetUI()->GetState().RunFunction( expressionIndex );
}

/*
================
sdUIWindowState::GetEvent
================
*/
sdUIEventHandle sdUIWindowState::GetEvent( const sdUIEventInfo& info ) const {
	return window->GetEvent( info );
}

/*
================
sdUIWindowState::AddEvent
================
*/
void sdUIWindowState::AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) {
	window->AddEvent( info, scriptHandle );
}

/*
================
sdUIWindowState::AddExpression
================
*/
int sdUIWindowState::AddExpression( sdUIExpression* expression ) {
	return window->GetUI()->GetState().AddExpression( expression );
}

/*
============
sdUIWindowState::GetExpression
============
*/
sdUIExpression*	sdUIWindowState::GetExpression( int index ) {
	return window->GetUI()->GetState().GetExpression( index );
}
