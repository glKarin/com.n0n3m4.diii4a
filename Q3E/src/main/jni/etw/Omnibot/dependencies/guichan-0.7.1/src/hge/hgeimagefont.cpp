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

#include "guichan/hge/hgeimagefont.hpp"

#include "guichan/color.hpp"
#include "guichan/graphics.hpp"

namespace gcn
{
    HGEImageFont::HGEImageFont(const std::string& filename)
    {
        mHGEFont = new hgeFont(filename.c_str());
    }

    HGEImageFont::~HGEImageFont()
    {
        delete mHGEFont;
    }

    hgeFont *HGEImageFont::getFont() const
    {
        return mHGEFont;
    }

    int HGEImageFont::getWidth(const std::string &text) const
    {
        return static_cast<int>(mHGEFont->GetStringWidth(text.c_str())) + 1;
    }
  
    int HGEImageFont::getWidth(char character) const
    {
        char buf[2];

        sprintf(buf, "%c", character);

        return mHGEFont->GetStringWidth(buf);
    }

    int HGEImageFont::getHeight() const
    {
        return  static_cast<int>(mHGEFont->GetHeight());
    }

    int HGEImageFont::getStringIndexAt(const std::string& text, int x)
    {
        unsigned int i;
        int size = 0;

        for (i = 0; i < text.size(); ++i)
        {
            size += getWidth(text.at(i));

            if (size > x)
            {
                return i;
            }
        }

        return text.size();
    }

    void HGEImageFont::drawString(Graphics *graphics, const std::string &text, int x, int y)
    {
        ClipRectangle const &top = graphics->getCurrentClipArea();

        x += top.xOffset;
        y += top.yOffset;

        Color color = graphics->getColor();

        mHGEFont->SetColor(ARGB(color.a, color.r, color.g, color.b));
        mHGEFont->Render(x, y, HGETEXT_LEFT, text.c_str());
    }
}
