#include "q3esdl2.h"
#include "../deplibs/SDL2/src/core/android/SDL_android_env.h"

#include <dlfcn.h>

#include <android/keycodes.h>

SDL_Android_GetAPI_f sdl_api;
int USING_SDL = 0;
static char *sdl_clipboardText = NULL;

/** Standard activity result: operation canceled. */
#define RESULT_CANCELED     0
/** Standard activity result: operation succeeded. */
#define RESULT_OK            -1
/** Start of user-defined activity results. */
#define RESULT_FIRST_USER    1

#define INTERFACE_METHOD(ret, name, args) ret (* name) args;
#include "../deplibs/SDL2/src/core/android/SDL_android_interface.h"

const char * clipboardGetText()
{
    if(sdl_clipboardText)
    {
        free(sdl_clipboardText);
        sdl_clipboardText = NULL;
    }
    sdl_clipboardText = get_clipboard_text();
    return sdl_clipboardText;
}

int clipboardHasText()
{
    char *text = get_clipboard_text();
    int res = text && text[0];
    if(text)
        free(text);
    return res;
}

void clipboardSetText(const char *text)
{
    copy_to_clipboard(text);
}

int createCustomCursor(int *pixels, int w, int h, int x, int y)
{
    return 0;
}

void destroyCustomCursor(int id)
{
    
}

void * getContext()
{
    return NULL;
}

void * getDisplayDPI()
{
    return NULL;
}

void * getNativeSurface()
{
    return window;
}

void initTouch()
{
    
}

int isAndroidTV()
{
    return Q3E_FALSE;
}

int isChromebook()
{
    return Q3E_FALSE;
}

int isDeXMode()
{
    return Q3E_FALSE;
}

int isScreenKeyboardShown()
{
    return Q3E_FALSE;
}

int isTablet()
{
    return Q3E_FALSE;
}

void manualBackButton()
{
    
}

void minimizeWindow()
{
    
}

int openURL(const char *url)
{
    open_url(url);
    return Q3E_TRUE;
}

void requestPermission(const char *permission, int requestCode)
{
    LOGI("Q3E SDL request permission '%s' with request code '%d'", permission, requestCode);
    CALL_SDL(nativePermissionResult, requestCode, RESULT_OK);
}

int showToast(const char *message, int duration, int gravity, int xOffset, int yOffset)
{
    show_toast(message);
    return Q3E_TRUE;
}

int sendMessage(int command, int param)
{
    return Q3E_FALSE;
}

int setActivityTitle(const char *title)
{
    return Q3E_FALSE;
}

int setCustomCursor(int cursorID)
{
    return Q3E_FALSE;
}

void setWindowStyle(int fullscreen)
{
    
}

int shouldMinimizeOnFocusLoss()
{
    return Q3E_FALSE;
}

int showTextInput(int x, int y, int w, int h)
{
    return Q3E_FALSE;
}

int supportsRelativeMouse()
{
    return Q3E_TRUE;
}

int setSystemCursor(int cursorID)
{
    return Q3E_FALSE;
}

void setOrientation(int w, int h, int resizable, const char *hint)
{
    
}

int setRelativeMouseEnabled(int on)
{
    grab_mouse(on);
    return Q3E_TRUE;
}

int getManifestEnvironmentVariables()
{
    return Q3E_FALSE;
}

int * getAudioOutputDevices()
{
    return NULL;
}

int * getAudioInputDevices()
{
    return NULL;
}

int * audioOpen(int freq, int audioformat, int channels, int samples, int device_id)
{
    return NULL;
}

void audioWriteByteBuffer(char *buf)
{
    
}

void audioWriteShortBuffer(short *buf)
{
    
}

void audioWriteFloatBuffer(float *buf)
{
    
}

void audioClose()
{
    
}

int * captureOpen(int freq, int audioformat, int channels, int samples, int device_id)
{
    return NULL;
}

int captureReadByteBuffer(char *buf, int b)
{
    return 0;
}

int captureReadShortBuffer(short *buf, int b)
{
    return 0;
}

int captureReadFloatBuffer(float *buf, int b)
{
    return 0;
}

void captureClose()
{
    
}

void audioSetThreadPriority(int iscapture, int device_id)
{
    
}

void pollInputDevices()
{
    
}

void pollHapticDevices()
{
    
}

void hapticRun(int device_id, float intensity, int length)
{
    
}

void hapticStop(int device_id)
{
    
}

int messageboxShowMessageBox(int flags, const char *title, const char *message, int *button_flags, int *button_ids,  const char *button_texts[], int *colors)
{
    int num = ARRAY_ELEMENT_LENGTH(button_texts, const char *);
    int res = open_dialog(title, message, num, button_texts);
    switch(res)
    {
        case 1:
            if(num > 0)
                return button_ids[0];
        case 2:
            if(num > 1)
                return button_ids[1];
        case 3:
            if(num > 2)
                return button_ids[2];
        default: // -1 0
            return res;
    }
    return -2; // unknown
}

void attachCurrentThread()
{
    Android_AttachThread();
}

void detachCurrentThread()
{
    Android_DetachThread();
}

void Q3E_ShutdownSDL(void)
{
    if(sdl_clipboardText)
    {
        free(sdl_clipboardText);
        sdl_clipboardText = NULL;
    }
    LOGI("Q3E SDL2 shutdown");
}

