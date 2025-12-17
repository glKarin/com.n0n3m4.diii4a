#ifndef __BOTRIX_TYPES_H__
#define __BOTRIX_TYPES_H__


#include <good/string.h>
#include <good/vector.h>


enum TPlayerIndexInvalid
{
    EPlayerIndexInvalid = -1                     ///< Invalid player index.
};
typedef int TPlayerIndex;                        ///< Index of player in array of players.
typedef int TWeaponId;                           ///< Weapon id.
typedef int TTeam;                               ///< Team of bot.
typedef int TClass;                              ///< Class of bot.


//****************************************************************************************************************
/// Supported Source Mods.
//****************************************************************************************************************
enum TModIds
{
    EModId_Invalid = -1,                         ///< Unknown mods will be using HL2DM mod.
    EModId_HL2DM = 0,                            ///< Half Life 2 Deathmatch. Unknown mods will be using this one.
    EModId_TF2,                                  ///< Team fortress 2.
    EModId_CSS,                                  ///< Counter Strike Source.
    EModId_Borzh,                                ///< BorzhMod.

    EModId_Total                                 ///< Total amount of supported mods.
};
typedef int TModId;                              ///< Supported Source Mods.


//****************************************************************************************************************
/// Enum that represent events that need to be executed in next frame after being received.
//****************************************************************************************************************
enum TFrameEventId
{
    EFrameEventActivated = 0,                    ///< Player put in server.
    EFrameEventRespawned,                        ///< Player respawned.
    EFrameEventTotal                             ///< Amount of frame events.
};
typedef int TFrameEvent;                         ///< Events that need to be executed in next frame after being received.


//****************************************************************************************************************
/// Bot intelligence.
//****************************************************************************************************************
enum TBotIntelligences
{
    EBotFool = 0,                                ///< Inaccurate fire, no hearing, slow reaction to damage.
    EBotStupied,                                 ///< Better fire, inaccurate hearing, reaction to damage.
    EBotNormal,                                  ///< Normail fire, accurate hearing, normal reaction to damage.
    EBotSmart,                                   ///< Precise fire, fast reaction to damage.
    EBotPro,                                     ///< Invincible.

    EBotIntelligenceTotal                        ///< Amount of available intelligences.
};
typedef int TBotIntelligence;                    ///< Bot intelligence.


//****************************************************************************************************************
/// Bot's objectives.
//****************************************************************************************************************
enum TObjectives
{
    EObjectiveUnknown = -1,                      ///< No current objective.
    EObjectiveStop = 0,                          ///< Stop for a certain time.
    EObjectiveFollow,                            ///< Follow team mate.
    EObjectiveKill,                              ///< Kill enemy.
    EObjectiveSitDown,                           ///< Sit down.
    EObjectiveStandUp,                           ///< Stand up.
    EObjectiveJump,                              ///< Leave.
};
typedef int TObjective;                          ///< Bot's objectives.


//****************************************************************************************************************
/// Bot sentencies that bot can say.
//****************************************************************************************************************
enum TBotChats
{
    EBotChatUnknown = -1,                        ///< Unknown talk.

    EBotChatError = 0,                           ///< Can't understand what player said.

    EBotChatGreeting,                            ///< Initiation of conversation.
    EBotChatBye,                                 ///< End conversation.
    EBotChatBusy,                                ///< Bot is busy, cancel conversation.

    EBotChatAffirmative,                         ///< Affirmative/yes.
    EBotChatNegative,                            ///< Negative/no.
    EBotChatAffirm,                              ///< Affirm action request.
    EBotChatNegate,                              ///< Negate action request.

    EBotChatCall,                                ///< Addressing someone (hey %player).
    EBotChatCallResponse,                        ///< Callee answers positively (what do you need?).

    EBotChatHelp,                                ///< Asking for help (can you help me?).

    EBotChatStop,                                ///< Stop moving.
    EBotChatCome,                                ///< Come and stay at current player position.
    EBotChatFollow,                              ///< Follow player.
    EBotChatAttack,                              ///< Attack until new command.
    EBotChatNoKill,                              ///< Stop attacking until new command.
    EBotChatSitDown,                             ///< Crouch.
    EBotChatStandUp,                             ///< Stop crouching.
    EBotChatJump,                                ///< Jump.
    EBotChatLeave,                               ///< Continue playing, stop helping (you can leave now).
    EBotChatDontHurt,                            ///< Stop hurting me!

