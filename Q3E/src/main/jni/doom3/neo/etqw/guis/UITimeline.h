// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_UITIMELINE_H__
#define __GAME_GUIS_UITIMELINE_H__

#include "UserInterfaceTypes.h"
#include "UserInterfaceScript.h"

extern const char sdUITimelineEvent_Identifier[];
extern const char sdUITemplateFunctionInstance_IdentifierTimeline[];

class sdUITimeline;

class sdUITimelineManager :
	public sdUserInterfaceScope {
public:
							sdUITimelineManager( sdUserInterfaceLocal& ui, sdUserInterfaceScope& enclosingScope, sdUIScript& script );
	virtual					~sdUITimelineManager() { Clear(); }

	bool					CreateTimelines( const sdDeclGUITimelineHolder& timelineInfo, const sdDeclGUI* guiDecl );
	bool					CreateEvents( const sdDeclGUITimelineHolder& timelineInfo, const sdDeclGUI* guiDecl, idTokenCache& tokenCache );
	bool					CreateProperties( const sdDeclGUITimelineHolder& timelineInfo, const sdDeclGUI* guiDecl, const idTokenCache& tokenCache );

							// destroy all timelines
	void					Clear();


	void					ResetAllTimelines();
	void					ClearAllTimelines();
	void					PushAllTimelines();

	void					PostTimelineEvent( int offset, int index, sdUITimeline* scope );
	void					ClearTimelineEvents( sdUITimeline* scope );
	void					DeferredResetTimeline( sdUITimeline* scope ) { resetScopes.Append( scope ); }

	void					OnSnapshotHitch( int delta );

	void					Run( int time );

	sdUserInterfaceScope&	GetScope()	{ return enclosingScope; }
	sdUIScript&				GetScript() { return script; }
	virtual bool			IsReadOnly() const { return false; }
	virtual int							NumSubScopes() const { return 0; }
	virtual const char*					GetSubScopeNameForIndex( int index ) { assert( 0 ); return ""; }
	virtual sdUserInterfaceScope*		GetSubScopeForIndex( int index ) { assert( 0 ); return NULL; }

	virtual const char*					FindPropertyNameByKey( int index, sdUserInterfaceScope*& scope ) { assert( false ); return NULL; }
	virtual const char*					FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { assert( false ); return NULL; }

	virtual sdUserInterfaceLocal*		GetUI() { return &ui; }

	virtual sdProperties::sdProperty*	GetProperty( const char* name, sdProperties::ePropertyType type )	{ assert( 0 ); return NULL;  }
	virtual sdProperties::sdProperty*	GetProperty( const char* name )										{ assert( 0 ); return NULL;  }
	virtual sdUIFunctionInstance*		GetFunction( const char* name )										{ assert( 0 ); return NULL;  }
	virtual sdUIEvaluatorTypeBase*		GetEvaluator( const char* name )									{ assert( 0 ); return NULL;  }
	virtual sdProperties::sdPropertyHandler& GetProperties()												{ static sdProperties::sdPropertyHandler handler; assert( 0 ); return handler; }

	virtual void						SetPropertyExpression( int propertyKey, int propertyIndex, sdUIExpression* expression ) { assert( 0 ); }
	virtual int							IndexForProperty( sdProperties::sdProperty*	property )				{ assert( 0 ); return -1; }	
	virtual void						ClearPropertyExpression( int propertyKey, int propertyIndex )		{ assert( 0 ); }
	virtual void						RunFunction( int expressionIndex )									{ assert( 0 ); }

	virtual sdUIEventHandle				GetEvent( const sdUIEventInfo& info ) const							{ assert( 0 ); return sdUIEventHandle();  }
	virtual void						AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle )	{ assert( 0 ); }

	virtual int							AddExpression( sdUIExpression* expression )							{ assert( 0 ); return -1;  }
	virtual sdUIExpression*				GetExpression( int index )											{ assert( 0 ); return NULL;  }
	

	virtual const char*					GetName() const														{ return "sdUITimelineManager"; }
	virtual const char*					GetScopeClassName() const											{ return "sdUITimelineManager"; }

	virtual sdUserInterfaceScope*		GetSubScope( const char* name );
	

