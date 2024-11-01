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

#ifndef GCN_GRAPHICS_HPP
#define GCN_GRAPHICS_HPP

#include <iosfwd>
#include <stack>

#include "guichan/cliprectangle.hpp"
#include "guichan/platform.hpp"
#include "guichan/color.hpp"

namespace gcn
{
    class Color;
    class Font;
    class Image;

    /**
     * Abstract class for providing drawing primitve functions. 
     * It contains all vital functions for drawing.
     *
     * Guichan contains implementations of Graphics for common 
     * libraries like the Allegro library, the HGE library, 
     * the OpenGL library, the OpenLayer library, and the SDL library. 
     * To make Guichan usable with other libraries, a Graphics class
     * must be implemented.
     *
     * In Graphics you can set clip areas to limit drawing to certain
     * areas of the screen. Clip areas are put on a stack, which 
     * means that you can push smaller and smaller clip areas onto the 
     * stack. All coordinates will be relative to the top most clip area. 
     * In most cases you won't have to worry about the clip areas, 
     * unless you want to implement some really complex widget. 
     * Pushing and poping of clip areas are handled automatically by 
     * container widgets when their child widgets are drawn.
     *
     * IMPORTANT: Remember to pop each clip area that you pushed on the stack
     * after you are done with it.
     *
     * If you feel that Graphics is to restrictive for your needs, 
     * there is no one stopping you from using your own code for drawing 
     * in widgets. You could for instance use pure SDL in the drawing of 
     * widgets bypassing Graphics. This might however hurt portability of 
     * your application.
     *
     * If you implement a Graphics class not present in Guichan we would 
     * be very happy to add it to Guichan.
     *
     * @see AllegroGraphics, HGEGraphics, OpenLayerGraphics, OpenGLGraphics, 
     *      SDLGraphics, Image
     * @since 0.1.0
     */
    class GCN_CORE_DECLSPEC Graphics
    {
    public:
        /**
         * Alignments for text drawing.
         */
        enum Alignment
        {
            LEFT = 0,
            CENTER,
            RIGHT
        };

        /**
         * Constructor.
         */
        Graphics();

        /**
         * Destructor.
         */
        virtual ~Graphics() { }

        /**
         * Initializes drawing. Called by the Gui when Gui::draw() is called.
         * It is needed by some implementations of Graphics to perform
         * preparations before drawing. An example of such an implementation
         * is the OpenGLGraphics.
         *
         * NOTE: You will never need to call this function yourself, unless
         *       you use a Graphics object outside of Guichan.
         *
         * @see _endDraw, Gui::draw
         */
        virtual void _beginDraw() { }

        /**
         * Deinitializes drawing. Called by the Gui when a Gui::draw() is done.
         * done. It should reset any state changes made by _beginDraw().
         *
         * NOTE: You will never need to call this function yourself, unless
         *       you use a Graphics object outside of Guichan.
         *
         * @see _beginDraw, Gui::draw
         */
        virtual void _endDraw() { }

        /**
         * Pushes a clip area onto the stack. The x and y coordinates in the
         * rectangle is  relative to the last pushed clip area.
         * If the new area falls outside the current clip area, it will be
         * clipped as necessary.
         *
         * If a clip area is outside of the top clip area a clip area with
         * zero width and height will be pushed.
         *
         * @param area The clip area to be pushed onto the stack.
         * @return False if the the new area lays outside the current clip 
         *         area.
         */
        virtual bool pushClipArea(Rectangle area);

        /**
         * Removes the top most clip area from the stack.
         *
         * @throws Exception if the stack is empty.
         */
        virtual void popClipArea();

        /**
         * Gets the current clip area. Usefull if you want to do drawing
         * bypassing Graphics.
         *
         * @return The current clip area.
         */
        virtual const ClipRectangle& getCurrentClipArea();

