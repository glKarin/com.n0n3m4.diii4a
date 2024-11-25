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

#ifndef GCN_WINDOW_HPP
#define GCN_WINDOW_HPP

#include <string>

#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widgets/container.hpp"

namespace gcn
{
    /**
     * An implementation of a movable window that can contain other widgets.
     */
    class GCN_CORE_DECLSPEC Window : public Container,
                                     public MouseListener
    {
    public:
        /**
         * Constructor.
         */
        Window();

        /**
         * Constructor. The window will be automatically resized in height
         * to fit the caption.
         *
         * @param caption the caption of the window.
         */
        Window(const std::string& caption);

        /**
         * Destructor.
         */
        virtual ~Window();

        /**
         * Sets the caption of the window.
         *
         * @param caption The caption of the window.
         * @see getCaption
         */
        void setCaption(const std::string& caption);

        /**
         * Gets the caption of the window.
         *
         * @return the caption of the window.
         * @see setCaption
         */
        const std::string& getCaption() const;

        /**
         * Sets the alignment of the caption.
         *
         * @param alignment The alignment of the caption.
         * @see getAlignment, Graphics
         */
        void setAlignment(Graphics::Alignment alignment);

        /**
         * Gets the alignment of the caption.
         *
         * @return The alignment of caption.
         * @see setAlignment, Graphics
         */
        Graphics::Alignment getAlignment() const;

        /**
         * Sets the padding of the window. The padding is the distance between the
         * window border and the content.
         *
         * @param padding The padding of the window.
         * @see getPadding
         */
        void setPadding(unsigned int padding);

        /**
         * Gets the padding of the window. The padding is the distance between the
         * window border and the content.
         *
         * @return The padding of the window.
         * @see setPadding
         */
        unsigned int getPadding() const;

        /**
         * Sets the title bar height.
         *
         * @param height The title height value.
         * @see getTitleBarHeight
         */
        void setTitleBarHeight(unsigned int height);

        /**
         * Gets the title bar height.
         *
         * @return The title bar height.
         * @see setTitleBarHeight
         */
        unsigned int getTitleBarHeight();

        /**
         * Sets the window to be moveble or not.
         *
         * @param movable True if the window should be movable, false otherwise.
         * @see isMovable
         */
        void setMovable(bool movable);

        /**
         * Checks if the window is movable.
         *
         * @return True if the window is movable, false otherwise.
         * @see setMovable
         */
        bool isMovable() const;

        /**
         * Sets the window to be opaque or not. An opaque window will draw it's background
         * and it's content. A non opaque window will only draw it's content.
         *
         * @param opaque True if the window should be opaque, false otherwise.
         * @see isOpaque
         */
        void setOpaque(bool opaque);

        /**
         * Checks if the window is opaque.
         *
         * @return True if the window is opaque, false otherwise.
         * @see setOpaque
         */
        bool isOpaque();

        /**
         * Resizes the window to fit the content.
         */
        virtual void resizeToContent();


        // Inherited from BasicContainer

        virtual Rectangle getChildrenArea();


        // Inherited from Widget

        virtual void draw(Graphics* graphics);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        virtual void mouseReleased(MouseEvent& mouseEvent);

    protected:
        /**
         * Holds the caption of the window.
         */
        std::string mCaption;

        /**
         * Holds the alignment of the caption.
         */
        Graphics::Alignment mAlignment;

        /**
         * Holds the padding of the window.
         */ 
        unsigned int mPadding;

        /**
         * Holds the title bar height of the window.
         */
        unsigned int mTitleBarHeight;

        /**
         * True if the window is movable, false otherwise.
         */
        bool mMovable;

        /**
         * True if the window is opaque, false otherwise.
         */
        bool mOpaque;

        /**
         * Holds a drag offset as an x coordinate where the drag of the window
         * started if the window is being dragged. It's used to move the window 
         * correctly when dragged.
         */
        int mDragOffsetX;

        /**
         * Holds a drag offset as an y coordinate where the drag of the window
         * started if the window is being dragged. It's used to move the window 
         * correctly when dragged.
         */
        int mDragOffsetY;

        /**
         * True if the window is being moved, false otherwise.
         */
        bool mMoved;
    };
}

#endif // end GCN_WINDOW_HPP
