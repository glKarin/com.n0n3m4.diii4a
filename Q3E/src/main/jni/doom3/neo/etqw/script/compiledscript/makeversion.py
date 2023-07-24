#!/usr/bin/env python
#
# version management
#

import os
import sys
import re

def GetVersion():
    keys = ( 'ENGINE_VERSION_MAJOR', 'ENGINE_VERSION_MINOR', 'ENGINE_SRC_REVISION', 'ENGINE_MEDIA_REVISION' )
    version = [ '-1','-1','-1','-1' ]
    buildVersion = {}

    try:
        f = file( 'BuildVersion.cpp', 'r' )
    except IOError, (errno, strerror):
        return version

    for line in f.readlines():
        m = re.match( 'int\s+(\w+) = ([\-0-9]+);', line )

        if m:
            buildVersion[ m.group( 1 ) ] = m.group( 2 )

    f.close()

    i = 0
    for key in keys:
        if buildVersion.has_key( key ):
            version[ i ] = buildVersion[ key ]
        i = i + 1

    sys.stdout.write( "Version major: %s minor: %s src: %s media: %s\n" % ( version[0], version[1], version[2], version[3] ) )

    return version

def WriteRc( filename, modulename, engineVersionMajor, engineVersionMinor, srcVersion, mediaVersion ):
    try:
        f = file( filename + '.template', 'r' )
    except IOError, ( errno, strerror ):
        sys.stdout.write( 'Failed to open %s: %s\n' % ( filename, strerror ) )
        return False

    versionrc = f.readlines()
    f.close()

    valueDict = {
        'CompanyName' : 'Splash Damage, Ltd.',
        'FileDescription' : 'Enemy Territory: Quake Wars',
        'InternalName' : modulename,
        'LegalCopyright' : 'Copyright (C) 2007 Splash Damage, Ltd.',
        'OriginalFilename' : modulename,
        'ProductName' : 'Enemy Territory: Quake Wars',
    }

    try:
        f = file( filename, 'w' )
    except IOError, ( errno, strerror ):
        sys.stdout.write( 'Failed to open %s: %s\n' % ( filename, strerror ) )
        return False

    for line in versionrc:
        m = re.match( '^\s+FILEVERSION (\d+),(\d+),(\d+),(\d+)', line )
        if m:
            f.write( ' FILEVERSION %s,%s,%s,%s\n' % ( engineVersionMajor, engineVersionMinor, srcVersion, mediaVersion ) )
            continue

        m = re.match( '^\s+PRODUCTVERSION (\d+),(\d+),(\d+),(\d+)', line )
        if m:
            f.write( ' PRODUCTVERSION %s,%s,%s,%s\n' % ( engineVersionMajor, engineVersionMinor, srcVersion, mediaVersion ) )
            continue

        m = re.match( '^\s+VALUE \"(\S+)\", \"(.+)\"', line )
        if m:
            if m.group( 1 ) == 'FileVersion':
                f.write( '            VALUE "FileVersion", "%s.%s.%s.%s"\n' % ( engineVersionMajor, engineVersionMinor, srcVersion, mediaVersion ) )
                continue
            elif m.group( 1 ) == 'ProductVersion':
                f.write( '            VALUE "ProductVersion", "%s.%s.%s.%s"\n' % ( engineVersionMajor, engineVersionMinor, srcVersion, mediaVersion ) )
                continue
            elif valueDict.has_key( m.group( 1 ) ):
                f.write( '            VALUE "%s", "%s"\n' % (  m.group( 1 ), valueDict[ m.group( 1 ) ] ) )
                continue

        f.write( line )

    f.close()

    return True

def main():
    version = GetVersion()

    WriteRc( 'version.rc', 'compiledscriptx86.dll', version[0], version[1], version[2], version[3] )

# can be used as module or by direct call
if __name__ == '__main__':
    main()
