%{
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
%}

%{
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

%}

/* BISON Declarations */

%union {
  INDEX i;
  float f;
  const char *str;
  float v2[2];
  float v3[3];
  int   i3[3];
  float f12[12];
}

%token <f> c_float
%token <i> c_int
%token <str> c_string

%token k_SE_MESH
%token k_SE_SKELETON
%token k_PARENT
%token k_BONES
%token k_VERTICES
%token k_NORMALS
%token k_UVMAPS
%token k_NAME
%token k_TEXCOORDS
%token k_SURFACES
%token k_TRIANGLE_SET
%token k_WEIGHTS
%token k_WEIGHT_SET
%token k_MORPHS
%token k_RELATIVE
%token k_TRUE
%token k_FALSE
%token k_MORPH_SET
%token k_SE_MESH_END
%token k_SE_SKELETON_END
%token k_SE_ANIM
%token k_SEC_PER_FRAME
%token k_FRAMES
%token k_BONEENVELOPES
%token k_MORPHENVELOPES
%token k_DEFAULT_POSE
%token k_SE_ANIM_END
%token k_ANIM_SET_LIST
%token k_ANIM_ID
%token k_MAX_DISTANCE
%token k_MESHLODLIST
%token k_SKELETONLODLIST
%token k_TRESHOLD
%token k_COMPRESION
%token k_LENGTH
%token k_ANIMSPEED
%token k_SHADER_PARAMS
%token k_SHADER_PARAMS_END
%token k_SHADER_SURFACES
%token k_SHADER_SURFACE
%token k_SHADER_NAME
%token k_SHADER_TEXTURES
%token k_SHADER_UVMAPS
%token k_SHADER_COLORS
%token k_SHADER_FLOATS
%token k_SHADER_FLAGS
%token k_FULL_FACE_FORWARD
%token k_HALF_FACE_FORWARD


%type <v2>  uv2
%type <v3>  vert3
%type <v3>  norm3
%type <f>   float_const
%type <i>   int_const
%type <i3>  triangle
%type <f12> matrix
%type <i>   boolean


%start program

%%

/*
 * Global structure of the source file.
 */

program
: meshlod
| skeletonlod_list
| animset_array 
| animset_list
| meshlod_list
;

/*
 * Mesh lod list
 */

meshlod_list
: k_MESHLODLIST '{' meshlod_array_opt '}'
;
// optional array of mesh lods
meshlod_array_opt
: /*null*/
| meshlod_array
;
// array of mesh lods
meshlod_array
: meshlod
| meshlod_array meshlod
;
// lod max distance
max_distance
: /*null*/
{
  // fill with default values
}
| k_MAX_DISTANCE float_const ';'
{
  _fCurentMaxDistance = $2;
}
;

meshlod
: max_distance shader_params_opt mesh_begin is_full_faceforward_opt is_half_faceforward_opt vertices normals uvmaps surfaces weights morphs mesh_end
;


/* 
 * AnimSet importer
 */

animset_list
: k_ANIM_SET_LIST '{' animset_array_opt '}' 
;

animset_array_opt
: /*null*/
| animset_array
;

animset_array
: animset
| animset_array animset
;

animset
: treshold_opt compresion_opt animspeed_opt animset_begin animation_header bone_envelopes morph_envelopes animset_end
;

compresion_opt
:/*null*/
{ bCompresion = FALSE; }
| k_COMPRESION boolean
{ bCompresion  = $2; }

treshold_opt 
: /*null*/
{ _fTreshold = 0.0f; }
| k_TRESHOLD float_const ';'
{ _fTreshold = $2; }
;

animspeed_opt
: /*null*/
{
  _fAnimSpeed = -1;
}
 | k_ANIMSPEED float_const ';'
{
  _fAnimSpeed = $2;
}
;

animset_begin
: k_SE_ANIM c_float ';'
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
}
;

