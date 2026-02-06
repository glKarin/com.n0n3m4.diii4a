//Released under the terms of the gpl as this file uses a bit of quake derived code. All sections of the like are marked as such

/*
Network limitations:
	googletalk:
		username: same as gmail (foobar@gmail.com).
		FIXME: need to test foobar@googlemail.com
		auth mechanism: oauth2(tls+nontls) or plain(tls-only). no digests supported, so mitm can easily grab your password if they use certificate authority hackery, so DO NOT log in from work.
		oauth2: I've registered a clientid for use with googletalk's network, but the whole web-browser-is-required crap makes it near unusable. We'll try it if they omit a password.
		otherwise a complete implementation.
		other users appear unresponsive and permanently away. this is a wtf on google's part and not something I can trivially work around. these people are really offline but have previously used 'google hangouts', and google insist on the UI nightmare infecting other clients too.
		appears to hack avatar vcards into all presence messages, which is just an interesting thing to note as it seems to keep fucking up resulting in extra queries for avatar images.

	facebook:
		username: foobar@chat.facebook.com
		auth mechanism: digest-md5, x-facebook-platform.
		gateway implementation: no arbitary iq support (no invite/join/voice).
		no roster control
		completely untested. I've no interest in signing up to be tracked constantly (but somehow google is okay... go figure... I guess I'm just trying to avoid a double-whammy)
		oauth2: no idea where to register a clientid, or what the correct addresses are. a google search implies they don't do refresh tokens properly. sticking with digest-md5 should work.
		*should* work for chat.

	msn:
		username: foobar@messenger.live.com (NOT foobar@live.com - this will timeout)
		auth mechansim: x-messenger-oath ONLY
		non-standard unusable crap.
		uses incorrect certificates. any client that doesn't warn about that is buggy as fuck.
		probably doesn't have iq support, no idea, can't log in to test that
		requires annoying see-other-host redirection.
		no roster control
		stun servers are listed in srv records for live.com and messanger.live.com but not messenger.live.com. retards.
		oauth2: too lazy to register a clientid. stupid crap. I hate having to register everywhere.

	ejabberd:
		auth mechanism: digest-md5, scram-sha1, plain.
		complete implementation. no issues.
		may be lacking srv entries, depends on installation.
		may have self-signed certificate issues, depends on installation.

client compat:
	hangouts:
		UI nightmare infects the entire network and thus other clients also.
		voip not supported. does not advertise any extensions and thus no voip.
		no file transfer support.

	googletalk:
		impossible to download from google any more. completely unsupported.
		implements old version of jingle. voice calls appear to not work.
		does not support SI file transfer.
		not tested by me.

	pidgin:
		(linux) has issues with jingle+ice, and can easily be made to crash. voip uses speex. pidgin's ice seems vulnerable to dropped packets.
		(windows) doesn't support voice calls
		file transfer works.
		otherwise works.
*/

#include "xmpp.h"
#include <time.h>

static plugsubconsolefuncs_t *confuncs;
static plug2dfuncs_t *drawfuncs;	//needed for avatars.
plugnetfuncs_t *netfuncs;
plugfsfuncs_t *filefuncs;
static plugclientfuncs_t *clientfuncs;	//for more accurate presence info.
static struct
{
	int width;
	int height;
} pvid;

//#define USE_GOOGLE_MAIL_NOTIFY

#ifdef DEFAULTDOMAIN
	#define EXAMPLEDOMAIN DEFAULTDOMAIN	//used in examples / default text field (but not otherwise assumed when omitted)
#else
	#define EXAMPLEDOMAIN "example.com"	//used in examples
#endif


#ifdef JINGLE
icefuncs_t *piceapi;
#endif
static qboolean jclient_needreadconfig;
static qboolean jclient_configdirty;
static qboolean jclient_updatebuddylist;
static jclient_t *jclient_action_cl;
static buddy_t *jclient_action_buddy;
static enum
{
	ACT_NONE,
	ACT_OAUTH,
	ACT_NEWACCOUNT,
	ACT_SETAUSERNAME,
	ACT_SETADOMAIN,
	ACT_SETASERVER,
	ACT_SETARESOURCE,
	ACT_SETAPASSWORD,
	ACT_ADDFRIEND,
	ACT_SETBALIAS,
} jclient_action;

#define BUDDYLISTTITLE "Buddy List"

#define COL_NAME_THEM	"^1" //red
#define COL_NAME_US		"^5" //cyan
#define COL_TEXT_THEM	"^7" //white
#define COL_TEXT_US		"^3" //yellow
#define IMG_FB_THEM "gfx/menudot1.lmp"
#define IMG_FB_US "gfx/menuplyr.lmp"

#define Q_strncpyz(o, i, l) do {strncpy(o, i, l-1);o[l-1]='\0';}while(0)

static qboolean (*Con_TrySubPrint)(const char *conname, const char *message);
qboolean Fallback_ConPrint(const char *conname, const char *message)
{
	plugfuncs->Print(message);
	return true;
}

void Con_SubPrintf(const char *subname, const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format,argptr);
	va_end (argptr);

	Con_TrySubPrint(subname, string);
}


//porting zone:
	#define COLOURWHITE "^7" // word
	#define COMMANDPREFIX "xmpp"
	#define COMMANDPREFIX2 "jab"
	#define COMMANDPREFIX3 "jabbercl"

	static char *JCL_ParseOut (char *data, char *buf, int bufsize)	//GPL: this is taken out of quake
	{
		int		c;
		int		len;

		len = 0;
		buf[0] = 0;

		if (!data)
			return NULL;

	// skip whitespace
		while ( (c = *data) <= ' ')
		{
			if (c == 0)
				return NULL;			// end of file;
			data++;
		}

	// handle quoted strings specially
		if (c == '\"')
		{
			data++;
			while (1)
			{
				if (len >= bufsize-1)
					return data;

				c = *data++;
				if (c=='\"' || !c)
				{
					buf[len] = 0;
					return data;
				}
				buf[len] = c;
				len++;
			}
		}

	// parse a regular word
		do
		{
			if (len >= bufsize-1)
				return data;

			buf[len] = c;
			data++;
			len++;
			c = *data;
		} while (c>32);

		buf[len] = 0;
		return data;
	}


char *JCL_Info_ValueForKey (char *s, const char *key, char *valuebuf, int valuelen)	//GPL: ripped from quake
{
	char	pkey[1024];
	char	*o;

	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				*valuebuf='\0';
				return valuebuf;
			}
			*o++ = *s++;
			if (o+2 >= pkey+sizeof(pkey))	//hrm. hackers at work..
			{
				*valuebuf='\0';
				return valuebuf;
			}
		}
		*o = 0;
		s++;

		o = valuebuf;

		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				*valuebuf='\0';
				return valuebuf;
			}
			*o++ = *s++;

			if (o+2 >= valuebuf+valuelen)	//hrm. hackers at work..
			{
				*valuebuf='\0';
				return valuebuf;
			}
		}
		*o = 0;

		if (!strcmp (key, pkey) )
			return valuebuf;

		if (!*s)
		{
			*valuebuf='\0';
			return valuebuf;
		}
		s++;
	}
}




/*
this is a fairly basic implementation.
don't expect it to do much.
You can probably get a better version from somewhere.
This has been tweaked to do xml markup in %s. Use %+s if you want to include xml-formatted text as-is.
*/
int Q_vsnprintxf(char *buffer, size_t maxlen, const char *format, va_list vargs)
{
	int tokens=0;
	char *string;
	char tempbuffer[64];
	char sign;
	unsigned int _uint;
	int _int;
	float _float;
	int i;
	int use0s;
	int width, useprepad, plus;
	int precision;

	if (!maxlen)
		return 0;
maxlen--;

	while(*format)
	{
		switch(*format)
		{
		case '%':
			plus = 0;
			width= 0;
			precision=-1;
			useprepad=0;
			use0s= 0;
retry:
			switch(*(++format))
			{
			case '-':
				useprepad=true;
				goto retry;
			case '+':
				plus = true;
				goto retry;
			case '.':
				precision = 0;
				while (format[1] >= '0' && format[1] <= '9')
					precision = precision*10+*++format-'0';
				goto retry;
			case '0':
				if (!width)
				{
					use0s=true;
					goto retry;
				}
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				width=width*10+*format-'0';
				goto retry;
			case '%':	/*emit a %*/
				if (maxlen-- == 0) 
					{*buffer++='\0';return tokens;}
				*buffer++ = *format;
				break;
			case 's':
				string = va_arg(vargs, char *);
				if (!string)
					string = "(null)";
				if (!plus)
				{
					while (*string)
					{
						char *rep;
						size_t replen;
						char c = *string++;
						switch(c)
						{
						default:
							if (maxlen-- == 0) 
								{*buffer++='\0';return tokens;}
							*buffer++ = c;
							continue;
						case '<': rep = "&lt;"; break;
						case '>': rep = "&gt;"; break;
						case '&': rep = "&amp;"; break;
						case '\'': rep = "&apos;"; break;
						case '\"': rep = "&quot;"; break;
						}
						replen = strlen(rep);
						if (maxlen < replen)
							{*buffer++='\0';return tokens;}
						maxlen -= replen;
						memcpy(buffer, rep, replen);
						buffer += replen;
					}
				}
				else if (width)
				{
					while (*string && width--)
					{
						if (maxlen-- == 0) 
							{*buffer++='\0';return tokens;}
						*buffer++ = *string++;
					}
				}
				else
				{
					while (*string)
					{
						if (maxlen-- == 0) 
							{*buffer++='\0';return tokens;}
						*buffer++ = *string++;
					}
				}
				tokens++;
				break;
			/*case 'c':
				_int = va_arg(vargs, int);
				if (maxlen-- == 0) 
					{*buffer++='\0';return tokens;}
				*buffer++ = _int;
				tokens++;
				break;*/
			case 'p':
				if (1)
				_uint = (size_t)va_arg(vargs, void*);
				else
			case 'x':
				_uint = va_arg(vargs, unsigned int);
				i = sizeof(tempbuffer)-2;
				tempbuffer[i+1] = '\0';
				while(_uint)
				{
					tempbuffer[i] = (_uint&0xf) + '0';
					if (tempbuffer[i] > '9')
						tempbuffer[i] = tempbuffer[i] - ':' + 'a';
					_uint/=16;
					i--;
				}
				string = tempbuffer+i+1;

				if (!*string)
				{
					i=61;
					string = tempbuffer+i+1;
					string[0] = '0';
					string[1] = '\0';
				}

				width -= 62-i;
				while (width>0)
				{
					string--;
					if (use0s)
						*string = '0';
					else
						*string = ' ';
					width--;
				}

				while (*string)
				{
					if (maxlen-- == 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}
				tokens++;
				break;
			case 'd':
			case 'u':
			case 'i':
				_int = va_arg(vargs, int);
				if (useprepad)
				{

				}
				if (_int < 0)
				{
					sign = '-';
					_int *= -1;
				}
				else if (plus)
					sign = '+';
				else
					sign = 0;
				i = sizeof(tempbuffer)-2;
				tempbuffer[sizeof(tempbuffer)-1] = '\0';
				while(_int)
				{
					tempbuffer[i--] = _int%10 + '0';
					_int/=10;
				}
				if (sign)
					tempbuffer[i--] = sign;
				string = tempbuffer+i+1;

				if (!*string)
				{
					i=61;
					string = tempbuffer+i+1;
					string[0] = '0';
					string[1] = '\0';
				}

				width -= 62-i;
/*				while (width>0)
				{
					string--;
					*string = ' ';
					width--;
				}
*/
				while(width>0)
				{
					if (maxlen-- == 0) 
						{*buffer++='\0';return tokens;}
					if (use0s)
						*buffer++ = '0';
					else
						*buffer++ = ' ';
					width--;
				}

				while (*string)
				{
					if (maxlen-- == 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}
				tokens++;
				break;
			case 'f':
				_float = (float)va_arg(vargs, double);

//integer part.
				_int = (int)_float;
				if (_int < 0)
				{
					if (maxlen-- == 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = '-';
					_int *= -1;
				}
				i = sizeof(tempbuffer)-2;
				tempbuffer[sizeof(tempbuffer)-1] = '\0';
				if (!_int)
				{
					tempbuffer[i--] = '0';
				}
				else
				{
					while(_int)
					{
						tempbuffer[i--] = _int%10 + '0';
						_int/=10;
					}
				}
				string = tempbuffer+i+1;
				while (*string)
				{
					if (maxlen-- == 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}

				_int = sizeof(tempbuffer)-2-i;

//floating point part.
				_float -= (int)_float;
				i = 0;
				tempbuffer[i++] = '.';
				if (precision < 0)
					precision = 7;
				while(_float - (int)_float)
				{
					if (i > precision)	//remove the excess presision.
						break;

					_float*=10;
					tempbuffer[i++] = (int)_float%10 + '0';
				}
				if (i == 1)	//no actual fractional part
				{
					tokens++;
					break;
				}

				//concatinate to our string
				tempbuffer[i] = '\0';
				string = tempbuffer;
				while (*string)
				{
					if (maxlen-- == 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}

				tokens++;
				break;
			default:
				string = "ERROR IN FORMAT";
				while (*string)
				{
					if (maxlen-- == 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}
				break;
			}
			break;
		default:
			if (maxlen-- == 0) 
				{*buffer++='\0';return tokens;}
			*buffer++ = *format;
			break;
		}
		format++;
	}
	{*buffer++='\0';return tokens;}
}




#if defined(_WIN32) && defined(HAVE_PACKET)
#include <windns.h>
static DNS_STATUS (WINAPI *pDnsQuery_UTF8) (PCSTR pszName, WORD wType, DWORD Options, PIP4_ARRAY aipServers, PDNS_RECORD *ppQueryResults, PVOID *pReserved);
static VOID (WINAPI *pDnsRecordListFree)(PDNS_RECORD pRecordList, DNS_FREE_TYPE FreeType);
static HMODULE dnsapi_lib;
qboolean NET_DNSLookup_SRV(const char *host, char *out, int outlen)
{
	DNS_RECORD *result = NULL;
	if (!dnsapi_lib)
	{
		dnsapi_lib = LoadLibrary("dnsapi.dll");
		pDnsQuery_UTF8 = (void*)GetProcAddress(dnsapi_lib, "DnsQuery_UTF8");
		pDnsRecordListFree = (void*)GetProcAddress(dnsapi_lib, "DnsRecordListFree");
	}
	//win98?
	if (!pDnsQuery_UTF8 || !pDnsRecordListFree)
		return false;
	//do lookup
	pDnsQuery_UTF8(host, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &result, NULL);
	if (result)
	{
		Q_snprintf(out, outlen, "[%s]:%i", result->Data.SRV.pNameTarget, result->Data.SRV.wPort);
		pDnsRecordListFree(result, DnsFreeRecordList);
		return true;
	}
	return false;
}
#elif (defined(__unix__) || defined(__MACH__) || defined(__linux__)) && !defined(ANDROID)
#include <resolv.h>
#include <arpa/nameser.h>
qboolean NET_DNSLookup_SRV(const char *host, char *out, int outlen)
{
	int questions;
	int answers;
	qbyte answer[512];
	qbyte dname[512];
	int len, i;
	qbyte *msg, *eom;

	len = res_query(host, C_IN, T_SRV, answer, sizeof(answer));
	if (len < 12)
	{
		Con_Printf("srv lookup failed for %s\n", host);
		return false;
	}

	eom = answer+len;

	questions = (answer[4]<<8) | answer[5];
	answers = (answer[6]<<8) | answer[7];
//	id @ 0
//	bits @ 2
//	questioncount@4
///	answer count@6
//	nameserver record count @8
//	additional record count @10

//	questions@12
//	answers@12+sizeof(questions)

	if (answers < 1)
		return false;

	msg = answer+12;

	while(questions --> 0)
	{
		dn_expand(answer, eom, msg, dname, sizeof(dname));
//		Con_Printf("Skip question %s\n", dname);
		i = dn_skipname(msg, eom);
		if (i <= 0)
			return false;
		msg += i;
		msg += 2;//query type
		msg += 2;//query class
	}

	while(answers --> 0)
	{
		i = dn_expand(answer, eom, msg, dname, sizeof(dname));
//		i = dn_skipname(msg, eom);
		msg += i;
		msg += 2;//query type
		msg += 2;//query class
		msg += 4;//ttl
		i = (msg[0]<<8) | msg[1];
		msg+=2;
		//noone tried to send the wrong type then, woo.
		if (!strcmp(dname, host))
		{
			int port;
			//we're not serving to other dns servers, and it seems they're already getting randomized, so just grab the first without rerandomizing.
			msg += 2;//priority
			msg += 2;//weight
			port = (msg[0]<<8) | msg[1];
			msg += 2;//port
			dn_expand(answer, eom, msg, dname, sizeof(dname));
			Q_snprintf(out, outlen, "[%s]:%i", dname, port);
//			Con_Printf("Resolved to %s\n", out);
			return true;
		}
		dn_expand(answer, eom, msg, out, outlen);
//		Con_Printf("Ignoring resolution to %s\n", out);
		msg += i;
	}

//type (2 octets)
//class (2 octets)
//TTL (4 octets)
//resource data length (2 octets)
//resource data (variable length)

	if (i < 0)
		return false;
	return true;
}
#else
qboolean NET_DNSLookup_SRV(const char *host, char *out, int outlen)
{
	return false;
}
#endif


static char base64[((4096+3)*4/3)+1];
static unsigned int base64_len;	//current output length
static unsigned int base64_cur;	//current pending value
static unsigned int base64_bits;//current pending bits
char Base64_From64(int byt)
{
	if (byt >= 0 && byt < 26)
		return 'A' + byt - 0;
	if (byt >= 26 && byt < 52)
		return 'a' + byt - 26;
	if (byt >= 52 && byt < 62)
		return '0' + byt - 52;
	if (byt == 62)
		return '+';
	if (byt == 63)
		return '/';
	return '!';
}
void Base64_Byte(unsigned int byt)
{
	if (base64_len+4>=sizeof(base64)-1)
		return;
	base64_cur |= byt<<(16-	base64_bits);//first byte fills highest bits
	base64_bits += 8;
	if (base64_bits == 24)
	{
		base64[base64_len++] = Base64_From64((base64_cur>>18)&63);
		base64[base64_len++] = Base64_From64((base64_cur>>12)&63);
		base64[base64_len++] = Base64_From64((base64_cur>>6)&63);
		base64[base64_len++] = Base64_From64((base64_cur>>0)&63);
		base64[base64_len] = '\0';
//		Con_Printf("base64: %s\n", base64+base64_len-4);
		base64_bits = 0;
		base64_cur = 0;
	}
}

void Base64_Add(const char *s, int len)
{
	const unsigned char *us = (const unsigned char *)s;
	while(len-->0)
		Base64_Byte(*us++);
}

char *Base64_Finish(void)
{
	//output is always a multiple of four

	//0(0)->0(0)
	//1(8)->2(12)
	//2(16)->3(18)
	//3(24)->4(24)

	if (base64_bits != 0)
	{
		base64[base64_len++]=Base64_From64((base64_cur>>18)&63);
		base64[base64_len++]=Base64_From64((base64_cur>>12)&63);
		if (base64_bits == 8)
		{
			base64[base64_len++]= '=';
			base64[base64_len++]= '=';
		}
		else
		{
			base64[base64_len++]=Base64_From64((base64_cur>>6)&63);
			if (base64_bits == 16)
				base64[base64_len++]= '=';
			else
				base64[base64_len++]=Base64_From64((base64_cur>>0)&63);
		}
	}
	base64[base64_len++] = '\0';

	base64_len = 0; //for next time (use strlen)
	base64_bits = 0;
	base64_cur = 0;

	return base64;
}

//decode a base64 byte to a 0-63 value. Cannot cope with =.
static int Base64_DecodeByte(char byt)
{
    if (byt >= 'A' && byt <= 'Z')
        return (byt-'A') + 0;
    if (byt >= 'a' && byt <= 'z')
        return (byt-'a') + 26;
    if (byt >= '0' && byt <= '9')
        return (byt-'0') + 52;
    if (byt == '+')
        return 62;
    if (byt == '/')
        return 63;
    return -1;
}
//FIXME: we should be able to skip whitespace.
int Base64_Decode(char *out, int outlen, const char *src, int srclen)
{
	int len = 0;
	int result;

	//4 input chars give 3 output chars
	while(srclen >= 4)
	{
		if (len+3 > outlen)
			break;
		result = Base64_DecodeByte(src[0])<<18;
		result |= Base64_DecodeByte(src[1])<<12;
		out[len++] = (result>>16)&0xff;
		if (src[2] != '=')
		{
			result |= Base64_DecodeByte(src[2])<<6;
			out[len++] = (result>>8)&0xff;
			if (src[3] != '=')
			{
				result |= Base64_DecodeByte(src[3])<<0;
				out[len++] = (result>>0)&0xff;
			}
		}
		if (result & 0xff000000)
			return 0;	//some kind of invalid char

		src += 4;
		srclen -= 4;
	}

	//some kind of error
	if (srclen)
		return 0;
	
	return len;
}



void XMPP_Menu_Password(jclient_t *acc);
void RenameConsole(char *totrim);
void JCL_Command(int accid, char *consolename);
void JCL_LoadConfig(void);
void JCL_WriteConfig(void);

static struct {
	char *names;
	unsigned int cap;
} capnames[] =
{
	{"avatars", CAP_AVATARS},
	{"jingle_voice", CAP_VOICE},
	{"jingle_video", CAP_VIDEO},
	{"google_voice", CAP_GOOGLE_VOICE},
	{"quake_invite", CAP_GAMEINVITE},
	{"poke", CAP_POKE},
#ifdef FILETRANSFERS
	{"si_filetransfer", CAP_SIFT},
#endif
	{NULL}
};

static void JCL_ExecuteCommand_f(void)
{
	qboolean isinsecure = cmdfuncs->IsInsecure();
	char cmd[256];
	cmdfuncs->Argv(0, cmd, sizeof(cmd));
	if (!strcmp(cmd, COMMANDPREFIX) || !strcmp(cmd, COMMANDPREFIX2) || !strcmp(cmd, COMMANDPREFIX3))
	{
		if (!isinsecure || cmdfuncs->Argc() == 1)
			JCL_Command(0, "");
	}
	else if (!strncmp(cmd, COMMANDPREFIX, strlen(COMMANDPREFIX)))
	{
		if (!isinsecure || cmdfuncs->Argc() == 1)
			JCL_Command(atoi(cmd+strlen(COMMANDPREFIX)), "");
	}
}

qboolean JCL_ConsoleLink(void);
qboolean JCL_ConsoleLinkMouseOver(float x, float y);
int JCL_ConExecuteCommand(qboolean isinsecure);

void JCL_Frame(double realtime, double gametime);
void JCL_Shutdown(void);

static void QDECL JCL_UpdateVideo(int width, int height, qboolean restarted)
{
	pvid.width = width;
	pvid.height = height;

	//FIXME: clear/reload images.
}

qboolean Plug_Init(void)
{
	const char *cmddesc = "XMPP client - ^[/"COMMANDPREFIX" /help^] for help.";
	jclient_needreadconfig = true;

	confuncs = (plugsubconsolefuncs_t*)plugfuncs->GetEngineInterface(plugsubconsolefuncs_name, sizeof(*confuncs));
	drawfuncs = (plug2dfuncs_t*)plugfuncs->GetEngineInterface(plug2dfuncs_name, sizeof(*drawfuncs));
	netfuncs = (plugnetfuncs_t*)plugfuncs->GetEngineInterface(plugnetfuncs_name, sizeof(*netfuncs));
	filefuncs = (plugfsfuncs_t*)plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	clientfuncs = (plugclientfuncs_t*)plugfuncs->GetEngineInterface(plugclientfuncs_name, sizeof(*clientfuncs));

	if (netfuncs && filefuncs &&
		plugfuncs->ExportFunction("Tick", JCL_Frame) &&
		plugfuncs->ExportFunction("Shutdown", JCL_Shutdown))
	{
		Con_Printf("XMPP Plugin Loaded. For help, use: ^[/"COMMANDPREFIX" /help^]\n");

		plugfuncs->ExportFunction("UpdateVideo", JCL_UpdateVideo);
		plugfuncs->ExportFunction("ConsoleLink", JCL_ConsoleLink);
		if (drawfuncs)
			plugfuncs->ExportFunction("ConsoleLinkMouseOver", JCL_ConsoleLinkMouseOver);

		if (!confuncs || !plugfuncs->ExportFunction("ConExecuteCommand", JCL_ConExecuteCommand))
		{
			Con_Printf("XMPP plugin in single-console mode\n");
			Con_TrySubPrint = Fallback_ConPrint;
		}
		else
			Con_TrySubPrint = confuncs->SubPrint;

		cmdfuncs->AddCommand(COMMANDPREFIX, JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX2, JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX3, JCL_ExecuteCommand_f, cmddesc);

		cmdfuncs->AddCommand(COMMANDPREFIX"0", JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX"1", JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX"2", JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX"3", JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX"4", JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX"5", JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX"6", JCL_ExecuteCommand_f, cmddesc);
		cmdfuncs->AddCommand(COMMANDPREFIX"7", JCL_ExecuteCommand_f, cmddesc);

		//flags&1 == archive
		cvarfuncs->GetNVFDG("xmpp_nostatus",			"0", 0, NULL, "xmpp");
		cvarfuncs->GetNVFDG("xmpp_showstatusupdates",	"0", 0, NULL, "xmpp");
		cvarfuncs->GetNVFDG("xmpp_autoacceptjoins",		"0", 0, NULL, "xmpp");
		cvarfuncs->GetNVFDG("xmpp_autoacceptinvites",	"0", 0, NULL, "xmpp");
		cvarfuncs->GetNVFDG("xmpp_autoacceptvoice",		"0", 0, NULL, "xmpp");
		cvarfuncs->GetNVFDG("xmpp_debug",				"0", 0, NULL, "xmpp");

#ifdef JINGLE
		piceapi = plugfuncs->GetEngineInterface(ICE_API_CURRENT, sizeof(*piceapi));
#endif

		return 1;
	}
	else
		Con_Printf("JCL Client Plugin failed\n");
	return 0;
}










//\r\n is used to end a line.
//meaning \0s are valid.
//but never used cos it breaks strings

static jclient_t *jclients[8];
static int jclient_curtime;
static int jclient_poketime;

typedef struct saslmethod_s
{
	char *method;
	int (*sasl_initial)(struct sasl_ctx_s *ctx, char *buf, int bufsize);
	int (*sasl_challenge)(struct sasl_ctx_s *ctx, char *inbuf, int insize, char *outbuf, int outsize);
	int (*sasl_success)(struct sasl_ctx_s *ctx, char *inbuf, int insize);
} saslmethod_t;

//#define OAUTH_CLIENT_ID_MSN "0"
#ifdef OAUTH_CLIENT_ID_MSN
static int sasl_plain_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	//"https://oauth.live.com/authorize?client_id=" OAUTH_CLIENT_ID_MSN "&scope=wl.messenger,wl.basic,wl.offline_access,wl.contacts_create,wl.share&response_type=token&redirect_uri=http://localhost/";
}
#endif

//\0username\0password
static int sasl_plain_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	int len = 0;

	if (ctx->issecure?ctx->allowauth_plaintls:ctx->allowauth_plainnontls)
	{
		if (!*ctx->password_plain)
			return -2;

		//realm isn't specified
		buf[len++] = 0;
		memcpy(buf+len, ctx->username, strlen(ctx->username));
		len += strlen(ctx->username);
		buf[len++] = 0;
		memcpy(buf+len, ctx->password_plain, strlen(ctx->password_plain));
		len += strlen(ctx->password_plain);

		return len;
	}
	return -1;
}


static int saslattr(char *out, int outlen, char *srcbuf, int srclen, char *arg)
{
	char *vn;
	char *s = srcbuf;
	char *e = s + srclen;
	char *vs;
	while(s < e)
	{
		while (s < e && *s == ',')
			s++;
		vn = s;
		s++;
		while (s < e && *s >= 'a' && *s <= 'z')
			s++;
		if (*s == '=')
		{
			vs = ++s;
			if (*s == '\"')
			{
				vs = ++s;
				while (s < e && *s != '\"')
					s++;
				outlen = s - vs;
				s++;
			}
			else
			{
				while (s < e && *s != ',')
					s++;
				outlen = s - vs;
			}

			if (!strncmp(vn, arg, strlen(arg)) && vn[strlen(arg)] == '=')
			{
				memcpy(out, vs, outlen);
				out[outlen] = 0;
				return outlen;
			}
		}
	}
	out[0] = 0;
	return 0;
}

static int sasl_digestmd5_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	if (ctx->allowauth_digestmd5)
	{
		if (!*ctx->password_plain && ctx->password_hash_size != 16)
			return -2;

		//FIXME: randomize the cnonce and check the auth key
		//although really I'm not entirely sure what the point is.
		//if we just authenticated with a mitm attacker relay, we're screwed either way.
		strcpy(ctx->digest.authnonce, "abcdefghijklmnopqrstuvwxyz");
		//nothing. server does the initial data.
		return 0;
	}
	return -1;
}
char *MD5_ToHex(char *input, int inputlen, char *ret, int retlen);
char *MD5_ToBinary(char *input, int inputlen, char *ret, int retlen);
static int sasl_digestmd5_challenge(struct sasl_ctx_s *ctx, char *in, int inlen, char *out, int outlen)
{
	char *username = ctx->username;
	char *cnonce = ctx->digest.authnonce;
	char rspauth[512];
	char realm[512];
	char nonce[512];
	char qop[512];
	char charset[512];
	char algorithm[512];
	char X[512];
	char Y[33];
	char A1[512];
	char A2[512];
	char HA1[33];
	char HA2[33];
	char KD[512];
	char Z[33];
	char *nc = "00000001";
	char digesturi[512];
	char *authzid = "";

	saslattr(rspauth, sizeof(rspauth), in, inlen, "rspauth");
	if (*rspauth)
	{
		return 0;	//we don't actually send any data back, just an xml 'response' tag to tell the server that we accept it.
	}

	saslattr(realm, sizeof(realm), in, inlen, "realm");
	saslattr(nonce, sizeof(nonce), in, inlen, "nonce");
	saslattr(qop, sizeof(qop), in, inlen, "qop");
	saslattr(charset, sizeof(charset), in, inlen, "charset");
	saslattr(algorithm, sizeof(algorithm), in, inlen, "algorithm");

	if (!*realm)
		Q_strlcpy(realm, ctx->domain, sizeof(realm));
	if (Q_snprintfz(digesturi, sizeof(digesturi), "xmpp/%s", realm))
		return -1;


	if (Q_snprintfz(X, sizeof(X), "%s:%s:", username, realm))
		return -1;
	if (ctx->password_hash_size == 16 && !strcmp(X, ctx->password_validity))
		memcpy(Y, ctx->password_hash, 16);	//use the hashed password, instead of the (missing) plain one
	else if (*ctx->password_plain)
	{
		Q_strlcpy(ctx->password_validity, X, sizeof(ctx->password_validity));

		if (Q_snprintfz(X, sizeof(X), "%s:%s:%s", username, realm, ctx->password_plain))
			return -1;
		MD5_ToBinary(X, strlen(X), Y, sizeof(Y));

		ctx->password_hash_size = 16;
		memcpy(ctx->password_hash, Y, 16);	//save that hash for later.
	}
	else
		return -1;	//err, we didn't have a password...

	memcpy(A1, Y, 16);
	if (*authzid)
	{
		if (Q_snprintfz(A1+16, sizeof(A1)-16, ":%s:%s:%s", nonce, cnonce, authzid))
			return -1;
	}
	else
	{
		if (Q_snprintfz(A1+16, sizeof(A1)-16, ":%s:%s", nonce, cnonce))
			return -1;
	}
	if (Q_snprintfz(A2, sizeof(A2), "%s:%s", "AUTHENTICATE", digesturi))
		return -1;
	MD5_ToHex(A1, strlen(A1+16)+16, HA1, sizeof(HA1));
	MD5_ToHex(A2, strlen(A2), HA2, sizeof(HA2));
	if (Q_snprintfz(KD, sizeof(KD), "%s:%s:%s:%s:%s:%s", HA1, nonce, nc, cnonce, qop, HA2))
		return -1;
	MD5_ToHex(KD, strlen(KD), Z, sizeof(Z));

	if (*authzid)
		Q_snprintf(out, outlen, "username=\"%s\",realm=\"%s\",nonce=\"%s\",cnonce=\"%s\",nc=\"%s\",qop=\"%s\",digest-uri=\"%s\",response=\"%s\",charset=\"%s\",authzid=\"%s\"",
					username, realm, nonce, cnonce, nc, qop, digesturi, Z, charset, authzid);
	else
		Q_snprintf(out, outlen, "username=\"%s\",realm=\"%s\",nonce=\"%s\",cnonce=\"%s\",nc=\"%s\",qop=\"%s\",digest-uri=\"%s\",response=\"%s\",charset=\"%s\"",
					username, realm, nonce, cnonce, nc, qop, digesturi, Z, charset);

	return strlen(out);
}

typedef struct
{
	int len;
	char buf[512];
} buf_t;
static int sasl_scram_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize, hashfunc_t *hashfunc, qboolean plus)
{
	if (ctx->allowauth_scramsha1)
	{
		unsigned int t;
		buf_t bindingdata;
		int bs;
		if (!*ctx->password_plain && ctx->password_hash_size != hashfunc->digestsize)
			return -2;

#if 1//FIXME: test that we're compliant when we're verifying channel bindings.
		//the channel binding thing helps to avoid MITM attacks by tying the auth to the tls keys.
		//a MITM attacker will have different binding ids each side, resulting in screwed hashes (they would need to use the public keys (which would require knowing private keys too), or something).
		if (ctx->issecure)
		{
			bindingdata.len = sizeof(bindingdata.buf);
			if (netfuncs)
				bs = netfuncs->GetTLSBinding(ctx->socket, bindingdata.buf, &bindingdata.len);
			else
				bs = -1;
		}
		else
#endif
		{
			bindingdata.len = 0;
			bs = -1;
		}
		//couldn't use plus for some reason
		if (bs < 0)
		{	//not implemented by our tls implementation.
			if (plus)
				return -1;	//can't auth with this mechanism
			Q_strlcpy(ctx->scram.authcbindtype, "n", sizeof(ctx->scram.authcbindtype));
		}
		else if (plus && bs > 0)
		{	//both sides should support it.
			Q_strlcpy(ctx->scram.authcbindtype, "p=tls-unique", sizeof(ctx->scram.authcbindtype));
			Base64_Add(bindingdata.buf, bindingdata.len);
			Q_snprintf(ctx->scram.authcbinding, sizeof(ctx->scram.authcbinding), "%s", Base64_Finish());
		}
		else
		{	//we support it, but the server does not appear to (we failed the plus method, or it just wasn't listed).
			//server will fail the auth if it can do channel bindings (assuming someone stripped the plus method).
			Q_strlcpy(ctx->scram.authcbindtype, "y", sizeof(ctx->scram.authcbindtype));
		}

		//FIXME: this should be more random, to validate that the server actually knows our password too
		//we can't really do anything until we've already signed in, so why bother?
		t = plugfuncs->GetMilliseconds();
		Base64_Add((void*)&t, sizeof(t));
		Base64_Add("0123456789abcdef", 16);
		Base64_Add((void*)&jclient_curtime, sizeof(jclient_curtime));
		strcpy(ctx->scram.authnonce, Base64_Finish());
		ctx->scram.hashfunc = hashfunc;

		Q_snprintf(buf, bufsize, "%s,,n=%s,r=%s", ctx->scram.authcbindtype, ctx->username, ctx->scram.authnonce);
		return strlen(buf);
	}
	return -1;
}
static int sasl_scramsha1minus_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	return sasl_scram_initial(ctx, buf, bufsize, &hash_sha1, false);
}
static int sasl_scramsha256minus_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	return sasl_scram_initial(ctx, buf, bufsize, &hash_sha2_256, false);
}
static int sasl_scramsha512minus_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	return sasl_scram_initial(ctx, buf, bufsize, &hash_sha2_512, false);
}
static int sasl_scramsha1plus_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	return sasl_scram_initial(ctx, buf, bufsize, &hash_sha1, true);
}
static int sasl_scramsha256plus_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	return sasl_scram_initial(ctx, buf, bufsize, &hash_sha2_256, true);
}
static int sasl_scramsha512plus_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	return sasl_scram_initial(ctx, buf, bufsize, &hash_sha2_512, true);
}

