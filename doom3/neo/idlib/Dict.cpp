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

#ifdef _SPLASHDAMAGE
#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#endif

idStrPool*		idDict::globalKeys		= NULL;
idStrPool*		idDict::globalValues	= NULL;
#else
idStrPool		idDict::globalKeys;
idStrPool		idDict::globalValues;
#endif

/*
================
idDict::operator=

  clear existing key/value pairs and copy all key/value pairs from other
================
*/
idDict &idDict::operator=(const idDict &other)
{
	int i;

	// check for assignment to self
	if (this == &other) {
		return *this;
	}

	Clear();

	args = other.args;
	argHash = other.argHash;

	for (i = 0; i < args.Num(); i++) {
#ifdef _SPLASHDAMAGE
        args[ i ].key	= globalKeys->CopyString( args[i].key );
        args[ i ].value	= globalValues->CopyString( args[i].value );
#else
		args[i].key = globalKeys.CopyString(args[i].key);
		args[i].value = globalValues.CopyString(args[i].value);
#endif
	}

	return *this;
}

/*
================
idDict::Copy

  copy all key value pairs without removing existing key/value pairs not present in the other dict
================
*/
void idDict::Copy(const idDict &other)
{
	int i, n, *found;
	idKeyValue kv;

	// check for assignment to self
	if (this == &other) {
		return;
	}

	n = other.args.Num();

	if (args.Num()) {
		found = (int *) _alloca16(other.args.Num() * sizeof(int));

		for (i = 0; i < n; i++) {
			found[i] = FindKeyIndex(other.args[i].GetKey());
		}
	} else {
		found = NULL;
	}

	for (i = 0; i < n; i++) {
		if (found && found[i] != -1) {
			// first set the new value and then free the old value to allow proper self copying
			const idPoolStr *oldValue = args[found[i]].value;
#ifdef _SPLASHDAMAGE
            args[found[i]].value = globalValues->CopyString( other.args[i].value );
            globalValues->FreeString( oldValue );
#else
			args[found[i]].value = globalValues.CopyString(other.args[i].value);
			globalValues.FreeString(oldValue);
#endif
		} else {
#ifdef _SPLASHDAMAGE
            kv.key = globalKeys->CopyString( other.args[i].key );
            kv.value = globalValues->CopyString( other.args[i].value );
#else
			kv.key = globalKeys.CopyString(other.args[i].key);
			kv.value = globalValues.CopyString(other.args[i].value);
#endif
			argHash.Add(argHash.GenerateKey(kv.GetKey(), false), args.Append(kv));
		}
	}
}

/*
================
idDict::TransferKeyValues

  clear existing key/value pairs and transfer key/value pairs from other
================
*/
void idDict::TransferKeyValues(idDict &other)
{
	int i, n;

	if (this == &other) {
		return;
	}

#if !defined(_SPLASHDAMAGE)
	if (other.args.Num() && other.args[0].key->GetPool() != &globalKeys) {
		common->FatalError("idDict::TransferKeyValues: can't transfer values across a DLL boundary");
		return;
	}
#endif

	Clear();

	n = other.args.Num();
	args.SetNum(n);

	for (i = 0; i < n; i++) {
		args[i].key = other.args[i].key;
		args[i].value = other.args[i].value;
	}

	argHash = other.argHash;

	other.args.Clear();
	other.argHash.Free();
}

/*
================
idDict::Parse
================
*/
bool idDict::Parse(idParser &parser)
{
	idToken	token;
	idToken	token2;
	bool	errors;

	errors = false;

#ifdef _SPLASHDAMAGE
    if ( !parser.ExpectTokenString( "{" ) ) {
        return false;
    }
#else
	parser.ExpectTokenString("{");
#endif
	parser.ReadToken(&token);

	while ((token.type != TT_PUNCTUATION) || (token != "}")) {
		if (token.type != TT_STRING) {
			parser.Error("Expected quoted string, but found '%s'", token.c_str());
#ifdef _SPLASHDAMAGE
            break;
#endif
		}

		if (!parser.ReadToken(&token2)) {
			parser.Error("Unexpected end of file");
#ifdef _SPLASHDAMAGE
            break;
#endif
		}

		if (FindKey(token)) {
			parser.Warning("'%s' already defined", token.c_str());
			errors = true;
		}

		Set(token, token2);

		if (!parser.ReadToken(&token)) {
			parser.Error("Unexpected end of file");
#ifdef _SPLASHDAMAGE
            break;
#endif
		}
	}

	return !errors;
}

