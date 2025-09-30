                   ,---------------------------------------.
                   |   _                     _       ____  |
                   |  (_)___  __ _ _  _ __ _| |_____|__ /  |
                   |  | / _ \/ _` | || / _` | / / -_)|_ \  |
                   |  |_\___/\__, |\_,_\__,_|_\_\___|___/  |
                   |            |_|                        |
                   |                                       |
                   `---------- http://ioquake3.org --------'

The intent of this project is to provide a baseline Quake 3 which may be used
for further development. Some of the major features currently implemented are:

  * SDL backend for unix-like operating systems
  * OpenAL sound API support (multiple speaker support and better sound
    quality)
  * Full x86_64 support on Linux
  * MinGW compilation support on Windows and cross compilation support on Linux
  * AVI video capture of demos
  * Much improved console autocompletion
  * Persistent console history
  * Colorized terminal output
  * Optional Ogg Vorbis support
  * Much improved QVM tools
  * Support for various esoteric operating systems (see
    http://ioquake3.org/?page=status)
  * cl_guid support
  * HTTP/FTP download redirection (using cURL)
  * Multiuser support on Windows systems (user specific game data
    is stored in "%APPDATA%\Quake3")
  * PNG support
  * Fullscreen Alt-Tab support
  * Many, many bug fixes

The map editor and associated compiling tools are not included. We suggest you
use a modern copy.

The original id software readme that accompanied the Q3 source release has been
renamed to id-readme.txt so as to prevent confusion. Please refer to the
web-site for updated status.


--------------------------------------------- Compilation and installation -----

For *nix
  1. Change to the directory containing this readme.
  2. Run 'make'.

For Windows, using MinGW
  1. Download and install MinGW and MSys from http://www.mingw.org/.
  2. Download http://www.libsdl.org/extras/win32/common/directx-devel.tar.gz
     and untar it into your MinGW directory (usually C:\MinGW).
  3. Open an MSys terminal, and follow the instructions for compiling on *nix.

For Windows, using MSVC
  1. Run Visual Studio and open the quake3.sln file in the code/win32/msvc
     directory.
  2. Build.
  3. Copy the resultant Quake3.exe to your quake 3 directory, make a backup if
     you want to keep your original. If you wish to use native libraries, copy
     the resultant dlls to your baseq3 directory.

For Mac OS X, building a Universal Binary
  1. Install the MacOSX10.2.8.sdk and MacOSX10.4u.sdk which are included in
     XCode 2.2 and newer.
  2. Change to the directory containing this README file.
  3. Run './make-macosx-ub.sh'
  4. Copy the resulting ioquake3.app in /build/release-darwin-ub to your
     /Applications/ioquake3 folder.

Installation, for *nix
  1. Set the COPYDIR variable in the shell to be where you installed Quake 3
     to.  By default it will be /usr/local/games/quake3 if you haven't set it.
     This is the path as used by the original Linux Q3 installer and subsequent
     point releases.
  2. Run 'make copyfiles'.

It is also possible to cross compile for Windows under *nix using MinGW. A
script is available to build a cross compilation environment from
http://www.libsdl.org/extras/win32/cross/build-cross.sh. The gcc/binutils
version numbers that the script downloads may need to be altered.
Alternatively, your distribution may have mingw32 packages available. On
debian/Ubuntu, these are mingw32, mingw32-runtime and mingw32-binutils. Cross
compiling is simply a case of using './cross-make-mingw.sh' in place of 'make',
though you may find you need to change the value of the variables in this
script to match your environment.

If the make based build system is being used (i.e. *nix or MinGW), the
following variables may be set, either on the command line or in
Makefile.local:

  OPTIMIZE            - use this for custom CFLAGS
  V                   - set to show cc command line when building
  DEFAULT_BASEDIR     - extra path to search for baseq3 and such
  DEFAULT_LIBDIR      - extra path to search for libraries
  HOMEPATH            - alternative home directory
  BUILD_SERVER        - build the 'ioq3ded' server binary
  BUILD_CLIENT        - build the 'ioquake3' client binary
  BUILD_CLIENT_SMP    - build the 'ioquake3-smp' client binary
  BUILD_GAME_SO       - build the game shared libraries
  BUILD_GAME_QVM      - build the game qvms
  USE_SDL             - use the SDL backend where available
  USE_OPENAL          - use OpenAL where available
  USE_OPENAL_DLOPEN   - link with OpenAL at runtime
  USE_CURL            - use libcurl for http/ftp download support
  USE_CURL_DLOPEN     - link with libcurl at runtime
  USE_CODEC_VORBIS    - enable Ogg Vorbis support
  USE_LOCAL_HEADERS   - use headers local to ioq3 instead of system ones
  USE_CCACHE          - use ccache compiler caching tool
  USE_AUTH            - use the authentication system for Urban Terror 4.2
  USE_DEMO_FORMAT_42  - use the demo format used in Urban Terror 4.2
  COPYDIR             - the target installation directory

The defaults for these variables differ depending on the target platform.


------------------------------------------------------------------ Console -----

New cvars
  cl_autoRecordDemo                 - record a new demo on each map change
  cl_aviFrameRate                   - the framerate to use when capturing video
  cl_aviMotionJpeg                  - use the mjpeg codec when capturing video
  cl_mouseAccelStyle                - Set to 1 for QuakeLive mouse acceleration
                                      behaviour, 2 for UrbanTerror newest mode
                                      or set to 0 for standard q3 mode.
  cl_mouseAccelOffset               - Tuning the acceleration curve, see below

  s_useOpenAL                       - use the OpenAL sound backend if available
  s_alPrecache                      - cache OpenAL sounds before use
  s_alGain                          - the value of AL_GAIN for each source
  s_alSources                       - the total number of sources (memory) to
                                      allocate
  s_alDopplerFactor                 - the value passed to alDopplerFactor
  s_alDopplerSpeed                  - the value passed to alDopplerVelocity
  s_alMinDistance                   - the value of AL_REFERENCE_DISTANCE for
                                      each source
  s_alMaxDistance                   - the maximum distance before sounds start
                                      to become inaudible.
  s_alRolloff                       - the value of AL_ROLLOFF_FACTOR for each
                                      source
  s_alGraceDistance                 - after having passed MaxDistance, length
                                      until sounds are completely inaudible.
  s_alDriver                        - which OpenAL library to use
  s_alDevice                        - which OpenAL device to use
  s_alAvailableDevices              - list of available OpenAL devices

  s_sdlBits                         - SDL bit resolution
  s_sdlSpeed                        - SDL sample rate
  s_sdlChannels                     - SDL number of channels
  s_sdlDevSamps                     - SDL DMA buffer size override
  s_sdlMixSamps                     - SDL mix buffer size override

  ttycon_ansicolor                  - enable use of ANSI escape codes in the tty
  r_GLlibCoolDownMsec               - wait for some milliseconds to close GL
                                      library
  com_altivec                       - enable use of altivec on PowerPC systems
  s_backend                         - read only, indicates the current sound
                                      backend
  in_shiftedKeys                    - non-SDL Linux only; enables binding to
                                      shifted keys
  in_joystickNo                     - SDL only; select which joystick to use
  cl_consoleHistory                 - read only, stores the console history
  cl_platformSensitivity            - read only, indicates the mouse input
                                      scaling
  r_ext_texture_filter_anisotropic  - anisotropic texture filtering
  cl_guidServerUniq                 - makes cl_guid unique for each server
  cl_cURLLib                        - filename of cURL library to load
  sv_dlURL                          - the base of the HTTP or FTP site that
                                      holds custom pk3 files for your server
  r_minimize                        - when set to non-0, ioq3 will be minimized
                                       then the value will be reset to 0
  cl_altTab                         - Alt-Tab key combo will minimize when in
                                      fullscreen mode
  win_fastModeChange                - when set to non-0 a vid_restart will
                                      not be forced when switching display
                                      resolution (e.g. Alt-Tab).  This may
                                      cause texture corruption on some drivers.

New commands
  video [filename]        - start video capture (use with demo command)
  stopvideo               - stop video capture


------------------------------------------------------------ Miscellaneous -----

Using shared libraries instead of qvm
  To force Q3 to use shared libraries instead of qvms run it with the following
  parameters: +set sv_pure 0 +set vm_cgame 0 +set vm_game 0 +set vm_ui 0

Using Demo Data Files
  Copy demoq3/pak0.pk3 from the demo installer to your baseq3 directory. The
  qvm files in this pak0.pk3 will not work, so you have to use the native
  shared libraries or qvms from this project. To use the new qvms, they must be
  put into a pk3 file. A pk3 file is just a zip file, so any compression tool
  that can create such files will work. The shared libraries should already be
  in the correct place. Use the instructions above to use them.

  Please bear in mind that you will not be able to play online using the demo
  data, nor is it something that we like to spend much time maintaining or
  supporting.


QuakeLive mouse acceleration (patch and this text written by TTimo from id)
  I've been using an experimental mouse acceleration code for a while, and
  decided to make it available to everyone. Don't be too worried if you don't
  understand the explanations below, this is mostly intended for advanced
  players:
  To enable it, set cl_mouseAccelStyle 1 (0 is the default/legacy behavior)

  New style is controlled with 3 cvars:

  sensitivity
  cl_mouseAccel
  cl_mouseAccelOffset

  The old code (cl_mouseAccelStyle 0) can be difficult to calibrate because if
  you have a base sensitivity setup, as soon as you set a non zero acceleration
  your base sensitivity at low speeds will change as well. The other problem
  with style 0 is that you are stuck on a square (power of two) acceleration
  curve.

  The new code tries to solve both problems:

  Once you setup your sensitivity to feel comfortable and accurate enough for
  low mouse deltas with no acceleration (cl_mouseAccel 0), you can start
  increasing cl_mouseAccel and tweaking cl_mouseAccelOffset to get the
  amplification you want for high deltas with little effect on low mouse deltas.

  cl_mouseAccel is a power value. Should be >= 1, 2 will be the same power curve
  as style 0. The higher the value, the faster the amplification grows with the
  mouse delta.

  cl_mouseAccelOffset sets how much base mouse delta will be doubled by
  acceleration. The closer to zero you bring it, the more acceleration will
  happen at low speeds. This is also very useful if you are changing to a new
  mouse with higher dpi, if you go from 500 to 1000 dpi, you can divide your
  cl_mouseAccelOffset by two to keep the same overall 'feel' (you will likely
  gain in precision when you do that, but that is not related to mouse
  acceleration).

  Mouse acceleration is tricky to configure, and when you do you'll have to
  re-learn your aiming. But you will find that it's very much forth it in the
  long run.

  If you try the new acceleration code and start using it, I'd be very
  interested by your feedback.

UrbanTerror newest mouse acceleration mode, enabled by set cl_mouseAccelStyle 2
  It uses the same 3 cvars as QuakeLive mouse acceleration (see above).
  The difference is that the curve used is a logistic one, and it is applied
  to change sensitivity by a factor depending on mouse movement rate (speed).
  For low speeds the factor is almost 1 (i.e., no acceleration), and for highest
  speeds the factor is (1+cl_mouseAccel), that is the maximum acceleration.
  
  cl_mouseAccelOffset fixes the point where the acceleration in the factor 
  applied to sensitivity will be half the maximum:  (1+0.5*cl_mouseAccel).
  
  The result is a controllable smooth transition into mouse acceleration, and
  that the maximum acceleration is naturally limited (by the logistic curve).

  To configure it, try to set sensitivity to a value confortable for fine aim
  moving the mouse slowly, taking note of how much angle is turned by moving
  slowly only the hand (wrist) from side to side. Then set cl_mouseAccel to
  a value (greater than 0) to expand that range (e.g., value 1 will double it) 
  to obtain the maximum range reachable with a very fast wrist turn. 
  Then, adjust the value of cl_mouseOffset to feel confortable with transition.
  
  Of course, the operating system mouse acceleration (or in hardware) must be 
  dissabled to obtain controllable results with any in-game acceleration.

64bit mods
  If you wish to compile external mods as shared libraries on a 64bit platform,
  and the mod source is derived from the id Q3 SDK, you will need to modify the
  interface code a little. Open the files ending in _syscalls.c and change
  every instance of int to intptr_t in the declaration of the syscall function
  pointer and the dllEntry function. Also find the vmMain function for each
  module (usually in cg_main.c g_main.c etc.) and similarly replace the return
  value in the prototype with intptr_t (arg0, arg1, ...stay int).

  Add the following code snippet to q_shared.h:

    #ifdef Q3_VM
    typedef int intptr_t;
    #else
    #include <stdint.h>
    #endif

  Note if you simply wish to run mods on a 64bit platform you do not need to
  recompile anything since by default Q3 uses a virtual machine system.

Creating mods compatible with Q3 1.32b
  If you're using this package to create mods for the last official release of
  Q3, it is necessary to pass the commandline option '-vq3' to your invocation
  of q3asm. This is because by default q3asm outputs an updated qvm format that
  is necessary to fix a bug involving the optimizing pass of the x86 vm JIT
  compiler. See http://www.quakesrc.org/forums/viewtopic.php?t=5665 (if it
  still exists when you read this) for more details.

cl_guid Support
  cl_guid is a cvar which is part of the client's USERINFO string.  Its value
  is a 32 character string made up of [a-f] and [0-9] characters.  This
  value is pseudo-unique for every player.  Id's Quake 3 Arena client also
  sets cl_guid, but only if Punkbuster is enabled on the client.

  If cl_guidServerUniq is non-zero (the default), then this value is also
  pseudo-unique for each server a client connects to (based on IP:PORT of
  the server).

  The purpose of cl_guid is to add an identifier for each player on
  a server.  This value can be reset by the client at any time so it's not
  useful for blocking access.  However, it can have at least two uses in
  your mod's game code:
    1) improve logging to allow statistical tools to index players by more
       than just name
    2) granting some weak admin rights to players without requiring passwords

Using HTTP/FTP Download Support (Server)
  You can enable redirected downloads on your server even if it's not
  an ioquake3 server.  You simply need to use the 'sets' command to put
  the sv_dlURL cvar into your SERVERINFO string and ensure sv_allowDownloads
  is set to 1

  sv_dlURL is the base of the URL that contains your custom .pk3 files
  the client will append both fs_game and the filename to the end of
  this value.  For example, if you have sv_dlURL set to
  "http://ioquake3.org", fs_game is "baseq3", and the client is
  missing "test.pk3", it will attempt to download from the URL
  "http://ioquake3.org/baseq3/test.pk3"

  sv_allowDownload's value is now a bitmask made up of the following
  flags:
    1 - ENABLE
    2 - do not use HTTP/FTP downloads
    4 - do not use UDP downloads
    8 - do not ask the client to disconnect when using HTTP/FTP

  Server operators who are concerned about potential "leeching" from their
  HTTP servers from other ioquake3 servers can make use of the HTTP_REFERER
  that ioquake3 sets which is "ioQ3://{SERVER_IP}:{SERVER_PORT}".  For,
  example, Apache's mod_rewrite can restrict access based on HTTP_REFERER.

Using HTTP/FTP Download Support (Client)
  Simply setting cl_allowDownload to 1 will enable HTTP/FTP downloads
  assuming ioquake3 was compiled with USE_CURL=1 (the default).
  like sv_allowDownload, cl_allowDownload also uses a bitmask value
  supporting the following flags:
    1 - ENABLE
    2 - do not use HTTP/FTP downloads
    4 - do not use UDP downloads

  When ioquake3 is built with USE_CURL_DLOPEN=1 (default on some platforms),
  it will use the value of the cvar cl_cURLLib as the filename of the cURL
  library to dynamically load.

Multiuser Support on Windows systems
  On Windows, all user specific files such as autogenerated configuration,
  demos, videos, screenshots, and autodownloaded pk3s are now saved in a
  directory specific to the user who is running ioquake3.

  On NT-based such as Windows XP, this is usually a directory named:
    "C:\Documents and Settings\%USERNAME%\Application Data\Quake3\"

  Windows 95, Windows 98, and Windows ME will use a directory like:
    "C:\Windows\Application Data\Quake3"
  in single-user mode, or:
    "C:\Windows\Profiles\%USERNAME%\Application Data\Quake3"
  if multiple logins have been enabled.

  In order to access this directory more easily, the installer may create a
  Shortcut which has its target set to:
    "%APPDATA%\Quake3\"
  This Shortcut would work for all users on the system regardless of the
  locale settings.  Unfortunately, this environment variable is only
  present on Windows NT based systems.

  You can revert to the old single-user behaviour by setting the fs_homepath
  cvar to the directory where ioquake3 is installed.  For example:
    ioquake3.exe +set fs_homepath "c:\ioquake3"
  Note that this cvar MUST be set as a command line parameter.

SDL Keyboard Differences
  ioquake3 clients built againt SDL (e.g. Linux and Mac OS X) have different
  keyboard behaviour than the original Quake3 clients.

    * "Caps Lock" and "Num Lock" can not be used as normal binds since they
      do not send a KEYUP event until the key is pressed again.

    * SDL > 1.2.9 does not support disabling "Dead Key" recognition.
      In order to send "Dead Key" characters (e.g. ~, ', `, and ^), you
      must key a Space (or sometimes the same character again) after the
      character to send it on many international keyboard layouts.

    * The SDL client supports many more keys than the original Quake3 client.
      For example the keys: "Windows", "SysReq", "ScrollLock", and "Break".
      For non-US keyboards, all of the so called "World" keys are now
      supported as well as F13, F14, F15, and the country-specific
      mode/meta keys.

  SDL's "Dead Key" behaviour makes the hard-coded toggleConsole binds ~ and `
  annoying to use on many non-US keyboards.  In response, an additional
  toggleConsole bind has been added on the key combination Shift-Esc.

