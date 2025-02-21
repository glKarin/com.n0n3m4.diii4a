#!/bin/bash
#sets up dependancies for debian-jessie (8.7)
#this script must be run twice. first time as root, which installs system packages
#second time as a regular user (probably not your normal one), which installs 3rd-party stuff

SVNROOT=$(cd "$(dirname "$BASH_SOURCE")" && pwd)
FTEROOT=$(realpath $SVNROOT/..)
FTEROOT=${FTEROOT:-~}
FTECONFIG=$SVNROOT/build.cfg

BUILDFOLDER=`echo ~`/htdocs
BUILDLOGFOLDER=$BUILDFOLDER/build_logs

#mac defaults
OSXCROSSROOT=$FTEROOT/osxcross

#emscripten defaults
EMSCRIPTENROOT=$FTEROOT/emsdk-portable

#android defaults
ANDROIDROOT=$FTEROOT/android
if [ ! -z "$(uname -o 2>&1 | grep Cygwin)" ]; then
	ANDROID_HOSTSYSTEM=windows-x86_64
else
	ANDROID_HOSTSYSTEM=linux-$(uname -m)
fi
ANDROIDBUILDTOOLS=25.0.0
ANDROID_ZIPALIGN=$ANDROIDROOT/build-tools/$ANDROIDBUILDTOOLS/zipalign	#relative to ndk tools

THREADS="-j 4"

TARGETS_LINUX="qcc-rel rel dbg plugins-rel plugins-dbg" #gl-rel vk-rel 
TARGETS_WINDOWS="sv-rel m-rel qcc-rel qccgui-scintilla qccgui-dbg m-dbg sv-dbg plugins-dbg plugins-rel" #gl-rel vk-rel mingl-rel d3d-rel 


PLUGINS_DROID="qi ezhud irc hl2"
PLUGINS_LINUXx86="openxr ode qi ezhud xmpp irc hl2"
PLUGINS_LINUXx64="openxr ode qi ezhud xmpp irc hl2"
PLUGINS_LINUXx32="qi ezhud xmpp irc hl2"
PLUGINS_LINUXarmhf="qi ezhud xmpp irc hl2"
PLUGINS_LINUXaarch64="qi ezhud xmpp irc hl2"
if [ "$(uname -m)" != "x86_64" ]; then
	PLUGINS_LINUXx86="openxr ode qi ezhud xmpp irc hl2"
fi
if [ "$(uname -m)" == "x86_64" ]; then
	PLUGINS_LINUX64="openxr ode qi ezhud xmpp irc hl2"
fi
#windows is always cross compiled, so we don't have issues with non-native ffmpeg
#windows doesn't cross compile, so no system dependancy issues
#skip some dependancies if we're running on cygwin, ode is buggy.
if [ "$(uname -s)" == "Linux" ]; then
	PLUGINS_WIN32="ode qi ezhud xmpp irc hl2"
	PLUGINS_WIN64="ode qi ezhud xmpp irc hl2"
else
	PLUGINS_WIN32="qi ezhud xmpp irc hl2"
	PLUGINS_WIN64="qi ezhud xmpp irc hl2"
fi

echo
echo "This is Spike's script to set up various cross compilers and dependancies."
echo "This script will check dependancies. If something isn't installed you can either rerun the script as root (which will ONLY install system packages), or manually apt-get or whatever. You can then re-run the script as a regular user to finish configuring 3rd party dependancies."
echo
echo "You can change your choices later by just re-running this script"
echo "(Your settings will be autosaved in $FTECONFIG)"
echo
echo "If you just want to compile a native build, just use the following command:"
echo "cd $SVNROOT/engine && make gl-rel"
echo "(if you're in cygwin, add FTE_TARGET=win32 to compile for native windows)"
echo "(add plugins-rel qcc-rel qccgui-rel sv-rel vk-rel etc for additional targets)"
echo "(or use -dbg if you want debug builds for whatever reason)"
echo

#always execute it if it exists, so that we preserve custom paths etc that are not prompted for here
if [ -e $FTECONFIG ]; then
	. $FTECONFIG

	if [ $UID -eq 0 ]; then
		REUSE_CONFIG="y"	#root shouldn't be writing/owning the config file.
	else
		REUSE_CONFIG="u"
	fi
