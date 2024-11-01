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

#include "guichan/allegro/allegrographics.hpp"
#include "guichan/allegro/allegroimage.hpp"
#include "guichan/rectangle.hpp"
#include "guichan/exception.hpp"
#include "guichan/cliprectangle.hpp"
#include "guichan/color.hpp"

namespace gcn
{
    AllegroGraphics::AllegroGraphics()
    {
        mTarget = NULL;
        mClipNull = false;
    }

    AllegroGraphics::AllegroGraphics(BITMAP *target)
    {
        mTarget = target;
    }

    AllegroGraphics::~AllegroGraphics()
    {
    }

    void AllegroGraphics::setTarget(BITMAP *target)
    {
        mTarget = target;
    }

    BITMAP *AllegroGraphics::getTarget()
    {
        return mTarget;
    }

    void AllegroGraphics::_beginDraw()
    {
        if (mTarget == NULL)
        {
            throw GCN_EXCEPTION("Target BITMAP is null, set it with setTarget first.");
        }

        // push a clip area the size of the target bitmap
        pushClipArea(Rectangle(0, 0, mTarget->w, mTarget->h));
    }

    void AllegroGraphics::_endDraw()
    {
        // pop the clip area pushed in _beginDraw
        popClipArea();
    }

    bool AllegroGraphics::pushClipArea(Rectangle area)
    {
        bool result = Graphics::pushClipArea(area);

        const ClipRectangle& cr = mClipStack.top();

        // Allegro won't let you set clip areas
        // that have zero width or height
        // so we have to check for that.
        if (cr.width == 0 || cr.height == 0)
        {
            mClipNull = true;
        }
        else
        {
            mClipNull = false;
#if ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0
            set_clip(mTarget,
                     cr.x,
                     cr.y,
                     cr.x + cr.width - 1,
                     cr.y + cr.height - 1);
#else
            set_clip_rect(mTarget,
                          cr.x,
                          cr.y,
                          cr.x + cr.width - 1,
                          cr.y + cr.height - 1);
#endif
        }

        return result;
    }

    void AllegroGraphics::popClipArea()
    {
        Graphics::popClipArea();

        if (mClipStack.empty())
        {
            return;
        }

        const ClipRectangle& cr = mClipStack.top();

        // Allegro won't let you set clip areas
        //that have zero width or height
        // so we have to check for that.
        if (cr.width == 0 || cr.height == 0)
        {
            mClipNull = true;
        }
        else
        {
            mClipNull = false;
#if ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 0
            set_clip(mTarget,
                     cr.x,
                     cr.y,
                     cr.x + cr.width - 1,
                     cr.y + cr.height - 1);
#else
            set_clip_rect(mTarget,
                          cr.x,
                          cr.y,
                          cr.x + cr.width - 1,
                          cr.y + cr.height - 1);
#endif
        }
    }

    void AllegroGraphics::drawImage(const Image* image,
                                    int srcX,
                                    int srcY,
                                    int dstX,
                                    int dstY,
                                    int width,
                                    int height)
    {
        if (mClipNull)
        {
            return;
        }

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion "
                                "outside of _beginDraw() and _endDraw()?");
        }

        const int xOffset = mClipStack.top().xOffset;
        const int yOffset = mClipStack.top().yOffset;

        const AllegroImage* srcImage = dynamic_cast<const AllegroImage*>(image);

        if (srcImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an AllegroImage.");
        }

        masked_blit(srcImage->getBitmap(),
                    mTarget,
                    srcX,
                    srcY,
                    dstX + xOffset,
                    dstY + yOffset,
                    width,
                    height);
    }

    void AllegroGraphics::drawPoint(int x, int y)
    {
        if (mClipNull)
        {
            return;
        }

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion "
                                "outside of _beginDraw() and _endDraw()?");
        }

        const int xOffset = mClipStack.top().xOffset;
        const int yOffset = mClipStack.top().yOffset;

        putpixel(mTarget,
                 x + xOffset,
                 y + yOffset,
                 mAllegroColor);
    }

    void AllegroGraphics::drawLine(int x1, int y1, int x2, int y2)
    {
        if (mClipNull)
        {
            return;
        }

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion "
                                "outside of _beginDraw() and _endDraw()?");
        }

        const int xOffset = mClipStack.top().xOffset;
        const int yOffset = mClipStack.top().yOffset;

        line(mTarget,
             x1 + xOffset,
             y1 + yOffset,
             x2 + xOffset,
             y2 + yOffset,
             mAllegroColor);
    }

    void AllegroGraphics::drawRectangle(const Rectangle& rectangle)
    {
        if (mClipNull)
        {
            return;
        }

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion " 
                                "outside of _beginDraw() and _endDraw()?");
        }

        const int xOffset = mClipStack.top().xOffset;
        const int yOffset = mClipStack.top().yOffset;

        rect(mTarget,
             rectangle.x + xOffset,
             rectangle.y + yOffset,
             rectangle.x + rectangle.width - 1 + xOffset,
             rectangle.y + rectangle.height - 1 + yOffset,
             mAllegroColor);
    }

    void AllegroGraphics::fillRectangle(const Rectangle& rectangle)
    {
        if (mClipNull)
        {
            return;
        }

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion " 
                                "outside of _beginDraw() and _endDraw()?");
        }

        const int xOffset = mClipStack.top().xOffset;
        const int yOffset = mClipStack.top().yOffset;

        rectfill(mTarget,
                 rectangle.x + xOffset,
                 rectangle.y + yOffset,
                 rectangle.x + rectangle.width - 1 + xOffset,
                 rectangle.y + rectangle.height - 1 + yOffset,
                 mAllegroColor);
    }

    void AllegroGraphics::setColor(const Color& color)
    {
        mColor = color;
        mAllegroColor = makecol(color.r, color.g, color.b);

        if (color.a != 255)
        {
            set_trans_blender(255, 255, 255, color.a);
            drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
        }
        else
        {
            solid_mode();
        }
    }

    const Color& AllegroGraphics::getColor() const
    {
        return mColor;
    }

    int AllegroGraphics::getAllegroColor() const
    {
        return mAllegroColor;
    }

    void AllegroGraphics::drawBitmap(BITMAP* bitmap, int dstX, int dstY)
    {
        const int xOffset = mClipStack.top().xOffset;
        const int yOffset = mClipStack.top().yOffset;

        masked_blit(bitmap,
                    mTarget,
                    0,
                    0,
                    dstX + xOffset,
                    dstY + yOffset,
                    bitmap->w,
                    bitmap->h);
    }
}