PNG support
  ioquake3 supports the use of PNG (Portable Network Graphic) images as
  textures. It should be noted that the use of such images in a map will
  result in missing placeholder textures where the map is used with the id
  Quake 3 client or earlier versions of ioquake3.

  Recent versions of GtkRadiant and q3map2 support PNG images without
  modification. However GtkRadiant is not aware that PNG textures are supported
  by ioquake3. To change this behaviour open the file 'q3.game' in the 'games'
  directory of the GtkRadiant base directory with an editor and change the
  line:

    texturetypes="tga jpg"

  to

    texturetypes="tga jpg png"

  Restart GtkRadiant and PNG textures are now available.


dmaHD sound system
  by p5yc0runn3r <p5yc0runn3r@gmail.com>
  
  Features:
    o Advanced 3D-Hybrid-HRTF function with Bauer Delay and Spatialization. 
    o Low/High frequency band pass filtering and extraction for increased effects.
    o 3 different mixers to chose from all mixing at a maximum of 44.1KHz 
    o Automatic memory management! (No need for CVAR's)
    o Low CPU usage!
    o Logarithmic attenuation in different mediums!
    o Speed-of-sound mapping with Doppler in air and water!
    o Weapon sounds are more pronounced.
    o Faithful to original listening distance of default Quake 3 sound.
    o Increased sound quality with cubic/Hermite 4-point spline interpolation.

  For best listening experience use good quality headphones with good bass response.

  Following are the CVARS this new build gives:
    /dmaHD_enable 
      This will enable (1) or disable (0) dmaHD. 
      Default: "1"

    /dmaHD_interpolation
      This will set the type of sound re-sampling interpolation used.
      0 = No interpolation
      1 = Linear interpolation
      2 = 4-point Cubic spline interpolation
      3 = 4-point Hermite spline interpolation
      (This option needs a total game restart after change)
      Default: "3"

    /dmaHD_mixer
      This will set the active mixer:
      10 = Hybrid-HRTF [3D]
      11 = Hybrid-HRTF [2D]
      20 = dmaEX2
      21 = dmaEX2 [No reverb]
      30 = dmaEX
      (This option changes mixers on the fly)
      Default: "10"

    /in_mouse
      Set to "2" to enable RAW mouse input.
      (This option needs a total game restart after change)
      Default: "1"

  The following are some other CVARS that affect dmaHD. Please set them as specified:
    /com_soundMegs
      This has no effect anymore. This should be set to default "8"
      (This option needs a total game restart after change)

    /s_khz 
      Set to "44" for best sound but lower it to "22" or "11" in case of FPS drops.
      (This option needs a total game restart after change)

    /s_mixahead
      This is for fine-tuning the mixer. It will mix ahead the number of seconds specified.
      The more you increase the better the sound but it will increase latency which you do not want.
      (This option needs a total game restart after change)
      Default: "0.1"

    /s_mixPreStep
      This is for fine-tuning the mixer. It will mix this number of seconds every mixing step.
      The more you increase the better the sound but it will increase drastically the amount of processing power needed.
      (This option needs a total game restart after change)
      Default: "0.05"

------------------------------------------------------------- Contributing -----

Please send all patches to bugzilla (https://bugzilla.icculus.org), or join the
mailing list (quake3-subscribe@icculus.org) and submit your patch there.  The
best case scenario is that you submit your patch to bugzilla, and then post the
URL to the mailing list. If you're too lazy for either method, then it would be
better if you emailed your patches to zakk@icculus.org directly than not at
all.

The focus for ioquake3 to develop a stable base suitable for further
development, and provide players with the same Quake 3 experience they've had
for years.  As such ioq3 does not have any significant graphical enhancements
and none are planned at this time. However, improved graphics and sound
patches will be accepted as long as they are entirely optional, do not
require new media and are off by default.


--------------------------------------------- Building Official Installers -----

We need help getting automated installers on all the platforms that ioquake3
supports. We don't neccesarily care about all the installers being identical,
but we have some general guidelines:

  * Please include the id patch pk3s in your installer, which are available
    from http://ioquake3.org/?page=getdata subject to agreement to the id
    EULA. Your installer shall also ask the user to agree to this EULA (which
    is in the /web/include directory for your convenience) and subsequently
    refuse to continue the installation of the patch pk3s and pak0.pk3 if they
    do not.

  * Please don't require pak0.pk3, since not everyone using the engine
    plans on playing Quake 3 Arena on it. It's fine to (optionally) assist the
    user in copying the file or tell them how.

  * It is fine to just install the binaries without requiring id EULA agreement,
    providing pak0.pk3 and the patch pk3s are not refered to or included in the
    installer.

  * Please include at least an SDL so/dylib on every platform but Windows
    (which doesn't use it yet).

  * Please include an OpenAL so/dylib/dll, since every platform should be using
    it by now.

  * We'll bump the version to 1.34 as soon as we get enough people who can
    demonstrate a competent build for Windows and Mac.

  * Please contact the mailing list and/or zakk@timedoctor.org after you've
    made your installer.

  * Please be prepared to alter your installer on the whim of the maintainers.

  * Your installer will be mirrored to an "official" directory, thus making it
    a done deal.

------------------------------------------------------------------ Credits -----

Maintainers
  Aaron Gyes <floam at sh dot nu>
  Ludwig Nussel <ludwig.nussel@suse.de>
  Thilo Schulz <arny@ats.s.bawue.de>
  Tim Angus <tim@ngus.net>
  Tony J. White <tjw@tjw.org>
  Zachary J. Slater <zakk@timedoctor.org>

Significant contributions from
  Ryan C. Gordon <icculus@icculus.org>
  Andreas Kohn <andreas@syndrom23.de>
  Joerg Dietrich <Dietrich_Joerg@t-online.de>
  Stuart Dalton <badcdev@gmail.com>
  Vincent S. Cojot <vincent at cojot dot name>
  optical <alex@rigbo.se>
