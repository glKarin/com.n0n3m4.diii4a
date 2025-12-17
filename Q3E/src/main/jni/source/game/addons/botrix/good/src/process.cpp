#ifdef WIN32

#pragma warning(disable : 4533) // Disable warning about skipping inicialization of some variable, when using goto.


#include <windows.h>

#include "good/file.h"
#include "good/process.h"


namespace good
{

	DWORD WINAPI process_impl_process_proc(LPVOID lpprocessParameter); // Forward declaration.

	//----------------------------------------------------------------------------------------------------------------
	// process implementation.
	//----------------------------------------------------------------------------------------------------------------
	class process_impl
	{
	public:
		inline process_impl(): m_hProcess(NULL), m_hWriteChildInput(NULL), m_hReadChildOutput(NULL), m_hReadChildError(NULL) {}

		inline virtual ~process_impl()
		{
			dispose();
		}

		//------------------------------------------------------------------------------------------------------------
		// Set process parameters.
		//------------------------------------------------------------------------------------------------------------
		inline void set_params( const good::string& sExe, const good::string& sCmd, bool bRedirect, bool bChangeWorkingDir )
		{
			GoodAssert( sExe.size() > 0 || sCmd.size() > 0 );
			m_sExe.assign(sExe, true);
			m_sCmd.assign(sCmd, true);

			m_bChangeWorkingDir = bChangeWorkingDir;
			m_bRedirect = bRedirect;
			if ( bChangeWorkingDir )
				m_sCurrentDir = file::dir(sExe);
		}

