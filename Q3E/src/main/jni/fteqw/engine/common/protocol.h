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
// protocol.h -- communications protocols

#include "bothdefs.h"

#define PEXT_SETVIEW			0x00000001

#define PEXT_SCALE				0x00000002
#define PEXT_LIGHTSTYLECOL		0x00000004
#define PEXT_TRANS				0x00000008
#define PEXT_VIEW2_				0x00000010
//#define PEXT_BULLETENS			0x00000020 //obsolete
#define PEXT_ACCURATETIMINGS	0x00000040
#define PEXT_SOUNDDBL			0x00000080	//revised startsound protocol
#define PEXT_FATNESS			0x00000100	//GL only (or servers)
#define PEXT_HLBSP				0x00000200
#define PEXT_TE_BULLET			0x00000400
#define PEXT_HULLSIZE			0x00000800
#define PEXT_MODELDBL			0x00001000
#define PEXT_ENTITYDBL			0x00002000	//max of 1024 ents instead of 512
#define PEXT_ENTITYDBL2			0x00004000	//max of 1024 ents instead of 512
#define PEXT_FLOATCOORDS		0x00008000	//supports floating point origins.
//#define PEXT_VWEAP				0x00010000	//cause an extra qbyte to be sent, and an extra list of models for vweaps.
#define PEXT_Q2BSP_				0x00020000
#define PEXT_Q3BSP_				0x00040000

#define PEXT_COLOURMOD			0x00080000	//this replaces an older value which would rarly have caried any actual data.

#define PEXT_SPLITSCREEN		0x00100000
#define PEXT_HEXEN2				0x00200000	//more stats and working particle builtin.
#define PEXT_SPAWNSTATIC2		0x00400000	//Sends an entity delta instead of a baseline.
#define PEXT_CUSTOMTEMPEFFECTS	0x00800000	//supports custom temp ents.
#define PEXT_256PACKETENTITIES	0x01000000	//Client can recieve 256 packet entities.
//#define PEXT_NEVERUSED		0x02000000	//reserved for a future multicastmask
#define PEXT_SHOWPIC			0x04000000
#define PEXT_SETATTACHMENT		0x08000000	//md3 tags (needs networking, they need to lerp).
//#define PEXT_NEVERUSED		0x10000000	//reserved for a future multicastmask
#define PEXT_CHUNKEDDOWNLOADS	0x20000000	//alternate file download method. Hopefully it'll give quadroupled download speed, especially on higher pings.
#define PEXT_CSQC				0x40000000	//csqc additions
#define PEXT_DPFLAGS			0x80000000	//extra flags for viewmodel/externalmodel and possible other persistant style flags.
#define PEXT_SERVERADVERTISE	~0u
#define PEXT_CLIENTSUPPORT		(PEXT_SETVIEW|PEXT_SCALE|PEXT_LIGHTSTYLECOL|PEXT_TRANS|PEXT_VIEW2_|PEXT_ACCURATETIMINGS|PEXT_SOUNDDBL|PEXT_FATNESS|PEXT_HLBSP|PEXT_TE_BULLET|PEXT_HULLSIZE|PEXT_MODELDBL|PEXT_ENTITYDBL|PEXT_ENTITYDBL2|PEXT_FLOATCOORDS|PEXT_Q2BSP_|PEXT_Q3BSP_|PEXT_COLOURMOD|PEXT_SPLITSCREEN|PEXT_HEXEN2|PEXT_SPAWNSTATIC2|PEXT_CUSTOMTEMPEFFECTS|PEXT_256PACKETENTITIES|PEXT_SHOWPIC|PEXT_SETATTACHMENT|PEXT_CHUNKEDDOWNLOADS|PEXT_CSQC|PEXT_DPFLAGS)

#ifdef CSQC_DAT
	#define PEXT_BIGUSERINFOS	PEXT_CSQC	//FIXME: while useful for csqc, we should include something else that isn't so often stripped, or is available in ezquake, or something.
#else
	#define PEXT_BIGUSERINFOS	0xffffffff
#endif
#ifdef SIDEVIEWS
	#define PEXT_VIEW2			PEXT_VIEW2_
#endif
#ifdef Q2BSPS
	#define PEXT_Q2BSP			PEXT_Q2BSP_
#endif
#ifdef Q3BSPS
	#define PEXT_Q3BSP			PEXT_Q3BSP_
#endif
#define PEXT1_HIDEPROTOCOLS		(PEXT_Q3BSP_|PEXT_Q2BSP_|PEXT_HLBSP)	//These are hints for the server, and not useful to the client (they can figure stuff out themselves)
//#define	PEXT1_DEPRECATED			(PEXT_SCALE|PEXT_TRANS|PEXT_ACCURATETIMINGS|PEXT_SOUNDDBL|PEXT_FATNESS|PEXT_HULLSIZE|PEXT_MODELDBL|PEXT_ENTITYDBL|PEXT_ENTITYDBL2|PEXT_COLOURMOD|PEXT_SPAWNSTATIC2|PEXT_256PACKETENTITIES|PEXT_SETATTACHMENT|PEXT_DPFLAGS)	//deprecated by replacementdeltas
#define PEXT1_MVDSUPPORT			(PEXT1_CLIENTSUPPORT&~PEXT1_DEPRECATED&~PEXT1_HIDEPROTOCOLS)	//pext2 extensions to use when recording mvds.

#define PEXT2_PRYDONCURSOR			0x00000001
#define PEXT2_VOICECHAT				0x00000002
#define PEXT2_SETANGLEDELTA			0x00000004
#define PEXT2_REPLACEMENTDELTAS		0x00000008	//weaponframe was part of the entity state. that flag is now the player's v_angle.
#define PEXT2_MAXPLAYERS			0x00000010	//Client is able to cope with more players than 32. abs max becomes 255, due to colormap issues.
#define PEXT2_PREDINFO				0x00000020	//movevar stats, NQ input sequences+acks.
#define PEXT2_NEWSIZEENCODING		0x00000040	//richer size encoding.
#define PEXT2_INFOBLOBS				0x00000080	//serverinfo+userinfo lengths can be MUCH higher (protocol is unbounded, but expect low sanity limits on userinfo), and contain nulls etc.
#define PEXT2_STUNAWARE				0x00000100	//changes the netchan to biased-bigendian (so lead two bits are 1 and not stun's 0, so we don't get confused)
#define PEXT2_VRINPUTS				0x00000200	//clc_move changes, more buttons etc. vr stuff!
#define PEXT2_LERPTIME				0x00000400	//fitz-bloat parity. redefines UF_16BIT as UF_LERPEND in favour of length coding.
#define PEXT2_SERVERADVERTISE		~0u
#define PEXT2_CLIENTSUPPORT			(PEXT2_PRYDONCURSOR|PEXT2_VOICECHAT|PEXT2_SETANGLEDELTA|PEXT2_REPLACEMENTDELTAS|PEXT2_MAXPLAYERS|PEXT2_PREDINFO|PEXT2_NEWSIZEENCODING|PEXT2_INFOBLOBS|PEXT2_STUNAWARE|PEXT2_VRINPUTS|PEXT2_LERPTIME) //warn if we see bits not listed here.
#define PEXT2_DEPRECATEDORNEW		(PEXT2_INFOBLOBS|PEXT2_VRINPUTS|PEXT2_LERPTIME) //extensions that are outdated
#define PEXT2_MVDSUPPORT			(PEXT2_CLIENTSUPPORT&~PEXT2_DEPRECATED&~PEXT2_STUNAWARE)	//pext2 extensions to use when recording mvds.

#define PEXT2_LONGINDEXES			0	//boosts the maximum player+stat index.

//EzQuake/Mvdsv extensions. (use ezquake name, to avoid confusion about .mvd format and its protocol differences)
#define EZPEXT1_FLOATENTCOORDS		0x00000001	//quirky - doesn't apply to broadcasts, just players+ents. this gives more precision, but will bug out if you try using it to increase map bounds in ways that may not be immediately apparent. iiuc this was added instead of fixing some inconsistent rounding...
#define EZPEXT1_SETANGLEREASON		0x00000002	//specifies the reason for an svc_setangles call. the mvdsv implementation will fuck over any mods that writebyte them. we'd need to modify our preparse stuff to work around the issue.
//#define EZPEXT1_SERVERSIDEWEAPON	0x00000004	//looks half-baked. would be better to predict grabs clientside (oh noes! backpack knowledge!).
//#define EZPEXT1_DEBUG_WEAPON		0x00000008	//debug? not gonna bother.
//#define EZPEXT1_DEBUG_ANTILAG		0x00000010	//debug? not gonna bother.
#define EZPEXT1_HIDDEN_MESSAGES		0x00000020	//mvd bloat. shouldn't be seen on actual servers.


//#define MVD_PEXT1_SERVERSIDEWEAPON  (1 <<  2) // Server-side weapon selection
#define MVD_PEXT1_DEBUG_WEAPON      (1 <<  3) // Send weapon-choice explanation to server for logging
#define MVD_PEXT1_DEBUG_ANTILAG     (1 <<  4) // Send predicted positions to server (compare to antilagged positions)
#define MVD_PEXT1_HIDDEN_MESSAGES   (1 <<  5) // dem_multiple(0) packets are in format (<length> <type-id>+ <packet-data>)*


#define EZPEXT1_SERVERADVERTISE		(EZPEXT1_FLOATENTCOORDS/* - implemented, but interactions with replacementdeltas is not defined*/ /*|EZPEXT1_SETANGLEREASON - potentially causes compat issues with mods that stuffcmd it (common in nq)*/)
#define EZPEXT1_CLIENTADVERTISE		EZPEXT1_FLOATENTCOORDS			//might as well ask for it, as a way around mvdsv's writecoord/PM_NudgePosition rounding difference bug.
#define EZPEXT1_CLIENTSUPPORT		(EZPEXT1_FLOATENTCOORDS|EZPEXT1_SETANGLEREASON|EZPEXT1_HIDDEN_MESSAGES)	//ones we can support in demos. warning if other bits.

//ZQuake transparent protocol extensions.
#define Z_EXT_PM_TYPE		(1<<0)	// basic PM_TYPE functionality (reliable jump_held)
#define Z_EXT_PM_TYPE_NEW	(1<<1)	// adds PM_FLY, PM_SPECTATOR
#define Z_EXT_VIEWHEIGHT	(1<<2)	// STAT_VIEWHEIGHT
#define Z_EXT_SERVERTIME	(1<<3)	// STAT_TIME
#define Z_EXT_PITCHLIMITS	(1<<4)	// serverinfo maxpitch & minpitch
#define Z_EXT_JOIN_OBSERVE	(1<<5)	// server: "join" and "observe" commands are supported
									// client: on-the-fly spectator <-> player switching supported

#define Z_EXT_PF_ONGROUND	(1<<6)	// server: PF_ONGROUND is valid for all svc_playerinfo
#define Z_EXT_VWEP			(1<<7)
#define Z_EXT_PF_SOLID		(1<<8)	//conflicts with many FTE extensions.

#ifdef QUAKESTATS
#define SERVER_SUPPORTED_Z_EXTENSIONS (Z_EXT_PM_TYPE|Z_EXT_PM_TYPE_NEW|Z_EXT_VIEWHEIGHT|Z_EXT_SERVERTIME|Z_EXT_PITCHLIMITS|Z_EXT_JOIN_OBSERVE|Z_EXT_PF_ONGROUND|Z_EXT_VWEP|Z_EXT_PF_SOLID)
#else
#define SERVER_SUPPORTED_Z_EXTENSIONS (Z_EXT_PM_TYPE|Z_EXT_PM_TYPE_NEW|Z_EXT_PITCHLIMITS|Z_EXT_JOIN_OBSERVE|Z_EXT_PF_ONGROUND|Z_EXT_VWEP|Z_EXT_PF_SOLID)
#endif
#define BUGGY_EZQUAKE_Z_EXTENSIONS    (Z_EXT_PF_ONGROUND|Z_EXT_PF_SOLID) //ezquake bugs out on these when ANY fteextension is present. hack the serverinfo to hide these.
#define CLIENT_SUPPORTED_Z_EXTENSIONS (SERVER_SUPPORTED_Z_EXTENSIONS|Z_EXT_PF_ONGROUND|Z_EXT_PF_SOLID)


#define PROTOCOL_VERSION_VARLENGTH		(('v'<<0) + ('l'<<8) + ('e'<<16) + ('n' << 24))	//variable length handshake

