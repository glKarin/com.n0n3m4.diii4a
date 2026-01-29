#include "../plugin.h"
extern plugnetfuncs_t *netfuncs;
extern plugfsfuncs_t *filefuncs;

#include "xml.h"


//configuration

//#define NOICE	//if defined, we only do simple raw udp connections.
#define FILETRANSFERS		//IBB only, speeds suck. autoaccept is forced on. no protection from mods stuffcmding sendfile commands. needs more extensive testing
#define QUAKECONNECT		//including quake ICE connections (depends upon jingle)
#define VOIP				//enables voice chat (depends upon jingle)
//#define VOIP_LEGACY			//enables google-only voice chat compat. google have not kept up with the standardisation of jingle (aka: gingle).
//#define VOIP_LEGACY_ONLY	//disables jingle feature (advert+detection only)
#define JINGLE				//enables jingle signalling

#ifdef JINGLE
	#include "../../engine/common/netinc.h"
#else
	#undef VOIP
	#undef VOIP_LEGACY
#endif


#define JCL_BUILD "5"
//#define DEFAULTDOMAIN "triptohell.info"
#define DEFAULTRESOURCE "Quake"
#define QUAKEMEDIAXMLNS "http://fteqw.com/protocol/quake"
#define DISCONODE		"http://fteqw.com/ftexmpp"	//some sort of client identifier
#define DEFAULTICEMODE ICEM_ICE

#define MEDIATYPE_QUAKE "quake"
#define MEDIATYPE_VIDEO "video"
#define MEDIATYPE_AUDIO "audio"



#define JCL_MAXMSGLEN 0x10000

//values are not on the wire or anything
#define CAP_VOICE			(1u<<0)		//supports voice
#define CAP_GOOGLE_VOICE	(1u<<1)		//google supports some old non-standard version of jingle.
#define CAP_VIDEO			(1u<<2)		//supports video calls

#define CAP_GAMEINVITE		(1u<<3)		//supports game invites. custom/private protocol
#define CAP_POKE			(1u<<4)		//can be slapped.
#define CAP_SIFT			(1u<<5)		//non-jingle file transfers
#define	CAP_AVATARS			(1u<<6)		//can enable querying for user avatars, but cannot disable advertising our own.

//not actually capabilities, but just down to how we handle querying them.
#define CAP_QUERYING		(1u<<29)	//we've sent a query and are waiting for the response.
#define CAP_QUERIED			(1u<<30)	//feature capabilities are actually know.
#define CAP_QUERYFAILED		(1u<<31)	//caps request failed due to bad hash or some such.

//features that default to disabled.
#define CAP_DEFAULTENABLEDCAPS (CAP_VOICE/*|CAP_VIDEO*/|CAP_GAMEINVITE|CAP_POKE|CAP_AVATARS/*|CAP_SIFT*/|CAP_GOOGLE_VOICE)

