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

#ifndef GCN_HGEINPUT_HPP
#define GCN_HGEINPUT_HPP

#include <queue>
#include <hge.h>

#if defined(DELETE)
#undef DELETE
#endif

#include "guichan/input.hpp"
#include "guichan/keyinput.hpp"
#include "guichan/mouseinput.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    /**
     * HGE implementation of Input.
     *
     * @author Kevin Lynx
     * @since 0.6.1
     */
    class GCN_EXTENSION_DECLSPEC HGEInput : public Input
    {
    public:

        /**
         *  Constructor.
         */
        HGEInput();

        /**
         *  Destructor.
         */
        virtual ~HGEInput();


        //Inherited from Input

        virtual bool isKeyQueueEmpty();

        virtual bool isMouseQueueEmpty();
    
        virtual KeyInput dequeueKeyInput();
    
        virtual MouseInput dequeueMouseInput();

        virtual void _pollInput();

    protected:
        /**
         * Handles key input. The function is called by _pollInput.
         *
         * @param ki the hge input event to handle.
         */
        void pollKeyInput(hgeInputEvent &ki);
    
        /**
         * Handles mouse input. The function is called by _pollInput.
         *
         * This function directly deal with the mouse input , and it avoid 
         * the odd things in HGE.
         */
        void pollMouseInput();

        /**
         * Converts the keycode and characters to a Key object.
         *
         * @param key The key to convert.
         * @param chr The character to convert.
         */
        Key convertToKey(int key, int chr);
    
        /**
         * Checks if a keyCode is numeric.
         *
         * @param keyCode The key code to check.
         */
        bool isNumericPad(int keyCode);

    protected:
        static HGE *mHGE;

        std::queue<KeyInput> mKeyInputQueue;
        std::queue<MouseInput> mMouseInputQueue;

        float mMouseX;
        float mMouseY;

        bool mLeftMouseButtonDown;
        bool mRightMouseButtonDown;
        bool mMiddleMouseButtonDown;
    };
}

#endif // end GCN_HGEINPUT_HPP