    EBorzhChatOk,                                ///< Ok.
    EBorzhChatDone,                              ///< Done, finished task.
    EBorzhChatWait,                              ///< Please wait until I finish.
    EBorzhChatNoMoves,                           ///< No more moves.
    EBorzhChatThink,                             ///< Bot will think (use FF).
    EBorzhChatExplore,                           ///< Explore current area.
    EBorzhChatExploreNew,                        ///< Explore new area.
    EBorzhChatFinishExplore,                     ///< Bot ends exploring this area.

    EBorzhChatNewArea,                           ///< Bot enters new area.
    EBorzhChatChangeArea,                        ///< Bot changes area.

    EBorzhChatWeaponFound,                       ///< Bot founds a weapon.

    EBorzhChatDoorFound,                         ///< Bot founds a new door.
    EBorzhChatDoorChange,                        ///< Bot sees a change in door status.
    EBorzhChatDoorNoChange,                      ///< Bot sees no change in door status.

    EBorzhChatSeeButton,                         ///< Bot sees a button.
    EBorzhChatButtonCanPush,                     ///< Bot can press a button.
    EBorzhChatButtonCantPush,                    ///< Bot can't reach this button.

    EBorzhChatBoxFound,                          ///< Bot founds a box.
    EBorzhChatBoxLost,                           ///< Bot lost a box.
    EBorzhChatHaveGravityGun,                    ///< Bot has a gravity gun to lift a box.
    EBorzhChatNeedGravityGun,                    ///< Bot need a gravity gun to lift a box.

    EBorzhChatWallFound,                         ///< Bot founds a wall that can be climbed with help of box.
    EBorzhChatBoxNeed,                           ///< Bot needs a box to climb the wall.
    EBorzhChatBoxTry,                            ///< Bot offers to bring a box to area for climbing the wall.

    EBorzhChatBoxITake,                          ///< Bot will take a box.
    EBorzhChatBoxYouTake,                        ///< Bot says to player to take a box.
    EBorzhChatBoxIDrop,                          ///< Bot will drop the box.
    EBorzhChatBoxYouDrop,                        ///< Bot says to player to drop the box.

    EBorzhChatButtonWeapon,                      ///< Bot has weapon to shoot button.
    EBorzhChatButtonNoWeapon,                    ///< Bot doesn't have weapon to shoot button.

    EBorzhChatDoorTry,                           ///< Bot says to find button that opens given door.
    EBorzhChatButtonTry,                         ///< Bot says to find doors that opens given button.
    EBorzhChatButtonTryGo,                       ///< Bot says to try to go to given button.
    EBorzhChatButtonDoor,                        ///< Bot says to check button-door configuration.
    EBorzhChatButtonToggles,                     ///< Bot says that button $button toggles door $door.
    EBorzhChatButtonNoToggles,                   ///< Bot says that button $button doesn't affect door $door.

    EBorzhChatButtonIPush,                       ///< Bot will try to push button.
    EBorzhChatButtonYouPush,                     ///< Bot orders to push button to other bot.
    EBorzhChatButtonIShoot,                      ///< Bot will try to shoot button.
    EBorzhChatButtonYouShoot,                    ///< Bot orders to shoot button to other bot.

    EBorzhChatAreaGo,                            ///< Bot orders to go to some area to other bot.
    EBorzhChatAreaCantGo,                        ///< Bot founds closed door when it tries to pass to given area.
    EBorzhChatDoorGo,                            ///< Bot orders to go to some door to other bot.
    EBorzhChatButtonGo,                          ///< Bot orders to go to some button to other bot.

    EBorzhChatTaskCancel,                        ///< Bot releases other players from task.
    EBorzhChatBetterIdea,                        ///< Bot has better idea.

    EBorzhChatFoundPlan,                         ///< Bot founds a plan to reach goal.

    EBotChatTotal                                ///< Amount of bot sentences.
};
typedef int TBotChat;                            ///< Bot sentence.



