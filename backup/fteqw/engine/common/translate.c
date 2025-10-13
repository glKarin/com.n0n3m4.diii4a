#include "quakedef.h"
#include <wctype.h>

//#define COLOURMISSINGSTRINGS		//for english people to more easily see what's not translatable (text still white)
//#define COLOURUNTRANSLATEDSTRINGS	//show empty translations as alt-text versions of the original string

//client may remap messages from the server to a regional bit of text.
//server may remap progs messages

//basic language is english (cos that's what (my version of) Quake uses).
//translate is english->lang
//untranslate is lang->english for console commands.

static void FilterPurge(void);
static void FilterInit(const char *file);

int com_language;
char sys_language[64] = "";
static char langpath[MAX_OSPATH] = "";
struct language_s languages[MAX_LANGUAGES];

static void QDECL TL_LanguageChanged(struct cvar_s *var, char *oldvalue)
{
	com_language = TL_FindLanguage(var->string);
}

cvar_t language = CVARAFCD("lang", sys_language, "prvm_language", CVAR_USERINFO|CVAR_NORESET/*otherwise gamedir switches will be annoying*/, TL_LanguageChanged, "This cvar contains the language_dialect code of your language, used to find localisation strings.");

static void Filter_Reload_f(void)
{
	char *file = FS_MallocFile("filter.txt", FS_ROOT, NULL);
	FilterInit(file?file:"");
	FS_FreeFile(file);
}
void TranslateInit(void)
{
	Cmd_AddCommand("com_reloadfilter", Filter_Reload_f);
	Cvar_Register(&language, "Internationalisation");
}

void TL_Shutdown(void)
{
	int j;

	for (j = 0; j < MAX_LANGUAGES; j++)
	{
		if (!languages[j].name)
			continue;
		free(languages[j].name);
		languages[j].name = NULL;
		PO_Close(languages[j].po);
		languages[j].po = NULL;

		PO_Close(languages[j].po_qex);
		languages[j].po_qex = NULL;
	}
	FilterPurge();
}

static int TL_LoadLanguage(char *lang)
{
	vfsfile_t *f;
	int j;
	char *u;
	for (j = 0; j < MAX_LANGUAGES; j++)
	{
		if (!languages[j].name)
			break;
		if (!stricmp(languages[j].name, lang))
			return j;
	}

	//err... oops, ran out of languages...
	if (j == MAX_LANGUAGES)
		return 0;

	if (*lang)
		f = FS_OpenVFS(va("%sfteqw.%s.po", langpath, lang), "rb", FS_SYSTEM);
	else
		f = NULL;
	if (!f && *lang)
	{
		//keep truncating until we can find a name that works
		u = strrchr(lang, '_');
		if (u)
		{
			*u = 0;
			return TL_LoadLanguage(lang);
		}
	}
	languages[j].name = strdup(lang);
	languages[j].po = NULL;
	
#if !defined(COLOURUNTRANSLATEDSTRINGS) && !defined(COLOURMISSINGSTRINGS)
	if (f)
#endif
	{
		languages[j].po = PO_Create();
		PO_Merge(languages[j].po, f);
	}

	return j;
}
int TL_FindLanguage(const char *lang)
{
	char trimname[64];
	Q_strncpyz(trimname, lang, sizeof(trimname));
	return TL_LoadLanguage(trimname);
}

//need to set up default languages for any early prints before cvars are inited.
void TL_InitLanguages(const char *newlangpath)
{
	int i;
	char *lang;

	if (!newlangpath)
		newlangpath = "";

	Q_strncpyz(langpath, newlangpath, sizeof(langpath));

	//lang can override any environment or system settings.
	if ((i = COM_CheckParm("-lang")))
		Q_strncpyz(sys_language, com_argv[i+1], sizeof(sys_language));
	else
	{
		lang = NULL;
		if (!lang)
			lang = getenv("LANGUAGE");
		if (!lang)
			lang = getenv("LC_ALL");
		if (!lang)
			lang = getenv("LC_MESSAGES");
		if (!lang)
			lang = getenv("LANG");
		if (!lang)
			lang = "";
		if (!strcmp(lang, "C") || !strcmp(lang, "POSIX"))
			lang = "";

		//windows will have already set the locale from the windows settings, so only replace it if its actually valid.
		if (*lang)
			Q_strncpyz(sys_language, lang, sizeof(sys_language));
	}

	//clean it up.
	//takes the form: [language[_territory][.codeset][@modifier]]
	//we don't understand modifiers
	lang = strrchr(sys_language, '@');
	if (lang)
		*lang = 0;
	//we don't understand codesets sadly.
	lang = strrchr(sys_language, '.');
	if (lang)
		*lang = 0;
	//we also only support the single primary locale (no fallbacks, we're just using the language[+territory])
	lang = strrchr(sys_language, ':');
	if (lang)
		*lang = 0;
	//but we do support territories.
	
	com_language = TL_FindLanguage(sys_language);

	//make sure a fallback exists, but not as language 0
	TL_FindLanguage("");
}




