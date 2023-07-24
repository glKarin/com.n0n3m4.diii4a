// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __IDLIB_ATOMIC_H__
#define __IDLIB_ATOMIC_H__

class sdAtomic {
public:
	static bool	CompareAndSwap( volatile int* dest, int comperand, int exchange );	// atomic compare and swap, is a memory barrier
};

ID_INLINE bool sdAtomic::CompareAndSwap( volatile int* dest, int comperand, int exchange ) {
#if defined( _XENON ) || defined( _WIN32 )
	return ( comperand == ::InterlockedCompareExchange( dest, exchange, comperand ) );
#elif defined( __linux__ )
	// CHECKME: untested
	int old;

	__asm__ __volatile__ (
		"lock\n"										\
		"cmpxchgl %2, %1\n"								\
		: "a" (old), "=m" (*dest),						\
		: "r" (exchange), "m" (*dest), "0" (comperand)	\
		: "memory" );

	return old == comperand;
#elif defined( MACOS_X )
	// can likely use the linux version above for osx-x86
	#error TODO: implement sdAtomic::CompareAndSwap
#endif
}

#endif /* !__IDLIB_ATOMIC_H__ */
