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

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/OSXCarbon/WindowImplCarbon.hpp>
// #include <SFML/Window/OSXCarbon/JoystickSupport.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>
#include <vector>


namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
// Static member data
////////////////////////////////////////////////////////////
unsigned int      WindowImplCarbon::ourWindowCount      = 0;
WindowImplCarbon* WindowImplCarbon::ourDummyWindow      = NULL;

#if SFML_JOYSTICK_SUPPORT
JoystickSupport joySupport;
std::vector<JoystickDevice> joysticks;
#endif // SFML_JOYSTICK_SUPPORT

WindowAttributes  windowAttrib = kWindowStandardDocumentAttributes |
                                 kWindowStandardHandlerAttribute;

EventTypeSpec eventSpec[] = { { kEventClassCommand, kEventCommandProcess },
                              { kEventClassMouse, kEventMouseDown },
                              { kEventClassMouse, kEventMouseUp }, 
                              { kEventClassMouse, kEventMouseDragged },
                              { kEventClassMouse, kEventMouseWheelMoved },
                              { kEventClassWindow, kEventWindowCollapsing },
                              { kEventClassWindow, kEventWindowActivated },
                              { kEventClassWindow, kEventWindowClose },
                              { kEventClassWindow, kEventWindowDrawContent },
                              { kEventClassWindow, kEventWindowBoundsChanged },
                              { kEventClassWindow, kEventWindowBoundsChanging },
                              { kEventClassKeyboard, kEventRawKeyDown },
                              { kEventClassKeyboard, kEventRawKeyUp },
/*                              { kEventClassKeyboard, kEventRawKeyModifiersChanged }*/ };

////////////////////////////////////////////////////////////
/// Default constructor
/// (creates a dummy window to provide a valid OpenGL context)
////////////////////////////////////////////////////////////
WindowImplCarbon::WindowImplCarbon() :
myHandle  (NULL),
myGLContext (0)
{
    // Use small dimensions
    myWidth  = 1;
    myHeight = 1;
	
    Rect viewRect = { 0, 0, myHeight, myWidth };

    // Create a dummy window (disabled and hidden)
    CreateNewWindow(kDocumentWindowClass, windowAttrib, &viewRect, &myHandle);
    HideWindow(myHandle);

    // Create the rendering context
    if (myHandle)
        CreateContext(VideoMode(myWidth, myHeight, 32), false, 0);

    // Save as the dummy window
    if (!ourDummyWindow)
        ourDummyWindow = this;
}


////////////////////////////////////////////////////////////
/// Create the window implementation from an existing control
////////////////////////////////////////////////////////////
WindowImplCarbon::WindowImplCarbon(WindowHandle Handle, int AntialiasingLevel) :
myGLContext (0)
{
	// Save window handle
    myHandle = static_cast <WindowRef> (Handle);

    if (myHandle)
    {
        // Get window client size
		Rect viewRect;
		GetWindowBounds(myHandle, kWindowContentRgn, &viewRect);
        myWidth  = viewRect.right - viewRect.left;
        myHeight = viewRect.bottom - viewRect.top;

        // Create the rendering context
        VideoMode Mode = VideoMode::GetDesktopMode();
        Mode.Width  = myWidth;
        Mode.Height = myHeight;
        CreateContext(Mode, false, AntialiasingLevel);

#if SFML_JOYSTICK_SUPPORT
        // Setup joysticks
        SetupJoysticks();
#endif // SFML_JOYSTICK_SUPPORT
    }
}

