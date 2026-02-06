/*
Interactive Connectivity Establishment (rfc 5245)
find out your peer's potential ports.
spam your peer with stun packets.
see what sticks.
the 'controller' assigns some final candidate pair to ensure that both peers send+receive from a single connection.
if no candidates are available, try using stun to find public nat addresses.

in fte, a 'pair' is actually in terms of each local socket and remote address. hopefully that won't cause too much weirdness.
	this does limit which interfaces we can send packets from, which may cause issues with local TURN relays(does not result in extra prflx candidates) and VPNs(may need to be set up as the default route), and prevents us from being able to report reladdr in candidate offers (again mostly only of use with TURN)
	lan connections should resolve to a host interface anyway

stun test packets must contain all sorts of info. username+integrity+fingerprint for validation. priority+usecandidate+icecontrol(ing) to decree the priority of any new remote candidates, whether its finished, and just who decides whether its finished.
peers don't like it when those are missing.

host candidates - addresses that are directly known (but are probably unroutable private things)
server reflexive candidates - addresses that we found from some public stun server (useful for NATs that use a single public port for each unique private port)
peer reflexive candidates - addresses that our peer finds out about as we spam them
relayed candidates - some sort of socks5 or something proxy.


Note: Even after the ICE connection becomes active, you should continue to collect local candidates and transmit them to the peer out of band.
this allows the connection to pick a new route if some router dies (like a relay kicking us).
FIXME: the client currently disconnects from the broker. the server tracks players via ip rather than ICE.

tcp rtp framing should generally be done with a 16-bit network-endian length prefix followed by the data.

NOTE: we do NOT distinguish between media-level and session-level attributes, as such we can only handle ONE media stream per session. we also don't support rtcp.
*/
/*
webrtc
basically just sctp-over-dtls-over-ice or srtp(negotiated via dtls)-over-ice.
the sctp part is pure bloat+pain for us, but as its required for browser compat we have to support it anyway - but we only use it where we must.
we don't do any srtp stuff at all.
*/
/*
broker
ftemaster provides a broker service
*/

#include "quakedef.h"
#include "netinc.h"

typedef struct
{
	unsigned short msgtype;
	unsigned short msglen;
	unsigned int magiccookie;
	unsigned int transactid[3];
} stunhdr_t;
//class
#define STUN_REQUEST	0x0000
#define STUN_REPLY		0x0100
#define STUN_ERROR		0x0110
#define STUN_INDICATION	0x0010
//request
#define STUN_BINDING	0x0001
#define STUN_ALLOCATE	0x0003 //TURN
#define STUN_REFRESH	0x0004 //TURN
#define STUN_SEND		0x0006 //TURN
#define STUN_DATA		0x0007 //TURN
#define STUN_CREATEPERM	0x0008 //TURN
#define STUN_CHANBIND	0x0009 //TURN

//misc stuff...
#define STUN_MAGIC_COOKIE	0x2112a442

//attributes
#define STUNATTR_MAPPED_ADDRESS			0x0001
#define STUNATTR_USERNAME				0x0006
#define STUNATTR_MSGINTEGRITIY_SHA1		0x0008
#define STUNATTR_ERROR_CODE				0x0009
//#define STUNATTR_CHANNELNUMBER			0x000c	//TURN
#define STUNATTR_LIFETIME				0x000d	//TURN
#define STUNATTR_XOR_PEER_ADDRESS		0x0012	//TURN
#define STUNATTR_DATA					0x0013	//TURN
#define STUNATTR_REALM					0x0014	//TURN
#define STUNATTR_NONCE					0x0015	//TURN
#define STUNATTR_XOR_RELAYED_ADDRESS	0x0016	//TURN
#define STUNATTR_REQUESTED_ADDRFAM		0x0017	//TURN
//#define STUNATTR_EVEN_PORT			0x0018	//TURN
#define STUNATTR_REQUESTED_TRANSPORT	0x0019	//TURN
#define STUNATTR_DONT_FRAGMENT			0x001a	//TURN
#define STUNATTR_MSGINTEGRITIY_SHA2_256	0x001C
#define STUNATTR_PASSWORD_ALGORITHM		0x001D	//yay, screw md5
#define	STUNATTR_XOR_MAPPED_ADDRESS		0x0020
#define	STUNATTR_ICE_PRIORITY			0x0024	//ICE
#define	STUNATTR_ICE_USE_CANDIDATE		0x0025	//ICE

//0x8000 attributes are optional, and may be silently ignored without issue.
#define STUNATTR_ADDITIONAL_ADDRFAM		0x8000	//TURN -- listen for ipv6 in addition to ipv4
#define STUNATTR_SOFTWARE				0x8022	//TURN
#define STUNATTR_FINGERPRINT			0x8028
#define	STUNATTR_ICE_CONTROLLED			0x8029	//ICE
#define STUNATTR_ICE_CONTROLLING		0x802A	//ICE


typedef struct
{
	unsigned short attrtype;
	unsigned short attrlen;
} stunattr_t;
#if defined(SUPPORT_ICE) || (defined(MASTERONLY) && defined(AVAIL_ZLIB))
#include "zlib.h"
#endif
#ifdef SUPPORT_ICE
static cvar_t net_ice_exchangeprivateips = CVARFD("net_ice_exchangeprivateips", "0", CVAR_NOTFROMSERVER, "Boolean. When set to 0, hides private IP addresses from your peers - only addresses determined from the other side of your router will be shared. You should only need to set this to 1 if mdns is unavailable.");
static cvar_t net_ice_allowstun = CVARFD("net_ice_allowstun", "1", CVAR_NOTFROMSERVER, "Boolean. When set to 0, prevents the use of stun to determine our public address (does not prevent connecting to our peer's server-reflexive candidates).");
static cvar_t net_ice_allowturn = CVARFD("net_ice_allowturn", "1", CVAR_NOTFROMSERVER, "Boolean. When set to 0, prevents registration of turn connections (does not prevent connecting to our peer's relay candidates).");
static cvar_t net_ice_allowmdns = CVARFD("net_ice_allowmdns", "1", CVAR_NOTFROMSERVER, "Boolean. When set to 0, prevents the use of multicast-dns to obtain candidates using random numbers instead of revealing private network info.");
static cvar_t net_ice_relayonly = CVARFD("net_ice_relayonly", "0", CVAR_NOTFROMSERVER, "Boolean. When set to 1, blocks reporting non-relay local candidates, does not attempt to connect to remote candidates other than via a relay.");
static cvar_t net_ice_usewebrtc = CVARFD("net_ice_usewebrtc", "", CVAR_NOTFROMSERVER, "Use webrtc's extra overheads rather than simple ICE. This makes packets larger and is slower to connect, but is compatible with the web port.");
static cvar_t net_ice_servers = CVARFD("net_ice_servers", "", CVAR_NOTFROMSERVER, "A space-separated list of ICE servers, eg stun:host.example:3478 or turn:host.example:3478?user=foo?auth=blah");
static cvar_t net_ice_debug = CVARFD("net_ice_debug", "0", CVAR_NOTFROMSERVER, "0: Hide messy details.\n1: Show new candidates.\n2: Show each connectivity test.");

#if 0	//enable this for testing only. will generate excessive error messages with non-hacked turn servers...
	#define ASCOPE_TURN_REQUIRESCOPE	ASCOPE_HOST //try sending it any address.
#else
	#define ASCOPE_TURN_REQUIRESCOPE	ASCOPE_LAN	//don't report loopback/link-local addresses to turn relays.
#endif

struct icecandidate_s
{
	struct icecandinfo_s info;

	struct icecandidate_s *next;

	netadr_t peer;
	//peer needs telling or something.
	qboolean dirty;
	qboolean ismdns;		//indicates that the candidate is a .local domain and thus can be shared without leaking private info.

	//these are bitmasks. one bit for each local socket.
	unsigned int reachable;	//looked like it was up a while ago...
	unsigned int reached;	//timestamp of when it was last reachable. pick a new one if too old.
	unsigned int tried;		//don't try again this round
};
struct icestate_s
{
	struct icestate_s *next;
	void *module;

	netadr_t qadr;			//address reported to the rest of the engine (packets from our peer get remapped to this)
	netadr_t chosenpeer;	//address we're sending our data to.

	struct iceserver_s
	{	//stun/turn servers
		netadr_t addr;
		qboolean isstun;
		unsigned int stunretry;	//once a second, extended to once a minite on reply
		unsigned int stunrnd[3];

		//turn stuff.
		ftenet_generic_connection_t *con;	//TURN needs unique client addresses otherwise it gets confused and starts reporting errors about client ids. make sure these connections are unique.
											//FIXME: should be able to get away with only one if the addr field is unique... move to icestate_s
		netadrtype_t family;	//ipv4 or ipv6. can't send to other peers.
		char *user, *auth;
		enum {
			TURN_UNINITED,		//never tried to poke server, need to send a quicky allocate request.
			TURN_HAVE_NONCE,	//nonce should be valid, need to send a full allocate request.
			TURN_ALLOCATED,		//we have an allocation that we need to refresh every now and then.
			TURN_TERMINATING,		//we have an allocation that we need to refresh every now and then.
		} state;
		unsigned int expires;	//allocation expiry time.
		char *nonce, *realm;	//gathered from the server
//		netadr_t relay, srflx;	//our relayed port, and our local address for the sake of it.

		unsigned int peers;
		struct
		{
			unsigned int expires;		//once a second, extended to once a minite on reply
			unsigned int retry;			//gotta keep retrying
			unsigned int stunrnd[3];	//so we know when to stop retrying.
			struct icecandidate_s *rc;	//if its not rc then we need to reauth now.
		} peer[32];
	} server[8];
	unsigned int servers;

	qboolean brokerless;	//we lost connection to our broker... clean up on failure status.
	unsigned int icetimeout;	//time when we consider the connection dead
	unsigned int keepalive;	//sent periodically...
	unsigned int retries;	//bumped after each round of connectivity checks. affects future intervals.
	enum iceproto_e proto;
	enum icemode_e mode;
	qboolean initiator;		//sends the initial sdpoffer.
	qboolean controlled;	//controller chooses final ports.
	enum icestate_e state;
	char *conname;		//internal id.
	char *friendlyname;	//who you're talking to.

	unsigned int originid;	//should be randomish
	unsigned int originversion;//bumped each time something in the sdp changes.
	char originaddress[16];

	struct icecandidate_s *lc;
	char *lpwd;
	char *lufrag;

	struct icecandidate_s *rc;
	char *rpwd;
	char *rufrag;

	unsigned int tiehigh;
	unsigned int tielow;
	int foundation;

	qboolean blockcandidates;		//don't send candidates yet. FIXME: replace with gathering.
#ifdef HAVE_DTLS
	void *dtlsstate;
	struct sctp_s *sctp;	//ffs! extra processing needed.

	const dtlsfuncs_t *dtlsfuncs;
	qboolean dtlspassive;	//true=server, false=client (separate from ice controller and whether we're hosting. yay...)
	dtlscred_t cred;	//credentials info for dtls (both peer and local info)
	quint16_t mysctpport;
	quint16_t peersctpport;
	qboolean sctpoptional;
	qboolean peersctpoptional; //still uses dtls at least.
#endif

	ftenet_connections_t *connections;	//used only for PRIVATE sockets.

	struct icecodecslot_s
	{
		//FIXME: we should probably include decode state in here somehow so multiple connections don't clobber each other.
		int id;
		char *name;
	} codecslot[34];		//96-127. don't really need to care about other ones.

	struct
	{	//this block is for our inbound udp broker reliability, ensuring we get candidate info to where its needed...
		char *text;
		unsigned int inseq;
		unsigned int outseq;
	} u;
};

typedef struct sctp_s
{
	char *friendlyname;	//for printing/debugging.
	struct icestate_s *icestate;	//for forwarding over ice connections...
	const struct dtlsfuncs_s *dtlsfuncs;	//for forwarding over dtls connects (ice-lite)
	void *dtlsstate;

	quint16_t myport, peerport;
	qboolean peerhasfwdtsn;
	double nextreinit;
	void *cookie;	//sent part of the handshake (might need resending).
	size_t cookiesize;
	struct
	{
		quint32_t verifycode;
		qboolean writable;
		quint32_t tsn;	//Transmit Sequence Number

		quint32_t ctsn;	//acked tsn
		quint32_t losttsn; //total gap size...
	} o;
	struct
	{
		quint32_t verifycode;
		int ackneeded;
		quint32_t ctsn;
		quint32_t htsn;	//so we don't have to walk so many packets to count gaps.
#define SCTP_RCVSIZE 2048	//cannot be bigger than 65536
		qbyte received[SCTP_RCVSIZE/8];

		struct
		{
			quint32_t	firsttns; //so we only ack fragments that were complete.
			quint32_t	tsn; //if a continuation doesn't match, we drop it.
			quint32_t	ppid;
			quint16_t	sid;
			quint16_t	seq;
			size_t		size;
			qboolean	toobig;
			qbyte		buf[65536];
		} r;
	} i;
	unsigned short qstreamid;	//in network endian.
} sctp_t;
#ifdef HAVE_DTLS

static const struct
{
	const char *name;
	hashfunc_t *hash;
} webrtc_hashes[] =
{	//RFC8211 specifies this list of hashes
//	{"md2",	&hash_md2},	//deprecated, hopefully we won't see it
//	{"md5",	&hash_md5},	//deprecated, hopefully we won't see it
	{"sha-1",	&hash_sha1},
	{"sha-224",	&hash_sha2_224},
	{"sha-256",	&hash_sha2_256},
	{"sha-384",	&hash_sha2_384},
	{"sha-512",	&hash_sha2_512},
};
#endif
static neterr_t ICE_Transmit(void *cbctx, const qbyte *data, size_t datasize);
static neterr_t TURN_Encapsulate(struct icestate_s *ice, netadr_t *to, const qbyte *data, size_t datasize);
static void TURN_AuthorisePeer(struct icestate_s *con, struct iceserver_s *srv, int peer);
#ifdef HAVE_DTLS
static neterr_t SCTP_Transmit(sctp_t *sctp, const void *data, size_t length);
#endif
static qboolean ICE_SetFailed(struct icestate_s *con, const char *reasonfmt, ...) LIKEPRINTF(2);
static struct icestate_s *icelist;


const char *ICE_GetCandidateType(struct icecandinfo_s *info)
{
	switch(info->type)
	{
	case ICE_HOST:	return "host";
	case ICE_SRFLX:	return "srflx";
	case ICE_PRFLX:	return "prflx";
	case ICE_RELAY:	return "relay";
	}
	return "?";
}


static struct icecodecslot_s *ICE_GetCodecSlot(struct icestate_s *ice, int slot)
{
	if (slot >= 96 && slot < 96+32)
		return &ice->codecslot[slot-96];
	else if (slot == 0)
		return &ice->codecslot[32];
	else if (slot == 8)
		return &ice->codecslot[33];
	return NULL;
}


#if !defined(SERVERONLY) && defined(VOICECHAT)
extern cvar_t snd_voip_send;
struct rtpheader_s
{
	unsigned char v2_p1_x1_cc4;
	unsigned char m1_pt7;
	unsigned short seq;
	unsigned int timestamp;
	unsigned int ssrc;
	unsigned int csrc[1];	//sized according to cc
};
void S_Voip_RTP_Parse(unsigned short sequence, const char *codec, const unsigned char *data, unsigned int datalen);
qboolean S_Voip_RTP_CodecOkay(const char *codec);
static qboolean NET_RTP_Parse(void)
{
	struct rtpheader_s *rtpheader = (void*)net_message.data;
	if (net_message.cursize >= sizeof(*rtpheader) && (rtpheader->v2_p1_x1_cc4 & 0xc0) == 0x80)
	{
		int hlen;
		int padding = 0;
		struct icestate_s *con;
		//make sure this really came from an accepted rtp stream
		//note that an rtp connection equal to the game connection will likely mess up when sequences start to get big
		//(especially problematic in sane clients that start with a random sequence)
		for (con = icelist; con; con = con->next)
		{
			if (con->state != ICE_INACTIVE && (con->proto == ICEP_VIDEO || con->proto == ICEP_VOICE) && NET_CompareAdr(&net_from, &con->chosenpeer))
			{
				struct icecodecslot_s *codec = ICE_GetCodecSlot(con, rtpheader->m1_pt7 & 0x7f);
				if (codec)	//untracked slot
				{
					char *codecname = codec->name;
					if (!codecname)	//inactive slot
						continue;

					if (rtpheader->v2_p1_x1_cc4 & 0x20)
						padding = net_message.data[net_message.cursize-1];
					hlen = sizeof(*rtpheader);
					hlen += ((rtpheader->v2_p1_x1_cc4 & 0xf)-1) * sizeof(int);
					if (con->proto == ICEP_VOICE)
						S_Voip_RTP_Parse((unsigned short)BigShort(rtpheader->seq), codecname, hlen+(char*)(rtpheader), net_message.cursize - padding - hlen);
//					if (con->proto == ICEP_VIDEO)
//						S_Video_RTP_Parse((unsigned short)BigShort(rtpheader->seq), codecname, hlen+(char*)(rtpheader), net_message.cursize - padding - hlen);
					return true;
				}
			}
		}
	}
	return false;
}
qboolean NET_RTP_Active(void)
{
	struct icestate_s *con;
	for (con = icelist; con; con = con->next)
	{
		if (con->state == ICE_CONNECTED && con->proto == ICEP_VOICE)
			return true;
	}
	return false;
}
qboolean NET_RTP_Transmit(unsigned int sequence, unsigned int timestamp, const char *codec, char *cdata, int clength)
{
	sizebuf_t buf;
	char pdata[512];
	int i;
	struct icestate_s *con;
	qboolean built = false;

	memset(&buf, 0, sizeof(buf));
	buf.maxsize = sizeof(pdata);
	buf.cursize = 0;
	buf.allowoverflow = true;
	buf.data = pdata;

	for (con = icelist; con; con = con->next)
	{
		if (con->state == ICE_CONNECTED && con->proto == ICEP_VOICE)
		{
			for (i = 0; i < countof(con->codecslot); i++)
			{
				if (con->codecslot[i].name && !strcmp(con->codecslot[i].name, codec))
				{
					if (!built)
					{
						built = true;
						MSG_WriteByte(&buf, (2u<<6) | (0u<<5) | (0u<<4) | (0<<0));	//v2_p1_x1_cc4
						MSG_WriteByte(&buf, (0u<<7) | (con->codecslot[i].id<<0));	//m1_pt7
						MSG_WriteShort(&buf, BigShort(sequence&0xffff));	//seq
						MSG_WriteLong(&buf, BigLong(timestamp));	//timestamp
						MSG_WriteLong(&buf, BigLong(0));			//ssrc
						SZ_Write(&buf, cdata, clength);
						if (buf.overflowed)
							return built;
					}
					ICE_Transmit(con, buf.data, buf.cursize);
					break;
				}
			}
		}
	}
	return built;
}
#endif



static struct icestate_s *QDECL ICE_Find(void *module, const char *conname)
{
	struct icestate_s *con;

	for (con = icelist; con; con = con->next)
	{
		if (con->module == module && !strcmp(con->conname, conname))
			return con;
	}
	return NULL;
}
static ftenet_connections_t *ICE_PickConnection(struct icestate_s *con)
{
	if (con->connections)
		return con->connections;
	switch(con->proto)
	{
	default:
		break;
#ifndef SERVERONLY
	case ICEP_VOICE:
	case ICEP_QWCLIENT:
		return cls.sockets;
#endif
#ifndef CLIENTONLY
	case ICEP_QWSERVER:
		return svs.sockets;
#endif
	}
	return NULL;
}
static qboolean TURN_AddXorAddressAttrib(sizebuf_t *buf, unsigned int attr, netadr_t *to)
{	//12 or 24 bytes.
	int alen, atype, aofs, i;
	if (to->type == NA_IP)
	{
		alen = 4;
		atype = 1;
		aofs = 0;
	}
	else if (to->type == NA_IPV6 &&
				!*(int*)&to->address.ip6[0] &&
				!*(int*)&to->address.ip6[4] &&
				!*(short*)&to->address.ip6[8] &&
				*(short*)&to->address.ip6[10] == (short)0xffff)
	{	//just because we use an ipv6 address for ipv4 internally doesn't mean we should tell the peer that they're on ipv6...
		alen = 4;
		atype = 1;
		aofs = sizeof(to->address.ip6) - sizeof(to->address.ip);
	}
	else if (to->type == NA_IPV6)
	{
		alen = 16;
		atype = 2;
		aofs = 0;
	}
	else
		return false;

	MSG_WriteShort(buf, BigShort(attr));
	MSG_WriteShort(buf, BigShort(4+alen));
	MSG_WriteShort(buf, BigShort(atype));
	MSG_WriteShort(buf, to->port ^ *(short*)(buf->data+4));
	for (i = 0; i < alen; i++)
		MSG_WriteByte(buf, ((qbyte*)&to->address)[aofs+i] ^ (buf->data+4)[i]);
	return true;
}
static qboolean TURN_AddAuth(sizebuf_t *buf, struct iceserver_s *srv)
{	//adds auth info to a stun packet
	unsigned short len;
	char integrity[DIGEST_MAXSIZE];
	hashfunc_t *hash = &hash_sha1;
	hashfunc_t *pwdhash = &hash_md5;

	if (!srv->user || !srv->nonce || !srv->realm)
		return false;
	MSG_WriteShort(buf, BigShort(STUNATTR_USERNAME));
	len = strlen(srv->user);
	MSG_WriteShort(buf, BigShort(len));
	SZ_Write (buf, srv->user, len);
	if (len&3)
		SZ_Write (buf, "\0\0\0\0", 4-(len&3));

	MSG_WriteShort(buf, BigShort(STUNATTR_REALM));
	len = strlen(srv->realm);
	MSG_WriteShort(buf, BigShort(len));
	SZ_Write (buf, srv->realm, len);
	if (len&3)
		SZ_Write (buf, "\0\0\0\0", 4-(len&3));

	MSG_WriteShort(buf, BigShort(STUNATTR_NONCE));
	len = strlen(srv->nonce);
	MSG_WriteShort(buf, BigShort(len));
	SZ_Write (buf, srv->nonce, len);
	if (len&3)
		SZ_Write (buf, "\0\0\0\0", 4-(len&3));

	if (pwdhash != &hash_md5)
	{
		MSG_WriteShort(buf, BigShort(STUNATTR_PASSWORD_ALGORITHM));
		len = strlen(srv->nonce);
		MSG_WriteShort(buf, 4);
		if (pwdhash == &hash_md5)
			MSG_WriteShort(buf, 1);
		else if (pwdhash == &hash_sha2_256)
			MSG_WriteShort(buf, 2);
		else
			return false;	//not defined... panic.
		MSG_WriteShort(buf, 0);	//paramlength
		//no params.
	}

	//message integrity is a bit annoying
	buf->data[2] = ((buf->cursize+4+hash->digestsize-20)>>8)&0xff;	//hashed header length is up to the end of the hmac attribute
	buf->data[3] = ((buf->cursize+4+hash->digestsize-20)>>0)&0xff;
	//but the hash is to the start of the attribute's header
	{	//long-term credentials do stuff weird.
		char *tmpkey = va("%s:%s:%s", srv->user, srv->realm, srv->auth);
		len = CalcHash(pwdhash, integrity,sizeof(integrity), tmpkey, strlen(tmpkey));
	}
	len = CalcHMAC(hash, integrity, sizeof(integrity), buf->data, buf->cursize, integrity,len);
	if (hash == &hash_sha2_256)
		MSG_WriteShort(buf, BigShort(STUNATTR_MSGINTEGRITIY_SHA2_256));
	else if (hash == &hash_sha1)
		MSG_WriteShort(buf, BigShort(STUNATTR_MSGINTEGRITIY_SHA1));
	else
		return false;	//not defined!
	MSG_WriteShort(buf, BigShort(len));	//integrity length
	SZ_Write(buf, integrity, len);	//integrity data
	return true;
}

static const char *ICE_NetworkToName(struct icestate_s *ice, int network)
{
	network -= 1;
	if (network >= 0 && network < MAX_CONNECTIONS)
	{	//return the cvar name
		ftenet_connections_t *col = ICE_PickConnection(ice);
		if (col && col->conn[network])
			return col->conn[network]->name;
	}
	else if (network >= MAX_CONNECTIONS)
	{
		network -= MAX_CONNECTIONS;
		if (network >= countof(ice->server))
		{	//a peer-reflexive address from poking a TURN server...
			network -= countof(ice->server);
			if (network < ice->servers)
				return "turn-reflexive";
		}
		else
			return va("turn:%s", ice->server[network].realm);
	}

	return "<UNKNOWN>";
}
static neterr_t TURN_Encapsulate(struct icestate_s *ice, netadr_t *to, const qbyte *data, size_t datasize)
{
	sizebuf_t buf;
	unsigned int network = to->connum-1;
	struct iceserver_s *srv;
	if (to->type == NA_INVALID)
		return NETERR_NOROUTE;
	if (to->connum && network >= MAX_CONNECTIONS)
	{	//fancy turn-related gubbins
		network -= MAX_CONNECTIONS;
		if (network >= countof(ice->server))
		{	//really high, its from the raw socket, unstunned.
			network -= countof(ice->server);
			if (network >= countof(ice->server))
				return NETERR_NOROUTE;
			srv = &ice->server[network];

			if (!srv->con || net_ice_relayonly.ival)
				return NETERR_CLOGGED;
			return srv->con->SendPacket(srv->con, datasize, data, &srv->addr);
		}
		srv = &ice->server[network];

		memset(&buf, 0, sizeof(buf));
		buf.maxsize =	20+//stun header
						8+16+//(max)peeraddr
						4+((datasize+3)&~3);//data
		buf.cursize = 0;
		buf.data = alloca(buf.maxsize);

		MSG_WriteShort(&buf, BigShort(STUN_INDICATION|STUN_SEND));
		MSG_WriteShort(&buf, 0);	//fill in later
		MSG_WriteLong(&buf, BigLong(STUN_MAGIC_COOKIE));
		MSG_WriteLong(&buf, 0);	//randomid
		MSG_WriteLong(&buf, 0);	//randomid
		MSG_WriteLong(&buf, 0);	//randomid

		if (!TURN_AddXorAddressAttrib(&buf, STUNATTR_XOR_PEER_ADDRESS, to))
			return NETERR_NOROUTE;

		MSG_WriteShort(&buf, BigShort(STUNATTR_DATA));
		MSG_WriteShort(&buf, BigShort(datasize));
		SZ_Write(&buf, data, datasize);
		if (datasize&3)
			SZ_Write(&buf, "\0\0\0\0", 4-(datasize&3));

		//fill in the length (for the final time, after filling in the integrity and fingerprint)
		buf.data[2] = ((buf.cursize-20)>>8)&0xff;
		buf.data[3] = ((buf.cursize-20)>>0)&0xff;

		if (!srv->con)
			return NETERR_CLOGGED;
		return srv->con->SendPacket(srv->con, buf.cursize, buf.data, &srv->addr);
	}
	if (net_ice_relayonly.ival)
		return NETERR_CLOGGED;
	return NET_SendPacket(ICE_PickConnection(ice), datasize, data, to);
}
static neterr_t ICE_Transmit(void *cbctx, const qbyte *data, size_t datasize)
{
	struct icestate_s *ice = cbctx;
	if (ice->chosenpeer.type == NA_INVALID)
	{
		if (ice->state == ICE_FAILED)
			return NETERR_DISCONNECTED;
		else
			return NETERR_CLOGGED;
	}

	return TURN_Encapsulate(ice, &ice->chosenpeer, data, datasize);
}

static struct icestate_s *QDECL ICE_Create(void *module, const char *conname, const char *peername, enum icemode_e mode, enum iceproto_e proto, qboolean initiator)
{
	ftenet_connections_t *collection;
	struct icestate_s *con;
	static unsigned int icenum;

	//only allow modes that we actually support.
	if (mode != ICEM_RAW && mode != ICEM_ICE && mode != ICEM_WEBRTC)
		return NULL;
#ifndef HAVE_DTLS
	if (mode == ICEM_WEBRTC)
		return NULL;
#endif

	//only allow protocols that we actually support.
	switch(proto)
	{
	default:
		return NULL;
//	case ICEP_VIDEO:
//		collection = NULL;
//		break;
#if !defined(SERVERONLY) && defined(VOICECHAT)
	case ICEP_VOICE:
	case ICEP_VIDEO:
		collection = cls.sockets;
		if (!collection)
		{
			NET_InitClient(false);
			collection = cls.sockets;
		}
		break;
#endif
#ifndef SERVERONLY
	case ICEP_QWCLIENT:
		collection = cls.sockets;
		if (!collection)
		{
			NET_InitClient(false);
			collection = cls.sockets;
		}
		break;
#endif
#ifndef CLIENTONLY
	case ICEP_QWSERVER:
		collection = svs.sockets;
		break;
#endif
	}
	if (!collection)
		return NULL;	//not initable or something

