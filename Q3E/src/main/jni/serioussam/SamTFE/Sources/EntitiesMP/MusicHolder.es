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

222
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/EnemyBase.h"
#include "EntitiesMP/EnemySpawner.h"
#include "EntitiesMP/Trigger.h"
#include "EntitiesMP/Woman.h"
%}


enum MusicType {
  0 MT_LIGHT  "light",
  1 MT_MEDIUM "medium",
  2 MT_HEAVY  "heavy",
  3 MT_EVENT  "event",
  4 MT_CONTINUOUS  "continuous",
};

event EChangeMusic {
  enum MusicType mtType,
  CTFileName fnMusic,
  FLOAT fVolume,
  BOOL bForceStart,
};

%{
#define MUSIC_VOLUMEMIN   0.02f     // minimum volume (considered off)
#define MUSIC_VOLUMEMAX   0.98f     // maximum volume (considered full)

float FadeInFactor(TIME fFadeTime)
{
  return (float) pow(MUSIC_VOLUMEMAX/MUSIC_VOLUMEMIN, 1/(fFadeTime/_pTimer->TickQuantum));
}
float FadeOutFactor(TIME fFadeTime)
{
  return (float) pow(MUSIC_VOLUMEMIN/MUSIC_VOLUMEMAX, 1/(fFadeTime/_pTimer->TickQuantum));
}
%}

class CMusicHolder : CRationalEntity {
name      "MusicHolder";
thumbnail "Thumbnails\\MusicHolder.tbn";
features "HasName", "IsTargetable", "IsImportant";

properties:
  1 CTString m_strName     "" = "MusicHolder",
  2 FLOAT m_fScoreMedium "Score Medium" = 100.0f,
  3 FLOAT m_fScoreHeavy  "Score Heavy"  = 1000.0f,

 10 CTFileName m_fnMusic0 "Music Light" 'M' = CTFILENAME(""),
 11 CTFileName m_fnMusic1 "Music Medium"    = CTFILENAME(""),
 12 CTFileName m_fnMusic2 "Music Heavy"     = CTFILENAME(""),
 13 CTFileName m_fnMusic3                   = CTFILENAME(""),  // event music
 14 CTFileName m_fnMusic4                   = CTFILENAME(""),  // continuous music

 20 FLOAT m_fVolume0  "Volume Light" 'V' = 1.0f,
 21 FLOAT m_fVolume1  "Volume Medium"    = 1.0f,
 22 FLOAT m_fVolume2  "Volume Heavy"     = 1.0f,
 23 FLOAT m_fVolume3                     = 1.0f,  // event volume
 24 FLOAT m_fVolume4                     = 1.0f,  // continuous volume

// internals

100 CEntityPointer m_penBoss,    // current boss if any
102 CEntityPointer m_penCounter,   // enemy counter for wave-fight progress display
104 INDEX m_ctEnemiesInWorld = 0,   // count of total enemies in world
105 CEntityPointer m_penRespawnMarker,    // respawn marker for coop
106 INDEX m_ctSecretsInWorld = 0,   // count of total secrets in world
101 FLOAT m_tmFade = 1.0f,    // music cross-fade speed
103 enum MusicType m_mtCurrentMusic = MT_LIGHT, // current active channel

// for cross-fade purposes
110 FLOAT m_fCurrentVolume0a  = 1.0f,
210 FLOAT m_fCurrentVolume0b  = 1.0f,
111 FLOAT m_fCurrentVolume1a  = 1.0f,
211 FLOAT m_fCurrentVolume1b  = 1.0f,
112 FLOAT m_fCurrentVolume2a  = 1.0f,
212 FLOAT m_fCurrentVolume2b  = 1.0f,
113 FLOAT m_fCurrentVolume3a  = 1.0f,
213 FLOAT m_fCurrentVolume3b  = 1.0f,
114 FLOAT m_fCurrentVolume4a  = 1.0f,
214 FLOAT m_fCurrentVolume4b  = 1.0f,

// the music channels
120 CSoundObject m_soMusic0a,
220 CSoundObject m_soMusic0b,
121 CSoundObject m_soMusic1a,
221 CSoundObject m_soMusic1b,
122 CSoundObject m_soMusic2a,
222 CSoundObject m_soMusic2b,
123 CSoundObject m_soMusic3a,
223 CSoundObject m_soMusic3b,
124 CSoundObject m_soMusic4a,
224 CSoundObject m_soMusic4b,

// next free subchannel markers (all starts at subchannel 1(b), first switch goes to subchannel 0(a))
130 INDEX m_iSubChannel0 = 1,
131 INDEX m_iSubChannel1 = 1,
132 INDEX m_iSubChannel2 = 1,
133 INDEX m_iSubChannel3 = 1,
134 INDEX m_iSubChannel4 = 1,