/*
================
idDict::SetDefaults
================
*/
void idDict::SetDefaults(const idDict *dict)
{
	int i, n;
	const idKeyValue *kv, *def;
	idKeyValue newkv;

	n = dict->args.Num();

	for (i = 0; i < n; i++) {
		def = &dict->args[i];
		kv = FindKey(def->GetKey());

		if (!kv) {
#ifdef _SPLASHDAMAGE
            newkv.key = globalKeys->CopyString( def->key );
            newkv.value = globalValues->CopyString( def->value );
#else
			newkv.key = globalKeys.CopyString(def->key);
			newkv.value = globalValues.CopyString(def->value);
#endif
			argHash.Add(argHash.GenerateKey(newkv.GetKey(), false), args.Append(newkv));
		}
	}
}

/*
================
idDict::Clear
================
*/
void idDict::Clear(void)
{
	int i;

	for (i = 0; i < args.Num(); i++) {
#ifdef _SPLASHDAMAGE
        globalKeys->FreeString( args[i].key );
        globalValues->FreeString( args[i].value );
#else
		globalKeys.FreeString(args[i].key);
		globalValues.FreeString(args[i].value);
#endif
	}

	args.Clear();
	argHash.Free();
}

/*
================
idDict::Print
================
*/
void idDict::Print() const
{
	int i;
	int n;

	n = args.Num();

	for (i = 0; i < n; i++) {
		idLib::common->Printf("%s = %s\n", args[i].GetKey().c_str(), args[i].GetValue().c_str());
	}
}

int KeyCompare(const idKeyValue *a, const idKeyValue *b)
{
	return idStr::Cmp(a->GetKey(), b->GetKey());
}

/*
================
idDict::Checksum
================
*/
int	idDict::Checksum(void) const
{
	unsigned int ret;
	int i, n;

	idList<idKeyValue> sorted = args;
	sorted.Sort(KeyCompare);
	n = sorted.Num();
	CRC32_InitChecksum(ret);

	for (i = 0; i < n; i++) {
		CRC32_UpdateChecksum(ret, sorted[i].GetKey().c_str(), sorted[i].GetKey().Length());
		CRC32_UpdateChecksum(ret, sorted[i].GetValue().c_str(), sorted[i].GetValue().Length());
	}

	CRC32_FinishChecksum(ret);
	return ret;
}

/*
================
idDict::Allocated
================
*/
size_t idDict::Allocated(void) const
{
	int		i;
	size_t	size;

	size = args.Allocated() + argHash.Allocated();

	for (i = 0; i < args.Num(); i++) {
		size += args[i].Size();
	}

	return size;
}

/*
================
idDict::Set
================
*/
#ifdef _SPLASHDAMAGE
bool idDict::Set( const char *key, const char *value )
{
    int i;
    idKeyValue kv;

    assert( key );

    /*	if ( key == NULL || key[0] == '\0' ) {
    		return;
    	}*/

    i = FindKeyIndex( key );
    if ( i != -1 ) {
        // allocstring is more expensive than this simple check
        if( !args[i].value->Icmp( value ) ) {
            return false;
        }

        // first set the new value and then free the old value to allow proper self copying
        const idPoolStr *oldValue = args[i].value;
        args[i].value = globalValues->AllocString( value );
        globalValues->FreeString( oldValue );
    } else {
        kv.key = globalKeys->AllocString( key );
        kv.value = globalValues->AllocString( value );
        argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), args.Append( kv ) );
    }

    return true;
}
#else
void idDict::Set(const char *key, const char *value)
{
	int i;
	idKeyValue kv;

	if (key == NULL || key[0] == '\0') {
		return;
	}

	i = FindKeyIndex(key);

	if (i != -1) {
		// first set the new value and then free the old value to allow proper self copying
		const idPoolStr *oldValue = args[i].value;
		args[i].value = globalValues.AllocString(value);
		globalValues.FreeString(oldValue);
	} else {
		kv.key = globalKeys.AllocString(key);
		kv.value = globalValues.AllocString(value);
		argHash.Add(argHash.GenerateKey(kv.GetKey(), false), args.Append(kv));
	}
}
#endif