else
	if [ $UID -eq 0 ]; then
		exit	#root can't create the output, as that would take ownership.
	else
		REUSE_CONFIG="n"
	fi
fi

if [ "$BUILD_CLEAN" == "n" ]; then
	NOUPDATE="y"
fi

#check args (and override config as desired)
while [[ $# -gt 0 ]]
do
	case $1 in
	-r)				#for people that want to build a specific revision for some reason.
		SVN_REV_ARG="-r $2"
		NOUPDATE=
		shift
		;;
	-j)
		THREADS="-j $2"
		shift
		;;
	-help|--help)
		echo "  -r VER       Specifies the SVN revision to update to"
		echo "  -j THREADS   Specifies how many jobs to make with"
		echo "  --help       This text"
		exit 0
		;;
	-build|--build)	#for custom build settings
		TARGET="FTE_CONFIG=$2"
		shift
		;;
	--fast)			#for people that want to live dangerously.
		BUILD_CLEAN="n"
		;;
	--noupdate)		#for people living privately or building old revisions...
		NOUPDATE="y"
		;;
	--unattended)	#don't prompt, use various defaults.
		UNATTENDED="y"
		REUSE_CONFIG="y"
		;;
	*)
		echo "Unknown option $1"
		;;
	esac
	shift
done

if [ "$REUSE_CONFIG" == "u" ]; then
	read -n 1 -p "Reuse previous build config? [y/N] " REUSE_CONFIG && echo
	REUSE_CONFIG=${REUSE_CONFIG:-n}
fi

if [ "$REUSE_CONFIG" != "y" ]; then
	#linux compiles are native-only, so don't bug out on cygwin which lacks a cross compiler.
	BUILD_LINUXx86=n
	BUILD_LINUXx64=n
	BUILD_LINUXx32=n
	BUILD_LINUXarmhf=n
	if [ "$(uname -s)" == "Linux" ]; then
		read -n 1 -p "Build for Linux x86? [Y/n] " BUILD_LINUXx86 && echo
		read -n 1 -p "Build for Linux x86_64? [Y/n] " BUILD_LINUXx64 && echo
		read -n 1 -p "Build for Linux x32? [y/N] " BUILD_LINUXx32 && echo
		#ubuntu's gcc-multilib-arm-foo package conflicts with gcc-multilib...
		#the whole point of multilib was to avoid conflicts... someone fucked up.
		#read -n 1 -p "Build for Linux armhf [y/N] " BUILD_LINUXarmhf && echo
	else
		echo "Skipping Linux options."
	fi
	BUILD_CYGWIN=n
	BUILD_MSVC=n
	if [ "$(uname -o)" == "Cygwin" ]; then
		read -n 1 -p "Build for Cygwin? [y/N] " BUILD_CYGWIN && echo
		read -n 1 -p "Build with MSVC? (requires windows7 sdk) [y/N] " BUILD_MSVC && echo
	else
		echo "Skipping Cygwin options."
	fi
	read -n 1 -p "Build for Windows x86? [Y/n] " BUILD_WIN32 && echo
	read -n 1 -p "Build for Windows x86_64? [Y/n] " BUILD_WIN64 && echo
	BUILD_DOS=n
	if [ "$(uname -o)" == "Cygwin" ]; then
		read -n 1 -p "Build for Dos? [y/N] " BUILD_DOS && echo
	fi
	BUILD_SDL_LINUXx86=n
	BUILD_SDL_LINUXx64=n
	BUILD_SDL_WIN32=n
	BUILD_SDL_WIN64=n
	if [ "$(uname -sm)" == "Linux i686" ]; then
		read -n 1 -p "Build for Linux x86 SDL? [y/N] " BUILD_SDL_LINUXx32 && echo
	fi
	if [ "$(uname -sm)" == "Linux x86_64" ]; then
		read -n 1 -p "Build for Linux x86_64 SDL? [y/N] " BUILD_SDL_LINUXx64 && echo
	fi
	read -n 1 -p "Build for Android? [y/N] " BUILD_ANDROID && echo
	read -n 1 -p "Build for Emscripten? [y/N] " BUILD_WEB && echo
	if [ 0 -ne 0 ]; then
		read -n 1 -p "Build for MacOSX? [y/N] " BUILD_MAC && echo
	else
		echo "Skipping mac option."
	fi
