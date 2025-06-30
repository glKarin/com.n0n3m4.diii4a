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

#include <Engine/Base/Anim.h>

#include <Engine/Base/Memory.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Timer.h>
#include <Engine/Math/Functions.h>

#include <Engine/Templates/Stock_CAnimData.h>

#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/DynamicArray.cpp>

/*
 * One animation of an animateable object
 */
class COneAnim
{
public:
  COneAnim();
  ~COneAnim();
  // copy constructor
	COneAnim &operator=(const COneAnim &oaAnim);
  NAME oa_Name;
  TIME oa_SecsPerFrame;	    // speed of this animation
  INDEX oa_NumberOfFrames;
  INDEX *oa_FrameIndices;   // array of frame indices
};

/*
 * Node used for linking ptrs to COneAnim objects while loading
 * script file before turning them into an array
 * Class is used only for loading script files
 */
class COneAnimNode
{
public:
	~COneAnimNode();
  COneAnimNode(COneAnim *AnimToInsert, CListHead *LH);
	CListNode coan_Node;
	COneAnim *coan_OneAnim;
};
COneAnimNode::~COneAnimNode()
{
  ASSERT( coan_OneAnim != NULL);
  delete coan_OneAnim;
}

/*
 * This temporary list head class is used for automatic deleting of temporary list on exit
 */
class CTmpListHead : public CListHead
{
public:
	~CTmpListHead();
};

CTmpListHead::~CTmpListHead()
{
	FORDELETELIST(COneAnimNode, coan_Node, *this, it)
		delete &it.Current();
}

// Remember ptr to animation and add this node at the end of given animation list
COneAnimNode::COneAnimNode(COneAnim *AnimToInsert, CListHead *LH)
{
	coan_OneAnim = AnimToInsert;
	LH->AddTail( coan_Node);
}

// Constructor sets invalid data
COneAnim::COneAnim()
{
	oa_FrameIndices = NULL;
}

// Free allocated frame indices array for this animation
COneAnim::~COneAnim()
{
	ASSERT(oa_FrameIndices != NULL);
	FreeMemory( oa_FrameIndices);
  oa_FrameIndices = NULL;
}

/*
 * Copy constructor.
 */
COneAnim &COneAnim::operator=(const COneAnim &oaAnim)
{
  ASSERT( oaAnim.oa_NumberOfFrames > 0);
  strcpy(oa_Name, oaAnim.oa_Name);
  oa_SecsPerFrame = oaAnim.oa_SecsPerFrame;
  oa_NumberOfFrames = oaAnim.oa_NumberOfFrames;
  if( oa_FrameIndices != NULL)
  {
    FreeMemory(oa_FrameIndices);
  }
  oa_FrameIndices = (INDEX *) AllocMemory( sizeof(INDEX) * oa_NumberOfFrames);
  for( INDEX iFrame = 0; iFrame<oa_NumberOfFrames; iFrame++)
  {
    oa_FrameIndices[ iFrame] = oaAnim.oa_FrameIndices[ iFrame];
  }
  return *this;
}


// Remember given file name and add this node into string list
CFileNameNode::CFileNameNode(const char *NewFileName, CListHead *LH)
{
	ASSERT(NewFileName != NULL);
	ASSERT(strlen(NewFileName)>0);
	strcpy( cfnn_FileName, NewFileName);
	LH->AddTail( cfnn_Node);
}

CAnimData::CAnimData()
{
	ad_Anims = NULL;
  ad_NumberOfAnims = 0;
}

CAnimData::~CAnimData()
{
  Clear();
}

void CAnimData::Clear()
{
	if(ad_Anims != NULL)
	  delete[] ad_Anims;
	ad_Anims = NULL;
  ad_NumberOfAnims = 0;

  // clear serial object
  CSerial::Clear();
}

// get amount of memory used by this object
SLONG CAnimData::GetUsedMemory(void)
{
  SLONG slUsed = sizeof(*this)+sizeof(COneAnim)*ad_NumberOfAnims;
  slUsed += strlen(GetName())+1;

  for(INDEX iAnim=0; iAnim<ad_NumberOfAnims; iAnim++) {
    slUsed += ad_Anims[iAnim].oa_NumberOfFrames*sizeof(INDEX);
  }
  return slUsed;
}

// check if this kind of objects is auto-freed
BOOL CAnimData::IsAutoFreed(void)
{
  return FALSE;
}

/////////////////////////////////////////////////////////////////////
// Reference counting functions
void CAnimData::AddReference(void)
{
  ASSERT(this!=NULL);
  MarkUsed();
}

void CAnimData::RemReference(void)
{
  ASSERT(this!=NULL);
  RemReference_internal();
}

void CAnimData::RemReference_internal(void)
{
  _pAnimStock->Release(this);
}

// creates given number of default animations (1 frame, given name and speed)
void CAnimData::CreateAnimations( INDEX ctAnimations, CTString strName/*="None"*/,
                                  INDEX iDefaultFrame/*=0*/, TIME tmSpeed/*=0.02f*/)
{
  ASSERT(strlen(strName)<NAME_SIZE);
  // clear existing animations
  Clear();
  // set new number of anims
  ad_NumberOfAnims = ctAnimations;
  // create requested number of animations
  ad_Anims = new COneAnim[ctAnimations];
  // set default data for aeach new animation
  for( INDEX iAnimations=0; iAnimations<ctAnimations; iAnimations++)
  {
    // set default (or given) name
    strcpy(ad_Anims[ iAnimations].oa_Name, strName);
    // set default (or given) speed
    ad_Anims[ iAnimations].oa_SecsPerFrame = tmSpeed;
    // create one frame for this animation
    ad_Anims[ iAnimations].oa_NumberOfFrames = 1;
    ad_Anims[ iAnimations].oa_FrameIndices = (INDEX *) AllocMemory( sizeof(INDEX));
    ad_Anims[ iAnimations].oa_FrameIndices[0] = iDefaultFrame;
  }
}