#ifdef HEXEN2
//this stuff is for hexen2 translation strings.
//(hexen2 is uuuuggllyyyy...)
static char *strings_list;
static char **strings_table;
static int strings_count;
static qboolean strings_loaded;
void T_FreeStrings(void)
{	//on map change, following gamedir change
	if (strings_loaded)
	{
		BZ_Free(strings_list);
		BZ_Free(strings_table);
		strings_count = 0;
		strings_loaded = false;
	}
}
void T_LoadString(void)
{
	int i;
	char *s, *s2;
	//count new lines
	strings_loaded = true;
	strings_count = 0;
	strings_list = FS_LoadMallocFile("strings.txt", NULL);
	if (!strings_list)
		return;

	for (s = strings_list; *s; s++)
	{
		if (*s == '\n')
			strings_count++;
	}
	strings_table = BZ_Malloc(sizeof(char*)*strings_count);

	s = strings_list;
	for (i = 0; i < strings_count; i++)
	{
		strings_table[i] = s;
		s2 = strchr(s, '\n');
		if (!s2)
			break;

		while (s < s2)
		{
			if (*s == '\r')
				*s = '\0';
			else if (*s == '^' || *s == '@')	//becomes new line
				*s = '\n';
			s++;
		}
		s = s2+1;
		*s2 = '\0';
	}
}
char *T_GetString(int num)
{
	if (!strings_loaded)
	{
		T_LoadString();
	}
	if (num<0 || num >= strings_count)
		return "BAD STRING";

	return strings_table[num];
}

#ifdef HAVE_CLIENT
//for hexen2's objectives and stuff.
static char *info_strings_list;
static char **info_strings_table;
static int info_strings_count;
static qboolean info_strings_loaded;
void T_FreeInfoStrings(void)
{	//on map change, following gamedir change
	if (info_strings_loaded)
	{
		BZ_Free(info_strings_list);
		BZ_Free(info_strings_table);
		info_strings_count = 0;
		info_strings_loaded = false;
	}
}
void T_LoadInfoString(void)
{
	int i;
	char *s, *s2;
	//count new lines
	info_strings_loaded = true;
	info_strings_count = 0;
	info_strings_list = FS_LoadMallocFile("infolist.txt", NULL);
	if (!info_strings_list)
		return;

	for (s = info_strings_list; *s; s++)
	{
		if (*s == '\n')
			info_strings_count++;
	}
	info_strings_table = BZ_Malloc(sizeof(char*)*info_strings_count);

	s = info_strings_list;
	for (i = 0; i < info_strings_count; i++)
	{
		info_strings_table[i] = s;
		s2 = strchr(s, '\n');
		if (!s2)
			break;

		while (s < s2)
		{
			if (*s == '\r')
				*s = '\0';
			else if (*s == '^' || *s == '@')	//becomes new line
				*s = '\n';
			s++;
		}
		s = s2+1;
		*s2 = '\0';
	}
}
char *T_GetInfoString(int num)
{
	if (!info_strings_loaded)
	{
		T_LoadInfoString();
	}
	if (num<0 || num >= info_strings_count)
		return "BAD STRING";

	return info_strings_table[num];
}
#endif
#endif

struct poline_s
{
	bucket_t buck;
	struct poline_s *next;
	char *orig;
	char *translated;
};

struct po_s
{
	hashtable_t hash;

	struct poline_s *lines;
};

