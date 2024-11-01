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

#include "guichan/imagefont.hpp"

#include <sstream>

#include "guichan/color.hpp"
#include "guichan/exception.hpp"
#include "guichan/graphics.hpp"
#include "guichan/image.hpp"

namespace gcn
{
    ImageFont::ImageFont(const std::string& filename, 
                         const std::string& glyphs)
    {
        mFilename = filename;
        mImage = Image::load(filename, false);

        Color separator = mImage->getPixel(0, 0);
                
        int i = 0;
        for (i = 0;
             i < mImage->getWidth() && separator == mImage->getPixel(i, 0);
             ++i)
        {
        }
        
        if (i >= mImage->getWidth())
        {
            throw GCN_EXCEPTION("Corrupt image.");
        }

        int j = 0;
        for (j = 0; j < mImage->getHeight(); ++j)
        {
            if (separator == mImage->getPixel(i, j))
            {
                break;
            }
        }

        mHeight = j;
        int x = 0, y = 0;
        unsigned char k;

        for (i = 0; i < (int)glyphs.size(); ++i)
        {
            k = glyphs.at(i);
            mGlyph[k] = scanForGlyph(k, x, y, separator);
            // Update x och y with new coordinates.
            x = mGlyph[k].x + mGlyph[k].width;
            y =  mGlyph[k].y;
        }

        int w = mImage->getWidth();
        int h = mImage->getHeight();
		w;h;
        mImage->convertToDisplayFormat();

        mRowSpacing = 0;
        mGlyphSpacing = 0;
    }
    
    ImageFont::ImageFont(Image* image, 
                         const std::string& glyphs)
    {
        mFilename = "Image*";
        if (image == NULL)
        {
                GCN_EXCEPTION("Font image is NULL");
        }
        mImage = image;

        Color separator = mImage->getPixel(0, 0);
                
        int i = 0;
        for (i = 0;
             i < mImage->getWidth() && separator == mImage->getPixel(i, 0);
             ++i)
        {
        }
        
        if (i >= mImage->getWidth())
        {
            throw GCN_EXCEPTION("Corrupt image.");
        }

        int j = 0;
        for (j = 0; j < mImage->getHeight(); ++j)
        {
            if (separator == mImage->getPixel(i, j))
            {
                break;
            }
        }

        mHeight = j;
        int x = 0, y = 0;
        unsigned char k;

        for (i = 0; i < (int)glyphs.size(); ++i)
        {
            k = glyphs.at(i);
            mGlyph[k] = scanForGlyph(k, x, y, separator);
            // Update x och y with new coordinates.
            x = mGlyph[k].x + mGlyph[k].width;
            y =  mGlyph[k].y;
        }

        int w = mImage->getWidth();
        int h = mImage->getHeight();
		w;h;
        mImage->convertToDisplayFormat();

        mRowSpacing = 0;
        mGlyphSpacing = 0;
    }

    ImageFont::ImageFont(const std::string& filename, 
                         unsigned char glyphsFrom,
                         unsigned char glyphsTo)
    {
        mFilename = filename;
        mImage = Image::load(filename, false);

        Color separator = mImage->getPixel(0, 0);

        int i = 0;
        for (i=0; separator == mImage->getPixel(i, 0)
                 && i < mImage->getWidth(); ++i)
        {
        }

        if (i >= mImage->getWidth())
        {
            throw GCN_EXCEPTION("Corrupt image.");
        }

        int j = 0;
        for (j = 0; j < mImage->getHeight(); ++j)
        {
            if (separator == mImage->getPixel(i, j))
            {
                break;
            }
        }

        mHeight = j;
        int x = 0, y = 0;

        for (i = glyphsFrom; i < glyphsTo + 1; i++)
        {
            mGlyph[i] = scanForGlyph((unsigned char)i, x, y, separator);
            // Update x och y with new coordinates.
            x = mGlyph[i].x + mGlyph[i].width;
            y =  mGlyph[i].y; 
        }

        int w = mImage->getWidth();
        int h = mImage->getHeight();
		w;h;
        mImage->convertToDisplayFormat();

        mRowSpacing = 0;
        mGlyphSpacing = 0;
    }

