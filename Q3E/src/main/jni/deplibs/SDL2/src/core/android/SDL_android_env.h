#ifndef SDL_ANDROID_ENV_H
#define SDL_ANDROID_ENV_H

#define ARRAY_LENGTH(x) *((int *)(((int *)x) - 1))
#define ARRAY_ELEMENT_LENGTH(x, t) (ARRAY_LENGTH(x)) / sizeof(t)
#define ARRAY_FREE(x) free(((int *)x) - 1)
#define ARRAY_ALLOC(v, t, l) t *v = (t *)calloc(l * sizeof(t) + 4, 1); ARRAY_LENGTH(v) = l; v = (t *)((char *)v + 4);

typedef struct SDL_Android_Callback_s
{
#define CALLBACK_METHOD(ret, name, args) ret (* name) args;
#include "SDL_android_callback.h"
} SDL_Android_Callback_t;

typedef struct SDL_Android_Interface_s
{
#define INTERFACE_METHOD(ret, name, args) ret (* name) args;
#include "SDL_android_interface.h"
} SDL_Android_Interface_t;

typedef void (* SDL_Android_GetAPI_f)(const SDL_Android_Callback_t *callbacks, SDL_Android_Interface_t *interfaces);

#ifdef __cplusplus
extern "C"
#endif
__attribute__((visibility("default"))) void SDL_Android_GetAPI(const SDL_Android_Callback_t *callbacks, SDL_Android_Interface_t *interfaces);

#endif