static struct poline_s *PO_AddText(struct po_s *po, const char *orig, const char *trans)
{	//input is assumed to be utf-8, but that's not always what quake uses. on the plus side we do have our own silly markup to handle unicode (and colours etc).
	size_t olen = strlen(orig)+1;
	size_t tlen;
	struct poline_s *line;
	const char *s;
	char temp[64];

	//figure out the required length for the encoding we're actually going to use
	if (com_parseutf8.ival != 1)
	{
		tlen = 0;
		for (s = trans, tlen = 0; *s; )
		{
			unsigned int err;
			unsigned int chr = utf8_decode(&err, s, &s);
			tlen += unicode_encode(temp, chr, sizeof(temp), true);
		}
		tlen++;
	}
	else
		tlen = strlen(trans)+1;

	line = Z_Malloc(sizeof(*line)+olen+tlen);
	memcpy(line+1, orig, olen);
	orig = (const char*)(line+1);
	line->translated = (char*)(line+1)+olen;
	if (com_parseutf8.ival != 1)
	{
		//do the loop again now we know we've got enough space for it.
		for (s = trans, tlen = 0; *s; )
		{
			unsigned int err;
			unsigned int chr = utf8_decode(&err, s, &s);
			tlen += unicode_encode(line->translated+tlen, chr, sizeof(temp), true);
		}
		line->translated[tlen] = 0;
	}
	else
		memcpy(line->translated, trans, tlen);
	trans = (const char*)(line->translated);
	Hash_Add(&po->hash, orig, line, &line->buck);

	line->next = po->lines;
	po->lines = line;
	return line;
}
void PO_Merge(struct po_s *po, vfsfile_t *file)
{
	char *instart, *in, *end;
	int inlen;
	char msgid[32768];
	char msgstr[32768];
	struct {
		quint32_t magic;
		quint32_t revision;
		quint32_t numstrings;
		quint32_t offset_orig;
		quint32_t offset_trans;
//		quint32_t hashsize;
//		quint32_t offset_hash;
	} *moheader;

	qboolean allowblanks = !!COM_CheckParm("-translatetoblank");
	if (!file)
		return;

	inlen = file?VFS_GETLEN(file):0;
	instart = in = BZ_Malloc(inlen+1);
	if (file)
		VFS_READ(file, in, inlen);
	in[inlen] = 0;
	if (file)
		VFS_CLOSE(file);

	moheader = (void*)in;
	if (inlen >= sizeof(*moheader) && moheader->magic == 0x950412de)
	{
		struct
		{
			quint32_t length;
			quint32_t offset;
		} *src = (void*)(in+moheader->offset_orig), *dst = (void*)(in+moheader->offset_trans);
		quint32_t i;
		for (i = moheader->numstrings; i-- > 0; src++, dst++)
			PO_AddText(po, in+src->offset, in+dst->offset);
	}
    else
	{
		end = in + inlen;
		while(in < end)
		{
			while(*in == ' ' || *in == '\n' || *in == '\r' || *in == '\t')
				in++;
			if (*in == '#')
			{
				while (*in && *in != '\n')
					in++;
			}
			else if (!strncmp(in, "msgid", 5) && (in[5] == ' ' || in[5] == '\t' || in[5] == '\r' || in[5] == '\n'))
			{
				size_t start = 0;
				size_t ofs = 0;
				in += 5;
				while(1)
				{
					while(*in == ' ' || *in == '\n' || *in == '\r' || *in == '\t')
						in++;
					if (*in == '\"')
					{
						in = COM_ParseCString(in, msgid+start, sizeof(msgid) - start, &ofs);
						start += ofs;
					}
					else
						break;
				}
			}
			else if (!strncmp(in, "msgstr", 6) && (in[6] == ' ' || in[6] == '\t' || in[6] == '\r' || in[6] == '\n'))
			{
				size_t start = 0;
				size_t ofs = 0;
				in += 6;
				while(1)
				{
					while(*in == ' ' || *in == '\n' || *in == '\r' || *in == '\t')
						in++;
					if (*in == '\"')
					{
						in = COM_ParseCString(in, msgstr+start, sizeof(msgstr) - start, &ofs);
						start += ofs;
					}
					else
						break;
				}

				if ((*msgid && start) || allowblanks)
					PO_AddText(po, msgid, msgstr);
#ifdef COLOURUNTRANSLATEDSTRINGS
				else if (!start)
				{
					char temp[1024];
					int i;
					Q_snprintfz(temp, sizeof(temp), "%s", *msgstr?msgstr:msgid);
					for (i = 0; temp[i]; i++)
					{
						if (temp[i] == '%')
						{
							while (temp[i] > ' ')
								i++;
						}
						else if (temp[i] >= ' ')
							temp[i] |= 0x80;
					}
					PO_AddText(po, msgid, temp);
				}
#endif
			}
			else
			{
				//some sort of junk?
				in++;
				while (*in && *in != '\n')
					in++;
			}
		}
	}

	BZ_Free(instart);
}
struct po_s *PO_Create(void)
{
	struct po_s *po;
	unsigned int buckets = 1024;

