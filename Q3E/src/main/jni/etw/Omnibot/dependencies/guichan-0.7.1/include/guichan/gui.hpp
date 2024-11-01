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

#ifndef GCN_GUI_HPP
#define GCN_GUI_HPP

#include <list>
#include <deque>

#include "guichan/keyevent.hpp"
#include "guichan/mouseevent.hpp"
#include "guichan/mouseinput.hpp"
#include "guichan/platform.hpp"

namespace gcn
{
    class FocusHandler;
    class Graphics;
    class Input;
    class KeyListener;
    class Widget;

    // The following comment will appear in the doxygen main page.
    /**
     * @mainpage
     * @section Introduction
     * This documentation is mostly intended as a reference to the API. If you want to get started with Guichan, we suggest you check out the programs in the examples directory of the Guichan release.
     * @n
     * @n
     * This documentation is, and will always be, work in progress. If you find any errors, typos or inconsistencies, or if you feel something needs to be explained in more detail - don't hesitate to tell us.
     */

    /**
     * Contains a Guichan GUI. This is the core class of Guichan to which
     * implementations of back ends are passed, to make Guichan work with
     * a specific library, and to where the top widget (root widget of GUI)
     * is added. If you want to be able to have more then one widget in your 
     * GUI, the top widget should be a container.
     *
     * A Gui object cannot work properly without passing back end 
     * implementations to it. A Gui object must have an implementation of a
     * Graphics and an implementation of Input. 
     *
     * NOTE: A complete GUI also must have the ability to load images.
     *       Images are loaded with the Image class, so to make Guichan
     *       able to load images an implementation of ImageLoader must be
     *       passed to Image.
     *
     * @see Graphics, Input, Image
     */
    class GCN_CORE_DECLSPEC Gui
    {
    public:

        /**
         * Constructor.
         */
        Gui();

        /**
         * Destructor.
         */
        virtual ~Gui();

        /**
         * Sets the top widget. The top widget is the root widget
         * of the GUI. If you want a GUI to be able to contain more
         * than one widget the top widget should be a container.
         *
         * @param top The top widget.
         * @see Container
         * @since 0.1.0
         */
        virtual void setTop(Widget* top);

        /**
         * Gets the top widget. The top widget is the root widget
         * of the GUI.
         *
         * @return The top widget. NULL if no top widget has been set.
         * @since 0.1.0
         */
        virtual Widget* getTop() const;

        /**
         * Sets the graphics object to use for drawing.
         *
         * @param graphics The graphics object to use for drawing.
         * @see getGraphics, AllegroGraphics, HGEGraphics, 
         *      OpenLayerGraphics, OpenGLGraphics, SDLGraphics
         * @since 0.1.0
         */
        virtual void setGraphics(Graphics* graphics);

        /**
         * Gets the graphics object used for drawing.
         *
         *  @return The graphics object used for drawing. NULL if no
         *          graphics object has been set.
         * @see setGraphics, AllegroGraphics, HGEGraphics, 
         *      OpenLayerGraphics, OpenGLGraphics, SDLGraphics
         * @since 0.1.0
         */
        virtual Graphics* getGraphics() const;

        /**
         * Sets the input object to use for input handling.
         *
         * @param input The input object to use for input handling.
         * @see getInput, AllegroInput, HGEInput, OpenLayerInput,
         *      SDLInput
         * @since 0.1.0
         */
        virtual void setInput(Input* input);

        /**
         * Gets the input object being used for input handling.
         *
         *  @return The input object used for handling input. NULL if no
         *          input object has been set.
         * @see setInput, AllegroInput, HGEInput, OpenLayerInput,
         *      SDLInput
         * @since 0.1.0
         */
        virtual Input* getInput() const;

        /**
         * Performs logic of the GUI. By calling this function all logic
         * functions down in the GUI heirarchy will be called. When logic
         * is called for Gui, user input will be handled.
         *
         * @see Widget::logic
         * @since 0.1.0
         */
        virtual void logic();

        /**
         * Draws the GUI. By calling this funcion all draw functions
         * down in the GUI hierarchy will be called. When draw is called
         * the used Graphics object will be initialised and drawing of
         * the top widget will commence.
         *
         * @see Widget::draw
         * @since 0.1.0
         */
        virtual void draw();

        /**
         * Focuses none of the widgets in the Gui.
         *
         * @since 0.1.0
         */
        virtual void focusNone();

        /**
         * Sets tabbing enabled, or not. Tabbing is the usage of
         * changing focus by utilising the tab key.
         *
         * @param tabbing True if tabbing should be enabled, false
         *                otherwise.
         * @see isTabbingEnabled
         * @since 0.1.0
         */
        virtual void setTabbingEnabled(bool tabbing);

        /**
         * Checks if tabbing is enabled.
         *
         * @return True if tabbing is enabled, false otherwise.
         * @see setTabbingEnabled
         * @since 0.1.0
         */
        virtual bool isTabbingEnabled();

        /**
         * Adds a global key listener to the Gui. A global key listener
         * will receive all key events generated from the GUI and global
         * key listeners will receive the events before key listeners
         * of widgets.
         *
         * @param keyListener The key listener to add.
         * @see removeGlobalKeyListener
         * @since 0.5.0
         */
        virtual void addGlobalKeyListener(KeyListener* keyListener);

        /**
         * Removes global key listener from the Gui.
         *
         * @param keyListener The key listener to remove.
         * @throws Exception if the key listener hasn't been added.
         * @see addGlobalKeyListener
         * @since 0.5.0
         */
        virtual void removeGlobalKeyListener(KeyListener* keyListener);

