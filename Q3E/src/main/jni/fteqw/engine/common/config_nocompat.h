// Build-Config file for Quake-derived total-conversion mods that have chosen to break compatibility.
// to use: make FTE_CONFIG=nocompat

// Features should either be commented or not. If you change undefs to defines or vice versa then expect problems.
// Later code will disable any features if they're not supported on the current platform, so don't worry about win/lin/mac/android/web/etc here - any such issues should be fixed elsewhere.

//general rebranding
//#define DISTRIBUTION			"FTE"							//should be kept short. 8 or less letters is good, with no spaces.
//#define DISTRIBUTIONLONG		"Forethought Entertainment"		//think of this as your company name. It isn't shown too often, so can be quite long.
//#define FULLENGINENAME		"FTE Engine"						//nominally user-visible name.
//#define ENGINEWEBSITE			"http://fte.triptohell.info"	//for shameless self-promotion purposes.
//#define BRANDING_ICON			"fte_eukara.ico"				//The file to use in windows' resource files - for linux your game should include an icon.[png|ico] file in the game's data.

//filesystem rebranding
//#define GAME_SHORTNAME		"fte"			//short alphanumeric description
//#define GAME_FULLNAME			FULLENGINENAME 	//full name of the game we're playing
//#define GAME_BASEGAMES		GAME_SHORTNAME	//comma-separate list of basegame strings to use
//#define GAME_PROTOCOL			"FTE-Generic"	//so other games won't show up in the server browser
//#define GAME_DEFAULTPORT		27500			//slightly reduces the chance of people connecting to the wrong type of server
//#define GAME_IDENTIFYINGFILES	NULL			//with multiple games, this string-list gives verification that the basedir is actually valid. if null, will just be assumed correct.
//#define GAME_DOWNLOADSURL		NULL			//url for the package manger to update from
//#define GAME_DEFAULTCMDS		NULL			//a string containing the things you want to exec in order to override default.cfg 


// Allowed renderers... There should ONLY be undefs here (other C files won't be pulled in automatically)
//#undef GLQUAKE
//#undef D3D8QUAKE
//#undef D3D9QUAKE
//#undef D3D11QUAKE
//#undef VKQUAKE
//#undef HEADLESSQUAKE			//no-op renderer...
//#undef WAYLANDQUAKE			//linux only

//Misc Renderer stuff
//#define PSET_CLASSIC			//support the 'classic' particle system, for that classic quake feel.
#define PSET_SCRIPT				//scriptable particles (both fte's and importing effectinfo)
#define RTLIGHTS
//#define RUNTIMELIGHTING		//automatic generation of .lit files

//Extra misc features.
//#define CLIENTONLY			//
#define MULTITHREAD				//misc basic multithreading - dsound, downloads, basic stuff that's unlikely to have race conditions.
#define LOADERTHREAD			//worker threads for loading misc stuff. falls back on main thread if not supported.
#define AVAIL_DINPUT
//#define SIDEVIEWS   4   		//enable secondary/reverse views.
#define MAX_SPLITS 4u
#define	VERTEXINDEXBYTES	2	//16bit indexes work everywhere but may break some file types, 32bit indexes are optional in gles<=2 and d3d<=9 and take more memory/copying but allow for bigger batches/models. Plugins need to be compiled the same way so this is no longer set per-renderer.
//#define TEXTEDITOR			//my funky text editor! its awesome!
#define PLUGINS					//support for external plugins (like huds or fancy menus or whatever)
//#define USE_SQLITE			//sql-database-as-file support
//#define IPLOG					//track player's ip addresses (any decent server will hide ip addresses, so this probably isn't that useful, but nq players expect it)

//Filesystem formats
#define PACKAGE_PK3				//aka zips. we support utf8,zip64,spans,weakcrypto,(deflate),(bzip2),symlinks. we do not support strongcrypto nor any of the other compression schemes.
//#define PACKAGE_Q1PAK			//also q2
//#define PACKAGE_DOOMWAD		//doom wad support (generates various file names, and adds support for doom's audio, sprites, etc)
//#define AVAIL_XZDEC			//.xz decompression
#define AVAIL_GZDEC				//.gz decompression
#define AVAIL_ZLIB				//whether pk3s can be compressed or not.
//#define AVAIL_BZLIB			//whether pk3s can use bz2 compression
//#define PACKAGE_DZIP			//.dzip support for smaller demos (which are actually more like pak files and can store ANY type of file)

//Map formats
#define Q1BSPS					//Quake1
#define Q2BSPS					//Quake2
#define Q3BSPS					//Quake3, as well as a load of other games too...
#define RFBSPS					//qfusion's bsp format / jk2o etc.
//#define TERRAIN				//FTE's terrain, as well as .map support
//#define DOOMWADS				//map support, filesystem support is separate.
//#define MAP_PROC				//doom3...

