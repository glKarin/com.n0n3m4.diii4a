/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __BOTHDEFS_H
#define __BOTHDEFS_H

// release version
#define FTE_VER_MAJOR 1
#define FTE_VER_MINOR 7

#if defined(__APPLE__) && defined(__MACH__)
	#define MACOSX
#endif

#if defined(__MINGW32_VERSION) || defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__)
	#define MINGW
#endif
#if !defined(MINGW) && defined(__GNUC__) && defined(_WIN32)
	#define MINGW	//Erm, why is this happening?
#endif

#ifdef ANDROID
#if !defined(_DIII4A) //karin: using jpeg png ogg
	#define NO_PNG
	#define NO_JPEG
	#define NO_OGG
#endif
#endif

#ifdef _XBOX
	#define NO_PNG
	#define NO_JPEG
	#define NO_OGG
	#define NO_ZLIB
	#define NOMEDIA
	#define NO_FREETYPE
	#define HAVE_PACKET
#endif

#ifndef MULTITHREAD
	#if !defined(_WIN32) || defined(FTE_SDL) //win32 is annoying
		#define NO_MULTITHREAD
	#endif
#endif

#ifdef FTE_TARGET_WEB
	//no Sys_LoadLibrary support, so we might as well kill this stuff off.
	#define NO_PNG
	#define NO_JPEG
	#define NO_OGG
	#ifndef NO_FREETYPE
		#define NO_FREETYPE
	#endif
#endif

#ifdef D3DQUAKE
	#define D3D9QUAKE
	//#define D3D11QUAKE
	#undef D3DQUAKE
#endif

#define STRINGIFY2(s) #s
#define STRINGIFY(s) STRINGIFY2(s)

#ifndef CONFIG_FILE_NAME
	#ifdef HAVE_CONFIG_H
		#define CONFIG_FILE_NAME config.h
	#elif defined(NOLEGACY)
		#undef NOLEGACY
		#define CONFIG_FILE_NAME config_nocompat.h
	#elif defined(MINIMAL)
		#define CONFIG_FILE_NAME config_minimal.h
	#else
		#define CONFIG_FILE_NAME config_fteqw.h
	#endif
#endif

#undef MULTITHREAD
#define HEADLESSQUAKE	//usable renderers are normally specified via the makefile, but HEADLESS is considered a feature rather than an actual renderer, so usually gets forgotten about...

//yup, C89 allows this (doesn't like C's token concat though).
#include STRINGIFY(CONFIG_FILE_NAME)


#ifndef MSVCLIBSPATH
	#ifdef MSVCLIBPATH
		#define MSVCLIBSPATH STRINGIFY(MSVCLIBPATH)
	#elif _MSC_VER == 1200
		#define MSVCLIBSPATH "../libs/vc6-libs/"
	#else
		#define MSVCLIBSPATH "../libs/"
	#endif
#endif

#if defined(IMGTOOL) || defined(IQMTOOL)
	#undef WEBCLIENT
	#undef LOADERTHREAD
#elif defined(MASTERONLY)
	#define SV_MASTER
	#undef SUBSERVERS
	#undef PLUGINS
	#undef HUFFNETWORK
	#undef SUPPORT_ICE
	#undef WEBCLIENT
	#undef MULTITHREAD
	#undef LOADERTHREAD
	#undef PACKAGEMANAGER
	#undef PACKAGE_PK3
	#undef PACKAGE_Q1PAK
	#undef PACKAGE_DOOMWAD
	#undef PACKAGE_VPK
	#undef PACKAGE_DZIP
	#undef AVAIL_XZDEC
	#undef AVAIL_GZDEC
	#undef SUBSERVERS
	#undef HAVE_LEGACY
	#undef IPLOG
#else
	#if defined(SERVERONLY) && defined(CLIENTONLY)
		#undef CLIENTONLY	//impossible build. assume the config had CLIENTONLY and they tried building a dedicated server
	#endif
	#ifndef WEBSVONLY
		#ifndef CLIENTONLY
			#define HAVE_SERVER
		#endif
		#ifndef SERVERONLY
			#define HAVE_CLIENT
		#endif
	#endif
#endif
#ifndef NOLEGACY
	#define HAVE_LEGACY
#endif

#ifndef HAVE_SERVER
	#undef MVD_RECORDING
#endif

//software rendering is just too glitchy, don't use it - unless its the only choice.
#if defined(SWQUAKE) && !defined(_DEBUG) && !defined(__DJGPP__)
	#undef SWQUAKE
#endif
#if defined(USE_EGL) && !defined(GLQUAKE)
	#undef USE_EGL
#endif
#if defined(WAYLANDQUAKE) && !(defined(__linux__) && (defined(VKQUAKE) || (defined(GLQUAKE) && defined(USE_EGL))))
	#undef WAYLANDQUAKE
#endif

//include a file to update the various configurations for game-specific configs (hopefully just names)
#ifdef BRANDING_INC
	#include STRINGIFY(BRANDING_INC)
#endif
#ifndef DISTRIBUTION
	#define DISTRIBUTION "FTE"	//short name used to identify this engine. must be a single word
#endif
#ifndef DISTRIBUTIONLONG
	#define DISTRIBUTIONLONG "Forethought Entertainment"	//effectively the 'company' name
#endif
#ifndef FULLENGINENAME
	#define FULLENGINENAME "FTE QW"	//the posh name for the engine, note that 'Quake' is trademarked so we should not be using it here.
#endif
#ifndef ENGINEWEBSITE
	#define ENGINEWEBSITE "^8https://^4fte^8.^4triptohell^8.^4info"	//url for program
#endif

#if !defined(_WIN32) || defined(WINRT)
	#undef HAVE_SPEECHTOTEXT
	#undef AVAIL_MP3_ACM
	#undef AVAIL_DSOUND
	#undef AVAIL_XAUDIO2
	#undef AVAIL_WASAPI
