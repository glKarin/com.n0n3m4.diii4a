#ifndef __GOOD_MEMORY_H__
#define __GOOD_MEMORY_H__


#include <stdlib.h> // malloc

#include "good/defines.h"


namespace good
{


//************************************************************************************************************
/** @brief Class for contruct/destroy and allocating/deallocating objects.
 *
 * Contains functions to allocate/deallocate enough space to contain some objects of type T. Note that
 * constructors/destructors of that objects are not being called when allocated/deallocated.
 */
//************************************************************************************************************
template <typename T>
class allocator
{

public:
    typedef T value_type;
    typedef T* pointer_t;
    typedef T& reference_t;
    typedef const T& const_reference_t;

    template <class _Other>
    struct rebind
    {
        typedef allocator<_Other> other;
    };

    //--------------------------------------------------------------------------------------------------------
    /// Call copy constructor for object allocated in memory 'where'.
    //--------------------------------------------------------------------------------------------------------
    void construct(pointer_t pWhere, const_reference_t tCopy) const
    {
        new (pWhere) T(tCopy);
    }

    //--------------------------------------------------------------------------------------------------------
    /// Call destructor for object allocated in memory at location pWhere.
    //--------------------------------------------------------------------------------------------------------
    void destroy(pointer_t pWhere) const
    {
        (void)pWhere; // To remove ugly VS warning 4100.
        pWhere->~T();
    }

    //--------------------------------------------------------------------------------------------------------
    /// Allocate memory for @p iSize objects of type T.
    //--------------------------------------------------------------------------------------------------------
    pointer_t allocate( int iSize ) const
    {
        return (pointer_t)malloc( iSize * sizeof(T) );
    }

    //--------------------------------------------------------------------------------------------------------
    /// Reallocate memory for iNewCount objects of type T.
    //--------------------------------------------------------------------------------------------------------
    pointer_t reallocate( pointer_t pOld, int iNewSize, int /*iOldSize*/ ) const
    {
        return (pointer_t)realloc( pOld, iNewSize * sizeof(T) );
    }

    //--------------------------------------------------------------------------------------------------------
    /// Deallocate memory pointing to an object of type T.
    //--------------------------------------------------------------------------------------------------------
    void deallocate( pointer_t pPtr, int /*iSize*/ = 0 ) const
    {
        free(pPtr);
    }
};


//************************************************************************************************************
/// Class that holds pointer that is erased at destructor.
//************************************************************************************************************
template < typename T >
class unique_ptr
{

public:
    //--------------------------------------------------------------------------------------------------------
    /// Default contructor (null pointer).
    //--------------------------------------------------------------------------------------------------------
    unique_ptr(): m_pPtr(NULL) {}

    //--------------------------------------------------------------------------------------------------------
    /// Contructor by copy, other looses pointer.
    //--------------------------------------------------------------------------------------------------------
    unique_ptr(const unique_ptr& other): m_pPtr(other.m_pPtr) { ((unique_ptr&)(other)).m_pPtr = NULL; }

    //--------------------------------------------------------------------------------------------------------
    /// Contructor using existing pointer.
    //--------------------------------------------------------------------------------------------------------
    unique_ptr(T* p): m_pPtr(p) {}

    //--------------------------------------------------------------------------------------------------------
    /// Destructor. If decrementing m_iCounter gives 0, will free current pointer.
    //--------------------------------------------------------------------------------------------------------
    ~unique_ptr() { reset(NULL); }

    //--------------------------------------------------------------------------------------------------------
    /// Get true if current pointer is not NULL.
    //--------------------------------------------------------------------------------------------------------
    operator bool() const { return (m_pPtr); }