	if (!conname)
	{
		int rnd[2];
		Sys_RandomBytes((void*)rnd, sizeof(rnd));
		conname = va("fte%08x%08x", rnd[0], rnd[1]);
	}

	if (ICE_Find(module, conname))
		return NULL;	//don't allow dupes.
	
	con = Z_Malloc(sizeof(*con));
	con->conname = Z_StrDup(conname);
	icenum++;
	con->friendlyname = peername?Z_StrDup(peername):Z_StrDupf("%i", icenum);
	con->proto = proto;
	con->rpwd = Z_StrDup("");
	con->rufrag = Z_StrDup("");
	Sys_RandomBytes((void*)&con->originid, sizeof(con->originid));
	con->originversion = 1;
	Q_strncpyz(con->originaddress, "127.0.0.1", sizeof(con->originaddress));

	con->initiator = initiator;
	con->controlled = !initiator;
	con->blockcandidates = true;	//until offers/answers are sent.

#ifdef HAVE_DTLS
	con->dtlspassive = (proto == ICEP_QWSERVER);	//note: may change later.

	if (mode == ICEM_WEBRTC)
	{	//dtls+sctp is a mandatory part of our connection, sadly.
		if (!con->dtlsfuncs)
		{
			if (con->dtlspassive)
				con->dtlsfuncs = DTLS_InitServer();
			else
				con->dtlsfuncs = DTLS_InitClient();	//credentials are a bit different, though fingerprints make it somewhat irrelevant.
		}
		if (con->dtlsfuncs && con->dtlsfuncs->GenTempCertificate && !con->cred.local.certsize)
		{
			Con_DPrintf("Generating dtls certificate...\n");
			if (!con->dtlsfuncs->GenTempCertificate(NULL, &con->cred.local))
			{
				con->dtlsfuncs = NULL;
				mode = ICEM_ICE;
				Con_DPrintf("Failed\n");
			}
			else
				Con_DPrintf("Done\n");
		}
		else
		{	//failure if we can't do the whole dtls thing.
			con->dtlsfuncs = NULL;
			Con_Printf(CON_WARNING"DTLS %s support unavailable, disabling encryption (and webrtc compat).\n", con->dtlspassive?"server":"client");
			mode = ICEM_ICE;	//fall back on unencrypted (this doesn't depend on the peer, so while shitty it hopefully shouldn't be exploitable with a downgrade-attack)
		}

		if (mode == ICEM_WEBRTC && !net_ice_usewebrtc.ival)
			con->sctpoptional = true;

		if (mode == ICEM_WEBRTC)
			con->mysctpport = 27500;
	}

	con->qadr.port = htons(con->mysctpport);
#endif
	con->qadr.type = NA_ICE;
	con->qadr.prot = NP_DGRAM;
	Q_strncpyz(con->qadr.address.icename, con->friendlyname, sizeof(con->qadr.address.icename));
	con->qadr.port = icenum;

	con->mode = mode;

	con->next = icelist;
	icelist = con;

	{
		int rnd[1];	//'must have at least 24 bits randomness'
		Sys_RandomBytes((void*)rnd, sizeof(rnd));
		con->lufrag = Z_StrDupf("%08x", rnd[0]);
	}
	{
		int rnd[4];	//'must have at least 128 bits randomness'
		Sys_RandomBytes((void*)rnd, sizeof(rnd));
		con->lpwd = Z_StrDupf("%08x%08x%08x%08x", rnd[0], rnd[1], rnd[2], rnd[3]);
	}

	Sys_RandomBytes((void*)&con->tiehigh, sizeof(con->tiehigh));
	Sys_RandomBytes((void*)&con->tielow, sizeof(con->tielow));

	if (collection)
	{
		int i, m;

		netadr_t	addr[64];
		struct ftenet_generic_connection_s			*gcon[sizeof(addr)/sizeof(addr[0])];
		unsigned int			flags[sizeof(addr)/sizeof(addr[0])];
		const char *params[sizeof(addr)/sizeof(addr[0])];

		m = NET_EnumerateAddresses(collection, gcon, flags, addr, params, sizeof(addr)/sizeof(addr[0]));

		for (i = 0; i < m; i++)
		{
			if (addr[i].type == NA_IP || addr[i].type == NA_IPV6)
			{
				if (flags[i] & ADDR_REFLEX)
					ICE_AddLCandidateInfo(con, &addr[i], i, ICE_SRFLX); //FIXME: needs reladdr relport info
				else
					ICE_AddLCandidateInfo(con, &addr[i], i, ICE_HOST);
			}
		}
	}

	return con;
}
//if either remotecand is null, new packets will be sent to all.
static qboolean ICE_SendSpam(struct icestate_s *con)
{
	struct icecandidate_s *rc;
	int i;
	int bestlocal = -1;
	struct icecandidate_s *bestpeer = NULL;
	ftenet_connections_t *collection = ICE_PickConnection(con);
	if (!collection)
		return false;

	//only send one ping to each.
	for(rc = con->rc; rc; rc = rc->next)
	{
		for (i = 0; i < MAX_CONNECTIONS; i++)
		{
			if (collection->conn[i] && collection->conn[i]->prot == NP_DGRAM && (collection->conn[i]->addrtype[0]==NA_IP||collection->conn[i]->addrtype[0]==NA_IPV6))
			{
				if (!(rc->tried & (1u<<i)))
				{					
					if (!bestpeer || bestpeer->info.priority < rc->info.priority)
					{
						if (NET_ClassifyAddress(&rc->peer, NULL) < ASCOPE_TURN_REQUIRESCOPE)
						{	//don't waste time asking the relay to poke its loopback. if only because it'll report lots of errors.
							rc->tried |= (1u<<i);
							continue;
						}

						bestpeer = rc;
						bestlocal = i;
					}
				}
			}
		}

		//send via appropriate turn servers
		if (rc->info.type == ICE_RELAY || rc->info.type == ICE_SRFLX)
		{
			for (i = 0; i < con->servers; i++)
			{
				if (con->server[i].state!=TURN_ALLOCATED)
					continue;	//not ready yet...
				if (!con->server[i].con)
					continue;	//can't...
				if (!(rc->tried & (1u<<(MAX_CONNECTIONS+i))))
				{
					//fixme: no local priority. a multihomed machine will try the same ip from different ports.
					if (!bestpeer || bestpeer->info.priority < rc->info.priority)
					{
						if (con->server[i].family && rc->peer.type != con->server[i].family)
							continue;	//if its ipv4-only then don't send it ipv6 packets or whatever.
						bestpeer = rc;
						bestlocal = MAX_CONNECTIONS+i;
					}
				}
			}
		}
	}


	if (bestpeer && bestlocal >= 0)
	{
		neterr_t err;
		netadr_t to;
		sizebuf_t buf;
		char data[512];
		char integ[20];
		int crc;
		qboolean usecandidate = false;
		const char *candtype;
		unsigned int priority;
		memset(&buf, 0, sizeof(buf));
		buf.maxsize = sizeof(data);
		buf.cursize = 0;
		buf.data = data;

		bestpeer->tried |= (1u<<bestlocal);

		to = bestpeer->peer;
		to.connum = 1+bestlocal;

		if (to.type == NA_INVALID)
			return true; //erk?

		safeswitch(bestpeer->info.type)
		{
		case ICE_HOST:	candtype="host";	break;
		case ICE_SRFLX:	candtype="srflx";	break;
		case ICE_PRFLX:	candtype="prflx";	break;
		case ICE_RELAY:	candtype="relay";	break;
		safedefault: candtype="?";
		}

		if (bestlocal >= MAX_CONNECTIONS)
		{
			struct iceserver_s *srv = &con->server[bestlocal-MAX_CONNECTIONS];
			unsigned int i;
			unsigned int now = Sys_Milliseconds();

			for (i = 0; ; i++)
			{
				if (i == srv->peers)
				{
					if (i == countof(srv->peer))
						return true;
					srv->peer[i].expires = now;
					srv->peer[i].rc = bestpeer;
					Sys_RandomBytes((char*)srv->peer[i].stunrnd, sizeof(srv->peer[i].stunrnd));
					srv->peer[i].retry = now;
					srv->peers++;
				}
				if (srv->peer[i].rc == bestpeer)
				{
					break;
				}
			}
			if ((int)(srv->peer[i].retry-now) <= 0)
			{
				srv->peer[i].retry = now + (con->state==ICE_CONNECTED?2000:50);	//keep retrying till we get an ack. be less agressive once it no longer matters so much
				TURN_AuthorisePeer(con, srv, i);
			}
		}

		if (!con->controlled && NET_CompareAdr(&to, &con->chosenpeer) && to.connum == con->chosenpeer.connum)
			usecandidate = true;

		MSG_WriteShort(&buf, BigShort(STUN_BINDING));
		MSG_WriteShort(&buf, 0);	//fill in later
		MSG_WriteLong(&buf, BigLong(STUN_MAGIC_COOKIE));	//magic
		MSG_WriteLong(&buf, BigLong(0));					//randomid
		MSG_WriteLong(&buf, BigLong(0));					//randomid
		MSG_WriteLong(&buf, BigLong(0x80000000|bestlocal));	//randomid

		if (usecandidate)
		{
			MSG_WriteShort(&buf, BigShort(STUNATTR_ICE_USE_CANDIDATE));//ICE-USE-CANDIDATE
			MSG_WriteShort(&buf, BigShort(0));	//just a flag, so no payload to this attribute
		}

		//username
		MSG_WriteShort(&buf, BigShort(STUNATTR_USERNAME));	//USERNAME
		MSG_WriteShort(&buf, BigShort(strlen(con->rufrag) + 1 + strlen(con->lufrag)));
		SZ_Write(&buf, con->rufrag, strlen(con->rufrag));
		MSG_WriteChar(&buf, ':');
		SZ_Write(&buf, con->lufrag, strlen(con->lufrag));
		while(buf.cursize&3)
			MSG_WriteChar(&buf, 0);	//pad

		//priority
		priority =
			//FIXME. should be set to:
			//			priority =	(2^24)*(type preference) +
			//						(2^8)*(local preference) +
			//						(2^0)*(256 - component ID)
			//type preference should be 126 and is a function of the candidate type (direct sending should be highest priority at 126)
			//local preference should reflect multi-homed preferences. ipv4+ipv6 count as multihomed.
			//component ID should be 1 (rtcp would be 2 if we supported it)
			(1<<24)*(bestlocal>=MAX_CONNECTIONS?0:126) +
			(1<<8)*((bestpeer->peer.type == NA_IP?32768:0)+bestlocal*256+(255/*-adrno*/)) +
			(1<<0)*(256 - bestpeer->info.component);
		MSG_WriteShort(&buf, BigShort(STUNATTR_ICE_PRIORITY));//ICE-PRIORITY
		MSG_WriteShort(&buf, BigShort(4));
		MSG_WriteLong(&buf, BigLong(priority));

		//these two attributes carry a random 64bit tie-breaker.
		//the controller is the one with the highest number.
		if (con->controlled)
		{
			MSG_WriteShort(&buf, BigShort(STUNATTR_ICE_CONTROLLED));//ICE-CONTROLLED
			MSG_WriteShort(&buf, BigShort(8));
			MSG_WriteLong(&buf, BigLong(con->tiehigh));
			MSG_WriteLong(&buf, BigLong(con->tielow));
		}
		else
		{
			MSG_WriteShort(&buf, BigShort(STUNATTR_ICE_CONTROLLING));//ICE-CONTROLLING
			MSG_WriteShort(&buf, BigShort(8));
			MSG_WriteLong(&buf, BigLong(con->tiehigh));
			MSG_WriteLong(&buf, BigLong(con->tielow));
		}

		//message integrity is a bit annoying
		data[2] = ((buf.cursize+4+sizeof(integ)-20)>>8)&0xff;	//hashed header length is up to the end of the hmac attribute
		data[3] = ((buf.cursize+4+sizeof(integ)-20)>>0)&0xff;
		//but the hash is to the start of the attribute's header
		CalcHMAC(&hash_sha1, integ, sizeof(integ), data, buf.cursize, con->rpwd, strlen(con->rpwd));
		MSG_WriteShort(&buf, BigShort(STUNATTR_MSGINTEGRITIY_SHA1));	//MESSAGE-INTEGRITY
		MSG_WriteShort(&buf, BigShort(20));	//sha1 key length
		SZ_Write(&buf, integ, sizeof(integ));	//integrity data

		data[2] = ((buf.cursize+8-20)>>8)&0xff;	//dummy length
		data[3] = ((buf.cursize+8-20)>>0)&0xff;
		crc = crc32(0, data, buf.cursize)^0x5354554e;
		MSG_WriteShort(&buf, BigShort(STUNATTR_FINGERPRINT));	//FINGERPRINT
		MSG_WriteShort(&buf, BigShort(sizeof(crc)));
		MSG_WriteLong(&buf, BigLong(crc));

		//fill in the length (for the fourth time, after filling in the integrity and fingerprint)
		data[2] = ((buf.cursize-20)>>8)&0xff;
		data[3] = ((buf.cursize-20)>>0)&0xff;

		err = TURN_Encapsulate(con, &to, buf.data, buf.cursize);

		switch(err)
		{
		case NETERR_SENT:
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY"[%s]: checking %s -> %s:%i (%s)\n", con->friendlyname, ICE_NetworkToName(con, to.connum), bestpeer->info.addr, bestpeer->info.port, candtype);
			break;
		case NETERR_CLOGGED:	//oh well... we retry anyway.
			break;

		case NETERR_NOROUTE:
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY"ICE NETERR_NOROUTE to %s:%i(%s)\n", bestpeer->info.addr, bestpeer->info.port, candtype);
			break;
		case NETERR_DISCONNECTED:
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY"ICE NETERR_DISCONNECTED to %s:%i(%s)\n", bestpeer->info.addr, bestpeer->info.port, candtype);
			break;
		case NETERR_MTU:
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY"ICE NETERR_MTU to %s:%i(%s) (%i bytes)\n", bestpeer->info.addr, bestpeer->info.port, candtype, buf.cursize);
			break;
		default:
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY"ICE send error(%i) to %s:%i(%s)\n", err, bestpeer->info.addr, bestpeer->info.port, candtype);
			break;
		}
		return true;
	}
	return false;
}

#ifdef HAVE_TCP
struct turntcp_connection_s
{	//this sends packets only to the relay, and accepts them only from there too. all packets must be stun packets (for byte-count framing)
	ftenet_generic_connection_t pub;
	vfsfile_t *f;
	struct
	{
		qbyte buf[20+65536];
		unsigned int offset;
		unsigned int avail;
	} recv, send;
	netadr_t adr;
};
static qboolean TURN_TCP_GetPacket(struct ftenet_generic_connection_s *con)
{
	struct turntcp_connection_s *n = (struct turntcp_connection_s*)con;
	qboolean tried = false;
	int err;

	if (n->send.avail)
	{
		err = VFS_WRITE(n->f, n->send.buf+n->send.offset, n->send.avail);
		if (err >= 0)
		{
			n->send.avail -= err;
			n->send.offset += err;
		}
	}

	for (;;)
	{
		if (n->recv.avail >= 20) //must be a stun packet...
		{
			unsigned int psize = 20 + (n->recv.buf[n->recv.offset+2]<<8)+n->recv.buf[n->recv.offset+3];
			if (psize <= n->recv.avail)
			{
				memcpy(net_message.data, n->recv.buf+n->recv.offset, psize);
				n->recv.avail -= psize;
				n->recv.offset += psize;
				net_message.cursize = psize;
				net_from = n->adr;
				return true;
			}
		}
		if (n->recv.offset && n->recv.offset+n->recv.avail == sizeof(n->recv.buf))
		{
			memmove(n->recv.buf, n->recv.buf+n->recv.offset, n->recv.avail);
			n->recv.offset = 0;
		}
		else if (tried)
			return false;	//don't infinitely loop.
		err = VFS_READ(n->f, n->recv.buf+n->recv.offset+n->recv.avail, sizeof(n->recv.buf) - (n->recv.offset+n->recv.avail));
		if (!err)	//no data...
			return false;
		else if (err < 0)
			return false;
		else
		{
			tried = true;
			n->recv.avail += err;
		}
	}
}
static neterr_t TURN_TCP_SendPacket(struct ftenet_generic_connection_s *con, int length, const void *data, netadr_t *to)
{
	int err;
	struct turntcp_connection_s *n = (struct turntcp_connection_s*)con;
	if (!NET_CompareAdr(to, &n->adr))
		return NETERR_NOROUTE;

	//validate the packet - make sure its a TURN one. only checking size here cos we're lazy and that's enough.
	if (length < 20 || length != 20 + (((const qbyte*)data)[2]<<8)+((const qbyte*)data)[3])
		return NETERR_NOROUTE;

	if (!n->send.avail && length < sizeof(n->send.buf))
	{	//avoid copying if we have to
		err = VFS_WRITE(n->f, data, length);
		if (err >= 0 && err < length)
		{	//but sometimes its partial.
			data = (const char*)data + err;
			length -= err;
			n->send.offset = 0;
			memcpy(n->send.buf, data, length);
			n->send.avail = length;
		}
		if (!err)
			return NETERR_CLOGGED;	//mostly so we don't spam while still doing tcp/tls handshakes.
	}
	else
	{
		if (               n->send.avail+length > sizeof(n->send.buf))
			return NETERR_CLOGGED; //can't possibly fit.
		if (n->send.offset+n->send.avail+length > sizeof(n->send.buf))
		{	//move it down if we need.
			memmove(n->send.buf, n->send.buf+n->send.offset, n->send.avail);
			n->send.offset = 0;
		}
		memcpy(n->send.buf+n->send.offset, data, length);
		n->send.avail += length;
		err = VFS_WRITE(n->f, n->send.buf+n->send.offset, n->send.avail);
		if (err >= 0)
		{
			n->send.offset += err;
			n->send.avail -= err;
		}
	}
	if (err >= 0)
	{
		return NETERR_SENT;	//sent something at least.
	}
	else
	{
		//one of the VFS_ERROR_* codes
		return NETERR_DISCONNECTED;
	}
}
static void TURN_TCP_Close(struct ftenet_generic_connection_s *con)
{
	struct turntcp_connection_s *n = (struct turntcp_connection_s*)con;
	if (n->f)
		VFS_CLOSE(n->f);
}
static ftenet_generic_connection_t *TURN_TCP_EstablishConnection(ftenet_connections_t *col, const char *address, netadr_t adr)
{
	struct turntcp_connection_s *n = Z_Malloc(sizeof(*n));
	n->pub.thesocket = TCP_OpenStream(&adr, address);
	n->f = FS_WrapTCPSocket(n->pub.thesocket, true, address);

#ifdef HAVE_SSL
	//convert to tls...
	if (adr.prot == NP_TLS || adr.prot == NP_RTC_TLS)
		n->f = FS_OpenSSL(address, n->f, false);
#endif

	if (!n->f)
	{
		Z_Free(n);
		return NULL;
	}
	n->pub.owner = col;
	n->pub.SendPacket = TURN_TCP_SendPacket;
	n->pub.GetPacket = TURN_TCP_GetPacket;
	n->pub.Close = TURN_TCP_Close;
	n->adr = adr;

	return &n->pub;
}
#endif

static void ICE_ToStunServer(struct icestate_s *con, struct iceserver_s *srv)
{
	sizebuf_t buf;
	char data[512];
	int crc;
	ftenet_connections_t *collection = ICE_PickConnection(con);
	neterr_t err;
	if (!collection)
		return;

	memset(&buf, 0, sizeof(buf));
	buf.maxsize = sizeof(data);
	buf.cursize = 0;
	buf.data = data;

	if (srv->isstun)
	{
		if (!net_ice_allowstun.ival || net_ice_relayonly.ival)
			return;
		if (net_ice_debug.ival >= 2)
			Con_Printf(S_COLOR_GRAY"[%s]: STUN: Checking public IP via %s\n", con->friendlyname, NET_AdrToString(data, sizeof(data), &srv->addr));
		MSG_WriteShort(&buf, BigShort(STUN_BINDING));
	}
	else
	{
		if (!net_ice_allowturn.ival)
		{
			if (net_ice_relayonly.ival)
			{
				Con_Printf("%s: forcing %s on\n", net_ice_relayonly.name, net_ice_allowturn.name);
				Cvar_Set(&net_ice_allowturn, "1");
			}
			return;
		}
		if (!srv->con)
		{
			if (srv->addr.type == NA_INVALID)
				return; //nope...
			if (srv->addr.prot != NP_DGRAM)
			{
#ifdef HAVE_TCP
				srv->con = TURN_TCP_EstablishConnection(collection, srv->realm, srv->addr);
#else
				srv->con = NULL;
#endif
			}
			else
			{
				netadr_t localadr;
				memset(&localadr, 0, sizeof(localadr));
				localadr.type = srv->addr.type;
				localadr.prot = srv->addr.prot;
				srv->con = FTENET_Datagram_EstablishConnection(collection, srv->realm, localadr, NULL);
			}
			if (!srv->con)
			{
				srv->addr.type = NA_INVALID; //fail it.
				return;
			}
			srv->con->connum = 1+MAX_CONNECTIONS+countof(con->server)+(srv-con->server);	//*sigh*
		}

		if (srv->state==TURN_TERMINATING)
		{
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY"[%s]: TURN: Terminating %s\n", con->friendlyname, NET_AdrToString(data, sizeof(data), &srv->addr));
			MSG_WriteShort(&buf, BigShort(STUN_REFRESH));
		}
		else if (srv->state==TURN_ALLOCATED)
		{
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY"[%s]: TURN: Refreshing %s\n", con->friendlyname, NET_AdrToString(data, sizeof(data), &srv->addr));
			MSG_WriteShort(&buf, BigShort(STUN_REFRESH));
		}
		else
		{
			if (net_ice_debug.ival >= 2)
				Con_Printf(S_COLOR_GRAY "[%s]: TURN: Allocating %s\n", con->friendlyname, NET_AdrToString(data, sizeof(data), &srv->addr));
			MSG_WriteShort(&buf, BigShort(STUN_ALLOCATE));
		}
	}
	Sys_RandomBytes((char*)srv->stunrnd, sizeof(srv->stunrnd));

	MSG_WriteShort(&buf, 0);	//fill in later
	MSG_WriteLong(&buf, BigLong(STUN_MAGIC_COOKIE));
	MSG_WriteLong(&buf, srv->stunrnd[0]);	//randomid
	MSG_WriteLong(&buf, srv->stunrnd[1]);	//randomid
	MSG_WriteLong(&buf, srv->stunrnd[2]);	//randomid

	if (!srv->isstun)
	{
		if (srv->state<TURN_ALLOCATED)
		{
			MSG_WriteShort(&buf, BigShort(STUNATTR_REQUESTED_TRANSPORT));
			MSG_WriteShort(&buf, BigShort(4));
			MSG_WriteLong(&buf, 17/*udp*/);

			switch (srv->family)
			{
			case NA_IP:
				//MSG_WriteShort(&buf, BigShort(STUNATTR_REQUESTED_ADDRFAM));
				//MSG_WriteShort(&buf, BigShort(4));
				//MSG_WriteLong(&buf, 1/*ipv4*/);
				break;
			case NA_IPV6:
				MSG_WriteShort(&buf, BigShort(STUNATTR_REQUESTED_ADDRFAM));
				MSG_WriteShort(&buf, BigShort(4));
				MSG_WriteLong(&buf, 2/*ipv6*/);
				break;
			case NA_INVALID:	//ask for both ipv4+ipv6.
				MSG_WriteShort(&buf, BigShort(STUNATTR_ADDITIONAL_ADDRFAM));
				MSG_WriteShort(&buf, BigShort(4));
				MSG_WriteLong(&buf, 2/*ipv6*/);
				break;
			default:
				return;	//nope... not valid.
			}
		}

//		MSG_WriteShort(&buf, BigShort(STUNATTR_DONT_FRAGMENT));
//		MSG_WriteShort(&buf, BigShort(0));

/*		MSG_WriteShort(&buf, BigShort(STUNATTR_SOFTWARE));
		crc = strlen(FULLENGINENAME);
		MSG_WriteShort(&buf, BigShort(crc));
		SZ_Write (&buf, FULLENGINENAME, crc);
		if (crc&3)
			SZ_Write (&buf, "\0\0\0\0", 4-(crc&3));
*/

		if (srv->state==TURN_TERMINATING)
		{
			MSG_WriteShort(&buf, BigShort(STUNATTR_LIFETIME));
			MSG_WriteShort(&buf, BigShort(4));
			MSG_WriteLong(&buf, 0);
		}
		else
		{
//			MSG_WriteShort(&buf, BigShort(STUNATTR_LIFETIME));
//			MSG_WriteShort(&buf, BigShort(4));
//			MSG_WriteLong(&buf, BigLong(300));

			if (srv->state != TURN_UNINITED)
			{
				if (!TURN_AddAuth(&buf, srv))
					return;
			}
		}
	}
	else
	{
		data[2] = ((buf.cursize+8-20)>>8)&0xff;	//dummy length
		data[3] = ((buf.cursize+8-20)>>0)&0xff;
		crc = crc32(0, data, buf.cursize)^0x5354554e;
		MSG_WriteShort(&buf, BigShort(STUNATTR_FINGERPRINT));
		MSG_WriteShort(&buf, BigShort(sizeof(crc)));
		MSG_WriteLong(&buf, BigLong(crc));
	}

	//fill in the length (for the final time, after filling in the integrity and fingerprint)
	data[2] = ((buf.cursize-20)>>8)&0xff;
	data[3] = ((buf.cursize-20)>>0)&0xff;

	if (srv->isstun)
		err = NET_SendPacket(collection, buf.cursize, data, &srv->addr);
	else if (srv->con)
		err = srv->con->SendPacket(srv->con, buf.cursize, data, &srv->addr);
	else
		return;
	if (err == NETERR_CLOGGED)
		srv->stunretry = Sys_Milliseconds();	//just keep retrying until it actually goes through.
}

static void TURN_AuthorisePeer(struct icestate_s *con, struct iceserver_s *srv, int peer)
{
	struct icecandidate_s *rc = srv->peer[peer].rc;
	sizebuf_t buf;
	netadr_t to2;
	char data[512];
	if (srv->state != TURN_ALLOCATED)
		return;
	memset(&buf, 0, sizeof(buf));
	buf.maxsize = sizeof(data);
	buf.cursize = 0;
	buf.data = data;

	MSG_WriteShort(&buf, BigShort(STUN_CREATEPERM));
	MSG_WriteShort(&buf, 0);	//fill in later
	MSG_WriteLong(&buf, BigLong(STUN_MAGIC_COOKIE));		//magic
	MSG_WriteLong(&buf, srv->peer[peer].stunrnd[0]);	//randomid
	MSG_WriteLong(&buf, srv->peer[peer].stunrnd[1]);	//randomid
	MSG_WriteLong(&buf, srv->peer[peer].stunrnd[2]);	//randomid

	if (!TURN_AddXorAddressAttrib(&buf, STUNATTR_XOR_PEER_ADDRESS, &rc->peer))
		return;
	if (*rc->info.reladdr && strcmp(rc->info.addr, rc->info.reladdr))	//allow the relay to bypass the peer's relay if its different (TURN doesn't care about port permissions).
		if (NET_StringToAdr(rc->info.reladdr, rc->info.relport, &to2))
			TURN_AddXorAddressAttrib(&buf, STUNATTR_XOR_PEER_ADDRESS, &to2);
	if (!TURN_AddAuth(&buf, srv))
		return;

	buf.data[2] = ((buf.cursize-20)>>8)&0xff;
	buf.data[3] = ((buf.cursize-20)>>0)&0xff;
	srv->con->SendPacket(srv->con, buf.cursize, buf.data, &srv->addr);

	if (net_ice_debug.ival >= 1)
		Con_Printf(S_COLOR_GRAY"[%s]: (re)registering %s -> %s:%i (%s)\n", con->friendlyname, srv->realm, rc->info.addr, rc->info.port, ICE_GetCandidateType(&rc->info));
}

static void QDECL ICE_AddRCandidateInfo(struct icestate_s *con, struct icecandinfo_s *n);

static struct mdns_peer_s
{
	double nextretry;
	int tries;	//stop sending after 4, forget a couple seconds after that.

	struct icestate_s *con;
	struct icecandinfo_s can;

	struct mdns_peer_s *next;
} *mdns_peers;
static SOCKET mdns_socket = INVALID_SOCKET;
static char mdns_name[2][43];	//client, server (so we reply with the right set of sockets... make per-ice?)

