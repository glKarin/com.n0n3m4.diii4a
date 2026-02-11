#if !defined(MINIMAL) && !defined(OMIT_QCC)

#include "qcc.h"
#include "time.h"

#define MEMBERFIELDNAME "__m%s"

#define STRCMP(s1,s2) (((*s1)!=(*s2)) || strcmp(s1,s2))	//saves about 2-6 out of 120 - expansion of idea from fastqcc

void QCC_PR_PreProcessor_Define(pbool append);
pbool QCC_PR_UndefineName(const char *name);
const char *QCC_PR_CheckCompConstString(const char *def);
CompilerConstant_t *QCC_PR_CheckCompConstDefined(const char *def);
int QCC_PR_CheckCompConst(void);
void QCC_FreeDef(QCC_def_t *def);
void QCC_PR_LexComment(char **comment);

extern pbool	destfile_explicit;
extern char		destfile[1024];
//static const QCC_sref_t nullsref = {0};

#define MAXINCLUDEDIRS 8
char	qccincludedir[MAXINCLUDEDIRS][256];	//the -src path, for #includes
struct qccincludeonced_s
{
	struct qccincludeonced_s *next;
	char name[1];
} *qccincludeonced;	//the -src path, for #includes

char *compilingfile;

int			pr_source_line;

char		*pr_file_p;
char		*pr_line_start;		// start of current source line

int			pr_bracelevel;

char		*pr_token_precomment;
char		pr_token[8192];
token_type_t	pr_token_type;
int				pr_token_line;
int				pr_token_line_last;
QCC_type_t		*pr_immediate_type;
QCC_eval_t		pr_immediate;

char	pr_immediate_string[8192];
size_t	pr_immediate_strlen;

int		pr_error_count;
int		pr_warning_count;

extern pbool expandedemptymacro;

//really these should not be in here
extern unsigned int locals_end, locals_start;
extern QCC_type_t *pr_classtype;
QCC_function_t *QCC_PR_ParseImmediateStatements (QCC_def_t *def, QCC_type_t *type, pbool dowrap);
QCC_type_t *QCC_PR_FieldType (QCC_type_t *pointsto);

static void Q_strlcpy(char *dest, const char *src, int sizeofdest)
{
	if (sizeofdest)
	{
		int slen = strlen(src);
		slen = min((sizeofdest-1), slen);
		memcpy(dest, src, slen);
		dest[slen] = 0;
	}
}



char	*pr_punctuation[] =
// longer symbols must be before a shorter partial match
{"&&", "||", "<=>", "<=", ">=","==", "!=", "/=", "*=", "+=", "-=", "(+)", "(-)", "|=", "&~=", "&=", "++", "--", "->", "^=", "::", ";", ",", "!", "*^", "*", "/", "(", ")", "-", "+", "=", "[", "]", "{", "}", "...", "..", ".", "><", "<<=", "<<", "<", ">>=", ">>", ">" , "?", "#" , "@", "&" , "|", "%", "^^", "^", "~", ":", NULL};

char *pr_punctuationremap[] =	//a nice bit of evilness.
//(+) -> |=
//-> -> .
//(-) -> &~=
{"&&", "||", "<=>", "<=", ">=","==", "!=", "/=", "*=", "+=", "-=", "|=",  "&~=", "|=", "&~=", "&=", "++", "--", ".",  "^=", "::", ";", ",", "!", "*^", "*", "/", "(", ")", "-", "+", "=", "[", "]", "{", "}", "...", "..", ".", "><", "<<=", "<<", "<", ">>=", ">>", ">" , "?", "#" , "@", "&" , "|", "%", "^^", "^", "~", ":", NULL};

// simple types.  function types are dynamically allocated
QCC_type_t	*type_void;				//void
QCC_type_t	*type_string;			//string
QCC_type_t	*type_float;			//float
QCC_type_t	*type_double;			//double
QCC_type_t	*type_vector;			//vector
QCC_type_t	*type_entity;			//entity
QCC_type_t	*type_field;			//.void
QCC_type_t	*type_function;			//void()
QCC_type_t	*type_floatfunction;	//float()
QCC_type_t	*type_pointer;			//??? * - careful with this one
QCC_type_t	*type_sint8;				//char	- these small types are technically ev_bitfield, but typedefed and aligned.
QCC_type_t	*type_uint8;			//unsigned char
QCC_type_t	*type_sint16;			//short
QCC_type_t	*type_uint16;			//unsigned short
QCC_type_t	*type_integer;			//int32
QCC_type_t	*type_uint;				//uint32
QCC_type_t	*type_int64;			//int64
QCC_type_t	*type_uint64;			//uint64
QCC_type_t	*type_variant;			//__variant
QCC_type_t	*type_invalid;			//technically void, but shouldn't really ever be used.
QCC_type_t	*type_floatpointer;		//float *
QCC_type_t	*type_intpointer;		//int *
QCC_type_t	*type_bint;				//int (0 or 1)
QCC_type_t	*type_bfloat;			//float (0.0 or 1.0, and never -0.0)

QCC_type_t	*type_floatfield;// = {ev_field/*, &def_field*/, NULL, &type_float};

QCC_def_t	def_ret, def_parms[MAX_PARMS];

//QCC_def_t	*def_for_type[9] = {&def_void, &def_string, &def_float, &def_vector, &def_entity, &def_field, &def_function, &def_pointer, &def_integer};

void QCC_PR_LexWhitespace (pbool inhibitpreprocessor);


QCC_type_t *QCC_PR_ParseEnum(pbool flags);

//for compiler constants and file includes.

qcc_includechunk_t *currentchunk;
void QCC_PR_CloseProcessor(void)
{
	int i;
	for (i = 0; i < MAXINCLUDEDIRS; i++)
		*qccincludedir[i] = 0;
	currentchunk = NULL;
	qccincludeonced = NULL;
}
void QCC_PR_AddIncludePath(const char *newinc)
{
	int i;

	if (!*newinc)
	{
		newinc = ".";
//		QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Invalid include path.");
//		return;
	}

	for (i = 0; i < MAXINCLUDEDIRS; i++)
	{
		if (!*qccincludedir[i])
		{
			pbool trunc;
			const char *e = newinc + strlen(newinc)-1;
			trunc = !QC_strlcpy(qccincludedir[i], newinc, sizeof(qccincludedir));
			if (*e != '/' && *e != '\\')
				trunc |= !QC_strlcat(qccincludedir[i], "/", sizeof(qccincludedir));
			if (trunc)
			{
				QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Include path too long.");
				*qccincludedir[i] = 0;
			}
			break;
		}
		if (!strcmp(qccincludedir[i], newinc))
			break;
	}
	if (i == MAXINCLUDEDIRS)
	{
		QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Too many include dirs. Ignoring and hoping the stars align.");
	}
}
void QCC_PR_IncludeChunkEx (char *data, pbool duplicate, char *filename, CompilerConstant_t *cnst)
{
	qcc_includechunk_t *chunk = qccHunkAlloc(sizeof(qcc_includechunk_t));
	chunk->prev = currentchunk;
	currentchunk = chunk;

	chunk->currentdatapoint = pr_file_p;
	chunk->currentfilename = s_filen;
	chunk->currentlinenumber = pr_source_line;
	chunk->cnst = cnst;
	if( cnst )
	{
#if 0
		s_filen = cnst->fromfile;
		pr_source_line = cnst->fromline;
#else
		int b = strlen(s_filen)+1+8+strlen(cnst->name);
		char *p;
		if (b > 128)
			b = 128;
		s_filen = p = qccHunkAlloc(b);
		QC_snprintfz(p, b, "%s:%i:%s", chunk->currentfilename, chunk->currentlinenumber, cnst->name);
		pr_source_line = 1;
#endif
		cnst->inside++;
	}
	else
		pr_source_line = 1;

	if (duplicate)
	{
		pr_file_p = qccHunkAlloc(strlen(data)+1);
		strcpy(pr_file_p, data);
	}
	else
		pr_file_p = data;
	chunk->datastart = pr_file_p;
}
void QCC_PR_IncludeChunk (char *data, pbool duplicate, char *filename)
{
	QCC_PR_IncludeChunkEx(data, duplicate, filename, NULL);
}

pbool QCC_PR_UnInclude(void)
{
	if (!currentchunk)
		return false;

	if( currentchunk->cnst )
		currentchunk->cnst->inside--;

	pr_file_p = currentchunk->currentdatapoint;
	pr_source_line = currentchunk->currentlinenumber;
	s_filen = currentchunk->currentfilename;

	currentchunk = currentchunk->prev;

	return true;
}

//expresses a relative path relative to an existing FILEname. any directories in base must be / terminated properly.
void QCC_JoinPaths(char *fullname, size_t fullnamesize, const char *newfile, const char *base)
{
	char *end;

	if (*newfile == '/' || *newfile == '\\')
	{	//its an absolute path...
		QC_strlcpy(fullname, newfile, fullnamesize);
		return;
	}
	QC_strlcpy(fullname, base, fullnamesize);
	end = fullname+strlen(fullname);
	while (end > fullname)
	{
		end--;
		if (*end == '/' || *end == '\\')
		{
			end++;
			break;
		}
	}
	QC_strlcpy(end, newfile, fullnamesize - (end-fullname));

	//FIXME: do we want to convert /segment/../ into just / ?
	//fteqw might insist on it for its filesystem sandboxing.
	//but it breaks symlink weirdness (itself a possible security hole).
	//should probably be a separate function.
}

extern char qccmsourcedir[];
//also meant to include it.
void QCC_FindBestInclude(char *newfile, char *currentfile, pbool includetype)
{
	struct qccincludeonced_s *onced;
	int includepath = 0;
	char fullname[1024];

	if (!*newfile)
		return;

	while(1)
	{
		if (includepath)
		{
			if (includepath > MAXINCLUDEDIRS || !*qccincludedir[includepath-1])
				QCC_Error(ERR_COULDNTOPENFILE, "Couldn't open file %s", newfile);

			currentfile = qccincludedir[includepath-1];
		}

		QCC_JoinPaths(fullname, sizeof(fullname), newfile, currentfile);

		{
			extern progfuncs_t *qccprogfuncs;
			if (qccprogfuncs->funcs.parms->FileSize(fullname) == -1)
			{
				includepath++;
				continue;
			}
		}
		break;
	}

	for(onced = qccincludeonced; onced; onced = onced->next)
	{
		if (!strcmp(onced->name, fullname))
			return;
	}

	if (includetype && verbose >= VERBOSE_PROGRESS)
	{
		if (includetype == 2)
		{
			if (autoprototype)
				externs->Printf("prototyping %s\n", fullname);
			else
				externs->Printf("compiling %s\n", fullname);
		}
		else
		{
			if (autoprototype)
				externs->Printf("prototyping include %s\n", fullname);
			else
				externs->Printf("including %s\n", fullname);
		}
	}
	QCC_Include(fullname, includetype);
}

pbool defaultnoref;
pbool defaultnosave;
pbool defaultstatic;
int ForcedCRC;
float qcc_framerate;

int QCC_PR_LexInteger (void);
void	QCC_AddFile (char *filename);
void QCC_PR_LexString (void);
pbool QCC_PR_SimpleGetToken (void);
pbool QCC_PR_SimpleGetString(void);

#define PPI_VALUE 0
#define PPI_NOT 1
#define PPI_DEFINED 2
#define PPI_COMPARISON 3
#define PPI_LOGICAL 4
#define PPI_TOPLEVEL 5
static int ParsePrecompilerIf(int level)
{
	CompilerConstant_t *c;
	int eval = 0;
//	pbool notted = false;

	//single term end-of-chain
	if (level == PPI_VALUE)
	{
		/*skip whitespace*/
		while (*pr_file_p && qcc_iswhite(*pr_file_p) && *pr_file_p != '\n')
		{
			pr_file_p++;
		}

		if (*pr_file_p == '(')
		{	//try brackets
			pr_file_p++;
			eval = ParsePrecompilerIf(PPI_TOPLEVEL);
			while (*pr_file_p == ' ' || *pr_file_p == '\t')
				pr_file_p++;
			if (*pr_file_p != ')')
				QCC_PR_ParseError(ERR_EXPECTED, "unclosed bracket condition\n");
			pr_file_p++;
		}
		else if (*pr_file_p == '!')
		{	//try brackets
			pr_file_p++;
			eval = !ParsePrecompilerIf(PPI_NOT);
		}
		else
		{	//simple token...
			if (!strncmp(pr_file_p, "defined", 7))
			{
				pbool brackets;
				pr_file_p+=7;
				while (*pr_file_p == ' ' || *pr_file_p == '\t')
					pr_file_p++;
				brackets = *pr_file_p == '(';
				pr_file_p += brackets;

				QCC_PR_SimpleGetToken();
				eval = !!QCC_PR_CheckCompConstDefined(pr_token);

				if (brackets)
				{
					while (*pr_file_p == ' ' || *pr_file_p == '\t')
						pr_file_p++;
					if (*pr_file_p != ')')
						QCC_PR_ParseError(ERR_EXPECTED, "unclosed defined condition\n");
					pr_file_p++;
				}
			}
			else
			{
				if (!QCC_PR_SimpleGetToken())
					QCC_PR_ParseError(ERR_EXPECTED, "unexpected end-of-line\n");
				c = QCC_PR_CheckCompConstDefined(pr_token);
				if (!c)
					eval = atoi(pr_token);
				else
					eval = atoi(c->value);
			}
		}
		return eval;
	}

	eval = ParsePrecompilerIf(level-1);

	while (*pr_file_p && qcc_iswhite(*pr_file_p) && *pr_file_p != '\n')
	{
		pr_file_p++;
	}

	switch(level)
	{
	case PPI_LOGICAL:
		if (!strncmp(pr_file_p, "||", 2))
		{
			pr_file_p+=2;
			eval = ParsePrecompilerIf(level)||eval;
		}
		else if (!strncmp(pr_file_p, "&&", 2))
		{
			pr_file_p+=2;
			eval = ParsePrecompilerIf(level)&&eval;
		}
		break;
	case PPI_COMPARISON:
		if (!strncmp(pr_file_p, "<=", 2))
		{
			pr_file_p+=2;
			eval = eval <= ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, ">=", 2))
		{
			pr_file_p += 2;
			eval = eval >= ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, "<", 1))
		{
			pr_file_p += 1;
			eval = eval < ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, ">", 1))
		{
			pr_file_p += 1;
			eval = eval > ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, "!=", 2))
		{
			pr_file_p += 2;
			eval = eval != ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, "==", 2))
		{
			pr_file_p += 2;
			eval = eval == ParsePrecompilerIf(level);
		}
		break;
	}
	return eval;
}

struct deflist_s
{
	char *buffer;
	size_t length;
	size_t buffersize;
};
static void QCC_PR_GetDefinesListEnumerate(void *vctx, void *data)
{
	struct deflist_s *ctx = vctx;
	CompilerConstant_t *def = data;
	char term[8192];
	size_t termsize;
	pbool success = true;

	QC_snprintfz(term, sizeof(term), "\n%s", def->name);
	if (def->numparams >= 0)
	{
		int i;
		success &= QC_strlcat(term, "(", sizeof(term));
		for (i = 0; i < def->numparams; i++)
		{
			if (i)
				success &= QC_strlcat(term, ",", sizeof(term));
			success &= QC_strlcat(term, def->params[i], sizeof(term));
		}
		success &= QC_strlcat(term, ")", sizeof(term));
	}
	if (def->value && *def->value)
	{
		char *o, *i;
		success &= QC_strlcat(term, "=", sizeof(term));

		//annoying logic to skip whitespace... hopefully it won't fuck stuff up too much.
		for (o = term+strlen(term), i = def->value; o < term + sizeof(term)-1 && *i; )
		{
			if (*i == ' ' || *i == '\t' || *i == '\n' || *i == '\r')
				i++;
			else
				*o++ = *i++;
		}
		*o = 0;
	}
	if (!success)	//this define was too long. don't show truncated stuff.
		return;

	termsize = strlen(term);
	if (ctx->length + termsize+1 > ctx->buffersize)
	{
		ctx->buffersize = (ctx->length + termsize+1)*2;
		ctx->buffer = realloc(ctx->buffer, ctx->buffersize);
	}
	memcpy(ctx->buffer+ctx->length, term, termsize);
	ctx->length += termsize;
	ctx->buffer[ctx->length] = 0;
}
char *QCC_PR_GetDefinesList(void)
{
	struct deflist_s ctx = {NULL};
	Hash_Enumerate(&compconstantstable, QCC_PR_GetDefinesListEnumerate, &ctx);
	return ctx.buffer;
}

//returns true if it was white/comments only. false if there was actual text that was skipped.
static void QCC_PR_SkipToEndOfLine(pbool errorifnonwhite)
{
	pbool handleccomments = true;
	while(*pr_file_p != '\n' && *pr_file_p != '\0')	//read on until the end of the line
	{
		if (*pr_file_p == '/' && pr_file_p[1] == '*' && handleccomments)
		{
			pr_file_p += 2;
			while(*pr_file_p)
			{
				if (*pr_file_p == '*' && pr_file_p[1] == '/')
				{
					pr_file_p+=2;
					break;
				}
				if (*pr_file_p == '\n')
					pr_source_line++;
				pr_file_p++;
			}
		}
		else if (*pr_file_p == '/' && pr_file_p[1] == '/' && handleccomments)
		{
			handleccomments = false;
			pr_file_p += 2;
			/*while(*pr_file_p)
			{
				if (*pr_file_p == '\n')
					break;
				pr_file_p++;
			}*/
		}
		else if (*pr_file_p == '\\' && pr_file_p[1] == '\r' && pr_file_p[2] == '\n')
		{	/*windows endings*/
			pr_file_p+=3;
			pr_source_line++;
		}
		else if (*pr_file_p == '\"' && handleccomments)
		{
			if (errorifnonwhite)
			{
				errorifnonwhite = false;
				QCC_PR_ParseWarning (ERR_UNKNOWNPUCTUATION, "unexpected tokens at end of line");
			}
			pr_file_p++;
			while (*pr_file_p)
			{
				if (*pr_file_p == '\n')
					break;	//this text is junk/ignored, so ignore the obvious error here.
				else if (*pr_file_p == '\"')
				{
					pr_file_p++;
					break;
				}
				else if (*pr_file_p == '\\' && pr_file_p[1] == '\"')
					pr_file_p+=2;	//don't trip on "\"/*"
				else if (*pr_file_p == '\\' && pr_file_p[1] == '\\')
					pr_file_p+=2;	//don't trip on "\\"//"foo
				else
					pr_file_p++;
				//any other \ should be part of the actual string, which we don't care about here
			}
		}
		else if (*pr_file_p == '\\' && pr_file_p[1] == '\n')
		{	/*linux endings*/
			pr_file_p+=2;

			pr_source_line++;
		}
		else
		{
			if (errorifnonwhite && handleccomments && !qcc_iswhite(*pr_file_p))
			{
				errorifnonwhite = false;
				QCC_PR_ParseWarning(ERR_UNKNOWNPUCTUATION, "unexpected tokens at end of line");
			}

			pr_file_p++;
		}
	}
}

//if hadtrue, then we allow elses, otherwise we skip them.
static pbool QCC_PR_FalsePreProcessorIf(pbool hadtrue, int originalline)
{
	int eval;
	int level = 1;
	while (1)
	{
		while(*pr_file_p && (*pr_file_p==' ' || *pr_file_p == '\t'))
			pr_file_p++;

		if (!*pr_file_p)
		{
			pr_source_line = originalline;
			QCC_PR_ParseError (ERR_NOENDIF, "#if with no endif");
		}

		if (*pr_file_p == '#')
		{
			pr_file_p++;
			while(*pr_file_p==' ' || *pr_file_p == '\t')
				pr_file_p++;
			if (!strncmp(pr_file_p, "endif", 5))
				level--;
			if (!strncmp(pr_file_p, "if", 2))
				level++;
			if (!hadtrue && !strncmp(pr_file_p, "else", 4) && level == 1)
			{
				pr_file_p+=4;
				QCC_PR_SkipToEndOfLine(true);
				return true;
			}
			if (!hadtrue && !strncmp(pr_file_p, "elif", 4) && level == 1)
			{
//				QCC_PR_ParseError(ERR_UNKNOWNPUCTUATION, "#elif not supported\n");

				pr_file_p += 4;
				if (!strncmp(pr_file_p, "def", 3))
				{
					eval = 1;
					pr_file_p += 3;
				}
				else if (!strncmp(pr_file_p, "ndef", 4))
				{
					eval = 0;
					pr_file_p += 4;
				}
				else
					eval = 2;
				if (*pr_file_p != ' ' && *pr_file_p != '\t')
					QCC_PR_ParseError(ERR_UNKNOWNPUCTUATION, "malformed #elif\n");
				if (eval == 2)
					eval = ParsePrecompilerIf(PPI_TOPLEVEL);
				else
				{
					QCC_PR_SimpleGetToken ();
					if (eval)
						eval = !!QCC_PR_CheckCompConstDefined(pr_token);
					else
						eval = !QCC_PR_CheckCompConstDefined(pr_token);
				}
				if (eval)
				{
					QCC_PR_SkipToEndOfLine(true);
					return true;
				}
			}
		}

		QCC_PR_SkipToEndOfLine(false);

		if (level <= 0)
			return false;
		pr_file_p++;	//next line
		pr_source_line++;
	}
}

#if 0
static void QCC_PR_PackagerMessage(void *userctx, char *message, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,message);
	QC_vsnprintf (string,sizeof(string)-1,message,argptr);
	va_end (argptr);

	externs->Printf ("%s", string);
}
#endif