#endif

//#if !(defined(__linux__) || defined(__CYGWIN__)) || defined(ANDROID)
//	#undef HAVE_GNUTLS
//#endif
#if !defined(_WIN32) || (defined(_MSC_VER) && (_MSC_VER < 1300)) || defined(FTE_SDL)
	#undef HAVE_WINSSPI
#endif
//subservers only has code for win32 threads and linux
#if !((defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT)) || (defined(__linux__) && !defined(ANDROID) && !defined(FTE_SDL)))
	#undef SUBSERVERS
#endif

#ifndef HAVE_MIXER
	//disable various sound drivers if we can't use them anyway.
	#undef AVAIL_DSOUND
	#undef AVAIL_XAUDIO2
	#undef AVAIL_WASAPI

	#undef AUDIO_ALSA
	#undef AUDIO_PULSE
#endif


#ifdef NOMEDIA
	#undef HAVE_CDPLAYER		//includes cd playback. actual cds. faketracks are supported regardless.
	#undef HAVE_JUKEBOX			//includes built-in jukebox crap
	#undef HAVE_MEDIA_DECODER	//can play cin/roq, more with plugins
	#undef HAVE_MEDIA_ENCODER	//capture/capturedemo work.
	#undef AVAIL_MP3_ACM		//microsoft's Audio Compression Manager api
	#undef HAVE_SPEECHTOTEXT	//windows speech-to-text thing
#endif

#if defined(_XBOX)
	#define D3D8QUAKE
	#undef HAVE_TCP		//FIXME
	#undef HAVE_PACKET	//FIXME
	#undef SUPPORT_ICE	//screw that
	#undef PLUGINS		//would need LoadLibrary working properly.

	#undef AVAIL_DINPUT	//xbox apparently only really does controllers.
	#undef AVAIL_DSOUND	//FIXME
	#undef TEXTEDITOR	//its hard to edit text when you have just a controller (and no onscreen keyboard)
	#undef RAGDOLL		//needs a proper physics engine
	#undef AVAIL_MP3_ACM		//api not supported
	#undef AVAIL_OPENAL
	#undef HAVE_SPEECHTOTEXT	//api not supported
	#undef MULTITHREAD			//no CreateThread stuff.
	#undef SUBSERVERS			//single-process.
	#undef VOICECHAT
	#undef TERRAIN
	#undef Q2CLIENT
	#undef Q2SERVER
	#undef Q3CLIENT
	#undef Q3SERVER
	#undef HLCLIENT
	#undef HLSERVER
	#undef VM_Q1
	#undef VM_LUA
	#undef HALFLIFEMODELS
	#undef RUNTIMELIGHTING
	#undef HEXEN2
	#undef PACKAGE_DOOMWAD	
	#undef MAP_PROC	
	#undef Q1BSPS
	#undef Q2BSPS
	#undef Q3BSPS
	#undef RFBSPS
	#undef FTPSERVER		//ftp server
	#undef WEBCLIENT		//http client.
	#undef FTPCLIENT		//ftp client.
#endif

#ifdef __DJGPP__
	//no bsd sockets library.
	#undef HAVE_TCP
	#undef HAVE_PACKET
	#undef SUPPORT_ICE
	//too lazy to deal with no dlopen
	#undef PLUGINS
	#undef Q2SERVER
	#undef Q3SERVER
	#undef Q2CLIENT	//fixme...
	#undef Q3CLIENT	//might as well.
	//too lazy to write the code to boot up more cores. dosbox would probably hate it so why bother.
	#undef MULTITHREAD
	//too lazy to deal with various libraries
	#undef VOICECHAT
	#undef AVAIL_JPEGLIB
	#undef AVAIL_PNGLIB
	#undef AVAIL_OGGVORBIS
#endif

#ifdef FTE_TARGET_WEB
	//sandboxing means some stuff CANNOT work...
	#undef HAVE_TCP		//websockets are not real tcp.
	#undef HAVE_PACKET	//no udp support

	//try to trim the fat
//	#undef VOICECHAT	//too lazy to compile opus
	#undef HLCLIENT		//dlls...
	#undef HLSERVER		//dlls...
//	#undef CL_MASTER	//bah. use the site to specify the servers.
	#undef SV_MASTER	//yeah, because that makes sense in a browser
	#undef RAGDOLL		//no ode
	#undef TCPCONNECT	//err...
	#undef IRCCONNECT	//not happening
	#if !defined(USE_INTERNAL_BULLET) && !defined(USE_INTERNAL_ODE) && !defined(MODELFMT_GLTF) && !defined(STATIC_EZHUD) && !defined(STATIC_OPENSSL) && !defined(STATIC_Q3)
		#undef PLUGINS		//pointless
	#endif
	#undef MAP_PROC		//meh
//	#undef HALFLIFEMODELS	//blurgh
	#undef SUPPORT_ICE	//requires udp, so not usable. webrtc could be used instead, but that logic is out of our hands.
//	#undef HAVE_MIXER	//depend upon openal instead.

	//extra features stripped to try to reduce memory footprints
	#undef RUNTIMELIGHTING	//too slow anyway (kinda needs threads)
	#undef Q2SERVER	//requires a dll anyway.
//	#undef Q2CLIENT //match Q2SERVER (networking is a pain)
//	#undef Q3CLIENT	//no bots, and networking is a pain
//	#undef Q3SERVER //match Q3CLIENT
//	#undef Q2BSPS	//emscripten can't cope with bss, leading to increased download time. too lazy to fix.
//	#undef Q3BSPS	//emscripten can't cope with bss, leading to increased download time. too lazy to fix.
//	#undef TERRAIN
//	#undef PSET_SCRIPT	//bss+size
	#define GLSLONLY	//pointless having the junk
	#define GLESONLY	//should reduce the conditions a little
	#ifndef R_MAX_RECURSE
		#define R_MAX_RECURSE 2 //less bss
	#endif
