/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "Engine/Base/Parser.y"

/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include <Engine/StdH.h>

#include <Engine/Base/Console.h>
#include <Engine/Base/Shell.h>
#include "ParsingSymbols.h"

#include <Engine/Templates/DynamicStackArray.cpp>
#include <Engine/Templates/AllocationArray.cpp>

#line 28 "Engine/Base/Parser.y"

// turn off over-helpful bit of bison... --ryan.
#ifdef __GNUC__
#define __attribute__(x)
#endif

#define YYERROR_VERBOSE 1
// if error occurs in parsing
void yyerror(const char *str)
{
  // just report the string
  _pShell->ErrorF("%s", str);
};

static BOOL _bExecNextElse = FALSE;
FLOAT fDummy = -666.0f;

static INDEX _iStack = 0;
static UBYTE _ubStack[1024];

INDEX PushExpression(value &v)
{
  if (v.sttType==STT_FLOAT) {
    FLOAT f = v.fFloat;
    memcpy(_ubStack+_iStack, &f, sizeof(f));
    _iStack+=sizeof(f);
    return sizeof(f);
  } else if (v.sttType==STT_INDEX) {
    INDEX i = v.iIndex;
    memcpy(_ubStack+_iStack, &i, sizeof(i));
    _iStack+=sizeof(i);
    return sizeof(i);
  } else if (v.sttType==STT_STRING) {
    CTString &str = _shell_astrTempStrings.Push();
    str = v.strString;
    CTString *pstr = &str;
    memcpy(_ubStack+_iStack, &pstr, sizeof(pstr));
    _iStack+=sizeof(pstr);
    return sizeof(pstr);
  } else {
    return 0;
  }
}

BOOL MatchTypes(value &v1, value &v2)
{
  if (v1.sttType==STT_FLOAT && v2.sttType==STT_FLOAT) {
    return TRUE;
  } else if (v1.sttType==STT_STRING && v2.sttType==STT_STRING) {
    return TRUE;
  } else if (v1.sttType==STT_INDEX && v2.sttType==STT_INDEX) {
    return TRUE;
  } else {
    v1.sttType = STT_ILLEGAL;
    v2.sttType = STT_ILLEGAL;
    _pShell->ErrorF("Type mismatch");
    return FALSE;
  }
}

void Declaration(
  ULONG ulQualifiers, INDEX istType, CShellSymbol &ssNew,
  INDEX (*pPreFunc)(INDEX), void (*pPostFunc)(INDEX))
{
  // if external
  if (ulQualifiers&SSF_EXTERNAL) {
    // get it a new value
    if (_shell_ast[istType].st_sttType==STT_INDEX
      ||_shell_ast[istType].st_sttType==STT_FLOAT) {
      _pvNextToDeclare = &_shell_afExtFloats.Push();
    } else if (_shell_ast[istType].st_sttType==STT_STRING) {
      _pvNextToDeclare = &_shell_astrExtStrings.Push();
    } else {
      NOTHING;
    }
  }

  // if not parsing an external declaration
  if (_pvNextToDeclare==NULL) {
    // error
    _pShell->ErrorF("Only external declarations are supported");
    return;
  }

  // if the symbol is declared already
  if (ssNew.IsDeclared()) {
    // if the declaration is not identical
    if (!ShellTypeIsSame(ssNew.ss_istType, istType) || 
      ((ssNew.ss_ulFlags&SSF_CONSTANT)!=(ulQualifiers&SSF_CONSTANT))) {
      // error
      _pShell->ErrorF("Symbol '%s' is already declared diferrently", (const char *) ssNew.ss_strName);
      return;
    }

    // copy its value
    if (_shell_ast[ssNew.ss_istType].st_sttType==STT_INDEX) {
      *(INDEX*)_pvNextToDeclare = *(INDEX*)ssNew.ss_pvValue;
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_FLOAT) {
      *(FLOAT*)_pvNextToDeclare = *(FLOAT*)ssNew.ss_pvValue;
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_STRING) {
      *(CTString*)_pvNextToDeclare = *(CTString*)ssNew.ss_pvValue;
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_ARRAY) {
      NOTHING;  // array values are not retained
    } else if (_shell_ast[ssNew.ss_istType].st_sttType==STT_FUNCTION) {
      NOTHING;  // function values are not retained
    } else {
      // error
      _pShell->ErrorF("'%s': old value couldn't be retained", (const char *) ssNew.ss_strName);
      return;
    }
  }

  // set the type to given type
  if (!ssNew.IsDeclared()) {
    ssNew.ss_istType = ShellTypeMakeDuplicate(istType);
  }
  // set the value for the external symbol if not already set
  if (ssNew.ss_pvValue==NULL || !(ulQualifiers&SSF_EXTERNAL)) {
    ssNew.ss_pvValue = _pvNextToDeclare;
  }
  // remember qualifiers (if already predeclared - keep old flags)
  ssNew.ss_ulFlags |= ulQualifiers;
  // remember pre and post functions
  if (ssNew.ss_pPreFunc==NULL) {
    ssNew.ss_pPreFunc = (BOOL (*)(void *))pPreFunc;
  }
  if (ssNew.ss_pPostFunc==NULL) {
    ssNew.ss_pPostFunc = (void (*)(void *))pPostFunc;
  }
}

void DoComparison(value &vRes, value &v0, value &v1, int token)
{
  MatchTypes(v0, v1);

  vRes.sttType = STT_INDEX;
  if (v0.sttType == STT_FLOAT) {
    switch (token) {
    case '<': vRes.iIndex = v0.fFloat <v1.fFloat; break;
    case '>': vRes.iIndex = v0.fFloat >v1.fFloat; break;
    case '=': vRes.iIndex = v0.fFloat==v1.fFloat; break;
    case '!': vRes.iIndex = v0.fFloat!=v1.fFloat; break;
    case '}': vRes.iIndex = v0.fFloat>=v1.fFloat; break;
    case '{': vRes.iIndex = v0.fFloat<=v1.fFloat; break;
    default: ASSERT(FALSE);
      vRes.sttType = STT_INDEX;
      vRes.iIndex = 0;
    }
  } else if (v0.sttType == STT_INDEX) {
    switch (token) {
    case '<': vRes.iIndex = v0.iIndex <v1.iIndex; break;
    case '>': vRes.iIndex = v0.iIndex >v1.iIndex; break;
    case '=': vRes.iIndex = v0.iIndex==v1.iIndex; break;
    case '!': vRes.iIndex = v0.iIndex!=v1.iIndex; break;
    case '}': vRes.iIndex = v0.iIndex>=v1.iIndex; break;
    case '{': vRes.iIndex = v0.iIndex<=v1.iIndex; break;
    default: ASSERT(FALSE);
      vRes.sttType = STT_INDEX;
      vRes.iIndex = 0;
    }
  } else if (v0.sttType == STT_STRING) {
    switch (token) {
    case '<': vRes.iIndex = stricmp(v0.strString, v1.strString)  < 0; break;
    case '>': vRes.iIndex = stricmp(v0.strString, v1.strString)  > 0; break;
    case '=': vRes.iIndex = stricmp(v0.strString, v1.strString) == 0; break;
    case '!': vRes.iIndex = stricmp(v0.strString, v1.strString) != 0; break;
    case '}': vRes.iIndex = stricmp(v0.strString, v1.strString) >= 0; break;
    case '{': vRes.iIndex = stricmp(v0.strString, v1.strString) <= 0; break;
    default: ASSERT(FALSE);
      vRes.sttType = STT_INDEX;
      vRes.iIndex = 0;
    }
  } else {
    vRes.sttType = STT_INDEX;
    vRes.iIndex = 0;
  }
}