/*
==============
QCC_PR_Precompiler
==============

Runs precompiler stage
*/
static pbool QCC_PR_Precompiler(void)
{
	char msg[1024];
	int ifmode;
	int a;
	static int ifs = 0;
	pbool eval = false;

	if (*pr_file_p == '#')
	{
		char *directive;
		for (directive = pr_file_p+1; *directive; directive++)	//so #    define works
		{
			if (*directive == '\r' || *directive == '\n')
				QCC_PR_ParseError(ERR_UNKNOWNPUCTUATION, "Hanging # with no directive\n");
			if (*directive > ' ')
				break;
		}
		if (!strncmp(directive, "define", 6))
		{
			pr_file_p = directive;
			QCC_PR_PreProcessor_Define(false);
			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "append", 6))
		{
			pr_file_p = directive;
			QCC_PR_PreProcessor_Define(true);
			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "undef", 5))
		{
			pr_file_p = directive+5;
			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			QCC_PR_SimpleGetToken ();
			QCC_PR_UndefineName(pr_token);

	//		QCC_PR_ConditionCompilation();
			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "if", 2))
		{
			int originalline = pr_source_line;
 			pr_file_p = directive+2;
			if (!strncmp(pr_file_p, "def", 3))
			{
				ifmode = 0;
				pr_file_p+=3;
			}
			else if (!strncmp(pr_file_p, "ndef", 4))
			{
				ifmode = 1;
				pr_file_p+=4;
			}
			else
			{
				ifmode = 2;
				pr_file_p+=0;
				//QCC_PR_ParseError("bad \"#if\" type");
			}

			if (!qcc_iswhite(*pr_file_p))
			{
				pr_file_p = directive;
				QCC_PR_SimpleGetToken ();
				QCC_PR_ParseWarning(WARN_BADPRAGMA, "Unknown pragma \'%s\'", qcc_token);
			}
			else
			{
				if (ifmode == 2)
				{
					eval = ParsePrecompilerIf(PPI_TOPLEVEL);
				}
				else
				{
					QCC_PR_SimpleGetToken ();

		//			if (!STRCMP(pr_token, "COOP_MODE"))
		//				eval = false;
					if (QCC_PR_CheckCompConstDefined(pr_token))
						eval = true;

					if (ifmode == 1)
						eval = eval?false:true;
				}

				QCC_PR_SkipToEndOfLine(true);

				if (eval)
					ifs+=1;
				else
					ifs += QCC_PR_FalsePreProcessorIf(false, originalline);
			}
		}
		else if (!strncmp(directive, "else", 4) || !strncmp(directive, "elif", 4))
		{
			int originalline = pr_source_line;

			if (!ifs)
				QCC_PR_ParseError(ERR_UNKNOWNPUCTUATION, "#else outside of #if\n");

			ifs -= 1;

			pr_file_p = directive+4;
			if (!strncmp(directive, "elif", 4))
				QCC_PR_SkipToEndOfLine(false);
			else
				QCC_PR_SkipToEndOfLine(true);

			ifs += QCC_PR_FalsePreProcessorIf(true, originalline);
		}
		else if (!strncmp(directive, "endif", 5))
		{
			pr_file_p = directive+5;
			QCC_PR_SkipToEndOfLine(true);
			if (ifs <= 0)
				QCC_PR_ParseError(ERR_NOPRECOMPILERIF, "unmatched #endif");
			else
				ifs-=1;
		}
		else if (!strncmp(directive, "eof", 3))
		{
			pr_file_p = NULL;
			return true;
		}
		else if (!strncmp(directive, "error", 5))
		{
			pr_file_p = directive+5;
			for (a = 0; a < sizeof(msg)-1 && pr_file_p[a] != '\t' && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(false);

			QCC_PR_ParseError(ERR_HASHERROR, "#Error: %s", msg);
		}
		else if (!strncmp(directive, "warning", 7))
		{
			pr_file_p = directive+7;
			for (a = 0; a < 1023 && pr_file_p[a] != '\r' && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(false);

			QCC_PR_ParseWarning(WARN_PRECOMPILERMESSAGE, "#warning: %s", msg);
		}
		else if (!strncmp(directive, "message", 7))
		{
			pr_file_p = directive+7;
			for (a = 0; a < sizeof(msg)-1 && pr_file_p[a] != '\r' && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			if (flag_msvcstyle)
				externs->Printf ("%s(%i) : #message: %s\n", s_filen, pr_source_line, msg);
			else
				externs->Printf ("%s:%i: #message: %s\n", s_filen, pr_source_line, msg);
			QCC_PR_SkipToEndOfLine(false);
		}
		else if (!strncmp(directive, "copyright", 9))
		{
			pr_file_p = directive+9;
			for (a = 0; a < sizeof(msg)-1 && qcc_iswhite(pr_file_p[a]) && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(false);

			if (strlen(msg) >= sizeof(QCC_copyright))
				QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Copyright message is too long\n");
			QC_strlcpy(QCC_copyright, msg, sizeof(QCC_copyright)-1);
		}
		else if (!strncmp(directive, "package", 7))
		{
			pr_file_p=directive+7;

			QCC_PR_SkipToEndOfLine(true);

#if 0
			if (!autoprototype)
			{
				struct pkgctx_s *ctx = Packager_Create(QCC_PR_PackagerMessage, NULL);
				Packager_ParseText(ctx, pr_file_p);
				Packager_WriteDataset(ctx, NULL);
				Packager_Destroy(ctx);
			}
#endif

			pr_file_p += strlen(pr_file_p);
		}
		else if (!strncmp(directive, "pack", 4))
		{
			ifmode = 0;
			pr_file_p=directive+4;
			if (!strncmp(pr_file_p, "id", 2))
				pr_file_p+=3;
			else
			{
				ifmode = QCC_PR_LexInteger();
				if (ifmode == 0)
					ifmode = 1;
				pr_file_p++;
			}
			for (a = 0; a < sizeof(msg)-1 && qcc_iswhite(pr_file_p[a]) && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(true);

			if (ifmode == 0)
				QCC_packid = atoi(msg);
			else if (ifmode <= 5)
				strcpy(QCC_Packname[ifmode-1], msg);
			else
				QCC_PR_ParseError(ERR_TOOMANYPACKFILES, "No more than 5 packs are allowed");
		}
		else if (!strncmp(directive, "forcecrc", 8))
		{
			pr_file_p=directive+8;

			ForcedCRC = QCC_PR_LexInteger();

			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "merge", 5))
		{
			pr_file_p=directive+5;

			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			QCC_PR_SimpleGetString();
			externs->Printf("Merging from %s\n", pr_token);
			QCC_ImportProgs(pr_token);
			if (!*destfile && !destfile_explicit)
			{
				QCC_JoinPaths(destfile, sizeof(destfile), pr_token, compilingfile);
				externs->Printf("Outputfile: %s\n", destfile);
			}

			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "includelist", 11))
		{
			int defines=0;
			pr_file_p=directive+11;

			QCC_PR_SkipToEndOfLine(true);

			while(1)
			{
				QCC_PR_LexWhitespace(false);
				if (!flag_hashonly && QCC_PR_CheckCompConst())
				{
					defines++;
					continue;
				}
				if (!QCC_PR_SimpleGetToken())
				{
					if (!*pr_file_p)
					{
						if (defines>0)
						{
							defines--;
							QCC_PR_UnInclude();
							continue;
						}
						QCC_Error(ERR_EOF, "eof in includelist");
					}
					else
					{
						pr_file_p++;
						pr_source_line++;
					}
					continue;
				}
				if (!strcmp(pr_token, "#endlist"))
				{
					QCC_PR_SkipToEndOfLine(true);
					break;
				}

				if (pr_error_count)	//if we had an error, don't keep including more stuff that'll hide the actual error.
				{
					pr_file_p = "";
					QCC_PR_ParseError(0, NULL);
				}

				QCC_FindBestInclude(pr_token, compilingfile, true);

				if (*pr_file_p == '\r')
					pr_file_p++;

//				QCC_PR_SkipToEndOfLine(true);
			}
		}
		else if (!strncmp(directive, "include", 7))
		{
			char sm;

			pr_file_p=directive+7;

			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			msg[0] = '\0';
			if (*pr_file_p == '\"')
				sm = '\"';
			else if (*pr_file_p == '<')
				sm = '>';
			else
			{
				QCC_PR_ParseError(0, "Not a string literal (on a #include)");
				sm = 0;
			}
			pr_file_p++;
			a=0;
			while(*pr_file_p != sm)
			{
				if (*pr_file_p == '\n')
				{
					QCC_PR_ParseError(0, "#include continued over line boundry\n");
					break;
				}
				msg[a++] = *pr_file_p;
				pr_file_p++;
			}
			msg[a] = 0;
			pr_file_p++;

			if (pr_error_count)	//if we had an error, don't keep including more stuff that'll hide the actual error.
			{
				pr_file_p = "";
				QCC_PR_ParseError(0, NULL);
			}
			QCC_PR_SkipToEndOfLine(true);

			QCC_FindBestInclude(msg, compilingfile, false);
			QCC_PR_Precompiler();	//preprocessor directives normally require a leading newline to be considered a directive. we won't have one in the new file so lets just do it explicitly.
		}
		else if (!strncmp(directive, "datafile", 8))
		{
			pr_file_p=directive+8;

			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			QCC_PR_SimpleGetString();
			externs->Printf("Including datafile: %s\n", pr_token);
			QCC_AddFile(pr_token);

			pr_file_p++;

			for (a = 0; a < sizeof(msg)-1 && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];

			msg[a-1] = '\0';

			while(*pr_file_p != '\n' && *pr_file_p != '\0')	//read on until the end of the line
			{
				pr_file_p++;
			}
		}
		else if (!strncmp(directive, "output", 6))
		{
			pr_file_p=directive+6;

			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			QCC_PR_SimpleGetString();
			if (!destfile_explicit)
			{
				QCC_JoinPaths(destfile, sizeof(destfile), pr_token, compilingfile);
				externs->Printf("Outputfile: %s\n", pr_token);
			}

			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "pragma", 6))
		{
			pr_file_p=directive+6;
			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			qcc_token[0] = '\0';
			for(a = 0; *pr_file_p != '\n' && *pr_file_p != '\0'; pr_file_p++)	//read on until the end of the line
			{
				if ((*pr_file_p == ' ' || *pr_file_p == '\t'|| *pr_file_p == '(') && !*qcc_token)
				{
					msg[a] = '\0';
					strcpy(qcc_token, msg);
					a=0;
					if (*pr_file_p != '(')
						continue;
				}
				msg[a++] = *pr_file_p;
			}

			msg[a] = '\0';
			{
				char *end;
				for (end = msg + a-1; end>=msg && qcc_iswhite(*end); end--)
					*end = '\0';
			}

			if (!*qcc_token)
			{
				strcpy(qcc_token, msg);
				msg[0] = '\0';
			}

			{
				char *end;
				for (end = msg + a-1; end>=msg && qcc_iswhite(*end); end--)
					*end = '\0';
			}

			if (!QC_strcasecmp(qcc_token, "DONT_COMPILE_THIS_FILE"))
			{
				QCC_PR_LexWhitespace(false);
				while (*pr_file_p)
				{
					if (!qcc_iswhite(*pr_file_p))
						pr_file_p++;
					QCC_PR_LexWhitespace(false);
				}
			}
			else if (!QC_strcasecmp(qcc_token, "COPYRIGHT"))
			{
				char *e = strrchr(msg+1, '\"');
				if (*msg == '\"' && e && e != msg)
				{	//FIXME: handle \ns
					memmove(msg, msg+1, e-(msg+1));
					msg[e-(msg+1)] = 0;
				}
				if (!QC_strlcpy(QCC_copyright, msg, sizeof(QCC_copyright)))
					QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Copyright message is too long\n");
			}
			else if (!QC_strcasecmp(qcc_token, "compress"))
			{
				extern pbool compressoutput;
				compressoutput = atoi(msg);
			}
			else if (!QC_strcasecmp(qcc_token, "forcecrc"))
			{
				ForcedCRC = atoi(msg);
			}
			else if (!QC_strcasecmp(qcc_token, "framerate"))
			{
				qcc_framerate = atof(msg);
				if (qcc_framerate < 0)
					qcc_framerate = 0;
			}
			else if (!QC_strcasecmp(qcc_token, "once"))
			{
				struct qccincludeonced_s *onced = qccHunkAlloc(sizeof(*onced) + strlen(compilingfile));
				strcpy(onced->name, compilingfile);
				onced->next = qccincludeonced;
				qccincludeonced = onced;
			}
			else if (!QC_strcasecmp(qcc_token, "file"))
			{	//#pragma file(foobar.qc)
				if (!flag_nopragmafileline)
				{
					char *e;
					char *m = msg;
					if (*m == '(')
					{
						m++;
						e = strchr(m, ')');
						if (e)
							*e = 0;
					}

					s_filen = e = qccHunkAlloc(strlen(m)+1);
					strcpy(e, m);
					if (opt_filenames)
					{
						optres_filenames += strlen(m);
						s_filed = 0;
					}
					else
						s_filed = QCC_CopyString (m);
				}
			}
			else if (!QC_strcasecmp(qcc_token, "line"))
			{	//#pragma line(666)
				if (!flag_nopragmafileline)
				{
					char *m = msg;
					if (*m == '(')
						m++;
					pr_source_line = strtoul(m, &m, 0)-1;
				}
			}
			else if (!QC_strcasecmp(qcc_token, "includedir"))
			{
				char newinc[1024];
				int i;
				QCC_COM_Parse(msg);

				if (*qcc_token)
				{
					 i = qcc_token[strlen(qcc_token)-1];
					 if (i != '/' && i != '\\')
						 QC_strlcat(qcc_token, "/", sizeof(qcc_token));
				}

				QCC_JoinPaths(newinc, sizeof(newinc), qcc_token, compilingfile);
				QCC_PR_AddIncludePath(newinc);
			}
			else if (!QC_strcasecmp(qcc_token, "noref"))
				defaultnoref = !!atoi(msg);
			else if (!QC_strcasecmp(qcc_token, "nosave"))
				defaultnosave = !!atoi(msg);
			else if (!QC_strcasecmp(qcc_token, "defaultstatic"))
				defaultstatic = !!atoi(msg);
			else if (!QC_strcasecmp(qcc_token, "autoproto"))
			{
				if (!autoprototyped)
				{
					if (numpr_globals != RESERVED_OFS)
						QCC_PR_ParseWarning(WARN_BADPRAGMA, "#pragma autoproto must appear before any definitions");
					else
						autoprototype = *msg?!!atoi(msg):true;
				}
			}
			else if (!QC_strcasecmp(qcc_token, "wrasm"))
			{
				pbool on = atoi(msg);

				if (asmfile && !on)
				{
					fclose(asmfile);
					asmfile = NULL;
				}
				if (!asmfile && on)
				{
					if (asmfilebegun)
						asmfile = fopen("qc.asm", "ab");
					else
						asmfile = fopen("qc.asm", "wb");
					if (asmfile)
						asmfilebegun = true;
				}
			}
			else if (!QC_strcasecmp(qcc_token, "optimise") || !QC_strcasecmp(qcc_token, "optimize"))	//bloomin' americans.
			{
				int o;
				extern pbool qcc_nopragmaoptimise;
				if (pr_scope)
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s: unable to change optimisation options mid-function", qcc_token);
				else if (*msg >= '0' && *msg <= '3')
				{
					int lev = atoi(msg);
					pbool state;
					int once = false;
					for (o = 0; optimisations[o].enabled; o++)
					{
						state = optimisations[o].optimisationlevel <= lev;
						if (qcc_nopragmaoptimise && *optimisations[o].enabled != state)
						{
							if (!once++)
								QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s %s: overriden by commandline", qcc_token, msg);
						}
						else
							*optimisations[o].enabled = state;
					}
				}
				else if (!strnicmp(msg, "addon", 5) || !strnicmp(msg, "mutator", 7))
				{
					int lev = 2;
					pbool state = false;
					for (o = 0; optimisations[o].enabled; o++)
					{
						if (optimisations[o].optimisationlevel > lev)
						{
							if (qcc_nopragmaoptimise && *optimisations[o].enabled != state)
								QCC_PR_ParseWarning(WARN_IGNORECOMMANDLINE, "pragma %s %s: disabling %s", qcc_token, msg, optimisations[o].fullname);
							*optimisations[o].enabled = state;
						}
					}
				}
				else
				{
					char *opt = msg;
					pbool state = true;
					if (!strnicmp(msg, "no-", 3))
					{
						state = false;
						opt += 3;
					}
					for (o = 0; optimisations[o].enabled; o++)
						if ((*optimisations[o].abbrev && !stricmp(opt, optimisations[o].abbrev)) || !stricmp(opt, optimisations[o].fullname))
						{
							if (qcc_nopragmaoptimise && *optimisations[o].enabled != state)
								QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s %s: overriden by commandline", qcc_token, optimisations[o].fullname);
							else
								*optimisations[o].enabled = state;
							break;
						}
					if (!optimisations[o].enabled)
						QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s: %s unsupported", qcc_token, opt);
				}
			}
			else if (!QC_strcasecmp(qcc_token, "sourcefile"))
			{
				char *s = msg;
				while ((s = QCC_COM_Parse(s)))
					QCC_RegisterSourceFile(qcc_token);
			}
			else if (!QC_strcasecmp(qcc_token, "TARGET"))
			{
				QCC_COM_Parse(msg);
				if (!QCC_OPCodeSetTargetName(qcc_token))
					QCC_PR_ParseWarning(WARN_BADTARGET, "Unknown target \'%s\'. Ignored.\nValid targets are: ID, HEXEN2, FTE, FTEH2, KK7, DP(patched)", qcc_token);
			}
			else if (!QC_strcasecmp(qcc_token, "PROGS_SRC"))
			{	//doesn't make sense, but silenced if you are switching between using a certain precompiler app used with CuTF.
			}
			else if (!QC_strcasecmp(qcc_token, "PROGS_DAT"))
			{	//doesn't make sence, but silenced if you are switching between using a certain precompiler app used with CuTF.
				char olddest[1024];
				Q_strlcpy(olddest, destfile, sizeof(olddest));
				QCC_COM_Parse(msg);

				if (!destfile_explicit) //if output file is named on the commandline, don't change it mid-compile
					QCC_JoinPaths(destfile, sizeof(destfile), qcc_token, compilingfile);

				if (strcmp(destfile, olddest))
					externs->Printf("Outputfile: %s\n", destfile);
			}
			else if (!QC_strcasecmp(qcc_token, "opcode"))
			{
				int st;
				char *s = QCC_COM_Parse(msg);
				if (!QC_strcasecmp(qcc_token, "enable") || !QC_strcasecmp(qcc_token, "on"))
					st = 1;
				else if (!QC_strcasecmp(qcc_token, "disable") || !QC_strcasecmp(qcc_token, "off"))
					st = 0;
				else
				{
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "opcode state not recognised");
					st = -1;
				}

				if (st >= 0)
				{
					int f;
					while ((s = QCC_COM_Parse(s)))
					{
						for (f = 0; pr_opcodes[f].opname; f++)
						{
							if (!QC_strcasecmp(pr_opcodes[f].opname, qcc_token))
							{
								if (f >= OP_NUMREALOPS)
									QCC_PR_ParseWarning(WARN_BADPRAGMA, "opcode %s is internal", qcc_token);	//these will change with later opcodes, do not allow them to be written into the output.
								else if (st)
									pr_opcodes[f].flags |= OPF_VALID;
								else
									pr_opcodes[f].flags &= ~OPF_VALID;
								break;
							}
						}
						if (!pr_opcodes[f].opname)
							QCC_PR_ParseWarning(WARN_BADPRAGMA, "opcode %s not recognised", qcc_token);
					}
				}
			}
			else if (!QC_strcasecmp(qcc_token, "keyword") || !QC_strcasecmp(qcc_token, "flag"))
			{
				char *s;
				int st;
				s = QCC_COM_Parse(msg);
				if (!QC_strcasecmp(qcc_token, "enable") || !QC_strcasecmp(qcc_token, "on"))
					st = 1;
				else if (!QC_strcasecmp(qcc_token, "disable") || !QC_strcasecmp(qcc_token, "off"))
					st = 0;
				else
				{
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "compiler flag state not recognised");
					st = -1;
				}
				if (st >= 0)
				{
					int f;
					while ((s = QCC_COM_Parse(s)))
					{
						for (f = 0; compiler_flag[f].enabled; f++)
						{
							if (!QC_strcasecmp(compiler_flag[f].abbrev, qcc_token))
							{
								if (compiler_flag[f].flags & FLAG_MIDCOMPILE)
								{
									*compiler_flag[f].enabled = st;
									if (compiler_flag[f].enabled == &flag_cpriority)
										QCC_PrioritiseOpcodes();
								}
								else
									QCC_PR_ParseWarning(WARN_BADPRAGMA, "Cannot enable/disable keyword/flag via a pragma");
								break;
							}
						}
						if (!compiler_flag[f].enabled)
							QCC_PR_ParseWarning(WARN_BADPRAGMA, "keyword/flag %s not recognised", qcc_token);
					}
				}
			}
			else if (!QC_strcasecmp(qcc_token, "warning"))
			{
				int st;
				char *s;
				s = QCC_COM_Parse(msg);
				if (!stricmp(qcc_token, "enable") || !stricmp(qcc_token, "on"))
					st = WA_WARN;
				else if (!stricmp(qcc_token, "disable") || !stricmp(qcc_token, "off") || !stricmp(qcc_token, "ignore"))
					st = WA_IGNORE;
				else if (!stricmp(qcc_token, "error"))
					st = WA_ERROR;
				else if (!stricmp(qcc_token, "toggle"))
					st = 3;
				else
				{
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "warning state not recognised");
					st = -1;
				}
				if (st>=0)
				{
					int wn;
					while ((s = QCC_COM_Parse(s)))
					{
						wn = QCC_WarningForName(qcc_token);
						if (wn < 0)
							QCC_PR_ParseWarning(WARN_BADPRAGMA, "warning id not recognised");
						else
						{
							if (st == 3)	//toggle
								qccwarningaction[wn] = !!qccwarningaction[wn];
							else
								qccwarningaction[wn] = st;
						}
					}
				}
			}
			else
			{
				QCC_PR_SkipToEndOfLine(false);
				QCC_PR_ParseWarning(WARN_BADPRAGMA, "Unknown pragma \'%s\'", qcc_token);
			}

			QCC_PR_SkipToEndOfLine(true);
		}
		return true;
	}

	return false;
}

/*
==============
PR_NewLine

Call at start of file and when *pr_file_p == '\n'
==============
*/
void QCC_PR_NewLine (pbool incomment)
{
	pr_source_line++;
	pr_line_start = pr_file_p;
	while(*pr_file_p==' ' || *pr_file_p == '\t')
		pr_file_p++;
	if (incomment)	//no constants if in a comment.
	{
	}
	else if (QCC_PR_Precompiler())
	{
	}

//	if (pr_dumpasm)
//		PR_PrintNextLine ();
}

/*
==============
PR_LexString

Parses a quoted string
==============
*/
int QCC_PR_LexEscapedCodepoint(void)
{	//for "\foo" or '\foo' handling.
	//caller will have read the \ already.
	int t;
	int c = *pr_file_p++;
	if (!c)
		QCC_PR_ParseError (ERR_EOF, "EOF inside quote");
	if (c == 'n')
		c = '\n';
	else if (c == 'r')
		c = '\r';
	else if (c == '#')	//avoid preqcc expansion in strings.
		c = '#';
	else if (c == '"')
		c = '"';
	else if (c == 't')
		c = '\t';	//tab
	else if (c == 'a')
		c = '\a';	//bell
	else if (c == 'v')
		c = '\v';	//vertical tab
	else if (c == 'f')
		c = '\f';	//form feed
//	else if (c == 's' || c == 'b')
//		c = 0;	//invalid...
	//else if (c == 'b')
	//	c = '\b';
	else if (c == '[')
		c = 0xe010;	//quake specific
	else if (c == ']')
		c = 0xe011;	//quake specific
	else if (c == '{')
	{
		int d;
		c = 0;
		if (*pr_file_p == 'x')
		{
			pr_file_p++;
			while ((d = *pr_file_p++) != '}')
			{
				if (d >= '0' && d <= '9')
					c = c * 16 + d - '0';
				else if (d >= 'a' && d <= 'f')
					c = c * 16 + 10+d - 'a';
				else if (d >= 'A' && d <= 'F')
					c = c * 16 + 10+d - 'A';
				else
					QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Bad character code");
			}
		}
		else
		{
			while ((d = *pr_file_p++) != '}')
			{
				if (d >= '0' && d <= '9')
					c = c * 10 + d - '0';
				else
					QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Bad character code");
			}
		}
	}
	else if (c == '.')
		c = 0xe01c;
	else if (c == '<')
		c = 0xe01d;	//separator start
	else if (c == '-')
		c = 0xe01e;	//separator middle
	else if (c == '>')
		c = 0xe01f;	//separator end
	else if (c == '(')
		c = 0xe080;	//slider start
	else if (c == '=')
		c = 0xe081;	//slider middle
	else if (c == ')')
		c = 0xe082;	//slider end
	else if (c == '+')
		c = 0xe083;	//slider box
	else if (c == 'u' || c == 'U')
	{
		//lower case u specifies exactly 4 nibbles.
		//upper case U specifies exactly 8 nibbles.
		unsigned int nibbles = (c=='u')?4:8;
		c = 0;
		while (nibbles --> 0)
		{
			t = (unsigned char)*pr_file_p;
			if (t >= '0' && t <= '9')
				c = (c*16) + (t - '0');
			else if (t >= 'A' && t <= 'F')
				c = (c*16) + (t - 'A') + 10;
			else if (t >= 'a' && t <= 'f')
				c = (c*16) + (t - 'a') + 10;
			else
				break;
			pr_file_p++;
		}
		if (nibbles)
			QCC_PR_ParseWarning(ERR_BADCHARACTERCODE, "Unicode character terminated unexpectedly");
	}
	else if (c == 'x' || c == 'X')
	{
		int d;
		c = 0;

		d = (unsigned char)*pr_file_p++;
		if (d >= '0' && d <= '9')
			c += d - '0';
		else if (d >= 'A' && d <= 'F')
			c += d - 'A' + 10;
		else if (d >= 'a' && d <= 'f')
			c += d - 'a' + 10;
		else
			QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Bad character code");

		c *= 16;

		d = (unsigned char)*pr_file_p++;
		if (d >= '0' && d <= '9')
			c += d - '0';
		else if (d >= 'A' && d <= 'F')
			c += d - 'A' + 10;
		else if (d >= 'a' && d <= 'f')
			c += d - 'a' + 10;
		else
		{	//oops. only one char valid...
			c >>= 4;
			pr_file_p --;
		}
	}
	else if (c == '\\')
		c = '\\';
	else if (c == '?')	//triglyphs are dumb.
		c = '?';
	else if (c == '\'')
		c = '\'';
	else if (c >= '0' && c <= '9')
	{
		if (flag_qcfuncs)
			c = 0xe012 + c - '0';	//WARNING: This is not an octal code, but uses 'yellow' numbers instead (as on hud).
		else	//C compat
		{
			int d = c, base = 8;
			c = 0;

			if (d >= '0' && d < '0'+base)
				c = c*base + (d - '0');
			else
				QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Bad character code \\%c", d);

			d = (unsigned char)*pr_file_p++;
			if (d >= '0' && d < '0'+base)
				c = c*base + (d - '0');
			else
				pr_file_p --;	//oops. only one char valid...

			d = (unsigned char)*pr_file_p++;
			if (d >= '0' && d < '0'+base)
				c = c*base + (d - '0');
			else
				pr_file_p --;	//oops. only twoish chars valid...
		}
	}
	else if (c == '\r')
	{	//sigh
		c = *pr_file_p++;
		if (c != '\n')
			QCC_PR_ParseWarning(WARN_HANGINGSLASHR, "Hanging \\\\\r");
		pr_source_line++;
	}
	else if (c == '\n')
	{	//sigh
		pr_source_line++;
	}
	else
		QCC_PR_ParseError (ERR_INVALIDSTRINGIMMEDIATE, "Unknown escape char %c", c);

	return c;
}
void QCC_PR_LexString (void)
{
	unsigned int	c, t;
	int bytecount;
	int		len = 0;
	char	*end;
	const char *cnst;
	int		raw;
	char	rawdelim[64];
	int		stringtype;
			//0 - quake output, input is 8bit. warnings when its not ascii. \u will still give utf-8 text, other chars as-is. Expect \s to screw everything up with utf-8 output.
			//1 - quake output, input is utf-8. due to editors not supporting it, that generally means the input (ab)uses markup.
			//2 - utf-8 output, input is utf-8. welcome to the future! unfortunately not the present.

	int texttype;
	pbool first = true;

	for(;;)
	{
		raw = 0;
		texttype = 0;

		QCC_PR_LexComment(&pr_token_precomment);

		if (flag_qccx && *pr_file_p == ':')
		{
			pr_file_p++;
			pr_token[len++] = 0;
			continue;
		}

		if (*pr_file_p == 'R' && pr_file_p[1] == '\"')
		{
			/*R"delim(fo
			o)delim" -> "fo\no"
			the []
			*/
			raw = 1;
			pr_file_p+=2;

			while (1)
			{
				c = *pr_file_p++;
				if (c == '(')
				{
					rawdelim[0] = ')';
					break;
				}
				if (!c || raw >= sizeof(rawdelim)-1)
					QCC_PR_ParseError (ERR_EOF, "EOF while parsing raw string delimiter. Expected: R\"delim(string)delim\"");
				rawdelim[raw++] = c;
			}
			rawdelim[raw++] = '\"';

			//these two conditions are generally part of the C preprocessor.
			if (!strncmp(pr_file_p, "\\\r\n", 3))
			{	//dos format
				pr_file_p += 3;
				pr_source_line++;
			}
			else if (!strncmp(pr_file_p, "\\\r", 2) || !strncmp(pr_file_p, "\\\n", 2))
			{	//mac + unix format
				pr_file_p += 2;
				pr_source_line++;
			}
			stringtype = 0;
		}
		else if (*pr_file_p == 'Q' && pr_file_p[1] == '\"')
		{	//quake output with utf-8 input (expect to need markup).
			stringtype = 1;
			pr_file_p+=2;
		}
		else if ((*pr_file_p == 'U' || *pr_file_p == 'u' || *pr_file_p == 'L') && pr_file_p[1] == '\"')
		{	//unicode string, char32_t, char16_t, wchar_t respectively. we spit out utf-8 regardless.
			QCC_PR_ParseWarning(WARN_NOTUTF8, "char32_t/char16_t/wchar_t strings are not supported, treating as u8 prefix (as utf-8)");
			stringtype = 2;
			pr_file_p+=2;
		}
		else if (*pr_file_p == 'u' && pr_file_p[1] == '8' && pr_file_p[2] == '\"')
		{	//utf-8 string.
			stringtype = 2;
			pr_file_p+=3;
		}
		else if (*pr_file_p == '\"')
		{
			stringtype = flag_utf8strings?2:0;
			pr_file_p++;
		}
		else if (first)
		{
			stringtype = 0;
			QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Expected string constant");
		}
		else
			break;
		first = false;

		for(;;)
		{
			c = *pr_file_p++;
			if (!c)
				QCC_PR_ParseError (ERR_EOF, "EOF inside quote");

			if (raw)
			{
				//raw strings contain very little parsing. just delimiter and initial \NL support.
				if (c == rawdelim[0] && !strncmp(pr_file_p, rawdelim+1, raw-1))
				{
					pr_file_p += raw-1;
					break;
				}

				//make sure line numbers are correct though.
				if (c == '\r' && *pr_file_p != '\n')
					pr_source_line++;	//mac
				if (c == '\n')	//dos/unix
					pr_source_line++;
				goto forcebyte;
			}
			else
			{
				if (c=='\n')
					QCC_PR_ParseError (ERR_INVALIDSTRINGIMMEDIATE, "newline inside quote");
				if (c=='\\')
				{	// escape char
					c = *pr_file_p;	//peek at it, for our hacks.
					if (c == 's' || c == 'b')
					{
						pr_file_p++;
						texttype ^= 0xe080;
						continue;
					}
					else if (c == '.')
					{
						pr_file_p++;
						c = 0xe01c | texttype;
					}
					else if (c == 'u' || c == 'U')
					{	//special hack, \u is a utf-8 code regardless of output encoding...
						c = QCC_PR_LexEscapedCodepoint();
						goto forceutf8;
					}
					else if (c == 'x' || c == 'X')
					{	//special hack, \xXX in a string is an explicit byte regardless of encoding.
						c = QCC_PR_LexEscapedCodepoint();
//						if (c > 0xff)
//							QCC_PR_ParseWarning(ERR_BADCHARACTERCODE, "Bad unicode character code - codepoint %#x is above 0xFF", c);
						goto forcebyte;
					}
					else
					{
						c = QCC_PR_LexEscapedCodepoint();
//						if (stringtype != 2 && c > 0xff)
//							QCC_PR_ParseWarning(ERR_BADCHARACTERCODE, "Bad legacy character code - codepoint %#x is above 0xFF", c);
					}
				}
				else if (c=='\"')
				{
					break;
				}
				else if (c == '#' && flag_macroinstrings)
				{
					for (end = pr_file_p; ; end++)
					{
						if (qcc_iswhite(*end))
							break;

						if (*end == ')'
							||	*end == '('
							||	*end == '+'
							||	*end == '-'
							||	*end == '*'
							||	*end == '/'
							||	*end == '\\'
							||	*end == '|'
							||	*end == '&'
							||	*end == '='
							||	*end == '^'
							||	*end == '~'
							||	*end == '['
							||	*end == ']'
							||	*end == '\"'
							||	*end == '{'
							||	*end == '}'
							||	*end == ';'
							||	*end == ':'
							||	*end == ','
							||	*end == '.'
							||	*end == '#')
								break;
					}

					c = *end;
					*end = '\0';
					cnst = QCC_PR_CheckCompConstString(pr_file_p);
					if (cnst==pr_file_p)
					{
						if (*pr_file_p)
							QCC_PR_ParseWarning(WARN_MACROINSTRING, "Unable to expand string macro %s", pr_file_p);

					}
					else if (cnst)
					{
						QCC_PR_ParseWarning(WARN_MACROINSTRING, "Macro %s expansion in string", pr_file_p);
						*end = c;

						if (len+strlen(cnst) >= sizeof(pr_token)-1)
							QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %u", (unsigned)sizeof(pr_token)-1);

						strcpy(pr_token+len, cnst);
						bytecount=strlen(cnst);
						while (bytecount > 0 && qcc_iswhitesameline(pr_token[len+bytecount-1]))
							bytecount--;	//make sure there's no trailing whitespace on the end.
						len+=bytecount;
						pr_file_p = end;
						continue;
					}
					*end = c;
					c = '#';	//undo
				}
				else if (c == 0x7C && flag_acc)	//reacc support... reacc is strange.
					c = '\n';
				else
				{
					unsigned int cp = c;
					unsigned int len = stringtype?utf8_check(pr_file_p-1, &cp):0;
					if (!len)
					{	//invalid utf-8 encoding? don't treat it as utf-8!
						if (stringtype)
							QCC_PR_ParseWarning(ERR_BADCHARACTERCODE, "Input string is not valid utf-8");
						if (c >= ' ')
							c |= texttype;
						goto forcebyte;
					}
					if (texttype)
					{
						if (cp < ' ')
							c = cp;	//don't mask C0 chars like \t or \n
						else if (cp < 0x80)
							c = cp|0xe080;	//DO mask other ascii chars (and map to the private-use range at the same time, because this isn't standard unicode any more)
						else
						{
							QCC_PR_ParseWarning(ERR_BADCHARACTERCODE, "Unable to mask non-ascii chars. Attempting to mask bytes");
							c |= texttype;
							goto forcequake;
						}
					}
					else
						c = cp;
					pr_file_p += len-1;
				}
			}

//			if (c >= 0x20 && c < 0x80)
//				c |= 0xe000;	//TEST
			if (stringtype == 2)
			{	//we're outputting a utf-8 string.
forceutf8:
				if (c > 0x10FFFFu)	//RFC 3629 imposes the same limit as UTF-16 surrogate pairs.
					QCC_PR_ParseWarning(WARN_NOTUTF8, "Bad unicode character code - codepoint is above 0x10FFFFu");

				//figure out the count of bytes required to encode this char
				bytecount = 1;
				t = 0x80;
				while (c >= t)
				{
					if (bytecount == 1)
						t <<= 4;
					else if (bytecount < 7)
						t <<= 5;
					else
						t <<= 6;
					bytecount++;
				}

				//error if needed
				if (len+bytecount >= sizeof(pr_token))
					QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %u", (unsigned)sizeof(pr_token)-1);

				//output it.
				if (bytecount == 1)
					pr_token[len++] = (unsigned char)(c&0x7f);
				else
				{
					t = bytecount*6;
					t = t-6;
					pr_token[len++] = (unsigned char)((c>>t)&(0x0000007f>>bytecount)) | (0xffffff00 >> bytecount);
					do
					{
						t = t-6;
						pr_token[len++] = (unsigned char)((c>>t)&0x3f) | 0x80;
					} while(t);
				}
			}
			else
			{
forcequake:
				//we need to convert it to a quake char...
				if (c >= 0xe000 && c <= 0xe0ff)
					c = c & 0xff;	//this private use range is commonly used for quake's glyphs.
				else if (c >= 0 && c <= 0x7f)
					; //FIXME: SOME c0 codes are known to quake, but many got reused for random glyphs. however I'm going to treat quake as full ascii.
				else if (c >= 0x80)
				{
					//FIXME: spit it out as ^{xxxxxx} instead
					QCC_PR_ParseWarning(WARN_NOTUTF8, "Cannot convert codepoint %#x to quake's charset", c);
				}

forcebyte:
				if (len >= sizeof(pr_token)-1)
					QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %u", (unsigned)sizeof(pr_token)-1);
				pr_token[len] = c;
				len++;
			}
		}
	}

	if (len > sizeof(pr_immediate_string)-1)
		QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %u", (unsigned)sizeof(pr_immediate_string)-1);

	pr_token[len] = 0;
	pr_token_type = tt_immediate;
	pr_immediate_type = type_string;
	memcpy(pr_immediate_string, pr_token, len+1);
	pr_immediate_strlen = len;

	/*if (qccwarningaction[WARN_NOTUTF8] && stringtype != 1)
	{
		unsigned int		code;
		size_t c;
		for (c = 0; c < pr_immediate_strlen; )
		{
			len = utf8_check(&pr_token[c], &code);
			if (!len || c+len>pr_immediate_strlen)
			{
				QCC_PR_ParseWarning(WARN_NOTUTF8, "String literal is not valid utf-8");
				break;
			}
			c += len;
		}
	}*/
}

