/*
    q3edw.c - DWARF debug info symbolization for Android native backtraces.

    Resolves raw backtrace lines such as:
        #01 pc 00000000003f2970  /data/app/.../lib/arm64/libdoom3.so (RB_GLSL_CreateDrawInteractions_shadowMapping(drawSurf_s const*)+896)

    into the corresponding source file and line number by reading DWARF line info
    from the on-disk .so using libdwarf.
*/

#include "q3edw.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <android/log.h>
#include <dlfcn.h>

#include <libdwarf/libdwarf.h>
#include <libdwarf/dwarf.h>

#include "q3estd.h"

#define LOG_TAG        "Q3E::dwarf"
#define LOGI2(fmt, args...) do { printf(fmt "\n", ##args); __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args); } while(0);

#define MAX_OS_PATH 1024

typedef struct dwarfFrame_s {
    char *text; // raw text content (trimmed)
    int pc; // frame index (#NN -> NN)
    uintptr_t address; // hex address string
    char *lib; // library path
    char *function; // function name
    int offset; // function offset

    char *file; // file path
    char *symbol; // real symbol name
    int line; // line num
} dwarfFrame_t;

typedef struct dwarfFrameGroup_s {
    Dwarf_Debug context;
    dwarfFrame_t **dw;
    int num;
} dwarfFrameGroup_t;

static void Q3E_DW_AllocDwarfFrame(dwarfFrame_t *df)
{
    memset(df, 0, sizeof(*df));
}

static void Q3E_DW_FreeDwarfFrame(dwarfFrame_t *df)
{
    free(df->text);
    free(df->lib);
    free(df->function);

    free(df->file);
    free(df->symbol);
}

static void Q3E_DW_AllocDwarfFrameGroup(dwarfFrameGroup_t *group, int num)
{
    memset(group, 0, sizeof(*group));
    group->num = num;
    group->dw = (dwarfFrame_t **)malloc(sizeof(*group->dw) * num);
}

static void Q3E_DW_FreeDwarfFrameGroup(dwarfFrameGroup_t *group)
{
    int i;
    for(i = 0; i < group->num; i++)
        Q3E_DW_FreeDwarfFrame(group->dw[i]);
    free(group->dw);
    if(group->context)
        dwarf_finish(group->context);
}

#define STRDUP(x) strdup(x)
#define ISSPACE(x) isspace((unsigned char)(x))
#define ISDIGIT(x) isdigit((unsigned char)(x))

// copy and alloc string
// if start == -1, start from index 0
// if length == -1, until src's end
static char * Q3E_DW_Strdup(const char *src, int start, int length)
{
    if (!src)
        return NULL;

    int src_len = (int)strlen(src);
    if (start < 0)
        start = 0;
    if (length < 0)
        length = src_len;

    if (start >= src_len || length == 0)
        return strdup("");

    int avail = src_len - start;
    if (length > avail)
        length = avail;

    char *dst = (char *)malloc((size_t)length + 1);
    if (!dst)
        return NULL;

    memcpy(dst, src + start, (size_t)length);
    dst[length] = '\0';

    return dst;
}

// trim space head and tail characters, return index of first non-space character and length
static int Q3E_DW_TrimString(const char *str, int *length)
{
    if (length)
        *length = 0;
    if (!str)
        return 0;

    int total = (int)strlen(str);

    /* find first non-whitespace from the head */
    int start = 0;
    while (start < total && ISSPACE(str[start]))
        ++start;

    /* find last non-whitespace from the tail */
    int end = total;
    while (end > start && ISSPACE(str[end - 1]))
        --end;

    if (length)
        *length = end - start;

    return start;
}

// skip space characters, return address, and skip num of characters
static char * Q3E_DW_SkipSpace(char *str, int *num)
{
    if (num)
        *num = 0;
    if (!str)
        return NULL;

    int i = 0;
    while (str[i] && ISSPACE(str[i]))
        ++i;

    if (num)
        *num = i;

    return str + i;
}

static char * Q3E_DW_SkipChars(char *str, const char *find, int *num)
{
    if (num)
        *num = 0;
    if (!str)
        return NULL;
    if (!find || !find[0])
        return str;

    int i = 0;
    while (str[i] && strchr(find, str[i]))
        ++i;

    if (num)
        *num = i;

    return str + i;
}

static char * Q3E_DW_ReadLine(char *str, int *num)
{
    if (num)
        *num = 0;
    if (!str)
        return NULL;

    int i = 0;
    while (str[i] && str[i] != '\n' && str[i] != '\r')
        ++i;

    char *res = str + i;

    if (str[i] == '\r')
        ++i;
    if (str[i] == '\n')
        ++i;

    if (num)
        *num = i;

    return res;
}

