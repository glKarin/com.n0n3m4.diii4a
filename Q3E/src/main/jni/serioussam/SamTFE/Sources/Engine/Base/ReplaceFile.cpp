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

#include "Engine/StdH.h"

#include <Engine/Base/ReplaceFile.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Anim.h>
#include <Engine/Base/Shell.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Models/ModelObject.h>
#include <Engine/Sound/SoundObject.h>
#include <Engine/Ska/ModelInstance.h>
#include <Engine/Ska/Mesh.h>
#include <Engine/Ska/Skeleton.h>
#include <Engine/Ska/AnimSet.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Templates/Stock_CMesh.h>
#include <Engine/Templates/Stock_CSkeleton.h>
#include <Engine/Templates/Stock_CAnimSet.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/DynamicContainer.cpp>
//#include <Engine/Templates/Stock_CShader.h>

#include <Engine/Base/ListIterator.inl>

#define FILTER_TEX            "Textures (*.tex)\0*.tex\0"
#define FILTER_MDL            "Models (*.mdl)\0*.mdl\0"
#define FILTER_ANI            "Animations (*.ani)\0*.ani\0"
#define FILTER_END            "\0"

BOOL _bFileReplacingApplied;

#ifndef NDEBUG
  #define ENGINEGUI_DLL_NAME "EngineGUID.dll"
#else
  #define ENGINEGUI_DLL_NAME "EngineGUI.dll"
#endif

extern INDEX wed_bUseBaseForReplacement;

static CTFileName CallFileRequester(char *achrTitle, char *achrSelectedFile, const char *pFilter)
{
#ifdef PLATFORM_WIN32
  typedef CTFileName FileRequester_t(
    char *pchrTitle, 
    char *pchrFilters,
    char *pchrRegistry,
    char *pchrDefaultFileSelected);

  HMODULE hGUI = GetModuleHandleA(ENGINEGUI_DLL_NAME);
  if (hGUI==NULL) {
    WarningMessage(TRANS("Cannot load %s:\n%s\nCannot replace files!"), 
      ENGINEGUI_DLL_NAME, GetWindowsError(GetLastError()));
    return CTString("");
  }
  FileRequester_t *pFileRequester = (FileRequester_t*)GetProcAddress(hGUI, 
    "?FileRequester@@YA?AVCTFileName@@PAD000@Z");
  if (pFileRequester==NULL) {
    WarningMessage(TRANS("Error in %s:\nFileRequester() function not found\nCannot replace files!"),
      ENGINEGUI_DLL_NAME);
    return CTString("");
  }

  // !!! FIXME: make this const correct?  --ryan.
  return pFileRequester( achrTitle, (char *) pFilter, "Replace file directory", achrSelectedFile);

#else

    STUBBED("wtf?!");
    return CTString("");

#endif

}

BOOL GetReplacingFile(CTFileName fnSourceFile, CTFileName &fnReplacingFile,
                      const char *pFilter)
{
  // don't replace files if this console variable is set
  if (!wed_bUseBaseForReplacement) {
    return FALSE;
  }

  CTString strBaseForReplacingFiles;
  // try to find replacing texture in base
  try
  {
  	char achrLine[ 256];
  	char achrSource[ 256];
  	char achrRemap[ 256];
        
    // open file containing file names for replacing textures
    CTFileStream fsBase;
    CTFileName fnBaseName = CTString("Data\\BaseForReplacingFiles.txt");
    fsBase.Open_t( fnBaseName);
    while( !fsBase.AtEOF())
    {
      fsBase.GetLine_t( achrLine, 256);
      sscanf( achrLine, "\"%[^\"]\" \"%[^\"]\"", achrSource, achrRemap);
      if (CTString( achrSource) ==  achrRemap) {
        continue; // skip remaps to self
      }
      if( CTString( achrSource) == fnSourceFile)
      {
        fnReplacingFile = CTString( achrRemap);
        return TRUE;
      }
    }
    fsBase.Close();
  }
  // if file containing base can't be opened
  catch (const char *strError)
  {
    (void) strError;
  }
  CTString strTitle;
  strTitle.PrintF(TRANSV("For:\"%s\""), (const char *) (CTString&)fnSourceFile);
  // call file requester for substituting file
  CTString strDefaultFile;
  strDefaultFile = fnSourceFile.FileName() + fnSourceFile.FileExt();
  fnReplacingFile = CallFileRequester((char*)(const char*)strTitle, (char*)(const char*)strDefaultFile, pFilter);
  if( fnReplacingFile == "") return FALSE;

  try
  {
    // add new remap to end of remapping base file
    CTFileName fnBaseName = CTString("Data\\BaseForReplacingFiles.txt");
    CTString strBase;
    if( FileExists( fnBaseName))
    {
      strBase.Load_t( fnBaseName);
    }
    CTString strNewRemap;
    strNewRemap.PrintF( "\"%s\" \"%s\"\n", (const char *) (CTString&)fnSourceFile, (const char *) (CTString&)fnReplacingFile);
    strBase += strNewRemap;
    strBase.Save_t( fnBaseName);
  }
  catch (const char *strError)
  {
    WarningMessage( strError);
    return FALSE;
  }
  return TRUE;
}


