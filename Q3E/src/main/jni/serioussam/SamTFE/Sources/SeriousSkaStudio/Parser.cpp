
/*  A Bison parser, made from seriousskastudio/parser.y with Bison version GNU Bison version 1.24
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	c_float	258
#define	c_int	259
#define	c_string	260
#define	k_SE_MESH	261
#define	k_SE_SKELETON	262
#define	k_PARENT	263
#define	k_BONES	264
#define	k_VERTICES	265
#define	k_NORMALS	266
#define	k_UVMAPS	267
#define	k_NAME	268
#define	k_TEXCOORDS	269
#define	k_SURFACES	270
#define	k_TRIANGLE_SET	271
#define	k_WEIGHTS	272
#define	k_WEIGHT_SET	273
#define	k_MORPHS	274
#define	k_RELATIVE	275
#define	k_TRUE	276
#define	k_FALSE	277
#define	k_MORPH_SET	278
#define	k_SE_MESH_END	279
#define	k_SE_SKELETON_END	280
#define	k_SE_ANIM	281
#define	k_SEC_PER_FRAME	282
#define	k_FRAMES	283
#define	k_BONEENVELOPES	284
#define	k_MORPHENVELOPES	285
#define	k_DEFAULT_POSE	286
#define	k_SE_ANIM_END	287
#define	k_ANIM_SET_LIST	288
#define	k_ANIM_ID	289
#define	k_MAX_DISTANCE	290
#define	k_MESHLODLIST	291
#define	k_SKELETONLODLIST	292
#define	k_TRESHOLD	293
#define	k_COMPRESION	294
#define	k_LENGTH	295
#define	k_ANIMSPEED	296
#define	k_SHADER_PARAMS	297
#define	k_SHADER_PARAMS_END	298
#define	k_SHADER_SURFACES	299
#define	k_SHADER_SURFACE	300
#define	k_SHADER_NAME	301
#define	k_SHADER_TEXTURES	302
#define	k_SHADER_UVMAPS	303
#define	k_SHADER_COLORS	304
#define	k_SHADER_FLOATS	305
#define	k_SHADER_FLAGS	306
#define	k_FULL_FACE_FORWARD	307
#define	k_HALF_FACE_FORWARD	308

#line 1 "seriousskastudio/parser.y"

#include "StdAfx.h"
#include "ParsingSymbols.h"

#include <Engine/Templates/DynamicStackArray.cpp>
#include <Engine/Ska/StringTable.h>
#include <Engine/Ska/Render.h>

extern CTFileName strCurentFileName;
MeshLOD *pMeshLOD;
SkeletonLOD *pSkeletonLOD;

static INDEX _ctVertices  = 0;
static INDEX _ctNormals   = 0;
static INDEX _ctUVMaps    = 0;
static INDEX _ctTexCoords = 0;
static INDEX _ctSurfaces  = 0;
static INDEX _ctTriangles = 0;
static INDEX _ctWeights   = 0;
static INDEX _ctVertexWeights = 0;
static INDEX _ctMorphs    = 0;
static INDEX _ctVertexMorphs = 0;
static INDEX _ctBones     = 0;
static INDEX _ctBoneEnvelopes = 0;
static INDEX _ctMorphEnvelopes = 0;
static INDEX _ctFrames = 0;

static INDEX _iBone       = 0;
static INDEX _iVertex     = 0;
static INDEX _iNormal     = 0;
static INDEX _iUVMap      = 0;
static INDEX _iTexCoord   = 0;
static INDEX _iSurface    = 0;
static INDEX _iTriangle   = 0;
static INDEX _iWeight     = 0;
static INDEX _iVertexWeight  = 0;
static INDEX _iMorph      = 0;
static INDEX _iVertexMorph = 0;
static INDEX _iBoneEnvelope = 0;
static INDEX _iMorphEnvelope = 0;
static INDEX _iFrame = 0;

/*
** Shader
*/
struct SurfaceShader
{
  CTString fnShaderName;
  INDEX ss_iSurfaceID;
  ShaderParams ss_spShaderParams;
};
INDEX _ishParamIndex; // current index of shader param
INDEX _ctshParamsMax; // size of array allocated for shader params
CStaticArray<struct SurfaceShader> _assSurfaceShaders;
INDEX _iShaderSurfIndex = 0;
INDEX _ctShaderSurfaces = 0;

/*
** 
*/
float _fCurentMaxDistance;// max distance for next lod
float _fTreshold = 0;// treshold for next animation
float _fAnimSpeed = -1;
BOOL bCompresion = FALSE;// is animation is using compresions
#line 67 "seriousskastudio/parser.y"

#define YYERROR_VERBOSE 1                  

// if error occurs in parsing
void yyerror(const char *strFormat, ...)
{
  va_list arg;
  va_start(arg, strFormat);
  CTString strError;
  strError.VPrintF(strFormat, arg);
  // throw the string
  ThrowF_t("File '%s' (line %d)\n%s",(const char*)strCurentFileName, _yy_iLine, (const char*)strError);
};


#line 85 "seriousskastudio/parser.y"
typedef union {
  INDEX i;
  float f;
  const char *str;
  float v2[2];
  float v3[3];
  int   i3[3];
  float f12[12];
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		427
#define	YYFLAG		-32768
#define	YYNTBASE	58

#define YYTRANSLATE(x) ((unsigned)(x) <= 308 ? yytranslate[x] : 184)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    57,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    56,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    54,     2,    55,     2,     2,     2,     2,     2,
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
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     6,     8,    10,    15,    16,    18,    20,
    23,    24,    28,    41,    46,    47,    49,    51,    54,    63,
    64,    67,    68,    72,    73,    77,    81,    91,    92,    99,
   100,   102,   104,   107,   108,   120,   122,   125,   128,   129,
   136,   137,   139,   141,   144,   145,   152,   154,   157,   160,
   163,   166,   171,   172,   174,   176,   179,   184,   188,   189,
   196,   197,   199,   201,   204,   205,   217,   222,   225,   226,
   228,   229,   230,   243,   244,   246,   248,   251,   260,   268,
   269,   277,   278,   280,   282,   285,   288,   289,   297,   298,
   300,   302,   305,   308,   309,   317,   318,   320,   322,   325,
   328,   329,   337,   338,   340,   342,   345,   348,   349,   351,
   355,   359,   360,   363,   364,   367,   368,   375,   377,   380,
   383,   384,   391,   393,   396,   399,   400,   407,   408,   410,
   412,   415,   416,   428,   430,   433,   436,   437,   444,   446,
   449,   450,   462,   464,   467,   470,   471,   478,   479,   481,
   483,   486,   487,   499,   501,   504,   511,   512,   519,   520,
   522,   524,   527,   528,   542,   544,   547,   564,   567,   569,
   571,   573,   577,   583,   589,   595,   619,   622
};

