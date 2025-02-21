#include "quakedef.h"
#include "fs.h"
#include "../plugin.h"
static plugsubconsolefuncs_t *confuncs;
static plugfsfuncs_t *filefuncs;
static plugclientfuncs_t *clientfuncs;

#include "../jabber/xml.h"

#define DATABASEURL		"https://www.quaddicted.com/reviews/quaddicted_database.xml"
#define FILEIMAGEURL	"https://www.quaddicted.com/reviews/screenshots/%s_injector.jpg"
#define FILEDOWNLOADURL	"https://www.quaddicted.com/filebase/%s.zip"
#define FILEREVIEWURL	"https://www.quaddicted.com/reviews/%s.html"
#define WINDOWTITLE		"Quaddicted Map+Mod Archive"
#define WINDOWNAME		"QI"

/*
<file id="downloadname" type="1=map. 2=mod" rating="5-star-rating">
	<author>meh</author>
	<title>some readable name</title>
	<md5sum>xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx</md5sum>
	<size>size-in-kb</size>
	<date>dd.mm.yy</date>
	<description>HTML-encoded text. just to be awkward</description>
	<techinfo>
		<zipbasedir>additional path needed to make it relative to the quake directory</zipbasedir>
		<requirements>
			<file id="quoth" />
		</requirements>
	</techinfo>
</file>
*/

static xmltree_t *thedatabase;
static qhandle_t dlcontext = -1;
static vfsfile_t *packagemanager;

static struct
{
	char namefilter[256];
	int minrating;
	int maxrating;
	int type;
} filters;

static struct
{
	int width;
	int height;
} pvid;
static void QDECL QI_UpdateVideo(int width, int height, qboolean restarted)
{
	pvid.width = width;
	pvid.height = height;
}

void Con_SubPrintf(const char *subname, char *format, ...)
{
	va_list		argptr;
	static char		string[8192];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format,argptr);
	va_end (argptr);

	confuncs->SubPrint(subname, string);
}


static void QI_Shutdown(void)
{
	if (dlcontext != -1)
	{	//we're still downloading something? :o
		filefuncs->Close(dlcontext);
		dlcontext = -1;
	}
	if (thedatabase)
		XML_Destroy(thedatabase);
	thedatabase = NULL;
}

static qboolean QI_SetupWindow(const char *console, qboolean force)
{
	if (!confuncs)
		return false;

	//only redraw the window if it actually exists. if they closed it, then don't mess things up.
	if (!force && confuncs->GetConsoleFloat(console, "iswindow") <= 0)
		return false;

	if (confuncs->GetConsoleFloat(console, "iswindow") != true)
	{
		confuncs->SetConsoleString(console, "title", WINDOWTITLE);
		confuncs->SetConsoleFloat(console, "iswindow", true);
		confuncs->SetConsoleFloat(console, "forceutf8", true);
		confuncs->SetConsoleFloat(console, "linebuffered", false);
		confuncs->SetConsoleFloat(console, "maxlines", 16384);	//the line limit is more a sanity thing than anything else. so long as we explicitly clear before spamming more, then its not an issue...
		confuncs->SetConsoleFloat(console, "wnd_x", 8);
		confuncs->SetConsoleFloat(console, "wnd_y", 8);
		confuncs->SetConsoleFloat(console, "wnd_w", pvid.width-16);
		confuncs->SetConsoleFloat(console, "wnd_h", pvid.height-16);
		confuncs->SetConsoleString(console, "footer", "");
	}
	confuncs->SetConsoleFloat(console, "linecount", 0);	//clear it
	if (force)
		confuncs->SetActive(console);
	return true;
}
static void QI_DeHTML(const char *in, qboolean escapes, char *out, size_t outsize)
{
	outsize--;
	while(*in && outsize > 0)
	{
		if (*in == '\r' || *in == '\n')
			in++;
		else if (*in == '<')
		{
			char tag[256];
			int i;
			qboolean open = false;
			qboolean close = false;
			in++;
			if (*in == '/')
			{
				in++;
				close = true;
			}
			else
				open = true;
			while (*in == ' ' || *in == '\n' || *in == '\t' || *in == '\r')
				in++;
			for (i = 0; i < countof(tag)-1; )
			{
				if (*in == '>')
					break;
				if (!*in || *in == ' ' || *in == '\n' || *in == '\t' || *in == '\r' || (in[0] == '/' && in[1] == '>'))
					break;
				tag[i++] = *in++;
			}
			tag[i] = 0;
			while (*in && *in)
			{
				if (*in == '/' && in[1] == '>')
				{
					in += 2;
					close = true;
					break;
				}
				if (*in++ == '>')
					break;
			}
			if (!strcmp(tag, "br") && open && outsize > 0)
			{	//new lines!
				*out++ = '\n';
				outsize--;
			}
			else if (!strcmp(tag, "b") && open != close && outsize > 1)
			{	//bold
				*out++ = '^';
				outsize--;
				*out++ = 'a';
				outsize--;
			}
			else if (!strcmp(tag, "i") && open != close && outsize > 1)
			{	//italics
				*out++ = '^';
				outsize--;
				*out++ = close?'7':'3';
				outsize--;
			}
		}
		else if (*in == '^')
		{
			if (outsize > 1)
			{
				*out++ = *in;
				outsize--;
				*out++ = *in++;
				outsize--;
			}
		}
		else if ((*in == '\"') && escapes && outsize >= 2)
		{
			*out++ = '\\';
			*out++ = *in++;
			outsize-=2;
		}
		else
		{
			*out++ = *in++;
			outsize--;
		}
	}
	*out = 0;
}

