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

#ifndef GCN_ACTIONEVENT_HPP
#define GCN_ACTIONEVENT_HPP

#include "guichan/event.hpp"
#include "guichan/platform.hpp"

#include <string>

namespace gcn
{
    class Widget;

    /**
     * Represents an action event. An action event is an event
     * that can be fired by a widget whenever an action has occured.
     * What exactly an action is is up to the widget that fires
     * the action event. An example is a Button which fires an action
     * event as soon as the Button is clicked, another example is
     * TextField which fires an action event as soon as the enter
     * key is pressed.
     *
     * Any object can listen for actions from widgets by implementing
     * the ActionListener interface.
     *
     * If you have implement a widget of your own it's a good idea to 
     * let the widget fire action events whenever you feel an action
     * has occured so action listeners of the widget can be informed
     * of the state of the widget.
     *
     * @see Widget::addActionListener, Widget::removeActionListener,
     *      Widget::distributeActionEvent
     * @author Olof Naessén
     * @since 0.6.0
     */
    class GCN_CORE_DECLSPEC ActionEvent: public Event
    {
    public:

        /**
         * Constructor.
         *
         * @param source The source widget of the event.
         * @param id An identifier of the event.
         */
        ActionEvent(Widget* source, const std::string& id);

        /**
         * Destructor.
         */
        virtual ~ActionEvent();

        /**
         * Gets the identifier of the event. An identifier can
         * be used to distinguish from two actions from the same
         * widget or to let many widgets fire the same widgets
         * that should be treated equally.
         *
         * @return The identifier of the event.
         */
        const std::string& getId() const;

    protected:
        /**
         * Holds the identifier of the event.
         */
        std::string mId;
    };
}

#endif // GCN_ACTIONEVENT_HPP

