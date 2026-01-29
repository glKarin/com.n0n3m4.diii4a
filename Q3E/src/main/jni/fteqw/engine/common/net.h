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
// net.h -- quake's interface to the networking layer

#define	PORT_ANY	-1

#if defined(FTE_TARGET_WEB)
#define HAVE_WEBSOCKCL
#endif

#ifdef __linux__
//#define UNIXSOCKETS
#endif

//FIXME: should split this into loopback/dgram/stream/dtls/tls/irc
//with the ipv4/v6/x as a separate parameter
typedef enum {
	NA_INVALID,
	NA_LOOPBACK,
	/*NA_HYBRID,*/	//ipv6 hybrid socket that might accept ipv4 packets too.
	NA_IP,
	NA_IPV6,
	NA_IPX,
#ifdef UNIXSOCKETS
	NA_UNIX,
#endif
#ifdef IRCCONNECT
	NA_IRC/*remove!*/,
#endif
#ifdef HAVE_WEBSOCKCL
	NA_WEBSOCKET,
#endif
#ifdef SUPPORT_ICE
	NA_ICE
#endif
} netadrtype_t;
typedef enum {
	NP_DGRAM,
	NP_DTLS,	//connected via ICE/WebRTC
	NP_KEXLAN,	//layered over some silly lobby mess
#define NP_ISLAYERED(np) (np>=NP_DTLS && np<=NP_KEXLAN)

	NP_STREAM,
	NP_TLS,
	NP_WS,
	NP_WSS,
	NP_NATPMP,	//server-only scheme for registering public ports.
	NP_RTC_TCP,
	NP_RTC_TLS,	//really need a better way to do this than two copies of every protocol...

	NP_INVALID
} netproto_t;

typedef struct netadr_s
{
	netadrtype_t	type;
	netproto_t		prot;

	unsigned short	port;	//stored as network-endian.
	unsigned short	connum;	//which quake connection/socket the address is talking about. 1-based. 0 is unspecified. this is NOT used for address equivelency.
	unsigned int scopeid;	//ipv6 interface id thing.

	union {
		qbyte	ip[4];
		qbyte	ip6[16];
		qbyte	ipx[10];
#ifdef IRCCONNECT
		struct {
			char host[32];
			char user[32];
			char channel[12];
		} irc;
#endif
#ifdef SUPPORT_ICE
		char icename[64];
#endif
#ifdef HAVE_WEBSOCKCL
		char websocketurl[64];
#endif
#ifdef UNIXSOCKETS
		struct
		{
			int len;	//abstract addresses contain nulls, so this is needed.
			char path[108];
		} un;
#endif
	} address;
} netadr_t;

struct sockaddr_qstorage
{
	short dontusesa_family;
	unsigned char dontusesa_pad[6];
	qint64_t sa_align;
	unsigned char sa_pad2[112];
};


extern	netadr_t	net_local_cl_ipadr;
extern	netadr_t	net_from;		// address of who sent the packet
extern	struct ftenet_generic_connection_s *net_from_connection;
extern	sizebuf_t	net_message;
//#define	MAX_UDP_PACKET	(MAX_MSGLEN*2)	// one more than msg + header
#define	MAX_UDP_PACKET	8192	// one more than msg + header
extern	FTE_ALIGN(4) qbyte		net_message_buffer[MAX_OVERALLMSGLEN];

typedef enum
{
	NETERR_SENT			= 0,	//all is well
	NETERR_NOROUTE		= 1,	//destination isn't valid for this socket/etc. try a different one if possible
	NETERR_DISCONNECTED = 2,	//socket can no longer send anything
	NETERR_MTU			= 3,	//packet wasn't sent due to MTU
	NETERR_CLOGGED		= 4		//socket is suffering from conjestion
} neterr_t;

extern	cvar_t	hostname;

int TCP_OpenStream (netadr_t *remoteaddr, const char *remotename);	//makes things easier. remotename is printable-only

