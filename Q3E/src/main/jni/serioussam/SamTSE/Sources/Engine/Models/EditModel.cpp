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

#include <Engine/Models/ModelObject.h>
#include <Engine/Models/ModelData.h>
#include <Engine/Models/EditModel.h>
#include <Engine/Models/MipMaker.h>
#include <Engine/Math/Geometry.inl>
#include <Engine/Models/Model_internal.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Models/Normals.h>
#include <Engine/Graphics/DrawPort.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/Stock_CTextureData.h>

// Globally instanciated object containing routines for dealing with progres messages
CProgressRoutines ProgresRoutines;

// constants important to this module
#define MAX_ALLOWED_DISTANCE 0.0001f

#define	PC_ALLWAYS_ON (1UL << 30)
#define	PC_ALLWAYS_OFF (1UL << 31)


// origin triangle for transforming object
INDEX aiTransVtx[3] = {-1,-1,-1};

class CExtractSurfaceVertex
{
public:
	INDEX esv_Surface;
	SLONG esv_TextureVertexRemap;
	INDEX esv_MipGlobalIndex;
};

CThumbnailSettings::CThumbnailSettings( void)
{
  ts_bSet = FALSE;
}

void CThumbnailSettings::Read_t( CTStream *strFile)
{
  *strFile>>ts_bSet;
  *strFile>>ts_plLightPlacement;
  *strFile>>ts_plModelPlacement;
	*strFile>>ts_fTargetDistance;
	*strFile>>ts_vTarget;
	*strFile>>ts_angViewerOrientation;
  *strFile>>ts_LightDistance;
  *strFile>>ts_LightColor;
  *strFile>>ts_colAmbientColor;
	*strFile>>ts_PaperColor;
	*strFile>>ts_InkColor;
	*strFile>>ts_IsWinBcgTexture;
	*strFile>>ts_WinBcgTextureName;
  ts_RenderPrefs.Read_t( strFile);
}

void CThumbnailSettings::Write_t( CTStream *strFile)
{
  *strFile<<ts_bSet;
  *strFile<<ts_plLightPlacement;
  *strFile<<ts_plModelPlacement;
	*strFile<<ts_fTargetDistance;
	*strFile<<ts_vTarget;
	*strFile<<ts_angViewerOrientation;
  *strFile<<ts_LightDistance;
  *strFile<<ts_LightColor;
  *strFile<<ts_colAmbientColor;
	*strFile<<ts_PaperColor;
	*strFile<<ts_InkColor;
	*strFile<<ts_IsWinBcgTexture;
	*strFile<<ts_WinBcgTextureName;
  ts_RenderPrefs.Write_t( strFile);
}

CEditModel::CEditModel()
{
  edm_md.md_bIsEdited = TRUE; // this model data is edited
  edm_iActiveCollisionBox = 0;
}

CEditModel::~CEditModel()
{
  FORDELETELIST( CTextureDataInfo, tdi_ListNode, edm_WorkingSkins, litDel3)
  {
    ASSERT( litDel3->tdi_TextureData != NULL);
    _pTextureStock->Release( litDel3->tdi_TextureData);
    delete &litDel3.Current();
  }
}

CProgressRoutines::CProgressRoutines()
{
  SetProgressMessage = NULL;
  SetProgressRange = NULL;
  SetProgressState = NULL;
}


//----------------------------------------------------------------------------------------------
/*
 * This routine loads animation data from opened model script file and converts loaded data
 * to model's frame vertices format
 */

struct VertexNeighbors { CStaticStackArray<INDEX> vp_aiNeighbors; };

void CEditModel::LoadModelAnimationData_t( CTStream *pFile, const FLOATmatrix3D &mStretch) // throw char *
{
  try {
  CObject3D::BatchLoading_t(TRUE);

  INDEX i;
	CObject3D OB3D;
	CListHead FrameNamesList;
	FLOATaabbox3D OneFrameBB;
	FLOATaabbox3D AllFramesBB;

  INDEX ctFramesBefore = edm_md.md_FramesCt;
  edm_md.ClearAnimations();

	OB3D.ob_aoscSectors.Lock();
	// there must be at least one mip model loaded, throw if not
	if( edm_md.md_VerticesCt == 0) {
		throw( "Trying to update model's animations, but model doesn't exists!");
	}
	edm_md.LoadFromScript_t( pFile, &FrameNamesList);		// load model's animation data from script

  // if recreating animations, frame count must be the same
  if( (ctFramesBefore != 0) && (FrameNamesList.Count() != ctFramesBefore) )
  {
		throw( "If you are updating animations, you can't change number of frames. \
      If you want to add or remove some frames or animations, please recreate the model.");
  }
	edm_md.md_FramesCt = FrameNamesList.Count();

	/*
	 * Now we will allocate frames and frames info array and array od 3D objects,
	 * one for each frame.
	 */

  if( ProgresRoutines.SetProgressMessage != NULL) {
    ProgresRoutines.SetProgressMessage( "Calculating bounding boxes ...");
  }
  if( ProgresRoutines.SetProgressRange != NULL) {
    ProgresRoutines.SetProgressRange( FrameNamesList.Count());
  }

  edm_md.md_FrameInfos.New( edm_md.md_FramesCt);
	if( edm_md.md_Flags & MF_COMPRESSED_16BIT) {
    edm_md.md_FrameVertices16.New( edm_md.md_FramesCt * edm_md.md_VerticesCt);
  } else {
    edm_md.md_FrameVertices8.New( edm_md.md_FramesCt * edm_md.md_VerticesCt);
  }

	INDEX iO3D = 0;                        // index used for progress dialog
  CStaticStackArray<FLOAT3D> avVertices; // for caching all vertices in all frames

  BOOL bOrigin = FALSE;
  FLOATmatrix3D mOrientation;
  // set bOrigin if aiTransVtx is valid
  if((aiTransVtx[0] >=0) && (aiTransVtx[1] >=0) && (aiTransVtx[2] >=0))
  {
    bOrigin = TRUE;
  }

  {FOREACHINLIST( CFileNameNode, cfnn_Node, FrameNamesList, itFr)
	{
    CFileNameNode &fnnFileNameNode = itFr.Current();
    if( ProgresRoutines.SetProgressState != NULL) ProgresRoutines.SetProgressState(iO3D);
		OB3D.Clear();
    OB3D.LoadAny3DFormat_t( CTString(itFr->cfnn_FileName), mStretch);
    if( edm_md.md_VerticesCt != OB3D.ob_aoscSectors[0].osc_aovxVertices.Count()) {
			ThrowF_t( "File %s, one of animation frame files has wrong number of points.", 
        (const char *) (CTString)fnnFileNameNode.cfnn_FileName);
		}
    if(bOrigin)
    {
      // calc matrix for vertex transform
      FLOAT3D vY = DOUBLEtoFLOAT(OB3D.ob_aoscSectors[0].osc_aovxVertices[aiTransVtx[2]]-OB3D.ob_aoscSectors[0].osc_aovxVertices[aiTransVtx[0]]);
      FLOAT3D vZ = DOUBLEtoFLOAT(OB3D.ob_aoscSectors[0].osc_aovxVertices[aiTransVtx[0]]-OB3D.ob_aoscSectors[0].osc_aovxVertices[aiTransVtx[1]]);
      FLOAT3D vX = vY*vZ;
      vY = vZ*vX;
      // make a rotation matrix from those vectors
      vX.Normalize();
      vY.Normalize();
      vZ.Normalize();

      mOrientation(1,1) = vX(1); mOrientation(1,2) = vY(1); mOrientation(1,3) = vZ(1);
      mOrientation(2,1) = vX(2); mOrientation(2,2) = vY(2); mOrientation(2,3) = vZ(2);
      mOrientation(3,1) = vX(3); mOrientation(3,2) = vY(3); mOrientation(3,3) = vZ(3);
      mOrientation = !mOrientation;
    }

    // normalize (clear) our Bounding Box
		OB3D.ob_aoscSectors[0].LockAll();
    OneFrameBB = FLOATaabbox3D();				 		 
    // Bounding Box makes union with all points in this frame
    for( i=0; i<edm_md.md_VerticesCt; i++)	{ 
      FLOAT3D vVtx = DOUBLEtoFLOAT( OB3D.ob_aoscSectors[0].osc_aovxVertices[i]);
      if(bOrigin)
      {
        // transform vertex
        vVtx -= DOUBLEtoFLOAT(OB3D.ob_aoscSectors[0].osc_aovxVertices[aiTransVtx[0]]);
        vVtx *= mOrientation;
      }
      OneFrameBB |= FLOATaabbox3D(vVtx);
      avVertices.Push() = vVtx;  // cache vertex
    }
		OB3D.ob_aoscSectors[0].UnlockAll();
    // remember this frame's Bounding Box
		edm_md.md_FrameInfos[iO3D].mfi_Box = OneFrameBB;	
    // make union with Bounding Box of all frames
		AllFramesBB |= OneFrameBB;							
    // load next frame
		iO3D++;																	
  }}

  // calculate stretch vector
	edm_md.md_Stretch = AllFramesBB.Size()/2.0f;       // get size of bounding box

  // correct invalid stretch factors
  if( edm_md.md_Stretch(1) == 0.0f) edm_md.md_Stretch(1) = 1.0f;
  if( edm_md.md_Stretch(2) == 0.0f) edm_md.md_Stretch(2) = 1.0f;
  if( edm_md.md_Stretch(3) == 0.0f) edm_md.md_Stretch(3) = 1.0f;

  // build links from vertices to polygons
  CStaticArray<VertexNeighbors> avnVertices;
  avnVertices.New( edm_md.md_VerticesCt);
  
  // lost 1st frame (one frame is enough because all frames has same poly->edge->vertex links)
	OB3D.Clear();
  const CTString &fnmFirstFrame = LIST_HEAD( FrameNamesList, CFileNameNode, cfnn_Node)->cfnn_FileName;
  OB3D.LoadAny3DFormat_t( fnmFirstFrame, mStretch);
	OB3D.ob_aoscSectors[0].LockAll();

  // loop thru polygons
  INDEX iPolyNo=0;
  {FOREACHINDYNAMICARRAY( OB3D.ob_aoscSectors[0].osc_aopoPolygons, CObjectPolygon, itPoly)
  {
    CObjectPolygon &opo = *itPoly;
    // only triangles are supported!
    ASSERT( opo.opo_PolygonEdges.Count() == 3);  
    if( opo.opo_PolygonEdges.Count() != 3) {
  		ThrowF_t( "Non-triangle polygon encountered in model file %s !", (const char *) fnmFirstFrame);
    }
    // get all 3 vetrices of current polygon and sorted them
    opo.opo_PolygonEdges.Lock();
    CObjectPolygonEdge &opeCurr = opo.opo_PolygonEdges[0];
    CObjectPolygonEdge &opeNext = opo.opo_PolygonEdges[1];
    CObjectVertex *povxCurr, *povxPrev, *povxNext;
    if( !opeCurr.ope_Backward) {
      povxCurr = opeCurr.ope_Edge->oed_Vertex1;
      povxPrev = opeCurr.ope_Edge->oed_Vertex0;
      ASSERT( opeNext.ope_Edge->oed_Vertex0 == povxCurr);
    } else {
      povxCurr = opeCurr.ope_Edge->oed_Vertex0;
      povxPrev = opeCurr.ope_Edge->oed_Vertex1;
      ASSERT( opeNext.ope_Edge->oed_Vertex1 == povxCurr);
    }
    if( !opeNext.ope_Backward) {
      povxNext = opeNext.ope_Edge->oed_Vertex1;
      ASSERT( opeNext.ope_Edge->oed_Vertex0 == povxCurr);
    } else {
      povxNext = opeNext.ope_Edge->oed_Vertex0;
      ASSERT( opeNext.ope_Edge->oed_Vertex1 == povxCurr);
    }
    INDEX iVtx0 = OB3D.ob_aoscSectors[0].osc_aovxVertices.Index(povxPrev);
    INDEX iVtx1 = OB3D.ob_aoscSectors[0].osc_aovxVertices.Index(povxCurr);
    INDEX iVtx2 = OB3D.ob_aoscSectors[0].osc_aovxVertices.Index(povxNext);
    // add neighbor vertices for each of this vertices
    avnVertices[iVtx0].vp_aiNeighbors.Push() = iVtx2;
    avnVertices[iVtx0].vp_aiNeighbors.Push() = iVtx1;
    avnVertices[iVtx1].vp_aiNeighbors.Push() = iVtx0;
    avnVertices[iVtx1].vp_aiNeighbors.Push() = iVtx2;
    avnVertices[iVtx2].vp_aiNeighbors.Push() = iVtx1;
    avnVertices[iVtx2].vp_aiNeighbors.Push() = iVtx0;
    // advance to next poly
    opo.opo_PolygonEdges.Unlock();
    iPolyNo++;
  }}
  // vertex->polygons links created
	OB3D.ob_aoscSectors[0].UnlockAll();


  // cache strecthing reciprocal for faster calc
  FLOAT f1oStretchX, f1oStretchY, f1oStretchZ;
  if( edm_md.md_Flags & MF_COMPRESSED_16BIT) {
    f1oStretchX = 32767.0f / edm_md.md_Stretch(1);
    f1oStretchY = 32767.0f / edm_md.md_Stretch(2);
    f1oStretchZ = 32767.0f / edm_md.md_Stretch(3);
  } else {
    f1oStretchX = 127.0f / edm_md.md_Stretch(1);
    f1oStretchY = 127.0f / edm_md.md_Stretch(2);
    f1oStretchZ = 127.0f / edm_md.md_Stretch(3);
  }

  // remember center vector
  FLOAT3D vCenter = AllFramesBB.Center();  // obtain bbox center
  edm_md.md_vCenter = vCenter;
	
  // prepare progress bar
  if( ProgresRoutines.SetProgressMessage != NULL) {
    ProgresRoutines.SetProgressMessage( "Calculating gouraud normals and stretching vertices ...");
  }
  if( ProgresRoutines.SetProgressRange != NULL) {
    ProgresRoutines.SetProgressRange( edm_md.md_FramesCt);
  }

  // loop thru frames
  iO3D=0;  // index for progress
	INDEX iFVtx=0;  // count for all vertices in all frames
  for( INDEX iFr=0; iFr<edm_md.md_FramesCt; iFr++)
  {
    // calculate all polygon normals for this frame
    if( ProgresRoutines.SetProgressState != NULL) ProgresRoutines.SetProgressState(iO3D);
		for( INDEX iVtx=0; iVtx<edm_md.md_VerticesCt; iVtx++)	// for all vertices in one frame
		{
      FLOAT3D &vVtx = avVertices[iFVtx];
    	if( edm_md.md_Flags & MF_COMPRESSED_16BIT) {
        edm_md.md_FrameVertices16[iFVtx].mfv_SWPoint = SWPOINT3D(
			    FloatToInt( (vVtx(1) - vCenter(1)) * f1oStretchX),
			    FloatToInt( (vVtx(2) - vCenter(2)) * f1oStretchY),
			    FloatToInt( (vVtx(3) - vCenter(3)) * f1oStretchZ) );
      } else {                                                              
        edm_md.md_FrameVertices8[iFVtx].mfv_SBPoint = SBPOINT3D(           
			    FloatToInt( (vVtx(1) - vCenter(1)) * f1oStretchX),
			    FloatToInt( (vVtx(2) - vCenter(2)) * f1oStretchY),
			    FloatToInt( (vVtx(3) - vCenter(3)) * f1oStretchZ) );
      }

			// calculate vector of gouraud normal in this vertice
      ANGLE   aSum = 0;
			FLOAT3D vSum( 0.0f, 0.0f, 0.0f);
      INDEX iFrOffset = edm_md.md_VerticesCt * iFr;
      VertexNeighbors &vnCurr = avnVertices[iVtx];
      for( INDEX iNVtx=0; iNVtx<vnCurr.vp_aiNeighbors.Count(); iNVtx+=2) { // loop thru neighbors
        INDEX iPrev = vnCurr.vp_aiNeighbors[iNVtx+0];
        INDEX iNext = vnCurr.vp_aiNeighbors[iNVtx+1];
        FLOAT3D v0  = avVertices[iPrev+iFrOffset] - vVtx;
        FLOAT3D v1  = avVertices[iNext+iFrOffset] - vVtx;
        v0.Normalize();
        v1.Normalize();
        FLOAT3D v = v1*v0;
        FLOAT fLength = v.Length();
        ANGLE a = ASin(fLength);
        //ASSERT( a>=0 && a<=180);
        aSum += a;
        vSum += (v/fLength) * a;
			}

      // normalize sum of polygon normals
      //ASSERT( aSum>=0);
      vSum /= aSum;
      vSum.Normalize();

      // save compressed gouraud normal
    	if( edm_md.md_Flags & MF_COMPRESSED_16BIT) {
        CompressNormal_HQ( vSum, edm_md.md_FrameVertices16[iFVtx].mfv_ubNormH,
                                 edm_md.md_FrameVertices16[iFVtx].mfv_ubNormP);
      } else {
			  edm_md.md_FrameVertices8[iFVtx].mfv_NormIndex = (UBYTE)GouraudNormal(vSum);
      }

      // advance to next vertex in model
			iFVtx++;
		}

    // advance to next frame
    iO3D++;
  }

  // list of filenames is no longer needed
	FORDELETELIST( CFileNameNode, cfnn_Node, FrameNamesList, litDel) delete &litDel.Current();

  // create compressed vector center that will be used for setting object handle
  edm_md.md_vCompressedCenter(1) = -edm_md.md_vCenter(1) * f1oStretchX;
  edm_md.md_vCompressedCenter(2) = -edm_md.md_vCenter(2) * f1oStretchY;
  edm_md.md_vCompressedCenter(3) = -edm_md.md_vCenter(3) * f1oStretchZ;

  // adjust stretching for compressed format
  if( edm_md.md_Flags & MF_COMPRESSED_16BIT) {
    edm_md.md_Stretch(1) /= 32767.0f; 
    edm_md.md_Stretch(2) /= 32767.0f;
    edm_md.md_Stretch(3) /= 32767.0f;
  } else {
    edm_md.md_Stretch(1) /= 127.0f; 
    edm_md.md_Stretch(2) /= 127.0f;
    edm_md.md_Stretch(3) /= 127.0f;
  }

  // all done
	OB3D.ob_aoscSectors.Unlock();

  CObject3D::BatchLoading_t(FALSE);
  } catch (const char*) {
  CObject3D::BatchLoading_t(FALSE);
  throw;
  }
}