//Model formats
#define SPRMODELS				//Quake's sprites
//#define SP2MODELS				//Quake2's models
//#define DSPMODELS				//Doom sprites!
//#define MD1MODELS				//Quake's models.
//#define MD2MODELS				//Quake2's models
//#define MD3MODELS				//Quake3's models, also often used for q1 etc too.
//#define MD5MODELS				//Doom3 models.
//#define ZYMOTICMODELS			//nexuiz uses these, for some reason.
//#define DPMMODELS				//these keep popping up, despite being a weak format.
//#define PSKMODELS				//unreal's interchange format. Undesirable in terms of load times.
//#define HALFLIFEMODELS		//horrible format that doesn't interact well with the rest of FTE's code. Unusable tools (due to license reasons).
#define INTERQUAKEMODELS		//Preferred model format, at least from an idealism perspective.
//#define MODELFMT_MDX			//kingpin's format (for hitboxes+geomsets).
//#define MODELFMT_OBJ			//lame mesh-only format that needs far too much processing and even lacks a proper magic identifier too
//#define MODELFMT_GLTF			//khronos 'transmission format'. .gltf or .glb extension. PBR. Version 2 only, for now.
#define RAGDOLL					//ragdoll support. requires RBE support (via a plugin...).

//Image formats
#define IMAGEFMT_KTX			//Khronos TeXture. common on gles3 devices for etc2 compression
//#define IMAGEFMT_PKM			//file format generally written by etcpack or android's etc1tool. doesn't support mips.
//#define IMAGEFMT_ASTC			//lame simple header around a single astc image. not needed for astc in ktx files etc. its better to use ktx files.
//#define IMAGEFMT_PBM			//pbm/ppm/pgm/pfm family formats.
//#define IMAGEFMT_PSD			//baselayer only.
//#define IMAGEFMT_XCF			//flattens, most of the time
//#define IMAGEFMT_HDR			//an RGBE format.
#define IMAGEFMT_DDS			//.dds files embed mipmaps and texture compression. faster to load.
#define IMAGEFMT_TGA			//somewhat mandatory
#define IMAGEFMT_LMP			//mandatory for quake
#define IMAGEFMT_PNG			//common in quakeworld, useful for screenshots.
//#define IMAGEFMT_JPG			//common in quake3, useful for screenshots.
//#define IMAGEFMT_GIF			//for the luls. loads as a texture2DArray
//#define IMAGEFMT_BLP			//legacy crap
//#define IMAGEFMT_BMP			//windows bmp. yuck. also includes .ico for the luls
//#define IMAGEFMT_PCX			//paletted junk. required for qw player skins, q2 and a few old skyboxes.
//#define IMAGEFMT_EXR			//openexr, via Industrial Light & Magic's rgba api, giving half-float data.
#define AVAIL_PNGLIB			//.png image format support (read+screenshots)
//#define AVAIL_JPEGLIB			//.jpeg image format support (read+screenshots)
//#define AVAIL_STBI			//make use of Sean T. Barrett's lightweight public domain stb_image[_write] single-file-library, to avoid libpng/libjpeg dependancies.
//#define PACKAGE_TEXWAD			//quake's image wad support
#define AVAIL_FREETYPE			//for truetype font rendering
#define DECOMPRESS_ETC2			//decompress etc2(core in gles3/gl4.3) if the graphics driver doesn't support it (eg d3d or crappy gpus with vulkan).
//#define DECOMPRESS_S3TC			//allows bc1-3 to work even when drivers don't support it. This is probably only an issue on mobile chips. WARNING: not entirely sure if all patents expired yet...
//#define DECOMPRESS_RGTC			//bc4+bc5
//#define DECOMPRESS_BPTC			//bc6+bc7
//#define DECOMPRESS_ASTC		//ASTC, for drivers that don't support it properly.

// Game/Gamecode Support
#define CSQC_DAT
#define MENU_DAT
//#define MENU_NATIVECODE		//Use an external dll for menus.
//#define VM_Q1					//q1qvm implementation, to support ktx.
//#define VM_LUA				//optionally supports lua instead of ssqc.
//#define Q2SERVER				//q2 server+gamecode.
//#define Q2CLIENT				//q2 client. file formats enabled separately.
//#define Q3CLIENT				//q3 client stuff.
//#define Q3SERVER				//q3 server stuff.
//#define AVAIL_BOTLIB			//q3 botlib
//#undef BOTLIB_STATIC			//should normally be set only in the makefile, and only if AVAIL_BOTLIB is defined above.
//#define HEXEN2				//runs hexen2 gamecode, supports hexen2 file formats.
//#define HUFFNETWORK			//crappy network compression. probably needs reseeding.
//#define NETPREPARSE			//allows for running both nq+qw on the same server (if not, protocol used must match gamecode).
//#define SUBSERVERS			//Allows the server to fork itself, each acting as an MMO-style server instance of a single 'realm'.
//#define HLCLIENT 7			//we can run HL gamecode (not protocol compatible, set to 6 or 7)
//#define HLSERVER 140			//we can run HL gamecode (not protocol compatible, set to 138 or 140)
#define SAVEDGAMES				//Can save the game.
#define MVD_RECORDING			//server can record MVDs.
//#define ENGINE_ROUTING		//Engine-provided routing logic (possibly threaded)
//#define USE_INTERNAL_BULLET	//Statically link against bullet physics plugin (instead of using an external plugin)
//#define USE_INTERNAL_ODE               //Statically link against ode physics plugin (instead of using an external plugin)

