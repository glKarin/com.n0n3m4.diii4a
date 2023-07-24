// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DECLGUI_H__
#define __DECLGUI_H__

const char* const GUI_DEFAULT_TIMELINE_NAME = "default";

extern const char sdDeclGUIProperty_Identifier[];
extern const char sdDeclGUITimeEvent_Identifier[];
extern const char sdDeclGUITimeline_Identifier[];
extern const char sdDeclGUIEvent_Identifier[];
extern const char sdDeclGUIWindow_Identifier[];


/*
============
sdDeclGUITimeEvent
============
*/
class sdDeclGUITimeEvent : public sdPoolAllocator< sdDeclGUITimeEvent, sdDeclGUITimeEvent_Identifier, 128 > {
public:
											sdDeclGUITimeEvent( void );
											~sdDeclGUITimeEvent( void );

	bool									Parse( idLexer& src, const idToken& token );

	int										GetStartTime( void ) const			{ return startTime; }
	const char*								GetTextInfo( void ) const			{ return va( "onTime %i", startTime ); }
	const idList<unsigned short>&			GetTokenIndices( void ) const		{ return tokens; }

private:
	int										startTime;
	idList<unsigned short>					tokens;
};


/*
============
sdDeclGUITimeline
============
*/
class sdDeclGUIProperty;
class sdDeclGUITimeline : public sdPoolAllocator< sdDeclGUITimeline, sdDeclGUITimeline_Identifier, 128 > {
public:
	sdDeclGUITimeline() : events( 1 ), properties( 1 ) {}
	~sdDeclGUITimeline() {
		events.DeleteContents( true );
		properties.DeleteContents( true );
	}

	idList< sdDeclGUITimeEvent* >&			GetEvents() 			{ return events; }
	const idList< sdDeclGUITimeEvent* >&	GetEvents()	const		{ return events; }
	idList< sdDeclGUIProperty* >&			GetProperties()			{ return properties; }
	const idList< sdDeclGUIProperty* >&		GetProperties()	const	{ return properties; }

private:
	idList< sdDeclGUITimeEvent* >			events;
	idList< sdDeclGUIProperty* >			properties;
};

typedef sdHashMapGeneric< idStr, sdDeclGUITimeline*, sdHashCompareStrIcmp, sdHashGeneratorIHash > declTimelineHash_t;

/*
============
sdDeclGUITimelineHolder
============
*/
class sdDeclGUITimelineHolder {
public:
	sdDeclGUITimelineHolder () : timelines( 1 ) {
	}

	~sdDeclGUITimelineHolder() {
		timelines.DeleteValues();
		timelines.Clear();
	}
	int										GetNumTimelines() const { return timelines.Num(); }
	const char*								GetTimelineName( int index ) const;
	const sdDeclGUITimeline*				GetTimeline( int index ) const;

	declTimelineHash_t&						GetTimelines() { return timelines; }
	const declTimelineHash_t&				GetTimelines() const { return timelines; }

	bool									ParseTimeline( idLexer& src );

	void									Clear() { timelines.Clear(); }

protected:
	declTimelineHash_t						timelines;
};


/*
============
sdDeclGUIEvent
============
*/
class sdDeclGUIEvent : public sdPoolAllocator< sdDeclGUIEvent, sdDeclGUIEvent_Identifier, 512 > {
public:
											sdDeclGUIEvent( void );
											~sdDeclGUIEvent( void );

	bool									Parse( idLexer& src, const idToken& token );

	unsigned short							GetName( void ) const 			{ return name; }
	const idList<unsigned short>&			GetFlags( void ) const			{ return flags; }
	const idList<unsigned short>&			GetTokenIndices( void ) const	{ return tokens; }

private:
	idList<unsigned short>					tokens;
	idList<unsigned short>					flags;
	unsigned short							name;

};

/*
============
sdDeclGUIProperty
============
*/
class sdDeclGUIProperty : public sdPoolAllocator< sdDeclGUIProperty, sdDeclGUIProperty_Identifier, 1024 > {
public:
											sdDeclGUIProperty( void );
											~sdDeclGUIProperty( void );

	bool									Parse( idLexer& src, const idToken& token );

