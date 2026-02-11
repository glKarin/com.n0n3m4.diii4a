//This is basically a sample program.
//It deomnstrates the code required to get qclib up and running.
//This code does not demonstrate entities, however.
//It does demonstrate the built in qc compiler, and does demonstrate a globals-only progs interface.
//It also demonstrates basic builtin(s).



#include "progtype.h"
#include "progslib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

enum{false,true};


//builtins and builtin management.
void PF_puts (pubprogfuncs_t *prinst, struct globalvars_s *gvars)
{
	char *s;
	s = prinst->VarString(prinst, 0);

	printf("%s", s);
}
void PF_strcat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT(OFS_RETURN) = prinst->TempString(prinst, prinst->VarString(prinst, 0));
}
void PF_ftos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char temp[64];
	sprintf(temp, "%g", G_FLOAT(OFS_PARM0));
	G_INT(OFS_RETURN) = prinst->TempString(prinst, temp);
}
void PF_vtos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char temp[64];
	sprintf(temp, "'%g %g %g'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
	G_INT(OFS_RETURN) = prinst->TempString(prinst, temp);
}
void PF_etos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char temp[64];
	sprintf(temp, "%i", G_INT(OFS_PARM0));
	G_INT(OFS_RETURN) = prinst->TempString(prinst, temp);
}
void PF_itos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char temp[64];
	sprintf(temp, "%x", G_INT(OFS_PARM0));
	G_INT(OFS_RETURN) = prinst->TempString(prinst, temp);
}
void PF_ltos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char temp[64];
	sprintf(temp, "%"PRIx64, G_INT64(OFS_PARM0));
	G_INT(OFS_RETURN) = prinst->TempString(prinst, temp);
}
void PF_dtos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char temp[64];
	sprintf(temp, "%g", G_DOUBLE(OFS_PARM0));
	G_INT(OFS_RETURN) = prinst->TempString(prinst, temp);
}
void PF_stof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = strtod(PR_GetStringOfs(prinst, OFS_PARM0), NULL);
}
void PF_stov (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	sscanf(PR_GetStringOfs(prinst, OFS_PARM0), " ' %f %f %f ' ",
		&G_FLOAT(OFS_RETURN+0),
		&G_FLOAT(OFS_RETURN+1),
		&G_FLOAT(OFS_RETURN+2));
}
void PF_strcmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (prinst->callargc >= 3)
		G_FLOAT(OFS_RETURN) = strncmp(PR_GetStringOfs(prinst, OFS_PARM0), PR_GetStringOfs(prinst, OFS_PARM1), G_FLOAT(OFS_PARM2));
	else
		G_FLOAT(OFS_RETURN) = strcmp(PR_GetStringOfs(prinst, OFS_PARM0), PR_GetStringOfs(prinst, OFS_PARM1));
}
void PF_vlen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *v = G_VECTOR(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}
void PF_normalize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *v = G_VECTOR(OFS_PARM0);
	double l = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
	if (l)
		l = 1./l;
	else
		l = 0;
	G_VECTOR(OFS_RETURN)[0] = v[0] * l;
	G_VECTOR(OFS_RETURN)[1] = v[1] * l;
	G_VECTOR(OFS_RETURN)[2] = v[2] * l;
}
void PF_floor (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}
void PF_pow (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = pow(G_FLOAT(OFS_PARM0), G_FLOAT(OFS_PARM1));
}
void PF_sqrt (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = sqrt(G_FLOAT(OFS_PARM0));
}

void PF_putv (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	printf("%f %f %f\n", G_FLOAT(OFS_PARM0+0), G_FLOAT(OFS_PARM0+1), G_FLOAT(OFS_PARM0+2));
}

void PF_putf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	printf("%f\n", G_FLOAT(OFS_PARM0));
}

#ifdef _WIN32
	#define Q_snprintfz _snprintf
	#define Q_vsnprintf _vsnprintf
#else
	#define Q_snprintfz snprintf
	#define Q_vsnprintf vsnprintf
#endif
char	*va(char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
	va_start (argptr, format);
	Q_vsnprintf (string, sizeof(string), format,argptr);
	va_end (argptr);
	return string;	
}

