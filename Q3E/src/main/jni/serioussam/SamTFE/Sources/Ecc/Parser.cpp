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
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "Ecc/Parser.y"

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

// rcg10042001 Changed to specify Ecc directory...
#include "Ecc/StdH.h"
#include "Ecc/Main.h"

// turn off over-helpful bit of bison... --ryan.
#ifdef __GNUC__
#define __attribute__(x)
#endif

#define YYINITDEPTH 1000

static const char *_strCurrentClass;
static int _iCurrentClassID;
static const char *_strCurrentBase;
static const char *_strCurrentDescription;
static const char *_strCurrentThumbnail;
static const char *_strCurrentEnum;
static int _bClassIsExported = 0;

static const char *_strCurrentPropertyID;
static const char *_strCurrentPropertyIdentifier; 
static const char *_strCurrentPropertyPropertyType;
static const char *_strCurrentPropertyEnumType;   
static const char *_strCurrentPropertyDataType;
static const char *_strCurrentPropertyName;       
static const char *_strCurrentPropertyShortcut;
static const char *_strCurrentPropertyColor;
static const char *_strCurrentPropertyFlags;
static const char *_strCurrentPropertyDefaultCode;

static const char *_strCurrentComponentIdentifier;
static const char *_strCurrentComponentType;
static const char *_strCurrentComponentID;     
static const char *_strCurrentComponentFileName;

static int _ctInProcedureHandler = 0;
static char _strLastProcedureName[256];

static char _strInWaitName[256];
static char _strAfterWaitName[256];
static char _strInWaitID[256];
static char _strAfterWaitID[256];

static char _strInLoopName[256];
static char _strAfterLoopName[256];
static char _strInLoopID[256];
static char _strAfterLoopID[256];
static char _strCurrentStateID[256];

static int _bInProcedure;   // set if currently compiling a procedure
static int _bInHandler;
static int _bHasOtherwise;  // set if current 'wait' block has an 'otherwise' statement

static const char *_strCurrentEvent;
static int _bFeature_AbstractBaseClass;
static int _bFeature_ImplementsOnInitClass;
static int _bFeature_ImplementsOnEndClass;
static int _bFeature_ImplementsOnPrecache;
static int _bFeature_ImplementsOnWorldInit;
static int _bFeature_ImplementsOnWorldEnd;
static int _bFeature_ImplementsOnWorldTick;
static int _bFeature_ImplementsOnWorldRender;
static int _bFeature_CanBePredictable;

static int _iNextFreeID;
inline int CreateID(void) {
  return _iNextFreeID++;
}

static int _ctBraces = 0;
void OpenBrace(void) {
  _ctBraces++;
}
void CloseBrace(void) {
  _ctBraces--;
}
SType Braces(int iBraces) {
  static char strBraces[50];
  memset(strBraces, '}', sizeof(strBraces));
  strBraces[iBraces] = 0;
  return SType(strBraces);
}
char *RemoveLineDirective(char *str) 
{
  if (str[0]=='\n' && str[1]=='#') {
    return strchr(str+2, '\n')+1;
  } else {
    return str;
  }
}
const char *GetLineDirective(SType &st)
{
  char *str = st.strString;
  if (str[0]=='\n' && str[1]=='#' && str[2]=='l') {
    char *strResult = strdup(str);
    strchr(strResult+3,'\n')[1] = 0;
    return strResult;
  } else {
    return "";
  }
}
void AddHandlerFunction(char *strProcedureName, int iStateID)
{
  fprintf(_fDeclaration, "  BOOL %s(const CEntityEvent &__eeInput);\n", strProcedureName);
  fprintf(_fTables, " {0x%08x, -1, CEntity::pEventHandler(&%s::%s), "
    "DEBUGSTRING(\"%s::%s\")},\n",
    iStateID, _strCurrentClass, strProcedureName, _strCurrentClass, strProcedureName);
}


void AddHandlerFunction(char *strProcedureName, char *strStateID, char *strBaseStateID)
{
  fprintf(_fDeclaration, "  BOOL %s(const CEntityEvent &__eeInput);\n", strProcedureName);
  fprintf(_fTables, " {%s, %s, CEntity::pEventHandler(&%s::%s),"
    "DEBUGSTRING(\"%s::%s\")},\n",
    strStateID, strBaseStateID, _strCurrentClass, strProcedureName,
    _strCurrentClass, RemoveLineDirective(strProcedureName));
  strcpy(_strLastProcedureName, RemoveLineDirective(strProcedureName));
  _ctInProcedureHandler = 0;
}

void CreateInternalHandlerFunction(char *strFunctionName, char *strID)
{
  int iID = CreateID();
  _ctInProcedureHandler++;
  sprintf(strID, "0x%08x", iID);
  sprintf(strFunctionName, "H0x%08x_%s_%02d", iID, _strLastProcedureName, _ctInProcedureHandler);
  AddHandlerFunction(strFunctionName, iID);
}

void DeclareFeatureProperties(void)
{
  if (_bFeature_CanBePredictable) {
    fprintf(_fTables, " CEntityProperty(CEntityProperty::EPT_ENTITYPTR, NULL, (0x%08x<<8)+%s, _offsetof(%s, %s), %s, %s, %s, %s),\n",
      _iCurrentClassID,
      "255",
      _strCurrentClass,
      "m_penPrediction",
      "\"\"",
      "0",
      "0",
      "0");
    fprintf(_fDeclaration, "  %s %s;\n",
      "CEntityPointer",
      "m_penPrediction"
      );
    fprintf(_fImplementation, "  m_penPrediction = NULL;\n");
  }
}

#define YYERROR_VERBOSE 1