typedef struct bresource_s
{
	char bstatus[128];	//basic status
	char fstatus[128];	//full status
	char server[256];
	int servertype;	//0=none, 1=already a client, 2=joinable
	int priority;

	unsigned int buggycaps;
	unsigned int caps;
	char *client_node;	//vendor name
	char *client_ver;	//cap hash value
	char *client_hash;	//cap hash method
	char *client_ext;	//deprecated. additionally queried

	struct bresource_s *next;

	char resource[1];
} bresource_t;
typedef struct buddy_s
{
	bresource_t *resources;
	bresource_t *defaultresource;	//this is the one that last replied (must be null for chatrooms, so we only talk to the room in general)
	bresource_t *ourselves;			//this is set back to ourselves when in a chatroom
	int defaulttimestamp;
	qboolean askfriend;
	qboolean friended;
	qboolean vcardphotochanged;
	enum {
		BT_UNKNOWN, //this buddy isn't known...
		BT_ROSTER,	//this is a friend! or at least on our list of potential friends anyway.
		BT_CHATROOM //we're treating this 'guy' as a MUC, each of their resources is a different person. which is weird.
	} btype;

	qboolean room_autojoin;
	char *room_nick;
	char *room_password;
	char *room_topic;

	char name[256];
	char vcardphotohash[41];
	qhandle_t image;

	struct buddy_s *next;
	char accountdomain[1];	//no resource on there
} buddy_t;
typedef struct jclient_s
{
	int accountnum;	//a private id to track which client links are associated with

	char redirserveraddr[64];	//if empty, do an srv lookup.

	enum
	{
		JCL_INACTIVE,	//not trying to connect.
		JCL_DEAD,		//not connected. connection died or something.
		JCL_AUTHING,	//connected, but not able to send any info on it other than to auth
		JCL_ACTIVE		//we're connected, we got a buddy list and everything
	} status;
	unsigned int timeout;		//reconnect/ping timer
	char errormsg[256];

	unsigned int enabledcapabilities;

	qhandle_t socket;

	qhandle_t rcon_pipe;	//contains console prints
	char rcon_peer[256];	//the name of the guy currently receiving console prints

	//we buffer output for times when the outgoing socket is full.
	//mostly this only happens at the start of the connection when the socket isn't actually open yet.
	char *outbuf;
	int outbufpos;
	int outbuflen;
	int outbufmax;

	char bufferedinmessage[JCL_MAXMSGLEN+1];	//servers are required to be able to handle messages no shorter than a specific size.
												//which means we need to be able to handle messages when they get to us.
												//servers can still handle larger messages if they choose, so this might not be enough.
	int bufferedinammount;

	char defaultdest[256];

	//config info
	char serveraddr[64];	//if empty, do an srv lookup.
	int serverport;
	char domain[256];
	char username[256];
	char resource[256];
	char certificatedomain[256];
	int forcetls;	//-1=off, 0=ifpossible, 1=fail if can't upgrade, 2=old-style tls
	qboolean savepassword;

	char fulljid[256];	//this is our full username@domain/resource string
	char barejid[256];	//this is our bare username@domain string
	char localalias[256];//this is what's shown infront of outgoing messages. >> by default until we can get our name.
	char vcardphotohash[20];	//20-byte sha1 hash.
	enum
	{
		VCP_UNKNOWN,
		VCP_NONE,
		VCP_KNOWN
	} vcardphotohashstatus;
	qboolean vcardphotohashchanged;	//forces a presence send.

	int instreampos;

	qboolean connecting; //still waiting for intial stream tag
	qboolean connected;	//fully on server and authed and everything.
	qboolean issecure;	//tls enabled (either upgraded or initially)
	int streamdebug;	//echo the stream to subconsoles

	qboolean preapproval;	//server supports presence preapproval

	char curquakeserver[2048];
	char defaultnamespace[2048];	//should be 'jabber:client' or blank (and spammy with all the extra xmlns attribs)

	struct sasl_ctx_s
	{
		char *username;	//might be different from the account name, but probably isn't.
		char *domain;	//might be different from the account domain, but probably isn't.
		qboolean issecure;	//tls enabled (either upgraded or initially)
		int socket;

		//this stuff should be saved
		char password_plain[256];		//plain password. scrubbed if we auth using a hashed auth.
		qbyte password_hash[256];		//safer password, not encrypted, but stored hashed.
		size_t password_hash_size;
		char password_validity[256];	//internal string used to check that the salt was unchanged

		struct saslmethod_s *authmethod;	//null name = oauth2->saslmethod
		qboolean allowauth_plainnontls;	//allow plain plain
		qboolean allowauth_plaintls;	//allow tls plain
		qboolean allowauth_digestmd5;	//allow digest-md5 auth
		qboolean allowauth_scramsha1;	//allow scram-sha-1 auth
		qboolean allowauth_oauth2;		//use oauth2 where possible

		struct
		{
			char authnonce[256];
		} digest;

		struct
		{
			qboolean plus;
			char authnonce[256];
			char authvhash[DIGEST_MAXSIZE];
			char authcbindtype[20];
			char authcbinding[256];
			hashfunc_t *hashfunc;
		} scram;

		struct
		{
			char saslmethod[64];
			char obtainurl[256];
			char refreshurl[256];
			char clientid[256];
			char clientsecret[256];
			char *useraccount;
			char *scope;
			char *accesstoken;	//one-shot access token
			char *refreshtoken;	//long-term token that we can use to get new access tokens
			char *authtoken;	//short-term authorisation token, usable to get an access token (and a refresh token if we're lucky)
		} oauth2;
	} sasl;

	struct iq_s
	{
		struct iq_s *next;
		char id[64];
		int timeout;
		qboolean (*callback) (struct jclient_s *jcl, struct subtree_s *tree, struct iq_s *iq);
		void *usrptr;
		char to[1];
	} *pendingiqs;

#ifdef JINGLE
	struct c2c_s
	{
		struct c2c_s *next;
		qboolean accepted;	//connection is going
		qboolean creator;	//true if we're the creator.
		unsigned int peercaps;
		qboolean displayed;	//temp flag for displaying jingle sessions with people that are not on our buddy list for whatever reasons

		struct
		{
			char name[64];	//uniquely identifies the content within the session.
			enum iceproto_e mediatype;
			enum icemode_e method;	//ICE_RAW or ICE_ICE. this is what the peer asked for. updated if we degrade it.
			struct icestate_s *ice;
			char *peeraddr;
			int peerport;
		} content[3];
		int contents;

		char *with;
		char sid[1];
	} *c2c;
#endif

#ifdef FILETRANSFERS
	struct ft_s
	{
		struct ft_s *next;
		char fname[MAX_QPATH];
		int size;
		int sizedone;
		char *with;
		char md5hash[16];
		int privateid;
		char iqid[64];
		char sid[64];
		int blocksize;
		unsigned short seq;
		qhandle_t file;
		qhandle_t stream;
		qboolean begun;	//handshake
		qboolean eof;
		qboolean transmitting;	//we're offering
		qboolean allowed;	//if false, don't handshake the transfer

		struct
		{
			char jid[128];
			char host[40];
			int port;
		} streamhosts[16];
		int nexthost;
		enum
		{
			STRM_IDLE,
			STRM_AUTH,
			STRM_AUTHED,
			STRM_ACTIVE
		} streamstatus;
		char indata[64];	//only for handshake data
		int inlen;

		enum
		{
			FT_NOTSTARTED,
			FT_IBB,			//in-band bytestreams
			FT_BYTESTREAM	//aka: relay
		} method;
	} *ft;
	int privateidseq;
#endif


	//persistant achived info about buddies, to avoid spamming the server every time we connect
	//such things might result in lag mid game!
	struct buddyinfo_s
	{
		struct buddyinfo_s *next;
		char *image;
		char *imagehash;
		char *imagemime;
		char accountdomain[1];
	} *buddyinfo;

	buddy_t *buddies;
	struct iq_s *avatarupdate;	//we only grab one buddy's photo at a time, this is to avoid too much spam.
} jclient_t;


