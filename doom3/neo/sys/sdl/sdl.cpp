#include <SDL.h>

#include "../../idlib/precompiled.h"
#include "../sys_public.h"

extern void Sys_InitThreads();

void Sys_InitSDL(void)
{
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)) // init joystick to work around SDL 2.0.9 bug #4391
        Sys_Error("Error while initializing SDL: %s", SDL_GetError());

    Sys_InitThreads();
}