////////////////////////////////////////////////////////////
/// Create the window implementation
////////////////////////////////////////////////////////////
WindowImplCarbon::WindowImplCarbon(VideoMode Mode, const std::string& Title, unsigned long WindowStyle, int AntialiasingLevel) :
myHandle  (NULL),
myGLContext (0)
{
    PreInitialize();

    // Compute position and size
    Rect viewRect;
    int desktopWidth;
    int desktopHeight;
	
	int useFullscreen = (WindowStyle & Style::Fullscreen);
	
    if (useFullscreen)
    {
        CFDictionaryRef desktopVideoMode = CGDisplayCurrentMode(kCGDirectMainDisplay);
	
        CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(desktopVideoMode, kCGDisplayWidth), kCFNumberIntType, &desktopWidth);
        CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(desktopVideoMode, kCGDisplayHeight), kCFNumberIntType, &desktopHeight);

        viewRect.left = (desktopWidth - Mode.Width) / 2;
        viewRect.top  = (desktopHeight - Mode.Height) / 2;
    }
    else
    { 
        viewRect.left = 0;
        viewRect.top = 0;
    }

    viewRect.right = myWidth = (viewRect.left + Mode.Width);
    viewRect.bottom = myHeight = (viewRect.top + Mode.Height);

    windowAttrib ^= ((WindowStyle & Style::Titlebar) ? kWindowResizableAttribute | kWindowLiveResizeAttribute :  0 );

    // Create the window
    CreateNewWindow(kDocumentWindowClass, windowAttrib, &viewRect, &myHandle);

    // Create the rendering context
    if (myHandle)
        CreateContext(Mode, useFullscreen, AntialiasingLevel);

    PostInitialize();
		
    // Increment window count
#if SFML_JOYSTICK_SUPPORT
    if (ourWindowCount == 0)
        SetupJoysticks();
#endif // SFML_JOYSTICK_SUPPORT

    ourWindowCount++;

    if (!useFullscreen)
    {
        CFStringRef windowTitle = CFStringCreateWithCString(kCFAllocatorDefault, Title.c_str(), kCFStringEncodingUTF8);		
        SetWindowTitleWithCFString(myHandle, windowTitle);
        RepositionWindow(myHandle, NULL, kWindowCenterOnMainScreen);
    }

    ShowWindow(myHandle);
}


////////////////////////////////////////////////////////////
/// Destructor
////////////////////////////////////////////////////////////
WindowImplCarbon::~WindowImplCarbon()
{
    if (myHandle)
    {
        Cleanup();
        DisposeWindow(myHandle);
    }

    // Decrement the window count
    ourWindowCount--;

    if (ourWindowCount == 0)
    {
#if SFML_JOYSTICK_SUPPORT
        joySupport.ReleaseDevices(joysticks);
        joysticks.clear();
#endif // SFML_JOYSTICK_SUPPORT
    }
}


////////////////////////////////////////////////////////////
/// /see WindowImpl::Display
////////////////////////////////////////////////////////////
void WindowImplCarbon::Display()
{
    if (myGLContext)
		aglSwapBuffers(myGLContext);
}


////////////////////////////////////////////////////////////
/// /see WindowImpl::ProcessEvents
////////////////////////////////////////////////////////////
void WindowImplCarbon::ProcessEvents()
{
    EventRef event;
    EventTargetRef target = GetEventDispatcherTarget();

    while (ReceiveNextEvent(0, NULL, 0.0, true, &event)== noErr)
    {
        SendEventToEventTarget (event, target);
        ReleaseEvent(event);
    }
	
#if SFML_JOYSTICK_SUPPORT
	ReadJoystickStates();
#endif // SFML_JOYSTICK_SUPPORT
}


////////////////////////////////////////////////////////////
/// /see WindowImpl::MakeCurrent
////////////////////////////////////////////////////////////
void WindowImplCarbon::MakeActive(bool Active) const
{
    if (myGLContext)
	{
		if (Active)
			aglSetCurrentContext(myGLContext);
		else
			aglSetCurrentContext(NULL);
	}
}


////////////////////////////////////////////////////////////
/// /see WindowImpl::UseVerticalSync
////////////////////////////////////////////////////////////
void WindowImplCarbon::UseVerticalSync(bool Enabled)
{
    aglSetInteger(myGLContext, AGL_SWAP_INTERVAL, (GLint*)&Enabled);
}


////////////////////////////////////////////////////////////
/// /see WindowImpl::ShowMouseCursor
////////////////////////////////////////////////////////////
void WindowImplCarbon::ShowMouseCursor(bool Show)
{
    if (Show)
        ShowCursor();
    else
        HideCursor();	
}

////////////////////////////////////////////////////////////
/// See WindowImple::SetCursorPosition
////////////////////////////////////////////////////////////
void WindowImplCarbon::SetCursorPosition(unsigned int Left, unsigned int Top)
{
#warning TO DO
}

