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

class CBasePlayerItem;
class CBasePlayer;
class CItem;
class CBasePlayerAmmo;

#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536

#define STARTMONEY_MAX 32000
#define STARTMONEY_MIN 800
enum
{
	GR_NONE = 0,
	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,
	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,
	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,
	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,
	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO
};

enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL
};

class CGameRules
{
public:
	virtual void RefreshSkillData(void);
	virtual void Think(void) = 0;
	virtual BOOL IsAllowedToSpawn(CBaseEntity *pEntity) = 0;
	virtual BOOL FAllowFlashlight(void) = 0;
	virtual BOOL FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon) = 0;
	virtual BOOL GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon) = 0;
	virtual BOOL IsMultiplayer(void) = 0;
	virtual BOOL IsDeathmatch(void) = 0;
	virtual BOOL IsTeamplay(void) { return FALSE; }
	virtual BOOL IsCoOp(void) = 0;
	virtual const char *GetGameDescription(void) { return "Counter-Strike"; }
	virtual BOOL ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]) = 0;
	virtual void InitHUD(CBasePlayer *pl) = 0;
	virtual void ClientDisconnected(edict_t *pClient) = 0;
	virtual void UpdateGameMode(CBasePlayer *pPlayer) {}
	virtual float FlPlayerFallDamage(CBasePlayer *pPlayer) = 0;
	virtual BOOL FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker) { return TRUE; }
	virtual BOOL ShouldAutoAim(CBasePlayer *pPlayer, edict_t *target) { return TRUE; }
	virtual void PlayerSpawn(CBasePlayer *pPlayer) = 0;
	virtual void PlayerThink(CBasePlayer *pPlayer) = 0;
	virtual BOOL FPlayerCanRespawn(CBasePlayer *pPlayer) = 0;
	virtual float FlPlayerSpawnTime(CBasePlayer *pPlayer) = 0;
	virtual edict_t *GetPlayerSpawnSpot(CBasePlayer *pPlayer);
	virtual BOOL AllowAutoTargetCrosshair(void) { return TRUE; }
	virtual BOOL ClientCommand_DeadOrAlive(CBasePlayer *pPlayer, const char *pcmd) { return FALSE; }
	virtual BOOL ClientCommand(CBasePlayer *pPlayer, const char *pcmd) { return FALSE; }
	virtual void ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer) {}
	virtual int IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled) = 0;
	virtual void PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor) = 0;
	virtual void DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor) = 0;
	virtual BOOL CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	virtual void PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon) = 0;
	virtual int WeaponShouldRespawn(CBasePlayerItem *pWeapon) = 0;
	virtual float FlWeaponRespawnTime(CBasePlayerItem *pWeapon) = 0;
	virtual float FlWeaponTryRespawn(CBasePlayerItem *pWeapon) = 0;
	virtual Vector VecWeaponRespawnSpot(CBasePlayerItem *pWeapon) = 0;
	virtual BOOL CanHaveItem(CBasePlayer *pPlayer, CItem *pItem) = 0;
	virtual void PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem) = 0;
	virtual int ItemShouldRespawn(CItem *pItem) = 0;
	virtual float FlItemRespawnTime(CItem *pItem) = 0;
	virtual Vector VecItemRespawnSpot(CItem *pItem) = 0;
	virtual BOOL CanHaveAmmo(CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry);
	virtual void PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount) = 0;
	virtual int AmmoShouldRespawn(CBasePlayerAmmo *pAmmo) = 0;
	virtual float FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo) = 0;
	virtual Vector VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo) = 0;
	virtual float FlHealthChargerRechargeTime(void) = 0;
	virtual float FlHEVChargerRechargeTime(void) { return 0; }
	virtual int DeadPlayerWeapons(CBasePlayer *pPlayer) = 0;
	virtual int DeadPlayerAmmo(CBasePlayer *pPlayer) = 0;
	virtual const char *GetTeamID(CBaseEntity *pEntity) = 0;
	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget) = 0;
	virtual int GetTeamIndex(const char *pTeamName) { return -1; }
	virtual const char *GetIndexedTeamName(int teamIndex) { return ""; }
	virtual BOOL IsValidTeam(const char *pTeamName) { return TRUE; }
	virtual void ChangePlayerTeam(CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib) {}
	virtual const char *SetDefaultPlayerTeam(CBasePlayer *pPlayer) { return ""; }
	virtual BOOL PlayTextureSounds(void) { return TRUE; }
	virtual BOOL FAllowMonsters(void) = 0;
	virtual void EndMultiplayerGame(void) {}
	virtual BOOL IsFreezePeriod(void) { return m_bFreezePeriod; }
	virtual void ServerDeactivate(void) {}
	virtual void CheckMapConditions(void) {}