static char *QI_strcasestr(const char *haystack, const char *needle)
{
	int c1, c2, c2f;
	int i;
	c2f = *needle;
	if (c2f >= 'a' && c2f <= 'z')
		c2f -= ('a' - 'A');
	if (!c2f)
		return (char*)haystack;
	while (1)
	{
		c1 = *haystack;
		if (!c1)
			return NULL;
		if (c1 >= 'a' && c1 <= 'z')
			c1 -= ('a' - 'A');
		if (c1 == c2f)
		{
			for (i = 1; ; i++)
			{
				c1 = haystack[i];
				c2 = needle[i];
				if (c1 >= 'a' && c1 <= 'z')
					c1 -= ('a' - 'A');
				if (c2 >= 'a' && c2 <= 'z')
					c2 -= ('a' - 'A');
				if (!c2)
					return (char*)haystack;	//end of needle means we found a complete match
				if (!c1)	//end of haystack means we can't possibly find needle in it any more
					return NULL;
				if (c1 != c2)	//mismatch means no match starting at haystack[0]
					break;
			}
		}
		haystack++;
	}
	return NULL;	//didn't find it
}

static void QI_RefreshMapList(qboolean forcedisplay)
{
	xmltree_t *file;
	const char *console = WINDOWNAME;
	char descbuf[1024];
	float donestats[4];
	if (!QI_SetupWindow(console, forcedisplay))
		return;

	if (!thedatabase)
	{
		if (dlcontext != -1)
			Con_SubPrintf(console, "Downloading database...\n");
		else
			Con_SubPrintf(console, "Unable to download HTTP database\n");
		return;
	}

	if (!thedatabase->child)
	{
		Con_SubPrintf(console, "No maps in database... SPIRIT! YOU BROKE IT!\n");
		return;
	}

	for (file = thedatabase->child; file; file = file->sibling)
	{
		const char *id = XML_GetParameter(file, "id", "unnamed");
		const char *rating = XML_GetParameter(file, "rating", "");
		int ratingnum = atoi(rating);
		const char *author = XML_GetChildBody(file, "author", "unknown");
		const char *desc = XML_GetChildBody(file, "description", "<NO DESCRIPTION>");
		const char *type;
		const char *date;
		int year, month, day;
		int startmapnum, i;
		char ratingtext[65];
		xmltree_t *tech;
		xmltree_t *startmap;
		if (strcmp(file->name, "file"))
			continue;	//erk?
		if (atoi(XML_GetParameter(file, "hide", "")) || atoi(XML_GetParameter(file, "fte_hide", "")))
			continue;
		type = XML_GetParameter(file, "type", "");
		if (filters.type && atoi(type) != filters.type)
			continue;
		switch(atoi(type))
		{
		case 1:
			type = "map";	//'single map file(s)'
			break;
		case 2:
			type = "mod";	//'Partial conversion'
			break;
		case 4:
			type = "spd";	//'speedmapping'
			break;
		case 5:
			type = "otr";	//'misc files'
			break;
		default:
			type = "???";	//no idea
			break;
		}

		if (filters.maxrating>=0 && ratingnum > filters.maxrating)
			continue;
		if (filters.minrating>=0 && ratingnum < filters.minrating)
			continue;

		tech = XML_ChildOfTree(file, "techinfo", 0);
		//if the filter isn't contained in the id/desc then don't display it.
		if (*filters.namefilter)
		{
			if (!QI_strcasestr(id, filters.namefilter) && !QI_strcasestr(desc, filters.namefilter) && !QI_strcasestr(author, filters.namefilter))
			{
				//check map list too
				for (startmapnum = 0; ; startmapnum++)
				{
					startmap = XML_ChildOfTree(tech, "startmap", startmapnum);
					if (!startmap)
						break;
					if (QI_strcasestr(startmap->body, filters.namefilter))
						break;
				}
				if (!startmap)
					continue;
			}
		}

		if (ratingnum > (sizeof(ratingtext)-5)/6)
			ratingnum = (sizeof(ratingtext)-5)/6;
		if (ratingnum)
		{
			Q_snprintf(ratingtext, sizeof(ratingtext), "^a");
			for (i = 0; i < ratingnum; i++)
				Q_snprintf(ratingtext + i+2, sizeof(ratingtext)-i*2+2, "*");
			Q_snprintf(ratingtext + i+2, sizeof(ratingtext)-i*2+2, "^a");
		}
		else if (*rating)
			Q_snprintf(ratingtext, sizeof(ratingtext), "%s", rating);
		else
			Q_snprintf(ratingtext, sizeof(ratingtext), "%s", "unrated");

		
		date = XML_GetChildBody(file, "date", "1.1.1990");
		day = atoi(date?date:"1");
		date = date?strchr(date, '.'):NULL;
		month = atoi(date?date+1:"1");
		date = date?strchr(date+1, '.'):NULL;
		year = atoi(date?date+1:"1990");
		if (year < 90)
			year += 2000;
		else if (year < 1900)
			year += 1900;
		Q_snprintf(descbuf, sizeof(descbuf), "^aId:^a %s\n^aAuthor(s):^a %s\n^aDate:^a %04u-%02u-%02u\n^aRating:^a %s\n\n", id, author, year, month, day, ratingtext);

		QI_DeHTML(desc, false, descbuf + strlen(descbuf), sizeof(descbuf) - strlen(descbuf));
		desc = descbuf;

		Con_SubPrintf(console, "%s %s ^[^4%s: ^1%s\\tip\\%s\\tipimg\\"FILEIMAGEURL"\\id\\%s^]", type, ratingtext, id, XML_GetChildBody(file, "title", "<NO TITLE>"), desc, id, id);
		for (startmapnum = 0; ; startmapnum++)
		{
			char bspfile[MAX_QPATH];
			startmap = XML_ChildOfTree(tech, "startmap", startmapnum);
			if (!startmap)
				break;

			Q_snprintf(bspfile, sizeof(bspfile), "maps/%s.bsp", startmap->body);
			if (clientfuncs && clientfuncs->MapLog_Query(va(FILEDOWNLOADURL, id), bspfile, donestats))
				Con_SubPrintf(console, " ^[^2[%s, complete %.1f]\\tip\\^7^aBest Time:^a ^2%.9f^7\n^aCompletion Time:^a %.9f\n^aKills:^a %.9f\n^aSecrets:^a %.9f\n\n\n%s"/*"\\tipimg\\"FILEIMAGEURL*/"\\id\\%s\\startmap\\%s^]", startmap->body, donestats[0], donestats[0], donestats[1], donestats[2], donestats[3], desc, /*id,*/ id, startmap->body);
			else
				Con_SubPrintf(console, " ^[^4[%s]\\tip\\%s"/*"\\tipimg\\"FILEIMAGEURL*/"\\id\\%s\\startmap\\%s^]", startmap->body, desc, /*id,*/ id, startmap->body);
		}
		Con_SubPrintf(console, "\n");
	}

	Con_SubPrintf(console, "\nFilter:\n");
	if (*filters.namefilter)
		Con_SubPrintf(console, "Contains: %s ", filters.namefilter);
	Con_SubPrintf(console, "^[Change Filter^]\n");
	Con_SubPrintf(console, "^[Maps^] %s\n", (filters.type!=2)?"shown":"hidden");
	Con_SubPrintf(console, "^[Mods^] %s\n", (filters.type!=1)?"shown":"hidden");
	Con_SubPrintf(console, "Sort: ^[Alphabetically^] ^[Date^] ^[Rating^]\n");
	if (filters.minrating == filters.maxrating)
	{
		char *gah[] = {"Any Rating", "Unrated", "1","2","3","4","5"};
		int i;
		Con_SubPrintf(console, "Rating");
		for (i = 0; i < countof(gah); i++)
		{
			if (i == filters.minrating+1)
				Con_SubPrintf(console, " %s", gah[i]);
			else
				Con_SubPrintf(console, " ^[%s^]", gah[i]);
		}
		Con_SubPrintf(console, "\n");
	}
	else
	{
		if (filters.minrating)
			Con_SubPrintf(console, "Min Rating: %i stars\n", filters.minrating);
		if (filters.maxrating)
			Con_SubPrintf(console, "Max Rating: %i stars\n", filters.maxrating);
	}
}