//--------------------------------------------------------------------------------------------
/*
 * Routine saves model's .h file (#define ......)
 */
void CEditModel::SaveIncludeFile_t( CTFileName fnFileName, CTString strDefinePrefix) // throw char *
{
  CTFileStream strmHFile;
  char line[ 1024];
  INDEX i;

  strmHFile.Create_t( fnFileName, CTStream::CM_TEXT);
  strcpy( line, strDefinePrefix);
  strupr( line);
  strDefinePrefix = CTString( line);

  sprintf( line, "// Animation names\n");
  strmHFile.Write_t( line, strlen( line));
  // force animation prefix string to be upper case
  char achrUprName[ 256];
  strcpy( achrUprName, strDefinePrefix);
  strcat( achrUprName, "_ANIM_");
  CTString strAnimationPrefix = achrUprName;
  edm_md.ExportAnimationNames_t( &strmHFile, achrUprName);

  sprintf( line, "\n// Color names\n");
  strmHFile.Write_t( line, strlen( line));

  for( i=0; i<MAX_COLOR_NAMES; i++)
  {
    if( edm_md.md_ColorNames[ i] != "")
    {
      sprintf( line, "#define %s_PART_%s ((1L) << %d)\n", (const char *) strDefinePrefix, (const char *) edm_md.md_ColorNames[ i], i);
      strmHFile.Write_t( line, strlen( line));
    }
  }
  sprintf( line, "\n// Patch names\n");
  strmHFile.Write_t( line, strlen( line));

  for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    CTString strPatchName = edm_md.md_mpPatches[ iPatch].mp_strName;
    if( strPatchName != "")
    {
      sprintf( line, "#define %s_PATCH_%s %d\n", (const char *) strDefinePrefix, (const char *) strPatchName, i);
      strmHFile.Write_t( line, strlen( line));
    }
  }
  // save collision box names
  sprintf( line, "\n// Names of collision boxes\n");
  strmHFile.Write_t( line, strlen( line));

  edm_md.md_acbCollisionBox.Lock();
  // save all collision boxes
  for( INDEX iCollisionBox=0; iCollisionBox<edm_md.md_acbCollisionBox.Count(); iCollisionBox++)
  {
    // prepare collision box name as define
    sprintf( line, "#define %s_COLLISION_BOX_%s %d\n", (const char *) strDefinePrefix, (const char *) GetCollisionBoxName( iCollisionBox),
      iCollisionBox);
    strmHFile.Write_t( line, strlen( line));
  }
  edm_md.md_acbCollisionBox.Unlock();

  // save all attaching positions
  sprintf( line, "\n// Attaching position names\n");
  strmHFile.Write_t( line, strlen( line));
  INDEX iAttachingPlcement = 0;
  FOREACHINDYNAMICARRAY(edm_aamAttachedModels, CAttachedModel, itam)
  {
    char achrUpper[ 256];
    strcpy( achrUpper, itam->am_strName);
    strupr( achrUpper);
    sprintf( line, "#define %s_ATTACHMENT_%s %d\n", (const char *) strDefinePrefix, achrUpper, iAttachingPlcement);
    strmHFile.Write_t( line, strlen( line));
    iAttachingPlcement++;
  }
  sprintf( line, "\n// Sound names\n");
  strmHFile.Write_t( line, strlen( line));

  for( INDEX iSound=0; iSound<edm_aasAttachedSounds.Count(); iSound++)
  {
    if( edm_aasAttachedSounds[iSound].as_fnAttachedSound != "")
    {
      CTString strLooping;
      if( edm_aasAttachedSounds[iSound].as_bLooping) strLooping = "L";
      else                                           strLooping = "NL";

      CTString strDelay = "";
      if( edm_aasAttachedSounds[iSound].as_fDelay == 0.0f)
        strDelay = "0.0f";
      else
        strDelay.PrintF( "%gf", edm_aasAttachedSounds[iSound].as_fDelay);

      CAnimInfo aiInfo;
      edm_md.GetAnimInfo( iSound, aiInfo);

      CTString strWithQuotes;
      strWithQuotes.PrintF( "\"%s\",", (const char *) CTString(edm_aasAttachedSounds[iSound].as_fnAttachedSound));

      sprintf( line, "//sound SOUND_%s_%-16s %-32s // %s, %s, %s\n",
        (const char *) strDefinePrefix,
        aiInfo.ai_AnimName,
        (const char *) strWithQuotes,
        (const char *) (strAnimationPrefix+aiInfo.ai_AnimName),
        (const char *) strLooping,
        (const char *) strDelay);
      strmHFile.Write_t( line, strlen( line));
    }
  }

  strmHFile.Close();
}

// overloaded save function
void CEditModel::Save_t( CTFileName fnFileName) // throw char *
{
  CTFileName fnMdlFileName = fnFileName.FileDir() + fnFileName.FileName() + ".mdl";
  edm_md.Save_t( fnMdlFileName);

  CTFileName fnHFileName = fnFileName.FileDir() + fnFileName.FileName() + ".h";
  CTString strPrefix = fnFileName.FileName();
  if (strPrefix.Length()>0 && !isalpha(strPrefix[0]) && strPrefix[0]!='_') {
    strPrefix="_"+strPrefix;
  }
  SaveIncludeFile_t( fnHFileName, strPrefix);

  CTFileName fnIniFileName = fnFileName.FileDir() + fnFileName.FileName() + ".ini";
  CSerial::Save_t( fnIniFileName);
}

// overloaded load function
void CEditModel::Load_t( CTFileName fnFileName)
{
  CTFileName fnMdlFileName = fnFileName.FileDir() + fnFileName.FileName() + ".mdl";
  edm_md.Load_t( fnMdlFileName);

  CTFileName fnIniFileName = fnFileName.FileDir() + fnFileName.FileName() + ".ini";
  // try to load ini file
  try
  {
    CSerial::Load_t( fnIniFileName);
  }
  catch (const char *strError)
  {
    // ignore errors
    (void) strError;
    CreateEmptyAttachingSounds();
  }
}

CTextureDataInfo *CEditModel::AddTexture_t(const CTFileName &fnFileName, const MEX mexWidth,
                            const MEX mexHeight)
{
  CTextureDataInfo *pNewTDI = new CTextureDataInfo;
  pNewTDI->tdi_FileName = fnFileName;

  try
  {
    pNewTDI->tdi_TextureData = _pTextureStock->Obtain_t( pNewTDI->tdi_FileName);
  }
  catch (const char *strError)
  {
    (void) strError;
    delete pNewTDI;
    return NULL;
  }

  // reload the texture
  pNewTDI->tdi_TextureData->Reload();

  edm_WorkingSkins.AddTail( pNewTDI->tdi_ListNode);
  return pNewTDI;
}

CAttachedModel::CAttachedModel(void)
{
  am_strName = "No name";
  am_iAnimation = 0;
  am_bVisible = TRUE;
}

CAttachedModel::~CAttachedModel(void)
{
  Clear();
}

void CAttachedModel::Clear(void)
{
  am_moAttachedModel.mo_toTexture.SetData(NULL);
  am_moAttachedModel.mo_toReflection.SetData(NULL);
  am_moAttachedModel.mo_toSpecular.SetData(NULL);
  am_moAttachedModel.mo_toBump.SetData(NULL);
  am_moAttachedModel.SetData(NULL);
}

void CAttachedModel::Read_t( CTStream *pstrmFile) // throw char *
{
  *pstrmFile >> am_bVisible;
  *pstrmFile >> am_strName;
  // this data is used no more
  CTFileName fnModel, fnDummy;
  *pstrmFile >> fnModel;
  
  // new attached model format has saved index of animation
  if( pstrmFile->PeekID_t() == CChunkID("AMAN"))
  {
    pstrmFile->ExpectID_t( CChunkID( "AMAN"));
    *pstrmFile >> am_iAnimation;
  }
  else
  {
    *pstrmFile >> fnDummy; // ex model's texture
  }

  try
  {
    SetModel_t( fnModel);
  }
  catch (const char *strError)
  {
    (void) strError;
    try
    {
      SetModel_t( CTFILENAME("Models\\Editor\\Axis.mdl"));
    }
    catch (const char *strError)
    {
      FatalError( strError);
    }
  }
}

void CAttachedModel::Write_t( CTStream *pstrmFile) // throw char *
{
  *pstrmFile << am_bVisible;
  *pstrmFile << am_strName;
  *pstrmFile << am_moAttachedModel.GetName();

  // new attached model format has saved index of animation
  pstrmFile->WriteID_t( CChunkID("AMAN"));
  *pstrmFile << am_iAnimation;
}

void CAttachedModel::SetModel_t(CTFileName fnModel)
{
  am_moAttachedModel.SetData_t(fnModel);
  am_moAttachedModel.AutoSetTextures();
}

CAttachedSound::CAttachedSound( void)
{
  as_fDelay = 0.0f;
  as_fnAttachedSound = CTString("");
  as_bLooping = FALSE;
  as_bPlaying = TRUE;
}

void CAttachedSound::Read_t(CTStream *strFile)
{
  *strFile>>as_bLooping;
  *strFile>>as_bPlaying;
  *strFile>>as_fnAttachedSound;
  *strFile>>as_fDelay;
}

void CAttachedSound::Write_t(CTStream *strFile)
{
  *strFile<<as_bLooping;
  *strFile<<as_bPlaying;
  *strFile<<as_fnAttachedSound;
  *strFile<<as_fDelay;
}

void CEditModel::CreateEmptyAttachingSounds( void)
{
  ASSERT( edm_md.GetAnimsCt() > 0);
  edm_aasAttachedSounds.Clear();
  edm_aasAttachedSounds.New( edm_md.GetAnimsCt());
}

void CEditModel::Read_t( CTStream *pFile) // throw char *
{
  CTFileName fnFileName;
  INDEX i, iWorkingTexturesCt;

  pFile->ExpectID_t( CChunkID( "WTEX"));
  *pFile >> iWorkingTexturesCt;

  for( i=0; i<iWorkingTexturesCt; i++)
  {
    *pFile >> fnFileName;
    try
    {
      AddTexture_t( fnFileName, edm_md.md_Width, edm_md.md_Height);
    }
    // This is here because we want to load model even if its texture is not valid
    catch (const char *err_str){ (char *) err_str;}
  }

  // skip patches saved in old format (patches do not exist inside EditModel any more)
  if( pFile->PeekID_t() == CChunkID("PATM"))
  {
    pFile->GetID_t();
    ULONG ulDummySizeOfLong;
    ULONG ulOldExistingPatches;

    *pFile >> ulDummySizeOfLong;
    *pFile >> ulOldExistingPatches;
    for( i=0; i<MAX_TEXTUREPATCHES; i++)
    {
      if( ((1UL << i) & ulOldExistingPatches) != 0)
      {
        CTFileName fnPatchName;
        *pFile >> fnPatchName;
      }
    }
  }

  // try to load attached models
  try
  {
    pFile->ExpectID_t( CChunkID( "ATTM"));
    INDEX ctSavedModels;
    *pFile >> ctSavedModels;

    // clamp no of saved attachments to no of model's data attached positions
    INDEX ctMDAttachments = edm_md.md_aampAttachedPosition.Count();
    INDEX ctToLoad = ClampUp( ctSavedModels, ctMDAttachments);
    INDEX ctToSkip = ctSavedModels - ctToLoad;

    // add attached models
    edm_aamAttachedModels.Clear();
    if( ctToLoad != 0)
    {
      edm_aamAttachedModels.New( ctSavedModels);
      // read all attached models
      FOREACHINDYNAMICARRAY(edm_aamAttachedModels, CAttachedModel, itam)
      {
        itam->Read_t(pFile);
      }
    }

    // skip unused attached models
    for( INDEX iSkip=0; iSkip<ctToSkip; iSkip++)
    {
      CAttachedModel atmDummy;
      atmDummy.Read_t(pFile);
    }
  }
  catch (const char *strError)
  {
    (void) strError;
    // clear attached models
    edm_aamAttachedModels.Clear();
    edm_md.md_aampAttachedPosition.Clear();
  }

  CreateEmptyAttachingSounds();
  // try to load attached sounds
  try
  {
    pFile->ExpectID_t( CChunkID( "ATSD"));
    INDEX ctAttachedSounds;
    *pFile >> ctAttachedSounds;
    INDEX ctExisting = edm_aasAttachedSounds.Count();
    INDEX ctToRead = ClampUp( ctAttachedSounds, ctExisting);

    // read all saved attached sounds
    for( INDEX iSound=0; iSound<ctToRead; iSound++)
    {
      CAttachedSound &as = edm_aasAttachedSounds[ iSound];
      as.Read_t(pFile);
    }

    // skipped saved but now obsolite
    INDEX ctToSkip = ctAttachedSounds - ctToRead;
    for( INDEX iSkip=0; iSkip<ctToSkip; iSkip++)
    {
      CAttachedSound asDummy;
      asDummy.Read_t(pFile);
    }
  }
  catch (const char *strError)
  {
    (void) strError;
  }

  try
  {
    // load last taken thumbnail settings
    pFile->ExpectID_t( CChunkID( "TBST"));
    edm_tsThumbnailSettings.Read_t( pFile);
  }
  catch (const char *strError)
  {
    // ignore errors
    (void) strError;
  }

  // load names of effect textures
  // --- specular texture
  try {
    pFile->ExpectID_t( CChunkID( "FXTS"));
    *pFile >> edm_fnSpecularTexture;
  } catch (const char *strError) { (void) strError; }

  // --- reflection texture
  try {
    pFile->ExpectID_t( CChunkID( "FXTR"));
    *pFile >> edm_fnReflectionTexture;
  } catch (const char *strError) { (void) strError; }

  // --- bump texture
  try {
    pFile->ExpectID_t( CChunkID( "FXTB"));
    *pFile >> edm_fnBumpTexture;
  } catch (const char *strError) { (void) strError; }
}

