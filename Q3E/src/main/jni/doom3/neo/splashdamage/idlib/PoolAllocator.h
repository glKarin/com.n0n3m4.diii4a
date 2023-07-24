// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __LIB_POOL_ALLOCATOR_H__
#define __LIB_POOL_ALLOCATOR_H__

namespace sdDetails {
/*
===============================================================================

Block based allocator for fixed size objects. (based off of idBlockAlloc)
All objects of the 'type' are NOT constructed.

===============================================================================
*/

template<class type, int blockSize>
class sdPoolAlloc {
public:
	sdPoolAlloc( void );
	~sdPoolAlloc( void );

	void					Shutdown( void );

	type *					Alloc( void );
	void					Free( type *element );
	int						Compact( void );

	int						GetTotalCount( void ) const { return total; }
	int						GetAllocCount( void ) const { return active; }
	int						GetFreeCount( void ) const { return total - active; }

private:
	struct element_t {
		element_t *				next;
		char					t[ sizeof( type ) ];
	};
	struct block_t {
		element_t				elements[blockSize];
		block_t *				next;
	};

	block_t *					blocks;
	element_t *					free;
	int							total;
	int							active;
	int							numFree;

private:
	bool						IsFree( const element_t* element );
};

/*
============
sdPoolAlloc<type,blockSize>::sdPoolAlloc
============
*/
template<class type, int blockSize>
sdPoolAlloc<type,blockSize>::sdPoolAlloc( void ) {
	blocks = NULL;
	free = NULL;
	total = active = numFree = 0;
}

/*
============
sdPoolAlloc<type,blockSize>::~sdPoolAlloc
============
*/
template<class type, int blockSize>
sdPoolAlloc<type,blockSize>::~sdPoolAlloc( void ) {
	Shutdown();
}

/*
============
sdPoolAlloc<type,blockSize>::IsFree
============
*/
template<class type, int blockSize>
bool sdPoolAlloc<type,blockSize>::IsFree( const element_t* element ) {
	element_t* iter = free;
	while( iter != NULL ) {
		if( iter == element ) {
			return true;
		}
		iter = iter->next;
	}
	return false;
}

/*
============
sdPoolAlloc<type,blockSize>::Alloc
============
*/
template<class type, int blockSize>
type *sdPoolAlloc<type,blockSize>::Alloc( void ) {
	if ( !free ) {
		block_t *block = new block_t;
		block->next = blocks;
		blocks = block;
		for ( int i = 0; i < blockSize; i++ ) {
			block->elements[i].next = free;
			free = &block->elements[i];
		}
		total += blockSize;
	}
	active++;
	element_t *element = free;
	free = free->next;
	element->next = NULL;
	return reinterpret_cast< type* >( &element->t );
}

/*
============
sdPoolAlloc<type,blockSize>::Free
============
*/
template<class type, int blockSize>
void sdPoolAlloc<type,blockSize>::Free( type *t ) {
	if( t == NULL ) {
		return;
	}
	element_t *element = (element_t *)( ( (unsigned char *) t ) - ( (UINT_PTR) &((element_t *)0)->t ) );
	//element_t *element;
	//element = ( element_t * )( ( (unsigned char *)t ) - offsetof( element_t, t ) );
	element->next = free;
	free = element;
	active--;
	numFree++;
}

/*
============
sdPoolAlloc<type,blockSize>::Compact
find blocks that are completely empty and free them

this is relatively slow and should only be called after an event that is known to 
free a large chunk of elements

returns the number of bytes freed
============
*/
template<class type, int blockSize>
int sdPoolAlloc<type,blockSize>::Compact( void ) {
	bool			usedHeap = false;
	block_t**		blocksToUnlink = NULL;
	int				blocksToUnlinkNum = 0;

	if( numFree >= ( 512 * 1024 ) ) {
		usedHeap = true;
		blocksToUnlink = static_cast< block_t** >( Mem_AllocAligned( numFree, ALIGN_16 ) );
	} else {
		blocksToUnlink = static_cast< block_t** >( _alloca16( numFree ) );
	}

	block_t* block = blocks;
	block_t* prev = NULL;

	while( block != NULL ) {
		bool allFreed = true;

		for( int i = 0; i < blockSize; i++ ) {
			if( !IsFree( &block->elements[ i ] ) ) {
				allFreed = false;
				break;
			}
		}

		// all elements are in the free list, unlink and deallocate this block
		if( allFreed ) {
			// unlink all elements from the free list
			
			for( int i = 0; i < blockSize; i++ ) {
				element_t* current = &block->elements[ i ];
				element_t* prev = NULL;
				element_t* iter = free;
				while( iter != NULL ) {
					element_t* next = iter->next;					
					if( iter == current ) {
						if( prev != NULL ) {
							prev->next = next;
						} else {
							free = next;
						}
						break;
					}
					prev = iter;					
					iter = next;
				}
			}
			blocksToUnlink[ blocksToUnlinkNum++ ] = block;
		} else {
			prev = block;
		}
		block = block->next;
	}

	assert( blocksToUnlinkNum <= numFree );

	for( int i = 0; i < blocksToUnlinkNum; i++ ) {
		block_t* current = blocksToUnlink[ i ];
		block_t* prev = NULL;
		block_t* iter = blocks;
		while( iter != NULL ) {
			block_t* next = iter->next;					
			if( iter == current ) {
				if( prev != NULL ) {
					prev->next = next;
				} else {
					blocks = next;
				}
				break;
			}
			prev = iter;					
			iter = next;
		}
		delete current;
	}

	if( usedHeap ) {
		Mem_FreeAligned( blocksToUnlink );
		blocksToUnlink = NULL;
	}

	numFree -= blocksToUnlinkNum;

	total -= blockSize * blocksToUnlinkNum;
	return blocksToUnlinkNum * blockSize * sizeof( type );
}

/*
============
sdPoolAlloc<type,blockSize>::Shutdown
============
*/
template<class type, int blockSize>
void sdPoolAlloc<type,blockSize>::Shutdown( void ) {
	while( blocks != NULL ) {
		block_t *block = blocks;
		blocks = blocks->next;
		delete block;
	}
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

}

// derive class T from sdPoolAllocator< T > to allocate all objects of type T from a memory pool
// use new and delete like normal on the objects
#define SD_DISAMBIGUATE_POOL_ALLOCATOR( Allocator )															\
	void* operator new[]( size_t size )					{ return Allocator::operator new[]( size ); }		\
	void operator delete[]( void* ptr, size_t size )	{ Allocator::operator delete[]( ptr, size ); }		\
	void* operator new( size_t size )					{ return Allocator::operator new( size ); }			\
	void operator delete( void* ptr, size_t size )		{ Allocator::operator delete( ptr, size ); }		\
	void* operator new[]( size_t size, size_t t1, int t2, char *fileName, int lineNumber )					{ return Allocator::operator new[]( size, t1, t2, fileName, lineNumber ); }	\
	void operator delete[]( void *ptr, size_t size, int t2, char *fileName, int lineNumber )				{ Allocator::operator delete[]( ptr, size, t2, fileName, lineNumber ); }	\
	void* operator new( size_t size, size_t t1, int t2, char *fileName, int lineNumber )					{ return Allocator::operator new( size, t1, t2, fileName, lineNumber ); }	\
	void operator delete( void *ptr, size_t size, int t2, char *fileName, int lineNumber )					{ Allocator::operator delete( ptr, size, t2, fileName, lineNumber ); }

extern const char sdPoolAllocator_DefaultIdentifier[];
class sdDynamicBlockManagerBase {
public:
	virtual				~sdDynamicBlockManagerBase() {}
	
