////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007 Brad Leffler (brad.leffler@gmail.com) and Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#ifndef SFML_WINDOWIMPLCARBON_HPP
#define SFML_WINDOWIMPLCARBON_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/Event.hpp>
#include <SFML/Window/WindowImpl.hpp>
#include <Carbon/Carbon.h>
#include <AGL/agl.h>
#include <set>
#include <string>


namespace sf
{
	namespace priv
{
	////////////////////////////////////////////////////////////
	/// WindowImplCarbon is the Carbon implementation of WindowImpl
	////////////////////////////////////////////////////////////
	class WindowImplCarbon : public WindowImpl
{
public :
	
    ////////////////////////////////////////////////////////////
    /// Default constructor
    /// (creates a dummy window to provide a valid OpenGL context)
    ///
    ////////////////////////////////////////////////////////////
    WindowImplCarbon();
	
    ////////////////////////////////////////////////////////////
    /// Construct the window implementation from an existing control
    ///
    /// \param Handle :            Platform-specific handle of the control
    /// \param AntialiasingLevel : Level of antialiasing
    ///
    ////////////////////////////////////////////////////////////
    WindowImplCarbon(WindowHandle Handle, int AntialiasingLevel);
	
    ////////////////////////////////////////////////////////////
    /// Create the window implementation
    ///
    /// \param Mode :              Video mode to use
    /// \param Title :             Title of the window
    /// \param WindowStyle :       Window style (resizable, fixed, or fullscren)
    /// \param AntialiasingLevel : Level of antialiasing
    ///
    ////////////////////////////////////////////////////////////
    WindowImplCarbon(VideoMode Mode, const std::string& Title, unsigned long WindowStyle, int AntialiasingLevel);
	
    ////////////////////////////////////////////////////////////
    /// Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~WindowImplCarbon();
	
private :
		
	////////////////////////////////////////////////////////////
	/// /see sfWindowImpl::Display
	///
	///////////////////////////////////////////////////////////
	virtual void Display();
	
    ////////////////////////////////////////////////////////////
    /// /see sfWindowImpl::ProcessEvents
    ///
    ////////////////////////////////////////////////////////////
    virtual void ProcessEvents();
	
    ////////////////////////////////////////////////////////////
    /// /see sfWindowImpl::MakeActive
    ///
    ////////////////////////////////////////////////////////////
    virtual void MakeActive(bool Active = true) const;
	
    ////////////////////////////////////////////////////////////
    /// /see sfWindowImpl::UseVerticalSync
    ///
    ////////////////////////////////////////////////////////////
    virtual void UseVerticalSync(bool Enabled);
	
    ////////////////////////////////////////////////////////////
    /// /see sfWindowImpl::ShowMouseCursor
    ///
    ////////////////////////////////////////////////////////////
    virtual void ShowMouseCursor(bool Show);
	
	////////////////////////////////////////////////////////////
    /// Change the position of the mouse cursor
    ///
    /// \param Left : Left coordinate of the cursor, relative to the window
    /// \param Top :  Top coordinate of the cursor, relative to the window
    ///
    ////////////////////////////////////////////////////////////
    virtual void SetCursorPosition(unsigned int Left, unsigned int Top);
	
    ////////////////////////////////////////////////////////////
    /// /see sfWindowImpl::SetPosition
    ///
    ////////////////////////////////////////////////////////////
    virtual void SetPosition(int Left, int Top);
	
	////////////////////////////////////////////////////////////
    /// See WindowImpl::Show
    ////////////////////////////////////////////////////////////
    virtual void Show(bool State);
	
	////////////////////////////////////////////////////////////
    /// See WindowImpl::EnableKeyRepeat
    ////////////////////////////////////////////////////////////
    virtual void EnableKeyRepeat(bool Enabled);
	
    ////////////////////////////////////////////////////////////
    /// Create the OpenGL rendering context
    ///
    /// \param Mode :              Video mode to use
    /// \param Fullscreen :        True to set fullscreen, false to stay in windowed mode
    /// \param AntialiasingLevel : Level of antialiasing
    ///
    ////////////////////////////////////////////////////////////
    void CreateContext(VideoMode Mode, bool Fullscreen, int AntialiasingLevel);
	
    ////////////////////////////////////////////////////////////
    /// Do some common initializations before the window can be created
    ///
    ////////////////////////////////////////////////////////////
    void PreInitialize();
	
    ////////////////////////////////////////////////////////////
    /// Run the event loop after the window has been created
    ///
    ////////////////////////////////////////////////////////////
    void PostInitialize();
	
    ////////////////////////////////////////////////////////////
    /// Free all the graphical resources attached to the window
    ///
    ////////////////////////////////////////////////////////////
    void Cleanup();
	
    ////////////////////////////////////////////////////////////
    /// Process a Carbon command event
    ///
    /// \param nextHandler : Next registered handler for this event type
    /// \param event       : The event
    /// \param userData    : Data passed in by the caller
    ///
    ////////////////////////////////////////////////////////////	
    OSStatus CommandEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData);
	
    ////////////////////////////////////////////////////////////
    /// Process a Carbon mouse event
    ///
    /// \param nextHandler : Next registered handler for this event type
    /// \param event       : The event
    /// \param userData    : Data passed in by the caller
    ///
    ////////////////////////////////////////////////////////////
	OSStatus MouseEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData);
	
    ////////////////////////////////////////////////////////////
    /// Process a Carbon keyboard event
    ///
    /// \param nextHandler : Next registered handler for this event type
    /// \param event       : The event
    /// \param userData    : Data passed in by the caller
    ///
    ////////////////////////////////////////////////////////////
	OSStatus KeyboardEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData);
	
    ////////////////////////////////////////////////////////////
    /// Process a Carbon window event
    ///
    /// \param nextHandler : Next registered handler for this event type
    /// \param event       : The event
    /// \param userData    : Data passed in by the caller
    ///
    ////////////////////////////////////////////////////////////
    OSStatus WindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData);
	
    ////////////////////////////////////////////////////////////
    /// Setup the joysticks
    ////////////////////////////////////////////////////////////
    void SetupJoysticks();
	
    ////////////////////////////////////////////////////////////
    /// Process the joystick state
    ////////////////////////////////////////////////////////////
    void ReadJoystickStates();
	
    ////////////////////////////////////////////////////////////
    /// Convert a Carbon key code to a SFML key code
    ///
    /// \param VirtualKey : Virtual key code to convert
    ///
    /// \return SFML key code corresponding to VirtualKey
    ///
    ////////////////////////////////////////////////////////////
	static Key::Code VirtualKeyCodeToSF(UInt32 VirtualKey);
	
    ////////////////////////////////////////////////////////////
    /// Function called whenever one of our windows receives a message
    ///
    /// \param Handler  : Carbon handle of the window
    /// \param Event    : Message received
    /// \param userData : First parameter of the message
    ///
    /// \return Something...
    ///
    ////////////////////////////////////////////////////////////
    static OSStatus GlobalOnEvent(EventHandlerCallRef nextHandler, EventRef event, void* userData);
	
    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    static unsigned int      ourWindowCount;      ///< Number of windows that we own
    static WindowImplCarbon* ourDummyWindow;      ///< Dummy window
	
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    WindowRef   myHandle;          ///< Carbon handle of the window
    AGLContext  myGLContext;       ///< OpenGL rendering context associated to the window
};

} // namespace priv

} // namespace sf

#endif // SFML_WINDOWIMPLCARBON_HPP