struct ftenet_connections_s;
void		NET_Init (void);
void		NET_Tick (void);
void		SVNET_RegisterCvars(void);
void		NET_InitClient (qboolean loopbackonly);
void		NET_CloseClient(void);
void		NET_InitServer (void);
qboolean	NET_WasSpecialPacket(struct ftenet_connections_s *col);
void		NET_CloseServer (void);
void		UDP_CloseSocket (int socket);
void		NET_Shutdown (void);
qboolean	NET_GetRates(struct ftenet_connections_s *collection, float *pi, float *po, float *bi, float *bo);
qboolean	NET_UpdateRates(struct ftenet_connections_s *collection, qboolean inbound, size_t size);	//for demos to not be weird
void		NET_ReadPackets (struct ftenet_connections_s *collection);
neterr_t	NET_SendPacket (struct ftenet_connections_s *col, int length, const void *data, netadr_t *to);
int			NET_LocalAddressForRemote(struct ftenet_connections_s *collection, netadr_t *remote, netadr_t *local, int idx);
void		NET_PrintAddresses(struct ftenet_connections_s *collection);
qboolean	NET_AddressSmellsFunny(netadr_t *a);
struct dtlspeercred_s;
qboolean	NET_EnsureRoute(struct ftenet_connections_s *collection, char *routename, const struct dtlspeercred_s *peerinfo, const char *adrstring, netadr_t *adr, qboolean outgoing);
void		NET_TerminateRoute(struct ftenet_connections_s *collection, netadr_t *adr);
void		NET_PrintConnectionsStatus(struct ftenet_connections_s *collection);

enum addressscope_e
{
	ASCOPE_PROCESS=0,	//unusable
	ASCOPE_HOST=1,		//unroutable
	ASCOPE_LINK=2,		//unpredictable
	ASCOPE_LAN=3,		//private
	ASCOPE_NET=4		//aka hopefully globally routable
};
enum addressscope_e NET_ClassifyAddress(netadr_t *adr, const char **outdesc);

struct urischeme_s
{
	const char *name;
	netproto_t prot;		//NP_INVALID means its a game-specific one that should really be handled elsewhere but is silently ignored by the netcode.
	netadrtype_t family;	//usually NA_INVALID, unless ipv4/ipv6-specific
	enum
	{
		URISCHEME_NEEDSRESOURCE = (1<<0),	//forwards it on to the server
	} flags;
};
const struct urischeme_s *NET_IsURIScheme(const char *possible);

qboolean NET_AddrIsReliable(netadr_t *adr);	//hints that the protocol is reliable. if so, we don't need to wait for acks
qboolean	NET_IsEncrypted(netadr_t *adr);
qboolean	NET_CompareAdr (netadr_t *a, netadr_t *b);
qboolean	NET_CompareBaseAdr (netadr_t *a, netadr_t *b);
void		NET_AdrToStringResolve (netadr_t *adr, void (*resolved)(void *ctx, void *data, size_t a, size_t b), void *ctx, size_t a, size_t b);
char		*NET_AdrToString (char *s, int len, netadr_t *a);
char		*NET_SockadrToString (char *s, int len, struct sockaddr_qstorage *a, size_t sizeofa);
char		*NET_BaseAdrToString (char *s, int len, netadr_t *a);
size_t		NET_StringToSockaddr2 (const char *s, int defaultport, netadrtype_t afhint, struct sockaddr_qstorage *sadr, int *addrfamily, int *addrsize, size_t addrcount);
qboolean NET_StringToAdr_NoDNS(const char *address, int port, netadr_t *out);
#define NET_StringToSockaddr(s,p,a,f,z) (NET_StringToSockaddr2(s,p,NA_INVALID,a,f,z,1)>0)
size_t		NET_StringToAdr2 (const char *s, int defaultport, netadr_t *a, size_t addrcount, const char **pathstart);
#define NET_StringToAdr(s,p,a) NET_StringToAdr2(s,p,a,1,NULL)
qboolean	NET_PortToAdr (netadrtype_t adrfamily, netproto_t adrprot, const char *s, netadr_t *a);
qboolean NET_IsClientLegal(netadr_t *adr);

qboolean	NET_IsLoopBackAddress (netadr_t *adr);