void SetTextureWithPossibleReplacing_t(CTextureObject &to, CTFileName &fnmTexture)
{
  // try to load texture
  for(;;)
  {
    try
    {
      to.SetData_t(fnmTexture);
      break;
    }
    catch (const char *strError)
    {
      (void) strError;
      // if texture was not found, ask for replacing texture
      CTFileName fnReplacingTexture;
      if( GetReplacingFile( fnmTexture, fnReplacingTexture, FILTER_TEX FILTER_END))
      {
        // if replacing texture was provided, repeat reading of polygon's textures
        fnmTexture = fnReplacingTexture;
      }
      else
      {
        if(_pShell->GetINDEX("wed_bUseGenericTextureReplacement")) {
          fnmTexture = CTString("Textures\\Editor\\Default.tex");
          to.SetData_t(fnmTexture);
        } else {
          ThrowF_t( TRANS("Unable to load world because texture \"%s\" can't be found."),
            (const char *) ((CTString&)fnmTexture));
        }
      }
    }
  }
}

// read/write a texture object
void ReadTextureObject_t(CTStream &strm, CTextureObject &to)
{
  // read model texture data filename
  CTFileName fnTexture;
  strm>>fnTexture;
  // try to load texture
  for(;;) {
    try {
      // set the texture data
      to.SetData_t(fnTexture);
      break;
    } catch (const char *strError) {
      (void) strError;
      // if texture was not found, ask for replacing texture
      CTFileName fnReplacingTexture;
      if( GetReplacingFile( fnTexture, fnReplacingTexture, FILTER_TEX FILTER_END)) {
        // replacing texture was provided
        fnTexture = fnReplacingTexture;
      } else {
        ThrowF_t( TRANS("Cannot find substitution for \"%s\""), (const char *) (CTString&)fnTexture);
      }
    }
  }
  // read main texture anim object
  to.Read_t(&strm);
}
void SkipTextureObject_t(CTStream &strm)
{
  // skip texture filename
  CTFileName fnDummy;
  strm>>fnDummy;
  // skip texture object
  CTextureObject toDummy;
  toDummy.Read_t(&strm);
}
void WriteTextureObject_t(CTStream &strm, CTextureObject &to)
{
  // write model texture filename
  CTextureData *ptd = (CTextureData *)to.GetData();
  if (ptd!=NULL) {
    strm<<ptd->GetName();
  } else {
    strm<<CTFileName(CTString(""));
  }
  // write texture anim object
  to.Write_t(&strm);
}