static void buf_cat(buf_t *buf, char *data, int len)
{
	memcpy(buf->buf + buf->len, data, len);
	buf->len += len;
	buf->buf[buf->len] = 0;
}
static void buf_cats(buf_t *buf, char *data)
{
	buf_cat(buf, data, strlen(data));
}
static size_t HMAC_Hi(hashfunc_t *hashfunc, char *out, char *password, int passwordlen, buf_t *salt, int times)
{
	size_t digestsize;
	char prev[64];
	int i, j;

	//first iteration is special
	buf_cat(salt, "\0\0\0\1", 4);
	digestsize = CalcHMAC(hashfunc, prev, sizeof(prev), salt->buf, salt->len, password, passwordlen);
	memcpy(out, prev, digestsize);
	
	//later iterations just use the previous iteration
	for (i = 1; i < times; i++)
	{
		CalcHMAC(hashfunc, prev, digestsize, prev, digestsize, password, passwordlen);

		for (j = 0; j < digestsize; j++)
			out[j] ^= prev[j];
	}
	return digestsize;
}
static int sasl_scram_challenge(struct sasl_ctx_s *ctx, char *in, int inlen, char *out, int outlen)
{
	hashfunc_t *func = ctx->scram.hashfunc;
	size_t digestsize = func->digestsize;
	//sasl SCRAM-SHA-1 challenge
	//send back the same 'r' attribute
	buf_t saslchal;
	int i;
	buf_t salt;
	buf_t csn;
	buf_t itr;
	buf_t final;
	buf_t sigkey;
	unsigned char salted_password[DIGEST_MAXSIZE];
	unsigned char proof[DIGEST_MAXSIZE];
	unsigned char clientkey[DIGEST_MAXSIZE];
	unsigned char serverkey[DIGEST_MAXSIZE];
	unsigned char storedkey[DIGEST_MAXSIZE];
	unsigned char clientsignature[DIGEST_MAXSIZE];
	char *username = ctx->username;
	char validationstr[256];
	const unsigned char *tmp;

	if (digestsize > DIGEST_MAXSIZE)
		return -1;

	saslchal.len = 0;
	buf_cat(&saslchal, in, inlen);
	
	//be warned, these CAN contain nulls.
	csn.len = saslattr(csn.buf, sizeof(csn.buf), saslchal.buf, saslchal.len, "r");
	salt.len = saslattr(salt.buf, sizeof(salt.buf), saslchal.buf, saslchal.len, "s");
	itr.len = saslattr(itr.buf, sizeof(itr.buf), saslchal.buf, saslchal.len, "i");

	salt.len = Base64_Decode(salt.buf, sizeof(salt.buf), salt.buf, salt.len);

	//csn MUST be prefixed with out authnone, to avoid mess.
	if (strncmp(csn.buf, ctx->scram.authnonce, strlen(ctx->scram.authnonce)))
		return -1;

	//this is the first part of the message we're about to send, with no proof.
	//c(channel) is mandatory but nulled and forms part of the hash
	final.len = 0;
	buf_cats(&final, "c=");
	Base64_Add(ctx->scram.authcbindtype, strlen(ctx->scram.authcbindtype));
	Base64_Add(",,", 2);
	Base64_Add(ctx->scram.authcbinding, strlen(ctx->scram.authcbinding));
	buf_cats(&final, Base64_Finish());
	buf_cats(&final, ",r=");
	buf_cat(&final, csn.buf, csn.len);

	//our original message + ',' + challenge + ',' + the message we're about to send.
	sigkey.len = 0;
	buf_cats(&sigkey, "n=");
	buf_cats(&sigkey, username);
	buf_cats(&sigkey, ",r=");
	buf_cats(&sigkey, ctx->scram.authnonce);
	buf_cats(&sigkey, ",");
	buf_cat(&sigkey, saslchal.buf, saslchal.len);
	buf_cats(&sigkey, ",");
	buf_cat(&sigkey, final.buf, final.len);

	Base64_Add(salt.buf, salt.len);
	Q_snprintf(validationstr, sizeof(validationstr), "%i:%s", atoi(itr.buf), Base64_Finish());
	if (ctx->password_hash_size == digestsize && !strcmp(validationstr, ctx->password_validity))
	{
		//restore the hash
		memcpy(salted_password, ctx->password_hash, digestsize);
	}
	else if (*ctx->password_plain)
	{
		ctx->password_hash_size = HMAC_Hi(func, salted_password, ctx->password_plain, strlen(ctx->password_plain), &salt, atoi(itr.buf));

		//save that hash for later.
		Q_strlcpy(ctx->password_validity, validationstr, sizeof(ctx->password_validity));
		memcpy(ctx->password_hash, salted_password, ctx->password_hash_size);
	}
	else
		return -2;	//panic. password not known any more. the server should not be changing salt/itr.

	CalcHMAC(func, clientkey, digestsize, "Client Key", strlen("Client Key"), salted_password, digestsize);
//Note: if we wanted to be fancy, we could store both clientkey and serverkey instead of salted_password, but I'm not sure there's all that much point.
	tmp = clientkey;
	CalcHash(func, storedkey, digestsize, tmp, digestsize);
	CalcHMAC(func, clientsignature, digestsize, sigkey.buf, sigkey.len, storedkey, digestsize);

	for (i = 0; i < digestsize; i++)
		proof[i] = clientkey[i] ^ clientsignature[i];

	Base64_Add(proof, digestsize);
	Base64_Finish();

	//to validate the server...
	CalcHMAC(func, serverkey, digestsize, "Server Key", strlen("Server Key"), salted_password, digestsize);
	CalcHMAC(func, ctx->scram.authvhash, digestsize, sigkey.buf, sigkey.len, serverkey, digestsize); //aka:serversignature

	//"c=biws,r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,p=v0X8v3Bz2T0CJGbJQyF0X+HI4Ts="
	Q_snprintf(out, outlen, "%s,p=%s", final.buf, base64);
	return strlen(out);
}

static int sasl_scram_final(struct sasl_ctx_s *ctx, char *in, int inlen)
{
	buf_t valid;

	valid.len = saslattr(valid.buf, sizeof(valid.buf), in, inlen, "v");
	valid.len = Base64_Decode(valid.buf, sizeof(valid.buf), valid.buf, valid.len);
	if (valid.len != ctx->scram.hashfunc->digestsize || memcmp(ctx->scram.authvhash, valid.buf, valid.len))
		return -1;	//server didn't give us the right answer. this is NOT the server we're looking for. give up now.
	return true;
}