struct dnshdr_s
{
	unsigned short tid, flags, questions, answerrr, authrr, addrr;
};
static qbyte *MDNS_ReadCName(qbyte *in, qbyte *end, char *out, char *outend)
{
	char *cname = out;
	while (*in && in < end)
	{
		if (cname != out)
			*cname++ = '.';
		if (cname+1+*in > outend)
			return end;	//if it overflows then its an error.
		memcpy(cname, in+1, *in);
		cname += *in;
		in += 1+*in;
	}
	*cname = 0;
	return ++in;
}
static void MDNS_ProcessPacket(qbyte *inmsg, size_t inmsgsize, netadr_t *source)
{
	struct dnshdr_s *rh = (struct dnshdr_s *)inmsg;
	unsigned short flags;
	qbyte *in, *end;
	struct {
		unsigned short idx, count;
		char name[256];
		unsigned short type;
		unsigned short class;
		unsigned int ttl;
		qbyte *data;
		unsigned short datasize;
	} a;
	struct mdns_peer_s *peer;

	end = inmsg + inmsgsize;

	//ignore packets from outside the lan...
	if (NET_ClassifyAddress(source, NULL) > ASCOPE_LAN)
		return;
	if (source->port != htons(5353))
		return;	//don't answer/read anything unless its actually mdns. there's supposed to be legacy stuff, but browsers don't seem to respond to that either so lets just play safe.

	if (inmsgsize < sizeof(*rh))
		return;	//some kind of truncation...

	flags = ntohs(rh->flags);
	if (flags & 0x780f)
		return;	//opcode must be 0, response must be 0

	in = (qbyte*)(rh+1);

	if (rh->questions)
	{
		a.count = ntohs(rh->questions);
		for (a.idx = 0; a.idx < a.count; a.idx++)
		{
			ftenet_connections_t *collection = NULL;

			qbyte *questionstart = in;
			in = MDNS_ReadCName(in, end, a.name, a.name+sizeof(a.name));
			if (in+4 > end)
				return;	//error...
			a.type = *in++<<8;
			a.type |= *in++<<0;
			a.class = *in++<<8;
			a.class |= *in++<<0;

#ifdef HAVE_CLIENT
			if (*mdns_name[0] && !strcmp(a.name, mdns_name[0]))
				collection = cls.sockets;
#endif
#ifdef HAVE_SERVER
			if (*mdns_name[1] && !strcmp(a.name, mdns_name[1]))
				collection = svs.sockets;
#endif
			if (collection && (a.type == 1/*A*/ || a.type == 28/*AAAA*/) && a.class == 1/*IN*/)
			{
				qbyte resp[512], *o = resp;
				int n,m, found=0, sz,ty;
				netadr_t	addr[16];
				struct ftenet_generic_connection_s			*gcon[sizeof(addr)/sizeof(addr[0])];
				unsigned int			flags[sizeof(addr)/sizeof(addr[0])];
				const char *params[sizeof(addr)/sizeof(addr[0])];
				struct sockaddr_qstorage dest;
				const unsigned int ttl = 120;

				m = NET_EnumerateAddresses(collection, gcon, flags, addr, params, sizeof(addr)/sizeof(addr[0]));
				*o++ = 0;*o++ = 0;	//tid - must be 0 for mdns responses.
				*o++=0x84;*o++= 0;	//flags
				*o++ = 0;*o++ = 0;	//questions
				*o++ = 0;*o++ = 0;	//answers
				*o++ = 0;*o++ = 0;	//auths
				*o++ = 0;*o++ = 0;	//additionals
				for (n = 0; n < m; n++)
				{
					if (NET_ClassifyAddress(&addr[n], NULL) == ASCOPE_LAN)
					{	//just copy a load of crap over
						if (addr[n].type == NA_IP)
							sz = 4, ty=1;/*A*/
						else if (addr[n].type == NA_IPV6)
							sz = 16, ty=28;/*AAAA*/
						else
							continue;	//nope.
						if (ty != a.type)
							continue;
						a.class |= 0x8000;

						if (o+(in-questionstart)+6+sz > resp+sizeof(resp))
							break;	//won't fit.

						memcpy(o, questionstart, in-questionstart-4);
						o += in-questionstart-4;
						*o++ = ty>>8; *o++ = ty;
						*o++ = a.class>>8; *o++ = a.class;
						*o++ = ttl>>24; *o++ = ttl>>16; *o++ = ttl>>8; *o++ = ttl>>0;
						*o++ = sz>>8; *o++ = sz;
						memcpy(o, &addr[n].address, sz);
						o+=sz;

						found++;
					}
				}
				resp[6] = found>>8; resp[7] = found&0xff;	//replace the answer count now that we actually know

				if (!found)	//don't bother if we can't actually answer it.
					continue;

				//send a multicast response... (browsers don't seem to respond to unicasts).
				if (a.type & 0x8000)
				{	//they asked for a unicast response.
					resp[0] = inmsg[0]; resp[1] = inmsg[1];	//redo the tid.
					sz = NetadrToSockadr(source, &dest);
				}
				else
				{
					sz = sizeof(struct sockaddr_in);
					memset(&dest, 0, sz);
					((struct sockaddr_in*)&dest)->sin_family = AF_INET;
					((struct sockaddr_in*)&dest)->sin_port = htons(5353);
					((struct sockaddr_in*)&dest)->sin_addr.s_addr = inet_addr("224.0.0.251");
				}
				sendto(mdns_socket, resp, o-resp, 0, (struct sockaddr*)&dest, sz);
				if (net_ice_debug.ival)
					Con_Printf(S_COLOR_GRAY"%s: Answering mdns (%s)\n", NET_AdrToString(resp, sizeof(resp), source), a.name);
			}
		}
	}

	a.count = ntohs(rh->answerrr);
	for (a.idx = 0; a.idx < a.count; a.idx++)
	{
		in = MDNS_ReadCName(in, end, a.name, a.name+sizeof(a.name));
		if (in+10 > end)
			return;	//error...
		a.type = *in++<<8;
		a.type |= *in++<<0;
		a.class = *in++<<8;
		a.class |= *in++<<0;
		a.ttl = *in++<<24;
		a.ttl |= *in++<<16;
		a.ttl |= *in++<<8;
		a.ttl |= *in++<<0;
		a.datasize = *in++<<8;
		a.datasize |= *in++<<0;
		a.data = in;
		in += a.datasize;
		if (in > end)
			return;

		for (peer = mdns_peers; peer; peer = peer->next)
		{
			if (!strcmp(a.name, peer->can.addr))
			{	//this is the record we were looking for. yay.

				struct icestate_s *ice;
				for	(ice = icelist; ice; ice = ice->next)
					if (ice == peer->con)
						break;
				if (!ice)
					break;	//no longer valid...

				if ((a.type&0x7fff) == 1/*A*/ && (a.class&0x7fff) == 1/*IN*/ && a.datasize == 4)
				{	//we got a proper ipv4 address. yay.
					Q_snprintfz(peer->can.addr, sizeof(peer->can.addr), "%i.%i.%i.%i", a.data[0], a.data[1], a.data[2], a.data[3]);
				}
				else if ((a.type&0x7fff) == 28/*AAAA*/ && (a.class&0x7fff) == 1/*IN*/ && a.datasize == 16)
				{	//we got a proper ipv4 address. yay.
					Q_snprintfz(peer->can.addr, sizeof(peer->can.addr), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
							(a.data[ 0]<<8)|a.data[ 1],
							(a.data[ 2]<<8)|a.data[ 3],
							(a.data[ 4]<<8)|a.data[ 5],
							(a.data[ 6]<<8)|a.data[ 7],
							(a.data[ 8]<<8)|a.data[ 9],
							(a.data[10]<<8)|a.data[11],
							(a.data[12]<<8)|a.data[13],
							(a.data[14]<<8)|a.data[15]);
				}
				else
				{
					Con_Printf("Useless answer\n");
					break;
				}
				if (net_ice_debug.ival)
					Con_Printf(S_COLOR_GRAY"[%s]: Resolved %s to %s\n", peer->con->friendlyname, a.name, peer->can.addr);
				if (peer->tries != 4)
				{	//first response?...
					peer->tries = 4;
					peer->nextretry = Sys_DoubleTime()+0.5;
				}
				ICE_AddRCandidateInfo(peer->con, &peer->can);	//restore it, so we can handle alternatives properly.
				Q_strncpyz(peer->can.addr, a.name, sizeof(peer->can.addr));
				break;
			}
		}
	}
}
static void MDNS_ReadPackets(void)
{
	qbyte inmsg[9000];
	int inmsgsize;
	netadr_t adr;
	struct sockaddr_qstorage source;

	for(;;)
	{
		int slen = sizeof(source);
		inmsgsize = recvfrom(mdns_socket, inmsg, sizeof(inmsg), 0, (struct sockaddr*)&source, &slen);
		if (inmsgsize <= 0)
			break;
		SockadrToNetadr(&source, slen, &adr);
		MDNS_ProcessPacket(inmsg, inmsgsize, &adr);
	}
}

static void MDNS_Shutdown(void)
{
	if (mdns_socket == INVALID_SOCKET)
		return;
	closesocket(mdns_socket);
	mdns_socket = INVALID_SOCKET;
}

static void MDNS_GenChars(char *d, size_t len, qbyte *s)
{	//big endian hex, big endian data, can just do it by bytes.
	static char tohex[16] = "0123456789abcdef";
	for (; len--; s++)
	{
		*d++ = tohex[*s>>4];
		*d++ = tohex[*s&15];
	}
}
static void MDNS_Generate(char name[43])
{	//generate a suitable mdns name.
	unsigned char uuid[16];
	Sys_RandomBytes((char*)uuid, sizeof(uuid));

	uuid[8]&=~(1<<6);	//clear clk_seq_hi_res bit 6
	uuid[8]|=(1<<7);	//set clk_seq_hi_res bit 7

	uuid[6] &= ~0xf0;	//clear time_hi_and_version's high 4 bits
	uuid[6] |= 0x40;	//replace with version

	MDNS_GenChars(name+0, 4, uuid+0);
	name[8] = '-';
	MDNS_GenChars(name+9, 2, uuid+4);
	name[13] = '-';
	MDNS_GenChars(name+14, 2, uuid+6);
	name[18] = '-';
	MDNS_GenChars(name+19, 2, uuid+8);
	name[23] = '-';
	MDNS_GenChars(name+24, 6, uuid+10);
	strcpy(name+36, ".local");
}

static qboolean MDNS_Setup(void)
{
	struct sockaddr_in adr;
	int _true = true;
//	int _false = false;
	struct ip_mreq mbrship;
	qboolean success = true;

	if (mdns_socket != INVALID_SOCKET)
		return true;	//already got one

	memset(&adr, 0, sizeof(adr));
	adr.sin_family = AF_INET;
	adr.sin_port = htons(5353);
	adr.sin_addr.s_addr = INADDR_ANY;

	memset(&mbrship, 0, sizeof(mbrship));
	mbrship.imr_multiaddr.s_addr = inet_addr("224.0.0.251");

	//browsers don't seem to let us take the easy route, so we can't just use our existing helpers.
	mdns_socket = socket (PF_INET, SOCK_CLOEXEC|SOCK_DGRAM, IPPROTO_UDP);
	if (mdns_socket == INVALID_SOCKET) success = false;
	if (!success || 0 > ioctlsocket(mdns_socket, FIONBIO, (void*)&_true)) success = false;
	//other processes on the same host may be listening for mdns packets to insert their own responses (eg two browsers), so we need to ensure that other processes can use the same port. there's no real security here for us (that comes from stun's user/pass stuff).
	if (!success || 0 > setsockopt (mdns_socket, SOL_SOCKET, SO_REUSEADDR, (const void*)&_true, sizeof(_true))) success = false;
#ifdef SO_REUSEPORT	//not on windows.
	if (!success || 0 > setsockopt (mdns_socket, SOL_SOCKET, SO_REUSEPORT, (const void*)&_true, sizeof(_true))) success = false;
#endif
#if IP_MULTICAST_LOOP	//ideally we'd prefer to not receive our own requests, but this is host-level, not socket-level, so unusable for communication with browsers on the same machine
//	if (success)		setsockopt (mdns_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &_false, sizeof(_false));
#endif
	if (!success || 0 > setsockopt (mdns_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void*)&mbrship, sizeof(mbrship))) success = false;

	adr.sin_addr.s_addr = INADDR_ANY;
	if (!success || bind (mdns_socket, (void *)&adr, sizeof(adr)) < 0)
		success = false;
	if (!success)
	{
		MDNS_Shutdown();
		Con_Printf("mdns setup failed\n");
	}
	else
		MDNS_Generate(mdns_name[0]), MDNS_Generate(mdns_name[1]);
	return success;
}

static void MDNS_SendQuery(struct mdns_peer_s *peer)
{
	char *n = peer->can.addr, *dot;
	struct sockaddr_in dest;
	qbyte outmsg[1024], *o = outmsg;

	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(5353);
	dest.sin_addr.s_addr = inet_addr("224.0.0.251");

	*o++ = 0;*o++ = 0;	//tid
	*o++ = 0;*o++ = 0;	//flags
	*o++ = 0;*o++ = 1;	//questions
	*o++ = 0;*o++ = 0;	//answers
	*o++ = 0;*o++ = 0;	//auths
	*o++ = 0;*o++ = 0;	//additionals

	//mdns is strictly utf-8. no punycode needed.
	for(;;)
	{
		dot = strchr(n, '.');
		if (!dot)
			dot = n + strlen(n);
		if (dot == n)
			return; //err... can't write a 0-length label.
		*o++ = dot-n;
		memcpy(o, n, dot-n); o += dot-n; n += dot-n;
		if (!*n)
			break;
		n++;
	}
	*o++ = 0;

	*o++ = 0; *o++ = 1; //type: 'A' record
	*o++ = 0; *o++ = 1; //class: 'IN'

	sendto(mdns_socket, outmsg, o-outmsg, 0, (struct sockaddr*)&dest, sizeof(dest));
	peer->tries++;
	peer->nextretry = Sys_DoubleTime() + (50/1000.0);
}
static void MDNS_SendQueries(void)
{
	double time;
	struct mdns_peer_s *peer, **link;
	if (mdns_socket == INVALID_SOCKET)
		return;
	MDNS_ReadPackets();
	if (!mdns_peers)
		return;
	time = Sys_DoubleTime();

	for (link = &mdns_peers; (peer=*link); )
	{
		if (peer->nextretry < time)
		{
			if (peer->tries == 4)
			{	//bye bye.
				*link = peer->next;
				BZ_Free(peer);
				continue;
			}

			MDNS_SendQuery(peer);

			if (peer->tries == 4)
				peer->nextretry = Sys_DoubleTime() + 2.0;
			break;	//don't spam multiple each frame.
		}
		link = &peer->next;
	}
}
static void MDNS_AddQuery(struct icestate_s *con, struct icecandinfo_s *can)
{
	struct mdns_peer_s *peer;
	if (!MDNS_Setup())
		return;
	peer = Z_Malloc(sizeof(*peer));
	peer->con = con;
	peer->can = *can;
	peer->next = mdns_peers;
	peer->tries = 0;
	peer->nextretry = Sys_DoubleTime();
	mdns_peers = peer;
	MDNS_SendQuery(peer);
}

static qboolean MDNS_CharsAreHex(char *s, size_t len)
{
	for (; len--; s++)
	{
		if (*s >= '0' && *s <= '9')
			;
		else if (*s >= 'a' && *s <= 'f')
			;
		else
			return false;
	}
	return true;
}

int ParsePartialIP(const char *s, netadr_t *a);
static void QDECL ICE_AddRCandidateInfo(struct icestate_s *con, struct icecandinfo_s *n)
{
	struct icecandidate_s *o;
	qboolean isnew;
	netadr_t peer;
	int peerbits;
	//I don't give a damn about rtcp.
	if (n->component != 1)
		return;
	if (n->transport != 0)
		return;	//only UDP is supported.

	//check if its an mDNS name - must be a UUID, with a .local on the end.
	if (net_ice_allowmdns.ival &&
		MDNS_CharsAreHex(n->addr, 8) && n->addr[8]=='-' &&
		MDNS_CharsAreHex(n->addr+9, 4) && n->addr[13]=='-' &&
		MDNS_CharsAreHex(n->addr+14, 4) && n->addr[18]=='-' &&
		MDNS_CharsAreHex(n->addr+19, 4) && n->addr[23]=='-' &&
		MDNS_CharsAreHex(n->addr+24, 12) && !strcmp(&n->addr[36], ".local"))
	{
		MDNS_AddQuery(con, n);
		return;
	}

	//don't use the regular string->addr, they can fail and stall and make us unresponsive etc. hostnames don't really make sense here anyway.
	peerbits = ParsePartialIP(n->addr, &peer);
	peer.prot = NP_DGRAM;
	peer.port = htons(n->port);
	if (peer.type == NA_IP && peerbits == 32)
	{
		//ignore invalid addresses
		if (!peer.address.ip[0] && !peer.address.ip[1] && !peer.address.ip[2] && !peer.address.ip[3])
			return;
	}
	else if (peer.type == NA_IPV6 && peerbits == 128)
	{
		//ignore invalid addresses
		int i;
		for (i = 0; i < countof(peer.address.ip6); i++)
			if (peer.address.ip6[i])
				break;
		if (i == countof(peer.address.ip6))
			return; //all clear. in6_addr_any
	}
	else
		return;	//bad address type, or partial.

	if (*n->candidateid)
	{
		for (o = con->rc; o; o = o->next)
		{
			//not sure that updating candidates is particuarly useful tbh, but hey.
			if (!strcmp(o->info.candidateid, n->candidateid))
				break;
		}
	}
	else
	{
		for (o = con->rc; o; o = o->next)
		{
			//avoid dupes.
			if (!strcmp(o->info.addr, n->addr) && o->info.port == n->port)
				break;
		}
	}
	if (!o)
	{
		o = Z_Malloc(sizeof(*o));
		o->next = con->rc;
		con->rc = o;
		Q_strncpyz(o->info.candidateid, n->candidateid, sizeof(o->info.candidateid));

		isnew = true;
	}
	else
	{
		isnew = false;
	}
	Q_strncpyz(o->info.addr, n->addr, sizeof(o->info.addr));
	o->info.port = n->port;
	o->info.type = n->type;
	o->info.priority = n->priority;
	o->info.network = n->network;
	o->info.generation = n->generation;
	o->info.foundation = n->foundation;
	o->info.component = n->component;
	o->info.transport = n->transport;
	o->dirty = true;
	o->peer = peer;
	o->tried = 0;
	o->reachable = 0;

	if (net_ice_debug.ival >= 1)
		Con_Printf(S_COLOR_GRAY"[%s]: %s remote candidate %s: [%s]:%i\n", con->friendlyname, isnew?"Added":"Updated", o->info.candidateid, o->info.addr, o->info.port);

	if (n->type == ICE_RELAY && *n->reladdr && (strcmp(n->addr, n->reladdr) || n->port != n->relport))
	{	//for relay candidates, add an srflx candidate too.
		struct icecandinfo_s t = o->info;
		t.type = ICE_SRFLX;
		strcpy(t.addr, n->reladdr);
		t.port = n->relport;
		*t.reladdr = 0;
		t.relport = 0;
		t.priority |= 1<<24;	//nudge its priority up slightly to favour more direct links when we can.
		*t.candidateid = 0;		//anonymous...
		ICE_AddRCandidateInfo(con, &t);
	}
}

static qboolean QDECL ICE_Set(struct icestate_s *con, const char *prop, const char *value);
static void ICE_ParseSDPLine(struct icestate_s *con, const char *value)
{
	if      (!strncmp(value, "a=ice-pwd:", 10))
		ICE_Set(con, "rpwd", value+10);
	else if (!strncmp(value, "a=ice-ufrag:", 12))
		ICE_Set(con, "rufrag", value+12);
#ifdef HAVE_DTLS
	else if (!strncmp(value, "a=setup:", 8))
	{	//this is their state, so we want the opposite.
		if (!strncmp(value+8, "passive", 7))
			con->dtlspassive = false;
		else if (!strncmp(value+8, "active", 6))
			con->dtlspassive = true;
	}
	else if (!strncmp(value, "a=fingerprint:", 14))
	{
		hashfunc_t *hash = NULL;
		int i;
		char name[64];
		value = COM_ParseOut(value+14, name, sizeof(name));
		for (i = 0; i < countof(webrtc_hashes); i++)
		{
			if (!strcasecmp(name, webrtc_hashes[i].name))
			{
				hash = webrtc_hashes[i].hash;
				break;
			}
		}
		if (hash && (!con->cred.peer.hash || hash->digestsize>con->cred.peer.hash->digestsize))	//FIXME: digest size is not a good indicator of whether its exploitable or not, but should work for sha1/sha2 options. the sender here is expected to be trustworthy anyway.
		{
			int b, o, v;
			while (*value == ' ')
				value++;
			for (b = 0; b < hash->digestsize; )
			{
				v = *value;
				if      (v >= '0' && v <= '9')
					o = (v-'0');
				else if (v >= 'A' && v <= 'F')
					o = (v-'A'+10);
				else if (v >= 'a' && v <= 'f')
					o = (v-'a'+10);
				else
					break;
				o <<= 4;
				v = *++value;
				if      (v >= '0' && v <= '9')
					o |= (v-'0');
				else if (v >= 'A' && v <= 'F')
					o |= (v-'A'+10);
				else if (v >= 'a' && v <= 'f')
					o |= (v-'a'+10);
				else
					break;
				con->cred.peer.digest[b++] = o;
				v = *++value;
				if (v != ':')
					break;
				value++;
			}
			if (b == hash->digestsize)
				con->cred.peer.hash = hash;	//it was the right size, woo.
			else
				con->cred.peer.hash = NULL; //bad! (should we 0-pad?)
		}
	}
	else if (!strncmp(value, "a=sctp-port:", 12))
		con->peersctpport = atoi(value+12);
	else if (!strncmp(value, "a=sctp-optional:", 16))
		con->peersctpoptional = atoi(value+16);
#endif
	else if (!strncmp(value, "a=rtpmap:", 9))
	{
		char name[64];
		int codec;
		char *sl;
		value += 9;
		codec = strtoul(value, (char**)&value, 0);
		if (*value == ' ') value++;

		COM_ParseOut(value, name, sizeof(name));
		sl = strchr(name, '/');
		if (sl)
			*sl = '@';
		ICE_Set(con, va("codec%i", codec), name);
	}
	else if (!strncmp(value, "a=candidate:", 12))
	{
		struct icecandinfo_s n;
		memset(&n, 0, sizeof(n));

		value += 12;
		n.foundation = strtoul(value, (char**)&value, 0);

		if(*value == ' ')value++;
		n.component = strtoul(value, (char**)&value, 0);

		if(*value == ' ')value++;
		if (!strncasecmp(value, "UDP ", 4))
		{
			n.transport = 0;
			value += 3;
		}
		else
			return;

		if(*value == ' ')value++;
		n.priority = strtoul(value, (char**)&value, 0);

		if(*value == ' ')value++;
		value = COM_ParseOut(value, n.addr, sizeof(n.addr));
		if (!value) return;

		if(*value == ' ')value++;
		n.port = strtoul(value, (char**)&value, 0);

		if(*value == ' ')value++;
		if (strncmp(value, "typ ", 4)) return;
		value += 3;

		if(*value == ' ')value++;
		if (!strncmp(value, "host", 4))
			n.type = ICE_HOST;
		else if (!strncmp(value, "srflx", 4))
			n.type = ICE_SRFLX;
		else if (!strncmp(value, "prflx", 4))
			n.type = ICE_PRFLX;
		else if (!strncmp(value, "relay", 4))
			n.type = ICE_RELAY;
		else
			return;

		while (*value)
		{
			if(*value == ' ')value++;
			if (!strncmp(value, "raddr ", 6))
			{
				value += 6;
				value = COM_ParseOut(value, n.reladdr, sizeof(n.reladdr));
				if (!value)
					break;
			}
			else if (!strncmp(value, "rport ", 6))
			{
				value += 6;
				n.relport = strtoul(value, (char**)&value, 0);
			}
			/*else if (!strncmp(value, "network-cost ", 13))
			{
				value += 13;
				n.netcost = strtoul(value, (char**)&value, 0);
			}*/
			/*else if (!strncmp(value, "ufrag ", 6))
			{
				value += 6;
				while (*value && *value != ' ')
					value++;
			}*/
			else
			{
				//this is meant to be extensible.
				while (*value && *value != ' ')
					value++;
				if(*value == ' ')value++;
				while (*value && *value != ' ')
					value++;
			}
		}
		ICE_AddRCandidateInfo(con, &n);
	}
}

