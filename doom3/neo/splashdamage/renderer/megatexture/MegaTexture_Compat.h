//
// Only for compat with jmarshall23's idMegaTexture of DarklightNG
//

#ifndef _SD_MEGATEXTURE_COMPAT_H
#define _SD_MEGATEXTURE_COMPAT_H

#define qglCompressedTexSubImage2DARB qglCompressedTexSubImage2D
#define qglCompressedTexSubImage2DARB qglCompressedTexSubImage2D

#define R_SetGLSLProgramEnvParameter(shaderType, index, value) GL_Uniform4fv(SHADER_PARMS_ADDR(u_vertexParm, index), value);

#include "../sys/threading/SysLock.h"

class sdRecursiveLock {
public:
                        sdRecursiveLock();
						~sdRecursiveLock();

    bool				Acquire( bool blocking = true );
    void				Release();

#ifndef _WIN32
    lockHandle_t*		GetHandle() const { return (lockHandle_t *)&handle; }
#endif

protected:
    lockHandle_t		handle;
};


template<typename LockT>
class sdLockGuard {
public:
    sdLockGuard( LockT& lock, bool blocking = true ) : lock( lock ) {
        lock.Acquire( blocking );
    }
    ~sdLockGuard() {
        lock.Release();
    }
private:
    sdLockGuard( const sdLockGuard& rhs );
    sdLockGuard& operator=( const sdLockGuard& rhs );
    LockT& lock;
};



/*
=============
sdRecursiveLock::sdRecursiveLock
=============
*/
ID_INLINE sdRecursiveLock::sdRecursiveLock() {
    //karin: add recursive flag
    sdSysLock::Init( handle, true );
}

/*
=============
sdRecursiveLock::~sdRecursiveLock
=============
*/
ID_INLINE sdRecursiveLock::~sdRecursiveLock() {
    sdSysLock::Destroy( handle );
}

/*
=============
sdRecursiveLock::Acquire
=============
*/
ID_INLINE bool sdRecursiveLock::Acquire( bool blocking ) {
    return sdSysLock::Acquire( handle, blocking );
}

/*
=============
sdRecursiveLock::Release
=============
*/
ID_INLINE void sdRecursiveLock::Release() {
    sdSysLock::Release( handle );
}



template<typename T>
class sdAtomic
{
public:
    sdAtomic(void) {
        Store(T());
    }
    sdAtomic(T newval) {
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
    sdAtomic<T> operator=(T newval) { Store(newval); return *this; }
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

template<typename LockT>
class sdUniqueLock {
public:
    sdUniqueLock( LockT& lock ) : lock( lock ), locked(false) {
        Lock();
    }
    ~sdUniqueLock() {
        Unlock();
    }
    void Lock() {
        if (!locked.Load())
        {
            lock.Acquire( true );
            locked = true;
        }
    }
    void Unlock() {
        if (locked.Load())
        {
            lock.Release();
            locked = false;
        }
    }
private:
    sdUniqueLock( const sdUniqueLock& rhs );
    sdUniqueLock& operator=( const sdUniqueLock& rhs );
    LockT& lock;
    sdAtomic<bool> locked;
};

#endif //_SD_MEGATEXTURE_COMPAT_H
