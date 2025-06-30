/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWDISPLAY.H -- LightWave Host Display Info
 *
 * The host application provides a variety of services for getting user
 * input, but if all else fails the plug-in may need to open windows.
 * Since it runs in the host's context, it needs to get the host's display
 * information to do this.  This info, which can be normally accessed
 * with the "Host Display Info" global service, contains information about
 * the windows and display context used by the host.  If this ID yeilds a
 * null pointer, the server is probably running in a batch mode and has no
 * display context.
 */
#ifndef LWSDK_DISPLAY_H
#define LWSDK_DISPLAY_H

#ifdef _WIN32
 #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0400
 #endif
 #include <windows.h>
#endif
#ifdef _XGL
 #include <X11/Xlib.h>
#endif
#ifdef _MACOS
 #include <Quickdraw.h>
#endif


/*
 * The fields of the HostDisplayInfo structure vary from system to system,
 * but all include the window pointer of the main application window or
 * null if there is none.  On X systems, the window session handle is
 * passed.  On Win32 systems, the application instance is provided, even
 * though it belongs to the host and is probably useless.
 */
#define LWHOSTDISPLAYINFO_GLOBAL        "Host Display Info"

typedef struct st_HostDisplayInfo {
    #ifdef _WIN32
        HANDLE           instance;
        HWND             window;
    #endif
    #ifdef _XGL
        Display         *xsys;
        Window           window;
    #endif
    #ifdef _MACOS
        WindowPtr        window;
    #endif
} HostDisplayInfo;

#endif

