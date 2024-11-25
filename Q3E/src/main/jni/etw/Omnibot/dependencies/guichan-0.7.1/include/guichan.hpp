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

#ifndef GCN_GUICHAN_HPP
#define GCN_GUICHAN_HPP

#include <guichan/actionevent.hpp>
#include <guichan/actionlistener.hpp>
#include <guichan/cliprectangle.hpp>
#include <guichan/color.hpp>
#include <guichan/deathlistener.hpp>
#include <guichan/event.hpp>
#include <guichan/exception.hpp>
#include <guichan/focushandler.hpp>
#include <guichan/focuslistener.hpp>
#include <guichan/font.hpp>
#include <guichan/genericinput.hpp>
#include <guichan/graphics.hpp>
#include <guichan/gui.hpp>
#include <guichan/image.hpp>
#include <guichan/imagefont.hpp>
#include <guichan/imageloader.hpp>
#include <guichan/input.hpp>
#include <guichan/inputevent.hpp>
#include <guichan/key.hpp>
#include <guichan/keyevent.hpp>
#include <guichan/keyinput.hpp>
#include <guichan/keylistener.hpp>
#include <guichan/listmodel.hpp>
#include <guichan/mouseevent.hpp>
#include <guichan/mouseinput.hpp>
#include <guichan/mouselistener.hpp>
#include <guichan/rectangle.hpp>
#include <guichan/selectionevent.hpp>
#include <guichan/selectionlistener.hpp>
#include <guichan/widget.hpp>
#include <guichan/widgetlistener.hpp>
#include <guichan/widgets/button.hpp>
#include <guichan/widgets/checkbox.hpp>
#include <guichan/widgets/container.hpp>
#include <guichan/widgets/dropdown.hpp>
#include <guichan/widgets/icon.hpp>
#include <guichan/widgets/imagebutton.hpp>
#include <guichan/widgets/label.hpp>
#include <guichan/widgets/listbox.hpp>
#include <guichan/widgets/scrollarea.hpp>
#include <guichan/widgets/slider.hpp>
#include <guichan/widgets/radiobutton.hpp>
#include <guichan/widgets/tab.hpp>
#include <guichan/widgets/tabbedarea.hpp>
#include <guichan/widgets/textbox.hpp>
#include <guichan/widgets/textfield.hpp>
#include <guichan/widgets/window.hpp>

// contribs
#include <adjustingcontainer/adjustingcontainer.hpp>
#include <propertysheet/propertysheet.hpp>

#include "guichan/platform.hpp"


class Widget;

extern "C"
{
    /**
     * Gets the the version of Guichan. As it is a C function
     * it can be used to check for Guichan with autotools.
     *
     * @return the version of Guichan.
     */
    GCN_CORE_DECLSPEC extern const char* gcnGuichanVersion();
}

#endif // end GCN_GUICHAN_HPP