#define PROTOCOL_VERSION_FTE1			(('F'<<0) + ('T'<<8) + ('E'<<16) + ('X' << 24))	//fte extensions.
#define PROTOCOL_VERSION_FTE2			(('F'<<0) + ('T'<<8) + ('E'<<16) + ('2' << 24))	//fte extensions.
#define PROTOCOL_VERSION_EZQUAKE1		(('M'<<0) + ('V'<<8) + ('D'<<16) + ('1' << 24)) //ezquake/mvdsv extensions
#define PROTOCOL_VERSION_HUFFMAN		(('H'<<0) + ('U'<<8) + ('F'<<16) + ('F' << 24))	//packet compression
#define PROTOCOL_VERSION_FRAGMENT		(('F'<<0) + ('R'<<8) + ('A'<<16) + ('G' << 24))	//supports fragmentation/packets larger than 1450
#ifdef HAVE_DTLS
#define PROTOCOL_VERSION_DTLSUPGRADE	(('D'<<0) + ('T'<<8) + ('L'<<16) + ('S' << 24))	//server supports dtls. clients should dtlsconnect THEN continue connecting (also allows dtls rcon!).
#endif

#define PROTOCOL_INFO_GUID				(('G'<<0) + ('U'<<8) + ('I'<<16) + ('D' << 24))	//globally 'unique' client id info.

#define	PROTOCOL_VERSION_QW				28

#define	PROTOCOL_VERSION_Q2_DEMO_MIN	26	//we can parse this server
#define	PROTOCOL_VERSION_Q2_MIN			31	//we can join these outdated servers
#define	PROTOCOL_VERSION_Q2				34	//we host this
#define	PROTOCOL_VERSION_Q2EXDEMO		2022
#define	PROTOCOL_VERSION_Q2EX			2023
#define	PROTOCOL_VERSION_R1Q2			35
#define	PROTOCOL_VERSION_Q2PRO			36

#define	PROTOCOL_VERSION_Q2_DEMO_MAX	PROTOCOL_VERSION_Q2PRO

//=========================================

//#define	GAME_DEFAULTPORT	XXXXX	//rebranding allows selection of a different default port, which slightly reduces protocol conflicts.
#define	PORT_NQSERVER	26000
#define PORT_DPMASTER	PORT_Q3MASTER
#define	PORT_QWCLIENT	27001
#define	PORT_QWMASTER	27000
#define	PORT_QWSERVER	27500
#define PORT_H2SERVER	26900
#define PORT_Q2MASTER	27900
#define PORT_Q2CLIENT	27901
#define PORT_Q2SERVER	27910
#define PORT_Q2EXSERVER	5069
#define PORT_Q3MASTER	27950
#define PORT_Q3SERVER	27960
#define PORT_ICEBROKER	PORT_DPMASTER

#ifdef GAME_DEFAULTPORT
	#define PORT_DEFAULTSERVER	GAME_DEFAULTPORT
#else
	#define PORT_DEFAULTSERVER	PORT_QWSERVER
#endif

//=========================================

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will always be \n if the message isn't a single
// qbyte long (?? not true anymore?)

#define	S2C_CHALLENGE		'c'
#define	S2C_CONNECTION		'j'
#define	A2A_PING			'k'	// respond with an A2A_ACK
#define	A2A_ACK				'l'	// general acknowledgement without info
#define	A2A_NACK			'm'	// [+ comment] general failure
#define A2A_ECHO			'e' // for echoing
#define	A2C_PRINT			'n'	// print a message on client

#define	S2M_HEARTBEAT		'a'	// + serverinfo + userlist + fraglist
#define	A2C_CLIENT_COMMAND	'B'	// + command line
#define	S2M_SHUTDOWN		'C'

#define C2M_MASTER_REQUEST  'c'
#define M2C_MASTER_REPLY	'd'	// + \n + qw server port list

//for S2C 'status' packets.
#define	STATUS_OLDSTYLE					0 //equivelent to STATUS_SERVERINFO|STATUS_PLAYERS
#define	STATUS_SERVERINFO				1
#define	STATUS_PLAYERS					2
#define	STATUS_SPECTATORS				4
#define	STATUS_SPECTATORS_AS_PLAYERS	8 //for ASE - change only frags: show as "S"
#define	STATUS_SHOWTEAMS				16
#define	STATUS_QTVLIST					32 //qtv destid "name" "streamid@host:port" numviewers
#define STATUS_LOGININFO				64

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#define	svc_bad						0
#define	svc_nop						1
#define	svc_disconnect				2
#define	svcqw_updatestatbyte		3	// [qbyte] [qbyte]
#define	svcnq_updatestatlong		3	// [qbyte] [long]
#define	svc_version					4	// [long] server version
#define	svc_setview					5	// [short] entity number
#define	svc_sound					6	// <see code>
#define	svc_time					7	// [float] server time
#define	svc_print					8	// [qbyte] id [string] null terminated string
#define	svc_stufftext				9	// [string] stuffed into client's console buffer
										// the string should be \n terminated
#define	svc_setangle				10	// [angle3] set the view angle to this absolute value

#define	svc_serverdata				11	// [long] protocol ...
#define	svc_lightstyle				12	// [qbyte] [string]
#define	svc_updatename				13	// [qbyte] [string]
#define	svc_updatefrags				14	// [qbyte] [short]
#define	svcnq_clientdata			15	// <shortbits + data>
#define	svc_stopsound				16	// <see code>
#define	svc_updatecolors			17	// [qbyte] [qbyte] [qbyte]
#define	svc_particle				18	// [vec3] <variable>
#define	svc_damage					19

#define	svc_spawnstatic				20
#define	svcfte_spawnstatic2			21
#define	svc_spawnbaseline			22

#define	svc_temp_entity				23	// variable
#define	svc_setpause				24	// [qbyte] on / off
#define	svcnq_signonnum				25	// [qbyte]  used for the signon sequence
#define	svcfte_splitscreenconfig	25	// [qbyte] seats, player[seat]. spectator flags passed via userinfo.

#define	svc_centerprint				26	// [string] to put in center of the screen

#define	svc_killedmonster			27
#define	svc_foundsecret				28

#define	svc_spawnstaticsound		29	// [coord3] [qbyte] samp [qbyte] vol [qbyte] aten

#define	svc_intermission			30		// [vec3_t] origin [vec3_t] angle
#define	svc_finale					31		// [string] text

#define	svc_cdtrack					32		// [qbyte] track
#define svc_sellscreen				33

#define svc_cutscene				34	//hmm... nq only... added after qw tree splitt?



//QW svcs
#define	svc_smallkick				34		// set client punchangle to 2
#define	svc_bigkick					35		// set client punchangle to 4

#define	svc_updateping				36		// [qbyte] [short]
#define	svc_updateentertime			37		// [qbyte] [float]

#define	svcqw_updatestatlong		38		// [qbyte] [long]

#define	svc_muzzleflash				39		// [short] entity

#define	svc_updateuserinfo			40		// [qbyte] slot [long] uid [string] userinfo

#define	svc_download				41		// [short] size [size bytes]
#define	svc_playerinfo				42		// variable
#define	svc_nails					43		// [qbyte] num [48 bits] xyzpy 12 12 12 4 8
#define	svc_chokecount				44		// [qbyte] packets choked
#define	svc_modellist				45		// [strings]
#define	svc_soundlist				46		// [strings]
#define	svc_packetentities			47		// [...]
#define	svc_deltapacketentities		48		// [...]
#define svc_maxspeed				49		// maxspeed change, for prediction
#define svc_entgravity				50		// gravity change, for prediction
#define svc_setinfo					51		// setinfo on a client
#define svc_serverinfo				52		// serverinfo
#define svc_updatepl				53		// [qbyte] [qbyte]

//mvdsv extended svcs (for mvd playback)
#define svc_nails2					54		//qwe - [qbyte] num [52 bits] nxyzpy 8 12 12 12 4 8

//FTE extended svcs
#ifdef PEXT_SOUNDDBL
#define svcfte_soundextended		55
#define svcfte_soundlistshort		56
#endif
#ifdef PEXT_LIGHTSTYLECOL
#define svcfte_lightstylecol		57
#endif

//#define svcfte_svcremoved			58

//#define	svcfte_svcremoved		59

#ifdef PEXT_MODELDBL
#define	svcfte_modellistshort		60		// [strings]
#endif

//#define svc_ftesetclientpersist	61	//ushort DATA

#define svc_setportalstate			62

#define	svcfte_particle2			63
#define	svcfte_particle3			64
#define	svcfte_particle4			65
#define svcfte_spawnbaseline2		66

#define	svcfte_customtempent		67

#define svcfte_choosesplitclient	68
#define svcfte_showpic				69
#define svcfte_hidepic				70
#define svcfte_movepic				71
#define svcfte_updatepic			72

//73

#define svcfte_effect				74		// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
#define svcfte_effect2				75		// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate

#define svcfte_csqcentities			76	//entity lump for csqc

#define svcfte_precache				77

#define svcfte_updatestatstring		78
#define svcfte_updatestatfloat		79

#define svcfte_trailparticles		80		// [short] entnum [short] effectnum [vector] start [vector] end
#define svcfte_pointparticles		81		// [short] effectnum [vector] start [vector] velocity [short] count
#define svcfte_pointparticles1		82		// [short] effectnum [vector] start, same as svc_pointparticles except velocity is zero and count is 1

#define svcfte_cgamepacket			83
#define svcfte_voicechat			84
#define	svcfte_setangledelta		85	// [angle3] add this to the current viewangles
#define svcfte_updateentities		86
#define svcfte_brushedit			87	// networked brush editing, paired with clcfte_brushedit.
#define	svcfte_updateseats			88	// byte count, byte playernum[count]
#define svcfte_setinfoblob			89	// [8] 1-based index [string] key [32] isfinal<<31|offset [16] chunksize [chunksize] data
#define svcfte_cgamepacket_sized	90	//svcfte_cgamepacket with an extra short size right after the svc.
#define	svcfte_temp_entity_sized	91	//svc_temp_entity with an extra short size right after the svc (high bit means nq, unset means qw).
#define svcfte_csqcentities_sized	92	//entity lump for csqc (with size info)
#define svcfte_setanglebase			93	//updates the base angle (and optionally locks the view, otherwise nudging it without race conditions.)
#define svcfte_spawnstaticsound2	94	//*sigh*

//fitz svcs
#define svcfitz_skybox				37
#define svcfitz_bf					40
#define svcfitz_fog					41
#define svcfitz_spawnbaseline2		42
#define svcfitz_spawnstatic2		43
#define svcfitz_spawnstaticsound2	44

//nehahra svcs
#define svcneh_skyboxsize			50  // [coord] size (default is 4096)
#define svcneh_fog					51	// [byte] enable <optional past this point, only included if enable is true> [float] density [byte] red [byte] green [byte] blue

//QuakeEx(aka: rerelease) svcs.
//	these are not really documented anywhere. we're trying to stick with protocol 15 (because that's the only documented protocol it supports properly thanks to demos)
//	however we still need some special case svcs
//  (there's also some problematic c2s differences too)
#define svcqex_updateping			46	// [byte] slot, [signed_qe] ping
#define svcqex_updatesocial			47	// [byte] slot, 8 bytes of unknown
#define svcqex_updateplinfo			48	// [byte] slot, [leb128] health, [leb128] armour
#define svcqex_locprint				49	// uses qe's localised string formatting, otherwise treat as svc_print.
#define svcqex_servervars			50	// [leb128] changedvalues, [???] value...
#define svcqex_seq					51	// [leb128] input sequence ack
#define svcqex_achievement			52	// [string] codename
#define svcqex_chat					53
#define svcqex_levelcompleted		54
#define svcqex_backtolobby			55
#define svcqex_localsound			56
#define svcqex_prompt				57
#define svcqex_loccenterprint		58	// [string] codename

//DP extended svcs
#define svcdp_downloaddata			50
#define svcdp_updatestatbyte		51
#define svcnq_effect				52		// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
#define svcnq_effect2				53		// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate
#define	svcdp_precache				54		// [short] precacheindex [string] filename, precacheindex is + 0 for modelindex and +32768 for soundindex
#define svcdp_spawnbaseline2		55
#define svcdp_spawnstatic2			56
#define svcdp_entities				57
#define svcdp_csqcentities			58
#define	svcdp_spawnstaticsound2		59	// [coord3] [short] samp [byte] vol [byte] aten
#define svcdp_trailparticles		60		// [short] entnum [short] effectnum [vector] start [vector] end
#define svcdp_pointparticles		61		// [short] effectnum [vector] start [vector] velocity [short] count
#define svcdp_pointparticles1		62		// [short] effectnum [vector] start, same as svc_pointparticles except velocity is zero and count is 1



#define svc_invalid			256


