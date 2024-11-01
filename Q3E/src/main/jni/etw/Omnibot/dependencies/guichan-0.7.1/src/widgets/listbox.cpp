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

#include "guichan/widgets/listbox.hpp"

#include "guichan/basiccontainer.hpp"
#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/key.hpp"
#include "guichan/listmodel.hpp"
#include "guichan/mouseinput.hpp"
#include "guichan/selectionlistener.hpp"

namespace gcn
{
    ListBox::ListBox(ListModel *listModel)
        : mSelected(-1)
        , mListModel(NULL)
        , mWrappingEnabled(false)
        , mTitleBarColor(Color(50,50,50))
    {
        setWidth(100);

        setListModel(listModel);
        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
    }
	
    void ListBox::draw(Graphics* graphics)
    {
        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));

        if (mListModel == NULL)
        {
            return;
        }

        graphics->setColor(getForegroundColor());
        graphics->setFont(getFont());

        // Check the current clip area so we don't draw unnecessary items
        // that are not visible.
        const ClipRectangle currentClipArea = graphics->getCurrentClipArea();
		const int rowHeight = getRowHeight();
        
		std::string columnTitle;
		int columnWidth = 0;
		const int columnBorder = mListModel->getColumnBorderSize();
		const int numColumns = mListModel->getNumberOfColumns();

		// Calculate the number of rows to draw by checking the clip area.
		// The addition of two makes covers a partial visible row at the top
		// and a partial visible row at the bottom.
		
		int numberOfRows = currentClipArea.height / rowHeight + 2;

        if (numberOfRows > mListModel->getNumberOfElements())
        {
            numberOfRows = mListModel->getNumberOfElements();
        }

		// Calculate which row to start drawing. If the list box 
		// has a negative y coordinate value we should check if
		// we should drop rows in the begining of the list as
		// they might not be visible. A negative y value is very
		// common if the list box for instance resides in a scroll
		// area and the user has scrolled the list box downwards.
		int startRow;    	
		if (getY() < 0)
		{
			startRow = -1 * (getY() / rowHeight);
		}
		else
		{
			startRow = 0;
		}

		// The y coordinate where we start to draw the text is
		// simply the y coordinate multiplied with the font height.
		bool showColumn = false;
		int y = rowHeight * startRow;
        for (int i = startRow; i < /*startRow +*/ numberOfRows /*- showColumn*/; ++i)
        {			
			int columnOffset = 1;
			for(int c = 0; c < numColumns; ++c)
			{
				// Column titles
				showColumn |= mListModel->getColumnTitle(c,columnTitle,columnWidth);
				if(showColumn && i == startRow)
				{
					/*if(mAutoAdjustColumnWidth)
					{
						int txtWidth = getFont()->getWidth(mColumnTitles[c]);
						if(txtWidth > mColumnWidths[c])
							mColumnWidths[c] = txtWidth;
					}*/
					if(c == 0)
					{
						graphics->setColor(getTitleBarColor());
						graphics->fillRectangle(Rectangle(0, y, getWidth(), rowHeight));
						graphics->setColor(getForegroundColor());
					}

					graphics->drawText(columnTitle, columnOffset, y);

					y += rowHeight;
				}

				// Set the selection color
				if (i == mSelected)
				{
					graphics->setColor(getSelectionColor());
					graphics->fillRectangle(Rectangle(0, y, getWidth(), rowHeight));
					graphics->setColor(getForegroundColor());
				}

				// If the row height is greater than the font height we
				// draw the text with a center vertical alignment.
				if (rowHeight > getFont()->getHeight())
				{
					graphics->drawText(
						mListModel->getElementAt(i,c), 
						columnOffset, 
						y + rowHeight / 2 - getFont()->getHeight() / 2);
				}
				else
				{
					graphics->drawText(mListModel->getElementAt(i,c), columnOffset, y);
				}

				columnOffset += columnWidth + columnBorder;

				if(showColumn && i == startRow && c < numColumns-1)
					y -= rowHeight;
			}
			y += rowHeight;
        }
    }

	void ListBox::drawBorder(Graphics* graphics)
	{
		Color faceColor = getBaseColor();
		Color highlightColor, shadowColor;
		int alpha = getBaseColor().a;
		int width = getWidth() + getFrameSize() * 2 - 1;
		int height = getHeight() + getFrameSize() * 2 - 1;
		highlightColor = faceColor + 0x303030;
		highlightColor.a = alpha;
		shadowColor = faceColor - 0x303030;
		shadowColor.a = alpha;

		for (int i = 0; i < getFrameSize(); ++i)
		{
			graphics->setColor(shadowColor);
			graphics->drawLine(i,i, width - i, i);
			graphics->drawLine(i,i + 1, i, height - i - 1);
			graphics->setColor(highlightColor);
			graphics->drawLine(width - i,i + 1, width - i, height - i);
			graphics->drawLine(i,height - i, width - i - 1, height - i);
		}
	}

    void ListBox::logic()
    {
        adjustSize();
    }

	int ListBox::getSelected() const
    {
        return mSelected;
    }

	std::string ListBox::getSelectedText() const
	{
		std::string s;

		if(getSelected()==-1)
			return s;

		if(mListModel)
		{
			if(getSelected() >= mListModel->getNumberOfElements())
				return s;

			s = mListModel->getElementAt(getSelected(),0);
		}
		return s;
	}

    void ListBox::setSelected(int selected)
    {
        if (mListModel == NULL)
        {
            mSelected = -1;
        }
        else
        {
            if (selected < 0)
            {
                mSelected = -1;
            }
            else if (selected >= mListModel->getNumberOfElements())
            {
                mSelected = mListModel->getNumberOfElements() - 1;
            }
            else
            {
                mSelected = selected;
            }

            Rectangle scroll;

            if (mSelected < 0)
            {
                scroll.y = 0;
            }
            else
            {
                scroll.y = getRowHeight() * mSelected;
            }

            scroll.height = getRowHeight();
            showPart(scroll);
        }

        distributeValueChangedEvent();
    }

	bool ListBox::setSelectedText(const std::string &sel)
	{
		if(mListModel)
		{
			std::string s;
			int iNumElements = mListModel->getNumberOfElements();
			for(int i = 0; i < iNumElements; ++i)
			{
				s = mListModel->getElementAt(i,0);
				if(s==sel)
				{
					setSelected(i);
					return true;
				}
			}
		}
		return false;
	}

    void ListBox::keyPressed(KeyEvent& keyEvent)
    {
        Key key = keyEvent.getKey();

        if (key.getValue() == Key::ENTER || key.getValue() == Key::SPACE)
        {
            distributeActionEvent();
            keyEvent.consume();
        }
        else if (key.getValue() == Key::UP)
        {
            setSelected(mSelected - 1);

            if (mSelected == -1)
            {
                if (mWrappingEnabled)
                {
                    setSelected(getListModel()->getNumberOfElements() - 1);
                }
                else
                {
                    setSelected(0);
                }
            }
            
            keyEvent.consume();
        }
        else if (key.getValue() == Key::DOWN)
        {
            if (mWrappingEnabled
                && getSelected() == getListModel()->getNumberOfElements() - 1)
            {
                setSelected(0);
            }
            else
            {
                setSelected(getSelected() + 1);
            }
            
            keyEvent.consume();
        }
        else if (key.getValue() == Key::HOME)
        {
            setSelected(0);
            keyEvent.consume();
        }
        else if (key.getValue() == Key::END)
        {
            setSelected(getListModel()->getNumberOfElements() - 1);
            keyEvent.consume();
        }
    }

    void ListBox::mousePressed(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getButton() == MouseEvent::LEFT)
        {
			int selected = mouseEvent.getY() / getFont()->getHeight();

			int columnWidth = 0;
			std::string title;
			if(mListModel && mListModel->getColumnTitle(0,title,columnWidth))
			{
				if(selected==0)
					return;
				selected--;
			}
            setSelected(selected);
            distributeActionEvent();
        }
    }

    void ListBox::mouseWheelMovedUp(MouseEvent& mouseEvent)
    {
        if (isFocused())
        {
            if (getSelected() > 0 )
            {
                setSelected(getSelected() - 1);
            }

            mouseEvent.consume();
        }
    }

    void ListBox::mouseWheelMovedDown(MouseEvent& mouseEvent)
    {
        if (isFocused())
        {
            setSelected(getSelected() + 1);

            mouseEvent.consume();
        }
    }

    void ListBox::mouseDragged(MouseEvent& mouseEvent)
    {
        mouseEvent.consume();
    }

    void ListBox::setListModel(ListModel *listModel)
    {
        mSelected = -1;
        mListModel = listModel;
        adjustSize();
    }

    ListModel* ListBox::getListModel()
    {
        return mListModel;
    }

    void ListBox::adjustSize()
    {
        if (mListModel != NULL)
        {
			int height = getRowHeight() * mListModel->getNumberOfElements();

			// account for the column titles for the width
			std::string title;
			bool hasTitles = false;
			int width = 0;
			for(int c = 0; c < mListModel->getNumberOfColumns(); ++c)
			{
				int colWidth = 0;
				mListModel->getColumnTitle(c,title,colWidth);
				width += colWidth;
				hasTitles = true;
				break;
			}
			
			if(hasTitles)
			{
				height += getFont()->getHeight();				
			}
			setHeight(height);
			
			
			/*for(int i = 0; i < mListModel->getNumberOfElements(); ++i)
			{
				int rowWidth = 0;
				for(int c = 0; c < mListModel->getNumberOfColumns(); ++c)
				{
					rowWidth += getFont()->getWidth(mListModel->getElementAt(i,c));
				}

				if(rowWidth > width)
					width = rowWidth;
			}
			setWidth(width);*/
        }
    }

    bool ListBox::isWrappingEnabled() const
    {
        return mWrappingEnabled;
    }

    void ListBox::setWrappingEnabled(bool wrappingEnabled)
    {
        mWrappingEnabled = wrappingEnabled;
    }
        
    void ListBox::addSelectionListener(SelectionListener* selectionListener)
    {
        mSelectionListeners.push_back(selectionListener);
    }
   
    void ListBox::removeSelectionListener(SelectionListener* selectionListener)
    {
        mSelectionListeners.remove(selectionListener);
    }

    void ListBox::distributeValueChangedEvent()
    {
        SelectionListenerIterator iter;

        for (iter = mSelectionListeners.begin(); iter != mSelectionListeners.end(); ++iter)
        {
            SelectionEvent event(this);
            (*iter)->valueChanged(event);
        }
    }

	int ListBox::getRowHeight() const
	{
		return getFont()->getHeight();
	}
}