		//------------------------------------------------------------------------------------------------------------
		// Execute process. 
		//------------------------------------------------------------------------------------------------------------
		inline bool launch( bool bShowProcessWindow, bool bDaemon )
		{
			#define SET_LAUNCH_ERROR(error) { SetError(error, __LINE__-1); goto process_launch_error; }

			GoodAssert( m_sExe.size() > 0 );

			m_szLastError[0] = 0; // No error.
			m_bDaemon = bDaemon;

			HANDLE hChildInput = NULL, hChildOutput = NULL, hChildError = NULL;

			// Set up the security attributes struct.
			static SECURITY_ATTRIBUTES sa;// = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
			// Set up the security attributes struct.
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = TRUE;

			// Set up the start up info struct.
			STARTUPINFO si;// = { sizeof(STARTUPINFO) };
			DWORD dwFlags;
			ZeroMemory(&si,sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			if ( m_bRedirect )
				si.dwFlags |= STARTF_USESTDHANDLES;
			if ( bShowProcessWindow )
			{
				dwFlags = CREATE_NEW_CONSOLE;
			}
			else
			{
				dwFlags = 0;
				si.dwFlags |= STARTF_USESHOWWINDOW;
				si.wShowWindow = SW_HIDE;
			}

			if ( m_bRedirect )
			{
				// Create the child input pipe, inheritable.
				if ( !CreatePipe(&si.hStdInput, &m_hWriteChildInput, &sa, 0) )
					SET_LAUNCH_ERROR("CreatePipe");

				// Ensure the write handle to the pipe for STDIN is not inherited.
				if ( !SetHandleInformation(m_hWriteChildInput, HANDLE_FLAG_INHERIT, 0) )
					SET_LAUNCH_ERROR("SetHandleInformation");

				// Create the child output pipe, inheritable.
				if ( !CreatePipe(&m_hReadChildOutput, &si.hStdOutput, &sa, 0) )
					SET_LAUNCH_ERROR("CreatePipe");

				if ( !SetHandleInformation(m_hReadChildOutput, HANDLE_FLAG_INHERIT, 0) )
					SET_LAUNCH_ERROR("SetHandleInformation");

				// Create the child output pipe, inheritable.
				if ( !CreatePipe(&m_hReadChildError, &si.hStdError, &sa, 0) )
					SET_LAUNCH_ERROR("CreatePipe");

				if ( !SetHandleInformation(m_hReadChildError, HANDLE_FLAG_INHERIT, 0) )
					SET_LAUNCH_ERROR("SetHandleInformation");
			}

			PROCESS_INFORMATION pi;
			if ( !CreateProcess(m_sExe.c_str(), (LPTSTR)m_sCmd.c_str(), NULL, NULL, TRUE, dwFlags, NULL, m_sCurrentDir.size() > 0 ? m_sCurrentDir.c_str() : NULL, &si, &pi) )
				SET_LAUNCH_ERROR("CreateProcess");

			// Set global child process handle to cause processs to exit.
			m_hProcess = pi.hProcess;

			// Close any unnecessary handles.
			if ( !CloseHandle(pi.hThread) )
				SET_LAUNCH_ERROR("CloseHandle");

			// Close pipe handles (do not continue to modify the parent). You need to make sure that no handles 
			// to the write end of the output pipe are maintained in this process or else the pipe will not close 
			// when the child process exits and the ReadFile will hang.
			if ( hChildInput && !CloseHandle(hChildInput) )
				SET_LAUNCH_ERROR("CloseHandle");
			if ( hChildOutput && !CloseHandle(hChildOutput) )
				SET_LAUNCH_ERROR("CloseHandle");
			if ( hChildError && !CloseHandle(hChildError) )
				SET_LAUNCH_ERROR("CloseHandle");

			return true;

			// Error handling.
		process_launch_error:
			if ( hChildInput )
				CloseHandle(hChildInput);
			if ( hChildOutput )
				CloseHandle(hChildOutput);
			if ( hChildError )
				CloseHandle(hChildError);
			if ( m_hWriteChildInput )
			{
				CloseHandle(m_hWriteChildInput);
				m_hWriteChildInput = NULL;
			}
			if ( m_hReadChildOutput )
			{
				CloseHandle(m_hReadChildOutput);
				m_hReadChildOutput = NULL;
			}
			if ( m_hReadChildError )
			{
				CloseHandle(m_hReadChildError);
				m_hReadChildError = NULL;
			}
			return false;
		}

		//------------------------------------------------------------------------------------------------------------
		// Free all handles and memory. Terminate process if not daemon.
		//------------------------------------------------------------------------------------------------------------
		inline void dispose()
		{
			if ( m_hProcess )
			{
				if ( !m_bDaemon && !is_finished() )
					TerminateProcess(m_hProcess, 2);
				CloseHandle(m_hProcess);
				if ( m_hWriteChildInput )
					CloseHandle(m_hWriteChildInput);
				if ( m_hReadChildOutput )
					CloseHandle(m_hReadChildOutput);
				if ( m_hReadChildError )
					CloseHandle(m_hReadChildError);
				m_hProcess = m_hWriteChildInput = m_hReadChildOutput = m_hReadChildError = NULL;
			}
		}

		//------------------------------------------------------------------------------------------------------------
		// Join a process for a given time.
		//------------------------------------------------------------------------------------------------------------
		inline bool join( int iMSecs = TIME_INFINITE )
		{
			GoodAssert( m_hProcess );
			return WaitForSingleObject(m_hProcess, iMSecs) == WAIT_OBJECT_0;
		}


		//------------------------------------------------------------------------------------------------------------
		// Terminate process.
		//------------------------------------------------------------------------------------------------------------
		inline void terminate()
		{
			GoodAssert( m_hProcess );
			TerminateProcess(m_hProcess, 1);
		}

		//------------------------------------------------------------------------------------------------------------
		// Check if process was launched previously.
		//------------------------------------------------------------------------------------------------------------
		inline bool is_launched() { return m_hProcess != NULL; }

		//------------------------------------------------------------------------------------------------------------
		// Check if process is finished.
		//------------------------------------------------------------------------------------------------------------
		inline bool is_finished()
		{
			GoodAssert( m_hProcess );
			DWORD iExitCode = 0;
			GetExitCodeProcess(m_hProcess, &iExitCode); // TODO: Handle error.
			return iExitCode != STILL_ACTIVE;
		}

		//----------------------------------------------------------------------------------------------------------------
		// Close process stdin, end of input.
		//------------------------------------------------------------------------------------------------------------
		void close_stdin()
		{
			GoodAssert( m_hWriteChildInput );
			CloseHandle( m_hWriteChildInput );
			m_hWriteChildInput = NULL;
		}

		//----------------------------------------------------------------------------------------------------------------
		// Write input for the process. Return false on error or when child exits.
		//----------------------------------------------------------------------------------------------------------------
		inline bool write_stdin( void* pBuffer, int iSize )
		{
			m_szLastError[0] = 0; // No error.

			GoodAssert(m_hWriteChildInput);
			DWORD iTotal = 0;
			while ( iTotal < (DWORD)iSize )
			{
				DWORD iWritten;
				if ( !WriteFile(m_hWriteChildInput, (char*)pBuffer + iTotal, iSize, &iWritten, NULL) )
				{
					if ( GetLastError() != ERROR_NO_DATA ) // Pipe was closed (normal exit path).
						SetError("WriteFile", __LINE__ - 3);
					CloseHandle(m_hWriteChildInput);
					m_hWriteChildInput = NULL;
					return false;
				}
				iTotal += iWritten;
			}
			return true;
		}

		// Return true if child has data on stdout.
		inline bool has_data_stdout()
		{
			GoodAssert(m_hReadChildOutput);
			DWORD iBytesAvailable;
			if ( !PeekNamedPipe(m_hReadChildOutput, NULL, 0, NULL, &iBytesAvailable, NULL) )
				return true; // Force to read and thus fail.
			return iBytesAvailable > 0;
		}

		// Read output of the process, returning true, if there is more output.
		inline bool read_stdout( void* pBuffer, int iMaxSize, int& iReadSize )
		{
			return read( m_hReadChildOutput, pBuffer, iMaxSize, iReadSize );
		}

		// Return true if child has data on stderr.
		inline bool has_data_stderr()
		{
			GoodAssert(m_hReadChildError);
			DWORD iBytesAvailable;
			if ( !PeekNamedPipe(m_hReadChildError, NULL, 0, NULL, &iBytesAvailable, NULL) )
				return true; // Force to read and thus fail.
			return iBytesAvailable > 0;
		}

		// Read error of the process, returning true, if there is more output.
		inline bool read_stderr( void* pBuffer, int iMaxSize, int& iReadSize )
		{
			return read( m_hReadChildError, pBuffer, iMaxSize, iReadSize );
		}

		// Get last error.
		inline const char* get_last_error()
		{
			return (m_szLastError[0] == 0) ? NULL : m_szLastError;
		}

	protected:
		// Read handle.
		bool read( HANDLE& hHandle, void* pBuffer, int iMaxSize, int& iReadSize )
		{
			GoodAssert(hHandle);

			m_szLastError[0] = 0; // No error.

			DWORD dwReadSize;
			bool result = (ReadFile(hHandle, pBuffer, iMaxSize, &dwReadSize, NULL) == TRUE);
			iReadSize = dwReadSize;

			if ( !result || (dwReadSize == 0) )
			{
				if ( GetLastError() != ERROR_BROKEN_PIPE )
					SetError("ReadFile", __LINE__ - 6); // Something bad happened.
				CloseHandle(hHandle);
				hHandle = NULL;
				return false; // Error or pipe gone = child exited, no more data.
			}
			return true;

			// Display the character read on the screen.
			//if ( !WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),lpBuffer,nBytesRead,&nCharsWritten,NULL))
			//	SetError("WriteConsole");
		}

		// Displays the error number and corresponding message.
		void SetError( char* szFunction, int iLine )
		{
			GoodAssert( GetLastError() != 0 );

			LPVOID szError;
			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&szError, 0, NULL);

#if defined(DEBUG) || defined(_DEBUG)
			wsprintf(m_szLastError, "Error %d from %s(), line %d: %s", GetLastError(), szFunction, iLine, szError);
#else
			wsprintf(m_szLastError, "Error %d from %s(): %s", GetLastError(), szFunction, szError);
#endif
			LocalFree(szError);
		}