// replaces frames array with given one
void CAnimData::SetFrames( INDEX iAnimation, INDEX ctFrames, INDEX *pNewFrames)
{
  ASSERT(iAnimation<ad_NumberOfAnims);
  // clear existing animation
  if(ad_Anims[iAnimation].oa_FrameIndices != NULL)
  {
    FreeMemory( ad_Anims[iAnimation].oa_FrameIndices);
  }

	// create array for new frames
  ad_Anims[iAnimation].oa_FrameIndices = (INDEX *) AllocMemory( sizeof(INDEX)*ctFrames);
  // copy given array of frames
  for( INDEX iFrames=0; iFrames<ctFrames; iFrames++)
  {
    ad_Anims[iAnimation].oa_FrameIndices[ iFrames]=pNewFrames[iFrames];
  }
  // set new number of frames
  ad_Anims[iAnimation].oa_NumberOfFrames = ctFrames;
}

// Fill animation data to contain one animation named "OnlyAnim" with one frame
void CAnimData::DefaultAnimation()
{
	ASSERT( ad_Anims == NULL);
  ad_NumberOfAnims = 1;
	ad_Anims = new COneAnim[1];
	strcpy( ad_Anims->oa_Name,"OnlyAnim");
	ad_Anims->oa_SecsPerFrame = (TIME) 0.02;
	ad_Anims->oa_NumberOfFrames = 1;
	ad_Anims->oa_FrameIndices = (INDEX *) AllocMemory( sizeof(INDEX));
	ad_Anims->oa_FrameIndices[0] = 0;
}

// Returns index of given frame name in global frame names list. If it is not found
// new CFileNameObject is added into frames list
INDEX FindFrameIndex( CListHead *pFrameFileList, const char *pFileName)
{
	UWORD i=0;

	FOREACHINLIST(CFileNameNode, cfnn_Node, *pFrameFileList, it) {
		if( strcmpi(it->cfnn_FileName, pFileName) == 0)
			return( i);
		i++;
	}
	new CFileNameNode( pFileName, pFrameFileList);
	return( i);
}

CTString GetFrameFileName( CListHead *pFrameFileList, INDEX iMemberInList)
{
	ASSERT( iMemberInList<pFrameFileList->Count());
  UWORD iMember=0;
	FOREACHINLIST(CFileNameNode, cfnn_Node, *pFrameFileList, it)
  {
		if( iMember == iMemberInList) return CTString( it->cfnn_FileName);
		iMember++;
	}
	ASSERTALWAYS( "Frame with given index is not found in list of frames");
  return "";
}

// If found given word at the beginning of curently loaded line
#define EQUAL_SUB_STR( str) (strnicmp( ld_line, str, strlen(str)) == 0)