enum clustercmdops_e
{
	ccmd_bad = 0,			//abort!
	ccmd_stuffcmd,		//regular ol stuffcmd
			//string concommand
	ccmd_setcvar,		//master->server to order a cvar change.
	ccmd_print,
			//string message
	ccmd_acceptserver,
			//serverid
	ccmd_lostplayer,	//player dropped/timed out
			//long plid
	ccmd_foundplayer,	//server->master, saying that a player tried to connect directly (and we need their info)
			//string name
			//string clientaddress
			//string guid
	ccmd_takeplayer,	//master->server, saying to allocate a slot for a player.
			//long plid
			//long fromsvid (0=no reply needed)
			//byte statcount
			//float stats[statcount]
	ccmd_transferplayer,	//server->master, asking to move them to a new server.
			//long plid
			//string map
			//byte ipv4=0, ipv6=1
			//byte statcount
			//float stats[statcount]
	ccmd_transferedplayer,	//master->server, saying the transfer was completed. original server no longer owns the player.
			//long toserver,
			//long playerid
	ccmd_tookplayer,	//server->master->server, saying that a player was taken.
			//long svid (this is always the *other* server)
			//long plid
			//string addr (this is the client's address when sent to the master, and the server's address that took the message in the message to the source server)
	ccmd_transferabort,	//server->master->server, saying that a player was rejected.
			//long plid
			//long fromsvid
			//string server
	ccmd_saveplayer,	//server->master, saves a player's stats.
			//long plid
			//byte statcount
			//float stats[statcount]
	ccmd_serveraddress,	//server->master, contains a few net addresses
			//string addresses[]
			//byte 0
	ccmd_stringcmd,
			//string dest (black = broadcast to all)
			//string source (player name)
			//string cmd (type of event, handled by receiving server/forwarded to client)
			//string msg (extra info, like the typed text)
};


enum svcq2_ops_e
{
	svcq2_bad,	//0

	// these ops are known to the game dll
	svcq2_muzzleflash,	//1
	svcq2_muzzleflash2,	//2
	svcq2_temp_entity,	//3
	svcq2_layout,		//4
	svcq2_inventory,	//5

	// the rest are private to the client and server
	svcq2_nop,			//6
	svcq2_disconnect,	//7
	svcq2_reconnect,	//8
	svcq2_sound,		//9			// <see code>
	svcq2_print,		//10			// [qbyte] id [string] null terminated string
	svcq2_stufftext,	//11			// [string] stuffed into client's console buffer, should be \n terminated
	svcq2_serverdata,	//12			// [long] protocol ...
	svcq2_configstring,	//13		// [short] [string]
	svcq2_spawnbaseline,//14
	svcq2_centerprint,	//15		// [string] to put in center of the screen
	svcq2_download,		//16		// [short] size [size bytes]
	svcq2_playerinfo,	//17			// variable
	svcq2_packetentities,//18			// [...]
	svcq2_deltapacketentities,//19	// [...]
	svcq2_frame,			//20 (the bastard to implement.)

	svcq2ex_splitclient = 21,
	svcq2ex_configblast = 22,
	svcq2ex_spawnbaselineblast = 23,
	svcq2ex_levelrestart = 24,
	svcq2ex_danage = 25,
	svcq2ex_locprint = 26,
	svcq2ex_fog = 27,
	svcq2ex_waiting = 28,
	svcq2ex_botchat = 29,
	svcq2ex_mapmarker = 30,
	svcq2ex_routemarker = 31,
	svcq2ex_muzzleflash3 = 32,
	svcq2ex_achievement = 33,


	svcr1q2_zpacket = 21,
	svcr1q2_zdownload = 22,
	svcr1q2_playerupdate = 23,
	svcr1q2_setting = 24,
	svcq2pro_gamestate = 23,
	svcq2pro_setting = 24,
};

enum clcq2_ops_e
{
	clcq2_bad = 0,
	clcq2_nop = 1,
	clcq2_move = 2,			// [[usercmd_t]
	clcq2_userinfo = 3,		// [[userinfo string]
	clcq2_stringcmd = 4,	// [string] message

	clcr1q2_setting = 5,	// [setting][value] R1Q2 settings support.
	clcr1q2_multimoves = 6,	// for crappy clients that can't lerp

	//fte-extended
	clcq2_stringcmd_seat = 30,
#ifdef VOICECHAT
	clcq2_voicechat = 31
#endif
};

enum {
	R1Q2_CLSET_NOGUN			= 0,
	R1Q2_CLSET_NOBLEND			= 1,
	R1Q2_CLSET_RECORDING		= 2,
	R1Q2_CLSET_PLAYERUPDATES	= 3,
	R1Q2_CLSET_FPS				= 4
};
enum {
	R1Q2_SVSET_PLAYERUPDATES	= 0,
	R1Q2_SVSET_FPS				= 1
};

//==============================================

//
// client to server
//
#define	clc_bad				0
#define	clc_nop 			1
#define	clc_disconnect		2	//nq only
#define	clc_move			3	// [[usercmd_t]
#define	clc_stringcmd		4	// [string] message
#define	clc_delta			5	// [qbyte] sequence number, requests delta compression of message
#define clc_tmove			6	// teleport request, spectator only
#define clc_upload			7	// 

#define clcdp_ackframe			50
#define clcdp_ackdownloaddata	51

#define clcfte_qcrequest		81
#define clcfte_prydoncursor		82
#define clcfte_voicechat		83
#define clcfte_brushedit		84
#define clcfte_move				85	//part of PEXT2_VRINPUTS. replaces clc_move+clcfte_prydoncursor+clcdp_ackframe
#define clcfte_stringcmd_seat	86

#define VRM_LOSS	(1u<<0)	//for server packetloss reports
#define VRM_DELAY	(1u<<1)	//for server to compute lag properly.
#define VRM_SEATS	(1u<<2) //for splitscreen to work properly
#define VRM_FRAMES	(1u<<3) //number of input frames in this packet.
#define VRM_ACKS	(1u<<4)	//number of sequence acks included in message.

//==============================================

#define DLERR_FILENOTFOUND		-1	//server cannot serve the file
#define DLERR_PERMISSIONS		-2	//server refuses to serve the file
#define DLERR_UNKNOWN			-3	//server bugged out while trying to serve the request
#define DLERR_REDIRECTFILE		-4	//client should download the specified file instead.
#define DLERR_SV_REDIRECTPACK	-5	//client should download the specified package instead. may also be an http(s):// location.
#define DLERR_SV_PACKAGE		-6	//not networked. packages require special file access.

#define DLBLOCKSIZE			1024 //chunked downloads use fixed-size chunks (which I somewhat regret, but too late now I guess, really ought to use ranges.).

//these flags are sent as part of the svc_precache index, for any-time precaches. using the upper two bits means we still have 16k available models/sounds/etc
#define PC_TYPE			0xc000
#define PC_MODEL		0x0000
#define PC_SOUND		0x8000
#define PC_PARTICLE		0x4000
#define PC_UNUSED		0xc000

#define GAME_COOP		0
#define GAME_DEATHMATCH	1


// playerinfo flags from server
// playerinfo always sends: playernum, flags, origin[] and framenumber

#define	PF_MSEC			(1<<0)	//msecs says how long the player command was sitting on the server before it was sent back to the client
#define	PF_COMMAND		(1<<1)	//angles and movement values for other players (no msec or impulse)
#define	PF_VELOCITY1	(1<<2)
#define	PF_VELOCITY2	(1<<3)
#define	PF_VELOCITY3	(1<<4)
#define	PF_MODEL		(1<<5)
#define	PF_SKINNUM		(1<<6)
#define	PF_EFFECTS		(1<<7)
#define	PF_WEAPONFRAME	(1<<8)		// only sent for view player
#define	PF_DEAD			(1<<9)		// don't block movement any more
#define	PF_GIB			(1<<10)		// offset the view height differently

//ZQuake.
#define PF_PMC_SHIFT	11
#define	PF_PMC_MASK		((1<<11) | \
						 (1<<12) | \
						 (1<<13))
#ifdef PEXT_HULLSIZE
#define	PF_HULLSIZE_Z	(1<<14)
#endif
#define PF_EXTRA_PFS	(1<<15)

#ifdef PEXT_SCALE
#define	PF_SCALE		(1<<16)
#endif
#ifdef PEXT_TRANS
#define	PF_TRANS		(1<<17)
#endif
#ifdef PEXT_FATNESS
#define	PF_FATNESS		(1<<18)
#endif

#define	PF_COLOURMOD	(1<<19)
//#define	PF_UNUSED	(1<<20)	//remember to faff with zext
//#define	PF_UNUSED	(1<<21)	//remember to faff with zext
//note that if you add any more, you may need to change the check in the client so more can be parsed
#define PF_ONGROUND		(1<<22)	//or 14, depending on extensions... messy.
#define PF_SOLID		(1<<23) //or 15, depending on extensions... messy.


//not networked
#define PF_INWATER		(1u<<31) //for network smartjump.



// player move types
#define PMC_NORMAL			0		// normal ground movement
#define PMC_NORMAL_JUMP_HELD	1	// normal ground novement + jump_held
#define PMC_OLD_SPECTATOR	2		// fly through walls (QW compatibility mode)
#define PMC_SPECTATOR		3		// fly through walls
#define PMC_FLY				4		// fly, bump into walls
#define PMC_NONE			5		// can't move (client had better lerp the origin...)
#define PMC_FREEZE			6		// TODO: lerp movement and viewangles
#define PMC_WALLWALK		7		// future extension

//any more will require a different protocol message.

//==============================================

// if the high bit of the client to server qbyte is set, the low bits are
// client move cmd bits
// ms and angle2 are always sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE3 	(1<<1)
#define	CM_FORWARD	(1<<2)
#define	CM_SIDE		(1<<3)
#define	CM_UP		(1<<4)
#define	CM_BUTTONS	(1<<5)
#define	CM_IMPULSE	(1<<6)
#define	CM_ANGLE2 	(1<<7)

//sigh...
#define	Q2CM_ANGLE1 	(1<<0)
#define	Q2CM_ANGLE2 	(1<<1)
#define	Q2CM_ANGLE3 	(1<<2)
#define	Q2CM_FORWARD	(1<<3)
#define	Q2CM_SIDE		(1<<4)
#define	Q2CM_UP			(1<<5)
#define	Q2CM_BUTTONS	(1<<6)
#define	Q2CM_IMPULSE	(1<<7)

#define R1Q2_BUTTON_BYTE_FORWARD  4
#define R1Q2_BUTTON_BYTE_SIDE     8
#define R1Q2_BUTTON_BYTE_UP       16
#define R1Q2_BUTTON_BYTE_ANGLE1   32
#define R1Q2_BUTTON_BYTE_ANGLE2   64

//==============================================

// the first 16 bits of a packetentities update holds 9 bits
// of entity number and 7 bits of flags
#define	U_UNUSABLE	(1<<8)
#define	U_ORIGIN1	(1<<9)
#define	U_ORIGIN2	(1<<10)
#define	U_ORIGIN3	(1<<11)
#define	U_ANGLE2	(1<<12)
#define	U_FRAME		(1<<13)
#define	U_REMOVE	(1<<14)		// REMOVE this entity, don't add it
#define	U_MOREBITS	(1<<15)

// if MOREBITS is set, these additional flags are read in next
#define	U_ANGLE1	(1<<0)
#define	U_ANGLE3	(1<<1)
#define	U_MODEL		(1<<2)
#define	U_COLORMAP	(1<<3)
#define	U_SKIN		(1<<4)
#define	U_EFFECTS	(1<<5)
#define	U_SOLID		(1<<6)		// the entity should be solid for prediction
#ifdef PROTOCOLEXTENSIONS
#define U_EVENMORE	(1<<7)	//extension info follows

//fte extensions
//EVENMORE flags
#ifdef PEXT_SCALE
#define U_SCALE		(1<<0)	//scaler of alias models
#endif
#ifdef PEXT_TRANS
#define U_TRANS		(1<<1)	//transparency value
#endif
#ifdef PEXT_FATNESS
#define U_FATNESS	(1<<2)	//qbyte describing how fat an alias model should be. (moves verticies along normals). Useful for vacuum chambers...
#endif
#ifdef PEXT_MODELDBL
#define U_MODELDBL	(1<<3)	//extra bit for modelindexes
#endif
#define U_UNUSED1	(1<<4)
//FIXME: IMPLEMENT
#ifdef PEXT_ENTITYDBL
#define U_ENTITYDBL	(1<<5)	//use an extra qbyte for origin parts, cos one of them is off
#endif
#ifdef PEXT_ENTITYDBL2
#define U_ENTITYDBL2 (1<<6)	//use an extra qbyte for origin parts, cos one of them is off
#endif
#define U_YETMORE	(1<<7)	//even more extension info stuff.

#define U_DRAWFLAGS	(1<<8)	//use an extra qbyte for origin parts, cos one of them is off
#define U_ABSLIGHT	(1<<9)	//Force a lightlevel

#define U_COLOURMOD	(1<<10)	//rgb

#define U_DPFLAGS (1<<11)


#define U_TAGINFO (1<<12)
#define U_LIGHT (1<<13)
#define	U_EFFECTS16	(1<<14)

#define U_FARMORE (1<<15)

#endif

//FTE Replacement Deltas
//first byte contains the stuff that's most likely to change constantly
#define UF_FRAME		(1u<<0)
#define UF_ORIGINXY		(1u<<1)
#define UF_ORIGINZ		(1u<<2)
#define UF_ANGLESXZ		(1u<<3)
#define UF_ANGLESY		(1u<<4)
#define UF_EFFECTS		(1u<<5)
#define UF_PREDINFO		(1u<<6)	/*ent is predicted, probably a player*/
#define UF_EXTEND1		(1u<<7)

