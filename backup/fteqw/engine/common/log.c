// log.c: handles console logging functions and cvars

#include "quakedef.h"

// cvar callbacks
static void QDECL Log_Dir_Callback (struct cvar_s *var, char *oldvalue);
static void QDECL Log_Name_Callback (struct cvar_s *var, char *oldvalue);

// cvars
#define CONLOGGROUP "Console logging"
cvar_t		log_enable[LOG_TYPES]	= {	CVARF("log_enable", "0", CVAR_NOTFROMSERVER),
										CVARF("log_enable_players", "0", CVAR_NOTFROMSERVER),
										CVARF("log_enable_rcon", "1", CVAR_NOTFROMSERVER)
								};
cvar_t		log_name[LOG_TYPES] = { CVARFC("log_name", "qconsole", CVAR_NOTFROMSERVER, Log_Name_Callback),
									CVARFC("log_name_players", "players", CVAR_NOTFROMSERVER, Log_Name_Callback),
									CVARFC("log_name_rcon", "rcon", CVAR_NOTFROMSERVER, Log_Name_Callback)};
cvar_t		log_dir_var = CVARAFC("log_dir", "", "sv_logdir", CVAR_NOTFROMSERVER, Log_Dir_Callback);
cvar_t		log_readable = CVARFD("log_readable", "7", CVAR_NOTFROMSERVER, "Bitfield describing what to convert/strip. If 0, exact byte representation will be used.\n&1: Dequakify text.\n&2: Strip special markup.\n&4: Strip ansi control codes.");
cvar_t		log_developer = CVARFD("log_developer", "0", CVAR_NOTFROMSERVER, "Enables logging of console prints when set to 1. Otherwise unimportant messages will not fill up your log files.");
cvar_t		log_rotate_files = CVARF("log_rotate_files", "0", CVAR_NOTFROMSERVER);
cvar_t		log_rotate_size = CVARF("log_rotate_size", "131072", CVAR_NOTFROMSERVER);
cvar_t		log_timestamps = CVARF("log_timestamps", "1", CVAR_NOTFROMSERVER);
#ifdef _WIN32
cvar_t		log_dosformat = CVARF("log_dosformat", "1", CVAR_NOTFROMSERVER);
#else
cvar_t		log_dosformat = CVARF("log_dosformat", "0", CVAR_NOTFROMSERVER);
#endif
qboolean	log_newline[LOG_TYPES];

#ifdef IPLOG
cvar_t		iplog_autodump = CVARFD("ipautodump", "0", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "Enables dumping the 'iplog.txt' file, which contains a log of usernames seen for a given IP, which is useful for detecting fake-nicks.");
#endif

static char log_dir[MAX_OSPATH];
static enum fs_relative log_root = FS_GAMEONLY;


// Log_Dir_Callback: called when a log_dir is changed
static void QDECL Log_Dir_Callback (struct cvar_s *var, char *oldvalue)
{
	char *t = var->string;
	char *e = t + (*t?strlen(t):0);

	// sanity check for directory. // is equivelent to /../ on some systems, so make sure that can't be used either. : is for drives on windows or amiga, or alternative thingies on windows, so block thoses completely.
	if (strstr(t, "..") || strstr(t, ":") || *t == '/' || *t == '\\' || *e == '/' || *e == '\\' || strstr(t, "//") || strstr(t, "\\\\"))
	{
		Con_Printf(CON_NOTICE "%s forced to default due to invalid characters.\n", var->name);
		// recursion is avoided by assuming the default value is sane
		Cvar_ForceSet(var, var->enginevalue);
	}

	if (!strncmp(var->string, "./", 2)||!strncmp(var->string, ".\\", 2))
	{
		strcpy(log_dir, var->string+2);
		log_root = FS_ROOT;
	}
	else
	{
		strcpy(log_dir, var->string);
		log_root = FS_GAMEONLY;
	}
}

// Log_Name_Callback: called when a log_name is changed
static void QDECL Log_Name_Callback (struct cvar_s *var, char *oldvalue)
{
	char *t = var->string;

	// sanity check for directory
	if (strstr(t, "..") || strstr(t, ":") || strstr(t, "/") || strstr(t, "\\"))
	{
		Con_Printf(CON_NOTICE "%s forced to default due to invalid characters.\n", var->name);
		// recursion is avoided by assuming the default value is sane
		Cvar_ForceSet(var, var->enginevalue);
	}
}