static void QI_UpdateFilter(char *filtertext)
{
	if (!strcmp(filtertext, "all"))
	{
		filters.type = 0;
		filters.minrating = filters.maxrating = -1;
		Q_strlcpy(filters.namefilter, "", sizeof(filters.namefilter));
	}
	else if (*filtertext == '>')
		filters.minrating = atoi(filtertext+1);
	else if (*filtertext == '<')
		filters.maxrating = atoi(filtertext+1);
	else if (*filtertext == '=')
		filters.minrating = filters.maxrating = atoi(filtertext+1);
	else if (!strcmp(filtertext, "any"))
	{
		filters.type = 0;
		filters.minrating = filters.maxrating = -1;
	}
	else if (!strcmp(filtertext, "maps"))
		filters.type = 1;
	else if (!strcmp(filtertext, "mods"))
		filters.type = 2;
	else
		Q_strlcpy(filters.namefilter, filtertext, sizeof(filters.namefilter));
}

static xmltree_t *QI_FindArchive(const char *name)
{
	xmltree_t *file;
	for (file = thedatabase->child; file; file = file->sibling)
	{
		const char *id = XML_GetParameter(file, "id", "unnamed");
		if (strcmp(file->name, "file"))
			continue;	//erk?

		if (!strcmp(id, name))
			return file;
	}
	return NULL;
}
static void QI_AddPackages(xmltree_t *qifile)
{
	const char *id;
	char extra[1024];
	char clean[512];
	const char *hash;
	unsigned int i;

	xmltree_t *tech;
	const char *basedir;
	xmltree_t *requires;
	xmltree_t *req;
	//quaddicted's database contains various zips that are meant to be extracted to various places.
	//this can either be the quake root (no path), a mod directory (typically the name of the mod), or the maps directory (id1/maps or some such)
	//unfortunately, quake files are relative to the subdir, so we need to strip the first subdir. anyone that tries to put dlls in there is evil. and if there isn't one, we need to get the engine to compensate.
	//we also need to clean up the paths so they're not badly formed
	tech = XML_ChildOfTree(qifile, "techinfo", 0);
	basedir = XML_GetChildBody(tech, "zipbasedir", "");

	//skip any dodgy leading slashes
	while (*basedir == '/' || *basedir == '\\')
		basedir++;
	if (!*basedir)
		strcpy(clean, "..");	//err, there wasn't a directory... we still need to 'strip' it though.
	else
	{
		//skip the gamedir
		while (*basedir && *basedir != '/' && *basedir != '\\')
			basedir++;
		//skip any trailing
		while (*basedir == '/' || *basedir == '\\')
			basedir++;
		for (i = 0; *basedir; i++)
		{
			if (i >= sizeof(clean)-1)
				break;
			if (*basedir == '\\')	//sigh
				clean[i] = '/';
			else
				clean[i] = *basedir;
			basedir++;
		}
		while (i > 0 && clean[i-1] == '/')
			i--;
		clean[i] = 0;
	}

	requires = XML_ChildOfTree(tech, "requirements", 0);
	if (requires)
	{
		for (i = 0; ; i++)
		{
			req = XML_ChildOfTree(requires, "file", i);
			if (!req)
				break;
			id = XML_GetParameter(req, "id", "unknown");
			QI_AddPackages(QI_FindArchive(id));
		}
	}

	hash = XML_GetChildBody(qifile, "md5hash", "");
	if (strchr(hash, '\\') || strchr(hash, '\"'))	//if it has a dodgy escape-breaking char then drop it.
		hash = "";

	id = XML_GetParameter(qifile, "id", "unknown");
	if (strchr(clean, '\\') || strchr(clean, '\"') || strchr(clean, '\n') || strchr(clean, ';'))
		return;
	if (strchr(id, '\\') || strchr(id, '\"') || strchr(id, '\n') || strchr(id, ';'))
		return;


	Q_snprintf(extra, sizeof(extra), " package \""FILEDOWNLOADURL"\" prefix \"%s\" hash \"md5:%s\"", id, clean, hash);
	cmdfuncs->AddText(extra, false);
}
static void QI_RunMap(xmltree_t *qifile, const char *map)
{
	char gamedir[65];
	char buf[256];
	if (!qifile)
	{
		return;
	}

	//type 1 (maps) often don't list any map names
	if (!*map)// && atoi(XML_GetParameter(qifile, "type", "0")) == 1)
		map = XML_GetParameter(qifile, "id", "unknown");
	if (!*map || strchr(map, '\\') || strchr(map, '\"') || strchr(map, '\n') || strchr(map, ';'))
		map = "";

	strcpy(gamedir, "id1");
	{
		char descbuf[1024];
		xmltree_t *tech = XML_ChildOfTree(qifile, "techinfo", 0);
		const char *cmdline = XML_GetChildBody(tech, "commandline", "");
		while (cmdline)
		{
			cmdline = cmdfuncs->ParseToken(cmdline, descbuf, sizeof(descbuf), NULL);
			if (!strcmp(descbuf, "-game"))
				cmdline = cmdfuncs->ParseToken(cmdline, gamedir, sizeof(gamedir), NULL);
			else if (!strcmp(descbuf, "-hipnotic") || !strcmp(descbuf, "-rogue") || !strcmp(descbuf, "-quoth"))
			{
				if (!*gamedir)
					strcpy(gamedir, descbuf+1);
			}
		}
	}

	cmdfuncs->AddText("fs_changemod", false);
	if (*gamedir)
	{
		cmdfuncs->AddText(" dir ", false);
		cmdfuncs->AddText(cmdfuncs->QuotedString(gamedir, buf, sizeof(buf), false), false);
	}
	if (map && *map)
	{
		cmdfuncs->AddText(" spmap ", false);
		cmdfuncs->AddText(cmdfuncs->QuotedString(map, buf, sizeof(buf), false), false);
	}
	QI_AddPackages(qifile);
//	Con_Printf("Command: %s\n", cmd);
	cmdfuncs->AddText("\n", false);
}