	po = Z_Malloc(sizeof(*po) + Hash_BytesForBuckets(buckets));
	Hash_InitTable(&po->hash, buckets, po+1);
	return po;
}
void PO_Close(struct po_s *po)
{
	if (!po)
		return;
	while(po->lines)
	{
		struct poline_s *r = po->lines;
		po->lines = r->next;
		Z_Free(r);
	}
	Z_Free(po);
}

const char *PO_GetText(struct po_s *po, const char *msg)
{
	struct poline_s *line;
	if (!po || !msg)
		return msg;
	line = Hash_Get(&po->hash, msg);

#ifdef COLOURMISSINGSTRINGS
	if (!line)
	{
		char temp[1024];
		int i;
		const char *in = msg;
		for (i = 0; *in && i < sizeof(temp)-1; )
		{
			if (*in == '%')
			{	//don't mess up % formatting too much
				while (*in > ' ' && i < sizeof(temp)-1)
					temp[i++] = *in++;
			}
			else if (in > ' ' && *in < 128)	//otherwise force any ascii chars to the 0xe0XX range so it doesn't use any freetype fonts so its instantly recognisable as bad.
				i += utf8_encode(temp+i, *in++|0xe080, sizeof(temp)-1-i);
			else
				temp[i++] = *in++;	//don't mess with any c0/extended codepoints
		}
		temp[i] = 0;
		line = PO_AddText(po, msg, temp);
	}
#endif

	if (line)
		return line->translated;
	return msg;
}


static void PO_Merge_Rerelease(struct po_s *po, const char *langname, const char *fmt)
{
	//FOO <plat,plat> = "CString"
	char line[32768];
	char key[256];
	char val[32768];
	char *s;
	vfsfile_t *file = NULL;

	if (!file && *langname)	//use system locale names
		file = FS_OpenVFS(va(fmt, langname), "rb", FS_GAME);
	if (!file)	//make a guess
	{
		s = NULL;
		if (langname[0] && langname[1] && (!langname[2] || langname[2] == '-' || langname[2] == '_'))
		{	//try to map the user's formal locale to the rerelease's arbitrary names (at least from the perspective of anyone who doesn't speak english).
			if (!strncmp(langname, "fr", 2))
				s = "french";
			else if (!strncmp(langname, "de", 2))
				s = "german";
			else if (!strncmp(langname, "it", 2))
				s = "italian";
			else if (!strncmp(langname, "ru", 2))
				s = "russian";
			else if (!strncmp(langname, "es", 2))
				s = "spanish";
		}
		if (s)
			file = FS_OpenVFS(va(fmt, s), "rb", FS_GAME);
	}
	if (!file)	//fall back on misnamed american, for lack of a better default.
		file = FS_OpenVFS(va(fmt, "english"), "rb", FS_GAME);
	if (file)
	{
		*key = '$';
		while(VFS_GETS(file, line, sizeof(line)))
		{
			s = COM_ParseOut(line, key+1, sizeof(key)-1);
			s = COM_ParseOut(s, val, sizeof(val));
			if (strcmp(val,"="))
				continue;
			s = COM_ParseCString(s, val, sizeof(val), NULL);
			if (!s)
				continue;
			PO_AddText(po, key, val);
		}
		VFS_CLOSE(file);
	}
}