    protected:
        /**
         * Handles all mouse input.
         *
         * @since 0.6.0
         */
        virtual void handleMouseInput();

        /**
         * Handles key input.
         *
         * @since 0.6.0
         */
        virtual void handleKeyInput();

        /**
         * Handles mouse moved input.
         *
         * @param mouseInput The mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseMoved(const MouseInput& mouseInput);

        /**
         * Handles mouse pressed input.
         *
         * @param mouseInput The mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMousePressed(const MouseInput& mouseInput);

        /**
         *
         * Handles mouse wheel moved down input.
         *
         * @param mouseInput The mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseWheelMovedDown(const MouseInput& mouseInput);

        /**
         * Handles mouse wheel moved up input.
         *
         * @param mouseInput The mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseWheelMovedUp(const MouseInput& mouseInput);

        /**
         * Handles mouse released input.
         *
         * @param mouseInput The mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseReleased(const MouseInput& mouseInput);

        /**
         * Handles modal focus. Modal focus needs to be checked at 
         * each logic iteration as it might be necessary to distribute
         * mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalFocus();

        /**
         * Handles modal mouse input focus. Modal mouse input focus needs 
         * to be checked at each logic iteration as it might be necessary to 
         * distribute mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalMouseInputFocus();

        /**
         * Handles modal focus gained. If modal focus has been gaind it might 
         * be necessary to distribute mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalFocusGained();

        /**
         * Handles modal mouse input focus gained. If modal focus has been gaind 
         * it might be necessary to distribute mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalFocusReleased();

        /**
         * Distributes a mouse event.
         *
         * @param type The type of the event to distribute,
         * @param button The button of the event (if any used) to distribute.
         * @param x The x coordinate of the event.
         * @param y The y coordinate of the event.
         * @param fource indicates whether the distribution should be forced or not.
         *               A forced distribution distributes the event even if a widget
         *               is not enabled, not visible, another widget has modal
         *               focus or another widget has modal mouse input focus. 
         *               Default value is false.
         * @param toSourceOnly indicates whether the distribution should be to the
         *                     source widget only or to it's parent's mouse listeners
         *                     as well.
         *
         * @since 0.6.0
         */
        virtual void distributeMouseEvent(Widget* source,
                                          int type,
                                          int button,
                                          int x,
                                          int y,
                                          bool force = false,
                                          bool toSourceOnly = false);

        /**
         * Distributes a key event.
         *
         * @param keyEvent The key event to distribute.

         * @since 0.6.0
         */
        virtual void distributeKeyEvent(KeyEvent& keyEvent);

        /**
         * Distributes a key event to the global key listeners.
         *
         * @param keyEvent The key event to distribute.
         *
         * @since 0.6.0
         */
        virtual void distributeKeyEventToGlobalKeyListeners(KeyEvent& keyEvent);

        /**
         * Gets the widget at a certain position.
         *
         * @return The widget at a certain position.
         * @since 0.6.0
         */
        virtual Widget* getWidgetAt(int x, int y);

        /**
         * Gets the source of the mouse event.
         *
         * @return The source widget of the mouse event.
         * @since 0.6.0
         */
        virtual Widget* getMouseEventSource(int x, int y);

        /**
         * Gets the source of the key event.
         *
         * @return The source widget of the key event.
         * @since 0.6.0
         */
        virtual Widget* getKeyEventSource();

        /**
         * Holds the top widget.
         */
        Widget* mTop;

        /**
         * Holds the graphics implementation used.
         */
        Graphics* mGraphics;

        /**
         * Holds the input implementation used.
         */
        Input* mInput;

        /**
         * Holds the focus handler for the Gui.
         */
        FocusHandler* mFocusHandler;

        /**
         * True if tabbing is enabled, false otherwise.
         */
        bool mTabbing;

        /**
         * Typedef.
         */
        typedef std::list<KeyListener*> KeyListenerList;

        /**
         * Typedef.
         */
        typedef KeyListenerList::iterator KeyListenerListIterator;

        /**
         * Holds the global key listeners of the Gui.
         */
        KeyListenerList mKeyListeners;
        
        /**
         * True if shift is pressed, false otherwise.
         */
        bool mShiftPressed;

        /**
         * True if meta is pressed, false otherwise.
         */
        bool mMetaPressed;

        /**
         * True if control is pressed, false otherwise.
         */
        bool mControlPressed;

        /**
         * True if alt is pressed, false otherwise.
         */
        bool mAltPressed;

        /**
         * Holds the last mouse button pressed.
         */
        unsigned int mLastMousePressButton;

        /**
         * Holds the last mouse press time stamp.
         */
        int mLastMousePressTimeStamp;

        /**
         * Holds the last mouse x coordinate.
         */
        int mLastMouseX;

        /**
         * Holds the last mouse y coordinate.
         */
        int mLastMouseY;

        /**
         * Holds the current click count. Used to keep track
         * of clicks for a the last pressed button.
         */
        int mClickCount;

        /**
         * Holds the last button used when a drag of a widget
         * was initiated. Used to be able to release a drag
         * when the same button is released.
         */
        int mLastMouseDragButton;

        /**
         * Holds a stack with all the widgets with the mouse.
         * Used to properly distribute mouse events.
         */
        std::deque<Widget*> mWidgetWithMouseQueue;
    };
}

#endif // end GCN_GUI_HPP

/* yakslem  - "Women, it's a constant struggle."
 * finalman - "Yes, but sometimes they succeed with their guesses."
 * yaklsem  - "...eh...I was talking about love."
 * finalman - "Oh...ok..."
 * An awkward silence followed.
 */
