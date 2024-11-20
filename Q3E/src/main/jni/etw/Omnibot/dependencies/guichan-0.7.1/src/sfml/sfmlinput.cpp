/*      _______   __   __   __   ______   __   __   _______   __   __
*     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
*    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
*   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
*  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
* /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
* \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
*
* Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
*
*                                                         Js_./
* Per Larsson a.k.a finalman                          _RqZ{a<^_aa
* Olof Naessén a.k.a jansem/yakslem                _asww7!uY`>  )\a//
*                                                 _Qhm`] _f "'c  1!5m
* Visit: http://guichan.darkbits.org             )Qk<P ` _: :+' .'  "{[
*                                               .)j(] .d_/ '-(  P .   S
* License: (BSD)                                <Td/Z <fP"5(\"??"\a.  .L
* Redistribution and use in source and          _dV>ws?a-?'      ._/L  #'
* binary forms, with or without                 )4d[#7r, .   '     )d`)[
* modification, are permitted provided         _Q-5'5W..j/?'   -?!\)cam'
* that the following conditions are met:       j<<WP+k/);.        _W=j f
* 1. Redistributions of source code must       .$%w\/]Q  . ."'  .  mj$
*    retain the above copyright notice,        ]E.pYY(Q]>.   a     J@\
*    this list of conditions and the           j(]1u<sE"L,. .   ./^ ]{a
*    following disclaimer.                     4'_uomm\.  )L);-4     (3=
* 2. Redistributions in binary form must        )_]X{Z('a_"a7'<a"a,  ]"[
*    reproduce the above copyright notice,       #}<]m7`Za??4,P-"'7. ).m
*    this list of conditions and the            ]d2e)Q(<Q(  ?94   b-  LQ/
*    following disclaimer in the                <B!</]C)d_, '(<' .f. =C+m
*    documentation and/or other materials      .Z!=J ]e []('-4f _ ) -.)m]'
*    provided with the distribution.          .w[5]' _[ /.)_-"+?   _/ <W"
* 3. Neither the name of Guichan nor the      :$we` _! + _/ .        j?
*    names of its contributors may be used     =3)= _f  (_yQmWW$#(    "
*    to endorse or promote products derived     -   W,  sQQQQmZQ#Wwa]..
*    from this software without specific        (js, \[QQW$QWW#?!V"".
*    prior written permission.                    ]y:.<\..          .
*                                                 -]n w/ '         [.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT       )/ )/           !
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY         <  (; sac    ,    '
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING,               ]^ .-  %
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF            c <   r
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR            aga<  <La
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE          5%  )P'-3L
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR        _bQf` y`..)a
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          ,J?4P'.P"_(\?d'.,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES               _Pa,)!f/<[]/  ?"
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT      _2-..:. .r+_,.. .
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     ?a.<%"'  " -'.a_ _,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                     ^
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* For comments regarding functions please see the header file.
*/

#include "guichan/sfml/sfmlinput.hpp"

#include "guichan/exception.hpp"

namespace gcn
{
	SFMLInput::SFMLInput()
	{
		mMouseInWindow = true;
		mMouseDown = false;
	}

	bool SFMLInput::isKeyQueueEmpty()
	{
		return mKeyInputQueue.empty();
	}

	KeyInput SFMLInput::dequeueKeyInput()
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

	bool SFMLInput::isMouseQueueEmpty()
	{
		return mMouseInputQueue.empty();
	}

	MouseInput SFMLInput::dequeueMouseInput()
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

