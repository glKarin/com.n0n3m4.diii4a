// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "wiki.h"
#include "../script/Script_Program.h"

/*
==================
sdWikiFormatter::EventArgToString
==================
*/
const char* sdWikiFormatter::EventArgToString( char argType, bool isResult ) {
	switch ( argType ) {
		case D_EVENT_INTEGER:
			return "[[ScriptType:integer|integer]]";
		case D_EVENT_BOOLEAN:
			return "[[ScriptType:boolean|boolean]]";
		case D_EVENT_HANDLE:
			return "[[ScriptType:handle|handle]]";
		case D_EVENT_FLOAT:
			return "[[ScriptType:float|float]]";
		case D_EVENT_VECTOR:
			return "[[ScriptType:vector|vector]]";
		case D_EVENT_STRING:
			return "[[ScriptType:string|string]]";
		case D_EVENT_WSTRING:
			return "[[ScriptType:wide string|wide string]]";
		case D_EVENT_ENTITY:
			if ( !isResult ) {
				return "[[ScriptType:entity_nonull|entity (cannot be null)]]";
			}
		case D_EVENT_ENTITY_NULL:
			return "[[ScriptType:entity|entity]]";
		case D_EVENT_OBJECT:
			return "[[ScriptType:object|object]]";
	}

	return NULL;
}

/*
==================
sdWikiFormatter::AddScriptFileLink
==================
*/
void sdWikiFormatter::AddScriptFileLink( const char* fileName, sdStringBuilder_Heap& output ) {
	output += va( "[[Script:Files:%s|%s]]", fileName, fileName );
}

/*
==================
sdWikiFormatter::AddDeclTypeLink
==================
*/
void sdWikiFormatter::AddDeclTypeLink( const char* type, sdStringBuilder_Heap& output ) {
	output += va( "[[DeclType:%s|%s]]", type, type );
}

/*
==================
sdWikiFormatter::AddScriptClassLink
==================
*/
void sdWikiFormatter::AddScriptClassLink( const sdProgram::sdTypeInfo* type, sdStringBuilder_Heap& output ) {
	output += va( "[[ScriptClass:%s|%s]]", type->GetName(), type->GetName() );
}

/*
==================
sdWikiFormatter::AddScriptEventLink
==================
*/
void sdWikiFormatter::AddScriptEventLink( const idEventDef* evt, sdStringBuilder_Heap& output ) {
	output += va( "[[ScriptEvent:%s|%s]]", evt->GetName(), evt->GetName() );
}

/*
==================
sdWikiFormatter::AddEntityClassLink
==================
*/
void sdWikiFormatter::AddEntityClassLink( const idTypeInfo* type, sdStringBuilder_Heap& output ) {
	output += va( "[[EntityClass:%s|%s]]", type->classname, type->classname );
}

/*
==================
sdWikiFormatter::AddHeader
==================
*/
void sdWikiFormatter::AddHeader( const char* name ) {
	info += va( "== %s ==\r\n", name );
}

/*
==================
sdWikiFormatter::AddSubHeader
==================
*/
void sdWikiFormatter::AddSubHeader( const char* name ) {
	info += va( "=== %s ===\r\n", name );
}

/*
==================
sdWikiFormatter::OutputDollarExpression
==================
*/
bool sdWikiFormatter::OutputDollarExpression( const char* input, int startIndex, int endIndex, sdStringBuilder_Heap& output ) {
	if ( endIndex == ( startIndex + 1 ) ) {
		output += '$';
		return true;
	}

	int len = ( endIndex - startIndex ) - 1;
	char* buffer = ( char* )_alloca( len + 1 );
	memcpy( buffer, &input[ startIndex + 1 ], len );
	buffer[ len ] = '\0';

	idStrList list;
	idSplitStringIntoList( list, buffer, ":" );

	idStr& command = list[ 0 ];

	if ( command.Icmp( "decl" ) == 0 ) {
		if ( list.Num() == 1 ) {
			output += "[[DeclType:Overview|decl]]";
			return true;
		}

		if ( list.Num() != 2 ) {
			return false;
		}

		AddDeclTypeLink( list[ 1 ].c_str(), output );
	} else if ( command.Icmp( "event" ) == 0 ) {
		if ( list.Num() != 2 ) {
			return false;
		}

		const idEventDef* def = idEventDef::FindEvent( list[ 1 ].c_str() );
		if ( def == NULL ) {
			return false;
		}

		AddScriptEventLink( def, output );
	} else if ( command.Icmp( "null" ) == 0 ) {
		if ( list.Num() != 1 ) {
			return false;
		}

		output += "[[ScriptConstant:null|null]]";
	} else if ( command.Icmp( "class" ) == 0 ) {
		if ( list.Num() == 1 ) {
			output += "[[EntityClass:Overview|class]]";
			return true;
		}

		if ( list.Num() != 2 ) {
			return false;
		}

		const idTypeInfo* type = idClass::GetClass( list[ 1 ].c_str() );
		if ( type == NULL ) {
			return false;
		}

		AddEntityClassLink( type, output );
	} else {
		return false;
	}

	return true;
}

