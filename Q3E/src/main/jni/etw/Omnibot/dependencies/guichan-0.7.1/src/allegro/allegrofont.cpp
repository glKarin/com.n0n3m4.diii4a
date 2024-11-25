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

#include "guichan/allegro/allegrofont.hpp"
#include "guichan/allegro/allegrographics.hpp"

#include "guichan/exception.hpp"

namespace gcn
{

    AllegroFont::AllegroFont(FONT* font)
            :mAutoFree(false),
             mAllegroFont(font)
          
    {
        if (font == NULL)
        {
            throw GCN_EXCEPTION("Allegro font is not usable. Have you forgotten to load it?");
        }
    }

    AllegroFont::AllegroFont(const std::string& filename)
            :mAutoFree(true),
             mAllegroFont(load_font(filename.c_str(), 0, 0))
          
    {
        if (mAllegroFont == NULL)
        {
            throw GCN_EXCEPTION("Unable to load Allegro font from file.");
        }
    }

    AllegroFont::~AllegroFont()
    {
        if (mAutoFree)
        {
            if (mAllegroFont != NULL)
            {
                destroy_font(mAllegroFont);
            }

            mAllegroFont = NULL;
        }
    }

    int AllegroFont::getWidth(const std::string& text) const
    {
        return text_length(mAllegroFont, text.c_str());
    }

    int AllegroFont::getHeight() const
    {
        return text_height(mAllegroFont);
    }

    void AllegroFont::drawString(gcn::Graphics* graphics, const std::string& text, int x, int y)
    {
        gcn::AllegroGraphics* const allegroGraphics = dynamic_cast<gcn::AllegroGraphics*>(graphics);
        if (!allegroGraphics)
        {
            throw GCN_EXCEPTION("Graphics is not of type AllegroGraphics");
        }

        BITMAP* const db = allegroGraphics->getTarget();
        
        const gcn::ClipRectangle& rec = graphics->getCurrentClipArea();
        
        textout_ex(db, 
                   mAllegroFont, 
                   text.c_str(), 
                   x + rec.x, 
                   y + rec.y, 
                   allegroGraphics->getAllegroColor(),
                   -1);
    }
}
