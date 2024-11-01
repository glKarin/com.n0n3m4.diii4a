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

#ifndef GCN_HGEGRAPHICS_HPP
#define GCN_HGEGRAPHICS_HPP

#include <hge.h>

#include "guichan/graphics.hpp"
#include "guichan/platform.hpp"
#include "guichan/color.hpp"

namespace gcn
{
    class Color;

    /**
     * HGE implementation of Graphics.
     *
     * @author Kevin Lynx
     * @since 0.6.1
     */
    class GCN_EXTENSION_DECLSPEC HGEGraphics : public Graphics
    {
    public:
        /**
         * Constructor.
         */
        HGEGraphics();

        /**
         * Destructor.
         */
        ~HGEGraphics();


        //Inherited from Graphics

        void _beginDraw();
        
        void _endDraw();

        void drawImage(const Image *image, 
                       int srcX, 
                       int srcY, 
                       int dstX, 
                       int dstY, 
                       int width, 
                       int height);
    
        void drawImage(const Image* image, int dstX, int dstY);

        void drawPoint(int x, int y);

        void drawLine(int x1, int y1, int x2, int y2);

        void drawRectangle(const Rectangle &rectangle);

        void fillRectangle(const Rectangle &rectangle);

        void setColor(const Color &color);

        const Color &getColor() const;

        bool pushClipArea(Rectangle area);
    
        void popClipArea();

    protected:
        static  HGE *mHGE;
        Color mColor;
        DWORD mHardwareColor;
        bool mClipNull;
    };
}

#endif // end GCN_HGEGRAPHICS_HPP