//	#undef RTLIGHTS
	#undef HEADLESSQUAKE
	#ifndef NO_FREETYPE
	#define NO_FREETYPE
	#endif
#endif
#ifdef WINRT
	//microsoft do not support winsock any more.
	#undef HAVE_TCP
	#undef HAVE_PACKET

	#undef TCPCONNECT	//err...
	#undef IRCCONNECT	//not happening
	#undef AVAIL_DSOUND	//yeah, good luck there
	#undef AVAIL_DINPUT	//nope, not supported.
	#undef SV_MASTER	//no socket interface
	#undef CL_MASTER	//no socket interface
	#undef MULTITHREAD
	#undef HEADLESSQUAKE
#endif
#ifdef ANDROID
	#define GLESONLY	//should reduce the conditions a little
//	#undef HEADLESSQUAKE
#if !defined(_DIII4A) //karin: use OpenAL and freetype
	#ifndef NO_FREETYPE
		#define NO_FREETYPE
	#endif
	#define NO_OPENAL
#endif
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1500)) || defined(FTE_SDL)
	#undef AVAIL_WASAPI	//wasapi is available in the vista sdk, while that's compatible with earlier versions, its not really expected until 2008
#endif

#if !defined(HAVE_SERVER) && !defined(SV_MASTER)
	#undef HAVE_HTTPSV
#endif

#ifdef NO_MULTITHREAD
	#undef MULTITHREAD
#endif
#ifndef MULTITHREAD
	//database code requires threads to do stuff async.
	#undef USE_SQLITE
	#undef USE_MYSQL
	#undef AUDIO_PULSE
#endif
#ifdef NO_LIBRARIES //catch-all...
#define NO_DIRECTX
	#define NO_PNG
	#define NO_JPEG
	#define NO_ZLIB
	#define NO_OGG
	#define NO_FREETYPE
#endif
#ifdef NO_OPENAL
	#undef AVAIL_OPENAL
#endif
#ifdef NO_PNG
	#undef AVAIL_PNGLIB
#endif
#ifdef NO_JPEG
	#undef AVAIL_JPEGLIB
#endif
#ifdef NO_OGG
	#undef AVAIL_OGGVORBIS
#endif
#ifdef NO_FREETYPE
	#undef AVAIL_FREETYPE
#endif
#ifdef NO_ZLIB
	#undef AVAIL_ZLIB
	#undef AVAIL_PNGLIB
	#undef AVAIL_XZDEC
	#undef AVAIL_GZDEC
#endif
#ifdef NO_GNUTLS
	#undef HAVE_GNUTLS
#endif
#ifdef NO_WINSSPI
	#undef HAVE_WINSSPI
#endif
#ifdef NO_OPENGL
	#undef GLQUAKE
	#undef USE_EGL
#endif

#if (defined(HAVE_GNUTLS) || defined(HAVE_WINSSPI) || defined(PLUGINS)) && !defined(FTE_TARGET_WEB)
	#define HAVE_SSL
#endif
#if (defined(HAVE_GNUTLS) || defined(HAVE_WINSSPI) || defined(PLUGINS)) && !defined(FTE_TARGET_WEB)
	//FIXME: HAVE_WINSSPI does not work as a server.
	//FIXME: advertising dtls without a valid certificate will probably bug out if a client tries to auto-upgrade.
	//FIXME: we don't cache server certs
	#define HAVE_DTLS
#endif

#if defined(USE_SQLITE) || defined(USE_MYSQL)
	#define SQL
#endif

#if defined(AVAIL_GZDEC) && (!defined(AVAIL_ZLIB) || defined(NO_ZLIB))
	//gzip needs zlib to work (pk3s can still contain non-compressed files)
	#undef AVAIL_GZDEC
#endif

#if defined(RFBSPS) && !defined(Q3BSPS)
	#define Q3BSPS	//rbsp might as well depend upon q3bsp - its the same thing but with more lightstyles (support for which can bog down the renderer a little).
#endif

#if defined(QWOVERQ3) && !defined(Q3SERVER)
	#undef QWOVERQ3
#endif

#if !defined(NQPROT) || defined(SERVERONLY) || !defined(AVAIL_ZLIB) || defined(DYNAMIC_ZLIB)
	#undef PACKAGE_DZIP
#endif

#if (defined(NOLOADERTHREAD) || !defined(MULTITHREAD)) && defined(LOADERTHREAD)
	#undef LOADERTHREAD
#endif

#ifndef _WIN32
	#undef QTERM	//not supported - FIXME: move to native plugin
#endif

#if defined(Q3BSPS) && !defined(Q2BSPS)
//	#define Q2BSPS	//FIXME: silently enable that as a dependancy, for now
#endif

#if (defined(Q2CLIENT) || defined(Q2SERVER))
	#ifndef Q2BSPS
		#error "Q2 game support without Q2BSP support. doesn't make sense"
	#endif
	#if !defined(MD2MODELS) || !defined(SP2MODELS)
		#error "Q2 game support without full Q2 model support. doesn't make sense"
	#endif
#endif

#ifndef AVAIL_ZLIB
	#undef SUPPORT_ICE	//depends upon zlib's crc32 for fingerprinting. I cba writing my own.
#endif

#ifndef HAVE_TCP
	#undef TCPCONNECT
	#undef IRCCONNECT
	#undef FTPSERVER		//ftp server
	#undef FTPCLIENT		//ftp client.
	#if !defined(FTE_TARGET_WEB)
		#undef WEBCLIENT
	#endif
