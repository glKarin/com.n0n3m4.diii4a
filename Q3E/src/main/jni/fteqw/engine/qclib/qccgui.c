
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <stdio.h>
#include <sys/stat.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "qcc.h"
#include "gui.h"

//#define AVAIL_PNGLIB
//#define AVAIL_ZLIB

#define EMBEDDEBUG

#define IDI_ICON_FTEQCC MAKEINTRESOURCE(101)

void OptionsDialog(void);
static void GUI_CreateInstaller_Windows(void);
static void GUI_CreateInstaller_Android(void);
static void SetProgsSrcFileAndPath(char *filename);
static void CreateOutputWindow(pbool doannoates);
void AddSourceFile(const char *parentsrc, const char *filename);

#ifndef TVM_SETBKCOLOR
#define TVM_SETBKCOLOR              (TV_FIRST + 29)
#endif
#ifndef TreeView_SetBkColor
#define TreeView_SetBkColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETBKCOLOR, 0, (LPARAM)(clr))
#endif


#ifndef TTF_TRACK
#define TTF_TRACK			0x0020
#endif
#ifndef TTF_ABSOLUTE
#define TTF_ABSOLUTE		0x0080
#endif
#ifndef TTM_SETMAXTIPWIDTH
#define TTM_SETMAXTIPWIDTH	(WM_USER + 24)
#endif
#ifndef TTM_TRACKACTIVATE
#define TTM_TRACKACTIVATE	(WM_USER + 17)
#endif
#ifndef TTM_TRACKPOSITION
#define TTM_TRACKPOSITION	(WM_USER + 18)
#endif


//scintilla stuff
#define SCI_GETLENGTH 2006
#define SCI_GETCHARAT 2007
#define SCI_GETCURRENTPOS 2008
#define SCI_GETANCHOR 2009
#define SCI_REDO 2011
#define SCI_SETSAVEPOINT 2014
#define SCI_CANREDO 2016
#define SCI_GETCURLINE 2027
#define SCI_CONVERTEOLS 2029
#define SC_EOL_CRLF 0
#define SC_EOL_CR 1
#define SC_EOL_LF 2
#define SCI_SETEOLMODE 2031
#define SCI_SETTABWIDTH 2036
#define SCI_SETCODEPAGE 2037
#define SCI_MARKERDEFINE 2040
#define SCI_MARKERSETFORE 2041
#define SCI_MARKERSETBACK 2042
#define SCI_MARKERADD 2043
#define SCI_MARKERDELETE 2044
#define SCI_MARKERDELETEALL 2045
#define SCI_MARKERSETALPHA 2476
#define SCI_MARKERGET 2046
#define SCI_MARKERNEXT 2047
#define SCI_STYLECLEARALL 2050
#define SCI_STYLESETFORE 2051
#define SCI_STYLESETBACK 2052
#define SCI_STYLESETBOLD 2053
#define SCI_STYLESETITALIC 2054
#define SCI_STYLESETSIZE 2055
#define SCI_STYLESETFONT 2056
#define SCI_STYLERESETDEFAULT 2058
#define SCI_STYLESETUNDERLINE 2059
#define SCI_STYLESETCASE 2060
#define SCI_AUTOCSHOW 2100
#define SCI_AUTOCCANCEL 2101
#define SCI_AUTOCACTIVE 2102
#define SCI_AUTOCSETFILLUPS 2112
#define SCI_GETLINE 2153
#define SCI_SETSEL 2160
#define SCI_GETSELTEXT 2161
#define SCI_LINEFROMPOSITION 2166
#define SCI_POSITIONFROMLINE 2167
#define SCI_REPLACESEL 2170
#define SCI_CANUNDO 2174
#define SCI_UNDO 2176
#define SCI_CUT 2177
#define SCI_COPY 2178
#define SCI_PASTE 2179
#define SCI_SETTEXT 2181
#define SCI_GETTEXT 2182
#define SCI_CALLTIPSHOW 2200
#define SCI_CALLTIPCANCEL 2201
#define SCI_TOGGLEFOLD 2231
#define SCI_SETMARGINWIDTHN 2242
#define SCI_SETMARGINMASKN 2244
#define SCI_SETMARGINSENSITIVEN 2246
#define SCI_SETMOUSEDWELLTIME 2264
#define SCI_CHARLEFT 2304
#define SCI_CHARRIGHT 2306
#define SCI_BACKTAB 2328 
#define SCI_SEARCHANCHOR 2366
#define SCI_SEARCHNEXT 2367
#define SCI_SEARCHPREV 2368
#define SCI_STYLEGETFORE 2481
#define SCI_STYLEGETBACK 2482
#define SCI_STYLEGETBOLD 2483
#define SCI_STYLEGETITALIC 2484
#define SCI_STYLEGETSIZE 2485
#define SCI_STYLEGETFONT 2486
#define SCI_STYLEGETUNDERLINE 2488
#define SCI_STYLEGETCASE 2489
#define SCI_BRACEHIGHLIGHTINDICATOR 2498
#define SCI_BRACEBADLIGHTINDICATOR 2499
#define SCI_LINELENGTH 2350
#define SCI_BRACEHIGHLIGHT 2351
#define SCI_BRACEBADLIGHT 2352
#define SCI_BRACEMATCH 2353
#define SCI_SETVIEWEOL 2356
#define SCI_USEPOPUP 2371
#define SCI_ANNOTATIONSETTEXT 2540
#define SCI_ANNOTATIONGETTEXT 2541
#define SCI_ANNOTATIONSETSTYLE 2542
#define SCI_ANNOTATIONGETSTYLE 2543
#define SCI_ANNOTATIONSETSTYLES 2544
#define SCI_ANNOTATIONGETSTYLES 2545
#define SCI_ANNOTATIONGETLINES 2546
#define SCI_ANNOTATIONCLEARALL 2547
#define ANNOTATION_HIDDEN 0
#define ANNOTATION_STANDARD 1
#define ANNOTATION_BOXED 2
#define ANNOTATION_INDENTED 3
#define SCI_ANNOTATIONSETVISIBLE 2548
#define SCI_ANNOTATIONGETVISIBLE 2549
#define SCI_ANNOTATIONSETSTYLEOFFSET 2550
#define SCI_ANNOTATIONGETSTYLEOFFSET 2551
#define SCI_AUTOCSETORDER 2660
#define SCI_SETREPRESENTATION 2665
#define SCI_SETLEXER 4001
#define SCI_SETPROPERTY 4004
#define SCI_SETKEYWORDS 4005

#define SC_ORDER_PERFORMSORT 1

#define SC_CP_UTF8 65001
#define SCLEX_CPP 3

#define SCE_C_DEFAULT 0
#define SCE_C_COMMENT 1
#define SCE_C_COMMENTLINE 2
#define SCE_C_COMMENTDOC 3
#define SCE_C_NUMBER 4
#define SCE_C_WORD 5
#define SCE_C_STRING 6
#define SCE_C_CHARACTER 7
#define SCE_C_UUID 8
#define SCE_C_PREPROCESSOR 9
#define SCE_C_OPERATOR 10
#define SCE_C_IDENTIFIER 11
#define SCE_C_STRINGEOL 12
#define SCE_C_VERBATIM 13
#define SCE_C_REGEX 14
#define SCE_C_COMMENTLINEDOC 15
#define SCE_C_WORD2 16
#define SCE_C_COMMENTDOCKEYWORD 17
#define SCE_C_COMMENTDOCKEYWORDERROR 18
#define SCE_C_GLOBALCLASS 19
#define SCE_C_STRINGRAW 20
#define SCE_C_TRIPLEVERBATIM 21
#define SCE_C_HASHQUOTEDSTRING 22
#define SCE_C_PREPROCESSORCOMMENT 23
#define SCE_C_PREPROCESSORCOMMENTDOC 24
#define SCE_C_USERLITERAL 25
#define SCE_C_TASKMARKER 26
#define SCE_C_ESCAPESEQUENCE 27

#define STYLE_DEFAULT 32
#define STYLE_BRACELIGHT 34
#define STYLE_BRACEBAD 35
#define STYLE_LASTPREDEFINED 39

#define SC_MARKNUM_FOLDEREND 25
#define SC_MARKNUM_FOLDEROPENMID 26
#define SC_MARKNUM_FOLDERMIDTAIL 27
#define SC_MARKNUM_FOLDERTAIL 28
#define SC_MARKNUM_FOLDERSUB 29
#define SC_MARKNUM_FOLDER 30
#define SC_MARKNUM_FOLDEROPEN 31
#define SC_MASK_FOLDERS 0xFE000000
#define SC_MARK_CIRCLE 0
#define SC_MARK_ROUNDRECT 1
#define SC_MARK_ARROW 2
#define SC_MARK_SMALLRECT 3
#define SC_MARK_SHORTARROW 4
#define SC_MARK_EMPTY 5
#define SC_MARK_ARROWDOWN 6
#define SC_MARK_MINUS 7
#define SC_MARK_PLUS 8
#define SC_MARK_VLINE 9
#define SC_MARK_LCORNER 10
#define SC_MARK_TCORNER 11
#define SC_MARK_BOXPLUS 12
#define SC_MARK_BOXPLUSCONNECTED 13
#define SC_MARK_BOXMINUS 14
#define SC_MARK_BOXMINUSCONNECTED 15
#define SC_MARK_LCORNERCURVE 16
#define SC_MARK_TCORNERCURVE 17
#define SC_MARK_CIRCLEPLUS 18
#define SC_MARK_CIRCLEPLUSCONNECTED 19
#define SC_MARK_CIRCLEMINUS 20
#define SC_MARK_CIRCLEMINUSCONNECTED 21
#define SC_MARK_BACKGROUND 22
#define SC_MARK_DOTDOTDOT 23
#define SC_MARK_ARROWS 24
#define SC_MARK_PIXMAP 25
#define SC_MARK_FULLRECT 26
#define SC_MARK_LEFTRECT 27
#define SC_MARK_AVAILABLE 28
#define SC_MARK_UNDERLINE 29
#define SC_MARK_RGBAIMAGE 30
#define SC_MARK_BOOKMARK 31

#define SCN_CHARADDED 2001
#define SCN_SAVEPOINTREACHED 2002
#define SCN_SAVEPOINTLEFT 2003
#define SCN_UPDATEUI 2007
#define SCN_MARGINCLICK 2010
#define SCN_DWELLSTART 2016
#define SCN_DWELLEND 2017
#define SCN_FOCUSOUT 2029

struct SCNotification {
	NMHDR nmhdr;
	int position;
	int ch;
	int modifiers;
	int modificationType;
	const char *text;
	int length;
	int linesAdded;
	int message;
	DWORD_PTR wParam;
	LONG_PTR lParam;
	int line;
	int foldLevelNow;
	int foldLevelPrev;
	int margin;
	int listType;
	int x;
	int y;
	int token;
	int annotationLinesAdded;
	int updated;
};


//these all run on the main thread
typedef struct editor_s {
	char filename[MAX_PATH];	//abs
	HWND window;
	HWND editpane;
	HWND tooltip;
	char tooltiptext[1024];
	int curline;
	pbool modified;
	pbool scintilla;
	int savefmt;
	time_t filemodifiedtime;
	struct editor_s *next;

	//for avoiding silly redraws etc when titles don't actually change...
	int oldsavefmt;
	int oldline;
} editor_t;
editor_t *editors;

typedef struct
{
	editor_t *editor;	//will need to be validated
	unsigned int selpos;
	unsigned int anchorpos;
} navhistory_t;
navhistory_t navhistory[8];
const unsigned int navhistory_size = sizeof(navhistory)/sizeof(navhistory[0]);
unsigned int navhistory_first;	//don't allow rewinding past this.
unsigned int navhistory_pos;

//the engine thread simply sits waiting for responses from the engine
typedef struct
{
	int pipeclosed;
	DWORD tid;
	HWND window;
	HWND refocuswindow;
	HANDLE thread;
	HANDLE pipefromengine;
	HANDLE pipetoengine;
	size_t embedtype;	//0 = not. 1 = separate. 2 = mdi child
} enginewindow_t;
static pbool EngineCommandf(char *message, ...);
static void EngineGiveFocus(void);

/*
static pbool QCC_RegGetStringValue(HKEY base, char *keyname, char *valuename, void *data, int datalen)
{
	pbool result = false;
	HKEY subkey;
	DWORD type = REG_NONE;
	if (RegOpenKeyEx(base, keyname, 0, KEY_READ, &subkey) == ERROR_SUCCESS)
	{
		DWORD dwlen = datalen-1;
		result = ERROR_SUCCESS == RegQueryValueEx(subkey, valuename, NULL, &type, data, &dwlen);
		datalen = dwlen;
		RegCloseKey (subkey);
	}

	if (type == REG_SZ || type == REG_EXPAND_SZ)
		((char*)data)[datalen] = 0;
	else
		((char*)data)[0] = 0;
	return result;
}
static pbool QCC_RegSetValue(HKEY base, char *keyname, char *valuename, int type, void *data, int datalen)
{
	pbool result = false;
	HKEY subkey;

	if (RegCreateKeyEx(base, keyname, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subkey, NULL) == ERROR_SUCCESS)
	{
		if (ERROR_SUCCESS == RegSetValueEx(subkey, valuename, 0, type, data, datalen))
			result = true;
		RegCloseKey (subkey);
	}
	return result;
}
*/

#undef printf
#undef Sys_Error
void Sys_Error(const char *text, ...);

extern pbool qcc_vfiles_changed;
extern vfile_t *qcc_vfiles;
HWND mainwindow;
HINSTANCE ghInstance;
static INT CALLBACK StupidBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData) ;
void QCC_SaveVFiles(void)
{
	vfile_t *f;
	if (qcc_vfiles_changed)
	{
		switch (MessageBox(mainwindow, "Save files as archive?", "FTEQCCGUI", MB_YESNOCANCEL))
		{
		case IDYES:
			{
				char filename[MAX_PATH];
				char oldpath[MAX_PATH+10];
				OPENFILENAME ofn;
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hInstance = ghInstance;
				ofn.lpstrFile = filename;
				ofn.lpstrTitle = "Output archive";
				ofn.nMaxFile = sizeof(filename)-1;
				ofn.lpstrFilter = "QuakeC Projects\0*.zip\0All files\0*.*\0";
				memset(filename, 0, sizeof(filename));
				GetCurrentDirectory(sizeof(oldpath)-1, oldpath);
				ofn.lpstrInitialDir = oldpath;
				if (GetSaveFileName(&ofn))
				{
					int h = SafeOpenWrite(ofn.lpstrFile, -1);

					progfuncs_t funcs;
					progexterns_t ext;
					memset(&funcs, 0, sizeof(funcs));
					funcs.funcs.parms = &ext;
					memset(&ext, 0, sizeof(ext));
					ext.ReadFile = GUIReadFile;
					ext.FileSize = GUIFileSize;
					ext.WriteFile = QCC_WriteFile;
					ext.Sys_Error = Sys_Error;
					ext.Printf = GUIprintf;

					qccprogfuncs = &funcs;
					WriteSourceFiles(qcc_vfiles, h, true, false);
					qccprogfuncs = NULL;

					SafeClose(h);

					qcc_vfiles_changed = false;
					return;
				}
			}
			break;
		case IDNO:
			{
				char oldworkingdir[MAX_PATH], newdir[MAX_PATH+10], workingdir[MAX_PATH];
				BROWSEINFO bi;
				LPITEMIDLIST il;
				memset(&bi, 0, sizeof(bi));
				bi.hwndOwner = mainwindow;
				bi.pidlRoot = NULL;
				GetCurrentDirectory(sizeof(oldworkingdir)-1, oldworkingdir);
				GetCurrentDirectory(sizeof(workingdir)-1, workingdir);
				bi.pszDisplayName = workingdir;
				bi.lpszTitle = "Where do you want the source?";
				bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT;
				bi.lpfn = StupidBrowseCallbackProc;
				bi.lParam = 0;
				bi.iImage = 0;
				il = SHBrowseForFolder(&bi);
				if (il)
				{
					SHGetPathFromIDList(il, newdir);
					CoTaskMemFree(il);

					for (f = qcc_vfiles; f; f = f->next)
					{
						char nname[MAX_PATH];
						int h;
						QC_snprintfz(nname, sizeof(nname), "%s\\%s", newdir, f->filename);
						h = SafeOpenWrite(f->filename, -1);
						if (h >= 0)
						{
							SafeWrite(h, f->file, f->size);
							SafeClose(h);
						}
					}
				}
				SetCurrentDirectory(oldworkingdir);	//revert microsoft stupidity.
			}
			break;
		default:
			return;
		}
	}
}

void QCC_EnumerateFilesResult(const char *name, const void *compdata, size_t compsize, int method, size_t plainsize)
{
	void *buffer = malloc(plainsize);
	if (QC_decode(NULL, compsize, plainsize, method, compdata, buffer))
		QCC_AddVFile(name, buffer, plainsize);

	free(buffer);
}

/*
==============
LoadFile
==============
*/
static void *QCC_ReadFile(const char *fname, unsigned char *(*buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size)
//unsigned char *PDECL QCC_ReadFile (const char *fname, void *buffer, int len, size_t *sz)
{
	size_t len;
	FILE *f;
	char *buffer;
	vfile_t *v = QCC_FindVFile(fname);
	if (v)
	{
		len = v->size;
		if (buf_get)
			buffer = buf_get(buf_ctx, len+1);
		else
			buffer = malloc(len+1);
		if (!buffer)
			return NULL;
		((char*)buffer)[len] = 0;
		if (len > v->size)
			len = v->size;
		memcpy(buffer, v->file, len);
		if (out_size)
			*out_size = len;
		return buffer;
	}

	f = fopen(fname, "rb");
	if (!f)
	{
		if (out_size)
			*out_size = 0;
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (buf_get)
		buffer = buf_get(buf_ctx, len+1);
	else
		buffer = malloc(len+1);
	((char*)buffer)[len] = 0;
	if (len != fread(buffer, 1, len, f))
	{
		if (!buf_get)
			free(buffer);
		buffer = NULL;
	}
	fclose(f);

	if (out_size)
		*out_size = len;
	return buffer;
}
int PDECL QCC_RawFileSize (const char *fname)
{
	long    length;
	FILE *f;

	vfile_t *v = QCC_FindVFile(fname);
	if (v)
		return v->size;

	f = fopen(fname, "rb");
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fclose(f);
	return length;
}
int PDECL QCC_PopFileSize (const char *fname)
{
	extern int qcc_compileactive;
	int len = QCC_RawFileSize(fname);
	if (len >= 0 && qcc_compileactive)
	{
		AddSourceFile(compilingrootfile,	fname);
	}
	return len;
}

#ifdef AVAIL_ZLIB
#include "../libs/zlib.h"
#endif

pbool PDECL QCC_WriteFileW (const char *name, wchar_t *data, int maxchars)
{
	char *u8start = malloc(3+maxchars*4+1);
	char *u8 = u8start;
	int offset;
	pbool result = false;
	unsigned int inc;
	FILE *f;

	//start with the bom
	//lets just always write a BOM when the file contains something outside ascii. it'll just be more robust when microsoft refuse to use utf8 by default.
	//its just much less likely to fuck up when people use notepad/wordpad. :s
	inc = 0xfeff;
	*u8++ = ((inc>>12) & 0xf) | 0xe0;
	*u8++ = ((inc>>6) & 0x3f) | 0x80;
	*u8++ = ((inc>>0) & 0x3f) | 0x80;
	offset = u8-u8start;	//assume its not needed. will set to 0 if it is.

	while(*data)
	{
		inc = *data++;
		//handle surrogates
		if (inc >= 0xd800u && inc < 0xdc00u)
		{
			unsigned int l = *data;
			if (l >= 0xdc00u && l < 0xe000u)
			{
				data++;
				inc = (((inc & 0x3ffu)<<10) | (l & 0x3ffu)) + 0x10000;
			}
		}
		if (inc <= 127)
			*u8++ = inc;
		else 
		{
			offset = 0;
			if (inc <= 0x7ff)
			{
				*u8++ = ((inc>>6) & 0x1f) | 0xc0;
				*u8++ = ((inc>>0) & 0x3f) | 0x80;
			}
			else if (inc <= 0xffff)
			{
				*u8++ = ((inc>>12) & 0xf) | 0xe0;
				*u8++ = ((inc>>6) & 0x3f) | 0x80;
				*u8++ = ((inc>>0) & 0x3f) | 0x80;
			}
			else if (inc <= 0x1fffff)
			{
				*u8++ = ((inc>>18) & 0x07) | 0xf0;
				*u8++ = ((inc>>12) & 0x3f) | 0x80;
				*u8++ = ((inc>> 6) & 0x3f) | 0x80;
				*u8++ = ((inc>> 0) & 0x3f) | 0x80;
			}
			else
			{
				inc = 0xFFFD;
				*u8++ = ((inc>>12) & 0xf) | 0xe0;
				*u8++ = ((inc>>6) & 0x3f) | 0x80;
				*u8++ = ((inc>>0) & 0x3f) | 0x80;
			}
		}
	}

	f = fopen(name, "wb");
	if (f)
	{
		result = fwrite(u8start+offset, 1, u8-(u8start+offset), f) == (u8-(u8start+offset));
		fclose(f);
	}
	free(u8start);
	return result;
}

pbool PDECL QCC_WriteFile (const char *name, void *data, int len)
{
	long    length;
	FILE *f;

	char *ext = strrchr(name, '.');
	if (ext && !stricmp(ext, ".gz"))
	{
#ifdef AVAIL_ZLIB
		pbool okay = true;
		char out[1024*8];

		z_stream strm = {
			data,
			len,
			0,

			out,
			sizeof(out),
			0,

			NULL,
			NULL,

			NULL,
			NULL,
			NULL,

			Z_BINARY,
			0,
			0
		};

		f = fopen(name, "wb");
		if (!f)
			return false;
		deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS|16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		while(okay && deflate(&strm, Z_FINISH) == Z_OK)
		{
			if (sizeof(out) - strm.avail_out != fwrite(out, 1, sizeof(out) - strm.avail_out, f))
				okay = false;
			strm.next_out = out;
			strm.avail_out = sizeof(out);
		}
		if (sizeof(out) - strm.avail_out != fwrite(out, 1, sizeof(out) - strm.avail_out, f))
			okay = false;
		deflateEnd(&strm);
		fclose(f);
		if (!okay)
			unlink(name);
		return okay;
#else
		return false;
#endif
	}

	if (QCC_FindVFile(name))
		return !!QCC_AddVFile(name, data, len);

	f = fopen(name, "wb");
	if (!f)
		return false;
	length = fwrite(data, 1, len, f);
	fclose(f);

	if (length != len)
		return false;

	return true;
}

#undef printf
#undef Sys_Error

void Sys_Error(const char *text, ...)
{
	va_list argptr;
	static char msg[2048];	

	va_start (argptr,text);
	QC_vsnprintf (msg,sizeof(msg)-1, text,argptr);
	va_end (argptr);

	QCC_Error(ERR_INTERNAL, "%s", msg);
}


FILE *logfile;
int logprintf(const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];

	va_start (argptr, format);
#ifdef _WIN32
	_vsnprintf (string,sizeof(string)-1, format,argptr);
#else
	vsnprintf (string,sizeof(string), format,argptr);
#endif
	va_end (argptr);

	printf("%s", string);
	if (logfile)
		fputs(string, logfile);

	return 0;
}











#define Edit_Redo(hwndCtl)                      ((BOOL)(DWORD)SNDMSG((hwndCtl), EM_REDO, 0L, 0L))


#define MAIN_WINDOW_CLASS_NAME "FTEMAINWINDOW"
#define MDI_WINDOW_CLASS_NAME "FTEMDIWINDOW"
#define EDIT_WINDOW_CLASS_NAME "FTEEDITWINDOW"
#define OPTIONS_WINDOW_CLASS_NAME "FTEOPTIONSWINDOW"
#define ENGINE_WINDOW_CLASS_NAME "FTEEMBEDDEDWINDOW"

#define EM_GETSCROLLPOS  (WM_USER + 221)
#define EM_SETSCROLLPOS  (WM_USER + 222)



int GUIprintf(const char *msg, ...);
void GUIPrint(HWND wnd, char *msg);

char finddef[256];
char greptext[256];
extern pbool fl_extramargins;
extern int fl_tabsize;
extern char enginebinary[MAX_OSPATH];
extern char enginebasedir[MAX_OSPATH];
extern char enginecommandline[8192];
extern QCC_def_t *sourcefilesdefs[];
extern int sourcefilesnumdefs;

void RunCompiler(char *args, pbool quick);
void RunEngine(void);

HINSTANCE ghInstance;
HMODULE richedit;
HMODULE scintilla;

pbool resetprogssrc;	//progs.src was changed, reload project info.

HWND mainwindow;
HWND gamewindow;
HWND mdibox;
HWND watches;
HWND optionsmenu;
HWND outputbox;
HWND projecttree;
HWND search_name;
HACCEL accelerators;


//our splitter...
#define SPLITTER_SIZE 4
static struct splits_s
{
	HWND wnd;
	HWND splitter;
	int minsize;
	int cury;
	int cursize;
	float frac;

} *splits;
static size_t numsplits;
static RECT splitterrect;

static struct splits_s *SplitterGet(HWND id)
{
	size_t s;
	for (s = 0; s < numsplits; s++)
	{
		if (splits[s].wnd == id)
			return &splits[s];
	}
	return NULL;
}
static int SplitterShrinkPrior(size_t s, int px)
{
	int found = 0;
	int avail;
	for (; px && s > 0; s--)
	{
		avail = splits[s].cursize - splits[s].minsize;
		if (avail > px)
			avail = px;

		splits[s].cursize -= avail;
		found += avail;
		px -= avail;
	}

	if (px)
	{
		avail = splits[0].cursize - splits[0].minsize;
		if (avail > px)
			avail = px;

		splits[0].cursize -= avail;
		found += avail;
		px -= avail;
	}

	return found;
}
static int SplitterShrinkNext(size_t s, int px)
{
	int found = 0;
	int avail;
	for (; px && s < numsplits; s++)
	{
		avail = splits[s].cursize - splits[s].minsize;
		if (avail > px)
			avail = px;

		splits[s].cursize -= avail;
		found += avail;
		px -= avail;
	}
	return found;
}
static void SplitterUpdate(void);
static LRESULT CALLBACK SplitterWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	size_t s;
	PAINTSTRUCT ps;
	RECT rect;
	int y;
	switch(message)
	{
	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		return TRUE;
	case WM_MOUSEMOVE:
		if (wParam & MK_LBUTTON)
			if (GetCapture() == hWnd)
				goto doresize;
		return true;
	case WM_LBUTTONUP:
		ReleaseCapture();
	doresize:
		y = GET_Y_LPARAM(lParam);
		GetClientRect(hWnd, &rect);
		y = y - rect.top - SPLITTER_SIZE/2;
		for (s = 1; s < numsplits; s++)
		{
			if (splits[s].splitter == hWnd)
			{
				if (y < 0)
					splits[s].cursize += SplitterShrinkPrior(s-1, -y);
				else
					splits[s-1].cursize += SplitterShrinkNext(s, y);
				SplitterUpdate();
				break;
			}
		}
		return TRUE;
	case WM_PAINT:
		BeginPaint(hWnd,(LPPAINTSTRUCT)&ps);
		EndPaint(hWnd,(LPPAINTSTRUCT)&ps);
		return TRUE;
	default:
		return DefWindowProc(hWnd,message,wParam,lParam);
	}
}
static void SplitterUpdate(void)
{
	int y = 0;
	size_t s;
	if (!numsplits)
		return;

	y = splitterrect.bottom-splitterrect.top;

	//now figure out their positions relative to that
	for (s = numsplits; s-- > 1; )
	{
		y -= splits[s].cursize;
		splits[s].cury = y;
		y -= SPLITTER_SIZE;
	}

	splits[0].cursize = y;
	splits[0].cury = 0;
	if (splits[0].cursize < splits[0].minsize)
		splits[0].cursize += SplitterShrinkNext(1, splits[0].minsize-splits[0].cursize);

	for (s = 0; s < numsplits; s++)
	{
		if (s)
		{
			if (!splits[s].splitter)
			{
				WNDCLASSA wclass;
				wclass.style = 0;
				wclass.lpfnWndProc = SplitterWndProc;
				wclass.cbClsExtra = 0;
				wclass.cbWndExtra = 0;
				wclass.hInstance = ghInstance;
				wclass.hIcon = NULL;
				wclass.hCursor = LoadCursor(0, IDC_SIZENS);
				wclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
				wclass.lpszMenuName = NULL;
				wclass.lpszClassName = "splitter";
				RegisterClassA(&wclass);
				splits[s].splitter = CreateWindowExA(0, wclass.lpszClassName, "", WS_CHILD|WS_VISIBLE, splitterrect.left, splitterrect.top+splits[s].cury-SPLITTER_SIZE, splitterrect.right-splitterrect.left, SPLITTER_SIZE, mainwindow, NULL, ghInstance, NULL);
			}
			else
				SetWindowPos(splits[s].splitter, HWND_TOP, splitterrect.left, splitterrect.top+splits[s].cury-SPLITTER_SIZE, splitterrect.right-splitterrect.left, SPLITTER_SIZE, SWP_NOZORDER);
		}
		else
		{
			if (splits[s].splitter)
			{
				DestroyWindow(splits[s].splitter);
				splits[s].splitter = NULL;
			}
		}
		SetWindowPos(splits[s].wnd, HWND_TOP, splitterrect.left, splitterrect.top+splits[s].cury, splitterrect.right-splitterrect.left, splits[s].cursize, SWP_NOZORDER);
	}
}
static void SplitterAdd(HWND w, int minsize, int newsize)
{
	struct splits_s *n = malloc(sizeof(*n)*(numsplits+1));
	memcpy(n, splits, sizeof(*n)*numsplits);
	free(splits);
	splits = n;
	n += numsplits;

	n->wnd = w;
	n->splitter = NULL;
	n->minsize = minsize;
	n->cursize = newsize;
	n->cury = 0;

	numsplits++;

	SplitterUpdate();
	ShowWindow(w, SW_SHOW);
}
//adds if needed.
static void SplitterFocus(HWND w, int minsize, int newsize)
{
	struct splits_s *s = SplitterGet(w);
	if (s)
	{
		if (s->cursize < newsize)
		{
			s->cursize += SplitterShrinkPrior(s-splits-1, (newsize-s->cursize)/2);
			if (s->cursize < newsize)
				s->cursize += SplitterShrinkNext(s-splits+1, newsize-s->cursize);
			if (s->cursize < newsize)
				s->cursize += SplitterShrinkPrior(s-splits-1, newsize-s->cursize);
			SplitterUpdate();
		}
	}
	else
		SplitterAdd(w, minsize, newsize);

	SetFocus(w);
}
static void SplitterRemove(HWND w)
{
	struct splits_s *s = SplitterGet(w);
	size_t idx;
	if (!s)
		return;
	if (s->splitter)
		DestroyWindow(s->splitter);
	idx = s-splits;
	numsplits--;
	memmove(splits+idx, splits+idx+1, sizeof(*s)*(numsplits-idx));

	ShowWindow(w, SW_HIDE);

	SplitterUpdate();
}




FILE *logfile;

void GrepAllFiles(char *string);

struct{
	char *text;
	HWND hwnd;
	int washit;
} buttons[] = {
	{"Compile"},
#ifdef EMBEDDEBUG
	{NULL},
	{"Debug"},
#endif
	{"Options"},
	{"Def"},
	{"Grep"}
};

enum
{
	ID_COMPILE = 0,
#ifdef EMBEDDEBUG
	ID_NULL,
	ID_RUN,
#endif
	ID_OPTIONS,
	ID_DEF,
	ID_GREP
};

#define NUMBUTTONS sizeof(buttons)/sizeof(buttons[0])



