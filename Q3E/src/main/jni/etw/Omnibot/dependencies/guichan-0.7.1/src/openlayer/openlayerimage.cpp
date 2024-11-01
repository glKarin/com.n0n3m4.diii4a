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

#include "guichan/openlayer/openlayerimage.hpp"

#include "guichan/exception.hpp"

namespace gcn
{
    OpenLayerImage::OpenLayerImage(BITMAP* allegroBitmap, bool autoFree)
            : mOpenLayerBitmap(NULL),
              mAllegroBitmap(allegroBitmap),
              mAutoFree(autoFree)
        
    {
        if (mAllegroBitmap == NULL)
        {
            throw GCN_EXCEPTION(std::string("Unable to load bitmap."));
        }
    }

    OpenLayerImage::~OpenLayerImage()
    {
        if (mAutoFree)
        {
            free();
        }
    }

    ol::Bitmap* OpenLayerImage::getBitmap() const
    {
        return mOpenLayerBitmap;
    }

    void OpenLayerImage::free()
    {
        if (mOpenLayerBitmap != NULL)
        {
            delete mOpenLayerBitmap;
            mOpenLayerBitmap = NULL;
        }
        
        if (mAllegroBitmap != NULL)
        {
            destroy_bitmap(mAllegroBitmap);
            mAllegroBitmap = NULL;
        }
    }

    int OpenLayerImage::getWidth() const
    {
        if (mAllegroBitmap == NULL && mOpenLayerBitmap == NULL)
        {
            throw GCN_EXCEPTION("Trying to get the width of a non loaded image.");
        }

        if (mOpenLayerBitmap == NULL)
        {
            return mAllegroBitmap->w;
        }

        return mOpenLayerBitmap->Width();
    }

    int OpenLayerImage::getHeight() const
    {
        if (mAllegroBitmap == NULL && mOpenLayerBitmap == NULL)
        {
            throw GCN_EXCEPTION("Trying to get the height of a non loaded image.");
        }
        
        if (mOpenLayerBitmap == NULL)
        {
            return mAllegroBitmap->h;
        }

        return mOpenLayerBitmap->Height();
    }

    Color OpenLayerImage::getPixel(int x, int y)
    {
        if (mAllegroBitmap == NULL && mOpenLayerBitmap == NULL)
        {
            throw GCN_EXCEPTION("Trying to get a pixel from a non loaded image.");
        }

        if (mOpenLayerBitmap == NULL)
        {
            int color = getpixel(mAllegroBitmap, x, y);
            return Color(getr(color), getg(color), getb(color), 255);
        }
        
        return Color(mOpenLayerBitmap->GetPixelPacked(x, y));

    }

    void OpenLayerImage::putPixel(int x, int y, const Color& color)
    {
        if (mAllegroBitmap == NULL && mOpenLayerBitmap == NULL)
        {
            throw GCN_EXCEPTION("Trying to put a pixel in a non loaded image.");
        }

        if (mAllegroBitmap == NULL)
        {
            return;
        }

        int c = makeacol_depth(32, color.r, color.g, color.b, color.a);

        putpixel(mAllegroBitmap, x, y, c);
    }

    void OpenLayerImage::convertToDisplayFormat()
    {
        if (mAllegroBitmap == NULL)
        {
            throw GCN_EXCEPTION("Trying to convert a non loaded image to display format.");
        }

        if (mOpenLayerBitmap != NULL)
        {
            throw GCN_EXCEPTION("Trying to convert an image to display format which has already been converted.");
        }

        mOpenLayerBitmap = new ol::Bitmap(mAllegroBitmap, false, true);
        mOpenLayerBitmap->SendToGPU();
        
        destroy_bitmap(mAllegroBitmap);
        mAllegroBitmap = NULL;
    }
}