/*
================
idDict::GetFloat
================
*/
bool idDict::GetFloat(const char *key, const char *defaultString, float &out) const
{
	const char	*s;
	bool		found;

	found = GetString(key, defaultString, &s);
	out = atof(s);
	return found;
}

/*
================
idDict::GetInt
================
*/
bool idDict::GetInt(const char *key, const char *defaultString, int &out) const
{
	const char	*s;
	bool		found;

	found = GetString(key, defaultString, &s);
	out = atoi(s);
	return found;
}

/*
================
idDict::GetBool
================
*/
bool idDict::GetBool(const char *key, const char *defaultString, bool &out) const
{
	const char	*s;
	bool		found;

	found = GetString(key, defaultString, &s);
	out = (atoi(s) != 0);
	return found;
}

/*
================
idDict::GetAngles
================
*/
bool idDict::GetAngles(const char *key, const char *defaultString, idAngles &out) const
{
	bool		found;
	const char	*s;

	if (!defaultString) {
		defaultString = "0 0 0";
	}

	found = GetString(key, defaultString, &s);
	out.Zero();
	sscanf(s, "%f %f %f", &out.pitch, &out.yaw, &out.roll);
	return found;
}

/*
================
idDict::GetVector
================
*/
bool idDict::GetVector(const char *key, const char *defaultString, idVec3 &out) const
{
	bool		found;
	const char	*s;

	if (!defaultString) {
		defaultString = "0 0 0";
	}

	found = GetString(key, defaultString, &s);
	out.Zero();
	sscanf(s, "%f %f %f", &out.x, &out.y, &out.z);
	return found;
}

/*
================
idDict::GetVec2
================
*/
bool idDict::GetVec2(const char *key, const char *defaultString, idVec2 &out) const
{
	bool		found;
	const char	*s;

	if (!defaultString) {
		defaultString = "0 0";
	}

	found = GetString(key, defaultString, &s);
	out.Zero();
	sscanf(s, "%f %f", &out.x, &out.y);
	return found;
}

/*
================
idDict::GetVec4
================
*/
bool idDict::GetVec4(const char *key, const char *defaultString, idVec4 &out) const
{
	bool		found;
	const char	*s;

	if (!defaultString) {
		defaultString = "0 0 0 0";
	}

	found = GetString(key, defaultString, &s);
	out.Zero();
	sscanf(s, "%f %f %f %f", &out.x, &out.y, &out.z, &out.w);
	return found;
}

/*
================
idDict::GetMatrix
================
*/
bool idDict::GetMatrix(const char *key, const char *defaultString, idMat3 &out) const
{
	const char	*s;
	bool		found;

	if (!defaultString) {
		defaultString = "1 0 0 0 1 0 0 0 1";
	}

	found = GetString(key, defaultString, &s);
	out.Identity();		// sccanf has a bug in it on Mac OS 9.  Sigh.
	sscanf(s, "%f %f %f %f %f %f %f %f %f", &out[0].x, &out[0].y, &out[0].z, &out[1].x, &out[1].y, &out[1].z, &out[2].x, &out[2].y, &out[2].z);
	return found;
}

/*
================
WriteString
================
*/
static void WriteString(const char *s, idFile *f)
{
#ifdef _SPLASHDAMAGE
    int	len = idStr::Length( s );
#else
	int	len = strlen(s);
#endif

	if (len >= MAX_STRING_CHARS-1) {
		idLib::common->Error("idDict::WriteToFileHandle: bad string");
	}

#ifdef _SPLASHDAMAGE
    f->Write( s, idStr::Length( s ) + 1 );
#else
	f->Write(s, strlen(s) + 1);
#endif
}

/*
================
idDict::FindKey
================
*/
const idKeyValue *idDict::FindKey(const char *key) const
{
	int i, hash;

	if (key == NULL || key[0] == '\0') {
		idLib::common->DWarning("idDict::FindKey: empty key");
		return NULL;
	}

	hash = argHash.GenerateKey(key, false);

#ifdef _SPLASHDAMAGE
    for ( i = argHash.GetFirst( hash ); i != idHashIndex::NULL_INDEX; i = argHash.GetNext( i ) )
#else
	for (i = argHash.First(hash); i != -1; i = argHash.Next(i)) 
#endif
	{
		if (args[i].GetKey().Icmp(key) == 0) {
			return &args[i];
		}
	}

	return NULL;
}

