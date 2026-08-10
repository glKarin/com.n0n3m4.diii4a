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
#elif defined(__ANDROID__) // gcc/clang
	return __sync_bool_compare_and_swap(dest, comperand, exchange);
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



template<typename T>
class sdAtomicValue
{
public:
    sdAtomicValue(void) {
        Store(T());
    }
    sdAtomicValue(T newval) {
        Store(newval);
    }

    void Store(T newval) {
#ifdef _MSC_VER
        _InterlockedExchange((long *)&value, newval);
#elif defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__)
        __sync_lock_test_and_set(&value, newval);
#elif defined(__APPLE__) || defined(MACOS_X)
#error "sdAtomic::store"
#else
		lock.Acquire();
		value = newval;
		lock.Release();
#endif
    }

    T Load(void) const {
#ifdef _MSC_VER
        return _InterlockedOr((long *)&value, 0);
#elif defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__)
        return __sync_or_and_fetch((volatile T *)&value, 0);
#elif defined(__APPLE__) || defined(MACOS_X)
#error "sdAtomic::load"
#else
		lock.Acquire();
		T tmp = value;
		lock.Release();
		return tmp;
#endif
    }

    T Exchange(T newval) {
#ifdef _MSC_VER
        return _InterlockedExchange((long *)&value, (long)newval);
#elif defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__)
        return __sync_lock_test_and_set(&value, newval);
#elif defined(__APPLE__) || defined(MACOS_X)
#error "sdAtomic::exchange"
#else
		lock.Acquire();
		T tmp = value;
		value = newval;
		lock.Release();
		return tmp;
#endif
    }

    operator T(void) const { return Load(); }
    sdAtomicValue<T> operator=(T newval) { Store(newval); return *this; }
    T operator++(void) {
        return FetchAdd(1) + 1;
    }
    T operator++(int) {
        return FetchAdd(1);
    }
    T operator--(void) {
        return FetchAdd(-1) - 1;
    }
    T operator--(int) {
        return FetchAdd(-1);
    }
    T Add(T newval) {
        return FetchAdd(newval) + newval;
    }

protected:
    T FetchAdd(T newval) {
#ifdef _MSC_VER
        return _InterlockedExchangeAdd((long *)&value, newval);
#elif defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__)
        return __sync_fetch_and_add(&value, newval);
#elif defined(__APPLE__) || defined(MACOS_X)
#error "sdAtomic::add"
#else
		lock.Acquire();
		T tmp = value;
		value += newval;
		lock.Release();
		return tmp;
#endif
    }

private:
    volatile T value;
#if !defined(_MSC_VER) \
	&& !defined(__GNUC__) && !defined(__clang__) && !defined(__MINGW32__) \
	&& !defined(__linux__) \
	&& !defined(__APPLE__) && !defined(MACOS_X)
	sdRecursiveLock lock;
#endif
};

#endif /* !__IDLIB_ATOMIC_H__ */
