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

#include "guichan/widgets/tabbedarea.hpp"

#include "guichan/exception.hpp"
#include "guichan/focushandler.hpp"
#include "guichan/font.hpp"
#include "guichan/graphics.hpp"

#include "guichan/widgets/container.hpp"
#include "guichan/widgets/tab.hpp"

#include <algorithm>

namespace gcn
{
    TabbedArea::TabbedArea()
            :mSelectedTab(NULL)
    {
        setFocusable(true);
        addKeyListener(this);
        addMouseListener(this);

        mTabContainer = new Container();
        mTabContainer->setOpaque(false);
        mWidgetContainer = new Container();

        add(mTabContainer);
        add(mWidgetContainer);
    }

    TabbedArea::~TabbedArea()
    {
        remove(mTabContainer);
        remove(mWidgetContainer);

        delete mTabContainer;
        delete mWidgetContainer;

        unsigned int i;
        for (i = 0; i < mTabsToDelete.size(); i++)
        {
            delete mTabsToDelete[i];
        }
    }

    void TabbedArea::addTab(const std::string& caption, Widget* widget)
    {
        Tab* tab = new Tab();
        tab->setCaption(caption);
        mTabsToDelete.push_back(tab);

        addTab(tab, widget);
    }

    void TabbedArea::addTab(Tab* tab, Widget* widget)
    {
        tab->setTabbedArea(this);
        tab->addActionListener(this);

        mTabContainer->add(tab);
        mTabs.push_back(std::pair<Tab*, Widget*>(tab, widget));

        if (mSelectedTab == NULL)
        {
            setSelectedTab(tab);
        }

        adjustTabPositions();
        adjustSize();
    }

    void TabbedArea::removeTabWithIndex(int index)
    {
        if (index >= (int)mTabs.size())
        {
            throw GCN_EXCEPTION("No such tab index.");
        }

        removeTab(mTabs[index].first);
    }

    void TabbedArea::removeTab(Tab* tab)
    {
        int tabIndexToBeSelected = - 1;

        if (tab == mSelectedTab)
        {
            int index = getSelectedTabIndex();

            if (index == (int)mTabs.size() - 1
                && mTabs.size() >= 2)
            {
                tabIndexToBeSelected = index--;
            }
            else if (index == (int)mTabs.size() - 1
                     && mTabs.size() == 1)
            {
                tabIndexToBeSelected = -1;
            }
            else
            {
                tabIndexToBeSelected = index;
            }
        }

        std::vector<std::pair<Tab*, Widget*> >::iterator iter;
        for (iter = mTabs.begin(); iter != mTabs.end(); iter++)
        {
            if (iter->first == tab)
            {
                mTabContainer->remove(tab);
                mTabs.erase(iter);
                break;
            }
        }

        std::vector<Tab*>::iterator iter2;
        for (iter2 = mTabsToDelete.begin(); iter2 != mTabsToDelete.end(); iter2++)
        {
            if (*iter2 == tab)
            {
                mTabsToDelete.erase(iter2);
                delete tab;
                break;
            }
        }

        if (tabIndexToBeSelected == -1)
        {
            mSelectedTab = NULL;
            mWidgetContainer->clear();
        }
        else
        {
            setSelectedTab(tabIndexToBeSelected);
        }

        adjustSize();
        adjustTabPositions();
    }

    bool TabbedArea::isTabSelected(int index) const
    {
        if (index >= (int)mTabs.size())
        {
            throw GCN_EXCEPTION("No such tab index.");
        }

        return mSelectedTab == mTabs[index].first;
    }

    bool TabbedArea::isTabSelected(Tab* tab) const
    {
        return mSelectedTab == tab;
    }

    void TabbedArea::setSelectedTab(int index)
    {
        if (index >= (int)mTabs.size())
        {
            throw GCN_EXCEPTION("No such tab index.");
        }

        setSelectedTab(mTabs[index].first);
    }

    void TabbedArea::setSelectedTab(Tab* tab)
    {
        unsigned int i;
        for (i = 0; i < mTabs.size(); i++)
        {
            if (mTabs[i].first == mSelectedTab)
            {
                mWidgetContainer->remove(mTabs[i].second);
            }
        }

        for (i = 0; i < mTabs.size(); i++)
        {
            if (mTabs[i].first == tab)
            {
                mSelectedTab = tab;
                mWidgetContainer->add(mTabs[i].second);
            }
        }
    }

    int TabbedArea::getSelectedTabIndex() const
    {
        unsigned int i;
        for (i = 0; i < mTabs.size(); i++)
        {
            if (mTabs[i].first == mSelectedTab)
            {
                return i;
            }
        }

        return -1;
    }

    Tab* TabbedArea::getSelectedTab()
    {
        return mSelectedTab;
    }