#endif
#ifndef HAVE_PACKET
	#undef SV_MASTER
	#ifndef FTE_TARGET_WEB
		#undef CL_MASTER	//can use websockets to get a list of usable ws:// or rtc:// servers
	#endif
	#undef SUPPORT_ICE	//webrtc takes all control away from us, the implementation is completely different.
#endif

#ifdef SERVERONLY	//remove options that don't make sense on only a server
	#undef Q2CLIENT
	#undef Q3CLIENT
	#undef HLCLIENT
	#undef VM_UI
	#undef VM_CG
	#undef TEXTEDITOR
	#undef RUNTIMELIGHTING

	#undef PSET_SCRIPT
	#undef PSET_CLASSIC
	#undef PSET_DARKPLACES
#endif
#ifdef CLIENTONLY	//remove optional server components that make no sence on a client only build.
	#undef Q2SERVER
	#undef Q3SERVER
	#undef HLSERVER
	#undef FTPSERVER
	#undef SUBSERVERS
	#undef VM_Q1
	#undef SQL
#endif

#ifndef PLUGINS
	#undef USE_INTERNAL_BULLET
	#undef USE_INTERNAL_ODE
#endif


#if (defined(CSQC_DAT) || !defined(CLIENTONLY)) && (defined(PLUGINS)||defined(USE_INTERNAL_BULLET)||defined(USE_INTERNAL_ODE))	//use ode only if we have a constant world state, and the library is enbled in some form.
	#define USERBE
#endif

#if  defined(MD1MODELS) || defined(MD2MODELS) || defined(MD3MODELS)
	#define NONSKELETALMODELS
#endif
#if  defined(ZYMOTICMODELS) || defined(MD5MODELS) || defined(DPMMODELS) || defined(PSKMODELS) || defined(INTERQUAKEMODELS) 
	#define SKELETALMODELS	//defined if we have a skeletal model.
#endif
#if (defined(CSQC_DAT) || !defined(CLIENTONLY)) && defined(SKELETALMODELS)
	#define SKELETALOBJECTS	//the skeletal objects API is only used if we actually have skeletal models, and gamecode that uses the builtins.
#endif
#if !defined(USERBE) || !defined(SKELETALMODELS)
	#undef RAGDOLL	//not possible to ragdoll if we don't have certain other features.
#endif

#if !defined(RTLIGHTS)
	#undef MAP_PROC	//doom3 maps kinda NEED rtlights to look decent
#endif

#if !defined(Q3BSPS)
	#undef Q3CLIENT //reconsider this (later)
	#undef Q3SERVER //reconsider this (later)
#endif
#if defined(DEBUG) || defined(_DEBUG)
	#undef NOQCDESCRIPTIONS	//don't disable writing fteextensions.qc in debug builds, otherwise how would you ever build one? :o
#endif


#ifndef Q3CLIENT
	#undef VM_CG	// :(
	#undef VM_UI
#else
	#define VM_CG
	#define VM_UI
#endif

#if defined(VM_Q1) || defined(VM_UI) || defined(VM_CG) || defined(Q3SERVER)
	#define VM_ANY
#endif

#if (defined(HAVE_CLIENT) || defined(HAVE_SERVER)) && defined(WEBCLIENT) && defined(PACKAGEMANAGER)
	#define MANIFESTDOWNLOADS
#endif

#if (defined(D3D8QUAKE) || defined(D3D9QUAKE) || defined(D3D11QUAKE)) && !defined(D3DQUAKE)
	#define D3DQUAKE	//shouldn't still matter
#endif

#define PROTOCOLEXTENSIONS

#ifdef MINIMAL
	#define IFMINIMAL(x,y) x
#else
	#define IFMINIMAL(x,y) y
#endif
#ifdef FTE_TARGET_WEB
	#define IFWEB(x,y) x
#else
	#define IFWEB(x,y) y
#endif

// defs common to client and server

#ifndef PLATFORM
	#if defined(FTE_TARGET_WEB)
		#define PLATFORM		"Web"
		#define ARCH_CPU_POSTFIX "web"
		#define ARCH_DL_POSTFIX ".wasm"
	#elif defined(_WIN32_WCE)
		#define PLATFORM		"WinCE"
		#define ARCH_DL_POSTFIX ".dll"
	#elif defined(_WIN32)
		#if defined(WINRT)
			#define PLATFORM	"WinRT"		/*those poor poor souls. maybe just maybe I'll actually get the tools for a port, its just a shame that I won't be able to release said port*/
		#elif defined(_XBOX)
			#define PLATFORM	"Xbox"
		#else
			#define PLATFORM	"Win"
		#endif
		#define ARCH_DL_POSTFIX ".dll"
	#elif defined(_WIN16)
		#define PLATFORM		"Win16"
		#define ARCH_DL_POSTFIX ".dll"
	#elif defined(__CYGWIN__)
		#define PLATFORM		"Cygwin"	/*technically also windows*/
		#define ARCH_DL_POSTFIX ".dll"
	#elif defined(ANDROID) || defined(__ANDROID__)
		#define PLATFORM		"Android"	/*technically also linux*/
	#elif defined(__linux__)
		#define PLATFORM		"Linux"
	#elif defined(__APPLE__)
		#include "TargetConditionals.h"
		#if TARGET_IPHONE_SIMULATOR
			 #define PLATFORM	"iOSSim"
		#elif TARGET_OS_IPHONE
			#define PLATFORM	"iOS"
		#elif TARGET_OS_MAC
			#define PLATFORM	"Mac"
		#else
			#define PLATFORM	"Apple"
		#endif
	#elif defined(__FreeBSD__)
		#define PLATFORM	"FreeBSD"
	#elif defined(__OpenBSD__)
		#define PLATFORM	"OpenBSD"
	#elif defined(__NetBSD__)
		#define PLATFORM	"NetBSD"
	#elif defined(BSD)
		#define PLATFORM	"BSD"
	#elif defined(__MORPHOS__)
		#define PLATFORM	"MorphOS"
	#elif defined(__amigaos__)
		#define PLATFORM	"AmigaOS"
	#elif defined(MACOSX)
		#define PLATFORM	"MacOS X"
	#elif defined(__DOS__)
		#define PLATFORM	"Dos"
	#else
		#define PLATFORM	"Unknown"
	#endif
