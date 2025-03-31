/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "declxdata.h"

tdmDeclXData::tdmDeclXData()
{
}

tdmDeclXData::~tdmDeclXData()
{
	FreeData();
}

size_t tdmDeclXData::Size() const
{
	return sizeof(tdmDeclXData);
}

const char *tdmDeclXData::DefaultDefinition() const
{
	return "{}";
}

void tdmDeclXData::FreeData()
{
	m_data.ClearFree();
}

// Note Our coding standards require using gotos in this sort of code.
bool tdmDeclXData::Parse( const char *text, const int textLength )
{
	// Only set to true if we have successfully parsed the decl.
	bool		successfulParse = false;

	idLexer		src;
	idToken		tKey;
	idToken		tVal;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );

	bool		precache = false;
	idStr		value;
	idDict		importKeys;
	const tdmDeclXData *xd;
	const idDict *importData;
	const idKeyValue *kv;
	const idKeyValue *kv2;
	int i;

	// Skip until the opening brace. I don't trust using the
	// skipUntilString function, since it could be fooled by
	// a string containing a brace.
	do {
		if ( !src.ReadToken( &tKey ) ) {
			goto Quit;
		}
	} while ( tKey.type != TT_PUNCTUATION || tKey.subtype != P_BRACEOPEN );
	//src.SkipUntilString( "{" );

	while (1)
	{
		// If there's an EOF, fail to load.
		if ( !src.ReadToken( &tKey ) ) {
			src.Warning( "Unclosed xdata decl." );
			goto Quit;
		}

		// Quit upon encountering the closing brace.
		if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACECLOSE) {
			break;
		}

		if ( tKey.type == TT_STRING ) {

			if ( !src.ReadToken( &tVal ) ||
				 tVal.type != TT_PUNCTUATION ||
				 tVal.subtype != P_COLON ) {
				src.Warning( "Abandoned key: %s", tKey.c_str() );
				goto Quit;
			}

			// We're parsing a key/value pair.
			if ( !src.ReadToken( &tVal ) ) {
				src.Warning("Unexpected EOF in key:value pair.");
				goto Quit;
			}

			if ( tVal.type == TT_STRING ) {

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), tVal.c_str() );

			} else if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACEOPEN ) {

				value = "";

				while (1) {
					if ( !src.ReadToken( &tVal ) ) {
						src.Warning("EOF encountered inside value block.");
						goto Quit;
					}

					if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_BRACECLOSE ) {
						break;
					}

					if ( tVal.type != TT_STRING ) {
						src.Warning( "Non-string encountered in value block: %s", tVal.c_str() );
						goto Quit;
					}

					value += tVal + "\n";
				}

				// Set the key:value pair.
				m_data.Set( tKey.c_str(), value.c_str() );

			} else {
				src.Warning( "Invalid value: %s", tVal.c_str() );
				goto Quit;
			}

		} else if ( tKey.type == TT_NAME ) {

			if ( tKey.Icmp("precache") == 0 ) {
				precache = true;
			} else if ( tKey.Icmp("import") == 0 ) {

				if ( !src.ReadToken( &tKey ) )
				{
					src.Warning("Unexpected EOF in import statement.");
					goto Quit;
				}

				if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACEOPEN ) {

					// Initialize the list of keys to copy over.
					importKeys.Clear();

					while (1) {

						if ( !src.ReadToken( &tKey ) ) {
							src.Warning("Unexpected EOF in import block.");
							goto Quit;
						}

						if ( tKey.type == TT_PUNCTUATION && tKey.subtype == P_BRACECLOSE ) {
							break;
						}

						if ( tKey.type != TT_STRING ) {
							src.Warning( "Invalid source key: %s", tKey.c_str() );
							goto Quit;
						}

						if ( !src.ReadToken( &tVal ) ) {
							src.Warning("Unexpected EOF in import block.");
							goto Quit;
						}

						if ( tVal.type == TT_PUNCTUATION && tVal.subtype == P_POINTERREF ) {

							if ( !src.ReadToken( &tVal ) ) {
								src.Warning("Unexpected EOF in import block.");
								goto Quit;
							}

							if ( tVal.type != TT_STRING ) {
								src.Warning( "Invalid target key: %s", tVal.c_str() );
								goto Quit;
							}

							importKeys.Set( tKey.c_str(), tVal.c_str() );

						} else {

							// We accidently read too far.
							src.UnreadToken( &tVal );

							importKeys.Set( tKey.c_str(), tKey.c_str() );
						}

					}

					if ( !src.ReadToken( &tKey ) ||
						 tKey.type != TT_NAME ||
						 tKey.Icmp("from") != 0 ) {
						src.Warning( "Missing from statement: %s.", tKey.c_str() );
						goto Quit;
					}

					if ( !src.ReadToken( &tKey ) ||
						 tKey.type != TT_STRING ) {
						src.Warning( "Invalid xdata for importation." );
						goto Quit;
					}

					xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, tKey.c_str(), false ) );
					if ( xd != NULL ) {

						importData = &(xd->m_data);

						i = importKeys.GetNumKeyVals();
						while (i--) {
							kv = importKeys.GetKeyVal(i);
							kv2 = importData->FindKey( kv->GetKey() );
							m_data.Set( kv->GetValue(), kv2->GetValue() );
						}

					} else {
						src.Warning( "Unable to load xdata for importation: %s", tKey.c_str() );
						goto Quit;
					}

				} else if ( tKey.type == TT_STRING ) {

					xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, tKey.c_str(), false ) );
					if ( xd != NULL ) {
						m_data.Copy( xd->m_data );
					} else {
						src.Warning( "Unable to load xdata for importation: %s", tKey.c_str() );
						goto Quit;
					}

				} else {
					src.Warning("Syntax error immediately after import statement.");
					goto Quit;
				}

			} else {
				src.Warning( "Unrecognized command: %s", tKey.c_str() );
				goto Quit;
			}

		}
	}

	if (precache) {
		gameLocal.CacheDictionaryMedia( &m_data );
	}

	successfulParse = true;

	Quit:
	if (!successfulParse) {
		MakeDefault();
	}
	return successfulParse;
}
