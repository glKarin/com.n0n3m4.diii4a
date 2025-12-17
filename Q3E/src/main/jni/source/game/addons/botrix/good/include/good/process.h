#ifndef __GOOD_PROCESS_H__
#define __GOOD_PROCESS_H__


#include "good/string.h"


namespace good
{

    //************************************************************************************************************
    /// Process handling.
    //************************************************************************************************************
    class process
    {
    public:
        /// Exit current process.
        static void exit( int iExitCode );

        /// Default constructor.
        process();

        /// Constructor with params.
        process( const good::string& sExe, const good::string& sCmd, bool bRedirect = false, bool bChangeWorkingDir = false );

        /// Destructor. Will terminate process if it is not a daemon.
        virtual ~process();

        /// Set process parameters. Redirect if you are planning to use write_stdin() / read_stdout() / read_stderr().
        void set_params( const good::string& sExe, const good::string& sCmd, bool bRedirect = false, bool bChangeWorkingDir = false );

        /// Execute process.
        bool launch( bool bShowProcessWindow = true, bool bDaemon = true );

        /// Join a process for a given time. Return true if process is terminated.
        bool join( int iMSecs = TIME_INFINITE );

        /// Terminate process.
        void terminate();

        /// Free all handles and memory. Terminate process if not daemon.
        void dispose();

        /// Check if process is launched.
        bool is_launched();

        /// Check if process is running.
        inline bool is_running() { return !is_finished(); }

        /// Check if process is finished.
        bool is_finished();

        /// Write stdin for the process.
        bool write_stdin( void* pBuffer, int iSize );

        /// Close stdin for the process.
        void close_stdin();

        /// Return true if child has data on stdout.
        bool has_data_stdout();

        /// Read output of the process, returning false, if process exited. This function is blocking.
        bool read_stdout( void* pBuffer, int iMaxSize, int& iReadSize );

        /// Return true if child has data on stderr.
        bool has_data_stderr();

        /// Read error handle of the process, returning false, if process exited. This function is blocking.
        bool read_stderr( void* pBuffer, int iMaxSize, int& iReadSize );

        /// Get last error.
        const char* get_last_error();

    protected:
        void* m_pImpl; // Process implementation.
    };


} // namespace good


#endif // __GOOD_PROCESS_H__
