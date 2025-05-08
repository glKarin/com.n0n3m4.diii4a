#ifndef _KARIN_QUEUE_LIST_H
#define _KARIN_QUEUE_LIST_H

#define _QUEUE_LIST_RECYCLE 1
#if 1
#define _QUEUE_LIST_DEBUG(x)
#else
#define _QUEUE_LIST_DEBUG(x) do { printf(#x " -> "); PrintInfo(); } while(0)
#endif

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
#ifdef _QUEUE_LIST_RECYCLE
    explicit idQueueList( bool recycle = false );
#else
	idQueueList();
#endif
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
#ifdef _QUEUE_LIST_RECYCLE
	void				SetRecycle( bool recycle );
#endif

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
    void                FreeNode( idQueueListNode* node );
    idQueueListNode*    AllocNode( void );
    void                PrintInfo( void );
    void                AppendNode( idQueueListNode* node );
	void				Clean( void );

private:
    idQueueListNode     *head;
    idQueueListNode     *tail;
#ifdef _QUEUE_LIST_RECYCLE
    idQueueList         *recycleList;
#endif
};

template< class T >
#ifdef _QUEUE_LIST_RECYCLE
idQueueList<T>::idQueueList( bool recycle )
#else
idQueueList<T>::idQueueList()
#endif
{
	head = NULL;
    tail = NULL;
#ifdef _QUEUE_LIST_RECYCLE
    if(recycle)
        recycleList = new idQueueList(false);
	else
		recycleList = NULL;
#endif
}

template< class T >
idQueueList<T>::~idQueueList()
{
	Clean();
#ifdef _QUEUE_LIST_RECYCLE
    if(recycleList)
        delete recycleList;
#endif
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
        FreeNode(node);
        node = next;
	}

    head = tail = NULL;
}

template< class T >
void idQueueList<T>::Clean( void )
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

#ifdef _QUEUE_LIST_RECYCLE
    if(recycleList)
        recycleList->Clean();
#endif
}

template< class T >
ID_INLINE void idQueueList<T>::FreeNode( idQueueListNode* node )
{
#ifdef _QUEUE_LIST_RECYCLE
    if(recycleList)
        recycleList->AppendNode(node);
    else
#endif
    delete node;
}

template< class T >
ID_INLINE typename idQueueList<T>::idQueueListNode * idQueueList<T>::AllocNode( void )
{
#ifdef _QUEUE_LIST_RECYCLE
    if(recycleList)
    {
        idQueueListNode *node = recycleList->Take();
        if(node)
            return node;
    }
#endif
    return new idQueueListNode;
}

template< class T >
ID_INLINE void idQueueList<T>::AppendNode( idQueueListNode* node )
{
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

template< class T >
ID_INLINE void idQueueList<T>::Remove( void )
{
    if(head)
    {
        idQueueListNode *node = head;
        head = head->next;
		if(!head)
			tail = NULL;
        FreeNode(node);
        _QUEUE_LIST_DEBUG(Remove);
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
    _QUEUE_LIST_DEBUG(Get);
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
		if(!head)
			tail = NULL;
        return node;
    }
    else
        return NULL;
}

template< class T >
void idQueueList<T>::Append( const T& val )
{
    idQueueListNode *node = AllocNode();
	node->data = val;
    AppendNode(node);
    _QUEUE_LIST_DEBUG(Append);
}

#ifdef _QUEUE_LIST_RECYCLE
template< class T >
void idQueueList<T>::SetRecycle( bool recycle )
{
	if(recycle)
	{
		if(!recycleList)
			recycleList = new idQueueList(false);
	}
	else
	{
		if(recycleList)
		{
			delete recycleList;
			recycleList = NULL;
		}
	}
}
#endif

template< class T >
void idQueueList<T>::PrintInfo( void )
{
#ifdef _QUEUE_LIST_RECYCLE
    if(recycleList)
        printf("idQueueList::%p -> num=%d, head=%p, tail=%p. recycle=%p(num=%d, head=%p, tail=%p)\n", this, Num(), head, tail, recycleList, recycleList->Num(), recycleList->head, recycleList->tail);
    else
#endif
    printf("idQueueList::%p -> num=%d, head=%p, tail=%p\n", this, Num(), head, tail);
}

#endif /* !_KARIN_QUEUE_LIST_H */
