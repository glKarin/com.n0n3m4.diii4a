#!/usr/bin/env python

import sys, shutil, os, re

MODVERSION_FILE='code/qcommon/q_shared.h'
INFO_PATH='binaries/Quake3-UrT.app/Contents/Info.plist'
ENGINE_PATH='binaries/Quake3-UrT.app/Contents/MacOS'

if ( __name__ == '__main__' ):
    build_dir = sys.argv[1]
    engine_exe = sys.argv[2].lstrip('/')
    engine_path = os.path.join( build_dir, engine_exe )
    engine_name = os.path.basename( engine_exe )
    print( '%s -> %s' % ( repr( engine_path ), repr( ENGINE_PATH ) ) )
    shutil.copy( engine_path, ENGINE_PATH )

    l = filter( lambda l : l.find( '#define Q3_VERSION ' ) == 0, file( MODVERSION_FILE ).readlines() )[0]
    version = re.match( '.*"(.*)"', l ).groups()[0]

    info_plist = file( INFO_PATH, 'w' )
    info_plist.write( '''
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>%s</string>
	<key>CFBundleGetInfoString</key>
	<string>%s</string>
	<key>NSHumanReadableCopyright</key>
	<string></string>
	<key>CFBundleIconFile</key>
	<string>quake3-urt.icns</string>
	<key>CFBundleIdentifier</key>
	<string>com.frozensand.quake3-urt</string>
	<key>CFBundleName</key>
	<string>Quake3-UrT</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleSignature</key>
	<string>Q3URT</string>
	<key>CFBundleShortVersionString</key>
	<string>%s</string>
	<key>CFBundleVersion</key>
	<string>%s</string>
	<key>NSExtensions</key>
	<dict/>
	<key>NSPrincipalClass</key>
	<string>NSApplication</string>
</dict>
</plist>
''' % ( engine_name, version, version, version ) )

    print( 'OSX binary is ready: %s' % version )
