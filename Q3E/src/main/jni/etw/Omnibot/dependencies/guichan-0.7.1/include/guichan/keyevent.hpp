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

#ifndef GCN_KEYEVENT_HPP
#define GCN_KEYEVENT_HPP

#include "guichan/inputevent.hpp"
#include "guichan/key.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    class Widget;

    /**
     * Represents a key event.
     */
    class GCN_CORE_DECLSPEC KeyEvent: public InputEvent
    {
    public:
        /**
         * Key event types.
         */
        enum
        {
            PRESSED = 0,
            RELEASED
        };

        /**
         * Constructor.
         *
         * @param source The source widget of the event.
         * @param isShiftPressed True if shift is pressed, false otherwise.
         * @param isControlPressed True if control is pressed, false otherwise.
         * @param isAltPressed True if alt is pressed, false otherwise.
         * @param isMetaPressed True if meta is pressed, false otherwise.
         * @param type The type of the event. A value from KeyEventType.
         * @param isNumericPad True if the event occured on the numeric pad,
         *                     false otherwise.
         * @param key The key of the event.
         */
        KeyEvent(Widget* source,
                 bool isShiftPressed,
                 bool isControlPressed,
                 bool isAltPressed,
                 bool isMetaPressed,
                 unsigned int type,
                 bool isNumericPad,
                 const Key& key);

        /**
         * Destructor.
         */
        virtual ~KeyEvent();

        /**
         * Gets the type of the event.
         *
         * @return The type of the event.
         */
         unsigned int getType() const;

        /**
         * Checks if the key event occured on the numeric pad.
         *
         * @return True if key event occured on the numeric pad,
         *         false otherwise.
         *
         */
        bool isNumericPad() const;

        /**
         * Gets the key of the event.
         *
         * @return The key of the event.
         */
        const Key& getKey() const;

    protected:
        /**
         * Holds the type of the key event.
         */
        unsigned int mType;

        /**
         * True if the numeric pad was used, false otherwise.
         */
        bool mIsNumericPad;

        /** 
         * Holds the key of the key event.
         */
        Key mKey;
    };
}

#endif // end GCN_KEYEVENT_HPP
