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

#ifndef GCN_DROPDOWN_HPP
#define GCN_DROPDOWN_HPP

#include "guichan/actionlistener.hpp"
#include "guichan/basiccontainer.hpp"
#include "guichan/deathlistener.hpp"
#include "guichan/focushandler.hpp"
#include "guichan/focuslistener.hpp"
#include "guichan/keylistener.hpp"
#include "guichan/listmodel.hpp"
#include "guichan/mouselistener.hpp"
#include "guichan/platform.hpp"
#include "guichan/selectionlistener.hpp"
#include "guichan/widgets/listbox.hpp"
#include "guichan/widgets/scrollarea.hpp"

namespace gcn
{
    /**
     * An implementation of a drop downable list from which an item can be selected.
     * The drop down consists of an internal ScrollArea and an internal ListBox. 
     * The drop down also uses an internal FocusHandler to handle the focus of the 
     * internal ScollArea and the internal ListBox. The scroll area and the list box
     * can be passed to the drop down if a custom scroll area and or a custom list box
     * is preferable.
     *
     * To be able display a list the drop down uses a user provided list model. 
     * A list model can be any class that implements the ListModel interface. 
     *
     * If an item is selected in the drop down a select event will be sent to all selection 
     * listeners of the drop down. If an item is selected by using a mouse click or by using 
     * the enter or space key an action event will be sent to all action listeners of the 
     * drop down.
     */
    class GCN_CORE_DECLSPEC DropDown :
        public BasicContainer,
		public ActionListener,
        public KeyListener,
        public MouseListener,
        public FocusListener,
        public SelectionListener
    {
    public:
        /**
         * Contructor.
         *
         * @param listModel the ListModel to use.
         * @param scrollArea the ScrollArea to use.
         * @param listBox the listBox to use.
         * @see ListModel, ScrollArea, ListBox.
         */
        DropDown(ListModel *listModel = NULL,
                 ScrollArea *scrollArea = NULL,
                 ListBox *listBox = NULL);

        /**
         * Destructor.
         */
        virtual ~DropDown();

        /**
         * Gets the selected item as an index in the list model.
         *
         * @return the selected item as an index in the list model.
         * @see setSelected
         */
        int getSelected() const;

        /**
         * Sets the selected item. The selected item is represented by
         * an index from the list model.
         *
         * @param selected the selected item as an index from the list model.
         * @see getSelected
         */
        void setSelected(int selected);

        /**
         * Sets the list model to use when displaying the list.
         *
         * @param listModel the list model to use.
         * @see getListModel
         */
        void setListModel(ListModel *listModel);

        /**
         * Gets the list model used.
         *
         * @return the ListModel used.
         * @see setListModel
         */
        ListModel *getListModel();

		/**
		* Gets the list box used.
		*
		* @return the ListBox used.
		*/
		ListBox *getListBox();

		/**
		* Gets the scroll area used.
		*
		* @return the ScrollArea used.
		*/
		ScrollArea *getScrollArea();

        /**
         * Adjusts the height of the drop down to fit the height of the
         * drop down's parent's height. It's used to not make the drop down
         * draw itself outside of it's parent if folded down.
         */
        void adjustHeight();

        /**
         * Adds a selection listener to the drop down. When the selection
         * changes an event will be sent to all selection listeners of the
         * drop down.
         *
         * @param selectionListener the selection listener to add.
         * @since 0.8.0
         */
        void addSelectionListener(SelectionListener* selectionListener);

        /**
         * Removes a selection listener from the drop down.
         *
         * @param selectionListener the selection listener to remove.
         * @since 0.8.0
         */
        void removeSelectionListener(SelectionListener* selectionListener);


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        void setBaseColor(const Color& color);

        void setBackgroundColor(const Color& color);

        void setForegroundColor(const Color& color);

		void setFont(Font *font);

        void setSelectionColor(const Color& color);


        // Inherited from BasicContainer

        virtual Rectangle getChildrenArea();


        // Inherited from FocusListener

        virtual void focusLost(const Event& event);


        // Inherited from ActionListener

        virtual void action(const ActionEvent& actionEvent);


        // Inherited from DeathListener

        virtual void death(const Event& event);


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseReleased(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);


        // Inherited from SelectionListener

        virtual void valueChanged(const SelectionEvent& event);

    protected:
        /**
         * Draws the button of the drop down.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawButton(Graphics *graphics);

        /**
         * Sets the drop down to be dropped down.
         */
        virtual void dropDown();

        /**
         * Sets the drop down to be folded up.
         */
        virtual void foldUp();

        /**
         * Distributes a value changed event to all selection listeners
         * of the drop down.
         * 
         * @since 0.8.0
         */
        void distributeValueChangedEvent();

        /**
         * True if the drop down is dropped down, false otherwise.
         */
        bool mDroppedDown;

        /**
         * True if the drop down has been pushed with the mouse, false
         * otherwise.
         */ 
        bool mPushed;

        /**
         * Holds what the height is if the drop down is folded up. Used when
         * checking if the list of the drop down was clicked or if the upper part
         * of the drop down was clicked on a mouse click 
         */
        int mFoldedUpHeight;

        /**
         * The scroll area used.
         */
        ScrollArea* mScrollArea;

        /**
         * The list box used.
         */
        ListBox* mListBox;

        /**
         * The internal focus handler used to keep track of focus for the
         * internal list box.
         */
        FocusHandler mInternalFocusHandler;

        /**
         * True if an internal scroll area is used, false if a scroll area
         * has been passed to the drop down which the drop down should not
         * deleted in it's destructor.
         */
        bool mInternalScrollArea;

        /**
         * True if an internal list box is used, false if a list box
         * has been passed to the drop down which the drop down should not
         * deleted in it's destructor.
         */
        bool mInternalListBox;

        /**
         * True if the drop down is dragged.
         */
        bool mIsDragged;

        /**
         * Typedef.
         */
        typedef std::list<SelectionListener*> SelectionListenerList;

        /**
         * The selection listener's of the drop down.
         */
        SelectionListenerList mSelectionListeners;

        /**
         * Typedef.
         */
        typedef SelectionListenerList::iterator SelectionListenerIterator;
    };
}

#endif // end GCN_DROPDOWN_HPP