void CEditModel::Write_t( CTStream *pFile) // throw char *
{
  pFile->WriteID_t( CChunkID( "WTEX"));

  INDEX iWorkingTexturesCt = edm_WorkingSkins.Count();
  *pFile << iWorkingTexturesCt;

  FOREACHINLIST( CTextureDataInfo, tdi_ListNode, edm_WorkingSkins, it)
  {
    *pFile << it->tdi_FileName;
  }

  // CEditModel class has no patches in new patch data format

  pFile->WriteID_t( CChunkID( "ATTM"));
  INDEX ctAttachedModels = edm_aamAttachedModels.Count();
  *pFile << ctAttachedModels;
  // write all attached models
  FOREACHINDYNAMICARRAY(edm_aamAttachedModels, CAttachedModel, itam)
  {
    itam->Write_t(pFile);
  }

  pFile->WriteID_t( CChunkID( "ATSD"));
  INDEX ctAttachedSounds = edm_aasAttachedSounds.Count();
  *pFile << ctAttachedSounds;
  // write all attached models
  FOREACHINSTATICARRAY(edm_aasAttachedSounds, CAttachedSound, itas)
  {
    itas->Write_t(pFile);
  }

  // save last taken thumbnail settings
  pFile->WriteID_t( CChunkID( "TBST"));
  edm_tsThumbnailSettings.Write_t( pFile);

  // save names of effect textures
  // --- specular texture
  pFile->WriteID_t( CChunkID( "FXTS"));
  *pFile << edm_fnSpecularTexture;
  // --- reflection texture
  pFile->WriteID_t( CChunkID( "FXTR"));
  *pFile << edm_fnReflectionTexture;
  // --- bump texture
  pFile->WriteID_t( CChunkID( "FXTB"));
  *pFile << edm_fnBumpTexture;
}
//----------------------------------------------------------------------------------------------
/*
 * Routine saves defult script file containing only one animation with default data
 * Input file name is .LWO file name, not .SCR
 */
void CEditModel::CreateScriptFile_t(CTFileName &fnO3D) // throw char *
{
  CTFileName fnScriptName = fnO3D.FileDir() + fnO3D.FileName() + ".scr";
  CTFileStream File;
  char line[ 256];

  File.Create_t( fnScriptName, CTStream::CM_TEXT);
  File.PutLine_t( ";******* Creation settings");
  File.PutLine_t( "TEXTURE_DIM 2.0 2.0");
  File.PutLine_t( "SIZE 1.0");
  File.PutLine_t( "MAX_SHADOW 0");
  File.PutLine_t( "HI_QUALITY YES");
  File.PutLine_t( "FLAT NO");
  File.PutLine_t( "HALF_FLAT NO");
  File.PutLine_t( "STRETCH_DETAIL NO");
  File.PutLine_t( "");
  File.PutLine_t( ";******* Mip models");
  sprintf( line, "DIRECTORY %s", (const char *) (const CTString&)fnO3D.FileDir());
  File.PutLine_t( line);
  File.PutLine_t( "MIP_MODELS 1");
  sprintf( line, "    %s", (const char *) (const CTString&)(fnO3D.FileName() + fnO3D.FileExt()));
  File.PutLine_t( line);
  File.PutLine_t( "");
  File.PutLine_t( "ANIM_START");
  File.PutLine_t( ";******* Start of animation block");
  File.PutLine_t( "");
  sprintf( line, "DIRECTORY %s", (const char *) (const CTString&)fnO3D.FileDir());
  File.PutLine_t( line);
  File.PutLine_t( "ANIMATION Default");
  File.PutLine_t( "SPEED 0.1");
  sprintf( line, "    %s", (const char *) (const CTString&)(fnO3D.FileName() + fnO3D.FileExt()));
  File.PutLine_t( line);
  File.PutLine_t( "");
  File.PutLine_t( ";******* End of animation block");
  File.PutLine_t( "ANIM_END");
  File.PutLine_t( "");
  File.PutLine_t( "END");
  File.Close();
}
//----------------------------------------------------------------------------------------------
/*
 * This routine load lines from script file and executes appropriate actions
 */
#define EQUAL_SUB_STR( str) (strnicmp( ld_line, str, strlen(str)) == 0)

void CEditModel::LoadFromScript_t(CTFileName &fnScriptName) // throw char *
{
  try {
  CObject3D::BatchLoading_t(TRUE);

  INDEX i;
  CTFileStream File;
	CObject3D O3D;
  CTFileName fnOpened, fnClosed, fnUnwrapped, fnImportMapping;
	char ld_line[ 128];
	char flag_str[ 128];
	char base_path[ PATH_MAX] = "";
	char file_name[ PATH_MAX];
  char mapping_file_name[ PATH_MAX] = "";
	char full_path[ PATH_MAX];
  FLOATmatrix3D mStretch;
  mStretch.Diagonal(1.0f);
	BOOL bMappingDimFound ;
	BOOL bAnimationsFound;
  BOOL bLoadInitialMapping;

	O3D.ob_aoscSectors.Lock();
	File.Open_t( fnScriptName);				// open script file for reading

	// if these flags will not be TRUE at the end of script, throw error
	bMappingDimFound = FALSE;
	bAnimationsFound = FALSE;
  bLoadInitialMapping = FALSE;

  // to hold number of line's chars
  int iLineChars;
	FOREVER
	{
		do
    {
      File.GetLine_t(ld_line, 128);
      iLineChars = strlen( ld_line);
    }
		while( (iLineChars == 0) || (ld_line[0]==';') );

		// If key-word is "DIRECTORY", remember base path it and add "\" character at the
		// end of new path if it is not yet there
		if( EQUAL_SUB_STR( "DIRECTORY"))
		{
			_strupr( ld_line);
      sscanf( ld_line, "DIRECTORY %s", base_path);
			if( base_path[ strlen( base_path) - 1] != '\\')
				strcat( base_path,"\\");
		}
		// Key-word "SIZE" defines stretch factor
		else if( EQUAL_SUB_STR( "SIZE"))
		{
  	  _strupr( ld_line);
      FLOAT fStretch = 1.0f;
		  sscanf( ld_line, "SIZE %g", &fStretch);
      mStretch *= fStretch;
		}
		else if( EQUAL_SUB_STR( "TRANSFORM")) 
    {
  	  _strupr( ld_line);
      FLOATmatrix3D mTran;
      mTran.Diagonal(1.0f);
		  sscanf( ld_line, "TRANSFORM %g %g %g %g %g %g %g %g %g", 
        &mTran(1,1), &mTran(1,2), &mTran(1,3),
        &mTran(2,1), &mTran(2,2), &mTran(2,3),
        &mTran(3,1), &mTran(3,2), &mTran(3,3));
      mStretch *= mTran;
    }
		// Key-word "FLAT" means that model will be mapped as face - forward, using only
    // zooming of texture
		else if( EQUAL_SUB_STR( "FLAT"))
		{
  	  _strupr( ld_line);
		  sscanf( ld_line, "FLAT %s", flag_str);
      if( strcmp( flag_str, "YES") == 0)
      {
        edm_md.md_Flags |= MF_FACE_FORWARD;
        edm_md.md_Flags &= ~MF_HALF_FACE_FORWARD;
      }
		}
		else if( EQUAL_SUB_STR( "HALF_FLAT"))
		{
  	  _strupr( ld_line);
		  sscanf( ld_line, "HALF_FLAT %s", flag_str);
      if( strcmp( flag_str, "YES") == 0)
        edm_md.md_Flags |= MF_FACE_FORWARD|MF_HALF_FACE_FORWARD;
		}
    else if( EQUAL_SUB_STR( "STRETCH_DETAIL"))
    {
  	  _strupr( ld_line);
		  sscanf( ld_line, "STRETCH_DETAIL %s", flag_str);
      if( strcmp( flag_str, "YES") == 0)
      {
        edm_md.md_Flags |= MF_STRETCH_DETAIL;
      }
    }
		else if( EQUAL_SUB_STR( "HI_QUALITY"))
		{
  	  _strupr( ld_line);
		  sscanf( ld_line, "HI_QUALITY %s", flag_str);
      if( strcmp( flag_str, "YES") == 0)
      {
        edm_md.md_Flags |= MF_COMPRESSED_16BIT;
      }
		}
		// Key-word "REFLECTIONS" has been used in old reflections
		else if( EQUAL_SUB_STR( "REFLECTIONS"))
    {
    }
		// Key-word "MAX_SHADOW" determines maximum quality of shading that model can obtain
		else if( EQUAL_SUB_STR( "MAX_SHADOW"))
    {
  	  _strupr( ld_line);
		  INDEX iShadowQuality;
      sscanf( ld_line, "MAX_SHADOW %d", &iShadowQuality);
      edm_md.md_ShadowQuality = iShadowQuality;
    }
		// Key-word "MipModel" must follow name of this mipmodel file
    else if( EQUAL_SUB_STR( "MIP_MODELS"))
    {
			INDEX iMipCt;
      sscanf( ld_line, "MIP_MODELS %d", &iMipCt);
      if( (iMipCt <= 0) || (iMipCt >= MAX_MODELMIPS))
			{
				ThrowF_t("Invalid number of mip models. Number must range from 0 to %d.", MAX_MODELMIPS-1);
			}
      if( ProgresRoutines.SetProgressMessage != NULL)
        ProgresRoutines.SetProgressMessage( "Loading and creating mip-models ...");
      if( ProgresRoutines.SetProgressRange != NULL)
        ProgresRoutines.SetProgressRange( iMipCt);
      for( i=0; i<iMipCt; i++)
      {
        if( ProgresRoutines.SetProgressState != NULL)
          ProgresRoutines.SetProgressState( i);
		    do
        {
          File.GetLine_t(ld_line, 128);
        }
		    while( (strlen( ld_line)== 0) || (ld_line[0]==';'));
			  _strupr( ld_line);
			  sscanf( ld_line, "%s", file_name);
			  sprintf( full_path, "%s%s", base_path, file_name);
        // remember name of first mip to define UV mapping
        if( i==0)
        {
          fnImportMapping = CTString( full_path);
        }
			  O3D.Clear();                            // clear possible existing O3D's data
        O3D.LoadAny3DFormat_t( CTString(full_path), mStretch);
			  if( edm_md.md_VerticesCt == 0)					// If there are no vertices in model, call New Model
			  {
				  if( bMappingDimFound == FALSE)
				  {
					  ThrowF_t("Found key word \"MIP_MODELS\" but texture dimension wasn't found.\n"
					    "There must be key word \"TEXTURE_DIM\" before key word \"MIP_MODELS\" in script file.");
				  }
				  NewModel( &O3D);
			  }
        else
        {
          O3D.ob_aoscSectors[0].LockAll();
				  AddMipModel( &O3D);										// else this is one of model's mip definitions so call Add Mip Model
	        O3D.ob_aoscSectors[0].UnlockAll();
        }
		  }
      // set default mip factors
      // all mip models will be spreaded beetween distance 0 and default maximum distance
      edm_md.SpreadMipSwitchFactors( 0, 5.0f);
    }
		// Key-word "DEFINE_MAPPING" must follow three lines with names of files used to define mapping
    else if( EQUAL_SUB_STR( "DEFINE_MAPPING"))
    {
      if( edm_md.md_VerticesCt == 0)
			{
				ThrowF_t("Found key word \"DEFINE_MAPPING\" but model is not yet created.");
			}

      File.GetLine_t(ld_line, 128);
  	  sscanf( ld_line, "%s", file_name);
      sprintf( full_path, "%s%s", base_path, file_name);
      fnOpened = CTString( full_path);

      File.GetLine_t(ld_line, 128);
  	  sscanf( ld_line, "%s", file_name);
      sprintf( full_path, "%s%s", base_path, file_name);
      fnClosed = CTString( full_path);

      File.GetLine_t(ld_line, 128);
  	  sscanf( ld_line, "%s", file_name);
      sprintf( full_path, "%s%s", base_path, file_name);
      fnUnwrapped = CTString( full_path);
    }
    else if( EQUAL_SUB_STR( "IMPORT_MAPPING"))
    {
      if( edm_md.md_VerticesCt == 0)
			{
				ThrowF_t("Found key word \"IMPORT_MAPPING\" but model is not yet created.");
			}
      File.GetLine_t(ld_line, 128);
  	  sscanf( ld_line, "%s", file_name);
      sprintf( full_path, "%s%s", base_path, file_name);
      fnImportMapping = CTString( full_path);
    }
		/*
		 * Line containing key-word "TEXTURE_DIM" gives us texture dimensions
		 * so we can create default mapping
		 */
		else if( EQUAL_SUB_STR( "TEXTURE_DIM"))
		{
			_strupr( ld_line);
			FLOAT fWidth, fHeight;
      sscanf( ld_line, "TEXTURE_DIM %f %f", &fWidth, &fHeight);	// read given texture dimensions
			edm_md.md_Width = MEX_METERS( fWidth);
      edm_md.md_Height = MEX_METERS( fHeight);
      bMappingDimFound = TRUE;
		}
		// Key-word "ANIM_START" starts loading of Animation Data object
		else if( EQUAL_SUB_STR( "ANIM_START"))
		{
			LoadModelAnimationData_t( &File, mStretch);	// loads and sets model's animation data
      // add one collision box
      edm_md.md_acbCollisionBox.New();
      // reset attaching sounds
      CreateEmptyAttachingSounds();
			bAnimationsFound = TRUE;				// mark that we found animations section in script-file
		}
    else if( EQUAL_SUB_STR( "ORIGIN_TRI"))
    {
      sscanf( ld_line, "ORIGIN_TRI %d %d %d", &aiTransVtx[0], &aiTransVtx[1], &aiTransVtx[2]);	// read given vertices
    }
		// Key-word "END" ends infinite loop and script loading is over
		else if( EQUAL_SUB_STR( "END"))
		{
			break;
		}
    // ignore old key-words
    else if( EQUAL_SUB_STR( "MAPPING")) {}
		else if( EQUAL_SUB_STR( "TEXTURE_REFLECTION")) {}
		else if( EQUAL_SUB_STR( "TEXTURE_SPECULAR")) {}
		else if( EQUAL_SUB_STR( "TEXTURE_BUMP")) {}
		else if( EQUAL_SUB_STR( "TEXTURE")) {}
		// If none of known key-words isnt recognised, we have wierd key-word, so throw error
		else
		{
      ThrowF_t("Unrecognizible key-word found in line: \"%s\".", ld_line);
		}
	}
	/*
	 * At the end we check if we found animations in script file and if initial mapping was done
	 * during loading of script file what means that key-word 'TEXTURE_DIM' was found
	 */
	if( bAnimationsFound != TRUE)
		throw( "There are no animations defined for this model, and that can't be. Probable cause: script missing key-word \"ANIM_START\".");

	if( bMappingDimFound != TRUE)
		throw( "Initial mapping not done, and that can't be. Probable cause: script missing key-word \"TEXTURE_DIM\".");

  edm_md.LinkDataForSurfaces(TRUE);

  // try to
  try
  {
    // load mapping
    LoadMapping_t( CTString(fnScriptName.NoExt()+".map"));
  }
  // if not successful
  catch (const char *strError)
  {
    // ignore error message
    (void)strError;
  }
  
  // import mapping
  if( (fnImportMapping != "") ||
      ((fnClosed != "") && (fnOpened != "") && (fnUnwrapped != "")) )
  {
    CObject3D o3dClosed, o3dOpened, o3dUnwrapped;
	  
    o3dClosed.Clear();
    o3dOpened.Clear();
    o3dUnwrapped.Clear();

    // if mapping is defined using three files
    if( (fnClosed != "") && (fnOpened != "") && (fnUnwrapped != "") )
    {
      o3dClosed.LoadAny3DFormat_t( fnOpened, mStretch);
      o3dOpened.LoadAny3DFormat_t( fnClosed, mStretch);
      o3dUnwrapped.LoadAny3DFormat_t( fnUnwrapped, mStretch);
    }
    // if mapping is defined using one file
    else
    {
      o3dClosed.LoadAny3DFormat_t( fnImportMapping, mStretch, CObject3D::LT_NORMAL);
      o3dOpened.LoadAny3DFormat_t( fnImportMapping, mStretch, CObject3D::LT_OPENED);
      o3dUnwrapped.LoadAny3DFormat_t( fnImportMapping, mStretch, CObject3D::LT_UNWRAPPED);

      // multiply coordinates with size of texture
      o3dUnwrapped.ob_aoscSectors.Lock();
      o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Lock();
      INDEX ctVertices = o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Count();
      for(INDEX ivtx=0; ivtx<ctVertices; ivtx++)
      {
        o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices[ivtx](1) *= edm_md.md_Width/1024.0f;
        o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices[ivtx](2) *= edm_md.md_Height/1024.0f;
      }
      o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Unlock();
      o3dUnwrapped.ob_aoscSectors.Unlock();
    }

    o3dClosed.ob_aoscSectors.Lock();
    o3dOpened.ob_aoscSectors.Lock();
    o3dUnwrapped.ob_aoscSectors.Lock();

    INDEX ctModelVertices = edm_md.md_VerticesCt;
    INDEX ctModelPolygons = edm_md.md_MipInfos[0].mmpi_PolygonsCt;

    o3dClosed.ob_aoscSectors[0].osc_aovxVertices.Lock();
    INDEX ctClosedVertices = o3dClosed.ob_aoscSectors[0].osc_aovxVertices.Count();
    INDEX ctClosedPolygons = o3dClosed.ob_aoscSectors[0].osc_aopoPolygons.Count();
    o3dClosed.ob_aoscSectors[0].osc_aovxVertices.Unlock();

    o3dOpened.ob_aoscSectors[0].osc_aovxVertices.Lock();
    INDEX ctOpenedVertices = o3dOpened.ob_aoscSectors[0].osc_aovxVertices.Count();
    INDEX ctOpenedPolygons = o3dOpened.ob_aoscSectors[0].osc_aopoPolygons.Count();
    o3dOpened.ob_aoscSectors[0].osc_aovxVertices.Unlock();

    o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Lock();
    INDEX ctUnwrappedVertices = o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Count();
    INDEX ctUnwrappedPolygons = o3dUnwrapped.ob_aoscSectors[0].osc_aopoPolygons.Count();
    o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Unlock();

    if((ctModelPolygons != ctClosedPolygons) ||
      (ctModelPolygons != ctOpenedPolygons) ||
      (ctModelPolygons != ctUnwrappedPolygons) )
    {
      ThrowF_t("ERROR: Object used to create model and some of objects used to define mapping don't have same number of polygons!");
    }

    if( ctModelVertices != ctClosedVertices)
    {
      ThrowF_t("ERROR: Object used to create model and object that defines closed mapping don't have same number of vertices!");
		}
      
    if( ctUnwrappedVertices != ctOpenedVertices)
    {
      ThrowF_t("ERROR: Objects that define opened and unwrapped mapping don't have same number of vertices!");
		}

    o3dClosed.ob_aoscSectors.Unlock();
    o3dOpened.ob_aoscSectors.Unlock();
    o3dUnwrapped.ob_aoscSectors.Unlock();

    CalculateUnwrappedMapping( o3dClosed, o3dOpened, o3dUnwrapped);
    CalculateMappingForMips();
  }
  else
  {
    ThrowF_t("ERROR: Mapping not defined!");    
  }

	O3D.ob_aoscSectors.Unlock();
	File.Close();

  if( edm_aasAttachedSounds.Count() == 0)
    CreateEmptyAttachingSounds();

  CObject3D::BatchLoading_t(FALSE);
  } catch (const char*) {
  CObject3D::BatchLoading_t(FALSE);
  throw;
  }
}

