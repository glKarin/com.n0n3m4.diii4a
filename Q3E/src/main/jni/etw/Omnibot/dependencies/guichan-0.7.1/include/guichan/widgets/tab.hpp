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

#ifndef GCN_TAB_HPP
#define GCN_TAB_HPP

#include <map>
#include <string>

#include "guichan/basiccontainer.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    class Label;
    class TabbedArea;

    /**
     * An implementation of a simple tab to be used in a tabbed area.
     *
     * @see TabbedArea
     */
    class GCN_CORE_DECLSPEC Tab:
        public BasicContainer,
        public MouseListener
    {
    public:

        /**
         * Constructor.
         */
        Tab();

        /**
         * Destructor.
         */
        virtual ~Tab();

        /**
         * Adjusts the size of the tab fit the caption.
         */
        void adjustSize();

        /**
         * Sets the tabbed area the tab should be a part of.
         *
         * @param tabbedArea The tabbed area the tab should be a part of.
         * @see getTabbedArea
         */
        void setTabbedArea(TabbedArea* tabbedArea);

        /**
         * Gets the tabbed are the tab is a part of.
         *
         * @return The tabbed are the tab is a part of.
         * @see setTabbedArea
         */
        TabbedArea* getTabbedArea();

        /**
         * Sets the caption of the tab. It's advisable to call
         * adjustSize after setting the caption to make the tab
         * fit the caption.
         *
         * @param caption The caption of the tab.
         * @see getCaption, adjustSize
         */
        void setCaption(const std::string& caption);

        /**
         * Gets the caption of the tab.
         *
         * @return The caption of the tab.
         * @see setCaption
         */
        const std::string& getCaption() const;


        // Inherited from Widget

        virtual void draw(Graphics *graphics);


        // Inherited from MouseListener

        virtual void mouseEntered(MouseEvent& mouseEvent);

        virtual void mouseExited(MouseEvent& mouseEvent);

    protected:
        /**
         * Holds the label of the tab.
         */
        Label* mLabel;

        /**
         * True if the tab has the mouse, false otherwise.
         */
        bool mHasMouse;

        /**
         * Holds the tabbed area the tab is a part of.
         */
        TabbedArea* mTabbedArea;
    };
}

#endif // end GCN_TABBEDAREA_HPP
