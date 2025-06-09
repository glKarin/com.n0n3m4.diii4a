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

#ifndef SE_INCL_ANIM_H
#define SE_INCL_ANIM_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Serial.h>

#include <Engine/Base/Lists.h>

#define NAME_SIZE 32
typedef char NAME[NAME_SIZE];

#if (!defined PATH_MAX)
  #define PATH_MAX 260
#endif

typedef char FILE_NAME[PATH_MAX];

/*
 * An object used for obtaining animation's information
 */
class CAnimInfo {
public:
  NAME ai_AnimName;
  TIME ai_SecsPerFrame;	    // speed of this animation
  INDEX ai_NumberOfFrames;
};

/*
 * Node used for linking file names representing frames.
 * Nodes of this kind are returned result of LoadFromScript function.
 */
class ENGINE_API CFileNameNode {
public:
	FILE_NAME cfnn_FileName;
	CListNode cfnn_Node;
	CFileNameNode(const char *NewFileName, CListHead *LH);
};

/*
 * Animation data for a class of animateable objects
 */
class CAnimData : public CSerial {
public:
  INDEX ad_NumberOfAnims;
  class COneAnim *ad_Anims;	    // array of animations

public:

  // fill member variables with invalid data
	ENGINE_API CAnimData();
	// Free allocated data (ad_Anims array), check invalid data
	ENGINE_API ~CAnimData();
  // clears animation data object
  ENGINE_API void Clear();
  // check if this kind of objects is auto-freed
  virtual BOOL IsAutoFreed(void);
  // reference counting functions
  virtual void RemReference_internal(void);

  // get amount of memory used by this object
  SLONG GetUsedMemory(void);

  // reference counting functions
  void AddReference(void);
  void RemReference(void);

  // creates given number of default animations (1 frame, given name and apeed)
  ENGINE_API void CreateAnimations( INDEX ctAnimations, CTString strName="None",
                         INDEX iDefaultFrame=0,TIME tmSpeed=0.02f);
  // replaces frames array with given one
  ENGINE_API void SetFrames( INDEX iAnimation, INDEX ctFrames, INDEX *pNewFrames);
  // replaces requested animation's name with given one
  ENGINE_API void SetName( INDEX iAnimation, CTString strNewName);
  // replaces requested animation's speed with given one
  ENGINE_API void SetSpeed( INDEX iAnimation, TIME tmSpeed);
  // obtains frame index for given place in array representing given animation
  ENGINE_API INDEX GetFrame( INDEX iAnimation, INDEX iFramePlace);
  // sets frame index for given place in array representing given animation
  ENGINE_API void SetFrame( INDEX iAnimation, INDEX iFramePlace, INDEX iNewFrame);
  // fill animation data object vith valid data containing one animation, one frame
	ENGINE_API void DefaultAnimation();
  /* Get animation's info. */
  ENGINE_API void GetAnimInfo(INDEX iAnimNo, CAnimInfo &aiInfo) const;
  /* Add animation */
  ENGINE_API void AddAnimation(void);
  /* Delete animation */
  ENGINE_API void DeleteAnimation(INDEX iAnim);
  /* Get number of animations. */
  ENGINE_API INDEX GetAnimsCt(void) const;
	// load list of frames from script file
  ENGINE_API void LoadFromScript_t( CTStream *File, CListHead *FrameFileList); // throw char *
  // print #define <animation name> lines for all animations into given file
  void ExportAnimationNames_t( CTStream *ostrFile, CTString strAnimationPrefix);  // throw char *
	void Read_t( CTStream *istrFile); // throw char *
	void Write_t( CTStream *ostrFile); // throw char *
};


/*
 * An instance of animateable object
 */
#define AOF_PAUSED        (1L<<0)     // current animation is paused
#define AOF_LOOPING       (1L<<1)     // anim object is playing a looping animation
#define AOF_NORESTART     (1L<<2)     // don't restart anim (used for PlayAnim())
#define AOF_SMOOTHCHANGE  (1L<<3)     // smoothly change between anims

class CAnimObject : public CChangeable {
public:
  TIME ao_tmAnimStart;      // time when current anim was started
  INDEX ao_iCurrentAnim;	  // index of active animation
  ULONG ao_ulFlags;         // flags
  INDEX ao_iLastAnim;       // index of last animation (for smooth transition)