animation_header
: k_SEC_PER_FRAME c_float ';' k_FRAMES c_int ';' k_ANIM_ID c_string ';'
{
  _ctFrames = $5;
  if(_ctFrames<0) {
    yyerror("Negative frame count found %d",_ctFrames);
  }

  // get current animset count
  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  // get ref to last animation
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  // set animations seconds per frame
  an.an_fSecPerFrame = $2;
  // set animation frame count
  an.an_iFrames = _ctFrames;
  // set animation ID
  an.an_iID = ska_GetIDFromStringTable($8);
  // set animation source file (current file)
  an.an_fnSourceFile = strCurentFileName;
  // set animation custom speed (if any)
  an.an_bCustomSpeed = FALSE;
  if(_fAnimSpeed>=0)
  {
    an.an_bCustomSpeed = TRUE;
    an.an_fSecPerFrame = _fAnimSpeed;
  }
}
;

bone_envelopes
: k_BONEENVELOPES c_int
{
  // set bone envelopes count
  _ctBoneEnvelopes = $2;

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
}
 '{' bone_env_header_opt '}'
{
  if(_iBoneEnvelope!=_ctBoneEnvelopes) {
    yyerror("Incorect number of bone envelopes.\nExpecting %d but found only %d",_ctBoneEnvelopes,_iBoneEnvelope);
  }
}
;

bone_env_header_opt
: /*null*/
| bone_env_header
;

bone_env_header
: bone_envelope
| bone_env_header_opt bone_envelope
;

bone_envelope
: k_NAME c_string k_DEFAULT_POSE '{' matrix ';'
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
  be.be_iBoneID = ska_GetIDFromStringTable($2);
  // fill default pos matrix
  memcpy(&be.be_mDefaultPos,$5,sizeof(float)*12);
  
  _iFrame = 0;
}
 '}' '{' bone_env_array '}'
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
}
;


bone_env_array
: bone_env_m
| bone_env_array bone_env_m
; 

bone_env_m
: matrix ';'
{
  if(_iFrame>=_ctFrames) {
    yyerror("Incorect number of bone envelope frames - %d",_ctFrames);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  QVect qvPlacement;
  Matrix12ToQVect(qvPlacement,$1);
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  BoneEnvelope &be = an.an_abeBones[_iBoneEnvelope];

  be.be_apPos[_iFrame].ap_vPos = qvPlacement.vPos;
  be.be_arRot[_iFrame].ar_qRot = qvPlacement.qRot;
  _iFrame++;
}
;

morph_envelopes
: k_MORPHENVELOPES c_int
{
  // set morph envelopes count
  _ctMorphEnvelopes = $2;

  if(_ctMorphEnvelopes<0) {
    yyerror("Negative morph envelope count found %d",_ctMorphEnvelopes);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  _yy_pAnimSet->as_Anims[ctan-1].an_ameMorphs.New(_ctMorphEnvelopes);
  _iMorphEnvelope = 0;
}
 '{' morph_env_header '}'
{
  if(_iMorphEnvelope!=_ctMorphEnvelopes) {
    yyerror("Incorect number of morph envelopes.\nExpecting %d but found only %d",_ctMorphEnvelopes,_iMorphEnvelope);
  }
}
;

morph_env_header
: /*null*/
| morph_env_header_notnull
;

morph_env_header_notnull
: morph_env
| morph_env_header morph_env
;

morph_env
: k_NAME c_string
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
    me.me_iMorphMapID = ska_GetIDFromStringTable($2);
    _iFrame = 0;
}
 '{' morph_env_array '}'
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
}
;

morph_env_array
: morph_env_i
| morph_env_array morph_env_i
;