    void TabbedArea::draw(Graphics *graphics)
    {
        const Color &faceColor = getBaseColor();
        const int alpha = getBaseColor().a;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        // Draw a border.
        graphics->setColor(highlightColor);
        graphics->drawLine(0,
                           mTabContainer->getHeight(),
                           0,
                           getHeight() - 2);
        graphics->setColor(shadowColor);
        graphics->drawLine(getWidth() - 1,
                           mTabContainer->getHeight() + 1,
                           getWidth() - 1,
                           getHeight() - 1);
       graphics->drawLine(1,
                          getHeight() - 1,
                          getWidth() - 1,
                          getHeight() - 1);

        graphics->setColor(getBaseColor());
        graphics->fillRectangle(Rectangle(1, 1, getWidth() - 2, getHeight() - 2));


        // Draw a line underneath the tabs.
        graphics->setColor(highlightColor);
        graphics->drawLine(1,
                           mTabContainer->getHeight(),
                           getWidth() - 1,
                           mTabContainer->getHeight());

        // If a tab is selected, remove the line right underneath
        // the selected tab.
        if (mSelectedTab != NULL)
        {
            graphics->setColor(getBaseColor());
            graphics->drawLine(mSelectedTab->getX() + 1,
                               mTabContainer->getHeight(),
                               mSelectedTab->getX() + mSelectedTab->getWidth() - 2,
                               mTabContainer->getHeight());

        }

        drawChildren(graphics);
    }

    void TabbedArea::logic()
    {

    }

    void TabbedArea::adjustSize()
    {
        int maxTabHeight = 0;
        unsigned int i;
        for (i = 0; i < mTabs.size(); i++)
        {
            if (mTabs[i].first->getHeight() > maxTabHeight)
            {
                maxTabHeight = mTabs[i].first->getHeight();
            }
        }

        mTabContainer->setSize(getWidth() - 2,
                               maxTabHeight);

        mWidgetContainer->setPosition(1, maxTabHeight + 1);
        mWidgetContainer->setSize(getWidth() - 2,
                                  getHeight() - maxTabHeight - 2);
    }

    void TabbedArea::adjustTabPositions()
    {
        int maxTabHeight = 0;
        unsigned int i;
        for (i = 0; i < mTabs.size(); i++)
        {
            if (mTabs[i].first->getHeight() > maxTabHeight)
            {
                maxTabHeight = mTabs[i].first->getHeight();
            }
        }

        int x = 0;
        for (i = 0; i < mTabs.size(); i++)
        {
            Tab* tab = mTabs[i].first;
            tab->setPosition(x, maxTabHeight - tab->getHeight());
            x += tab->getWidth();
        }
    }

    void TabbedArea::setWidth(int width)
    {
        Widget::setWidth(width);
        adjustSize();
    }


    void TabbedArea::setHeight(int height)
    {
        Widget::setHeight(height);
        adjustSize();
    }

    void TabbedArea::setSize(int width, int height)
    {
        setWidth(width);
        setHeight(height);
    }

    void TabbedArea::setDimension(const Rectangle& dimension)
    {
        setX(dimension.x);
        setY(dimension.y);
        setWidth(dimension.width);
        setHeight(dimension.height);
    }

    void TabbedArea::keyPressed(KeyEvent& keyEvent)
    {
        if (keyEvent.isConsumed() || !isFocused())
        {
            return;
        }

        if (keyEvent.getKey().getValue() == Key::LEFT)
        {
            int index = getSelectedTabIndex();
            index--;

            if (index < 0)
            {
                return;
            }
            else
            {
                setSelectedTab(mTabs[index].first);
            }

            keyEvent.consume();
        }
        else if (keyEvent.getKey().getValue() == Key::RIGHT)
        {
            int index = getSelectedTabIndex();
            index++;

            if (index >= (int)mTabs.size())
            {
                return;
            }
            else
            {
                setSelectedTab(mTabs[index].first);
            }

            keyEvent.consume();
        }
    }


    void TabbedArea::mousePressed(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed()
            && mouseEvent.getSource()->isFocusable())
        {
            return;
        }

        if (mouseEvent.getButton() == MouseEvent::LEFT)
        {
            Widget* widget = mTabContainer->getWidgetAt(mouseEvent.getX(), mouseEvent.getY());
            Tab* tab = dynamic_cast<Tab*>(widget);

            if (tab != NULL)
            {
                setSelectedTab(tab);
            }
        }

        requestFocus();
    }

    void TabbedArea::death(const Event& event)
    {
        Widget* source = event.getSource();
        Tab* tab = dynamic_cast<Tab*>(source);

        if (tab != NULL)
        {
            removeTab(tab);
        }
        else
        {
            BasicContainer::death(event);
        }
    }

    void TabbedArea::action(const ActionEvent& actionEvent)
    {
        Widget* source = actionEvent.getSource();
        Tab* tab = dynamic_cast<Tab*>(source);

        if (tab == NULL)
        {
            throw GCN_EXCEPTION("Received an action from a widget that's not a tab!");
        }

        setSelectedTab(tab);
    }
}
