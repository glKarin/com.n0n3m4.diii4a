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

#ifndef __DICT_H__
#define __DICT_H__

#include <cstdlib>

/*
===============================================================================

Key/value dictionary

This is a dictionary class that tracks an arbitrary number of key / value
pair combinations. It is used for map entity spawning, GUI state management,
and other things.

Keys are compared case-insensitive.

Does not allocate memory until the first key/value pair is added.

===============================================================================
*/

class idKeyValue {
	friend class idDict;

public:
	ID_FORCE_INLINE const idStr &GetKey( void ) const { return *key; }
	ID_FORCE_INLINE const idStr &GetValue( void ) const { return *value; }

	size_t				Allocated( void ) const { return key->Allocated() + value->Allocated(); }
	size_t				Size( void ) const { return sizeof( *this ) + key->Size() + value->Size(); }

	bool				operator==( const idKeyValue &kv ) const { return ( key == kv.key && value == kv.value ); }

private:
	const idPoolStr *	key;
	const idPoolStr *	value;
};

class idDict {
public:
						idDict( void );
						idDict( const idDict &other );	// allow declaration with assignment
						~idDict( void );

						// set the granularity for the index
	void				SetGranularity( int granularity );
						// set hash size
	void				SetHashSize( int hashSize );
						// clear existing key/value pairs and copy all key/value pairs from other
	idDict &			operator=( const idDict &other );
						// copy from other while leaving existing key/value pairs in place
	void				Copy( const idDict &other );
						// clear existing key/value pairs and transfer key/value pairs from other
	void				TransferKeyValues( idDict &other );
						// parse dict from parser
	bool				Parse( idParser &parser );
						// copy key/value pairs from other dict not present in this dict
	void				SetDefaults( const idDict *dict );
						// Tels: like SetDefaults(), but skip all keys starting with "skip"
	void				SetDefaults( const idDict *dict, const idStr &skip );
						// clear dict retaining memory
	void				Clear( void );
						// clear dict freeing up memory
	void				ClearFree( void );
						// print the dict
	void				Print() const;

	size_t				Allocated( void ) const;
	size_t				Size( void ) const { return sizeof( *this ) + Allocated(); }

	void				Set( const char *key, const char *value );
	void				SetFloat( const char *key, float val );
	void				SetInt( const char *key, int val );
	void				SetBool( const char *key, bool val );
	void				SetVector( const char *key, const idVec3 &val );
	void				SetVec2( const char *key, const idVec2 &val );
	void				SetVec4( const char *key, const idVec4 &val );
	void				SetAngles( const char *key, const idAngles &val );
	void				SetMatrix( const char *key, const idMat3 &val );

						// these return default values of 0.0, 0 and false
	const char *		GetString( const char *key, const char *defaultString = "" ) const;
	float				GetFloat( const char *key, const char *defaultString = "0" ) const;
	int					GetInt( const char *key, const char *defaultString = "0" ) const;
	bool				GetBool( const char *key, const char *defaultString = "0" ) const;
	idVec3				GetVector( const char *key, const char *defaultString = NULL ) const;
	idVec2				GetVec2( const char *key, const char *defaultString = NULL ) const;
	idVec4				GetVec4( const char *key, const char *defaultString = NULL ) const;
	idAngles			GetAngles( const char *key, const char *defaultString = NULL ) const;
	idMat3				GetMatrix( const char *key, const char *defaultString = NULL ) const;

	bool				GetString( const char *key, const char *defaultString, const char **out ) const;
	bool				GetString( const char *key, const char *defaultString, idStr &out ) const;
	bool				GetFloat( const char *key, const char *defaultString, float &out ) const;
	bool				GetInt( const char *key, const char *defaultString, int &out ) const;
	bool				GetBool( const char *key, const char *defaultString, bool &out ) const;
	bool				GetVector( const char *key, const char *defaultString, idVec3 &out ) const;
	bool				GetVec2( const char *key, const char *defaultString, idVec2 &out ) const;
	bool				GetVec4( const char *key, const char *defaultString, idVec4 &out ) const;
	bool				GetAngles( const char *key, const char *defaultString, idAngles &out ) const;
	bool				GetMatrix( const char *key, const char *defaultString, idMat3 &out ) const;

	int					GetNumKeyVals( void ) const;
	const idKeyValue *	GetKeyVal( int index ) const;
						// returns the key/value pair with the given key
						// returns NULL if the key/value pair does not exist
	const idKeyValue *	FindKey( const char *key ) const;
						// returns the index to the key/value pair with the given key
						// returns -1 if the key/value pair does not exist
	int					FindKeyIndex( const char *key ) const;
						// delete the key/value pair with the given key
	void				Delete( const char *key );
						// finds the next key/value pair with the given key prefix.
						// lastMatch can be used to do additional searches past the first match.
	const idKeyValue *	MatchPrefix( const char *prefix, const idKeyValue *lastMatch = NULL ) const;
						// randomly chooses one of the key/value pairs with the given key prefix and returns it's value
	const char *		RandomPrefix( const char *prefix, idRandom &random ) const;

