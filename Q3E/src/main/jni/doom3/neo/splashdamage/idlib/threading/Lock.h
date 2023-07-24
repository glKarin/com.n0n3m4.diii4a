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

#ifndef _WIN32
	lockHandle_t*		GetHandle() const { return &handle; }
#endif

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


#endif /* !__LOCK_H__ */