#ifdef JINGLE
extern icefuncs_t *piceapi;
#endif


qboolean NET_DNSLookup_SRV(const char *host, char *out, int outlen);

//xmpp functionality
struct iq_s *JCL_SendIQNode(jclient_t *jcl, qboolean (*callback) (jclient_t *jcl, xmltree_t *tree, struct iq_s *iq), const char *iqtype, const char *target, xmltree_t *node, qboolean destroynode);
void JCL_AddClientMessagef(jclient_t *jcl, const char *fmt, ...);
void JCL_AddClientMessageString(jclient_t *jcl, const char *msg);
qboolean JCL_FindBuddy(jclient_t *jcl, const char *jid, buddy_t **buddy, bresource_t **bres, qboolean create);
void JCL_ForgetBuddy(jclient_t *jcl, buddy_t *buddy, bresource_t *bres);

//quake functionality
void JCL_GenLink(jclient_t *jcl, char *out, int outlen, const char *action, const char *context, const char *contextres, const char *sid, const char *txtfmt, ...);
void Con_SubPrintf(const char *subname, const char *format, ...);
void XMPP_ConversationPrintf(const char *context, const char *title, qboolean takefocus, char *format, ...);

//jingle functions
void JCL_Join(jclient_t *jcl, const char *target, const char *sid, qboolean allow, int protocol);
void JCL_JingleTimeouts(jclient_t *jcl, qboolean killall);
//jingle iq message handlers
qboolean JCL_HandleGoogleSession(jclient_t *jcl, xmltree_t *tree, const char *from, const char *id);
qboolean JCL_ParseJingle(jclient_t *jcl, xmltree_t *tree, const char *from, const char *id);

void XMPP_FT_AcceptFile(jclient_t *jcl, int fileid, qboolean accept);
qboolean XMPP_FT_OfferAcked(jclient_t *jcl, xmltree_t *x, struct iq_s *iq);
qboolean XMPP_FT_ParseIQSet(jclient_t *jcl, const char *iqfrom, const char *iqid, xmltree_t *tree);
void XMPP_FT_SendFile(jclient_t *jcl, const char *console, const char *to, const char *fname);
void XMPP_FT_Frame(jclient_t *jcl);

void Base64_Add(const char *s, int len);
char *Base64_Finish(void);
int Base64_Decode(char *out, int outlen, const char *src, int srclen);
