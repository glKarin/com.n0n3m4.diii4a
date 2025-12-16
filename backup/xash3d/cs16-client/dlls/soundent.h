/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#define MAX_WORLD_SOUNDS 64

#define bits_SOUND_NONE 0
#define bits_SOUND_COMBAT (1<<0)
#define bits_SOUND_WORLD (1<<1)
#define bits_SOUND_PLAYER (1<< 2)
#define bits_SOUND_CARCASS (1<< 3)
#define bits_SOUND_MEAT (1<< 4)
#define bits_SOUND_DANGER (1<< 5)
#define bits_SOUND_GARBAGE (1<< 6)

#define bits_ALL_SOUNDS 0xFFFFFFFF

#define SOUNDLIST_EMPTY	-1

#define SOUNDLISTTYPE_FREE 1
#define SOUNDLISTTYPE_ACTIVE 2

#define SOUND_NEVER_EXPIRE -1

class CSound
{
public:
	void Clear(void);
	void Reset(void);

public:
	Vector m_vecOrigin;
	int m_iType;
	int m_iVolume;
	float m_flExpireTime;
	int m_iNext;
	int m_iNextAudible;

public:
	BOOL FIsSound(void);
	BOOL FIsScent(void);
};

class CSoundEnt : public CBaseEntity 
{
public:
	void Precache(void);
	void Spawn(void);
	void Think(void);
	void Initialize(void);
	int ObjectCaps(void) { return FCAP_DONT_SAVE; }

public:
	static void InsertSound(int iType, const Vector &vecOrigin, int iVolume, float flDuration);
	static void FreeSound(int iSound, int iPrevious);
	static int ActiveList(void);
	static int FreeList(void);
	static CSound *SoundPointerForIndex(int iIndex);
	static int ClientSoundIndex(edict_t *pClient);

public:
	BOOL IsEmpty(void) { return m_iActiveSound == SOUNDLIST_EMPTY; }
	int ISoundsInList(int iListType);
	int IAllocSound(void);

public:
	int m_iFreeSound;
	int m_iActiveSound;
	int m_cLastActiveSounds;
	BOOL m_fShowReport;

private:
	CSound m_SoundPool[MAX_WORLD_SOUNDS];
};