/*
Copyright (C) 2020-2021 David Knapp (Cloudwalk)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// com_list.h - generic doubly linked list interface, adapted from Linux list.h

#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include "qtypes.h"
#include "qdefs.h"

typedef struct llist_s
{
	struct llist_s *prev;
	struct llist_s *next;
} llist_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct llist_s name = LIST_HEAD_INIT(name)

/*
 * Get the struct for this entry
 */
#define List_Entry(ptr, type, member) ContainerOf(ptr, type, member)

/*
 * Get the first element from a list
 * The list is expected to not be empty
 */
#define List_First_Entry(ptr, type, member) List_Entry((ptr)->next, type, member)

/*
 * Get the last element from the list
 * The list is expected to not be empty
 */
#define List_Last_Entry(ptr, type, member) List_Entry((ptr)->prev, type, member)

/*
 * Get the first element from the list, but return NULL if it's empty
 */
#define List_First_Entry_Or_Null(ptr, type, member) ({ \
	struct llist_s *head__ = (ptr); \
	struct llist_s *pos__ = head__->next; \
	pos__ != head__ ? List_Entry(pos__, type, member) : NULL; \
})

/*
 * Get the next element in the list
 */
#define List_Next_Entry(pos, type, member) \
	List_Entry((pos)->member.next, type, member)

/*
 * Get the prev element in the list
 */
#define List_Prev_Entry(pos, type, member) \
	List_Entry((pos)->member.prev, type, member)

/*
 * Iterate over a list
 */
#define List_For_Each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/*
 * Continue iteration over a list, after the current position
 */
#define List_For_Each_Continue(pos, head) \
	for (pos = pos->next; pos != (head); pos = pos->next)

/*
 * Iterate over a list backwards
 */
#define List_For_Each_Prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/*
 * Iterate over a list, safe against removal of list entry
 */
#define List_For_Each_Safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
	     pos = n, n = pos->next)

/*
 * Iterate over a list backwards, safe against removal of list entry
 */
#define List_For_Each_Prev_Safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/*
 * Test if the entry points to the head of the list
 */
#define List_Entry_Is_Head(pos, head, member) \
	(&pos->member == (head))

/*
 * Iterate over a list of a given type
 */