/*
==============
PR_LexNumber
==============
*/
int QCC_PR_LexInteger (void)
{
	int		c;
	int		len;

	len = 0;
	c = *pr_file_p;
	if (pr_file_p[0] == '0' && (pr_file_p[1] == 'x' || pr_file_p[1] == 'X'))
	{
		pr_token[0] = '0';
		pr_token[1] = 'x';
		len = 2;
		c = *(pr_file_p+=2);
	}
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while ((c >= '0' && c<= '9') || (c == '.'&&pr_file_p[1]!='.') || (c>='a' && c <= 'f'));
	pr_token[len] = 0;
	return atoi (pr_token);
}

#ifdef _MSC_VER
#define longlong __int64
#define LL(x) x##i64
#else
#define longlong long long
#define LL(x) x##ll
#endif

static void QCC_PR_LexNumber (void)
{
	int tokenlen = 0;
	longlong num=0;
	int base=0;
	int c;
	int sign=1;
	if (*pr_file_p == '-')
	{
		sign=-1;
		pr_file_p++;

		pr_token[tokenlen++] = '-';
	}
	if (pr_file_p[0] == '0' && (pr_file_p[1] == 'x' || pr_file_p[1] == 'X'))
	{
		pr_file_p+=2;
		base = 16;

		pr_token[tokenlen++] = '0';
		pr_token[tokenlen++] = 'x';
	}
	else if (pr_file_p[0] == '0')
	{
		pr_file_p++;
		if (*pr_file_p >= '0' && *pr_file_p <= '9')
			QCC_PR_ParseWarning(WARN_OCTAL_IMMEDIATE, "A leading 0 is interpreted as base-8.");
		base = 8;

		pr_token[tokenlen++] = '0';
	}

	pr_immediate_type = NULL;
	//assume base 10 if not stated
	if (!base)
		base = 10;

	while((c = *pr_file_p))
	{
		if (c >= '0' && c <= '9' && c < '0'+base)
		{
			pr_token[tokenlen++] = c;
			num*=base;
			num += c-'0';
		}
		else if (c >= 'a' && c <= 'z' && c < 'a'+base-10)
		{
			pr_token[tokenlen++] = c;
			num*=base;
			num += c -'a'+10;
		}
		else if (c >= 'A' && c <= 'Z' && c < 'A'+base-10)
		{
			pr_token[tokenlen++] = c;
			num*=base;
			num += c -'A'+10;
		}
		else if (c == '.' && pr_file_p[1]!='.')
		{
			pr_token[tokenlen++] = c;
			pr_file_p++;
			pr_immediate_type = flag_assume_double?type_double:type_float;
			while(1)
			{
				c = *pr_file_p;
				if (c >= '0' && c <= '9')
				{
					pr_token[tokenlen++] = c;
				}
				else if (c == 'f' || c == 'F')
				{
					pr_immediate_type = type_float;
					pr_file_p++;
					break;
				}
				else if (c == 'd' || c == 'D')
				{
					pr_immediate_type = type_double;
					pr_file_p++;
					break;
				}
				else
				{
					break;
				}
				pr_file_p++;
			}
			pr_token[tokenlen++] = 0;
			if (pr_immediate_type == type_double)
				pr_immediate._double = atof(pr_token);
			else
				pr_immediate._float = (float)atof(pr_token);
			goto checkjunk;
		}
		else if (c == 'f' || c == 'F')
		{
			pr_token[tokenlen++] = c;
			pr_token[tokenlen++] = 0;
			pr_file_p++;
			pr_immediate_type = type_float;
			pr_immediate._float = num*sign;

			num*=sign;
			if ((longlong)pr_immediate._float != (longlong)num)
				QCC_PR_ParseWarning(WARN_OVERFLOW, "numerical overflow");
			goto checkjunk;
		}
		else if (c == 'd' || c == 'D')
		{	//note: conflicts with hex. add a dot before it or something.
			pr_token[tokenlen++] = c;
			pr_token[tokenlen++] = 0;
			pr_file_p++;
			pr_immediate_type = type_double;
			pr_immediate._double = num*sign;

			num*=sign;
			if ((longlong)pr_immediate._double != (longlong)num)
				QCC_PR_ParseWarning(WARN_OVERFLOW, "numerical overflow");
			goto checkjunk;
		}
		else if (c == 'i' || c == 'u' || c == 'l' || c == 'I' || c == 'U' || c == 'L')
		{	//length and sign flags can be any order. LL suffix must have the same case (but not necessarily match the sign suffix)
			int isunsigned;
			int islong = (c == 'l')||(c == 'L');
			pr_token[tokenlen++] = c;
			pr_file_p++;
			if (islong)
			{	//length suffix was first.
				//long-long?
				if (*pr_file_p == c)
					islong++, pr_token[tokenlen++] = *pr_file_p++;
				//check for signed suffix...
				c = *pr_file_p;
				isunsigned = (c == 'u')||(c=='U');
				if (c == 'i' || c == 'I' || isunsigned)	//ignore an explicit redundant 'i' char, for not-a-float.
					pr_token[tokenlen++] = *pr_file_p++;
			}
			else
			{
				isunsigned = (c == 'u')||(c=='U');
				//we already made sure it u or i, and its not an l
				c = *pr_file_p;
				if (c == 'l' || c == 'L')
				{
					pr_token[tokenlen++] = *pr_file_p++;
					islong = true;
					//long-long?
					if (*pr_file_p == c)
						islong++, pr_token[tokenlen++] = *pr_file_p++;
				}
			}
			pr_token[tokenlen++] = 0;
			num *= sign;
			if (islong >= (flag_ILP32?2:1))
			{	//enough longs for our 64bit type.
				pr_immediate_type = (isunsigned)?type_uint64:type_int64;
				pr_immediate.i64 = num;
			}
			else
			{
				pr_immediate_type = (isunsigned)?type_uint:type_integer;
				pr_immediate._int = num;

				if ((longlong)pr_immediate._int != (longlong)num)
				{
					if (((longlong)pr_immediate._int & LL(0xffffffff80000000)) != LL(0xffffffff80000000))
						QCC_PR_ParseWarning(WARN_OVERFLOW, "numerical overflow");
				}
			}
			goto checkjunk;
		}
		else
			break;
		pr_file_p++;
	}
	pr_token[tokenlen++] = 0;

	if (!pr_immediate_type)
	{
		//float f = num;
		if (flag_assume_integer)// || (base != 10 && sign > 0 && (long long)f != (long long)num))
		{
			if (num > UINT64_C(0x7fffffffffffffff))
				pr_immediate_type = type_uint64;
			else if (num > 0xffffffffu)
				pr_immediate_type = type_int64;
			else if (num > 0x7fffffffu)
				pr_immediate_type = type_uint;
			else
				pr_immediate_type = type_integer;
		}
		else if (flag_qccx && base == 16)
		{
			pr_immediate_type = type_float;
			goto qccxhex;
		}
		else
			pr_immediate_type = type_float;
	}

	if (pr_immediate_type == type_int64 || pr_immediate_type == type_uint64)
		pr_immediate.i64 = num*sign;
	else if (pr_immediate_type == type_integer || pr_immediate_type == type_uint)
	{
qccxhex:
		pr_immediate._int = num*sign;

		num*=sign;
		if ((longlong)pr_immediate._int != (longlong)num)
		{
			if (((longlong)pr_immediate._int & LL(0xffffffff80000000)) != LL(0xffffffff80000000))
					QCC_PR_ParseWarning(WARN_OVERFLOW, "numerical overflow");
		}
	}
	else
	{
		pr_immediate_type = type_float;
		// at this point, we know there's no . in it, so the NaN bug shouldn't happen
		// and we cannot use atof on tokens like 0xabc, so use num*sign, it SHOULD be safe
		//pr_immediate._float = atof(pr_token);
		pr_immediate._float = (float)(num*sign);

		num*=sign;
		if ((longlong)pr_immediate._float != (longlong)num && base == 16)
			QCC_PR_ParseWarning(WARN_OVERFLOW, "numerical overflow %lld will be rounded to %f", num, pr_immediate._float);
	}

checkjunk:
	c = *pr_file_p;
	if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9') || (c & 0x80))
		QCC_PR_ParseWarning(ERR_NOTANUMBER, "bad suffix on number %s", pr_token);
}


static float QCC_PR_LexFloat (void)
{
	int		c;
	int		len;

	len = 0;
	c = *pr_file_p;
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while ((c >= '0' && c<= '9') || (c == '.'&&pr_file_p[1]!='.'));	//only allow a . if the next isn't too...
	if (*pr_file_p == 'f')
		pr_file_p++;
	pr_token[len] = 0;
	return (float)atof (pr_token);
}

/*
==============
PR_LexVector

Parses a single quoted vector
==============
*/
static void QCC_PR_LexVector (void)
{
	int		i;

	pr_file_p++;	//skip the leading ' char

	if (*pr_file_p == '\\')
	{//extended character constant
		pr_file_p++;
		pr_token_type = tt_immediate;
		if (flag_assume_integer)
		{
			pr_immediate_type = type_integer;
			pr_immediate._int = QCC_PR_LexEscapedCodepoint();
		}
		else
		{
			pr_immediate_type = type_float;
			pr_immediate._float = QCC_PR_LexEscapedCodepoint();
		}
		if (*pr_file_p != '\'')
			QCC_PR_ParseError (ERR_INVALIDVECTORIMMEDIATE, "Bad character constant");
		pr_file_p++;
		return;
	}
	if ((unsigned char)*pr_file_p >= 0x80)
	{
		int b = utf8_check(pr_file_p, &pr_immediate._int);	//utf-8 codepoint.
		pr_token_type = tt_immediate;
		if (flag_assume_integer)
			pr_immediate_type = type_integer;
		else
		{
			pr_immediate_type = type_float;
			if (flag_qccx)
				QCC_PR_ParseWarning(WARN_DENORMAL, "char constant: denormal");
			else
				pr_immediate._float = pr_immediate._int;
		}
		pr_file_p+=b+1;
		return;
	}
	else if (pr_file_p[1] == '\'')
	{//character constant
		pr_token_type = tt_immediate;
		if (flag_assume_integer)
		{
			pr_immediate_type = type_integer;
			pr_immediate._int = (unsigned char)pr_file_p[0];
		}
		else
		{
			pr_immediate_type = type_float;
			if (flag_qccx)
			{
				QCC_PR_ParseWarning(WARN_DENORMAL, "char constant: denormal");
				pr_immediate._int = pr_file_p[0];
			}
			else
				pr_immediate._float = pr_file_p[0];
		}
		pr_file_p+=2;
		return;
	}
	pr_token_type = tt_immediate;
	pr_immediate_type = type_vector;
	QCC_PR_LexWhitespace (false);
	for (i=0 ; i<3 ; i++)
	{
		pr_immediate.vector[i] = QCC_PR_LexFloat ();
		QCC_PR_LexWhitespace (false);

		if (*pr_file_p == '\'' && i == 1)
		{
			if (i < 2)
				QCC_PR_ParseWarning (WARN_FTE_SPECIFIC, "2d vector");

			for (i++ ; i<3 ; i++)
				pr_immediate.vector[i] = 0;
			break;
		}
	}
	if (*pr_file_p != '\'')
		QCC_PR_ParseError (ERR_INVALIDVECTORIMMEDIATE, "Bad vector");
	pr_file_p++;
}

/*
==============
PR_LexName

Parses an identifier
==============
*/
static void QCC_PR_LexName (void)
{
	unsigned int		c;
	int		len;

	len = 0;
	do
	{
		int b = utf8_check(pr_file_p, &c);
		if (!b)
		{
			unsigned char lead = *pr_file_p++;
			char *o;
			while(*pr_file_p && !utf8_check(pr_file_p, &c))
				pr_file_p++;
			o = pr_file_p;
			while (qcc_iswhite(*pr_file_p))
			{
				if (*pr_file_p == '\n')
					break;
				pr_file_p++;
			}
			if (*pr_file_p == '\n')
				QCC_PR_ParseError(ERR_NOTANAME, "Invalid UTF-8 code sequence at end of line. Lead byte was %#2x", lead);
			else
			{
				len = 0;
				while (*pr_file_p && !qcc_iswhite(*pr_file_p))
					pr_token[len++] = *pr_file_p++;
				pr_token[len++] = 0;
				pr_file_p = o;
				QCC_PR_ParseError(ERR_NOTANAME, "Invalid UTF-8 code sequence before %s. Lead byte was %#2x", pr_token, lead);
			}
			return;
		}
		while(b-->0)
		{
			pr_token[len] = *pr_file_p++;
			len++;
		}
		c = *pr_file_p;
	} while ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
	|| (c >= '0' && c <= '9') || (c & 0x80));

	pr_token[len] = 0;
	pr_token_type = tt_name;
}

/*
==============
PR_LexPunctuation
==============
*/
static void QCC_PR_LexPunctuation (void)
{
	int		i;
	int		len;
	char	*p;

	pr_token_type = tt_punct;

	if (pr_file_p[0] == '*' && pr_file_p[1] == '*' && flag_dblstarexp)
	{	//for compat with gmqcc. fteqcc uses *^ internally (which does not conflict with multiplying by dereferenced pointers - sucks for MSCLR c++ syntax)
		QCC_PR_ParseWarning(WARN_GMQCC_SPECIFIC, "** is unsafe around pointers, use *^ instead.");
		strcpy (pr_token, "*^");
		pr_file_p += 2;
		return;
	}

	for (i=0 ; (p = pr_punctuation[i]) != NULL ; i++)
	{
		len = strlen(p);
		if (!strncmp(p, pr_file_p, len) )
		{
			strcpy (pr_token, pr_punctuationremap[i]);
			if (p[0] == '{')
				pr_bracelevel++;
			else if (p[0] == '}')
				pr_bracelevel--;
			pr_file_p += len;
			return;
		}
	}

	if ((unsigned char)*pr_file_p == (unsigned char)'\\' && pr_file_p[1] == '\r' && pr_file_p[2] == '\n')
		pr_file_p+=3;
	else if ((unsigned char)*pr_file_p == (unsigned char)'\\' && (pr_file_p[1] == '\r' || pr_file_p[1] == '\n'))
		pr_file_p+=2;
	else
	{
		if ((unsigned char)*pr_file_p == (unsigned char)0xa0)
			QCC_PR_ParseWarning (ERR_UNKNOWNPUCTUATION, "Unsupported punctuation: '\\x%x' - non-breaking space", (unsigned char)*pr_file_p);
		else
			QCC_PR_ParseWarning (ERR_UNKNOWNPUCTUATION, "Unknown punctuation: '\\x%x'", *pr_file_p);
		pr_file_p++;
	}

	QCC_PR_Lex();
}


/*
==============
PR_LexWhitespace
==============
*/
void QCC_PR_LexWhitespace (pbool inhibitpreprocessor)
{
	int		c;

	while (1)
	{
	// skip whitespace
		while ((c = *pr_file_p) && qcc_iswhite(c))
		{
			if (qcc_islineending(c, pr_file_p[1]))
			{
				pr_file_p++;
				if (!inhibitpreprocessor)
					QCC_PR_NewLine (false);
				else
					pr_source_line++;
				if (!pr_file_p)
					return;
			}
			else
				pr_file_p++;
		}
		if (c == 0)
			return;		// end of file

	// skip // comments
		if (c=='/' && pr_file_p[1] == '/')
		{
			while (*pr_file_p && !qcc_islineending(pr_file_p[0], pr_file_p[1]))
				pr_file_p++;

			if (*pr_file_p)
				pr_file_p++;	//don't break on eof.
			if (!inhibitpreprocessor)
				QCC_PR_NewLine(false);
			else
				pr_source_line++;
			continue;
		}

	// skip /* */ comments
		if (c=='/' && pr_file_p[1] == '*')
		{
			pr_file_p+=1;
			do
			{
				pr_file_p++;
				if (qcc_islineending(pr_file_p[0], pr_file_p[1]))
				{
					if (!inhibitpreprocessor)
						QCC_PR_NewLine(true);
					else
						pr_source_line++;
				}
				if (pr_file_p[1] == 0)
				{
					QCC_PR_ParseError(0, "EOF inside comment\n");
					pr_file_p++;
					return;
				}
				if (pr_file_p[0] == '/' && pr_file_p[1] == '*')
					QCC_PR_ParseWarning(WARN_NESTEDCOMMENT, "\"/*\" inside comment");
			} while (pr_file_p[0] != '*' || pr_file_p[1] != '/');
			pr_file_p+=2;
			continue;
		}

		break;		// a real character has been found
	}
}

//============================================================================

#define	MAX_FRAMES	8192
char	pr_framemodelname[64];
struct
{
	char name[64];
	int value;
	const char *file;	//compare to s_filen to see if its current or not
} pr_framemacro[MAX_FRAMES];
int		pr_nummacros;
int		pr_macrovalue;	//next value to use
int		pr_savedmacro;	//for sub-groups.

void QCC_PR_ClearGrabMacros (pbool newfile)
{
	if (!newfile)
		pr_nummacros = 0;
	pr_macrovalue = 0;
	pr_savedmacro = -1;
}

static int QCC_PR_FindMacro (char *name)
{
	int		i;

	for (i=pr_nummacros-1 ; i>=0 ; i--)
	{
		if (!STRCMP (name, pr_framemacro[i].name))
		{
			if (pr_framemacro[i].file != s_filen)
				QCC_PR_ParseWarning(WARN_STALEMACRO, "Stale macro used (%s, defined in %s)", pr_token, pr_framemacro[i].file);
			return pr_framemacro[i].value;
		}
	}
	for (i=pr_nummacros-1 ; i>=0 ; i--)
	{
		if (!stricmp (name, pr_framemacro[i].name))
		{
			QCC_PR_ParseWarning(WARN_CASEINSENSITIVEFRAMEMACRO, "Case insensitive frame macro (using %s)", pr_framemacro[i].name);
			if (pr_framemacro[i].file != s_filen)
				QCC_PR_ParseWarning(WARN_STALEMACRO, "Stale macro used (%s, defined in %s)", pr_token, pr_framemacro[i].file);
			return pr_framemacro[i].value;
		}
	}
	return -1;
}

static void QCC_PR_ExpandMacro(void)
{
	int		i = QCC_PR_FindMacro(pr_token);

	if (i < 0)
		QCC_PR_ParseError (ERR_BADFRAMEMACRO, "Unknown frame macro $%s", pr_token);

	QC_snprintfz(pr_token, sizeof(pr_token),"%d", i);
	pr_token_type = tt_immediate;
	pr_immediate_type = type_float;
	pr_immediate._float = (float)i;
}