	virtual void		Init() = 0;
	virtual	void		Shutdown() = 0;

	virtual void		PrintInfo() = 0;
	virtual void		Accumulate( int& baseBytes, int& freeBytes ) = 0;
	virtual const char* GetName() const = 0;
	virtual int			Compact() = 0;
	virtual void		Purge() = 0;
	virtual bool		IsValid() const = 0;

	static void			MemoryReport( const idCmdArgs& args );
	
	static void			InitPools();
	static void			ShutdownPools();
	static void			CompactPools();

protected:
	typedef idList< sdDynamicBlockManagerBase* > blockManagerList_t;
	static blockManagerList_t& GetList() {
		static blockManagerList_t list;		
		return list;
	}
};

/*
============
sdDynamicBlockManager
============
*/
template< class T, const char* blockName, size_t blockSize >
class sdDynamicBlockManager :
	public sdDynamicBlockManagerBase {
public:
	typedef sdDetails::sdPoolAlloc< T, blockSize > allocatorType_t;
	
					sdDynamicBlockManager() {
						Init();
						GetList().Append( this );
					}
					virtual	~sdDynamicBlockManager() {
					}

	virtual void	Init() {
						assert( allocator == NULL );
						allocator = new allocatorType_t;
					}
	virtual void	Shutdown() {
						if( allocator != NULL ) {
							allocator->Shutdown();
							delete allocator;
							allocator = NULL;
						}
						GetList().Remove( this );
					}

	const char*		GetName() const { return blockName; }

	T*				Alloc() {
						assert( allocator != NULL );
						if( allocator == NULL ) {
							return NULL;
						}
						return allocator->Alloc();
					}

	void			Free( T* ptr ) {
						assert( allocator != NULL );
						if( allocator == NULL ) {
							return;
						}
						allocator->Free( ptr );
					}

	virtual int		Compact() {
						if( allocator == NULL ) {
							return 0;
						}
						return allocator->Compact();
					}

	virtual void	Purge() {
						if( allocator != NULL ) {
							allocator->Shutdown();
						}
					}

	virtual void	PrintInfo() {
						idLib::Printf( "\n%s\n", blockName );
						idLib::Printf( "===================\n", blockName );
						idLib::Printf( "Base Block Memory %i bytes free, %i bytes total\n", allocator->GetFreeCount() * sizeof( T ), allocator->GetTotalCount() * sizeof( T ) );
					}

	virtual void	Accumulate( int& baseBytes, int& freeBytes ) {
						assert( allocator != NULL );
						baseBytes += allocator->GetTotalCount();
						freeBytes += allocator->GetFreeCount();
					}

	virtual bool	IsValid() const { return allocator != NULL; }

private:
	allocatorType_t* allocator;
};

class sdLockingPolicy_None {
public:
	void Acquire() {}
	void Release() {}
};

class sdLockingPolicy_Lock {
public:
	void Acquire() { lock.Acquire(); }
	void Release() { lock.Release(); }

private:
	sdLock lock;
};

/*
============
sdPoolAllocator
============
*/
template< class T, const char* name = sdPoolAllocator_DefaultIdentifier, size_t baseBlockSize = 512, class lockingPolicy = sdLockingPolicy_None >		
class sdPoolAllocator {

public:	
	static const size_t ELEMENTS_PER_PAGE = baseBlockSize;
	
