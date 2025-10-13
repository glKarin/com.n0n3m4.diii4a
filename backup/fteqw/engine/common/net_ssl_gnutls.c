//This file should be easily portable.
//The biggest strength of this plugin system is that ALL interactions are performed via
//named functions, this makes it *really* easy to port plugins from one engine to another.

#include "quakedef.h"
#include "netinc.h"

#ifndef GNUTLS_STATIC
	#define GNUTLS_DYNAMIC 	//statically linking is bad, because that just dynamically links to a .so that probably won't exist.
							//on the other hand, it does validate that the function types are correct.
#endif

#ifdef HAVE_GNUTLS

	#include <gnutls/gnutls.h>
	#if GNUTLS_VERSION_MAJOR >= 3
		#include <gnutls/abstract.h>
	#endif
	#include <gnutls/x509.h>
	#if GNUTLS_VERSION_MAJOR >= 3 && defined(HAVE_DTLS)
		#include <gnutls/dtls.h>
	#else
		#undef HAVE_DTLS
	#endif
	#define gnutls_connection_end_t unsigned int

		#if GNUTLS_VERSION_MAJOR < 3 || (GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR < 3)
			#define GNUTLS_SONUM 26	//cygwin or something.
		#endif
		#if GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR == 3
			#define GNUTLS_SOPREFIX "-deb0"	//not sure what this is about.
			#define GNUTLS_SONUM 28	//debian jessie
		#endif
		#if GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR == 4
			#define GNUTLS_SONUM 30	//ubuntu 16.04
		#endif
		#if GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR == 5
			#define GNUTLS_SONUM 30	//debian stretch
		#endif
		#if GNUTLS_VERSION_MAJOR == 3 && GNUTLS_VERSION_MINOR > 5
			#define GNUTLS_SONUM 30	//no idea what the future holds. maybe we'll be lucky...
		#endif
		#ifndef GNUTLS_SONUM
			#pragma message "GNUTLS version not recognised. Will probably not be loadable."
		#endif
		#ifndef GNUTLS_SOPREFIX
			#define GNUTLS_SOPREFIX
		#endif

#if GNUTLS_VERSION_MAJOR >= 3
#if GNUTLS_VERSION_MAJOR >= 3
	#define GNUTLS_HAVE_SYSTEMTRUST
#endif
#if GNUTLS_VERSION_MAJOR >= 4 || (GNUTLS_VERSION_MAJOR == 3 && (GNUTLS_VERSION_MINOR > 1 || (GNUTLS_VERSION_MINOR == 1 && GNUTLS_VERSION_PATCH >= 1)))
	#define GNUTLS_HAVE_VERIFY3
#endif




#ifdef GNUTLS_HAVE_SYSTEMTRUST
	#define GNUTLS_TRUSTFUNCS GNUTLS_FUNC(gnutls_certificate_set_x509_system_trust,int,(gnutls_certificate_credentials_t cred))
#else
	#define GNUTLS_TRUSTFUNCS GNUTLS_FUNC(gnutls_certificate_set_x509_trust_file,void,(void))
#endif
#ifdef GNUTLS_HAVE_VERIFY3
	#define GNUTLS_VERIFYFUNCS \
		GNUTLS_FUNC(gnutls_certificate_verify_peers3,int,(gnutls_session_t session,const char *hostname,unsigned int *status)) \
		GNUTLS_FUNC(gnutls_certificate_verification_status_print,int,(unsigned int status, gnutls_certificate_type_t type, gnutls_datum_t * out, unsigned int flags))	\
		GNUTLS_FUNC(gnutls_certificate_type_get,gnutls_certificate_type_t,(gnutls_session_t session))	\
		GNUTLS_FUNC(gnutls_certificate_get_peers,const gnutls_datum_t *,(gnutls_session_t session, unsigned int *list_size))
#else
	#define GNUTLS_VERIFYFUNCS \
		GNUTLS_FUNC(gnutls_certificate_verify_peers2,int,(gnutls_session_t session, unsigned int *status)) \
		GNUTLS_FUNC(gnutls_x509_crt_check_hostname,unsigned,(gnutls_x509_crt_t cert, const char *hostname)) \
		GNUTLS_FUNC(gnutls_x509_crt_import,int,(gnutls_x509_crt_t cert, const gnutls_datum_t * data, gnutls_x509_crt_fmt_t format)) \
		GNUTLS_FUNC(gnutls_certificate_get_peers,const gnutls_datum_t *,(gnutls_session_t session, unsigned int *list_size))
#endif

#ifdef HAVE_DTLS
#define GNUTLS_DTLS_STUFF \
		GNUTLS_FUNC(gnutls_key_generate,int,(gnutls_datum_t *key, unsigned int key_size)) \
		GNUTLS_FUNC(gnutls_privkey_sign_hash,int,(gnutls_privkey_t signer, gnutls_digest_algorithm_t hash_algo, unsigned int flags, const gnutls_datum_t * hash_data, gnutls_datum_t * signature)) \
		GNUTLS_FUNC(gnutls_certificate_get_x509_key,int,(gnutls_certificate_credentials_t res, unsigned index, gnutls_x509_privkey_t *key)) \
		GNUTLS_FUNC(gnutls_transport_set_pull_timeout_function,void,(gnutls_session_t session, gnutls_pull_timeout_func func)) \
		GNUTLS_FUNC(gnutls_dtls_cookie_verify,int,(gnutls_datum_t *key, void *client_data, size_t client_data_size, void *_msg, size_t msg_size, gnutls_dtls_prestate_st *prestate)) \
		GNUTLS_FUNC(gnutls_dtls_cookie_send,int,(gnutls_datum_t *key, void *client_data, size_t client_data_size, gnutls_dtls_prestate_st *prestate, gnutls_transport_ptr_t ptr, gnutls_push_func push_func)) \
		GNUTLS_FUNC(gnutls_dtls_prestate_set,void,(gnutls_session_t session, gnutls_dtls_prestate_st *prestate)) \
		GNUTLS_FUNC(gnutls_dtls_set_mtu,void,(gnutls_session_t session, unsigned int mtu)) \
		GNUTLS_FUNC(gnutls_psk_allocate_server_credentials,int,(gnutls_psk_server_credentials_t *sc)) \
		GNUTLS_FUNC(gnutls_psk_set_server_credentials_function,void,(gnutls_psk_server_credentials_t cred, gnutls_psk_server_credentials_function *func)) \
		GNUTLS_FUNC(gnutls_psk_set_server_credentials_hint,int,(gnutls_psk_server_credentials_t res, const char *hint)) \
		GNUTLS_FUNC(gnutls_psk_allocate_client_credentials,int,(gnutls_psk_client_credentials_t *sc)) \
		GNUTLS_FUNC(gnutls_psk_set_client_credentials_function,void,(gnutls_psk_client_credentials_t cred, gnutls_psk_client_credentials_function *func)) \
		GNUTLS_FUNC(gnutls_psk_client_get_hint,const char *,(gnutls_session_t session))
#else
	#define GNUTLS_DTLS_STUFF
#endif


#define GNUTLS_X509_STUFF \
	GNUTLS_FUNC(gnutls_certificate_server_set_request,void,(gnutls_session_t session, gnutls_certificate_request_t req)) \
	GNUTLS_FUNC(gnutls_sec_param_to_pk_bits,unsigned int,(gnutls_pk_algorithm_t algo, gnutls_sec_param_t param))	\
	GNUTLS_FUNC(gnutls_x509_crt_init,int,(gnutls_x509_crt_t * cert))	\
	GNUTLS_FUNC(gnutls_x509_crt_deinit,void,(gnutls_x509_crt_t cert))	\
	GNUTLS_FUNC(gnutls_x509_crt_import,int,(gnutls_x509_crt_t cert, const gnutls_datum_t * data, gnutls_x509_crt_fmt_t format))	\
	GNUTLS_FUNC(gnutls_x509_crt_set_version,int,(gnutls_x509_crt_t crt, unsigned int version))	\
	GNUTLS_FUNC(gnutls_x509_crt_set_activation_time,int,(gnutls_x509_crt_t cert, time_t act_time))	\
	GNUTLS_FUNC(gnutls_x509_crt_set_expiration_time,int,(gnutls_x509_crt_t cert, time_t exp_time))	\
	GNUTLS_FUNC(gnutls_x509_crt_set_serial,int,(gnutls_x509_crt_t cert, const void *serial, size_t serial_size))	\
	GNUTLS_FUNC(gnutls_x509_crt_set_dn,int,(gnutls_x509_crt_t crt, const char *dn, const char **err))	\
	GNUTLS_FUNC(gnutls_x509_crt_get_dn3,int,(gnutls_x509_crt_t crt, gnutls_datum_t * dn, unsigned flags))	\
	GNUTLS_FUNC(gnutls_x509_crt_set_issuer_dn,int,(gnutls_x509_crt_t crt,const char *dn, const char **err))	\
	GNUTLS_FUNC(gnutls_x509_crt_set_key,int,(gnutls_x509_crt_t crt, gnutls_x509_privkey_t key))	\
	GNUTLS_FUNC(gnutls_x509_crt_export2,int,(gnutls_x509_crt_t cert, gnutls_x509_crt_fmt_t format, gnutls_datum_t * out))	\
	GNUTLS_FUNC(gnutls_x509_privkey_init,int,(gnutls_x509_privkey_t * key))	\
	GNUTLS_FUNC(gnutls_x509_privkey_deinit,void,(gnutls_x509_privkey_t key))	\
	GNUTLS_FUNC(gnutls_x509_privkey_generate,int,(gnutls_x509_privkey_t key, gnutls_pk_algorithm_t algo, unsigned int bits, unsigned int flags))	\
	GNUTLS_FUNC(gnutls_x509_privkey_export2,int,(gnutls_x509_privkey_t key, gnutls_x509_crt_fmt_t format, gnutls_datum_t * out))	\
	GNUTLS_FUNC(gnutls_x509_crt_privkey_sign,int,(gnutls_x509_crt_t crt, gnutls_x509_crt_t issuer, gnutls_privkey_t issuer_key, gnutls_digest_algorithm_t dig, unsigned int flags))	\
	GNUTLS_FUNC(gnutls_privkey_init,int,(gnutls_privkey_t * key))	\
	GNUTLS_FUNC(gnutls_privkey_deinit,void,(gnutls_privkey_t key))	\
	GNUTLS_FUNC(gnutls_privkey_import_x509,int,(gnutls_privkey_t pkey, gnutls_x509_privkey_t key, unsigned int flags))	\
	GNUTLS_FUNC(gnutls_certificate_set_x509_key_mem,int,(gnutls_certificate_credentials_t res, const gnutls_datum_t * cert, const gnutls_datum_t * key, gnutls_x509_crt_fmt_t type))	\
	GNUTLS_FUNC(gnutls_pubkey_init,int,(gnutls_pubkey_t * key))	\
	GNUTLS_FUNC(gnutls_pubkey_deinit,void,(gnutls_pubkey_t key))	\
	GNUTLS_FUNC(gnutls_pubkey_import_x509,int,(gnutls_pubkey_t key, gnutls_x509_crt_t crt, unsigned int flags))	\
	GNUTLS_FUNC(gnutls_pubkey_verify_hash2,int,(gnutls_pubkey_t key, gnutls_sign_algorithm_t algo, unsigned int flags, const gnutls_datum_t * hash, const gnutls_datum_t * signature))	\
	GNUTLS_FUNC(gnutls_certificate_get_ours,const gnutls_datum_t*,(gnutls_session_t session))	\
	GNUTLS_FUNC(gnutls_certificate_get_crt_raw,int,(gnutls_certificate_credentials_t sc, unsigned idx1, unsigned idx2, gnutls_datum_t * cert))