void VARGS VFS_PRINTF(vfsfile_t *vf, const char *format, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, format);
	vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	VFS_PUTS(vf, string);
}
static void QI_WriteUpdateList(xmltree_t *database, vfsfile_t *pminfo)
{
	xmltree_t *file;
	char descbuf[1024], *d, *nl;

	if (database)
	for (file = database->child; file; file = file->sibling)
	{
		const char *id = XML_GetParameter(file, "id", "unnamed");
		const char *rating = XML_GetParameter(file, "rating", "");
		int ratingnum = atoi(rating);
		const char *author = XML_GetChildBody(file, "author", "unknown");
		const char *desc = XML_GetChildBody(file, "description", "<NO DESCRIPTION>");
		const char *type = XML_GetParameter(file, "type", "");
		const char *date = XML_GetChildBody(file, "date", "1.1.1990");
		xmltree_t *tech = XML_ChildOfTree(file, "techinfo", 0);
		const char *cmdline = XML_GetChildBody(tech, "commandline", "");
		const char *zipbasedir = XML_GetChildBody(tech, "zipbasedir", "");
		int year, month, day;
		int startmapnum, i;
		char ratingtext[65];
		char gamedir[65];
		char prefix[MAX_QPATH];
		xmltree_t *startmap;
		if (strcmp(file->name, "file"))
			continue;	//erk?
		if (atoi(XML_GetParameter(file, "hide", "")) || atoi(XML_GetParameter(file, "fte_hide", "")))
			continue;
		switch(atoi(type))
		{
		case 1:
			type = "map";	//'single map file(s)'
			break;
		case 2:
			type = "mod";	//'Partial conversion'
			break;
		case 4:
			type = "spd";	//'speedmapping'
			break;
		case 5:
			type = "otr";	//'misc files'
			break;
		default:
			type = "???";	//no idea
			break;
		}

		if (ratingnum > (sizeof(ratingtext)-5)/6)
			ratingnum = (sizeof(ratingtext)-5)/6;
		if (ratingnum)
		{
			Q_snprintf(ratingtext, sizeof(ratingtext), "^a");
			for (i = 0; i < ratingnum; i++)
				Q_snprintf(ratingtext + i+2, sizeof(ratingtext)-i*2+2, "*");
			Q_snprintf(ratingtext + i+2, sizeof(ratingtext)-i*2+2, "^a");
		}
		else if (*rating)
			Q_snprintf(ratingtext, sizeof(ratingtext), "%s", rating);
		else
			Q_snprintf(ratingtext, sizeof(ratingtext), "%s", "unrated");


		day = atoi(date?date:"1");
		date = date?strchr(date, '.'):NULL;
		month = atoi(date?date+1:"1");
		date = date?strchr(date+1, '.'):NULL;
		year = atoi(date?date+1:"1990");
		if (year < 90)
			year += 2000;
		else if (year < 1900)
			year += 1900;

		strcpy(gamedir, "id1");
		while (cmdline)
		{
			cmdline = cmdfuncs->ParseToken(cmdline, descbuf, sizeof(descbuf), NULL);
			if (!strcmp(descbuf, "-game"))
				cmdline = cmdfuncs->ParseToken(cmdline, gamedir, sizeof(gamedir), NULL);
			else if (!strcmp(descbuf, "-hipnotic") || !strcmp(descbuf, "-rogue") || !strcmp(descbuf, "-quoth"))
			{
				if (!*gamedir)
					strcpy(gamedir, descbuf+1);
			}
		}
		if (!*gamedir)
			continue;	//bad package


		*descbuf = 0;
		QI_DeHTML(desc, true, descbuf + strlen(descbuf), sizeof(descbuf) - strlen(descbuf));
		desc = descbuf;

		VFS_PRINTF(pminfo, "{\n"
								"\tpackage \"qi_%s\"\n"
								"\ttitle \"%s %s\"\n"
								"\tcategory \"Quaddicted - %i\"\n"
								"\tlicense \"Unknown\"\n"
								"\tauthor \"Unknown\"\n"
								"\tqhash \"\"\n"
								,id, ratingtext, XML_GetChildBody(file, "title", "<NO TITLE>"), year);

		VFS_PRINTF(pminfo, 		"\tgamedir \"%s\"\n", gamedir);

		VFS_PRINTF(pminfo, 		"\tver %04u-%02u-%02u\n", year, month, day);
		if (!strchr(author, '\"') && !strchr(author, '\n'))
			VFS_PRINTF(pminfo, 		"\tauthor \"%s\"\n", author);
		VFS_PRINTF(pminfo, 		"\twebsite \""FILEREVIEWURL"\"\n", id);	//in lieu of an actual site, lets fill it with quaddicted's reviews.

		VFS_PRINTF(pminfo, 		"\turl \""FILEDOWNLOADURL"\"\n", id);


		//skip any dodgy leading slashes
		while (*zipbasedir == '/' || *zipbasedir == '\\')
			zipbasedir++;
		if (!*zipbasedir)
			strcpy(prefix, "..");	//err, there wasn't a directory... we still need to 'strip' it though.
		else
		{
			//skip the zip's gamedir
			while (*zipbasedir && *zipbasedir != '/' && *zipbasedir != '\\')
				zipbasedir++;
			//skip any trailing
			while (*zipbasedir == '/' || *zipbasedir == '\\')
				zipbasedir++;
			for (i = 0; *zipbasedir; i++)
			{
				if (i >= sizeof(prefix)-1)
					break;
				if (*zipbasedir == '\\')	//sigh
					prefix[i] = '/';
				else if (*zipbasedir == '\"' || *zipbasedir == '\n' || *zipbasedir == '\r')
					break;	//bad char...
				else
					prefix[i] = *zipbasedir;
				zipbasedir++;
			}
			while (i > 0 && prefix[i-1] == '/')
				i--;
			prefix[i] = 0;
		}
		if (*prefix)
			VFS_PRINTF(pminfo, 		"\tpackprefix \"%s\"\n", prefix);

		for(d = descbuf;;)
		{
			nl = strchr(d, '\n');
			if (nl)
				*nl++ = 0;
			VFS_PRINTF(pminfo, 		"\tdesc \"%s\"\n", d);
			if (!nl)
				break;
			d = nl;
		}

		//VFS_PRINTF(pminfo,		"\tpreview \""FILEIMAGEURL"\"\n", id);
		for (startmapnum = 0; ; startmapnum++)
		{
			startmap = XML_ChildOfTree(tech, "startmap", startmapnum);
			if (!startmap)
				break;

			VFS_PRINTF(pminfo, "\tmap \"%s\"\n", startmap->body);
		}
		if (!startmapnum)	//if there's no start maps listed, use the package's id instead. these are not intended for anything else.
			VFS_PRINTF(pminfo, "\tmap \"%s\"\n", id);
		VFS_PRINTF(pminfo, "}\n");
	}
}