morph_env_i
: c_float ';'
{
  if(_iFrame>=_ctFrames) {
    yyerror("Incorect number of morph envelope frames - %d",_ctFrames);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  MorphEnvelope &me = an.an_ameMorphs[_iMorphEnvelope];
  me.me_aFactors[_iFrame] = $1;

  _iFrame++;
}
 | c_int ';'
{
  if(_iFrame>=_ctFrames) {
    yyerror("Incorect number of morph envelope frames - %d",_ctFrames);
  }

  INDEX ctan = _yy_pAnimSet->as_Anims.Count();
  Animation &an = _yy_pAnimSet->as_Anims[ctan-1];
  MorphEnvelope &me = an.an_ameMorphs[_iMorphEnvelope];
  me.me_aFactors[_iFrame] = $1;

  _iFrame++;
}
;

animset_end
: k_SE_ANIM_END ';'
;


/*
 * Skeleton importer
 */

skeletonlod_list
: k_SKELETONLODLIST '{' opt_skeletonlod_array '}'
;

opt_skeletonlod_array
: /*null*/
| skeletonlod_array
;

skeletonlod_array
: skeletonlod
| skeletonlod_array skeletonlod
;

skeletonlod
: max_distance skeleton_begin bones skeleton_end
;

skeleton_begin
: k_SE_SKELETON c_float ';'
{
  // check for version of skeleton ascii file
}
;

bones
: k_BONES c_int '{' 
{
  _ctBones = $2;
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
}
bone_headers '}'
{
  if(_iBone!=_ctBones) {
    yyerror("Incorect number of bones.\nExpecting %d but found only %d",_ctBones,_iBone);
  }
}
;

bone_headers
: /*null*/
| bone_headers_notnull
;

bone_headers_notnull
: bone_header
| bone_headers bone_header
;

bone_header
: k_NAME c_string ';' k_PARENT c_string ';' k_LENGTH float_const 
{
  if(_iBone>=_ctBones) {
    yyerror("Incorect number of bones - %d",_ctBones);
  }

  SkeletonBone &sb = pSkeletonLOD->slod_aBones[_iBone];
  // get bone ID
  sb.sb_iID = ska_GetIDFromStringTable($2);
  // get parent bone ID
  sb.sb_iParentID = ska_GetIDFromStringTable($5);
  // get bone length
  sb.sb_fBoneLength = $8;
}
';' bone
;

bone
: '{' matrix  ';' '}'
{
  SkeletonBone &sb = pSkeletonLOD->slod_aBones[_iBone];
  // convert matrix to qvect struct
  Matrix12ToQVect(sb.sb_qvRelPlacement,$2);
  // copy matrix to bone as its absolute placement
  memcpy(&sb.sb_mAbsPlacement,$2,sizeof(float)*12);
  // calculate bone offset length
  FLOAT3D vOffset = FLOAT3D($2[3],$2[7],$2[11]);
  sb.sb_fOffSetLen = vOffset.Length();
  // increase bone count
  _iBone++;
}
;

skeleton_end
: k_SE_SKELETON_END ';'
{
  // end of skeleton parsing
}

/*
** Shader params
*/
shader_params_opt
:/*null*/
{
  // no shader for next surface
  _assSurfaceShaders.Clear();
  _ctShaderSurfaces=0;
  _iShaderSurfIndex=0;
}
|shader_params
;

shader_params
: k_SHADER_PARAMS float_const ';' k_SHADER_SURFACES c_int '{'
{
  _assSurfaceShaders.Clear();
  _ctShaderSurfaces = $5;
  // create array of shader params for each surface in mesh file
  _assSurfaceShaders.New(_ctShaderSurfaces);
  // reset surface index
  _iShaderSurfIndex=0;
} surface_shader_params_array_opt '}' ';'
{
  // check if surfaces count match count of read surfaces
  if(_iShaderSurfIndex!=_ctShaderSurfaces) yyerror("Incorect number of surfaces");
} k_SHADER_PARAMS_END ; 

surface_shader_params_array_opt
: /*null*/
| surface_shader_params_array
;

surface_shader_params_array
: surface_shader_params
| surface_shader_params_array surface_shader_params;

surface_shader_params
: shader_name shader_textures shader_uvmaps shader_colors shader_floats shader_flag_opt'}' ';' 
{
  _iShaderSurfIndex++;
}
;

shader_name
: k_SHADER_SURFACE c_string ';' '{' k_SHADER_NAME c_string ';'
{
  if(_iShaderSurfIndex>=_ctShaderSurfaces) yyerror("Incorect number of surfaces");
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_iSurfaceID = ska_GetIDFromStringTable($2);
  // set shader fn
  SurfShader.fnShaderName = (CTString)$6
};

shader_textures
: k_SHADER_TEXTURES c_int
{
  _ctshParamsMax = $2;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_aiTextureIDs.New(_ctshParamsMax);
  _ishParamIndex = 0;
} '{' shader_texture_array_opt '}' ';'
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect texture params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
};

shader_texture_array_opt
: /*null*/
| shader_texture_array;

shader_texture_array
: shader_texture
| shader_texture_array shader_texture
;

shader_texture
: c_string ';'
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect texture params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  // set ID of current texture name
  SurfShader.ss_spShaderParams.sp_aiTextureIDs[_ishParamIndex] = ska_GetIDFromStringTable($1);
  // increase shading parametar index
  _ishParamIndex++;
};