#define GNUTLS_FUNCS \
	GNUTLS_FUNC(gnutls_bye,int,(gnutls_session_t session, gnutls_close_request_t how))	\
	GNUTLS_FUNC(gnutls_alert_get,gnutls_alert_description_t,(gnutls_session_t session)) \
	GNUTLS_FUNC(gnutls_alert_get_name,const char *,(gnutls_alert_description_t alert)) \
	GNUTLS_FUNC(gnutls_strerror,const char *,(int error))	\
	GNUTLS_FUNC(gnutls_handshake,int,(gnutls_session_t session))	\
	GNUTLS_FUNC(gnutls_transport_set_ptr,void,(gnutls_session_t session, gnutls_transport_ptr_t ptr))	\
	GNUTLS_FUNC(gnutls_transport_set_push_function,void,(gnutls_session_t session, gnutls_push_func push_func))	\
	GNUTLS_FUNC(gnutls_transport_set_pull_function,void,(gnutls_session_t session, gnutls_pull_func pull_func))	\
	GNUTLS_FUNC(gnutls_transport_set_errno,void,(gnutls_session_t session, int err))	\
	GNUTLS_FUNC(gnutls_error_is_fatal,int,(int error))	\
	GNUTLS_FUNC(gnutls_credentials_set,int,(gnutls_session_t, gnutls_credentials_type_t type, void* cred))	\
	GNUTLS_FUNC(gnutls_init,int,(gnutls_session_t * session, gnutls_connection_end_t con_end))	\
	GNUTLS_FUNC(gnutls_deinit,void,(gnutls_session_t session))	\
	GNUTLS_FUNC(gnutls_set_default_priority,int,(gnutls_session_t session))	\
	GNUTLS_FUNC(gnutls_certificate_allocate_credentials,int,(gnutls_certificate_credentials_t *sc))	\
	GNUTLS_FUNC(gnutls_certificate_free_credentials,void,(gnutls_certificate_credentials_t sc))	\
	GNUTLS_FUNC(gnutls_session_channel_binding,int,(gnutls_session_t session, gnutls_channel_binding_t cbtype, gnutls_datum_t * cb))	\
	GNUTLS_FUNC(gnutls_global_init,int,(void))	\
	GNUTLS_FUNC(gnutls_global_deinit,void,(void))	\
	GNUTLS_FUNC(gnutls_record_send,ssize_t,(gnutls_session_t session, const void *data, size_t sizeofdata))	\
	GNUTLS_FUNC(gnutls_record_recv,ssize_t,(gnutls_session_t session, void *data, size_t sizeofdata))	\
	GNUTLS_FUNC(gnutls_certificate_set_verify_function,void,(gnutls_certificate_credentials_t cred, gnutls_certificate_verify_function *func))	\
	GNUTLS_FUNC(gnutls_session_get_ptr,void*,(gnutls_session_t session))	\
	GNUTLS_FUNC(gnutls_session_set_ptr,void,(gnutls_session_t session, void *ptr))	\
	GNUTLS_FUNCPTR(gnutls_malloc,void*,(size_t sz),(sz))	\
	GNUTLS_FUNCPTR(gnutls_free,void,(void *ptr),(ptr))	\
	GNUTLS_FUNC(gnutls_server_name_set,int,(gnutls_session_t session, gnutls_server_name_type_t type, const void * name, size_t name_length))	\
	GNUTLS_TRUSTFUNCS	\
	GNUTLS_VERIFYFUNCS	\
	GNUTLS_DTLS_STUFF	\
	GNUTLS_X509_STUFF


#ifdef GNUTLS_DYNAMIC
	#define GNUTLS_FUNC(n,ret,args) static ret (VARGS *q##n)args;
	#define GNUTLS_FUNCPTR(n,ret,arglist,callargs) static ret (VARGS **q##n)arglist;
#else
	#define GNUTLS_FUNC(n,ret,args) static ret (VARGS *q##n)args = n;
	#define GNUTLS_FUNCPTR(n,ret,arglist,callargs) static ret VARGS q##n arglist {return n(callargs);};
#endif

#ifdef HAVE_DTLS
	GNUTLS_FUNC(gnutls_set_default_priority_append,int,(gnutls_session_t session, const char *add_prio, const char **err_pos, unsigned flags))
#endif
GNUTLS_FUNCS

#undef GNUTLS_FUNC
#undef GNUTLS_FUNCPTR

#if defined(GNUTLS_DYNAMIC) && defined(HAVE_DTLS)
static int VARGS fallback_gnutls_set_default_priority_append(gnutls_session_t session, const char *add_prio, const char **err_pos, unsigned flags)
{
	return qgnutls_set_default_priority(session);
}
#endif

static struct
{
#ifdef GNUTLS_DYNAMIC
	dllhandle_t *hmod;
#endif
	int initstatus[2];
} gnutls;

static qboolean Init_GNUTLS(void)
{
#ifdef GNUTLS_DYNAMIC
	dllfunction_t functable[] =
	{
#define GNUTLS_FUNC(nam,ret,args) {(void**)&q##nam, #nam},
#define GNUTLS_FUNCPTR(nam,ret,arglist,calllist) {(void**)&q##nam, #nam},
		GNUTLS_FUNCS
#undef GNUTLS_FUNC
#undef GNUTLS_FUNCPTR
		{NULL, NULL}
	};
	
#ifdef GNUTLS_SONUM
	#ifdef __CYGWIN__
		gnutls.hmod = Sys_LoadLibrary("cyggnutls"GNUTLS_SOPREFIX"-"STRINGIFY(GNUTLS_SONUM)".dll", functable);
	#else
		gnutls.hmod = Sys_LoadLibrary("libgnutls"GNUTLS_SOPREFIX".so."STRINGIFY(GNUTLS_SONUM), functable);
	#endif
#else
	gnutls.hmod = Sys_LoadLibrary("libgnutls"GNUTLS_SOPREFIX".so", functable);	//hope and pray
#endif
	if (!gnutls.hmod)
		return false;

#ifdef HAVE_DTLS
	qgnutls_set_default_priority_append = Sys_GetAddressForName(gnutls.hmod, "gnutls_set_default_priority_append");
	if (!qgnutls_set_default_priority_append)
		qgnutls_set_default_priority_append = fallback_gnutls_set_default_priority_append;
#endif
#endif
	return true;
}

typedef struct 
{
	vfsfile_t funcs;
	vfsfile_t *stream;

	char certname[512];
	gnutls_session_t session;

	qboolean handshaking;
	qboolean datagram;

	int pullerror;	//adding these two because actual networking errors are not getting represented properly, at least with regard to timeouts.
	int pusherror;

	gnutls_certificate_credentials_t certcred;
	hashfunc_t *peerhashfunc;
	qbyte peerdigest[DIGEST_MAXSIZE];

	qboolean challenging;	//not sure this is actually needed, but hey.
	void *cbctx;
	neterr_t(*cbpush)(void *cbctx, const qbyte *data, size_t datasize);
	qbyte *readdata;
	size_t readsize;
#ifdef HAVE_DTLS
	gnutls_dtls_prestate_st prestate;
#endif
//	int mtu;
} gnutlsfile_t;