fi

BUILD_CLEAN=${BUILD_CLEAN:-y}
BUILD_LINUXx86=${BUILD_LINUXx86:-y}
BUILD_LINUXx64=${BUILD_LINUXx64:-y}
BUILD_LINUXx32=${BUILD_LINUXx32:-n}
BUILD_LINUXarmhf=${BUILD_LINUXarmhf:-n}
BUILD_LINUXaarch64=${BUILD_LINUXaarch64:-n}
BUILD_CYGWIN=${BUILD_CYGWIN:-n}
BUILD_WIN32=${BUILD_WIN32:-y}
BUILD_WIN64=${BUILD_WIN64:-y}
BUILD_DOS=${BUILD_DOS:-n}
BUILD_MSVC=${BUILD_MSVC:-n}
BUILD_SDL=${BUILD_SDL:-n}
BUILD_ANDROID=${BUILD_ANDROID:-n}
BUILD_WEB=${BUILD_WEB:-n}
BUILD_MAC=${BUILD_MAC:-n}

if [ "$UID" != "0" ]; then
	echo "#path config for fte build scripts"		>$FTECONFIG
	echo "THREADS=\"$THREADS\""				>>$FTECONFIG
	echo "BUILDFOLDER=\"$BUILDFOLDER\""			>>$FTECONFIG
	echo "BUILDLOGFOLDER=\"$BUILDLOGFOLDER\""		>>$FTECONFIG
	echo "SVNROOT=\"$SVNROOT\""				>>$FTECONFIG
	echo "ANDROIDROOT=\"$ANDROIDROOT\""			>>$FTECONFIG
	echo "export ANDROID_HOSTSYSTEM=\"$ANDROID_HOSTSYSTEM\""	>>$FTECONFIG
	echo "export ANDROID_ZIPALIGN=\"$ANDROID_ZIPALIGN\""	>>$FTECONFIG
	echo "EMSCRIPTENROOT=\"$EMSCRIPTENROOT\""		>>$FTECONFIG
	echo "OSXCROSSROOT=\"$OSXCROSSROOT\""			>>$FTECONFIG

	echo "BUILD_CLEAN=\"$BUILD_CLEAN\""		>>$FTECONFIG

	echo "BUILD_LINUXx86=\"$BUILD_LINUXx86\""		>>$FTECONFIG
	echo "BUILD_LINUXx64=\"$BUILD_LINUXx64\""		>>$FTECONFIG
	echo "BUILD_LINUXx32=\"$BUILD_LINUXx32\""		>>$FTECONFIG
	echo "BUILD_LINUXarmhf=\"$BUILD_LINUXarmhf\""		>>$FTECONFIG
	echo "BUILD_LINUXaarch64=\"$BUILD_LINUXaarch64\""		>>$FTECONFIG
	echo "BUILD_CYGWIN=\"$BUILD_CYGWIN\""			>>$FTECONFIG
	echo "BUILD_WIN32=\"$BUILD_WIN32\""			>>$FTECONFIG
	echo "BUILD_WIN64=\"$BUILD_WIN64\""			>>$FTECONFIG
	echo "BUILD_DOS=\"$BUILD_DOS\""				>>$FTECONFIG
	echo "BUILD_MSVC=\"$BUILD_MSVC\""			>>$FTECONFIG
	echo "BUILD_ANDROID=\"$BUILD_ANDROID\""			>>$FTECONFIG
	echo "BUILD_SDL=\"$BUILD_SDL\""				>>$FTECONFIG
	echo "BUILD_WEB=\"$BUILD_WEB\""				>>$FTECONFIG
	echo "BUILD_MAC=\"$BUILD_MAC\""				>>$FTECONFIG

	echo "TARGETS_WINDOWS=\"$TARGETS_WINDOWS\""		>>$FTECONFIG
	echo "TARGETS_LINUX=\"$TARGETS_LINUX\""			>>$FTECONFIG
	echo "PLUGINS_WIN32=\"$PLUGINS_WIN32\""		>>$FTECONFIG
	echo "PLUGINS_WIN64=\"$PLUGINS_WIN64\""		>>$FTECONFIG
	echo "PLUGINS_LINUXx86=\"$PLUGINS_LINUXx86\""		>>$FTECONFIG
	echo "PLUGINS_LINUXx64=\"$PLUGINS_LINUXx64\""		>>$FTECONFIG
	echo "PLUGINS_LINUXx32=\"$PLUGINS_LINUXx32\""		>>$FTECONFIG
	echo "PLUGINS_LINUXarmhf=\"$PLUGINS_LINUXarmhf\""	>>$FTECONFIG
	echo "PLUGINS_LINUXaarch64=\"$PLUGINS_LINUXaarch64\""	>>$FTECONFIG
	echo "PLUGINS_DROID=\"$PLUGINS_DROID\""	>>$FTECONFIG