#line 276 "Engine/Base/Parser.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "Parser.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_c_float = 3,                    /* c_float  */
  YYSYMBOL_c_int = 4,                      /* c_int  */
  YYSYMBOL_c_string = 5,                   /* c_string  */
  YYSYMBOL_c_char = 6,                     /* c_char  */
  YYSYMBOL_identifier = 7,                 /* identifier  */
  YYSYMBOL_k_INDEX = 8,                    /* k_INDEX  */
  YYSYMBOL_k_FLOAT = 9,                    /* k_FLOAT  */
  YYSYMBOL_k_CTString = 10,                /* k_CTString  */
  YYSYMBOL_k_void = 11,                    /* k_void  */
  YYSYMBOL_k_const = 12,                   /* k_const  */
  YYSYMBOL_k_user = 13,                    /* k_user  */
  YYSYMBOL_k_persistent = 14,              /* k_persistent  */
  YYSYMBOL_k_extern = 15,                  /* k_extern  */
  YYSYMBOL_k_pre = 16,                     /* k_pre  */
  YYSYMBOL_k_post = 17,                    /* k_post  */
  YYSYMBOL_k_help = 18,                    /* k_help  */
  YYSYMBOL_k_if = 19,                      /* k_if  */
  YYSYMBOL_k_else = 20,                    /* k_else  */
  YYSYMBOL_k_else_if = 21,                 /* k_else_if  */
  YYSYMBOL_SHL = 22,                       /* SHL  */
  YYSYMBOL_SHR = 23,                       /* SHR  */
  YYSYMBOL_EQ = 24,                        /* EQ  */
  YYSYMBOL_NEQ = 25,                       /* NEQ  */
  YYSYMBOL_GEQ = 26,                       /* GEQ  */
  YYSYMBOL_LEQ = 27,                       /* LEQ  */
  YYSYMBOL_LOGAND = 28,                    /* LOGAND  */
  YYSYMBOL_LOGOR = 29,                     /* LOGOR  */
  YYSYMBOL_block_beg = 30,                 /* block_beg  */
  YYSYMBOL_block_end = 31,                 /* block_end  */
  YYSYMBOL_32_ = 32,                       /* '='  */
  YYSYMBOL_33_ = 33,                       /* '&'  */
  YYSYMBOL_34_ = 34,                       /* '^'  */
  YYSYMBOL_35_ = 35,                       /* '|'  */
  YYSYMBOL_36_ = 36,                       /* '<'  */
  YYSYMBOL_37_ = 37,                       /* '>'  */
  YYSYMBOL_38_ = 38,                       /* '-'  */
  YYSYMBOL_39_ = 39,                       /* '+'  */
  YYSYMBOL_40_ = 40,                       /* '*'  */
  YYSYMBOL_41_ = 41,                       /* '/'  */
  YYSYMBOL_42_ = 42,                       /* '%'  */
  YYSYMBOL_TYPECAST = 43,                  /* TYPECAST  */
  YYSYMBOL_SIGN = 44,                      /* SIGN  */
  YYSYMBOL_45_ = 45,                       /* '!'  */
  YYSYMBOL_46_ = 46,                       /* ':'  */
  YYSYMBOL_47_ = 47,                       /* ','  */
  YYSYMBOL_48_ = 48,                       /* ';'  */
  YYSYMBOL_49_ = 49,                       /* '('  */
  YYSYMBOL_50_ = 50,                       /* ')'  */
  YYSYMBOL_51_ = 51,                       /* '['  */
  YYSYMBOL_52_ = 52,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 53,                  /* $accept  */
  YYSYMBOL_program = 54,                   /* program  */
  YYSYMBOL_block = 55,                     /* block  */
  YYSYMBOL_statements = 56,                /* statements  */
  YYSYMBOL_declaration_qualifiers = 57,    /* declaration_qualifiers  */
  YYSYMBOL_opt_string = 58,                /* opt_string  */
  YYSYMBOL_type_specifier = 59,            /* type_specifier  */
  YYSYMBOL_pre_func_opt = 60,              /* pre_func_opt  */
  YYSYMBOL_post_func_opt = 61,             /* post_func_opt  */
  YYSYMBOL_parameter_list_opt = 62,        /* parameter_list_opt  */
  YYSYMBOL_parameter_list = 63,            /* parameter_list  */
  YYSYMBOL_declaration = 64,               /* declaration  */
  YYSYMBOL_statement = 65,                 /* statement  */
  YYSYMBOL_66_1 = 66,                      /* $@1  */
  YYSYMBOL_67_2 = 67,                      /* $@2  */
  YYSYMBOL_opt_else = 68,                  /* opt_else  */
  YYSYMBOL_69_3 = 69,                      /* $@3  */
  YYSYMBOL_70_4 = 70,                      /* $@4  */
  YYSYMBOL_71_5 = 71,                      /* $@5  */
  YYSYMBOL_lvalue = 72,                    /* lvalue  */
  YYSYMBOL_argument_expression_list_opt = 73, /* argument_expression_list_opt  */
  YYSYMBOL_argument_expression_list = 74,  /* argument_expression_list  */
  YYSYMBOL_expression = 75                 /* expression  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 223 "Engine/Base/Parser.y"

  extern int yylex(YYSTYPE *lvalp);

#line 389 "Engine/Base/Parser.cpp"


#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  35
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   446

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  53
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  23
/* YYNRULES -- Number of rules.  */
#define YYNRULES  82
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  156

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   288


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    45,     2,     2,     2,    42,    33,     2,
      49,    50,    40,    39,    47,    38,     2,    41,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    46,    48,
      36,    32,    37,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    51,     2,    52,    34,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    35,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    43,    44
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   290,   290,   291,   295,   296,   300,   301,   305,   308,
     311,   314,   317,   322,   325,   332,   335,   338,   341,   346,
     349,   362,   365,   378,   382,   388,   396,   407,   411,   420,
     430,   433,   436,   450,   488,   522,   526,   530,   534,   544,
     534,   551,   553,   569,   553,   574,   574,   582,   607,   647,
     652,   658,   663,   670,   674,   678,   682,   700,   713,   727,
     741,   755,   771,   785,   800,   819,   835,   851,   873,   889,
     892,   895,   898,   901,   904,   910,   924,   936,   949,   963,
     977,   994,  1090
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "c_float", "c_int",
  "c_string", "c_char", "identifier", "k_INDEX", "k_FLOAT", "k_CTString",
  "k_void", "k_const", "k_user", "k_persistent", "k_extern", "k_pre",
  "k_post", "k_help", "k_if", "k_else", "k_else_if", "SHL", "SHR", "EQ",
  "NEQ", "GEQ", "LEQ", "LOGAND", "LOGOR", "block_beg", "block_end", "'='",
  "'&'", "'^'", "'|'", "'<'", "'>'", "'-'", "'+'", "'*'", "'/'", "'%'",
  "TYPECAST", "SIGN", "'!'", "':'", "','", "';'", "'('", "')'", "'['",
  "']'", "$accept", "program", "block", "statements",
  "declaration_qualifiers", "opt_string", "type_specifier", "pre_func_opt",
  "post_func_opt", "parameter_list_opt", "parameter_list", "declaration",
  "statement", "$@1", "$@2", "opt_else", "$@3", "$@4", "$@5", "lvalue",
  "argument_expression_list_opt", "argument_expression_list", "expression", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-123)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-7)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      94,  -123,  -123,  -123,   -34,     4,   -29,   116,   124,   124,
     124,  -123,    21,    27,  -123,  -123,   424,  -123,    77,     2,
     281,   124,   124,   -30,   124,     6,   424,  -123,  -123,  -123,
    -123,   -14,     7,    11,   194,  -123,  -123,  -123,  -123,  -123,
    -123,  -123,  -123,  -123,    55,  -123,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,  -123,    13,    18,   362,   152,
      22,    23,   223,  -123,    66,   124,   124,   124,  -123,    42,
     308,   201,   110,   118,   118,   118,   118,   383,   383,   404,
     404,   404,   118,   118,   -32,   -32,  -123,  -123,  -123,  -123,
     124,  -123,  -123,  -123,  -123,    44,  -123,  -123,  -123,    32,
     124,   258,   124,    62,  -123,   362,    53,    78,   335,  -123,
      36,    40,   173,    43,    85,  -123,  -123,  -123,    85,   258,
      84,    96,  -123,    57,    12,    61,  -123,    62,  -123,  -123,
    -123,    65,  -123,  -123,    85,    53,   124,    63,  -123,   252,
    -123,  -123,    53,  -123,    12,  -123
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       8,    53,    54,    55,    47,     0,     0,     8,     0,     0,
       0,    30,     0,     0,    31,     3,     0,     2,     8,    56,
       0,    49,     0,    35,     0,     0,     0,    56,    75,    76,
      77,     0,     0,     0,     0,     1,    16,    15,    17,    18,
       9,    10,    11,    12,     0,     7,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,     0,    50,    51,     0,
       0,     0,     0,     4,     0,     0,     0,     0,    82,    19,
       0,    57,    58,    71,    72,    73,    74,    62,    63,    59,
      61,    60,    69,    70,    65,    64,    66,    67,    68,    81,
       0,    48,    36,    37,    38,     0,    79,    78,    80,     0,
       0,    23,     0,    21,    33,    52,     0,     0,     0,    25,
       0,    24,     0,     0,    13,    39,    20,    34,    13,     0,
      19,     0,    14,     0,    41,     0,    26,    21,    22,    27,
      45,     0,    40,    28,    13,     0,     0,     0,    46,     0,
      29,    42,     0,    43,    41,    44
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -123,  -123,   -81,     0,   117,  -122,   -23,   -20,   -19,  -123,
    -123,  -123,  -123,  -123,  -123,   -24,  -123,  -123,  -123,     5,
    -123,  -123,    -8
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    13,    14,    15,    26,   133,    44,   113,   124,   120,
     121,    17,    18,   116,   134,   142,   152,   154,   145,    27,
      66,    67,    20
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      28,    29,    30,    74,    34,    19,   135,    25,    62,    63,
      64,    23,    19,    68,    69,    21,    72,    22,    45,    70,
      24,    71,   147,    19,     1,     2,     3,    35,     4,    31,
      32,    33,   140,   141,    46,   125,    75,    73,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    76,   109,     8,
       9,    77,    79,    99,   148,   100,    10,   106,   107,   108,
      12,   153,   102,   105,   110,   103,   110,    -6,   117,   123,
       1,     2,     3,     7,     4,   126,   128,   129,   119,   131,
     132,   111,   115,   112,    -6,     5,     6,     1,     2,     3,
     109,     4,   118,   138,   122,   139,   136,     7,    -6,   143,
     137,   150,     5,     6,   146,     8,     9,    16,   144,     1,
       2,     3,    10,     4,     7,    11,    12,     1,     2,     3,
     155,     4,     8,     9,     5,     6,     0,     0,   149,    10,
      47,    48,    11,    12,     0,     0,     7,    -6,    60,    61,
      62,    63,    64,     0,     8,     9,    60,    61,    62,    63,
      64,    10,     8,     9,    11,    12,     0,     0,     0,    10,
       0,     0,     0,    12,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    47,    48,    49,    50,    51,
      52,    53,    54,     0,   101,     0,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    47,    48,    49,    50,
      51,    52,    53,    54,    48,   130,     0,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,     0,     0,    60,
      61,    62,    63,    64,    78,    47,    48,    49,    50,    51,
      52,    53,    54,     0,     0,     0,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    36,    37,    38,    39,
       0,     0,     0,   104,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,     0,     0,     0,     0,     0,
       0,     0,   151,    47,    48,    49,    50,    51,    52,    53,
      54,     0,     0,     0,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,     0,     0,     0,     0,     0,    65,
      47,    48,    49,    50,    51,    52,    53,    54,     0,     0,
       0,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,     0,     0,     0,     0,     0,   114,    47,    48,    49,
      50,    51,    52,    53,    54,     0,     0,     0,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,     0,     0,
       0,     0,     0,   127,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    47,    48,    49,    50,    51,
      52,     0,     0,     0,     0,     0,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    47,    48,    49,    50,
      51,    52,    36,    37,    38,    39,    40,    41,    42,    43,
      58,    59,    60,    61,    62,    63,    64
};

