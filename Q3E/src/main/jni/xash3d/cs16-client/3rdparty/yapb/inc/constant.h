//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// forwards
class Bot;
class BotGraph;
class BotManager;

// defines bots tasks
CR_DECLARE_SCOPED_ENUM (Task,
   Normal = 0,
   Pause,
   MoveToPosition,
   FollowUser,
   PickupItem,
   Camp,
   PlantBomb,
   DefuseBomb,
   Attack,
   Hunt,
   SeekCover,
   ThrowExplosive,
   ThrowFlashbang,
   ThrowSmoke,
   DoubleJump,
   EscapeFromBomb,
   ShootBreakable,
   Hide,
   Blind,
   Spraypaint,
   Max
)

// bot menu ids
CR_DECLARE_SCOPED_ENUM (Menu,
   None = 0,
   Main,
   Features,
   Control,
   WeaponMode,
   Personality,
   Difficulty,
   TeamSelect,
   TerroristSelect,
   CTSelect,
   TerroristSelectCZ,
   CTSelectCZ,
   Commands,
   NodeMainPage1,
   NodeMainPage2,
   NodeRadius,
   NodeType,
   NodeFlag,
   NodeAutoPath,
   NodePath,
   NodeDebug,
   CampDirections,
   KickPage1,
   KickPage2,
   KickPage3,
   KickPage4
)

// bomb say string
CR_DECLARE_SCOPED_ENUM (BombPlantedSay,
  ChatSay = cr::bit (1),
  Chatter = cr::bit (2)
)

// chat types id's
CR_DECLARE_SCOPED_ENUM (Chat,
   Kill = 0, // id to kill chat array
   Dead, // id to dead chat array
   Plant, // id to bomb chat array
   TeamAttack, // id to team-attack chat array
   TeamKill, // id to team-kill chat array
   Hello, // id to welcome chat array
   NoKeyword, // id to no keyword chat array
   Count // number for array
)

// personalities defines
CR_DECLARE_SCOPED_ENUM (Personality,
   Normal = 0,
   Rusher,
   Careful,
   Invalid = -1
)

// bot difficulties
CR_DECLARE_SCOPED_ENUM (Difficulty,
   Noob,
   Easy,
   Normal,
   Hard,
   Expert,
   Invalid = -1
)

// collision states
CR_DECLARE_SCOPED_ENUM (CollisionState,
   Undecided,
   Probing,
   NoMove,
   Jump,
   Duck,
   StrafeLeft,
   StrafeRight
)

// counter-strike team id's
CR_DECLARE_SCOPED_ENUM (Team,
   Terrorist = 0,
   CT,
   Spectator,
   Unassigned,
   Invalid = -1
)

// item status for StatusIcon message
CR_DECLARE_SCOPED_ENUM (ItemStatus,
   Nightvision = cr::bit (0),
   DefusalKit = cr::bit (1)
)

// client flags
CR_DECLARE_SCOPED_ENUM (ClientFlags,
   Used = cr::bit (0),
   Alive = cr::bit (1),
   Admin = cr::bit (2),
   Icon = cr::bit (3)
)

// bot create status
CR_DECLARE_SCOPED_ENUM (BotCreateResult,
   Success,
   MaxPlayersReached,
   GraphError,
   TeamStacked
)

// radio messages
CR_DECLARE_SCOPED_ENUM (Radio,
   CoverMe = 1,
   YouTakeThePoint = 2,
   HoldThisPosition = 3,
   RegroupTeam = 4,
   FollowMe = 5,
   TakingFireNeedAssistance = 6,
   GoGoGo = 11,
   TeamFallback = 12,
   StickTogetherTeam = 13,
   GetInPositionAndWaitForGo = 14,
   StormTheFront = 15,
   ReportInTeam = 16,
   RogerThat = 21,
   EnemySpotted = 22,
   NeedBackup = 23,
   SectorClear = 24,
   ImInPosition = 25,
   ReportingIn = 26,
   ShesGonnaBlow = 27,
   Negative = 28,
   EnemyDown = 29
)