void GUI_DialogPrint(const char *title, const char *text)
{
	MessageBox(mainwindow, text, title, 0);
}

static void FindNextScintilla(editor_t *editor, char *findtext, pbool next)
{
	int pos = SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0);
	Edit_SetSel(editor->editpane, pos+1, pos+1);
	SendMessage(editor->editpane, SCI_SEARCHANCHOR, 0, 0);
	if (SendMessage(editor->editpane, next?SCI_SEARCHNEXT:SCI_SEARCHPREV, 0, (LPARAM)findtext) != -1)
		Edit_ScrollCaret(editor->editpane);	//make sure its focused
	else
	{
		Edit_SetSel(editor->editpane, pos, pos);	//revert the selection change as nothing was found
		MessageBox(editor->editpane, "No more occurences found", "FTE Editor", 0);
	}
}
static char *WordUnderCursor(editor_t *editor, char *word, int wordsize, char *term, int termsize, int position);
pbool GenAutoCompleteList(char *prefix, char *buffer, int buffersize);

//available in xp+
typedef LRESULT (CALLBACK *SUBCLASSPROC)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
BOOL (WINAPI * pSetWindowSubclass)(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT (WINAPI *pDefSubclassProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MySubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	editor_t *editor;
	if (uMsg == WM_CHAR || uMsg == WM_UNICHAR)
	{
		switch(wParam)
		{
		case VK_ESCAPE:
			SplitterRemove(outputbox);
			break;
		case VK_SPACE:
			{
				BYTE keystate[256];
				GetKeyboardState(keystate);
				if ((keystate[VK_CONTROL] | keystate[VK_LCONTROL]) & 128)
				{
					for (editor = editors; editor; editor = editor->next)
					{
						if (editor->editpane == hWnd)
							break;
					}
					if (editor->scintilla)
					{
						if (!SendMessage(editor->editpane, SCI_AUTOCACTIVE, 0, 0))
						{
							static char buffer[65536];
							char prefixbuffer[128];
							char *pre = WordUnderCursor(editor, prefixbuffer, sizeof(prefixbuffer), NULL, 0, SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0));
							if (pre && *pre)
								if (GenAutoCompleteList(pre, buffer, sizeof(buffer)))
								{
									SendMessage(editor->editpane, SCI_AUTOCSETFILLUPS, 0, (LPARAM)".,[<>(*/+-=\t\n");
									SendMessage(editor->editpane, SCI_AUTOCSHOW, strlen(pre), (LPARAM)buffer);
								}
							return FALSE;
						}
					}
				}
			}
			break;
		}
	}
	if (uMsg == WM_LBUTTONDBLCLK && hWnd == outputbox)
	{
		CHARRANGE selrange = {0};
		SendMessage(hWnd, EM_EXGETSEL, 0, (LPARAM)&selrange);
		if (1)	//some text is selected.
		{
			unsigned int bytes;
			char line[1024];
			char *colon1, *colon2 = NULL;

			int l1;
			int l2;

			l1 = Edit_LineFromChar(hWnd, selrange.cpMin);
			l2 = Edit_LineFromChar(hWnd, selrange.cpMax);
			if (l1 == l2)
			{
				bytes = Edit_GetLine(hWnd, Edit_LineFromChar(outputbox, selrange.cpMin), line, sizeof(line)-1);
				line[bytes] = 0;

				for (colon1 = line+strlen(line)-1; *colon1 <= ' ' && colon1>=line; colon1--)
					*colon1 = '\0';
				if (!strncmp(line, "warning: ", 9))
					memmove(line, line+9, sizeof(line)-9);
				colon1=line;
				do
				{
					colon1 = strchr(colon1+1, ':');
				} while (colon1 && colon1[1] == '\\');

				if (colon1)
				{
					colon2 = strchr(colon1+1, ':');
					while (colon2 && colon2[1] == '\\')
					{
						colon2 = strchr(colon2+1, ':');
					}
					if (colon2)
					{
						*colon1 = '\0';
						*colon2 = '\0';
						EditFile(line, atoi(colon1+1)-1, 2);
					}
					else if (!strncmp(line, "Source file: ", 13))
						EditFile(line+13, -1, 2);
					else if (!strncmp(line, "Including: ", 11))
						EditFile(line+11, -1, 2);
				}
				else if (!strncmp(line, "including ", 10))
					EditFile(line+10, -1, 2);
				else if (!strncmp(line, "compiling ", 10))
					EditFile(line+10, -1, 2);
				else if (!strncmp(line, "prototyping ", 12))
					EditFile(line+12, -1, 2);
				else if (!strncmp(line, "Couldn't open file ", 19))
					EditFile(line+19, -1, 2);
				Edit_SetSel(hWnd, selrange.cpMin, selrange.cpMin);	//deselect it.
			}
		}
		return 0;
	}
	return pDefSubclassProc(hWnd, uMsg, wParam, lParam);
}

HWND CreateAnEditControl(HWND parent, pbool *scintillaokay)
{
	HWND newc = NULL;

#ifdef SCISTATIC
	extern int Scintilla_RegisterClasses(void *hinst);
	scintilla = ghInstance;
	Scintilla_RegisterClasses(scintilla);
#else
	if (!scintilla && scintillaokay)
	{
#ifdef _WIN64
		scintilla = LoadLibrary("SciLexer_64.dll");
		if (!scintilla)
#endif
		scintilla = LoadLibrary("SciLexer.dll");
	}
#endif

	if (!richedit)
		richedit = LoadLibrary("RICHED32.DLL");

	if (!newc && scintilla && scintillaokay)
	{
		newc=CreateWindowEx(WS_EX_CLIENTEDGE,
			"Scintilla",
			"",
			WS_CHILD /*| ES_READONLY*/ | WS_VISIBLE | 
			WS_HSCROLL | WS_VSCROLL | ES_LEFT | ES_WANTRETURN |
			ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 0, 0,
			parent,
			NULL,
			ghInstance,
			NULL);
	}
	if (newc)
		*scintillaokay = true;
	else if (scintillaokay)
	{
		*scintillaokay = false;
		scintillaokay = NULL;
	}

	if (!newc)
		newc=CreateWindowExW(WS_EX_CLIENTEDGE,
			richedit?RICHEDIT_CLASSW:L"EDIT",
			L"",
			WS_CHILD /*| ES_READONLY*/ | WS_VISIBLE | 
			WS_HSCROLL | WS_VSCROLL | ES_LEFT | ES_WANTRETURN |
			ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 0, 0,
			parent,
			NULL,
			ghInstance,
			NULL);

	if (!newc)
		newc=CreateWindowEx(WS_EX_CLIENTEDGE,
			richedit?RICHEDIT_CLASS10A:"EDIT",	//fall back to the earlier version
			"",
			WS_CHILD /*| ES_READONLY*/ | WS_VISIBLE | 
			WS_HSCROLL | WS_VSCROLL | ES_LEFT | ES_WANTRETURN |
			ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 0, 0,
			parent,
			NULL,
			ghInstance,
			NULL);

	if (!newc)
	{	//you've not got RICHEDIT installed properly, I guess
		FreeLibrary(richedit);
		richedit = NULL;
		newc=CreateWindowEx(WS_EX_CLIENTEDGE,
			"EDIT",
			"",
			WS_CHILD /*| ES_READONLY*/ | WS_VISIBLE | 
			WS_HSCROLL | WS_VSCROLL | ES_LEFT | ES_WANTRETURN |
			ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 0, 0,
			parent,
			NULL,
			ghInstance,
			NULL);
	}
	if (!newc)
		return NULL;

	if (scintillaokay)
	{
		FILE *f;
		int i;

		SendMessage(newc, SCI_STYLERESETDEFAULT, 0, 0);
		SendMessage(newc, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)"Consolas");
		SendMessage(newc, SCI_STYLECLEARALL, 0, 0);

		SendMessage(newc, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
		SendMessage(newc, SCI_SETLEXER,		SCLEX_CPP,						0);
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_DEFAULT,					RGB(0x00, 0x00, 0x00));
		SendMessage(newc, SCI_STYLECLEARALL,0,								0);
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_COMMENT,					RGB(0x00, 0x80, 0x00));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_COMMENTLINE,				RGB(0x00, 0x80, 0x00));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_COMMENTDOC,				RGB(0x00, 0x80, 0x00));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_NUMBER,					RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_WORD,						RGB(0x00, 0x00, 0xFF));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_STRING,					RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_CHARACTER,				RGB(0xA0, 0x10, 0x10));
//		SendMessage(newc, SCI_STYLESETFORE, SCE_C_UUID,						RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_PREPROCESSOR,				RGB(0x00, 0x00, 0xFF));
//		SendMessage(newc, SCI_STYLESETFORE, SCE_C_OPERATOR,					RGB(0x00, 0x00, 0x00));
//		SendMessage(newc, SCI_STYLESETFORE, SCE_C_IDENTIFIER,				RGB(0x00, 0x00, 0x00));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_STRINGEOL,				RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_VERBATIM,					RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_REGEX,					RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_COMMENTLINEDOC,			RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_WORD2,					RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_COMMENTDOCKEYWORD,		RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_COMMENTDOCKEYWORDERROR,	RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_GLOBALCLASS,				RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_STRINGRAW,				RGB(0xA0, 0x00, 0x00));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_TRIPLEVERBATIM,			RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_HASHQUOTEDSTRING,			RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_PREPROCESSORCOMMENT,		RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_PREPROCESSORCOMMENTDOC,	RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_USERLITERAL,				RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_TASKMARKER,				RGB(0xA0, 0x10, 0x10));
		SendMessage(newc, SCI_STYLESETFORE, SCE_C_ESCAPESEQUENCE,			RGB(0xA0, 0x10, 0x10));

		SendMessage(newc, SCI_STYLESETFORE, STYLE_BRACELIGHT,				RGB(0x00, 0x00, 0x3F));
		SendMessage(newc, SCI_STYLESETBACK, STYLE_BRACELIGHT,				RGB(0xef, 0xaf, 0xaf));
		SendMessage(newc, SCI_STYLESETBOLD, STYLE_BRACELIGHT,				TRUE);
		SendMessage(newc, SCI_STYLESETFORE, STYLE_BRACEBAD,					RGB(0x3F, 0x00, 0x00));
		SendMessage(newc, SCI_STYLESETBACK, STYLE_BRACEBAD,					RGB(0xff, 0xaf, 0xaf));

		//SCE_C_WORD
		SendMessage(newc, SCI_SETKEYWORDS,	0,	(LPARAM)
					"if else for do not while asm break case const continue "
					"default enum enumflags extern "
					"float goto __in __out __inout noref "
					"nosave shared state optional string "
					"struct switch thinktime until loop "
					"typedef union var "
					"accessor get set inline "
					"virtual nonvirtual class static nonstatic local return "
					"string float vector void int integer __variant entity"
					);

		//SCE_C_WORD2
		{
			char buffer[65536];
			GenBuiltinsList(buffer, sizeof(buffer));
			SendMessage(newc, SCI_SETKEYWORDS,	1,	(LPARAM)buffer);
		}
		//SCE_C_COMMENTDOCKEYWORDERROR
		//SCE_C_GLOBALCLASS
		SendMessage(newc, SCI_SETKEYWORDS,	3,	(LPARAM)
					""
					);
		//preprocessor listing
		{
			char *deflist = QCC_PR_GetDefinesList();
			SendMessage(newc, SCI_SETKEYWORDS,	4,	(LPARAM)deflist);
			free(deflist);
		}
		//task markers (in comments only)
		SendMessage(newc, SCI_SETKEYWORDS,	5,	(LPARAM)
					"TODO FIXME BUG"
					);

		SendMessage(newc, SCI_USEPOPUP, 0/*SC_POPUP_NEVER*/, 0);	//so we can do right-click menus ourselves.

		SendMessage(newc, SCI_SETMOUSEDWELLTIME, 1000, 0);
		SendMessage(newc, SCI_AUTOCSETORDER, SC_ORDER_PERFORMSORT, 0);
		SendMessage(newc, SCI_AUTOCSETFILLUPS, 0, (LPARAM)".,[<>(*/+-=\t\n");

		//Set up gui options.
		SendMessage(newc, SCI_SETMARGINWIDTHN,		0, (LPARAM)fl_extramargins?40:0);	//line numbers+folding
		SendMessage(newc, SCI_SETTABWIDTH,			fl_tabsize, 0);		//tab size

		//add margin for breakpoints
		SendMessage(newc, SCI_SETMARGINMASKN,		1, (LPARAM)~SC_MASK_FOLDERS);
		SendMessage(newc, SCI_SETMARGINWIDTHN,		1, (LPARAM)16);
		SendMessage(newc, SCI_SETMARGINSENSITIVEN,	1, (LPARAM)true);
		//give breakpoints a nice red circle.
		SendMessage(newc, SCI_MARKERDEFINE,			0,	SC_MARK_CIRCLE);
		SendMessage(newc, SCI_MARKERSETFORE,		0,	RGB(0x7F, 0x00, 0x00));
		SendMessage(newc, SCI_MARKERSETBACK,		0,	RGB(0xFF, 0x00, 0x00));
		//give current line a yellow arrow
		SendMessage(newc, SCI_MARKERDEFINE,			1,	SC_MARK_SHORTARROW);
		SendMessage(newc, SCI_MARKERSETFORE,		1,	RGB(0xFF, 0xFF, 0x00));
		SendMessage(newc, SCI_MARKERSETBACK,		1,	RGB(0x7F, 0x7F, 0x00));
		SendMessage(newc, SCI_MARKERDEFINE,			2,	SC_MARK_BACKGROUND);
		SendMessage(newc, SCI_MARKERSETFORE,		2,	RGB(0x00, 0x00, 0x00));
		SendMessage(newc, SCI_MARKERSETBACK,		2,	RGB(0xFF, 0xFF, 0x00));
		SendMessage(newc, SCI_MARKERSETALPHA,		2,	0x40);

		//add margin for folding
		SendMessage(newc, SCI_SETPROPERTY,  (WPARAM)"fold", (LPARAM)"1");
		SendMessage(newc, SCI_SETMARGINWIDTHN,		2, (LPARAM)fl_extramargins?16:0);
		SendMessage(newc, SCI_SETMARGINMASKN,		2, (LPARAM)SC_MASK_FOLDERS);
		SendMessage(newc, SCI_SETMARGINSENSITIVEN,	2, (LPARAM)true);
		//stop the images from being stupid
		SendMessage(newc, SCI_MARKERDEFINE,		SC_MARKNUM_FOLDEROPEN,		SC_MARK_BOXMINUS);
		SendMessage(newc, SCI_MARKERDEFINE,		SC_MARKNUM_FOLDER,			SC_MARK_BOXPLUS);
		SendMessage(newc, SCI_MARKERDEFINE,		SC_MARKNUM_FOLDERSUB,		SC_MARK_VLINE);
		SendMessage(newc, SCI_MARKERDEFINE,		SC_MARKNUM_FOLDERTAIL,		SC_MARK_LCORNERCURVE);
		SendMessage(newc, SCI_MARKERDEFINE,		SC_MARKNUM_FOLDEREND,		SC_MARK_BOXPLUSCONNECTED);
		SendMessage(newc, SCI_MARKERDEFINE,		SC_MARKNUM_FOLDEROPENMID,	SC_MARK_BOXMINUSCONNECTED);
		SendMessage(newc, SCI_MARKERDEFINE,		SC_MARKNUM_FOLDERMIDTAIL,	SC_MARK_TCORNERCURVE);
		//and fuck with colours so that its visible.
#define FOLDBACK RGB(0x50, 0x50, 0x50)
		SendMessage(newc, SCI_MARKERSETFORE,	SC_MARKNUM_FOLDER,			RGB(0xFF, 0xFF, 0xFF));
		SendMessage(newc, SCI_MARKERSETBACK,	SC_MARKNUM_FOLDER,			FOLDBACK);
		SendMessage(newc, SCI_MARKERSETFORE,	SC_MARKNUM_FOLDEROPEN,		RGB(0xFF, 0xFF, 0xFF));
		SendMessage(newc, SCI_MARKERSETBACK,	SC_MARKNUM_FOLDEROPEN,		FOLDBACK);
		SendMessage(newc, SCI_MARKERSETFORE,	SC_MARKNUM_FOLDEROPENMID,	RGB(0xFF, 0xFF, 0xFF));
		SendMessage(newc, SCI_MARKERSETBACK,	SC_MARKNUM_FOLDEROPENMID,	FOLDBACK);
		SendMessage(newc, SCI_MARKERSETBACK,	SC_MARKNUM_FOLDERSUB,		FOLDBACK);
		SendMessage(newc, SCI_MARKERSETFORE,	SC_MARKNUM_FOLDEREND,		RGB(0xFF, 0xFF, 0xFF));
		SendMessage(newc, SCI_MARKERSETBACK,	SC_MARKNUM_FOLDEREND,		FOLDBACK);
		SendMessage(newc, SCI_MARKERSETBACK,	SC_MARKNUM_FOLDERTAIL,		FOLDBACK);
		SendMessage(newc, SCI_MARKERSETBACK,	SC_MARKNUM_FOLDERMIDTAIL,	FOLDBACK);

		//disable preprocessor tracking, because QC preprocessor is not specific to an individual file, and even if it was, includes would be messy.
//		SendMessage(newc, SCI_SETPROPERTY,  (WPARAM)"lexer.cpp.track.preprocessor", (LPARAM)"0");

		for (i = 0; i < 0x100; i++)
		{
			char *lowtab[32] = {"QNUL",NULL,NULL,NULL,NULL,".",NULL,NULL,NULL,NULL,NULL,"#",NULL,">",".",".",
								"[","]","0","1","2","3","4","5","6","7","8","9",".","<-","-","->"};
			char *hightab[32] = {"(=","=","=)","=#=","White",".","Green","Red","Yellow","Blue",NULL,"Purple",NULL,">",".",".",
								"[","]","0","1","2","3","4","5","6","7","8","9",".","<-","-","->"};
			char foo[4];
			char bar[4];
			unsigned char c = i;
			foo[0] = i;	//these are invalid encodings or control chars.
			foo[1] = 0;

			if (c >= 0 && c < 32)
			{
				if (lowtab[c])
					SendMessage(newc, SCI_SETREPRESENTATION,	(WPARAM)foo,	(LPARAM)lowtab[c]);
			}
			else if (c >= (128|0) && c < (128|32))
			{
				if (hightab[c-128])
					SendMessage(newc, SCI_SETREPRESENTATION,	(WPARAM)foo,	(LPARAM)hightab[c-128]);
			}
			else if (c < 128)
				continue;	//don't do anything weird for ascii (other than control chars)
			else
			{
				int b = 0;
				bar[b++] = c&0x7f;
				bar[b++] = 0;
				SendMessage(newc, SCI_SETREPRESENTATION,	(WPARAM)foo,	(LPARAM)bar);
			}
		}

		for (i = 0xe000; i < 0xe100; i++)
		{
			char *lowtab[32] = {"QNUL",NULL,NULL,NULL,NULL,".",NULL,NULL,NULL,NULL,NULL,"#",NULL,">",".",".",
								"[","]","0","1","2","3","4","5","6","7","8","9",".","<-","-","->"};
			char *hightab[32] = {"(=","=","=)","=#=","White",".","Green","Red","Yellow","Blue",NULL,"Purple",NULL,">",".",".",
								"[","]","^0","^1","^2","^3","^4","^5","^6","^7","^8","^9",".","^<-","^-","^->"};
			char foo[4];
			char bar[4];
			unsigned char c = i;
			foo[0] = ((i>>12) & 0xf) | 0xe0;
			foo[1] = ((i>>6) & 0x3f) | 0x80;
			foo[2] = ((i>>0) & 0x3f) | 0x80;
			foo[3] = 0;

			if (c >= 0 && c < 32)
			{
				if (lowtab[c])
					SendMessage(newc, SCI_SETREPRESENTATION,	(WPARAM)foo,	(LPARAM)lowtab[c]);
			}
			else if (c >= (128|0) && c < (128|32))
			{
				if (hightab[c-128])
					SendMessage(newc, SCI_SETREPRESENTATION,	(WPARAM)foo,	(LPARAM)hightab[c-128]);
			}
			else
			{
				int b = 0;
				if (c >= 128)
					bar[b++] = '^';
				bar[b++] = c&0x7f;
				bar[b++] = 0;
				SendMessage(newc, SCI_SETREPRESENTATION,	(WPARAM)foo,	(LPARAM)bar);
			}
		}


		f = fopen("scintilla.cfg", "rt");
		if (f)
		{
			char buf[256];
			while(fgets(buf, sizeof(buf)-1, f))
			{
				int msg;
				int lparam;
				int wparam;
				char *c;
				buf[sizeof(buf)-1] = 0;
				c = buf;
				while(*c == ' ' || *c == '\t')
					c++;
				if (c[0] == '#')
					continue;
				if (c[0] == '/' && c[1] == '/')
					continue;
				if (c[0] == '\r' || c[0] == '\n' || !c[0])
					continue;
				msg = strtoul(c, &c, 0);
				while(*c == ' ' || *c == '\t')
					c++;
				if (*c == '\"')
				{
					c++;
					wparam = (LPARAM)c;
					c = strrchr(c, '\"');
					if (c)
						*c++ = 0;
				}
				else
					wparam = strtoul(c, &c, 0);
				while(*c == ' ' || *c == '\t')
					c++;
				if (*c == '\"')
				{
					c++;
					lparam = (LPARAM)c;
					c = strrchr(c, '\"');
					if (c)
						*c++ = 0;
				}
				else
					lparam = strtoul(c, &c, 0);

				SendMessage(newc, msg,	wparam,	lparam);
			}
			if (!ftell(f))
			{
				fclose(f);
				f = fopen("scintilla.cfg", "wt");
				if (f)
				{
					int i;
					int val;
					for (i = 0; i < STYLE_LASTPREDEFINED; i++)
					{
						val = SendMessage(newc, SCI_STYLEGETFORE,	i,	0);
						fprintf(f, "%i\t%i\t%#x\n", SCI_STYLESETFORE, i, val);
						val = SendMessage(newc, SCI_STYLEGETBACK,	i,	0);
						fprintf(f, "%i\t%i\t%#x\n", SCI_STYLESETBACK, i, val);
						val = SendMessage(newc, SCI_STYLEGETBOLD,	i,	0);
						fprintf(f, "%i\t%i\t%#x\n", SCI_STYLESETBOLD, i, val);
						val = SendMessage(newc, SCI_STYLEGETITALIC,	i,	0);
						fprintf(f, "%i\t%i\t%#x\n", SCI_STYLESETITALIC, i, val);
						val = SendMessage(newc, SCI_STYLEGETSIZE,	i,	0);
						fprintf(f, "%i\t%i\t%#x\n", SCI_STYLESETSIZE, i, val);
						val = SendMessage(newc, SCI_STYLEGETFONT,	i,	(LPARAM)buf);
						fprintf(f, "%i\t%i\t\"%s\"\n", SCI_STYLESETFONT, i, buf);
						val = SendMessage(newc, SCI_STYLEGETUNDERLINE,	i,	0);
						fprintf(f, "%i\t%i\t%#x\n", SCI_STYLESETUNDERLINE, i, val);
						val = SendMessage(newc, SCI_STYLEGETCASE,	i,	0);
						fprintf(f, "%i\t%i\t%#x\n", SCI_STYLESETCASE, i, val);
					}
					fclose(f);
				}
			}
			else
				fclose(f);
		}
	}
	else
	{
		//go to lucidia console, 10pt
		CHARFORMAT cf;
		memset(&cf, 0, sizeof(cf));
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_BOLD | CFM_FACE;// | CFM_SIZE;
		strcpy(cf.szFaceName, "Lucida Console");
		cf.yHeight = 5;

		SendMessage(newc, EM_SETCHARFORMAT, SCF_ALL, (WPARAM)&cf);
	
		if (richedit)
		{
			SendMessage(newc, EM_EXLIMITTEXT, 0, 1<<20);
		}
	}

	if (!pDefSubclassProc || !pSetWindowSubclass)
	{
		HMODULE lib = LoadLibrary("comctl32.dll");
		if (lib)
		{
			pDefSubclassProc = (void*)GetProcAddress(lib, "DefSubclassProc");
			pSetWindowSubclass = (void*)GetProcAddress(lib, "SetWindowSubclass");
		}
	}
	if (pDefSubclassProc && pSetWindowSubclass)
		pSetWindowSubclass(newc, MySubclassWndProc, 0, (DWORD_PTR)parent);

	ShowWindow(newc, SW_SHOW);

	return newc;
}




enum {
	IDM_OPENDOCU=32,
	IDM_OPENNEW,
	IDM_OPENPROJECT,
	IDM_GREP,
	IDM_GOTODEF,
	IDM_RETURNDEF,
	IDM_OUTPUT_WINDOW,
	IDM_UI_SHOWLINENUMBERS,
	IDM_UI_TABSIZE,
	IDM_SAVE,
	IDM_RECOMPILE,
	IDM_FIND,
	IDM_FINDNEXT,
	IDM_FINDPREV,
	IDM_QUIT,
	IDM_UNDO,
	IDM_REDO,
	IDM_CUT,
	IDM_COPY,
	IDM_PASTE,
	IDM_ABOUT,
	IDM_CASCADE,
	IDM_TILE_HORIZ,
	IDM_TILE_VERT,
	IDM_DEBUG_REBUILD,
	IDM_DEBUG_BUILD_OPTIONS,
	IDM_DEBUG_SETNEXT,
	IDM_DEBUG_RUN,
	IDM_DEBUG_STEPOVER,
	IDM_DEBUG_STEPINTO,
	IDM_DEBUG_STEPOUT,
	IDM_DEBUG_TOGGLEBREAK,
	IDM_ENCODING_PRIVATEUSE,
	IDM_ENCODING_DEPRIVATEUSE,
	IDM_ENCODING_UNIX,
	IDM_ENCODING_WINDOWS,
	IDM_CREATEINSTALLER_WINDOWS,
	IDM_CREATEINSTALLER_ANDROID,
	IDM_CREATEINSTALLER_PACKAGES,

	IDI_O_LEVEL0,
	IDI_O_LEVEL1,
	IDI_O_LEVEL2,
	IDI_O_LEVEL3,
	IDI_O_DEFAULT,
	IDI_O_DEBUG,
//	IDI_O_CHANGE_PROGS_SRC,
	IDI_O_ADDITIONALPARAMETERS,
	IDI_O_OPTIMISATION,
	IDI_O_COMPILER_FLAG,
	IDI_O_APPLYSAVE,
	IDI_O_APPLY,
	IDI_O_TARGETH2,
	IDI_O_TARGETFTE,
	IDI_O_ENGINE,
	IDI_O_ENGINEBASEDIR,
	IDI_O_ENGINECOMMANDLINE,

	IDM_FIRSTCHILD
};

static void EditorReload(editor_t *editor);
int EditorSave(editor_t *edit);
void EditFile(const char *name, int line, pbool setcontrol);
pbool EditorModified(editor_t *e);

void QueryOpenFile(void)
{
	char filename[MAX_PATH];
	char oldpath[MAX_PATH+10];
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename)-1;
	memset(filename, 0, sizeof(filename));
	GetCurrentDirectory(sizeof(oldpath)-1, oldpath);
	if (GetOpenFileName(&ofn))
		EditFile(filename, -1, false);
	SetCurrentDirectory(oldpath);
}

static void Packager_MessageCallback(void *ctx, const char *fmt, ...);

//IDM_ stuff that needs no active window
void GenericMenu(WPARAM wParam)
{
	switch(LOWORD(wParam))
	{
	case IDM_OPENPROJECT:
		{
			char filename[MAX_PATH];
			char oldpath[MAX_PATH+10];
			OPENFILENAME ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hInstance = ghInstance;
			ofn.lpstrFile = filename;
			ofn.lpstrTitle = "Please find progs.src or progs.dat";
			ofn.nMaxFile = sizeof(filename)-1;
			ofn.lpstrFilter = "QuakeC Projects\0*.src;*.dat\0All files\0*.*\0";
			memset(filename, 0, sizeof(filename));
			GetCurrentDirectory(sizeof(oldpath)-1, oldpath);
			ofn.lpstrInitialDir = oldpath;
			if (GetOpenFileName(&ofn))
			{
				SetProgsSrcFileAndPath(filename);
			}
			resetprogssrc = true;
		}
		break;

	case IDM_OPENNEW:
		QueryOpenFile();
		break;

	case IDM_QUIT:
		PostQuitMessage(0);
		break;

	case IDM_RECOMPILE:
		buttons[ID_COMPILE].washit = true;
		break;

	case IDM_CREATEINSTALLER_WINDOWS:
		GUI_CreateInstaller_Windows();
		break;
	case IDM_CREATEINSTALLER_ANDROID:
		GUI_CreateInstaller_Android();
		break;
	case IDM_CREATEINSTALLER_PACKAGES:
		{
			struct pkgctx_s *ctx;

			CreateOutputWindow(false);
			GUIprintf("");

			ctx = Packager_Create(Packager_MessageCallback, NULL);
			Packager_ParseFile(ctx, "packages.src");
			Packager_WriteDataset(ctx, NULL);
			Packager_Destroy(ctx);
		}
		break;

	case IDM_ABOUT:
#if defined(SVNREVISION) && defined(SVNDATE)
		MessageBox(NULL, "FTE QuakeC Compiler "STRINGIFY(SVNREVISION)" ("STRINGIFY(SVNDATE)")\nWritten by Forethought Entertainment, whoever that is.\n\nIf you have problems with wordpad corrupting your qc files, try saving them using utf-16 encoding via notepad.\nDecompiler component derived from frikdec.", "About", 0);
#elif defined(SVNREVISION)
		MessageBox(NULL, "FTE QuakeC Compiler "STRINGIFY(SVNREVISION)" ("__DATE__" "__TIME__")\nWritten by Forethought Entertainment, whoever that is.\n\nIf you have problems with wordpad corrupting your qc files, try saving them using utf-16 encoding via notepad.\nDecompiler component derived from frikdec.", "About", 0);
#else
		MessageBox(NULL, "FTE QuakeC Compiler ("__DATE__")\nWritten by Forethought Entertainment, whoever that is.\n\nIf you have problems with wordpad corrupting your qc files, try saving them using utf-16 encoding via notepad.\nDecompiler component derived from frikdec.", "About", 0);
#endif
		break;

	case IDM_CASCADE:
		SendMessage(mdibox, WM_MDICASCADE, 0, 0);
		break;
	case IDM_TILE_HORIZ:
		SendMessage(mdibox, WM_MDITILE, MDITILE_HORIZONTAL, 0);
		break;
	case IDM_TILE_VERT:
		SendMessage(mdibox, WM_MDITILE, MDITILE_VERTICAL, 0);
		break;


	case IDM_OUTPUT_WINDOW:
		if (GetFocus() == outputbox)
			SplitterRemove(outputbox);
		else
			SplitterFocus(outputbox, 64, 128);
		break;
	case IDM_UI_SHOWLINENUMBERS:
		{
			editor_t *ed;
			MENUITEMINFO mii = {sizeof(mii)};
			fl_extramargins = !fl_extramargins;
			mii.fMask = MIIM_STATE;
			mii.fState = fl_extramargins?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(GetMenu(mainwindow), IDM_UI_SHOWLINENUMBERS, FALSE, &mii);

			for (ed = editors; ed; ed = ed->next)
			{
				if (ed->scintilla)
				{
					SendMessage(ed->editpane, SCI_SETMARGINWIDTHN,		0, (LPARAM)fl_extramargins?40:0);
					SendMessage(ed->editpane, SCI_SETMARGINWIDTHN,		2, (LPARAM)fl_extramargins?16:0);
				}
			}
		}
		break;
	case IDM_UI_TABSIZE:
		{
			editor_t *ed;
			MENUITEMINFO mii = {sizeof(mii)};
			fl_tabsize = (fl_tabsize>4)?4:8;
			mii.fMask = MIIM_STATE;
			mii.fState = (fl_tabsize>4)?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(GetMenu(mainwindow), IDM_UI_TABSIZE, FALSE, &mii);

			for (ed = editors; ed; ed = ed->next)
			{
				if (ed->scintilla)
				{
					SendMessage(ed->editpane, SCI_SETTABWIDTH,		fl_tabsize, 0);
				}
			}
		}
		break;
	case IDM_DEBUG_RUN:
		EditFile(NULL, -1, true);
		EngineGiveFocus();
		if (!EngineCommandf("qcresume\n"))
			RunEngine();
		return;
	case IDM_DEBUG_REBUILD:
		buttons[ID_COMPILE].washit = true;
		return;
	case IDM_DEBUG_BUILD_OPTIONS:
		OptionsDialog();
		return;
	case IDM_DEBUG_STEPOVER:
		EditFile(NULL, -1, true);
		EngineCommandf("qcstep over\n");
		return;
	case IDM_DEBUG_STEPINTO:
		EditFile(NULL, -1, true);
		EngineCommandf("qcstep into\n");
		return;
	case IDM_DEBUG_STEPOUT:
		EditFile(NULL, -1, true);
		EngineCommandf("qcstep out\n");
		return;
	}
}