// Loads part of given script file until word AnimEnd is reached
// Fills ACanimData (its instance) with apropriate data (animations and their frame indices)
// and fills given list head with string nodes containing file names representing frames
// needed to be loaded by a parent object
void CAnimData::LoadFromScript_t( CTStream *File, CListHead *pFrameFileList) // throw char *
{
	UWORD i;
	char error_str[ 256];
  char key_word[ 256];
	char base_path[ PATH_MAX] = "";
	char file_name[ PATH_MAX];
	char anim_name[ 256];
	char full_path[ PATH_MAX];
	char ld_line[ 128];
	CTmpListHead TempAnimationList;
	SLONG lc;
	//BOOL ret_val;

	//ASSERT( ad_Anims == NULL);
  // clears possible animations
  CAnimData::Clear();

	//ret_val = TRUE;
	FOREVER
	{
		// Repeat reading of one line of script file until it is not empty or comment
		do
    {
      File->GetLine_t(ld_line, 128);
    }
		while( (strlen( ld_line)== 0) || (ld_line[0]==';'));

		// If key-word is "/*", search end of comment block
		if( EQUAL_SUB_STR( "/*"))
    {
		  do
      {
        File->GetLine_t(ld_line, 128);
      }
		  while( !EQUAL_SUB_STR( "*/"));
    }
		// If key-word is "DIRECTORY", remember it and add "\" character at the end of new path
		// if it is not yet there
		else if( EQUAL_SUB_STR( "DIRECTORY"))
		{
			// !!! FIXME : rcg10092001 You can't uppercase filenames!
			//_strupr( ld_line);
			sscanf( ld_line, "DIRECTORY %s", base_path);

			// !!! FIXME : rcg10092001 Use CFileSystem::GetDirSeparator().
			if( base_path[ strlen( base_path) - 1] != '\\')
				strcat( base_path,"\\");
		}
		// Key-word animation must follow its name (in same line),
		// its speed and its number of frames (new lines)
		else if( EQUAL_SUB_STR( "ANIMATION"))
		{
	    if( strlen( ld_line) <= (strlen( "ANIMATION") + 1))
      {
        throw("You have to give descriptive name to every animation.");
      }
			// Create new animation
			COneAnim *poaOneAnim = new COneAnim;
			_strupr( ld_line);
      sscanf( ld_line, "ANIMATION %s", poaOneAnim->oa_Name);
			File->GetLine_t(ld_line, 128);
			if( !EQUAL_SUB_STR( "SPEED"))
      {
				throw("Expecting key word \"SPEED\" after key word \"ANIMATION\".");
      }
			_strupr( ld_line);
			sscanf( ld_line, "SPEED %f", &poaOneAnim->oa_SecsPerFrame);

      CDynamicArray<CTString> astrFrames;
      SLONG slLastPos;
      FOREVER
      {
        slLastPos = File->GetPos_t();
        File->GetLine_t(ld_line, 128);
        _strupr( ld_line);
			  // jump over old key word "FRAMES" and comments
        if( EQUAL_SUB_STR( "FRAMES") || (ld_line[0]==';') ) continue;
        // key words that start or end animations or empty line breaks frame reading
        if( (EQUAL_SUB_STR( "ANIMATION")) ||
            (strlen( ld_line)== 0) ||
            (EQUAL_SUB_STR( "ANIM_END")) ) break;

        sscanf( ld_line, "%s", key_word);
        if( key_word == CTString( "ANIM"))
        {
				  // read file name from line and add it at the end of last path string loaded
				  sscanf( ld_line, "%s %s", error_str, anim_name);
          // search trough all allready readed animations for macro one
	        FOREACHINLIST(COneAnimNode, coan_Node, TempAnimationList, itOAN)
	        {
            if( itOAN->coan_OneAnim->oa_Name == CTString( anim_name))
            {
              CTString *pstrMacroFrames = astrFrames.New( itOAN->coan_OneAnim->oa_NumberOfFrames);
              for( INDEX iMacroFrame = 0; iMacroFrame<itOAN->coan_OneAnim->oa_NumberOfFrames; iMacroFrame++)
              {
                *pstrMacroFrames = GetFrameFileName( pFrameFileList, itOAN->coan_OneAnim->oa_FrameIndices[iMacroFrame]);
                pstrMacroFrames++;
              }
            }
          }
        }
        else
        {
				  // read file name from line and add it at the end of last path string loaded
				  sscanf( ld_line, "%s", file_name);
				  sprintf( full_path, "%s%s", base_path, file_name);
          CTString *pstrNewFile = astrFrames.New(1);
          *pstrNewFile = CTString( full_path);
        }
			}
      if( astrFrames.Count() == 0)
      {
        ThrowF_t( "Can't find any frames for animation %s.\nThere must be at least 1 frame "
          "per animation.\nList of frames must start at line after line containing key"
          "word SPEED.", poaOneAnim->oa_Name);
      }
      // set position before last line readed
      File->SetPos_t( slLastPos);
			// Allocate array of indices
      poaOneAnim->oa_NumberOfFrames = astrFrames.Count();
			poaOneAnim->oa_FrameIndices = (INDEX *) AllocMemory( poaOneAnim->oa_NumberOfFrames * sizeof( INDEX));

      INDEX iFrame = 0;
      FOREACHINDYNAMICARRAY( astrFrames, CTString, itStrFrame)
      {
        // find existing index (of insert new one) for this file name into FileNameList
			  poaOneAnim->oa_FrameIndices[ iFrame] = FindFrameIndex( pFrameFileList, *itStrFrame);
        iFrame++;
      }
      // clear used array
      astrFrames.Clear();
			// Add this new animation instance to temporary animation list
			new COneAnimNode( poaOneAnim, &TempAnimationList);
      ad_NumberOfAnims ++;
		}
		else if( EQUAL_SUB_STR( "ANIM_END"))
			break;
		else
		{
			sprintf(error_str, "Incorrect word readed from script file.\n");
			strcat(error_str, "Probable cause: missing \"ANIM_END\" key-word at end of animation list.");
			throw(error_str);
		}
	}

	lc = TempAnimationList.Count();
	ASSERT(lc!=0);

	// create array of OneAnim object containing members as many as temporary list
	ad_Anims = new COneAnim[ lc];

	// copy list to array
	lc=0;
	FOREACHINLIST(COneAnimNode, coan_Node, TempAnimationList, it2)
	{
    strcpy( ad_Anims[ lc].oa_Name, it2->coan_OneAnim->oa_Name);
    ad_Anims[ lc].oa_SecsPerFrame = it2->coan_OneAnim->oa_SecsPerFrame;
    ad_Anims[ lc].oa_NumberOfFrames = it2->coan_OneAnim->oa_NumberOfFrames;
    ad_Anims[ lc].oa_FrameIndices = (INDEX *) AllocMemory( ad_Anims[ lc].oa_NumberOfFrames *
                                    sizeof(INDEX));
    for( i=0; i<it2->coan_OneAnim->oa_NumberOfFrames; i++)
      ad_Anims[ lc].oa_FrameIndices[ i] = it2->coan_OneAnim->oa_FrameIndices[ i];
		lc++;
	}
  FORDELETELIST( COneAnimNode, coan_Node, TempAnimationList, litDel)
    delete &litDel.Current();

}