//****************************************************************************************************************
/// Result of executing console command.
//****************************************************************************************************************
enum TCommandResults
{
    ECommandPerformed,                            ///< All okay.
	ECommandNotFound,                             ///< Command not found.
	ECommandNotImplemented,                       ///< Command not implemented.
	ECommandRequireAccess,                        ///< User don't have access to command.
	ECommandError,                                ///< Error occurred.
	ECommandTotal,                                ///< Total amount.
};
typedef int TCommandResult;


//****************************************************************************************************************
/// Flags to access console commands.
//****************************************************************************************************************
enum TCommandAccessFlag
{
    FCommandAccessInvalid      = -1,             ///< Invalid access flag.
    FCommandAccessNone         = 0,              ///< No access to commands.

    FCommandAccessWaypoint     = 1<<0,           ///< Access to waypoint commands.
    FCommandAccessBot          = 1<<1,           ///< Access to bot's commands.
    FCommandAccessConfig       = 1<<2,           ///< Access to configuration's commands.

    ECommandAccessFlagTotal    = 3,              ///< Amount of access flags.
    FCommandAccessAll          = (1<<3) - 1      ///< Access to all commands.
};
typedef int TCommandAccessFlags;


//****************************************************************************************************************
/// Waypoint flag.
//****************************************************************************************************************
enum TWaypointFlag
{
    FWaypointNone              = 0,              ///< Just reachable position.

    // First byte.
    FWaypointStop              = 1<<0,           ///< You need to stop totally to use waypoint.
    FWaypointCamper            = 1<<1,           ///< Camp at spot and wait for enemy. Argument are 2 angles to aim (low and high words).
    FWaypointSniper            = 1<<2,           ///< Sniper spot. Argument are 2 angles to aim (low and high words).
    FWaypointWeapon            = 1<<3,           ///< Weapon is there. Arguments are weapon index/subindex (2nd byte).
    FWaypointAmmo              = 1<<4,           ///< Ammo is there. Arguments are ammo count (1st byte) and weapon (2nd byte).
    FWaypointHealth            = 1<<5,           ///< Health is there. Argument is health amount (3rd byte).
    FWaypointArmor             = 1<<6,           ///< Armor is there. Argument is armor amount (4th byte).
    FWaypointHealthMachine     = 1<<7,           ///< Health machine is there. Arguments are 1 angle (low word) and health amount (3rd byte).

    // Second byte.
    FWaypointArmorMachine      = 1<<8,           ///< Armor machine is there. Arguments are 1 angle (low word) and health amount (3rd byte).
    FWaypointButton            = 1<<9,           ///< A button is there. Arguments are angle (low word)  button index+1 (3rd byte), door index+1 (4th byte).
    FWaypointSeeButton         = 1<<10,          ///< Button is visible. Arguments are angle (low word), button index+1 (3rd byte), door index+1 (4th byte).
    FWaypointUse               = 1<<11,          ///< Always need to USE (along with first angle).
    FWaypointElevator          = 1<<12,          ///< Waypoint should be marked as button. Instead of door's index it has elevator's index.
    FWaypointLadder            = 1<<13,          ///< Ladder point.

    EWaypointFlagTotal         = 14,             ///< Amount of waypoint flags.
    FWaypointAll               = (1<<14) - 1,    ///< All flags set.
};
typedef short TWaypointFlags;                    ///< Set of waypoint flags.
typedef int TWaypointArgument;                   ///< Waypoint argument.


//****************************************************************************************************************
/// Waypoint flag.
//****************************************************************************************************************
enum TPathFlag
{
    FPathNone                  = 0,              ///< Just run to reach adjacent waypoint.

    FPathCrouch                = 1<<0,           ///< Crouch to reach adjacent waypoint.
    FPathJump                  = 1<<1,           ///< Jump to reach adjacent waypoint. If crouch flag is set will use jump with duck.
    FPathBreak                 = 1<<2,           ///< Use weapon to break something to reach adjacent waypoint.
    FPathSprint                = 1<<3,           ///< Need to sprint to get to adjacent waypoint. Usefull for long jump.
    FPathLadder                = 1<<4,           ///< Need to use ladder move (IN_FORWARD & IN_BACK) to get to adjacent waypoint.
    FPathStop                  = 1<<5,           ///< Stop at current waypoint to go in straight line to next one. Usefull when risk of falling.
    FPathDamage                = 1<<6,           ///< Need to take damage to get to adjacent waypoint. Argument is damage count.
    FPathFlashlight            = 1<<7,           ///< Need to turn on flashlight.

