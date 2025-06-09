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

#ifndef YY_YY_ENGINE_SKA_SMCPARS_HPP_INCLUDED
# define YY_YY_ENGINE_SKA_SMCPARS_HPP_INCLUDED
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
    c_float = 258,                 /* c_float  */
    c_int = 259,                   /* c_int  */
    c_string = 260,                /* c_string  */
    c_modelinstance = 261,         /* c_modelinstance  */
    k_SE_SMC = 262,                /* k_SE_SMC  */
    k_SE_END = 263,                /* k_SE_END  */
    k_NAME = 264,                  /* k_NAME  */
    k_TFNM = 265,                  /* k_TFNM  */
    k_MESH = 266,                  /* k_MESH  */
    k_SKELETON = 267,              /* k_SKELETON  */
    k_ANIMSET = 268,               /* k_ANIMSET  */
    K_ANIMATION = 269,             /* K_ANIMATION  */
    k_TEXTURES = 270,              /* k_TEXTURES  */
    k_PARENTBONE = 271,            /* k_PARENTBONE  */
    k_OFFSET = 272,                /* k_OFFSET  */
    k_COLISION = 273,              /* k_COLISION  */
    k_ALLFRAMESBBOX = 274,         /* k_ALLFRAMESBBOX  */
    k_ANIMSPEED = 275,             /* k_ANIMSPEED  */
    k_COLOR = 276                  /* k_COLOR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 59 "Engine/Ska/smcPars.y"

  int i;
  float f;
  const char *str;
  CModelInstance *pmi;
  float f6[6];

#line 93 "Engine/Ska/smcPars.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_ENGINE_SKA_SMCPARS_HPP_INCLUDED  */
