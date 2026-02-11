/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// winquake.h: Win32-specific Quake header file

#ifndef WINQUAKE_H
#define WINQUAKE_H

#ifdef _WIN32
void GLVID_Crashed(void);

#if defined(_WIN32) && !defined(WIN32)
#define WIN32 _WIN32
#endif

#ifdef MSVCDISABLEWARNINGS 
#pragma warning( disable : 4229 )  // mgraph gets this
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define WIN32_LEAN_AND_MEAN
#define byte winbyte
#ifdef _XBOX
	#include <xtl.h>
	#include <WinSockX.h>
#else
	#include <windows.h>
	#include <mmsystem.h>
	#include <mmreg.h>
#endif

#define _LPCWAVEFORMATEX_DEFINED


#if defined(WINAPI_FAMILY) && !defined(WINRT)
	#if WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP
		//don't just define it. things that don't #include winquake.h / glquake.h need it too.
		#error "WINRT needs to be defined for non-desktop"
	#endif
#endif


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL					0x020A
#endif
#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND					0x0319
#endif

#define WM_USER_SPEECHTOTEXT			(WM_USER+0)	//used by stt
#define WM_USER_VIDSHUTDOWN				(WM_USER+4)	//used by multithreading
#define WM_USER_VKPRESENT				(WM_USER+7)	//used by vulkan
#define WM_USER_NVVKPRESENT				(WM_USER+8)	//used by vulkan-over-opengl

#undef byte

extern	HINSTANCE	global_hInstance;
extern	int			global_nCmdShow;

extern HWND sys_parentwindow;
extern unsigned int sys_parentleft;
extern unsigned int sys_parenttop;
extern unsigned int sys_parentwidth;
extern unsigned int sys_parentheight;

#ifndef SERVERONLY

#ifdef HAVE_CDPLAYER
#ifdef _WIN32
LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
#endif


//shell32 stuff that doesn't exist in win95
#define COBJMACROS

#ifndef _XBOX
#include <shlobj.h>
#include <shellapi.h>
extern LPITEMIDLIST (STDAPICALLTYPE *pSHBrowseForFolderW)(LPBROWSEINFOW lpbi);
extern BOOL (STDAPICALLTYPE *pSHGetPathFromIDListW)(LPCITEMIDLIST pidl, LPWSTR pszPath);
extern BOOL (STDAPICALLTYPE *pSHGetSpecialFolderPathW)(HWND hwnd, LPWSTR pszPath, int csidl, BOOL fCreate);
extern BOOL (STDAPICALLTYPE *pShell_NotifyIconW)(DWORD dwMessage, PNOTIFYICONDATAW lpData);
#endif

//void	VID_LockBuffer (void);
//void	VID_UnlockBuffer (void);

#endif

extern HWND			mainwindow;
extern qboolean		Minimized;

extern qboolean	WinNT;

void INS_UpdateGrabs(int fullscreen, int activeapp);
void INS_RestoreOriginalMouseState (void);
void INS_SetQuakeMouseState (void);
void INS_MouseEvent (int mstate);
void INS_RawInput_Read(HANDLE in_device_handle);

extern qboolean	winsock_lib_initialized;

extern int		window_center_x, window_center_y;
extern RECT		window_rect;

extern qboolean	mouseinitialized;

//extern HANDLE	hinput, houtput;

extern HCURSOR	hArrowCursor, hCustomCursor;
enum uploadfmt;
void *WIN_CreateCursor(const qbyte *imagedata, int width, int height, enum uploadfmt format, float hotx, float hoty, float scale);
qboolean WIN_SetCursor(void *cursor);
void WIN_DestroyCursor(void *cursor);
void WIN_WindowCreated(HWND window);

void INS_UpdateClipCursor (void);
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify);
void INS_TranslateKeyEvent(WPARAM wParam, LPARAM lParam, qboolean down, int pnum, qboolean genkeystate);
int INS_AppCommand(LPARAM lParam);
void INS_DeviceChanged(void *ctx, void *data, size_t a ,size_t b);

void S_BlockSound (void);
void S_UnblockSound (void);

void VID_SetDefaultMode (void);
/*
int (PASCAL FAR *pWSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);
int (PASCAL FAR *pWSACleanup)(void);
int (PASCAL FAR *pWSAGetLastError)(void);
SOCKET (PASCAL FAR *psocket)(int af, int type, int protocol);
int (PASCAL FAR *pioctlsocket)(SOCKET s, long cmd, u_long FAR *argp);
int (PASCAL FAR *psetsockopt)(SOCKET s, int level, int optname,
							  const char FAR * optval, int optlen);
int (PASCAL FAR *precvfrom)(SOCKET s, char FAR * buf, int len, int flags,
							struct sockaddr FAR *from, int FAR * fromlen);
int (PASCAL FAR *psendto)(SOCKET s, const char FAR * buf, int len, int flags,
						  const struct sockaddr FAR *to, int tolen);
int (PASCAL FAR *pclosesocket)(SOCKET s);
int (PASCAL FAR *pgethostname)(char FAR * name, int namelen);
struct hostent FAR * (PASCAL FAR *pgethostbyname)(const char FAR * name);
struct hostent FAR * (PASCAL FAR *pgethostbyaddr)(const char FAR * addr,
												  int len, int type);
int (PASCAL FAR *pgetsockname)(SOCKET s, struct sockaddr FAR *name,
							   int FAR * namelen);
*/
#endif

#endif

