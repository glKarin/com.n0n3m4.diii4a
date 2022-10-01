/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/

#ifdef _RAVEN
template< typename type >
class idQueueNode {
public:
	idQueueNode(void) { next = NULL; }

	type* GetNext(void) const { return next; }
	void		SetNext(type* next) { this->next = next; }

private:
	type* next;
};

template< typename type, idQueueNode<type> type::* nodePtr >
class idQueue {
public:
	idQueue(void);

	void		Add(type* element);
	type* RemoveFirst(void);

	static void	Test(void);

private:
	type* first;
	type* last;
};

template< typename type, idQueueNode<type> type::* nodePtr >
idQueue<type, nodePtr>::idQueue(void) {
	first = last = NULL;
}

template< typename type, idQueueNode<type> type::* nodePtr >
void idQueue<type, nodePtr>::Add(type* element) {
	(element->*nodePtr).SetNext(NULL);
	if (last) {
		(last->*nodePtr).SetNext(element);
	}
	else {
		first = element;
	}
	last = element;
}

template< typename type, idQueueNode<type> type::* nodePtr >
type* idQueue<type, nodePtr>::RemoveFirst(void) {
	type* element;

	element = first;
	if (element) {
		first = (first->*nodePtr).GetNext();
		if (last == element) {
			last = NULL;
		}
		(element->*nodePtr).SetNext(NULL);
	}
	return element;
}

template< typename type, idQueueNode<type> type::* nodePtr >
void idQueue<type, nodePtr>::Test(void) {

	class idMyType {
	public:
		idQueueNode<idMyType> queueNode;
	};

	idQueue<idMyType, &idMyType::queueNode> myQueue;

	idMyType* element = new idMyType;
	myQueue.Add(element);
	element = myQueue.RemoveFirst();
	delete element;
}

#else
#define idQueue( type, next )		idQueueTemplate<type, (int)&(((type*)NULL)->next)>
#endif

template< class type, int nextOffset >
class idQueueTemplate
{
	public:
		idQueueTemplate(void);

		void					Add(type *element);
		type 					*Get(void);

	private:
		type 					*first;
		type 					*last;
};

#define QUEUE_NEXT_PTR( element )		(*((type**)(((byte*)element)+nextOffset)))

template< class type, int nextOffset >
idQueueTemplate<type,nextOffset>::idQueueTemplate(void)
{
	first = last = NULL;
}

template< class type, int nextOffset >
void idQueueTemplate<type,nextOffset>::Add(type *element)
{
	QUEUE_NEXT_PTR(element) = NULL;

	if (last) {
		QUEUE_NEXT_PTR(last) = element;
	} else {
		first = element;
	}

	last = element;
}

template< class type, int nextOffset >
type *idQueueTemplate<type,nextOffset>::Get(void)
{
	type *element;

	element = first;

	if (element) {
		first = QUEUE_NEXT_PTR(first);

		if (last == element) {
			last = NULL;
		}

		QUEUE_NEXT_PTR(element) = NULL;
	}

	return element;
}

#ifdef _RAVEN

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

#endif

#endif /* !__QUEUE_H__ */