#line 241 "Ecc/Parser.cpp"

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
  YYSYMBOL_c_char = 3,                     /* c_char  */
  YYSYMBOL_c_int = 4,                      /* c_int  */
  YYSYMBOL_c_float = 5,                    /* c_float  */
  YYSYMBOL_c_bool = 6,                     /* c_bool  */
  YYSYMBOL_c_string = 7,                   /* c_string  */
  YYSYMBOL_identifier = 8,                 /* identifier  */
  YYSYMBOL_cppblock = 9,                   /* cppblock  */
  YYSYMBOL_k_while = 10,                   /* k_while  */
  YYSYMBOL_k_for = 11,                     /* k_for  */
  YYSYMBOL_k_if = 12,                      /* k_if  */
  YYSYMBOL_k_else = 13,                    /* k_else  */
  YYSYMBOL_k_enum = 14,                    /* k_enum  */
  YYSYMBOL_k_switch = 15,                  /* k_switch  */
  YYSYMBOL_k_case = 16,                    /* k_case  */
  YYSYMBOL_k_class = 17,                   /* k_class  */
  YYSYMBOL_k_do = 18,                      /* k_do  */
  YYSYMBOL_k_void = 19,                    /* k_void  */
  YYSYMBOL_k_const = 20,                   /* k_const  */
  YYSYMBOL_k_inline = 21,                  /* k_inline  */
  YYSYMBOL_k_static = 22,                  /* k_static  */
  YYSYMBOL_k_virtual = 23,                 /* k_virtual  */
  YYSYMBOL_k_return = 24,                  /* k_return  */
  YYSYMBOL_k_autowait = 25,                /* k_autowait  */
  YYSYMBOL_k_autocall = 26,                /* k_autocall  */
  YYSYMBOL_k_waitevent = 27,               /* k_waitevent  */
  YYSYMBOL_k_event = 28,                   /* k_event  */
  YYSYMBOL_k_name = 29,                    /* k_name  */
  YYSYMBOL_k_thumbnail = 30,               /* k_thumbnail  */
  YYSYMBOL_k_features = 31,                /* k_features  */
  YYSYMBOL_k_uses = 32,                    /* k_uses  */
  YYSYMBOL_k_export = 33,                  /* k_export  */
  YYSYMBOL_k_texture = 34,                 /* k_texture  */
  YYSYMBOL_k_sound = 35,                   /* k_sound  */
  YYSYMBOL_k_model = 36,                   /* k_model  */
  YYSYMBOL_k_properties = 37,              /* k_properties  */
  YYSYMBOL_k_components = 38,              /* k_components  */
  YYSYMBOL_k_functions = 39,               /* k_functions  */
  YYSYMBOL_k_procedures = 40,              /* k_procedures  */
  YYSYMBOL_k_wait = 41,                    /* k_wait  */
  YYSYMBOL_k_on = 42,                      /* k_on  */
  YYSYMBOL_k_otherwise = 43,               /* k_otherwise  */
  YYSYMBOL_k_call = 44,                    /* k_call  */
  YYSYMBOL_k_jump = 45,                    /* k_jump  */
  YYSYMBOL_k_stop = 46,                    /* k_stop  */
  YYSYMBOL_k_resume = 47,                  /* k_resume  */
  YYSYMBOL_k_pass = 48,                    /* k_pass  */
  YYSYMBOL_k_CTString = 49,                /* k_CTString  */
  YYSYMBOL_k_CTStringTrans = 50,           /* k_CTStringTrans  */
  YYSYMBOL_k_CTFileName = 51,              /* k_CTFileName  */
  YYSYMBOL_k_CTFileNameNoDep = 52,         /* k_CTFileNameNoDep  */
  YYSYMBOL_k_BOOL = 53,                    /* k_BOOL  */
  YYSYMBOL_k_COLOR = 54,                   /* k_COLOR  */
  YYSYMBOL_k_FLOAT = 55,                   /* k_FLOAT  */
  YYSYMBOL_k_INDEX = 56,                   /* k_INDEX  */
  YYSYMBOL_k_RANGE = 57,                   /* k_RANGE  */
  YYSYMBOL_k_CEntityPointer = 58,          /* k_CEntityPointer  */
  YYSYMBOL_k_CModelObject = 59,            /* k_CModelObject  */
  YYSYMBOL_k_CModelInstance = 60,          /* k_CModelInstance  */
  YYSYMBOL_k_CAnimObject = 61,             /* k_CAnimObject  */
  YYSYMBOL_k_CSoundObject = 62,            /* k_CSoundObject  */
  YYSYMBOL_k_CPlacement3D = 63,            /* k_CPlacement3D  */
  YYSYMBOL_k_FLOATaabbox3D = 64,           /* k_FLOATaabbox3D  */
  YYSYMBOL_k_FLOATmatrix3D = 65,           /* k_FLOATmatrix3D  */
  YYSYMBOL_k_FLOATquat3D = 66,             /* k_FLOATquat3D  */
  YYSYMBOL_k_ANGLE = 67,                   /* k_ANGLE  */
  YYSYMBOL_k_FLOAT3D = 68,                 /* k_FLOAT3D  */
  YYSYMBOL_k_ANGLE3D = 69,                 /* k_ANGLE3D  */
  YYSYMBOL_k_FLOATplane3D = 70,            /* k_FLOATplane3D  */
  YYSYMBOL_k_ANIMATION = 71,               /* k_ANIMATION  */
  YYSYMBOL_k_ILLUMINATIONTYPE = 72,        /* k_ILLUMINATIONTYPE  */
  YYSYMBOL_k_FLAGS = 73,                   /* k_FLAGS  */
  YYSYMBOL_74_ = 74,                       /* ';'  */
  YYSYMBOL_75_ = 75,                       /* '{'  */
  YYSYMBOL_76_ = 76,                       /* '}'  */
  YYSYMBOL_77_ = 77,                       /* ','  */
  YYSYMBOL_78_ = 78,                       /* ':'  */
  YYSYMBOL_79_ = 79,                       /* '('  */
  YYSYMBOL_80_ = 80,                       /* ')'  */
  YYSYMBOL_81_ = 81,                       /* '='  */
  YYSYMBOL_82_ = 82,                       /* '-'  */
  YYSYMBOL_83_ = 83,                       /* '~'  */
  YYSYMBOL_84_ = 84,                       /* '*'  */
  YYSYMBOL_85_ = 85,                       /* '&'  */
  YYSYMBOL_86_ = 86,                       /* '<'  */
  YYSYMBOL_87_ = 87,                       /* '>'  */
  YYSYMBOL_88_ = 88,                       /* '+'  */
  YYSYMBOL_89_ = 89,                       /* '!'  */
  YYSYMBOL_90_ = 90,                       /* '|'  */
  YYSYMBOL_91_ = 91,                       /* '/'  */
  YYSYMBOL_92_ = 92,                       /* '%'  */
  YYSYMBOL_93_ = 93,                       /* '^'  */
  YYSYMBOL_94_ = 94,                       /* '['  */
  YYSYMBOL_95_ = 95,                       /* ']'  */
  YYSYMBOL_96_ = 96,                       /* '.'  */
  YYSYMBOL_97_ = 97,                       /* '?'  */
  YYSYMBOL_YYACCEPT = 98,                  /* $accept  */
  YYSYMBOL_program = 99,                   /* program  */
  YYSYMBOL_100_1 = 100,                    /* $@1  */
  YYSYMBOL_101_2 = 101,                    /* $@2  */
  YYSYMBOL_102_3 = 102,                    /* $@3  */
  YYSYMBOL_103_4 = 103,                    /* $@4  */
  YYSYMBOL_104_5 = 104,                    /* $@5  */
  YYSYMBOL_opt_global_cppblock = 105,      /* opt_global_cppblock  */
  YYSYMBOL_uses_list = 106,                /* uses_list  */
  YYSYMBOL_uses_statement = 107,           /* uses_statement  */
  YYSYMBOL_enum_and_event_declarations_list = 108, /* enum_and_event_declarations_list  */
  YYSYMBOL_enum_declaration = 109,         /* enum_declaration  */
  YYSYMBOL_110_6 = 110,                    /* $@6  */
  YYSYMBOL_opt_comma = 111,                /* opt_comma  */
  YYSYMBOL_enum_values_list = 112,         /* enum_values_list  */
  YYSYMBOL_enum_value = 113,               /* enum_value  */
  YYSYMBOL_event_declaration = 114,        /* event_declaration  */
  YYSYMBOL_115_7 = 115,                    /* $@7  */
  YYSYMBOL_event_members_list = 116,       /* event_members_list  */
  YYSYMBOL_non_empty_event_members_list = 117, /* non_empty_event_members_list  */
  YYSYMBOL_event_member = 118,             /* event_member  */
  YYSYMBOL_opt_class_declaration = 119,    /* opt_class_declaration  */
  YYSYMBOL_class_declaration = 120,        /* class_declaration  */
  YYSYMBOL_121_8 = 121,                    /* $@8  */
  YYSYMBOL_122_9 = 122,                    /* $@9  */
  YYSYMBOL_123_10 = 123,                   /* $@10  */
  YYSYMBOL_124_11 = 124,                   /* $@11  */
  YYSYMBOL_125_12 = 125,                   /* $@12  */
  YYSYMBOL_126_13 = 126,                   /* $@13  */
  YYSYMBOL_127_14 = 127,                   /* $@14  */
  YYSYMBOL_class_optexport = 128,          /* class_optexport  */
  YYSYMBOL_opt_features = 129,             /* opt_features  */
  YYSYMBOL_130_15 = 130,                   /* $@15  */
  YYSYMBOL_features_list = 131,            /* features_list  */
  YYSYMBOL_feature = 132,                  /* feature  */
  YYSYMBOL_opt_internal_properties = 133,  /* opt_internal_properties  */
  YYSYMBOL_internal_property_list = 134,   /* internal_property_list  */
  YYSYMBOL_internal_property = 135,        /* internal_property  */
  YYSYMBOL_property_declaration_list = 136, /* property_declaration_list  */
  YYSYMBOL_nonempty_property_declaration_list = 137, /* nonempty_property_declaration_list  */
  YYSYMBOL_empty_property_declaration_list = 138, /* empty_property_declaration_list  */
  YYSYMBOL_property_declaration = 139,     /* property_declaration  */
  YYSYMBOL_property_id = 140,              /* property_id  */
  YYSYMBOL_property_identifier = 141,      /* property_identifier  */
  YYSYMBOL_property_type = 142,            /* property_type  */
  YYSYMBOL_property_wed_name_opt = 143,    /* property_wed_name_opt  */
  YYSYMBOL_property_shortcut_opt = 144,    /* property_shortcut_opt  */
  YYSYMBOL_property_color_opt = 145,       /* property_color_opt  */
  YYSYMBOL_property_flags_opt = 146,       /* property_flags_opt  */
  YYSYMBOL_property_default_opt = 147,     /* property_default_opt  */
  YYSYMBOL_property_default_expression = 148, /* property_default_expression  */
  YYSYMBOL_component_declaration_list = 149, /* component_declaration_list  */
  YYSYMBOL_nonempty_component_declaration_list = 150, /* nonempty_component_declaration_list  */
  YYSYMBOL_empty_component_declaration_list = 151, /* empty_component_declaration_list  */
  YYSYMBOL_component_declaration = 152,    /* component_declaration  */
  YYSYMBOL_component_id = 153,             /* component_id  */
  YYSYMBOL_component_identifier = 154,     /* component_identifier  */
  YYSYMBOL_component_filename = 155,       /* component_filename  */
  YYSYMBOL_component_type = 156,           /* component_type  */
  YYSYMBOL_function_list = 157,            /* function_list  */
  YYSYMBOL_function_implementation = 158,  /* function_implementation  */
  YYSYMBOL_opt_tilde = 159,                /* opt_tilde  */
  YYSYMBOL_opt_export = 160,               /* opt_export  */
  YYSYMBOL_opt_const = 161,                /* opt_const  */
  YYSYMBOL_opt_virtual = 162,              /* opt_virtual  */
  YYSYMBOL_opt_semicolon = 163,            /* opt_semicolon  */
  YYSYMBOL_parameters_list = 164,          /* parameters_list  */
  YYSYMBOL_non_void_parameters_list = 165, /* non_void_parameters_list  */
  YYSYMBOL_parameter_declaration = 166,    /* parameter_declaration  */
  YYSYMBOL_return_type = 167,              /* return_type  */
  YYSYMBOL_any_type = 168,                 /* any_type  */
  YYSYMBOL_procedure_list = 169,           /* procedure_list  */
  YYSYMBOL_opt_override = 170,             /* opt_override  */
  YYSYMBOL_procedure_implementation = 171, /* procedure_implementation  */
  YYSYMBOL_172_16 = 172,                   /* $@16  */
  YYSYMBOL_event_specification = 173,      /* event_specification  */
  YYSYMBOL_expression = 174,               /* expression  */
  YYSYMBOL_type_keyword = 175,             /* type_keyword  */
  YYSYMBOL_case_constant_expression = 176, /* case_constant_expression  */
  YYSYMBOL_statements = 177,               /* statements  */
  YYSYMBOL_statement = 178,                /* statement  */
  YYSYMBOL_statement_if = 179,             /* statement_if  */
  YYSYMBOL_statement_if_else = 180,        /* statement_if_else  */
  YYSYMBOL_statement_while = 181,          /* statement_while  */
  YYSYMBOL_182_17 = 182,                   /* $@17  */
  YYSYMBOL_statement_dowhile = 183,        /* statement_dowhile  */
  YYSYMBOL_184_18 = 184,                   /* $@18  */
  YYSYMBOL_statement_for = 185,            /* statement_for  */
  YYSYMBOL_186_19 = 186,                   /* $@19  */
  YYSYMBOL_statement_wait = 187,           /* statement_wait  */
  YYSYMBOL_188_20 = 188,                   /* $@20  */
  YYSYMBOL_statement_autowait = 189,       /* statement_autowait  */
  YYSYMBOL_statement_waitevent = 190,      /* statement_waitevent  */
  YYSYMBOL_opt_eventvar = 191,             /* opt_eventvar  */
  YYSYMBOL_statement_autocall = 192,       /* statement_autocall  */
  YYSYMBOL_wait_expression = 193,          /* wait_expression  */
  YYSYMBOL_statement_jump = 194,           /* statement_jump  */
  YYSYMBOL_statement_call = 195,           /* statement_call  */
  YYSYMBOL_event_expression = 196,         /* event_expression  */
  YYSYMBOL_jumptarget = 197,               /* jumptarget  */
  YYSYMBOL_statement_stop = 198,           /* statement_stop  */
  YYSYMBOL_statement_resume = 199,         /* statement_resume  */
  YYSYMBOL_statement_pass = 200,           /* statement_pass  */
  YYSYMBOL_statement_return = 201,         /* statement_return  */
  YYSYMBOL_opt_expression = 202,           /* opt_expression  */
  YYSYMBOL_handler = 203,                  /* handler  */
  YYSYMBOL_handlers_list = 204             /* handlers_list  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




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
typedef yytype_int16 yy_state_t;

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
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3393

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  98
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  107
/* YYNRULES -- Number of rules.  */
#define YYNRULES  302
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  490

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   328


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
       2,     2,     2,    89,     2,     2,     2,    92,    85,     2,
      79,    80,    84,    88,    77,    82,    96,    91,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    78,    74,
      86,    81,    87,    97,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    94,     2,    95,    93,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    75,    90,    76,    83,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   268,   268,   269,   279,   282,   287,   288,   269,   299,
     300,   304,   305,   308,   317,   318,   319,   325,   325,   336,
     336,   338,   339,   343,   353,   353,   376,   377,   381,   382,
     386,   396,   397,   401,   404,   416,   421,   425,   426,   432,
     435,   402,   501,   502,   506,   507,   507,   520,   521,   524,
     567,   568,   571,   572,   575,   585,   592,   601,   602,   605,
     609,   632,   633,   636,   641,   646,   651,   656,   661,   666,
     671,   676,   681,   686,   691,   696,   701,   706,   711,   716,
     721,   726,   731,   736,   741,   746,   751,   756,   761,   769,
     774,   779,   782,   787,   790,   794,   797,   802,   824,   833,
     833,   833,   833,   833,   834,   835,   836,   837,   838,   845,
     851,   859,   860,   863,   867,   880,   881,   882,   885,   886,
     887,   888,   895,   896,   900,   919,   920,   924,   925,   935,
     936,   939,   940,   943,   944,   947,   948,   949,   952,   953,
     956,   960,   961,   965,   966,   967,   968,   969,   970,   971,
     972,   973,   974,   975,   983,   984,   988,   989,   995,   995,
    1033,  1036,  1039,  1045,  1045,  1045,  1045,  1045,  1046,  1047,
    1048,  1048,  1048,  1048,  1048,  1048,  1048,  1048,  1048,  1048,
    1048,  1048,  1048,  1048,  1048,  1048,  1048,  1048,  1048,  1049,
    1050,  1051,  1052,  1053,  1054,  1055,  1056,  1057,  1058,  1059,
    1060,  1061,  1062,  1063,  1064,  1065,  1066,  1067,  1068,  1069,
    1070,  1071,  1072,  1073,  1074,  1077,  1077,  1077,  1077,  1078,
    1078,  1078,  1078,  1078,  1079,  1079,  1079,  1079,  1079,  1080,
    1080,  1080,  1080,  1080,  1080,  1080,  1081,  1081,  1081,  1082,
    1083,  1086,  1086,  1086,  1086,  1086,  1087,  1094,  1095,  1098,
    1099,  1100,  1101,  1102,  1103,  1104,  1105,  1106,  1107,  1108,
    1109,  1110,  1111,  1112,  1113,  1114,  1115,  1116,  1117,  1118,
    1123,  1139,  1161,  1161,  1186,  1186,  1212,  1212,  1229,  1229,
    1263,  1287,  1312,  1315,  1320,  1344,  1347,  1353,  1362,  1374,
    1377,  1383,  1386,  1392,  1398,  1403,  1408,  1422,  1423,  1427,
    1436,  1446,  1447
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
  "\"end of file\"", "error", "\"invalid token\"", "c_char", "c_int",
  "c_float", "c_bool", "c_string", "identifier", "cppblock", "k_while",
  "k_for", "k_if", "k_else", "k_enum", "k_switch", "k_case", "k_class",
  "k_do", "k_void", "k_const", "k_inline", "k_static", "k_virtual",
  "k_return", "k_autowait", "k_autocall", "k_waitevent", "k_event",
  "k_name", "k_thumbnail", "k_features", "k_uses", "k_export", "k_texture",
  "k_sound", "k_model", "k_properties", "k_components", "k_functions",
  "k_procedures", "k_wait", "k_on", "k_otherwise", "k_call", "k_jump",
  "k_stop", "k_resume", "k_pass", "k_CTString", "k_CTStringTrans",
  "k_CTFileName", "k_CTFileNameNoDep", "k_BOOL", "k_COLOR", "k_FLOAT",
  "k_INDEX", "k_RANGE", "k_CEntityPointer", "k_CModelObject",
  "k_CModelInstance", "k_CAnimObject", "k_CSoundObject", "k_CPlacement3D",
  "k_FLOATaabbox3D", "k_FLOATmatrix3D", "k_FLOATquat3D", "k_ANGLE",
  "k_FLOAT3D", "k_ANGLE3D", "k_FLOATplane3D", "k_ANIMATION",
  "k_ILLUMINATIONTYPE", "k_FLAGS", "';'", "'{'", "'}'", "','", "':'",
  "'('", "')'", "'='", "'-'", "'~'", "'*'", "'&'", "'<'", "'>'", "'+'",
  "'!'", "'|'", "'/'", "'%'", "'^'", "'['", "']'", "'.'", "'?'", "$accept",
  "program", "$@1", "$@2", "$@3", "$@4", "$@5", "opt_global_cppblock",
  "uses_list", "uses_statement", "enum_and_event_declarations_list",
  "enum_declaration", "$@6", "opt_comma", "enum_values_list", "enum_value",
  "event_declaration", "$@7", "event_members_list",
  "non_empty_event_members_list", "event_member", "opt_class_declaration",
  "class_declaration", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13",
  "$@14", "class_optexport", "opt_features", "$@15", "features_list",
  "feature", "opt_internal_properties", "internal_property_list",
  "internal_property", "property_declaration_list",
  "nonempty_property_declaration_list", "empty_property_declaration_list",
  "property_declaration", "property_id", "property_identifier",
  "property_type", "property_wed_name_opt", "property_shortcut_opt",
  "property_color_opt", "property_flags_opt", "property_default_opt",
  "property_default_expression", "component_declaration_list",
  "nonempty_component_declaration_list",
  "empty_component_declaration_list", "component_declaration",
  "component_id", "component_identifier", "component_filename",
  "component_type", "function_list", "function_implementation",
  "opt_tilde", "opt_export", "opt_const", "opt_virtual", "opt_semicolon",
  "parameters_list", "non_void_parameters_list", "parameter_declaration",
  "return_type", "any_type", "procedure_list", "opt_override",
  "procedure_implementation", "$@16", "event_specification", "expression",
  "type_keyword", "case_constant_expression", "statements", "statement",
  "statement_if", "statement_if_else", "statement_while", "$@17",
  "statement_dowhile", "$@18", "statement_for", "$@19", "statement_wait",
  "$@20", "statement_autowait", "statement_waitevent", "opt_eventvar",
  "statement_autocall", "wait_expression", "statement_jump",
  "statement_call", "event_expression", "jumptarget", "statement_stop",
  "statement_resume", "statement_pass", "statement_return",
  "opt_expression", "handler", "handlers_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-359)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-40)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      -3,  -359,     9,    10,  -359,  -359,  -359,  -359,     7,    35,
    -359,  -359,   -23,    -2,  -359,    65,   102,    10,  -359,  -359,
    -359,  -359,  -359,    44,    55,   114,   128,  3207,   100,  -359,
    -359,   126,   127,    60,  -359,    52,   131,  3207,    56,  3207,
    3207,  3207,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -359,  -359,  -359,    68,  -359,  -359,    17,
    -359,  -359,    63,   139,   128,    71,  3207,  -359,    15,  -359,
      15,    15,    15,  3207,    72,  -359,  -359,  -359,   141,  -359,
    -359,    76,    22,  -359,    80,    81,  -359,  -359,  -359,   130,
     148,    86,   132,   154,    89,  -359,   133,  -359,  -359,   158,
     129,  -359,    41,  -359,    91,  -359,   158,   163,  -359,  -359,
    -359,    93,  -359,  -359,  3320,    96,   163,  -359,   164,  -359,
    -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,   165,   166,  -359,  -359,  -359,  -359,  -359,
    -359,   168,  3142,   138,   174,    97,  -359,  -359,    19,   101,
    -359,   134,    26,   149,   107,   178,   108,  -359,  -359,  -359,
    -359,  -359,  -359,   110,  -359,  -359,  3052,    26,  -359,   111,
     112,  -359,  -359,  -359,  -359,   115,  -359,  -359,    18,  3052,
    3052,  -359,  -359,  -359,  -359,  -359,  -359,  -359,   116,  1532,
     117,   -21,  -359,   118,   -22,   -31,   -70,   -45,   119,   -43,
     120,   121,   -53,  -359,  -359,  -359,  -359,  1627,  -359,  -359,
    3052,  3052,   156,   178,  -359,  -359,  -359,  -359,  -359,   185,
    1722,  1817,  -359,  -359,  1912,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,   122,  -359,   123,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -359,  -359,  -359,  3052,  2007,  2102,   135,
    -359,  -359,   189,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,    25,  -359,   167,  -359,   182,   136,  -359,
    3231,  -359,    56,   137,    15,   198,  -359,   201,   142,   140,
    -359,   143,   202,   144,  3296,   203,   146,  -359,    56,   147,
     153,  -359,    29,  -359,   145,   192,  3207,  -359,   207,  -359,
    -359,   157,  -359,   159,   160,  -359,   161,  -359,   297,   209,
     392,   155,   162,   169,   170,    64,   171,  3052,   172,   216,
     172,   172,   216,   216,   173,   176,   179,  -359,  -359,   180,
    1437,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,   180,  3052,
    3052,  3052,  3052,  -359,  -359,  -359,  -359,  -359,  -359,   184,
    -359,  3052,   183,  2197,   190,   187,   177,   228,  -359,   188,
     193,  -359,  -359,  -359,   487,  -359,  -359,  -359,  -359,  -359,
    2292,  2387,  2482,  2577,  -359,   582,  -359,  -359,  2672,  -359,
     195,  3052,   230,   191,  3052,  3052,  -359,   677,  -359,  3052,
     200,   204,  -359,  -359,   232,  3052,   196,  -359,   194,  -359,
     197,   206,  -359,   208,  2767,  -359,  -359,   233,  -359,   234,
    -359,   -27,   210,   213,  -359,  3052,   772,   867,   199,   230,
     211,   212,  -359,  -359,  -359,  -359,   962,  2862,   231,  -359,
    3052,   214,   202,   202,  -359,  -359,  1342,  2957,  -359,   215,
     226,   217,  -359,   219,   218,   236,  -359,  -359,   235,   241,
    1057,  -359,  -359,  -359,  1152,  1247,   180,   180,  -359,  -359
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     3,     0,     9,     1,    10,     4,    11,     5,     0,
      14,    12,     0,     6,    13,     0,     0,     9,    15,    16,
      17,    24,     7,     0,     0,    31,     0,    26,    42,     8,
      32,     0,     0,    19,    21,   144,     0,     0,     0,   239,
       0,   240,   215,   216,   217,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   237,   236,   238,   234,   235,    19,    27,    28,     0,
     143,    43,     0,     0,    20,     0,     0,   145,   152,   148,
     149,   150,   151,    20,     0,    30,   146,   147,     0,    23,
      22,     0,     0,    29,     0,     0,    18,   153,    25,     0,
       0,     0,     0,     0,     0,    34,    44,    45,    35,     0,
       0,    49,     0,    47,     0,    46,     0,    59,    48,    61,
      36,    19,    55,    57,     0,    50,    20,    56,     0,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    85,    84,
      86,    88,    87,     0,     0,    52,    37,    58,    63,    64,
      62,    89,     0,     0,    91,    97,    51,    53,     0,     0,
      92,    93,     0,    95,     0,   113,     0,    90,   102,    99,
     100,   101,   103,   104,   239,   240,     0,     0,    98,     0,
       0,    60,    54,   115,    38,    19,   109,   111,     0,     0,
       0,   166,   163,   164,   165,   167,   168,   185,   184,     0,
     170,   172,   188,   178,   177,   173,   174,   171,   175,   176,
     179,   180,   181,   182,   183,   186,   187,     0,   169,   107,
       0,     0,     0,    20,   110,   121,   119,   120,   118,     0,
       0,     0,   193,   189,     0,   199,   207,   191,   192,   210,
     203,   194,   202,   198,   201,   197,   206,   190,   200,   204,
     195,   208,   209,   205,   196,   108,   214,     0,     0,     0,
     112,   116,     0,    94,   105,   213,   212,   211,   106,    96,
     122,   117,   114,   127,   128,     0,   123,   131,     0,   132,
       0,   154,   142,   125,   141,    40,   126,     0,     0,     0,
     155,     0,   160,     0,   135,   161,     0,    41,   136,     0,
     137,   138,     0,   162,   156,   129,     0,   140,     0,   158,
     130,     0,   139,     0,     0,   247,     0,   247,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   297,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   269,   247,   133,
       0,   248,   257,   258,   254,   255,   256,   259,   260,   261,
     263,   268,   262,   264,   265,   266,   267,   157,   133,     0,
       0,     0,     0,   244,   241,   242,   243,   245,   246,     0,
     247,   298,     0,     0,     0,   291,     0,     0,   278,     0,
       0,   293,   294,   295,     0,   134,   124,   249,   247,   159,
       0,     0,     0,     0,   251,     0,   296,   285,     0,   280,
       0,   290,   282,     0,   290,   290,   252,     0,   272,     0,
       0,     0,   274,   286,     0,   289,     0,   283,     0,   301,
       0,     0,   253,     0,     0,   247,   247,     0,   292,     0,
     281,     0,     0,     0,   247,     0,     0,     0,     0,   282,
       0,     0,   279,   302,   288,   287,     0,     0,   270,   250,
       0,     0,   160,   160,   273,   276,     0,     0,   284,     0,
       0,     0,   271,     0,     0,     0,   247,   275,     0,     0,
       0,   247,   247,   277,     0,     0,   133,   133,   299,   300
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -359,  -359,  -359,  -359,  -359,  -359,  -359,   246,  -359,  -359,
    -359,  -359,  -359,   -59,  -359,   237,  -359,  -359,  -359,  -359,
     175,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -359,   220,  -359,  -359,  -359,  -359,  -359,
    -359,   205,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
      58,  -359,  -359,  -359,    49,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -359,  -359,  -358,  -359,  -359,   -64,  -359,
     -35,  -359,  -359,  -359,  -359,  -346,  -186,   -19,  -359,  -324,
    -177,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,  -359,
    -359,  -359,  -359,  -131,  -359,  -219,  -359,  -359,  -291,  -217,
    -359,  -359,  -359,  -359,  -359,  -359,  -359
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     2,     3,     7,    10,    17,    25,     6,     8,    11,
      13,    18,    23,    75,    33,    34,    19,    24,    66,    67,
      68,    29,    30,   106,   110,   125,   163,   232,   285,   299,
      31,   108,   109,   112,   113,   156,   162,   167,   120,   121,
     122,   123,   124,   161,   154,   165,   171,   177,   191,   173,
     188,   194,   195,   196,   197,   198,   272,   282,   239,   283,
     286,   297,   287,   321,   290,   396,   309,   310,   311,   293,
      69,   295,   319,   300,   324,   306,   266,   228,   379,   328,
     351,   352,   353,   354,   433,   355,   437,   356,   471,   357,
     413,   358,   359,   428,   360,   384,   361,   362,   426,   386,
     363,   364,   365,   366,   382,   453,   441
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     227,     1,    78,   330,    80,    81,    82,    84,    70,     4,
     399,   254,    15,   240,   241,   450,   451,   255,    70,     5,
      70,    70,    70,   244,   394,    85,    16,   174,   263,   178,
     179,   180,   181,   182,   183,   235,   256,   317,   259,     9,
     264,    92,    12,   257,   267,   268,   184,   260,   185,   452,
     252,    14,   236,   237,   238,   253,   405,    70,   284,   250,
     246,   247,   127,   251,    70,   -39,   248,   373,   374,   375,
     376,   377,   378,    20,   417,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    86,
      87,    86,    87,    86,    87,   186,    86,    87,   187,    97,
      21,   446,   447,    86,    87,   115,   469,   470,   116,    26,
     456,   387,   388,   430,   431,   389,   390,   168,   488,   489,
      27,    28,    32,    71,    72,    73,   234,    74,    76,    77,
      79,    88,   350,    70,   350,    83,    89,    91,    94,    95,
      96,   381,   480,   189,    98,   101,    99,   484,   485,   100,
     102,   104,   103,   105,   107,   111,   114,   119,   189,   117,
     126,   155,   158,   159,   160,   164,   169,   170,   172,   175,
     190,   192,   193,   400,   401,   402,   403,   199,   176,   200,
     230,   231,   233,   271,   242,   269,   281,   408,   245,   249,
     258,   261,   262,   276,   277,   289,   298,   288,   350,   301,
     305,   313,   320,   280,   291,   323,   303,   367,   307,   350,
     296,   302,   304,   318,   385,   425,   314,   315,   425,   425,
     316,   350,   325,   434,   369,   327,   412,   326,   427,   329,
     438,   370,   449,   448,   466,   229,   380,   391,   371,   372,
     392,   383,   322,   393,   395,   294,   411,   406,    93,   457,
     350,   350,   404,    22,   409,   410,   429,   414,   440,   312,
     350,    70,   415,   424,   467,   435,   439,   442,   460,   436,
     350,   312,   270,   444,   454,    70,   443,   455,   468,   472,
     462,   463,   476,   477,   350,   474,   478,    70,   350,   350,
     201,   202,   203,   204,   205,   206,   475,   331,   332,   333,
     481,    90,   334,   335,   479,   336,   482,   184,   461,   185,
       0,   337,   338,   339,   340,     0,     0,     0,     0,     0,
       0,   157,     0,     0,     0,     0,   118,     0,   341,     0,
       0,   342,   343,   344,   345,   346,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   347,   348,   349,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,   331,   332,   333,     0,     0,   334,   335,     0,
     336,     0,   184,     0,   185,     0,   337,   338,   339,   340,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   341,     0,     0,   342,   343,   344,   345,
     346,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,   347,   348,   368,   207,
     208,   209,     0,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,   331,   332,   333,
       0,     0,   334,   335,     0,   336,     0,   184,     0,   185,
       0,   337,   338,   339,   340,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   341,     0,
       0,   342,   343,   344,   345,   346,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   347,   348,   416,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,   331,   332,   333,     0,     0,   334,   335,     0,
     336,     0,   184,     0,   185,     0,   337,   338,   339,   340,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   341,     0,     0,   342,   343,   344,   345,
     346,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,   347,   348,   422,   207,
     208,   209,     0,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,   331,   332,   333,
       0,     0,   334,   335,     0,   336,     0,   184,     0,   185,
       0,   337,   338,   339,   340,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   341,     0,
       0,   342,   343,   344,   345,   346,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   347,   348,   432,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,   331,   332,   333,     0,     0,   334,   335,     0,
     336,     0,   184,     0,   185,     0,   337,   338,   339,   340,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   341,     0,     0,   342,   343,   344,   345,
     346,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,   347,   348,   458,   207,
     208,   209,     0,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,   331,   332,   333,
       0,     0,   334,   335,     0,   336,     0,   184,     0,   185,
       0,   337,   338,   339,   340,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   341,     0,
       0,   342,   343,   344,   345,   346,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   347,   348,   459,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,   331,   332,   333,     0,     0,   334,   335,     0,
     336,     0,   184,     0,   185,     0,   337,   338,   339,   340,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   341,     0,     0,   342,   343,   344,   345,
     346,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,   347,   348,   464,   207,
     208,   209,     0,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,   331,   332,   333,
       0,     0,   334,   335,     0,   336,     0,   184,     0,   185,
       0,   337,   338,   339,   340,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   341,     0,
       0,   342,   343,   344,   345,   346,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   347,   348,   483,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,   331,   332,   333,     0,     0,   334,   335,     0,
     336,     0,   184,     0,   185,     0,   337,   338,   339,   340,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   341,     0,     0,   342,   343,   344,   345,
     346,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,   347,   348,   486,   207,
     208,   209,     0,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,   331,   332,   333,
       0,     0,   334,   335,     0,   336,     0,   184,     0,   185,
       0,   337,   338,   339,   340,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   341,     0,
       0,   342,   343,   344,   345,   346,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   347,   348,   487,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,   331,   332,   333,     0,     0,   334,   335,     0,
     336,     0,   184,     0,   185,     0,   337,   338,   339,   340,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   341,     0,     0,   342,   343,   344,   345,
     346,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,   347,   348,     0,   207,
     208,   209,     0,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   397,   398,     0,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   243,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,     0,     0,     0,   207,   208,   209,   265,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   273,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,     0,     0,     0,   207,   208,   209,   274,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   275,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,     0,     0,     0,   207,   208,   209,   278,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   279,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,     0,     0,     0,   207,   208,   209,   407,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   418,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   419,     0,     0,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   420,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,     0,     0,     0,   207,   208,   209,   421,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   423,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,   445,     0,     0,   207,   208,   209,     0,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,   465,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
       0,     0,     0,     0,   207,   208,   209,   473,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   201,   202,   203,   204,   205,
     206,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   184,     0,   185,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,     0,     0,     0,     0,   207,
     208,   209,     0,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
      35,     0,     0,     0,     0,     0,    36,     0,     0,    37,
       0,    38,    39,    40,    41,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    35,     0,     0,   166,     0,
       0,    36,     0,     0,    37,     0,    38,    39,    40,    41,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    35,
       0,     0,     0,     0,     0,    36,     0,     0,    37,     0,
     292,    39,    40,    41,     0,     0,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    35,     0,     0,     0,     0,     0,
      36,     0,     0,    37,     0,   308,    39,    40,    41,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   128,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153
};