static char *WordUnderCursor(editor_t *editor, char *word, int wordsize, char *term, int termsize, int position)
{
	unsigned char linebuf[1024];
	DWORD charidx;
	DWORD lineidx;
	POINT pos;
	RECT rect;
	if (editor->scintilla)
	{
		DWORD len;

		lineidx = SendMessage(editor->editpane, SCI_LINEFROMPOSITION, position, 0);
		charidx = position - SendMessage(editor->editpane, SCI_POSITIONFROMLINE, lineidx, 0);

		len = SendMessage(editor->editpane, SCI_LINELENGTH, lineidx, 0);
		if (len >= sizeof(linebuf))
			return "";
		len = SendMessage(editor->editpane, SCI_GETLINE, lineidx, (LPARAM)linebuf);
		linebuf[len] = 0;
		if (charidx >= len)
			charidx = len-1;
	}
	else
	{
		GetCursorPos(&pos);
		GetWindowRect(editor->editpane, &rect);
		pos.x -= rect.left;
		pos.y -= rect.top;
		charidx = SendMessage(editor->editpane, EM_CHARFROMPOS, 0, (LPARAM)&pos);
		lineidx = SendMessage(editor->editpane, EM_LINEFROMCHAR, charidx, 0);
		charidx -= SendMessage(editor->editpane, EM_LINEINDEX, lineidx, 0);
		Edit_GetLine(editor->editpane, lineidx, linebuf, sizeof(linebuf));
	}

	if (word)
	{
		//skip back to the start of the word
		while(charidx > 0 && (
			(linebuf[charidx-1] >= 'a' && linebuf[charidx-1] <= 'z') ||
			(linebuf[charidx-1] >= 'A' && linebuf[charidx-1] <= 'Z') ||
			(linebuf[charidx-1] >= '0' && linebuf[charidx-1] <= '9') ||
			linebuf[charidx-1] == '_' || linebuf[charidx-1] == ':' ||
			linebuf[charidx-1] >= 128
			))
		{
			charidx--;
		}
		//copy the result out
		lineidx = 0;
		wordsize--;
		while (wordsize && (
			(linebuf[charidx] >= 'a' && linebuf[charidx] <= 'z') ||
			(linebuf[charidx] >= 'A' && linebuf[charidx] <= 'Z') ||
			(linebuf[charidx] >= '0' && linebuf[charidx] <= '9') ||
			linebuf[charidx] == '_' || linebuf[charidx] == ':' ||
			linebuf[charidx] >= 128
			))
		{
			word[lineidx++] = linebuf[charidx++];
			wordsize--;
		}
		word[lineidx++] = 0;
	}

	if (term)
	{
		//skip back to the start of the word
		while(charidx > 0 && (
			(linebuf[charidx-1] >= 'a' && linebuf[charidx-1] <= 'z') ||
			(linebuf[charidx-1] >= 'A' && linebuf[charidx-1] <= 'Z') ||
			(linebuf[charidx-1] >= '0' && linebuf[charidx-1] <= '9') ||
			linebuf[charidx-1] == '_' || linebuf[charidx-1] == ':' || linebuf[charidx-1] == '.' ||
			linebuf[charidx-1] == '[' || linebuf[charidx-1] == ']' ||
			linebuf[charidx-1] >= 128
			))
		{
			charidx--;
		}
		//copy the result out
		lineidx = 0;
		termsize--;
		while (termsize && (
			(linebuf[charidx] >= 'a' && linebuf[charidx] <= 'z') ||
			(linebuf[charidx] >= 'A' && linebuf[charidx] <= 'Z') ||
			(linebuf[charidx] >= '0' && linebuf[charidx] <= '9') ||
			linebuf[charidx] == '_' || linebuf[charidx] == ':' || linebuf[charidx] == '.' ||
			linebuf[charidx] == '[' || linebuf[charidx] == ']' ||
			linebuf[charidx] >= 128
			))
		{
			term[lineidx++] = linebuf[charidx++];
			termsize--;
		}
		term[lineidx++] = 0;
	}
	return word;
}
static char *ReadTextSelection(editor_t *editor, char *word, int wordsize)
{
	int total;
	if (editor->scintilla)
	{
		total = SendMessage(editor->editpane, SCI_GETSELTEXT, 0, (LPARAM)NULL);
		if (total < wordsize)
			total = SendMessage(editor->editpane, SCI_GETSELTEXT, 0, (LPARAM)word);
		else
			total = 0;
	}
	else
	{
		CHARRANGE ffs;
		SendMessage(editor->editpane, EM_EXGETSEL, 0, (LPARAM)&ffs);
		if (ffs.cpMax-ffs.cpMin > wordsize-1)
			total = 0;	//don't crash through the use of a crappy API.
		else
			total = SendMessage(editor->editpane, EM_GETSELTEXT, (WPARAM)0, (LPARAM)word);
	}
	if (total)
		word[total]='\0';
	else
	{
		if (*WordUnderCursor(editor, word, wordsize, NULL, 0, SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0)))
			return word;
		return NULL;
	}
	return word;
}

static void GUI_Recode(editor_t *editor, int target)
{
	if (target == UTF8_BOM && editor->savefmt == UTF_ANSI)
	{	//we're currently using some ansi-like format. convert it to quake's format.
		pbool errors = false;
		int len;
		char *wfile, *in;

		if (IDCANCEL==MessageBox(editor->window, "Really convert?", editor->filename, MB_OKCANCEL))
			return;

		if (editor->scintilla)
		{
			char *afile, *out;
			len = SendMessage(editor->editpane, SCI_GETLENGTH, 0, 0);
			wfile = malloc(len+1);
			SendMessage(editor->editpane, SCI_GETTEXT, len, (LPARAM)wfile);
			wfile[len] = 0;

			afile = malloc((len+1)*3);

			in = wfile;
			out = afile;
			while(*in)
			{
				unsigned int c = (unsigned char)*in++;
				//fixme: do we care about ascii control codes? quake tends not to, but also abuses them...
				if ((c >= 32 && c < 0x80) || c == '\n' || c == '\r' || c == '\t')
					*out++ = c;	//ascii chars are still ascii
				else if (c >= 0 && c < 0xff) //controll chars and high-value chars are not considered ascii and thus not safe
				{
					c |= 0xe000;	//maps to private use

//					*out++ = ((c>>6) & 0x1f) | 0xc0;
//					*out++ = ((c>>0) & 0x3f) | 0x80;

					*out++ = ((c>>12) & 0xf) | 0xe0;
					*out++ = ((c>>6) & 0x3f) | 0x80;
					*out++ = ((c>>0) & 0x3f) | 0x80;
				}
				else
				{
					*out++ = c;
					errors = true;
				}
			}
			*out++ = 0;

			if (errors)
				errors = IDCANCEL==MessageBox(editor->window, "Encoding quake's char set to utf-8 will corrupt some characters (and cannot be displayed correctly in this editor). continue anyway?", editor->filename, MB_OKCANCEL);

			if (!errors)
			{
				editor->savefmt = UTF8_BOM;	//always use a bom, because notepad is shite.
				SendMessage(editor->editpane, SCI_SETTEXT, 0, (LPARAM)afile);
				SendMessage(editor->editpane, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
			}
			free(afile);
		}
		else
		{
			wchar_t *afile, *out;

			len = GetWindowTextLengthA(editor->editpane);
			wfile = malloc(len+1);
			GetWindowTextA(editor->editpane, wfile, len+1);
		
			afile = malloc((len+1)*2);

			in = wfile;
			out = afile;
			while(*in)
			{
				unsigned char c = *in++;
				//fixme: do we care about ascii control codes? quake tends not to, but also abuses them...
				if ((c >= 32 && c < 0x80) || c == '\n' || c == '\r' || c == '\t')
					*out++ = c;	//ascii chars are still ascii
				else if (c >= 0 && c < 0xff) //controll chars and high-value chars are not considered ascii and thus not safe
					*out++ = c | 0xe000;	//maps to private use
				else
				{
					*out++ = c;
					errors = true;
				}
			}
			*out++ = 0;

			if (errors)
				errors = IDCANCEL==MessageBox(editor->window, "Encoding quake's char set to utf-8 will corrupt some characters (and cannot be displayed correctly in this editor). continue anyway?", editor->filename, MB_OKCANCEL);

			if (!errors)
			{
				editor->savefmt = UTF8_BOM;	//always use a bom, because notepad is shite.
				SetWindowTextW(editor->editpane, afile);
			}
			free(afile);
		}
		
		free(wfile);
	}
	else if (target == UTF_ANSI && editor->savefmt != UTF_ANSI)
	{	//we're currently using some unicode format. convert it to quake's format.
		pbool errors = false;
		int len;
		wchar_t *wfile, *in;
		char *afile, *out;

		if (IDCANCEL==MessageBox(editor->window, "Really convert?", editor->filename, MB_OKCANCEL))
			return;

		len = GetWindowTextLengthW(editor->editpane);
		wfile = malloc((len+1)*2);
		afile = malloc(len+1);
		GetWindowTextW(editor->editpane, wfile, len+1);

		in = wfile;
		out = afile;
		while(*in)
		{
			//fixme: do we care about ascii control codes? quake tends not to, but also abuses them...
			if (*in >= 0 && *in < 0x80)
				*out++ = *in++;	//ascii is ascii
			else if (*in >= 0xe000 && *in < 0xe100)
				*out++ = *in++;	//quake's charset is quake's charset
			//FIXME: no utf-16 surrogates
			else
			{
				*out++ = *in++;
				errors = true;
			}
		}
		*out++ = 0;

		if (errors)
			errors = IDCANCEL==MessageBox(editor->window, "Encoding to quake's char set will corrupt some characters (and cannot be displayed correctly in this editor). continue anyway?", editor->filename, MB_OKCANCEL);

		if (!errors)
		{
			editor->savefmt = UTF_ANSI;
			if (editor->scintilla)
			{
				SendMessage(editor->editpane, SCI_SETCODEPAGE, 28591, 0);
				SendMessage(editor->editpane, SCI_SETTEXT, 0, (LPARAM)afile);
			}
			else
				SetWindowTextA(editor->editpane, afile);
		}
		
		free(wfile);
		free(afile);
	}
}

void EditorMenu(editor_t *editor, WPARAM wParam)
{
	switch(LOWORD(wParam))
	{
	case IDM_OPENDOCU:
		{
			char buffer[1024];
			if (!ReadTextSelection(editor, buffer, sizeof(buffer)))
			{
				MessageBox(NULL, "There is no name currently selected.", "Whoops", 0);
				break;
			}
			else
				EditFile(buffer, -1, false);
		}
		break;
	case IDM_SAVE:
		EditorSave(editor);
		break;
	case IDM_FIND:
		SetFocus(search_name);
		break;
	case IDM_FINDNEXT:
	case IDM_FINDPREV:
		{
			char buffer[128];
			GetWindowText(search_name, buffer, sizeof(buffer));
			if (*buffer != 0)
			{
				HWND ew = (HWND)SendMessage(mdibox, WM_MDIGETACTIVE, 0, 0);
				editor_t *editor;
				for (editor = editors; editor; editor = editor->next)
				{
					if (editor->window == ew)
						break;
				}
				if (editor && editor->scintilla)
				{
					FindNextScintilla(editor, buffer, LOWORD(wParam) == IDM_FINDNEXT);
					SetFocus(editor->window);
					SetFocus(editor->editpane);
				}
			}
		}
		break;
	case IDM_GREP:
		{
			char buffer[1024];
			if (!ReadTextSelection(editor, buffer, sizeof(buffer)))
			{
				MessageBox(NULL, "There is no search text specified.", "Whoops", 0);
				break;
			}
			else
				GrepAllFiles(buffer);
		}
		break;
	case IDM_RETURNDEF:
		if (navhistory_pos > navhistory_first)
		{
			editor_t *ed;
			navhistory_pos--;
			//search for the editor to make sure its still open
			for (ed = editors; ed; ed = ed->next)
			{
				if (ed == navhistory[navhistory_pos&navhistory_size].editor)
				{
					SetFocus(ed->window);
					SetFocus(ed->editpane);
					SendMessage(ed->editpane, SCI_SETSEL, navhistory[navhistory_pos&navhistory_size].selpos, navhistory[navhistory_pos&navhistory_size].anchorpos);
					break;
				}
			}
		}
		break;
	case IDM_GOTODEF:
		{
			char buffer[1024];

			{
				navhistory[navhistory_pos&navhistory_size].editor = editor;
				navhistory[navhistory_pos&navhistory_size].selpos = SendMessage(editor->editpane, SCI_GETANCHOR, 0, 0);
				navhistory[navhistory_pos&navhistory_size].anchorpos = SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0);
				navhistory_pos++;
				if (navhistory_pos > navhistory_first + navhistory_size)
					navhistory_first = navhistory_pos - navhistory_size;
			}

			if (!ReadTextSelection(editor, buffer, sizeof(buffer)))
			{
				MessageBox(NULL, "There is no name currently selected.", "Whoops", 0);
				break;
			}
			else
				GoToDefinition(buffer);
		}
		break;

	case IDM_UNDO:
		if (editor->scintilla)
			SendMessage(editor->editpane, SCI_UNDO, 0, 0);
		else
			Edit_Undo(editor->editpane);
		break;
	case IDM_REDO:
		if (editor->scintilla)
			SendMessage(editor->editpane, SCI_REDO, 0, 0);
		else
			Edit_Redo(editor->editpane);
		break;

	case IDM_CUT:
		if (editor->scintilla)
			SendMessage(editor->editpane, SCI_CUT, 0, 0);
		break;
	case IDM_COPY:
		if (editor->scintilla)
			SendMessage(editor->editpane, SCI_COPY, 0, 0);
		break;
	case IDM_PASTE:
		if (editor->scintilla)
			SendMessage(editor->editpane, SCI_PASTE, 0, 0);
		break;

	case IDM_DEBUG_TOGGLEBREAK:
		{
			int mode;
			if (editor->scintilla)
			{
				mode = !(SendMessage(editor->editpane, SCI_MARKERGET, editor->curline, 0) & 1);
				SendMessage(editor->editpane, mode?SCI_MARKERADD:SCI_MARKERDELETE, editor->curline, 0);
			}
			else
				mode = 2;

			EngineCommandf("qcbreakpoint %i \"%s\" %i\n", mode, editor->filename, editor->curline+1);
		}
		return;
	case IDM_DEBUG_SETNEXT:
		EngineCommandf("qcjump \"%s\" %i\n", editor->filename, editor->curline+1);
		return;

	case IDM_ENCODING_PRIVATEUSE:
		GUI_Recode(editor, UTF8_BOM);
		break;
	case IDM_ENCODING_DEPRIVATEUSE:
		GUI_Recode(editor, UTF_ANSI);
		break;

	case IDM_ENCODING_UNIX:
		SendMessage(editor->editpane, SCI_CONVERTEOLS, SC_EOL_LF, 0);
		SendMessage(editor->editpane, SCI_SETVIEWEOL, false, 0);
		break;
	case IDM_ENCODING_WINDOWS:
		SendMessage(editor->editpane, SCI_CONVERTEOLS, SC_EOL_CRLF, 0);
		SendMessage(editor->editpane, SCI_SETVIEWEOL, false, 0);
		break;

	default:
		GenericMenu(wParam);
		break;
	}
}

editor_t *tooltip_editor = NULL;
char tooltip_variable[256];
char tooltip_type[256];
char tooltip_comment[2048];
size_t tooltip_position;

char *GetTooltipText(editor_t *editor, int pos, pbool dwell)
{
	static char buffer[1024];
	char wordbuf[256], *text;
	char term[256];
	char *defname;
	defname = WordUnderCursor(editor, wordbuf, sizeof(wordbuf), term, sizeof(term), pos);
	if (!*defname)
		return NULL;
	else if (globalstable.numbuckets)
	{
		QCC_def_t *def;
		int fno;
		int line;
		int best, bestline;
		char *macro = QCC_PR_CheckCompConstTooltip(defname, buffer, buffer + sizeof(buffer));
		if (macro && *macro)
			return macro;

		if (dwell)
		{
			tooltip_editor = NULL;
			*tooltip_variable = 0;
			tooltip_position = 0;
			*tooltip_type = 0;
			*tooltip_comment = 0;
		}

		line = SendMessage(editor->editpane, SCI_LINEFROMPOSITION, pos, 0);

		for (best = 0,bestline=0, fno = 1; fno < numfunctions; fno++)
		{
			if (line > functions[fno].line && bestline < functions[fno].line)
			{
				if (!strcmp(editor->filename, functions[fno].filen))
				{
					best = fno;
					bestline = functions[fno].line;
				}
			}
		}
		if (best)
		{
			if (strstr(functions[best].name, "::"))
			{
				QCC_type_t *type;
				char tmp[256];
				char *c;
				QC_strlcpy(tmp, functions[best].name, sizeof(tmp));
				c = strstr(tmp, "::");
				if (c)
					*c = 0;
				type = QCC_TypeForName(tmp);

				if (type->type == ev_entity)
				{
					QCC_def_t *def;
					QC_snprintfz(tmp, sizeof(tmp), "%s::__m%s", type->name, term);

					for (fno = 0, def = NULL; fno < sourcefilesnumdefs && !def; fno++)
					{
						for (def = sourcefilesdefs[fno]; def; def = def->next)
						{
							if (def->scope && def->scope != &functions[best])
								continue;
//							OutputDebugString(def->name);
//							OutputDebugString("\n");
							if (!strcmp(def->name, tmp))
							{
								//FIXME: look at the scope's function to find the start+end of the function and filter based upon that, to show locals
								break;
							}
						}
					}

					if (def && def->type->type == ev_field)
					{
//						QC_strlcpy(tmp, term, sizeof(tmp));
						QC_snprintfz(term, sizeof(term), "self.%s", tmp);
					}
					else
					{
						for (fno = 0, def = NULL; fno < sourcefilesnumdefs && !def; fno++)
						{
							for (def = sourcefilesdefs[fno]; def; def = def->next)
							{
								if (def->scope && def->scope != &functions[best])
									continue;
								if (!strcmp(def->name, term))
								{
									//FIXME: look at the scope's function to find the start+end of the function and filter based upon that, to show locals
									break;
								}
							}
						}
						if (def && def->type->type == ev_field)
						{
							QC_strlcpy(tmp, term, sizeof(tmp));
							QC_snprintfz(term, sizeof(term), "self.%s", tmp);
						}
					}
				}
			}
		}

		//FIXME: we may need to display types too
		for (fno = 0, def = NULL; fno < sourcefilesnumdefs && !def; fno++)
		{
			for (def = sourcefilesdefs[fno]; def; def = def->next)
			{
				if (def->scope)
					continue;
				if (!strcmp(def->name, defname))
				{
					//FIXME: look at the scope's function to find the start+end of the function and filter based upon that, to show locals
					break;
				}
			}
		}

		if (def)
		{
			char typebuf[1024];
			char valuebuf[1024];
			char *value = "";
			if (def->constant && def->type->type == ev_float)
				QC_snprintfz(value=valuebuf, sizeof(valuebuf), " = %g", def->symboldata[def->ofs]._float);
			else if (def->constant && def->type->type == ev_integer)
				QC_snprintfz(value=valuebuf, sizeof(valuebuf), " = %i", def->symboldata[def->ofs]._int);
			else if (def->constant && def->type->type == ev_vector)
				QC_snprintfz(value=valuebuf, sizeof(valuebuf), " = '%g %g %g'", def->symboldata[def->ofs].vector[0], def->symboldata[def->ofs].vector[1], def->symboldata[def->ofs].vector[2]);
			//note function argument names do not persist beyond the function def. we might be able to read the function's localdefs for them, but that's unreliable/broken with builtins where they're most needed.
			if (def->comment)
				QC_snprintfz(buffer, sizeof(buffer)-1, "%s %s%s\r\n%s", TypeName(def->type, typebuf, sizeof(typebuf)), def->name, value, def->comment);
			else
				QC_snprintfz(buffer, sizeof(buffer)-1, "%s %s%s", TypeName(def->type, typebuf, sizeof(typebuf)), def->name, value);

			if (dwell)
			{
				strncpy(tooltip_type, TypeName(def->type, typebuf, sizeof(typebuf)), sizeof(tooltip_type)-1);
				if (def->comment)
					strncpy(tooltip_comment, def->comment, sizeof(tooltip_comment)-1);
			}

			text = buffer;
		}
		else
			text = NULL;

		if (dwell)
		{
			strncpy(tooltip_variable, term, sizeof(tooltip_variable));
			tooltip_variable[sizeof(tooltip_variable)-1] = 0;
			tooltip_position = pos;
			tooltip_editor = editor;

			EngineCommandf("qcinspect \"%s\" \"%s\"\n", term, (def && def->scope)?def->scope->name:"");

			if (text)
				SendMessage(editor->editpane, SCI_CALLTIPSHOW, (WPARAM)pos, (LPARAM)text);
		}

		return text;
	}
	else
		return NULL;//"Type info not available. Compile first.";
}

//scans the preceeding line(s) to find the ideal indentation for the highlighted line
//indentbuf may contain spaces or tabs. preferably tabs.
static void scin_get_line_indent(HWND editpane, int lineidx, char *indentbuf, size_t sizeofbuf)
{
	size_t i, len;
	while (lineidx --> 0)
	{
		len = SendMessage(editpane, SCI_LINELENGTH, lineidx, 0);
		*indentbuf = 0;
		if (len+2 < sizeofbuf)
		{
			//FIXME: ignore whitespace
			len = SendMessage(editpane, SCI_GETLINE, lineidx, (LPARAM)indentbuf);
			for (i = 0; i < len; i++)
			{
				if (indentbuf[i] == ' ' || indentbuf[i] == '\t')
					continue;
				break;
			}
			if (i == len)
				continue;

			if (len >= 3 && indentbuf[len-3] == '{')
				indentbuf[i++] = '\t';	//add an indent
			indentbuf[i] = 0;
			return;
		}
	}
	*indentbuf = 0;	//failed
}

void Scin_HandleCharAdded(editor_t *editor, struct SCNotification *not, int pos)
{
	if (not->ch == '(')
	{
		char *s = GetTooltipText(editor, pos-1, FALSE);
		tooltip_editor = NULL;
		if (s)
			SendMessage(editor->editpane, SCI_CALLTIPSHOW, (WPARAM)pos, (LPARAM)s);
	}
	else if (not->ch == '}')
	{	//if the first char on the line, fix up indents to match previous-1
		char prevline[65536];
		char newline[4096];
		int pos = SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0);
		int lineidx = SendMessage(editor->editpane, SCI_LINEFROMPOSITION, pos, 0);
		int linestart = SendMessage(editor->editpane, SCI_POSITIONFROMLINE, lineidx, 0);
		int plen;
		int nlen = SendMessage(editor->editpane, SCI_LINELENGTH, lineidx, 0);
		if (nlen >= sizeof(newline))
			return;
		nlen = SendMessage(editor->editpane, SCI_GETLINE, lineidx, (LPARAM)newline);
		if (linestart > 2)
		{
			scin_get_line_indent(editor->editpane, lineidx, prevline, sizeof(prevline));
			plen = strlen(prevline);
			if (plen > nlen)
				return;	//already indented a bit or something
			if (!strncmp(prevline, newline, plen))	//same indent
			{
				SendMessage(editor->editpane, SCI_CHARLEFT, 0, 0);	//move to the indent
				SendMessage(editor->editpane, SCI_BACKTAB, 0, 0);	//do shift-tab to un-indent the current selection (one line supposedly)
				SendMessage(editor->editpane, SCI_CHARRIGHT, 0, 0);	//and move back to the right of the }
			}
		}
	}
	else if (not->ch == '\r' || not->ch == '\n')
	{
		char linebuf[65536];
		int pos = SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0);
		int lineidx = SendMessage(editor->editpane, SCI_LINEFROMPOSITION, pos, 0);
		int linestart = SendMessage(editor->editpane, SCI_POSITIONFROMLINE, lineidx, 0);
		//int len = SendMessage(editor->editpane, SCI_LINELENGTH, lineidx, 0);
		if (pos == linestart)
		{
			scin_get_line_indent(editor->editpane, lineidx, linebuf, sizeof(linebuf));
			SendMessage(editor->editpane, SCI_REPLACESEL, 0, (LPARAM)linebuf);
		}
	}
/*
	else if (0)//(!SendMessage(editor->editpane, SCI_AUTOCACTIVE, 0, 0))
	{
		char buffer[65536];
		char prefixbuffer[128];
		char *pre = WordUnderCursor(editor, prefixbuffer, sizeof(prefixbuffer), NULL, 0, SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0));
		if (pre && *pre)
			if (GenAutoCompleteList(pre, buffer, sizeof(buffer)))
			{
				SendMessage(editor->editpane, SCI_AUTOCSETFILLUPS, 0, (LPARAM)"\t\n");
				SendMessage(editor->editpane, SCI_AUTOCSHOW, strlen(pre), (LPARAM)buffer);
			}
	}
*/
}

static void UpdateEditorTitle(editor_t *editor)
{
	char title[2048];
	char *encoding = "unknown";
	if (editor->oldsavefmt == editor->savefmt && editor->oldline == editor->curline)
		return;	//nothing changed.
	editor->oldsavefmt = editor->savefmt;
	editor->oldline = editor->curline;

	switch(editor->savefmt)
	{
	case UTF8_RAW:
		encoding = "utf-8";
		break;
	case UTF8_BOM:
		encoding = "utf-8(bom)";
		break;
	case UTF_ANSI:
		encoding = "unspecified";
		break;
	case UTF16LE:
		encoding = "utf-16(le)";
		break;
	case UTF16BE:
		encoding = "utf-16(be)";
		break;
	case UTF32LE:
		encoding = "utf-32(le)";
		break;
	case UTF32BE:
		encoding = "utf-32(be)";
		break;
	default:
		encoding = "unknown";
		break;
	}
	if (QCC_FindVFile(editor->filename))
		sprintf(title, "%s:%i - Virtual", editor->filename, 1+editor->curline);
	else if (editor->modified)
		sprintf(title, "*%s:%i - %s", editor->filename, 1+editor->curline, encoding);
	else
		sprintf(title, "%s:%i - %s", editor->filename, 1+editor->curline, encoding);
	SetWindowText(editor->window, title);
}