// read a model object from a file together with its model data filename
void ReadModelObject_t(CTStream &strm, CModelObject &mo)
{
  // read model data filename
  CTFileName fnModel;
  strm>>fnModel;
  // try to load model
  for(;;) {
    try {
      // set the model data
      mo.SetData_t(fnModel);
      break;
    } catch (const char *strError) {
      (void) strError;
      // if model was not found, ask for replacing model
      CTFileName fnReplacingModel;
      if( GetReplacingFile( fnModel, fnReplacingModel, FILTER_MDL FILTER_END)) {
        // replacing model was provided
        fnModel = fnReplacingModel;
      } else {
        ThrowF_t( TRANS("Cannot find substitution for \"%s\""), (const char *) (CTString&)fnModel);
      }
    }
  }
  // read model anim object
  mo.Read_t(&strm);

  // if model object has multiple textures
  if (strm.PeekID_t() == CChunkID("MTEX")) { // 'multi-texturing'
    // read all textures
    strm.ExpectID_t("MTEX");  // 'multi-texturing''
    ReadTextureObject_t(strm, mo.mo_toTexture);
    ReadTextureObject_t(strm, mo.mo_toBump);
    ReadTextureObject_t(strm, mo.mo_toReflection);
    ReadTextureObject_t(strm, mo.mo_toSpecular);

  // if model has single texture (old format)
  } else {
    // read main texture
    ReadTextureObject_t(strm, mo.mo_toTexture);
  }

  // if model object has attachments
  if (strm.PeekID_t() == CChunkID("ATCH")) { // 'attachments'
    // read attachments header
    strm.ExpectID_t("ATCH");  // 'attachments'
    INDEX ctAttachments;
    strm>>ctAttachments;
    // for each attachment
    for(INDEX iAttachment=0; iAttachment<ctAttachments; iAttachment++) {
      // read its position and create the attachment
      INDEX iPosition;
      strm>>iPosition;
      CAttachmentModelObject *pamo = mo.AddAttachmentModel(iPosition);
      if (pamo!=NULL) {
        // read its placement and model
        strm>>pamo->amo_plRelative;
        ReadModelObject_t(strm, pamo->amo_moModelObject);
      } else {
        // skip its placement and model
        CPlacement3D plDummy;
        strm>>plDummy;
        SkipModelObject_t(strm);
      }
    }
  }
}

void SkipModelObject_t(CTStream &strm)
{
  CTFileName fnDummy;
  CModelObject moDummy;
  // skip model data filename
  strm>>fnDummy;
  // skip model object
  moDummy.Read_t(&strm);

  // if model object has multiple textures
  if (strm.PeekID_t() == CChunkID("MTEX")) { // 'multi-texturing'
    // skip all textures
    strm.ExpectID_t("MTEX");  // 'multi-texturing''
    SkipTextureObject_t(strm);  // texture
    SkipTextureObject_t(strm);  // bump
    SkipTextureObject_t(strm);  // reflection
    SkipTextureObject_t(strm);  // specular

  // if model has single texture (old format)
  } else {
    // skip main texture
    SkipTextureObject_t(strm);
  }

  // if model object has attachments
  if (strm.PeekID_t() == CChunkID("ATCH")) { // 'attachments'
    // read attachments header
    strm.ExpectID_t("ATCH");  // 'attachments'
    INDEX ctAttachments;
    strm>>ctAttachments;
    // for each attachment
    for(INDEX iAttachment=0; iAttachment<ctAttachments; iAttachment++) {
      // skip its position, placement and model
      INDEX iPosition;
      strm>>iPosition;
      CPlacement3D plDummy;
      strm>>plDummy;
      SkipModelObject_t(strm);
    }
  }
}

void WriteModelObject_t(CTStream &strm, CModelObject &mo)
{
  // write model data filename
  CAnimData *pad = (CAnimData *)mo.GetData();
  if (pad!=NULL) {
    strm<<pad->GetName();
  } else {
    strm<<CTFileName(CTString(""));
  }
  // write model anim object
  mo.Write_t(&strm);

  // write all textures
  strm.WriteID_t("MTEX");  // 'multi-texturing''
  WriteTextureObject_t(strm, mo.mo_toTexture);
  WriteTextureObject_t(strm, mo.mo_toBump);
  WriteTextureObject_t(strm, mo.mo_toReflection);
  WriteTextureObject_t(strm, mo.mo_toSpecular);

  // if model object has attachments
  if (!mo.mo_lhAttachments.IsEmpty()) {
    // write attachments header
    strm.WriteID_t("ATCH");  // 'attachments'
    strm<<mo.mo_lhAttachments.Count();
    // for each attachment
    FOREACHINLIST( CAttachmentModelObject, amo_lnInMain, mo.mo_lhAttachments, itamo) {
      CAttachmentModelObject *pamo = itamo;
      // write its position, placement and model
      strm<<pamo->amo_iAttachedPosition;
      strm<<pamo->amo_plRelative;
      WriteModelObject_t(strm, pamo->amo_moModelObject);
    }
  }
}

