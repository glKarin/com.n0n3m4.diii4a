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

#include "guichan/openlayer/openlayerfont.hpp"
#include "guichan/openlayer/openlayergraphics.hpp"

#include "guichan/exception.hpp"

namespace gcn
{
    OpenLayerTTFont::OpenLayerTTFont(const std::string& filename, int size)
      : mTextRenderer(filename.c_str(), size, size)
    {
        if (!mTextRenderer.IsValid())
        {
            throw GCN_EXCEPTION("Unable to load font.");
        }
    }

    OpenLayerTTFont::OpenLayerTTFont(const std::string& filename, int width, int height)
        : mTextRenderer(filename.c_str(), width, height)
    {
        if (!mTextRenderer.IsValid())
        {
            throw GCN_EXCEPTION("Unable to load font.");
        }
    }

    OpenLayerTTFont::~OpenLayerTTFont()
    {

    }

    int OpenLayerTTFont::getWidth(const std::string& text) const
    {
        return mTextRenderer.Width(text);
    }

    int OpenLayerTTFont::getHeight() const
    {
        return mTextRenderer.FontTotalHeight();
    }

    void OpenLayerTTFont::drawString(Graphics* graphics, const std::string& text, int x, int y)
    {
        const ClipRectangle& rec = graphics->getCurrentClipArea();

        OpenLayerGraphics* const openLayerGraphics = dynamic_cast<gcn::OpenLayerGraphics*>(graphics);
        if (!openLayerGraphics)
        {
            throw GCN_EXCEPTION("Graphics is not of type OpenLayerGraphics");
        }
        
        mTextRenderer.SetColor(openLayerGraphics->getOpenLayerColor());
        mTextRenderer.Print(text, 
                            x + rec.xOffset, 
                            y + rec.yOffset + mTextRenderer.Height(text));
    }
}
