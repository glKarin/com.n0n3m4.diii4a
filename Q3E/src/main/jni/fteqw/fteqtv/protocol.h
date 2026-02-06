

//limitations of the protocol
#define	MAX_SERVERINFO_STRING	1024	//standard quakeworld has 512 here.
#define MAX_USERINFO 1024	//standard quakeworld has 192 here.
#define MAX_CLIENTS 32
#define MAX_LIST 256
#define MAX_MODELS MAX_LIST
#define MAX_SOUNDS MAX_LIST
#define MAX_ENTITIES 512
#define MAX_STATICSOUNDS 256
#define MAX_STATICENTITIES 128
#define MAX_LIGHTSTYLES 64

#define MAX_PROXY_INBUFFER 4096		//max bytes from a downstream proxy.
#define MAX_PROXY_BUFFER (1<<17)	//must be power-of-two (buffer sizes for downstream, both sv/cl)
#define PREFERRED_PROXY_BUFFER	4096 //the ammount of data we try to leave in our input buffer (must be large enough to contain any single mvd frame)

#define ENTS_PER_FRAME 64 //max number of entities per frame (OUCH!).
#define ENTITY_FRAMES 64 //number of frames to remember for deltaing


#define Z_EXT_SERVERTIME	(1<<3)	// STAT_TIME
#define Z_EXT_STRING "8"

//qw specific
#define PRINT_CHAT 3
#define PRINT_HIGH 2
#define PRINT_MEDIUM 1
#define PRINT_LOW 0


#define MAX_STATS 32
#define	STAT_HEALTH			0
#define	STAT_FRAGS			1
#define	STAT_WEAPONMODELI	2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
#define STAT_ITEMS			15

#define STAT_TIME 17	//A ZQ hack, sending time via a stat.
						//this allows l33t engines to interpolate properly without spamming at a silly high fps.




//limits
#define NQ_PACKETS_PER_SECOND 20
#define MAX_MSGLEN 8192		//the biggest datagram size we allow
#define MAX_NQMSGLEN 8000	//nq has large reliable packets for the connection data
#define MAX_QWMSGLEN 1450	//qw is fully split into individual packets
#define MAX_NQDATAGRAM 1024	//nq datagrams are only 1k
#define MAX_BACKBUF_SIZE 1000	//this is smaller so we don't loose too many entities when lagging


//NQ transport layer defines
#define NETFLAG_LENGTH_MASK	0x0000ffff
#define NETFLAG_DATA		0x00010000
#define NETFLAG_ACK			0x00020000
#define NETFLAG_NAK			0x00040000
#define NETFLAG_EOM			0x00080000
#define NETFLAG_UNRELIABLE	0x00100000
#define NETFLAG_CTL			0x80000000

#define CCREQ_CONNECT		0x01
#define CCREQ_SERVER_INFO	0x02

#define CCREP_ACCEPT		0x81
#define CCREP_REJECT		0x82
#define CCREP_SERVER_INFO	0x83

#define NQ_NETCHAN_GAMENAME		"QUAKE"
#define NQ_NETCHAN_VERSION	3
//end NQ specific




//the clcs sent via the udp connections
enum {
	clc_bad		= 0,
	clc_nop		= 1,
	clc_disconnect	= 2,		//NQ only
	clc_move	= 3,		// [[usercmd_t]
	clc_stringcmd	= 4,		// [string] message
	clc_delta	= 5,		// [byte] sequence number, requests delta compression of message
	clc_tmove	= 6,		// teleport request, spectator only
	clc_upload	= 7		// teleport request, spectator only
};

//these are the clcs sent upstream via the tcp streams
enum {
	qtv_clc_bad = 0,
	qtv_clc_stringcmd = 1,
	qtv_clc_commentarydata = 8
};






#define	svc_bad				0
#define	svc_nop				1
#define	svc_disconnect		2
#define	svc_updatestat		3	// [qbyte] [qbyte]
//#define	svc_version			4	// [long] server version (not used anywhere)
#define	svc_nqsetview			5	// [short] entity number
#define	svc_sound			6	// <see code>
#define	svc_nqtime			7	// [float] server time
#define	svc_print			8	// [qbyte] id [string] null terminated string
#define	svc_stufftext		9	// [string] stuffed into client's console buffer
								// the string should be \n terminated
#define	svc_setangle		10	// [angle3] set the view angle to this absolute value

#define	svc_serverdata		11	// [long] protocol ...
#define	svc_lightstyle		12	// [qbyte] [string]
#define	svc_nqupdatename		13	// [qbyte] [string]
#define	svc_updatefrags		14	// [qbyte] [short]
#define	svc_nqclientdata		15	// <shortbits + data>
//#define	svc_stopsound		16	// <see code> (not used anywhere)
#define	svc_nqupdatecolors	17	// [qbyte] [qbyte] [qbyte]
#define	svc_particle		18	// [vec3] <variable>
#define	svc_damage			19

#define	svc_spawnstatic		20
//#define	svc_spawnstatic2	21 (not used anywhere)
#define	svcfte_spawnstatic2	21
#define	svc_spawnbaseline	22

