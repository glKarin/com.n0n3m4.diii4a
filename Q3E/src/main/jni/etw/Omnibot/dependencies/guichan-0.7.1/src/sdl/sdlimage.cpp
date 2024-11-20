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

/*
 * For comments regarding functions please see the header file.
 */

#include "guichan/sdl/sdlimage.hpp"

#include "guichan/exception.hpp"
#include "guichan/sdl/sdlpixel.hpp"

namespace gcn
{
    SDLImage::SDLImage(SDL_Surface* surface, bool autoFree)
    {
        mAutoFree = autoFree;
        mSurface = surface;
    }

    SDLImage::~SDLImage()
    {
        if (mAutoFree)
        {
            free();
        }
    }

    SDL_Surface* SDLImage::getSurface() const
    {
        return mSurface;
    }

    int SDLImage::getWidth() const
    {
        if (mSurface == NULL)
        {
            throw GCN_EXCEPTION("Trying to get the width of a non loaded image.");
        }

        return mSurface->w;
    }

    int SDLImage::getHeight() const
    {
        if (mSurface == NULL)
        {
            throw GCN_EXCEPTION("Trying to get the height of a non loaded image.");
        }

        return mSurface->h;
    }

    Color SDLImage::getPixel(int x, int y)
    {
        if (mSurface == NULL)
        {
            throw GCN_EXCEPTION("Trying to get a pixel from a non loaded image.");
        }

        return SDLgetPixel(mSurface, x, y);
    }

    void SDLImage::putPixel(int x, int y, const Color& color)
    {
        if (mSurface == NULL)
        {
            throw GCN_EXCEPTION("Trying to put a pixel in a non loaded image.");
        }

        SDLputPixel(mSurface, x, y, color);
    }

    void SDLImage::convertToDisplayFormat()
    {
        if (mSurface == NULL)
        {
            throw GCN_EXCEPTION("Trying to convert a non loaded image to display format.");
        }

        int i;
        bool hasPink = false;
        bool hasAlpha = false;

        for (i = 0; i < mSurface->w * mSurface->h; ++i)
        {
            if (((unsigned int*)mSurface->pixels)[i] == SDL_MapRGB(mSurface->format,255,0,255))
            {
                hasPink = true;
                break;
            }
        }

        for (i = 0; i < mSurface->w * mSurface->h; ++i)
        {
            Uint8 r, g, b, a;

            SDL_GetRGBA(((unsigned int*)mSurface->pixels)[i], mSurface->format,
                        &r, &g, &b, &a);

            if (a != 255)
            {
                hasAlpha = true;
                break;
            }
        }

        SDL_Surface *tmp;

        if (hasAlpha)
        {
            tmp = SDL_DisplayFormatAlpha(mSurface);
            SDL_FreeSurface(mSurface);
            mSurface = NULL;
        }
        else
        {
            tmp = SDL_DisplayFormat(mSurface);
            SDL_FreeSurface(mSurface);
            mSurface = NULL;
        }

        if (hasPink)
        {
            SDL_SetColorKey(tmp, SDL_SRCCOLORKEY,
                            SDL_MapRGB(tmp->format,255,0,255));
        }
        if (hasAlpha)
        {
            SDL_SetAlpha(tmp, SDL_SRCALPHA, 255);
        }

        mSurface = tmp;
    }

    void SDLImage::free()
    {
        SDL_FreeSurface(mSurface);
    }
}