shader_uvmaps
:k_SHADER_UVMAPS c_int 
{
  _ctshParamsMax = $2;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_aiTexCoordsIndex.New(_ctshParamsMax);
  _ishParamIndex = 0;
} '{' shader_uvmaps_array_opt '}' ';'
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect uvmap params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
};

shader_uvmaps_array_opt
: /*null*/
| shader_uvmaps_array;

shader_uvmaps_array
: shader_uvmap
| shader_uvmaps_array shader_uvmap;

shader_uvmap
: c_int ';'
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect uvmap params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  // set index of current uvmap name
  SurfShader.ss_spShaderParams.sp_aiTexCoordsIndex[_ishParamIndex] = $1;
  // increase shading parametar index
  _ishParamIndex++;
};

shader_colors
: k_SHADER_COLORS c_int
{
  _ctshParamsMax = $2;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_acolColors.New(_ctshParamsMax);
  _ishParamIndex = 0;
} '{' shader_colors_array_opt '}' ';'
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect color params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
};

shader_colors_array_opt
: /*null*/
| shader_colors_array;

shader_colors_array
: shader_color
| shader_colors_array shader_color;

shader_color
: c_int ';'
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect colors params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  // set color
  SurfShader.ss_spShaderParams.sp_acolColors[_ishParamIndex] = $1;
  // increase color parametar index
  _ishParamIndex++;
};

shader_floats
: k_SHADER_FLOATS c_int
{
  _ctshParamsMax = $2;
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_afFloats.New(_ctshParamsMax);
  _ishParamIndex = 0;

} '{' shader_floats_array_opt '}' ';'
{
  // incorect params count
  if(_ishParamIndex!=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect floats params count\nExpecting %d but found %d",_ctshParamsMax,_ishParamIndex);
    yyerror((char*)(const char*)strErr);
  }
};

shader_floats_array_opt
: /*null*/
| shader_floats_array;

shader_floats_array
: shader_float
| shader_floats_array shader_float;

shader_float
: float_const ';'
{
  if(_ishParamIndex>=_ctshParamsMax)
  {
    CTString strErr = CTString(0,"Incorect floats params count %d",_ctshParamsMax);
    yyerror((char*)(const char*)strErr);
  }
  // set color
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_afFloats[_ishParamIndex] = $1;
  // increase floats parametar index
  _ishParamIndex++;
};

shader_flag_opt
: /*null*/
| shader_flags;

shader_flags
: k_SHADER_FLAGS c_int ';'
{
  SurfaceShader &SurfShader = _assSurfaceShaders[_iShaderSurfIndex];
  SurfShader.ss_spShaderParams.sp_ulFlags = $2;
}

// Mesh importer
mesh_begin
: k_SE_MESH c_float ';'
{
  if ($2!=0.1) NOTHING; //parseMesh->error
  INDEX ctmshlod = _yy_pMesh->msh_aMeshLODs.Count();
  _yy_pMesh->msh_aMeshLODs.Expand(ctmshlod+1);
  pMeshLOD = &_yy_pMesh->msh_aMeshLODs[ctmshlod];
  pMeshLOD->mlod_fnSourceFile = strCurentFileName;
  pMeshLOD->mlod_fMaxDistance = _fCurentMaxDistance;
  _yy_iWIndex=0;
  _yy_iMIndex=0;
}
;

is_full_faceforward_opt
: /*null*/
{
  pMeshLOD->mlod_ulFlags &= ~ML_FULL_FACE_FORWARD;
}
| k_FULL_FACE_FORWARD boolean
{
  if($2 == TRUE) {
    pMeshLOD->mlod_ulFlags|=ML_FULL_FACE_FORWARD;
  } else {
    pMeshLOD->mlod_ulFlags &=~ML_FULL_FACE_FORWARD;
  }
}

is_half_faceforward_opt
: /*null*/
{
  pMeshLOD->mlod_ulFlags &= ~ML_HALF_FACE_FORWARD;
}
| k_HALF_FACE_FORWARD boolean
{
  if($2 == TRUE) {
    pMeshLOD->mlod_ulFlags|=ML_HALF_FACE_FORWARD;
  } else {
    pMeshLOD->mlod_ulFlags &=~ML_HALF_FACE_FORWARD;
  }
}


