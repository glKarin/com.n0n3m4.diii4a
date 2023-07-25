Place the files from this directory into the base/src directory in your build 
products path, along with the other game data. For instance, on my machine,
I have:

Output/Enemy Territory QUAKE Wars.app
...
Output/base/game000.pk4
...
Output/base/src/compiledscript.so-Info.plist.base
Output/base/src/CompiledScript.xcodeproj/project.pbxproj.base

When exporting scripts from the game, the generated source and project files
will be in

~/Library/Application Support/ETQW/base/src

In order to build the compiled script bundle, you will need to inform XCode
of where your ETQW source base is. This is done using a Source Tree, called
ETQW_Source. It should point to the base of your Quake Wars source tree, so
that 

$ETQW_Source/game/script/compiledscript

is a valid path.