/*stuff which is common on ent spawning*/
#define UF_RESET		(1u<<8)	/*client will reset entire strict to its baseline*/
#define UF_16BIT_LERPTIME	(1u<<9)	/*either included frame/skin/model is 16bit (not part of the deltaing itself), or there's nextthink info*/
#define UF_MODEL		(1u<<10)
#define UF_SKIN			(1u<<11)
#define UF_COLORMAP		(1u<<12)
#define UF_SOLID		(1u<<13)
#define UF_FLAGS		(1u<<14)
#define UF_EXTEND2		(1u<<15)

/*the rest is optional extensions*/
#define UF_ALPHA		(1u<<16)
#define UF_SCALE		(1u<<17)
#define UF_BONEDATA		(1u<<18)
#define UF_DRAWFLAGS	(1u<<19)
#define UF_TAGINFO		(1u<<20)
#define UF_LIGHT		(1u<<21)
#define UF_TRAILEFFECT	(1u<<22)
#define UF_EXTEND3		(1u<<23)

#define UF_COLORMOD		(1u<<24)
#define UF_GLOW			(1u<<25)
#define UF_FATNESS		(1u<<26)
#define UF_MODELINDEX2  (1u<<27)
#define UF_GRAVITYDIR	(1u<<28)
#define UF_EFFECTS2_OLD	(1u<<29) /*specified >8bit effects, replaced with variable length*/
#define UF_UNUSED1		(1u<<30)
#define UF_EXTEND4		(1u<<31)

/*these flags are generally not deltaed as they're changing constantly*/
#define UFP_FORWARD		(1u<<0)
#define UFP_SIDE		(1u<<1)
#define UFP_UP			(1u<<2)
#define UFP_MOVETYPE	(1u<<3)	/*deltaed*/
#define UFP_VELOCITYXY	(1u<<4)
#define UFP_VELOCITYZ	(1u<<5)
#define UFP_MSEC		(1u<<6)
#define UFP_WEAPONFRAME_OLD	(1u<<7)	//no longer used. just a stat now that I rewrote stat deltas.
#define UFP_VIEWANGLE	(1u<<7)

#define UF_SV_REMOVE   UF_16BIT_LERPTIME	/*special flag  - lerptime isn't delta tracked serverside (reset sent as required with other fields)*/



#ifdef NQPROT
//NQ svc_clientdata stat updates.
#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define	SU_ITEMS		(1<<9)
#define	SU_ONGROUND		(1<<10)		// no data follows, the bit is it
#define	SU_INWATER		(1<<11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPONMODEL	(1<<14)
#define	SU_EXTEND1		(1<<15)

#define FITZSU_WEAPONMODEL2	(1<<16) // 1 byte, this is .weaponmodel & 0xFF00 (second byte)
#define FITZSU_ARMOR2		(1<<17) // 1 byte, this is .armorvalue & 0xFF00 (second byte)
#define FITZSU_AMMO2		(1<<18) // 1 byte, this is .currentammo & 0xFF00 (second byte)
#define FITZSU_SHELLS2		(1<<19) // 1 byte, this is .ammo_shells & 0xFF00 (second byte)
#define FITZSU_NAILS2		(1<<20) // 1 byte, this is .ammo_nails & 0xFF00 (second byte)
#define FITZSU_ROCKETS2		(1<<21) // 1 byte, this is .ammo_rockets & 0xFF00 (second byte)
#define FITZSU_CELLS2		(1<<22) // 1 byte, this is .ammo_cells & 0xFF00 (second byte)
#define SU_EXTEND2		(1<<23) // another byte to follow

#define FITZSU_WEAPONFRAME2	(1<<24) // 1 byte, this is .weaponframe & 0xFF00 (second byte)
#define FITZSU_WEAPONALPHA	(1<<25) // 1 byte, this is alpha for weaponmodel, uses ENTALPHA_ENCODE, not sent if ENTALPHA_DEFAULT
#define FITZSU_UNUSED26		(1<<26)
#define FITZSU_UNUSED27		(1<<27)
#define FITZSU_UNUSED28		(1<<28)
#define FITZSU_UNUSED29		(1<<29)
#define FITZSU_UNUSED30		(1<<30)
#define SU_EXTEND3		(1<<31) // another byte to follow, future expansion

//builds on top of fitz
#define	QEX_SU_FLOATCOORDS	(1<<8)
#define QEX_SU_ENTFLAGS		(1<<26)	// ULEB128 copy of the player's .flags field

// first extend byte
#define DPSU_PUNCHVEC1		(1<<16)
#define DPSU_PUNCHVEC2		(1<<17)
#define DPSU_PUNCHVEC3		(1<<18)
#define DPSU_VIEWZOOM		(1<<19) // byte factor (0 = 0.0 (not valid), 255 = 1.0)
#define DPSU_UNUSED20		(1<<20)
#define DPSU_UNUSED21		(1<<21)
#define DPSU_UNUSED22		(1<<22)
// second extend byte
#define DPSU_UNUSED24		(1<<24)
#define DPSU_UNUSED25		(1<<25)
#define DPSU_UNUSED26		(1<<26)
#define DPSU_UNUSED27		(1<<27)
#define DPSU_UNUSED28		(1<<28)
#define DPSU_UNUSED29		(1<<29)
#define DPSU_UNUSED30		(1<<30)



//NQ fast updates
#define	NQU_MOREBITS	(1<<0)
#define	NQU_ORIGIN1	(1<<1)
#define	NQU_ORIGIN2	(1<<2)
#define	NQU_ORIGIN3	(1<<3)
#define	NQU_ANGLE2	(1<<4)
#define	NQU_NOLERP	(1<<5)		// don't interpolate movement
#define	NQU_FRAME		(1<<6)
#define NQU_SIGNAL	(1<<7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	NQU_ANGLE1	(1<<8)
#define	NQU_ANGLE3	(1<<9)
#define	NQU_MODEL		(1<<10)
#define	NQU_COLORMAP	(1<<11)
#define	NQU_SKIN		(1<<12)
#define	NQU_EFFECTS	(1<<13)
#define	NQU_LONGENTITY	(1<<14)


// LordHavoc's: protocol extension
#define DPU_EXTEND1		(1<<15)
// LordHavoc: first extend byte
#define DPU_DELTA			(1<<16) // no data, while this is set the entity is delta compressed (uses previous frame as a baseline, meaning only things that have changed from the previous frame are sent, except for the forced full update every half second)
#define DPU_ALPHA			(1<<17) // 1 byte, 0.0-1.0 maps to 0-255, not sent if exactly 1, and the entity is not sent if <=0 unless it has effects (model effects are checked as well)
#define DPU_SCALE			(1<<18) // 1 byte, scale / 16 positive, not sent if 1.0
#define DPU_EFFECTS2		(1<<19) // 1 byte, this is .effects & 0xFF00 (second byte)
#define DPU_GLOWSIZE		(1<<20) // 1 byte, encoding is float/4.0, unsigned, not sent if 0
#define DPU_GLOWCOLOR		(1<<21) // 1 byte, palette index, default is 254 (white), this IS used for darklight (allowing colored darklight), however the particles from a darklight are always black, not sent if default value (even if glowsize or glowtrail is set)
// LordHavoc: colormod feature has been removed, because no one used it
#define DPU_COLORMOD		(1<<22) // 1 byte, 3 bit red, 3 bit green, 2 bit blue, this lets you tint an object artifically, so you could make a red rocket, or a blue fiend...
#define DPU_EXTEND2		(1<<23) // another byte to follow
// LordHavoc: second extend byte
#define DPU_GLOWTRAIL		(1<<24) // leaves a trail of particles (of color .glowcolor, or black if it is a negative glowsize)
#define DPU_VIEWMODEL		(1<<25) // attachs the model to the view (origin and angles become relative to it), only shown to owner, a more powerful alternative to .weaponmodel and such
#define DPU_FRAME2		(1<<26) // 1 byte, this is .frame & 0xFF00 (second byte)
#define DPU_MODEL2		(1<<27) // 1 byte, this is .modelindex & 0xFF00 (second byte)
#define DPU_EXTERIORMODEL	(1<<28) // causes this model to not be drawn when using a first person view (third person will draw it, first person will not)
#define DPU_UNUSED29		(1<<29) // future expansion
#define DPU_UNUSED30		(1<<30) // future expansion
#define DPU_EXTEND3		(1<<31) // another byte to follow, future expansion

#define FITZU_ALPHA			(1<<16)
#define FITZU_FRAME2		(1<<17)
#define FITZU_MODEL2		(1<<18)
#define FITZU_LERPFINISH	(1<<19)
#define RMQU_SCALE			(1<<20)

#define QE_U_FLOATCOORDS	(1<<21)	//set on the local player entity, to boost precision for prediction.
#define QE_U_SOLIDTYPE		(1<<22)	//for prediction I suppose.
//#define QE_U_EXTEND		(1<<23)
#define QE_U_ENTFLAGS		(1<<24)	//not sure why this needs to be networked, oh well. redundant with clientdata
#define QE_U_HEALTH			(1<<25)	//not sure why this needs to be networked, oh well.
#define QE_U_UNKNOWN26		(1<<26)	//seems to be some sort of nodraw flag (presumably for solid bmodels that need a modelindex for collisions).
#define QE_U_UNUSED27		(1<<27)
#define QE_U_UNUSED28		(1<<28)
#define QE_U_UNUSED29		(1<<29)
#define QE_U_UNUSED30		(1<<30)
#define QE_U_UNUSED31		(1u<<31)

#endif



#define	Q2U_ORIGIN1		(1<<0)
#define	Q2U_ORIGIN2		(1<<1)
#define	Q2U_ANGLE2		(1<<2)
#define	Q2U_ANGLE3		(1<<3)
#define	Q2U_FRAME8		(1<<4)		// frame is a qbyte
#define	Q2U_EVENT		(1<<5)
#define	Q2U_REMOVE		(1<<6)		// REMOVE this entity, don't add it
#define	Q2U_MOREBITS1	(1<<7)		// read one additional qbyte

// second qbyte
#define	Q2U_NUMBER16	(1<<8)		// NUMBER8 is implicit if not set
#define	Q2U_ORIGIN3		(1<<9)
#define	Q2U_ANGLE1		(1<<10)
#define	Q2U_MODEL		(1<<11)
#define Q2U_RENDERFX8	(1<<12)		// fullbright, etc
#define Q2UX_ANGLE16	(1<<13)
#define	Q2U_EFFECTS8	(1<<14)		// autorotate, trails, etc
#define	Q2U_MOREBITS2	(1<<15)		// read one additional qbyte

// third qbyte
#define	Q2U_SKIN8			(1<<16)
#define	Q2U_FRAME16			(1<<17)		// frame is a short
#define	Q2U_RENDERFX16		(1<<18)		// 8 + 16 = 32
#define	Q2U_EFFECTS16		(1<<19)		// 8 + 16 = 32
#define	Q2U_MODEL2			(1<<20)		// weapons, flags, etc
#define	Q2U_MODEL3			(1<<21)
#define	Q2U_MODEL4			(1<<22)
#define	Q2U_MOREBITS3		(1<<23)		// read one additional qbyte

// fourth qbyte
#define	Q2U_OLDORIGIN		(1<<24)		// FIXME: get rid of this
#define	Q2U_SKIN16			(1<<25)
#define	Q2U_SOUND			(1<<26)
#define	Q2U_SOLID			(1<<27)
#define Q2UX_INDEX16		(1<<28)		//model or sound is 16bit
#define Q2UEX_EFFECTS64		(1<<29)
#define Q2UEX_ALPHA			(1<<30)
#define Q2UEX_MOREBITS4		(1u<<31)
#define Q2UEX_SCALE			(1ull<<32)
#define Q2UEX_INSTANCE		(1ull<<33)
#define Q2UEX_OWNER			(1ull<<34)
#define Q2UEX_OLDFRAME		(1ull<<35)

#define Q2UX_UNUSED		(Q2UX_UNUSED1|Q2UX_UNUSED2|Q2UX_UNUSED3|Q2UX_UNUSED4)

//QuakeEx-specific stuff
//gamevar info
#define QEX_GV_DEATHMATCH		(1<<0)
#define QEX_GV_IDEALPITCHSCALE	(1<<1)
#define QEX_GV_FRICTION			(1<<2)
#define QEX_GV_EDGEFRICTION		(1<<3)
#define QEX_GV_STOPSPEED		(1<<4)
#define QEX_GV_MAXVELOCITY		(1<<5)
#define QEX_GV_GRAVITY			(1<<6)
#define QEX_GV_NOSTEP			(1<<7)
#define QEX_GV_MAXSPEED			(1<<8)
#define QEX_GV_ACCELERATE		(1<<9)
#define QEX_GV_CONTROLLERONLY	(1<<10)
#define QEX_GV_TIMELIMIT		(1<<11)
#define QEX_GV_FRAGLIMIT		(1<<12)
#define QEX_GV_TEAMPLAY			(1<<13)
#define QEX_GV_ALL				((1<<14)-1)

