/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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

#include "precompiled.h"
#pragma hdrstop


/*
============
idLangDict::idLangDict
============
*/
idLangDict::idLangDict(void)
{
	args.SetGranularity(256);
	hash.SetGranularity(256);
	hash.Clear(4096, 8192);
	baseID = 0;
}

/*
============
idLangDict::~idLangDict
============
*/
idLangDict::~idLangDict(void)
{
	Clear();
}

/*
============
idLangDict::Clear
============
*/
void idLangDict::Clear(void)
{
	args.Clear();
	hash.Clear();
}

/*
============
idLangDict::Load
============
*/
bool idLangDict::Load(const char *fileName, bool clear /* _D3XP */)
{

	if (clear) {
		Clear();
	}

	const char *buffer = NULL;

	idLexer src(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT);

	int len = idLib::fileSystem->ReadFile(fileName, (void **)&buffer);

	if (len <= 0) {
		// let whoever called us deal with the failure (so sys_lang can be reset)
		return false;
	}

#ifdef _WCHAR_LANG
    bool utf8 = false;

    utf8Encoding_t encoding;
    idStr::IsValidUTF8( buffer, len, encoding );
    if( encoding == UTF8_INVALID )
    {
        idLib::Warning( "Language file %s is not valid UTF-8 or plain ASCII.", fileName );
    }
    else if( encoding == UTF8_INVALID_BOM )
    {
        idLib::Warning( "Language file %s is marked as UTF-8 but has invalid encoding.", fileName );
    }
    else if( encoding == UTF8_ENCODED_NO_BOM )
    {
        idLib::Warning( "Language file %s has no byte order marker. Fix this or roll back to a version that has the marker.", fileName );
    }
    else if( encoding != UTF8_ENCODED_BOM && encoding != UTF8_PURE_ASCII )
    {
        idLib::Warning( "Language file %s has unknown utf8Encoding_t.", fileName );
    }

    if( encoding == UTF8_ENCODED_BOM )
    {
        utf8 = true;
    }
#if 0 // else always as ascii text
    else if( encoding == UTF8_PURE_ASCII )
    {
        utf8 = false;
    }
    else
    {
        assert( false );	// this should have been handled in VerifyUTF8 with a FatalError
        return false;
    }
#endif

    if( utf8 )
    {
        idLib::common->Printf( " as UTF-8\n" );
    }
    else
    {
        idLib::common->Printf( " as ASCII\n" );
    }

    if(utf8)
    {
        return LoadUTF8((const byte* )buffer, len, fileName);
    }
#endif
	src.LoadMemory(buffer, strlen(buffer), fileName);

	if (!src.IsLoaded()) {
		return false;
	}

	idToken tok, tok2;
	src.ExpectTokenString("{");

	while (src.ReadToken(&tok)) {
		if (tok == "}") {
			break;
		}

		if (src.ReadToken(&tok2)) {
			if (tok2 == "}") {
				break;
			}

			idLangKeyValue kv;
			kv.key = tok;
			kv.value = tok2;
			// DG: D3LE has #font_ entries in english.lang, maybe from D3BFG? not supported here, just skip them
			if(kv.key.Cmpn("#font_", 6) != 0) {
			assert(kv.key.Cmpn(STRTABLE_ID, STRTABLE_ID_LENGTH) == 0);
			hash.Add(GetHashKey(kv.key), args.Append(kv));
			}
		}
	}

	idLib::common->Printf("%i strings read from %s\n", args.Num(), fileName);
	idLib::fileSystem->FreeFile((void *)buffer);

	return true;
}

/*
============
idLangDict::Save
============
*/
void idLangDict::Save(const char *fileName)
{
	idFile *outFile = idLib::fileSystem->OpenFileWrite(fileName);
	outFile->WriteFloatString("// string table\n// english\n//\n\n{\n");

	for (int j = 0; j < args.Num(); j++) {
		outFile->WriteFloatString("\t\"%s\"\t\"", args[j].key.c_str());
		int l = args[j].value.Length();
		char slash = '\\';
		char tab = 't';
		char nl = 'n';

		for (int k = 0; k < l; k++) {
			char ch = args[j].value[k];

			if (ch == '\t') {
				outFile->Write(&slash, 1);
				outFile->Write(&tab, 1);
			} else if (ch == '\n' || ch == '\r') {
				outFile->Write(&slash, 1);
				outFile->Write(&nl, 1);
#ifdef _HUMANHEAD
			}
            else if ( ch == slash )
            {
                outFile->Write( &slash, 1 );
                outFile->Write( &slash, 1 );
#endif
			} else {
				outFile->Write(&ch, 1);
			}
		}

		outFile->WriteFloatString("\"\n");
	}

	outFile->WriteFloatString("\n}\n");
	idLib::fileSystem->CloseFile(outFile);
}