// chatter system (extending enum above, messages 30-39 is reserved)
CR_DECLARE_SCOPED_ENUM (Chatter,
   SpotTheBomber = 40,
   FriendlyFire,
   DiePain,
   Blind,
   GoingToPlantBomb,
   RescuingHostages,
   GoingToCamp,
   TeamAttack,
   TeamKill,
   ReportingIn,
   GuardingPlantedC4,
   Camping,
   PlantingBomb,
   DefusingBomb,
   InCombat,
   SeekingEnemies,
   Nothing,
   EnemyDown,
   UsingHostages,
   FoundC4,
   WonTheRound,
   ScaredEmotion,
   HeardTheEnemy,
   SpottedOneEnemy,
   SpottedTwoEnemies,
   SpottedThreeEnemies,
   TooManyEnemies,
   SniperWarning,
   SniperKilled,
   VIPSpotted,
   GuardingEscapeZone,
   GuardingVIPSafety,
   GoingToGuardEscapeZone,
   GoingToGuardRescueZone,
   GoingToGuardVIPSafety,
   QuickWonRound,
   OneEnemyLeft,
   TwoEnemiesLeft,
   ThreeEnemiesLeft,
   NoEnemiesLeft,
   FoundC4Plant,
   WhereIsTheC4,
   DefendingBombsite,
   BarelyDefused,
   NiceShotCommander,
   NiceShotPall,
   GoingToGuardHostages,
   GoingToGuardDroppedC4,
   OnMyWay,
   LeadOnSir,
   PinnedDown,
   GottaFindC4,
   YouHeardTheMan,
   LostCommander,
   NewRound,
   CoverMe,
   BehindSmoke,
   BombsiteSecured,
   OnARoll,
   Count
)

// counter strike weapon classes (types)
CR_DECLARE_SCOPED_ENUM (WeaponType,
   None,
   Melee,
   Pistol,
   Shotgun,
   ZoomRifle,
   Rifle,
   SMG,
   Sniper,
   Heavy
)

// counter-strike weapon id's
CR_DECLARE_SCOPED_ENUM (Weapon,
   P228 = 1,
   Shield = 2,
   Scout = 3,
   Explosive = 4,
   XM1014 = 5,
   C4 = 6,
   MAC10 = 7,
   AUG = 8,
   Smoke = 9,
   Elite = 10,
   FiveSeven = 11,
   UMP45 = 12,
   SG550 = 13,
   Galil = 14,
   Famas = 15,
   USP = 16,
   Glock18 = 17,
   AWP = 18,
   MP5 = 19,
   M249 = 20,
   M3 = 21,
   M4A1 = 22,
   TMP = 23,
   G3SG1 = 24,
   Flashbang = 25,
   Deagle = 26,
   SG552 = 27,
   AK47 = 28,
   Knife = 29,
   P90 = 30,
   Armor = 31,
   ArmorHelm = 32,
   Defuser = 33
)

// buy counts
CR_DECLARE_SCOPED_ENUM (BuyState,
   PrimaryWeapon = 0,
   ArmorVestHelm,
   SecondaryWeapon,
   Ammo,
   DefusalKit,
   Grenades,
   NightVision,
   Done
)

// economics limits
CR_DECLARE_SCOPED_ENUM (EcoLimit,
   PrimaryGreater = 0,
   SmgCTGreater,
   SmgTEGreater,
   ShotgunGreater,
   ShotgunLess,
   HeavyGreater,
   HeavyLess,
   ProstockNormal,
   ProstockRusher,
   ProstockCareful,
   ShieldGreater
)

// defines for pickup items
CR_DECLARE_SCOPED_ENUM (Pickup,
   None = 0,
   Weapon,
   DroppedC4,
   PlantedC4,
   Hostage,
   Button,
   Shield,
   DefusalKit,
   Items,
   AmmoAndKits
)

// fight style type
CR_DECLARE_SCOPED_ENUM (Fight,
   None = 0,
   Strafe,
   Stay
)

// dodge type
CR_DECLARE_SCOPED_ENUM (Dodge,
   None = 0,
   Left,
   Right
)

// reload state
CR_DECLARE_SCOPED_ENUM (Reload,
   None = 0, // no reload state currently
   Primary, // primary weapon reload state
   Secondary  // secondary weapon reload state
)

// collision probes
CR_DECLARE_SCOPED_ENUM (CollisionProbe, uint32_t,
   Jump = cr::bit (0), // probe jump when colliding
   Duck = cr::bit (1), // probe duck when colliding
   Strafe = cr::bit (2) // probe strafing when colliding
)

// game start messages for counter-strike...
CR_DECLARE_SCOPED_ENUM (BotMsg,
   None = 1,
   TeamSelect = 2,
   ClassSelect = 3,
   Buy = 100,
   Radio = 200,
   Say = 10000,
   SayTeam = 10001
)

// sensing states
CR_DECLARE_SCOPED_ENUM_TYPE (Sense, uint32_t,
   SeeingEnemy = cr::bit (0), // seeing an enemy
   HearingEnemy = cr::bit (1), // hearing an enemy
   SuspectEnemy = cr::bit (2), // suspect enemy behind obstacle
   PickupItem = cr::bit (3), // pickup item nearby
   ThrowExplosive = cr::bit (4), // could throw he grenade
   ThrowFlashbang = cr::bit (5), // could throw flashbang
   ThrowSmoke = cr::bit (6) // could throw smokegrenade
)

