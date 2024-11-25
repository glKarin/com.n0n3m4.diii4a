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

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#import <SFML/Window/OSXCocoa/WindowImplCocoa.hpp>
#import <SFML/Window/WindowStyle.hpp>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <Carbon/Carbon.h>
#import <iostream>
#import <Foundation/NSDebug.h>

#define SFML_OSX_DEBUG

// -- Ceylo --
// These macros are used to simply make autorelease pools for each function
// using Cocoa. Forgetting to set an autorelease pool causes memory leaks
// and crashes.
#define LOCAL_POOL_START	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];\
if (pool == nil) error(__FILE__, __LINE__, "couldn't create local autorelease pool");
#define LOCAL_POOL_END	[pool release]


static void PRNT(char const *filename, unsigned line, id ptr, char const *name)
{
	NSLog (@"%@ (line %d) : %p named %@ (class %@)\n",\
		   [[NSString stringWithUTF8String:filename] lastPathComponent],\
		   line, ptr, [NSString stringWithUTF8String:name],\
		   [ptr className]);
}

static void error (char const *filename, unsigned line, char const *description, ...)
{
	LOCAL_POOL_START;
	va_list ap;
	char *buffer = NULL;
	
	va_start(ap, description);
	vasprintf(&buffer, description, ap);
	
	printf("*** [SFML] An critical error occured in %s line %d : %s\n",
		   [[[NSString stringWithUTF8String:filename] lastPathComponent] UTF8String],
		   line, (buffer != NULL) ? buffer : "no description");
	
	LOCAL_POOL_END;
	exit (EXIT_FAILURE);
}



#define WindowedContext false
#define FullscreenContext true

// Can't make Objective-C class declarations or implementations in a namespace
// So we put it here
@implementation NSApplication (SFML)
- (void)setRunning
{
	_running = 1;
}

@end


namespace sf
{
namespace priv
{

////////////////////////////////////////////////////////////
/// Default constructor
/// (creates a dummy window to provide a valid OpenGL context)
////////////////////////////////////////////////////////////
WindowImplCocoa::WindowImplCocoa() :
mainPool(nil),
controller(nil),
windowHandle(nil),
glContext(nil),
glView(nil),
useKeyRepeat(false)
{
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	// We make a main autorelease pool to be sure there will always
	// be at least one while notifications are sent by application
	mainPool = [[NSAutoreleasePool alloc] init];
	
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	if (mainPool == nil) {
		error(__FILE__, __LINE__, "couldn't create main autorelease pool");
	}
	
	LOCAL_POOL_START;
	
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	
	NSOpenGLPixelFormat *pixFormat = nil;
	
	// Register application if needed and launch it
	RunApplication ();
	
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	// -- Ceylo --
    // We just want to have a valid support for an OpenGL context
	
	// So we create the OpenGL pixel format
	WindowSettings params = WindowSettings();
	pixFormat = NewGLPixelFormat (params, WindowedContext);
	
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	if (pixFormat != nil) {
		// And the OpenGL context
		glContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat
											   shareContext:[NSOpenGLContext currentContext]];
		
#ifdef SFML_OSX_DEBUG
		[NSAutoreleasePool showPools];
#endif
		
		if (glContext != nil) {
			// We make it the current active OpenGL context
			MakeActive();
			
#ifdef SFML_OSX_DEBUG
			[NSAutoreleasePool showPools];
#endif
		} else {
			error(__FILE__, __LINE__, "unable to make dummy OpenGL context");
		}
	} else {
		error(__FILE__, __LINE__, "unable to make dummy OpenGL pixel format");
	}
	