pbool QCC_PR_SimpleGetString(void)
{
	int		c;
	int		i = 0;
	char *f;

	pr_token[0] = 0;

// skip whitespace
	while ((c = *pr_file_p) && qcc_iswhite(c))
	{
		if (c=='\n')
			return false;
		pr_file_p++;
	}
	if (c == 0)	//eof
		return false;
//abort if there's a comment.
	if (pr_file_p[0] == '/')
	{
		if (pr_file_p[1] == '/')
		{	//comment alert
			while(*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;
			return false;
		}
		if (pr_file_p[1] == '*')
			return false;
	}

	if (*pr_file_p != '\"')
		return false;	//nope, not a string.
	f = pr_file_p+1;
	while (*f)
	{
		if (*f == '\n' || !*f)
		{	//bad string
			QCC_Error (ERR_INTERNAL, "new line inside string");
			pr_token[0] = 0;
			return false;
		}
		if (*f == '\"')
		{	//end-of-string
			pr_token[i] = 0;
			pr_file_p = f+1;
			return false;
		}
		if (i == sizeof(qcc_token)-1)
			QCC_Error (ERR_INTERNAL, "token exceeds %i chars", i);
		if (*f == '\\')
		{
			f++;
			if (!*f)
				f = "";
			else if (*f == 'n')
			{
				pr_token[i++] = '\n';
				f++;
			}
			else if (*f == 'r')
			{
				pr_token[i++] = '\r';
				f++;
			}
			else if (*f == 't')
			{
				pr_token[i++] = '\t';
				f++;
			}
			else		
				pr_token[i++] = *f++;
		}
		else
			pr_token[i++] = *f++;
	}
	return true;
}
// just parses text, returning false if an eol is reached
pbool QCC_PR_SimpleGetToken (void)
{
	int		c;
	int		i;

	pr_token[0] = 0;

// skip whitespace
	while ((c = *pr_file_p) && qcc_iswhite(c))
	{
		if (c=='\n')
			return false;
		pr_file_p++;
	}
	if (c == 0)	//eof
		return false;
	if (pr_file_p[0] == '/')
	{
		if (pr_file_p[1] == '/')
		{	//comment alert
			while(*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;
			return false;
		}
		if (pr_file_p[1] == '*')
			return false;
	}

	i = 0;
	while ((c = *pr_file_p) && !qcc_iswhite(c) && c != ',' && c != ';' && c != ')' && c != '(' && c != ']' && !(c == '/' && pr_file_p[1] == '/'))
	{
		if (i == sizeof(qcc_token)-1)
			QCC_Error (ERR_INTERNAL, "token exceeds %i chars", i);
		pr_token[i] = c;
		i++;
		pr_file_p++;
	}
	pr_token[i] = 0;
	return i!=0;
}

static pbool QCC_PR_LexMacroName(void)
{
	int		c;
	int		i;

	pr_token[0] = 0;

// skip whitespace
	while ((c = *pr_file_p) && qcc_iswhite(c))
	{
		if (c=='\n')
			return false;
		pr_file_p++;
	}
	if (!c)
		return false;
	if (pr_file_p[0] == '/')
	{
		if (pr_file_p[1] == '/')
		{	//comment alert
			while(*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;
			return false;
		}
		if (pr_file_p[1] == '*')
			return false;
	}

	i = 0;
	while ( (c = *pr_file_p) > ' ' && c != '\n' && c != ',' && c != ';' && c != '&' && c != '|' && c != ')' && c != '(' && c != ']' && !(pr_file_p[0] == '.' && pr_file_p[1] == '.'))
	{
		if (i == sizeof(qcc_token)-1)
			QCC_Error (ERR_INTERNAL, "token exceeds %i chars", i);
		pr_token[i] = c;
		i++;
		pr_file_p++;
	}
	pr_token[i] = 0;
	return i!=0;
}

static void QCC_PR_MacroFrame(char *name, int value, pbool force)
{
	int i;
	for (i=pr_nummacros-1 ; i>=0 ; i--)
	{
		if (!STRCMP (name, pr_framemacro[i].name))
		{
			//vanilla macro behaviour is to not realise that there's dupes. lookups find the first, so dupes end up as dead gaps.
			//our caller incremented the value externally
			//so warn+ignore if its from the same file
			if (pr_framemacro[i].file == s_filen && !force)
				QCC_PR_ParseWarning(WARN_DUPLICATEMACRO, "Duplicate macro defined (%s). Rename it.", pr_token);
			else
			{
				pr_framemacro[i].value = value;	//old file, override it, whatever the old value was is redundant now
				pr_framemacro[i].file = s_filen;
			}
			return;
		}
	}

	if (strlen(name)+1 > sizeof(pr_framemacro[0].name))
		QCC_PR_ParseWarning(ERR_TOOMANYFRAMEMACROS, "Name for frame macro %s is too long", name);
	else
	{
		strcpy (pr_framemacro[pr_nummacros].name, name);
		pr_framemacro[pr_nummacros].value = value;
		pr_framemacro[pr_nummacros].file = s_filen;
		pr_nummacros++;
		if (pr_nummacros >= MAX_FRAMES)
			QCC_PR_ParseError(ERR_TOOMANYFRAMEMACROS, "Too many frame macros defined");
	}
}

static void QCC_PR_ParseFrame (void)
{
	while (QCC_PR_LexMacroName ())
	{
		QCC_PR_MacroFrame(pr_token, pr_macrovalue++, false);
	}
}

/*
==============
PR_LexGrab

Deals with counting sequence numbers and replacing frame macros
==============
*/
static void QCC_PR_LexGrab (void)
{
	pr_file_p++;	// skip the $
//	if (!QCC_PR_SimpleGetToken ())
//		QCC_PR_ParseError ("hanging $");
	if (qcc_iswhite(*pr_file_p))
		QCC_PR_ParseError (ERR_BADFRAMEMACRO, "hanging $");
	QCC_PR_LexMacroName();
	if (!*pr_token)
		QCC_PR_ParseError (ERR_BADFRAMEMACRO, "hanging $");

// check for $frame
	if (!STRCMP (pr_token, "frame") || !STRCMP (pr_token, "framesave"))
	{
		QCC_PR_ParseFrame ();
		QCC_PR_Lex ();
	}
// ignore other known $commands - just for model/spritegen
	else if (!STRCMP (pr_token, "cd")
	|| !STRCMP (pr_token, "origin")
	|| !STRCMP (pr_token, "base")
	|| !STRCMP (pr_token, "flags")
	|| !STRCMP (pr_token, "scale")
	|| !STRCMP (pr_token, "skin") )
	{	// skip to end of line
		while (QCC_PR_LexMacroName ())
		;
		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "flush"))
	{
		QCC_PR_ClearGrabMacros(true);
		while (QCC_PR_LexMacroName ())
		;
		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "frame_reset"))
	{	//for compat with qfcc. full reset of all frame macros.
		QCC_PR_ClearGrabMacros(false);
		while (QCC_PR_LexMacroName ())
		;
		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "framevalue"))
	{
		QCC_PR_LexMacroName ();
		pr_macrovalue = atoi(pr_token);

		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "framerestore"))
	{
		QCC_PR_LexMacroName ();
		QCC_PR_ExpandMacro();
		pr_macrovalue = (int)pr_immediate._float;

		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "modelname"))
	{
		int i;
		QCC_PR_LexMacroName ();

		if (*pr_framemodelname)
			QCC_PR_MacroFrame(pr_framemodelname, pr_macrovalue, true);

		if (!QC_strlcpy(pr_framemodelname, pr_token, sizeof(pr_framemodelname)))
			QCC_PR_ParseWarning (WARN_STRINGTOOLONG, "$modelname name too long");

		i = QCC_PR_FindMacro(pr_framemodelname);
		if (i)
			pr_macrovalue = i;
		else
			i = 0;

		QCC_PR_Lex ();
	}
// look for a frame name macro
	else
		QCC_PR_ExpandMacro ();
}

//===========================
//compiler constants	- dmw

pbool QCC_PR_UndefineName(const char *name)
{
//	int a;
	CompilerConstant_t *c;
	c = pHash_Get(&compconstantstable, name);
	if (!c)
	{
		QCC_PR_ParseWarning(WARN_UNDEFNOTDEFINED, "Precompiler constant %s was not defined", name);
		return false;
	}

	Hash_Remove(&compconstantstable, name);
	return true;
}

CompilerConstant_t *QCC_PR_DefineName(const char *name, const char *value)
{
	int i;
	CompilerConstant_t *cnst;

	if (strlen(name) >= MAXCONSTANTNAMELENGTH || !*name)
		QCC_PR_ParseError(ERR_NAMETOOLONG, "Compiler constant name length is too long or short");

	cnst = pHash_Get(&compconstantstable, name);
	if (cnst)
	{
		if (strcmp(cnst->value, value?value:"") || cnst->numparams!=-1 || cnst->varg)
			QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "Duplicate definition for Precompiler constant %s", name);
		Hash_Remove(&compconstantstable, name);
	}

	cnst = qccHunkAlloc(sizeof(CompilerConstant_t));

	cnst->used = false;
	cnst->numparams = -1;
	cnst->evil = false;
	strcpy(cnst->name, name);
	cnst->namelen = strlen(name);
	cnst->value = cnst->name + strlen(cnst->name);
	for (i = 0; i < MAXCONSTANTPARAMS; i++)
		cnst->params[i][0] = '\0';

	pHash_Add(&compconstantstable, cnst->name, cnst, qccHunkAlloc(sizeof(bucket_t)));

	if (value && *value)
		cnst->value = strcpy(qccHunkAlloc(strlen(value)+1), value);

	return cnst;
}

void QCC_PR_PreProcessor_Define(pbool append)
{
	char *d;
	char *dbuf;
	int dbuflen;
	char *s;
	int quote=false;
	pbool preprocessorhack = false;
	CompilerConstant_t *cnst, *oldcnst;

	QCC_PR_SimpleGetToken ();

	if (!QCC_PR_SimpleGetToken ())
		QCC_PR_ParseError(ERR_NONAME, "No name defined for compiler constant");

	oldcnst = pHash_Get(&compconstantstable, pr_token);
	if (oldcnst)
		Hash_Remove(&compconstantstable, oldcnst->name);

	cnst = QCC_PR_DefineName(pr_token, NULL);

	if (*pr_file_p == '(')
	{
		cnst->numparams = 0;
		pr_file_p++;
		while(qcc_iswhitesameline(*pr_file_p))
			pr_file_p++;
		s = pr_file_p;
		for (;;)
		{
			if (*pr_file_p == ',' || *pr_file_p == ')')
			{
				int nl;
				nl = pr_file_p-s;
				while(nl > 0 && qcc_iswhitesameline(s[nl-1]))
					nl--;
				if (cnst->numparams >= MAXCONSTANTPARAMS)
					QCC_PR_ParseError(ERR_MACROTOOMANYPARMS, "May not have more than %i parameters to a macro", MAXCONSTANTPARAMS);
				if (nl >= MAXCONSTANTPARAMLENGTH)
					QCC_PR_ParseError(ERR_MACROTOOMANYPARMS, "parameter name is too long (max %i)", MAXCONSTANTPARAMLENGTH);
				if (nl == 3 && s[0] == '.' && s[1] == '.' && s[2] == '.')
				{
					cnst->varg = true;
					if (*pr_file_p != ')')
						QCC_PR_ParseError(ERR_MACROTOOMANYPARMS, "varadic argument must be last");
				}
				else
				{
					memcpy(cnst->params[cnst->numparams], s, nl);
					cnst->params[cnst->numparams][nl] = '\0';
					for (nl = 0; nl < cnst->numparams; nl++)
					{
						if (!strcmp(cnst->params[nl], cnst->params[cnst->numparams]))
							QCC_PR_ParseError(ERR_MACROTOOMANYPARMS, "duplicate macro paramter name '%s'", cnst->params[nl]);
					}
					cnst->numparams++;
				}
				if (*pr_file_p++ == ')')
					break;
				while(qcc_iswhitesameline(*pr_file_p))
					pr_file_p++;
				s = pr_file_p;
			}
			if(!*pr_file_p++)
			{
				QCC_PR_ParseError(ERR_EXPECTED, "missing ) in macro parameter list");
				break;
			}
		}
	}
	else cnst->numparams = -1;

	//disable append mode if they're trying to do something stupid
	if (append)
	{
		if (!oldcnst)
			append = false;	//append with no previous define is treated as just a regular define. huzzah.
		else if (cnst->numparams != oldcnst->numparams || cnst->varg != oldcnst->varg)
		{
			QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "different number of macro arguments in macro append");
			append = false;
		}
		else
		{
			int i;
			//arguments need to be specified, if only so that appends with arguments are still vaugely readable.
			//argument names need to match because the expansion is too lame to cope if they're different.
			for (i = 0; i < cnst->numparams; i++)
			{
				if (strcmp(cnst->params[i], oldcnst->params[i]))
					break;
			}
			if (i < cnst->numparams)
			{
				QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "arguments differ in macro append");
				append = false;
			}
			else
				append = true;
		}
	}

	s = pr_file_p;
	d = dbuf = NULL;
	dbuflen = 0;

	if (append)
	{
		//start with the old value
		int olen = strlen(oldcnst->value);
		dbuflen = olen + 128;
		dbuf = qccHunkAlloc(dbuflen);
		memcpy(dbuf, oldcnst->value, olen);
		d = dbuf + olen;
		*d++ = ' ';
	}

	cnst->fromfile = s_filen;
	cnst->fromline = pr_source_line;

	while(*s == ' ' || *s == '\t')
		s++;
	while(1)
	{
		if ((d - dbuf) + 2 >= dbuflen)
		{
			int len = d - dbuf;
			dbuflen = (len+128) * 2;
			dbuf = qccHunkAlloc(dbuflen);
			memcpy(dbuf, d - len, len);
			d = dbuf + len;
		}

		if( *s == '\\' )
		{
			// read over a newline if necessary
			if( s[1] == '\n' || s[1] == '\r' )
			{
				char *exploitcheck;
				s++;	//skip the \ char
				if (*s == '\r' && s[1] == '\n')
					s++;	//skip the \r. the \n will become part of the macro.
				s++;	//skip the \n
				QCC_PR_NewLine(true);

/*
This began as a bug. It is still evil, but its oh so useful.

In C,
#define foobar \
foo\
bar\
moo

becomes foobarmoo, not foo\nbar\nmoo

#define hacks however, require that it becomes
foo\nbar\nmoo

# cannot be used on the first line of the macro, and then is only valid as the first non-white char of the following lines
so if present, the preceeding \\\n and following \\\n must become an actual \n instead of being stripped.
*/

				for (exploitcheck = s; *exploitcheck && qcc_iswhitesameline(*exploitcheck); exploitcheck++)
					;
				if (*exploitcheck == '#')
				{
					*d++ = '\n';
					if (!cnst->evil)
						QCC_PR_ParseWarning(WARN_EVILPREPROCESSOR, "preprocessor directive within preprocessor macro %s", cnst->name);
					cnst->evil = true;
					preprocessorhack = true;
				}
				else if (preprocessorhack)
				{
					*d++ = '\n';
					preprocessorhack = false;
				}
			}
		}
		else if(*s == '\r' || *s == '\n' || *s == '\0')
		{
			break;
		}
		if (!quote && s[0]=='/'&&s[1]=='/')
			break;	//c++ style comments can just be ignored
		if (!quote && s[0]=='/'&&s[1]=='*')
		{	//multi-line c style comments become part of the define itself. this also negates the need for \ at the end of lines.
			//although we don't bother embedding.
			s+=2;
			for(;;)
			{
				if (!s[0])
				{
					QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "EOF inside quote in define %s", cnst->name);
					break;
				}
				if (s[0]=='*'&&s[1]=='/')
				{
					s+=2;
					break;
				}
				if (s[0] == '\n')
					pr_source_line++;
				s++;
			}
			continue;
		}
		if (*s == '\"')
			quote=!quote;

		*d = *s;
		d++;
		s++;
	}

	while (d>dbuf && qcc_iswhitesameline(d[-1]))
		d--;
	*d = '\0';

	cnst->value = dbuf;

	if (oldcnst && !append)
	{	//we always warn if it was already defined
		//we use different warning codes so that -Wno-mundane can be used to ignore identical redefinitions.
		if (strcmp(oldcnst->value, cnst->value))
			QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "Alternate precompiler definition of %s (%s -> %s)", pr_token, oldcnst->value, cnst->value);
		else
			QCC_PR_ParseWarning(WARN_IDENTICALPRECOMPILER, "Identical precompiler definition of %s", pr_token);
	}

	pr_file_p = s;
}

/* *buffer, *bufferlen and *buffermax should be NULL/0 at the start */
static void QCC_PR_ExpandStrCat(char **buffer, size_t *bufferlen, size_t *buffermax,   char *newdata, size_t newlen)
{
	size_t newmax = *bufferlen + newlen;

	if (newmax < *bufferlen)//check for overflow
	{
		QCC_PR_ParseWarning(ERR_INTERNAL, "exceeds 4gb");
		return;
	}
	if (newmax > *buffermax)
	{
		char *newbuf;
		if (newmax < 64)
			newmax = 64;
		if (newmax < *bufferlen * 2)
		{
			newmax = *bufferlen * 2;
			if (newmax < *bufferlen) /*overflowed?*/
			{
				QCC_PR_ParseWarning(ERR_INTERNAL, "exceeds 4gb");
				return;
			}
		}
		newbuf = realloc(*buffer, newmax);
		if (!newbuf)
		{
			QCC_PR_ParseWarning(ERR_INTERNAL, "out of memory");
			return; /*OOM*/
		}
		*buffer = newbuf;
		*buffermax = newmax;
	}
	memcpy(*buffer + *bufferlen, newdata, newlen);
	*bufferlen += newlen;
	/*no null terminator, remember to cat one if required*/
}
/* *buffer, *bufferlen and *buffermax should be NULL/0 at the start */
static void QCC_PR_ExpandStrCatMarkup(char **buffer, size_t *bufferlen, size_t *buffermax,   char *newdata, size_t newlen)
{
	size_t newmax = *bufferlen + newlen*2;

	if (newmax < *bufferlen)//check for overflow
	{
		QCC_PR_ParseWarning(ERR_INTERNAL, "exceeds 4gb");
		return;
	}
	if (newmax > *buffermax)
	{
		char *newbuf;
		if (newmax < 64)
			newmax = 64;
		if (newmax < *bufferlen * 2)
		{
			newmax = *bufferlen * 2;
			if (newmax < *bufferlen) /*overflowed?*/
			{
				QCC_PR_ParseWarning(ERR_INTERNAL, "exceeds 4gb");
				return;
			}
		}
		newbuf = realloc(*buffer, newmax);
		if (!newbuf)
		{
			QCC_PR_ParseWarning(ERR_INTERNAL, "out of memory");
			return; /*OOM*/
		}
		*buffer = newbuf;
		*buffermax = newmax;
	}

	while (newlen--)
	{
		if (*newdata == '\n')
		{
			(*buffer)[*bufferlen+0] = '\\';
			(*buffer)[*bufferlen+1] = '\n';
			*bufferlen += 2;
		}
		else if (*newdata == '\\')
		{
			(*buffer)[*bufferlen+0] = '\\';
			(*buffer)[*bufferlen+1] = '\\';
			*bufferlen += 2;
		}
		else if (*newdata == '\0')
		{
			(*buffer)[*bufferlen+0] = '\\';
			(*buffer)[*bufferlen+1] = '0';
			*bufferlen += 2;
		}
		else if (*newdata == '\"')
		{
			(*buffer)[*bufferlen+0] = '\\';
			(*buffer)[*bufferlen+1] = '\"';
			*bufferlen += 2;
		}
		else
		{
			(*buffer)[*bufferlen] = *newdata;
			*bufferlen += 1;
		}
		newdata++;
	}
	/*no null terminator, remember to cat one if required*/
}

static const struct tm *QCC_CurrentTime(void)
{
	//if SOURCE_DATE_EPOCH environment is defined, use that as seconds from epoch (and show utc)
	//this helps give reproducable builds (which is for some debian project, demonstrating that noone is hacking binaries).
	const char *env = getenv("SOURCE_DATE_EPOCH");
	time_t t;
	if (env && *env)
	{
		t = strtoull(env, NULL, 0);
		if (t)
			return gmtime(&t);
	}

	t = time(NULL);
	return localtime(&t);
}

#if _POSIX_C_SOURCE >= 2 || defined(_WIN32)
#define HAVE_POPEN
#endif
#ifdef HAVE_POPEN
static char *QCC_PR_PopenMacro(const char *macroname, const char *cmd, char *retbuf, size_t retbufsize)
{
	char *ret = retbuf;
	char temp[65536], *t = temp;
#ifdef _WIN32
	FILE *f = _popen(cmd, "rt");
#else
	FILE *f = popen(cmd, "r");
#endif
	int len;
	if (!f)
	{
		QCC_PR_ParseWarning(ERR_CONSTANTNOTDEFINED, "%s: Unable to execute \"%s\" for value", macroname, cmd);
		return NULL;
	}
	retbufsize-=3;	// '""\0'
	*retbuf++ = '\"';
	for (;;)
	{
		len = fread(temp, 1, sizeof(temp), f);
		if (len <= 0)
			break;
		else for (t=temp; len --> 0 && *t && retbufsize > 1; t++)
		{
			if      (*t == '\"')	retbuf[1] = '\"';
			else if (*t == '\n')	retbuf[1] = 'n';
			else if (*t == '\r')	retbuf[1] = 'r';
			else if (*t == '#')		retbuf[1] = '#';	//so we don't get preqcc-style expansion in strings.
			else
			{
				*retbuf++ = *t;
				retbufsize--;
				continue;
			}
			retbuf[0] = '\\';
			retbuf+=2;
			retbufsize-=2;
		}
	}
	if (retbuf[-1] == 'n' && retbuf[-2] == '\\')
		retbuf -= 2;
	if (retbuf[-1] == 'r' && retbuf[-2] == '\\')
		retbuf -= 2;
	*retbuf++ = '\"';
	*retbuf++ = 0;
#ifdef _WIN32
	_pclose(f);
#else
	pclose(f);
#endif
	return ret;
}
#endif

static char *QCC_PR_CheckBuiltinCompConst(char *constname, char *retbuf, size_t retbufsize)
{
	if (constname[0] != '_' || constname[1] != '_')
		return NULL;
	if (!strcmp(constname, "__TIME__"))
	{
		strftime( retbuf, retbufsize,	"\"%H:%M\"", QCC_CurrentTime());
		return retbuf;
	}
	if (!strcmp(constname, "__DATE__"))
	{
		strftime( retbuf, retbufsize,	"\"%a %d %b %Y\"", QCC_CurrentTime());
		return retbuf;
	}
#ifdef HAVE_POPEN
	if (!strcmp(constname, "__GITURL__"))
		return QCC_PR_PopenMacro(constname, "git remote get-url origin", retbuf, retbufsize);	//some git url...
	if (!strcmp(constname, "__GITHASH__"))
		return QCC_PR_PopenMacro(constname, "git log -1 --format=%H", retbuf, retbufsize);	//just a hash
	if (!strcmp(constname, "__GITDATE__"))
		return QCC_PR_PopenMacro(constname, "git log -1 --format=%cs", retbuf, retbufsize);	//YYYY-MM-DD
	if (!strcmp(constname, "__GITDATETIME__"))
		return QCC_PR_PopenMacro(constname, "git log -1 --format=%ci", retbuf, retbufsize);	//YYYY-MM-DD HH:MM:SS +TZ
	if (!strcmp(constname, "__GITDESC__"))
		return QCC_PR_PopenMacro(constname, "git describe", retbuf, retbufsize);
#endif
	if (!strcmp(constname, "__RAND__"))
	{
		QC_snprintfz(retbuf, retbufsize, "%i", rand());
		return retbuf;
	}
#if defined(SVNREVISION) && defined(SVNDATE)
	if (!strcmp(constname, "__QCCREV__"))
		return STRINGIFY(SVNREVISION); //no M or anything, so you can compare revisions properly
#endif
	if (!strcmp(constname, "__QCCVER__"))
	{
#if defined(SVNREVISION) && defined(SVNDATE)
		return "\"FTEQCC " STRINGIFY(SVNREVISION) " "STRINGIFY(SVNDATE)"\"";
#elif defined(SVNREVISION)
		return "\"FTEQCC " STRINGIFY(SVNREVISION) " "__DATE__"\"";
#else
		return "\""__DATE__"\"";
#endif
	}
	if (!strcmp(constname, "__FILE__"))
	{
		char *erk;
		QC_snprintfz(retbuf, retbufsize, "\"%s\"", s_filen);
		erk = strchr(retbuf, ':');
		if (erk)
		{
			erk[0] = '\"';
			erk[1] = 0;
		}
		return retbuf;
	}
	if (!strcmp(constname, "__LINE__"))
	{
		int line = pr_source_line;
		if (currentchunk && currentchunk->cnst)	//if we're in a macro, use the line the macro was on.
			line = currentchunk->currentlinenumber;
		QC_snprintfz(retbuf, retbufsize, "%i", line);
		return retbuf;
	}
	if (!strcmp(constname, "__LINESTR__"))
	{
		int line = pr_source_line;
		if (currentchunk && currentchunk->cnst)	//if we're in a macro, use the line the macro was on.
			line = currentchunk->currentlinenumber;
		QC_snprintfz(retbuf, retbufsize, "\"%i\"", line);
		return retbuf;
	}
	if (!strcmp(constname, "__FUNC__") || !strcmp(constname, "__func__"))
	{
		QC_snprintfz(retbuf, retbufsize, "\"%s\"",pr_scope?pr_scope->name:"<NO FUNCTION>");
		return retbuf;
	}
	if (!strcmp(constname, "__NULL__"))
	{
		return "0i";
	}
	return NULL;	//didn't match
}