static qboolean QDECL ICE_Set(struct icestate_s *con, const char *prop, const char *value)
{
	if (!strcmp(prop, "state"))
	{
		int oldstate = con->state;
		if (!strcmp(value, STRINGIFY(ICE_CONNECTING)))
		{
			con->state = ICE_CONNECTING;
			if (net_ice_debug.ival >= 1)
				Con_Printf(S_COLOR_GRAY"[%s]: ice state connecting\n", con->friendlyname);
		}
		else if (!strcmp(value, STRINGIFY(ICE_INACTIVE)))
		{
			con->state = ICE_INACTIVE;
			if (net_ice_debug.ival >= 1)
				Con_Printf(S_COLOR_GRAY"[%s]: ice state inactive\n", con->friendlyname);
		}
		else if (!strcmp(value, STRINGIFY(ICE_FAILED)))
		{
			if (con->state != ICE_FAILED)
			{
				con->state = ICE_FAILED;
				if (net_ice_debug.ival >= 1)
					Con_Printf(S_COLOR_GRAY"[%s]: ice state failed\n", con->friendlyname);
			}
		}
		else if (!strcmp(value, STRINGIFY(ICE_CONNECTED)))
		{
			con->state = ICE_CONNECTED;
			if (net_ice_debug.ival >= 1)
				Con_Printf(S_COLOR_GRAY"[%s]: ice state connected\n", con->friendlyname);
		}
		else
		{
			Con_Printf("ICE_Set invalid state %s\n", value);
			con->state = ICE_INACTIVE;
		}
		con->icetimeout = Sys_Milliseconds() + 30*1000;

		con->retries = 0;

#ifndef SERVERONLY
		if (con->state == ICE_CONNECTING && (con->proto == ICEP_QWCLIENT || con->proto == ICEP_VOICE))
			if (!cls.sockets)
				NET_InitClient(false);
#endif

		if (con->state >= ICE_CONNECTING)
		{
#ifndef SERVERONLY
			if (con->proto == ICEP_QWCLIENT)
				CL_Transfer(&con->qadr);	//okay, the client should be using this ice connection now. FIXME: this should only switch them over if they're still trying to use the aerlier broker.
#endif
#ifdef HAVE_DTLS
			if (con->mode == ICEM_WEBRTC)
			{
				if (!con->dtlsstate && con->dtlsfuncs)
				{
					if (con->cred.peer.hash)
						con->dtlsstate = con->dtlsfuncs->CreateContext(&con->cred, con, ICE_Transmit, con->dtlspassive);
					else if (net_enable_dtls.ival >= 3)
					{	//peer doesn't seem to support dtls.
						ICE_SetFailed(con, "peer does not support dtls. Set net_enable_dtls to 1 to make optional.\n");
					}
					else if (con->state == ICE_CONNECTING && net_enable_dtls.ival>=2)
						Con_Printf(CON_WARNING"WARNING: [%s]: peer does not support dtls.\n", con->friendlyname);
				}
				if (!con->sctp && (!con->sctpoptional || !con->peersctpoptional) && con->mysctpport && con->peersctpport)
				{
					con->sctp = Z_Malloc(sizeof(*con->sctp));
					con->sctp->icestate = con;
					con->sctp->friendlyname = con->friendlyname;
					con->sctp->myport = htons(con->mysctpport);
					con->sctp->peerport = htons(con->peersctpport);
					con->sctp->o.tsn = rand() ^ (rand()<<16);
					Sys_RandomBytes((void*)&con->sctp->o.verifycode, sizeof(con->sctp->o.verifycode));
					Sys_RandomBytes((void*)&con->sctp->i.verifycode, sizeof(con->sctp->i.verifycode));
				}
			}
			else if (!con->dtlsstate && con->cred.peer.hash)
			{
				if (!con->peersctpoptional)
					ICE_SetFailed(con, "peer is trying to use dtls.%s\n", net_enable_dtls.ival?"":" Set ^[/net_enable_dtls 1^].");
			}
#endif
		}

		if (oldstate != con->state && con->state == ICE_INACTIVE)
		{	//forget our peer
			struct icecandidate_s *c;
			int i;
			memset(&con->chosenpeer, 0, sizeof(con->chosenpeer));

#ifdef HAVE_DTLS
			if (con->sctp)
			{
				Z_Free(con->sctp->cookie);
				Z_Free(con->sctp);
				con->sctp = NULL;
			}
			if (con->dtlsstate)
			{
				con->dtlsfuncs->DestroyContext(con->dtlsstate);
				con->dtlsstate = NULL;
			}
#endif
			while(con->rc)
			{
				c = con->rc;
				con->rc = c->next;
				Z_Free(c);
			}
			while(con->lc)
			{
				c = con->lc;
				con->lc = c->next;
				Z_Free(c);
			}
			for (i = 0; i < con->servers; i++)
			{
				struct iceserver_s *s = &con->server[i];
				if (s->con)
				{	//make sure we tell the TURN server to release our allocation.
					s->state = TURN_TERMINATING;
					ICE_ToStunServer(con, s);

					s->con->Close(s->con);
					s->con = NULL;
				}
				Z_Free(s->nonce);
				s->nonce = NULL;
				s->peers = 0;
			}
		}

		if (oldstate != con->state && con->state == ICE_CONNECTED)
		{
			if (con->chosenpeer.type == NA_INVALID)
				ICE_SetFailed(con, "ICE failed. peer not valid.\n");
#ifndef CLIENTONLY
			else if (con->proto == ICEP_QWSERVER && con->mode != ICEM_WEBRTC)
			{
				net_from = con->qadr;
				SVC_GetChallenge(false);
			}
#endif
			if (con->state == ICE_CONNECTED)
			{
				if (con->proto >= ICEP_VOICE || net_ice_debug.ival)
				{
					char msg[256];
					Con_Printf(S_COLOR_GRAY "[%s]: %s connection established (peer %s, via %s).\n", con->friendlyname, con->proto == ICEP_VOICE?"voice":"data", NET_AdrToString(msg, sizeof(msg), &con->chosenpeer), ICE_NetworkToName(con, con->chosenpeer.connum));
				}
			}
		}

#if !defined(SERVERONLY) && defined(VOICECHAT)
		snd_voip_send.ival = (snd_voip_send.ival & ~4) | (NET_RTP_Active()?4:0);
#endif
	}
	else if (!strcmp(prop, "controlled"))
		con->controlled = !!atoi(value);
	else if (!strcmp(prop, "controller"))
		con->controlled = !atoi(value);
	else if (!strncmp(prop, "codec", 5))
	{
		struct icecodecslot_s *codec = ICE_GetCodecSlot(con, atoi(prop+5));
		if (!codec)
			return false;
		codec->id = atoi(prop+5);
#if !defined(SERVERONLY) && defined(VOICECHAT)
		if (!S_Voip_RTP_CodecOkay(value))
#endif
		{
			Z_Free(codec->name);
			codec->name = NULL;
			return false;
		}
		Z_Free(codec->name);
		codec->name = Z_StrDup(value);
	}
	else if (!strcmp(prop, "rufrag"))
	{
		Z_Free(con->rufrag);
		con->rufrag = Z_StrDup(value);
	}
	else if (!strcmp(prop, "rpwd"))
	{
		Z_Free(con->rpwd);
		con->rpwd = Z_StrDup(value);
	}
	else if (!strcmp(prop, "server"))
	{
		netadr_t hostadr[1];
		qboolean okay;
		qboolean tcp = false;
		qboolean tls;
		qboolean stun;
		char *s, *next;
		const char *user=NULL, *auth=NULL;
		const char *host;
		netadrtype_t family = NA_INVALID;
		if (!strncmp(value, "stun:", 5))
			stun=true, tls=false, value+=5;
		else if (!strncmp(value, "turn:", 5))
			stun=false, tls=false, value+=5;
		else if (!strncmp(value, "turns:", 6))
			stun=false, tls=true, value+=6;
		else
			return false;	//nope, uri not known.

		host = Z_StrDup(value);

		s = strchr(host, '?');
		for (;s;s=next)
		{
			*s++ = 0;
			next = strchr(s, '?');
			if (next)
				*next = 0;

			if (!strncmp(s, "transport=", 10))
			{
				if (!strcmp(s+10, "udp"))
					tcp=false;
				else if (!strcmp(s+10, "tcp"))
					tcp=true;
			}
			else if (!strncmp(s, "user=", 5))
				user = s+5;
			else if (!strncmp(s, "auth=", 5))
				auth = s+5;
			else if (!strncmp(s, "fam=", 4))
			{
				if (!strcmp(s+4, "ipv4") || !strcmp(s+4, "ip4") || !strcmp(s+4, "ip") || !strcmp(s+4, "4"))
					family=NA_IP;
				else if (!strcmp(s+4, "ipv6") || !strcmp(s+4, "ip6") || !strcmp(s+4, "6"))
					family=NA_IPV6;
			}
		}

		okay = !strchr(host, '/');
		if (con->servers == countof(con->server))
			okay = false;
		else if (okay)
		{
			struct iceserver_s *srv = &con->server[con->servers];

			//handily both stun and turn default to the same port numbers.
			//FIXME: worker thread...
			okay = NET_StringToAdr2(host, tls?5349:3478, hostadr, countof(hostadr), NULL);
			if (okay)
			{
				if (tls)
					hostadr->prot = tcp?NP_TLS:NP_DTLS;
				else
					hostadr->prot = tcp?NP_STREAM:NP_DGRAM;

				con->servers++;
				srv->isstun = stun;
				srv->family = family;
				srv->realm = Z_StrDup(host);
				Sys_RandomBytes((char*)srv->stunrnd, sizeof(srv->stunrnd));
				srv->stunretry = Sys_Milliseconds();	//'now'...
				srv->addr = *hostadr;
				srv->user = user?Z_StrDup(user):NULL;
				srv->auth = auth?Z_StrDup(auth):NULL;
			}
		}

		Z_Free(s);
		return !!okay;
	}
	else if (!strcmp(prop, "sdp") || !strcmp(prop, "sdpoffer") || !strcmp(prop, "sdpanswer"))
	{
		char line[8192];
		const char *eol;
		for (; *value; value = eol)
		{
			eol = strchr(value, '\n');
			if (!eol)
				eol = value+strlen(value);

			if (eol-value < sizeof(line))
			{
				memcpy(line, value, eol-value);
				line[eol-value] = 0;
				if (eol>value && line[eol-value-1] == '\r')
					line[eol-value-1] = 0;
				ICE_ParseSDPLine(con, line);
			}

			if (eol && *eol)
				eol++;
		}
	}
	else
		return false;
	return true;
}
static qboolean ICE_SetFailed(struct icestate_s *con, const char *reasonfmt, ...)
{
	va_list		argptr;
	char		string[256];

	va_start (argptr, reasonfmt);
	Q_vsnprintfz (string,sizeof(string)-1, reasonfmt,argptr);
	va_end (argptr);

	if (con->state == ICE_FAILED)
		Con_Printf(S_COLOR_GRAY"[%s]: %s\n", con->friendlyname, string);	//we were probably already expecting this. don't show it as a warning.
	else
		Con_Printf(CON_WARNING"[%s]: %s\n", con->friendlyname, string);
	return ICE_Set(con, "state", STRINGIFY(ICE_FAILED));	//does the disconnection stuff.
}
static char *ICE_CandidateToSDP(struct icecandidate_s *can, char *value, size_t valuelen)
{
	Q_snprintfz(value, valuelen, "a=candidate:%i %i %s %i %s %i typ %s",
			can->info.foundation,
			can->info.component,
			can->info.transport==0?"UDP":"ERROR",
			can->info.priority,
			can->info.addr,
			can->info.port,
			ICE_GetCandidateType(&can->info)
			);
	if (can->info.generation)
		Q_strncatz(value, va(" generation %i", can->info.generation), valuelen);	//firefox doesn't like this.
	if (can->info.type != ICE_HOST)
	{
		if (net_ice_relayonly.ival)
		{	//don't leak srflx info (technically this info is mandatory)
			Q_strncatz(value, va(" raddr %s", can->info.addr), valuelen);
			Q_strncatz(value, va(" rport %i", can->info.port), valuelen);
		}
		else
		{
			if (*can->info.reladdr)
				Q_strncatz(value, va(" raddr %s", can->info.reladdr), valuelen);
			else
				Q_strncatz(value, " raddr 0.0.0.0", valuelen);
			Q_strncatz(value, va(" rport %i", can->info.relport), valuelen);
		}
	}

	return value;
}
static qboolean ICE_LCandidateIsPrivate(struct icecandidate_s *caninfo)
{	//return true for the local candidates that we're actually allowed to report. they'll stay flagged as 'dirty' otherwise.
	if (!net_ice_exchangeprivateips.ival && caninfo->info.type == ICE_HOST && !caninfo->ismdns)
		return true;
	if (net_ice_relayonly.ival && caninfo->info.type != ICE_RELAY)
		return true;
	return false;
}
static qboolean QDECL ICE_Get(struct icestate_s *con, const char *prop, char *value, size_t valuelen)
{
	if (!strcmp(prop, "sid"))
		Q_strncpyz(value, con->conname, valuelen);
	else if (!strcmp(prop, "state"))
	{
		switch(con->state)
		{
		case ICE_INACTIVE:
			Q_strncpyz(value, STRINGIFY(ICE_INACTIVE), valuelen);
			break;
		case ICE_FAILED:
			Q_strncpyz(value, STRINGIFY(ICE_FAILED), valuelen);
			break;
		case ICE_GATHERING:
			Q_strncpyz(value, STRINGIFY(ICE_GATHERING), valuelen);
			break;
		case ICE_CONNECTING:
			Q_strncpyz(value, STRINGIFY(ICE_CONNECTING), valuelen);
			break;
		case ICE_CONNECTED:
			Q_strncpyz(value, STRINGIFY(ICE_CONNECTED), valuelen);
			break;
		}
	}
	else if (!strcmp(prop, "lufrag"))
		Q_strncpyz(value, con->lufrag, valuelen);
	else if (!strcmp(prop, "lpwd"))
		Q_strncpyz(value, con->lpwd, valuelen);
	else if (!strncmp(prop, "codec", 5))
	{
		int codecid = atoi(prop+5);
		struct icecodecslot_s *codec = ICE_GetCodecSlot(con, atoi(prop+5));
		if (!codec || codec->id != codecid)
			return false;
		if (codec->name)
			Q_strncpyz(value, codec->name, valuelen);
		else
			Q_strncpyz(value, "", valuelen);
	}
	else if (!strcmp(prop, "newlc"))
	{
		struct icecandidate_s *can;
		Q_strncpyz(value, "0", valuelen);
		for (can = con->lc; can; can = can->next)
		{
			if (can->dirty && !ICE_LCandidateIsPrivate(can))
			{
				Q_strncpyz(value, "1", valuelen);
				break;
			}
		}
	}
	else if (!strcmp(prop, "peersdp"))
	{	//for debugging.
		Q_strncpyz(value, "", valuelen);
		if ((con->proto == ICEP_QWSERVER || con->proto == ICEP_QWCLIENT) && con->mode == ICEM_WEBRTC)
		{
#ifdef HAVE_DTLS
			if (con->cred.peer.hash)
			{
				int b;
				Q_strncatz(value, "a=fingerprint:", valuelen);
				for (b = 0; b < countof(webrtc_hashes); b++)
				{
					if (con->cred.peer.hash == webrtc_hashes[b].hash)
						break;
				}
				Q_strncatz(value, (b==countof(webrtc_hashes))?"UNKNOWN":webrtc_hashes[b].name, valuelen);
				for (b = 0; b < con->cred.peer.hash->digestsize; b++)
					Q_strncatz(value, va(b?":%02X":" %02X", con->cred.peer.digest[b]), valuelen);
				Q_strncatz(value, "\n", valuelen);
			}
#endif
		}
		Q_strncatz(value, va("a=ice-pwd:%s\n", con->rpwd), valuelen);
		Q_strncatz(value, va("a=ice-ufrag:%s\n", con->rufrag), valuelen);

#ifdef HAVE_DTLS
		if (con->peersctpport)
			Q_strncatz(value, va("a=sctp-port:%i\n", con->peersctpport), valuelen);	//stupid hardcoded thing.
		if (con->peersctpoptional)
			Q_strncatz(value, "a=sctp-optional:1\n", valuelen);
#endif



	}
	else if (!strcmp(prop, "sdp") || !strcmp(prop, "sdpoffer") || !strcmp(prop, "sdpanswer"))
	{
		struct icecandidate_s *can;
		netadr_t sender;
		char tmpstr[MAX_QPATH], *at;
		int i;

		{
			netadr_t	addr[1];
			struct ftenet_generic_connection_s *gcon[countof(addr)];
			int			flags[countof(addr)];
			const char *params[countof(addr)];

			if (!NET_EnumerateAddresses(ICE_PickConnection(con), gcon, flags, addr, params, countof(addr)))
			{
				sender.type = NA_INVALID;
				sender.port = 0;
			}
			else
				sender = *addr;
		}

		Q_strncpyz(value, "v=0\n", valuelen);	//version...
		Q_strncatz(value, va("o=%s %u %u IN IP4 %s\n", "-", con->originid, con->originversion, con->originaddress), valuelen);	//originator. usually just dummy info.
		Q_strncatz(value, va("s=%s\n", con->conname), valuelen);	//session name.
		Q_strncatz(value, "t=0 0\n", valuelen);	//start+end times...
		Q_strncatz(value, va("a=ice-options:trickle\n"), valuelen);

		if (con->proto == ICEP_QWSERVER || con->proto == ICEP_QWCLIENT)
		{
#ifdef HAVE_DTLS
			if (net_enable_dtls.ival >= 3)
			{	//this is a preliminary check to avoid wasting time
				if (!con->cred.local.certsize)
					return false;	//fail if we cannot do dtls when its required.
				if (!strcmp(prop, "sdpanswer") && !con->cred.peer.hash)
					return false;	//don't answer if they failed to provide a cert
			}
			if (con->cred.local.certsize)
			{
				qbyte fingerprint[DIGEST_MAXSIZE];
				int b;
				hashfunc_t *hash = &hash_sha2_256;	//browsers use sha-256, lets match them.
				CalcHash(hash, fingerprint, sizeof(fingerprint), con->cred.local.cert, con->cred.local.certsize);
				Q_strncatz(value, "a=fingerprint:sha-256", valuelen);
				for (b = 0; b < hash->digestsize; b++)
					Q_strncatz(value, va(b?":%02X":" %02X", fingerprint[b]), valuelen);
				Q_strncatz(value, "\n", valuelen);

				if (con->mode == ICEM_WEBRTC)
				{
					Q_strncatz(value, "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\n", valuelen);
					if (con->mysctpport)
						Q_strncatz(value, va("a=sctp-port:%i\n", con->mysctpport), valuelen);	//stupid hardcoded thing.
					if (con->sctpoptional)
						Q_strncatz(value, "a=sctp-optional:1\n", valuelen);
				}
				else
					Q_strncatz(value, "m=application 9 UDP/DTLS\n", valuelen);
			}
			else
				Q_strncatz(value, "m=application 9 UDP\n", valuelen);
#endif
		}
//		Q_strncatz(value, va("c=IN %s %s\n", sender.type==NA_IPV6?"IP6":"IP4", NET_BaseAdrToString(tmpstr, sizeof(tmpstr), &sender)), valuelen);
		Q_strncatz(value, "c=IN IP4 0.0.0.0\n", valuelen);

		for (can = con->lc; can; can = can->next)
		{
			char canline[256];
			if (ICE_LCandidateIsPrivate(can))
				continue;	//ignore it.
			can->dirty = false;	//doesn't matter now.
			ICE_CandidateToSDP(can, canline, sizeof(canline));
			Q_strncatz(value, canline, valuelen);
			Q_strncatz(value, "\n", valuelen);
		}

		Q_strncatz(value, va("a=ice-pwd:%s\n", con->lpwd), valuelen);
		Q_strncatz(value, va("a=ice-ufrag:%s\n", con->lufrag), valuelen);

#ifdef HAVE_DTLS
		if (con->dtlsfuncs)
		{
			if (!strcmp(prop, "sdpanswer"))
			{	//answerer decides.
				if (con->dtlspassive)
					Q_strncatz(value, va("a=setup:passive\n"), valuelen);
				else
					Q_strncatz(value, va("a=setup:active\n"), valuelen);
			}
			else if (!strcmp(prop, "sdpoffer"))
				Q_strncatz(value, va("a=setup:actpass\n"), valuelen);	//don't care if we're active or passive
		}
#endif

		/*fixme: merge the codecs into a single media line*/
		for (i = 0; i < countof(con->codecslot); i++)
		{
			int id = con->codecslot[i].id;
			if (!con->codecslot[i].name)
				continue;

			Q_strncatz(value, va("m=audio %i RTP/AVP %i\n", sender.port, id), valuelen);
			Q_strncatz(value, va("b=RS:0\n"), valuelen);
			Q_strncatz(value, va("b=RR:0\n"), valuelen);
			Q_strncpyz(tmpstr, con->codecslot[i].name, sizeof(tmpstr));
			at = strchr(tmpstr, '@');
			if (at)
			{
				*at = '/';
				Q_strncatz(value, va("a=rtpmap:%i %s\n", id, tmpstr), valuelen);
			}
			else
				Q_strncatz(value, va("a=rtpmap:%i %s/%i\n", id, tmpstr, 8000), valuelen);

			for (can = con->lc; can; can = can->next)
			{
				char canline[256];
				can->dirty = false;	//doesn't matter now.
				ICE_CandidateToSDP(can, canline, sizeof(canline));
				Q_strncatz(value, canline, valuelen);
				Q_strncatz(value, "\n", valuelen);
			}
		}
	}
	else
		return false;
	return true;
}

static void ICE_PrintSummary(struct icestate_s *con, qboolean islisten)
{
	char msg[64];

	Con_Printf(S_COLOR_GRAY" ^[[%s]\\ice\\%s^]: ", con->friendlyname, con->friendlyname);
	switch(con->proto)
	{
	case ICEP_VOICE:	Con_Printf(S_COLOR_GRAY"(voice) "); break;
	case ICEP_VIDEO:	Con_Printf(S_COLOR_GRAY"(video) "); break;
	default:			break;
	}
	switch(con->state)
	{
	case ICE_INACTIVE:		Con_Printf(S_COLOR_RED "inactive"); break;
	case ICE_FAILED:		Con_Printf(S_COLOR_RED "failed"); break;
	case ICE_GATHERING:		Con_Printf(S_COLOR_YELLOW "gathering"); break;
	case ICE_CONNECTING:	Con_Printf(S_COLOR_YELLOW "connecting"); break;
	case ICE_CONNECTED:		Con_Printf(S_COLOR_GRAY"%s via %s", NET_AdrToString(msg,sizeof(msg), &con->chosenpeer), ICE_NetworkToName(con, con->chosenpeer.connum)); break;
	}
#ifdef HAVE_DTLS
	if (con->dtlsstate)
		Con_Printf(S_COLOR_GREEN " (encrypted%s)", con->sctp?", sctp":"");
	else if (con->sctp)
		Con_Printf(S_COLOR_RED " (plain-text, sctp)");	//weeeeeeird and pointless...
	else
#endif
		Con_Printf(S_COLOR_RED " (plain-text)");
	Con_Printf("\n");
}
static void ICE_Debug(struct icestate_s *con)
{
	const char *addrclass;
	struct icecandidate_s *can;
	char buf[65536];
	int i;
	ICE_Get(con, "state", buf, sizeof(buf));
	Con_Printf("ICE [%s] (%s):\n", con->friendlyname, buf);
	if (con->brokerless)
		Con_Printf(" timeout: %g\n", (int)(con->icetimeout-Sys_Milliseconds())/1000.0);
	else
	{
		unsigned int idle = (Sys_Milliseconds()+30*1000 - con->icetimeout);
		if (idle > 500)
			Con_Printf(" idle: %g\n", idle/1000.0);
	}
	if (net_ice_debug.ival >= 2)
	{	//rather uninteresting really...
		if (con->initiator)
			ICE_Get(con, "sdpoffer", buf, sizeof(buf));
		else
			ICE_Get(con, "sdpanswer", buf, sizeof(buf));
		Con_Printf("sdp:\n"S_COLOR_YELLOW "%s\n", buf);

		//incomplete anyway
		ICE_Get(con, "peersdp", buf, sizeof(buf));
		Con_Printf("peer:\n"S_COLOR_YELLOW"%s\n", buf);
	}

	Con_Printf(" servers:\n");
	for (i = 0; i < con->servers; i++)
	{
		const char *status = "?";
		switch(con->server[i].state)
		{
		case TURN_UNINITED:		status = "uninited";	break;
		case TURN_HAVE_NONCE:	status = "registering";	break;
		case TURN_ALLOCATED:	status = "allocated";	break;
		case TURN_TERMINATING:	status = "terminating";	break;
		}
		NET_AdrToString(buf,sizeof(buf), &con->server[i].addr);
		Con_Printf("  %s:%s %s realm=%s user=%s auth=%s\n", con->server[i].isstun?"stun":"turn", buf, status, con->server[i].realm, con->server[i].user?con->server[i].user:"<unspecified>", con->server[i].auth?"<hidden>":"<none>");
	}

	Con_Printf(" local:\n");
	for (can = con->lc; can; can = can->next)
	{
		ICE_CandidateToSDP(can, buf, sizeof(buf));
		if (con->chosenpeer.type!=NA_INVALID && con->chosenpeer.connum == can->info.network)
			Con_Printf(S_COLOR_GREEN "  %s"S_COLOR_GRAY" <chosen>\n", buf);
		else if (can->dirty)
			Con_Printf(S_COLOR_RED   "  %s"S_COLOR_GRAY" <not sent>\n", buf);
		else
			Con_Printf(S_COLOR_YELLOW"  %s\n", buf);
	}

	Con_Printf(" remote:\n");
	for (can = con->rc; can; can = can->next)
	{
		ICE_CandidateToSDP(can, buf, sizeof(buf));
		if (can->reachable)
		{
			if (con->chosenpeer.type!=NA_INVALID && NET_CompareAdr(&can->peer,&con->chosenpeer))
				Con_Printf(S_COLOR_GREEN "  %s"S_COLOR_GRAY" <chosen>\n", buf);
			else
				Con_Printf(S_COLOR_YELLOW"  %s"S_COLOR_GRAY" <reachable>\n", buf);
		}
		else if (NET_ClassifyAddress(&can->peer, &addrclass) < ASCOPE_TURN_REQUIRESCOPE)
			Con_Printf(S_COLOR_RED"  %s"S_COLOR_GRAY" <ignored: %s>\n", buf, addrclass);
		else
			Con_Printf(S_COLOR_RED"  %s"S_COLOR_GRAY" <unreachable>\n", buf);
	}
}
static void ICE_Show_f(void)
{
	const char *findname = Cmd_Argv(1);
	struct icestate_s *ice;
	for (ice = icelist; ice; ice = ice->next)
	{
		if (!*findname || !strcmp(findname, ice->friendlyname))
			ICE_Debug(ice);
	}
}
static struct icecandinfo_s *QDECL ICE_GetLCandidateInfo(struct icestate_s *con)
{
	struct icecandidate_s *can;
	for (can = con->lc; can; can = can->next)
	{
		if (can->dirty)
		{
			if (ICE_LCandidateIsPrivate(can))
				continue;

			can->dirty = false;
			return &can->info;
		}
	}
	return NULL;
}

static qboolean QDECL ICE_GetLCandidateSDP(struct icestate_s *con, char *out, size_t outsize)
{
	struct icecandinfo_s *info = ICE_GetLCandidateInfo(con);
	if (info)
	{
		struct icecandidate_s *can = (struct icecandidate_s*)info;
		ICE_CandidateToSDP(can, out, outsize);
		return true;
	}
	return false;
}

static unsigned int ICE_ComputePriority(netadr_t *adr, struct icecandinfo_s *info)
{
	int tpref, lpref;
	switch(info->type)
	{
	case ICE_HOST:	tpref = 126;	break;	//ideal
	case ICE_PRFLX: tpref = 110;	break;
	case ICE_SRFLX: tpref = 100;	break;
	default:
	case ICE_RELAY:	tpref = 0;	break;	//relays suck
	}
	lpref = 0;
	if (info->transport == 0)
		lpref += 0x8000;	//favour udp the most (tcp sucks for stalls)
	lpref += (255-info->network)<<16;	//favour the first network/socket specified
	lpref += (255-NET_ClassifyAddress(adr, NULL))<<8;
	if (adr->type == NA_IP)
		lpref += 0x0001;	//favour ipv4 over ipv6 (less header/mtu overhead...). this is only slight,

	return (tpref<<24) + (lpref<<lpref) + ((256 - info->component)<<0);
}

//adrno is 0 if the type is anything but host.
void QDECL ICE_AddLCandidateInfo(struct icestate_s *con, netadr_t *adr, int adrno, int type)
{
	int rnd[2];
	struct icecandidate_s *cand;
	if (!con)
		return;

	switch(adr->type)
	{
	case NA_IP:
	case NA_IPV6:
		switch(NET_ClassifyAddress(adr, NULL))
		{
		case ASCOPE_PROCESS://doesn't make sense.
		case ASCOPE_HOST:	//don't waste time asking the relay to poke its loopback. if only because it'll report lots of errors.
			return;
		case ASCOPE_NET:	//public addresses, just add local candidates as normal
			break;
		case ASCOPE_LINK:	//random screwy addresses... hopefully don't need em if we're talking to a broker... no dhcp server is weird.
			return;
		case ASCOPE_LAN:	//private addresses. give them random info instead...
			if (net_ice_allowmdns.ival && MDNS_Setup())
			{
				for (cand = con->lc; cand; cand = cand->next)
				{
					if (cand->ismdns)
						return;	//DUPE
				}

				cand = Z_Malloc(sizeof(*cand));
				cand->next = con->lc;
				con->lc = cand;
				Q_strncpyz(cand->info.addr, mdns_name[con->proto == ICEP_QWSERVER], sizeof(cand->info.addr));
				cand->info.port = ntohs(adr->port);
				cand->info.type = type;
				cand->info.generation = 0;
				cand->info.foundation = 1;
				cand->info.component = 1;
				cand->info.network = adr->connum;
				cand->dirty = true;
				cand->ismdns = true;

				Sys_RandomBytes((void*)rnd, sizeof(rnd));
				Q_strncpyz(cand->info.candidateid, va("x%08x%08x", rnd[0], rnd[1]), sizeof(cand->info.candidateid));

				cand->info.priority = ICE_ComputePriority(adr, &cand->info);
				return;
			}
			break;
		}
		break;
	default:	//bad protocols
		return;	//no, just no.
	}
	switch(adr->prot)
	{
	case NP_DTLS:
	case NP_DGRAM:
		break;
	default:
		return;	//don't report any tcp/etc connections...
	}

	for (cand = con->lc; cand; cand = cand->next)
	{
		if (NET_CompareAdr(adr, &cand->peer))
			return; //DUPE
	}

	cand = Z_Malloc(sizeof(*cand));
	cand->next = con->lc;
	con->lc = cand;
	NET_BaseAdrToString(cand->info.addr, sizeof(cand->info.addr), adr);
	cand->info.port = ntohs(adr->port);
	cand->info.type = type;
	cand->info.generation = 0;
	cand->info.foundation = 1;
	cand->info.component = 1;
	cand->info.network = adr->connum;
	cand->dirty = true;

	Sys_RandomBytes((void*)rnd, sizeof(rnd));
	Q_strncpyz(cand->info.candidateid, va("x%08x%08x", rnd[0], rnd[1]), sizeof(cand->info.candidateid));

	cand->info.priority = ICE_ComputePriority(adr, &cand->info);
}
void QDECL ICE_AddLCandidateConn(ftenet_connections_t *col, netadr_t *addr, int type)
{
	struct icestate_s *ice;
	for (ice = icelist; ice; ice = ice->next)
	{
		if (ICE_PickConnection(ice) == col)
			ICE_AddLCandidateInfo(ice, addr, 0, type);
	}
}

static void ICE_Destroy(struct icestate_s *con)
{
	struct icecandidate_s *c;

	ICE_Set(con, "state", STRINGIFY(ICE_INACTIVE));

#ifdef HAVE_DTLS
	if (con->sctp)
	{
		Z_Free(con->sctp->cookie);
		Z_Free(con->sctp);
	}
	if (con->dtlsstate)
		con->dtlsfuncs->DestroyContext(con->dtlsstate);
	if (con->cred.local.cert)
		Z_Free(con->cred.local.cert);
	if (con->cred.local.key)
		Z_Free(con->cred.local.key);
#endif
	while(con->rc)
	{
		c = con->rc;
		con->rc = c->next;
		Z_Free(c);
	}
	while(con->lc)
	{
		c = con->lc;
		con->lc = c->next;
		Z_Free(c);
	}
	while (con->servers)
	{
		struct iceserver_s *s = &con->server[--con->servers];
		if (s->con)
		{	//make sure we tell the TURN server to release our allocation.
			s->state = TURN_TERMINATING;
			ICE_ToStunServer(con, s);

			s->con->Close(s->con);
		}
		Z_Free(s->user);
		Z_Free(s->auth);
		Z_Free(s->realm);
		Z_Free(s->nonce);
	}
	if (con->connections)
		FTENET_CloseCollection(con->connections);
	Z_Free(con->lufrag);
	Z_Free(con->lpwd);
	Z_Free(con->rufrag);
	Z_Free(con->rpwd);
	Z_Free(con->friendlyname);
	Z_Free(con->conname);
	//has already been unlinked
	Z_Free(con);
}