//==============================================
//obsolete demo players info
#define DF_ORIGINX		(1u<<0)
#define DF_ORIGINY		(1u<<1)
#define DF_ORIGINZ		(1u<<2)
#define DF_ORIGINALL	(DF_ORIGINX|DF_ORIGINY|DF_ORIGINZ)
#define DF_ANGLEX		(1u<<3)
#define DF_ANGLEY		(1u<<4)
#define DF_ANGLEZ		(1u<<5)
#define DF_ANGLESALL	(DF_ANGLEX|DF_ANGLEY|DF_ANGLEZ)
#define DF_EFFECTS		(1u<<6)
#define DF_SKINNUM		(1u<<7)
#define DF_DEAD			(1u<<8)
#define DF_GIB			(1u<<9)
#define DF_WEAPONFRAME	(1u<<10)
#define DF_MODEL		(1u<<11)
#define DF_RESET (DF_ORIGINALL|DF_ANGLESALL|DF_EFFECTS|DF_SKINNUM|DF_WEAPONFRAME|DF_MODEL)

//==============================================

// a sound with no channel is a local only sound
// the sound field has bits 0-2: channel, 3-12: entity, 13: unused, 14-15: flags
#define	QWSND_VOLUME		(1<<15)		// a qbyte
#define	QWSND_ATTENUATION	(1<<14)		// a qbyte

#define	NQSND_VOLUME		(1<<0)		// a qbyte
#define	NQSND_ATTENUATION	(1<<1)		// a qbyte
//#define DPSND_LOOPING		(1<<2)		// a long, supposedly
#define FTESND_MOREFLAGS	(1<<2)		// actually, chan flags, mostly.
#define NQSND_LARGEENTITY	(1<<3)		//both dp+fitz
#define NQSND_LARGESOUND	(1<<4)		//both dp+fitz
#define	DPSND_SPEEDUSHORT4000	(1<<5)		// ushort speed*4000 (speed is usually 1.0, a value of 0.0 is the same as 1.0)
#define FTESND_TIMEOFS		(1<<6)		//signed short, in milliseconds.
#define FTESND_PITCHADJ		(1<<7)			//a byte (speed percent (0=100%))
//more flags are weird.
#define FTESND_VELOCITY		(CF_NET_SENTVELOCITY<<8)	//borrowed.
//FTESND_NOSPACIALISE		(CF_NOSPACIALISE<<8)
//FTESND_NOREVERB			(CF_NOREVERB<<8)
//FTESND_FORCELOOP			(CF_FORCELOOP<<8)
//FTESND_FOLLOW				(CF_FOLLOW<<8)
//FTESND_RESERVED			(CF_RESERVEDN<<8)

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//baseline flags
#define FITZ_B_LARGEMODEL	(1<<0)
#define FITZ_B_LARGEFRAME	(1<<1)
#define FITZ_B_ALPHA		(1<<2)
#define RMQFITZ_B_SCALE		(1<<3)

#define QEX_B_SOLID			(1<<3)
#define QEX_B_UNKNOWN4		(1<<4)
#define QEX_B_UNKNOWN5		(1<<5)
#define QEX_B_UNKNOWN6		(1<<6)
#define QEX_B_UNKNOWN7		(1<<7)

#define	DEFAULT_VIEWHEIGHT	22


// svc_print messages have an id, so messages can be filtered
#define	PRINT_LOW			0
#define	PRINT_MEDIUM		1
#define	PRINT_HIGH			2
#define	PRINT_CHAT			3	// also go to chat buffer

//
// temp entity events
//
enum {
	TE_SPIKE				= 0,
	TE_SUPERSPIKE			= 1,
	TEQW_QWGUNSHOT			= 2,	//qw has count byte, nq does not
	TENQ_NQGUNSHOT			= 2,	//nq has no count byte
	TEQW_QWEXPLOSION		= 3,	//remapped to TEQW_EXPLOSIONNOSPRITE for nq.
	TENQ_NQEXPLOSION		= 3,	//remapped to TEQW_EXPLOSIONNOSPRITE for nq.
	TE_TAREXPLOSION			= 4,
	TE_LIGHTNING1			= 5,
	TE_LIGHTNING2			= 6,
	TE_WIZSPIKE				= 7,
	TE_KNIGHTSPIKE			= 8,
	TE_LIGHTNING3			= 9,
	TE_LAVASPLASH			= 10,
	TE_TELEPORT				= 11,

	TEQW_QWBLOOD			= 12,	//implemented as a particle() in nq
	TENQ_EXPLOSION2			= 12,	//remapped to TEQW_EXPLOSION2 for qw
	TEQW_LIGHTNINGBLOOD		= 13,	//implemented as a particle() in nq
	TENQ_BEAM				= 13,	//remapped to TEQW_BEAM for qw

#ifdef PEXT_TE_BULLET
	TE_BULLET				= 14,
	TEQW_SUPERBULLET		= 15,
#endif
	TENQ_RAILTRAIL			= 15,	//gah	[vector] origin [coord] red [coord] green [coord] blue
	TE_EXPLOSION3_NEH		= 16,	//gah	[vector] origin [coord] red [coord] green [coord] blue
	TEQW_RAILTRAIL			= 17,	//use the builtin, luke.
	TENQ_NEHLIGHTNING4		= 17,	//gah	[string] model [entity] entity [vector] start [vector] end
	TEQW_NEHLIGHTNING4		= 1000,	//give a real value if its ever properly implemented
	TEQW_BEAM				= 18,	//use the builtin, luke.
	TENQ_NEHSMOKE			= 18,	//gah	[vector] origin [byte] palette
	TEQW_EXPLOSION2			= 19,	//use the builtin, luke.
	TEQW_NQEXPLOSION		= 20,	//nq-style explosion over qw
	TENQ_QWEXPLOSION		= 20,	//qw-style explosion over nq
	TEQW_NQGUNSHOT			= 21,	//nq has count byte, qw does not
	TENQ_QWGUNSHOT			= 21,	//nq has count byte, qw does not

	// hexen 2
	TEH2_STREAM_LIGHTNING_SMALL	= 24,
	TEH2_STREAM_CHAIN			= 25,
	TEH2_STREAM_SUNSTAFF1		= 26,
	TEH2_STREAM_SUNSTAFF2		= 27,
	TEH2_STREAM_LIGHTNING		= 28,
	TEH2_STREAM_COLORBEAM		= 29,
	TEH2_STREAM_ICECHUNKS		= 30,
	TEH2_STREAM_GAZE			= 31,
	TEH2_STREAM_FAMINE		= 32,
	TEH2_PARTICLEEXPLOSION = 33,

	TEDP_BLOOD			= 50, // [coord*3] origin [byte*3] vel [byte] count
	TEDP_SPARK			= 51,
	TEDP_BLOODSHOWER	= 52,
	TEDP_EXPLOSIONRGB	= 53,
	TEDP_PARTICLECUBE	= 54,
	TEDP_PARTICLERAIN	= 55, // [vector] min [vector] max [vector] dir [short] count [byte] color
	TEDP_PARTICLESNOW	= 56, // [vector] min [vector] max [vector] dir [short] count [byte] color
	TEDP_GUNSHOTQUAD	= 57, // [vector] origin
	TEDP_SPIKEQUAD		= 58, // [vector] origin
	TEDP_SUPERSPIKEQUAD	= 59, // [vector] origin
	TEDP_EXPLOSIONQUAD	= 70, // [vector] origin
	TEDP_SMALLFLASH		= 72, // [vector] origin
	TEDP_CUSTOMFLASH	= 73,
	TEDP_FLAMEJET		= 74,
	TEDP_PLASMABURN		= 75,
	TEDP_TEI_G3			= 76,
	TEDP_SMOKE			= 77,
	TEDP_TEI_BIGEXPLOSION = 78,
	TEDP_TEI_PLASMAHIT	= 79,
};


#define CTE_CUSTOMCOUNT		1
#define CTE_CUSTOMDIRECTION	2
#define CTE_STAINS			4
#define CTE_GLOWS			8
#define CTE_CHANNELFADE		16
#define CTE_CUSTOMVELOCITY	32
#define CTE_PERSISTANT		64
#define CTE_ISBEAM			128
//CTE_ISBEAM && (CTE_CUSTOMVELOCITY||CTE_CUSTOMDIRECTION) = BOX particles.

//FTE's version of TEI_SHOWLMP2. tei's values and coding scheme which makes no sense to anyone but him.
#define SL_ORG_NW	0
#define SL_ORG_NE	1
#define SL_ORG_SW	2
#define SL_ORG_SE	3
#define SL_ORG_CC	4
#define SL_ORG_CN	5
#define SL_ORG_CS	6
#define SL_ORG_CW	7
#define SL_ORG_CE	8

//relative coding where offsets are predictable
#define SL_ORG_TL	20
#define SL_ORG_TR	21
#define SL_ORG_BL	22
#define SL_ORG_BR	23
#define SL_ORG_MM	24
#define SL_ORG_TM	25
#define SL_ORG_BM	26
#define SL_ORG_ML	27
#define SL_ORG_MR	28

/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#ifndef MAX_CLIENTS
#define	MAX_CLIENTS		255	/*max 255, min 32*/
#endif
#define	QWMAX_CLIENTS	32 /*QW's standard max. clients might have issues above this value*/
#define	NQMAX_CLIENTS	16 /*NQ's standard max. clients might have issues above this value*/

#define	UPDATE_BACKUP	64	// copies of entity_state_t to keep buffered
							// must be power of two
#define	UPDATE_MASK		(UPDATE_BACKUP-1)

#define	Q2UPDATE_BACKUP	16	// copies of entity_state_t to keep buffered
							// must be power of two
#define	Q2UPDATE_MASK		(Q2UPDATE_BACKUP-1)

#define	Q3UPDATE_BACKUP	32	// copies of entity_state_t to keep buffered
							// must be power of two
#define	Q3UPDATE_MASK		(Q3UPDATE_BACKUP-1)


// entity_state_t is the information conveyed from the server
// in an update message

typedef struct entity_state_s
{
	unsigned int		number;			// edict index
	unsigned int		sequence;

	unsigned short		modelindex;
	qbyte				inactiveflag;
	qbyte				bonecount;		//for networked bones
	unsigned int		boneoffset;		//offset into the frame (not a pointer, to avoid issues with reallocing extra storage)

//	unsigned int		eflags;			// nolerp, etc

	unsigned int		effects;

	vec3_t	origin;
	vec3_t	angles;
	union
	{
#if defined(Q2CLIENT) || defined(Q2SERVER)
		struct
		{
			unsigned int		renderfx;		//q2
			vec3_t	old_origin;		//q2/q3

			unsigned short		modelindex3;	//q2
			unsigned short		modelindex4;	//q2
			unsigned short		sound;			//q2
			qbyte				event;			//q2
			qbyte				instance;		//q2ex (splitcreen, so specific seats see it or not)
			unsigned short		owner;			//q2ex for splitscreen prediction I guess (can't just network it as non-solid).
			unsigned short		oldframe;		//q2ex
		} q2;
#endif
		struct
		{
			/*info to predict other players, so I don't get yelled at if fte were to stop supporting it*/
			qbyte pmovetype;	//&128 means onground.
			qbyte msec;
			short vangle[3];

			short movement[3];
			short velocity[3]; // 1/8th

			unsigned short weaponframe;
			unsigned char gravitydir[2];	//pitch/yaw, no roll

			unsigned short traileffectnum;
			unsigned short emiteffectnum;

			vec3_t predorg;
		} q1;
	} u;

	unsigned short		modelindex2;	//q2/vweps
	unsigned short		frame;

	unsigned short		baseframe;
	qbyte				basebone;
	qbyte				solidtype;
#define EST_FTE		0	//q2pro/r1q2, also used for fte's replacement deltas etc.
#define EST_Q2EX	1	//q2ex packs it differently.
#define EST_Q2		2	//16bit

	unsigned int solidsize;
#define ES_SOLID_NOT 0
#define ES_SOLID_BSP 31
#define ES_SOLID_HULL1 0x80201810
#define ES_SOLID_HULL2 0x80401820
#define ES_SOLID_HAS_EXTRA_BITS(solid) ((solid&0x0707) || (((solid>>16)-32768+32) & 7))	//needs to be 32bit.

	unsigned int		skinnum; /*for q2 this often contains rgba*/

	unsigned short		colormap;
	qbyte glowsize;
	qbyte glowcolour;

	qbyte	scale;	//4.4 precision
	char	fatness; //1/16th
	qbyte	hexen2flags;
	qbyte	abslight;

	qbyte	dpflags;
	qbyte	colormod[3];//3.5 precision

	qbyte	glowmod[3];	//3.5 precision
	qbyte	trans;	//254==1, 255==1-or-wateralpha

	unsigned short light[4];

	float lerpend;	//fitz rubbish

	qbyte lightstyle;
	qbyte lightpflags;
	unsigned short tagindex;	//~0 == weird portal thing.

	unsigned int tagentity;
} entity_state_t;
extern entity_state_t nullentitystate;