vertices
: k_VERTICES c_int '{' {
  _ctVertices = $2;
  if(_ctVertices<0) {
    yyerror("Negative vertex count found %d",_ctVertices);
  }

  pMeshLOD->mlod_aVertices.New(_ctVertices);
  _iVertex = 0;
}
 vert3_array '}'
{
  if(_iVertex!=_ctVertices) {
    yyerror("Incorect number of vertices.\nExpecting %d but found only %d",_ctVertices,_iVertex);
  }
}
;

vert3_array 
: vert3_array_value 
| vert3_array vert3_array_value
;

vert3_array_value
: vert3 ';' {
  if(_iVertex>=_ctVertices) {
    yyerror("Incorect number of vertices - %d",_ctVertices);
  }

  pMeshLOD->mlod_aVertices[_iVertex].x = $1[0];
  pMeshLOD->mlod_aVertices[_iVertex].y = $1[1];
  pMeshLOD->mlod_aVertices[_iVertex].z = $1[2];
  _iVertex++;
}
;


normals
: k_NORMALS c_int '{' {
  _ctNormals = $2;
  if(_ctNormals != _ctVertices) {
    yyerror("Number of normals differs from the number of vertices!");
  }
  if(_ctNormals <0) {
    yyerror("Negative normal count found %d",_ctNormals);
  }

  _iNormal = 0;

  pMeshLOD->mlod_aNormals.New(_ctNormals);
}
 norm3_array '}'
{
  if(_iNormal!=_ctNormals) {
    yyerror("Incorect number of normals.\nExpecting %d but found only %d",_ctNormals,_iNormal);
  }
}
;

norm3_array
: norm3_array_value
| norm3_array norm3_array_value
;

norm3_array_value
: norm3 ';'
{
  if(_iNormal>=_ctNormals) {
    yyerror("Incorect number of normals - %d",_ctNormals);
  }

  pMeshLOD->mlod_aNormals[_iNormal].nx = $1[0];
  pMeshLOD->mlod_aNormals[_iNormal].ny = $1[1];
  pMeshLOD->mlod_aNormals[_iNormal].nz = $1[2];
  _iNormal++;
}
;

uvmaps
: k_UVMAPS c_int '{' {

  _ctUVMaps = $2;
  if(_ctUVMaps <0) {
    yyerror("Negative UVMaps count found %d",_ctUVMaps);
  }
  _iUVMap = 0;

  pMeshLOD->mlod_aUVMaps.New(_ctUVMaps);
}
uvmaps_opt_array '}'
{
  if(_iUVMap!=_ctUVMaps) {
    yyerror("Incorect number of UVMaps.\nExpecting %d but found only %d",_ctUVMaps,_iUVMap);
  }
}
;

uvmaps_opt_array
: /*null*/
| uvmaps_array
;

uvmaps_array
: uvmap 
| uvmaps_array uvmap
;

uvmap
: '{' k_NAME c_string ';' k_TEXCOORDS c_int '{'
{
  if(_iUVMap>=_ctUVMaps) {
    yyerror("Incorect number of UVMaps - %d",_ctUVMaps);
  }

  _ctTexCoords = $6;
  if(_ctTexCoords != _ctVertices) {
    yyerror("Number of texcords differs from the number of vertices!");
  }
  if(_ctTexCoords <0) {
    yyerror("Negative texcoords count found %d",_ctTexCoords);
  }
  _iTexCoord = 0;

  MeshUVMap &uvmap = pMeshLOD->mlod_aUVMaps[_iUVMap];
  uvmap.muv_aTexCoords.New(_ctTexCoords);
  uvmap.muv_iID = ska_GetIDFromStringTable($3);
}
uv2_array '}' '}'
{
  _iUVMap++;
  if(_iTexCoord!=_ctTexCoords) {
    yyerror("Incorect number of TexCoords for UVMap %d.\nExpecting %d but found only %d",_iUVMap,_ctTexCoords,_iTexCoord);
  }
}
;

uv2_array
: uv2_array_value
| uv2_array uv2_array_value
;