void URL_Split(char *url, char *proto, int protosize, char *host, int hostsize, char *res, int ressize)
{
	char *s;
	*proto = 0;
	*host = 0;
	*res = 0;
	s = strchr(url, ':');
	if (!s)
		return;
	protosize = min(protosize-1, s-url);
	memcpy(proto, url, protosize);
	proto[protosize] = 0;
	s++;
	if (s[0] == '/' && s[1] == '/')
		s+=2;
	url = s;
	s = strchr(url, '/');
	if (!s)
		s = url+strlen(url);
	hostsize = min(hostsize-1, s-url);
	memcpy(host, url, hostsize);
	host[hostsize] = 0;
	url = s;
	s = url+strlen(url);
	ressize = min(ressize-1, s-url);
	memcpy(res, url, ressize);
	res[ressize] = 0;
}
void Q_strlcat_urlencode(char *d, const char *s, int n)
{
	char hex[16] = "0123456789ABCDEF";
	int clen = strlen(d);
	d += clen;
	n -= clen;
	n--;
	if (s)
	while (*s)
	{
		if ((*s >= '0' && *s <= '9') ||
			(*s >= 'a' && *s <= 'z') ||
			(*s >= 'A' && *s <= 'Z') ||
			*s == '-' || *s == '_' || *s == '.')
		{
			if (!n)
				break;
			n--;
			*d++ = *s++;
		}
		else
		{
			if (n < 3)
				break;
			n -= 3;
			*d++ = '%';
			*d++ = hex[*s>>4];
			*d++ = hex[*s&15];
			s++;
		}
	}
	*d = 0;
}
static int sasl_oauth2_initial(struct sasl_ctx_s *ctx, char *buf, int bufsize)
{
	char proto[256];
	char host[256];
	char resource[256];
	int sock, l, rl=0;
	char result[8192];

	xmltree_t *x;

	if (*ctx->password_plain)
		return -1;

	if (0)//*jcl->password_plain)
	{
		char body[4096];
		char header[4096];

		URL_Split(ctx->oauth2.refreshurl, proto, sizeof(proto), host, sizeof(host), resource, sizeof(resource));

		*body = 0;
		Q_strlcat(body, "client_id=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.clientid, sizeof(body));
		Q_strlcat(body, "&client_secret=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.clientsecret, sizeof(body));
		Q_strlcat(body, "&", sizeof(body));

		Q_strlcat(body, "grant_type=password&username=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.useraccount, sizeof(body));
		Q_strlcat(body, "&password=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->password_plain, sizeof(body));

		Q_strlcat(body, "&response_type=code", sizeof(body));

		Q_strlcat(body, "&redirect_uri=", sizeof(body));
		Q_strlcat_urlencode(body, "urn:ietf:wg:oauth:2.0:oob", sizeof(body));

		Q_strlcat(body, "&scope=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.scope, sizeof(body));

		Q_snprintf(header, sizeof(header),
			"POST %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			//"Authorization: Basic %s\r\n"
			"Content-length: %u\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"Connection: close\r\n"
			"\r\n",
				resource,
				host,
				(unsigned)strlen(body));

		sock = netfuncs->TCPConnect(host, 443);
		if (netfuncs->SetTLSClient(sock, host) >= 0)
		{
			netfuncs->Send(sock, header, strlen(header));
			netfuncs->Send(sock, body, strlen(body));
			while(1)
			{
				//FIXME: infinite loop!
				l = netfuncs->Recv(sock, result+rl, sizeof(result)-rl);
				if (l < 0)
					break;
				else
					rl += l;
			}
		}
		netfuncs->Close(sock);
		result[rl] = 0;

		Con_Printf("Got %s\n", result);
	}
	
	//if we have nothing, load up a browser to ask for the first token
	if (!*ctx->oauth2.refreshtoken && !*ctx->oauth2.authtoken)
	{
		char url[4096];
		*url = 0;
		Q_strlcat(url, ctx->oauth2.obtainurl, sizeof(url));
		Q_strlcat(url, "?redirect_uri=", sizeof(url));
		Q_strlcat_urlencode(url, "urn:ietf:wg:oauth:2.0:oob", sizeof(url));
		Q_strlcat(url, "&%72esponse_type=code&client_id=", sizeof(url));	//%72 = r. fucking ezquake colour codes. works with firefox anyway. no idea if that's the server changing it to an r or not. :s
		Q_strlcat_urlencode(url, ctx->oauth2.clientid, sizeof(url));
		Q_strlcat(url, "&scope=", sizeof(url));
		Q_strlcat_urlencode(url, ctx->oauth2.scope, sizeof(url));
		Q_strlcat(url, "&access_type=offline", sizeof(url));
		Q_strlcat(url, "&login_hint=", sizeof(url));
		Q_strlcat_urlencode(url, ctx->oauth2.useraccount, sizeof(url));

//		Con_Printf("Please visit ^[^4%s\\url\\%s^] and then enter:\n^[/"COMMANDPREFIX"%i /oa2token <TOKEN>^]\nNote: you can right-click the link to copy it to your browser, and you can use ctrl+v to paste the resulting auth token as part of the given command.\n", url, url, jcl->accountnum);

		//wait for user to act.
		return -2;
	}

	//refresh token is not known, try and get one
	if (!*ctx->oauth2.refreshtoken && *ctx->oauth2.authtoken)
	{
		xmltree_t *x;
		char body[4096];
		char header[4096];

		//send a refresh request

		*body = 0;
		Q_strlcat(body, "code=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.authtoken, sizeof(body));
		Q_strlcat(body, "&client_id=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.clientid, sizeof(body));
		Q_strlcat(body, "&client_secret=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.clientsecret, sizeof(body));
		Q_strlcat(body, "&redirect_uri=", sizeof(body));
		Q_strlcat_urlencode(body, "urn:ietf:wg:oauth:2.0:oob", sizeof(body));
		Q_strlcat(body, "&grant_type=", sizeof(body));
		Q_strlcat_urlencode(body, "authorization_code", sizeof(body));
		URL_Split(ctx->oauth2.refreshurl, proto, sizeof(proto), host, sizeof(host), resource, sizeof(resource));

		Q_snprintf(header, sizeof(header),
			"POST %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			//"Authorization: Basic %s\r\n"
			"Content-length: %u\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"user-agent: fteqw-plugin-xmpp\r\n"
			"Connection: close\r\n"
			"\r\n",
			resource, host, (unsigned)strlen(body));

		Con_Printf("XMPP: Requesting access token\n");
		sock = netfuncs->TCPConnect(host, 443);
		if (netfuncs->SetTLSClient(sock, host) >= 0)
		{
			netfuncs->Send(sock, header, strlen(header));
			netfuncs->Send(sock, body, strlen(body));
			while(1)
			{
				//FIXME: infinite loop!
				l = netfuncs->Recv(sock, result+rl, sizeof(result)-rl);
				if (l < 0)
					break;
				else
					rl += l;
			}
		}
		result[rl] = 0;
		netfuncs->Close(sock);

		//should contain something like:
		//{
		//"access_token" : "ya29.AHES6ZR-_Sx0UpexZdgqQwR8LFqTx-GFi-Zrq4nKrcLLA98N7g",
		//"token_type" : "Bearer",
		//"expires_in" : 3600
		//}

		l = strstr(result, "\r\n\r\n")-result;
		l+= 4;
		if (l < 0 || l > rl)
			l = rl;
		x = XML_FromJSON(NULL, "oauth2", result, &l, rl);
		XML_ConPrintTree(x, "", 1);
		free(ctx->oauth2.accesstoken);
		free(ctx->oauth2.refreshtoken);
		ctx->oauth2.accesstoken = strdup(XML_GetChildBody(x, "access_token", ""));
//		ctx->oauth2.token_type = strdup(XML_GetChildBody(x, "token_type", ""));
//		ctx->oauth2.expires_in = strdup(XML_GetChildBody(x, "expires_in", ""));
		ctx->oauth2.refreshtoken = strdup(XML_GetChildBody(x, "refresh_token", ""));

		//in theory, the auth token is no longer valid/needed
		free(ctx->oauth2.authtoken);
		ctx->oauth2.authtoken = strdup("");
	}

	//refresh our refresh token, obtaining a usable sign-in token at the same time.
	else if (!*ctx->oauth2.accesstoken)
	{
		char body[4096];
		char header[4096];
		const char *newrefresh;

		//send a refresh request

		*body = 0;
		Q_strlcat(body, "client_id=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.clientid, sizeof(body));
		Q_strlcat(body, "&client_secret=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.clientsecret, sizeof(body));
		Q_strlcat(body, "&grant_type=", sizeof(body));
		Q_strlcat_urlencode(body, "refresh_token", sizeof(body));
		Q_strlcat(body, "&refresh_token=", sizeof(body));
		Q_strlcat_urlencode(body, ctx->oauth2.refreshtoken, sizeof(body));
		URL_Split(ctx->oauth2.refreshurl, proto, sizeof(proto), host, sizeof(host), resource, sizeof(resource));

		Q_snprintf(header, sizeof(header),
			"POST %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			//"Authorization: Basic %s\r\n"
			"Content-length: %u\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"user-agent: fteqw-plugin-xmpp\r\n"
			"Connection: close\r\n"
			"\r\n",
			resource, host, (unsigned)strlen(body));

		Con_Printf("XMPP: Refreshing access token\n");
		sock = netfuncs->TCPConnect(host, 443);
		if (netfuncs->SetTLSClient(sock, host) >= 0)
		{
			netfuncs->Send(sock, header, strlen(header));
			netfuncs->Send(sock, body, strlen(body));
			while(1)
			{
				//FIXME: infinite loop!
				l = netfuncs->Recv(sock, result+rl, sizeof(result)-rl);
				if (l < 0)
					break;
				else
					rl += l;
			}
		}
		netfuncs->Close(sock);
		result[rl] = 0;

		l = strstr(result, "\r\n\r\n")-result;
		l+= 4;
		if (l < 0 || l > rl)
			l = rl;
		x = XML_FromJSON(NULL, "oauth2", result, &l, rl);
//		XML_ConPrintTree(x, "", 1);

		newrefresh = XML_GetChildBody(x, "refresh_token", NULL);
		free(ctx->oauth2.accesstoken);
		ctx->oauth2.accesstoken = strdup(XML_GetChildBody(x, "access_token", ""));
		if (newrefresh || !*ctx->oauth2.accesstoken)
		{
			free(ctx->oauth2.refreshtoken);
			ctx->oauth2.refreshtoken = strdup(XML_GetChildBody(x, "refresh_token", ""));
		}
//		ctx->oauth2.token_type = strdup(XML_GetChildBody(x, "token_type", ""));
//		ctx->oauth2.expires_in = strdup(XML_GetChildBody(x, "expires_in", ""));

		//refresh token may mutate. follow the mutation.
	}
	else if (*ctx->oauth2.accesstoken)
		Con_Printf("XMPP: Using explicit access token\n");

	if (*ctx->oauth2.accesstoken)
	{
		int len = 0;
		if (*ctx->oauth2.useraccount)
		{
			//realm isn't specified
			buf[len++] = 0;
			memcpy(buf+len, ctx->oauth2.useraccount, strlen(ctx->oauth2.useraccount));
			len += strlen(ctx->oauth2.useraccount);
			buf[len++] = 0;
		}
		memcpy(buf+len, ctx->oauth2.accesstoken, strlen(ctx->oauth2.accesstoken));
		len += strlen(ctx->oauth2.accesstoken);

		Con_Printf("XMPP: Signing in\n");
		free(ctx->oauth2.accesstoken);
		ctx->oauth2.accesstoken = strdup("");
		return len;
	}

	//if the reply has a refresh token in it, clear out any password info
	return -2;
}

//in descending priority order
static saslmethod_t saslmethods[] =
{
	{"SCRAM-SHA-512-PLUS",	sasl_scramsha512plus_initial,	sasl_scram_challenge,		sasl_scram_final},	//lots of unreadable hashing, with added channel bindings
	{"SCRAM-SHA-256-PLUS",	sasl_scramsha256plus_initial,	sasl_scram_challenge,		sasl_scram_final},	//lots of unreadable hashing, with added channel bindings
	{"SCRAM-SHA-1-PLUS",	sasl_scramsha1plus_initial,		sasl_scram_challenge,		sasl_scram_final},	//lots of unreadable hashing, with added channel bindings
	{"SCRAM-SHA-512",		sasl_scramsha512minus_initial,	sasl_scram_challenge,		sasl_scram_final},	//lots of unreadable hashing
	{"SCRAM-SHA-256",		sasl_scramsha256minus_initial,	sasl_scram_challenge,		sasl_scram_final},	//lots of unreadable hashing
	{"SCRAM-SHA-1",			sasl_scramsha1minus_initial,	sasl_scram_challenge,		sasl_scram_final},	//lots of unreadable hashing
	{"DIGEST-MD5",			sasl_digestmd5_initial,			sasl_digestmd5_challenge,	NULL},				//kinda silly but still better than plaintext.
	{"PLAIN",				sasl_plain_initial,				NULL,						NULL},				//realm\0username\0password
	{NULL,					sasl_oauth2_initial,			NULL,						NULL}				//potentially avoids having to ask+store their password. a browser is required to obtain auth token for us, so basically like pulling teeth.
};

/*
pidgin's msn request
https://oauth.live.com/authorize?client_id=000000004C07035A&scope=wl.messenger,wl.basic,wl.offline_access,wl.contacts_create,wl.share&response_type=token&redirect_uri=http://pidgin.im/
*/


struct subtree_s;

//void JCL_AddClientMessagef(jclient_t *jcl, char *fmt, ...);
//qboolean JCL_FindBuddy(jclient_t *jcl, const char *jid, buddy_t **buddy, bresource_t **bres, qboolean create);
void JCL_GeneratePresence(jclient_t *jcl, qboolean force);
struct iq_s *JCL_SendIQf(jclient_t *jcl, qboolean (*callback) (jclient_t *jcl, struct subtree_s *tree, struct iq_s *iq), const char *iqtype, const char *target, const char *fmt, ...);
//struct iq_s *JCL_SendIQNode(jclient_t *jcl, qboolean (*callback) (jclient_t *jcl, xmltree_t *tree, struct iq_s *iq), const char *iqtype, const char *target, xmltree_t *node, qboolean destroynode);
void JCL_CloseConnection(jclient_t *jcl, const char *reason, qboolean reconnect);
void JCL_JoinMUCChat(jclient_t *jcl, const char *room, const char *server, const char *myhandle, const char *password);
static qboolean JCL_BuddyVCardReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq);

void JCL_GenLink(jclient_t *jcl, char *out, int outlen, const char *action, const char *context, const char *contextres, const char *sid, const char *txtfmt, ...)
{
	va_list		argptr;
	qboolean textonly = false;
	*out = 0;
	if (!strchr(txtfmt, '%'))
	{	//protect against potential bugs and exploits.
		Q_strlcpy(out, "bad link text", outlen);
		return;
	}
	//FIXME: validate that there is no \\ markup within any section that would break the link.
	//FIXME: validate that ^[ and ^] are not used, as that would also mess things up. add markup for every ^?
	if (textonly)
	{
		va_start(argptr, txtfmt);
		Q_strlcat(out, "[", outlen);
		Q_vsnprintf(out, outlen, txtfmt, argptr);
		Q_strlcat(out, "]", outlen);
		va_end(argptr);
		return;
	}
	Q_strlcat(out, "^[[", outlen);
	va_start(argptr, txtfmt);
	Q_vsnprintf(out+3, outlen-3, txtfmt, argptr);
	va_end(argptr);
	Q_strlcat(out, "]", outlen);

	if (jcl && jcl->accountnum)
	{
		char acc[32];
		Q_snprintf(acc, sizeof(acc), "%i", jcl->accountnum);
		Q_strlcat(out, "\\xmppacc\\", outlen);
		Q_strlcat(out, acc, outlen);
	}

	if (action)
	{
		Q_strlcat(out, "\\xmppact\\", outlen);
		Q_strlcat(out, action, outlen);
	}
	if (context)
	{
		Q_strlcat(out, "\\xmpp\\", outlen);
		Q_strlcat(out, context, outlen);
		if (contextres)
		{
			Q_strlcat(out, "/", outlen);
			Q_strlcat(out, contextres, outlen);
		}
	}
	if (sid)
	{
		Q_strlcat(out, "\\xmppsid\\", outlen);
		Q_strlcat(out, sid, outlen);
	}
	Q_strlcat(out, "^]", outlen);
}

char *TrimResourceFromJid(char *jid)
{
	char *slash;
	slash = strchr(jid, '/');
	if (slash)
	{
		*slash = '\0';
		return slash+1;
	}
	return NULL;
}

qboolean JCL_ConsoleLinkMouseOver(float x, float y)
{
	jclient_t *jcl;
//	char text[256];
	char link[256];
	char who[256];
	char what[256];
	char which[256];
	char *actiontext;
	int i;
	buddy_t *b, *me = NULL;
	bresource_t *br;

//	cmdfuncs->Argv(0, text, sizeof(text));
	cmdfuncs->Argv(1, link, sizeof(link));

	JCL_Info_ValueForKey(link, "xmpp", who, sizeof(who));
	JCL_Info_ValueForKey(link, "xmppact", what, sizeof(what));
	JCL_Info_ValueForKey(link, "xmppacc", which, sizeof(which));

	if (!*who)
		return false;

	i = atoi(which);
	i = bound(0, i, sizeof(jclients)/sizeof(jclients[0]));
	jcl = jclients[i];

	x += 16;

	if (!jcl)
		return false;

	if (jcl->status != JCL_ACTIVE)
	{
		drawfuncs->String(x, y, "^&C0You are currently offline");
		return true;
	}

	if (!strcmp(what, "pauth"))
		actiontext = "Befriend";
	else if (!strcmp(what, "pdeny"))
		actiontext = "Decline";
#ifdef FILETRANSFERS
	else if (!strcmp(what, "fauth") && (jcl->enabledcapabilities & CAP_SIFT))
		actiontext = "Receive";
	else if (!strcmp(what, "fdeny") && (jcl->enabledcapabilities & CAP_SIFT))
		actiontext = "Decline";
#endif
#ifdef JINGLE
	else if (!strcmp(what, "jauth") && (jcl->enabledcapabilities & (CAP_GAMEINVITE|CAP_VOICE|CAP_VIDEO|CAP_GOOGLE_VOICE)))
		actiontext = "Answer";
	else if (!strcmp(what, "jdeny") && (jcl->enabledcapabilities & (CAP_GAMEINVITE|CAP_VOICE|CAP_VIDEO|CAP_GOOGLE_VOICE)))
		actiontext = "Hang Up";
	else if (!strcmp(what, "join") && (jcl->enabledcapabilities & CAP_GAMEINVITE))
		actiontext = "Join Game";
	else if (!strcmp(what, "invite") && (jcl->enabledcapabilities & CAP_GAMEINVITE))
		actiontext = "Invite To Game";
	else if (!strcmp(what, "call") && (jcl->enabledcapabilities & (CAP_VOICE|CAP_GOOGLE_VOICE)))
		actiontext = "Call";
	else if (!strcmp(what, "vidcall") && (jcl->enabledcapabilities & CAP_VIDEO))
		actiontext = "Video Call";
#endif
	else if (!strcmp(what, "mucjoin"))
		actiontext = "Join Chat:";
	else if ((*who && !*what) || !strcmp(what, "msg"))
		actiontext = "Chat With";
	else
		return false;

	JCL_FindBuddy(jcl, who, &b, &br, false);
	if (!b)
		return false;
	JCL_FindBuddy(jcl, jcl->fulljid, &me, NULL, true);

	if ((jcl->enabledcapabilities & CAP_AVATARS) && drawfuncs)
	{
		if (b->vcardphotochanged && b->friended && !jcl->avatarupdate)
		{
			b->vcardphotochanged = false;
			Con_Printf("Querying %s's photo\n", b->accountdomain);
			jcl->avatarupdate = JCL_SendIQf(jcl, JCL_BuddyVCardReply, "get", b->accountdomain, "<vCard xmlns='vcard-temp'/>");
		}
		if (b->image)
		{
			//xep-0153: The image height and width SHOULD be between thirty-two (32) and ninety-six (96) pixels; the recommended size is sixty-four (64) pixels high and sixty-four (64) pixels wide.
			//96 just feels far too large for a game that was origionally running at a resolution of 320*200.
			//FIXME: we should proably respect the image's aspect ratio...
#define IMGSIZE 96/2
			drawfuncs->Image (x, y, IMGSIZE, IMGSIZE, 0, 0, 1, 1, b->image);
			x += IMGSIZE+8;
		}
	}

	drawfuncs->String(x, y, va("^&F0%s ^2%s", actiontext, b->name));
	y+=8;
	drawfuncs->String(x, y, va("^&F0%s", b->accountdomain));
	y+=8;
	if (br)
	{
		drawfuncs->String(x, y, va("^&F0  %s", br->resource));
		y+=8;
	}
	if (b == me)
		drawfuncs->String(x, y, "^&90" "You");
	else if (!b->friended)
		drawfuncs->String(x, y, "^&C0" "Unknown");
	y+=8;

	return true;
}
qboolean JCL_ConsoleLink(void)
{
	jclient_t *jcl;
	char link[256];
	char who[256];
	char what[256];
	char which[256];
	int i;
	buddy_t *b = NULL;
//	cmdfuncs->Argv(0, text, sizeof(text));
	cmdfuncs->Argv(1, link, sizeof(link));

	JCL_Info_ValueForKey(link, "xmpp", who, sizeof(who));
	JCL_Info_ValueForKey(link, "xmppact", what, sizeof(what));
	JCL_Info_ValueForKey(link, "xmppacc", which, sizeof(which));

	if (!*who && !*what)
		return false;

	i = atoi(which);
	i = bound(0, i, sizeof(jclients)/sizeof(jclients[0]));
	jcl = jclients[i];

	jclient_updatebuddylist = true;

	if (!strcmp(what, "pauth"))
	{
		//we should friend them too.
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_AddClientMessagef(jcl, "<presence to='%s' type='subscribed'/>", who);
		return true;
	}
	else if (!strcmp(what, "pdeny"))
	{
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_AddClientMessagef(jcl, "<presence to='%s' type='unsubscribed'/>", who);
		return true;
	}
#ifdef FILETRANSFERS
	else if (!strcmp(what, "fauth") && (jcl->enabledcapabilities & CAP_SIFT))
	{
		JCL_Info_ValueForKey(link, "xmppsid", what, sizeof(what));
		if (jcl && jcl->status == JCL_ACTIVE)
			XMPP_FT_AcceptFile(jcl, atoi(what), true);
		return true;
	}
	else if (!strcmp(what, "fdeny") && (jcl->enabledcapabilities & CAP_SIFT))
	{
		JCL_Info_ValueForKey(link, "xmppsid", what, sizeof(what));
		if (jcl && jcl->status == JCL_ACTIVE)
			XMPP_FT_AcceptFile(jcl, atoi(what), false);
		return true;
	}
#endif
#ifdef JINGLE
	//jauth/jdeny are used to accept/cancel all jingle/gingle content types.
	else if (!strcmp(what, "jauth") && (jcl->enabledcapabilities & (CAP_VOICE|CAP_VIDEO|CAP_GAMEINVITE|CAP_GOOGLE_VOICE)))
	{
		JCL_Info_ValueForKey(link, "xmppsid", what, sizeof(what));
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_Join(jcl, who, what, true, ICEP_INVALID);
		jclient_updatebuddylist = true;
		return true;
	}
	else if (!strcmp(what, "jdeny") && (jcl->enabledcapabilities & (CAP_VOICE|CAP_VIDEO|CAP_GAMEINVITE|CAP_GOOGLE_VOICE)))
	{
		JCL_Info_ValueForKey(link, "xmppsid", what, sizeof(what));
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_Join(jcl, who, what, false, ICEP_INVALID);
		jclient_updatebuddylist = true;
		return true;
	}
	else if (!strcmp(what, "join") && (jcl->enabledcapabilities & CAP_GAMEINVITE))
	{
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_Join(jcl, who, NULL, true, ICEP_QWCLIENT);
		jclient_updatebuddylist = true;
		return true;
	}
	else if (!strcmp(what, "invite") && (jcl->enabledcapabilities & CAP_GAMEINVITE))
	{
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_Join(jcl, who, NULL, true, ICEP_QWSERVER);
		jclient_updatebuddylist = true;
		return true;
	}
	else if (!strcmp(what, "call") && (jcl->enabledcapabilities & (CAP_VOICE|CAP_GOOGLE_VOICE)))
	{
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_Join(jcl, who, NULL, true, ICEP_VOICE);
		jclient_updatebuddylist = true;
		return true;
	}
	else if (!strcmp(what, "vidcall") && (jcl->enabledcapabilities & (CAP_VIDEO)))
	{
		if (jcl && jcl->status == JCL_ACTIVE)
			JCL_Join(jcl, who, NULL, true, ICEP_VIDEO);
		jclient_updatebuddylist = true;
		return true;
	}
#endif
	else if (!strcmp(what, "mucjoin"))
	{	//conference/chat join
		if (jcl)
		{
			JCL_Info_ValueForKey(link, "xmppsid", what, sizeof(what));
			JCL_JoinMUCChat(jcl, who, NULL, NULL, what);
		}
	}
	else if (!strcmp(what, "addfriend"))
	{
		if (jcl)
		{
			confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
			confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter your friend's account name");
			jclient_action_cl = jcl;
			jclient_action_buddy = NULL;
			jclient_action = ACT_ADDFRIEND;
		}
	}
	else if (!strcmp(what, "connect"))
	{
		if (jcl && (jcl->status == JCL_INACTIVE || jcl->status == JCL_DEAD))
		{
			jcl->status = JCL_DEAD;	//flag it as still trying to connect.
			jcl->timeout = jclient_curtime;	//do it now
		}
	}
	else if (!strcmp(what, "disconnect"))
	{
		if (jcl)
		{
			if (jcl->status == JCL_INACTIVE)
				JCL_CloseConnection(jcl, "", false);
			else
			{
				JCL_CloseConnection(jcl, "", true);
				jcl->status = JCL_INACTIVE;
			}
		}
	}
	else if (!strcmp(what, "forgetacc"))
	{
		if (jcl)
		{
			JCL_CloseConnection(jcl, "", false);
		}
	}
	else if (!strcmp(what, "newaccount"))
	{
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
		confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter your XMPP account name\neg: example@"EXAMPLEDOMAIN);
		jclient_action_cl = jcl;
		jclient_action_buddy = NULL;
		jclient_action = ACT_NEWACCOUNT;
	}
	else if (!strcmp(what, "setausername"))
	{
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
		confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter your user name");
		jclient_action_cl = jcl;
		jclient_action_buddy = NULL;
		jclient_action = ACT_SETAUSERNAME;
	}
	else if (!strcmp(what, "setadomain"))
	{
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
		confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter the domain for which your account is valid");
		jclient_action_cl = jcl;
		jclient_action_buddy = NULL;
		jclient_action = ACT_SETADOMAIN;
	}
	else if (!strcmp(what, "setaserver"))
	{
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
		confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter the server to connect to.");
		jclient_action_cl = jcl;
		jclient_action_buddy = NULL;
		jclient_action = ACT_SETASERVER;
	}
	else if (!strcmp(what, "setaresource"))
	{
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
		confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter some resource name.");
		jclient_action_cl = jcl;
		jclient_action_buddy = NULL;
		jclient_action = ACT_SETARESOURCE;
	}
	else if (!strcmp(what, "setapassword"))
	{
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
		confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter your XMPP account name\neg: example@"EXAMPLEDOMAIN);
		jclient_action_cl = jcl;
		jclient_action_buddy = NULL;
		jclient_action = ACT_SETAPASSWORD;
	}
	else if (!strcmp(what, "setasavepassword"))
		jcl->savepassword = !jcl->savepassword;
	else if (!strcmp(what, "accopts"))
	{
		if (jcl)
		{
			char footer[2048];
			char link[512];
			if (jcl->status == JCL_INACTIVE)
				JCL_GenLink(jcl, link, sizeof(link), "forgetacc", NULL, NULL, NULL, "%s", "Forget Account");
			else if (jcl->status == JCL_DEAD)
				JCL_GenLink(jcl, link, sizeof(link), "disconnect", NULL, NULL, NULL, "%s", "Disable");
			else
				JCL_GenLink(jcl, link, sizeof(link), "disconnect", NULL, NULL, NULL, "%s", "Disconnect");
			Q_strlcpy(footer, "\n", sizeof(footer));
			Q_strlcat(footer, link, sizeof(footer));

			if (jcl->status == JCL_INACTIVE)
				JCL_GenLink(jcl, link, sizeof(link), "connect", NULL, NULL, NULL, "%s", "Connect");
			else if (jcl->status == JCL_DEAD)
				JCL_GenLink(jcl, link, sizeof(link), "connect", NULL, NULL, NULL, "%s", "Reconnect");
			else if (jcl->status == JCL_ACTIVE)
				JCL_GenLink(jcl, link, sizeof(link), "addfriend", NULL, NULL, NULL, "%s", "Add Friend");
			else
				*link = 0;

			if (*link)
			{
				Q_strlcat(footer, "\n", sizeof(footer));
				Q_strlcat(footer, link, sizeof(footer));
			}

			if (jcl->status == JCL_INACTIVE)
			{
				JCL_GenLink(jcl, link, sizeof(link), "setausername", NULL, NULL, NULL, "Username: %s", jcl->username);
				Q_strlcat(footer, "\n", sizeof(footer));
				Q_strlcat(footer, link, sizeof(footer));

				JCL_GenLink(jcl, link, sizeof(link), "setadomain", NULL, NULL, NULL, "Domain: %s", jcl->domain);
				Q_strlcat(footer, "\n", sizeof(footer));
				Q_strlcat(footer, link, sizeof(footer));

				JCL_GenLink(jcl, link, sizeof(link), "setaserver", NULL, NULL, NULL, "Server: %s", *jcl->serveraddr?jcl->serveraddr:"<AUTO>");
				Q_strlcat(footer, "\n", sizeof(footer));
				Q_strlcat(footer, link, sizeof(footer));

				JCL_GenLink(jcl, link, sizeof(link), "setaresource", NULL, NULL, NULL, "Resource: %s", jcl->resource);
				Q_strlcat(footer, "\n", sizeof(footer));
				Q_strlcat(footer, link, sizeof(footer));
			}
//			if (jcl->status == JCL_INACTIVE)
			{
				JCL_GenLink(jcl, link, sizeof(link), "setapassword", NULL, NULL, NULL, "Password: %s", jcl->sasl.password_plain);
				Q_strlcat(footer, "\n", sizeof(footer));
				Q_strlcat(footer, link, sizeof(footer));

				JCL_GenLink(jcl, link, sizeof(link), "setasavepassword", NULL, NULL, NULL, "Save Password: %s", jcl->savepassword?"true":"false");
				Q_strlcat(footer, "\n", sizeof(footer));
				Q_strlcat(footer, link, sizeof(footer));
			}

			confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", footer);
		}
	}
	else if (!strcmp(what, "buddyopts"))
	{
		char footer[2048];
		char chatlink[512];
		char unfriend[512];
		char realias[512];
		JCL_FindBuddy(jcl, *who?who:jcl->defaultdest, &b, NULL, false);
		if (b)
		{
			if (b->btype == BT_CHATROOM)
			{
				JCL_GenLink(jcl, chatlink, sizeof(chatlink), NULL, b->accountdomain, NULL, NULL, "Chat in %s", b->name);
				JCL_GenLink(jcl, unfriend, sizeof(unfriend), "unfriend", b->accountdomain, NULL, NULL, "Forget %s", b->name);
				JCL_GenLink(jcl, realias, sizeof(realias), "setbalias", b->accountdomain, NULL, NULL, "Set alias for %s", b->name);
			}
			else
			{
				JCL_GenLink(jcl, chatlink, sizeof(chatlink), NULL, b->accountdomain, NULL, NULL, "Chat with %s", b->name);
				JCL_GenLink(jcl, unfriend, sizeof(unfriend), "unfriend", b->accountdomain, NULL, NULL, "Unfriend %s", b->name);
				JCL_GenLink(jcl, realias, sizeof(realias), "setbalias", b->accountdomain, NULL, NULL, "Set alias for %s", b->name);
			}
			Q_snprintf(footer, sizeof(footer), "\n%s\n%s\n%s", chatlink, unfriend, realias);

			confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", footer);
		}
	}
	else if (!strcmp(what, "unfriend"))
	{
		JCL_FindBuddy(jcl, *who?who:jcl->defaultdest, &b, NULL, false);
		if (b)
		{
			if (b->btype == BT_CHATROOM)
			{
				JCL_AddClientMessagef(jcl, "<presence to='%s/%s' type='unavailable'/>", b->accountdomain, b->ourselves?b->ourselves->resource:"");
				JCL_ForgetBuddy(jcl, b, NULL);
			}
			else if (b->btype == BT_ROSTER)
			{
				//hide from em
				JCL_AddClientMessagef(jcl, "<presence to='%s' type='unsubscribed'/>", b->accountdomain);

				//stop looking for em
				JCL_AddClientMessagef(jcl, "<presence to='%s' type='unsubscribe'/>", b->accountdomain);

				//stop listing em
				JCL_SendIQf(jcl, NULL, "set", NULL, "<query xmlns='jabber:iq:roster'><item jid='%s' subscription='remove' /></query>", b->accountdomain);
//				b->btype = BT_UNKNOWN;
				JCL_ForgetBuddy(jcl, b, NULL);
			}
			else
			{
				JCL_AddClientMessagef(jcl, "<presence to='%s' type='unavailable'/>", b->accountdomain);
				JCL_ForgetBuddy(jcl, b, NULL);
			}
		}
	}
	else if (!strcmp(what, "setbalias"))
	{
		JCL_FindBuddy(jcl, *who?who:jcl->defaultdest, &b, NULL, false);
		if (b)
		{
			confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
			confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter what you want to see them as");
			jclient_action_cl = jcl;
			jclient_action_buddy = b;
			jclient_action = ACT_SETBALIAS;
		}
	}
	else if ((*who && !*what) || !strcmp(what, "msg"))
	{
		if (jcl)
		{
			char *f;
			buddy_t *b = NULL;
			bresource_t *br = NULL;
			JCL_FindBuddy(jcl, *who?who:jcl->defaultdest, &b, &br, true);
			if (b)
			{
				f = b->accountdomain;
				if (b->btype != BT_CHATROOM)
					b->defaultresource = br;

				if (confuncs)
				{
					confuncs->SetConsoleString(f, "title", b->name);
					confuncs->SetConsoleFloat(f, "iswindow", true);
					confuncs->SetConsoleFloat(f, "forceutf8", true);
					confuncs->SetConsoleFloat(f, "wnd_w", 256);
					confuncs->SetConsoleFloat(f, "wnd_h", 320);
				}
				if (confuncs)
					confuncs->SetActive(f);
			}
		}
		return true;
	}
	else
	{
		Con_Printf("Unsupported xmpp action (%s) in link\n", what);
	}

	return false;
}

void JCL_ToJID(jclient_t *jcl, const char *in, char *out, int outsize, qboolean assumeresource)
{
	//decompose links first
	if (in[0] == '^' && in[1] == '[')
	{
		const char *sl;
		const char *le;
		sl = in+2;
		sl = strchr(in, '\\');
		if (sl)
		{
			le = strstr(sl, "^]");
			if (le)
			{
				char info[512];
				if (le-sl < 512)
				{
					memcpy(info, sl, le-sl);
					info[le-sl] = 0;
					JCL_Info_ValueForKey(info, "xmpp", out, outsize);
				}
				return;
			}
		}
	}

	if (!strchr(in, '@') && jcl)
	{
		//no @? probably its an alias, but could also be a server/domain perhaps. not sure we care. you'll just have to rename your friend.
		//check to see if we can find a friend going by that name
		//fixme: allow resources to make it through here
		buddy_t *b;
		for (b = jcl->buddies; b; b = b->next)
		{
			if (!strcasecmp(b->name, in))
			{
				if (b->defaultresource && assumeresource)
					Q_snprintf(out, outsize, "%s/%s", b->accountdomain, b->defaultresource->resource);
				else
					Q_strlcpy(out, b->accountdomain, outsize);
				return;
			}
		}
	}
	
	if (assumeresource)
	{
		buddy_t *b;
		for (b = jcl->buddies; b; b = b->next)
		{
			if (!strcasecmp(b->accountdomain, in))
			{
				if (b->defaultresource && assumeresource)
					Q_snprintf(out, outsize, "%s/%s", b->accountdomain, b->defaultresource->resource);
				else
					Q_strlcpy(out, b->accountdomain, outsize);
				return;
			}
		}
	}

	//a regular jabber account name
	Q_strlcpy(out, in, outsize);
}

void XMPP_AddFriend(jclient_t *jcl, const char *account, const char *nick)
{
	char jid[256];
	//FIXME: validate the name. deal with xml markup.
	//try and make sense of the name given
	JCL_ToJID(jcl, account, jid, sizeof(jid), false);
	if (!strchr(jid, '@'))
		Con_Printf("Missing @ character. Trying anyway, but this will be assumed to be a server rather than a user.\n");

	//can also rename. We should probably read back the groups for the update.
	JCL_SendIQf(jcl, NULL, "set", NULL, "<query xmlns='jabber:iq:roster'><item jid='%s' name='%s'></item></query>", jid, nick);

	//start looking for em
	JCL_AddClientMessagef(jcl, "<presence to='%s' type='subscribe'/>", jid);

	//let em see us
	if (jcl->preapproval)
		JCL_AddClientMessagef(jcl, "<presence to='%s' type='subscribed'/>", jid);
}
jclient_t *JCL_Connect(int accnum, char *server, int forcetls, char *account, char *password);
int JCL_ConExecuteCommand(qboolean isinsecure)
{
	buddy_t *b;
	char consolename[256];
	jclient_t *jcl;
	int i;

	cmdfuncs->Argv(0, consolename, sizeof(consolename));

	if (!strcmp(consolename, BUDDYLISTTITLE))
	{
		char args[512];
		cmdfuncs->Args(args, sizeof(args));
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", false);
		confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "");
		jclient_updatebuddylist = true;

		//FIXME: validate that the client is still active.
		switch(jclient_action)
		{
		case ACT_NONE:
			break;
		case ACT_OAUTH:
			jcl = jclient_action_cl;
			if (jcl)
			{
				free(jcl->sasl.oauth2.authtoken);
				jcl->sasl.oauth2.authtoken = strdup(args);
				if (jcl->status == JCL_INACTIVE)
					jcl->status = JCL_DEAD;
			}
			break;
		case ACT_NEWACCOUNT:
			if (!*args)
				break;	//they didn't enter anything! oh well.
			for (i = 0; i < sizeof(jclients)/sizeof(jclients[0]); i++)
			{
				if (jclients[i])
					continue;
				jclients[i] = JCL_Connect(i, "", 1, args, "");
				break;
			}
			if (i == sizeof(jclients)/sizeof(jclients[0]))
				confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Too many accounts open");
			else if (!jclients[i])
				confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Unable to create account");
			else
			{	//now ask for a password instantly. because oauth2 is basically unusable.
				jclients[i]->status = JCL_INACTIVE;
				confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
				confuncs->SetConsoleString(BUDDYLISTTITLE, "footer", "Please enter password");
				jclient_action_cl = jclients[i];
				jclient_action_buddy = NULL;
				jclient_action = ACT_SETAPASSWORD;
				return true;
			}
			break;
		case ACT_SETAUSERNAME:
			Q_strncpyz(jclient_action_cl->username, args, sizeof(jclient_action_cl->username));
			break;
		case ACT_SETADOMAIN:
			Q_strncpyz(jclient_action_cl->domain, args, sizeof(jclient_action_cl->domain));
			break;
		case ACT_SETASERVER:
			Q_strncpyz(jclient_action_cl->serveraddr, args, sizeof(jclient_action_cl->serveraddr));
			break;
		case ACT_SETARESOURCE:
			Q_strncpyz(jclient_action_cl->resource, args, sizeof(jclient_action_cl->resource));
			break;
		case ACT_SETAPASSWORD:
			if (*args)
			{
				Q_strncpyz(jclient_action_cl->sasl.password_plain, args, sizeof(jclient_action_cl->sasl.password_plain));
				jclient_action_cl->sasl.password_hash_size = 0; //invalidate it
			}
			if (jclient_action_cl->status == JCL_INACTIVE)
				jclient_action_cl->status = JCL_DEAD;
			jclient_action = ACT_NONE;
			jclient_configdirty = true;
			return 2;	//ask to not store in history.
		case ACT_ADDFRIEND:
			if (*args)
				XMPP_AddFriend(jclient_action_cl, args, "");
			break;
		case ACT_SETBALIAS:
			jclient_configdirty = true;
			Q_strncpyz(jclient_action_buddy->name, args, sizeof(jclient_action_buddy->name));
			if (jclient_action_buddy->btype == BT_ROSTER)
				JCL_SendIQf(jclient_action_cl, NULL, "set", NULL, "<query xmlns='jabber:iq:roster'><item jid='%s' name='%s'></item></query>", jclient_action_buddy->accountdomain, jclient_action_buddy->name);
			if (confuncs)
				confuncs->SetConsoleString(jclient_action_buddy->accountdomain, "title", jclient_action_buddy->name);
			break;
		}
		jclient_action = ACT_NONE;
		return true;
	}

	for (i = 0; i < sizeof(jclients) / sizeof(jclients[0]); i++)
	{
		jcl = jclients[i];
		if (!jcl)
			continue;
		for (b = jcl->buddies; b; b = b->next)
		{
			if (!strcmp(b->accountdomain, consolename))
			{
				if (b->defaultresource)
					Q_snprintf(jcl->defaultdest, sizeof(jcl->defaultdest), "%s/%s", b->accountdomain, b->defaultresource->resource);
				else
					Q_snprintf(jcl->defaultdest, sizeof(jcl->defaultdest), "%s", b->accountdomain);
				JCL_Command(i, consolename);
				return true;
			}
		}
	}
	for (i = 0; i < sizeof(jclients) / sizeof(jclients[0]); i++)
	{
		jcl = jclients[i];
		if (!jcl)
			continue;
		JCL_Command(i, consolename);
		return true;
	}

	Con_SubPrintf(consolename, "You were disconnected\n");
	return true;
}

void JCL_FlushOutgoing(jclient_t *jcl)
{
	int sent;
	if (!jcl || !jcl->outbuflen || jcl->socket == -1)
		return;

	sent = netfuncs->Send(jcl->socket, jcl->outbuf + jcl->outbufpos, jcl->outbuflen);	//FIXME: This needs rewriting to cope with errors.
	if (sent > 0)
	{
		//and print it on some subconsole if we're debugging
		if (jcl->streamdebug)
		{
			char t = jcl->outbuf[jcl->outbufpos+sent];
			jcl->outbuf[jcl->outbufpos+sent] = 0;
			XMPP_ConversationPrintf("xmppout", "xmppout", false, "^3%s\n", jcl->outbuf + jcl->outbufpos);
			jcl->outbuf[jcl->outbufpos+sent] = t;
		}

		jcl->outbufpos += sent;
		jcl->outbuflen -= sent;
	}
	else if (sent < 0)
		Con_Printf("XMPP: Error sending\n");
//	else
//		Con_Printf("Unable to send anything\n");
}
void JCL_AddClientMessage(jclient_t *jcl, const char *msg, int datalen)
{
	//handle overflows
	if (jcl->outbufpos+jcl->outbuflen+datalen > jcl->outbufmax)
	{
		if (jcl->outbuflen+datalen <= jcl->outbufmax)
		{
			//can get away with just moving the data
			memmove(jcl->outbuf, jcl->outbuf + jcl->outbufpos, jcl->outbuflen);
			jcl->outbufpos = 0;
		}
		else
		{
			//need to expand the buffer.
			int newmax = (jcl->outbuflen+datalen)*2;
			char *newbuf;

			if (newmax < jcl->outbuflen)
				newbuf = NULL;	//eep... some special kind of evil overflow.
			else
				newbuf = malloc(newmax+1);

			if (newbuf)
			{
				memcpy(newbuf, jcl->outbuf + jcl->outbufpos, jcl->outbuflen);
				jcl->outbufmax = newmax;
				jcl->outbufpos = 0;
				jcl->outbuf = newbuf;
			}
			else
				datalen = 0;	//eep!
		}
	}
	//and write our data to it
	memcpy(jcl->outbuf + jcl->outbufpos + jcl->outbuflen, msg, datalen);
	jcl->outbuflen += datalen;

	//try and flush it now
	JCL_FlushOutgoing(jcl);
}
void JCL_AddClientMessageString(jclient_t *jcl, const char *msg)
{
	JCL_AddClientMessage(jcl, msg, strlen(msg));
}
void JCL_AddClientMessagef(jclient_t *jcl, const char *fmt, ...)
{
	va_list		argptr;
	char body[4096];

	va_start (argptr, fmt);
	Q_vsnprintxf (body, sizeof(body), fmt, argptr);
	va_end (argptr);

	JCL_AddClientMessageString(jcl, body);
}
qboolean JCL_Reconnect(jclient_t *jcl)
{
	char *serveraddr;
	//destroy any data that never got sent
	free(jcl->outbuf);
	jcl->outbuf = NULL;
	jcl->outbuflen = 0;
	jcl->outbufpos = 0;
	jcl->outbufmax = 0;
	jcl->instreampos = 0;
	jcl->bufferedinammount = 0;
	Q_strlcpy(jcl->localalias, ">>", sizeof(jcl->localalias));
	jcl->sasl.authmethod = NULL;

	if (*jcl->redirserveraddr)
		serveraddr = jcl->redirserveraddr;
	else
		serveraddr = jcl->serveraddr;

	if (!*serveraddr)
	{
		//jcl->tlsconnect requires an explicit hostname, so should not be able to take this path.
		char srv[256];
		char srvserver[256];
		if (!Q_snprintfz(srv, sizeof(srv), "_xmpp-client._tcp.%s", jcl->domain) && NET_DNSLookup_SRV(srv, srvserver, sizeof(srvserver)))
		{
			Con_DPrintf("XMPP: Trying to connect to %s (%s)\n", jcl->domain, srvserver);
			jcl->socket = netfuncs->TCPConnect(srvserver, jcl->serverport);	//port is should already be part of the srvserver name
		}
		else
		{
			//SRV lookup failed. attempt to just use the domain directly.
			Con_DPrintf("XMPP: Trying to connect to %s\n", jcl->domain);
			jcl->socket = netfuncs->TCPConnect(jcl->domain, jcl->serverport);	//port is only used if the url doesn't contain one. It's a default.
		}
	}
	else
	{
		Con_DPrintf("XMPP: Trying to connect to %s\n", jcl->domain);
		jcl->socket = netfuncs->TCPConnect(serveraddr, jcl->serverport);	//port is only used if the url doesn't contain one. It's a default.
	}

	//not yet blocking. So no frequent attempts please...
	//non blocking prevents connect from returning worthwhile sensible value.
	if ((int)jcl->socket < 0)
	{
		Q_strncpyz(jcl->errormsg, "Unable to connect", sizeof(jcl->errormsg));
		return false;
	}

	jcl->issecure = false;
	if (jcl->forcetls==2)
		if (netfuncs->SetTLSClient(jcl->socket, jcl->certificatedomain)>=0)
			jcl->issecure = true;

	jcl->status = JCL_AUTHING;
	jcl->connecting = true;

	JCL_AddClientMessageString(jcl,
		"<?xml version='1.0' ?>"
		"<stream:stream to='");
	JCL_AddClientMessageString(jcl, jcl->domain);
	JCL_AddClientMessageString(jcl, "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>");

	jclient_updatebuddylist = true;

	return true;
}

jclient_t *JCL_ConnectXML(xmltree_t *acc)
{
	//this is a list of xmpp networks that use oauth2 for which we have a clientid registered. this allows us to get the right oauth defaults.
	struct 
	{
		char *domain;
		char *saslmethod;
		char *obtainurl;
		char *refreshurl;
		char *revokeurl;
		char *scope;
		char *clientid;
		char *clientsecret;
		//char *signouturl;
		//char *clientregistrationurl;
	} *oa, dfltoauth[] = {
		{
			"gmail.com",
			"X-OAUTH2",
			"https://accounts.google.com/o/oauth2/auth",	//authorize
			"https://accounts.google.com/o/oauth2/token",	//refresh
			"https://accounts.google.com/o/oauth2/revoke",	//revoke
			"https://www.googleapis.com/auth/googletalk",	//scope
			"1060926168015.apps.googleusercontent.com",		//clientid
			"mptCRRTE5I626npsnoZ_RqoG"						//client secrit. there really is no securing this. I'll just have to avoid any pay-for google apis. *shrug*
			//"https://accounts.google.com/IssuedAuthSubTokens"
			//"https://code.google.com/apis/console"
		},
/*
		{
			"messenger.live.com",
			"X-MESSENGER-OAUTH2",
			"https://oauth.live.com/authorize",
			"https://oauth.live.com/token",
			"",	//FIXME fill in revoke url
			"wl.messenger,wl.basic,wl.offline_access,wl.contacts_create,wl.share",	//no idea what's actually needed.
			"",	//client-id - none registered. go register it yourself.
			""	//client-secret - client not registered, go do it yourself.
			//""
			//"https://manage.dev.live.com/"
		},
*/
/*
		{
			"messenger.live.com",
			"X-FACEBOOK-PLATFORM",
			"", //FIXME fill in	obtain url
			"",	//FIXME fill in	refresh url
			"",	//FIXME fill in revoke url
			"",	//FIXME: fill in scope
			"",	//client-id - none registered. go register it yourself.
			""	//client-secret - client not registered, go do it yourself.
			//""
			//""
		},
*/
		{
			NULL,
			"",
			"",
			"",
			"",
			"",
			"",
			""
		}
	};
	jclient_t *jcl;
	xmltree_t *oauth2;
	xmltree_t *features;
	char oauthname[512];
	struct buddyinfo_s *bi;
	int bn;

	jcl = malloc(sizeof(jclient_t));
	if (!jcl)
		return NULL;

	memset(jcl, 0, sizeof(jclient_t));
	jcl->socket = -1;
	jcl->rcon_pipe = -1;

	jcl->enabledcapabilities = CAP_DEFAULTENABLEDCAPS;

	jcl->accountnum = atoi(XML_GetParameter(acc, "id", "1"));

	//make sure dependant properties are listed beneath their dependancies...
	jcl->forcetls = atoi(XML_GetChildBody(acc, "forcetls", "1"));
	jcl->streamdebug = atoi(XML_GetChildBody(acc, "streamdebug", "0"));
	jcl->streamdebug = bound(0, jcl->streamdebug, 2);
	Q_strlcpy(jcl->serveraddr, XML_GetChildBody(acc, "serveraddr", ""), sizeof(jcl->serveraddr));
	jcl->serverport = atoi(XML_GetChildBody(acc, "serverport", (jcl->forcetls==2)?"5223":"5222"));
	Q_strlcpy(jcl->username, XML_GetChildBody(acc, "username", "user"), sizeof(jcl->username));
	Q_strlcpy(jcl->domain, XML_GetChildBody(acc, "domain", "localhost"), sizeof(jcl->domain));
	Q_strlcpy(jcl->resource, XML_GetChildBody(acc, "resource", ""), sizeof(jcl->resource));

	//half of these networks seem to have weird domains. especially microsoft.
	if (strchr(jcl->username, '@'))
		Q_strlcpy(oauthname, jcl->username, sizeof(oauthname));
	else
		Q_snprintf(oauthname, sizeof(oauthname), "%s@%s", jcl->username, jcl->domain);
	for (oa = dfltoauth; oa->domain; oa++)
	{
		if (!strcmp(oa->domain, jcl->domain))
			break;
	}
	oauth2 = XML_ChildOfTree(acc, "oauth2", 0);
	Q_strlcpy(jcl->sasl.oauth2.saslmethod, XML_GetParameter(oauth2, "method", oa->saslmethod), sizeof(jcl->sasl.oauth2.saslmethod));
	Q_strlcpy(jcl->sasl.oauth2.obtainurl, XML_GetChildBody(oauth2, "obtain-url", oa->obtainurl), sizeof(jcl->sasl.oauth2.obtainurl));
	Q_strlcpy(jcl->sasl.oauth2.refreshurl, XML_GetChildBody(oauth2, "refresh-url", oa->refreshurl), sizeof(jcl->sasl.oauth2.refreshurl));
	Q_strlcpy(jcl->sasl.oauth2.clientid, XML_GetChildBody(oauth2, "client-id", oa->clientid), sizeof(jcl->sasl.oauth2.clientid));
	Q_strlcpy(jcl->sasl.oauth2.clientsecret, XML_GetChildBody(oauth2, "client-secret", oa->clientsecret), sizeof(jcl->sasl.oauth2.clientsecret));
	jcl->sasl.oauth2.scope = strdup(XML_GetChildBody(oauth2, "scope", oa->scope));
	jcl->sasl.oauth2.useraccount = strdup(XML_GetChildBody(oauth2, "authname", oauthname));
	jcl->sasl.oauth2.authtoken = strdup(XML_GetChildBody(oauth2, "auth-token", ""));
	jcl->sasl.oauth2.refreshtoken = strdup(XML_GetChildBody(oauth2, "refresh-token", ""));
	jcl->sasl.oauth2.accesstoken = strdup(XML_GetChildBody(oauth2, "access-token", ""));

	Q_strlcpy(jcl->sasl.password_plain, XML_GetChildBody(acc, "password", ""), sizeof(jcl->sasl.password_plain));

	if (!*jcl->resource)
	{	//the default resource matches the game that they're trying to play.
		char gamename[64], *res, *o;
		if (cvarfuncs->GetString("fs_gamename", gamename, sizeof(gamename)))
		{
			//strip out any weird chars (namely whitespace)
			for (o = gamename, res = gamename; *res; )
			{
				if (*res == ' ' || *res == '\t')
					res++;
				else
					*o++ = *res++;
			}
			if (!*gamename)
				Q_strlcpy(jcl->resource, "FTE", sizeof(jcl->resource));
			else
				Q_strlcpy(jcl->resource, gamename, sizeof(jcl->resource));
		}
	}
	Q_strlcpy(jcl->certificatedomain, XML_GetChildBody(acc, "certificatedomain", jcl->domain), sizeof(jcl->certificatedomain));
	jcl->status = atoi(XML_GetChildBody(acc, "inactive", "0"))?JCL_INACTIVE:JCL_DEAD;
	jcl->sasl.allowauth_plainnontls	= atoi(XML_GetChildBody(acc, "allowauth_plain_nontls",	"0"));
	jcl->sasl.allowauth_plaintls		= atoi(XML_GetChildBody(acc, "allowauth_plain_tls",		"1"));	//required 1 for googletalk, otherwise I'd set it to 0.
	jcl->sasl.allowauth_digestmd5	= atoi(XML_GetChildBody(acc, "allowauth_digest_md5",	"1"));
	jcl->sasl.allowauth_scramsha1	= atoi(XML_GetChildBody(acc, "allowauth_scram_sha_1",	"1"));
	jcl->sasl.allowauth_oauth2		= atoi(XML_GetChildBody(acc, "allowauth_oauth2",		*jcl->sasl.oauth2.saslmethod?"1":"0"));

	jcl->savepassword	= atoi(XML_GetChildBody(acc, "savepassword",	"0"));

	features = XML_ChildOfTree(acc, "features", 0);
	if (features && XML_GetParameter(features, "ver", JCL_BUILD))
	{
		const char *val;
		int j;
		for (j = 0; capnames[j].names; j++)
		{
			val = XML_GetChildBody(features, capnames[j].names, NULL);
			if (val)
			{
				if (atoi(val))
					jcl->enabledcapabilities |= capnames[j].cap;
				else
					jcl->enabledcapabilities &= ~capnames[j].cap;
			}
		}
	}


	//parse chatrooms
	features = XML_ChildOfTree(acc, "chats", 0);
	if (features)
	{
		for (bn=0; ; bn++)
		{
			xmltree_t *b = XML_ChildOfTree(features, "room", bn);
			if (b)
			{
				buddy_t *room;
				const char *jid = XML_GetParameter(b, "name", "");
				const char *alias = XML_GetParameter(b, "alias", jid);
				const char *topic = XML_GetParameter(b, "topic", NULL);
				const char *nick = XML_GetParameter(b, "nick", NULL);
				const char *password = XML_GetParameter(b, "password", NULL);
				qboolean autojoin = atoi(XML_GetParameter(b, "autojoin", "1"));

				if (JCL_FindBuddy(jcl, jid, &room, NULL, true) && room)
				{
					room->btype = BT_CHATROOM;
					Q_strlcpy(room->name, alias, sizeof(room->name));
					room->room_topic = topic?strdup(topic):NULL;
					room->room_nick = nick?strdup(nick):NULL;
					room->room_password = password?strdup(password):NULL;
					room->room_autojoin = autojoin;
				}
			}
			else
				break;
		}
	}
	else
	{
		/*buddy_t *room;
		const char *jid = "quake@somedomain";
		const char *alias = "FTE Users";
		const char *topic = NULL;
		const char *nick = NULL;
		const char *password = NULL;

		if (JCL_FindBuddy(jcl, jid, &room, NULL, true) && room)
		{
			room->btype = BT_CHATROOM;
			Q_strlcpy(room->name, alias, sizeof(room->name));
			room->room_topic = topic?strdup(topic):NULL;
			room->room_nick = nick?strdup(nick):NULL;
			room->room_password = password?strdup(password):NULL;
		}*/
	}


	features = XML_ChildOfTree(acc, "buddyinfo", 0);
	for (bn=0; ; bn++)
	{
		xmltree_t *b = XML_ChildOfTree(features, "buddy", bn);
		if (b)
		{
			const char *buddyid = XML_GetParameter(b, "name", JCL_BUILD);
			const char *buddyimage = XML_GetChildBody(b, "image", NULL);
			const char *buddyimagemime = XML_GetChildBody(b, "imagemime", NULL);
			const char *buddyimagehash = XML_GetChildBody(b, "imagehash", NULL);

			bi = malloc(sizeof(*bi) + strlen(buddyid));
			strcpy(bi->accountdomain, buddyid);
			bi->image = buddyimage?strdup(buddyimage):NULL;
			bi->imagemime = buddyimagemime?strdup(buddyimagemime):NULL;
			bi->imagehash = buddyimagehash?strdup(buddyimagehash):NULL;
			bi->next = jcl->buddyinfo;
			jcl->buddyinfo = bi;
		}
		else
			break;
	}

	return jcl;
}

jclient_t *JCL_Connect(int accnum, char *server, int forcetls, char *account, char *password)
{
	char gamename[64];
	jclient_t *jcl;
	char *domain;
	char *res;
	xmltree_t *x;

	res = TrimResourceFromJid(account);
	if (!res || !*res)
	{
		//the default resource matches the game that they're trying to play.
		if (cvarfuncs->GetString("fs_gamename", gamename, sizeof(gamename)))
		{
			//strip out any weird chars (namely whitespace)
			char *o;
			for (o = gamename, res = gamename; *res; )
			{
				if (*res == ' ' || *res == '\t')
					res++;
				else
					*o++ = *res++;
			}
			res = gamename;
		}
	}

/*
	if (forcetls>0)
	{
		if (!BUILTINISVALID(Net_SetTLSClient))
		{
			Con_Printf("XMPP: TLS is not supported\n");
			return NULL;
		}
	}
*/

	domain = strchr(account, '@');
	if (domain)
	{
		*domain = '\0';
		domain++;
	}
	else
	{
#ifdef DEFAULTDOMAIN
		domain = DEFAULTDOMAIN;
		Con_Printf("XMPP: domain not specified, assuming %s\n", domain);
#else
		Con_Printf("XMPP: domain not specified\n");
		return NULL;
#endif
	}

	x = XML_CreateNode(NULL, "account", "", "");
	XML_AddParameteri(x, "id", accnum);
	XML_CreateNode(x, "serveraddr", "", server);
	XML_CreateNode(x, "username", "", account);
	XML_CreateNode(x, "domain", "", domain);
	XML_CreateNode(x, "resource", "", res);
	XML_CreateNode(x, "password", "", password);
	jcl = JCL_ConnectXML(x);
	XML_Destroy(x);
	return jcl;
}

void JCL_ForgetBuddyResource(jclient_t *jcl, buddy_t *buddy, bresource_t *bres)
{
	bresource_t **link;
	bresource_t *r;
	for (link = &buddy->resources; *link; )
	{
		r = *link;
		if (!bres || bres == r)
		{
			*link = r->next;
			free(r);
			if (bres)
				break;
		}
		else
			link = &r->next;
	}
}
void JCL_ForgetBuddy(jclient_t *jcl, buddy_t *buddy, bresource_t *bres)
{
	buddy_t **link;
	buddy_t *b;
	for (link = &jcl->buddies; *link; )
	{
		b = *link;
		if (!buddy || buddy == b)
		{
			*link = b->next;
			JCL_ForgetBuddyResource(jcl, b, bres);
			if (jclient_action_buddy == b)
			{
				confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", false);
				jclient_action = ACT_NONE;
				jclient_action_buddy = NULL;
				jclient_action_cl = NULL;
			}
			free(b);
			if (buddy)
				break;
		}
		else
			link = &b->next;
	}

	jclient_updatebuddylist = true;
}

struct stringprep_range
{
	unsigned int cp_min;
	unsigned int cp_max;
	unsigned int remap[2];
};
static struct stringprep_range stringprep_A1[] =
{
	{0x0234,0x024F},
	{0x02AE,0x02AF},
	{0x02EF,0x02FF},
	{0x0350,0x035F},
	{0x0370,0x0373},
	{0x0376,0x0379},
	{0x037B,0x037D},
	{0x037F,0x0383},
	{0x038B},
	{0x038D},
	{0x03A2},
	{0x03CF},
	{0x03F7,0x03FF},
	{0x0487},
	{0x04CF},
	{0x04F6,0x04F7},
	{0x04FA,0x04FF},
	{0x0510,0x0530},
	{0x0557,0x0558},
	{0x0560},
	{0x0588},
	{0x058B,0x0590},
	{0x05A2},
	{0x05BA},
	{0x05C5,0x05CF},
	{0x05EB,0x05EF},
	{0x05F5,0x060B},
	{0x0600,~0},	//FIXME rest of A.1 (utf)
};
static struct stringprep_range stringprep_B1[] =
{
	{0x00AD},
	{0x034F},
	{0x1806},
	{0x180B},
	{0x180C},
	{0x180D},
	{0x200B},
	{0x200C},
	{0x200D},
	{0x2060},
	{0xFE00},
	{0xFE01},
	{0xFE02},
	{0xFE03},
	{0xFE04},
	{0xFE05},
	{0xFE06},
	{0xFE07},
	{0xFE08},
	{0xFE09},
	{0xFE0A},
	{0xFE0B},
	{0xFE0C},
	{0xFE0D},
	{0xFE0E},
	{0xFE0F},
	{0xFEFF},
};
static struct stringprep_range stringprep_B2[] =
{
	{0x0041, 0x005A,	{0x0061}},
	{0x00B5, 0x00B5,	{0x03BC}},
	{0x00C0, 0x00DE,	{0x00E0}},
	{0x00DF, 0,			{0x0073, 0x0073}},
};
static struct stringprep_range stringprep_C1[] =
{
	//C1.1
	{0x0020},
	//C2.2
	{0x0020},
	{0x00A0},
	{0x1680},
	//FIXME... utf
};
static struct stringprep_range stringprep_C2[] =
{
	//C.2.1
	{0x0000, 0x001F},
	{0x007F, 0x007F},
	//C.2.1
	{0x0080, 0x009F},			//C.2.2
	//FIXME... utf
	{0x06DD, ~0},
};
static struct stringprep_range stringprep_C3[] =
{
	{0xE000, 0xF8FF},
	{0xF0000, 0xFFFFD},
	{0x100000, 0x10FFFD},
};
static struct stringprep_range stringprep_C4[] =
{
	{0xFDD0, 0xFDEF},
	{0xFFFE, 0xFFFF},
	{0x1FFFE, 0x1FFFF},
	{0x2FFFE, 0x2FFFF},
	{0x3FFFE, 0x3FFFF},
	{0x4FFFE, 0x4FFFF},
	{0x5FFFE, 0x5FFFF},
	{0x6FFFE, 0x6FFFF},
	{0x7FFFE, 0x7FFFF},
	{0x8FFFE, 0x8FFFF},
	{0x9FFFE, 0x9FFFF},
	{0xAFFFE, 0xAFFFF},
	{0xBFFFE, 0xBFFFF},
	{0xCFFFE, 0xCFFFF},
	{0xDFFFE, 0xDFFFF},
	{0xEFFFE, 0xEFFFF},
	{0xFFFFE, 0xFFFFF},
	{0x10FFFE, 0x10FFFF},
};
static struct stringprep_range stringprep_C5[] =
{
	{0xD800, 0xDFFF},
};
static struct stringprep_range stringprep_C6[] =
{
	{0xFFF9},
	{0xFFFA},
	{0xFFFB},
	{0xFFFC},
	{0xFFFD},
};
static struct stringprep_range stringprep_C7[] =
{
	{0x2FF0, 0x2FFB},
};
static struct stringprep_range stringprep_C8[] =
{
	{0x0340},
	{0x0341},
	{0x200E},
	{0x200F},
	{0x202A},
	{0x202B},
	{0x202C},
	{0x202D},
	{0x202E},
	{0x206A},
	{0x206B},
	{0x206C},
	{0x206D},
	{0x206E},
	{0x206F},
};
static struct stringprep_range stringprep_C9[] =
{
	{0xE0001},
	{0xE0001},
	{0xE0020, 0xE007F},
};

static struct stringprep_range *StringPrep_InRange_(unsigned int codepoint, struct stringprep_range *range, size_t slots)
{
	int i;
	for (i = 0; i < slots; i++)
	{
		if (codepoint < range[i].cp_min)
			return NULL;	//early out
		if (range[i].cp_max)
		{
			if (codepoint < range[i].cp_max)
				return &range[i];
		}
		else
		{
			if (codepoint == range[i].cp_min)
				return &range[i];
		}
	}
	return NULL;
}
#define StringPrep_InRange(cp,r) StringPrep_InRange_(cp,r,sizeof(r)/sizeof(r[0]))

static qboolean JCL_NamePrep(const char *in, size_t insize, char *out, size_t outsize)
{
	int j;
	unsigned int offset;
	unsigned int codepoint;
	struct stringprep_range *mapping;
	struct stringprep_range nomap;
	outsize--;
	while (insize --> 0)
	{
		codepoint = *in++;
		if (codepoint >= 0x80)
			return false;	//FIXME: utf-8 decode.

		if (StringPrep_InRange(codepoint, stringprep_A1)) return false;	//no unassigned codepoints

		mapping = StringPrep_InRange(codepoint, stringprep_B1);
		if (!mapping)
			mapping = StringPrep_InRange(codepoint, stringprep_B2);
		if (!mapping)
		{	//no mapping, its probably fine.
			nomap.cp_min = codepoint;
			nomap.cp_max = codepoint;
			nomap.remap[0] = codepoint;
			nomap.remap[1] = 0;
			mapping = &nomap;
		}
		offset = codepoint - mapping->cp_min;

		for (j = 0; j < sizeof(mapping->remap)/sizeof(mapping->remap[0]) && mapping->remap[j]; j++)
		{
			codepoint = mapping->remap[j] + offset;
			if (StringPrep_InRange(codepoint, stringprep_C1)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C2)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C3)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C4)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C5)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C6)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C7)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C8)) return false;
			if (StringPrep_InRange(codepoint, stringprep_C9)) return false;

			//FIXME: utf-8 encode
			if (outsize < 1)
				return false;
			outsize--;
			*out++ = codepoint;
		}
	}
	*out = 0;
	return true;
}

static qboolean JCL_NameResourcePrep(const char *in, char *nout, size_t noutsize, char **res)
{
	char *resstart = strchr(in, '/');
	if (resstart)
	{
		*res = resstart+1;
		//FIXME: resprep the resource.
		if (!JCL_NamePrep(in, resstart-in, nout, noutsize))
			return false;
	}
	else
	{
		*res = NULL;
		if (!JCL_NamePrep(in, strlen(in), nout, noutsize))
			return false;
	}
	return true;
}

//FIXME: add flags to avoid creation
qboolean JCL_FindBuddy(jclient_t *jcl, const char *jid, buddy_t **buddy, bresource_t **bres, qboolean create)
{
	char name[256];
	char *res;
	buddy_t *b;
	bresource_t *r = NULL;
	if (!jid || !*jid || !JCL_NameResourcePrep(jid, name, sizeof(name), &res))
	{	//no name / nameprep failed.
		if (buddy)
			*buddy = NULL;
		if (bres)
			*bres = NULL;
		return false;
	}

	for (b = jcl->buddies; b; b = b->next)
	{
		if (!strcmp(b->accountdomain, name))
			break;
	}
	if (!b && create)
	{
		b = malloc(sizeof(*b) + strlen(name));
		memset(b, 0, sizeof(*b));
		b->next = jcl->buddies;
		jcl->buddies = b;
//		b->vcardphotochanged = true;	//don't know what it is, query their photo as needed. google sucks, and things stop working.
		strcpy(b->accountdomain, name);
		Q_strlcpy(b->name, name, sizeof(b->name));	//default
	}
	*buddy = b;
	if (res && bres && b)
	{
		for (r = b->resources; r; r = r->next)
		{
			if (!strcmp(r->resource, res))
				break;
		}
		if (!r && create)
		{
			r = malloc(sizeof(*r) + strlen(res));
			memset(r, 0, sizeof(*r));
			r->next = b->resources;
			b->resources = r;
			strcpy(r->resource, res);
		}
		*bres = r;
	}
	else if (bres)
		*bres = NULL;

	if (bres)
		return *bres != NULL;
	return *buddy != NULL;
}

void JCL_IQTimeouts(jclient_t *jcl)
{
	struct iq_s *iq, **link;
	for (link = &jcl->pendingiqs; *link; )
	{
		iq = *link;
		if (iq->timeout < jclient_curtime)
		{
			iq = *link;
			*link = iq->next;
			if (iq->callback)
			{
				Con_DPrintf("IQ timeout with %s\n", iq->to);
				iq->callback(jcl, NULL, iq);
			}
			free(iq);
		}
		else
			link = &(*link)->next;
	}
}
struct iq_s *JCL_SendIQ(jclient_t *jcl, qboolean (*callback) (jclient_t *jcl, xmltree_t *tree, struct iq_s *iq), const char *iqtype, const char *target, const char *body)
{
	struct iq_s *iq;

	if (!target)
		target = "";

	iq = malloc(sizeof(*iq) + strlen(target));
	iq->next = jcl->pendingiqs;
	jcl->pendingiqs = iq;
	Q_snprintf(iq->id, sizeof(iq->id), "%i", rand());
	iq->callback = callback;
	iq->timeout = jclient_curtime + 30*1000;
	strcpy(iq->to, target);

	if (*target)
	{
		if (*jcl->fulljid)
			JCL_AddClientMessagef(jcl, "<iq type='%s' id='%s' from='%s' to='%s'>", iqtype, iq->id, jcl->fulljid, target);
		else
			JCL_AddClientMessagef(jcl, "<iq type='%s' id='%s' to='%s'>", iqtype, iq->id, target);
	}
	else
	{
		if (*jcl->fulljid)
			JCL_AddClientMessagef(jcl, "<iq type='%s' id='%s' from='%s'>", iqtype, iq->id, jcl->fulljid);
		else
			JCL_AddClientMessagef(jcl, "<iq type='%s' id='%s'>", iqtype, iq->id);
	}
	JCL_AddClientMessageString(jcl, body);
	JCL_AddClientMessageString(jcl, "</iq>");
	return iq;
}
struct iq_s *JCL_SendIQf(jclient_t *jcl, qboolean (*callback) (jclient_t *jcl, xmltree_t *tree, struct iq_s *iq), const char *iqtype, const char *target, const char *fmt, ...)
{
	va_list		argptr;
	char body[2048];

	va_start (argptr, fmt);
	Q_vsnprintxf (body, sizeof(body), fmt, argptr);
	va_end (argptr);

	return JCL_SendIQ(jcl, callback, iqtype, target, body);
}
struct iq_s *JCL_SendIQNode(jclient_t *jcl, qboolean (*callback) (jclient_t *jcl, xmltree_t *tree, struct iq_s *iq), const char *iqtype, const char *target, xmltree_t *node, qboolean destroynode)
{
	struct iq_s *n;
	char *s = XML_GenerateString(node, false);
	n = JCL_SendIQ(jcl, callback, iqtype, target, s);
	free(s);
	if (destroynode)
		XML_Destroy(node);
	return n;
}

#ifdef USE_GOOGLE_MAIL_NOTIFY
qboolean XMPP_NewGoogleMailsReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	int i, j;
	xmltree_t *mailbox = XML_ChildOfTreeNS(tree, "google:mail:notify", "mailbox", 0);
//	char *url = XML_GetParameter(mailbox, "url", "");
//	int totalmatched = atoi(XML_GetParameter(mailbox, "total-matched", "0"));
//	char *resulttime = XML_GetParameter(mailbox, "result-time", "");
	xmltree_t *mailthread;
	xmltree_t *senders, *sender;
	char *subject;
	char *sendername;
	char *cleanup;
	char *mailurl;

	for (i = 0; ; i++)
	{
		mailthread = XML_ChildOfTree(mailbox, "mail-thread-info", i);
		if (!mailthread)
			break;

//		tid = XML_GetParameter(mailthread, "tid", "");
		mailurl = XML_GetParameter(mailthread, "url", "");
//		participation = XML_GetParameter(mailthread, "participation", "");
//		messages = XML_GetParameter(mailthread, "messages", "");
//		date = XML_GetParameter(mailthread, "date", "");
//		labels = XML_GetChildBody(mailthread, "labels", "");
		subject = XML_GetChildBody(mailthread, "subject", "");
		if (!*subject)
			subject = XML_GetChildBody(mailthread, "snippet", "<NO SUBJECT>");

		senders = XML_ChildOfTree(mailthread, "senders", 0);
		for (j = 0; ; j++)
		{
			sender = XML_ChildOfTree(senders, "sender", j);
			if (!sender)
				break;
//			address = XML_GetParameter(sender, "address", "");
			sendername = XML_GetParameter(sender, "name", "");
			if (!*sendername)
				sendername = XML_GetParameter(sender, "address", "");
//			originator = XML_GetParameter(sender, "originator", "");
			if (atoi(XML_GetParameter(sender, "unread", "1")))
			{
				//we trust the server to not feed us gibberish like \r or \n chars.
				//however, other chars may be problematic and could break/hack the link markup.
				for (cleanup = sendername; (cleanup = strchr(cleanup, '^')) != NULL; )
					*cleanup = ' ';
				for (cleanup = sendername; (cleanup = strchr(cleanup, '\\')) != NULL; )
					*cleanup = '/';
				for (cleanup = subject; (cleanup = strchr(cleanup, '^')) != NULL; )
					*cleanup = ' ';
				for (cleanup = subject; (cleanup = strchr(cleanup, '\\')) != NULL; )
					*cleanup = '/';
				for (cleanup = mailurl; (cleanup = strstr(cleanup, "^]")) != NULL; )
					*cleanup = '_';	//FIXME: %5E
				for (cleanup = mailurl; (cleanup = strchr(cleanup, '\\')) != NULL; )
					*cleanup = '/';	//FIXME: %5C
				Con_Printf("^[^4New spam from %s: %s\\url\\%s^]\n", sendername, subject, mailurl);
			}
		}
	}
	return true;
}
#endif