void WriteMeshInstances_t(CTStream &strm, CModelInstance &mi)
{
  // write mesh instance header
  strm.WriteID_t("MSHI");
  // write all mesh instances for this model instance
  INDEX ctmshi=mi.mi_aMeshInst.Count();
  strm<<ctmshi;
  // for each mesh
  for(INDEX imshi=0;imshi<ctmshi;imshi++) {
    MeshInstance &mshi = mi.mi_aMeshInst[imshi];
    CTFileName fnMesh = mshi.mi_pMesh->GetName();
    strm.WriteID_t("MESH");
    // write binary mesh file name
    strm<<fnMesh;

    strm.WriteID_t("MITS");
    // write texture instances for this mesh instance
    INDEX ctti = mshi.mi_tiTextures.Count();
    strm<<ctti;
    // for each texture instance
    for(INDEX iti=0;iti<ctti;iti++) {
      // write texture file name and texture ID
      TextureInstance &ti = mshi.mi_tiTextures[iti];
      CTextureData *ptd = (CTextureData*)ti.ti_toTexture.GetData();
      CTFileName fnTex = ptd->GetName();
      CTString strTexID = ska_GetStringFromTable(ti.GetID());
      strm.WriteID_t("TITX");
      strm<<fnTex;
      strm<<strTexID;
    }
  }
}

void WriteSkeleton_t(CTStream &strm, CModelInstance &mi)
{
  // write this model instance skeleton binary file name
  CSkeleton *pSkeleton = mi.mi_psklSkeleton;
  BOOL bHasSkeleton = (pSkeleton!=NULL);
  strm.WriteID_t("SKEL");
  strm<<bHasSkeleton;
  if(bHasSkeleton) {
    CTFileName fnSkeleton = pSkeleton->GetName();
    strm<<fnSkeleton;
  }
}

void WriteAnimSets(CTStream &strm, CModelInstance &mi)
{
  strm.WriteID_t("ANAS");
  // write animsets file names
  INDEX ctas = mi.mi_aAnimSet.Count();
  strm<<ctas;
  // for each animset in model instance
  for(INDEX ias=0;ias<ctas;ias++) {
    CAnimSet &as = mi.mi_aAnimSet[ias];
    CTFileName fnAnimSet = as.GetName();
    // write animset binary file name
    strm<<fnAnimSet;
  }
}
void WriteColisionBoxes(CTStream &strm, CModelInstance &mi)
{
  strm.WriteID_t("MICB");
  // write colision boxes and index of current colision box
  INDEX ctcb = mi.mi_cbAABox.Count();
  strm<<ctcb;
  // for each colision box
  for(INDEX icb=0;icb<ctcb;icb++) {
    ColisionBox &cb = mi.mi_cbAABox[icb];
    // write colision box
    strm<<cb.Min();
    strm<<cb.Max();
    strm<<cb.GetName();
  }
  // write all frames bounding box
  strm.WriteID_t("AFBB");
  strm<<mi.mi_cbAllFramesBBox.Min();
  strm<<mi.mi_cbAllFramesBBox.Max();
}