const char *TL_Translate(int language, const char *src)
{
	if (*src == '$')
	{
		if (!languages[language].po_qex)
		{
			char lang[64], *h;
			vfsfile_t *f = NULL;
			languages[language].po_qex = PO_Create();
			PO_Merge_Rerelease(languages[language].po_qex, languages[language].name, "localization/loc_%s.txt");

			Q_strncpyz(lang, languages[language].name, sizeof(lang));
			while ((h = strchr(lang, '-')))
				*h = '_';	//standardise it
			if (*lang)
				f = FS_OpenVFS(va("localisation/%s.po", lang), "rb", FS_GAME);	//long/specific form
			if (!f)
			{
				if ((h = strchr(lang, '_')))
				{
					*h = 0;
					if (*lang)
						f = FS_OpenVFS(va("localisation/%s.po", lang), "rb", FS_GAME);	//short/general form
				}
			}
			if (f)
				PO_Merge(languages[language].po_qex, f);
		}
		src = PO_GetText(languages[language].po_qex, src);
	}
	return src;
}
void TL_Reformat(int language, char *out, size_t outsize, size_t numargs, const char **arg)
{
	const char *fmt;
	const char *a;
	size_t alen;
	unsigned int lastindex = 0;

	fmt = (numargs>0&&arg[0])?arg[0]:"";
	fmt = TL_Translate(language, fmt);

	outsize--;
	while (outsize > 0)
	{
		if (!*fmt)
			break;
		else if (*fmt == '{' && fmt[1] == '{')
			*out++ = '{', fmt+=2, outsize--;
		else if (*fmt == '}' && fmt[1] == '}')
			*out++ = '}', fmt+=2, outsize--;
		else if (*fmt == '{')
		{
			const char *idxstr = fmt+1;
			unsigned int index = strtoul(idxstr, (char**)&fmt, 10)+1;
			int size = 0;
			if (idxstr == fmt)	//when no index value was specified, just go for the next one
				index = lastindex+1;
			if (*fmt == ',')
				size = strtol(fmt+1, (char**)&fmt, 10);
			if (*fmt == ':')
			{	//formatting, which we don't support because its all strings.
				fmt = fmt+1;
				while (*fmt && *fmt != '}')
					fmt++;
			}
			if (*fmt == '}')
				fmt++;
			else
				break;	//some formatting error

			if (index >= numargs || !arg[index])
				a = "";
			else
				a = TL_Translate(language, arg[index]);

			lastindex = index;

			alen = strlen(a);
			if (alen > outsize)
				alen = outsize;
			if (size > 0)
			{	//right aligned
				if (alen > size)
					alen = size;
				memcpy(out, a, alen);
			}
			else if (size < 0)
			{	//left aligned
				if (alen > -size)
					alen = -size;
				memcpy(out, a, alen);
			}
			else //no alignment, no padding.
				memcpy(out, a, alen);
			out += alen;
			outsize -= alen;
		}
		else
			*out++ = *fmt++, outsize--;
	}
	*out = 0;
}

