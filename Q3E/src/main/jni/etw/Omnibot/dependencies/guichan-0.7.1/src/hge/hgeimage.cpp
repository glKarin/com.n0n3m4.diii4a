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

#include "guichan/hge/hgeimage.hpp"

#include "guichan/exception.hpp"

namespace gcn
{
    HGE *HGEImage::mHGE = NULL;

    HGEImage::HGEImage(HTEXTURE texture, bool autoFree)
        : mTexture(texture),
          mAutoFree(autoFree)
    {
        mHGE = hgeCreate(HGE_VERSION);
    }

    HGEImage::~HGEImage()
    {
        if (mAutoFree)
        { 
            free();
        }

        mHGE->Release();
    }

    void HGEImage::free()
    {
        if (mHGESprite != NULL)
        {
            delete mHGESprite;
            mHGESprite =  NULL;
        }

        mHGE->Texture_Free(mTexture);
    }

    hgeSprite *HGEImage::getSprite() const
    {
        return mHGESprite;
    }

    int HGEImage::getWidth() const
    {
        return mHGE->Texture_GetWidth(mTexture, true);
    }

    int HGEImage::getHeight() const
    {
        return mHGE->Texture_GetHeight(mTexture, true);
    }

    Color HGEImage::getPixel(int x, int y)
    {
        DWORD *pLockPtr = mHGE->Texture_Lock(mTexture);

        if (pLockPtr == NULL)
        {
            throw GCN_EXCEPTION("Locking of the texture failed. HGE only support locking of 32bit textures.");
        }

        DWORD color = pLockPtr[x + y * mHGE->Texture_GetWidth(mTexture, true)];

        mHGE->Texture_Unlock(mTexture);

        return Color(GETR(color),
                     GETG(color), 
                     GETB(color), 
                     GETA(color ));
    }

    void  HGEImage::putPixel(int x, int y, const Color& color)
    {
        DWORD hardwareColor = ARGB(color.a, color.r, color.g, color.b);

        DWORD *pLockPtr = mHGE->Texture_Lock(mTexture, false);

        if (pLockPtr == NULL)
        {
            throw GCN_EXCEPTION("Locking of the texture failed. HGE only support locking of 32bit textures.");
        }

        pLockPtr[x + y * mHGE->Texture_GetWidth(mTexture, true)] = hardwareColor;

        mHGE->Texture_Unlock(mTexture);
    }

    void HGEImage::convertToDisplayFormat()
    {
        DWORD *pLockPtr = mHGE->Texture_Lock(mTexture);

        if (pLockPtr == NULL)
        {
            throw GCN_EXCEPTION("Locking of the texture failed. HGE only support locking of 32bit textures.");
        }

        int i;
        int end = mHGE->Texture_GetWidth(mTexture, true) *
            mHGE->Texture_GetHeight(mTexture, true);

        for (i = 0; i < end; i++)
        {
            DWORD color = pLockPtr[i];
            if (GETR(color) == 0xFF
                && GETG(color) == 0x00
                && GETB(color) == 0xFF)
            {
                pLockPtr[i] = ARGB(0x00, 0x00, 0x00, 0x00);
            }
        }

        mHGE->Texture_Unlock(mTexture);

        mHGESprite = new hgeSprite(mTexture,  
                                   0, 
                                   0, 
                                   mHGE->Texture_GetWidth(mTexture, true),
                                   mHGE->Texture_GetHeight(mTexture, true));
    }
}
