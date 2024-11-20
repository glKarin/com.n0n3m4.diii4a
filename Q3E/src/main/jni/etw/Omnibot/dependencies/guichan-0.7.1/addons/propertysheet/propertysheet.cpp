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

#include "propertysheet.hpp"
#include "guichan/widgets/label.hpp"

#include "guichan/exception.hpp"
#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/mouseinput.hpp"

template<class T> inline T max(T a, T b) { return a > b ? a : b; }

namespace gcn
{
	namespace contrib
	{
		PropertySheet::PropertySheet()
			: mSpacing(32)
			, mColorRows(true)
		{
		}

		PropertySheet::~PropertySheet()
		{
		}

		void PropertySheet::logic()
		{
			Container::logic();
			adjustContent();
		}

		void PropertySheet::draw(Graphics* graphics)
		{
			const bool opaque = isOpaque();
			if(mColorRows)
			{
				// hack so the parent doesn't render the background, since we're handlign that here.
				setOpaque(false);

				gcn::Color colors[2] =
				{
					getBaseColor(),
					getBaseColor() * 0.9f,
				};

				int labelWidth = 0;
				int widgetWidth = 0;

				// find the biggest lablel/widget
				for(PropertyList::iterator it = mPropertyList.begin();
					it != mPropertyList.end();
					++it)
				{
					Property &p = (*it);
					labelWidth = max(p.mLabel->getWidth(), labelWidth);
					widgetWidth = max(p.mWidget->getWidth(), widgetWidth);
				}
								
				// render alternating backgrounds
				int cnt = 0;
				for(PropertyList::iterator it = mPropertyList.begin();
					it != mPropertyList.end();
					++it)
				{
					Property &p = (*it);

					gcn::Rectangle rect = p.mLabel->getDimension();
					rect.merge(p.mWidget->getDimension());
					rect.width = labelWidth + widgetWidth + getSpacing();

					p.mLabel->setBaseColor(colors[cnt%2]);
					p.mWidget->setBaseColor(colors[cnt%2]);
					graphics->setColor(colors[cnt%2]);

					graphics->fillRectangle(rect);
					cnt++;
				}
			}

			Container::draw(graphics);

			setOpaque(opaque);
		}

		void PropertySheet::adjustSize()
		{
			int width = 0, height = 0;
			for(PropertyList::iterator it = mPropertyList.begin();
				it != mPropertyList.end();
				++it)
			{
				Property &p = (*it);
				
				width = max(width, p.mWidget->getX() + p.mWidget->getWidth());
				height += max(p.mLabel->getHeight(), p.mWidget->getHeight());
			}
			setSize(width,height);
		}

		void PropertySheet::adjustContent()
		{
			adjustSize();

			int labelWidth = 0;
			int widgetWidth = 0;

			// find the biggest lablel/widget
			for(PropertyList::iterator it = mPropertyList.begin();
				it != mPropertyList.end();
				++it)
			{
				Property &p = (*it);
				labelWidth = max(p.mLabel->getWidth(), labelWidth);
				widgetWidth = max(p.mWidget->getWidth(), widgetWidth);
			}

			// adjust the position of everything.
			int x = 0, y = 0;
			for(PropertyList::iterator it = mPropertyList.begin();
				it != mPropertyList.end();
				++it)
			{
				Property &p = (*it);
				
				p.mLabel->setPosition(x, y);
				p.mLabel->setWidth(labelWidth);

				p.mWidget->setPosition(labelWidth + getSpacing(), y);
				p.mWidget->setWidth(widgetWidth);

				y += max(p.mLabel->getHeight(), p.mWidget->getHeight());
			}
		}

		void PropertySheet::add(Widget *widget)
		{
			addProperty(widget->getId(),widget);
		}

		void PropertySheet::add(Widget *widget, int x, int y)
		{
			x;y;
			addProperty(widget->getId(),widget);
		}

		void PropertySheet::remove(Widget *widget)
		{
			for(PropertyList::iterator it = mPropertyList.begin();
				it != mPropertyList.end();
				)
			{
				if((*it).mWidget == widget)
					it = mPropertyList.erase(it);
				else
					++it;
			}
			Container::remove(widget);
		}

		void PropertySheet::addProperty(const std::string &propname, gcn::Widget *widget)
		{
			Property prop;
			prop.mLabel = new gcn::Label(propname);
			prop.mLabel->setWidth(getFont()->getWidth(propname));
			prop.mWidget = widget;

			Container::add(prop.mLabel);
			Container::add(prop.mWidget);

			mPropertyList.push_back(prop);
		}

		void PropertySheet::clear(bool deletewidget)
		{
			mPropertyList.clear();
			Container::clear(deletewidget);
		}

		void PropertySheet::death(const Event& event)
		{
			for(PropertyList::iterator it = mPropertyList.begin();
				it != mPropertyList.end();
				)
			{
				if((*it).mWidget == event.getSource())
					it = mPropertyList.erase(it);
				else
					++it;
			}
			Container::death(event);
		}
	}
}
