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

#include "guichan/focushandler.hpp"

#include "guichan/focuslistener.hpp"
#include "guichan/exception.hpp"
#include "guichan/widget.hpp"

namespace gcn
{
    FocusHandler::FocusHandler()
        :mFocusedWidget(NULL),
         mModalFocusedWidget(NULL),
         mModalMouseInputFocusedWidget(NULL),
         mDraggedWidget(NULL),
         mLastWidgetWithMouse(NULL),
         mLastWidgetWithModalFocus(NULL),
         mLastWidgetWithModalMouseInputFocus(NULL),
         mLastWidgetPressed(NULL)
    {

    }

    void FocusHandler::requestFocus(Widget* widget)
    {
        if (widget == NULL
            || widget == mFocusedWidget)
        {
            return;
        }
        
        unsigned int i = 0;
        int toBeFocusedIndex = -1;
        for (i = 0; i < mWidgets.size(); ++i)
        {
            if (mWidgets[i] == widget)
            {
                toBeFocusedIndex = i;
                break;
            }
        }
        
        if (toBeFocusedIndex < 0)
        {
            throw GCN_EXCEPTION("Trying to focus a none existing widget.");
        }
        
        Widget *oldFocused = mFocusedWidget;
        
        if (oldFocused != widget)
        {
            mFocusedWidget = mWidgets.at(toBeFocusedIndex);
            
            if (oldFocused != NULL)
            {
                Event focusEvent(oldFocused);
                distributeFocusLostEvent(focusEvent);
            }
            
            Event focusEvent(mWidgets.at(toBeFocusedIndex));
            distributeFocusGainedEvent(focusEvent);
        }        
    }

    void FocusHandler::requestModalFocus(Widget* widget)
    {
        if (mModalFocusedWidget != NULL && mModalFocusedWidget != widget)
        {
            throw GCN_EXCEPTION("Another widget already has modal focus.");
        }

        mModalFocusedWidget = widget;

        if (mFocusedWidget != NULL 
            && !mFocusedWidget->isModalFocused())
        {
            focusNone();
        }
    }

    void FocusHandler::requestModalMouseInputFocus(Widget* widget)
    {
        if (mModalMouseInputFocusedWidget != NULL
            && mModalMouseInputFocusedWidget != widget)
        {
            throw GCN_EXCEPTION("Another widget already has modal input focus.");
        }

        mModalMouseInputFocusedWidget = widget;
    }

    void FocusHandler::releaseModalFocus(Widget* widget)
    {
        if (mModalFocusedWidget == widget)
        {
            mModalFocusedWidget = NULL;
        }
    }

    void FocusHandler::releaseModalMouseInputFocus(Widget* widget)
    {
        if (mModalMouseInputFocusedWidget == widget)
        {
            mModalMouseInputFocusedWidget = NULL;
        }
    }

    Widget* FocusHandler::getFocused() const
    {
        return mFocusedWidget;
    }

    Widget* FocusHandler::getModalFocused() const
    {
        return mModalFocusedWidget;
    }

    Widget* FocusHandler::getModalMouseInputFocused() const
    {
        return mModalMouseInputFocusedWidget;
    }

