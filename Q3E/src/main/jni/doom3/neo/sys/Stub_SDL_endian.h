// karin: for compat with dhewm3
#ifndef STUB_SDL_ENDIAN_H
#define STUB_SDL_ENDIAN_H

#define SDL_LIL_ENDIAN  1234
#define SDL_BIG_ENDIAN  4321

#if BUILD_IS_BIG_ENDIAN // this is from config.h, set by cmake
#define SDL_BYTEORDER SDL_BIG_ENDIAN
#else
#define SDL_BYTEORDER SDL_LIL_ENDIAN
#endif

#endif // STUB_SDL_ENDIAN_H
