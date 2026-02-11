#ifndef _Q3E_SDL2_H
#define _Q3E_SDL2_H

#define INTERFACE_METHOD(ret, name, args) extern ret (* name) args;
#include "../deplibs/SDL2/src/core/android/SDL_android_interface.h"

#define Q3E_SDL_AUDIO_DRIVER_DEFAULT 0
#define Q3E_SDL_AUDIO_DRIVER_OPENSLES 1
#define Q3E_SDL_AUDIO_DRIVER_AAUDIO 2

extern int USING_SDL;
#define CALL_SDL(name, ...) if(name) name(__VA_ARGS__)
#define EXEC_SDL(name, ...) if(USING_SDL) name(__VA_ARGS__)
#define CALLRET_SDL(name, def, ...) name ? name(__VA_ARGS__) : (def)
#define INIT_SDL() Q3E_InitSDL()

void Q3E_InitSDL(void);
void Q3E_ShutdownSDL(void);
//void SDL_Register(JNIEnv *env);
void Q3E_InitSDLJava(JNIEnv *env, jclass q3eCallbackClass);

void Q3E_SDL_SetAudioDriver(int driver);

#endif