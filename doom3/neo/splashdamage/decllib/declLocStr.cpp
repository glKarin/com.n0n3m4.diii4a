// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "declLocStr.h"
#include "framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclLocStr

===============================================================================
*/

size_t sdDeclLocStr::Size( void ) const {
	return sizeof(sdDeclLocStr);
}

const char * sdDeclLocStr::DefaultDefinition() const {
	return "{  }";
}

bool sdDeclLocStr::Parse( const char *text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclLocStr::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "text" )) {
			if( !src.ReadToken(&token)) {
				src.Error( "sdDeclLocStr::Parse: failed to parse text" );
				break;
			}
			locText = common->GetLanguageDict()->GetString(token);
			continue;
		}

		if( !token.Icmp( "arguments" )) {
			numArgs = src.ParseInt();
			continue;
		}

		src.Warning( "sdDeclLocStr::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

void sdDeclLocStr::FreeData( void ) {
	locText.Clear();
	numArgs = 0;
}

void sdDeclLocStr::Print( void ) const {
}

bool sdDeclLocStr::Format( idWStr& result, const idWStrList& inputs ) const {
	sdWStringBuilder_Heap buf;
	wchar_t ch;
	bool found = false;
	int bracketFound = 0;
	idStr temp;
	int i = 0;

	while (i < locText.Length()) {
		ch = locText[i];

		 if (ch == L'[') { //skip [special label]
			bracketFound++;
			i++;
			continue;
		}
		else if (ch == L']') { //skip [special label]
			bracketFound--;
			i++;
			continue;
		}
		if(bracketFound)
		{
			i++;
			continue;
		}

		// color
		if (idWStr::IsColor(locText.c_str() + i)) {
			buf.Append(locText.c_str() + i, 2);
			i += 2;
			continue;
		}

		if (ch == L'%') {
			if (found) {
				if (temp.IsEmpty()) { // %%
					found = false;
					buf.Append(L'%');
				}
				else { // %1
					int placeholderIndex = atoi(temp.c_str()) - 1;
					if (placeholderIndex >= 0 && placeholderIndex < inputs.Num()) {
						buf.Append(inputs[placeholderIndex].c_str());
					}
					else {
						for (int m = 0; m < temp.Length(); ++m) {
							buf.Append(temp[i]);
						}
					}
					temp.Clear();
					found = false;
				}
			}
			else {
				found = true;
				temp.Clear();
			}
		}
		else if (ch >= L'0' && ch <= L'9') {
			if (found) {
				temp.Append((char)ch);
			}
			else {
				buf.Append(ch);
			}
		}
		else {
			// handle last
			if (found) {
				if (temp.IsEmpty()) { // %%
					found = false;
					buf.Append(L'%');
				}
				else { // %1
					int placeholderIndex = atoi(temp.c_str()) - 1;
					if (placeholderIndex >= 0 && placeholderIndex < inputs.Num()) {
						buf.Append(inputs[placeholderIndex].c_str());
					}
					else {
						for (int m = 0; m < temp.Length(); ++m) {
							buf.Append(temp[i]);
						}
					}
					temp.Clear();
					found = false;
				}
			}
			// append now
			buf.Append(ch);
		}

		i++;
	}

	result = buf.c_str();
	// handle real [ ] escape
	result.Replace(L"&lbr", L"[");
	result.Replace(L"&rbr", L"]");
	// for (int i = 0; i < inputs.Num(); i++) {
	// 	common->Printf("%d:%ls|||", i, inputs[i].c_str());
	// }
	// common->Printf("\nLLL|%ls|%ls\n", locText.c_str(), buf.c_str());
	return true;
}