void WriteAnimQueue_t(CTStream &strm, CModelInstance &mi)
{
  strm.WriteID_t("MIAQ");  // model instance animation queue
  // write animation queue
  AnimQueue &aq = mi.mi_aqAnims;
  INDEX ctal = aq.aq_Lists.Count();
  strm<<ctal;
  // for each anim list
  for(INDEX ial=0;ial<ctal;ial++) {
    AnimList &al = aq.aq_Lists[ial];

    strm.WriteID_t("AQAL");  // animation queue animation list
    // save anim list and get all played anims
    strm<<al.al_fStartTime;
    strm<<al.al_fFadeTime;
    INDEX ctpa = al.al_PlayedAnims.Count();
    strm<<ctpa;
    // for each played anim
    for(INDEX ipa=0;ipa<ctpa;ipa++) {
      // save played anim
      PlayedAnim &pa = al.al_PlayedAnims[ipa];
      strm.WriteID_t("ALPA");  // animation list played anim
      strm<<pa.pa_fStartTime;
      strm<<pa.pa_ulFlags;
      strm<<pa.pa_Strength;
      strm<<pa.pa_GroupID;
      CTString strPlayedAnimID = ska_GetStringFromTable(pa.pa_iAnimID);
      strm<<strPlayedAnimID;
      // write anim speed mul
      strm.WriteID_t("PASP");  // played animation speed
      strm<<pa.pa_fSpeedMul;
    }
  }
}

void WriteOffsetAndChildren(CTStream &strm, CModelInstance &mi)
{
  strm.WriteID_t("MIOF");  // model instance offset
  // write model instance offset and parent bone
  strm.Write_t(&mi.mi_qvOffset, sizeof(QVect));
  CTString strParenBoneID = ska_GetStringFromTable(mi.mi_iParentBoneID);
  strm<<strParenBoneID;

  strm.WriteID_t("MICH");  // model instance child
  // write model instance children
  INDEX ctcmi = mi.mi_cmiChildren.Count();
  strm<<ctcmi;
  // for each child model instance
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    CModelInstance &cmi = mi.mi_cmiChildren[icmi];
    // save child model instance
    WriteModelInstance_t(strm, cmi);
  }
}

void WriteModelInstance_t(CTStream &strm, CModelInstance &mi)
{
  strm.WriteID_t("MI03"); // model instance 03
  // write model instance name
  strm<<mi.GetName();
  // write index of current colision box
  strm<<mi.mi_iCurentBBox;
  // write stretch
  strm<<mi.mi_vStretch;
  // write color
  strm<<mi.mi_colModelColor;

  WriteMeshInstances_t(strm,mi);
  WriteSkeleton_t(strm, mi);
  WriteAnimSets(strm, mi);
  WriteAnimQueue_t(strm, mi);
  WriteColisionBoxes(strm, mi);
  WriteOffsetAndChildren(strm, mi);
  strm.WriteID_t("ME03"); // model instance end 03
}


