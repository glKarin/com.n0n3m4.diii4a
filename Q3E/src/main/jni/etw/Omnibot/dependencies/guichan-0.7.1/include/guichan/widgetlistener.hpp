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

#ifndef GCN_WIDGETLISTENER_HPP
#define GCN_WIDGETLISTENER_HPP

#include <string>

#include "guichan/event.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    /**
     * Interface for listening for events from widgets. When a widget's size,
     * location or visibility changes, the relevant method of the listener is
     * invoked.
     *
     * @see Widget::addWidgetListener, Widget::removeWidgetListener
     * @author Olof Naessén
     * @since 0.8.0
     */
    class GCN_CORE_DECLSPEC WidgetListener
    {
    public:

        /**
         * Destructor.
         */
        virtual ~WidgetListener() { }

        /**
         * Invoked when a widget changes its size.
         *
         * @param event Describes the event.
         * @since 0.8.0
         */
        virtual void widgetResized(const Event& event) { event; }

        /**
         * Invoked when a widget is moved.
         *
         * @param event Describes the event.
         * @since 0.8.0
         */
        virtual void widgetMoved(const Event& event) { event; }

        /**
         * Invoked when a widget is hidden, i.e it's set to be
         * not visible.
         *
         * @param event Describes the event.
         * @since 0.8.0
         */
        virtual void widgetHidden(const Event& event) { event; }

        /**
         * Invoked when a widget is shown, i.e it's set to be
         * visible.
         *
         * @param event Describes the event.
         * @since 0.8.0
         */
        virtual void widgetShown(const Event& event) { event; }

        protected:
        /**
         * Constructor.
         *
         * You should not be able to make an instance of WidgetListener,
         * therefore its constructor is protected.
         */
        WidgetListener() { }

    };
}

#endif // end GCN_WIDGETLISTENER_HPP