qboolean NET_StringToAdrMasked (const char *s, qboolean allowdns, netadr_t *a, netadr_t *amask);
char	*NET_AdrToStringMasked (char *s, int len, netadr_t *a, netadr_t *amask);
void NET_IntegerToMask (netadr_t *a, netadr_t *amask, int bits);
qboolean NET_CompareAdrMasked(netadr_t *a, netadr_t *b, netadr_t *mask);

qboolean FTENET_AddToCollection(struct ftenet_connections_s *col, const char *name, const char *address, netadrtype_t addrtype, netproto_t addrprot);

enum certprops_e
{
	QCERT_ISENCRYPTED,		//0 or error
	QCERT_PEERSUBJECT,		//null terminated. should be a hash of the primary cert, ignoring chain.
	QCERT_PEERCERTIFICATE,	//should be the primary cert, ignoring chain. no fixed maximum size required, mostly 2k but probably best to allow at leasy 5k.. or 8k.

	QCERT_LOCALCERTIFICATE,	//the cert we're using/advertising. may have no context. to tell people what fp to expect.

	QCERT_LOBBYSTATUS,		//for special-case lobby wrappers.
	QCERT_LOBBYSENDCHAT,	//to send chat via the stupid lobby instead of the game itself.
};
int NET_GetConnectionCertificate(struct ftenet_connections_s *col, netadr_t *a, enum certprops_e prop, char *out, size_t outsize);

#ifdef HAVE_DTLS
struct dtlscred_s;
struct dtlsfuncs_s;
qboolean NET_DTLS_Create(struct ftenet_connections_s *col, netadr_t *to, const struct dtlscred_s *cred, qboolean outgoing);
qboolean NET_DTLS_Decode(struct ftenet_connections_s *col);
qboolean NET_DTLS_Disconnect(struct ftenet_connections_s *col, netadr_t *to);
extern cvar_t dtls_psk_hint, dtls_psk_user, dtls_psk_key;
extern cvar_t net_enable_dtls;
#endif
#ifdef SUPPORT_ICE
neterr_t ICE_SendPacket(size_t length, const void *data, netadr_t *to);
void ICE_Terminate(netadr_t *to); //if we kicked the client/etc, kill their ICE too.
int ICE_GetPeerCertificate(netadr_t *to, enum certprops_e prop, char *out, size_t outsize);
void ICE_Init(void);
#endif
extern cvar_t timeout;
extern cvar_t tls_ignorecertificateerrors;	//evil evil evil.
struct ftecrypto_s;
qboolean NET_RegisterCrypto(void *module, struct ftecrypto_s *driver);

//============================================================================

#define	OLD_AVG		0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

//#define	MAX_LATENT	32
#define MAX_ADR_SIZE	64