static char * Q3E_DW_UntilChars(char *str, const char *find, int *num)
{
    if (num)
        *num = 0;
    if (!str)
        return NULL;
    if (!find || !find[0])
        return str;

    int i = 0;
    while (str[i] && (!strchr(find, str[i]) && !ISSPACE(str[i])))
        ++i;

    if (num)
        *num = i;

    return str + i;
}

// line is #01 pc 00000000003f2970  /data/app/.../lib/arm64/libdoom3.so (RB_GLSL_CreateDrawInteractions_shadowMapping(drawSurf_s const*)+896)
// return 1 means success, 0 means fail
static int Q3E_DW_ParseDwarfFrame(dwarfFrame_t *df, const char *line)
{
    char ch;

    if (!line)
        return 0;

    Q3E_DW_AllocDwarfFrame(df);

    int text_length = 0;
    int start_index = Q3E_DW_TrimString(line, &text_length);
    df->text = Q3E_DW_Strdup(line, start_index, text_length);

    char *last_p = df->text;
    char *p = last_p;
    if(*p != '#')
    {
        LOGE("expect token '#', but found '%c' in '%s'", *p, df->text);
        Q3E_DW_FreeDwarfFrame(df);
        return 0;
    }
    ++p;
    //p = Q3E_DW_SkipChars(p, "0", NULL);
    last_p = p;
    while (ISDIGIT(*p))
        ++p;
    if(last_p != p)
    {
        ch = *p;
        *p = '\0';
        df->pc = atoi(last_p);
        *p = ch;
    }
    else
    {
        df->pc = 0;
    }

    p = Q3E_DW_SkipSpace(p, NULL);

    // skip pc token
    if ((p[0] == 'p' || p[0] == 'P') && (p[1] == 'c' || p[1] == 'C'))
        p += 2;
    else
    {
        LOGE("expect token 'pc', but found '%c%c' in '%s'", p[0], p[1], df->text);
        Q3E_DW_FreeDwarfFrame(df);
        return 0;
    }

    p = Q3E_DW_SkipSpace(p, NULL);

    // parse address
    last_p = p;
    if (*p == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        p += 2;
        last_p = p;
    }
    while (isxdigit((unsigned char)*p))
        ++p;
    int length = (int)(p - last_p);
    if (length == 0)
    {
        LOGE("expect hex address, but found '%s' in '%s'", p, df->text);
        Q3E_DW_FreeDwarfFrame(df);
        return 0;
    }
    ch = *p;
    *p = '\0';
    df->address = (uintptr_t)strtoull(last_p, NULL, 16);
    *p = ch;

    p = Q3E_DW_SkipSpace(p, NULL);

    // parse library path
    last_p = p;
    p = Q3E_DW_UntilChars(p, "(", &length);
    if (length == 0)
    {
        LOGE("expect library path, but found '%s' in '%s'", p, df->text);
        Q3E_DW_FreeDwarfFrame(df);
        return 0;
    }
    df->lib = Q3E_DW_Strdup(last_p, 0, length);

    p = Q3E_DW_SkipSpace(p, NULL);

    // parse function and offset if exists
    if (*p == '(') {
        ++p;
        last_p = p;

        /* find the matching ')' by walking the trimmed text backwards;
           function names may contain '(' (params), so match from the end */
        if (df->text[text_length - 1] != ')')
        {
            LOGE("expect token ')', but found '%c' in '%s'", df->text[text_length - 1], df->text);
            Q3E_DW_FreeDwarfFrame(df);
            return 0;
        }
        char *end_ptr = &df->text[text_length - 1];

        p = end_ptr - 1;
        while(p > last_p && ISDIGIT(*p))
            --p;

        if(p <= last_p || !ISDIGIT(*p) && *p != '+')
        {
            /* no offset digits: "(Func)" form - function is whole content.
               p either ran past last_p, or stopped on a non-digit,
               non-'+' char (e.g. the 'c' of "Func"). */
            ch = *end_ptr;
            *end_ptr = '\0';
            length = (int)(end_ptr - last_p);
            df->function = Q3E_DW_Strdup(last_p, 0, length);
            *end_ptr = ch;

            df->offset = 0;
        }
        else
        {
/*             if(*p != '+')
            {
                LOGE("expect token '+', but found '%c' in '%s'", *p, df->text);
                Q3E_DW_FreeDwarfFrame(df);
                return 0;
            } */

            // parse offset
            ch = *end_ptr;
            *end_ptr = '\0';
            df->offset = atoi(p + 1);
            *end_ptr = ch;

            // parse function name: last_p .. p (exclusive of '+')
            length = (int)(p - last_p);
            df->function = Q3E_DW_Strdup(last_p, 0, length);
        }
    }

    return 1;
}