/*
==================
sdWikiFormatter::ParseDollarExpressions
==================
*/
void sdWikiFormatter::ParseDollarExpressions( const char* input, sdStringBuilder_Heap& output ) {
	int len = idStr::Length( input );
	for ( int i = 0; i < len; ) {
		if ( input[ i ] != '$' ) {
			output += input[ i ];
			i++;
			continue;
		}

		int j;
		for ( j = i + 1; j < len; j++ ) {
			if ( input[ j ] == '$' ) {
				if ( !OutputDollarExpression( input, i, j, output ) ) {
					j = len;
				}
				break;
			}
		}
		if ( j == len ) {
			gameLocal.Error( "sdWikiFormatter::ParseDollarExpressions Failed to parse '%s'", input ); 
		} else {
			i = j + 1;
		}
	}
}

/*
==================
sdWikiFormatter::BuildEventInfo
==================
*/
void sdWikiFormatter::BuildEventInfo( const idEventDef* evt ) {
	AddHeader( "Parameters" );

	int numArgs = evt->GetNumArgs();
	if ( numArgs == 0 ) {
		info += "None.\r\n";
	} else {
		for ( int i = 0; i < numArgs; i++ ) {
			sdStringBuilder_Heap descriptionTemp;
			ParseDollarExpressions( evt->GetArgDescription( i ), descriptionTemp );
			info += va( " %s %s -- %s\r\n", EventArgToString( evt->GetArgFormat()[ i ] ), evt->GetArgName( i ), descriptionTemp.c_str() );
		}
	}

	AddHeader( "Result" );
	const char* resultTypeName = EventArgToString( evt->GetReturnType(), true );
	if ( resultTypeName == NULL ) {
		info += "None.\r\n";
	} else {
		info += va( "%s\r\n", resultTypeName );
	}

	AddHeader( "Supported By" );

	int numSupported = 0;
	for ( int i = 0; i < idClass::GetNumTypes(); i++ ) {
		idTypeInfo* type = idClass::GetType( i );
		if ( !type->RespondsTo( *evt ) ) {
			continue;
		}

		bool parentRepspondsTo = type->super == NULL ? false : type->super->RespondsTo( *evt );
		if ( parentRepspondsTo ) {
			continue;
		}

		info += " ";
		AddEntityClassLink( type, info );
		info += "\r\n";
		numSupported++;
	}
	if ( numSupported == 0 ) {
		info += "Nothing.\r\n";
	}

	const char* comments = evt->GetComments();
	if ( comments != NULL ) {
		idStrList list;
		idSplitStringIntoList( list, comments, "\n" );

		AddHeader( "Comments" );

		for ( int i = 0; i < list.Num(); i++ ) {
			sdStringBuilder_Heap temp;
			ParseDollarExpressions( list[ i ].c_str(), temp );
			info += va( "*%s\r\n", temp.c_str() );
		}
	}

	AddHeader( "Functionality" );

	sdStringBuilder_Heap descriptionTemp;
	ParseDollarExpressions( evt->GetDescription(), descriptionTemp );
	info += descriptionTemp.c_str();
	info += "\r\n";
}

/*
==================
sdWikiFormatter::ListSuperClasses
==================
*/
int sdWikiFormatter::ListSuperClasses( const idTypeInfo* type ) {
	const idTypeInfo* super = NULL;
	int count = 0;
	while ( super != type->super ) {
		super = ListSuperClasses( type, super, ++count );
	}
	return count;
}

/*
==================
sdWikiFormatter::ListSuperClasses
==================
*/
const idTypeInfo* sdWikiFormatter::ListSuperClasses( const idTypeInfo* type, const idTypeInfo* root, int tabCount ) {
	const idTypeInfo* super = type->super;
	assert( super != NULL );
	while ( super->super != root ) {
		super = super->super;
	}

	for ( int i = 0; i < tabCount; i++ ) {
		info += '*';
	}

	AddEntityClassLink( super, info );
	info += "\r\n";

	return super;
}