    FPathDoor                  = 1<<8,           ///< There is a door on the way. Argument has door number / button number.
    FPathElevator              = 1<<9,           ///< Stand still until reach next waypoint. Argument has elevator number / button number.
    FPathTotem                 = 1<<10,          ///< Need to make ladder of living corpses. Argument is count of players needed (1..).
	
	EPathFlagUserTotal         = 11,             ///< Amount of path flags for the user.
	FPathAll                   = (1<<11)-1,      ///< All userpath flags.

    FPathDemo                  = 1<<15,          ///< Flag for use demo to reach adjacent waypoints. Demo number is at lower bits. Not implemented yet.
    EPathFlagTotal             = 16,             ///< Amount of path flags.
};
typedef short TPathFlags;                        ///< Set of waypoint path flags.
typedef short TPathArgument;                     ///< Waypoint path argument.

enum TInvalidWaypoint
{
    EWaypointIdInvalid         = -1              ///< Constant to indicate that waypoint is invalid.
};
typedef int TWaypointId;                         ///< Waypoint ID is index of waypoint in array of waypoints.

enum TInvalidArea
{
    EAreaIdInvalid             = 255             ///< Constant to indicate that waypoint's area is invalid.
};
typedef unsigned char TAreaId;                   ///< Waypoints are grouped in areas. This is a type for area id.

//****************************************************************************************************************
/// Flags for draw type of waypoints.
//****************************************************************************************************************
enum TWaypointDrawFlag
{
    FWaypointDrawNone          = 0,              ///< Don't draw waypoints.
    FWaypointDrawBeam          = 1<<0,           ///< Draw beam.
    FWaypointDrawLine          = 1<<1,           ///< Draw line.
    FWaypointDrawBox           = 1<<2,           ///< Draw box.
    FWaypointDrawText          = 1<<3,           ///< Draw text (id, area, etc.).

    EWaypointDrawFlagTotal     = 4,              ///< Amount of draw type flags.
    FWaypointDrawAll           = (1<<4)-1,       ///< Draw all.
};
typedef int TWaypointDrawFlags;                  ///< Set of waypoint draw types.


//****************************************************************************************************************
/// Flags for draw type of waypoints paths.
//****************************************************************************************************************
enum TPathDrawFlag
{
    FPathDrawNone              = 0,              ///< Don't draw paths.
    FPathDrawBeam              = 1<<0,           ///< Draw beam.
    FPathDrawLine              = 1<<1,           ///< Draw line.

    EPathDrawFlagTotal         = 2,              ///< Amount of draw type flags.

    FPathDrawAll               = (1<<2)-1,       ///< Draw beams and lines.
};
typedef int TPathDrawFlags;                      ///< Set of draw types for paths.


//****************************************************************************************************************
/// Event types.
//****************************************************************************************************************
typedef enum TEventType
{
    EEventTypeKeyValues,                         ///< Event is of type KeyValues for receiver IGameEventListener.
    EEventTypeIGameEvent                         ///< Event is of type IGameEvent for receiver IGameEventListener2.
} TEventType;


//****************************************************************************************************************
/// Items types / object / other entities.
//****************************************************************************************************************
enum TItemTypes
{
    EItemTypeInvalid = -1,                     ///< Invalid entity type.
    EItemTypeHealth = 0,                       ///< Item that restores players health. Can be health machine also.
    EItemTypeArmor,                            ///< Item that restores players armor. Can be armor machine also.
    EItemTypeWeapon,                           ///< Weapon.
    EItemTypeAmmo,                             ///< Ammo for weapon.
    EItemTypeCanPickTotal,                     ///< Items that bot can pick up.