#define MAX_EXTENDED_PACKET_ENTITIES	256	//sanity limit.
#define	MAX_STANDARD_PACKET_ENTITIES	64	// doesn't count nails
#define	MAX_MVDPACKET_ENTITIES	196	// doesn't count nails
typedef struct
{
	float			servertime;
	int				num_entities;
	int				max_entities;
	entity_state_t	*entities;
	int				fixangles[MAX_SPLITS];	//these should not be in here
	vec3_t			fixedangles[MAX_SPLITS];
	vec3_t			punchangle[MAX_SPLITS];
	vec3_t			punchorigin[MAX_SPLITS];

	qbyte			*bonedata;
	size_t			bonedatacur;
	size_t			bonedatamax;
} packet_entities_t;

struct vrdevinfo_s
{
	unsigned int	status;
#define VRSTATUS_ORG	(1u<<0)
#define VRSTATUS_ANG	(1u<<1)
#define VRSTATUS_VEL	(1u<<2)
#define VRSTATUS_AVEL	(1u<<3)
	short			angles[3];
	short			avelocity[3];
	vec3_t			origin;
	vec3_t			velocity;
	quint64_t		weapon;
#define VRDEV_LEFT	0
#define VRDEV_RIGHT	1
#define VRDEV_HEAD	2
#define VRDEV_COUNT	3
};
typedef struct usercmd_s
{
	short	angles[3];
	signed int		forwardmove,sidemove,upmove;
	quint64_t	impulse;
	unsigned int	lightlevel;

	unsigned int	sequence;	// just for debugging prints
	float	msec;		//replace msec, but with more precision
	quint64_t buttons;	//replaces buttons, but with more bits.
	quint64_t weapon;//q3 has a separate weapon field to supplement impulse.
	unsigned int servertime;	//q3 networks the time in order to calculate msecs
	double	fservertime;//used as part of nq msec calcs
	double	fclienttime;//not used?

	//prydon cursor crap
	vec2_t	cursor_screen;
	vec3_t	cursor_start;
	vec3_t	cursor_impact;
	unsigned int	cursor_entitynumber;

	//vr things
	struct vrdevinfo_s vr[VRDEV_COUNT];	//left, right, head.
} usercmd_t;

typedef struct q2usercmd_s
{	//visible to gamecode so can't be changed (and the prediction code)
	qbyte	msec;
	qbyte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	qbyte	impulse;
	qbyte	lightlevel;
} q2usercmd_t;

typedef struct q1usercmd_s
{	//as written to qwd demos so can't be changed.
	qbyte	msec;
	vec3_t	angles;
	short	forwardmove, sidemove, upmove;
	qbyte	buttons;
	qbyte	impulse;
} q1usercmd_t;
#define SHORT2ANGLE(x) (x) * (360.0/65536)
#define ANGLE2SHORT(x) (x) * (65536/360.0)




//
// per-level limits
//
#define	Q2MAX_CLIENTS			256		// absolute limit
#define	Q2MAX_EDICTS			1024	// must change protocol to increase more
#define	Q2MAX_LIGHTSTYLES		256
#define	Q2MAX_MODELS			256		// these are sent over the net as bytes
#define	Q2MAX_SOUNDS			256		// so they cannot be blindly increased
#define	Q2MAX_IMAGES			256
#define	Q2MAX_ITEMS				256
#define Q2MAX_GENERAL			(Q2MAX_CLIENTS*2)	// general config strings


#define	Q2CS_NAME				0
#define	Q2CS_CDTRACK			1
#define	Q2CS_SKY				2
#define	Q2CS_SKYAXIS			3		// %f %f %f format
#define	Q2CS_SKYROTATE			4
#define	Q2CS_STATUSBAR			5		// display program string

#define Q2CS_AIRACCEL			29		// air acceleration control
#define	Q2CS_MAXCLIENTS			30
#define	Q2CS_MAPCHECKSUM		31		// for catching cheater maps

#define	Q2CS_MODELS				32
#define	Q2CS_SOUNDS				(Q2CS_MODELS	+Q2MAX_MODELS)
#define	Q2CS_IMAGES				(Q2CS_SOUNDS	+Q2MAX_SOUNDS)
#define	Q2CS_LIGHTS				(Q2CS_IMAGES	+Q2MAX_IMAGES)
#define	Q2CS_ITEMS				(Q2CS_LIGHTS	+Q2MAX_LIGHTSTYLES)
#define	Q2CS_PLAYERSKINS		(Q2CS_ITEMS		+Q2MAX_ITEMS)
#define Q2CS_GENERAL			(Q2CS_PLAYERSKINS	+Q2MAX_CLIENTS)
#define	Q2MAX_CONFIGSTRINGS		(Q2CS_GENERAL	+Q2MAX_GENERAL)


#define	Q2EXMAX_CLIENTS			Q2MAX_CLIENTS		// absolute limit
#define	Q2EXMAX_EDICTS			8192	// must change protocol to increase more
#define	Q2EXMAX_LIGHTSTYLES		256
#define	Q2EXMAX_RTLIGHTS		256
#define	Q2EXMAX_MODELS			8192		// these are sent over the net as bytes
#define	Q2EXMAX_SOUNDS			2048		// so they cannot be blindly increased
#define	Q2EXMAX_IMAGES			512
#define	Q2EXMAX_ITEMS			256
#define	Q2EXMAX_WWHEEL			32
#define Q2EXMAX_GENERAL			(Q2EXMAX_CLIENTS*2)	// general config strings

#define	Q2EXCS_NAME				Q2CS_NAME
#define	Q2EXCS_CDTRACK			Q2CS_CDTRACK
#define	Q2EXCS_SKY				Q2CS_SKY
#define	Q2EXCS_SKYAXIS			Q2CS_SKYAXIS		// %f %f %f format
#define	Q2EXCS_SKYROTATE		Q2CS_SKYROTATE
#define	Q2EXCS_STATUSBAR		Q2CS_STATUSBAR		// display program string
#define Q2EXCS_AIRACCEL			59		// air acceleration control
#define	Q2EXCS_MAXCLIENTS		60
#define	Q2EXCS_MAPCHECKSUM		61		// for catching cheater maps
#define	Q2EXCS_MODELS			62
#define	Q2EXCS_SOUNDS			(Q2EXCS_MODELS			+Q2EXMAX_MODELS)
#define	Q2EXCS_IMAGES			(Q2EXCS_SOUNDS			+Q2EXMAX_SOUNDS)
#define	Q2EXCS_LIGHTS			(Q2EXCS_IMAGES			+Q2EXMAX_IMAGES)
#define	Q2EXCS_RTLIGHTS			(Q2EXCS_LIGHTS			+Q2EXMAX_LIGHTSTYLES)
#define	Q2EXCS_ITEMS			(Q2EXCS_RTLIGHTS		+Q2EXMAX_RTLIGHTS)
#define	Q2EXCS_PLAYERSKINS		(Q2EXCS_ITEMS			+Q2EXMAX_ITEMS)
#define Q2EXCS_GENERAL			(Q2EXCS_PLAYERSKINS		+Q2EXMAX_CLIENTS)
#define	Q2ECS_WHEEL_WEAPONS		(Q2EXCS_GENERAL			+Q2EXMAX_GENERAL)	//item|icon|ammotype|minammo|powerup|sortid|warnammo|droppable
#define	Q2ECS_WHEEL_AMMO		(Q2ECS_WHEEL_WEAPONS	+Q2EXMAX_WWHEEL)	//item|icon
#define	Q2ECS_WHEEL_POWERUPS	(Q2ECS_WHEEL_AMMO		+Q2EXMAX_WWHEEL)	//item|icon|toggled|sortid|droppable|ammotype
#define	Q2ECS_CD_LOOP_COUNT		(Q2ECS_WHEEL_POWERUPS	+Q2EXMAX_WWHEEL)
#define	Q2ECS_GAME_STYLE		(Q2ECS_CD_LOOP_COUNT	+1)
#define	Q2EXMAX_CONFIGSTRINGS	(Q2ECS_GAME_STYLE		+1)

// player_state->stats[] indexes
#define Q2STAT_HEALTH_ICON		0
#define	Q2STAT_HEALTH				1
#define	Q2STAT_AMMO_ICON			2
#define	Q2STAT_AMMO				3
#define	Q2STAT_ARMOR_ICON			4
#define	Q2STAT_ARMOR				5
#define	Q2STAT_SELECTED_ICON		6
#define	Q2STAT_PICKUP_ICON		7
#define	Q2STAT_PICKUP_STRING		8
#define	Q2STAT_TIMER_ICON			9
#define	Q2STAT_TIMER				10
#define	Q2STAT_HELPICON			11
#define	Q2STAT_SELECTED_ITEM		12
#define	Q2STAT_LAYOUTS			13
#define	Q2STAT_FRAGS				14
#define	Q2STAT_FLASHES			15		// cleared each frame, 1 = health, 2 = armor
#define Q2STAT_CHASE				16
#define Q2STAT_SPECTATOR			17

#define	Q2MAX_STATS				32


//for the local player
#define	Q2PS_M_TYPE				(1<<0)
#define	Q2PS_M_ORIGIN			(1<<1)
#define	Q2PS_M_VELOCITY			(1<<2)
#define	Q2PS_M_TIME				(1<<3)
#define	Q2PS_M_FLAGS			(1<<4)
#define	Q2PS_M_GRAVITY			(1<<5)
#define	Q2PS_M_DELTA_ANGLES		(1<<6)
#define	Q2PS_VIEWOFFSET			(1<<7)
#define	Q2PS_VIEWANGLES			(1<<8)
#define	Q2PS_KICKANGLES			(1<<9)
#define	Q2PS_BLEND				(1<<10)
#define	Q2PS_FOV				(1<<11)
#define	Q2PS_WEAPONINDEX		(1<<12)
#define	Q2PS_WEAPONFRAME		(1<<13)
#define	Q2PS_RDFLAGS			(1<<14)
#define	Q2PS_EXTRABITS			(1<<15)
#define Q2FTEPS_INDEX16			(1<<16)
#define Q2EXPS_DAMAGEBLEND		(1<<16)
#define Q2FTEPS_CLIENTNUM		(1<<17)
#define Q2EXPS_TEAMID			(1<<17)
#define Q2PS_UNUSED6			(1<<18)
#define Q2PS_UNUSED5			(1<<19)
#define Q2PS_UNUSED4			(1<<20)
#define Q2PS_UNUSED3			(1<<21)
#define Q2PS_UNUSED2			(1<<22)
#define Q2PS_UNUSED1			(1<<23)

#define Q2PSX_GUNOFFSET			(1<<0)
#define Q2PSX_GUNANGLES			(1<<1)
#define Q2PSX_M_VELOCITY2		(1<<2)
#define Q2PSX_M_ORIGIN2			(1<<3)
#define Q2PSX_VIEWANGLE2		(1<<4)
#define Q2PSX_STATS				(1<<5)
#define Q2PSX_CLIENTNUM			(1<<6)
#define Q2PSX_OLD				(1<<8)	//not part of the protocol, just lazy handling.


// entity_state_t->renderfx flags
#define	Q2RF_MINLIGHT			(1u<<0)		//ni	always have some light (viewmodel)
#define	RF_EXTERNALMODEL		(1u<<1)		//i 	don't draw through eyes, only mirrors
#define	RF_WEAPONMODEL			(1u<<2)		//i 	only draw through eyes
#define	RF_FULLBRIGHT			(1u<<3)		//i 	always draw full intensity
#define	RF_DEPTHHACK			(1u<<4)		//i 	for view weapon Z crunching
#define	RF_TRANSLUCENT			(1u<<5)		//forces shader sort order and BEF_FORCETRANSPARENT
#define	Q2RF_FRAMELERP			(1u<<6)		//q2only
#define Q2RF_BEAM				(1u<<7)		//mostly q2only
//
#define	Q2RF_CUSTOMSKIN			(1u<<8)		//not even in q2		skin is an index in image_precache
#define	Q2RF_GLOW				(1u<<9)		//i		pulse lighting for bonus items
#define Q2RF_SHELL_RED			(1u<<10)	//q2only
#define	Q2RF_SHELL_GREEN		(1u<<11)	//q2only
#define Q2RF_SHELL_BLUE			(1u<<12)	//q2only
//
#define RF_NOSHADOW				(1u<<13)
#define Q2REX_CASTSHADOW		(1u<<14)
//ROGUE start
#define Q2RF_IR_VISIBLE			(1u<<15)	// shows red with Q2RDF_IRGOGGLES
#define	Q2RF_SHELL_DOUBLE		(1u<<16)	//q2only
#define	Q2RF_SHELL_HALF_DAM		(1u<<17)	//q2only
#define Q2RF_USE_DISGUISE		(1u<<18)	//ni	entity is displayed with skin 'players/$MODEL/disguise.pcx' instead
//ROGUE end
//#define Q2EXRF_SHELL_LITE_GREEN	(1u<<19)
#define Q2EXRF_CUSTOM_LIGHT		(1u<<20)
#define Q2EXRF_FLARE			(1u<<21)	//changes the interpretation of a lot of fields, basically replacing the entire ent.
//#define Q2EXRF_OLD_FRAME_LERP	(1u<<22)	//This flag signals that `s.old_frame` should be used for the next frame and respected by the client. This can be used for custom frame interpolation; its use in this engine is specific to fixing interpolation bugs on certain monster animations.
//#define Q2EXRF_BLOB_SHADOW		(1u<<23)	//
//#define Q2EXRF_LOW_PRIORITY		(1u<<24)	//
//#define Q2EXRF_NO_LOD				(1u<<25)	//
//#define Q2EXRF_STAIRSTEP			(1u<<26)	//

