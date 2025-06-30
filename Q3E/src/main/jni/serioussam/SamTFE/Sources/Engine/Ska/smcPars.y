%{
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
#include <Engine/Ska/ModelInstance.h>
#include <Engine/Ska/AnimSet.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Shell.h>
#include <Engine/Templates/DynamicContainer.cpp>

// for static linking mojo...
#ifdef __GNUC__
#define yyparse engine_ska_yyparse
#define yyerror engine_ska_yyerror
#define yylex engine_ska_yylex
#endif
#include "ParsingSmbs.h"

extern BOOL bRememberSourceFN;
BOOL bOffsetAllreadySet = FALSE;
%}

%{
// turn off over-helpful bit of bison... --ryan.
#ifdef __GNUC__
#define __attribute__(x)
#endif

#define YYERROR_VERBOSE 0

// if error occurs in parsing
#ifdef __GNUC__
void yyerror(const char *str)
#else
void syyerror(const char *str)
#endif
{
  //_pShell->ErrorF("%s", str);
  _pShell->ErrorF("File '%s'\n %s (line %d)",SMCGetBufferName(), str, SMCGetBufferLineNumber());
}
%}

/* BISON Declarations */

%union {
  int i;
  float f;
  const char *str;
  CModelInstance *pmi;
  float f6[6];
}

%token <f> c_float
%token <i> c_int
%token <str> c_string
%token <pmi> c_modelinstance

%type <f> float_const
%type <i> int_const
%type <pmi> k_PARENTBONE
%type <f6> offset_opt
%type <f6> offset


%token k_SE_SMC
%token k_SE_END
%token k_NAME
%token k_TFNM
%token k_MESH
%token k_SKELETON
%token k_ANIMSET
%token K_ANIMATION
%token k_TEXTURES
%token k_PARENTBONE
%token k_OFFSET
%token k_COLISION
%token k_ALLFRAMESBBOX
%token k_ANIMSPEED
%token k_COLOR

%start parent_model

%%

/*/////////////////////////////////////////////////////////
 * Global structure of the source file.
 */

parent_model
: offset_opt k_NAME c_string ';'
{
  if(_yy_mi==0) {
    yyerror("_yy_mi = NULL");
  }
  // create new model instance
  // _yy_mi = CreateModelInstance($3);
  _yy_mi->SetName($3);
  // set its offset
  _yy_mi->SetOffset($1);
  // mark offset as read
  bOffsetAllreadySet = FALSE;
  // check if flag to remember source file name is set
  if(bRememberSourceFN)
  {
    // remember source file name
    _yy_mi->mi_fnSourceFile = CTString(SMCGetBufferName());
  }
}
'{' components '}'
;

components
: /*null*/
| component components 
;

component
: mesh
| skeleton
| animset_header
| animation_opt
| child_model
| colision_header
| all_frames_bbox_opt
| mdl_color_opt
;

mdl_color_opt
: /*null*/
| mdl_color
;

mdl_color
: k_COLOR c_int ';'
{
  COLOR c = $2;
  // _yy_mi->SetModelColor($2);
}
;

colision_header
: k_COLISION '{' colision_opt '}'
{

}
;
colision_opt
:/*null*/
| colision_array
;

colision_array
: colision
| colision_array colision
;

colision
: c_string '{' float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ';' '}'
{
  FLOAT3D vMin = FLOAT3D($3, $5, $7);
  FLOAT3D vMax = FLOAT3D($9,$11,$13);
  // add new colision box to current model instance
  _yy_mi->AddColisionBox($1,vMin,vMax);
}
;

all_frames_bbox_opt
:/*null*/
| all_frames_bbox
;

all_frames_bbox
: k_ALLFRAMESBBOX float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ';'
{
  // add new colision box to current model instance
  _yy_mi->mi_cbAllFramesBBox.SetMin(FLOAT3D($2, $4, $6));
  _yy_mi->mi_cbAllFramesBBox.SetMax(FLOAT3D($8,$10,$12));
}
;

offset_opt
:/*null*/
{
  // set offset with default offset values
  $$[0] = 0;
  $$[1] = 0;
  $$[2] = 0;
  $$[3] = 0;
  $$[4] = 0;
  $$[5] = 0;
}
| offset offset_opt
{
  // return new offset
  memcpy($$,$1,sizeof(float)*6);
}
;
offset
: k_OFFSET float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ';' 
{
  // if offset is not set
  if(!bOffsetAllreadySet)
  {
    // set offset
    $$[0] = $2;
    $$[1] = $4;
    $$[2] = $6;
    $$[3] = $8;
    $$[4] = $10;
    $$[5] = $12;
    // mark it as set now
    bOffsetAllreadySet = TRUE;
  }
}
;

child_model
: k_PARENTBONE c_string ';' offset_opt k_NAME c_string ';'
{
  // get parent ID
  int iParentBoneID = ska_FindStringInTable($2);
  if(iParentBoneID<0) iParentBoneID=0;
  // remember current model instance in parent bone token
  $1 = _yy_mi;
  // set _yy_mi as new child
  _yy_mi = CreateModelInstance($6);
  // add child to parent model instance 
  $1->AddChild(_yy_mi);
  // add offset
  _yy_mi->SetOffset($4);
  // set its parent bone
  _yy_mi->SetParentBone(iParentBoneID);
  // 
  bOffsetAllreadySet = FALSE;
  // if flag to remember source file is set
  if(bRememberSourceFN)
  {
    // remember source name
    _yy_mi->mi_fnSourceFile = CTString(SMCGetBufferName());
  }
}
// read child components
'{' components '}' 
{
   // set parent model instance to _yy_mi again
  _yy_mi = $1;
}
;


mesh
: k_MESH k_TFNM c_string ';'
{
  // add mesh to current model instance
  _yy_mi->AddMesh_t((CTString)$3);
}
 opt_textures
;

skeleton
: k_SKELETON k_TFNM c_string ';'
{
  // add skeleton to current model instance
  _yy_mi->AddSkeleton_t((CTString)$3);
}
;
animset_header
: k_ANIMSET animset
| k_ANIMSET '{' animset_array '}' 
;

animset_array
: animset
| animset_array animset
;

animset
: k_TFNM c_string ';'
{
  // add animset to curent model instnce 
  _yy_mi->AddAnimSet_t((CTString)$2);
}
;

animation_opt
:/*null*/
| animation
;

animation
: K_ANIMATION c_string ';' 
{
  // set new clear state in model instance
  _yy_mi->NewClearState(1);
  // get anim ID
  INDEX iAnimID = ska_GetIDFromStringTable($2);
  // add animation to curent model instance
  _yy_mi->AddAnimation(iAnimID,AN_LOOPING,1,0);
}
;

opt_textures
: /*null*/
| opt_textures textures
;

textures
: k_TEXTURES '{' textures_array'}'
;

textures_array
: /*null*/
| textures_array texture 
;

texture 
: c_string k_TFNM c_string ';' 
{
  // add texture to current model instance
  _yy_mi->AddTexture_t((CTString)$3,$1,NULL);
}
;
float_const
: c_float
{
  $$ = $1;
}
| c_int
{
  $$ = (float)$1;
}
;
  
int_const
: c_int
{
  $$ = $1;
}
;

%%
