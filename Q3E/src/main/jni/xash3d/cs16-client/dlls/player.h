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
#ifndef PLAYER_H
#define PLAYER_H

#include "pm_materials.h"
#include "hintmessage.h"
#include "weapons.h"

#define MAX_PLAYER_NAME_LENGTH 32
#define MAX_AUTOBUY_LENGTH 256
#define MAX_REBUY_LENGTH 256
#define MAX_LACTION_LENGTH 32

#define PLAYER_FATAL_FALL_SPEED (float)1100
#define PLAYER_MAX_SAFE_FALL_SPEED (float)500
#define DAMAGE_FOR_FALL_SPEED (float)100.0 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED)
#define PLAYER_MIN_BOUNCE_SPEED (float)350
#define PLAYER_FALL_PUNCH_THRESHOLD (float)250.0

#define PFLAG_ONLADDER (1<<0)
#define PFLAG_ONSWING (1<<0)
#define PFLAG_ONTRAIN (1<<1)
#define PFLAG_ONBARNACLE (1<<2)
#define PFLAG_DUCKING (1<<3)
#define PFLAG_USING (1<<4)
#define PFLAG_OBSERVER (1<<5)

#define TRAIN_ACTIVE 0x80
#define TRAIN_NEW 0xc0
#define TRAIN_OFF 0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW 0x02
#define TRAIN_MEDIUM 0x03
#define TRAIN_FAST 0x04
#define TRAIN_BACK 0x05

#define DHF_ROUND_STARTED (1<<1)
#define DHF_HOSTAGE_SEEN_FAR (1<<2)
#define DHF_HOSTAGE_SEEN_NEAR (1<<3)
#define DHF_HOSTAGE_USED (1<<4)
#define DHF_HOSTAGE_INJURED (1<<5)
#define DHF_HOSTAGE_KILLED (1<<6)
#define DHF_FRIEND_SEEN (1<<7)
#define DHF_ENEMY_SEEN (1<<8)
#define DHF_FRIEND_INJURED (1<<9)
#define DHF_FRIEND_KILLED (1<<10)
#define DHF_ENEMY_KILLED (1<<11)
#define DHF_BOMB_RETRIEVED (1<<12)
#define DHF_AMMO_EXHAUSTED (1<<15)
#define DHF_IN_TARGET_ZONE (1<<16)
#define DHF_IN_RESCUE_ZONE (1<<17)
#define DHF_IN_ESCAPE_ZONE (1<<18)
#define DHF_IN_VIPSAFETY_ZONE (1<<19)
#define DHF_NIGHTVISION (1<<20)
#define DHF_HOSTAGE_CTMOVE (1<<21)
#define	DHF_SPEC_DUCK (1<<22)

#define DHM_ROUND_CLEAR (DHF_ROUND_STARTED | DHF_HOSTAGE_KILLED | DHF_FRIEND_KILLED | DHF_BOMB_RETRIEVED)
#define DHM_CONNECT_CLEAR (DHF_HOSTAGE_SEEN_FAR | DHF_HOSTAGE_SEEN_NEAR | DHF_HOSTAGE_USED | DHF_HOSTAGE_INJURED | DHF_FRIEND_SEEN | DHF_ENEMY_SEEN | DHF_FRIEND_INJURED | DHF_ENEMY_KILLED | DHF_AMMO_EXHAUSTED | DHF_IN_TARGET_ZONE | DHF_IN_RESCUE_ZONE | DHF_IN_ESCAPE_ZONE | DHF_IN_VIPSAFETY_ZONE | DHF_HOSTAGE_CTMOVE | DHF_SPEC_DUCK)

#define SIGNAL_BUY (1<<0)
#define SIGNAL_BOMB (1<<1)
#define SIGNAL_RESCUE (1<<2)
#define SIGNAL_ESCAPE (1<<3)
#define SIGNAL_VIPSAFETY (1<<4)

class CUnifiedSignals
{
public:
	CUnifiedSignals(void)
	{
		m_flSignal = 0;
		m_flState = 0;
	}

public:
	void Update(void)
	{
		m_flState = m_flSignal;
		m_flSignal = 0;
	}