// Con_Log: log string to console log
void Log_String (logtype_t lognum, const char *s)
{
	vfsfile_t *fi;
	char *f; // filename
	char *t;
	char utf8[2048];
	int i;
	char fbase[MAX_QPATH];
	char fname[MAX_QPATH];
	conchar_t cline[2048], *c;
	unsigned int u, flags;

	if (!log_enable[lognum].value)
		return;

	if (log_name[lognum].string[0])
		f = log_name[lognum].string;
	else
		f = log_name[lognum].enginevalue;

	if (!f)
		return;

	COM_ParseFunString(CON_WHITEMASK, s, cline, sizeof(cline), !(log_readable.ival & 2));
	t = utf8;
	for (c = cline; *c; )
	{
		c = Font_Decode(c, &flags, &u);
		if ((flags & CON_HIDDEN) && (log_readable.ival & 2))
			continue;
		if (log_readable.ival&1)
			u = COM_DeQuake(u);

		//at the start of a new line, we might want a timestamp (so timestamps are correct for the first char of the line, instead of the preceeding \n)
		if (log_newline[lognum])
		{
			if (log_timestamps.ival)
			{
				time_t unixtime = time(NULL);
				int bufferspace = utf8+sizeof(utf8)-1-t;
				if (bufferspace > 0)
				{
					strftime(t, bufferspace, "%Y-%m-%d %H:%M:%S ", localtime(&unixtime));
					t += strlen(t);
				}
			}
			log_newline[lognum] = false;
		}

		//make sure control codes are stripped. no exploiting xterm bugs please.
		if ((log_readable.ival & 4) && ((u < 32 && u != '\t' && u != '\n') || u == 127 || (u >= 128 && u < 128+32))) //\r is stripped too
			u = '?';
		//if dos format logs, we insert a \r before every \n (also flag next char as the start of a new line)
		if (u == '\n')
		{
			log_newline[lognum] = true;
			if (log_dosformat.ival)
				t += utf8_encode(t, '\r', utf8+sizeof(utf8)-1-t);
		}
		t += utf8_encode(t, u, utf8+sizeof(utf8)-1-t);
	}
	*t = 0;

	if (*log_dir)
		Q_snprintfz(fbase, sizeof(fname)-4, "%s/%s", log_dir, f);
	else
		Q_snprintfz(fbase, sizeof(fname)-4, "%s", f);
	Q_snprintfz(fname, sizeof(fname), "%s.log", fbase);

	// file rotation
	if (log_rotate_size.value >= 4096 && log_rotate_files.value >= 1)
	{
		int x;
		vfsfile_t *fi;

		// check file size, use x as temp
		if ((fi = FS_OpenVFS(fname, "rb", log_root)))
		{
			x = VFS_GETLEN(fi);
			VFS_CLOSE(fi);
			x += strlen(utf8); // add string size to file size to never go over
		}
		else
			x = 0;

		if (x > (int)log_rotate_size.value)
		{
			char newf[MAX_QPATH];
			char oldf[MAX_QPATH];

			i = log_rotate_files.value;

			// unlink file at the top of the chain
			Q_snprintfz(oldf, sizeof(oldf), "%s.%i.log", fbase, i);
			FS_Remove(oldf, log_root);

			// rename files through chain
			for (x = i-1; x > 0; x--)
			{
				strcpy(newf, oldf);
				Q_snprintfz(oldf, sizeof(oldf), "%s.%i.log", fbase, x);

				// check if file exists, otherwise skip
				if ((fi = FS_OpenVFS(oldf, "rb", log_root)))
					VFS_CLOSE(fi);
				else
					continue; // skip nonexistant files

				if (!FS_Rename(oldf, newf, log_root))
				{
					// rename failed, disable log and bug out
					Cvar_ForceSet(&log_enable[lognum], "0");
					Con_Printf("Unable to rotate log files. Logging disabled.\n");
					return;
				}
			}

			// TODO: option to compress file somewhere in here?
			// rename our base file, which had better exist...
			if (!FS_Rename(fname, oldf, log_root))
			{
				// rename failed, disable log and bug out
				Cvar_ForceSet(&log_enable[lognum], "0");
				Con_Printf("Unable to rename base log file. Logging disabled.\n");
				return;
			}
		}
	}

	FS_CreatePath(fname, log_root);
	if ((fi = FS_OpenVFS(fname, "ab", log_root)))
	{
		VFS_WRITE(fi, utf8, strlen(utf8));
		VFS_CLOSE(fi);
	}
	else
	{
		// write failed, bug out
		Cvar_ForceSet(&log_enable[lognum], "0");
		Con_Printf("Unable to write to log file. Logging disabled.\n");
		return;
	}
}

void Con_Log (const char *s)
{
	Log_String(LOG_CONSOLE, s);
}


#ifdef HAVE_SERVER
//still to add stuff at:
//connects
//disconnects
//kicked
void SV_LogPlayer(client_t *cl, char *msg)
{
	char line[2048];
	char remote_adr[MAX_ADR_SIZE];
	char realip_adr[MAX_ADR_SIZE];

	if (cl->protocol == SCP_BAD)
		return;	//don't log botclients

	Q_snprintfz(line, sizeof(line)-1,
			"%s\\%s\\%i\\%s\\%s\\%i\\guid\\%s",
			msg, cl->name, cl->userid,
			NET_BaseAdrToString(remote_adr, sizeof(remote_adr), &cl->netchan.remote_address), (cl->realip_status > 0 ? NET_BaseAdrToString(realip_adr, sizeof(realip_adr), &cl->realip) : "??"),
			cl->netchan.remote_address.port, cl->guid);
	InfoBuf_ToString(&cl->userinfo, line+strlen(line), sizeof(line)-1-strlen(line), NULL, NULL, NULL, NULL, NULL);
	Q_strncatz(line, "\n", sizeof(line));

	Log_String(LOG_PLAYER, line);
}
#endif