#define RF_ADDITIVE				(1u<<27)	//forces shader sort order and BEF_FORCEADDITIVE
#define RF_NODEPTHTEST			(1u<<28)	//forces shader sort order and BEF_FORCENODEPTH
#define RF_FORCECOLOURMOD		(1u<<29)	//forces BEF_FORCECOLOURMOD
#define RF_WEAPONMODELNOBOB		(1u<<30)
#define RF_FIRSTPERSON			(1u<<31)	//only draw through eyes
#define	RF_XFLIP				Q2EXRF_FLARE	//flip horizontally (for q2's left-handed weapons)

// player_state_t->refdef flags
#define	RDF_UNDERWATER			(1u<<0)		// warp the screen as apropriate (fov trick)
#define RDF_NOWORLDMODEL		(1u<<1)		// used for player configuration screen
//ROGUE
#define	Q2RDF_IRGOGGLES			(1u<<2)		//ents with Q2RF_IR_VISIBLE show up pure red.
#define Q2RDF_UVGOGGLES			(1u<<3)		//usused / reserved
//ROGUE

#define RDF_BLOOM				(1u<<16)
#define RDF_FISHEYE				(1u<<17)
#define RDF_WATERWARP			(1u<<18)
#define RDF_CUSTOMPOSTPROC		(1u<<19)
#define RDF_ANTIALIAS			(1u<<20)	//fxaa, or possibly even just fsaa
#define RDF_RENDERSCALE			(1u<<21)
#define RDF_SCENEGAMMA			(1u<<22)
#define RDF_DISABLEPARTICLES	(1u<<23)	//mostly for skyrooms
#define RDF_SKIPSKY				(1u<<24)	//we drew a skyroom, skip drawing sky chains for this scene.
#define RDF_SKYROOMENABLED		(1u<<25)	//skyroom position is known, be prepared to draw the skyroom if its visible.

#define RDF_ALLPOSTPROC			(RDF_BLOOM|RDF_FISHEYE|RDF_WATERWARP|RDF_CUSTOMPOSTPROC|RDF_ANTIALIAS|RDF_SCENEGAMMA)	//these flags require rendering to an fbo for the various different post-processing shaders.





#define	Q2SND_VOLUME		(1u<<0)		// a qbyte
#define	Q2SND_ATTENUATION	(1u<<1)		// a qbyte
#define	Q2SND_POS			(1u<<2)		// three coordinates
#define	Q2SND_ENT			(1u<<3)		// a short 0-2: channel, 3-12: entity
#define	Q2SND_OFFSET		(1u<<4)		// a qbyte, msec offset from frame start
#define Q2SNDFTE_LARGEIDX	(1u<<5)		// idx is a short
#define Q2SNDEX_EXPLICITPOS	(1u<<5)		// ?
#define Q2SNDEX_LARGEENT	(1u<<6)		// 32bit index
#define Q2SND_EXTRABITS		(1u<<7)		// unused for now, reserved.

#define Q2DEFAULT_SOUND_PACKET_VOLUME	1.0
#define Q2DEFAULT_SOUND_PACKET_ATTENUATION 1.0


#define ATTN_NONE	0
#define ATTN_NORM	1
#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4

#define	Q2MZ_BLASTER			0
#define Q2MZ_MACHINEGUN		1
#define	Q2MZ_SHOTGUN			2
#define	Q2MZ_CHAINGUN1		3
#define	Q2MZ_CHAINGUN2		4
#define	Q2MZ_CHAINGUN3		5
#define	Q2MZ_RAILGUN			6
#define	Q2MZ_ROCKET			7
#define	Q2MZ_GRENADE			8
#define	Q2MZ_LOGIN			9
#define	Q2MZ_LOGOUT			10
#define	Q2MZ_RESPAWN			11
#define	Q2MZ_BFG				12
#define	Q2MZ_SSHOTGUN			13
#define	Q2MZ_HYPERBLASTER		14
#define	Q2MZ_ITEMRESPAWN		15
// RAFAEL
#define Q2MZ_IONRIPPER		16
#define Q2MZ_BLUEHYPERBLASTER 17
#define Q2MZ_PHALANX			18
#define Q2MZ_SILENCED			128		// bit flag ORed with one of the above numbers

//ROGUE
#define Q2MZ_ETF_RIFLE		30
//#define Q2MZ_UNUSED			31
#define Q2MZ_SHOTGUN2			32
#define Q2MZ_HEATBEAM			33
#define Q2MZ_BLASTER2			34
#define	Q2MZ_TRACKER			35
#define	Q2MZ_NUKE1			36
#define	Q2MZ_NUKE2			37
#define	Q2MZ_NUKE4			38
#define	Q2MZ_NUKE8			39
//ROGUE

#define Q2EXMZ_BFG2		19
#define Q2EXMZ_PHALANX2		20
#define Q2EXMZ_PROX		31
#define Q2EXMZ_ETF_RIFLE_2		32


//
// monster muzzle flashes
//
#define Q2MZ2_TANK_BLASTER_1				1
#define Q2MZ2_TANK_BLASTER_2				2
#define Q2MZ2_TANK_BLASTER_3				3
#define Q2MZ2_TANK_MACHINEGUN_1			4
#define Q2MZ2_TANK_MACHINEGUN_2			5
#define Q2MZ2_TANK_MACHINEGUN_3			6
#define Q2MZ2_TANK_MACHINEGUN_4			7
#define Q2MZ2_TANK_MACHINEGUN_5			8
#define Q2MZ2_TANK_MACHINEGUN_6			9
#define Q2MZ2_TANK_MACHINEGUN_7			10
#define Q2MZ2_TANK_MACHINEGUN_8			11
#define Q2MZ2_TANK_MACHINEGUN_9			12
#define Q2MZ2_TANK_MACHINEGUN_10			13
#define Q2MZ2_TANK_MACHINEGUN_11			14
#define Q2MZ2_TANK_MACHINEGUN_12			15
#define Q2MZ2_TANK_MACHINEGUN_13			16
#define Q2MZ2_TANK_MACHINEGUN_14			17
#define Q2MZ2_TANK_MACHINEGUN_15			18
#define Q2MZ2_TANK_MACHINEGUN_16			19
#define Q2MZ2_TANK_MACHINEGUN_17			20
#define Q2MZ2_TANK_MACHINEGUN_18			21
#define Q2MZ2_TANK_MACHINEGUN_19			22
#define Q2MZ2_TANK_ROCKET_1				23
#define Q2MZ2_TANK_ROCKET_2				24
#define Q2MZ2_TANK_ROCKET_3				25

#define Q2MZ2_INFANTRY_MACHINEGUN_1		26
#define Q2MZ2_INFANTRY_MACHINEGUN_2		27
#define Q2MZ2_INFANTRY_MACHINEGUN_3		28
#define Q2MZ2_INFANTRY_MACHINEGUN_4		29
#define Q2MZ2_INFANTRY_MACHINEGUN_5		30
#define Q2MZ2_INFANTRY_MACHINEGUN_6		31
#define Q2MZ2_INFANTRY_MACHINEGUN_7		32
#define Q2MZ2_INFANTRY_MACHINEGUN_8		33
#define Q2MZ2_INFANTRY_MACHINEGUN_9		34
#define Q2MZ2_INFANTRY_MACHINEGUN_10		35
#define Q2MZ2_INFANTRY_MACHINEGUN_11		36
#define Q2MZ2_INFANTRY_MACHINEGUN_12		37
#define Q2MZ2_INFANTRY_MACHINEGUN_13		38

#define Q2MZ2_SOLDIER_BLASTER_1			39
#define Q2MZ2_SOLDIER_BLASTER_2			40
#define Q2MZ2_SOLDIER_SHOTGUN_1			41
#define Q2MZ2_SOLDIER_SHOTGUN_2			42
#define Q2MZ2_SOLDIER_MACHINEGUN_1		43
#define Q2MZ2_SOLDIER_MACHINEGUN_2		44

#define Q2MZ2_GUNNER_MACHINEGUN_1			45
#define Q2MZ2_GUNNER_MACHINEGUN_2			46
#define Q2MZ2_GUNNER_MACHINEGUN_3			47
#define Q2MZ2_GUNNER_MACHINEGUN_4			48
#define Q2MZ2_GUNNER_MACHINEGUN_5			49
#define Q2MZ2_GUNNER_MACHINEGUN_6			50
#define Q2MZ2_GUNNER_MACHINEGUN_7			51
#define Q2MZ2_GUNNER_MACHINEGUN_8			52
#define Q2MZ2_GUNNER_GRENADE_1			53
#define Q2MZ2_GUNNER_GRENADE_2			54
#define Q2MZ2_GUNNER_GRENADE_3			55
#define Q2MZ2_GUNNER_GRENADE_4			56

#define Q2MZ2_CHICK_ROCKET_1				57

#define Q2MZ2_FLYER_BLASTER_1				58
#define Q2MZ2_FLYER_BLASTER_2				59

#define Q2MZ2_MEDIC_BLASTER_1				60

#define Q2MZ2_GLADIATOR_RAILGUN_1			61

#define Q2MZ2_HOVER_BLASTER_1				62

#define Q2MZ2_ACTOR_MACHINEGUN_1			63

#define Q2MZ2_SUPERTANK_MACHINEGUN_1		64
#define Q2MZ2_SUPERTANK_MACHINEGUN_2		65
#define Q2MZ2_SUPERTANK_MACHINEGUN_3		66
#define Q2MZ2_SUPERTANK_MACHINEGUN_4		67
#define Q2MZ2_SUPERTANK_MACHINEGUN_5		68
#define Q2MZ2_SUPERTANK_MACHINEGUN_6		69
#define Q2MZ2_SUPERTANK_ROCKET_1			70
#define Q2MZ2_SUPERTANK_ROCKET_2			71
#define Q2MZ2_SUPERTANK_ROCKET_3			72

#define Q2MZ2_BOSS2_MACHINEGUN_L1			73
#define Q2MZ2_BOSS2_MACHINEGUN_L2			74
#define Q2MZ2_BOSS2_MACHINEGUN_L3			75
#define Q2MZ2_BOSS2_MACHINEGUN_L4			76
#define Q2MZ2_BOSS2_MACHINEGUN_L5			77
#define Q2MZ2_BOSS2_ROCKET_1				78
#define Q2MZ2_BOSS2_ROCKET_2				79
#define Q2MZ2_BOSS2_ROCKET_3				80
#define Q2MZ2_BOSS2_ROCKET_4				81

#define Q2MZ2_FLOAT_BLASTER_1				82

#define Q2MZ2_SOLDIER_BLASTER_3			83
#define Q2MZ2_SOLDIER_SHOTGUN_3			84
#define Q2MZ2_SOLDIER_MACHINEGUN_3		85
#define Q2MZ2_SOLDIER_BLASTER_4			86
#define Q2MZ2_SOLDIER_SHOTGUN_4			87
#define Q2MZ2_SOLDIER_MACHINEGUN_4		88
#define Q2MZ2_SOLDIER_BLASTER_5			89
#define Q2MZ2_SOLDIER_SHOTGUN_5			90
#define Q2MZ2_SOLDIER_MACHINEGUN_5		91
#define Q2MZ2_SOLDIER_BLASTER_6			92
#define Q2MZ2_SOLDIER_SHOTGUN_6			93
#define Q2MZ2_SOLDIER_MACHINEGUN_6		94
#define Q2MZ2_SOLDIER_BLASTER_7			95
#define Q2MZ2_SOLDIER_SHOTGUN_7			96
#define Q2MZ2_SOLDIER_MACHINEGUN_7		97
#define Q2MZ2_SOLDIER_BLASTER_8			98
#define Q2MZ2_SOLDIER_SHOTGUN_8			99
#define Q2MZ2_SOLDIER_MACHINEGUN_8		100