/*
==================
sdWikiFormatter::ListClassEvents
==================
*/
int sdWikiFormatter::ListClassEvents( const idTypeInfo* type ) {
	int count = 0;
	for ( int i = 0; i < idEventDef::NumEventCommands(); i++ ) {
		const idEventDef* evt = idEventDef::GetEventCommand( i );

		if ( !evt->GetAllowFromScript() ) {
			continue;
		}

		if ( !type->RespondsTo( *evt ) ) {
			continue;
		}

		bool parentRepspondsTo = type->super == NULL ? false : type->super->RespondsTo( *evt );
		if ( parentRepspondsTo ) {
			continue;
		}

		info += " ";
		AddScriptEventLink( evt, info );
		info += "\r\n";

		count++;
	}

	return count;
}

/*
==================
sdWikiFormatter::SortEventsByName
==================
*/
int sdWikiFormatter::SortEventsByName( const idEventDef* a, const idEventDef* b ) {
	return idStr::Cmp( a->GetName(), b->GetName() );
}

/*
==================
sdWikiFormatter::ListAllEvents
==================
*/
void sdWikiFormatter::ListAllEvents( void ) {
	AddHeader( "All Events" );

	idList< const idEventDef* > events;

	for ( int i = 0; i < idEventDef::NumEventCommands(); i++ ) {
		const idEventDef* evt = idEventDef::GetEventCommand( i );
		if ( !evt->GetAllowFromScript() ) {
			continue;
		}

		events.Append( evt );
	}

	events.Sort( SortEventsByName );

	char lastLetter = '\0';

	for ( int i = 0; i < events.Num(); i++ ) {
		const idEventDef* evt = events[ i ];

		if ( lastLetter != evt->GetName()[ 0 ] ) {
			lastLetter = evt->GetName()[ 0 ];

			int left = 0;
			for ( int j = i; j < events.Num(); j++ ) {
				if ( lastLetter != events[ j ]->GetName()[ 0 ] ) {
					break;
				}

				if ( events[ j ]->GetDescription() == NULL ) {
					left++;
				}
			}
			if ( left == 0 ) {
				AddSubHeader( va( "%c", lastLetter ) );
			} else {
				AddSubHeader( va( "%c --%d left--", lastLetter, left ) );
			}
		}

		int numSupported = 0;
		for ( int i = 0; i < idClass::GetNumTypes(); i++ ) {
			idTypeInfo* type = idClass::GetType( i );
			if ( !type->RespondsTo( *evt ) ) {
				continue;
			}
			numSupported++;
		}

		info += " ";
		AddScriptEventLink( evt, info );
		if ( evt->GetDescription() == NULL ) {
			info += "*";
		}
		if ( numSupported == 0 ) {
			info += " '''DEAD'''";
		}
		info += "\r\n";
	}
}

/*
==================
sdWikiFormatter::BuildClassInfo
==================
*/
void sdWikiFormatter::BuildClassInfo( const idTypeInfo* type ) {
	AddHeader( "Class Tree" );
	int count = ListSuperClasses( type );
	for ( int i = 0; i <= count; i++ ) {
		info += '*';
	}
	info += va( "'''%s'''\r\n", type->classname );

	int numChildClasses = 0;

	AddHeader( "Child Classes" );
	for ( int i = 0; i < idClass::GetNumTypes(); i++ ) {
		idTypeInfo* other = idClass::GetType( i );
		if ( other->super != type ) {
			continue;
		}

		info += " ";
		AddEntityClassLink( other, info );
		info += "\r\n";

		numChildClasses++;
	}
	if ( numChildClasses == 0 ) {
		info += "None.\r\n";
	}

	AddHeader( "Description" );
	info += "UNKNOWN\r\n";

	AddHeader( "Supported Events" );
	if ( ListClassEvents( type ) == 0 ) {
		info += "None.\r\n";
	}
}

/*
==================
sdWikiFormatter::BuildClassTree
==================
*/
void sdWikiFormatter::BuildClassTree( const idTypeInfo* type ) {
	BuildClassTree_r( type, 0 );
}

/*
==================
sdWikiFormatter::WriteToFile
==================
*/
bool sdWikiFormatter::WriteToFile( const char* fileName ) {
	idFile* file = fileSystem->OpenFileWrite( fileName );
	if ( file == NULL ) {
		gameLocal.Warning( "Failed to open '%s'", fileName );
		return false;
	}

	file->Write( info.c_str(), info.Length() );
	fileSystem->CloseFile( file );

	return true;
}

/*
==================
sdWikiFormatter::CopyToClipBoard
==================
*/
void sdWikiFormatter::CopyToClipBoard( void ) {
	sys->SetClipboardData( va( L"%hs", info.c_str() ) );
}

