#ifndef _KARIN_QUEUE_LIST_H
#define _KARIN_QUEUE_LIST_H

template< class T >
class idQueueList
{
public:
    struct idQueueListNode
    {
        T data;
        idQueueListNode *next;
    };

public:
	idQueueList();
	~idQueueList();

    void                Append( const T &val );
    void				Remove( void );
	bool				IsEmpty( void ) const;
    bool				NotEmpty( void ) const;
	int					Num( void ) const;
	void				Clear( void );
    // raw
    idQueueListNode *   Head( void );
    idQueueListNode *   Take( void );
    // safe
    bool                Get( T &ret );
    bool                Get( T *&ret );
    // unsafe
    T &                 Get( void );
    const T &           Get( void ) const;

#if 0
    // only return first, only for compat with idList
    T &                 operator[](int) {
        return head->data;
    }
    const T &           operator[](int) const {
        return head->data;
    }
    T &                 RemoveIndex(int) {
        Remove();
    }
    void                Resize( int, int ) {}
#endif

private:
    idQueueListNode     *head;
    idQueueListNode     *tail;
};

template< class T >
idQueueList<T>::idQueueList()
{
	head = NULL;
    tail = NULL;
}

template< class T >
idQueueList<T>::~idQueueList()
{
	Clear();
}

template< class T >
ID_INLINE bool idQueueList<T>::IsEmpty( void ) const
{
	return head == NULL;
}

template< class T >
ID_INLINE bool idQueueList<T>::NotEmpty( void ) const
{
    return head != NULL;
}

template< class T >
int idQueueList<T>::Num( void ) const
{
	idQueueListNode*	node;
	int					num;

	num = 0;
	for( node = head; node; node = node->next )
	{
		num++;
	}

	return num;
}

template< class T >
void idQueueList<T>::Clear( void )
{
	idQueueListNode*	node;
	idQueueListNode*	next;

    node = head;
	while( node )
	{
	    next = node->next;
	    delete node;
        node = next;
	}

    head = tail = NULL;
}

template< class T >
ID_INLINE void idQueueList<T>::Remove( void )
{
    if(head)
    {
        idQueueListNode *node = head;
        head = head->next;
        delete node;
    }
}

template< class T >
ID_INLINE bool idQueueList<T>::Get( T &ret )
{
    if(head)
    {
        ret = head->data;
        return true;
    }
    return false;
}

template< class T >
ID_INLINE bool idQueueList<T>::Get( T *&ret )
{
    if(head)
    {
        ret = &head->data;
        return true;
    }
    return false;
}

template< class T >
ID_INLINE T & idQueueList<T>::Get( void )
{
    return head->data;
}

template< class T >
ID_INLINE const T & idQueueList<T>::Get( void ) const
{
    return head->data;
}

template< class T >
typename idQueueList<T>::idQueueListNode * idQueueList<T>::Head( void )
{
    return head;
}

template< class T >
typename idQueueList<T>::idQueueListNode * idQueueList<T>::Take( void )
{
    if(head)
    {
        idQueueListNode *node = head;
        head = head->next;
        return node;
    }
    else
        return NULL;
}

template< class T >
void idQueueList<T>::Append( const T& val )
{
    idQueueListNode *node = new idQueueListNode;
    node->data = val;
    if(!head)
    {
        head = tail = node;
    }
    else
    {
        tail->next = node;
        tail = node;
    }
    tail->next = NULL;
}

#endif /* !_KARIN_QUEUE_LIST_H */