#ifdef HAVE_LEGACY
static struct {
	const char *commandname;
	const char *desc;
} legacylog[] =
{
	{"logfile",		""},
	{"logplayers",	" players"},
	{"logrcon",		" frags"},
};

static void Log_Logfile_f (void)
{	//these legacy commands are just toggles. not args (other than the commandname to know which log type to toggle)
	size_t logtype;
	const char *cmd = Cmd_Argv(0);
	for (logtype = 0; logtype < countof(legacylog); logtype++)
		if (!Q_strcasecmp(legacylog[logtype].commandname, cmd))
			break;

	if (log_enable[logtype].value)
	{
		Cvar_SetValue(&log_enable[logtype], 0);
		Con_Printf("Logging%s disabled.\n", legacylog[logtype].desc);
	}
	else
	{
		const char *f;
		char syspath[MAX_OSPATH];

		if (log_name[logtype].string[0])
			f = log_name[logtype].string;
		else
			f = log_name[logtype].enginevalue;

		if (*log_dir)
			f = va("%s/%s.log", log_dir, f);
		else
			f = va("%s.log", f);

		if (FS_DisplayPath(f, log_root, syspath, sizeof(syspath)))
			Con_Printf("%s", va("Logging%s to %s\n", legacylog[logtype].desc, syspath));
		else
			Con_Printf("%s", va("Logging%s to %s\n", legacylog[logtype].desc, f));
		Cvar_SetValue(&log_enable[logtype], 1);
	}

}
#endif