    //--------------------------------------------------------------------------------------------------------
    /// Get current pointer. Should be used only for checking for NULL.
    //--------------------------------------------------------------------------------------------------------
    T* get() { return m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Get current pointer. Should be used only for checking for NULL.
    //--------------------------------------------------------------------------------------------------------
    const T* get() const { return m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Reset current pointer.
    //--------------------------------------------------------------------------------------------------------
    void reset( T* p = NULL )
    {
        if (m_pPtr)
            delete m_pPtr;
        m_pPtr = p;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
    //--------------------------------------------------------------------------------------------------------
    T* operator->() { GoodAssert(m_pPtr); return m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
    //--------------------------------------------------------------------------------------------------------
    const T* operator->() const { GoodAssert(m_pPtr); return m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Copy operator.
    //--------------------------------------------------------------------------------------------------------
    unique_ptr& operator=( T* p )
    {
        reset( p );
        return *this;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Copy operator.
    //--------------------------------------------------------------------------------------------------------
    unique_ptr& operator=( const unique_ptr& other )
    {
        reset( other.m_pPtr );
        ((unique_ptr&)(other)).m_pPtr = NULL;
        return *this;
    }

protected:
    T* m_pPtr;
};


//************************************************************************************************************
/// Class that holds pointer shared among application. Not thread-safe.
//************************************************************************************************************
template < typename T, typename Alloc = good::allocator<int> >
class shared_ptr
{
public:
    typedef T* pointer_t;
    typedef const T* const_pointer_t;
    typedef T& reference_t;
    typedef const T& const_reference_t;
    typedef int* counter_t;

    //--------------------------------------------------------------------------------------------------------
    /// Default contructor (null pointer).
    //--------------------------------------------------------------------------------------------------------
    shared_ptr(): m_iCounter(0), m_pPtr(NULL) {}

    //--------------------------------------------------------------------------------------------------------
    /// Contructor by copy another shared_p. Will increment m_iCounter.
    //--------------------------------------------------------------------------------------------------------
    shared_ptr(const shared_ptr& ptr): m_iCounter(ptr.m_iCounter), m_pPtr(ptr.m_pPtr)
    {
        if (m_iCounter)
            ++(*m_iCounter);
    }

    //--------------------------------------------------------------------------------------------------------
    /// Contructor using existing pointer.
    //--------------------------------------------------------------------------------------------------------
    shared_ptr(pointer_t ptr): m_pPtr(ptr)
    {
        m_iCounter = m_cAlloc.allocate(1);
        *m_iCounter = 1;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Destructor. If decrementing m_iCounter gives 0, will free current pointer.
    //--------------------------------------------------------------------------------------------------------
    ~shared_ptr()
    {
        reset(NULL, NULL);
    }

    //--------------------------------------------------------------------------------------------------------
    /// Get true if current pointer is not NULL.
    //--------------------------------------------------------------------------------------------------------
    operator bool() const { return (m_pPtr != NULL); }

    //--------------------------------------------------------------------------------------------------------
    /// Get current pointer. Should be used only for checking for NULL.
    //--------------------------------------------------------------------------------------------------------
    pointer_t get() { return m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Get current pointer. Should be used only for checking for NULL.
    //--------------------------------------------------------------------------------------------------------
    const_pointer_t get() const { return m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Reset current pointer.
    //--------------------------------------------------------------------------------------------------------
    void reset( pointer_t ptr = NULL, counter_t c = NULL )
    {
        if (m_iCounter)
        {
            GoodAssert(m_pPtr);
            if ( (--(*m_iCounter)) == 0 )
            {
                m_cAlloc.deallocate(m_iCounter, 1);
                delete m_pPtr;
            }
        }
        m_pPtr = ptr;
        m_iCounter = c;
    }

    //--------------------------------------------------------------------------------------------------------
    /// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
    //--------------------------------------------------------------------------------------------------------
    reference_t operator*() const { GoodAssert(m_pPtr); return *m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Perform operation on pointer. Assertion is used to ensure that pointer is valid.
    //--------------------------------------------------------------------------------------------------------
    pointer_t operator->() const { GoodAssert(m_pPtr); return m_pPtr; }

    //--------------------------------------------------------------------------------------------------------
    /// Copy operator.
    //--------------------------------------------------------------------------------------------------------
    shared_ptr& operator=( pointer_t ptr )
    {
        reset( ptr, m_cAlloc.allocate(1) );
        return *this;
    }
    //--------------------------------------------------------------------------------------------------------
    /// Copy operator.
    //--------------------------------------------------------------------------------------------------------
    shared_ptr& operator=( const shared_ptr& ptr )
    {
        reset( ptr.m_pPtr, ptr.m_iCounter );
        if (m_iCounter)
            ++(*m_iCounter);
        return *this;
    }

protected:
    Alloc m_cAlloc;           // Allocator class.
    counter_t m_iCounter;     // Shared m_iCounter.
    pointer_t m_pPtr;         // Pointer himself.
};


} // namespace good


#endif // __GOOD_MEMORY_H__