void ReadModelInstanceOld_t(CTStream &strm, CModelInstance &mi)
{
  strm.ExpectID_t("SKMI");
  // read model instance name
  CTString strModelInstanceName;
  strm>>strModelInstanceName;
  mi.SetName(strModelInstanceName);

  // read all mesh instances for this model instance
  INDEX ctmshi = 0; // mesh instance count
  INDEX ctti = 0;   // texture instance count
  INDEX ctas = 0;   // animset count
  INDEX ctcb = 0;   // colision boxes count
  INDEX ctal = 0;   // anim lists count
  INDEX ctpa = 0;   // played anims count
  INDEX ctcmi = 0;  // child model instance count

  strm>>ctmshi;
  mi.mi_aMeshInst.New(ctmshi);
  
  // for each mesh
  for(INDEX imshi=0;imshi<ctmshi;imshi++) {
    MeshInstance &mshi = mi.mi_aMeshInst[imshi];
    CTFileName fnMesh;
    // read binary mesh file name
    strm>>fnMesh;
    mshi.mi_pMesh = _pMeshStock->Obtain_t(fnMesh);

    // read texture instances for this mesh instance
    strm>>ctti;
    mshi.mi_tiTextures.New(ctti);
    // for each texture instance
    for(INDEX iti=0;iti<ctti;iti++) {
      // read texture file name and texture ID
      TextureInstance &ti = mshi.mi_tiTextures[iti];

      CTFileName fnTex;
      CTString strTexID;
      strm>>fnTex;
      strm>>strTexID;

      ti.SetName(strTexID);
      ti.ti_toTexture.SetData_t(fnTex);
    }
  }

  // read this model instance skeleton binary file name
  BOOL bHasSkeleton;
  mi.mi_psklSkeleton = NULL;

  strm>>bHasSkeleton;
  if(bHasSkeleton) {
    CTFileName fnSkeleton;
    strm>>fnSkeleton;
    mi.mi_psklSkeleton = _pSkeletonStock->Obtain_t(fnSkeleton);
  }

  // read animsets file names
  strm>>ctas;
  // for each animset in model instance
  for(INDEX ias=0;ias<ctas;ias++) {
    // read animset binary file name
    CTFileName fnAnimSet;
    strm>>fnAnimSet;
    CAnimSet *pas = _pAnimSetStock->Obtain_t(fnAnimSet);
    mi.mi_aAnimSet.Add(pas);
  }

  // read colision boxes
  strm>>ctcb;
  mi.mi_cbAABox.New(ctcb);
  // for each colision box
  for(INDEX icb=0;icb<ctcb;icb++) {
    ColisionBox &cb = mi.mi_cbAABox[icb];
    // read colision box
    strm>>cb.Min();
    strm>>cb.Max();
  }

  // read index of current colision box
  strm>>mi.mi_iCurentBBox;

  // read stretch
  strm>>mi.mi_vStretch;
  // read color
  strm>>mi.mi_colModelColor;

  // read animation queue
  AnimQueue &aq = mi.mi_aqAnims;
  strm>>ctal;
  if(ctal>0) aq.aq_Lists.Push(ctal);
  // for each anim list
  for(INDEX ial=0;ial<ctal;ial++) {
    AnimList &al = aq.aq_Lists[ial];

    // read anim list and get all played anims
    strm>>al.al_fStartTime;
    strm>>al.al_fFadeTime;
    strm>>ctpa;
    if(ctpa>0) al.al_PlayedAnims.Push(ctpa);
    // for each played anim
    for(INDEX ipa=0;ipa<ctpa;ipa++) {
      // save played anim
      PlayedAnim &pa = al.al_PlayedAnims[ipa];
      strm>>pa.pa_fStartTime;
      strm>>pa.pa_ulFlags;
      strm>>pa.pa_Strength;
      strm>>pa.pa_GroupID;
      CTString strPlayedAnimID;
      strm>>strPlayedAnimID;
      pa.pa_iAnimID = ska_GetIDFromStringTable(strPlayedAnimID);
    }
  }

  // read model instance offset and parent bone
  strm.Read_t(&mi.mi_qvOffset, sizeof(QVect));
  CTString strParenBoneID;
  strm>>strParenBoneID;
  mi.mi_iParentBoneID = ska_GetIDFromStringTable(strParenBoneID);

  // read model instance children
  strm>>ctcmi;
  // for each child model instance
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    // create empty model instance
    CModelInstance *pcmi = CreateModelInstance("Temp");
    // add as child to parent model isntance
    mi.mi_cmiChildren.Add(pcmi);
    // read child model instance
    ReadModelInstance_t(strm, *pcmi);
  }
}

void ReadMeshInstances_t(CTStream &strm, CModelInstance &mi)
{
  INDEX ctmshi = 0;
  INDEX ctti = 0;

  strm.ExpectID_t("MSHI");
  // Read mesh instance
  strm>>ctmshi;
  mi.mi_aMeshInst.New(ctmshi);
  // for each mesh
  for(INDEX imshi=0;imshi<ctmshi;imshi++) {
    MeshInstance &mshi = mi.mi_aMeshInst[imshi];
    CTFileName fnMesh;

    strm.ExpectID_t("MESH");
    // read binary mesh file name
    strm>>fnMesh;
    mshi.mi_pMesh = _pMeshStock->Obtain_t(fnMesh);

    // read texture instances for this mesh instance
    strm.ExpectID_t("MITS");  // mesh instance texture instance
    strm>>ctti;
    mshi.mi_tiTextures.New(ctti);
    // for each texture instance
    for(INDEX iti=0;iti<ctti;iti++) {
      // read texture file name and texture ID
      TextureInstance &ti = mshi.mi_tiTextures[iti];
      strm.ExpectID_t("TITX");  // texture instance texture
      CTFileName fnTex;
      CTString strTexID;
      strm>>fnTex;
      strm>>strTexID;

      ti.SetName(strTexID);
      ti.ti_toTexture.SetData_t(fnTex);
    }
  }
}