//send pings to establish/keep the connection alive
void ICE_Tick(void)
{
	struct icestate_s **link, *con;
	unsigned int curtime;

	if (!icelist)
		return;
	curtime = Sys_Milliseconds();

	MDNS_SendQueries();

	for (link = &icelist; (con=*link);)
	{
		if (con->brokerless)
		{
			if (con->state <= ICE_GATHERING)
			{
				*link = con->next;
				ICE_Destroy(con);
				continue;
			}
			else if ((signed int)(curtime-con->icetimeout) > 0)
				ICE_SetFailed(con, S_COLOR_GRAY"[%s]: ice timeout\n", con->friendlyname);
		}

		switch(con->mode)
		{
		case ICEM_RAW:
			//raw doesn't do handshakes or keepalives. it should just directly connect.
			//raw just uses the first (assumed only) option
			if (con->state == ICE_CONNECTING)
			{
				struct icecandidate_s *rc;
				rc = con->rc;
				if (!rc || !NET_StringToAdr(rc->info.addr, rc->info.port, &con->chosenpeer))
					con->chosenpeer.type = NA_INVALID;
				ICE_Set(con, "state", STRINGIFY(ICE_CONNECTED));
			}
			break;
		case ICEM_WEBRTC:
		case ICEM_ICE:
			if (con->state == ICE_CONNECTING || con->state == ICE_CONNECTED)
			{
				size_t i, j;
				struct iceserver_s *srv;
				for (i = 0; i < con->servers; i++)
				{
					srv = &con->server[i];
					if ((signed int)(srv->stunretry-curtime) < 0)
					{
						srv->stunretry = curtime + 2*1000;
						ICE_ToStunServer(con, srv);
					}
					for (j = 0; j < srv->peers; j++)
					{
						if ((signed int)(srv->peer[j].retry-curtime) < 0)
						{
							TURN_AuthorisePeer(con, srv, j);
							srv->peer[j].retry = curtime + 2*1000;
						}
					}
					if (srv->con)
					{
						while (srv->con->GetPacket(srv->con))
						{
							net_from.connum = srv->con->connum;
							net_from_connection = srv->con;
							srv->con->owner->ReadGamePacket();
						}
					}
				}
				if (con->keepalive < curtime)
				{
					if (!ICE_SendSpam(con))
					{
						struct icecandidate_s *rc;
						struct icecandidate_s *best = NULL;

						for (rc = con->rc; rc; rc = rc->next)
						{	//FIXME:
							if (rc->reachable && (!best || rc->info.priority > best->info.priority))
								best = rc;
						}

						if (best)
						{
							netadr_t nb = best->peer;
							for (i = 0; ; i++)
							{
								if (best->reachable&(1<<i))
								{
									best->tried &= ~(1<<i);	//keep poking it...
									nb.connum = i+1;
									break;
								}
							}
							if (memcmp(&con->chosenpeer, &nb, sizeof(nb)) && (con->chosenpeer.type==NA_INVALID || !con->controlled))
							{	//it actually changed... let them know NOW.
								best->tried &= ~(1<<(con->chosenpeer.connum-1));	//make sure we resend this one.
								con->chosenpeer = nb;
								ICE_SendSpam(con);

								if (net_ice_debug.ival >= 1)
								{
									char msg[64];
									Con_Printf(S_COLOR_GRAY"[%s]: New peer chosen %s (%s), via %s.\n", con->friendlyname, NET_AdrToString(msg, sizeof(msg), &con->chosenpeer), ICE_GetCandidateType(&best->info), ICE_NetworkToName(con, con->chosenpeer.connum));
								}
							}
						}
						/*if (con->state == ICE_CONNECTED && best)
						{	//once established, back off somewhat
							for (rc = con->rc; rc; rc = rc->next)
								rc->tried &= ~rc->reachable;
						}
						else*/
						{
							for (rc = con->rc; rc; rc = rc->next)
								rc->tried = 0;
						}

						con->retries++;
						if (con->retries > 32)
							con->retries = 32;
						con->keepalive = curtime + 200*(con->retries);	//RTO... ish.
					}
					else
						con->keepalive = curtime + 50*(con->retries+1);	//Ta... absmin of 5ms
				}
			}
			if (con->state == ICE_CONNECTED)
			{
#ifdef HAVE_DTLS
				if (con->sctp)
					SCTP_Transmit(con->sctp, NULL,0);	//try to keep it ticking...
				if (con->dtlsstate)
					con->dtlsfuncs->Timeouts(con->dtlsstate);
#endif

				//FIXME: We should be sending a stun binding indication every 15 secs with a fingerprint attribute
			}
			break;
		}

		link = &con->next;
	}
}
static void QDECL ICE_Close(struct icestate_s *con, qboolean force)
{
	struct icestate_s **link;

	for (link = &icelist; *link; )
	{
		if (con == *link)
		{
			if (!force)
				con->brokerless = true;
			else
			{
				*link = con->next;
				ICE_Destroy(con);
			}
			return;
		}
		else
			link = &(*link)->next;
	}
}
static void QDECL ICE_CloseModule(void *module)
{
	struct icestate_s **link, *con;

	for (link = &icelist; *link; )
	{
		con = *link;
		if (con->module == module)
		{
			*link = con->next;
			ICE_Destroy(con);
		}
		else
			link = &(*link)->next;
	}
}

icefuncs_t iceapi =
{
	ICE_Create,
	ICE_Set,
	ICE_Get,
	ICE_GetLCandidateInfo,
	ICE_AddRCandidateInfo,
	ICE_Close,
	ICE_CloseModule,
	ICE_GetLCandidateSDP,
	ICE_Find
};
#endif



#if defined(SUPPORT_ICE) && defined(HAVE_DTLS)
//========================================
//WebRTC's interpretation of SCTP. its annoying, but hey its only 28 wasted bytes... along with the dtls overhead too. most of this is redundant.
//we only send unreliably.
//there's no point in this code without full webrtc code.

struct sctp_header_s
{
	quint16_t srcport;
	quint16_t dstport;
	quint32_t verifycode;
	quint32_t crc;
};
struct sctp_chunk_s
{
	qbyte type;
#define SCTP_TYPE_DATA 0
#define SCTP_TYPE_INIT 1
#define SCTP_TYPE_INITACK 2
#define SCTP_TYPE_SACK 3
#define SCTP_TYPE_PING 4
#define SCTP_TYPE_PONG 5
#define SCTP_TYPE_ABORT 6
#define SCTP_TYPE_SHUTDOWN 7
#define SCTP_TYPE_SHUTDOWNACK 8
#define SCTP_TYPE_ERROR 9
#define SCTP_TYPE_COOKIEECHO 10
#define SCTP_TYPE_COOKIEACK 11
#define SCTP_TYPE_SHUTDOWNDONE 14
#define SCTP_TYPE_PAD 132
#define SCTP_TYPE_FORWARDTSN 192
	qbyte flags;
	quint16_t length;
	//value...
};
struct sctp_chunk_data_s
{
	struct sctp_chunk_s chunk;
	quint32_t tsn;
	quint16_t sid;
	quint16_t seq;
	quint32_t ppid;
#define SCTP_PPID_DCEP 50 //datachannel establishment protocol
#define SCTP_PPID_DATA 53 //our binary quake data.
};
struct sctp_chunk_init_s
{
	struct sctp_chunk_s chunk;
	quint32_t verifycode;
	quint32_t arwc;
	quint16_t numoutstreams;
	quint16_t numinstreams;
	quint32_t tsn;
};
struct sctp_chunk_sack_s
{
	struct sctp_chunk_s chunk;
	quint32_t tsn;
	quint32_t arwc;
	quint16_t gaps;
	quint16_t dupes;
	/*struct {
		quint16_t first;
		quint16_t last;
	} gapblocks[];	//actually received rather than gaps, but same meaning.
	quint32_t dupe_tsns[];*/
};
struct sctp_chunk_fwdtsn_s
{
	struct sctp_chunk_s chunk;
	quint32_t tsn;
	/*struct
	{
		quint16_t sid;
		quint16_t seq;
	} streams[];*/
};

static neterr_t SCTP_PeerSendPacket(sctp_t *sctp, int length, const void *data)
{	//sends to the dtls layer (which will send to the generic ice dispatcher that'll send to the dgram stuff... layers on layers.
	struct icestate_s *peer = sctp->icestate;
	if (sctp->dtlsstate)
		return sctp->dtlsfuncs->Transmit(sctp->dtlsstate, data, length);
	else if (peer)
	{
		if (peer->dtlsstate)
			return peer->dtlsfuncs->Transmit(peer->dtlsstate, data, length);
		else if (peer->chosenpeer.type != NA_INVALID)
			return ICE_Transmit(peer, data, length);
		else if (peer->state < ICE_CONNECTING)
			return NETERR_DISCONNECTED;
		else
			return NETERR_CLOGGED;
	}
	else
		return NETERR_NOROUTE;
}

static quint32_t SCTP_Checksum(const struct sctp_header_s *h, size_t size)
{
    int k;
    const qbyte *buf = (const qbyte*)h;
    size_t ofs;
    uint32_t crc = 0xFFFFFFFF;

	for (ofs = 0; ofs < size; ofs++)
    {
		if (ofs >= 8 && ofs < 8+4)
			;	//the header's crc should be read as 0.
		else
			crc ^= buf[ofs];
        for (k = 0; k < 8; k++)	            //CRC-32C polynomial 0x1EDC6F41 in reversed bit order.
            crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1;
    }
    return ~crc;
}

neterr_t SCTP_Transmit(sctp_t *sctp, const void *data, size_t length)
{
	qbyte pkt[65536];
	size_t pktlen = 0;
	struct sctp_header_s *h = (void*)pkt;
	struct sctp_chunk_data_s *d;
	struct sctp_chunk_fwdtsn_s *fwd;
	if (length > sizeof(pkt))
		return NETERR_MTU;

	h->dstport = sctp->peerport;
	h->srcport = sctp->myport;
	h->verifycode = sctp->o.verifycode;
	pktlen += sizeof(*h);

	//advance our ctsn if we're received the relevant packets
	while(sctp->i.htsn)
	{
		quint32_t tsn = sctp->i.ctsn+1;
		if (!(sctp->i.received[(tsn>>3)%sizeof(sctp->i.received)] & 1<<(tsn&7)))
			break;
		//advance our cumulative ack.
		sctp->i.received[(tsn>>3)%sizeof(sctp->i.received)] &= ~(1<<(tsn&7));
		sctp->i.ctsn = tsn;
		sctp->i.htsn--;
	}

	if (!sctp->o.writable)
	{
		double time = Sys_DoubleTime();
		if (time > sctp->nextreinit)
		{
			sctp->nextreinit = time + 0.5;
			if (!sctp->cookie)
			{
				struct sctp_chunk_init_s *init = (struct sctp_chunk_init_s*)&pkt[pktlen];
				struct {
					quint16_t ptype;
					quint16_t plen;
				} *ftsn = (void*)(init+1);
				h->verifycode = 0;
				init->chunk.type = SCTP_TYPE_INIT;
				init->chunk.flags = 0;
				init->chunk.length = BigShort(sizeof(*init)+sizeof(*ftsn));
				init->verifycode = sctp->i.verifycode;
				init->arwc = BigLong(65535);
				init->numoutstreams = BigShort(2);
				init->numinstreams = BigShort(2);
				init->tsn = BigLong(sctp->o.tsn);
				ftsn->ptype = BigShort(49152);
				ftsn->plen = BigShort(sizeof(*ftsn));
				pktlen += sizeof(*init) + sizeof(*ftsn);

				h->crc = SCTP_Checksum(h, pktlen);
				return SCTP_PeerSendPacket(sctp, pktlen, h);
			}
			else
			{
				struct sctp_chunk_s *cookie = (struct sctp_chunk_s*)&pkt[pktlen];

				if (pktlen + sizeof(*cookie) + sctp->cookiesize > sizeof(pkt))
					return NETERR_DISCONNECTED;
				cookie->type = SCTP_TYPE_COOKIEECHO;
				cookie->flags = 0;
				cookie->length = BigShort(sizeof(*cookie)+sctp->cookiesize);
				memcpy(cookie+1, sctp->cookie, sctp->cookiesize);
				pktlen += sizeof(*cookie) + sctp->cookiesize;

				h->crc = SCTP_Checksum(h, pktlen);
				return SCTP_PeerSendPacket(sctp, pktlen, h);
			}
		}

		return NETERR_CLOGGED;	//nope, not ready yet
	}

	if (sctp->peerhasfwdtsn && (int)(sctp->o.ctsn-sctp->o.tsn) < -5 && sctp->o.losttsn)
	{	
		fwd = (struct sctp_chunk_fwdtsn_s*)&pkt[pktlen];
		fwd->chunk.type = SCTP_TYPE_FORWARDTSN;
		fwd->chunk.flags = 0;
		fwd->chunk.length = BigShort(sizeof(*fwd));
		fwd->tsn = BigLong(sctp->o.tsn-1);

		//we only send unordered unreliables, so this stream stuff here is irrelevant.
//		fwd->streams[0].sid = sctp->qstreamid;
//		fwd->streams[0].seq = BigShort(0);
		pktlen += sizeof(*fwd);
	}

	if (sctp->i.ackneeded >= 2)
	{
		struct sctp_chunk_sack_s *rsack;
		struct sctp_chunk_sack_gap_s {
			uint16_t first;
			uint16_t last;
		} *rgap;
		quint32_t otsn;

		rsack = (struct sctp_chunk_sack_s*)&pkt[pktlen];
		rsack->chunk.type = SCTP_TYPE_SACK;
		rsack->chunk.flags = 0;
		rsack->chunk.length = BigShort(sizeof(*rsack));
		rsack->tsn = BigLong(sctp->i.ctsn);
		rsack->arwc = BigLong(65535);
		rsack->gaps = 0;
		rsack->dupes = BigShort(0);
		pktlen += sizeof(*rsack);

		rgap = (struct sctp_chunk_sack_gap_s*)&pkt[pktlen];
		for (otsn = 0; otsn < sctp->i.htsn; otsn++)
		{
			quint32_t tsn = sctp->i.ctsn+otsn;
			if (!(sctp->i.received[(tsn>>3)%sizeof(sctp->i.received)] & 1<<(tsn&7)))
				continue;	//missing, don't report it in the 'gaps'... yeah, backwards naming.
			if (rsack->gaps && rgap[-1].last == otsn-1)
				rgap[-1].last = otsn;	//merge into the last one.
			else
			{
				rgap->first = otsn;	//these values are Offset from the Cumulative TSN, to save storage.
				rgap->last = otsn;
				rgap++;
				rsack->gaps++;
				pktlen += sizeof(*rgap);
				if (pktlen >= 500)
					break;	//might need fragmentation... just stop here.
			}
		}
		for (otsn = 0, rgap = (struct sctp_chunk_sack_gap_s*)&pkt[pktlen]; otsn < rsack->gaps; otsn++)
		{
			rgap[otsn].first = BigShort(rgap[otsn].first);
			rgap[otsn].last = BigShort(rgap[otsn].last);
		}
		rsack->gaps = BigShort(rsack->gaps);

		sctp->i.ackneeded = 0;
	}

	if (pktlen + sizeof(*d) + length >= 500 && length && pktlen != sizeof(*h))
	{	//probably going to result in fragmentation issues. send separate packets.
		h->crc = SCTP_Checksum(h, pktlen);
		SCTP_PeerSendPacket(sctp, pktlen, h);

		//reset to the header
		pktlen = sizeof(*h);
	}

	if (length)
	{
		d = (void*)&pkt[pktlen];
		d->chunk.type = SCTP_TYPE_DATA;
		d->chunk.flags = 3|4;
		d->chunk.length = BigShort(sizeof(*d) + length);
		d->tsn = BigLong(sctp->o.tsn++);
		d->sid = sctp->qstreamid;
		d->seq = BigShort(0); //not needed for unordered
		d->ppid = BigLong(SCTP_PPID_DATA);
		memcpy(d+1, data, length);
		pktlen += sizeof(*d) + length;

		//chrome insists on pointless padding at the end. its annoying.
		while(pktlen&3)
			pkt[pktlen++]=0;
	}
	if (pktlen == sizeof(*h))
		return NETERR_SENT; //nothing to send...

	h->crc = SCTP_Checksum(h, pktlen);
	return SCTP_PeerSendPacket(sctp, pktlen, h);
}

static void SCTP_DecodeDCEP(sctp_t *sctp, qbyte *resp)
{	//send an ack...
	size_t pktlen = 0;
	struct sctp_header_s *h = (void*)resp;
	struct sctp_chunk_data_s *d;
	char *data = "\02";
	size_t length = 1; //*sigh*...

	struct
	{
		qbyte type;
		qbyte chantype;
		quint16_t priority;
		quint32_t relparam;
		quint16_t labellen;
		quint16_t protocollen;
	} *dcep = (void*)sctp->i.r.buf;

	if (dcep->type == 3)
	{
		char *label = (qbyte*)(dcep+1);
		char *prot = label + strlen(label)+1;

		sctp->qstreamid = sctp->i.r.sid;
		if (net_ice_debug.ival >= 1)
			Con_Printf(S_COLOR_GRAY"[%s]: New SCTP Channel: \"%s\" (%s)\n", sctp->friendlyname, label, prot);

		h->dstport = sctp->peerport;
		h->srcport = sctp->myport;
		h->verifycode = sctp->o.verifycode;
		pktlen += sizeof(*h);

		pktlen = (pktlen+3)&~3;	//pad.
		d = (void*)&resp[pktlen];
		d->chunk.type = SCTP_TYPE_DATA;
		d->chunk.flags = 3;
		d->chunk.length = BigShort(sizeof(*d) + length);
		d->tsn = BigLong(sctp->o.tsn++);
		d->sid = sctp->qstreamid;
		d->seq = BigShort(0); //not needed for unordered
		d->ppid = BigLong(SCTP_PPID_DCEP);
		memcpy(d+1, data, length);
		pktlen += sizeof(*d) + length;

		h->crc = SCTP_Checksum(h, pktlen);
		SCTP_PeerSendPacket(sctp, pktlen, h);
	}
}

struct sctp_errorcause_s
{
	quint16_t cause;
	quint16_t length;
};
static void SCTP_ErrorChunk(sctp_t *sctp, const char *errortype, struct sctp_errorcause_s *s, size_t totallen)
{
	quint16_t cc, cl;
	while(totallen > 0)
	{
		if (totallen < sizeof(*s))
			return;	//that's an error in its own right
		cc = BigShort(s->cause);
		cl = BigShort(s->length);
		if (totallen < cl)
			return;	//err..

		if (net_ice_debug.ival >= 1) switch(cc)
		{
		case 1:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Invalid Stream Identifier\n",	sctp->friendlyname, errortype);	break;
        case 2:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Missing Mandatory Parameter\n",	sctp->friendlyname, errortype);	break;
        case 3:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Stale Cookie Error\n",			sctp->friendlyname, errortype);	break;
        case 4:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Out of Resource\n",				sctp->friendlyname, errortype);	break;
        case 5:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Unresolvable Address\n",			sctp->friendlyname, errortype);	break;
        case 6:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Unrecognized Chunk Type\n",		sctp->friendlyname, errortype);	break;
        case 7:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Invalid Mandatory Parameter\n",	sctp->friendlyname, errortype);	break;
        case 8:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Unrecognized Parameters\n",		sctp->friendlyname, errortype);	break;
        case 9:		Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: No User Data\n",					sctp->friendlyname, errortype);	break;
        case 10:	Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Cookie Received While Shutting Down\n",			sctp->friendlyname, errortype);	break;
        case 11:	Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Restart of an Association with New Addresses\n",	sctp->friendlyname, errortype);	break;
        case 12:	Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: User Initiated Abort\n",			sctp->friendlyname, errortype);	break;
        case 13:	Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Protocol Violation [%s]\n",		sctp->friendlyname, errortype, (char*)(s+1));	break;
        default:	Con_Printf(S_COLOR_GRAY"[%s]: SCTP %s: Unknown Reason\n",				sctp->friendlyname, errortype);	break;
		}

		totallen -= cl;
		totallen &= ~3;
		s = (struct sctp_errorcause_s*)((qbyte*)s + ((cl+3)&~3));
	}
}

void SCTP_Decode(sctp_t *sctp, ftenet_connections_t *col)
{
	qbyte resp[4096];

	qbyte *msg = net_message.data;
	qbyte *msgend = msg+net_message.cursize;
	struct sctp_header_s *h = (struct sctp_header_s*)msg;
	struct sctp_chunk_s *c = (struct sctp_chunk_s*)(h+1);
	quint16_t clen;
	if ((qbyte*)c+1 > msgend)
		return;	//runt
	if (h->dstport != sctp->myport)
		return;	//not for us...
	if (h->srcport != sctp->peerport)
		return; //not from them... we could check for a INIT but its over dtls anyway so why give a damn.
	if (h->verifycode != ((c->type == SCTP_TYPE_INIT)?0:sctp->i.verifycode))
		return;	//wrong cookie... (be prepared to parse dupe inits if our ack got lost...
	if (h->crc != SCTP_Checksum(h, net_message.cursize))
		return;	//crc wrong. assume corruption.
	if (net_message.cursize&3)
	{
		if (net_ice_debug.ival >= 2)
			Con_Printf(S_COLOR_GRAY"[%s]: SCTP: packet not padded\n", sctp->friendlyname);
		return;	//mimic chrome, despite it being pointless.
	}

	//passed the simple header checks, spend a memcpy...
	msg = alloca(net_message.cursize);
	memcpy(msg, net_message.data, net_message.cursize);
	msgend = msg+net_message.cursize;
	h = (struct sctp_header_s*)msg;
	c = (struct sctp_chunk_s*)(h+1);

	while ((qbyte*)(c+1) <= msgend)
	{
		clen = BigShort(c->length);
		if ((qbyte*)c + clen > msgend || clen < sizeof(*c))
		{
			Con_Printf(CON_ERROR"Corrupt SCTP message\n");
			break;
		}
		safeswitch(c->type)
		{
		case SCTP_TYPE_DATA:
			if (clen >= sizeof(struct sctp_chunk_data_s))
			{
				struct sctp_chunk_data_s *dc = (void*)c;
				quint32_t tsn = BigLong(dc->tsn), u;
				qint32_t adv = tsn - sctp->i.ctsn;
				sctp->i.ackneeded++;
				if (adv >= SCTP_RCVSIZE)
				{
					if (net_ice_debug.ival >= 1)
						Con_Printf(S_COLOR_GRAY"[%s]: SCTP: Future Packet\n", sctp->friendlyname);/*too far in the future. we can't track such things*/
				}
				else if (adv <= 0)
				{
					if (net_ice_debug.ival >= 2)
						Con_Printf(S_COLOR_GRAY"[%s]: SCTP: PreCumulative\n", sctp->friendlyname);/*already acked this*/
				}
				else if (sctp->i.received[(tsn>>3)%sizeof(sctp->i.received)] & 1<<(tsn&7))
				{
					if (net_ice_debug.ival >= 2)
						Con_DPrintf(S_COLOR_GRAY"[%s]: SCTP: Dupe\n", sctp->friendlyname);/*already processed it. FIXME: Make a list for the next SACK*/
				}
				else
				{
					qboolean err = false;

					if (c->flags & 2)
					{	//beginning...
						sctp->i.r.firsttns = tsn;
						sctp->i.r.tsn = tsn;
						sctp->i.r.size = 0;
						sctp->i.r.ppid = dc->ppid;
						sctp->i.r.sid = dc->sid;
						sctp->i.r.seq = dc->seq;
						sctp->i.r.toobig = false;
					}
					else
					{
						if (sctp->i.r.tsn != tsn || sctp->i.r.ppid != dc->ppid)
							err = true;
					}
					if (err)
						;	//don't corrupt anything in case we get a quick resend that fixes it.
					else
					{
						size_t dlen = clen-sizeof(*dc);
						if (adv > sctp->i.htsn)	//weird maths in case it wraps.
							sctp->i.htsn = adv;
						sctp->i.r.tsn++;
						if (sctp->i.r.size + dlen > sizeof(sctp->i.r.buf))
						{
							if (net_ice_debug.ival >= 2)
								Con_Printf(S_COLOR_GRAY"[%s]: SCTP: Oversized\n", sctp->friendlyname);
							sctp->i.r.toobig = true;	//reassembled packet was too large, just corrupt it.
						}
						else
						{
							memcpy(sctp->i.r.buf+sctp->i.r.size, dc+1, dlen);	//include the dc header
							sctp->i.r.size += dlen;
						}
						if (c->flags & 1)	//an ending. we have the complete packet now.
						{
							for (u = sctp->i.r.tsn - sctp->i.r.firsttns; u --> 0; )
							{
								tsn = sctp->i.r.firsttns + u;
								sctp->i.received[(tsn>>3)%sizeof(sctp->i.received)] |= 1<<(tsn&7);
							}
							if (sctp->i.r.toobig)
								;/*ignore it when it cannot be handled*/
							else if (sctp->i.r.ppid == BigLong(SCTP_PPID_DATA))
							{
								memmove(net_message.data, sctp->i.r.buf, sctp->i.r.size);
								net_message.cursize = sctp->i.r.size;
								col->ReadGamePacket();
								if (net_message.cursize != sctp->i.r.size)
								{
									net_message.cursize = 0;
									return;	//something weird happened...
								}
							}
							else if (sctp->i.r.ppid == BigLong(SCTP_PPID_DCEP))
								SCTP_DecodeDCEP(sctp, resp);
						}
					}

					//FIXME: we don't handle reordering properly at all.

//					if (c->flags & 4)
//						Con_Printf("\tUnordered\n");
//					Con_Printf("\tStream Id %i\n", BigShort(dc->sid));
//					Con_Printf("\tStream Seq %i\n", BigShort(dc->seq));
//					Con_Printf("\tPPID %i\n", BigLong(dc->ppid));
				}
			}
			break;
		case SCTP_TYPE_INIT:
		case SCTP_TYPE_INITACK:
			if (clen >= sizeof(struct sctp_chunk_init_s))
			{
				qboolean isack = c->type==SCTP_TYPE_INITACK;
				struct sctp_chunk_init_s *init = (void*)c;
				struct {
						quint16_t ptype;
						quint16_t plen;
				} *p = (void*)(init+1);

				sctp->i.ctsn = BigLong(init->tsn)-1;
				sctp->i.htsn = 0;
				sctp->o.verifycode = init->verifycode;
				(void)BigLong(init->arwc);
				(void)BigShort(init->numoutstreams);
				(void)BigShort(init->numinstreams);

				while ((qbyte*)p+sizeof(*p) <= (qbyte*)c+clen)
				{
					unsigned short ptype = BigShort(p->ptype);
					unsigned short plen = BigShort(p->plen);
					switch(ptype)
					{
					case 7:	//init cookie
						if (sctp->cookie)
							Z_Free(sctp->cookie);
						sctp->cookiesize = plen - sizeof(*p);
						sctp->cookie = Z_Malloc(sctp->cookiesize);
						memcpy(sctp->cookie, p+1, sctp->cookiesize);
						break;
					case 32773:	//Padding
					case 32776:	//ASCONF
						break;
					case 49152:
						sctp->peerhasfwdtsn = true;
						break;
					default:
						if (net_ice_debug.ival >= 2)
							Con_Printf(S_COLOR_GRAY"[%s]: SCTP: Found unknown init parameter %i||%#x\n", sctp->friendlyname, ptype, ptype);

						if (ptype&0x4000)
							; //FIXME: SCTP_TYPE_ERROR(6,"Unrecognized Chunk Type")
						if (!(ptype&0x8000))
							return;	//'do not process nay further chunks'
						//otherwise parse the next as normal.
						break;
					}
					p = (void*)((qbyte*)p + ((plen+3)&~3));
				}

				if (isack)
				{
					sctp->nextreinit = 0;
					if (sctp->cookie)
						SCTP_Transmit(sctp, NULL, 0);	//make sure we send acks occasionally even if we have nothing else to say.
				}
				else
				{
					struct sctp_header_s *rh = (void*)resp;
					struct sctp_chunk_init_s *rinit = (void*)(rh+1);
					struct {
						quint16_t ptype;
						quint16_t plen;
						struct {
							qbyte data[16];
						} cookie;
					} *rinitcookie = (void*)(rinit+1);
					struct {
						quint16_t ptype;
						quint16_t plen;
					} *rftsn = (void*)(rinitcookie+1);
					qbyte *end = sctp->peerhasfwdtsn?(void*)(rftsn+1):(void*)(rinitcookie+1);

					rh->srcport = sctp->myport;
					rh->dstport = sctp->peerport;
					rh->verifycode = sctp->o.verifycode;
					rh->crc = 0;
					*rinit = *init;
					rinit->chunk.type = SCTP_TYPE_INITACK;
					rinit->chunk.flags = 0;
					rinit->chunk.length = BigShort(end-(qbyte*)rinit);
					rinit->verifycode = sctp->i.verifycode;
					rinit->arwc = BigLong(65536);
					rinit->numoutstreams = init->numoutstreams;
					rinit->numinstreams = init->numinstreams;
					rinit->tsn = BigLong(sctp->o.tsn);
					rinitcookie->ptype = BigShort(7);
					rinitcookie->plen = BigShort(sizeof(*rinitcookie));
					memcpy(&rinitcookie->cookie, "deadbeefdeadbeef", sizeof(rinitcookie->cookie));	//frankly the contents of the cookie are irrelevant to anything. we've already verified the peer's ice pwd/ufrag stuff as well as their dtls certs etc.
					rftsn->ptype = BigShort(49152);
					rftsn->plen = BigShort(sizeof(*rftsn));

					//complete. calc the proper crc and send it off.
					rh->crc = SCTP_Checksum(rh, end-resp);
					SCTP_PeerSendPacket(sctp, end-resp, rh);
				}
			}
			break;
		case SCTP_TYPE_SACK:
			if (clen >= sizeof(struct sctp_chunk_sack_s))
			{
				struct sctp_chunk_sack_s *sack = (void*)c;
				quint32_t tsn = BigLong(sack->tsn);
				sctp->o.ctsn = tsn;

				sctp->o.losttsn = BigShort(sack->gaps);	//if there's a gap then they're telling us they got a later one.

				//Con_Printf(CON_ERROR"Sack %#x (%i in flight)\n"
				//			"\tgaps: %i, dupes %i\n",
				//			tsn, sctp->o.tsn-tsn,
				//			BigShort(sack->gaps), BigShort(sack->dupes));
			}
			break;
		case SCTP_TYPE_PING:
			if (clen >= sizeof(struct sctp_chunk_s))
			{
				struct sctp_chunk_s *ping = (void*)c;
				struct sctp_header_s *pongh = Z_Malloc(sizeof(*pongh) + clen);

				pongh->srcport = sctp->myport;
				pongh->dstport = sctp->peerport;
				pongh->verifycode = sctp->o.verifycode;
				pongh->crc = 0;
				memcpy(pongh+1, ping, clen);
				((struct sctp_chunk_s*)(pongh+1))->type = SCTP_TYPE_PONG;

				//complete. calc the proper crc and send it off.
				pongh->crc = SCTP_Checksum(pongh, sizeof(*pongh) + clen);
				SCTP_PeerSendPacket(sctp, sizeof(*pongh) + clen, pongh);
				Z_Free(pongh);
			}
			break;
//		case SCTP_TYPE_PONG:	//we don't send pings
		case SCTP_TYPE_ABORT:
			SCTP_ErrorChunk(sctp, "Abort", (struct sctp_errorcause_s*)(c+1), clen-sizeof(*c));
			if (sctp->icestate)
				ICE_SetFailed(sctp->icestate, "SCTP Abort");
			break;
		case SCTP_TYPE_SHUTDOWN:	//FIXME. we should send an ack...
			if (sctp->icestate)
				ICE_SetFailed(sctp->icestate, "SCTP Shutdown");
			if (net_ice_debug.ival >= 1)
				Con_Printf(S_COLOR_GRAY"[%s]: SCTP: Shutdown\n", sctp->friendlyname);
			break;
//		case SCTP_TYPE_SHUTDOWNACK:	//we don't send shutdowns, cos we're lame like that...
		case SCTP_TYPE_ERROR:
			//not fatal...
			SCTP_ErrorChunk(sctp, "Error", (struct sctp_errorcause_s*)(c+1), clen-sizeof(*c));
			break;
		case SCTP_TYPE_COOKIEECHO:
			if (clen >= sizeof(struct sctp_chunk_s))
			{
				struct sctp_header_s *rh = (void*)resp;
				struct sctp_chunk_s *rack = (void*)(rh+1);
				qbyte *end = (void*)(rack+1);

				rh->srcport = sctp->myport;
				rh->dstport = sctp->peerport;
				rh->verifycode = sctp->o.verifycode;
				rh->crc = 0;
				rack->type = SCTP_TYPE_COOKIEACK;
				rack->flags = 0;
				rack->length = BigShort(sizeof(*rack));

				//complete. calc the proper crc and send it off.
				rh->crc = SCTP_Checksum(rh, end-resp);
				SCTP_PeerSendPacket(sctp, end-resp, rh);

				sctp->o.writable = true;	//channel SHOULD now be open for data.
			}
			break;
		case SCTP_TYPE_COOKIEACK:
			sctp->o.writable = true;	//we know the other end is now open.
			break;
		case SCTP_TYPE_PAD:
			//don't care.
			break;
		case SCTP_TYPE_FORWARDTSN:
			if (clen >= sizeof(struct sctp_chunk_fwdtsn_s))
			{
				struct sctp_chunk_fwdtsn_s *fwd = (void*)c;
				quint32_t tsn = BigLong(fwd->tsn), count;
				count = tsn - sctp->i.ctsn;
				if ((int)count < 0)
					break;	//overflow? don't go backwards.
				if (count > 1024)
					count = 1024; //don't advance too much in one go. we'd block and its probably an error anyway.
				while(count --> 0)
				{
					tsn = ++sctp->i.ctsn;
					sctp->i.received[(tsn>>3)%sizeof(sctp->i.received)] &= ~(1<<(tsn&7));
					if (sctp->i.htsn)
						sctp->i.htsn--;
					sctp->i.ackneeded++;	//flag for a sack if we actually completed something here.
				}
			}
			break;
//		case SCTP_TYPE_SHUTDOWNDONE:
		safedefault:
			//no idea what this chunk is, just ignore it...
			if (net_ice_debug.ival >= 1)
				Con_Printf(S_COLOR_GRAY"[%s]: SCTP: Unsupported chunk %i\n", sctp->friendlyname, c->type);

			switch (c->type>>6)
			{
			case 0:
				clen = (qbyte*)msgend - (qbyte*)c;	//'do not process any further chunks'
				break;
			case 1:
				clen = (qbyte*)msgend - (qbyte*)c;	//'do not process any further chunks'
				/*FIXME: SCTP_TYPE_ERROR(6,"Unrecognized Chunk Type")*/
				break;
			case 2:
				//silently ignore it
				break;
			case 3:
				//ignore-with-error
				/*FIXME: SCTP_TYPE_ERROR(6,"Unrecognized Chunk Type")*/
				break;
			}
			break;
		}
		c = (struct sctp_chunk_s*)((qbyte*)c + ((clen+3)&~3));	//next chunk is 4-byte aligned.
	}

	if (sctp->i.ackneeded >= 5)
		SCTP_Transmit(sctp, NULL, 0);	//make sure we send acks occasionally even if we have nothing else to say.

	//we already made sense of it all.
	net_message.cursize = 0;
}