static pbool QCC_PR_ExpandPreProcessorMacro(CompilerConstant_t *c, char **buffer, size_t *bufferlen, size_t *buffermax)
{
	int p;
	char *start;
	char *starttok;
	char *argsend;
	int argsendline;
	size_t whitestart;
	char *paramoffset[MAXCONSTANTPARAMS+1];
	int param=0, extraparam=0;
	int plevel=0;
	pbool noargexpand;

	char *end;
	char retbuf[256];

	pr_file_p++;
	QCC_PR_LexWhitespace(false);
	start = pr_file_p;
	while(1)
	{
		// handle strings correctly by ignoring them
		if (*pr_file_p == '\"')
		{
			do {
				pr_file_p++;
			} while( (pr_file_p[-1] == '\\' || pr_file_p[0] != '\"') && *pr_file_p && *pr_file_p != '\n' );
		}
		if (*pr_file_p == '(')
			plevel++;
		else if (!plevel && (*pr_file_p == ',' || *pr_file_p == ')'))
		{
			if (*pr_file_p == ',' && c->varg && param >= c->numparams)
				extraparam++;	//skip extra trailing , arguments if we're varging.
			else
			{
				paramoffset[param++] = start;
				start = pr_file_p+1;
				if (*pr_file_p == ')')
				{
					*pr_file_p = '\0';
					pr_file_p++;
					break;
				}
				*pr_file_p = '\0';
				pr_file_p++;
				QCC_PR_LexWhitespace(false);
				start = pr_file_p;
				// move back by one char because we move forward by one at the end of the loop
				pr_file_p--;
				if (param == MAXCONSTANTPARAMS || param > c->numparams)
					QCC_PR_ParseError(ERR_TOOMANYPARAMS, "Too many parameters in macro call");
			}
		} else if (*pr_file_p == ')' )
			plevel--;
		else if(*pr_file_p == '\n')
			QCC_PR_NewLine(false);

		// see that *pr_file_p = '\0' up there? Must ++ BEFORE checking for !*pr_file_p
		pr_file_p++;
		if (!*pr_file_p)
			QCC_PR_ParseError(ERR_EOF, "EOF on macro call");
	}
	if (param < c->numparams)
		QCC_PR_ParseError(ERR_TOOFEWPARAMS, "Not enough macro parameters");
	paramoffset[param] = start;

	*buffer = NULL;
	*bufferlen = 0;
	*buffermax = 0;

//				QCC_PR_LexWhitespace(false);
	argsend = pr_file_p;
	argsendline = pr_source_line;
	pr_file_p = c->value;
	for(;;)
	{
		noargexpand = false;
		whitestart = *bufferlen;
		starttok = pr_file_p;
		/*while(qcc_iswhite(*pr_file_p))	//copy across whitespace
		{
			if (!*pr_file_p)
				break;
			pr_file_p++;
		}*/
		QCC_PR_LexWhitespace(true);
		if (starttok != pr_file_p)
		{
			QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   starttok, pr_file_p - starttok);
		}

		if(*pr_file_p == '\"')
		{
			starttok = pr_file_p;
			do
			{
				pr_file_p++;
			} while( (pr_file_p[-1] == '\\' || pr_file_p[0] != '\"') && *pr_file_p && *pr_file_p != '\n' );
			if(*pr_file_p == '\"')
				pr_file_p++;

			QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   starttok, pr_file_p - starttok);
			continue;
		}
		else if (*pr_file_p == '#')	//if you ask for #a##b you will be shot. use #a #b instead, or chain macros.
		{
			if (pr_file_p[1] == '#')
			{	//concatinate (strip out whitespace before the token)
				*bufferlen = whitestart;
				pr_file_p+=2;
				noargexpand = true;
			}
			else
			{	//stringify
				pr_file_p++;
				pr_file_p = QCC_COM_Parse2(pr_file_p);
				if (!pr_file_p)
					break;

				for (p = 0; p < param; p++)
				{
					if (!STRCMP(qcc_token, c->params[p]))
					{
						QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   "\"", 1);
						QCC_PR_ExpandStrCatMarkup(buffer,bufferlen,buffermax,   paramoffset[p], strlen(paramoffset[p]));
						QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   "\"", 1);
						break;
					}
				}
				if (p == param)
				{
					QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   "#", 1);
					QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   qcc_token, strlen(qcc_token));
					if (!c->evil)
						QCC_PR_ParseWarning(0, "'#%s' is not a macro parameter in %s", qcc_token, c->name);
				}
				continue;	//already did this one
			}
		}

		end = qcc_token;
		pr_file_p = QCC_COM_Parse2(pr_file_p);
		if (!pr_file_p)
			break;

		for (p = 0; p < c->numparams; p++)
		{
			if (!STRCMP(qcc_token, c->params[p]))
			{
				char *argstart, *argend;

				for (start = pr_file_p; qcc_iswhite(*start); start++)
					;
				if (noargexpand || (start[0] == '#' && start[1] == '#'))
					QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   paramoffset[p], strlen(paramoffset[p]));
				else
				{
					for (argstart = paramoffset[p]; *argstart; argstart = argend)
					{
						argend = argstart;
						while (qcc_iswhite(*argend))
							argend++;
						if (*argend == '\"')
						{
							do
							{
								argend++;
							} while( (argend[-1] == '\\' || argend[0] != '\"') && *argend && *argend != '\n' );
							if(*argend == '\"')
								argend++;
							end = NULL;
						}
						else
						{
							argend = QCC_COM_Parse2(argend);
							if (!argend)
								break;
							end = QCC_PR_CheckBuiltinCompConst(qcc_token, retbuf, sizeof(retbuf));
						}
						//FIXME: we should be testing all defines instead of just built-in ones.
						if (end)
							QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   end, strlen(end));
						else
							QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   argstart, argend-argstart);
					}
				}
				break;
			}
		}
		if (c->varg && !STRCMP(qcc_token, "__VA_ARGS__"))
		{	//c99
			if (param-1 == c->numparams)
				QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   paramoffset[c->numparams], strlen(paramoffset[c->numparams]));
			else if (noargexpand)
			{
				if(*bufferlen>0 && (*buffer)[*bufferlen-1] == ',')
					*bufferlen-=1;
			}
		}
		else if (c->varg && !STRCMP(qcc_token, "__VA_COUNT__"))
		{	//not c99
			char tmp[64];
			if (param < c->numparams)
				QCC_PR_ParseError(ERR_TOOFEWPARAMS, "__VA_COUNT__ without any variable args");
			QC_snprintfz(tmp, sizeof(tmp), "%i", param-1+extraparam);
			QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   tmp, strlen(tmp));
		}
		else if (p == c->numparams)
		{
			/*CompilerConstant_t *c2 = pHash_Get(&compconstantstable, qcc_token);
			if (c2 && c2->numparams >= 0 && *pr_file_p == '(')
			{	//oh dear god this is vile bullshit
				char *sub = NULL;
				size_t sublen;
				size_t submax;
				pr_file_p++;
				if (QCC_PR_ExpandPreProcessorMacro(c, &sub,&sublen,&submax))
				{
					QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   sub, sublen);
				}
				free(sub);
			}
			else*/
				QCC_PR_ExpandStrCat(buffer,bufferlen,buffermax,   qcc_token, strlen(qcc_token));
		}
	}

	for (p = 0; p < param-1; p++)
		paramoffset[p][strlen(paramoffset[p])] = ',';
	paramoffset[p][strlen(paramoffset[p])] = ')';

	if (c->inside>8)
		return false;

	pr_file_p = argsend;
	pr_source_line = argsendline;

	if (flag_debugmacros)
	{
		if (flag_msvcstyle)
			externs->Printf ("%s(%i) : macro %s: %s\n", s_filen, pr_source_line, c->name, pr_file_p);
		else
			externs->Printf ("%s:%i: macro %s: %s\n", s_filen, pr_source_line, c->name, pr_file_p);
	}
	return true;
}

int QCC_PR_CheckCompConst(void)
{
	//FIXME: FOO(SPLAT(FOO(BAR)))
	//the above should expand the inner foo before expanding the outer foo

	char		*initial_file_p = pr_file_p;
	int			initial_line = pr_source_line;

	CompilerConstant_t *c;

	char *end, *tok;
	char retbuf[256];

	for (end = pr_file_p; ; end++)
	{
		if (!*end || qcc_iswhite(*end))
			break;

		if (*end == ')'
			||	*end == '('
			||	*end == '+'
			||	*end == '-'
			||	*end == '*'
			||	*end == '/'
			||	*end == '|'
			||	*end == '&'
			||	*end == '='
			||	*end == '^'
			||	*end == '~'
			||	*end == '['
			||	*end == ']'
			||	*end == '\"'
			||	*end == '{'
			||	*end == '}'
			||	*end == ';'
			||	*end == ':'
			||	*end == '<'
			||	*end == '>'
			||	*end == ','
			||	*end == '.'
			||	*end == '#')
				break;
	}
	if (!QC_strnlcpy(pr_token, pr_file_p, end-pr_file_p, sizeof(pr_token)))
		return false;	//name too long to be a valid macro

//	externs->Printf("%s\n", pr_token);
	c = pHash_Get(&compconstantstable, pr_token);

	if (c && (!currentchunk || currentchunk->cnst != c))	//macros don't expand themselves
	{
		pr_file_p = initial_file_p+strlen(c->name);
		while(*pr_file_p == ' ' || *pr_file_p == '\t')
			pr_file_p++;
		if (c->numparams>=0)
		{
			if (*pr_file_p == '(')
			{
				char *buffer = NULL;
				size_t bufferlen, buffermax;
				if (!QCC_PR_ExpandPreProcessorMacro(c, &buffer, &bufferlen, &buffermax))
				{
					free(buffer);
					pr_file_p = initial_file_p;
					pr_source_line = initial_line;
					return false;
				}

				if (!bufferlen)
					expandedemptymacro = true;
				else
				{
					QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   "\0", 1);
					QCC_PR_IncludeChunkEx(buffer, true, NULL, c);
				}
				expandedemptymacro = true;
				free(buffer);
			}
			else
			{
				//QCC_PR_ParseError(ERR_TOOFEWPARAMS, "Macro without argument list");
				//such macros don't get expanded.
				pr_file_p = initial_file_p;
				pr_source_line = initial_line;
				return false;
			}
		}
		else
		{
			if (c->inside >= 8)
			{
				pr_file_p = initial_file_p;
				pr_source_line = initial_line;
				return false;
			}

			if (*c->value)
				QCC_PR_IncludeChunkEx(c->value, false, NULL, c);
			expandedemptymacro = true;
		}
		return true;
	}

	tok = QCC_PR_CheckBuiltinCompConst(pr_token, retbuf, sizeof(retbuf));
	if (tok)
	{
		pr_file_p = end;
		QCC_PR_IncludeChunkEx(tok, true, NULL, NULL);
		return true;
	}
	return false;
}

const char *QCC_PR_CheckCompConstString(const char *def)
{
	const char *s;

	CompilerConstant_t *c;

	c = pHash_Get(&compconstantstable, def);

	if (c)
	{
		s = QCC_PR_CheckCompConstString(c->value);
		return s;
	}
	return def;
}

CompilerConstant_t *QCC_PR_CheckCompConstDefined(const char *def)
{
	CompilerConstant_t *c = pHash_Get(&compconstantstable, def);
	return c;
}

char *QCC_PR_CheckCompConstTooltip(char *word, char *outstart, char *outend)
{
	int i;
	CompilerConstant_t *c = QCC_PR_CheckCompConstDefined(word);
	if (c)
	{
		char *out = outstart;
		if (c->numparams >= 0)
		{
			QC_snprintfz(out, outend-out, "#define %s(", c->name);
			out += strlen(out);
			for (i = 0; i < c->numparams-1; i++)
			{
				QC_snprintfz(out, outend-out, "%s,", c->params[i]);
				out += strlen(out);
			}
			if (i < c->numparams)
			{
				QC_snprintfz(out, outend-out, "%s", c->params[i]);
				out += strlen(out);
			}
			QC_snprintfz(out, outend-out, ")");
		}
		else
			QC_snprintfz(out, outend-out, "#define %s", c->name);
		out += strlen(out);
		if (c->value && *c->value)
			QC_snprintfz(out, outend-out, "\n%s", c->value);

		return outstart;
	}
	return NULL;
}

//============================================================================

/*
==============
PR_Lex

Sets pr_token, pr_token_type, and possibly pr_immediate and pr_immediate_type
==============
*/
void QCC_PR_Lex (void)
{
	int		c;

	pr_token[0] = 0;

	if (!pr_file_p)
	{
		if (QCC_PR_UnInclude())
		{
			QCC_PR_Lex();
			return;
		}
		pr_token_type = tt_eof;
		return;
	}

	pr_token_precomment = NULL;
	QCC_PR_LexComment(&pr_token_precomment);
//	QCC_PR_LexWhitespace (false);

	pr_token_line_last = pr_token_line;
	pr_token_line = pr_source_line;
	if (currentchunk)
		pr_token_line += currentchunk->currentlinenumber-1;

	if (!pr_file_p)
	{
		if (QCC_PR_UnInclude())
		{
			QCC_PR_Lex();
			return;
		}
		pr_token_type = tt_eof;
		return;
	}

	c = *pr_file_p;

	if (!c)
	{
		if (QCC_PR_UnInclude())
		{
			QCC_PR_Lex();
			return;
		}
		pr_token_type = tt_eof;
		return;
	}

// handle quoted strings as a unit
	if (c == '\"' || ((c == 'R' || c == 'Q' || c == 'u' || c == 'U') && pr_file_p[1] == '\"') || (c == 'u' && pr_file_p[1] == '8' && pr_file_p[2] == '\"'))
	{
		QCC_PR_LexString ();
		return;
	}

// handle quoted vectors as a unit
	if (c == '\'')
	{
		QCC_PR_LexVector ();
		return;
	}

// if the first character is a valid identifier, parse until a non-id
// character is reached
	if ((c == '%') && flag_qccx && (pr_file_p[1] == '-' || (pr_file_p[1] >= '0' && pr_file_p[1] <= '9')))
	{
		//with qccx, %5 is a denormalized float.
		pr_file_p++;
		pr_token_type = tt_immediate;
		pr_immediate_type = type_float;
		QCC_PR_ParseWarning(WARN_DENORMAL, "denormalized immediate");
		pr_immediate._int = QCC_PR_LexInteger ();
		return;
	}
	if ( c == '0' && pr_file_p[1] == 'x')
	{
		pr_token_type = tt_immediate;
		QCC_PR_LexNumber();
		return;
	}
	if ( (c == '.'&&pr_file_p[1]!='.'&&pr_file_p[1] >='0' && pr_file_p[1] <= '9') || (c >= '0' && c <= '9') || ( c=='-' && pr_file_p[1]>='0' && pr_file_p[1] <='9') )
	{
		pr_token_type = tt_immediate;
		QCC_PR_LexNumber ();
		return;
	}

	if (/*!flag_qccx &&*/ c == '#' && !(pr_file_p[1]==')' || pr_file_p[1]==',' || pr_file_p[1]=='\"' || pr_file_p[1]=='-' || (pr_file_p[1]>='0' && pr_file_p[1] <='9')))	//hash and not number
	{
		pr_file_p++;
		if (!QCC_PR_CheckCompConst())
		{
			if (!QCC_PR_SimpleGetToken())
				strcpy(pr_token, "unknown");
			QCC_PR_ParseError(ERR_CONSTANTNOTDEFINED, "Explicit precompiler usage when not defined %s", pr_token);
		}
		else
		{
			QCC_PR_Lex();
			if (pr_token_type == tt_eof)
				QCC_PR_Lex();
		}

		return;
	}

	if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c & 0x80))
	{
		if (flag_hashonly || !QCC_PR_CheckCompConst())	//look for a macro.
			QCC_PR_LexName ();
		else
		{
			//we expanded a macro. we need to read the tokens out of it now though
			QCC_PR_Lex();
			if (pr_token_type == tt_eof)
			{
				if (QCC_PR_UnInclude())
				{
					QCC_PR_Lex();
					return;
				}
				pr_token_type = tt_eof;
			}
		}
		return;
	}

	if (c == '$')
	{
		QCC_PR_LexGrab ();
		return;
	}

// parse symbol strings until a non-symbol is found
	QCC_PR_LexPunctuation ();
}

//=============================================================================

pbool QCC_Temp_Describe(QCC_def_t *def, char *buffer, int buffersize);
void QCC_PR_ParsePrintDef (int type, QCC_def_t *def)
{
	if (!qccwarningaction[type])
		return;
	if (def->filen)
	{
		char tybuffer[512];
		char tmbuffer[512];
		char vlbuffer[512];
		char *modifiers;
		if (QCC_Temp_Describe(def, tmbuffer, sizeof(tmbuffer)))
		{
			externs->Printf ("%s:%i:    (%s)(%s)\n", def->filen, def->s_line, TypeName(def->type, tybuffer, sizeof(tybuffer)), tmbuffer);
		}
		else
		{
			modifiers = "";
			if (def->constant)
				modifiers = "const ";
			else if (def->isstatic)
				modifiers = "static ";


			if (def && def->initialized && def->constant && !def->arraysize && def->symboldata)
			{
				const QCC_eval_t *ev = (const QCC_eval_t*)&def->symboldata[0];
				switch(def->type->type)
				{
				case ev_float:	QC_snprintfz(vlbuffer, sizeof(vlbuffer), " = %g", ev->_float);	break;
				case ev_double:	QC_snprintfz(vlbuffer, sizeof(vlbuffer), " = %g", ev->_double);	break;
				case ev_integer:QC_snprintfz(vlbuffer, sizeof(vlbuffer), " = %i", ev->_int);	break;
				case ev_uint:	QC_snprintfz(vlbuffer, sizeof(vlbuffer), " = %u", ev->_uint);	break;
				case ev_int64:	QC_snprintfz(vlbuffer, sizeof(vlbuffer), " = %"pPRIi64, ev->i64);	break;
				case ev_uint64:	QC_snprintfz(vlbuffer, sizeof(vlbuffer), " = %"pPRIu64, ev->u64);	break;
				default:		*vlbuffer = 0;													break;
				}
			}
			else
				*vlbuffer = 0;
			if (flag_msvcstyle)
				externs->Printf ("%s%s(%i) :    %s%s%s %s%s%s%s%s is defined here\n", col_location, def->filen, def->s_line, col_type, modifiers, TypeName(def->type, tybuffer, sizeof(tybuffer)), col_symbol, def->name, col_type, vlbuffer, col_none);
			else
				externs->Printf ("%s%s:%i:    %s%s%s %s%s%s%s%s is defined here\n", col_location, def->filen, def->s_line, col_type, modifiers, TypeName(def->type, tybuffer, sizeof(tybuffer)), col_symbol, def->name, col_type, vlbuffer, col_none);
		}
	}
}
void QCC_PR_ParsePrintSRef (int type, QCC_sref_t def)
{
	QCC_PR_ParsePrintDef(type, def.sym);
}

void *errorscope;
static void QCC_PR_PrintMacro (qcc_includechunk_t *chunk)
{
	if (chunk)
	{
		QCC_PR_PrintMacro(chunk->prev);
		if (chunk->cnst)
		{
#if 1
			externs->Printf ("%s%s:%i: macro %s%s%s is defined here\n", col_location, chunk->cnst->fromfile, chunk->cnst->fromline, col_symbol, chunk->cnst->name, col_none);
#else
			externs->Printf ("%s:%i: expanding %s\n", chunk->currentfilename, chunk->currentlinenumber, chunk->cnst->name);
#endif
			if (verbose >= VERBOSE_STANDARD)
				externs->Printf ("%s\n", chunk->datastart);
		}
		else
			externs->Printf ("%s:%i:\n", chunk->currentfilename, chunk->currentlinenumber);
	}
}
static void QCC_PR_PrintScope (void)
{
	QCC_PR_PrintMacro(currentchunk);
	if (pr_scope)
	{
		if (errorscope != pr_scope)
			externs->Printf ("in function %s%s%s (line %i),\n", col_symbol, pr_scope->name, col_none, pr_scope->line);
		errorscope = pr_scope;
	}
	else
	{
		if (errorscope)
			externs->Printf ("at global scope,\n");
		errorscope = NULL;
	}
}
void QCC_PR_ResetErrorScope(void)
{
	errorscope = NULL;
}
/*
============
PR_ParseError

Aborts the current file load
============
*/
#ifndef QCC
void editbadfile(const char *file, int line);
#endif
//will abort.
NORETURN void VARGS QCC_PR_ParseError (int errortype, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

#ifndef QCC
	editbadfile(s_filen, pr_source_line);
#endif

	if (error)
	{
		QCC_PR_PrintScope();
		if (flag_msvcstyle)
			externs->Printf ("%s%s(%i) : %serror%s: %s\n", col_location, s_filen, pr_source_line, col_error, col_none, string);
		else
			externs->Printf ("%s%s:%i: %serror%s: %s\n", col_location, s_filen, pr_source_line, col_error, col_none, string);
	}

	longjmp (pr_parse_abort, 1);
}
//will abort.
NORETURN void VARGS QCC_PR_ParseErrorPrintDef (int errortype, QCC_def_t *def, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

#ifndef QCC
	editbadfile(s_filen, pr_source_line);
#endif
	QCC_PR_PrintScope();
	if (flag_msvcstyle)
		externs->Printf ("%s%s(%i) : %serror%s: %s\n", col_location, s_filen, pr_source_line, col_error, col_none, string);
	else
		externs->Printf ("%s%s:%i: %serror%s: %s\n", col_location, s_filen, pr_source_line, col_error, col_none, string);

	QCC_PR_ParsePrintDef(WARN_ERROR, def);

	longjmp (pr_parse_abort, 1);
}

NORETURN void VARGS QCC_PR_ParseErrorPrintSRef (int errortype, QCC_sref_t def, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

#ifndef QCC
	editbadfile(s_filen, pr_source_line);
#endif
	QCC_PR_PrintScope();
	if (flag_msvcstyle)
		externs->Printf ("%s%s(%i) : %serror%s: %s\n", col_location, s_filen, pr_source_line, col_error, col_none, string);
	else
		externs->Printf ("%s%s:%i: %serror%s: %s\n", col_location, s_filen, pr_source_line, col_error, col_none, string);

	QCC_PR_ParsePrintSRef(WARN_ERROR, def);

	longjmp (pr_parse_abort, 1);
}

static pbool VARGS QCC_PR_PrintWarning (int type, const char *file, int line, const char *string)
{
	char *wnam = QCC_NameForWarning(type);
	if (!wnam)
		wnam = "";

	if (string)
		QCC_PR_PrintScope();

	if (type >= ERR_PARSEERRORS)
	{
		if (!string)
			;
		else if (!file || !*file)
			externs->Printf (":: %serror%s%s: %s\n", col_error, wnam, col_none, string);
		else if (flag_msvcstyle)
			externs->Printf ("%s%s(%i) : %serror%s%s: %s\n", col_location, file, line, col_error, wnam, col_none, string);
		else
			externs->Printf ("%s%s:%i: %serror%s%s: %s\n", col_location, file, line, col_error, wnam, col_none, string);
		pr_error_count++;
	}
	else if (qccwarningaction[type] == 2)
	{	//-werror
		if (!string)
			;
		else if (!file || !*file)
			externs->Printf (":: %swerror%s%s: %s\n", col_error, wnam, col_none, string);
		else if (flag_msvcstyle)
			externs->Printf ("%s%s(%i) : %swerror%s%s: %s\n", col_location, file, line, col_error, wnam, col_none, string);
		else
			externs->Printf ("%s%s:%i: %swerror%s%s: %s\n", col_location, file, line, col_error, wnam, col_none, string);
		pr_error_count++;
	}
	else
	{
		if (!string)
			;
		else if (!file || !*file)
			externs->Printf (":: %swarning%s%s: %s\n", col_warning, wnam, col_none, string);
		else if (flag_msvcstyle)
			externs->Printf ("%s%s(%i) : %swarning%s%s: %s\n", col_location, file, line, col_warning, wnam, col_none, string);
		else
			externs->Printf ("%s%s:%i: %swarning%s%s: %s\n", col_location, file, line, col_warning, wnam, col_none, string);
		pr_warning_count++;
	}
	return true;
}
pbool VARGS QCC_PR_Warning (int type, const char *file, int line, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!qccwarningaction[type])
		return false;

	if (!error)
		return QCC_PR_PrintWarning(type, file, line, NULL);

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	return QCC_PR_PrintWarning(type, file, line, string);
}

//can be used for errors, qcc execution will continue.
pbool VARGS QCC_PR_ParseWarning (int type, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!qccwarningaction[type])
		return false;

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	return QCC_PR_PrintWarning(type, s_filen, pr_source_line, string);
}

void VARGS QCC_PR_Note (int type, const char *file, int line, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!qccwarningaction[type])
		return;

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	QCC_PR_PrintScope();
	if (!file)
		externs->Printf ("note: %s\n", string);
	else if (flag_msvcstyle)
		externs->Printf ("%s(%i) : note: %s\n", file, line, string);
	else
		externs->Printf ("%s:%i: note: %s\n", file, line, string);
}


/*
=============
PR_Expect

Issues an error if the current token isn't equal to string
Gets the next token
=============
*/
#ifndef COMMONINLINES
void QCC_PR_Expect (const char *string)
{
	if (STRCMP (string, pr_token))
	{
		if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
		{
			if (pr_immediate_strlen > 32)
				QCC_PR_ParseError (ERR_EXPECTED, "expected %s%s%s, found string immediate", col_location, string, col_none);
			else
				QCC_PR_ParseError (ERR_EXPECTED, "expected %s%s%s, found %s\"%s\"%s", col_location, string, col_none, col_name, pr_token, col_none);
		}
		else if (pr_token_type == tt_eof)
			QCC_PR_ParseError (ERR_EXPECTED, "expected %s%s%s, found %s%s%s", col_location, string, col_none, col_name, "<EOF>", col_none);
		else
			QCC_PR_ParseError (ERR_EXPECTED, "expected %s%s%s, found %s%s%s", col_location, string, col_none, col_name, pr_token, col_none);
	}
	QCC_PR_Lex ();
}
#endif