static const short yyrhs[] = {    63,
     0,    90,     0,    66,     0,    64,     0,    59,     0,    36,
    54,    60,    55,     0,     0,    61,     0,    63,     0,    61,
    63,     0,     0,    35,   176,    56,     0,    62,   103,   133,
   134,   135,   136,   140,   144,   152,   159,   167,   175,     0,
    33,    54,    65,    55,     0,     0,    66,     0,    67,     0,
    66,    67,     0,    69,    68,    70,    71,    72,    73,    81,
    89,     0,     0,    39,   183,     0,     0,    38,   176,    56,
     0,     0,    41,   176,    56,     0,    26,     3,    56,     0,
    27,     3,    56,    28,     4,    56,    34,     5,    56,     0,
     0,    29,     4,    74,    54,    75,    55,     0,     0,    76,
     0,    77,     0,    75,    77,     0,     0,    13,     5,    31,
    54,   182,    56,    78,    55,    54,    79,    55,     0,    80,
     0,    79,    80,     0,   182,    56,     0,     0,    30,     4,
    82,    54,    83,    55,     0,     0,    84,     0,    85,     0,
    83,    85,     0,     0,    13,     5,    86,    54,    87,    55,
     0,    88,     0,    87,    88,     0,     3,    56,     0,     4,
    56,     0,    32,    56,     0,    37,    54,    91,    55,     0,
     0,    92,     0,    93,     0,    92,    93,     0,    62,    94,
    95,   102,     0,     7,     3,    56,     0,     0,     9,     4,
    54,    96,    97,    55,     0,     0,    98,     0,    99,     0,
    97,    99,     0,     0,    13,     5,    56,     8,     5,    56,
    40,   176,   100,    56,   101,     0,    54,   182,    56,    55,
     0,    25,    56,     0,     0,   104,     0,     0,     0,    42,
   176,    56,    44,     4,    54,   105,   107,    55,    56,   106,
    43,     0,     0,   108,     0,   109,     0,   108,   109,     0,
   110,   111,   116,   121,   126,   131,    55,    56,     0,    45,
     5,    56,    54,    46,     5,    56,     0,     0,    47,     4,
   112,    54,   113,    55,    56,     0,     0,   114,     0,   115,
     0,   114,   115,     0,     5,    56,     0,     0,    48,     4,
   117,    54,   118,    55,    56,     0,     0,   119,     0,   120,
     0,   119,   120,     0,     4,    56,     0,     0,    49,     4,
   122,    54,   123,    55,    56,     0,     0,   124,     0,   125,
     0,   124,   125,     0,     4,    56,     0,     0,    50,     4,
   127,    54,   128,    55,    56,     0,     0,   129,     0,   130,
     0,   129,   130,     0,   176,    56,     0,     0,   132,     0,
    51,     4,    56,     0,     6,     3,    56,     0,     0,    52,
   183,     0,     0,    53,   183,     0,     0,    10,     4,    54,
   137,   138,    55,     0,   139,     0,   138,   139,     0,   179,
    56,     0,     0,    11,     4,    54,   141,   142,    55,     0,
   143,     0,   142,   143,     0,   180,    56,     0,     0,    12,
     4,    54,   145,   146,    55,     0,     0,   147,     0,   148,
     0,   147,   148,     0,     0,    54,    13,     5,    56,    14,
     4,    54,   149,   150,    55,    55,     0,   151,     0,   150,
   151,     0,   178,    56,     0,     0,    15,     4,    54,   153,
   154,    55,     0,   155,     0,   154,   155,     0,     0,    54,
    13,     5,    56,    16,     4,    54,   156,   157,    55,    55,
     0,   158,     0,   157,   158,     0,   181,    56,     0,     0,
    17,     4,    54,   160,   161,    55,     0,     0,   162,     0,
   163,     0,   161,   163,     0,     0,    54,    13,     5,    56,
    18,     4,    54,   164,   165,    55,    55,     0,   166,     0,
   165,   166,     0,    54,     4,    56,   176,    56,    55,     0,
     0,    19,     4,    54,   168,   169,    55,     0,     0,   170,
     0,   171,     0,   169,   171,     0,     0,    54,    13,     5,
    56,    20,   183,    23,     4,   172,    54,   173,    55,    55,
     0,   174,     0,   173,   174,     0,    54,     4,    56,   176,
    57,   176,    57,   176,    56,   176,    57,   176,    57,   176,
    56,    55,     0,    24,    56,     0,     3,     0,     4,     0,
     4,     0,   176,    57,   176,     0,   176,    57,   176,    57,
   176,     0,   176,    57,   176,    57,   176,     0,   177,    57,
   177,    57,   177,     0,   176,    57,   176,    57,   176,    57,
   176,    57,   176,    57,   176,    57,   176,    57,   176,    57,
   176,    57,   176,    57,   176,    57,   176,     0,    21,    56,
     0,    22,    56,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   168,   169,   170,   171,   172,   180,   184,   185,   189,   190,
   194,   198,   205,   214,   218,   219,   223,   224,   228,   232,
   234,   238,   240,   245,   249,   256,   272,   302,   319,   328,
   329,   333,   334,   338,   366,   383,   384,   388,   407,   420,
   429,   430,   434,   435,   439,   453,   469,   470,   474,   487,
   503,   512,   516,   517,   521,   522,   526,   530,   537,   559,
   568,   569,   573,   574,   578,   592,   596,   612,   621,   628,
   632,   640,   644,   647,   648,   652,   653,   656,   663,   673,
   679,   690,   691,   694,   695,   699,   714,   720,   731,   732,
   735,   736,   739,   754,   760,   771,   772,   775,   776,   779,
   794,   801,   812,   813,   816,   817,   820,   835,   836,   839,
   847,   861,   865,   875,   879,   890,   899,   908,   909,   913,
   927,   940,   949,   950,   954,   968,   978,   987,   988,   992,
   993,   997,  1016,  1026,  1027,  1031,  1047,  1064,  1073,  1074,
  1078,  1120,  1130,  1131,  1135,  1152,  1162,  1171,  1172,  1176,
  1177,  1181,  1198,  1208,  1209,  1213,  1235,  1246,  1255,  1256,
  1259,  1260,  1264,  1282,  1292,  1293,  1297,  1320,  1325,  1329,
  1335,  1341,  1348,  1356,  1364,  1372,  1389,  1393
};

static const char * const yytname[] = {   "$","error","$undefined.","c_float",
"c_int","c_string","k_SE_MESH","k_SE_SKELETON","k_PARENT","k_BONES","k_VERTICES",
"k_NORMALS","k_UVMAPS","k_NAME","k_TEXCOORDS","k_SURFACES","k_TRIANGLE_SET",
"k_WEIGHTS","k_WEIGHT_SET","k_MORPHS","k_RELATIVE","k_TRUE","k_FALSE","k_MORPH_SET",
"k_SE_MESH_END","k_SE_SKELETON_END","k_SE_ANIM","k_SEC_PER_FRAME","k_FRAMES",
"k_BONEENVELOPES","k_MORPHENVELOPES","k_DEFAULT_POSE","k_SE_ANIM_END","k_ANIM_SET_LIST",
"k_ANIM_ID","k_MAX_DISTANCE","k_MESHLODLIST","k_SKELETONLODLIST","k_TRESHOLD",
"k_COMPRESION","k_LENGTH","k_ANIMSPEED","k_SHADER_PARAMS","k_SHADER_PARAMS_END",
"k_SHADER_SURFACES","k_SHADER_SURFACE","k_SHADER_NAME","k_SHADER_TEXTURES","k_SHADER_UVMAPS",
"k_SHADER_COLORS","k_SHADER_FLOATS","k_SHADER_FLAGS","k_FULL_FACE_FORWARD","k_HALF_FACE_FORWARD",
"'{'","'}'","';'","','","program","meshlod_list","meshlod_array_opt","meshlod_array",
"max_distance","meshlod","animset_list","animset_array_opt","animset_array",
"animset","compresion_opt","treshold_opt","animspeed_opt","animset_begin","animation_header",
"bone_envelopes","@1","bone_env_header_opt","bone_env_header","bone_envelope",
"@2","bone_env_array","bone_env_m","morph_envelopes","@3","morph_env_header",
"morph_env_header_notnull","morph_env","@4","morph_env_array","morph_env_i",
"animset_end","skeletonlod_list","opt_skeletonlod_array","skeletonlod_array",
"skeletonlod","skeleton_begin","bones","@5","bone_headers","bone_headers_notnull",
"bone_header","@6","bone","skeleton_end","shader_params_opt","shader_params",
"@7","@8","surface_shader_params_array_opt","surface_shader_params_array","surface_shader_params",
"shader_name","shader_textures","@9","shader_texture_array_opt","shader_texture_array",
"shader_texture","shader_uvmaps","@10","shader_uvmaps_array_opt","shader_uvmaps_array",
"shader_uvmap","shader_colors","@11","shader_colors_array_opt","shader_colors_array",
"shader_color","shader_floats","@12","shader_floats_array_opt","shader_floats_array",
"shader_float","shader_flag_opt","shader_flags","mesh_begin","is_full_faceforward_opt",
"is_half_faceforward_opt","vertices","@13","vert3_array","vert3_array_value",
"normals","@14","norm3_array","norm3_array_value","uvmaps","@15","uvmaps_opt_array",
"uvmaps_array","uvmap","@16","uv2_array","uv2_array_value","surfaces","@17",
"surfaces_array","triangleset","@18","triangle_array","triangle_array_value",
"weights","@19","weights_array","weights_array_notnull","weightset","@20","weight_map_array",
"weight_map_array_value","morphs","@21","morphs_array","morphs_array_notnull",
"morphset","@22","morph_set_array","morph_set_array_value","mesh_end","float_const",
"int_const","uv2","vert3","norm3","triangle","matrix","boolean",""
};
#endif