////////////////////////////////////////////////////////////
/// /see WindowImpl::SetPosition
////////////////////////////////////////////////////////////
void WindowImplCarbon::SetPosition(int Left, int Top)
{
    MoveWindow(myHandle, Left, Top, false);
}

////////////////////////////////////////////////////////////
/// See WindowImpl::Show
////////////////////////////////////////////////////////////
void WindowImplCarbon::Show(bool State)
{
#warning TO DO
}

////////////////////////////////////////////////////////////
/// See WindowImpl::EnableKeyRepeat
////////////////////////////////////////////////////////////
void WindowImplCarbon::EnableKeyRepeat(bool Enabled)
{
#warning TO DO
}

////////////////////////////////////////////////////////////
/// Do some common initializations before the window can be created
////////////////////////////////////////////////////////////
void WindowImplCarbon::PreInitialize()
{
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFURLRef bundle_url = CFBundleCopyBundleURL( bundle );

    if (bundle && bundle_url)
    {
        CFStringRef sRef = CFURLCopyFileSystemPath( bundle_url, kCFURLPOSIXPathStyle );

        if (sRef)
        {			
            char myPath[FILENAME_MAX];
            CFStringGetCString( sRef, myPath, FILENAME_MAX, kCFStringEncodingASCII );
			
			
			// -- Ceylo --
			// CFBundleGetMainBundle() should return NULL if we're not using a bundle application
			// but it doesn't, so we check the bundle path is a directory and has .app suffix.
			// This assumes .app directories are bundle applications, which is actually
			// the case most of the time (always the case until the user wants to trap SFML).
			if (CFURLHasDirectoryPath(bundle_url) && CFStringHasSuffix(sRef, CFSTR(".app"))) {
				chdir( "../" );
			}
			
            CFRelease( sRef );
        }
	
        CFRelease( bundle_url );
    }	
}

////////////////////////////////////////////////////////////
/// Run the event loop after the window has been created
////////////////////////////////////////////////////////////
void WindowImplCarbon::PostInitialize()
{
    EventHandlerUPP CommandHandler = NewEventHandlerUPP(reinterpret_cast<EventHandlerProcPtr>(&WindowImplCarbon::GlobalOnEvent));
    InstallEventHandler(GetWindowEventTarget(myHandle), CommandHandler, GetEventTypeCount(eventSpec), eventSpec, this, 0 );
}