/*
================
idDict::FindKeyIndex
================
*/
int idDict::FindKeyIndex(const char *key) const
{
#if !defined(_SPLASHDAMAGE)

	if (key == NULL || key[0] == '\0') {
		idLib::common->DWarning("idDict::FindKeyIndex: empty key");
		return -1;
	}
#endif

	int hash = argHash.GenerateKey(key, false);

#ifdef _SPLASHDAMAGE
    for ( int i = argHash.GetFirst( hash ); i != idHashIndex::NULL_INDEX; i = argHash.GetNext( i ) )
#else
	for (int i = argHash.First(hash); i != -1; i = argHash.Next(i)) 
#endif
	{
		if (args[i].GetKey().Icmp(key) == 0) {
			return i;
		}
	}

	return -1;
}

/*
================
idDict::Delete
================
*/
void idDict::Delete(const char *key)
{
	int hash, i;

	hash = argHash.GenerateKey(key, false);

#ifdef _SPLASHDAMAGE
    for ( i = argHash.GetFirst( hash ); i != -1; i = argHash.GetNext( i ) ) {
        if ( args[i].GetKey().Icmp( key ) == 0 ) {
            globalKeys->FreeString( args[i].key );
            globalValues->FreeString( args[i].value );
            args.RemoveIndex( i );
            argHash.RemoveIndex( hash, i );
            break;
        }
    }
#else
	for (i = argHash.First(hash); i != -1; i = argHash.Next(i)) {
		if (args[i].GetKey().Icmp(key) == 0) {
			globalKeys.FreeString(args[i].key);
			globalValues.FreeString(args[i].value);
			args.RemoveIndex(i);
			argHash.RemoveIndex(hash, i);
			break;
		}
	}
#endif

#if 0

	// make sure all keys can still be found in the hash index
	for (i = 0; i < args.Num(); i++) {
		assert(FindKey(args[i].GetKey()) != NULL);
	}

#endif
}

/*
================
idDict::MatchPrefix
================
*/
const idKeyValue *idDict::MatchPrefix(const char *prefix, const idKeyValue *lastMatch) const
{
	int	i;
	int len;
	int start;

	assert(prefix);
#ifdef _SPLASHDAMAGE
    len = idStr::Length( prefix );
#else
	len = strlen(prefix);
#endif

	start = -1;

	if (lastMatch) {
		start = args.FindIndex(*lastMatch);
		assert(start >= 0);

		if (start < 1) {
			start = 0;
		}
	}

	for (i = start + 1; i < args.Num(); i++) {
		if (!args[i].GetKey().Icmpn(prefix, len)) {
			return &args[i];
		}
	}

	return NULL;
}

/*
================
idDict::RandomPrefix
================
*/
const char *idDict::RandomPrefix(const char *prefix, idRandom &random) const
{
	int count;
	const int MAX_RANDOM_KEYS = 2048;
	const char *list[MAX_RANDOM_KEYS];
	const idKeyValue *kv;

	list[0] = "";

	for (count = 0, kv = MatchPrefix(prefix); kv && count < MAX_RANDOM_KEYS; kv = MatchPrefix(prefix, kv)) {
		list[count++] = kv->GetValue().c_str();
	}

	return list[random.RandomInt(count)];
}

/*
================
idDict::WriteToFileHandle
================
*/
void idDict::WriteToFileHandle(idFile *f) const
{
	int c = LittleLong(args.Num());
	f->Write(&c, sizeof(c));

	for (int i = 0; i < args.Num(); i++) {	// don't loop on the swapped count use the original
		WriteString(args[i].GetKey().c_str(), f);
		WriteString(args[i].GetValue().c_str(), f);
	}
}