static const short yyr1[] = {     0,
    58,    58,    58,    58,    58,    59,    60,    60,    61,    61,
    62,    62,    63,    64,    65,    65,    66,    66,    67,    68,
    68,    69,    69,    70,    70,    71,    72,    74,    73,    75,
    75,    76,    76,    78,    77,    79,    79,    80,    82,    81,
    83,    83,    84,    84,    86,    85,    87,    87,    88,    88,
    89,    90,    91,    91,    92,    92,    93,    94,    96,    95,
    97,    97,    98,    98,   100,    99,   101,   102,   103,   103,
   105,   106,   104,   107,   107,   108,   108,   109,   110,   112,
   111,   113,   113,   114,   114,   115,   117,   116,   118,   118,
   119,   119,   120,   122,   121,   123,   123,   124,   124,   125,
   127,   126,   128,   128,   129,   129,   130,   131,   131,   132,
   133,   134,   134,   135,   135,   137,   136,   138,   138,   139,
   141,   140,   142,   142,   143,   145,   144,   146,   146,   147,
   147,   149,   148,   150,   150,   151,   153,   152,   154,   154,
   156,   155,   157,   157,   158,   160,   159,   161,   161,   162,
   162,   164,   163,   165,   165,   166,   168,   167,   169,   169,
   170,   170,   172,   171,   173,   173,   174,   175,   176,   176,
   177,   178,   179,   180,   181,   182,   183,   183
};

static const short yyr2[] = {     0,
     1,     1,     1,     1,     1,     4,     0,     1,     1,     2,
     0,     3,    12,     4,     0,     1,     1,     2,     8,     0,
     2,     0,     3,     0,     3,     3,     9,     0,     6,     0,
     1,     1,     2,     0,    11,     1,     2,     2,     0,     6,
     0,     1,     1,     2,     0,     6,     1,     2,     2,     2,
     2,     4,     0,     1,     1,     2,     4,     3,     0,     6,
     0,     1,     1,     2,     0,    11,     4,     2,     0,     1,
     0,     0,    12,     0,     1,     1,     2,     8,     7,     0,
     7,     0,     1,     1,     2,     2,     0,     7,     0,     1,
     1,     2,     2,     0,     7,     0,     1,     1,     2,     2,
     0,     7,     0,     1,     1,     2,     2,     0,     1,     3,
     3,     0,     2,     0,     2,     0,     6,     1,     2,     2,
     0,     6,     1,     2,     2,     0,     6,     0,     1,     1,
     2,     0,    11,     1,     2,     2,     0,     6,     1,     2,
     0,    11,     1,     2,     2,     0,     6,     0,     1,     1,
     2,     0,    11,     1,     2,     6,     0,     6,     0,     1,
     1,     2,     0,    13,     1,     2,    16,     2,     1,     1,
     1,     3,     5,     5,     5,    23,     2,     2
};

static const short yydefact[] = {    22,
     0,     0,     0,     0,     0,     5,    69,     1,     4,    22,
    17,    20,     2,    22,   169,   170,     0,    11,    11,     0,
     0,     0,    70,    18,     0,    24,     0,    22,    12,     0,
    11,     9,     0,     0,    11,    55,    23,     0,     0,   112,
     0,     0,    21,     0,     0,    14,     6,    10,     0,     0,
    52,    56,     0,     0,     0,   114,   177,   178,     0,     0,
     0,     0,     0,     0,     0,   111,   113,     0,     0,    25,
     0,     0,     0,    58,     0,     0,    57,     0,   115,     0,
     0,    26,     0,     0,     0,    59,    68,    71,     0,     0,
     0,     0,    28,     0,     0,    61,    74,   116,     0,     0,
     0,     0,     0,    39,     0,    19,     0,     0,    62,    63,
     0,     0,    75,    76,     0,     0,   121,     0,     0,     0,
     0,    30,     0,    51,     0,    60,    64,     0,     0,    77,
     0,     0,     0,   118,     0,     0,     0,   126,     0,     0,
     0,     0,     0,     0,    31,    32,    41,     0,     0,    72,
    80,     0,     0,   117,   119,     0,   120,     0,   123,     0,
     0,   128,   137,     0,     0,     0,     0,     0,    29,    33,
     0,     0,    42,    43,     0,     0,     0,     0,    87,     0,
     0,     0,   122,   124,     0,   125,     0,     0,   129,   130,
     0,   146,     0,     0,    13,     0,     0,    45,    40,    44,
     0,     0,    73,    82,     0,    94,     0,   108,     0,     0,
     0,   127,   131,     0,     0,   139,   148,   157,   168,    27,
     0,     0,     0,     0,     0,     0,    83,    84,    89,     0,
   101,     0,     0,   109,   173,     0,     0,     0,   138,   140,
     0,     0,   149,   150,   159,     0,     0,     0,     0,    79,
    86,     0,    85,     0,     0,    90,    91,    96,     0,     0,
     0,   174,     0,     0,     0,   147,   151,     0,     0,   160,
   161,     0,    34,     0,     0,     0,    47,    65,    81,    93,
     0,    92,     0,     0,    97,    98,   103,   110,    78,     0,
     0,     0,     0,   158,   162,     0,     0,    49,    50,    46,
    48,     0,    88,   100,     0,    99,     0,   104,   105,     0,
     0,     0,     0,     0,     0,     0,     0,    95,     0,   106,
   107,   132,     0,     0,     0,     0,     0,     0,    66,   102,
     0,   141,     0,     0,     0,     0,    36,     0,     0,     0,
   134,     0,     0,     0,   152,     0,     0,    35,    37,    38,
     0,     0,   135,     0,   136,   171,     0,   143,     0,     0,
     0,     0,     0,    67,   133,   172,     0,   144,     0,   145,
     0,     0,   154,   163,     0,   142,     0,     0,     0,   155,
     0,     0,     0,     0,   153,     0,     0,   175,     0,     0,
     0,   165,     0,     0,     0,     0,   166,     0,   156,     0,
   164,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   176,
     0,     0,     0,   167,     0,     0,     0
};

static const short yydefgoto[] = {   425,
     6,    30,    31,     7,     8,     9,    27,    10,    11,    26,
    12,    45,    61,    73,    85,   103,   144,   145,   146,   297,
   336,   337,    95,   123,   172,   173,   174,   222,   276,   277,
   106,    13,    34,    35,    36,    50,    64,    96,   108,   109,
   110,   302,   329,    77,    22,    23,    97,   177,   112,   113,
   114,   115,   132,   178,   226,   227,   228,   153,   205,   255,
   256,   257,   181,   230,   284,   285,   286,   208,   259,   307,
   308,   309,   233,   234,    40,    56,    69,    81,   116,   133,
   134,    91,   137,   158,   159,   101,   162,   188,   189,   190,
   331,   340,   341,   120,   191,   215,   216,   344,   357,   358,
   141,   217,   242,   243,   244,   361,   372,   373,   166,   245,
   269,   270,   271,   381,   391,   392,   195,   246,   359,   343,
   136,   161,   360,   338,    43
};