static LRESULT CALLBACK EditorWndProc(HWND hWnd,UINT message,
				     WPARAM wParam,LPARAM lParam)
{
	RECT rect;
	PAINTSTRUCT ps;

	editor_t *editor;
	for (editor = editors; editor; editor = editor->next)
	{
		if (editor->window == hWnd)
			break;
		if (editor->window == NULL)
			break;	//we're actually creating it now.
	}
	if (!editor)
		goto gdefault;

	switch (message)
	{
	case WM_CLOSE:
	case WM_QUIT:
		if (editor->modified)
		{
			switch (MessageBox(hWnd, "Would you like to save?", editor->filename, MB_YESNOCANCEL))
			{
			case IDCANCEL:
				return false;
			case IDYES:
				if (!EditorSave(editor))
					return false;
			case IDNO:
			default:
				break;
			}
		}
		goto gdefault;
	case WM_DESTROY:
		{
			editor_t *e;
			if (editor == editors)
			{
				editors = editor->next;
				free(editor);
				return 0;
			}
			for (e = editors; e; e = e->next)
			{
				if (e->next == editor)
				{
					e->next = editor->next;
					free(editor);
					return 0;
				}
			}
			MessageBox(0, "Couldn't destroy file reference", "WARNING", 0);
		}
		goto gdefault;
	case WM_CREATE:
		editor->editpane = CreateAnEditControl(hWnd, &editor->scintilla);
		if (richedit)
		{
			SendMessage(editor->editpane, EM_EXLIMITTEXT, 0, 1<<31);
			SendMessage(editor->editpane, EM_SETUNDOLIMIT, 256, 256);
		}

		editor->tooltip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, ghInstance, NULL);
		if (editor->tooltip)
		{
			TOOLINFO toolInfo = { 0 };
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hWnd;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRACK | TTF_ABSOLUTE;
			toolInfo.uId = (UINT_PTR)editor->editpane;
			toolInfo.lpszText = "";
			SendMessage(editor->tooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
			SendMessage(editor->tooltip, TTM_SETMAXTIPWIDTH, 0, 500);
		}
		goto gdefault;
	case WM_SETFOCUS:
		SetFocus(editor->editpane);
		goto gdefault;
	case WM_SIZE:
		GetClientRect(hWnd, &rect);
		SetWindowPos(editor->editpane, NULL, 0, 0, rect.right-rect.left, rect.bottom-rect.top, 0);
		goto gdefault;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		BeginPaint(hWnd,(LPPAINTSTRUCT)&ps);

		EndPaint(hWnd,(LPPAINTSTRUCT)&ps);
		return TRUE;
		break;
	case WM_SETCURSOR:
		if (!editor->scintilla)
		{
			POINT pos;
			char *newtext;
			TOOLINFO toolInfo = { 0 };
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hWnd;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRACK | TTF_ABSOLUTE;
			toolInfo.uId = (UINT_PTR)editor->editpane;
			newtext = GetTooltipText(editor, -1, FALSE);
			toolInfo.lpszText = editor->tooltiptext;
			if (!newtext)
				newtext = "";
			if (strcmp(editor->tooltiptext, newtext))
			{
				strncpy(editor->tooltiptext, newtext, sizeof(editor->tooltiptext)-1);
				SendMessage(editor->tooltip, TTM_UPDATETIPTEXT, (WPARAM)0, (LPARAM)&toolInfo);
				if (*editor->tooltiptext)
					SendMessage(editor->tooltip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&toolInfo);
				else
					SendMessage(editor->tooltip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&toolInfo);
			}

			GetCursorPos(&pos);
			if (pos.x >= 60)
				pos.x -= 60;
			else
				pos.x = 0;
			pos.y += 30;
			SendMessage(editor->tooltip, TTM_TRACKPOSITION, (WPARAM)0, MAKELONG(pos.x, pos.y));
		}
		goto gdefault;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == editor->editpane)
		{
			if (!editor->modified && !editor->scintilla)
			{
				CHARRANGE chrg;

				if (!editor->modified)
					editor->oldline=~0;
				editor->modified = true;
				if (EditorModified(editor))
					if (MessageBox(NULL, "warning: file was modified externally. reload?", "Modified!", MB_YESNO) == IDYES)
						EditorReload(editor);


				SendMessage(editor->editpane, EM_EXGETSEL, 0, (LPARAM) &chrg);
				editor->curline = Edit_LineFromChar(editor->editpane, chrg.cpMin);
				UpdateEditorTitle(editor);
			}
		}
		else
		{
//			if (mdibox)
//				goto gdefault;
			EditorMenu(editor, wParam);
		}
		break;
	case WM_CONTEXTMENU:
		{
			char buffer[1024];
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			HMENU menu = CreatePopupMenu();
			if (x == -1 && y == -1)
			{
				POINT p;
				GetCursorPos(&p);	//not the best. but too lazy to work out scintilla/richedit.
				x = p.x;
				y = p.y;
			}

			if (ReadTextSelection(editor, buffer, sizeof(buffer)))
			{
				char tmp[1024];
				QC_snprintfz(tmp, sizeof(tmp), "Go to definition: %s", buffer);
				AppendMenuA(menu, MF_ENABLED,
					IDM_GOTODEF, tmp);

				QC_snprintfz(tmp, sizeof(tmp), "Grep for %s", buffer);
				AppendMenuA(menu, MF_ENABLED,
					IDM_GREP, tmp);

				AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
			}

			AppendMenuA(menu, MF_ENABLED,
				IDM_DEBUG_TOGGLEBREAK,		"Toggle Breakpoint");

			if (gamewindow)
			{
				AppendMenuA(menu, MF_ENABLED, IDM_DEBUG_SETNEXT, "Set next statement");
				AppendMenuA(menu, MF_ENABLED, IDM_DEBUG_RUN, "Resume");
			}
			else
				AppendMenuA(menu, MF_ENABLED, IDM_DEBUG_RUN, "Begin Debugging");

			AppendMenuA(menu, MF_SEPARATOR, 0, NULL);

			AppendMenuA(menu, editor->modified?MF_ENABLED:(MF_DISABLED|MF_GRAYED),
				IDM_SAVE,		"Save File");
		//	AppendMenuA(menu, MF_ENABLED, IDM_FIND,		"&Find");
			AppendMenuA(menu, (editor->scintilla&&!SendMessage(editor->editpane, SCI_CANUNDO,0,0))?(MF_DISABLED|MF_GRAYED):MF_ENABLED,
				IDM_UNDO,		"Undo");
			AppendMenuA(menu, (editor->scintilla&&!SendMessage(editor->editpane, SCI_CANREDO,0,0))?(MF_DISABLED|MF_GRAYED):MF_ENABLED,
				IDM_REDO,		"Redo");
			AppendMenuA(menu, MF_ENABLED,
				IDM_CUT,		"Cut");
			AppendMenuA(menu, MF_ENABLED,
				IDM_COPY,		"Copy");
			AppendMenuA(menu, MF_ENABLED,
				IDM_PASTE,		"Paste");
			
			TrackPopupMenu(menu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON, x, y, 0, hWnd, NULL);
			DestroyMenu(menu);
		}
		break;
	case WM_NOTIFY:
		{
			NMHDR *nmhdr;
			nmhdr = (NMHDR *)lParam;
			if (editor->scintilla)
			{
				struct SCNotification *not = (struct SCNotification*)nmhdr;
				int pos = SendMessage(editor->editpane, SCI_GETCURRENTPOS, 0, 0);
				int l = SendMessage(editor->editpane, SCI_LINEFROMPOSITION, pos, 0);
				int mode;
				if (editor->curline != l)
					editor->curline = l;
				switch(nmhdr->code)
				{
				case SCN_MARGINCLICK:
					l = SendMessage(editor->editpane, SCI_LINEFROMPOSITION, not->position, 0);
					if (not->margin == 1)
					{
						/*fixme: should we scan the statements to ensure the line is valid? this applies to the f9 key too*/
						mode = !(SendMessage(editor->editpane, SCI_MARKERGET, l, 0) & 1);
						SendMessage(editor->editpane, mode?SCI_MARKERADD:SCI_MARKERDELETE, l, 0);
						EngineCommandf("qcbreakpoint %i \"%s\" %i\n", mode, editor->filename, l+1);
					}
					else if (not->margin == 2)
					{
						SendMessage(editor->editpane, SCI_TOGGLEFOLD, l, 0);
					}
					break;
				case SCN_CHARADDED:
					Scin_HandleCharAdded(editor, not, pos);
					break;
				case SCN_SAVEPOINTREACHED:
					editor->oldline=~0;
					editor->modified = false;
					break;
				case SCN_SAVEPOINTLEFT:
					editor->oldline=~0;
					editor->modified = true;

					if (EditorModified(editor))
						if (MessageBox(NULL, "warning: file was modified externally. reload?", "Modified!", MB_YESNO) == IDYES)
							EditorReload(editor);
					break;
				case SCN_UPDATEUI:
					{
						int pos1, pos2;
						if (strchr("{}[]()", SendMessage(editor->editpane, SCI_GETCHARAT, pos, 0)))
							pos1 = pos;
						else if (strchr("{}[]()", SendMessage(editor->editpane, SCI_GETCHARAT, pos-1, 0)))
							pos1 = pos-1;
						else
							pos1 = -1;
						if (pos1 != -1)
							pos2 = SendMessage(editor->editpane, SCI_BRACEMATCH, pos1, 0);
						else
							pos2 = -1;
						if (pos2 == -1)
							SendMessage(editor->editpane, SCI_BRACEBADLIGHT, pos1, 0);
						else
							SendMessage(editor->editpane, SCI_BRACEHIGHLIGHT, pos1, pos2);
					}
					break;
				case SCN_DWELLSTART:
					GetTooltipText(editor, not->position, TRUE);
					break;
				case SCN_DWELLEND:
				case SCN_FOCUSOUT:
					tooltip_editor = NULL;
					SendMessage(editor->editpane, SCI_CALLTIPCANCEL, 0, 0);
					break;
				}
				UpdateEditorTitle(editor);
			}
			else
			{
				SELCHANGE *sel;
				switch(nmhdr->code)
				{
				case EN_SELCHANGE:
					sel = (SELCHANGE *)nmhdr;
					editor->curline = Edit_LineFromChar(editor->editpane, sel->chrg.cpMin);
					UpdateEditorTitle(editor);
					break;
				}
			}
		}
	default:
	gdefault:
		if (mdibox)
			return DefMDIChildProc(hWnd,message,wParam,lParam);
		else
			return DefWindowProc(hWnd,message,wParam,lParam);
	}
	return 0;
}

static void EditorReload(editor_t *editor)
{
	struct stat sbuf;
	size_t flensz;
	char *rawfile;
	char *file;
	size_t flen;
	pbool dofree;
	rawfile = QCC_ReadFile(editor->filename, NULL, NULL, &flensz);
	flen = flensz;

	file = QCC_SanitizeCharSet(rawfile, &flen, &dofree, &editor->savefmt);

	stat(editor->filename, &sbuf);
	editor->filemodifiedtime = sbuf.st_mtime;

	if (editor->scintilla)
	{
		int endings = 0;
		char *e, *stop;
		for (e = file, stop=file+flen; e < stop; )
		{
			if (*e == '\r')
			{
				e++;
				if (*e == '\n')
				{
					e++;
					endings |= 4;
				}
				else
					endings |= 2;
			}
			else if (*e == '\n')
			{
				e++;
				endings |= 1;
			}
			else
				e++;
		}
		switch(endings)
		{
		case 0:	//new file with no endings, default to windows on windows.
		case 4:	//windows
			SendMessage(editor->editpane, SCI_SETEOLMODE, SC_EOL_CRLF, 0);
			SendMessage(editor->editpane, SCI_SETVIEWEOL, false, 0);
			break;
		case 1:	//unix
			SendMessage(editor->editpane, SCI_SETEOLMODE, SC_EOL_LF, 0);
			SendMessage(editor->editpane, SCI_SETVIEWEOL, false, 0);
			break;
		case 2:	//mac. traditionally qccs have never supported this. one of the mission packs has a \r in the middle of some single-line comment.
			SendMessage(editor->editpane, SCI_SETEOLMODE, SC_EOL_CR, 0);
			SendMessage(editor->editpane, SCI_SETVIEWEOL, false, 0);
			break;
		default:	//panic! everyone panic!
			SendMessage(editor->editpane, SCI_SETEOLMODE, SC_EOL_LF, 0);
			SendMessage(editor->editpane, SCI_SETVIEWEOL, true, 0);
			break;
		}

//		SendMessage(editor->editpane, SCI_SETTEXT, 0, (LPARAM)file);
//		SendMessage(editor->editpane, SCI_SETUNDOCOLLECTION, 0, 0);
		SendMessage(editor->editpane, SCI_SETTEXT, 0, (LPARAM)file);
//		SendMessage(editor->editpane, SCI_SETUNDOCOLLECTION, 1, 0);
		SendMessage(editor->editpane, EM_EMPTYUNDOBUFFER, 0, 0);
		SendMessage(editor->editpane, SCI_SETSAVEPOINT, 0, 0);

		if (editor->savefmt == UTF_ANSI)
			SendMessage(editor->editpane, SCI_SETCODEPAGE, 28591, 0);
		else
			SendMessage(editor->editpane, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
	}
	else
	{
		SendMessage(editor->editpane, EM_SETEVENTMASK, 0, 0);

		/*clear it out*/
		Edit_SetSel(editor->editpane,0,Edit_GetTextLength(editor->editpane));
		Edit_ReplaceSel(editor->editpane,"");

		if (file)
		{
			pbool errors;
			wchar_t *ch = QCC_makeutf16(file, flen, NULL, &errors);
			Edit_SetSel(editor->editpane,0,0);
			SetWindowTextW(editor->editpane, ch);
			if (errors)
			{
//				char msg[1024];
				editor->savefmt = UTF_ANSI;
				SetWindowTextA(editor->editpane, file);

//				QC_snprintfz(msg, sizeof(msg), "%s contains unicode encoding errors. File will be interpreted as ansi.", editor->filename);
//				MessageBox(editor->editpane, msg, "Encoding errors.", MB_ICONWARNING);
			}
			free(ch);
		}
		SendMessage(editor->editpane, EM_SETEVENTMASK, 0, ENM_SELCHANGE|ENM_CHANGE);
	}

	if (dofree)
		free(file);
	free(rawfile);

	editor->modified = false;


	if (editor->scintilla)
	{
	}
	else
	{
		CHARRANGE chrg;
		SendMessage(editor->editpane, EM_EXGETSEL, 0, (LPARAM) &chrg);
		editor->curline = Edit_LineFromChar(editor->editpane, chrg.cpMin);
	}
	UpdateEditorTitle(editor);
}

//line is 0-based. use -1 for no reselection
//setcontrol is the reason we're opening it.
//0: just load and go to the line.
//1: show the line as the executing one
//2: draw extra focus to it
void EditFile(const char *name, int line, pbool setcontrol)
{
	const char *ext;
	char title[1024];
	editor_t *neweditor;
	WNDCLASS wndclass;
	HMENU menu, menufile, menuhelp, menunavig;

	if (setcontrol)
	{
		for (neweditor = editors; neweditor; neweditor = neweditor->next)
		{
			if (neweditor->scintilla)
			{
				SendMessage(neweditor->editpane, SCI_MARKERDELETEALL, 1, 0);
				SendMessage(neweditor->editpane, SCI_MARKERDELETEALL, 2, 0);
			}
		}
	}
	if (!name)
		return;

	for (neweditor = editors; neweditor; neweditor = neweditor->next)
	{
		if (neweditor->window && !strcmp(neweditor->filename, name))
		{
			if (line >= 0)
			{
				if (setcontrol)
					Edit_SetSel(neweditor->editpane, Edit_LineIndex(neweditor->editpane, line+1)-1, Edit_LineIndex(neweditor->editpane, line+1)-1);
				else
					Edit_SetSel(neweditor->editpane, Edit_LineIndex(neweditor->editpane, line), Edit_LineIndex(neweditor->editpane, line+1)-1);
				Edit_ScrollCaret(neweditor->editpane);

				if (setcontrol && neweditor->scintilla)
				{
					SendMessage(neweditor->editpane, SCI_MARKERADD, line, 1);
					SendMessage(neweditor->editpane, SCI_MARKERADD, line, 2);
				}
			}
			if (mdibox)
				SendMessage(mdibox, WM_MDIACTIVATE, (WPARAM)neweditor->window, 0);
			SetFocus(neweditor->window);
			SetFocus(neweditor->editpane);
			return;
		}
	}

	if (QCC_RawFileSize(name) == -1)
	{
		QC_snprintfz(title, sizeof(title), "File not found:\n%s\nCreate it?", name);
		if (MessageBox(NULL, title, "Error", MB_ICONWARNING|MB_YESNO|MB_DEFBUTTON2) != IDYES)
			return;
	}

	ext = strrchr(name, '.');
	if (ext)
	{
		if (!QC_strcasecmp(ext, ".wav"))
		{
			size_t flensz;
			char *rawfile = QCC_ReadFile(name, NULL, NULL, &flensz);
			//fixme: thread this...
			BOOL (WINAPI *pPlaySound)(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
			HMODULE winmm = LoadLibrary("winmm.dll");
			pPlaySound = (void*)GetProcAddress(winmm, "PlaySoundA");
			if (pPlaySound)
				pPlaySound(rawfile, NULL, SND_MEMORY|SND_SYNC|SND_NODEFAULT);
			free(rawfile);
			return;
		}
		else if (!QC_strcasecmp(ext, ".ogg") || !QC_strcasecmp(ext, ".mp3") || !QC_strcasecmp(ext, ".opus") ||
				!QC_strcasecmp(ext, ".bsp") || !QC_strcasecmp(ext, ".mdl") || !QC_strcasecmp(ext, ".md2") || !QC_strcasecmp(ext, ".md3") || !QC_strcasecmp(ext, ".iqm") ||
				!QC_strcasecmp(ext, ".wad") || !QC_strcasecmp(ext, ".lmp") || !QC_strcasecmp(ext, ".png") || !QC_strcasecmp(ext, ".tga") || !QC_strcasecmp(ext, ".jpeg") ||
				!QC_strcasecmp(ext, ".jpg") || !QC_strcasecmp(ext, ".dds") || !QC_strcasecmp(ext, ".ktx") || !QC_strcasecmp(ext, ".bmp") || !QC_strcasecmp(ext, ".pcx") ||
				!QC_strcasecmp(ext, ".bin") || !QC_strcasecmp(ext, ".dat") || !QC_strcasecmp(ext, ".pak") || !QC_strcasecmp(ext, ".pk3") || !QC_strcasecmp(ext, ".dem") || !QC_strcasecmp(ext, ".spr"))
		{
			if (IDOK != MessageBox(NULL, "The file extension implies that it is a binary file. Open as text anyway?", "FTEQCCGUI", MB_OKCANCEL))
				return;
		}
	}

	neweditor = malloc(sizeof(editor_t));
	if (!neweditor)
	{
		MessageBox(NULL, "Low memory", "Error", 0);
		return;
	}

	neweditor->next = editors;
	editors = neweditor;

	neweditor->savefmt = UTF8_RAW;
	strncpy(neweditor->filename, name, sizeof(neweditor->filename)-1);

	if (!mdibox)
	{
		menu = CreateMenu();
		menufile = CreateMenu();
		menuhelp = CreateMenu();
		menunavig = CreateMenu();
		AppendMenu(menu, MF_POPUP, (UINT_PTR)menufile,	"&File");
		AppendMenu(menu, MF_POPUP, (UINT_PTR)menunavig,	"&Navigation");
		AppendMenu(menu, MF_POPUP, (UINT_PTR)menuhelp,	"&Help");
		AppendMenu(menufile, 0, IDM_OPENNEW,	"Open new file ");
		AppendMenu(menufile, 0, IDM_SAVE,		"&Save          ");
	//	AppendMenu(menufile, 0, IDM_FIND,		"&Find");
		AppendMenu(menufile, 0, IDM_UNDO,		"Undo          Ctrl+Z");
		AppendMenu(menufile, 0, IDM_REDO,		"Redo          Ctrl+Y");
		AppendMenu(menunavig, 0, IDM_GOTODEF, "Go to definition");
		AppendMenu(menunavig, 0, IDM_OPENDOCU, "Open selected file");
		AppendMenu(menuhelp, 0, IDM_ABOUT, "About");
	}
	else
		menu = NULL;


	
	wndclass.style      = 0;
	wndclass.lpfnWndProc   = EditorWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = ghInstance;
	wndclass.hIcon         = LoadIcon(ghInstance, IDI_ICON_FTEQCC);
	wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wndclass.hbrBackground = (void *)COLOR_WINDOW;
	wndclass.lpszMenuName  = 0;
	wndclass.lpszClassName = EDIT_WINDOW_CLASS_NAME;
	RegisterClass(&wndclass);

	neweditor->window = NULL;
	if (mdibox)
	{
		MDICREATESTRUCT mcs;

		sprintf(title, "%s", name);

		mcs.szClass = EDIT_WINDOW_CLASS_NAME;
		mcs.szTitle = name;
		mcs.hOwner = ghInstance;
		mcs.x = mcs.cx = CW_USEDEFAULT;
		mcs.y = mcs.cy = CW_USEDEFAULT;
		mcs.style = WS_OVERLAPPEDWINDOW|WS_MAXIMIZE;
		mcs.lParam = 0;

		neweditor->window = (HWND) SendMessage (mdibox, WM_MDICREATE, 0, 
			(LONG_PTR) (LPMDICREATESTRUCT) &mcs); 
	}
	else
	{
		sprintf(title, "%s - FTEEditor", name);

		neweditor->window=CreateWindow(EDIT_WINDOW_CLASS_NAME, title, WS_OVERLAPPEDWINDOW,
			0, 0, 640, 480, NULL, NULL, ghInstance, NULL);
	}

	if (menu)
		SetMenu(neweditor->window, menu);

	if (!neweditor->window)
	{
		MessageBox(NULL, "Failed to create editor window", "Error", 0);
		return;
	}
	SetWindowLongPtr(neweditor->window, GWLP_USERDATA, (LONG_PTR)neweditor);

	EditorReload(neweditor);

	if (line >= 0)
	{
		if (setcontrol)
			Edit_SetSel(neweditor->editpane, Edit_LineIndex(neweditor->editpane, line+1)-1, Edit_LineIndex(neweditor->editpane, line+1)-1);
		else
			Edit_SetSel(neweditor->editpane, Edit_LineIndex(neweditor->editpane, line), Edit_LineIndex(neweditor->editpane, line+1)-1);
	}
	else
		Edit_SetSel(neweditor->editpane, Edit_LineIndex(neweditor->editpane, 0), Edit_LineIndex(neweditor->editpane, 0));

	Edit_ScrollCaret(neweditor->editpane);

	ShowWindow(neweditor->window, SW_SHOW);
	SetFocus(mainwindow);
	SetFocus(neweditor->window);
	SetFocus(neweditor->editpane);

	if (setcontrol && neweditor->scintilla)
	{
		SendMessage(neweditor->editpane, SCI_MARKERADD, line, 1);
		SendMessage(neweditor->editpane, SCI_MARKERADD, line, 2);
	}
}

int EditorSave(editor_t *edit)
{
	struct stat sbuf;
	int len;
	wchar_t *wfile;
	char *afile;
	BOOL failed = TRUE;
	int saved = false;
	if (edit->scintilla)
	{
		//wordpad will corrupt any embedded quake chars if we force a bom, because it'll re-save using the wrong char encoding by default.
		int bomlen = 0;
		char *bom = "";
		if (edit->savefmt == UTF32BE || edit->savefmt == UTF32LE || edit->savefmt == UTF16BE)
			edit->savefmt = UTF16LE;

		if (edit->savefmt == UTF8_BOM)
		{
			bomlen = 3;
			bom = "\xEF\xBB\xBF";
		}
		else if (edit->savefmt == UTF16BE)
		{
			bomlen = 2;
			bom = "\xFE\xFF";
		}
		else if (edit->savefmt == UTF16LE)
		{
			bomlen = 2;
			bom = "\xFF\xFE";
		}
		else if (edit->savefmt == UTF32BE)
		{
			bomlen = 4;
			bom = "\x00\x00\xFE\xFF";
		}
		else if (edit->savefmt == UTF32LE)
		{
			bomlen = 4;
			bom = "\xFF\xFE\x00\x00";
		}
		len = SendMessage(edit->editpane, SCI_GETLENGTH, 0, 0);
		afile = malloc(bomlen+len+1);
		if (!afile)
		{
			MessageBox(NULL, "Save failed - not enough mem", "Error", 0);
			return false;
		}
		memcpy(afile, bom, bomlen);
		SendMessage(edit->editpane, SCI_GETTEXT, len+1, bomlen+(LPARAM)afile);

		//because wordpad saves in ansi by default instead of the format the file was originally saved in, we HAVE to use ansi without
		if (edit->savefmt != UTF8_BOM && edit->savefmt != UTF8_RAW)
		{
			int mchars;
			char *mc;
			int wchars = MultiByteToWideChar(CP_UTF8, 0, afile, len, NULL, 0);
			if (wchars)
			{
				wchar_t *wc = malloc(wchars * sizeof(wchar_t));
				MultiByteToWideChar(CP_UTF8, 0, afile, len, wc, wchars);

				if (edit->savefmt == UTF_ANSI)
				{
					mchars = WideCharToMultiByte(CP_ACP, 0, wc, wchars, NULL, 0, "", &failed);
					if (mchars)
					{
						mc = malloc(mchars);
						WideCharToMultiByte(CP_ACP, 0, wc, wchars, mc, mchars, "", &failed);
						if (!failed)
						{
							if (!QCC_WriteFile(edit->filename, mc, mchars))
								saved = -1;
							else
								saved = true;
						}
						free(mc);
					}
				}
				else
				{
					if (!QCC_WriteFile(edit->filename, wc, wchars))
						saved = -1;
					else
						saved = true;
				}
				free(wc);
			}
		}

		if (!saved)
		{
			if (!QCC_WriteFile(edit->filename, afile, bomlen+len))
				saved = -1;
			else
				saved = true;
		}
		free(afile);
		if (saved < 0)
		{
			MessageBox(NULL, "Save failed\nCheck path and ReadOnly flags", "Failure", 0);
			return false;
		}
		SendMessage(edit->editpane, SCI_SETSAVEPOINT, 0, 0);
	}
	else
	{
		if (edit->savefmt == UTF_ANSI)
		{
			len = GetWindowTextLengthA(edit->editpane);
			afile = malloc(len+1);
			if (!afile)
			{
				MessageBox(NULL, "Save failed - not enough mem", "Error", 0);
				return false;
			}
			GetWindowText(edit->editpane, afile, len+1);
			if (!QCC_WriteFile(edit->filename, afile, len))
			{
				free(afile);
				MessageBox(NULL, "Save failed\nCheck path and ReadOnly flags", "Failure", 0);
				return false;
			}
			free(afile);
		}
		else
		{
			len = GetWindowTextLengthW(edit->editpane);
			wfile = malloc((len+1)*2);
			if (!wfile)
			{
				MessageBox(NULL, "Save failed - not enough mem", "Error", 0);
				return false;
			}
			GetWindowTextW(edit->editpane, wfile, len+1);
			if (!QCC_WriteFileW(edit->filename, wfile, len))
			{
				free(wfile);
				MessageBox(NULL, "Save failed\nCheck path and ReadOnly flags", "Failure", 0);
				return false;
			}
			free(wfile);
		}
	}

	/*now whatever is on disk should have the current time*/
	edit->modified = false;
	stat(edit->filename, &sbuf);
	edit->filemodifiedtime = sbuf.st_mtime;

	//remove the * in a silly way.
	edit->oldline=~0;
	UpdateEditorTitle(edit);

	return true;
}
void EditorsRun(void)
{
}

static unsigned char *buf_get_malloc(void *ctx, size_t len)
{
	return malloc(len);
}
void *GUIReadFile(const char *fname, unsigned char *(*buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size, pbool issourcefile)
{
	editor_t *e;
	size_t blen;
	unsigned char *buffer;
	if (!buf_get)
		buf_get = buf_get_malloc;
	for (e = editors; e; e = e->next)
	{
		if (e->window && !strcmp(e->filename, fname))
		{
			//our qcc itself is fine with utf-16, so long as it has a BOM.
			if (e->scintilla)
			{	//take the opportunity to grab a predefined preprocessor list for this file
				char *deflist = QCC_PR_GetDefinesList();
				SendMessage(e->editpane, SCI_SETKEYWORDS,	4,	(LPARAM)deflist);
				free(deflist);

				{	//and just in case some system defs changed.
					char buffer[65536];
					GenBuiltinsList(buffer, sizeof(buffer));
					SendMessage(e->editpane, SCI_SETKEYWORDS,	1,	(LPARAM)buffer);
				}

				blen = SendMessage(e->editpane, SCI_GETLENGTH, 0, 0);
				buffer = buf_get(buf_ctx, blen+1);
				blen = SendMessage(e->editpane, SCI_GETTEXT, blen+1, (LPARAM)buffer);
			}
			else if (e->savefmt == UTF_ANSI)
			{
				blen = GetWindowTextLengthA(e->editpane);
				buffer = buf_get(buf_ctx, blen);
				GetWindowTextA(e->editpane, buffer, blen);
			}
			else
			{
				blen = (GetWindowTextLengthW(e->editpane)+1)*2;
				buffer = buf_get(buf_ctx, blen);
				*(wchar_t*)buffer = 0xfeff;
				GetWindowTextW(e->editpane, (wchar_t*)buffer+1, blen-sizeof(wchar_t));
			}

			if (e->modified)
			{
				if (EditorModified(e))
				{
					if (MessageBox(e->window, "File was modified on disk. Overwrite?", e->filename, MB_YESNO) == IDYES)
					{
						if (e->scintilla)
						{
							QCC_WriteFile(e->filename, buffer, blen);
							SendMessage(e->editpane, SCI_SETSAVEPOINT, 0, 0);	//tell the control that it was saved.
						}
						else
						{
							QCC_WriteFileW(e->filename, (wchar_t*)buffer+1, blen);
						}
					}
				}
			}

			*out_size = blen;
			return buffer;
		}
	}

	if (issourcefile)
		AddSourceFile(compilingrootfile,	fname);

	return QCC_ReadFile(fname, buf_get, buf_ctx, out_size);
}

int GUIFileSize(const char *fname)
{
	editor_t *e;
	for (e = editors; e; e = e->next)
	{
		if (e->window && !strcmp(e->filename, fname))
		{
			int len;
			if (e->scintilla)
				len = SendMessage(e->editpane, SCI_GETLENGTH, 0, 0);
			else if (e->savefmt == UTF_ANSI)
				len = GetWindowTextLengthA(e->editpane);
			else
				len = (GetWindowTextLengthW(e->editpane)+1)*2;
			return len;
		}
	}
	return QCC_PopFileSize(fname);
}

/*checks if the file has been modified externally*/
pbool EditorModified(editor_t *e)
{
	struct stat sbuf;
	stat(e->filename, &sbuf);
	if (e->filemodifiedtime != sbuf.st_mtime)
		return true;

	return false;
}

char *COM_ParseOut (const char *data, char *out, int outlen)
{
	int		c;
	int		len;

	len = 0;
	out[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}

//skip / * comments
	if (c == '/' && data[1] == '*')
	{
		data+=2;
		while(*data)
		{
			if (*data == '*' && data[1] == '/')
			{
				data+=2;
				goto skipwhite;
			}
			data++;
		}
		goto skipwhite;
	}

// handle marked up quoted strings specially (c-style, but with leading \ before normal opening ")
	if (c == '\\' && data[1] == '\"')
	{
		data+=2;
		while (1)
		{
			if (len >= outlen-2)
			{
				out[len] = '\0';
				return (char*)data;
			}

			c = *data++;
			if (!c)
			{
				out[len] = 0;
				return (char*)data-1;
			}
			if (c == '\\')
			{
				c = *data++;
				switch(c)
				{
				case '\r':
					if (*data == '\n')
						data++;
				case '\n':
					continue;
				case 'n':
					c = '\n';
					break;
				case 't':
					c = '\t';
					break;
				case 'r':
					c = '\r';
					break;
				case '$':
				case '\\':
				case '\'':
					break;
				case '"':
					c = '"';
					out[len] = c;
					len++;
					continue;
				default:
					c = '?';
					break;
				}
			}
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return (char*)data;
			}
			out[len] = c;
			len++;
		}
	}

// handle legacy quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= outlen-1)
			{
				out[len] = 0;
				return (char*)data;
			}

			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return (char*)data;
			}
			out[len] = c;
			len++;
		}
	}