// Networking options
//#define NQPROT				//act as an nq client/server, with nq gamecode.
#define HAVE_PACKET				//we can send unreliable messages!
#define HAVE_TCP				//we can create/accept TCP connections.
#define HAVE_GNUTLS				//on linux
#define HAVE_WINSSPI			//on windows
//#define FTPSERVER				//sv_ftp cvar.
#define WEBCLIENT				//uri_get+any internal downloads etc
#define HAVE_HTTPSV				//net_enable_http/websocket
//#define TCPCONNECT			//support for playing over tcp sockets, instead of just udp. compatible with qizmo.
//#define IRCCONNECT			//lame support for routing game packets via irc server. not a good idea.
#define SUPPORT_ICE				//Internet Connectivity Establishment, for use by plugins to establish voice or game connections.
#define CL_MASTER				//Clientside Server Browser functionality.
//#define PACKAGEMANAGER			//Allows the user to enable/disable/download(with WEBCLIENT) packages and plugins.

// Audio Drivers
#define AVAIL_OPENAL
#define AVAIL_WASAPI			//windows advanced sound api
#define AVAIL_DSOUND
#define HAVE_MIXER				//support non-openal audio drivers

// Audio Formats
#define AVAIL_OGGVORBIS			//.ogg support
//#define AVAIL_MP3_ACM			//.mp3 support (windows only).

// Other Audio Options
#define VOICECHAT
//#define HAVE_SPEEX			//Support the speex codec.
#define HAVE_OPUS				//Support the opus codec.
#define HAVE_MEDIA_DECODER		//can play cin/roq, more with plugins
#define HAVE_MEDIA_ENCODER		//capture/capturedemo work.
//#define HAVE_CDPLAYER			//includes cd playback. actual cds. named/numbered tracks are supported regardless (though you need to use the 'music' command to play them without this).
//#define HAVE_JUKEBOX			//includes built-in jukebox crap
//#define HAVE_SPEECHTOTEXT		//windows speech-to-text thing

// Features required by vanilla quake/quakeworld...
#define QUAKETC
//#define QUAKESTATS			//defines STAT_HEALTH etc. if omitted, you'll need to provide that functionality yourself.
//#define QUAKEHUD				//support for drawing the vanilla hud.
#define QWSKINS					//disabling this means no qw .pcx skins nor enemy/team skin/colour forcing
#define NOBUILTINMENUS
#define NOLEGACY				//just spike trying to kill off crappy crap...
#define USEAREAGRID				//world collision optimisation. REQUIRED for performance with xonotic. hopefully it helps a few other mods too.
//#define NOQCDESCRIPTIONS 2	//if 2, disables writing fteextensions.qc completely. 1 just omits the text. (ignored in debug builds.)

// Outdated stuff
//#define SVRANKING				//legacy server-side ranking system.
////#define QTERM				//qterm... adds a console command that allows running programs from within quake - bit like xterm.
//#define SVCHAT				//ancient lame builtin to support NPC-style chat...
////#define SV_MASTER			//Support running the server as a master server. Should probably not be used.
////#define QUAKESPYAPI			//define this if you want the engine to be usable via gamespy/quakespy, which has been dead for a long time now. forces the client to use a single port for all outgoing connections, which hurts reconnects.


#ifdef COMPILE_OPTS
//things to configure qclib, which annoyingly doesn't include this file itself
//-DOMIT_QCC	//disable the built-in qcc 
//-DSIMPLE_QCVM	//disable qc debugging and 32bit opcodes
#ifndef AVAIL_ZLIB
//-DNO_ZLIB	//disable zlib
#endif
#ifdef AVAIL_PNGLIB
-DLINK_PNG
#endif
#ifdef AVAIL_JPEGLIB
-DLINK_JPEG
#endif

//-DNO_OPUS
//-DNO_SPEEX	//disable static speex
#ifndef AVAIL_BOTLIB
-DNO_BOTLIB	//disable static botlib
#endif
//-DNO_VORBISFILE	//disable static vorbisfile



//-Os		//optimise for size instead of speed. less cpu cache needed means that its sometimes faster anyway.
#endif