static unsigned int QI_GetDate(xmltree_t *file)
{
	unsigned int day, month, year;
	const char *date = XML_GetChildBody(file, "date", "1.1.1990");
	day = atoi(date?date:"1");
	date = date?strchr(date, '.'):NULL;
	month = atoi(date?date+1:"1");
	date = date?strchr(date+1, '.'):NULL;
	year = atoi(date?date+1:"1990");
	if (year < 90)
		year += 2000;
	else if (year < 1900)
		year += 1900;
	return year*10000 + month*100 + day;
}
static int QDECL QI_ResortName (const void *v1, const void *v2)
{
	const char *n1 = XML_GetChildBody(*(xmltree_t*const*)v1, "title", "<NO TITLE>");
	const char *n2 = XML_GetChildBody(*(xmltree_t*const*)v2, "title", "<NO TITLE>");
	return strcasecmp(n1,n2);
}
static int QDECL QI_ResortDate (const void *v1, const void *v2)
{
	unsigned int d1 = QI_GetDate(*(xmltree_t*const*)v1);
	unsigned int d2 = QI_GetDate(*(xmltree_t*const*)v2);

	if (d1>d2)
		return 1;
	if (d1<d2)
		return -1;
	return QI_ResortName(v1,v2);
}
static int QDECL QI_ResortRating (const void *v1, const void *v2)
{
	int r1 = atoi(XML_GetParameter(*(xmltree_t*const*)v1, "rating", ""));
	int r2 = atoi(XML_GetParameter(*(xmltree_t*const*)v2, "rating", ""));

	if (r1>r2)
		return 1;
	if (r1<r2)
		return -1;
	return QI_ResortDate(v1,v2);	//equal... go by date.
}

