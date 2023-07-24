// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACEOBJECT_H__
#define __GAME_GUIS_USERINTERFACEOBJECT_H__

#include "UserInterfaceTypes.h"
#include "UITimeline.h"
#include "UIObjectScriptState.h"

class sdUIObject;


class sdUITypeInfo;

template< class T >
struct sdClass_StaticClassInitialization {
	sdClass_StaticClassInitialization() {
		T::InitClass();
	}
	~sdClass_StaticClassInitialization() {
		T::ShutdownClass();
	}
};

#define SD_UI_DECLARE_CLASS_STATIC_INIT( ClassName )																							\
	private:																																\
	static sdClass_StaticClassInitialization< ClassName > sdClass_ClassInit;

#define SD_UI_DECLARE_TYPE_INFO																												\
	static sdUITypeInfo Type;																										\
	virtual bool IsType( const sdUITypeInfo& rhs ) const	{ return Type.IsType( rhs ); }												\
	virtual const sdUITypeInfo& GetType() const			{ return Type; }															\

#define SD_UI_IMPLEMENT_CLASS_STATIC_INIT( ClassName )																							\
	sdClass_StaticClassInitialization< ClassName > ClassName ::sdClass_ClassInit;


#define SD_UI_DECLARE_CLASS( ClassName )																										\
public:																																		\
	SD_UI_DECLARE_TYPE_INFO

#define SD_UI_DECLARE_ABSTRACT_CLASS( ClassName )																								\
public:																																		\
	SD_UI_DECLARE_TYPE_INFO