public:
	BOOL m_bFreezePeriod;
	BOOL m_bBombDropped;
};

extern char *GetTeam(int teamNo);
extern void Broadcast(const char *sentence, int pitch = 100 );
extern CGameRules *InstallGameRules(void);

class CHalfLifeRules : public CGameRules
{
public:
	CHalfLifeRules(void);

public:
	virtual void Think(void);
	virtual BOOL IsAllowedToSpawn(CBaseEntity *pEntity);
	virtual BOOL FAllowFlashlight(void) { return TRUE; }
	virtual BOOL FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	virtual BOOL GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon);
	virtual BOOL IsMultiplayer(void);
	virtual BOOL IsDeathmatch(void);
	virtual BOOL IsCoOp(void);
	virtual BOOL ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
	virtual void InitHUD(CBasePlayer *pl);
	virtual void ClientDisconnected(edict_t *pClient);
	virtual float FlPlayerFallDamage(CBasePlayer *pPlayer);
	virtual void PlayerSpawn(CBasePlayer *pPlayer);
	virtual void PlayerThink(CBasePlayer *pPlayer);
	virtual BOOL FPlayerCanRespawn(CBasePlayer *pPlayer);
	virtual float FlPlayerSpawnTime(CBasePlayer *pPlayer);
	virtual edict_t *GetPlayerSpawnSpot(CBasePlayer *pPlayer);
	virtual BOOL AllowAutoTargetCrosshair(void);
	virtual int IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled);
	virtual void PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor);
	virtual void DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor);
	virtual void PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	virtual int WeaponShouldRespawn(CBasePlayerItem *pWeapon);
	virtual float FlWeaponRespawnTime(CBasePlayerItem *pWeapon);
	virtual float FlWeaponTryRespawn(CBasePlayerItem *pWeapon);
	virtual Vector VecWeaponRespawnSpot(CBasePlayerItem *pWeapon);
	virtual BOOL CanHaveItem(CBasePlayer *pPlayer, CItem *pItem);
	virtual void PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem);
	virtual int ItemShouldRespawn(CItem *pItem);
	virtual float FlItemRespawnTime(CItem *pItem);
	virtual Vector VecItemRespawnSpot(CItem *pItem);
	virtual void PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount);
	virtual int AmmoShouldRespawn(CBasePlayerAmmo *pAmmo);
	virtual float FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo);
	virtual Vector VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo);
	virtual float FlHealthChargerRechargeTime(void);
	virtual int DeadPlayerWeapons(CBasePlayer *pPlayer);
	virtual int DeadPlayerAmmo(CBasePlayer *pPlayer);
	virtual BOOL FAllowMonsters(void);
	virtual const char *GetTeamID(CBaseEntity *pEntity) { return ""; }
	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
};

#define MAX_MAPS 100
#define MAX_VIPQUEUES 5

enum
{
	WINSTATUS_CT = 1,
	WINSTATUS_TERRORIST,
	WINSTATUS_DRAW
};

#define Target_Bombed 1
#define VIP_Escaped 2
#define VIP_Assassinated 3
#define Terrorists_Escaped 4
#define CTs_PreventEscape 5
#define Escaping_Terrorists_Neutralized 6
#define Bomb_Defused 7
#define CTs_Win 8
#define Terrorists_Win 9
#define Round_Draw 10
#define All_Hostages_Rescued 11
#define Target_Saved 12
#define Hostages_Not_Rescued 13
#define Terrorists_Not_Escaped 14
#define VIP_Not_Escaped 15
#define Game_Commencing 16

#include "voice_gamemgr.h"

