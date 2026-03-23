To make the final bundle rename Doom 3 to doom3 and place the d3go binary
besides doom3 (make sure the binary is compiled for 32bit).  Then rename
the Executable file from Doom 3 to d3go in the Info.plist file.

Also copy the game.dyld and game-d3xp.dyld files to Contents/Frameworks and
issue:

install_name_tool -change /usr/local/lib/game-d3xp.dylib @executable_path/../Frameworks/game.dylib Doom\ 3.app/Contents/MacOS/Doom\ 3 

and

install_name_tool -change /usr/local/lib/game.dylib @executable_path/../Frameworks/game.dylib Doom\ 3.app/Contents/MacOS/Doom\ 3 

