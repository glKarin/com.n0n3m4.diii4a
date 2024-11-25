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

#include "guichan/hge/hgegraphics.hpp"

#include "guichan/cliprectangle.hpp"
#include "guichan/exception.hpp"
#include "guichan/rectangle.hpp"
#include "guichan/hge/hgeimage.hpp"

namespace gcn
{
    HGE *HGEGraphics::mHGE = NULL;

    HGEGraphics::HGEGraphics()
        :mClipNull(false)
    {
        mHGE = hgeCreate(HGE_VERSION);

        mHardwareColor = 0;
    }

    HGEGraphics::~HGEGraphics()
    {
        mHGE->Release();
    }

    void HGEGraphics::_beginDraw()
    {
        pushClipArea(Rectangle(0, 
                               0,
                               mHGE->System_GetState(HGE_SCREENWIDTH), 
                               mHGE->System_GetState(HGE_SCREENHEIGHT)));
    }

    void HGEGraphics::_endDraw()
    {
        // pop the clip area pushed in _beginDraw
        popClipArea();
    }

    bool HGEGraphics::pushClipArea(Rectangle area)
    {
        bool result = Graphics::pushClipArea(area);

        const ClipRectangle top = mClipStack.top();

        // HGE won't let you set clip areas
        // that have zero width or height
        // so we have to check for that.
        if (top.width == 0 || top.height == 0)
        {
            mClipNull = true;
        }
        else
        {
            mClipNull = false;
            mHGE->Gfx_SetTransform(top.xOffset,
                                   top.yOffset);
            mHGE->Gfx_SetClipping(top.x, 
                                  top.y, 
                                  top.width, 
                                  top.height);
        }
        return result;
    }

    void HGEGraphics::popClipArea()
    {
        Graphics::popClipArea();

        if (mClipStack.empty())
        {
            mHGE->Gfx_SetClipping();

            return;
        }
        else
        {
            const ClipRectangle top = mClipStack.top();

            // HGE won't let you set clip areas
            //that have zero width or height
            // so we have to check for that.
            if (top.width == 0 || top.height == 0)
            {
                mClipNull = true;
            }
            else
            {
                mClipNull = false;
                mHGE->Gfx_SetTransform(top.xOffset,
                                       top.yOffset);
                mHGE->Gfx_SetClipping(top.x, 
                                      top.y, 
                                      top.width, 
                                      top.height);
            }
        }
    }

    void HGEGraphics::drawImage(const Image *image, 
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

        const HGEImage *hgeImage = static_cast<const HGEImage*>(image);

        if (hgeImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an HGEImage.");
        }

        ClipRectangle const top = mClipStack.top();

        dstX += top.xOffset;
        dstY += top.yOffset;

        hgeImage->getSprite()->SetTextureRect(srcX, srcY, width, height);
        hgeImage->getSprite()->Render(dstX, dstY);
    }

    void HGEGraphics::drawImage(const Image *image, int dstX, int dstY)
    {
        if (mClipNull)
        {
            return;
        }
       
        drawImage(image, 0, 0, dstX, dstY, image->getWidth(), image->getHeight());
    }

    void HGEGraphics::drawPoint(int x, int y)
    {
        if (mClipNull)
        {
            return;
        }

        ClipRectangle const top = mClipStack.top();

        x += top.xOffset;
        y += top.yOffset;

        mHGE->Gfx_RenderLine(x, y, x + 1, y, mHardwareColor);
    }

    void HGEGraphics::drawLine(int x1, int y1, int x2, int y2)
    {
        if (mClipNull)
        {
            return;
        }

        // As HGE omits the last pixel we need to adjust the coordinates
        // before drawing the line. If it's a vertical or horizontal line
        // all we have to do is add the omitted pixel.
        if (y1 == y2 || x1 == x2)
        {
            x2++;
            y2++;
        }
        // If it's not a vertical or horizontal line it gets a little bit
        // trickier.
        else 
        {
            // If y2 is greater than y1 then we know y2 will be omitted as
            // it will be a part of the last pixel coordinate.
            if (y2 > y1)
            {
                y2++;
            }
            // Else will y1 be omitted.
            else
            {
                y1++;
            }
            // The same thing applies for the x coordinates. If x2 is greater 
            // than x1 then we know x2 will be omitted as it will be a part of
            // the last pixel coordinate.
            if (x2 > x1)
            {
                x2++;
            }
            // Else will x1 be omitted.
            else
            {
                x1++;
            }
        }
       
        ClipRectangle const top = mClipStack.top();

        x1 += top.xOffset;
        y1 += top.yOffset;
        x2 += top.xOffset;
        y2 += top.yOffset;

        mHGE->Gfx_RenderLine(x1, y1, x2, y2, mHardwareColor);
    }

    void HGEGraphics::drawRectangle(const Rectangle &rectangle)
    {
        if (mClipNull)
        {
            return;
        }

        int x1 = rectangle.x;
        int y1 = rectangle.y;
        int x2 = rectangle.x + rectangle.width;
        int y2 = rectangle.y + rectangle.height;

        ClipRectangle const top = mClipStack.top();

        x1 += top.xOffset;
        y1 += top.yOffset;
        x2 += top.xOffset;
        y2 += top.yOffset;
     
        mHGE->Gfx_RenderLine(x1, y1 + 1, x2, y1, mHardwareColor);
        mHGE->Gfx_RenderLine(x2, y1 + 1, x2, y2 - 1, mHardwareColor);
        mHGE->Gfx_RenderLine(x2, y2, x1 + 1, y2, mHardwareColor);
        mHGE->Gfx_RenderLine(x1 + 1, y2, x1 + 1, y1 + 1, mHardwareColor);
    }

    void HGEGraphics::fillRectangle(const Rectangle &rectangle)
    {
        if (mClipNull)
        {
            return;
        }

        // We need to compensate for the fact that HGE doesn't
        // seem to include the the coordinate pixels when rendering
        // a quad.
        int x1 = rectangle.x;
        int y1 = rectangle.y;
        int x2 = rectangle.x + rectangle.width;
        int y2 = rectangle.y + rectangle.height;

        ClipRectangle const top = mClipStack.top();

        x1 += top.xOffset;
        y1 += top.yOffset;
        x2 += top.xOffset;
        y2 += top.yOffset;

        hgeQuad quad;

        quad.tex = NULL;

        quad.v[0].x   = x1;
        quad.v[0].y   = y1;
        quad.v[0].col = mHardwareColor;

        quad.v[1].x   = x2;
        quad.v[1].y   = y1;
        quad.v[1].col = mHardwareColor;

        quad.v[2].x   = x2;
        quad.v[2].y   = y2;
        quad.v[2].col = mHardwareColor;

        quad.v[3].x   = x1;
        quad.v[3].y   = y2;
        quad.v[3].col = mHardwareColor;

        int i;
        for (i = 0; i < 4; ++i)
        {
            quad.v[i].z = 0.5f;
        }

        quad.blend = BLEND_DEFAULT;

        mHGE->Gfx_RenderQuad(&quad);
    }

    void HGEGraphics::setColor(const Color &color)
    {
        mColor = color;

        mHardwareColor = ARGB(color.a, color.r, color.g, color.b);  
    }

    const Color& HGEGraphics::getColor() const
    {
        return  mColor;
    }
}