#ifdef IPLOG
/*for fuck sake, why can people still not write simple files. proquake is writing binary files as text ones. this function is to try to deal with that fuckup*/
static size_t IPLog_Read_Fucked(qbyte *file, size_t *offset, size_t totalsize, qbyte *out, size_t outsize)
{
	size_t read = 0;
	while (outsize-- > 0 && *offset < totalsize)
	{
		if (file[*offset] == '\r' && *offset+1 < totalsize && file[*offset+1] == '\n')
		{
			out[read] = '\n';
			*offset += 2;
			read += 1;
		}
		else
		{
			out[read] = file[*offset];
			*offset += 1;
			read += 1;
		}
	}
	return read;
}
/*need to make sure any 13 bytes are followed by 10s so that we don't bug out when read back in *sigh* */
static size_t IPLog_Write_Fucked(vfsfile_t *file, qbyte *out, size_t outsize)
{
	qbyte tmp[64];
	size_t write = 0;
	size_t block = 0;
	while (outsize-- > 0)
	{
		if (block >= sizeof(tmp)-4)
		{
			VFS_WRITE(file, tmp, block);
			write += block;
			block = 0;
		}
		if (*out == '\n')
			tmp[block++] = '\r';
		tmp[block++] = *out++;
	}
	if (block)
	{
		VFS_WRITE(file, tmp, block);
		write += block;
	}
	return write;
}
qboolean IPLog_Merge_File(const char *fname)
{
	char ip[MAX_ADR_SIZE];
	char name[256];
	char line[1024];
	vfsfile_t *f;
	if (!*fname)
		fname = "iplog.txt";
	f = FS_OpenVFS(fname, "rb", FS_PUBBASEGAMEONLY);
	if (!f)
		f = FS_OpenVFS(fname, "rb", FS_GAME);
	if (!f)
		return false;
	if (!Q_strcasecmp(COM_FileExtension(fname, name, sizeof(name)), "dat"))
	{	//we don't write this format because of it being limited to ipv4, as well as player name lengths
		size_t l = VFS_GETLEN(f), offset = 0;
		qbyte *ffs = malloc(l+1);
		VFS_READ(f, ffs, l);
		ffs[l] = 0;
		while (IPLog_Read_Fucked(ffs, &offset, l, line, 20) == 20)
		{	//yes, these addresses are weird.
			Q_snprintfz(ip, sizeof(ip), "%i.%i.%i.xxx", (qbyte)line[2], (qbyte)line[1], (qbyte)line[0]);
			memcpy(name, line+4, 20-4);
			name[20-4] = 0;
			IPLog_Add(ip, name);
		}
		free(ffs);
	}
	else
	{
		while (VFS_GETS(f, line, sizeof(line)-1))
		{
			//whether the name contains quotes or what is an awkward one.
			//we always write quotes (including string markup to avoid issues)
			//dp doesn't, and our parser is lazy, so its possible we'll get gibberish that way
			if (COM_ParseOut(COM_ParseOut(line, ip, sizeof(ip)), name, sizeof(name)))
				IPLog_Add(ip, name);
		}
	}
	VFS_CLOSE(f);
	return true;
}
struct iplog_entry
{
	netadr_t adr;
	netadr_t mask;
	char name[1];
} **iplog_entries;
size_t iplog_num, iplog_max;
void IPLog_Add(const char *ipstr, const char *name)
{
	size_t i;
	netadr_t a, m;
	while (*ipstr == ' ' || *ipstr == '\t')
		ipstr++;
	if (*ipstr != '[' && *ipstr < '0' && *ipstr > '9')
		return;
	if (*ipstr == '[')
		ipstr++;
	//some names are dodgy.
	if (!*name 
		//|| !Q_strcasecmp(name, /*nq default*/"player") || !Q_strcasecmp(name, /*qw default*/"unnamed")
		|| !strcmp(name, /*nq fallback*/"unconnected") || !strncmp(name, "BOT:", 4))
		return;
	memset(&a, 0, sizeof(a));
	memset(&m, 0, sizeof(m));
	if (!NET_StringToAdrMasked(ipstr, false, &a, &m))
		return;
	//might be x.y.z.w:port
	//might be x.y.z.FUCKED
	//might be x.y.z.0/24
	//might be [::]:port
	//might be [::]/bits
	//or other ways to express an ip address

	//FIXME: ignore private addresses?

	//check for dupes
	for (i = 0; i < iplog_num; i++)
	{
		if (!memcmp(&a, &iplog_entries[i]->adr, sizeof(netadr_t)) && !memcmp(&m, &iplog_entries[i]->mask, sizeof(netadr_t)) && !Q_strcasecmp(name, iplog_entries[i]->name))
			return;
	}

	//looks like its new...
	if (iplog_num == iplog_max)
		Z_ReallocElements((void**)&iplog_entries, &iplog_max, iplog_max+64, sizeof(*iplog_entries));
	iplog_entries[iplog_num] = BZ_Malloc(sizeof(struct iplog_entry) + strlen(name));
	iplog_entries[iplog_num]->adr = a;
	iplog_entries[iplog_num]->mask = m;
	strcpy(iplog_entries[iplog_num]->name, name);
	iplog_num++;
}
static void IPLog_Identify(netadr_t *adr, netadr_t *mask, char *fmt, ...)
{
	va_list		argptr;

	qboolean found = false;
	char line[256];
	size_t i;
		
	va_start(argptr, fmt);
	vsnprintf(line, sizeof(line), fmt, argptr);
	va_end(argptr);
	Con_Printf("%s: ", line);

	for (i = 0; i < iplog_num; i++)
	{
		if (NET_CompareAdrMasked(adr, &iplog_entries[i]->adr, mask?mask:&iplog_entries[i]->mask))
		{
			if (found)
				Con_Printf(", ");
			found=true;
			Con_Printf("%s", iplog_entries[i]->name);
		}
	}
	if (!found)
		Con_Printf("<no matches>");
	Con_Printf("\n");
}
#include "cl_ignore.h"
static void IPLog_Identify_f(void)
{
	const char *nameorip = Cmd_Argv(1);
	netadr_t adr, mask;
	char clean[256];
	char *endofnum;
	strtoul(nameorip, &endofnum, 10);

	if (*endofnum && NET_StringToAdrMasked (nameorip, false, &adr, &mask))
	{	//if not a single number, try to parse as an ip
		//treading carefully here, to avoid dns name lookups weirding everything out.
		IPLog_Identify(&adr, &mask, "Identity of %s", NET_AdrToStringMasked(clean, sizeof(clean), &adr, &mask));
	}
#ifdef HAVE_SERVER
	else if (sv.active)
	{	//if server is active, walk players to see if there's a name match to get their address and guess an address mask
		client_t *cl;
		int clnum = -1;
		while((cl = SV_GetClientForString(nameorip, &clnum)))
		{
			if (cl->realip_status)
			{
				IPLog_Identify(&cl->realip, NULL, "Identity of %s (real) [%s]", cl->name, NET_AdrToString(clean, sizeof(clean), &cl->realip));
				IPLog_Identify(&cl->netchan.remote_address, NULL, "Identity of %s (proxy) [%s]", cl->name, NET_AdrToString(clean, sizeof(clean), &cl->realip));
			}
			else
				IPLog_Identify(&cl->netchan.remote_address, NULL, "Identity of %s [%s]", cl->name, NET_AdrToString(clean, sizeof(clean), &cl->realip));
		}
	}
#endif
#ifdef HAVE_CLIENT
	else if (cls.state >= ca_connected)
	{	//else if client is active, walk players to see if there's a name match, to get their address+mask if known via nq hacks
		int slot;
		netadr_t adr;
		if ((slot = Player_StringtoSlot(nameorip)) < 0)
			Con_Printf("%s: no player with userid %s\n", Cmd_Argv(0), nameorip);
		else if (!*cl.players[slot].ip)
			Con_Printf("%s: ip address of %s is not known\n", Cmd_Argv(0), cl.players[slot].name);
		else
		{
			if (NET_StringToAdrMasked(cl.players[slot].ip, false, &adr, &mask))
				IPLog_Identify(&adr, &mask, "Identity of %s [%s]", cl.players[slot].name, cl.players[slot].ip);
			else
				Con_Printf("ip address of %s not known, cannot identify\n", cl.players[slot].name);
		}
	}
#endif
	else
		Con_Printf("%s: not connected, nor raw address\n", Cmd_Argv(0));
}
static int IPLog_Dump(const char *fname)
{
	size_t i;
	vfsfile_t *f;
	qbyte line[20];
	if (!*fname)
		fname = "iplog.txt";

	if (!iplog_num && !COM_FCheckExists(fname))
		return 2;	//no entries, nothing to overwrite

	f = FS_OpenVFS(fname, "wb", FS_PUBBASEGAMEONLY);
	if (!f)
		return false;
	if (!Q_strcasecmp(COM_FileExtension(fname, line, sizeof(line)), "dat"))
	{
		for (i = 0; i < iplog_num; i++)
		{
			//this shitty format supports only ipv4.
			if (iplog_entries[i]->adr.type != NA_IP)
				continue;
			line[0] = iplog_entries[i]->adr.address.ip[2];
			line[1] = iplog_entries[i]->adr.address.ip[1];
			line[2] = iplog_entries[i]->adr.address.ip[0];
			line[3] = 0;
			strncpy(line+4, iplog_entries[i]->name, sizeof(line)-4);
			IPLog_Write_Fucked(f, line, sizeof(line));	//convert \n to \r\n, to avoid fucking up any formatting with binary data (inside the address part, so *.13.10.* won't corrupt the file)
		}
	}
	else
	{
		VFS_PRINTF(f, "//generated by "FULLENGINENAME"\n");
		for (i = 0; i < iplog_num; i++)
		{
			char ip[512];
			char buf[1024];
			char buf2[1024];
			VFS_PRINTF(f, log_dosformat.value?"%s %s\r\n":"%s %s\n", COM_QuotedString(NET_AdrToStringMasked(ip, sizeof(ip), &iplog_entries[i]->adr, &iplog_entries[i]->mask), buf2, sizeof(buf2), false), COM_QuotedString(iplog_entries[i]->name, buf, sizeof(buf), false));
		}
	}
	VFS_CLOSE(f);
	return true;
}
static void IPLog_Dump_f(void)
{
	char displaypath[MAX_OSPATH];
	const char *fname = Cmd_Argv(1);
	if (FS_DisplayPath(fname, FS_GAMEONLY, displaypath, sizeof(displaypath)))
		Q_strncpyz(displaypath, fname, sizeof(displaypath));
	IPLog_Merge_File(fname);	//merge from the existing file, so that we're hopefully more robust if multiple processes are poking the same file.
	switch (IPLog_Dump(fname))
	{
	case 0:
		Con_Printf("unable to write %s\n", fname);
		break;
	default:
	case 1:
		Con_Printf("wrote %s\n", displaypath);
		break;
	case 2:
		Con_Printf("nothing to write\n");
		break;
	}
}
static void IPLog_Merge_f(void)
{
	const char *fname = Cmd_Argv(1);
	if (!IPLog_Merge_File(fname))
		Con_Printf("unable to read iplog \"%s\" for merging\n", fname);
}
#endif

