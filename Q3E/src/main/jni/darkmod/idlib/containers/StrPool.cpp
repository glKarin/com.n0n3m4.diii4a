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


/*
================
idStrPool::AllocString
================
*/
const idPoolStr *idStrPool::AllocString( const char *string ) {
	int i, hash;
	idPoolStr *poolStr;

	hash = poolHash.GenerateKey( string, caseSensitive );
	if ( caseSensitive ) {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Cmp( string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	} else {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Icmp( string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	}

	poolStr = new idPoolStr;
	*static_cast<idStr *>(poolStr) = string;
	poolStr->pool = this;
	poolStr->numUsers = 1;

	int index;
	if (freeList.Num() == 0)
		index = pool.AddGrow( poolStr );
	else {
		//stgatilov: reuse previously freed indices
		index = freeList.Pop();
		pool[index] = poolStr;
	}

	poolHash.Add( hash, index );
	return poolStr;
}

/*
================
idStrPool::FreeString
================
*/
void idStrPool::FreeString( const idPoolStr *poolStr ) {
	int i, hash;

	assert( poolStr->numUsers >= 1 );
	assert( poolStr->pool == this );

	poolStr->numUsers--;
	if ( poolStr->numUsers <= 0 ) {
		hash = poolHash.GenerateKey( poolStr->c_str(), caseSensitive );
		if ( caseSensitive ) { 
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Cmp( poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		} else {
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Icmp( poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		}
		assert( i != -1 );
		assert( pool[i] == poolStr );
		delete pool[i];
#if 0
		//original O(N) code
		poolHash.RemoveIndex( hash, i );
		pool.RemoveIndex( i );
#else
		//stgatilov: add freed slot to freelist
		poolHash.Remove( hash, i );
		pool[i] = nullptr;
		freeList.AddGrow(i);
#endif
	}
}

/*
================
idStrPool::CopyString
================
*/
const idPoolStr *idStrPool::CopyString( const idPoolStr *poolStr ) {

	assert( poolStr->numUsers >= 1 );

	if ( poolStr->pool == this ) {
		// the string is from this pool so just increase the user count
		poolStr->numUsers++;
		return poolStr;
	} else {
		// the string is from another pool so it needs to be re-allocated from this pool.
		return AllocString( poolStr->c_str() );
	}
}

/*
================
idStrPool::ClearFree
================
*/
void idStrPool::ClearFree( void ) {
	int i;

	for ( i = 0; i < pool.Num(); i++ ) if ( pool[i] )  {
		pool[i]->numUsers = 0;
		delete pool[i];
	}
	freeList.ClearFree();
	pool.ClearFree();
	poolHash.ClearFree();
}

/*
================
idStrPool::Compress
================
*/
void idStrPool::Compress( void ) {
	if (freeList.Num() == 0)
		return;	//no zombie slots

	poolHash.Clear();
	int k = 0;
	for (int i = 0; i < pool.Num(); i++) if ( pool[i] ) {
		int newIdx = k++;
		pool[newIdx] = pool[i];
		int hash = poolHash.GenerateKey( pool[newIdx]->c_str(), caseSensitive );
		poolHash.Add(hash, newIdx);
	}
	pool.SetNum(k);
	freeList.SetNum(0);
}

/*
================
idStrPool::Allocated
================
*/
size_t idStrPool::Allocated( void ) const {
	int i;
	size_t size;

	size = pool.Allocated() + poolHash.Allocated() + freeList.Allocated();
	for ( i = 0; i < pool.Num(); i++ ) if ( pool[i] ) {
		size += pool[i]->Allocated();
	}
	return size;
}

/*
================
idStrPool::Size
================
*/
size_t idStrPool::Size( void ) const {
	int i;
	size_t size;

	size = pool.Size() + poolHash.Size() + freeList.Size();
	for ( i = 0; i < pool.Num(); i++ ) if ( pool[i] ) {
		size += pool[i]->Size();
	}
	return size;
}

/*
================
idStrPool::PrintAll
================
*/
void idStrPool::PrintAll( const char *label ) {
	int i;
	idList<const idPoolStr *> valueStrings;

	for ( i = 0; i < pool.Num(); i++ ) {
		if ( pool[i] ) 
			valueStrings.AddGrow( pool[i] );
	}

	valueStrings.Sort([](const idPoolStr* const* a, const idPoolStr* const* b) -> int {
		return idStr::Icmp((*a)->c_str(), (*b)->c_str());
	});

	for ( i = 0; i < valueStrings.Num(); i++ ) {
		common->Printf( "%s\n", valueStrings[i]->c_str() );
	}

	common->Printf( "Pool \"%s\":  %5d strings\n", label, valueStrings.Num() );
}
