/*
** tmemory.h
** Smart pointers.
**
**---------------------------------------------------------------------------
** Copyright 2015 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#ifndef __TMEMORY_H__
#define __TMEMORY_H__

#include <stddef.h>
#ifndef _WIN32
#include <stdint.h>
#endif

/* Provides default delete implementation for smart pointers. Also extracts
 * the type T from T or T[] since otherwise we would need a separate template
 * for that purpose alone.
 */
template<class T> struct TDefaultDelete
{
	typedef T Type;
	explicit TDefaultDelete(T * obj) { delete obj; }

	// Determine if another TDefaultDelete is compatible with this one. If not
	// the template should fail to instantiate.
	// Note: This is only needed in custom deleters if you need downcasting
	// support.
	template<class D>
	static inline void CheckConversion() { static_cast<Ptr>((typename D::Ptr)0); }

private:
	template<class> friend struct TDefaultDelete;
	typedef T *Ptr;
};
template<class T> struct TDefaultDelete<T[]>
{
	typedef T Type;
	explicit TDefaultDelete(T * obj) { delete[] obj; }

	template<class D>
	static inline void CheckConversion()
	{
		// Array types can't be cast, so this will allow us to check if both
		// types are arrays.
		(void)sizeof(Array);
		(void)sizeof(typename D::Array);
		static_cast<Type*>((typename D::Type*)0);
	}

private:
	template<class> friend struct TDefaultDelete;
	typedef T Array[1];
};

/* Allows C cleanup functions to be used as a deleter.
 */
template<class T, void (*Func)(T*)> struct TFuncDeleter
{
	explicit TFuncDeleter(T * obj) { Func(obj); }

	template<class D>
	static inline void CheckConversion() { static_cast<Ptr>((typename D::Ptr)0); }

private:
	template<class A, void(*)(A*)> friend struct TFuncDeleter;
	typedef T *Ptr;
};

template<class T> struct TMoveInsert;

// TUniquePtr ---------------------------------------------------------------

/* Manages a pointer which should be deleted when the object goes out of scope.
 * Note that all operations are not necessarily safe when used with array
 * types, and there are ways to force invalid copies.
 *
 * Under normal conditions, TUniquePtr objects can not be copied, but rather
 * must be Released. However, TMoveInsert is allowed to break const correctness
 * in order to emulate move semantics within TArray and TMap. In these cases
 * const TUniquePtr& will behave as if it was an rvalue reference. This is
 * unfortunately the best that can be done without C++11 features while still
 * allowing the use within containers. This does mean that you must be aware
 * that inserting or pushing a unique pointer into a container will always
 * strip the pointer from the object.
 */
template<class Type, class Deleter=TDefaultDelete<Type> >
class TUniquePtr
{
	typedef TUniquePtr<Type, Deleter> Self;
	typedef typename TDefaultDelete<Type>::Type T;
	friend struct TMoveInsert<Self>;

	// Mutable so that TMoveInsert can work on const objects.
	mutable T *p;

	// Disable any means of copying. Since we implicitly convert to a raw
	// pointer, any function that takes another object of this type needs to
	// have a const ref version disabled here.
	explicit TUniquePtr(const Self&);
	Self &operator=(const Self&);
	void Reset(const Self&);
	void Swap(const Self&);
	template<class T2, class D2> explicit TUniquePtr(const TUniquePtr<T2,D2>&);
	template<class T2, class D2> Self &operator=(const TUniquePtr<T2,D2>&);
	template<class T2, class D2> void Reset(const TUniquePtr<T2,D2>&);
	template<class T2, class D2> void Swap(const TUniquePtr<T2,D2>&);
	operator void*() const;
public:
	TUniquePtr(T *p=NULL) : p(p) {}
	~TUniquePtr() { (void)Deleter(p); }

	T *Release()
	{
		T *ret = p;
		p = NULL;
		return ret;
	}

	void Reset(T *newptr=NULL)
	{
		if(newptr == p)
			return;

		T * const oldptr = p;
		p = newptr;
		(void)Deleter(oldptr);
	}

	inline void Reset(Self &other) { Reset(other.Release()); }
	// This version allows polymorphic pointers to work, provided that their
	// deleters are compatible.
	template<class T2, class D2>
	inline void Reset(TUniquePtr<T2,D2> &other)
	{
		Deleter::template CheckConversion<D2>();
		Reset(other.Release());
	}