  {
    // array of enemies that make fuss
    CDynamicContainer<CEntity> m_cenFussMakers;
  }

components:
  1 model   MODEL_MARKER     "Models\\Editor\\MusicHolder.mdl",
  2 texture TEXTURE_MARKER   "Models\\Editor\\MusicHolder.tex"


functions:
  // count enemies in current world
  void CountEnemies(void)
  {
    m_ctEnemiesInWorld = 0;
    m_ctSecretsInWorld = 0;
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(GetWorld()->wo_cenEntities, CEntity, iten) {
      CEntity *pen = iten;
      // if enemybase
      if (IsDerivedFromClass(pen, "Enemy Base")) {
        CEnemyBase *penEnemy = (CEnemyBase *)pen;
        // if not template
        if (!penEnemy->m_bTemplate) {
          // count one
          m_ctEnemiesInWorld++;
		      // if this is a woman kamikaze carrier, add another one to count
		      if (IsOfClass(pen, "Woman")) {
			      if (((CWoman *)&*pen)->m_bKamikazeCarrier) { m_ctEnemiesInWorld++; }
		      }
        }
      // if spawner
      } else if (IsDerivedFromClass(pen, "Enemy Spawner")) {
        CEnemySpawner *penSpawner = (CEnemySpawner *)pen;
        // if not teleporting
        if (penSpawner->m_estType!=EST_TELEPORTER) {
          // add total count
          m_ctEnemiesInWorld+=penSpawner->m_ctTotal;
          // if this spawner points to a woman kamikaze carrier template, increase count once more
          if (penSpawner->m_penTarget) {
            if (IsOfClass(penSpawner->m_penTarget, "Woman")) {
              if (((CWoman *) penSpawner->m_penTarget.ep_pen)->m_bKamikazeCarrier) { m_ctEnemiesInWorld+=penSpawner->m_ctTotal; }
            }
          }
        }
      // if trigger
      } else if (IsDerivedFromClass(pen, "Trigger")) {
        CTrigger *penTrigger = (CTrigger *)pen;
        // if has score
        if (penTrigger->m_fScore>0) {
          // it counts as a secret
          m_ctSecretsInWorld++;
        }
      }
    }}
  }

  // check for stale fuss-makers
  void CheckOldFussMakers(void)
  {
    TIME tmNow = _pTimer->CurrentTick();
    TIME tmTooOld = tmNow-10.0f;
    CDynamicContainer<CEntity> cenOldFussMakers;
    // for each fussmaker
    {FOREACHINDYNAMICCONTAINER(m_cenFussMakers, CEntity, itenFussMaker) {
      CEnemyBase & enFussMaker = (CEnemyBase&)*itenFussMaker;
      // if haven't done fuss for too long
      if (enFussMaker.m_tmLastFussTime<tmTooOld) {
        // add to old fuss makers
        cenOldFussMakers.Add(&enFussMaker);
      }
    }}
    // for each old fussmaker
    {FOREACHINDYNAMICCONTAINER(cenOldFussMakers, CEntity, itenOldFussMaker) {
      CEnemyBase &enOldFussMaker = (CEnemyBase&)*itenOldFussMaker;
      // remove from fuss
      enOldFussMaker.RemoveFromFuss();
    }}
  }
  
  // get total score of all active fuss makers
  INDEX GetFussMakersScore(void) {
    INDEX iScore = 0;
    {FOREACHINDYNAMICCONTAINER(m_cenFussMakers, CEntity, itenFussMaker) {
      CEnemyBase &enFussMaker = (CEnemyBase&)*itenFussMaker;
      iScore += (INDEX) enFussMaker.m_iScore;
    }}
    return iScore;
  }

  // change given music channel
  void ChangeMusicChannel(enum MusicType mtType, const CTFileName &fnNewMusic, FLOAT fNewVolume)
  {
    INDEX &iSubChannel = (&m_iSubChannel0)[mtType];
    // take next sub-channel if needed
    if (fnNewMusic!="") {
      iSubChannel = (iSubChannel+1)%2;
    }
    // find channel's variables
    FLOAT &fVolume = (&m_fVolume0)[mtType];
    CSoundObject &soMusic = (&m_soMusic0a)[mtType*2+iSubChannel];
    FLOAT &fCurrentVolume = (&m_fCurrentVolume0a)[mtType*2+iSubChannel];

    // setup looping/non looping flags
    ULONG ulFlags;
    if (mtType==MT_EVENT) {
      ulFlags = SOF_MUSIC;
    } else {
      ulFlags = SOF_MUSIC|SOF_LOOP|SOF_NONGAME;
    }

    // remember volumes
    fVolume = fNewVolume;
    // start new music file if needed
    if (fnNewMusic!="") {
      PlaySound( soMusic, fnNewMusic, ulFlags);
      // initially, not playing
      fCurrentVolume = MUSIC_VOLUMEMIN;
      soMusic.Pause();
      soMusic.SetVolume(fCurrentVolume, fCurrentVolume);
    }
  }

  // fade out one channel
  void FadeOutChannel(INDEX iChannel, INDEX iSubChannel)
  {
    // find channel's variables
    FLOAT &fVolume = (&m_fVolume0)[iChannel];
    CSoundObject &soMusic = (&m_soMusic0a)[iChannel*2+iSubChannel];
    FLOAT &fCurrentVolume = (&m_fCurrentVolume0a)[iChannel*2+iSubChannel];

    // do nothing, if music is not playing
    if( !soMusic.IsPlaying()) { return; }

    // do nothing, if music is already paused
    if( soMusic.IsPaused()) { return; }

    // if minimum volume reached 
    if( fCurrentVolume<MUSIC_VOLUMEMIN) {
      // pause music
      soMusic.Pause();
    } else {
      // music isn't even faded yet, so continue on fading it out
      fCurrentVolume *= FadeOutFactor( m_tmFade);
      soMusic.SetVolume( fCurrentVolume*fVolume, fCurrentVolume*fVolume);
    }
  }

  // fade in one channel
  void FadeInChannel(INDEX iChannel, INDEX iSubChannel)
  {
    // find channel's variables
    FLOAT &fVolume = (&m_fVolume0)[iChannel];
    CSoundObject &soMusic = (&m_soMusic0a)[iChannel*2+iSubChannel];
    FLOAT &fCurrentVolume = (&m_fCurrentVolume0a)[iChannel*2+iSubChannel];

    // do nothing, if music is not playing
    if( !soMusic.IsPlaying()) { return; }

    // resume music if needed
    if( soMusic.IsPaused()) {
      soMusic.Resume();
    }
    // fade in music if needed
    if( fCurrentVolume<MUSIC_VOLUMEMAX) {
      fCurrentVolume *= FadeInFactor( m_tmFade);
      fCurrentVolume = ClampUp( fCurrentVolume, 1.0f);
    }
    soMusic.SetVolume( fCurrentVolume*fVolume, fCurrentVolume*fVolume);
  }

  // fade one channel in or out
  void CrossFadeOneChannel(enum MusicType mtType)
  {
    INDEX iSubChannelActive = (&m_iSubChannel0)[mtType];
    INDEX iSubChannelInactive = (iSubChannelActive+1)%2;
    // if it is current channel
    if (mtType==m_mtCurrentMusic) {
      // fade in active subchannel
      FadeInChannel(mtType, iSubChannelActive);
      // fade out inactive subchannel
      FadeOutChannel(mtType, iSubChannelInactive);
    // if it is not current channel
    } else {
      // fade it out
      FadeOutChannel(mtType, 0);
      FadeOutChannel(mtType, 1);
    }
  }
  
