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

#ifndef GCN_DEFAULTFONT_HPP
#define GCN_DEFAULTFONT_HPP

#include "guichan/font.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    /**
     * A font only capable of drawing rectangles. It is used by default
     * in Guichan if no font has been set merely to show that no font has 
     * been set.
     */
    class GCN_CORE_DECLSPEC DefaultFont : public Font
    {
    public:

        /**
         * Destructor.
         */
        virtual ~DefaultFont(){}

        /**
         * Draws a glyph as a rectangle. The glyphs will always be drawn as
         * rectangles no matter the glyph.
         *
         * NOTE: You normally won't use this function to draw text since
         *       the Graphics class contains better functions for drawing
         *       text.
         *
         * @param graphics A Graphics object to be used for drawing.
         * @param glyph The glyph to draw.
         * @param x The x coordinate where to draw the glyph.
         * @param y The y coordinate where to draw the glyph.
         * @return The width of the drawn glyph in pixels.
         */
        virtual int drawGlyph(Graphics* graphics, unsigned char glyph, int x, int y);


        // Inherited from Font

        virtual void drawString(Graphics* graphics, const std::string& text, int x, int y);

        virtual int getWidth(const std::string& text) const;

        virtual int getHeight() const;

        virtual int getStringIndexAt(const std::string& text, int x);
    };
}

#endif // end GCN_DEFAULTFONT_HPP