/*
================
ReadString
================
*/
static idStr ReadString(idFile *f)
{
	char	str[MAX_STRING_CHARS];
	int		len;

	for (len = 0; len < MAX_STRING_CHARS; len++) {
		f->Read((void *)&str[len], 1);

		if (str[len] == 0) {
			break;
		}
	}

	if (len == MAX_STRING_CHARS) {
		idLib::common->Error("idDict::ReadFromFileHandle: bad string");
	}

	return idStr(str);
}

/*
================
idDict::ReadFromFileHandle
================
*/
void idDict::ReadFromFileHandle(idFile *f)
{
	int c;
	idStr key, val;

	Clear();

	f->Read(&c, sizeof(c));
	c = LittleLong(c);

	for (int i = 0; i < c; i++) {
		key = ReadString(f);
		val = ReadString(f);
		Set(key, val);
	}
}

/*
================
idDict::Init
================
*/
void idDict::Init(void)
{
#ifdef _SPLASHDAMAGE
    if ( !globalKeys ) {
        globalKeys = new idStrPool( 1024 );
        globalKeys->SetCaseSensitive( false );
    }

    if ( !globalValues ) {
        globalValues = new idStrPool( 1024 );
        globalValues->SetCaseSensitive( true );
    }
#else
	globalKeys.SetCaseSensitive(false);
	globalValues.SetCaseSensitive(true);
#endif
}

/*
================
idDict::Shutdown
================
*/
void idDict::Shutdown(void)
{
#ifdef _SPLASHDAMAGE
    if ( globalKeys ) {
        globalKeys->Clear();
        delete globalKeys;
        globalKeys = NULL;
    }

    if ( globalValues ) {
        globalValues->Clear();
        delete globalValues;
        globalValues = NULL;
    }
#else
	globalKeys.Clear();
	globalValues.Clear();
#endif
}

/*
================
idDict::ShowMemoryUsage_f
================
*/
void idDict::ShowMemoryUsage_f(const idCmdArgs &args)
{
#ifdef _SPLASHDAMAGE
    idLib::common->Printf( "%5zd KB in %d keys\n", globalKeys->Size() >> 10, globalKeys->Num() );
    idLib::common->Printf( "%5zd KB in %d values\n", globalValues->Size() >> 10, globalValues->Num() );
#else
	idLib::common->Printf("%5zd KB in %d keys\n", globalKeys.Size() >> 10, globalKeys.Num());
	idLib::common->Printf("%5zd KB in %d values\n", globalValues.Size() >> 10, globalValues.Num());
#endif
}

/*
================
idDictStringSortCmp
================
*/
// NOTE: the const wonkyness is required to make msvc happy
template<>
ID_INLINE int idListSortCompare(const idPoolStr *const *a, const idPoolStr *const *b)
{
	return (*a)->Icmp(**b);
}

/*
================
idDict::ListKeys_f
================
*/
void idDict::ListKeys_f(const idCmdArgs &args)
{
	int i;
	idList<const idPoolStr *> keyStrings;

#ifdef _SPLASHDAMAGE
    for ( i = 0; i < globalKeys->Num(); i++ ) {
        keyStrings.Append( ( *globalKeys )[ i ] );
    }
#else
	for (i = 0; i < globalKeys.Num(); i++) {
		keyStrings.Append(globalKeys[i]);
	}
#endif

	keyStrings.Sort();

	for (i = 0; i < keyStrings.Num(); i++) {
		idLib::common->Printf("%s\n", keyStrings[i]->c_str());
	}

	idLib::common->Printf("%5d keys\n", keyStrings.Num());
}

/*
================
idDict::ListValues_f
================
*/
void idDict::ListValues_f(const idCmdArgs &args)
{
	int i;
	idList<const idPoolStr *> valueStrings;

#ifdef _SPLASHDAMAGE
    for ( i = 0; i < globalValues->Num(); i++ ) {
        valueStrings.Append( ( *globalValues )[ i ] );
    }
#else
	for (i = 0; i < globalValues.Num(); i++) {
		valueStrings.Append(globalValues[i]);
	}
#endif

	valueStrings.Sort();

	for (i = 0; i < valueStrings.Num(); i++) {
		idLib::common->Printf("%s\n", valueStrings[i]->c_str());
	}

	idLib::common->Printf("%5d values\n", valueStrings.Num());
}

