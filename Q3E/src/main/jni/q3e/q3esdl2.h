#ifndef _Q3E_SDL2_H
#define _Q3E_SDL2_H

#define Q3E_TRUE 1
#define Q3E_FALSE 0

#define INTERFACE_METHOD(ret, name, args) ret (* name) args;
#include "../deplibs/SDL2/src/core/android/SDL_android_interface.h"

extern int USING_SDL;
#define CALL_SDL(name, ...) if(name) name(__VA_ARGS__)
#define EXEC_SDL(name, ...) if(USING_SDL) name(__VA_ARGS__)
#define INIT_SDL() Q3E_InitSDL()

void Q3E_InitSDL(void);

void Q3E_SDL_MotionEvent(float dx, float dy);
void Q3E_SDL_KeyEvent(int key, int down);
void Q3E_SDL_SetWindowSize(int w, int h);
void Q3E_SDL_SetRelativeMouseMode(int on);

#endif