class CHalfLifeMultiplay : public CGameRules
{
public:
	CHalfLifeMultiplay(void);

public:
	virtual void Think(void);
	virtual void RefreshSkillData(void);
	virtual BOOL IsAllowedToSpawn(CBaseEntity *pEntity);
	virtual BOOL FAllowFlashlight(void);
	virtual BOOL FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	virtual BOOL GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon);
	virtual BOOL IsMultiplayer(void);
	virtual BOOL IsDeathmatch(void);
	virtual BOOL IsCoOp(void);
	virtual BOOL ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
	virtual void InitHUD(CBasePlayer *pl);
	virtual void ClientDisconnected(edict_t *pClient);
	virtual void UpdateGameMode(CBasePlayer *pPlayer);
	virtual float FlPlayerFallDamage(CBasePlayer *pPlayer);
	virtual BOOL FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker);
	virtual void PlayerSpawn(CBasePlayer *pPlayer);
	virtual void PlayerThink(CBasePlayer *pPlayer);
	virtual BOOL FPlayerCanRespawn(CBasePlayer *pPlayer);
	virtual float FlPlayerSpawnTime(CBasePlayer *pPlayer);
	virtual edict_t *GetPlayerSpawnSpot(CBasePlayer *pPlayer);
	virtual BOOL AllowAutoTargetCrosshair(void);
	virtual BOOL ClientCommand_DeadOrAlive(CBasePlayer *pPlayer, const char *pcmd);
	virtual BOOL ClientCommand(CBasePlayer *pPlayer, const char *pcmd);
	virtual void ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer);
	virtual int IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled);
	virtual void PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor);
	virtual void DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor);
	virtual BOOL CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	virtual void PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	virtual int WeaponShouldRespawn(CBasePlayerItem *pWeapon);
	virtual float FlWeaponRespawnTime(CBasePlayerItem *pWeapon);
	virtual float FlWeaponTryRespawn(CBasePlayerItem *pWeapon);
	virtual Vector VecWeaponRespawnSpot(CBasePlayerItem *pWeapon);
	virtual BOOL CanHaveItem(CBasePlayer *pPlayer, CItem *pItem);
	virtual void PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem);
	virtual int ItemShouldRespawn(CItem *pItem);
	virtual float FlItemRespawnTime(CItem *pItem);
	virtual Vector VecItemRespawnSpot(CItem *pItem);
	virtual void PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount);
	virtual int AmmoShouldRespawn(CBasePlayerAmmo *pAmmo);
	virtual float FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo);
	virtual Vector VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo);
	virtual float FlHealthChargerRechargeTime(void);
	virtual float FlHEVChargerRechargeTime(void);
	virtual int DeadPlayerWeapons(CBasePlayer *pPlayer);
	virtual int DeadPlayerAmmo(CBasePlayer *pPlayer);
	virtual const char *GetTeamID(CBaseEntity *pEntity) { return ""; }
	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
	virtual BOOL PlayTextureSounds(void) { return FALSE; }
	virtual BOOL FAllowMonsters(void);
	virtual void EndMultiplayerGame(void) { GoToIntermission(); }
	virtual void CheckMapConditions(void);
	virtual void ServerDeactivate(void);
	virtual void CleanUpMap(void);
	virtual void RestartRound(void);
	virtual void CheckWinConditions(void);
	virtual void RemoveGuns(void);
	virtual void GiveC4(void);
	virtual void ChangeLevel(void);
	virtual void GoToIntermission(void);
	virtual void SetRestartServerAtRoundEnd();
	virtual BOOL ShouldRestart();
public:
	void SendMOTDToClient(edict_t *client);
	void InitializePlayerCounts(int &NumAliveTerrorist, int &NumAliveCT, int &NumDeadTerrorist, int &NumDeadCT);
	BOOL NeededPlayersCheck(BOOL &bNeededPlayers);
	BOOL VIPRoundEndCheck(BOOL bNeededPlayers);
	BOOL PrisonRoundEndCheck(int NumAliveTerrorist, int NumAliveCT, int NumDeadTerrorist, int NumDeadCT, BOOL bNeededPlayers);
	BOOL BombRoundEndCheck(BOOL bNeededPlayers);
	BOOL TeamExterminationCheck(int NumAliveTerrorist, int NumAliveCT, int NumDeadTerrorist, int NumDeadCT, BOOL bNeededPlayers);
	BOOL HostageRescueRoundEndCheck(BOOL bNeededPlayers);
	void BalanceTeams(void);
	BOOL IsThereABomber(void);
	BOOL IsThereABomb(void);
	BOOL TeamFull(int team_id);
	BOOL TeamStacked(int newTeam_id, int curTeam_id);
	void StackVIPQueue(void);
	void CheckVIPQueue(void);
	BOOL IsVIPQueueEmpty(void);
	BOOL AddToVIPQueue(CBasePlayer *pPlayer);
	void ResetCurrentVIP(void);
	void PickNextVIP(void);
	void CheckFreezePeriodExpired(void);
	void CheckRoundTimeExpired(void);
	void CheckLevelInitialized(void);
	void CheckRestartRound(void);
	BOOL CheckTimeLimit(void);
	BOOL CheckMaxRounds(void);
	BOOL CheckGameOver(void);
	BOOL CheckWinLimit(void);
	void CheckAllowSpecator(void);
	void CheckGameCvar(void);
	void DisplayMaps(CBasePlayer *pPlayer, int mapId);
	void ResetAllMapVotes(void);
	void ProcessMapVote(CBasePlayer *pPlayer, int mapId);
	void UpdateTeamScores(void);
	void SwapAllPlayers(void);
	void TerminateRound(float tmDelay, int iWinStatus);
	void QueueCareerRoundEndMenu(float tmDelay, int iWinStatus);
	float TimeRemaining(void) { return m_iRoundTimeSecs - gpGlobals->time + m_fRoundCount; }
	BOOL HasRoundTimeExpired(void);
	BOOL IsBombPlanted(void);
	void MarkLivingPlayersOnTeamAsNotReceivingMoneyNextRound(int team);
	void CareerRestart(void);
	BOOL IsCareer(void) { return FALSE; }