static qboolean QI_Resort(int sorttype)
{
	xmltree_t *file;
	xmltree_t **files;
	unsigned int count;
	if (!thedatabase || !thedatabase->child)
		return true;

	for (count = 0, file = thedatabase->child; file; file = file->sibling)
		count++;
	files = malloc(sizeof(*files)*count);
	for (count = 0, file = thedatabase->child; file; file = file->sibling)
		files[count++] = file;
	if (sorttype == 0)
		qsort(files, count, sizeof(*files), QI_ResortName);
	else if (sorttype == 1)
		qsort(files, count, sizeof(*files), QI_ResortDate);
	else if (sorttype == 2)
		qsort(files, count, sizeof(*files), QI_ResortRating);
	thedatabase->child = NULL;
	while (count --> 0)
	{
		file = files[count];
		file->sibling = thedatabase->child;
		thedatabase->child = file;
	}
	QI_RefreshMapList(true);
	return true;
}
static qboolean QDECL QI_ConsoleLink(void)
{
	xmltree_t *file;
	char *map;
	char *id;
	char *e;
	char text[2048];
	char link[8192];
	cmdfuncs->Argv(0, text, sizeof(text));
	cmdfuncs->Argv(1, link, sizeof(link));

	if (!*link)
	{
		if (!strcmp(text, "Change Filter"))
		{
			const char *console = WINDOWNAME;
			confuncs->SetConsoleFloat(console, "linebuffered", true);
			confuncs->SetConsoleString(console, "footer", "Please enter filter:");
			return true;
		}
		if (!strcmp(text, "Maps"))
		{
			filters.type = (filters.type==2)?0:2;
			QI_RefreshMapList(true);
			return true;
		}
		if (!strcmp(text, "Mods"))
		{
			filters.type = (filters.type==1)?0:1;
			QI_RefreshMapList(true);
			return true;
		}
		if (!strcmp(text, "Any Rating"))
		{
			filters.minrating = filters.maxrating = -1;
			QI_RefreshMapList(true);
			return true;
		}
		if (atoi(text) || !strcmp(text, "Unrated"))
		{
			filters.minrating = filters.maxrating = atoi(text);
			QI_RefreshMapList(true);
			return true;
		}
		if (atoi(text) || !strcmp(text, "Alphabetically"))
			return QI_Resort(0);
		if (atoi(text) || !strcmp(text, "Date"))
			return QI_Resort(1);
		if (atoi(text) || !strcmp(text, "Rating"))
			return QI_Resort(2);
	}


	id = strstr(link, "\\id\\");
	map = strstr(link, "\\startmap\\");
	if (id)
	{
		id+=4;
		e = strchr(id, '\\');
		if (e)
			*e = 0;
		if (map)
		{
			map += 10;
			e = strchr(map, '\\');
			if (e)
				*e = 0;
		}
		else
			map = "";

		file = QI_FindArchive(id);
		if (!file)
		{
			Con_Printf("Unknown file \"%s\"\n", id);
			return true;
		}
		QI_RunMap(file, map);
		return true;
	}
	return false;
}
static void QDECL QI_Tick(double realtime, double gametime)
{
	if (dlcontext != -1)
	{
		qofs_t flen=0;
		if (dlcontext < 0 || filefuncs->GetLen(dlcontext, &flen))
		{
			int ofs = 0;
			char *file;
			qboolean archive = true;
			if (flen == 0)
			{
				filefuncs->Close(dlcontext);
				flen = filefuncs->Open("**plugconfig", &dlcontext, 1);
				if (dlcontext == -1)
				{	
					QI_RefreshMapList(false);

					if (packagemanager)
					{
						VFS_CLOSE(packagemanager);
						packagemanager = NULL;
					}
					return;
				}
				archive = false;
			}
			file = malloc(flen+1);
			file[flen] = 0;
			filefuncs->Read(dlcontext, file, flen);
			filefuncs->Close(dlcontext);
			if (archive)
			{
				filefuncs->Open("**plugconfig", &dlcontext, 2);
				if (dlcontext != -1)
				{
					filefuncs->Write(dlcontext, file, flen);
					filefuncs->Close(dlcontext);
				}
			}
			dlcontext = -1;
			do
			{
				if (thedatabase)
					XML_Destroy(thedatabase);
				thedatabase = XML_Parse(file, &ofs, flen, false, "");
			} while(thedatabase && !thedatabase->child);
			free(file);

			QI_RefreshMapList(false);

//			XML_ConPrintTree(thedatabase, "quadicted_xml", 0);
		}
	}
	else if (packagemanager)
	{
		if (!thedatabase && dlcontext == -1)
		{
			dlcontext = -2;
			if (filefuncs->Open(DATABASEURL, &dlcontext, 1) >= 0)
				return;
		}
		else
		{
			VFS_PRINTF(packagemanager, "version 3\n");
			QI_WriteUpdateList(thedatabase, packagemanager);
		}
		VFS_CLOSE(packagemanager);
		packagemanager = NULL;
	}
}