void ReadSkeleton_t(CTStream &strm, CModelInstance &mi)
{
  // read this model instance skeleton binary file name
  BOOL bHasSkeleton;
  mi.mi_psklSkeleton = NULL;

  strm.ExpectID_t("SKEL");
  strm>>bHasSkeleton;
  if(bHasSkeleton) {
    CTFileName fnSkeleton;
    strm>>fnSkeleton;
    mi.mi_psklSkeleton = _pSkeletonStock->Obtain_t(fnSkeleton);
  }
}

void ReadAnimSets_t(CTStream &strm, CModelInstance &mi)
{
  INDEX ctas = 0;
  strm.ExpectID_t("ANAS");
  // read animsets file names
  strm>>ctas;
  // for each animset in model instance
  for(INDEX ias=0;ias<ctas;ias++) {
    // read animset binary file name
    CTFileName fnAnimSet;
    strm>>fnAnimSet;
    CAnimSet *pas = _pAnimSetStock->Obtain_t(fnAnimSet);
    mi.mi_aAnimSet.Add(pas);
  }
}

void ReadAnimQueue_t(CTStream &strm, CModelInstance &mi)
{
  INDEX ctal = 0;
  INDEX ctpa = 0;
  strm.ExpectID_t("MIAQ");  // model instance animation queue
  // read animation queue
  AnimQueue &aq = mi.mi_aqAnims;
  strm>>ctal;
  if(ctal>0) aq.aq_Lists.Push(ctal);
  // for each anim list
  for(INDEX ial=0;ial<ctal;ial++) {
    AnimList &al = aq.aq_Lists[ial];
    strm.ExpectID_t("AQAL");  // animation queue animation list
    // read anim list and get all played anims
    strm>>al.al_fStartTime;
    strm>>al.al_fFadeTime;
    strm>>ctpa;
    if(ctpa>0) al.al_PlayedAnims.Push(ctpa);
    // for each played anim
    for(INDEX ipa=0;ipa<ctpa;ipa++) {
      // read played anim
      PlayedAnim &pa = al.al_PlayedAnims[ipa];
      strm.ExpectID_t("ALPA");  // animation list played anim
      strm>>pa.pa_fStartTime;
      strm>>pa.pa_ulFlags;
      strm>>pa.pa_Strength;
      strm>>pa.pa_GroupID;
      CTString strPlayedAnimID;
      strm>>strPlayedAnimID;
      pa.pa_iAnimID = ska_GetIDFromStringTable(strPlayedAnimID);
      if(strm.PeekID_t()==CChunkID("PASP")) {
        strm.ExpectID_t("PASP");  // played animation speed
        strm>>pa.pa_fSpeedMul;
      }
    }
  }
}

void ReadColisionBoxes_t(CTStream &strm, CModelInstance &mi)
{
  INDEX ctcb = 0;
  strm.ExpectID_t("MICB");  // model instance colision boxes
  // read colision boxes
  strm>>ctcb;
  mi.mi_cbAABox.New(ctcb);
  // for each colision box
  for(INDEX icb=0;icb<ctcb;icb++) {
    ColisionBox &cb = mi.mi_cbAABox[icb];
    CTString strName;
    // read colision box
    strm>>cb.Min();
    strm>>cb.Max();
    strm>>strName;
    cb.SetName(strName);
  }
  strm.ExpectID_t("AFBB");  // all frames bounding box
  // read all frames bounding box
  strm>>mi.mi_cbAllFramesBBox.Min();
  strm>>mi.mi_cbAllFramesBBox.Max();
}