qboolean XMPP_CarbonsEnabledReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	//don't care. if we don't get carbons then no real loss.
	//xmltree_t *etype, *error = XML_ChildOfTreeNS(tree, NULL, "error", 0);
	//etype = XML_ChildOfTreeNS(error, NULL, "forbidden", 0);
	//etype = XML_ChildOfTreeNS(error, NULL, "not-allowed", 0);
	return true;
}

static void JCL_RosterUpdate(jclient_t *jcl, xmltree_t *listp, const char *from)
{
	xmltree_t *i;
	buddy_t *buddy;
	int cnum = 0;
	const char *at = strrchr(from, '@');
	if (at)
	{
		if (strlen(jcl->username) != at-from || strncasecmp(from, jcl->username, at-from))
			return;
		from = at+1;
	}
	if (strcmp(from, jcl->domain))
		return;	//ignore if from somewhere invalid

	while ((i = XML_ChildOfTree(listp, "item", cnum++)))
	{
		const char *name = XML_GetParameter(i, "name", "");
		const char *jid = XML_GetParameter(i, "jid", "");
		const char *sub = XML_GetParameter(i, "subscription", "both");
		const char *ask = XML_GetParameter(i, "ask", "");
		JCL_FindBuddy(jcl, jid, &buddy, NULL, true);
		buddy->btype = BT_ROSTER;

		if (*name)
			Q_strlcpy(buddy->name, name, sizeof(buddy->name));
		else
			buddy->vcardphotochanged = true;	//try to query their actual name
		if (strcasecmp(sub, "none"))
			buddy->friended = true;
		if (*ask)
			buddy->askfriend = true;
	}
	jclient_updatebuddylist = true;
}
static qboolean JCL_RosterReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	xmltree_t *c;
	//we're probably connected once we've had this reply.
	jcl->status = JCL_ACTIVE;
	JCL_GeneratePresence(jcl, true);
	c = XML_ChildOfTree(tree, "query", 0);
	if (c)
	{
		JCL_RosterUpdate(jcl, c, jcl->domain);
		return true;
	}
	return false;
}

static qboolean JCL_BindReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	xmltree_t *c;
	c = XML_ChildOfTree(tree, "bind", 0);
	if (c)
	{
		c = XML_ChildOfTree(c, "jid", 0);
		if (c)
		{
			char myjid[512];
			Q_strlcpy(jcl->fulljid, c->body, sizeof(jcl->fulljid));
			JCL_GenLink(jcl, myjid, sizeof(myjid), NULL, jcl->fulljid, NULL, NULL, "%s", jcl->fulljid);
			Con_DPrintf("Bound to jid %s\n", jcl->fulljid);
			return true;
		}
	}
	return false;
}
static qboolean JCL_BuddyVCardReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	char myself[512];
	char photodata[65536];
	xmltree_t *vc, *photo, *photobinval;
	const char *nickname;
	const char *photomime;

	buddy_t *b = NULL;
	char *from = iq->to;

	if (!*from)
	{
		Q_snprintf(myself, sizeof(myself), "%s@%s", jcl->username, jcl->domain);
		from = myself;
	}

	if (jcl->avatarupdate == iq)
	{
		if (!tree)
			Con_DPrintf("timeout for %s's photo\n", iq->to);
		else
			Con_DPrintf("Response for %s's photo\n", iq->to);
		jcl->avatarupdate = NULL;
	}

	JCL_FindBuddy(jcl, from, &b, NULL, true);
	if (!b)
	{
		Con_DPrintf("unknown vcard from %s\n", from);
		return false;
	}

	jclient_updatebuddylist = true;	//make sure any new info is displayed properly.

	vc = XML_ChildOfTree(tree, "vCard", 0);
	if (!vc)
		Con_DPrintf("invalid vcard from %s\n", from);
	else
	{
		if (!strcmp(b->name, b->accountdomain))
		{
			nickname = XML_GetChildBody(vc, "NICKNAME", NULL);
			if (!nickname)
				nickname = XML_GetChildBody(vc, "FN", NULL);
			if (nickname)
				Q_strlcpy(b->name, nickname, sizeof(b->name));
		}

		photo = XML_ChildOfTree(vc, "PHOTO", 0);
		photobinval = XML_ChildOfTree(photo, "BINVAL", 0);

		if ((jcl->enabledcapabilities & CAP_AVATARS) && drawfuncs)
		{
			if (photobinval)
			{
				struct buddyinfo_s *bi;
				unsigned int photosize = Base64_Decode(photodata, sizeof(photodata), photobinval->body, strlen(photobinval->body));
				photomime = XML_GetChildBody(photo, "TYPE", "");
				//xep-0153: If the <TYPE/> is something other than image/gif, image/jpeg, or image/png, it SHOULD be ignored.
				if (strcmp(photomime, "image/png") && strcmp(photomime, "image/jpeg") && strcmp(photomime, "image/gif"))
					photomime = "";
				b->image = drawfuncs->LoadImageData(va("xmpp/%s.png", b->accountdomain), photomime, photodata, photosize);
				Con_DPrintf("vcard photo updated from %s\n", from);

				for (bi = jcl->buddyinfo; bi; bi = bi->next)
				{
					if (!strcmp(bi->accountdomain, b->accountdomain))
						break;
				}
				if (!bi)
				{
					bi = malloc(sizeof(*bi)+strlen(b->accountdomain));
					memset(bi, 0, sizeof(*bi));
					strcpy(bi->accountdomain, b->accountdomain);
					bi->next = jcl->buddyinfo;
					jcl->buddyinfo = bi;
				}

				if (bi)
				{
					char *hex = "0123456789abcdef";
					char hash[20];
					char hasha[41];
					int i, o;
					free(bi->image);
					free(bi->imagehash);
					free(bi->imagemime);
					CalcHash(&hash_sha1, hash, sizeof(hash), photodata, photosize);
					for (i = 0, o = 0; i < sizeof(hash); i++)
					{
						hasha[o++] = hex[(hash[i]>>4) & 0xf];
						hasha[o++] = hex[(hash[i]>>0) & 0xf];
					}
					hasha[o] = 0;
					bi->imagehash = strdup(hasha);
					bi->image = strdup(photobinval->body);
					bi->imagemime = strdup(photomime);

					jclient_configdirty = true;
				}
			}
			else
			{
				b->image = drawfuncs->LoadImageData(va("xmpp/%s.png", b->accountdomain), "", NULL, 0);
				Con_DPrintf("vcard photo invalidated from %s\n", from);
			}
		}
		else
			Con_DPrintf("vcard photo ignored from %s\n", from);
	}

	return true;
}
static qboolean JCL_MyVCardReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	char photodata[65536];
	char digest[20];
	xmltree_t *vc, *fn, *nickname, *photo, *photobinval;

	//hack the from parametmer so it looks legit
	Q_snprintf(photodata, sizeof(photodata), "%s@%s", jcl->username, jcl->domain);

	//make sure our image is loaded etc
	JCL_BuddyVCardReply(jcl, tree, iq);

	vc = XML_ChildOfTree(tree, "vCard", 0);
	fn = XML_ChildOfTree(vc, "FN", 0);
	nickname = XML_ChildOfTree(vc, "NICKNAME", 0);

	photo = XML_ChildOfTree(vc, "PHOTO", 0);
	photobinval = XML_ChildOfTree(photo, "BINVAL", 0);
	if (!tree || !photobinval)
	{
		//server doesn't support vcards?
		if (jcl->vcardphotohashstatus != VCP_NONE)
		{
			jcl->vcardphotohashstatus = VCP_NONE;
			jcl->vcardphotohashchanged = true;
			*jcl->vcardphotohash = 0;
		}
	}
	else
	{
		int photosize = Base64_Decode(photodata, sizeof(photodata), photobinval->body, strlen(photobinval->body));
		CalcHash(&hash_sha1, digest, sizeof(digest), photodata, photosize);
		if (jcl->vcardphotohashstatus != VCP_KNOWN || memcmp(jcl->vcardphotohash, digest, sizeof(jcl->vcardphotohash)))
		{
			memcpy(jcl->vcardphotohash, digest, sizeof(jcl->vcardphotohash));
			jcl->vcardphotohashchanged = true;
			jcl->vcardphotohashstatus = VCP_KNOWN;
		}
	}

	if (nickname && *nickname->body)
		Q_strlcpy(jcl->localalias, nickname->body, sizeof(jcl->localalias));
	else if (fn && *fn->body)
		Q_strlcpy(jcl->localalias, fn->body, sizeof(jcl->localalias));
	else
		Q_strlcpy(jcl->localalias, jcl->barejid, sizeof(jcl->localalias));	//barejid or just username?
	return true;
}
static qboolean JCL_ServerFeatureReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	xmltree_t *query = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/disco#info", "query", 0);
	xmltree_t *feature;
	const char *featurename;
	int f;
#ifdef USE_GOOGLE_MAIL_NOTIFY
	qboolean gmail = false;
#endif
	qboolean carbons = false;

	if (!query)
		return false;

	for (f = 0; ; f++)
	{
		feature = XML_ChildOfTree(query, "feature", f);
		if (!feature)
			break;
		featurename = XML_GetParameter(feature, "var", "");
#ifdef USE_GOOGLE_MAIL_NOTIFY
		if (!strcmp(featurename, "google:mail:notify"))
			gmail = true;
		else
#endif
			if (!strcmp(featurename, "urn:xmpp:carbons:2"))
			carbons = true;
		else
		{
			Con_DPrintf("Server supports feature %s\n", featurename);
		}
	}

#ifdef USE_GOOGLE_MAIL_NOTIFY
	if (gmail)
		JCL_SendIQf(jcl, XMPP_NewGoogleMailsReply, "get", NULL, "<query xmlns='google:mail:notify'/>");
#endif
	if (carbons)
		JCL_SendIQf(jcl, XMPP_CarbonsEnabledReply, "set", NULL, "<enable xmlns='urn:xmpp:carbons:2'/>");
	
	return true;
}
/*static qboolean JCL_ServerPeerReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	xmltree_t *query = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/disco#info", "query", 0);
	xmltree_t *feature;
	xmltree_t *identity;
	const char *from = iq->to;
	const char *name;
	const char *type;
	const char *category;
	int f;

	if (!query)
		return false;

	for (f = 0; ; f++)
	{
		identity = XML_ChildOfTree(query, "identity", f);
		if (!identity)
			break;
		name = XML_GetParameter(identity, "name", "");
		type = XML_GetParameter(identity, "type", "");
		category = XML_GetParameter(identity, "category", "");
		Con_Printf("Server %s supports identity type=%s category=%s name=%s\n", from, type, category, name);
	}

	for (f = 0; ; f++)
	{
		feature = XML_ChildOfTree(query, "feature", f);
		if (!feature)
			break;
		name = XML_GetParameter(feature, "var", "");
		Con_Printf("Server %s supports feature %s\n", from, name);
	}

	return true;
}
static qboolean JCL_ServerItemsReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	xmltree_t *query = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/disco#items", "query", 0);
	xmltree_t *item;
	const char *itemname;
	int f;

	if (!query)
		return false;

	for (f = 0; ; f++)
	{
		item = XML_ChildOfTree(query, "item", f);
		if (!item)
			break;
		itemname = XML_GetParameter(item, "jid", "");
		if (*itemname)
		{
			Con_DPrintf("Server reports peer %s\n", itemname);
			JCL_SendIQf(jcl, JCL_ServerPeerReply, "get", itemname, "<query xmlns='http://jabber.org/protocol/disco#info'/>");
		}
	}
	return true;
}*/
static qboolean JCL_SessionReply(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	buddy_t *b;
	JCL_SendIQf(jcl, JCL_RosterReply, "get", NULL, "<query xmlns='jabber:iq:roster'/>");
	JCL_SendIQf(jcl, JCL_MyVCardReply, "get", jcl->barejid, "<vCard xmlns='vcard-temp'/>");
	JCL_SendIQf(jcl, JCL_ServerFeatureReply, "get", jcl->domain, "<query xmlns='http://jabber.org/protocol/disco#info'/>");
//	JCL_SendIQf(jcl, JCL_ServerItemsReply, "get", jcl->domain, "<query xmlns='http://jabber.org/protocol/disco#items'/>");

	for (b = jcl->buddies; b; b = b->next)
	{
		if (b->btype == BT_CHATROOM)
			if (b->room_autojoin)
				JCL_JoinMUCChat(jcl, b->accountdomain, NULL, b->room_nick, b->room_password);
	}
	return true;
}

