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

#include "guichan/allegro/allegroinput.hpp"

#include <allegro.h>

#include "guichan/exception.hpp"

namespace gcn
{
    AllegroInput::AllegroInput()
    {
        mMouseButton1 = mMouseButton2 = mMouseButton3 = false;
        mLastMouseZ = 0;
        mLastMouseX = 0;
        mLastMouseY = 0;
    }

    bool AllegroInput::isKeyQueueEmpty()
    {
        return mKeyQueue.empty();
    }

    KeyInput AllegroInput::dequeueKeyInput()
    {
        if (isKeyQueueEmpty())
        {
            throw GCN_EXCEPTION("Key queue is empty.");
        }

        KeyInput ki = mKeyQueue.front();
        mKeyQueue.pop();

        return ki;
    }

    bool AllegroInput::isMouseQueueEmpty()
    {
        return mMouseQueue.empty();
    }

    MouseInput AllegroInput::dequeueMouseInput()
    {
        if (isMouseQueueEmpty())
        {
            throw GCN_EXCEPTION("Mouse queue is empty.");
        }

        MouseInput mi = mMouseQueue.front();
        mMouseQueue.pop();

        return mi;
    }

    void AllegroInput::_pollInput()
    {
        pollMouseInput();
        pollKeyInput();
    }

    void AllegroInput::pollMouseInput()
    {
        if (mouse_needs_poll())
        {
            poll_mouse();
        }
        int mouseX = mouse_x;
        int mouseY = mouse_y;
        int mouseZ = mouse_z;
        int mouseB1 = mouse_b & 1;
        int mouseB2 = mouse_b & 2;
        int mouseB3 = mouse_b & 4;

        // Check mouse movement
        if (mouseX != mLastMouseX || mouseY != mLastMouseY)
        {
            mMouseQueue.push(MouseInput(MouseInput::EMPTY,
                                        MouseInput::MOVED,
                                        mouseX,
                                        mouseY,
                                        0));
            mLastMouseX = mouseX;
            mLastMouseY = mouseY;
        }

        // Check mouse Wheel
        while (mLastMouseZ < mouseZ)
        {
            mMouseQueue.push(MouseInput(MouseInput::EMPTY,
                                        MouseInput::WHEEL_MOVED_UP,
                                        mouseX,
                                        mouseY,
                                        0));
            mLastMouseZ++;
        }

        while (mLastMouseZ > mouseZ)
        {
            mMouseQueue.push(MouseInput(MouseInput::EMPTY,
                                        MouseInput::WHEEL_MOVED_DOWN,
                                        mouseX,
                                        mouseY,
                                        0));
            mLastMouseZ--;
        }

        // Check mouse buttons
        if (!mMouseButton1 && mouseB1)
        {
            mMouseQueue.push(MouseInput(MouseInput::LEFT,
                                        MouseInput::PRESSED,
                                        mouseX,
                                        mouseY,
                                        0));
        }

        if (mMouseButton1 && !mouseB1)
        {
            mMouseQueue.push(MouseInput(MouseInput::LEFT,
                                        MouseInput::RELEASED,
                                        mouseX,
                                        mouseY,
                                        0));
        }


        if (!mMouseButton2 && mouseB2)
        {
            mMouseQueue.push(MouseInput(MouseInput::RIGHT,
                                        MouseInput::PRESSED,
                                        mouseX,
                                        mouseY,
                                        0));
        }

        if (mMouseButton2 && !mouseB2)
        {
            mMouseQueue.push(MouseInput(MouseInput::RIGHT,
                                        MouseInput::RELEASED,
                                        mouseX,
                                        mouseY,
                                        0));
        }


        if (!mMouseButton3 && mouseB3)
        {
            mMouseQueue.push(MouseInput(MouseInput::MIDDLE,
                                        MouseInput::PRESSED,
                                        mouseX,
                                        mouseY,
                                        0));
        }

        if (mMouseButton3 && !mouseB3)
        {
            mMouseQueue.push(MouseInput(MouseInput::MIDDLE,
                                        MouseInput::RELEASED,
                                        mouseX,
                                        mouseY,
                                        0));
        }

        mMouseButton1 = mouseB1;
        mMouseButton2 = mouseB2;
        mMouseButton3 = mouseB3;
    }

