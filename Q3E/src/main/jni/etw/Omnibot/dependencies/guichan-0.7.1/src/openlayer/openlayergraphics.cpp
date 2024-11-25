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

#include "guichan/openlayer/openlayergraphics.hpp"

#include <OpenLayer.hpp>

#include <string>

#include "guichan/exception.hpp"
#include "guichan/openlayer/openlayerimage.hpp"

namespace gcn
{
    OpenLayerGraphics::OpenLayerGraphics()
    {
        setTargetPlane(640, 480);
    }
    
    OpenLayerGraphics::OpenLayerGraphics(int width, int height)
    {
        setTargetPlane(width, height);
    }

    OpenLayerGraphics::~OpenLayerGraphics()
    {

    }

    void OpenLayerGraphics::_beginDraw()
    {
        pushClipArea(Rectangle(0, 0, mWidth, mHeight));
    }

    void OpenLayerGraphics::_endDraw()
    {
        popClipArea();
    }

    bool OpenLayerGraphics::pushClipArea(Rectangle area)
    {
        bool result = Graphics::pushClipArea(area);
        
        ol::Transforms::SetPosition(mClipStack.top().xOffset,
                                    mClipStack.top().yOffset);
        
        ol::Canvas::SetClipping(mClipStack.top().x,
                                mClipStack.top().y,
                                mClipStack.top().width,
                                mClipStack.top().height);        

        return result;
    }

    void OpenLayerGraphics::popClipArea()
    {
        Graphics::popClipArea();

        if (mClipStack.empty())
        {
            ol::Transforms::SetPosition(0, 0);
            ol::Canvas::DisableClipping();
        } 
        else
        {
            const ClipRectangle top = mClipStack.top();
            ol::Transforms::SetPosition(top.xOffset,
                                        top.yOffset);
            ol::Canvas::SetClipping(top.x,
                                    top.y,
                                    top.width,
                                    top.height);
        }
    }

    void OpenLayerGraphics::setTargetPlane(int width, int height)
    {
        mWidth = width;
        mHeight = height;
    }

    void OpenLayerGraphics::drawImage(const Image* image,
                                      int srcX,
                                      int srcY,
                                      int dstX,
                                      int dstY,
                                      int width,
                                      int height)
    {
        const OpenLayerImage* srcImage = dynamic_cast<const OpenLayerImage*>(image);

        if (srcImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an OpenLayerImage.");
        }
        
        srcImage->getBitmap()->Blit(dstX - srcX,
                                    dstY - srcY,
                                    ol::ClippedMode(srcX,
                                                    srcY,
                                                    width,
                                                    height),
                                    1.0f);
    }
    
    void OpenLayerGraphics::drawPoint(int x, int y)
    {
        ol::GfxRend::Point(x + 0.5f, y - 0.5f, mRgba);
    }
    
    void OpenLayerGraphics::drawLine(int x1, int y1, int x2, int y2)
    {
        mRgba.Select();
        glDisable(GL_TEXTURE_2D);
        glLineWidth(1.0f);
        
        glBegin(GL_LINES);
        glVertex2f(x1+0.5f, y1+0.5f);
        glVertex2f(x2+0.5f, y2+0.5f);
        glEnd();

        glBegin(GL_POINTS);
        glVertex2f(x1+0.5f, y1+0.5f);
        glEnd();

        glBegin(GL_POINTS);
        glVertex2f(x2+0.5f, y2+0.5f);
        glEnd();

        if (ol::Settings::TextureMappingUsed())
        {
            glEnable(GL_TEXTURE_2D);
        }
    }

    void OpenLayerGraphics::drawRectangle(const Rectangle& rectangle)
    {
        ol::GfxRend::RectOutline(rectangle.x + 0.5f,
                                 rectangle.y + 0.5f,
                                 rectangle.width - 0.5f,
                                 rectangle.height - 0.5f,
                                 mRgba);
    }
    
    void OpenLayerGraphics::fillRectangle(const Rectangle& rectangle)
    {
        ol::GfxRend::Rect(rectangle.x,
                          rectangle.y,
                          rectangle.width,
                          rectangle.height,
                          mRgba);
    }

    void OpenLayerGraphics::setColor(const Color& color)
    {
        mColor = color;
        mRgba.r = color.r / 255.0f;
        mRgba.g = color.g / 255.0f;
        mRgba.b = color.b / 255.0f;
        mRgba.a = color.a / 255.0f;
        mRgba.Select();
    }

    const Color& OpenLayerGraphics::getColor() const
    {
        return mColor;
    }

    const ol::Rgba& OpenLayerGraphics::getOpenLayerColor() const
    {
        return mRgba;
    }
}
