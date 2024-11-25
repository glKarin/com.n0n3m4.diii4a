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

#include "guichan/widgets/scrollarea.hpp"

#include "guichan/exception.hpp"
#include "guichan/graphics.hpp"

namespace gcn
{
    ScrollArea::ScrollArea()
    {
        mVScroll = 0;
        mHScroll = 0;
        mHPolicy = SHOW_AUTO;
        mVPolicy = SHOW_AUTO;
        mScrollbarWidth = 12;
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mUpButtonScrollAmount = 10;
        mDownButtonScrollAmount = 10;
        mLeftButtonScrollAmount = 10;
        mRightButtonScrollAmount = 10;
        mIsVerticalMarkerDragged = false;
        mIsHorizontalMarkerDragged =false;
        mOpaque = true;

        addMouseListener(this);
    }

    ScrollArea::ScrollArea(Widget *content)
    {
        mVScroll = 0;
        mHScroll = 0;
        mHPolicy = SHOW_AUTO;
        mVPolicy = SHOW_AUTO;
        mScrollbarWidth = 12;
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mUpButtonScrollAmount = 10;
        mDownButtonScrollAmount = 10;
        mLeftButtonScrollAmount = 10;
        mRightButtonScrollAmount = 10;
        mIsVerticalMarkerDragged = false;
        mIsHorizontalMarkerDragged =false;
        mOpaque = true;

        setContent(content);
        addMouseListener(this);
    }

    ScrollArea::ScrollArea(Widget *content, 
                           ScrollPolicy hPolicy, 
                           ScrollPolicy vPolicy)
    {
        mVScroll = 0;
        mHScroll = 0;
        mHPolicy = hPolicy;
        mVPolicy = vPolicy;
        mScrollbarWidth = 12;
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mUpButtonScrollAmount = 10;
        mDownButtonScrollAmount = 10;
        mLeftButtonScrollAmount = 10;
        mRightButtonScrollAmount = 10;
        mIsVerticalMarkerDragged = false;
        mIsHorizontalMarkerDragged =false;
        mOpaque = true;

        setContent(content);
        addMouseListener(this);
    }

    ScrollArea::~ScrollArea()
    {
        setContent(NULL);
    }

    void ScrollArea::setContent(Widget* widget)
    {
        if (widget != NULL)
        {
            clear();
            add(widget);
            widget->setPosition(0,0);
        }
        else
        {
            clear();
        }

        checkPolicies();
    }

    Widget* ScrollArea::getContent()
    {
        if (mWidgets.size() > 0)
        {
            return *mWidgets.begin();
        }

        return NULL;
    }

    void ScrollArea::setHorizontalScrollPolicy(ScrollPolicy hPolicy)
    {
        mHPolicy = hPolicy;
        checkPolicies();
    }

    ScrollArea::ScrollPolicy ScrollArea::getHorizontalScrollPolicy() const
    {
        return mHPolicy;
    }

    void ScrollArea::setVerticalScrollPolicy(ScrollPolicy vPolicy)
    {
        mVPolicy = vPolicy;
        checkPolicies();
    }

    ScrollArea::ScrollPolicy ScrollArea::getVerticalScrollPolicy() const
    {
        return mVPolicy;
    }

    void ScrollArea::setScrollPolicy(ScrollPolicy hPolicy, ScrollPolicy vPolicy)
    {
        mHPolicy = hPolicy;
        mVPolicy = vPolicy;
        checkPolicies();
    }

    void ScrollArea::setVerticalScrollAmount(int vScroll)
    {
        int max = getVerticalMaxScroll();

        mVScroll = vScroll;

        if (vScroll > max)
        {
            mVScroll = max;
        }

        if (vScroll < 0)
        {
            mVScroll = 0;
        }
    }

    int ScrollArea::getVerticalScrollAmount() const
    {
        return mVScroll;
    }

    void ScrollArea::setHorizontalScrollAmount(int hScroll)
    {
        int max = getHorizontalMaxScroll();

        mHScroll = hScroll;

        if (hScroll > max)
        {
            mHScroll = max;
        }
        else if (hScroll < 0)
        {
            mHScroll = 0;
        }
    }

    int ScrollArea::getHorizontalScrollAmount() const
    {
        return mHScroll;
    }

    void ScrollArea::setScrollAmount(int hScroll, int vScroll)
    {
        setHorizontalScrollAmount(hScroll);
        setVerticalScrollAmount(vScroll);
    }

