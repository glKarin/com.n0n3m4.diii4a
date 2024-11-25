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

#ifndef GCN_PROPERTYSHEET_HPP
#define GCN_PROPERTYSHEET_HPP

#include <string>

#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/widgets/container.hpp"

namespace gcn
{
	class Label;

	namespace contrib
	{
		/**
		* An implementation of a movable window that can contain other widgets.
		*/
		class GCN_CORE_DECLSPEC PropertySheet : public gcn::Container
		{
		public:
			/**
			* Constructor.
			*/
			PropertySheet();

			/**
			* Constructor. The window will be automatically resized in height
			* to fit the caption.
			*
			* @param caption the caption of the window.
			*/
			PropertySheet(const std::string& caption);

			/**
			* Destructor.
			*/
			virtual ~PropertySheet();

			/**
			* Rearrange the widgets and resize the container.
			*/
			virtual void adjustContent();

			void addProperty(const std::string &propname, gcn::Widget *widget);

			int getSpacing() const { return mSpacing; }
			void setSpacing(int sp) { mSpacing = sp; }


			// Inherited from Container

			virtual void draw(Graphics* graphics);

			virtual void logic();

			virtual void add(Widget *widget);

			virtual void add(Widget *widget, int x, int y);

			virtual void remove(Widget *widget);

			virtual void clear(bool deletewidget = false);

			// Inherited from DeathListener

			virtual void death(const Event& event);
		protected:
			struct Property
			{
				gcn::Label	*mLabel;
				gcn::Widget *mWidget;
			};

			 typedef std::list<Property> PropertyList;

			 PropertyList	mPropertyList;

			 virtual void adjustSize();

			 int			mSpacing;

			 bool			mColorRows;
		};
	}
}

#endif // end GCN_WINDOW_HPP