typedef struct
{
	qboolean	fatal_error;

#ifdef NQPROT
	int	isnqprotocol;
	qboolean	nqreliable_allowed;	//says the peer has acked the last reliable (or timed out and needs resending).
	float		nqreliable_resendtime;//force nqreliable_allowed, thereby forcing a resend of anything n
	qbyte		nqunreliableonly;	//nq can't cope with certain reliables some times. if 2, we have a reliable that result in a block (that should be sent). if 1, we are blocking. if 0, we can send reliables freely. if 3, then we just want to ignore clc_moves
#endif
	unsigned int flags;	//NCF_ bitmask
			#define NCF_CLIENT		(1u<<0)	//clientside sends the qport.
			#define NCF_SERVER		(0u<<0)	//serverside reads the qport.
			#define NCF_FRAGABLE	(1u<<1)	//fte's packet fragmentation extension, to avoid issues with low mtus.
			#define NCF_STUNAWARE	(1u<<2)	//prevent the two lead-bits of packets from being either 0(stun), so stray stun packets cannot mess things up for us.
	struct netprim_s netprim;
	int			dupe;				//how many times to dupe packets

	//Packetisation Layer Path MTU Discovery (PLPMTUD / RFC4821)
	int			mtu_cur;				//the path mtu, if known
	int			mtu_min;				//minimum mtu to drop to. lower values being abusive.
	int			mtu_max;				//the size the user specified.
	int			mtu_resends;			//number of times we had to resend a reliable.
	int			mtu_overhead;			//extra slop vs a
	int			outgoing_mtu_probe;		//sequence number that was a probe. if its acked we can send a new one straight away to grow fast. if its not acked, we back off.
	int			mtu_probes;				//number of probes sent without success. give up if we get no growth.
	double		mtu_reprobetime;		//send an extra mtu probe every 30ish secs to see if it grew
	unsigned short sentsizes[64];		//to bump known mtu if an oversized packet got received.
	int			outgoing_sequence_last;	//to detect gaps in outgoing sequences (servers get forced to match client's)

	float		last_received;		// for timeouts

// the statistics are cleared at each client begin, because
// the server connecting process gives a bogus picture of the data
	float		frame_latency;		// rolling average
	float		frame_rate;

	int			drop_count;			// dropped packets, cleared each level

	int			bytesin;
	int			bytesout;

	netadr_t	remote_address;
	int			qport;
	int			qportsize;

// bandwidth estimator
	double		cleartime;			// if realtime > nc->cleartime, free to go
//	double		rate;				// seconds / qbyte

// sequencing variables
	int			incoming_unreliable;	//dictated by the other end.
	int			incoming_sequence;
	int			incoming_acknowledged;
	int			incoming_reliable_acknowledged;	// single bit

	int			incoming_reliable_sequence;		// single bit, maintained local

	int			outgoing_unreliable;
	int			outgoing_sequence;
	int			reliable_sequence;			// single bit
	int			last_reliable_sequence;		// sequence number of last send

// reliable staging and holding areas
	sizebuf_t	message;		// writing buffer to send to server
	qbyte		message_buf[MAX_OVERALLMSGLEN];

	//nq has message truncation.
	int			reliable_length;
	int			reliable_start;
	qbyte		reliable_buf[MAX_OVERALLMSGLEN];	// unacked reliable message

// time and size data to calculate bandwidth
//	int			outgoing_size[MAX_LATENT];
//	double		outgoing_time[MAX_LATENT];
	struct huffman_s	*compresstable;

	//nq servers must recieve truncated packets.
	int in_fragment_length;
	char in_fragment_buf[MAX_OVERALLMSGLEN];
	int in_fragment_start;
} netchan_t;

extern	int	net_drop;		// packets dropped before this one

void Net_Master_Init(void);

void Netchan_Init (void);
size_t Netchan_GetMaxUnreliable(netchan_t *chan);
int Netchan_Transmit (netchan_t *chan, int length, qbyte *data, int rate);
void Netchan_OutOfBand (unsigned int ncflags, netadr_t *adr, int length, const qbyte *data);
void VARGS Netchan_OutOfBandPrint (unsigned int ncflags, netadr_t *adr, char *format, ...) LIKEPRINTF(3);
void VARGS Netchan_OutOfBandTPrintf (unsigned int ncflags, netadr_t *adr, int language, translation_t text, ...);
qboolean Netchan_Process (netchan_t *chan);
void Netchan_Setup (unsigned int ncflags, netchan_t *chan, netadr_t *adr, int qport, unsigned int mtu);
unsigned int Net_PextMask(unsigned int protover, qboolean fornq);
extern cvar_t net_mtu;

qboolean Netchan_CanPacket (netchan_t *chan, int rate);
void Netchan_Block (netchan_t *chan, int bytes, int rate);
qboolean Netchan_CanReliable (netchan_t *chan, int rate);
#ifdef NQPROT
enum nqnc_packettype_e
{
	NQNC_IGNORED,
	NQNC_ACK,
	NQNC_RELIABLE,
	NQNC_UNRELIABLE,
};
enum nqnc_packettype_e NQNetChan_Process(netchan_t *chan);
#endif

#ifdef HUFFNETWORK
#define HUFFCRC_QUAKE3 0x286f2e8d