#if defined(HAVE_DTLS) && defined(HAVE_CLIENT)	//requires UI prompts
struct certlog_s
{
	link_t l;
	char *hostname;
	qboolean trusted;	//when true, the user has given explicit trust
						//when false we buldozed straight through and will only complain when it changes (legacy mode).
	size_t certsize;
	qbyte cert[1];
};
#define CERTLOG_FILENAME "knowncerts.txt"
static link_t certlog;
static qboolean certlog_inited = false;
static void CertLog_Import(const char *filename);
static struct certlog_s *CertLog_Find(const char *hostname)
{
	struct certlog_s *l;
	for (l = (struct certlog_s*)certlog.next ; l != (struct certlog_s*)&certlog ; l = (struct certlog_s*)l->l.next)
	{
		if (!strcmp(l->hostname, hostname))
			return l;
	}
	return NULL;
}
static void CertLog_Update(const char *hostname, const void *cert, size_t certsize, qboolean trusted)
{
	struct certlog_s *l = CertLog_Find(hostname);
	if (l)
	{
		RemoveLink(&l->l);
		Z_Free(l);
	}
	l = Z_Malloc(sizeof(*l) + certsize + strlen(hostname));
	l->trusted = trusted;
	l->certsize = certsize;
	l->hostname = l->cert + l->certsize;
	memcpy(l->cert, cert, certsize);
	strcpy(l->hostname, hostname);
	InsertLinkAfter(&l->l, &certlog);
}
static void CertLog_Write(void)
{
	struct certlog_s *l;
	vfsfile_t *f = FS_OpenVFS(CERTLOG_FILENAME, "wb", FS_ROOT);
	if (f)
	{
		VFS_PRINTF(f, "version 1.1\n");

		for (l=(struct certlog_s*)certlog.next ; l != (struct certlog_s*)&certlog ; l = (struct certlog_s*)l->l.next)
		{
			char certhex[32768];
			size_t i;
			const char *hex = "0123456789abcdef";
			for (i = 0; i < l->certsize; i++)
			{
				certhex[i*2+0] = hex[l->cert[i]>>4];
				certhex[i*2+1] = hex[l->cert[i]&0xf];
			}
			certhex[i*2] = 0;
			VFS_PRINTF(f, "%s \"", l->hostname);
			VFS_PUTS(f, certhex);
			VFS_PRINTF(f, "\" %i\n", l->trusted?true:false);
		}
		VFS_CLOSE(f);
	}
	else
		Con_Printf(CON_ERROR"Unable to write %s\n", CERTLOG_FILENAME);
}
static void CertLog_Purge(void)
{
	while (certlog.next != &certlog)
	{
		struct certlog_s *l = (struct certlog_s*)certlog.next;
		RemoveLink(&l->l);
		Z_Free(l);
	}

	certlog_inited = false;
}
static int hexdecode(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}
static void CertLog_Import(const char *filename)
{
	char addressstring[512];
	char certhex[32768];
	char certdata[16384];
	char trusted[16];
	char line[65536], *l;
	size_t i, certsize;
	vfsfile_t *f;
	if (!certlog_inited && filename)
		CertLog_Import(NULL);
	certlog_inited |= !filename;
	f = FS_OpenVFS(filename?filename:CERTLOG_FILENAME, "rb", FS_ROOT);
	if (!f)
		return;
	//CertLog_Purge();
	VFS_GETS(f, line, sizeof(line));
	if (strncmp(line, "version 1.", 10))
		return;	//unsupported...
	while (VFS_GETS(f, line, sizeof(line)))
	{
		l = line;
		l = COM_ParseOut(l, addressstring, sizeof(addressstring));
		l = COM_ParseOut(l, certhex, sizeof(certhex));
		l = COM_ParseOut(l, trusted, sizeof(trusted));

		certsize = 0;
		for (i = 0; certsize < sizeof(certdata); i++)
		{
			if (!certhex[(i<<1)+0] || !certhex[(i<<1)+1])
				break;
			certdata[certsize++] = (hexdecode(certhex[(i<<1)+0])<<4)|hexdecode(certhex[(i<<1)+1]);
		}
		CertLog_Update(addressstring, certdata, certsize, atoi(trusted));
	}
	VFS_CLOSE(f);
}
static void CertLog_UntrustAll_f(void)
{
	CertLog_Purge();
}
static void CertLog_Import_f(void)
{
	const char *fname = Cmd_Argv(1);
	if (Cmd_IsInsecure())
		return;
	if (!*fname)
		fname = NULL;
	CertLog_Import(fname);
}
struct certprompt_s
{
	char *hostname;

