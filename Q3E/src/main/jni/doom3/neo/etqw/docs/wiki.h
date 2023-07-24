// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DOCS_WIKI_H__
#define __DOCS_WIKI_H__

class sdWikiFormatter {
public:
	static const char*		EventArgToString( char argType, bool isResult = false );


	void					BuildEventInfo( const idEventDef* evt );
	void					BuildClassInfo( const idTypeInfo* type );
	void					BuildClassTree( const idTypeInfo* type );

	void					AddHeader( const char* name );
	void					AddSubHeader( const char* name );
	static void				AddEntityClassLink( const idTypeInfo* type, sdStringBuilder_Heap& output );
	static void				AddScriptEventLink( const idEventDef* evt, sdStringBuilder_Heap& output );
	static void				AddScriptClassLink( const sdProgram::sdTypeInfo* type, sdStringBuilder_Heap& output );
	static void				AddDeclTypeLink( const char* type, sdStringBuilder_Heap& output );
	static void				AddScriptFileLink( const char* fileName, sdStringBuilder_Heap& output );

	int						ListSuperClasses( const idTypeInfo* type );
	const idTypeInfo*		ListSuperClasses( const idTypeInfo* type, const idTypeInfo* root, int tabCount );
	int						ListClassEvents( const idTypeInfo* type );
	void					ListAllEvents( void );

	void					FormatCode( const char* fileName );
	void					BuildScriptClassTree( const sdProgram::sdTypeInfo* type );
	void					BuildScriptClassInfo( const sdProgram::sdTypeInfo* type );
	void					BuildScriptFileList( void );

	bool					WriteToFile( const char* fileName );
	void					CopyToClipBoard( void );

private:
	static bool				OutputDollarExpression( const char* input, int startIndex, int endIndex, sdStringBuilder_Heap& output );
	static void				ParseDollarExpressions( const char* input, sdStringBuilder_Heap& output );
	static int				SortEventsByName( const idEventDef* a, const idEventDef* b );

	bool					BreaksCodeWord( char c );
	void					FormatCodeLine( const char* line, bool& inComment );
	void					FormatCodeWord( const char* word );
	void					BuildClassTree_r( const idTypeInfo* type, int tabCount );
	void					BuildScriptClassTree_r( const sdProgram::sdTypeInfo* type, int tabCount );

	sdStringBuilder_Heap	info;
};

#endif // __DOCS_WIKI_H__
