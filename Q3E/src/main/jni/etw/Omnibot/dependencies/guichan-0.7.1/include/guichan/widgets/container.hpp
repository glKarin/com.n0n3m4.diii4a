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

#ifndef GCN_CONTAINER_HPP
#define GCN_CONTAINER_HPP

#include <list>

#include "guichan/basiccontainer.hpp"
#include "guichan/graphics.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    /**
     * An implementation of a container able to contain other widgets. A widget's 
     * position in the container is relative to the container itself and not the screen. 
     * A container is the most common widget to use as the Gui's top widget as makes the Gui
     * able to contain more than one widget.
     *
     * @see Gui::setTop
     */
    class GCN_CORE_DECLSPEC Container: public BasicContainer
    {
    public:

        /**
         * Constructor. A container is opauqe as default, if you want a
         * none opaque container call setQpaque(false).
         *
         * @see setOpaque, isOpaque
         */
        Container();

        /**
         * Destructor.
         */
        virtual ~Container();

        /**
         * Sets the container to be opaque or not. If the container
         * is opaque it's background will be drawn, if it's not opaque 
         * it's background will not be drawn, and thus making the container
         * completely transparent.
         *
         * NOTE: This is not the same as to set visibility. A non visible
         *       container will not itself nor will it draw it's content.
         *
         * @param opaque True if the container should be opaque, false otherwise.
         * @see isOpaque
         */
        void setOpaque(bool opaque);

        /**
         * Checks if the container is opaque or not.
         *
         * @return True if the container is opaque, false otherwise.
         * @see setOpaque
         */
        bool isOpaque() const;

        /**
         * Adds a widget to the container.
         *
         * @param widget The widget to add.
         * @see remove, clear
         */
        virtual void add(Widget* widget);

        /**
         * Adds a widget to the container and also specifices the widget's
         * postion in the container. The position is relative to the container
         * and not relative to the screen.
         *
         * @param widget The widget to add.
         * @param x The x coordinat for the widget.
         * @param y The y coordinat for the widget.
         * @see remove, clear
         */
        virtual void add(Widget* widget, int x, int y);

        /**
         * Removes a widget from the Container.
         *
         * @param widget The widget to remove.
         * @throws Exception when the widget has not been added to the
         *                   container.
         * @see add, clear
         */
        virtual void remove(Widget* widget);

        /**
         * Clears the container of all widgets.
         *
         * @see add, remove
         */
        virtual void clear(bool deletewidget = false);

        /**
         * Finds a widget given an id.
         *
         * @param id The id to find a widget by.
         * @return A widget with a corrosponding id, NULL if no widget 
         *         is found.
         * @see Widget::setId
         */
        virtual Widget* findWidgetById(const std::string &id);


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

    protected:
        /**
         * True if the container is opaque, false otherwise.
         */
        bool mOpaque;
    };
}

#endif // end GCN_CONTAINER_HPP