void QCC_PR_LexComment(char **comment)
{
	char c;
	char *start;
	int nl;
	char *old;
	int oldlen;
	pbool replace = true;
	pbool nextcomment = true;

	// skip whitespace
	nl = false;
	while(nextcomment)
	{
		nextcomment = false;

		while ((c = *pr_file_p) && qcc_iswhite(c))
		{
			if (qcc_islineending(c, pr_file_p[1]))	//allow new lines, but only if there's whitespace before any tokens, and no double newlines.
			{
				if (nl)
				{
					pr_file_p++;
					QCC_PR_NewLine(false);
					break;
				}
				nl = true;
			}
			else
			{
				pr_file_p++;
				nl = false;
			}
		}
		if (nl)
			break;

		// parse // comments
		if (c=='/' && pr_file_p[1] == '/')
		{
			pr_file_p += 2;
			while (*pr_file_p == ' ' || *pr_file_p == '\t')
				pr_file_p++;
			start = pr_file_p;
			while (*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;

			if (*pr_file_p == '\n')
			{
				pr_file_p++;
				QCC_PR_NewLine(false);
			}

			old = replace?NULL:*comment;
			replace = false;
			oldlen = old?strlen(old)+1:0;
			*comment = qccHunkAlloc(oldlen + (pr_file_p-start)+1);
			if (oldlen)
			{
				memcpy(*comment, old, oldlen-1);
				memcpy(*comment+oldlen-1, "\n", 1);
			}
			memcpy(*comment + oldlen, start, pr_file_p - start);
			oldlen = oldlen+pr_file_p - start;
			while(oldlen > 0 && ((*comment)[oldlen-1] == '\r' || (*comment)[oldlen-1] == '\n' || (*comment)[oldlen-1] == '\t' || (*comment)[oldlen-1] == ' '))
				oldlen--;
			(*comment)[oldlen] = 0;
			nextcomment = true;	//include the next // too
			nl = true;
		}
		// parse /* comments
		else if (c=='/' && pr_file_p[1] == '*' && replace)
		{
			pr_file_p+=1;
			start = pr_file_p+1;

			do
			{
				pr_file_p++;
				if (pr_file_p[0]=='\n')
				{
					QCC_PR_NewLine(true);
				}
				else if (pr_file_p[1] == 0)
				{
					QCC_PR_ParseError(0, "EOF inside comment\n");
					break;
				}
				if (pr_file_p[0] == '/' && pr_file_p[1] == '*')
					QCC_PR_ParseWarning(WARN_NESTEDCOMMENT, "\"/*\" inside comment");
			} while (pr_file_p[0] != '*' || pr_file_p[1] != '/');

			if (pr_file_p[1] == 0)
				break;

			old = replace?NULL:*comment;
			replace = false;
			oldlen = old?strlen(old):0;
			*comment = qccHunkAlloc(oldlen + (pr_file_p-start)+1);
			memcpy(*comment, old, oldlen);
			memcpy(*comment + oldlen, start, pr_file_p - start);
			(*comment)[oldlen+pr_file_p - start] = 0;

			pr_file_p+=2;
		}
	}

	QCC_PR_LexWhitespace(false);
}

pbool QCC_PR_CheckTokenComment(const char *string, char **comment)
{
	if (pr_token_type != tt_punct)
		return false;

	if (STRCMP (string, pr_token))
		return false;

	if (comment)
		QCC_PR_LexComment(comment);

	//and then do the rest properly.
	QCC_PR_Lex ();
	return true;
}
/*
=============
PR_Check

Returns true and gets the next token if the current token equals string
Returns false and does nothing otherwise
=============
*/
#ifndef COMMONINLINES
pbool QCC_PR_CheckToken (const char *string)
{
	if (pr_token_type != tt_punct)
		return false;

	if (STRCMP (string, pr_token))
		return false;

	QCC_PR_Lex ();
	return true;
}

pbool QCC_PR_PeekToken (const char *string)
{
	if (pr_token_type != tt_punct)
		return false;
	if (STRCMP (string, pr_token))
		return false;
	return true;
}

pbool QCC_PR_CheckImmediate (const char *string)
{
	if (pr_token_type != tt_immediate)
		return false;

	if (STRCMP (string, pr_token))
		return false;

	QCC_PR_Lex ();
	return true;
}

pbool QCC_PR_CheckName(const char *string)
{
	if (pr_token_type != tt_name)
		return false;
	if (flag_caseinsensitive)
	{
		if (stricmp (string, pr_token))
			return false;
	}
	else
	{
		if (STRCMP(string, pr_token))
			return false;
	}
	QCC_PR_Lex ();
	return true;
}

pbool QCC_PR_CheckKeyword(int keywordenabled, const char *string)
{
	if (pr_token[0] == '_' && pr_token[1] == '_')
	{
		//lets just always go insensitive with a leading underscore pair.
		if (stricmp(string, pr_token+2))
			return false;
		QCC_PR_Lex ();
		return true;
	}
	else
	{
		if (!keywordenabled)
			return false;
		if (flag_caseinsensitive)
		{
			if (stricmp (string, pr_token))
				return false;
		}
		else
		{
			if (STRCMP(string, pr_token))
				return false;
		}
	}
	QCC_PR_Lex ();
	return true;
}
#endif


/*
============
PR_ParseName

Checks to see if the current token is a valid name
============
*/
char *QCC_PR_ParseName (void)
{
	char	ident[MAX_NAME];
	char *ret;

	if (pr_token_type != tt_name)
	{
		if (pr_token_type == tt_eof)
			QCC_PR_ParseError (ERR_EOF, "unexpected EOF");
		else if (strcmp(pr_token, "..."))	//seriously? someone used '...' as an intrinsic NAME?
			QCC_PR_ParseError (ERR_NOTANAME, "\"%s%s%s\" - not a name", col_name, pr_token, col_none);
	}
	if (strlen(pr_token) >= MAX_NAME-1)
		QCC_PR_ParseError (ERR_NAMETOOLONG, "name too long");
	strcpy (ident, pr_token);
	QCC_PR_Lex ();

	ret = qccHunkAlloc(strlen(ident)+1);
	strcpy(ret, ident);
	return ret;
//	return ident;
}

/*
============
PR_FindType

Returns a preexisting complex type that matches the parm, or allocates
a new one and copies it out.
============
*/

//requires EVERYTHING to be the same
static int typecmp_strict(QCC_type_t *a, QCC_type_t *b)
{
	int i;
	if (a == b)
		return 0;
	if (!a || !b)
		return 1;	//different (^ and not both null)

	if (a->type != b->type)
		return 1;
	if (a->num_parms != b->num_parms)
		return 1;
	if (a->vargs != b->vargs)
		return 1;
	if (a->vargcount != b->vargcount)
		return 1;

	if (a->size != b->size || a->bits != b->bits || a->align != b->align)
		return 1;

	if (a->accessors != b->accessors)
		return 1;

	if (STRCMP(a->name, b->name))
		return 1;

	if (typecmp_strict(a->aux_type, b->aux_type))
		return 1;

	i = a->num_parms;
	while(i-- > 0)
	{
		if (!a->params[i].paramname || !b->params[i].paramname)
		{
			if (a->params[i].paramname || b->params[i].paramname)
				return 1;	//two array types getting compared?
		}
		else if (STRCMP(a->params[i].paramname, b->params[i].paramname))
			return 1;
		if (typecmp_strict(a->params[i].type, b->params[i].type))
			return 1;
		if (a->params[i].defltvalue.cast || b->params[i].defltvalue.cast)
		{
			if (typecmp_strict(a->params[i].defltvalue.cast, b->params[i].defltvalue.cast) ||
				a->params[i].defltvalue.sym != b->params[i].defltvalue.sym ||
				a->params[i].defltvalue.ofs != b->params[i].defltvalue.ofs)
				return 1;
		}
	}

	return 0;
}

//reports if they're functionally equivelent (allows assignments) 
int typecmp(QCC_type_t *a, QCC_type_t *b)
{
	int i;
	if (a == b)
		return 0;
	if (!a || !b)
		return 1;	//different (^ and not both null)

	if (a->type != b->type)
		return 1;
	if (a->num_parms != b->num_parms)
		return 1;
	if (a->vargs != b->vargs)
		return 1;
	if (a->vargcount != b->vargcount)
		return 1;

	if (a->size != b->size || a->bits != b->bits)
		return 1;

	if ((a->type == ev_entity && a->parentclass) || a->type == ev_struct || a->type == ev_union)
	{
		if (STRCMP(a->name, b->name))
			return 1;
	}

	if (typecmp(a->aux_type, b->aux_type))
		return 1;

	i = a->num_parms;
	while(i-- > 0)
	{
		if (a->type == ev_union && (!a->params[i].paramname || !b->params[i].paramname))	//special array weirdness.
		{
			if (a->params[i].paramname || b->params[i].paramname)
				return 1;
		}
		else if (a->type != ev_function && STRCMP(a->params[i].paramname, b->params[i].paramname))
			return 1;
		if (typecmp(a->params[i].type, b->params[i].type))
			return 1;
		if ((a->type != ev_function && (a->params[i].defltvalue.cast || b->params[i].defltvalue.cast)) ||
			(a->type == ev_function && (a->params[i].defltvalue.cast && b->params[i].defltvalue.cast)))
		{
			if (typecmp(a->params[i].defltvalue.cast, b->params[i].defltvalue.cast) ||
				a->params[i].defltvalue.sym != b->params[i].defltvalue.sym ||
				a->params[i].defltvalue.ofs != b->params[i].defltvalue.ofs)
				return 1;
		}
	}

	return 0;
}

//compares the types, but doesn't complain if there are optional arguments which differ
int typecmp_lax(QCC_type_t *a, QCC_type_t *b)
{
	unsigned int minargs = 0;
	unsigned int t;

	if (a == b)
		return 0;
	if (!a || !b)
		return 1;	//different (^ and not both null)

	if (a->type != b->type)
	{
		if (a->type == ev_accessor && a->parentclass)
			if (!typecmp_lax(a->parentclass, b))
				return 0;
		if (b->type == ev_accessor && b->parentclass)
			if (!typecmp_lax(a, b->parentclass))
				return 0;
		if (a->type != ev_variant && b->type != ev_variant)
			return 1;
	}
	else if (a->size != b->size)
		return 1;

	if (a->vargcount != b->vargcount)
		return 1;

	t = a->num_parms;
	minargs = t;
	t = b->num_parms;
	if (minargs > t)
		minargs = t;

//	if (STRCMP(a->name, b->name))	//This isn't 100% clean.
//		return 1;

	if (typecmp_lax(a->aux_type, b->aux_type))
		return 1;

	//variants and args don't make sense, and are considered equivelent(ish).
	if (a->type == ev_variant || b->type == ev_variant)
		return 0;

	//optional arg types must match, even if they're not specified in one.
	for (t = 0; t < minargs; t++)
	{
		if (a->params[t].type->type != b->params[t].type->type)
			return 1;
		if (a->params[t].out != b->params[t].out)
			return 1;
		//classes/structs/unions are matched on class names rather than the contents of the class
		//it gets too painful otherwise, with recursive definitions.
		if (a->params[t].type->type == ev_entity || a->params[t].type->type == ev_struct || a->params[t].type->type == ev_union)
		{
			if (STRCMP(a->params[t].type->name, b->params[t].type->name))
				return 1;
		}
		else
		{
			if (typecmp_lax(a->params[t].type, b->params[t].type))
				return 1;
		}

		if ((a->type != ev_function && (a->params[t].defltvalue.cast || b->params[t].defltvalue.cast)) ||
			(a->type == ev_function && (a->params[t].defltvalue.cast && b->params[t].defltvalue.cast)))
		{
			if (typecmp(a->params[t].defltvalue.cast, b->params[t].defltvalue.cast) ||
				a->params[t].defltvalue.sym != b->params[t].defltvalue.sym ||
				a->params[t].defltvalue.ofs != b->params[t].defltvalue.ofs)
				return 1;
		}
	}
	if (a->num_parms > minargs)
	{
		for (t = minargs; t < a->num_parms; t++)
		{
			if (!a->params[t].optional)
				return 1;
		}
	}
	if (b->num_parms > minargs)
	{
		for (t = minargs; t < b->num_parms; t++)
		{
			if (!b->params[t].optional)
				return 1;
		}
	}

	return 0;
}


QCC_type_t *QCC_PR_DuplicateType(QCC_type_t *in, pbool recurse)
{
	QCC_type_t *out;
	if (!in)
		return NULL;

	out = QCC_PR_NewType(in->name, in->type, false);
	out->aux_type = recurse?QCC_PR_DuplicateType(in->aux_type, recurse):in->aux_type;
	out->num_parms = in->num_parms;
	out->params = qccHunkAlloc(sizeof(*out->params) * out->num_parms);
	memcpy(out->params, in->params, sizeof(*out->params) * out->num_parms);
	out->accessors = in->accessors;
	out->size = in->size;
	out->bits = in->bits;
	out->align = in->align;
	out->num_parms = in->num_parms;
	out->name = in->name;
	out->parentclass = in->parentclass;
	out->vargs = in->vargs;
	out->vargtodouble = in->vargtodouble;
	out->vargcount = in->vargcount;

	return out;
}

static void Q_strlcat(char *dest, const char *src, int sizeofdest)
{
	if (sizeofdest)
	{
		int dlen = strlen(dest);
		int slen = strlen(src)+1;
		memcpy(dest+dlen, src, min((sizeofdest-1)-dlen, slen));
		dest[sizeofdest - 1] = 0;
	}
}

char *TypeName(QCC_type_t *type, char *buffer, int buffersize)
{
	char *ret;
	/*if (type->typedefed)
	{
		if (buffersize < 0)
			return buffer;
		*buffer = 0;
		Q_strlcat(buffer, type->name, buffersize);
		return buffer;
	}*/

	if (type->type == ev_void)
	{
		if (buffersize < 0)
			return buffer;
		*buffer = 0;
		Q_strlcat(buffer, "void", buffersize);
		return buffer;
	}
	if (type->type == ev_enum)
	{
		if (buffersize < 0)
			return buffer;
		*buffer = 0;
		Q_strlcat(buffer, "enum ", buffersize);
		Q_strlcat(buffer, type->name, buffersize);
		Q_strlcat(buffer, ":", buffersize);
		TypeName(type->aux_type, buffer+strlen(buffer), buffersize-strlen(buffer));
		return buffer;
	}
	if (type->type == ev_struct)
	{
		if (buffersize < 0)
			return buffer;
		*buffer = 0;
		Q_strlcat(buffer, "struct ", buffersize);
		Q_strlcat(buffer, type->name, buffersize);
		return buffer;
	}
	if (type->type == ev_union)
	{
		if (buffersize < 0)
			return buffer;
		*buffer = 0;
		Q_strlcat(buffer, "union ", buffersize);
		Q_strlcat(buffer, type->name, buffersize);
		return buffer;
	}

	if (type->type == ev_pointer)
	{
		if (buffersize < 0)
			return buffer;
		TypeName(type->aux_type, buffer, buffersize-2);
		Q_strlcat(buffer, "*", buffersize);
		return buffer;
	}

	ret = buffer;
	if (type->type == ev_field)
	{
		type = type->aux_type;
		*ret++ = '.';
	}
	*ret = 0;

	if (type->type == ev_function)
	{
		int args = type->num_parms;
		size_t l;
		pbool vargs = type->vargs;
		unsigned int i;
		Q_strlcat(buffer, type->aux_type->name, buffersize);
		if (type->vargtodouble)
			Q_strlcat(buffer, "(*)", buffersize);	//make it distinctly C-ey
		Q_strlcat(buffer, "(", buffersize);
		for (i = 0; i < type->num_parms; )
		{
			if (type->params[i].out)
				Q_strlcat(buffer, "inout ", buffersize-1);
			if (type->params[i].optional)
				Q_strlcat(buffer, "optional ", buffersize-1);
			args--;

			l = strlen(buffer);
			TypeName(type->params[i].type, buffer+l, buffersize-1-l);
			if (type->params[i].paramname && *type->params[i].paramname)
			{
				Q_strlcat(buffer, " ", buffersize-1);
				Q_strlcat(buffer, type->params[i].paramname, buffersize-1);
			}

			if (type->params[i].defltvalue.cast)
			{
				Q_strlcat(buffer, " = ", buffersize-1);
				Q_strlcat(buffer, QCC_VarAtOffset(type->params[i].defltvalue), buffersize-1);
			}

			if (++i < type->num_parms || vargs)
				Q_strlcat(buffer, ", ", buffersize-1);
		}
		if (vargs)
			Q_strlcat(buffer, "...", buffersize-1);
		Q_strlcat(buffer, ")", buffersize);
	}
	else if (type->type == ev_entity && type->parentclass)
	{
		ret = buffer;
		*ret = 0;
		Q_strlcat(buffer, "class ", buffersize);
		Q_strlcat(buffer, type->name, buffersize);
/*		strcat(ret, " {");
		type = type->param;
		while(type)
		{
			strcat(ret, type->name);
			type = type->next;

			if (type)
				strcat(ret, ", ");
		}
		strcat(ret, "}");
*/
	}
	else
		Q_strlcat(buffer, type->name, buffersize);

	return buffer;
}
//#define typecmp(a, b) (a && ((a)->type==(b)->type) && !STRCMP((a)->name, (b)->name))

static QCC_type_t *QCC_PR_FindType (QCC_type_t *type)
{
	int t;
	for (t = 0; t < numtypeinfos; t++)
	{
//		check = &qcc_typeinfo[t];
		if (typecmp_strict(&qcc_typeinfo[t], type))
			continue;

//		c2 = check->next;
//		n2 = type->next;
//		for (i=0 ; n2&&c2 ; i++)
//		{
//			if (!typecmp((c2), (n2)))
//				break;
//			c2=c2->next;
//			n2=n2->next;
//		}

//		if (n2==NULL&&c2==NULL)
		{
			return &qcc_typeinfo[t];
		}
	}

	type = QCC_PR_DuplicateType(type, false);
	return type;
}
/*
QCC_type_t *QCC_PR_NextSubType(QCC_type_t *type, QCC_type_t *prev)
{
	int p;
	if (!prev)
		return type->next;

	for (p = prev->num_parms; p; p--)
		prev = QCC_PR_NextSubType(prev, NULL);
	if (prev->num_parms)

	switch(prev->type)
	{
	case ev_function:

	}

	return prev->next;
}
*/

QCC_type_t *QCC_TypeForName(const char *name)
{
	QCC_type_t *t = pHash_Get(&typedeftable, name);
	while (t && t->scope)
	{
		if (t->scope == pr_scope)
			break;	//its okay after all.
		t = pHash_GetNext(&typedeftable, name, t);
	}
	if (t && t->type == ev_typedef)
		return t->aux_type;	//just use its real type.
	return t;
}

/*
============
PR_SkipToSemicolon

For error recovery, also pops out of nested braces
============
*/
void QCC_PR_SkipToSemicolon (void)
{
	//escape out of any #define
	while (currentchunk && currentchunk->cnst)
		QCC_PR_UnInclude();

	do
	{
		if (!pr_bracelevel && QCC_PR_CheckToken (";"))
			return;
		QCC_PR_Lex ();
	} while (pr_token_type != tt_eof);
}

qc_inlinestatic QCC_eval_t *QCC_SRef_Data(QCC_sref_t ref)
{
	return (QCC_eval_t*)&ref.sym->symboldata[ref.ofs];
}

/*
============
PR_ParseType

Parses a variable type, including field and functions types
============
*/
#ifdef MAX_EXTRA_PARMS
char	pr_parm_names[MAX_PARMS+MAX_EXTRA_PARMS][MAX_NAME];
#else
char	pr_parm_names[MAX_PARMS][MAX_NAME];
#endif
char *pr_parm_argcount_name;

int recursivefunctiontype;

QCC_type_t *QCC_PR_MakeThiscall(QCC_type_t *orig, QCC_type_t *thistype)
{
	QCC_type_t	ftype = *orig;

	ftype.ptrto = NULL;
	ftype.typedefed = false;
	ftype.num_parms++;
	ftype.params = qccHunkAlloc(sizeof(*ftype.params) * ftype.num_parms);
	memcpy(ftype.params+1, orig->params, sizeof(*ftype.params) * orig->num_parms);
	ftype.params[0].paramname = "this";
	ftype.params[0].type = QCC_PR_PointerType(thistype);
	ftype.params[0].isvirtual = true;

	memmove(&pr_parm_names[1], &pr_parm_names[0], sizeof(*pr_parm_names)*orig->num_parms);
	strcpy(pr_parm_names[0], "this");

	orig = QCC_PR_FindType (&ftype);
	if (!orig)
	{
		orig = QCC_PR_NewType(ftype.name, ftype.type, false);
		*orig = ftype;
	}
	return orig;
}

QCC_type_t *QCC_PR_ParseArrayType(QCC_type_t *basetype, int *arraysize)
{
	int dims = 0;
	unsigned long dim[64];
	dim[0] = 0;
	do
	{
		if (dims == sizeof(dim)/sizeof(dim[0]))
			QCC_PR_ParseError(ERR_NOTANAME, "too many dimensions");
		dim[dims] = QCC_PR_IntConstExpr();
		if (!dim[dims])
			QCC_PR_ParseError(ERR_NOTANAME, "cannot cope with 0-sized arrays");
		dims++;
		QCC_PR_Expect("]");
	} while (QCC_PR_CheckToken("["));
	while(dims-- > 1)
		basetype = QCC_GenArrayType(basetype, dim[dims]);
	*arraysize = dim[0];
	return basetype;
}

//expects a ( to have already been parsed.
QCC_type_t *QCC_PR_ParseFunctionType (int newtype, QCC_type_t *returntype)
{
	QCC_type_t	*ftype = NULL, *t;
	char	*name;
	int definenames = !recursivefunctiontype;
	int numparms = 0;
	struct QCC_typeparam_s paramlist[MAX_PARMS+MAX_EXTRA_PARMS];

	if (QCC_PR_PeekToken("*"))
		return NULL;	//C pointer-to-function. this ain't the args.

	recursivefunctiontype++;

	if (definenames)
		pr_parm_argcount_name = NULL;

	if (QCC_PR_CheckToken (")"))
	{
		if (!ftype)
		{
			ftype = QCC_PR_NewType(type_function->name, ev_function, false);
			ftype->aux_type = returntype;	// return type
			ftype->num_parms = 0;
		}
		if (!flag_qcfuncs)
			ftype->vargs = true;	//'void name()' is vargs/undefined in C89 (disallowed in c99, interpreted as void in c23).
	}
	else
	{
		do
		{
			int inout = -1;
			pbool isopt = false;
			if (numparms>=MAX_PARMS+MAX_EXTRA_PARMS)
				QCC_PR_ParseError(ERR_TOOMANYTOTALPARAMETERS, "Too many parameters. Sorry. (limit is %i)\n", MAX_PARMS+MAX_EXTRA_PARMS);

			if (QCC_PR_CheckToken ("..."))
			{
				t = QCC_PR_ParseType(false, true, false);	//the evil things I do...
				if (!t)
				{
					if (!ftype)
					{
						ftype = QCC_PR_NewType(type_function->name, ev_function, false);
						ftype->aux_type = returntype;	// return type
						ftype->num_parms = 0;
					}
					ftype->vargs = true;
					break;
				}
				else
				{	//its a ... followed by a type...
					//undo the damage from having parsed the ... already.
					t = QCC_PR_FieldType(t);
					t = QCC_PR_FieldType(t);
					t = QCC_PR_FieldType(t);
				}
			}
			else
			{
				pbool maybename = numparms==0;
				while(1)
				{
					if (!isopt && QCC_PR_CheckKeyword(keyword_optional, "optional"))
						isopt = true;
					else if (inout<0 && QCC_PR_CheckKeyword(keyword_inout, "inout"))
						inout = true;
					else if (inout<0 && QCC_PR_CheckKeyword(keyword_inout, "out"))
						inout = 2;
					else if (inout<0 && QCC_PR_CheckKeyword(keyword_inout, "in"))
						inout = false;
					else
						break;
					maybename=false;	//if we parsed something meaningful then its not a `void(name)(type)` thing
				}

				t = QCC_PR_ParseType(false, maybename, false);
				if (!t)
				{
					if (!flag_qcfuncs)
						t = type_integer;	//c89 assumes int.
					else
						return NULL;
				}
			}

			if (!ftype)
			{
				ftype = QCC_PR_NewType(type_function->name, ev_function, false);
				ftype->aux_type = returntype;	// return type
				ftype->num_parms = 0;
			}

			paramlist[numparms].optional = isopt;
			paramlist[numparms].isvirtual = false;
			paramlist[numparms].out = inout>=0?inout:0;
			paramlist[numparms].defltvalue.cast = NULL;
			paramlist[numparms].ofs = 0;
			paramlist[numparms].arraysize = 0;
			paramlist[numparms].type = t;
			if (!paramlist[numparms].type)
				QCC_PR_ParseError(0, "Expected type\n");

			while (QCC_PR_CheckToken("*"))
				paramlist[numparms].type = QCC_PointerTypeTo(paramlist[numparms].type);

			if (inout < 0 && QCC_PR_CheckToken("&"))
			{	//accept c++ syntax, at least on arguments. its not quite the same, but it'll do.
				paramlist[numparms].out = true;
			}

//			type->name = "FUNC PARAMETER";

			paramlist[numparms].paramname = "";
			if (QCC_PR_CheckToken ("..."))
			{
				ftype->vargs = true;
				break;
			}
			name = "";
			if (QCC_PR_CheckToken("("))
			{
				QCC_PR_CheckToken("*");	//one is normal for a function pointer/ref. non-ptr makes no sense here.
				name = QCC_PR_ParseName ();
				if (QCC_PR_CheckToken("["))
				{
					if (!QCC_PR_CheckToken("]"))
					{	//don't really care, doesn't matter
						paramlist[numparms].arraysize = QCC_PR_IntConstExpr();
						QCC_PR_Expect("]");
					}
				}
				QCC_PR_Expect(")");

				QCC_PR_Expect("(");
				paramlist[numparms].type = QCC_PR_ParseFunctionType(false, paramlist[numparms].type);
				if (!paramlist[numparms].type)
					QCC_PR_ParseError(ERR_BADNOTTYPE, "expected function arg list");
			}
			else if (pr_token_type == tt_name)
				name = QCC_PR_ParseName ();
			paramlist[numparms].paramname = name;
			if (definenames)
				strcpy (pr_parm_names[numparms], name);
			if (*name)
				newtype = true;
			else if (paramlist[numparms].type->type == ev_void)
				break; //float(void) has no actual args

			if (!paramlist[numparms].arraysize && QCC_PR_CheckToken("["))
			{
				if (QCC_PR_CheckToken("]"))	//length omitted. just treat it as a pointer...?
				{
					//QCC_PR_ParseWarning(ERR_BADARRAYSIZE, "unsized array argument");
					paramlist[numparms].type = QCC_PointerTypeTo(paramlist[numparms].type);
				}
				else
				{	//proper array
					int arraysize;
					paramlist[numparms].type = QCC_PR_ParseArrayType(paramlist[numparms].type, &arraysize);

					if (flag_qcfuncs)
						paramlist[numparms].arraysize = arraysize;
					else
						paramlist[numparms].type = QCC_PointerTypeTo(paramlist[numparms].type);	//just turn it into a pointer and ditch the top-level size. C style.
				}
			}

			if (!flag_qcfuncs)
			{	//if its an array type, promote it to pointer here.
				if (t->type == ev_union && t->num_parms == 1 && !t->params[0].paramname)
					paramlist[numparms].type = QCC_PointerTypeTo(t->params[0].type);
			}

			if (QCC_PR_CheckToken("="))
			{
				paramlist[numparms].defltvalue = QCC_PR_ParseDefaultInitialiser(paramlist[numparms].type);
				QCC_FreeTemp(paramlist[numparms].defltvalue);
			}
			numparms++;
		} while (QCC_PR_CheckToken (","));

		if (ftype->vargs)
		{
			if (!QCC_PR_CheckToken (")"))
			{
				name = QCC_PR_ParseName();
				if (definenames)
				{
					pr_parm_argcount_name = qccHunkAlloc(strlen(name)+1);
					strcpy(pr_parm_argcount_name, name);
				}
				ftype->vargcount = true;
				QCC_PR_Expect (")");
			}
		}
		else
			QCC_PR_Expect (")");
	}
	ftype->vargtodouble = !flag_qcfuncs && flag_assume_double;
	ftype->num_parms = numparms;
	ftype->params = qccHunkAlloc(sizeof(*ftype->params) * numparms);
	memcpy(ftype->params, paramlist, sizeof(*ftype->params) * numparms);
	recursivefunctiontype--;

	if (newtype)
		return ftype;
	return QCC_PR_FindType (ftype);
}
QCC_type_t *QCC_PR_ParseFunctionTypeReacc (int newtype, QCC_type_t *returntype)
{
	QCC_type_t	*ftype, *nptype;
	char	*name;
	int definenames = !recursivefunctiontype;
	int numparms = 0;
	struct QCC_typeparam_s paramlist[MAX_PARMS+MAX_EXTRA_PARMS];

	recursivefunctiontype++;

	ftype = QCC_PR_NewType(type_function->name, ev_function, false);

	ftype->aux_type = returntype;	// return type
	ftype->num_parms = 0;

	pr_parm_argcount_name = NULL;

	if (!QCC_PR_CheckToken (")"))
	{
		do
		{
			if (numparms>=MAX_PARMS+MAX_EXTRA_PARMS)
				QCC_PR_ParseError(ERR_TOOMANYTOTALPARAMETERS, "Too many parameters. Sorry. (limit is %i)\n", MAX_PARMS+MAX_EXTRA_PARMS);

			if (QCC_PR_CheckToken ("..."))
			{
				ftype->vargs = true;
				break;
			}

			if (QCC_PR_CheckName("arg"))
			{
				name = "";
				nptype = QCC_PR_NewType("Variant", ev_variant, false);
			}
			else if (QCC_PR_CheckName("vect"))	//this can only be of vector sizes, so...
			{
				name = "";
				nptype = QCC_PR_NewType("Vector", ev_vector, false);
			}
			else
			{
				name = QCC_PR_ParseName();
				QCC_PR_Expect(":");
				nptype = QCC_PR_ParseType(true, false, false);
			}

			if (!nptype)
				QCC_PR_ParseError(0, "Expected type\n");
			if (nptype->type == ev_void)
				break;
//			type->name = "FUNC PARAMETER";

			paramlist[numparms].out = false;
			paramlist[numparms].optional = false;
			paramlist[numparms].isvirtual = false;
			paramlist[numparms].ofs = 0;
			paramlist[numparms].bitofs = 0;
			paramlist[numparms].arraysize = 0;
			paramlist[numparms].type = nptype;

			if (!*name)
				paramlist[numparms].paramname = "";
			else
			{
				paramlist[numparms].paramname = qccHunkAlloc(strlen(name)+1);
				strcpy(paramlist[numparms].paramname, name);
			}
			if (definenames)
				QC_snprintfz(pr_parm_names[numparms], MAX_NAME, "%s", name);

			numparms++;
		} while (QCC_PR_CheckToken (";"));
		QCC_PR_Expect (")");
	}
	ftype->num_parms = numparms;
	ftype->params = qccHunkAlloc(sizeof(*ftype->params) * numparms);
	memcpy(ftype->params, paramlist, sizeof(*ftype->params) * numparms);
	recursivefunctiontype--;
	if (newtype)
		return ftype;
	return QCC_PR_FindType (ftype);
}
QCC_type_t *QCC_PR_PointerType (QCC_type_t *pointsto)
{
	QCC_type_t	*ptype;
	char name[128];
	if (pointsto->ptrto)
		return pointsto->ptrto;
	QC_snprintfz(name, sizeof(name), "%s*", pointsto->name);
	ptype = QCC_PR_NewType(strcpy(qccHunkAlloc(strlen(name)+1), name), ev_pointer, false);
	ptype->aux_type = pointsto;
	return pointsto->ptrto = QCC_PR_FindType (ptype);
}
QCC_type_t *QCC_PR_FieldType (QCC_type_t *pointsto)
{
	QCC_type_t	*ptype;
	char name[128];
	if (pointsto->fldto)
		return pointsto->fldto;
	QC_snprintfz(name, sizeof(name), ".%s", pointsto->name);
	ptype = QCC_PR_NewType(strcpy(qccHunkAlloc(strlen(name)+1), name), ev_field, false);
	ptype->aux_type = pointsto;
	ptype->size = ptype->aux_type->size;
	return pointsto->fldto = QCC_PR_FindType (ptype);
}
QCC_type_t *QCC_PR_GenFunctionType (QCC_type_t *rettype, struct QCC_typeparam_s *args, int numargs)
{
	int i;
	struct QCC_typeparam_s *p;
	QCC_type_t *ftype;
	ftype = QCC_PR_NewType("$func", ev_function, false);
	ftype->aux_type = rettype;
	ftype->num_parms = numargs;
	ftype->params = p = qccHunkAlloc(sizeof(*ftype->params)*numargs);
	ftype->vargs = false;
	ftype->vargcount = false;
	for (i = 0; i < numargs; i++, p++)
	{
		if (args[i].paramname)
		{
			p->paramname = qccHunkAlloc(strlen(args[i].paramname)+1);
			strcpy(p->paramname, args[i].paramname);
		}
		else
			p->paramname = "";
		p->type = args[i].type;
		p->out = args[i].out;
		p->optional = args[i].optional;
		p->isvirtual = args[i].isvirtual;
		p->ofs = args[i].ofs;
		p->arraysize = args[i].arraysize;
		p->defltvalue.cast = NULL;
	}
	return QCC_PR_FindType (ftype);
}


struct accessor_s *QCC_PR_ParseAccessorMember(QCC_type_t *classtype, pbool isinline, pbool setnotget)
{
	struct accessor_s  *acc, *pacc;
	char *fieldtypename;
	QCC_type_t *fieldtype;
	QCC_type_t *indextype;
	QCC_sref_t def;
	QCC_type_t *functype;
	QCC_type_t *parenttype;
	struct QCC_typeparam_s arg[3];
	int args;
	char *indexname;
	pbool isref;
	char *accessorname;

	if (QCC_PR_CheckToken("&"))
		isref = 2;
	else
		isref = QCC_PR_CheckToken("*");

	fieldtypename = QCC_PR_ParseName();
	fieldtype = QCC_TypeForName(fieldtypename);
	if (!fieldtype)
		QCC_PR_ParseError(ERR_NOTATYPE, "Invalid type: %s", fieldtypename);
	while(QCC_PR_CheckToken("*"))
		fieldtype = QCC_PR_PointerType(fieldtype);

	if (pr_token_type != tt_punct)
		accessorname = QCC_PR_ParseName();
	else
		accessorname = "";

	indextype = NULL;
	indexname = "index";
	if (QCC_PR_CheckToken("["))
	{
		fieldtypename = QCC_PR_ParseName();
		indextype = QCC_TypeForName(fieldtypename);

		if (!QCC_PR_CheckToken("]"))
		{
			indexname = QCC_PR_ParseName();
			QCC_PR_Expect("]");
		}
	}

	QCC_PR_Expect("=");

	args = 0;
	memset(arg, 0, sizeof(arg));
	strcpy (pr_parm_names[args], "this");
	arg[args].paramname = "this";
	if (isref == 2)
	{
		arg[args].type = classtype;
		arg[args].out = 1;	//inout
	}
	else if (isref)
		arg[args].type = QCC_PointerTypeTo(classtype);
	else
		arg[args].type = classtype;
	args++;
	if (indextype)
	{
		strcpy (pr_parm_names[args], indexname);
		arg[args].paramname = indexname;
		arg[args++].type = indextype;
	}
	if (setnotget)
	{
		strcpy (pr_parm_names[args], "value");
		arg[args].paramname = "value";
		arg[args++].type = fieldtype;
	}
	functype = QCC_PR_GenFunctionType(setnotget?type_void:fieldtype, arg, args);

	if (pr_token_type != tt_name)
	{
		QCC_function_t *f;
		char funcname[256];
		QC_snprintfz(funcname, sizeof(funcname), "%s::%s_%s", classtype->name, setnotget?"set":"get", accessorname);

		def = QCC_PR_GetSRef(functype, funcname, NULL, true, 0, GDF_CONST | (isinline?GDF_INLINE:0));

		if (autoprototype)
		{
			if (QCC_PR_CheckToken("["))
			{
				while (!QCC_PR_CheckToken("]"))
				{
					if (pr_token_type == tt_eof)
						break;
					QCC_PR_Lex();
				}
			}
			QCC_PR_Expect("{");

			{
				int blev = 1;
				//balance out the { and }
				while(blev)
				{
					if (pr_token_type == tt_eof)
						break;
					if (QCC_PR_CheckToken("{"))
						blev++;
					else if (QCC_PR_CheckToken("}"))
						blev--;
					else
						QCC_PR_Lex();	//ignore it.
				}
			}
		}
		else
		{
			pr_classtype = ((classtype->type==ev_entity)?classtype:NULL);
			f = QCC_PR_ParseImmediateStatements (def.sym, functype, false);
			pr_classtype = NULL;
			pr_scope = NULL;
			QCC_SRef_Data(def)->function = f - functions;
			f->def = def.sym;
			def.sym->initialized = 1;
		}
	}
	else
	{
		const char *funcname = QCC_PR_ParseName();
		def = QCC_PR_GetSRef(functype, funcname, NULL, true, 0, GDF_CONST|(isinline?GDF_INLINE:0));
		if (!def.cast)
			QCC_Error(ERR_NOFUNC, "%s::set_%s: %s was not defined", classtype->name, accessorname, funcname);
	}
	if (!def.cast || !def.sym || def.sym->temp)
		QCC_Error(ERR_NOFUNC, "%s::%s_%s function invalid", classtype->name, setnotget?"set":"get", accessorname);

	for (acc = classtype->accessors; acc; acc = acc->next)
		if (!strcmp(acc->fieldname, accessorname))
			break;
	if (!acc)
	{
		acc = qccHunkAlloc(sizeof(*acc));
		acc->fieldname = accessorname;
		acc->next = classtype->accessors;
		acc->type = fieldtype;
		acc->indexertype = indextype;
		classtype->accessors = acc;
	}

	if (acc->getset_func[setnotget].cast && (
		acc->getset_func[setnotget].sym != def.sym ||
		acc->getset_func[setnotget].cast != def.cast ||
		acc->getset_func[setnotget].ofs != def.ofs))
		QCC_Error(ERR_TOOMANYINITIALISERS, "%s::%s_%s already declared", classtype->name, setnotget?"set":"get", accessorname);
	acc->getset_func[setnotget] = def;
	acc->getset_isref[setnotget] = isref;
	QCC_FreeTemp(def);

	for (parenttype = classtype->parentclass; parenttype; parenttype = parenttype->parentclass)
	{
		if (!parenttype->accessors)
			continue;
		for (pacc = parenttype->accessors; pacc; pacc = pacc->next)
		{
			if (!strcmp(acc->fieldname, pacc->fieldname))
				QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "%s::%s shadows parent %s", classtype->name, acc->fieldname?acc->fieldname:"<anon>", parenttype->name);
		}
	}

	return acc;
}

