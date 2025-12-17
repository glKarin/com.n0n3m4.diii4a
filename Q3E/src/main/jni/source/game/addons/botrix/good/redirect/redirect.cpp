/*++

  Copyright (c) 1998  Microsoft Corporation

  Module Name:

     Redirect.c

  Description:
      This sample illustrates how to spawn a child console based
      application with redirected standard handles.

      The following import libraries are required:
      user32.lib

  Dave McPherson (davemm)   11-March-98

--*/ 
#include <stdio.h>
#include<windows.h>
#pragma comment(lib, "User32.lib")
void DisplayError(char *pszAPI);
void ReadAndHandleOutput(HANDLE hPipeRead);
void PrepAndLaunchRedirectedChild(HANDLE hChildStdOut,
                                 HANDLE hChildStdIn,
                                 HANDLE hChildStdErr);
DWORD WINAPI GetAndSendInputThread(LPVOID lpvThreadParam);

HANDLE hChildProcess = NULL;
HANDLE hStdIn = NULL; // Handle to parents std input.
BOOL bRunThread = TRUE;


void main ()
{
  HANDLE hOutputReadTmp,hOutputRead,hOutputWrite;
  HANDLE hInputWriteTmp,hInputRead=NULL,hInputWrite=NULL;
  HANDLE hErrorWrite;
  HANDLE hThread;
  DWORD ThreadId;
  SECURITY_ATTRIBUTES sa;


  // Set up the security attributes struct.
  sa.nLength= sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;


  // Create the child output pipe.
  if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))
     DisplayError("CreatePipe");


  // Create a duplicate of the output write handle for the std error
  // write handle. This is necessary in case the child application
  // closes one of its std output handles.
  if (!DuplicateHandle(GetCurrentProcess(),hOutputWrite,
                       GetCurrentProcess(),&hErrorWrite,0,
                       TRUE,DUPLICATE_SAME_ACCESS))
     DisplayError("DuplicateHandle");


  // Create the child input pipe.
#ifdef CREATE_STDIN
  if (!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0))
     DisplayError("CreatePipe");
#endif

  // Create new output read handle and the input write handles. Set
  // the Properties to FALSE. Otherwise, the child inherits the
  // properties and, as a result, non-closeable handles to the pipes
  // are created.
  if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
                       GetCurrentProcess(),
                       &hOutputRead, // Address of new handle.
                       0,FALSE, // Make it uninheritable.
                       DUPLICATE_SAME_ACCESS))
     DisplayError("DupliateHandle");

#ifdef CREATE_STDIN
  if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
                       GetCurrentProcess(),
                       &hInputWrite, // Address of new handle.
                       0,FALSE, // Make it uninheritable.
                       DUPLICATE_SAME_ACCESS))
  DisplayError("DupliateHandle");
#endif

  // Close inheritable copies of the handles you do not want to be
  // inherited.
  if (!CloseHandle(hOutputReadTmp)) DisplayError("CloseHandle");
#ifdef CREATE_STDIN
  if (!CloseHandle(hInputWriteTmp)) DisplayError("CloseHandle");
#endif

  // Get std input handle so you can close it and force the ReadFile to
  // fail when you want the input thread to exit.
  if ( (hStdIn = GetStdHandle(STD_INPUT_HANDLE)) ==
                                            INVALID_HANDLE_VALUE )
     DisplayError("GetStdHandle");

  PrepAndLaunchRedirectedChild(hOutputWrite,hInputRead,hErrorWrite);


  // Close pipe handles (do not continue to modify the parent).
  // You need to make sure that no handles to the write end of the
  // output pipe are maintained in this process or else the pipe will
  // not close when the child process exits and the ReadFile will hang.
  if (!CloseHandle(hOutputWrite)) DisplayError("CloseHandle");
#ifdef CREATE_STDIN
  if (!CloseHandle(hInputRead )) DisplayError("CloseHandle");
#endif
  if (!CloseHandle(hErrorWrite)) DisplayError("CloseHandle");


  // Launch the thread that gets the input and sends it to the child.
#ifdef CREATE_STDIN
  hThread = CreateThread(NULL,0,GetAndSendInputThread,
                          (LPVOID)hInputWrite,0,&ThreadId);
  if (hThread == NULL) DisplayError("CreateThread");
#endif

  // Read the child's output.
  ReadAndHandleOutput(hOutputRead);
  // Redirection is complete


  // Force the read on the input to return by closing the stdin handle.
  if (!CloseHandle(hStdIn)) DisplayError("CloseHandle");


  // Tell the thread to exit and wait for thread to die.
  bRunThread = FALSE;