uv2_array_value
: uv2 ';'
{
  if(_iTexCoord>=_ctTexCoords) {
    yyerror("Incorect number of TexCoords - %d",_ctTexCoords);
  }

  MeshUVMap &uvmap = pMeshLOD->mlod_aUVMaps[_iUVMap];
  MeshTexCoord &mtc = uvmap.muv_aTexCoords[_iTexCoord];

  mtc.u = $1[0];
  mtc.v = $1[1];
  _iTexCoord++;
}
;

surfaces
: k_SURFACES c_int '{'
{
  _ctSurfaces = $2;
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
}
 surfaces_array '}'
{
  if(_iSurface!=_ctSurfaces) {
    yyerror("Incorect number of surfaces.\nExpecting %d but found only %d",_ctSurfaces,_iSurface);
  }
}
;

surfaces_array
: triangleset 
| surfaces_array triangleset
;

triangleset
: '{' k_NAME c_string ';' k_TRIANGLE_SET c_int '{'
{
  _ctTriangles = $6;
  if(_ctTriangles <0) {
    yyerror("Negative triangle count found %d",_ctTriangles);
  }

  if(_iSurface>=_ctSurfaces) {
    yyerror("Incorect number of surfaces - %d",_ctSurfaces);
  }

  MeshSurface &msrf = pMeshLOD->mlod_aSurfaces[_iSurface];
  msrf.msrf_aTriangles.New(_ctTriangles);
  msrf.msrf_iSurfaceID = ska_GetIDFromStringTable($3);
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
}
 triangle_array '}' '}'
{
  _iSurface++;
  if(_iTriangle!=_ctTriangles) {
    yyerror("Incorect number of triangles for surface %d.\nExpecting %d but found only %d",_iSurface,_ctTriangles,_iTriangle);
  }
}
;

triangle_array
: triangle_array_value
| triangle_array triangle_array_value
;

triangle_array_value
: triangle ';'
{
  if(_iTriangle>=_ctTriangles) {
    yyerror("Incorect number of triangles - %d",_ctTriangles);
  }

  MeshSurface &msrf = pMeshLOD->mlod_aSurfaces[_iSurface];
  MeshTriangle &mt = msrf.msrf_aTriangles[_iTriangle];
  mt.iVertex[0]=$1[0];
  mt.iVertex[1]=$1[1];
  mt.iVertex[2]=$1[2];
  _iTriangle++;
}
;


weights
: k_WEIGHTS c_int '{'
{
  _ctWeights = $2;
  if(_ctWeights <0) {
    yyerror("Negative weights count found %d",_ctWeights);
  }

  _iWeight = 0;
  pMeshLOD->mlod_aWeightMaps.New(_ctWeights);
}
 weights_array '}'
{
  if(_iWeight!=_ctWeights) {
    yyerror("Incorect number of weights.\nExpecting %d but found only %d",_ctWeights,_iWeight);
  }
}
;

weights_array
:/*null*/
|weights_array_notnull
;

weights_array_notnull
: weightset
| weights_array weightset
;

weightset
: '{' k_NAME c_string ';' k_WEIGHT_SET c_int '{'
{
  _ctVertexWeights = $6;
  if(_ctVertexWeights <0) {
    yyerror("Negative vertex weights count found %d",_ctVertexWeights);
  }

  if(_iWeight>=_ctWeights) {
    yyerror("Incorect number of weights - %d",_ctWeights);
  }

  MeshWeightMap &mwm = pMeshLOD->mlod_aWeightMaps[_iWeight];

  mwm.mwm_iID = ska_GetIDFromStringTable($3);
  mwm.mwm_aVertexWeight.New(_ctVertexWeights);
  _iVertexWeight = 0;
}
  weight_map_array '}' '}'
{
  _iWeight++;
  if(_iVertexWeight!=_ctVertexWeights) {
    yyerror("Incorect number of vertex weights.\nExpecting %d but found only %d",_ctVertexWeights,_iVertexWeight);
  }
}
;

weight_map_array
: weight_map_array_value
| weight_map_array weight_map_array_value
;

weight_map_array_value
: '{' c_int ';' float_const ';' '}'
{
  if(_iVertexWeight>=_ctVertexWeights) {
    yyerror("Incorect number of vertex weights - %d",_ctVertexWeights);
  }

  MeshWeightMap &mwm = pMeshLOD->mlod_aWeightMaps[_iWeight];
  MeshVertexWeight &mvw = mwm.mwm_aVertexWeight[_iVertexWeight];
  
  FLOAT fWeight = $4;
  if(fWeight<0) {
    yyerror("Weight value is negative");
  }

  mvw.mww_iVertex = $2;
  mvw.mww_fWeight = fWeight;
  _iVertexWeight++;
}
;