static const yytype_int16 yycheck[] =
{
       8,     9,    10,    26,    12,     0,   128,     7,    40,    41,
      42,     7,     7,    21,    22,    49,    24,    51,    18,    49,
      49,    51,   144,    18,     3,     4,     5,     0,     7,     8,
       9,    10,    20,    21,    32,   116,    50,    31,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    50,    16,    38,
      39,    50,     7,    50,   145,    47,    45,    75,    76,    77,
      49,   152,    50,     7,    32,    52,    32,     0,    46,    17,
       3,     4,     5,    30,     7,     7,    50,    47,   111,    46,
       5,    49,   100,    51,     0,    18,    19,     3,     4,     5,
      16,     7,   110,     7,   112,    48,   129,    30,    31,    48,
     130,    48,    18,    19,    49,    38,    39,     0,   137,     3,
       4,     5,    45,     7,    30,    48,    49,     3,     4,     5,
     154,     7,    38,    39,    18,    19,    -1,    -1,   146,    45,
      22,    23,    48,    49,    -1,    -1,    30,    31,    38,    39,
      40,    41,    42,    -1,    38,    39,    38,    39,    40,    41,
      42,    45,    38,    39,    48,    49,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    49,    22,    23,    24,    25,    26,    27,
      28,    29,    -1,    -1,    -1,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    22,    23,    24,    25,    26,
      27,    28,    29,    -1,    52,    -1,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    22,    23,    24,    25,
      26,    27,    28,    29,    23,    52,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    -1,    -1,    38,
      39,    40,    41,    42,    50,    22,    23,    24,    25,    26,
      27,    28,    29,    -1,    -1,    -1,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,     8,     9,    10,    11,
      -1,    -1,    -1,    50,    22,    23,    24,    25,    26,    27,
      28,    29,    -1,    -1,    -1,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    50,    22,    23,    24,    25,    26,    27,    28,
      29,    -1,    -1,    -1,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    -1,    -1,    -1,    -1,    -1,    48,
      22,    23,    24,    25,    26,    27,    28,    29,    -1,    -1,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    -1,    -1,    -1,    -1,    -1,    48,    22,    23,    24,
      25,    26,    27,    28,    29,    -1,    -1,    -1,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    -1,    -1,
      -1,    -1,    -1,    48,    22,    23,    24,    25,    26,    27,
      28,    29,    -1,    -1,    -1,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    22,    23,    24,    25,    26,
      27,    -1,    -1,    -1,    -1,    -1,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    22,    23,    24,    25,
      26,    27,     8,     9,    10,    11,    12,    13,    14,    15,
      36,    37,    38,    39,    40,    41,    42
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     7,    18,    19,    30,    38,    39,
      45,    48,    49,    54,    55,    56,    57,    64,    65,    72,
      75,    49,    51,     7,    49,    56,    57,    72,    75,    75,
      75,     8,     9,    10,    75,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    59,    56,    32,    22,    23,    24,
      25,    26,    27,    28,    29,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    48,    73,    74,    75,    75,
      49,    51,    75,    31,    59,    50,    50,    50,    50,     7,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    50,
      47,    52,    50,    52,    50,     7,    75,    75,    75,    16,
      32,    49,    51,    60,    48,    75,    66,    46,    75,    59,
      62,    63,    75,    17,    61,    55,     7,    48,    50,    47,
      52,    46,     5,    58,    67,    58,    59,    60,     7,    48,
      20,    21,    68,    48,    61,    71,    49,    58,    55,    75,
      48,    50,    69,    55,    70,    68
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    53,    54,    54,    55,    55,    56,    56,    57,    57,
      57,    57,    57,    58,    58,    59,    59,    59,    59,    60,
      60,    61,    61,    62,    62,    63,    63,    64,    64,    64,
      65,    65,    65,    65,    65,    65,    65,    65,    66,    67,
      65,    68,    69,    70,    68,    71,    68,    72,    72,    73,
      73,    74,    74,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     0,     2,     0,     2,
       2,     2,     2,     0,     1,     1,     1,     1,     1,     0,
       3,     0,     3,     0,     1,     1,     3,     7,     8,    10,
       1,     1,     2,     4,     6,     2,     4,     4,     0,     0,
       8,     0,     0,     0,     8,     0,     3,     1,     4,     0,
       1,     1,     3,     1,     1,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     2,     2,     4,     4,
       4,     4,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 8: /* declaration_qualifiers: %empty  */
#line 305 "Engine/Base/Parser.y"
                {
  (yyval.ulFlags) = 0;
}
#line 1512 "Engine/Base/Parser.cpp"
    break;

  case 9: /* declaration_qualifiers: declaration_qualifiers k_const  */
#line 308 "Engine/Base/Parser.y"
                                 {
  (yyval.ulFlags) = (yyvsp[-1].ulFlags) | SSF_CONSTANT;
}
#line 1520 "Engine/Base/Parser.cpp"
    break;

  case 10: /* declaration_qualifiers: declaration_qualifiers k_user  */
#line 311 "Engine/Base/Parser.y"
                                {
  (yyval.ulFlags) = (yyvsp[-1].ulFlags) | SSF_USER;
}
#line 1528 "Engine/Base/Parser.cpp"
    break;

  case 11: /* declaration_qualifiers: declaration_qualifiers k_persistent  */
#line 314 "Engine/Base/Parser.y"
                                      {
  (yyval.ulFlags) = (yyvsp[-1].ulFlags) | SSF_PERSISTENT;
}
#line 1536 "Engine/Base/Parser.cpp"
    break;

  case 12: /* declaration_qualifiers: declaration_qualifiers k_extern  */
#line 317 "Engine/Base/Parser.y"
                                  {
  (yyval.ulFlags) = (yyvsp[-1].ulFlags) | SSF_EXTERNAL;
}
#line 1544 "Engine/Base/Parser.cpp"
    break;

  case 13: /* opt_string: %empty  */
#line 322 "Engine/Base/Parser.y"
                {
  (yyval.val).strString = "";
}
#line 1552 "Engine/Base/Parser.cpp"
    break;

  case 14: /* opt_string: c_string  */
#line 325 "Engine/Base/Parser.y"
           {
  // !!!! remove this option
  //_pShell->ErrorF("Warning: symbol comments are not supported");
  (yyval.val).strString = (yyvsp[0].val).strString;
}
#line 1562 "Engine/Base/Parser.cpp"
    break;

  case 15: /* type_specifier: k_FLOAT  */
#line 332 "Engine/Base/Parser.y"
          {
  (yyval.istType) = ShellTypeNewFloat();
}
#line 1570 "Engine/Base/Parser.cpp"
    break;

  case 16: /* type_specifier: k_INDEX  */
#line 335 "Engine/Base/Parser.y"
          {
  (yyval.istType) = ShellTypeNewIndex();
}
#line 1578 "Engine/Base/Parser.cpp"
    break;

  case 17: /* type_specifier: k_CTString  */
#line 338 "Engine/Base/Parser.y"
             {
  (yyval.istType) = ShellTypeNewString();
}
#line 1586 "Engine/Base/Parser.cpp"
    break;

  case 18: /* type_specifier: k_void  */
#line 341 "Engine/Base/Parser.y"
         {
  (yyval.istType) = ShellTypeNewVoid();
}
#line 1594 "Engine/Base/Parser.cpp"
    break;

  case 19: /* pre_func_opt: %empty  */
#line 346 "Engine/Base/Parser.y"
  {
  (yyval.pPreFunc) = NULL;
}
#line 1602 "Engine/Base/Parser.cpp"
    break;

  case 20: /* pre_func_opt: k_pre ':' identifier  */
#line 349 "Engine/Base/Parser.y"
                       {
  if (_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_sttType!=STT_FUNCTION
    ||_shell_ast[_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istBaseType].st_sttType!=STT_INDEX
    ||_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istFirstArgument!=_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istLastArgument
    ||_shell_ast[_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istFirstArgument].st_sttType!=STT_INDEX) {
    _pShell->ErrorF("'%s' must return 'INDEX' and take 'INDEX' as input", (const char *) (yyvsp[0].pssSymbol)->ss_strName);
  } else {
    void *pv = (yyvsp[0].pssSymbol)->ss_pvValue;
    (yyval.pPreFunc) = (INDEX(*)(INDEX))(yyvsp[0].pssSymbol)->ss_pvValue;
  }
}
#line 1618 "Engine/Base/Parser.cpp"
    break;

  case 21: /* post_func_opt: %empty  */
#line 362 "Engine/Base/Parser.y"
  {
  (yyval.pPostFunc) = NULL;
}
#line 1626 "Engine/Base/Parser.cpp"
    break;

  case 22: /* post_func_opt: k_post ':' identifier  */
#line 365 "Engine/Base/Parser.y"
                        {
  if (_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_sttType!=STT_FUNCTION
    ||_shell_ast[_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istBaseType].st_sttType!=STT_VOID
    ||_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istFirstArgument!=_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istLastArgument
    ||_shell_ast[_shell_ast[(yyvsp[0].pssSymbol)->ss_istType].st_istFirstArgument].st_sttType!=STT_INDEX) {
    _pShell->ErrorF("'%s' must return 'void' and take 'INDEX' as input", (const char *) (yyvsp[0].pssSymbol)->ss_strName);
  } else {
    (yyval.pPostFunc) = (void(*)(INDEX))(yyvsp[0].pssSymbol)->ss_pvValue;
  }
}
#line 1641 "Engine/Base/Parser.cpp"
    break;

  case 23: /* parameter_list_opt: %empty  */
#line 378 "Engine/Base/Parser.y"
                {
  (yyval.istType) = ShellTypeNewFunction(0);
  ShellTypeAddFunctionArgument((yyval.istType), ShellTypeNewVoid());
}
#line 1650 "Engine/Base/Parser.cpp"
    break;

  case 24: /* parameter_list_opt: parameter_list  */
#line 382 "Engine/Base/Parser.y"
                 {
  (yyval.istType) = (yyvsp[0].istType);
}
#line 1658 "Engine/Base/Parser.cpp"
    break;

  case 25: /* parameter_list: type_specifier  */
#line 388 "Engine/Base/Parser.y"
                 {
  (yyval.istType) = ShellTypeNewFunction(0);
  ShellTypeAddFunctionArgument((yyval.istType), (yyvsp[0].istType));
}
#line 1667 "Engine/Base/Parser.cpp"
    break;

  case 26: /* parameter_list: parameter_list ',' type_specifier  */
#line 396 "Engine/Base/Parser.y"
                                    {
  (yyval.istType) = (yyvsp[-2].istType);
  ShellTypeAddFunctionArgument((yyval.istType), (yyvsp[0].istType));
}
#line 1676 "Engine/Base/Parser.cpp"
    break;

  case 27: /* declaration: declaration_qualifiers type_specifier identifier pre_func_opt post_func_opt opt_string ';'  */
#line 407 "Engine/Base/Parser.y"
                                                                                             {
  Declaration((yyvsp[-6].ulFlags), (yyvsp[-5].istType), *(yyvsp[-4].pssSymbol), (yyvsp[-3].pPreFunc), (yyvsp[-2].pPostFunc));
  ShellTypeDelete((yyvsp[-5].istType));
}
#line 1685 "Engine/Base/Parser.cpp"
    break;

  case 28: /* declaration: declaration_qualifiers type_specifier identifier '(' parameter_list_opt ')' opt_string ';'  */
#line 411 "Engine/Base/Parser.y"
                                                                                             {
  // take function from the parameter list and set its return type
  _shell_ast[(yyvsp[-3].istType)].st_istBaseType = (yyvsp[-6].istType);
  (yyvsp[-6].istType) = (yyvsp[-3].istType);
  // declare it
  Declaration((yyvsp[-7].ulFlags), (yyvsp[-6].istType), *(yyvsp[-5].pssSymbol), NULL, NULL);
  // free the type (declaration will make a copy)
  ShellTypeDelete((yyvsp[-6].istType));
}
#line 1699 "Engine/Base/Parser.cpp"
    break;

  case 29: /* declaration: declaration_qualifiers type_specifier identifier '[' expression ']' pre_func_opt post_func_opt opt_string ';'  */
#line 420 "Engine/Base/Parser.y"
                                                                                                                 {
  if ((yyvsp[-5].val).sttType!=STT_INDEX) {
    _pShell->ErrorF("Array size is not integral");
  }
  (yyvsp[-8].istType) = ShellTypeNewArray((yyvsp[-8].istType), (yyvsp[-5].val).iIndex);
  Declaration((yyvsp[-9].ulFlags), (yyvsp[-8].istType), *(yyvsp[-7].pssSymbol), NULL, NULL);
  ShellTypeDelete((yyvsp[-8].istType));
}
#line 1712 "Engine/Base/Parser.cpp"
    break;

  case 30: /* statement: ';'  */
#line 430 "Engine/Base/Parser.y"
      {
  // dummy
}
#line 1720 "Engine/Base/Parser.cpp"
    break;

  case 31: /* statement: block  */
#line 433 "Engine/Base/Parser.y"
        {
  // dummy
}
#line 1728 "Engine/Base/Parser.cpp"
    break;

  case 32: /* statement: expression ';'  */
#line 436 "Engine/Base/Parser.y"
                 {
  // print its value
  if ((yyvsp[-1].val).sttType == STT_VOID) {
    NOTHING;
  } else if ((yyvsp[-1].val).sttType == STT_FLOAT) {
    CPrintF("%g\n", (yyvsp[-1].val).fFloat);
  } else if ((yyvsp[-1].val).sttType == STT_STRING) {
    CPrintF("\"%s\"\n", (yyvsp[-1].val).strString);
  } else if ((yyvsp[-1].val).sttType == STT_INDEX) {
    CPrintF("%d(0x%08X)\n", (yyvsp[-1].val).iIndex, (yyvsp[-1].val).iIndex);
  } else {
    _pShell->ErrorF("Expression cannot be printed");
  }
}
#line 1747 "Engine/Base/Parser.cpp"
    break;

  case 33: /* statement: lvalue '=' expression ';'  */
#line 450 "Engine/Base/Parser.y"
                            {
  // if it is constant
  if ((yyvsp[-3].lvLValue).lv_pssSymbol->ss_ulFlags&SSF_CONSTANT) {
    _pShell->ErrorF("Symbol '%s' is a constant", (const char *) (yyvsp[-3].lvLValue).lv_pssSymbol->ss_strName);
  // if it is not constant
  } else {
    // if it can be changed
    if ((yyvsp[-3].lvLValue).lv_pssSymbol->ss_pPreFunc==NULL || (yyvsp[-3].lvLValue).lv_pssSymbol->ss_pPreFunc((yyvsp[-3].lvLValue).lv_pvAddress)) {
      // if floats
      if ((yyvsp[-3].lvLValue).lv_sttType == STT_FLOAT && (yyvsp[-1].val).sttType==STT_FLOAT) {
        // assign value
        *(FLOAT*)(yyvsp[-3].lvLValue).lv_pvAddress = (yyvsp[-1].val).fFloat;
      // if indices
      } else if ((yyvsp[-3].lvLValue).lv_sttType == STT_INDEX && (yyvsp[-1].val).sttType==STT_INDEX) {
        // assign value
        *(INDEX*)(yyvsp[-3].lvLValue).lv_pvAddress = (yyvsp[-1].val).iIndex;

      // if strings
      } else if ((yyvsp[-3].lvLValue).lv_sttType == STT_STRING && (yyvsp[-1].val).sttType==STT_STRING) {
        // assign value
        *(CTString*)(yyvsp[-3].lvLValue).lv_pvAddress = (yyvsp[-1].val).strString;

      // if assigning index to float
      } else if ((yyvsp[-3].lvLValue).lv_sttType == STT_FLOAT && (yyvsp[-1].val).sttType==STT_INDEX) {
        *(FLOAT*)(yyvsp[-3].lvLValue).lv_pvAddress = (yyvsp[-1].val).iIndex;
      // otherwise
      } else {
        // error
        _pShell->ErrorF("Cannot assign: different types");
      }

      // call post-change function
      if ((yyvsp[-3].lvLValue).lv_pssSymbol->ss_pPostFunc!=NULL) {
        (yyvsp[-3].lvLValue).lv_pssSymbol->ss_pPostFunc((yyvsp[-3].lvLValue).lv_pvAddress);
      }
    }
  }
}
#line 1790 "Engine/Base/Parser.cpp"
    break;

  case 34: /* statement: declaration_qualifiers type_specifier identifier '=' expression ';'  */
#line 488 "Engine/Base/Parser.y"
                                                                      {
  Declaration((yyvsp[-5].ulFlags), (yyvsp[-4].istType), *(yyvsp[-3].pssSymbol), NULL, NULL);
  ShellTypeDelete((yyvsp[-4].istType));

  CShellSymbol &ssSymbol = *(yyvsp[-3].pssSymbol);
  // if it is constant
  if (ssSymbol.ss_ulFlags&SSF_CONSTANT) {
    // error
    _pShell->ErrorF("Symbol '%s' is a constant", (const char *) ssSymbol.ss_strName);
  }

  // get symbol type
  ShellTypeType stt = _shell_ast[(yyvsp[-4].istType)].st_sttType;

  // if floats
  if (stt == STT_FLOAT && (yyvsp[-1].val).sttType==STT_FLOAT) {
    // assign value
    *(FLOAT*)ssSymbol.ss_pvValue = (yyvsp[-1].val).fFloat;
  // if indices
  } else if (stt == STT_INDEX && (yyvsp[-1].val).sttType==STT_INDEX) {
    // assign value
    *(INDEX*)ssSymbol.ss_pvValue = (yyvsp[-1].val).iIndex;
  // if strings
  } else if (stt == STT_STRING && (yyvsp[-1].val).sttType==STT_STRING) {
    // assign value
    *(CTString*)ssSymbol.ss_pvValue = (yyvsp[-1].val).strString;
  // !!!! remove this conversion
  } else if (stt == STT_FLOAT && (yyvsp[-1].val).sttType==STT_INDEX) {
    _pShell->ErrorF("Warning: assigning INDEX to FLOAT!");  
    *(FLOAT*)ssSymbol.ss_pvValue = (yyvsp[-1].val).iIndex;
  } else {
    _pShell->ErrorF("Symbol '%s' and its initializer have different types", (const char *) ssSymbol.ss_strName);
  }
}
#line 1829 "Engine/Base/Parser.cpp"
    break;

  case 35: /* statement: k_help identifier  */
#line 522 "Engine/Base/Parser.y"
                    { 
extern void PrintShellSymbolHelp(const CTString &strSymbol);
  PrintShellSymbolHelp((yyvsp[0].pssSymbol)->ss_strName);
}
#line 1838 "Engine/Base/Parser.cpp"
    break;

  case 36: /* statement: k_help identifier '(' ')'  */
#line 526 "Engine/Base/Parser.y"
                            { 
extern void PrintShellSymbolHelp(const CTString &strSymbol);
  PrintShellSymbolHelp((yyvsp[-2].pssSymbol)->ss_strName);
}
#line 1847 "Engine/Base/Parser.cpp"
    break;

  case 37: /* statement: k_help identifier '[' ']'  */
#line 530 "Engine/Base/Parser.y"
                            { 
extern void PrintShellSymbolHelp(const CTString &strSymbol);
  PrintShellSymbolHelp((yyvsp[-2].pssSymbol)->ss_strName);
}
#line 1856 "Engine/Base/Parser.cpp"
    break;

  case 38: /* $@1: %empty  */
#line 534 "Engine/Base/Parser.y"
                          { 
  _bExecNextBlock = FALSE;
  if ((yyvsp[-1].val).sttType == STT_INDEX) {
    _bExecNextBlock = (yyvsp[-1].val).iIndex!=0;
  } else if ((yyvsp[-1].val).sttType == STT_FLOAT) {
    _bExecNextBlock = (yyvsp[-1].val).fFloat!=0;
  } else {
    _pShell->ErrorF("If expression is not integral");
  }
  (yyvsp[-3].ulFlags) = _bExecNextBlock;
}
#line 1872 "Engine/Base/Parser.cpp"
    break;

  case 39: /* $@2: %empty  */
#line 544 "Engine/Base/Parser.y"
        {
  _bExecNextElse = !(yyvsp[-5].ulFlags);
  _bExecNextBlock = TRUE;
}
#line 1881 "Engine/Base/Parser.cpp"
    break;

  case 42: /* $@3: %empty  */
#line 553 "Engine/Base/Parser.y"
                               {
  if (_bExecNextElse) {  
    _bExecNextBlock = FALSE;
    if ((yyvsp[-1].val).sttType == STT_INDEX) {
      _bExecNextBlock = (yyvsp[-1].val).iIndex!=0;
    } else if ((yyvsp[-1].val).sttType == STT_FLOAT) {
      _bExecNextBlock = (yyvsp[-1].val).fFloat!=0;
    } else {
      _pShell->ErrorF("If expression is not integral");
    }
    (yyvsp[-3].ulFlags) = _bExecNextBlock;
  } else {
    _bExecNextBlock = FALSE;
    _bExecNextElse = FALSE;
    (yyvsp[-3].ulFlags) = TRUE;
  }
}
#line 1903 "Engine/Base/Parser.cpp"
    break;

  case 43: /* $@4: %empty  */
#line 569 "Engine/Base/Parser.y"
        {
  _bExecNextElse = !(yyvsp[-5].ulFlags);
  _bExecNextBlock = TRUE;
}
#line 1912 "Engine/Base/Parser.cpp"
    break;

  case 45: /* $@5: %empty  */
#line 574 "Engine/Base/Parser.y"
         {
  _bExecNextBlock = _bExecNextElse;  
}
#line 1920 "Engine/Base/Parser.cpp"
    break;

  case 46: /* opt_else: k_else $@5 block  */
#line 576 "Engine/Base/Parser.y"
         {
  _bExecNextBlock = TRUE;
}
#line 1928 "Engine/Base/Parser.cpp"
    break;

  case 47: /* lvalue: identifier  */
#line 582 "Engine/Base/Parser.y"
             {
  CShellSymbol &ssSymbol = *(yyvsp[0].pssSymbol);
  const ShellType &stType = _shell_ast[ssSymbol.ss_istType];

  (yyval.lvLValue).lv_pssSymbol = &ssSymbol;
  if (!ssSymbol.IsDeclared()) {
    // error
    _pShell->ErrorF("Identifier '%s' is not declared", (const char *) (yyvsp[0].pssSymbol)->ss_strName);
    fDummy = -666;
    (yyval.lvLValue).lv_sttType = STT_VOID;
    (yyval.lvLValue).lv_pvAddress = &fDummy;
  // if the identifier is a float, int or string
  } else if (stType.st_sttType==STT_FLOAT || stType.st_sttType==STT_INDEX || stType.st_sttType==STT_STRING) {
    // get its value and type
    (yyval.lvLValue).lv_sttType = stType.st_sttType;
    (yyval.lvLValue).lv_pvAddress = ssSymbol.ss_pvValue;
  // if the identifier is something else
  } else {
    // error
    _pShell->ErrorF("'%s' doesn't have a value", (const char *) (yyvsp[0].pssSymbol)->ss_strName);
    fDummy = -666.0f;
    (yyval.lvLValue).lv_sttType = STT_VOID;
    (yyval.lvLValue).lv_pvAddress = &fDummy;
  }
}
#line 1958 "Engine/Base/Parser.cpp"
    break;

  case 48: /* lvalue: identifier '[' expression ']'  */
#line 607 "Engine/Base/Parser.y"
                                {
  CShellSymbol &ssSymbol = *(yyvsp[-3].pssSymbol);
  const ShellType &stType = _shell_ast[ssSymbol.ss_istType];
  (yyval.lvLValue).lv_pssSymbol = &ssSymbol;

  int iIndex = 0;
  // if subscript is index
  if ((yyvsp[-1].val).sttType==STT_INDEX) {
    // get the index
    iIndex = (yyvsp[-1].val).iIndex;
  // if subscript is not index
  } else {
    // error
    _pShell->ErrorF("Array subscript is not integral");
  }
  // if the symbol is array 
  if (stType.st_sttType==STT_ARRAY) {
    const ShellType &stBase = _shell_ast[stType.st_istBaseType];
    // if it is float or int array
    if (stBase.st_sttType==STT_FLOAT || stBase.st_sttType==STT_INDEX) {
      // if the index is out of range
      if (iIndex<0 || iIndex>=stType.st_ctArraySize) {
        _pShell->ErrorF("Array member out of range");
        fDummy = -666.0f;
        (yyval.lvLValue).lv_pvAddress = &fDummy;
      } else {
        // get its value and type
        (yyval.lvLValue).lv_sttType = stBase.st_sttType;
        (yyval.lvLValue).lv_pvAddress = (FLOAT*)ssSymbol.ss_pvValue+iIndex;
      }
    }
  } else {
    _pShell->ErrorF("'%s[]' doesn't have a value", (const char *) (yyvsp[-3].pssSymbol)->ss_strName);
    fDummy = -666.0f;
    (yyval.lvLValue).lv_pvAddress = &fDummy;
  }
}
#line 2000 "Engine/Base/Parser.cpp"
    break;

  case 49: /* argument_expression_list_opt: %empty  */
#line 647 "Engine/Base/Parser.y"
                {
  (yyval.arg).istType = ShellTypeNewFunction(ShellTypeNewVoid());
  ShellTypeAddFunctionArgument((yyval.arg).istType, ShellTypeNewVoid());
  (yyval.arg).ctBytes = 0;
}
#line 2010 "Engine/Base/Parser.cpp"
    break;

  case 50: /* argument_expression_list_opt: argument_expression_list  */
#line 652 "Engine/Base/Parser.y"
                           {
  (yyval.arg) = (yyvsp[0].arg);
}
#line 2018 "Engine/Base/Parser.cpp"
    break;

  case 51: /* argument_expression_list: expression  */
#line 658 "Engine/Base/Parser.y"
             {
  (yyval.arg).istType = ShellTypeNewFunction(ShellTypeNewVoid());
  ShellTypeAddFunctionArgument((yyval.arg).istType, ShellTypeNewByType((yyvsp[0].val).sttType));
  (yyval.arg).ctBytes = PushExpression((yyvsp[0].val));
}
#line 2028 "Engine/Base/Parser.cpp"
    break;

  case 52: /* argument_expression_list: argument_expression_list ',' expression  */
#line 663 "Engine/Base/Parser.y"
                                          {
  (yyval.arg) = (yyvsp[-2].arg);
  ShellTypeAddFunctionArgument((yyval.arg).istType, ShellTypeNewByType((yyvsp[0].val).sttType));
  (yyval.arg).ctBytes += PushExpression((yyvsp[0].val));
}
#line 2038 "Engine/Base/Parser.cpp"
    break;

  case 53: /* expression: c_float  */
#line 670 "Engine/Base/Parser.y"
          {
  (yyval.val).sttType = STT_FLOAT;  
  (yyval.val).fFloat = (yyvsp[0].val).fFloat;
}
#line 2047 "Engine/Base/Parser.cpp"
    break;

  case 54: /* expression: c_int  */
#line 674 "Engine/Base/Parser.y"
        {
  (yyval.val).sttType = STT_INDEX;  
  (yyval.val).iIndex = (yyvsp[0].val).iIndex;
}
#line 2056 "Engine/Base/Parser.cpp"
    break;

  case 55: /* expression: c_string  */
#line 678 "Engine/Base/Parser.y"
           {
  (yyval.val).sttType = STT_STRING;  
  (yyval.val).strString = (yyvsp[0].val).strString;
}
#line 2065 "Engine/Base/Parser.cpp"
    break;

  case 56: /* expression: lvalue  */
#line 682 "Engine/Base/Parser.y"
         {
  // get its value
  (yyval.val).sttType = (yyvsp[0].lvLValue).lv_sttType;
  if ((yyvsp[0].lvLValue).lv_sttType==STT_VOID) {
    NOTHING;
  } else if ((yyvsp[0].lvLValue).lv_sttType==STT_FLOAT) {
    (yyval.val).fFloat = *(FLOAT*)(yyvsp[0].lvLValue).lv_pvAddress;
  } else if ((yyvsp[0].lvLValue).lv_sttType==STT_INDEX) {
    (yyval.val).iIndex = *(INDEX*)(yyvsp[0].lvLValue).lv_pvAddress;
  } else if ((yyvsp[0].lvLValue).lv_sttType==STT_STRING) {
    (yyval.val).strString = (const char*)*(CTString*)(yyvsp[0].lvLValue).lv_pvAddress;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
    _pShell->ErrorF("'%s' is of wrong type", (const char *) (yyvsp[0].lvLValue).lv_pssSymbol->ss_strName);
  }
}
#line 2087 "Engine/Base/Parser.cpp"
    break;

  case 57: /* expression: expression SHL expression  */
#line 700 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex<<(yyvsp[0].val).iIndex;
  } else {
    _pShell->ErrorF("Wrong arguments for '<<'");
    (yyval.val).sttType = STT_INDEX;
    (yyval.val).iIndex = -666;
  }
}
#line 2105 "Engine/Base/Parser.cpp"
    break;

  case 58: /* expression: expression SHR expression  */