static const short yypact[] = {    34,
   -26,    30,   -23,    -1,    30,-32768,    15,-32768,-32768,    16,
-32768,    22,-32768,   -18,-32768,-32768,    28,   -17,   -11,    29,
    30,    49,-32768,-32768,    14,    42,    20,   -16,-32768,    31,
   -10,-32768,    61,    32,    -9,-32768,-32768,    33,    85,    38,
    35,    36,-32768,    30,    67,-32768,-32768,-32768,    91,    86,
-32768,-32768,    52,    41,    14,    45,-32768,-32768,    43,    97,
    74,    46,    99,    79,   101,-32768,-32768,    14,    96,-32768,
    51,   105,    81,-32768,    57,    56,-32768,    59,-32768,   111,
   106,-32768,    60,   114,    89,-32768,-32768,-32768,    66,   117,
   110,    95,-32768,   120,    93,   113,    82,-32768,    75,   124,
   115,   128,    80,-32768,    77,-32768,   131,     4,-32768,-32768,
   132,    83,    82,-32768,    92,    30,-32768,    87,   136,   125,
    88,   130,    94,-32768,    90,-32768,-32768,   102,   103,-32768,
   141,   104,     1,-32768,    98,   107,    30,-32768,   108,   143,
   134,   116,   144,     8,-32768,-32768,   138,   149,   112,-32768,
-32768,   156,   118,-32768,-32768,    30,-32768,     3,-32768,   119,
   109,   121,-32768,   123,   157,   140,   163,   139,-32768,-32768,
   164,    10,-32768,-32768,   166,   126,   135,   127,-32768,   169,
   129,   133,-32768,-32768,    30,-32768,   161,   137,   121,-32768,
   142,-32768,   145,   146,-32768,   147,   150,-32768,-32768,-32768,
   152,   175,-32768,   177,   151,-32768,   180,   155,    30,   153,
   181,-32768,-32768,   172,    -5,-32768,   158,-32768,-32768,-32768,
    30,   159,   148,   160,   162,   154,   177,-32768,   183,   165,
-32768,   185,   167,-32768,-32768,    30,   168,   186,-32768,-32768,
   182,    -3,-32768,-32768,   171,   170,   173,    70,    30,-32768,
-32768,   174,-32768,   176,   178,   183,-32768,   189,   184,   179,
   187,-32768,   197,   188,   192,-32768,-32768,   201,    23,-32768,
-32768,    30,-32768,   190,   193,     5,-32768,-32768,-32768,-32768,
   194,-32768,   195,   198,   189,-32768,    30,-32768,-32768,   196,
   199,   200,   212,-32768,-32768,   191,   202,-32768,-32768,-32768,
-32768,   203,-32768,-32768,   204,-32768,   206,    30,-32768,   207,
   208,   216,   205,   209,    30,   210,   213,-32768,   215,-32768,
-32768,-32768,   214,   217,   211,   218,    30,    30,-32768,-32768,
    30,-32768,   219,    14,    30,     7,-32768,   220,   221,     9,
-32768,   222,   225,   224,-32768,   229,   226,-32768,-32768,-32768,
   223,   227,-32768,    30,-32768,-32768,    11,-32768,   230,   228,
   232,   233,    30,-32768,-32768,-32768,   234,-32768,   224,-32768,
   235,    25,-32768,-32768,   231,-32768,   236,   238,   237,-32768,
   241,    30,   224,    30,-32768,   242,   240,-32768,   243,   250,
    27,-32768,    30,   245,   246,   248,-32768,   244,-32768,    30,
-32768,    30,   247,   251,    30,    30,   252,   253,    30,    30,
   249,   254,    30,    30,   255,   257,    30,    30,   258,-32768,
    30,   260,   262,-32768,   266,   269,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,    13,    12,-32768,-32768,   276,    19,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    50,-32768,
-32768,  -138,-32768,-32768,-32768,-32768,    54,-32768,-32768,   -75,
-32768,-32768,-32768,-32768,   239,-32768,-32768,-32768,-32768,-32768,
   256,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   259,-32768,-32768,-32768,-32768,-32768,    18,-32768,-32768,-32768,
-32768,   -20,-32768,-32768,-32768,-32768,   -45,-32768,-32768,-32768,
-32768,   -67,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   122,-32768,-32768,-32768,    84,-32768,-32768,-32768,-32768,    69,
-32768,-32768,   -68,-32768,-32768,-32768,    76,-32768,-32768,   -59,
-32768,-32768,-32768,-32768,    65,-32768,-32768,   -53,-32768,-32768,
-32768,-32768,    53,-32768,-32768,   -73,-32768,    -2,  -342,-32768,
-32768,-32768,-32768,  -219,   -54
};


#define	YYLAST		419


static const short yytable[] = {    17,
    67,   247,    20,    15,    16,    15,    16,   274,   275,    15,
    16,    15,    16,    79,   356,    -3,   107,     2,    38,     5,
   143,     5,   171,     2,     2,     2,   377,    14,    24,    32,
    18,    33,    15,    16,    41,    42,   -15,    -7,   -16,   -11,
   388,    59,    48,   -53,    -8,   -54,    24,    33,   214,   239,
   241,   266,    19,     5,    39,   154,    21,   183,   126,   300,
    25,   348,   169,   352,   199,   367,     1,    49,     2,     3,
     4,     5,   274,   275,    46,   -11,   268,   294,   371,   379,
   390,   396,    44,    29,    37,    47,    51,    54,    53,    55,
    57,    58,    60,    62,    63,    65,    66,    68,    70,    71,
    72,    74,    75,    76,    78,    80,    82,    83,   339,    84,
    86,    87,    88,   135,    89,    92,    90,    93,    94,    98,
    99,   100,   102,   104,   105,   107,   111,   118,   117,   119,
   135,   121,   124,   122,   160,   125,   128,   129,   131,   139,
   138,   140,   143,   142,   151,   148,   164,   147,   168,   167,
   171,   152,   165,   182,   156,   160,   175,   149,   150,   179,
   193,   163,   157,   194,   186,   176,   180,   196,   198,   197,
   201,   202,   206,   211,   187,   185,   192,   203,   207,   224,
   204,   225,   210,   231,   238,   237,   254,   249,   260,   209,
   264,   212,   283,   170,   265,   214,   292,   349,   218,   311,
   301,   219,   220,   221,   229,   232,   235,   223,   252,   236,
   290,   241,   248,   293,   312,   250,   314,   251,   258,   323,
   333,   261,   324,   263,   268,   200,   272,   356,   273,   279,
   334,   280,   281,   262,   288,   282,   374,   287,   378,   306,
   320,   184,   289,   291,   253,   298,   278,   315,   299,   303,
   304,   362,   305,   395,   155,   313,   316,   213,   317,   318,
   319,   322,   321,   327,   325,   426,   328,   332,   427,   296,
   330,   353,   345,    52,   335,   350,   351,   364,   354,   346,
   355,   365,   363,   370,   310,   371,   369,   382,   376,    28,
   240,   385,   383,   384,   386,   390,   393,   368,   394,   399,
   402,   400,   401,   405,   413,   310,   267,   406,   409,   410,
   414,   417,   326,   418,   421,   423,   424,   397,   380,     0,
     0,   295,     0,     0,     0,     0,     0,     0,   342,     0,
     0,     0,   347,     0,     0,     0,     0,   342,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   366,     0,     0,     0,     0,     0,     0,     0,     0,
   375,     0,     0,   127,     0,     0,     0,     0,     0,     0,
     0,   130,     0,     0,     0,     0,     0,     0,     0,   387,
     0,   389,     0,     0,     0,     0,     0,     0,     0,     0,
   398,     0,     0,     0,     0,     0,     0,   403,     0,   404,
     0,     0,   407,   408,     0,     0,   411,   412,     0,     0,
   415,   416,     0,     0,   419,   420,     0,     0,   422
};