////////////////////////////////////////////////////////////
/// Create the OpenGL rendering context
////////////////////////////////////////////////////////////
void WindowImplCarbon::CreateContext(VideoMode Mode, bool Fullscreen, int AntialiasingLevel)
{
    GLint attributes[] =
    {
        AGL_RGBA,
        AGL_DOUBLEBUFFER,
        AGL_DEPTH_SIZE, Mode.BitsPerPixel,
        AGL_SAMPLES_ARB, AntialiasingLevel,
        AGL_SAMPLE_BUFFERS_ARB, 1,
		AGL_FULLSCREEN,
        AGL_NONE
    };

    AGLPixelFormat myAGLPixelFormat = 0;
    if (Fullscreen)
    {
        boolean_t exactMatch;

        CGDisplayCapture(kCGDirectMainDisplay); 
        CFDictionaryRef displayMode = CGDisplayBestModeForParametersAndRefreshRate(kCGDirectMainDisplay, Mode.BitsPerPixel, Mode.Width, Mode.Height, 60, &exactMatch);

        if (displayMode && exactMatch)
        {
            GDHandle gdhDisplay;
            CGDisplaySwitchToMode(kCGDirectMainDisplay, displayMode);

            if (DMGetGDeviceByDisplayID ((DisplayIDType)CGMainDisplayID(), &gdhDisplay, false) == noErr)
            {
                if (AntialiasingLevel > 0)
                {
                    myAGLPixelFormat = aglChoosePixelFormat(&gdhDisplay, 1, attributes);
                    if (!myAGLPixelFormat && AntialiasingLevel > 2)
                    {
                        // No format matching our needs : reduce the multisampling level
                        std::cerr << "Failed to find a pixel format supporting " << AntialiasingLevel << " antialiasing levels ; trying with 2 levels" << std::endl;

                        attributes[5] = 2;
                        myAGLPixelFormat = aglChoosePixelFormat(&gdhDisplay, 1, attributes);

                        if (!myAGLPixelFormat)
                        {
                            // Cannot find any pixel format supporting multisampling ; disabling antialiasing
                            std::cerr << "Failed to find a pixel format supporting antialiasing ; antialiasing will be disabled" << std::endl;
                        }
                    }
                }
				
                if (!myAGLPixelFormat)
                {
                    attributes[4] = attributes[8];
                    attributes[5] = AGL_NONE;
                    myAGLPixelFormat = aglChoosePixelFormat(&gdhDisplay, 1, attributes);
                }
            }
        }
    }
    else
    {
        attributes[8] = AGL_NONE;
	
        if (AntialiasingLevel > 0)
        {
            myAGLPixelFormat = aglChoosePixelFormat(NULL, 0, attributes);
            if (!myAGLPixelFormat && AntialiasingLevel > 2)
            {
                // No format matching our needs : reduce the multisampling level
                std::cerr << "Failed to find a pixel format supporting " << AntialiasingLevel << " antialiasing levels ; trying with 2 levels" << std::endl;

                attributes[5] = 2;
                myAGLPixelFormat = aglChoosePixelFormat(NULL, 0, attributes);

                if (!myAGLPixelFormat)
                {
                    // Cannot find any pixel format supporting multisampling ; disabling antialiasing
                    std::cerr << "Failed to find a pixel format supporting antialiasing ; antialiasing will be disabled" << std::endl;
                }
            }
        }

        if (!myAGLPixelFormat)
        {
            attributes[4] = AGL_NONE;
            myAGLPixelFormat = aglChoosePixelFormat(NULL, 0, attributes);
        }
    }

    if (myAGLPixelFormat)
    {
        if (Fullscreen)
        {
            // NOTE: Fullscreen contexts don't seem to share well so we don't do it for now
            myGLContext = aglCreateContext(myAGLPixelFormat, NULL);
            aglDestroyPixelFormat(myAGLPixelFormat);

            if (myGLContext)
            {
                if (aglSetCurrentContext(myGLContext) == GL_TRUE)
                {
                    if (aglSetFullScreen(myGLContext, Mode.Width, Mode.Height, 0, 0) == GL_TRUE)
                    {
                        GLint swap = 1;
                        aglSetInteger(myGLContext, AGL_SWAP_INTERVAL, &swap);
                    }
                }
            }
        }
        else
        {
            // If a current context exists, share it's objects with this new context
            myGLContext = aglCreateContext(myAGLPixelFormat, aglGetCurrentContext());
            aglDestroyPixelFormat(myAGLPixelFormat);

            if (myGLContext)
            {
                aglSetDrawable(myGLContext, GetWindowPort(myHandle));
            }
        }

        // Set our context as the current OpenGL context for rendering
        SetActive();
    }
}

////////////////////////////////////////////////////////////
/// Free all the graphical resources attached to the window
////////////////////////////////////////////////////////////
void WindowImplCarbon::Cleanup()
{
    // Unhide the mouse cursor (in case it was hidden)
    ShowMouseCursor(true);

    // Destroy the OpenGL context
    if (myGLContext)
    {
        // If this is not the dummy window, we must set it as the valid rendering context to avoid a crash with next OpenGL command
        if (this != ourDummyWindow)
        {
            if (ourDummyWindow)
                ourDummyWindow->SetActive();
        }
        else
        {
            ourDummyWindow = NULL;
            aglSetCurrentContext(NULL);
        }

        // Destroy the context
        aglSetDrawable(myGLContext, NULL);
        aglDestroyContext(myGLContext);
        myGLContext = NULL;
    }
}

////////////////////////////////////////////////////////////
/// Function called whenever one of our windows receives a message
////////////////////////////////////////////////////////////
OSStatus WindowImplCarbon::CommandEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData)
{
    HICommand command;
    GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(command), NULL, &command);

    switch(command.commandID)
    {
        case kHICommandNew :
			return noErr;
        break;

        case kHICommandQuit :
        case kHICommandClose :
            return noErr;
        break;
    }
	
    return 	CallNextEventHandler(nextHandler, event);
}