    EItemTypeButton = EItemTypeCanPickTotal,   ///< Button.
    EItemTypeDoor,                             ///< Door.
    EItemTypeElevator = EItemTypeDoor,         ///< Elevators are treated like doors.
    EItemTypeLadder,                           ///< Ladder.
    EItemTypeObject,                           ///< Object that can stuck player (or optionally be moved).
    EItemTypeTotalNotObject = EItemTypeObject, ///< All that is not object nor other.

    EItemTypeCollisionTotal,                   ///< Items that bot can collision with.

    /// Entity where player spawns.
    EItemTypePlayerSpawn = EItemTypeCollisionTotal,                      
    EItemTypeOther,                            ///< All other type of entities.

    EItemTypeKnownTotal = EItemTypeOther,      ///< Amount of known item types (other doens't count).
    EItemTypeAll,                              ///< Total for all.
};
typedef int TItemType;                         ///<  Items types / object / other entities.

enum TItemClassIndexInvalid
{
    EItemClassIndexInvalid = -1                ///< Invalid entity class index.
};
typedef int TItemClassIndex;                   ///< Index of entity class in CItems::GetClass().

enum TItemIndexInvalid
{
    EItemIndexInvalid = -1                     ///< Invalid entity index.
};
typedef int TItemIndex;                        ///< Index of entity in CItems::GetItems().
typedef int TItemId;                           ///< Server index of entity (m_EntIndex).

enum TItemTypeFlag
{
    FItemTypeAll = (1<<(EItemTypeOther+1))-1   ///< Flag to draw all items.
};
typedef int TItemTypeFlags;                    ///< Item type flags (used to define which items to draw).


//****************************************************************************************************************
/// Mod variables.
//****************************************************************************************************************
enum TModVars
{
    // If you change the order, go to mod.cpp and check the order of defaults array.
    EModVarPlayerMaxHealth = 0,                ///< Player's max health.
    EModVarPlayerMaxArmor,                     ///< Player's height.

    EModVarPlayerWidth,                        ///< Player's width.
    EModVarPlayerHeight,                       ///< Player's height.
    EModVarPlayerHeightCrouched,               ///< Player's height crouched.
    EModVarPlayerEye,                          ///< Player's eye, for waypoints.
    EModVarPlayerEyeCrouched,                  ///< Player's eye crouched.
    
    EModVarPlayerVelocityCrouch,               ///< Player's velocity while crouched.
    EModVarPlayerVelocityWalk,                 ///< Player's walk velocity.
    EModVarPlayerVelocityRun,                  ///< Player's run velocity.
    EModVarPlayerVelocitySprint,               ///< Player's sprint velocity.

    EModVarPlayerObstacleToJump,               ///< Obstacle height so need to jump.
    EModVarPlayerJumpHeight,                   ///< Player's jump height.
    EModVarPlayerJumpHeightCrouched,           ///< Player's jump height crouched.
    
    EModVarHeightForFallDamage,                ///< Max height so fall doesn't produce damage.
    EModVarSlopeGradientToSlideOff,            ///< Player will slide off if gradient is more that this value.

    EModVarTotal                               ///< Mod vars count.
};
typedef int TModVar;                           ///< Mod variable.


//****************************************************************************************************************
/// Item types.
//****************************************************************************************************************
enum TItemDrawFlag
{
    FItemDontDraw              = 0,              ///< Don't draw item.
    FItemDrawStats             = 1<<0,           ///< Draw item class name, stats, model.
    FItemDrawBoundBox          = 1<<1,           ///< Draw bound box around item.
    FItemDrawWaypoint          = 1<<2,           ///< Draw line to nearest waypoint.

    EItemDrawFlagTotal         = 3,              ///< Amount of draw type flags.
    FItemDrawAll               = (1<<3)-1,       ///< Draw all.
};
typedef int TItemDrawFlags;                      ///< Item draw flags.


//****************************************************************************************************************
/// Item flags.
//****************************************************************************************************************
enum TItemFlag
{
    FEntityNone                = 0,              ///< Entity has no flags.
    FItemUse                   = 1<<0,           ///< Press button USE to use entity (like health/armor machines).
    FEntityRespawnable         = 1<<1,           ///< Entity is respawnable.
    FObjectExplosive           = 1<<2,           ///< Entity is explosive.
    FObjectHeavy               = 1<<3,           ///< Can't use physcannon on this object.
    FObjectBox                 = 1<<4,           ///< Can use this entity to jump on.