//----------------------------------------------------------------------------------------------
/*
 * Routine takes Object 3D class as input and creates new model (model data)
 * with its polygons, vertices, surfaces
 */
void CEditModel::NewModel(CObject3D *pO3D)
{
  pO3D->ob_aoscSectors.Lock();
  pO3D->ob_aoscSectors[0].LockAll();
  edm_md.md_VerticesCt = pO3D->ob_aoscSectors[0].osc_aovxVertices.Count();	// see how many vertices we will have
	edm_md.md_TransformedVertices.New( edm_md.md_VerticesCt); // create buffer for rotated vertices
  edm_md.md_MainMipVertices.New( edm_md.md_VerticesCt); // create buffer for main mip vertices
	edm_md.md_VertexMipMask.New( edm_md.md_VerticesCt);	// create buffer for vertex masks

  for( INDEX i=0; i<edm_md.md_VerticesCt; i++)
	{
    // copy vertex coordinates into md_MainMipVertices array so we colud make
		// mip-models later (we will search original coordinates in this array)
		edm_md.md_MainMipVertices[ i] =
      DOUBLEtoFLOAT(pO3D->ob_aoscSectors[0].osc_aovxVertices[ i]);
    edm_md.md_VertexMipMask[ i] = 0L; // mark to all vertices that they don't exist in any mip-model
	}

  AddMipModel( pO3D);								 // we add main model, first mip-model
	pO3D->ob_aoscSectors[0].UnlockAll();
  pO3D->ob_aoscSectors.Unlock();
}

//----------------------------------------------------------------------------------------------
/*
 * Routine takes 3D object as input and adds one mip model
 * The main idea is: for every vertice get distances to all vertices in md_MainMipVertices
 * array. If minimum distance is found, set that this vertice exists. Loop for all vertices.
 * Throw error if minimum distance isn't found. Set also new mip-model polygons info.
 */
void CEditModel::AddMipModel(	CObject3D *pO3D)
{
	INDEX i, j;
	BOOL same_found;

  // this is mask for vertices in current mip level
  ULONG mip_vtx_mask = (1L) << edm_md.md_MipCt;

	struct ModelMipInfo *pmmpi = &edm_md.md_MipInfos[ edm_md.md_MipCt]; // point to mip model that we will create

  // for each vertex
  for( INDEX iVertex=0; iVertex<edm_md.md_VerticesCt; iVertex++)
  {
    // mark that it is not in this mip model
    edm_md.md_VertexMipMask[ iVertex] &= ~mip_vtx_mask;
  }

	INDEX o3dvct = pO3D->ob_aoscSectors[0].osc_aovxVertices.Count();
	/*
	 * For each vertex in 3D object we calculate distances to all vertices in main mip-model.
	 * If distance (size of vector that is result of substraction of two vertice vectors) is
	 * less than some minimal float number, we assume that these vertices are the same.
	 * Processed vertex of 3D object gets its main-mip-model-vertex-friend's index as tag and
	 * mask value showing that it exists in this mip-model.
	 */
	for( i=0; i<o3dvct; i++)
	{
		same_found = FALSE;
		for( j=0; j<edm_md.md_VerticesCt; j++)
		{
			FLOAT3D vVertex = DOUBLEtoFLOAT(pO3D->ob_aoscSectors[0].osc_aovxVertices[ i]);
      FLOAT fAbsoluteDistance = Abs( (vVertex - edm_md.md_MainMipVertices[ j]).Length() );
			if( fAbsoluteDistance < MAX_ALLOWED_DISTANCE)
			{
				edm_md.md_VertexMipMask[ j] |= mip_vtx_mask;// we mark that this vertice exists in this mip model
				pO3D->ob_aoscSectors[0].osc_aovxVertices[ i].ovx_Tag = j;// remapping verice index must be remembered
				same_found = TRUE;								// mark that this vertex's remap is found
				break;
			}
		}
		if( same_found == FALSE)	// if no vertice close enough is found, we have error
		{
			ThrowF_t("Vertex from mip model %d with number %d, coordinates (%f,%f,%f), can't be found in main mip model.\n"
			  "There can't be new vertices in rougher mip-models,"
			  "but only vertices from main mip model can be removed and polygons reorganized.\n",
				edm_md.md_MipCt, i,
        pO3D->ob_aoscSectors[0].osc_aovxVertices[ i](1), pO3D->ob_aoscSectors[0].osc_aovxVertices[ i](2), pO3D->ob_aoscSectors[0].osc_aovxVertices[ i](3));
		}
	}

	/*
	 * We will create three arays for this mip polygon info:
	 *	1) array for polygons
	 *	2) array for mapping surfaces
	 *	3) array for polygon vertices
	 *	4) array for texture vertices
	 */

	/*
	 * First we create array large enough to accept object 3D's polygons.
	 */
	pmmpi->mmpi_PolygonsCt = pO3D->ob_aoscSectors[0].osc_aopoPolygons.Count();
	pmmpi->mmpi_Polygons.New( pmmpi->mmpi_PolygonsCt);

	/*
	 * Then we will create array for mapping surfaces and set their names
	 */
  pmmpi->mmpi_MappingSurfaces.New( pO3D->ob_aoscSectors[0].osc_aomtMaterials.Count());	// create array for mapping surfaces
	for( i=0; i<pO3D->ob_aoscSectors[0].osc_aomtMaterials.Count(); i++)
	{
    MappingSurface &ms = pmmpi->mmpi_MappingSurfaces[ i];
    ms.ms_ulOnColor = PC_ALLWAYS_ON;							// set default ON and OFF masking colors
		ms.ms_ulOffColor = PC_ALLWAYS_OFF;
		ms.ms_Name = CTFileName( pO3D->ob_aoscSectors[0].osc_aomtMaterials[ i].omt_Name);
    ms.ms_vSurface2DOffset = FLOAT3D( 1.0f, 1.0f, 1.0f);
    ms.ms_HPB = FLOAT3D( 0.0f, 0.0f, 0.0f);
    ms.ms_Zoom = 1.0f;

    ms.ms_colColor =
      pO3D->ob_aoscSectors[0].osc_aomtMaterials[ i].omt_Color | CT_OPAQUE; // copy surface color, set no alpha
    ms.ms_sstShadingType = SST_MATTE;
    ms.ms_sttTranslucencyType = STT_OPAQUE;
    ms.ms_ulRenderingFlags = SRF_DIFFUSE|SRF_NEW_TEXTURE_FORMAT;
	}

	/*
	 * Then we will count how many ModelPolygonVertices we need and create array for them.
	 * This number is equal to sum of all vertices used by all object 3D's polygons
	 */
	INDEX pvct = 0;
	for( i=0; i<pmmpi->mmpi_PolygonsCt; i++)
	{
		pvct += pO3D->ob_aoscSectors[0].osc_aopoPolygons[ i].opo_PolygonEdges.Count();	// we have vertices as many as edges
	}
	/*
	 * Now we will create an static array of temporary structures used for extracting
	 * vertice-surface connection. We need this because we have to set for all model vertices:
	 *	1) their texture vertices
	 *	2) their transformed vertices.
	 * First we will set surface and transformed indexes to every polygon vertice
	 */
	INDEX esvct = 0;			// for counting polygon vertices
	CStaticArray< CExtractSurfaceVertex> aesv;
	aesv.New( pvct);			// array with same number of members as polygon vertex array

  {FOREACHINDYNAMICARRAY( pO3D->ob_aoscSectors[0].osc_aopoPolygons, CObjectPolygon, it1)
	{
		INDEX iPolySurface = pO3D->ob_aoscSectors[0].osc_aomtMaterials.Index( it1->opo_Material);	// this polygon's surface index
	  FOREACHINDYNAMICARRAY( it1->opo_PolygonEdges, CObjectPolygonEdge, it2)
		{
			aesv[ esvct].esv_Surface = iPolySurface; // all these vertices are members of same polygon so they have same surface index
			aesv[ esvct].esv_MipGlobalIndex = pO3D->ob_aoscSectors[0].osc_aovxVertices.Index( it2->ope_Edge->oed_Vertex0); // global index
			esvct++;
		}
	}}


	/*
	 * Then we will choose one verice from this array and see if there is any vertice
	 * processed until now that have same surface and global index. If souch
	 * vertice exists, copy its remap value, if it doesn't exists, set its remap value
	 * to value of current texture vertex counter. After counting souch surface-dependent
	 * vertices (texture vertices, tvct) we will create array for them
	 */
	BOOL same_vtx_found;
	INDEX tvct = 0;
	for( i=0; i<pvct; i++)
	{
		same_vtx_found = FALSE;
		for( INDEX j=0; j<i; j++)
		{
			if( (aesv[ j].esv_Surface == aesv[ i].esv_Surface) &&				// if surface and global
				  (aesv[ j].esv_MipGlobalIndex == aesv[ i].esv_MipGlobalIndex)) // vertex index are the same
			{
					same_vtx_found = TRUE;									// if yes, copy remap value
					aesv[ i].esv_TextureVertexRemap = aesv[ j].esv_TextureVertexRemap;
					break;
			}
		}
		if( same_vtx_found == FALSE)									// if not, set value to current counter
		{
			aesv[ i].esv_TextureVertexRemap = tvct;
			tvct ++;
		}
	}
	pmmpi->mmpi_TextureVertices.New( tvct);						// create array for texture vertices

	/*
	 * Now we will set texture vertex data for all surface unique vertices. We will do it by
	 * looping this to all polygon vertices: copy coordinates of vertex from global vertex array
	 * to UVW coordinates of texture vertex. That way we will have little overhead (some
	 * vertices will be copied many times) but it doesn't really matter.
	 */
  for( i=0; i<pvct; i++)
	{
    pmmpi->mmpi_TextureVertices[ aesv[ i].esv_TextureVertexRemap].mtv_UVW =
		  DOUBLEtoFLOAT(pO3D->ob_aoscSectors[0].osc_aovxVertices[ aesv[ i].esv_MipGlobalIndex]);
	}


  /*
	 * Now we intend to create data for all polygons (that includes setting polygon's
	 * texture and transformed vertex ptrs)
	 */
	INDEX mpvct = 0;																// start polygon vertex counter
	for( i=0; i<pmmpi->mmpi_PolygonsCt; i++)				// loop all model polygons
	{
    struct ModelPolygon *pmp = &pmmpi->mmpi_Polygons[ i];		// ptr to activ model polygon
		pmp->mp_Surface = pO3D->ob_aoscSectors[0].osc_aomtMaterials.Index( pO3D->ob_aoscSectors[0].osc_aopoPolygons[ i].opo_Material); // copy surface index
		pmp->mp_ColorAndAlpha =
      pO3D->ob_aoscSectors[0].osc_aopoPolygons[ i].opo_Material->omt_Color | CT_OPAQUE; // copy surface color, set no alpha
		INDEX ctVertices = pO3D->ob_aoscSectors[0].osc_aopoPolygons[ i].opo_PolygonEdges.Count(); // set no of polygon's vertices
		pmp->mp_PolygonVertices.New( ctVertices); // create array for them
		for( j=0; j<ctVertices; j++)				// fill data for this polygon's vertices
		{
			/*
			 * Here we really remap one mip models's vertex in a way that we set its transformed
			 * vertex ptr after remapping it using link (tag) to its original mip-model's vertex
			 */
			ULONG trans_vtx_idx = pO3D->ob_aoscSectors[0].osc_aovxVertices[ aesv[ mpvct].esv_MipGlobalIndex].ovx_Tag;

			pmp->mp_PolygonVertices[ j].mpv_ptvTransformedVertex =
				&edm_md.md_TransformedVertices[ (INDEX) trans_vtx_idx ]; // remapped ptr to transformed vertex
			pmp->mp_PolygonVertices[ j].mpv_ptvTextureVertex =
				&pmmpi->mmpi_TextureVertices[ aesv[ mpvct].esv_TextureVertexRemap];	// ptr to unique vertex in surface
			mpvct ++;
		}
	}

	edm_md.md_MipCt ++;	// finally, this mip-model is done.
}