////////////////////////////////////////////////////////////
/// Function called whenever a mouse event occurs
////////////////////////////////////////////////////////////
OSStatus WindowImplCarbon::MouseEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData)
{
    EventMouseButton button = 0;
	HIPoint          location = {0.0f, 0.0f};
	
    GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button);
    GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &location);

    Event Evt;
    Evt.MouseButton.Button = Mouse::Left;
    Evt.MouseMove.X = (unsigned)location.x;
    Evt.MouseMove.Y = (unsigned)location.y;
    switch( button )
    {
        case kEventMouseButtonPrimary :
            Evt.MouseButton.Button = Mouse::Left;
			break;
        case kEventMouseButtonSecondary :
            Evt.MouseButton.Button = Mouse::Right;
			break;
        case kEventMouseButtonTertiary :
            Evt.MouseButton.Button = Mouse::Middle;
			break;
    }
	
    switch(GetEventKind(event))
    {
        // Mouse button down event
        case kEventMouseDown :
        {
            Evt.Type = Event::MouseButtonPressed;
            SendEvent(Evt);
            break;
        }

        // Mouse button up event
        case kEventMouseUp :
        {
            Evt.Type = Event::MouseButtonReleased;
            SendEvent(Evt);
            break;
		}

        // Mouse moved event
        case kEventMouseMoved :
        case kEventMouseDragged :
        {
            Evt.Type = Event::MouseMoved;
            SendEvent(Evt);
            break;
        }

        // Mouse wheel event
        case kEventMouseWheelMoved :
        {
            long wheelDelta = 0;
            GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(long), NULL, &wheelDelta);
			
            Evt.Type = Event::MouseWheelMoved;
            Evt.MouseWheel.Delta = wheelDelta;
            SendEvent(Evt);
            break;
        }
    }	
	
    return CallNextEventHandler(nextHandler, event);
}


////////////////////////////////////////////////////////////
/// Function called whenever a keyboard event occurs
////////////////////////////////////////////////////////////
OSStatus WindowImplCarbon::KeyboardEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData)
{
    UInt32 keyCode;
    UInt32 modifiers;

    GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
    GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 0, sizeof(UInt32), 0, &modifiers);

    Event Evt;
    Evt.Key.Code    = VirtualKeyCodeToSF(keyCode);
    Evt.Key.Alt     = (modifiers & optionKey) || (modifiers & rightOptionKey);
    Evt.Key.Control = (modifiers & controlKey) || (modifiers & rightControlKey);
    Evt.Key.Shift   = (modifiers & shiftKey) || (modifiers & rightShiftKey);

    switch(GetEventKind(event))
    {
        case kEventRawKeyDown :
        {
            Evt.Type = Event::KeyPressed;
            SendEvent(Evt);
            break;
        }
        case kEventRawKeyUp :
        {
            Evt.Type = Event::KeyReleased;
            SendEvent(Evt);
            break;
        }
    }

    return CallNextEventHandler(nextHandler, event);
}

////////////////////////////////////////////////////////////
/// Process a Carbon window event
////////////////////////////////////////////////////////////
OSStatus WindowImplCarbon::WindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void* userData)
{
    Event Evt;

	switch(GetEventKind(event))
	{
		case kEventWindowDrawContent :
		break;
			
		// Window closed event
		case kEventWindowClose :
		{
            Evt.Type = Event::Closed;
			SendEvent(Evt);
			break;
		}

        // Update window size
        case kEventWindowBoundsChanging :
        case kEventWindowBoundsChanged :
        {
            SetActive();
            aglUpdateContext(myGLContext);

            Rect viewRect;
            GetWindowBounds(myHandle, kWindowContentRgn, &viewRect);
			
            myWidth  = viewRect.right - viewRect.left;
            myHeight = viewRect.bottom - viewRect.top;

            Evt.Type        = Event::Resized;
            Evt.Size.Width  = myWidth;
            Evt.Size.Height = myHeight;
            SendEvent(Evt);
            break;
        }

        // Gain focus event
        case kEventWindowActivated :
        {
            Evt.Type = Event::GainedFocus;
            SendEvent(Evt);
            break;
        }

        // Lost focus event
        case kEventWindowCollapsing :
        {
            Evt.Type = Event::LostFocus;
            SendEvent(Evt);
            break;
        }
    }
	
    return CallNextEventHandler(nextHandler, event);
}