void QCBUILTIN PF_sprintf_internal (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals, const char *s, int firstarg, char *outbuf, int outbuflen)
{
	const char *s0;
	char *o = outbuf, *end = outbuf + outbuflen, *err;
	int width, precision, thisarg, flags;
	char formatbuf[16];
	char *f;
	int argpos = firstarg;
	int isfloat;
	static int dummyivec[3] = {0, 0, 0};
	static float dummyvec[3] = {0, 0, 0};

#define PRINTF_ALTERNATE 1
#define PRINTF_ZEROPAD 2
#define PRINTF_LEFT 4
#define PRINTF_SPACEPOSITIVE 8
#define PRINTF_SIGNPOSITIVE 16

	formatbuf[0] = '%';

#define GETARG_FLOAT(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_FLOAT(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_VECTOR(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_VECTOR(OFS_PARM0 + 3 * (a))) : dummyvec)
#define GETARG_INT(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_INT(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_INTVECTOR(a) (((a)>=firstarg && (a)<prinst->callargc) ? ((int*) G_VECTOR(OFS_PARM0 + 3 * (a))) : dummyivec)
#define GETARG_STRING(a) (((a)>=firstarg && (a)<prinst->callargc) ? (PR_GetStringOfs(prinst, OFS_PARM0 + 3 * (a))) : "")

	for(;;)
	{
		s0 = s;
		switch(*s)
		{
			case 0:
				goto finished;
			case '%':
				++s;

				if(*s == '%')
					goto verbatim;

				// complete directive format:
				// %3$*1$.*2$ld
				
				width = -1;
				precision = -1;
				thisarg = -1;
				flags = 0;
				isfloat = -1;

				// is number following?
				if(*s >= '0' && *s <= '9')
				{
					width = strtol(s, &err, 10);
					if(!err)
					{
						printf("PF_sprintf: bad format string: %s\n", s0);
						goto finished;
					}
					if(*err == '$')
					{
						thisarg = width + (firstarg-1);
						width = -1;
						s = err + 1;
					}
					else
					{
						if(*s == '0')
						{
							flags |= PRINTF_ZEROPAD;
							if(width == 0)
								width = -1; // it was just a flag
						}
						s = err;
					}
				}

				if(width < 0)
				{
					for(;;)
					{
						switch(*s)
						{
							case '#': flags |= PRINTF_ALTERNATE; break;
							case '0': flags |= PRINTF_ZEROPAD; break;
							case '-': flags |= PRINTF_LEFT; break;
							case ' ': flags |= PRINTF_SPACEPOSITIVE; break;
							case '+': flags |= PRINTF_SIGNPOSITIVE; break;
							default:
								goto noflags;
						}
						++s;
					}
noflags:
					if(*s == '*')
					{
						++s;
						if(*s >= '0' && *s <= '9')
						{
							width = strtol(s, &err, 10);
							if(!err || *err != '$')
							{
								printf("PF_sprintf: invalid format string: %s\n", s0);
								goto finished;
							}
							s = err + 1;
						}
						else
							width = argpos++;
						width = GETARG_FLOAT(width);
						if(width < 0)
						{
							flags |= PRINTF_LEFT;
							width = -width;
						}
					}
					else if(*s >= '0' && *s <= '9')
					{
						width = strtol(s, &err, 10);
						if(!err)
						{
							printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
						}
						s = err;
						if(width < 0)
						{
							flags |= PRINTF_LEFT;
							width = -width;
						}
					}
					// otherwise width stays -1
				}

				if(*s == '.')
				{
					++s;
					if(*s == '*')
					{
						++s;
						if(*s >= '0' && *s <= '9')
						{
							precision = strtol(s, &err, 10);
							if(!err || *err != '$')
							{
								printf("PF_sprintf: invalid format string: %s\n", s0);
								goto finished;
							}
							s = err + 1;
						}
						else
							precision = argpos++;
						precision = GETARG_FLOAT(precision);
					}
					else if(*s >= '0' && *s <= '9')
					{
						precision = strtol(s, &err, 10);
						if(!err)
						{
							printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
						}
						s = err;
					}
					else
					{
						printf("PF_sprintf: invalid format string: %s\n", s0);
						goto finished;
					}
				}

				for(;;)
				{
					switch(*s)
					{
						case 'h': isfloat = 1; break;
						case 'l': isfloat = 0; break;
						case 'L': isfloat = 0; break;
						case 'j': break;
						case 'z': break;
						case 't': break;
						default:
							goto nolength;
					}
					++s;
				}
nolength:

				// now s points to the final directive char and is no longer changed
				if (*s == 'p' || *s == 'P')
				{
					//%p is slightly different from %x.
					//always 8-bytes wide with 0 padding, always ints.
					flags |= PRINTF_ZEROPAD;
					if (width < 0) width = 8;
					if (isfloat < 0) isfloat = 0;
				}
				else if (*s == 'i')
				{
					//%i defaults to ints, not floats.
					if(isfloat < 0) isfloat = 0;
				}

				//assume floats, not ints.
				if(isfloat < 0)
					isfloat = 1;

				if(thisarg < 0)
					thisarg = argpos++;

				if(o < end - 1)
				{
					f = &formatbuf[1];
					if(*s != 's' && *s != 'c')
						if(flags & PRINTF_ALTERNATE) *f++ = '#';
					if(flags & PRINTF_ZEROPAD) *f++ = '0';
					if(flags & PRINTF_LEFT) *f++ = '-';
					if(flags & PRINTF_SPACEPOSITIVE) *f++ = ' ';
					if(flags & PRINTF_SIGNPOSITIVE) *f++ = '+';
					*f++ = '*';
					if(precision >= 0)
					{
						*f++ = '.';
						*f++ = '*';
					}
					if (*s == 'p')
						*f++ = 'x';
					else if (*s == 'P')
						*f++ = 'X';
					else
						*f++ = *s;
					*f++ = 0;

					if(width < 0) // not set
						width = 0;

					switch(*s)
					{
						case 'd': case 'i':
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (int) GETARG_FLOAT(thisarg) : (int) GETARG_INT(thisarg)));
							else
								Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (int) GETARG_FLOAT(thisarg) : (int) GETARG_INT(thisarg)));
							o += strlen(o);
							break;
						case 'o': case 'u': case 'x': case 'X': case 'p': case 'P':
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg)));
							else
								Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg)));
							o += strlen(o);
							break;
						case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (double) GETARG_FLOAT(thisarg) : (double) GETARG_INT(thisarg)));
							else
								Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (double) GETARG_FLOAT(thisarg) : (double) GETARG_INT(thisarg)));
							o += strlen(o);
							break;
						case 'v': case 'V':
							f[-2] += 'g' - 'v';
							if(precision < 0) // not set
								Q_snprintfz(o, end - o, va("%s %s %s", /* NESTED SPRINTF IS NESTED */ formatbuf, formatbuf, formatbuf),
									width, (isfloat ? (double) GETARG_VECTOR(thisarg)[0] : (double) GETARG_INTVECTOR(thisarg)[0]),
									width, (isfloat ? (double) GETARG_VECTOR(thisarg)[1] : (double) GETARG_INTVECTOR(thisarg)[1]),
									width, (isfloat ? (double) GETARG_VECTOR(thisarg)[2] : (double) GETARG_INTVECTOR(thisarg)[2])
								);
							else
								Q_snprintfz(o, end - o, va("%s %s %s", /* NESTED SPRINTF IS NESTED */ formatbuf, formatbuf, formatbuf),
									width, precision, (isfloat ? (double) GETARG_VECTOR(thisarg)[0] : (double) GETARG_INTVECTOR(thisarg)[0]),
									width, precision, (isfloat ? (double) GETARG_VECTOR(thisarg)[1] : (double) GETARG_INTVECTOR(thisarg)[1]),
									width, precision, (isfloat ? (double) GETARG_VECTOR(thisarg)[2] : (double) GETARG_INTVECTOR(thisarg)[2])
								);
							o += strlen(o);
							break;
						case 'c':
							//UTF-8-FIXME: figure it out yourself