// --- Xian shit below ---
#define	Q2MZ2_MAKRON_BFG					101
#define Q2MZ2_MAKRON_BLASTER_1			102
#define Q2MZ2_MAKRON_BLASTER_2			103
#define Q2MZ2_MAKRON_BLASTER_3			104
#define Q2MZ2_MAKRON_BLASTER_4			105
#define Q2MZ2_MAKRON_BLASTER_5			106
#define Q2MZ2_MAKRON_BLASTER_6			107
#define Q2MZ2_MAKRON_BLASTER_7			108
#define Q2MZ2_MAKRON_BLASTER_8			109
#define Q2MZ2_MAKRON_BLASTER_9			110
#define Q2MZ2_MAKRON_BLASTER_10			111
#define Q2MZ2_MAKRON_BLASTER_11			112
#define Q2MZ2_MAKRON_BLASTER_12			113
#define Q2MZ2_MAKRON_BLASTER_13			114
#define Q2MZ2_MAKRON_BLASTER_14			115
#define Q2MZ2_MAKRON_BLASTER_15			116
#define Q2MZ2_MAKRON_BLASTER_16			117
#define Q2MZ2_MAKRON_BLASTER_17			118
#define Q2MZ2_MAKRON_RAILGUN_1			119
#define	Q2MZ2_JORG_MACHINEGUN_L1			120
#define	Q2MZ2_JORG_MACHINEGUN_L2			121
#define	Q2MZ2_JORG_MACHINEGUN_L3			122
#define	Q2MZ2_JORG_MACHINEGUN_L4			123
#define	Q2MZ2_JORG_MACHINEGUN_L5			124
#define	Q2MZ2_JORG_MACHINEGUN_L6			125
#define	Q2MZ2_JORG_MACHINEGUN_R1			126
#define	Q2MZ2_JORG_MACHINEGUN_R2			127
#define	Q2MZ2_JORG_MACHINEGUN_R3			128
#define	Q2MZ2_JORG_MACHINEGUN_R4			129
#define Q2MZ2_JORG_MACHINEGUN_R5			130
#define	Q2MZ2_JORG_MACHINEGUN_R6			131
#define Q2MZ2_JORG_BFG_1					132
#define Q2MZ2_BOSS2_MACHINEGUN_R1			133
#define Q2MZ2_BOSS2_MACHINEGUN_R2			134
#define Q2MZ2_BOSS2_MACHINEGUN_R3			135
#define Q2MZ2_BOSS2_MACHINEGUN_R4			136
#define Q2MZ2_BOSS2_MACHINEGUN_R5			137

//ROGUE
#define	Q2MZ2_CARRIER_MACHINEGUN_L1		138
#define	Q2MZ2_CARRIER_MACHINEGUN_R1		139
#define	Q2MZ2_CARRIER_GRENADE				140
#define Q2MZ2_TURRET_MACHINEGUN			141
#define Q2MZ2_TURRET_ROCKET				142
#define Q2MZ2_TURRET_BLASTER				143
#define Q2MZ2_STALKER_BLASTER				144
#define Q2MZ2_DAEDALUS_BLASTER			145
#define Q2MZ2_MEDIC_BLASTER_2				146
#define	Q2MZ2_CARRIER_RAILGUN				147
#define	Q2MZ2_WIDOW_DISRUPTOR				148
#define	Q2MZ2_WIDOW_BLASTER				149
#define	Q2MZ2_WIDOW_RAIL					150
#define	Q2MZ2_WIDOW_PLASMABEAM			151		// PMM - not used
#define	Q2MZ2_CARRIER_MACHINEGUN_L2		152
#define	Q2MZ2_CARRIER_MACHINEGUN_R2		153
#define	Q2MZ2_WIDOW_RAIL_LEFT				154
#define	Q2MZ2_WIDOW_RAIL_RIGHT			155
#define	Q2MZ2_WIDOW_BLASTER_SWEEP1		156
#define	Q2MZ2_WIDOW_BLASTER_SWEEP2		157
#define	Q2MZ2_WIDOW_BLASTER_SWEEP3		158
#define	Q2MZ2_WIDOW_BLASTER_SWEEP4		159
#define	Q2MZ2_WIDOW_BLASTER_SWEEP5		160
#define	Q2MZ2_WIDOW_BLASTER_SWEEP6		161
#define	Q2MZ2_WIDOW_BLASTER_SWEEP7		162
#define	Q2MZ2_WIDOW_BLASTER_SWEEP8		163
#define	Q2MZ2_WIDOW_BLASTER_SWEEP9		164
#define	Q2MZ2_WIDOW_BLASTER_100			165
#define	Q2MZ2_WIDOW_BLASTER_90			166
#define	Q2MZ2_WIDOW_BLASTER_80			167
#define	Q2MZ2_WIDOW_BLASTER_70			168
#define	Q2MZ2_WIDOW_BLASTER_60			169
#define	Q2MZ2_WIDOW_BLASTER_50			170
#define	Q2MZ2_WIDOW_BLASTER_40			171
#define	Q2MZ2_WIDOW_BLASTER_30			172
#define	Q2MZ2_WIDOW_BLASTER_20			173
#define	Q2MZ2_WIDOW_BLASTER_10			174
#define	Q2MZ2_WIDOW_BLASTER_0				175
#define	Q2MZ2_WIDOW_BLASTER_10L			176
#define	Q2MZ2_WIDOW_BLASTER_20L			177
#define	Q2MZ2_WIDOW_BLASTER_30L			178
#define	Q2MZ2_WIDOW_BLASTER_40L			179
#define	Q2MZ2_WIDOW_BLASTER_50L			180
#define	Q2MZ2_WIDOW_BLASTER_60L			181
#define	Q2MZ2_WIDOW_BLASTER_70L			182
#define	Q2MZ2_WIDOW_RUN_1					183
#define	Q2MZ2_WIDOW_RUN_2					184
#define	Q2MZ2_WIDOW_RUN_3					185
#define	Q2MZ2_WIDOW_RUN_4					186
#define	Q2MZ2_WIDOW_RUN_5					187
#define	Q2MZ2_WIDOW_RUN_6					188
#define	Q2MZ2_WIDOW_RUN_7					189
#define	Q2MZ2_WIDOW_RUN_8					190
#define	Q2MZ2_CARRIER_ROCKET_1			191
#define	Q2MZ2_CARRIER_ROCKET_2			192
#define	Q2MZ2_CARRIER_ROCKET_3			193
#define	Q2MZ2_CARRIER_ROCKET_4			194
#define	Q2MZ2_WIDOW2_BEAMER_1				195
#define	Q2MZ2_WIDOW2_BEAMER_2				196
#define	Q2MZ2_WIDOW2_BEAMER_3				197
#define	Q2MZ2_WIDOW2_BEAMER_4				198
#define	Q2MZ2_WIDOW2_BEAMER_5				199
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_1			200
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_2			201
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_3			202
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_4			203
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_5			204
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_6			205
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_7			206
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_8			207
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_9			208
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_10		209
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_11		210






#define MAX_MAP_AREA_BYTES		32

// edict->drawflags (hexen2 stuff)
#define MLS_MASK				7
#define MLS_NONE				0
#define MLS_LIGHTSTYLE25		1	//indexes style 25 instead of using real lighting info
#define MLS_LIGHTSTYLE26		2	//indexes style 26
#define MLS_LIGHTSTYLE27		3	//27
#define MLS_LIGHTSTYLE28		4	//...
#define MLS_LIGHTSTYLE29		5	//duh
#define MLS_ADDLIGHT			6	//adds abslight to normal lighting
#define MLS_ABSLIGHT			(MLS_MASK)	//uses abslight specifically.
#define SCALE_TYPE_MASK			(SCALE_TYPE_UNIFORM|SCALE_TYPE_XYONLY|SCALE_TYPE_ZONLY)
#define SCALE_TYPE_UNIFORM		0	// Scale X, Y, and Z
#define SCALE_TYPE_XYONLY		8	// Scale X and Y
#define SCALE_TYPE_ZONLY		16	// Scale Z
#define SCALE_TYPE_UNUSED		(SCALE_TYPE_XYONLY|SCALE_TYPE_ZONLY)
#define SCALE_ORIGIN_MASK		(SCALE_ORIGIN_TOP|SCALE_ORIGIN_BOTTOM|SCALE_ORIGIN_CENTER)
#define SCALE_ORIGIN_CENTER		0	// Scaling origin at object center
#define SCALE_ORIGIN_BOTTOM		32	// Scaling origin at object bottom
#define SCALE_ORIGIN_TOP		64	// Scaling origin at object top
#define SCALE_ORIGIN_ORIGIN		(SCALE_ORIGIN_TOP|SCALE_ORIGIN_BOTTOM)	// Scaling origin at object origin
#define DRF_TRANSLUCENT			128	//alpha is controlled by r_wateralpha


//TENEBRAE_GFX_DLIGHTS
#define PFLAGS_NOSHADOW		1
#define PFLAGS_CORONA		2
#define PFLAGS_FULLDYNAMIC	128	//NOTE: this is a dp-ism. for tenebrae compat, this should be effects&16 and not pflags&128, as effects&16 already means something else

#define RENDER_STEP 1
#define RENDER_GLOWTRAIL 2
#define RENDER_VIEWMODEL 4
#define RENDER_EXTERIORMODEL 8
#define RENDER_LOWPRECISION 16 // send as low precision coordinates to save bandwidth
#define RENDER_COLORMAPPED 32	//networked colormap field is a direct (top<<4)|bottom value rather than a player slot (the |1024 thing d does)
//#define RENDER_WORLDOBJECT 64
#define RENDER_COMPLEXANIMATION 128

//darkplaces protocols 5 to 7 use these
// reset all entity fields (typically used if status changed)
#define E5_FULLUPDATE (1<<0)
// E5_ORIGIN32=0: short[3] = s->origin[0] * 8, s->origin[1] * 8, s->origin[2] * 8
// E5_ORIGIN32=1: float[3] = s->origin[0], s->origin[1], s->origin[2]
#define E5_ORIGIN (1<<1)
// E5_ANGLES16=0: byte[3] = s->angle[0] * 256 / 360, s->angle[1] * 256 / 360, s->angle[2] * 256 / 360
// E5_ANGLES16=1: short[3] = s->angle[0] * 65536 / 360, s->angle[1] * 65536 / 360, s->angle[2] * 65536 / 360
#define E5_ANGLES (1<<2)
// E5_MODEL16=0: byte = s->modelindex
// E5_MODEL16=1: short = s->modelindex
#define E5_MODEL (1<<3)
// E5_FRAME16=0: byte = s->frame
// E5_FRAME16=1: short = s->frame
#define E5_FRAME (1<<4)
// byte = s->skin
#define E5_SKIN (1<<5)
// E5_EFFECTS16=0 && E5_EFFECTS32=0: byte = s->effects
// E5_EFFECTS16=1 && E5_EFFECTS32=0: short = s->effects
// E5_EFFECTS16=0 && E5_EFFECTS32=1: int = s->effects
// E5_EFFECTS16=1 && E5_EFFECTS32=1: int = s->effects
#define E5_EFFECTS (1<<6)
// bits >= (1<<8)
#define E5_EXTEND1 (1<<7)

// byte = s->renderflags
#define E5_FLAGS (1<<8)
// byte = bound(0, s->alpha * 255, 255)
#define E5_ALPHA (1<<9)
// byte = bound(0, s->scale * 16, 255)
#define E5_SCALE (1<<10)
// flag
#define E5_ORIGIN32 (1<<11)
// flag
#define E5_ANGLES16 (1<<12)
// flag
#define E5_MODEL16 (1<<13)
// byte = s->colormap
#define E5_COLORMAP (1<<14)
// bits >= (1<<16)
#define E5_EXTEND2 (1<<15)

// short = s->tagentity
// byte = s->tagindex
#define E5_ATTACHMENT (1<<16)
// short[4] = s->light[0], s->light[1], s->light[2], s->light[3]
// byte = s->lightstyle
// byte = s->lightpflags
#define E5_LIGHT (1<<17)
// byte = s->glowsize
// byte = s->glowcolor
#define E5_GLOW (1<<18)
// short = s->effects
#define E5_EFFECTS16 (1<<19)
// int = s->effects
#define E5_EFFECTS32 (1<<20)
// flag
#define E5_FRAME16 (1<<21)
// unused
#define E5_COLORMOD (1<<22)
// bits >= (1<<24)
#define E5_EXTEND3 (1<<23)

// unused
#define E5_GLOWMOD (1<<24)
// unused
#define E5_COMPLEXANIMATION (1<<25)
// unused
#define E5_TRAILEFFECTNUM (1<<26)
// unused
#define E5_UNUSED27 (1<<27)
// unused
#define E5_UNUSED28 (1<<28)
// unused
#define E5_UNUSED29 (1<<29)
// unused
#define E5_UNUSED30 (1<<30)
// bits2 > 0
#define E5_EXTEND4 (1u<<31)

#define E5_ALLUNUSED (E5_UNUSED27|E5_UNUSED28|E5_UNUSED29|E5_UNUSED30)
#define E5_SERVERPRIVATE (E5_EXTEND1|E5_EXTEND2|E5_EXTEND3|E5_EXTEND4)
#define E5_SERVERREMOVE E5_EXTEND1
