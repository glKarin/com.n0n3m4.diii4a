#ifndef _KARIN_SYS_ANDROID_H
#define _KARIN_SYS_ANDROID_H

#include <stdio.h>
#include <android/native_window.h>

// DOOM3 call Android::JNI, all in DOOM3 main thread
typedef struct
{
    // AudioTrack
    void (*AudioTrack_init)(void *buffer, int size); // idAudioHardwareAndroid init AudioTrack
    int (*AudioTrack_write)(int offset, int length); // idAudioHardwareAndroid write data to AudioTrack
    void (*AudioTrack_shutdown)(void); // idAudioHardwareAndroid shutdown AudioTrack

    // Input
    void (*Input_grabMouse)(int grab); // Android grab mouse
    int (*Input_pullEvent)(int execCmd); // Android pull input event
    void (*Input_setupSmoothJoystick)(int enable); // enable Android smooth joystick

    // System
    void (*Sys_attachThread)(void); // Attach C/C++ created thread to JNI
    FILE * (*Sys_tmpfile)(void); // for Android tmpfile C function
    void (*Sys_copyToClipboard)(const char *text); // copy text to clipboard
    char * (*Sys_getClipboardText)(void); // get text from clipboard
    void (*Sys_openKeyboard)(void); // open virtual keyboard
    void (*Sys_closeKeyboard)(void); // close virtual keyboard
    void (*Sys_openURL)(const char *url); // open URL
    void (*Sys_exitFinish)(void); // finish activity
    void (*Sys_showCursor)(int on); // show mouse cursor

    // GUI
    void (*Gui_ShowToast)(const char *text); // show info
    int (*Gui_openDialog)(const char *title, const char *message, int num, const char *buttons[]); // open dialog(allow buttons and block current thread)

    // Other
    void (*set_state)(int st); // Tell Android game current state
} Q3E_Callback_t;

// Android::JNI call DOOM3 before main()
typedef struct
{
    // OpenGL
    int openGL_format; // 0x8888 0x565 0x4444 0x5551 0xaaa2
    int openGL_depth; // 24 16 32
    int openGL_msaa; // 0 1 2 4 8 16
    int openGL_version; // 0x2000 0x3000 0x3010 0x3020

    // Other
    const char *nativeLibraryDir; // game library directory after apk installed
    int redirectOutputToFile; // stdout/stderr redirect output to file
    int noHandleSignals; // don't handle signals
    int multithread; // enable multi-threading rendering
    int mouseAvailable; // using mouse
    int continueWhenNoGLContext; // Continue when missing OpenGL context
    const char *gameDataDir; // game data directory
    const char *appHomeDir; // application home directory
    int refreshRate; // screen refresh rate
    int smoothJoystick; // smooth joystick
    int consoleMaxHeightFrac; // max console height frac(0 - 100)
    int usingExternalLibs; // using external libraries

    ANativeWindow *window;
    int width;
    int height;
} Q3E_InitialContext_t;

// Android::JNI call DOOM3 after main()
typedef struct
{
    // any thread(Java): before idCommon Initialized
    int  (*main)(int argc, char **argv); // call main(int, const char **)
    void (*setCallbacks)(const void *func);
    void (*setInitialContext)(const void *context);

    // any thread(Java): after idCommon Initialized
    void (*pause)(void); // pause
    void (*resume)(void); // resume
    void (*exit)(void); // exit

    // SurfaceView thread(Java)
    void (*setGLContext)(ANativeWindow *window); // set OpenGL surface view window
    void (*requestThreadQuit)(void); // request main thread quit

    // DOOM3 main thread(C/C++)
    void (*keyEvent)(int state, int key, int chr); // mouse-click/keyboard event
    void (*analogEvent)(int enable, float x, float y); // analog event
    void (*motionEvent)(float x, float y); // mouse-motion event

} Q3E_Interface_t;

#endif // _KARIN_SYS_ANDROID_H