/*
============
idLangDict::GetString
============
*/
const char *idLangDict::GetString(const char *str) const
{

	if (str == NULL || str[0] == '\0') {
		return "";
	}

	if (idStr::Cmpn(str, STRTABLE_ID, STRTABLE_ID_LENGTH) != 0) {
		return str;
	}

#ifdef _RAVEN //k: Quake4 internal lang string is 6 digits ID, and startis with 1, e.g. #str_107018
	idStr nstr(str);
	const char *ptr = str;
	bool changed = false;
	if(nstr.Length() == STRTABLE_ID_LENGTH + 5) // DOOM3 lang key length
	{
		nstr = idStr(STRTABLE_ID) + "1" + nstr.Right(5); 
		ptr = nstr.c_str();
		changed = true;
	}

	int hashKey = GetHashKey(ptr);

	for (int i = hash.First(hashKey); i != -1; i = hash.Next(i)) {
		if (args[i].key.Cmp(ptr) == 0) {
			return args[i].value;
		}
	}
	// try DOOM3 key
	if(changed)
	{
		hashKey = GetHashKey(ptr);

		for (int i = hash.First(hashKey); i != -1; i = hash.Next(i)) {
			if (args[i].key.Cmp(str) == 0) {
				return args[i].value;
			}
		}
	}

	if(changed)
		idLib::common->Warning("Unknown string id %s(Quake4 string id %s)", str, ptr);
	else
		idLib::common->Warning("Unknown string id %s", str);
	return str;
#else
	int hashKey = GetHashKey(str);

	for (int i = hash.First(hashKey); i != -1; i = hash.Next(i)) {
		if (args[i].key.Cmp(str) == 0) {
			return args[i].value;
		}
	}

	idLib::common->Warning("Unknown string id %s", str);
	return str;
#endif
}

/*
============
idLangDict::AddString
============
*/
const char *idLangDict::AddString(const char *str)
{

	if (ExcludeString(str)) {
		return str;
	}

	int c = args.Num();

	for (int j = 0; j < c; j++) {
		if (idStr::Cmp(args[j].value, str) == 0) {
			return args[j].key;
		}
	}

	int id = GetNextId();
	idLangKeyValue kv;
#ifdef _HUMANHEAD
    //kv.key = va( "#str_%08i", id );
    kv.key = va( "#str_%05i", id );	// HUMANHEAD pdm: changed back
#else
	// _D3XP
	kv.key = va("#str_%08i", id);
	// kv.key = va( "#str_%05i", id );
#endif
	kv.value = str;
#ifdef _WCHAR_LANG
    bool isUtf8 = idStr::IsValidUTF8(str, (int)strlen(str));
    if(isUtf8)
        kv.value.ConvertToUTF8();
#endif
	c = args.Append(kv);
	assert(kv.key.Cmpn(STRTABLE_ID, STRTABLE_ID_LENGTH) == 0);
	hash.Add(GetHashKey(kv.key), c);
	return args[c].key;
}

/*
============
idLangDict::GetNumKeyVals
============
*/
int idLangDict::GetNumKeyVals(void) const
{
	return args.Num();
}

/*
============
idLangDict::GetKeyVal
============
*/
const idLangKeyValue *idLangDict::GetKeyVal(int i) const
{
	return &args[i];
}

/*
============
idLangDict::AddKeyVal
============
*/
void idLangDict::AddKeyVal(const char *key, const char *val)
{
	idLangKeyValue kv;
	kv.key = key;
	kv.value = val;
#ifdef _WCHAR_LANG
    bool isUtf8 = idStr::IsValidUTF8(val, (int)strlen(val));
    if(isUtf8)
        kv.value.ConvertToUTF8();
#endif
	assert(kv.key.Cmpn(STRTABLE_ID, STRTABLE_ID_LENGTH) == 0);
	hash.Add(GetHashKey(kv.key), args.Append(kv));
}

/*
============
idLangDict::ExcludeString
============
*/
bool idLangDict::ExcludeString(const char *str) const
{
	if (str == NULL) {
		return true;
	}

	int c = strlen(str);

	if (c <= 1) {
		return true;
	}

	if (idStr::Cmpn(str, STRTABLE_ID, STRTABLE_ID_LENGTH) == 0) {
		return true;
	}

	if (idStr::Icmpn(str, "gui::", strlen("gui::")) == 0) {
		return true;
	}

	if (str[0] == '$') {
		return true;
	}

	int i;

	for (i = 0; i < c; i++) {
		if (isalpha(str[i])) {
			break;
		}
	}

	if (i == c) {
		return true;
	}

	return false;
}