// parse a regular word
	do
	{
		if (len >= outlen-1)
		{
			out[len] = 0;
			return (char*)data;
		}

		out[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	out[len] = 0;
	return (char*)data;
}

static void EngineGiveFocus(void)
{
	HWND game;
	if (gamewindow)
	{
		enginewindow_t *ctx = (enginewindow_t*)(LONG_PTR)GetWindowLongPtr(gamewindow, GWLP_USERDATA);
		if (ctx)
		{
			if (ctx->refocuswindow)
			{
				SetForegroundWindow(ctx->refocuswindow);
				return;
			}
		}

		SetFocus(gamewindow);
		game = GetWindow(gamewindow, GW_CHILD);
		if (game)
			SetForegroundWindow(game);	//make sure the game itself has focus
	}
}

static pbool EngineCommandWnd(HWND wnd, char *message)
{
	//qcresume			- resume running
	//qcinto			- singlestep. execute-with-debugging child functions
	//qcover			- singlestep. execute-without-debugging child functions
	//qcout				- singlestep. leave current function and enter parent.
	//qcbreak "$loc"	- set breakpoint
	//qcwatch "$var"	- set watchpoint
	//qcstack			- force-report stack trace
	enginewindow_t *ctx;
	if (wnd)
	{
		ctx = (enginewindow_t*)(LONG_PTR)GetWindowLongPtr(gamewindow, GWLP_USERDATA);
		if (ctx)
		{
			if (ctx->pipetoengine)
			{
				DWORD written = 0;
				WriteFile(ctx->pipetoengine, message, strlen(message), &written, NULL);
				return TRUE;
			}
		}
	}
	return FALSE;
}
static pbool EngineCommandf(char *message, ...)
{
	va_list		va;
	char		finalmessage[1024];
	va_start (va, message);
	vsnprintf (finalmessage, sizeof(finalmessage)-1, message, va);
	va_end (va);
	return EngineCommandWnd(gamewindow, finalmessage);
}
static pbool EngineCommandWndf(HWND wnd, char *message, ...)
{
	va_list		va;
	char		finalmessage[1024];
	va_start (va, message);
	vsnprintf (finalmessage, sizeof(finalmessage)-1, message, va);
	va_end (va);
	return EngineCommandWnd(wnd, finalmessage);
}

DWORD WINAPI threadwrapper(void *args)
{
	pbool hadstatus = false;
	enginewindow_t *ctx = args;
	{
		char workingdir[MAX_PATH+10];
		char absexe[MAX_PATH+10];
		char absbase[MAX_PATH+10];
		char mssucks[MAX_PATH+10];
		char *gah;
		PROCESS_INFORMATION childinfo;
		STARTUPINFO startinfo;
		SECURITY_ATTRIBUTES pipesec = {sizeof(pipesec), NULL, TRUE};
		char cmdline[8192];
		_snprintf(cmdline, sizeof(cmdline), "\"%s\" %s -qcdebug", enginebinary, enginecommandline);

		memset(&startinfo, 0, sizeof(startinfo));
		startinfo.cb = sizeof(startinfo);
		startinfo.hStdInput = NULL;
		startinfo.hStdError = NULL;
		startinfo.hStdOutput = NULL;
		startinfo.dwFlags |= STARTF_USESTDHANDLES;

		//create pipes for the stdin/stdout.
		CreatePipe(&ctx->pipefromengine, &startinfo.hStdOutput, &pipesec, 0);
		CreatePipe(&startinfo.hStdInput, &ctx->pipetoengine, &pipesec, 0);

		SetHandleInformation(ctx->pipefromengine, HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(ctx->pipetoengine, HANDLE_FLAG_INHERIT, 0);

		//let the engine know who to give focus to 
		{
			char message[256];
			DWORD written;
			_snprintf(message, sizeof(message)-1, "debuggerwnd %#"PRIxPTR"\n",  (uintptr_t)(void*)mainwindow);
			WriteFile(ctx->pipetoengine, message, strlen(message), &written, NULL);
		}

		//let the engine know which window to embed itself in
		if (ctx->embedtype)
		{
			char message[256];
			DWORD written;
			RECT rect;
			GetClientRect(ctx->window, &rect);
			_snprintf(message, sizeof(message)-1, "vid_recenter %i %i %i %i %#"PRIxPTR"\n", 0, 0, (int)(rect.right - rect.left), (int)(rect.bottom-rect.top), (uintptr_t)(void*)ctx->window);
			WriteFile(ctx->pipetoengine, message, strlen(message), &written, NULL);
		}

		GetCurrentDirectory(sizeof(workingdir)-1, workingdir);
		strcpy(mssucks, enginebasedir);
		while ((gah = strchr(mssucks, '/')))
			*gah = '\\';
		PathCombine(absbase, workingdir, mssucks);
		strcpy(mssucks, enginebinary);
		while ((gah = strchr(mssucks, '/')))
			*gah = '\\';
		PathCombine(absexe, absbase, mssucks);
		if (!CreateProcess(absexe, cmdline, NULL, NULL, TRUE, 0, NULL, absbase, &startinfo, &childinfo))
		{
			HRESULT hr = GetLastError();
			switch(hr)
			{
			case ERROR_FILE_NOT_FOUND:
				MessageBox(mainwindow, "File Not Found", "Cannot Start Engine", 0);
				break;
			case ERROR_PATH_NOT_FOUND:
				MessageBox(mainwindow, "Path Not Found", "Cannot Start Engine", 0);
				break;
			case ERROR_ACCESS_DENIED:
				MessageBox(mainwindow, "Access Denied", "Cannot Start Engine", 0);
				break;
			default:
				MessageBox(mainwindow, qcva("gla: %x", (unsigned)hr), "Cannot Start Engine", 0);
				break;
			}
			hadstatus = true;	//don't warn about other stuff
		}

		//these ends of the pipes were inherited by now, so we can discard them in the caller.
		CloseHandle(startinfo.hStdOutput);
		CloseHandle(startinfo.hStdInput);
	}

	{
		char buffer[8192];
		unsigned int bufoffs = 0;
		char *nl;
		while(1)
		{
			DWORD avail;
			//use Peek so we can read exactly how much there is without blocking, so we don't have to read byte-by-byte.
			PeekNamedPipe(ctx->pipefromengine, NULL, 0, NULL, &avail, NULL);
			if (!avail)
				avail = 1;	//so we do actually sleep.
			if (avail > sizeof(buffer)-1 - bufoffs)
				avail = sizeof(buffer)-1 - bufoffs;
			if (!ReadFile(ctx->pipefromengine, buffer + bufoffs, avail, &avail, NULL) || !avail)
			{
				break;
			}

			bufoffs += avail;
			while(1)
			{
				buffer[bufoffs] = 0;
				nl = strchr(buffer, '\n');
				if (nl)
				{
					*nl = 0;
					if (!strncmp(buffer, "status ", 7))
					{
						//SetWindowText(ctx->window, buffer+7);
						hadstatus = true;
					}
					else if (!strcmp(buffer, "status"))
					{
						//SetWindowText(ctx->window, "Engine");
						hadstatus = true;
					}
					else if (!strcmp(buffer, "curserver"))
					{
						//not interesting
					}
					else if (!strncmp(buffer, "qcstack ", 6))
					{
						//qcvm is giving a stack trace
						//stack reset
						//stack "$func" "$loc"
						//local $depth
					}
					else if (!strncmp(buffer, "qcstep ", 7) || !strncmp(buffer, "qcfault ", 8))
					{
						//post it, because of thread ownership issues.
						static char filenamebuffer[256];
						char line[16];
						char error[256];
						char *l = COM_ParseOut(buffer+7, filenamebuffer, sizeof(filenamebuffer));
						while (*l == ' ')
							l++;
						if (*l == ':')
							l++;
						l = COM_ParseOut(l, line, sizeof(line));
						l = COM_ParseOut(l, error, sizeof(error));
						PostMessage(ctx->window, WM_USER, atoi(line), (LPARAM)filenamebuffer);	//and tell the owning window to try to close it again
						if (*error)
							PostMessage(ctx->window, WM_USER+3, 0, (LPARAM)strdup(error));	//and tell the owning window to try to close it again
					}
					else if (!strncmp(buffer, "qcvalue ", 8))
					{
						//qcvalue "$variableformula" "$value"
						//update tooltip to show engine's current value
						PostMessage(ctx->window, WM_USER+2, 0, (LPARAM)strdup(buffer+8));	//and tell the owning window to try to close it again
					}
					else if (!strncmp(buffer, "qcreloaded ", 10))
					{
						//so we can resend any breakpoint commands
						//qcreloaded "$vmname" "$progsname"
						char caption[256];
						HWND gw = GetWindow(ctx->window, GW_CHILD);
						if (gw)
						{
							GetWindowText(gw, caption, sizeof(caption));
							SetWindowText(ctx->window, caption);
						}
						PostMessage(ctx->window, WM_USER+1, 0, 0);	//and tell the owning window to try to close it again
						hadstatus = true;
					}
					else if (!strncmp(buffer, "refocuswindow", 13) && (buffer[13] == ' ' || !buffer[13]))
					{
						char *l = buffer+13;
						while(*l == ' ')
							l++;
						ctx->refocuswindow = (HWND)(size_t)strtoull(l, &l, 0);
						ShowWindow(ctx->window, SW_HIDE);
						hadstatus = true;
					}
					else
					{
						//handle anything else we need to handle here
						printf("Unknown command from engine \"%s\"\n", buffer);
					}
					nl++;
					bufoffs -= (nl-buffer);
					memmove(buffer, nl, bufoffs);
				}
				else
					break;
			}
		}
		CloseHandle(ctx->pipefromengine);
		ctx->pipefromengine = NULL;
		CloseHandle(ctx->pipetoengine);
		ctx->pipetoengine = NULL;

		if (!hadstatus)
			MessageBox(mainwindow, "Engine terminated without acknowledging debug session.\nCurrently only FTE supports debugging.", "Debugging Failed", MB_OK);
	}

	ctx->pipeclosed = true;
	PostMessage(ctx->window, WM_CLOSE, 0, 0);	//and tell the owning window to try to close it again
	return 0;
}

static LRESULT CALLBACK EngineWndProc(HWND hWnd,UINT message,
				     WPARAM wParam,LPARAM lParam)
{
	enginewindow_t *ctx;
	editor_t *editor;
	switch (message)
	{
	case WM_CREATE:
		ctx = malloc(sizeof(*ctx));
		memset(ctx, 0, sizeof(*ctx));
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);
		ctx->window = hWnd;
		ctx->embedtype = (size_t)((CREATESTRUCT*)lParam)->lpCreateParams;
		ctx->thread = (HANDLE)CreateThread(NULL, 0, threadwrapper, ctx, 0, &ctx->tid);
		break;
	case WM_SIZE:
		ctx = (enginewindow_t*)(LONG_PTR)GetWindowLongPtr(gamewindow, GWLP_USERDATA);
		if (ctx && ctx->embedtype)
		{
			RECT r;
			GetClientRect(hWnd, &r);
			EngineCommandWndf(hWnd, "vid_recenter %i %i %i %i %#p\n", r.left, r.top, r.right-r.left, r.bottom - r.top, (void*)ctx->window);
		}
		goto gdefault;
	case WM_CLOSE:
		//ask the engine to quit
		ctx = (enginewindow_t*)(LONG_PTR)GetWindowLongPtr(gamewindow, GWLP_USERDATA);
		if (ctx && !ctx->pipeclosed)
		{
			EngineCommandWnd(hWnd, "quit force\n");
			break;
		}
		goto gdefault;
	case WM_DESTROY:
		EngineCommandWnd(hWnd, "quit force\n");	//just in case
		ctx = (enginewindow_t*)(LONG_PTR)GetWindowLongPtr(gamewindow, GWLP_USERDATA);
		if (ctx)
		{
			WaitForSingleObject(ctx->thread, INFINITE);
			CloseHandle(ctx->thread);
			free(ctx);
		}
		if (hWnd == gamewindow)
		{
			SplitterRemove(watches);
			gamewindow = NULL;
		}
		break;
	case WM_USER:
		//engine broke. show code.
		if (lParam)
			SetForegroundWindow(mainwindow);
		EditFile((char*)lParam, wParam-1, true);

		if (watches)
		{
			char text[MAX_PATH];
			int i, lim = ListView_GetItemCount(watches);
			for (i = 0; i < lim; i++)
			{
				ListView_GetItemText(watches, i, 0, text, sizeof(text));
				EngineCommandWndf(hWnd, "qcinspect \"%s\" \"%s\"\n", text, ""); //term, scope
			}
		}
		break;
	case WM_USER+1:
		//engine loaded a progs, reset breakpoints.
		for (editor = editors; editor; editor = editor->next)
		{
			int line = -1;
			if (!editor->scintilla)
				continue;

			for (;;)
			{
				line = SendMessage(editor->editpane, SCI_MARKERNEXT, line, 1);
				if (line == -1)
					break;	//no more.
				line++;

				EngineCommandWndf(hWnd, "qcbreakpoint 1 \"%s\" %i\n", editor->filename, line);
			}
		}
		//and now let the engine continue
		SetFocus(hWnd);
		EngineCommandWnd(hWnd, "qcresume\n");
		break;
	case WM_USER+2:
		{
			char varname[1024];
			char varvalue[1024];
			char *line = (char*)lParam;
			line = COM_ParseOut(line, varname, sizeof(varname));
			line = COM_ParseOut(line, varvalue, sizeof(varvalue));
			if (tooltip_editor && !strcmp(varname, tooltip_variable))
			{
				char tip[2048];
				if (*tooltip_comment)
					_snprintf(tip, sizeof(tip)-1, "%s %s = %s\r\n%s", tooltip_type, tooltip_variable, varvalue, tooltip_comment);
				else
					_snprintf(tip, sizeof(tip)-1, "%s %s = %s", tooltip_type, tooltip_variable, varvalue);

				SendMessage(tooltip_editor->editpane, SCI_CALLTIPSHOW, (WPARAM)tooltip_position, (LPARAM)tip);
			}
			if (watches)
			{
				char text[MAX_PATH];
				int i, lim = ListView_GetItemCount(watches);
				for (i = 0; i < lim; i++)
				{
					ListView_GetItemText(watches, i, 0, text, sizeof(text));
					if (!strcmp(text, varname))
						ListView_SetItemText(watches, i, 1, varvalue);
				}
			}
			free((char*)lParam);
		}
		break;
	case WM_USER+3:
		{
			char *msg = (char*)lParam;
			MessageBox(mainwindow, msg, "QC Fault", 0);
			free(msg);
		}
		break;

	default:
	gdefault:
		return DefMDIChildProc(hWnd,message,wParam,lParam);
	}
	return 0;
}
static INT CALLBACK StupidBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData) 
{	//'stolen' from microsoft's knowledge base.
	//required to work around microsoft being annoying.
	TCHAR szDir[MAX_PATH];
	char *foo;
	switch(uMsg) 
	{
	case BFFM_INITIALIZED: 
		if (GetCurrentDirectory(sizeof(szDir)/sizeof(TCHAR), szDir))
		{
			foo = strrchr(szDir, '\\');
			if (foo)
				*foo = 0;
			foo = strrchr(szDir, '\\');
			if (foo)
				*foo = 0;
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)szDir);
		}
		break;
	case BFFM_SELCHANGED: 
		if (SHGetPathFromIDList((LPITEMIDLIST) lp ,szDir))
		{
			while((foo = strchr(szDir, '\\')))
				*foo = '/';
			//fixme: verify that id1 is a subdir perhaps?
			SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
		}
		break;
	}
	return 0;
}

pbool PromptForFile(const char *prompt, const char *filter, const char *basepath, const char *defaultfile, char outname[], size_t outsize, pbool create)
{
	char oldworkingdir[MAX_PATH+10];	//cmdlg changes it...
	char workingdir[MAX_PATH+10];

#ifndef OFN_DONTADDTORECENT
#define OFN_DONTADDTORECENT 0x02000000
#endif

	char *s;
	char initialdir[MAX_PATH+10];
	char absengine[MAX_PATH+10];
	OPENFILENAME ofn;
	pbool okay;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = mainwindow;
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = absengine;
	ofn.Flags = OFN_EXPLORER|(create?OFN_PATHMUSTEXIST|OFN_CREATEPROMPT:OFN_FILEMUSTEXIST)|OFN_DONTADDTORECENT;
	ofn.lpstrTitle = prompt;
	ofn.nMaxFile = outsize-1;
	ofn.lpstrFilter = filter;
	GetCurrentDirectory(sizeof(oldworkingdir)-1, oldworkingdir);
	memcpy(workingdir, oldworkingdir, sizeof(workingdir));
	_snprintf(absengine, sizeof(absengine), "%s/%s", basepath, defaultfile);
	for (s = absengine; *s; s++)
		if (*s == '/')
			*s = '\\';
	strrchr(absengine, '\\')[1] = 0;
	PathCombine(initialdir, workingdir, absengine);
	if (strchr(defaultfile, '/'))
		strcpy(absengine, strrchr(defaultfile, '/')+1);
	else
		strcpy(absengine, defaultfile);
	//and the fuck-you-microsoft loop
	for (s = initialdir; *s; s++)
		if (*s == '/')
			*s = '\\';
	ofn.lpstrInitialDir = initialdir;
	okay = GetOpenFileName(&ofn);
	while (!okay)
	{
		switch(CommDlgExtendedError())
		{
		case FNERR_INVALIDFILENAME:
			*outname = 0;
			okay = GetOpenFileName(&ofn);
			continue;
		}
		break;
	}
	if (!PathRelativePathToA(outname, initialdir, FILE_ATTRIBUTE_DIRECTORY, absengine, FILE_ATTRIBUTE_DIRECTORY))
		QC_strlcpy(outname, absengine, sizeof(outsize));
	if (!strncmp(outname, ".\\", 2))
		memmove(outname, outname+2, strlen(outname+2)+1);
	//undo any damage caused by microsoft's stupidity
	SetCurrentDirectory(oldworkingdir);
	return okay;
}
void PromptForEngine(int force)
{
	char oldworkingdir[MAX_PATH+10];	//cmdlg changes it...
	char workingdir[MAX_PATH+10];
	GetCurrentDirectory(sizeof(oldworkingdir)-1, oldworkingdir);
	if (!*enginebasedir || force==1)
	{
		BROWSEINFO bi;
		LPITEMIDLIST il;
		memset(&bi, 0, sizeof(bi));
		bi.hwndOwner = mainwindow;
		bi.pidlRoot = NULL;
		GetCurrentDirectory(sizeof(workingdir)-1, workingdir);
		bi.pszDisplayName = workingdir;
		bi.lpszTitle = "Please locate your base directory";
		bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT;
		bi.lpfn = StupidBrowseCallbackProc;
		bi.lParam = 0;
		bi.iImage = 0;
		il = SHBrowseForFolder(&bi);
		SetCurrentDirectory(oldworkingdir);	//revert microsoft stupidity.
		if (il)
		{
			char *foo;
			char absbase[MAX_PATH+10];
			SHGetPathFromIDList(il, absbase);
			CoTaskMemFree(il);
			GetCurrentDirectory(sizeof(workingdir)-1, workingdir);
			//use the relative path instead. this'll be stored in a file, and I expect people will zip+email without thinking.
			if (!PathRelativePathToA(enginebasedir, workingdir, FILE_ATTRIBUTE_DIRECTORY, absbase, FILE_ATTRIBUTE_DIRECTORY))
				QC_strlcpy(enginebasedir, absbase, sizeof(enginebasedir));
			while((foo = strchr(enginebasedir, '\\')))
				*foo = '/';
		}
		else
			return;

		if (optionsmenu)
			DestroyWindow(optionsmenu);
		buttons[ID_OPTIONS].washit = true;
	}

	if (!*enginebinary || force==2)
	{
		if (!PromptForFile("Please choose an engine", "Executables\0*.exe\0All files\0*.*\0", enginebasedir, "fteglqw.exe", enginebinary, sizeof(enginebinary), false))
			return;

		if (optionsmenu)
			DestroyWindow(optionsmenu);
		buttons[ID_OPTIONS].washit = true;

		if (*enginebinary && (!*enginecommandline || force==2))
		{
			char absbase[MAX_PATH+10];
			char guessdir[MAX_PATH+10];
			char *slash;
			GetCurrentDirectory(sizeof(workingdir)-1, workingdir);
			_snprintf(guessdir, sizeof(guessdir), "%s/", enginebasedir);
			for (slash = guessdir; *slash; slash++)
				if (*slash == '/')
					*slash = '\\';
			PathCombine(absbase, workingdir, guessdir);
			if (PathRelativePathToA(guessdir, absbase, FILE_ATTRIBUTE_DIRECTORY, workingdir, FILE_ATTRIBUTE_DIRECTORY))
			{
				if (!strncmp(guessdir, ".\\", 2))
					memmove(guessdir, guessdir+2, strlen(guessdir+2)+1);
				slash = strchr(guessdir, '/');
				if (slash)
					*slash = 0;
				slash = strchr(guessdir, '\\');
				if (slash)
					*slash = 0;
				if (!*guessdir)
					QC_snprintfz(enginecommandline, sizeof(enginecommandline), "-window -nohome");
				else if (!strchr(guessdir, ' '))
					QC_snprintfz(enginecommandline, sizeof(enginecommandline), "-window -nohome -game %s", guessdir);
				else
					QC_snprintfz(enginecommandline, sizeof(enginecommandline), "-window -nohome -game \"%s\"", guessdir);
			}
		}

	}
}
void RunEngine(void)
{
	size_t embedtype = 0;	//0 has focus issues.
	if (!gamewindow)
	{
		WNDCLASS wndclass;
		MDICREATESTRUCT mcs;

		PromptForEngine(0);

		memset(&wndclass, 0, sizeof(wndclass));
		wndclass.style      = 0;
		wndclass.lpfnWndProc   = EngineWndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = ghInstance;
		wndclass.hIcon         = 0;
		wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
		wndclass.hbrBackground = (void *)COLOR_WINDOW;
		wndclass.lpszMenuName  = 0;
		wndclass.lpszClassName = ENGINE_WINDOW_CLASS_NAME;
		RegisterClass(&wndclass);

		if (embedtype != 2)
		{
			gamewindow = CreateWindowA(ENGINE_WINDOW_CLASS_NAME, "Debug", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, NULL, NULL, ghInstance, (void*)embedtype);
			if (embedtype)
				ShowWindow(gamewindow, SW_SHOW);
		}
		else
		{
			memset(&mcs, 0, sizeof(mcs));
			mcs.szClass = ENGINE_WINDOW_CLASS_NAME;
			mcs.szTitle = "Debug";
			mcs.hOwner = ghInstance;
			mcs.x = CW_USEDEFAULT;
			mcs.y = CW_USEDEFAULT;
			mcs.cx = 640;
			mcs.cy = 480;
			mcs.style = WS_OVERLAPPEDWINDOW;
			mcs.lParam = embedtype;

			gamewindow = (HWND) SendMessage (mdibox, WM_MDICREATE, 0, (LONG_PTR) (LPMDICREATESTRUCT) &mcs); 
		}
		SplitterFocus(watches, 64, 64);
	}
	else
	{
//		enginewindow_t *e = (enginewindow_t*)(LONG_PTR)GetWindowLongPtr(gamewindow, GWLP_USERDATA);
	}
//	SendMessage(mdibox, WM_MDIACTIVATE, (WPARAM)gamewindow, 0);
	PostMessage(mainwindow, WM_SIZE, 0, 0);
}

static void SetProgsSrcFileAndPath(char *filename)
{
	char *s, *s2;
	strcpy(progssrcdir, filename);
	for(s = progssrcdir; s; s = s2)
	{
		s2 = strchr(s+1, '\\');
		if (!s2)
			break;
		s = s2;
	}
	if (s)
	{
		*s = '\0';
		strcpy(progssrcname, s+1);
	}
	else
		strcpy(progssrcname, filename);

	SetCurrentDirectory(progssrcdir);
	*progssrcdir = '\0';
}

qcc_cachedsourcefile_t *androidfiles;
static void Android_FreeFiles(void)
{
	qcc_cachedsourcefile_t *f;
	while((f = androidfiles))
	{
		androidfiles = f->next;
		free(f);
	}
}
static void Android_CopyFile(const char *name, const void *compdata, size_t compsize, int method, size_t plainsize)
{
	qcc_cachedsourcefile_t *nf, **link;
	if (!strncmp(name, "META-INF", 8))
		return;	//ignore any existing signatures.
	for (link = &androidfiles; *link; link = &(*link)->next)
	{
		nf = *link;
		if (!stricmp(name, nf->filename))
		{	//nuke the old file if we have a dupe.
			*link = nf->next;
			free(nf);
			break;
		}
	}

	nf = malloc(sizeof(*nf) + plainsize);
	if (!nf)
	{
		GUIprintf("Error: out of memory\n", name, plainsize);
		return;
	}
	QC_strlcpy(nf->filename, name, sizeof(nf->filename));
	nf->file = (char*)(nf+1);
	nf->size = plainsize;
	nf->type = FT_DATA;

	if (QC_decode(NULL, compsize, nf->size, method, compdata, nf->file))
	{
		GUIprintf("Android: Including %s (%i bytes)\n", name, plainsize);
		nf->next = androidfiles;
		androidfiles = nf;
	}
	else
	{
		GUIprintf("Android: Unable to read %s from source apk\n", name, plainsize);
		free(nf);
	}
}
static pbool Android_PrepareAPK(FILE *f)
{
	if (f)
	{
		char *buf;
		size_t size;

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		buf = malloc(size);
		fread(buf, 1, size, f);
		fclose(f);
		QC_EnumerateFilesFromBlob(buf, size, Android_CopyFile);
		free(buf);
		return true;
	}
	return false;
}
void GUI_CreateInstaller_Android(void)
{
	FILE *f;
	char inputapkname[MAX_PATH];
	//files
	char *keystore = "my-release-key.keystore";
	char targetapk[MAX_PATH];
	//other stuff
	char *storepass = "fte123";
	char *alias = "FTEDroid";
	char tmp[MAX_PATH];
	char *mandata = NULL;
	char *pngdata = NULL;
	char *modname = "my_application";
	size_t manlen, pnglen;
	int h;
	char cmdline[2048];
	FILE *inputapk;

	if (MessageBox(mainwindow, "The 'Create Installer' option is still experimental.\nIt's probably still defective.\nSo be sure to test stuff extensively.", "Create Installer", MB_OKCANCEL|MB_DEFBUTTON2) != IDOK)
		return;

	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "default.fmf");
		mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "%s.fmf", modname);
		mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "../default.fmf");
		mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "../%s.fmf", modname);
		mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "%s.fmf", modname);
		if (PromptForFile("Please Select Manifest File", "FTE Manifests\0*.fmf\0All files\0*.*\0", ".", tmp, tmp, sizeof(tmp), false))
			mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		MessageBox(mainwindow, "No manifest selected.\n", "Create Installer", MB_OK|MB_ICONERROR);
		return;
	}

	PathCombine(inputapkname, enginebasedir, "FTEDroid.apk");
	inputapk = fopen(inputapkname, "rb");
	if (!inputapk)
	{
		if (PromptForFile("Please find base FTE android package", "The FTE Android Package\0*.apk\0All files\0*.*\0", enginebasedir, "FTEDroid.apk", tmp, sizeof(tmp), false))
		{
			PathCombine(inputapkname, enginebasedir, tmp);
			inputapk = fopen(inputapkname, "rb");
		}
	}

	if (inputapk)
	{
		QC_snprintfz(tmp, sizeof(tmp), "%s.apk", modname);
		if (PromptForFile("Please output apk", "Android Packages\0*.apk\0All files\0*.*\0", ".", tmp, tmp, sizeof(tmp), true))
			PathCombine(targetapk, enginebasedir, tmp);

		GUIprintf("");

		//read the files from an existing apk
		if (!Android_PrepareAPK(inputapk))
			MessageBox(mainwindow, "Unable to read source package", "Create Installer", MB_OK|MB_ICONERROR);
		else
		{
			//add/replace some existing files
			pngdata = QCC_ReadFile ("../droid_72.png", NULL, 0, &pnglen);
			if (!pngdata)
				GUIprintf("Could not open ../droid_72.png launcher icon\n");
			Android_CopyFile("res/drawable-hdpi/icon.png", pngdata, pnglen, 0, pnglen);
			free(pngdata);

			pngdata = QCC_ReadFile ("../droid_48.png", NULL, 0, &pnglen);
			if (!pngdata)
				GUIprintf("Could not open ../droid_48.png launcher icon\n");
			Android_CopyFile("res/drawable-mdpi/icon.png", NULL, 0, 0, 0);
			free(pngdata);

			Android_CopyFile("default.fmf", mandata, manlen, 0, manlen);

			//write out the new zip... err... apk... :)
			h = SafeOpenWrite (targetapk, 2*1024*1024);
			if (h < 0)
			{
				GUIprintf("Unable to open %s\n", targetapk);
			}
			else
			{
				progfuncs_t funcs;
				progexterns_t ext;
				memset(&funcs, 0, sizeof(funcs));
				funcs.funcs.parms = &ext;
				memset(&ext, 0, sizeof(ext));
				ext.ReadFile = GUIReadFile;
				ext.FileSize = GUIFileSize;
				ext.WriteFile = QCC_WriteFile;
				ext.Sys_Error = Sys_Error;
				ext.Printf = GUIprintf;
				qccprogfuncs = &funcs;
				WriteSourceFiles(androidfiles, h, true, false);
				if (!SafeClose(h))
					GUIprintf("Error: Unable to write output android package %s\n", targetapk);
				else
				{
					f = fopen(keystore, "rb");
					if (f)
						fclose(f);
					else
					{
						GUIprintf("Key store does not exist. Trying to create\n");
						//try to create a keystore for them, as this is their first time.
						QC_snprintfz(cmdline, sizeof(cmdline), "keytool -genkey -keystore %s -storepass %s -keypass %s -alias %s -keyalg RSA -keysize 2048 -validity 10000", keystore, storepass, storepass, alias);
						system(cmdline);
					}

					f = fopen(keystore, "rb");
					if (f)
					{
						fclose(f);
						//we now need to invoke the jarsigner program, so I hope you have java installed.
						QC_snprintfz(cmdline, sizeof(cmdline), "jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore %s -storepass %s %s %s", keystore, storepass, targetapk, alias);
						if (EXIT_SUCCESS == system(cmdline))
							GUIprintf("Android Package Complete. Go ahead and test it!\n");
						else
							GUIprintf("Failed to sign package.\n");
					}
					else
						GUIprintf("Keystore creation failed or was aborted.\n");
				}
			}
		}
	}
	Android_FreeFiles();

	free(mandata);
}

#ifdef AVAIL_PNGLIB
//size info that microsoft recommends
static const struct
{
	int width;
	int height;
	int bpp;
} icosizes[] = {
//	{96, 96, 32},
	{48, 48, 32},
	{32, 32, 32},
	{16, 16, 32},
//	{16, 16, 4},
//	{48, 48, 4},
//	{32, 32, 4},
//	{16, 16, 1},
//	{48, 48, 1},
//	{32, 32, 1},
	{256, 256, 32}	//vista!
};
#endif

//dates back to 16bit windows. bah.
#pragma pack(push)
#pragma pack(2)
typedef struct
{
	WORD idReserved;
	WORD idType;
	WORD idCount;
	struct
	{
		BYTE  bWidth;
		BYTE  bHeight;
		BYTE  bColorCount;
		BYTE  bReserved;
		WORD  wPlanes;
		WORD  wBitCount;
		DWORD dwBytesInRes;
		WORD  nId;
	} idEntries[256];
} icon_group_t;
#pragma pack(pop)


#ifdef AVAIL_PNGLIB
static void Image_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow;
	unsigned	frac, fracstep;

	/*if (gl_lerpimages.ival)
	{
		Image_Resample32Lerp(in, inwidth, inheight, out, outwidth, outheight);
		return;
	}*/

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = outwidth*fracstep;
		j=outwidth;
		while ((j)&3)
		{
			j--;
			frac -= fracstep;
			out[j] = inrow[frac>>16];
		}
		for ( ; j>=4 ;)
		{
			j-=4;
			frac -= fracstep;
			out[j+3] = inrow[frac>>16];
			frac -= fracstep;
			out[j+2] = inrow[frac>>16];
			frac -= fracstep;
			out[j+1] = inrow[frac>>16];
			frac -= fracstep;
			out[j+0] = inrow[frac>>16];
		}
	}
}
#endif

#ifndef MSVCLIBSPATH
#ifdef MSVCLIBPATH
	#define MSVCLIBSPATH STRINGIFY(MSVCLIBPATH)
#elif _MSC_VER == 1200
	#define MSVCLIBSPATH "../" "../libs/vc6-libs/"
#else
	#define MSVCLIBSPATH "../" "../libs/"
#endif
#endif

#ifdef AVAIL_PNGLIB
	#ifndef AVAIL_ZLIB
		#error PNGLIB requires ZLIB
	#endif

	#undef channels

	#ifndef PNG_SUCKS_WITH_SETJMP
		#if defined(MINGW)
			#include "./mingw-libs/png.h"
		#elif defined(_WIN32)
			#include "../png.h"
		#else
			#include <png.h>
		#endif
	#endif

	#ifdef DYNAMIC_LIBPNG
		#define PSTATIC(n)
		static dllhandle_t *libpng_handle;
		#define LIBPNG_LOADED() (libpng_handle != NULL)
	#else
		#define LIBPNG_LOADED() 1
		#define PSTATIC(n) = &n
		#ifdef _MSC_VER
			#ifdef _WIN64
				#pragma comment(lib, MSVCLIBSPATH "libpng64.lib")
			#else
				#pragma comment(lib, MSVCLIBSPATH "libpng.lib")
			#endif
		#endif
	#endif

#ifndef PNG_NORETURN
#define PNG_NORETURN
#endif
#ifndef PNG_ALLOCATED
#define PNG_ALLOCATED
#endif

#if PNG_LIBPNG_VER < 10500
	#define png_const_infop png_infop
	#define png_const_structp png_structp
	#define png_const_bytep png_bytep
	#define png_const_unknown_chunkp png_unknown_chunkp
#endif
#if PNG_LIBPNG_VER < 10600
	#define png_inforp png_infop
	#define png_const_inforp png_const_infop
	#define png_structrp png_structp
	#define png_const_structrp png_const_structp
#endif

void (PNGAPI *qpng_error) PNGARG((png_const_structrp png_ptr, png_const_charp error_message)) PSTATIC(png_error);
void (PNGAPI *qpng_read_end) PNGARG((png_structp png_ptr, png_infop info_ptr)) PSTATIC(png_read_end);
void (PNGAPI *qpng_read_image) PNGARG((png_structp png_ptr, png_bytepp image)) PSTATIC(png_read_image);
png_byte (PNGAPI *qpng_get_bit_depth) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_bit_depth);
png_byte (PNGAPI *qpng_get_channels) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_channels);
#if PNG_LIBPNG_VER < 10400
	png_uint_32 (PNGAPI *qpng_get_rowbytes) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_rowbytes);
#else
	png_size_t (PNGAPI *qpng_get_rowbytes) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_rowbytes);