#ifdef _RAVEN
/*
================
idDict::RandomPrefix
================
*/
// RAVEN BEGIN
// abahr: added default value param
const char *idDict::RandomPrefix( const char *prefix, idRandom &random, const char* defaultValue ) const {
	int count;
	const int MAX_RANDOM_KEYS = 2048;
	const char *list[MAX_RANDOM_KEYS];
	const idKeyValue *kv;

// RAVEN BEGIN
// abahr: added defaultValue param
	list[0] = defaultValue;
// RAVEN END
	for ( count = 0, kv = MatchPrefix( prefix ); kv && count < MAX_RANDOM_KEYS; kv = MatchPrefix( prefix, kv ) ) {
		list[count++] = kv->GetValue().c_str();
	}
	return list[random.RandomInt( count )];
}
#endif

#ifdef _SPLASHDAMAGE

/*
================
idDict::Parse
================
*/
bool idDict::Parse( idLexer &parser )
{
    idToken	token;
    idToken	token2;
    bool	errors;

    errors = false;

    if ( !parser.ExpectTokenString( "{" ) ) {
        return false;
    }

    parser.ReadToken( &token );
    while( ( token.type != TT_PUNCTUATION ) || ( token != "}" ) ) {
        if ( token.type != TT_STRING ) {
            parser.Error( "Expected quoted string, but found '%s'", token.c_str() );
            break;
        }

        if ( !parser.ReadToken( &token2 ) ) {
            parser.Error( "Unexpected end of file" );
            break;
        }

        if ( FindKey( token ) ) {
            parser.Warning( "'%s' already defined", token.c_str() );
            errors = true;
        }
        Set( token, token2 );

        if ( !parser.ReadToken( &token ) ) {
            parser.Error( "Unexpected end of file" );
            break;
        }
    }

    return !errors;
}

/*
================
idDict::Set
================
*/
bool idDict::Set( int index, const char *value )
{
    assert( index >= 0 && index < args.Num() );

    // allocstring is more expensive than this simple check
    if( !args[ index ].value->Icmp( value ) ) {
        return false;
    }

    // first set the new value and then free the old value to allow proper self copying
    const idPoolStr *oldValue = args[index].value;
    args[index].value = globalValues->AllocString( value );
    globalValues->FreeString( oldValue );

    return true;
}

/*
================
idDict::GenerateKey
================
*/
int idDict::GenerateKey( const char *key )
{
    int hash = argHash.GenerateKey( key, false );
    for ( int i = argHash.GetFirst( hash ); i != -1; i = argHash.GetNext( i ) ) {
        if ( args[i].GetKey().Icmp( key ) == 0 ) {
            return i;
        }
    }

    int keyIndex;

    idKeyValue kv;
    kv.key = globalKeys->AllocString( key );
    kv.value = globalValues->AllocString( "" );
    keyIndex = args.Append( kv );
    argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), keyIndex );

    return keyIndex;
}

/*
============
idDict::Write
write the key value pairs to a file
============
*/
bool idDict::WriteIndented( idFile* file, bool indentFirst ) const
{
    sdTextUtil::GetInstance().Write( file, "{\n", indentFirst );

    sdTextUtil::GetInstance().Indent( file );
    for( int i = 0; i < GetNumKeyVals(); i++ ) {
        const idKeyValue* kv = GetKeyVal( i );
        sdTextUtil::GetInstance().Write( file, va( "\"%s\"\t\"%s\"\n", kv->GetKey().c_str(), kv->GetValue().c_str() ));
    }
    sdTextUtil::GetInstance().Unindent( file );

    sdTextUtil::GetInstance().Write( file, "}\n" );
    return true;
}

/*
============
idDict::SetGlobalPools
============
*/
void idDict::SetGlobalPools( idStrPool* _globalKeys, idStrPool* _globalValues )
{
    if( globalValues || globalKeys ) {
        return;
    }
    /*
    	assert( globalKeys == NULL );
    	assert( globalValues == NULL );
    */
    globalKeys		= _globalKeys;
    globalValues	= _globalValues;
}

/*
============
idDict::GetGlobalPools
============
*/
void idDict::GetGlobalPools( idStrPool*& _globalKeys, idStrPool*& _globalValues )
{
    _globalKeys		= globalKeys;
    _globalValues	= globalValues;
}
#endif