	size_t certsize;
	qbyte cert[1];
};
static struct certprompt_s *certlog_curprompt;
static void CertLog_Add_Prompted(void *vctx, promptbutton_t button)
{
	struct certprompt_s *ctx = vctx;
	if (button == PROMPT_YES)	//button_yes / button_left
	{
		CertLog_Update(ctx->hostname, ctx->cert, ctx->certsize, true);
		CertLog_Write();

		CL_BeginServerReconnect();
	}
	else
		CL_Disconnect("Server certificate rejected");

	certlog_curprompt = NULL;
}
qboolean CertLog_ConnectOkay(const char *hostname, void *cert, size_t certsize, unsigned int certlogproblems)
{	//this is specifically for dtls certs.
	struct certlog_s *l;
	qboolean trusted = (net_enable_dtls.ival >= 2);
	char digest[DIGEST_MAXSIZE];
	char fp[DIGEST_MAXSIZE*2+1];

	if (certlog_curprompt)
		return false;
	COM_AssertMainThread("CertLog_ConnectOkay");

	if (!certlog_inited)
		CertLog_Import(NULL);
	l = CertLog_Find(hostname);

	if (!l && !trusted)
	{	//cert is new, but we don't care about full trust. don't bother to prompt when the user doesn't much care.
		//(but do pin so we at least know when its MITMed after the fact)
		Con_Printf(CON_WARNING"Auto-Pinning certificate for %s."CON_DEFAULT" ^[/seta %s 2^]+ for actual security.\n", hostname, net_enable_dtls.name);
		if (certsize)
			Base64_EncodeBlockURI(digest, CalcHash(&hash_certfp, digest, sizeof(digest), cert, certsize), fp, sizeof(fp));
		else
			strcpy(fp, "<No Certificate>");
		Con_Printf(S_COLOR_GRAY"  fp: %s\n", fp);
		CertLog_Update(hostname, cert, certsize, false);
		CertLog_Write();
	}
	else if (!l || l->certsize != certsize || memcmp(l->cert, cert, certsize) || (trusted && !l->trusted))
	{	//new or different
		if (certsize)
			Base64_EncodeBlockURI(digest, CalcHash(&hash_certfp, digest, sizeof(digest), cert, certsize), fp, sizeof(fp));
		else
			strcpy(fp, "<No Certificate>");
		if (qrenderer)
		{
			unsigned int i;
			size_t len;
			char *text;
			const char *accepttext;
			const char *lines[] = {
									va(localtext("Certificate for %s\n(fp:"S_COLOR_GRAY"%s"S_COLOR_WHITE")\n"), hostname, fp),
									(certlogproblems&CERTLOG_WRONGHOST)?localtext("^1Certificate does not match host\n"):"",
									((certlogproblems&(CERTLOG_MISSINGCA|CERTLOG_WRONGHOST))==CERTLOG_MISSINGCA)?localtext("^1Certificate authority is untrusted.\n"):"",
									(certlogproblems&CERTLOG_EXPIRED)?localtext("^1Expired Certificate\n"):"",
									l?localtext("\n^1WARNING: Certificate has changed since previously trusted."):""};
			struct certprompt_s *ctx = certlog_curprompt = Z_Malloc(sizeof(*ctx)+certsize + strlen(hostname));
			ctx->hostname = ctx->cert + certsize;
			ctx->certsize = certsize;
			memcpy(ctx->cert, cert, certsize);
			strcpy(ctx->hostname, hostname);

			if (l)	//FIXME: show expiry info for the old cert, warn if more than a month?
				accepttext = localtext("Replace");
			else if (!certlogproblems)
				accepttext = localtext("Pin");
			else
				accepttext = localtext("Trust");

			for (i = 0, len = 0; i < countof(lines); i++)
				len += strlen(lines[i]);
			text = alloca(len+1);
			for (i = 0, len = 0; i < countof(lines); i++)
			{
				strcpy(text+len, lines[i]);
				len += strlen(lines[i]);
			}
			text[len] = 0;

			//FIXME: display some sort of fingerprint
			Menu_Prompt(CertLog_Add_Prompted, ctx, text, accepttext, NULL, localtext("Disconnect"), true);
		}
		return false;	//can't connect yet...
	}
	else if (!l->trusted)
		Con_Printf(CON_WARNING"Server certificate for %s was previously auto-pinned."CON_DEFAULT" ^[/seta %s 2^]+ for actual security.\n", hostname, net_enable_dtls.name);
	return true;
}
#endif



