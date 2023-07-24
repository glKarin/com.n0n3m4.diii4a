// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LANGDICT_H__
#define __LANGDICT_H__

/*
===============================================================================

	Simple dictionary specifically for the localized string tables.

===============================================================================
*/

class idLangKeyValue {
public:
	idStr					key;
	idWStr					value;
};

class idLangDict {
public:
							idLangDict();
							~idLangDict();

	void					Clear();
	bool					Load( const char *fileName, bool clear = true );
	void					Save( const char *fileName );

	const wchar_t*			GetString( const char *str ) const;
	
	const idLangKeyValue*	FindKeyValue( const char* str ) const;

							// adds the value and key as passed (doesn't generate a "#str_xxxxx" key or ensure the key/value pair is unique)
	void					AddKeyVal( const char* key, const wchar_t* val );

	int						GetNumKeyVals() const;
	const idLangKeyValue*	GetKeyVal( int i ) const;

private:
	idList<idLangKeyValue>	args;
	idHashIndex				hash;

	int						GetHashKey( const char *str ) const;
};

#endif /* !__LANGDICT_H__ */
