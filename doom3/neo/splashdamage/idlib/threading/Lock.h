// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LOCK_H__
#define __LOCK_H__

class sdLock {
public:
						sdLock();
						~sdLock();

	bool				Acquire( bool blocking = true );
	void				Release();

//#ifndef _WIN32
	lockHandle_t*		GetHandle() const { return (lockHandle_t *)&handle; }
//#endif

protected:
	lockHandle_t		handle;
};

template< bool doLock >
class sdScopedLock {
public:
};

template<>
class sdScopedLock< true > {
public:
	sdScopedLock( sdLock& lock, bool blocking = true ) : lock( lock ) {
		lock.Acquire( blocking );
	}
	~sdScopedLock() {
		lock.Release();
	}
private:
	sdScopedLock( const sdScopedLock& rhs );
	sdScopedLock& operator=( const sdScopedLock& rhs );
	sdLock& lock;
};


template<>
class sdScopedLock< false > {
public:
	sdScopedLock( sdLock& lock, bool blocking = true ) {
	}
	~sdScopedLock() {
	}
private:
	sdScopedLock( const sdScopedLock& rhs );
	sdScopedLock& operator=( const sdScopedLock& rhs );
};



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

#include "Atomic.h"

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
    sdAtomicValue<bool> locked;
};

#endif /* !__LOCK_H__ */
