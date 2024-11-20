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

#ifndef GCN_CONTRIB_ALLEGROTRUETYPEFONT_HPP
#define GCN_CONTRIB_ALLEGROTRUETYPEFONT_HPP

#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/image.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    namespace contrib
    {
struct GLYPH_FACE;
struct GLYPH_REND;
struct GLYPH_KEEP;
        class Graphics;

namespace gcn
{
    /**
         * True type font support using Allegro and GlyphKeeper.
     *
     * Note: GlyphKeeper must be compiled for allegro (define 
     *       GLYPH_TARGET=GLYPH_TARGET_ALLEGRO) and must be 
     *       initialized before using this class. 
     *
     * There's only basic functionality implemented here, so 
     * you can add more and advanced functionality by deriving 
     * from this class. 
     *
     * @author Tobias Scheuer. 
     */
        class GCN_EXTENSION_DECLSPEC AllegroGlyphKeeperFont : public Font
    {
    public:
        /**
         * Constructor.
         *     
         * @param filename the filename of the True Type Font.
         * @param size the size the font should be in.
         */
            AllegroGlyphKeeperFont(const std::string& filename, int size);

        /**
         * Constructor.
         *     
         * @param filename the filename of the True Type Font.
         * @param width the width the font should be in.
         * @param height the height the font should be in.
         */
            AllegroGlyphKeeperFont(const std::string& filename, int width, int height);

        /**
         * Destructor.
         */
            ~AllegroGlyphKeeperFont();


        // Inherited from Font

        virtual int getWidth(const std::string& text) const;

        virtual int getHeight() const;

        virtual void drawString(Graphics* graphics, const std::string& text, int x, int y);

    protected:
        void load(const std::string& filename, int w, int h);
        GLYPH_FACE* mFace;
        GLYPH_REND* mRend;
        GLYPH_KEEP* mKeeper;
    };
}
}
#endif