#ifdef CREATE_STDIN
  if (WaitForSingleObject(hThread,INFINITE) == WAIT_FAILED)
     DisplayError("WaitForSingleObject");
#endif

  if (!CloseHandle(hOutputRead)) DisplayError("CloseHandle");
#ifdef CREATE_STDIN
  if (!CloseHandle(hInputWrite)) DisplayError("CloseHandle");
#endif
  gets(new char[512]);
  system("pause");
}


/////////////////////////////////////////////////////////////////////// 
// PrepAndLaunchRedirectedChild
// Sets up STARTUPINFO structure, and launches redirected child.
/////////////////////////////////////////////////////////////////////// 
void PrepAndLaunchRedirectedChild(HANDLE hChildStdOut,
                                 HANDLE hChildStdIn,
                                 HANDLE hChildStdErr)
{
  PROCESS_INFORMATION pi;
  STARTUPINFO si;

  // Set up the start up info struct.
  ZeroMemory(&si,sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = hChildStdOut;
  si.hStdInput  = hChildStdIn;
  si.hStdError  = hChildStdErr;
  // Use this if you want to hide the child:
  //     si.wShowWindow = SW_HIDE;
  // Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
  // use the wShowWindow flags.


  // Launch the process that you want to redirect (in this case,
  // Child.exe). Make sure Child.exe is in the same directory as
  // redirect.c launch redirect from a command line to prevent location
  // confusion.
  if (!CreateProcess(NULL,"echo.exe",NULL,NULL,TRUE,
                     CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
     DisplayError("CreateProcess");


  // Set global child process handle to cause threads to exit.
  hChildProcess = pi.hProcess;


  // Close any unnecessary handles.
  if (!CloseHandle(pi.hThread)) DisplayError("CloseHandle");
}


/////////////////////////////////////////////////////////////////////// 
// ReadAndHandleOutput
// Monitors handle for input. Exits when child exits or pipe breaks.
/////////////////////////////////////////////////////////////////////// 
void ReadAndHandleOutput(HANDLE hPipeRead)
{
  CHAR lpBuffer[256];
  DWORD nBytesRead;
  DWORD nCharsWritten;

  while(TRUE)
  {
     if (!ReadFile(hPipeRead,lpBuffer,sizeof(lpBuffer),
                                      &nBytesRead,NULL) || !nBytesRead)
     {
        if (GetLastError() == ERROR_BROKEN_PIPE)
           break; // pipe done - normal exit path.
        else
           DisplayError("ReadFile"); // Something bad happened.
     }

     // Display the character read on the screen.
     if (!WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),lpBuffer,
                       nBytesRead,&nCharsWritten,NULL))
        DisplayError("WriteConsole");
  }
}


/////////////////////////////////////////////////////////////////////// 
// GetAndSendInputThread
// Thread procedure that monitors the console for input and sends input
// to the child process through the input pipe.
// This thread ends when the child application exits.
/////////////////////////////////////////////////////////////////////// 
DWORD WINAPI GetAndSendInputThread(LPVOID lpvThreadParam)
{
  CHAR read_buff[256];
  DWORD nBytesRead,nBytesWrote;
  HANDLE hPipeWrite = (HANDLE)lpvThreadParam;

  // Get input from our console and send it to child through the pipe.
  char* s[2] = {"hello\n", "exit\n"};
 // while (bRunThread)
  for (int i=0; i<2; ++i)
  {
    /* if(!ReadConsole(hStdIn,read_buff,1,&nBytesRead,NULL))
        DisplayError("ReadConsole");

     read_buff[nBytesRead] = '\0'; // Follow input with a NULL.*/

	  nBytesRead = strlen(s[i]);
     if (!WriteFile(hPipeWrite,s[i],nBytesRead,&nBytesWrote,NULL))
     {
        if (GetLastError() == ERROR_NO_DATA)
           break; // Pipe was closed (normal exit path).
        else
			DisplayError("WriteFile");
     }
  }

  return 1;
}


/////////////////////////////////////////////////////////////////////// 
// DisplayError
// Displays the error number and corresponding message.
/////////////////////////////////////////////////////////////////////// 
void DisplayError(char *pszAPI)
{
   LPVOID lpvMessageBuffer;
   CHAR szPrintBuffer[512];
   DWORD nCharsWritten;

   FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpvMessageBuffer, 0, NULL);

   wsprintf(szPrintBuffer,
     "ERROR: API    = %s.\n   error code = %d.\n   message    = %s.\n",
            pszAPI, GetLastError(), (char *)lpvMessageBuffer);

   WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),szPrintBuffer,
                 lstrlen(szPrintBuffer),&nCharsWritten,NULL);

   LocalFree(lpvMessageBuffer);
   ExitProcess(GetLastError());
}