    void AllegroInput::pollKeyInput()
    {
        int unicode, scancode;

        if (keyboard_needs_poll())
        {
            poll_keyboard();
        }

        while (keypressed())
        {
            unicode = ureadkey(&scancode);
            Key keyObj = convertToKey(scancode, unicode);

            KeyInput keyInput(keyObj, KeyInput::PRESSED);

            keyInput.setNumericPad(isNumericPad(scancode));
            keyInput.setShiftPressed(key_shifts & KB_SHIFT_FLAG);
            keyInput.setAltPressed(key_shifts & KB_ALT_FLAG);
            keyInput.setControlPressed(key_shifts & KB_CTRL_FLAG);
#ifdef KB_COMMAND_FLAG
            keyInput.setMetaPressed(key_shifts & (KB_COMMAND_FLAG |
                                           KB_LWIN_FLAG |
                                           KB_RWIN_FLAG));
#else
            keyInput.setMetaPressed(key_shifts & (KB_LWIN_FLAG |
                                                  KB_RWIN_FLAG));
#endif


            mKeyQueue.push(keyInput);

            mPressedKeys[scancode] = keyInput;
        }

        if (key[KEY_ALT] && mPressedKeys.find(KEY_ALT) == mPressedKeys.end())
		{
			KeyInput keyInput(convertToKey(KEY_ALT, 0), KeyInput::PRESSED);
            mKeyQueue.push(keyInput);
            mPressedKeys[KEY_ALT] = keyInput;
		}

		if (key[KEY_ALTGR] && mPressedKeys.find(KEY_ALTGR) == mPressedKeys.end())
		{
			KeyInput keyInput(convertToKey(KEY_ALTGR, 0), KeyInput::PRESSED);
            mKeyQueue.push(keyInput);
            mPressedKeys[KEY_ALTGR] = keyInput;
		}

		if (key[KEY_LSHIFT] && mPressedKeys.find(KEY_LSHIFT) == mPressedKeys.end())
		{
			KeyInput keyInput(convertToKey(KEY_LSHIFT, 0), KeyInput::PRESSED);
            mKeyQueue.push(keyInput);
            mPressedKeys[KEY_LSHIFT] = keyInput;
		}

		if (key[KEY_RSHIFT] && mPressedKeys.find(KEY_RSHIFT) == mPressedKeys.end())
		{
			KeyInput keyInput(convertToKey(KEY_RSHIFT, 0), KeyInput::PRESSED);
            mKeyQueue.push(keyInput);
            mPressedKeys[KEY_RSHIFT] = keyInput;
		}
		
		if (key[KEY_LCONTROL] && mPressedKeys.find(KEY_LCONTROL) == mPressedKeys.end())
		{
			KeyInput keyInput(convertToKey(KEY_LCONTROL, 0), KeyInput::PRESSED);
            mKeyQueue.push(keyInput);
            mPressedKeys[KEY_LCONTROL] = keyInput;
		}

		if (key[KEY_RCONTROL] && mPressedKeys.find(KEY_RCONTROL) == mPressedKeys.end())
		{
			KeyInput keyInput(convertToKey(KEY_RCONTROL, 0), KeyInput::PRESSED);
            mKeyQueue.push(keyInput);
            mPressedKeys[KEY_RCONTROL] = keyInput;
		}

         // Check for released keys
        std::map<int, KeyInput>::iterator iter, tempIter;
        for (iter = mPressedKeys.begin(); iter != mPressedKeys.end(); )
         {
            if (!key[iter->first])
            {
                KeyInput keyInput(iter->second.getKey(), KeyInput::RELEASED);
                keyInput.setNumericPad(iter->second.isNumericPad());
                keyInput.setShiftPressed(iter->second.isShiftPressed());
                keyInput.setAltPressed(iter->second.isAltPressed());
                keyInput.setControlPressed(iter->second.isControlPressed());

                mKeyQueue.push(keyInput);

                tempIter = iter;
                iter++;
                mPressedKeys.erase(tempIter);
            }
            else
            {
                iter++;
            }
        }
    }

