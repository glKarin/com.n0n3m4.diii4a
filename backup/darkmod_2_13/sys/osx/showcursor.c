/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
/*
 cc -o showcursor showcursor.c -framework IOKit
*/

#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <assert.h>

mach_port_t	masterPort;

io_connect_t OpenEventDriver( void )
{
    register kern_return_t	kr;
    mach_port_t		ev, service, iter;

    kr = IOServiceGetMatchingServices( masterPort, IOServiceMatching( kIOHIDSystemClass ), &iter);
    assert( KERN_SUCCESS == kr);

    service = IOIteratorNext( iter );
    assert(service);

    kr = IOServiceOpen( service, mach_task_self(), kIOHIDParamConnectType, &ev);
    assert( KERN_SUCCESS == kr );

    IOObjectRelease( service );
    IOObjectRelease( iter );

    return( ev );
}


void TestParams( io_connect_t ev, boolean_t show )
{
    kern_return_t	kr;

    kr = IOHIDSetCursorEnable( ev, show );
    assert(KERN_SUCCESS == kr);
}

int main(int argc, char **argv)
{
    kern_return_t		kr;
    boolean_t show;

    if (argc != 2)
        show = 1;
    else
        show = (atoi(argv[0]) != 0);

    assert( KERN_SUCCESS == ( kr = IOMasterPort( bootstrap_port, &masterPort) ));
    TestParams( OpenEventDriver(), show);

    return( 0 );
}
