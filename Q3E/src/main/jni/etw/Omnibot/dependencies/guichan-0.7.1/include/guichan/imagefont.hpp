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

#ifndef GCN_IMAGEFONT_HPP
#define GCN_IMAGEFONT_HPP

#include <string>

#include "guichan/font.hpp"
#include "guichan/platform.hpp"
#include "guichan/rectangle.hpp"

namespace gcn
{
    class Color;
    class Graphics;
    class Image;

    /**
     * A font using an image containing the font data. ImageFont can be used
     * with any supported by the current used ImageLoader.
     *
     * These are two examples of an image containing a font.
     *  \image html imagefontexample.bmp
     *  \image html imagefontexample2.bmp
     *
     * The Image font format works like this: The first pixel, the pixal at
     * coordinate (0,0), tells which color to look for when seperating glyphs.
     * You create an image with your glyphs and simple separates them with
     * the seperation color. When you create your image font you supply the
     * constructor with the glyphs present in your image. 
     *
     * To create an ImageFont from the first image example above the following
     * constructor call should be made:
     * @code gcn::ImageFont imageFont("fixedfont_big.bmp"," abcdefghijklmno\
pqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"); @endcode
     *
     * Note worthy is that the first glyph actually gives the width of space.
     * Glyphs can, as seen in the second image example above, be seperated with
     * horizontal lines making it possible to draw glyphs on more then one
     * line in the image. However, these vertical lines must have a height of
     * one pixel!
     */
    class GCN_CORE_DECLSPEC ImageFont: public Font
    {
    public:

        /**
         * Constructor. Takes an image file containing the font and
         * a string containing the glyphs. The glyphs in the string should
         * be in the same order as they appear in the font image.
         *
         * @param filename The filename of the image.
         * @param glyphs The glyphs found in the image.
         * @throws Exception when glyph list is incorrect or the font file is
         *                   corrupt or if no ImageLoader exists.
         */
        ImageFont(const std::string& filename, const std::string& glyphs);

        /**
         * Constructor. Takes an image containing the font and
         * a string containing the glyphs. The glyphs in the string should
         * be in the same order as they appear in the font image.
         * The image will be deleted in the destructor.
         *
         * @param image The image with font glyphs.
         * @param glyphs The glyphs found in the image.
         * @throws Exception when glyph list is incorrect or the font image is
         *                   is missing.
         */
        ImageFont(Image* image, const std::string& glyphs);

        /**
         * Constructor. Takes an image file containing the font and
         * two boundaries of ASCII values. The font image should include
         * all glyphs specified with the boundaries in increasing ASCII
         * order. The boundaries are inclusive.
         *
         * @param filename The filename of the image.
         * @param glyphsFrom The ASCII value of the first glyph found in the
         *                   image.
         * @param glyphsTo The ASCII value of the last glyph found in the
         *                 image.
         * @throws Exception when glyph bondaries are incorrect or the font
         *                   file is corrupt or if no ImageLoader exists.
         */
        ImageFont(const std::string& filename, 
                  unsigned char glyphsFrom=32,
                  unsigned char glyphsTo=126);

        /**
         * Destructor.
         */
        virtual ~ImageFont();

        /**
         * Draws a glyph.
         *
         * NOTE: You normally won't use this function to draw text since
         *       the Graphics class contains better functions for drawing
         *       text.
         *
         * @param graphics A graphics object used for drawing.
         * @param glyph A glyph to draw.
         * @param x The x coordinate where to draw the glyph.
         * @param y The y coordinate where to draw the glyph.
         * @return The width of the glyph in pixels.
         */
        virtual int drawGlyph(Graphics* graphics, unsigned char glyph,
                              int x, int y);

        /**
         * Sets the space between rows in pixels. Default is 0 pixels.
         * The space can be negative.
         *
         * @param spacing The space between rows in pixels.
         * @see getRowSpacing
         */
        virtual void setRowSpacing(int spacing);

        /**
         * Gets the space between rows in pixels.
         *
         * @return The space between rows in pixels.
         * @see setRowSpacing
         */
        virtual int getRowSpacing();

        /**
         * Sets the spacing between glyphs in pixels. Default is 0 pixels.
         * The space can be negative.
         *
         * @param spacing The glyph space in pixels.
         * @see getGlyphSpacing
         */
        virtual void setGlyphSpacing(int spacing);

        /**
         * Gets the spacing between letters in pixels.
         *
         * @return the spacing.
         * @see setGlyphSpacing
         */
        virtual int getGlyphSpacing();

        /**
         * Gets a width of a glyph in pixels.
         *
         * @param glyph The glyph which width will be returned.
         * @return The width of a glyph in pixels.
         */
        virtual int getWidth(unsigned char glyph) const;


        // Inherited from Font

        virtual int getWidth(const std::string& text) const;

        virtual void drawString(Graphics* graphics, const std::string& text,
                                int x, int y);

        virtual int getHeight() const;

        virtual int getStringIndexAt(const std::string& text, int x);

    protected:
        /**
         * Scans for a certain glyph.
         *
         * @param glyph The glyph to scan for. Used for exception messages.
         * @param x The x coordinate where to begin the scan. The coordinate
         *          will be updated with the end x coordinate of the glyph
         *          when the scan is complete.
         * @param y The y coordinate where to begin the scan. The coordinate
         *          will be updated with the end y coordinate of the glyph
         *          when the scan is complete.
         * @param separator The color separator to look for where the glyph ends.
         * @return A rectangle with the found glyph dimension in the image
         *         with the font.
         * @throws Exception when no glyph is found.
         */
         Rectangle scanForGlyph(unsigned char glyph, 
                                int x, 
                                int y, 
                                const Color& separator);

        /**
         * Holds the glyphs areas in the image.
         */
        Rectangle mGlyph[256];

        /**
         * Holds the height of the image font.
         */
        int mHeight;

        /**
         * Holds the glyph spacing of the image font.
         */
        int mGlyphSpacing;

        /**
         * Holds the row spacing of the image font.
         */
        int mRowSpacing;

        /**
         * Holds the image with the font data.
         */
        Image* mImage;

        /**
         * Holds the filename of the image with the font data.
         */
        std::string mFilename;
    };
}

#endif // end GCN_IMAGEFONT_HPP