#if 0
qboolean SCTP_Handshake(const dtlsfuncs_t *dtlsfuncs, void *dtlsstate, sctp_t **out)
{
	const int myport = ICELITE_SCTP_PORT;
	struct cookiedata_s
	{
		qbyte peerhasfwdtsn;
		int overifycode;
		int iverifycode;
		int ictsn;
		int otsn;
//		int checkcode;
	};
	//if this is an sctp init packet, send a cookie.
	//if its an initack then create the new state.
	qbyte resp[4096];

	qbyte *msg = net_message.data;
	qbyte *msgend = msg+net_message.cursize;
	struct sctp_header_s *h = (struct sctp_header_s*)msg;
	struct sctp_chunk_s *c = (struct sctp_chunk_s*)(h+1);
	quint16_t clen;
	if (net_message.cursize&3)
		return false;
	if ((qbyte*)c+1 > msgend)
		return false;	//runt
	if (h->dstport != htons(myport))
		return false;	//not for us...
	clen = BigShort(c->length);
	if ((qbyte*)c + ((clen+3)&~3) == msgend)	//don't allow multiple chucks
	switch(c->type)
	{
	default:
		return false;	//not the right kind of packet.
	case SCTP_TYPE_COOKIEECHO:
		if (clen == sizeof(struct sctp_chunk_s)+sizeof(struct cookiedata_s))
		{
			struct cookiedata_s *cookie = (struct cookiedata_s *)(c+1);
			sctp_t *sctp;
			struct sctp_header_s *rh = (void*)resp;
			struct sctp_chunk_s *rack = (void*)(rh+1);
			qbyte *end = (void*)(rack+1);
			if (h->verifycode == cookie->iverifycode)
			if (h->crc == SCTP_Checksum(h, net_message.cursize))	//make sure the crc matches.
			{	//looks okay.
				NET_AdrToString(resp, sizeof(resp), &net_from);

				*out = sctp = Z_Malloc(sizeof(*sctp) + strlen(resp));
				sctp->friendlyname = strcpy((char*)(sctp+1), resp);
				sctp->icestate = NULL;
				sctp->dtlsfuncs = dtlsfuncs;
				sctp->dtlsstate = dtlsstate;

				sctp->myport = h->dstport;
				sctp->peerport = h->srcport;
				sctp->o.tsn = cookie->otsn;
				sctp->i.ctsn = cookie->ictsn;
				sctp->o.verifycode = cookie->overifycode;
				sctp->i.verifycode = cookie->iverifycode;
				sctp->peerhasfwdtsn = cookie->peerhasfwdtsn;
				sctp->o.writable = true;	//channel SHOULD now be open for data (once it gets the ack anyway).

				//let our peer know too.
				rh->srcport = sctp->myport;
				rh->dstport = sctp->peerport;
				rh->verifycode = sctp->o.verifycode;
				rh->crc = 0;
				rack->type = SCTP_TYPE_COOKIEACK;
				rack->flags = 0;
				rack->length = BigShort(sizeof(*rack));

				//complete. calc the proper crc and send it off.
				rh->crc = SCTP_Checksum(rh, end-resp);
				SCTP_PeerSendPacket(sctp, end-resp, rh);
				return true;
			}
		}
		break;
	case SCTP_TYPE_INIT:
		if (h->verifycode == 0)	//this must be 0 for inits.
		if (clen >= sizeof(struct sctp_chunk_init_s))
		if (h->crc == SCTP_Checksum(h, net_message.cursize))	//make sure the crc matches.
		{
			struct sctp_chunk_init_s *init = (void*)c;
			struct {
					quint16_t ptype;
					quint16_t plen;
			} *p = (void*)(init+1);

			struct cookiedata_s cookie = {0};

			cookie.otsn = rand() ^ (rand()<<16);
			Sys_RandomBytes((void*)&cookie.iverifycode, sizeof(cookie.iverifycode));
			cookie.ictsn = BigLong(init->tsn)-1;
			cookie.overifycode = init->verifycode;
			(void)BigLong(init->arwc);
			(void)BigShort(init->numoutstreams);
			(void)BigShort(init->numinstreams);

			while ((qbyte*)p+sizeof(*p) <= (qbyte*)c+clen)
			{
				unsigned short ptype = BigShort(p->ptype);
				unsigned short plen = BigShort(p->plen);
				switch(ptype)
				{
				case 32776:	//ASCONF
					break;
				case 49152:
					cookie.peerhasfwdtsn = true;
					break;
				default:
					if (net_ice_debug.ival >= 2)
						Con_Printf(S_COLOR_GRAY"[%s]: SCTP: Found unknown init parameter %i||%#x\n", NET_AdrToString(resp,sizeof(resp), &net_from), ptype, ptype);
					break;
				}
				p = (void*)((qbyte*)p + ((plen+3)&~3));
			}

			{
				struct sctp_header_s *rh = (void*)resp;
				struct sctp_chunk_init_s *rinit = (void*)(rh+1);
				struct {
					quint16_t ptype;
					quint16_t plen;
					struct cookiedata_s cookie;
				} *rinitcookie = (void*)(rinit+1);
				struct {
					quint16_t ptype;
					quint16_t plen;
				} *rftsn = (void*)(rinitcookie+1);
				qbyte *end = cookie.peerhasfwdtsn?(void*)(rftsn+1):(void*)(rinitcookie+1);

				rh->srcport = h->dstport;
				rh->dstport = h->srcport;
				rh->verifycode = init->verifycode;
				rh->crc = 0;
				*rinit = *init;
				rinit->chunk.type = SCTP_TYPE_INITACK;
				rinit->chunk.flags = 0;
				rinit->chunk.length = BigShort(end-(qbyte*)rinit);
				rinit->verifycode = cookie.iverifycode;
				rinit->arwc = BigLong(65536);
				rinit->numoutstreams = init->numoutstreams;
				rinit->numinstreams = init->numinstreams;
				rinit->tsn = BigLong(cookie.otsn);
				rinitcookie->ptype = BigShort(7);
				rinitcookie->plen = BigShort(sizeof(*rinitcookie));
				memcpy(&rinitcookie->cookie, &cookie, sizeof(rinitcookie->cookie));	//frankly the contents of the cookie are irrelevant to anything. we've already verified the peer's ice pwd/ufrag stuff as well as their dtls certs etc.
				rftsn->ptype = BigShort(49152);
				rftsn->plen = BigShort(sizeof(*rftsn));

				//complete. calc the proper crc and send it off.
				rh->crc = SCTP_Checksum(rh, end-resp);
				dtlsfuncs->Transmit(dtlsstate, resp, end-resp);
			}
			return true;
		}
		break;
	}

	return false;
}
#endif

//========================================
#endif

