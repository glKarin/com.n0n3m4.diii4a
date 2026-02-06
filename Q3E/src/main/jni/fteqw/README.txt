Yay, you found out the secrit location to download the sauce code from!

Right, urm, now what?
Yeah, good question.

Urm.



Quick start guide:
cd engine
make sv-rel -j4
make gl-rel -j4
cd  ..
engine/release/fteqw.sv -nohome -basedir ~/quake
engine/release/fteqw.gl -nohome -basedir ~/quake

You do not need to configure. The makefile will automatically do that depending on the target build/system.



Easy Build Bot System:
If you want to set up a linux box that cross-compiles each target with your own private customisations, then you can run the build_setup.sh script to set up which targets you wish to support.
You can then just run the build_wip.sh script any time your code changes to have it rebuild every target you previously picked.
The script can also be run from cygwin, but does not support compiling for linux then.
(The setup script will install android+emscripten dependancies for you, so you're likely to find this an easier way to deal with those special targets).
(note that the android sdk can be a big download, while installing emscripten may require several hours to compile clang and about 40gb of disk space if emscripten doesn't provide prebuilt stuff for your distro).




To compile the FTEDroid port with cygwin:
make droid-rel PATH=C:\Cygwin\bin\ DROID_SDK_PATH=/cygdrive/c/Games/tools/android-sdk DROID_NDK_PATH=/cygdrive/c/Games/tools/android-ndk-r7 ANT=/cygdrive/c/Games/tools/apache-ant-1.8.2/bin/ant JAVATOOL="/cygdrive/c/Program\ Files/Java/jdk1.7.0_02/bin/" DROID_ARCH="armeabi x86" -j4 DROID_PACKSU=/cygdrive/c/games/quake/id1/pak0.pak
On linux you can omit the PATH, ANT, and JAVATOOL parts as they should already be in the path (yes I copied the above out of a batch file).
Then install the release/FTEDroid.apk file on your android device.
The DROID_PACKSU part is used to include the pak file within the android package. Ideally you would use a pk3 file instead. Also you would use something that will not violate iD software's copyright. THIS IS AN EXAMPLE ONLY. You can omit the setting entirely if you require the user to provide their own packages.
Note that there is no way to install the package with a different name at this time.



Browser versions of FTE:
The FTE browser plugin is available only in windows.
Compile with 'make npfte-rel FTE_TARGET=win32'.
This will yield an npfte.dll file.
You can 'register' this dll by running 'regsvr32 npfte.dll' at a command prompt (this is the standard way to register an activex control - any setup software should provide some mechanism to do this at install time). This will register both the netscape/firefox/chrome/opera version, and the activex/IE version of the plugin.
Note that the plugin will run the engine in a separate process and thus requires a valid fteqw.exe file in the same directory as the plugin.
If given an 'npfte.txt' file that contains the line 'relexe foo', the plugin will try to run foo.exe instead of fteqw, basedir "foo" can be used to invoke it with a different basedir. You can use this if you'd rather run the mingl or gl-only version, or if you'd like to retarget npfte to invoke a different quake engine intead. Note that different quake engines will need to support the -plugin argument and the stdin/stdout parsing for embedding - at the time of writing, no others do.
The following chunk of html can then be included on a web page to embed it. Yes. Two nested objects.
<object	name="ieplug" type="text/x-quaketvident" classid="clsid:7d676c9f-fb84-40b6-b3ff-e10831557eeb" width=100% height=100% ><param name="splash" value="http://127.0.0.1:27599/qtvsplash.jpg"><param name="game" value="q1"><param name="dataDownload" value=''><object	name="npplug" type="text/x-quaketvident" width=100% height=100% ><param name="splash" value="http://127.0.0.1:27599/qtvsplash.jpg"><param name="game" value="q1"><param name="dataDownload" value=''>Plugin failed to load</object></object>


This stuff has separate directories
engine: FTEQW game engine itself. Both client and dedicated server.
engine/ftequake: location of old msvc6 project file. Might not work.
engine/dotnet2005: location of microsoft visual c 2005 project file. Most likely to be up to date, but also most likely to be specific to Spike's config.
engine/dotnet2010: location of microsoft visual c 2010 (express) project file. Might not work.
engine/release: the makefile writes its release-build binaries here. Intermediate files are contained within a sub-directory.
engine/debug: the makefile writes its debug-build binaries here. Intermediate files are contained within a sub-directory.
fteqtv: the qtv proxy server program.
plugins: several optional plugins that do various interesting things, though not so interesting
q3asm2: my quick hack at a qvm assembler which is not horribly slow. ignore it.
quakec: Various quakec mods. Some interesting, some not.
quakec/basemod: TimeServ's attempt to bugfix and modify vanilla quake
quakec/csaddon: ingame csqc-controlled editors. Currently contains the camquake featureset (thanks Jogi), rtlights editor, terrain editor ui, particle editor.
quakec/csqctest: my csqc sample mod. Originally created as a feature testbed for the csqc api. Useful as a reference/sample, but you perhaps don't want to use it as a base.
specs: modder/advanced documentation and samples.



Interesting commandline arguments:
-nohome         disables the use of home directories. all file access will occur within the gamedir.
-basedir $FOO   tells the engine where to find the game.
-game $FOO      traditional NQ way to specify a mod.
-mem $FOO       may still be needed on linux, at least for now. Not required on windows. Value is in MB.
+sv_public 0    stop the server from reporting to master servers.
-window         override config files to force fte into windowed mode at startup. Can still be made fullscreen via the menus or console.



NQ compat cvars:
sv_port 26000 (fte listens on port 27500 by default, as its derived from quakeworld).
cl_defaultport 26000 (see above. you can always specify the port in the connect command).
sv_nomsec 1 (disables player prediction, giving authentic nq player physics).
sv_listen_nq 1 (if 0, disables NQ clients. if 1, allows qsmurfing. if 2(default), blocks qsmurfing but may have compat issues with some clients).



Limit breaking stuff:
pr_maxedicts 32767 (max ent limit)
sv_bigcoords 1	(expands coord sizes, but has compat issues)



Hexen2 / Quake2 / Quake3:
Copy the engine to your h2/q2/q3 dir, and run from there.
Alternatively, specify the h2/q2/q3 dir via -basedir.
The engine will autodetect the game from its basedir, and reconfigure as appropriate if appropriate.


Problems?
You can report problems on IRC.
Join irc.quakenet.org #fte

If you refuse to use IRC, you can instead report issues to Spike via the inside3d.com forums, or the quakeone.com forums, depending on where you're able to create an account.
Bug reports are (generally) always apreciated! :)