morphs
: k_MORPHS c_int '{'
{
  _ctMorphs = $2;

  if(_ctMorphs<0) {
    yyerror("Negative morph count found %d",_ctMorphs);
  }

  pMeshLOD->mlod_aMorphMaps.New($2);
  _iMorph = 0;
}
 morphs_array '}'
{
  if(_iMorph!=_ctMorphs) {
    yyerror("Incorect number of morphs.\nExpecting %d but found only %d",_ctMorphs,_iMorph);
  }
}
;

morphs_array
: /*null*/
| morphs_array_notnull
;
morphs_array_notnull
: morphset
| morphs_array morphset
;

morphset
: '{' k_NAME c_string ';' k_RELATIVE boolean k_MORPH_SET c_int
{
  _ctVertexMorphs = $8;

  if(_ctVertexMorphs <0) {
    yyerror("Negative vertex morphs count found %d",_ctVertexMorphs);
  }

  if(_iMorph>=_ctMorphs) {
    yyerror("Incorect number of morphs - %d",_ctMorphs);
  }

  MeshMorphMap &mmm = pMeshLOD->mlod_aMorphMaps[_iMorph];
  mmm.mmp_aMorphMap.New($8);
  mmm.mmp_iID = ska_GetIDFromStringTable($3);
  mmm.mmp_bRelative = $6;
  _iVertexMorph = 0;
}
 '{' morph_set_array '}' '}'
{
  _iMorph++;
  if(_iVertexMorph!=_ctVertexMorphs) {
    yyerror("Incorect number of vertex morphs.\nExpecting %d but found only %d",_ctVertexMorphs,_iVertexMorph);
  }
}
;

morph_set_array
: morph_set_array_value
| morph_set_array morph_set_array_value
;

morph_set_array_value
: '{' c_int ';' float_const ',' float_const ',' float_const ';'
                float_const ',' float_const ',' float_const ';' '}'
{
  if(_iVertexMorph>=_ctVertexMorphs) {
    yyerror("Incorect number of vertex morphs - %d",_ctVertexMorphs);
  }

  // add mesh vertex morph to morph array
  MeshMorphMap &mmm = pMeshLOD->mlod_aMorphMaps[_iMorph];
  MeshVertexMorph &mwm = mmm.mmp_aMorphMap[_iVertexMorph];
  mwm.mwm_iVxIndex = $2;
  mwm.mwm_x = $4;
  mwm.mwm_y = $6;
  mwm.mwm_z = $8;
  mwm.mwm_nx = $10;
  mwm.mwm_ny = $12;
  mwm.mwm_nz = $14;
   _iVertexMorph++;
}
;


mesh_end
: k_SE_MESH_END ';'
;


float_const
: c_float
{
  $$ = $1;
}
| c_int {
  $$ = (float)$1;
}
;
  
int_const
: c_int {
  $$ = $1;
}
;

uv2
: float_const ',' float_const {
  $$[0] = $1;
  $$[1] = $3;
}
;

vert3
: float_const ',' float_const ',' float_const {
  $$[0] = $1;
  $$[1] = $3;
  $$[2] = $5;
}
;

norm3
: float_const ',' float_const ',' float_const {
  $$[0] = $1;
  $$[1] = $3;
  $$[2] = $5;
}
;

triangle
: int_const ',' int_const ',' int_const {
  $$[0] = $1;
  $$[1] = $3;
  $$[2] = $5;
}
;

matrix
: float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const ',' float_const {
  $$[0] = $1;
  $$[1] = $3;
  $$[2] = $5;
  $$[3] = $7;
  $$[4] = $9;
  $$[5] = $11;
  $$[6] = $13;
  $$[7] = $15;
  $$[8] = $17;
  $$[9] = $19;
  $$[10] = $21;
  $$[11] = $23;
}
;

boolean
: k_TRUE ';'
{
  $$ = TRUE;
}
| k_FALSE ';'
{
  $$ = FALSE;
}
;
%%
