/*
** linkedlist.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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
**
*/

#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#define EMBEDDEDLIST_UNLINKED ((typename EmbeddedList<T>::Node*)~0)

/**
 * Provides an interface for having a linked list embedded into a class.
 *
 * The class should inherit from EmbeddedList<T>::Node. T can be any type
 * although typically it will be the class. Then, a EmbeddedList<T>::List must
 * exist in order to define the starting point of the chain. Only the head of
 * the list is stored, so new items are pushed to the front of the list.
 *
 * Being that the nodes in the list is embedded into the class, each object can
 * only exist in one list, although there may be many lists the object can be
 * apart of.
 */
template<class T> class EmbeddedList
{
public:
	class Iterator;
	class ConstIterator;
	class List;

	// It was pointed out that this may be more flexible if left as a POD type.
	// Thus the node should not be considered valid until added to a list or
	// ValidateNode has been called on it.
	class Node
	{
	protected:
		friend class EmbeddedList<T>::Iterator;
		friend class EmbeddedList<T>::ConstIterator;
		friend class EmbeddedList<T>::List;

		Node *elNext, *elPrev;
	};

	class Iterator
	{
	public:
		Iterator() : node(NULL) {}
		Iterator(typename EmbeddedList<T>::Node *node) : node(node)
		{
		}
		/**
		 * Initializes an interator pointing to the item before the first item
		 * in the list.
		 */
		Iterator(typename EmbeddedList<T>::List &list) :
			node(reinterpret_cast<typename EmbeddedList<T>::Node *>(&list))
		{
		}

		bool HasNext() { return node->elNext != NULL; }
		bool HasPrev() { return node->elPrev != NULL; }

		Iterator &Next()
		{
			node = node->elNext;
			return *this;
		}
		Iterator &Prev()
		{
			node = node->elPrev;
			return *this;
		}
		Iterator &operator++() { return Next(); }
		Iterator operator++(int) { Iterator copy(*this); Next(); return copy; }
		Iterator &operator--() { return Prev(); }
		Iterator operator--(int) { Iterator copy(*this); Prev(); return copy; }

		T &operator*() const
		{
			return *static_cast<T*>(node);
		}
		T *operator->() const
		{
			return static_cast<T*>(node);
		}

		T *Item() const { return static_cast<T*>(node); }
		T *NextItem() const { return static_cast<T*>(node->elNext); }
		T *PrevItem() const { return static_cast<T*>(node->elPrev); }

		operator T*() const { return static_cast<T*>(node); }
		operator bool() const { return node != NULL; }
	private:
		typename EmbeddedList<T>::Node *node;
	};

	class ConstIterator
	{
	public:
		ConstIterator() : node(NULL) {}
		ConstIterator(const typename EmbeddedList<T>::Node *node) : node(node)
		{
		}
		ConstIterator(const typename EmbeddedList<T>::List &list) :
			node(reinterpret_cast<const typename EmbeddedList<T>::Node *>(&list))
		{
		}

		bool HasNext() { return node->elNext != NULL; }
		bool HasPrev() { return node->elPrev != NULL; }

		ConstIterator &Next()
		{
			node = node->elNext;
			return *this;
		}
		ConstIterator &Prev()
		{
			node = node->elPrev;
			return *this;
		}
		ConstIterator &operator++() { return Next(); }
		ConstIterator operator++(int) { ConstIterator copy(*this); Next(); return copy; }
		ConstIterator &operator--() { return Prev(); }
		ConstIterator operator--(int) { ConstIterator copy(*this); Prev(); return copy; }

		const T &operator*() const
		{
			return *static_cast<const T*>(node);
		}
		const T *operator->() const
		{
			return static_cast<const T*>(node);
		}

		const T *Item() const { return static_cast<const T*>(node); }
		const T *NextItem() const { return static_cast<const T*>(node->elNext); }
		const T *PrevItem() const { return static_cast<const T*>(node->elPrev); }

		operator const T*() const { return static_cast<const T*>(node); }
		operator bool() const { return node != NULL; }
	private:
		const typename EmbeddedList<T>::Node *node;
	};

	class List
	{
	public:
		typedef typename EmbeddedList<T>::Node N;

		List() : head(NULL), size(0)
		{
		}

		typename EmbeddedList<T>::Iterator Head()
		{
			return typename EmbeddedList<T>::Iterator(head);
		}
		typename EmbeddedList<T>::ConstIterator Head() const
		{
			return typename EmbeddedList<T>::ConstIterator(head);
		}

		/**
		 * Returns the tail of the list. Since we don't store the tail, this
		 * function is slow, but is useful for copying lists while preserving
		 * order.
		 */
		typename EmbeddedList<T>::Iterator Tail()
		{
			if(!head)
				return EmbeddedList<T>::Iterator(head);

			Iterator iter(head);
			while(iter.HasNext())
				++iter;

			return iter;
		}
		typename EmbeddedList<T>::ConstIterator Tail() const
		{
			if(!head)
				return EmbeddedList<T>::ConstIterator(head);

			ConstIterator iter(head);
			while(iter.HasNext())
				++iter;

			return iter;
		}

		void Push(N *node)
		{
			++size;

			node->elNext = head;
			node->elPrev = NULL;
			if(head)
				head->elPrev = node;
			head = node;
		}
		void Remove(N *node)
		{
			if(!IsLinked(node))
				return;

			if(node->elNext)
				node->elNext->elPrev = node->elPrev;

			if(node->elPrev)
				node->elPrev->elNext = node->elNext;
			else
				head = node->elNext;

			node->elNext = node->elPrev = EMBEDDEDLIST_UNLINKED;
			--size;
		}

		unsigned int Size() const
		{
			return size;
		}

		/**
		 * Returns true if a node isn linked to some list. This function is not
		 * valid until the node has been linked to some list.
		 *
		 * This function would make sense as a member of the Node, but having
		 * this here prevents name conflicts if an object has more than one
		 * EmbeddedList::Node.
		 */
		static bool IsLinked(const N *node)
		{
			return node->elNext != EMBEDDEDLIST_UNLINKED;
		}

		// Initializes a node.
		static void ValidateNode(N *node)
		{
			node->elNext = node->elPrev = EMBEDDEDLIST_UNLINKED;
		}
	private:
		List(const List &other) {}

		// This should be the first member so we can type pun as Node::elNext.
		typename EmbeddedList<T>::Node *head;
		unsigned int size;
	};
};