static const yytype_int16 yycheck[] =
{
     186,     4,    37,   327,    39,    40,    41,    66,    27,     0,
     368,    81,    14,   199,   200,    42,    43,    87,    37,     9,
      39,    40,    41,   209,   348,     8,    28,     8,    81,     3,
       4,     5,     6,     7,     8,    17,    81,     8,    81,    32,
      93,    76,     7,    88,   230,   231,    20,    90,    22,    76,
      81,    74,    34,    35,    36,    86,   380,    76,    33,    81,
      81,    82,   121,    85,    83,    40,    87,     3,     4,     5,
       6,     7,     8,     8,   398,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    84,
      85,    84,    85,    84,    85,    79,    84,    85,    82,    87,
       8,   435,   436,    84,    85,    74,   462,   463,    77,    75,
     444,   340,   341,   414,   415,   342,   343,   162,   486,   487,
      75,    17,     4,    33,     8,     8,   195,    77,    86,     8,
      84,    78,   328,   162,   330,    77,     7,    76,    76,     8,
      74,   337,   476,   172,    74,     7,    75,   481,   482,    29,
      74,     7,    30,    74,    31,     7,    37,     4,   187,    78,
      77,    75,     8,     8,     8,     7,    38,     3,    81,    78,
      31,    74,     4,   369,   370,   371,   372,    79,    54,    79,
      79,    79,    77,     8,    78,    39,     7,   383,    81,    81,
      81,    81,    81,    81,    81,    23,     8,    40,   394,     8,
       8,     8,    20,    78,    78,     8,    76,     8,    74,   405,
      83,    79,    79,    78,     8,   411,    80,    80,   414,   415,
      77,   417,    75,   419,    79,    75,     8,    78,     8,    78,
       8,    79,     8,    10,    13,   187,    75,    74,    79,    79,
      74,    79,   316,    74,    74,   290,    79,    74,    83,   445,
     446,   447,    78,    17,    74,    78,    75,    79,    74,   304,
     456,   290,    79,    78,   460,    75,    80,    80,    79,    75,
     466,   316,   233,    75,    74,   304,    80,    74,    74,   466,
      79,    79,    75,    74,   480,    80,    78,   316,   484,   485,
       3,     4,     5,     6,     7,     8,    80,    10,    11,    12,
      75,    74,    15,    16,    78,    18,    75,    20,   449,    22,
      -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,   126,    -1,    -1,    -1,    -1,   116,    -1,    41,    -1,
      -1,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    75,    76,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    12,    -1,    -1,    15,    16,    -1,
      18,    -1,    20,    -1,    22,    -1,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    10,    11,    12,
      -1,    -1,    15,    16,    -1,    18,    -1,    20,    -1,    22,
      -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    75,    76,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    12,    -1,    -1,    15,    16,    -1,
      18,    -1,    20,    -1,    22,    -1,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    10,    11,    12,
      -1,    -1,    15,    16,    -1,    18,    -1,    20,    -1,    22,
      -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    75,    76,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    12,    -1,    -1,    15,    16,    -1,
      18,    -1,    20,    -1,    22,    -1,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    10,    11,    12,
      -1,    -1,    15,    16,    -1,    18,    -1,    20,    -1,    22,
      -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    75,    76,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    12,    -1,    -1,    15,    16,    -1,
      18,    -1,    20,    -1,    22,    -1,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    10,    11,    12,
      -1,    -1,    15,    16,    -1,    18,    -1,    20,    -1,    22,
      -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    75,    76,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    12,    -1,    -1,    15,    16,    -1,
      18,    -1,    20,    -1,    22,    -1,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    76,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    10,    11,    12,
      -1,    -1,    15,    16,    -1,    18,    -1,    20,    -1,    22,
      -1,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    75,    76,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    12,    -1,    -1,    15,    16,    -1,
      18,    -1,    20,    -1,    22,    -1,    24,    25,    26,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    74,    75,    -1,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    75,    -1,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    -1,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    -1,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    -1,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    -1,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    -1,    -1,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    -1,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    74,    -1,    -1,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    -1,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       8,    -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,    17,
      -1,    19,    20,    21,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,     8,    -1,    -1,    76,    -1,
      -1,    14,    -1,    -1,    17,    -1,    19,    20,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,
      -1,    -1,    -1,    -1,    -1,    14,    -1,    -1,    17,    -1,
      19,    20,    21,    22,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,     8,    -1,    -1,    -1,    -1,    -1,
      14,    -1,    -1,    17,    -1,    19,    20,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     4,    99,   100,     0,     9,   105,   101,   106,    32,
     102,   107,     7,   108,    74,    14,    28,   103,   109,   114,
       8,     8,   105,   110,   115,   104,    75,    75,    17,   119,
     120,   128,     4,   112,   113,     8,    14,    17,    19,    20,
      21,    22,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,   116,   117,   118,   168,
     175,    33,     8,     8,    77,   111,    86,     8,   168,    84,
     168,   168,   168,    77,   111,     8,    84,    85,    78,     7,
     113,    76,   168,   118,    76,     8,    74,    87,    74,    75,
      29,     7,    74,    30,     7,    74,   121,    31,   129,   130,
     122,     7,   131,   132,    37,    74,    77,    78,   132,     4,
     136,   137,   138,   139,   140,   123,    77,   111,    14,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,   142,    75,   133,   139,     8,     8,
       8,   141,   134,   124,     7,   143,    76,   135,   168,    38,
       3,   144,    81,   147,     8,    78,    54,   145,     3,     4,
       5,     6,     7,     8,    20,    22,    79,    82,   148,   175,
      31,   146,    74,     4,   149,   150,   151,   152,   153,    79,
      79,     3,     4,     5,     6,     7,     8,    77,    78,    79,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,   174,   175,   148,
      79,    79,   125,    77,   111,    17,    34,    35,    36,   156,
     174,   174,    78,    80,   174,    81,    81,    82,    87,    81,
      81,    85,    81,    86,    81,    87,    81,    88,    81,    81,
      90,    81,    81,    81,    93,    80,   174,   174,   174,    39,
     152,     8,   154,    80,    80,    80,    81,    81,    80,    80,
      78,     7,   155,   157,    33,   126,   158,   160,    40,    23,
     162,    78,    19,   167,   168,   169,    83,   159,     8,   127,
     171,     8,    79,    76,    79,     8,   173,    74,    19,   164,
     165,   166,   168,     8,    80,    80,    77,     8,    78,   170,
      20,   161,   166,     8,   172,    75,    78,    75,   177,    78,
     177,    10,    11,    12,    15,    16,    18,    24,    25,    26,
      27,    41,    44,    45,    46,    47,    48,    74,    75,    76,
     174,   178,   179,   180,   181,   183,   185,   187,   189,   190,
     192,   194,   195,   198,   199,   200,   201,     8,    76,    79,
      79,    79,    79,     3,     4,     5,     6,     7,     8,   176,
      75,   174,   202,    79,   193,     8,   197,   193,   193,   197,
     197,    74,    74,    74,   177,    74,   163,    74,    75,   163,
     174,   174,   174,   174,    78,   177,    74,    80,   174,    74,
      78,    79,     8,   188,    79,    79,    76,   177,    80,    74,
      80,    80,    76,    80,    78,   174,   196,     8,   191,    75,
     196,   196,    76,   182,   174,    75,    75,   184,     8,    80,
      74,   204,    80,    80,    75,    74,   177,   177,    10,     8,
      42,    43,    76,   203,    74,    74,   177,   174,    76,    76,
      79,   191,    79,    79,    76,    80,    13,   174,    74,   173,
     173,   186,   178,    80,    80,    80,    75,    74,    78,    78,
     177,    75,    75,    76,   177,   177,    76,    76,   163,   163
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    98,    99,   100,   101,   102,   103,   104,    99,   105,
     105,   106,   106,   107,   108,   108,   108,   110,   109,   111,
     111,   112,   112,   113,   115,   114,   116,   116,   117,   117,
     118,   119,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   120,   128,   128,   129,   130,   129,   131,   131,   132,
     133,   133,   134,   134,   135,   136,   136,   137,   137,   138,
     139,   140,   141,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   143,
     143,   144,   144,   145,   145,   146,   146,   147,   147,   148,
     148,   148,   148,   148,   148,   148,   148,   148,   148,   149,
     149,   150,   150,   151,   152,   153,   154,   155,   156,   156,
     156,   156,   157,   157,   158,   159,   159,   160,   160,   161,
     161,   162,   162,   163,   163,   164,   164,   164,   165,   165,
     166,   167,   167,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   169,   169,   170,   170,   172,   171,
     173,   173,   173,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   176,   176,   176,   176,   176,   176,   177,   177,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     179,   180,   182,   181,   184,   183,   186,   185,   188,   187,
     189,   190,   191,   191,   192,   193,   193,   194,   195,   196,
     196,   197,   197,   198,   199,   200,   201,   202,   202,   203,
     203,   204,   204
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,     0,     0,     0,     0,    11,     0,
       1,     0,     2,     3,     0,     2,     2,     0,     8,     0,
       1,     1,     3,     3,     0,     8,     0,     1,     1,     3,
       2,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,    34,     1,     2,     0,     0,     4,     1,     3,     1,
       0,     3,     0,     2,     3,     1,     2,     1,     3,     0,
       6,     1,     1,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       3,     0,     1,     0,     4,     0,     4,     0,     2,     1,
       1,     1,     1,     1,     1,     4,     4,     2,     3,     1,
       2,     1,     3,     0,     4,     1,     1,     1,     1,     1,
       1,     1,     0,     2,    13,     0,     1,     0,     1,     0,
       1,     0,     1,     0,     1,     0,     1,     1,     1,     3,
       2,     1,     1,     1,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     4,     0,     2,     0,     5,     0,    10,
       0,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     3,     3,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     2,
       7,     3,     3,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       7,     9,     0,     8,     0,    10,     0,    12,     0,     6,
       3,     5,     0,     1,     8,     2,     3,     6,     6,     1,
       0,     1,     4,     2,     2,     2,     3,     0,     1,     9,
       9,     0,     2
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


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
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
      yychar = yylex ();
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
  case 2: /* program: %empty  */
#line 268 "Ecc/Parser.y"
                     {}
#line 2402 "Ecc/Parser.cpp"
    break;

  case 3: /* $@1: %empty  */
#line 269 "Ecc/Parser.y"
          {
    int iID = atoi(yyvsp[0].strString);
    if(iID>32767) {
      yyerror("Maximum allowed id for entity source file is 32767");
    }
    _iCurrentClassID = iID;
    _iNextFreeID = iID<<16;
    fprintf(_fDeclaration, "#ifndef _%s_INCLUDED\n", _strFileNameBaseIdentifier);
    fprintf(_fDeclaration, "#define _%s_INCLUDED 1\n", _strFileNameBaseIdentifier);

  }
#line 2418 "Ecc/Parser.cpp"
    break;

  case 4: /* $@2: %empty  */
#line 279 "Ecc/Parser.y"
                        {

    //fprintf(_fImplementation, "\n#undef DECL_DLL\n#define DECL_DLL _declspec(dllimport)\n");
  }
#line 2427 "Ecc/Parser.cpp"
    break;

  case 5: /* $@3: %empty  */
#line 282 "Ecc/Parser.y"
              {
    //fprintf(_fImplementation, "\n#undef DECL_DLL\n#define DECL_DLL _declspec(dllexport)\n");

    fprintf(_fImplementation, "#include <%s.h>\n", _strFileNameBase);
    fprintf(_fImplementation, "#include <%s_tables.h>\n", _strFileNameBase);
  }
#line 2438 "Ecc/Parser.cpp"
    break;

  case 6: /* $@4: %empty  */
#line 287 "Ecc/Parser.y"
                                     {
  }
#line 2445 "Ecc/Parser.cpp"
    break;

  case 7: /* $@5: %empty  */
#line 288 "Ecc/Parser.y"
                        {
  }
#line 2452 "Ecc/Parser.cpp"
    break;

  case 8: /* program: c_int $@1 opt_global_cppblock $@2 uses_list $@3 enum_and_event_declarations_list $@4 opt_global_cppblock $@5 opt_class_declaration  */
#line 289 "Ecc/Parser.y"
                          {
    fprintf(_fDeclaration, "#endif // _%s_INCLUDED\n", _strFileNameBaseIdentifier);
  }
#line 2460 "Ecc/Parser.cpp"
    break;

  case 10: /* opt_global_cppblock: cppblock  */
#line 300 "Ecc/Parser.y"
             { fprintf(_fImplementation, "%s\n", yyvsp[0].strString); }
#line 2466 "Ecc/Parser.cpp"
    break;

  case 13: /* uses_statement: k_uses c_string ';'  */
#line 308 "Ecc/Parser.y"
                        {
    char *strUsedFileName = strdup(yyvsp[-1].strString);
    strUsedFileName[strlen(strUsedFileName)-1] = 0;
    fprintf(_fDeclaration, "#include <%s.h>\n", strUsedFileName+1);
  }
#line 2476 "Ecc/Parser.cpp"
    break;

  case 17: /* $@6: %empty  */
#line 325 "Ecc/Parser.y"
                      { 
    _strCurrentEnum = yyvsp[0].strString;
    fprintf(_fTables, "EP_ENUMBEG(%s)\n", _strCurrentEnum );
    fprintf(_fDeclaration, "extern DECL_DLL CEntityPropertyEnumType %s_enum;\n", _strCurrentEnum );
    fprintf(_fDeclaration, "enum %s {\n", _strCurrentEnum );
  }
#line 2487 "Ecc/Parser.cpp"
    break;

  case 18: /* enum_declaration: k_enum identifier $@6 '{' enum_values_list opt_comma '}' ';'  */
#line 330 "Ecc/Parser.y"
                                           {
    fprintf(_fTables, "EP_ENUMEND(%s);\n\n", _strCurrentEnum);
    fprintf(_fDeclaration, "};\n");
    fprintf(_fDeclaration, "DECL_DLL inline void ClearToDefault(%s &e) { e = (%s)0; } ;\n", _strCurrentEnum, _strCurrentEnum);
  }
#line 2497 "Ecc/Parser.cpp"
    break;

  case 23: /* enum_value: c_int identifier c_string  */
#line 343 "Ecc/Parser.y"
                              {
    fprintf(_fTables, "  EP_ENUMVALUE(%s, %s),\n", yyvsp[-1].strString, yyvsp[0].strString);
    fprintf(_fDeclaration, "  %s = %s,\n", yyvsp[-1].strString, yyvsp[-2].strString);
  }
#line 2506 "Ecc/Parser.cpp"
    break;

  case 24: /* $@7: %empty  */
#line 353 "Ecc/Parser.y"
                       { 
    _strCurrentEvent = yyvsp[0].strString;
    int iID = CreateID();
    fprintf(_fDeclaration, "#define EVENTCODE_%s 0x%08x\n", _strCurrentEvent, iID);
    fprintf(_fDeclaration, "class DECL_DLL %s : public CEntityEvent {\npublic:\n",
      _strCurrentEvent);
    fprintf(_fDeclaration, "%s();\n", _strCurrentEvent );
    fprintf(_fDeclaration, "CEntityEvent *MakeCopy(void);\n");
    fprintf(_fImplementation, 
      "CEntityEvent *%s::MakeCopy(void) { "
      "CEntityEvent *peeCopy = new %s(*this); "
      "return peeCopy;}\n",
      _strCurrentEvent, _strCurrentEvent);
    fprintf(_fImplementation, "%s::%s() : CEntityEvent(EVENTCODE_%s) {;\n",
      _strCurrentEvent, _strCurrentEvent, _strCurrentEvent);
  }
#line 2527 "Ecc/Parser.cpp"
    break;

  case 25: /* event_declaration: k_event identifier $@7 '{' event_members_list opt_comma '}' ';'  */
#line 368 "Ecc/Parser.y"
                                             {
    fprintf(_fImplementation, "};\n");
    fprintf(_fDeclaration, "};\n");
    fprintf(_fDeclaration, "DECL_DLL inline void ClearToDefault(%s &e) { e = %s(); } ;\n", _strCurrentEvent, _strCurrentEvent);
  }
#line 2537 "Ecc/Parser.cpp"
    break;

  case 30: /* event_member: any_type identifier  */
#line 386 "Ecc/Parser.y"
                        {
    fprintf(_fDeclaration, "%s %s;\n", yyvsp[-1].strString, yyvsp[0].strString);
    fprintf(_fImplementation, " ClearToDefault(%s);\n", yyvsp[0].strString);
  }
#line 2546 "Ecc/Parser.cpp"
    break;

  case 34: /* $@8: %empty  */
#line 404 "Ecc/Parser.y"
                           {
    _strCurrentClass = yyvsp[-9].strString;
    _strCurrentBase = yyvsp[-7].strString;
    _strCurrentDescription = yyvsp[-4].strString;
    _strCurrentThumbnail = yyvsp[-1].strString;

    fprintf(_fTables, "#define ENTITYCLASS %s\n\n", _strCurrentClass);
    fprintf(_fDeclaration, "extern \"C\" DECL_DLL CDLLEntityClass %s_DLLClass;\n",
      _strCurrentClass);
    fprintf(_fDeclaration, "%s %s : public %s {\npublic:\n",
      yyvsp[-10].strString, _strCurrentClass, _strCurrentBase);

  }
#line 2564 "Ecc/Parser.cpp"
    break;

  case 35: /* $@9: %empty  */
#line 416 "Ecc/Parser.y"
                 {
    fprintf(_fDeclaration, "  %s virtual void SetDefaultProperties(void);\n", _bClassIsExported?"":"DECL_DLL");
    fprintf(_fImplementation, "void %s::SetDefaultProperties(void) {\n", _strCurrentClass);
    fprintf(_fTables, "CEntityProperty %s_properties[] = {\n", _strCurrentClass);

  }
#line 2575 "Ecc/Parser.cpp"
    break;

  case 36: /* $@10: %empty  */
#line 421 "Ecc/Parser.y"
                                               {
    fprintf(_fImplementation, "  %s::SetDefaultProperties();\n}\n", _strCurrentBase);

    fprintf(_fTables, "CEntityComponent %s_components[] = {\n", _strCurrentClass);
  }
#line 2585 "Ecc/Parser.cpp"
    break;

  case 37: /* $@11: %empty  */
#line 425 "Ecc/Parser.y"
                            {
  }
#line 2592 "Ecc/Parser.cpp"
    break;

  case 38: /* $@12: %empty  */
#line 426 "Ecc/Parser.y"
                                                {
    _bTrackLineInformation = 1;
    fprintf(_fTables, "CEventHandlerEntry %s_handlers[] = {\n", _strCurrentClass);

    _bInProcedure = 0;
    _bInHandler = 0;
  }
#line 2604 "Ecc/Parser.cpp"
    break;

  case 39: /* $@13: %empty  */
#line 432 "Ecc/Parser.y"
                                  {

    _bInProcedure = 1;
  }
#line 2613 "Ecc/Parser.cpp"
    break;

  case 40: /* $@14: %empty  */
#line 435 "Ecc/Parser.y"
                                    {
  }
#line 2620 "Ecc/Parser.cpp"
    break;

  case 41: /* class_declaration: class_optexport identifier ':' identifier '{' k_name c_string ';' k_thumbnail c_string ';' $@8 opt_features $@9 k_properties ':' property_declaration_list $@10 opt_internal_properties $@11 k_components ':' component_declaration_list $@12 k_functions ':' function_list $@13 k_procedures ':' procedure_list $@14 '}' ';'  */
#line 436 "Ecc/Parser.y"
            {
    fprintf(_fTables, "};\n#define %s_handlersct ARRAYCOUNT(%s_handlers)\n", 
      _strCurrentClass, _strCurrentClass);
    fprintf(_fTables, "\n");

    if (_bFeature_AbstractBaseClass) {
      fprintf(_fTables, "CEntity *%s_New(void) { return NULL; };\n",
        _strCurrentClass);
    } else {
      fprintf(_fTables, "CEntity *%s_New(void) { return new %s; };\n",
        _strCurrentClass, _strCurrentClass);
    }

    if (!_bFeature_ImplementsOnInitClass) {
      fprintf(_fTables, "void %s_OnInitClass(void) {};\n", _strCurrentClass);
    } else {
      fprintf(_fTables, "void %s_OnInitClass(void);\n", _strCurrentClass);
    }

    if (!_bFeature_ImplementsOnEndClass) {
      fprintf(_fTables, "void %s_OnEndClass(void) {};\n", _strCurrentClass);
    } else {
      fprintf(_fTables, "void %s_OnEndClass(void);\n", _strCurrentClass);
    }

    if (!_bFeature_ImplementsOnPrecache) {
      fprintf(_fTables, "void %s_OnPrecache(CDLLEntityClass *pdec, INDEX iUser) {};\n", _strCurrentClass);
    } else {
      fprintf(_fTables, "void %s_OnPrecache(CDLLEntityClass *pdec, INDEX iUser);\n", _strCurrentClass);
    }

    if (!_bFeature_ImplementsOnWorldEnd) {
      fprintf(_fTables, "void %s_OnWorldEnd(CWorld *pwo) {};\n", _strCurrentClass);
    } else {
      fprintf(_fTables, "void %s_OnWorldEnd(CWorld *pwo);\n", _strCurrentClass);
    }

    if (!_bFeature_ImplementsOnWorldInit) {
      fprintf(_fTables, "void %s_OnWorldInit(CWorld *pwo) {};\n", _strCurrentClass);
    } else {
      fprintf(_fTables, "void %s_OnWorldInit(CWorld *pwo);\n", _strCurrentClass);
    }

    if (!_bFeature_ImplementsOnWorldTick) {
      fprintf(_fTables, "void %s_OnWorldTick(CWorld *pwo) {};\n", _strCurrentClass);
    } else {
      fprintf(_fTables, "void %s_OnWorldTick(CWorld *pwo);\n", _strCurrentClass);
    }

    if (!_bFeature_ImplementsOnWorldRender) {
      fprintf(_fTables, "void %s_OnWorldRender(CWorld *pwo) {};\n", _strCurrentClass);
    } else {
      fprintf(_fTables, "void %s_OnWorldRender(CWorld *pwo);\n", _strCurrentClass);
    }

    fprintf(_fTables, "ENTITY_CLASSDEFINITION(%s, %s, %s, %s, 0x%08x);\n",
      _strCurrentClass, _strCurrentBase, 
      _strCurrentDescription, _strCurrentThumbnail, _iCurrentClassID);
    fprintf(_fTables, "DECLARE_CTFILENAME(_fnm%s_tbn, %s);\n", _strCurrentClass, _strCurrentThumbnail);

    fprintf(_fDeclaration, "};\n");
  }
#line 2687 "Ecc/Parser.cpp"
    break;

  case 42: /* class_optexport: k_class  */
#line 501 "Ecc/Parser.y"
            { yyval = yyvsp[0]; _bClassIsExported = 0; }
#line 2693 "Ecc/Parser.cpp"
    break;

  case 43: /* class_optexport: k_class k_export  */
#line 502 "Ecc/Parser.y"
                     { yyval = yyvsp[-1]+" DECL_DLL "; _bClassIsExported = 1; }
#line 2699 "Ecc/Parser.cpp"
    break;

  case 45: /* $@15: %empty  */
#line 507 "Ecc/Parser.y"
               {
    _bFeature_ImplementsOnWorldInit = 0;
    _bFeature_ImplementsOnWorldEnd = 0;
    _bFeature_ImplementsOnWorldTick = 0;
    _bFeature_ImplementsOnWorldRender = 0;
    _bFeature_ImplementsOnInitClass = 0;
    _bFeature_ImplementsOnEndClass = 0;
    _bFeature_ImplementsOnPrecache = 0;
    _bFeature_AbstractBaseClass = 0;
    _bFeature_CanBePredictable = 0;
  }
#line 2715 "Ecc/Parser.cpp"
    break;

  case 49: /* feature: c_string  */
#line 524 "Ecc/Parser.y"
             {
    if (strcmp(yyvsp[0].strString, "\"AbstractBaseClass\"")==0) {
      _bFeature_AbstractBaseClass = 1;
    } else if (strcmp(yyvsp[0].strString, "\"IsTargetable\"")==0) {
      fprintf(_fDeclaration, "virtual BOOL IsTargetable(void) const { return TRUE; };\n");
    } else if (strcmp(yyvsp[0].strString, "\"IsImportant\"")==0) {
      fprintf(_fDeclaration, "virtual BOOL IsImportant(void) const { return TRUE; };\n");
    } else if (strcmp(yyvsp[0].strString, "\"HasName\"")==0) {
      fprintf(_fDeclaration, 
        "virtual const CTString &GetName(void) const { return m_strName; };\n");
    } else if (strcmp(yyvsp[0].strString, "\"CanBePredictable\"")==0) {
      fprintf(_fDeclaration, 
        "virtual CEntity *GetPredictionPair(void) { return m_penPrediction; };\n");
      fprintf(_fDeclaration, 
        "virtual void SetPredictionPair(CEntity *penPair) { m_penPrediction = penPair; };\n");
      _bFeature_CanBePredictable = 1;
    } else if (strcmp(yyvsp[0].strString, "\"HasDescription\"")==0) {
      fprintf(_fDeclaration, 
        "virtual const CTString &GetDescription(void) const { return m_strDescription; };\n");
    } else if (strcmp(yyvsp[0].strString, "\"HasTarget\"")==0) {
      fprintf(_fDeclaration, 
        "virtual CEntity *GetTarget(void) const { return m_penTarget; };\n");
    } else if (strcmp(yyvsp[0].strString, "\"ImplementsOnInitClass\"")==0) {
      _bFeature_ImplementsOnInitClass = 1;
    } else if (strcmp(yyvsp[0].strString, "\"ImplementsOnEndClass\"")==0) {
      _bFeature_ImplementsOnEndClass = 1;
    } else if (strcmp(yyvsp[0].strString, "\"ImplementsOnPrecache\"")==0) {
      _bFeature_ImplementsOnPrecache = 1;
    } else if (strcmp(yyvsp[0].strString, "\"ImplementsOnWorldInit\"")==0) {
      _bFeature_ImplementsOnWorldInit = 1;
    } else if (strcmp(yyvsp[0].strString, "\"ImplementsOnWorldEnd\"")==0) {
      _bFeature_ImplementsOnWorldEnd = 1;
    } else if (strcmp(yyvsp[0].strString, "\"ImplementsOnWorldTick\"")==0) {
      _bFeature_ImplementsOnWorldTick = 1;
    } else if (strcmp(yyvsp[0].strString, "\"ImplementsOnWorldRender\"")==0) {
      _bFeature_ImplementsOnWorldRender = 1;
    } else {
      yyerror((SType("Unknown feature: ")+yyvsp[0]).strString);
    }
  }
#line 2760 "Ecc/Parser.cpp"
    break;

  case 54: /* internal_property: any_type identifier ';'  */
#line 575 "Ecc/Parser.y"
                            { 
    fprintf(_fDeclaration, "%s %s;\n", yyvsp[-2].strString, yyvsp[-1].strString);
  }
#line 2768 "Ecc/Parser.cpp"
    break;

  case 55: /* property_declaration_list: empty_property_declaration_list  */
#line 585 "Ecc/Parser.y"
                                    {
    DeclareFeatureProperties(); // this won't work, but at least it will generate an error!!!!
    fprintf(_fTables, "  CEntityProperty()\n};\n");
    fprintf(_fTables, "#define %s_propertiesct 0\n", _strCurrentClass);
    fprintf(_fTables, "\n");
    fprintf(_fTables, "\n");
  }
#line 2780 "Ecc/Parser.cpp"
    break;

  case 56: /* property_declaration_list: nonempty_property_declaration_list opt_comma  */
#line 592 "Ecc/Parser.y"
                                                 {
    DeclareFeatureProperties();
    fprintf(_fTables, "};\n");
    fprintf(_fTables, "#define %s_propertiesct ARRAYCOUNT(%s_properties)\n", 
      _strCurrentClass, _strCurrentClass);
    fprintf(_fTables, "\n");
  }
#line 2792 "Ecc/Parser.cpp"
    break;

  case 60: /* property_declaration: property_id property_type property_identifier property_wed_name_opt property_default_opt property_flags_opt  */
#line 609 "Ecc/Parser.y"
                                                                                                                {
    fprintf(_fTables, " CEntityProperty(%s, %s, (0x%08x<<8)+%s, _offsetof(%s, %s), %s, %s, %s, %s),\n",
      _strCurrentPropertyPropertyType,
      _strCurrentPropertyEnumType,
      _iCurrentClassID,
      _strCurrentPropertyID,
      _strCurrentClass,
      _strCurrentPropertyIdentifier,
      _strCurrentPropertyName,
      _strCurrentPropertyShortcut,
      _strCurrentPropertyColor,
      _strCurrentPropertyFlags);
    fprintf(_fDeclaration, "  %s %s;\n",
      _strCurrentPropertyDataType,
      _strCurrentPropertyIdentifier
      );

    if (strlen(_strCurrentPropertyDefaultCode)>0) {
      fprintf(_fImplementation, "  %s\n", _strCurrentPropertyDefaultCode);
    }
  }
#line 2818 "Ecc/Parser.cpp"
    break;

  case 61: /* property_id: c_int  */
#line 632 "Ecc/Parser.y"
                    { _strCurrentPropertyID = yyvsp[0].strString; }
#line 2824 "Ecc/Parser.cpp"
    break;

  case 62: /* property_identifier: identifier  */
#line 633 "Ecc/Parser.y"
                                 { _strCurrentPropertyIdentifier = yyvsp[0].strString; }
#line 2830 "Ecc/Parser.cpp"
    break;

  case 63: /* property_type: k_enum identifier  */
#line 636 "Ecc/Parser.y"
                      {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_ENUM"; 
    _strCurrentPropertyEnumType = (SType("&")+yyvsp[0]+"_enum").strString; 
    _strCurrentPropertyDataType = (SType("enum ")+yyvsp[0].strString).strString;
  }
#line 2840 "Ecc/Parser.cpp"
    break;

  case 64: /* property_type: k_FLAGS identifier  */
#line 641 "Ecc/Parser.y"
                       {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FLAGS"; 
    _strCurrentPropertyEnumType = (SType("&")+yyvsp[0]+"_enum").strString; 
    _strCurrentPropertyDataType = "ULONG";
  }
#line 2850 "Ecc/Parser.cpp"
    break;

  case 65: /* property_type: k_CTString  */
#line 646 "Ecc/Parser.y"
               {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_STRING"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CTString";
  }
#line 2860 "Ecc/Parser.cpp"
    break;

  case 66: /* property_type: k_CTStringTrans  */
#line 651 "Ecc/Parser.y"
                    {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_STRINGTRANS"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CTStringTrans";
  }
#line 2870 "Ecc/Parser.cpp"
    break;

  case 67: /* property_type: k_CTFileName  */
#line 656 "Ecc/Parser.y"
                 {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FILENAME"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CTFileName";
  }
#line 2880 "Ecc/Parser.cpp"
    break;

  case 68: /* property_type: k_CTFileNameNoDep  */
#line 661 "Ecc/Parser.y"
                      {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FILENAMENODEP"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CTFileNameNoDep";
  }
#line 2890 "Ecc/Parser.cpp"
    break;

  case 69: /* property_type: k_BOOL  */
#line 666 "Ecc/Parser.y"
           {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_BOOL"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "BOOL";
  }
#line 2900 "Ecc/Parser.cpp"
    break;

  case 70: /* property_type: k_COLOR  */
#line 671 "Ecc/Parser.y"
            {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_COLOR"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "COLOR";
  }
#line 2910 "Ecc/Parser.cpp"
    break;

  case 71: /* property_type: k_FLOAT  */
#line 676 "Ecc/Parser.y"
            {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FLOAT"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "FLOAT";
  }
#line 2920 "Ecc/Parser.cpp"
    break;

  case 72: /* property_type: k_INDEX  */
#line 681 "Ecc/Parser.y"
            {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_INDEX"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "INDEX";
  }
#line 2930 "Ecc/Parser.cpp"
    break;

  case 73: /* property_type: k_RANGE  */
#line 686 "Ecc/Parser.y"
            {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_RANGE"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "RANGE";
  }
#line 2940 "Ecc/Parser.cpp"
    break;

  case 74: /* property_type: k_CEntityPointer  */
#line 691 "Ecc/Parser.y"
                     {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_ENTITYPTR"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CEntityPointer";
  }
#line 2950 "Ecc/Parser.cpp"
    break;

  case 75: /* property_type: k_CModelObject  */
#line 696 "Ecc/Parser.y"
                   {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_MODELOBJECT"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CModelObject";
  }
#line 2960 "Ecc/Parser.cpp"
    break;

  case 76: /* property_type: k_CModelInstance  */
#line 701 "Ecc/Parser.y"
                     {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_MODELINSTANCE"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CModelInstance";
  }
#line 2970 "Ecc/Parser.cpp"
    break;

  case 77: /* property_type: k_CAnimObject  */
#line 706 "Ecc/Parser.y"
                  {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_ANIMOBJECT"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CAnimObject";
  }
#line 2980 "Ecc/Parser.cpp"
    break;

  case 78: /* property_type: k_CSoundObject  */
#line 711 "Ecc/Parser.y"
                   {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_SOUNDOBJECT"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CSoundObject";
  }
#line 2990 "Ecc/Parser.cpp"
    break;

  case 79: /* property_type: k_CPlacement3D  */
#line 716 "Ecc/Parser.y"
                   {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_PLACEMENT3D"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "CPlacement3D";
  }
#line 3000 "Ecc/Parser.cpp"
    break;

  case 80: /* property_type: k_FLOATaabbox3D  */
#line 721 "Ecc/Parser.y"
                    {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FLOATAABBOX3D"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "FLOATaabbox3D";
  }
#line 3010 "Ecc/Parser.cpp"
    break;

  case 81: /* property_type: k_FLOATmatrix3D  */
#line 726 "Ecc/Parser.y"
                    {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FLOATMATRIX3D"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "FLOATmatrix3D";
  }
#line 3020 "Ecc/Parser.cpp"
    break;

  case 82: /* property_type: k_FLOATquat3D  */
#line 731 "Ecc/Parser.y"
                  {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FLOATQUAT3D"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "FLOATquat3D";
  }
#line 3030 "Ecc/Parser.cpp"
    break;

  case 83: /* property_type: k_ANGLE  */
#line 736 "Ecc/Parser.y"
            {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_ANGLE"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "ANGLE";
  }
#line 3040 "Ecc/Parser.cpp"
    break;

  case 84: /* property_type: k_ANGLE3D  */
#line 741 "Ecc/Parser.y"
              {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_ANGLE3D"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "ANGLE3D";
  }
#line 3050 "Ecc/Parser.cpp"
    break;

  case 85: /* property_type: k_FLOAT3D  */
#line 746 "Ecc/Parser.y"
              {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FLOAT3D"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "FLOAT3D";
  }
#line 3060 "Ecc/Parser.cpp"
    break;

  case 86: /* property_type: k_FLOATplane3D  */
#line 751 "Ecc/Parser.y"
                   {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_FLOATplane3D"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "FLOATplane3D";
  }
#line 3070 "Ecc/Parser.cpp"
    break;

  case 87: /* property_type: k_ILLUMINATIONTYPE  */
#line 756 "Ecc/Parser.y"
                       {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_ILLUMINATIONTYPE"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "ILLUMINATIONTYPE";
  }
#line 3080 "Ecc/Parser.cpp"
    break;

  case 88: /* property_type: k_ANIMATION  */
#line 761 "Ecc/Parser.y"
                {
    _strCurrentPropertyPropertyType = "CEntityProperty::EPT_ANIMATION"; 
    _strCurrentPropertyEnumType = "NULL"; 
    _strCurrentPropertyDataType = "ANIMATION";
  }
#line 3090 "Ecc/Parser.cpp"
    break;

  case 89: /* property_wed_name_opt: %empty  */
#line 769 "Ecc/Parser.y"
               {
    _strCurrentPropertyName = "\"\""; 
    _strCurrentPropertyShortcut = "0"; 
    _strCurrentPropertyColor = "0"; // this won't be rendered anyway
  }
#line 3100 "Ecc/Parser.cpp"
    break;

  case 90: /* property_wed_name_opt: c_string property_shortcut_opt property_color_opt  */
#line 774 "Ecc/Parser.y"
                                                      {
    _strCurrentPropertyName = yyvsp[-2].strString; 
  }
#line 3108 "Ecc/Parser.cpp"
    break;

  case 91: /* property_shortcut_opt: %empty  */
#line 779 "Ecc/Parser.y"
               {
    _strCurrentPropertyShortcut = "0"; 
  }
#line 3116 "Ecc/Parser.cpp"
    break;

  case 92: /* property_shortcut_opt: c_char  */
#line 782 "Ecc/Parser.y"
           {
    _strCurrentPropertyShortcut = yyvsp[0].strString; 
  }
#line 3124 "Ecc/Parser.cpp"
    break;

  case 93: /* property_color_opt: %empty  */
#line 787 "Ecc/Parser.y"
               {
    _strCurrentPropertyColor = "0x7F0000FFUL"; // dark red
  }
#line 3132 "Ecc/Parser.cpp"
    break;

  case 94: /* property_color_opt: k_COLOR '(' expression ')'  */
#line 790 "Ecc/Parser.y"
                               {
    _strCurrentPropertyColor = yyvsp[-1].strString; 
  }
#line 3140 "Ecc/Parser.cpp"
    break;

  case 95: /* property_flags_opt: %empty  */
#line 794 "Ecc/Parser.y"
               {
    _strCurrentPropertyFlags = "0"; // dark red
  }
#line 3148 "Ecc/Parser.cpp"
    break;

  case 96: /* property_flags_opt: k_features '(' expression ')'  */
#line 797 "Ecc/Parser.y"
                                  {
    _strCurrentPropertyFlags = yyvsp[-1].strString; 
  }
#line 3156 "Ecc/Parser.cpp"
    break;

  case 97: /* property_default_opt: %empty  */
#line 802 "Ecc/Parser.y"
               {
    if (strcmp(_strCurrentPropertyDataType,"CEntityPointer")==0)  {
      _strCurrentPropertyDefaultCode = (SType(_strCurrentPropertyIdentifier)+" = NULL;").strString;
    } else if (strcmp(_strCurrentPropertyDataType,"CModelObject")==0)  {
      _strCurrentPropertyDefaultCode = 
        (SType(_strCurrentPropertyIdentifier)+".SetData(NULL);\n"+
        _strCurrentPropertyIdentifier+".mo_toTexture.SetData(NULL);").strString;
    } else if (strcmp(_strCurrentPropertyDataType,"CModelInstance")==0)  {
      _strCurrentPropertyDefaultCode = 
        (SType(_strCurrentPropertyIdentifier)+".Clear();\n").strString;
    } else if (strcmp(_strCurrentPropertyDataType,"CAnimObject")==0)  {
      _strCurrentPropertyDefaultCode = 
        (SType(_strCurrentPropertyIdentifier)+".SetData(NULL);\n").strString;
    } else if (strcmp(_strCurrentPropertyDataType,"CSoundObject")==0)  {
      _strCurrentPropertyDefaultCode = 
        (SType(_strCurrentPropertyIdentifier)+".SetOwner(this);\n"+
         _strCurrentPropertyIdentifier+".Stop_internal();").strString;
    } else {
      yyerror("this kind of property must have default value");
      _strCurrentPropertyDefaultCode = "";
    }
  }
#line 3183 "Ecc/Parser.cpp"
    break;

  case 98: /* property_default_opt: '=' property_default_expression  */
#line 824 "Ecc/Parser.y"
                                    {
    if (strcmp(_strCurrentPropertyDataType,"CEntityPointer")==0)  {
      yyerror("CEntityPointer type properties always default to NULL");
    } else {
      _strCurrentPropertyDefaultCode = (SType(_strCurrentPropertyIdentifier)+" = "+yyvsp[0].strString+";").strString;
    }
  }
#line 3195 "Ecc/Parser.cpp"
    break;

  case 104: /* property_default_expression: identifier  */
#line 834 "Ecc/Parser.y"
               {yyval = yyvsp[0] + " ";}
#line 3201 "Ecc/Parser.cpp"
    break;

  case 105: /* property_default_expression: identifier '(' expression ')'  */
#line 835 "Ecc/Parser.y"
                                  {yyval = yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3207 "Ecc/Parser.cpp"
    break;

  case 106: /* property_default_expression: type_keyword '(' expression ')'  */
#line 836 "Ecc/Parser.y"
                                    {yyval = yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3213 "Ecc/Parser.cpp"
    break;

  case 107: /* property_default_expression: '-' property_default_expression  */
#line 837 "Ecc/Parser.y"
                                    {yyval = yyvsp[-1]+yyvsp[0];}
#line 3219 "Ecc/Parser.cpp"
    break;

  case 108: /* property_default_expression: '(' expression ')'  */
#line 838 "Ecc/Parser.y"
                       {yyval = yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3225 "Ecc/Parser.cpp"
    break;

  case 109: /* component_declaration_list: empty_component_declaration_list  */
#line 845 "Ecc/Parser.y"
                                     {
    fprintf(_fTables, "  CEntityComponent()\n};\n");
    fprintf(_fTables, "#define %s_componentsct 0\n", _strCurrentClass);
    fprintf(_fTables, "\n");
    fprintf(_fTables, "\n");
  }
#line 3236 "Ecc/Parser.cpp"
    break;

  case 110: /* component_declaration_list: nonempty_component_declaration_list opt_comma  */
#line 851 "Ecc/Parser.y"
                                                  {
    fprintf(_fTables, "};\n");
    fprintf(_fTables, "#define %s_componentsct ARRAYCOUNT(%s_components)\n", 
      _strCurrentClass, _strCurrentClass);
    fprintf(_fTables, "\n");
  }
#line 3247 "Ecc/Parser.cpp"
    break;

  case 114: /* component_declaration: component_id component_type component_identifier component_filename  */
#line 867 "Ecc/Parser.y"
                                                                        {
  fprintf(_fTables, "#define %s ((0x%08x<<8)+%s)\n",
      _strCurrentComponentIdentifier,
      _iCurrentClassID,
      _strCurrentComponentID);
    fprintf(_fTables, " CEntityComponent(%s, %s, \"%s%s\" %s),\n",
      _strCurrentComponentType,
      _strCurrentComponentIdentifier,
      "EF","NM",
      _strCurrentComponentFileName);
  }
#line 3263 "Ecc/Parser.cpp"
    break;

  case 115: /* component_id: c_int  */
#line 880 "Ecc/Parser.y"
                     { _strCurrentComponentID = yyvsp[0].strString; }
#line 3269 "Ecc/Parser.cpp"
    break;

  case 116: /* component_identifier: identifier  */
#line 881 "Ecc/Parser.y"
                                  { _strCurrentComponentIdentifier = yyvsp[0].strString; }
#line 3275 "Ecc/Parser.cpp"
    break;

  case 117: /* component_filename: c_string  */
#line 882 "Ecc/Parser.y"
                              { _strCurrentComponentFileName = yyvsp[0].strString; }
#line 3281 "Ecc/Parser.cpp"
    break;

  case 118: /* component_type: k_model  */
#line 885 "Ecc/Parser.y"
              { _strCurrentComponentType = "ECT_MODEL"; }
#line 3287 "Ecc/Parser.cpp"
    break;

  case 119: /* component_type: k_texture  */
#line 886 "Ecc/Parser.y"
              { _strCurrentComponentType = "ECT_TEXTURE"; }
#line 3293 "Ecc/Parser.cpp"
    break;

  case 120: /* component_type: k_sound  */
#line 887 "Ecc/Parser.y"
              { _strCurrentComponentType = "ECT_SOUND"; }
#line 3299 "Ecc/Parser.cpp"
    break;

  case 121: /* component_type: k_class  */
#line 888 "Ecc/Parser.y"
              { _strCurrentComponentType = "ECT_CLASS"; }
#line 3305 "Ecc/Parser.cpp"
    break;

  case 122: /* function_list: %empty  */
#line 895 "Ecc/Parser.y"
    { yyval = "";}
#line 3311 "Ecc/Parser.cpp"
    break;

  case 123: /* function_list: function_list function_implementation  */
#line 896 "Ecc/Parser.y"
                                          {yyval = yyvsp[-1]+yyvsp[0];}
#line 3317 "Ecc/Parser.cpp"
    break;

  case 124: /* function_implementation: opt_export opt_virtual return_type opt_tilde identifier '(' parameters_list ')' opt_const '{' statements '}' opt_semicolon  */
#line 901 "Ecc/Parser.y"
                                   {
    const char *strReturnType = yyvsp[-10].strString;
    const char *strFunctionHeader = (yyvsp[-9]+yyvsp[-8]+yyvsp[-7]+yyvsp[-6]+yyvsp[-5]+yyvsp[-4]).strString;
    const char *strFunctionBody = (yyvsp[-3]+yyvsp[-2]+yyvsp[-1]).strString;
    if (strcmp(yyvsp[-8].strString, _strCurrentClass)==0) {
      if (strcmp(strReturnType+strlen(strReturnType)-4, "void")==0 ) {
        strReturnType = "";
      } else {
        yyerror("use 'void' as return type for constructors");
      }
    }
    fprintf(_fDeclaration, " %s %s %s %s;\n", 
      yyvsp[-12].strString, yyvsp[-11].strString, strReturnType, strFunctionHeader);
    fprintf(_fImplementation, "  %s %s::%s %s\n", 
      strReturnType, _strCurrentClass, strFunctionHeader, strFunctionBody);
  }
#line 3338 "Ecc/Parser.cpp"
    break;

  case 125: /* opt_tilde: %empty  */
#line 919 "Ecc/Parser.y"
    { yyval = "";}
#line 3344 "Ecc/Parser.cpp"
    break;

  case 126: /* opt_tilde: '~'  */
#line 920 "Ecc/Parser.y"
        { yyval = " ~ "; }
#line 3350 "Ecc/Parser.cpp"
    break;

  case 127: /* opt_export: %empty  */
#line 924 "Ecc/Parser.y"
    { yyval = "";}
#line 3356 "Ecc/Parser.cpp"
    break;

  case 128: /* opt_export: k_export  */
#line 925 "Ecc/Parser.y"
             { 
    if (_bClassIsExported) {
      yyval = ""; 
    } else {
      yyval = " DECL_DLL "; 
    }
  }
#line 3368 "Ecc/Parser.cpp"
    break;

  case 129: /* opt_const: %empty  */
#line 935 "Ecc/Parser.y"
    { yyval = "";}
#line 3374 "Ecc/Parser.cpp"
    break;

  case 130: /* opt_const: k_const  */
#line 936 "Ecc/Parser.y"
            { yyval = yyvsp[0]; }
#line 3380 "Ecc/Parser.cpp"
    break;

  case 131: /* opt_virtual: %empty  */
#line 939 "Ecc/Parser.y"
    { yyval = "";}
#line 3386 "Ecc/Parser.cpp"
    break;

  case 132: /* opt_virtual: k_virtual  */
#line 940 "Ecc/Parser.y"
              { yyval = yyvsp[0]; }
#line 3392 "Ecc/Parser.cpp"
    break;

  case 135: /* parameters_list: %empty  */
#line 947 "Ecc/Parser.y"
    { yyval = "";}
#line 3398 "Ecc/Parser.cpp"
    break;

  case 139: /* non_void_parameters_list: non_void_parameters_list ',' parameter_declaration  */
#line 953 "Ecc/Parser.y"
                                                       {yyval = yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3404 "Ecc/Parser.cpp"
    break;

  case 140: /* parameter_declaration: any_type identifier  */
#line 956 "Ecc/Parser.y"
                        { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3410 "Ecc/Parser.cpp"
    break;

  case 145: /* any_type: k_enum identifier  */
#line 967 "Ecc/Parser.y"
                      { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3416 "Ecc/Parser.cpp"
    break;

  case 146: /* any_type: any_type '*'  */
#line 968 "Ecc/Parser.y"
                 { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3422 "Ecc/Parser.cpp"
    break;

  case 147: /* any_type: any_type '&'  */
#line 969 "Ecc/Parser.y"
                 { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3428 "Ecc/Parser.cpp"
    break;

  case 148: /* any_type: k_void '*'  */
#line 970 "Ecc/Parser.y"
               { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3434 "Ecc/Parser.cpp"
    break;

  case 149: /* any_type: k_const any_type  */
#line 971 "Ecc/Parser.y"
                     { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3440 "Ecc/Parser.cpp"
    break;

  case 150: /* any_type: k_inline any_type  */
#line 972 "Ecc/Parser.y"
                      { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3446 "Ecc/Parser.cpp"
    break;

  case 151: /* any_type: k_static any_type  */
#line 973 "Ecc/Parser.y"
                      { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3452 "Ecc/Parser.cpp"
    break;

  case 152: /* any_type: k_class any_type  */
#line 974 "Ecc/Parser.y"
                     { yyval=yyvsp[-1]+" "+yyvsp[0]; }
#line 3458 "Ecc/Parser.cpp"
    break;

  case 153: /* any_type: identifier '<' any_type '>'  */
#line 975 "Ecc/Parser.y"
                                { yyval=yyvsp[-3]+" "+yyvsp[-2]+" "+yyvsp[-1]+" "+yyvsp[0]; }
#line 3464 "Ecc/Parser.cpp"
    break;

  case 154: /* procedure_list: %empty  */
#line 983 "Ecc/Parser.y"
    { yyval = "";}
#line 3470 "Ecc/Parser.cpp"
    break;

  case 155: /* procedure_list: procedure_list procedure_implementation  */
#line 984 "Ecc/Parser.y"
                                            {yyval = yyvsp[-1]+yyvsp[0];}
#line 3476 "Ecc/Parser.cpp"
    break;

  case 156: /* opt_override: %empty  */
#line 988 "Ecc/Parser.y"
    { yyval = "-1"; }
#line 3482 "Ecc/Parser.cpp"
    break;

  case 157: /* opt_override: ':' identifier ':' ':' identifier  */
#line 989 "Ecc/Parser.y"
                                      {
    yyval = SType("STATE_")+yyvsp[-3]+"_"+yyvsp[0];
  }
#line 3490 "Ecc/Parser.cpp"
    break;

  case 158: /* $@16: %empty  */
#line 995 "Ecc/Parser.y"
                                                        {
    char *strProcedureName = yyvsp[-4].strString;
    char strInputEventType[80];
    char strInputEventName[80];
    sscanf(yyvsp[-2].strString, "%s %s", strInputEventType, strInputEventName);

    char strStateID[256];
    const char *strBaseStateID = "-1";
    if(strcmp(RemoveLineDirective(strProcedureName), "Main")==0){
      strcpy(strStateID, "1");
      if(strncmp(strInputEventType, "EVoid", 4)!=0 && _strCurrentThumbnail[2]!=0) {
        yyerror("procedure 'Main' can take input parameters only in classes without thumbnails");
      }
    } else {
      sprintf(strStateID, "0x%08x", CreateID());
    }

    sprintf(_strCurrentStateID, "STATE_%s_%s", 
      _strCurrentClass, RemoveLineDirective(strProcedureName));
    fprintf(_fDeclaration, "#define  %s %s\n", _strCurrentStateID, strStateID);
    AddHandlerFunction(strProcedureName, strStateID, yyvsp[0].strString);
    fprintf(_fImplementation, 
      "BOOL %s::%s(const CEntityEvent &__eeInput) {\n#undef STATE_CURRENT\n#define STATE_CURRENT %s\n", 
      _strCurrentClass, strProcedureName, _strCurrentStateID);
    fprintf(_fImplementation, 
      "  ASSERTMSG(__eeInput.ee_slEvent==EVENTCODE_%s, \"%s::%s expects '%s' as input!\");",
      strInputEventType, _strCurrentClass, RemoveLineDirective(strProcedureName), 
      strInputEventType);
    fprintf(_fImplementation, "  const %s &%s = (const %s &)__eeInput;",
      strInputEventType, strInputEventName, strInputEventType);

  }
#line 3527 "Ecc/Parser.cpp"
    break;

  case 159: /* procedure_implementation: identifier '(' event_specification ')' opt_override $@16 '{' statements '}' opt_semicolon  */
#line 1026 "Ecc/Parser.y"
                                     {
    char *strFunctionBody = yyvsp[-2].strString;
    fprintf(_fImplementation, "%s ASSERT(FALSE); return TRUE;};", strFunctionBody);
  }
#line 3536 "Ecc/Parser.cpp"
    break;

  case 160: /* event_specification: %empty  */
#line 1033 "Ecc/Parser.y"
    {
    yyval="EVoid e";
  }
#line 3544 "Ecc/Parser.cpp"
    break;

  case 161: /* event_specification: identifier  */
#line 1036 "Ecc/Parser.y"
               {
    yyval=yyvsp[0]+" e";
  }
#line 3552 "Ecc/Parser.cpp"
    break;

  case 162: /* event_specification: identifier identifier  */
#line 1039 "Ecc/Parser.y"
                          {
    yyval=yyvsp[-1]+" "+yyvsp[0];
  }
#line 3560 "Ecc/Parser.cpp"
    break;

  case 168: /* expression: identifier  */
#line 1046 "Ecc/Parser.y"
               {yyval = yyvsp[0] + " ";}
#line 3566 "Ecc/Parser.cpp"
    break;

  case 189: /* expression: '(' ')'  */
#line 1049 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3572 "Ecc/Parser.cpp"
    break;

  case 190: /* expression: '+' '+'  */
#line 1050 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3578 "Ecc/Parser.cpp"
    break;

  case 191: /* expression: '-' '-'  */
#line 1051 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3584 "Ecc/Parser.cpp"
    break;

  case 192: /* expression: '-' '>'  */
#line 1052 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3590 "Ecc/Parser.cpp"
    break;

  case 193: /* expression: ':' ':'  */
#line 1053 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3596 "Ecc/Parser.cpp"
    break;

  case 194: /* expression: '&' '&'  */
#line 1054 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3602 "Ecc/Parser.cpp"
    break;

  case 195: /* expression: '|' '|'  */
#line 1055 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3608 "Ecc/Parser.cpp"
    break;

  case 196: /* expression: '^' '^'  */
#line 1056 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3614 "Ecc/Parser.cpp"
    break;

  case 197: /* expression: '>' '>'  */
#line 1057 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3620 "Ecc/Parser.cpp"
    break;

  case 198: /* expression: '<' '<'  */
#line 1058 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3626 "Ecc/Parser.cpp"
    break;

  case 199: /* expression: '=' '='  */
#line 1059 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3632 "Ecc/Parser.cpp"
    break;

  case 200: /* expression: '!' '='  */
#line 1060 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3638 "Ecc/Parser.cpp"
    break;

  case 201: /* expression: '>' '='  */
#line 1061 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3644 "Ecc/Parser.cpp"
    break;

  case 202: /* expression: '<' '='  */
#line 1062 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3650 "Ecc/Parser.cpp"
    break;

  case 203: /* expression: '&' '='  */
#line 1063 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3656 "Ecc/Parser.cpp"
    break;

  case 204: /* expression: '|' '='  */
#line 1064 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3662 "Ecc/Parser.cpp"
    break;

  case 205: /* expression: '^' '='  */
#line 1065 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3668 "Ecc/Parser.cpp"
    break;

  case 206: /* expression: '+' '='  */
#line 1066 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3674 "Ecc/Parser.cpp"
    break;

  case 207: /* expression: '-' '='  */
#line 1067 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3680 "Ecc/Parser.cpp"
    break;

  case 208: /* expression: '/' '='  */
#line 1068 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3686 "Ecc/Parser.cpp"
    break;

  case 209: /* expression: '%' '='  */
#line 1069 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3692 "Ecc/Parser.cpp"
    break;

  case 210: /* expression: '*' '='  */
#line 1070 "Ecc/Parser.y"
            {yyval=yyvsp[-1]+yyvsp[0];}
#line 3698 "Ecc/Parser.cpp"
    break;

  case 211: /* expression: '>' '>' '='  */
#line 1071 "Ecc/Parser.y"
                {yyval=yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3704 "Ecc/Parser.cpp"
    break;

  case 212: /* expression: '<' '<' '='  */
#line 1072 "Ecc/Parser.y"
                {yyval=yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3710 "Ecc/Parser.cpp"
    break;

  case 213: /* expression: '(' expression ')'  */
#line 1073 "Ecc/Parser.y"
                       {yyval = yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3716 "Ecc/Parser.cpp"
    break;

  case 214: /* expression: expression expression  */
#line 1074 "Ecc/Parser.y"
                          {yyval = yyvsp[-1]+" "+yyvsp[0];}
#line 3722 "Ecc/Parser.cpp"
    break;

  case 246: /* case_constant_expression: identifier  */
#line 1087 "Ecc/Parser.y"
               {yyval = yyvsp[0] + " ";}
#line 3728 "Ecc/Parser.cpp"
    break;

  case 247: /* statements: %empty  */
#line 1094 "Ecc/Parser.y"
    { yyval = "";}
#line 3734 "Ecc/Parser.cpp"
    break;

  case 248: /* statements: statements statement  */
#line 1095 "Ecc/Parser.y"
                         { yyval = yyvsp[-1]+yyvsp[0]; }
#line 3740 "Ecc/Parser.cpp"
    break;

  case 249: /* statement: expression ';'  */
#line 1098 "Ecc/Parser.y"
                   {yyval=yyvsp[-1]+yyvsp[0];}
#line 3746 "Ecc/Parser.cpp"
    break;

  case 250: /* statement: k_switch '(' expression ')' '{' statements '}'  */
#line 1099 "Ecc/Parser.y"
                                                   {yyval=yyvsp[-6]+yyvsp[-5]+yyvsp[-4]+yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3752 "Ecc/Parser.cpp"
    break;

  case 251: /* statement: k_case case_constant_expression ':'  */
#line 1100 "Ecc/Parser.y"
                                        {yyval=yyvsp[-2]+" "+yyvsp[-1]+yyvsp[0]+" ";}
#line 3758 "Ecc/Parser.cpp"
    break;

  case 252: /* statement: '{' statements '}'  */
#line 1101 "Ecc/Parser.y"
                       {yyval=yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3764 "Ecc/Parser.cpp"
    break;

  case 253: /* statement: expression '{' statements '}'  */
#line 1102 "Ecc/Parser.y"
                                  {yyval=yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];}
#line 3770 "Ecc/Parser.cpp"
    break;

  case 270: /* statement_if: k_if '(' expression ')' '{' statements '}'  */
#line 1123 "Ecc/Parser.y"
                                               {
    if (yyvsp[-1].bCrossesStates) {
      char strAfterIfName[80], strAfterIfID[11];
      CreateInternalHandlerFunction(strAfterIfName, strAfterIfID);
      yyval = yyvsp[-6]+"(!"+yyvsp[-5]+yyvsp[-4]+yyvsp[-3]+"){ Jump(STATE_CURRENT,"+strAfterIfID+", FALSE, EInternal());return TRUE;}"+yyvsp[-1]+
        "Jump(STATE_CURRENT,"+strAfterIfID+", FALSE, EInternal());return TRUE;}"+
        "BOOL "+_strCurrentClass+"::"+strAfterIfName+"(const CEntityEvent &__eeInput){"+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+strAfterIfID+"\n";
    } else {
      yyval = yyvsp[-6]+yyvsp[-5]+yyvsp[-4]+yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];
    }
  }
#line 3788 "Ecc/Parser.cpp"
    break;

  case 271: /* statement_if_else: k_if '(' expression ')' '{' statements '}' k_else statement  */
#line 1139 "Ecc/Parser.y"
                                                                {
    if (yyvsp[-3].bCrossesStates || yyvsp[0].bCrossesStates) {
      char strAfterIfName[80], strAfterIfID[11];
      char strElseName[80], strElseID[11];
      CreateInternalHandlerFunction(strAfterIfName, strAfterIfID);
      CreateInternalHandlerFunction(strElseName, strElseID);
      yyval = yyvsp[-8]+"(!"+yyvsp[-7]+yyvsp[-6]+yyvsp[-5]+"){ Jump(STATE_CURRENT,"+strElseID+", FALSE, EInternal());return TRUE;}"+
        yyvsp[-3]+"Jump(STATE_CURRENT,"+strAfterIfID+", FALSE, EInternal());return TRUE;}"+
        "BOOL "+_strCurrentClass+"::"+strElseName+"(const CEntityEvent &__eeInput){"+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+strElseID+"\n"+
        yyvsp[0]+"Jump(STATE_CURRENT,"+strAfterIfID+", FALSE, EInternal());return TRUE;}\n"+
        "BOOL "+_strCurrentClass+"::"+strAfterIfName+"(const CEntityEvent &__eeInput){"+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+strAfterIfID+"\n";
    } else {
      yyval = yyvsp[-8]+yyvsp[-7]+yyvsp[-6]+yyvsp[-5]+yyvsp[-4]+yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+" "+yyvsp[0];
    }
  }
#line 3812 "Ecc/Parser.cpp"
    break;

  case 272: /* $@17: %empty  */
#line 1161 "Ecc/Parser.y"
                               {
    if (strlen(_strInLoopName)>0) {
      yyerror("Nested loops are not implemented yet");
    }
  }
#line 3822 "Ecc/Parser.cpp"
    break;

  case 273: /* statement_while: k_while '(' expression ')' $@17 '{' statements '}'  */
#line 1165 "Ecc/Parser.y"
                       {
    if (yyvsp[-1].bCrossesStates) {
      CreateInternalHandlerFunction(_strInLoopName, _strInLoopID);
      CreateInternalHandlerFunction(_strAfterLoopName, _strAfterLoopID);
      yyval = SType(GetLineDirective(yyvsp[-7]))+"Jump(STATE_CURRENT,"+_strInLoopID+", FALSE, EInternal());return TRUE;}"+
        "BOOL "+_strCurrentClass+"::"+_strInLoopName+"(const CEntityEvent &__eeInput)"+yyvsp[-2]+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strInLoopID+"\n"+
        "if(!"+yyvsp[-6]+yyvsp[-5]+yyvsp[-4]+"){ Jump(STATE_CURRENT,"+_strAfterLoopID+", FALSE, EInternal());return TRUE;}"+
        yyvsp[-1]+"Jump(STATE_CURRENT,"+_strInLoopID+", FALSE, EInternal());return TRUE;"+yyvsp[0]+
        "BOOL "+_strCurrentClass+"::"+_strAfterLoopName+"(const CEntityEvent &__eeInput) {"+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strAfterLoopID+"\n";
    } else {
      yyval = yyvsp[-7]+yyvsp[-6]+yyvsp[-5]+yyvsp[-4]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];
    } 
    _strInLoopName[0] = 0;
  }
#line 3845 "Ecc/Parser.cpp"
    break;

  case 274: /* $@18: %empty  */
#line 1186 "Ecc/Parser.y"
                            {
    if (strlen(_strInLoopName)>0) {
      yyerror("Nested loops are not implemented yet");
    }
    _strInLoopName[0] = 0;
  }
#line 3856 "Ecc/Parser.cpp"
    break;

  case 275: /* statement_dowhile: k_do '{' statements '}' $@18 k_while '(' expression ')' ';'  */
#line 1191 "Ecc/Parser.y"
                                   {
    if (yyvsp[-7].bCrossesStates) {
      CreateInternalHandlerFunction(_strInLoopName, _strInLoopID);
      CreateInternalHandlerFunction(_strAfterLoopName, _strAfterLoopID);
      yyval = SType(GetLineDirective(yyvsp[-9]))+"Jump(STATE_CURRENT,"+_strInLoopID+", FALSE, EInternal());return TRUE;}"+
        "BOOL "+_strCurrentClass+"::"+_strInLoopName+"(const CEntityEvent &__eeInput)"+yyvsp[-8]+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strInLoopID+"\n"+yyvsp[-7]+
        "if(!"+yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+"){ Jump(STATE_CURRENT,"+_strAfterLoopID+", FALSE, EInternal());return TRUE;}"+
        "Jump(STATE_CURRENT,"+_strInLoopID+", FALSE, EInternal());return TRUE;"+yyvsp[-6]+
        "BOOL "+_strCurrentClass+"::"+_strAfterLoopName+"(const CEntityEvent &__eeInput) {"+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strAfterLoopID+"\n";
    } else {
      yyval = yyvsp[-9]+yyvsp[-8]+yyvsp[-7]+yyvsp[-6]+yyvsp[-4]+yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];
    } 
    _strInLoopName[0] = 0;
  }
#line 3879 "Ecc/Parser.cpp"
    break;

  case 276: /* $@19: %empty  */
#line 1212 "Ecc/Parser.y"
                                                           {
    if (strlen(_strInLoopName)>0) {
      yyerror("Nested loops are not implemented yet");
    }
  }
#line 3889 "Ecc/Parser.cpp"
    break;

  case 277: /* statement_for: k_for '(' expression ';' expression ';' expression ')' $@19 '{' statements '}'  */
#line 1216 "Ecc/Parser.y"
                       {
    if (yyvsp[-1].bCrossesStates) {
      CreateInternalHandlerFunction(_strInLoopName, _strInLoopID);
      CreateInternalHandlerFunction(_strAfterLoopName, _strAfterLoopID);
      yyerror("For loops across states are not supported");
    } else {
      yyval = yyvsp[-11]+yyvsp[-10]+yyvsp[-9]+yyvsp[-8]+yyvsp[-7]+yyvsp[-6]+yyvsp[-5]+yyvsp[-4]+yyvsp[-2]+yyvsp[-1]+yyvsp[0];
    } 
    _strInLoopName[0] = 0;
  }
#line 3904 "Ecc/Parser.cpp"
    break;

  case 278: /* $@20: %empty  */
#line 1229 "Ecc/Parser.y"
                           {
      if (!_bInProcedure) {
        yyerror("Cannot have 'wait' in functions");
      }
      CreateInternalHandlerFunction(_strInWaitName, _strInWaitID);
      CreateInternalHandlerFunction(_strAfterWaitName, _strAfterWaitID);
      _bHasOtherwise = 0;
      _bInHandler = 1;
  }
#line 3918 "Ecc/Parser.cpp"
    break;

  case 279: /* statement_wait: k_wait wait_expression $@20 '{' handlers_list '}'  */
#line 1237 "Ecc/Parser.y"
                          {
    if (yyvsp[-1].bCrossesStates) {
      yyerror("'wait' statements must not be nested");
      yyval = "";
    } else {
      SType stDefault;
      if (!_bHasOtherwise) {
        stDefault = SType("default: return FALSE; break;");
      } else {
        stDefault = SType("");
      }

      yyval = SType(GetLineDirective(yyvsp[-5]))+yyvsp[-4]+";\n"+
        "Jump(STATE_CURRENT, "+_strInWaitID+", FALSE, EBegin());return TRUE;}"+
        "BOOL "+_strCurrentClass+"::"+_strInWaitName+"(const CEntityEvent &__eeInput) {"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strInWaitID+"\n"+
        "switch(__eeInput.ee_slEvent)"+yyvsp[-2]+yyvsp[-1]+stDefault+yyvsp[0]+
        "return TRUE;}BOOL "+_strCurrentClass+"::"+_strAfterWaitName+"(const CEntityEvent &__eeInput){"+
        "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
        "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strAfterWaitID+"\n";
      yyval.bCrossesStates = 1;
      _bInHandler = 0;
    }
  }
#line 3947 "Ecc/Parser.cpp"
    break;

  case 280: /* statement_autowait: k_autowait wait_expression ';'  */
#line 1263 "Ecc/Parser.y"
                                   {
    if (!_bInProcedure) {
      yyerror("Cannot have 'autowait' in functions");
    }
    CreateInternalHandlerFunction(_strInWaitName, _strInWaitID);
    CreateInternalHandlerFunction(_strAfterWaitName, _strAfterWaitID);
    _bHasOtherwise = 0;

    yyval = SType(GetLineDirective(yyvsp[-2]))+yyvsp[-1]+";\n"+
      "Jump(STATE_CURRENT, "+_strInWaitID+", FALSE, EBegin());return TRUE;}"+
      "BOOL "+_strCurrentClass+"::"+_strInWaitName+"(const CEntityEvent &__eeInput) {"+
      "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strInWaitID+"\n"+
      "switch(__eeInput.ee_slEvent) {"+
      "case EVENTCODE_EBegin: return TRUE;"+
      "case EVENTCODE_ETimer: Jump(STATE_CURRENT,"+_strAfterWaitID+", FALSE, EInternal()); return TRUE;"+
      "default: return FALSE; }}"+
      "BOOL "+_strCurrentClass+"::"+_strAfterWaitName+"(const CEntityEvent &__eeInput){"+
      "\nASSERT(__eeInput.ee_slEvent==EVENTCODE_EInternal);"+
      "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strAfterWaitID+"\n"+yyvsp[0];
    yyval.bCrossesStates = 1;
  }
#line 3973 "Ecc/Parser.cpp"
    break;

  case 281: /* statement_waitevent: k_waitevent wait_expression identifier opt_eventvar ';'  */
#line 1287 "Ecc/Parser.y"
                                                            {
    if (!_bInProcedure) {
      yyerror("Cannot have 'autocall' in functions");
    }
    CreateInternalHandlerFunction(_strInWaitName, _strInWaitID);
    CreateInternalHandlerFunction(_strAfterWaitName, _strAfterWaitID);
    _bHasOtherwise = 0;

    yyval = SType(GetLineDirective(yyvsp[-4]))+yyvsp[-3]+";\n"+
      "Jump(STATE_CURRENT, "+_strInWaitID+", FALSE, EBegin());return TRUE;}"+
      "BOOL "+_strCurrentClass+"::"+_strInWaitName+"(const CEntityEvent &__eeInput) {"+
      "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strInWaitID+"\n"+
      "switch(__eeInput.ee_slEvent) {"+
      "case EVENTCODE_EBegin: return TRUE;"+
      "case EVENTCODE_"+yyvsp[-2]+": Jump(STATE_CURRENT,"+_strAfterWaitID+", FALSE, __eeInput); return TRUE;"+
      "default: return FALSE; }}"+
      "BOOL "+_strCurrentClass+"::"+_strAfterWaitName+"(const CEntityEvent &__eeInput){"+
      "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strAfterWaitID+"\n"+
      "const "+yyvsp[-2]+"&"+yyvsp[-1]+"= ("+yyvsp[-2]+"&)__eeInput;\n"+yyvsp[0];
    yyval.bCrossesStates = 1;
  }
#line 3999 "Ecc/Parser.cpp"
    break;

  case 282: /* opt_eventvar: %empty  */
#line 1312 "Ecc/Parser.y"
    {
    yyval = SType("__e");
  }
#line 4007 "Ecc/Parser.cpp"
    break;

  case 283: /* opt_eventvar: identifier  */
#line 1315 "Ecc/Parser.y"
               {
    yyval = yyvsp[0];
  }
#line 4015 "Ecc/Parser.cpp"
    break;

  case 284: /* statement_autocall: k_autocall jumptarget '(' event_expression ')' identifier opt_eventvar ';'  */
#line 1320 "Ecc/Parser.y"
                                                                               {
    if (!_bInProcedure) {
      yyerror("Cannot have 'autocall' in functions");
    }
    CreateInternalHandlerFunction(_strInWaitName, _strInWaitID);
    CreateInternalHandlerFunction(_strAfterWaitName, _strAfterWaitID);
    _bHasOtherwise = 0;

    yyval = SType(GetLineDirective(yyvsp[-7]))+yyvsp[-6]+";\n"+
      "Jump(STATE_CURRENT, "+_strInWaitID+", FALSE, EBegin());return TRUE;}"+
      "BOOL "+_strCurrentClass+"::"+_strInWaitName+"(const CEntityEvent &__eeInput) {"+
      "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strInWaitID+"\n"+
      "switch(__eeInput.ee_slEvent) {"+
      "case EVENTCODE_EBegin: Call"+yyvsp[-5]+"STATE_CURRENT, "+yyvsp[-6]+", "+yyvsp[-4]+yyvsp[-3]+";return TRUE;"+
      "case EVENTCODE_"+yyvsp[-2]+": Jump(STATE_CURRENT,"+_strAfterWaitID+", FALSE, __eeInput); return TRUE;"+
      "default: return FALSE; }}"+
      "BOOL "+_strCurrentClass+"::"+_strAfterWaitName+"(const CEntityEvent &__eeInput){"+
      "\n#undef STATE_CURRENT\n#define STATE_CURRENT "+_strAfterWaitID+"\n"+
      "const "+yyvsp[-2]+"&"+yyvsp[-1]+"= ("+yyvsp[-2]+"&)__eeInput;\n"+yyvsp[0];
    yyval.bCrossesStates = 1;
  }
#line 4041 "Ecc/Parser.cpp"
    break;

  case 285: /* wait_expression: '(' ')'  */
#line 1344 "Ecc/Parser.y"
            {
    yyval = SType("SetTimerAt(THINKTIME_NEVER)"); 
  }
#line 4049 "Ecc/Parser.cpp"
    break;

  case 286: /* wait_expression: '(' expression ')'  */
#line 1347 "Ecc/Parser.y"
                       {
    yyval = SType("SetTimerAfter")+yyvsp[-2]+yyvsp[-1]+yyvsp[0]; 
  }
#line 4057 "Ecc/Parser.cpp"
    break;

  case 287: /* statement_jump: k_jump jumptarget '(' event_expression ')' ';'  */
#line 1353 "Ecc/Parser.y"
                                                   {
    if (!_bInProcedure) {
      yyerror("Cannot have 'jump' in functions");
    }
    yyval = SType(GetLineDirective(yyvsp[-5]))+"Jump"+yyvsp[-3]+"STATE_CURRENT, "+yyvsp[-4]+", "+yyvsp[-2]+yyvsp[-1]+";return TRUE;";
  }
#line 4068 "Ecc/Parser.cpp"
    break;

  case 288: /* statement_call: k_call jumptarget '(' event_expression ')' ';'  */
#line 1362 "Ecc/Parser.y"
                                                  {
    if (!_bInProcedure) {
      yyerror("Cannot have 'call' in functions");
    }
    if (!_bInHandler) {
      yyerror("'call' must be inside a 'wait' statement");
    }
    yyval = SType(GetLineDirective(yyvsp[-5]))+"Call"+yyvsp[-3]+"STATE_CURRENT, "+yyvsp[-4]+", "+yyvsp[-2]+yyvsp[-1]+";return TRUE;";
  }
#line 4082 "Ecc/Parser.cpp"
    break;

  case 289: /* event_expression: expression  */
#line 1374 "Ecc/Parser.y"
               { 
    yyval = yyvsp[0];
  }
#line 4090 "Ecc/Parser.cpp"
    break;

  case 290: /* event_expression: %empty  */
#line 1377 "Ecc/Parser.y"
    {
    yyval = SType("EVoid()");
  }
#line 4098 "Ecc/Parser.cpp"
    break;

  case 291: /* jumptarget: identifier  */
#line 1383 "Ecc/Parser.y"
               {
    yyval = SType("STATE_")+_strCurrentClass+"_"+yyvsp[0]+", TRUE";
  }
#line 4106 "Ecc/Parser.cpp"
    break;

  case 292: /* jumptarget: identifier ':' ':' identifier  */
#line 1386 "Ecc/Parser.y"
                                  {
    yyval = SType("STATE_")+yyvsp[-3]+"_"+yyvsp[0]+", FALSE";
  }
#line 4114 "Ecc/Parser.cpp"
    break;

  case 293: /* statement_stop: k_stop ';'  */
#line 1392 "Ecc/Parser.y"
               {
    yyval = SType(GetLineDirective(yyvsp[-1]))+"UnsetTimer();Jump(STATE_CURRENT,"
      +_strAfterWaitID+", FALSE, EInternal());"+"return TRUE"+yyvsp[0];
  }
#line 4123 "Ecc/Parser.cpp"
    break;

  case 294: /* statement_resume: k_resume ';'  */
#line 1398 "Ecc/Parser.y"
                 {
    yyval = SType(GetLineDirective(yyvsp[-1]))+"return TRUE"+yyvsp[0];
  }
#line 4131 "Ecc/Parser.cpp"
    break;

  case 295: /* statement_pass: k_pass ';'  */
#line 1403 "Ecc/Parser.y"
               {
    yyval = SType(GetLineDirective(yyvsp[-1]))+"return FALSE"+yyvsp[0];
  }
#line 4139 "Ecc/Parser.cpp"
    break;

  case 296: /* statement_return: k_return opt_expression ';'  */
#line 1408 "Ecc/Parser.y"
                                {
    if (!_bInProcedure) {
      yyval = yyvsp[-2]+" "+yyvsp[-1]+yyvsp[0];
    } else {
      if (strlen(yyvsp[-1].strString)==0) {
        yyvsp[-1] = SType("EVoid()");
      }
      yyval = SType(GetLineDirective(yyvsp[-2]))
        +"Return(STATE_CURRENT,"+yyvsp[-1]+");"
        +yyvsp[-2]+" TRUE"+yyvsp[0];
    }
  }
#line 4156 "Ecc/Parser.cpp"
    break;

  case 297: /* opt_expression: %empty  */
#line 1422 "Ecc/Parser.y"
    {yyval = "";}
#line 4162 "Ecc/Parser.cpp"
    break;

  case 299: /* handler: k_on '(' event_specification ')' ':' '{' statements '}' opt_semicolon  */
#line 1427 "Ecc/Parser.y"
                                                                          {
    char strInputEventType[80];
    char strInputEventName[80];
    sscanf(yyvsp[-6].strString, "%s %s", strInputEventType, strInputEventName);

    yyval = SType("case")+yyvsp[-7]+"EVENTCODE_"+strInputEventType+yyvsp[-5]+yyvsp[-4]+yyvsp[-3]+
      "const "+strInputEventType+"&"+strInputEventName+"= ("+
      strInputEventType+"&)__eeInput;\n"+yyvsp[-2]+yyvsp[-1]+"ASSERT(FALSE);break;";
  }
#line 4176 "Ecc/Parser.cpp"
    break;

  case 300: /* handler: k_otherwise '(' event_specification ')' ':' '{' statements '}' opt_semicolon  */
#line 1436 "Ecc/Parser.y"
                                                                                 {
    char strInputEventType[80];
    char strInputEventName[80];
    sscanf(yyvsp[-6].strString, "%s %s", strInputEventType, strInputEventName);

    yyval = SType("default")+yyvsp[-4]+yyvsp[-3]+yyvsp[-2]+yyvsp[-1]+"ASSERT(FALSE);break;";
    _bHasOtherwise = 1;
  }
#line 4189 "Ecc/Parser.cpp"
    break;

  case 301: /* handlers_list: %empty  */
#line 1446 "Ecc/Parser.y"
    { yyval = "";}
#line 4195 "Ecc/Parser.cpp"
    break;

  case 302: /* handlers_list: handlers_list handler  */
#line 1447 "Ecc/Parser.y"
                          { yyval = yyvsp[-1]+yyvsp[0]; }
#line 4201 "Ecc/Parser.cpp"
    break;


#line 4205 "Ecc/Parser.cpp"

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

#line 1450 "Ecc/Parser.y"