	void Swap(Self &other)
	{
		T *oldptr = p;
		p = other.p;
		other.p = oldptr;
	}

	inline T *Get() const { return p; }
	inline T &operator*() const { return *p; }
	inline T *operator->() const { return p; }

	inline Self &operator=(T *other) { Reset(other); return *this; }

	inline operator bool() const { return p != NULL; }
	inline operator T*() const { return p; }
};

#define TSmartOper(type, op) \
	template<class T1, class D1, class T2, class D2> \
	inline bool operator op (const type<T1, D1> &p1, const type<T2, D2> &p2) \
	{ return p1.Get() op p2.Get(); } \
	template<class T1, class D, class T2> \
	inline bool operator op (const type<T1, D> &p1, T2 *p2) \
	{ return p1.Get() op p2; } \
	template<class T1, class T2, class D> \
	inline bool operator op (T2 *p1, const type<T1, D> &p2) \
	{ return p1 op p2.Get(); }
TSmartOper(TUniquePtr,==)
TSmartOper(TUniquePtr,!=)
TSmartOper(TUniquePtr,<=)
TSmartOper(TUniquePtr,>=)
TSmartOper(TUniquePtr,<)
TSmartOper(TUniquePtr,>)

// For comparisons with NULL.
template<class T, class D>
inline bool operator==(const TUniquePtr<T, D> &p1, intptr_t p2) \
{ return (intptr_t)p1.Get() == p2; }
template<class T, class D>
inline bool operator==(intptr_t p1, const TUniquePtr<T, D> &p2) \
{ return p1 == (intptr_t)p2.Get(); }
template<class T, class D>
inline bool operator!=(const TUniquePtr<T, D> &p1, intptr_t p2) \
{ return (intptr_t)p1.Get() == p2; }
template<class T, class D>
inline bool operator!=(intptr_t p1, const TUniquePtr<T, D> &p2) \
{ return p1 == (intptr_t)p2.Get(); }

template<class T, class D>
struct TMoveInsert<TUniquePtr<T, D> >
{
	explicit TMoveInsert(void *mem, const TUniquePtr<T, D> &other)
	{
		::new (mem) TUniquePtr<T, D>(other.p);
		other.p = NULL;
	}
};

// TSharedPtr ---------------------------------------------------------------

template<class Type> class TWeakPtr;

struct TSharedPtrRef
{
	unsigned shared;
	unsigned weak;

	TSharedPtrRef() : shared(1), weak(1) {}

	template<class> struct NullRef
	{
		static TSharedPtrRef Null;
	};
};

// ODR our NULL reference count so we don't need a separate compilation unit.
template<class T>
TSharedPtrRef TSharedPtrRef::NullRef<T>::Null;

/* Manages a reference counted pointer. Object is deleted when the last
 * TSharedPtr goes out of scope.
 *
 * This implementation is not thread safe.
 */
template<class Type, class Deleter=TDefaultDelete<Type> >
class TSharedPtr
{
	typedef TSharedPtr<Type, Deleter> Self;
	typedef typename TDefaultDelete<Type>::Type T;
	template<class>
	friend class TWeakPtr;
	template<class, class>
	friend class TSharedPtr;

	T *p;
	TSharedPtrRef *r;

	void Dereference()
	{
		if(--r->shared == 0)
		{
			assert(r != &TSharedPtrRef::NullRef<void>::Null);
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5))
			// Static analysis sees the possibility of deleting a stack
			// object, but if the code is written right the reference count
			// should never drop to 0.
			if(r == &TSharedPtrRef::NullRef<void>::Null)
				__builtin_unreachable();
#endif

			(void)Deleter(p);

			// All active SharedPtrs count as 1 weak reference in order to
			// optimize the TWeakPtr implementation slightly.
			if(--r->weak == 0)
				delete r;
		}
	}
	TSharedPtrRef *Reference() const
	{
		++r->shared;
		return r;
	}

	template<class T2, class D2> void Swap(const TSharedPtr<T2,D2>&);
	operator void*() const;