// lines is #01 pc 00000000003f2970  /data/app/.../lib/arm64/libdoom3.so (RB_GLSL_CreateDrawInteractions_shadowMapping(drawSurf_s const*)+896)
//         #02 pc 00000000003f2970  /data/app/.../lib/arm64/libdoom3.so (RB_GLSL_CreateDrawInteractions_shadowMapping(drawSurf_s const*)+896)
//         ...
// if df_list only return num of lines
// return num of parsing
static int Q3E_DW_ParseDwarfFrames(dwarfFrame_t df_list[], int length, const char *lines)
{
    int res = 0;
    char ch;

    if (!lines)
        return 0;

    int parse_mode = (df_list && length > 0);
    char *text = STRDUP(lines);
    char *p = text;
    char *last_p;
    int line_len = 0;

    while (1) {
        last_p = p;
        p = Q3E_DW_ReadLine(p, &line_len);
        if (line_len == 0)
            break;

        if (parse_mode) {
            if (res >= length)
                break;

            ch = *p;
            *p = '\0';
			//LOGI("%s", last_p);

            if (Q3E_DW_ParseDwarfFrame(&df_list[res], last_p))
                ++res;

            *p = ch;
        } else {
            ++res;
        }
        p = last_p + line_len;
    }

    free(text);
    return res;
}

// group df_list by library path. Consecutive frames sharing the same lib
// are folded into one group. dw points to the first frame of the group,
// num is the count of consecutive frames in that group.
// If groups == NULL or length <= 0, only return the number of groups.
static int Q3E_DW_GroupDwarfFrames(dwarfFrameGroup_t groups[], int length, dwarfFrame_t df_list[], int num_df)
{
    int res = 0;
    int parse_mode = (groups && length > 0);

    if (!df_list || num_df <= 0)
        return 0;

    /* libs[k] = representative lib string for group k
       counts[k] = number of frames in group k */
    const char **libs = (const char **)malloc(sizeof(*libs) * num_df);
    int *counts = (int *)malloc(sizeof(*counts) * num_df);
    /* for parse_mode: which group index each df_list[i] belongs to */
    int *slot = parse_mode ? (int *)malloc(sizeof(*slot) * num_df) : NULL;
    dwarfFrame_t *dw;

    int i, j;
    for (i = 0; i < num_df; i++) {
        dw = &df_list[i];
        for (j = 0; j < res; j++) {
            if (!strcmp(libs[j], dw->lib))
                break;
        }

        if (j == res) {
            /* new group */
            libs[res] = dw->lib;
            counts[res] = 1;
            if (slot)
                slot[i] = res;
            ++res;
        } else {
            ++counts[j];
            if (slot)
                slot[i] = j;
        }
    }

    if (parse_mode) {
        /* filled[g] = how many dw[] entries written into group g so far.
           out_idx[g] = output slot in groups[] for group id g
                        (-1 means not allocated yet, -2 means skipped). */
        int *filled  = (int *)calloc((size_t)res, sizeof(*filled));
        int *out_idx = (int *)malloc(sizeof(*out_idx) * res);
        int ng = 0; /* number of groups actually allocated (<= length) */
        for (i = 0; i < res; i++)
            out_idx[i] = -1;

        for (i = 0; i < num_df; i++) {
            int g = slot[i];
            if (out_idx[g] < 0) {
                /* first frame of this group: allocate the output group */
                if (ng >= length) {
                    out_idx[g] = -2;  /* buffer full, skip this group */
                    continue;
                }
                out_idx[g] = ng;
                Q3E_DW_AllocDwarfFrameGroup(&groups[ng], counts[g]);
                groups[ng].dw[0] = &df_list[i];
                filled[g] = 1;
                ++ng;
            } else if (out_idx[g] >= 0) {
                /* subsequent frame of an already-allocated group */
                int oi = out_idx[g];
                groups[oi].dw[filled[g]] = &df_list[i];
                ++filled[g];
            }
            /* out_idx[g] == -2: group skipped, drop frame */
        }
        free(filled);
        free(out_idx);
    }

    free(libs);
    free(counts);
    free(slot);

    return res;
}

/* -------------------------------------------------------------------------- */
/* helpers                                                                    */
/* -------------------------------------------------------------------------- */