public:
	CVoiceGameMgr m_VoiceGameMgr;
	float m_fTeamCount;
	float m_flCheckWinConditions;
	float m_fRoundCount;
	int m_iRoundTime;
	int m_iRoundTimeSecs;
	int m_iIntroRoundTime;
	float m_fIntroRoundCount;
	int m_iAccountTerrorist;
	int m_iAccountCT;
	int m_iNumTerrorist;
	int m_iNumCT;
	int m_iNumSpawnableTerrorist;
	int m_iNumSpawnableCT;
	int m_iSpawnPointCount_Terrorist;
	int m_iSpawnPointCount_CT;
	int m_iHostagesRescued;
	int m_iHostagesTouched;
	int m_iRoundWinStatus;
	short m_iNumCTWins;
	short m_iNumTerroristWins;
	bool m_bTargetBombed;
	bool m_bBombDefused;
	bool m_bMapHasBombTarget;
	bool m_bMapHasBombZone;
	bool m_bMapHasBuyZone;
	bool m_bMapHasRescueZone;
	bool m_bMapHasEscapeZone;
	int m_iMapHasVIPSafetyZone;
	int m_bMapHasCameras;
	int m_iC4Timer;
	int m_iC4Guy;
	int m_iLoserBonus;
	int m_iNumConsecutiveCTLoses;
	int m_iNumConsecutiveTerroristLoses;
	float m_fMaxIdlePeriod;
	int m_iLimitTeams;
	bool m_bLevelInitialized;
	bool m_bRoundTerminating;
	bool m_bCompleteReset;
	float m_flRequiredEscapeRatio;
	int m_iNumEscapers;
	int m_iHaveEscaped;
	bool m_bCTCantBuy;
	bool m_bTCantBuy;
	float m_flBombRadius;
	int m_iConsecutiveVIP;
	int m_iTotalGunCount;
	int m_iTotalGrenadeCount;
	int m_iTotalArmourCount;
	int m_iUnBalancedRounds;
	int m_iNumEscapeRounds;
	int m_iMapVotes[MAX_MAPS];
	int m_iLastPick;
	int m_iMaxMapTime;
	int m_iMaxRounds;
	int m_iTotalRoundsPlayed;
	int m_iMaxRoundsWon;
	int m_iStoredSpectValue;
	float m_flForceCameraValue;
	float m_flForceChaseCamValue;
	float m_flFadeToBlackValue;
	CBasePlayer *m_pVIP;
	CBasePlayer *VIPQueue[MAX_VIPQUEUES];
	float m_flIntermissionEndTime;
	float m_flIntermissionStartTime;
	int m_iEndIntermissionButtonHit;
	float m_tmNextPeriodicThink;
	bool m_bFirstConnected;
	bool m_bInCareerGame;
	float m_fCareerRoundMenuTime;
	int m_iCareerMatchWins;
	int m_iRoundWinDifference;
	float m_fCareerMatchMenuTime;
	bool m_bSkipSpawn;
	BOOL m_bShouldRestart;
};

extern DLL_GLOBAL CHalfLifeMultiplay *g_pGameRules;