//----------------------------------------------------------------------------------------------
/*
 * Routine sets unwrapped mapping from given three objects
 */
void CEditModel::CalculateUnwrappedMapping( CObject3D &o3dClosed, CObject3D &o3dOpened, CObject3D &o3dUnwrapped)
{
  o3dOpened.ob_aoscSectors.Lock();
  o3dClosed.ob_aoscSectors.Lock();
  o3dUnwrapped.ob_aoscSectors.Lock();
  // get first mip model
  struct ModelMipInfo *pMMI = &edm_md.md_MipInfos[ 0];
  // for each surface in first mip model
  for( INDEX iSurface = 0; iSurface < pMMI->mmpi_MappingSurfaces.Count(); iSurface++)
  {
    MappingSurface *pmsSurface = &pMMI->mmpi_MappingSurfaces[iSurface];
    // for each texture vertex in surface
    for(INDEX iSurfaceTextureVertex=0; iSurfaceTextureVertex<pmsSurface->ms_aiTextureVertices.Count(); iSurfaceTextureVertex++)
    {
      INDEX iGlobalTextureVertex = pmsSurface->ms_aiTextureVertices[iSurfaceTextureVertex];
      ModelTextureVertex *pmtvTextureVertex = &pMMI->mmpi_TextureVertices[iGlobalTextureVertex];
      // obtain index of model vertex
      INDEX iModelVertex = pmtvTextureVertex->mtv_iTransformedVertex;
      // for each polygon in opened with same surface
      for(INDEX iOpenedPolygon=0; iOpenedPolygon< o3dOpened.ob_aoscSectors[0].osc_aopoPolygons.Count(); iOpenedPolygon++)
      {
        DOUBLE3D vClosedVertex;
        DOUBLE3D vOpenedVertex;

        // get coordinate from model vertex in closed
        o3dClosed.ob_aoscSectors[0].osc_aovxVertices.Lock();
        vClosedVertex = o3dClosed.ob_aoscSectors[0].osc_aovxVertices[ iModelVertex];
        o3dClosed.ob_aoscSectors[0].osc_aovxVertices.Unlock();

        // find vertex in opened with same coordinate
        o3dOpened.ob_aoscSectors[0].osc_aopoPolygons.Lock();
        CObjectPolygon *popoOpenedPolygon = &o3dOpened.ob_aoscSectors[0].osc_aopoPolygons[ iOpenedPolygon];
        o3dOpened.ob_aoscSectors[0].osc_aopoPolygons.Unlock();
        if( popoOpenedPolygon->opo_Material->omt_Name != pmsSurface->ms_Name) continue;
        for( INDEX iOpenedPolyEdge=0; iOpenedPolyEdge<popoOpenedPolygon->opo_PolygonEdges.Count(); iOpenedPolyEdge++)
        {
          popoOpenedPolygon->opo_PolygonEdges.Lock();
          CObjectVertex *povOpenedVertex;
          if( !popoOpenedPolygon->opo_PolygonEdges[iOpenedPolyEdge].ope_Backward)
          {
            povOpenedVertex = popoOpenedPolygon->opo_PolygonEdges[iOpenedPolyEdge].ope_Edge->oed_Vertex0;
          }
          else
          {
            povOpenedVertex = popoOpenedPolygon->opo_PolygonEdges[iOpenedPolyEdge].ope_Edge->oed_Vertex1;
          }
          popoOpenedPolygon->opo_PolygonEdges.Unlock();
          
          vOpenedVertex = *povOpenedVertex;

          // if these two vertices have same coordinates
          FLOAT fAbsoluteDistance = Abs( (vClosedVertex - vOpenedVertex).Length());
			    if( fAbsoluteDistance < MAX_ALLOWED_DISTANCE)
          {
            o3dClosed.ob_aoscSectors[0].osc_aovxVertices.Lock();
            o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Lock();
            // find index in opened
            o3dOpened.ob_aoscSectors[0].osc_aovxVertices.Lock();
            INDEX iOpenedModelVertex = o3dOpened.ob_aoscSectors[0].osc_aovxVertices.Index( povOpenedVertex);
            o3dOpened.ob_aoscSectors[0].osc_aovxVertices.Unlock();
            // get coordinate from unwrapped using index
            DOUBLE3D vMappingCoordinate = o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices[ iOpenedModelVertex];
            // set new mapping coordinates
            pmtvTextureVertex->mtv_UVW = DOUBLEtoFLOAT( vMappingCoordinate);
            pmtvTextureVertex->mtv_UVW(2) = -pmtvTextureVertex->mtv_UVW(2);
            MEX2D mexUV;
            mexUV(1) = MEX_METERS(pmtvTextureVertex->mtv_UVW(1));
            mexUV(2) = MEX_METERS(pmtvTextureVertex->mtv_UVW(2));
            pmtvTextureVertex->mtv_UV = mexUV;
            o3dClosed.ob_aoscSectors[0].osc_aovxVertices.Unlock();
            o3dUnwrapped.ob_aoscSectors[0].osc_aovxVertices.Unlock();
          }
        }
      }
      // reset surface position, rotation and zoom
      pmsSurface->ms_HPB = FLOAT3D( 0.0f, 0.0f, 0.0f);
      pmsSurface->ms_Zoom = 1.0f;
      pmsSurface->ms_vSurface2DOffset = FLOAT3D( 0.0f, 0.0f, 0.0f);
    }
  }
  o3dOpened.ob_aoscSectors.Unlock();
  o3dClosed.ob_aoscSectors.Unlock();
  o3dUnwrapped.ob_aoscSectors.Unlock();
}
//----------------------------------------------------------------------------------------------
/*
 * Routine calculate mapping for mip models (except for main mip)
 */
void CEditModel::CalculateMappingForMips( void)
{
  // for each mip model except first
  for( INDEX iCurMip = 1; iCurMip< edm_md.md_MipCt; iCurMip++)
  {
    // get current mip model
    struct ModelMipInfo *pMMICur = &edm_md.md_MipInfos[ iCurMip];
    // get previous mip model
    struct ModelMipInfo *pMMIPrev = &edm_md.md_MipInfos[ iCurMip-1];
    // for each surface in current mip model
    for( INDEX iSurfaceCur = 0; iSurfaceCur < pMMICur->mmpi_MappingSurfaces.Count(); iSurfaceCur++)
    {
      MappingSurface *pmsSurfCur = &pMMICur->mmpi_MappingSurfaces[iSurfaceCur];
      // for each texture vertex in surface
      for(INDEX iSurfCurTV=0; iSurfCurTV<pmsSurfCur->ms_aiTextureVertices.Count(); iSurfCurTV++)
      {
        INDEX iCurGlobalTV = pmsSurfCur->ms_aiTextureVertices[iSurfCurTV];
        ModelTextureVertex *pmtvCur = &pMMICur->mmpi_TextureVertices[iCurGlobalTV];
        // obtain index of model vertex
        INDEX iCurMV = pmtvCur->mtv_iTransformedVertex;
        
        // get 3D coordinate of vertex from main mip
        FLOAT3D vMainMipCoordCur = edm_md.md_MainMipVertices[ iCurMV];

        // -------- Find closest vertex (using 3D coordinate) in previous mip
 
        // in previous mip model find surface with same name
        MappingSurface *pmsSurfPrev = NULL;
        for( INDEX iSurfacePrev = 0; iSurfacePrev < pMMIPrev->mmpi_MappingSurfaces.Count(); iSurfacePrev++)
        {
          pmsSurfPrev = &pMMIPrev->mmpi_MappingSurfaces[iSurfacePrev];
          if( pmsSurfCur->ms_Name == pmsSurfPrev->ms_Name)
          {
            break;
          }
        }
          
        // new surfaces can't appear 
        ASSERT(pmsSurfPrev != NULL);
        if( pmsSurfPrev == NULL)
        {
          WarningMessage( "Mip model %d has surface that does not exist in previous mip. That is not allowed.", iCurMip);
          break;
        }

        // set hudge distance as current minimum
        FLOAT fMinDistance = 99999999.0f;
        ModelTextureVertex *pmtvClosestPrev = NULL;

        // for each texture vertex in previous mip's surface with same name
        for(INDEX iSurfPrevTV=0; iSurfPrevTV<pmsSurfPrev->ms_aiTextureVertices.Count(); iSurfPrevTV++)
        {
          INDEX iPrevGlobalTV = pmsSurfPrev->ms_aiTextureVertices[iSurfPrevTV];
          ModelTextureVertex *pmtvPrev = &pMMIPrev->mmpi_TextureVertices[iPrevGlobalTV];
          // obtain index of model vertex
          INDEX iPrevMV = pmtvPrev->mtv_iTransformedVertex;
          // get 3D coordinate of vertex from main mip
          FLOAT3D vMainMipCoordPrev = edm_md.md_MainMipVertices[ iPrevMV];
          // get distance of these two vertices
          FLOAT fAbsoluteDistance = Abs( (vMainMipCoordPrev - vMainMipCoordCur).Length());
			    if( fAbsoluteDistance < fMinDistance)
          {
            // remember current texture vertex as closest one
            fMinDistance = fAbsoluteDistance;
            pmtvClosestPrev = pmtvPrev;
          }
        }
        ASSERT( pmtvClosestPrev != NULL);
        // copy mapping coordinates from closest mapping vertex in previous mip
        pmtvCur->mtv_UVW = pmtvClosestPrev->mtv_UVW;
        pmtvCur->mtv_UV  = pmtvClosestPrev->mtv_UV;
        pmtvCur->mtv_vU  = pmtvClosestPrev->mtv_vU;
        pmtvCur->mtv_vV  = pmtvClosestPrev->mtv_vV;
      }
    }
  }
}

/*
 * This routine opens last script file loaded, repeats reading key-words until it finds
 * key-word "ANIM_START". Then it calls animation data load from script routine.
 */
void CEditModel::UpdateAnimations_t(CTFileName &fnScriptName) // throw char *
{
	CTFileStream File;
	char ld_line[ 128];
	CListHead FrameNamesList;
  FLOATmatrix3D mStretch;
  mStretch.Diagonal(1.0f);

	File.Open_t( fnScriptName); // open script file for reading

  FOREVER
	{
		do
    {
      File.GetLine_t(ld_line, 128);
    }
		while( (strlen( ld_line)== 0) || (ld_line[0]==';'));

		if( EQUAL_SUB_STR( "SIZE"))
    {
  	  _strupr( ld_line);
      FLOAT fStretch = 1.0f;
		  sscanf( ld_line, "SIZE %g", &fStretch);
      mStretch *= fStretch;
    }
		else if( EQUAL_SUB_STR( "TRANSFORM")) 
    {
  	  _strupr( ld_line);
      FLOATmatrix3D mTran;
      mTran.Diagonal(1.0f);
		  sscanf( ld_line, "TRANSFORM %g %g %g %g %g %g %g %g %g", 
        &mTran(1,1), &mTran(1,2), &mTran(1,3),
        &mTran(2,1), &mTran(2,2), &mTran(2,3),
        &mTran(3,1), &mTran(3,2), &mTran(3,3));
      mStretch *= mTran;
    }
		else if( EQUAL_SUB_STR( "ANIM_START"))
		{
			LoadModelAnimationData_t( &File, mStretch);	// load and set model's animation data
			break;													// we found our animations, we loaded them so we will stop forever loop
		}
    else if( EQUAL_SUB_STR( "ORIGIN_TRI"))
    {
      sscanf( ld_line, "ORIGIN_TRI %d %d %d", &aiTransVtx[0], &aiTransVtx[1], &aiTransVtx[2]);	// read given vertices
    }
	}
  File.Close();

  CreateEmptyAttachingSounds();
}
//----------------------------------------------------------------------------------------------
void CEditModel::CreateMipModels_t(CObject3D &objRestFrame, CObject3D &objMipSourceFrame,
         INDEX iVertexRemoveRate, INDEX iSurfacePreservingFactor)
{
  // free possible mip-models except main mip model
  INDEX iMipModel;
  for( iMipModel=1; iMipModel<edm_md.md_MipCt; iMipModel++)
	{
		edm_md.md_MipInfos[ iMipModel].Clear();
	}
	edm_md.md_MipCt = 1;

  // create mip model structure
  CMipModel mmMipModel;
  mmMipModel.FromObject3D_t( objRestFrame, objMipSourceFrame);

  if( ProgresRoutines.SetProgressMessage != NULL)
    ProgresRoutines.SetProgressMessage( "Calculating mip models ...");
  INDEX ctVerticesInRestFrame = mmMipModel.mm_amvVertices.Count();
  if( ProgresRoutines.SetProgressRange != NULL)
    ProgresRoutines.SetProgressRange(ctVerticesInRestFrame);
  // create maximum 32 mip models
  for( iMipModel=0; iMipModel<31; iMipModel++)
  {
    // if unable to create mip models
    if( !mmMipModel.CreateMipModel_t( iVertexRemoveRate, iSurfacePreservingFactor))
    {
      // stop creating more mip models
      break;
    }
    if( ProgresRoutines.SetProgressState != NULL)
      ProgresRoutines.SetProgressState(ctVerticesInRestFrame - mmMipModel.mm_amvVertices.Count());
    CObject3D objMipModel;
    mmMipModel.ToObject3D( objMipModel);

    objMipModel.ob_aoscSectors.Lock();
    objMipModel.ob_aoscSectors[0].LockAll();
    AddMipModel( &objMipModel);
    objMipModel.ob_aoscSectors[0].UnlockAll();
    objMipModel.ob_aoscSectors.Unlock();
  }
  ProgresRoutines.SetProgressState(ctVerticesInRestFrame);
  edm_md.SpreadMipSwitchFactors( 0, 5.0f);
  edm_md.LinkDataForSurfaces(FALSE);
  CalculateMappingForMips();
}
//----------------------------------------------------------------------------------------------
/*
 * This routine discards all mip-models except main (mip-model 0). Then it opens script file
 * with file name of last script loaded. Then it repeats reading key-words until it counts two
 * key-words "MIPMODEL". For second and all other key-words routine calls add mip-map routine
 */