#endif
void (PNGAPI *qpng_read_update_info) PNGARG((png_structp png_ptr, png_infop info_ptr)) PSTATIC(png_read_update_info);
void (PNGAPI *qpng_set_strip_16) PNGARG((png_structp png_ptr)) PSTATIC(png_set_strip_16);
void (PNGAPI *qpng_set_expand) PNGARG((png_structp png_ptr)) PSTATIC(png_set_expand);
void (PNGAPI *qpng_set_gray_to_rgb) PNGARG((png_structp png_ptr)) PSTATIC(png_set_gray_to_rgb);
void (PNGAPI *qpng_set_tRNS_to_alpha) PNGARG((png_structp png_ptr)) PSTATIC(png_set_tRNS_to_alpha);
png_uint_32 (PNGAPI *qpng_get_valid) PNGARG((png_const_structp png_ptr, png_const_infop info_ptr, png_uint_32 flag)) PSTATIC(png_get_valid);
#if PNG_LIBPNG_VER >= 10400
void (PNGAPI *qpng_set_expand_gray_1_2_4_to_8) PNGARG((png_structp png_ptr)) PSTATIC(png_set_expand_gray_1_2_4_to_8);
#else
void (PNGAPI *qpng_set_gray_1_2_4_to_8) PNGARG((png_structp png_ptr)) PSTATIC(png_set_gray_1_2_4_to_8);
#endif
void (PNGAPI *qpng_set_bgr) PNGARG((png_structp png_ptr)) PSTATIC(png_set_bgr);
void (PNGAPI *qpng_set_filler) PNGARG((png_structp png_ptr, png_uint_32 filler, int flags)) PSTATIC(png_set_filler);
void (PNGAPI *qpng_set_palette_to_rgb) PNGARG((png_structp png_ptr)) PSTATIC(png_set_palette_to_rgb);
png_uint_32 (PNGAPI *qpng_get_IHDR) PNGARG((png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 *width, png_uint_32 *height,
			int *bit_depth, int *color_type, int *interlace_method, int *compression_method, int *filter_method)) PSTATIC(png_get_IHDR);
void (PNGAPI *qpng_read_info) PNGARG((png_structp png_ptr, png_infop info_ptr)) PSTATIC(png_read_info);
void (PNGAPI *qpng_set_sig_bytes) PNGARG((png_structp png_ptr, int num_bytes)) PSTATIC(png_set_sig_bytes);
void (PNGAPI *qpng_set_read_fn) PNGARG((png_structp png_ptr, png_voidp io_ptr, png_rw_ptr read_data_fn)) PSTATIC(png_set_read_fn);
void (PNGAPI *qpng_destroy_read_struct) PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr)) PSTATIC(png_destroy_read_struct);
png_infop (PNGAPI *qpng_create_info_struct) PNGARG((png_const_structrp png_ptr)) PSTATIC(png_create_info_struct);
png_structp (PNGAPI *qpng_create_read_struct) PNGARG((png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn)) PSTATIC(png_create_read_struct);
int (PNGAPI *qpng_sig_cmp) PNGARG((png_const_bytep sig, png_size_t start, png_size_t num_to_check)) PSTATIC(png_sig_cmp);

void (PNGAPI *qpng_write_end) PNGARG((png_structrp png_ptr, png_inforp info_ptr)) PSTATIC(png_write_end);
void (PNGAPI *qpng_write_image) PNGARG((png_structrp png_ptr, png_bytepp image)) PSTATIC(png_write_image);
void (PNGAPI *qpng_write_info) PNGARG((png_structrp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_write_info);
void (PNGAPI *qpng_set_IHDR) PNGARG((png_const_structrp png_ptr, png_infop info_ptr, png_uint_32 width, png_uint_32 height,
			int bit_depth, int color_type, int interlace_method, int compression_method, int filter_method)) PSTATIC(png_set_IHDR);
void (PNGAPI *qpng_set_compression_level) PNGARG((png_structrp png_ptr, int level)) PSTATIC(png_set_compression_level);
void (PNGAPI *qpng_init_io) PNGARG((png_structp png_ptr, png_FILE_p fp)) PSTATIC(png_init_io);
png_voidp (PNGAPI *qpng_get_io_ptr) PNGARG((png_const_structrp png_ptr)) PSTATIC(png_get_io_ptr);
void (PNGAPI *qpng_destroy_write_struct) PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr)) PSTATIC(png_destroy_write_struct);
png_structp (PNGAPI *qpng_create_write_struct) PNGARG((png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn)) PSTATIC(png_create_write_struct);
void (PNGAPI *qpng_set_unknown_chunks) PNGARG((png_const_structrp png_ptr, png_inforp info_ptr, png_const_unknown_chunkp unknowns, int num_unknowns)) PSTATIC(png_set_unknown_chunks);

png_voidp (PNGAPI *qpng_get_error_ptr) PNGARG((png_const_structrp png_ptr)) PSTATIC(png_get_error_ptr);

pbool LibPNG_Init(void)
{
#ifdef DYNAMIC_LIBPNG
	static dllfunction_t pngfuncs[] =
	{
		{(void **) &qpng_error,							"png_error"},
		{(void **) &qpng_read_end,						"png_read_end"},
		{(void **) &qpng_read_image,					"png_read_image"},
		{(void **) &qpng_get_bit_depth,					"png_get_bit_depth"},
		{(void **) &qpng_get_channels,					"png_get_channels"},
		{(void **) &qpng_get_rowbytes,					"png_get_rowbytes"},
		{(void **) &qpng_read_update_info,				"png_read_update_info"},
		{(void **) &qpng_set_strip_16,					"png_set_strip_16"},
		{(void **) &qpng_set_expand,					"png_set_expand"},
		{(void **) &qpng_set_gray_to_rgb,				"png_set_gray_to_rgb"},
		{(void **) &qpng_set_tRNS_to_alpha,				"png_set_tRNS_to_alpha"},
		{(void **) &qpng_get_valid,						"png_get_valid"},
#if PNG_LIBPNG_VER > 10400
		{(void **) &qpng_set_expand_gray_1_2_4_to_8,	"png_set_expand_gray_1_2_4_to_8"},
#else
		{(void **) &qpng_set_gray_1_2_4_to_8,	"png_set_gray_1_2_4_to_8"},
#endif
		{(void **) &qpng_set_bgr,						"png_set_bgr"},
		{(void **) &qpng_set_filler,					"png_set_filler"},
		{(void **) &qpng_set_palette_to_rgb,			"png_set_palette_to_rgb"},
		{(void **) &qpng_get_IHDR,						"png_get_IHDR"},
		{(void **) &qpng_read_info,						"png_read_info"},
		{(void **) &qpng_set_sig_bytes,					"png_set_sig_bytes"},
		{(void **) &qpng_set_read_fn,					"png_set_read_fn"},
		{(void **) &qpng_destroy_read_struct,			"png_destroy_read_struct"},
		{(void **) &qpng_create_info_struct,			"png_create_info_struct"},
		{(void **) &qpng_create_read_struct,			"png_create_read_struct"},
		{(void **) &qpng_sig_cmp,						"png_sig_cmp"},

		{(void **) &qpng_write_end,						"png_write_end"},
		{(void **) &qpng_write_image,					"png_write_image"},
		{(void **) &qpng_write_info,					"png_write_info"},
		{(void **) &qpng_set_IHDR,						"png_set_IHDR"},
		{(void **) &qpng_set_compression_level,			"png_set_compression_level"},
		{(void **) &qpng_init_io,						"png_init_io"},
		{(void **) &qpng_get_io_ptr,					"png_get_io_ptr"},
		{(void **) &qpng_destroy_write_struct,			"png_destroy_write_struct"},
		{(void **) &qpng_create_write_struct,			"png_create_write_struct"},
		{(void **) &qpng_set_unknown_chunks,			"png_set_unknown_chunks"},

		{(void **) &qpng_get_error_ptr,					"png_get_error_ptr"},
		{NULL, NULL}
	};
	static qboolean tried;
	if (!tried)
	{
		tried = true;

		if (!LIBPNG_LOADED())
		{
			char *libnames[] =
			{
			#ifdef _WIN32
				va("libpng%i", PNG_LIBPNG_VER_DLLNUM);
			#else
				//linux...
				//lsb uses 'libpng12.so' specifically, so make sure that works.
				"libpng" STRINGIFY(PNG_LIBPNG_VER_MAJOR) STRINGIFY(PNG_LIBPNG_VER_MINOR) ".so." STRINGIFY(PNG_LIBPNG_VER_SONUM),
				"libpng" STRINGIFY(PNG_LIBPNG_VER_MAJOR) STRINGIFY(PNG_LIBPNG_VER_MINOR) ".so",
				"libpng.so." STRINGIFY(PNG_LIBPNG_VER_SONUM)
				"libpng.so",
			#endif
			};
			size_t i;
			for (i = 0; i < countof(libnames); i++)
			{
				libpng_handle = Sys_LoadLibrary(libnames[i], pngfuncs);
				if (libpng_handle)
					break;
			}
			if (!libpng_handle)
				Con_Printf("Unable to load %s\n", libnames[0]);
		}

//		if (!LIBPNG_LOADED())
//			libpng_handle = Sys_LoadLibrary("libpng", pngfuncs);
	}
#endif
	return LIBPNG_LOADED();
}

typedef struct {
	char *data;
	int readposition;
	int filelen;
} pngreadinfo_t;

static void VARGS readpngdata(png_structp png_ptr,png_bytep data,png_size_t len)
{
	pngreadinfo_t *ri = (pngreadinfo_t*)qpng_get_io_ptr(png_ptr);
	if (ri->readposition+len > ri->filelen)
	{
		qpng_error(png_ptr, "unexpected eof");
		return;
	}
	memcpy(data, &ri->data[ri->readposition], len);
	ri->readposition+=len;
}

struct pngerr
{
	const char *fname;
	jmp_buf jbuf;
};
static void VARGS png_onerror(png_structp png_ptr, png_const_charp error_msg)
{
	struct pngerr *err = qpng_get_error_ptr(png_ptr);
//	Con_Printf("libpng %s: %s\n", err->fname, error_msg);
	longjmp(err->jbuf, 1);
	abort();
}

static void VARGS png_onwarning(png_structp png_ptr, png_const_charp warning_msg)
{
	struct pngerr *err = qpng_get_error_ptr(png_ptr);
//	Con_DPrintf("libpng %s: %s\n", err->fname, warning_msg);
}

qbyte *ReadPNGFile(qbyte *buf, int length, int *width, int *height, const char *fname)
{
	qbyte header[8], **rowpointers = NULL, *data = NULL;
	png_structp png;
	png_infop pnginfo;
	int y, bitdepth, colortype, interlace, compression, filter, bytesperpixel;
	unsigned long rowbytes;
	pngreadinfo_t ri;
	png_uint_32 pngwidth, pngheight;
	struct pngerr errctx;

	if (!LibPNG_Init())
		return NULL;

	memcpy(header, buf, 8);

	errctx.fname = fname;
	if (setjmp(errctx.jbuf))
	{
error:
		if (data)
			free(data);
		if (rowpointers)
			free(rowpointers);
		qpng_destroy_read_struct(&png, &pnginfo, NULL);
		return NULL;
	}

	if (qpng_sig_cmp(header, 0, 8))
	{
		return NULL;
	}

	if (!(png = qpng_create_read_struct(PNG_LIBPNG_VER_STRING, &errctx, png_onerror, png_onwarning)))
	{
		return NULL;
	}

	if (!(pnginfo = qpng_create_info_struct(png)))
	{
		qpng_destroy_read_struct(&png, &pnginfo, NULL);
		return NULL;
	}

	ri.data=buf;
	ri.readposition=8;
	ri.filelen=length;
	qpng_set_read_fn(png, &ri, readpngdata);

	qpng_set_sig_bytes(png, 8);
	qpng_read_info(png, pnginfo);
	qpng_get_IHDR(png, pnginfo, &pngwidth, &pngheight, &bitdepth, &colortype, &interlace, &compression, &filter);

	*width = pngwidth;
	*height = pngheight;

	if (colortype == PNG_COLOR_TYPE_PALETTE)
	{
		qpng_set_palette_to_rgb(png);
		qpng_set_filler(png, 255, PNG_FILLER_AFTER);
	}

	if (colortype == PNG_COLOR_TYPE_GRAY && bitdepth < 8)
	{
		#if PNG_LIBPNG_VER > 10400
			qpng_set_expand_gray_1_2_4_to_8(png);
		#else
			qpng_set_gray_1_2_4_to_8(png);
		#endif
	}

	if (qpng_get_valid( png, pnginfo, PNG_INFO_tRNS))
		qpng_set_tRNS_to_alpha(png);

	if (bitdepth >= 8 && colortype == PNG_COLOR_TYPE_RGB)
		qpng_set_filler(png, 255, PNG_FILLER_AFTER);

	if (colortype == PNG_COLOR_TYPE_GRAY || colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		qpng_set_gray_to_rgb( png );
		qpng_set_filler(png, 255, PNG_FILLER_AFTER);
	}

	if (bitdepth < 8)
		qpng_set_expand (png);
	else if (bitdepth == 16)
		qpng_set_strip_16(png);


	qpng_read_update_info(png, pnginfo);
	rowbytes = qpng_get_rowbytes(png, pnginfo);
	bytesperpixel = qpng_get_channels(png, pnginfo);
	bitdepth = qpng_get_bit_depth(png, pnginfo);

	if (bitdepth != 8 || bytesperpixel != 4)
	{
//		Con_Printf ("Bad PNG color depth and/or bpp (%s)\n", fname);
		qpng_destroy_read_struct(&png, &pnginfo, NULL);
		return NULL;
	}

	data = malloc(*height * rowbytes);
	rowpointers = malloc(*height * sizeof(*rowpointers));

	if (!data || !rowpointers)
		goto error;

	for (y = 0; y < *height; y++)
		rowpointers[y] = data + y * rowbytes;

	qpng_read_image(png, rowpointers);
	qpng_read_end(png, NULL);

	qpng_destroy_read_struct(&png, &pnginfo, NULL);
	free(rowpointers);
	return data;
}
#endif

static void GUI_CreateInstaller_Windows(void)
{
#define RESLANG MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK)
	unsigned char *mandata = NULL;
	size_t manlen;
	unsigned char *pngdata = NULL;
	size_t pnglen;
	char *error = NULL;
	HANDLE bin;
	char ourname[MAX_PATH];
	char *basedir = enginebasedir;
	char newname[MAX_PATH];
	char modname[MAX_PATH+10] = "unknownmod";
	char tmpname[MAX_PATH];
	char tmp[MAX_PATH];

	if (MessageBox(mainwindow, "The 'Create Installer' option is still experimental.\nIt's probably still defective.\nSo be sure to test stuff extensively.", "Create Installer", MB_OKCANCEL|MB_DEFBUTTON2) != IDOK)
		return;

	PromptForEngine(0);


	{
		char workingdir[MAX_PATH];
		char absbase[MAX_PATH];
		char *slash;
		GetCurrentDirectory(sizeof(workingdir)-1, workingdir);
		_snprintf(modname, sizeof(modname), "%s/", enginebasedir);
		for (slash = modname; *slash; slash++)
			if (*slash == '/')
				*slash = '\\';
		PathCombine(absbase, workingdir, modname);
		if (PathRelativePathToA(modname, absbase, FILE_ATTRIBUTE_DIRECTORY, workingdir, FILE_ATTRIBUTE_DIRECTORY))
		{
			if (!strncmp(modname, ".\\", 2))
				memmove(modname, modname+2, strlen(modname+2)+1);
			slash = strchr(modname, '/');
			if (slash)
				*slash = 0;
			slash = strchr(modname, '\\');
			if (slash)
				*slash = 0;
			if (!*modname)
				_snprintf(modname, sizeof(modname), "unknownmod");
		}
	}

	QC_snprintfz(tmp, sizeof(tmp), "default.fmf");
	mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "%s.fmf", modname);
		mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "../default.fmf");
		mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "../%s.fmf", modname);
		mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	if (!mandata)
	{
		QC_snprintfz(tmp, sizeof(tmp), "%s.fmf", modname);
		if (!PromptForFile("Please Select Manifest File", "FTE Manifests\0*.fmf\0All files\0*.*\0", ".", tmp, tmp, sizeof(tmp), true))
			mandata = QCC_ReadFile (tmp, NULL, 0, &manlen);
	}
	
	if (!mandata)
	{
		FILE *f;
		if (MessageBox(mainwindow, "Creating an installer requires a manifest.\nCreate+edit one now?", "Create Installer", MB_OKCANCEL) != IDOK)
			return;

		f = fopen(tmp, "wb");
		fprintf(f, "FTEManifestVer 1\n");
		fprintf(f, "///basic information\n");
		fprintf(f, "game \"quake\" ///change this to isolate your mod from quake. if the game is known to the engine itself then other settings will receive default values unless overriden. Should be safe to use as a filename, so should have no spaces or full stops.\n");
		fprintf(f, "name \"Quake\" ///this is the full name of your game that you wish the engine to display. Use spaces.\n");
		fprintf(f, "//protocolname \"FTE-Quake\" ///allows isolation from other games using the same engine. Should only be changed for standalone total conversions.\n");
		fprintf(f, "///filesystem\n");
		fprintf(f, "//basegame \"id1\"\n");
		fprintf(f, "//basegame \"qw\"\n");
		fprintf(f, "//basegame \"*fte\" ///* prefix means its never networked\n");
		fprintf(f, "gamedir \"%s\"\n", modname);
		fprintf(f, "//disablehomedir 0\n");
		fprintf(f, "///required packages. add more as needed. these will be downloaded+installed from the get-go\n");
		fprintf(f, "///the engine will tell you the correct crc if you get it wrong/don't know it. Its not mandatory, but allows for autoupdates if the fmf changes.\n");
		fprintf(f, "//package id1/example.pk3\tmirror \"https://example.com/example.pak\"\t//crc 0xdeadbeef\n");
		fprintf(f, "///updateurl points to an (updated) copy of this manifest file, so you can update basic stuff easily. should always be https\n");
		fprintf(f, "//updateurl \"https://example.com/example.fmf\"\n");
		fprintf(f, "///downloadsurl is a list of optional updates, including engine updates, displayed via the updates menu. should always be https\n");
		fprintf(f, "//downloadsurl \"https://fte.triptohell.info/downloadables.php\"\n");
		fprintf(f, "///eula displayed when first installing\n");
		fprintf(f, "//eula \"By using this game software, you assign your eternal soul to me for me to do as I wish, including but not limited to trading it for a pint of beer. This example is not legally binding.\"\n");
		fclose(f);
		EditFile(tmp, -1, false);
		return;
	}
	else
	{


		

		QC_snprintfz(newname, sizeof(newname), "%s_setup.exe", modname);

		if (!PromptForFile("Please Select Output Executable", "Executables\0*.exe\0All files\0*.*\0", basedir, newname, tmpname, sizeof(tmpname), true))
			return;

		PathCombine(newname, basedir, tmpname);
		PathCombine(tmpname, basedir, "tmp.exe");
		PathCombine(ourname, enginebasedir, enginebinary);

		if (!CopyFile(ourname, tmpname, FALSE))
			error = "output already exists or cannot be written";

		if (!(bin = BeginUpdateResource(tmpname, FALSE)))
			error = "BeginUpdateResource failed";
		else
		{
			QC_snprintfz(tmp, sizeof(tmp), "%s.ico", modname);
			pngdata = QCC_ReadFile (tmp, NULL, 0, &pnglen);
#ifdef AVAIL_PNGLIB
			if (!pngdata)
			{
				QC_snprintfz(tmp, sizeof(tmp), "%s.png", modname);
				pngdata = QCC_ReadFile (tmp, NULL, 0, &pnglen);
			}
			if (!pngdata)
				pngdata = QCC_ReadFile ("default.png", NULL, 0, &pnglen);
#endif

			if (!pngdata)
				if (PromptForFile("Please Select Icon", "Icons\0*.ico"
#ifdef AVAIL_PNGLIB
					";*.png"
#endif
					"\0All files\0*.*\0", ".", tmp, tmp, sizeof(tmp), false))
					pngdata = QCC_ReadFile (tmp, NULL, 0, &pnglen);

			if (pngdata && pngdata[0] == 0 && pngdata[1] == 0 && pngdata[2] == 1 && pngdata[3] == 0)
			{
				unsigned int iconid = 1, img;
				unsigned short images;
				struct {
					BYTE  bWidth;
					BYTE  bHeight;
					BYTE  bColorCount;
					BYTE  bReserved;
					WORD  wPlanes;
					WORD  wBitCount;
					DWORD dwBytesInRes;
					DWORD dwOffset;
				} *iconinfo;
				icon_group_t icondata;
				memset(&icondata, 0, sizeof(icondata));
				icondata.idType = 1;

				images = pngdata[4] | (pngdata[5]<<8);

				UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(1), RESLANG, NULL, 0);
				UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(2), RESLANG, NULL, 0);

				for (iconinfo = (void*)(pngdata+6), img = 0; img < images; iconinfo++, img++)
				{
					if (!error && !UpdateResource(bin, RT_ICON, MAKEINTRESOURCE(iconid), 0, pngdata+iconinfo->dwOffset, iconinfo->dwBytesInRes))
						error = "UpdateResource failed (icon data)";

					//and make a copy of it in the icon list
					icondata.idEntries[icondata.idCount].bWidth = iconinfo->bWidth;
					icondata.idEntries[icondata.idCount].bHeight = iconinfo->bHeight;
					icondata.idEntries[icondata.idCount].wBitCount = iconinfo->wBitCount;
					icondata.idEntries[icondata.idCount].wPlanes = iconinfo->wPlanes;
					icondata.idEntries[icondata.idCount].bColorCount = iconinfo->bColorCount;
					icondata.idEntries[icondata.idCount].dwBytesInRes = iconinfo->dwBytesInRes;
					icondata.idEntries[icondata.idCount].nId = iconid++;
					icondata.idCount++;
				}

				if (!error && !UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(1), RESLANG, &icondata, (pbyte*)&icondata.idEntries[icondata.idCount] - (pbyte*)&icondata))
					error = "UpdateResource failed (icon group)";
			}
#ifdef AVAIL_PNGLIB
			else if (pngdata)
			{
				icon_group_t icondata;
				qbyte *rgbadata;
				int imgwidth, imgheight;
				int iconid = 1;
				memset(&icondata, 0, sizeof(icondata));
				icondata.idType = 1;

				if (MessageBox(mainwindow, error, "Embedding PNGs is probably buggy/suboptimal. You should consider using an .ico instead.\nContinue anyway?", MB_OKCANCEL) != IDOK)
					error = "User aborted";

				UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(1), RESLANG, NULL, 0);
				UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(2), RESLANG, NULL, 0);
	//			UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(3), RESLANG, NULL, 0);

				rgbadata = ReadPNGFile(pngdata, pnglen, &imgwidth, &imgheight, "default.png");
				if (!rgbadata)
					error = "unable to read icon image";
				else
				{
					void *data = NULL;
					unsigned int datalen = 0;
					unsigned int i;

//					extern cvar_t gl_lerpimages;
//					gl_lerpimages.ival = 1;
					for (i = 0; i < sizeof(icosizes)/sizeof(icosizes[0]); i++)
					{
						unsigned int x,y;
						unsigned int pixels;
						if (icosizes[i].width > imgwidth || icosizes[i].height > imgheight)
							continue;	//ignore icons if they're bigger than the original icon.

						if (icosizes[i].bpp == 32 && icosizes[i].width >= 128 && icosizes[i].height >= 128 && icosizes[i].width == imgwidth && icosizes[i].height == imgheight)
						{	//png compression. oh look. we originally loaded a png!
							data = pngdata;
							datalen = pnglen;
						}
						else
						{
							//generate the bitmap info
							BITMAPV4HEADER *bi;
							qbyte *out, *outmask;
							qbyte *in, *inrow;
							unsigned int outidx;

							pixels = icosizes[i].width * icosizes[i].height;

							bi = data = malloc(sizeof(*bi) + icosizes[i].width * icosizes[i].height * 5 + icosizes[i].height*4);
							memset(bi,0, sizeof(BITMAPINFOHEADER));
							bi->bV4Size				= sizeof(BITMAPINFOHEADER);
							bi->bV4Width			= icosizes[i].width;
							bi->bV4Height			= icosizes[i].height * 2;	//icons are logically double-height, with the second half being a silly alpha mask.
							bi->bV4Planes			= 1;
							bi->bV4BitCount			= icosizes[i].bpp;
							bi->bV4V4Compression	= BI_RGB;
							bi->bV4ClrUsed			= (icosizes[i].bpp>=32?0:(1u<<icosizes[i].bpp));

							datalen = bi->bV4Size;
							out = (qbyte*)data + datalen;
							datalen += ((icosizes[i].width*icosizes[i].bpp/8+3)&~3) * icosizes[i].height;
							outmask = (qbyte*)data + datalen;
							datalen += ((icosizes[i].width+31)&~31)/8 * icosizes[i].height;

							in = malloc(pixels*4);
							Image_ResampleTexture((unsigned int*)rgbadata, imgwidth, imgheight, (unsigned int*)in, icosizes[i].width, icosizes[i].height);

							inrow = in;
							outidx = 0;
							if (icosizes[i].bpp == 32)
							{
								for (y = 0; y < icosizes[i].height; y++)
								{
									inrow = in + 4*icosizes[i].width*(icosizes[i].height-1-y);
									for (x = 0; x < icosizes[i].width; x++)
									{
										if (inrow[3] == 0)	//transparent
											outmask[outidx>>3] |= 1u<<(outidx&7);
										else
										{
											out[0] = inrow[2];
											out[1] = inrow[1];
											out[2] = inrow[0];
										}
										out += 4;
										outidx++;
										inrow += 4;
									}
									if (x & 3)
										out += 4 - (x&3);
									outidx = (outidx + 31)&~31;
								}
							}
						}

						if (!error && !UpdateResource(bin, RT_ICON, MAKEINTRESOURCE(iconid), 0, data, datalen))
							error = "UpdateResource failed (icon data)";

						//and make a copy of it in the icon list
						icondata.idEntries[icondata.idCount].bWidth = (icosizes[i].width<256)?icosizes[i].width:0;
						icondata.idEntries[icondata.idCount].bHeight = (icosizes[i].height<256)?icosizes[i].height:0;
						icondata.idEntries[icondata.idCount].wBitCount = icosizes[i].bpp;
						icondata.idEntries[icondata.idCount].wPlanes = 1;
						icondata.idEntries[icondata.idCount].bColorCount = (icosizes[i].bpp>=8)?0:(1u<<icosizes[i].bpp);
						icondata.idEntries[icondata.idCount].dwBytesInRes = datalen;
						icondata.idEntries[icondata.idCount].nId = iconid++;
						icondata.idCount++;
					}
				}

				if (!error && !UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(1), RESLANG, &icondata, (qbyte*)&icondata.idEntries[icondata.idCount] - (qbyte*)&icondata))
					error = "UpdateResource failed (icon group)";
			}
#endif
			else
				error = "icon format not supported";


			if (mandata)
			{
				if (!error && !UpdateResource(bin, RT_RCDATA, MAKEINTRESOURCE(1), 0, mandata, manlen+1))
					error = "UpdateResource failed (manicfest)";
			}
			else
				error = "fmf not found in working directory";

			if (!EndUpdateResource(bin, !!error) && !error)
				error = "EndUpdateResource failed. Check access permissions.";

			DeleteFile(newname);
			MoveFile(tmpname, newname);
		}
	}

	free(pngdata);
	free(mandata);

	if (error)
		MessageBox(mainwindow, error, "Create Installer Error", MB_ICONERROR);
	else
		MessageBox(mainwindow, "Installer Created!\nNow go and upload it somewhere!\n\nBe sure to flush windows' icon cache if you changed the icon.", "Create Installer", MB_OK);
}