    EItemFlagsUserTotal        = 5,              ///< Amount of entity flags.
    FEntityAll                 = (1<<5)-1,       ///< Entity flags that are configurable at config.ini or through console.

    FTaken                     = 1<<5,           ///< This flag is set for all weapons that belong to some player.
    EItemFlagsTotal            = 6,              ///< All item flags.
};
typedef int TItemFlags;                          ///< Entity flags.


//****************************************************************************************************************
/// Weapon types.
//****************************************************************************************************************
enum TWeaponTypes
{
    EWeaponIdInvalid = -1,                       ///< Invalid weapon id.
    EWeaponMelee = 0,                            ///< Knife, stunstick, crowbar.
    EWeaponGrenade,                              ///< Grenade.

    // Ranged.
    EWeaponPhysics,                              ///< Physcannon.
    EWeaponRemoteDetonation,                     ///< Remote detonation.
    EWeaponPistol,                               ///< Desert eagle, usp, etc.
    EWeaponRifle,                                ///< Automatic, just press once.
    EWeaponShotgun,                              ///< Shotgun, reload one by one.
    EWeaponRocket,                               ///< Rpg-like, reload one by one.

    EWeaponTotal                                 ///< Amount of weapon flags.

};
typedef int TWeaponType;                         ///< Weapon type.


//****************************************************************************************************************
/// Enum for weapon flags.
//****************************************************************************************************************
enum TWeaponFlagIds
{
    FWeaponZoom              = 1<<0,             ///< This weapon function will zoom.
    FWeaponTrigger           = 1<<1,             ///< This weapon function is trigger for remote detonation.
    FWeaponCure              = 1<<2,             ///< This weapon function will cure teammates.
    FWeaponPushAway          = 1<<3,             ///< Will push away enemy.
    FWeaponDeflect           = 1<<4,             ///< Deflect proyectiles.
    FWeaponExtinguish        = 1<<5,             ///< Will extinguish fire.
    FWeaponPrepare           = 1<<6,             ///< Will prepare weapon (spin for minigun). Will use shot time as preparing time.
    FWeaponSlowing           = 1<<7,             ///< Will slow enemy.

    FWeaponHasSecondary      = (1<<8)-1,         ///< Mask to know if weapon has secondary function.

    FWeaponForceRange        = 1<<8,             ///< Don't use weapon if target is not in range.
    FWeaponForceAim          = 1<<9,             ///< Force to mantain aim while shooting (to rpg like weapons).
    FWeaponSameBullets       = 1<<10,            ///< Secondary attack uses same bullets as primary (shotgun for example).
    FWeaponDontAddClip       = 1<<11,            ///< Bug for some mods: some weapons add clip size to extra ammo when weapon is picked (crossbow for hl2dm).
    FWeaponDefaultClipEmpty  = 1<<12,            ///< Clip is empty when respawned / grab this weapon.
    FWeaponBackgroundReload  = 1<<13,            ///< Bug for some mods: some weapons reload while you hold another weapon (pistol for hl2dm).

    EWeaponFlagsTotal        = 14,               ///< Amount of flags.
    FWeaponAll               = (1<<14)-1         ///< Mask for all weapon flags.
};
typedef int TWeaponFlags;                        ///< Weapon flags.


//****************************************************************************************************************
/// Weapon aim.
//****************************************************************************************************************
enum TWeaponAimIds
{
    EWeaponAimBody = 0,                          ///< Aim at body.
    EWeaponAimHead,                              ///< Aim at head.
    EWeaponAimFoot,                              ///< Aim at foot.
    EWeaponAimTotal,                             ///< Total amounts of aim for weapon.
};
typedef int TWeaponAim;                          ///< Weapon aim.


