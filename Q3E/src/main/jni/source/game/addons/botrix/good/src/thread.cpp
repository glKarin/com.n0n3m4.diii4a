#ifdef _WIN32

#include <windows.h>

#include "good/thread.h"

namespace good
{

    DWORD WINAPI thread_impl_thread_proc(LPVOID lpThreadParameter); // Forward declaration.

    //----------------------------------------------------------------------------------------------------------------
    // Thread implementation.
    //----------------------------------------------------------------------------------------------------------------
    class thread_impl
    {
    public:
        thread_impl(good::thread::thread_func_t thread_func): m_hThread(NULL), m_pThreadFunc(thread_func) {}
        ~thread_impl()
        {
            dispose();
        }

        /// Set function.
        inline void set_func( thread::thread_func_t thread_func ) { m_pThreadFunc = thread_func; }

        // Execute thread.
        inline void launch( void* pThreadParameter, bool bDaemon = false )
        {
            GoodAssert( m_hThread == NULL );
            m_bDaemon = bDaemon;
            m_pThreadParameter = pThreadParameter;
            m_hThread = CreateThread(NULL, 0, thread_impl_thread_proc, (LPVOID)this, 0, NULL);
            GoodAssert(m_hThread);
        }

        /// Free all handles and memory. Terminate thread if not daemon.
        inline void dispose()
        {
            if ( m_hThread )
            {
                // TODO: interrupt thread.
                CloseHandle(m_hThread);
                m_hThread = NULL;
            }
        }

        // Wait for this thread.  Return true if thread is terminated.
        inline bool join( int iMSecs )
        {
            GoodAssert( m_hThread );
            return WaitForSingleObject(m_hThread, iMSecs) == WAIT_OBJECT_0;
        }

        // Terminate thread.
        inline void terminate()
        {
            GoodAssert( m_hThread );
            TerminateThread(m_hThread, 1);
        }

        // Check if thread was launched previously.
        inline bool is_launched() { return m_hThread != NULL; }

        // Check if thread is finished.
        inline bool is_finished()
        {
            GoodAssert( m_hThread );
            DWORD iExitCode = 0;
            GetExitCodeThread(m_hThread, &iExitCode); // TODO: Handle error.
            return iExitCode != STILL_ACTIVE;
        }

    protected:
        friend DWORD WINAPI thread_impl_thread_proc(LPVOID lpThreadParameter);

        bool m_bDaemon;
        HANDLE m_hThread;
        good::thread::thread_func_t m_pThreadFunc;
        LPVOID m_pThreadParameter;
    };


    //----------------------------------------------------------------------------------------------------------------
    // Thread functions.
    //----------------------------------------------------------------------------------------------------------------
    void thread::sleep( int iMSecs )
    {
        Sleep(iMSecs);
    }

    //----------------------------------------------------------------------------------------------------------------
    void thread::exit( int iExitCode )
    {
        ExitThread(iExitCode);
    }

    //----------------------------------------------------------------------------------------------------------------
    thread::thread()
    {
        m_pImpl = new thread_impl( NULL );
    }

    thread::thread( thread_func_t thread_func )
    {
        m_pImpl = new thread_impl( thread_func );
    }

    //----------------------------------------------------------------------------------------------------------------
    thread::~thread()
    {
        delete (thread_impl*)m_pImpl;
    }

    //----------------------------------------------------------------------------------------------------------------
    void thread::set_func( thread_func_t thread_func )
    {
        ((thread_impl*)m_pImpl)->set_func(thread_func);
    }

    //----------------------------------------------------------------------------------------------------------------
    void thread::launch( void* pThreadParameter, bool bDaemon )
    {
        ((thread_impl*)m_pImpl)->launch(pThreadParameter, bDaemon);
    }

    //----------------------------------------------------------------------------------------------------------------
    void thread::terminate()
    {
        ((thread_impl*)m_pImpl)->terminate();
    }

    //----------------------------------------------------------------------------------------------------------------
    void thread::dispose()
    {
        ((thread_impl*)m_pImpl)->dispose();
    }

    //----------------------------------------------------------------------------------------------------------------
    bool thread::join( int iMSecs )
    {
        return ((thread_impl*)m_pImpl)->join(iMSecs);
    }

    //----------------------------------------------------------------------------------------------------------------
    bool thread::is_launched()
    {
        return ((thread_impl*)m_pImpl)->is_launched();
    }

    //----------------------------------------------------------------------------------------------------------------
    bool thread::is_finished()
    {
        return ((thread_impl*)m_pImpl)->is_finished();
    }


    //----------------------------------------------------------------------------------------------------------------
    // Thread function.
    //----------------------------------------------------------------------------------------------------------------
    DWORD WINAPI thread_impl_thread_proc(LPVOID lpThreadParameter)
    {
        good::thread_impl* pImpl = (good::thread_impl*)lpThreadParameter;
        GoodAssert( pImpl->m_pThreadFunc );
        pImpl->m_pThreadFunc(pImpl->m_pThreadParameter);
        ExitThread(0);
        //return 0;
    }

} // namespace good



#else // _WIN32



///** @brief Clase que representa un hilo de ejecucion. */
//class CThread
//{
//public:
//    /// Destructor virtual.
//    virtual ~CThread() {}

//    /// Funcion abstracta que se ejecuta en un hilo nuevo.
//    virtual void run() = 0;

//    /**
//     * @brief Ejecuta el metodo CThread::run() en un hilo nuevo.
//     * @return resultado de ejecucion.
//     */
//    int start()
//    {
//        return pthread_create(&thread, NULL, CThread::thread_func, (void*)this);
//    }

//    /**
//     * @brief Trata de esperar al hilo que se esta ejecutando.
//     * @return false, en el caso que la espera falle.
//     */
//    bool join()
//    {
//        return pthread_join(thread, NULL) == 0;
//    }

//protected:
//    pthread_t thread; ///< Hilo pthread.

//    //CThread( const CThread& copy ); // Denegar constructor por copia.

//    /// Funcion del hilo para creacion usando libreria pthread.
//    static void *thread_func( void *d )
//    {
//        static_cast<CThread*>(d)->run();
//        return NULL;
//    }

//};

#endif // _WIN32
