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

#ifndef GCN_TEXTFIELD_HPP
#define GCN_TEXTFIELD_HPP

#include "guichan/keylistener.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"

#include <string>

namespace gcn
{
    /**
     * An implementation of a text field where a user can enter a line of text.
     */
    class GCN_CORE_DECLSPEC TextField:
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:
        /**
         * Constructor.
         */
        TextField();

        /**
         * Constructor. The text field will be automatically resized
         * to fit the text.
         *
         * @param text The default text of the text field.
         */
        TextField(const std::string& text);

        /**
         * Sets the text of the text field.
         *
         * @param text The text of the text field.
         * @see getText
         */
        void setText(const std::string& text);

        /**
         * Gets the text of the text field.
         *
         * @return The text of the text field.
         * @see setText
         */
        const std::string& getText() const;

        /**
         * Adjusts the size of the text field to fit the text.
         */
        void adjustSize();

        /**
         * Adjusts the height of the text field to fit caption.
         */
        void adjustHeight();

        /**
         * Sets the caret position. As there is only one line of text
         * in a text field the position is the caret's x coordinate.
         *
         * @param position The caret position.
         * @see getCaretPosition
         */
        void setCaretPosition(int position);

        /**
         * Gets the caret position. As there is only one line of text
         * in a text field the position is the caret's x coordinate.
         *
         * @return The caret position.
         * @see setCaretPosition
         */
        int getCaretPosition() const;

		bool isReadOnly() const { return mReadOnly; }
		void setReadOnly(bool b) { mReadOnly = b; }

        // Inherited from Widget

        virtual void fontChanged();

        virtual void draw(Graphics* graphics);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);
        

        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);

    protected:
        /**
         * Draws the caret. Overloaded this method if you want to
         * change the style of the caret.
         *
         * @param graphics the Graphics object to draw with.
         * @param x the caret's x-position.
         */
        virtual void drawCaret(Graphics* graphics, int x);

        /**
         * Scrolls the text horizontally so that the caret shows if needed.
         * The method is used any time a user types in the text field so the
         * caret always will be shown.
         */
        void fixScroll();

        /**
         * Holds the text of the text box.
         */
        std::string mText;

        /**
         * Holds the caret position.
         */
        int mCaretPosition;

        /**
         * Holds the amount scrolled in x. If a user types more characters than
         * the text field can display, due to the text field being to small, the
         * text needs to scroll in order to show the last type character.
         */
        int mXScroll;

		bool mReadOnly;
    };
}

#endif // end GCN_TEXTFIELD_HPP