	// free the entire pool
	static void PurgeAllocator() { 
		GetMemoryManager().Purge();
	}

	/*
	============
	operator new
	============
	*/
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new( size_t size ) {
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		if( size != sizeof( T )) {
			assert( 0 );
		}
		
		lock.Acquire();
		void* retVal = static_cast< void* >( GetMemoryManager().Alloc() );
		lock.Release();
		return retVal;
	}

#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new( size_t size, size_t t1, int t2, char *fileName, int lineNumber ) {
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		if( size != sizeof( T )) {
			assert( 0 );
		}
		
		lock.Acquire();
		void* retVal = static_cast< void* >( GetMemoryManager().Alloc() );
		lock.Release();
		return retVal;
	}

	/*
	============
	operator delete
	============
	*/
	void operator delete( void* ptr, size_t size ) {
		lock.Acquire();
		GetMemoryManager().Free( static_cast< T* >( ptr ) );
		lock.Release();
	}

	/*
	============
	operator delete
	============
	*/
	void operator delete( void *ptr, size_t size, int t2, char *fileName, int lineNumber ) {
		lock.Acquire();
		GetMemoryManager().Free( static_cast< T* >( ptr ) );
		lock.Release();
	}

	/*
	============
	operator new[]
	============
	*/
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new[]( size_t size ) {
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		return ::new char[ size ];
	}

	/*
	============
	operator new[]
	============
	*/
#ifdef ID_REDIRECT_NEWDELETE
#undef new
#endif
	void* operator new[]( size_t size, size_t t1, int t2, char *fileName, int lineNumber ) {
#ifdef ID_REDIRECT_NEWDELETE
#define new ID_DEBUG_NEW
#endif
		return ::new char[ size ];
	}

	/*
	============
	operator delete[]
	============
	*/
	void operator delete[]( void* ptr, size_t size ) {
		::delete[]( static_cast< char* >( ptr ) );
	}

	/*
	============
	operator delete[]
	============
	*/
	void operator delete[]( void *ptr, size_t size, int t2, char *fileName, int lineNumber ) {
		::delete[]( static_cast< char* >( ptr ) );
	}

private:
	typedef lockingPolicy LockingPolicy;
	static LockingPolicy lock;

	typedef sdDynamicBlockManager< T, name, baseBlockSize > memoryManager_t;
	static memoryManager_t& GetMemoryManager() {
		static memoryManager_t manager;
		if( !manager.IsValid() ) {
			manager.Init();
		}
		return manager;
	}
};

template< class T, const char* name, size_t baseBlockSize, class lockingPolicy >
typename sdPoolAllocator< T, name, baseBlockSize, lockingPolicy >::LockingPolicy sdPoolAllocator< T, name, baseBlockSize, lockingPolicy >::lock;

#endif // !__LIB_POOL_ALLOCATOR_H__
