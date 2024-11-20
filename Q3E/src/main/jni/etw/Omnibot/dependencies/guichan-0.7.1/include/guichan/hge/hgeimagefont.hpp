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

#ifndef GCN_HGEIMAGEFONT_HPP
#define GCN_HGEIMAGEFONT_HPP

#include <string>
#include <hgefont.h>

#include "guichan/font.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    class Graphics;

    /**
     * Font to be used with HGE. As HGE does not support colorkey blitting of images it's recommended
     * to use HGE's internal font in Guichan.
     *
     * @author Kevin Lynx
     * @since 0.6.1
     */
    class GCN_EXTENSION_DECLSPEC HGEImageFont : public Font
    {
    public:
        /**
         * Constructor. Creates a font from a HGE font file.
         *
         * @param filename the filename of the HGE font.
         */
        HGEImageFont(const std::string& filename);

        /**
         * Destructor.
         */
        ~HGEImageFont();

        /**
         * Gets the loaded HGE font. The HGE font can be used to set certain properties like
         * setting the blend mode. Properties of the HGE font will effect the Guichan font as well.
         *
         * @return the loaded HGE font. 
         */
        virtual hgeFont *getFont() const;

        /**
         * Gets the width from a character.
         *
         * @param character the character to get the width from.
         */
        int getWidth(char character) const;


        //Inherited from Font

        int getWidth(const std::string &text) const;
    
        int getHeight() const;

        int getStringIndexAt(const std::string &text, int x);

        void drawString(Graphics *graphics, const std::string &text, int x, int y);

    protected:
        hgeFont *mHGEFont;
    };
}

#endif // end GCN_HGEIMAGEFONT_HPP
