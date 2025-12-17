#ifndef __GOOD_MUTEX_H__
#define __GOOD_MUTEX_H__

#ifndef WIN32
    #include <pthread.h>
#endif


namespace good
{

    //************************************************************************************************************
    /// Mutex class.
    //************************************************************************************************************
    class mutex
    {
    public:
        /// Constructor.
        mutex();

        /// Destructor.
        virtual ~mutex();

        /// Lock mutex.
        void lock();

        /// Try to lock mutex, return true if could lock.
        bool try_lock();

        /// Unlock mutex.
        void unlock();

    protected:
        mutex( const mutex& cOther ); ///< Deny copy constructor.

#ifdef _WIN32
        void *m_hMutex; ///< Mutex handle.
#else
        pthread_mutex_t m_cMutex; ///< Mutex handle.
#endif
    };

} // namespace good


#endif // __GOOD_MUTEX_H__