#if defined(HAVE_SERVER) && defined(HAVE_CLIENT)
static struct maplog_entry
{
	struct maplog_entry *next;
	float bestkills;
	float bestsecrets;
	float besttime;	 //updated when besttime<newtime (note: doesn't respond to user changelevels from the console...)
	float fulltime; //updated when bestkills>=newkills
	char name[1];
} *maplog_enties;
static void Log_MapsRead(void)
{
	struct maplog_entry *m, **link = &maplog_enties;
	vfsfile_t *f;
	static qboolean maplog_loaded;
	char line[8192], *s;
	if (maplog_loaded)
		return;
	maplog_loaded = true;
	f = FS_OpenVFS("maptimes.txt", "rb", FS_ROOT);
	if (!f)
		return; //no info yet.
	while (VFS_GETS(f, line, sizeof(line)))
	{
		s = line;
		s = COM_Parse(s);
		m = Z_Malloc(sizeof(*m) + strlen(com_token));
		strcpy(m->name, com_token);

		s = COM_Parse(s);
		m->besttime = atof(com_token);
		s = COM_Parse(s);
		m->fulltime = atof(com_token);
		s = COM_Parse(s);
		m->bestkills = atof(com_token);
		s = COM_Parse(s);
		m->bestsecrets = atof(com_token);

		*link = m;
		link = &m->next;
	}
	VFS_CLOSE(f);
}
struct maplog_entry *Log_FindMap(const char *purepackage, const char *mapname)
{
	char name[MAX_OSPATH];
	struct maplog_entry *m;
	if (Q_snprintfz(name, sizeof(name), "%s/%s", purepackage, mapname))
		return NULL;
	Log_MapsRead();
	for (m = maplog_enties; m; m = m->next)
	{
		if (!strcmp(m->name, name))
			break;
	}
	return m;
}
static void Log_MapsDump(void)
{
	if (maplog_enties)
	{
		struct maplog_entry *m;
		vfsfile_t *f = FS_OpenVFS("maptimes.txt", "wbp", FS_ROOT);
		if (f)
		{
			for(m = maplog_enties; m; m = m->next)
			{
				VFS_PRINTF(f, "\"%s\" %.9g %.9g %.9g %.9g\n", m->name, m->besttime, m->fulltime, m->bestkills, m->bestsecrets);
			}
			VFS_CLOSE(f);
		}
	}
}
qboolean Log_CheckMapCompletion(const char *packagename, const char *mapname, float *besttime, float *fulltime, float *bestkills, float *bestsecrets)
{
	struct maplog_entry *m;
	if (!packagename)
	{
		flocation_t loc;
		if (!FS_FLocateFile(mapname, FSLF_DONTREFERENCE|FSLF_IGNORELINKS, &loc))
			return false;	//no idea which package, don't guess.
		packagename = FS_GetRootPackagePath(&loc);
		if (!packagename)
			return false;
	}
	m = Log_FindMap(packagename, mapname);
	if (m)
	{
		*besttime = m->besttime;
		*fulltime = m->fulltime;
		*bestkills = m->bestkills;
		*bestsecrets = m->bestsecrets;
		return true;
	}
	return false;
}
void Log_MapNowCompleted(void)
{
	struct maplog_entry *m;
	flocation_t loc;
	float kills, secrets, oldprogress, newprogress, maptime;
	const char *packagename;

	//don't log it if its deathmatch/coop/cheating.
	extern int sv_allow_cheats;
	if (deathmatch.ival || coop.ival || sv_allow_cheats == 1)
		return;

	if (!FS_FLocateFile(sv.world.worldmodel->name, FSLF_DONTREFERENCE|FSLF_IGNORELINKS, &loc))
	{
		Con_Printf("completion log: unable to determine logical path for map\n");
		return;	//don't know
	}
	packagename = FS_GetRootPackagePath(&loc);
	if (!packagename)
	{
		Con_Printf("completion log: unable to determine logical path for map\n");
		return;
	}

	m = Log_FindMap(packagename, sv.world.worldmodel->name);
	if (!m)
	{
		m = Z_Malloc(sizeof(*m)+strlen(packagename)+strlen(sv.world.worldmodel->name)+2);
		sprintf(m->name, "%s/%s", packagename, sv.world.worldmodel->name);

		m->fulltime = m->besttime = FLT_MAX;
		m->bestkills = m->bestsecrets = 0;
		m->next = maplog_enties;
		maplog_enties = m;
	}

	kills = pr_global_struct->killed_monsters;
	secrets = pr_global_struct->found_secrets;
	maptime = sv.world.physicstime;

	newprogress = secrets*10+kills;
	oldprogress = m->bestsecrets*10+m->bestkills;

	//if they got a new time record, update.
	if (maptime<m->besttime)
		m->besttime = maptime;
	//if they got a new kills record, update
	if (newprogress > oldprogress || (newprogress==oldprogress && maptime<m->fulltime))
	{
		m->bestkills = kills;
		m->bestsecrets = secrets;
		m->fulltime = maptime;
	}
}
#endif

