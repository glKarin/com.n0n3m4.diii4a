
#include "precompiled.h"
#pragma hdrstop


/*
============
idLangDict::idLangDict
============
*/
idLangDict::idLangDict( void ) {
	args.SetGranularity( 256 );
	hash.SetGranularity( 256 );
	hash.Clear( 4096, 8192 );
	baseID = 0;
}

/*
============
idLangDict::~idLangDict
============
*/
idLangDict::~idLangDict( void ) {
	Clear();
}

/*
============
idLangDict::Clear
============
*/
void idLangDict::Clear( void ) {
	args.Clear();
	hash.Clear();
}

/*
============
idLangDict::Load
============
*/
bool idLangDict::Load( const char *fileName, bool clear ) {
	if ( clear ) {
		Clear();
	}

	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	int len = idLib::fileSystem->ReadFile( fileName, (void**)&buffer );
	if ( len <= 0 ) {
		// let whoever called us deal with the failure (so sys_lang can be reset)
		return false;
	}
	src.LoadMemory( buffer, strlen( buffer ), fileName );
	if ( !src.IsLoaded() ) {
		return false;
	}

	idToken tok, tok2;
	src.ExpectTokenString( "{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == "}" ) {
			break;
		}

		if ( src.ReadToken( &tok2 ) ) {
			if ( tok2 == "}" ) {
				break;
			}
			idLangKeyValue kv;
			kv.key = tok;
			kv.value = tok2;
// RAVEN BEGIN
			if( kv.key.CmpPrefix( STRTABLE_ID ) ) {
				common->Warning( "Invalid token id \'%s\' in \'%s\' line %d", kv.key.c_str(), fileName, src.GetLineNum() );
			} else {
				hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
			}
// RAVEN END
		}
	}
	idLib::common->Printf( "%i strings read from %s\n", args.Num(), fileName );
	idLib::fileSystem->FreeFile( (void*)buffer );
	
	return true;
}

/*
============
idLangDict::Save
============
*/
void idLangDict::Save( const char *fileName ) {
	idFile *outFile = idLib::fileSystem->OpenFileWrite( fileName );
// RAVEN BEGIN
	if( !outFile ) {
		common->Printf( "Could not open file \'%s\'for writing\n", fileName );
		return;
	}
// RAVEN END
	outFile->WriteFloatString( "// string table" NEWLINE "// english" NEWLINE "//" NEWLINE NEWLINE "{" NEWLINE );
	for ( int j = 0; j < args.Num(); j++ ) {
		outFile->WriteFloatString( "\t\"%s\"\t\"", args[j].key.c_str() );
		int l = args[j].value.Length();
		char slash = '\\';
		char tab = 't';
		char nl = 'n';
		for ( int k = 0; k < l; k++ ) {
			char ch = args[j].value[k];
			if ( ch == '\t' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &tab, 1 );
			} else if ( ch == '\n' || ch == '\r' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &nl, 1 );
			} else {
				outFile->Write( &ch, 1 );
			}
		}
		outFile->WriteFloatString( "\"" NEWLINE );
	}
	outFile->WriteFloatString( NEWLINE "}" NEWLINE );
	idLib::fileSystem->CloseFile( outFile );
}

/*
============
idLangDict::GetString
============
*/
const char *idLangDict::GetString( const char *str ) const {

	if ( str == NULL || str[0] == '\0' ) {
		return "";
	}

	if ( idStr::Cmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 ) {
		return str;
	}

	int hashKey = GetHashKey( str );
	for ( int i = hash.First( hashKey ); i != -1; i = hash.Next( i ) ) {
		if ( args[i].key.Cmp( str ) == 0 ) {
			return args[i].value;
		}
	}

	idLib::common->Warning( "Unknown string id %s", str );
	return str;
}

/*
============
idLangDict::AddString
============
*/
const char *idLangDict::AddString( const char *str ) {
	
	if ( ExcludeString( str ) ) {
		return str;
	}

	int c = args.Num();
	for ( int j = 0; j < c; j++ ) {
		if ( idStr::Cmp( args[j].value, str ) == 0 ){
			return args[j].key;
		}
	}

	int id = GetNextId();
	idLangKeyValue kv;
	kv.key = va( "#str_%06i", id );
	kv.value = str;
	c = args.Append( kv );
	assert( kv.key.CmpPrefix( STRTABLE_ID ) == 0 );
	hash.Add( GetHashKey( kv.key ), c );
	return args[c].key;
}

/*
============
idLangDict::GetNumKeyVals
============
*/
int idLangDict::GetNumKeyVals( void ) const {
	return args.Num();
}

/*
============
idLangDict::GetKeyVal
============
*/
const idLangKeyValue * idLangDict::GetKeyVal( int i ) const {
	return &args[i];
}

/*
============
idLangDict::AddKeyVal
============
*/
void idLangDict::AddKeyVal( const char *key, const char *val ) {
	idLangKeyValue kv;
	kv.key = key;
	kv.value = val;
	assert( kv.key.CmpPrefix( STRTABLE_ID ) == 0 );
	hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
}

/*
============
idLangDict::ExcludeString
============
*/
bool idLangDict::ExcludeString( const char *str ) const {
	if ( str == NULL ) {
		return true;
	}

	int c = strlen( str );
	if ( c <= 1 ) {
		return true;
	}

	if ( idStr::Cmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
		return true;
	}

	if ( idStr::Icmpn( str, "gui::", strlen( "gui::" ) ) == 0 ) {
		return true;
	}

	if ( str[0] == '$' ) {
		return true;
	}

	int i;
	for ( i = 0; i < c; i++ ) {
		if ( isalpha( str[i] ) ) {
			break;
		}
	}
	if ( i == c ) {
		return true;
	}
	
	return false;
}

/*
============
idLangDict::GetNextId
============
*/
int idLangDict::GetNextId( void ) const {
	int c = args.Num();
	//Let and external user supply the base id for this dictionary
	int id = baseID;

	if ( c == 0 ) {
		return id;
	}

	idStr work;
	for ( int j = 0; j < c; j++ ) {
		work = args[j].key;
		work.StripLeading( STRTABLE_ID );
		int test = atoi( work );
		if ( test > id ) {
			id = test;
		}
	}
	return id + 1;
}

/*
============
idLangDict::GetHashKey
============
*/
int idLangDict::GetHashKey( const char *str ) const {
	int hashKey = 0;
	for ( str += STRTABLE_ID_LENGTH; str[0] != '\0'; str++ ) {
		assert( str[0] >= '0' && str[0] <= '9' );
		hashKey = hashKey * 10 + str[0] - '0';
	}
	return hashKey;
}