static const short yycheck[] = {     2,
    55,   221,     5,     3,     4,     3,     4,     3,     4,     3,
     4,     3,     4,    68,     4,     0,    13,    35,    21,    38,
    13,    38,    13,    35,    35,    35,   369,    54,    10,    18,
    54,    19,     3,     4,    21,    22,    55,    55,    55,     6,
   383,    44,    31,    55,    55,    55,    28,    35,    54,    55,
    54,    55,    54,    38,     6,    55,    42,    55,    55,    55,
    39,    55,    55,    55,    55,    55,    33,     7,    35,    36,
    37,    38,     3,     4,    55,    42,    54,    55,    54,    55,
    54,    55,    41,    56,    56,    55,    55,     3,    56,    52,
    56,    56,    26,     3,     9,    44,    56,    53,    56,     3,
    27,    56,     4,    25,     4,    10,    56,     3,   328,    29,
    54,    56,    54,   116,     4,    56,    11,     4,    30,    54,
     4,    12,    28,     4,    32,    13,    45,     4,    54,    15,
   133,     4,    56,    54,   137,     5,     5,    55,    47,     4,
    54,    17,    13,    56,     4,    56,     4,    54,     5,    34,
    13,    48,    19,   156,    57,   158,     8,    56,    56,     4,
     4,    54,    56,    24,    56,    54,    49,     5,     5,    31,
     5,    46,     4,    13,    54,    57,    54,    43,    50,     5,
    54,     5,   185,     4,    13,     5,     4,    40,     4,    57,
     5,    55,     4,   144,    13,    54,     5,   336,    54,     4,
   276,    56,    56,    54,    54,    51,   209,    56,    55,    57,
    14,    54,    54,    13,    16,    56,     5,    56,    54,     4,
     4,    55,    18,    56,    54,   172,    57,     4,    56,    56,
    20,    56,    55,   236,    56,   256,     4,    54,     4,   285,
   308,   158,    56,    56,   227,    56,   249,    57,    56,    56,
    56,    23,    55,     4,   133,    56,    55,   189,    56,    56,
    55,    54,    56,    54,    56,     0,    54,    54,     0,   272,
    56,   340,    54,    35,    57,    56,    56,    55,    57,   334,
    56,    55,    57,    56,   287,    54,    57,    57,    55,    14,
   215,    55,    57,    56,    54,    54,    57,   357,    56,    55,
    57,    56,    55,    57,    56,   308,   242,    57,    57,    57,
    57,    57,   315,    57,    57,    56,    55,   391,   372,    -1,
    -1,   269,    -1,    -1,    -1,    -1,    -1,    -1,   331,    -1,
    -1,    -1,   335,    -1,    -1,    -1,    -1,   340,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,   354,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
   363,    -1,    -1,   108,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,   113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   382,
    -1,   384,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
   393,    -1,    -1,    -1,    -1,    -1,    -1,   400,    -1,   402,
    -1,    -1,   405,   406,    -1,    -1,   409,   410,    -1,    -1,
   413,   414,    -1,    -1,   417,   418,    -1,    -1,   421
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#undef YYERROR_VERBOSE
#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 192 "bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#else
#define YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#endif

int
yyparse(YYPARSE_PARAM)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 11:
#line 195 "seriousskastudio/parser.y"
{
  // fill with default values
;
    break;}
case 12:
#line 199 "seriousskastudio/parser.y"
{
  _fCurentMaxDistance = yyvsp[-1].f;
;
    break;}
case 20:
#line 233 "seriousskastudio/parser.y"
{ bCompresion = FALSE; ;
    break;}
case 21:
#line 235 "seriousskastudio/parser.y"
{ bCompresion  = yyvsp[0].i; ;
    break;}
case 22:
#line 239 "seriousskastudio/parser.y"
{ _fTreshold = 0.0f; ;
    break;}
case 23:
#line 241 "seriousskastudio/parser.y"
{ _fTreshold = yyvsp[-1].f; ;
    break;}
case 24:
#line 246 "seriousskastudio/parser.y"
{
  _fAnimSpeed = -1;
;
    break;}
case 25:
#line 250 "seriousskastudio/parser.y"
{
  _fAnimSpeed = yyvsp[-1].f;
;
    break;}
case 26:
#line 257 "seriousskastudio/parser.y"
{
  // get curent animation count
  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  // increase curent animation count
  _yy_pAnimSet->as_Anims.Expand(ctan+1);
  // get ref to new created animation
  Animation &an = _yy_pAnimSet->as_Anims[ctan];
  // set animation allready read treshold
  an.an_fTreshold = _fTreshold;
  // set animation allready read compresion flag
  an.an_bCompresed =  bCompresion;
;
    break;}
case 27:
#line 273 "seriousskastudio/parser.y"
{
  _ctFrames = yyvsp[-4].i;
  if(_ctFrames<0) {
    yyerror("Negative frame count found %d",_ctFrames);
  }

  // get current animset count
  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  // get ref to last animation
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  // set animations seconds per frame
  an.an_fSecPerFrame = yyvsp[-7].f;
  // set animation frame count
  an.an_iFrames = _ctFrames;
  // set animation ID
  an.an_iID = ska_GetIDFromStringTable(yyvsp[-1].str);
  // set animation source file (current file)
  an.an_fnSourceFile = strCurentFileName;
  // set animation custom speed (if any)
  an.an_bCustomSpeed = FALSE;
  if(_fAnimSpeed>=0)
  {
    an.an_bCustomSpeed = TRUE;
    an.an_fSecPerFrame = _fAnimSpeed;
  }
;
    break;}
case 28:
#line 303 "seriousskastudio/parser.y"
{
  // set bone envelopes count
  _ctBoneEnvelopes = yyvsp[0].i;

  if(_ctBoneEnvelopes<0) {
    yyerror("Negative bone envelope count found %d",_ctBoneEnvelopes);
  }

  // count animations
  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  // add new bone envelope array
  an.an_abeBones.New(_ctBoneEnvelopes);
  // set current bone envelope index
  _iBoneEnvelope = 0;
;
    break;}
case 29:
#line 320 "seriousskastudio/parser.y"
{
  if(_iBoneEnvelope!=_ctBoneEnvelopes) {
    yyerror("Incorect number of bone envelopes.\nExpecting %d but found only %d",_ctBoneEnvelopes,_iBoneEnvelope);
  }
;
    break;}
case 34:
#line 339 "seriousskastudio/parser.y"
{
  if(_iBoneEnvelope>=_ctBoneEnvelopes) {
    yyerror("Incorect number of bone envelopes - %d",_ctBoneEnvelopes);
  }
  
  // get last animation
  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  BoneEnvelope &be = an.an_abeBones[_iBoneEnvelope];

  // create array of bone posiotions and rotations
  be.be_apPos.New(_ctFrames);
  be.be_arRot.New(_ctFrames);
  // for each frame
  for(INDEX ifn=0;ifn<_ctFrames;ifn++)
  {
    // set bone position and rotation frame number
    be.be_apPos[ifn].ap_iFrameNum = ifn;
    be.be_arRot[ifn].ar_iFrameNum = ifn;
  }
  // get id of this bone envelope
  be.be_iBoneID = ska_GetIDFromStringTable(yyvsp[-4].str);
  // fill default pos matrix
  memcpy(&be.be_mDefaultPos,yyvsp[-1].f12,sizeof(float)*12);
  
  _iFrame = 0;
;
    break;}
case 35:
#line 367 "seriousskastudio/parser.y"
{
  _iBoneEnvelope++;
  if(_iFrame!=_ctFrames) {
    INDEX ctan = _yy_pAnimSet->as_Anims.Count();
    Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
    BoneEnvelope &be = an.an_abeBones[_iBoneEnvelope-1];
    CTString strBoneEnvelope = ska_GetStringFromTable(be.be_iBoneID);

     yyerror("Incorect number of bone envelope frames count for bone envelope '%s'.\nExpecting %d but found only %d",
             (const char*)strBoneEnvelope,_ctFrames,_iFrame);
  }
;
    break;}
case 38:
#line 389 "seriousskastudio/parser.y"
{
  if(_iFrame>=_ctFrames) {
    yyerror("Incorect number of bone envelope frames - %d",_ctFrames);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  QVect qvPlacement;
  Matrix12ToQVect(qvPlacement,yyvsp[-1].f12);
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  BoneEnvelope &be = an.an_abeBones[_iBoneEnvelope];

  be.be_apPos[_iFrame].ap_vPos = qvPlacement.vPos;
  be.be_arRot[_iFrame].ar_qRot = qvPlacement.qRot;
  _iFrame++;
;
    break;}
case 39:
#line 408 "seriousskastudio/parser.y"
{
  // set morph envelopes count
  _ctMorphEnvelopes = yyvsp[0].i;

  if(_ctMorphEnvelopes<0) {
    yyerror("Negative morph envelope count found %d",_ctMorphEnvelopes);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  _yy_pAnimSet->as_Anims[ctan-1].an_ameMorphs.New(_ctMorphEnvelopes);
  _iMorphEnvelope = 0;
;
    break;}
case 40:
#line 421 "seriousskastudio/parser.y"
{
  if(_iMorphEnvelope!=_ctMorphEnvelopes) {
    yyerror("Incorect number of morph envelopes.\nExpecting %d but found only %d",_ctMorphEnvelopes,_iMorphEnvelope);
  }
;
    break;}
case 45:
#line 440 "seriousskastudio/parser.y"
{
  if(_iMorphEnvelope>=_ctMorphEnvelopes) {
    yyerror("Incorect number of morph envelopes - %d",_ctMorphEnvelopes);
  }
    INDEX ctan = _yy_pAnimSet->as_Anims.Count();
    // get ref to animation and morph envelope
    Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
    MorphEnvelope &me = an.an_ameMorphs[_iMorphEnvelope];
    // create array for morphs factors
    me.me_aFactors.New(_ctFrames);
    me.me_iMorphMapID = ska_GetIDFromStringTable(yyvsp[0].str);
    _iFrame = 0;
;
    break;}
case 46:
#line 454 "seriousskastudio/parser.y"
{
  _iMorphEnvelope++;
  if(_iFrame!=_ctFrames) {
    INDEX ctan = _yy_pAnimSet->as_Anims.Count();
    Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
    MorphEnvelope &me = an.an_ameMorphs[_iMorphEnvelope-1];
    CTString strMorphEnvelope = ska_GetStringFromTable(me.me_iMorphMapID);

     yyerror("Incorect number of morph envelope frames count for morph envelope '%s'.\nExpecting %d but found only %d",
             (const char*)strMorphEnvelope,_ctFrames,_iFrame);
  }
;
    break;}
case 49:
#line 475 "seriousskastudio/parser.y"
{
  if(_iFrame>=_ctFrames) {
    yyerror("Incorect number of morph envelope frames - %d",_ctFrames);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  MorphEnvelope &me = an.an_ameMorphs[_iMorphEnvelope];
  me.me_aFactors[_iFrame] = yyvsp[-1].f;

  _iFrame++;
;
    break;}
case 50:
#line 488 "seriousskastudio/parser.y"
{
  if(_iFrame>=_ctFrames) {
    yyerror("Incorect number of morph envelope frames - %d",_ctFrames);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  MorphEnvelope &me = an.an_ameMorphs[_iMorphEnvelope];
  me.me_aFactors[_iFrame] = yyvsp[-1].i;

  _iFrame++;
;
    break;}
case 58:
#line 531 "seriousskastudio/parser.y"
{
  // check for version of skeleton ascii file
;
    break;}
case 59:
#line 538 "seriousskastudio/parser.y"
{
  _ctBones = yyvsp[-1].i;
  if(_ctBones<0) {
    yyerror("Negative bone count found %d",_ctBones);
  }

  // set bone index to 0
  _iBone = 0;
  // get skeleton lods count
  INDEX ctskllod = _yy_pSkeleton->skl_aSkeletonLODs.Count();
  // increase skeleton lods count
  _yy_pSkeleton->skl_aSkeletonLODs.Expand(ctskllod+1);
  // get ref to new skeleton created
  pSkeletonLOD = &_yy_pSkeleton->skl_aSkeletonLODs[ctskllod];
  // read source file name of skeleton ascii file
  pSkeletonLOD->slod_fnSourceFile = strCurentFileName;
  // get allready remembered max distance
  pSkeletonLOD->slod_fMaxDistance = _fCurentMaxDistance;
  // create new array for bones
  pSkeletonLOD->slod_aBones.New(_ctBones);
;
    break;}
case 60:
#line 560 "seriousskastudio/parser.y"
{
  if(_iBone!=_ctBones) {
    yyerror("Incorect number of bones.\nExpecting %d but found only %d",_ctBones,_iBone);
  }
;
    break;}
case 65:
#line 579 "seriousskastudio/parser.y"
{
  if(_iBone>=_ctBones) {
    yyerror("Incorect number of bones - %d",_ctBones);
  }

  SkeletonBone &sb = pSkeletonLOD->slod_aBones[_iBone];
  // get bone ID
  sb.sb_iID = ska_GetIDFromStringTable(yyvsp[-6].str);
  // get parent bone ID
  sb.sb_iParentID = ska_GetIDFromStringTable(yyvsp[-3].str);
  // get bone length
  sb.sb_fBoneLength = yyvsp[0].f;
;
    break;}
case 67:
#line 597 "seriousskastudio/parser.y"
{
  SkeletonBone &sb = pSkeletonLOD->slod_aBones[_iBone];
  // convert matrix to qvect struct
  Matrix12ToQVect(sb.sb_qvRelPlacement,yyvsp[-2].f12);
  // copy matrix to bone as its absolute placement
  memcpy(&sb.sb_mAbsPlacement,yyvsp[-2].f12,sizeof(float)*12);
  // calculate bone offset length
  FLOAT3D vOffset = FLOAT3D(yyvsp[-2].f12[3],yyvsp[-2].f12[7],yyvsp[-2].f12[11]);
  sb.sb_fOffSetLen = vOffset.Length();
  // increase bone count
  _iBone++;
;
    break;}
case 68:
#line 613 "seriousskastudio/parser.y"
{
  // end of skeleton parsing
;
    break;}
case 69:
#line 622 "seriousskastudio/parser.y"
{
  // no shader for next surface
  _assSurfaceShaders.Clear();
  _ctShaderSurfaces=0;
  _iShaderSurfIndex=0;
;
    break;}
case 71:
#line 633 "seriousskastudio/parser.y"
{
  _assSurfaceShaders.Clear();
  _ctShaderSurfaces = yyvsp[-1].i;
  // create array of shader params for each surface in mesh file
  _assSurfaceShaders.New(_ctShaderSurfaces);
  // reset surface index
  _iShaderSurfIndex=0;
;
    break;}
case 72:
#line 641 "seriousskastudio/parser.y"
{
  // check if surfaces count match count of read surfaces
  if(_iShaderSurfIndex!=_ctShaderSurfaces) yyerror("Incorect number of surfaces");
;
    break;}
case 78:
#line 657 "seriousskastudio/parser.y"
{
  _iShaderSurfIndex++;
;
    break;}
case 79:
#line 664 "seriousskastudio/parser.y"
{
  if(_iShaderSurfIndex>=_ctShaderSurfaces) yyerror("Incorect number of surfaces");
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_iSurfaceID = ska_GetIDFromStringTable(yyvsp[-5].str);
  // set shader fn
  SurfShader.fnShaderName = (CTString)yyvsp[-1].str
;
    break;}
case 80:
#line 674 "seriousskastudio/parser.y"
{
  _ctshParamsMax = yyvsp[0].i;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_aiTextureIDs.New(_ctshParamsMax);
  _ishParamIndex = 0;
;
    break;}
case 81:
#line 680 "seriousskastudio/parser.y"
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect texture params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
;
    break;}
case 86:
#line 700 "seriousskastudio/parser.y"
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect texture params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  // set ID of current texture name
  SurfShader.ss_spShaderParams.sp_aiTextureIDs[_ishParamIndex] = ska_GetIDFromStringTable(yyvsp[-1].str);
  // increase shading parametar index
  _ishParamIndex++;
;
    break;}
case 87:
#line 715 "seriousskastudio/parser.y"
{
  _ctshParamsMax = yyvsp[0].i;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_aiTexCoordsIndex.New(_ctshParamsMax);
  _ishParamIndex = 0;
;
    break;}
case 88:
#line 721 "seriousskastudio/parser.y"
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect uvmap params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
;
    break;}
case 93:
#line 740 "seriousskastudio/parser.y"
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect uvmap params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  // set index of current uvmap name
  SurfShader.ss_spShaderParams.sp_aiTexCoordsIndex[_ishParamIndex] = yyvsp[-1].i;
  // increase shading parametar index
  _ishParamIndex++;
;
    break;}
case 94:
#line 755 "seriousskastudio/parser.y"
{
  _ctshParamsMax = yyvsp[0].i;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_acolColors.New(_ctshParamsMax);
  _ishParamIndex = 0;
;
    break;}
case 95:
#line 761 "seriousskastudio/parser.y"
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect color params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
;
    break;}