	LOCAL_POOL_END;
	
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
}


////////////////////////////////////////////////////////////
/// Create the window implementation from an existing control
////////////////////////////////////////////////////////////
WindowImplCocoa::WindowImplCocoa(WindowHandle Handle, WindowSettings& params) :
mainPool(nil),
controller(nil),
windowHandle(nil),
glContext(nil),
glView(nil),
useKeyRepeat(false)
{
	// We make a main autorelease pool to be sure there will always
	// be at least one while notifications are sent by application
	mainPool = [[NSAutoreleasePool alloc] init];
	
	if (mainPool == nil) {
		error(__FILE__, __LINE__, "couldn't create main autorelease pool");
	}
	
	LOCAL_POOL_START;
	
	// Register application if needed and launch it
	RunApplication();
	
	// Make a WindowController to handle notifications
	controller = [[WindowController controllerWithWindow:(void *) this] retain];
	
	// Use existing window
	windowHandle = [(NSWindow *) Handle retain];
	
	if (windowHandle != nil) {
		// Initialize myWidth and myHeight members from base class with the window size
		myWidth = (unsigned) [windowHandle frame].size.width;
		myHeight = (unsigned) [windowHandle frame].size.height;
		
		// will be used to check whether mouse moved
		prevMousePos = [NSEvent mouseLocation];
		
		// We make the OpenGL context, associate it to the OpenGL view
		// and add the view to our window
		AddOpenGLView (params);
	}
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// Create the window implementation
////////////////////////////////////////////////////////////
WindowImplCocoa::WindowImplCocoa(VideoMode Mode, const std::string& Title, unsigned long WindowStyle, WindowSettings& params) :
mainPool(nil),
controller(nil),
windowHandle(nil),
glContext(nil),
glView(nil),
useKeyRepeat(false)
{
	LOCAL_POOL_START;
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	
	// We make a main autorelease pool to be sure there will always
	// be at least one while notifications are sent by application
	mainPool = [[NSAutoreleasePool alloc] init];
	
	if (mainPool == nil) {
		error(__FILE__, __LINE__, "couldn't create main autorelease pool");
	}	
	
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	
	// Register application if needed and launch it
	RunApplication();
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	
	// Make a WindowController to handle notifications
	controller = [[WindowController controllerWithWindow:(void *) this] retain];
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	
    // Create a new window with given size, title and style
	if (WindowStyle & Style::Fullscreen) {
		// Not usable for now
		error(__FILE__, __LINE__, "unimplemented section");
	} else {
		// First we define some objects used for our window
		NSString *title = [NSString stringWithUTF8String:Title.c_str()];
		NSRect frame = NSMakeRect (0.0f, 0.0f, (float) Mode.Width, (float) Mode.Height);
		unsigned int mask = 0;
		
		// We grab options from WindowStyle and add them to our window mask
		if (WindowStyle & Style::None) {
			mask |= NSBorderlessWindowMask;
		} else {
			if (WindowStyle & Style::Titlebar) {
				mask |= NSTitledWindowMask;
				mask |= NSMiniaturizableWindowMask;
			}
			
			if (WindowStyle & Style::Resize) {
				mask |= NSTitledWindowMask;
				mask |= NSMiniaturizableWindowMask;
				mask |= NSResizableWindowMask;
			}
			
			if (WindowStyle & Style::Close) {
				mask |= NSTitledWindowMask;
				mask |= NSClosableWindowMask;
				mask |= NSMiniaturizableWindowMask;
			}
		}
		
		// Now we make the window with the values we got
		// Note: defer flag set to NO to be able to use OpenGL in our window
		windowHandle = [[NSWindow alloc] initWithContentRect:frame
												   styleMask:mask
													 backing:NSBackingStoreBuffered
													   defer:NO];
#ifdef SFML_OSX_DEBUG
		[NSAutoreleasePool showPools];
#endif
		
		if (windowHandle != nil) {
			// We set title and window position
			[windowHandle setTitle:title];
			[windowHandle center];
			
			// We set the myWidth and myHeight members to the correct values
			myWidth = Mode.Width;
			myHeight = Mode.Height;
			
			// will be used to check whether mouse moved
			prevMousePos = [NSEvent mouseLocation];
			
			// We make the OpenGL context, associate it to the OpenGL view
			// and add the view to our window
#ifdef SFML_OSX_DEBUG
			[NSAutoreleasePool showPools];
#endif
			AddOpenGLView(params);
#ifdef SFML_OSX_DEBUG
			[NSAutoreleasePool showPools];
#endif
		} else {
			error(__FILE__, __LINE__, "failed to make window");
		}
	}
#warning Crash here >> something more should probably be retained
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// Destructor
////////////////////////////////////////////////////////////
WindowImplCocoa::~WindowImplCocoa()
{
	LOCAL_POOL_START;
	
    // Destroy the OpenGL context, the window and every resource allocated by this class
	[[NSNotificationCenter defaultCenter] removeObserver:windowHandle];
	[[NSNotificationCenter defaultCenter] removeObserver:glView];
	[controller release];
	
	[glContext release];
	[glView release];
	[windowHandle release];
	
	LOCAL_POOL_END;
	[mainPool release];
	[NSApp release];
}

////////////////////////////////////////////////////////////
/// Handle event sent by the default NSNotificationCenter
////////////////////////////////////////////////////////////
void WindowImplCocoa::HandleNotifiedEvent(Event& event)
{
	// Set myWidth and myHeight to correct value if
	// window size changed
	switch (event.Type) {
		case Event::Resized:
			myWidth = event.Size.Width;
			myHeight = event.Size.Height;
			break;
			
		default:
			break;
	}
	
	// And send the event
	SendEvent(event);
}

////////////////////////////////////////////////////////////
/// /see sfWindowImpl::Display
////////////////////////////////////////////////////////////
void WindowImplCocoa::Display()
{
	LOCAL_POOL_START;
	
	// I don't want to flush the context if it's not linked to our view
	// but I'll have to change this when I implement full screen handling
	if ([glContext view] == glView) {
		[glContext flushBuffer];
	}
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// /see sfWindowImpl::ProcessEvents
////////////////////////////////////////////////////////////
void WindowImplCocoa::ProcessEvents()
{
	LOCAL_POOL_START;
	
    // Process every event for this window
	NSEvent *event = nil;
	
	if (![NSApp isRunning]) {
		LOCAL_POOL_END;
		return;
	}
	
	// Loop until there is no more event to take
	while (nil != (event = [windowHandle nextEventMatchingMask:NSAnyEventMask
													 untilDate:nil
														inMode:NSEventTrackingRunLoopMode
													   dequeue:YES])) {
		Event sfEvent;
		bool usedEvent = false;
		unichar chr = 0;
		unsigned mods = [event modifierFlags];
		
		switch ([event type]) {
			case NSKeyDown:
				if ([[event characters] length]) {
					chr = [[event characters] characterAtIndex:0];
				}
				
				if (mods & NSCommandKeyMask) {
					// Application reserved commands
					
					if (chr == 'q' &&
						!(mods & NSControlKeyMask) &&
						!(mods & NSAlternateKeyMask)) {
						QuitApplication();
						
					} else if (chr == 'h' &&
							   !(mods & NSControlKeyMask) &&
							   !(mods & NSAlternateKeyMask)) {
						[NSApp hide:nil];
						
					} else if (chr == 'h' &&
							   !(mods & NSControlKeyMask) &&
							   (mods & NSAlternateKeyMask)){
						// Note: something is wrong with this case (not working)
						[NSApp hideOtherApplications:nil];
					}
				} else {
					// User events
					
					if (!useKeyRepeat && [event isARepeat]) {
						break;
					}
					
					// If C string length is greater than 0, it's a TextEntered event
					if ([[event characters] cStringLength]) {
						sfEvent.Type = Event::TextEntered;
						sfEvent.Text.Unicode = chr;
						
						SendEvent(sfEvent);
					}
					
					// Anyway it's also a KeyPressed event
					sfEvent.Type = Event::KeyPressed;
					if (Key::Code(0) == (sfEvent.Key.Code = KeyForUnicode(chr))) {
						sfEvent.Key.Code = KeyForVirtualCode([event keyCode]);
					}
					
					sfEvent.Key.Alt = mods & NSAlternateKeyMask;
					sfEvent.Key.Control = mods & NSControlKeyMask;
					sfEvent.Key.Shift = mods & NSShiftKeyMask;
					
					SendEvent(sfEvent);
				}
				
				usedEvent = true;
				break;
				
			case NSKeyUp:
				if ([[event characters] length]) {
					chr = [[event characters] characterAtIndex:0];
				}
				
				if (!(mods & NSCommandKeyMask)) {
					// User events
					
					sfEvent.Type = Event::KeyReleased;
					if (Key::Code(0) == (sfEvent.Key.Code = KeyForUnicode(chr))) {
						sfEvent.Key.Code = KeyForVirtualCode([event keyCode]);
					}
					
					sfEvent.Key.Alt = mods & NSAlternateKeyMask;
					sfEvent.Key.Control = mods & NSControlKeyMask;
					sfEvent.Key.Shift = mods & NSShiftKeyMask;
					
					SendEvent(sfEvent);
				}
				
				usedEvent = true;
				break;
				
			case NSFlagsChanged:
				// uh....... lol
				// I let this for later xD
				// Should be handled as KeyPressed or KeyReleased
				// but a bit more tricky to define than other key events :P
				break;
				
			case NSScrollWheel:				
				sfEvent.Type = Event::MouseWheelMoved;
				
				// FIXME: is it only Y axis ?
				sfEvent.MouseWheel.Delta = (int)[event deltaY];
				
				SendEvent(sfEvent);
				usedEvent = true;
				break;
				
			case NSLeftMouseDown:
				sfEvent.Type = Event::MouseButtonPressed;
				
				if (mods & NSControlKeyMask) {
					sfEvent.MouseButton.Button = Mouse::Right;
				} else {
					sfEvent.MouseButton.Button = Mouse::Left;
				}
					
					SendEvent(sfEvent);
				break;
				
			case NSLeftMouseUp:
				sfEvent.Type = Event::MouseButtonReleased;
				
				if (mods & NSControlKeyMask) {
					sfEvent.MouseButton.Button = Mouse::Right;
				} else {
					sfEvent.MouseButton.Button = Mouse::Left;
				}
					
					SendEvent(sfEvent);
				break;
				
			case NSRightMouseDown:
				sfEvent.Type = Event::MouseButtonPressed;
				sfEvent.MouseButton.Button = Mouse::Right;
				
				SendEvent(sfEvent);
				break;
				
			case NSRightMouseUp:
				sfEvent.Type = Event::MouseButtonReleased;
				sfEvent.MouseButton.Button = Mouse::Right;
				
				SendEvent(sfEvent);
				break;
				
			default:
				break;
		}
		
		if (!usedEvent) {
			[NSApp sendEvent:event];
		}
	}
	
	// Easier way to handle mouse moved events :
	// we check mouse position changed
	NSPoint loc = [NSEvent mouseLocation];
	
	if ([windowHandle isKeyWindow] && 
		(loc.x != prevMousePos.x || loc.y != prevMousePos.y)) {
		prevMousePos = loc;
		
		if (NSPointInRect(loc, [windowHandle frame])) {
			Event sfEvent;
			NSPoint relativePt = [windowHandle convertScreenToBase:loc];
			relativePt.y = [windowHandle frame].size.height - relativePt.y;
			sfEvent.Type = Event::MouseMoved;
			
			sfEvent.MouseMove.X = (unsigned) relativePt.x;
			sfEvent.MouseMove.Y = (unsigned) relativePt.y;
			
			SendEvent(sfEvent);
		}
	}
	
    // Generate a sf::Event and call SendEvent(Evt) for each event
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// /see sfWindowImpl::MakeActive
////////////////////////////////////////////////////////////
void WindowImplCocoa::MakeActive(bool Active) const
{
	LOCAL_POOL_START;
	
	if (Active) {
		[glContext makeCurrentContext];
	} else {
		[NSOpenGLContext clearCurrentContext];
	}
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// /see sfWindowImpl::UseVerticalSync
////////////////////////////////////////////////////////////
void WindowImplCocoa::UseVerticalSync(bool Enabled)
{
	LOCAL_POOL_START;
	
	GLint enable = (Enabled) ? 1 : 0;
	[glContext setValues:&enable forParameter:NSOpenGLCPSwapInterval];
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// /see sfWindowImpl::ShowMouseCursor
////////////////////////////////////////////////////////////
void WindowImplCocoa::ShowMouseCursor(bool Show)
{
	LOCAL_POOL_START;
	
	if (Show) {
		[NSCursor unhide];
	} else {
		[NSCursor hide];
	}
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// /see sfWindowImpl::SetCursorPosition
////////////////////////////////////////////////////////////
void WindowImplCocoa::SetCursorPosition(unsigned int Left, unsigned int Top)
{
	LOCAL_POOL_START;
	
    // Change the cursor position (Left and Top are relative to this window)
	NSPoint pos = NSMakePoint ((float) Left, (float) Top);
	pos = [windowHandle convertScreenToBase:pos];
	pos.y = [windowHandle frame].size.height - pos.y;
	
	// Warning: not tested
	CGWarpMouseCursorPosition (CGPointMake (pos.x, pos.y));
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// /see sfWindowImpl::SetPosition
////////////////////////////////////////////////////////////
void WindowImplCocoa::SetPosition(int Left, int Top)
{
	LOCAL_POOL_START;
	
    // Change the window position
	Top = (int) [[windowHandle screen] frame].size.height - Top;
	[windowHandle setFrameTopLeftPoint:NSMakePoint(Left, Top)];
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// /see sfWindowImpl::Show
////////////////////////////////////////////////////////////
void WindowImplCocoa::Show(bool State)
{
	LOCAL_POOL_START;
	
	if (State) {
		[windowHandle makeKeyAndOrderFront:nil];
	} else {
		[windowHandle close];
	}
	
	LOCAL_POOL_END;
}

////////////////////////////////////////////////////////////
/// /see sfWindowImpl::EnableKeyRepeat
////////////////////////////////////////////////////////////
void WindowImplCocoa::EnableKeyRepeat(bool Enabled)
{
	useKeyRepeat = Enabled;
}


////////////////////////////////////////////////////////////
/// Register our NSApplication and launch it if needed
////////////////////////////////////////////////////////////
void WindowImplCocoa::RunApplication(void)
{
	LOCAL_POOL_START;
	
	if ([NSApp isRunning]) {
		LOCAL_POOL_END;
		return;
	}
	
	// We want our application to appear in the Dock and be able
	// to get focus
	ProcessSerialNumber psn;
	
    if (!GetCurrentProcess(&psn)) {
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        SetFrontProcess(&psn);
    }
	
    if (NSApp == nil) {
        if (nil == [NSApplication sharedApplication]) {
			error(__FILE__, __LINE__, "failed to make application instance");
		}
    }
	
	[NSApp finishLaunching];
    [NSApp setRunning];
	
	LOCAL_POOL_END;
}

////////////////////////////////////////////////////////////
/// Terminate the current running application
////////////////////////////////////////////////////////////
void WindowImplCocoa::QuitApplication(void)
{
	// Let the user choose if he has anything else to do after
	// application end (that's why we don't use -[NSApp terminate:])
	[NSApp makeWindowsPerform:@selector(close) inOrder:NO];
}

////////////////////////////////////////////////////////////
/// Make the OpenGL pixel format from the given attributes
////////////////////////////////////////////////////////////
NSOpenGLPixelFormat *WindowImplCocoa::NewGLPixelFormat(WindowSettings& params, bool fullscreen)
{
	LOCAL_POOL_START;
	
	NSOpenGLPixelFormat *pixFormat = nil;
	unsigned idx = 0, samplesIdx = 0;
	
	// Attributes list
	NSOpenGLPixelFormatAttribute attribs[15] = {(NSOpenGLPixelFormatAttribute) 0};
	
	attribs[idx++] = NSOpenGLPFAClosestPolicy;
	attribs[idx++] = NSOpenGLPFADoubleBuffer;
	attribs[idx++] = NSOpenGLPFAAccelerated;
	
	if (fullscreen) {
		attribs[idx++] = NSOpenGLPFAFullScreen;
	} else {
		attribs[idx++] = NSOpenGLPFAWindow;
	}
	
	attribs[idx++] = NSOpenGLPFAColorSize;
	attribs[idx++] = (NSOpenGLPixelFormatAttribute) VideoMode::GetDesktopMode().BitsPerPixel;
	
	attribs[idx++] = NSOpenGLPFADepthSize;
	attribs[idx++] = (NSOpenGLPixelFormatAttribute) params.DepthBits;
	
	attribs[idx++] = NSOpenGLPFAStencilSize;
	attribs[idx++] = (NSOpenGLPixelFormatAttribute) params.StencilBits;
	
	if (params.AntialiasingLevel) {
		samplesIdx = idx;
		
		attribs[idx++] = NSOpenGLPFASamples;
		attribs[idx++] = (NSOpenGLPixelFormatAttribute) params.AntialiasingLevel;
		
		attribs[idx++] = NSOpenGLPFASampleBuffers;
		attribs[idx++] = (NSOpenGLPixelFormatAttribute) GL_TRUE;
	}
	
	pixFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	
	// -- Ceylo --
	// If pixel format creation fails and antialiasing level is
	// greater than 2, we set it to 2.
	if (pixFormat == nil && params.AntialiasingLevel > 2) {
		std::cerr << "Failed to find a pixel format supporting " << params.AntialiasingLevel << " antialiasing levels ; trying with 2 levels" << std::endl;
		params.AntialiasingLevel = attribs[samplesIdx + 1] = (NSOpenGLPixelFormatAttribute) 2;
		
		pixFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	}
	
	// -- Ceylo --
	// If pixel format creation fails and antialiasing is enabled,
	// we disable it.
	if (pixFormat == nil && params.AntialiasingLevel > 0) {
		std::cerr << "Failed to find a pixel format supporting antialiasing ; antialiasing will be disabled" << std::endl;
		attribs[samplesIdx] = (NSOpenGLPixelFormatAttribute) nil;
		
		pixFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	}
	
	PRNT (__FILE__, __LINE__, pixFormat, "pixFormat");
	
#ifdef SFML_OSX_DEBUG
	for (unsigned i = 0;attribs[i];i++) {
		printf("attrib[%d] is %d\n", i, attribs[i]);
	}
#endif
	
	LOCAL_POOL_END;
	
	return [pixFormat autorelease];
}


////////////////////////////////////////////////////////////
/// Make the OpenGL context, the OpenGL view and add it to the window
////////////////////////////////////////////////////////////
void WindowImplCocoa::AddOpenGLView (WindowSettings& params)
{
	LOCAL_POOL_START;
#ifdef SFML_OSX_DEBUG
	[NSAutoreleasePool showPools];
#endif
	
	if (windowHandle != nil) {
#ifdef SFML_OSX_DEBUG
		PRNT (__FILE__, __LINE__, windowHandle, "windowHandle");
#endif
		
		NSOpenGLPixelFormat *pixFormat = nil;
		
		// Create an OpenGL context in this window and make it active
		pixFormat = NewGLPixelFormat(params, WindowedContext);
		
#ifdef SFML_OSX_DEBUG
		PRNT (__FILE__, __LINE__, pixFormat, "pixFormat");
#endif
		
		if (pixFormat != nil) {
			// We make the OpenGL context from our pixel format and use shared context
			glContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat
												   shareContext:[NSOpenGLContext currentContext]];
			
#ifdef SFML_OSX_DEBUG
			PRNT (__FILE__, __LINE__, glContext, "glContext");
#endif
			
			if (glContext != nil) {
				NSRect frame = NSMakeRect (0.0f, 0.0f, (float) myWidth, (float) myHeight);
				
				// And we set the myDepthBits and myStencilBits members to the correct values
				// (we grab values from the pixel format to be sure these are exactly the good one)
				CGLContextObj cglContext = (CGLContextObj) [glContext CGLContextObj];
				CGLPixelFormatObj cglPixFormat = (CGLPixelFormatObj) [pixFormat CGLPixelFormatObj];
				CGLError cglErr = (CGLError) 0;
				GLint screenID = 0;
				GLint tmpColorSize = 0, tmpDepthSize = 0, tmpStencilBits = 0, tmpAntialiasingLevel = 0;
				
				if (cglErr = CGLGetVirtualScreen(cglContext, &screenID)) {
					error(__FILE__, __LINE__, "CGLGetVirtualScreen() returned %d (%s)",
						  cglErr, CGLErrorString(cglErr));
				} else {
					if ((cglErr = CGLDescribePixelFormat(cglPixFormat, screenID, kCGLPFAColorSize, &tmpColorSize)) ||
						(cglErr = CGLDescribePixelFormat(cglPixFormat, screenID, kCGLPFADepthSize, &tmpDepthSize)) ||
						(cglErr = CGLDescribePixelFormat(cglPixFormat, screenID, kCGLPFAStencilSize, &tmpStencilBits)) ||
						(cglErr = CGLDescribePixelFormat(cglPixFormat, screenID, kCGLPFASamples, &tmpAntialiasingLevel))) {
						error(__FILE__, __LINE__, "CGLDescribePixelFormat() returned %d (%s)",
							  cglErr, CGLErrorString(cglErr));
					}
				}
				
				params.DepthBits = (unsigned) tmpDepthSize;
				params.StencilBits = (unsigned) tmpStencilBits;
				params.AntialiasingLevel = (unsigned) tmpAntialiasingLevel;
				
#ifdef SFML_OSX_DEBUG
				// Some information
				std::cout << "Using :" << std::endl;
				std::cout << "  Color depth : " << tmpColorSize << " bits" << std::endl;
				std::cout << "  Z-buffer size : " << tmpDepthSize << " bits" << std::endl;
				std::cout << "  Stencil size : " << tmpStencilBits << " bits" << std::endl;
				std::cout << "  Antialiasing level : " << tmpAntialiasingLevel << std::endl;
#endif
				
				// We make the NSOpenGLView
				glView = [[NSOpenGLView alloc] initWithFrame:frame];
				
#ifdef SFML_OSX_DEBUG
				PRNT (__FILE__, __LINE__, glView, "glView");
#endif
				
				if (glView != nil) {
					// We add the NSOpenGLView to the window
#warning Crash here >>
					[[windowHandle contentView] addSubview:glView];
					[glView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
					[glView setOpenGLContext:glContext];
					[glContext setView:glView];
					
					// We need to update the OpenGL view when it changes
					NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
					[nc addObserver:controller
						   selector:@selector(viewFrameDidChange:)
							   name:NSViewFrameDidChangeNotification
							 object:glView];
					
					[nc addObserver:controller
						   selector:@selector(windowDidBecomeMain:)
							   name:NSWindowDidBecomeMainNotification
							 object:windowHandle];
					
					[nc addObserver:controller
						   selector:@selector(windowDidResignMain:)
							   name:NSWindowDidResignMainNotification
							 object:windowHandle];
					
					[nc addObserver:controller
						   selector:@selector(windowWillClose:)
							   name:NSWindowWillCloseNotification
							 object:windowHandle];
					
					// Needed not to make application crash when releasing the window in our destructor
					[windowHandle setReleasedWhenClosed:NO];
				} else {
					error(__FILE__, __LINE__, "failed to make view");
				}
			} else {
				error(__FILE__, __LINE__, "failed to make context");
			}
		} else {
			error(__FILE__, __LINE__, "failed to make pixFormat");
		}
	} else {
		error(__FILE__, __LINE__, "NULL window handle");
	}
	
	LOCAL_POOL_END;
}


////////////////////////////////////////////////////////////
/// Return the SFML key corresponding to a key code
////////////////////////////////////////////////////////////
Key::Code WindowImplCocoa::KeyForVirtualCode(unsigned short vCode)
{
    switch (vCode)
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

typedef struct {
	unsigned short character;
	Key::Code sfKey;
} Character;

static Character unicodeTable[] =
{
	{'!', Key::Code(0)}, //< No Key for this code
	{'"', Key::Code(0)}, //< No Key for this code
	{'#', Key::Code(0)}, //< No Key for this code
	{'$', Key::Code(0)}, //< No Key for this code
	{'%', Key::Code(0)}, //< No Key for this code
	{'&', Key::Code(0)}, //< No Key for this code
	{'\'', Key::Quote},
	{'(', Key::Code(0)}, //< No Key for this code
	{')', Key::Code(0)}, //< No Key for this code
	{'*', Key::Multiply},
	{'+', Key::Add},
	{',', Key::Comma},
	//	{'-', Key::Dash},    //< Handled by KeyForVirtualCode()
	{'.', Key::Period},
	//	{'/', Key::Slash},   //< Handled by KeyForVirtualCode()
	//	{'0', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'1', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'2', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'3', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'4', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'5', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'6', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'7', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'8', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	//	{'9', Key::Code(0)}, //< Handled by KeyForVirtualCode()
	{':', Key::Code(0)}, //< No Key for this code
	{';', Key::SemiColon},
	{'<', Key::Code(0)}, //< No Key for this code
	{'=', Key::Equal},
	{'>', Key::Code(0)}, //< No Key for this code
	{'?', Key::Code(0)}, //< No Key for this code
	{'@', Key::Code(0)}, //< No Key for this code
	{'A', Key::A},
	{'B', Key::B},
	{'C', Key::C},
	{'D', Key::D},
	{'E', Key::E},
	{'F', Key::F},
	{'G', Key::G},
	{'H', Key::H},
	{'I', Key::I},
	{'J', Key::J},
	{'K', Key::K},
	{'L', Key::L},
	{'M', Key::M},
	{'N', Key::N},
	{'O', Key::O},
	{'P', Key::P},
	{'Q', Key::Q},
	{'R', Key::R},
	{'S', Key::S},
	{'T', Key::T},
	{'U', Key::U},
	{'V', Key::V},
	{'W', Key::W},
	{'X', Key::X},
	{'Y', Key::Y},
	{'Z', Key::Z},
	{'[', Key::LBracket},
	{'\\', Key::BackSlash},
	{']', Key::RBracket},
	{'^', Key::Code(0)}, //< No Key for this code
	{'_', Key::Code(0)}, //< No Key for this code
	{'`', Key::Code(0)}, //< No Key for this code
	{'a', Key::A},
	{'b', Key::B},
	{'c', Key::C},
	{'d', Key::D},
	{'e', Key::E},
	{'f', Key::F},
	{'g', Key::G},
	{'h', Key::H},
	{'i', Key::I},
	{'j', Key::J},
	{'k', Key::K},
	{'l', Key::L},
	{'m', Key::M},
	{'n', Key::N},
	{'o', Key::O},
	{'p', Key::P},
	{'q', Key::Q},
	{'r', Key::R},
	{'s', Key::S},
	{'t', Key::T},
	{'u', Key::U},
	{'v', Key::V},
	{'w', Key::W},
	{'x', Key::X},
	{'y', Key::Y},
	{'z', Key::Z},
	{'{', Key::Code(0)}, //< No Key for this code
	{'|', Key::Code(0)}, //< No Key for this code
	{'}', Key::Code(0)}, //< No Key for this code
	{'~', Key::Tilde},
	{0, Key::Code(0)}
};

Key::Code WindowImplCocoa::KeyForUnicode(unsigned short uniCode)
{
	Key::Code result = Key::Code(0);
	
	for (unsigned i = 0;unicodeTable[i].character;i++) {
		if (unicodeTable[i].character == uniCode) {
			result = unicodeTable[i].sfKey;
			break;
		}
	}
	
	return result;
}

/*===========================================================
            STRATEGY FOR OPENGL CONTEXT CREATION

- If the requested level of anti-aliasing is not supported and is greater than 2, try with 2
  --> if level 2 fails, disable anti-aliasing
  --> it's important not to generate an error if anti-aliasing is not supported

- Use the best of all available pixel modes (with highest color, depth and stencil bits)

- Don't forget to initialize the myDepthBits and myStencilBits members of base class

- IMPORTANT : all OpenGL contexts must be shared (usually a call to xxxShareLists)

===========================================================*/


/*===========================================================
               STRATEGY FOR EVENT HANDLING

- Process any event matching with the ones in sf::Event::EventType
  --> Create a sf::Event, fill the members corresponding to the event type
  --> No need to handle joystick events, they are handled by WindowImpl::ProcessJoystickEvents
  --> Event::TextEntered must provide UTF-16 characters
      (see http://www.unicode.org/Public/PROGRAMS/CVTUTF/ for unicode conversions)
  --> Don't forget to process any destroy-like event (ie. when the window is destroyed externally)

- Use SendEvent function from base class to propagate the created events

===========================================================*/


} // namespace priv

} // namespace sf
