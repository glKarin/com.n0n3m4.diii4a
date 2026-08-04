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

#ifndef __STRPOOL_H__
#define __STRPOOL_H__

/*
===============================================================================

	idStrPool

===============================================================================
*/

class idStrPool;

class idPoolStr : public idStr {
	friend class idStrPool;

public:
						idPoolStr() { numUsers = 0; }
						~idPoolStr() { assert( numUsers == 0 ); }

						// returns total size of allocated memory
	size_t				Allocated( void ) const { return idStr::Allocated(); }
						// returns total size of allocated memory including size of string pool type
	size_t				Size( void ) const { return sizeof( *this ) + Allocated(); }
						// returns a pointer to the pool this string was allocated from
	const idStrPool *	GetPool( void ) const { return pool; }

private:
	idStrPool *			pool;
	mutable int			numUsers;
};

class idStrPool {
public:
						idStrPool() { caseSensitive = true; }

	void				SetCaseSensitive( bool caseSensitive );

	size_t				Allocated( void ) const;
	size_t				Size( void ) const;

	//stgatilov: be sure to call Compress before using these methods!
	int					Num( void ) const { return pool.Num(); }
	const idPoolStr *	operator[]( int index ) const { return pool[index]; }

	const idPoolStr *	AllocString( const char *string );
	void				FreeString( const idPoolStr *poolStr );
	const idPoolStr *	CopyString( const idPoolStr *poolStr );
	void				ClearFree( void );
	void				Compress( void );

	void				PrintAll( const char *label );

private:
	bool				caseSensitive;
	idList<int>			freeList;	//stgatilov: list of free slot indices
	idList<idPoolStr *>	pool;		//may contain NULLs
	idHashIndex			poolHash;
};

/*
================
idStrPool::SetCaseSensitive
================
*/
ID_INLINE void idStrPool::SetCaseSensitive( bool caseSensitive ) {
	this->caseSensitive = caseSensitive;
}

#endif /* !__STRPOOL_H__ */