void CAnimData::Write_t( CTStream *ostrFile)  // throw char *
{
	SLONG i;
	// First we save main ID
	ostrFile->WriteID_t( CChunkID( "ADAT"));
	// Then we save number of how many animations do we have and then save them all
	ostrFile->Write_t( &ad_NumberOfAnims, sizeof( INDEX));
	for( i=0; i<ad_NumberOfAnims; i++)
	{
		// Next block saves all data for one animation
		ostrFile->Write_t( &ad_Anims[i].oa_Name, sizeof( NAME));
		ostrFile->Write_t( &ad_Anims[i].oa_SecsPerFrame, sizeof( TIME));
		ostrFile->Write_t( &ad_Anims[i].oa_NumberOfFrames, sizeof( INDEX));
		ostrFile->Write_t( ad_Anims[i].oa_FrameIndices,
							ad_Anims[i].oa_NumberOfFrames * sizeof( INDEX));
	}
}

// print #define <animation name> lines for all animations into given file
void CAnimData::ExportAnimationNames_t( CTStream *ostrFile, CTString strAnimationPrefix) // throw char *
{
  char chrLine[ 256];
  // for each animation
  for( INDEX iAnimation=0; iAnimation<ad_NumberOfAnims; iAnimation++)
  {
    // prepare one #define line (add prefix)
    sprintf( chrLine, "#define %s%s %d", (const char *) strAnimationPrefix.str_String, ad_Anims[ iAnimation].oa_Name,
             iAnimation);
    // put it into file
    ostrFile->PutLine_t( chrLine);
  }
}

// Get info about some animation
void CAnimData::GetAnimInfo(INDEX iAnimNo, CAnimInfo &aiInfo) const
{
  if(iAnimNo>=ad_NumberOfAnims) {
    iAnimNo = 0;
  }
  strcpy( aiInfo.ai_AnimName, ad_Anims[ iAnimNo].oa_Name);
  aiInfo.ai_SecsPerFrame = ad_Anims[ iAnimNo].oa_SecsPerFrame;
  aiInfo.ai_NumberOfFrames = ad_Anims[ iAnimNo].oa_NumberOfFrames;
}

// Add animation
void CAnimData::AddAnimation(void)
{
	COneAnim *pNewAnims = new COneAnim[ ad_NumberOfAnims+1];
  for( INDEX iOldAnim=0; iOldAnim<ad_NumberOfAnims; iOldAnim++)
  {
    pNewAnims[ iOldAnim] = ad_Anims[ iOldAnim];
  }
  // set default values for added animation
  strcpy(pNewAnims[ ad_NumberOfAnims].oa_Name, "New animation");
  pNewAnims[ ad_NumberOfAnims].oa_SecsPerFrame = 0.02f;
  // create one frame for this animation
  pNewAnims[ ad_NumberOfAnims].oa_NumberOfFrames = 1;
  pNewAnims[ ad_NumberOfAnims].oa_FrameIndices = (INDEX *) AllocMemory( sizeof( INDEX));
  pNewAnims[ ad_NumberOfAnims].oa_FrameIndices[0] = 0;

  // release old array
  delete[] ad_Anims;
  // copy animations ptr
  ad_Anims = pNewAnims;
  ad_NumberOfAnims++;
}

// replaces requested animation's name with given one
void CAnimData::SetName( INDEX iAnimation, CTString strNewName)
{
  ASSERT(strlen(strNewName)<NAME_SIZE);
  strcpy( ad_Anims[iAnimation].oa_Name, strNewName);
}

// replaces requested animation's speed with given one
void CAnimData::SetSpeed( INDEX iAnimation, TIME tmSpeed)
{
  ad_Anims[iAnimation].oa_SecsPerFrame = tmSpeed;
}

// obtains frame index for given place in array representing given animation
INDEX CAnimData::GetFrame( INDEX iAnimation, INDEX iFramePlace)
{
  ASSERT( iFramePlace<ad_Anims[iAnimation].oa_NumberOfFrames);
  return ad_Anims[iAnimation].oa_FrameIndices[iFramePlace];
}

// sets frame index for given place in array representing given animation
void CAnimData::SetFrame( INDEX iAnimation, INDEX iFramePlace, INDEX iNewFrame)
{
  ASSERT( iFramePlace<ad_Anims[iAnimation].oa_NumberOfFrames);
  ad_Anims[iAnimation].oa_FrameIndices[iFramePlace] = iNewFrame;
}

/* Get number of animations. */
INDEX CAnimData::GetAnimsCt(void) const
{
  return ad_NumberOfAnims;
}

// Delete animation
void CAnimData::DeleteAnimation(INDEX iAnim)
{
  if( ad_NumberOfAnims <= 1) return;
	COneAnim *pNewAnims = new COneAnim[ ad_NumberOfAnims-1];
  INDEX iNewAnim = 0;
  for( INDEX iOldAnim=0; iOldAnim<ad_NumberOfAnims; iOldAnim++)
  {
    // copy all animations except one to delete
    if( iOldAnim != iAnim)
    {
      pNewAnims[ iNewAnim] = ad_Anims[ iOldAnim];
      iNewAnim++;
    }
  }
  // release old array of animation
	delete[] ad_Anims;
  // copy animations ptr
  ad_Anims = pNewAnims;
  ad_NumberOfAnims--;
}

