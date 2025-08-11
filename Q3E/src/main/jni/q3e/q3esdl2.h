#ifndef _Q3E_SDL2_H
#define _Q3E_SDL2_H

#define Q3E_TRUE 1
#define Q3E_FALSE 0

#define INTERFACE_METHOD(ret, name, args) ret (* name) args;
#include "../deplibs/SDL2/src/core/android/SDL_android_interface.h"

#define Q3E_SDL_AUDIO_DRIVER_DEFAULT 0
#define Q3E_SDL_AUDIO_DRIVER_OPENSLES 1
#define Q3E_SDL_AUDIO_DRIVER_AAUDIO 2

extern int USING_SDL;
#define CALL_SDL(name, ...) if(name) name(__VA_ARGS__)
#define EXEC_SDL(name, ...) if(USING_SDL) name(__VA_ARGS__)
#define INIT_SDL() Q3E_InitSDL()

void Q3E_InitSDL(void);
void Q3E_ShutdownSDL(void);

void Q3E_SDL_MotionEvent(float dx, float dy);
void Q3E_SDL_MouseEvent(float x, float y);
void Q3E_SDL_KeyEvent(int key, int down, int ch);
void Q3E_SDL_SetWindowSize(int w, int h);
void Q3E_SDL_SetRelativeMouseMode(int on);
void Q3E_SDL_SetAudioDriver(int driver);
void Q3E_SDL_TextEvent(JNIEnv *env, jstring text);
void Q3E_SDL_CharEvent(int ch);
void Q3E_SDL_WheelEvent(float x, float y);

#endif