#include <ctype.h>
static qbyte *filter[256]; //one list per lead char, simple optimisation instead of some big decision tree.
static qbyte *filtermem;
static int FilterCompareWords(const void *v1, const void *v2)
{
	const char *s1 = *(const char*const*)v1;
	const char *s2 = *(const char*const*)v2;
	return strcmp(s2,s1);
}
static void FilterPurge(void)
{
	memset(filter, 0, sizeof(filter));
	free(filtermem);
	filtermem = NULL;
}
static void FilterInit(const char *file)
{
	qbyte *tempmemstart = malloc(strlen(file)+1);
	qbyte *tempmem = tempmemstart;
	const char **words;
	size_t count = 1, i, l;
	size_t bytes;
	const char *c;

	FilterPurge();

	for (c = file; *c; c++)
		if (*c == '\n')
			count++;

	words = malloc(sizeof(qbyte*)*count);
	count = 0;
	for (c = file; *c; )
	{
		while (*c == '\n')
			c++;	//don't add 0-byte strings...
		words[count] = tempmem;
		for (; *c; c++)
		{
			if (*c == ' ')
				continue; //block even if they omit the spaces.
			if (*c == '\n')
				break;
			*tempmem++ = tolower(*c);
		}
		*tempmem++ = 0;
		if (*words[count])
			count++;
	}
	qsort(words, count, sizeof(words[0]), FilterCompareWords);	//sort by lead byte... and longest first...
	i = 0;
	for (i = 0, bytes = 0; i < count; i++)
		bytes += strlen(words[i])+1;
	bytes += countof(filter);
	filtermem = tempmem = malloc(bytes);

	for (l = countof(filter), i = 0; l-- > 0; )
	{
		if (i < count && words[i][0] == l)
		{
			filter[l] = tempmem;
			while (i < count && *words[i] == l)
			{	//second copy... urgh. can forget the first char and replace with a length.
				*tempmem++ = strlen(words[i]+1);
				memcpy(tempmem, words[i]+1, tempmem[-1]);	//just the text, no null needed. tighly packed.
				tempmem += tempmem[-1];
				i++;
			}
			*tempmem++ = 0;
		}
		else
			filter[l] = NULL;
	}
	free(tempmemstart);
	free(words);
}
#define whiteish(c) (c == ',' || c == '.' || c == ' ' || c == '\t' || c == '\r' || c == '\n')
char *FilterObsceneString(const qbyte *in, char *outbuf, size_t bufsize)
{	//input must be utf-8... if there's any ^ crap in there then strip it first. no bypassing filters with colour codes.
	char *ret = outbuf;
	if (strlen(in) >= bufsize)
		Sys_Error("output buffer too small!");
	if (!filtermem)
		Filter_Reload_f();
restart:
	while (*in)
	{
		qbyte c = tolower(*in);
		if (filter[c])
		{
			qbyte *m = filter[c];
			while (*m)
			{	//for each word starting with this letter...
				const qbyte *test = in+1;
				qbyte len = *m;
				const qbyte *match = m+1;
				m += 1+len;
				while (*test)
				{	//don't let 'foo bar' through when 'foobar' is a bad word.
					if (whiteish(*test))
					{
						test++;
						continue;
					}

					if (tolower(*test) == *match)
					{
						test++, match++;
						if (--len == 0)
						{	//a match.
							if (*test && !whiteish(*test))
								break;	//assassinate!
							while (test > in)
							{	//censor it.
								*outbuf = "#*@$"[(outbuf-ret)&3];
								outbuf++;
								in++;
							}
							goto restart; //double breaks suck
						}
						continue;
					}
					break;
				}
			}
		}
		while (*in)
		{
			if (whiteish(*in))
			{
				*outbuf++ = *in++;
				break;
			}
			*outbuf++ = *in++;
		}
	}
	*outbuf++ = 0;	//make sure its null terminated.
	return ret;
}
qboolean TL_FilterObsceneCCStringInplace(conchar_t *in, conchar_t *end)
{	//FIXME: filters are meant to be utf-8, but our strings are not.
	qboolean obscene = false;
//	conchar_t *start = in;
	conchar_t *next;
	if (!filtermem)
		Filter_Reload_f();
restart:
	while(in < end)
	{
		unsigned int c, cflags;
		next = Font_Decode(in, &cflags, &c);
		c = towlower(c);
		if (c < 255 && filter[c])
		{
			qbyte *m = filter[c];
			while (*m)
			{	//for each word starting with this letter...
				conchar_t *test = next;
				qbyte len = *m;
				const qbyte *match = m+1;
				int err;
				m += 1+len;
				while(*match && test < end)
				{	//don't let 'foo bar' through when 'foobar' is a bad word.
					test = Font_Decode(test, &cflags, &c);
					if (whiteish(c))
						continue;

					if (towlower(c) == utf8_decode(&err, match, (char const**)&match))
					{
						if (--len == 0)
						{	//a match.

							//peek the next and reject it if we're still mid word
							if (test < end)
								Font_Decode(test, &cflags, &c);
							else
								c = 0;
							if (c && !whiteish(c))
								break;	//assassinate!

							//okay, not mid-word, obuscate the swears.
							while (test > in)
							{	//censor it.
								if (*in & CON_LONGCHAR && !(*in & CON_RICHFORECOLOUR))
									*in = CON_LONGCHAR;	//no other flags here.
								else
								{
									//*in = "#@*$"[(in-start)&3] | CON_WHITEMASK;
									*in	= 0x26a0 | CON_WHITEMASK | (*in&CON_HIDDEN);
									obscene = true;
								}
								in++;
							}
							goto restart; //double breaks suck
						}
						continue;
					}
					break;
				}
			}
		}
		for(; next < end; next = Font_Decode(next, &cflags, &c))
		{
			if (whiteish(c))
				break;
		}
		in = next;
	}
	return obscene;
}
