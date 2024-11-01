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

#ifndef GCN_BUTTON_HPP
#define GCN_BUTTON_HPP

#include <string>

#include "guichan/focuslistener.hpp"
#include "guichan/graphics.hpp"
#include "guichan/keylistener.hpp"
#include "guichan/mouseevent.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"

namespace gcn
{
    /**
     * An implementation of a regular clickable button. A button is capable of
     * displaying a caption. 
     *
     * If a button is clicked an action event will be sent to all action listener's 
     * of the button.
     *
     * @see ImageButton
     */
    class GCN_CORE_DECLSPEC Button : public Widget,
                                     public MouseListener,
                                     public KeyListener,
                                     public FocusListener
    {
    public:
        /**
         * Constructor.
         */
        Button();

        /**
         * Constructor. The button will be automatically resized
         * to fit the caption.
         *
         * @param caption The caption of the button.
         */
        Button(const std::string& caption);

        /**
         * Sets the caption of the button. It's advisable to call
         * adjustSize after setting of the caption to adjust the
         * button's size to fit the caption.
         *
         * @param caption The caption of the button.
         * @see getCaption, adjustSize
         */
        void setCaption(const std::string& caption);

        /**
         * Gets the caption of the button.
         *
         * @return The caption of the button.
         */
        const std::string& getCaption() const;

        /**
         * Sets the alignment of the caption. The alignment is relative
         * to the center of the button.
         *
         * @param alignment The alignment of the caption.
         * @see getAlignment, Graphics
         */
        void setAlignment(Graphics::Alignment alignment);

        /**
         * Gets the alignment of the caption.
         *
         * @return The alignment of the caption.
         * @see setAlignment, Graphics
         */
        Graphics::Alignment getAlignment() const;

        /**
         * Sets the spacing between the border of the button and its caption.
         *
         * @param spacing The default value for spacing is 4 and can be changed 
         *                using this method.
         * @see getSpacing
         */
        void setSpacing(unsigned int spacing);

        /**
         * Gets the spacing between the border of the button and its caption.
         *
         * @return spacing.
         * @see setSpacing
         */
        unsigned int getSpacing() const;

        /**
         * Adjusts the button's size to fit the caption.
         */
        void adjustSize();


        //Inherited from Widget

        virtual void draw(Graphics* graphics);


        // Inherited from FocusListener

        virtual void focusLost(const Event& event);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseReleased(MouseEvent& mouseEvent);

        virtual void mouseEntered(MouseEvent& mouseEvent);

        virtual void mouseExited(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);

        virtual void keyReleased(KeyEvent& keyEvent);

    protected:
        /**
         * Checks if the button is pressed. Convenient method to use
         * when overloading the draw method of the button.
         *
         * @return True if the button is pressed, false otherwise.
         */
        bool isPressed() const;

        /**
         * Holds the caption of the button.
         */
        std::string mCaption;

        /**
         * True if the mouse is ontop of the button, false otherwise.
         */
        bool mHasMouse;

        /**
         * True if a key has been pressed, false otherwise.
         */
        bool mKeyPressed;

        /**
         * True if a mouse has been pressed, false otherwise.
         */
        bool mMousePressed;

		bool mActivateOnKbPress;

        /**
         * Holds the alignment of the caption.
         */
        Graphics::Alignment mAlignment;

        /**
         * Holds the spacing between the border and the caption.
         */
        unsigned int mSpacing;
    };
}

#endif // end GCN_BUTTON_HPP