static void free_error(Dwarf_Debug dbg, Dwarf_Error err)
{
    if (err) {
		int eno = dwarf_errno(err);
		const char *emsg = dwarf_errmsg(err);
		LOGE("dwarf error: %d -> %s", eno, emsg);
        if (dbg)
            dwarf_dealloc_error(dbg, err);
    }
}

static int copy_to_buf(char *dst, int cap, const char *src)
{
    if (!dst || cap <= 0)
        return 0;
    dst[0] = '\0';
    if (!src)
        return 0;
    int n = (int)strlen(src);
    if (n >= cap)
        n = cap - 1;
    if (n > 0)
        memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
    return n;
}

/* Demangle a C++ symbol name using __cxa_demangle (from the C++ runtime).
   If demangling succeeds the result is written to out (up to cap bytes);
   otherwise the original mangled name is copied verbatim. */
static void demangle_copy(const char *mangled, char *out, int cap)
{
    if (!mangled || !out || cap <= 0)
        return;

    /* __cxa_demangle is in the C++ runtime (libc++_shared.so or libc++.so).
       Use dlsym to obtain it so this C file does not need a C++ link dep. */
    typedef char *(*demangle_fn)(const char *, char *, size_t *, int *);
    static demangle_fn fn = (demangle_fn)-1;
    if (fn == (demangle_fn)-1) {
        fn = (demangle_fn)(uintptr_t)dlsym(RTLD_DEFAULT, "__cxa_demangle");
        //if (!fn) {
            //LOGW("Q3E::dwarf: __cxa_demangle not found, C++ names will stay mangled");
		//}
    }

    if (fn) {
        int status = 0;
        char *demangled = fn(mangled, NULL, NULL, &status);
        if (status == 0 && demangled) {
            copy_to_buf(out, cap, demangled);
            free(demangled);
            return;
        }
        if (demangled)
            free(demangled);
    }

    /* fallback: copy mangled name as-is */
    copy_to_buf(out, cap, mangled);
}

/*
    Parse a backtrace line into its address + library path.
    Accepted form:
        #01 pc 00000000003f2970  /path/libfoo.so (Symbol+off)
    Returns 1 on success, 0 on parse failure. out_addr is filled only on success.
    out_lib is filled even if the symbol part in parentheses is present.
*/
static int parse_backtrace_line(const char *line,
                                Dwarf_Addr *out_addr,
                                char *out_lib,
                                int   lib_cap)
{
    const char *p = line;
    if (!p)
        return 0;

    /* skip leading '#NN' */
    while (*p == '#') {
        ++p;
        while (ISDIGIT(*p))
            ++p;
    }

    /* skip whitespace */
    while (*p == ' ' || *p == '\t')
        ++p;

    /* 'pc' (case-insensitive) */
    if ((p[0] == 'p' || p[0] == 'P') && (p[1] == 'c' || p[1] == 'C'))
        p += 2;
    while (*p == ' ' || *p == '\t')
        ++p;

    /* hex address (no '0x' prefix in Android backtraces) */
    if (*p == '0' && (p[1] == 'x' || p[1] == 'X'))
        p += 2;

    Dwarf_Addr addr = 0;
    int digits = 0;
    while (*p) {
        int v;
        if (*p >= '0' && *p <= '9')      v = *p - '0';
        else if (*p >= 'a' && *p <= 'f') v = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F') v = *p - 'A' + 10;
        else break;
        addr = (addr << 4) | (Dwarf_Addr)v;
        ++p;
        ++digits;
    }
    if (digits == 0)
        return 0;
    *out_addr = addr;

    /* skip whitespace */
    while (*p == ' ' || *p == '\t')
        ++p;

    /* library path ends at the next whitespace or '(' */
    const char *start = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '(' && *p != '\n' && *p != '\r')
        ++p;

    int n = (int)(p - start);
    if (n <= 0 || n >= lib_cap)
        return 0;

    memcpy(out_lib, start, (size_t)n);
    out_lib[n] = '\0';
    return 1;
}

/* -------------------------------------------------------------------------- */
/* function name lookup via DW_TAG_subprogram                                 */
/* -------------------------------------------------------------------------- */

/* Try to extract a name string from a DIE via dwarf_attr + dwarf_formstring.
   Checks DW_AT_linkage_name first, then DW_AT_name.  Returns 1 on success. */
