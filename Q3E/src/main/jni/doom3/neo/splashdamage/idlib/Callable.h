// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __CALLABLE_H__
#define __CALLABLE_H__

namespace sdFunctions {
	class sdEmptyType {};

/*
============
sdBinderMember0
============
*/
extern const char sdBinderMember0_Identifier[];
template< class RetType, class T, class Func, class T1 = sdEmptyType, class T2 = sdEmptyType, class T3 = sdEmptyType >
class sdBinderMember0 /*:
	public sdPoolAllocator< sdBinderMember0, sdBinderMember0_Identifier > */{
public:
	sdBinderMember0() : obj( NULL ) {}
	sdBinderMember0( Func func_, T* obj_ ) : obj( obj_ ) { function = func_; }

	RetType operator()() { return ((*obj).*function)(); }
	RetType operator()( T1 arg1) { return ((*obj).*function)( arg1 ); }
	RetType operator()( T1 arg1, T2 arg2 ) { return ((*obj).*function)( arg1, arg2 ); }
	RetType operator()( T1 arg1, T2 arg2, T3 arg3 ) { return ((*obj).*function)( arg1, arg2, arg3 ); }

private:
	Func function;
	T*	obj;
};

/*
============
sdBinderMember0
============
*/
template< class RetType, class T1, class T2, class T3, class T, class Func >
ID_INLINE sdBinderMember0< RetType, T, Func, T1, T2, T3 > sdBindMem0( Func func, T* obj ) { return sdBinderMember0< RetType, T, Func, T1, T2, T3 >( func, obj ); }

/*
============
sdBinderMember1
============
*/

extern const char sdBinderMember1_Identifier[];
template< class RetType, class T, class Func, class T1 = sdEmptyType, class T2 = sdEmptyType, class T3 = sdEmptyType >
class sdBinderMember1 /*:
	public sdPoolAllocator< sdBinderMember1, sdBinderMember1_Identifier > */ {
public:
	sdBinderMember1() : obj( NULL ) {}
	sdBinderMember1( Func func_, T* obj_, T1 arg1_ ) : arg1( arg1_ ), obj( obj_ ) { function = func_; }

	RetType operator()() { return ((*obj).*function)( arg1 ); }
	RetType operator()( T2 arg2 ) { return ((*obj).*function)( arg1, arg2 ); }
	RetType operator()( T2 arg2, T3 arg3 ) { return ((*obj).*function)( arg1, arg2, arg3 ); }

private:
	T1 arg1;
	Func function;
	T*	obj;
};

/*
============
sdBinderMember1
============
*/
template< class RetType, class T1, class T2, class T3, class T, class Func >
ID_INLINE sdBinderMember1< RetType, T, Func, T1, T2, T3 > sdBindMem1( Func func, T* obj, T1 arg1 ) { return sdBinderMember1< RetType, T, Func, T1, T2, T3 >( func, obj, arg1 ); }

/*
============
sdCallable
============
*/

/*
============
sdCallableBase0
============
*/
template< class Ret >
class sdCallableBase0 {
public:
	virtual ~sdCallableBase0() {}
	virtual Ret operator()() const = 0;
	virtual sdCallableBase0< Ret >* Clone() const = 0;
};

/*
============
sdCallableBase1
============
*/
template< class Ret, class T1 >
class sdCallableBase1 {
public:
	virtual ~sdCallableBase1() {}
	virtual Ret operator()( T1 arg1 ) const = 0;
	virtual sdCallableBase1< Ret, T1 >* Clone() const = 0;
};

/*
============
sdCallableBase2
============
*/
template< class Ret, class T1, class T2 >
class sdCallableBase2 {
public:
	virtual ~sdCallableBase2() {}
	virtual Ret operator()( T1 arg1, T2 arg2 ) const = 0;
	virtual sdCallableBase2< Ret, T1, T2 >* Clone() const = 0;
};

/*
============
sdCallableBase3
============
*/
template< class Ret, class T1, class T2, class T3 >
class sdCallableBase3 {
public:
	virtual ~sdCallableBase3() {}
	virtual Ret operator()( T1 arg1, T2 arg2, T3 arg3 ) const = 0;
	virtual sdCallableBase3< Ret, T1, T2, T3 >* Clone() const = 0;
};

/*
============
sdCallableCaller0
============
*/
extern const char sdCallableCaller0_Identifier[];
template< class Func, class Ret >
class sdCallableCaller0 : 
	public sdCallableBase0< Ret >/*,
	public sdPoolAllocator< sdCallableCaller0< Func, Ret >, sdCallableCaller0_Identifier > */{
public:
	sdCallableCaller0( Func f ) : function( f ) {}
	virtual ~sdCallableCaller0() {}

	mutable Func function;
	virtual Ret operator()() const { return function(); }
	virtual sdCallableBase0< Ret >* Clone() const { return new sdCallableCaller0( function ); }
};

/*
============
sdCallableCaller1
============
*/
template< class Func = sdEmptyType, class R = sdEmptyType, class T1 = sdEmptyType > 
class sdCallableCaller1;

extern const char sdCallableCaller1_Identifier[];
template< class Func, class Ret, class T1 >
class sdCallableCaller1 : 
	public sdCallableBase1< Ret, T1 >/*,
	public sdPoolAllocator< sdCallableCaller1< Func, Ret, T1 >, sdCallableCaller1_Identifier > */{
public:
	sdCallableCaller1( Func f ) : function( f ) {}
	virtual ~sdCallableCaller1() {}

	mutable Func function;
	virtual Ret operator()( T1 arg1 ) const { return function( arg1 ); }
	virtual sdCallableBase1< Ret, T1 >* Clone() const { return new sdCallableCaller1( function ); }
};

template< class Func = sdEmptyType, class R = sdEmptyType, class T1 = sdEmptyType, class T2 = sdEmptyType > 
class sdCallableCaller2;
/*
============
sdCallableCaller2
============
*/
extern const char sdCallableCaller2_Identifier[];
template< class Func, class Ret, class T1, class T2 >
class sdCallableCaller2 : 
	public sdCallableBase2< Ret, T1, T2 >/*,
	public sdPoolAllocator< sdCallableCaller2< Func, Ret, T1, T2 >, sdCallableCaller2_Identifier >*/ {
public:
	sdCallableCaller2( Func f ) : function( f ) {}
	virtual ~sdCallableCaller2() {}

	mutable Func function;
	virtual Ret operator()( T1 arg1, T2 arg2 ) const { return function( arg1, arg2 ); }
	virtual sdCallableBase2< Ret, T1, T2 >* Clone() const { return new sdCallableCaller2( function ); }
};

template< class Func = sdEmptyType, class R = sdEmptyType, class T1 = sdEmptyType, class T2 = sdEmptyType , class T3 = sdEmptyType > 
class sdCallableCaller3;
/*
============
sdCallableCaller3
============
*/
extern const char sdCallableCaller3_Identifier[];
template< class Func, class Ret, class T1, class T2, class T3 >
class sdCallableCaller3 : 
	public sdCallableBase3< Ret, T1, T2, T3 >/*,
	public sdPoolAllocator< sdCallableCaller3< Func, Ret, T1, T2, T3 >, sdCallableCaller3_Identifier > */{
public:
	sdCallableCaller3( Func f ) : function( f ) {}
	virtual ~sdCallableCaller3() {}

	mutable Func function;
	virtual Ret operator()( T1 arg1, T2 arg2, T3 arg3 ) const { return function( arg1, arg2, arg3 ); }
	virtual sdCallableBase3< Ret, T1, T2, T3 >* Clone() const { return new sdCallableCaller3( function ); }
};

/*
============
sdCallable
============
*/
template< class Ret, class T1 = sdEmptyType, class T2 = sdEmptyType, class T3 = sdEmptyType >
class sdCallable {};

extern const char sdCallableNoParms_Identifier[];
template< class Ret >
class sdCallable< Ret() > /*:
	public sdPoolAllocator< sdCallable< Ret() >, sdCallableNoParms_Identifier > */{
public:
	sdCallable() : function( NULL ) {}
	~sdCallable() { Release(); }

	template< class Func >
	sdCallable( Func f ) : function( NULL ) {
		function = new sdCallableCaller0< Func, Ret >( f );
	}

	sdCallable& operator=( const sdCallable& f ) {
		Release();
		if( f.function ) {
			function = f.function->Clone();
		}

		return *this;
	}

	sdCallable( const sdCallable& rhs ) : function( NULL ) {
		*this = rhs;
	}

	Ret operator()() const {
		return (*function)();
	}

	void Release() {
		delete function;
		function = NULL;
	}

	bool IsValid() const { return function != NULL; }

private:
	mutable sdCallableBase0< Ret >* function;
};

/*
============
sdCallable< Ret( T1 ) >
============
*/
extern const char sdCallableOneParm_Identifier[];
template< class Ret, class T1 >
class sdCallable< Ret( T1 ) > /*:
	public sdPoolAllocator< sdCallable< Ret( T1 ) >, sdCallableOneParm_Identifier > */{
public:
	sdCallable() : function( NULL ) {}
	~sdCallable() { Release(); }

	template< class Func >
	sdCallable( Func f ) : function( NULL ) {
		function = new sdCallableCaller1< Func, Ret, T1 >( f );
	}

	sdCallable& operator=( const sdCallable& f ) {
		Release();
		if( f.function ) {
			function = f.function->Clone();
		}

		return *this;
	}

	sdCallable( const sdCallable& rhs ) : function( NULL ) {
		*this = rhs;
	}

	Ret operator()( T1 arg1 ) const {
		return (*function)( arg1 );
	}

	void Release() {
		delete function;
		function = NULL;
	}

	bool IsValid() const { return function != NULL; }

private:
	mutable sdCallableBase1< Ret, T1 >* function;
};

/*
============
sdCallable< Ret( T1, T2 ) >
============
*/
extern const char sdCallableTwoParms_Identifier[];
template< class Ret, class T1, class T2 >
class sdCallable< Ret( T1, T2 ) > /*:
	public sdPoolAllocator< sdCallable< Ret( T1, T2 ) >, sdCallableTwoParms_Identifier > */{
public:
	sdCallable() : function( NULL ) {}
	~sdCallable() { Release(); }

	template< class Func >
	sdCallable( Func f ) : function( NULL ) {
		function = new sdCallableCaller2< Func, Ret, T1, T2 >( f );
	}

	sdCallable& operator=( const sdCallable& f ) {
		Release();
		if( f.function ) {
			function = f.function->Clone();
		}
		
		return *this;
	}

	sdCallable( const sdCallable& rhs ) : function( NULL ) {
		*this = rhs;
	}

	Ret operator()( T1 arg1, T2 arg2 ) const {
		return (*function)( arg1, arg2 );
	}

	void Release() { 
		delete function;
		function = NULL;
	}

	bool IsValid() const { return function != NULL; }


private:
	mutable sdCallableBase2< Ret, T1, T2 >* function;
};

/*
============
sdCallable< Ret( T1, T2, T3 ) >
============
*/
extern const char sdCallableThreeParms_Identifier[];
template< class Ret, class T1, class T2, class T3 >
class sdCallable< Ret( T1, T2, T3 ) > /*:
	public sdPoolAllocator< sdCallable< Ret( T1, T2, T3 ) >, sdCallableThreeParms_Identifier >  */{
public:
	sdCallable() : function( NULL ) {}
	~sdCallable() { Release(); }

	template< class Func >
	sdCallable( Func f ) : function( NULL ) {
		function = new sdCallableCaller3< Func, Ret, T1, T2, T3 >( f );
	}

	sdCallable& operator=( const sdCallable& f ) {
		Release();
		if( f.function ) {
			function = f.function->Clone();
		}

		return *this;
	}

	sdCallable( const sdCallable& rhs ) : function( NULL ) {
		*this = rhs;
	}

	Ret operator()( T1 arg1, T2 arg2, T3 arg3 ) const {
		return (*function)( arg1, arg2, arg3 );
	}

	void Release() { 
		delete function;
		function = NULL;
	}

	bool IsValid() const { return function != NULL; }

private:
	mutable sdCallableBase3< Ret, T1, T2, T3 >* function;
};


// jrad - from boost...keep msvc from stripping const by indirecting like mad
// we don't seem to need this unless we want to extract the type back out...which we don't :)
// if we ever do, we need to change the &t in the sdReferenceWrapper constructor to addressof( t )
/*
template< typename T > struct remove_reference { typedef T type; };

template <typename T>
struct add_pointer
{
	typedef typename remove_reference< T >::type no_ref_type;
	typedef no_ref_type* type;
};

template <typename T> typename 
add_pointer< T >::type addressof( T& t ) {
	return reinterpret_cast< T* >( &const_cast< char& >( reinterpret_cast< const volatile char & >( t )));
}

*/

template< class T >
class sdReferenceWrapper {
public:
	explicit sdReferenceWrapper( T& t) : _t( &t /* addressof( t ) */ ) {}
	operator T&() const { return *_t; }

private:
	T* _t;
};


template< class T >
ID_INLINE sdReferenceWrapper< T > sdRef( T& t ) { return sdReferenceWrapper< T >( t ); }

template< class T >
ID_INLINE sdReferenceWrapper< T const > const sdCRef( T const & t ) { return sdReferenceWrapper< T const >( t ); }

}// namespace sdCallable

#endif // __CALLABLE_H__