#define	svc_temp_entity		23	// variable
#define	svc_setpause		24	// [qbyte] on / off
#define	svc_nqsignonnum		25	// [qbyte]  used for the signon sequence

#define	svc_centerprint		26	// [string] to put in center of the screen

#define	svc_killedmonster	27
#define	svc_foundsecret		28

#define	svc_spawnstaticsound	29	// [coord3] [qbyte] samp [qbyte] vol [qbyte] aten

#define	svc_intermission	30		// [vec3_t] origin [vec3_t] angle (show scoreboard and stuff)
#define	svc_finale			31		// [string] text ('congratulations blah blah')

#define	svc_cdtrack			32		// [qbyte] track
#define svc_sellscreen		33

//#define svc_cutscene		34	//hmm... nq only... added after qw tree splitt? (intermission without the scores)

#define	svc_smallkick		34		// set client punchangle to 2
#define	svc_bigkick			35		// set client punchangle to 4

#define	svc_updateping		36		// [qbyte] [short]
#define	svc_updateentertime	37		// [qbyte] [float]

#define	svc_updatestatlong	38		// [qbyte] [long]

#define	svc_muzzleflash		39		// [short] entity

#define	svc_updateuserinfo	40		// [qbyte] slot [long] uid
									// [string] userinfo

#define	svc_download		41		// [short] size [size bytes]
#define	svc_playerinfo		42		// variable
#define	svc_nails			43		// [qbyte] num [48 bits] xyzpy 12 12 12 4 8
#define	svc_chokecount		44		// [qbyte] packets choked
#define	svc_modellist		45		// [strings]
#define	svc_soundlist		46		// [strings]
#define	svc_packetentities	47		// [...]
#define	svc_deltapacketentities	48		// [...]
#define svc_maxspeed		49		// maxspeed change, for prediction
#define svc_entgravity		50		// gravity change, for prediction
#define svc_setinfo			51		// setinfo on a client
#define svc_serverinfo		52		// serverinfo
#define svc_updatepl		53		// [qbyte] [qbyte]
#define svc_nails2			54		//mvd only - [qbyte] num [52 bits] nxyzpy 8 12 12 12 4 8

#define svcfte_soundlistshort		56
#define	svcfte_modellistshort		60
#define svcfte_spawnbaseline2		66



#define dem_audio		0

#define dem_cmd			0
#define dem_read		1
#define dem_set			2
#define dem_multiple	3
#define dem_single		4
#define dem_stats		5
#define dem_all			6

#define dem_qtvdata		(dem_all | (1<<4))	//special packet that contains qtv data (spectator chat, etc. clients need to parse this as soon as it is sent to them, which might or might not be awkward for them)

#define dem_mask		7


#define PROTOCOL_VERSION_NQ	15
#define	PROTOCOL_VERSION	28

#define PROTOCOL_VERSION_FTE			(('F'<<0) + ('T'<<8) + ('E'<<16) + ('X' << 24))	//fte extensions.
#define PROTOCOL_VERSION_FTE2			(('F'<<0) + ('T'<<8) + ('E'<<16) + ('2' << 24))	//fte extensions.
#define PROTOCOL_VERSION_EZQUAKE1		(('M'<<0) + ('V'<<8) + ('D'<<16) + ('1' << 24)) //ezquake/mvdsv extensions
#define PROTOCOL_VERSION_HUFFMAN		(('H'<<0) + ('U'<<8) + ('F'<<16) + ('F' << 24))	//packet compression
#define PROTOCOL_VERSION_VARLENGTH		(('v'<<0) + ('l'<<8) + ('e'<<16) + ('n' << 24))	//variable length handshake
#define PROTOCOL_VERSION_FRAGMENT		(('F'<<0) + ('R'<<8) + ('A'<<16) + ('G' << 24))	//supports fragmentation/packets larger than 1450



