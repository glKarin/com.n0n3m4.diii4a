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

#include <cctype>
#include "guichan/widgets/textbox.hpp"

#include "guichan/basiccontainer.hpp"
#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/key.hpp"
#include "guichan/mouseinput.hpp"

namespace gcn
{
    TextBox::TextBox()
    {
        mCaretColumn = 0;
        mCaretRow = 0;
        mEditable = true;
        mOpaque = true;

        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
        adjustSize();
        setText("");
    }

    TextBox::TextBox(const std::string& text)
    {
        mCaretColumn = 0;
        mCaretRow = 0;
        mEditable = true;
        mOpaque = true;

        setText(text);

        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
        adjustSize();
    }

    void TextBox::setText(const std::string& text)
    {
        mCaretColumn = 0;
        mCaretRow = 0;

        mTextRows.clear();

        std::string::size_type pos, lastPos = 0;
        int length;
        do
        {
            pos = text.find("\n", lastPos);

            if (pos != std::string::npos)
            {
                length = pos - lastPos;
            }
            else
            {
                length = text.size() - lastPos;
            }
            std::string sub = text.substr(lastPos, length);
            mTextRows.push_back(sub);
            lastPos = pos + 1;

        } while (pos != std::string::npos);

        adjustSize();
    }

    void TextBox::draw(Graphics* graphics)
    {
        /*
        int width = getWidth() + getBorderSize() * 2 - 1;
        int height = getHeight() + getBorderSize() * 2 - 1;

        graphics->setColor(getBackgroundColor());

        unsigned int i;
        for (i = 0; i < getBorderSize(); ++i)
        {
            graphics->drawLine(i,i, width - i, i);
            graphics->drawLine(i,i + 1, i, height - i - 1);
            graphics->drawLine(width - i,i + 1, width - i, height - i);
            graphics->drawLine(i,height - i, width - i - 1, height - i);
        }
        */

        unsigned int i;

        if (mOpaque)
        {
            graphics->setColor(getBackgroundColor());
            graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));
        }

        if (isFocused() && isEditable())
        {
            drawCaret(graphics, getFont()->getWidth(mTextRows[mCaretRow].substr(0, mCaretColumn)), mCaretRow * getFont()->getHeight());
        }

        graphics->setColor(getForegroundColor());
        graphics->setFont(getFont());

        for (i = 0; i < mTextRows.size(); i++)
        {
            // Move the text one pixel so we can have a caret before a letter.
            graphics->drawText(mTextRows[i], 1, i * getFont()->getHeight());
        }
    }

    void TextBox::drawCaret(Graphics* graphics, int x, int y)
    {
        graphics->setColor(getForegroundColor());
        graphics->drawLine(x, getFont()->getHeight() + y, x, y);
    }

    void TextBox::mousePressed(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getButton() == MouseEvent::LEFT)
        {
            mCaretRow = mouseEvent.getY() / getFont()->getHeight();

            if (mCaretRow >= (int)mTextRows.size())
            {
                mCaretRow = mTextRows.size() - 1;
            }

            mCaretColumn = getFont()->getStringIndexAt(mTextRows[mCaretRow], mouseEvent.getX());
        }
    }

    void TextBox::mouseDragged(MouseEvent& mouseEvent)
    {
        mouseEvent.consume();
    }

    void TextBox::keyPressed(KeyEvent& keyEvent)
    {
        Key key = keyEvent.getKey();

        if (key.getValue() == Key::LEFT)
        {
            --mCaretColumn;
            if (mCaretColumn < 0)
            {
                --mCaretRow;

                if (mCaretRow < 0)
                {
                    mCaretRow = 0;
                    mCaretColumn = 0;
                }
                else
                {
                    mCaretColumn = mTextRows[mCaretRow].size();
                }
            }
        }

        else if (key.getValue() == Key::RIGHT)
        {
            ++mCaretColumn;
            if (mCaretColumn > (int)mTextRows[mCaretRow].size())
            {
                ++mCaretRow;

                if (mCaretRow >= (int)mTextRows.size())
                {
                    mCaretRow = mTextRows.size() - 1;
                    if (mCaretRow < 0)
                    {
                        mCaretRow = 0;
                    }

                    mCaretColumn = mTextRows[mCaretRow].size();
                }
                else
                {
                    mCaretColumn = 0;
                }
            }
        }

        else if (key.getValue() == Key::DOWN)
        {
            setCaretRow(mCaretRow + 1);
        }

        else if (key.getValue() == Key::UP)
        {
            setCaretRow(mCaretRow - 1);
        }

        else if (key.getValue() == Key::HOME)
        {
            mCaretColumn = 0;
        }

        else if (key.getValue() == Key::END)
        {
            mCaretColumn = mTextRows[mCaretRow].size();
        }

        else if (key.getValue() == Key::ENTER && mEditable)
        {
            mTextRows.insert(mTextRows.begin() + mCaretRow + 1,
                             mTextRows[mCaretRow].substr(mCaretColumn, mTextRows[mCaretRow].size() - mCaretColumn));
            mTextRows[mCaretRow].resize(mCaretColumn);
            ++mCaretRow;
            mCaretColumn = 0;
        }

        else if (key.getValue() == Key::BACKSPACE
                 && mCaretColumn != 0
                 && mEditable)
        {
            mTextRows[mCaretRow].erase(mCaretColumn - 1, 1);
            --mCaretColumn;
        }

        else if (key.getValue() == Key::BACKSPACE
                 && mCaretColumn == 0
                 && mCaretRow != 0
                 && mEditable)
        {
            mCaretColumn = mTextRows[mCaretRow - 1].size();
            mTextRows[mCaretRow - 1] += mTextRows[mCaretRow];
            mTextRows.erase(mTextRows.begin() + mCaretRow);
            --mCaretRow;
        }

        else if (key.getValue() == Key::DELETE
                 && mCaretColumn < (int)mTextRows[mCaretRow].size()
                 && mEditable)
        {
            mTextRows[mCaretRow].erase(mCaretColumn, 1);
        }

        else if (key.getValue() == Key::DELETE
                 && mCaretColumn == (int)mTextRows[mCaretRow].size()
                 && mCaretRow < ((int)mTextRows.size() - 1)
                 && mEditable)
        {
            mTextRows[mCaretRow] += mTextRows[mCaretRow + 1];
            mTextRows.erase(mTextRows.begin() + mCaretRow + 1);
        }

        else if(key.getValue() == Key::PAGE_UP)
        {
            Widget* par = getParent();

            if (par != NULL)
            {
                int rowsPerPage = par->getChildrenArea().height / getFont()->getHeight();
                mCaretRow -= rowsPerPage;

                if (mCaretRow < 0)
                {
                    mCaretRow = 0;
                }
            }
        }

        else if(key.getValue() == Key::PAGE_DOWN)
        {
            Widget* par = getParent();

            if (par != NULL)
            {
                int rowsPerPage = par->getChildrenArea().height / getFont()->getHeight();
                mCaretRow += rowsPerPage;

                if (mCaretRow >= (int)mTextRows.size())
                {
                    mCaretRow = mTextRows.size() - 1;
                }
            }
        }

        else if(key.getValue() == Key::TAB
                && mEditable)
        {
            mTextRows[mCaretRow].insert(mCaretColumn,std::string("    "));
            mCaretColumn += 4;
        }

        else if (key.isCharacter()
                 && mEditable)
        {
            mTextRows[mCaretRow].insert(mCaretColumn,std::string(1,(char)key.getValue()));
            ++mCaretColumn;
        }

        adjustSize();
        scrollToCaret();

        keyEvent.consume();
    }

    void TextBox::adjustSize()
    {
        unsigned int i;
        int width = 0;
        for (i = 0; i < mTextRows.size(); ++i)
        {
            int w = getFont()->getWidth(mTextRows[i]);
            if (width < w)
            {
                width = w;
            }
        }

        setWidth(width + 1);
        setHeight(getFont()->getHeight() * mTextRows.size());
    }

    void TextBox::setCaretPosition(int position)
    {
        int row;

        for (row = 0; row < (int)mTextRows.size(); row++)
        {
            if (position <= (int)mTextRows[row].size())
            {
                mCaretRow = row;
                mCaretColumn = position;
                return; // we are done
            }
            else
            {
                position--;
            }
        }

        // position beyond end of text
        mCaretRow = mTextRows.size() - 1;
        mCaretColumn = mTextRows[mCaretRow].size();
    }

    int TextBox::getCaretPosition() const
    {
        int pos = 0, row;

        for (row = 0; row < mCaretRow; row++)
        {
            pos += mTextRows[row].size();
        }

        return pos + mCaretColumn;
    }

    void TextBox::setCaretRowColumn(int row, int column)
    {
        setCaretRow(row);
        setCaretColumn(column);
    }

    void TextBox::setCaretRow(int row)
    {
        mCaretRow = row;

        if (mCaretRow >= (int)mTextRows.size())
        {
            mCaretRow = mTextRows.size() - 1;
        }

        if (mCaretRow < 0)
        {
            mCaretRow = 0;
        }

        setCaretColumn(mCaretColumn);
    }

    unsigned int TextBox::getCaretRow() const
    {
        return mCaretRow;
    }

    void TextBox::setCaretColumn(int column)
    {
        mCaretColumn = column;

        if (mCaretColumn > (int)mTextRows[mCaretRow].size())
        {
            mCaretColumn = mTextRows[mCaretRow].size();
        }

        if (mCaretColumn < 0)
        {
            mCaretColumn = 0;
        }
    }

    unsigned int TextBox::getCaretColumn() const
    {
        return mCaretColumn;
    }

    const std::string& TextBox::getTextRow(int row) const
    {
        return mTextRows[row];
    }

    void TextBox::setTextRow(int row, const std::string& text)
    {
        mTextRows[row] = text;

        if (mCaretRow == row)
        {
            setCaretColumn(mCaretColumn);
        }

        adjustSize();
    }

    unsigned int TextBox::getNumberOfRows() const
    {
        return mTextRows.size();
    }

    std::string TextBox::getText() const
    {
        if (mTextRows.size() == 0)
        {
            return std::string("");
        }

        int i;
        std::string text;

        for (i = 0; i < (int)mTextRows.size() - 1; ++i)
        {
            text = text + mTextRows[i] + "\n";
        }

        text = text + mTextRows[i];

        return text;
    }

    void TextBox::fontChanged()
    {
        adjustSize();
    }

    void TextBox::scrollToCaret()
    {
        Rectangle scroll;
        scroll.x = getFont()->getWidth(mTextRows[mCaretRow].substr(0, mCaretColumn));
        scroll.y = getFont()->getHeight() * mCaretRow;
        scroll.width = getFont()->getWidth(" ");
        scroll.height = getFont()->getHeight() + 2; // add 2 for some extra space

        showPart(scroll);
    }

    void TextBox::setEditable(bool editable)
    {
        mEditable = editable;
    }

    bool TextBox::isEditable() const
    {
        return mEditable;
    }

    void TextBox::addRow(const std::string row)
    {
        mTextRows.push_back(row);
        adjustSize();
    }

    bool TextBox::isOpaque()
    {
        return mOpaque;
    }

    void TextBox::setOpaque(bool opaque)
    {
        mOpaque = opaque;
    }

	void TextBox::getWordPosAtCaret(int &start, int &end, std::string &word)
	{	
		const int iCaretPos = getCaretPosition();
		int i1 = iCaretPos-1, i2 = iCaretPos;

		std::string s = getText();

		// search backward for the first bad character
		bool done = false;
		while(!done && i1>0)
		{
			bool bValid = false;
			if(std::isalnum(s[i1]))
			{
				bValid = true;
				--i1;
				continue;
			}

			if(!bValid)
			{
				done = true;
				break;
			}			
		}

		word = s.substr(start,i2-i1);
		start = i1;
		end = i2;
	}

	void TextBox::replaceWordAtCaret(const std::string &with)
	{
		int i1 = 0, i2 = 0;
		std::string s = getText();
		std::string w;
		getWordPosAtCaret(i1,i2,w);
		if(i1 != i2)
		{
			s.replace(i1,i2-i1,with.c_str());
			setText(s);
			setCaretPosition(i1+with.length());
		}		
	}
}
