#ifndef _KARIN_DOOM3_ANDROID_H
#define _KARIN_DOOM3_ANDROID_H

// DOOM3 call Android::JNI, all in DOOM3 main thread
typedef struct
{
    // AudioTrack
    void (*AudioTrack_init)(void *buffer, int size); // idAudioHardwareAndroid init AudioTrack
    int (*AudioTrack_write)(int offset, int length); // idAudioHardwareAndroid write data to AudioTrack

    // Input
    void (*Input_grabMouse)(int grab); // Android grab mouse
    void (*Input_pullEvent)(int execCmd); // Android pull input event

    // System
    void (*Sys_attachThread)(void); // Attach C/C++ created thread to JNI
    FILE * (*Sys_tmpfile)(void); // for Android tmpfile C function
    void (*Sys_copyToClipboard)(const char *text); // copy text to clipboard
    char * (*Sys_getClipboardText)(void); // get text from clipboard

    // Other
    void (*set_state)(int st); // Tell Android game current state
} Q3E_Callback_t;

// Android::JNI call DOOM3 before main()
typedef struct
{
    // OpenGL
    int openGL_format; // 0x8888 0x565 0x4444
    int openGL_msaa; // 0 1 2 4
    int openGL_version; // 20 30

    // Other
    const char *nativeLibraryDir; // game library directory after apk installed
    int redirectOutputToFile; // stdout/stderr redirect output to file
    int noHandleSignals; // don't handle signals
    int multithread; // enable multi-threading rendering
    int continueWhenNoGLContext; // Continue when missing OpenGL context
} Q3E_InitialContext_t;

// Android::JNI call DOOM3 after main()
typedef struct
{
    // any thread(Java): before idCommon Initialized
    int  (*main)(int argc, const char **argv); // call main(int, const char **)
    void (*setCallbacks)(const void *func);
    void (*setInitialContext)(const void *context);
    void (*setResolution)(int width, int height);

    // any thread(Java): after idCommon Initialized
    void (*pause)(void); // pause
    void (*resume)(void); // resume
    void (*exit)(void); // exit

    // SurfaceView thread(Java)
    void (*setGLContext)(ANativeWindow *window); // set OpenGL surface view window

    // GLSurfaceView render thread(Java)
    void (*frame)(void); // call common->Frame()
    void (*vidRestart)(void); // UNUSED

    // DOOM3 main thread(C/C++)
    void (*keyEvent)(int state, int key, int chr); // mouse-click/keyboard event
    void (*analogEvent)(int enable, float x, float y); // analog event
    void (*motionEvent)(float x, float y); // mouse-motion event

} Q3E_Interface_t;

#endif // _KARIN_DOOM3_ANDROID_H