case 100:
#line 780 "seriousskastudio/parser.y"
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect colors params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  // set color
  SurfShader.ss_spShaderParams.sp_acolColors[_ishParamIndex] = yyvsp[-1].i;
  // increase color parametar index
  _ishParamIndex++;
;
    break;}
case 101:
#line 795 "seriousskastudio/parser.y"
{
  _ctshParamsMax = yyvsp[0].i;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_afFloats.New(_ctshParamsMax);
  _ishParamIndex = 0;

;
    break;}
case 102:
#line 802 "seriousskastudio/parser.y"
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect floats params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
;
    break;}
case 107:
#line 821 "seriousskastudio/parser.y"
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect floats params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  // set color
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_afFloats[_ishParamIndex] = yyvsp[-1].f;
  // increase floats parametar index
  _ishParamIndex++;
;
    break;}
case 110:
#line 840 "seriousskastudio/parser.y"
{
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_ulFlags = yyvsp[-1].i;
;
    break;}
case 111:
#line 848 "seriousskastudio/parser.y"
{
  if (yyvsp[-1].f!=0.1) NOTHING; //parseMesh->error
  INDEX ctmshlod = _yy_pMesh->msh_aMeshLODs.Count();
  _yy_pMesh->msh_aMeshLODs.Expand(ctmshlod+1);
  pMeshLOD = &_yy_pMesh->msh_aMeshLODs[ctmshlod];
  pMeshLOD->mlod_fnSourceFile = strCurentFileName;
  pMeshLOD->mlod_fMaxDistance = _fCurentMaxDistance;
  _yy_iWIndex=0;
  _yy_iMIndex=0;
;
    break;}
case 112:
#line 862 "seriousskastudio/parser.y"
{
  pMeshLOD->mlod_ulFlags &= ~ML_FULL_FACE_FORWARD;
;
    break;}