#endif

#ifndef ARCH_DL_POSTFIX
	#define ARCH_DL_POSTFIX ".so"
#endif

#ifdef _DIII4A //karin: remove library name tail
#define ARCH_CPU_POSTFIX ""
#endif
#ifndef ARCH_CPU_POSTFIX
	#if defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)
		#ifdef __ILP32__
			#define ARCH_CPU_POSTFIX "x32"	//32bit pointers, with 16 registers.
		#else
			#ifdef _WIN32
				#define ARCH_CPU_POSTFIX "x64"
			#else
				#define ARCH_CPU_POSTFIX "amd64"
				#define ARCH_ALTCPU_POSTFIX "x86_64"
			#endif
		#endif
	#elif defined(_M_IX86) || defined(__i386__)
		#define ARCH_CPU_POSTFIX "x86"
	#elif defined(__powerpc__) || defined(__ppc__)
		#define ARCH_CPU_POSTFIX "ppc"
	#elif defined(__aarch64__) || defined(__arm64__)
		#define ARCH_CPU_POSTFIX "arm64"
	#elif defined(__arm__)
		#ifdef __SOFTFP__
			#define ARCH_CPU_POSTFIX "arm"
		#else
			#define ARCH_CPU_POSTFIX "armhf"
		#endif
	#else
		#define ARCH_CPU_POSTFIX "unk"
	#endif
#endif

#if defined(_WIN32)
	#define FTE_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__)
	#ifdef __ORDER_BIG_ENDIAN__
		#if (__BYTE_ORDER__==__ORDER_BIG_ENDIAN__) && (__FLOAT_WORD_ORDER__==__ORDER_BIG_ENDIAN__)
			#define FTE_BIG_ENDIAN
		#endif
	#endif
	#ifdef __ORDER_LITTLE_ENDIAN__
		#if (__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__) && (__FLOAT_WORD_ORDER__==__ORDER_LITTLE_ENDIAN__)
			#define FTE_LITTLE_ENDIAN
		#endif
	#endif
#elif defined(__LITTLE_ENDIAN__)
	#define FTE_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN__)
	#define FTE_BIG_ENDIAN
#endif

#ifdef _MSC_VER
	#define VARGS __cdecl
	#define MSVCDISABLEWARNINGS
	#if _MSC_VER >= 1300
		#define FTE_DEPRECATED __declspec(deprecated)
		#ifndef _CRT_SECURE_NO_WARNINGS
			#define _CRT_SECURE_NO_WARNINGS
		#endif
		#ifndef _CRT_NONSTDC_NO_WARNINGS
			#define _CRT_NONSTDC_NO_WARNINGS
		#endif
	#endif
	#define NORETURN __declspec(noreturn)
#endif
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
	#define FTE_DEPRECATED  __attribute__((__deprecated__))	//no idea about the actual gcc version
	#if defined(_WIN32)
		#include <stdio.h>
		#ifdef __MINGW_PRINTF_FORMAT
			#define LIKEPRINTF(x) __attribute__((format(__MINGW_PRINTF_FORMAT,x,x+1)))
		#else
			#define LIKEPRINTF(x) __attribute__((format(ms_printf,x,x+1)))
		#endif
	#else
		#define LIKEPRINTF(x) __attribute__((format(printf,x,x+1)))
	#endif
#endif
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))
	#define NORETURN __attribute__((noreturn))
#endif

//unreachable marks the path leading to it as unreachable too.
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
	#define FTE_UNREACHABLE __builtin_unreachable()
#endif

//I'm making my own restrict, because msvc's headers can't cope if I #define restrict to __restrict, and quite possibly other platforms too
#if __STDC_VERSION__ >= 199901L
	#define fte_restrict restrict
#elif defined(_MSC_VER) && _MSC_VER >= 1400 || __GNUC__ >= 4
	#define fte_restrict __restrict
#else
	#define fte_restrict
#endif

#if _MSC_VER >= 1300
	#define FTE_ALIGN(a) __declspec(align(a))
#elif defined(__clang__)
	#define FTE_ALIGN(a) __attribute__((aligned(a)))
#elif __GNUC__ >= 3
	#define FTE_ALIGN(a) __attribute__((aligned(a)))
#else
	#define FTE_ALIGN(a)
#endif

#if __STDC_VERSION__ >= 201112L
	#include <stdalign.h>
	#define fte_alignof(type) alignof(qintptr_t)
#elif _MSC_VER
	#define fte_alignof(type) __alignof(qintptr_t)
#else
	#define fte_alignof(type) sizeof(qintptr_t)
#endif

//WARNING: FTE_CONSTRUCTOR things are unordered.
#ifdef __cplusplus
	//use standard constructors in any c++ code...
	#define FTE_CONSTRUCTOR(fn) \
		static void fn(void);	\
		class atinit_##fn {atinit_##fn(void){fn();}};	\
		static void fn(void)
