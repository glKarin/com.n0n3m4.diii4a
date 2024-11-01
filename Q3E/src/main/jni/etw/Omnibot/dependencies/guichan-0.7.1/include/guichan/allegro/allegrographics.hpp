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

#ifndef GCN_ALLEGROGRAPHICS_HPP
#define GCN_ALLEGROGRAPHICS_HPP

#include <allegro.h>

#include "guichan/color.hpp"
#include "guichan/graphics.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    class Font;

    /**
     * Allegro implementation of Graphics.
     */
    class GCN_EXTENSION_DECLSPEC AllegroGraphics : public Graphics
    {
    public:

        // Needed so that drawImage(gcn::Image *, int, int) is visible.
        using Graphics::drawImage;

        /**
         * Constructor.
         */
        AllegroGraphics();

        /**
         * Contsructor. Sets the drawing target.
         *
         * @param target the target to draw to.
         */
        AllegroGraphics(BITMAP *target);

        /**
         * Destructor.
         */
        virtual ~AllegroGraphics();

        /**
         * Sets the target bitmap to draw to. It can be any bitmap with the same
         * bit-depth as the screen. However, if you pass the screen bitmap you
         * will probably get flicker. Use a double buffer!
         *
         * @param target the bitmap to draw to.
         */
        virtual void setTarget(BITMAP *target);

        /**
         * Gets the target bitmap.
         *
         * @return the target bitmap.
         */
        virtual BITMAP *getTarget();

        /**
         * Gets the color in Allegro format.
         *
         * @return the color in Allegro format.
         */
        int getAllegroColor() const;

        /**
         * Draws an Allegro bitmap.
         *
         * @param bitmap the bitmap to draw.
         * @param dstX the x-coordinate where to draw the bitmap.
         * @param dstY the y-coordinate where to draw the bitmap.
         */
        virtual void drawBitmap(BITMAP* bitmap, int dstX, int dstY);


        // Inherited from Graphics

        virtual void _beginDraw();

        virtual void _endDraw();

        virtual bool pushClipArea(Rectangle area);

        virtual void popClipArea();

        virtual void drawImage(const Image* image,
                               int srcX,
                               int srcY,
                               int dstX,
                               int dstY,
                               int width,
                               int height);

        virtual void drawPoint(int x, int y);

        virtual void drawLine(int x1, int y1, int x2, int y2);

        virtual void drawRectangle(const Rectangle& rectangle);

        virtual void fillRectangle(const Rectangle& rectangle);

        virtual void setColor(const Color& color);

        virtual const Color& getColor() const;

    protected:
        BITMAP *mTarget;
        bool mClipNull;
        int mAllegroColor;
        Color mColor;
    };
}

#endif // end GCN_ALLEGROGRAPHICS_HPP

