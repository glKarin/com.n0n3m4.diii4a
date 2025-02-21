/*
 
 Copyright (C) 2001-2002       A Nourai
 Copyright (C) 2006            Jacek Piszczek (Mac OSX port)
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 
 See the included (GNU.txt) GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>
#include "quakedef.h"

id _p;
int evcnt = 2;

// Jacek: some keys are bogus, my ibook kb lacks keys and apple's docs lack
// keyCode documentation

unsigned char keyconv[] =
{
	'a', /* 0 */
	's',
	'd',
	'f',
	'h',
	'g',
	'z',
	'x',
	'c',
	'v',
	'0', /* 10 */
	'b',
	'q',
	'w',
	'e',
	'r',
	'y',
	't',
	'1',
	'2',
	'3', /* 20 */
	'4',
	'6',
	'5',
	'=',
	'9',
	'7',
	'-',
	'8',
	'0',
	']', /* 30 */
	'o',
	'u',
	'[',
	'i',
	'p',
	K_ENTER,
	'l',
	'j',
	'\'',
	'k', /* 40 */
	';',
	'\\',
	',',
	'/',
	'n',
	'm',
	'.',
	K_TAB,
	K_SPACE,
	'`', /* 50 */
	K_BACKSPACE,
	'v',
	K_ESCAPE,
	'n',
	'm',
	',',
	'.',
	'/',
	0,
	K_KP_DEL, /* 60 */
	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	' ',
	K_KP_DEL,
	K_TAB,
	K_KP_STAR,
	K_ENTER,
	K_KP_PLUS,
	K_DEL, /* 70 */
	K_INS,
	K_PGUP,
	K_PGDN,
	K_KP_MINUS,
	K_KP_SLASH,
	K_KP_ENTER,
	0,
	K_KP_MINUS,
	0,
	K_F1, /* 80 */
	K_F2,
	K_KP_INS,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_HOME,
	0, /* 90 */
	K_KP_UPARROW,
	K_KP_PGUP,
	0,
	K_KP_PLUS,
	0,
	K_LSHIFT,
	K_RSHIFT,
	0,
	K_RCTRL,
	K_ALT, /* 100 */
	K_ALT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	K_PAUSE, /* 110 */
	K_F12,
	0,
	0,
	0,
	K_HOME,
	K_PGUP,
	K_DEL,
	0,
	K_END,
	0, /* 120 */
	K_PGDN,
	0,
	K_LEFTARROW,
	K_RIGHTARROW,
	K_DOWNARROW,
	K_UPARROW,
	0,
	0,
	0,
	0, /* 130 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 140 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 150 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 160 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 170 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 180 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 190 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 200 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 210 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 220 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 230 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 240 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 250 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

// validate the depth
int checkDepth(int d)
{
	if (d == 24)
		 d = 32;
	if (d != 15 && d != 16 && d != 32)
		 d = 32;
    
	return d;
}

@interface FTEApplication : NSApplication
{
	NSOpenGLContext    *_openGLContext;
	NSTimer            *_timer;
	double time, oldtime, newtime;
	unsigned int        oldmflags;
	CFDictionaryRef     olddmode;
}
- (void)initDisplayWidth:(int)width height:(int)height depth:(int)depth;
- (void)flushBuffer;
- (void)runLoop:(NSTimer *)timer;
@end

@implementation FTEApplication

- (id)init
{
	if((self = [super init]))
		[self setDelegate:self];
	
	oldmflags = 0;
    
	return self;
}

- (void)initDisplayWidth:(int)width height:(int)height depth:(int)depth;
{
	long                            value = 1;
	NSOpenGLPixelFormat*            format;
	NSOpenGLPixelFormatAttribute    attributes[] = {
		NSOpenGLPFAFullScreen,
		NSOpenGLPFAScreenMask,
		CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay),
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADepthSize, checkDepth(depth),
		0};

	olddmode = CGDisplayCurrentMode(kCGDirectMainDisplay);

	// zeros mean we use the default screen! (but with 32bit depth)
	if (!((width == 0) && (height == 0) && (depth == 0)))
	{
		depth = checkDepth(depth);
		if (width == 0)
			 width = CGDisplayPixelsWide(kCGDirectMainDisplay);
		if (height == 0)
			 height = CGDisplayPixelsHigh(kCGDirectMainDisplay);
		CFDictionaryRef dmode = CGDisplayBestModeForParameters(
			kCGDirectMainDisplay,
			checkDepth(depth),
			width,
			height,
			FALSE);
		CGDisplaySwitchToMode(kCGDirectMainDisplay,dmode);
	}

	// get screen size
	vid.pixelwidth = CGDisplayPixelsWide(kCGDirectMainDisplay);
	vid.pixelheight = CGDisplayPixelsHigh(kCGDirectMainDisplay);

	// capture the display!
	CGDisplayCapture(kCGDirectMainDisplay);
	CGDisplayHideCursor(kCGDirectMainDisplay);
	CGAssociateMouseAndMouseCursorPosition(false);

	format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	_openGLContext = [[NSOpenGLContext alloc] 
			initWithFormat:format 
			shareContext:nil];
	[format release];
	if(_openGLContext == nil)
	{
		NSLog(@"Cannot create OpenGL context");
		[NSApp terminate:nil];
		return;
	}
    
	[_openGLContext setFullScreen];
	[_openGLContext setValues:&value forParameter:kCGLCPSwapInterval];

	[_openGLContext makeCurrentContext];
    
	_timer = [[NSTimer scheduledTimerWithTimeInterval:1.0/250.0
		target:self 
		selector:@selector(runLoop:) 
		userInfo:nil 
		repeats:YES] 
		retain];
        
	[[NSApp mainWindow] setAcceptsMouseMovedEvents:YES];
}

- (void)dealloc
{
	CGAssociateMouseAndMouseCursorPosition(true);
	CGDisplayRelease(kCGDirectMainDisplay);
	CGDisplayRestoreColorSyncSettings();
	CGDisplaySwitchToMode(kCGDirectMainDisplay,olddmode);
	CGDisplayMoveCursorToPoint(kCGDirectMainDisplay,
                               CGPointMake(vid.width/2,vid.height/2));
	[_openGLContext release];
	[_timer invalidate];
	[_timer release];
    
	[super dealloc];
}

- (void) sendEvent:(NSEvent*)event
{
	if ([event type] == NSKeyDown)
	{
		int code = keyconv[[event keyCode]];
		Key_Event(0, code, code>=128?0:code, TRUE);
		//printf("%d\n",[event keyCode]);
		return;
	}

	if ([event type] == NSKeyUp)
	{
		int code = keyconv[[event keyCode]];
		Key_Event(0, code, 0, FALSE);
		return;
	}

	if ([event type] == NSFlagsChanged)
	{
		unsigned int mflags = [event modifierFlags];

		if ((mflags & NSAlternateKeyMask) ^ (oldmflags & NSAlternateKeyMask))
		{
			Key_Event(0, K_ALT, 0, (mflags & NSAlternateKeyMask) ? TRUE : FALSE);
		}

		if ((mflags & NSControlKeyMask) ^ (oldmflags & NSControlKeyMask))
		{
			Key_Event(0, K_LCTRL, 0, (mflags & NSControlKeyMask) ? TRUE : FALSE);
		}
        
		if ((mflags & NSShiftKeyMask) ^ (oldmflags & NSShiftKeyMask))
		{
			Key_Event(0, K_LSHIFT, 0, (mflags & NSShiftKeyMask) ? TRUE : FALSE);
		}

		if ((mflags & NSCommandKeyMask) ^ (oldmflags & NSCommandKeyMask))
		{
			Key_Event(0, K_LWIN, 0, (mflags & NSCommandKeyMask) ? TRUE : FALSE);
		}

		if ((mflags & NSAlphaShiftKeyMask) ^ (oldmflags & NSAlphaShiftKeyMask))
		{
			Key_Event(0, K_CAPSLOCK, 0, (mflags & NSAlphaShiftKeyMask) ? TRUE : FALSE);
		}

		oldmflags = mflags;
		return;
	}

	if ([event type] == NSMouseMoved)
	{
		IN_MouseMove(0, false, [event deltaX], [event deltaY], 0, 0);

		// lame hack to avoid mouse ptr moving to the top of the screen since
		// a click there causes the mouse to appear and lock the event stream
		// Apple sucks :(
		// NOTE: it seems this is still needed for 10.3.x!
		CGDisplayMoveCursorToPoint(kCGDirectMainDisplay,
			CGPointMake(vid.width - 1,vid.height - 1));
		return;
	}

	if ([event type] == NSLeftMouseDragged)
	{
		IN_MouseMove(0, false, [event deltaX], [event deltaY], 0, 0);
		CGDisplayMoveCursorToPoint(kCGDirectMainDisplay,
			CGPointMake(vid.width - 1,vid.height - 1));
		return;
	}

	if ([event type] == NSRightMouseDragged)
	{
		IN_MouseMove(0, false, [event deltaX], [event deltaY], 0, 0);
		CGDisplayMoveCursorToPoint(kCGDirectMainDisplay,
			CGPointMake(vid.width - 1,vid.height - 1));
		return;
	}

	if ([event type] == NSOtherMouseDragged)
	{
		IN_MouseMove(0, false, [event deltaX], [event deltaY], 0, 0);
		CGDisplayMoveCursorToPoint(kCGDirectMainDisplay,
			CGPointMake(vid.width - 1,vid.height - 1));
		return;
	}

	if ([event type] == NSLeftMouseDown)
	{
		Key_Event(0, K_MOUSE1, 0, TRUE);
		return;
	}

	if ([event type] == NSLeftMouseUp)
	{
		Key_Event(0, K_MOUSE1, 0, FALSE);
		return;
	}

	if ([event type] == NSRightMouseDown)
	{
		Key_Event(0, K_MOUSE2, 0, TRUE);
		return;
	}

	if ([event type] == NSRightMouseUp)
	{
		Key_Event(0, K_MOUSE2, 0, FALSE);
		return;
	}

	if ([event type] == NSOtherMouseDown)
	{
		Key_Event(0, K_MOUSE3, 0, TRUE);
		return;
	}

	if ([event type] == NSOtherMouseUp)
	{
		Key_Event(0, K_MOUSE3, 0, FALSE);
		return;
	}
    
	if ([event type] == NSScrollWheel)
	{
		Key_Event(0, ([event deltaY] > 0.0) ? K_MWHEELUP : K_MWHEELDOWN, 0, TRUE);
		return;
	}
}    

- (void)flushBuffer
{
	// synchronise display
	[_openGLContext flushBuffer];
}

// called on a timer event
- (void)runLoop:(NSTimer *)timer
{
	newtime = Sys_DoubleTime ();
	time = newtime - oldtime;
	oldtime = newtime;
    
	Host_Frame(time);
}

- (void)run
{
	oldtime = Sys_DoubleTime ();
	[super run];
}

@end

static FTEApplication *fteglapp;

BOOL initCocoa(rendererstate_t *info)
{
	// init the application the hard way since we don't want to run it
	// immediately
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	fteglapp = [FTEApplication sharedApplication];

	// store the var for later disposal
	_p = pool;

	// init the display
	[fteglapp initDisplayWidth:info->width height:info->height depth:info->bpp];

	return TRUE;
}

qboolean glcocoaRunLoop(void)
{
	if (!fteglapp)
		return false;
	// this will initialise the NSTimer and run the app
	[NSApp run];

	return true;
}

void killCocoa(void)
{
	// terminates FTEApplicaiton
	[NSApp terminate:nil];
	[_p release];

	fteglapp = NULL;
}

void flushCocoa(void)
{
	// synchronises display
	[NSApp flushBuffer];
}

void cocoaGamma(unsigned short *r,unsigned short *g,unsigned short *b)
{
	uint8_t gammatable[3*256];
	int i;

	// convert the gamma values
	for(i=0;i<256;i++)
	{
		gammatable[i] = r[i] >> 8;
		gammatable[i+256] = g[i] >> 8;
		gammatable[i+512] = b[i] >> 8;
	}
    
	//... and set them
	CGSetDisplayTransferByByteTable(kCGDirectMainDisplay,256,
		gammatable,
		gammatable + 256,
		gammatable + 512);
}