/*
==================
sdWikiFormatter::BuildClassTree_r
==================
*/
void sdWikiFormatter::BuildClassTree_r( const idTypeInfo* type, int tabCount ) {
	int count = 0;
	for ( int i = 0; i < idEventDef::NumEventCommands(); i++ ) {
		const idEventDef* evt = idEventDef::GetEventCommand( i );

		if ( !evt->GetAllowFromScript() ) {
			continue;
		}

		if ( !type->RespondsTo( *evt ) ) {
			continue;
		}

		bool parentRepspondsTo = type->super == NULL ? false : type->super->RespondsTo( *evt );
		if ( parentRepspondsTo ) {
			continue;
		}

		if ( evt->GetDescription() == NULL ) {
			count++;
		}
	}

	for ( int i = 0; i < tabCount; i++ ) {
		info += '*';
	}
	AddEntityClassLink( type, info );
	if ( count > 0 ) {
		info += va( " %i events left", count );
	}
	info += "\r\n";

	for ( int i = 0; i < idClass::GetNumTypes(); i++ ) {
		idTypeInfo* other = idClass::GetType( i );
		if ( other->super != type ) {
			continue;
		}

		BuildClassTree_r( other, tabCount + 1 );
	}
}

/*
==================
sdWikiFormatter::FormatCodeWord
==================
*/
void sdWikiFormatter::FormatCodeWord( const char* word ) {
	const char* keywords[] = {
		"if",
		"while",
		"for",
		"void",
		"boolean",
		"string",
		"wstring",
		"handle",
		"float",
		"entity",
		"object",
		"vector",
		"thread",
		"return",
		"self",
		"$null",
		"$null_entity",
		"else",
		"do",
		"scriptEvent",
		"virtual",
	};
	int count = _arraycount( keywords );

	if ( word[ 0 ] == '#' ) {
		info += va( "<span style=\"color:blue\">%s</span>", word );
		return;
	}

	if ( word[ 0 ] == '"' ) {
		info += va( "<span style=\"color:brown\">%s</span>", word );
		return;
	}

	for ( int i = 0; i < count; i++ ) {
		if ( idStr::Cmp( keywords[ i ], word ) != 0 ) {
			continue;
		}

		info += va( "<span style=\"color:blue\">%s</span>", word );
		return;
	}

	const idEventDef* def = idEventDef::FindEvent( word );
	if ( def != NULL ) {
		AddScriptEventLink( def, info );
		return;
	}

	idProgram* program = dynamic_cast< idProgram* >( gameLocal.program );
	if ( program != NULL ) {
		idTypeDef* type = program->FindType( word );
		if ( type != NULL && type->Type() == ev_object ) {
			AddScriptClassLink( type, info );
			return;
		}
	}

	info += word;
}

/*
==================
sdWikiFormatter::BreaksCodeWord
==================
*/
bool sdWikiFormatter::BreaksCodeWord( char c ) {
	if ( c <= ' ' ) {
		return true;
	}

	return idStr::FindChar( "!.()<>{}=;:|&*+-/", c ) != -1;
}

/*
==================
sdWikiFormatter::FormatCodeLine
==================
*/
void sdWikiFormatter::FormatCodeLine( const char* line, bool& inComment ) {
	int len = idStr::Length( line );

	int i;
	for ( i = 0; i < len; i++ ) {
		if ( line[ i ] > ' ' ) {
			break;
		}
	}
	if ( i == len ) {
		info += " \r\n";
		return; // ignore the content of any lines which are only whitespace
	}

	info += ' ';

	int index = 0;
	bool inString = false;
	int wordStart = inComment ? 0 : -1;
	while ( index < len ) {
		if ( inComment ) {
			if ( line[ index ] == '*' && line[ index + 1 ] == '/' ) {
				idStr temp( line, wordStart, index + 2 );
				info += va( "<span style=\"color:green\">%s</span>", temp.c_str() );
				wordStart = -1;
				index++;
				inComment = false;
			}
			index++;
			continue;
		}

		if ( BreaksCodeWord( line[ index ] ) ) {
			if ( !inString ) {
				if ( wordStart != -1 ) {
					idStr temp( line, wordStart, index );
					FormatCodeWord( temp.c_str() );
					wordStart = -1;
				}

				if ( line[ index ] == '/' && line[ index + 1 ] == '/' ) {
					info += va( "<span style=\"color:green\">%s</span>", &line[ index ] );
					break;
				} else if ( line[ index ] == '/' && line[ index + 1 ] == '*' ) {
					wordStart = index;
					inComment = true;
					index += 2;
					continue;
				} else if ( line[ index ] == '\t' ) {
					info += "    ";
				} else {
					info += line[ index ];
				}
			}
			index++;
			continue;
		}

		if ( line[ index ] == '"' ) {
			inString = !inString;

			if ( wordStart != -1 ) {
				if ( !inString ) {
					index++;
				}
				idStr temp( line, wordStart, index );
				FormatCodeWord( temp.c_str() );
				wordStart = -1;
				if ( !inString ) {
					continue;
				}
			}
		}

		if ( wordStart == -1 ) {
			wordStart = index;
		}
		index++;
	}

	if ( wordStart != -1 ) {
		idStr temp( line, wordStart, len );
		if ( inComment ) {
			info += va( "<span style=\"color:green\">%s</span>", temp.c_str() );
		} else {
			FormatCodeWord( temp.c_str() );
		}
	}

	info += "\r\n";
}