	void Signal(int flags) { m_flSignal |= flags; }
	int GetSignal(void) { return m_flSignal; }
	int GetState(void) { return m_flState; }

private:
	int m_flSignal;
	int m_flState;
};

#define IGNOREMSG_NONE 0
#define IGNOREMSG_ENEMY 1
#define IGNOREMSG_TEAM 2

#define CSUITPLAYLIST 4

#define SUIT_GROUP TRUE
#define SUIT_SENTENCE FALSE

#define SUIT_REPEAT_OK 0
#define SUIT_NEXT_IN_30SEC 30
#define SUIT_NEXT_IN_1MIN 60
#define SUIT_NEXT_IN_5MIN 300
#define SUIT_NEXT_IN_10MIN 600
#define SUIT_NEXT_IN_30MIN 1800
#define SUIT_NEXT_IN_1HOUR 3600

#define CSUITNOREPEAT 32

#define SOUND_FLASHLIGHT_ON "items/flashlight1.wav"
#define SOUND_FLASHLIGHT_OFF "items/flashlight1.wav"

#define TEAM_NAME_LENGTH 16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_ATTACK2,
	PLAYER_FLINCH,
	PLAYER_LARGE_FLINCH,
	PLAYER_RELOAD,
	PLAYER_HOLDBOMB
}
PLAYER_ANIM;

typedef enum
{
	Menu_OFF,
	Menu_ChooseTeam,
	Menu_IGChooseTeam,
	Menu_ChooseAppearance,
	Menu_Buy,
	Menu_BuyPistol,
	Menu_BuyRifle,
	Menu_BuyMachineGun,
	Menu_BuyShotgun,
	Menu_BuySubMachineGun,
	Menu_BuyItem,
	Menu_Radio1,
	Menu_Radio2,
	Menu_Radio3,
	Menu_ClientBuy
}
Menu;

typedef enum
{
	MODEL_UNASSIGNED,
	MODEL_URBAN,
	MODEL_TERROR,
	MODEL_LEET,
	MODEL_ARCTIC,
	MODEL_GSG9,
	MODEL_GIGN,
	MODEL_SAS,
	MODEL_GUERILLA,
	MODEL_VIP,
	MODEL_MILITIA,
	MODEL_SPETSNAZ
}
ModelName;

typedef enum
{
	JOINED,
	SHOWLTEXT,
	READINGLTEXT,
	SHOWTEAMSELECT,
	PICKINGTEAM,
	GETINTOGAME
}
JoinState;

typedef struct
{
	int m_primaryWeapon;
	int m_primaryAmmo;
	int m_secondaryWeapon;
	int m_secondaryAmmo;
	int m_heGrenade;
	int m_flashbang;
	int m_smokeGrenade;
	BOOL m_defuser;
	BOOL m_nightVision;
	int m_armor;
}
RebuyStruct;

typedef enum
{
	THROW_NONE,
	THROW_FORWARD,
	THROW_BACKWARD,
	THROW_HITVEL,
	THROW_BOMB,
	THROW_GRENADE,
	THROW_HITVEL_MINUS_AIRVEL
}
ThrowDirection;

#define MAX_ID_RANGE 2048
#define MAX_SPECTATOR_ID_RANGE 8192
#define SBAR_STRING_SIZE 128

#define SBAR_TARGETTYPE_TEAMMATE 1
#define SBAR_TARGETTYPE_ENEMY 2
#define SBAR_TARGETTYPE_HOSTAGE 3

enum sbar_data
{
	SBAR_ID_TARGETTYPE = 1,
	SBAR_ID_TARGETNAME,
	SBAR_ID_TARGETHEALTH,
	SBAR_END
};

typedef enum
{
	SILENT,
	CALM,
	INTENSE
}
MusicState;

#define CHAT_INTERVAL 1.0

