#ifndef __GOOD_THREAD_H__
#define __GOOD_THREAD_H__


#include "good/defines.h"


namespace good
{

    //************************************************************************************************************
    /// Thread handling.
    //************************************************************************************************************
    class thread
    {
    public:
        typedef void (*thread_func_t)(void*);

        /// Sleep current thread for a given amount of time.
        static void sleep( int iMSecs );

        /// Exit current thread.
        static void exit( int iExitCode );

        /// Default constructor.
        thread();

        /// Constructor with func address.
        thread( thread_func_t thread_func );

        /// Destructor. Will terminate thread if it is not a daemon.
        ~thread();

        /// Set function.
        void set_func( thread_func_t thread_func );

        /// Execute thread. TODO: add it to list of threads.
        void launch( void* pThreadParameter, bool bDaemon = true );

        /// Free all handles and memory. Interrupts thread if not daemon.
        // http://stackoverflow.com/questions/14290394/terminatethread-locks-up-when-thread-has-infinate-loop
        void dispose();

        /// Wait for this thread. Return true if thread is terminated.
        bool join( int iMSecs = TIME_INFINITE );

        /// Interrupt thread.
        void terminate();

        /// Check if thread was launched previously.
        bool is_launched();

        /// Check if thread is finished.
        bool is_finished();

    protected:
        void* m_pImpl; // Thread implementation.
    };


} // namespace good


#endif // __GOOD_THREAD_H__