#define SD_UI_IMPLEMENT_TYPE_INFO( ClassName, SuperClassName )																					\
	sdUITypeInfo ClassName::Type( #ClassName, #SuperClassName );

#define SD_UI_IMPLEMENT_CLASS( ClassName, SuperClassName )																						\
	SD_UI_IMPLEMENT_TYPE_INFO( ClassName, SuperClassName )

#define SD_UI_IMPLEMENT_ABSTRACT_CLASS( ClassName, SuperClassName )																			\
	SD_UI_IMPLEMENT_TYPE_INFO( ClassName, SuperClassName ) 

/*
============
sdUITypeInfo
============
*/
class sdUITypeInfo {
public:
	typedef idHierarchy< sdUITypeInfo > node_t;
	typedef sdUITypeInfo* Ptr;
	typedef idList< sdUITypeInfo::Ptr > typeList_t;

	sdUITypeInfo( const char* name_, const char* superName_ ) : typeBegin( 0 ), typeEnd( 0 ), name( name_ ), superName( superName_ ) {
		Ptr existing = FindType( name, true );
		if( existing ) {
			throw idException( va( "Type '%s' is already defined", name ));
		}
		GetTypeList().Append( this );
		node.SetOwner( this );
	}
	~sdUITypeInfo() {}

	bool			operator==( const char* rhs ) const {
		return idStr::Cmp( rhs, name ) == 0;
	}

	bool			IsType( const sdUITypeInfo& rhs ) const {
		return typeBegin >= rhs.typeBegin && typeEnd <= rhs.typeEnd;
	}

	const char*		GetName() const { return name; }

	node_t&			GetNode() { return node; }

	static void 	Init();
	static void 	Shutdown();

	static void 	ListClasses();
	static int		NumTypes() { return GetTypeList().Num(); }

private:
	const char* name;
	const char*	superName;
	size_t		typeBegin;
	size_t		typeEnd;

	node_t		node;

	static bool	initialized;

private:
	static void 		ListClasses( node_t& root );
	static void 		NumberTypes( node_t& root, size_t& id );
	static Ptr			FindType( const char* typeName, bool allowNotFound = false );
	static typeList_t&	GetTypeList();
	static Ptr			GetTypeRoot( Ptr root = NULL );		
};

extern const char sdUITemplateFunctionInstance_IdentifierWindow[];
extern const char sdUITemplateFunctionInstance_IdentifierObject[];


/*
============
sdUIObject
============
*/
SD_UI_PROPERTY_TAG(
	alias = "object";
)
class sdUIObject {
public:
	SD_UI_DECLARE_ABSTRACT_CLASS( sdUIObject )
	typedef idHierarchy< sdUIObject > hierarchyNode_t;

	typedef enum windowEvent_e {
		OE_NOOP,
		OE_CONSTRUCTOR,
		OE_CREATE,
		OE_NAMED,
		OE_PROPCHANGED,
		OE_CVARCHANGED,
		OE_NUM_EVENTS,
	} windowEvent_t;

	template< class T >
	T*				Cast() {
						if( this == NULL ) {
							return NULL;
						}
						if( !GetType().IsType( T::Type )) {
							return NULL;
						}
						return static_cast< T* >( this );
					}

	template< class T >
	const T*		Cast() const {
						if( this == NULL ) {
							return NULL;
						}
						if( !GetType().IsType( T::Type )) {
							return NULL;
						}
						return static_cast< const T* >( this );
					}

	enum eZOrder { ZO_FRONT, ZO_BACK };

	enum objectFlag_t {
		OF_FIXED_LAYOUT			= BITT< 0 >::VALUE,		// Object should be immune to any potential layout changes
		// Make sure to update NEXT_OBJECT_FLAG below!
	};
	static const int NEXT_OBJECT_FLAG = 1;	// derived classes should use bit fields from this bit up to avoid conflicts

											sdUIObject();
	virtual									~sdUIObject();

	virtual const char*						GetName( void ) const { return name.GetValue().c_str(); }

	virtual void							OnCreate() {}
	virtual void							LayoutChanged() {}
	virtual bool							PostEvent( const sdSysEvent* event ) { return false; }
	virtual bool							HandleFocus( const sdSysEvent* event ) { return false; }
	virtual void							OnActivate() {}
	virtual void							OnGainFocus() {}
	virtual void							OnLoseFocus() {}

	virtual void							MakeLayoutDirty() {}

											// doesn't invalidate the layout of the parent parameter
	static void								MakeLayoutDirty_r( sdUIObject* parent );

	virtual void							OnLanguageInit() {}
	virtual void							OnLanguageShutdown() {}

	static void								OnLanguageInit_r( sdUIObject* parent );
	static void								OnLanguageShutdown_r( sdUIObject* parent );

											// this should be called by derived versions AFTER they have done any necessary work
	virtual void							ApplyLayout();

	virtual sdUIObject*						UpdateToolTip( const idVec2& cursor ) { return NULL; }

	bool									TestFlag( const int flag ) const { return ( idMath::Ftoi( flags ) & flag ) != 0; }


	static sdProperties::ePropertyType		PropertyTypeForName( const char* typeName );
	static void								SetupProperties( sdProperties::sdPropertyHandler& handler, const idList< sdDeclGUIProperty* >& properties, sdUserInterfaceLocal* ui, const idTokenCache& tokenCache );
	virtual void							CreateProperties( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, const idTokenCache& tokenCache );
	void									InitEvents();
	virtual void							CreateEvents( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, idTokenCache& tokenCache );
	virtual void							CreateTimelines( sdUserInterfaceLocal* _ui, const sdDeclGUIWindow* windowParms, idTokenCache& tokenCache );
	virtual void							CacheEvents() {}

	virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );

	virtual const char*						GetScopeClassName() const = 0;
	virtual sdUserInterfaceScope&			GetScope( void ) { return scriptState; }

	void									Script_SetParent( sdUIFunctionStack& stack );
	void									Script_IsChild( sdUIFunctionStack& stack );
	void									Script_ChangeZOrder( sdUIFunctionStack& stack );
	void									Script_PostNamedEvent( sdUIFunctionStack& stack );
	void									Script_PostOptionalNamedEvent( sdUIFunctionStack& stack );	

	sdUserInterfaceLocal*					GetUI( void ) { return ui; }
	const sdUserInterfaceLocal*				GetUI( void ) const { return ui; }

	sdUITimelineManager*					GetTimelineManager() { return timelines.Get(); }

	virtual sdUIFunctionInstance*			GetFunction( const char* name );
	virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	bool									RunEventHandle( const sdUIEventHandle handle );
	bool									RunEvent( const sdUIEventInfo& info );

	// Events
	int										GetNumEvents( const sdUIEventHandle type ) { return events.Num( type ); }
	virtual sdUIEventHandle					GetEvent( const sdUIEventInfo& info ) const;
	virtual void							AddEvent( const sdUIEventInfo& info, sdUIEventHandle scriptHandle );

	virtual int								GetFirstEventType() const { return 2; }
	virtual int								GetNextEventType( int event ) const { return ++event; }
	virtual int								GetMaxEventTypes( void ) const { return OE_NUM_EVENTS; }
	virtual const char*						GetEventNameForType( int event ) const { return eventNames[ event ]; }

	hierarchyNode_t&						GetNode( void )			{ return hierarchyNode; }
	const hierarchyNode_t&					GetNode( void ) const	{ return hierarchyNode; }

	void									SetParent( sdUIObject* parent );

	int										NamedEventHandleForString( const char* name );

	bool									PostNamedEvent( const char* event, bool optional );

	virtual void							EndLevelLoad() {}

	static const sdUITemplateFunction< sdUIObject >*	FindFunction( const char* name );
	static void											InitFunctions( void );
	static void											ShutdownFunctions( void ) { objectFunctions.DeleteContents(); }


	SD_UI_PROPERTY_TAG(
	title				= "1. Common/Name";
	desc				= "Name of the window. Must be unique within a GUI.";
	editor				= "edit";
	option1				= "{editorSpecial} {name}";
	datatype			= "string";
	)
	sdStringProperty	name;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. Behavior/Flags";
	desc				= "A bitfield of behaviors.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		flags;
	// ===========================================

protected:
	typedef sdPair< sdProperties::sdProperty*, sdProperties::CallbackHandle > callbackHandler_t;	
	void									ConnectGlobalCallback( const callbackHandler_t& cb ) {
												toDisconnect.Append( cb );
											}

	virtual void							InitProperties() {}	
	void									SetWindowFlag( const int flag ) { flags = idMath::Ftoi( flags ) | flag; }
	void									ClearWindowFlag( const int flag ) { flags = idMath::Ftoi( flags ) & ~flag; }
	bool									FlagActivated( const float oldValue, const float newValue, const int flag ) const	{ return	( !( idMath::Ftoi( oldValue ) & flag ) && ( idMath::Ftoi( newValue ) & flag ) ); }
	bool									FlagDeactivated( const float oldValue, const float newValue, const int flag ) const	{ return	( ( idMath::Ftoi( oldValue ) & flag ) && !( idMath::Ftoi( newValue ) & flag ) ); }
	bool									FlagChanged( const float oldValue, const float newValue, const int flag ) const		{ return	FlagActivated( oldValue, newValue, flag ) || FlagDeactivated( oldValue, newValue, flag ); }

public:
											// this should be called explicitly in all derived destructors
											void									DisconnectGlobalCallbacks() {
												for( int i = 0; i < toDisconnect.Num(); i++ ) {
													toDisconnect[ i ].first->RemoveOnChangeHandler( toDisconnect[ i ].second );
												}
												toDisconnect.Clear();
												cvarCallbacks.DeleteContents( true );
											}
private:
	void									OnStringPropertyChanged( int event, const idStr& oldValue, const idStr& newValue );
	void									OnWStringPropertyChanged( int event, const idWStr& oldValue, const idWStr& newValue );
	void									OnIntPropertyChanged( int event, const int oldValue, const int newValue );
	void									OnFloatPropertyChanged( int event, const float oldValue, const float newValue );
	void									OnVec4PropertyChanged( int event, const idVec4& oldValue, const idVec4& newValue );
	void									OnVec3PropertyChanged( int event, const idVec3& oldValue, const idVec3& newValue );
	void									OnVec2PropertyChanged( int event, const idVec2& oldValue, const idVec2& newValue );
	void									OnCVarChanged( idCVar& cvar, int id );	

protected:
	sdUserInterfaceLocal*					ui;

	sdUIWindowState							scriptState;

	sdUIEventTable							events;

	hierarchyNode_t							hierarchyNode;

	int										lastClickTime;

	sdAutoPtr< sdUITimelineManager >		timelines;

	idStrList								namedEvents;

private:
	typedef sdUICVarCallback< sdUIObject >	cvarCallback_t;
	friend class sdUICVarCallback< sdUIObject >;

	idList< cvarCallback_t* >				cvarCallbacks;

	idList< callbackHandler_t >				toDisconnect;

	static idHashMap< sdUITemplateFunction< sdUIObject >* >	objectFunctions;
	static const char* eventNames[ OE_NUM_EVENTS ];
};

/*
============
sdUIObject_Drawable
============
*/
class sdUIObject_Drawable :
	public sdUIObject {
public:
	SD_UI_DECLARE_ABSTRACT_CLASS( sdUIObject_Drawable )

	virtual					~sdUIObject_Drawable() {}

	virtual void			Draw() = 0;
	virtual void			FinalDraw() {};
};

#endif // __GAME_GUIS_USERINTERFACEOBJECT_H__
