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
#import <SFML/Window/OSXCocoa/WindowController.h>
#import <SFML/Window/OSXCocoa/WindowImplCocoa.hpp>
#import <OpenGL/gl.h>
#import <iostream>

@implementation WindowController

////////////////////////////////////////////////////////////
/// Forbide use of WindowController without any linked WindowImplCocoa object
////////////////////////////////////////////////////////////
- (id)init
{
	std::cout << "*** -[WindowController init] -- initialization without any linked window is forbidden ; nil returned" << std::endl;
	[self autorelease];
	return nil;
}

////////////////////////////////////////////////////////////
/// Initialize a new WindowController object and link it
/// to the 'window' object.
////////////////////////////////////////////////////////////
- (id)initWithWindow:(void *)window
{
	if (window == NULL) {
		std::cout << "*** -[WindowController initWithWindow:NULL] -- initialization without any linked window is forbidden ; nil returned" << std::endl;
		[self autorelease];
		return nil;
	}
	
	self = [super init];
	
	if (self != nil) {
		parentWindow = window;
	}
	
	return self;
}

////////////////////////////////////////////////////////////
/// Return a new autoreleased WindowController object linked
/// to the 'window' WindowImplCocoa object.
////////////////////////////////////////////////////////////
+ (id)controllerWithWindow:(void *)window
{
	return [[(WindowController *)[WindowController alloc] initWithWindow:window] autorelease];
}

////////////////////////////////////////////////////////////
/// Send event to the linked window
////////////////////////////////////////////////////////////
- (void)pushEvent:(sf::Event)sfEvent
{
	((sf::priv::WindowImplCocoa *)parentWindow)->HandleNotifiedEvent(sfEvent);
}

////////////////////////////////////////////////////////////
/// Notification method receiver when OpenGL view size changes
////////////////////////////////////////////////////////////
- (void)viewFrameDidChange:(NSNotification *)notification
{
	NSOpenGLView *glView = [notification object];
	[[glView openGLContext] update];
	
	sf::Event ev;
	ev.Type = sf::Event::Resized;
	ev.Size.Width = (unsigned) [glView frame].size.width;
	ev.Size.Height = (unsigned) [glView frame].size.height;
	
	[self pushEvent:ev];
}

////////////////////////////////////////////////////////////
/// Notification method receiver when the window gains focus
////////////////////////////////////////////////////////////
- (void)windowDidBecomeMain:(NSNotification *)notification
{
	sf::Event ev;
	ev.Type = sf::Event::GainedFocus;
	
	[self pushEvent:ev];
}

////////////////////////////////////////////////////////////
/// Notification method receiver when the window loses focus
////////////////////////////////////////////////////////////
- (void)windowDidResignMain:(NSNotification *)notification
{
	sf::Event ev;
	ev.Type = sf::Event::LostFocus;
	
	[self pushEvent:ev];
}

////////////////////////////////////////////////////////////
/// Notification method receiver when the window closes
////////////////////////////////////////////////////////////
- (void)windowWillClose:(NSNotification *)notification
{
	sf::Event ev;
	ev.Type = sf::Event::Closed;
	
	[self pushEvent:ev];
}

@end

