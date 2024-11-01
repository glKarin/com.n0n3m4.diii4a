////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2008 Laurent Gomila (laurent.gom@gmail.com)
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

#ifndef SFML_WINDOWIMPLCOCOA_HPP
#define SFML_WINDOWIMPLCOCOA_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/Event.hpp>
#include <SFML/Window/WindowImpl.hpp>
#include <string>

// -- Ceylo --
// using __OBJC__ to prevent .cpp files from generating errors because of
// obj-C objects. It's easier than using obj-C++ for all C++ files
// using this header. WindowImplCocoa.mm compilation defines __OBJC__.
#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#import <SFML/Window/OSXCocoa/WindowController.h>
#endif

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
/// WindowImplCocoa is the Cocoa implementation of WindowImpl
////////////////////////////////////////////////////////////
class WindowImplCocoa : public WindowImpl
{
public :

    ////////////////////////////////////////////////////////////
    /// Default constructor
    /// (creates a dummy window to provide a valid OpenGL context)
    ///
    ////////////////////////////////////////////////////////////
    WindowImplCocoa();

    ////////////////////////////////////////////////////////////
    /// Construct the window implementation from an existing control
    ///
    /// \param Handle : Platform-specific handle of the control
    /// \param Params : Creation parameters
    ///
	/// Note: the NSWindow object must not be defered !
    ////////////////////////////////////////////////////////////
    WindowImplCocoa(WindowHandle Handle, WindowSettings& params);

    ////////////////////////////////////////////////////////////
    /// Create the window implementation
    ///
    /// \param Mode :           Video mode to use
    /// \param Title :          Title of the window
    /// \param WindowStyle :    Window style
    /// \param Params :			Creation parameters
    ///
    ////////////////////////////////////////////////////////////
    WindowImplCocoa(VideoMode Mode, const std::string& Title, unsigned long WindowStyle, WindowSettings& params);

    ////////////////////////////////////////////////////////////
    /// Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~WindowImplCocoa();

	////////////////////////////////////////////////////////////
	/// Handle event sent by the default NSNotificationCenter
	////////////////////////////////////////////////////////////
	void HandleNotifiedEvent(Event& event);
private :

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::Display
    ///
    ////////////////////////////////////////////////////////////
    virtual void Display();

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::ProcessEvents
    ///
    ////////////////////////////////////////////////////////////
    virtual void ProcessEvents();
	
    ////////////////////////////////////////////////////////////
    /// see WindowImpl::MakeActive
    ///
    ////////////////////////////////////////////////////////////
    virtual void MakeActive(bool Active = true) const;

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::UseVerticalSync
    ///
    ////////////////////////////////////////////////////////////
    virtual void UseVerticalSync(bool Enabled);

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::ShowMouseCursor
    ///
    ////////////////////////////////////////////////////////////
    virtual void ShowMouseCursor(bool Show);

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::SetCursorPosition
    ///
    ////////////////////////////////////////////////////////////
    virtual void SetCursorPosition(unsigned int Left, unsigned int Top);

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::SetPosition
    ///
    ////////////////////////////////////////////////////////////
    virtual void SetPosition(int Left, int Top);

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::Show
    ///
    ////////////////////////////////////////////////////////////
    virtual void Show(bool State);

    ////////////////////////////////////////////////////////////
    /// see WindowImpl::EnableKeyRepeat
    ///
    ////////////////////////////////////////////////////////////
    virtual void EnableKeyRepeat(bool Enabled);
	
	// -- Ceylo --
	// using __OBJC__ to prevent .cpp files from generating errors because of
	// obj-C objects. It's easier than using obj-C++ for all C++ files
	// using this header. WindowImplCocoa.mm compilation defines __OBJC__.
#if defined(__OBJC__)
	
	////////////////////////////////////////////////////////////
	/// Register our application and launch it if needed
	////////////////////////////////////////////////////////////
	void RunApplication(void);
	
	////////////////////////////////////////////////////////////
	/// Terminate the current running application
	////////////////////////////////////////////////////////////
	void QuitApplication(void);
	
	////////////////////////////////////////////////////////////
	/// Make the OpenGL pixel format from the given attributes
	////////////////////////////////////////////////////////////
	NSOpenGLPixelFormat *NewGLPixelFormat(WindowSettings& params, bool fullscreen);
	
	////////////////////////////////////////////////////////////
	/// Make the OpenGL context, the OpenGL view and add it to the window
	////////////////////////////////////////////////////////////
	void AddOpenGLView (WindowSettings& params);
	
	////////////////////////////////////////////////////////////
	/// Return the SFML key corresponding to a virtual key code
	////////////////////////////////////////////////////////////
	Key::Code KeyForVirtualCode(unsigned short VirtualKey);
	
	////////////////////////////////////////////////////////////
	/// Return the SFML key corresponding to a unicode code
	////////////////////////////////////////////////////////////
	Key::Code KeyForUnicode(unsigned short uniCode);
	
	////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
	NSAutoreleasePool *mainPool;
	WindowController *controller;
	NSWindow *windowHandle;
	NSOpenGLContext *glContext;
	NSOpenGLView *glView;
	NSPoint prevMousePos;
#endif // __OBJC__
	
	bool useKeyRepeat;
};

} // namespace priv

} // namespace sf

#endif // SFML_WINDOWIMPLCOCOA_HPP