class CBasePlayer : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void) { }
	virtual void Restart(void) { }
	virtual int Save(CSave &save) { return 1; }
	virtual int Restore(CRestore &restore) { return 1; }
	virtual int ObjectCaps(void) { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Classify(void) { return 0; }
	virtual void TraceAttack(entvars_t *pevAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType) { }
	virtual int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) { return 0; }
	virtual int TakeHealth(float flHealth, int bitsDamageType) { return 0; }
	virtual void Killed(entvars_t *pevAttacker, int iGib);
	virtual void AddPoints(int score, BOOL bAllowNegativeScore) {}
	virtual void AddPointsToTeam(int score, BOOL bAllowNegativeScore) {}
	virtual BOOL AddPlayerItem(CBasePlayerItem *pItem) { return false; }
	virtual BOOL RemovePlayerItem(CBasePlayerItem *pItem) { return false; }
	virtual int GiveAmmo(int iAmount, char *szName, int iMax){ return 0; }
	virtual void StartSneaking(void) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking(void) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking(void) { return m_tSneaking <= gpGlobals->time; }
	virtual BOOL IsAlive(void) { return pev->deadflag == DEAD_NO && pev->health > 0; }
	virtual BOOL IsPlayer(void) { return TRUE; }
	virtual BOOL IsNetClient(void) { return TRUE; }
	virtual const char *TeamID(void) { return NULL; }
	virtual BOOL FBecomeProne(void) { return TRUE; }
	virtual Vector BodyTarget(const Vector &posSrc) { return Center() + pev->view_ofs * RANDOM_FLOAT(0.5, 1.1); }
	virtual int Illumination(void) { return 0; }
	virtual BOOL ShouldFadeOnDeath(void) { return FALSE; }
	virtual void ResetMaxSpeed(void) {  }
	virtual void Jump(void) { }
	virtual void Duck(void) { }
	virtual void PreThink(void) { }
	virtual void PostThink(void) { }
	virtual Vector GetGunPosition(void);
	virtual BOOL IsBot(void) { return FALSE; }
	virtual void UpdateClientData(void) { }
	virtual void ImpulseCommands(void) { }
	virtual void RoundRespawn(void) { }
	virtual Vector GetAutoaimVector(float flDelta) { return g_vecZero; }
	virtual void Blind(float flUntilTime, float flHoldTime, float flFadeTime, int iAlpha) { }
	virtual void OnTouchingWeapon(CBasePlayerWeapon *pWeapon) {}

