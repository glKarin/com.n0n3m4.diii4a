// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __LIB_SMARTPTR_H__
#define __LIB_SMARTPTR_H__

// this version eats up all instances where T doesn't have GetThisPtr enabled
ID_INLINE void sdDetail_AssignThis( ... ) { }

/*
============
sdSmartPtr
============
*/
#ifdef _DEBUG
template< class T, class CleanupPolicy = sdDefaultCleanupPolicy< T >, class CheckingPolicy = sdNULLPtrCheck< T > >
#else
template< class T, class CleanupPolicy = sdDefaultCleanupPolicy< T >, class CheckingPolicy = sdNoPtrCheck< T > >
#endif
class sdSmartPtr {
public:
	typedef T& Reference;
	typedef T* Pointer;

	typedef const T& ConstReference;
	typedef const T* ConstPointer;

	typedef CheckingPolicy checkingPolicy;
	typedef CleanupPolicy cleanupPolicy;

					explicit sdSmartPtr( Pointer p = NULL ) : pointee( p ), isWeak( false ) {
						count = GetCountAllocator().Alloc();
						*count = 1;
						sdDetail_AssignThis( pointee, *this );						
					}

					explicit sdSmartPtr( idNullPtr p ) : pointee( p ), isWeak( false ) {
						count = GetCountAllocator().Alloc();
						*count = 1;
						sdDetail_AssignThis( pointee, *this );
					}

					sdSmartPtr( const sdSmartPtr& rhs ) :
						pointee( rhs.pointee ),
						count( rhs.count ),
						isWeak( false ) {

						if( !isWeak ) {
							++( *count );
						}
					}

					~sdSmartPtr() {
						if( !isWeak ) {
							--( *count );
							if( ( *count ) == 0 ) {
								cleanupPolicy::Free( pointee );
								GetCountAllocator().Free( count );
							}
						}
						count	= NULL;
						pointee = NULL;
						isWeak	= true;
					}
	int				Num() const {
						return *count;	
					}

	template< class U >
	bool friend		operator==( const Pointer lhs, const sdSmartPtr< U >& rhs ) 	{ return lhs == rhs.pointee; }

	template< class U >
	bool friend		operator==( const sdSmartPtr< U >& lhs, const Pointer rhs ) 	{ return lhs.pointee == rhs; }

	template< class U >
	bool friend		operator==( const sdSmartPtr< U >& lhs, const sdSmartPtr& rhs ) { return lhs.Get() == rhs.pointee; }

	// operator !=
	template< class U >
	bool friend		operator!=( const Pointer lhs, const sdSmartPtr< U >& rhs ) 	{ return lhs != rhs.pointee; }

	template< class U >
	bool friend		operator!=( const sdSmartPtr< U >& lhs, const Pointer rhs ) 	{ return lhs.pointee != rhs; }

	template< class U >
	bool friend		operator!=( const sdSmartPtr& lhs, const sdSmartPtr< U >& rhs ) { return lhs.pointee != rhs.pointee; }


	// dereferencing
	Pointer			operator->()			{ checkingPolicy::Check( pointee ); return pointee; }
	ConstPointer	operator->() const		{ checkingPolicy::Check( pointee ); return pointee; }

	Reference		operator*()				{ checkingPolicy::Check( pointee ); return *pointee; }
	ConstReference	operator*()	const		{ checkingPolicy::Check( pointee ); return *pointee; }

	// accessors
	Pointer			Get()				{ return pointee; }
	const Pointer	Get() const			{ return pointee; }


	sdSmartPtr& 	operator=( const sdSmartPtr& rhs ) {
						sdSmartPtr( rhs ).Swap( *this );
						return *this;
					}

	sdSmartPtr& 	Reset( const Pointer rhs ) {
						sdSmartPtr( rhs ).Swap( *this );
						return *this;
					}

	sdSmartPtr& 	ResetWeak( const sdSmartPtr& rhs ) {
						MakeWeak();
						assert( count == NULL );
						pointee = rhs.pointee;
						count = rhs.count;
						return *this;
					}

	void			Swap( const sdSmartPtr& rhs ) {
						idSwap( isWeak, rhs.isWeak );
						idSwap( pointee, const_cast< Pointer >( rhs.pointee ) );
						idSwap( count, const_cast< int* >( rhs.count ) );
					}
protected:

	// we don't support this, use != NULL instead
	bool			operator!() const;
	
	void			MakeWeak() {
						assert( pointee == NULL || isWeak );
						if( !isWeak ) {
							GetCountAllocator().Free( count );
							count = NULL;
						}
						isWeak = true;
					}

private:
	Pointer							pointee;
	int*							count;
	mutable bool					isWeak;

	typedef idBlockAlloc< int, 64 > countAllocator_t;
	static countAllocator_t& GetCountAllocator() {
		static countAllocator_t	countAllocator;
		return countAllocator;
	}

};


template< class T, class CleanupPolicy, class CheckingPolicy >
void Swap( const sdSmartPtr< T, CleanupPolicy, CheckingPolicy >& lhs, const sdSmartPtr< T, CleanupPolicy, CheckingPolicy >& rhs ) {
	lhs.Swap( rhs );
}


template< class T >
class sdSmartPtrFromThis {
public:
	sdSmartPtrFromThis() {}
	sdSmartPtrFromThis( const sdSmartPtrFromThis& rhs ) {}
	sdSmartPtr< T >&			GetThisPtr()		{ return thisPtr; }
	const sdSmartPtr< T >&		GetThisPtr() const	{ return thisPtr; }

	sdSmartPtrFromThis&			operator=( const sdSmartPtrFromThis& rhs ) { return *this; }

	mutable sdSmartPtr< T > thisPtr;
};

template< class T > 
void sdDetail_AssignThis( const sdSmartPtrFromThis< T >* p, const sdSmartPtr< T >& ptr ) {
	if( p != NULL ) {
		p->thisPtr.ResetWeak( ptr );
	}
}


#endif // ! __LIB_SMARTPTR_H__