/**
 * Implements a container linked list with similar interface as the embedded
 * structures.
 */
template<class T> class LinkedList
{
	class Node
	{
	public:
		Node *elNext, *elPrev;
		T item;

		Node(const T &item, Node *&head) : elNext(head), elPrev(NULL), item(item)
		{
			if(elNext)
				elNext->elPrev = this;
			head = this;
		}
	};

	// Like EmbeddedList, ensure that this can be punned to a Node.
	Node *head;
	unsigned int size;
public:
	class Iterator
	{
	public:
		Iterator() : node(NULL) {}
		Iterator(Node *node) : node(node)
		{
		}
		/**
		 * Initializes an interator pointing to the item before the first item
		 * in the list.
		 */
		Iterator(LinkedList<T> &list) :
			node(reinterpret_cast<typename LinkedList<T>::Node *>(&list))
		{
		}

		bool HasNext() { return node->elNext != NULL; }
		bool HasPrev() { return node->elPrev != NULL; }

		Iterator &Next()
		{
			node = node->elNext;
			return *this;
		}
		Iterator &Prev()
		{
			node = node->elPrev;
			return *this;
		}
		Iterator &operator++() { return Next(); }
		Iterator operator++(int) { Iterator copy(*this); Next(); return copy; }
		Iterator &operator--() { return Prev(); }
		Iterator operator--(int) { Iterator copy(*this); Prev(); return copy; }

		T &operator*() const
		{
			return node->item;
		}
		T *operator->() const
		{
			return &node->item;
		}

		T &Item() const { return node->item; }
		T &NextItem() const { return node->elNext->item; }
		T &PrevItem() const { return node->elPrev->item; }

		operator T&() const { return node->item; }
		operator bool() const { return node != NULL; }
	private:
		friend class LinkedList<T>;

		Node *node;
	};

	class ConstIterator
	{
	public:
		ConstIterator() : node(NULL) {}
		ConstIterator(const Node *node) : node(node)
		{
		}
		ConstIterator(const LinkedList<T> &list) :
			node(reinterpret_cast<const typename LinkedList<T>::Node *>(&list))
		{
		}

		bool HasNext() { return node->elNext != NULL; }
		bool HasPrev() { return node->elPrev != NULL; }

		ConstIterator &Next()
		{
			node = node->elNext;
			return *this;
		}
		ConstIterator &Prev()
		{
			node = node->elPrev;
			return *this;
		}
		ConstIterator &operator++() { return Next(); }
		ConstIterator operator++(int) { ConstIterator copy(*this); Next(); return copy; }
		ConstIterator &operator--() { return Prev(); }
		ConstIterator operator--(int) { ConstIterator copy(*this); Prev(); return copy; }

		const T &operator*() const
		{
			return node->item;
		}
		const T *operator->() const
		{
			return &node->item;
		}

		const T &Item() const { return node->item; }
		const T &NextItem() const { return node->elNext->item; }
		const T &PrevItem() const { return node->elPrev->item; }

		operator const T&() const { return node->item; }
		operator bool() const { return node != NULL; }
	private:
		friend class LinkedList<T>;

		const Node *node;
	};

	LinkedList() : head(NULL), size(0)
	{
	}
	LinkedList(const LinkedList &other) : head(NULL), size(0)
	{
		ConstIterator iter = other.Tail();
		if(iter)
		{
			do
			{
				Push(iter);
			}
			while(--iter);
		}
		assert(Size() == other.Size());
	}
	~LinkedList()
	{
		Clear();
	}

	void Clear()
	{
		Iterator iter = Head();
		if(iter)
		{
			Iterator current;
			do
			{
				current = iter++;
				delete current.node;
			}
			while(iter);
			
		}
	}

	Iterator Head() { return Iterator(head); }
	ConstIterator Head() const { return ConstIterator(head); }
	Iterator Tail()
	{
		if(!head)
			return Iterator(head);

		Iterator iter(head);
		while(iter.HasNext())
			++iter;

		return iter;
	}
	ConstIterator Tail() const
	{
		if(!head)
			return ConstIterator(head);

		ConstIterator iter(head);
		while(iter.HasNext())
			++iter;

		return iter;
	}

	Iterator Push(const T &item)
	{
		++size;
		return Iterator(new Node(item, head));
	}

	void Remove(Iterator pos)
	{
		if(pos.node->elNext)
			pos.node->elNext->elPrev = pos.node->elPrev;

		if(pos.node->elPrev)
			pos.node->elPrev->elNext = pos.node->elNext;
		else
			head = pos.node->elNext;

		delete pos.node;
		--size;
	}

	unsigned int Size() const { return size; }
};

#endif