void CEditModel::UpdateMipModels_t(CTFileName &fnScriptName) // throw char *
{
  try {
  CObject3D::BatchLoading_t(TRUE);
	CTFileStream File;
	CObject3D O3D;
	char base_path[ PATH_MAX] = "";
	char file_name[ PATH_MAX];
	char full_path[ PATH_MAX];
	char ld_line[ 128];
  FLOATmatrix3D mStretch;
  mStretch.Diagonal(1.0f);

	O3D.ob_aoscSectors.Lock();

  ASSERT( edm_md.md_VerticesCt != 0);

	File.Open_t( fnScriptName); // open script file for reading

    INDEX i;
	for( i=1; i<edm_md.md_MipCt; i++)
	{
		edm_md.md_MipInfos[ i].Clear();	// free possible mip-models except main mip model
	}
	edm_md.md_MipCt = 1;

	FOREVER
	{
		do
    {
      File.GetLine_t(ld_line, 128);
    }
		while( (strlen( ld_line)== 0) || (ld_line[0]==';'));

		if( EQUAL_SUB_STR( "SIZE"))
    {
  	  _strupr( ld_line);
      FLOAT fStretch = 1.0f;
		  sscanf( ld_line, "SIZE %g", &fStretch);
      mStretch *= fStretch;
    }
		else if( EQUAL_SUB_STR( "TRANSFORM")) 
    {
  	  _strupr( ld_line);
      FLOATmatrix3D mTran;
      mTran.Diagonal(1.0f);
		  sscanf( ld_line, "TRANSFORM %g %g %g %g %g %g %g %g %g", 
        &mTran(1,1), &mTran(1,2), &mTran(1,3),
        &mTran(2,1), &mTran(2,2), &mTran(2,3),
        &mTran(3,1), &mTran(3,2), &mTran(3,3));
      mStretch *= mTran;
    }
		else if( EQUAL_SUB_STR( "DIRECTORY"))
		{
			_strupr( ld_line);
			sscanf( ld_line, "DIRECTORY %s", base_path);
			if( base_path[ strlen( base_path) - 1] != '\\')
				strcat( base_path,"\\");
		}
		else if( EQUAL_SUB_STR( "MIP_MODELS"))
		{
			_strupr( ld_line);
			INDEX no_of_mip_models;
      sscanf( ld_line, "MIP_MODELS %d", &no_of_mip_models);

      // Jump over first mip model file name
      File.GetLine_t(ld_line, 128);

			for( i=0; i<no_of_mip_models-1; i++)
			{
        File.GetLine_t(ld_line, 128);
  			_strupr( ld_line);
				sscanf( ld_line, "%s", file_name);
				sprintf( full_path, "%s%s", base_path, file_name);

			  O3D.Clear();                            // clear possible existing O3D's data
				O3D.LoadAny3DFormat_t( CTString(full_path), mStretch);

        if( edm_md.md_VerticesCt < O3D.ob_aoscSectors[0].osc_aovxVertices.Count())
        {
          ThrowF_t(
            "It is unlikely that mip-model \"%s\" is valid.\n"
            "It contains more vertices than main mip-model so it can't be mip-model.", 
            full_path);
        }

        if( edm_md.md_MipCt < MAX_MODELMIPS)
				{
	        O3D.ob_aoscSectors[0].LockAll();;
					AddMipModel( &O3D);	// else this is one of model's mip definitions so call Add Mip Model
	        O3D.ob_aoscSectors[0].UnlockAll();;
				}
				else
				{
          ThrowF_t("There are too many mip-models defined in script file. Maximum of %d mip-models allowed.", MAX_MODELMIPS-1);
				}
			}
		}
    else if( EQUAL_SUB_STR( "ORIGIN_TRI"))
    {
      sscanf( ld_line, "ORIGIN_TRI %d %d %d", &aiTransVtx[0], &aiTransVtx[1], &aiTransVtx[2]);	// read given vertices
    }
		else if( EQUAL_SUB_STR( "END"))
		{
			break;
		}
	}
	O3D.ob_aoscSectors.Unlock();
  edm_md.LinkDataForSurfaces(TRUE);

  CObject3D::BatchLoading_t(FALSE);
  } catch (const char*) {
  CObject3D::BatchLoading_t(FALSE);
  throw;
  }
}

/*
 * Draws given surface in wire frame
 */
void CEditModel::DrawWireSurface( CDrawPort *pDP, INDEX iCurrentMip, INDEX iCurrentSurface,
                                  FLOAT fMagnifyFactor, PIX offx, PIX offy,
                                  COLOR clrVisible, COLOR clrInvisible)
{
  FLOAT3D f3dTr0, f3dTr1, f3dTr2;
  struct ModelTextureVertex *pVtx0, *pVtx1;

  // for each polygon
  for( INDEX iPoly=0; iPoly<edm_md.md_MipInfos[iCurrentMip].mmpi_PolygonsCt; iPoly++)
  {
    struct ModelPolygon *pPoly = &edm_md.md_MipInfos[iCurrentMip].mmpi_Polygons[iPoly];
    if( pPoly->mp_Surface == iCurrentSurface)
    { // readout poly vertices
      f3dTr0(1) = (FLOAT)pPoly->mp_PolygonVertices[0].mpv_ptvTextureVertex->mtv_UV(1);
      f3dTr0(2) = (FLOAT)pPoly->mp_PolygonVertices[0].mpv_ptvTextureVertex->mtv_UV(2);
      f3dTr0(3) = 0.0f;
      f3dTr1(1) = (FLOAT)pPoly->mp_PolygonVertices[1].mpv_ptvTextureVertex->mtv_UV(1);
      f3dTr1(2) = (FLOAT)pPoly->mp_PolygonVertices[1].mpv_ptvTextureVertex->mtv_UV(2);
      f3dTr1(3) = 0.0f;
      f3dTr2(1) = (FLOAT)pPoly->mp_PolygonVertices[2].mpv_ptvTextureVertex->mtv_UV(1);
      f3dTr2(2) = (FLOAT)pPoly->mp_PolygonVertices[2].mpv_ptvTextureVertex->mtv_UV(2);
      f3dTr2(3) = 0.0f;

      // determine line visibility
      FLOAT3D f3dNormal = (f3dTr2-f3dTr1)*(f3dTr0-f3dTr1);
      COLOR clrWire;
      ULONG ulLineType;
      if( f3dNormal(3) < 0) {
        clrWire = clrVisible;
        ulLineType = _FULL_;
      } else {
        clrWire = clrInvisible;
        ulLineType = _POINT_;
      }
      // draw lines
      PIX pixX0, pixY0, pixX1, pixY1;
      for( INDEX iVtx=0; iVtx<pPoly->mp_PolygonVertices.Count()-1; iVtx++) {
        pVtx0 = pPoly->mp_PolygonVertices[iVtx+0].mpv_ptvTextureVertex;
        pVtx1 = pPoly->mp_PolygonVertices[iVtx+1].mpv_ptvTextureVertex;
        pixX0 = (PIX)(pVtx0->mtv_UV(1) * fMagnifyFactor) - offx;
        pixY0 = (PIX)(pVtx0->mtv_UV(2) * fMagnifyFactor) - offy;
        pixX1 = (PIX)(pVtx1->mtv_UV(1) * fMagnifyFactor) - offx;
        pixY1 = (PIX)(pVtx1->mtv_UV(2) * fMagnifyFactor) - offy;
        pDP->DrawLine( pixX0, pixY0, pixX1, pixY1, clrWire|CT_OPAQUE, ulLineType);
      }
      // draw last line
      pVtx0 = pPoly->mp_PolygonVertices[0].mpv_ptvTextureVertex;
      pixX0 = (PIX)(pVtx0->mtv_UV(1) * fMagnifyFactor) - offx;
      pixY0 = (PIX)(pVtx0->mtv_UV(2) * fMagnifyFactor) - offy;
      pDP->DrawLine( pixX0, pixY0, pixX1, pixY1, clrWire|CT_OPAQUE, ulLineType);
    }
  }
}


/*
 * Flat fills given surface
 */
void CEditModel::DrawFilledSurface( CDrawPort *pDP, INDEX iCurrentMip, INDEX iCurrentSurface,
                                    FLOAT fMagnifyFactor, PIX offx, PIX offy,
                                    COLOR clrVisible, COLOR clrInvisible)
{
  FLOAT3D f3dTr0, f3dTr1, f3dTr2;
  struct ModelTextureVertex *pVtx0, *pVtx1, *pVtx2;

  // for each polygon
  for( INDEX iPoly=0; iPoly<edm_md.md_MipInfos[iCurrentMip].mmpi_PolygonsCt; iPoly++)
  {
    struct ModelPolygon *pPoly = &edm_md.md_MipInfos[iCurrentMip].mmpi_Polygons[iPoly];
    if( pPoly->mp_Surface == iCurrentSurface)
    { // readout poly vertices
      f3dTr0(1) = (FLOAT)pPoly->mp_PolygonVertices[0].mpv_ptvTextureVertex->mtv_UV(1);
      f3dTr0(2) = (FLOAT)pPoly->mp_PolygonVertices[0].mpv_ptvTextureVertex->mtv_UV(2);
      f3dTr0(3) = 0.0f;
      f3dTr1(1) = (FLOAT)pPoly->mp_PolygonVertices[1].mpv_ptvTextureVertex->mtv_UV(1);
      f3dTr1(2) = (FLOAT)pPoly->mp_PolygonVertices[1].mpv_ptvTextureVertex->mtv_UV(2);
      f3dTr1(3) = 0.0f;
      f3dTr2(1) = (FLOAT)pPoly->mp_PolygonVertices[2].mpv_ptvTextureVertex->mtv_UV(1);
      f3dTr2(2) = (FLOAT)pPoly->mp_PolygonVertices[2].mpv_ptvTextureVertex->mtv_UV(2);
      f3dTr2(3) = 0.0f;

      // determine poly visibility
      COLOR clrFill;
      FLOAT3D f3dNormal = (f3dTr2-f3dTr1)*(f3dTr0-f3dTr1);
      if( f3dNormal(3) < 0) clrFill = clrVisible|0xFF;
      else clrFill = clrInvisible|0xFF;

      // draw traingle(s) fan
      pDP->InitTexture( NULL);
      pVtx0 = pPoly->mp_PolygonVertices[0].mpv_ptvTextureVertex;
      PIX pixX0 = (PIX)(pVtx0->mtv_UV(1) * fMagnifyFactor) - offx;
      PIX pixY0 = (PIX)(pVtx0->mtv_UV(2) * fMagnifyFactor) - offy;
      for( INDEX iVtx=1; iVtx<pPoly->mp_PolygonVertices.Count()-1; iVtx++) {
        pVtx1 = pPoly->mp_PolygonVertices[iVtx+0].mpv_ptvTextureVertex;
        pVtx2 = pPoly->mp_PolygonVertices[iVtx+1].mpv_ptvTextureVertex;
        PIX pixX1 = (PIX)(pVtx1->mtv_UV(1) * fMagnifyFactor) - offx;
        PIX pixY1 = (PIX)(pVtx1->mtv_UV(2) * fMagnifyFactor) - offy;
        PIX pixX2 = (PIX)(pVtx2->mtv_UV(1) * fMagnifyFactor) - offx;
        PIX pixY2 = (PIX)(pVtx2->mtv_UV(2) * fMagnifyFactor) - offy;
        pDP->AddTriangle( pixX0,pixY0, pixX1,pixY1, pixX2,pixY2, clrFill);
      }
      // to buffer with it
      pDP->FlushRenderingQueue();
    }
  }
}


/*
 * Prints surface numbers
 */
void CEditModel::PrintSurfaceNumbers( CDrawPort *pDP, CFontData *pFont,
     INDEX iCurrentMip, FLOAT fMagnifyFactor, PIX offx, PIX offy, COLOR clrInk)
{
  char achrLine[ 256];

 	// clear Z-buffer
  pDP->FillZBuffer( ZBUF_BACK);

  // get mip model ptr
  struct ModelMipInfo *pMMI = &edm_md.md_MipInfos[ iCurrentMip];


  // for all surfaces
  for( INDEX iSurf=0;iSurf<pMMI->mmpi_MappingSurfaces.Count(); iSurf++)
  {
    MappingSurface *pms= &pMMI->mmpi_MappingSurfaces[iSurf];
    MEXaabbox2D boxSurface;
    // for each texture vertex in surface
    for(INDEX iSurfaceTextureVertex=0; iSurfaceTextureVertex<pms->ms_aiTextureVertices.Count(); iSurfaceTextureVertex++)
    {
      INDEX iGlobalTextureVertex = pms->ms_aiTextureVertices[iSurfaceTextureVertex];
      ModelTextureVertex *pmtv = &pMMI->mmpi_TextureVertices[iGlobalTextureVertex];
      boxSurface |= pmtv->mtv_UV;
    }
   
    MEX2D mexCenter = boxSurface.Center();
    PIX2D pixCenter = PIX2D(mexCenter(1)*fMagnifyFactor-offx, mexCenter(2)*fMagnifyFactor-offy);

    // print active surface's number into print line
    sprintf( achrLine, "%d", iSurf);

    // set font
    pDP->SetFont( pFont);
    // print line
    pDP->PutText( achrLine, pixCenter(1)-strlen(achrLine)*4, pixCenter(2)-6);
  }
}

/*
 * Exports surface names and numbers under given file name
 */