case 113:
#line 866 "seriousskastudio/parser.y"
{
  if(yyvsp[0].i == TRUE) {
    pMeshLOD->mlod_ulFlags|=ML_FULL_FACE_FORWARD;
  } else {
    pMeshLOD->mlod_ulFlags &=~ML_FULL_FACE_FORWARD;
  }
;
    break;}
case 114:
#line 876 "seriousskastudio/parser.y"
{
  pMeshLOD->mlod_ulFlags &= ~ML_HALF_FACE_FORWARD;
;
    break;}
case 115:
#line 880 "seriousskastudio/parser.y"
{
  if(yyvsp[0].i == TRUE) {
    pMeshLOD->mlod_ulFlags|=ML_HALF_FACE_FORWARD;
  } else {
    pMeshLOD->mlod_ulFlags &=~ML_HALF_FACE_FORWARD;
  }
;
    break;}
case 116:
#line 890 "seriousskastudio/parser.y"
{
  _ctVertices = yyvsp[-1].i;
  if(_ctVertices<0) {
    yyerror("Negative vertex count found %d",_ctVertices);
  }

  pMeshLOD->mlod_aVertices.New(_ctVertices);
  _iVertex = 0;
;
    break;}
case 117:
#line 900 "seriousskastudio/parser.y"
{
  if(_iVertex!=_ctVertices) {
    yyerror("Incorect number of vertices.\nExpecting %d but found only %d",_ctVertices,_iVertex);
  }
;
    break;}
case 120:
#line 913 "seriousskastudio/parser.y"
{
  if(_iVertex>=_ctVertices) {
    yyerror("Incorect number of vertices - %d",_ctVertices);
  }

  pMeshLOD->mlod_aVertices[_iVertex].x = yyvsp[-1].v3[0];
  pMeshLOD->mlod_aVertices[_iVertex].y = yyvsp[-1].v3[1];
  pMeshLOD->mlod_aVertices[_iVertex].z = yyvsp[-1].v3[2];
  _iVertex++;
;
    break;}
case 121:
#line 927 "seriousskastudio/parser.y"
{
  _ctNormals = yyvsp[-1].i;
  if(_ctNormals != _ctVertices) {
    yyerror("Number of normals differs from the number of vertices!");
  }
  if(_ctNormals <0) {
    yyerror("Negative normal count found %d",_ctNormals);
  }

  _iNormal = 0;

  pMeshLOD->mlod_aNormals.New(_ctNormals);
;
    break;}
case 122:
#line 941 "seriousskastudio/parser.y"
{
  if(_iNormal!=_ctNormals) {
    yyerror("Incorect number of normals.\nExpecting %d but found only %d",_ctNormals,_iNormal);
  }
;
    break;}
case 125:
#line 955 "seriousskastudio/parser.y"
{
  if(_iNormal>=_ctNormals) {
    yyerror("Incorect number of normals - %d",_ctNormals);
  }

  pMeshLOD->mlod_aNormals[_iNormal].nx = yyvsp[-1].v3[0];
  pMeshLOD->mlod_aNormals[_iNormal].ny = yyvsp[-1].v3[1];
  pMeshLOD->mlod_aNormals[_iNormal].nz = yyvsp[-1].v3[2];
  _iNormal++;
;
    break;}
case 126:
#line 968 "seriousskastudio/parser.y"
{

  _ctUVMaps = yyvsp[-1].i;
  if(_ctUVMaps <0) {
    yyerror("Negative UVMaps count found %d",_ctUVMaps);
  }
  _iUVMap = 0;

  pMeshLOD->mlod_aUVMaps.New(_ctUVMaps);
;
    break;}
case 127:
#line 979 "seriousskastudio/parser.y"
{
  if(_iUVMap!=_ctUVMaps) {
    yyerror("Incorect number of UVMaps.\nExpecting %d but found only %d",_ctUVMaps,_iUVMap);
  }
;
    break;}
case 132:
#line 998 "seriousskastudio/parser.y"
{
  if(_iUVMap>=_ctUVMaps) {
    yyerror("Incorect number of UVMaps - %d",_ctUVMaps);
  }

  _ctTexCoords = yyvsp[-1].i;
  if(_ctTexCoords != _ctVertices) {
    yyerror("Number of texcords differs from the number of vertices!");
  }
  if(_ctTexCoords <0) {
    yyerror("Negative texcoords count found %d",_ctTexCoords);
  }
  _iTexCoord = 0;

  MeshUVMap &uvmap = pMeshLOD->mlod_aUVMaps[_iUVMap];
  uvmap.muv_aTexCoords.New(_ctTexCoords);
  uvmap.muv_iID = ska_GetIDFromStringTable(yyvsp[-4].str);
;
    break;}
case 133:
#line 1017 "seriousskastudio/parser.y"
{
  _iUVMap++;
  if(_iTexCoord!=_ctTexCoords) {
    yyerror("Incorect number of TexCoords for UVMap %d.\nExpecting %d but found only %d",_iUVMap,_ctTexCoords,_iTexCoord);
  }
;
    break;}
case 136:
#line 1032 "seriousskastudio/parser.y"
{
  if(_iTexCoord>=_ctTexCoords) {
    yyerror("Incorect number of TexCoords - %d",_ctTexCoords);
  }

  MeshUVMap &uvmap = pMeshLOD->mlod_aUVMaps[_iUVMap];
  MeshTexCoord &mtc = uvmap.muv_aTexCoords[_iTexCoord];

  mtc.u = yyvsp[-1].v2[0];
  mtc.v = yyvsp[-1].v2[1];
  _iTexCoord++;
;
    break;}
case 137:
#line 1048 "seriousskastudio/parser.y"
{
  _ctSurfaces = yyvsp[-1].i;
  if(_ctSurfaces <0) {
    yyerror("Negative surface count found %d",_ctSurfaces);
  }

  _iSurface = 0;

  pMeshLOD->mlod_aSurfaces.New(_ctSurfaces);

  // reset shader for each surface
  for(INDEX isrf=0;isrf<_ctSurfaces;isrf++) {
    MeshSurface &msrf = pMeshLOD->mlod_aSurfaces[isrf];
    msrf.msrf_pShader = NULL;
  }
;
    break;}
case 138:
#line 1065 "seriousskastudio/parser.y"
{
  if(_iSurface!=_ctSurfaces) {
    yyerror("Incorect number of surfaces.\nExpecting %d but found only %d",_ctSurfaces,_iSurface);
  }
;
    break;}
case 141:
#line 1079 "seriousskastudio/parser.y"
{
  _ctTriangles = yyvsp[-1].i;
  if(_ctTriangles <0) {
    yyerror("Negative triangle count found %d",_ctTriangles);
  }

  if(_iSurface>=_ctSurfaces) {
    yyerror("Incorect number of surfaces - %d",_ctSurfaces);
  }

  MeshSurface &msrf = pMeshLOD->mlod_aSurfaces[_iSurface];
  msrf.msrf_aTriangles.New(_ctTriangles);
  msrf.msrf_iSurfaceID = ska_GetIDFromStringTable(yyvsp[-4].str);
  msrf.msrf_pShader = NULL;

  _iTriangle = 0;

  INDEX ctSrfSha = _assSurfaceShaders.Count();
  // find this surface in array of shader surfaces that was parsed before mesh
  for(INDEX iSrfSha=0;iSrfSha<ctSrfSha;iSrfSha++)
  {
    // reset shader
    SurfaceShader &ssSurfShader = _assSurfaceShaders[iSrfSha];
    if(ssSurfShader.ss_iSurfaceID == msrf.msrf_iSurfaceID)
    {
      // copy shader texture ID's to mesh
      msrf.msrf_ShadingParams.sp_aiTextureIDs = ssSurfShader.ss_spShaderParams.sp_aiTextureIDs;
      msrf.msrf_ShadingParams.sp_aiTexCoordsIndex = ssSurfShader.ss_spShaderParams.sp_aiTexCoordsIndex;
      msrf.msrf_ShadingParams.sp_acolColors = ssSurfShader.ss_spShaderParams.sp_acolColors;
      msrf.msrf_ShadingParams.sp_afFloats = ssSurfShader.ss_spShaderParams.sp_afFloats;
      msrf.msrf_ShadingParams.sp_ulFlags  = ssSurfShader.ss_spShaderParams.sp_ulFlags;
      // set shader for this surface
      if(ssSurfShader.fnShaderName.Length()>0) {
        ChangeSurfaceShader_t(msrf,ssSurfShader.fnShaderName);
      } else {
        msrf.msrf_pShader=NULL;
      }
      break;
    }
  }
;
    break;}