static int QDECL QI_ConExecuteCommand(qboolean isinsecure)
{
	char console[256];
	char filter[256];
	if (isinsecure)
		return false;

	cmdfuncs->Argv(0, console, sizeof(console));
	cmdfuncs->Args(filter, sizeof(filter));
	QI_UpdateFilter(filter);

	QI_RefreshMapList(true);
	confuncs->SetConsoleFloat(console, "linebuffered", false);
	confuncs->SetConsoleString(console, "footer", "");
	return true;
}

static void QI_ExecuteCommand_f(void)
{
	char cmd[256];
	if (cmdfuncs->Argc() > 1)
	{
		cmdfuncs->Args(cmd, sizeof(cmd));
		QI_UpdateFilter(cmd);
	}
	else if (QI_SetupWindow(WINDOWNAME, false))
	{
		confuncs->SetActive(WINDOWNAME);
		return;
	}

	if (!thedatabase && dlcontext == -1)
		filefuncs->Open(DATABASEURL, &dlcontext, 1);

	QI_RefreshMapList(true);
}

void QI_GenPackages(const char *url, vfsfile_t *pipe, qboolean favourcache)
{
	if (thedatabase)
	{	//already read it?... fine.
		VFS_PRINTF(pipe, "version 3\n");
		QI_WriteUpdateList(thedatabase, pipe);
		VFS_CLOSE(pipe);
		return;
	}
	else if (favourcache)
	{	//just read the cached copy from disk. use the menu to update it.
		xmltree_t *t = NULL;
		qhandle_t fd = -1;
		int ofs = 0;

		int flen = filefuncs->Open("**plugconfig", &fd, 1);
		if (flen >= 0)
		{
			qbyte *file = malloc(flen+1);
			file[flen] = 0;
			filefuncs->Read(fd, file, flen);
			filefuncs->Close(fd);

			do
			{
				if (t)
					XML_Destroy(t);
				t = XML_Parse(file, &ofs, flen, false, "");
			} while(t && !t->child);
			free(file);

			VFS_PRINTF(pipe, "version 3\n");
			QI_WriteUpdateList(t, pipe);

			if (t)
				XML_Destroy(t);
			VFS_CLOSE(pipe);
			return;
		}
	}

	//fill it in once we've got it...
	if (packagemanager)
		VFS_CLOSE(packagemanager);
	packagemanager = pipe;
}
static plugupdatesourcefuncs_t sourcefuncs =
{
	"Quaddicted Maps",
	QI_GenPackages
};