////////////////////////////////////////////////////////////
/// Setup the joysticks
////////////////////////////////////////////////////////////
#if SFML_JOYSTICK_SUPPORT
void WindowImplCarbon::SetupJoysticks()
{
    joySupport.EnumerateDevices(joysticks);
}
#endif // SFML_JOYSTICK_SUPPORT


////////////////////////////////////////////////////////////
/// Read the joystick states
////////////////////////////////////////////////////////////
#if SFML_JOYSTICK_SUPPORT
void WindowImplCarbon::ReadJoystickStates()
{
	std::vector<JoystickDevice>::iterator end = joysticks.end();
	std::vector<JoystickDevice>::iterator iter = joysticks.begin();
	
	for (; iter != end; ++iter)
	{
		IOHIDDeviceInterface    **interface = (*iter).hidDeviceInterface;
        IOHIDElementCookie      cookie;
        IOHIDEventStruct        hidEvent;
        int                     count;
	
        for (count = 0; count < (*iter).axis.size(); ++count)
        {
            cookie = (*iter).axis[count].cookie;
            
			if ((*interface)->getElementValue(interface, cookie, &hidEvent) == noErr)
            {
                if (hidEvent.value != (*iter).axis[count].value)
                {
                    Event Evt;
                    Evt.Type                = Event::JoyMoved;
                    Evt.JoyMove.JoystickId = (*iter).locationID;

                    switch((*iter).axis[count].usage)
                    {
                        case kHIDUsage_GD_X :
                        case kHIDUsage_GD_Rx :
                            Evt.JoyMove.Axis.AxisX = (2.0f*(hidEvent.value - (*iter).axis[count].min) / ((*iter).axis[count].max - (*iter).axis[count].min)) - 1.0f;
                        break;
                        case kHIDUsage_GD_Y :
                        case kHIDUsage_GD_Ry :
                            Evt.JoyMove.AxisY = (2.0f*(hidEvent.value - (*iter).axis[count].min) / ((*iter).axis[count].max - (*iter).axis[count].min)) - 1.0f;
                        break;
                        case kHIDUsage_GD_Z :
                        case kHIDUsage_GD_Rz :
                            Evt.JoyMove.AxisZ = (2.0f*(hidEvent.value - (*iter).axis[count].min) / ((*iter).axis[count].max - (*iter).axis[count].min)) - 1.0f;
                        break;
                    }

                    SendEvent(Evt);
                    (*iter).axis[count].value = hidEvent.value;					
                }
            }
        }

        for (count = 0; count < (*iter).axis.size(); ++count)
        {
            cookie = (*iter).buttons[count].cookie;
            
			if ((*interface)->getElementValue(interface, cookie, &hidEvent) == noErr)
            {
                if (hidEvent.value)
				{
				std::cout << "HID event type: " << hidEvent.type << std::endl;
				std::cout << "HID event value: " << hidEvent.value << std::endl;
                }
				if (hidEvent.value != (*iter).buttons[count].value)
                {
                    Event Evt;
                    Evt.Type                = (hidEvent.value == 0) ? Event::JoyButtonPressed : Event::JoyButtonReleased;
                    Evt.JoyMove.JoystickId = (*iter).locationID;
                    Evt.JoyButton.Button     = count;
					std::cout << "Sending button event" << std::endl;

                    SendEvent(Evt);
                }
                (*iter).buttons[count].value = hidEvent.value;
            }
        }
	}
}
#endif


