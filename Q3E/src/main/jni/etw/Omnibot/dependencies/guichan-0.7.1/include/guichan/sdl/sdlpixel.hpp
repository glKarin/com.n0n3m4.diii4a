/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naessén and Per Larsson
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naessén a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GCN_SDLPIXEL_HPP
#define GCN_SDLPIXEL_HPP

#include "SDL.h"
#include "guichan/color.hpp"

namespace gcn
{

    /**
     * Checks a pixels color of an SDL_Surface.
     *
     * @param surface an SDL_Surface where to check for a pixel color.
     * @param x the x coordinate on the surface.
     * @param y the y coordinate on the surface.
     * @return a color of a pixel.
     */
    inline const Color SDLgetPixel(SDL_Surface* surface, int x, int y)
    {
        int bpp = surface->format->BytesPerPixel;

        SDL_LockSurface(surface);

        Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

        unsigned int color = 0;

        switch(bpp)
        {
          case 1:
              color = *p;
              break;

          case 2:
              color = *(Uint16 *)p;
              break;

          case 3:
              if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
              {
                  color = p[0] << 16 | p[1] << 8 | p[2];
              }
              else
              {
                  color = p[0] | p[1] << 8 | p[2] << 16;
              }
              break;

          case 4:
              color = *(Uint32 *)p;
              break;

        }

        unsigned char r,g,b,a;

        SDL_GetRGBA(color, surface->format, &r, &g, &b, &a);
        SDL_UnlockSurface(surface);

        return Color(r,g,b,a);
    }

    /**
     * Puts a pixel on an SDL_Surface.
     *
     * @param x the x coordinate on the surface.
     * @param y the y coordinate on the surface.
     * @param color the color the pixel should be in.
     */
    inline void SDLputPixel(SDL_Surface* surface, int x, int y, const Color& color)
    {
        int bpp = surface->format->BytesPerPixel;

        SDL_LockSurface(surface);

        Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

        Uint32 pixel = SDL_MapRGB(surface->format, color.r, color.g, color.b);

        switch(bpp)
        {
          case 1:
              *p = pixel;
              break;

          case 2:
              *(Uint16 *)p = pixel;
              break;

          case 3:
              if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
              {
                  p[0] = (pixel >> 16) & 0xff;
                  p[1] = (pixel >> 8) & 0xff;
                  p[2] = pixel & 0xff;
              }
              else
              {
                  p[0] = pixel & 0xff;
                  p[1] = (pixel >> 8) & 0xff;
                  p[2] = (pixel >> 16) & 0xff;
              }
              break;

          case 4:
              *(Uint32 *)p = pixel;
              break;
        }

        SDL_UnlockSurface(surface);
    }

    /**
     * Blends two 32 bit colors together.
     *
     * @param src the source color.
     * @param dst the destination color.
     * @param a alpha.
     */
    inline unsigned int SDLAlpha32(unsigned int src, unsigned int dst, unsigned char a)
    {
        unsigned int b = ((src & 0xff) * a + (dst & 0xff) * (255 - a)) >> 8;
        unsigned int g = ((src & 0xff00) * a + (dst & 0xff00) * (255 - a)) >> 8;
        unsigned int r = ((src & 0xff0000) * a + (dst & 0xff0000) * (255 - a)) >> 8;

        return (b & 0xff) | (g & 0xff00) | (r & 0xff0000);
    }

    /**
     * Blends two 16 bit colors together.
     *
     * @param src the source color.
     * @param dst the destination color.
     * @param a alpha.
     */
    inline unsigned short SDLAlpha16(unsigned short src, unsigned short dst, unsigned char a, const SDL_PixelFormat *f)
    {
        unsigned int b = ((src & f->Rmask) * a + (dst & f->Rmask) * (255 - a)) >> 8;
        unsigned int g = ((src & f->Gmask) * a + (dst & f->Gmask) * (255 - a)) >> 8;
        unsigned int r = ((src & f->Bmask) * a + (dst & f->Bmask) * (255 - a)) >> 8;

        return (unsigned short)((b & f->Rmask) | (g & f->Gmask) | (r & f->Bmask));
    }

    /*
    typedef struct{
        SDL_Palette *palette;
        Uint8  BitsPerPixel;
        Uint8  BytesPerPixel;
        Uint32 Rmask, Gmask, Bmask, Amask;
        Uint8  Rshift, Gshift, Bshift, Ashift;
        Uint8  Rloss, Gloss, Bloss, Aloss;
        Uint32 colorkey;
        Uint8  alpha;
    } SDL_PixelFormat;
    */

    /**
     * Puts a pixel on an SDL_Surface with alpha
     *
     * @param x the x coordinate on the surface.
     * @param y the y coordinate on the surface.
     * @param color the color the pixel should be in.
     */
    inline void SDLputPixelAlpha(SDL_Surface* surface, int x, int y, const Color& color)
    {
        int bpp = surface->format->BytesPerPixel;

        SDL_LockSurface(surface);

        Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

        Uint32 pixel = SDL_MapRGB(surface->format, color.r, color.g, color.b);

        switch(bpp)
        {
          case 1:
              *p = pixel;
              break;

          case 2:
              *(Uint16 *)p = SDLAlpha16(pixel, *(Uint32 *)p, color.a, surface->format);
              break;

          case 3:
              if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
              {
                  unsigned int r = (p[0] * (255 - color.a) + color.r * color.a) >> 8;
                  unsigned int g = (p[1] * (255 - color.a) + color.g * color.a) >> 8;
                  unsigned int b = (p[2] * (255 - color.a) + color.b * color.a) >> 8;

                  p[2] = b;
                  p[1] = g;
                  p[0] = r;
              }
              else
              {
                  unsigned int r = (p[2] * (255 - color.a) + color.r * color.a) >> 8;
                  unsigned int g = (p[1] * (255 - color.a) + color.g * color.a) >> 8;
                  unsigned int b = (p[0] * (255 - color.a) + color.b * color.a) >> 8;

                  p[0] = b;
                  p[1] = g;
                  p[2] = r;
              }
              break;

          case 4:
              *(Uint32 *)p = SDLAlpha32(pixel, *(Uint32 *)p, color.a);
              break;
        }

        SDL_UnlockSurface(surface);
    }
}

#endif // end GCN_SDLPIXEL_HPP