extern char *basictypenames[];
extern QCC_type_t **basictypes[];
static QCC_type_t *QCC_PR_ParseStruct(etype_t structtype)
{
	QCC_type_t *newt, *type, *newparm, *oldtype = NULL;
	struct QCC_typeparam_s *parms = NULL, *oldparm;
	int numparms = 0;
	int ofs, bitofs;
	unsigned int arraysize;
	char *parmname;

	pbool isnonvirt = false;
	pbool isstatic = false;
	pbool isvirt = false;
	pbool definedsomething = false;
	unsigned int bitsize;
	unsigned int bitalign = 0;	//alignment of largest element...

	if (QCC_PR_CheckToken("{"))
	{
		//nameless struct
		newt = QCC_PR_NewType(structtype==ev_union?"<union>":"<struct>", structtype, false);
	}
	else
	{
		QCC_type_t *parenttype;
		char *tname = QCC_PR_ParseName();

		if (structtype == ev_struct && QCC_PR_CheckToken(":"))
		{
			char *parentname = QCC_PR_ParseName();
			parenttype = QCC_TypeForName(parentname);
			if (!parenttype)
				QCC_PR_ParseError(ERR_NOTANAME, "Parent type %s was not yet defined", parentname);
			if (parenttype->type != ev_struct)
				QCC_PR_ParseError(ERR_NOTANAME, "Parent type %s is not a struct", parentname);
		}
		else
			parenttype = NULL;

		newt = QCC_TypeForName(tname);
		if (!newt)
		{
			newt = QCC_PR_NewType(tname, ev_struct, true);
			newt->parentclass = parenttype;
		}
		else if (!newt->size && !newt->parentclass)
			newt->parentclass = parenttype;
		else if (parenttype && newt->parentclass != parenttype)
			QCC_PR_ParseError(ERR_NOTANAME, "Redeclaration of struct with different parent type");

		//struct declaration only, not definition.
		if (parenttype)
			QCC_PR_Expect("{");
		else if (!QCC_PR_CheckToken("{"))
			return newt;

		if (newt->size)
		{
//			QCC_PR_ParseError(ERR_NOTANAME, "%s %s is already defined", structtype==ev_union?"union":"struct", newt->name);
			oldtype = newt;
			newt = QCC_PR_NewType(tname, ev_struct, false);
			newt->parentclass = parenttype;
		}
	}
	if (newt->parentclass)
		newt->size = newt->parentclass->size;
	else
		newt->size=0;
	bitsize = newt->size<<5;

	type = NULL;

	newparm = NULL;
	for (;;)
	{
		//in qc, functions are assignable references like anything else, so no modifiers means the qc will need to assign to it somewhere.
		//virtual functions are still references, but we initialise them somewhere
		//nonvirtual functions and static functions are kinda the same thing
		QCC_sref_t defaultval;
		if (QCC_PR_CheckToken("}"))
		{
			if (newparm)
				QCC_PR_ParseError(ERR_EXPECTED, "missing semi-colon");
			break;
		}
		else if (QCC_PR_CheckToken(";"))
		{
			newparm = NULL;
			continue;
		}
		else if (QCC_PR_CheckToken(","))
		{	//same as last type, unless initial/after-semicolon
			if (!newparm)
				QCC_PR_ParseError(ERR_EXPECTED, "element missing type");
		}
		else
		{	//new type!
			if (newparm)
				QCC_PR_ParseError(ERR_EXPECTED, "missing semi-colon");	//allow a missing semi-colon on functions, for mixed-style functions.

			//reset these...
			isnonvirt = false;
			isstatic = false;
			isvirt = false;

			//parse field modifiers
			if (QCC_PR_CheckKeyword(1, "public"))
			{
				/*ispublic = true;*/
				QCC_PR_Expect(":");
				continue;
			}
			else if (QCC_PR_CheckKeyword(1, "private"))
			{
				/*isprivate = true;*/
				QCC_PR_Expect(":");
				continue;
			}
			else if (QCC_PR_CheckKeyword(1, "protected"))
			{
				/*isprotected = true;*/
				QCC_PR_Expect(":");
				continue;
			}

			//if (QCC_PR_CheckKeyword(1, "nonvirtual"))
			//	isnonvirt = true;
			//else
			if (QCC_PR_CheckKeyword(1, "static"))
				isstatic = true;
			else if (QCC_PR_CheckKeyword(1, "virtual"))
				isvirt = true;
//				else if (QCC_PR_CheckKeyword(1, "ignore"))
//					isignored = true;
//				else if (QCC_PR_CheckKeyword(1, "strip"))
//					isignored = true;

			//now parse the actual type.
			newparm = QCC_PR_ParseType(false, false, true);
			definedsomething = false;
		}
		type = newparm;

		while (QCC_PR_CheckToken("*"))
			type = QCC_PointerTypeTo(type);

		arraysize = 0;
		if (QCC_PR_CheckToken(";"))
		{	//annonymous structs do weird scope stuff.
			if (!definedsomething && (type->type != ev_struct && type->type != ev_union))
			{
				if (flag_qcfuncs && type->type == ev_function && type->aux_type == newt && QCC_PR_PeekToken(";"))
					QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "constructors are not supported in structs at this time");
				else
					QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "declaration does not declare anything (%s)", newparm->name);
			}
			newparm = NULL;
			parmname = "";
		}
		else
		{
			if (QCC_PR_CheckToken("("))
			{
				QCC_PR_CheckToken("*");	//this is fine...
				parmname = QCC_PR_ParseName();
				QCC_PR_Expect(")");
			}
			else
				parmname = QCC_PR_ParseName();
			definedsomething = true;
			if (QCC_PR_CheckToken("["))
				type = QCC_PR_ParseArrayType(type, &arraysize);	//Checks for [][y][x] arrays.

			if (QCC_PR_CheckToken("("))
			{
				type = QCC_PR_ParseFunctionType(false, type);
				if (!type)
					QCC_PR_ParseError(ERR_BADNOTTYPE, "expected function arg list");
			}
		}

		if (type == newt || ((type->type == ev_struct || type->type == ev_union) && !type->size))
		{
			QCC_PR_ParseWarning(ERR_NOTANAME, "type %s not fully defined yet", type->name);
			continue;
		}

		if (QCC_PR_CheckToken(":"))
		{
			int bits = QCC_PR_IntConstExpr();
			if (bits > 64)
				QCC_PR_ParseWarning(ERR_BADNOTTYPE, "too many bits");
			else if (type->type == ev_integer || type->type == ev_uint || type->type == ev_int64 || type->type == ev_uint64)
			{
				QCC_type_t *bitfld;
				//prmote to bigger if big.
				if (bits > 32 && type->type == ev_integer)
					type = type_int64;
				if (bits > 32 && type->type == ev_uint)
					type = type_uint64;
				bitfld = QCC_PR_NewType("bitfld", ev_bitfld, false);
				bitfld->bits = bits;
				bitfld->size = type->size;
				bitfld->parentclass = type;
				bitfld->align = type->align;	//use the parent's alignment, for some reason.
				type = bitfld;
			}
			else
				QCC_PR_ParseWarning(ERR_BADNOTTYPE, "bitfields must be integer types");
		}

		if ((isnonvirt || isvirt) && type->type != ev_function)
			QCC_PR_ParseWarning(ERR_INTERNAL, "[non]virtual members must be functions");

		//static members are technically just funny-named globals, and do not generate fields.
		if (isnonvirt || isstatic || isvirt)
		{	//either way its a regular global. the difference being static has no implicit this/self argument.
			QCC_def_t *d;
			char membername[2048];
			if (!isstatic)
				type = QCC_PR_MakeThiscall(type, newt);

			QC_snprintfz(membername, sizeof(membername), "%s::%s", newt->name, parmname);
			d = QCC_PR_GetDef(type, membername, NULL, true, 0, (type->type==ev_function)?GDF_CONST:0);
			if (QCC_PR_CheckToken("=") || (type->type == ev_function && QCC_PR_PeekToken("{")))
			{
//FIXME: methods cannot be compiled yet, as none of the fields are not actually defined yet.
				pr_classtype = newt;
				QCC_PR_ParseInitializerDef(d, 0);
				pr_classtype = NULL;
			}
			QCC_FreeDef(d);
			if (!QCC_PR_PeekToken(","))
				newparm = NULL;

			if (isvirt)
			{
				defaultval.ofs = 0;
				defaultval.cast = d->type;
				defaultval.sym = d;
			}
			else
				continue;
		}
		else
		{
			defaultval.cast = NULL;
			if (QCC_PR_CheckToken("="))
			{
				defaultval = QCC_PR_ParseDefaultInitialiser(type);
				QCC_PR_ParseWarning(ERR_INTERNAL, "TODO: pre-initialised struct members are not implemented yet");
			}
		}

		parms = realloc(parms, sizeof(*parms) * (numparms+4));
		oldparm = QCC_PR_FindStructMember(newt, parmname, &ofs, &bitofs);
		if (type->align)
		{
			if (bitalign < type->align)
				bitalign = type->align;	//bigger than that...
		}
		else
			bitalign = 32;	//struct must be word aligned.
		if (oldparm && oldparm->arraysize == arraysize && !typecmp_lax(oldparm->type, type))
		{
			ofs <<= 5;
			ofs += bitofs;
			if (!isvirt)
				continue;
		}
		else if (structtype == ev_union)
		{
			ofs = 0;
			if (type->bits)
			{
				if (bitsize < type->bits*(arraysize?arraysize:1))
					bitsize = type->bits*(arraysize?arraysize:1);
			}
			else
			{
				if (bitsize < 32*type->size*(arraysize?arraysize:1))
					bitsize = 32*type->size*(arraysize?arraysize:1);
			}
		}
		else
		{
			if (type->bits)
			{
				if (type->bits < type->align && type->type == ev_bitfld)
				{	//only realigns when it cross its parent's alignment boundary.
					if ((bitsize&(type->align-1)) + type->bits > type->align)
						bitsize = (bitsize+type->align-1)&~(type->align-1);	//these must be aligned, so we can take pointers etc.
				}
				else if (type->align)
					bitsize = (bitsize+type->align-1)&~(type->align-1);	//these must be aligned, so we can take pointers etc.
				ofs = bitsize;
				bitsize += type->bits*(arraysize?arraysize:1);
			}
			else
			{
				bitsize = (bitsize+31)&~31;	//provide padding to keep non-bitfield stuff word aligned
				ofs = bitsize;
				bitsize += 32 * type->size*(arraysize?arraysize:1);
			}
		}

		parms[numparms].ofs = ofs>>5;
		parms[numparms].bitofs = ofs - (parms[numparms].ofs<<5);
		parms[numparms].arraysize = arraysize;
		parms[numparms].out = false;
		parms[numparms].optional = false;
		parms[numparms].isvirtual = isvirt;
		parms[numparms].paramname = parmname;
		parms[numparms].type = type;
		parms[numparms].defltvalue = defaultval;
		numparms++;

		/*if (type->type == ev_vector && arraysize == 0)
		{	//add in vec_x/y/z members too...?
			int c;
			for (c = 0; c < 3; c++)
			{
				parms[numparms].ofs = ofs + c;
				parms[numparms].arraysize = arraysize;
				parms[numparms].out = false;
				parms[numparms].optional = true;
				parms[numparms].isvirtual = isvirt;
				parms[numparms].paramname = qccHunkAlloc(strlen(parmname)+3);
				sprintf(parms[numparms].paramname, "%s_%c", parmname, 'x'+c);
				parms[numparms].type = type_float;
				parms[numparms].defltvalue = nullsref;
				numparms++;
			}
		}*/
	}

	if (bitalign)	//compute tail padding.
		bitsize = (bitsize+bitalign-1)&~(bitalign-1);

	newt->size = (bitsize+31)>>5;	//FIXME: change to bytes, for pointers.
	newt->align = bitalign;
	if (bitalign&31)
		newt->bits = bitsize;
	else
		newt->bits = 0;	//all word aligned. nothing special going on sizewise (maybe inside though).

	if (!numparms)
		QCC_PR_ParseError(ERR_NOTANAME, "%s %s has no members", structtype==ev_union?"union":"struct", newt->name);

	newt->num_parms = numparms;
	newt->params = qccHunkAlloc(sizeof(*type->params) * numparms);
	memcpy(newt->params, parms, sizeof(*type->params) * numparms);
	free(parms);

	if (oldtype)
	{
		if (typecmp_strict(newt, oldtype))
			QCC_PR_ParseError(ERR_NOTANAME, "%s %s redeclared differently", structtype==ev_union?"union":"struct", newt->name);
		newt = oldtype;
	}

	return newt;
}

