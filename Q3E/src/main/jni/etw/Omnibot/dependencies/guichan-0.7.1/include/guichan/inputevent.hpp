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

#ifndef GCN_INPUTEVENT_HPP
#define GCN_INPUTEVENT_HPP

#include "guichan/event.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    /**
     * Base class for all events concerning input.
     *
     * @author Olof Naessén
     * @since 0.6.0
     */
    class GCN_CORE_DECLSPEC InputEvent: public Event
    {
    public:

        /**
         * Constructor.
         *
         * @param source The source widget of the event.
         * @param isShiftPressed True if shift is pressed, false otherwise.
         * @param isControlPressed True if control is pressed, false otherwise.
         * @param isAltPressed True if alt is pressed, false otherwise.
         * @param isMetaPressed True if meta is pressed, false otherwise.
         */
        InputEvent(Widget* source,
                   bool isShiftPressed,
                   bool isControlPressed,
                   bool isAltPressed,
                   bool isMetaPressed);

        /**
         * Checks if shift is pressed.
         *
         * @return True if shift was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isShiftPressed() const;

        /**
         * Checks if control is pressed.
         *
         * @return True if control was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isControlPressed() const;

        /**
         * Checks if alt is pressed.
         *
         * @return True if alt was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isAltPressed() const;

        /**
         * Checks whether meta is pressed.
         *
         * @return True if meta was pressed at the same time as the key,
         *         false otherwise.
         */
        bool isMetaPressed() const;

        /**
         * Marks the event as consumed. Input event listeners may discard
         * consumed input or act on consumed input. An example of a widget
         * that discards consumed input is the ScrollArea widget that 
         * discards consumed mouse wheel events so the ScrollArea will not 
         * scroll if for instance a Slider's value inside the ScrollArea was 
         * changed with the mouse wheel.
         *
         * @see isConsumed
         */
        void consume();

        /**
         * Checks if the input event is consumed.
         *
         * @return True if the input event is consumed, 
         *         false otherwise.
         * @see consume
         */
        bool isConsumed() const;

    protected:
        /**
         * True if shift is pressed, false otherwise.
         */
        bool mShiftPressed;

        /**
         * True if control is pressed, false otherwise.
         */
        bool mControlPressed;

        /**
         * True if alt is pressed, false otherwise.
         */
        bool mAltPressed;

        /**
         * True if meta is pressed, false otherwise.
         */
        bool mMetaPressed;

        /**
         * True if the input event is consumed, 
         * false otherwise.
         */
        bool mIsConsumed;
    };
}

#endif // end GCN_INPUTEVENT_HPP