procedures:
  // initialize music
  Main(EVoid) {

    // init as model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    // wait for game to start
    autowait(_pTimer->TickQuantum);

    // prepare initial music channel values
    ChangeMusicChannel(MT_LIGHT,        m_fnMusic0, m_fVolume0);
    ChangeMusicChannel(MT_MEDIUM,       m_fnMusic1, m_fVolume1);
    ChangeMusicChannel(MT_HEAVY,        m_fnMusic2, m_fVolume2);
    ChangeMusicChannel(MT_EVENT,        m_fnMusic3, m_fVolume3);
    ChangeMusicChannel(MT_CONTINUOUS,   m_fnMusic4, m_fVolume4);

    // start with light music
    m_mtCurrentMusic = MT_LIGHT;
    m_fCurrentVolume0a = MUSIC_VOLUMEMAX*0.98f;
    m_tmFade = 0.01f;
    CrossFadeOneChannel(MT_LIGHT);

    // must react after enemyspawner and all enemies, but before player for proper enemy counting
    // (total wait is two ticks so far)
    autowait(_pTimer->TickQuantum);

    // count enemies in current world
    CountEnemies();

    // main loop
    while(TRUE) {
      // wait a bit
      wait(0.1f) {
        on (ETimer) : {
          stop;
        };
        // if music is to be changed
        on (EChangeMusic ecm) : { 
          // change parameters
          ChangeMusicChannel(ecm.mtType, ecm.fnMusic, ecm.fVolume);
          // if force started
          if (ecm.bForceStart) {
            // set as current music
            m_mtCurrentMusic = ecm.mtType;
          }
          // stop waiting
          stop;
        }
      }
      // check fuss
      CheckOldFussMakers();
      // get total score of all active fuss makers
      FLOAT fFussScore = GetFussMakersScore();
      // if event is on
      if (m_mtCurrentMusic==MT_EVENT) {
        // if event has ceased playing
        if (!m_soMusic3a.IsPlaying() && !m_soMusic3b.IsPlaying()) {
          // switch to light music
          m_mtCurrentMusic=MT_LIGHT;
        }
      }
      // if heavy fight is on
      if (m_mtCurrentMusic==MT_HEAVY) {
        // if no more fuss
        if (fFussScore<=0.0f) {
          // switch to no fight
          m_mtCurrentMusic=MT_LIGHT;
        }
      // if medium fight is on
      } else if (m_mtCurrentMusic==MT_MEDIUM) {
        // if no more fuss
        if (fFussScore<=0.0f) {
          // switch to no fight
          m_mtCurrentMusic=MT_LIGHT;
        // if larger fuss
        } else if (fFussScore>=m_fScoreHeavy) {
          // switch to heavy fight
          m_mtCurrentMusic=MT_HEAVY;
        }
      // if no fight is on
      } else if (m_mtCurrentMusic==MT_LIGHT) {
        // if heavy fuss
        if (fFussScore>=m_fScoreHeavy) {
          // switch to heavy fight
          m_mtCurrentMusic=MT_HEAVY;
        // if medium fuss
        } else if (fFussScore>=m_fScoreMedium) {
          // switch to medium fight
          m_mtCurrentMusic=MT_MEDIUM;
        }
      }

      // setup fade speed depending on music type
      if (m_mtCurrentMusic==MT_LIGHT) {
        m_tmFade = 2.0f;
      } else if (m_mtCurrentMusic==MT_MEDIUM) {
        m_tmFade = 1.0f;
      } else if (m_mtCurrentMusic==MT_HEAVY) {
        m_tmFade = 1.0f;
      } else if (m_mtCurrentMusic==MT_EVENT || m_mtCurrentMusic==MT_CONTINUOUS) {
        m_tmFade = 0.5f;
      }

      // fade all channels
      CrossFadeOneChannel(MT_LIGHT);
      CrossFadeOneChannel(MT_MEDIUM);
      CrossFadeOneChannel(MT_HEAVY);
      CrossFadeOneChannel(MT_EVENT);
      CrossFadeOneChannel(MT_CONTINUOUS);
    }
    return;
  }
};