// While loading object containing DataObject and expect DataObject definition to be
// loaded, call its Load function. Then it will call this Read function to load data
// from an open file
void CAnimData::Read_t( CTStream *istrFile) // throw char *
{
	ASSERT( ad_Anims == NULL);
	SLONG i;
	// First we recognize main ID
	istrFile->ExpectID_t( CChunkID( "ADAT"));
	// Then we load and create number of animations
	(*istrFile)>>ad_NumberOfAnims;
	ad_Anims = new COneAnim[ ad_NumberOfAnims];
	for( i=0; i<ad_NumberOfAnims; i++)
	{
		// Next block reads and allocates all data for one animation
		istrFile->Read_t(ad_Anims[i].oa_Name, sizeof (ad_Anims[i].oa_Name));  // char[32]
		(*istrFile)>>ad_Anims[i].oa_SecsPerFrame;
		(*istrFile)>>ad_Anims[i].oa_NumberOfFrames;
		ad_Anims[i].oa_FrameIndices = (INDEX *)
								AllocMemory( ad_Anims[i].oa_NumberOfFrames * sizeof( INDEX));
        for (SLONG j = 0; j < ad_Anims[i].oa_NumberOfFrames; j++)
            (*istrFile)>>ad_Anims[i].oa_FrameIndices[j];
	}
}

/*
 * Default constructor.
 */
CAnimObject::CAnimObject(void)
{
  // set invalid data for validation check
	ao_AnimData = NULL;
  ao_tmAnimStart = 0.0f;
	ao_iCurrentAnim = -1;
	ao_iLastAnim = -1;
  ao_ulFlags = AOF_PAUSED;
}

/* Destructor. */
CAnimObject::~CAnimObject(void)
{
  if(ao_AnimData != NULL) ao_AnimData->RemReference();
}

// copy from another object of same class
ENGINE_API void CAnimObject::Copy(CAnimObject &aoOther)
{
  SetData(aoOther.GetData());
  ao_tmAnimStart  = aoOther.ao_tmAnimStart;
  ao_iCurrentAnim = aoOther.ao_iCurrentAnim;
	ao_iLastAnim    = aoOther.ao_iLastAnim;
  ao_ulFlags      = aoOther.ao_ulFlags;
}
// synchronize with another animation object (set same anim and frames)
ENGINE_API void CAnimObject::Synchronize(CAnimObject &aoOther)
{
  // copy animations, time and flags
  INDEX ctAnims = GetAnimsCt();
  ao_tmAnimStart  = aoOther.ao_tmAnimStart;
  ao_iCurrentAnim = ClampUp(aoOther.ao_iCurrentAnim, ctAnims-1);
	ao_iLastAnim    = ClampUp(aoOther.ao_iLastAnim, ctAnims-1);
  ao_ulFlags      = aoOther.ao_ulFlags;
}

/*
 * Get animation's length.
 */
FLOAT CAnimObject::GetAnimLength(INDEX iAnim) const
{
  if(ao_AnimData == NULL) return 1.0f;
	ASSERT( ao_AnimData != NULL);
  if(iAnim>=ao_AnimData->ad_NumberOfAnims) {
    iAnim = 0;
  }
	ASSERT( (iAnim >= 0) && (iAnim < ao_AnimData->ad_NumberOfAnims) );
	COneAnim *pCOA = &ao_AnimData->ad_Anims[iAnim];
  return pCOA->oa_NumberOfFrames*pCOA->oa_SecsPerFrame;
}

FLOAT CAnimObject::GetCurrentAnimLength(void) const
{
  return GetAnimLength(ao_iCurrentAnim);
}


/*
 * Calculate frame that coresponds to given time.
 */
INDEX CAnimObject::FrameInTime(TIME time) const
{
	ASSERT( ao_AnimData != NULL);
	ASSERT( (ao_iCurrentAnim >= 0) && (ao_iCurrentAnim < ao_AnimData->ad_NumberOfAnims) );
  INDEX iFrameInAnim;

	COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
  if( ao_ulFlags&AOF_PAUSED) {
    // return index of paused frame inside global frame array
    iFrameInAnim = ClipFrame(pCOA->oa_NumberOfFrames + ClipFrame( FloatToInt(ao_tmAnimStart/pCOA->oa_SecsPerFrame)));
  } else {
    // return index of frame inside global frame array of frames in given moment
	  iFrameInAnim = ClipFrame( FloatToInt((time - ao_tmAnimStart)/pCOA->oa_SecsPerFrame));
  }
  return pCOA->oa_FrameIndices[iFrameInAnim];
}


/*
 * Pauses animation
 */
void CAnimObject::PauseAnim(void)
{
  if( ao_ulFlags&AOF_PAUSED) return;                          // dont pause twice
  ao_ulFlags |= AOF_PAUSED;
  ao_tmAnimStart = _pTimer->CurrentTick() - ao_tmAnimStart;  // set difference from current time as start time,
  MarkChanged();                                  // so get frame will get correct current frame
}

/*
 * Continues paused animation
 */
void CAnimObject::ContinueAnim(void)
{
  if( !(ao_ulFlags&AOF_PAUSED)) return;
  // calculate freezed frame index inside current animation (not in global list of frames!)
  COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
  if (pCOA->oa_NumberOfFrames<=0) {
    return;
  }
  INDEX iStoppedFrame = (pCOA->oa_NumberOfFrames + (SLONG)(ao_tmAnimStart/pCOA->oa_SecsPerFrame)
                         % pCOA->oa_NumberOfFrames) % pCOA->oa_NumberOfFrames;
  // using current frame index calculate time so animation continues from same frame
  ao_tmAnimStart = _pTimer->CurrentTick() - pCOA->oa_SecsPerFrame * iStoppedFrame;
  ao_ulFlags &= ~AOF_PAUSED;
  MarkChanged();
}

