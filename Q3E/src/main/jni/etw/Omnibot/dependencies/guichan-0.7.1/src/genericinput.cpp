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

/*
 * For comments regarding functions please see the header file.
 */

#include "guichan/genericinput.hpp"

#include "guichan/exception.hpp"

namespace gcn
{
    GenericInput::GenericInput()
    {

    }
    
    void GenericInput::pushKeyPressed(int unicode)
    {
		unicode;
        // TODO
    }
    
    void GenericInput::pushKeyReleased(int unicode)
    {
		unicode;
        // TODO
    }
        
    void GenericInput::pushMouseButtonPressed(int x, int y, int button)
    {
        MouseInput mouseInput;
        mouseInput.setX(x);
        mouseInput.setY(y);
        mouseInput.setButton(button);
        mouseInput.setType(MouseInput::PRESSED);

        mMouseInputQueue.push(mouseInput);
    }
    
    void GenericInput::pushMouseButtonReleased(int x, int y, int button)
    {
        MouseInput mouseInput;
        mouseInput.setX(x);
        mouseInput.setY(y);
        mouseInput.setButton(button);
        mouseInput.setType(MouseInput::RELEASED);

        mMouseInputQueue.push(mouseInput);
    }
    
    void GenericInput::pushMouseWheelMovedUp(int x, int y)
    {
        MouseInput mouseInput;
        mouseInput.setX(x);
        mouseInput.setY(y);
        mouseInput.setType(MouseInput::WHEEL_MOVED_UP);

        mMouseInputQueue.push(mouseInput);
    }
    
    void GenericInput::pushMouseWheelMovedDown(int x, int y)
    {
        MouseInput mouseInput;
        mouseInput.setX(x);
        mouseInput.setY(y);
        mouseInput.setType(MouseInput::WHEEL_MOVED_DOWN);

        mMouseInputQueue.push(mouseInput);
    }
    
    void GenericInput::pushMouseMoved(int x, int y)
    {
        MouseInput mouseInput;
        mouseInput.setX(x);
        mouseInput.setY(y);
        mouseInput.setType(MouseInput::MOVED);

        mMouseInputQueue.push(mouseInput);
    }

    bool GenericInput::isKeyQueueEmpty()
    {
        return mKeyInputQueue.empty();
    }

    KeyInput GenericInput::dequeueKeyInput()
    {
        KeyInput keyInput;

        if (mKeyInputQueue.empty())
        {
            throw GCN_EXCEPTION("The queue is empty.");
        }

        keyInput = mKeyInputQueue.front();
        mKeyInputQueue.pop();

        return keyInput;
    }
    
    bool GenericInput::isMouseQueueEmpty()
    {
        return mMouseInputQueue.empty();
    }

    MouseInput GenericInput::dequeueMouseInput()
    {
        MouseInput mouseInput;
        
        if (mMouseInputQueue.empty())
        {
            throw GCN_EXCEPTION("The queue is empty.");
        }
        
        mouseInput = mMouseInputQueue.front();
        mMouseInputQueue.pop();
        
        return mouseInput;        
    }
    
    void GenericInput::_pollInput()
    {
        // Does nothing.
    }
}