case 142:
#line 1121 "seriousskastudio/parser.y"
{
  _iSurface++;
  if(_iTriangle!=_ctTriangles) {
    yyerror("Incorect number of triangles for surface %d.\nExpecting %d but found only %d",_iSurface,_ctTriangles,_iTriangle);
  }
;
    break;}
case 145:
#line 1136 "seriousskastudio/parser.y"
{
  if(_iTriangle>=_ctTriangles) {
    yyerror("Incorect number of triangles - %d",_ctTriangles);
  }

  MeshSurface &msrf = pMeshLOD->mlod_aSurfaces[_iSurface];
  MeshTriangle &mt = msrf.msrf_aTriangles[_iTriangle];
  mt.iVertex[0]=yyvsp[-1].i3[0];
  mt.iVertex[1]=yyvsp[-1].i3[1];
  mt.iVertex[2]=yyvsp[-1].i3[2];
  _iTriangle++;
;
    break;}
case 146:
#line 1153 "seriousskastudio/parser.y"
{
  _ctWeights = yyvsp[-1].i;
  if(_ctWeights <0) {
    yyerror("Negative weights count found %d",_ctWeights);
  }

  _iWeight = 0;
  pMeshLOD->mlod_aWeightMaps.New(_ctWeights);
;
    break;}
case 147:
#line 1163 "seriousskastudio/parser.y"
{
  if(_iWeight!=_ctWeights) {
    yyerror("Incorect number of weights.\nExpecting %d but found only %d",_ctWeights,_iWeight);
  }
;
    break;}
case 152:
#line 1182 "seriousskastudio/parser.y"
{
  _ctVertexWeights = yyvsp[-1].i;
  if(_ctVertexWeights <0) {
    yyerror("Negative vertex weights count found %d",_ctVertexWeights);
  }

  if(_iWeight>=_ctWeights) {
    yyerror("Incorect number of weights - %d",_ctWeights);
  }

  MeshWeightMap &mwm = pMeshLOD->mlod_aWeightMaps[_iWeight];

  mwm.mwm_iID = ska_GetIDFromStringTable(yyvsp[-4].str);
  mwm.mwm_aVertexWeight.New(_ctVertexWeights);
  _iVertexWeight = 0;
;
    break;}
case 153:
#line 1199 "seriousskastudio/parser.y"
{
  _iWeight++;
  if(_iVertexWeight!=_ctVertexWeights) {
    yyerror("Incorect number of vertex weights.\nExpecting %d but found only %d",_ctVertexWeights,_iVertexWeight);
  }
;
    break;}
case 156:
#line 1214 "seriousskastudio/parser.y"
{
  if(_iVertexWeight>=_ctVertexWeights) {
    yyerror("Incorect number of vertex weights - %d",_ctVertexWeights);
  }

  MeshWeightMap &mwm = pMeshLOD->mlod_aWeightMaps[_iWeight];
  MeshVertexWeight &mvw = mwm.mwm_aVertexWeight[_iVertexWeight];
  
  FLOAT fWeight = yyvsp[-2].f;
  if(fWeight<0) {
    yyerror("Weight value is negative");
  }

  mvw.mww_iVertex = yyvsp[-4].i;
  mvw.mww_fWeight = fWeight;
  _iVertexWeight++;
;
    break;}
case 157:
#line 1236 "seriousskastudio/parser.y"
{
  _ctMorphs = yyvsp[-1].i;

  if(_ctMorphs<0) {
    yyerror("Negative morph count found %d",_ctMorphs);
  }

  pMeshLOD->mlod_aMorphMaps.New(yyvsp[-1].i);
  _iMorph = 0;
;
    break;}
case 158:
#line 1247 "seriousskastudio/parser.y"
{
  if(_iMorph!=_ctMorphs) {
    yyerror("Incorect number of morphs.\nExpecting %d but found only %d",_ctMorphs,_iMorph);
  }
;
    break;}
case 163:
#line 1265 "seriousskastudio/parser.y"
{
  _ctVertexMorphs = yyvsp[0].i;

  if(_ctVertexMorphs <0) {
    yyerror("Negative vertex morphs count found %d",_ctVertexMorphs);
  }

  if(_iMorph>=_ctMorphs) {
    yyerror("Incorect number of morphs - %d",_ctMorphs);
  }

  MeshMorphMap &mmm = pMeshLOD->mlod_aMorphMaps[_iMorph];
  mmm.mmp_aMorphMap.New(yyvsp[0].i);
  mmm.mmp_iID = ska_GetIDFromStringTable(yyvsp[-5].str);
  mmm.mmp_bRelative = yyvsp[-2].i;
  _iVertexMorph = 0;
;
    break;}
case 164:
#line 1283 "seriousskastudio/parser.y"
{
  _iMorph++;
  if(_iVertexMorph!=_ctVertexMorphs) {
    yyerror("Incorect number of vertex morphs.\nExpecting %d but found only %d",_ctVertexMorphs,_iVertexMorph);
  }
;
    break;}
case 167:
#line 1299 "seriousskastudio/parser.y"
{
  if(_iVertexMorph>=_ctVertexMorphs) {
    yyerror("Incorect number of vertex morphs - %d",_ctVertexMorphs);
  }

  // add mesh vertex morph to morph array
  MeshMorphMap &mmm = pMeshLOD->mlod_aMorphMaps[_iMorph];
  MeshVertexMorph &mwm = mmm.mmp_aMorphMap[_iVertexMorph];
  mwm.mwm_iVxIndex = yyvsp[-14].i;
  mwm.mwm_x = yyvsp[-12].f;
  mwm.mwm_y = yyvsp[-10].f;
  mwm.mwm_z = yyvsp[-8].f;
  mwm.mwm_nx = yyvsp[-6].f;
  mwm.mwm_ny = yyvsp[-4].f;
  mwm.mwm_nz = yyvsp[-2].f;
   _iVertexMorph++;
;
    break;}
case 169:
#line 1326 "seriousskastudio/parser.y"
{
  yyval.f = yyvsp[0].f;
;
    break;}
case 170:
#line 1329 "seriousskastudio/parser.y"
{
  yyval.f = (float)yyvsp[0].i;
;
    break;}
case 171:
#line 1335 "seriousskastudio/parser.y"
{
  yyval.i = yyvsp[0].i;
;
    break;}
case 172:
#line 1341 "seriousskastudio/parser.y"
{
  yyval.v2[0] = yyvsp[-2].f;
  yyval.v2[1] = yyvsp[0].f;
;
    break;}
case 173:
#line 1348 "seriousskastudio/parser.y"
{
  yyval.v3[0] = yyvsp[-4].f;
  yyval.v3[1] = yyvsp[-2].f;
  yyval.v3[2] = yyvsp[0].f;
;
    break;}
case 174:
#line 1356 "seriousskastudio/parser.y"
{
  yyval.v3[0] = yyvsp[-4].f;
  yyval.v3[1] = yyvsp[-2].f;
  yyval.v3[2] = yyvsp[0].f;
;
    break;}
case 175:
#line 1364 "seriousskastudio/parser.y"
{
  yyval.i3[0] = yyvsp[-4].i;
  yyval.i3[1] = yyvsp[-2].i;
  yyval.i3[2] = yyvsp[0].i;
;
    break;}
case 176:
#line 1372 "seriousskastudio/parser.y"
{
  yyval.f12[0] = yyvsp[-22].f;
  yyval.f12[1] = yyvsp[-20].f;
  yyval.f12[2] = yyvsp[-18].f;
  yyval.f12[3] = yyvsp[-16].f;
  yyval.f12[4] = yyvsp[-14].f;
  yyval.f12[5] = yyvsp[-12].f;
  yyval.f12[6] = yyvsp[-10].f;
  yyval.f12[7] = yyvsp[-8].f;
  yyval.f12[8] = yyvsp[-6].f;
  yyval.f12[9] = yyvsp[-4].f;
  yyval.f12[10] = yyvsp[-2].f;
  yyval.f12[11] = yyvsp[0].f;
;
    break;}
case 177:
#line 1390 "seriousskastudio/parser.y"
{
  yyval.i = TRUE;
;
    break;}
case 178:
#line 1394 "seriousskastudio/parser.y"
{
  yyval.i = FALSE;
;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 487 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 1398 "seriousskastudio/parser.y"