////////////////////////////////////////////////////////////
/// Convert a Carbon key code to a SFML key code
////////////////////////////////////////////////////////////
Key::Code WindowImplCarbon::VirtualKeyCodeToSF(UInt32 VirtualKey)
{
    switch (VirtualKey)
    {
        case 0x35 :   return Key::Escape;
        case 0x31 :   return Key::Space;
        case 0x4C :   return Key::Return;
        case 0x33 :   return Key::Back;
        case 0x30 :   return Key::Tab;
        case 0x74 :   return Key::PageUp;
        case 0x79 :   return Key::PageDown;
        case 0x77 :   return Key::End;
        case 0x73 :   return Key::Home;
        case 0x72 :   return Key::Insert;
        case 0x75 :   return Key::Delete;
        case 0x45 :   return Key::Add;
        case 0x4E :   return Key::Subtract;
        case 0x43 :   return Key::Multiply;
        case 0x4B :   return Key::Divide;
        case 0x7A :   return Key::F1;
        case 0x78 :   return Key::F2;
        case 0x63 :   return Key::F3;
        case 0x76 :   return Key::F4;
        case 0x60 :   return Key::F5;
        case 0x61 :   return Key::F6;
        case 0x62 :   return Key::F7;
        case 0x64 :   return Key::F8;
        case 0x65 :   return Key::F9;
        case 0x6D :   return Key::F10;
        case 0x67 :   return Key::F11;
        case 0x6F :   return Key::F12;
        case 0x69 :   return Key::F13;
        case 0x6B :   return Key::F14;
        case 0x71 :   return Key::F15;
        case 0x7B :   return Key::Left;
        case 0x7C :   return Key::Right;
        case 0x7E :   return Key::Up;
        case 0x7D :   return Key::Down;
        case 0x52 :   return Key::Numpad0;
        case 0x53 :   return Key::Numpad1;
        case 0x54 :   return Key::Numpad2;
        case 0x55 :   return Key::Numpad3;
        case 0x56 :   return Key::Numpad4;
        case 0x57 :   return Key::Numpad5;
        case 0x58 :   return Key::Numpad6;
        case 0x59 :   return Key::Numpad7;
        case 0x5B :   return Key::Numpad8;
        case 0x5C :   return Key::Numpad9;
        case 0x00 :   return Key::A;
        case 0x0B :   return Key::B;
        case 0x08 :   return Key::C;
        case 0x02 :   return Key::D;
        case 0x0E :   return Key::E;
        case 0x03 :   return Key::F;
        case 0x05 :   return Key::G;
        case 0x04 :   return Key::H;
        case 0x22 :   return Key::I;
        case 0x26 :   return Key::J;
        case 0x28 :   return Key::K;
        case 0x25 :   return Key::L;
        case 0x2E :   return Key::M;
        case 0x2D :   return Key::N;
        case 0x1F :   return Key::O;
        case 0x23 :   return Key::P;
        case 0x0C :   return Key::Q;
        case 0x0F :   return Key::R;
        case 0x01 :   return Key::S;
        case 0x11 :   return Key::T;
        case 0x20 :   return Key::U;
        case 0x09 :   return Key::V;
        case 0x0D :   return Key::W;
        case 0x07 :   return Key::X;
        case 0x10 :   return Key::Y;
        case 0x06 :   return Key::Z;
        case 0x1D :   return Key::Num0;
        case 0x12 :   return Key::Num1;
        case 0x13 :   return Key::Num2;
        case 0x14 :   return Key::Num3;
        case 0x15 :   return Key::Num4;
        case 0x17 :   return Key::Num5;
        case 0x16 :   return Key::Num6;
        case 0x1A :   return Key::Num7;
        case 0x1C :   return Key::Num8;
        case 0x19 :   return Key::Num9;
    }

    return Key::Code(0);
}


////////////////////////////////////////////////////////////
/// Function called whenever one of our windows receives a message
////////////////////////////////////////////////////////////
OSStatus WindowImplCarbon::GlobalOnEvent(EventHandlerCallRef nextHandler, EventRef event, void* userData)
{
    WindowImplCarbon* This = (WindowImplCarbon*)userData;

    // Forward the event to the appropriate function
    if (This)
    {
        switch (GetEventClass(event))
        {
            case kEventClassCommand :
                This->CommandEventHandler( nextHandler, event, userData );
                break;
            case kEventClassWindow :
                This->WindowEventHandler( nextHandler, event, userData );
                break;
            case kEventClassMouse :
                This->MouseEventHandler( nextHandler, event, userData );
                break;
            case kEventClassKeyboard :
                This->KeyboardEventHandler( nextHandler, event, userData );
                break;
        }
    }

    return noErr;
}

} // namespace priv

} // namespace sf