void CEditModel::ExportSurfaceNumbersAndNames( CTFileName fnFile)
{
  CTString strExport;
  // get mip model ptr
  struct ModelMipInfo *pMMI = &edm_md.md_MipInfos[ 0];

  // for all surfaces
  for( INDEX iSurf=0; iSurf<pMMI->mmpi_MappingSurfaces.Count(); iSurf++)
  {
    MappingSurface *pms= &pMMI->mmpi_MappingSurfaces[iSurf];
    CTString strExportLine;
    strExportLine.PrintF( "%d) %s\n", iSurf, (const char *) pms->ms_Name);
    strExport+=strExportLine;
  }

  try
  {
    strExport.Save_t( fnFile);
  }
  catch (const char *strError)
  {
    // report error
    WarningMessage( strError);
  }
}

/*
 * Retrieves given surface's name
 */
const char *CEditModel::GetSurfaceName(INDEX iCurrentMip, INDEX iCurrentSurface)
{
  struct MappingSurface *pSurface;
  pSurface = &edm_md.md_MipInfos[ iCurrentMip].mmpi_MappingSurfaces[ iCurrentSurface];
  return( pSurface->ms_Name);
}
//--------------------------------------------------------------------------------------------
/*
 * Sets first empty position in existing patches mask
 */
BOOL CEditModel::GetFirstEmptyPatchIndex( INDEX &iMaskBit)
{
  iMaskBit = 0;
  for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    CTextureData *pTD = (CTextureData *) edm_md.md_mpPatches[ iPatch].mp_toTexture.GetData();
    if( pTD == NULL)
    {
      iMaskBit = iPatch;
      return TRUE;
    }
  }
  return FALSE;
}
//--------------------------------------------------------------------------------------------
/*
 * Sets first occupied position in existing patches mask
 */
BOOL CEditModel::GetFirstValidPatchIndex( INDEX &iMaskBit)
{
  iMaskBit = 0;
  for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    CTextureData *pTD = (CTextureData *) edm_md.md_mpPatches[ iPatch].mp_toTexture.GetData();
    if( pTD != NULL)
    {
      iMaskBit = iPatch;
      return TRUE;
    }
  }
  return FALSE;
}
//--------------------------------------------------------------------------------------------
/*
 * Sets previous valid patch position in existing patches mask
 */