#line 713 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex>>(yyvsp[0].val).iIndex;
  } else {
    _pShell->ErrorF("Wrong arguments for '>>'");
    (yyval.val).sttType = STT_INDEX;
    (yyval.val).iIndex = -666;
  }
}
#line 2123 "Engine/Base/Parser.cpp"
    break;

  case 59: /* expression: expression '&' expression  */
#line 727 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    _pShell->ErrorF("'&' is illegal for FLOAT values");
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex&(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2142 "Engine/Base/Parser.cpp"
    break;

  case 60: /* expression: expression '|' expression  */
#line 741 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    _pShell->ErrorF("'|' is illegal for FLOAT values");
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex|(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2161 "Engine/Base/Parser.cpp"
    break;

  case 61: /* expression: expression '^' expression  */
#line 755 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    _pShell->ErrorF("'^' is illegal for FLOAT values");
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex^(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2180 "Engine/Base/Parser.cpp"
    break;

  case 62: /* expression: expression LOGAND expression  */
#line 771 "Engine/Base/Parser.y"
                               {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    _pShell->ErrorF("'&&' is illegal for FLOAT values");
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex&&(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2199 "Engine/Base/Parser.cpp"
    break;

  case 63: /* expression: expression LOGOR expression  */
#line 785 "Engine/Base/Parser.y"
                              {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    _pShell->ErrorF("'||' is illegal for FLOAT values");
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex||(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2218 "Engine/Base/Parser.cpp"
    break;

  case 64: /* expression: expression '+' expression  */
#line 800 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    (yyval.val).fFloat = (yyvsp[-2].val).fFloat+(yyvsp[0].val).fFloat;
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex+(yyvsp[0].val).iIndex;
  } else if ((yyvsp[-2].val).sttType == STT_STRING) {
    CTString &strNew = _shell_astrTempStrings.Push();
    strNew = CTString((yyvsp[-2].val).strString)+(yyvsp[0].val).strString;
    (yyval.val).strString = (const char*)strNew;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2241 "Engine/Base/Parser.cpp"
    break;

  case 65: /* expression: expression '-' expression  */
#line 819 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    (yyval.val).fFloat = (yyvsp[-2].val).fFloat-(yyvsp[0].val).fFloat;
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex-(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2260 "Engine/Base/Parser.cpp"
    break;

  case 66: /* expression: expression '*' expression  */
#line 835 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    (yyval.val).fFloat = (yyvsp[-2].val).fFloat*(yyvsp[0].val).fFloat;
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex*(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }

}
#line 2280 "Engine/Base/Parser.cpp"
    break;

  case 67: /* expression: expression '/' expression  */
#line 851 "Engine/Base/Parser.y"
                            {

  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    (yyval.val).fFloat = (yyvsp[-2].val).fFloat/(yyvsp[0].val).fFloat;
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    if ((yyvsp[0].val).iIndex==0) {
      _pShell->ErrorF("Division by zero!\n");
      (yyval.val).iIndex = 0;
    } else {
      (yyval.val).iIndex = (yyvsp[-2].val).iIndex/(yyvsp[0].val).iIndex;
    }
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }

}
#line 2305 "Engine/Base/Parser.cpp"
    break;

  case 68: /* expression: expression '%' expression  */
#line 873 "Engine/Base/Parser.y"
                            {
  MatchTypes((yyvsp[-2].val), (yyvsp[0].val));

  (yyval.val).sttType = (yyvsp[-2].val).sttType;
  if ((yyvsp[-2].val).sttType == STT_FLOAT) {
    _pShell->ErrorF("'%' is illegal for FLOAT values");
  } else if ((yyvsp[-2].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[-2].val).iIndex%(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }

}
#line 2324 "Engine/Base/Parser.cpp"
    break;

  case 69: /* expression: expression '<' expression  */
#line 889 "Engine/Base/Parser.y"
                            {
  DoComparison((yyval.val), (yyvsp[-2].val), (yyvsp[0].val), '<');
}
#line 2332 "Engine/Base/Parser.cpp"
    break;

  case 70: /* expression: expression '>' expression  */
#line 892 "Engine/Base/Parser.y"
                            {
  DoComparison((yyval.val), (yyvsp[-2].val), (yyvsp[0].val), '>');
}
#line 2340 "Engine/Base/Parser.cpp"
    break;

  case 71: /* expression: expression EQ expression  */
#line 895 "Engine/Base/Parser.y"
                           {
  DoComparison((yyval.val), (yyvsp[-2].val), (yyvsp[0].val), '=');
}
#line 2348 "Engine/Base/Parser.cpp"
    break;

  case 72: /* expression: expression NEQ expression  */
#line 898 "Engine/Base/Parser.y"
                            {
  DoComparison((yyval.val), (yyvsp[-2].val), (yyvsp[0].val), '!');
}
#line 2356 "Engine/Base/Parser.cpp"
    break;

  case 73: /* expression: expression GEQ expression  */
#line 901 "Engine/Base/Parser.y"
                            {
  DoComparison((yyval.val), (yyvsp[-2].val), (yyvsp[0].val), '}');
}
#line 2364 "Engine/Base/Parser.cpp"
    break;

  case 74: /* expression: expression LEQ expression  */
#line 904 "Engine/Base/Parser.y"
                            {
  DoComparison((yyval.val), (yyvsp[-2].val), (yyvsp[0].val), '{');
}
#line 2372 "Engine/Base/Parser.cpp"
    break;

  case 75: /* expression: '-' expression  */
#line 910 "Engine/Base/Parser.y"
                            {
  (yyval.val).sttType = (yyvsp[0].val).sttType;
  if ((yyvsp[0].val).sttType == STT_FLOAT) {
    (yyval.val).fFloat = -(yyvsp[0].val).fFloat;
  } else if ((yyvsp[0].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = -(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2388 "Engine/Base/Parser.cpp"
    break;

  case 76: /* expression: '+' expression  */
#line 924 "Engine/Base/Parser.y"
                            {
  (yyval.val).sttType = (yyvsp[0].val).sttType;
  if ((yyvsp[0].val).sttType == STT_FLOAT) {
    (yyval.val).fFloat = (yyvsp[0].val).fFloat;
  } else if ((yyvsp[0].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2404 "Engine/Base/Parser.cpp"
    break;

  case 77: /* expression: '!' expression  */
#line 936 "Engine/Base/Parser.y"
                            {
  (yyval.val).sttType = (yyvsp[0].val).sttType;
  if ((yyvsp[0].val).sttType == STT_FLOAT) {
    _pShell->ErrorF("'!' is illegal for FLOAT values");
    (yyval.val).fFloat = (yyvsp[0].val).fFloat;
  } else if ((yyvsp[0].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = !(yyvsp[0].val).iIndex;
  } else {
    (yyval.val).sttType = STT_FLOAT;
    (yyval.val).fFloat = -666.0f;
  }
}
#line 2421 "Engine/Base/Parser.cpp"
    break;

  case 78: /* expression: '(' k_FLOAT ')' expression  */
#line 949 "Engine/Base/Parser.y"
                                            {
  (yyval.val).sttType = STT_FLOAT;
  if ((yyvsp[0].val).sttType == STT_FLOAT) {
    (yyval.val).fFloat = (yyvsp[0].val).fFloat;
  } else if ((yyvsp[0].val).sttType == STT_INDEX) {
    (yyval.val).fFloat = FLOAT((yyvsp[0].val).iIndex);
  } else if ((yyvsp[0].val).sttType == STT_STRING) {
    (yyval.val).fFloat = atof((yyvsp[0].val).strString);
  } else {
    _pShell->ErrorF("Cannot convert to FLOAT");
    (yyval.val).sttType = STT_VOID;
  }
}
#line 2439 "Engine/Base/Parser.cpp"
    break;

  case 79: /* expression: '(' k_INDEX ')' expression  */
#line 963 "Engine/Base/Parser.y"
                                            {
  (yyval.val).sttType = STT_INDEX;
  if ((yyvsp[0].val).sttType == STT_FLOAT) {
    (yyval.val).iIndex = INDEX((yyvsp[0].val).fFloat);
  } else if ((yyvsp[0].val).sttType == STT_INDEX) {
    (yyval.val).iIndex = (yyvsp[0].val).iIndex;
  } else if ((yyvsp[0].val).sttType == STT_STRING) {
    (yyval.val).iIndex = atol((yyvsp[0].val).strString);
  } else {
    _pShell->ErrorF("Cannot convert to INDEX");
    (yyval.val).sttType = STT_VOID;
  }
}
#line 2457 "Engine/Base/Parser.cpp"
    break;

  case 80: /* expression: '(' k_CTString ')' expression  */
#line 977 "Engine/Base/Parser.y"
                                               {
  CTString &strNew = _shell_astrTempStrings.Push();
  (yyval.val).sttType = STT_STRING;
  if ((yyvsp[0].val).sttType == STT_FLOAT) {
    strNew.PrintF("%g", (yyvsp[0].val).fFloat);
  } else if ((yyvsp[0].val).sttType == STT_INDEX) {
    strNew.PrintF("%d", (yyvsp[0].val).iIndex);
  } else if ((yyvsp[0].val).sttType == STT_STRING) {
    strNew = (yyvsp[0].val).strString;
  } else {
    _pShell->ErrorF("Cannot convert to CTString");
    (yyval.val).sttType = STT_VOID;
  }
  (yyval.val).strString = (const char*)strNew;
}
#line 2477 "Engine/Base/Parser.cpp"
    break;

  case 81: /* expression: identifier '(' argument_expression_list_opt ')'  */
#line 994 "Engine/Base/Parser.y"
                                                  {
  // if the identifier is not declared
  if (!(yyvsp[-3].pssSymbol)->IsDeclared()) {
    // error
    _pShell->ErrorF("Identifier '%s' is not declared", (const char *) (yyvsp[-3].pssSymbol)->ss_strName);
  // if the identifier is declared
  } else {
    // get its type
    ShellType &stFunc = _shell_ast[(yyvsp[-3].pssSymbol)->ss_istType];

    // if the identifier is a function
    if (stFunc.st_sttType==STT_FUNCTION) {
      // determine result type
      ShellType &stResult =  _shell_ast[stFunc.st_istBaseType];
      // match argument list result to that result
      _shell_ast[_shell_ast[(yyvsp[-1].arg).istType].st_istBaseType].st_sttType = stResult.st_sttType;
      // if types are same
      if (ShellTypeIsSame((yyvsp[-1].arg).istType, (yyvsp[-3].pssSymbol)->ss_istType)) {
        bool callfunc = true;

// !!! FIXME: maybe just dump the win32 codepath here? This will break on Win64, and maybe break on different compilers/compiler versions, etc.
#ifdef PLATFORM_WIN32
        #define CALLPARAMS
        #define FUNCSIG void
        #define PUSHPARAMS memcpy(_alloca((yyvsp[-1].arg).ctBytes), _ubStack+_iStack-(yyvsp[-1].arg).ctBytes, (yyvsp[-1].arg).ctBytes);
#else
        // This is possibly more portable, but no less scary than the alloca hack.
        #define MAXSCRIPTFUNCARGS 5
        void *ARG[MAXSCRIPTFUNCARGS];
        if (((yyvsp[-1].arg).ctBytes > sizeof (ARG)))
        {
            _pShell->ErrorF("Function '%s' has too many arguments!", (const char *) (yyvsp[-3].pssSymbol)->ss_strName);
            callfunc = false;
        }
        else
        {
            memcpy(ARG, _ubStack+_iStack-(yyvsp[-1].arg).ctBytes, (yyvsp[-1].arg).ctBytes);
            memset(((char *) ARG) + (yyvsp[-1].arg).ctBytes, '\0', sizeof (ARG) - (yyvsp[-1].arg).ctBytes);
        }
        #define PUSHPARAMS
        #define FUNCSIG void*, void*, void*, void*, void*
        #define CALLPARAMS ARG[0], ARG[1], ARG[2], ARG[3], ARG[4]
#endif
  
        if (callfunc) {
          // if void
          if (stResult.st_sttType==STT_VOID) {
            // just call the function
            (yyval.val).sttType = STT_VOID;
            PUSHPARAMS;
            ((void (*)(FUNCSIG))(yyvsp[-3].pssSymbol)->ss_pvValue)(CALLPARAMS);
          // if index
          } else if (stResult.st_sttType==STT_INDEX) {
            // call the function and return result
            (yyval.val).sttType = STT_INDEX;
            PUSHPARAMS;
            (yyval.val).iIndex = ((INDEX (*)(FUNCSIG))(yyvsp[-3].pssSymbol)->ss_pvValue)(CALLPARAMS);
          // if float
          } else if (stResult.st_sttType==STT_FLOAT) {
            // call the function and return result
            (yyval.val).sttType = STT_FLOAT;
            PUSHPARAMS;
            (yyval.val).fFloat = ((FLOAT (*)(FUNCSIG))(yyvsp[-3].pssSymbol)->ss_pvValue)(CALLPARAMS);
          // if string
          } else if (stResult.st_sttType==STT_STRING) {
            // call the function and return result
            (yyval.val).sttType = STT_STRING;
            CTString &strNew = _shell_astrTempStrings.Push();
            PUSHPARAMS;
            strNew = ((CTString (*)(FUNCSIG))(yyvsp[-3].pssSymbol)->ss_pvValue)(CALLPARAMS);
            (yyval.val).strString = (const char*)strNew;
          } else {
            ASSERT(FALSE);
            (yyval.val).sttType = STT_FLOAT;
            (yyval.val).fFloat = -666.0f;
          }
        }
      // if types are different
      } else {
        // error
        (yyval.val).sttType = STT_VOID;
        _pShell->ErrorF("Wrong parameters for '%s'", (const char *) (yyvsp[-3].pssSymbol)->ss_strName);
      }
    // if the identifier is something else
    } else {
      // error
      (yyval.val).sttType = STT_VOID;
      _pShell->ErrorF("Can't call '%s'", (const char *) (yyvsp[-3].pssSymbol)->ss_strName);
    }
  }

  // pop arguments and free type info
  _iStack-=(yyvsp[-1].arg).ctBytes;
  ShellTypeDelete((yyvsp[-1].arg).istType);
}
#line 2577 "Engine/Base/Parser.cpp"
    break;

  case 82: /* expression: '(' expression ')'  */
#line 1090 "Engine/Base/Parser.y"
                     {
  (yyval.val) = (yyvsp[-1].val);
}
#line 2585 "Engine/Base/Parser.cpp"
    break;


#line 2589 "Engine/Base/Parser.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1095 "Engine/Base/Parser.y"

