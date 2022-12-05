
#ifndef __LINKLIST_H__
#define __LINKLIST_H__

/*
==============================================================================

idLinkList

Circular linked list template

==============================================================================
*/

template< class type >
class idLinkList {
public:
						idLinkList();
						~idLinkList();

	bool				IsListEmpty( void ) const;
	bool				InList( void ) const;
	int					Num( void ) const;
	void				Clear( void );

	void				InsertBefore( idLinkList &node );
	void				InsertAfter( idLinkList &node );
	void				AddToEnd( idLinkList &node );
	void				AddToFront( idLinkList &node );

	void				Remove( void );

	type *				Next( void ) const;
	type *				Prev( void ) const;

	type *				Owner( void ) const;
	void				SetOwner( type *object );

	idLinkList *		ListHead( void ) const;
	idLinkList *		NextNode( void ) const;
	idLinkList *		PrevNode( void ) const;

private:
	idLinkList *		head;
	idLinkList *		next;
	idLinkList *		prev;
	type *				owner;
};

/*
================
idLinkList<type>::idLinkList

Node is initialized to be the head of an empty list
================
*/
template< class type >
idLinkList<type>::idLinkList() {
	owner	= NULL;
	head	= this;	
	next	= this;
	prev	= this;
}

/*
================
idLinkList<type>::~idLinkList

Removes the node from the list, or if it's the head of a list, removes
all the nodes from the list.
================
*/
template< class type >
idLinkList<type>::~idLinkList() {
	Clear();
}

/*
================
idLinkList<type>::IsListEmpty

Returns true if the list is empty.
================
*/
template< class type >
// RAVEN BEGIN
// bdube: inlined
ID_INLINE bool idLinkList<type>::IsListEmpty( void ) const {
// RAVEN END
	return head->next == head;
}

/*
================
idLinkList<type>::InList

Returns true if the node is in a list.  If called on the head of a list, will always return false.
================
*/
template< class type >
// RAVEN BEGIN
// bdube: inlined
ID_INLINE bool idLinkList<type>::InList( void ) const {
// RAVEN END
	return head != this;
}

/*
================
idLinkList<type>::Num

Returns the number of nodes in the list.
================
*/
template< class type >
int idLinkList<type>::Num( void ) const {
	idLinkList<type>	*node;
	int					num;

	num = 0;
	for( node = head->next; node != head; node = node->next ) {
		num++;
	}

	return num;
}

/*
================
idLinkList<type>::Clear

If node is the head of the list, clears the list.  Otherwise it just removes the node from the list.
================
*/
template< class type >
void idLinkList<type>::Clear( void ) {
	if ( head == this ) {
		while( next != this ) {
			next->Remove();
		}
	} else {
		Remove();
	}
}

/*
================
idLinkList<type>::Remove

Removes node from list
================
*/
template< class type >
void idLinkList<type>::Remove( void ) {
	prev->next = next;
	next->prev = prev;

	next = this;
	prev = this;
	head = this;
}

/*
================
idLinkList<type>::InsertBefore

Places the node before the existing node in the list.  If the existing node is the head,
then the new node is placed at the end of the list.
================
*/
template< class type >
void idLinkList<type>::InsertBefore( idLinkList &node ) {
	Remove();

	next		= &node;
	prev		= node.prev;
	node.prev	= this;
	prev->next	= this;
	head		= node.head;
}

/*
================
idLinkList<type>::InsertAfter

Places the node after the existing node in the list.  If the existing node is the head,
then the new node is placed at the beginning of the list.
================
*/
template< class type >
void idLinkList<type>::InsertAfter( idLinkList &node ) {
	Remove();

	prev		= &node;
	next		= node.next;
	node.next	= this;
	next->prev	= this;
	head		= node.head;
}

/*
================
idLinkList<type>::AddToEnd

Adds node at the end of the list
================
*/
template< class type >
// RAVEN BEGIN
// bdube: inlined
ID_INLINE void idLinkList<type>::AddToEnd( idLinkList &node ) {
// RAVEN END
	InsertBefore( *node.head );
}

/*
================
idLinkList<type>::AddToFront

Adds node at the beginning of the list
================
*/
template< class type >
// RAVEN BEGIN
// bdube: inlined
ID_INLINE void idLinkList<type>::AddToFront( idLinkList &node ) {
// RAVEN END
	InsertAfter( *node.head );
}

/*
================
idLinkList<type>::ListHead

Returns the head of the list.  If the node isn't in a list, it returns
a pointer to itself.
================
*/
template< class type >
// RAVEN BEGIN
// bdube: inlined
ID_INLINE idLinkList<type> *idLinkList<type>::ListHead( void ) const {
// RAVEN END
	return head;
}

/*
================
idLinkList<type>::Next

Returns the next object in the list, or NULL if at the end.
================
*/
template< class type >
// RAVEN BEGIN
// bdube: inlined
ID_INLINE type *idLinkList<type>::Next( void ) const {
// RAVEN END
	if ( !next || ( next == head ) ) {
		return NULL;
	}
	return next->owner;
}

/*
================
idLinkList<type>::Prev

Returns the previous object in the list, or NULL if at the beginning.
================
*/
template< class type >
type *idLinkList<type>::Prev( void ) const {
	if ( !prev || ( prev == head ) ) {
		return NULL;
	}
	return prev->owner;
}

/*
================
idLinkList<type>::NextNode

Returns the next node in the list, or NULL if at the end.
================
*/
template< class type >
idLinkList<type> *idLinkList<type>::NextNode( void ) const {
	if ( next == head ) {
		return NULL;
	}
	return next;
}

/*
================
idLinkList<type>::PrevNode

Returns the previous node in the list, or NULL if at the beginning.
================
*/
template< class type >
idLinkList<type> *idLinkList<type>::PrevNode( void ) const {
	if ( prev == head ) {
		return NULL;
	}
	return prev;
}

/*
================
idLinkList<type>::Owner

Gets the object that is associated with this node.
================
*/
template< class type >
type *idLinkList<type>::Owner( void ) const {
	return owner;
}

/*
================
idLinkList<type>::SetOwner

Sets the object that this node is associated with.
================
*/
template< class type >
void idLinkList<type>::SetOwner( type *object ) {
	owner = object;
}

#endif /* !__LINKLIST_H__ */