HWND targitem_hexen2;
HWND targitem_fte;
HWND nokeywords_coexistitem;
HWND autoprototype_item;
//HWND autohighlight_item;
HWND extraparmsitem;
#ifdef EMBEDDEBUG
HWND w_enginebinary;
HWND w_enginebasedir;
HWND w_enginecommandline;
#endif
static LRESULT CALLBACK OptionsWndProc(HWND hWnd,UINT message,
				     WPARAM wParam,LPARAM lParam)
{
	int i;
	switch (message)
	{
	case WM_DESTROY:
		optionsmenu = NULL;
		break;

	case WM_COMMAND:
		switch(wParam)
		{
		case IDI_O_APPLYSAVE:
		case IDI_O_APPLY:
			for (i = 0; optimisations[i].enabled; i++)
			{
				if (optimisations[i].flags & FLAG_HIDDENINGUI)
					continue;

				if (Button_GetCheck(optimisations[i].guiinfo))
					optimisations[i].flags |= FLAG_SETINGUI;
				else
					optimisations[i].flags &= ~FLAG_SETINGUI;
			}
			fl_hexen2 = Button_GetCheck(targitem_hexen2);
			fl_ftetarg = Button_GetCheck(targitem_fte);
			for (i = 0; compiler_flag[i].enabled; i++)
			{
				if (compiler_flag[i].flags & FLAG_HIDDENINGUI)
					continue;
				if (Button_GetCheck(compiler_flag[i].guiinfo))
					compiler_flag[i].flags |= FLAG_SETINGUI;
				else
					compiler_flag[i].flags &= ~FLAG_SETINGUI;
			}
			Edit_GetText(extraparmsitem, parameters, sizeof(parameters)-1);
#ifdef EMBEDDEBUG
			Edit_GetText(w_enginebinary, enginebinary, sizeof(enginebinary)-1);
			Edit_GetText(w_enginebasedir, enginebasedir, sizeof(enginebasedir)-1);
			Edit_GetText(w_enginecommandline, enginecommandline, sizeof(enginecommandline)-1);
#endif

			if (wParam == IDI_O_APPLYSAVE)
				GUI_SaveConfig();
			DestroyWindow(hWnd);
			break;
/*		case IDI_O_CHANGE_PROGS_SRC:
			{
				char filename[MAX_PATH];
				char oldpath[MAX_PATH+10];
				OPENFILENAME ofn;
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hInstance = ghInstance;
				ofn.lpstrFile = filename;
				ofn.lpstrTitle = "Please find progs.src";
				ofn.nMaxFile = sizeof(filename)-1;
				ofn.lpstrFilter = "QuakeC source\0*.src\0All files\0*.*\0";
				memset(filename, 0, sizeof(filename));
				GetCurrentDirectory(sizeof(oldpath)-1, oldpath);
				ofn.lpstrInitialDir = oldpath;
				if (GetOpenFileName(&ofn))
				{
					SetProgsSrcFileAndPath(filename);
					resetprogssrc = true;
				}
			}
			break;*/
		case IDI_O_LEVEL0:
		case IDI_O_LEVEL1:
		case IDI_O_LEVEL2:
		case IDI_O_LEVEL3:
			for (i = 0; optimisations[i].enabled; i++)
			{
				if (optimisations[i].flags & FLAG_HIDDENINGUI)
					continue;

				if (optimisations[i].optimisationlevel<=(int)wParam-IDI_O_LEVEL0)
					Button_SetCheck(optimisations[i].guiinfo, 1);
				else
					Button_SetCheck(optimisations[i].guiinfo, 0);
			}
			if (!fl_nondfltopts)
			{
				for (i = 0; optimisations[i].enabled; i++)
				{
					if (optimisations[i].guiinfo)
						EnableWindow(optimisations[i].guiinfo, TRUE);
				}
				fl_nondfltopts = true;
			}
			break;
		case IDI_O_DEBUG:
			for (i = 0; optimisations[i].enabled; i++)
			{
				if (optimisations[i].flags & FLAG_HIDDENINGUI)
					continue;

				if (optimisations[i].flags&FLAG_KILLSDEBUGGERS)
					Button_SetCheck(optimisations[i].guiinfo, 0);
			}
			if (!fl_nondfltopts)
			{
				for (i = 0; optimisations[i].enabled; i++)
				{
					if (optimisations[i].guiinfo)
						EnableWindow(optimisations[i].guiinfo, TRUE);
				}
				fl_nondfltopts = true;
			}
			break;
		case IDI_O_DEFAULT:
			for (i = 0; optimisations[i].enabled; i++)
			{
				if (optimisations[i].flags & FLAG_HIDDENINGUI)
					continue;

				if (optimisations[i].flags & FLAG_ASDEFAULT)
					Button_SetCheck(optimisations[i].guiinfo, 1);
				else
					Button_SetCheck(optimisations[i].guiinfo, 0);
			}
			if (fl_nondfltopts)
			{
				for (i = 0; optimisations[i].enabled; i++)
				{
					if (optimisations[i].guiinfo)
						EnableWindow(optimisations[i].guiinfo, FALSE);
				}
				fl_nondfltopts = false;
			}
			break;
		}
		break;
	case WM_HELP:
		{
			HELPINFO *hi;
			hi = (HELPINFO *)lParam;
			switch(hi->iCtrlId) 
			{
			case IDI_O_DEFAULT:
				MessageBox(hWnd, "Sets the default optimisations", "Help", MB_OK|MB_ICONINFORMATION);
				break;
			case IDI_O_DEBUG:
				MessageBox(hWnd, "Clears all optimisations which can make your progs harder to debug", "Help", MB_OK|MB_ICONINFORMATION);
				break;
			case IDI_O_LEVEL0:
			case IDI_O_LEVEL1:
			case IDI_O_LEVEL2:
			case IDI_O_LEVEL3:
				MessageBox(hWnd, "Sets a specific optimisation level", "Help", MB_OK|MB_ICONINFORMATION);
				break;
//			case IDI_O_CHANGE_PROGS_SRC:
//				MessageBox(hWnd, "Use this button to change your root source file.\nNote that fteqcc compiles sourcefiles from editors first, rather than saving. This means that changes are saved ONLY when you save them, but means that switching project mid-compile can result in problems.", "Help", MB_OK|MB_ICONINFORMATION);
//				break;
			case IDI_O_ADDITIONALPARAMETERS:
				MessageBox(hWnd, "Type in additional commandline parameters here. Use -Dname to define a named precompiler constant before compiling.", "Help", MB_OK|MB_ICONINFORMATION);
				break;
			case IDI_O_APPLY:
				MessageBox(hWnd, "Apply changes shown.", "Help", MB_OK|MB_ICONINFORMATION);
				break;
			case IDI_O_APPLYSAVE:
				MessageBox(hWnd, "Apply changes shown and save the settings for next time.", "Help", MB_OK|MB_ICONINFORMATION);
				break;
			case IDI_O_OPTIMISATION:
				for (i = 0; optimisations[i].enabled; i++)
				{
					if (optimisations[i].guiinfo == hi->hItemHandle)
					{
						MessageBox(hWnd, optimisations[i].description, "Help", MB_OK|MB_ICONINFORMATION);
						break;
					}
				}
				break;
			case IDI_O_COMPILER_FLAG:
				for (i = 0; compiler_flag[i].enabled; i++)
				{
					if (compiler_flag[i].guiinfo == hi->hItemHandle)
					{
						MessageBox(hWnd, compiler_flag[i].description, "Help", MB_OK|MB_ICONINFORMATION);
						break;
					}
				}
				break;
			case IDI_O_TARGETH2:
				MessageBox(hWnd, "Click here to compile a hexen2 compatible progs, as well as enable all hexen2 keywords. Note that this uses the -Thexen2. There are other targets available.", "Help", MB_OK|MB_ICONINFORMATION);
				break;
			case IDI_O_TARGETFTE:
				MessageBox(hWnd, "Click here to allow the use of extended instructions not found in the original instruction set.", "Help", MB_OK|MB_ICONINFORMATION);
				break;
			}
		}
		break;
	default:
		return DefWindowProc(hWnd,message,wParam,lParam);
	}
	return 0;
}
static void AddTip(HWND tipwnd, HWND tool, char *message)
{
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = tool;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)tool;
	toolInfo.lpszText = message;
	SendMessage(tipwnd, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}
void OptionsDialog(void)
{
	char nicername[256], *us;
	HWND subsection;
	RECT r;
	WNDCLASS wndclass;
	HWND wnd, tipwnd;
	int i;
	int flagcolums=1;

	int x;
	int y;
	int my;
	int lheight;
	int rheight;
	int num;
	int cflagsshown;

	if (optionsmenu)
	{
		BringWindowToTop(optionsmenu);
		return;
	}


	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.style      = 0;
	wndclass.lpfnWndProc   = OptionsWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = ghInstance;
	wndclass.hIcon         = LoadIcon(ghInstance, IDI_ICON_FTEQCC);
	wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wndclass.hbrBackground = (void *)COLOR_WINDOW;
	wndclass.lpszMenuName  = 0;
	wndclass.lpszClassName = OPTIONS_WINDOW_CLASS_NAME;
	RegisterClass(&wndclass);

	lheight = 0;
	for (i = 0; optimisations[i].enabled; i++)
	{
		if (optimisations[i].flags & FLAG_HIDDENINGUI)
			continue;

		lheight++;
	}
	lheight = (lheight+1)/2;	//double columns for optimisations
	lheight *= 16;
	lheight += 112;
	lheight += 88;

	cflagsshown = 0;
	cflagsshown += 2; //hexenc, extended opcodes
	for (i = 0; compiler_flag[i].enabled; i++)
	{
		if (compiler_flag[i].flags & FLAG_HIDDENINGUI)
			continue;

		cflagsshown++;
	}

	do
	{
		flagcolums++;
		cflagsshown += flagcolums-1;	//round up
		rheight = (cflagsshown/flagcolums)*16;

		rheight += 16+4+20;	//extra parms cap,gap,parmsbox(min)
	}while (rheight > lheight*flagcolums);

	r.right = 408 + flagcolums*168;
	if (r.right < 640)
		r.right = 640;

	r.left = GetSystemMetrics(SM_CXSCREEN)/2-320;
	r.top = GetSystemMetrics(SM_CYSCREEN)/2-240;
	if (rheight > lheight)
		r.bottom = r.top + rheight;
	else
	{
		r.bottom = r.top + lheight;
		rheight = lheight;
	}
	r.right  += r.left;



	AdjustWindowRectEx (&r, WS_CAPTION|WS_SYSMENU, FALSE, 0);

	optionsmenu=CreateWindowEx(WS_EX_CONTEXTHELP, OPTIONS_WINDOW_CLASS_NAME, "Options - FTE QuakeC compiler", WS_CAPTION|WS_SYSMENU,
		r.left, r.top, r.right-r.left, r.bottom-r.top, NULL, NULL, ghInstance, NULL);

	tipwnd = CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, optionsmenu, NULL, ghInstance, NULL);
	SetWindowPos(tipwnd, HWND_TOPMOST,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SendMessage(tipwnd, TTM_SETMAXTIPWIDTH, 0, 500);

	subsection = CreateWindow("BUTTON", "Optimisations", WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
		0, 0, 400, lheight-40*4+24, optionsmenu, NULL, ghInstance, NULL);

	num = 0;
	for (i = 0; optimisations[i].enabled; i++)
	{
		if (optimisations[i].flags & FLAG_HIDDENINGUI)
		{
			optimisations[i].guiinfo = NULL;
			continue;
		}

		QC_strlcpy(nicername, optimisations[i].fullname, sizeof(nicername));
		while((us = strchr(nicername, '_')))
			*us = ' ';

		optimisations[i].guiinfo = wnd = CreateWindow("BUTTON",nicername,
			   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			   8+200*(num&1),16+16*(num/2),200-16,16,
			   subsection,
			   (HMENU)IDI_O_OPTIMISATION,
			   ghInstance,
			   NULL);

		if (optimisations[i].flags&FLAG_SETINGUI)
			Button_SetCheck(wnd, 1);
		else
			Button_SetCheck(wnd, 0);

		if (!fl_nondfltopts)
			EnableWindow(wnd, FALSE);

		AddTip(tipwnd, wnd,	optimisations[i].description);

		num++;
	}

	wnd = CreateWindow("BUTTON","O0",
		   WS_CHILD | WS_VISIBLE,
		   8,lheight-40*5+24,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_LEVEL0,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,	"Disable optimisations completely, giving code more similar to vanilla.");
	wnd = CreateWindow("BUTTON","O1",
		   WS_CHILD | WS_VISIBLE,
		   8+64,lheight-40*5+24,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_LEVEL1,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,	"Enable simple optimisations (primarily size). Probably still breaks decompilers.");
	wnd = CreateWindow("BUTTON","O2",
		   WS_CHILD | WS_VISIBLE,
		   8+64*2,lheight-40*5+24,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_LEVEL2,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,	"Enable most optimisations. Does not optimise anything that is likely to break any engines.");
	wnd = CreateWindow("BUTTON","O3",
		   WS_CHILD | WS_VISIBLE,
		   8+64*3,lheight-40*5+24,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_LEVEL3,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,	"Enable unsafe optimisations. The extra optimisations may cause the progs to fail in certain cases, especially if used to compile addon modules.");
	wnd = CreateWindow("BUTTON","Debug",
		   WS_CHILD | WS_VISIBLE,
		   8+64*4,lheight-40*5+24,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_DEBUG,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,	"Disable any optimisations that might interfere with debugging somehow.");
	wnd = CreateWindow("BUTTON","Default",
		   WS_CHILD | WS_VISIBLE,
		   8+64*5,lheight-40*5+24,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_DEFAULT,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,	"Default optimsations are aimed at increasing capacity without breaking debuggers or common decompilers (although gotos, switches, arrays, etc, will still result in issues).");

#ifdef EMBEDDEBUG
	w_enginebinary = CreateWindowEx(WS_EX_CLIENTEDGE,
		"EDIT",
		enginebinary,
		WS_CHILD /*| ES_READONLY*/ | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
		8, lheight-40-30*3, 400-16, 22,
		optionsmenu,
		(HMENU)IDI_O_ENGINE,
		ghInstance,
		NULL);
	w_enginebasedir = CreateWindowEx(WS_EX_CLIENTEDGE,
		"EDIT",
		enginebasedir,
		WS_CHILD /*| ES_READONLY*/ | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
		8, lheight-40-30*2, 400-16, 22,
		optionsmenu,
		(HMENU)IDI_O_ENGINEBASEDIR,
		ghInstance,
		NULL);
	w_enginecommandline = CreateWindowEx(WS_EX_CLIENTEDGE,
		"EDIT",
		enginecommandline,
		WS_CHILD /*| ES_READONLY*/ | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
		8, lheight-40-30, 400-16, 22,
		optionsmenu,
		(HMENU)IDI_O_ENGINECOMMANDLINE,
		ghInstance,
		NULL);

	AddTip(tipwnd, w_enginebinary,		"This is the engine that you wish to debug with.\nCurrently only FTEQW supports actual debugging, while specifying other engines here merely provides you with a quick way to start them up");
	AddTip(tipwnd, w_enginebasedir,		"This is your base directory (typically the directory your engine executable is in)");
	AddTip(tipwnd, w_enginecommandline,	"This is the commandline to use to invoke your mod.\nYou'll likely want -game here.\n-window is also handy.\n-nohome can be used to inhibit the use of home directories.\nYou may also want to add '+map start' or some such.");
#endif

	wnd = CreateWindow("BUTTON","Apply",
		   WS_CHILD | WS_VISIBLE,
		   8,lheight-40,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_APPLY,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,		"Use selected settings without saving them to disk.");
	wnd = CreateWindow("BUTTON","Save",
		   WS_CHILD | WS_VISIBLE,
		   8+64,lheight-40,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_APPLYSAVE,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,		"Use selected settings and save them to disk so that they're also used the next time you start fteqccgui.");
	/*wnd = CreateWindow("BUTTON","progs.src",
		   WS_CHILD | WS_VISIBLE,
		   8+64*2,lheight-40,64,32,
		   optionsmenu,
		   (HMENU)IDI_O_CHANGE_PROGS_SRC,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, wnd,		"Change the initial src file.");*/



		y=4;
	targitem_hexen2 = wnd = CreateWindow("BUTTON","HexenC",
		   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		   408,y,200-16,16,
		   optionsmenu,
		   (HMENU)IDI_O_TARGETH2,
		   ghInstance,
		   NULL);
	y+=16;
	if (fl_hexen2)
		Button_SetCheck(wnd, 1);
	else
		Button_SetCheck(wnd, 0);
	AddTip(tipwnd, wnd,	"Compile for hexen2.\nThis changes the opcodes slightly, the progs crc, and enables some additional keywords.");

	targitem_fte = wnd = CreateWindow("BUTTON","Extended Instructions",
		   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		   408,y,200-16,16,
		   optionsmenu,
		   (HMENU)IDI_O_TARGETFTE,
		   ghInstance,
		   NULL);
	y+=16;
	if (fl_ftetarg)
		Button_SetCheck(wnd, 1);
	else
		Button_SetCheck(wnd, 0);
	AddTip(tipwnd, wnd,	"Enables the use of additional opcodes, which only FTE supports at this time.\nThis gives both smaller and faster code, as well as allowing pointers, ints, and other extensions not possible with the vanilla QCVM.");

/*	autohighlight_item = wnd = CreateWindow("BUTTON","Syntax Highlighting",
		   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		   408,y,200-16,16,
		   optionsmenu,
		   (HMENU)IDI_O_SYNTAX_HIGHLIGHTING,
		   ghInstance,
		   NULL);
	y+=16;
	if (fl_autohighlight)
		Button_SetCheck(wnd, 1);
	else
		Button_SetCheck(wnd, 0);
*/
	x = 408;
	my = y;
	for (i = 0; compiler_flag[i].enabled; i++)
	{
		if (compiler_flag[i].flags & FLAG_HIDDENINGUI)
		{
			compiler_flag[i].guiinfo = NULL;
			continue;
		}

		if (y > (cflagsshown/flagcolums)*16)
		{
			y = 4;
			x += 168;
		}

		compiler_flag[i].guiinfo = wnd = CreateWindow("BUTTON",compiler_flag[i].fullname,
			   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			   x,y,168,16,
			   optionsmenu,
			   (HMENU)IDI_O_COMPILER_FLAG,
			   ghInstance,
			   NULL);
		y+=16;

		if (my < y)
			my = y;

		if (compiler_flag[i].flags & FLAG_SETINGUI)
			Button_SetCheck(wnd, 1);
		else
			Button_SetCheck(wnd, 0);
		AddTip(tipwnd, wnd,	compiler_flag[i].description);
	}

	CreateWindow("STATIC","Extra Parameters:",
		   WS_CHILD | WS_VISIBLE,
		   408,my,200-16,16,
		   optionsmenu,
		   (HMENU)0,
		   ghInstance,
		   NULL);
	my+=16;
	extraparmsitem = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT",parameters,
		   WS_CHILD | WS_VISIBLE|ES_LEFT | ES_WANTRETURN |
		ES_MULTILINE | ES_AUTOVSCROLL,
		   408,my,r.right-r.left - 408 - 8,rheight-my-4,
		   optionsmenu,
		   (HMENU)IDI_O_ADDITIONALPARAMETERS,
		   ghInstance,
		   NULL);
	AddTip(tipwnd, extraparmsitem,	"You can specify any additional commandline arguments here.\nAdd -DFOO=bar to define the FOO preprocessor constant as bar.");

	ShowWindow(optionsmenu, SW_SHOWDEFAULT);
}











#undef printf

WNDPROC combosubclassproc;
static LRESULT CALLBACK SearchComboSubClass(HWND hWnd,UINT message,
				     WPARAM wParam,LPARAM lParam)
{
	switch (message) 
	{ 
	case WM_KEYDOWN: 
		switch (wParam) 
		{
		case VK_RETURN:
			PostMessage(mainwindow, WM_COMMAND, ID_DEF, (LPARAM)buttons[ID_DEF].hwnd);
			return true;
		}
	}
	return CallWindowProc(combosubclassproc, hWnd, message, wParam, lParam); 
}

static LRESULT CALLBACK MainWndProc(HWND hWnd,UINT message,
				     WPARAM wParam,LPARAM lParam)
{
	int i;
	RECT rect;
	PAINTSTRUCT ps;
	editor_t *editor;
	switch (message)
	{
	case WM_CLOSE:
		//if any child editors are still open, send close requests to them first.
		//this allows them to display prompts, instead of silently losing changes.
		for (editor = editors; editor;)
		{
			editor_t *n = editor->next;
			if (editor->window)
				SendMessage(editor->window, WM_CLOSE, 0, 0);
			editor = n;
		}
		//okay, they're all dead. we can kill ourselves now.
		if (!editors)
			DestroyWindow(hWnd);
		return 0;
	case WM_CREATE:
		{
			CLIENTCREATESTRUCT ccs;

			HMENU rootmenu, windowmenu, m;

			DragAcceptFiles(hWnd, TRUE);

			rootmenu = CreateMenu();
			
				AppendMenu(rootmenu, MF_POPUP, (UINT_PTR)(m = CreateMenu()),	"&File");
					AppendMenu(m, 0, IDM_OPENPROJECT,							"Open Project / Decompile");
					AppendMenu(m, 0, IDM_OPENNEW,								"Open File");
					AppendMenu(m, 0, IDM_SAVE,									"&Save\tCtrl+S");
				//	AppendMenu(m, 0, IDM_FIND,									"&Find");
					AppendMenu(m, 0, IDM_UNDO,									"Undo\tCtrl+Z");
					AppendMenu(m, 0, IDM_REDO,									"Redo\tCtrl+Y");
					AppendMenu(m, MF_SEPARATOR, 0, NULL);
					AppendMenu(m, 0, IDM_CREATEINSTALLER_WINDOWS,				"Create Windows Installer");
					AppendMenu(m, 0, IDM_CREATEINSTALLER_ANDROID,				"Create Android Installer");
					AppendMenu(m, 0, IDM_CREATEINSTALLER_PACKAGES,				"Create Packages");
					AppendMenu(m, MF_SEPARATOR, 0, NULL);
					AppendMenu(m, 0, IDM_QUIT,									"Exit");
				AppendMenu(rootmenu, MF_POPUP, (UINT_PTR)(m = CreateMenu()),	"&Navigation");
					AppendMenu(m, 0, IDM_GOTODEF,								"Go To Definition\tF12");
					AppendMenu(m, 0, IDM_RETURNDEF,								"Return From Definition\tShift+F12");
					AppendMenu(m, 0, IDM_GREP,									"Grep For Selection\tCtrl+G");
					AppendMenu(m, 0, IDM_OPENDOCU,								"Open Selected File");
					AppendMenu(m, 0, IDM_OUTPUT_WINDOW,							"Show Output Window\tF6");
					AppendMenu(m, (fl_extramargins?MF_CHECKED:MF_UNCHECKED), IDM_UI_SHOWLINENUMBERS, "Show Line Numbers");
					AppendMenu(m, ((fl_tabsize>4)?MF_CHECKED:MF_UNCHECKED), IDM_UI_TABSIZE, "Large Tabs");
					AppendMenu(m, MF_SEPARATOR, 0, NULL);
					AppendMenu(m, 0, IDM_ENCODING_PRIVATEUSE,					"Convert to UTF-8");
					AppendMenu(m, 0, IDM_ENCODING_DEPRIVATEUSE,					"Convert to Quake encoding");
					AppendMenu(m, 0, IDM_ENCODING_UNIX,							"Convert to Unix Endings");
					AppendMenu(m, 0, IDM_ENCODING_WINDOWS,						"Convert to Dos Endings");

				AppendMenu(rootmenu, MF_POPUP, (UINT_PTR)(m = windowmenu = CreateMenu()),	"&Window");
					AppendMenu(m, 0, IDM_CASCADE,								"Cascade");
					AppendMenu(m, 0, IDM_TILE_HORIZ,							"Tile Horizontally");
					AppendMenu(m, 0, IDM_TILE_VERT,								"Tile Vertically");
				AppendMenu(rootmenu, MF_POPUP, (UINT_PTR)(m = CreateMenu()),	"&Debug");
					AppendMenu(m, 0, IDM_DEBUG_REBUILD,							"Rebuild\tF7");
					AppendMenu(m, 0, IDM_DEBUG_BUILD_OPTIONS,					"Build Options");
					AppendMenu(m, MF_SEPARATOR, 0, NULL);
					AppendMenu(m, 0, IDM_DEBUG_SETNEXT,							"Set Next Statement\tF8");
					AppendMenu(m, 0, IDM_DEBUG_RUN,								"Run/Resume\tF5");
					AppendMenu(m, 0, IDM_DEBUG_STEPOVER,						"Step Over\tF10");
					AppendMenu(m, 0, IDM_DEBUG_STEPINTO,						"Step Into\tF11");
					AppendMenu(m, 0, IDM_DEBUG_STEPOUT,							"Step Out\tShift-F11");
					AppendMenu(m, 0, IDM_DEBUG_TOGGLEBREAK,						"Set Breakpoint\tF9");
				AppendMenu(rootmenu, MF_POPUP, (UINT_PTR)(m = CreateMenu()),	"&Help");
					AppendMenu(m, 0, IDM_ABOUT,									"About");

			SetMenu(hWnd, rootmenu);

			// Retrieve the handle to the window menu and assign the
			// first child window identifier.

			memset(&ccs, 0, sizeof(ccs));
			ccs.hWindowMenu = windowmenu;
			ccs.idFirstChild = IDM_FIRSTCHILD;

			// Create the MDI client window.

			mdibox = CreateWindow( "MDICLIENT", (LPCTSTR) NULL,
					WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
					0, 0, 320, 200, hWnd, (HMENU) 0xCAC, ghInstance, (LPSTR) &ccs);
			ShowWindow(mdibox, SW_SHOW);

			watches = CreateWindow(WC_LISTVIEW, (LPCTSTR) NULL,
					WS_CHILD | WS_VSCROLL | WS_HSCROLL | LVS_REPORT | LVS_EDITLABELS,
					0, 0, 320, 200, hWnd, (HMENU) 0xCAD, ghInstance, NULL);

			SplitterAdd(mdibox, 32, 32);

			if (watches)
			{
				LVCOLUMN col;
				LVITEM newi;

//				ListView_SetUnicodeFormat(watches, TRUE);
				ListView_SetExtendedListViewStyle(watches, LVS_EX_GRIDLINES);
				memset(&col, 0, sizeof(col));
				col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
				col.fmt = LVCFMT_LEFT;
				col.cx = 320;
				col.pszText = "Variable";
				ListView_InsertColumn(watches, 0, &col);
				col.pszText = "Value";
				ListView_InsertColumn(watches, 1, &col);



				memset(&newi, 0, sizeof(newi));                      

				newi.pszText = "<click to add>";
				newi.mask = LVIF_TEXT | LVIF_PARAM;
				newi.lParam = ~0;
				newi.iSubItem = 0;
				ListView_InsertItem(watches, &newi); 
			}

			projecttree = CreateWindow(WC_TREEVIEW, (LPCTSTR) NULL,
					WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL
					|	TVS_HASBUTTONS |TVS_LINESATROOT|TVS_HASLINES,
			0, 0, 320, 200, hWnd, (HMENU) 0xCAC, ghInstance, (LPSTR) &ccs);
			ShowWindow(projecttree, SW_SHOW);

			if (projecttree)
			{
				search_name = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", (LPCTSTR) NULL,
						WS_CHILD | WS_CLIPCHILDREN|CBS_DROPDOWN|CBS_SORT,
						0, 0, 320, 200, hWnd, (HMENU) 0x4403, ghInstance, (LPSTR) NULL);
				{
					//microsoft suck big hairy donkey balls.
					//this tries to get the edit box of the combo control.
					HWND comboedit = GetWindow(search_name, GW_CHILD);
					combosubclassproc = (WNDPROC) SetWindowLongPtr(comboedit, GWLP_WNDPROC, (DWORD_PTR) SearchComboSubClass);
				}
				ShowWindow(search_name, SW_SHOW);
			}
		}
		break;
	case WM_CTLCOLORBTN:
		return (LRESULT)GetSysColorBrush(COLOR_HIGHLIGHT);//COLOR_BACKGROUND;
	case WM_DESTROY:
		DragAcceptFiles(hWnd, FALSE);
		mainwindow = NULL;
		break;

	case WM_DROPFILES:
		{
			HDROP p = (HDROP)wParam;
			char fname[MAX_PATH];
			if (DragQueryFile(p, ~0, (LPSTR) NULL, 0) == 1)
			{
				DragQueryFile(p, 0, fname, sizeof(fname));
				SetProgsSrcFileAndPath(fname);
				resetprogssrc = true;
			}
			DragFinish(p);
		}
		break;

	case WM_SIZE:
		{
			int y;
			GetClientRect(mainwindow, &rect);
			y = rect.bottom;

			for (i = 0; i < NUMBUTTONS; i+=2)
			{
				y -= 24;
				if (!buttons[i+1].hwnd)
					SetWindowPos(buttons[i].hwnd, NULL, 0, y, 192, 24, SWP_NOZORDER);
				else
				{
					SetWindowPos(buttons[i].hwnd, NULL, 0, y, 192/2, 24, SWP_NOZORDER);
					SetWindowPos(buttons[i+1].hwnd, NULL, 192/2, y, 192-192/2, 24, SWP_NOZORDER);
				}
			}

			y -= 24;
			SetWindowPos(search_name, NULL, 0, y, 192, 24, SWP_NOZORDER);

			if (projecttree)
				SetWindowPos(projecttree, NULL, 0, 0, 192, y, SWP_NOZORDER);

			splitterrect.left = 192;
			splitterrect.right = rect.right-rect.left;
			splitterrect.bottom = rect.bottom-rect.top;
			SplitterUpdate();
		}
		break;
//		goto gdefault;
	case WM_ERASEBKGND:
		return TRUE;	//background is clear... or doesn't need clearing (if its fully obscured)
	case WM_PAINT:
		BeginPaint(hWnd,(LPPAINTSTRUCT)&ps);

		EndPaint(hWnd,(LPPAINTSTRUCT)&ps);
		return TRUE;
	case WM_COMMAND:
		i = LOWORD(wParam);
		if (i == 0x4403)
		{
			char buffer[65536];
			char text[128];
			switch(HIWORD(wParam))
			{
			case CBN_EDITUPDATE:
				GetWindowText(search_name, text, sizeof(text)-1);
				if (GenAutoCompleteList(text, buffer, sizeof(buffer)))
				{
					char token[128];
					char *list;
					DWORD start=0,end=0;
					SendMessage(search_name, CB_GETEDITSEL, (WPARAM)&start, (LPARAM)&end);
					ComboBox_ResetContent(search_name);	//windows is shit. this clears the text too.
					SetWindowText(search_name, text);
					ComboBox_SetEditSel(search_name, start, end);
					for (list = buffer; ; )
					{
						list = COM_ParseOut(list, token, sizeof(token));
						if (!*token)
							break;
						ComboBox_AddString(search_name, token);
					}
				}
				return true;
			}
			goto gdefault;
		}
		if (i>=20 && i < 20+NUMBUTTONS)
		{
			i -= 20;
			if (i == ID_DEF)
			{
				GetWindowText(search_name, finddef, sizeof(finddef)-1);
				return true;
			}
			if (i == ID_GREP)
			{
				GetWindowText(search_name, greptext, sizeof(greptext)-1);
				return true;
			}
			buttons[i].washit = 1;
			break;
		}
		if (i < IDM_FIRSTCHILD)
		{
			HWND ew;
			editor_t *editor;
	
			ew = (HWND)SendMessage(mdibox, WM_MDIGETACTIVE, 0, 0);

			for (editor = editors; editor; editor = editor->next)
			{
				if (editor->window == ew)
					break;
			}
			if (editor)
				EditorMenu(editor, wParam);
			else
				GenericMenu(wParam);
			break;
		}
		goto gdefault;
	case WM_NOTIFY:
		if (lParam)
		{
			NMHDR *nm;
			HANDLE item;
			TVITEM i;
			char filename[256];
			char itemtext[256];
			int oldlen;
			int newlen;
			nm = (NMHDR*)lParam;
			if (nm->hwndFrom == watches)
			{
				switch(nm->code)
				{
				case LVN_BEGINLABELEDITA:
					return FALSE;	//false to allow...
				case LVN_BEGINLABELEDITW:
//					OutputDebugString("Begin EditW\n");
					return FALSE;	//false to allow...
				case LVN_ENDLABELEDITA:
					if (((NMLVDISPINFOA*)nm)->item.iItem == ListView_GetItemCount(watches)-1)
					{
						LVITEM newi;
						memset(&newi, 0, sizeof(newi));
						newi.iItem = ListView_GetItemCount(watches);
						newi.pszText = "<click to add>";
						newi.mask = LVIF_TEXT | LVIF_PARAM;
						newi.lParam = ~0;
						newi.iSubItem = 0;
						ListView_InsertItem(watches, &newi);
					}
					EngineCommandf("qcinspect \"%s\" \"%s\"\n", ((NMLVDISPINFOA*)nm)->item.pszText, ""); //term, scope
					PostMessage(mainwindow, WM_SIZE, 0, 0);
					return TRUE;	//true to allow...
/*				case LVN_ENDLABELEDITW:
//					OutputDebugString("End EditW\n");
					if (((NMLVDISPINFOW*)nm)->item.iItem == ListView_GetItemCount(watches)-1)
					{
						LVITEM newi;
						memset(&newi, 0, sizeof(newi));
						newi.iItem = ListView_GetItemCount(watches);
						newi.pszText = "<click to add>";
						newi.mask = LVIF_TEXT | LVIF_PARAM;
						newi.lParam = ~0;
						newi.iSubItem = 0;
						ListView_InsertItem(watches, &newi);
					}
					EngineCommandf("qcinspect \"%s\" \"%s\"\n", ((NMLVDISPINFOW*)nm)->item.pszText, ""); //term, scope
					return TRUE;	//true to allow...
*/
				case LVN_ITEMCHANGING:
//					OutputDebugString("Changing\n");
					return FALSE;	//false to allow...
				case LVN_ITEMCHANGED:
//					OutputDebugString("Changed\n");
					return FALSE;
				case LVN_GETDISPINFOA:
//					OutputDebugString("LVN_GETDISPINFOA\n");
					return FALSE;
//				case LVN_GETDISPINFOW:
//					OutputDebugString("LVN_GETDISPINFOW\n");
//					return FALSE;
				case NM_DBLCLK:
//					OutputDebugString("NM_DBLCLK\n");
					{
						NMITEMACTIVATE *ia = (NMITEMACTIVATE*)nm;
						LVHITTESTINFO ht;
						memset(&ht, 0, sizeof(ht));
						ht.pt = ia->ptAction;
						ListView_SubItemHitTest(watches, &ht);
						ListView_EditLabel(watches, ht.iItem);
					}
					return TRUE;
				case LVN_ITEMACTIVATE:
//					OutputDebugString("LVN_ITEMACTIVATE\n");
					return FALSE;	//must return false
				case LVN_COLUMNCLICK:
//					OutputDebugString("LVN_COLUMNCLICK\n");
					break;
				default:
//					sprintf(filename, "%i\n", nm->code);
//					OutputDebugString(filename);
					break;
				}
				return FALSE;
			}
			else if (nm->hwndFrom == projecttree)
			{
				switch(nm->code)
				{
				case NM_DBLCLK:
					item = TreeView_GetSelection(projecttree);
					memset(&i, 0, sizeof(i));
					i.hItem = item;
					i.mask = TVIF_TEXT|TVIF_PARAM;
					i.pszText = itemtext;
					i.cchTextMax = sizeof(itemtext)-1;
					if (!TreeView_GetItem(projecttree, &i))
						return 0;
					if (!i.lParam)
						return 0;
					strcpy(filename, i.pszText);
					while(item)
					{
						item = TreeView_GetParent(projecttree, item);
						i.hItem = item;
						if (!TreeView_GetItem(projecttree, &i))
							break;
						if (!TreeView_GetParent(projecttree, item))
							break;

						oldlen = strlen(filename);
						newlen = strlen(i.pszText);
						if (oldlen + newlen + 2 > sizeof(filename))
							break;	//don't overflow.
						memmove(filename+newlen+1, filename, oldlen+1);
						filename[newlen] = '/';
						memcpy(filename, i.pszText, newlen);
					}
					EditFile(filename, -1, false);
					break;
				}
			}
		}
	default:
	gdefault:
		if (mdibox)
			return DefFrameProc(hWnd,mdibox,message,wParam,lParam);
		else
			return DefWindowProc(hWnd,message,wParam,lParam);
	}
	return 0;
}