	void				WriteToFileHandle( idFile *f ) const;
	void				ReadFromFileHandle( idFile *f );

	static void			Init( void );
	static void			Shutdown( void );

	static void			ShowMemoryUsage_f( const idCmdArgs &args );
	void				PrintMemory( void ) const;
	static void			ListKeys_f( const idCmdArgs &args );
	static void			ListValues_f( const idCmdArgs &args );

private:
	idList<idKeyValue>	args;
	idHashIndex			argHash;

	static idStrPool	globalKeys;
	static idStrPool	globalValues;
};


ID_INLINE idDict::idDict( void ) {
	args.SetGranularity( 16 );
	argHash.SetGranularity( 16 );
	argHash.ClearFree( 128, 16 );
}

ID_INLINE idDict::idDict( const idDict &other ) {
	*this = other;
}

ID_INLINE idDict::~idDict( void ) {
	Clear();
}

ID_INLINE void idDict::SetGranularity( int granularity ) {
	args.SetGranularity( granularity );
	argHash.SetGranularity( granularity );
}

ID_INLINE void idDict::SetHashSize( int hashSize ) {
	if ( args.Num() == 0 ) {
		argHash.ClearFree( hashSize, 16 );
	}
}

ID_INLINE void idDict::SetFloat( const char *key, float val ) {
	Set( key, va( "%f", val ) );
}

ID_INLINE void idDict::SetInt( const char *key, int val ) {
	Set( key, va( "%i", val ) );
}

ID_INLINE void idDict::SetBool( const char *key, bool val ) {
	Set( key, va( "%i", val ) );
}

ID_INLINE void idDict::SetVector( const char *key, const idVec3 &val ) {
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetVec4( const char *key, const idVec4 &val ) {
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetVec2( const char *key, const idVec2 &val ) {
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetAngles( const char *key, const idAngles &val ) {
	Set( key, val.ToString() );
}

ID_INLINE void idDict::SetMatrix( const char *key, const idMat3 &val ) {
	Set( key, val.ToString() );
}

ID_INLINE bool idDict::GetString( const char *key, const char *defaultString, const char **out ) const {
	const idKeyValue *kv = FindKey( key );
	if ( kv ) {
		*out = kv->GetValue();
		return true;
	}
	*out = defaultString;
	return false;
}

ID_INLINE bool idDict::GetString( const char *key, const char *defaultString, idStr &out ) const {
	const idKeyValue *kv = FindKey( key );
	if ( kv ) {
		out = kv->GetValue();
		return true;
	}
	out = defaultString;
	return false;
}

ID_INLINE const char *idDict::GetString( const char *key, const char *defaultString ) const {
	const idKeyValue *kv = FindKey( key );
	if ( kv ) {
		return kv->GetValue();
	}
	return defaultString;
}

ID_INLINE float idDict::GetFloat( const char *key, const char *defaultString ) const {
	return atof( GetString( key, defaultString ) );
}

ID_INLINE int idDict::GetInt( const char *key, const char *defaultString ) const {
	return atoi( GetString( key, defaultString ) );
}

ID_INLINE bool idDict::GetBool( const char *key, const char *defaultString ) const {
	return ( atoi( GetString( key, defaultString ) ) != 0 );
}

ID_INLINE idVec3 idDict::GetVector( const char *key, const char *defaultString ) const {
	idVec3 out;
	GetVector( key, defaultString, out );
	return out;
}

ID_INLINE idVec2 idDict::GetVec2( const char *key, const char *defaultString ) const {
	idVec2 out;
	GetVec2( key, defaultString, out );
	return out;
}

ID_INLINE idVec4 idDict::GetVec4( const char *key, const char *defaultString ) const {
	idVec4 out;
	GetVec4( key, defaultString, out );
	return out;
}

ID_INLINE idAngles idDict::GetAngles( const char *key, const char *defaultString ) const {
	idAngles out;
	GetAngles( key, defaultString, out );
	return out;
}

ID_INLINE idMat3 idDict::GetMatrix( const char *key, const char *defaultString ) const {
	idMat3 out;
	GetMatrix( key, defaultString, out );
	return out;
}

ID_INLINE int idDict::GetNumKeyVals( void ) const {
	return args.Num();
}

ID_INLINE const idKeyValue *idDict::GetKeyVal( int index ) const {
	if ( index >= 0 && index < args.Num() ) {
		return &args[ index ];
	}
	return NULL;
}

#endif /* !__DICT_H__ */