fi

true
true=$?
false
false=$?

if [ "$(uname -s)" == "Linux" ]; then
	. /etc/os-release
fi
function debianpackages {
	#make sure apt-get is installed
	if [ -z `which apt-get 2>>/dev/null` ]; then
		return $false
	fi
	local ret=$true
	for i in "$@"
	do
		dpkg -s $i 2>&1 >> /dev/null
		if [ $? -eq 1 ]; then
			echo "Package missing: $i"
			ret=$false
		fi
	done

	if [ $ret == $false ]; then
		echo "Packages are not installed. Press enter to continue (or ctrl+c and install)."
		if [ "$UNATTENDED" != "y" ]; then
			read
		fi
		ret=$true
	fi
	return $ret
}
function jessiepackages {
	if [ "$PRETTY_NAME" != "Debian GNU/Linux 8 (jessie)" ]; then
		return $false
	fi

	debianpackages $@
	return $?
}

#we don't really know what system we're on. assume they have any system dependancies.
#fixme: args are programs findable with which
function otherpackages {
	if [ -z "$PRETTY_NAME" ]; then
		return $true
	fi
	return $false
}


#Note: only the native linux-sdl target can be compiled, as libSDL[2]-dev doesn't support multiarch properly, and we depend upon it instead of building from source (thus ensuring it has whatever distro stuff needed... though frankly that should be inside the .so instead of the headers).

#if [ $UID -eq 0 ] && [ ! -z `which apt-get` ]; then
	#because multiarch requires separate packages for some things, we'll need to set that up now (in case noone did that yet)
#	dpkg --add-architecture i386
#	apt-get update
#fi

#generic crap. much of this is needed to set up and decompress dependancies and stuff.
debianpackages git make automake libtool p7zip-full zip ca-certificates || otherpackages z7 make git || exit

if [ "$BUILD_LINUXx86" == "y" ]; then
	#for building linux targets
	debianpackages gcc-multilib g++-multilib mesa-common-dev libasound2-dev libxcursor-dev || otherpackages gcc || exit
	jessiepackages libgnutls28-dev || debianpackages libgnutls28-dev || otherpackages gcc || exit
	if [[ "$PLUGINS_LINUXx86" =~ "ffmpeg" ]]; then
		debianpackages libswscale-dev libavcodec-dev || otherpackages || exit
	fi
	if [[ "$PLUGINS_LINUXx86" =~ "openxr" ]]; then
		debianpackages libopenxr-dev || otherpackages || exit
	fi
fi
if [ "$BUILD_LINUXx64" == "y" ]; then
	#for building linux targets
	debianpackages gcc-multilib g++-multilib mesa-common-dev libasound2-dev libxcursor-dev || otherpackages gcc || exit
	jessiepackages libgnutls28-dev || debianpackages libgnutls28-dev || otherpackages gcc || exit
	if [[ "$PLUGINS_LINUXx64" =~ "ffmpeg" ]]; then
		debianpackages libswscale-dev libavcodec-dev || otherpackages || exit
	fi
	if [[ "$PLUGINS_LINUXx64" =~ "openxr" ]]; then
		debianpackages libopenxr-dev || otherpackages || exit
	fi
fi
if [ "$BUILD_LINUXx32" == "y" ]; then
	#for building linux targets
	debianpackages gcc-multilib g++-multilib mesa-common-dev libasound2-dev libxcursor-dev || otherpackages gcc || exit
	jessiepackages libgnutls28-dev || debianpackages libgnutls28-dev || otherpackages gcc || exit