void CEditModel::GetPreviousValidPatchIndex( INDEX &iMaskBit)
{
  ASSERT( (iMaskBit>=0) && (iMaskBit<MAX_TEXTUREPATCHES) );
  for( INDEX iPatch=iMaskBit+MAX_TEXTUREPATCHES-1; iPatch>iMaskBit; iPatch--)
  {
    INDEX iCurrentPatch = iPatch%32;
    CTString strPatchName = edm_md.md_mpPatches[ iCurrentPatch].mp_strName;
    if( strPatchName != "")
    {
      iMaskBit = iCurrentPatch;
      return;
    }
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Sets next valid patch position in existing patches mask
 */
void CEditModel::GetNextValidPatchIndex( INDEX &iMaskBit)
{
  ASSERT( (iMaskBit>=0) && (iMaskBit<MAX_TEXTUREPATCHES) );
  for( INDEX iPatch=iMaskBit+1; iPatch<iMaskBit+MAX_TEXTUREPATCHES; iPatch++)
  {
    INDEX iCurrentPatch = iPatch%32;
    CTString strPatchName = edm_md.md_mpPatches[ iCurrentPatch].mp_strName;
    if( strPatchName != "")
    {
      iMaskBit = iCurrentPatch;
      return;
    }
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Moves patch relatively for given coordinates
 */
void CEditModel::MovePatchRelative( INDEX iMaskBit, MEX2D mexOffset)
{
  CTFileName fnPatch = edm_md.md_mpPatches[ iMaskBit].mp_toTexture.GetName();
  if( fnPatch == "") return;
  edm_md.md_mpPatches[ iMaskBit].mp_mexPosition += mexOffset;
  CalculatePatchesPerPolygon();
}
//--------------------------------------------------------------------------------------------
/*
 * Sets patch stretch
 */
void CEditModel::SetPatchStretch( INDEX iMaskBit, FLOAT fNewStretch)
{
  CTFileName fnPatch = edm_md.md_mpPatches[ iMaskBit].mp_toTexture.GetName();
  if( fnPatch == "") return;
  edm_md.md_mpPatches[ iMaskBit].mp_fStretch = fNewStretch;
  CalculatePatchesPerPolygon();
}
//--------------------------------------------------------------------------------------------
/*
 * Searches for first available empty patch position index and adds patch
 */
BOOL CEditModel::EditAddPatch( CTFileName fnPatchName, MEX2D mexPos, INDEX &iMaskBit)
{
  if( !GetFirstEmptyPatchIndex( iMaskBit))
    return FALSE;

  try
  {
    edm_md.md_mpPatches[ iMaskBit].mp_toTexture.SetData_t( fnPatchName);
  }
  catch (const char *strError)
  {
    (void)strError;
    return FALSE;
  }
  edm_md.md_mpPatches[ iMaskBit].mp_mexPosition = mexPos;
  edm_md.md_mpPatches[ iMaskBit].mp_fStretch = 1.0f;
  edm_md.md_mpPatches[ iMaskBit].mp_strName.PrintF( "Patch%02d", iMaskBit);
  CalculatePatchesPerPolygon();
  return TRUE;
}
//--------------------------------------------------------------------------------------------
/*
 * Removes patch with given index from existing mask and erases its file name
 */
void CEditModel::EditRemovePatch( INDEX iMaskBit)
{
  edm_md.md_mpPatches[ iMaskBit].mp_toTexture.SetData(NULL);
  CalculatePatchesPerPolygon();
}

void CEditModel::EditRemoveAllPatches(void)
{
  for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    edm_md.md_mpPatches[ iPatch].mp_toTexture.SetData(NULL);
  }
  CalculatePatchesPerPolygon();
}

INDEX CEditModel::CountPatches(void)
{
  INDEX iResult = 0;
  for(INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    if( edm_md.md_mpPatches[ iPatch].mp_toTexture.GetName() != "")
    {
      iResult++;
    }
  }
  return iResult;
}

ULONG CEditModel::GetExistingPatchesMask(void)
{
  ULONG ulResult = 0;
  for(INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    if( edm_md.md_mpPatches[ iPatch].mp_toTexture.GetName() != "")
    {
      ulResult |= 1UL << iPatch;
    }
  }
  return ulResult;
}
//--------------------------------------------------------------------------------------------
void CEditModel::CalculatePatchesPerPolygon(void)
{
  // count existing patches
  INDEX ctPatches = CountPatches();

  // for each mip model
  for( INDEX iMip=0; iMip<edm_md.md_MipCt; iMip++)
  {
    ModelMipInfo *pMMI = &edm_md.md_MipInfos[ iMip];
    // clear previously existing array
    pMMI->mmpi_aPolygonsPerPatch.Clear();
    // if patches are visible in this mip model
    if( (pMMI->mmpi_ulFlags & MM_PATCHES_VISIBLE) && (ctPatches != 0) )
    {
      // add description member for each patch
      pMMI->mmpi_aPolygonsPerPatch.New( ctPatches);
      INDEX iExistingPatch = 0;
      // for each patch
      for(INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
      {
        // if patch exists
        if( edm_md.md_mpPatches[ iPatch].mp_toTexture.GetName() != "")
        {
          // allocate temporary array of indices for each polygon in mip model
          CStaticArray<INDEX> aiPolygons;
          aiPolygons.New( pMMI->mmpi_PolygonsCt);
          // clear counter of occupied polygons
          INDEX ctOccupiedPolygons = 0;
          // get patch occupying box
          CTextureData *pTD = (CTextureData *) edm_md.md_mpPatches[ iPatch].mp_toTexture.GetData();
          ASSERT( pTD != NULL);
          MEX2D mex2dPosition = edm_md.md_mpPatches[ iPatch].mp_mexPosition;
          FLOAT fStretch = edm_md.md_mpPatches[ iPatch].mp_fStretch;
          MEXaabbox2D boxPatch = MEXaabbox2D(
                  mex2dPosition, MEX2D( mex2dPosition(1)+pTD->GetWidth()*fStretch,
                                 mex2dPosition(2)+pTD->GetHeight()*fStretch) );
          // for each polygon
          for(INDEX iPolygon=0; iPolygon<pMMI->mmpi_PolygonsCt; iPolygon++)
          {
            ModelPolygon *pMP = &pMMI->mmpi_Polygons[iPolygon];
            // for all vertices in polygon
            MEXaabbox2D boxMapping;
            for( INDEX iVertex=0; iVertex<pMP->mp_PolygonVertices.Count(); iVertex++)
            {
              ModelTextureVertex *pMTV = pMP->mp_PolygonVertices[iVertex].mpv_ptvTextureVertex;
              // calculate bounding box of mapping coordinates
              boxMapping |= MEXaabbox2D(pMTV->mtv_UV);
            }
            // if bounding box of polygon's mapping coordinates touches patch
            if( boxPatch.HasContactWith( boxMapping))
            {
              // add polygon index to list of occupied polygons
              aiPolygons[ ctOccupiedPolygons] = iPolygon;
              ctOccupiedPolygons++;
            }
          }
          if( ctOccupiedPolygons != 0)
          {
            // copy temporary array of polygon indices to mip model's array of polygon indices
            pMMI->mmpi_aPolygonsPerPatch[ iExistingPatch].ppp_iPolygons.New( ctOccupiedPolygons);
            for( INDEX iOccupied=0; iOccupied<ctOccupiedPolygons; iOccupied++)
            {
              pMMI->mmpi_aPolygonsPerPatch[ iExistingPatch].ppp_iPolygons[iOccupied] =
                aiPolygons[ iOccupied];
            }
          }
          // count existing patches
          iExistingPatch++;
        }
      }
    }
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Writes settings of given mip model into file
 */
void CEditModel::WriteMipSettings_t( CTStream *ostrFile, INDEX iMip)
{
  ASSERT( iMip < edm_md.md_MipCt);

  // write indetification of one mip's mapping info
  ostrFile->WriteID_t( CChunkID( "MIPS"));

  // get count
  INDEX iSurfacesCt = edm_md.md_MipInfos[ iMip].mmpi_MappingSurfaces.Count();
  // write count
  (*ostrFile) << iSurfacesCt;
  // for all surfaces
  FOREACHINSTATICARRAY(edm_md.md_MipInfos[ iMip].mmpi_MappingSurfaces, MappingSurface, itSurface)
  {
    // write setings for current surface
    itSurface->WriteSettings_t( ostrFile);
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Reads settigns of given mip model from file
 */
void CEditModel::ReadMipSettings_t(CTStream *istrFile, INDEX iMip)
{
  MappingSurface msTmp;

  ASSERT( iMip < edm_md.md_MipCt);

  // check chunk
  istrFile->ExpectID_t( CChunkID( "MIPS"));
  // get count
  INDEX iSurfacesCt;
  *istrFile >> iSurfacesCt;

  // for all saved surfaces
  for( INDEX iSurface=0; iSurface<iSurfacesCt; iSurface++)
  {
    // read mapping surface settings
    msTmp.ReadSettings_t( istrFile);

    // for all surfaces in given mip
    for( INDEX i=0; i<edm_md.md_MipInfos[ iMip].mmpi_MappingSurfaces.Count(); i++)
    {
      MappingSurface &ms = edm_md.md_MipInfos[ iMip].mmpi_MappingSurfaces[ i];
      // are these surfaces the same?
      if( ms == msTmp)
      {
        // try to set new position and angles
        ms.ms_sstShadingType = msTmp.ms_sstShadingType;
        ms.ms_sttTranslucencyType = msTmp.ms_sttTranslucencyType;
        ms.ms_ulRenderingFlags = msTmp.ms_ulRenderingFlags;
        ms.ms_colDiffuse = msTmp.ms_colDiffuse;
        ms.ms_colReflections = msTmp.ms_colReflections;
        ms.ms_colSpecular = msTmp.ms_colSpecular;
        ms.ms_colBump = msTmp.ms_colBump;
        ms.ms_ulOnColor = msTmp.ms_ulOnColor;
        ms.ms_ulOffColor = msTmp.ms_ulOffColor;
      }
    }
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Saves mapping data for whole model (iMip = -1) or for just one mip model
 */
void CEditModel::SaveMapping_t( CTFileName fnFileName, INDEX iMip /*=-1*/)
{
  CTFileStream strmMappingFile;

  // create file
  strmMappingFile.Create_t( fnFileName, CTStream::CM_BINARY);
  // write file ID
  strmMappingFile.WriteID_t( CChunkID( "MPNG"));
  // write version
  strmMappingFile.WriteID_t( CChunkID(MAPPING_VERSION));

  // set as we have only one mip
  INDEX iStartCt = iMip;
  INDEX iMaxCt = iMip+1;

  // if iMip is -1 means that we want all mips in model
  if( iMip == -1)
  {
    iStartCt = 0;
    iMaxCt = edm_md.md_MipCt;
  }

  // for wanted mip models
  for( INDEX iMipCt=iStartCt; iMipCt<iMaxCt; iMipCt++)
  {
    // write settings for current mip
    WriteMipSettings_t( &strmMappingFile, iMipCt);
  }

  // save attached sounds
  strmMappingFile<<edm_aasAttachedSounds.Count();
  for( INDEX iSound=0; iSound<edm_aasAttachedSounds.Count(); iSound++)
  {
    edm_aasAttachedSounds[iSound].Write_t( &strmMappingFile);
  }

  // save attached models
  INDEX ctAttachmentPositions = edm_aamAttachedModels.Count();
  ASSERT( edm_md.md_aampAttachedPosition.Count() == ctAttachmentPositions);
  strmMappingFile<<ctAttachmentPositions;
  FOREACHINDYNAMICARRAY(edm_aamAttachedModels, CAttachedModel, itam)
  {
    itam->Write_t( &strmMappingFile);
  }
  FOREACHINDYNAMICARRAY(edm_md.md_aampAttachedPosition, CAttachedModelPosition, itamp)
  {
    itamp->Write_t( &strmMappingFile);
  }

  // save collision boxes
  INDEX ctCollisionBoxes = edm_md.md_acbCollisionBox.Count();
  ASSERT( ctCollisionBoxes>0);
  if(ctCollisionBoxes == 0)
  {
    WarningMessage( "Trying to save 0 collision boxes into mapping file.");
  }
  strmMappingFile<<ctCollisionBoxes;
  FOREACHINDYNAMICARRAY(edm_md.md_acbCollisionBox, CModelCollisionBox, itcb)
  {
    itcb->Write_t( &strmMappingFile);
  }

  // save patches
  for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
  {
    edm_md.md_mpPatches[ iPatch].Write_t( &strmMappingFile);
  }
}
//--------------------------------------------------------------------------------------------
/*
 * Loads mapping data for whole model (iMip = -1) or just for one mip model
 */
void CEditModel::LoadMapping_t( CTFileName fnFileName, INDEX iMip /*=-1*/)
{
  CTFileStream strmMappingFile;

  BOOL bReadPolygonsPerSurface = FALSE;
  BOOL bReadSoundsAndAttachments = FALSE;
  BOOL bReadCollision = FALSE;
  BOOL bReadPatches = FALSE;
  BOOL bReadSurfaceColors = FALSE;
  // open binary file
  strmMappingFile.Open_t( fnFileName);
  // recognize file ID
  strmMappingFile.ExpectID_t( CChunkID( "MPNG"));
  // get version of mapping file
  CChunkID cidVersion = strmMappingFile.GetID_t();
  // act acording to version of mapping file
  if( cidVersion == CChunkID(MAPPING_VERSION_WITHOUT_POLYGONS_PER_SURFACE) )
  {
  }
  else if( cidVersion == CChunkID( MAPPING_VERSION_WITHOUT_SOUNDS_AND_ATTACHMENTS))
  {
    bReadPolygonsPerSurface = TRUE;
  }
  else if( cidVersion == CChunkID( MAPPING_VERSION_WITHOUT_COLLISION))
  {
    bReadPolygonsPerSurface = TRUE;
    bReadSoundsAndAttachments = TRUE;
  }
  else if( cidVersion == CChunkID( MAPPING_VERSION_WITHOUT_PATCHES))
  {
    bReadPolygonsPerSurface = TRUE;
    bReadSoundsAndAttachments = TRUE;
    bReadCollision = TRUE;
  }
  else if( cidVersion == CChunkID( MAPPING_WITHOUT_SURFACE_COLORS))
  {
    bReadPolygonsPerSurface = TRUE;
    bReadSoundsAndAttachments = TRUE;
    bReadCollision = TRUE;
    bReadPatches = TRUE;
  }
  else if( cidVersion == CChunkID( MAPPING_VERSION))
  {
    bReadPolygonsPerSurface = TRUE;
    bReadSoundsAndAttachments = TRUE;
    bReadCollision = TRUE;
    bReadPatches = TRUE;
    bReadSurfaceColors = TRUE;
  }
  else
  {
    throw( "Invalid version of mapping file.");
  }
  // set as we have only one mip
  INDEX iStartCt = iMip;
  INDEX iMaxCt = iMip+1;

  // if iMip is -1 means that we want all mips in model
  if( iMip == -1)
  {
    iStartCt = 0;
    iMaxCt = edm_md.md_MipCt;
  }

  // for wanted mip models
  for( INDEX iMipCt=iStartCt; iMipCt<iMaxCt; iMipCt++)
  {
    if( strmMappingFile.PeekID_t()==CChunkID("MIPS"))
    {
      // read mapping for current mip
      ReadMipSettings_t( &strmMappingFile, iMipCt);
    }
  }

  // skip data for mip models that were saved but haven't been
  // readed in previous loop
  while( strmMappingFile.PeekID_t()==CChunkID("MIPS"))
  {
    MappingSurface msDummy;
    strmMappingFile.ExpectID_t( CChunkID( "MIPS"));
    // for all saved surfaces
    INDEX iSurfacesCt;
    strmMappingFile >> iSurfacesCt;
    for( INDEX iSurface=0; iSurface<iSurfacesCt; iSurface++)
    {
      // skip mapping surface
      msDummy.ReadSettings_t( &strmMappingFile);
    }
  }

  if( bReadSoundsAndAttachments)
  {
    // load attached sounds
    INDEX ctSounds;
    strmMappingFile>>ctSounds;
    ASSERT(ctSounds > 0);
    edm_aasAttachedSounds.Clear();
    edm_aasAttachedSounds.New( ctSounds);
    for( INDEX iSound=0; iSound<edm_aasAttachedSounds.Count(); iSound++)
    {
      edm_aasAttachedSounds[iSound].Read_t( &strmMappingFile);
    }
    // if number of animations does not match number of sounds saved in map file, reset sounds
    if(ctSounds != edm_md.GetAnimsCt())
    {
      edm_aasAttachedSounds.Clear();
      edm_aasAttachedSounds.New( edm_md.GetAnimsCt());
    }

    // load attached models
    INDEX ctAttachmentPositions;
    strmMappingFile>>ctAttachmentPositions;
    edm_aamAttachedModels.Clear();
    edm_md.md_aampAttachedPosition.Clear();
    if( ctAttachmentPositions != 0)
    {
      edm_aamAttachedModels.New(ctAttachmentPositions);
      edm_md.md_aampAttachedPosition.New(ctAttachmentPositions);
      FOREACHINDYNAMICARRAY(edm_aamAttachedModels, CAttachedModel, itam)
      {
        try
        {
          itam->Read_t( &strmMappingFile);
        }
        catch (const char *strError)
        {
          (void) strError;
          edm_aamAttachedModels.Clear();
          edm_md.md_aampAttachedPosition.Clear();
          ThrowF_t( "Error ocured while reading attahment model, maybe model does"
                    " not exist.");
        }
      }
      FOREACHINDYNAMICARRAY(edm_md.md_aampAttachedPosition, CAttachedModelPosition, itamp)
      {
        itamp->Read_t( &strmMappingFile);
      }
    }
  }

  if( bReadCollision)
  {
    // read collision boxes
    edm_md.md_acbCollisionBox.Clear();
    INDEX ctCollisionBoxes;
    strmMappingFile>>ctCollisionBoxes;
    ASSERT(ctCollisionBoxes>0);
    if( ctCollisionBoxes>0)
    {
      edm_md.md_acbCollisionBox.New( ctCollisionBoxes);
      FOREACHINDYNAMICARRAY(edm_md.md_acbCollisionBox, CModelCollisionBox, itcb)
      {
        itcb->Read_t( &strmMappingFile);
        itcb->ReadName_t( &strmMappingFile);
      }
    }
    else
    {
      edm_md.md_acbCollisionBox.New( 1);
      throw( "Trying to load 0 collision boxes from mapping file.");
    }
  }

  if( bReadPatches)
  {
    EditRemoveAllPatches();
    for( INDEX iPatch=0; iPatch<MAX_TEXTUREPATCHES; iPatch++)
    {
      edm_md.md_mpPatches[ iPatch].Read_t( &strmMappingFile);
    }
    CalculatePatchesPerPolygon();
  }
}

void CEditModel::AddCollisionBox(void)
{
  // add one collision box
  edm_md.md_acbCollisionBox.New();
  // select newly added collision box
  edm_iActiveCollisionBox = edm_md.md_acbCollisionBox.Count()-1;
}

void CEditModel::DeleteCurrentCollisionBox(void)
{
  INDEX ctCollisionBoxes = edm_md.md_acbCollisionBox.Count();
  // if we have more than 1 collision box
  if( ctCollisionBoxes != 1)
  {
    edm_md.md_acbCollisionBox.Lock();
    edm_md.md_acbCollisionBox.Delete( &edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox]);
    edm_md.md_acbCollisionBox.Unlock();
    // if this was last collision box
    if( edm_iActiveCollisionBox == (ctCollisionBoxes-1) )
    {
      // select last collision box
      edm_iActiveCollisionBox = ctCollisionBoxes-2;
    }
  }
}

void CEditModel::ActivatePreviousCollisionBox(void)
{
  // get count of collision boxes
  INDEX ctCollisionBoxes = edm_md.md_acbCollisionBox.Count();
  if( edm_iActiveCollisionBox != 0)
  {
    edm_iActiveCollisionBox -= 1;
  }
}

void CEditModel::ActivateNextCollisionBox(void)
{
  // get count of collision boxes
  INDEX ctCollisionBoxes = edm_md.md_acbCollisionBox.Count();
  if( edm_iActiveCollisionBox != (ctCollisionBoxes-1) )
  {
    edm_iActiveCollisionBox += 1;
  }
}

void CEditModel::SetCollisionBox(FLOAT3D vMin, FLOAT3D vMax)
{
  edm_md.md_acbCollisionBox.Lock();
  edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_vCollisionBoxMin = vMin;
  edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_vCollisionBoxMax = vMax;
  edm_md.md_acbCollisionBox.Unlock();
  CorrectCollisionBoxSize();
}

CTString CEditModel::GetCollisionBoxName(INDEX iCollisionBox)
{
  // get count of collision boxes
  INDEX ctCollisionBoxes = edm_md.md_acbCollisionBox.Count();
  ASSERT( iCollisionBox < ctCollisionBoxes);
  if( iCollisionBox >= ctCollisionBoxes)
  {
    iCollisionBox = ctCollisionBoxes-1;
  }
  CTString strCollisionBoxName;
  edm_md.md_acbCollisionBox.Lock();
  strCollisionBoxName = edm_md.md_acbCollisionBox[ iCollisionBox].mcb_strName;
  edm_md.md_acbCollisionBox.Unlock();
  return strCollisionBoxName;
}

CTString CEditModel::GetCollisionBoxName(void)
{
  CTString strCollisionBoxName;
  edm_md.md_acbCollisionBox.Lock();
  strCollisionBoxName = edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_strName;
  edm_md.md_acbCollisionBox.Unlock();
  return strCollisionBoxName;
}

void CEditModel::SetCollisionBoxName(CTString strNewName)
{
  edm_md.md_acbCollisionBox.Lock();
  edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_strName = strNewName;
  edm_md.md_acbCollisionBox.Unlock();
}

void CEditModel::CorrectCollisionBoxSize(void)
{
  // no correction needed if colliding as cube
  if( edm_md.md_bCollideAsCube) return;
  edm_md.md_acbCollisionBox.Lock();
  // get equality radio initial value
  INDEX iEqualityType = GetCollisionBoxDimensionEquality();
  // get min and max vectors of currently active collision box
  FLOAT3D vMin = edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_vCollisionBoxMin;
  FLOAT3D vMax = edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_vCollisionBoxMax;
  FLOAT3D vOldCenter;

  vOldCenter(1) = (vMax(1)+vMin(1))/2.0f;
  vOldCenter(3) = (vMax(3)+vMin(3))/2.0f;

  // calculate vector of collision box diagonale
  FLOAT3D vCorrectedDiagonale = vMax-vMin;
  // apply minimal collision box conditions
  if( vCorrectedDiagonale(1) < 0.1f) vCorrectedDiagonale(1) = 0.01f;
  if( vCorrectedDiagonale(2) < 0.1f) vCorrectedDiagonale(2) = 0.01f;
  if( vCorrectedDiagonale(3) < 0.1f) vCorrectedDiagonale(3) = 0.01f;
  // according to equality type flag (which dimensions are same)
  switch( iEqualityType)
  {
    case HEIGHT_EQ_WIDTH:
    {
      // don't allow that unlocked dimension is smaller than locked ones
      if( vCorrectedDiagonale(3) < vCorrectedDiagonale(1) )
      {
        vCorrectedDiagonale(3) = vCorrectedDiagonale(1);
      }
      // height = width
      vCorrectedDiagonale(2) = vCorrectedDiagonale(1);
      break;
    }
    case LENGTH_EQ_WIDTH:
    {
      // don't allow that unlocked dimension is smaller than locked ones
      if( vCorrectedDiagonale(2) < vCorrectedDiagonale(1) )
      {
        vCorrectedDiagonale(2) = vCorrectedDiagonale(1);
      }
      // length = width
      vCorrectedDiagonale(3) = vCorrectedDiagonale(1);
      break;
    }
    case LENGTH_EQ_HEIGHT:
    {
      // don't allow that unlocked dimension is smaller than locked ones
      if( vCorrectedDiagonale(1) < vCorrectedDiagonale(2) )
      {
        vCorrectedDiagonale(1) = vCorrectedDiagonale(2);
      }
      // length = height
      vCorrectedDiagonale(3) = vCorrectedDiagonale(2);
      break;
    }
    default:
    {
      ASSERTALWAYS( "Invalid collision box dimension equality value found.");
    }
  }
  // set new, corrected max vector
  FLOAT3D vNewMin, vNewMax;
  vNewMin(1) = vOldCenter(1)-vCorrectedDiagonale(1)/2.0f;
  vNewMin(2) = vMin(2);
  vNewMin(3) = vOldCenter(3)-vCorrectedDiagonale(3)/2.0f;

  vNewMax(1) = vOldCenter(1)+vCorrectedDiagonale(1)/2.0f;
  vNewMax(2) = vMin(2)+vCorrectedDiagonale(2);
  vNewMax(3) = vOldCenter(3)+vCorrectedDiagonale(3)/2.0f;

  edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_vCollisionBoxMin = vNewMin;
  edm_md.md_acbCollisionBox[ edm_iActiveCollisionBox].mcb_vCollisionBoxMax = vNewMax;
  edm_md.md_acbCollisionBox.Unlock();
}
//---------------------------------------------------------------------------------------------
// collision box handling functions
FLOAT3D &CEditModel::GetCollisionBoxMin(void)
{
  edm_md.md_acbCollisionBox.Lock();
  FLOAT3D &vMin = edm_md.md_acbCollisionBox[edm_iActiveCollisionBox].mcb_vCollisionBoxMin;
  edm_md.md_acbCollisionBox.Unlock();
  return vMin;
};
FLOAT3D &CEditModel::GetCollisionBoxMax(void)
{
  edm_md.md_acbCollisionBox.Lock();
  FLOAT3D &vMax = edm_md.md_acbCollisionBox[edm_iActiveCollisionBox].mcb_vCollisionBoxMax;
  edm_md.md_acbCollisionBox.Unlock();
  return vMax;
};

// returns HEIGHT_EQ_WIDTH, LENGTH_EQ_WIDTH or LENGTH_EQ_HEIGHT
INDEX CEditModel::GetCollisionBoxDimensionEquality()
{
  return edm_md.GetCollisionBoxDimensionEquality(edm_iActiveCollisionBox);
};
// set new collision box equality value
void CEditModel::SetCollisionBoxDimensionEquality( INDEX iNewDimEqType)
{
  edm_md.md_acbCollisionBox.Lock();
  edm_md.md_acbCollisionBox[edm_iActiveCollisionBox].mcb_iCollisionBoxDimensionEquality =
    iNewDimEqType;
  edm_md.md_acbCollisionBox.Unlock();
  CorrectCollisionBoxSize();
};





#if 0
        // only triangles are supported!
        ASSERT( opo.opo_PolygonEdges.Count() == 3);  
        if( opo.opo_PolygonEdges.Count() != 3) {
  			  ThrowF_t( "Non-triangle polygon encountered in model file %s !", (CTString)itFr->cfnn_FileName);
        }

        CObjectPolygonEdge &ope0 = opo.opo_PolygonEdges[0];
        CObjectPolygonEdge &ope1 = opo.opo_PolygonEdges[1];
        if( ope0.ope_Backward) {
          povx0 = ope0.ope_Edge->oed_Vertex1;
          povx1 = ope0.ope_Edge->oed_Vertex0;
          povx2 = ope1.ope_Edge->oed_Vertex0;
          ASSERT( ope1.ope_Edge->oed_Vertex1 == povx1);
        } else {
          povx0 = ope0.ope_Edge->oed_Vertex0;
          povx1 = ope0.ope_Edge->oed_Vertex1;
          povx2 = ope1.ope_Edge->oed_Vertex1;
          ASSERT( ope1.ope_Edge->oed_Vertex0 == povx1);
        }

        if( povx1==&ovxThis || povx0==&ovxThis || povx2==&ovxThis) {
          DOUBLE3D v0 = (*povx0)-(*povx1);
          DOUBLE3D v1 = (*povx2)-(*povx1);
          v0.Normalize();
          v1.Normalize();
          ANGLE a = ASin( (v0*v1).Length());
          ASSERT( a>=0 && a<=180);
          aSum += a;
          vSum += DOUBLEtoFLOAT( (DOUBLE3D&)*(itPoly->opo_Plane)) * a;
          break;
        }
#endif
