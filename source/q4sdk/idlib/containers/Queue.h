
#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/

template< class type, int nextOffset >
class idQueueTemplate {
public:
							idQueueTemplate( void );

	void					Add( type *element );
	type *					Get( void );

private:
	type *					first;
	type *					last;
};

#define QUEUE_NEXT_PTR( element )		(*((type**)(((byte*)element)+nextOffset)))

template< class type, int nextOffset >
idQueueTemplate<type,nextOffset>::idQueueTemplate( void ) {
	first = last = NULL;
}

template< class type, int nextOffset >
void idQueueTemplate<type,nextOffset>::Add( type *element ) {
	QUEUE_NEXT_PTR(element) = NULL;
	if ( last ) {
		QUEUE_NEXT_PTR(last) = element;
	} else {
		first = element;
	}
	last = element;
}

template< class type, int nextOffset >
type *idQueueTemplate<type,nextOffset>::Get( void ) {
	type *element;

	element = first;
	if ( element ) {
		first = QUEUE_NEXT_PTR(first);
		if ( last == element ) {
			last = NULL;
		}
		QUEUE_NEXT_PTR(element) = NULL;
	}
	return element;
}

/*
===============================================================================

	Raven Queue template

===============================================================================
*/
template<class TYPE, int CAPACITY>
class rvQueue
{
	TYPE			mData[CAPACITY];								// Total capacity of the queue
	int				mNewest;										// Head
	int				mOldest;										// Tail
	int				mSize;											// Size... yes, this could be calculated...

public:
	rvQueue()									{clear();}
	int		size()								{return mSize;}
	bool	empty()								{return mSize==0;}
	bool	full()								{return mSize>=CAPACITY;}
	int		begin()								{return (full())?(mOldest+1):(mOldest);}
	int		end()								{return mNewest-1;}
	void	clear()								{mNewest = mOldest = mSize = 0;}
	bool	valid(int i)						{return ((mNewest>=mOldest)?(i>=mOldest && i<mNewest):(!(i>=mNewest && i<mOldest)));}
	void	inc(int& i)							{i = (i>=(CAPACITY-1))?(0):(i+1);}
	TYPE&	first()								{assert(!empty()); return mData[mOldest];}
	TYPE&	last()								{assert(!empty()); return mData[mNewest-1];}
	TYPE&	operator[] (int i)					{assert(valid(i)); return mData[i];}
	int		push(const TYPE& value)				{assert(!full()); mData[mNewest]=value; inc(mNewest); return (mSize++);}
	int		pop()								{assert(!empty());						inc(mOldest); return (mSize--);}
};

/*
===============================================================================

	Raven Bit Vector Template

===============================================================================
*/
template<int CAPACITY>
class rvBits
{
	enum
	{
		BITS_SHIFT		= 5,										// 5.  Such A Nice Number
		BITS_INT_SIZE	= 32,										// Size Of A Single Word
		BITS_AND		= (BITS_INT_SIZE - 1),						// Used For And Operation
		ARRAY_SIZE		= ((CAPACITY + BITS_AND)/(BITS_INT_SIZE)),	// Num Words Used
		DATA_BYTE_SIZE	= (ARRAY_SIZE*sizeof(unsigned int)),		// Num Bytes Used
	};

	unsigned int	mData[ARRAY_SIZE];

public:
	rvBits()									{clear();}
	void	clear()								{memset(mData, 0, (size_t)(ARRAY_SIZE * sizeof( unsigned int )));}
	bool	valid(const int i)					{return (i>=0 && i<CAPACITY);}
	void	unset(const int i)					{assert(valid(i));			mData[i>>BITS_SHIFT] &= ~(1<<(i&BITS_AND));}
	void	set(const int i)					{assert(valid(i));			mData[i>>BITS_SHIFT] |=  (1<<(i&BITS_AND));}
	bool	operator[](const int i)				{assert(valid(i));	return (mData[i>>BITS_SHIFT] &   (1<<(i&BITS_AND)))!=0;}
};

/*
===============================================================================

	Raven Pool

===============================================================================
*/
template<class TYPE, int CAPACITY>
class rvPool
{
	TYPE			mData[CAPACITY];
	TYPE*			mFree[CAPACITY];
	int				mSize;

public:
	rvPool()									{clear();}
	int		size()								{return mSize;}
	bool	empty()								{return mSize==0;}
	bool	full()								{return mSize>=CAPACITY;}
	void	clear()								{mSize=0; for (int i=0; i<CAPACITY; i++)				mFree[i]=&mData[i];}
	void	free(TYPE* t)						{assert(!empty());	mSize--;							mFree[mSize]=t;}
	TYPE*	alloc()								{assert(!full());	mSize++;							return mFree[mSize-1];}
};


/*
===============================================================================

	Raven Index Pool

===============================================================================
*/
template<class TYPE, int CAPACITY, int INDEXNUM>
class rvIndexPool
{
	TYPE			mData[CAPACITY];
	TYPE*			mFree[CAPACITY];
	TYPE*			mIndx[INDEXNUM];
	int				mSize;

public:
	rvIndexPool()								{clear();}
	int		size()								{return mSize;}
	bool	empty()								{return mSize==0;}
	bool	full()								{return mSize>=CAPACITY;}
	void	clear()								{mSize=0; for (int i=0; i<CAPACITY; i++) mFree[i]=&mData[i]; memset(mIndx, 0, sizeof(mIndx));}
	bool	valid(int i)						{return (i>=0 && i<INDEXNUM && mIndx[i]);}
	TYPE*	operator[](int i)					{assert(valid(i));	return mIndx[i];}
	TYPE*	alloc(int i)						{assert(!full());	mSize++;	mIndx[i]=mFree[mSize-1];	return mFree[mSize-1];}
	void	free(int i)							{assert(valid(i));	mSize--;	mFree[mSize]=mIndx[i];		mIndx[i]=NULL;}
};

#endif /* !__QUEUE_H__ */