fi
if [ "$BUILD_LINUXarmhf" == "y" ]; then
	#for building linux targets
	debianpackages gcc-multilib-arm-linux-gnueabihf g++-multilib-arm-linux-gnueabihf mesa-common-dev libasound2-dev libxcursor-dev || otherpackages gcc || exit
	jessiepackages libgnutls28-dev || debianpackages libgnutls28-dev || otherpackages gcc || exit
fi
if [ "$BUILD_SDL" == "y" ]; then
	#for building SDL targets
	debianpackages libSDL1.2-dev libSDL2-dev libspeex-dev libspeexdsp-dev || otherpackages || exit
fi

if [ "$BUILD_WIN32" == "y" ] || [ "$BUILD_WIN64" == "y" ]; then
	#for building windows targets
	#tools package provides pkg-config
	#python is needed to configure scintilla properly.
	debianpackages mingw-w64 mingw-w64-tools python || otherpackages x86_64-w64-mingw32-gcc python || exit
fi


if [ "$BUILD_ANDROID" == "y" ]; then
	( (jessiepackages openjdk-8-jdk-headless || debianpackages openjdk-8-jdk-headless ) && debianpackages ant) || otherpackages || exit
fi

if [ "$BUILD_WEB" == "y" ]; then
	( (jessiepackages cmake || debianpackages cmake) && debianpackages git build-essential) || exit
fi

if [ "$BUILD_MAC" == "y" ]; then
	debianpackages git cmake libxml2-dev fuse || otherpackages || exit
fi
debianpackages subversion make build-essential || otherpackages svn make || exit

echo "System Package checks complete."

if [ "$UID" == "0" ]; then
	#avoid root taking ownership of anything.
	echo "Refusing to update/rebuild toolchains as root."
	echo "Please continue running this script as a regular user."
	exit
fi

if [ "$UNATTENDED" != "y" ]; then
	echo
	echo "(Any new toolchains will be installed to $FTEROOT)"
	echo "(Say no if you're certain you already set up everything)"
	read -n 1 -p "Rebuild/update any toolchains now? [y/N] " REBUILD_TOOLCHAINS && echo
else
	REBUILD_TOOLCHAINS="y"
fi
REBUILD_TOOLCHAINS=${REBUILD_TOOLCHAINS:-n}
mkdir -p $FTEROOT

#dos shit
if [ "$BUILD_DOS" == "y" ] && [ $UID -ne 0 ] && [ $REBUILD_TOOLCHAINS == "y" ]; then
	echo "You'll need to manually install djgpp for DOS builds."
fi

#android shit. WARNING: should come first as it spits out some EULAs that need confirming.
if [ "$BUILD_ANDROID" == "y" ] && [ $UID -ne 0 ] && [ $REBUILD_TOOLCHAINS == "y" ]; then
	mkdir -p $ANDROIDROOT
	cd $ANDROIDROOT
	wget -N https://dl.google.com/android/repository/tools_r25.2.3-linux.zip
	unzip -qn tools_r25.2.3-linux.zip
	cd tools/bin
	#yes, android-8 is fucking old now. newer versions won't work on older devices.
	echo "downloading android build tools"
	./sdkmanager "build-tools;$ANDROID_BUILDTOOLS"
	echo "downloading android platform tools"
	./sdkmanager "platform-tools"
	echo "downloading android-9"
	./sdkmanager "platforms;android-9"
	echo "downloading android ndk"
	./sdkmanager "ndk-bundle"
	cd ~
fi

#emscripten/web shit
if [ "$BUILD_WEB" == "y" ] && [ $UID -ne 0 ] && [ $REBUILD_TOOLCHAINS == "y" ]; then
	mkdir -p $EMSCRIPTENROOT
	cd $EMSCRIPTENROOT/..
	wget -N https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-portable.tar.gz
	cd $EMSCRIPTENROOT
	tar xzf ../emsdk-portable.tar.gz --strip-components=1
	./emsdk install latest
	./emsdk activate latest
	cd ~
fi


#osxcross, for mac crap
if [ "$BUILD_MAC" == "y" ] && [ $UID -ne 0 ] && [ $REBUILD_TOOLCHAINS == "y" ] && [ "$UNATTENDED" != "y" ]; then
	echo "Setting up OSXCross... THIS IS TOTALLY UNTESTED"
	read -p "You need to download xcode first. Where did you download the .dmg file to?" XCODE
	git clone https://github.com/tpoechtrager/osxcross.git $OSXCROSSROOT
	cd $OSXCROSSROOT
	tools/gen_sdk_package_darling_dmg.sh $XCODE
	cp *.tar.xz
	SDK_VERSION=10.10 UNATTENDED=0 ./build.sh
	cd ~