#if defined(SUPPORT_ICE) || (defined(MASTERONLY) && defined(AVAIL_ZLIB))
qboolean ICE_WasStun(ftenet_connections_t *col)
{
#ifdef SUPPORT_ICE
	if (net_from.type == NA_ICE)
		return false;	//this stuff over an ICE connection doesn't make sense.
#endif
//	if (net_from.prot != NP_DGRAM)
//		return false;

#if defined(HAVE_CLIENT) && defined(VOICECHAT)
	if (col == cls.sockets)
	{
		if (NET_RTP_Parse())
			return true;
	}
#endif

	if ((net_from.type == NA_IP || net_from.type == NA_IPV6) && net_message.cursize >= 20 && *net_message.data<2)
	{
		stunhdr_t *stun = (stunhdr_t*)net_message.data;
		int stunlen = BigShort(stun->msglen);
#ifdef SUPPORT_ICE
		if ((stun->msgtype == BigShort(STUN_BINDING|STUN_REPLY) || stun->msgtype == BigShort(STUN_BINDING|STUN_ERROR)) && net_message.cursize == stunlen + sizeof(*stun))
		{
			//binding reply (or error)
			netadr_t adr = net_from;
			char xor[16];
			short portxor;
			stunattr_t *attr = (stunattr_t*)(stun+1);
			int alen;
			unsigned short attrval;
			int err = 0;
			char errmsg[64];
			*errmsg = 0;

			adr.type = NA_INVALID;
			while(stunlen)
			{
				stunlen -= sizeof(*attr);
				alen = (unsigned short)BigShort(attr->attrlen);
				if (alen > stunlen)
					return false;
				stunlen -= (alen+3)&~3;
				attrval = BigShort(attr->attrtype);
				switch(attrval)
				{
				case STUNATTR_USERNAME:
				case STUNATTR_MSGINTEGRITIY_SHA1:
					break;
				default:
					if (attrval & 0x8000)
						break;	//okay to ignore
					return true;
				case STUNATTR_MAPPED_ADDRESS:
					if (adr.type != NA_INVALID)
						break;	//ignore it if we already got an address...
				//fallthrough
				case STUNATTR_XOR_MAPPED_ADDRESS:
					if (attrval == STUNATTR_XOR_MAPPED_ADDRESS)
					{
						portxor = *(short*)&stun->magiccookie;
						memcpy(xor, &stun->magiccookie, sizeof(xor));
					}
					else
					{
						portxor = 0;
						memset(xor, 0, sizeof(xor));
					}
					if (alen == 8 && ((qbyte*)attr)[5] == 1)		//ipv4
					{
						adr.type = NA_IP;
						adr.port = (((short*)attr)[3]) ^ portxor;
						*(int*)adr.address.ip = *(int*)(&((qbyte*)attr)[8]) ^ *(int*)xor;
					}
					else if (alen == 20 && ((qbyte*)attr)[5] == 2)	//ipv6
					{
						adr.type = NA_IPV6;
						adr.port = (((short*)attr)[3]) ^ portxor;
						((int*)adr.address.ip6)[0] = ((int*)&((qbyte*)attr)[8])[0] ^ ((int*)xor)[0];
						((int*)adr.address.ip6)[1] = ((int*)&((qbyte*)attr)[8])[1] ^ ((int*)xor)[1];
						((int*)adr.address.ip6)[2] = ((int*)&((qbyte*)attr)[8])[2] ^ ((int*)xor)[2];
						((int*)adr.address.ip6)[3] = ((int*)&((qbyte*)attr)[8])[3] ^ ((int*)xor)[3];
					}
					break;
				case STUNATTR_ERROR_CODE:
					{
						unsigned short len = BigShort(attr->attrlen)-4;
						if (len > sizeof(errmsg)-1)
							len = sizeof(errmsg)-1;
						memcpy(errmsg, &((qbyte*)attr)[8], len);
						errmsg[len] = 0;
						if (err==0)
							err = (((qbyte*)attr)[6]*100) + (((qbyte*)attr)[7]%100);
					}
					break;
				}
				alen = (alen+3)&~3;
				attr = (stunattr_t*)((char*)(attr+1) + alen);
			}

			if (err)
			{
				char sender[256];
				if (net_ice_debug.ival >= 1)
					Con_Printf("%s: Stun error code %u : %s\n", NET_AdrToString(sender, sizeof(sender), &net_from), err, errmsg);
			}
			else if (adr.type!=NA_INVALID && !err)
			{
				struct icestate_s *con;
				for (con = icelist; con; con = con->next)
				{
					struct icecandidate_s *rc;
					size_t i;
					struct iceserver_s *s;
					if (con->mode == ICEM_RAW)
						continue;

					for (i = 0; i < con->servers; i++)
					{
						s = &con->server[i];
						if (NET_CompareAdr(&net_from, &s->addr) &&	s->stunrnd[0] == stun->transactid[0] &&
																	s->stunrnd[1] == stun->transactid[1] &&
																	s->stunrnd[2] == stun->transactid[2])
						{	//check to see if this is a new server-reflexive address, which happens when the peer is behind a nat.
							for (rc = con->lc; rc; rc = rc->next)
							{
								if (NET_CompareAdr(&adr, &rc->peer))
									break;
							}
							if (!rc)
							{
								//netadr_t reladdr;
								//int relflags;
								//const char *relpath;
								int rnd[2];
								struct icecandidate_s *src;	//server Reflexive Candidate
								char str[256];
								src = Z_Malloc(sizeof(*src));
								src->next = con->lc;
								con->lc = src;
								src->peer = adr;
								NET_BaseAdrToString(src->info.addr, sizeof(src->info.addr), &adr);
								src->info.port = ntohs(adr.port);
								//if (net_from.connum >= 1 && net_from.connum < 1+MAX_CONNECTIONS && col->conn[net_from.connum-1])
								//	col->conn[net_from.connum-1]->GetLocalAddresses(col->conn[net_from.connum-1], &relflags, &reladdr, &relpath, 1);
								//FIXME: we don't really know which one... NET_BaseAdrToString(src->info.reladdr, sizeof(src->info.reladdr), &reladdr);
								//src->info.relport = ntohs(reladdr.port);
								src->info.type = ICE_SRFLX;
								src->info.component = 1;
								src->info.network = net_from.connum;
								src->dirty = true;
								src->info.priority = ICE_ComputePriority(&src->peer, &src->info);	//FIXME

								Sys_RandomBytes((void*)rnd, sizeof(rnd));
								Q_strncpyz(src->info.candidateid, va("x%08x%08x", rnd[0], rnd[1]), sizeof(src->info.candidateid));

								if (net_ice_debug.ival >= 1)
									Con_Printf(S_COLOR_GRAY"[%s]: Public address: %s\n", con->friendlyname, NET_AdrToString(str, sizeof(str), &adr));
							}
							s->stunretry = Sys_Milliseconds() + 60*1000;
							return true;
						}
					}

					//check to see if this is a new peer-reflexive address, which happens when the peer is behind a nat.
					for (rc = con->rc; rc; rc = rc->next)
					{
						if (NET_CompareAdr(&net_from, &rc->peer))
						{
							if (!(rc->reachable & (1u<<(net_from.connum-1))))
							{
								char str[256];
								if (net_ice_debug.ival >= 1)
									Con_Printf(S_COLOR_GRAY"[%s]: We can reach %s (%s) via %s\n", con->friendlyname, NET_AdrToString(str, sizeof(str), &net_from), ICE_GetCandidateType(&rc->info), ICE_NetworkToName(con, net_from.connum));
							}
							rc->reachable |= 1u<<(net_from.connum-1);
							rc->reached = Sys_Milliseconds();

							if (NET_CompareAdr(&net_from, &con->chosenpeer) && (stun->transactid[2] & BigLong(0x80000000)))
							{
								if (con->state == ICE_CONNECTING)
									ICE_Set(con, "state", STRINGIFY(ICE_CONNECTED));
							}
							return true;
						}
					}
				}

				//only accept actual responses, not spoofed stuff.
				if (stun->magiccookie == BigLong(0x2112a442)
					&& stun->transactid[0]==col->srflx_tid[0]
					&& stun->transactid[1]==col->srflx_tid[1]
					&& stun->transactid[2]==col->srflx_tid[2]
					&& !NET_CompareAdr(&col->srflx[adr.type!=NA_IP], &adr))
				{
					if (col->srflx[adr.type!=NA_IP].type==NA_INVALID)
						Con_Printf(S_COLOR_GRAY"Public address reported as %s\n", NET_AdrToString(errmsg, sizeof(errmsg), &adr));
					else
						Con_Printf(CON_ERROR"Server reflexive address changed to %s\n", NET_AdrToString(errmsg, sizeof(errmsg), &adr));
					col->srflx[adr.type!=NA_IP] = adr;
				}
			}
			return true;
		}
		else if (stun->msgtype == BigShort(STUN_BINDING|STUN_INDICATION))// && net_message.cursize == stunlen + sizeof(*stun) && stun->magiccookie == BigLong(0x2112a442))
		{
			//binding indication. used as an rtp keepalive. should have a fingerprint
			return true;
		}
		else if (stun->msgtype == BigShort(STUN_DATA|STUN_INDICATION)
				 && net_message.cursize == stunlen + sizeof(*stun) && stun->magiccookie == BigLong(STUN_MAGIC_COOKIE))
		{
			//TURN relayed data
			//these MUST come from a _known_ turn server.
			netadr_t adr;
			char xor[16];
			short portxor;
			void *data = NULL;
			unsigned short datasize = 0;
			unsigned short attrval;
			stunattr_t *attr = (stunattr_t*)(stun+1);
			int alen;
			unsigned int network = net_from.connum-1;	//also net_from_connection->connum
			struct icestate_s *con;

			if (network < MAX_CONNECTIONS)
				return true;	//don't handle this if its on the non-turn sockets.
			network -= MAX_CONNECTIONS;
			if (network < countof(con->server))
				return true; //don't double-decapsulate...
			network -= countof(con->server);

			for (con = icelist; con; con = con->next)
			{
				if (network < con->servers && net_from_connection == con->server[network].con)
					break;
			}
			if (!con)
				return true;	//don't know what it was. just ignore it.
			if (network >= con->servers || !NET_CompareAdr(&net_from, &con->server[network].addr))
				return true;	//right socket, but not from the server that we expected...

			adr.type = NA_INVALID;
			while(stunlen>0)
			{
				stunlen -= sizeof(*attr);
				alen = (unsigned short)BigShort(attr->attrlen);
				if (alen > stunlen)
					return false;
				stunlen -= (alen+3)&~3;
				attrval = BigShort(attr->attrtype);
				switch(attrval)
				{
				default:
					if (attrval & 0x8000)
						break;	//okay to ignore
					return true;
				case STUNATTR_DATA:
					data = attr+1;
					datasize = alen;
					break;
				case STUNATTR_XOR_PEER_ADDRESS:
					//always xor
					portxor = *(short*)&stun->magiccookie;
					memcpy(xor, &stun->magiccookie, sizeof(xor));

					adr.prot = NP_DGRAM;
					adr.connum = net_from.connum;
					adr.scopeid = net_from.scopeid;
					if (alen == 8 && ((qbyte*)attr)[5] == 1)		//ipv4
					{
						adr.type = NA_IP;
						adr.port = (((short*)attr)[3]) ^ portxor;
						*(int*)adr.address.ip = *(int*)(&((qbyte*)attr)[8]) ^ *(int*)xor;
					}
					else if (alen == 20 && ((qbyte*)attr)[5] == 2)	//ipv6
					{
						adr.type = NA_IPV6;
						adr.port = (((short*)attr)[3]) ^ portxor;
						((int*)adr.address.ip6)[0] = ((int*)&((qbyte*)attr)[8])[0] ^ ((int*)xor)[0];
						((int*)adr.address.ip6)[1] = ((int*)&((qbyte*)attr)[8])[1] ^ ((int*)xor)[1];
						((int*)adr.address.ip6)[2] = ((int*)&((qbyte*)attr)[8])[2] ^ ((int*)xor)[2];
						((int*)adr.address.ip6)[3] = ((int*)&((qbyte*)attr)[8])[3] ^ ((int*)xor)[3];
					}
					break;
				}
				alen = (alen+3)&~3;
				attr = (stunattr_t*)((char*)(attr+1) + alen);
			}
			if (data)
			{
				memmove(net_message.data, data, net_message.cursize = datasize);
				adr.connum = net_from.connum-countof(con->server);	//came via the relay.
				net_from = adr;
				col->ReadGamePacket();
				return true;
			}
		}

		else if ((stun->msgtype == BigShort(STUN_CREATEPERM|STUN_REPLY) || stun->msgtype == BigShort(STUN_CREATEPERM|STUN_ERROR))
				 && net_message.cursize == stunlen + sizeof(*stun) && stun->magiccookie == BigLong(STUN_MAGIC_COOKIE))
		{
			//TURN CreatePermissions reply (or error)
			unsigned short attrval;
			stunattr_t *attr = (stunattr_t*)(stun+1), *nonce=NULL, *realm=NULL;
			int alen;
			struct iceserver_s *s = NULL;
			int i, j;
			struct icestate_s *con;
			char errmsg[128];
			int err = 0;
			*errmsg = 0;

			//make sure it makes sense.
			while(stunlen>0)
			{
				stunlen -= sizeof(*attr);
				alen = (unsigned short)BigShort(attr->attrlen);
				if (alen > stunlen)
					return false;
				stunlen -= (alen+3)&~3;
				attrval = BigShort(attr->attrtype);
				switch(attrval)
				{
				default:
					if (attrval & 0x8000)
						break;	//okay to ignore
					return true;
				case STUNATTR_NONCE:
					nonce = attr;
					break;
				case STUNATTR_REALM:
					realm = attr;
					break;
				case STUNATTR_ERROR_CODE:
					{
						unsigned short len = BigShort(attr->attrlen)-4;
						if (len > sizeof(errmsg)-1)
							len = sizeof(errmsg)-1;
						memcpy(errmsg, &((qbyte*)attr)[8], len);
						errmsg[len] = 0;
						if (err==0)
							err = (((qbyte*)attr)[6]*100) + (((qbyte*)attr)[7]%100);
					}
					break;
				case STUNATTR_MSGINTEGRITIY_SHA1:
					break;
				}
				alen = (alen+3)&~3;
				attr = (stunattr_t*)((char*)(attr+1) + alen);
			}

			//now figure out what it acked.
			for (con = icelist; con; con = con->next)
			{
				for (i = 0; i < con->servers; i++)
				{
					s = &con->server[i];
					if (NET_CompareAdr(&net_from, &s->addr))
						for (j = 0; j < s->peers; j++)
						{
							if (s->peer[j].stunrnd[0] == stun->transactid[0] && s->peer[j].stunrnd[1] == stun->transactid[1] && s->peer[j].stunrnd[2] == stun->transactid[2])
							{	//the lifetime of a permission is a fixed 5 mins (this is separately from the port allocation)
								unsigned int now = Sys_Milliseconds();

								if (err)
								{
									if (err == 438 && realm && nonce)
									{
										alen = BigShort(nonce->attrlen);
										Z_Free(s->nonce);
										s->nonce = Z_Malloc(alen+1);
										memcpy(s->nonce, nonce+1, alen);
										s->nonce[alen] = 0;

										alen = BigShort(realm->attrlen);
										Z_Free(s->realm);
										s->realm = Z_Malloc(alen+1);
										memcpy(s->realm, realm+1, alen);
										s->realm[alen] = 0;

										s->peer[j].retry = now;	//retry fast.
									}
								}
								else
								{
									now -= 25;	//we don't know when it acked, so lets pretend we're a few MS ago.
									s->peer[j].expires = now + 5*60*1000;
									s->peer[j].retry = now + 4*60*1000;	//start trying to refresh it a min early (which will do resends).
								}

								//next attempt will use a different id.
								Sys_RandomBytes((char*)s->peer[i].stunrnd, sizeof(s->peer[i].stunrnd));
								return true;
							}
						}
				}
				if (i < con->servers)
					break;
			}

			return true;
		}
		else if ((stun->msgtype == BigShort(STUN_ALLOCATE|STUN_REPLY) || stun->msgtype == BigShort(STUN_ALLOCATE|STUN_ERROR)||
				 (stun->msgtype == BigShort(STUN_REFRESH|STUN_REPLY) || stun->msgtype == BigShort(STUN_REFRESH|STUN_ERROR)))
				 && net_message.cursize == stunlen + sizeof(*stun) && stun->magiccookie == BigLong(STUN_MAGIC_COOKIE))
		{
			//TURN allocate reply (or error)
			netadr_t adrs[2], ladr, *adr;	//the last should be our own ip.
			char xor[16];
			short portxor;
			unsigned short attrval;
			stunattr_t *attr = (stunattr_t*)(stun+1);
			int alen;
			int err = 0;
			char errmsg[64];
			struct iceserver_s *s = NULL;
			int i;
			struct icestate_s *con;
			qboolean noncechanged = false;
			unsigned int lifetime = 0;

			//gotta have come from our private socket.
			unsigned int network = net_from.connum-1;	//also net_from_connection->connum
			if (network < MAX_CONNECTIONS)
				return true;	//don't handle this if its on the non-turn sockets.
			network -= MAX_CONNECTIONS;
			if (network < countof(con->server))
				return true; //TURN-over-TURN is bad...
			network -= countof(con->server);

			for (con = icelist; con; con = con->next)
			{
				if (network < con->servers && net_from_connection == con->server[network].con)
				{
					s = &con->server[network];
					if (s->stunrnd[0] == stun->transactid[0] && s->stunrnd[1] == stun->transactid[1] && s->stunrnd[2] == stun->transactid[2] && NET_CompareAdr(&net_from, &s->addr))
						break;
					if (net_ice_debug.ival)
						Con_Printf(S_COLOR_GRAY"Stale transaction id (got %x, expected %x)\n", stun->transactid[0], s->stunrnd[0]);
				}
			}
			if (!con)
				return true;	//don't know what it was. just ignore it.

			network += 1 + MAX_CONNECTIONS;	//fix it up to refer to the relay rather than the private socket.

			adrs[0].type = NA_INVALID;
			adrs[1].type = NA_INVALID;
			ladr.type = NA_INVALID;

			while(stunlen>0)
			{
				stunlen -= sizeof(*attr);
				alen = (unsigned short)BigShort(attr->attrlen);
				if (alen > stunlen)
					return false;
				stunlen -= (alen+3)&~3;
				attrval = BigShort(attr->attrtype);
				switch(attrval)
				{
				case STUNATTR_LIFETIME:
					if (alen >= 4)
						lifetime = BigLong(*(int*)(attr+1));
					break;
//				case STUNATTR_SOFTWARE:
				case STUNATTR_MSGINTEGRITIY_SHA1:
//				case STUNATTR_FINGERPRINT:
					break;
				default:
					if (attrval & 0x8000)
						break;	//okay to ignore
					err = -1;	//got an attribute we 'must' handle...
					return true;
				case STUNATTR_NONCE:
					Z_Free(s->nonce);
					s->nonce = Z_Malloc(alen+1);
					memcpy(s->nonce, attr+1, alen);
					s->nonce[alen] = 0;
					noncechanged = true;
					break;
				case STUNATTR_REALM:
					Z_Free(s->realm);
					s->realm = Z_Malloc(alen+1);
					memcpy(s->realm, attr+1, alen);
					s->realm[alen] = 0;
					break;
				case STUNATTR_XOR_RELAYED_ADDRESS:
				case STUNATTR_XOR_MAPPED_ADDRESS:
					if (BigShort(attr->attrtype) == STUNATTR_XOR_MAPPED_ADDRESS)
						adr = &ladr;
					else
						adr = adrs[0].type?&adrs[1]:&adrs[0];
					//always xor
					portxor = *(short*)&stun->magiccookie;
					memcpy(xor, &stun->magiccookie, sizeof(xor));

					adr->prot = NP_DGRAM;
					adr->connum = net_from.connum;
					adr->scopeid = net_from.scopeid;
					if (alen == 8 && ((qbyte*)attr)[5] == 1)		//ipv4
					{
						adr->type = NA_IP;
						adr->port = (((short*)attr)[3]) ^ portxor;
						*(int*)adr->address.ip = *(int*)(&((qbyte*)attr)[8]) ^ *(int*)xor;
					}
					else if (alen == 20 && ((qbyte*)attr)[5] == 2)	//ipv6
					{
						adr->type = NA_IPV6;
						adr->port = (((short*)attr)[3]) ^ portxor;
						((int*)adr->address.ip6)[0] = ((int*)&((qbyte*)attr)[8])[0] ^ ((int*)xor)[0];
						((int*)adr->address.ip6)[1] = ((int*)&((qbyte*)attr)[8])[1] ^ ((int*)xor)[1];
						((int*)adr->address.ip6)[2] = ((int*)&((qbyte*)attr)[8])[2] ^ ((int*)xor)[2];
						((int*)adr->address.ip6)[3] = ((int*)&((qbyte*)attr)[8])[3] ^ ((int*)xor)[3];
					}
					break;
				case STUNATTR_ERROR_CODE:
					{
						unsigned short len = BigShort(attr->attrlen)-4;
						if (len > sizeof(errmsg)-1)
							len = sizeof(errmsg)-1;
						memcpy(errmsg, &((qbyte*)attr)[8], len);
						errmsg[len] = 0;
						if (!len)
							Q_strncpyz(errmsg, "<no description>", len);
						if (err==0)
							err = (((qbyte*)attr)[6]*100) + (((qbyte*)attr)[7]%100);
					}
					break;
				}
				alen = (alen+3)&~3;
				attr = (stunattr_t*)((char*)(attr+1) + alen);
			}

			if (err)
			{
				char sender[256];

				if (err == 438/*stale nonce*/)
				{	//reset everything.
					s->state = noncechanged?TURN_HAVE_NONCE:TURN_UNINITED;
					s->stunretry = Sys_Milliseconds();

					if (net_ice_debug.ival >= 1)
						Con_Printf(S_COLOR_GRAY"[%s]: %s: TURN error code %u : %s\n", con->friendlyname, NET_AdrToString(sender, sizeof(sender), &net_from), err, errmsg);
				}
				else if (err == 403/*forbidden*/)	//something bad...
				{
					s->state = TURN_UNINITED, s->stunretry = Sys_Milliseconds() + 60*1000;
					if (net_ice_debug.ival >= 1)
						Con_Printf(CON_ERROR"[%s]: %s: TURN error code %u : %s\n", con->friendlyname, NET_AdrToString(sender, sizeof(sender), &net_from), err, errmsg);
				}
				else if (err == 401 && s->state == TURN_UNINITED && s->nonce)	//failure when sending auth... give up for a min
				{	//this happens from initial auth. we need to reply with the real auth request now.
					s->state = TURN_HAVE_NONCE, s->stunretry = Sys_Milliseconds();
				}
				else
				{
					s->stunretry = Sys_Milliseconds() + 60*1000;
//					if (net_ice_debug.ival >= 1)
						Con_Printf(CON_ERROR"[%s]: %s: TURN error code %u : %s\n", con->friendlyname, NET_AdrToString(sender, sizeof(sender), &net_from), err, errmsg);
				}
			}
			else
			{
				struct icecandidate_s *lc;
				for (i = 0; i < countof(adrs); i++)
				{
					if (adrs[i].type != NA_INVALID && stun->msgtype == BigShort(STUN_ALLOCATE|STUN_REPLY))
					{
						s->state = TURN_ALLOCATED;

						if (!i)
							s->family = adrs[i].type;
						if (s->family != adrs[i].type)
							s->family = NA_INVALID;	//send it both types.

						if (ladr.type != NA_INVALID)
							adr = &ladr;	//can give a proper reflexive address
						else
							adr = &adrs[i];	//no info... give something.

						//check to see if this is a new server-reflexive address, which happens when the peer is behind a nat.
						for (lc = con->lc; lc; lc = lc->next)
						{
							if (NET_CompareAdr(&adrs[i], &lc->peer))
								break;
						}
						if (!lc)
						{
							int rnd[2];
							struct icecandidate_s *src;	//server Reflexive Candidate
							char str[256];
							src = Z_Malloc(sizeof(*src));
							src->next = con->lc;
							con->lc = src;
							src->peer = adrs[i];
							NET_BaseAdrToString(src->info.addr, sizeof(src->info.addr), &adrs[i]);
							src->info.port = ntohs(adrs[i].port);
							NET_BaseAdrToString(src->info.reladdr, sizeof(src->info.reladdr), adr);
							src->info.relport = ntohs(adr->port);
							src->info.type = ICE_RELAY;
							src->info.component = 1;
							src->info.network = network;
							src->dirty = true;
							src->info.priority = ICE_ComputePriority(&adrs[i], &src->info);

							Sys_RandomBytes((void*)rnd, sizeof(rnd));
							Q_strncpyz(src->info.candidateid, va("x%08x%08x", rnd[0], rnd[1]), sizeof(src->info.candidateid));

							if (net_ice_debug.ival >= 1)
								Con_Printf(S_COLOR_GRAY"[%s]: Relayed local candidate: %s\n", con->friendlyname, NET_AdrToString(str, sizeof(str), &adrs[i]));
						}
					}
				}
				if (lifetime < 60)	//don't spam reauth requests too often...
					lifetime = 60;
				s->stunretry = Sys_Milliseconds() + (lifetime-50)*1000;
				return true;
			}
			return true;
		}

#endif
			if (stun->msgtype == BigShort(STUN_BINDING|STUN_REQUEST) && net_message.cursize == stunlen + sizeof(*stun) && stun->magiccookie == BigLong(STUN_MAGIC_COOKIE))
		{
			char username[256];
			char integrity[20];
#ifdef SUPPORT_ICE
			struct icestate_s *con;
			int role = 0;
			unsigned int tiehigh = 0;
			unsigned int tielow = 0;
			qboolean usecandidate = false;
			unsigned int priority = 0;
			char *lpwd = NULL;
#endif
			char *integritypos = NULL;
			int error = 0;

			sizebuf_t buf;
			char data[512];
			int alen = 0, atype = 0, aofs = 0;
			int crc;

			//binding request
			stunattr_t *attr = (stunattr_t*)(stun+1);
			*username = 0;
			while(stunlen)
			{
				alen = (unsigned short)BigShort(attr->attrlen);
				if (alen+sizeof(*attr) > stunlen)
					return false;
				switch((unsigned short)BigShort(attr->attrtype))
				{				case 0xc057: /*'network cost'*/ break;
				default:
					//unknown attributes < 0x8000 are 'mandatory to parse', and such packets must be dropped in their entirety.
					//other ones are okay.
					if (!((unsigned short)BigShort(attr->attrtype) & 0x8000))
						return false;
					break;
				case STUNATTR_USERNAME:
					if (alen < sizeof(username))
					{
						memcpy(username, attr+1, alen);
						username[alen] = 0;
//						Con_Printf("Stun username = \"%s\"\n", username);
					}
					break;
				case STUNATTR_MSGINTEGRITIY_SHA1:
					memcpy(integrity, attr+1, sizeof(integrity));
					integritypos = (char*)(attr+1);
					break;
#ifdef SUPPORT_ICE
				case STUNATTR_ICE_PRIORITY:
//					Con_Printf("priority = \"%i\"\n", priority);
					priority = BigLong(*(int*)(attr+1));
					break;
				case STUNATTR_ICE_USE_CANDIDATE:
					usecandidate = true;
					break;
#endif
				case STUNATTR_FINGERPRINT:
//					Con_Printf("fingerprint = \"%08x\"\n", BigLong(*(int*)(attr+1)));
					break;
#ifdef SUPPORT_ICE
				case STUNATTR_ICE_CONTROLLED:
				case STUNATTR_ICE_CONTROLLING:
					role = (unsigned short)BigShort(attr->attrtype);
					tiehigh = BigLong(((int*)(attr+1))[0]);
					tielow = BigLong(((int*)(attr+1))[1]);
					break;
#endif
				}
				alen = (alen+3)&~3;
				attr = (stunattr_t*)((char*)(attr+1) + alen);
				stunlen -= alen+sizeof(*attr);
			}

#ifdef SUPPORT_ICE
			if (*username || integritypos)
			{
				//we need to know which connection its from in order to validate the integrity
				for (con = icelist; con; con = con->next)
				{
					if (!strcmp(va("%s:%s", con->lufrag, con->rufrag), username))
						break;
				}
				if (!con)
				{
					if (net_ice_debug.ival >= 2)
						Con_Printf("Received STUN request from unknown user \"%s\"\n", username);
					return true;
				}
				/*else if (con->chosenpeer.type != NA_INVALID)
				{	//got one.
					if (!NET_CompareAdr(&net_from, &con->chosenpeer))
						return true;	//FIXME: we're too stupid to handle switching. pretend to be dead.
				}*/
				else if (con->state == ICE_INACTIVE)
					return true;	//bad timing
				else
				{
					struct icecandidate_s *rc;

					if (net_ice_debug.ival >= 2)
						Con_Printf(S_COLOR_GRAY"[%s]: got binding request on %s from %s\n", con->friendlyname, ICE_NetworkToName(con, net_from.connum), NET_AdrToString(username,sizeof(username), &net_from));

					if (integritypos)
					{
						char key[20];
						//the hmac is a bit weird. the header length includes the integrity attribute's length, but the checksum doesn't even consider the attribute header.
						stun->msglen = BigShort(integritypos+sizeof(integrity) - (char*)stun - sizeof(*stun));
						CalcHMAC(&hash_sha1, key, sizeof(key), (qbyte*)stun, integritypos-4 - (char*)stun, con->lpwd, strlen(con->lpwd));
						if (memcmp(key, integrity, sizeof(integrity)))
						{
							Con_DPrintf(CON_WARNING"Integrity is bad! needed %x got %x\n", *(int*)key, *(int*)integrity);
							return true;
						}
					}

					//check to see if this is a new peer-reflexive address, which happens when the peer is behind a nat.
					for (rc = con->rc; rc; rc = rc->next)
					{
						if (NET_CompareAdr(&net_from, &rc->peer))
							break;
					}
					if (!rc)
					{
						//netadr_t reladdr;
						//int relflags;
						//const char *relpath;
						struct icecandidate_s *rc;
						rc = Z_Malloc(sizeof(*rc));
						rc->next = con->rc;
						con->rc = rc;

						rc->peer = net_from;
						NET_BaseAdrToString(rc->info.addr, sizeof(rc->info.addr), &net_from);
						rc->info.port = ntohs(net_from.port);
						//if (net_from.connum >= 1 && net_from.connum < 1+MAX_CONNECTIONS && col->conn[net_from.connum-1])
						//	col->conn[net_from.connum-1]->GetLocalAddresses(col->conn[net_from.connum-1], &relflags, &reladdr, &relpath, 1);
						//FIXME: we don't really know which one... NET_BaseAdrToString(rc->info.reladdr, sizeof(rc->info.reladdr), &reladdr);
						//rc->info.relport = ntohs(reladdr.port);
						rc->info.type = ICE_PRFLX;
						rc->dirty = true;
						rc->info.priority = priority;
					}

					//flip ice control role, if we're wrong.
					if (role && role != (con->controlled?STUNATTR_ICE_CONTROLLING:STUNATTR_ICE_CONTROLLED))
					{
						if (tiehigh == con->tiehigh && tielow == con->tielow)
						{
							Con_Printf("ICE: Evil loopback hack enabled\n");
							if (usecandidate)
							{
								if ((con->chosenpeer.connum != net_from.connum || !NET_CompareAdr(&con->chosenpeer, &net_from)) && net_ice_debug.ival >= 1)
								{
									char msg[64];
									if (con->chosenpeer.connum-1 >= MAX_CONNECTIONS)
										Con_Printf(S_COLOR_GRAY"[%s]: New peer imposed %s, via %s.\n", con->friendlyname, NET_AdrToString(msg, sizeof(msg), &net_from), ICE_NetworkToName(con, con->chosenpeer.connum));
									else
										Con_Printf(S_COLOR_GRAY"[%s]: New peer imposed %s.\n", con->friendlyname, NET_AdrToString(msg, sizeof(msg), &net_from));
								}
								con->chosenpeer = net_from;

								if (con->state == ICE_CONNECTING)
									ICE_Set(con, "state", STRINGIFY(ICE_CONNECTED));
							}
						}
						else
						{
							con->controlled = (tiehigh > con->tiehigh) || (tiehigh == con->tiehigh && tielow > con->tielow);
							if (net_ice_debug.ival >= 1)
								Con_Printf(S_COLOR_GRAY"[%s]: role conflict detected. We should be %s\n", con->friendlyname, con->controlled?"controlled":"controlling");
							error = 87;
						}
					}
					else if (usecandidate && con->controlled)
					{
						//in the controlled role, we're connected once we're told the pair to use (by the usecandidate flag).
						//note that this 'nominates' candidate pairs, from which the highest priority is chosen.
						//so we just pick select the highest.
						//this is problematic, however, as we don't actually know the real priority that the peer thinks we'll nominate it with.

						if ((con->chosenpeer.connum != net_from.connum || !NET_CompareAdr(&con->chosenpeer, &net_from)) && net_ice_debug.ival >= 1)
						{
							char msg[64];
							Con_Printf(S_COLOR_GRAY"[%s]: New peer imposed %s, via %s.\n", con->friendlyname, NET_AdrToString(msg, sizeof(msg), &net_from), ICE_NetworkToName(con, net_from.connum));
						}
						con->chosenpeer = net_from;

						if (con->state == ICE_CONNECTING)
							ICE_Set(con, "state", STRINGIFY(ICE_CONNECTED));
					}
					lpwd = con->lpwd;
				}
			}//otherwise its just an ip check
			else
				con = NULL;
#else
			(void)integritypos;
#endif

			memset(&buf, 0, sizeof(buf));
			buf.maxsize = sizeof(data);
			buf.cursize = 0;
			buf.data = data;

			if (net_from.type == NA_IP)
			{
				alen = 4;
				atype = 1;
				aofs = 0;
			}
			else if (net_from.type == NA_IPV6 &&
						!*(int*)&net_from.address.ip6[0] &&
						!*(int*)&net_from.address.ip6[4] &&
						!*(short*)&net_from.address.ip6[8] &&
						*(short*)&net_from.address.ip6[10] == (short)0xffff)
			{	//just because we use an ipv6 address for ipv4 internally doesn't mean we should tell the peer that they're on ipv6...
				alen = 4;
				atype = 1;
				aofs = sizeof(net_from.address.ip6) - sizeof(net_from.address.ip);
			}
			else if (net_from.type == NA_IPV6)
			{
				alen = 16;
				atype = 2;
				aofs = 0;
			}
			else
			{
				alen = 0;
				atype = 0;
			}

//Con_DPrintf("STUN from %s\n", NET_AdrToString(data, sizeof(data), &net_from));

			MSG_WriteShort(&buf, BigShort(error?(STUN_BINDING|STUN_ERROR):(STUN_BINDING|STUN_REPLY)));
			MSG_WriteShort(&buf, BigShort(0));	//fill in later
			MSG_WriteLong(&buf, stun->magiccookie);
			MSG_WriteLong(&buf, stun->transactid[0]);
			MSG_WriteLong(&buf, stun->transactid[1]);
			MSG_WriteLong(&buf, stun->transactid[2]);

			if (error == 87)
			{
				char *txt = "Role Conflict";
				MSG_WriteShort(&buf, BigShort(STUNATTR_ERROR_CODE));
				MSG_WriteShort(&buf, BigShort(4 + strlen(txt)));
				MSG_WriteShort(&buf, 0);	//reserved
				MSG_WriteByte(&buf, 0);		//class
				MSG_WriteByte(&buf, error);	//code
				SZ_Write(&buf, txt, strlen(txt));	//readable
				while(buf.cursize&3)		//padding
					MSG_WriteChar(&buf, 0);
			}
			else if (1)
			{	//xor mapped
				netadr_t xored = net_from;
				int i;
				xored.port ^= *(short*)(data+4);
				for (i = 0; i < alen; i++)
					((qbyte*)&xored.address)[aofs+i] ^= ((qbyte*)data+4)[i];
				MSG_WriteShort(&buf, BigShort(STUNATTR_XOR_MAPPED_ADDRESS));
				MSG_WriteShort(&buf, BigShort(4+alen));
				MSG_WriteShort(&buf, BigShort(atype));
				MSG_WriteShort(&buf, xored.port);
				SZ_Write(&buf, (char*)&xored.address + aofs, alen);
			}
			else
			{	//non-xor mapped
				MSG_WriteShort(&buf, BigShort(STUNATTR_MAPPED_ADDRESS));
				MSG_WriteShort(&buf, BigShort(4+alen));
				MSG_WriteShort(&buf, BigShort(atype));
				MSG_WriteShort(&buf, net_from.port);
				SZ_Write(&buf, (char*)&net_from.address + aofs, alen);
			}

			MSG_WriteShort(&buf, BigShort(STUNATTR_USERNAME));	//USERNAME
			MSG_WriteShort(&buf, BigShort(strlen(username)));
			SZ_Write(&buf, username, strlen(username));
			while(buf.cursize&3)
				MSG_WriteChar(&buf, 0);

#ifdef SUPPORT_ICE
			if (lpwd)
			{
				//message integrity is a bit annoying
				data[2] = ((buf.cursize+4+hash_sha1.digestsize-20)>>8)&0xff;	//hashed header length is up to the end of the hmac attribute
				data[3] = ((buf.cursize+4+hash_sha1.digestsize-20)>>0)&0xff;
				//but the hash is to the start of the attribute's header
				CalcHMAC(&hash_sha1, integrity, sizeof(integrity), data, buf.cursize, lpwd, strlen(lpwd));
				MSG_WriteShort(&buf, BigShort(STUNATTR_MSGINTEGRITIY_SHA1));
				MSG_WriteShort(&buf, BigShort(hash_sha1.digestsize));	//sha1 key length
				SZ_Write(&buf, integrity, hash_sha1.digestsize);	//integrity data
			}
#endif

			data[2] = ((buf.cursize+8-20)>>8)&0xff;	//dummy length
			data[3] = ((buf.cursize+8-20)>>0)&0xff;
			crc = crc32(0, data, buf.cursize)^0x5354554e;
			MSG_WriteShort(&buf, BigShort(STUNATTR_FINGERPRINT));	//FINGERPRINT
			MSG_WriteShort(&buf, BigShort(sizeof(crc)));
			MSG_WriteLong(&buf, BigLong(crc));

			data[2] = ((buf.cursize-20)>>8)&0xff;
			data[3] = ((buf.cursize-20)>>0)&0xff;

#ifdef SUPPORT_ICE
			if (con)
				TURN_Encapsulate(con, &net_from, data, buf.cursize);
			else
#endif
				NET_SendPacket(col, buf.cursize, data, &net_from);
			return true;
		}
	}


#ifdef SUPPORT_ICE
	{
		struct icestate_s *con;
		struct icecandidate_s *rc;
		for (con = icelist; con; con = con->next)
		{
			for (rc = con->rc; rc; rc = rc->next)
			{
				if (NET_CompareAdr(&net_from, &rc->peer))
				{
				//	if (rc->reachable)
					{	//found it. fix up its source address to our ICE connection (so we don't have path-switching issues) and keep chugging along.
						con->icetimeout = Sys_Milliseconds() + 1000*30;	//not dead yet...

#ifdef HAVE_DTLS
						if (con->dtlsstate)
						{
							switch(con->dtlsfuncs->Received(con->dtlsstate, &net_message))
							{
							case NETERR_SENT:
								break;	//
							case NETERR_NOROUTE:
								return false;	//not a dtls packet at all. don't de-ICE it when we're meant to be using ICE.
							case NETERR_DISCONNECTED:	//dtls failure. ICE failed.
								ICE_SetFailed(con, "DTLS Terminated");
								return true;
							default: //some kind of failure decoding the dtls packet. drop it.
								return true;
							}
						}
#endif
						net_from = con->qadr;
#ifdef HAVE_DTLS
						if (con->sctp)
							SCTP_Decode(con->sctp, col);
						else
#endif
						if (net_message.cursize)
							col->ReadGamePacket();
						return true;
					}
				}
			}
		}
	}
#endif
	return false;
}
#ifdef SUPPORT_ICE
int ICE_GetPeerCertificate(netadr_t *to, enum certprops_e prop, char *out, size_t outsize)
{
#ifdef HAVE_DTLS
	struct icestate_s *con;
	int i, c;
	for (con = icelist; con; con = con->next)
	{
		if (NET_CompareAdr(to, &con->qadr))
		{
			if (prop==QCERT_LOBBYSTATUS)
			{
				*out = 0;
				switch(con->state)
				{
				case ICE_INACTIVE:
					Q_strncpyz(out, "idle", outsize);
					break;
				case ICE_FAILED:
					Q_strncpyz(out, "Failed", outsize);
					break;
				case ICE_GATHERING:
					Q_strncpyz(out, "Gathering", outsize);
					break;
				case ICE_CONNECTING:
					for (i = 0, c = false; i < con->servers; i++)
						if (!con->server[i].isstun)
						{
							if (con->server[i].state == TURN_ALLOCATED)
								break;
							c = true;
						}
					if (i == con->servers)
					{
						if (net_ice_relayonly.ival)
							Q_strncpyz(out, "Probing ("CON_ERROR"NO TURN SERVER"CON_DEFAULT")", outsize);	//can't work, might still get an allocation though.
						else if (c)
							Q_strncpyz(out, "Probing ("CON_WARNING"waiting for TURN allocation"CON_DEFAULT")", outsize);	//still good for latency. not for privacy though.
						else
							Q_strncpyz(out, "Probing ("CON_WARNING"no relay configured"CON_DEFAULT")", outsize);	//still good for latency. not for privacy though.
					}
					else
						Q_strncpyz(out, "Probing ("S_COLOR_GREEN"with fallback"CON_DEFAULT")", outsize);	//we have a relay for a fallback, all is good, hopefully. except we're still at this stage...
					break;
				case ICE_CONNECTED:	//past the ICE stage (but maybe not the dtls+sctp layers, these should be less likely to fail, but dtls versions may become an issue)
					//if (con->dtlsstate && notokay)
					if (con->sctp && !con->sctp->o.writable)
						Q_strncpyz(out, "Establishing", outsize);	//will also block for the dtls channel of course. its not as easy check the dtls layer.
					else
						Q_strncpyz(out, "Established", outsize);
					break;
				}
				return strlen(out);
			}
			else if (con->dtlsstate && con->dtlsfuncs->GetPeerCertificate)
				return con->dtlsfuncs->GetPeerCertificate(con->dtlsstate, prop, out, outsize);
			else if (prop==QCERT_ISENCRYPTED && con->dtlsstate)
				return 0;
		}
	}
#endif
	return -1;
}
void ICE_Terminate(netadr_t *to)
{
	struct icestate_s *con;
	for (con = icelist; con; con = con->next)
	{
		if (NET_CompareAdr(to, &con->qadr))
		{
			ICE_Set(con, "state", STRINGIFY(ICE_INACTIVE));
			return;
		}
	}
}
neterr_t ICE_SendPacket(size_t length, const void *data, netadr_t *to)
{
	struct icestate_s *con;
	for (con = icelist; con; con = con->next)
	{
		if (NET_CompareAdr(to, &con->qadr))
		{
			con->icetimeout = Sys_Milliseconds()+30*1000;	//keep it alive a little longer.

			if (con->state == ICE_CONNECTING)
				return NETERR_CLOGGED;
			else if (con->state != ICE_CONNECTED)
				return NETERR_DISCONNECTED;
#ifdef HAVE_DTLS
			if (con->sctp)
				return SCTP_Transmit(con->sctp, data, length);
			if (con->dtlsstate)
				return con->dtlsfuncs->Transmit(con->dtlsstate, data, length);
#endif
			if (con->chosenpeer.type != NA_INVALID)
				return ICE_Transmit(con, data, length);
			return NETERR_CLOGGED;	//still pending
		}
	}
	return NETERR_DISCONNECTED;
}
#endif
#endif


#ifdef SUPPORT_ICE
//this is the clientside part of our custom accountless broker protocol
//basically just keeps the broker processing, but doesn't send/receive actual game packets.
//inbound messages can change ice connection states.
//clients only handle one connection. servers need to handle multiple
typedef struct {
	ftenet_generic_connection_t generic;

	//config state
	char brokername[64];	//dns name
	netadr_t brokeradr;		//actual ip
	char gamename[64];		//what we're trying to register as/for with the broker

	//broker connection state
	vfsfile_t *broker;
	qboolean handshaking;
	double nextping;		//send heartbeats every now and then
	double heartbeat;
	double timeout;			//detect if the broker goes dead, so we can reconnect reliably (instead of living for two hours without anyone able to connect).
	qbyte in[8192];
	size_t insize;
	qbyte out[8192];
	size_t outsize;
	int error;	//outgoing data is corrupt. kill it.



	//client state...
	struct icestate_s *ice;
	int serverid;

	//server state...
	struct
	{
		struct icestate_s *ice;
	} *clients;
	size_t numclients;
} ftenet_ice_connection_t;
static void FTENET_ICE_Close(ftenet_generic_connection_t *gcon)
{
	ftenet_ice_connection_t *b = (void*)gcon;
	int cl;
	if (b->broker)
		VFS_CLOSE(b->broker);

	for (cl = 0; cl < b->numclients; cl++)
		if (b->clients[cl].ice)
			iceapi.Close(b->clients[cl].ice, true);
	Z_Free(b->clients);
	if (b->ice)
		iceapi.Close(b->ice, true);

	Z_Free(b);
}