#define List_For_Each_Entry(pos, head, type, member) \
	for (pos = List_First_Entry(head, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = List_Next_Entry(pos, type, member))

/*
 * Iterate over a list of a given type backwards
 */
#define List_For_Each_Prev_Entry(pos, head, type, member) \
	for (pos = List_Last_Entry(head, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = List_Prev_Entry(pos, type, member))

/*
 * Prepares a pos entry for use as a start point in List_For_Each_Entry_Continue()
 */
#define List_Prepare_Entry(pos, head, type, member) \
	((pos) ? : List_Entry(head, type, member))

/*
 * Continue iteration over a list of a given type, after the current position
 */
#define List_For_Each_Entry_Continue(pos, head, type, member) \
	for (pos = List_Next_Entry(pos, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = List_Next_Entry(pos, type, member))

/*
 * Continue iteration over a list of a given type backwards, after the current position
 */
#define List_For_Each_Prev_Entry_Continue(pos, head, type, member) \
	for (pos = List_Prev_Entry(pos, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = List_Prev_Entry(pos, type, member))

/*
 * Continue iteration over a list of a given type, from the current position
 */
#define List_For_Each_Entry_From(pos, head, type, member) \
	for (; !List_Entry_Is_Head(pos, head, member); \
	     pos = List_Next_Entry(pos, type, member))

/*
 * Continue iteration over a list of a given type backwards, from the current position
 */
#define List_For_Each_Prev_Entry_From(pos, head, type, member) \
	for (; !List_Entry_Is_Head(pos, head, member); \
	     pos = List_Prev_Entry(pos, type, member))

/*
 * Iterate over a list of a given type, safe against removal of list entry
 */
#define List_For_Each_Entry_Safe(pos, n, head, type, member) \
	for (pos = List_First_Entry(head, type, member), \
	     n = List_Next_Entry(pos, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = n, n = List_Next_Entry(n, type, member))

/*
 * Continue iteration over a list of a given type, after the current position, safe against removal of list entry
 */
#define List_For_Each_Entry_Safe_Continue(pos, n, head, type, member) \
	for (pos = List_Next_Entry(pos, type, member), \
	     n = List_Next_Entry(pos, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = n, n = List_Next_Entry(n, type, member))

/*
 * Continue iteration over a list of a given type, from the current position, safe against removal of list entry
 */
#define List_For_Each_Entry_Safe_From(pos, n, head, type, member) \
	for (n = List_Next_Entry(pos, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = n, n = List_Next_Entry(n, type, member))

/*
 * Iterate over a list of a given type backwards, safe against removal of list entry
 */
#define List_For_Each_Prev_Entry_Safe(pos, n, head, type, member) \
	for (pos = List_Last_Entry(head, type, member), \
	     n = List_Prev_Entry(pos, type, member); \
	     !List_Entry_Is_Head(pos, head, member); \
	     pos = n, n = List_Prev_Entry(n, type, member))

/*
 * Reset a stale List_For_Each_Entry_Safe loop
 */
#define List_Safe_Reset_Next(pos, n, type, member) \
	n = List_Next_Entry(pos, type, member)

static inline qbool List_Is_Empty(const llist_t *list)
{
	return list->next == list;
}

/*
 * Creates a new linked list. Initializes the head to point to itself.
 * If it's a list header, the result is an empty list.
 */
static inline void List_Create(llist_t *list)
{
	list->next = list->prev = list;
}

/*
 * Insert a node between two known nodes.
 * 
 * Only use when prev and next are known.
 */
static inline void __List_Add(llist_t *node, llist_t *prev, llist_t *next)
{
	next->prev = node;
	node->next = next;
	node->prev = prev;
	prev->next = node;
}

/*
 * Insert a node immediately after head.
 */
static inline void List_Add(llist_t *node, llist_t *head)
{
	__List_Add(node, head, head->next);
}

/*
 * Insert a node immediately before head.
 */
static inline void List_Add_Tail(llist_t *node, llist_t *head)
{
	__List_Add(node, head->prev, head);
}

/*
 * Bridge prev and next together, when removing the parent of them.
 */
static inline void __List_Delete(llist_t *prev, llist_t *next)
{
	next->prev = prev;
	prev->next = next;
}

/*
 * Redundant?
 */
static inline void __List_Delete_Node(llist_t *node)
{
	__List_Delete(node->prev, node->next);
}

/*
 * Removes a node from its list. Sets its pointers to NULL.
 */
static inline void List_Delete(llist_t *node)
{
	__List_Delete_Node(node);
	node->next = node->prev = NULL;
}

/*
 * Removes a node from its list. Reinitialize it.
 */
static inline void List_Delete_Init(llist_t *node)
{
	__List_Delete_Node(node);
	node->next = node->prev = node;
}

/*
 * Replace old with new. Old is overwritten if empty.
 */
static inline void List_Replace(llist_t *old, llist_t *_new)
{
	_new->next = old->next;
	_new->next->prev = _new;
	_new->prev = old->prev;
	_new->prev->next = _new;
	old->next = old->prev = old;
}

/*
 * Replace old with new. Initialize old.
 * Old is overwritten if empty.
 */
static inline void List_Replace_Init(llist_t *old, llist_t *_new)
{
	List_Replace(old, _new);
	List_Create(old);
}

/*
 * Swap node1 with node2 in place.
 */
static inline void List_Swap(llist_t *node1, llist_t *node2)
{
	llist_t *pos = node2->prev;
	List_Delete_Init(node2);
	List_Replace(node1, node2);
	if(pos == node1)
		pos = node2;
	List_Add(node1, pos);
}

/*
 * Delete list from its... list, then insert after head.
 */
static inline void List_Move(llist_t *list, llist_t *head)
{
	__List_Delete_Node(list);
	List_Add(list, head);
}

/*
 * Delete list from its... list, then insert before head.
 */
static inline void List_Move_Tail(llist_t *list, llist_t *head)
{
	__List_Delete_Node(list);
	List_Add_Tail(list, head);
}

/*
 * Move the first node of a range of nodes immediately after head.
 * All three parameters must belong to the same list.
 */

static inline void List_Bulk_Move_Tail(llist_t *head, llist_t *first, llist_t *last)
{
	first->prev->next = last->next;
	last->next->prev = first->prev;

	head->prev->next = first;
	first->prev = head->prev;

	last->next = head;
	head->prev = last;
}

/*
 * Shift the head to the right (like rotating a wheel counterclockwise).
 * The node immediately to the right becomes the new head.
 */
static inline void List_Rotate_Left(llist_t *head)
{
	llist_t *first;

	if (!List_Is_Empty(head))
	{
		first = head->next;
		List_Move_Tail(first, head);
	}
}

/*
 * Make list the new head.
 */
static inline void List_Rotate_To_Front(llist_t *list, llist_t *head)
{
	List_Move_Tail(head, list);
}

/*
 * Concatenate two lists. The head of list will be discarded.
 */
static inline void __List_Splice(const llist_t *list, llist_t *prev, llist_t *next)
{
	llist_t *first = list->next;
	llist_t *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/*
 * Concatenate two lists. The first node of list will be inserted after head.
 */
static inline void List_Splice(const llist_t *list, llist_t *head)
{
	if(!List_Is_Empty(list))
		__List_Splice(list, head, head->next);
}

/*
 * Concatenate two lists. The tail of list will be inserted before head.
 */
static inline void List_Splice_Tail(const llist_t *list, llist_t *head)
{
	if (!List_Is_Empty(list))
		__List_Splice(list, head->prev, head);
}

static inline qbool List_Is_First(llist_t *list, llist_t *start)
{
	return list->prev == start;
}

static inline qbool List_Is_Last(llist_t *list, llist_t *start)
{
	return list->next == start;
}

#endif