  /* Calculate frame that coresponds to given time. */
  INDEX FrameInTime(TIME time) const;

public:
  CAnimData *ao_AnimData;

public:

  // some of usual smart pointer functions are implemented, because AnimObjects
  // behave as smart pointers to AnimData objects
  /* Default constructor. */
  ENGINE_API CAnimObject(void);
  /* Destructor. */
  ENGINE_API ~CAnimObject(void);
  // copy from another object of same class
  ENGINE_API void Copy(CAnimObject &aoOther);
  // synchronize with another animation object (set same anim and frames)
  ENGINE_API void Synchronize(CAnimObject &aoOther);

  // copying of AnimObjects is not allowed
  inline CAnimObject(const CAnimObject &aoOther) {
    ASSERT(FALSE); };
  inline const CAnimObject &operator=(const CAnimObject &aoOther) {
    ASSERT(FALSE); return *this;};

  // clip frame index to be inside valid range (wrap around for looping anims)
  INDEX ClipFrame(INDEX iFrame) const;
  /* Loop anims forward */
  ENGINE_API void NextAnim(void);
  /* Loop anims backward */
  ENGINE_API void PrevAnim(void);
  /* Loop frames forward */
  ENGINE_API void NextFrame(void);
  /* Loop frames backward */
  ENGINE_API void PrevFrame(void);
  /* Select frame in given time offset */
  ENGINE_API void SelectFrameInTime(TIME tmOffset);
  /* Select first frame */
  ENGINE_API void FirstFrame(void);
  /* Select last frame */
  ENGINE_API void LastFrame(void);
  /* Test if some updateable object is up to date with this anim object. */
  ENGINE_API BOOL IsUpToDate(const CUpdateable &ud) const;
	void Read_t( CTStream *istrFile); // throw char *
	void Write_t( CTStream *ostrFile); // throw char *

  /* Get animation's info. */
  ENGINE_API void GetAnimInfo(INDEX iAnimNo, CAnimInfo &aiInfo) const;

  /* Attach data to this object. */
  ENGINE_API void SetData(CAnimData *pAD);
  // obtain animation and set it for this object
  ENGINE_API void SetData_t(const CTFileName &fnmAnim); // throw char *

  /* Get current anim data ptr. */
  __forceinline CAnimData *GetData() { return ao_AnimData; };

  /* Get animation's length. */
  ENGINE_API FLOAT GetCurrentAnimLength(void) const;
  ENGINE_API FLOAT GetAnimLength(INDEX iAnim) const;
  /* Get number of animations in current anim data */
  ENGINE_API INDEX GetAnimsCt() const;
  /* If animation has finished */
  ENGINE_API BOOL IsAnimFinished(void) const;
  /* Get passed time from start of animation */
  ENGINE_API TIME GetPassedTime(void) const;

  /* Start new animation -- obsolete. */
	ENGINE_API void StartAnim(INDEX iNew);
  /* Start playing an animation. */
	ENGINE_API void PlayAnim(INDEX iNew, ULONG ulFlags);
  /* Seamlessly continue playing another animation from same point. */
	ENGINE_API void SwitchToAnim(INDEX iNew);
  /* Set new animation but doesn't starts it. */
	ENGINE_API void SetAnim(INDEX iNew);
  /* Reset anim (restart) */
  ENGINE_API void ResetAnim();
  /* Pauses current animation. */
	ENGINE_API void PauseAnim();
  /* Continues paused animation. */
	ENGINE_API void ContinueAnim();
  /* Offsets the animation phase */
	ENGINE_API void OffsetPhase(TIME tm);
  /* Retrieves paused flag */
  ENGINE_API BOOL IsPaused(void);
  /* Gets the number of current animation */
  ENGINE_API INDEX GetAnim(void) const;
  /* Gets the number of current frame. */
  ENGINE_API INDEX GetFrame(void) const;
  /* Gets number of frames in current anim. */
  ENGINE_API INDEX GetFramesInCurrentAnim(void) const;
  /* Get  information for linear interpolation beetween frames. */
  ENGINE_API void GetFrame( INDEX &iFrame0, INDEX &iFrame1, FLOAT &fRatio) const;
};


#endif  /* include-once check. */