typedef struct huffman_s huffman_t;
int Huff_PreferedCompressionCRC (void);
void Huff_EncryptPacket(sizebuf_t *msg, int offset);
void Huff_DecryptPacket(sizebuf_t *msg, int offset);
huffman_t *Huff_CompressionCRC(int crc);
void Huff_CompressPacket(huffman_t *huff, sizebuf_t *msg, int offset);
void Huff_DecompressPacket(huffman_t *huff, sizebuf_t *msg, int offset);
int Huff_GetByte(qbyte *buffer, int *count);
void Huff_EmitByte(int ch, qbyte *buffer, int *count);
#endif

#ifdef NQPROT
//taken from nq's net.h
//refer to that for usage info. :)

#define NETFLAG_LENGTH_MASK	0x0000ffff
#define NETFLAG_DATA		0x00010000
#define NETFLAG_ACK			0x00020000
#define NETFLAG_NAK			0x00040000
#define NETFLAG_EOM			0x00080000
#define NETFLAG_UNRELIABLE	0x00100000
#define NETFLAG_ZLIB		0x00200000	//QEx - payload contains the real (full) packet
#define NETFLAG_CTL			0x80000000

#define NQ_NETCHAN_GAMENAME	"QUAKE"
#define NQ_NETCHAN_VERSION	3
#define NQ_NETCHAN_VERSION_QEX	4	//the rerelease's id, used for nqish-over dtls.


#define CCREQ_CONNECT		0x01
#define CCREQ_SERVER_INFO	0x02
#define CCREQ_PLAYER_INFO	0x03
#define CCREQ_RULE_INFO		0x04
#define CCREQ_PROQUAKE_RCON	0x05

#define CCREP_ACCEPT		0x81
#define CCREP_REJECT		0x82
#define CCREP_SERVER_INFO	0x83
#define CCREP_PLAYER_INFO	0x84
#define CCREP_RULE_INFO		0x85

//server->client protocol info
#define PROTOCOL_VERSION_NQ		15
#define PROTOCOL_VERSION_H2		19
#define PROTOCOL_VERSION_NEHD	250
#define PROTOCOL_VERSION_FITZ	666
#define PROTOCOL_VERSION_RMQ	999
#define PROTOCOL_VERSION_DP5	3502
#define PROTOCOL_VERSION_DP6	3503
#define PROTOCOL_VERSION_DP7	3504
#define PROTOCOL_VERSION_BJP1	10000
#define PROTOCOL_VERSION_BJP2	10001
#define PROTOCOL_VERSION_BJP3	10002

#define MOD_PROQUAKE 1
//#define MOD_PROQUAKE_VERSION (10*3.1)		//password feature added
//#define MOD_PROQUAKE_VERSION (10*3.2)		//first 'cheatfree'
#define MOD_PROQUAKE_VERSION (10*3.3)		//no real changes, but w/e, this is the highest we can claim without having serverside issues.
//#define MOD_PROQUAKE_VERSION (10*3.4)		//added nat wait weirdness that's redundant and breaks the whole single-port thing by using two ports on the client too. *sigh*.
//#define MOD_PROQUAKE_VERSION (10*3.5)		//optional cheatfree encryption
//#define MOD_PROQUAKE_VERSION (10*4.51)	//current version

/*RMQ protocol flags*/
#define RMQFL_SHORTANGLE	(1 << 1)
#define RMQFL_FLOATANGLE	(1 << 2)
#define RMQFL_24BITCOORD	(1 << 3)
#define RMQFL_FLOATCOORD	(1 << 4)
#define RMQFL_EDICTSCALE	(1 << 5)
#define RMQFL_ALPHASANITY	(1 << 6)
#define RMQFL_INT32COORD	(1 << 7)
#define RMQFL_MOREFLAGS		(1 << 31)

#endif

int UDP_OpenSocket (int port);
int UDP6_OpenSocket (int port);
int IPX_OpenSocket (int port);
int NetadrToSockadr (netadr_t *a, struct sockaddr_qstorage *s);
void SockadrToNetadr (struct sockaddr_qstorage *s, int sizeofsockaddr, netadr_t *a);
qboolean NET_Sleep(float seconds, qboolean stdinissocket);