static void SSL_SetCertificateName(gnutlsfile_t *f, const char *hostname)
{
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
			if (host && host-hostname < sizeof(f->certname))
			{
				memcpy(f->certname, hostname, host-hostname);
				f->certname[host-hostname] = 0;
				hostname = f->certname;
			}
		}
		else
		{	//eg: 127.0.0.1:port - strip the port number if specified.
			host = strchr(hostname, ':');
			if (host && host-hostname < sizeof(f->certname))
			{
				memcpy(f->certname, hostname, host-hostname);
				f->certname[host-hostname] = 0;
				hostname = f->certname;
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
		*f->certname = 0;
	else if (hostname == f->certname)
		;
	else if (strlen(hostname) >= sizeof(f->certname))
		*f->certname = 0;
	else
		memcpy(f->certname, hostname, strlen(hostname)+1);
}

#define CAFILE "/etc/ssl/certs/ca-certificates.crt"

static void SSL_Close(vfsfile_t *vfs)
{
	gnutlsfile_t *file = (void*)vfs;

	file->handshaking = true;	//so further attempts to use it will fail.

	if (file->session)
	{
		qgnutls_bye (file->session, file->datagram?GNUTLS_SHUT_WR:GNUTLS_SHUT_RDWR);
		qgnutls_deinit(file->session);
		file->session = NULL;
	}

	if (file->certcred)
	{
		qgnutls_certificate_free_credentials(file->certcred);
		file->certcred = NULL;
	}
}
static qboolean QDECL SSL_CloseFile(vfsfile_t *vfs)
{
	gnutlsfile_t *file = (void*)vfs;
	SSL_Close(vfs);
	if (file->stream)
	{
		VFS_CLOSE(file->stream);
		file->stream = NULL;
	}
	if (file->certcred)
		qgnutls_certificate_free_credentials(file->certcred);
	Z_Free(vfs);
	return true;
}

static int SSL_CheckUserTrust(gnutls_session_t session, gnutlsfile_t *file, int gcertcode)
{
	int ret = gcertcode?GNUTLS_E_CERTIFICATE_ERROR:GNUTLS_E_SUCCESS;
#if defined(HAVE_CLIENT) && defined(HAVE_DTLS)
	unsigned int ferrcode;
	//when using dtls, we expect self-signed certs and persistent trust.
	if (file->datagram)
	{
		qbyte *certdata;
		size_t certsize;
		unsigned int certcount, j;
		const gnutls_datum_t *const certlist = qgnutls_certificate_get_peers(session, &certcount);
		for (certsize = 0, j = 0; j < certcount; j++)
			certsize += certlist[j].size;
		certdata = malloc(certsize);
		for (certsize = 0, j = 0; j < certcount; j++)
		{
			memcpy(certdata+certsize, certlist[j].data, certlist[j].size);
			certsize += certlist[j].size;
		}

		//if gcertcode is 0 then we can still pin it.
		ferrcode = 0;
		if (gcertcode & GNUTLS_CERT_SIGNER_NOT_FOUND)
			ferrcode |= CERTLOG_MISSINGCA;
		if (gcertcode & GNUTLS_CERT_UNEXPECTED_OWNER)
			ferrcode |= CERTLOG_WRONGHOST;
		if (gcertcode & GNUTLS_CERT_EXPIRED)
			ferrcode |= CERTLOG_EXPIRED;
		if (gcertcode & ~(GNUTLS_CERT_INVALID|GNUTLS_CERT_SIGNER_NOT_FOUND|GNUTLS_CERT_UNEXPECTED_OWNER|GNUTLS_CERT_EXPIRED))
			ferrcode |= CERTLOG_UNKNOWN;

		if (CertLog_ConnectOkay(file->certname, certdata, certsize, ferrcode))
			ret = GNUTLS_E_SUCCESS;	//user has previously authorised it.
		else
			ret = GNUTLS_E_CERTIFICATE_ERROR;	//user didn't trust it yet
		free(certdata);
	}
#endif

	return ret;
}

static int QDECL SSL_CheckCert(gnutls_session_t session)
{
	gnutlsfile_t *file = qgnutls_session_get_ptr (session);
	unsigned int certstatus;
	qboolean preverified = false;

	size_t knownsize;
	qbyte *knowndata = TLS_GetKnownCertificate(file->certname, &knownsize);

	if (knowndata)
	{
		unsigned int certcount, j;
		const gnutls_datum_t *const certlist = qgnutls_certificate_get_peers(session, &certcount);
		if (!certlist || !certcount)
		{
			BZ_Free(knowndata);
			return GNUTLS_E_CERTIFICATE_ERROR;
		}
		else
		{
			size_t offset = 0;

			for (j = 0; j < certcount; offset += certlist[j++].size)
			{
				if (certlist[j].size+offset > knownsize)
					break;	//overflow...
				if (memcmp(certlist[j].data, knowndata+offset, certlist[j].size))
					break;
			}

			if (j && j == certcount && offset == knownsize)
				preverified = true;
			else
			{
#ifdef _DEBUG
				for (j = 0, offset = 0; j < certcount; j++)
					offset += certlist[j].size;
				Con_Printf("%s cert %"PRIuSIZE" bytes (chain %u)\n", file->certname, offset, certcount);
				Con_Printf("/*%s*/\"", file->certname);
				for (j = 0; file->certname[j]; j++)
					Con_Printf("\\x%02x", file->certname[j]^0xff);
				Con_Printf("\\xff");
				Con_Printf("\\x%02x\\x%02x", (unsigned)offset&0xff, ((unsigned)offset>>8)&0xff);
				for (j = 0; j < certcount; j++)
				{
					unsigned char *data = certlist[j].data;
					unsigned int datasize = certlist[j].size, k;
					for (k = 0; k < datasize; k++)
						Con_Printf("\\x%02x", data[k]^0xff);
				}
				Con_Printf("\",\n\n");
#endif
				Con_Printf(CON_ERROR "%s: Reported certificate does not match known certificate. Possible MITM attack, alternatively just an outdated client.\n", file->certname);
				BZ_Free(knowndata);
				return GNUTLS_E_CERTIFICATE_ERROR;
			}
		}
		BZ_Free(knowndata);
	}

#ifdef GNUTLS_HAVE_VERIFY3
	if (qgnutls_certificate_verify_peers3(session, file->certname, &certstatus) >= 0)
	{
		gnutls_datum_t out = {NULL,0};
		gnutls_certificate_type_t type;
		int ret;

		if (preverified && (certstatus&~GNUTLS_CERT_EXPIRED) == (GNUTLS_CERT_INVALID|GNUTLS_CERT_SIGNER_NOT_FOUND))
			return 0;
		ret = SSL_CheckUserTrust(session, file, certstatus);
		if (!ret)
			return ret;

		type = qgnutls_certificate_type_get (session);
		if (qgnutls_certificate_verification_status_print(certstatus, type, &out, 0) >= 0)
		{
			Con_Printf(CON_ERROR "%s: %s (%x)\n", file->certname, out.data, certstatus);
			(*qgnutls_free)(out.data);
		}
		else
			Con_Printf(CON_ERROR "%s: UNKNOWN STATUS (%x)\n", file->certname, certstatus);

		if (tls_ignorecertificateerrors.ival)
		{
			Con_Printf(CON_ERROR "%s: Ignoring certificate errors (tls_ignorecertificateerrors is %i)\n", file->certname, tls_ignorecertificateerrors.ival);
			return 0;
		}
	}
#else
	if (qgnutls_certificate_verify_peers2(session, &certstatus) >= 0)
	{
		int certslen;
		//grab the certificate
		const gnutls_datum_t *const certlist = qgnutls_certificate_get_peers(session, &certslen);
		if (certlist && certslen)
		{
			//and make sure the hostname on it actually makes sense.
			int ret;
			gnutls_x509_crt_t cert;
			qgnutls_x509_crt_init(&cert);
			qgnutls_x509_crt_import(cert, certlist, GNUTLS_X509_FMT_DER);
			if (qgnutls_x509_crt_check_hostname(cert, file->certname))
			{
				if (preverified && (certstatus&~GNUTLS_CERT_EXPIRED) == (GNUTLS_CERT_INVALID|GNUTLS_CERT_SIGNER_NOT_FOUND))
					return 0;

				ret = SSL_CheckUserTrust(session, file, certstatus);	//looks okay... pin it by default...
				if (!ret)
					return ret;

				if (certstatus & GNUTLS_CERT_SIGNER_NOT_FOUND)
					Con_Printf(CON_ERROR "%s: Certificate authority is not recognised\n", file->certname);
				else if (certstatus & GNUTLS_CERT_INSECURE_ALGORITHM)
					Con_Printf(CON_ERROR "%s: Certificate uses insecure algorithm\n", file->certname);
				else if (certstatus & (GNUTLS_CERT_REVOCATION_DATA_ISSUED_IN_FUTURE|GNUTLS_CERT_REVOCATION_DATA_SUPERSEDED|GNUTLS_CERT_EXPIRED|GNUTLS_CERT_REVOKED|GNUTLS_CERT_NOT_ACTIVATED))
					Con_Printf(CON_ERROR "%s: Certificate has expired or was revoked or not yet valid\n", file->certname);
				else if (certstatus & GNUTLS_CERT_SIGNATURE_FAILURE)
					Con_Printf(CON_ERROR "%s: Certificate signature failure\n", file->certname);
				else
					Con_Printf(CON_ERROR "%s: Certificate error\n", file->certname);
				if (tls_ignorecertificateerrors.ival)
				{
					Con_Printf(CON_ERROR "%s: Ignoring certificate errors (tls_ignorecertificateerrors is %i)\n", file->certname, tls_ignorecertificateerrors.ival);
					return 0;
				}
			}
			else
				Con_DPrintf(CON_ERROR "%s: certificate is for a different domain\n", file->certname);
		}
	}
#endif

	Con_DPrintf(CON_ERROR "%s: rejecting certificate\n", file->certname);
	return GNUTLS_E_CERTIFICATE_ERROR;
}

#ifdef HAVE_DTLS
static int QDECL SSL_CheckFingerprint(gnutls_session_t session)
{	//actual certificate doesn't matter so long as it matches the hash we expect.
	gnutlsfile_t *file = qgnutls_session_get_ptr (session);
	unsigned int certcount, j;
	const gnutls_datum_t *const certlist = qgnutls_certificate_get_peers(session, &certcount);
	if (certlist && certcount)
	{
		qbyte digest[DIGEST_MAXSIZE];
		void *ctx = alloca(file->peerhashfunc->contextsize);

		file->peerhashfunc->init(ctx);
		for (j = 0; j < certcount; j++)
			file->peerhashfunc->process(ctx, certlist[j].data, certlist[j].size);
		file->peerhashfunc->terminate(digest, ctx);

		if (!memcmp(digest, file->peerdigest, file->peerhashfunc->digestsize))
			return 0;
		Con_Printf(CON_ERROR "%s: certificate chain (%i) does not match fingerprint\n", *file->certname?file->certname:"<anon>", certcount);
	}
	else
		Con_Printf(CON_ERROR "%s: peer did not provide any certificate\n", *file->certname?file->certname:"<anon>");
	return GNUTLS_E_CERTIFICATE_ERROR;
}
#endif

//return 1 to read data.
//-1 for error
//0 for not ready
static int SSL_DoHandshake(gnutlsfile_t *file)
{
	int err;
	//session was previously closed = error
	if (!file->session)
	{
		//Sys_Printf("null session\n");
		return VFS_ERROR_UNSPECIFIED;
	}

	err = qgnutls_handshake (file->session);
	if (err < 0)
	{	//non-fatal errors can just handshake again the next time the caller checks to see if there's any data yet
		//(e_again or e_intr)
		if (!qgnutls_error_is_fatal(err))
			return 0;
		if (developer.ival)
		{
			if (err == GNUTLS_E_FATAL_ALERT_RECEIVED)
			{	//peer doesn't like us.
				gnutls_alert_description_t desc = qgnutls_alert_get(file->session);
				Con_Printf(CON_ERROR"GNU%sTLS: %s: %s(%i)\n", file->datagram?"D":"", file->certname, qgnutls_alert_get_name(desc), desc);
			}
			else
			{
				//we didn't like the peer.
				Con_Printf(CON_ERROR"GNU%sTLS: %s: %s(%i)\n", file->datagram?"D":"", file->certname, qgnutls_strerror(err), err);
			}
		}

//		Con_Printf("%s: abort\n", file->certname);

		switch(err)
		{
		case GNUTLS_E_INSUFFICIENT_CREDENTIALS:
		case GNUTLS_E_CERTIFICATE_ERROR:		err = VFS_ERROR_UNTRUSTED;		break;
		case GNUTLS_E_SESSION_EOF:
		case GNUTLS_E_PREMATURE_TERMINATION:	err = VFS_ERROR_EOF;			break;
		case GNUTLS_E_PUSH_ERROR:				err = file->pusherror;			break;
		case GNUTLS_E_PULL_ERROR:				err = file->pullerror;			break;
		default:								err = VFS_ERROR_UNSPECIFIED;	break;
		}
		SSL_Close(&file->funcs);
		return err;
	}
	file->handshaking = false;
	return 1;
}

static int QDECL SSL_Read(struct vfsfile_s *f, void *buffer, int bytestoread)
{
	gnutlsfile_t *file = (void*)f;
	int read;

	if (file->handshaking)
	{
		read = SSL_DoHandshake(file);
		if (read <= 0)
			return read;
	}

	if (!bytestoread)	//gnutls doesn't like this.
		return VFS_ERROR_UNSPECIFIED;	//caller is expecting data that we can never return, or something.

	read = qgnutls_record_recv(file->session, buffer, bytestoread);
	if (read < 0)
	{
		if (read == GNUTLS_E_PREMATURE_TERMINATION)
		{
			Con_Printf("TLS Premature Termination from %s\n", file->certname);
			return VFS_ERROR_EOF;
		}
		else if (read == GNUTLS_E_REHANDSHAKE)
		{
			file->handshaking = false;//gnutls_safe_renegotiation_status();
			//if false, 'recommended' to send an GNUTLS_A_NO_RENEGOTIATION alert, no idea how.
		}
		else if (!qgnutls_error_is_fatal(read))
			return 0;	//caller is expected to try again later, no real need to loop here, just in case it repeats (eg E_AGAIN)
		else
		{
			if (read == GNUTLS_E_PULL_ERROR)
				Con_Printf("GNUTLS_E_PULL_ERROR (%s)\n", file->certname);
			else
				Con_Printf("GNUTLS Read Warning %i (bufsize %i)\n", read, bytestoread);
			return -1;
		}
	}
	else if (read == 0)
		return VFS_ERROR_EOF;	//closed by remote connection.
	return read;
}
static int QDECL SSL_Write(struct vfsfile_s *f, const void *buffer, int bytestowrite)
{
	gnutlsfile_t *file = (void*)f;
	int written;

	if (file->handshaking)
	{
		written = SSL_DoHandshake(file);
		if (written <= 0)
			return written;
	}

	written = qgnutls_record_send(file->session, buffer, bytestowrite);
	if (written < 0)
	{
		if (!qgnutls_error_is_fatal(written))
			return 0;
		else
		{
			Con_DPrintf("GNUTLS Send Error %i (%i bytes)\n", written, bytestowrite);
			return VFS_ERROR_UNSPECIFIED;
		}
	}
	else if (written == 0)
		return VFS_ERROR_EOF;	//closed by remote connection.
	return written;
}
static qboolean QDECL SSL_Seek (struct vfsfile_s *file, qofs_t pos)
{
	return false;
}
static qofs_t QDECL SSL_Tell (struct vfsfile_s *file)
{
	return 0;
}
static qofs_t QDECL SSL_GetLen (struct vfsfile_s *file)
{
	return 0;
}


#include <errno.h>

/*functions for gnutls to call when it wants to send data*/
static ssize_t SSL_Push(gnutls_transport_ptr_t p, const void *data, size_t size)
{
	gnutlsfile_t *file = p;
//	Sys_Printf("SSL_Push: %u\n", size);
	int done = VFS_WRITE(file->stream, data, size);
	if (done <= 0)
	{
		int eno;
		file->pusherror = done;
		switch(done)
		{
		case VFS_ERROR_EOF:			return 0;
		case VFS_ERROR_DNSFAILURE:
		case VFS_ERROR_NORESPONSE:	eno = ECONNRESET;	break;
		case VFS_ERROR_TRYLATER:	eno = EAGAIN;		break;
		case VFS_ERROR_REFUSED:		eno = ECONNREFUSED;	break;
//		case VFS_ERROR_UNSPECIFIED:
//		case VFS_ERROR_DNSFAILURE:
//		case VFS_ERROR_WRONGCERT:
//		case VFS_ERROR_UNTRUSTED:
		default:					eno = ECONNRESET;	break;
		}
		qgnutls_transport_set_errno(file->session, eno);
		return -1;
	}
	return done;
}
static ssize_t SSL_Pull(gnutls_transport_ptr_t p, void *data, size_t size)
{
	gnutlsfile_t *file = p;
//	Sys_Printf("SSL_Pull: %u\n", size);
	int done = VFS_READ(file->stream, data, size);
	if (done <= 0)
	{
		int eno;
		file->pullerror = done;
		switch(done)
		{
		case VFS_ERROR_EOF:			return 0;
		case VFS_ERROR_DNSFAILURE:
		case VFS_ERROR_NORESPONSE:	eno = ECONNRESET;	break;
		case VFS_ERROR_TRYLATER:	eno = EAGAIN;		break;
		case VFS_ERROR_REFUSED:		eno = ECONNREFUSED;	break;
		default:					eno = ECONNRESET;	break;
		}
		qgnutls_transport_set_errno(file->session, eno);
		return -1;
	}
	return done;
}

static ssize_t DTLS_Push(gnutls_transport_ptr_t p, const void *data, size_t size)
{
	gnutlsfile_t *file = p;

	neterr_t ne = file->cbpush(file->cbctx, data, size);

//	Sys_Printf("DTLS_Push: %u, err=%i\n", (unsigned)size, (int)ne);

	if (!file->session)
		return ne?-1:size;

	switch(ne)
	{
	case NETERR_CLOGGED:
	case NETERR_NOROUTE:
		qgnutls_transport_set_errno(file->session, EAGAIN);
		return -1;
	case NETERR_MTU:
		qgnutls_transport_set_errno(file->session, EMSGSIZE);
		return -1;
	case NETERR_DISCONNECTED:
		qgnutls_transport_set_errno(file->session, EPERM);
		return -1;
	default:
		qgnutls_transport_set_errno(file->session, 0);
		return size;
	}
}
static ssize_t DTLS_Pull(gnutls_transport_ptr_t p, void *data, size_t size)
{
	gnutlsfile_t *file = p;

//	Sys_Printf("DTLS_Pull: %u of %u\n", size, file->readsize);

	if (!file->readsize)
	{	//no data left
//		Sys_Printf("DTLS_Pull: EAGAIN\n");
		qgnutls_transport_set_errno(file->session, EAGAIN);
		return -1;
	}
	else if (file->readsize > size)
	{	//buffer passed is smaller than available data
//		Sys_Printf("DTLS_Pull: EMSGSIZE\n");
		memcpy(data, file->readdata, size);
		file->readsize = 0;
		qgnutls_transport_set_errno(file->session, EMSGSIZE);
		return -1;
	}
	else
	{	//buffer is big enough to read it all
		size = file->readsize;
		file->readsize = 0;
//		Sys_Printf("DTLS_Pull: reading %i\n", size);
		memcpy(data, file->readdata, size);
		qgnutls_transport_set_errno(file->session, 0);
		return size;
	}
}
#ifdef HAVE_DTLS
static int DTLS_Pull_Timeout(gnutls_transport_ptr_t p, unsigned int timeout)
{	//gnutls (pointlessly) requires this function for dtls.
	gnutlsfile_t *f = p;
//	Sys_Printf("DTLS_Pull_Timeout %i, %i\n", timeout, f->readsize);
	return f->readsize>0?1:0;
}
#endif

#ifdef USE_ANON
static gnutls_anon_client_credentials_t anoncred[2];
#else
static gnutls_certificate_credentials_t xcred[2];
static qboolean	servercertfail;
#endif
#ifdef HAVE_DTLS
static gnutls_datum_t cookie_key;
#endif

static vfsfile_t *SSL_OpenPrivKey(char *displayname, size_t displaysize)
{
#define privname "privkey.pem"
	vfsfile_t *privf;
	const char *mode = displayname?"wb":"rb";
	int i = COM_CheckParm("-privkey");
	if (i++)
	{
		if (displayname)
			if (!FS_DisplayPath(com_argv[i], FS_SYSTEM, displayname, displaysize))
				Q_strncpyz(displayname, com_argv[i], displaysize);
		privf = FS_OpenVFS(com_argv[i], mode, FS_SYSTEM);
	}
	else
	{
		if (displayname)
			if (!FS_DisplayPath(privname, FS_ROOT, displayname, displaysize))
				return NULL;

		privf = FS_OpenVFS(privname, mode, FS_ROOT);
	}
	return privf;
#undef privname
}
static vfsfile_t *SSL_OpenPubKey(char *displayname, size_t displaysize)
{
#define fullchainname "fullchain.pem"
#define pubname "cert.pem"
	vfsfile_t *pubf = NULL;
	const char *mode = displayname?"wb":"rb";
	int i = COM_CheckParm("-pubkey");
	if (i++)
	{
		if (displayname)
			Q_strncpyz(displayname, com_argv[i], displaysize);
		pubf = FS_OpenVFS(com_argv[i], mode, FS_SYSTEM);
	}
	else
	{
		if (!pubf && (!displayname || FS_DisplayPath(fullchainname, FS_ROOT, displayname, displaysize)))
			pubf = FS_OpenVFS(fullchainname, mode, FS_ROOT);
		if (!pubf && (!displayname || FS_DisplayPath(pubname, FS_ROOT, displayname, displaysize)))
			pubf = FS_OpenVFS(pubname, mode, FS_ROOT);
	}
	return pubf;
#undef pubname
}

static qboolean SSL_LoadPrivateCert(gnutls_certificate_credentials_t cred)
{
	int ret = -1;
	gnutls_datum_t priv, pub;
	vfsfile_t *privf = SSL_OpenPrivKey(NULL, 0);
	vfsfile_t *pubf = SSL_OpenPubKey(NULL, 0);
	const char *hostname = NULL;

	int i = COM_CheckParm("-certhost");
	if (i)
		hostname = com_argv[i+1];

	memset(&priv, 0, sizeof(priv));
	memset(&pub, 0, sizeof(pub));

	if ((!privf || !pubf))// && hostname)
	{	//not found? generate a new one.
		//FIXME: how to deal with race conditions with multiple servers on the same host?
		//delay till the first connection? we at least write both files at the sameish time.
		//even so they might get different certs the first time the server(s) run.
		//TODO: implement a lockfile
		gnutls_x509_privkey_t key;
		gnutls_x509_crt_t cert;
		char serial[64];
		const char *errstr;
		gnutls_pk_algorithm_t privalgo = GNUTLS_PK_RSA;

		if (privf)
		{
			VFS_CLOSE(privf);
			privf = NULL;
		}
		if (pubf)
		{
			VFS_CLOSE(pubf);
			pubf = NULL;
		}

		Con_Printf("Generating new GNUTLS key+cert...\n");

		qgnutls_x509_privkey_init(&key);
		ret = qgnutls_x509_privkey_generate(key, privalgo, qgnutls_sec_param_to_pk_bits(privalgo, GNUTLS_SEC_PARAM_HIGH), 0);
		if (ret < 0)
			Con_Printf(CON_ERROR"gnutls_x509_privkey_generate failed: %i\n", ret);
		ret = qgnutls_x509_privkey_export2(key, GNUTLS_X509_FMT_PEM, &priv);
		if (ret < 0)
			Con_Printf(CON_ERROR"gnutls_x509_privkey_export2 failed: %i\n", ret);

		//stoopid browsers insisting that serial numbers are different even on throw-away self-signed certs.
		//we should probably just go and make our own root ca/master. post it a cert and get a signed one (with sequential serial) back or something.
		//we'll probably want something like that for client certs anyway, for stat tracking.
		Q_snprintfz(serial, sizeof(serial), "%u", (unsigned)time(NULL));

		qgnutls_x509_crt_init(&cert);
		qgnutls_x509_crt_set_version(cert, 1);
		qgnutls_x509_crt_set_activation_time(cert, time(NULL)-1);
		qgnutls_x509_crt_set_expiration_time(cert, time(NULL)+(time_t)10*365*24*60*60);
		qgnutls_x509_crt_set_serial(cert, serial, strlen(serial));
		if (!hostname)
			/*qgnutls_x509_crt_set_key_usage(cert, GNUTLS_KEY_DIGITAL_SIGNATURE)*/;
		else
		{
			if (qgnutls_x509_crt_set_dn(cert, va("CN=%s", hostname), &errstr) < 0)
				Con_Printf(CON_ERROR"gnutls_x509_crt_set_dn failed: %s\n", errstr);
			if (qgnutls_x509_crt_set_issuer_dn(cert, va("CN=%s", hostname), &errstr) < 0)
				Con_Printf(CON_ERROR"gnutls_x509_crt_set_issuer_dn failed: %s\n", errstr);
//			qgnutls_x509_crt_set_key_usage(cert, GNUTLS_KEY_KEY_ENCIPHERMENT|GNUTLS_KEY_DATA_ENCIPHERMENT|);
		}
		qgnutls_x509_crt_set_key(cert, key);

		/*sign it with our private key*/
		{
			gnutls_privkey_t akey;
			qgnutls_privkey_init(&akey);
			qgnutls_privkey_import_x509(akey, key, GNUTLS_PRIVKEY_IMPORT_COPY);
			ret = qgnutls_x509_crt_privkey_sign(cert, cert, akey, GNUTLS_DIG_SHA256, 0);
			if (ret < 0)
				Con_Printf(CON_ERROR"gnutls_x509_crt_privkey_sign failed: %i\n", ret);
			qgnutls_privkey_deinit(akey);
		}
		ret = qgnutls_x509_crt_export2(cert, GNUTLS_X509_FMT_PEM, &pub);
		qgnutls_x509_crt_deinit(cert);
		qgnutls_x509_privkey_deinit(key);
		if (ret < 0)
			Con_Printf(CON_ERROR"gnutls_x509_crt_export2 failed: %i\n", ret);

		if (priv.size && pub.size)
		{
			char displayname[MAX_OSPATH];
			privf = SSL_OpenPrivKey(displayname, sizeof(displayname));
			if (privf)
			{
				VFS_WRITE(privf, priv.data, priv.size);
				VFS_CLOSE(privf);
				Con_Printf("Wrote %s\n", displayname);
			}
//			memset(priv.data, 0, priv.size);
			(*qgnutls_free)(priv.data);
			memset(&priv, 0, sizeof(priv));

			pubf = SSL_OpenPubKey(displayname, sizeof(displayname));
			if (pubf)
			{
				VFS_WRITE(pubf, pub.data, pub.size);
				VFS_CLOSE(pubf);
				Con_Printf("Wrote %s\n", displayname);
			}
			(*qgnutls_free)(pub.data);
			memset(&pub, 0, sizeof(pub));

			privf = SSL_OpenPrivKey(NULL, 0);
			pubf = SSL_OpenPubKey(NULL, 0);

			Con_Printf("Certificate generated\n");
		}
	}

	if (privf && pubf)
	{
		//read the two files now
		priv.size = VFS_GETLEN(privf);
		priv.data = (*qgnutls_malloc)(priv.size+1);
		if (priv.size != VFS_READ(privf, priv.data, priv.size))
			priv.size = 0;
		priv.data[priv.size] = 0;

		pub.size = VFS_GETLEN(pubf);
		pub.data = (*qgnutls_malloc)(pub.size+1);
		if (pub.size != VFS_READ(pubf, pub.data, pub.size))
			pub.size = 0;
		pub.data[pub.size] = 0;

		VFS_CLOSE(privf);
		VFS_CLOSE(pubf);
	}

	//FIXME: extend the expiration time if its old?

	if (priv.size && pub.size)
	{	//submit them to gnutls
		ret = qgnutls_certificate_set_x509_key_mem(cred, &pub, &priv, GNUTLS_X509_FMT_PEM);
		if (ret == GNUTLS_E_CERTIFICATE_KEY_MISMATCH)
			Con_Printf(CON_ERROR"gnutls_certificate_set_x509_key_mem failed: GNUTLS_E_CERTIFICATE_KEY_MISMATCH\n");
		else if (ret < 0)
			Con_Printf(CON_ERROR"gnutls_certificate_set_x509_key_mem failed: %i\n", ret);
	}
	else
		Con_Printf(CON_ERROR"Unable to read/generate cert ('-certhost HOSTNAME' commandline arguments to autogenerate one)\n");

	memset(priv.data, 0, priv.size);//just in case. FIXME: we didn't scrub the filesystem code. libc has its own caches etc. lets hope that noone comes up with some way to scrape memory remotely (although if they can inject code then we've lost either way so w/e)
	if (priv.data)
		(*qgnutls_free)(priv.data);
	if (pub.data)
		(*qgnutls_free)(pub.data);

	return ret>=0;
}

qboolean SSL_InitGlobal(qboolean isserver)
{
	int err;
	isserver = !!isserver;
	if (COM_CheckParm("-notls"))
		return false;
#ifdef LOADERTHREAD
	if (com_resourcemutex)
		Sys_LockMutex(com_resourcemutex);
#endif
	if (!gnutls.initstatus[isserver])
	{
		if (!Init_GNUTLS())
		{
#ifdef LOADERTHREAD
			if (com_resourcemutex)
				Sys_UnlockMutex(com_resourcemutex);
#endif
			Con_Printf("GnuTLS "GNUTLS_VERSION" library not available.\n");
			return false;
		}
		gnutls.initstatus[isserver] = true;
		qgnutls_global_init ();

#ifdef HAVE_DTLS
		if (isserver)
			qgnutls_key_generate(&cookie_key, GNUTLS_COOKIE_KEY_SIZE);
#endif


#ifdef USE_ANON
		qgnutls_anon_allocate_client_credentials (&anoncred[isserver]);
#else

		qgnutls_certificate_allocate_credentials (&xcred[isserver]);

#ifdef GNUTLS_HAVE_SYSTEMTRUST
		err = qgnutls_certificate_set_x509_system_trust (xcred[isserver]);
		if (err <= 0)
			Con_Printf(CON_ERROR"gnutls_certificate_set_x509_system_trust: error %i.\n", err);
#else
		qgnutls_certificate_set_x509_trust_file (xcred[isserver], CAFILE, GNUTLS_X509_FMT_PEM);
#endif

#ifdef LOADERTHREAD
		if (com_resourcemutex)
			Sys_UnlockMutex(com_resourcemutex);
#endif
		if (isserver)
		{
#if 1
			if (!SSL_LoadPrivateCert(xcred[isserver]))
				servercertfail = true;
#else
			int ret = -1;
			char keyfile[MAX_OSPATH];
			char certfile[MAX_OSPATH];
			*keyfile = *certfile = 0;
			if (FS_SystemPath("key.pem", FS_ROOT, keyfile, sizeof(keyfile)))
				if (FS_SystemPath("cert.pem", FS_ROOT, certfile, sizeof(certfile)))
					ret = qgnutls_certificate_set_x509_key_file(xcred[isserver], certfile, keyfile, GNUTLS_X509_FMT_PEM);
			if (ret < 0)
			{
				Con_Printf(CON_ERROR"No certificate or key was found in %s and %s\n", certfile, keyfile);
				gnutls.initstatus[isserver] = -1;
			}
#endif
		}
		else
		{
			qgnutls_certificate_set_verify_function (xcred[isserver], SSL_CheckCert);
//			qgnutls_certificate_set_retrieve_function (xcred[isserver], SSL_FindClientCert);
		}
#endif
	}
	else
	{
#ifdef LOADERTHREAD
		if (com_resourcemutex)
			Sys_UnlockMutex(com_resourcemutex);
#endif
	}

	if (gnutls.initstatus[isserver] < 0)
		return false;
	return true;
}

void GnuTLS_Shutdown(void)
{
	int isserver;
	for (isserver = 0; isserver < 2; isserver++)
		if (gnutls.initstatus[isserver])
		{
			qgnutls_certificate_free_credentials(xcred[isserver]);
			xcred[isserver] = NULL;
			gnutls.initstatus[isserver] = false;

			qgnutls_global_deinit();	//refcounted.
		}
#ifdef HAVE_DTLS
	if (cookie_key.data)
	{
		(*qgnutls_free)(cookie_key.data);
		memset(&cookie_key, 0, sizeof(cookie_key));
	}
#endif
#ifdef GNUTLS_DYNAMIC
	if (gnutls.hmod)
		Sys_CloseLibrary(gnutls.hmod);
	gnutls.hmod = NULL;
#endif
}


#ifdef HAVE_DTLS
static int GetPSKForUser(gnutls_session_t sess, const char *username, gnutls_datum_t * key)
{	//serverside. name must match what we expect (this isn't very secure), and we return the key we require for that user name.
	if (!strcmp(username, dtls_psk_user.string))
	{
		key->size = (strlen(dtls_psk_key.string)+1)/2;
		key->data = (*qgnutls_malloc)(key->size);
		key->size = Base16_DecodeBlock(dtls_psk_key.string, key->data, key->size);
		return 0;
	}
	return -1;
}
static int GetPSKForServer(gnutls_session_t sess, char **username, gnutls_datum_t *key)
{	//clientside. return the appropriate username for the hint, along with the matching key.
	//this could be made more fancy with a database, but we'll keep it simple with cvars.
	const char *svhint = qgnutls_psk_client_get_hint(sess);

	if (!svhint)
		svhint = "";

	if ((!*dtls_psk_hint.string&&*dtls_psk_user.string) || (*dtls_psk_hint.string&&!strcmp(svhint, dtls_psk_hint.string)))
	{	//okay, hints match (or ours is unset), report our user as appropriate.
#ifndef NOLEGACY
		if (*svhint)
		{
			//Try to avoid crashing QE servers by recognising its hint and blocking it when the hashes of the user+key are wrong.
			if (CalcHashInt(&hash_sha1, svhint, strlen(svhint)) == 0xb6c27b61)
			{
				if (strcmp(svhint, dtls_psk_user.string) || CalcHashInt(&hash_sha1, dtls_psk_key.string, strlen(dtls_psk_key.string)) != 0x3dd348e4)
				{
					Con_Printf(CON_WARNING	"Possible QEx Server, please set your ^[%s\\type\\%s^] and ^[%s\\type\\%s^] cvars correctly, their current values are likely to crash the server.\n", dtls_psk_user.name,dtls_psk_user.name, dtls_psk_key.name,dtls_psk_key.name);
					return -1; //don't report anything.
				}
			}
		}
#endif

		*username = strcpy((*qgnutls_malloc)(strlen(dtls_psk_user.string)+1), dtls_psk_user.string);

		key->size = (strlen(dtls_psk_key.string)+1)/2;
		key->data = (*qgnutls_malloc)(key->size);
		key->size = Base16_DecodeBlock(dtls_psk_key.string, key->data, key->size);
		return 0;
	}
	else if (!*dtls_psk_user.string && !*dtls_psk_hint.string)
		Con_Printf(CON_ERROR"Server requires a Pre-Shared Key (hint: \"%s\"). Please set %s, %s, and %s accordingly.\n", svhint, dtls_psk_hint.name, dtls_psk_user.name, dtls_psk_key.name);
	else
		Con_Printf(CON_ERROR"Server requires different Pre-Shared Key credentials (hint: \"%s\", expected \"%s\"). Please set %s, %s, and %s accordingly.\n", svhint, dtls_psk_hint.string, dtls_psk_hint.name, dtls_psk_user.name, dtls_psk_key.name);
	return -1;
}
#endif
static qboolean SSL_InitConnection(gnutlsfile_t *newf, qboolean isserver, qboolean datagram)
{
	// Initialize TLS session
	qgnutls_init (&newf->session, ((newf->certcred)?GNUTLS_FORCE_CLIENT_CERT:0)
									|GNUTLS_NONBLOCK
									|(isserver?GNUTLS_SERVER:GNUTLS_CLIENT)
									|(datagram?GNUTLS_DATAGRAM:0));

	if (!isserver)
		qgnutls_server_name_set(newf->session, GNUTLS_NAME_DNS, newf->certname, strlen(newf->certname));
	qgnutls_session_set_ptr(newf->session, newf);

	if (newf->certcred)
	{
		qgnutls_certificate_server_set_request(newf->session, GNUTLS_CERT_REQUIRE);	//we will need to validate their fingerprint.
		qgnutls_credentials_set (newf->session, GNUTLS_CRD_CERTIFICATE, newf->certcred);
		qgnutls_set_default_priority (newf->session);
	}
	else
	{
#ifdef USE_ANON
		//qgnutls_kx_set_priority (newf->session, kx_prio);
		qgnutls_credentials_set (newf->session, GNUTLS_CRD_ANON, anoncred[isserver]);
#else
#ifdef HAVE_DTLS

#if defined(MASTERONLY)
		qgnutls_certificate_server_set_request(newf->session, GNUTLS_CERT_IGNORE);	//don't request a cert. masters don't really need it and chrome bugs out if you connect to a websocket server that offers for the client to provide one. chrome users will just have to stick to webrtc.
#else
		qgnutls_certificate_server_set_request(newf->session, GNUTLS_CERT_REQUEST);	//request a cert, we'll use it for fingerprints.
#endif

		if (datagram && !isserver)
		{	//do psk as needed. we can still do the cert stuff if the server isn't doing psk.
			gnutls_psk_client_credentials_t pskcred;
			qgnutls_psk_allocate_client_credentials(&pskcred);
			qgnutls_psk_set_client_credentials_function(pskcred, GetPSKForServer);

			qgnutls_set_default_priority_append (newf->session, "+ECDHE-PSK:+DHE-PSK:+PSK", NULL, 0);
			qgnutls_credentials_set(newf->session, GNUTLS_CRD_PSK, pskcred);
		}
		else if (datagram && isserver && (*dtls_psk_user.string || servercertfail))
		{	//offer some arbitrary PSK for dtls clients.
			gnutls_psk_server_credentials_t pskcred;
			qgnutls_psk_allocate_server_credentials(&pskcred);
			qgnutls_psk_set_server_credentials_function(pskcred, GetPSKForUser);
			if (*dtls_psk_hint.string)
				qgnutls_psk_set_server_credentials_hint(pskcred, dtls_psk_hint.string);

			qgnutls_set_default_priority_append (newf->session, ("-KX-ALL:+ECDHE-PSK:+DHE-PSK:+PSK")+(servercertfail?0:8), NULL, 0);
			qgnutls_credentials_set(newf->session, GNUTLS_CRD_PSK, pskcred);
		}
		else
#endif
		{
			// Use default priorities for regular tls sessions
			qgnutls_set_default_priority (newf->session);
		}
#endif
		if (xcred[isserver])
			qgnutls_credentials_set (newf->session, GNUTLS_CRD_CERTIFICATE, xcred[isserver]);
	}

	// tell gnutls how to send/receive data
	qgnutls_transport_set_ptr (newf->session, newf);
	qgnutls_transport_set_push_function(newf->session, datagram?DTLS_Push:SSL_Push);
	//qgnutls_transport_set_vec_push_function(newf->session, SSL_PushV);
	qgnutls_transport_set_pull_function(newf->session, datagram?DTLS_Pull:SSL_Pull);
#ifdef HAVE_DTLS
	if (datagram)
		qgnutls_transport_set_pull_timeout_function(newf->session, DTLS_Pull_Timeout);
#endif

	newf->handshaking = true;

	return true;
}

static vfsfile_t *GNUTLS_OpenVFS(const char *hostname, vfsfile_t *source, qboolean isserver)
{
	gnutlsfile_t *newf;

	if (!source)
		return NULL;

	if (!SSL_InitGlobal(isserver))
		newf = NULL;
	else
		newf = Z_Malloc(sizeof(*newf));
	if (!newf)
	{
		return NULL;
	}
	newf->funcs.Close = SSL_CloseFile;
	newf->funcs.Flush = NULL;
	newf->funcs.GetLen = SSL_GetLen;
	newf->funcs.ReadBytes = SSL_Read;
	newf->funcs.WriteBytes = SSL_Write;
	newf->funcs.Seek = SSL_Seek;
	newf->funcs.Tell = SSL_Tell;
	newf->funcs.seekstyle = SS_UNSEEKABLE;

	SSL_SetCertificateName(newf, hostname);

	if (!SSL_InitConnection(newf, isserver, false))
	{
		VFS_CLOSE(&newf->funcs);
		return NULL;
	}
	newf->stream = source;

	return &newf->funcs;
}

static int GNUTLS_GetChannelBinding(vfsfile_t *vf, qbyte *binddata, size_t *bindsize)
{
	gnutls_datum_t cb;
	gnutlsfile_t *f = (gnutlsfile_t*)vf;
	if (vf->Close != SSL_CloseFile)
		return -1;	//err, not a gnutls connection.

	if (qgnutls_session_channel_binding(f->session, GNUTLS_CB_TLS_UNIQUE, &cb))
	{	//error of some kind
		//if the error is because of the other side not supporting it, then we should return 0 here.
		return -1;
	}
	else
	{
		if (cb.size > *bindsize)
			return 0;	//overflow
		*bindsize = cb.size;
		memcpy(binddata, cb.data, cb.size);
		return 1;
	}
}

//crypto: generates a signed blob
#ifdef HAVE_DTLS
static int GNUTLS_GenerateSignature(const qbyte *hashdata, size_t hashsize, qbyte *signdata, size_t signsizemax)
{
	gnutls_datum_t hash = {(qbyte*)hashdata, hashsize};
	gnutls_datum_t sign = {NULL, 0};

	gnutls_certificate_credentials_t cred;
	if (Init_GNUTLS())
	{
		qgnutls_certificate_allocate_credentials (&cred);
		if (SSL_LoadPrivateCert(cred))
		{
			gnutls_x509_privkey_t xkey;
			gnutls_privkey_t privkey;
			qgnutls_privkey_init(&privkey);
			qgnutls_certificate_get_x509_key(cred, 0, &xkey);
			qgnutls_privkey_import_x509(privkey, xkey, 0);

			qgnutls_privkey_sign_hash(privkey, GNUTLS_DIG_SHA512, 0, &hash, &sign);
			qgnutls_privkey_deinit(privkey);
		}
		else
			sign.size = 0;
		qgnutls_certificate_free_credentials(cred);
	}
	else
		Con_Printf("Unable to init gnutls\n");
	memcpy(signdata, sign.data, sign.size);
	return sign.size;
}
#else
#define GNUTLS_GenerateSignature NULL
#endif

//crypto: verifies a signed blob matches an authority's public cert. windows equivelent https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program-signing-a-hash-and-verifying-the-hash-signature
static enum hashvalidation_e GNUTLS_VerifyHash(const qbyte *hashdata, size_t hashsize, const qbyte *pubkeydata, size_t pubkeysize, const qbyte *signdata, size_t signsize)
{
	gnutls_datum_t hash = {(qbyte*)hashdata, hashsize};
	gnutls_datum_t sign = {(qbyte*)signdata, signsize};
	int r;

	gnutls_datum_t rawcert = {(qbyte*)pubkeydata, pubkeysize};
#if 1
	gnutls_pubkey_t pubkey;
	gnutls_x509_crt_t cert;

	if (!rawcert.data)
		return VH_AUTHORITY_UNKNOWN;
	if (!Init_GNUTLS())
		return VH_UNSUPPORTED;

	qgnutls_pubkey_init(&pubkey);
	qgnutls_x509_crt_init(&cert);
	qgnutls_x509_crt_import(cert, &rawcert, GNUTLS_X509_FMT_PEM);

	qgnutls_pubkey_import_x509(pubkey, cert, 0);
#else
	qgnutls_pubkey_import(pubkey, rawcert, GNUTLS_X509_FMT_PEM);
#endif

	r = qgnutls_pubkey_verify_hash2(pubkey, GNUTLS_SIGN_RSA_SHA512, 0, &hash, &sign);
	qgnutls_x509_crt_deinit(cert);
	qgnutls_pubkey_deinit(pubkey);
	if (r < 0)
	{
		if (r == GNUTLS_E_PK_SIG_VERIFY_FAILED)
		{
			Con_Printf(CON_ERROR"GNUTLS_VerifyHash: GNUTLS_E_PK_SIG_VERIFY_FAILED!\n");
			return VH_INCORRECT;
		}
		else if (r == GNUTLS_E_INSUFFICIENT_SECURITY)
		{
			Con_Printf(CON_ERROR"GNUTLS_VerifyHash: GNUTLS_E_INSUFFICIENT_SECURITY\n");
			return VH_AUTHORITY_UNKNOWN;	//should probably be incorrect or something, but oh well
		}
		return VH_INCORRECT;
	}
	else
		return VH_CORRECT;
}

#ifdef HAVE_DTLS

static void GNUDTLS_DestroyContext(void *ctx)
{
	SSL_Close(ctx);
	Z_Free(ctx);
}
static void *GNUDTLS_CreateContext(const dtlscred_t *credinfo, void *cbctx, neterr_t(*push)(void *cbctx, const qbyte *data, size_t datasize), qboolean isserver)
{
	gnutlsfile_t *newf;

	if (!SSL_InitGlobal(isserver))
		newf = NULL;
	else
		newf = Z_Malloc(sizeof(*newf));
	if (!newf)
		return NULL;
	newf->datagram = true;
	newf->cbctx = cbctx;
	newf->cbpush = push;
	newf->challenging = isserver;

//	Sys_Printf("DTLS_CreateContext: server=%i\n", isserver);

	if (credinfo && ((credinfo->local.cert && credinfo->local.key) || credinfo->peer.hash))
	{
		qgnutls_certificate_allocate_credentials (&newf->certcred);
		if (credinfo->local.cert && credinfo->local.key)
		{
			gnutls_datum_t	pub = {credinfo->local.cert, credinfo->local.certsize},
							priv = {credinfo->local.key, credinfo->local.keysize};
			qgnutls_certificate_set_x509_key_mem(newf->certcred, &pub, &priv, GNUTLS_X509_FMT_DER);
		}

		if (credinfo->peer.hash)
		{
			newf->peerhashfunc = credinfo->peer.hash;
			memcpy(newf->peerdigest, credinfo->peer.digest, newf->peerhashfunc->digestsize);
			qgnutls_certificate_set_verify_function (newf->certcred, SSL_CheckFingerprint);
		}
	}

	SSL_SetCertificateName(newf, credinfo?credinfo->peer.name:NULL);

	if (!SSL_InitConnection(newf, isserver, true))
	{
		SSL_Close(&newf->funcs);
		Z_Free(newf);
		return NULL;
	}

	return newf;
}

static neterr_t GNUDTLS_Transmit(void *ctx, const qbyte *data, size_t datasize)
{
	int ret;
	gnutlsfile_t *f = (gnutlsfile_t *)ctx;

	if (f->challenging)
		return NETERR_CLOGGED;
	if (f->handshaking)
	{
		ret = SSL_DoHandshake(f);
		if (!ret)
			return NETERR_CLOGGED;
		if (ret < 0)
			return NETERR_DISCONNECTED;
	}

	if (!datasize)
		return NETERR_SENT;

	ret = qgnutls_record_send(f->session, data, datasize);
	if (ret < 0)
	{
		if (ret == GNUTLS_E_LARGE_PACKET)
			return NETERR_MTU;

		if (qgnutls_error_is_fatal(ret))
			return NETERR_DISCONNECTED;
		return NETERR_CLOGGED;
	}
	return NETERR_SENT;
}

static neterr_t GNUDTLS_Received(void *ctx, sizebuf_t *message)
{
	int ret;
	gnutlsfile_t *f = (gnutlsfile_t *)ctx;

	if (f->challenging)
	{
		size_t asize;
		safeswitch (net_from.type)
		{
		case NA_LOOPBACK:	asize = 0; break;
		case NA_IP:			asize = sizeof(net_from.address.ip);	break;
		case NA_IPV6:		asize = sizeof(net_from.address.ip6);	break;
		case NA_IPX:		asize = sizeof(net_from.address.ipx);	break;
#ifdef UNIXSOCKETS
		case NA_UNIX:		asize = (qbyte*)&net_from.address.un.path[net_from.address.un.len]-(qbyte*)&net_from.address; break;	//unlikely to be spoofed...
#endif
#ifdef IRCCONNECT
		//case NA_IRC:
#endif
#ifdef HAVE_WEBSOCKCL
		//case NA_WEBSOCKET:	//basically web browser.
#endif
#ifdef SUPPORT_ICE
		case NA_ICE:		asize = strlen(net_from.address.icename);	break;
#endif
		case NA_INVALID:
		safedefault: return NETERR_NOROUTE;
		}

		memset(&f->prestate, 0, sizeof(f->prestate));
		ret = qgnutls_dtls_cookie_verify(&cookie_key,
				&net_from.address, asize,
				message->data, message->cursize,
				&f->prestate);

		if (ret == GNUTLS_E_BAD_COOKIE)
		{
			qgnutls_dtls_cookie_send(&cookie_key,
					&net_from.address, asize,
					&f->prestate,
					(gnutls_transport_ptr_t)f, DTLS_Push);
			return NETERR_CLOGGED;
		}
		else if (ret < 0)
			return NETERR_NOROUTE;
		f->challenging = false;

		qgnutls_dtls_prestate_set(f->session, &f->prestate);
		qgnutls_dtls_set_mtu(f->session, 1440);

		f->handshaking = true;
	}

	f->readdata = message->data;
	f->readsize = message->cursize;

	if (f->handshaking)
	{
		ret = SSL_DoHandshake(f);
		if (ret <= 0)
			f->readsize = 0;
		if (!ret)
			return NETERR_CLOGGED;
		if (ret < 0)
			return NETERR_DISCONNECTED;
	}

	ret = qgnutls_record_recv(f->session, message->data, message->maxsize);
//Sys_Printf("DTLS_Received returned %i of %i\n", ret, f->readsize);
	f->readsize = 0;
	if (ret <= 0)
	{
		if (!ret)
		{
//			Sys_Printf("DTLS_Received peer terminated connection\n");
			return NETERR_DISCONNECTED;
		}
		if (qgnutls_error_is_fatal(ret))
		{
			if (ret == GNUTLS_E_FATAL_ALERT_RECEIVED)
				Con_DPrintf(CON_ERROR"GNUDTLS_Received: fatal alert %s\n", qgnutls_alert_get_name(qgnutls_alert_get(f->session)));
			else
				Con_DPrintf(CON_ERROR"GNUDTLS_Received fatal error %s\n", qgnutls_strerror(ret));
			return NETERR_DISCONNECTED;
		}
		if (ret == GNUTLS_E_WARNING_ALERT_RECEIVED)
			Con_DPrintf(CON_WARNING"GNUDTLS_Received: alert %s\n", qgnutls_alert_get_name(qgnutls_alert_get(f->session)));
//		Sys_Printf("DTLS_Received temp error\n");
		return NETERR_CLOGGED;
	}
	message->cursize = ret;
	message->data[ret] = 0;
//	Sys_Printf("DTLS_Received returned %s\n", data);
	return NETERR_SENT;
}

static qboolean GNUDTLS_CheckConnection(void *cbctx, void *peeraddr, size_t peeraddrsize, void *indata, size_t insize, neterr_t(*push)(void *cbctx, const qbyte *data, size_t datasize), void (*EstablishTrueContext)(void **cbctx, void *state))
{	//called when we got a possibly-dtls packet out of the blue.
	gnutlsfile_t *f;
	int ret;
	gnutls_dtls_prestate_st prestate;

	memset(&prestate, 0, sizeof(prestate));
	ret = qgnutls_dtls_cookie_verify(&cookie_key,
			peeraddr, peeraddrsize,
			indata, insize,
			&prestate);

	if (ret == GNUTLS_E_BAD_COOKIE)
	{	//some sort of handshake with a bad/unknown cookie. send them a real one.
		gnutlsfile_t f;
		f.cbctx = cbctx;
		f.cbpush = push;
		f.session = NULL;

		qgnutls_dtls_cookie_send(&cookie_key,
				peeraddr, peeraddrsize,
				&prestate,
				(gnutls_transport_ptr_t)&f, DTLS_Push);
		return true;
	}
	else if (ret < 0)
		return false;	//dunno... might still be dtls but doesn't seem to be needed... oh well...

	//allocate our context
	f = GNUDTLS_CreateContext(NULL, cbctx, push, true);
	if (!f)
	{
		Con_Printf("GNUDTLS_CreateContext: failed\n");
		return false;
	}

	//tell caller that this is an actual valid connection
	EstablishTrueContext(&f->cbctx, f);
	if (!f->cbctx)
		return true;

	//we're done with the challenge stuff
	f->challenging = false;

	//and this is the result...
	qgnutls_dtls_prestate_set(f->session, &prestate);
	qgnutls_dtls_set_mtu(f->session, 1400);

	//still need to do the whole certificate thing though.
	f->handshaking = true;
	return true;
}

static neterr_t GNUDTLS_Timeouts(void *ctx)
{
	gnutlsfile_t *f = (gnutlsfile_t *)ctx;
	int ret;
	if (f->challenging)
		return NETERR_CLOGGED;
	if (f->handshaking)
	{
		f->readsize = 0;
		ret = SSL_DoHandshake(f);
		f->readsize = 0;
		if (!ret)
			return NETERR_CLOGGED;
		if (ret < 0)
			return NETERR_DISCONNECTED;

//		Sys_Printf("handshaking over?\n");
	}
	return NETERR_SENT;
}

static int GNUDTLS_GetPeerCertificate(void *ctx, enum certprops_e prop, char *out, size_t outsize)
{
	gnutlsfile_t *f = (gnutlsfile_t *)ctx;
	if (f && (f->challenging || f->handshaking))
		return -1;	//no cert locked down yet...
	safeswitch(prop)
	{
	case QCERT_ISENCRYPTED:
		return 0;	//well, should be...
	case QCERT_PEERSUBJECT:
		{
			unsigned int certcount;
			const gnutls_datum_t *const certlist = qgnutls_certificate_get_peers(f->session, &certcount);
			if (certlist)
			{
				gnutls_x509_crt_t cert = NULL;
				gnutls_datum_t dn={NULL};
				qgnutls_x509_crt_init(&cert);
				qgnutls_x509_crt_import(cert, certlist, GNUTLS_X509_FMT_DER);
				qgnutls_x509_crt_get_dn3(cert, &dn, 0);
				if (dn.size >= outsize)
					dn.size = -1;	//too big...
				else
				{
					memcpy(out, dn.data, dn.size);
					out[dn.size] = 0;
				}
				(*qgnutls_free)(dn.data);
				qgnutls_x509_crt_deinit(cert);
				return (int)dn.size;
			}
		}
		return -1;
	case QCERT_PEERCERTIFICATE:
		{
			unsigned int certcount;
			const gnutls_datum_t *const certlist = qgnutls_certificate_get_peers(f->session, &certcount);
			if (certlist && certlist->size <= outsize)
			{
				memcpy(out, certlist->data, certlist->size);
				return certlist->size;
			}
		}
		return -1;
	case QCERT_LOCALCERTIFICATE:
		{
			const gnutls_datum_t *cert;
			gnutls_datum_t d;

			if (f)
				cert = qgnutls_certificate_get_ours(f->session);
			else	//no actual context? get our default dtls server cert.
			{
				qgnutls_certificate_get_crt_raw (xcred[true], 0/*first chain*/, 0/*primary one*/, &d);
				cert = &d;
			}
			if (cert->size <= outsize)
			{
				memcpy(out, cert->data, cert->size);
				return cert->size;
			}
		}
		return -1;
	case QCERT_LOBBYSTATUS:
	case QCERT_LOBBYSENDCHAT:
		return -1;
	safedefault:
		return -1; //dunno what you want from me.
	}
}

static qboolean GNUDTLS_GenTempCertificate(const char *subject, struct dtlslocalcred_s *qcred)
{
	gnutls_datum_t priv = {NULL}, pub = {NULL};
	gnutls_x509_privkey_t key;
	gnutls_x509_crt_t cert;
	char serial[64];
	char randomsub[32+1];
	const char *errstr;
	gnutls_pk_algorithm_t privalgo = GNUTLS_PK_RSA;
	int ret;

	qgnutls_x509_privkey_init(&key);
	ret = qgnutls_x509_privkey_generate(key, privalgo, qgnutls_sec_param_to_pk_bits(privalgo, GNUTLS_SEC_PARAM_HIGH), 0);
	if (ret < 0)
		Con_Printf(CON_ERROR"gnutls_x509_privkey_generate failed: %i\n", ret);
	ret = qgnutls_x509_privkey_export2(key, GNUTLS_X509_FMT_DER, &priv);
	if (ret < 0)
		Con_Printf(CON_ERROR"gnutls_x509_privkey_export2 failed: %i\n", ret);

	//stoopid browsers insisting that serial numbers are different even on throw-away self-signed certs.
	//we should probably just go and make our own root ca/master. post it a cert and get a signed one (with sequential serial) back or something.
	//we'll probably want something like that for client certs anyway, for stat tracking.
	Q_snprintfz(serial, sizeof(serial), "%u", (unsigned)time(NULL));

	qgnutls_x509_crt_init(&cert);
	qgnutls_x509_crt_set_version(cert, 1);
	qgnutls_x509_crt_set_activation_time(cert, time(NULL)-1);
	qgnutls_x509_crt_set_expiration_time(cert, time(NULL)+(time_t)10*365*24*60*60);
	qgnutls_x509_crt_set_serial(cert, serial, strlen(serial));
//	qgnutls_x509_crt_set_key_usage(cert, GNUTLS_KEY_DIGITAL_SIGNATURE);
	if (!subject)
	{
		qbyte tmp[16];
		Sys_RandomBytes(tmp, sizeof(tmp));
		randomsub[Base16_EncodeBlock(tmp, sizeof(tmp), randomsub, sizeof(randomsub)-1)] = 0;
		subject = randomsub;
	}

	if (qgnutls_x509_crt_set_dn(cert, va("CN=%s", subject), &errstr) < 0)
		Con_Printf(CON_ERROR"gnutls_x509_crt_set_dn failed: %s\n", errstr);
	if (qgnutls_x509_crt_set_issuer_dn(cert, va("CN=%s", subject), &errstr) < 0)
		Con_Printf(CON_ERROR"gnutls_x509_crt_set_issuer_dn failed: %s\n", errstr);
//	qgnutls_x509_crt_set_key_usage(cert, GNUTLS_KEY_KEY_ENCIPHERMENT|GNUTLS_KEY_DATA_ENCIPHERMENT|);

	qgnutls_x509_crt_set_key(cert, key);

	/*sign it with our private key*/
	{
		gnutls_privkey_t akey;
		qgnutls_privkey_init(&akey);
		qgnutls_privkey_import_x509(akey, key, GNUTLS_PRIVKEY_IMPORT_COPY);
		ret = qgnutls_x509_crt_privkey_sign(cert, cert, akey, GNUTLS_DIG_SHA256, 0);
		if (ret < 0)
			Con_Printf(CON_ERROR"gnutls_x509_crt_privkey_sign failed: %i\n", ret);
		qgnutls_privkey_deinit(akey);
	}
	ret = qgnutls_x509_crt_export2(cert, GNUTLS_X509_FMT_DER, &pub);
	qgnutls_x509_crt_deinit(cert);
	qgnutls_x509_privkey_deinit(key);
	if (ret < 0)
		Con_Printf(CON_ERROR"gnutls_x509_crt_export2 failed: %i\n", ret);

	//okay, we have them in memory, make sure the rest of the engine can play with it.
	qcred->certsize = pub.size;
	memcpy(qcred->cert = Z_Malloc(pub.size), pub.data, pub.size);

	qcred->keysize = priv.size;
	memcpy(qcred->key = Z_Malloc(priv.size), priv.data, priv.size);

	(*qgnutls_free)(priv.data);
	(*qgnutls_free)(pub.data);

	return true;
}
static const dtlsfuncs_t dtlsfuncs_gnutls =
{
	GNUDTLS_CreateContext,
	GNUDTLS_CheckConnection,
	GNUDTLS_DestroyContext,
	GNUDTLS_Transmit,
	GNUDTLS_Received,
	GNUDTLS_Timeouts,
	GNUDTLS_GetPeerCertificate,
	GNUDTLS_GenTempCertificate
};
static const dtlsfuncs_t *GNUDTLS_InitServer(void)
{
	if (!SSL_InitGlobal(true))
		return NULL;	//unable to init a server certificate. don't allow dtls to init.
	if (servercertfail && !*dtls_psk_user.string)	//FIXME: with ICE connections we'll be using temporary certs anyway.
		return NULL;
	return &dtlsfuncs_gnutls;
}
static const dtlsfuncs_t *GNUDTLS_InitClient(void)
{
	if (!SSL_InitGlobal(false))
		return NULL;
	return &dtlsfuncs_gnutls;
}
#else
#define GNUDTLS_InitServer NULL
#define GNUDTLS_InitClient NULL
#endif

ftecrypto_t crypto_gnutls =
{
	"GNUTLS",
	GNUTLS_OpenVFS,
	GNUTLS_GetChannelBinding,
	GNUDTLS_InitClient,
	GNUDTLS_InitServer,
	GNUTLS_VerifyHash,
	GNUTLS_GenerateSignature,
};

#else
#warning "GNUTLS version is too old (3.0+ required). Please clean and then recompile with CFLAGS=-DNO_GNUTLS"

ftecrypto_t crypto_gnutls;
qboolean SSL_InitGlobal(qboolean isserver) {return false;}
#endif
#endif