/*
 * Offsets the animation phase
 */
void CAnimObject::OffsetPhase(TIME tm)
{
  ao_tmAnimStart += tm;
}


/*
 * Loop anims forward
 */
void CAnimObject::NextAnim()
{
	ASSERT( ao_iCurrentAnim != -1);
	ASSERT( ao_AnimData != NULL);
	ao_iCurrentAnim = (ao_iCurrentAnim + 1) % ao_AnimData->ad_NumberOfAnims;
  ao_iLastAnim = ao_iCurrentAnim;
	ao_tmAnimStart = _pTimer->CurrentTick();
  MarkChanged();
}

/*
 * Loop anims backward
 */
void CAnimObject::PrevAnim()
{
	ASSERT( ao_iCurrentAnim != -1);
	ASSERT( ao_AnimData != NULL);
	ao_iCurrentAnim = (ao_AnimData->ad_NumberOfAnims + ao_iCurrentAnim - 1) %
                   ao_AnimData->ad_NumberOfAnims;
  ao_iLastAnim = ao_iCurrentAnim;
	ao_tmAnimStart = _pTimer->CurrentTick();
  MarkChanged();
}

/*
 * Selects frame for given time offset from animation start (0)
 */
void CAnimObject::SelectFrameInTime(TIME tmOffset)
{
  ao_tmAnimStart = tmOffset;  // set fixed start time
  MarkChanged();
}

void CAnimObject::FirstFrame(void)
{
  SelectFrameInTime(0.0f);
}

void CAnimObject::LastFrame(void)
{
  class COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim]; 
  SelectFrameInTime(GetAnimLength(ao_iCurrentAnim)-pCOA->oa_SecsPerFrame);
}

/*
 * Loop frames forward
 */
void CAnimObject::NextFrame()
{
	ASSERT( ao_iCurrentAnim != -1);
	ASSERT( ao_AnimData != NULL);
  ASSERT( ao_ulFlags&AOF_PAUSED);
	ao_tmAnimStart += ao_AnimData->ad_Anims[ ao_iCurrentAnim].oa_SecsPerFrame;
  MarkChanged();
}

/*
 * Loop frames backward
 */
void CAnimObject::PrevFrame()
{
	ASSERT( ao_iCurrentAnim != -1);
	ASSERT( ao_AnimData != NULL);
  ASSERT( ao_ulFlags&AOF_PAUSED);
	ao_tmAnimStart -= ao_AnimData->ad_Anims[ ao_iCurrentAnim].oa_SecsPerFrame;
  MarkChanged();
}

/*
 * Retrieves paused flag
 */
BOOL CAnimObject::IsPaused()
{
  return ao_ulFlags&AOF_PAUSED;
}

/*
 * Test if some updateable object is up to date with this anim object.
 */
BOOL CAnimObject::IsUpToDate(const CUpdateable &ud) const
{
  // if the object itself has changed, or its data has changed
  if (!CChangeable::IsUpToDate(ud) || !ao_AnimData->IsUpToDate(ud)) {
    // something has changed
    return FALSE;
  }
  // otherwise, nothing has changed
  return TRUE;
}

/*
 * Attach data to this object.
 */
void CAnimObject::SetData(CAnimData *pAD)
{
  // mark new data as referenced once more
  if(pAD != NULL) pAD->AddReference();
  // mark old data as referenced once less
  if(ao_AnimData != NULL) ao_AnimData->RemReference();
  // remember new data
  ao_AnimData = pAD;
  if( pAD != NULL) StartAnim( 0);
  // mark that something has changed
  MarkChanged();
}

// obtain anim and set it for this object
void CAnimObject::SetData_t(const CTFileName &fnmAnim) // throw char *
{
  // if the filename is empty
  if (fnmAnim=="") {
    // release current anim
    SetData(NULL);

  // if the filename is not empty
  } else {
    // obtain it (adds one reference)
    CAnimData *pad = _pAnimStock->Obtain_t(fnmAnim);
    // set it as data (adds one more reference, and remove old reference)
    SetData(pad);
    // release it (removes one reference)
    _pAnimStock->Release(pad);
    // total reference count +1+1-1 = +1 for new data -1 for old data
  }
}


/*
 * Sets new animation (but doesn't starts it).
 */
void CAnimObject::SetAnim(INDEX iNew)
{
  if(ao_AnimData == NULL) return;
  // clamp animation
  if( iNew >= GetAnimsCt() )
  {
    iNew = 0;
  }
  // if new animation
  if (ao_iCurrentAnim!=iNew) {
    // remember starting time
	  ao_tmAnimStart = _pTimer->CurrentTick();
  }
  // set new animation number
	ao_iCurrentAnim=iNew;
  ao_iLastAnim=iNew;
  // mark that something has changed
  MarkChanged();
}

/*
 * Start new animation.
 */
void CAnimObject::StartAnim(INDEX iNew)
{
  if(ao_AnimData == NULL) return;
  // set new animation
  SetAnim( iNew);
  // set pause off, looping on
  ao_ulFlags = AOF_LOOPING;
}