void Q3E_InitSDL(void)
{
    if(!libdl)
    {
        LOGW("Please load game library first");
        return;
    }

    if(sdl_api)
    {
        LOGW("Q3E SDL2 has initialized");
        return;
    }

    void *ptr = dlsym(libdl, "SDL_Android_GetAPI");
    if(!ptr)
    {
        LOGI("Game library not use SDL2");
        return;
    }

    LOGI("SDL2 API found!");

    sdl_api = (SDL_Android_GetAPI_f)ptr;
    SDL_Android_Callback_t callbacks;
#define CALLBACK_METHOD(ret, name, args) callbacks.name = name;
#include "../deplibs/SDL2/src/core/android/SDL_android_callback.h"
            
    SDL_Android_Interface_t interfaces;

    sdl_api(&callbacks, &interfaces);

#define INTERFACE_METHOD(ret, name, args) name = interfaces.name;
#include "../deplibs/SDL2/src/core/android/SDL_android_interface.h"

    CALL_SDL(nativeSetupJNI);
    CALL_SDL(audio_nativeSetupJNI);
    CALL_SDL(controller_nativeSetupJNI);

    atexit(Q3E_ShutdownSDL);

    USING_SDL = 1;
    LOGI("Game library SDL2 initialized!");
}

typedef float mouse_pos_t;
static mouse_pos_t in_positionX = 0;
static mouse_pos_t in_positionY = 0;
static mouse_pos_t in_deltaX = 0;
static mouse_pos_t in_deltaY = 0;
//static int8_t mouseState;
static uint8_t relativeMouseMode = Q3E_FALSE;
static int IN_WINDOW_WIDTH;
static int IN_WINDOW_HEIGHT;

void Q3E_SDL_SetRelativeMouseMode(int on)
{
    relativeMouseMode = on;
    if(on)
    {
        in_deltaX = IN_WINDOW_WIDTH / 2;
        in_deltaY = IN_WINDOW_HEIGHT / 2;
        set_mouse_cursor_visible(false);
    }
    else
    {
        //in_positionX = 0;
        //in_positionY = 0;
        set_mouse_cursor_position(in_positionX, in_positionY);
        set_mouse_cursor_visible(true);
    }
}

void Q3E_SDL_SetWindowSize(int width, int height)
{
    if(IN_WINDOW_WIDTH && IN_WINDOW_HEIGHT)
        return;
    IN_WINDOW_WIDTH = width;
    IN_WINDOW_HEIGHT = height;
	in_deltaX = IN_WINDOW_WIDTH / 2;
	in_deltaY = IN_WINDOW_HEIGHT / 2;
}

static void Q3E_SDL_MouseMotion(float dx, float dy)
{
    if(relativeMouseMode)
    {
        mouse_pos_t x = dx;
        mouse_pos_t y = dy;

        int hw = IN_WINDOW_WIDTH / 2;
        if(x < -hw) {
            x = -hw;
        } else if(x > hw) {
            x = hw;
        }

        int hh = IN_WINDOW_HEIGHT / 2;
        if(y < -hh) {
            y = -hh;
        } else if(y > hh) {
            y = hh;
        }

        in_deltaX = x;
        in_deltaY = y;
    }
    else
    {
        mouse_pos_t x = in_positionX + dx;
        mouse_pos_t y = in_positionY + dy;

        if(x < 0) {
            x = 0;
        } else if(x >= IN_WINDOW_WIDTH) {
            x = IN_WINDOW_WIDTH - 1;
        }

        if(y < 0) {
            y = 0;
        } else if(y >= IN_WINDOW_HEIGHT) {
            y = IN_WINDOW_HEIGHT - 1;
        }

        in_positionX = x;
        in_positionY = y;

        set_mouse_cursor_position(in_positionX, in_positionY);
    }
}

#define ACTION_DOWN       0
#define ACTION_UP         1
#define ACTION_MOVE       2
#define ACTION_HOVER_MOVE 7
#define ACTION_SCROLL     8
#define BUTTON_PRIMARY    1
#define BUTTON_SECONDARY  2
#define BUTTON_TERTIARY   4
#define BUTTON_BACK       8
#define BUTTON_FORWARD    16

void Q3E_SDL_MotionEvent(float dx, float dy)
{
    // mouse motion event
    Q3E_SDL_MouseMotion(dx, dy);

    //if(!relativeMouseMode)
    {
        CALL_SDL(onNativeMouse, 0, ACTION_MOVE, relativeMouseMode ? in_deltaX : in_positionX, relativeMouseMode ? in_deltaY : in_positionY, relativeMouseMode);
    }
}

void Q3E_SDL_KeyEvent(int key, int down, int ch)
{
    if(key < 0)
    {
        CALL_SDL(onNativeMouse, -key, down ? ACTION_DOWN : ACTION_UP, relativeMouseMode ? in_deltaX : in_positionX, relativeMouseMode ? in_deltaY : in_positionY, relativeMouseMode);
        return;
    }
	
    if(down)
    {
        CALL_SDL(onNativeKeyDown, key);
		if(ch > 0)
		{
			const char text[] = { (char)ch, '\0' };
			CALL_SDL(connection_nativeCommitText, text, 1);
		}
    }
    else
    {
        CALL_SDL(onNativeKeyUp, key);
    }
}