	unsigned short							GetType( void ) const { return type; }
	unsigned short							GetName( void ) const { return name; }
	const idList<unsigned short>&				GetValue( void ) const { return value; }

private:
	unsigned short							type;
	unsigned short							name;
	idList<unsigned short>					value;
};

/*
============
sdDeclGUIWindow
============
*/
class sdDeclGUI;
class sdDeclGUIWindow : public sdPoolAllocator< sdDeclGUIWindow, sdDeclGUIWindow_Identifier, 128 > {
public:
											sdDeclGUIWindow( void );
											~sdDeclGUIWindow( void );

	bool									Parse( idLexer& src, sdDeclGUI* guiDecl );

	const char*								GetTypeName( void ) const { return type->c_str(); }
	const char*								GetName( void ) const { return name.c_str(); }

	const idList< sdDeclGUIProperty* >&		GetProperties( void ) const { return properties; }
	const idList< sdDeclGUIEvent* >&		GetEvents( void ) const { return events; }
	const idStrList&						GetChildren( void ) const { return children; }

	static bool								ParseProperties( idList< sdDeclGUIProperty* >& properties, idLexer& src );
	static bool								ParseEvents( idList< sdDeclGUIEvent* >& events, idLexer& src );	

#ifdef _DEBUG
	bool									GetBreakOnDraw() const { return breakOnDraw; }
	bool									GetBreakOnLayout() const { return breakOnLayout; }
#endif

	const sdDeclGUITimelineHolder&			GetTimelines() const { return timelines; }

private:
	static idStrPool						s_strPool;
	idList< sdDeclGUIProperty* >			properties;
	idList< sdDeclGUIEvent* >				events;
	idStrList								children;
	idStr									name;
	const idPoolStr*						type;

#ifdef _DEBUG
	bool									breakOnDraw;
	bool									breakOnLayout;
#endif
	sdDeclGUITimelineHolder					timelines;
};


/*
============
sdDeclGUI
============
*/
class sdDeclGUI :
	public idDecl,
	public sdDeclGUITimelineHolder {
public:	
	static const int LEXER_FLAGS			= LEXFL_NOSTRINGCONCAT	| LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOEMITSTRINGESCAPECHARS;
											sdDeclGUI( void );
	virtual									~sdDeclGUI( void );

	static void								CacheMaterialDictionary( const char* name, const idDict& materials );

	virtual const char*						DefaultDefinition( void ) const;
	virtual bool							Parse( const char *text, const int textLength );
	virtual void							FreeData( void );

	sdDeclGUIWindow*						ParseWindow( idLexer& lexer );

	int										GetNumWindows( void ) const { return windows.Num(); }
	const sdDeclGUIWindow*					GetWindow( int index ) const { return windows[ index ]; }

	const idList< sdDeclGUIProperty* >&		GetProperties( void ) const { return properties; }
	const idList< sdDeclGUIEvent* >&		GetEvents( void ) const { return events; }

	const idDict&							GetMaterials() const { return materials;}
	const idDict&							GetSounds() const { return sounds;}
	const idDict&							GetColors() const { return colors;}

	static void								CacheFromDict( const idDict& dict );

	static bool								CreateConstructor( const idList<sdDeclGUIProperty*>& properties, idList<unsigned short>& tokens, idTokenCache& tokenCache );

	const sdDeclGUITimelineHolder&			GetTimelines() const { return timelines; }

	const idStrList&						GetPartialLoadModels() const { return partialLoadModels; }

#ifdef _DEBUG
	bool									GetBreakOnDraw() const { return breakOnDraw; }
#endif

	static void								InitDefines() { defines.Resize( 256 ); }
	static void								AddDefine( const char* define ) { defines.Append( define ); }
	static void								ClearDefines( void ) { defines.Clear(); }

	static bool								ParseBracedSection( idLexer& src, idList< unsigned short >& tokens, const char* open = "{", const char* close = "}" );

private:
	idList< sdDeclGUIWindow* >				windows;
	idList< sdDeclGUIProperty* >			properties;
	idList< sdDeclGUIEvent* >				events;
	
	sdDeclGUITimelineHolder					timelines;
	
	idDict									materials;
	idDict									sounds;
	idDict									colors;
	idStrList								partialLoadModels;

	static idStrList						defines;

#ifdef _DEBUG
	bool									breakOnDraw;
#endif
};

#endif // __DECLGUI_H__