static void DoTranslateMessage(MSG *msg)
{
	if (!TranslateAccelerator(mainwindow, accelerators, msg))
	{
		TranslateMessage(msg);
		DispatchMessage(msg);
	}
}

void GUIPrint(HWND wnd, char *msg)
{
//	MSG        wmsg;
	int len;
	static int writing;

	if (writing)
		return;
	if (!mainwindow)
	{
		printf("%s", msg);
		return;
	}
	writing=true;
	len=Edit_GetTextLength(wnd);
/*	if ((unsigned)len>(32767-strlen(msg)))
		Edit_SetSel(wnd,0,len);
	else*/
		Edit_SetSel(wnd,len,len);
	Edit_ReplaceSel(wnd,msg);

	/*
	while (PeekMessage (&wmsg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&wmsg, NULL, 0, 0))
			break;
		DoTranslateMessage(&wmsg);
	}
	*/
	writing=false;
}

unsigned int utf8_decode(int *error, const void *in, char **out)
{
	//uc is the output unicode char
	unsigned int uc = 0xfffdu;	//replacement character
	//l is the length
	unsigned int l = 1;
	const unsigned char *str = in;

	if ((*str & 0xe0) == 0xc0)
	{
		if ((str[1] & 0xc0) == 0x80)
		{
			l = 2;
			uc = ((str[0] & 0x1f)<<6) | (str[1] & 0x3f);
			if (!uc || uc >= (1u<<7))	//allow modified utf-8
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xf0) == 0xe0)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80)
		{
			l = 3;
			uc = ((str[0] & 0x0f)<<12) | ((str[1] & 0x3f)<<6) | ((str[2] & 0x3f)<<0);
			if (uc >= (1u<<11))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xf8) == 0xf0)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80)
		{
			l = 4;
			uc = ((str[0] & 0x07)<<18) | ((str[1] & 0x3f)<<12) | ((str[2] & 0x3f)<<6) | ((str[3] & 0x3f)<<0);
			if (uc >= (1u<<16))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xfc) == 0xf8)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 5;
			uc = ((str[0] & 0x03)<<24) | ((str[1] & 0x3f)<<18) | ((str[2] & 0x3f)<<12) | ((str[3] & 0x3f)<<6) | ((str[4] & 0x3f)<<0);
			if (uc >= (1u<<21))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xfe) == 0xfc)
	{
		//six bytes
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 6;
			uc = ((str[0] & 0x01)<<30) | ((str[1] & 0x3f)<<24) | ((str[2] & 0x3f)<<18) | ((str[3] & 0x3f)<<12) | ((str[4] & 0x3f)<<6) | ((str[5] & 0x3f)<<0);
			if (uc >= (1u<<26))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	//0xfe and 0xff, while plausable leading bytes, are not permitted.
#if 0
	else if ((*str & 0xff) == 0xfe)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 7;
			uc = 0 | ((str[1] & 0x3f)<<30) | ((str[2] & 0x3f)<<24) | ((str[3] & 0x3f)<<18) | ((str[4] & 0x3f)<<12) | ((str[5] & 0x3f)<<6) | ((str[6] & 0x3f)<<0);
			if (uc >= (1u<<31))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xff) == 0xff)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 8;
			uc = 0 | ((str[1] & 0x3f)<<36) | ((str[2] & 0x3f)<<30) | ((str[3] & 0x3f)<<24) | ((str[4] & 0x3f)<<18) | ((str[5] & 0x3f)<<12) | ((str[6] & 0x3f)<<6) | ((str[7] & 0x3f)<<0);
			if (uc >= (1llu<<36))
				*error = false;
			else
				*error = 2;
		}
		else *error = 1;
	}
#endif
	else if (*str & 0x80)
	{
		//sequence error
		*error = 1;
		uc = 0xe000u + *str;
	}
	else 
	{
		//ascii char
		*error = 0;
		uc = *str;
	}

	*out = (void*)(str + l);

	if (!*error)
	{
		//try to deal with surrogates by decoding the low if we see a high.
		if (uc >= 0xd800u && uc < 0xdc00u)
		{
#if 1
			//cesu-8
			char *lowend;
			unsigned int lowsur = utf8_decode(error, str + l, &lowend);
			if (*error == 4)
			{
				*out = lowend;
				uc = (((uc&0x3ffu) << 10) | (lowsur&0x3ffu)) + 0x10000;
				*error = false;
			}
			else
#endif
			{
				*error = 3;	//bad - lead surrogate without tail.
			}
		}
		if (uc >= 0xdc00u && uc < 0xe000u)
			*error = 4;	//bad - tail surrogate

		//these are meant to be illegal too
		if (uc == 0xfffeu || uc == 0xffffu || uc > 0x10ffffu)
			*error = 2;	//illegal code
	}

	return uc;
}
//outlen is the size of out in _BYTES_.
wchar_t *widen(wchar_t *out, size_t outbytes, const char *utf8, const char *stripchars)
{
	size_t outlen;
	wchar_t *ret = out;
	//utf-8 to utf-16, not ucs-2.
	unsigned int codepoint;
	int error;
	outlen = outbytes/sizeof(wchar_t);
	if (!outlen)
		return L"";
	outlen--;
	while (*utf8)
	{
		if (stripchars && strchr(stripchars, *utf8))
		{	//skip certain ascii chars
			utf8++;
			continue;
		}
		codepoint = utf8_decode(&error, utf8, (void*)&utf8);
		if (error || codepoint > 0x10FFFFu)
			codepoint = 0xFFFDu;
		if (codepoint > 0xffff)
		{
			if (outlen < 2)
				break;
			outlen -= 2;
			codepoint -= 0x10000u;
			*out++ = 0xD800 | (codepoint>>10);
			*out++ = 0xDC00 | (codepoint&0x3ff);
		}
		else
		{
			if (outlen < 1)
				break;
			outlen -= 1;
			*out++ = codepoint;
		}
	}
	*out = 0;
	return ret;
}
int GUIEmitOutputText(HWND wnd, int start, char *text, int len, DWORD colour)
{
	wchar_t wc[2048];
	int c;
	CHARFORMAT cf;

	if (!len)
		return start;

	c = text[len];
	text[len] = '\0';
//	wc = QCC_makeutf16(text, len, &ol);
	widen(wc, sizeof(wc), text, "\r");
	text[len] = c;
	Edit_SetSel(wnd,start,start);
	SendMessageW(wnd, EM_REPLACESEL, 0L, (LPARAM)wc);
	len = wcslen(wc);

	Edit_SetSel(wnd,start,start+len);
	memset(&cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = colour;
	SendMessage(wnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	Edit_SetSel(wnd,start+len,start+len);
	Edit_ScrollCaret(wnd);

	return start + len;
}
int outlen;
int outstatus;
pbool gui_doannotates;
int GUIprintf(const char *msg, ...)
{
	va_list		argptr;
	char		buf[1024];
	char rn[3] = "\n";
	char *st, *s;
	int args;
//	MSG        wmsg;

	DWORD col;

	va_start (argptr,msg);
	args = QC_vsnprintf (buf,sizeof(buf)-1, msg,argptr);
	va_end (argptr);
	buf[sizeof(buf)-5] = '.';
	buf[sizeof(buf)-4] = '.';
	buf[sizeof(buf)-3] = '.';
	buf[sizeof(buf)-2] = '\n';
	buf[sizeof(buf)-1] = 0;

	printf("%s", buf);
	//OutputDebugStringA(buf);
	if (logfile)
		fprintf(logfile, "%s", buf);

	if (!*buf)
	{
		editor_t *ed;
		/*clear text*/
		SetWindowText(outputbox,"");
		outlen = 0;

		/*make sure its active so we can actually scroll. stupid windows*/
		SplitterFocus(outputbox, 64, 0);

		/*colour background to default*/
		TreeView_SetBkColor(projecttree, -1);
		outstatus = 0;


		if (gui_doannotates)
		{
			for (ed = editors; ed; ed = ed->next)
			{
				if (ed->scintilla)
					SendMessage(ed->editpane, SCI_ANNOTATIONCLEARALL, 0, 0);
			}
		}
		return 0;
	}

	if (strstr(buf, ": error") || strstr(buf, ": werror"))
	{
		if (outstatus < 2)
		{
			TreeView_SetBkColor(projecttree, RGB(255, 0, 0));
			outstatus = 2;
		}
		col = RGB(255, 0, 0);
	}
	else if (strstr(buf, ": warning"))
	{
		if (outstatus < 1)
		{
			TreeView_SetBkColor(projecttree, RGB(255, 255, 0));
			outstatus = 1;
		}
		col = RGB(128, 128, 0);
	}
	else
		col = RGB(0, 0, 0);

	s = st = buf;
	while(*s)
	{
		if (*s == '\n')
		{
			*s = '\0';
			if (!strncmp(st, "code: ", 6)) 
				st+=6;
			else
			{
				if (*st)
					outlen = GUIEmitOutputText(outputbox, outlen, st, strlen(st), col);
				outlen = GUIEmitOutputText(outputbox, outlen, rn, 1, col);
			}

			if (gui_doannotates)
			{
				char *colon1 = strchr(st, ':');
				if (colon1)
				{
					char *colon2 = strchr(colon1+1, ':');
					if (colon2)
					{
						unsigned int line;
						char *validation;
						*colon1 = 0;
						line = strtoul(colon1+1, &validation, 10);
						if (validation == colon2)
						{
							editor_t *ed;
							colon2++;
							while(*colon2 == ' ' || *colon2 == '\t')
								colon2++;
							for (ed = editors; ed; ed = ed->next)
							{
								if (!stricmp(ed->filename, st))
								{
									if (ed->scintilla)
									{
										if (!SendMessage(ed->editpane, SCI_ANNOTATIONGETLINES, line-1, 0))
										{
											SendMessage(ed->editpane, SCI_ANNOTATIONSETVISIBLE, ANNOTATION_BOXED, 0);
											SendMessage(ed->editpane, SCI_ANNOTATIONSETTEXT, line-1, (LPARAM)colon2);
										}
										else
										{
											char buf[8192];
											int clen = SendMessage(ed->editpane, SCI_ANNOTATIONGETTEXT, line-1, (LPARAM)NULL);
											if (clen+1+strlen(colon2) < sizeof(buf))
											{
												clen = SendMessage(ed->editpane, SCI_ANNOTATIONGETTEXT, line-1, (LPARAM)buf);
												buf[clen++] = '\n';
												memcpy(buf+clen, colon2, strlen(colon2)+1);
//												SendMessage(ed->editpane, SCI_ANNOTATIONSETVISIBLE, ANNOTATION_BOXED, 0);
												SendMessage(ed->editpane, SCI_ANNOTATIONSETTEXT, line-1, (LPARAM)buf);
											}
										}
									}
									break;
								}
							}
						}
					}
				}
			}
			st = s+1;
		}

		s++;
	}
	if (*st)
		outlen = GUIEmitOutputText(outputbox, outlen, st, strlen(st), col);

/*
	s = st = buf;
	while(*s)
	{
		if (*s == '\n')
		{
			*s = '\0';
			if (*st)
				GUIPrint(outputbox, st);
			GUIPrint(outputbox, "\r\n");
			st = s+1;
		}

		s++;
	}
	if (*st)
		GUIPrint(outputbox, st);
*/
	return args;
}
int Dummyprintf(const char *msg, ...){return 0;}

#undef Sys_Error

void compilecb(void)
{
	//used to repaint the output window periodically instead of letting it redraw as stuff gets sent to it. this can save significant time on mods with boatloads of warnings.
	MSG wmsg;
	if (!SplitterGet(outputbox))
		return;
	SendMessage(outputbox, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(outputbox, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
	while (PeekMessage (&wmsg, NULL, 0, 0, PM_REMOVE))
		DoTranslateMessage(&wmsg);
	SendMessage(outputbox, WM_SETREDRAW, FALSE, 0);
}

void Sys_Error(const char *text, ...);
void RunCompiler(char *args, pbool quick)
{
	const char *argv[256];
	int argc;
	progexterns_t ext;
	progfuncs_t funcs;

	editor_t *editor;
	for (editor = editors; editor; editor = editor->next)
	{
		if (editor->modified)
		{
			if (EditorModified(editor))
			{
				char msg[1024];
				sprintf(msg, "%s is modified in both memory and on disk. Overwrite external modification? (saying no will reload from disk)", editor->filename);
				switch(MessageBox(NULL, msg, "Modification conflict", MB_YESNOCANCEL))
				{
				case IDYES:
					EditorSave(editor);
					break;
				case IDNO:
					EditorReload(editor);
					break;
				case IDCANCEL:
					break; /*compiling will use whatever is in memory*/
				}
			}
			else
			{
				/*not modified on disk, but modified in memory? try and save it, cos we might as well*/
				EditorSave(editor);
			}
		}
		else
		{
			/*modified on disk but not in memory? just reload it off disk*/
			if (EditorModified(editor))
				EditorReload(editor);
		}
	}

	memset(&funcs, 0, sizeof(funcs));
	funcs.funcs.parms = &ext;
	memset(&ext, 0, sizeof(ext));
	ext.ReadFile = GUIReadFile;
	ext.FileSize = GUIFileSize;
	ext.WriteFile = QCC_WriteFile;
	ext.Sys_Error = Sys_Error;

	if (quick)
		ext.Printf = Dummyprintf;
	else
	{
		ext.Printf = GUIprintf;
		GUIprintf("");
	}
	ext.DPrintf = ext.Printf;
	
	if (logfile)
		fclose(logfile);
	if (fl_log && !quick)
		logfile = fopen("fteqcc.log", "wb");
	else
		logfile = NULL;

	if (SplitterGet(outputbox))
		SendMessage(outputbox, WM_SETREDRAW, FALSE, 0);

	argc = GUI_BuildParms(args, argv, sizeof(argv)/sizeof(argv[0]), quick);
	if (!argc)
		ext.Printf("Too many args\n");
	else if (CompileParams(&funcs, outputbox?compilecb:NULL, argc, argv))
	{
		if (!quick)
		{
			EngineGiveFocus();
			EngineCommandf("qcresume\nqcreload\n");
//			EngineCommandf("qcresume\nmenu_restart\nrestart\n");
		}
	}

	if (SplitterGet(outputbox))
	{
		SendMessage(outputbox, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(outputbox, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}

	if (logfile)
		fclose(logfile);
}


static void CreateOutputWindow(pbool doannoates)
{
	gui_doannotates = doannoates;

	if (!outputbox)
	{
		outputbox = CreateAnEditControl(mainwindow, NULL);
	}
	SplitterFocus(outputbox, 64, 128);
}


int GrepSubFiles(HTREEITEM node, char *string)
{
	HTREEITEM ch, p;
	char fullname[1024];
	char parentstring[256];
	int pl, nl;
	TV_ITEM parent;
	int found = 0;

	if (!node)
		return found;

	memset(&parent, 0, sizeof(parent));
	*fullname = 0;
	p = node;
	while (p)
	{
		parent.hItem = p;
		parent.mask = TVIF_TEXT;
		parent.pszText = parentstring;
		parent.cchTextMax = sizeof(parentstring)-1;
		if (!TreeView_GetItem(projecttree, &parent))
			break;
		nl = strlen(fullname);
		pl = strlen(parent.pszText);
		if (nl + 1 + pl + 1 > sizeof(fullname))
			return found;
		p = TreeView_GetParent(projecttree, p);
		if (!p && *fullname)
			break;

		//ignore the root node, unless we're actually querying that root node.
		memmove(fullname+pl+1, fullname, nl+1);
		memcpy(fullname, parent.pszText, pl);
		fullname[pl] = nl?'/':'\0';
	}
	//skip the leading progs.src/ if its there, because that's an abstraction and does not match the filesystem.
	found += Grep(fullname, string);

	ch = TreeView_GetChild(projecttree, node);
	found += GrepSubFiles(ch, string);

	ch = TreeView_GetNextSibling(projecttree, node);
	found += GrepSubFiles(ch, string);

	return found;
}
void GrepAllFiles(char *string)
{
	int found;
	CreateOutputWindow(false);
	GUIprintf("");
	found = GrepSubFiles(TreeView_GetChild(projecttree, TVI_ROOT), string);
	if (found)
		GUIprintf("grep found %i occurences\n", found);
	else
		GUIprintf("grep found nothing\n");
}
void AddSourceFile(const char *parentpath, const char *filename)
{
	char		string[1024];

	HANDLE pi;
	TVINSERTSTRUCT item;
	TV_ITEM parent;
	char parentstring[256];
	char *slash;

	while (!strncmp(filename, "./", 2))
		filename += 2;

	QC_strlcpy(string, filename, sizeof(string));


	memset(&item, 0, sizeof(item));
	memset(&parent, 0, sizeof(parent));

	pi = item.hParent = TVI_ROOT;
	item.hInsertAfter = TVI_LAST;//TVI_SORT;
	item.item.pszText = string;
	item.item.state = TVIS_EXPANDED;
	item.item.stateMask = TVIS_EXPANDED;
	item.item.mask = TVIF_TEXT|TVIF_STATE|TVIF_PARAM;

	if (parentpath && stricmp(parentpath, filename))
	{
		item.hParent = TreeView_GetChild(projecttree, item.hParent);
		do
		{
			parent.hItem = item.hParent;
			parent.mask = TVIF_TEXT;
			parent.pszText = parentstring;
			parent.cchTextMax = sizeof(parentstring)-1;
			if (TreeView_GetItem(projecttree, &parent))
			{
				if (!stricmp(parent.pszText, parentpath))
				{
					pi = item.hParent;
					break;
				}
			}
		} while((item.hParent=TreeView_GetNextSibling(projecttree, item.hParent)));
	}
	else
		parentpath = NULL;

	while(item.item.pszText)
	{
		if (parentpath)
		{
			slash = strchr(item.item.pszText, '/');
			if (slash)
				*slash++ = '\0';
		}
		else
			slash = NULL;

		item.hParent = TreeView_GetChild(projecttree, pi); 
		do
		{
			parent.hItem = item.hParent;
			parent.mask = TVIF_TEXT;
			parent.pszText = parentstring;
			parent.cchTextMax = sizeof(parentstring)-1;
			if (TreeView_GetItem(projecttree, &parent))
			{
				if (!stricmp(parent.pszText, item.item.pszText))
					break;
			}
		} while((item.hParent=TreeView_GetNextSibling(projecttree, item.hParent)));

		if (!item.hParent)
		{	//add a directory.
			item.hParent = pi;
			item.item.lParam = !slash;	//lparam = false if we're only adding this node to get at a child.
			item.item.state = ((*item.item.pszText!='.')?TVIS_EXPANDED:0);	//directories with a leading . should not be expanded by default
			pi = (HANDLE)SendMessage(projecttree,TVM_INSERTITEM,0,(LPARAM)&item);
			item.hParent = pi;
		}
		else pi = item.hParent;

		item.item.pszText = slash;
	}
}

//called when progssrcname has changed.
//progssrcname should already have been set.
void UpdateFileList(void)
{
	TVINSERTSTRUCT item;
	TV_ITEM parent;
	memset(&item, 0, sizeof(item));
	memset(&parent, 0, sizeof(parent));

	if (projecttree)
	{
		size_t size;
		char *buffer;

		AddSourceFile(NULL, progssrcname);

		buffer = QCC_ReadFile(progssrcname, NULL, 0, &size);

		pr_file_p = QCC_COM_Parse(buffer);
		if (*qcc_token == '#')
		{
			//aaaahhh! newstyle!
		}
		else
		{
			pr_file_p = QCC_COM_Parse(pr_file_p);	//we dont care about the produced progs.dat
			while(pr_file_p)
			{
				if (*qcc_token == '#')	//panic if there's preprocessor in there.
					break;

				AddSourceFile(progssrcname, qcc_token);
				pr_file_p = QCC_COM_Parse(pr_file_p);	//we dont care about the produced progs.dat
			}
		}
		free(buffer);

		RunCompiler(parameters, true);
	}
}

static void Packager_MessageCallback(void *ctx, const char *fmt, ...)
{
	va_list		va;
	char		message[1024];
	va_start (va, fmt);
	vsnprintf (message, sizeof(message)-1, fmt, va);
	va_end (va);

	outlen = GUIEmitOutputText(outputbox, outlen, message, strlen(message), RGB(0, 0, 0));
}

void GUI_DoDecompile(void *buf, size_t size)
{
	char *c = ReadProgsCopyright(buf, size);
	if (!c || !*c)
		c = "COPYRIGHT OWNER NOT KNOWN";	//all work is AUTOMATICALLY copyrighted under the terms of the Berne Convention in all major nations. It _IS_ copyrighted, even if there's no license etc included. Good luck guessing what rights you have.
	if (MessageBox(mainwindow, qcva("The copyright message from this progs is\n%s\n\nPlease respect the wishes and legal rights of the person who created this.", c), "Copyright", MB_OKCANCEL|MB_DEFBUTTON2|MB_ICONSTOP) == IDOK)
	{
		CreateOutputWindow(true);
		compilecb();
		DecompileProgsDat(progssrcname, buf, size);
		if (SplitterGet(outputbox))
		{
			SendMessage(outputbox, WM_SETREDRAW, TRUE, 0);
			RedrawWindow(outputbox, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
		}

		QCC_SaveVFiles();
	}
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	pbool fl_acc;
	unsigned int i;
	WNDCLASS wndclass;
	static ACCEL acceleratorlist[] =
	{
		{FCONTROL|FVIRTKEY, 'S', IDM_SAVE},
		{FCONTROL|FVIRTKEY, 'F', IDM_FIND},
		{FCONTROL|FVIRTKEY, 'G', IDM_GREP},
		{FVIRTKEY,			VK_F3, IDM_FINDNEXT},
		{FSHIFT|FVIRTKEY,	VK_F3, IDM_FINDPREV},
//		{FVIRTKEY,			VK_F4, IDM_NEXTERROR},
		{FVIRTKEY,			VK_F5, IDM_DEBUG_RUN},
		{FVIRTKEY,			VK_F6, IDM_OUTPUT_WINDOW},
		{FVIRTKEY,			VK_F7, IDM_DEBUG_REBUILD},
		{FVIRTKEY,			VK_F8, IDM_DEBUG_SETNEXT},
		{FVIRTKEY,			VK_F9, IDM_DEBUG_TOGGLEBREAK},
		{FVIRTKEY,			VK_F10, IDM_DEBUG_STEPOVER},
		{FVIRTKEY,			VK_F11, IDM_DEBUG_STEPINTO},
		{FSHIFT|FVIRTKEY,	VK_F11, IDM_DEBUG_STEPOUT},
		{FVIRTKEY,			VK_F12, IDM_GOTODEF},
		{FSHIFT|FVIRTKEY,	VK_F12, IDM_RETURNDEF}
	};
	int mode;
	ghInstance= hInstance;

	strcpy(enginebinary, "");
	strcpy(enginebasedir, "");
	strcpy(enginecommandline, "");

	GUI_SetDefaultOpts();
	mode = GUI_ParseCommandLine(lpCmdLine, false);

	if(mode == 1)
	{
		RunCompiler(lpCmdLine, false);
		return 0;
	}

	for (i = 0, fl_acc = false; compiler_flag[i].enabled; i++)
	{
		if (!strcmp("acc", compiler_flag[i].abbrev))
		{
			fl_acc = !!(compiler_flag[i].flags & FLAG_SETINGUI);
			break;
		}
	}

	InitCommonControls();

	if (!fl_acc && !*progssrcname)
	{
		strcpy(progssrcname, "preprogs.src");
		if (QCC_RawFileSize(progssrcname)==-1)
			strcpy(progssrcname, "progs.src");
		if (QCC_RawFileSize(progssrcname)==-1)
		{
			char filename[MAX_PATH];
			char oldpath[MAX_PATH+10];
			OPENFILENAME ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hInstance = ghInstance;
			ofn.lpstrFile = filename;
			ofn.lpstrTitle = "Please find progs.src or progs.dat";
			ofn.nMaxFile = sizeof(filename)-1;
			ofn.lpstrFilter = "QuakeC Projects\0*.src;*.dat\0All files\0*.*\0";
			memset(filename, 0, sizeof(filename));
			GetCurrentDirectory(sizeof(oldpath)-1, oldpath);
			ofn.lpstrInitialDir = oldpath;
			if (GetOpenFileName(&ofn))
				strcpy(progssrcname, filename);
			else
			{
				MessageBox(NULL, "You didn't select a file", "Error", 0);
				return 0;
			}
		}
	}

	resetprogssrc = true;

	wndclass.style      = 0;
	wndclass.lpfnWndProc   = MainWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = ghInstance;
	wndclass.hIcon         = LoadIcon(ghInstance, IDI_ICON_FTEQCC);
	wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wndclass.hbrBackground = (void *)COLOR_WINDOW;
	wndclass.lpszMenuName  = 0;
	wndclass.lpszClassName = MDI_WINDOW_CLASS_NAME;
	RegisterClass(&wndclass);

	accelerators = CreateAcceleratorTable(acceleratorlist, sizeof(acceleratorlist)/sizeof(acceleratorlist[0])); 

	mainwindow = CreateWindow(MDI_WINDOW_CLASS_NAME, "FTE QuakeC compiler", WS_OVERLAPPEDWINDOW,
		0, 0, 640, 480, NULL, NULL, ghInstance, NULL);

	if (mdibox)
	{
		SetWindowText(mainwindow, "FTE QuakeC Development Suite");
	}

	if (!mainwindow)
	{
		MessageBox(NULL, "Failed to create main window", "Error", 0);
		return 0;
	}

/*
	outputbox=CreateWindowEx(WS_EX_CLIENTEDGE,
		"EDIT",
		"",
		WS_CHILD | ES_READONLY | WS_VISIBLE | 
		WS_VSCROLL | ES_LEFT | ES_WANTRETURN |
		ES_MULTILINE | ES_AUTOVSCROLL,
		0, 0, 0, 0,
		mainwindow,
		NULL,
		ghInstance,
		NULL);
*/

	if (!mdibox)
		outputbox = CreateAnEditControl(mainwindow, NULL);

	for (i = 0; i < NUMBUTTONS; i++)
	{
		if (!buttons[i].text)
			buttons[i].hwnd = NULL;
		else
			buttons[i].hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
				"BUTTON",
				buttons[i].text,
				WS_CHILD | WS_VISIBLE,
				0, 0, 5, 5,
				mainwindow,
				(HMENU)(LONG_PTR)(i+20),
				ghInstance,
				NULL); 
	}

	ShowWindow(mainwindow, SW_SHOWDEFAULT);

	resetprogssrc = true;

	while(mainwindow || editors)
	{
		MSG        msg;

		if (resetprogssrc)
		{	//this here, with the compiler below, means that we don't run recursivly.
			if (projecttree)
				TreeView_DeleteAllItems(projecttree);

			//if progssrcname is a path, then change working directory now.
			//this shouldn't affect that much, but should ensure well-defined behaviour.
			{
				char *s, *s2;
				strcpy(progssrcdir, progssrcname);
				for(s = NULL, s2 = progssrcdir; s2;)
				{
					char *bs = strchr(s2, '\\');
					char *sl = strchr(s2, '/');
					if (bs)
						s2 = bs;
					else if (sl)
						s2 = sl;
					else
						break;
					s = s2++;
				}
				if (s)
				{
					*s = '\0';
					strcpy(progssrcname, s+1);
					SetCurrentDirectory(progssrcdir);
				}
				*progssrcdir = '\0';
			}

			//reset project/directory options
			GUI_SetDefaultOpts();
			GUI_ParseCommandLine(lpCmdLine, true);
			GUI_RevealOptions();

			//if the project is a .dat or .zip then decompile it now (so we can access the 'source')
			{
				char *ext = strrchr(progssrcname, '.');
				if (ext && (!QC_strcasecmp(ext, ".dat") || !QC_strcasecmp(ext, ".pak") || !QC_strcasecmp(ext, ".zip") || !QC_strcasecmp(ext, ".pk3")))
				{
					FILE *f = fopen(progssrcname, "rb");
					if (f)
					{
						char *buf;
						size_t size;

						fseek(f, 0, SEEK_END);
						size = ftell(f);
						fseek(f, 0, SEEK_SET);
						buf = malloc(size);
						fread(buf, 1, size, f);
						fclose(f);
						QCC_CloseAllVFiles();
						if (!QC_EnumerateFilesFromBlob(buf, size, QCC_EnumerateFilesResult) && !QC_strcasecmp(ext, ".dat"))
						{	//its a .dat and contains no .src files
							GUI_DoDecompile(buf, size);
						}
						else if (!QCC_FindVFile("progs.src"))
						{
							vfile_t *f;
							char *archivename = progssrcname;
							while(strchr(archivename, '\\'))
								 archivename = strchr(archivename, '\\')+1;
							AddSourceFile(NULL, archivename);
							for (f = qcc_vfiles; f; f = f->next)
								AddSourceFile(archivename,	f->filename);

							f = QCC_FindVFile("progs.dat");
							if (f)
								GUI_DoDecompile(f->file, f->size);
							else
								resetprogssrc = false;
						}
						free(buf);
						strcpy(progssrcname, "progs.src");
					}
					else
						strcpy(progssrcname, "progs.src");

					for (i = 0; ; i++)
					{
						if (!strcmp("embedsrc", compiler_flag[i].abbrev))
						{
							compiler_flag[i].flags |= FLAG_SETINGUI;
							break;
						}
					}
				}
			}

			if (fl_compileonstart)
			{
				if (resetprogssrc)
				{
					CreateOutputWindow(false);
					RunCompiler(lpCmdLine, false);
				}
			}
			else
			{
				if (!mdibox)
				{
					GUIprintf("Welcome to FTE QCC\n");
					GUIprintf("Source file: ");
					GUIprintf(progssrcname);
					GUIprintf("\n");

					RunCompiler("-?", false);
				}
			}
			if (resetprogssrc)
				UpdateFileList();
			resetprogssrc = false;
		}

		EditorsRun();

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				break;
			if (!mdibox || !TranslateMDISysAccel(mdibox, &msg))
				DoTranslateMessage(&msg);
		}

		if (mainwindow)
		{
			if (buttons[ID_COMPILE].washit)
			{
				CreateOutputWindow(true);
				RunCompiler(parameters, false);

				buttons[ID_COMPILE].washit = false;
			}
#ifdef EMBEDDEBUG
			if (buttons[ID_RUN].washit)
			{
				buttons[ID_RUN].washit = false;
				RunEngine();
			}
#endif
			if (buttons[ID_OPTIONS].washit)
			{
				buttons[ID_OPTIONS].washit = false;
				OptionsDialog();
			}
		}

		if (*finddef)
		{
			GoToDefinition(finddef);
			*finddef = '\0';
		}
		if (*greptext)
		{
			GrepAllFiles(greptext);
			*greptext = '\0';
		}

		Sleep(10);
	}

	return 0;
}
