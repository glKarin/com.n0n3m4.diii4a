// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __LIB_PTRPOLICIES_H__
#define __LIB_PTRPOLICIES_H__

/*
============
sdNULLPtrCheck
============
*/
template< class T >
class sdNULLPtrCheck {
public:
	typedef T* Pointer;
	static void Check( const Pointer p ) {
		assert( p != NULL && "NULL pointer dereferenced!" );
	}
};

/*
============
sdNoPtrCheck
============
*/
template< class T >
class sdNoPtrCheck {
public:
	typedef T* Pointer;
	static void Check( const Pointer p ) {}
};

/*
============
sdDefaultCleanupPolicy
standard cleanup via delete
============
*/
template< class T >
class sdDefaultCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
		delete p;
	}
};

/*
============
sdWeakCleanupPolicy
do nothing
============
*/
template< class T >
class sdWeakCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
	}
};

/*
============
sdArrayCleanupPolicy
============
*/
template< class T >
class sdArrayCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
		delete [] p;
	}
};

/*
============
sdAlignedMemCleanupPolicy
============
*/
template< class T >
class sdAlignedMemCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
		Mem_FreeAligned( p );
	}
};

#endif // ! __LIB_PTRPOLICIES_H__
