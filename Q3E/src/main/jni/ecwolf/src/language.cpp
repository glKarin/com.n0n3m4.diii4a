/*
** language.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "language.h"
#include "w_wad.h"
#include "scanner.h"

Language language;

void Language::SetupStrings(const char* language)
{
	// First load Blake Stone's strings so we can replace them in LANGUAGE later
	SetupBlakeStrings("LEVELDSC", "BLAKE_AREA_");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("LANGUAGE", &lastLump)) != -1)
	{
		// Read the lump defaulting to english
		ReadLump(lump, language == NULL ? "enu" : language);
	}
}

void Language::ReadLump(int lump, const char* language)
{
	Scanner sc(lump);

	int token = TK_NoToken;
	bool skip = false;
	bool noReplace = false;
	while(sc.GetNextToken())
	{
		token = sc->token;
		if(token == '[')
		{
			// match with language
			sc.MustGetToken(TK_Identifier);
			if(sc->str.Compare(language) != 0)
				skip = true;
			else
			{
				skip = false;
				noReplace = false;
			}

			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.Compare("default") == 0)
				{
					// if not the correct language, go in no replace mode.
					if(skip)
					{
						skip = false;
						noReplace = true;
					}
				}
				else
				{
					sc.ScriptMessage(Scanner::ERROR, "Unexpected identifier '%s'", sc->str.GetChars());
				}
			}

			sc.MustGetToken(']');
		}
		else if(token == TK_Identifier)
		{
			FName index = sc->str;
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			if(!skip)
			{
				if(!noReplace || (noReplace && strings.CheckKey(index) == NULL))
					strings[index] = sc->str;
			}
			sc.MustGetToken(';');
		}
		else
		{
			sc.ScriptMessage(Scanner::ERROR, "Unexpected token.");
		}
	}
}

/* Blake Stone strings are stored in text chunks. They're referenced by an
 * index. Strings are stored separated by ^XX at the end of a line.
 */
void Language::SetupBlakeStrings(const char* lumpname, const char* prefix)
{
	int lumpnum = Wads.CheckNumForName(lumpname);
	if(lumpnum == -1)
		return;

	FMemLump wadLump = Wads.ReadLump(lumpnum);

	unsigned int num = 1; // Start at prefix_1
	unsigned int pos = 0;
	unsigned int start = 0;
	const char* data = reinterpret_cast<const char*>(wadLump.GetMem());
	static const WORD endToken = ('X'<<8)|'X'; // Since both chars are the same this should be endian safe
	while(pos+2 < wadLump.GetSize())
	{
		if(data[pos] == '^' && *(WORD*)(data+pos+1) == endToken)
		{
			FString name;
			FString str(data+start, pos-start);
			name.Format("%s%d", prefix, num++);

			strings[name] = str;

			pos += 3;
			while((data[pos] == '\n' || data[pos] == '\r') && pos < wadLump.GetSize())
				++pos;
			start = pos;
		}
		else
			++pos;
	}
}

const char* Language::operator[] (const char* index) const
{
	const FString *it = strings.CheckKey(index);
	if(it != NULL)
		return it->GetChars();
	return index;
}