static void FTENET_ICE_Flush(ftenet_ice_connection_t *b)
{
	int r;
	if (!b->outsize || b->error || !b->broker)
		return;
	r = VFS_WRITE(b->broker, b->out, b->outsize);
	if (r > 0)
	{
		b->outsize -= r;
		memmove(b->out, b->out+r, b->outsize);
	}
	if (r < 0)
		b->error = true;
}
static neterr_t FTENET_ICE_SendPacket(ftenet_generic_connection_t *gcon, int length, const void *data, netadr_t *to)
{
	ftenet_ice_connection_t *b = (void*)gcon;
	if (to->prot != NP_RTC_TCP && to->prot != NP_RTC_TLS)
		return NETERR_NOROUTE;
	if (!NET_CompareAdr(to, &b->brokeradr))
		return NETERR_NOROUTE;	//its using some other broker, don't bother trying to handle it here.
	if (b->error)
		return NETERR_DISCONNECTED;
	return NETERR_CLOGGED;	//we'll switch to a connect localcmd when the connection completes, so we don't really need to send any packets when they're using ICE. Just make sure the client doesn't give up.
}

static void FTENET_ICE_SplurgeRaw(ftenet_ice_connection_t *b, const qbyte *data, size_t len)
{
//0: dropclient (cl=-1 drops entire connection)
	if (b->outsize+len > sizeof(b->out))
		b->error = true;
	else
	{
		memcpy(b->out+b->outsize, data, len);
		b->outsize += len;
	}
}
static void FTENET_ICE_SplurgeWS(ftenet_ice_connection_t *b, enum websocketpackettype_e pkttype, const qbyte *data1, size_t len1, const qbyte *data2, size_t len2)
{
	size_t tlen = len1+len2;
	qbyte header[8];
	header[0] = 0x80|pkttype;
	if (tlen >= 126)
	{
		header[1] = 126;
		header[2] = tlen>>8;	//bigendian
		header[3] = tlen&0xff;
		FTENET_ICE_SplurgeRaw(b, header, 4);
	}
	else
	{	//small data
		header[1] = tlen;
		FTENET_ICE_SplurgeRaw(b, header, 2);
	}
	FTENET_ICE_SplurgeRaw(b, data1, len1);
	FTENET_ICE_SplurgeRaw(b, data2, len2);
}
static void FTENET_ICE_SplurgeCmd(ftenet_ice_connection_t *b, int icemsg, int cl, const char *data)
{
	qbyte msg[3] = {icemsg, cl&0xff, (cl>>8)&0xff};	//little endian...
	FTENET_ICE_SplurgeWS(b, WS_PACKETTYPE_BINARYFRAME, msg, sizeof(msg), data, strlen(data));
}
static void FTENET_ICE_Heartbeat(ftenet_ice_connection_t *b)
{
	b->heartbeat = realtime+30;
#ifdef HAVE_SERVER
	if (b->generic.islisten)
	{
		char info[2048];
		SV_GeneratePublicServerinfo(info, info+sizeof(info));
		FTENET_ICE_SplurgeCmd(b, ICEMSG_SERVERINFO, -1, info);
	}
#endif
}
static void FTENET_ICE_SendOffer(ftenet_ice_connection_t *b, int cl, struct icestate_s *ice, const char *type)
{
	char buf[8192];
	//okay, now send the sdp to our peer.
	if (iceapi.Get(ice, type, buf, sizeof(buf)))
	{
		char json[8192+256];
		if (ice->state == ICE_GATHERING)
			ice->state = ICE_CONNECTING;
		if (ice->mode == ICEM_WEBRTC)
		{
			Q_strncpyz(json, va("{\"type\":\"%s\",\"sdp\":\"", type+3), sizeof(json));
			COM_QuotedString(buf, json+strlen(json), sizeof(json)-strlen(json)-2, true);
			Q_strncatz(json, "\"}", sizeof(json));
			FTENET_ICE_SplurgeCmd(b, ICEMSG_OFFER, cl, json);
		}
		else
			FTENET_ICE_SplurgeCmd(b, ICEMSG_OFFER, cl, buf);

		ice->blockcandidates = false;
	}
}
static void FTENET_ICE_Establish(ftenet_ice_connection_t *b, const char *peeraddr, int cl, struct icestate_s **ret)
{	//sends offer
	struct icestate_s *ice;
	qboolean usewebrtc;
	char *s;
	if (*ret)
		iceapi.Close(*ret, false);
#ifndef HAVE_DTLS
	usewebrtc = false;
#else
	if (!*net_ice_usewebrtc.string && net_enable_dtls.ival)
		usewebrtc = true;	//let the peer decide. this means we can use dtls, but not sctp.
	else
		usewebrtc = net_ice_usewebrtc.ival;
#endif
	ice = *ret = iceapi.Create(b, NULL, b->generic.islisten?((peeraddr&&*peeraddr)?va("%s:%i", peeraddr,cl):NULL):va("/%s", b->gamename), usewebrtc?ICEM_WEBRTC:ICEM_ICE, b->generic.islisten?ICEP_QWSERVER:ICEP_QWCLIENT, !b->generic.islisten);
	if (!*ret)
		return;	//some kind of error?!?

	iceapi.Set(ice, "server", va("stun:%s:%i", b->brokername, BigShort(b->brokeradr.port)));

	s = net_ice_servers.string;
	while((s=COM_Parse(s)))
		iceapi.Set(ice, "server", com_token);

	if (!b->generic.islisten)
		FTENET_ICE_SendOffer(b, cl, ice, "sdpoffer");
}
static void FTENET_ICE_Refresh(ftenet_ice_connection_t *b, int cl, struct icestate_s *ice)
{	//sends offer
	char buf[8192];
	if (ice->blockcandidates)
		return;	//don't send candidates before the offers...
	while (ice && iceapi.GetLCandidateSDP(ice, buf, sizeof(buf)))
	{
		char json[8192+256];
		if (ice->mode == ICEM_WEBRTC)
		{
			Q_strncpyz(json, "{\"candidate\":\"", sizeof(json));
			COM_QuotedString(buf+2, json+strlen(json), sizeof(json)-strlen(json)-2, true);
			Q_strncatz(json, "\",\"sdpMid\":\"0\",\"sdpMLineIndex\":0}", sizeof(json));
			FTENET_ICE_SplurgeCmd(b, ICEMSG_CANDIDATE, cl, json);
		}
		else
			FTENET_ICE_SplurgeCmd(b, ICEMSG_CANDIDATE, cl, buf);
	}
}
static void Buf_ReadString(const char **data, const char *end, char *out, size_t outsize)
{
	const char *in = *data;
	char c;
	outsize--;	//count the null early.
	while (in < end)
	{
		c = *in++;
		if (!c)
			break;
		if (outsize)
		{
			outsize--;
			*out++ = c;
		}
	}
	*out = 0;
	*data = in;
}
static qboolean FTENET_ICE_GetPacket(ftenet_generic_connection_t *gcon)
{
	json_t *json;
	ftenet_ice_connection_t *b = (void*)gcon;
	int ctrl, len, cmd, cl, ofs;
	const char *data;
	char n;

	if (!b->broker)
	{
		const char *s;
		if (b->timeout > realtime)
			return false;
		b->generic.thesocket = TCP_OpenStream(&b->brokeradr, b->brokername);	//save this for select.
		b->broker = FS_WrapTCPSocket(b->generic.thesocket, true, b->brokername);

#ifdef HAVE_SSL
		//convert to tls...
		if (b->brokeradr.prot == NP_TLS || b->brokeradr.prot == NP_RTC_TLS)
			b->broker = FS_OpenSSL(b->brokername, b->broker, false);
#endif

		if (!b->broker)
		{
			b->timeout = realtime + 30;
			Con_Printf("rtc broker connection to %s failed (retry: 30 secs)\n", b->brokername);
			return false;
		}

		b->insize = b->outsize = 0;

		COM_Parse(com_protocolname.string);

		b->handshaking = true;
		s = va("GET /%s/%s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"Connection: Upgrade\r\n"
			"Upgrade: websocket\r\n"
			"Sec-WebSocket-Version: 13\r\n"
			"Sec-WebSocket-Protocol: %s\r\n"
			"\r\n", com_token, b->gamename, b->brokername, b->generic.islisten?"rtc_host":"rtc_client");
		FTENET_ICE_SplurgeRaw(b, s, strlen(s));
		b->heartbeat = realtime;
		b->nextping = realtime + 100;
		b->timeout = realtime + 270;
	}
	if (b->error)
	{
handleerror:
		b->generic.thesocket = INVALID_SOCKET;
		if (b->broker)
			VFS_CLOSE(b->broker);
		b->broker = NULL;

		for (cl = 0; cl < b->numclients; cl++)
		{
			if (b->clients[cl].ice)
				iceapi.Close(b->clients[cl].ice, false);
			b->clients[cl].ice = NULL;
		}
		if (b->ice)
			iceapi.Close(b->ice, false);
		b->ice = NULL;
		if (b->error != 1 || !b->generic.islisten)
			return false;	//permanant error...
		b->error = false;
		b->insize = b->outsize = 0;
		b->timeout = realtime + 30;
		return false;
	}

	//keep checking for new candidate info.
	if (b->ice)
		FTENET_ICE_Refresh(b, b->serverid, b->ice);
	for (cl = 0; cl < b->numclients; cl++)
		if (b->clients[cl].ice)
			FTENET_ICE_Refresh(b, cl, b->clients[cl].ice);
	if (realtime >= b->heartbeat)
		FTENET_ICE_Heartbeat(b);

	len = VFS_READ(b->broker, b->in+b->insize, sizeof(b->in)-1-b->insize);
	if (!len)
	{
		FTENET_ICE_Flush(b);

		if (realtime > b->nextping)
		{	//nothing happening... make sure the connection isn't dead...
			FTENET_ICE_SplurgeWS(b, WS_PACKETTYPE_PING, NULL, 0, NULL, 0);
			b->nextping = realtime + 100;
		}
		return false;	//nothing new
	}
	if (len < 0)
	{
		if (!b->error)
			Con_Printf("rtc broker connection to %s failed (retry: 30 secs)\n", b->brokername);
		b->error = true;
		goto handleerror;
	}
	b->insize += len;
	b->in[b->insize] = 0;
	ofs = 0;

	b->nextping = realtime + 100;
	b->timeout = max(b->timeout, realtime + 270);

	if (b->handshaking)
	{	//we're still waiting for an http 101 code. websocket data starts straight after.
		char *end = strstr(b->in, "\r\n\r\n");
		if (!end)
			return false;	//not available yet...
		if (strncmp(b->in, "HTTP/1.1 101 ", 13))
		{
			b->error = ~0;
			return false;
		}
		end+=4;
		b->handshaking = false; //done...

		ofs = (qbyte*)end-b->in;
	}

	while (b->insize >= ofs+2)
	{
		ctrl = b->in[ofs+0];
		len = b->in[ofs+1];
		ofs+=2;
		if (len > 126)
		{//unsupported
			b->error = 1;
			break;
		}
		else if (len == 126)
		{
			if (b->insize <= 4)
				break;
			len = (b->in[ofs+0]<<8)|(b->in[ofs+1]);
			ofs+=2;
		}
		if (b->insize < ofs+len)
			break;
		n = b->in[ofs+len];
		b->in[ofs+len] = 0;

		switch(ctrl & 0xf)
		{
		case WS_PACKETTYPE_PING:
			FTENET_ICE_SplurgeWS(b, WS_PACKETTYPE_PONG, NULL, 0, b->in+ofs, len);
			FTENET_ICE_Flush(b);
			break;
		case WS_PACKETTYPE_CLOSE:
			b->error = true;
			break;
		default:
			break;
		case WS_PACKETTYPE_BINARYFRAME:
			cmd = b->in[ofs];
			cl = (short)(b->in[ofs+1] | (b->in[ofs+2]<<8));
			data = b->in+ofs+3;

			switch(cmd)
			{
			case ICEMSG_PEERLOST:	//the broker lost its connection to our peer...
				if (cl == -1)
				{
					b->error = true;
					if (net_ice_debug.ival)
						Con_Printf(S_COLOR_GRAY"[%s]: Broker host lost connection: %s\n", b->ice?b->ice->friendlyname:"?", *data?data:"<NO REASON>");
				}
				else if (cl >= 0 && cl < b->numclients)
				{
					if (net_ice_debug.ival)
						Con_Printf(S_COLOR_GRAY"[%s]: Broker client lost connection: %s\n", b->clients[cl].ice?b->clients[cl].ice->friendlyname:"?", *data?data:"<NO REASON>");
					if (b->clients[cl].ice)
						iceapi.Close(b->clients[cl].ice, false);
					b->clients[cl].ice = NULL;
				}
				break;
			case ICEMSG_NAMEINUSE:
				Con_Printf("Unable to listen on /%s - name already taken\n", b->gamename);
				b->error = true;	//try again later.
				break;
			case ICEMSG_GREETING:	//reports the trailing url we're 'listening' on. anyone else using that url will connect to us.
				data = strchr(data, '/');
				if (data++)
					Q_strncpyz(b->gamename, data, sizeof(b->gamename));
				Con_Printf("Publicly listening on /%s\n", b->gamename);
				break;
			case ICEMSG_NEWPEER:	//relay connection established with a new peer
				//note that the server ought to wait for an offer from the client before replying with any ice state, but it doesn't really matter for our use-case.
				{
					char peer[MAX_QPATH];
					char relay[MAX_QPATH];
					char *s;
					Buf_ReadString(&data, b->in+ofs+len, peer, sizeof(peer));
					Buf_ReadString(&data, b->in+ofs+len, relay, sizeof(relay));

					if (b->generic.islisten)
					{
	//					Con_DPrintf("Client connecting: %s\n", data);
						if (cl < 1024 && cl >= b->numclients)
						{	//looks like a new one... but don't waste memory
							Z_ReallocElements((void**)&b->clients, &b->numclients, cl+1, sizeof(b->clients[0]));
						}
						if (cl >= 0 && cl < b->numclients)
						{
							FTENET_ICE_Establish(b, *peer?peer:NULL, cl, &b->clients[cl].ice);
							for (s = relay; (s=COM_Parse(s)); )
								iceapi.Set(b->clients[cl].ice, "server", com_token);
							if (net_ice_debug.ival)
								Con_Printf(S_COLOR_GRAY"[%s]: New client spotted...\n", b->clients[cl].ice?b->clients[cl].ice->friendlyname:"?");
						}
						else if (net_ice_debug.ival)
							Con_Printf(S_COLOR_GRAY"[%s]: New client spotted, but index is unusable\n", "?");
					}
					else
					{
						//Con_DPrintf("Server found: %s\n", data);
						FTENET_ICE_Establish(b, *peer?peer:NULL, cl, &b->ice);
						b->serverid = cl;
						for (s = relay; (s=COM_Parse(s)); )
							iceapi.Set(b->ice, "server", com_token);
						if (net_ice_debug.ival)
							Con_Printf(S_COLOR_GRAY"[%s]: Meta channel to game server now open\n", b->ice?b->ice->friendlyname:"?");
					}
				}
				break;
			case ICEMSG_OFFER:	//we received an offer from a client
				json = JSON_Parse(data);
				if (json)
					//should probably also verify the type.
					data = JSON_GetString(json, "sdp", com_token,sizeof(com_token), NULL);
				if (b->generic.islisten)
				{
					if (cl >= 0 && cl < b->numclients && b->clients[cl].ice)
					{
						if (net_ice_debug.ival)
							Con_Printf(S_COLOR_GRAY"[%s]: Got offer:\n%s\n", b->clients[cl].ice?b->clients[cl].ice->friendlyname:"?", data);
						iceapi.Set(b->clients[cl].ice, "sdpoffer", data);
						iceapi.Set(b->clients[cl].ice, "state", STRINGIFY(ICE_CONNECTING));

						FTENET_ICE_SendOffer(b, cl, b->clients[cl].ice, "sdpanswer");
					}
					else if (net_ice_debug.ival)
						Con_Printf(S_COLOR_GRAY"[%s]: Got bad offer/answer:\n%s\n", b->clients[cl].ice?b->clients[cl].ice->friendlyname:"?", data);
				}
				else
				{
					if (b->ice)
					{
						if (net_ice_debug.ival)
							Con_Printf(S_COLOR_GRAY"[%s]: Got answer:\n%s\n", b->ice?b->ice->friendlyname:"?", data);
						iceapi.Set(b->ice, "sdpanswer", data);
						iceapi.Set(b->ice, "state", STRINGIFY(ICE_CONNECTING));
					}
					else if (net_ice_debug.ival)
						Con_Printf(S_COLOR_GRAY"[%s]: Got bad offer/answer:\n%s\n", b->ice?b->ice->friendlyname:"?", data);
				}
				JSON_Destroy(json);
				break;
			case ICEMSG_CANDIDATE:
				json = JSON_Parse(data);
				if (json)
				{
					data = com_token;
					com_token[0]='a';
					com_token[1]='=';
					com_token[2]=0;
					JSON_GetString(json, "candidate", com_token+2,sizeof(com_token)-2, NULL);
				}
//				Con_Printf("Candidate update: %s\n", data);
				if (b->generic.islisten)
				{
					if (cl >= 0 && cl < b->numclients && b->clients[cl].ice)
					{
						if (net_ice_debug.ival)
							Con_Printf(S_COLOR_GRAY"[%s]: Got candidate:\n%s\n", b->clients[cl].ice->friendlyname, data);
						iceapi.Set(b->clients[cl].ice, "sdp", data);
					}
				}
				else
				{
					if (b->ice)
					{
						if (net_ice_debug.ival)
							Con_Printf(S_COLOR_GRAY"[%s]: Got candidate:\n%s\n", b->ice->friendlyname, data);
						iceapi.Set(b->ice, "sdp", data);
					}
				}
				JSON_Destroy(json);
				break;
			default:
				if (net_ice_debug.ival)
					Con_Printf(S_COLOR_GRAY"[%s]: Broker send unknown packet: %i\n", b->ice?b->ice->friendlyname:"?", cmd);
				break;
			}
			break;
		}

		ofs+=len;
		b->in[ofs] = n;

	}

	if (ofs)
	{	//and eat the newly parsed data...
		b->insize -= ofs;
		memmove(b->in, b->in+ofs, b->insize);
	}

	FTENET_ICE_Flush(b);
	return false;
}
static void FTENET_ICE_PrintStatus(ftenet_generic_connection_t *gcon)
{
	ftenet_ice_connection_t *b = (void*)gcon;
	size_t c;

	if (b->ice)
		ICE_Debug(b->ice);
	if (b->numclients)
	{
		size_t activeice = 0;
		for (c = 0; c < b->numclients; c++)
			if (b->clients[c].ice)
			{
				activeice++;
				ICE_PrintSummary(b->clients[c].ice, b->generic.islisten);
			}
		Con_Printf("%u ICE connections\n", (unsigned)activeice);
	}
}
static int FTENET_ICE_GetLocalAddresses(struct ftenet_generic_connection_s *gcon, unsigned int *adrflags, netadr_t *addresses, const char **adrparms, int maxaddresses)
{
	ftenet_ice_connection_t *b = (void*)gcon;
	if (maxaddresses < 1)
		return 0;
	*addresses = b->brokeradr;
	*adrflags = 0;
	*adrparms = b->gamename;
	return 1;
}

static qboolean FTENET_ICE_ChangeLocalAddress(struct ftenet_generic_connection_s *gcon, const char *address, netadr_t *newadr)
{
	ftenet_ice_connection_t *b = (void*)gcon;
	netadr_t adr;
	const char *path;

	if (!NET_StringToAdr2(address, PORT_ICEBROKER, &adr, 1, &path))
		return true;	//err... something failed? don't break what works!

	if (!NET_CompareAdr(&adr, &b->brokeradr))
		return false;	//the broker changed! zomg! just kill it all!

	if (path && *path++=='/')
	{
		if (*path && strcmp(path, b->gamename))
			return false;	//it changed! and we care! break everything!
	}
	return true;
}

ftenet_generic_connection_t *FTENET_ICE_EstablishConnection(ftenet_connections_t *col, const char *address, netadr_t adr, const struct dtlspeercred_s *peerinfo)
{
	ftenet_ice_connection_t *newcon;
	const char *path;
	char *c;

	if (!NET_StringToAdr2(address, PORT_ICEBROKER, &adr, 1, &path))
		return NULL;
/*	if (adr.prot == NP_ICES)
		adr.prot = NP_TLS;
	else if (adr.prot == NP_ICE)
		adr.prot = NP_STREAM;
*/
	newcon = Z_Malloc(sizeof(*newcon));

	if (!strncmp(address, "ice://", 6)||!strncmp(address, "rtc://", 6))
		address+=6;
	else if (!strncmp(address, "ices://", 7)||!strncmp(address, "rtcs://", 7))
		address+=7;
	if (address == path && *path=='/')
	{
		if (!strncmp(net_ice_broker.string, "tls://", 6) || !strncmp(net_ice_broker.string, "tcp://", 6))
			Q_strncpyz(newcon->brokername, net_ice_broker.string+6, sizeof(newcon->brokername));	//name is for prints only.
		else
			Q_strncpyz(newcon->brokername, net_ice_broker.string, sizeof(newcon->brokername));	//name is for prints only.
		Q_strncpyz(newcon->gamename, path+1, sizeof(newcon->gamename));	//so we know what to tell the broker.
	}
	else
	{
		Q_strncpyz(newcon->brokername, address, sizeof(newcon->brokername));	//name is for prints only.
		if (path && *path == '/' && path-address < sizeof(newcon->brokername))
		{
			newcon->brokername[path-address] = 0;
			Q_strncpyz(newcon->gamename, path+1, sizeof(newcon->gamename));	//so we know what to tell the broker.
		}
		else
			*newcon->gamename = 0;
	}
	c = strchr(newcon->brokername, ':');
	if (c) *c = 0;

	newcon->brokeradr = adr;
	newcon->broker = NULL;
	newcon->timeout = realtime;
	newcon->heartbeat = realtime;
	newcon->nextping = realtime;
	newcon->generic.owner = col;
	newcon->generic.thesocket = INVALID_SOCKET;

	newcon->generic.addrtype[0] = NA_INVALID;
	newcon->generic.addrtype[1] = NA_INVALID;

	newcon->generic.GetPacket = FTENET_ICE_GetPacket;
	newcon->generic.SendPacket = FTENET_ICE_SendPacket;
	newcon->generic.Close = FTENET_ICE_Close;
	newcon->generic.PrintStatus = FTENET_ICE_PrintStatus;
	newcon->generic.GetLocalAddresses = FTENET_ICE_GetLocalAddresses;
	newcon->generic.ChangeLocalAddress = FTENET_ICE_ChangeLocalAddress;

	newcon->generic.islisten = col->islisten;

	return &newcon->generic;
}

void ICE_Init(void)
{
	Cvar_Register(&net_ice_exchangeprivateips, "networking");
	Cvar_Register(&net_ice_allowstun, "networking");
	Cvar_Register(&net_ice_allowturn, "networking");
	Cvar_Register(&net_ice_allowmdns, "networking");
	Cvar_Register(&net_ice_relayonly, "networking");
	Cvar_Register(&net_ice_usewebrtc, "networking");
	Cvar_Register(&net_ice_servers, "networking");
	Cvar_Register(&net_ice_debug,	"networking");
	Cmd_AddCommand("net_ice_show", ICE_Show_f);
}


#ifdef HAVE_SERVER
void SVC_ICE_Offer(void)
{	//handles an 'ice_offer' udp message from a broker
	extern cvar_t net_ice_servers;
	struct icestate_s *ice;
	static float throttletimer;
	const char *sdp, *s;
	char buf[1400];
	int sz;
	char *clientaddr = Cmd_Argv(1);	//so we can do ip bans on the client's srflx address
	char *brokerid = Cmd_Argv(2);	//specific id to identify the pairing on the broker.
	netadr_t adr;
	json_t *json;
	if (!sv.state)
		return;	//err..?
	if (net_from.prot != NP_DTLS && net_from.prot != NP_WSS && net_from.prot != NP_TLS)
	{	//a) dtls provides a challenge (ensuring we can at least ban them).
		//b) this contains the caller's ips. We'll be pinging them anyway, but hey. also it'll be too late at this point but it keeps the other side honest.
		Con_ThrottlePrintf(&throttletimer, 0, CON_WARNING"%s: ice handshake from %s was unencrypted\n", NET_AdrToString (buf, sizeof(buf), &net_from), clientaddr);
		return;
	}

	if (!NET_StringToAdr_NoDNS(clientaddr, 0, &adr))	//no dns-resolution denial-of-service attacks please.
	{
		Con_ThrottlePrintf(&throttletimer, 0, CON_WARNING"%s: ice handshake specifies bad client address: %s\n", NET_AdrToString (buf, sizeof(buf), &net_from), clientaddr);
		return;
	}
	if (SV_BannedReason(&adr)!=NULL)
	{
		Con_ThrottlePrintf(&throttletimer, 0, CON_WARNING"%s: ice handshake for %s - banned\n", NET_AdrToString (buf, sizeof(buf), &net_from), clientaddr);
		return;
	}

	ice = iceapi.Create(NULL, brokerid, clientaddr, ICEM_WEBRTC, ICEP_QWSERVER, false);
	if (!ice)
		return;	//some kind of error?!?
	//use the sender as a stun server. FIXME: put server's address in the request instead.
	iceapi.Set(ice, "server", va("stun:%s", NET_AdrToString (buf, sizeof(buf), &net_from)));	//the sender should be able to act as a stun server for use. should probably just pass who its sending to and call it a srflx anyway, tbh.

	s = net_ice_servers.string;
	while((s=COM_Parse(s)))
		iceapi.Set(ice, "server", com_token);

	sdp = MSG_ReadString();
	json = JSON_Parse(sdp);	//browsers are poo
	if (json)
		sdp = JSON_GetString(json, "sdp", buf,sizeof(buf), "");

	if (iceapi.Set(ice, "sdpoffer", sdp))
	{
		iceapi.Set(ice, "state", STRINGIFY(ICE_CONNECTING));	//skip gathering, just trickle.

		Q_snprintfz(buf, sizeof(buf), "\xff\xff\xff\xff""ice_answer %s", brokerid);
		sz = strlen(buf)+1;
		if (iceapi.Get(ice, "sdpanswer", buf+sz, sizeof(buf)-sz))
		{
			sz += strlen(buf+sz);

			NET_SendPacket(svs.sockets, sz, buf, &net_from);
		}
	}
	JSON_Destroy(json);

	//and because we won't have access to its broker, disconnect it from any persistent state to let it time out.
	iceapi.Close(ice, false);
}
void SVC_ICE_Candidate(void)
{	//handles an 'ice_ccand' udp message from a broker
	struct icestate_s *ice;
	json_t *json;
	const char *sdp;
	char buf[1400];
	char *brokerid = Cmd_Argv(1);	//specific id to identify the pairing on the broker.
	unsigned int seq = atoi(Cmd_Argv(2));	//their seq, to ack and prevent dupes
	unsigned int ack = atoi(Cmd_Argv(3));	//so we don't resend endlessly... *cough*
	if (net_from.prot != NP_DTLS && net_from.prot != NP_WSS && net_from.prot != NP_TLS)
	{
		return;
	}
	ice = iceapi.Find(NULL, brokerid);
	if (!ice)
		return;	//bad state. lost packet?

	//parse the inbound candidates
	for(;;)
	{
		sdp = MSG_ReadStringLine();
		if (msg_badread || !*sdp)
			break;
		if (seq++ < ice->u.inseq)
			continue;
		ice->u.inseq++;
		json = JSON_Parse(sdp);
		if (json)
		{
			sdp = buf;
			buf[0]='a';
			buf[1]='=';
			buf[2]=0;
			JSON_GetString(json, "candidate", buf+2,sizeof(buf)-2, NULL);
		}
		iceapi.Set(ice, "sdp", sdp);
		JSON_Destroy(json);
	}

	while (ack > ice->u.outseq)
	{	//drop an outgoing candidate line
		char *nl = strchr(ice->u.text, '\n');
		if (nl)
		{
			nl++;
			memmove(ice->u.text, nl, strlen(nl)+1);	//chop it away.
			ice->u.outseq++;
			continue;
		}
		//wut?
		if (ack > ice->u.outseq)
			ice->u.outseq = ack;	//a gap? oh noes!
		break;
	}

	//check for new candidates to include
	while (iceapi.GetLCandidateSDP(ice, buf, sizeof(buf)))
	{
		Z_StrCat(&ice->u.text, buf);
		Z_StrCat(&ice->u.text, "\n");
	}

	Q_snprintfz(buf, sizeof(buf), "\xff\xff\xff\xff""ice_scand %s %u %u\n%s", brokerid, ice->u.outseq, ice->u.inseq, ice->u.text?ice->u.text:"");
	NET_SendPacket(svs.sockets, strlen(buf), buf, &net_from);
}
#endif

#endif