#elif _MSC_VER
    #pragma section(".CRT$XCU",read)
    #if _MSC_VER >= 1500	//use '/include' so it doesn't get stripped from linker optimisations
		#define INITIALIZER2_(f,p) \
			static void f(void); \
			__declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
			__pragma(comment(linker,"/include:" p #f "_")) \
			static void f(void)
	#else	// '/include' doesn't exist, hope there's no linker optimisations.
		#define INITIALIZER2_(f,p) \
			static void f(void); \
			__declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
			static void f(void)
	#endif
    #ifdef _WIN64
        #define INITIALIZER(f) INITIALIZER2_(f,"")
    #else
        #define INITIALIZER(f) INITIALIZER2_(f,"_")
    #endif
#else
	//assume gcc/clang...
	#define FTE_CONSTRUCTOR(fn) \
		__attribute__((constructor)) static void fn(void)
#endif


//safeswitch(foo){safedefault: break;}
//switch, but errors for any omitted enum values despite the presence of a default case.
//(gcc will generally give warnings without the default, but sometimes you don't have control over the source of your enumeration values)
//note: android's gcc seems to screw up the pop, instead leaving the warnings enabled, which gets horrendously spammy.
#if (__GNUC__ >= 4) && !defined(ANDROID)
	#define safeswitch	\
		_Pragma("GCC diagnostic push")	\
		_Pragma("GCC diagnostic error \"-Wswitch-enum\"") \
		_Pragma("GCC diagnostic error \"-Wswitch-default\"") \
		switch
	#define safedefault _Pragma("GCC diagnostic pop") default
#else
	#define safeswitch switch
	#define safedefault default
#endif

//fte_inline must only be used in headers, and requires one and ONLY one fte_inlinebody elsewhere.
//fte_inlinebody must be used on a prototype OUTSIDE of a header.
//fte_inlinestatic must not be used inside any headers at all.
#if __STDC_VERSION__ >= 199901L
	//C99 specifies that an inline function is used as a hint. there should be an actual body/copy somewhere (extern inline foo).
	#define fte_inline inline	//must have non-line 'int foo();' somewhere
	#define fte_inlinebody extern inline
	#define fte_inlinestatic static inline
#elif defined(_MSC_VER)
	//msvc will inline like C++. and that's fine.
	#define fte_inline __inline //c++ style
	#define fte_inlinebody
	#define fte_inlinestatic static __inline
#elif (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))
	//gcc will generally inline where it can - so long as its static. but that doesn't stop it warning
	#define fte_inline __attribute__((unused)) static
	#define fte_inlinebody static
	#if __GNUC__ > 5
		#define fte_inlinestatic static inline
	#else
		#define fte_inlinestatic static
	#endif
#else
	//make it static so we at least don't get errors (might still get warnings. see above)
	#define fte_inline static
	#define fte_inlinebody static
	#define fte_inlinestatic static
#endif


#ifndef FTE_DEPRECATED
#define FTE_DEPRECATED
#endif
#ifndef FTE_UNREACHABLE
#define FTE_UNREACHABLE
#endif
#ifndef LIKEPRINTF
#define LIKEPRINTF(x)
#endif
#ifndef VARGS
#define VARGS
#endif
#ifndef NORETURN
#define NORETURN
#endif

#ifdef _WIN32
#define ZEXPORT VARGS
#define ZEXPORTVA VARGS
#endif

#ifdef _DEBUG
	#undef FTE_UNREACHABLE
	#define FTE_UNREACHABLE Sys_Error("Unreachable reached: %s %i\n", __FILE__, __LINE__)
#endif

#ifndef stricmp
	#ifdef _WIN32
		//Windows-specific...
		#define stricmp _stricmp
		#define strnicmp _strnicmp
	#else
		//Posix
		#define stricmp strcasecmp
		#define strnicmp strncasecmp
	#endif
#endif


// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#define	MAX_QPATH		128			// max length of a quake game pathname
#define	MAX_OSPATH		1024		// max length of a filesystem pathname (260 on windows, but needs to be longer for utf8)
#define OLD_MAX_QPATH	64			// it was baked into various file formats, which is unfortunate.

#define	ON_EPSILON		0.1			// point on plane side epsilon

#define	MAX_NQMSGLEN	65536		// max length of a reliable message. FIXME: should be 8000 to play safe with proquake
#define MAX_Q2MSGLEN	1400
#define MAX_QWMSGLEN	1450
#define MAX_OVERALLMSGLEN	65536	// mvdsv sends packets this big
#define	MAX_DATAGRAM	1450		// max length of unreliable message
#define MAX_Q2DATAGRAM	MAX_Q2MSGLEN
#define	MAX_NQDATAGRAM	1024		// max length of unreliable message with vanilla nq protocol
#define MAX_OVERALLDATAGRAM MAX_DATAGRAM

#define MAX_BACKBUFLEN	1200

#ifdef Q1BSPS
#define lightstyleindex_t unsigned short
#else
#define lightstyleindex_t qbyte
#endif
#define INVALID_LIGHTSTYLE ((lightstyleindex_t)(~0u))	//the style that's invalid, signifying to stop adding more.
#define INVALID_VLIGHTSTYLE ((qbyte)(~0u))	//the style that's invalid for verticies, signifying to stop adding more.

//
// per-level limits
//
#ifdef FTE_TARGET_WEB
#define MAX_EDICTS		((1<<15)-1)
#else
#define	MAX_EDICTS		((1<<22)-1)			// expandable up to 22 bits
//#define	MAX_EDICTS		((1<<18)-1)			// expandable up to 22 bits
#endif

#define	MAX_NET_LIGHTSTYLES		(INVALID_LIGHTSTYLE+1)		// 16bit. the last index MAY be used to signify an invalid lightmap in the bsp, but is still valid for rtlights.
#define MAX_STANDARDLIGHTSTYLES 64
#define	MAX_PRECACHE_MODELS		16384		// 14bit.
#define	MAX_PRECACHE_SOUNDS		4096		// 14bit.
#define MAX_SSPARTICLESPRE 1024				// 14bit. precached particle effect names, for server-side pointparticles/trailparticles.
#define MAX_VWEP_MODELS 32

#define	MAX_CSMODELS		2048			// these live entirly clientside
#define MAX_CSPARTICLESPRE	1024

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

#define MAX_Q2EDICTS 1024

//
// stats are integers communicated to the client by the server
//
#define MAX_QW_STATS 32
enum {
#ifdef QUAKESTATS
STAT_HEALTH			= 0,
//STAT_FRAGS		= 1,
STAT_WEAPONMODELI	= 2,
STAT_AMMO			= 3,
STAT_ARMOR			= 4,
STAT_WEAPONFRAME	= 5,
STAT_SHELLS			= 6,
STAT_NAILS			= 7,
STAT_ROCKETS		= 8,
STAT_CELLS			= 9,
STAT_ACTIVEWEAPON	= 10,
STAT_TOTALSECRETS	= 11,
STAT_TOTALMONSTERS	= 12,
STAT_SECRETS		= 13,		// bumped on client side by svc_foundsecret
STAT_MONSTERS		= 14,		// bumped by svc_killedmonster
STAT_ITEMS			= 15,
STAT_VIEWHEIGHT		= 16,	//same as zquake
STAT_TIME			= 17,	//zquake
STAT_MATCHSTARTTIME = 18,
//STAT_UNUSED		= 19,
#ifdef SIDEVIEWS
STAT_VIEW2			= 20,
#endif
STAT_VIEWZOOM		= 21, // DP
#define STAT_VIEWZOOM_SCALE 255
//STAT_UNUSED		= 22,
//STAT_UNUSED		= 23,
//STAT_UNUSED		= 24,
STAT_IDEALPITCH		= 25,	//nq-emu
STAT_PUNCHANGLE_X	= 26,	//nq-emu
STAT_PUNCHANGLE_Y	= 27,	//nq-emu
STAT_PUNCHANGLE_Z	= 28,	//nq-emu
STAT_PUNCHVECTOR_X	= 29,
STAT_PUNCHVECTOR_Y	= 30,
STAT_PUNCHVECTOR_Z	= 31,

#ifdef HEXEN2
//these stats are used only when running a hexen2 mod/hud, and will never be used for a quake mod/hud/generic code.
STAT_H2_LEVEL	= 32,				// changes stat bar
STAT_H2_INTELLIGENCE,				// changes stat bar
STAT_H2_WISDOM,						// changes stat bar
STAT_H2_STRENGTH,					// changes stat bar
STAT_H2_DEXTERITY,					// changes stat bar
STAT_H2_BLUEMANA,					// changes stat bar
STAT_H2_GREENMANA,					// changes stat bar
STAT_H2_EXPERIENCE,					// changes stat bar
#define STAT_H2_CNT_FIRST (STAT_H2_CNT_TORCH)
STAT_H2_CNT_TORCH,					// changes stat bar
STAT_H2_CNT_H_BOOST,				// changes stat bar
STAT_H2_CNT_SH_BOOST,				// changes stat bar
STAT_H2_CNT_MANA_BOOST,				// changes stat bar
STAT_H2_CNT_TELEPORT,				// changes stat bar
STAT_H2_CNT_TOME,					// changes stat bar
STAT_H2_CNT_SUMMON,					// changes stat bar
STAT_H2_CNT_INVISIBILITY,			// changes stat bar
STAT_H2_CNT_GLYPH,					// changes stat bar
STAT_H2_CNT_HASTE,					// changes stat bar
STAT_H2_CNT_BLAST,					// changes stat bar
STAT_H2_CNT_POLYMORPH,				// changes stat bar
STAT_H2_CNT_FLIGHT,					// changes stat bar
STAT_H2_CNT_CUBEOFFORCE,			// changes stat bar
STAT_H2_CNT_INVINCIBILITY,			// changes stat bar
#define STAT_H2_CNT_LAST (STAT_H2_CNT_INVINCIBILITY)
#define STAT_H2_CNT_COUNT (STAT_H2_CNT_LAST+1-STAT_H2_CNT_FIRST)
STAT_H2_ARTIFACT_ACTIVE,
STAT_H2_ARTIFACT_LOW,
STAT_H2_MOVETYPE,
STAT_H2_CAMERAMODE,	//entity
STAT_H2_HASTED,
STAT_H2_INVENTORY,
STAT_H2_RINGS_ACTIVE,

STAT_H2_RINGS_LOW,
STAT_H2_ARMOUR1,
STAT_H2_ARMOUR2,
STAT_H2_ARMOUR3,
STAT_H2_ARMOUR4,
STAT_H2_FLIGHT_T,
STAT_H2_WATER_T,
STAT_H2_TURNING_T,
STAT_H2_REGEN_T,
STAT_H2_PUZZLE1,	//string
STAT_H2_PUZZLE2,	//string
STAT_H2_PUZZLE3,	//string
STAT_H2_PUZZLE4,	//string
STAT_H2_PUZZLE5,	//string
STAT_H2_PUZZLE6,	//string
STAT_H2_PUZZLE7,	//string
STAT_H2_PUZZLE8,	//string
STAT_H2_MAXHEALTH,
STAT_H2_MAXMANA,
STAT_H2_FLAGS,
STAT_H2_PLAYERCLASS,

STAT_H2_OBJECTIVE1,	//integer
STAT_H2_OBJECTIVE2,	//integer
#endif

STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR		= 220, // DP
STAT_MOVEVARS_AIRCONTROL_PENALTY			= 221, // DP
STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW 			= 222, // DP
STAT_MOVEVARS_AIRSTRAFEACCEL_QW 			= 223, // DP
STAT_MOVEVARS_AIRCONTROL_POWER				= 224, // DP
STAT_MOVEFLAGS								= 225, // DP
STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL	= 226, // DP
STAT_MOVEVARS_WARSOWBUNNY_ACCEL				= 227, // DP
STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED			= 228, // DP
STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL			= 229, // DP
STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO	= 230, // DP
STAT_MOVEVARS_AIRSTOPACCELERATE				= 231, // DP
STAT_MOVEVARS_AIRSTRAFEACCELERATE			= 232, // DP
STAT_MOVEVARS_MAXAIRSTRAFESPEED				= 233, // DP
STAT_MOVEVARS_AIRCONTROL					= 234, // DP
STAT_FRAGLIMIT								= 235, // DP
STAT_TIMELIMIT								= 236, // DP
STAT_MOVEVARS_WALLFRICTION					= 237, // DP
STAT_MOVEVARS_FRICTION						= 238, // DP
STAT_MOVEVARS_WATERFRICTION					= 239, // DP
STAT_MOVEVARS_TICRATE						= 240, // DP
STAT_MOVEVARS_TIMESCALE						= 241, // DP
STAT_MOVEVARS_GRAVITY						= 242, // DP
STAT_MOVEVARS_STOPSPEED						= 243, // DP
STAT_MOVEVARS_MAXSPEED						= 244, // DP
STAT_MOVEVARS_SPECTATORMAXSPEED				= 245, // DP
STAT_MOVEVARS_ACCELERATE					= 246, // DP
STAT_MOVEVARS_AIRACCELERATE					= 247, // DP
STAT_MOVEVARS_WATERACCELERATE				= 248, // DP
STAT_MOVEVARS_ENTGRAVITY					= 249, // DP
STAT_MOVEVARS_JUMPVELOCITY					= 250, // DP
STAT_MOVEVARS_EDGEFRICTION					= 251, // DP
STAT_MOVEVARS_MAXAIRSPEED					= 252, // DP
STAT_MOVEVARS_STEPHEIGHT					= 253, // DP
STAT_MOVEVARS_AIRACCEL_QW					= 254, // DP
STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION	= 255, // DP
#endif
	MAX_CL_STATS = 256
};

#ifdef QUAKESTATS
//
// item flags
//
#define	IT_SHOTGUN				(1u<<0)
#define	IT_SUPER_SHOTGUN		(1u<<1)
#define	IT_NAILGUN				(1u<<2)
#define	IT_SUPER_NAILGUN		(1u<<3)

#define	IT_GRENADE_LAUNCHER		(1u<<4)
#define	IT_ROCKET_LAUNCHER		(1u<<5)
#define	IT_LIGHTNING			(1u<<6)
#define	IT_SUPER_LIGHTNING		(1u<<7)

#define	IT_SHELLS				(1u<<8)
#define	IT_NAILS				(1u<<9)
#define	IT_ROCKETS				(1u<<10)
#define	IT_CELLS				(1u<<11)

#define	IT_AXE					(1u<<12)

#define	IT_ARMOR1				(1u<<13)
#define	IT_ARMOR2				(1u<<14)
#define	IT_ARMOR3				(1u<<15)

#define	IT_SUPERHEALTH			(1u<<16)

#define	IT_KEY1					(1u<<17)
#define	IT_KEY2					(1u<<18)

#define	IT_INVISIBILITY			(1u<<19)

#define	IT_INVULNERABILITY		(1u<<20)
#define	IT_SUIT					(1u<<21)
#define	IT_QUAD					(1u<<22)

#define	IT_SIGIL1				(1u<<28)

#define	IT_SIGIL2				(1u<<29)
#define	IT_SIGIL3				(1u<<30)
#define	IT_SIGIL4				(1u<<31)
#endif

//
// print flags
//
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages



//split screen stuff
#ifndef MAX_SPLITS
#define MAX_SPLITS 1u	//disabled, but must be defined for sanities sake.
#endif




//savegame vars
#define	SAVEGAME_COMMENT_LENGTH		39
#define	SAVEGAME_VERSION_NQ			5
#define	SAVEGAME_VERSION_QW			6		//actually zQuake, but the functional difference is that its qw instead of nq.
#define	SAVEGAME_VERSION_FTE_LEG	667		//found in .sav files. this is for legacy-like saved games with multiple players.
#define SAVEGAME_VERSION_FTE_HUB	25000	//found in .fsv files. includes svs.gametype, so bumps should be large.
#define CACHEGAME_VERSION_OLD		513		//lame ordering.
#define CACHEGAME_VERSION_VERBOSE	514		//saved fields got names, making it more extensible.
#define CACHEGAME_VERSION_MODSAVED	515		//qc is responsible for saving all they need to, and restoring it after.


#define PM_DEFAULTSTEPHEIGHT	18


#define dem_cmd			0
#define dem_read		1
#define dem_set			2
#define dem_multiple	3
#define	dem_single		4
#define dem_stats		5
#define dem_all			6


#if 0 //fuck sake, just build it in an older chroot.
/*
glibc SUCKS. 64bit glibc is depending upon glibc 2.14 because of some implementation-defined copy direction change that breaks flash.
or something.
anyway, the actual interface is the same. the old version might be slower, but when updating glibc generally results in also installing systemd, requiring the new version is NOT an option.
*/
#if defined(__GNUC__) && defined(__amd64__) && defined(__linux__) && !defined(FTE_SDL)
	#include <features.h>       /* for glibc version */
	#if defined(__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 14)
		__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
		__asm__(".symver memmove,memmove@GLIBC_2.2.5");
	#endif
	#if defined(__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 29)
		__asm__(".symver exp,exp@GLIBC_2.2.5");
		__asm__(".symver log,log@GLIBC_2.2.5");
		__asm__(".symver pow,pow@GLIBC_2.2.5");
	#endif
#endif
/*end glibc workaround*/
#endif

#endif	//ifndef __BOTHDEFS_H