private:
	typedef sdHashMapGeneric< idStr, sdUITimeline*, sdHashCompareStrIcmp, sdHashGeneratorIHash > timelineHash_t;

	class sdUITimelineEvent :
		public sdPoolAllocator< sdUITimelineEvent, sdUITimelineEvent_Identifier, 128 > {
	public:
		int							index;
		int							time;
		sdUITimeline*				timeline;
	};

private:
	sdUserInterfaceLocal&			ui;
	sdUserInterfaceScope&			enclosingScope;
	sdUIScript&						script;
	timelineHash_t					timelines;
	idList< sdUITimelineEvent* >	timelineEvents;
	idList< sdUITimeline* >			resetScopes;
};

/*
============
sdUITimeline
============
*/
class sdUITimeline :
	public sdUIWritablePropertyHolder {
public:
	typedef enum timelineEvent_e {
		TE_NOOP,
		TE_CONSTRUCTOR,
		TE_ONTIME,
		TE_NUM_EVENTS
	};
													sdUITimeline( const char* name, sdUITimelineManager* manager );
	virtual											~sdUITimeline() {}

	virtual sdProperties::sdProperty*				GetProperty( const char* name );
	virtual sdProperties::sdProperty*				GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdUIFunctionInstance*					GetFunction( const char* name );
	virtual const char*								FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope );

	bool											RunTimeEvent( int time );
	virtual sdProperties::sdPropertyHandler&		GetProperties() { return properties; }
	
	bool											CreateEvents( const sdDeclGUITimeline& events, const sdDeclGUI* guiDecl, idTokenCache& tokenCache );
	bool											CreateProperties( const sdDeclGUITimeline& declProperties, const sdDeclGUI* guiDecl, const idTokenCache& tokenCache );

	void											Script_ResetTime( sdUIFunctionStack& stack );

													// this should be called whenever the UI is evaluating a script (onChange events, script function calls)
													// this ensures that the event is properly deferred and doesn't interfere with the execution of other events during the same frame
	void											DeferredReset();
	void											Reset();

	void											PushEvents();
	void											Clear();

	static void										InitFunctions();
	static void										ShutdownFunctions( void ) { timelineFunctions.DeleteContents(); }

	virtual const char*								GetName() const { return "sdUITimeline"; }

	virtual sdUIEventHandle							GetEvent( const sdUIEventInfo& info ) const				{ return events.GetEvent( info ); }
	virtual void									AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle ) { events.AddEvent( info, scriptHandle ); }

	virtual int										AddExpression( sdUIExpression* expression )				{ return manager->GetScope().AddExpression( expression ); }
	virtual sdUIExpression*							GetExpression( int index )								{ return manager->GetScope().GetExpression( index ); }
	virtual sdUIEvaluatorTypeBase*					GetEvaluator( const char* name )						{ return manager->GetScope().GetEvaluator( name ); }
	virtual void									RunFunction( int expressionIndex )						{ return manager->GetScope().RunFunction( expressionIndex ); }
	virtual sdUserInterfaceScope*					GetSubScope( const char* name )							{ return manager->GetScope().GetSubScope( name ); }
	virtual sdUserInterfaceLocal*					GetUI()													{ return manager->GetUI(); }

	virtual void									OnSnapshotHitch( int delta );

private:
													sdUITimeline( const sdUITimeline& rhs );
	sdUITimeline&									operator=( const sdUITimeline& rhs );

	void											OnActiveChanged( const float oldValue, const float newValue );

	static const sdUITemplateFunction< sdUITimeline >*	FindFunction( const char* name );

private:
	sdFloatProperty 									active;
	sdStringProperty 									name;

private:
	bool												needsReset;
	int													resetTime;
	sdProperties::sdPropertyHandler						properties;
	sdUITimelineManager*								manager;
	idList< int >										timeline;

	sdUIEventTable										events;
	sdUIScript											script;

	static idHashMap< sdUITemplateFunction< sdUITimeline >* >	timelineFunctions;
};

#endif // __GAME_GUIS_UITIMELINE_H__