static struct
{
	char *name;
	unsigned int withcap;
} caps[] =
{
#if 1
	{"http://jabber.org/protocol/caps"},
	{"http://jabber.org/protocol/disco#info"},
	{"http://jabber.org/protocol/disco#items"},

	{"jabber:iq:version"},
	#ifdef JINGLE
		{"urn:xmpp:jingle:1", CAP_GAMEINVITE|CAP_VOICE|CAP_VIDEO},
		{QUAKEMEDIAXMLNS, CAP_GAMEINVITE},
		#ifdef VOIP
			#ifdef VOIP_LEGACY
				{"http://www.google.com/xmpp/protocol/session", CAP_GOOGLE_VOICE},	//so google's non-standard clients can chat with us
				{"http://www.google.com/xmpp/protocol/voice/v1", CAP_GOOGLE_VOICE}, //so google's non-standard clients can chat with us
//				{"http://www.google.com/xmpp/protocol/camera/v1", CAP_GOOGLE_VOICE},	//can send video
//				{"http://www.google.com/xmpp/protocol/video/v1", CAP_GOOGLE_VOICE},	//can receive video
			#endif
			#ifndef VOIP_LEGACY_ONLY
				{"urn:xmpp:jingle:apps:rtp:1", CAP_VOICE|CAP_VIDEO},
				{"urn:xmpp:jingle:apps:rtp:audio", CAP_VOICE},
				{"urn:xmpp:jingle:apps:rtp:video", CAP_VIDEO},
			#endif
//			{"urn:xmpp:jingle:apps:rtp:video", CAP_VIDEO},	//we don't support rtp video chat
		#endif
		{"urn:xmpp:jingle:transports:raw-udp:1", CAP_GAMEINVITE|CAP_VOICE|CAP_VIDEO},
		#ifndef NOICE
			{"urn:xmpp:jingle:transports:ice-udp:1", CAP_GAMEINVITE|CAP_VOICE|CAP_VIDEO},
		#endif
	#endif
	#ifndef Q3_VM
		{"urn:xmpp:time"},
	#endif
	{"urn:xmpp:ping"},	//FIXME: I'm not keen on this. I only added support to stop errors from pidgin when trying to debug.
	{"urn:xmpp:attention:0"},	//poke.

	//file transfer
	#ifdef FILETRANSFERS
		{"http://jabber.org/protocol/si", CAP_SIFT},
		{"http://jabber.org/protocol/si/profile/file-transfer", CAP_SIFT},
		{"http://jabber.org/protocol/ibb", CAP_SIFT},
		{"http://jabber.org/protocol/bytestreams", CAP_SIFT},
	#endif

	{"urn:xmpp:avatar:data"},
	{"urn:xmpp:avatar:metadata"},
	{"urn:xmpp:avatar:metadata+notify"},
//	{"http://jabber.org/protocol/mood"},
//	{"http://jabber.org/protocol/mood+notify"},
///	{"http://jabber.org/protocol/tune"},
//	{"http://jabber.org/protocol/tune+notify"},
	{"http://jabber.org/protocol/nick"},
	{"http://jabber.org/protocol/nick+notify"},

	{"http://jabber.org/protocol/commands"},
#else
	//for testing, this is the list of features pidgin supports (which is the other client I'm testing against).

	"jabber:iq:last",
	"jabber:iq:oob",
	"urn:xmpp:time",
	"jabber:iq:version",
	"jabber:x:conference",
	"http://jabber.org/protocol/bytestreams",
	"http://jabber.org/protocol/caps",
	"http://jabber.org/protocol/chatstates",
	"http://jabber.org/protocol/disco#info",
	"http://jabber.org/protocol/disco#items",
	"http://jabber.org/protocol/muc",
	"http://jabber.org/protocol/muc#user",
	"http://jabber.org/protocol/si",
	"http://jabber.org/protocol/si/profile/file-transfer",
	"http://jabber.org/protocol/xhtml-im",
	"urn:xmpp:ping",
	"urn:xmpp:attention:0",
	"urn:xmpp:bob",
	"urn:xmpp:jingle:1",
	"http://www.google.com/xmpp/protocol/session",
	"http://www.google.com/xmpp/protocol/voice/v1",
	"http://www.google.com/xmpp/protocol/video/v1",
	"http://www.google.com/xmpp/protocol/camera/v1",
	"urn:xmpp:jingle:apps:rtp:1",
	"urn:xmpp:jingle:apps:rtp:audio",
	"urn:xmpp:jingle:apps:rtp:video",
	"urn:xmpp:jingle:transports:raw-udp:1",
	"urn:xmpp:jingle:transports:ice-udp:1",
	"urn:xmpp:avatar:metadata",
	"urn:xmpp:avatar:data",
	"urn:xmpp:avatar:metadata+notify",
	"http://jabber.org/protocol/mood",
	"http://jabber.org/protocol/mood+notify",
	"http://jabber.org/protocol/tune",
	"http://jabber.org/protocol/tune+notify",
	"http://jabber.org/protocol/nick",
	"http://jabber.org/protocol/nick+notify",
	"http://jabber.org/protocol/ibb",
#endif
	{NULL}
};
static void buildcaps(jclient_t *jcl, char *out, int outlen)
{
	int i;
	Q_strncpyz(out, "<identity category='client' type='pc' name='FTEQW'/>", outlen);

	for (i = 0; caps[i].name; i++)
	{
		if (caps[i].withcap && !(caps[i].withcap & jcl->enabledcapabilities))
			continue;
		Q_strlcat(out, "<feature var='", outlen);
		Q_strlcat(out, caps[i].name, outlen);
		Q_strlcat(out, "'/>", outlen);
	}
}
static int qsortcaps(const void *va, const void *vb)
{
	char *const a = *(char*const *)va;
	char *const b = *(char*const *)vb;
	return strcmp(a, b);
}
char *buildcapshash(jclient_t *jcl)
{
	int i, l;
	char out[8192];
	int outlen = sizeof(out);
	unsigned char digest[64];
	Q_strlcpy(out, "client/pc//FTEQW<", outlen);
	qsort(caps, sizeof(caps)/sizeof(caps[0]) - 1, sizeof(caps[0]), qsortcaps); 
	for (i = 0; caps[i].name; i++)
	{
		if (caps[i].withcap && !(caps[i].withcap & jcl->enabledcapabilities))
			continue;
		Q_strlcat(out, caps[i].name, outlen);
		Q_strlcat(out, "<", outlen);
	}
	l = CalcHash(&hash_sha1, digest, sizeof(digest), out, strlen(out));
	for (i = 0; i < l; i++)
		Base64_Byte(digest[i]);
	Base64_Finish();
	return base64;
}

//xep-0115 1.4+
//xep-0153
char *buildcapsvcardpresence(jclient_t *jcl, char *caps, size_t sizeofcaps)
{
	char *vcard;
	char *voiceext = "";	//xep-0115 1.0
#ifdef VOIP_LEGACY
	if (jcl->enabledcapabilities & CAP_GOOGLE_VOICE)
	{
		if (jcl->enabledcapabilities & CAP_VIDEO)
			voiceext = " ext='voice-v1 camera-v1 video-v1'";
		else
			voiceext = " ext='voice-v1'";
	}
#endif

	Q_snprintf(caps, sizeofcaps,
		"<c xmlns='http://jabber.org/protocol/caps'"
		" hash='sha-1'"
		" node='"DISCONODE"'"
		" ver='%s'"
		"%s/>"
		, buildcapshash(jcl), voiceext);

	//xep-0153
	vcard = caps+strlen(caps);
	sizeofcaps -= strlen(caps);
	if (jcl->vcardphotohashstatus == VCP_NONE)
	{
		//let other people know that we don't have one. yay. pointless. whatever.
		Q_snprintf(vcard, sizeofcaps,
				"<x xmlns='vcard-temp:x:update'><photo/></x>");
	}
	else if (jcl->vcardphotohashstatus == VCP_KNOWN)
	{
		unsigned char *hex = "0123456789abcdef";
		char inhex[41];
		int i, o;
		for (i = 0, o = 0; i < sizeof(jcl->vcardphotohash); i++)
		{
			inhex[o++] = hex[(jcl->vcardphotohash[i]>>4) & 0xf];
			inhex[o++] = hex[(jcl->vcardphotohash[i]>>0) & 0xf];
		}
		inhex[o] = 0;

		//if we know the vcard hash, we must tell other people what it is or if its changed, etc.
		Q_snprintf(vcard, sizeofcaps,
			"<x xmlns='vcard-temp:x:update'><photo>%s</photo></x>", inhex);
	}
	else
	{
		//always include a vcard update tag.
		//this says that we won't corrupt other resource's vcard.
		//note that googletalk seems to hack the current vcard hash anyway. don't test this feature on that network.
		Q_snprintf(vcard, sizeofcaps,
			"<x xmlns='vcard-temp:x:update'/>");
	}
	return caps;
}

void JCL_ParseIQ(jclient_t *jcl, xmltree_t *tree)
{
	qboolean unparsable = true;
	const char *from;
//	const char *to;
	const char *id;
	const char *f;
	xmltree_t *ot;

	//FIXME: block from people who we don't know.

	id = XML_GetParameter(tree, "id", "");
	from = XML_GetParameter(tree, "from", jcl->domain);
//	to = XML_GetParameter(tree, "to", "");

	f = XML_GetParameter(tree, "type", "");
	if (!strcmp(f, "get"))
	{
		ot = XML_ChildOfTree(tree, "query", 0);
		if (ot)
		{
			if (from && !strcmp(ot->xmlns, "http://jabber.org/protocol/disco#info"))
			{	//http://xmpp.org/extensions/xep-0030.html
				char msg[2048];
				char hashednode[1024];
				const char *node = XML_GetParameter(ot, "node", NULL);

				buildcaps(jcl, msg, sizeof(msg));
				Q_snprintf(hashednode, sizeof(hashednode),DISCONODE"#%s", buildcapshash(jcl));

				if (!node || !strcmp(node, hashednode) || !strcmp(node, DISCONODE"#") )
				{
					unparsable = false;
					JCL_AddClientMessagef(jcl,
							"<iq type='result' to='%s' id='%s'>"
								"<query xmlns='http://jabber.org/protocol/disco#info' node='%s'>"
									"%+s"
								"</query>"
							"</iq>", from, id, node?node:hashednode, msg);
				}
#ifdef VOIP_LEGACY
				else if (!strcmp(node, DISCONODE"#voice-v1") && (jcl->enabledcapabilities & CAP_GOOGLE_VOICE))
				{
					unparsable = false;
					JCL_AddClientMessagef(jcl,
							"<iq type='result' to='%s' id='%s'>"
								"<query xmlns='http://jabber.org/protocol/disco#info' node='%s'>"
									"<feature var='http://www.google.com/xmpp/protocol/voice/v1'/>"
								"</query>"
							"</iq>", from, id, node, msg);
				}
				else if (!strcmp(node, DISCONODE"#camera-v1") && (jcl->enabledcapabilities & CAP_GOOGLE_VOICE))
				{
					unparsable = false;
					JCL_AddClientMessagef(jcl,
							"<iq type='result' to='%s' id='%s'>"
								"<query xmlns='http://jabber.org/protocol/disco#info' node='%s'>"
									"<feature var='http://www.google.com/xmpp/protocol/camera/v1'/>"
								"</query>"
							"</iq>", from, id, node, msg);
				}
				else if (!strcmp(node, DISCONODE"#video-v1") && (jcl->enabledcapabilities & CAP_GOOGLE_VOICE))
				{
					unparsable = false;
					JCL_AddClientMessagef(jcl,
							"<iq type='result' to='%s' id='%s'>"
								"<query xmlns='http://jabber.org/protocol/disco#info' node='%s'>"
									"<feature var='http://www.google.com/xmpp/protocol/video/v1'/>"
								"</query>"
							"</iq>", from, id, node, msg);
				}
#endif
				else
				{
					JCL_AddClientMessagef(jcl,
							"<iq type='error' to='%s' id='%s'>"
								"<error code='404' type='cancel'>"
									"<item-not-found/>"
								"</error>"
							"</iq>", from, id);
				}
			}
			else if (from && !strcmp(ot->xmlns, "http://jabber.org/protocol/disco#items"))
			{	//http://xmpp.org/extensions/xep-0030.html
				const char *node = XML_GetParameter(ot, "node", NULL);
				if (!strcmp(node, "http://jabber.org/protocol/commands"))
				{
					const char *to = XML_GetParameter(tree, "to", "");
					unparsable = false;
					JCL_AddClientMessagef(jcl,
							"<iq type='result' to='%s' id='%s'>"
								"<query xmlns='http://jabber.org/protocol/disco#items' node='%s'>"
									"<item jid='%s' node='rcon' name='Toggle Remote Control'/>"
								"</query>"
							"</iq>", from, id, node, to);
				}
				else
				{	//send back an empty list if its something we don't recognise
					JCL_AddClientMessagef(jcl,
							"<iq type='result' to='%s' id='%s'>"
								"<query xmlns='http://jabber.org/protocol/disco#items' node='%s' />"
							"</iq>", from, id, node);
				}
			}
			else if (from && !strcmp(ot->xmlns, "jabber:iq:version"))
			{	//client->client version request
				char msg[2048];
				unparsable = false;

				Q_snprintf(msg, sizeof(msg),
						"<iq type='result' to='%s' id='%s'>"
							"<query xmlns='jabber:iq:version'>"
								"<name>FTEQW XMPP</name>"
								"<version>V"JCL_BUILD"</version>"
#ifdef Q3_VM
								"<os>QVM plugin</os>"
#else
								//don't specify the os otherwise, as it gives away required base addresses etc for exploits
#endif
							"</query>"
						"</iq>", from, id);

				JCL_AddClientMessageString(jcl, msg);
			}
			else if (from && !strcmp(ot->xmlns, "jabber:iq:last"))
			{
				unparsable = false;
				JCL_AddClientMessagef(jcl,
						"<iq type='error' to='%s' id='%s'>"
							"<error type='cancel'>"
								"<service-unavailable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
							"</error>"
						"</iq>", from, id);
			}
/*			else if (from && !strcmp(ot->xmlns, "jabber:iq:last"))
			{	//http://xmpp.org/extensions/xep-0012.html
				char msg[2048];
				int idletime = 0;
				unparsable = false;

				//last activity
				Q_snprintf(msg, sizeof(msg),
						"<iq type='result' to='%s' id='%s'>"
							"<query xmlns='jabber:iq:last' seconds='%i'/>"
						"</iq>", from, id, idletime);
				
				JCL_AddClientMessageString(jcl, msg);
			}
*/
		}
#ifndef Q3_VM
		ot = XML_ChildOfTree(tree, "time", 0);
		if (ot && !strcmp(ot->xmlns, "urn:xmpp:time"))
		{	//http://xmpp.org/extensions/xep-0202.html
			char msg[2048];
			char tz[256];
			char timestamp[256];
			struct tm * timeinfo;
			int tzh, tzm;
			time_t rawtime;
			time (&rawtime);
			timeinfo = localtime(&rawtime);
			tzh = timeinfo->tm_hour;
			tzm = timeinfo->tm_min;
			timeinfo = gmtime (&rawtime);
			tzh -= timeinfo->tm_hour;
			tzm -= timeinfo->tm_min;
			Q_snprintf(tz, sizeof(tz), "%+i:%i", tzh, tzm);
			strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
			unparsable = false;
			//strftime
			Q_snprintf(msg, sizeof(msg),
					"<iq type='result' to='%s' id='%s'>"
						"<time xmlns='urn:xmpp:time'>"
							"<tzo>%s</tzo>"
							"<utc>%s</utc>"
						"</time>"
					"</iq>", from, id, tz, timestamp);
			JCL_AddClientMessageString(jcl, msg);
		}
#endif

		ot = XML_ChildOfTree(tree, "ping", 0);
		if (ot && !strcmp(ot->xmlns, "urn:xmpp:ping"))
		{
			JCL_AddClientMessagef(jcl, "<iq type='result' to='%s' id='%s' />", from, id);
		}
			
		if (unparsable)
		{	//unsupported stuff
			char msg[2048];
			unparsable = false;

			Con_Printf("Unsupported iq get\n");
			XML_ConPrintTree(tree, "", 0);

			//tell them OH NOES, instead of requiring some timeout.
			Q_snprintf(msg, sizeof(msg),
					"<iq type='error' to='%s' id='%s'>"
						"<error type='cancel'>"
							"<service-unavailable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
						"</error>"
					"</iq>", from, id);
			JCL_AddClientMessageString(jcl, msg);
		}
	}
	else if (!strcmp(f, "set"))
	{
		xmltree_t *c;

#ifdef FILETRANSFERS
		if (XMPP_FT_ParseIQSet(jcl, from, id, tree))
			return;
#endif

		c = XML_ChildOfTree(tree, "query", 0);
		if (c && !strcmp(c->xmlns, "jabber:iq:roster"))
		{
			unparsable = false;
			if (strcmp(from, jcl->domain))
				Con_Printf("Roster push from unknown entity: \"%s\"\n", from);
			else
				JCL_RosterUpdate(jcl, c, from);
		}

		c = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/commands", "command", 0);
		if (c)
		{
			char *s;
			const char *key, *val;
			xmltree_t *rep, *cmd, *x, *fld;
			const char *node = XML_GetParameter(c, "node", "");
			char *completed = NULL;
			char *canceled = NULL;
			char infostring[65536];
			int i;
			unparsable = false;

			*infostring = 0;
			x = XML_ChildOfTreeNS(c, "jabber:x:data", "x", 0);
			for (i = 0; ; i++)
			{
				fld = XML_ChildOfTree(x, "field", i);
				if (!fld)
					break;
				key = XML_GetParameter(fld, "var", "");
				val = XML_GetChildBody(fld, "value", "");
				if (*key && *val)
				{
					Q_strlcat(infostring, "\\", sizeof(infostring));
					Q_strlcat(infostring, key, sizeof(infostring));
					Q_strlcat(infostring, "\\", sizeof(infostring));
					Q_strlcat(infostring, val, sizeof(infostring));
				}
			}

			if (!strcmp(XML_GetParameter(c, "action", "submit"), "cancel"))
				canceled = "";
			else if (!strcmp(node, "rcon"))
			{
				char pwd[64];
				JCL_Info_ValueForKey(infostring, "password", pwd, sizeof(pwd));
				if (*pwd)
				{
					char rcon_password[64];
					cvarfuncs->GetString("rcon_password", rcon_password, sizeof(rcon_password));
					if (*rcon_password && !strcmp(JCL_Info_ValueForKey(infostring, "password", pwd, sizeof(pwd)), rcon_password))
					{
						Q_strlcpy(jcl->rcon_peer, from, sizeof(jcl->rcon_peer));
						if (jcl->rcon_pipe >= 0)
							filefuncs->Close(jcl->rcon_pipe);
						if (confuncs)
							jcl->rcon_pipe = confuncs->POpen(NULL);

						if (jcl->rcon_pipe < 0)
							canceled = "Unable to read console";
						else
							completed = "Rcon Enabled";
					}
					else
						completed = "Invalid Password";
				}
			}
			else
				canceled = "Unrecognised request";

			rep = XML_CreateNode(NULL, "iq", NULL, "");
			XML_AddParameter(rep, "type", "result");
			XML_AddParameter(rep, "id", id);
			XML_AddParameter(rep, "to", from);
			cmd = XML_CreateNode(rep, "command", "http://jabber.org/protocol/commands", "");
			XML_AddParameter(cmd, "sessionid", XML_GetParameter(c, "sessionid", id));
			XML_AddParameter(cmd, "node", node);

			if (canceled)
			{
				if (*canceled)
				{
					XML_AddParameter(cmd, "status", "completed");
					XML_AddParameter(XML_CreateNode(cmd, "note", NULL, canceled), "type", "error");
				}
				else
					XML_AddParameter(cmd, "status", "canceled");
			}
			else if (completed)
			{
				XML_AddParameter(cmd, "status", "completed");
				if (*completed)
					XML_AddParameter(XML_CreateNode(cmd, "note", NULL, completed), "type", "info");
			}
			else
			{
				XML_AddParameter(cmd, "status", "executing");

//				x = XML_CreateNode(cmd, "actions", NULL, "");
//				XML_AddParameter(x, "execute", "complete");
//				XML_CreateNode(x, "complete", NULL, NULL);

//				x = XML_CreateNode(cmd, "actions", NULL, "");
//				XML_AddParameter(x, "execute", "next");
//				XML_CreateNode(x, "next", NULL, NULL);

				x = XML_CreateNode(cmd, "x", "jabber:x:data", "");
				XML_AddParameter(x, "type", "result");
				XML_CreateNode(x, "title", NULL, "Enable RCon");
				XML_CreateNode(x, "instructions", NULL, "RCon password required...\nWARNING: This is NOT end-to-end encrypted.");
				fld = XML_CreateNode(x, "field", NULL, NULL);
				XML_AddParameter(fld, "var", "password");
				XML_AddParameter(fld, "type", "text-private");
				XML_AddParameter(fld, "label", "Rcon Password");
				XML_CreateNode(fld, "value", NULL, "");
			}
			s = XML_GenerateString(rep, false);
			XML_Destroy(rep);
			JCL_AddClientMessagef(jcl, s, from, id);
			free(s);
		}

#ifdef USE_GOOGLE_MAIL_NOTIFY
		//google-specific - new mail notifications.
		c = XML_ChildOfTree(tree, "new-mail", 0);
		if (c && !strcmp(c->xmlns, "google:mail:notify") && !strcmp(from, jcl->domain))
		{
			JCL_AddClientMessagef(jcl, "<iq type='result' to='%s' id='%s' />", from, id);
			JCL_SendIQf(jcl, XMPP_NewGoogleMailsReply, "get", "", "<query xmlns='google:mail:notify'/>");
			return;
		}
#endif

#ifdef JINGLE
		c = XML_ChildOfTreeNS(tree, "urn:xmpp:jingle:1", "jingle", 0);
		if (c && (jcl->enabledcapabilities & (CAP_GAMEINVITE|CAP_VOICE|CAP_VIDEO)))
		{
			unparsable = !JCL_ParseJingle(jcl, c, from, id);
			jclient_updatebuddylist = true;
		}
#ifdef VOIP_LEGACY
		c = XML_ChildOfTreeNS(tree, "http://www.google.com/session", "session", 0);
		if (c && (jcl->enabledcapabilities & (CAP_GOOGLE_VOICE)))
		{
			unparsable = !JCL_HandleGoogleSession(jcl, c, from, id);
			jclient_updatebuddylist = true;
		}
#endif
#endif



		if (unparsable)
		{
			char msg[2048];
			//tell them OH NOES, instead of requiring some timeout.
			Q_snprintf(msg, sizeof(msg),
					"<iq type='error' to='%s' id='%s'>"
						"<error type='cancel'>"
							"<service-unavailable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
						"</error>"
					"</iq>", from, id);
			JCL_AddClientMessageString(jcl, msg);
			unparsable = false;
		}
	}
	else if (!strcmp(f, "result") || !strcmp(f, "error"))
	{
		const char *id = XML_GetParameter(tree, "id", "");
		struct iq_s **link, *iq;
		const char *from = XML_GetParameter(tree, "from", jcl->domain);
		unparsable = false;
		for (link = &jcl->pendingiqs; *link; link = &(*link)->next)
		{
			iq = *link;
			if (!strcmp(iq->id, id) && (!strcmp(iq->to, from) || !*iq->to))
				break;
		}
		if (*link)
		{
			iq = *link;
			*link = iq->next;

			if (iq->callback)
			{
				if (!iq->callback(jcl, !strcmp(f, "error")?NULL:tree, iq))
				{
					Con_Printf("Invalid iq result\n");
					XML_ConPrintTree(tree, "", 0);
				}
			}
			free(iq);
		}
		else
		{
			Con_Printf("Unrecognised iq result from %s\n", from);
//			XML_ConPrintTree(tree, "", 0);
		}
	}
	
	if (unparsable)
	{
		unparsable = false;
		Con_Printf("Unrecognised iq type\n");
		XML_ConPrintTree(tree, "", 0);
	}
}
void XMPP_ConversationPrintf(const char *context, const char *title, qboolean takefocus, char *format, ...)
{
	va_list		argptr;
	static char		string[1024];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format,argptr);
	va_end (argptr);

	if (*context && confuncs && confuncs->GetConsoleFloat(context, "iswindow") < true)
	{
		confuncs->SetConsoleFloat(context, "iswindow", true);
		confuncs->SetConsoleFloat(context, "forceutf8", true);
		confuncs->SetConsoleFloat(context, "wnd_w", 256);
		confuncs->SetConsoleFloat(context, "wnd_h", 320);
		confuncs->SetConsoleString(context, "title", title);
	}
	Con_TrySubPrint(context, string);
}
void JCL_ParseMessage(jclient_t *jcl, xmltree_t *tree)
{
	xmltree_t *ot;
	qboolean unparsable = true;
	const char *f = XML_GetParameter(tree, "from", jcl->domain);
	const char *type = XML_GetParameter(tree, "type", "normal");
	const char *ctx = f;

	xmltree_t *received = XML_ChildOfTree(tree, "received", 0);
	xmltree_t *sent = XML_ChildOfTree(tree, "sent", 0);
	const char *showicon = NULL;	//their icon

	if (received && sent)
		return;	//no, just no.
	if (received)
	{
		xmltree_t *forwarded = XML_ChildOfTree(received, "forwarded", 0);
		if (!forwarded || strcmp(f, jcl->barejid))
			return;	//no bugs, no spoofing
		tree = XML_ChildOfTree(forwarded, "message", 0);
		f = XML_GetParameter(tree, "from", jcl->domain);
	}
	if (sent)
	{
		xmltree_t *forwarded = XML_ChildOfTree(sent, "forwarded", 0);
		if (!forwarded || strcmp(f, jcl->barejid))
			return;	//no bugs, no spoofing
		tree = XML_ChildOfTree(forwarded, "message", 0);
		f = XML_GetParameter(tree, "to", jcl->domain);
	}

	if (!strcmp(f, jcl->fulljid))
		unparsable = false;
	else
	{
		if (f)
		{
			buddy_t *b = NULL;
			bresource_t *br = NULL;
			Q_strlcpy(jcl->defaultdest, f, sizeof(jcl->defaultdest));
			JCL_FindBuddy(jcl, f, &b, &br, true);
			if (b && b->btype == BT_CHATROOM && strchr(jcl->defaultdest, '/'))
				*strchr(jcl->defaultdest, '/') = 0;
			if (b)
			{
				ctx = b->accountdomain;
				if (!strcmp(type, "groupchat"))
				{	//clients can act as a MUC too, which is awkward.
					if (br)
						f = br->resource;
				}
				else if (b->btype == BT_CHATROOM)
				{
					ctx = f;	//one server with multiple rooms requires that we retain resource info.
					if (br)
						f = br->resource;
				}
				else
				{
					if (confuncs)
						showicon = b->accountdomain;
					f = b->name;
					if (br)
						b->defaultresource = br;
				}
			}
		}

		if (!strcmp(type, "error"))
		{
			const char *reason = NULL;
			unparsable = false;
			if (sent)
				return;
			ot = XML_ChildOfTree(tree, "error", 0);
			if (ot->child)
				reason = ot->child->name;
			if (XML_ChildOfTree(ot, "remote-server-not-found", 0))		reason = "Remote Server Not Found";
			if (XML_ChildOfTree(ot, "bad-request", 0))					reason = "Bad Request";
			if (XML_ChildOfTree(ot, "conflict", 0))						reason = "Conflict Error";
			if (XML_ChildOfTree(ot, "feature-not-implemented", 0))		reason = "feature-not-implemented";
			if (XML_ChildOfTree(ot, "forbidden", 0))					reason = "forbidden";
			if (XML_ChildOfTree(ot, "gone", 0))							reason = "'gone' Error";
			if (XML_ChildOfTree(ot, "internal-server-error", 0))		reason = "internal-server-error";
			if (XML_ChildOfTree(ot, "item-not-found", 0))				reason = "item-not-found";
			if (XML_ChildOfTree(ot, "jid-malformed", 0))				reason = "jid-malformed";
			if (XML_ChildOfTree(ot, "not-acceptable", 0))				reason = "not-acceptable";
			if (XML_ChildOfTree(ot, "not-allowed", 0))					reason = "not-allowed";
			if (XML_ChildOfTree(ot, "not-authorized", 0))				reason = "not-authorized";
			if (XML_ChildOfTree(ot, "policy-violation", 0))				reason = "policy-violation";
			if (XML_ChildOfTree(ot, "recipient-unavailable", 0))		reason = "recipient-unavailable";
			if (XML_ChildOfTree(ot, "redirect", 0))						reason = "'redirect' Error";
			if (XML_ChildOfTree(ot, "registration-required", 0))		reason = "registration-required";
			if (XML_ChildOfTree(ot, "remote-server-not-found", 0))		reason = "remote-server-not-found";
			if (XML_ChildOfTree(ot, "remote-server-timeout", 0))		reason = "remote-server-timeout";
			if (XML_ChildOfTree(ot, "resource-constraint", 0))			reason = "resource-constraint";
			if (XML_ChildOfTree(ot, "service-unavailable", 0))			reason = "service-unavailable";
			if (XML_ChildOfTree(ot, "subscription-required", 0))		reason = "subscription-required";
			if (XML_ChildOfTree(ot, "undefined-condition", 0))			reason = "undefined-condition";
			if (XML_ChildOfTree(ot, "unexpected-request", 0))			reason = "unexpected-request";
			reason = XML_GetChildBody(ot, "text", reason);

			ot = XML_ChildOfTree(tree, "body", 0);
			if (ot)
			{
				if (reason)
					XMPP_ConversationPrintf(ctx, f, false, "^1Error: %s (%s): ", reason, f);
				else
					XMPP_ConversationPrintf(ctx, f, false, "^1error sending message to %s: ", f);
				if (f)
				{
					if (!strncmp(ot->body, "/me ", 4))
						XMPP_ConversationPrintf(ctx, f, false, "* ^2%s^7%s\r", ((!strcmp(jcl->localalias, ">>"))?"me":jcl->localalias), ot->body+3);
					else
						XMPP_ConversationPrintf(ctx, f, false, "%s\r", ot->body);
				}
			}
			else
			{
				if (reason)
					XMPP_ConversationPrintf(ctx, f, false, "^1Error: %s (%s)\r", reason, f);
				else
					XMPP_ConversationPrintf(ctx, f, false, "error sending message to %s\r", f);
			}
			return;
		}

		if (f && !sent)
		{
			ot = XML_ChildOfTree(tree, "composing", 0);
			if (ot && !strcmp(ot->xmlns, "http://jabber.org/protocol/chatstates"))
			{
				unparsable = false;
				XMPP_ConversationPrintf(ctx, f, false, "%s is typing\r", f);
			}
			ot = XML_ChildOfTree(tree, "paused", 0);
			if (ot && !strcmp(ot->xmlns, "http://jabber.org/protocol/chatstates"))
			{
				unparsable = false;
				XMPP_ConversationPrintf(ctx, f, false, "%s has stopped typing\r", f);
			}
			ot = XML_ChildOfTree(tree, "inactive", 0);
			if (ot && !strcmp(ot->xmlns, "http://jabber.org/protocol/chatstates"))
			{
				unparsable = false;
				XMPP_ConversationPrintf(ctx, f, false, "\r", f);
			}
			ot = XML_ChildOfTree(tree, "active", 0);
			if (ot && !strcmp(ot->xmlns, "http://jabber.org/protocol/chatstates"))
			{
				unparsable = false;
				XMPP_ConversationPrintf(ctx, f, false, "\r", f);
			}
			ot = XML_ChildOfTree(tree, "gone", 0);
			if (ot && !strcmp(ot->xmlns, "http://jabber.org/protocol/chatstates"))
			{
				unparsable = false;
				XMPP_ConversationPrintf(ctx, f, false, "%s has gone away\r", f);
			}
		}

		ot = XML_ChildOfTreeNS(tree, "urn:xmpp:attention:0", "attention", 0);
		if (ot && !sent)
		{
			unparsable = false;
			if (jclient_poketime < jclient_curtime)	//throttle these.
			{
				jclient_poketime = jclient_curtime + 10*1000;
				XMPP_ConversationPrintf(ctx, f, true, "%s is an attention whore.\n", f);
				if (drawfuncs)
					drawfuncs->LocalSound("misc/talk.wav", 256, 1);
			}
		}
		
		ot = XML_ChildOfTree(tree, "subject", 0);
		if (ot && !strcmp(type, "groupchat"))
		{
			buddy_t *b;
			JCL_FindBuddy(jcl, ctx, &b, NULL, true);
			if (b && b->btype == BT_CHATROOM)
			{
				free(b->room_topic);
				b->room_topic = strdup(ot->body);
				jclient_updatebuddylist = true;
			}

			unparsable = false;
			if (sent)
				XMPP_ConversationPrintf(ctx, f, false, COL_NAME_US"You"COL_TEXT_US" have set the topic to: %s\n", ot->body);
			else
				XMPP_ConversationPrintf(ctx, f, false, COL_NAME_THEM"%s"COL_TEXT_THEM" has set the topic to: %s\n", f, ot->body);
		}

		ot = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/pubsub#event", "event", 0);
		if (ot)
		{
			buddy_t *b;
			unparsable = false;
			JCL_FindBuddy(jcl, f, &b, NULL, false);
			if (!sent)
			{
				xmltree_t *items = XML_ChildOfTree(ot, "items", 0);
				const char *node = XML_GetParameter(items, "node", "");
				if (!strcmp(node, "http://jabber.org/protocol/nick"))
				{
					xmltree_t *item = XML_ChildOfTree(items, "item", 0);
					const char *nick = XML_GetChildBody(item, "nick", NULL);
					if (nick && b)
						Q_strlcpy(b->name, nick, sizeof(b->name));
				}
				else if (!strcmp(node, "http://jabber.org/protocol/tune"))
				{
//					xmltree_t *item = XML_ChildOfTree(items, "item", 0);
//					xmltree_t *tune = XML_ChildOfTreeNS(item, "http://jabber.org/protocol/tune", "tune", 0);
				}
				else if (!strcmp(node, "http://jabber.org/protocol/mood"))
				{
//					xmltree_t *item = XML_ChildOfTree(items, "item", 0);
//					xmltree_t *mood = XML_ChildOfTreeNS(item, "http://jabber.org/protocol/mood", "mood", 0);
//					const char *text = XML_GetChildBody(mood, "text", NULL);
					//there's a whole army of options here. crazy stuff that makes it unusable.
				}
				else if (!strcmp(node, "urn:xmpp:avatar:metadata"))
				{
//					xmltree_t *item = XML_ChildOfTree(items, "item", 0);
//					xmltree_t *tune = XML_ChildOfTreeNS(item, "urn:xmpp:avatar:metadata", "metadata", 0);
					//this can get messy.
				}
				else
				{
					Con_DPrintf("Unknown pubsub/pep node \"%s\"\n", node);
				}
			}
		}

		ot = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/muc#user", "x", 0);
		if (ot && f && !strchr(f, '/'))
		{
			//this is an appaling extension protocol. we really have no way to know if someone's just making this shit up just to see our presence.
			//this message came from the groupchat server.
			xmltree_t *inv = XML_ChildOfTree(ot, "invite", 0);
			if (inv)
			{
				const char *who = XML_GetParameter(inv, "from", jcl->domain);
				const char *reason = XML_GetChildBody(inv, "reason", NULL);
				const char *password = XML_GetChildBody(ot, "password", 0);
				char link[512];
				buddy_t *b;
				if (!sent && JCL_FindBuddy(jcl, f, &b, NULL, true))
				{
					if (b->btype == BT_CHATROOM)
						return;	//we already know about it. don't spam.
					JCL_GenLink(jcl, link, sizeof(link), "mucjoin", f, NULL, password, "%s", f);
//					ctx = who;
					if (reason)
						XMPP_ConversationPrintf(ctx, f, true, "* ^2%s^7 has invited you to join %s: %s.\n", who, link, reason);
					else
						XMPP_ConversationPrintf(ctx, f, true, "* ^2%s^7 has invited you to join %s.\n", who, link);
				}
				return; //ignore any body/jabber:x:conference
			}
		}

		ot = XML_ChildOfTreeNS(tree, "jabber:x:conference", "x", 0);
		if (ot)
		{
			char link[512];
			const char *chatjit = XML_GetParameter(ot, "jid", "");
			const char *reason = XML_GetParameter(ot, "reason", NULL);
			const char *password = XML_GetParameter(ot, "password", NULL);
			unparsable = false;

			JCL_GenLink(jcl, link, sizeof(link), "mucjoin", chatjit, NULL, password, "%s", chatjit);
			if (sent)
			{
				if (reason)
					XMPP_ConversationPrintf(ctx, f, true, "* You have invited ^2%s^7 to join %s: %s.\n", f, link, reason);
				else
					XMPP_ConversationPrintf(ctx, f, true, "* You have invited ^2%s^7 to join %s.\n", f, link);
			}
			else
			{
				if (reason)
					XMPP_ConversationPrintf(ctx, f, true, "* ^2%s^7 has invited you to join %s: %s.\n", f, link, reason);
				else
					XMPP_ConversationPrintf(ctx, f, true, "* ^2%s^7 has invited you to join %s.\n", f, link);
			}
			return;	//ignore any body
		}

		ot = XML_ChildOfTree(tree, "body", 0);
		if (ot)
		{
			unparsable = false;
			if (f)
			{
				if (sent)
				{
					if (!strncmp(ot->body, "/me ", 4))
						XMPP_ConversationPrintf(ctx, f, false, "* "COL_NAME_US"%s"COL_TEXT_US"%s\n", jcl->localalias, ot->body+3);
					else if (showicon)
						XMPP_ConversationPrintf(ctx, f, false, "^[\\img\\xmpp/%s.png\\fbimg\\"IMG_FB_US"\\w\\32\\h\\32^]"COL_NAME_US"%s"COL_TEXT_US":\v%s\n", jcl->barejid, jcl->localalias, ot->body);
					else
						XMPP_ConversationPrintf(ctx, f, false, COL_NAME_US"%s"COL_TEXT_US": %s\n", jcl->localalias, ot->body);
				}
				else
				{
					if (!strncmp(ot->body, "/me ", 4))
						XMPP_ConversationPrintf(ctx, f, false, "* "COL_NAME_THEM"%s"COL_TEXT_THEM"%s\n", f, ot->body+3);
					else if (showicon)
						XMPP_ConversationPrintf(ctx, f, false, "^[\\img\\xmpp/%s.png\\fbimg\\"IMG_FB_THEM"\\w\\32\\h\\32^]"COL_NAME_THEM"%s"COL_TEXT_THEM":\v%s\n", showicon, f, ot->body);
					else
						XMPP_ConversationPrintf(ctx, f, false, COL_NAME_THEM"%s"COL_TEXT_THEM": %s\n", f, ot->body);
				}
			}
			else
			{	
				if (!sent)
					XMPP_ConversationPrintf(ctx, f, false, "NOTICE: %s\n", ot->body);
			}

			if (drawfuncs)
				drawfuncs->LocalSound("misc/talk.wav", 256, 1);
		}

		if (unparsable)
		{
			unparsable = false;
			if (jcl->streamdebug)
			{
				XMPP_ConversationPrintf(ctx, f, false, "Received a message without a body\n");
				XML_ConPrintTree(tree, "", 0);
			}
		}
	}
}

