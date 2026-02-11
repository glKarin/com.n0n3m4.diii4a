#ifndef FTEPLUGIN
#include "quakedef.h"
#define FTEENGINE	//we're getting statically linked. lucky us.
#define FTEPLUGIN
#define Plug_Init Plug_OpenSSL_Init

#define Q_strlcpy Q_strncpyz
#endif

#include "plugin.h"
#include "netinc.h"

static plugfsfuncs_t *fsfuncs;
static plugnetfuncs_t *netfuncs;

static cvar_t *pdtls_psk_hint, *pdtls_psk_user, *pdtls_psk_key;

#undef SHA1
#undef HMAC
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/conf.h"

#define assert(c) do{if (!(c)) Con_Printf("assert failed: "STRINGIFY(c)"\n");}while(0)

static qboolean OSSL_Init(void);
static int ossl_fte_certctx;
struct fte_certctx_s
{
	const char *peername;
	qboolean dtls;
	qboolean failure;

	hashfunc_t *hash;	//if set peer's cert MUST match the specified digest (with this hash function)
	qbyte digest[DIGEST_MAXSIZE];
};

static struct
{
	X509 *servercert;
	EVP_PKEY *privatekey;
} vhost;

static BIO_METHOD *biometh_vfs;

static int OSSL_Bio_FWrite(BIO *h, const char *buf, int size)
{
	vfsfile_t *f = BIO_get_data(h);
	int r = VFS_WRITE(f, buf, size);
	BIO_clear_retry_flags(h);
	if (r == 0)
	{
		BIO_set_retry_write(h);
		r = -1; //paranoia
	}
//	else if (r < 0)
//		Con_DPrintf("ossl Error: %i\n", r);
	return r;
}
static int OSSL_Bio_FRead(BIO *h, char *buf, int size)
{
	vfsfile_t *f = BIO_get_data(h);
	int r = VFS_READ(f, buf, size);
	BIO_clear_retry_flags(h);
	if (r == 0)	//no data yet.
	{
		BIO_set_retry_read(h);
		r = -1;	//shouldn't be needed, but I'm paranoid
	}
	return r;
}
static int OSSL_Bio_FPuts(BIO *h, const char *buf)
{
	return OSSL_Bio_FWrite(h, buf, strlen(buf));
}
static long OSSL_Bio_FCtrl(BIO *h, int cmd, long arg1, void *arg2)
{
	vfsfile_t *f = BIO_get_data(h);
	switch(cmd)
	{
	case BIO_CTRL_FLUSH:
		VFS_FLUSH(f);
		return 1;
	default:
//		Con_Printf("OSSL_Bio_FCtrl: unknown cmd %i\n", cmd);
	case BIO_CTRL_PUSH:
	case BIO_CTRL_POP:
		return 0;
	}
	return 0;	//failure
}
static long OSSL_Bio_FOtherCtrl(BIO *h, int cmd, BIO_info_cb *cb)
{
	switch(cmd)
	{
	default:
//		Con_Printf("OSSL_Bio_FOtherCtrl unknown cmd %i\n", cmd);
		return 0;
	}
	return 0;	//failure
}
static int OSSL_Bio_FCreate(BIO *h)
{	//we'll have to fill this in after we create the bio.
	BIO_set_data(h, NULL);
	return 1;
}
static int OSSL_Bio_FDestroy(BIO *h)
{
	vfsfile_t *f = BIO_get_data(h);
	VFS_CLOSE(f);
	BIO_set_data(h, NULL);
	return 1;
}

static int OSSL_PrintError_CB (const char *str, size_t len, void *u)
{
	Con_Printf("%s\n", str);
	return 1;
}

typedef struct
{
	vfsfile_t funcs;
	struct fte_certctx_s cert;
	SSL_CTX *ctx;
	BIO *bio;
	SSL *ssl;
} osslvfs_t;
static int QDECL OSSL_FRead (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	osslvfs_t *o = (osslvfs_t*)file;
	int r = BIO_read(o->bio, buffer, bytestoread);
	if (r <= 0)
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return -1;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return -1;	//should never happen
			}
		}
		if (BIO_should_retry(o->bio))
			return 0;
		return -1;	//eof or something
	}
	return r;
}
static int QDECL OSSL_FWrite (struct vfsfile_s *file, const void *buffer, int bytestowrite)
{
	osslvfs_t *o = (osslvfs_t*)file;
	int r = BIO_write(o->bio, buffer, bytestowrite);
	if (r <= 0)
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return -1;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return -1;	//should never happen
			}
		}
		if (BIO_should_retry(o->bio))
			return 0;
		return -1;	//eof or something
	}
	return r;
}
//static qboolean QDECL OSSL_Seek (struct vfsfile_s *file, qofs_t pos);	//returns false for error
//static qofs_t QDECL OSSL_Tell (struct vfsfile_s *file);
//static qofs_t QDECL OSSL_GetLen (struct vfsfile_s *file);	//could give some lag
static qboolean QDECL OSSL_Close (struct vfsfile_s *file)
{
	osslvfs_t *o = (osslvfs_t*)file;
	BIO_free(o->bio);
	SSL_CTX_free(o->ctx);
	free(o);
	return true;	//success, I guess
}
//static void QDECL OSSL_Flush (struct vfsfile_s *file);



