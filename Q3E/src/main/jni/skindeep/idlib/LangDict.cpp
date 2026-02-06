/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/Lexer.h"
#include "framework/FileSystem.h"

#include "idlib/LangDict.h"

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
bool idLangDict::Load( const char *fileName, bool clear /* _D3XP */ ) {

	if ( clear ) {
		Clear();
	}

	const char *buffer = NULL;
	idLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOCOMMENTSAREWHITESPACE );

	int len = idLib::fileSystem->ReadFile( fileName, (void**)&buffer );
	if ( len <= 0 ) {
		// let whoever called us deal with the failure (so sys_lang can be reset)
		return false;
	}

	// SM: Validate for UTF-8 BOM in .lang files, and skip over it for parsing
	if ( len < 3 || buffer[0] != '\xef' || buffer[1] != '\xbb' || buffer[2] != '\xbf' ) {
		idLib::common->Error( "Language dictionary %s is missing required UTF-8 BOM.", fileName );
		return false;
	}

	src.LoadMemory( buffer + 3, strlen( buffer ) - 3, fileName );
	//src.LoadMemory( buffer, strlen( buffer ), fileName );
	if ( !src.IsLoaded() ) {
		return false;
	}

	int lineIndex = 0;
	idToken tok, tok2, commentTok;
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
			// SM: Check to see if it has a comment which we'll save in the dictionary to help w/ localization
			if ( src.PeekTokenType( TT_COMMENT, 0, &commentTok ) ) {
				src.ReadToken( &commentTok ); // Actual comment
				kv.comment = commentTok;
			}
			// SM: Also allow #font_ keys because BFG font system needs them

			if (kv.key.Icmpn(STRTABLE_ID, STRTABLE_ID_LENGTH) == 0 || kv.key.Icmpn("#font_", 6) == 0)
			{
				//string is good.
			}
			else
			{
				//string is bad.
				idLib::common->Error("***** BAD LOCALIZATION STRING (line %d): %s", lineIndex + 1, kv.key.c_str());
				return false;
			}

			hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
			lineIndex++;
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
	// SM: Write the UTF-8 BOM
	outFile->WriteChar( '\xef' );
	outFile->WriteChar( '\xbb' );
	outFile->WriteChar( '\xbf' );
	outFile->WriteFloatString( "\n\n{\n" );
	for ( int j = 0; j < args.Num(); j++ ) {
		outFile->WriteFloatString( "\t\"%s\"\t\"", args[j].key.c_str() );
		int l = args[j].value.Length();
		char slash = '\\';
		char tab = 't';
		char nl = 'n';
		char qt = '\"';
		for ( int k = 0; k < l; k++ ) {
			char ch = args[j].value[k];
			if ( ch == '\t' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &tab, 1 );
			} else if ( ch == '\n' || ch == '\r' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &nl, 1 );
			} else if ( ch == '\"' ) { // SM: Fix bug with it not escaping quotes inside the value
				outFile->Write( &slash, 1 );
				outFile->Write( &qt, 1 );
			} else {
				outFile->Write( &ch, 1 );
			}
		}

		// SM: Write out comment if there is one
		if ( args[j].comment.Length() > 0 ) {
			outFile->WriteFloatString( "\" //" );
			outFile->WriteFloatString( args[j].comment.c_str() );
			outFile->WriteChar( '\n' );
		} else {
			outFile->WriteFloatString( "\"\n" );
		}
	}
	outFile->WriteFloatString( "\n}\n" );
	idLib::fileSystem->CloseFile( outFile );
}

/*
============
idLangDict::GetString
============
*/
const char *idLangDict::GetString( const char *str, bool warnIfMissing /*= true*/ ) const {

	if ( str == NULL || str[0] == '\0' ) {
		return "";
	}

	bool isFont = idStr::Icmpn( str, "#font_", 6 ) == 0;
	if ( idStr::Icmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 &&
		 !isFont ) {
		return str;
	}

	int hashKey = GetHashKey( str );
	for ( int i = hash.First( hashKey ); i != -1; i = hash.Next( i ) ) {
		if ( args[i].key.Icmp( str ) == 0 ) {
			return args[i].value;
		}
	}

	// SM: Added this because it's valid for the #font remappings to not exist
	if ( warnIfMissing && !isFont ) {
		idLib::common->Warning( "Unknown string id %s", str );
	}
	return str;
}

/*
============
idLangDict::AddString
============
*/
const char *idLangDict::AddString( const char *str, const char *prefix /*= "#str"*/ ) {

	if ( ExcludeString( str ) ) {
		return str;
	}

	int c = args.Num();
	for ( int j = 0; j < c; j++ ) {
		if ( idStr::Icmp( args[j].value, str ) == 0 ){
			return args[j].key;
		}
	}

	int id = GetNextId();
	idLangKeyValue kv;
	// _D3XP
	// SM: Added support for arbitrary prefixes when auto-generating keys
	kv.key = va( "%s_%06i", prefix, id );
	// kv.key = va( "#str_%05i", id );
	kv.value = str;
	c = args.Append( kv );
	assert( kv.key.Icmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
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
	assert( kv.key.Icmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
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

	if ( idStr::Icmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
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
		// SM: Change how this finds the ID for finding a unique number
		//work = args[j].key;
		int splitIdx = args[j].key.Last( '_' );
		args[j].key.Mid( splitIdx + 1, args[j].key.Length() - splitIdx - 1, work );
		//work.StripLeading( STRTABLE_ID );
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
	// SM: Change to use IHash from idStr (as in BFG)
	return idStr::IHash( str );
// 	int hashKey = 0;
// 	for ( str += STRTABLE_ID_LENGTH; str[0] != '\0'; str++ ) {
// 		assert( str[0] >= '0' && str[0] <= '9' );
// 		hashKey = hashKey * 10 + str[0] - '0';
// 	}
// 	return hashKey;
}

/*
============
idLangDict::Contains
// SM: Tells you if key is in lang dict
============
*/
bool idLangDict::ContainsKey( const char* str )
{
	if ( str == NULL || str[0] == '\0' ) {
		return false;
	}

	bool isFont = idStr::Icmpn( str, "#font_", 6 ) == 0;
	if ( idStr::Icmpn( str, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 &&
		!isFont ) {
		return false;
	}

	int hashKey = GetHashKey( str );
	for ( int i = hash.First( hashKey ); i != -1; i = hash.Next( i ) ) {
		if ( args[i].key.Icmp( str ) == 0 ) {
			return true;
		}
	}

	return false;
}

/*
============
idLangDict::AddMissingKeys
// SM: Will add any missing keys from other idLangDict into this one
============
*/
void idLangDict::AddMissingKeys( const idLangDict& otherDict )
{
	int c = otherDict.args.Num();
	for ( int j = 0; j < c; j++ ) {
		if ( !ContainsKey( otherDict.args[j].key ) ) {
			idLangKeyValue kv;
			kv.key = otherDict.args[j].key;
			kv.value = otherDict.args[j].value;
			kv.comment = otherDict.args[j].comment;
			hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
		}
	}
}