/* Start playing an animation. */
void CAnimObject::PlayAnim(INDEX iNew, ULONG ulFlags)
{
  if(ao_AnimData == NULL) return;
  // clamp animation
  if( iNew >= GetAnimsCt() ) {
    iNew = 0;
  }

  // if anim needs to be reset at start
  if (!(ulFlags&AOF_NORESTART) || ao_iCurrentAnim!=iNew) {
    
    // if smooth transition
    if (ulFlags&AOF_SMOOTHCHANGE) {
      // calculate time to end of the current anim
    	class COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
      TIME tmNow = _pTimer->CurrentTick();
      TIME tmLength = GetCurrentAnimLength();
      FLOAT fFrame = ((tmNow - ao_tmAnimStart)/pCOA->oa_SecsPerFrame);
      INDEX iFrame = INDEX(fFrame);
      FLOAT fFract = fFrame-iFrame;
      iFrame = ClipFrame(iFrame);
      TIME tmPassed = (iFrame+fFract)*pCOA->oa_SecsPerFrame;
      TIME tmLeft = tmLength-tmPassed;
      // set time ahead to end of the current animation
	    ao_tmAnimStart = tmNow+tmLeft;
      // remember last animation
      ao_iLastAnim = ao_iCurrentAnim;
      // set new animation number
	    ao_iCurrentAnim = iNew;

    // if normal transition
    } else {
      ao_iLastAnim    = iNew;
	    ao_iCurrentAnim = iNew;
      // remember starting time
	    ao_tmAnimStart = _pTimer->CurrentTick();
    }
  // if anim doesn't need be reset at start
  } else {
    // do nothing
    NOTHING;
  }
  // set pause off, looping flag from flags
  ao_ulFlags = ulFlags&(AOF_LOOPING|AOF_PAUSED);

  // mark that something has changed
  MarkChanged();
}

/* Seamlessly continue playing another animation from same point. */
void CAnimObject::SwitchToAnim(INDEX iNew)
{
  if(ao_AnimData == NULL) return;
  // clamp animation
  if( iNew >= GetAnimsCt() )
  {
    iNew = 0;
  }
  // set new animation number
	ao_iCurrentAnim=iNew;
  ao_iLastAnim = ao_iCurrentAnim;
}

/*
 * Reset anim (restart)
 */
void CAnimObject::ResetAnim()
{
  if(ao_AnimData == NULL) return;
  // remember starting time
	ao_tmAnimStart = _pTimer->CurrentTick();
  // mark that something has changed
  MarkChanged();
}

// Get info about some animation
void CAnimObject::GetAnimInfo(INDEX iAnimNo, CAnimInfo &aiInfo) const
{
  if (iAnimNo >= ao_AnimData->ad_NumberOfAnims) {
    iAnimNo = 0;
  }
  ASSERT( iAnimNo < ao_AnimData->ad_NumberOfAnims);
  strcpy( aiInfo.ai_AnimName, ao_AnimData->ad_Anims[ iAnimNo].oa_Name);
  aiInfo.ai_SecsPerFrame = ao_AnimData->ad_Anims[ iAnimNo].oa_SecsPerFrame;
  aiInfo.ai_NumberOfFrames = ao_AnimData->ad_Anims[ iAnimNo].oa_NumberOfFrames;
}

// clip frame index to be inside valid range (wrap around for looping anims)
INDEX CAnimObject::ClipFrame(INDEX iFrame) const
{
  if (ao_AnimData->ad_NumberOfAnims==0) {
    return 0;
  }
  class COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
  // if looping
  if (ao_ulFlags&AOF_LOOPING) {
    // wrap-around
    if (pCOA->oa_NumberOfFrames<=0) {
      return 0;
    }
    return ULONG(iFrame)%pCOA->oa_NumberOfFrames;
  // if not looping
  } else {
    // clamp
    if (iFrame<0) {
      return 0;
    } else if (iFrame>=pCOA->oa_NumberOfFrames) {
      return pCOA->oa_NumberOfFrames-1;
    } else {
      return iFrame;
    }
  }
}

// Get info about time passed until now in current animation
TIME CAnimObject::GetPassedTime(void) const
{
  if(ao_AnimData == NULL) return 0.0f;
  INDEX iStoppedFrame;
	class COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
  if( !(ao_ulFlags&AOF_PAUSED))
    iStoppedFrame = ClipFrame((INDEX)((_pTimer->CurrentTick() - ao_tmAnimStart)/pCOA->oa_SecsPerFrame));
  else
    iStoppedFrame = ClipFrame((INDEX)(ao_tmAnimStart/pCOA->oa_SecsPerFrame));
  return( iStoppedFrame * pCOA->oa_SecsPerFrame);
}

/*
 * If animation is finished
 */
BOOL CAnimObject::IsAnimFinished(void) const
{
  if(ao_AnimData == NULL) return FALSE;
  if(ao_ulFlags&AOF_LOOPING) return FALSE;

  INDEX iStoppedFrame;
	class COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
  if( !(ao_ulFlags&AOF_PAUSED))
    iStoppedFrame = ClipFrame((INDEX)((_pTimer->CurrentTick() - ao_tmAnimStart)/pCOA->oa_SecsPerFrame));
  else
    iStoppedFrame = ClipFrame((INDEX)(ao_tmAnimStart/pCOA->oa_SecsPerFrame));
  return( iStoppedFrame == pCOA->oa_NumberOfFrames-1);
}

