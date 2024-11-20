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

#include "guichan/graphics.hpp"

#include "guichan/exception.hpp"
#include "guichan/font.hpp"
#include "guichan/image.hpp"

namespace gcn
{

    Graphics::Graphics()
    {
        mFont = NULL;
    }

    bool Graphics::pushClipArea(Rectangle area)
    {
        if (mClipStack.empty())
        {
            ClipRectangle carea;
            carea.x = area.x;
            carea.y = area.y;
            carea.width = area.width;
            carea.height = area.height;
			carea.xOffset = area.x;
			carea.yOffset = area.y;
            mClipStack.push(carea);
            return true;
        }

        const ClipRectangle &top = mClipStack.top();
        ClipRectangle carea;
        carea = area;
        carea.xOffset = top.xOffset + carea.x;
        carea.yOffset = top.yOffset + carea.y;
        carea.x += top.xOffset;
        carea.y += top.yOffset;

        // Clamp the pushed clip rectangle.
        if (carea.x < top.x)
        {
            carea.x = top.x;
        }
        
        if (carea.y < top.y)
        {
            carea.y = top.y;            
        }
                
        if (carea.x + carea.width > top.x + top.width)
        {
            carea.width = top.x + top.width - carea.x;

			if (carea.width < 0)
			{
				carea.width = 0;
			}
        }
        
        if (carea.y + carea.height > top.y + top.height)
        {
            carea.height = top.y + top.height - carea.y;

			if (carea.height < 0)
			{
				carea.height = 0;
			}
        }

        bool result = carea.isIntersecting(top);

        mClipStack.push(carea);

        return result;
    }

    void Graphics::popClipArea()
    {

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Tried to pop clip area from empty stack.");
        }

        mClipStack.pop();
    }

    const ClipRectangle& Graphics::getCurrentClipArea()
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("The clip area stack is empty.");
        }

        return mClipStack.top();
    }

    void Graphics::drawImage(const Image* image, int dstX, int dstY)
    {
        drawImage(image, 0, 0, dstX, dstY, image->getWidth(), image->getHeight());
    }

    void Graphics::setFont(Font* font)
    {
        mFont = font;
    }

    void Graphics::drawText(const std::string& text, int x, int y,
                            Alignment alignment)
    {
        if (mFont == NULL)
        {
            throw GCN_EXCEPTION("No font set.");
        }

        switch (alignment)
        {
          case LEFT:
              mFont->drawString(this, text, x, y);
              break;
          case CENTER:
              mFont->drawString(this, text, x - mFont->getWidth(text) / 2, y);
              break;
          case RIGHT:
              mFont->drawString(this, text, x - mFont->getWidth(text), y);
              break;
          default:
              throw GCN_EXCEPTION("Unknown alignment.");
        }
    }
}