static int die_get_name(Dwarf_Debug dbg, Dwarf_Die die,
                        char *out, int cap)
{
    static const int name_attrs[] = { DW_AT_linkage_name, DW_AT_name };
    Dwarf_Error err = 0;
    int ai;
    for (ai = 0; ai < (int)(sizeof(name_attrs) / sizeof(name_attrs[0])); ++ai) {
        Dwarf_Attribute attr = 0;
        if (dwarf_attr(die, name_attrs[ai], &attr, &err) == DW_DLV_OK && attr) {
            char *s = 0;
            if (dwarf_formstring(attr, &s, &err) == DW_DLV_OK && s) {
                demangle_copy(s, out, cap);
                dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
                return 1;
            }
            free_error(dbg, err); err = 0;
            dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
        } else {
            free_error(dbg, err); err = 0;
        }
    }
    return 0;
}

/* Follow a DW_AT_abstract_origin or DW_AT_specification reference and
   extract the name from the target DIE.  Returns 1 on success. */
static int die_resolve_ref_name(Dwarf_Debug dbg, Dwarf_Die die,
                                unsigned attr_code,
                                char *out, int cap)
{
    Dwarf_Error err = 0;
    Dwarf_Attribute attr = 0;
    if (dwarf_attr(die, attr_code, &attr, &err) != DW_DLV_OK || !attr) {
        free_error(dbg, err);
        return 0;
    }
    Dwarf_Off ref_off = 0;
    int ok = 0;
    if (dwarf_global_formref(attr, &ref_off, &err) == DW_DLV_OK) {
        Dwarf_Die ref_die = 0;
        if (dwarf_offdie_b(dbg, ref_off, 1, &ref_die, &err) == DW_DLV_OK && ref_die)
            ok = die_get_name(dbg, ref_die, out, cap);
    }
    free_error(dbg, err); err = 0;
    dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
    return ok;
}

/* Get the function name from a DW_TAG_subprogram DIE, possibly following
   DW_AT_abstract_origin / DW_AT_specification. */
static int subprogram_name(Dwarf_Debug dbg, Dwarf_Die die,
                           char *out, int cap)
{
    if (die_get_name(dbg, die, out, cap))
        return 1;
    if (die_resolve_ref_name(dbg, die, DW_AT_abstract_origin, out, cap))
        return 1;
    if (die_resolve_ref_name(dbg, die, DW_AT_specification, out, cap))
        return 1;
    return 0;
}

/* Recursively search the DIE tree rooted at 'die' for a DW_TAG_subprogram
   whose address range contains 'pc'.  Fills out_func on success.
   Returns 1 if found, 0 otherwise. */
static int search_funcname(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Addr pc,
                           char *out_func, int out_func_cap)
{
    Dwarf_Error err = 0;
    int found = 0;

    while (die && !found) {
        Dwarf_Half tag = 0;
        if (dwarf_tag(die, &tag, &err) != DW_DLV_OK) {
            free_error(dbg, err);
            err = 0;
            break;
        }

        if (tag == DW_TAG_subprogram) {
            Dwarf_Addr lowpc = 0;
            if (dwarf_lowpc(die, &lowpc, &err) == DW_DLV_OK) {
                Dwarf_Addr highpc_val = 0;
                Dwarf_Half highpc_form = 0;
                enum Dwarf_Form_Class highpc_cls = 0;
                if (dwarf_highpc_b(die, &highpc_val, &highpc_form, &highpc_cls, &err) == DW_DLV_OK) {
                    Dwarf_Addr highpc = highpc_val;
                    if (highpc_form != DW_FORM_addr)
                        highpc = lowpc + highpc_val;
                    if (pc >= lowpc && pc < highpc)
                        found = subprogram_name(dbg, die, out_func, out_func_cap);
                }
            }
            free_error(dbg, err); err = 0;
        }

        /* Recurse into children (namespaces, classes, etc.) */
        if (!found) {
            Dwarf_Die child = 0;
            if (dwarf_child(die, &child, &err) == DW_DLV_OK && child)
                found = search_funcname(dbg, child, pc, out_func, out_func_cap);
            free_error(dbg, err); err = 0;
        }

        /* Move to next sibling */
        if (!found) {
            Dwarf_Die sibling = 0;
            if (dwarf_siblingof_b(dbg, die, 1, &sibling, &err) == DW_DLV_OK && sibling)
                die = sibling;
            else {
                free_error(dbg, err); err = 0;
                die = 0;
            }
        }
    }
    return found;
}

/* Look up the function name for 'pc' by walking all CUs and searching
   each CU's DIE tree for a matching DW_TAG_subprogram. */