#define PEXT_SETVIEW			0x00000001
#define PEXT_SCALE				0x00000002	//obsoleted by PEXT2_REPLACEMENTDELTAS
#define PEXT_LIGHTSTYLECOL		0x00000004
#define PEXT_TRANS				0x00000008	//obsoleted by PEXT2_REPLACEMENTDELTAS
#define PEXT_VIEW2				0x00000010
//#define PEXT_BULLETENS			0x00000020 //no longer supported
#define PEXT_ACCURATETIMINGS	0x00000040
#define PEXT_SOUNDDBL			0x00000080	//revised startsound protocol
#define PEXT_FATNESS			0x00000100	//obsoleted by PEXT2_REPLACEMENTDELTAS
#define PEXT_HLBSP				0x00000200
#define PEXT_TE_BULLET			0x00000400	//obsoleted by fully custom particle effects. its an ooold extension, okay? :/
#define PEXT_HULLSIZE			0x00000800	//obsoleted by PEXT2_REPLACEMENTDELTAS
#define PEXT_MODELDBL			0x00001000	//obsoleted by PEXT2_REPLACEMENTDELTAS
#define PEXT_ENTITYDBL			0x00002000	//max of 1024 ents instead of 512. obsoleted by PEXT2_REPLACEMENTDELTAS
#define PEXT_ENTITYDBL2			0x00004000	//max of 2048 ents instead of 512
#define PEXT_FLOATCOORDS		0x00008000	//supports floating point origins.
//#define PEXT_VWEAP				0x00010000	//cause an extra qbyte to be sent, and an extra list of models for vweaps.
#define PEXT_Q2BSP				0x00020000
#define PEXT_Q3BSP				0x00040000
#define PEXT_COLOURMOD			0x00080000	//this replaces an older value which would rarly have caried any actual data.
#define PEXT_SPLITSCREEN		0x00100000
#define PEXT_HEXEN2				0x00200000	//more stats and working particle builtin.
#define PEXT_SPAWNSTATIC2		0x00400000	//Sends an entity delta instead of a baseline. obsoleted by PEXT2_REPLACEMENTDELTAS
#define PEXT_CUSTOMTEMPEFFECTS	0x00800000	//supports custom temp ents.
#define PEXT_256PACKETENTITIES	0x01000000	//Client can recieve 256 packet entities. obsoleted by PEXT2_REPLACEMENTDELTAS
//#define PEXT_NEVERUSED		0x02000000
#define PEXT_SHOWPIC			0x04000000
#define PEXT_SETATTACHMENT		0x08000000	//md3 tags (needs networking, they need to lerp).
//#define PEXT_NEVERUSED		0x10000000
#define PEXT_CHUNKEDDOWNLOADS	0x20000000	//alternate file download method. Hopefully it'll give quadroupled download speed, especially on higher pings.
#define PEXT_CSQC				0x40000000	//csqc additions. extra stats, particle svcs
#define PEXT_DPFLAGS			0x80000000	//extra flags for viewmodel/externalmodel and possible other persistant style flags. obsoleted by PEXT2_REPLACEMENTDELTAS

#define PEXT2_PRYDONCURSOR			0x00000001
#define PEXT2_VOICECHAT				0x00000002
#define PEXT2_SETANGLEDELTA			0x00000004
#define PEXT2_OLDREPLACEMENTDELTAS	0x00000008	//weaponframe was part of the entity state. that flag is now the player's v_angle.
#define PEXT2_MAXPLAYERS			0x00000010	//Client is able to cope with more players than 32. abs max becomes 255, due to colormap issues.
#define PEXT2_PREDINFO				0x00000020	//movevar stats, NQ input sequences+acks.
#define PEXT2_NEWSIZEENCODING		0x00000040	//richer size encoding.
#define PEXT2_INFOBLOBS				0x00000080	//serverinfo+userinfo lengths can be MUCH higher (protocol is unbounded, but expect low sanity limits on userinfo), and contain nulls etc.
//#define PEXT2_PK3DOWNLOADS		0x10000000	//retrieve a list of pk3s/pk3s/paks for downloading (with optional URL and crcs)

#define PEXTE_HIDDENMESSAGES	0x20	//random demo metadata...

//flags on entities
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
#define	UX_EVENMORE	(1<<7)

#define UX_SCALE		(1<<16)	//scaler of alias models
#define UX_ALPHA		(1<<17)	//transparency value
#define UX_FATNESS		(1<<18)	//qbyte describing how fat an alias model should be. (moves verticies along normals). Useful for vacuum chambers...
#define UX_MODELDBL		(1<<19)	//extra bit for modelindexes
#define UX_UNUSED1		(1<<20)
#define UX_ENTITYDBL	(1<<21)	//use an extra qbyte for origin parts, cos one of them is off
#define UX_ENTITYDBL2	(1<<22)	//use an extra qbyte for origin parts, cos one of them is off
#define UX_YETMORE		(1<<23)	//even more extension info stuff.
#define UX_DRAWFLAGS	(1<<24)	//use an extra qbyte for origin parts, cos one of them is off
#define UX_ABSLIGHT		(1<<25)	//Force a lightlevel
#define UX_COLOURMOD	(1<<26)	//rgb
#define UX_DPFLAGS		(1<<27)
#define UX_TAGINFO		(1<<28)
#define UX_LIGHT		(1<<29)
#define	UX_EFFECTS16	(1<<30)
#define UX_FARMORE		(1<<31)



//flags on players
#define	PF_MSEC			(1<<0)
#define	PF_COMMAND		(1<<1)
#define	PF_VELOCITY1	(1<<2)
#define	PF_VELOCITY2	(1<<3)
#define	PF_VELOCITY3	(1<<4)
#define	PF_MODEL		(1<<5)
#define	PF_SKINNUM		(1<<6)
#define	PF_EFFECTS		(1<<7)
#define	PF_WEAPONFRAME	(1<<8)		// only sent for view player
#define	PF_DEAD			(1<<9)		// don't block movement any more
#define	PF_GIB			(1<<10)		// offset the view height differently

//flags on players in mvds
#define DF_ORIGIN		1
#define DF_ANGLES		(1<<3)
#define DF_EFFECTS		(1<<6)
#define DF_SKINNUM		(1<<7)
#define DF_DEAD			(1<<8)
#define DF_GIB			(1<<9)
#define DF_WEAPONFRAME	(1<<10)
#define DF_MODEL		(1<<11)




