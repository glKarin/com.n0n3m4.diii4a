// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __FUNCTION_SIGNALS_H__
#define __FUNCTION_SIGNALS_H__

namespace sdFunctions {

	/*
	============
	sdSignalBase
	============
	*/
	class sdSignalBase {
	public:		
		typedef sdUtility::sdHandle< int, -1 > connectionHandle_t;

		virtual			~sdSignalBase() {}
		virtual void	Disconnect( connectionHandle_t& handle ) = 0;
	};

	/*
	============
	sdConnection
	============
	*/
	class sdConnection {
	public:
		typedef sdSignalBase::connectionHandle_t connectionHandle_t;

							sdConnection( sdSignalBase* target_ = NULL, connectionHandle_t handle_ = connectionHandle_t() ) :
								target( target_ ),
								handle( handle_ ) {
							}

							~sdConnection() {												
							}
		
		void				Release() {
								target = NULL;
								handle.Release();
							}

		void				Disconnect() {
								if( target != NULL ) {
									target->Disconnect( handle );
									target = NULL;
								}
							}
	private:
		sdSignalBase*		target;
		connectionHandle_t	handle;
	};

	/*
	============
	sdScopedConnection
	============
	*/
	class sdScopedConnection {
	public:
		typedef sdSignalBase::connectionHandle_t connectionHandle_t;

							sdScopedConnection() {}
		explicit			sdScopedConnection( sdConnection& rhs ) { *this = rhs; }
							~sdScopedConnection() {
								connection.Disconnect();
							}

		void				Release() {
								connection.Release();
							}

		sdScopedConnection& operator=( sdScopedConnection& rhs ) {
								connection.Disconnect();
								connection = rhs.connection;
								rhs.Release();
								return *this;
							}
		sdScopedConnection& operator=( sdConnection& rhs ) {
								connection.Disconnect();
								connection = rhs;
								rhs.Release();
								return *this;
							}

	private:
		sdConnection connection;
	};


#define ITERATE_TARGETS								\
	for( int i = 0; i < targets.Num(); i++ ) {		\
		if( !targets.IsValid( i ) ) { continue; }

#define END_ITERATE_TARGETS	}

#define SIGNAL_IMPL \
	sdConnection	Connect( funcType_t func, int group = 0 ) {		\
		connectionHandle_t newItemHandle = targets.Acquire();		\
		targets[ newItemHandle ] = new funcType_t( func );			\
		return sdConnection ( this, newItemHandle );				\
	}																\
\
	virtual void	Disconnect( connectionHandle_t& handle ) {		\
		delete targets[ handle ];									\
		targets.Release( handle );									\
	}																\
	virtual			~sdSignal() {}

	template< class Ret, class T1 = sdEmptyType, class T2 = sdEmptyType, class T3 = sdEmptyType >
	class sdSignal {};

	template< class Ret >
	class sdSignal< Ret() > :
		public sdSignalBase {
	private:
		typedef sdCallable< Ret() > funcType_t;				
		typedef sdHandles< funcType_t* > container_t;
		typedef typename container_t::handle_t connectionHandle_t;

	public:
		SIGNAL_IMPL

		template< Ret >
		Ret						operator()(void) {
									return Ret();
								}

		void					operator()(void) {
								ITERATE_TARGETS
									(*targets[ i ])();
								END_ITERATE_TARGETS
							}
	private:
		container_t targets;
	};

	template< class Ret, class T1 >
	class sdSignal< Ret( T1 ) > :
		public sdSignalBase {
	private:
		typedef sdCallable< Ret( T1 ) > funcType_t;				
		typedef sdHandles< funcType_t* > container_t;
		typedef typename container_t::handle_t connectionHandle_t;

	public:
		SIGNAL_IMPL
			template< Ret >
			Ret					operator()( T1 t1 ) {
									return Ret();
								}
/*
			void			operator()( T1 t1 ) {
								ITERATE_TARGETS
									(*targets[ i ])( t1 );
								END_ITERATE_TARGETS
							}
						*/
	private:
		container_t targets;
	};

	template< class Ret, class T1, class T2 >
	class sdSignal< Ret( T1, T2 ) > : public sdSignalBase {
	private:
		typedef sdCallable< Ret( T1, T2 ) > funcType_t;				
		typedef sdHandles< funcType_t* > container_t;
		typedef typename container_t::handle_t connectionHandle_t;

	public:
		SIGNAL_IMPL

			void			operator()( T1 t1, T2 t2 ) {
								ITERATE_TARGETS
									(*targets[ i ])( t1, t2 );
								END_ITERATE_TARGETS
							}
	private:
		container_t targets;
	};

	template< class Ret, class T1, class T2, class T3 >
	class sdSignal< Ret( T1, T2, T3 ) > :
		public sdSignalBase {
	private:
		typedef sdCallable< Ret( T1, T2, T3 ) > funcType_t;				
		typedef sdHandles< funcType_t* > container_t;
		typedef typename container_t::handle_t connectionHandle_t;

	public:
		SIGNAL_IMPL

			void			operator()( T1 t1, T2 t2, T3 t3 ) {
								ITERATE_TARGETS
									(*targets[ i ])( t1, t2, t3 );
								END_ITERATE_TARGETS
							}
	private:
		container_t targets;
	};

#undef ITERATE_TARGETS	
#undef END_ITERATE_TARGETS
}

// useful typedefs
typedef sdFunctions::sdConnection		sdFunctionConnection;

#endif // !__SIGNALS_H__