static int lookup_funcname(Dwarf_Debug dbg, Dwarf_Addr pc,
                           char *out_func, int out_func_cap)
{
    if (!out_func || out_func_cap <= 0)
        return 0;
    out_func[0] = '\0';

    Dwarf_Error err = 0;
    Dwarf_Bool is_info = 1;
    Dwarf_Die cu_die = 0;
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Half extension_size = 0;
    Dwarf_Half length_size = 0;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_offset = 0;
    Dwarf_Half cu_type = 0;
    Dwarf_Sig8 signature;

    while (dwarf_next_cu_header_e(dbg, is_info,
                                  &cu_die,
                                  &cu_header_length, &version_stamp,
                                  &abbrev_offset, &address_size,
                                  &length_size, &extension_size,
                                  &signature, &typeoffset,
                                  &next_cu_offset,
                                  &cu_type, &err) == DW_DLV_OK) {
        if (search_funcname(dbg, cu_die, pc, out_func, out_func_cap)) {
            free_error(dbg, err);
            return 1;
        }
    }
    free_error(dbg, err);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* core resolution                                                            */
/* -------------------------------------------------------------------------- */

static int Q3E_DW_LoadFrame(dwarfFrame_t *df, Dwarf_Debug *in_dbg, int best_fit)
{
    Dwarf_Addr pc = df->address;
    int res = 0;

    /* The PC in a backtrace is a return address (the instruction after the
       call).  Subtract 1 so that the lookup falls into the calling
       instruction's range in the line table rather than the next line. */
    if (pc > 0)
        --pc;

    Dwarf_Debug dbg = *in_dbg;
    Dwarf_Error err = NULL;
    int rc;
    if(!dbg)
    {
        char        true_path[MAX_OS_PATH];
        rc = dwarf_init_path(df->lib,
                             true_path,
                             (unsigned int)sizeof(true_path),
                             DW_GROUPNUMBER_ANY,
            /*dw_errhand=*/NULL,
            /*dw_errarg=*/NULL,
                             &dbg,
                             &err);
        if (rc != DW_DLV_OK) {
            LOGE("Q3E_DW_LoadFrame: dwarf_init_path failed on %s (rc=%d)", df->lib, rc);
            free_error(dbg, err);
            return 0;
        }
        *in_dbg = dbg;
    }

    /* --- aranges: fast PC -> CU lookup --- */
    Dwarf_Arange *aranges = 0;
    Dwarf_Signed  arange_count = 0;
    rc = dwarf_get_aranges(dbg, &aranges, &arange_count, &err);
    if (rc != DW_DLV_OK) {
        //LOGW("Q3E_DW_LoadFrame: no .debug_aranges in %s, falling back to linear scan", df->lib);
        free_error(dbg, err);
        err = 0;
        aranges = 0;
    }

    Dwarf_Arange matched = 0;
    if (aranges) {
        if (dwarf_get_arange(aranges, (Dwarf_Unsigned)arange_count, pc, &matched, &err) == DW_DLV_OK) {
            /* ok */
        } else {
            free_error(dbg, err);
            err = 0;
        }
    }

    Dwarf_Die cu_die = 0;
    if (matched) {
        Dwarf_Off die_off = 0;
        rc = dwarf_get_arange_info_b(matched, 0, 0, 0, 0, &die_off, &err);
        if (rc != DW_DLV_OK) {
            free_error(dbg, err);
            goto done;
        }
        rc = dwarf_offdie_b(dbg, die_off, /*is_info=*/1, &cu_die, &err);
        if (rc != DW_DLV_OK) {
            free_error(dbg, err);
            goto done;
        }
    } else {
        /* linear scan: walk every CU, do a full line lookup in each, and
           keep the best (closest address <= pc) match across all CUs.
           This is slower than aranges but necessary when .debug_aranges
           is absent (Clang does not emit it by default). */
        Dwarf_Bool is_info = 1;
        Dwarf_Line best_line = 0;
        Dwarf_Addr best_addr = 0;
        Dwarf_Line_Context best_ctx = 0;
        Dwarf_Unsigned cu_header_length = 0;
        Dwarf_Unsigned abbrev_offset = 0;
        Dwarf_Half version_stamp = 0;
        Dwarf_Half address_size = 0;
        Dwarf_Half extension_size = 0;
        Dwarf_Half length_size = 0;
        Dwarf_Unsigned typeoffset = 0;
        Dwarf_Unsigned next_cu_offset = 0;
        Dwarf_Half cu_type = 0;
        Dwarf_Sig8 signature;
        while (dwarf_next_cu_header_e(dbg, is_info,
                                      &cu_die,
                                      &cu_header_length, &version_stamp,
                                      &abbrev_offset, &address_size,
                                      &length_size,&extension_size,
                                      &signature, &typeoffset,
                                      &next_cu_offset,
                                      &cu_type, &err) == DW_DLV_OK) {
            Dwarf_Line_Context lctx = 0;
            Dwarf_Unsigned      ver = 0;
            Dwarf_Small         tbl_count = 0;
            if (dwarf_srclines_b(cu_die, &ver, &tbl_count, &lctx, &err) == DW_DLV_OK && lctx) {
                Dwarf_Line *lines = 0;
                Dwarf_Signed lcount = 0;
                if (dwarf_srclines_from_linecontext(lctx, &lines, &lcount, &err) == DW_DLV_OK && lines) {
                    Dwarf_Signed j;
                    Dwarf_Line  cu_best = 0;
                    Dwarf_Addr  cu_best_addr = 0;
                    for (j = 0; j < lcount; ++j) {
                        Dwarf_Addr a = 0;
                        if (dwarf_lineaddr(lines[j], &a, &err) != DW_DLV_OK) { free_error(dbg, err); err = 0; continue; }
                        if (a == 0) continue;
                        if (a == pc) { cu_best = lines[j]; cu_best_addr = a; break; }
                        if (best_fit && a < pc && (!cu_best || a > cu_best_addr)) {
                            cu_best = lines[j];
                            cu_best_addr = a;
                        }
                    }
                    if (cu_best && (!best_line || cu_best_addr > best_addr)) {
                        if (best_ctx)
                            dwarf_srclines_dealloc_b(best_ctx);
                        best_line = cu_best;
                        best_addr = cu_best_addr;
                        best_ctx  = lctx;
                        lctx = 0; /* prevent dealloc below */
                        if (cu_best_addr == pc)
                            break; /* exact match, no need to scan further */
                    }
                }
                free_error(dbg, err);
                err = 0;
                if (lctx)
                    dwarf_srclines_dealloc_b(lctx);
            }
            cu_die = 0;
        }
        free_error(dbg, err);
        err = 0;

        if (!best_line) {
            if (best_ctx)
                dwarf_srclines_dealloc_b(best_ctx);
            goto done;
        }

        char *src = 0;
        Dwarf_Unsigned lineno = 0;
        int ok = 1;
        if (dwarf_linesrc(best_line, &src, &err) != DW_DLV_OK) { free_error(dbg, err); err = 0; ok = 0; }
        if (dwarf_lineno(best_line, &lineno, &err) != DW_DLV_OK) { free_error(dbg, err); err = 0; ok = 0; }

        if (ok && src && lineno > 0) {
            df->file = STRDUP(src);
            df->line = (int)lineno;
            res = 1;
        }
        if (src) dwarf_dealloc(dbg, src, DW_DLA_STRING);
        if (best_ctx)
            dwarf_srclines_dealloc_b(best_ctx);
        goto done;
    }

    /* --- fetch line table of the CU and find the best line --- */
    Dwarf_Line_Context ctx = 0;
    Dwarf_Unsigned      ver = 0;
    Dwarf_Small         tbl_count = 0;
    if (dwarf_srclines_b(cu_die, &ver, &tbl_count, &ctx, &err) != DW_DLV_OK || !ctx) {
        free_error(dbg, err);
        goto done;
    }

    Dwarf_Line *lines = 0;
    Dwarf_Signed lcount = 0;
    if (dwarf_srclines_from_linecontext(ctx, &lines, &lcount, &err) != DW_DLV_OK || lcount <= 0) {
        free_error(dbg, err);
        dwarf_srclines_dealloc_b(ctx);
        goto done;
    }

    Dwarf_Addr best_addr = 0;
    Dwarf_Line best = 0;
    Dwarf_Signed i;
    for (i = 0; i < lcount; ++i) {
        Dwarf_Addr a = 0;
        Dwarf_Unsigned ln = 0;
        if (dwarf_lineaddr(lines[i], &a, &err) != DW_DLV_OK) { free_error(dbg, err); err = 0; continue; }
        if (a == 0) continue; /* skip end-of-sequence markers that have no real address */

        if (a == pc) {
            /* exact match wins immediately */
            best = lines[i];
            best_addr = a;
            break;
        }
        if (best_fit && a < pc && (!best || a > best_addr)) {
            best = lines[i];
            best_addr = a;
        }
    }

    if (!best) {
        dwarf_srclines_dealloc_b(ctx);
        goto done;
    }

    /* extract src path + line number */
    char *src = 0;
    Dwarf_Unsigned lineno = 0;
    int ok = 1;
    if (dwarf_linesrc(best, &src, &err) != DW_DLV_OK) {
        free_error(dbg, err);
        err = 0;
        ok = 0;
    }
    if (dwarf_lineno(best, &lineno, &err) != DW_DLV_OK) {
        free_error(dbg, err);
        err = 0;
        ok = 0;
    }

    if (ok && src && lineno > 0) {
        df->file = STRDUP(src);
        df->line = (int)lineno;
        res = 1;
    }

    if (src)
        dwarf_dealloc(dbg, src, DW_DLA_STRING);

    dwarf_srclines_dealloc_b(ctx);

    char func_buf[512];
    func_buf[0] = '\0';
done:
    /* Function name lookup after line lookup (separate CU pass so it
       does not interfere with the CU iteration above). */

    if (lookup_funcname(dbg, pc, func_buf, (int)sizeof(func_buf)) && func_buf[0])
        df->symbol = STRDUP(func_buf);

    if (aranges)
        dwarf_dealloc(dbg, (void *)aranges, DW_DLA_ARANGE);

    return res;
}

static int Q3E_DW_LoadFrameGroup(dwarfFrameGroup_t *dfg, int best_fit)
{
    int i;
    int suc = 0;
    for (i = 0; i < dfg->num; i++) {
        if (Q3E_DW_LoadFrame(dfg->dw[i], &dfg->context, best_fit))
            suc++;
    }
	//LOGI("%s: %d %d", dfg->dw[0]->lib, suc, dfg->num);
    return suc;
}

static int Q3E_DW_LoadFrameGroupList(dwarfFrameGroup_t dfg_list[], int num, int best_fit)
{
    int i;
    int suc = 0;
    for (i = 0; i < num; i++) {
        suc += Q3E_DW_LoadFrameGroup(&dfg_list[i], best_fit);
    }
    return suc;
}

static void Q3E_DW_PrintFrame(dwarfFrame_t *df, const char *prefix)
{
	//LOGI2("%s%s", prefix ? prefix : "", df->text);
    LOGI2("%s#%02d pc %016zx  %s (%s+%d) %s:%d %s", prefix ? prefix : "", df->pc, df->address, df->lib, df->function, df->offset, df->file, df->line, df->symbol);
	//LOGI2("---------------");
}

static void Q3E_DW_PrintFrameList(dwarfFrame_t df_list[], int num, const char *prefix)
{
    int i;
    for (i = 0; i < num; i++) {
        Q3E_DW_PrintFrame(&df_list[i], prefix);
    }
}

int Q3E_DW_Addr2line(const char *text, int best_fit)
{
    int numDf = Q3E_DW_ParseDwarfFrames(NULL, 0, text);
    if(!numDf)
        return 0;

    dwarfFrame_t *df_list = (dwarfFrame_t *)malloc(sizeof(*df_list) * numDf);
    numDf = Q3E_DW_ParseDwarfFrames(df_list, numDf, text);

    int numGroup = Q3E_DW_GroupDwarfFrames(NULL, 0, df_list, numDf);
    dwarfFrameGroup_t *dfg_list = (dwarfFrameGroup_t *)malloc(sizeof(*dfg_list) * numGroup);
    numGroup = Q3E_DW_GroupDwarfFrames(dfg_list, numGroup, df_list, numDf);

    int res = Q3E_DW_LoadFrameGroupList(dfg_list, numGroup, best_fit);

    LOGI2("[dwarf]: %d", numDf);
    Q3E_DW_PrintFrameList(df_list, numDf, "\t");

    int i;
    for (i = 0; i < numGroup; i++) {
        Q3E_DW_FreeDwarfFrameGroup(&dfg_list[i]);
    }

    free(dfg_list);
    free(df_list);

    return res;
}

#if 0
char * read_file(const char *path)
{
	FILE *f = fopen(path, "r");
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	char *text = (char *)malloc(size);
	fseek(f, 0, SEEK_SET);
	fread(text, 1, size, f);
	text[size - 1] = 0;
	fclose(f);
	return text;
}

int main()
{
	char *backtrace_line = "#00 pc 00000000003890e4  /sdcard/diii4a/doom3/libdoom3.so (RB_CreateSingleDrawInteractions(drawSurf_s const*, void (*)(drawInteraction_t const*))+1252)";
	backtrace_line = "#09 pc 00000000003890e8  /sdcard/diii4a/doom3/libdoom3.so (_ZL9GfxInfo_fRK9idCmdArgs+24)";
	backtrace_line = "#07 pc 00000000003890e4  /sdcard/diii4a/doom3/libdoom3.so (_ZL9GfxInfo_fRK9idCmdArgs+20)";
	backtrace_line = read_file("test.log");

	Q3E_DW_Addr2line(backtrace_line, 1);

	free(backtrace_line);
	return 0;
}
#endif