    void FocusHandler::focusNext()
    {
        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        int focused = focusedWidget;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            ++focusedWidget;

            if (i==0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget >= (int)mWidgets.size())
            {
                focusedWidget = 0;
            }

            if (focusedWidget == focused)
            {
                return;
            }
        }
        while (!mWidgets.at(focusedWidget)->isFocusable());

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);

            Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::focusPrevious()
    {
        if (mWidgets.size() == 0)
        {
            mFocusedWidget = NULL;
            return;
        }

        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        int focused = focusedWidget;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            --focusedWidget;

            if (i==0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget <= 0)
            {
                focusedWidget = mWidgets.size() - 1;
            }

            if (focusedWidget == focused)
            {
                return;
            }
        }
        while (!mWidgets.at(focusedWidget)->isFocusable());

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);
            Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    bool FocusHandler::isFocused(const Widget* widget) const
    {
        return mFocusedWidget == widget;
    }

    void FocusHandler::add(Widget* widget)
    {
        mWidgets.push_back(widget);
    }

    void FocusHandler::remove(Widget* widget)
    {
        if (isFocused(widget))
        {
            mFocusedWidget = NULL;
        }

        WidgetIterator iter;

        for (iter = mWidgets.begin(); iter != mWidgets.end(); ++iter)
        {
            if ((*iter) == widget)
            {
                mWidgets.erase(iter);
                break;
            }
        }

        if (mDraggedWidget == widget)
        {
            mDraggedWidget = NULL;
            return;
        }   
        
        if (mLastWidgetWithMouse == widget)
        {
            mLastWidgetWithMouse = NULL;
            return;
        }

        if (mLastWidgetWithModalFocus == widget)
        {
            mLastWidgetWithModalFocus = NULL;
            return;
        }

        if (mLastWidgetWithModalMouseInputFocus == widget)
        {
            mLastWidgetWithModalMouseInputFocus = NULL;
            return;
        }

        if (mLastWidgetPressed == widget)
        {
            mLastWidgetPressed = NULL;
            return;
        }
    }

    void FocusHandler::focusNone()
    {
        if (mFocusedWidget != NULL)
        {
            Widget* focused = mFocusedWidget;
            mFocusedWidget = NULL;

            Event focusEvent(focused);
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::tabNext()
    {
        if (mFocusedWidget != NULL)
        {
            if (!mFocusedWidget->isTabOutEnabled())
            {
                return;
            }
        }

        if (mWidgets.size() == 0)
        {
            mFocusedWidget = NULL;
            return;
        }

        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        int focused = focusedWidget;
        bool done = false;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            ++focusedWidget;

            if (i==0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget >= (int)mWidgets.size())
            {
                focusedWidget = 0;
            }

            if (focusedWidget == focused)
            {
                return;
            }

            if (mWidgets.at(focusedWidget)->isFocusable() &&
                mWidgets.at(focusedWidget)->isTabInEnabled() &&
                (mModalFocusedWidget == NULL ||
                 mWidgets.at(focusedWidget)->isModalFocused()))
            {
                done = true;
            }
        }
        while (!done);

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);
            Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::tabPrevious()
    {
        if (mFocusedWidget != NULL)
        {
            if (!mFocusedWidget->isTabOutEnabled())
            {
                return;
            }
        }

        if (mWidgets.size() == 0)
        {
            mFocusedWidget = NULL;
            return;
        }

        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        int focused = focusedWidget;
        bool done = false;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            --focusedWidget;

            if (i==0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget <= 0)
            {
                focusedWidget = mWidgets.size() - 1;
            }

            if (focusedWidget == focused)
            {
                return;
            }

            if (mWidgets.at(focusedWidget)->isFocusable() &&
                mWidgets.at(focusedWidget)->isTabInEnabled() &&
                (mModalFocusedWidget == NULL ||
                 mWidgets.at(focusedWidget)->isModalFocused()))
            {
                done = true;
            }
        }
        while (!done);

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);
            Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::distributeFocusLostEvent(const Event& focusEvent)
    {
        Widget* sourceWidget = focusEvent.getSource();

        std::list<FocusListener*> focusListeners = sourceWidget->_getFocusListeners();

        // Send the event to all focus listeners of the widget.
        for (std::list<FocusListener*>::iterator it = focusListeners.begin();
             it != focusListeners.end();
             ++it)
        {
            (*it)->focusLost(focusEvent);
        }
    }

    void FocusHandler::distributeFocusGainedEvent(const Event& focusEvent)
    {
        Widget* sourceWidget = focusEvent.getSource();

        std::list<FocusListener*> focusListeners = sourceWidget->_getFocusListeners();

        // Send the event to all focus listeners of the widget.
        for (std::list<FocusListener*>::iterator it = focusListeners.begin();
             it != focusListeners.end();
             ++it)
        {
            (*it)->focusGained(focusEvent);
        }
    }

    Widget* FocusHandler::getDraggedWidget()
    {
        return mDraggedWidget;
    }

    void FocusHandler::setDraggedWidget(Widget* draggedWidget)
    {
        mDraggedWidget = draggedWidget;
    }

    Widget* FocusHandler::getLastWidgetWithMouse()
    {
        return mLastWidgetWithMouse;
    }

    void FocusHandler::setLastWidgetWithMouse(Widget* lastWidgetWithMouse)
    {
        mLastWidgetWithMouse = lastWidgetWithMouse;
    }

    Widget* FocusHandler::getLastWidgetWithModalFocus()
    {
        return mLastWidgetWithModalFocus;
    }

    void FocusHandler::setLastWidgetWithModalFocus(Widget* lastWidgetWithModalFocus)
    {
        mLastWidgetWithModalFocus = lastWidgetWithModalFocus;
    }

    Widget* FocusHandler::getLastWidgetWithModalMouseInputFocus()
    {
        return mLastWidgetWithModalMouseInputFocus;
    }

    void FocusHandler::setLastWidgetWithModalMouseInputFocus(Widget* lastWidgetWithModalMouseInputFocus)
    {
        mLastWidgetWithModalMouseInputFocus = lastWidgetWithModalMouseInputFocus;
    }

    Widget* FocusHandler::getLastWidgetPressed()
    {
        return mLastWidgetPressed;
    }

    void FocusHandler::setLastWidgetPressed(Widget* lastWidgetPressed)
    {
        mLastWidgetPressed = lastWidgetPressed;
    }
}