//							if(flags & PRINTF_ALTERNATE)
							{
								if(precision < 0) // not set
									Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg)));
								else
									Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg)));
								o += strlen(o);
							}
/*							else
							{
								unsigned int c = (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg));
								char charbuf16[16];
								const char *buf = u8_encodech(c, NULL, charbuf16);
								if(!buf)
									buf = "";
								if(precision < 0) // not set
									precision = end - o - 1;
								o += u8_strpad(o, end - o, buf, (flags & PRINTF_LEFT) != 0, width, precision);
							}
*/							break;
						case 's':
							//UTF-8-FIXME: figure it out yourself
//							if(flags & PRINTF_ALTERNATE)
							{
								if(precision < 0) // not set
									Q_snprintfz(o, end - o, formatbuf, width, GETARG_STRING(thisarg));
								else
									Q_snprintfz(o, end - o, formatbuf, width, precision, GETARG_STRING(thisarg));
								o += strlen(o);
							}
/*							else
							{
								if(precision < 0) // not set
									precision = end - o - 1;
								o += u8_strpad(o, end - o, GETARG_STRING(thisarg), (flags & PRINTF_LEFT) != 0, width, precision);
							}
*/							break;
						default:
							printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
					}
				}
				++s;
				break;
			default:
verbatim:
				if(o < end - 1)
					*o++ = *s;
				s++;
				break;
		}
	}