public:
	void Pain(int hitgroup, bool hitkevlar);
	void RenewItems(void);
	void PackDeadPlayerItems(void);
	void RemoveAllItems(BOOL removeSuit);
	void SwitchTeam(void);
	BOOL SwitchWeapon(CBasePlayerItem *pWeapon);
	BOOL IsOnLadder(void);
	BOOL FlashlightIsOn(void);
	void FlashlightTurnOn(void);
	void FlashlightTurnOff(void);
	void UpdatePlayerSound(void);
	void DeathSound(void);
	void SetAnimation(PLAYER_ANIM playerAnim);
	void SetWeaponAnimType(const char *szExtention);
	void CheatImpulseCommands(int iImpulse);
	void StartDeathCam(void);
	void StartObserver(const Vector &vecPosition, const Vector &vecViewAngle);
	CBaseEntity *Observer_IsValidTarget(int iTarget, bool bOnlyTeam);
	void Observer_FindNextPlayer(bool bReverse, char *name = NULL);
	void Observer_HandleButtons(void);
	void Observer_SetMode(int iMode);
	void Observer_CheckTarget(void);
	void Observer_CheckProperties(void);
	int IsObserver(void) { return pev->iuser1; }
	bool IsObservingPlayer(CBasePlayer *pTarget);
	void SetObserverAutoDirector(bool bState);
	BOOL CanSwitchObserverModes(void);
	void DropPlayerItem(const char *pszItemName);
	void ThrowPrimary(void);
	void ThrowWeapon(char *pszWeaponName);
	BOOL HasPlayerItem(CBasePlayerItem *pCheckItem);
	BOOL HasNamedPlayerItem(const char *pszItemName);
	BOOL HasWeapons(void);
	void SelectPrevItem(int iItem);
	void SelectNextItem(int iItem);
	void SelectLastItem(void);
	void SelectItem(const char *pstr);
	void ItemPreFrame(void);
	void ItemPostFrame(void);
	void GiveNamedItem(const char *szName);
	void EnableControl(BOOL fControl);
	void SendAmmoUpdate(void);
	void SendFOV(int iFOV);
	void SendHostagePos(void);
	void SendHostageIcons(void);
	void SendWeatherInfo(void);
	void WaterMove(void);
	void EXPORT PlayerDeathThink(void);
	void PlayerUse(void);
	void CheckSuitUpdate(void);
	void SetSuitUpdate(const char *name, int fgroup, int iNoRepeat);
	void UpdateGeigerCounter(void);
	void CheckTimeBasedDamage(void);
	void BarnacleVictimBitten(entvars_t *pevBarnacle);
	void BarnacleVictimReleased(void);
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory(int iAmmoIndex);
	void ResetAutoaim(void);
	Vector AutoaimDeflection(Vector &vecSrc, float flDist, float flDelta);
	void ForceClientDllUpdate(void);
	void SetCustomDecalFrames(int nFrames);
	int GetCustomDecalFrames(void);
	void TabulateAmmo(void);
	void SetProgressBarTime(int iTime);
	void SetProgressBarTime2(int iTime, float flLastTime);
	void SetPlayerModel(BOOL HasC4);
	void SetNewPlayerModel(const char *model);
	void CheckPowerups(entvars_t *pev);
	void SmartRadio(void);
	void Radio(const char *msg_id, const char *msg_verbose, int pitch = 100, bool showIcon = true);
	void GiveDefaultItems(void);
	void SetBombIcon(BOOL bFlash);
	void SetScoreAttrib(CBasePlayer *dest);
	void SetScoreboardAttributes(CBasePlayer *pPlayer = NULL);
	BOOL IsBombGuy(void);
	BOOL ShouldDoLargeFlinch(int nHitGroup, int nGunType);
	BOOL IsArmored(int nHitGroup);
	bool HintMessage(const char *pMessage, BOOL bDisplayIfDead = FALSE, BOOL bOverrideClientSettings = FALSE);
	void AddAccount(int amount, bool bTrackChange = true);
	void SyncRoundTimer(void);
	void MenuPrint(const char *text);
	void ResetMenu(void);
	void MakeVIP(void);
	void JoiningThink(void);
	void ResetStamina(void);
	void Disappear(void);
	void RemoveLevelText(void);
	void MoveToNextIntroCamera(void);
	void SpawnClientSideCorpse(void);
	void SetPrefsFromUserinfo(char *infobuffer);
	void HostageUsed(void);
	bool CanPlayerBuy(bool display);
	void StudioEstimateGait(void);
	void CalculatePitchBlend(void);
	void CalculateYawBlend(void);
	void StudioProcessGait(void);
	void HandleSignals(void);
	void EnterEscapeZone(void);
	void LeaveEscapeZone(void);
	void EnterVIPSafetyZone(void);
	void LeaveVIPSafetyZone(void);
	void InitStatusBar(void);
	void UpdateStatusBar(void);
	bool IsHittingShield(const Vector &vecDirection, TraceResult *ptr);
	bool IsReloading(void);
	bool IsThrowingGrenade(void);
	void StopReload(void);
	void DrawnShiled(void);
	bool HasShield(void);
	void UpdateShieldCrosshair(bool bShieldDrawn);
	void DropShield(bool bDeploy);
	void GiveShield(bool bRetire);
	bool IsProtectedByShield(void);
	void RemoveShield(void);
	void UpdateLocation(bool bForceUpdate);
	void ClientCommand(const char *arg0, const char *arg1 = NULL, const char *arg2 = NULL, const char *arg3 = NULL);
	void ClearAutoBuyData(void);
	void AddAutoBuyData(const char *string);
	void InitRebuyData(const char *string);
	void AutoBuy(void);
	bool ShouldExecuteAutoBuyCommand(const AutoBuyInfoStruct *commandInfo, bool boughtPrimary, bool boughtSecondary);
	AutoBuyInfoStruct *GetAutoBuyCommandInfo(const char *command);
	void PrioritizeAutoBuyString(char *autobuyString, const char *priorityString);
	void ParseAutoBuyString(const char *string, bool &boughtPrimary, bool &boughtSecondary);
	void PostAutoBuyCommandProcessing(const AutoBuyInfoStruct *commandInfo, bool &boughtPrimary, bool &boughtSecondary);
	void BuildRebuyStruct(void);
	void Rebuy(void);
	void RebuyPrimaryWeapon(void);
	void RebuyPrimaryAmmo(void);
	void RebuySecondaryWeapon(void);
	void RebuySecondaryAmmo(void);
	void RebuyHEGrenade(void);
	void RebuyFlashbang(void);
	void RebuySmokeGrenade(void);
	void RebuyDefuser(void);
	void RebuyNightVision(void);
	void RebuyArmor(void);

