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

#ifndef GCN_OPENLAYERFONT_HPP
#define GCN_OPENLAYERFONT_HPP

#include <OpenLayer.hpp>
#if defined (DELETE)
#undef DELETE
#endif
#include "guichan/font.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    /**
     * OpenLayer font implementation of Font that uses the native
     * OpenLayer font.
     */
    class GCN_EXTENSION_DECLSPEC OpenLayerTTFont : public Font
    {
    public:

        /**
         * Constructor. The font loaded with this constructor will be freed in the destructor.
         *
         * @param filename The filename of the font to load.
         * @param size The size of the font.
         */
        OpenLayerTTFont(const std::string& filename, int size);

        /**
         * Constructor. The font loaded with this constructor will be freed in the destructor.
         *
         * @param filename The filename of the font to load.
         * @param width The width of the font.
         * @param height The height of the font.
         */
        OpenLayerTTFont(const std::string& filename, int width, int height);

        /**
         * Destructor.
         */
        ~OpenLayerTTFont();


        // Inherited from Font

        virtual int getWidth(const std::string& text) const;

        virtual int getHeight() const;

        virtual void drawString(Graphics* graphics, const std::string& text, int x, int y);

    protected:
        ol::TextRenderer mTextRenderer;
    };
}

#endif // GCN_OPENLAYERFONT_HPP