finished:
	*o = 0;
}

void PF_printf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char outbuf[4096];
	PF_sprintf_internal(prinst, pr_globals, PR_GetStringOfs(prinst, OFS_PARM0), 1, outbuf, sizeof(outbuf));
	printf("%s", outbuf);
}

struct edict_s
{
	enum ereftype_e	ereftype;
	float			freetime;			// realtime when the object was freed
	unsigned int	entnum;
	unsigned int	fieldsize;
	pbool			readonly;	//causes error when QC tries writing to it. (quake's world entity)
	void			*fields;
};
void PF_spawn (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	struct edict_s	*ed;
	ed = ED_Alloc(prinst, false, 0);
	pr_globals = PR_globals(prinst, PR_CURRENT);
	RETURN_EDICT(prinst, ed);
}
void PF_remove (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	struct edict_s *ed = (void*)G_EDICT(prinst, OFS_PARM0);
	if (ed->ereftype == ER_FREE)
	{
		printf("Tried removing free entity\n");
		PR_StackTrace(prinst, false);
		return;
	}
	ED_Free (prinst, (void*)ed);
}

void PF_error(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	PF_puts(prinst, pr_globals);
}

void PF_bad (pubprogfuncs_t *prinst, struct globalvars_s *gvars)
{
	printf("bad builtin\n");
}

builtin_t builtins[] = {
	PF_bad,
	PF_puts,
	PF_ftos,
	PF_spawn,
	PF_remove,
	PF_vtos,
	PF_error,
	PF_vlen,
	PF_etos,
	PF_stof,
	PF_strcat,
	PF_strcmp,
	PF_normalize,
	PF_sqrt,
	PF_floor,
	PF_pow,
	PF_stov,
	PF_itos,
	PF_ltos,
	PF_dtos,
	PF_puts,
	PF_putv,
	PF_putf,
	PF_printf,
};




//Called when the qc library has some sort of serious error.
void Sys_Abort(const char *s, ...)
{	//quake handles this with a longjmp.
	va_list ap;
	va_start(ap, s);
	vprintf(s, ap);
	va_end(ap);
	exit(1);
}
//Called when the library has something to say.
//Kinda required for the compiler...
//Not really that useful for the normal vm.
int Sys_Printf(char *s, ...)
{	//look up quake's va function to find out how to deal with variable arguments properly.
	return printf("%s", s);
}

