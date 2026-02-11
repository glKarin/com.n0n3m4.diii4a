/*
This file is intended as a set of exports for an NQ-based engine.
This is supported _purely_ for clients, and will not work for servers (safe no-op).



[EndUser] how to use:
to join a qw server: connect "udp:127.0.0.1:27500"
to watch a qtv stream: connect "tcp:3@127.0.0.1:27599"  (where '3' is the streamid)
to watch an mvd demo: connect "demo:blahblah.mvd" - the demo will be loaded from $WORKINGDIR/qw/demos/ - note that $WORKINGDIR is NOT always the same dir
		as your engine is running from. The -basedir argument will break it, or engines that hunt down a 'proper' installation of quake instead.


[Developer] how to incorporate into an nq engine:
copy this file (net_qtv.h) into your source directory, next to your other net_*.h files
load up net_win.c
find the #include "net_wins.h" line.
dupe it, call it net_qtv.h
find the net_landrivers array. Dupe the first block, then edit the first block to be all QTV_foo functions.
bump net_numlandrivers.
For non-window operating systems, you'll need to do the same, just figure out which net_win.c equivelent function it uses first. :P
certain engines may do weird things with the port. probably its best to just use Cmd_Args() for the connect command instead of Cmd_Argv(1), and to add
		port parsing to XXXX_GetAddrFromName instead of messing around with port cvars etc and ruining server configs.
		If your engine already has weird port behaviour, then its entirely your problem to fix. :P
You probably want to tweak your menus a little to clean up the nq/qw/qtv connection distinctions.
If you do want to make changes to libqtv, please consider joining the FTE team (or at least the irc channel) in order to contribute without forking.

[Developer] how to compile libqtv:
cflags MUST define 'LIBQTV' or it won't compile properly.
The relevent exports are all tagged as 'EXPORT void PUBLIC fname(...)' (dllexport+cdecl in windows), feel free to define those properly if you're making a linux shared object without exporting all (potentially conflicting) internals.
This means you can compile it as a dll without any issues, one with a standardized interface. Any libqtv-specific bugfixes can be released independantly from engine(s).
Compiling a dll with msvc will generally automatically produce a .lib which you can directly link against. Alternatively, include both projects in your workspace and set up dependancies properly and it'll be automatically imported.

[PowerUser] issues:
its a full qtv proxy, but you can't get admin/rcon access to it.
it doesn't read any configs, and has no console, thus you cannot set an rcon password/port.
without console/rcon, you cannot enable any listening ports for other users.
if you need a public qtv proxy, use a standalone version.
*/

//#define EXPORT is defined declspec export for dll builds.
#ifdef _WIN32
#define PUBLIC __cdecl
#endif

#ifndef EXPORT
#define EXPORT
#endif
#ifndef PUBLIC
#define PUBLIC
#endif
#define PUBEXPORT EXPORT

//vanilla 'net_win.c' datagram driver function listings
PUBEXPORT int PUBLIC QTV_Init (void);
PUBEXPORT void PUBLIC QTV_Shutdown (void);
PUBEXPORT void PUBLIC QTV_Listen (qboolean state);
PUBEXPORT int PUBLIC QTV_OpenSocket (int port);
PUBEXPORT int PUBLIC QTV_CloseSocket (int socket);
PUBEXPORT int PUBLIC QTV_Connect (int socket, struct qsockaddr *addr);
PUBEXPORT int PUBLIC QTV_CheckNewConnections (void);
PUBEXPORT int PUBLIC QTV_Read (int socket, byte *buf, int len, struct qsockaddr *addr);
PUBEXPORT int PUBLIC QTV_Write (int socket, byte *buf, int len, struct qsockaddr *addr);
PUBEXPORT int PUBLIC QTV_Broadcast (int socket, byte *buf, int len);
PUBEXPORT char *PUBLIC QTV_AddrToString (struct qsockaddr *addr);
PUBEXPORT int PUBLIC QTV_StringToAddr (char *string, struct qsockaddr *addr);	//port is part of the string. libqtv will use its own default port.
PUBEXPORT int PUBLIC QTV_GetSocketAddr (int socket, struct qsockaddr *addr);
PUBEXPORT int PUBLIC QTV_GetNameFromAddr (struct qsockaddr *addr, char *name);
PUBEXPORT int PUBLIC QTV_GetAddrFromName (char *name, struct qsockaddr *addr);
PUBEXPORT int PUBLIC QTV_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2);
PUBEXPORT int PUBLIC QTV_GetSocketPort (struct qsockaddr *addr);
PUBEXPORT int PUBLIC QTV_SetSocketPort (struct qsockaddr *addr, int port);

//additional functions for enhanced engines that changed the net_dgrm system api or other uses.
PUBEXPORT void PUBLIC QTV_Command (char *command, char *resulttext, int resultlen);	//sends a console command to the qtv proxy. you can make it a formal proxy with this. must have been inited.
PUBEXPORT void PUBLIC QTV_RunFrame (void);	//runs a proxy frame without needing a connection to be active.
PUBEXPORT int PUBLIC QTV_StringPortToAddr (char *string, int port, struct qsockaddr *addr);	//port may be part of the string. the specified port will be used, or just pass 0 for libqtv to make it up. generally you'll want to pass the 'port' cvar for port, and the exact argument that was typed in the string arg.