void ReadOffsetAndChildren_t(CTStream &strm, CModelInstance &mi)
{
  INDEX ctcmi = 0;
  strm.ExpectID_t("MIOF");  // model instance offset
  // read model instance offset and parent bone
  strm.Read_t(&mi.mi_qvOffset, sizeof(QVect));
  CTString strParenBoneID;
  strm>>strParenBoneID;
  mi.mi_iParentBoneID = ska_GetIDFromStringTable(strParenBoneID);

  strm.ExpectID_t("MICH");  // model instance child
  // read model instance children
  strm>>ctcmi;
  // for each child model instance
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    // create empty model instance
    CModelInstance *pcmi = CreateModelInstance("Temp");
    // add as child to parent model isntance
    mi.mi_cmiChildren.Add(pcmi);
    // read child model instance
    ReadModelInstance_t(strm, *pcmi);
  }
}

void ReadModelInstanceNew_t(CTStream &strm, CModelInstance &mi)
{
  strm.ExpectID_t("MI03");  // model instance 02
  // Read model instance name
  CTString strModelInstanceName;
  strm>>strModelInstanceName;
  mi.SetName(strModelInstanceName);

  // read index of current colision box
  strm>>mi.mi_iCurentBBox;
  // read stretch
  strm>>mi.mi_vStretch;
  // read color
  strm>>mi.mi_colModelColor;

  ReadMeshInstances_t(strm,mi);
  ReadSkeleton_t(strm, mi);
  ReadAnimSets_t(strm, mi);
  ReadAnimQueue_t(strm, mi);
  ReadColisionBoxes_t(strm, mi);
  ReadOffsetAndChildren_t(strm, mi);
  strm.ExpectID_t("ME03"); // model instance end 02
}

void ReadModelInstance_t(CTStream &strm, CModelInstance &mi)
{
  // is model instance writen in old format
  if(strm.PeekID_t() == CChunkID("SKMI")) {
    ReadModelInstanceOld_t(strm, mi);
  // is model instance writen in new format
  } else if(strm.PeekID_t() == CChunkID("MI03")) {
    ReadModelInstanceNew_t(strm, mi);
  // unknown format
  } else {
    strm.Throw_t("Unknown model instance format");
    ASSERT(FALSE);
  }
}

void SkipModelInstance_t(CTStream &strm)
{
  CModelInstance miDummy;
  ReadModelInstance_t(strm,miDummy);
}

// read an anim object from a file together with its anim data filename
void ReadAnimObject_t(CTStream &strm, CAnimObject &ao)
{
  // read anim data filename
  CTFileName fnAnim;
  strm>>fnAnim;
  // try to load anim
  for(;;) {
    try {
      // set the anim data
      ao.SetData_t(fnAnim);
      break;
    } catch (const char *strError) {
      (void) strError;
      // if anim was not found, ask for replacing anim
      CTFileName fnReplacingAnim;
      if( GetReplacingFile( fnAnim, fnReplacingAnim, FILTER_ANI FILTER_END)) {
        // replacing anim was provided
        fnAnim = fnReplacingAnim;
      } else {
        ThrowF_t( TRANS("Cannot find substitution for \"%s\""), (const char *) (CTString&)fnAnim);
      }
    }
  }
  // read anim object
  ao.Read_t(&strm);
}

void SkipAnimObject_t(CTStream &strm)
{
  CTFileName fnDummy;
  CAnimObject aoDummy;
  // skip anim data filename
  strm>>fnDummy;
  // skip anim object
  aoDummy.Read_t(&strm);
}

void WriteAnimObject_t(CTStream &strm, CAnimObject &ao)
{
  // write anim data filename
  CAnimData *pad = (CAnimData *)ao.GetData();
  if (pad!=NULL) {
    strm<<pad->GetName();
  } else {
    strm<<CTFileName(CTString(""));
  }
  // write anim object
  ao.Write_t(&strm);
}

// read a sound object from a file together with its sound data filename
// NOTE: sound objects cannot be replaced
void ReadSoundObject_t(CTStream &strm, CSoundObject &so)
{
  so.Read_t(&strm);
}

void SkipSoundObject_t(CTStream &strm)
{
  CSoundObject soDummy;
  soDummy.Read_t(&strm);
}

void WriteSoundObject_t(CTStream &strm, CSoundObject &so)
{
  so.Write_t(&strm);
}