qboolean Plug_Init(void)
{
	confuncs = plugfuncs->GetEngineInterface(plugsubconsolefuncs_name, sizeof(*confuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	clientfuncs = plugfuncs->GetEngineInterface(plugclientfuncs_name, sizeof(*clientfuncs));

	plugfuncs->ExportInterface(plugupdatesourcefuncs_name, &sourcefuncs, sizeof(sourcefuncs));

	if (confuncs && filefuncs && clientfuncs)
	{
		filters.minrating = filters.maxrating = -1;
		plugfuncs->ExportFunction("UpdateVideo", QI_UpdateVideo);
		if (plugfuncs->ExportFunction("Tick", QI_Tick) &&
			plugfuncs->ExportFunction("Shutdown", QI_Shutdown) &&
			plugfuncs->ExportFunction("ConExecuteCommand", QI_ConExecuteCommand) &&
			plugfuncs->ExportFunction("ConsoleLink", QI_ConsoleLink))
		{
			cmdfuncs->AddCommand("qi", QI_ExecuteCommand_f, "Select or install maps or mods from Quaddicted's database.");
			cmdfuncs->AddCommand("quaddicted", QI_ExecuteCommand_f, "Select or install maps or mods from Quaddicted's database.");
			return true;
		}
	}
	//else not available in dedicated servers
	return false;
}