void Log_ShutDown(void)
{
#if defined(HAVE_SERVER) && defined(HAVE_CLIENT)
	Log_MapsDump();
#endif

#ifdef IPLOG
	if (iplog_autodump.ival)
		IPLog_Dump("iplog.txt");
//	IPLog_Dump("iplog.dat");

	while(iplog_num > 0)
	{
		iplog_num--;
		BZ_Free(iplog_entries[iplog_num]);
	}
	BZ_Free(iplog_entries);
	iplog_entries = NULL;
	iplog_max = iplog_num = 0;
#endif
}

void Log_Init(void)
{
	int i;
	// register cvars
	for (i = 0; i < LOG_TYPES; i++)
	{
#ifdef CLIENTONLY
		if (i != LOG_CONSOLE)
			continue;
#endif
		Cvar_Register (&log_enable[i], CONLOGGROUP);
		Cvar_Register (&log_name[i], CONLOGGROUP);
		log_newline[i] = true;
#ifdef HAVE_LEGACY
		Cmd_AddCommand(legacylog[i].commandname, Log_Logfile_f);
#endif
	}
	Cvar_Register (&log_dir_var, CONLOGGROUP);
	Cvar_Register (&log_readable, CONLOGGROUP);
	Cvar_Register (&log_developer, CONLOGGROUP);
	Cvar_Register (&log_rotate_size, CONLOGGROUP);
	Cvar_Register (&log_rotate_files, CONLOGGROUP);
	Cvar_Register (&log_dosformat, CONLOGGROUP);
	Cvar_Register (&log_timestamps, CONLOGGROUP);

#ifdef IPLOG
	Cmd_AddCommandD("identify", IPLog_Identify_f, "Looks up a player's ip to see if they're using a different name");
	Cmd_AddCommand("ipmerge", IPLog_Merge_f);
	Cmd_AddCommand("ipdump", IPLog_Dump_f);
	Cvar_Register (&iplog_autodump, CONLOGGROUP);
#endif

	// cmd line options, debug options
#ifdef CRAZYDEBUGGING
	Cvar_ForceSet(&log_enable[LOG_CONSOLE], "1");
	TRACE(("dbg: Con_Init: log_enable forced\n"));
#endif

	if (COM_CheckParm("-condebug"))
		Cvar_ForceSet(&log_enable[LOG_CONSOLE], "1");

#if defined(HAVE_DTLS) && defined(HAVE_CLIENT)
	ClearLink(&certlog);
	Cmd_AddCommand("dtls_untrustall", CertLog_UntrustAll_f);
	Cmd_AddCommand("dtls_importtrust", CertLog_Import_f);
#endif
}