fi


if [ $UID -ne 0 ] && [ $REBUILD_TOOLCHAINS == "y" ]; then
	#initial checkout of fte's svn
	if [ "$NOUPDATE"!="n" ]; then
		if [ ! -d $SVNROOT ]; then
			svn checkout https://svn.code.sf.net/p/fteqw/code/trunk $SVNROOT $SVN_REV_ARG
		else
			cd $SVNROOT
			svn up $SVN_REV_ARG
		fi
	fi

	#FIXME: there may be race conditions when compiling.
	#so make sure we've pre-built certain targets without using -j
	#linux distros vary too much with various dependancies and versions and such, so we might as well pre-build our own copies of certain libraries. this really only needs to be done once, but its safe to retry anyway.
	cd $SVNROOT/engine
	if [ "$BUILD_LINUXx86" == "y" ]; then
		echo "Making libraries (linux x86)..."
		make FTE_TARGET=linux32 makelibs CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_LINUXx64" == "y" ]; then
		echo "Making libraries (linux x86_64)..."
		make FTE_TARGET=linux64 makelibs CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_LINUXx32" == "y" ]; then
		echo "Making libraries (linux x32)..."
		make FTE_TARGET=linuxx32 makelibs CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_LINUXarmhf" == "y" ]; then
		echo "Making libraries (linux armhf)..."
		make FTE_TARGET=linuxarmhf makelibs CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_LINUXaarch64" == "y" ]; then
		echo "Making libraries (linux aarch64)..."
		make FTE_TARGET=linuxaarch64 makelibs CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_WIN32" == "y" ]; then
		echo "Making libraries (win32)..."
		make FTE_TARGET=win32 makelibs CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_WIN64" == "y" ]; then
		echo "Making libraries (win64)..."
		make FTE_TARGET=win64 makelibs CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi

	#These plugins have external 3rd-party dependancies that are downloaded as part of building.
	if [ "$BUILD_WIN32" == "y" ] && [[ "$PLUGINS_WIN32" =~ "ode" ]]; then
		echo "Prebuilding ODE library (win32)..."
		make FTE_TARGET=win32 plugins-rel NATIVE_PLUGINS=ode 2>&1 >>/dev/null
	fi
	if [ "$BUILD_WIN64" == "y" ] && [[ "$PLUGINS_WIN64" =~ "ode" ]]; then
		echo "Prebuilding ODE library (win64)..."
		make FTE_TARGET=win64 plugins-rel NATIVE_PLUGINS=ode 2>&1 >>/dev/null
	fi
	if [ "$BUILD_LINUXx86" == "y" ] && [[ "$PLUGINS_LINUXx86" =~ "ode" ]]; then
		echo "Prebuilding ODE library (linux x86)..."
		make FTE_TARGET=linux32 plugins-rel NATIVE_PLUGINS=ode CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_LINUXx64" == "y" ] && [[ "$PLUGINS_LINUXx64" =~ "ode" ]]; then
		echo "Prebuilding ODE library (linux x86_64)..."
		make FTE_TARGET=linux64 plugins-rel NATIVE_PLUGINS=ode CPUOPTIMISATIONS=-fno-finite-math-only 2>&1 >>/dev/null
	fi
	if [ "$BUILD_WIN32" == "y" ] && [[ "$PLUGINS_WIN32" =~ "ffmpeg" ]]; then
		echo "Obtaining ffmpeg library (win32)..."
		make FTE_TARGET=win32 plugins-rel NATIVE_PLUGINS=ffmpeg 2>&1 >>/dev/null
	fi
	if [ "$BUILD_WIN64" == "y" ] && [[ "$PLUGINS_WIN64" =~ "ffmpeg" ]]; then
		echo "Obtaining ffmpeg library (win64)..."
		make FTE_TARGET=win64 plugins-rel NATIVE_PLUGINS=ffmpeg 2>&1 >>/dev/null
	fi
	cd ~
fi



echo "Setup script complete."
echo "When you run build_wip.sh output will be written to $BUILDFOLDER/*"