	void SFMLInput::pushInput(const sf::Event &event)
	{
		KeyInput keyInput;
		MouseInput mouseInput;
		static unsigned int MouseX = 0, MouseY = 0;

		switch (event.Type)
		{
		case sf::Event::TextEntered:
			keyInput.setType(KeyInput::PRESSED);

			keyInput.setKey(event.Text.Unicode);			
			keyInput.setShiftPressed(event.Key.Shift);
			keyInput.setControlPressed(event.Key.Control);
			keyInput.setAltPressed(event.Key.Alt);
			//keyInput.setMetaPressed(event.key.keysym.mod & KMOD_META);
			keyInput.setNumericPad(event.Key.Code >= sf::Key::Numpad0
				&& event.Key.Code <= sf::Key::Numpad9);

			if(event.Text.Unicode != '`')
				mKeyInputQueue.push(keyInput);
			break;

		case sf::Event::KeyPressed:
		case sf::Event::KeyReleased:
			if(event.Type == sf::Event::KeyPressed)
				keyInput.setType(KeyInput::PRESSED);
			else
				keyInput.setType(KeyInput::RELEASED);

			keyInput.setKey(Key(convertKeyCharacter(event)));			
			keyInput.setShiftPressed(event.Key.Shift);
			keyInput.setControlPressed(event.Key.Control);
			keyInput.setAltPressed(event.Key.Alt);
			//keyInput.setMetaPressed(event.key.keysym.mod & KMOD_META);
			keyInput.setNumericPad(event.Key.Code >= sf::Key::Numpad0
				&& event.Key.Code <= sf::Key::Numpad9);

			if(!keyInput.getKey().isCharacter())
				mKeyInputQueue.push(keyInput);
			break;

		case sf::Event::MouseWheelMoved:
			mouseInput.setX(MouseX);
			mouseInput.setY(MouseY);
			mouseInput.setButton(MouseInput::EMPTY);

			if(event.MouseWheel.Delta < 0)
				mouseInput.setType(MouseInput::WHEEL_MOVED_DOWN);
			else if(event.MouseWheel.Delta > 0)
				mouseInput.setType(MouseInput::WHEEL_MOVED_UP);
			else
				break;

			mouseInput.setTimeStamp((int)(Clock.GetElapsedTime()*1000.f));
			mMouseInputQueue.push(mouseInput);
			break;

		case sf::Event::MouseButtonPressed:
			mMouseDown = true;
			mouseInput.setX(MouseX);
			mouseInput.setY(MouseY);
			mouseInput.setButton(convertMouseButton(event.MouseButton.Button));
			mouseInput.setType(MouseInput::PRESSED);
			mouseInput.setTimeStamp((int)(Clock.GetElapsedTime()*1000.f));
			mMouseInputQueue.push(mouseInput);
			break;

		case sf::Event::MouseButtonReleased:
			mMouseDown = false;
			mouseInput.setX(MouseX);
			mouseInput.setY(MouseY);
			mouseInput.setButton(convertMouseButton(event.MouseButton.Button));
			mouseInput.setType(MouseInput::RELEASED);
			mouseInput.setTimeStamp((int)(Clock.GetElapsedTime()*1000.f));
			mMouseInputQueue.push(mouseInput);
			break;

		case sf::Event::MouseMoved:
			MouseX = event.MouseMove.X;
			MouseY = event.MouseMove.Y;
			mouseInput.setX(MouseX);
			mouseInput.setY(MouseY);
			mouseInput.setButton(MouseInput::EMPTY);
			mouseInput.setType(MouseInput::MOVED);
			mouseInput.setTimeStamp((int)(Clock.GetElapsedTime()*1000.f));
			mMouseInputQueue.push(mouseInput);
			break;

		case sf::Event::GainedFocus:
			mMouseInWindow = true;
			break;

		case sf::Event::LostFocus:
			mMouseInWindow = false;

			if (!mMouseDown)
			{
				mouseInput.setX(-1);
				mouseInput.setY(-1);
				mouseInput.setButton(MouseInput::EMPTY);
				mouseInput.setType(MouseInput::MOVED);
				mMouseInputQueue.push(mouseInput);
			}
			break;

		} // end switch
	}

	int SFMLInput::convertMouseButton(int button)
	{
		switch (button)
		{
		case sf::Mouse::Left:
			return MouseInput::LEFT;
			break;
		case sf::Mouse::Right:
			return MouseInput::RIGHT;
			break;
		case sf::Mouse::Middle:
			return MouseInput::MIDDLE;
			break;
		default:
			// We have an unknown mouse type which is ignored.
			return button;
		}
	}

