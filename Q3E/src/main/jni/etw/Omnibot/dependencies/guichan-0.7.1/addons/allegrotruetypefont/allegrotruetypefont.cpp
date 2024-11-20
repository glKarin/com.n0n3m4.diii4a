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

#include "allegrotruetypefont.hpp"

#include "guichan/allegro.hpp"
#include "guichan/exception.hpp"
#include "guichan/graphics.hpp"

#include <glyph.h>

namespace gcn
{
    namespace contrib
    {
        void AllegroGlyphKeeperFont::load(const std::string& filename, int w, int h)
    {
        mKeeper = gk_create_keeper(0,0);

        if (mKeeper == NULL)
        {
            throw GCN_EXCEPTION("Can't create keeper.");
        }

        mFace = gk_load_face_from_file(filename.c_str(), 0);

        if (mFace == NULL)
        {
            throw GCN_EXCEPTION("Can't load font from file.");
        }

        mRend = gk_create_renderer(mFace,mKeeper);
        
        if (mRend == NULL)
        {
            throw GCN_EXCEPTION("Can't create renderer.");
        }

        gk_rend_set_hinting_off(mRend);
        gk_rend_set_size_pixels(mRend, w, h);
        gk_rend_set_text_color_rgb(mRend, 0, 0, 0);
    }

        AllegroGlyphKeeperFont::AllegroGlyphKeeperFont(const std::string& filename, int size)
        : mFace(0), 
          mRend(0), 
          mKeeper(0)
    {
        load(filename, size, size);
    }

        AllegroGlyphKeeperFont::AllegroGlyphKeeperFont(const std::string& filename, int w, int h)
        : mFace(0), 
          mRend(0), 
          mKeeper(0)
    {
        load(filename, w, h);
    }

        AllegroGlyphKeeperFont::~AllegroGlyphKeeperFont()
    {
        if (mRend != NULL)
        {
            gk_done_renderer(mRend);
        }

        mRend = NULL;

        if (mFace != NULL)
        {
            gk_unload_face(mFace);
        }

        mFace = NULL;

        if (mKeeper != NULL)
        {
            gk_done_keeper(mKeeper);
        }

        mKeeper = NULL;
    }

        int AllegroGlyphKeeperFont::getWidth(const std::string& text) const
    {
        return gk_text_width_utf8(mRend, text.c_str());
    }

        int AllegroGlyphKeeperFont::getHeight() const
    {
        return gk_rend_height_pixels(mRend);
    }

        void AllegroGlyphKeeperFont::drawString(Graphics* graphics, const std::string& text, int x, int y)
    {
        AllegroGraphics* const allegroGraphics = dynamic_cast<AllegroGraphics*>(graphics);
        if (!allegroGraphics)
        {
            throw GCN_EXCEPTION("Graphics is not of type AllegroGraphics");
        }

        BITMAP* const target = allegroGraphics->getTarget();
        
        gk_rend_set_text_color_combined(mRend, allegroGraphics->getAllegroColor());
        gk_rend_set_text_alpha(mRend, allegroGraphics->getColor().a);
        
        const ClipRectangle& rec = graphics->getCurrentClipArea();
        
        gk_render_line_utf8(target,
                            mRend, 
                            text.c_str(), 
                            x + rec.xOffset, 
                            y + rec.yOffset + gk_rend_ascender_pixels(mRend));
    }
}
}