/*
==================
sdWikiFormatter::FormatCode
==================
*/
void sdWikiFormatter::FormatCode( const char* fileName ) {
	idFile* inputFile = fileSystem->OpenFileRead( fileName );
	if ( inputFile == NULL ) {
		return;
	}

	bool inComment = false;

	sdStringBuilder_Heap line;

	while ( true ) {
		if ( inputFile->Tell() == inputFile->Length() ) {
			break;
		}

		char temp;
		inputFile->ReadChar( temp );

		if ( temp == '\r' ) {
			continue;
		}
		if ( temp == '\n' ) {
			FormatCodeLine( line.c_str(), inComment );
			line.Clear();
			continue;
		}

		line += temp;
	}

	if ( line.Length() != 0 ) {
		FormatCodeLine( line.c_str(), inComment );
	}
}

/*
==================
sdWikiFormatter::BuildScriptClassTree_r
==================
*/
void sdWikiFormatter::BuildScriptClassTree_r( const sdProgram::sdTypeInfo* type, int tabCount ) {
	for ( int i = 0; i < tabCount; i++ ) {
		info += '*';
	}
	AddScriptClassLink( type, info );
	info += "\r\n";

	sdProgram* program = gameLocal.program;
	int count = program->GetNumClasses();
	for ( int i = 0; i < count; i++ ) {
		const sdProgram::sdTypeInfo* subType = program->GetClass( i );
		if ( subType->GetSuperClass() != type ) {
			continue;
		}

		BuildScriptClassTree_r( subType, tabCount + 1 );
	}
}

/*
==================
sdWikiFormatter::BuildScriptClassTree
==================
*/
void sdWikiFormatter::BuildScriptClassTree( const sdProgram::sdTypeInfo* type ) {
	if ( type == NULL ) {
		sdProgram* program = gameLocal.program;
		int count = program->GetNumClasses();
		for ( int i = 0; i < count; i++ ) {
			const sdProgram::sdTypeInfo* other = program->GetClass( i );
			if ( other->GetSuperClass()->GetSuperClass() != NULL ) {
				continue;
			}

			BuildScriptClassTree_r( other, 1 );
		}
	} else {
		BuildScriptClassTree_r( type, 1 );
	}
}

/*
==================
sdWikiFormatter::BuildScriptFileList
==================
*/
void sdWikiFormatter::BuildScriptFileList( void ) {
	idProgram* program = dynamic_cast< idProgram* >( gameLocal.program );
	if ( program == NULL ) {
		return;
	}

	for ( int i = 0; i < program->NumFilenames(); i++ ) {
		info += '*'; 
		AddScriptFileLink( program->GetFilename( i ), info );
		info += "\r\n";
	}
}

/*
==================
sdWikiFormatter::BuildScriptClassInfo
==================
*/
void sdWikiFormatter::BuildScriptClassInfo( const sdProgram::sdTypeInfo* type ) {
	idProgram* program = dynamic_cast< idProgram* >( gameLocal.program );
	if ( program == NULL ) {
		return;
	}

	idStrList files;

	idTypeDef* realType = ( idTypeDef* )type;
	for ( int i = 0; i < realType->NumFunctions(); i++ ) {
		const function_t* function = realType->GetFunction( i );
		if ( function->def->scope->TypeDef() != realType ) {
			continue;
		}

		const char* fileName = program->GetFilename( program->GetStatement( function->firstStatement ).file );
		files.AddUnique( fileName );
	}

	AddHeader( "Implemented In" );
	for ( int i = 0; i < files.Num(); i++ ) {
		info += '*'; 
		AddScriptFileLink( files[ i ].c_str(), info );
		info += "\r\n";
	}
}
