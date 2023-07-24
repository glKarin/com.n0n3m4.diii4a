// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __CALLABLE_H__
#define __CALLABLE_H__

namespace sdFunctions {
	class sdEmptyType {};

	/*
	============
	sdBinder1
	============
	*/
	/*
	template< class RetType, class T1, class Func >
	class sdBinder1 {
	public:
	sdBinder1( Func func_, T1 arg1_ ) : arg1( arg1_ ) { function = func_; }

	RetType operator()() { return function( arg1 ); }
	template< class T2 >
	RetType operator()( T2 arg2 ) { return function( arg1, arg2 ); }

	template< class T2, class T3 >
	RetType operator()( T2 arg2, T3 arg3 ) { return function( arg1, arg2, arg3 ); }

	private:
	T1 arg1;
	Func function;
	};
	*/
	/*
	============
	sdBinder1
	============
	*/
	/*
	template< class RetType, class T1, class Func >
	sdBinder1< RetType, T1, Func > sdBind( Func func, T1 arg1 ) { return sdBinder1< RetType, T1, Func >( func, arg1 ); }
	*/
	/*
	============
	sdBinder2
	============
	*/
	/*
	template< class RetType, class T1, class T2, class Func >
	class sdBinder2 {
	public:
	sdBinder2( Func func_, T1 arg1_, T2 arg2_ ) : arg1( arg1_ ), arg2( arg2_ ) { function = func_; }

	RetType operator()() { return function( arg1, arg2 ); }

	template< class T3 >
	RetType operator()( T3 arg3 ) { return function( arg1, arg2, arg3 ); }

	template< class T3, class T4 >
	RetType operator()( T3 arg3, T4 arg4 ) { return function( arg1, arg2, arg3, arg4 ); }

	private:
	T1 arg1;
	T2 arg2;
	Func function;
	};
	*/
	/*
	============
	sdBinder2
	============
	*/
	/*
	template< class RetType, class T1, class T2, class Func >
	sdBinder2< RetType, T1, T2, Func > sdBind( Func func, T1 arg1, T2 arg2 ) { return sdBinder2< RetType, T1, T2, Func >( func, arg1, arg2 ); }
	*/
	/*
	============
	sdBinder3
	============
	*/
	/*
	template< class RetType, class T1, class T2, class T3, class Func >
	class sdBinder3 {
	public:
	sdBinder3( Func func_, T1 arg1_, T2 arg2_, T3 arg3_ ) : arg1( arg1_ ), arg2( arg2_ ), arg3( arg3_ ) { function = func_; }

	RetType operator()() { return function( arg1, arg2, arg3 ); }

	template< class T4 >
	RetType operator()( T4 arg4 ) { return function( arg1, arg2, arg3, arg4 ); }

	template< class T4, class T5 >
	RetType operator()( T3 arg3, T4 arg4 ) { return function( arg1, arg2, arg3, arg4, arg5 ); }

	private:
	T1 arg1;
	T2 arg2;
	T3 arg3;
	Func function;
	};
	*/
	/*
	============
	sdBinder3
	============
	*/
	//template< class RetType, class T1, class T2, class T3, class Func >
	//class sdBinder3< RetType, T1, T2, T3, Func > sdBind( Func func, T1 arg1, T2 arg2, T3 arg3 ) { return sdBinder3< RetType, T1, T2, T3, Func >( func, arg1, arg2, arg3 ); }

	/*
	============
	sdBinderMember0
	============
	*/
	template< class RetType, class T, class Func, class T1 = sdEmptyType, class T2 = sdEmptyType, class T3 = sdEmptyType >
	class sdBinderMember0 {
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

	template< class RetType, class T, class Func, class T1 = sdEmptyType, class T2 = sdEmptyType, class T3 = sdEmptyType >
	class sdBinderMember1 {
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
		virtual Ret operator()() = 0;
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
		virtual Ret operator()( T1 arg1 ) = 0;
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
		virtual Ret operator()( T1 arg1, T2 arg2 ) = 0;
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
		virtual Ret operator()( T1 arg1, T2 arg2, T3 arg3 ) = 0;
		virtual sdCallableBase3< Ret, T1, T2, T3 >* Clone() const = 0;
	};

	namespace {
		static const size_t minElements = 2048;
	}

	/*
	============
	sdCallableCaller0
	============
	*/
	template< class Func, class Ret >
	class sdCallableCaller0 : 
		public sdCallableBase0< Ret > {
	public:
		sdCallableCaller0( Func f ) : function( f ) {}

		class sdDynamicBlockManager {
		public:
			sdDynamicBlockManager() { allocator.Init(); }
			~sdDynamicBlockManager() { allocator.Shutdown(); }
			idDynamicBlockAlloc< sdCallableCaller0, minElements, 32, false > allocator;
		};

		Func function;
		virtual Ret operator()() { return function(); }

		static void* operator new( size_t size ) {
			if( size != sizeof( sdCallableCaller0 )) {
				return ::operator new( size );
			}
			return static_cast< void* >( memoryManager.allocator.Alloc( 1 ));
		}

		static void operator delete( void* ptr, size_t size ) {
			if( size != sizeof( sdCallableCaller0 )) {
				::operator delete( ptr );
				return;
			}

			memoryManager.allocator.Free( static_cast< sdCallableCaller0* >( ptr ));
		}

		virtual sdCallableBase0< Ret >* Clone() const {
			return new sdCallableCaller0( function );
		}

		static sdDynamicBlockManager memoryManager;
		};