// get number of animations in curent anim data
INDEX CAnimObject::GetAnimsCt(void) const
{
  if(ao_AnimData == NULL) return 1;
  ASSERT( ao_AnimData != NULL);
  return( ao_AnimData->ad_NumberOfAnims);
}

// get index of current animation
INDEX CAnimObject::GetAnim(void) const
{
	return( ao_iCurrentAnim);
}

/*
 * Gets the number of current frame.
 */
INDEX CAnimObject::GetFrame(void) const
{
  return FrameInTime(_pTimer->CurrentTick());  // return frame index that coresponds to current moment
}

/* Gets number of frames in current anim. */
INDEX CAnimObject::GetFramesInCurrentAnim(void) const
{
  ASSERT( ao_AnimData != NULL);
  return ao_AnimData->ad_Anims[ao_iCurrentAnim].oa_NumberOfFrames;
}

/*
 * Get  information for linear interpolation beetween frames.
 */
void CAnimObject::GetFrame( INDEX &iFrame0, INDEX &iFrame1, FLOAT &fRatio) const
{
  if(ao_AnimData == NULL || 
    ao_AnimData->ad_NumberOfAnims<=0 ||
    ao_AnimData->ad_Anims[ao_iCurrentAnim].oa_NumberOfFrames<=0)
  {
    iFrame0 = 0;
    iFrame1 = 0;
    fRatio =0.0f;
    return;
  }
	ASSERT( ao_AnimData != NULL);
	ASSERT( (ao_iCurrentAnim >= 0) && (ao_iCurrentAnim < ao_AnimData->ad_NumberOfAnims) );
  TIME tmNow = _pTimer->CurrentTick() + _pTimer->GetLerpFactor()*_pTimer->TickQuantum;

  if( ao_ulFlags&AOF_PAUSED)
  {
    // return index of paused frame inside global frame array
	  class COneAnim *pCOA = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
    INDEX iStoppedFrame = ClipFrame((SLONG)(ao_tmAnimStart/pCOA->oa_SecsPerFrame));
	  iFrame0 = iFrame1 = pCOA->oa_FrameIndices[ iStoppedFrame];
    fRatio = 0.0f;
  }
  else
  {
    // return index of frame inside global frame array of frames in given moment
    TIME tmCurrentRelative = tmNow - ao_tmAnimStart;
    if (tmCurrentRelative>=0) {
  	  class COneAnim *pOA0 = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
      float fFrameNow = (tmCurrentRelative)/pOA0->oa_SecsPerFrame;
	    iFrame0 = pOA0->oa_FrameIndices[ ClipFrame(ULONG(fFrameNow))];
	    iFrame1 = pOA0->oa_FrameIndices[ ClipFrame(ULONG(fFrameNow+1))];
      fRatio = fFrameNow - (float)floor(fFrameNow);
    } else {
  	  class COneAnim *pOA0 = &ao_AnimData->ad_Anims[ao_iLastAnim];
  	  class COneAnim *pOA1 = &ao_AnimData->ad_Anims[ao_iCurrentAnim];
      INDEX iAnim = ao_iCurrentAnim;
      ((CAnimObject*)this)->ao_iCurrentAnim = ao_iLastAnim;
      float fFrameNow = tmCurrentRelative/pOA0->oa_SecsPerFrame+pOA0->oa_NumberOfFrames;
	    iFrame0 = pOA0->oa_FrameIndices[ Clamp(SLONG(fFrameNow), (SLONG)0, pOA0->oa_NumberOfFrames-1)];
      INDEX iFrameNext = SLONG(fFrameNow+1);
      if (iFrameNext>=pOA0->oa_NumberOfFrames) {
	      iFrame1 = pOA1->oa_FrameIndices[0];
      } else {
	      iFrame1 = pOA0->oa_FrameIndices[ Clamp(iFrameNext,  (INDEX)0, pOA0->oa_NumberOfFrames-1)];
      }
      ((CAnimObject*)this)->ao_iCurrentAnim = iAnim;
      fRatio = fFrameNow - (float)floor(fFrameNow);
    }
  }
}

void CAnimObject::Write_t( CTStream *pstr) // throw char *
{
  (*pstr).WriteID_t("ANOB");
	(*pstr)<<ao_tmAnimStart;
	(*pstr)<<ao_iCurrentAnim;
	(*pstr)<<ao_iLastAnim;
	(*pstr)<<ao_ulFlags;
}

void CAnimObject::Read_t( CTStream *pstr) // throw char *
{
  if ((*pstr).PeekID_t()==CChunkID("ANOB")) {
    (*pstr).ExpectID_t("ANOB");
	  (*pstr)>>ao_tmAnimStart;
	  (*pstr)>>ao_iCurrentAnim;
	  (*pstr)>>ao_iLastAnim;
	  (*pstr)>>ao_ulFlags;
  } else {
	  (*pstr)>>ao_tmAnimStart;
	  (*pstr)>>ao_iCurrentAnim;
    ao_iLastAnim = ao_iCurrentAnim;
    ao_ulFlags = 0;
  }

  // clamp animation
  if (ao_AnimData==NULL || ao_iCurrentAnim >= GetAnimsCt() )
  {
    ao_iCurrentAnim = 0;
  }
  // clamp animation
  if (ao_AnimData==NULL || ao_iLastAnim >= GetAnimsCt() )
  {
    ao_iLastAnim = 0;
  }
}