static qboolean print_cn_name(X509_NAME* const name, const char *utf8match, const char *prefix)
{
    int idx = -1;
	qboolean success = 0;
    unsigned char *utf8 = NULL;
	X509_NAME_ENTRY* entry;
	ASN1_STRING* data;
	int length;

    do
    {
        if(!name) break; /* failed */

        idx = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
        if(!(idx > -1))  break; /* failed */

        entry = X509_NAME_get_entry(name, idx);
        if(!entry) break; /* failed */

        data = X509_NAME_ENTRY_get_data(entry);
        if(!data) break; /* failed */

        length = ASN1_STRING_to_UTF8(&utf8, data);
        if(!utf8 || !(length > 0))  break; /* failed */

		if (utf8match)
			success = !strcmp(utf8, utf8match);
		else
		{
			Con_Printf("%s%s", prefix, utf8);
			success = 1;
		}

    } while (0);

    if(utf8)
        OPENSSL_free(utf8);

	return success;
}

static int OSSL_Verify_Peer(int preverify_ok, X509_STORE_CTX *x509_ctx)
{
	SSL *ssl = X509_STORE_CTX_get_ex_data(x509_ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	struct fte_certctx_s *uctx = SSL_get_ex_data(ssl, ossl_fte_certctx);

	if (uctx->hash)
	{	//our special 'must-match-digest' mode without any other kind of trust.
		X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);
		size_t blobsize;
		qbyte *blob;
		qbyte *end;
		qbyte digest[DIGEST_MAXSIZE];
		void *hctx = alloca(uctx->hash->contextsize);
		blobsize = i2d_X509(cert, NULL);
		blob = alloca(blobsize);
		end = blob;
		i2d_X509(cert, &end);

		uctx->hash->init(hctx);
		uctx->hash->process(hctx, blob, blobsize);
		uctx->hash->terminate(digest, hctx);

		//return 1 for success
		if (memcmp(digest, uctx->digest, uctx->hash->digestsize))
		{
			uctx->failure = true;
			return 0;
		}
		return 1;
	}

	if(preverify_ok == 0)
	{
		int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
		int err = X509_STORE_CTX_get_error(x509_ctx);

		X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);
		X509_NAME* iname = cert ? X509_get_issuer_name(cert) : NULL;
		//X509_NAME* sname = cert ? X509_get_subject_name(cert) : NULL;

		if (err == X509_V_ERR_CERT_HAS_EXPIRED || err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
		{
			size_t knownsize;
			qbyte *knowndata = netfuncs->TLS_GetKnownCertificate(uctx->peername, &knownsize);
			if (knowndata)
			{	//check
				size_t blobsize;
				qbyte *blob;
				qbyte *end;
				blobsize = i2d_X509(cert, NULL);
				if (blobsize == knownsize)
				{	//sizes must match.
					blob = alloca(blobsize);
					end = blob;
					i2d_X509(cert, &end);
					if (!memcmp(blob, knowndata, blobsize))
						preverify_ok = 1;	//exact match to a known cert. yay. allow it.
				}
				plugfuncs->Free(knowndata);
				return preverify_ok;
			}

#ifdef HAVE_CLIENT
			if (uctx->dtls)
			{
				unsigned int probs = 0;
				size_t blobsize;
				qbyte *blob;
				qbyte *end;
				blobsize = i2d_X509(cert, NULL);
				blob = alloca(blobsize);
				end = blob;
				i2d_X509(cert, &end);

				switch(err)
				{
				case 0:
					probs |= CERTLOG_WRONGHOST;
					break;
				case X509_V_ERR_CERT_HAS_EXPIRED:
					probs |= CERTLOG_EXPIRED;
					break;
				case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
					probs |= CERTLOG_MISSINGCA;
					break;
				default:
					probs |= CERTLOG_UNKNOWN;
					break;
				}
				if (netfuncs->CertLog_ConnectOkay && netfuncs->CertLog_ConnectOkay(uctx->peername, blob, blobsize, probs))
					return 1; //ignore the errors...
				return 0;	//allow it.
			}
#endif
		}

		Con_Printf(CON_ERROR"%s ", uctx->peername);
		//FIXME: this is probably on a worker thread. expect munged prints.
		//print_cn_name(sname, NULL, CON_ERROR);
		Con_Printf(" (issued by ");
		print_cn_name(iname, NULL, S_COLOR_YELLOW);
		Con_Printf(")");
		if(depth == 0) {
			/* If depth is 0, its the server's certificate. Print the SANs too */
	//		print_san_name("Subject (san)", cert);
		}

		if(err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
			Con_Printf(":  Error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY\n");
		else if (err == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)
			Con_Printf(":  Error = X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE\n");
		else if(err == X509_V_ERR_CERT_UNTRUSTED)
			Con_Printf(":  Error = X509_V_ERR_CERT_UNTRUSTED\n");
		else if(err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
			Con_Printf(":  Error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN\n");
		else if(err == X509_V_ERR_CERT_NOT_YET_VALID)
			Con_Printf(":  Error = X509_V_ERR_CERT_NOT_YET_VALID\n");
		else if(err == X509_V_ERR_CERT_HAS_EXPIRED)
			Con_Printf(":  Error = X509_V_ERR_CERT_HAS_EXPIRED\n");
		else if(err == X509_V_OK)
			Con_Printf(":  Error = X509_V_OK\n");
		else if(err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
			Con_Printf(":  Error = X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT\n");
		else
			Con_Printf(":  Error = %d\n", err);
	}

	if (!preverify_ok && cvarfuncs->GetFloat("tls_ignorecertificateerrors"))
	{
		Con_Printf(CON_ERROR "%s: Ignoring certificate errors (tls_ignorecertificateerrors is set)\n", uctx->peername);
		return 1;
	}

	return preverify_ok;
}


static vfsfile_t *OSSL_OpenPrivKeyFile(char *nativename, size_t nativesize)
{
#define privname "privkey.pem"
	vfsfile_t *privf;
	const char *mode = nativename?"wb":"rb";
	/*int i = COM_CheckParm("-privkey");
	if (i++)
	{
		if (nativename)
			Q_strncpyz(nativename, com_argv[i], nativesize);
		privf = FS_OpenVFS(com_argv[i], mode, FS_SYSTEM);
	}
	else*/
	{
		if (nativename)
			if (!fsfuncs->NativePath(privname, FS_ROOT, nativename, nativesize))
				return NULL;

		privf = fsfuncs->OpenVFS(privname, mode, FS_ROOT);
	}
	return privf;
#undef privname
}
static vfsfile_t *OSSL_OpenPubKeyFile(char *nativename, size_t nativesize)
{
#define fullchainname "fullchain.pem"
#define pubname "cert.pem"
	vfsfile_t *pubf = NULL;
	const char *mode = nativename?"wb":"rb";
	/*int i = COM_CheckParm("-pubkey");
	if (i++)
	{
		if (nativename)
			Q_strncpyz(nativename, com_argv[i], nativesize);
		pubf = FS_OpenVFS(com_argv[i], mode, FS_SYSTEM);
	}
	else*/
	{
		if (!pubf && (!nativename || fsfuncs->NativePath(fullchainname, FS_ROOT, nativename, nativesize)))
			pubf = fsfuncs->OpenVFS(fullchainname, mode, FS_ROOT);
		if (!pubf && (!nativename || fsfuncs->NativePath(pubname, FS_ROOT, nativename, nativesize)))
			pubf = fsfuncs->OpenVFS(pubname, mode, FS_ROOT);
	}
	return pubf;
#undef pubname
}
static BIO *OSSL_BioFromFile(vfsfile_t *f)
{
	qbyte buf[4096];
	int r;
	BIO *b = BIO_new(BIO_s_mem());
	if (f)
	{
		for(;;)
		{
			r = VFS_READ(f, buf, sizeof(buf));
			if (r <= 0)
				break;
			BIO_write(b, buf, r);
		}
		VFS_CLOSE(f);
	}

	return b;
}
static void OSSL_OpenPrivKey(void)
{
	BIO *bio = OSSL_BioFromFile(OSSL_OpenPrivKeyFile(NULL,0));
	vhost.privatekey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
	BIO_free(bio);
}
static void OSSL_OpenPubKey(void)
{
	BIO *bio = OSSL_BioFromFile(OSSL_OpenPubKeyFile(NULL,0));
	vhost.servercert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);
}

static char *OSSL_SetCertificateName(char *out, const char *hostname)
{	//glorified strcpy...
	int i;
	if (hostname)
	{
		const char *host = strstr(hostname, "://");
		if (host)
			hostname = host+3;
		//any dtls:// prefix will have been stripped now.
		if (*hostname == '[')
		{	//eg: [::1]:foo - skip the lead [ and strip the ] and any trailing data (hopefully just a :port or nothing)
			hostname++;
			host = strchr(hostname, ']');
			if (host)
			{
				memcpy(out, hostname, host-hostname);
				out[host-hostname] = 0;
				hostname = out;
			}
		}
		else
		{	//eg: 127.0.0.1:port - strip the port number if specified.
			host = strchr(hostname, ':');
			if (host)
			{
				memcpy(out, hostname, host-hostname);
				out[host-hostname] = 0;
				hostname = out;
			}
		}
		for (i = 0; hostname[i]; i++)
		{
			if (hostname[i] >= 'a' && hostname[i] <= 'z')
				;
			else if (hostname[i] >= 'A' && hostname[i] <= 'Z')
				;
			else if (hostname[i] >= '0' && hostname[i] <= '9')
				;
			else if (hostname[i] == '-' || hostname[i] == '.')
				;
			else
			{
				hostname = NULL;	//something invalid. bum.
				break;
			}
		}
		//we should have a cleaned up host name now, ready for (ab)use in certificates.
	}

	if (!hostname)
		*out = 0;
	else if (hostname == out)
		;
	else
		memcpy(out, hostname, strlen(hostname)+1);

	return out;
}

static vfsfile_t *OSSL_OpenVFS(const char *hostname, vfsfile_t *source, qboolean isserver)
{
	BIO *sink;
	osslvfs_t *n;
	if (!OSSL_Init())
		return NULL;	//FAIL!

	if (!hostname)
		hostname = "";

	n = calloc(sizeof(*n) + strlen(hostname)+1, 1);

	n->funcs.ReadBytes = OSSL_FRead;
	n->funcs.WriteBytes = OSSL_FWrite;
	n->funcs.Seek = NULL;
	n->funcs.Tell = NULL;
	n->funcs.GetLen = NULL;
	n->funcs.Close = OSSL_Close;
	n->funcs.Flush = NULL;
	n->funcs.seekstyle = SS_UNSEEKABLE;

	n->cert.peername = OSSL_SetCertificateName((char*)(n+1), hostname);
	n->cert.dtls = false;

	ERR_print_errors_cb(OSSL_PrintError_CB, NULL);

	sink = BIO_new(biometh_vfs);
	if (sink)
	{
		n->ctx = SSL_CTX_new(isserver?TLS_server_method():TLS_client_method());
		if (n->ctx)
		{
			assert(1==SSL_CTX_set_cipher_list(n->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"));

			SSL_CTX_set_session_cache_mode(n->ctx, SSL_SESS_CACHE_OFF);

			SSL_CTX_set_default_verify_paths(n->ctx);
			SSL_CTX_set_verify(n->ctx, SSL_VERIFY_PEER, OSSL_Verify_Peer);
			SSL_CTX_set_verify_depth(n->ctx, 5);
			SSL_CTX_set_options(n->ctx, SSL_OP_NO_COMPRESSION);	//compression allows guessing the contents of the stream somehow.

			if (isserver)
			{
				if (vhost.servercert && vhost.privatekey)
				{
					SSL_CTX_use_certificate(n->ctx, vhost.servercert);
					SSL_CTX_use_PrivateKey(n->ctx, vhost.privatekey);
					assert(1==SSL_CTX_check_private_key(n->ctx));
				}
			}

			n->bio = BIO_new_ssl(n->ctx, !isserver);

			//set up the source/sink
			BIO_set_data(sink, source);	//source now belongs to the bio
			BIO_set_init(sink, true);	//our sink is now ready...
			n->bio = BIO_push(n->bio, sink);
			BIO_free(sink);
			sink = NULL;

			BIO_get_ssl(n->bio, &n->ssl);
			SSL_set_ex_data(n->ssl, ossl_fte_certctx, &n->cert);
			SSL_set_mode(n->ssl, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
			SSL_set_tlsext_host_name(n->ssl, n->cert.peername);	//let the server know which cert to send
			BIO_do_connect(n->bio);
			return &n->funcs;
		}
		BIO_free(sink);
	}
	return NULL;
}
/*static int OSSL_GetChannelBinding(vfsfile_t *vf, qbyte *binddata, size_t *bindsize)
{
	//FIXME: not yet supported. tbh I've no idea how to get that data. probably something convoluted.
	return -1;
}*/



static BIO_METHOD *biometh_dtls;
typedef struct {
	struct fte_certctx_s cert;

	void *cbctx;
	neterr_t(*push)(void *cbctx, const qbyte *data, size_t datasize);

	SSL_CTX *ctx;
	BIO *bio;
	SSL *ssl;

//	BIO *sink;
	qbyte *pending;
	size_t pendingsize;

	void	*peeraddr;
	size_t	peeraddrsize;
} ossldtls_t;
static int OSSL_Bio_DWrite(BIO *h, const char *buf, int size)
{
	ossldtls_t *f = BIO_get_data(h);
	neterr_t r = f->push(f->cbctx, buf, size);

	BIO_clear_retry_flags(h);
	switch(r)
	{
	case NETERR_SENT:
		return size;
	case NETERR_NOROUTE:
	case NETERR_DISCONNECTED:
		return -1;
	case NETERR_MTU:
		return -1;
	case NETERR_CLOGGED:
		BIO_set_retry_write(h);
		return -1;
	}
	return r;
}
static int OSSL_Bio_DRead(BIO *h, char *buf, int size)
{
	ossldtls_t *f = BIO_get_data(h);

	BIO_clear_retry_flags(h);
	if (f->pending)
	{
		size = min(size, f->pendingsize);
		memcpy(buf, f->pending, f->pendingsize);

		//we've read it now, don't read it again.
		f->pending = 0;
		f->pendingsize = 0;
		return size;
	}
	//nothing available.
	BIO_set_retry_read(h);
	return -1;
}
static long OSSL_Bio_DCtrl(BIO *h, int cmd, long arg1, void *arg2)
{
//	ossldtls_t *f = BIO_get_data(h);
	switch(cmd)
	{
	case BIO_CTRL_FLUSH:
		return 1;


	case BIO_CTRL_DGRAM_GET_PEER:
		return 0;

	case BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT:	//we're non-blocking, so this doesn't affect us.
	case BIO_CTRL_DGRAM_GET_MTU_OVERHEAD:
	case BIO_CTRL_WPENDING:
	case BIO_CTRL_DGRAM_QUERY_MTU:
	case BIO_CTRL_DGRAM_SET_MTU:
	case BIO_CTRL_DGRAM_GET_FALLBACK_MTU:
		return 0;


	default:
//		Con_Printf("OSSL_Bio_DCtrl: unknown cmd %i\n", cmd);
	case BIO_CTRL_PUSH:
	case BIO_CTRL_POP:
		return 0;
	}
	return 0;	//failure
}
static long OSSL_Bio_DOtherCtrl(BIO *h, int cmd, BIO_info_cb *cb)
{
	switch(cmd)
	{
	default:
//		Con_Printf("OSSL_Bio_DOtherCtrl unknown cmd %i\n", cmd);
		return 0;
	}
	return 0;	//failure
}
static int OSSL_Bio_DCreate(BIO *h)
{	//we'll have to fill this in after we create the bio.
	BIO_set_data(h, NULL);
	return 1;
}
static int OSSL_Bio_DDestroy(BIO *h)
{
	BIO_set_data(h, NULL);
	return 1;
}

static int dehex(int i)
{
	if      (i >= '0' && i <= '9')
		return (i-'0');
	else if (i >= 'A' && i <= 'F')
		return (i-'A'+10);
	else
		return (i-'a'+10);
}
static size_t Base16_DecodeBlock_(const char *in, qbyte *out, size_t outsize)
{
	qbyte *start = out;
	if (!out)
		return ((strlen(in)+1)/2) + 1;

	for (; ishexcode(in[0]) && ishexcode(in[1]) && outsize > 0; outsize--, in+=2)
		*out++ = (dehex(in[0])<<4) | dehex(in[1]);
	return out-start;
}

static unsigned int OSSL_SV_Validate_PSK(SSL *ssl, const char *identity, unsigned char *psk, unsigned int max_psk_len)
{
	if (!strcmp(identity, pdtls_psk_user->string))
	{	//Yay! We know this one!
		return Base16_DecodeBlock_(pdtls_psk_key->string, psk, max_psk_len);
	}
	return 0;	//0 for error, or something.
}
unsigned int OSSL_CL_Validate_PSK(SSL *ssl, const char *hint, char *identity, unsigned int max_identity_len, unsigned char *psk, unsigned int max_psk_len)
{	//if our hint cvar matches, then report our user+key cvars to the server
	if ((!*hint && *pdtls_psk_user->string && !*pdtls_psk_hint->string) || (*hint && !strcmp(hint, pdtls_psk_hint->string)))
	{
#ifndef NOLEGACY
		if (*hint)
		{
			//Try to avoid crashing QE servers by recognising its hint and blocking it when the hashes of the user+key are wrong.
			quint32_t digest[SHA_DIGEST_LENGTH/4];

			SHA1(hint, strlen(hint), (qbyte*)digest);
			if ((digest[0]^digest[1]^digest[2]^digest[3]^digest[4]) == 0xb6c27b61)
			{
				SHA1(pdtls_psk_key->string, strlen(pdtls_psk_key->string), (qbyte*)digest);
				if (strcmp(hint, pdtls_psk_user->string) || (digest[0]^digest[1]^digest[2]^digest[3]^digest[4]) != 0x3dd348e4)
				{
					Con_Printf(CON_WARNING	"Possible QEx Server, please set your ^[%s\\type\\%s^] and ^[%s\\type\\%s^] cvars correctly, their current values are likely to crash the server.\n", pdtls_psk_user->name,pdtls_psk_user->name, pdtls_psk_key->name,pdtls_psk_key->name);
					return 0; //don't report anything.
				}
			}
		}
#endif

		Q_strlcpy(identity, pdtls_psk_user->string, max_identity_len);
		return Base16_DecodeBlock_(pdtls_psk_key->string, psk, max_psk_len);
	}
	else if (*hint)
		Con_Printf(CON_WARNING	"Unable to supply PSK response to server (hint is \"%s\").\n"
								"Please set ^[%s\\type\\%s^], ^[%s\\type\\%s^], and ^[%s\\type\\%s^] cvars to match the server.\n", hint, pdtls_psk_hint->name,pdtls_psk_hint->name, pdtls_psk_user->name,pdtls_psk_user->name, pdtls_psk_key->name,pdtls_psk_key->name);
	return 0;	//we don't know what to report.
}

static void *OSSL_CreateContext(const dtlscred_t *cred, void *cbctx, neterr_t(*push)(void *cbctx, const qbyte *data, size_t datasize), qboolean isserver)
{	//if remotehost is null then their certificate will not be validated.
	ossldtls_t *n;
	BIO *sink;
	const char *remotehost = cred?cred->peer.name:NULL;

	if (!remotehost)
		remotehost = "";
	n = calloc(sizeof(*n) + strlen(remotehost)+1, 1);

	n->cbctx = cbctx;
	n->push = push;

	n->ctx = SSL_CTX_new(isserver?DTLS_server_method():DTLS_client_method());

	n->cert.peername = OSSL_SetCertificateName((char*)(n+1), remotehost);
	n->cert.dtls = true;
	if (cred)
	{
		n->cert.hash = cred->peer.hash;
		memcpy(n->cert.digest, cred->peer.digest, sizeof(cred->peer.digest));
	}
	else
	{
		n->cert.hash = NULL;
		memset(n->cert.digest, 0, sizeof(n->cert.digest));
	}

	if (n->ctx)
	{
		assert(1==SSL_CTX_set_cipher_list(n->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"));

		SSL_CTX_set_session_cache_mode(n->ctx, SSL_SESS_CACHE_OFF);

		SSL_CTX_set_verify(n->ctx, SSL_VERIFY_PEER|(n->cert.hash?SSL_VERIFY_FAIL_IF_NO_PEER_CERT:0), OSSL_Verify_Peer);
		SSL_CTX_set_verify_depth(n->ctx, 10);
		SSL_CTX_set_options(n->ctx, SSL_OP_NO_COMPRESSION|	//compression allows guessing the contents of the stream somehow.
									SSL_OP_NO_RENEGOTIATION);

		if (cred && (cred->local.certsize||cred->local.keysize))
		{
			X509 *cert = NULL;
			EVP_PKEY *key = NULL;
			const unsigned char *ffs;
			ffs = cred->local.cert;
			d2i_X509(&cert, &ffs, cred->local.certsize);
			SSL_CTX_use_certificate(n->ctx, cert);

			ffs = cred->local.key;
			d2i_PrivateKey(EVP_PKEY_RSA, &key, &ffs, cred->local.keysize);
			SSL_CTX_use_PrivateKey(n->ctx, key);
		}
		else if (isserver)
		{
			if (*pdtls_psk_user->string)
			{
				if (*pdtls_psk_user->string)
					SSL_CTX_use_psk_identity_hint(n->ctx, pdtls_psk_hint->string);
				SSL_CTX_set_psk_server_callback(n->ctx, OSSL_SV_Validate_PSK);
			}

			if (vhost.servercert && vhost.privatekey)
			{
				SSL_CTX_use_certificate(n->ctx, vhost.servercert);
				SSL_CTX_use_PrivateKey(n->ctx, vhost.privatekey);
				assert(1==SSL_CTX_check_private_key(n->ctx));
			}
		}
		else
		{
//			if (*pdtls_psk_user->string)
				SSL_CTX_set_psk_client_callback(n->ctx, OSSL_CL_Validate_PSK);
		}

		//SSL_CTX_use_certificate_file
		//FIXME: SSL_CTX_use_certificate_file aka SSL_CTX_use_certificate(PEM_read_bio_X509)
		//FIXME: SSL_CTX_use_PrivateKey_file aka SSL_CTX_use_PrivateKey(PEM_read_bio_PrivateKey)
		//assert(1==SSL_CTX_check_private_key(n->ctx));

		{
			n->bio = BIO_new_ssl(n->ctx, !isserver);

			//set up the source/sink
			sink = BIO_new(biometh_dtls);
			if (sink)
			{
				BIO_set_data(sink, n);
				BIO_set_init(sink, true);	//our sink is now ready...
				n->bio = BIO_push(n->bio, sink);
				BIO_free(sink);
				sink = NULL;
			}

			BIO_get_ssl(n->bio, &n->ssl);
			SSL_set_app_data(n->ssl, n);
			SSL_set_ex_data(n->ssl, ossl_fte_certctx, &n->cert);
			if (*n->cert.peername)
				SSL_set_tlsext_host_name(n->ssl, n->cert.peername);	//let the server know which cert to send
			BIO_do_connect(n->bio);
			ERR_print_errors_cb(OSSL_PrintError_CB, NULL);
			return n;
		}
	}

	return NULL;
}

static qbyte dtlscookiekey[16];
static int OSSL_GenCookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len)
{
	ossldtls_t *f = SSL_get_app_data(ssl);
	qbyte *blurgh = alloca(sizeof(dtlscookiekey) + f->peeraddrsize);

	memcpy(blurgh, dtlscookiekey, sizeof(dtlscookiekey));
	memcpy(blurgh+sizeof(dtlscookiekey), f->peeraddr, f->peeraddrsize);

	SHA1(blurgh, sizeof(dtlscookiekey) + f->peeraddrsize, cookie);
	*cookie_len = SHA_DIGEST_LENGTH;

	return 1;
}
static int OSSL_VerifyCookie(SSL *ssl, const unsigned char *cookie, unsigned int cookie_len)
{
	unsigned char match[DTLS1_COOKIE_LENGTH];
	unsigned int matchsize;
	if (OSSL_GenCookie(ssl, match, &matchsize))
		if (cookie_len == matchsize && !memcmp(cookie, match, matchsize))
			return 1;
	return 0;	//not valid.
}
qboolean OSSL_CheckConnection(void *cbctx, void *peeraddr, size_t peeraddrsize, void *indata, size_t insize, neterr_t(*push)(void *cbctx, const qbyte *data, size_t datasize), void (*EstablishTrueContext)(void **cbctx, void *state))
{
	int ret;
	static ossldtls_t *pending;
	BIO_ADDR *bioaddr = BIO_ADDR_new();

	if (!pending)
	{
		pending = OSSL_CreateContext(NULL, cbctx, push, true);

		SSL_CTX_set_cookie_generate_cb(pending->ctx, OSSL_GenCookie);
		SSL_CTX_set_cookie_verify_cb(pending->ctx, OSSL_VerifyCookie);
	}

	SSL_set_app_data(pending->ssl, pending);

	//make sure its kept current...
	pending->cbctx = cbctx;
	pending->push = push;

	pending->pending = indata;
	pending->pendingsize = insize;
	ret = DTLSv1_listen(pending->ssl, bioaddr);

	BIO_ADDR_free(bioaddr);

	if (ret >= 1)
	{
		pending->pending = NULL;
		pending->pendingsize = 0;

		EstablishTrueContext(&pending->cbctx, pending);
		pending = NULL;	//returned to called. next request gets a new one.
		return true;
	}
	//0 = nonfatal
	//-1 = fatal
	return false;
}
static void OSSL_DestroyContext(void *ctx)
{
	ossldtls_t *o = (ossldtls_t*)ctx;
	BIO_free(o->bio);
	SSL_CTX_free(o->ctx);
	free(o);
}
static neterr_t OSSL_Transmit(void *ctx, const qbyte *data, size_t datasize)
{	//we're sending data
	ossldtls_t *o = (ossldtls_t*)ctx;
	int r;
	if (datasize == 0)	//liveness test. return clogged while handshaking and sent when finished. openssl doesn't like 0-byte writes.
	{
		if (o->cert.failure)
			return NETERR_DISCONNECTED;	//actual security happens elsewhere.
		else if (SSL_is_init_finished(o->ssl))
			return NETERR_SENT;	//go on, send stuff.
		else if (BIO_should_retry(o->bio))
		{
			if (BIO_do_handshake(o->bio) == 1)
				return NETERR_SENT;
		}
		return NETERR_CLOGGED;	//can't send yet.
	}
	else
		r = BIO_write(o->bio, data, datasize);
	if (r <= 0)
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return NETERR_NOROUTE;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return NETERR_NOROUTE;	//should never happen
			}
		}
		if (BIO_should_retry(o->bio))
			return NETERR_CLOGGED;
		return NETERR_DISCONNECTED;	//eof or something
	}
	return NETERR_SENT;
}
static neterr_t OSSL_Received(void *ctx, sizebuf_t *message)
{	//we have received some encrypted data...
	ossldtls_t *o = (ossldtls_t*)ctx;
	int r;

	if (!message)
		r = BIO_read(o->bio, NULL, 0);
	else
	{
		o->pending = message->data;
		o->pendingsize = message->cursize;
		r = BIO_read(o->bio, message->data, message->maxsize);
		o->pending = NULL;
		o->pendingsize = 0;
	}

	if (r > 0)
	{
		message->cursize = r;
		return NETERR_SENT;
	}
	else
	{
		if (BIO_should_io_special(o->bio))
		{
			switch(BIO_get_retry_reason(o->bio))
			{
			//these are temporary errors, try again later.
			case BIO_RR_SSL_X509_LOOKUP:
				return NETERR_NOROUTE;	//certificate failure.
			case BIO_RR_ACCEPT:
			case BIO_RR_CONNECT:
				return NETERR_NOROUTE;	//should never happen
			}
		}
		if (BIO_should_retry(o->bio))
			return 0;
		return NETERR_DISCONNECTED;	//eof or something
	}
	return NETERR_NOROUTE;
}
static neterr_t OSSL_Timeouts(void *ctx)
{	//keep it ticking over, or something.
	return OSSL_Received(ctx, NULL);
}

static int OSSL_GetPeerCertificate(void *ctx, enum certprops_e prop, char *out, size_t outsize)
{
	ossldtls_t *o = (ossldtls_t*)ctx;
	X509 *cert;

	safeswitch(prop)
	{
	case QCERT_ISENCRYPTED:
		return 0;	//not an error.
	case QCERT_PEERCERTIFICATE:
		cert = SSL_get_peer_certificate(o->ssl);
		goto returncert;
	case QCERT_LOCALCERTIFICATE:
		cert = vhost.servercert;
		goto returncert;
returncert:
		if (cert)
		{
			size_t blobsize = i2d_X509(cert, NULL);
			qbyte *end = out;
			if (blobsize <= outsize)
				return i2d_X509(cert, &end);
			return -1;	//caller didn't pass a big enough buffer.
		}
		return -1;
	case QCERT_PEERSUBJECT:
		{
			int r;
			X509 *cert = SSL_get_peer_certificate(o->ssl);
			if (cert)
			{
				X509_NAME *iname = X509_get_subject_name(cert);
				BIO *bio = BIO_new(BIO_s_mem());
				X509_NAME_print_ex(bio, iname, 0, XN_FLAG_RFC2253);
				r = BIO_read(bio, out, outsize-1);
				out[(r<0)?0:r] = 0;
				BIO_free(bio);
				return r;
			}
		}
		return -1;
	safedefault:
	case QCERT_LOBBYSTATUS:		//for special-case lobby wrappers.
	case QCERT_LOBBYSENDCHAT:	//to send chat via the stupid lobby instead of the game itself.
		return -1;
	}
}
static qboolean OSSL_GenTempCertificate(const char *subject, struct dtlslocalcred_s *cred)
{
	qbyte *ffs;
#if OPENSSL_API_LEVEL >= 30000
	EVP_PKEY *pkey = EVP_RSA_gen(4096);
#else
	EVP_PKEY*pkey = EVP_PKEY_new();
	RSA		*rsa = RSA_new();
	BIGNUM	*pkexponent = BN_new();
	//The pseudo-random number generator must be seeded prior to calling RSA_generate_key_ex().
	BN_set_word(pkexponent, RSA_F4);
	RSA_generate_key_ex(rsa, 2048, pkexponent, NULL);
	BN_free(pkexponent);

	EVP_PKEY_assign_RSA(pkey, rsa);
#endif
	cred->keysize = i2d_PrivateKey(pkey, NULL);
	cred->key = ffs = plugfuncs->Malloc(cred->keysize);
	cred->keysize = i2d_PrivateKey(pkey, &ffs);

	{
		X509 *x509 = X509_new();
		ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
		X509_gmtime_adj(X509_get_notBefore(x509), 0);
		X509_gmtime_adj(X509_get_notAfter(x509), 365*24*60*60);	//lots of validity
		X509_set_pubkey(x509, pkey);

		{
			X509_NAME	*name = X509_get_subject_name(x509);
			X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (subject?subject:"localhost"), -1, -1, 0);
			X509_set_issuer_name(x509, name);
		}

		X509_sign(x509, pkey, EVP_sha1());

		cred->certsize = i2d_X509(x509, NULL);
		cred->cert = ffs = plugfuncs->Malloc(cred->certsize);
		cred->certsize = i2d_X509(x509, &ffs);

		X509_free(x509);
	}

	EVP_PKEY_free(pkey);	//also frees the rsa pointer.

	return true;
}

static dtlsfuncs_t ossl_dtlsfuncs =
{
	OSSL_CreateContext,
	OSSL_CheckConnection,
	OSSL_DestroyContext,
	OSSL_Transmit,
	OSSL_Received,
	OSSL_Timeouts,
	OSSL_GetPeerCertificate,
	OSSL_GenTempCertificate,
};
static const dtlsfuncs_t *OSSL_InitClient(void)
{
	if (OSSL_Init())
		return &ossl_dtlsfuncs;
	return NULL;
}
static const dtlsfuncs_t *OSSL_InitServer(void)
{
	if (OSSL_Init())
		return &ossl_dtlsfuncs;
	return NULL;
}










static struct
{
	qboolean inited;
	qboolean init_success;
} ossl;
static qboolean OSSL_Init(void)
{
	if (ossl.inited)
		return ossl.init_success;
	ossl.inited = true;
#if 0//def LOADERTHREAD
	Sys_LockMutex(com_resourcemutex);
	if (inited)	//now check again, just in case
	{
		Sys_UnlockMutex(com_resourcemutex);
		return init_success;
	}
#endif

	SSL_library_init();
    SSL_load_error_strings();
//	OPENSSL_config(NULL);
	ERR_print_errors_cb(OSSL_PrintError_CB, NULL);

	OSSL_OpenPubKey();
	OSSL_OpenPrivKey();

	biometh_vfs = BIO_meth_new(BIO_get_new_index()|BIO_TYPE_SOURCE_SINK|BIO_TYPE_DESCRIPTOR, "fte_vfs");
	if (biometh_vfs)
	{
		BIO_meth_set_write(biometh_vfs, OSSL_Bio_FWrite);
		BIO_meth_set_read(biometh_vfs, OSSL_Bio_FRead);
		BIO_meth_set_puts(biometh_vfs, OSSL_Bio_FPuts);	//I cannot see how gets/puts can work with dtls...
//		BIO_meth_set_gets(biometh_vfs, OSSL_Bio_FGets);
		BIO_meth_set_ctrl(biometh_vfs, OSSL_Bio_FCtrl);
		BIO_meth_set_create(biometh_vfs, OSSL_Bio_FCreate);
		BIO_meth_set_destroy(biometh_vfs, OSSL_Bio_FDestroy);
		BIO_meth_set_callback_ctrl(biometh_vfs, OSSL_Bio_FOtherCtrl);
		ossl.init_success |= 1;
	}

	biometh_dtls = BIO_meth_new(BIO_get_new_index()|BIO_TYPE_SOURCE_SINK|BIO_TYPE_DESCRIPTOR, "fte_dtls");
	if (biometh_dtls)
	{
		BIO_meth_set_write(biometh_dtls, OSSL_Bio_DWrite);
		BIO_meth_set_read(biometh_dtls, OSSL_Bio_DRead);
//		BIO_meth_set_puts(biometh_dtls, OSSL_Bio_DPuts);	//I cannot see how gets/puts can work with dtls...
//		BIO_meth_set_gets(biometh_dtls, OSSL_Bio_DGets);
		BIO_meth_set_ctrl(biometh_dtls, OSSL_Bio_DCtrl);
		BIO_meth_set_create(biometh_dtls, OSSL_Bio_DCreate);
		BIO_meth_set_destroy(biometh_dtls, OSSL_Bio_DDestroy);
		BIO_meth_set_callback_ctrl(biometh_dtls, OSSL_Bio_DOtherCtrl);
		ossl.init_success |= 2;
	}

	ossl_fte_certctx = SSL_get_ex_new_index(0, "ossl_fte_certctx", NULL, NULL, NULL);

#if 0//def LOADERTHREAD
	Sys_UnlockMutex(com_resourcemutex);
#endif
	return ossl.init_success;
}

static enum hashvalidation_e OSSL_VerifyHash(const qbyte *hashdata, size_t hashsize, const qbyte *pubkeydata, size_t pubkeysize, const qbyte *signdata, size_t signsize)
{	//accepts either DER or PEM certs
	int result = VH_UNSUPPORTED;
	BIO *bio_pubkey = BIO_new_mem_buf(pubkeydata, pubkeysize);
	if (bio_pubkey)
	{
		X509 *x509_pubkey = PEM_read_bio_X509(bio_pubkey, NULL, NULL, NULL);
		if (x509_pubkey)
		{
			EVP_PKEY *evp_pubkey = X509_get_pubkey(x509_pubkey);
			if (evp_pubkey)
			{
				EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(evp_pubkey, NULL);
				if (ctx)
				{
					EVP_PKEY_verify_init(ctx);
					EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
					EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha512());
					if (1 == EVP_PKEY_verify(ctx, signdata, signsize, hashdata, hashsize))
						result = VH_CORRECT;
					else
						result = VH_INCORRECT;
					EVP_PKEY_CTX_free(ctx);
				}
				EVP_PKEY_free(evp_pubkey);
			}
			X509_free(x509_pubkey);
		}
		BIO_free(bio_pubkey);
	}
	return result;
}

static ftecrypto_t crypto_openssl =
{
	"OpenSSL",
	OSSL_OpenVFS,
	NULL,//OSSL_GetChannelBinding,
	OSSL_InitClient,
	OSSL_InitServer,
	OSSL_VerifyHash,
	NULL,
};

static void OSSL_PluginShutdown(void)
{
	ossl.inited = false;
	ossl.init_success = false;

	X509_free(vhost.servercert);
	EVP_PKEY_free(vhost.privatekey);
	BIO_meth_free(biometh_vfs);
	BIO_meth_free(biometh_dtls);

	memset(&vhost, 0, sizeof(vhost));
}
static qboolean OSSL_PluginMayShutdown(void)
{
	//the engine has a habit of holding on to handles without any refcounts, so don't allow it to die early.
	return false;
}

qboolean Plug_Init(void)
{
	fsfuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*fsfuncs));
	netfuncs = plugfuncs->GetEngineInterface(plugnetfuncs_name, sizeof(*netfuncs));
	if (!fsfuncs || !netfuncs)
		return false;

	plugfuncs->ExportFunction("Shutdown", OSSL_PluginShutdown);
	plugfuncs->ExportFunction("MayUnload", OSSL_PluginMayShutdown);

	pdtls_psk_hint = cvarfuncs->GetNVFDG("dtls_psk_hint", "", 0, NULL, "DTLS stuff");
	pdtls_psk_user = cvarfuncs->GetNVFDG("dtls_psk_user", "", 0, NULL, "DTLS stuff");
	pdtls_psk_key  = cvarfuncs->GetNVFDG("dtls_psk_key",  "", 0, NULL, "DTLS stuff");
	netfuncs->RandomBytes(dtlscookiekey, sizeof(dtlscookiekey));	//something random so people can't guess cookies for arbitrary victim IPs.

	OSSL_Init();	//shoving this here solves threading issues (eg two loader threads racing to open an https image url)

	return plugfuncs->ExportInterface("Crypto", &crypto_openssl, sizeof(crypto_openssl)); //export a named interface struct to the engine
}