	template< class Func, class Ret >
	typename sdCallableCaller0< Func, Ret >::sdDynamicBlockManager sdCallableCaller0< Func, Ret >::memoryManager;


	/*
	============
	sdCallableCaller1
	============
	*/
	template< class Func = sdEmptyType, class R = sdEmptyType, class T1 = sdEmptyType > 
	class sdCallableCaller1;

	template< class Func, class Ret, class T1 >
	class sdCallableCaller1 : 
		public sdCallableBase1< Ret, T1 > {
	public:
		sdCallableCaller1( Func f ) : function( f ) {}

		class sdDynamicBlockManager {
		public:
			sdDynamicBlockManager() { allocator.Init(); }
			~sdDynamicBlockManager() { allocator.Shutdown(); }
			idDynamicBlockAlloc< sdCallableCaller1, minElements, 32, false > allocator;
		};

		Func function;
		virtual Ret operator()( T1 arg1 ) { return function( arg1 ); }

		static void* operator new( size_t size ) {
			if( size != sizeof( sdCallableCaller1 )) {
				return ::operator new( size );
			}
			return static_cast< void* >( memoryManager.allocator.Alloc( 1 ));
		}

		static void operator delete( void* ptr, size_t size ) {
			if( size != sizeof( sdCallableCaller1 )) {
				::operator delete( ptr );
				return;
			}

			memoryManager.allocator.Free( static_cast< sdCallableCaller1* >( ptr ));
		}

		virtual sdCallableBase1< Ret, T1 >* Clone() const {
			return new sdCallableCaller1( function );
		}

		static sdDynamicBlockManager memoryManager;
		};

	template< class Func, class Ret, class T1 >
		typename sdCallableCaller1< Func, Ret, T1 >::sdDynamicBlockManager sdCallableCaller1< Func, Ret, T1 >::memoryManager;

	template< class Func = sdEmptyType, class R = sdEmptyType, class T1 = sdEmptyType, class T2 = sdEmptyType > 
	class sdCallableCaller2;

	/*
	============
	sdCallableCaller2
	============
	*/
	template< class Func, class Ret, class T1, class T2 >
	class sdCallableCaller2 : 
		public sdCallableBase2< Ret, T1, T2 > {
	public:
		sdCallableCaller2( Func f ) : function( f ) {}

		class sdDynamicBlockManager {
		public:
			sdDynamicBlockManager() { allocator.Init(); }
			~sdDynamicBlockManager() { allocator.Shutdown(); }
			idDynamicBlockAlloc< sdCallableCaller2, minElements, 32, false > allocator;
		};

		Func function;
		virtual Ret operator()( T1 arg1, T2 arg2 ) { return function( arg1, arg2 ); }

		static void* operator new( size_t size ) {
			if( size != sizeof( sdCallableCaller2 )) {
				return ::operator new( size );
			}
			return static_cast< void* >( memoryManager.allocator.Alloc( 1 ));
		}

		static void operator delete( void* ptr, size_t size ) {
			if( size != sizeof( sdCallableCaller2 )) {
				::operator delete( ptr );
				return;
			}

			memoryManager.allocator.Free( static_cast< sdCallableCaller2* >( ptr ));
		}

		virtual sdCallableBase2< Ret, T1, T2  >* Clone() const {
			return new sdCallableCaller2( function );
		}

		static sdDynamicBlockManager memoryManager;
		};

	template< class Func, class Ret, class T1, class T2 >
	typename sdCallableCaller2< Func, Ret, T1, T2 >::sdDynamicBlockManager sdCallableCaller2< Func, Ret, T1, T2 >::memoryManager;