        /**
         * Draws a part of an image.
         *
         * NOTE: Width and height arguments will not scale the image but
         *       specifies the size of the part to be drawn. If you want
         *       to draw the whole image there is a simplified version of
         *       this function.
         *
         * EXAMPLE: @code drawImage(myImage, 10, 10, 20, 20, 40, 40); @endcode
         *          Will draw a rectangular piece of myImage starting at
         *          coordinate (10, 10) in myImage, with width and height 40.
         *          The piece will be drawn with it's top left corner at
         *          coordinate (20, 20).
         *
         * @param image The image to draw.
         * @param srcX The source image x coordinate.
         * @param srcY The source image y coordinate.
         * @param dstX The destination x coordinate.
         * @param dstY The destination y coordinate.
         * @param width The width of the piece.
         * @param height The height of the piece.
         */
        virtual void drawImage(const Image* image,
                               int srcX,
                               int srcY,
                               int dstX,
                               int dstY,
                               int width,
                               int height) = 0;
        /**
         * Draws an image. A simplified version of the other drawImage.
         * It will draw a whole image at the coordinate you specify.
         * It is equivalent to calling:
         * @code drawImage(myImage, 0, 0, dstX, dstY, image->getWidth(), \
         image->getHeight()); @endcode
         */
        virtual void drawImage(const Image* image, int dstX, int dstY);

        /**
         * Draws a single point/pixel.
         *
         * @param x The x coordinate.
         * @param y The y coordinate.
         */
        virtual void drawPoint(int x, int y) = 0;

		struct Point
		{
			int x, y;

			Color c;

			Point() {}
			Point(int _x, int _y, Color _c)
				: x(_x), y(_y), c(_c)
			{}
		};
		virtual void drawPoints(const Point *points, int numPoints) = 0;
        /**
         * Ddraws a line.
         *
         * @param x1 The first x coordinate.
         * @param y1 The first y coordinate.
         * @param x2 The second x coordinate.
         * @param y2 The second y coordinate.
         */
        virtual void drawLine(int x1, int y1, int x2, int y2) = 0;

		struct Line
		{
			int x1,y1,x2,y2;

			Color c;

			Line() {}
			Line(int _x1, int _y1, int _x2, int _y2, Color _c)
				: x1(_x1), y1(_y1), x2(_x2), y2(_y2), c(_c)
			{}
		};
		virtual void drawLines(const Line *lines, int numLines) = 0;

		struct Triangle
		{
			Point v0,v1,v2;
		};
		virtual void drawPolygon(const Triangle *tris, int numTris, bool filled = true) = 0;
        /**
         * Draws a simple, non-filled, rectangle with a one pixel width.
         *
         * @param rectangle The rectangle to draw.
         */
        virtual void drawRectangle(const Rectangle& rectangle) = 0;

        /**
         * Draws a filled rectangle.
         *
         * @param rectangle The filled rectangle to draw.
         */
        virtual void fillRectangle(const Rectangle& rectangle) = 0;

        /**
         * Sets the color to use when drawing.
         *
         * @param color A color.
         * @see getColor
         */
        virtual void setColor(const Color& color) = 0;

        /**
         * Gets the color to use when drawing.
         *
         * @return The color used when drawing.
         * @see setColor
         */
        virtual const Color& getColor() const = 0;

        /**
         * Sets the font to use when drawing text.
         *
         * @param font The font to use when drawing.
         */
        virtual void setFont(Font* font);

        /**
         * Draws text.
         *
         * @param text The text to draw.
         * @param x The x coordinate where to draw the text.
         * @param y The y coordinate where to draw the text.
         * @param alignment The alignemnt to use when drawing.
         * @throws Exception when no font has been set.
         */
        virtual void drawText(const std::string& text,
                              int x,
                              int y,
                              Alignment alignment = LEFT);

    protected:
        /**
         * Holds the clip area stack.
         */
        std::stack<ClipRectangle> mClipStack;

        /**
         * Holds the current font.
         */
        Font* mFont;
    };
}

#endif // end GCN_GRAPHICS_HPP