#if 0
static qboolean JCL_ValidateCaps(xmltree_t *query, const char *node, const char *ver, const char *hash)
{
	const char *respnode = XML_GetParameter(query, "node", "");
	int l;
	char out[8192];
	int outlen = sizeof(out);
	unsigned char digest[64];
	const char *features[1024];
	size_t i, numfeatures = 0;
	size_t nlen = strlen(node);
	if (strcmp(hash, "sha-1"))
		return false;	//unable to validate.
	if (strncmp(respnode, node, nlen))
		return false; //the client's name changed...
	respnode+=nlen;
	if (*respnode++ != '#')
		return false; //o.O
	if (strcmp(respnode, ver))
		return false; //the client's name changed...
	for(i = 0; i < 64; i++)
	{
		xmltree_t *ident = XML_ChildOfTree(query, "identity", i);
		if (ident)
		{
			const char *category = XML_GetParameter(ident, "category", "");
			const char *name = XML_GetParameter(ident, "name", "");
			const char *type = XML_GetParameter(ident, "type", "");
			const char *lang = XML_GetParameter(ident, "xml:lang", "");
			Q_snprintf(out, outlen, "%s/%s/%s/%s<", category, type, lang, name);
		}
		else
			break;
	}
	for(i = 0; i < countof(features); i++)
	{
		xmltree_t *feature = XML_ChildOfTree(query, "feature", i);
		if (feature)
		{
			const char *var = XML_GetParameter(feature, "var", "");
			features[numfeatures++] = var;
		}
		else
			break;
	}
//	qsort(caps, sizeof(caps)/sizeof(caps[0]) - 1, sizeof(caps[0]), qsortcaps); 
	for (i = 0; i < numfeatures; i++)
	{
		Q_strlcat(out, features[i], outlen);
		Q_strlcat(out, "<", outlen);
	}
	//fixme: add any form crap
	l = SHA1(digest, sizeof(digest), out, strlen(out));
	for (i = 0; i < l; i++)
		Base64_Byte(digest[i]);
	return !strcmp(respnode, Base64_Finish());	//make sure its mostly valid.
}
#endif
unsigned int JCL_ParseCaps(jclient_t *jcl, char *account, char *resource, xmltree_t *query)
{
	xmltree_t *feature;
	unsigned int caps = 0;
	qboolean rtp = false;
	qboolean rtpaudio = false;
	qboolean rtpvideo = false;
	qboolean quake = false;
	qboolean ice = false;
	qboolean raw = false;
	qboolean jingle = false;
	qboolean si = false;
	qboolean sift = false;
	qboolean ibb = false;
	int i = 0;
	const char *var;
//	XML_ConPrintTree(query, 0);
	while((feature = XML_ChildOfTree(query, "feature", i++)))
	{
		var = XML_GetParameter(feature, "var", "");
		//check ones we recognise.
//		Con_Printf("%s/%s: %s\n", account, resource, var);
		if (!strcmp(var, QUAKEMEDIAXMLNS))
			quake = true;
#ifndef VOIP_LEGACY_ONLY
		if (!strcmp(var, "urn:xmpp:jingle:apps:rtp:audio"))
			rtpaudio = true;
		if (!strcmp(var, "urn:xmpp:jingle:apps:rtp:video"))
			rtpvideo = true;
#endif
		if (!strcmp(var, "urn:xmpp:jingle:apps:rtp:1"))
			rtp = true;		//kinda implied, but ensures version is okay
		if (!strcmp(var, "urn:xmpp:jingle:transports:ice-udp:1"))
			ice = true;
		if (!strcmp(var, "urn:xmpp:jingle:transports:raw-udp:1"))
			raw = true;
		if (!strcmp(var, "urn:xmpp:jingle:1"))
			jingle = true;	//kinda implied, but ensures version is okay

#ifdef VOIP_LEGACY
		if (!strcmp(var, "http://www.google.com/xmpp/protocol/voice/v1"))
			caps |= CAP_GOOGLE_VOICE;	//legacy crap, so we can call google's official clients, which do not follow current xmpp standards.
#endif

		if (!strcmp(var, "http://jabber.org/protocol/si"))
			si = true;
		if (!strcmp(var, "http://jabber.org/protocol/si/profile/file-transfer"))
			sift = true;
		if (!strcmp(var, "http://jabber.org/protocol/ibb"))
			ibb = true;
	}

	if ((ice||raw) && jingle)
	{
		if (rtpaudio && rtp)
			caps |= CAP_VOICE;
		if (rtpvideo && rtp)
			caps |= CAP_VIDEO;
		if (quake)
			caps |= CAP_GAMEINVITE;
	}
	if (si && sift && ibb)
		caps |= CAP_SIFT;

	caps &= jcl->enabledcapabilities;

	return caps;
}

void JCL_CheckClientCaps(jclient_t *jcl, buddy_t *buddy, bresource_t *bres);
qboolean JCL_ClientDiscoInfo(jclient_t *jcl, xmltree_t *tree, struct iq_s *iq)
{
	xmltree_t *query = XML_ChildOfTree(tree, "query", 0);
	unsigned int caps = 0;
	buddy_t *b, *ob;
	bresource_t *r, *or;
	if (!JCL_FindBuddy(jcl, iq->to, &b, &r, true))
		return false;

	if (!query)
	{
		caps &= ~(CAP_QUERYING|CAP_QUERIED);
		caps |= CAP_QUERYFAILED;
	}
	else
	{
//		XML_ConPrintTree(tree, 0);
//		if (!JCL_ValidateCaps(query, r->client_node, r->client_ver, r->client_hash))
//			caps = 0;
//		else
			caps = JCL_ParseCaps(jcl, b->accountdomain, r->resource, query);
	}

	if (b && r)
	{
		if (!(caps & CAP_QUERYFAILED))
		{
//			Con_Printf("%s/%s caps = %x\n", b->accountdomain, r->resource, caps);
			if (!(r->caps & CAP_QUERIED))
				r->caps = CAP_QUERIED;	//reset it
			r->caps |= caps;

			//as we got a valid response, make sure other resources running the same software get the same caps
			for (ob = jcl->buddies; ob; ob = ob->next)
			{
				for (or = ob->resources; or; or = or->next)
				{
					//ignore evil clients.
					if (r == or || (or->caps & (CAP_QUERYFAILED|CAP_QUERIED|CAP_QUERYING)) || !or->client_node)
						continue;

					//all resources with the same details then get the same caps flags.
					if (!strcmp(r->client_node, or->client_node) && !strcmp(r->client_ver, or->client_ver) && !strcmp(r->client_hash, or->client_hash) && !strcmp(r->client_ext, or->client_ext))
					{
//						Con_Printf("%s/%s matches (updated) %s/%s (%s#%s)\n", ob->accountdomain, or->resource, b->accountdomain, r->resource, r->client_node, r->client_ver);
						or->caps = r->caps;
					}
				}
			}
		}
		else
		{
			if (!(r->caps & (CAP_QUERIED|CAP_QUERYFAILED)))
			{
				r->caps = CAP_QUERYFAILED;
				//as the response is invalid, we need to ensure that other resources that claim to use the same software are still queried anyway.
				//(this is needed in case the one that we asked was spoofing)
				for (ob = jcl->buddies; ob; ob = ob->next)
				{
					for (or = ob->resources; or; or = or->next)
					{
						//ignore evil clients.
						if (r == or || (or->caps & (CAP_QUERYFAILED|CAP_QUERIED|CAP_QUERYING)) || !or->client_node)
							continue;

						//all resources with the same details then get the same caps flags.
						if (r->client_node && !strcmp(r->client_node, or->client_node) && !strcmp(r->client_ver, or->client_ver) && !strcmp(r->client_hash, or->client_hash) && !strcmp(r->client_ext, or->client_ext))
						{
							JCL_CheckClientCaps(jcl, ob, or);
							return true;
						}
					}
				}
			}
		}
	}
	return true;
}
void JCL_CheckClientCaps(jclient_t *jcl, buddy_t *buddy, bresource_t *bres)
{
	buddy_t *b;
	bresource_t *r;
	char extname[64], *ext;

	//ignore it if we're already asking them...
	if (bres->caps & (CAP_QUERYING|CAP_QUERIED|CAP_QUERYFAILED))
		return;

	bres->buggycaps = 0;

#ifdef VOIP_LEGACY
	if (!bres->client_node || !bres->client_hash || !*bres->client_hash)
	{
		//one of google's nodes. ONLY google get this fucked up evil hack because they're the only ones that are arrogant enough to not bother to query what that 'ext' actually means - and then to not even bother to tell other clients.
		//every other client is expected to have its act together and not fuck up like this.
		if (bres->client_node && (!!strstr(bres->client_node, "google.com") || !!strstr(bres->client_node, "android.com")))
		{
			for (ext = bres->client_ext; ext; )
			{
				ext = JCL_ParseOut(ext, extname, sizeof(extname));
				if (googlefuckedup)
				{
					//work around repeated bugs in google's various clients.
					if (!strcmp(extname, "voice-v1"))
						bres->buggycaps |= CAP_GOOGLE_VOICE;
				}
			}
		}
	}
#endif

	//look for another resource that we might already know about that is the same client+ver and thus has the same caps.
	if (bres->client_node)
		for (b = jcl->buddies; b; b = b->next)
		{
			for (r = b->resources; r; r = r->next)
			{
				if (r == bres)
					continue;
				//ignore evil clients.
				if ((r->caps & CAP_QUERYFAILED) || !r->client_node)
					continue;

				if (!strcmp(r->client_node, bres->client_node) && !strcmp(r->client_ver, bres->client_ver) && !strcmp(r->client_hash, bres->client_hash) && !strcmp(r->client_ext, bres->client_ext))
				{
					if (r->caps & CAP_QUERIED)
					{
						bres->caps = r->caps;
//						Con_Printf("%s/%s matches (known) %s/%s (%s#%s)\n", buddy->accountdomain, bres->resource, b->accountdomain, r->resource, r->client_node, r->client_ver);
						return;
					}
					if (r->caps & CAP_QUERYING)
					{
						//we're already asking one client with the same software. don't ask for dupes.
						bres->caps = 0;
//						Con_Printf("%s/%s matches (pending) %s/%s (%s#%s)\n", buddy->accountdomain, bres->resource, b->accountdomain, r->resource, r->client_node, r->client_ver);
						return;
					}
				}
			}
		}

//	Con_Printf("%s/%s querying (%s#%s)\n", buddy->accountdomain, bres->resource, bres->client_node, bres->client_ver);
	//okay, this is the first time we've seen that software version. ask it what it supports, and hope we get a valid response...
	if (!bres->client_node || !bres->client_hash || !*bres->client_hash)
	{
		//if we cannot cache the result, don't bother asking.
		//this saves googletalk bugging out on us.
		bres->caps = CAP_QUERIED;
	}
	else
	{
		bres->caps = CAP_QUERYING;

		//ask for info about each extension too. which should only be used if the specified version isn't a hash.
		if (bres->client_hash && !*bres->client_hash)
		{
			for (ext = bres->client_ext; ext; )
			{
				ext = JCL_ParseOut(ext, extname, sizeof(extname));
				if (*extname)
					JCL_SendIQf(jcl, JCL_ClientDiscoInfo, "get", va("%s/%s", buddy->accountdomain, bres->resource), "<query xmlns='http://jabber.org/protocol/disco#info' node='%s#%s'/>", bres->client_node, extname);
			}
		}
		JCL_SendIQf(jcl, JCL_ClientDiscoInfo, "get", va("%s/%s", buddy->accountdomain, bres->resource), "<query xmlns='http://jabber.org/protocol/disco#info' node='%s#%s'/>", bres->client_node, bres->client_ver);
	}
}
void JCL_ParsePresence(jclient_t *jcl, xmltree_t *tree)
{
	buddy_t *buddy;
	bresource_t *bres;

	const char *from = XML_GetParameter(tree, "from", jcl->domain);
	xmltree_t *show = XML_ChildOfTree(tree, "show", 0);
	xmltree_t *status = XML_ChildOfTree(tree, "status", 0);
	xmltree_t *quake = XML_ChildOfTree(tree, "quake", 0);
	//xmltree_t *mucmain = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/muc", "x", 0);
	xmltree_t *mucuser = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/muc#user", "x", 0);
	xmltree_t *caps = XML_ChildOfTreeNS(tree, "http://jabber.org/protocol/caps", "c", 0);
	const char *type = XML_GetParameter(tree, "type", "");
	const char *serverip = NULL;
	const char *servermap = NULL;
	char startconvo[512];
	char oldbstatus[128];
	char oldfstatus[128];

	//should really be using pep for this.
	if (quake && !strcmp(quake->xmlns, "fteqw.com:game") && (jcl->enabledcapabilities & CAP_GAMEINVITE))
	{
		serverip = XML_GetParameter(quake, "serverip", NULL);
		servermap = XML_GetParameter(quake, "servermap", NULL);
	}

	jclient_updatebuddylist = true;

	if (type && !strcmp(type, "error"))
	{
		JCL_FindBuddy(jcl, from, &buddy, &bres, false);
		if (buddy && bres)
		{
			xmltree_t *error = XML_ChildOfTree(tree, "error", 0);
			if (error->child)
			{
				Q_strlcpy(bres->fstatus, error->child->name, sizeof(bres->fstatus));
			}
		}
	}
	else if (type && !strcmp(type, "subscribe"))
	{
		//they want us to let them see us.
		char pauth[512], pdeny[512];
		JCL_GenLink(jcl, startconvo, sizeof(startconvo), NULL, from, NULL, NULL, "%s", from);
		JCL_GenLink(jcl, pauth, sizeof(pauth), "pauth", from, NULL, NULL, "%s", "Authorize");
		JCL_GenLink(jcl, pdeny, sizeof(pdeny), "pdeny", from, NULL, NULL, "%s", "Deny");
		Con_Printf("%s wants to be your friend! %s %s\n", startconvo, pauth, pdeny);
	}
	else if (type && !strcmp(type, "subscribed"))
	{
		//they allowed us to add them.
		JCL_GenLink(jcl, startconvo, sizeof(startconvo), NULL, from, NULL, NULL, "%s", from);
		Con_Printf("%s is now your friend!\n", startconvo);
	}
	else if (type && !strcmp(type, "unsubscribe"))
	{
		//they removed us.
		JCL_GenLink(jcl, startconvo, sizeof(startconvo), NULL, from, NULL, NULL, "%s", from);
		Con_Printf("%s has unfriended you\n", startconvo);
	}
	else if (type && !strcmp(type, "unsubscribed"))
	{
		//we removed them.
		JCL_GenLink(jcl, startconvo, sizeof(startconvo), NULL, from, NULL, NULL, "%s", from);
		Con_Printf("%s is no longer your friend\n", startconvo);
	}
	else
	{
		JCL_FindBuddy(jcl, from, &buddy, &bres, true);
		if (!bres)
		{
			JCL_FindBuddy(jcl, va("%s/", from), &buddy, &bres, true);
		}
		JCL_GenLink(jcl, startconvo, sizeof(startconvo), NULL, from, NULL, NULL, "%s", buddy->name);

		if (bres)
		{
			if (servermap)
			{
				bres->servertype = 2;
				Q_strlcpy(bres->server, servermap, sizeof(bres->server));
			}
			else if (serverip)
			{
				bres->servertype = 1;
				Q_strlcpy(bres->server, serverip, sizeof(bres->server));
			}
			else
			{
				bres->servertype = 0;
				Q_strlcpy(bres->server, "", sizeof(bres->server));
			}

			Q_strlcpy(oldbstatus, bres->bstatus, sizeof(oldbstatus));
			Q_strlcpy(oldfstatus, bres->fstatus, sizeof(oldfstatus));

			Q_strlcpy(bres->fstatus, (status && *status->body)?status->body:"", sizeof(bres->fstatus));
			if (!tree->child)
			{
				Q_strlcpy(bres->bstatus, "offline", sizeof(bres->bstatus));
				bres->caps = 0;
			}
			else
			{
				xmltree_t *vcu;
				const char *photohash;
				buddy_t *me;
				vcu = XML_ChildOfTreeNS(tree, "vcard-temp:x:update", "x", 0);
				photohash = XML_GetChildBody(vcu, "photo", buddy->vcardphotohash);
				if (strcmp(buddy->vcardphotohash, photohash))
				{
					Con_DPrintf("%s changed their photo from \"%s\" to \"%s\"\n", from, buddy->vcardphotohash, photohash);

					Q_strlcpy(buddy->vcardphotohash, photohash, sizeof(buddy->vcardphotohash));
					buddy->vcardphotochanged = true;
				}

				JCL_FindBuddy(jcl, jcl->fulljid, &me, NULL, true);
				if (buddy == me)
				{
					if (strcmp(buddy->vcardphotohash, jcl->vcardphotohash))
					{	//if one of your other resources changed its image, we need to retrieve it so we can tell other clients about it if the other resource goes away
						jcl->vcardphotohashstatus = VCP_UNKNOWN;
						JCL_SendIQf(jcl, JCL_MyVCardReply, "get", NULL, "<vCard xmlns='vcard-temp'/>");
					}
				}

				Q_strlcpy(bres->bstatus, (show && *show->body)?show->body:"present", sizeof(bres->bstatus));

				if (caps)
				{
					const char *ext = XML_GetParameter(caps, "ext", "");	//deprecated
					const char *ver = XML_GetParameter(caps, "ver", "");
					const char *node = XML_GetParameter(caps, "node", "");
					const char *hash = XML_GetParameter(caps, "hash", "");

					if (!bres->client_hash || strcmp(ext, bres->client_ext) || strcmp(hash, bres->client_hash) || strcmp(node, bres->client_node) || strcmp(ver, bres->client_ver))
					{
						bres->caps &= ~(CAP_QUERIED|CAP_QUERYING|CAP_QUERYFAILED);	//no idea what the new caps are. 
						free(bres->client_ext);
						free(bres->client_hash);
						free(bres->client_node);
						free(bres->client_ver);
						bres->client_ext = strdup(ext);
						bres->client_hash = strdup(hash);
						bres->client_node = strdup(node);
						bres->client_ver = strdup(ver);
					}
				}
				JCL_CheckClientCaps(jcl, buddy, bres);
			}

			if (mucuser)
			{
				//<item affiliation='' jid='' role=''/>
				//<status code='xxx'/>
				//100=public jids
				//101=affiliation changed
				//102=shows unavailable members
				//103=no longer shows unavailable
				//104=room config changed
				//110=this is you
				//170=room logging enabled
				//171=room logging disabled
				//172=now annon
				//173=now semi-anon
				//201=new room created
				//210=roomnick changed
				//301=you're banned
				//303=
				//307=you're kicked
				//321=removed by affiliation change
				//322=not a member
				//332=shutdown
				JCL_GenLink(jcl, startconvo, sizeof(startconvo), NULL, from, NULL, NULL, "%s", bres->resource);
				if (type && !strcmp(type, "unavailable"))
					XMPP_ConversationPrintf(buddy->name, buddy->name, false, "%s has left the conversation\n", bres->resource);
				else if (strcmp(oldbstatus, bres->bstatus))
					XMPP_ConversationPrintf(buddy->name, buddy->name, false, "%s is now %s\n", startconvo, bres->bstatus);
			}
			else
			{
				char *conv = buddy->accountdomain;
				char *title = buddy->name;

				//if we're not currently talking with them, put the status update into the main console instead (which will probably then get dropped).
				if (!confuncs || confuncs->GetConsoleFloat(conv, "iswindow") != true)
					conv = "";

				if (bres->servertype == 2)
				{
					char joinlink[512];
					JCL_GenLink(jcl, joinlink, sizeof(joinlink), "join", from, NULL, NULL, "Playing Quake - %s", bres->server);
					XMPP_ConversationPrintf(conv, title, false, "%s is now %s\n", startconvo, joinlink);
				}
				else if (bres->servertype == 1)
					XMPP_ConversationPrintf(conv, title, false, "%s is now ^[[Playing Quake - %s]\\observe\\%s^]\n", startconvo, bres->server, bres->server);
				else if ((cvarfuncs->GetFloat("xmpp_showstatusupdates")||*conv) && (strcmp(oldbstatus, bres->bstatus) || strcmp(oldfstatus, bres->fstatus)))
				{
					if (*bres->fstatus)
						XMPP_ConversationPrintf(conv, title, false, "%s is now %s: %s\n", startconvo, bres->bstatus, bres->fstatus);
					else
						XMPP_ConversationPrintf(conv, title, false, "%s is now %s\n", startconvo, bres->bstatus);
				}
			}

			if (type && !strcmp(type, "unavailable"))
			{
				//remove this buddy resource
			}
		}
		else
		{
			Con_Printf("Weird presence:\n");
			XML_ConPrintTree(tree, "", 0);
		}
	}
}

#define JCL_DONE 0			//no more data available for now.
#define JCL_CONTINUE 1		//more data needs parsing.
#define JCL_KILL 2			//some error, needs reconnecting.
#define JCL_NUKEFROMORBIT 3	//permanent error (or logged on from elsewhere)
int JCL_ClientFrame(jclient_t *jcl, char **error)
{
	int pos;
	xmltree_t *tree, *ot;
	int ret;
	qboolean unparsable;

	ret = netfuncs->Recv(jcl->socket, jcl->bufferedinmessage+jcl->bufferedinammount, sizeof(jcl->bufferedinmessage)-1 - jcl->bufferedinammount);
	if (ret == 0)
	{
		if (!jcl->bufferedinammount)	//if we are half way through a message, read any possible conjunctions.
			return JCL_DONE;	//nothing more this frame
	}
	if (ret < 0)
	{
		*error = "Socket Error";
		if (jcl->socket != -1)
			netfuncs->Close(jcl->socket);
		jcl->socket = -1;
		return JCL_KILL;
	}

	if (ret>0)
	{
		jcl->bufferedinammount+=ret;
		jcl->bufferedinmessage[jcl->bufferedinammount] = 0;
	}

	//we never end parsing in the middle of a < >
	//this means we can filter out the <? ?>, <!-- --> and < /> stuff properly
	for (pos = jcl->instreampos; pos < jcl->bufferedinammount; pos++)
	{
		if (jcl->bufferedinmessage[pos] == '<')
		{
			jcl->instreampos = pos;
		}
		else if (jcl->bufferedinmessage[pos] == '>')
		{
			if (pos < 1)
				break;	//erm...

/*			if (jcl->bufferedinmessage[pos-1] != '/')	//<blah/> is a tag without a body
			{
				if (jcl->bufferedinmessage[jcl->instreampos+1] != '?')	//<? blah ?> is a tag without a body
				{
					if (jcl->bufferedinmessage[pos-1] != '?')
					{
						if (jcl->bufferedinmessage[jcl->instreampos+1] == '/')	//</blah> is the end of a tag with a body
							jcl->tagdepth--;
						else
							jcl->tagdepth++;			//<blah> is the start of a tag with a body
					}
				}
			}
*/
			jcl->instreampos=pos+1;
		}
	}

	pos = 0;
	while (jcl->connecting)
	{	//first bit of info
		tree = XML_Parse(jcl->bufferedinmessage, &pos, jcl->instreampos, true, "");
		if (tree && !strcmp(tree->name, "?xml"))
		{
			XML_Destroy(tree);
			continue;
		}

		if (jcl->streamdebug == 2)
		{
			char t = jcl->bufferedinmessage[pos];
			jcl->bufferedinmessage[pos] = 0;
			XMPP_ConversationPrintf("xmppin", "xmppin", false, "%s", jcl->bufferedinmessage);
			if (tree)
				XMPP_ConversationPrintf("xmppin", "xmppin", false, "\n");
			jcl->bufferedinmessage[pos] = t;
		}

		if (!tree)
		{
			*error = "Not an xml stream";
			return JCL_KILL;
		}
		if (strcmp(tree->name, "stream") || strcmp(tree->xmlns, "http://etherx.jabber.org/streams"))
		{
			*error = "Not an xmpp stream";
			return JCL_KILL;
		}
		Q_strlcpy(jcl->defaultnamespace, tree->xmlns_dflt, sizeof(jcl->defaultnamespace));

		ot = tree;
		tree = tree->child;
		ot->child = NULL;

//		Con_Printf("Discard\n");
//		XML_ConPrintTree(ot, 0);
		XML_Destroy(ot);

		jcl->connecting = false;
	}

/*	if (jcl->tagdepth != 1)
	{
		if (jcl->tagdepth < 1 && jcl->bufferedinammount==jcl->instreampos)
		{
			*error = "End of XML stream";
			return JCL_KILL;
		}
		return JCL_DONE;
	}
*/
	tree = XML_Parse(jcl->bufferedinmessage, &pos, jcl->instreampos, false, jcl->defaultnamespace);

	if (jcl->streamdebug == 2 && tree)
	{
		char t = jcl->bufferedinmessage[pos];
		jcl->bufferedinmessage[pos] = 0;
		XMPP_ConversationPrintf("xmppin", "xmppin", false, "%s", jcl->bufferedinmessage);
		XMPP_ConversationPrintf("xmppin", "xmppin", false, "\n");
		jcl->bufferedinmessage[pos] = t;
	}

	if (!tree)
	{
		//make sure any prior crap is flushed.
		memmove(jcl->bufferedinmessage, jcl->bufferedinmessage+pos, jcl->bufferedinammount-pos);
		jcl->bufferedinammount -= pos;
		jcl->instreampos -= pos;
		pos = 0;

//			Con_Printf("No input tree: %s", jcl->bufferedinmessage);
		return JCL_DONE;
	}

//	Con_Printf("read\n");
//	XML_ConPrintTree(tree, 0);

	if (jcl->streamdebug == 1)
	{
		XMPP_ConversationPrintf("xmppin", "xmppin", false, "");
		XML_ConPrintTree(tree, "xmppin", 0);
	}

	jcl->timeout = jclient_curtime + 60*1000;

	unparsable = true;
	if (!strcmp(tree->name, "features"))
	{
		if (Q_snprintfz(jcl->barejid, sizeof(jcl->barejid), "%s@%s", jcl->username, jcl->domain) ||
			Q_snprintfz(jcl->fulljid, sizeof(jcl->fulljid), "%s@%s/%s", jcl->username, jcl->domain, jcl->resource))
		{
			XML_Destroy(tree);
			return JCL_KILL;
		}
		if ((ot=XML_ChildOfTree(tree, "bind", 0)))
		{
			unparsable = false;
			JCL_SendIQf(jcl, JCL_BindReply, "set", NULL, "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'><resource>%s</resource></bind>", jcl->resource);
		}
		if ((ot=XML_ChildOfTree(tree, "session", 0)))
		{
			unparsable = false;
			JCL_SendIQf(jcl, JCL_SessionReply, "set", NULL, "<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>");
			jcl->connected = true;

			JCL_WriteConfig();
		}


		if (unparsable)
		{
			if ((!jcl->issecure) && XML_ChildOfTree(tree, "starttls", 0) != NULL && jcl->forcetls >= 0)
			{
				Con_DPrintf("XMPP: Attempting to switch to TLS\n");
				JCL_AddClientMessageString(jcl, "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls' />");
				unparsable = false;
			}
			else if ((ot=XML_ChildOfTree(tree, "mechanisms", 0)))
			{
				xmltree_t *m;
				int sm;
				char out[512];
				char *method = NULL;
				int outlen = -1;
				qboolean needpass = false;
				if (jcl->forcetls > 0 && !jcl->issecure)
				{
					*error = "Unable to switch to TLS. You are probably being man-in-the-middle attacked.";
					XML_ConPrintTree(tree, "", 0);
					XML_Destroy(tree);
					return JCL_KILL;
				}

				//init some sasl fields from the jcl state.
				jcl->sasl.username = jcl->username;	//auth user name
				jcl->sasl.domain = jcl->domain;		//auth domain
				jcl->sasl.issecure = jcl->issecure;	//says that its connected over tls, and that sock is the tls connection.
				jcl->sasl.socket = jcl->socket;			//just for channel bindings
				jcl->sasl.authmethod = NULL;		//unknown at this point

				//attempt to use a method based on the ones that we prefer.
				for (sm = 0; sm < sizeof(saslmethods)/sizeof(saslmethods[0]); sm++)
				{
					method = saslmethods[sm].method?saslmethods[sm].method:jcl->sasl.oauth2.saslmethod;
					if (!*method)
						continue;
					for (m = ot->child; m; m = m->sibling)
					{
						if (!strcmp(m->body, method))
						{
							outlen = saslmethods[sm].sasl_initial(&jcl->sasl, out, sizeof(out));
							if (outlen >= 0)
							{
								jcl->sasl.authmethod = &saslmethods[sm];
								break;
							}
							if (outlen == -2)
								needpass = true;
						}
					}
					if (outlen >= 0)
						break;
				}

				if (outlen < 0 || !jcl->sasl.authmethod)
				{
					XML_Destroy(tree);
					//can't authenticate for some reason
					if (needpass)
						XMPP_Menu_Password(jcl);
					*error = "Password needed";
					return JCL_NUKEFROMORBIT;
				}

				if (outlen >= 0)
				{
					Base64_Add(out, outlen);
					Base64_Finish();

					Con_DPrintf("XMPP: Authing with %s%s.\n", method, jcl->issecure?" over tls":" without encription");
					JCL_AddClientMessagef(jcl, "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='%s'"
							" auth:service='oauth2'"
							" xmlns:auth='http://www.google.com/talk/protocol/auth'"
							">%s</auth>", method, base64);
					unparsable = false;
				}
				else
				{
					*error = "No suitable auth methods";
					XML_ConPrintTree(tree, "", 0);
					XML_Destroy(tree);
					return JCL_KILL;
				}
			}
			else	//we cannot auth, no suitable method.
			{
				*error = "Neither SASL or TLS are usable";
				XML_Destroy(tree);
				return JCL_KILL;
			}
		}
	}
	else if (!strcmp(tree->name, "challenge") && !strcmp(tree->xmlns, "urn:ietf:params:xml:ns:xmpp-sasl") && jcl->sasl.authmethod)
	{
		char in[512];
		int inlen;
		char out[512];
		int outlen;
		inlen = Base64_Decode(in, sizeof(in), tree->body, strlen(tree->body));
		outlen = jcl->sasl.authmethod->sasl_challenge(&jcl->sasl, in, inlen, out, sizeof(out));
		if (outlen < 0)
		{
			*error = "Unable to auth with server";
			XML_Destroy(tree);
			return JCL_KILL;
		}
		Base64_Add(out, outlen);
		Base64_Finish();
		JCL_AddClientMessagef(jcl, "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>%s</response>", base64);
		unparsable = false;
	}
	else if (!strcmp(tree->name, "proceed"))
	{
		//switch to TLS, if we can

		//Restart everything, basically.
		jcl->bufferedinammount = 0;
		jcl->instreampos = 0;

		//when using srv records, the certificate must match the user's domain, rather than merely the hostname of the server.
		//if you want to match the hostname of the server, use (oldstyle) tlsconnect directly instead.
		if (netfuncs->SetTLSClient(jcl->socket, jcl->certificatedomain)<0)
		{
			*error = "failed to switch to TLS";
			XML_Destroy(tree);
			return JCL_KILL;
		}
		if (!*jcl->certificatedomain)
			Con_Printf("XMPP: WARNING: Connecting via TLS without validating certificate\n");
		jcl->issecure = true;

		jcl->connecting = true;
		JCL_AddClientMessageString(jcl,
			"<?xml version='1.0' ?>"
			"<stream:stream to='");
		JCL_AddClientMessageString(jcl, jcl->domain);
		JCL_AddClientMessageString(jcl, "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>");

		XML_Destroy(tree);
		return JCL_DONE;
	}
	else if (!strcmp(tree->name, "failure"))
	{
		if (tree->child)
			*error = va("Failure: %s\n", tree->child->name);
		else
			*error = "Unknown failure";
		XML_Destroy(tree);
		return JCL_KILL;
	}
	else if (!strcmp(tree->name, "error"))
	{
		xmltree_t *condition;
		condition = XML_ChildOfTree(tree, "success", 0);
		if (!condition)
			condition = XML_ChildOfTree(tree, "see-other-host", 0);
		if (!condition)
			condition = XML_ChildOfTree(tree, "invalid-xml", 0);

		if (condition && !strcmp(condition->name, "see-other-host"))
		{
			//msn needs this, apparently
			Q_strlcpy(jcl->redirserveraddr, condition->body, sizeof(jcl->redirserveraddr));
			JCL_CloseConnection(jcl, "Redirecting", true);
			if (!JCL_Reconnect(jcl))
			{
				*error = "Unable to redirect";
				return JCL_KILL;
			}
			return JCL_CONTINUE;
		}
		else if (condition && !strcmp(condition->name, "success"))
		{
			*error = "error: success";
			unparsable = false;
		}
		else
		{
			ot = XML_ChildOfTree(tree, "text", 0);
			if (ot)
				*error = va("error: %s", ot->body);
			else if (condition)
				*error = va("error: %s", condition->name);
			else
				*error = "Unknown error";

			ot = XML_ChildOfTree(tree, "conflict", 0);
			XML_Destroy(tree);

			if (ot)
				return JCL_NUKEFROMORBIT;
			else
				return JCL_KILL;
		}
	}
	else if (!strcmp(tree->name, "success") && !strcmp(tree->xmlns, "urn:ietf:params:xml:ns:xmpp-sasl"))
	{
		if (!jcl->sasl.authmethod || jcl->sasl.authmethod->sasl_success)
		{
			char in[512];
			int inlen;
			inlen = Base64_Decode(in, sizeof(in), tree->body, strlen(tree->body));
			if (!jcl->sasl.authmethod || jcl->sasl.authmethod->sasl_success(&jcl->sasl, in, inlen) < 0)
			{
				*error = "Server validation failed";
				XML_Destroy(tree);
				return JCL_KILL;
			}
		}

		//Restart everything, basically, AGAIN! (third time lucky?)
		jcl->bufferedinammount = 0;
		jcl->instreampos = 0;

		jcl->connecting = true;
		JCL_AddClientMessageString(jcl,
			"<?xml version='1.0' ?>"
			"<stream:stream to='");
		JCL_AddClientMessageString(jcl, jcl->domain);
		JCL_AddClientMessageString(jcl, "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>");

		return JCL_DONE;
	}
	else if (!strcmp(tree->name, "iq"))
	{
		JCL_ParseIQ(jcl, tree);
		unparsable = false;
	}
	else if (!strcmp(tree->name, "message"))
	{
		JCL_ParseMessage(jcl, tree);
		unparsable = false;
	}
	else if (!strcmp(tree->name, "presence"))
	{
		JCL_ParsePresence(jcl, tree);
		//we should keep a list of the people that we know of.
		unparsable = false;
	}
	else
	{
		Con_Printf("JCL unrecognised stanza: %s\n", tree->name);
		XML_ConPrintTree(tree, "", 0);
	}

	memmove(jcl->bufferedinmessage, jcl->bufferedinmessage+pos, jcl->bufferedinammount-pos);
	jcl->bufferedinammount -= pos;
	jcl->instreampos -= pos;

	if (unparsable)
	{
		*error = "Input corrupt, urecognised, or unusable.";
		XML_ConPrintTree(tree, "", 0);
		XML_Destroy(tree);
		return JCL_KILL;
	}
	XML_Destroy(tree);
	return JCL_CONTINUE;
}