	template< class Func = sdEmptyType, class R = sdEmptyType, class T1 = sdEmptyType, class T2 = sdEmptyType , class T3 = sdEmptyType > 
	class sdCallableCaller3;
	/*
	============
	sdCallableCaller3
	============
	*/
	template< class Func, class Ret, class T1, class T2, class T3 >
	class sdCallableCaller3 : 
		public sdCallableBase3< Ret, T1, T2, T3 > {
	public:
		sdCallableCaller3( Func f ) : function( f ) {}

		class sdDynamicBlockManager {
		public:
			sdDynamicBlockManager() { allocator.Init(); }
			~sdDynamicBlockManager() { allocator.Shutdown(); }
			idDynamicBlockAlloc< sdCallableCaller3, minElements, 32, false > allocator;
		};

		Func function;
		virtual Ret operator()( T1 arg1, T2 arg2, T3 arg3 ) { return function( arg1, arg2, arg3 ); }

		static void* operator new( size_t size ) {
			if( size != sizeof( sdCallableCaller3 )) {
				return ::operator new( size );
			}
			return static_cast< void* >( memoryManager.allocator.Alloc( 1 ));
		}

		static void operator delete( void* ptr, size_t size ) {
			if( size != sizeof( sdCallableCaller3 )) {
				::operator delete( ptr );
				return;
			}

			memoryManager.allocator.Free( static_cast< sdCallableCaller3* >( ptr ));
		}

		virtual sdCallableBase3< Ret, T1, T2, T3 >* Clone() const {
			return new sdCallableCaller3( function );
		}

		static sdDynamicBlockManager memoryManager;
		};

	template< class Func, class Ret, class T1, class T2, class T3 >
		typename sdCallableCaller3< Func, Ret, T1, T2, T3 >::sdDynamicBlockManager sdCallableCaller3< Func, Ret, T1, T2, T3 >::memoryManager;

	/*
	============
	sdCallable
	============
	*/
	template< class Ret, class T1 = sdEmptyType, class T2 = sdEmptyType, class T3 = sdEmptyType >
	class sdCallable {};

	template< class Ret >
	class sdCallable< Ret() > {
	public:
		sdCallable() : function( NULL ) {}
		~sdCallable() { Release(); }

		template< class Func >
			sdCallable( Func f ) {
				function = new sdCallableCaller0< Func, Ret >( f );		
			}

			sdCallable& operator=( const sdCallable& f ) {
				Release();
				if( f.function ) {
					function = f.function->Clone();
				}

				return *this;
			}

			sdCallable( const sdCallable& rhs ) {
				operator=( rhs );
			}

			Ret operator()() {
				return (*function)();
			}

			void Release() {
				delete function;
				function = NULL;
			}

	private:
		sdCallableBase0< Ret >* function;
	};

	/*
	============
	sdCallable< Ret( T1 ) >
	============
	*/
	template< class Ret, class T1 >
	class sdCallable< Ret( T1 ) > {
	public:
		sdCallable() : function( NULL ) {}
		~sdCallable() { Release(); }

		template< class Func >
			sdCallable( Func f ) {
				function = new sdCallableCaller1< Func, Ret, T1 >( f );
			}

			sdCallable& operator=( const sdCallable& f ) {
				Release();
				if( f.function ) {
					function = f.function->Clone();
				}

				return *this;
			}

			sdCallable( const sdCallable& rhs ) {
				operator=( rhs );
			}

			Ret operator()( T1 arg1 ) {
				return (*function)( arg1 );
			}

			void Release() {
				delete function;
				function = NULL;
			}

	private:
		sdCallableBase1< Ret, T1 >* function;
	};

	/*
	============
	sdCallable< Ret( T1, T2 ) >
	============
	*/
	template< class Ret, class T1, class T2 >
	class sdCallable< Ret( T1, T2 ) > {
	public:
		sdCallable() : function( NULL ) {}
		~sdCallable() { Release(); }

		template< class Func >
			sdCallable( Func f ) {
				function = new sdCallableCaller2< Func, Ret, T1, T2 >( f );
			}

			sdCallable& operator=( const sdCallable& f ) {
				Release();
				if( f.function ) {
					function = f.function->Clone();
				}

				return *this;
			}

			sdCallable( const sdCallable& rhs ) {
				operator=( rhs );
			}

			Ret operator()( T1 arg1, T2 arg2 ) {
				return (*function)( arg1, arg2 );
			}

			void Release() { 
				delete function;
				function = NULL;
			}

			bool IsValid() const {
				return ( function != NULL );
			}

	private:
		sdCallableBase2< Ret, T1, T2 >* function;
	};

	/*
	============
	sdCallable< Ret( T1, T2, T3 ) >
	============
	*/
	template< class Ret, class T1, class T2, class T3 >
	class sdCallable< Ret( T1, T2, T3 ) > {
	public:
		sdCallable() : function( NULL ) {}
		~sdCallable() { Release(); }

		template< class Func >
			sdCallable( Func f ) {
				function = new sdCallableCaller3< Func, Ret, T1, T2, T3 >( f );
			}

			sdCallable& operator=( const sdCallable& f ) {
				Release();
				if( f.function ) {
					function = f.function->Clone();
				}

				return *this;
			}

			sdCallable( const sdCallable& rhs ) {
				operator=( rhs );
			}

			Ret operator()( T1 arg1, T2 arg2, T3 arg3 ) {
				return (*function)( arg1, arg2, arg3 );
			}

			void Release() { 
				delete function;
				function = NULL;
			}

			bool IsValid() const {
				return ( function != NULL );
			}

	private:
		sdCallableBase3< Ret, T1, T2, T3 >* function;
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