public:
	static TYPEDESCRIPTION m_playerSaveData[];

public:
	int random_seed;
	unsigned short m_usPlayerBleed;
	EHANDLE m_hObserverTarget;
	float m_flNextObserverInput;
	int m_iObserverWeapon;
	int m_iObserverC4State;
	bool m_bObserverHasDefuser;
	int m_iObserverLastMode;
	float m_flFlinchTime;
	float m_flAnimTime;
	bool m_bHighDamage;
	float m_flVelocityModifier;
	int m_iLastZoom;
	bool m_bResumeZoom;
	float m_flEjectBrass;
	int m_iKevlar;
	bool m_bNotKilled;
	int m_iTeam;
	int m_iAccount;
	bool m_bHasPrimary;
	float m_flDeathThrowTime;
	int m_iThrowDirection;
	float m_flLastTalk;
	bool m_bJustConnected;
	bool m_bContextHelp;
	JoinState m_iJoiningState;
	CBaseEntity *m_pIntroCamera;
	float m_fIntroCamTime;
	float m_fLastMovement;
	bool m_bMissionBriefing;
	bool m_bTeamChanged;
	int m_iModelName;
	int m_iTeamKills;
	int m_iIgnoreGlobalChat;
	bool m_bHasNightVision;
	bool m_bNightVisionOn;
	Vector m_vRecentPath[20];
	float m_flIdleCheckTime;
	float m_flRadioTime;
	int m_iRadioMessages;
	bool m_bIgnoreRadio;
	bool m_bHasC4;
	bool m_bHasDefuser;
	bool m_bKilledByBomb;
	Vector m_vBlastVector;
	bool m_bKilledByGrenade;
	CHintMessageQueue m_hintMessageQueue;
	int m_flDisplayHistory;
	int m_iMenu;
	int m_iChaseTarget;
	CBaseEntity *m_pChaseTarget;
	BOOL m_fCamSwitch;
	bool m_bEscaped;
	bool m_bIsVIP;
	float m_tmNextRadarUpdate;
	Vector m_vLastOrigin;
	int m_iCurrentKickVote;
	float m_flNextVoteTime;
	bool m_bJustKilledTeammate;
	int m_iHostagesKilled;
	int m_iMapVote;
	bool m_bCanShoot;
	float m_flLastFired;
	float m_flLastAttackedTeammate;
	bool m_bHeadshotKilled;
	bool m_bPunishedForTK;
	bool m_bReceivesNoMoneyNextRound;
	int m_iTimeCheckAllowed;
	bool m_bHasChangedName;
	char m_szNewName[MAX_PLAYER_NAME_LENGTH];
	bool m_bIsDefusing;
	float m_tmHandleSignals;
	CUnifiedSignals m_signals;
	edict_t *m_pentCurBombTarget;
	int m_iPlayerSound;
	int m_iTargetVolume;
	int m_iWeaponVolume;
	int m_iExtraSoundTypes;
	int m_iWeaponFlash;
	float m_flStopExtraSoundTime;
	float m_flFlashLightTime;
	int m_iFlashBattery;
	int m_afButtonLast;
	int m_afButtonPressed;
	int m_afButtonReleased;
	edict_t *m_pentSndLast;
	float m_flSndRoomtype;
	float m_flSndRange;
	float m_flFallVelocity;
	int m_rgItems[MAX_ITEMS];
	int m_fNewAmmo;
	unsigned int m_afPhysicsFlags;
	float m_fNextSuicideTime;
	float m_flTimeStepSound;
	float m_flTimeWeaponIdle;
	float m_flSwimTime;
	float m_flDuckTime;
	float m_flWallJumpTime;
	float m_flSuitUpdate;
	int m_rgSuitPlayList[CSUITPLAYLIST];
	int m_iSuitPlayNext;
	int m_rgiSuitNoRepeat[CSUITNOREPEAT];
	float m_rgflSuitNoRepeatTime[CSUITNOREPEAT];
	int m_lastDamageAmount;
	float m_tbdPrev;
	float m_flgeigerRange;
	float m_flgeigerDelay;
	int m_igeigerRangePrev;
	int m_iStepLeft;
	char m_szTextureName[CBTEXTURENAMEMAX];
	char m_chTextureType;
	int m_idrowndmg;
	int m_idrownrestored;
	int m_bitsHUDDamage;
	BOOL m_fInitHUD;
	BOOL m_fGameHUDInitialized;
	int m_iTrain;
	BOOL m_fWeapon;
	EHANDLE m_pTank;
	float m_fDeadTime;
	BOOL m_fNoPlayerSound;
	BOOL m_fLongJump;
	float m_tSneaking;
	int m_iUpdateTime;
	int m_iClientHealth;
	int m_iClientBattery;
	int m_iHideHUD;
	int m_iClientHideHUD;
	int m_iFOV;
	int m_iClientFOV;
	int m_iNumSpawns;
	CBaseEntity *m_pObserver;
	CBasePlayerItem *m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;
	CBasePlayerItem *m_pLastItem;
	int m_rgAmmo[MAX_AMMO_SLOTS];
	int m_rgAmmoLast[MAX_AMMO_SLOTS];
	Vector m_vecAutoAim;
	BOOL m_fOnTarget;
	int m_iDeaths;
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	int m_lastx, m_lasty;
	int m_nCustomSprayFrames;
	float m_flNextDecalTime;
	char m_szTeamName[TEAM_NAME_LENGTH];
	int m_modelIndexPlayer;
	char m_szAnimExtention[32];
	int m_iGaitsequence;
	float m_flGaitframe;
	float m_flGaityaw;
	vec3_t m_prevgaitorigin;
	float m_flPitch;
	float m_flYaw;
	float m_flGaitMovement;
	int m_iAutoWepSwitch;
	bool m_bVGUIMenus;
	bool m_bShowHints;
	bool m_bShieldDrawn;
	bool m_bOwnsShield;
	bool m_bWasFollowing;
	float m_flNextFollowTime;
	float m_flYawModifier;
	float m_blindUntilTime;
	float m_blindStartTime;
	float m_blindHoldTime;
	float m_blindFadeTime;
	float m_blindAlpha;
	float m_allowAutoFollowTime;
	char m_autoBuyString[MAX_AUTOBUY_LENGTH];
	char *m_rebuyString;
	RebuyStruct m_rebuyStruct;
	bool m_bIsInRebuy;
	float m_flLastUpdateTime;
	char m_lastLocation[MAX_LACTION_LENGTH];
	int m_progressStart;
	int m_progressEnd;
	bool m_bObserverAutoDirector;
	bool m_canSwitchObserverModes;
	float m_heartBeatTime;
	int m_intenseTimestamp;
	int m_silentTimestamp;
	MusicState m_musicState;
	int m_flLastCommandTime[8];
};

#define AUTOAIM_2DEGREES 0.0348994967025
#define AUTOAIM_5DEGREES 0.08715574274766
#define AUTOAIM_8DEGREES 0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int gmsgHudText;
extern int gmsgShowMenu;
extern int gmsgVGUIMenu;
extern int gmsgScenarioIcon;
extern int gmsgBombDrop;

extern BOOL gInitHUD;

void SendItemStatus(CBasePlayer *pPlayer);

extern bool UseBotArgs;
extern const char *BotArgs[4];

#endif
