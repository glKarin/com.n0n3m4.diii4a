// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACESCRIPTSTATE_H__
#define __GAME_GUIS_USERINTERFACESCRIPTSTATE_H__

#include "UserInterfaceTypes.h"

class sdUIObject;

/*
============
sdUIWindowState
============
*/
class sdUIWindowState :
	public sdUserInterfaceScope {
public:
	sdUIWindowState( void );
	~sdUIWindowState( void );

	void									Init( sdUIObject* _window );	

	virtual const char*						GetScopeClassName() const;
	virtual bool							IsReadOnly() const { return false; }
	virtual int								NumSubScopes() const { return 0; }
	virtual const char*						GetSubScopeNameForIndex( int index ) { assert( 0 ); return ""; }
	virtual sdUserInterfaceScope*			GetSubScopeForIndex( int index ) { assert( 0 ); return NULL; }

	virtual sdUserInterfaceScope*			GetSubScope( const char* name );
	virtual sdProperties::sdProperty*		GetProperty( const char* name );
	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	virtual sdProperties::sdProperty*		GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdUIEvaluatorTypeBase*			GetEvaluator( const char* name );

	virtual int								IndexForProperty( sdProperties::sdProperty*	property );
	virtual void							SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression );
	virtual void							ClearPropertyExpression( int propertyKey, int propertyIndex );
	virtual void							RunFunction( int expressionIndex );

	virtual const char*						FindPropertyNameByKey( int index, sdUserInterfaceScope*& scope );
	virtual const char*						FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope );

	virtual int								AddExpression( sdUIExpression* expression );
	virtual sdUIExpression*					GetExpression( int index );

	virtual sdProperties::sdPropertyHandler& GetProperties( void ) { return properties; }

	virtual sdUIEventHandle					GetEvent( const sdUIEventInfo& info ) const;
	virtual void							AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle );

	virtual sdUserInterfaceLocal*			GetUI();

	sdUIObject*								GetWindow() { return window; }	

	virtual const char*						GetName() const;

private:
	sdProperties::sdPropertyHandler			properties;
	sdUIObject*								window;
};

#endif // __GAME_GUIS_USERINTERFACESCRIPTSTATE_H__
