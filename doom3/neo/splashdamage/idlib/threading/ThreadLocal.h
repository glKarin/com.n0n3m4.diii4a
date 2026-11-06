// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __THREADLOCAL_H__
#define __THREADLOCAL_H__

#if defined( _WIN32 )
#include <windows.h>
#define ID_TLS_SET(tlsIndex, v) TlsSetValue(tlsIndex, v)
#define ID_TLS_GET(tlsIndex) TlsGetValue(tlsIndex)
#else
#include <pthread.h>
#define ID_TLS_SET(tlsIndex, v) pthread_setspecific(tlsIndex, v)
#define ID_TLS_GET(tlsIndex) pthread_getspecific(tlsIndex)
#endif

template<class T>
class sdThreadLocal {
public:
						sdThreadLocal(void);
						~sdThreadLocal(void);

	bool				Has(void) const;
	bool				Get(T &t) const;
	T					Get(const T &def) const;
	void				Set(const T &other) const;
	void				Remove(void) const;
	void				Delete(void) const;
	bool				Take(T &ret) const;
	T					operator->(void);
	const T				operator->(void) const;
	sdThreadLocal<T> &	operator=(const T &other);
	bool				operator==(const T &other) const;
	friend bool			operator==(const T &other, const sdThreadLocal<T> &self) { return self == other; }
	T					operator++(int) const;

	operator T (void) const;

private:
	void *				ToLocal(const T &val) const;
	T					FromLocal(const void *val) const;

private:
#if defined( _WIN32 )
	DWORD tlsIndex;
#else
	pthread_key_t tlsIndex;
#endif
};

template<class T>
ID_INLINE sdThreadLocal<T>::sdThreadLocal(void) {
	char _assert[sizeof(T) <= sizeof(void *) ? 1 : -1];
	(void)_assert;
#if defined( _WIN32 )
	tlsIndex = TlsAlloc();
#else
    pthread_key_create(&tlsIndex, NULL);
#endif
}

template<class T>
sdThreadLocal<T>::~sdThreadLocal(void) {
#if defined( _WIN32 )
	TlsFree(tlsIndex);
#else
    pthread_key_delete(tlsIndex);
#endif
}

template<class T>
ID_INLINE bool sdThreadLocal<T>::Has(void) const {
	return NULL != ID_TLS_GET(tlsIndex);
}

template<class T>
ID_INLINE bool sdThreadLocal<T>::Get(T &ret) const {
	const void* cur = ID_TLS_GET(tlsIndex);
	if (cur) {
		ret = FromLocal(cur);
		return true;
	}
	return false;
}

template<class T>
ID_INLINE T sdThreadLocal<T>::Get(const T &def) const {
	const void* cur = ID_TLS_GET(tlsIndex);
	if (cur) {
		return FromLocal(cur);
	}
	return def;
}

template<class T>
ID_INLINE void sdThreadLocal<T>::Set(const T &other) const {
	ID_TLS_SET(tlsIndex, ToLocal(other));
}

template<class T>
ID_INLINE void sdThreadLocal<T>::Remove(void) const {
	ID_TLS_SET(tlsIndex, NULL);
}

template<class T>
ID_INLINE void sdThreadLocal<T>::Delete(void) const {
	void* cur = ID_TLS_GET(tlsIndex);
	if (cur) {
		delete FromLocal(cur);
		Remove();
	}
}

template<class T>
ID_INLINE bool sdThreadLocal<T>::Take(T &ret) const {
	void* cur = ID_TLS_GET(tlsIndex);
	if (cur) {
		ret = FromLocal(cur);
		Remove();
		return true;
	}
	return false;
}

template<class T>
ID_INLINE void * sdThreadLocal<T>::ToLocal(const T &val) const {
	return (void *)*(uintptr_t *)&val;
}

template<class T>
ID_INLINE T sdThreadLocal<T>::FromLocal(const void *val) const {
	return *((T *)&val);
}

template<class T>
ID_INLINE sdThreadLocal<T> & sdThreadLocal<T>::operator=(const T &other) {
	Set(other);
	return *this;
}

template<class T>
ID_INLINE bool sdThreadLocal<T>::operator==(const T &other) const {
	void* cur = ID_TLS_GET(tlsIndex);
	if (!cur) {
		return false;
	}
	return FromLocal(cur) == other;
}

template<class T>
ID_INLINE bool operator==(const T &other, const sdThreadLocal<T> &self) {
	return self == other;
}

template<class T>
ID_INLINE T sdThreadLocal<T>::operator++(int) const {
	void* cur = ID_TLS_GET(tlsIndex);
	if (!cur) {
		T i = T();
		Set(i + 1);
		return i;
	}
	else {
		T i = FromLocal(cur);
		Set(i + 1);
		return i;
	}
}

template<class T>
ID_INLINE sdThreadLocal<T>::operator T(void) const {
	void* cur = ID_TLS_GET(tlsIndex);
	if (cur) {
		return FromLocal(cur);
	}
	Set(T());
	return FromLocal(cur);
}

template<class T>
ID_INLINE T sdThreadLocal<T>::operator->(void) {
	void* cur = ID_TLS_GET(tlsIndex);
	if (cur) {
		return (T)FromLocal(cur);
	}
	return NULL;
}

template<class T>
ID_INLINE const T sdThreadLocal<T>::operator->(void) const {
	void* cur = ID_TLS_GET(tlsIndex);
	if (cur) {
		return (T)FromLocal(cur);
	}
	return NULL;
}

#undef ID_TLS_SET
#undef ID_TLS_GET

#endif /* !__THREADLOCAL_H__ */