QCC_type_t *QCC_PR_ParseEntClass(void)
{
	QCC_type_t *newt, *type, *fieldtype, *newparm;
	char membername[2048];
	char *classname;
	int forwarddeclaration;
	int numparms = 0;
	struct QCC_typeparam_s *parms = NULL;
	char *parmname;
	int arraysize;
	pbool redeclaration;
	int basicindex;
	QCC_def_t *d;
	QCC_type_t *pc;
	QCC_type_t *basetype;
	pbool found = false;
	int assumevirtual = 0;	//0=erk, 1=yes, -1=no

	parmname = QCC_PR_ParseName();
	classname = qccHunkAlloc(strlen(parmname)+1);
	strcpy(classname, parmname);

	newt = 0;

	if (QCC_PR_CheckToken(":"))
	{
		char *parentname = QCC_PR_ParseName();
		fieldtype = QCC_TypeForName(parentname);
		if (!fieldtype)
			QCC_PR_ParseError(ERR_NOTANAME, "Parent class %s was not yet defined", parentname);

		//FIXME: we should allow inheriting from 'object' too.
		if (fieldtype->type != ev_entity)
			QCC_PR_ParseError(ERR_NOTANAME, "Parent type %s is not a class/entity", parentname);
		forwarddeclaration = false;

		QCC_PR_Expect("{");
	}
	else
	{
		fieldtype = type_entity;
		forwarddeclaration = !QCC_PR_CheckToken("{");
	}

	newt = QCC_TypeForName(classname);
	if (newt && newt->num_parms != 0)
		redeclaration = true;
	else
		redeclaration = false;

	if (!newt)
	{
		newt = QCC_PR_NewType(classname, fieldtype->type, true);
		newt->size=type_entity->size;
	}

	type = NULL;

	if (forwarddeclaration)
		return newt;

	if (pr_scope)
		QCC_PR_ParseError(ERR_REDECLARATION, "Declaration of class %s%s%s within function", col_type,classname,col_none);
	if (redeclaration && fieldtype != newt->parentclass)
		QCC_PR_ParseError(ERR_REDECLARATION, "Parent class changed on redeclaration of %s%s%s", col_type,classname,col_none);
	newt->parentclass = fieldtype;
	newt->filen = s_filen;
	newt->line = pr_source_line;

	if (QCC_PR_CheckToken(","))
		QCC_PR_ParseError(ERR_NOTANAME, "member missing name");
	while (!QCC_PR_CheckToken("}"))
	{
		unsigned int gdf_flags;

		pbool wasvirt = false;
		pbool wasnonvirt = false;
		pbool wasstatic = false;
		pbool isconst = false;
		pbool isvar = false;
		pbool isignored = false;
		pbool isinline = false;
		pbool isget = false;
		pbool isset = false;
//		pbool ispublic = false;
//		pbool isprivate = false;
//		pbool isprotected = false;
		pbool firstkeyword = true;

		while(1)
		{
			if (QCC_PR_CheckKeyword(1, "nonvirtual"))
			{
				if (firstkeyword && QCC_PR_CheckToken(":"))
				{	//fteqcc-specific
					assumevirtual = -1;
					continue;
				}
				wasnonvirt = true;
			}
			else if (QCC_PR_CheckKeyword(1, "static"))
				wasstatic = true;
			else if (QCC_PR_CheckKeyword(1, "const"))
				isconst = wasstatic = true;
			else if (QCC_PR_CheckKeyword(1, "var"))
				isvar = true;
			else if (QCC_PR_CheckKeyword(1, "virtual"))
			{
				if (firstkeyword && QCC_PR_CheckToken(":"))
				{	//fteqcc-specific
					assumevirtual = 1;
					continue;
				}
				wasvirt = true;
			}
			else if (QCC_PR_CheckKeyword(1, "ignore"))
				isignored = true;
			else if (QCC_PR_CheckKeyword(1, "strip"))
				isignored = true;
			else if (QCC_PR_CheckKeyword(1, "inline"))
				isinline = true;
			else if (QCC_PR_CheckKeyword(1, "get"))
				isget = true;
			else if (QCC_PR_CheckKeyword(1, "set"))
				isset = true;
			else if (firstkeyword && QCC_PR_CheckKeyword(1, "public"))
			{
				/*ispublic = true;*/
				QCC_PR_Expect(":");
				continue;
			}
			else if (firstkeyword && QCC_PR_CheckKeyword(1, "private"))
			{
				QCC_PR_Expect(":");
				/*isprivate = true; */
				continue;
			}
			else if (firstkeyword && QCC_PR_CheckKeyword(1, "protected"))
			{
				QCC_PR_Expect(":");
				/*isprotected = true;*/ QCC_PR_Expect(":");
				continue;
			}
			else
				break;
			firstkeyword = false;
		}
		if (isget || isset)
		{
			if (wasvirt)
				QCC_PR_ParseWarning(ERR_INTERNAL, "virtual accessors are not supported at this time");
			if (wasstatic)
				QCC_PR_ParseError(ERR_INTERNAL, "static accessors are not supported");
			QCC_PR_ParseAccessorMember(newt, isinline, isset);
			QCC_PR_CheckToken(";");
			continue;
		}

		basetype = QCC_PR_ParseType(false, false, false);

		if (!basetype)
			QCC_PR_ParseError(ERR_INTERNAL, "In class %s, expected type, found %s", classname, pr_token);

		if (basetype->type == ev_struct || basetype->type == ev_union)	//we wouldn't be able to handle it.
			QCC_PR_ParseError(ERR_INTERNAL, "Struct or union in class %s", classname);


		for (;basetype;)
		{
			pbool havebody = false;
			pbool isnull = false;
			pbool isstatic = wasstatic;
			pbool isvirt = wasvirt;
			pbool isnonvirt = wasnonvirt;

			if (flag_qcfuncs && basetype->type == ev_function && basetype->aux_type == newt && QCC_PR_PeekToken(";"))
			{	//C++ style constructor (ie: missing return type followed by class name)
				//swap the class out for the appropriate function type...
				newparm = QCC_PR_GenFunctionType(type_void, basetype->params, basetype->num_parms);
				parmname = classname;
				arraysize = 0;
			}
			else if (!flag_qcfuncs && basetype == newt && QCC_PR_CheckToken("("))
			{
				newparm = QCC_PR_ParseFunctionType(false, type_void);
				if (!newparm)
					QCC_PR_ParseError(ERR_BADNOTTYPE, "expected function arg list");
				parmname = classname;
				arraysize = 0;
			}
			else
			{
				parmname = QCC_PR_ParseName();
				if (QCC_PR_CheckToken("["))
				{
					arraysize = QCC_PR_IntConstExpr();
					QCC_PR_Expect("]");
				}
				else
					arraysize = 0;

				if (QCC_PR_CheckToken("("))
				{
					//int fnc(), fld; is valid.
					newparm = QCC_PR_ParseFunctionType(false, basetype);
					if (!newparm)
						QCC_PR_ParseError(ERR_BADNOTTYPE, "expected function arg list");
				}
				else
					newparm = basetype;
			}

			if (newparm->type == ev_function)
			{
				if (!strcmp(classname, parmname))
				{
					if (isstatic)
						QCC_PR_ParseWarning(WARN_MISSINGMEMBERQUALIFIER, "Constructor %s::%s should not be static.", classname, pr_token);
					if (!isvirt)
						isnonvirt = true;//silently promote constructors to static
					if (newparm->aux_type->type != ev_void)
						QCC_PR_ParseWarning(WARN_MISSINGMEMBERQUALIFIER, "Constructor %s::%s does not return void.", classname, pr_token);
					if (newparm->num_parms != 0 || newparm->vargs)
						QCC_PR_ParseWarning(WARN_MISSINGMEMBERQUALIFIER, "Constructor arguments are not supported at this time %s::%s. Use named fields instead.", classname, pr_token);
				}
				else if (!isvirt && !isnonvirt && !isstatic)
				{
					if (assumevirtual == 1)
						isvirt = true;
					else if (assumevirtual == -1)
						isnonvirt = true;
					else
					{
						QCC_PR_ParseWarning(WARN_MISSINGMEMBERQUALIFIER, "%s::%s was not qualified. Assuming non-virtual.", classname, parmname);
						isnonvirt = true;
					}
				}
				else if (isvirt+isnonvirt+isstatic != 1)
					QCC_PR_ParseError(ERR_INTERNAL, "Multiple conflicting qualifiers on %s::%s.", classname, pr_token);

				if (isstatic)
				{	//static and non-virtual functions differ only in that static should not have a 'this' available. however there's no hiding 'self' for calling in to other functions.
					isstatic = false;
					isnonvirt = true;
//					QCC_PR_ParseError(ERR_INTERNAL, "%s::%s static member functions are not supported at this time.", classname, parmname);
				}
			}
			else
			{
				if (isvirt||isnonvirt)
					QCC_Error(ERR_INTERNAL, "virtual keyword on member that is not a function");
				if (isinline)
					QCC_Error(ERR_INTERNAL, "inline keyword on member that is not a function");
			}

			if (QCC_PR_CheckToken("="))
				havebody = true;
			else if (newparm->type == ev_function && QCC_PR_PeekToken("{"))
				havebody = true;
			if (isinline && (!havebody || isvirt))
				QCC_Error(ERR_INTERNAL, "inline keyword on function prototype or virtual function");

			gdf_flags = 0;
			if ((newparm->type == ev_function && !arraysize && !isvar) || isconst)
				gdf_flags = GDF_CONST;

			if (havebody)
			{
				QCC_def_t *def;
				if (pr_scope)
					QCC_Error(ERR_INTERNAL, "Nested function declaration");

				isnull = (QCC_PR_CheckImmediate("0") || QCC_PR_CheckImmediate("0i"));
				QC_snprintfz(membername, sizeof(membername), "%s::%s", classname, parmname);
				if (isnull)
				{
					if (isignored)
						def = NULL;
					else
					{
						def = QCC_PR_GetDef(newparm, membername, NULL, true, 0, gdf_flags);
						def->symboldata[def->ofs].function = 0;
						def->initialized = 1;
					}
				}
				else
				{
					if (isignored)
						def = NULL;
					else
						def = QCC_PR_GetDef(newparm, membername, NULL, true, 0, gdf_flags);

					if (newparm->type != ev_function && !isstatic)
						QCC_Error(ERR_INTERNAL, "Can only initialise member functions");
					else
					{
						if (autoprototype || isignored)
						{
							if (QCC_PR_CheckToken("["))
							{
								while (!QCC_PR_CheckToken("]"))
								{
									if (pr_token_type == tt_eof)
										break;
									QCC_PR_Lex();
								}
							}
							QCC_PR_Expect("{");

							{
								int blev = 1;
								//balance out the { and }
								while(blev)
								{
									if (pr_token_type == tt_eof)
										break;
									if (QCC_PR_CheckToken("{"))
										blev++;
									else if (QCC_PR_CheckToken("}"))
										blev--;
									else
										QCC_PR_Lex();	//ignore it.
								}
							}
						}
						else
						{
							pr_classtype = newt;
							QCC_PR_ParseInitializerDef(def, 0);
							pr_classtype = NULL;
							/*
							f = QCC_PR_ParseImmediateStatements (def, newparm);
							pr_classtype = NULL;
							pr_scope = NULL;
							def->symboldata[def->ofs].function = f - functions;
							f->def = def;
							def->initialized = 1;*/
						}
					}
				}
				if (def)
					QCC_FreeDef(def);

				if (!isvirt && !isignored)
				{
					QCC_def_t *fdef;
					QCC_type_t *pc;
					unsigned int i;

					for (pc = newt->parentclass; pc; pc = pc->parentclass)
					{
						for (i = 0; i < pc->num_parms; i++)
						{
							if (!strcmp(pc->params[i].paramname, parmname))
							{
								QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "%s::%s is virtual inside parent class '%s'. Did you forget the 'virtual' keyword?", newt->name, parmname, pc->name);
								break;
							}
						}
						if (i < pc->num_parms)
							break;
					}
					if (!pc)
					{
						fdef = QCC_PR_GetDef(NULL, parmname, NULL, false, 0, GDF_CONST);
						if (fdef && fdef->type->type == ev_field)
						{
							QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "%s::%s is virtual inside parent class 'entity'. Did you forget the 'virtual' keyword?", newt->name, parmname);
							QCC_PR_ParsePrintDef(0, fdef);
						}
						else if (fdef)
						{
							QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "%s::%s shadows a global", newt->name, parmname);
							QCC_PR_ParsePrintDef(0, fdef);
						}
						if (fdef)
							QCC_FreeDef(fdef);
					}
				}
			}

			if (!QCC_PR_CheckToken(","))
			{
				QCC_PR_Expect(";");
				basetype = NULL;
			}

			if (isignored)	//member doesn't really exist
				continue;

			//static members are technically just funny-named globals, and do not generate fields.
			if (isnonvirt || isstatic || (newparm->type == ev_function && !arraysize))
			{
				QC_snprintfz(membername, sizeof(membername), "%s::%s", classname, parmname);
				QCC_FreeDef(QCC_PR_GetDef(newparm, membername, NULL, true, 0, gdf_flags));

				if (isnonvirt || isstatic)
					continue;
			}


			fieldtype = QCC_PR_NewType(parmname, ev_field, false);
			fieldtype->aux_type = newparm;
			fieldtype->size = newparm->size;

			parms = realloc(parms, sizeof(*parms) * (numparms+1));
			parms[numparms].ofs = 0;
			parms[numparms].bitofs = 0;
			parms[numparms].out = false;
			parms[numparms].optional = false;
			parms[numparms].isvirtual = isvirt;
			parms[numparms].paramname = parmname;
			parms[numparms].arraysize = arraysize;
			parms[numparms].type = newparm;
			parms[numparms].defltvalue.cast = NULL;

			basicindex = 0;
			found = false;
			for(pc = newt; pc && !found; pc = pc->parentclass)
			{
				struct QCC_typeparam_s *pp;
				int numpc;
				int i;
				if (pc == newt)
				{
					pp = parms;
					numpc = numparms;
				}
				else
				{
					pp = pc->params;
					numpc = pc->num_parms;
				}
				for (i = 0; i < numpc; i++)
				{
					if (pp[i].type->type == newparm->type)
					{
						if (!strcmp(pp[i].paramname, parmname))
						{
							if (typecmp(pp[i].type, newparm))
							{
								char bufc[256];
								char bufp[256];
								TypeName(pp[i].type, bufp, sizeof(bufp));
								TypeName(newparm, bufc, sizeof(bufc));
								QCC_PR_ParseError(0, "%s defined as %s in %s, but %s in %s\n", parmname, bufc, newt->name, bufp, pc->name);
							}
							basicindex = pp[i].ofs;
							found = true;
							break;
						}
						if ((unsigned int)basicindex < pp[i].ofs+pp[i].type->size*(pp[i].arraysize?pp[i].arraysize:1))	//if we found one with the index
							basicindex = pp[i].ofs+pp[i].type->size*(pp[i].arraysize?pp[i].arraysize:1);	//make sure we don't union it.
					}
				}
			}


			/* iterate over every parent-class and see if our method already exists in conflicting nonvirtual type form */
			if (isvirt)
			{
				for(pc = newt; pc && !found; pc = pc->parentclass)
				{
					struct QCC_typeparam_s *pp;
					int numpc;
					int i;
					found = false;

					if (pc == newt)
						continue;

					pp = parms;
					numpc = numparms;

					/* iterate over all of the virtual methods */
					for (i = 0; i < numpc; i++)
					{
						if (pp[i].type->type == newparm->type)
						{
							/* if we found it, abandon this loop - we still have to check the other classes however */
							if (!strcmp(pp[i].paramname, parmname))
							{
								found = true;
								break;
							}
						}
					}

					/* we didn't find it as a field... so check if it exists as a nonvirtual method */
					if (found == false)
					{
						QC_snprintfz(membername, sizeof(membername), "%s::%s", pc->name, parmname);

						/* if we found it, game over */
						if (QCC_PR_GetDef(NULL, membername, NULL, false, 0, 0))
							QCC_PR_ParseError(0, "%s defined as virtual in %s, but nonvirtual in %s\n", parmname, newt->name, pc->name);
					}
				}
			}

			parms[numparms].ofs = basicindex;	//ulp, its new
			numparms++;

			if (found)
				continue;

			if (!*basictypes[newparm->type])
				QCC_PR_ParseError(0, "members of type %s are not supported (%s::%s)\n", basictypenames[newparm->type], classname, parmname);

			//make sure the union is okay
			d = QCC_PR_GetDef(NULL, parmname, NULL, 0, 0, GDF_CONST);
			if (d)
				basicindex = 0;
			else
			{	//don't go all weird with unioning generic fields
				QC_snprintfz(membername, sizeof(membername), "::*%s", basictypenames[newparm->type]);
				d = QCC_PR_GetDef(NULL, membername, NULL, 0, 0, GDF_CONST);
				if (!d)
				{
					d = QCC_PR_GetDef(QCC_PR_FieldType(*basictypes[newparm->type]), membername, NULL, 2, 0, GDF_CONST|GDF_POSTINIT|GDF_USED);
//					for (i = 0; (unsigned int)i < newparm->size*(arraysize?arraysize:1); i++)
//						d->symboldata[i]._int = pr.size_fields+i;
//					pr.size_fields += i;

					d->used = true;
					d->referenced = true;	//always referenced, so you can inherit safely.
				}
				if (d->arraysize < basicindex+(arraysize?arraysize:1))
				{
					if (d->symboldata)
						QCC_PR_ParseError(ERR_INTERNAL, "array members are kinda limited, sorry. try rearranging them or adding padding for alignment\n");	//FIXME: add relocs to cope with this all of a type can then be contiguous and thus allow arrays.
					else
					{
						int newsize = basicindex+(arraysize?arraysize:1);
						if (d->type->type == ev_union || d->type->type == ev_struct)
							d->arraysize = newsize;
						else while(d->arraysize < newsize)
						{
							QC_snprintfz(membername, sizeof(membername), "::%s[%i]", basictypenames[newparm->type], d->arraysize/d->type->size);
							QCC_PR_DummyDef(d->type, membername, d->scope, 0, d, d->arraysize, true, GDF_CONST);
							d->arraysize+=d->type->size;
						}
					}
				}
			}
			QCC_FreeDef(d);

			//and make sure we can do member::__fname
			//actually, that seems pointless.
			QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, classname, parmname);
//				externs->Printf("define %s -> %s\n", membername, d->name);
			d = QCC_PR_DummyDef(fieldtype, membername, pr_scope, arraysize, d, basicindex, true, (isnull?0:GDF_CONST)|(opt_classfields?GDF_STRIP:0));
			d->referenced = true;	//always referenced, so you can inherit safely.
		}
	}

	if (redeclaration)
	{
		int i;
		redeclaration = newt->num_parms != numparms;

		for (i = 0; i < numparms && (unsigned int)i < newt->num_parms; i++)
		{
			if (newt->params[i].arraysize != parms[i].arraysize || typecmp(newt->params[i].type, parms[i].type) || strcmp(newt->params[i].paramname, parms[i].paramname))
			{
				QCC_PR_ParseError(ERR_REDECLARATION, "Incompatible redeclaration of class %s. %s differs.", classname, parms[i].paramname);
				break;
			}
		}
		if (newt->num_parms != numparms)
			QCC_PR_ParseError(ERR_REDECLARATION, "Incompatible redeclaration of class %s.", classname);
	}
	else
	{
		newt->num_parms = numparms;
		newt->params = qccHunkAlloc(sizeof(*type->params) * numparms);
		memcpy(newt->params, parms, sizeof(*type->params) * numparms);
	}
	free(parms);

	{
		QCC_def_t *d;
		//if there's a constructor, make sure the spawnfunc_ function is defined so that its available to maps.
		QC_snprintfz(membername, sizeof(membername), "spawnfunc_%s", classname);
		d = QCC_PR_GetDef(type_function, membername, NULL, true, 0, GDF_CONST);
		d->funccalled = true;
		d->referenced = true;
		QCC_FreeDef(d);
	}

	return newt;
}

pbool type_inlinefunction;
/*newtype=true: creates a new type always
  silentfail=true: function is permitted to return NULL if it was not given a type, otherwise never returns NULL
*/
QCC_type_t *QCC_PR_ParseType (int newtype, pbool silentfail, pbool ignoreptr)
{
	QCC_type_t	*newt;
	QCC_type_t	*type;
	char	*name;

	type_inlinefunction = false;	//doesn't really matter so long as its not from an inline function type

//	int ofs;

	if (QCC_PR_CheckKeyword(keyword_const, "const"))
	{
		QCC_PR_ParseWarning (WARN_IGNOREDKEYWORD, "ignoring unsupported const keyword");
		silentfail = false;	//FIXME
	}
	if (QCC_PR_CheckKeyword(keyword_volatile, "volatile"))	//we don't really support this - everything is volatile.
		silentfail = false;

	if (QCC_PR_PeekToken ("...") )	//this is getting stupid
	{
		QCC_PR_LexWhitespace (false);
		if (*pr_file_p == '(')
		{	//work around gmqcc's "(...(" being misinterpreted as a cast syntax error, instead abort here so it can be treated as an intrinsic (with args) instead.
			if (silentfail)
				return NULL;
			QCC_PR_ParseError (ERR_NOTATYPE, "\"%s\" is not a type", pr_token);
		}
		QCC_PR_Lex ();

		type = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
		type->aux_type = QCC_PR_ParseType (false, false, ignoreptr);
		type->size = type->aux_type->size;

		newt = QCC_PR_FindType (type);
		type = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
		type->aux_type = newt;
		type->size = type->aux_type->size;

		newt = QCC_PR_FindType (type);
		type = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
		type->aux_type = newt;
		type->size = type->aux_type->size;

		if (newtype)
			return type;
		return QCC_PR_FindType (type);
	}
	if (QCC_PR_CheckToken (".."))	//so we don't end up with the user specifying '. .vector blah' (hexen2 added the .. token for array ranges)
	{
		newt = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
		newt->aux_type = QCC_PR_ParseType (false, false, ignoreptr);

		newt->size = newt->aux_type->size;

		newt = QCC_PR_FindType (newt);

		type = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
		type->aux_type = newt;

		type->size = type->aux_type->size;

		if (newtype)
			return type;
		return QCC_PR_FindType (type);
	}
	if (QCC_PR_CheckToken ("."))
	{
		//.float *foo; is annoying.
		//technically it is a pointer to a .float
		//most people will want a .(float*) foo;
		//so .*float will give you that.
		//however, we can't cope parsing that with regular types, so we support that ONLY when . was already specified.
		//this is pretty much an evil syntax hack.
		pbool ptr = QCC_PR_CheckToken ("*");
		type = QCC_PR_ParseType(false, false, ignoreptr);
		if (!type)
			QCC_PR_ParseError(0, "Expected type\n");
		if (ptr)
			type = QCC_PointerTypeTo(type);

		name = qccHunkAlloc(strlen(type->name)+2);
		*name = '.';
		strcpy(name+1, type->name);

		newt = QCC_PR_NewType(name, ev_field, false);
		newt->aux_type = type;
		newt->size = newt->aux_type->size;

		if (newtype)
			return newt;
		return QCC_PR_FindType (newt);
	}

	name = pr_token;
	if (pr_token_type != tt_name)
	{
		if (silentfail)
			return NULL;
		QCC_PR_ParseError (ERR_NOTATYPE, "\"%s\" is not a type", name);
	}
//	name = QCC_PR_CheckCompConstString(name);

	//accessors
	if (QCC_PR_CheckKeyword (keyword_accessor, "accessor"))
	{
		char parentname[256];
		char *accessorname;
		char *funcname;

		newt = NULL;

		funcname = QCC_PR_ParseName();
		accessorname = qccHunkAlloc(strlen(funcname)+1);
		strcpy(accessorname, funcname);

		/* Look to see if this type is already defined */
		newt = QCC_TypeForName(accessorname);
		if (newt && newt->type != ev_accessor)
			QCC_PR_ParseError(ERR_NOTANAME, "Type %s cannot be redefined as an accessor", accessorname);

		if (QCC_PR_CheckToken(":"))
		{
			type = QCC_PR_ParseType(false, false, false);
			if (type)
				TypeName(type, parentname, sizeof(parentname));
			else
				strcpy(parentname, "??");

			if (!type || type->type == ev_struct || type->type == ev_union)
				QCC_PR_ParseError(ERR_NOTANAME, "Accessor %s cannot be based upon %s", accessorname, parentname);
		}
		else
			type = NULL;

		if (!newt)
		{
			newt = QCC_PR_NewType(accessorname, ev_accessor, true);
			newt->size=type->size;
		}
		if (!newt->parentclass)
		{
			newt->parentclass = type;
			if (!newt->parentclass || newt->parentclass->type == ev_struct || newt->parentclass->type == ev_union || newt->size != newt->parentclass->size)
				QCC_PR_ParseError(ERR_NOTANAME, "Accessor %s cannot be based upon %s", accessorname, parentname);
		}
		else if (type != newt->parentclass)
		{
			char bufe[256];
			char bufn[256];
			QCC_PR_ParseError(ERR_NOTANAME, "Accessor %s basic type mismatch (%s, expected %s)", accessorname,
				TypeName(type, bufn, sizeof(bufn)), TypeName(newt->parentclass, bufe, sizeof(bufe)));
		}

		if (QCC_PR_CheckToken("{"))
		{
			pbool setnotget;
			pbool isinline;

			newt->filen = s_filen;
			newt->line = pr_source_line;

			do
			{
				isinline = QCC_PR_CheckName("inline");

				if (QCC_PR_CheckName("set"))
					setnotget = true;
				else if (QCC_PR_CheckName("get"))
					setnotget = false;
				else
					break;

				QCC_PR_ParseAccessorMember(newt, isinline, setnotget);
			} while (QCC_PR_CheckToken(",") || QCC_PR_CheckToken(";"));
			QCC_PR_Expect("}");
		}

		if (newtype)
			newt = QCC_PR_DuplicateType(newt, false);
		return newt;
	}

	if (flag_qcfuncs && QCC_PR_CheckKeyword (keyword_class, "class"))
		type = QCC_PR_ParseEntClass();
	else if (QCC_PR_CheckKeyword(keyword_enum, "enum"))
		type = QCC_PR_ParseEnum(false);
	else if (QCC_PR_CheckKeyword(keyword_enumflags, "enumflags"))
		type = QCC_PR_ParseEnum(true);
	else if (QCC_PR_CheckKeyword (keyword_union, "union"))
		type = QCC_PR_ParseStruct(ev_union);
	else if (QCC_PR_CheckKeyword (keyword_struct, "struct"))
		type = QCC_PR_ParseStruct(ev_struct);	//defaults to public
//	else if (QCC_PR_CheckKeyword (keyword_class, "class"))
//		structtype = ev_struct;	//defaults to private. no other difference.
	else
	{
		type = QCC_TypeForName(name);
		if (type)
			QCC_PR_Lex ();
		else
		{
			if (!*name)
			{
				QCC_PR_ParseError(ERR_NOTANAME, "type missing name");
				return NULL;
			}

			//some reacc types...
			if (flag_acc && !stricmp("Void", name))
				type = type_void;
			else if (flag_acc && !stricmp("Real", name))
				type = type_float;
			else if (flag_acc && !stricmp("Vector", name))
				type = type_vector;
			else if (flag_acc && !stricmp("Object", name))
				type = type_entity;
			else if (flag_acc && !stricmp("String", name))
				type = type_string;
			else if (flag_acc && !stricmp("PFunc", name))
				type = type_function;
			else
			{
				//try and handle C's types, which have weird and obtuse combinations (like long long, long int, short int).
				pbool isokay = false;
				pbool issigned = false;
				pbool isunsigned = false;
				pbool islong = false;
				pbool isfloat = false;
				int bits = 0;

	#define longlongbits 64
	#define longbits (flag_ILP32?32:64)

				if (QCC_PR_CheckKeyword(keyword_register, "register"))	//allow a leading register keyword. technically EVERYTHING is a register in qc, so we can just ignore this.
					isokay = true;
				while(true)
				{
					if (!isunsigned && !issigned && QCC_PR_CheckKeyword(keyword_signed, "signed"))
						issigned = isokay = true;
					else if (!issigned && !isunsigned && QCC_PR_CheckKeyword(keyword_unsigned, "unsigned"))
						isunsigned = isokay = true;
					else if (!bits && QCC_PR_CheckKeyword(keyword_long, "long"))
					{
						if (islong)
							bits = longlongbits;
						islong = isokay = true;
					}
					else if ((!bits || bits==16) && (QCC_PR_CheckKeyword(keyword_int, "int") || QCC_PR_CheckKeyword(keyword_integer, "integer")))
					{	//long int, short int, etc are allowed
						if (!bits)
							bits = 32;
						isokay = true;
					}
					else if (!bits && QCC_PR_CheckKeyword(keyword_short, "short"))
						bits = 16, isokay = true;
					else if (!bits && QCC_PR_CheckKeyword(keyword_char, "char"))
						bits = 8, isokay = true;
					else if (!bits && !issigned && !isunsigned && QCC_PR_CheckKeyword(true, "_Bool"))	//c99
					{
						if (keyword_int)
							type = type_bint;
						else
							type = type_bfloat;
						goto wasctype;
					}

					else if (!bits && !islong && QCC_PR_CheckKeyword(keyword_float, "float"))
						bits = 32, isfloat = isokay = true;
					else if ((!bits||islong) && QCC_PR_CheckKeyword(keyword_double, "double"))
						bits = islong?128:64, islong=false, isfloat = isokay = true;
					else
						break;
				}
				if (isokay)
				{
					if (!bits)
						bits = islong?longbits:32;	//<signed|unsigned|long> [int]
					if (isfloat)
					{
						if (isunsigned)
							QCC_PR_ParseWarning (WARN_IGNOREDKEYWORD, "ignoring unsupported unsigned keyword, type will be signed");
						if (bits > 64)
							type = type_double, QCC_PR_ParseWarning (WARN_IGNOREDKEYWORD, "long doubles are not supported, using double");	//permitted
						else if (bits == 64)
							type = type_double;
						else
							type = type_float;
					}
					else
					{
						if (bits > 64)
							type = (isunsigned?type_uint64:type_int64), QCC_PR_ParseWarning (WARN_IGNOREDKEYWORD, "long longs are not supported, using long");	//permitted
						else if (bits > 32)
							type = (isunsigned?type_uint64:type_int64);
						else if (bits <= 8)
							type = (isunsigned?type_uint8:type_sint8);
						else if (bits <= 16)
							type = (isunsigned?type_uint16:type_sint16);
						else
							type = (isunsigned?type_uint:type_integer);
					}
					goto wasctype;
				}


				if (silentfail)
					return NULL;

				QCC_PR_ParseError (ERR_NOTATYPE, "\"%s\" is not a type", name);
				type = type_float;	// shut up compiler warning
			}
		}
	}
wasctype:
	if (!type)
		return NULL;

	if (!ignoreptr)
	{
		while (QCC_PR_CheckToken("*"))
		{
			if (QCC_PR_CheckKeyword(keyword_const, "const"))
				QCC_PR_ParseWarning (WARN_IGNOREDKEYWORD, "ignoring unsupported const keyword");
			type = QCC_PointerTypeTo(type);
		}
	}

	if (flag_qcfuncs && QCC_PR_CheckToken ("("))	//this is followed by parameters. Must be a function.
	{
		type_inlinefunction = true;
		type = QCC_PR_ParseFunctionType(newtype, type);
		if (!type)
			QCC_PR_ParseError(ERR_BADNOTTYPE, "expected function arg list");
	}
	else
	{
		if (newtype)
		{
			type = QCC_PR_DuplicateType(type, false);
		}
	}
	return type;
}

#endif