void JCL_CloseConnection(jclient_t *jcl, const char *reason, qboolean reconnect)
{
	//send our signoff to the server, if we're still alive.
	if (jcl->status != JCL_DEAD && jcl->status != JCL_INACTIVE)
		Con_Printf("XMPP: Disconnected from %s@%s\n", jcl->username, jcl->domain);

	if (jcl->status == JCL_ACTIVE)
		JCL_AddClientMessageString(jcl, "<presence type='unavailable'/>");
	if (jcl->status != JCL_DEAD && jcl->status != JCL_INACTIVE)
		JCL_AddClientMessageString(jcl, "</stream:stream>");
	JCL_FlushOutgoing(jcl);

	//forget all our friends.
	JCL_ForgetBuddy(jcl, NULL, NULL);

	//destroy any data that never got sent
	free(jcl->outbuf);
	jcl->outbuf = NULL;
	jcl->outbuflen = 0;
	jcl->outbufpos = 0;
	jcl->outbufmax = 0;

	if (jcl->socket != -1)
		netfuncs->Close(jcl->socket);
	jcl->socket = -1;
	jcl->status = JCL_DEAD;
	Q_strncpyz(jcl->errormsg, reason, sizeof(jcl->errormsg));

	jcl->timeout = jclient_curtime + 30*1000;	//wait 30 secs before reconnecting, to avoid flood-prot-protection issues.

	if (!reconnect)
	{
		int i;
		free(jcl);
		for (i = 0; i < sizeof(jclients)/sizeof(jclients[0]); i++)
		{
			if (jclients[i] == jcl)
			{
				jclients[i] = NULL;
				jclient_configdirty = true;
			}
		}
	}

	jclient_updatebuddylist = true;
}

//can be polled for server address updates
void JCL_GeneratePresence(jclient_t *jcl, qboolean force)
{
	int dummystat;
	char serveraddr[1024*16];
	char servermap[1024*16];
	//get the last server address

	serveraddr[0] = 0;
	servermap[0] = 0;

	if (!cvarfuncs->GetFloat("xmpp_nostatus"))
	{
		if (cvarfuncs->GetFloat("sv.state"))
		{
			cvarfuncs->GetString("sv.mapname", servermap, sizeof(servermap));
		}
		else
		{
			if (!cvarfuncs->GetString("cl_serveraddress", serveraddr, sizeof(serveraddr)))
				serveraddr[0] = 0;
			if (clientfuncs)
			{
				//if we can't get any stats, its because we're not actually on the server.
				if (!clientfuncs->GetStats(0, &dummystat, 1))
					serveraddr[0] = 0;
			}
		}
	}

	if (force || jcl->vcardphotohashchanged || strcmp(jcl->curquakeserver, *servermap?servermap:serveraddr))
	{
		char *prior = "<priority>24</priority>";
		char caps[512];
		jcl->vcardphotohashchanged = false;
		Q_strlcpy(jcl->curquakeserver, *servermap?servermap:serveraddr, sizeof(jcl->curquakeserver));
		Con_DPrintf("Sending presence %s\n", jcl->curquakeserver);

		buildcapsvcardpresence(jcl, caps, sizeof(caps));

		if (!*jcl->curquakeserver)
			JCL_AddClientMessagef(jcl,
					"<presence from='%s'>"
						"%+s"
						"%+s"
					"</presence>", jcl->fulljid, prior, caps);
		else if (*servermap)	//if we're running a server, say so
			JCL_AddClientMessagef(jcl, 
						"<presence from='%s'>"
							"%+s"
							"<quake xmlns='fteqw.com:game' servermap='%s'/>"
							"%+s"
						"</presence>"
						, jcl->fulljid, prior, servermap, caps);
		else	//if we're connected to a server, say so
			JCL_AddClientMessagef(jcl, 
						"<presence from='%s'>"
							"%+s"
							"<quake xmlns='fteqw.com:game' serverip='%s' />"
							"%+s"
						"</presence>"
				, jcl->fulljid, prior, jcl->curquakeserver, caps);
	}
}