// positions to aim at
CR_DECLARE_SCOPED_ENUM_TYPE (AimFlags, uint32_t,
   Nav = cr::bit (0), // aim at nav point
   Camp = cr::bit (1), // aim at camp vector
   PredictPath = cr::bit (2), // aim at predicted path
   LastEnemy = cr::bit (3), // aim at last enemy
   Entity = cr::bit (4), // aim at entity like buttons, hostages
   Enemy = cr::bit (5), // aim at enemy
   Grenade = cr::bit (6), // aim for grenade throw
   Override = cr::bit (7), // overrides all others (blinded)
   Danger = cr::bit (8) // additional danger flag
)

// famas/glock burst mode status + m4a1/usp silencer
CR_DECLARE_SCOPED_ENUM (BurstMode,
   On = cr::bit (0),
   Off = cr::bit (1)
)

// visibility flags
CR_DECLARE_SCOPED_ENUM (Visibility,
   Head = cr::bit (1),
   Body = cr::bit (2),
   Other = cr::bit (3),
   None = 0
)

// goal tactic
CR_DECLARE_SCOPED_ENUM (GoalTactic,
   Defensive = 0,
   Camp,
   Offensive,
   Goal,
   RescueHostage
)

// some hard-coded desire defines used to override calculated ones
namespace TaskPri {
   constexpr auto Normal { 35.0f };
   constexpr auto Pause { 36.0f };
   constexpr auto Camp { 37.0f };
   constexpr auto Spraypaint { 38.0f };
   constexpr auto FollowUser { 39.0f };
   constexpr auto MoveToPosition { 50.0f };
   constexpr auto DefuseBomb { 89.0f };
   constexpr auto PlantBomb { 89.0f };
   constexpr auto Attack { 90.0f };
   constexpr auto SeekCover { 91.0f };
   constexpr auto Hide { 92.0f };
   constexpr auto Throw { 99.0f };
   constexpr auto DoubleJump { 99.0f };
   constexpr auto Blind { 100.0f };
   constexpr auto ShootBreakable { 100.0f };
   constexpr auto EscapeFromBomb { 100.0f };
};

constexpr auto kInfiniteDistance = 9999999.0f;
constexpr auto kInvalidLightLevel = kInfiniteDistance;
constexpr auto kGrenadeCheckTime = 0.6f;
constexpr auto kSprayDistance = 272.0f;
constexpr auto kSprayDistanceX2 = kSprayDistance * 2;
constexpr auto kMaxChatterRepeatInterval = 99.0f;
constexpr auto kViewFrameUpdate = 1.0f / 25.0f;
constexpr auto kGrenadeDamageRadius = 385.0f;
constexpr auto kMinMovedDistance = 3.0f;

constexpr auto kInfiniteDistanceLong = static_cast <int> (kInfiniteDistance);
constexpr auto kMaxWeapons = 32;
constexpr auto kNumWeapons = 26;
constexpr auto kMaxCollideMoves = 4;
constexpr auto kGameMaxPlayers = 32;
constexpr auto kGameTeamNum = 2;
constexpr auto kInvalidNodeIndex = -1;
constexpr auto kGrenadeInventoryEmpty = -1;
constexpr auto kInvalidRadioSlot = -1;
constexpr auto kConfigExtension = "cfg";

// weapon masks
constexpr auto kPrimaryWeaponMask = (cr::bit (Weapon::XM1014) |
                                     cr::bit (Weapon::M3) |
                                     cr::bit (Weapon::MAC10) |
                                     cr::bit (Weapon::UMP45) |
                                     cr::bit (Weapon::MP5) |
                                     cr::bit (Weapon::TMP) |
                                     cr::bit (Weapon::P90) |
                                     cr::bit (Weapon::AUG) |
                                     cr::bit (Weapon::M4A1) |
                                     cr::bit (Weapon::SG552) |
                                     cr::bit (Weapon::AK47) |
                                     cr::bit (Weapon::Scout) |
                                     cr::bit (Weapon::SG550) |
                                     cr::bit (Weapon::AWP) |
                                     cr::bit (Weapon::G3SG1) |
                                     cr::bit (Weapon::M249) |
                                     cr::bit (Weapon::Famas) |
                                     cr::bit (Weapon::Galil));

constexpr auto kSecondaryWeaponMask = (cr::bit (Weapon::P228)
                                       | cr::bit (Weapon::Elite)
                                       | cr::bit (Weapon::USP)
                                       | cr::bit (Weapon::Glock18)
                                       | cr::bit (Weapon::Deagle)
                                       | cr::bit (Weapon::FiveSeven));

constexpr auto kSniperWeaponMask = (cr::bit (Weapon::Scout)
                                    | cr::bit (Weapon::SG550)
                                    | cr::bit (Weapon::AWP)
                                    | cr::bit (Weapon::G3SG1));

// weapons < 7 are secondary
constexpr auto kPrimaryWeaponMinIndex = 7;

// grenade model names
inline constexpr StringRef kExplosiveModelName = "hegrenade.mdl";
inline constexpr StringRef kFlashbangModelName = "flashbang.mdl";
inline constexpr StringRef kSmokeModelName = "smokegrenade.mdl";
