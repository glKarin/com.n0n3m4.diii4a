/*
** i_main.mm
**
**---------------------------------------------------------------------------
** Copyright 2021 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "tarray.h"
#include "zstring.h"

#include <cstdlib>
#include <SDL.h>

#import <AppKit/AppKit.h>

static TArray<FString> g_args;

#if __MAC_OS_X_VERSION_MAX_ALLOWED < 1060
@interface ApplicationDelegate : NSObject
#else
@interface ApplicationDelegate : NSObject<NSApplicationDelegate>
#endif
{
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
@end

@implementation ApplicationDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	TArray<char*> argv;
	for(unsigned int i = 0; i < g_args.Size(); ++i)
		argv.Push(g_args[i].LockBuffer());

	extern int WL_Main(int argc, char *argv[]);
	exit(WL_Main(argv.Size(), &argv[0]));
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
	return NSTerminateCancel;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	const char* cFilename = [filename UTF8String];

	// Might be duplicated
	for(unsigned int i = 0; i < g_args.Size(); ++i)
	{
		if(g_args[i].Compare(cFilename) == 0)
			return FALSE;
	}

	g_args.Push("--file");
	g_args.Push(cFilename);
	return TRUE;
}
@end

ApplicationDelegate *appDelegate;

#undef main
int main(int argc, char *argv[])
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	// Set LC_NUMERIC environment variable in case some library decides to
	// clear the setlocale call at least this will be correct.
	// Note that the LANG environment variable is overridden by LC_*
	setenv("LC_NUMERIC", "C", 1);

	for(int i = 0; i < argc; ++i)
		g_args.Push(argv[i]);

	[NSApplication sharedApplication];

	appDelegate = [ApplicationDelegate new];
	[NSApp setDelegate:appDelegate];
	[NSApp run];

	[pool release];
	return 0;
}
	//
