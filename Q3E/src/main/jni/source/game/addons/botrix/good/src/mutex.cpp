#include <string.h>
#ifdef _WIN32
    #include "windows.h"
#endif


#include "good/defines.h"
#include "good/mutex.h"


namespace good
{


#ifdef _WIN32
    mutex::mutex()
    {
        m_hMutex = CreateMutex(NULL, FALSE, NULL); // Default security attibutes, initially not owned, mutex not named.
        GoodAssert(m_hMutex);
    }

    mutex::~mutex()
    {
        CloseHandle(m_hMutex);
    }

    void mutex::lock()
    {
        WaitForSingleObject(m_hMutex, INFINITE);
    }

    bool mutex::try_lock()
    {
        DWORD iResult = WaitForSingleObject(m_hMutex, 0);
        return (iResult == WAIT_ABANDONED) || (iResult == WAIT_OBJECT_0);
    }

    void mutex::unlock()
    {
        ReleaseMutex(m_hMutex);
    }

#else // _WIN32

    mutex::mutex() { memset(&m_cMutex, 0, sizeof(m_cMutex)); }

    mutex::~mutex() {}

    void mutex::lock()
    {
        pthread_mutex_lock( &m_cMutex );
    }

    bool mutex::try_lock()
    {
        return pthread_mutex_trylock( &m_cMutex ) == 0;
    }

    void mutex::unlock()
    {
        pthread_mutex_unlock( &m_cMutex );
    }

#endif // _WIN32


} // namespace good