static void JCL_PrintBuddyStatus(char *console, jclient_t *jcl, buddy_t *b, bresource_t *r)
{
	if (r->servertype == 2)
	{
		char joinlink[512];
		JCL_GenLink(jcl, joinlink, sizeof(joinlink), "join", b->accountdomain, r->resource, NULL, "Playing Quake - %s", r->server);
		Con_SubPrintf(console, "%s", joinlink);
	}
	else if (r->servertype)
		Con_SubPrintf(console, "^[[Playing Quake - %s]\\observe\\%s^]", r->server, r->server);
	else if (*r->fstatus)
		Con_SubPrintf(console, "%s - %s", r->bstatus, r->fstatus);
	else
		Con_SubPrintf(console, "%s", r->bstatus);

	if ((r->caps & CAP_GAMEINVITE) && !r->servertype)
	{
		char invitelink[512];
		JCL_GenLink(jcl, invitelink, sizeof(invitelink), "invite", b->accountdomain, r->resource, NULL, "%s", "Invite");
		Con_SubPrintf(console, " %s", invitelink);
	}
	if (r->caps & CAP_VOICE)
	{
		char calllink[512];
		JCL_GenLink(jcl, calllink, sizeof(calllink), "call", b->accountdomain, r->resource, NULL, "%s", "Call");
		Con_SubPrintf(console, " %s", calllink);
	}
	else if ((r->caps|r->buggycaps) & CAP_GOOGLE_VOICE)
	{
		char calllink[512];
		JCL_GenLink(jcl, calllink, sizeof(calllink), "call", b->accountdomain, r->resource, NULL, "%s", "Call");
		Con_SubPrintf(console, " %s", calllink);
	}
}
static void JCL_RegenerateBuddyList(qboolean force)
{
	const char *console = BUDDYLISTTITLE;
	jclient_t *jcl;
	buddy_t *b, *me;
	bresource_t *r;
	int i, j;
	char convolink[512];
	struct c2c_s *c2c;

	buddy_t *sortlist[256];
	int buds;

	jclient_updatebuddylist = false;

	if (!confuncs)
		return;

	//only redraw the window if it actually exists. if they closed it, then don't mess things up.
	if (!force && confuncs->GetConsoleFloat(console, "iswindow") <= 0)
		return;

	if (confuncs->GetConsoleFloat(console, "iswindow") != true)
	{
		confuncs->SetConsoleFloat(console, "iswindow", true);
		confuncs->SetConsoleFloat(console, "forceutf8", true);
		confuncs->SetConsoleFloat(console, "linebuffered", false);
		confuncs->SetConsoleFloat(console, "wnd_x", pvid.width - 256);
		confuncs->SetConsoleFloat(console, "wnd_y", true);
		confuncs->SetConsoleFloat(console, "wnd_w", 256);
		confuncs->SetConsoleFloat(console, "wnd_h", pvid.height);
	}
	confuncs->SetConsoleFloat(console, "linecount", 0);	//clear it
	if (force)
		confuncs->SetActive(console);

	for (i = 0; i < sizeof(jclients)/sizeof(jclients[0]); i++)
	{
		jcl = jclients[i];
		if (!jcl)
			continue;
		if (*jcl->localalias && *jcl->localalias != '>')
			Con_SubPrintf(console, "\n"COL_NAME_US"%s\n", jcl->localalias);
		else
			Con_SubPrintf(console, "\n"COL_NAME_US"%s@%s\n", jcl->username, jcl->domain);
		if (jcl->status == JCL_INACTIVE)
			Con_SubPrintf(console, "Not connected.\n", jcl->accountnum);
		else if (jcl->status == JCL_DEAD)
			Con_SubPrintf(console, "%s.\n", *jcl->errormsg?jcl->errormsg:"Connect failed", jcl->accountnum);
		else if (jcl->status == JCL_AUTHING)
			Con_SubPrintf(console, "Connecting... Please wait.\n");

		JCL_GenLink(jcl, convolink, sizeof(convolink), "accopts", NULL, NULL, NULL, "%s", "Options");
		Con_SubPrintf(console, "%s\n", convolink);
/*
		if (jcl->status == JCL_INACTIVE)
			JCL_GenLink(jcl, convolink, sizeof(convolink), "forgetacc", NULL, NULL, NULL, "%s", "Forget Account");
		else if (jcl->status == JCL_DEAD)
			JCL_GenLink(jcl, convolink, sizeof(convolink), "disconnect", NULL, NULL, NULL, "%s", "Disable");
		else
			JCL_GenLink(jcl, convolink, sizeof(convolink), "disconnect", NULL, NULL, NULL, "%s", "Disconnect");
		Con_SubPrintf(console, "%s", convolink);
		if (jcl->status == JCL_INACTIVE)
		{
			JCL_GenLink(jcl, convolink, sizeof(convolink), "connect", NULL, NULL, NULL, "%s", "Connect");
			Con_SubPrintf(console, " %s", convolink);
		}
		else if (jcl->status == JCL_DEAD)
		{
			JCL_GenLink(jcl, convolink, sizeof(convolink), "connect", NULL, NULL, NULL, "%s", "Reconnect");
			Con_SubPrintf(console, " %s", convolink);
		}
*/

		for (c2c = jcl->c2c; c2c; c2c = c2c->next)
			c2c->displayed = false;

		if (jcl->status != JCL_ACTIVE)
			Con_SubPrintf(console, "\n");
		else
		{
			qboolean youarealoner = true;

//			JCL_GenLink(jcl, convolink, sizeof(convolink), "addfriend", NULL, NULL, NULL, "%s", "Add Friend");
//			Con_SubPrintf(console, " %s\n", convolink);

			JCL_FindBuddy(jcl, jcl->fulljid, &me, NULL, true);

			for (b = jcl->buddies, buds = 0; b && buds < sizeof(sortlist)/sizeof(sortlist[0]); b = b->next)
			{
				if (b == me)
					continue;
				if (!b->resources)	//can't be online.
					continue;
				if (b->btype == BT_UNKNOWN)
					continue;		//don't list people we don't actually know

				for (j = buds; j > 0; j--)
				{
					if (strcasecmp(sortlist[j-1]->name, b->name) >= 0)
						break;
					sortlist[j] = sortlist[j-1];
				}
				buds++;
				sortlist[j] = b;
			}
			Con_SubPrintf(console, "\n");

			while(buds --> 0)
			{
				bresource_t *gameres = NULL;
				bresource_t *voiceres = NULL;
				bresource_t *chatres = NULL;

				b = sortlist[buds];
				if (b->btype == BT_ROSTER)
				{	//gather capabilities from all of their resources
					for (r = b->resources; r; r = r->next)
					{
						if (!strcmp(r->bstatus, "offline"))
							continue;

						if ((r->caps & CAP_VOICE) && (!voiceres || r->priority > voiceres->priority))
							voiceres = r;
						if ((r->caps & CAP_GAMEINVITE) && (!gameres || r->priority > gameres->priority))
							gameres = r;
						if (!chatres || r->priority > chatres->priority)
							chatres = r;
					}
				}
				if (b->defaultresource)
				{
					r = b->defaultresource;
					if (r->caps & CAP_VOICE)
						voiceres = r;
					if (r->caps & CAP_GAMEINVITE)
						gameres = r;
					chatres = r;
				}
				if (chatres || b->btype == BT_CHATROOM)
				{
					youarealoner = false;

					if (b->vcardphotochanged && b->friended)
					{
						struct buddyinfo_s *bi;
						for (bi = jcl->buddyinfo; bi; bi = bi->next)
						{
							if (!strcmp(bi->accountdomain, b->accountdomain))
								break;
						}
						if (bi && !strcmp(b->vcardphotohash, bi->imagehash))
						{
							char photodata[65536];
							unsigned int photosize = bi->image?Base64_Decode(photodata, sizeof(photodata), bi->image, strlen(bi->image)):0;
							b->image = drawfuncs->LoadImageData(va("xmpp/%s.png", b->accountdomain), bi->imagemime, photodata, photosize);
							b->vcardphotochanged = false;
						}
						else if (!*b->vcardphotohash)
						{	//this buddy has no photo, so don't bother querying it
							b->vcardphotochanged = false;
							b->image = 0;
						}
						else if (jcl->avatarupdate == NULL)
						{
							b->vcardphotochanged = false;
							Con_DPrintf("Querying %s's photo\n", b->accountdomain);
							jcl->avatarupdate = JCL_SendIQf(jcl, JCL_BuddyVCardReply, "get", b->accountdomain, "<vCard xmlns='vcard-temp'/>");
						}
					}

					Q_snprintf(convolink, sizeof(convolink), "^[%s\\xmppacc\\%i\\xmpp\\%s^]", b->name, jcl->accountnum, b->accountdomain);

					Con_SubPrintf(console, "^[\\img\\xmpp/%s.png\\fbimg\\"IMG_FB_THEM"\\w\\32\\h\\32^] ", b->accountdomain);
					Con_SubPrintf(console, "%s", convolink);

					if (chatres && *chatres->fstatus)
						Con_SubPrintf(console, "\v  %s", chatres->fstatus);
					else if (b->btype == BT_CHATROOM && b->room_topic)
						Con_SubPrintf(console, "\v  %s", b->room_topic);

					JCL_GenLink(jcl, convolink, sizeof(convolink), "buddyopts", b->accountdomain, NULL, NULL, "^h%s^h", "Options");
					if (chatres)
						Con_SubPrintf(console, "\v  %s    %s", chatres->bstatus, convolink);
					else if (b->btype == BT_CHATROOM)
						Con_SubPrintf(console, "\v  %s    %s", "chatroom", convolink);
					else
						Con_SubPrintf(console, "\v  %s    %s", "unknown", convolink);

					for (c2c = jcl->c2c; c2c; c2c = c2c->next)
					{
						buddy_t *peer = NULL;
						qboolean voice = false, video = false, server = false;
						int c;
						if (JCL_FindBuddy(jcl, c2c->with, &peer, NULL, false) && peer == b)
						{
							for (c = 0; c < c2c->contents; c++)
							{
								switch(c2c->content[c].mediatype)
								{
								case ICEP_INVALID:					break;
								case ICEP_VOICE:	voice = true; 	break;
								case ICEP_VIDEO:	video = true; 	break;
								case ICEP_QWSERVER: server = true; 	break;
								case ICEP_QWCLIENT: /*client = true;*/ 	break;
								}
							}
							c2c->displayed = true;
							if (server)
							{
								JCL_GenLink(jcl, convolink, sizeof(convolink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Kick");
								gameres = NULL;
							}
							else if (video || voice)
							{
								JCL_GenLink(jcl, convolink, sizeof(convolink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Hang Up");
								voiceres = NULL;
							}
							else /*if (client)*/
							{
								JCL_GenLink(jcl, convolink, sizeof(convolink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Disconnect");
								gameres = NULL;
							}
							Con_SubPrintf(console, " %s", convolink);
						}
					}
					if (gameres)
					{
						if (*gameres->server)
							JCL_GenLink(jcl, convolink, sizeof(convolink), "join", b->accountdomain, gameres->resource, NULL, "Playing Quake - %s", gameres->server);
						else
							JCL_GenLink(jcl, convolink, sizeof(convolink), "invite", b->accountdomain, gameres->resource, NULL, "%s", "Invite");
						Con_SubPrintf(console, " %s", convolink);
					}
					if (voiceres)
					{
						JCL_GenLink(jcl, convolink, sizeof(convolink), "call", b->accountdomain, voiceres->resource, NULL, "%s", "Call");
						Con_SubPrintf(console, " %s", convolink);
					}
					Con_SubPrintf(console, "\n");
				}
			}
			if (youarealoner)
				Con_SubPrintf(console, "    You have no friends\n");
		}

		//and show any lingering c2cs with anyone not on our list.
		for (c2c = jcl->c2c; c2c; c2c = c2c->next)
		{
			if (!c2c->displayed)
			{
				qboolean voice = false, video = false, server = false;
				int c;

				for (c = 0; c < c2c->contents; c++)
				{
					switch(c2c->content[c].mediatype)
					{
					case ICEP_INVALID:					break;
					case ICEP_VOICE:	voice = true; 	break;
					case ICEP_VIDEO:	video = true; 	break;
					case ICEP_QWSERVER: server = true; 	break;
					case ICEP_QWCLIENT: /*client = true;*/ 	break;
					}
				}
				c2c->displayed = true;
				if (server)
					JCL_GenLink(jcl, convolink, sizeof(convolink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Kick");
				else if (video || voice)
					JCL_GenLink(jcl, convolink, sizeof(convolink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Hang Up");
				else /*if (client)*/
					JCL_GenLink(jcl, convolink, sizeof(convolink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Disconnect");
				Con_SubPrintf(console, "%s: %s\n", c2c->with, convolink);
			}
		}
	}

	for (i = 0; i < sizeof(jclients)/sizeof(jclients[0]); i++)
		if (!jclients[i])
		{
			JCL_GenLink(NULL, convolink, sizeof(convolink), "newaccount", NULL, NULL, NULL, "%s", "Open New Account");
			Con_SubPrintf(console, "\n%s\n", convolink);
			break;
		}
}

static void JCL_PrintBuddyList(char *console, jclient_t *jcl, qboolean all)
{
	buddy_t *b;
	bresource_t *r;
	struct c2c_s *c2c;
	struct ft_s *ft;
	char convolink[512], actlink[512];
	int c;

	if (!jcl->buddies)
		Con_SubPrintf(console, "You have no friends\n");
	for (b = jcl->buddies; b; b = b->next)
	{	
		//if we don't actually know them, don't list them.
		if (b->btype == BT_UNKNOWN)
			continue;

		if (b->btype == BT_CHATROOM)
		{
			r = b->resources;
			JCL_GenLink(jcl, convolink, sizeof(convolink), NULL, b->accountdomain, r->resource, NULL, "%s", b->name);
			Con_SubPrintf(console, "%s: ", convolink);
			JCL_PrintBuddyStatus(console, jcl, b, r);
			Con_SubPrintf(console, "\n");
		}
		else if (!b->resources)	//offline
		{
			if (all)
			{
				JCL_GenLink(jcl, convolink, sizeof(convolink), NULL, b->accountdomain, NULL, NULL, "%s", b->name);
				Con_SubPrintf(console, "%s: offline\n", convolink);
			}
		}
		else if (b->resources->next)
		{	//multiple potential resources
			JCL_GenLink(jcl, convolink, sizeof(convolink), NULL, b->accountdomain, NULL, NULL, "%s", b->name);
			Con_SubPrintf(console, "%s:\n", convolink);
			for (r = b->resources; r; r = r->next)
			{
				JCL_GenLink(jcl, convolink, sizeof(convolink), NULL, b->accountdomain, r->resource, NULL, "%s", r->resource);
				Con_SubPrintf(console, "    %s: ", convolink);
				JCL_PrintBuddyStatus(console, jcl, b, r);
				Con_SubPrintf(console, "\n");
			}
		}
		else	//only one resource
		{
			r = b->resources;
			JCL_GenLink(jcl, convolink, sizeof(convolink), NULL, b->accountdomain, r->resource, NULL, "%s", b->name);
			Con_SubPrintf(console, "%s: ", convolink);
			JCL_PrintBuddyStatus(console, jcl, b, r);
			Con_SubPrintf(console, "\n");
		}
	}

#ifdef JINGLE
	if (jcl->c2c)
		Con_SubPrintf(console, "Active sessions:\n");
	for (c2c = jcl->c2c; c2c; c2c = c2c->next)
	{
		qboolean voice = false, video = false, server = false, client = false;
		JCL_FindBuddy(jcl, c2c->with, &b, &r, true);
		for (c = 0; c < c2c->contents; c++)
		{
			switch(c2c->content[c].mediatype)
			{
			case ICEP_INVALID:					break;
			case ICEP_VOICE:	voice = true; 	break;
			case ICEP_VIDEO:	video = true; 	break;
			case ICEP_QWSERVER: server = true; 	break;
			case ICEP_QWCLIENT: client = true; 	break;
			}
		}
		JCL_GenLink(jcl, convolink, sizeof(convolink), NULL, b->accountdomain, r->resource, NULL, "%s", b->name);
		Con_SubPrintf(console, "    %s: ", convolink);
		if (video)
			Con_SubPrintf(console, "video ");
		if (voice)
			Con_SubPrintf(console, "voice ");
		if (server)
			Con_SubPrintf(console, "server ");
		if (client)
			Con_SubPrintf(console, "client ");
		if (server)
			JCL_GenLink(jcl, actlink, sizeof(actlink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Kick");
		else if (video || voice)
			JCL_GenLink(jcl, actlink, sizeof(actlink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Hang Up");
		else
			JCL_GenLink(jcl, actlink, sizeof(actlink), "jdeny", c2c->with, NULL, c2c->sid, "%s", "Disconnect");
		Con_SubPrintf(console, "%s\n", actlink);
	}
#endif

#ifdef FILETRANSFERS
	if (jcl->ft)
		Con_SubPrintf(console, "Active file transfers:\n");
	for (ft = jcl->ft; ft; ft = ft->next)
	{
		JCL_FindBuddy(jcl, ft->with, &b, &r, true);
		JCL_GenLink(jcl, convolink, sizeof(convolink), NULL, b->accountdomain, r->resource, NULL, "%s", b->name);
		JCL_GenLink(jcl, actlink, sizeof(actlink), "fdeny", ft->with, NULL, ft->sid, "%s", "Cancel");
		Con_SubPrintf(console, "    %s: %s\n", convolink, ft->fname);
	}
#endif
}

//functions above this line allow connections to multiple servers.
//it is just the control functions that only allow one server.

void JCL_Frame(double realtime, double gametime)
{
	int i;
	jclient_curtime = realtime;
	if (jclient_needreadconfig)
	{
		JCL_LoadConfig();
		JCL_RegenerateBuddyList(false);
	}
	if (jclient_updatebuddylist)
		JCL_RegenerateBuddyList(false);

	for (i = 0; i < sizeof(jclients)/sizeof(jclients[0]); i++)
	{
		jclient_t *jcl = jclients[i];
		if (jcl && jcl->status != JCL_INACTIVE)
		{
			int stat = JCL_CONTINUE;
			if (jcl->status == JCL_DEAD)
			{
				if (jclient_curtime > jcl->timeout)
				{
					JCL_Reconnect(jcl);
					jcl->timeout = jclient_curtime + 60*1000;
					jclient_updatebuddylist = true;
				}
			}
			else
			{
				char *error = "";
				if (jcl->connected)
					JCL_GeneratePresence(jcl, false);
				while(stat == JCL_CONTINUE)
					stat = JCL_ClientFrame(jcl, &error);
				if (stat == JCL_NUKEFROMORBIT)
				{
					JCL_CloseConnection(jcl, error, true);
					jcl->status = JCL_INACTIVE;
				}
				else if (stat == JCL_KILL)
					JCL_CloseConnection(jcl, error, true);
				else
					JCL_FlushOutgoing(jcl);

				if (stat == JCL_DONE)
				{
					XMPP_FT_Frame(jcl);

					if (jclient_curtime > jcl->timeout)
					{
						//client needs to send something valid as a keep-alive so routers don't silently forget mappings.
						JCL_SendIQf(jcl, NULL, "get", NULL, "<ping xmlns='urn:xmpp:ping'/>");
						jcl->timeout = jclient_curtime + 60*1000;
					}

					if (jcl->status == JCL_ACTIVE)
					{
						if (jcl->rcon_pipe >= 0)
						{
							char rcondata[64000];
							int rconsize = netfuncs->Recv(jcl->rcon_pipe, rcondata, sizeof(rcondata)-1);
							if (rconsize > 0)
							{
								char *ls, *le;
								rcondata[rconsize] = 0;
								ls = rcondata;
								while (ls)
								{
									le = strchr(ls, '\n');
									if (le)
										*le++ = 0;
									JCL_AddClientMessagef(jcl, "<message to='%s'><body>%s</body></message>", jcl->rcon_peer, ls);
									ls = le;
								}
							}
							else if (rconsize < 0)
							{
								JCL_AddClientMessagef(jcl, "<message to='%s'><body>%s</body></message>", jcl->rcon_peer, "RCON ERROR");
								filefuncs->Close(jcl->rcon_pipe);
								jcl->rcon_pipe = -1;
							}
						}
					}
				}
			}

#ifdef JINGLE
			JCL_JingleTimeouts(jcl, false);
#endif
			JCL_IQTimeouts(jcl);
		}
	}
}

void JCL_WriteConfig(void)
{
	xmltree_t *m, *n, *oauth2, *features;
	int i, j;
	qhandle_t config;
	struct buddyinfo_s *bi;
	buddy_t *bud;

	if (!jclient_configdirty)
		return; //no point yet.
	jclient_configdirty = false;

	//don't write the config if we're meant to be reading it. avoid wiping it if we're killed fast.
	if (jclient_needreadconfig)
		return;


	m = XML_CreateNode(NULL, "xmppaccounts", "", "");
	for (i = 0; i < sizeof(jclients) / sizeof(jclients[0]);  i++)
	{
		jclient_t *jcl = jclients[i];
		if (jcl)
		{
			char foo[64];
			n = XML_CreateNode(m, "account", "", "");
			XML_AddParameteri(n, "id", i);

			Q_snprintf(foo, sizeof(foo), "%i", jcl->streamdebug);
			XML_CreateNode(n, "streamdebug", "", foo);
			Q_snprintf(foo, sizeof(foo), "%i", jcl->forcetls);
			XML_CreateNode(n, "forcetls", "", foo);
			XML_CreateNode(n, "savepassword", "", jcl->savepassword?"1":"0");
			XML_CreateNode(n, "allowauth_plain_nontls", "", jcl->sasl.allowauth_plainnontls?"1":"0");
			XML_CreateNode(n, "allowauth_plain_tls", "", jcl->sasl.allowauth_plaintls?"1":"0");
			XML_CreateNode(n, "allowauth_digest_md5", "", jcl->sasl.allowauth_digestmd5?"1":"0");
			XML_CreateNode(n, "allowauth_scram_sha_1", "", jcl->sasl.allowauth_scramsha1?"1":"0");

			if (*jcl->sasl.oauth2.saslmethod)
			{
				XML_CreateNode(n, "allowauth_oauth2", "", jcl->sasl.allowauth_oauth2?"1":"0");
				oauth2 = XML_CreateNode(n, "oauth2", "", "");
				XML_AddParameter(oauth2, "method", jcl->sasl.oauth2.saslmethod);
				XML_CreateNode(oauth2, "obtain-url", "", jcl->sasl.oauth2.obtainurl);
				XML_CreateNode(oauth2, "refresh-url", "", jcl->sasl.oauth2.refreshurl);
				XML_CreateNode(oauth2, "client-id", "", jcl->sasl.oauth2.clientid);
				XML_CreateNode(oauth2, "client-secret", "", jcl->sasl.oauth2.clientsecret);
				XML_CreateNode(oauth2, "scope", "", jcl->sasl.oauth2.scope);
				XML_CreateNode(oauth2, "auth-token", "", jcl->sasl.oauth2.authtoken);
				XML_CreateNode(oauth2, "refresh-token", "", jcl->sasl.oauth2.refreshtoken);
				XML_CreateNode(oauth2, "access-token", "", jcl->sasl.oauth2.accesstoken);
			}

			XML_CreateNode(n, "username", "", jcl->username);
			XML_CreateNode(n, "domain", "", jcl->domain);
			XML_CreateNode(n, "resource", "", jcl->resource);
			if (jcl->savepassword)
				XML_CreateNode(n, "password", "", jcl->sasl.password_plain);	//FIXME: should we base64 this just to obscure it? probably not. trivial obscurity does few favours.
			XML_CreateNode(n, "serveraddr", "", jcl->serveraddr);
			Q_snprintf(foo, sizeof(foo), "%i", jcl->serverport);
			XML_CreateNode(n, "serverport", "", foo);
			XML_CreateNode(n, "certificatedomain", "", jcl->certificatedomain);
			XML_CreateNode(n, "inactive", "", jcl->status == JCL_INACTIVE?"1":"0");

			//write out what optional/risky features should be enabled (like file transfers, jingle, etc)
			features = XML_CreateNode(n, "features", "", "");
			XML_AddParameter(features, "ver", JCL_BUILD);
			for (j = 0; capnames[j].names; j++)
			{
				XML_CreateNode(features, capnames[j].names, "", (jcl->enabledcapabilities & capnames[j].cap)?"1":"0");
			}

			//write a list of chatrooms that can't be saved in rosters
			features = XML_CreateNode(n, "chats", "", "");
			for (bud = jcl->buddies; bud; bud = bud->next)
			{
				if (bud->btype == BT_CHATROOM)
				{
					xmltree_t *b = XML_CreateNode(features, "room", "", "");
					XML_AddParameter(b, "name", bud->accountdomain);
					XML_AddParameter(b, "alias", bud->name);
					XML_AddParameter(b, "nick", bud->room_nick);
					XML_AddParameter(b, "password", bud->room_password);
					XML_AddParameteri(b, "autojoin", bud->room_autojoin);
				}
			}

			//write an avatar cache
			//FIXME: some sort of timeout? only people on our roster?
			features = XML_CreateNode(n, "buddyinfo", "", "");
			for (bi = jcl->buddyinfo; bi; bi = bi->next)
			{
				xmltree_t *b = XML_CreateNode(features, "buddy", "", "");
				XML_AddParameter(b, "name", bi->accountdomain);

				if (bi->image)
					XML_CreateNode(b, "image", "", bi->image);
				if (bi->imagemime)
					XML_CreateNode(b, "imagemime", "", bi->imagemime);
				if (bi->imagehash)
					XML_CreateNode(b, "imagehash", "", bi->imagehash);
			}

			//FIXME: client disco info
		}
	}

	filefuncs->Open("**plugconfig", &config, 2);
	if (config >= 0)
	{
		char *s = XML_GenerateString(m, true);
		filefuncs->Write(config, s, strlen(s));
		free(s);

		filefuncs->Close(config);
	}
	XML_Destroy(m);
}
void JCL_LoadConfig(void)
{
	jclient_needreadconfig = false;
	if (!jclients[0])
	{
		int len;
		qhandle_t config;
		char *buf;
		qboolean oldtls;
		jclient_configdirty = false;
		len = filefuncs->Open("**plugconfig", &config, 1);
		if (len >= 0)
		{
			buf = malloc(len+1);
			buf[len] = 0;
			filefuncs->Read(config, buf, len);
			filefuncs->Close(config);

			if (len && *buf != '<')
			{//legacy code, to be removed
				char *line = buf;
				char tls[256];
				char server[256];
				char account[256];
				char password[256];
				line = JCL_ParseOut(line, tls, sizeof(tls));
				line = JCL_ParseOut(line, server, sizeof(server));
				line = JCL_ParseOut(line, account, sizeof(account));
				line = JCL_ParseOut(line, password, sizeof(password));

				oldtls = atoi(tls);

				Con_Printf("Legacy config: %s (%i)\n", buf, len);
				jclients[0] = JCL_Connect(0, server, oldtls, account, password);
			}
			else
			{
				xmltree_t *accs;
				int start = 0;
				accs = XML_Parse(buf, &start, len, false, "");
				if (accs)
				{
					int i;
					xmltree_t *acc;
					for (i = 0; (acc = XML_ChildOfTree(accs, "account", i)); i++)
					{
						int id = atoi(XML_GetParameter(acc, "id", "0"));
						if (id < 0 || id >= sizeof(jclients) / sizeof(jclients[0]) || jclients[id])
							continue;

						jclients[id] = JCL_ConnectXML(acc);
					}
					XML_Destroy(accs);
				}
			}
			free(buf);
		}
	}
}

//on shutdown, write config and close connections.
void JCL_Shutdown(void)
{
	jclient_t *jcl;
	int i;
	JCL_WriteConfig();
	for (i = 0; i < sizeof(jclients)/sizeof(jclients[0]); i++)
	{
		jcl = jclients[i];
		if (jcl)
			JCL_CloseConnection(jcl, "", false);
	}

//	if (_CrtDumpMemoryLeaks())
//		OutputDebugStringA("Leaks detected\n");
}

void JCL_SendMessage(jclient_t *jcl, char *to, char *msg)
{
	char markup[1024];
	char *con, *title;
	buddy_t *b;
	bresource_t *br;
	JCL_FindBuddy(jcl, to, &b, &br, true);
	if (!b)
	{
		Con_Printf("Can't find buddy \"%s\"\n", to);
		return;
	}
	if (b->btype == BT_CHATROOM)
	{
		if (br)
		{	//unicast...
			JCL_AddClientMessagef(jcl, "<message to='%s/%s' type='chat'><body>", b->accountdomain, br->resource);
			con = to;
		}
		else
		{	//send to all recipients
			JCL_AddClientMessagef(jcl, "<message to='%s' type='groupchat'><body>", b->accountdomain);
			con = b->name;
		}
		title = con;
	}
	else
	{
		title = b->name;
		con = b->accountdomain;
		if (!br)
			br = b->defaultresource;
		if (br)
			JCL_AddClientMessagef(jcl, "<message to='%s/%s'><body>", b->accountdomain, br->resource);
		else
			JCL_AddClientMessagef(jcl, "<message to='%s'><body>", b->accountdomain);
	}
	JCL_AddClientMessage(jcl, markup, XML_Markup(msg, markup, sizeof(markup)) - markup);
	JCL_AddClientMessageString(jcl, "</body></message>");
	if (b->btype == BT_CHATROOM && !br)
		return;
	if (!strncmp(msg, "/me ", 4))
		XMPP_ConversationPrintf(con, title, false, "* "COL_NAME_US"%s^7"COL_TEXT_US"%s\n", ((!strcmp(jcl->localalias, ">>"))?"me":jcl->localalias), msg+3);
	else if (b->btype == BT_ROSTER && confuncs)
		XMPP_ConversationPrintf(con, title, false, "^[\\img\\xmpp/%s.png\\fbimg\\"IMG_FB_US"\\w\\32\\h\\32^]"COL_NAME_US"%s^7:\v"COL_TEXT_US"%s\n", jcl->barejid, jcl->localalias, msg);
	else
		XMPP_ConversationPrintf(con, title, false, COL_NAME_US"%s^7: "COL_TEXT_US"%s\n", jcl->localalias, msg);
}
void JCL_AttentionMessage(jclient_t *jcl, char *to, char *msg)
{
	char fullto[256];
	buddy_t *b = NULL;
	bresource_t *br = NULL;
	xmltree_t *m;
	char *s;

	JCL_FindBuddy(jcl, to, &b, &br, true);
	if (!b)
		return;
	if (!br)
		br = b->defaultresource;
	if (!br)
		br = b->resources;
	if (!br)
	{
		Con_SubPrintf(b->accountdomain, "User is not online\n");
		return;
	}
	Q_snprintf(fullto, sizeof(fullto), "%s/%s", b->accountdomain, br->resource);

	m = XML_CreateNode(NULL, "message", "", "");
	XML_AddParameter(m, "to", fullto);
//	XML_AddParameter(m, "type", "headline");

	XML_CreateNode(m, "attention", "urn:xmpp:attention:0", "");
	if (msg)
		XML_CreateNode(m, "body", "", msg);

	s = XML_GenerateString(m, false);
	JCL_AddClientMessageString(jcl, s);
	free(s);
	XML_Destroy(m);

	if (msg)
	{
		if (!strncmp(msg, "/me ", 4))
			Con_SubPrintf(b->accountdomain, "*"COL_NAME_US"%s^7"COL_TEXT_US"%s\n", ((!strcmp(jcl->localalias, ">>"))?"me":jcl->localalias), msg+3);
		else
			Con_SubPrintf(b->accountdomain, COL_NAME_US"%s^7: "COL_TEXT_US"%s\n", jcl->localalias, msg);
	}
}

static qboolean JCL_IsChatroom(jclient_t *jcl, char *room)
{
	buddy_t *b;

	if (!jcl)
		return false;

	if (!JCL_FindBuddy(jcl, room, &b, NULL, true))
		return false;

	return b->btype == BT_CHATROOM;
}

//server may be null, in which case its expected to be folded into room.
void JCL_JoinMUCChat(jclient_t *jcl, const char *room, const char *server, const char *myhandle, const char *password)
{
	char caps[512];
	char roomserverhandle[512];
	buddy_t *b;
//	bresource_t *r;
	if (server)
		Q_snprintf(roomserverhandle, sizeof(roomserverhandle), "%s@%s", room, server);
	else
		Q_snprintf(roomserverhandle, sizeof(roomserverhandle), "%s", room);
	if (!JCL_FindBuddy(jcl, roomserverhandle, &b, NULL, true))
		return;
	if (!myhandle)
	{
		if (b->room_nick)
			myhandle = b->room_nick;
		else
			myhandle = jcl->username;
	}
	b->btype = BT_CHATROOM;
	buildcapsvcardpresence(jcl, caps, sizeof(caps));
	//FIXME: check for errors
	JCL_AddClientMessagef(jcl, 
				"<presence to='%s/%s'>"
					"<x xmlns='http://jabber.org/protocol/muc'>"
						"<password>%s</password>"
						//"<history maxstanzas= maxchars= seconds= since=/>"
					"</x>"
					"%+s"
				"</presence>"
				, roomserverhandle, myhandle, password?password:"", caps);

	if (b->room_nick != myhandle)
	{
		free(b->room_nick);
		b->room_nick = myhandle?strdup(myhandle):NULL;
	}
	if (b->room_password != password)
	{
		free(b->room_password);
		b->room_password = myhandle?strdup(password):NULL;
	}
}

void XMPP_Menu_Password(jclient_t *acc)
{
//	int y;

	if (!jclient_action)
	{
		JCL_RegenerateBuddyList(true);
		confuncs->SetConsoleFloat(BUDDYLISTTITLE, "linebuffered", true);
		jclient_action_cl = acc;
		jclient_action_buddy = NULL;
		jclient_action = ACT_SETAPASSWORD;
	}

	/*
	cmdfuncs->AddText("conmenu\n"
				"{\n"
					"menuclear\n"
					"if (option == \"SignIn\")\n"
					"{\n"
					COMMANDPREFIX" /set savepassword $_t1\n"
					COMMANDPREFIX" /password ${0}\n"
					"}\n"
				"}\n", false);

	y = 36;
	cmdfuncs->AddText(va("menutext 48 %i \"^sXMPP Sign In\"\n", y), false); y+=16;
	cmdfuncs->AddText(va("menutext 48 %i \"^sPlease provide your password for\"\n", y), false); y+=16;
	cmdfuncs->AddText(va("menueditpriv 48 %i \"%s@%s\" \"example\"\n", y, acc->username, acc->domain), false);y+=16;
	cmdfuncs->AddText(va("set _t1 0\nmenucheck 48 %i \"Save Password\" _t1 1\n", y), false); y+=16;
	cmdfuncs->AddText(va("menutext 48 %i \"Sign In\" SignIn\n", y), false);
	cmdfuncs->AddText(va("menutext 256 %i \"Cancel\" cancel\n", y), false);
	*/
}
void XMPP_Menu_Connect(void)
{
	int y;
	cmdfuncs->AddText("conmenu\n"
				"{\n"
					"menuclear\n"
					"if (option == \"SignIn\")\n"
					"{\n"COMMANDPREFIX" /connect ${0}@${1}/${2}\n}\n"
				"}\n", false);

	y = 36;
	cmdfuncs->AddText(va("menutext 48 %i \"^sXMPP Sign In\"\n", y), false); y+=16;
	cmdfuncs->AddText(va("menueditpriv 48 %i \"Username\" \"example\"\n", y), false);y+=16;
	cmdfuncs->AddText(va("menueditpriv 48 %i \"Domain\" \""EXAMPLEDOMAIN"\"\n", y), false);y+=16;
	cmdfuncs->AddText(va("menueditpriv 48 %i \"Resource\" \"\"\n", y), false);y+=32;
	cmdfuncs->AddText(va("menutext 48 %i \"Sign In\" SignIn\n", y), false);
	cmdfuncs->AddText(va("menutext 256 %i \"Cancel\" cancel\n", y), false);
}

void JCL_Command(int accid, char *console)
{
	char imsg[8192];
	char arg[6][1024], *argstart[6];
	char *msg;
	int i;
	char nname[256];
	jclient_t *jcl;

	if (accid < 0 || accid >= sizeof(jclients)/sizeof(jclients[0]))
		return;
	jcl = jclients[accid];

	cmdfuncs->Args(imsg, sizeof(imsg));

	msg = imsg;
	for (i = 0; i < 6; i++)
	{
		if (!msg)
		{
			argstart[i] = "";
			*arg[i] = 0;
		}
		else
		{
			while (*msg == ' ')
				msg++;
			argstart[i] = msg;
			msg = JCL_ParseOut(msg, arg[i], sizeof(arg[i]));
		}
	}

	if (arg[0][0] == '/' && arg[0][1] != '/' && strcmp(arg[0]+1, "me"))
	{
		if (*console && !strcmp(arg[0]+1, "ban") && JCL_IsChatroom(jcl, console))
		{
			char *roomnick = arg[1];
			char *comment = arg[2];
			char *jid;
			if (strchr(roomnick, '@'))
				jid = roomnick;
			else
			{
				Con_TrySubPrint(console, "Unable to translate roomnicks. Please use their bare jid.\n");
				return;	//FIXME: translate from roomnick to real barejid
			}

			JCL_SendIQf(jcl, NULL, "set", console,
					"<query xmlns='http://jabber.org/protocol/muc#admin'>"
						"<item affiliation='outcast' jid='%s'>"
							"<reason>%s</reason>"
						"</item>"
					"</query>"
					, jid, comment
				);
		}
		else if (*console && !strcmp(arg[0]+1, "invite") && JCL_IsChatroom(jcl, console))
		{
			char *jid = arg[1];
			char *reason = arg[2];
			JCL_AddClientMessagef(jcl, 
				"<message to='%s'>"
					"<x xmlns='http://jabber.org/protocol/muc#user'>"
						"<invite to='%s'>"
							"<reason>%s</reason>"
						"</invite>"
					"</x>"
				"</message>"
				, console, jid, reason
				);
		}
		else if (*console && !strcmp(arg[0]+1, "join") && JCL_IsChatroom(jcl, console))
		{
			char *room = arg[1];
			char *nick = "";
			char *pass = arg[2];
			JCL_AddClientMessagef(jcl,
				"<presence to='%s/%s'>"
					"<x xmlns='http://jabber.org/protocol/muc#user'>"
						"<password>%s</password>"
					"</x>"
				"</presence>"
				, room, nick, pass
				);
		}
		else if (*console && !strcmp(arg[0]+1, "kick") && JCL_IsChatroom(jcl, console))
		{
			char *roomnick = arg[1];
			char *comment = arg[2];
			JCL_SendIQf(jcl, NULL, "set", console,
					"<query xmlns='http://jabber.org/protocol/muc#admin'>"
						"<item nick='%s' role='none'>"
							"<reason>%s</reason>"
						"</item>"
					"</query>"
					,roomnick, comment
				);
		}
		else if (*console && !strcmp(arg[0]+1, "msg") && JCL_IsChatroom(jcl, console))
		{
			char *whom = arg[1];
			char *msgtext = argstart[2];
			JCL_AddClientMessagef(jcl,
				"<message to='%s/%s' type='chat'>"
					"<body>%s</body>"
				"</message>"
				, console, whom, msgtext
				);
		}
		else if (*console && !strcmp(arg[0]+1, "nick") && JCL_IsChatroom(jcl, console))
		{	//FIXME: needs escape
			char *mynewnick = arg[1];
			JCL_AddClientMessagef(jcl,
				"<presence to='%s/%s'/>"
				, console, mynewnick
				);
		}
		else if (*console && !strcmp(arg[0]+1, "part") && JCL_IsChatroom(jcl, console))
		{	//FIXME: escapes
			buddy_t *b;
			char *myroomnick = "";
			char *comment = argstart[1];

			if (JCL_FindBuddy(jcl, console, &b, NULL, false))
				if (b && b->ourselves)
					myroomnick = b->ourselves->resource;

			JCL_AddClientMessagef(jcl,
				"<presence to='%s/%s' type='unavailable'>"
					"<status>%s</status>"
				"</presence>"
				, console, myroomnick, comment
				);

			if (b && b->btype == BT_CHATROOM)
				JCL_ForgetBuddy(jcl, b, NULL);
		}
		else if (*console && !strcmp(arg[0]+1, "topic") && JCL_IsChatroom(jcl, console))
		{	//FIXME: escapes
			char *newtopic = argstart[1];
			JCL_AddClientMessagef(jcl,
				"<message to='%s' type='groupchat'>"
					"<subject>%s</subject>"
				"</message>"
				, console, newtopic
				);
		}
		else if (*console && !strcmp(arg[0]+1, "userlist") && JCL_IsChatroom(jcl, console))
		{
			buddy_t *b;
			bresource_t *r;
			if (JCL_FindBuddy(jcl, console, &b, NULL, false))
			{
				Con_SubPrintf(console, "Users on channel %s\n", console);
				for (r = b->resources; r; r = r->next)
				{
					Con_SubPrintf(console, " %s:", r->resource);
					JCL_PrintBuddyStatus(console, jcl, b, r);
					Con_SubPrintf(console, "\n");
				}
				Con_SubPrintf(console, "<END>\n");
			}
		}
		else if (!strcmp(arg[0]+1, "open") || !strcmp(arg[0]+1, "connect") || !strcmp(arg[0]+1, "autoopen") || !strcmp(arg[0]+1, "autoconnect") || !strcmp(arg[0]+1, "plainopen") || !strcmp(arg[0]+1, "plainconnect") || !strcmp(arg[0]+1, "tlsopen") || !strcmp(arg[0]+1, "tlsconnect"))
		{	//tlsconnect is 'old'.
			int tls;
			if (!*arg[1])
			{
				XMPP_Menu_Connect();
				Con_SubPrintf(console, "%s <account@domain/resource> <password> <server>\n", arg[0]+1);
				return;
			}

			if (jcl)
			{
				Con_TrySubPrint(console, "You are already connected\nPlease /quit first\n");
				return;
			}
			if (!strncmp(arg[0]+1, "tls", 3))
				tls = 2;	//old initial-tls connect
			else if (!strncmp(arg[0]+1, "plain", 5))
				tls = -1;	//don't bother with tls. at all.
			else if (!strncmp(arg[0]+1, "auto", 4))
				tls = 0;	//use tls if its available, but don't really care otherwise.
			else
				tls = 1;	//require tls
			jcl = jclients[accid] = JCL_Connect(accid, arg[3], tls, arg[1], arg[2]);
			if (!jclients[accid])
			{
				Con_TrySubPrint(console, "Connect failed\n");
				return;
			}
			jclient_configdirty = true;
		}
		else if (!strcmp(arg[0]+1, "help")) 
		{
			Con_TrySubPrint(console, "^[/" COMMANDPREFIX " /connect USERNAME@DOMAIN/RESOURCE [PASSWORD] [XMPPSERVER]^]\n");
			Con_TrySubPrint(console, "eg for gmail: ^[/" COMMANDPREFIX " /connect myusername@gmail.com^] (using oauth2)\n");
			Con_TrySubPrint(console, "eg for gmail: ^[/" COMMANDPREFIX " /connect myusername@gmail.com mypassword^] (warning: password will be saved locally in plain text)\n");
//			Con_TrySubPrint(console, "eg for facebook: ^[/" COMMANDPREFIX " /connect myusername@chat.facebook.com mypassword chat.facebook.com^]\n");
//			Con_TrySubPrint(console, "eg for msn: ^[/" COMMANDPREFIX " /connect myusername@messanger.live.com mypassword^]\n");
			Con_TrySubPrint(console, "Note that this info will be used the next time you start quake.\n");

			//small note:
			//for the account 'me@example.com' the server to connect to can be displayed with:
			//nslookup -querytype=SRV _xmpp-client._tcp.example.com
			//srv resolving seems to be non-standard on each system, I don't like having to special case things.
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /help^]\n"
										"This text...\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /raw <XML STANZAS/>^]\n"
										"For debug hackery.\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /friend accountname@domain friendlyname^]\n"
										"Befriends accountname, and shows them in your various lists using the friendly name. Can also be used to rename friends.\n");
			Con_TrySubPrint(console,	"^[/" COMMANDPREFIX " /unfriend accountname@domain^]\n"
										"Ostracise your new best enemy. You will no longer see them and they won't be able to contact you.\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /blist^]\n"
										"Show all your friends! Names are clickable and will begin conversations.\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /quit^]\n"
										"Disconnect from the XMPP server, noone will be able to hear you scream.\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /join accountname@domain^]\n"
										"Joins your friends game (they will be prompted).\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /invite accountname@domain^]\n"
										"Invite someone to join your game (they will be prompted).\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /voice accountname@domain^]\n"
										"Begin a bi-directional peer-to-peer voice conversation with someone (they will be prompted).\n");
			Con_TrySubPrint(console, 	"^[/" COMMANDPREFIX " /msg ACCOUNTNAME@domain \"your message goes here\"^]\n"
										"Sends a message to the named person. If given a resource postfix, your message will be sent only to that resource.\n");
			Con_TrySubPrint(console, 	"If no arguments, will print out your friends list. If no /command is used, the arguments will be sent as a message to the person you last sent a message to.\n");
		}
		else if (!strcmp(arg[0]+1, "clear"))
		{
			//just clears the current console.
			if (*console)
			{
				confuncs->Destroy(console);
				Con_SubPrintf(console, "");
				if (confuncs)
					confuncs->SetActive(console);
			}
			else
				cmdfuncs->AddText("\nclear\n", true);
		}
		else if (!jcl)
		{
			Con_SubPrintf(console, "No account specified. Cannot %s\n", arg[0]);
		}
		else if (!strcmp(arg[0]+1, "oa2token"))
		{
			jclient_configdirty = true;
			free(jcl->sasl.oauth2.authtoken);
			jcl->sasl.oauth2.authtoken = strdup(arg[1]);
			if (jcl->status == JCL_INACTIVE)
				jcl->status = JCL_DEAD;
		}
		else if (!strcmp(arg[0]+1, "set"))
		{
			jclient_configdirty = true;
			if (!strcmp(arg[1], "savepassword"))
				jcl->savepassword = atoi(arg[2]);
			else if (!strcmp(arg[1], "avatars"))
			{
				jcl->enabledcapabilities = (jcl->enabledcapabilities & ~CAP_AVATARS) | (atoi(arg[2])?CAP_AVATARS:0);
				JCL_GeneratePresence(jcl, true);
			}
			else if (!strcmp(arg[1], "ft"))
			{
				jcl->enabledcapabilities = (jcl->enabledcapabilities & ~CAP_SIFT) | (atoi(arg[2])?CAP_SIFT:0);
				JCL_GeneratePresence(jcl, false);
			}
			else if (!strcmp(arg[1], "debug"))
				jcl->streamdebug = atoi(arg[2]);
			else if (!strcmp(arg[1], "resource"))
				Q_strlcpy(jcl->resource, arg[2], sizeof(jcl->resource));
			else
				Con_SubPrintf(console, "Sorry, setting not recognised.\n", arg[0]);
		}
		else if (!strcmp(arg[0]+1, "password"))
		{
			jclient_configdirty = true;
			Q_strncpyz(jcl->sasl.password_plain, arg[1], sizeof(jcl->sasl.password_plain));
			jcl->sasl.password_hash_size = 0;
			if (jcl->status == JCL_INACTIVE)
				jcl->status = JCL_DEAD;
		}
		else if (!strcmp(arg[0]+1, "quit"))
		{
			//disconnect from the xmpp server.
			JCL_CloseConnection(jcl, "/quit", false);
		}
		else if (jcl->status != JCL_ACTIVE)
		{
			Con_SubPrintf(console, "You are not authed. Please wait.\n", arg[0]);
		}
		else if (!strcmp(arg[0]+1, "blist"))
		{
			//print out a full list of everyone, even those offline.
			JCL_PrintBuddyList(console, jcl, true);
		}
		else if (!strcmp(arg[0]+1, "msg"))
		{
			//FIXME: validate the dest. deal with xml markup in dest.
			Q_strlcpy(jcl->defaultdest, arg[1], sizeof(jcl->defaultdest));
			msg = arg[2];

			//reparse the commands, so we get all trailing text
			msg = imsg;
			msg = JCL_ParseOut(msg, arg[0], sizeof(arg[0]));
			msg = JCL_ParseOut(msg, arg[1], sizeof(arg[1]));
			while(*msg == ' ')
				msg++;

			JCL_SendMessage(jcl, jcl->defaultdest, msg);
		}
		else if (!strcmp(arg[0]+1, "friend")) 
		{
			XMPP_AddFriend(jcl, arg[1], arg[2]);
		}
		else if (!strcmp(arg[0]+1, "unfriend")) 
		{
			buddy_t *b;
			if (JCL_FindBuddy(jcl, arg[1], &b, NULL, false))
			{
				if (b->btype == BT_CHATROOM)
					JCL_ForgetBuddy(jcl, b, NULL);
				else if (b->btype == BT_ROSTER)
				{
					//hide from em
					JCL_AddClientMessagef(jcl, "<presence to='%s' type='unsubscribed'/>", arg[1]);

					//stop looking for em
					JCL_AddClientMessagef(jcl, "<presence to='%s' type='unsubscribe'/>", arg[1]);

					//stop listing em
					JCL_SendIQf(jcl, NULL, "set", NULL, "<query xmlns='jabber:iq:roster'><item jid='%s' subscription='remove' /></query>", arg[1]);
				}
				else
					JCL_AddClientMessagef(jcl, "<presence to='%s' type='unavailable'/>", arg[1]);
			}
		}
#ifdef JINGLE
		else if (!strcmp(arg[0]+1, "join")) 
		{
			JCL_ToJID(jcl, *arg[1]?arg[1]:console, nname, sizeof(nname), true);
			JCL_Join(jcl, nname, NULL, true, ICEP_QWCLIENT);
		}
		else if (!strcmp(arg[0]+1, "invite")) 
		{
			JCL_ToJID(jcl, *arg[1]?arg[1]:console, nname, sizeof(nname), true);
			JCL_Join(jcl, nname, NULL, true, ICEP_QWSERVER);
		}
		else if (!strcmp(arg[0]+1, "voice") || !strcmp(arg[0]+1, "call")) 
		{
			JCL_ToJID(jcl, *arg[1]?arg[1]:console, nname, sizeof(nname), true);
			JCL_Join(jcl, nname, NULL, true, ICEP_VOICE);
		}
		else if (!strcmp(arg[0]+1, "kick")) 
		{
			JCL_ToJID(jcl, *arg[1]?arg[1]:console, nname, sizeof(nname), true);
			JCL_Join(jcl, nname, NULL, false, ICEP_INVALID);
		}
#endif
#ifdef FILETRANSFERS
		else if (!strcmp(arg[0]+1, "sendfile"))
		{
			char *fname = arg[1];
		
			if (!(jcl->enabledcapabilities & CAP_SIFT))
			{
				Con_SubPrintf(console, "XMPP: file transfers are not enabled for this account. Edit your config.\n");
				return;
			}

			if (!*fname || strchr(fname, '*'))
			{
				Con_SubPrintf(console, "XMPP: /sendfile FILENAME [to]\n");
				return;
			}
			else
			{
				JCL_ToJID(jcl, *arg[2]?arg[2]:console, nname, sizeof(nname), true);

				XMPP_FT_SendFile(jcl, console, nname, fname);
			}
		}
#endif
		else if (!strcmp(arg[0]+1, "joinchatroom") || !strcmp(arg[0]+1, "muc") || !strcmp(arg[0]+1, "joinmuc")) 
		{
			if (!*arg[1])
				Con_SubPrintf(console, "xmpp %s room@server roomnick password\n");
			else
				JCL_JoinMUCChat(jcl, arg[1], NULL, *arg[2]?arg[2]:NULL, arg[3]);
		}
		else if (!strcmp(arg[0]+1, "leavechatroom") || !strcmp(arg[0]+1, "leavemuc")) 
		{
			char roomserverhandle[512];
			buddy_t *b;
			bresource_t *r;
			if (!Q_snprintfz(roomserverhandle, sizeof(roomserverhandle), "%s@%s/%s", arg[1], arg[2], arg[3]) && JCL_FindBuddy(jcl, roomserverhandle, &b, &r, false))
			{
				JCL_AddClientMessagef(jcl, "<presence to='%s' type='unavailable'/>", roomserverhandle);
				JCL_ForgetBuddy(jcl, b, NULL);
			}
		}
		else if (!strcmp(arg[0]+1, "slap")) 
		{
			char *msgtab[] =
			{
				"/me slaps you around a bit with a large trout",
				"/me slaps you around a bit with a large tunafish",
				"/me slaps you around a bit with a slimy hagfish",
				"/me slaps a large trout around a bit with your face",
				"/me gets eaten by a rabid shark while trying to slap you with it",
				"/me gets crushed under the weight of a large whale",
				"/me searches for a fresh fish, but gets crabs instead",
				"/me searches for a fish, but there are no more fish in the sea",
				"/me tickles you around a bit with a large fish finger",
				"/me goes to order cod and chips. brb",
				"/me goes to watch some monty python"
			};
			JCL_ToJID(jcl, *arg[1]?arg[1]:console, nname, sizeof(nname), true);
			JCL_AttentionMessage(jcl, nname, msgtab[rand()%(sizeof(msgtab)/sizeof(msgtab[0]))]);
		}
		else if (!strcmp(arg[0]+1, "poke")) 
		{
			JCL_ToJID(jcl, *arg[1]?arg[1]:console, nname, sizeof(nname), true);
			JCL_AttentionMessage(jcl, nname, NULL);
		}
		else if (!strcmp(arg[0]+1, "raw")) 
		{
			//parse it first, so noone ever generates invalid xml and gets kicked... too obviously.
			int pos = 0;
			int maxpos = strlen(arg[1]);
			xmltree_t *t;
			while (pos != maxpos)
			{
				t = XML_Parse(arg[1], &pos, maxpos, false, "");
				if (t)
					XML_Destroy(t);
				else
					break;
			}
			if (pos == maxpos)
			{
				jcl->streamdebug = true;
				JCL_AddClientMessageString(jcl, arg[1]);
			}
			else
				Con_Printf("XML not well formed\n");
		}
		else
			Con_SubPrintf(console, "Unrecognised command: %s\n", arg[0]);
	}
	else
	{
		msg = imsg;
		if (jcl && jcl->status == JCL_ACTIVE)
		{
			if (!*msg)
			{
				if (!*console)
				{
					if (confuncs)
						JCL_RegenerateBuddyList(true);
					else
						JCL_PrintBuddyList(console, jcl, false);
					//Con_TrySubPrint(console, "For help, type \"^[/" COMMANDPREFIX " /help^]\"\n");
				}
			}
			else
			{
				JCL_ToJID(jcl, *console?console:jcl->defaultdest, nname, sizeof(nname), true);
				JCL_SendMessage(jcl, nname, msg);
			}
		}
		else
		{
			if (!*msg && confuncs)
				JCL_RegenerateBuddyList(true);
			else
				Con_TrySubPrint(console, "Not connected. For help, type \"^[/" COMMANDPREFIX " /help^]\"\n");
		}
	}
}