public:
	TSharedPtr() : p(NULL), r(&TSharedPtrRef::NullRef<void>::Null)
	{ ++TSharedPtrRef::NullRef<void>::Null.shared; }
	TSharedPtr(T *p) : p(p), r(new TSharedPtrRef) {}
	TSharedPtr(const Self &other) : p(other.p), r(other.Reference()) {}
	template<class T2, class D2>
	TSharedPtr(const TSharedPtr<T2,D2> &other) : p(other.p), r(other.Reference())
	{ Deleter::template CheckConversion<D2>(); }
	~TSharedPtr() { Dereference(); }

	template<class TConv>
	TSharedPtr<TConv, Deleter> StaticCast()
	{
		TSharedPtr<TConv, Deleter> tmp;
		tmp.Dereference();
		tmp.p = static_cast<TConv*>(p);
		tmp.r = Reference();
		return tmp;
	}

	void Reset()
	{
		Dereference();
		p = NULL;
		r = &TSharedPtrRef::NullRef<void>::Null;
		++TSharedPtrRef::NullRef<void>::Null.shared;
	}
	void Reset(T *newptr)
	{
		Dereference();
		p = newptr;
		r = new TSharedPtrRef;
	}
	void Reset(const Self &other)
	{
		Dereference();
		p = other.p;
		r = other.Reference();
	}
	template<class T2, class D2>
	inline void Reset(const TSharedPtr<T2,D2> &other)
	{
		Deleter::template CheckConversion<D2>();
		Dereference();
		p = other.p;
		r = other.Reference();
	}

	void Swap(Self &other)
	{
		T *oldptr = p;
		TSharedPtrRef *oldref = r;
		p = other.p;
		r = other.r;
		other.p = oldptr;
		other.r = r;
	}

	inline Self &operator=(T *newptr) { Reset(newptr); return *this; }
	inline Self &operator=(const Self &other) { Reset(other); return *this; }
	template<class T2, class D2>
	inline Self &operator=(const TSharedPtr<T2,D2> &other) { Reset(other); return *this; }

	inline T *Get() const { return p; }
	inline T &operator*() const { return *p; }
	inline T *operator->() const { return p; }

	inline operator bool() const { return p != NULL; }
	inline operator T*() const { return p;  }
};

TSmartOper(TSharedPtr,==)
TSmartOper(TSharedPtr,!=)
TSmartOper(TSharedPtr,<=)
TSmartOper(TSharedPtr,>=)
TSmartOper(TSharedPtr,<)
TSmartOper(TSharedPtr,>)

template<class T, class D>
inline bool operator==(const TSharedPtr<T, D> &p1, intptr_t p2) \
{ return (intptr_t)p1.Get() == p2; }
template<class T, class D>
inline bool operator==(intptr_t p1, const TSharedPtr<T, D> &p2) \
{ return p1 == (intptr_t)p2.Get(); }
template<class T, class D>
inline bool operator!=(const TSharedPtr<T, D> &p1, intptr_t p2) \
{ return (intptr_t)p1.Get() == p2; }
template<class T, class D>
inline bool operator!=(intptr_t p1, const TSharedPtr<T, D> &p2) \
{ return p1 == (intptr_t)p2.Get(); }

// TWeakPtr -----------------------------------------------------------------

/* Manages a weak reference to a shared pointer. If the object being pointed
 * to gets deleted, then this pointer will be NULLd.
 *
 * This implementation is not thread safe.
 */
template<class Type>
class TWeakPtr
{
	typedef TWeakPtr<Type> Self;
	typedef typename TDefaultDelete<Type>::Type T;
	template<class>
	friend class TWeakPtr;
	template<class, class>
	friend class TSharedPtr;

	mutable T *p;
	TSharedPtrRef *r;

	inline void Reference() { ++r->weak; }
	void Dereference()
	{
		if(--r->weak == 0)
			delete r;
	}

	inline T *CheckedAccess() const
	{
		if(p && r->shared == 0)
			Reset();
		return p;
	}