/*
============
idLangDict::GetNextId
============
*/
int idLangDict::GetNextId(void) const
{
	int c = args.Num();

	//Let and external user supply the base id for this dictionary
	int id = baseID;

	if (c == 0) {
		return id;
	}

	idStr work;

	for (int j = 0; j < c; j++) {
		work = args[j].key;
		work.StripLeading(STRTABLE_ID);
		int test = atoi(work);

		if (test > id) {
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
int idLangDict::GetHashKey(const char *str) const
{
	int hashKey = 0;

	for (str += STRTABLE_ID_LENGTH; str[0] != '\0'; str++) {

	//     (for D3LE mod that seems to have lots of entries like #str_adil_exis_pda_01_audio_info)
		//k assert(str[0] >= '0' && str[0] <= '9');
		hashKey = hashKey * 10 + str[0] - '0';
	}

	return hashKey;
}

#ifdef _WCHAR_LANG
/*
========================
idLangDict::Load
========================
*/
bool idLangDict::LoadUTF8( const byte* buffer, const int bufferLen, const char* name )
{
    if( buffer == NULL || bufferLen <= 0 )
    {
        // let whoever called us deal with the failure (so sys_lang can be reset)
        return false;
    }

    idStr tempKey;
    idStr tempVal;

    int line = 0;
    int numStrings = 0;

    int i = 0;
    while( i < bufferLen )
    {
        uint32_t c = buffer[i++];
        if( c == '/' )    // comment, read until new line
        {
            while( i < bufferLen )
            {
                c = buffer[i++];
                if( c == '\n' )
                {
                    line++;
                    break;
                }
            }
        }
        else if( c == '}' )
        {
            break;
        }
        else if( c == '\n' )
        {
            line++;
        }
        else if( c == '\"' )
        {
            int keyStart = i;
            int keyEnd = -1;
            while( i < bufferLen )
            {
                c = buffer[i++];
                if( c == '\"' )
                {
                    keyEnd = i - 1;
                    break;
                }
            }
            if( keyEnd < keyStart )
            {
                idLib::Error( "%s File ended while reading key at line %d", name, line );
            }
            tempKey.CopyRange( ( char* )buffer, keyStart, keyEnd );

            int valStart = -1;
            while( i < bufferLen )
            {
                c = buffer[i++];
                if( c == '\"' )
                {
                    valStart = i;
                    break;
                }
            }
            if( valStart < 0 )
            {
                idLib::Error( "%s File ended while reading value at line %d", name, line );
            }
            int valEnd = -1;
            tempVal.CapLength( 0 );
            while( i < bufferLen )
            {
                c = idStr::UTF8Char( buffer, i );
                if( c == '\"' )
                {
                    valEnd = i - 1;
                    continue;
                }
                if( c == '\n' )
                {
                    line++;
                    break;
                }
                if( c == '\r' )
                {
                    continue;
                }
                if( c == '\\' )
                {
                    c = idStr::UTF8Char( buffer, i );
                    if( c == 'n' )
                    {
                        c = '\n';
                    }
                    else if( c == 't' )
                    {
                        c = '\t';
                    }
                    else if( c == '\"' )
                    {
                        c = '\"';
                    }
                    else if( c == '\\' )
                    {
                        c = '\\';
                    }
                    else
                    {
                        idLib::Warning( "Unknown escape sequence %x at line %d", c, line );
                    }
                }
                tempVal.AppendUTF8Char( c );
            }
            if( valEnd < valStart )
            {
                idLib::Error( "%s File ended while reading value at line %d", name, line );
            }
#if 0
            if( lang_maskLocalizedStrings.GetBool() && tempVal.Length() > 0 && tempKey.Find( "#font_" ) == -1 )
            {
                int len = tempVal.Length();
                if( len > 0 )
                {
                    tempVal.Fill( 'W', len - 1 );
                }
                else
                {
                    tempVal.Empty();
                }
                tempVal.Append( 'X' );
            }
#endif
            idLangKeyValue kv;
            kv.key = tempKey;
			if(kv.key.Cmpn("#font_", 6) != 0) {
                kv.value = tempVal;
                assert(kv.key.Cmpn(STRTABLE_ID, STRTABLE_ID_LENGTH) == 0);
                hash.Add(GetHashKey(kv.key), args.Append(kv));
                numStrings++;
			}
        }
    }

    idLib::common->Printf( "%i strings read\n", numStrings );

    // get rid of any waste due to geometric list growth
    //mem.PushHeap();
#if 0
    args.Condense();
#endif
    //mem.PopHeap();

    return true;
}
#endif
