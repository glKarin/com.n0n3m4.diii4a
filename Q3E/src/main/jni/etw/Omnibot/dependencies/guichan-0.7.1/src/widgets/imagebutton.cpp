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

#include "guichan/widgets/imagebutton.hpp"

#include "guichan/graphics.hpp"
#include "guichan/image.hpp"

namespace gcn
{
    ImageButton::ImageButton()
        : mImage(0), 
          mInternalImage(false)
    {
        setWidth(0);
        setHeight(0);
    }

    ImageButton::ImageButton(const std::string& filename)
        : mImage(0), 
          mInternalImage(false)
    {
        mImage = Image::load(filename);
        mInternalImage = true;
        setWidth(mImage->getWidth() + mImage->getWidth() / 2);
        setHeight(mImage->getHeight() + mImage->getHeight() / 2);
    }

    ImageButton::ImageButton(const Image* image)
        : mImage(image), 
          mInternalImage(false)
    {
        setWidth(mImage->getWidth() + mImage->getWidth() / 2);
        setHeight(mImage->getHeight() + mImage->getHeight() / 2);
    }

    ImageButton::~ImageButton()
    {
        if (mInternalImage)
        {
            delete mImage;
        }
    }

    void ImageButton::setImage(const Image* image)
    {
        if (mInternalImage)
        {
            delete mImage;
        }

        mImage = image;
        mInternalImage = false;
    }

    const Image* ImageButton::getImage() const
    {
        return mImage;
    }

    void ImageButton::draw(Graphics* graphics)
    {
        gcn::Color faceColor = getBaseColor();
        gcn::Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;

        if (isPressed())
        {
            faceColor = faceColor - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor + 0x303030;
            shadowColor.a = alpha;
        }
        else
        {
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(1, 
                                          1, 
                                          getDimension().width - 1, 
                                          getHeight() - 1));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, getWidth() - 1, 0);
        graphics->drawLine(0, 1, 0, getHeight() - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(getWidth() - 1, 1, getWidth() - 1, getHeight() - 1);
        graphics->drawLine(1, getHeight() - 1, getWidth() - 1, getHeight() - 1);

        graphics->setColor(getForegroundColor());

        const int textX = (getWidth() - (mImage ? mImage->getWidth() : 0) ) / 2;
        const int textY = (getHeight() - (mImage ? mImage->getHeight() : 0) ) / 2;

        if (isPressed())
        {
            if(mImage)
                graphics->drawImage(mImage, textX + 1, textY + 1);
        }
        else
        {
            if(mImage)
                graphics->drawImage(mImage, textX, textY);

            if (isFocused())
            {
                graphics->drawRectangle(Rectangle(2, 
                                                  2, 
                                                  getWidth() - 4,
                                                  getHeight() - 4));
            }
        }
    }
}
