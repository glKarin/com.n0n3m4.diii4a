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

#ifndef YY_YY_ENGINE_BASE_PARSER_HPP_INCLUDED
# define YY_YY_ENGINE_BASE_PARSER_HPP_INCLUDED
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
    c_char = 261,                  /* c_char  */
    identifier = 262,              /* identifier  */
    k_INDEX = 263,                 /* k_INDEX  */
    k_FLOAT = 264,                 /* k_FLOAT  */
    k_CTString = 265,              /* k_CTString  */
    k_void = 266,                  /* k_void  */
    k_const = 267,                 /* k_const  */
    k_user = 268,                  /* k_user  */
    k_persistent = 269,            /* k_persistent  */
    k_extern = 270,                /* k_extern  */
    k_pre = 271,                   /* k_pre  */
    k_post = 272,                  /* k_post  */
    k_help = 273,                  /* k_help  */
    k_if = 274,                    /* k_if  */
    k_else = 275,                  /* k_else  */
    k_else_if = 276,               /* k_else_if  */
    SHL = 277,                     /* SHL  */
    SHR = 278,                     /* SHR  */
    EQ = 279,                      /* EQ  */
    NEQ = 280,                     /* NEQ  */
    GEQ = 281,                     /* GEQ  */
    LEQ = 282,                     /* LEQ  */
    LOGAND = 283,                  /* LOGAND  */
    LOGOR = 284,                   /* LOGOR  */
    block_beg = 285,               /* block_beg  */
    block_end = 286,               /* block_end  */
    TYPECAST = 287,                /* TYPECAST  */
    SIGN = 288                     /* SIGN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 212 "Engine/Base/Parser.y"

  value val;                  // for constants and expressions
  arguments arg;               // for function input arguments
  ULONG ulFlags;              // for declaration qualifiers
  INDEX istType;              // for types
  CShellSymbol *pssSymbol;    // for symbols
  struct LValue lvLValue;
  INDEX (*pPreFunc)(INDEX);  // pre-set function for a variable
  void (*pPostFunc)(INDEX); // post-set function for a variable

#line 108 "Engine/Base/Parser.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int yyparse (void);


#endif /* !YY_YY_ENGINE_BASE_PARSER_HPP_INCLUDED  */