    ImageFont::~ImageFont()
    {
        delete mImage;
    }

    int ImageFont::getWidth(unsigned char glyph) const
    {
        if (mGlyph[glyph].width == 0)
        {
            return mGlyph[(int)(' ')].width + mGlyphSpacing;
        }

        return mGlyph[glyph].width + mGlyphSpacing;
    }

    int ImageFont::getHeight() const
    {
        return mHeight + mRowSpacing;
    }

    int ImageFont::drawGlyph(Graphics* graphics, 
                             unsigned char glyph,
                             int x, int y)
    {
        // This is needed for drawing the glyph in the middle 
        // if we have spacing.
        int yoffset = getRowSpacing() / 2;

        if (mGlyph[glyph].width == 0)
        {
            graphics->drawRectangle(Rectangle(x, 
                                              y + 1 + yoffset,
                                              mGlyph[(int)(' ')].width - 1,
                                              mGlyph[(int)(' ')].height - 2));

            return mGlyph[(int)(' ')].width + mGlyphSpacing;
        }

        graphics->drawImage(mImage, 
                            mGlyph[glyph].x, 
                            mGlyph[glyph].y, 
                            x,
                            y + yoffset, 
                            mGlyph[glyph].width,
                            mGlyph[glyph].height);

        return mGlyph[glyph].width + mGlyphSpacing;
    }

    void ImageFont::drawString(Graphics* graphics, 
                               const std::string& text,
                               int x, 
                               int y)
    {
        unsigned int i;

        for (i = 0; i < text.size(); ++i)
        {
            drawGlyph(graphics, text.at(i), x, y);
            x += getWidth(text.at(i));
        }
    }

    void ImageFont::setRowSpacing(int spacing)
    {
        mRowSpacing = spacing;
    }

    int ImageFont::getRowSpacing()
    {
        return mRowSpacing;
    }

    void ImageFont::setGlyphSpacing(int spacing)
    {
        mGlyphSpacing = spacing;
    }

    int ImageFont::getGlyphSpacing()
    {
        return mGlyphSpacing;
    }

    Rectangle ImageFont::scanForGlyph(unsigned char glyph, 
                                      int x,
                                      int y,
                                      const Color& separator)
    {
        Color color;
        do
        {
            ++x;

            if (x >= mImage->getWidth())
            {
                y += mHeight + 1;
                x = 0;

                if (y >= mImage->getHeight())
                {
                    std::string str;
                    std::ostringstream os(str);
                    os << "Image ";
                    os << mFilename;
                    os << " with font is corrupt near character '";
                    os << glyph;
                    os << "'";
                    throw GCN_EXCEPTION(os.str());
                }
            }

            color = mImage->getPixel(x, y);

        } while (color == separator);

        int width = 0;

        do
        {
            ++width;

            if (x + width >= mImage->getWidth())
            {
                std::string str;
                std::ostringstream os(str);
                os << "Image ";
                os << mFilename;
                os << " with font is corrupt near character '";
                os << glyph;
                os << "'";
                throw GCN_EXCEPTION(os.str());
            }

            color = mImage->getPixel(x + width, y);

        } while (color != separator);

        return Rectangle(x, y, width, mHeight);
    }

    int ImageFont::getWidth(const std::string& text) const
    {
        unsigned int i;
        int size = 0;

        for (i = 0; i < text.size(); ++i)
        {
            size += getWidth(text.at(i));
        }

		return size - mGlyphSpacing;
    }

    int ImageFont::getStringIndexAt(const std::string& text, int x)
    {
        unsigned int i;
        int size = 0;

        for (i = 0; i < text.size(); ++i)
        {
            size += getWidth(text.at(i));

            if (size > x)
            {
                return i;
            }
        }

        return text.size();
    }
}
