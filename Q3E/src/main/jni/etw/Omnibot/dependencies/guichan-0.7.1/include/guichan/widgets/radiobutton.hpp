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

#ifndef GCN_RADIOBUTTON_HPP
#define GCN_RADIOBUTTON_HPP

#include <map>
#include <string>

#include "guichan/keylistener.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"

namespace gcn
{
    /**
     * An implementation of a radio button where a user can select or deselect
     * the radio button and where the status of the radio button is displayed to the user.
     * A radio button can belong to a group and when a radio button belongs to a
     * group only one radio button can be selected in the group. A radio button is
     * capable of displaying a caption.
     * 
     * If a radio button's state changes an action event will be sent to all action 
     * listeners of the check box.
     */
    class GCN_CORE_DECLSPEC RadioButton :
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:

        /**
         * Constructor.
         */
        RadioButton();

        /**
         * Constructor. The radio button will be automatically resized
         * to fit the caption.
         *
         * @param caption The caption of the radio button.
         * @param group The group the radio button should belong to.
         * @param selected True if the radio button should be selected.
         */
        RadioButton(const std::string &caption,
                    const std::string &group,
                    bool selected = false);

        /**
         * Destructor.
         */
        virtual ~RadioButton();

        /**
         * Checks if the radio button is selected.
         *
         * @return True if the radio button is selecte, false otherwise.
         * @see setSelected
         */
        bool isSelected() const;

        /**
         * Sets the radio button to selected or not.
         *
         * @param selected True if the radio button should be selected,
         *                 false otherwise.
         * @see isSelected
         */
        void setSelected(bool selected);

        /**
         * Gets the caption of the radio button.
         *
         * @return The caption of the radio button.
         * @see setCaption
         */
        const std::string &getCaption() const;

        /**
         * Sets the caption of the radio button. It's advisable to call
         * adjustSize after setting of the caption to adjust the
         * radio button's size to fit the caption.
         *
         * @param caption The caption of the radio button.
         * @see getCaption, adjustSize
         */
        void setCaption(const std::string caption);

        /**
         * Sets the group the radio button should belong to. Note that
         * a radio button group is unique per application, not per Gui object
         * as the group is stored in a static map.
         *
         * @param group The name of the group.
         * @see getGroup
         */
        void setGroup(const std::string &group);

        /**
         * Gets the group the radio button belongs to.
         *
         * @return The group the radio button belongs to.
         * @see setGroup
         */
        const std::string &getGroup() const;

        /**
         * Adjusts the radio button's size to fit the caption.
         */
        void adjustSize();


        // Inherited from Widget

        virtual void draw(Graphics* graphics);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);


        // Inherited from MouseListener

        virtual void mouseClicked(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

    protected:
        /**
         * Draws the box.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawBox(Graphics *graphics);

        /**
         * True if the radio button is selected, false otherwise.
         */
        bool mSelected;

        /**
         * Holds the caption of the radio button.
         */ 
        std::string mCaption;

        /**
         * Holds the group of the radio button.
         */
        std::string mGroup;

        /**
         * Typdef.
         */
        typedef std::multimap<std::string, RadioButton *> GroupMap;

        /**
         * Typdef.
         */
        typedef GroupMap::iterator GroupIterator;

        /**
         * Holds all available radio button groups.
         */
        static GroupMap mGroupMap;
    };
}

#endif // end GCN_RADIOBUTTON_HPP
