/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_ECC_PARSER_HPP_INCLUDED
# define YY_YY_ECC_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    c_char = 258,                  /* c_char  */
    c_int = 259,                   /* c_int  */
    c_float = 260,                 /* c_float  */
    c_bool = 261,                  /* c_bool  */
    c_string = 262,                /* c_string  */
    identifier = 263,              /* identifier  */
    cppblock = 264,                /* cppblock  */
    k_while = 265,                 /* k_while  */
    k_for = 266,                   /* k_for  */
    k_if = 267,                    /* k_if  */
    k_else = 268,                  /* k_else  */
    k_enum = 269,                  /* k_enum  */
    k_switch = 270,                /* k_switch  */
    k_case = 271,                  /* k_case  */
    k_class = 272,                 /* k_class  */
    k_do = 273,                    /* k_do  */
    k_void = 274,                  /* k_void  */
    k_const = 275,                 /* k_const  */
    k_inline = 276,                /* k_inline  */
    k_static = 277,                /* k_static  */
    k_virtual = 278,               /* k_virtual  */
    k_return = 279,                /* k_return  */
    k_autowait = 280,              /* k_autowait  */
    k_autocall = 281,              /* k_autocall  */
    k_waitevent = 282,             /* k_waitevent  */
    k_event = 283,                 /* k_event  */
    k_name = 284,                  /* k_name  */
    k_thumbnail = 285,             /* k_thumbnail  */
    k_features = 286,              /* k_features  */
    k_uses = 287,                  /* k_uses  */
    k_export = 288,                /* k_export  */
    k_texture = 289,               /* k_texture  */
    k_sound = 290,                 /* k_sound  */
    k_model = 291,                 /* k_model  */
    k_properties = 292,            /* k_properties  */
    k_components = 293,            /* k_components  */
    k_functions = 294,             /* k_functions  */
    k_procedures = 295,            /* k_procedures  */
    k_wait = 296,                  /* k_wait  */
    k_on = 297,                    /* k_on  */
    k_otherwise = 298,             /* k_otherwise  */
    k_call = 299,                  /* k_call  */
    k_jump = 300,                  /* k_jump  */
    k_stop = 301,                  /* k_stop  */
    k_resume = 302,                /* k_resume  */
    k_pass = 303,                  /* k_pass  */
    k_CTString = 304,              /* k_CTString  */
    k_CTStringTrans = 305,         /* k_CTStringTrans  */
    k_CTFileName = 306,            /* k_CTFileName  */
    k_CTFileNameNoDep = 307,       /* k_CTFileNameNoDep  */
    k_BOOL = 308,                  /* k_BOOL  */
    k_COLOR = 309,                 /* k_COLOR  */
    k_FLOAT = 310,                 /* k_FLOAT  */
    k_INDEX = 311,                 /* k_INDEX  */
    k_RANGE = 312,                 /* k_RANGE  */
    k_CEntityPointer = 313,        /* k_CEntityPointer  */
    k_CModelObject = 314,          /* k_CModelObject  */
    k_CModelInstance = 315,        /* k_CModelInstance  */
    k_CAnimObject = 316,           /* k_CAnimObject  */
    k_CSoundObject = 317,          /* k_CSoundObject  */
    k_CPlacement3D = 318,          /* k_CPlacement3D  */
    k_FLOATaabbox3D = 319,         /* k_FLOATaabbox3D  */
    k_FLOATmatrix3D = 320,         /* k_FLOATmatrix3D  */
    k_FLOATquat3D = 321,           /* k_FLOATquat3D  */
    k_ANGLE = 322,                 /* k_ANGLE  */
    k_FLOAT3D = 323,               /* k_FLOAT3D  */
    k_ANGLE3D = 324,               /* k_ANGLE3D  */
    k_FLOATplane3D = 325,          /* k_FLOATplane3D  */
    k_ANIMATION = 326,             /* k_ANIMATION  */
    k_ILLUMINATIONTYPE = 327,      /* k_ILLUMINATIONTYPE  */
    k_FLAGS = 328                  /* k_FLAGS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_ECC_PARSER_HPP_INCLUDED  */