    Key AllegroInput::convertToKey(int scancode, int unicode)
    {
        int keysym;
        bool pad = false;

        switch(scancode)
        {
          case KEY_ESC:
              keysym = Key::ESCAPE;
              break;

          case KEY_ALT:
              keysym = Key::LEFT_ALT;
              break;

          case KEY_ALTGR:
              keysym = Key::RIGHT_ALT;
              break;

          case KEY_LSHIFT:
              keysym = Key::LEFT_SHIFT;
              break;

          case KEY_RSHIFT:
              keysym = Key::RIGHT_SHIFT;
              break;

          case KEY_LCONTROL:
              keysym = Key::LEFT_CONTROL;
              break;

          case KEY_RCONTROL:
              keysym = Key::RIGHT_CONTROL;
              break;

          case KEY_LWIN:
              keysym = Key::LEFT_META;
              break;

          case KEY_RWIN:
              keysym = Key::RIGHT_META;
              break;

          case KEY_INSERT:
              keysym = Key::INSERT;
              break;

          case KEY_HOME:
              keysym = Key::HOME;
              break;

          case KEY_PGUP:
              keysym = Key::PAGE_UP;
              break;

          case KEY_PGDN:
              keysym = Key::PAGE_DOWN;
              break;

          case KEY_DEL:
              keysym = Key::DELETE;
              break;

          case KEY_DEL_PAD:
              keysym = Key::DELETE;
              pad = true;
              break;

          case KEY_END:
              keysym = Key::END;
              break;

          case KEY_CAPSLOCK:
              keysym = Key::CAPS_LOCK;
              break;

          case KEY_BACKSPACE:
              keysym = Key::BACKSPACE;
              break;

          case KEY_F1:
              keysym = Key::F1;
              break;

          case KEY_F2:
              keysym = Key::F2;
              break;

          case KEY_F3:
              keysym = Key::F3;
              break;

          case KEY_F4:
              keysym = Key::F4;
              break;

          case KEY_F5:
              keysym = Key::F5;
              break;

          case KEY_F6:
              keysym = Key::F6;
              break;

          case KEY_F7:
              keysym = Key::F7;
              break;

          case KEY_F8:
              keysym = Key::F8;
              break;

          case KEY_F9:
              keysym = Key::F9;
              break;

          case KEY_F10:
              keysym = Key::F10;
              break;

          case KEY_F11:
              keysym = Key::F11;
              break;

          case KEY_F12:
              keysym = Key::F12;
              break;

          case KEY_PRTSCR:
              keysym = Key::PRINT_SCREEN;
              break;

          case KEY_PAUSE:
              keysym = Key::PAUSE;
              break;

          case KEY_SCRLOCK:
              keysym = Key::SCROLL_LOCK;
              break;

          case KEY_NUMLOCK:
              keysym = Key::NUM_LOCK;
              break;

          case KEY_LEFT:
              keysym = Key::LEFT;
              break;

          case KEY_RIGHT:
              keysym = Key::RIGHT;
              break;

          case KEY_UP:
              keysym = Key::UP;
              break;

          case KEY_DOWN:
              keysym = Key::DOWN;
              break;

          case KEY_ENTER_PAD:
              pad = true;
          case KEY_ENTER:
              keysym = Key::ENTER;
              break;
     default:
              keysym = unicode;
        }

        Key k = Key(keysym);

        return k;

        //Now, THAT was fun to code! =D =D =D
    }

    bool AllegroInput::isNumericPad(int scancode)
    {
        switch (scancode)
        {
          case KEY_0_PAD:
          case KEY_1_PAD:
          case KEY_2_PAD:
          case KEY_3_PAD:
          case KEY_4_PAD:
          case KEY_5_PAD:
          case KEY_6_PAD:
          case KEY_7_PAD:
          case KEY_8_PAD:
          case KEY_9_PAD:
          case KEY_SLASH_PAD:
          case KEY_MINUS_PAD:
          case KEY_PLUS_PAD:
              return true;
          default:
              return false;
        }
    }
}