	operator void*() const;
public:
	TWeakPtr() : p(NULL), r(&TSharedPtrRef::NullRef<void>::Null)
	{ ++TSharedPtrRef::NullRef<void>::Null.weak; }
	TWeakPtr(const Self &other) : p(other.p), r(other.r) { Reference(); }
	template<class T2>
	TWeakPtr(const TWeakPtr<T2> &other) : p(other.p), r(other.r) { Reference(); }
	template<class T2, class D2>
	TWeakPtr(const TSharedPtr<T2,D2> &shared) : p(shared.p), r(shared.r) { Reference(); }
	~TWeakPtr() { Dereference(); }


	template<class TConv>
	TWeakPtr<TConv> StaticCast()
	{
		TWeakPtr<TConv> tmp;
		tmp.Dereference();
		tmp.p = static_cast<TConv*>(p);
		tmp.r = r;
		tmp.Reference();
		return tmp;
	}

	void Reset()
	{
		Dereference();
		p = NULL;
		r = &TSharedPtrRef::NullRef<void>::Null;
		++TSharedPtrRef::NullRef<void>::Null.weak;
	}
	void Reset(const Self &other)
	{
		Dereference();
		p = other.p;
		r = other.r;
		Reference();
	}
	template<class T2>
	void Reset(const TWeakPtr<T2> &other)
	{
		Dereference();
		p = other.p;
		r = other.r;
		Reference();
	}
	template<class T2, class D2>
	void Reset(const TSharedPtr<T2, D2> &other)
	{
		Dereference();
		p = other.p;
		r = other.r;
		Reference();
	}

	void Swap(Self &other)
	{
		T *oldptr = p;
		TSharedPtrRef *oldref = r;
		p = other.p;
		r = other.r;
		other.p = oldptr;
		other.r = oldref;
	}

	inline Self &operator=(const Self &other) { Reset(other); return *this; }
	template<class T2>
	inline Self &operator=(const TWeakPtr<T2> &other) { Reset(other); return *this; }
	template<class T2, class D2>
	inline Self &operator=(const TSharedPtr<T2,D2> &other) { Reset(other); return *this; }

	inline T *Get() const { return CheckedAccess(); }
#ifdef NDEBUG
	// If any of these are used without checking for NULL, we'll get a NULL
	// dereference.  So in release mode, don't bother checking the reference.
	inline T &operator*() const { return *p; }
	inline T *operator->() const { return p; }
#else
	inline T &operator*() const { return *CheckedAccess(); }
	inline T *operator->() const { return CheckedAccess(); }
#endif

	inline operator bool() const { return CheckedAccess() != NULL; }
	inline operator T*() const { return CheckedAccess();  }
};

#undef TSmartOper
#define TSmartOper(type, op) \
	template<class T1, class T2> \
	inline bool operator op (const type<T1> &p1, const type<T2> &p2) \
	{ return p1.Get() op p2.Get(); } \
	template<class T1, class T2, class D2> \
	inline bool operator op (const type<T1> &p1, const TSharedPtr<T2, D2> &p2) \
	{ return p1.Get() op p2.Get(); } \
	template<class T1, class T2, class D2> \
	inline bool operator op (const TSharedPtr<T1, D2> &p1, const type<T1> &p2) \
	{ return p1.Get() op p2.Get(); } \
	template<class T1, class T2> \
	inline bool operator op (const type<T1> &p1, T2 *p2) \
	{ return p1.Get() op p2; } \
	template<class T1, class T2> \
	inline bool operator op (T2 *p1, const type<T1> &p2) \
	{ return p1 op p2.Get(); }
TSmartOper(TWeakPtr,==)
TSmartOper(TWeakPtr,!=)
TSmartOper(TWeakPtr,<=)
TSmartOper(TWeakPtr,>=)
TSmartOper(TWeakPtr,<)
TSmartOper(TWeakPtr,>)

template<class T>
inline bool operator==(const TWeakPtr<T> &p1, intptr_t p2)
{ return (intptr_t)p1.Get() == p2; }
template<class T>
inline bool operator==(intptr_t p1, const TWeakPtr<T> &p2)
{ return p2 == (intptr_t)p2.Get(); }
template<class T>
inline bool operator!=(const TWeakPtr<T> &p1, intptr_t p2)
{ return (intptr_t)p1.Get() == p2; }
template<class T>
inline bool operator!=(intptr_t p1, const TWeakPtr<T> &p2)
{ return p1 == (intptr_t)p2.Get(); }

#undef TSmartOper
#endif