//****************************************************************************************************************
/// Enum for fight strategy flags.
//****************************************************************************************************************
enum TFightStrategyFlagIds
{
    FFightStrategyRunAwayIfNear   = 1<<0,        ///< Bot with weapon will run away from enemy if near.
    FFightStrategyComeCloserIfFar = 1<<1,        ///< Bot will come close if he is too far away.
    // TODO:
    //FFightStrategyForceStayFar    = 1<<2,        ///< Bot will always stay far away from enemy even if enemy is not visible.
    //FFightStrategyMeleeIfClose    = 1<<3,        ///< Bot will switch to melee weapon if enemy is close.

    EFightStrategyFlagTotal = 2,                 ///< Amount of bot strategy flags.
    FFightStrategyAll = (1<<2)-1,                ///< Mask for all strategies.
};
typedef int TFightStrategyFlags;                 ///< Fight strategy flags.


//****************************************************************************************************************
/// Enum for fight strategy arguments.
//****************************************************************************************************************
enum TFightStrategyArgIds
{
    EFightStrategyArgNearDistance = 0,           ///< Near distance if 'run-away-if-near' is set.
    EFightStrategyArgFarDistance,                ///< Bot will come close if he is too far away.

    EFightStrategyArgTotal,                      ///< Amount of bot strategy arguments.
};
typedef int TFightStrategyArg;                   ///< Fight strategy argument.


//****************************************************************************************************************
/// Enum for bot tasks.
//****************************************************************************************************************
enum TBotTaskIds
{
    EBotTaskInvalid = -1,                        ///< Bot has no tasks.
    EBotTaskFindHealth = 0,                      ///< Find health.
    EBotTaskFindArmor,                           ///< Find armor.
    EBotTaskFindWeapon,                          ///< Find weapon.
    EBotTaskFindAmmo,                            ///< Find ammo for weapon.
    EBotTaskEngageEnemy,                         ///< Fight enemy.
    EBotTaskFindEnemy,                           ///< Run randomly around and check if can see an enemy.
    EBotTasksTotal,                              ///< This should be first task number of enums of mod tasks.
};
typedef int TBotTask;                            ///< Bot task.


//****************************************************************************************************************
/// Enum for console commands auto-complete arguments.
//****************************************************************************************************************
enum TConsoleAutoCompleteArgs
{
	EConsoleAutoCompleteArgInvalid = -1,         ///< Invalid argument.
	EConsoleAutoCompleteArgBool = 0,             ///< Boolean: on/off.
	EConsoleAutoCompleteArgWaypoint,             ///< Argument is waypoint id: current/destination/other.
	EConsoleAutoCompleteArgWaypointForever,      ///< Same as waypoint, but forever.
	EConsoleAutoCompleteArgWeapon,               ///< Argument is weapon id.
	EConsoleAutoCompleteArgBots,                 ///< Argument is bot names once.
	EConsoleAutoCompleteArgBotsForever,          ///< Argument is bot names forever.
	EConsoleAutoCompleteArgUsers,                ///< Argument is client names once.
	EConsoleAutoCompleteArgUsersForever,         ///< Argument is client names once.
	EConsoleAutoCompleteArgPlayers,              ///< Argument is players names once.
	EConsoleAutoCompleteArgPlayersForever,       ///< Argument is players names forever.
	EConsoleAutoCompleteArgIgnore,               ///< Ignore auto-completion, like a number or new bot's name, for instance.
	EConsoleAutoCompleteArgValues,               ///< Auto-complete with given fixed array of strings in m_cAutoCompleteArguments once.
	EConsoleAutoCompleteArgValuesForever,        ///< Same as values, but forever. Useful for flags.
	EConsoleAutoCompleteArgsTotal,               ///< Total arguments.
};
typedef int TConsoleAutoCompleteArg;             ///< Console commands auto-complete arguments.


/*
///< Enum of useful flags for deathmatch mode.
enum TDeathmatchFlagId
{
    FDeathmatchTeamAllWeapons  = 1<<0,           ///< In deathmatch mode users can grab weapons of "other" teams.
    FDeathmatchClassAllWeapons = 1<<1,           ///< In deathmatch mode users can grab weapons of "other" classes.
};

typedef int TDeathmatchFlags;                    ///< Useful flags for deathmatch mode.
*/


typedef good::vector<good::string> StringVector; ///< Useful typedef for vector of strings.


#endif // __BOTRIX_TYPES_H__