    int ScrollArea::getHorizontalMaxScroll()
    {
        checkPolicies();

        if (getContent() == NULL)
        {
            return 0;
        }

        int value = getContent()->getWidth() - getChildrenArea().width +
            2 * getContent()->getFrameSize();

        if (value < 0)
        {
            return 0;
        }

        return value;
    }

    int ScrollArea::getVerticalMaxScroll()
    {
        checkPolicies();

        if (getContent() == NULL)
        {
            return 0;
        }

        int value;

        value = getContent()->getHeight() - getChildrenArea().height +
            2 * getContent()->getFrameSize();

        if (value < 0)
        {
            return 0;
        }

        return value;
    }

    void ScrollArea::setScrollbarWidth(int width)
    {
        if (width > 0)
        {
            mScrollbarWidth = width;
        }
        else
        {
            throw GCN_EXCEPTION("Width should be greater then 0.");
        }
    }

    int ScrollArea::getScrollbarWidth() const
    {
        return mScrollbarWidth;
    }

    void ScrollArea::mousePressed(MouseEvent& mouseEvent)
    {
        int x = mouseEvent.getX();
        int y = mouseEvent.getY();

        if (getUpButtonDimension().isPointInRect(x, y))
        {
            setVerticalScrollAmount(getVerticalScrollAmount()
                                    - mUpButtonScrollAmount);
            mUpButtonPressed = true;
        }
        else if (getDownButtonDimension().isPointInRect(x, y))
        {
            setVerticalScrollAmount(getVerticalScrollAmount()
                                    + mDownButtonScrollAmount);
            mDownButtonPressed = true;
        }
        else if (getLeftButtonDimension().isPointInRect(x, y))
        {
            setHorizontalScrollAmount(getHorizontalScrollAmount()
                                      - mLeftButtonScrollAmount);
            mLeftButtonPressed = true;
        }
        else if (getRightButtonDimension().isPointInRect(x, y))
        {
            setHorizontalScrollAmount(getHorizontalScrollAmount()
                                      + mRightButtonScrollAmount);
            mRightButtonPressed = true;
        }
        else if (getVerticalMarkerDimension().isPointInRect(x, y))
        {
            mIsHorizontalMarkerDragged = false;
            mIsVerticalMarkerDragged = true;

            mVerticalMarkerDragOffset = y - getVerticalMarkerDimension().y;
        }
        else if (getVerticalBarDimension().isPointInRect(x,y))
        {
            if (y < getVerticalMarkerDimension().y)
            {
                setVerticalScrollAmount(getVerticalScrollAmount()
                                        - (int)(getChildrenArea().height * 0.95));
            }
            else
            {
                setVerticalScrollAmount(getVerticalScrollAmount()
                                        + (int)(getChildrenArea().height * 0.95));
            }
        }
        else if (getHorizontalMarkerDimension().isPointInRect(x, y))
        {
            mIsHorizontalMarkerDragged = true;
            mIsVerticalMarkerDragged = false;

            mHorizontalMarkerDragOffset = x - getHorizontalMarkerDimension().x;
        }
        else if (getHorizontalBarDimension().isPointInRect(x,y))
        {
            if (x < getHorizontalMarkerDimension().x)
            {
                setHorizontalScrollAmount(getHorizontalScrollAmount()
                                          - (int)(getChildrenArea().width * 0.95));
            }
            else
            {
                setHorizontalScrollAmount(getHorizontalScrollAmount()
                                          + (int)(getChildrenArea().width * 0.95));
            }
        }
    }

    void ScrollArea::mouseReleased(MouseEvent& mouseEvent)
    {
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mIsHorizontalMarkerDragged = false;
        mIsVerticalMarkerDragged = false;

        mouseEvent.consume();
    }

    void ScrollArea::mouseDragged(MouseEvent& mouseEvent)
    {
        if (mIsVerticalMarkerDragged)
        {
            int pos = mouseEvent.getY() - getVerticalBarDimension().y  - mVerticalMarkerDragOffset;
            int length = getVerticalMarkerDimension().height;

            Rectangle barDim = getVerticalBarDimension();

            if ((barDim.height - length) > 0)
            {
                setVerticalScrollAmount((getVerticalMaxScroll() * pos)
                                         / (barDim.height - length));
            }
            else
            {
                setVerticalScrollAmount(0);
            }
        }

        if (mIsHorizontalMarkerDragged)
        {
            int pos = mouseEvent.getX() - getHorizontalBarDimension().x  - mHorizontalMarkerDragOffset;
            int length = getHorizontalMarkerDimension().width;

            Rectangle barDim = getHorizontalBarDimension();

            if ((barDim.width - length) > 0)
            {
                setHorizontalScrollAmount((getHorizontalMaxScroll() * pos)
                                          / (barDim.width - length));
            }
            else
            {
                setHorizontalScrollAmount(0);
            }
        }

        mouseEvent.consume();
    }