#include <stdio.h>
//copy file into buffer. note that the buffer will have been sized to fit the file (obtained via FileSize)
void *PDECL Sys_ReadFile(const char *fname, unsigned char *(PDECL *buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size, pbool issourcefile)
{
	void *buffer;
	int len;
	FILE *f;
	if (!strncmp(fname, "src/", 4))
		fname+=4;	//skip the src part
	f = fopen(fname, "rb");
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	len = ftell(f);

	buffer = buf_get(buf_ctx, len);
	fseek(f, 0, SEEK_SET);
	fread(buffer, 1, len, f);
	fclose(f);

	*out_size = len;
	return buffer;
}
//Finds the size of a file.
int Sys_FileSize (const char *fname)
{
	int len;
	FILE *f;
	if (!strncmp(fname, "src/", 4))
		fname+=4;	//skip the src part
	f = fopen(fname, "rb");
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fclose(f);
	return len;
}
//Writes a file.
pbool Sys_WriteFile (const char *fname, void *data, int len)
{
	FILE *f;
	f = fopen(fname, "wb");
	if (!f)
		return 0;
	fwrite(data, 1, len, f);
	fclose(f);
	return 1;
}


void ASMCALL StateOp(pubprogfuncs_t *prinst, float var, func_t func)
{	//note: inefficient. stupid globals abuse and not making assumptions about fields
	int *selfg = (int*)PR_FindGlobal(prinst, "self", 0, NULL);
	struct edict_s *ed = PROG_TO_EDICT(prinst, selfg?*selfg:0);
	float *time = (float*)PR_FindGlobal(prinst, "time", 0, NULL);
	eval_t *think = prinst->GetEdictFieldValue(prinst, ed, "think", ev_function, NULL);
	eval_t *nextthink = prinst->GetEdictFieldValue(prinst, ed, "nextthink", ev_float, NULL);
	eval_t *frame = prinst->GetEdictFieldValue(prinst, ed, "frame", ev_float, NULL);
	if (time && nextthink)
		nextthink->_float = *time+0.1;
	if (think)
		think->function = func;
	if (frame)
		frame->_float = var;
}
void ASMCALL CStateOp(pubprogfuncs_t *progs, float first, float last, func_t currentfunc)
{
/*
	float min, max;
	float step;
	float *vars = PROG_TO_WEDICT(progs, *w->g.self)->fields;
	float frame = e->v->frame;

//	if (progstype == PROG_H2)
//		e->v->nextthink = *w->g.time+0.05;
//	else
		e->v->nextthink = *w->g.time+0.1;
	e->v->think = currentfunc;

	if (csqcg.cycle_wrapped)
		*csqcg.cycle_wrapped = false;

	if (first > last)
	{	//going backwards
		min = last;
		max = first;
		step = -1.0;
	}
	else
	{	//forwards
		min = first;
		max = last;
		step = 1.0;
	}
	if (frame < min || frame > max)
		frame = first;	//started out of range, must have been a different animation
	else
	{
		frame += step;
		if (frame < min || frame > max)
		{	//became out of range, must have wrapped
			if (csqcg.cycle_wrapped)
				*csqcg.cycle_wrapped = true;
			frame = first;
		}
	}
	e->v->frame = frame;
*/
}
static void ASMCALL CWStateOp (pubprogfuncs_t *prinst, float first, float last, func_t currentfunc)
{
/*
	float min, max;
	float step;
	world_t *w = prinst->parms->user;
	wedict_t *e = PROG_TO_WEDICT(prinst, *w->g.self);
	float frame = e->v->weaponframe;

	e->v->nextthink = *w->g.time+0.1;
	e->v->think = currentfunc;

	if (csqcg.cycle_wrapped)
		*csqcg.cycle_wrapped = false;

	if (first > last)
	{	//going backwards
		min = last;
		max = first;
		step = -1.0;
	}
	else
	{	//forwards
		min = first;
		max = last;
		step = 1.0;
	}
	if (frame < min || frame > max)
		frame = first;	//started out of range, must have been a different animation
	else
	{
		frame += step;
		if (frame < min || frame > max)
		{	//became out of range, must have wrapped
			if (csqcg.cycle_wrapped)
				*csqcg.cycle_wrapped = true;
			frame = first;
		}
	}
	e->v->weaponframe = frame;
*/
}
void ASMCALL ThinkTimeOp(pubprogfuncs_t *prinst, struct edict_s *ed, float var)
{
	float *self = (float*)PR_FindGlobal(prinst, "self", 0, NULL);
	float *time = (float*)PR_FindGlobal(prinst, "time", 0, NULL);
	int *nextthink = (int*)PR_FindGlobal(prinst, "nextthink", 0, NULL);
	float *vars = PROG_TO_EDICT(prinst, self?*self:0)->fields;
	if (time && nextthink)
		vars[*nextthink] = *time+0.1;
}


void runtest(const char *progsname, const char **args)
{
	pubprogfuncs_t *pf;
	func_t func;
	progsnum_t pn;

	progparms_t ext;
	memset(&ext, 0, sizeof(ext));

	ext.progsversion = PROGSTRUCT_VERSION;
	ext.ReadFile = Sys_ReadFile;
	ext.FileSize= Sys_FileSize;
	ext.Sys_Error = Sys_Abort;
	ext.Abort = Sys_Abort;
	ext.Printf = printf;
	ext.stateop = StateOp;
	ext.cstateop = CStateOp;
	ext.cwstateop = CWStateOp;
	ext.thinktimeop = ThinkTimeOp;

	ext.numglobalbuiltins = sizeof(builtins)/sizeof(builtins[0]);
	ext.globalbuiltins = builtins;

	pf = InitProgs(&ext);
	pf->Configure(pf, 1024*1024*64, 1, false);	//memory quantity of 1mb. Maximum progs loadable into the instance of 1
//If you support multiple progs types, you should tell the VM the offsets here, via RegisterFieldVar
	pn = pf->LoadProgs(pf, progsname);	//load the progs.
	if (pn < 0)
		printf("test: Failed to load progs \"%s\"\n", progsname);
	else
	{
//allocate qc-acessable strings here for 64bit cpus. (allocate via AddString, tempstringbase is a holding area not used by the actual vm)
//you can call functions before InitEnts if you want. it's not really advised for anything except naming additional progs. This sample only allows one max.

		pf->InitEnts(pf, 10);		//Now we know how many fields required, we can say how many maximum ents we want to allow. 10 in this case. This can be huge without too many problems.

//now it's safe to ED_Alloc.

		func = pf->FindFunction(pf, "main", PR_ANY);	//find the function 'main' in the first progs that has it.
		if (!func)
			printf("Couldn't find function\n");
		else
		{	//feed it some complex args.
			void *pr_globals = PR_globals(pf, PR_CURRENT);
			int i;
			const char *atypes = *args++;
			for (i = 0; atypes[i]; i++) switch(atypes[i])
			{
			case 'f':
				G_FLOAT(OFS_PARM0+i*3) = atof(*args++);
				break;
			case 'v':
				sscanf(*args++, " %f %f %f ", &G_VECTOR(OFS_PARM0+i*3)[0], &G_VECTOR(OFS_PARM0+i*3)[1], &G_VECTOR(OFS_PARM0+i*3)[2]);
				break;
			case 's':
				G_INT(OFS_PARM0+i*3) = pf->TempString(pf, *args++);
				break;
			}

			pf->ExecuteProgram(pf, func);			//call the function
		}
	}
	pf->Shutdown(pf);
}

//Run a compiler and nothing else.
//Note that this could be done with an autocompile of PR_COMPILEALWAYS.
pbool compile(int argc, const char **argv)
{
	pbool success = false;
	pubprogfuncs_t *pf;

	progparms_t ext;

	if (0)
	{
		char *testsrcfile =	//newstyle progs.src must start with a #.
						//it's newstyle to avoid using multiple source files.
				 	"#pragma PROGS_DAT \"testprogs.dat\"\r\n"
					"//INTERMEDIATE FILE - EDIT TEST.C INSTEAD\r\n"
					"\r\n"
					"void(...) print = #1;\r\n"
					"void() main =\r\n"
					"{\r\n"
					"	print(\"hello world\\n\");\r\n"
					"};\r\n";

		//so that the file exists. We could insert it via the callbacks instead
		Sys_WriteFile("progs.src", testsrcfile, strlen(testsrcfile));
	}

	memset(&ext, 0, sizeof(ext));
	ext.progsversion = PROGSTRUCT_VERSION;
	ext.ReadFile = Sys_ReadFile;
	ext.FileSize= Sys_FileSize;
	ext.WriteFile= Sys_WriteFile;
	ext.Abort = Sys_Abort;
	ext.Printf = printf;

	pf = InitProgs(&ext);
	if (pf->StartCompile)
	{
		if (pf->StartCompile(pf, argc, argv))
		{
			while(pf->ContinueCompile(pf) == 1)
				;
			success = true;
		}
		else
			printf("compilation failed to start\n");
	}
	else
		printf("no compiler in this qcvm build\n");

	pf->Shutdown(pf);
	return success;
}

int main(int argc, const char **argv)
{
	int i, a=0;
	char atypes[9];
	const char *args[9] = {atypes};
	const char *dat = NULL;
	if (argc < 2)
	{
		printf("Invalid arguments!\nPlease run as, for example:\n%s testprogs.dat -srcfile progs.src\nThe first argument is the name of the progs.dat to run, the remaining arguments are the qcc args to use\n", argv[0]);
		return 0;
	}

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-float"))			{atypes[a] = 'f'; args[++a] = argv[++i];}
		else if (!strcmp(argv[i], "-vector"))	{atypes[a] = 'v'; args[++a] = argv[++i];}
		else if (!strcmp(argv[i], "-string"))	{atypes[a] = 's'; args[++a] = argv[++i];}
		else if (!strcmp(argv[i], "-srcfile"))	{if (!compile(argc-i, argv+i))return EXIT_FAILURE; break;}	//compile it, woo. consume the rest of the args, too
		else if (!dat && argv[i][0] != '-')		{dat = argv[i];}
		else									{printf("unknown arg %s\n", argv[i]); return EXIT_FAILURE;}
	}
	atypes[a] = 0;
	if (dat)
		runtest(dat, args);
	else
		printf("Nothing to run\n");

	return EXIT_SUCCESS;
}