		good::string m_sExe, m_sCmd, m_sCurrentDir;
		HANDLE m_hProcess, m_hWriteChildInput, m_hReadChildOutput, m_hReadChildError;
		char m_szLastError[512];
		bool m_bDaemon;
		bool m_bChangeWorkingDir;
		bool m_bRedirect;
	};


	//----------------------------------------------------------------------------------------------------------------
	void process::exit( int iExitCode )
	{
		ExitProcess(iExitCode);
	}

	//----------------------------------------------------------------------------------------------------------------
	// process functions.
	//----------------------------------------------------------------------------------------------------------------
	process::process()
	{
		m_pImpl = new process_impl();
	}

	//----------------------------------------------------------------------------------------------------------------
	process::process( const good::string& sExe, const good::string& sCmd, bool bRedirect, bool bChangeWorkingDir )
	{
		m_pImpl = new process_impl();
		set_params(sExe, sCmd, bRedirect, bChangeWorkingDir);
	}

	//----------------------------------------------------------------------------------------------------------------
	process::~process()
	{
		delete (process_impl*)m_pImpl;
	}

	//----------------------------------------------------------------------------------------------------------------
	void process::set_params( const good::string& sExe, const good::string& sCmd, bool bRedirect, bool bChangeWorkingDir)
	{
		((process_impl*)m_pImpl)->set_params(sExe, sCmd, bRedirect, bChangeWorkingDir);
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::launch( bool bShowProcessWindow, bool bDaemon )
	{
		return ((process_impl*)m_pImpl)->launch(bShowProcessWindow, bDaemon);
	}

	//----------------------------------------------------------------------------------------------------------------
	void process::terminate()
	{
		((process_impl*)m_pImpl)->terminate();
	}

	//----------------------------------------------------------------------------------------------------------------
	void process::dispose()
	{
		((process_impl*)m_pImpl)->dispose();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::join( int iMSecs )
	{
		return ((process_impl*)m_pImpl)->join(iMSecs);
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::is_launched()
	{
		return ((process_impl*)m_pImpl)->is_launched();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::is_finished()
	{
		return ((process_impl*)m_pImpl)->is_finished();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::write_stdin( void* pBuffer, int iSize )
	{
		return ((process_impl*)m_pImpl)->write_stdin(pBuffer, iSize);
	}

	//----------------------------------------------------------------------------------------------------------------
	void process::close_stdin()
	{
		return ((process_impl*)m_pImpl)->close_stdin();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::has_data_stdout()
	{
		return ((process_impl*)m_pImpl)->has_data_stdout();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::read_stdout( void* pBuffer, int iMaxSize, int& iReadSize )
	{
		return ((process_impl*)m_pImpl)->read_stdout(pBuffer, iMaxSize, iReadSize);
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::has_data_stderr()
	{
		return ((process_impl*)m_pImpl)->has_data_stderr();
	}

	//----------------------------------------------------------------------------------------------------------------
	bool process::read_stderr( void* pBuffer, int iMaxSize, int& iReadSize )
	{
		return ((process_impl*)m_pImpl)->read_stderr(pBuffer, iMaxSize, iReadSize);
	}

	//----------------------------------------------------------------------------------------------------------------
	const char* process::get_last_error()
	{
		return ((process_impl*)m_pImpl)->get_last_error();
	}
} // namespace good


#endif // WIN32