    void ScrollArea::draw(Graphics *graphics)
    {
        drawBackground(graphics);

        if (mVBarVisible)
        {
            drawUpButton(graphics);
            drawDownButton(graphics);
            drawVBar(graphics);
            drawVMarker(graphics);
        }

        if (mHBarVisible)
        {
            drawLeftButton(graphics);
            drawRightButton(graphics);
            drawHBar(graphics);
            drawHMarker(graphics);
        }

        if (mHBarVisible && mVBarVisible)
        {
            graphics->setColor(getBaseColor());
            graphics->fillRectangle(Rectangle(getWidth() - mScrollbarWidth,
                                              getHeight() - mScrollbarWidth,
                                              mScrollbarWidth,
                                              mScrollbarWidth));
        }

        drawChildren(graphics);
    }

    void ScrollArea::drawHBar(Graphics* graphics)
    {
        Rectangle dim = getHorizontalBarDimension();

        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color trackColor = getBaseColor() - 0x101010;
        trackColor.a = alpha;
        Color shadowColor = getBaseColor() - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(trackColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(shadowColor);
        graphics->drawLine(0, 0, dim.width, 0);

        graphics->popClipArea();
    }

    void ScrollArea::drawVBar(Graphics* graphics)
    {
        Rectangle dim = getVerticalBarDimension();

        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color trackColor = getBaseColor() - 0x101010;
        trackColor.a = alpha;
        Color shadowColor = getBaseColor() - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(trackColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(shadowColor);
        graphics->drawLine(0, 0, 0, dim.height);

        graphics->popClipArea();
    }

    void ScrollArea::drawBackground(Graphics *graphics)
    {
        if (isOpaque())
        {
        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(getChildrenArea());
    }
    }

    void ScrollArea::drawUpButton(Graphics* graphics)
    {
        Rectangle dim = getUpButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mUpButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.height / 2;
        int h = w / 2 + 2;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(w - i + offset,
                               i + h + offset,
                               w + i + offset,
                               i + h + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawDownButton(Graphics* graphics)
    {
        Rectangle dim = getDownButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mDownButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.height / 2;
        int h = w + 1;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(w - i + offset,
                               -i + h + offset,
                               w + i + offset,
                               -i + h + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawLeftButton(Graphics* graphics)
    {
        Rectangle dim = getLeftButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mLeftButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.width / 2;
        int h = w - 2;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(i + h + offset,
                               w - i + offset,
                               i + h + offset,
                               w + i + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawRightButton(Graphics* graphics)
    {
        Rectangle dim = getRightButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mRightButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.width / 2;
        int h = w + 1;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(-i + h + offset,
                               w - i + offset,
                               -i + h + offset,
                               w + i + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawVMarker(Graphics* graphics)
    {
        Rectangle dim = getVerticalMarkerDimension();
        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color faceColor = getBaseColor();
        faceColor.a = alpha;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(1, 1, dim.width - 1, dim.height - 1));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);

        graphics->popClipArea();
    }

    void ScrollArea::drawHMarker(Graphics* graphics)
    {
        Rectangle dim = getHorizontalMarkerDimension();
        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color faceColor = getBaseColor();
        faceColor.a = alpha;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(1, 1, dim.width - 1, dim.height - 1));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);

        graphics->popClipArea();
    }

    void ScrollArea::logic()
    {
        checkPolicies();

        setVerticalScrollAmount(getVerticalScrollAmount());
        setHorizontalScrollAmount(getHorizontalScrollAmount());

        if (getContent() != NULL)
        {
            getContent()->setPosition(-mHScroll + getContent()->getFrameSize(),
                                      -mVScroll + getContent()->getFrameSize());
            getContent()->logic();
        }
    }

    void ScrollArea::checkPolicies()
    {
        int w = getWidth();
        int h = getHeight();

        mHBarVisible = false;
        mVBarVisible = false;


        if (!getContent())
        {
            mHBarVisible = (mHPolicy == SHOW_ALWAYS);
            mVBarVisible = (mVPolicy == SHOW_ALWAYS);
            return;
        }

        if (mHPolicy == SHOW_AUTO &&
            mVPolicy == SHOW_AUTO)
        {
            if (getContent()->getWidth() <= w
                && getContent()->getHeight() <= h)
            {
                mHBarVisible = false;
                mVBarVisible = false;
            }

            if (getContent()->getWidth() > w)
            {
                mHBarVisible = true;
            }

            if ((getContent()->getHeight() > h)
                || (mHBarVisible && getContent()->getHeight() > h - mScrollbarWidth))
            {
                mVBarVisible = true;
            }

            if (mVBarVisible && getContent()->getWidth() > w - mScrollbarWidth)
            {
                mHBarVisible = true;
            }

            return;
        }

        switch (mHPolicy)
        {
          case SHOW_NEVER:
              mHBarVisible = false;
              break;

          case SHOW_ALWAYS:
              mHBarVisible = true;
              break;

          case SHOW_AUTO:
              if (mVPolicy == SHOW_NEVER)
              {
                  mHBarVisible = getContent()->getWidth() > w;
              }
              else // (mVPolicy == SHOW_ALWAYS)
              {
                  mHBarVisible = getContent()->getWidth() > w - mScrollbarWidth;
              }
              break;

          default:
              throw GCN_EXCEPTION("Horizontal scroll policy invalid.");
        }

        switch (mVPolicy)
        {
          case SHOW_NEVER:
              mVBarVisible = false;
              break;

          case SHOW_ALWAYS:
              mVBarVisible = true;
              break;

          case SHOW_AUTO:
              if (mHPolicy == SHOW_NEVER)
              {
                  mVBarVisible = getContent()->getHeight() > h;
              }
              else // (mHPolicy == SHOW_ALWAYS)
              {
                  mVBarVisible = getContent()->getHeight() > h - mScrollbarWidth;
              }
              break;
          default:
              throw GCN_EXCEPTION("Vertical scroll policy invalid.");
        }
    }

    Rectangle ScrollArea::getUpButtonDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         0,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getDownButtonDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mVBarVisible && mHBarVisible)
        {
            return Rectangle(getWidth() - mScrollbarWidth,
                             getHeight() - mScrollbarWidth*2,
                             mScrollbarWidth,
                             mScrollbarWidth);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         getHeight() - mScrollbarWidth,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getLeftButtonDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        return Rectangle(0,
                         getHeight() - mScrollbarWidth,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getRightButtonDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mVBarVisible && mHBarVisible)
        {
            return Rectangle(getWidth() - mScrollbarWidth*2,
                             getHeight() - mScrollbarWidth,
                             mScrollbarWidth,
                             mScrollbarWidth);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         getHeight() - mScrollbarWidth,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getChildrenArea()
    {
        if (mVBarVisible && mHBarVisible)
        {
            return Rectangle(0, 0, getWidth() - mScrollbarWidth,
                             getHeight() - mScrollbarWidth);
        }

        if (mVBarVisible)
        {
            return Rectangle(0, 0, getWidth() - mScrollbarWidth, getHeight());
        }

        if (mHBarVisible)
        {
            return Rectangle(0, 0, getWidth(), getHeight() - mScrollbarWidth);
        }

        return Rectangle(0, 0, getWidth(), getHeight());
    }

    Rectangle ScrollArea::getVerticalBarDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mHBarVisible)
        {
            return Rectangle(getWidth() - mScrollbarWidth,
                             getUpButtonDimension().height,
                             mScrollbarWidth,
                             getHeight()
                             - getUpButtonDimension().height
                             - getDownButtonDimension().height
                             - mScrollbarWidth);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         getUpButtonDimension().height,
                         mScrollbarWidth,
                         getHeight()
                         - getUpButtonDimension().height
                         - getDownButtonDimension().height);
    }

    Rectangle ScrollArea::getHorizontalBarDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mVBarVisible)
        {
            return Rectangle(getLeftButtonDimension().width,
                             getHeight() - mScrollbarWidth,
                             getWidth()
                             - getLeftButtonDimension().width
                             - getRightButtonDimension().width
                             - mScrollbarWidth,
                             mScrollbarWidth);
        }

        return Rectangle(getLeftButtonDimension().width,
                         getHeight() - mScrollbarWidth,
                         getWidth()
                         - getLeftButtonDimension().width
                         - getRightButtonDimension().width,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getVerticalMarkerDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        int length, pos;
        Rectangle barDim = getVerticalBarDimension();

        if (getContent() && getContent()->getHeight() != 0)
        {
            length = (barDim.height * getChildrenArea().height)
                / getContent()->getHeight();
        }
        else
        {
            length = barDim.height;
        }

        if (length < mScrollbarWidth)
        {
            length = mScrollbarWidth;
        }

        if (length > barDim.height)
        {
            length = barDim.height;
        }

        if (getVerticalMaxScroll() != 0)
        {
            pos = ((barDim.height - length) * getVerticalScrollAmount())
                / getVerticalMaxScroll();
        }
        else
        {
            pos = 0;
        }

        return Rectangle(barDim.x, barDim.y + pos, mScrollbarWidth, length);
    }

    Rectangle ScrollArea::getHorizontalMarkerDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        int length, pos;
        Rectangle barDim = getHorizontalBarDimension();

        if (getContent() && getContent()->getWidth() != 0)
        {
            length = (barDim.width * getChildrenArea().width)
                / getContent()->getWidth();
        }
        else
        {
            length = barDim.width;
        }

        if (length < mScrollbarWidth)
        {
            length = mScrollbarWidth;
        }

        if (length > barDim.width)
        {
            length = barDim.width;
        }

        if (getHorizontalMaxScroll() != 0)
        {
            pos = ((barDim.width - length) * getHorizontalScrollAmount())
                / getHorizontalMaxScroll();
        }
        else
        {
            pos = 0;
        }

        return Rectangle(barDim.x + pos, barDim.y, length, mScrollbarWidth);
    }

    void ScrollArea::showWidgetPart(Widget* widget, Rectangle area)
    {
        if (widget != getContent())
        {
            throw GCN_EXCEPTION("Widget not content widget");
        }

        BasicContainer::showWidgetPart(widget, area);

        setHorizontalScrollAmount(getContent()->getFrameSize() - getContent()->getX());
        setVerticalScrollAmount(getContent()->getFrameSize() - getContent()->getY());
    }

    Widget *ScrollArea::getWidgetAt(int x, int y)
    {
        if (getChildrenArea().isPointInRect(x, y))
        {
            return getContent();
        }

        return NULL;
    }

    void ScrollArea::mouseWheelMovedUp(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed())
        {
            return;
        }
        
        setVerticalScrollAmount(getVerticalScrollAmount() - getChildrenArea().height / 8);

        mouseEvent.consume();
    }

    void ScrollArea::mouseWheelMovedDown(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed())
        {
            return;
        }

        setVerticalScrollAmount(getVerticalScrollAmount() + getChildrenArea().height / 8);

        mouseEvent.consume();
    }

    void ScrollArea::setWidth(int width)
    {
        Widget::setWidth(width);
        checkPolicies();
    }

    void ScrollArea::setHeight(int height)
    {
        Widget::setHeight(height);
        checkPolicies();
    }

    void ScrollArea::setDimension(const Rectangle& dimension)
    {
        Widget::setDimension(dimension);
        checkPolicies();
    }

    void ScrollArea::setLeftButtonScrollAmount(int amount)
    {
        mLeftButtonScrollAmount = amount;
    }

    void ScrollArea::setRightButtonScrollAmount(int amount)
    {
        mRightButtonScrollAmount = amount;
    }

    void ScrollArea::setUpButtonScrollAmount(int amount)
    {
        mUpButtonScrollAmount = amount;
    }

    void ScrollArea::setDownButtonScrollAmount(int amount)
    {
        mDownButtonScrollAmount = amount;
    }

    int ScrollArea::getLeftButtonScrollAmount() const
    {
        return mLeftButtonScrollAmount;
    }

    int ScrollArea::getRightButtonScrollAmount() const
    {
        return mRightButtonScrollAmount;
    }

    int ScrollArea::getUpButtonScrollAmount() const
    {
        return mUpButtonScrollAmount;
    }

    int ScrollArea::getDownButtonScrollAmount() const
    {
        return mDownButtonScrollAmount;
    }

    void ScrollArea::setOpaque(bool opaque)
    {
        mOpaque = opaque;
    }

    
    bool ScrollArea::isOpaque() const
    {
        return mOpaque;
    }
}

/*
 * Wow! This is a looooong source file.
 */