	int SFMLInput::convertKeyCharacter(const sf::Event &event)
	{
		int value = (int)event.Key.Code;
		switch (event.Key.Code)
		{
		case sf::Key::Tab:
			value = Key::TAB;
			break;
		/*case SDLK_LALT:
			value = Key::LEFT_ALT;
			break;
		case SDLK_RALT:
			value = Key::RIGHT_ALT;
			break;*/
		case sf::Key::LShift:
			value = Key::LEFT_SHIFT;
			break;
		case sf::Key::RShift:
			value = Key::RIGHT_SHIFT;
			break;
		case sf::Key::LControl:
			value = Key::LEFT_CONTROL;
			break;
		case sf::Key::RControl:
			value = Key::RIGHT_CONTROL;
			break;
		case sf::Key::Back:
			value = Key::BACKSPACE;
			break;
		case sf::Key::Pause:
			value = Key::PAUSE;
			break;
		case sf::Key::Space:
			value = Key::SPACE;
			break;
		case sf::Key::Escape:
			value = Key::ESCAPE;
			break;
		case sf::Key::Delete:
			value = Key::DELETE;
			break;
		case sf::Key::Insert:
			value = Key::INSERT;
			break;
		case sf::Key::Home:
			value = Key::HOME;
			break;
		case sf::Key::End:
			value = Key::END;
			break;
		case sf::Key::PageUp:
			value = Key::PAGE_UP;
			break;
		case sf::Key::PrintScreen:
			value = Key::PRINT_SCREEN;
			break;
		case sf::Key::PageDown:
			value = Key::PAGE_DOWN;
			break;
		case sf::Key::F1:
			value = Key::F1;
			break;
		case sf::Key::F2:
			value = Key::F2;
			break;
		case sf::Key::F3:
			value = Key::F3;
			break;
		case sf::Key::F4:
			value = Key::F4;
			break;
		case sf::Key::F5:
			value = Key::F5;
			break;
		case sf::Key::F6:
			value = Key::F6;
			break;
		case sf::Key::F7:
			value = Key::F7;
			break;
		case sf::Key::F8:
			value = Key::F8;
			break;
		case sf::Key::F9:
			value = Key::F9;
			break;
		case sf::Key::F10:
			value = Key::F10;
			break;
		case sf::Key::F11:
			value = Key::F11;
			break;
		case sf::Key::F12:
			value = Key::F12;
			break;
		case sf::Key::F13:
			value = Key::F13;
			break;
		case sf::Key::F14:
			value = Key::F14;
			break;
		case sf::Key::F15:
			value = Key::F15;
			break;
		case sf::Key::NumLock:
			value = Key::NUM_LOCK;
			break;
		case sf::Key::CapsLock:
			value = Key::CAPS_LOCK;
			break;
		case sf::Key::ScrollLock:
			value = Key::SCROLL_LOCK;
			break;
		case sf::Key::RAlt:
			value = Key::RIGHT_META;
			break;
		case sf::Key::LAlt:
			value = Key::LEFT_META;
			break;
		case sf::Key::LSystem:
			value = Key::LEFT_SUPER;
			break;
		case sf::Key::RSystem:
			value = Key::RIGHT_SUPER;
			break;
		/*case SDLK_MODE:
			value = Key::ALT_GR;
			break;*/
		case sf::Key::Tilde:
			value = Key::ALT_GR;
			break;
		case sf::Key::Up:
			value = Key::UP;
			break;
		case sf::Key::Down:
			value = Key::DOWN;
			break;
		case sf::Key::Left:
			value = Key::LEFT;
			break;
		case sf::Key::Right:
			value = Key::RIGHT;
			break;
		case sf::Key::Return:
			value = Key::ENTER;
			break;
		/*case SDLK_KP_ENTER:
			value = Key::ENTER;
			break;*/
		case sf::Key::Numpad0:
			value = Key::INSERT;
			break;
		case sf::Key::Numpad1:
			value = Key::END;
			break;
		case sf::Key::Numpad2:
			value = Key::DOWN;
			break;
		case sf::Key::Numpad3:
			value = Key::PAGE_DOWN;
			break;
		case sf::Key::Numpad4:
			value = Key::LEFT;
			break;
		case sf::Key::Numpad5:
			value = 0;
			break;
		case sf::Key::Numpad6:
			value = Key::RIGHT;
			break;
		case sf::Key::Numpad7:
			value = Key::HOME;
			break;
		case sf::Key::Numpad8:
			value = Key::UP;
			break;
		case sf::Key::Numpad9:
			value = Key::PAGE_UP;
			break;
		default:
			break;
		}
		return value;
	}
}
