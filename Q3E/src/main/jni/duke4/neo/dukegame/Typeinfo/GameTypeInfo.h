
#ifndef __GAMETYPEINFO_H__
#define __GAMETYPEINFO_H__

/*
===================================================================================

	This file has been generated with the Type Info Generator v1.1 (c) 2004 id Software

	593 constants
	58 enums
	314 classes/structs/unions
	4 templates
	7 max inheritance level for 'DnPigcop'

===================================================================================
*/

typedef struct {
	const char * name;
	const char * type;
	const char * value;
} constantInfo_t;

typedef struct {
	const char * name;
	int value;
} enumValueInfo_t;

typedef struct {
	const char * typeName;
	const enumValueInfo_t * values;
} enumTypeInfo_t;

typedef struct {
	const char * type;
	const char * name;
	intptr_t offset;
	int size;
} classVariableInfo_t;

typedef struct {
	const char * typeName;
	const char * superType;
	int size;
	const classVariableInfo_t * variables;
} classTypeInfo_t;

static constantInfo_t constantInfo[] = {
	{ "const int", "INITIAL_RELEASE_BUILD_NUMBER", "1262" },
	{ "int", "ev_error", "-1" },
	{ "int", "ev_void", "0" },
	{ "int", "ev_scriptevent", "1" },
	{ "int", "ev_namespace", "2" },
	{ "int", "ev_string", "3" },
	{ "int", "ev_float", "4" },
	{ "int", "ev_vector", "5" },
	{ "int", "ev_entity", "6" },
	{ "int", "ev_field", "7" },
	{ "int", "ev_function", "8" },
	{ "int", "ev_virtualfunction", "9" },
	{ "int", "ev_pointer", "10" },
	{ "int", "ev_object", "11" },
	{ "int", "ev_jumpoffset", "12" },
	{ "int", "ev_argsize", "13" },
	{ "int", "ev_boolean", "14" },
	{ "int", "idVarDef::uninitialized", "0" },
	{ "int", "idVarDef::initializedVariable", "1" },
	{ "int", "idVarDef::initializedConstant", "2" },
	{ "int", "idVarDef::stackVariable", "3" },
	{ "const int", "ANIM_NumAnimChannels", "5" },
	{ "const int", "ANIM_MaxAnimsPerChannel", "3" },
	{ "const int", "ANIM_MaxSyncedAnims", "3" },
	{ "const int", "ANIMCHANNEL_ALL", "0" },
	{ "const int", "ANIMCHANNEL_TORSO", "1" },
	{ "const int", "ANIMCHANNEL_LEGS", "2" },
	{ "const int", "ANIMCHANNEL_HEAD", "3" },
	{ "const int", "ANIMCHANNEL_EYELIDS", "4" },
	{ "int", "JOINTMOD_NONE", "0" },
	{ "int", "JOINTMOD_LOCAL", "1" },
	{ "int", "JOINTMOD_LOCAL_OVERRIDE", "2" },
	{ "int", "JOINTMOD_WORLD", "3" },
	{ "int", "JOINTMOD_WORLD_OVERRIDE", "4" },
	{ "int", "FC_SCRIPTFUNCTION", "0" },
	{ "int", "FC_SCRIPTFUNCTIONOBJECT", "1" },
	{ "int", "FC_EVENTFUNCTION", "2" },
	{ "int", "FC_SOUND", "3" },
	{ "int", "FC_SOUND_VOICE", "4" },
	{ "int", "FC_SOUND_VOICE2", "5" },
	{ "int", "FC_SOUND_BODY", "6" },
	{ "int", "FC_SOUND_BODY2", "7" },
	{ "int", "FC_SOUND_BODY3", "8" },
	{ "int", "FC_SOUND_WEAPON", "9" },
	{ "int", "FC_SOUND_ITEM", "10" },
	{ "int", "FC_SOUND_GLOBAL", "11" },
	{ "int", "FC_SOUND_CHATTER", "12" },
	{ "int", "FC_SKIN", "13" },
	{ "int", "FC_TRIGGER", "14" },
	{ "int", "FC_TRIGGER_SMOKE_PARTICLE", "15" },
	{ "int", "FC_MELEE", "16" },
	{ "int", "FC_DIRECTDAMAGE", "17" },
	{ "int", "FC_BEGINATTACK", "18" },
	{ "int", "FC_ENDATTACK", "19" },
	{ "int", "FC_MUZZLEFLASH", "20" },
	{ "int", "FC_CREATEMISSILE", "21" },
	{ "int", "FC_LAUNCHMISSILE", "22" },
	{ "int", "FC_FIREMISSILEATTARGET", "23" },
	{ "int", "FC_FOOTSTEP", "24" },
	{ "int", "FC_LEFTFOOT", "25" },
	{ "int", "FC_RIGHTFOOT", "26" },
	{ "int", "FC_ENABLE_EYE_FOCUS", "27" },
	{ "int", "FC_DISABLE_EYE_FOCUS", "28" },
	{ "int", "FC_FX", "29" },
	{ "int", "FC_DISABLE_GRAVITY", "30" },
	{ "int", "FC_ENABLE_GRAVITY", "31" },
	{ "int", "FC_JUMP", "32" },
	{ "int", "FC_ENABLE_CLIP", "33" },
	{ "int", "FC_DISABLE_CLIP", "34" },
	{ "int", "FC_ENABLE_WALK_IK", "35" },
	{ "int", "FC_DISABLE_WALK_IK", "36" },
	{ "int", "FC_ENABLE_LEG_IK", "37" },
	{ "int", "FC_DISABLE_LEG_IK", "38" },
	{ "int", "FC_RECORDDEMO", "39" },
	{ "int", "FC_AVIGAME", "40" },
	{ "int", "FC_LAUNCH_PROJECTILE", "41" },
	{ "int", "FC_TRIGGER_FX", "42" },
	{ "int", "FC_START_EMITTER", "43" },
	{ "int", "FC_STOP_EMITTER", "44" },
	{ "int", "AF_JOINTMOD_AXIS", "0" },
	{ "int", "AF_JOINTMOD_ORIGIN", "1" },
	{ "int", "AF_JOINTMOD_BOTH", "2" },
	{ "int", "PVS_NORMAL", "0" },
	{ "int", "PVS_ALL_PORTALS_OPEN", "1" },
	{ "int", "PVS_CONNECTED_AREAS", "2" },
	{ "int", "GAME_SP", "0" },
	{ "int", "GAME_DM", "1" },
	{ "int", "GAME_TOURNEY", "2" },
	{ "int", "GAME_TDM", "3" },
	{ "int", "GAME_LASTMAN", "4" },
	{ "int", "GAME_CTF", "5" },
	{ "int", "GAME_COUNT", "6" },
	{ "int", "FLAGSTATUS_INBASE", "0" },
	{ "int", "FLAGSTATUS_TAKEN", "1" },
	{ "int", "FLAGSTATUS_STRAY", "2" },
	{ "int", "FLAGSTATUS_NONE", "3" },
	{ "int", "PLAYER_VOTE_NONE", "0" },
	{ "int", "PLAYER_VOTE_NO", "1" },
	{ "int", "PLAYER_VOTE_YES", "2" },
	{ "int", "PLAYER_VOTE_WAIT", "3" },
	{ "const int", "NUM_CHAT_NOTIFY", "5" },
	{ "const int", "CHAT_FADE_TIME", "400" },
	{ "const int", "FRAGLIMIT_DELAY", "2000" },
	{ "const int", "MP_PLAYER_MINFRAGS", "-100" },
	{ "const int", "MP_PLAYER_MAXFRAGS", "400" },
	{ "const int", "MP_PLAYER_MAXWINS", "100" },
	{ "const int", "MP_PLAYER_MAXPING", "999" },
	{ "const int", "MP_CTF_MAXPOINTS", "25" },
	{ "int", "SND_YOUWIN", "0" },
	{ "int", "SND_YOULOSE", "1" },
	{ "int", "SND_FIGHT", "2" },
	{ "int", "SND_VOTE", "3" },
	{ "int", "SND_VOTE_PASSED", "4" },
	{ "int", "SND_VOTE_FAILED", "5" },
	{ "int", "SND_THREE", "6" },
	{ "int", "SND_TWO", "7" },
	{ "int", "SND_ONE", "8" },
	{ "int", "SND_SUDDENDEATH", "9" },
	{ "int", "SND_FLAG_CAPTURED_YOURS", "10" },
	{ "int", "SND_FLAG_CAPTURED_THEIRS", "11" },
	{ "int", "SND_FLAG_RETURN", "12" },
	{ "int", "SND_FLAG_TAKEN_YOURS", "13" },
	{ "int", "SND_FLAG_TAKEN_THEIRS", "14" },
	{ "int", "SND_FLAG_DROPPED_YOURS", "15" },
	{ "int", "SND_FLAG_DROPPED_THEIRS", "16" },
	{ "int", "SND_COUNT", "17" },
	{ "int", "idMultiplayerGame::INACTIVE", "0" },
	{ "int", "idMultiplayerGame::WARMUP", "1" },
	{ "int", "idMultiplayerGame::COUNTDOWN", "2" },
	{ "int", "idMultiplayerGame::GAMEON", "3" },
	{ "int", "idMultiplayerGame::SUDDENDEATH", "4" },
	{ "int", "idMultiplayerGame::GAMEREVIEW", "5" },
	{ "int", "idMultiplayerGame::NEXTGAME", "6" },
	{ "int", "idMultiplayerGame::STATE_COUNT", "7" },
	{ "int", "idMultiplayerGame::MSG_SUICIDE", "0" },
	{ "int", "idMultiplayerGame::MSG_KILLED", "1" },
	{ "int", "idMultiplayerGame::MSG_KILLEDTEAM", "2" },
	{ "int", "idMultiplayerGame::MSG_DIED", "3" },
	{ "int", "idMultiplayerGame::MSG_VOTE", "4" },
	{ "int", "idMultiplayerGame::MSG_VOTEPASSED", "5" },
	{ "int", "idMultiplayerGame::MSG_VOTEFAILED", "6" },
	{ "int", "idMultiplayerGame::MSG_SUDDENDEATH", "7" },
	{ "int", "idMultiplayerGame::MSG_FORCEREADY", "8" },
	{ "int", "idMultiplayerGame::MSG_JOINEDSPEC", "9" },
	{ "int", "idMultiplayerGame::MSG_TIMELIMIT", "10" },
	{ "int", "idMultiplayerGame::MSG_FRAGLIMIT", "11" },
	{ "int", "idMultiplayerGame::MSG_TELEFRAGGED", "12" },
	{ "int", "idMultiplayerGame::MSG_JOINTEAM", "13" },
	{ "int", "idMultiplayerGame::MSG_HOLYSHIT", "14" },
	{ "int", "idMultiplayerGame::MSG_POINTLIMIT", "15" },
	{ "int", "idMultiplayerGame::MSG_FLAGTAKEN", "16" },
	{ "int", "idMultiplayerGame::MSG_FLAGDROP", "17" },
	{ "int", "idMultiplayerGame::MSG_FLAGRETURN", "18" },
	{ "int", "idMultiplayerGame::MSG_FLAGCAPTURE", "19" },
	{ "int", "idMultiplayerGame::MSG_SCOREUPDATE", "20" },
	{ "int", "idMultiplayerGame::MSG_COUNT", "21" },
	{ "int", "idMultiplayerGame::VOTE_RESTART", "0" },
	{ "int", "idMultiplayerGame::VOTE_TIMELIMIT", "1" },
	{ "int", "idMultiplayerGame::VOTE_FRAGLIMIT", "2" },
	{ "int", "idMultiplayerGame::VOTE_GAMETYPE", "3" },
	{ "int", "idMultiplayerGame::VOTE_KICK", "4" },
	{ "int", "idMultiplayerGame::VOTE_MAP", "5" },
	{ "int", "idMultiplayerGame::VOTE_SPECTATORS", "6" },
	{ "int", "idMultiplayerGame::VOTE_NEXTMAP", "7" },
	{ "int", "idMultiplayerGame::VOTE_COUNT", "8" },
	{ "int", "idMultiplayerGame::VOTE_NONE", "9" },
	{ "int", "idMultiplayerGame::VOTE_UPDATE", "0" },
	{ "int", "idMultiplayerGame::VOTE_FAILED", "1" },
	{ "int", "idMultiplayerGame::VOTE_PASSED", "2" },
	{ "int", "idMultiplayerGame::VOTE_ABORTED", "3" },
	{ "int", "idMultiplayerGame::VOTE_RESET", "4" },
	{ "const int", "MAX_GAME_MESSAGE_SIZE", "8192" },
	{ "const int", "MAX_ENTITY_STATE_SIZE", "512" },
	{ "const int", "ENTITY_PVS_SIZE", "(((1<<12)+31)>>5)" },
	{ "const int", "NUM_RENDER_PORTAL_BITS", "0(0)" },
	{ "const int", "MAX_EVENT_PARAM_SIZE", "128" },
	{ "int", "GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP", "0" },
	{ "int", "GAME_RELIABLE_MESSAGE_REMAP_DECL", "1" },
	{ "int", "GAME_RELIABLE_MESSAGE_SPAWN_PLAYER", "2" },
	{ "int", "GAME_RELIABLE_MESSAGE_DELETE_ENT", "3" },
	{ "int", "GAME_RELIABLE_MESSAGE_CHAT", "4" },
	{ "int", "GAME_RELIABLE_MESSAGE_TCHAT", "5" },
	{ "int", "GAME_RELIABLE_MESSAGE_SOUND_EVENT", "6" },
	{ "int", "GAME_RELIABLE_MESSAGE_SOUND_INDEX", "7" },
	{ "int", "GAME_RELIABLE_MESSAGE_DB", "8" },
	{ "int", "GAME_RELIABLE_MESSAGE_KILL", "9" },
	{ "int", "GAME_RELIABLE_MESSAGE_DROPWEAPON", "10" },
	{ "int", "GAME_RELIABLE_MESSAGE_RESTART", "11" },
	{ "int", "GAME_RELIABLE_MESSAGE_SERVERINFO", "12" },
	{ "int", "GAME_RELIABLE_MESSAGE_TOURNEYLINE", "13" },
	{ "int", "GAME_RELIABLE_MESSAGE_CALLVOTE", "14" },
	{ "int", "GAME_RELIABLE_MESSAGE_CASTVOTE", "15" },
	{ "int", "GAME_RELIABLE_MESSAGE_STARTVOTE", "16" },
	{ "int", "GAME_RELIABLE_MESSAGE_UPDATEVOTE", "17" },
	{ "int", "GAME_RELIABLE_MESSAGE_PORTALSTATES", "18" },
	{ "int", "GAME_RELIABLE_MESSAGE_PORTAL", "19" },
	{ "int", "GAME_RELIABLE_MESSAGE_VCHAT", "20" },
	{ "int", "GAME_RELIABLE_MESSAGE_STARTSTATE", "21" },
	{ "int", "GAME_RELIABLE_MESSAGE_MENU", "22" },
	{ "int", "GAME_RELIABLE_MESSAGE_WARMUPTIME", "23" },
	{ "int", "GAME_RELIABLE_MESSAGE_EVENT", "24" },
	{ "int", "GAMESTATE_UNINITIALIZED", "0" },
	{ "int", "GAMESTATE_NOMAP", "1" },
	{ "int", "GAMESTATE_STARTUP", "2" },
	{ "int", "GAMESTATE_ACTIVE", "3" },
	{ "int", "GAMESTATE_SHUTDOWN", "4" },
	{ "int", "idEventQueue::OUTOFORDER_IGNORE", "0" },
	{ "int", "idEventQueue::OUTOFORDER_DROP", "1" },
	{ "int", "idEventQueue::OUTOFORDER_SORT", "2" },
	{ "int", "SLOWMO_STATE_OFF", "0" },
	{ "int", "SLOWMO_STATE_RAMPUP", "1" },
	{ "int", "SLOWMO_STATE_ON", "2" },
	{ "int", "SLOWMO_STATE_RAMPDOWN", "3" },
	{ ": const static int", "idGameLocal::INITIAL_SPAWN_COUNT", "1" },
	{ "int", "AIM_HIT_NOTHING", "0" },
	{ "int", "AIM_HIT_AI", "1" },
	{ "int", "AIM_HIT_CIVILIAN", "2" },
	{ ": const static int", "dnGameLocal::INITIAL_SPAWN_COUNT", "1" },
	{ "int", "SND_CHANNEL_ANY", "0" },
	{ "int", "SND_CHANNEL_VOICE", "0" },
	{ "int", "SND_CHANNEL_VOICE2", "1" },
	{ "int", "SND_CHANNEL_BODY", "2" },
	{ "int", "SND_CHANNEL_BODY2", "3" },
	{ "int", "SND_CHANNEL_BODY3", "4" },
	{ "int", "SND_CHANNEL_WEAPON", "5" },
	{ "int", "SND_CHANNEL_ITEM", "6" },
	{ "int", "SND_CHANNEL_HEART", "7" },
	{ "int", "SND_CHANNEL_PDA", "8" },
	{ "int", "SND_CHANNEL_DEMONIC", "9" },
	{ "int", "SND_CHANNEL_RADIO", "10" },
	{ "int", "SND_CHANNEL_AMBIENT", "11" },
	{ "int", "SND_CHANNEL_DAMAGE", "12" },
	{ "const float", "DEFAULT_GRAVITY", "1066.0" },
	{ "const int", "CINEMATIC_SKIP_DELAY", "0(2.0)" },
	{ "int", "SRESULT_OK", "0" },
	{ "int", "SRESULT_ERROR", "1" },
	{ "int", "SRESULT_DONE", "2" },
	{ "int", "SRESULT_DONE_WAIT", "3" },
	{ "int", "SRESULT_WAIT", "4" },
	{ "int", "SRESULT_IDLE", "5" },
	{ "int", "SRESULT_SETSTAGE", "6" },
	{ "int", "SRESULT_DONE_FRAME", "7" },
	{ "int", "SRESULT_SETDELAY", "26" },
	{ "const int", "SFLAG_ONCLEAR", "0(0)" },
	{ "const int", "SFLAG_ONCLEARONLY", "0(1)" },
	{ "int", "FORCEFIELD_UNIFORM", "0" },
	{ "int", "FORCEFIELD_EXPLOSION", "1" },
	{ "int", "FORCEFIELD_IMPLOSION", "2" },
	{ "int", "FORCEFIELD_APPLY_FORCE", "0" },
	{ "int", "FORCEFIELD_APPLY_VELOCITY", "1" },
	{ "int", "FORCEFIELD_APPLY_IMPULSE", "2" },
	{ "int", "MM_OK", "0" },
	{ "int", "MM_SLIDING", "1" },
	{ "int", "MM_BLOCKED", "2" },
	{ "int", "MM_STEPPED", "3" },
	{ "int", "MM_FALLING", "4" },
	{ "int", "PM_NORMAL", "0" },
	{ "int", "PM_DEAD", "1" },
	{ "int", "PM_SPECTATOR", "2" },
	{ "int", "PM_FREEZE", "3" },
	{ "int", "PM_NOCLIP", "4" },
	{ "int", "WATERLEVEL_NONE", "0" },
	{ "int", "WATERLEVEL_FEET", "1" },
	{ "int", "WATERLEVEL_WAIST", "2" },
	{ "int", "WATERLEVEL_HEAD", "3" },
	{ "int", "CONSTRAINT_INVALID", "0" },
	{ "int", "CONSTRAINT_FIXED", "1" },
	{ "int", "CONSTRAINT_BALLANDSOCKETJOINT", "2" },
	{ "int", "CONSTRAINT_UNIVERSALJOINT", "3" },
	{ "int", "CONSTRAINT_HINGE", "4" },
	{ "int", "CONSTRAINT_HINGESTEERING", "5" },
	{ "int", "CONSTRAINT_SLIDER", "6" },
	{ "int", "CONSTRAINT_CYLINDRICALJOINT", "7" },
	{ "int", "CONSTRAINT_LINE", "8" },
	{ "int", "CONSTRAINT_PLANE", "9" },
	{ "int", "CONSTRAINT_SPRING", "10" },
	{ "int", "CONSTRAINT_CONTACT", "11" },
	{ "int", "CONSTRAINT_FRICTION", "12" },
	{ "int", "CONSTRAINT_CONELIMIT", "13" },
	{ "int", "CONSTRAINT_PYRAMIDLIMIT", "14" },
	{ "int", "CONSTRAINT_SUSPENSION", "15" },
	{ "static const int", "idSmokeParticles::MAX_SMOKE_PARTICLES", "10000" },
	{ "static const int", "DELAY_DORMANT_TIME", "3000" },
	{ "int", "TH_ALL", "-1" },
	{ "int", "TH_THINK", "1" },
	{ "int", "TH_PHYSICS", "2" },
	{ "int", "TH_ANIMATE", "4" },
	{ "int", "TH_UPDATEVISUALS", "8" },
	{ "int", "TH_UPDATEPARTICLES", "16" },
	{ "int", "SIG_TOUCH", "0" },
	{ "int", "SIG_USE", "1" },
	{ "int", "SIG_TRIGGER", "2" },
	{ "int", "SIG_REMOVED", "3" },
	{ "int", "SIG_DAMAGE", "4" },
	{ "int", "SIG_BLOCKED", "5" },
	{ "int", "SIG_MOVER_POS1", "6" },
	{ "int", "SIG_MOVER_POS2", "7" },
	{ "int", "SIG_MOVER_1TO2", "8" },
	{ "int", "SIG_MOVER_2TO1", "9" },
	{ "int", "NUM_SIGNALS", "10" },
	{ ": static const int", "idEntity::MAX_PVS_AREAS", "4" },
	{ "int", "idEntity::EVENT_STARTSOUNDSHADER", "0" },
	{ "int", "idEntity::EVENT_STOPSOUNDSHADER", "1" },
	{ "int", "idEntity::EVENT_MAXEVENTS", "2" },
	{ "int", "idAnimatedEntity::EVENT_ADD_DAMAGE_EFFECT", "2" },
	{ "int", "idAnimatedEntity::EVENT_MAXEVENTS", "3" },
	{ ": static const int", "idIK_Walk::MAX_LEGS", "8" },
	{ ": static const int", "idIK_Reach::MAX_ARMS", "2" },
	{ "const int", "GIB_DELAY", "200" },
	{ "int", "idPlayerStart::EVENT_TELEPORTPLAYER", "2" },
	{ "int", "idPlayerStart::EVENT_MAXEVENTS", "3" },
	{ "int", "idProjectile::EVENT_DAMAGE_EFFECT", "2" },
	{ "int", "idProjectile::EVENT_MAXEVENTS", "3" },
	{ "int", "idProjectile::SPAWNED", "0" },
	{ "int", "idProjectile::CREATED", "1" },
	{ "int", "idProjectile::LAUNCHED", "2" },
	{ "int", "idProjectile::FIZZLED", "3" },
	{ "int", "idProjectile::EXPLODED", "4" },
	{ "static const int", "AMMO_NUMTYPES", "16" },
	{ "static const int", "LIGHTID_WORLD_MUZZLE_FLASH", "1" },
	{ "static const int", "LIGHTID_VIEW_MUZZLE_FLASH", "100" },
	{ "int", "idWeapon::EVENT_RELOAD", "2" },
	{ "int", "idWeapon::EVENT_ENDRELOAD", "3" },
	{ "int", "idWeapon::EVENT_CHANGESKIN", "4" },
	{ "int", "idWeapon::EVENT_MAXEVENTS", "5" },
	{ "int", "idLight::EVENT_BECOMEBROKEN", "2" },
	{ "int", "idLight::EVENT_MAXEVENTS", "3" },
	{ "int", "idItem::EVENT_PICKUP", "2" },
	{ "int", "idItem::EVENT_RESPAWN", "3" },
	{ "int", "idItem::EVENT_RESPAWNFX", "4" },
	{ "int", "idItem::EVENT_TAKEFLAG", "5" },
	{ "int", "idItem::EVENT_DROPFLAG", "6" },
	{ "int", "idItem::EVENT_FLAGRETURN", "7" },
	{ "int", "idItem::EVENT_FLAGCAPTURE", "8" },
	{ "int", "idItem::EVENT_MAXEVENTS", "9" },
	{ "const float", "THIRD_PERSON_FOCUS_DISTANCE", "512.0" },
	{ "const int", "LAND_DEFLECT_TIME", "150" },
	{ "const int", "LAND_RETURN_TIME", "300" },
	{ "const int", "FOCUS_TIME", "300" },
	{ "const int", "FOCUS_GUI_TIME", "500" },
	{ "const float", "MIN_BOB_SPEED", "5.0" },
	{ "const int", "MAX_WEAPONS", "32" },
	{ "const int", "DEAD_HEARTRATE", "0" },
	{ "const int", "LOWHEALTH_HEARTRATE_ADJ", "20" },
	{ "const int", "DYING_HEARTRATE", "30" },
	{ "const int", "BASE_HEARTRATE", "70" },
	{ "const int", "ZEROSTAMINA_HEARTRATE", "115" },
	{ "const int", "MAX_HEARTRATE", "130" },
	{ "const int", "ZERO_VOLUME", "-40" },
	{ "const int", "DMG_VOLUME", "5" },
	{ "const int", "DEATH_VOLUME", "15" },
	{ "const int", "SAVING_THROW_TIME", "5000" },
	{ "const int", "ASYNC_PLAYER_INV_AMMO_BITS", "0(999)" },
	{ "const int", "ASYNC_PLAYER_INV_CLIP_BITS", "-7" },
	{ "int", "BERSERK", "0" },
	{ "int", "INVISIBILITY", "1" },
	{ "int", "MEGAHEALTH", "2" },
	{ "int", "ADRENALINE", "3" },
	{ "int", "INVULNERABILITY", "4" },
	{ "int", "HELLTIME", "5" },
	{ "int", "ENVIROSUIT", "6" },
	{ "int", "ENVIROTIME", "7" },
	{ "int", "MAX_POWERUPS", "8" },
	{ "int", "SPEED", "0" },
	{ "int", "PROJECTILE_DAMAGE", "1" },
	{ "int", "MELEE_DAMAGE", "2" },
	{ "int", "MELEE_DISTANCE", "3" },
	{ "int", "INFLUENCE_NONE", "0" },
	{ "int", "INFLUENCE_LEVEL1", "1" },
	{ "int", "INFLUENCE_LEVEL2", "2" },
	{ "int", "INFLUENCE_LEVEL3", "3" },
	{ "int", "idPlayer::EVENT_IMPULSE", "2" },
	{ "int", "idPlayer::EVENT_EXIT_TELEPORTER", "3" },
	{ "int", "idPlayer::EVENT_ABORT_TELEPORTER", "4" },
	{ "int", "idPlayer::EVENT_POWERUP", "5" },
	{ "int", "idPlayer::EVENT_SPECTATE", "6" },
	{ "int", "idPlayer::EVENT_PICKUPNAME", "7" },
	{ "int", "idPlayer::EVENT_MAXEVENTS", "8" },
	{ "static const int", "idPlayer::NUM_LOGGED_VIEW_ANGLES", "64" },
	{ "static const int", "idPlayer::NUM_LOGGED_ACCELS", "16" },
	{ "int", "idMover::ACCELERATION_STAGE", "0" },
	{ "int", "idMover::LINEAR_STAGE", "1" },
	{ "int", "idMover::DECELERATION_STAGE", "2" },
	{ "int", "idMover::FINISHED_STAGE", "3" },
	{ "int", "idMover::MOVER_NONE", "0" },
	{ "int", "idMover::MOVER_ROTATING", "1" },
	{ "int", "idMover::MOVER_MOVING", "2" },
	{ "int", "idMover::MOVER_SPLINE", "3" },
	{ "int", "idMover::DIR_UP", "-1" },
	{ "int", "idMover::DIR_DOWN", "-2" },
	{ "int", "idMover::DIR_LEFT", "-3" },
	{ "int", "idMover::DIR_RIGHT", "-4" },
	{ "int", "idMover::DIR_FORWARD", "-5" },
	{ "int", "idMover::DIR_BACK", "-6" },
	{ "int", "idMover::DIR_REL_UP", "-7" },
	{ "int", "idMover::DIR_REL_DOWN", "-8" },
	{ "int", "idMover::DIR_REL_LEFT", "-9" },
	{ "int", "idMover::DIR_REL_RIGHT", "-10" },
	{ "int", "idMover::DIR_REL_FORWARD", "-11" },
	{ "int", "idMover::DIR_REL_BACK", "-12" },
	{ "int", "idElevator::INIT", "0" },
	{ "int", "idElevator::IDLE", "1" },
	{ "int", "idElevator::WAITING_ON_DOORS", "2" },
	{ "int", "MOVER_POS1", "0" },
	{ "int", "MOVER_POS2", "1" },
	{ "int", "MOVER_1TO2", "2" },
	{ "int", "MOVER_2TO1", "3" },
	{ "int", "idExplodingBarrel::EVENT_EXPLODE", "2" },
	{ "int", "idExplodingBarrel::EVENT_MAXEVENTS", "3" },
	{ "int", "idExplodingBarrel::NORMAL", "0" },
	{ "int", "idExplodingBarrel::BURNING", "1" },
	{ "int", "idExplodingBarrel::BURNEXPIRED", "2" },
	{ "int", "idExplodingBarrel::EXPLODING", "3" },
	{ "int", "idSecurityCamera::SCANNING", "0" },
	{ "int", "idSecurityCamera::LOSINGINTEREST", "1" },
	{ "int", "idSecurityCamera::ALERT", "2" },
	{ "int", "idSecurityCamera::ACTIVATED", "3" },
	{ "int", "idBrittleFracture::EVENT_PROJECT_DECAL", "2" },
	{ "int", "idBrittleFracture::EVENT_SHATTER", "3" },
	{ "int", "idBrittleFracture::EVENT_MAXEVENTS", "4" },
	{ "const float", "SQUARE_ROOT_OF_2", "1.414213562" },
	{ "const float", "AI_TURN_PREDICTION", "0.2" },
	{ "const float", "AI_TURN_SCALE", "60.0" },
	{ "const float", "AI_SEEK_PREDICTION", "0.3" },
	{ "const float", "AI_FLY_DAMPENING", "0.15" },
	{ "const float", "AI_HEARING_RANGE", "2048.0" },
	{ "const int", "DEFAULT_FLY_OFFSET", "68" },
	{ "int", "MOVETYPE_DEAD", "0" },
	{ "int", "MOVETYPE_ANIM", "1" },
	{ "int", "MOVETYPE_SLIDE", "2" },
	{ "int", "MOVETYPE_FLY", "3" },
	{ "int", "MOVETYPE_STATIC", "4" },
	{ "int", "NUM_MOVETYPES", "5" },
	{ "int", "MOVE_NONE", "0" },
	{ "int", "MOVE_FACE_ENEMY", "1" },
	{ "int", "MOVE_FACE_ENTITY", "2" },
	{ "int", "NUM_NONMOVING_COMMANDS", "3" },
	{ "int", "MOVE_TO_ENEMY", "3" },
	{ "int", "MOVE_TO_ENEMYHEIGHT", "4" },
	{ "int", "MOVE_TO_ENTITY", "5" },
	{ "int", "MOVE_OUT_OF_RANGE", "6" },
	{ "int", "MOVE_TO_COVER", "7" },
	{ "int", "MOVE_TO_POSITION", "8" },
	{ "int", "MOVE_TO_POSITION_DIRECT", "9" },
	{ "int", "MOVE_SLIDE_TO_POSITION", "10" },
	{ "int", "MOVE_WANDER", "11" },
	{ "int", "NUM_MOVE_COMMANDS", "12" },
	{ "int", "TALK_NEVER", "0" },
	{ "int", "TALK_DEAD", "1" },
	{ "int", "TALK_OK", "2" },
	{ "int", "TALK_BUSY", "3" },
	{ "int", "NUM_TALK_STATES", "4" },
	{ "int", "MOVE_STATUS_DONE", "0" },
	{ "int", "MOVE_STATUS_MOVING", "1" },
	{ "int", "MOVE_STATUS_WAITING", "2" },
	{ "int", "MOVE_STATUS_DEST_NOT_FOUND", "3" },
	{ "int", "MOVE_STATUS_DEST_UNREACHABLE", "4" },
	{ "int", "MOVE_STATUS_BLOCKED_BY_WALL", "5" },
	{ "int", "MOVE_STATUS_BLOCKED_BY_OBJECT", "6" },
	{ "int", "MOVE_STATUS_BLOCKED_BY_ENEMY", "7" },
	{ "int", "MOVE_STATUS_BLOCKED_BY_MONSTER", "8" },
	{ "int", "PIGCOP_IDLE_WAITINGTPLAYER", "0" },
	{ "int", "PIGCOP_IDLE_ROAR", "1" },
	{ "int", "LIZTROOP_IDLE_WAITINGTPLAYER", "0" },
	{ "int", "LIZTROOP_IDLE_ROAR", "1" },
	{ "int", "DN_WEAPON_FEET", "0" },
	{ "int", "DN_WEAPON_PISTOL", "1" },
	{ "int", "DN_WEAPON_SHOTGUN", "2" },
	{ "const char * const", "RESULT_STRING", "<RESULT>" },
	{ "int", "OP_RETURN", "0" },
	{ "int", "OP_UINC_F", "1" },
	{ "int", "OP_UINCP_F", "2" },
	{ "int", "OP_UDEC_F", "3" },
	{ "int", "OP_UDECP_F", "4" },
	{ "int", "OP_COMP_F", "5" },
	{ "int", "OP_MUL_F", "6" },
	{ "int", "OP_MUL_V", "7" },
	{ "int", "OP_MUL_FV", "8" },
	{ "int", "OP_MUL_VF", "9" },
	{ "int", "OP_DIV_F", "10" },
	{ "int", "OP_MOD_F", "11" },
	{ "int", "OP_ADD_F", "12" },
	{ "int", "OP_ADD_V", "13" },
	{ "int", "OP_ADD_S", "14" },
	{ "int", "OP_ADD_FS", "15" },
	{ "int", "OP_ADD_SF", "16" },
	{ "int", "OP_ADD_VS", "17" },
	{ "int", "OP_ADD_SV", "18" },
	{ "int", "OP_SUB_F", "19" },
	{ "int", "OP_SUB_V", "20" },
	{ "int", "OP_EQ_F", "21" },
	{ "int", "OP_EQ_V", "22" },
	{ "int", "OP_EQ_S", "23" },
	{ "int", "OP_EQ_E", "24" },
	{ "int", "OP_EQ_EO", "25" },
	{ "int", "OP_EQ_OE", "26" },
	{ "int", "OP_EQ_OO", "27" },
	{ "int", "OP_NE_F", "28" },
	{ "int", "OP_NE_V", "29" },
	{ "int", "OP_NE_S", "30" },
	{ "int", "OP_NE_E", "31" },
	{ "int", "OP_NE_EO", "32" },
	{ "int", "OP_NE_OE", "33" },
	{ "int", "OP_NE_OO", "34" },
	{ "int", "OP_LE", "35" },
	{ "int", "OP_GE", "36" },
	{ "int", "OP_LT", "37" },
	{ "int", "OP_GT", "38" },
	{ "int", "OP_INDIRECT_F", "39" },
	{ "int", "OP_INDIRECT_V", "40" },
	{ "int", "OP_INDIRECT_S", "41" },
	{ "int", "OP_INDIRECT_ENT", "42" },
	{ "int", "OP_INDIRECT_BOOL", "43" },
	{ "int", "OP_INDIRECT_OBJ", "44" },
	{ "int", "OP_ADDRESS", "45" },
	{ "int", "OP_EVENTCALL", "46" },
	{ "int", "OP_OBJECTCALL", "47" },
	{ "int", "OP_SYSCALL", "48" },
	{ "int", "OP_STORE_F", "49" },
	{ "int", "OP_STORE_V", "50" },
	{ "int", "OP_STORE_S", "51" },
	{ "int", "OP_STORE_ENT", "52" },
	{ "int", "OP_STORE_BOOL", "53" },
	{ "int", "OP_STORE_OBJENT", "54" },
	{ "int", "OP_STORE_OBJ", "55" },
	{ "int", "OP_STORE_ENTOBJ", "56" },
	{ "int", "OP_STORE_FTOS", "57" },
	{ "int", "OP_STORE_BTOS", "58" },
	{ "int", "OP_STORE_VTOS", "59" },
	{ "int", "OP_STORE_FTOBOOL", "60" },
	{ "int", "OP_STORE_BOOLTOF", "61" },
	{ "int", "OP_STOREP_F", "62" },
	{ "int", "OP_STOREP_V", "63" },
	{ "int", "OP_STOREP_S", "64" },
	{ "int", "OP_STOREP_ENT", "65" },
	{ "int", "OP_STOREP_FLD", "66" },
	{ "int", "OP_STOREP_BOOL", "67" },
	{ "int", "OP_STOREP_OBJ", "68" },
	{ "int", "OP_STOREP_OBJENT", "69" },
	{ "int", "OP_STOREP_FTOS", "70" },
	{ "int", "OP_STOREP_BTOS", "71" },
	{ "int", "OP_STOREP_VTOS", "72" },
	{ "int", "OP_STOREP_FTOBOOL", "73" },
	{ "int", "OP_STOREP_BOOLTOF", "74" },
	{ "int", "OP_UMUL_F", "75" },
	{ "int", "OP_UMUL_V", "76" },
	{ "int", "OP_UDIV_F", "77" },
	{ "int", "OP_UDIV_V", "78" },
	{ "int", "OP_UMOD_F", "79" },
	{ "int", "OP_UADD_F", "80" },
	{ "int", "OP_UADD_V", "81" },
	{ "int", "OP_USUB_F", "82" },
	{ "int", "OP_USUB_V", "83" },
	{ "int", "OP_UAND_F", "84" },
	{ "int", "OP_UOR_F", "85" },
	{ "int", "OP_NOT_BOOL", "86" },
	{ "int", "OP_NOT_F", "87" },
	{ "int", "OP_NOT_V", "88" },
	{ "int", "OP_NOT_S", "89" },
	{ "int", "OP_NOT_ENT", "90" },
	{ "int", "OP_NEG_F", "91" },
	{ "int", "OP_NEG_V", "92" },
	{ "int", "OP_INT_F", "93" },
	{ "int", "OP_IF", "94" },
	{ "int", "OP_IFNOT", "95" },
	{ "int", "OP_CALL", "96" },
	{ "int", "OP_THREAD", "97" },
	{ "int", "OP_OBJTHREAD", "98" },
	{ "int", "OP_PUSH_F", "99" },
	{ "int", "OP_PUSH_V", "100" },
	{ "int", "OP_PUSH_S", "101" },
	{ "int", "OP_PUSH_ENT", "102" },
	{ "int", "OP_PUSH_OBJ", "103" },
	{ "int", "OP_PUSH_OBJENT", "104" },
	{ "int", "OP_PUSH_FTOS", "105" },
	{ "int", "OP_PUSH_BTOF", "106" },
	{ "int", "OP_PUSH_FTOB", "107" },
	{ "int", "OP_PUSH_VTOS", "108" },
	{ "int", "OP_PUSH_BTOS", "109" },
	{ "int", "OP_GOTO", "110" },
	{ "int", "OP_AND", "111" },
	{ "int", "OP_AND_BOOLF", "112" },
	{ "int", "OP_AND_FBOOL", "113" },
	{ "int", "OP_AND_BOOLBOOL", "114" },
	{ "int", "OP_OR", "115" },
	{ "int", "OP_OR_BOOLF", "116" },
	{ "int", "OP_OR_FBOOL", "117" },
	{ "int", "OP_OR_BOOLBOOL", "118" },
	{ "int", "OP_BITAND", "119" },
	{ "int", "OP_BITOR", "120" },
	{ "int", "OP_BREAK", "121" },
	{ "int", "OP_CONTINUE", "122" },
	{ "int", "NUM_OPCODES", "123" },
	{ NULL, NULL, NULL }
};

static enumValueInfo_t etype_t_typeInfo[] = {
	{ "ev_error", -1 },
	{ "ev_void", 0 },
	{ "ev_scriptevent", 1 },
	{ "ev_namespace", 2 },
	{ "ev_string", 3 },
	{ "ev_float", 4 },
	{ "ev_vector", 5 },
	{ "ev_entity", 6 },
	{ "ev_field", 7 },
	{ "ev_function", 8 },
	{ "ev_virtualfunction", 9 },
	{ "ev_pointer", 10 },
	{ "ev_object", 11 },
	{ "ev_jumpoffset", 12 },
	{ "ev_argsize", 13 },
	{ "ev_boolean", 14 },
	{ NULL, 0 }
};

static enumValueInfo_t idVarDef_initialized_t_typeInfo[] = {
	{ "uninitialized", 0 },
	{ "initializedVariable", 1 },
	{ "initializedConstant", 2 },
	{ "stackVariable", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t jointModTransform_t_typeInfo[] = {
	{ "JOINTMOD_NONE", 0 },
	{ "JOINTMOD_LOCAL", 1 },
	{ "JOINTMOD_LOCAL_OVERRIDE", 2 },
	{ "JOINTMOD_WORLD", 3 },
	{ "JOINTMOD_WORLD_OVERRIDE", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t frameCommandType_t_typeInfo[] = {
	{ "FC_SCRIPTFUNCTION", 0 },
	{ "FC_SCRIPTFUNCTIONOBJECT", 1 },
	{ "FC_EVENTFUNCTION", 2 },
	{ "FC_SOUND", 3 },
	{ "FC_SOUND_VOICE", 4 },
	{ "FC_SOUND_VOICE2", 5 },
	{ "FC_SOUND_BODY", 6 },
	{ "FC_SOUND_BODY2", 7 },
	{ "FC_SOUND_BODY3", 8 },
	{ "FC_SOUND_WEAPON", 9 },
	{ "FC_SOUND_ITEM", 10 },
	{ "FC_SOUND_GLOBAL", 11 },
	{ "FC_SOUND_CHATTER", 12 },
	{ "FC_SKIN", 13 },
	{ "FC_TRIGGER", 14 },
	{ "FC_TRIGGER_SMOKE_PARTICLE", 15 },
	{ "FC_MELEE", 16 },
	{ "FC_DIRECTDAMAGE", 17 },
	{ "FC_BEGINATTACK", 18 },
	{ "FC_ENDATTACK", 19 },
	{ "FC_MUZZLEFLASH", 20 },
	{ "FC_CREATEMISSILE", 21 },
	{ "FC_LAUNCHMISSILE", 22 },
	{ "FC_FIREMISSILEATTARGET", 23 },
	{ "FC_FOOTSTEP", 24 },
	{ "FC_LEFTFOOT", 25 },
	{ "FC_RIGHTFOOT", 26 },
	{ "FC_ENABLE_EYE_FOCUS", 27 },
	{ "FC_DISABLE_EYE_FOCUS", 28 },
	{ "FC_FX", 29 },
	{ "FC_DISABLE_GRAVITY", 30 },
	{ "FC_ENABLE_GRAVITY", 31 },
	{ "FC_JUMP", 32 },
	{ "FC_ENABLE_CLIP", 33 },
	{ "FC_DISABLE_CLIP", 34 },
	{ "FC_ENABLE_WALK_IK", 35 },
	{ "FC_DISABLE_WALK_IK", 36 },
	{ "FC_ENABLE_LEG_IK", 37 },
	{ "FC_DISABLE_LEG_IK", 38 },
	{ "FC_RECORDDEMO", 39 },
	{ "FC_AVIGAME", 40 },
	{ "FC_LAUNCH_PROJECTILE", 41 },
	{ "FC_TRIGGER_FX", 42 },
	{ "FC_START_EMITTER", 43 },
	{ "FC_STOP_EMITTER", 44 },
	{ NULL, 0 }
};

static enumValueInfo_t AFJointModType_t_typeInfo[] = {
	{ "AF_JOINTMOD_AXIS", 0 },
	{ "AF_JOINTMOD_ORIGIN", 1 },
	{ "AF_JOINTMOD_BOTH", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t pvsType_t_typeInfo[] = {
	{ "PVS_NORMAL", 0 },
	{ "PVS_ALL_PORTALS_OPEN", 1 },
	{ "PVS_CONNECTED_AREAS", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t gameType_t_typeInfo[] = {
	{ "GAME_SP", 0 },
	{ "GAME_DM", 1 },
	{ "GAME_TOURNEY", 2 },
	{ "GAME_TDM", 3 },
	{ "GAME_LASTMAN", 4 },
	{ "GAME_CTF", 5 },
	{ "GAME_COUNT", 6 },
	{ NULL, 0 }
};

static enumValueInfo_t flagStatus_t_typeInfo[] = {
	{ "FLAGSTATUS_INBASE", 0 },
	{ "FLAGSTATUS_TAKEN", 1 },
	{ "FLAGSTATUS_STRAY", 2 },
	{ "FLAGSTATUS_NONE", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t playerVote_t_typeInfo[] = {
	{ "PLAYER_VOTE_NONE", 0 },
	{ "PLAYER_VOTE_NO", 1 },
	{ "PLAYER_VOTE_YES", 2 },
	{ "PLAYER_VOTE_WAIT", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t snd_evt_t_typeInfo[] = {
	{ "SND_YOUWIN", 0 },
	{ "SND_YOULOSE", 1 },
	{ "SND_FIGHT", 2 },
	{ "SND_VOTE", 3 },
	{ "SND_VOTE_PASSED", 4 },
	{ "SND_VOTE_FAILED", 5 },
	{ "SND_THREE", 6 },
	{ "SND_TWO", 7 },
	{ "SND_ONE", 8 },
	{ "SND_SUDDENDEATH", 9 },
	{ "SND_FLAG_CAPTURED_YOURS", 10 },
	{ "SND_FLAG_CAPTURED_THEIRS", 11 },
	{ "SND_FLAG_RETURN", 12 },
	{ "SND_FLAG_TAKEN_YOURS", 13 },
	{ "SND_FLAG_TAKEN_THEIRS", 14 },
	{ "SND_FLAG_DROPPED_YOURS", 15 },
	{ "SND_FLAG_DROPPED_THEIRS", 16 },
	{ "SND_COUNT", 17 },
	{ NULL, 0 }
};

static enumValueInfo_t idMultiplayerGame_gameState_t_typeInfo[] = {
	{ "INACTIVE", 0 },
	{ "WARMUP", 1 },
	{ "COUNTDOWN", 2 },
	{ "GAMEON", 3 },
	{ "SUDDENDEATH", 4 },
	{ "GAMEREVIEW", 5 },
	{ "NEXTGAME", 6 },
	{ "STATE_COUNT", 7 },
	{ NULL, 0 }
};

static enumValueInfo_t idMultiplayerGame_msg_evt_t_typeInfo[] = {
	{ "MSG_SUICIDE", 0 },
	{ "MSG_KILLED", 1 },
	{ "MSG_KILLEDTEAM", 2 },
	{ "MSG_DIED", 3 },
	{ "MSG_VOTE", 4 },
	{ "MSG_VOTEPASSED", 5 },
	{ "MSG_VOTEFAILED", 6 },
	{ "MSG_SUDDENDEATH", 7 },
	{ "MSG_FORCEREADY", 8 },
	{ "MSG_JOINEDSPEC", 9 },
	{ "MSG_TIMELIMIT", 10 },
	{ "MSG_FRAGLIMIT", 11 },
	{ "MSG_TELEFRAGGED", 12 },
	{ "MSG_JOINTEAM", 13 },
	{ "MSG_HOLYSHIT", 14 },
	{ "MSG_POINTLIMIT", 15 },
	{ "MSG_FLAGTAKEN", 16 },
	{ "MSG_FLAGDROP", 17 },
	{ "MSG_FLAGRETURN", 18 },
	{ "MSG_FLAGCAPTURE", 19 },
	{ "MSG_SCOREUPDATE", 20 },
	{ "MSG_COUNT", 21 },
	{ NULL, 0 }
};

static enumValueInfo_t idMultiplayerGame_vote_flags_t_typeInfo[] = {
	{ "VOTE_RESTART", 0 },
	{ "VOTE_TIMELIMIT", 1 },
	{ "VOTE_FRAGLIMIT", 2 },
	{ "VOTE_GAMETYPE", 3 },
	{ "VOTE_KICK", 4 },
	{ "VOTE_MAP", 5 },
	{ "VOTE_SPECTATORS", 6 },
	{ "VOTE_NEXTMAP", 7 },
	{ "VOTE_COUNT", 8 },
	{ "VOTE_NONE", 9 },
	{ NULL, 0 }
};

static enumValueInfo_t idMultiplayerGame_vote_result_t_typeInfo[] = {
	{ "VOTE_UPDATE", 0 },
	{ "VOTE_FAILED", 1 },
	{ "VOTE_PASSED", 2 },
	{ "VOTE_ABORTED", 3 },
	{ "VOTE_RESET", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t enum_14_typeInfo[] = {
	{ "GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP", 0 },
	{ "GAME_RELIABLE_MESSAGE_REMAP_DECL", 1 },
	{ "GAME_RELIABLE_MESSAGE_SPAWN_PLAYER", 2 },
	{ "GAME_RELIABLE_MESSAGE_DELETE_ENT", 3 },
	{ "GAME_RELIABLE_MESSAGE_CHAT", 4 },
	{ "GAME_RELIABLE_MESSAGE_TCHAT", 5 },
	{ "GAME_RELIABLE_MESSAGE_SOUND_EVENT", 6 },
	{ "GAME_RELIABLE_MESSAGE_SOUND_INDEX", 7 },
	{ "GAME_RELIABLE_MESSAGE_DB", 8 },
	{ "GAME_RELIABLE_MESSAGE_KILL", 9 },
	{ "GAME_RELIABLE_MESSAGE_DROPWEAPON", 10 },
	{ "GAME_RELIABLE_MESSAGE_RESTART", 11 },
	{ "GAME_RELIABLE_MESSAGE_SERVERINFO", 12 },
	{ "GAME_RELIABLE_MESSAGE_TOURNEYLINE", 13 },
	{ "GAME_RELIABLE_MESSAGE_CALLVOTE", 14 },
	{ "GAME_RELIABLE_MESSAGE_CASTVOTE", 15 },
	{ "GAME_RELIABLE_MESSAGE_STARTVOTE", 16 },
	{ "GAME_RELIABLE_MESSAGE_UPDATEVOTE", 17 },
	{ "GAME_RELIABLE_MESSAGE_PORTALSTATES", 18 },
	{ "GAME_RELIABLE_MESSAGE_PORTAL", 19 },
	{ "GAME_RELIABLE_MESSAGE_VCHAT", 20 },
	{ "GAME_RELIABLE_MESSAGE_STARTSTATE", 21 },
	{ "GAME_RELIABLE_MESSAGE_MENU", 22 },
	{ "GAME_RELIABLE_MESSAGE_WARMUPTIME", 23 },
	{ "GAME_RELIABLE_MESSAGE_EVENT", 24 },
	{ NULL, 0 }
};

static enumValueInfo_t gameState_t_typeInfo[] = {
	{ "GAMESTATE_UNINITIALIZED", 0 },
	{ "GAMESTATE_NOMAP", 1 },
	{ "GAMESTATE_STARTUP", 2 },
	{ "GAMESTATE_ACTIVE", 3 },
	{ "GAMESTATE_SHUTDOWN", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t idEventQueue_outOfOrderBehaviour_t_typeInfo[] = {
	{ "OUTOFORDER_IGNORE", 0 },
	{ "OUTOFORDER_DROP", 1 },
	{ "OUTOFORDER_SORT", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t slowmoState_t_typeInfo[] = {
	{ "SLOWMO_STATE_OFF", 0 },
	{ "SLOWMO_STATE_RAMPUP", 1 },
	{ "SLOWMO_STATE_ON", 2 },
	{ "SLOWMO_STATE_RAMPDOWN", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t DnAimHitType_t_typeInfo[] = {
	{ "AIM_HIT_NOTHING", 0 },
	{ "AIM_HIT_AI", 1 },
	{ "AIM_HIT_CIVILIAN", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t gameSoundChannel_t_typeInfo[] = {
	{ "SND_CHANNEL_ANY", 0 },
	{ "SND_CHANNEL_VOICE", 0 },
	{ "SND_CHANNEL_VOICE2", 1 },
	{ "SND_CHANNEL_BODY", 2 },
	{ "SND_CHANNEL_BODY2", 3 },
	{ "SND_CHANNEL_BODY3", 4 },
	{ "SND_CHANNEL_WEAPON", 5 },
	{ "SND_CHANNEL_ITEM", 6 },
	{ "SND_CHANNEL_HEART", 7 },
	{ "SND_CHANNEL_PDA", 8 },
	{ "SND_CHANNEL_DEMONIC", 9 },
	{ "SND_CHANNEL_RADIO", 10 },
	{ "SND_CHANNEL_AMBIENT", 11 },
	{ "SND_CHANNEL_DAMAGE", 12 },
	{ NULL, 0 }
};

static enumValueInfo_t stateResult_t_typeInfo[] = {
	{ "SRESULT_OK", 0 },
	{ "SRESULT_ERROR", 1 },
	{ "SRESULT_DONE", 2 },
	{ "SRESULT_DONE_WAIT", 3 },
	{ "SRESULT_WAIT", 4 },
	{ "SRESULT_IDLE", 5 },
	{ "SRESULT_SETSTAGE", 6 },
	{ "SRESULT_DONE_FRAME", 7 },
	{ "SRESULT_SETDELAY", 26 },
	{ NULL, 0 }
};

static enumValueInfo_t forceFieldType_typeInfo[] = {
	{ "FORCEFIELD_UNIFORM", 0 },
	{ "FORCEFIELD_EXPLOSION", 1 },
	{ "FORCEFIELD_IMPLOSION", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t forceFieldApplyType_typeInfo[] = {
	{ "FORCEFIELD_APPLY_FORCE", 0 },
	{ "FORCEFIELD_APPLY_VELOCITY", 1 },
	{ "FORCEFIELD_APPLY_IMPULSE", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t monsterMoveResult_t_typeInfo[] = {
	{ "MM_OK", 0 },
	{ "MM_SLIDING", 1 },
	{ "MM_BLOCKED", 2 },
	{ "MM_STEPPED", 3 },
	{ "MM_FALLING", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t pmtype_t_typeInfo[] = {
	{ "PM_NORMAL", 0 },
	{ "PM_DEAD", 1 },
	{ "PM_SPECTATOR", 2 },
	{ "PM_FREEZE", 3 },
	{ "PM_NOCLIP", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t waterLevel_t_typeInfo[] = {
	{ "WATERLEVEL_NONE", 0 },
	{ "WATERLEVEL_FEET", 1 },
	{ "WATERLEVEL_WAIST", 2 },
	{ "WATERLEVEL_HEAD", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t constraintType_t_typeInfo[] = {
	{ "CONSTRAINT_INVALID", 0 },
	{ "CONSTRAINT_FIXED", 1 },
	{ "CONSTRAINT_BALLANDSOCKETJOINT", 2 },
	{ "CONSTRAINT_UNIVERSALJOINT", 3 },
	{ "CONSTRAINT_HINGE", 4 },
	{ "CONSTRAINT_HINGESTEERING", 5 },
	{ "CONSTRAINT_SLIDER", 6 },
	{ "CONSTRAINT_CYLINDRICALJOINT", 7 },
	{ "CONSTRAINT_LINE", 8 },
	{ "CONSTRAINT_PLANE", 9 },
	{ "CONSTRAINT_SPRING", 10 },
	{ "CONSTRAINT_CONTACT", 11 },
	{ "CONSTRAINT_FRICTION", 12 },
	{ "CONSTRAINT_CONELIMIT", 13 },
	{ "CONSTRAINT_PYRAMIDLIMIT", 14 },
	{ "CONSTRAINT_SUSPENSION", 15 },
	{ NULL, 0 }
};

static enumValueInfo_t enum_27_typeInfo[] = {
	{ "TH_ALL", -1 },
	{ "TH_THINK", 1 },
	{ "TH_PHYSICS", 2 },
	{ "TH_ANIMATE", 4 },
	{ "TH_UPDATEVISUALS", 8 },
	{ "TH_UPDATEPARTICLES", 16 },
	{ NULL, 0 }
};

static enumValueInfo_t signalNum_t_typeInfo[] = {
	{ "SIG_TOUCH", 0 },
	{ "SIG_USE", 1 },
	{ "SIG_TRIGGER", 2 },
	{ "SIG_REMOVED", 3 },
	{ "SIG_DAMAGE", 4 },
	{ "SIG_BLOCKED", 5 },
	{ "SIG_MOVER_POS1", 6 },
	{ "SIG_MOVER_POS2", 7 },
	{ "SIG_MOVER_1TO2", 8 },
	{ "SIG_MOVER_2TO1", 9 },
	{ "NUM_SIGNALS", 10 },
	{ NULL, 0 }
};

static enumValueInfo_t idEntity_enum_29_typeInfo[] = {
	{ "EVENT_STARTSOUNDSHADER", 0 },
	{ "EVENT_STOPSOUNDSHADER", 1 },
	{ "EVENT_MAXEVENTS", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t idAnimatedEntity_enum_30_typeInfo[] = {
	{ "EVENT_ADD_DAMAGE_EFFECT", 2 },
	{ "EVENT_MAXEVENTS", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idPlayerStart_enum_31_typeInfo[] = {
	{ "EVENT_TELEPORTPLAYER", 2 },
	{ "EVENT_MAXEVENTS", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idProjectile_enum_32_typeInfo[] = {
	{ "EVENT_DAMAGE_EFFECT", 2 },
	{ "EVENT_MAXEVENTS", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idProjectile_projectileState_t_typeInfo[] = {
	{ "SPAWNED", 0 },
	{ "CREATED", 1 },
	{ "LAUNCHED", 2 },
	{ "FIZZLED", 3 },
	{ "EXPLODED", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t idWeapon_enum_34_typeInfo[] = {
	{ "EVENT_RELOAD", 2 },
	{ "EVENT_ENDRELOAD", 3 },
	{ "EVENT_CHANGESKIN", 4 },
	{ "EVENT_MAXEVENTS", 5 },
	{ NULL, 0 }
};

static enumValueInfo_t idLight_enum_35_typeInfo[] = {
	{ "EVENT_BECOMEBROKEN", 2 },
	{ "EVENT_MAXEVENTS", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idItem_enum_36_typeInfo[] = {
	{ "EVENT_PICKUP", 2 },
	{ "EVENT_RESPAWN", 3 },
	{ "EVENT_RESPAWNFX", 4 },
	{ "EVENT_TAKEFLAG", 5 },
	{ "EVENT_DROPFLAG", 6 },
	{ "EVENT_FLAGRETURN", 7 },
	{ "EVENT_FLAGCAPTURE", 8 },
	{ "EVENT_MAXEVENTS", 9 },
	{ NULL, 0 }
};

static enumValueInfo_t enum_37_typeInfo[] = {
	{ "BERSERK", 0 },
	{ "INVISIBILITY", 1 },
	{ "MEGAHEALTH", 2 },
	{ "ADRENALINE", 3 },
	{ "INVULNERABILITY", 4 },
	{ "HELLTIME", 5 },
	{ "ENVIROSUIT", 6 },
	{ "ENVIROTIME", 7 },
	{ "MAX_POWERUPS", 8 },
	{ NULL, 0 }
};

static enumValueInfo_t enum_38_typeInfo[] = {
	{ "SPEED", 0 },
	{ "PROJECTILE_DAMAGE", 1 },
	{ "MELEE_DAMAGE", 2 },
	{ "MELEE_DISTANCE", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t enum_39_typeInfo[] = {
	{ "INFLUENCE_NONE", 0 },
	{ "INFLUENCE_LEVEL1", 1 },
	{ "INFLUENCE_LEVEL2", 2 },
	{ "INFLUENCE_LEVEL3", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idPlayer_enum_40_typeInfo[] = {
	{ "EVENT_IMPULSE", 2 },
	{ "EVENT_EXIT_TELEPORTER", 3 },
	{ "EVENT_ABORT_TELEPORTER", 4 },
	{ "EVENT_POWERUP", 5 },
	{ "EVENT_SPECTATE", 6 },
	{ "EVENT_PICKUPNAME", 7 },
	{ "EVENT_MAXEVENTS", 8 },
	{ NULL, 0 }
};

static enumValueInfo_t idMover_moveStage_t_typeInfo[] = {
	{ "ACCELERATION_STAGE", 0 },
	{ "LINEAR_STAGE", 1 },
	{ "DECELERATION_STAGE", 2 },
	{ "FINISHED_STAGE", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idMover_moverCommand_t_typeInfo[] = {
	{ "MOVER_NONE", 0 },
	{ "MOVER_ROTATING", 1 },
	{ "MOVER_MOVING", 2 },
	{ "MOVER_SPLINE", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idMover_moverDir_t_typeInfo[] = {
	{ "DIR_UP", -1 },
	{ "DIR_DOWN", -2 },
	{ "DIR_LEFT", -3 },
	{ "DIR_RIGHT", -4 },
	{ "DIR_FORWARD", -5 },
	{ "DIR_BACK", -6 },
	{ "DIR_REL_UP", -7 },
	{ "DIR_REL_DOWN", -8 },
	{ "DIR_REL_LEFT", -9 },
	{ "DIR_REL_RIGHT", -10 },
	{ "DIR_REL_FORWARD", -11 },
	{ "DIR_REL_BACK", -12 },
	{ NULL, 0 }
};

static enumValueInfo_t idElevator_elevatorState_t_typeInfo[] = {
	{ "INIT", 0 },
	{ "IDLE", 1 },
	{ "WAITING_ON_DOORS", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t moverState_t_typeInfo[] = {
	{ "MOVER_POS1", 0 },
	{ "MOVER_POS2", 1 },
	{ "MOVER_1TO2", 2 },
	{ "MOVER_2TO1", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idExplodingBarrel_enum_46_typeInfo[] = {
	{ "EVENT_EXPLODE", 2 },
	{ "EVENT_MAXEVENTS", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idExplodingBarrel_explode_state_t_typeInfo[] = {
	{ "NORMAL", 0 },
	{ "BURNING", 1 },
	{ "BURNEXPIRED", 2 },
	{ "EXPLODING", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idSecurityCamera_enum_48_typeInfo[] = {
	{ "SCANNING", 0 },
	{ "LOSINGINTEREST", 1 },
	{ "ALERT", 2 },
	{ "ACTIVATED", 3 },
	{ NULL, 0 }
};

static enumValueInfo_t idBrittleFracture_enum_49_typeInfo[] = {
	{ "EVENT_PROJECT_DECAL", 2 },
	{ "EVENT_SHATTER", 3 },
	{ "EVENT_MAXEVENTS", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t moveType_t_typeInfo[] = {
	{ "MOVETYPE_DEAD", 0 },
	{ "MOVETYPE_ANIM", 1 },
	{ "MOVETYPE_SLIDE", 2 },
	{ "MOVETYPE_FLY", 3 },
	{ "MOVETYPE_STATIC", 4 },
	{ "NUM_MOVETYPES", 5 },
	{ NULL, 0 }
};

static enumValueInfo_t moveCommand_t_typeInfo[] = {
	{ "MOVE_NONE", 0 },
	{ "MOVE_FACE_ENEMY", 1 },
	{ "MOVE_FACE_ENTITY", 2 },
	{ "NUM_NONMOVING_COMMANDS", 3 },
	{ "MOVE_TO_ENEMY", 3 },
	{ "MOVE_TO_ENEMYHEIGHT", 4 },
	{ "MOVE_TO_ENTITY", 5 },
	{ "MOVE_OUT_OF_RANGE", 6 },
	{ "MOVE_TO_COVER", 7 },
	{ "MOVE_TO_POSITION", 8 },
	{ "MOVE_TO_POSITION_DIRECT", 9 },
	{ "MOVE_SLIDE_TO_POSITION", 10 },
	{ "MOVE_WANDER", 11 },
	{ "NUM_MOVE_COMMANDS", 12 },
	{ NULL, 0 }
};

static enumValueInfo_t talkState_t_typeInfo[] = {
	{ "TALK_NEVER", 0 },
	{ "TALK_DEAD", 1 },
	{ "TALK_OK", 2 },
	{ "TALK_BUSY", 3 },
	{ "NUM_TALK_STATES", 4 },
	{ NULL, 0 }
};

static enumValueInfo_t moveStatus_t_typeInfo[] = {
	{ "MOVE_STATUS_DONE", 0 },
	{ "MOVE_STATUS_MOVING", 1 },
	{ "MOVE_STATUS_WAITING", 2 },
	{ "MOVE_STATUS_DEST_NOT_FOUND", 3 },
	{ "MOVE_STATUS_DEST_UNREACHABLE", 4 },
	{ "MOVE_STATUS_BLOCKED_BY_WALL", 5 },
	{ "MOVE_STATUS_BLOCKED_BY_OBJECT", 6 },
	{ "MOVE_STATUS_BLOCKED_BY_ENEMY", 7 },
	{ "MOVE_STATUS_BLOCKED_BY_MONSTER", 8 },
	{ NULL, 0 }
};

static enumValueInfo_t PIGCOP_IDLE_STATE_typeInfo[] = {
	{ "PIGCOP_IDLE_WAITINGTPLAYER", 0 },
	{ "PIGCOP_IDLE_ROAR", 1 },
	{ NULL, 0 }
};

static enumValueInfo_t LIZTROOP_IDLE_STATE_typeInfo[] = {
	{ "LIZTROOP_IDLE_WAITINGTPLAYER", 0 },
	{ "LIZTROOP_IDLE_ROAR", 1 },
	{ NULL, 0 }
};

static enumValueInfo_t dnWeapons_typeInfo[] = {
	{ "DN_WEAPON_FEET", 0 },
	{ "DN_WEAPON_PISTOL", 1 },
	{ "DN_WEAPON_SHOTGUN", 2 },
	{ NULL, 0 }
};

static enumValueInfo_t enum_57_typeInfo[] = {
	{ "OP_RETURN", 0 },
	{ "OP_UINC_F", 1 },
	{ "OP_UINCP_F", 2 },
	{ "OP_UDEC_F", 3 },
	{ "OP_UDECP_F", 4 },
	{ "OP_COMP_F", 5 },
	{ "OP_MUL_F", 6 },
	{ "OP_MUL_V", 7 },
	{ "OP_MUL_FV", 8 },
	{ "OP_MUL_VF", 9 },
	{ "OP_DIV_F", 10 },
	{ "OP_MOD_F", 11 },
	{ "OP_ADD_F", 12 },
	{ "OP_ADD_V", 13 },
	{ "OP_ADD_S", 14 },
	{ "OP_ADD_FS", 15 },
	{ "OP_ADD_SF", 16 },
	{ "OP_ADD_VS", 17 },
	{ "OP_ADD_SV", 18 },
	{ "OP_SUB_F", 19 },
	{ "OP_SUB_V", 20 },
	{ "OP_EQ_F", 21 },
	{ "OP_EQ_V", 22 },
	{ "OP_EQ_S", 23 },
	{ "OP_EQ_E", 24 },
	{ "OP_EQ_EO", 25 },
	{ "OP_EQ_OE", 26 },
	{ "OP_EQ_OO", 27 },
	{ "OP_NE_F", 28 },
	{ "OP_NE_V", 29 },
	{ "OP_NE_S", 30 },
	{ "OP_NE_E", 31 },
	{ "OP_NE_EO", 32 },
	{ "OP_NE_OE", 33 },
	{ "OP_NE_OO", 34 },
	{ "OP_LE", 35 },
	{ "OP_GE", 36 },
	{ "OP_LT", 37 },
	{ "OP_GT", 38 },
	{ "OP_INDIRECT_F", 39 },
	{ "OP_INDIRECT_V", 40 },
	{ "OP_INDIRECT_S", 41 },
	{ "OP_INDIRECT_ENT", 42 },
	{ "OP_INDIRECT_BOOL", 43 },
	{ "OP_INDIRECT_OBJ", 44 },
	{ "OP_ADDRESS", 45 },
	{ "OP_EVENTCALL", 46 },
	{ "OP_OBJECTCALL", 47 },
	{ "OP_SYSCALL", 48 },
	{ "OP_STORE_F", 49 },
	{ "OP_STORE_V", 50 },
	{ "OP_STORE_S", 51 },
	{ "OP_STORE_ENT", 52 },
	{ "OP_STORE_BOOL", 53 },
	{ "OP_STORE_OBJENT", 54 },
	{ "OP_STORE_OBJ", 55 },
	{ "OP_STORE_ENTOBJ", 56 },
	{ "OP_STORE_FTOS", 57 },
	{ "OP_STORE_BTOS", 58 },
	{ "OP_STORE_VTOS", 59 },
	{ "OP_STORE_FTOBOOL", 60 },
	{ "OP_STORE_BOOLTOF", 61 },
	{ "OP_STOREP_F", 62 },
	{ "OP_STOREP_V", 63 },
	{ "OP_STOREP_S", 64 },
	{ "OP_STOREP_ENT", 65 },
	{ "OP_STOREP_FLD", 66 },
	{ "OP_STOREP_BOOL", 67 },
	{ "OP_STOREP_OBJ", 68 },
	{ "OP_STOREP_OBJENT", 69 },
	{ "OP_STOREP_FTOS", 70 },
	{ "OP_STOREP_BTOS", 71 },
	{ "OP_STOREP_VTOS", 72 },
	{ "OP_STOREP_FTOBOOL", 73 },
	{ "OP_STOREP_BOOLTOF", 74 },
	{ "OP_UMUL_F", 75 },
	{ "OP_UMUL_V", 76 },
	{ "OP_UDIV_F", 77 },
	{ "OP_UDIV_V", 78 },
	{ "OP_UMOD_F", 79 },
	{ "OP_UADD_F", 80 },
	{ "OP_UADD_V", 81 },
	{ "OP_USUB_F", 82 },
	{ "OP_USUB_V", 83 },
	{ "OP_UAND_F", 84 },
	{ "OP_UOR_F", 85 },
	{ "OP_NOT_BOOL", 86 },
	{ "OP_NOT_F", 87 },
	{ "OP_NOT_V", 88 },
	{ "OP_NOT_S", 89 },
	{ "OP_NOT_ENT", 90 },
	{ "OP_NEG_F", 91 },
	{ "OP_NEG_V", 92 },
	{ "OP_INT_F", 93 },
	{ "OP_IF", 94 },
	{ "OP_IFNOT", 95 },
	{ "OP_CALL", 96 },
	{ "OP_THREAD", 97 },
	{ "OP_OBJTHREAD", 98 },
	{ "OP_PUSH_F", 99 },
	{ "OP_PUSH_V", 100 },
	{ "OP_PUSH_S", 101 },
	{ "OP_PUSH_ENT", 102 },
	{ "OP_PUSH_OBJ", 103 },
	{ "OP_PUSH_OBJENT", 104 },
	{ "OP_PUSH_FTOS", 105 },
	{ "OP_PUSH_BTOF", 106 },
	{ "OP_PUSH_FTOB", 107 },
	{ "OP_PUSH_VTOS", 108 },
	{ "OP_PUSH_BTOS", 109 },
	{ "OP_GOTO", 110 },
	{ "OP_AND", 111 },
	{ "OP_AND_BOOLF", 112 },
	{ "OP_AND_FBOOL", 113 },
	{ "OP_AND_BOOLBOOL", 114 },
	{ "OP_OR", 115 },
	{ "OP_OR_BOOLF", 116 },
	{ "OP_OR_FBOOL", 117 },
	{ "OP_OR_BOOLBOOL", 118 },
	{ "OP_BITAND", 119 },
	{ "OP_BITOR", 120 },
	{ "OP_BREAK", 121 },
	{ "OP_CONTINUE", 122 },
	{ "NUM_OPCODES", 123 },
	{ NULL, 0 }
};

static enumTypeInfo_t enumTypeInfo[] = {
	{ "etype_t", etype_t_typeInfo },
	{ "idVarDef::initialized_t", idVarDef_initialized_t_typeInfo },
	{ "jointModTransform_t", jointModTransform_t_typeInfo },
	{ "frameCommandType_t", frameCommandType_t_typeInfo },
	{ "AFJointModType_t", AFJointModType_t_typeInfo },
	{ "pvsType_t", pvsType_t_typeInfo },
	{ "gameType_t", gameType_t_typeInfo },
	{ "flagStatus_t", flagStatus_t_typeInfo },
	{ "playerVote_t", playerVote_t_typeInfo },
	{ "snd_evt_t", snd_evt_t_typeInfo },
	{ "idMultiplayerGame::gameState_t", idMultiplayerGame_gameState_t_typeInfo },
	{ "idMultiplayerGame::msg_evt_t", idMultiplayerGame_msg_evt_t_typeInfo },
	{ "idMultiplayerGame::vote_flags_t", idMultiplayerGame_vote_flags_t_typeInfo },
	{ "idMultiplayerGame::vote_result_t", idMultiplayerGame_vote_result_t_typeInfo },
	{ "enum_14", enum_14_typeInfo },
	{ "gameState_t", gameState_t_typeInfo },
	{ "idEventQueue::outOfOrderBehaviour_t", idEventQueue_outOfOrderBehaviour_t_typeInfo },
	{ "slowmoState_t", slowmoState_t_typeInfo },
	{ "DnAimHitType_t", DnAimHitType_t_typeInfo },
	{ "gameSoundChannel_t", gameSoundChannel_t_typeInfo },
	{ "stateResult_t", stateResult_t_typeInfo },
	{ "forceFieldType", forceFieldType_typeInfo },
	{ "forceFieldApplyType", forceFieldApplyType_typeInfo },
	{ "monsterMoveResult_t", monsterMoveResult_t_typeInfo },
	{ "pmtype_t", pmtype_t_typeInfo },
	{ "waterLevel_t", waterLevel_t_typeInfo },
	{ "constraintType_t", constraintType_t_typeInfo },
	{ "enum_27", enum_27_typeInfo },
	{ "signalNum_t", signalNum_t_typeInfo },
	{ "idEntity::enum_29", idEntity_enum_29_typeInfo },
	{ "idAnimatedEntity::enum_30", idAnimatedEntity_enum_30_typeInfo },
	{ "idPlayerStart::enum_31", idPlayerStart_enum_31_typeInfo },
	{ "idProjectile::enum_32", idProjectile_enum_32_typeInfo },
	{ "idProjectile::projectileState_t", idProjectile_projectileState_t_typeInfo },
	{ "idWeapon::enum_34", idWeapon_enum_34_typeInfo },
	{ "idLight::enum_35", idLight_enum_35_typeInfo },
	{ "idItem::enum_36", idItem_enum_36_typeInfo },
	{ "enum_37", enum_37_typeInfo },
	{ "enum_38", enum_38_typeInfo },
	{ "enum_39", enum_39_typeInfo },
	{ "idPlayer::enum_40", idPlayer_enum_40_typeInfo },
	{ "idMover::moveStage_t", idMover_moveStage_t_typeInfo },
	{ "idMover::moverCommand_t", idMover_moverCommand_t_typeInfo },
	{ "idMover::moverDir_t", idMover_moverDir_t_typeInfo },
	{ "idElevator::elevatorState_t", idElevator_elevatorState_t_typeInfo },
	{ "moverState_t", moverState_t_typeInfo },
	{ "idExplodingBarrel::enum_46", idExplodingBarrel_enum_46_typeInfo },
	{ "idExplodingBarrel::explode_state_t", idExplodingBarrel_explode_state_t_typeInfo },
	{ "idSecurityCamera::enum_48", idSecurityCamera_enum_48_typeInfo },
	{ "idBrittleFracture::enum_49", idBrittleFracture_enum_49_typeInfo },
	{ "moveType_t", moveType_t_typeInfo },
	{ "moveCommand_t", moveCommand_t_typeInfo },
	{ "talkState_t", talkState_t_typeInfo },
	{ "moveStatus_t", moveStatus_t_typeInfo },
	{ "PIGCOP_IDLE_STATE", PIGCOP_IDLE_STATE_typeInfo },
	{ "LIZTROOP_IDLE_STATE", LIZTROOP_IDLE_STATE_typeInfo },
	{ "dnWeapons", dnWeapons_typeInfo },
	{ "enum_57", enum_57_typeInfo },
	{ NULL, NULL }
};

static classVariableInfo_t DnComponent_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t DnMeshComponent_typeInfo[] = {
	{ ": qhandle_t", "renderEntityHandle", (intptr_t)(&((DnMeshComponent *)0)->renderEntityHandle), sizeof( ((DnMeshComponent *)0)->renderEntityHandle ) },
	{ "renderEntity_t", "renderEntityParams", (intptr_t)(&((DnMeshComponent *)0)->renderEntityParams), sizeof( ((DnMeshComponent *)0)->renderEntityParams ) },
	{ "idEntity *", "parentEntity", (intptr_t)(&((DnMeshComponent *)0)->parentEntity), sizeof( ((DnMeshComponent *)0)->parentEntity ) },
	{ "jointHandle_t", "bindJoint", (intptr_t)(&((DnMeshComponent *)0)->bindJoint), sizeof( ((DnMeshComponent *)0)->bindJoint ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnLightComponent_typeInfo[] = {
	{ "renderLight_t", "renderLightParams", (intptr_t)(&((DnLightComponent *)0)->renderLightParams), sizeof( ((DnLightComponent *)0)->renderLightParams ) },
	{ ": qhandle_t", "renderLightHandle", (intptr_t)(&((DnLightComponent *)0)->renderLightHandle), sizeof( ((DnLightComponent *)0)->renderLightHandle ) },
	{ "idEntity *", "parentEntity", (intptr_t)(&((DnLightComponent *)0)->parentEntity), sizeof( ((DnLightComponent *)0)->parentEntity ) },
	{ "jointHandle_t", "bindJoint", (intptr_t)(&((DnLightComponent *)0)->bindJoint), sizeof( ((DnLightComponent *)0)->bindJoint ) },
	{ "float", "forwardOffset", (intptr_t)(&((DnLightComponent *)0)->forwardOffset), sizeof( ((DnLightComponent *)0)->forwardOffset ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEventDef_typeInfo[] = {
	{ ": const char *", "name", (intptr_t)(&((idEventDef *)0)->name), sizeof( ((idEventDef *)0)->name ) },
	{ "const char *", "formatspec", (intptr_t)(&((idEventDef *)0)->formatspec), sizeof( ((idEventDef *)0)->formatspec ) },
	{ "unsigned int", "formatspecIndex", (intptr_t)(&((idEventDef *)0)->formatspecIndex), sizeof( ((idEventDef *)0)->formatspecIndex ) },
	{ "int", "returnType", (intptr_t)(&((idEventDef *)0)->returnType), sizeof( ((idEventDef *)0)->returnType ) },
	{ "int", "numargs", (intptr_t)(&((idEventDef *)0)->numargs), sizeof( ((idEventDef *)0)->numargs ) },
	{ "size_t", "argsize", (intptr_t)(&((idEventDef *)0)->argsize), sizeof( ((idEventDef *)0)->argsize ) },
	{ "int[8]", "argOffset", (intptr_t)(&((idEventDef *)0)->argOffset), sizeof( ((idEventDef *)0)->argOffset ) },
	{ "int", "eventnum", (intptr_t)(&((idEventDef *)0)->eventnum), sizeof( ((idEventDef *)0)->eventnum ) },
	{ "const idEventDef *", "next", (intptr_t)(&((idEventDef *)0)->next), sizeof( ((idEventDef *)0)->next ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEvent_typeInfo[] = {
	{ ": const idEventDef *", "eventdef", (intptr_t)(&((idEvent *)0)->eventdef), sizeof( ((idEvent *)0)->eventdef ) },
	{ "byte *", "data", (intptr_t)(&((idEvent *)0)->data), sizeof( ((idEvent *)0)->data ) },
	{ "int", "time", (intptr_t)(&((idEvent *)0)->time), sizeof( ((idEvent *)0)->time ) },
	{ "idClass *", "object", (intptr_t)(&((idEvent *)0)->object), sizeof( ((idEvent *)0)->object ) },
	{ "const idTypeInfo *", "typeinfo", (intptr_t)(&((idEvent *)0)->typeinfo), sizeof( ((idEvent *)0)->typeinfo ) },
	{ "idLinkList < idEvent >", "eventNode", (intptr_t)(&((idEvent *)0)->eventNode), sizeof( ((idEvent *)0)->eventNode ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEventArg_typeInfo[] = {
	{ ": int", "type", (intptr_t)(&((idEventArg *)0)->type), sizeof( ((idEventArg *)0)->type ) },
	{ "intptr_t", "value", (intptr_t)(&((idEventArg *)0)->value), sizeof( ((idEventArg *)0)->value ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAllocError_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idClass_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTypeInfo_typeInfo[] = {
	{ ": const char *", "classname", (intptr_t)(&((idTypeInfo *)0)->classname), sizeof( ((idTypeInfo *)0)->classname ) },
	{ "const char *", "superclass", (intptr_t)(&((idTypeInfo *)0)->superclass), sizeof( ((idTypeInfo *)0)->superclass ) },
	{ "idEventFunc < idClass > *", "eventCallbacks", (intptr_t)(&((idTypeInfo *)0)->eventCallbacks), sizeof( ((idTypeInfo *)0)->eventCallbacks ) },
	{ "eventCallback_t *", "eventMap", (intptr_t)(&((idTypeInfo *)0)->eventMap), sizeof( ((idTypeInfo *)0)->eventMap ) },
	{ "idTypeInfo *", "super", (intptr_t)(&((idTypeInfo *)0)->super), sizeof( ((idTypeInfo *)0)->super ) },
	{ "idTypeInfo *", "next", (intptr_t)(&((idTypeInfo *)0)->next), sizeof( ((idTypeInfo *)0)->next ) },
	{ "bool", "freeEventMap", (intptr_t)(&((idTypeInfo *)0)->freeEventMap), sizeof( ((idTypeInfo *)0)->freeEventMap ) },
	{ "int", "typeNum", (intptr_t)(&((idTypeInfo *)0)->typeNum), sizeof( ((idTypeInfo *)0)->typeNum ) },
	{ "int", "lastChild", (intptr_t)(&((idTypeInfo *)0)->lastChild), sizeof( ((idTypeInfo *)0)->lastChild ) },
	{ "idHierarchy < idTypeInfo >", "node", (intptr_t)(&((idTypeInfo *)0)->node), sizeof( ((idTypeInfo *)0)->node ) },
	{ NULL, 0 }
};

static classVariableInfo_t idSaveGame_typeInfo[] = {
	{ ": idFile *", "file", (intptr_t)(&((idSaveGame *)0)->file), sizeof( ((idSaveGame *)0)->file ) },
	{ "idList < const idClass * >", "objects", (intptr_t)(&((idSaveGame *)0)->objects), sizeof( ((idSaveGame *)0)->objects ) },
	{ NULL, 0 }
};

static classVariableInfo_t idRestoreGame_typeInfo[] = {
	{ ": int", "buildNumber", (intptr_t)(&((idRestoreGame *)0)->buildNumber), sizeof( ((idRestoreGame *)0)->buildNumber ) },
	{ "idFile *", "file", (intptr_t)(&((idRestoreGame *)0)->file), sizeof( ((idRestoreGame *)0)->file ) },
	{ "idList < idClass * >", "objects", (intptr_t)(&((idRestoreGame *)0)->objects), sizeof( ((idRestoreGame *)0)->objects ) },
	{ NULL, 0 }
};

static classVariableInfo_t idDebugGraph_typeInfo[] = {
	{ ": idList < float >", "samples", (intptr_t)(&((idDebugGraph *)0)->samples), sizeof( ((idDebugGraph *)0)->samples ) },
	{ "int", "index", (intptr_t)(&((idDebugGraph *)0)->index), sizeof( ((idDebugGraph *)0)->index ) },
	{ NULL, 0 }
};

static classVariableInfo_t function_t_typeInfo[] = {
	{ ": idStr", "name", (intptr_t)(&((function_t *)0)->name), sizeof( ((function_t *)0)->name ) },
	{ ": const idEventDef *", "eventdef", (intptr_t)(&((function_t *)0)->eventdef), sizeof( ((function_t *)0)->eventdef ) },
	{ "idVarDef *", "def", (intptr_t)(&((function_t *)0)->def), sizeof( ((function_t *)0)->def ) },
	{ "const idTypeDef *", "type", (intptr_t)(&((function_t *)0)->type), sizeof( ((function_t *)0)->type ) },
	{ "int", "firstStatement", (intptr_t)(&((function_t *)0)->firstStatement), sizeof( ((function_t *)0)->firstStatement ) },
	{ "int", "numStatements", (intptr_t)(&((function_t *)0)->numStatements), sizeof( ((function_t *)0)->numStatements ) },
	{ "int", "parmTotal", (intptr_t)(&((function_t *)0)->parmTotal), sizeof( ((function_t *)0)->parmTotal ) },
	{ "int", "locals", (intptr_t)(&((function_t *)0)->locals), sizeof( ((function_t *)0)->locals ) },
	{ "int", "filenum", (intptr_t)(&((function_t *)0)->filenum), sizeof( ((function_t *)0)->filenum ) },
	{ "idList < int >", "parmSize", (intptr_t)(&((function_t *)0)->parmSize), sizeof( ((function_t *)0)->parmSize ) },
	{ NULL, 0 }
};

static classVariableInfo_t eval_t_typeInfo[] = {
	{ "const char *", "stringPtr", (intptr_t)(&((eval_t *)0)->stringPtr), sizeof( ((eval_t *)0)->stringPtr ) },
	{ "float", "_float", (intptr_t)(&((eval_t *)0)->_float), sizeof( ((eval_t *)0)->_float ) },
	{ "float[3]", "vector", (intptr_t)(&((eval_t *)0)->vector), sizeof( ((eval_t *)0)->vector ) },
	{ "function_t *", "function", (intptr_t)(&((eval_t *)0)->function), sizeof( ((eval_t *)0)->function ) },
	{ "int", "_int", (intptr_t)(&((eval_t *)0)->_int), sizeof( ((eval_t *)0)->_int ) },
	{ "int", "entity", (intptr_t)(&((eval_t *)0)->entity), sizeof( ((eval_t *)0)->entity ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTypeDef_typeInfo[] = {
	{ ": etype_t", "type", (intptr_t)(&((idTypeDef *)0)->type), sizeof( ((idTypeDef *)0)->type ) },
	{ "idStr", "name", (intptr_t)(&((idTypeDef *)0)->name), sizeof( ((idTypeDef *)0)->name ) },
	{ "int", "size", (intptr_t)(&((idTypeDef *)0)->size), sizeof( ((idTypeDef *)0)->size ) },
	{ "idTypeDef *", "auxType", (intptr_t)(&((idTypeDef *)0)->auxType), sizeof( ((idTypeDef *)0)->auxType ) },
	{ "idList < idTypeDef * >", "parmTypes", (intptr_t)(&((idTypeDef *)0)->parmTypes), sizeof( ((idTypeDef *)0)->parmTypes ) },
	{ "idStrList", "parmNames", (intptr_t)(&((idTypeDef *)0)->parmNames), sizeof( ((idTypeDef *)0)->parmNames ) },
	{ "idList < const function_t * >", "functions", (intptr_t)(&((idTypeDef *)0)->functions), sizeof( ((idTypeDef *)0)->functions ) },
	{ ": idVarDef *", "def", (intptr_t)(&((idTypeDef *)0)->def), sizeof( ((idTypeDef *)0)->def ) },
	{ NULL, 0 }
};

static classVariableInfo_t idScriptObject_typeInfo[] = {
	{ ": idTypeDef *", "type", (intptr_t)(&((idScriptObject *)0)->type), sizeof( ((idScriptObject *)0)->type ) },
	{ ": byte *", "data", (intptr_t)(&((idScriptObject *)0)->data), sizeof( ((idScriptObject *)0)->data ) },
	{ NULL, 0 }
};

static classVariableInfo_t idCompileError_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t varEval_t_typeInfo[] = {
	{ "idScriptObject * *", "objectPtrPtr", (intptr_t)(&((varEval_t *)0)->objectPtrPtr), sizeof( ((varEval_t *)0)->objectPtrPtr ) },
	{ "char *", "stringPtr", (intptr_t)(&((varEval_t *)0)->stringPtr), sizeof( ((varEval_t *)0)->stringPtr ) },
	{ "float *", "floatPtr", (intptr_t)(&((varEval_t *)0)->floatPtr), sizeof( ((varEval_t *)0)->floatPtr ) },
	{ "idVec3 *", "vectorPtr", (intptr_t)(&((varEval_t *)0)->vectorPtr), sizeof( ((varEval_t *)0)->vectorPtr ) },
	{ "function_t *", "functionPtr", (intptr_t)(&((varEval_t *)0)->functionPtr), sizeof( ((varEval_t *)0)->functionPtr ) },
	{ "int *", "intPtr", (intptr_t)(&((varEval_t *)0)->intPtr), sizeof( ((varEval_t *)0)->intPtr ) },
	{ "byte *", "bytePtr", (intptr_t)(&((varEval_t *)0)->bytePtr), sizeof( ((varEval_t *)0)->bytePtr ) },
	{ "int *", "entityNumberPtr", (intptr_t)(&((varEval_t *)0)->entityNumberPtr), sizeof( ((varEval_t *)0)->entityNumberPtr ) },
	{ "int", "virtualFunction", (intptr_t)(&((varEval_t *)0)->virtualFunction), sizeof( ((varEval_t *)0)->virtualFunction ) },
	{ "int", "jumpOffset", (intptr_t)(&((varEval_t *)0)->jumpOffset), sizeof( ((varEval_t *)0)->jumpOffset ) },
	{ "int", "stackOffset", (intptr_t)(&((varEval_t *)0)->stackOffset), sizeof( ((varEval_t *)0)->stackOffset ) },
	{ "int", "argSize", (intptr_t)(&((varEval_t *)0)->argSize), sizeof( ((varEval_t *)0)->argSize ) },
	{ "varEval_s *", "evalPtr", (intptr_t)(&((varEval_t *)0)->evalPtr), sizeof( ((varEval_t *)0)->evalPtr ) },
	{ "int", "ptrOffset", (intptr_t)(&((varEval_t *)0)->ptrOffset), sizeof( ((varEval_t *)0)->ptrOffset ) },
	{ NULL, 0 }
};

static classVariableInfo_t idVarDef_typeInfo[] = {
	{ ": int", "num", (intptr_t)(&((idVarDef *)0)->num), sizeof( ((idVarDef *)0)->num ) },
	{ "varEval_t", "value", (intptr_t)(&((idVarDef *)0)->value), sizeof( ((idVarDef *)0)->value ) },
	{ "idVarDef *", "scope", (intptr_t)(&((idVarDef *)0)->scope), sizeof( ((idVarDef *)0)->scope ) },
	{ "int", "numUsers", (intptr_t)(&((idVarDef *)0)->numUsers), sizeof( ((idVarDef *)0)->numUsers ) },
	{ "initialized_t", "initialized", (intptr_t)(&((idVarDef *)0)->initialized), sizeof( ((idVarDef *)0)->initialized ) },
	{ ": idTypeDef *", "typeDef", (intptr_t)(&((idVarDef *)0)->typeDef), sizeof( ((idVarDef *)0)->typeDef ) },
	{ "idVarDefName *", "name", (intptr_t)(&((idVarDef *)0)->name), sizeof( ((idVarDef *)0)->name ) },
	{ "idVarDef *", "next", (intptr_t)(&((idVarDef *)0)->next), sizeof( ((idVarDef *)0)->next ) },
	{ NULL, 0 }
};

static classVariableInfo_t idVarDefName_typeInfo[] = {
	{ ": idStr", "name", (intptr_t)(&((idVarDefName *)0)->name), sizeof( ((idVarDefName *)0)->name ) },
	{ "idVarDef *", "defs", (intptr_t)(&((idVarDefName *)0)->defs), sizeof( ((idVarDefName *)0)->defs ) },
	{ NULL, 0 }
};

static classVariableInfo_t statement_t_typeInfo[] = {
	{ "unsigned short", "op", (intptr_t)(&((statement_t *)0)->op), sizeof( ((statement_t *)0)->op ) },
	{ "idVarDef *", "a", (intptr_t)(&((statement_t *)0)->a), sizeof( ((statement_t *)0)->a ) },
	{ "idVarDef *", "b", (intptr_t)(&((statement_t *)0)->b), sizeof( ((statement_t *)0)->b ) },
	{ "idVarDef *", "c", (intptr_t)(&((statement_t *)0)->c), sizeof( ((statement_t *)0)->c ) },
	{ "unsigned short", "linenumber", (intptr_t)(&((statement_t *)0)->linenumber), sizeof( ((statement_t *)0)->linenumber ) },
	{ "unsigned short", "file", (intptr_t)(&((statement_t *)0)->file), sizeof( ((statement_t *)0)->file ) },
	{ NULL, 0 }
};

static classVariableInfo_t idProgram_typeInfo[] = {
	{ ": idStrList", "fileList", (intptr_t)(&((idProgram *)0)->fileList), sizeof( ((idProgram *)0)->fileList ) },
	{ "idStr", "filename", (intptr_t)(&((idProgram *)0)->filename), sizeof( ((idProgram *)0)->filename ) },
	{ "int", "filenum", (intptr_t)(&((idProgram *)0)->filenum), sizeof( ((idProgram *)0)->filenum ) },
	{ "int", "numVariables", (intptr_t)(&((idProgram *)0)->numVariables), sizeof( ((idProgram *)0)->numVariables ) },
	{ "byte[296608]", "variables", (intptr_t)(&((idProgram *)0)->variables), sizeof( ((idProgram *)0)->variables ) },
	{ "idStaticList < byte , 296608 >", "variableDefaults", (intptr_t)(&((idProgram *)0)->variableDefaults), sizeof( ((idProgram *)0)->variableDefaults ) },
	{ "idStaticList < function_t , 3584 >", "functions", (intptr_t)(&((idProgram *)0)->functions), sizeof( ((idProgram *)0)->functions ) },
	{ "idStaticList < statement_t , 131072 >", "statements", (intptr_t)(&((idProgram *)0)->statements), sizeof( ((idProgram *)0)->statements ) },
	{ "idList < idTypeDef * >", "types", (intptr_t)(&((idProgram *)0)->types), sizeof( ((idProgram *)0)->types ) },
	{ "idList < idVarDefName * >", "varDefNames", (intptr_t)(&((idProgram *)0)->varDefNames), sizeof( ((idProgram *)0)->varDefNames ) },
	{ "idHashIndex", "varDefNameHash", (intptr_t)(&((idProgram *)0)->varDefNameHash), sizeof( ((idProgram *)0)->varDefNameHash ) },
	{ "idList < idVarDef * >", "varDefs", (intptr_t)(&((idProgram *)0)->varDefs), sizeof( ((idProgram *)0)->varDefs ) },
	{ "idVarDef *", "sysDef", (intptr_t)(&((idProgram *)0)->sysDef), sizeof( ((idProgram *)0)->sysDef ) },
	{ "int", "top_functions", (intptr_t)(&((idProgram *)0)->top_functions), sizeof( ((idProgram *)0)->top_functions ) },
	{ "int", "top_statements", (intptr_t)(&((idProgram *)0)->top_statements), sizeof( ((idProgram *)0)->top_statements ) },
	{ "int", "top_types", (intptr_t)(&((idProgram *)0)->top_types), sizeof( ((idProgram *)0)->top_types ) },
	{ "int", "top_defs", (intptr_t)(&((idProgram *)0)->top_defs), sizeof( ((idProgram *)0)->top_defs ) },
	{ "int", "top_files", (intptr_t)(&((idProgram *)0)->top_files), sizeof( ((idProgram *)0)->top_files ) },
	{ ": idVarDef *", "returnDef", (intptr_t)(&((idProgram *)0)->returnDef), sizeof( ((idProgram *)0)->returnDef ) },
	{ "idVarDef *", "returnStringDef", (intptr_t)(&((idProgram *)0)->returnStringDef), sizeof( ((idProgram *)0)->returnStringDef ) },
	{ NULL, 0 }
};

static classVariableInfo_t frameBlend_t_typeInfo[] = {
	{ "int", "cycleCount", (intptr_t)(&((frameBlend_t *)0)->cycleCount), sizeof( ((frameBlend_t *)0)->cycleCount ) },
	{ "int", "frame1", (intptr_t)(&((frameBlend_t *)0)->frame1), sizeof( ((frameBlend_t *)0)->frame1 ) },
	{ "int", "frame2", (intptr_t)(&((frameBlend_t *)0)->frame2), sizeof( ((frameBlend_t *)0)->frame2 ) },
	{ "float", "frontlerp", (intptr_t)(&((frameBlend_t *)0)->frontlerp), sizeof( ((frameBlend_t *)0)->frontlerp ) },
	{ "float", "backlerp", (intptr_t)(&((frameBlend_t *)0)->backlerp), sizeof( ((frameBlend_t *)0)->backlerp ) },
	{ NULL, 0 }
};

static classVariableInfo_t jointAnimInfo_t_typeInfo[] = {
	{ "int", "nameIndex", (intptr_t)(&((jointAnimInfo_t *)0)->nameIndex), sizeof( ((jointAnimInfo_t *)0)->nameIndex ) },
	{ "int", "parentNum", (intptr_t)(&((jointAnimInfo_t *)0)->parentNum), sizeof( ((jointAnimInfo_t *)0)->parentNum ) },
	{ "int", "animBits", (intptr_t)(&((jointAnimInfo_t *)0)->animBits), sizeof( ((jointAnimInfo_t *)0)->animBits ) },
	{ "int", "firstComponent", (intptr_t)(&((jointAnimInfo_t *)0)->firstComponent), sizeof( ((jointAnimInfo_t *)0)->firstComponent ) },
	{ NULL, 0 }
};

static classVariableInfo_t jointInfo_t_typeInfo[] = {
	{ "jointHandle_t", "num", (intptr_t)(&((jointInfo_t *)0)->num), sizeof( ((jointInfo_t *)0)->num ) },
	{ "jointHandle_t", "parentNum", (intptr_t)(&((jointInfo_t *)0)->parentNum), sizeof( ((jointInfo_t *)0)->parentNum ) },
	{ "int", "channel", (intptr_t)(&((jointInfo_t *)0)->channel), sizeof( ((jointInfo_t *)0)->channel ) },
	{ NULL, 0 }
};

static classVariableInfo_t jointMod_t_typeInfo[] = {
	{ "jointHandle_t", "jointnum", (intptr_t)(&((jointMod_t *)0)->jointnum), sizeof( ((jointMod_t *)0)->jointnum ) },
	{ "idMat3", "mat", (intptr_t)(&((jointMod_t *)0)->mat), sizeof( ((jointMod_t *)0)->mat ) },
	{ "idVec3", "pos", (intptr_t)(&((jointMod_t *)0)->pos), sizeof( ((jointMod_t *)0)->pos ) },
	{ "jointModTransform_t", "transform_pos", (intptr_t)(&((jointMod_t *)0)->transform_pos), sizeof( ((jointMod_t *)0)->transform_pos ) },
	{ "jointModTransform_t", "transform_axis", (intptr_t)(&((jointMod_t *)0)->transform_axis), sizeof( ((jointMod_t *)0)->transform_axis ) },
	{ NULL, 0 }
};

static classVariableInfo_t frameLookup_t_typeInfo[] = {
	{ "int", "num", (intptr_t)(&((frameLookup_t *)0)->num), sizeof( ((frameLookup_t *)0)->num ) },
	{ "int", "firstCommand", (intptr_t)(&((frameLookup_t *)0)->firstCommand), sizeof( ((frameLookup_t *)0)->firstCommand ) },
	{ NULL, 0 }
};

static classVariableInfo_t class_28_class_28_typeInfo[] = {
//	{ "const idSoundShader *", "soundShader", (intptr_t)(&((class_28::class_28 *)0)->soundShader), sizeof( ((class_28::class_28 *)0)->soundShader ) },
//	{ "const function_t *", "function", (intptr_t)(&((class_28::class_28 *)0)->function), sizeof( ((class_28::class_28 *)0)->function ) },
//	{ "const idDeclSkin *", "skin", (intptr_t)(&((class_28::class_28 *)0)->skin), sizeof( ((class_28::class_28 *)0)->skin ) },
//	{ "int", "index", (intptr_t)(&((class_28::class_28 *)0)->index), sizeof( ((class_28::class_28 *)0)->index ) },
	{ NULL, 0 }
};

static classVariableInfo_t frameCommand_t_typeInfo[] = {
	{ "frameCommandType_t", "type", (intptr_t)(&((frameCommand_t *)0)->type), sizeof( ((frameCommand_t *)0)->type ) },
	{ "idStr *", "string", (intptr_t)(&((frameCommand_t *)0)->string), sizeof( ((frameCommand_t *)0)->string ) },
	{ NULL, 0 }
};

static classVariableInfo_t animFlags_t_typeInfo[] = {
//	{ "bool", "prevent_idle_override", (intptr_t)(&((animFlags_t *)0)->prevent_idle_override), sizeof( ((animFlags_t *)0)->prevent_idle_override ) },
//	{ "bool", "random_cycle_start", (intptr_t)(&((animFlags_t *)0)->random_cycle_start), sizeof( ((animFlags_t *)0)->random_cycle_start ) },
//	{ "bool", "ai_no_turn", (intptr_t)(&((animFlags_t *)0)->ai_no_turn), sizeof( ((animFlags_t *)0)->ai_no_turn ) },
//	{ "bool", "anim_turn", (intptr_t)(&((animFlags_t *)0)->anim_turn), sizeof( ((animFlags_t *)0)->anim_turn ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMD5Anim_typeInfo[] = {
	{ ": int", "numFrames", (intptr_t)(&((idMD5Anim *)0)->numFrames), sizeof( ((idMD5Anim *)0)->numFrames ) },
	{ "int", "frameRate", (intptr_t)(&((idMD5Anim *)0)->frameRate), sizeof( ((idMD5Anim *)0)->frameRate ) },
	{ "int", "animLength", (intptr_t)(&((idMD5Anim *)0)->animLength), sizeof( ((idMD5Anim *)0)->animLength ) },
	{ "int", "numJoints", (intptr_t)(&((idMD5Anim *)0)->numJoints), sizeof( ((idMD5Anim *)0)->numJoints ) },
	{ "int", "numAnimatedComponents", (intptr_t)(&((idMD5Anim *)0)->numAnimatedComponents), sizeof( ((idMD5Anim *)0)->numAnimatedComponents ) },
	{ "idList < idBounds >", "bounds", (intptr_t)(&((idMD5Anim *)0)->bounds), sizeof( ((idMD5Anim *)0)->bounds ) },
	{ "idList < jointAnimInfo_t >", "jointInfo", (intptr_t)(&((idMD5Anim *)0)->jointInfo), sizeof( ((idMD5Anim *)0)->jointInfo ) },
	{ "idList < idJointQuat >", "baseFrame", (intptr_t)(&((idMD5Anim *)0)->baseFrame), sizeof( ((idMD5Anim *)0)->baseFrame ) },
	{ "idList < float >", "componentFrames", (intptr_t)(&((idMD5Anim *)0)->componentFrames), sizeof( ((idMD5Anim *)0)->componentFrames ) },
	{ "idStr", "name", (intptr_t)(&((idMD5Anim *)0)->name), sizeof( ((idMD5Anim *)0)->name ) },
	{ "idVec3", "totaldelta", (intptr_t)(&((idMD5Anim *)0)->totaldelta), sizeof( ((idMD5Anim *)0)->totaldelta ) },
	{ "mutable int", "ref_count", (intptr_t)(&((idMD5Anim *)0)->ref_count), sizeof( ((idMD5Anim *)0)->ref_count ) },
	{ "idRenderModel *", "renderModel", (intptr_t)(&((idMD5Anim *)0)->renderModel), sizeof( ((idMD5Anim *)0)->renderModel ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAnim_typeInfo[] = {
	{ ": const idDeclModelDef *", "modelDef", (intptr_t)(&((idAnim *)0)->modelDef), sizeof( ((idAnim *)0)->modelDef ) },
	{ "const idMD5Anim *[3]", "anims", (intptr_t)(&((idAnim *)0)->anims), sizeof( ((idAnim *)0)->anims ) },
	{ "int", "numAnims", (intptr_t)(&((idAnim *)0)->numAnims), sizeof( ((idAnim *)0)->numAnims ) },
	{ "idStr", "name", (intptr_t)(&((idAnim *)0)->name), sizeof( ((idAnim *)0)->name ) },
	{ "idStr", "realname", (intptr_t)(&((idAnim *)0)->realname), sizeof( ((idAnim *)0)->realname ) },
	{ "idList < frameLookup_t >", "frameLookup", (intptr_t)(&((idAnim *)0)->frameLookup), sizeof( ((idAnim *)0)->frameLookup ) },
	{ "idList < frameCommand_t >", "frameCommands", (intptr_t)(&((idAnim *)0)->frameCommands), sizeof( ((idAnim *)0)->frameCommands ) },
	{ "animFlags_t", "flags", (intptr_t)(&((idAnim *)0)->flags), sizeof( ((idAnim *)0)->flags ) },
	{ NULL, 0 }
};

static classVariableInfo_t idDeclModelDef_typeInfo[] = {
	{ ": idVec3", "offset", (intptr_t)(&((idDeclModelDef *)0)->offset), sizeof( ((idDeclModelDef *)0)->offset ) },
	{ "idList < jointInfo_t >", "joints", (intptr_t)(&((idDeclModelDef *)0)->joints), sizeof( ((idDeclModelDef *)0)->joints ) },
	{ "idList < int >", "jointParents", (intptr_t)(&((idDeclModelDef *)0)->jointParents), sizeof( ((idDeclModelDef *)0)->jointParents ) },
	{ "idList < int >[5]", "channelJoints", (intptr_t)(&((idDeclModelDef *)0)->channelJoints), sizeof( ((idDeclModelDef *)0)->channelJoints ) },
	{ "idRenderModel *", "modelHandle", (intptr_t)(&((idDeclModelDef *)0)->modelHandle), sizeof( ((idDeclModelDef *)0)->modelHandle ) },
	{ "idList < idAnim * >", "anims", (intptr_t)(&((idDeclModelDef *)0)->anims), sizeof( ((idDeclModelDef *)0)->anims ) },
	{ "const idDeclSkin *", "skin", (intptr_t)(&((idDeclModelDef *)0)->skin), sizeof( ((idDeclModelDef *)0)->skin ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAnimBlend_typeInfo[] = {
	{ ": const idDeclModelDef *", "modelDef", (intptr_t)(&((idAnimBlend *)0)->modelDef), sizeof( ((idAnimBlend *)0)->modelDef ) },
	{ "int", "starttime", (intptr_t)(&((idAnimBlend *)0)->starttime), sizeof( ((idAnimBlend *)0)->starttime ) },
	{ "int", "endtime", (intptr_t)(&((idAnimBlend *)0)->endtime), sizeof( ((idAnimBlend *)0)->endtime ) },
	{ "int", "timeOffset", (intptr_t)(&((idAnimBlend *)0)->timeOffset), sizeof( ((idAnimBlend *)0)->timeOffset ) },
	{ "float", "rate", (intptr_t)(&((idAnimBlend *)0)->rate), sizeof( ((idAnimBlend *)0)->rate ) },
	{ "int", "blendStartTime", (intptr_t)(&((idAnimBlend *)0)->blendStartTime), sizeof( ((idAnimBlend *)0)->blendStartTime ) },
	{ "int", "blendDuration", (intptr_t)(&((idAnimBlend *)0)->blendDuration), sizeof( ((idAnimBlend *)0)->blendDuration ) },
	{ "float", "blendStartValue", (intptr_t)(&((idAnimBlend *)0)->blendStartValue), sizeof( ((idAnimBlend *)0)->blendStartValue ) },
	{ "float", "blendEndValue", (intptr_t)(&((idAnimBlend *)0)->blendEndValue), sizeof( ((idAnimBlend *)0)->blendEndValue ) },
	{ "float[3]", "animWeights", (intptr_t)(&((idAnimBlend *)0)->animWeights), sizeof( ((idAnimBlend *)0)->animWeights ) },
	{ "short", "cycle", (intptr_t)(&((idAnimBlend *)0)->cycle), sizeof( ((idAnimBlend *)0)->cycle ) },
	{ "short", "frame", (intptr_t)(&((idAnimBlend *)0)->frame), sizeof( ((idAnimBlend *)0)->frame ) },
	{ "short", "animNum", (intptr_t)(&((idAnimBlend *)0)->animNum), sizeof( ((idAnimBlend *)0)->animNum ) },
	{ "bool", "allowMove", (intptr_t)(&((idAnimBlend *)0)->allowMove), sizeof( ((idAnimBlend *)0)->allowMove ) },
	{ "bool", "allowFrameCommands", (intptr_t)(&((idAnimBlend *)0)->allowFrameCommands), sizeof( ((idAnimBlend *)0)->allowFrameCommands ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFPoseJointMod_typeInfo[] = {
	{ "AFJointModType_t", "mod", (intptr_t)(&((idAFPoseJointMod *)0)->mod), sizeof( ((idAFPoseJointMod *)0)->mod ) },
	{ "idMat3", "axis", (intptr_t)(&((idAFPoseJointMod *)0)->axis), sizeof( ((idAFPoseJointMod *)0)->axis ) },
	{ "idVec3", "origin", (intptr_t)(&((idAFPoseJointMod *)0)->origin), sizeof( ((idAFPoseJointMod *)0)->origin ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAnimator_typeInfo[] = {
	{ ": const idDeclModelDef *", "modelDef", (intptr_t)(&((idAnimator *)0)->modelDef), sizeof( ((idAnimator *)0)->modelDef ) },
	{ "idEntity *", "entity", (intptr_t)(&((idAnimator *)0)->entity), sizeof( ((idAnimator *)0)->entity ) },
	{ "idAnimBlend[15]", "channels", (intptr_t)(&((idAnimator *)0)->channels), sizeof( ((idAnimator *)0)->channels ) },
	{ "idList < jointMod_t * >", "jointMods", (intptr_t)(&((idAnimator *)0)->jointMods), sizeof( ((idAnimator *)0)->jointMods ) },
	{ "int", "numJoints", (intptr_t)(&((idAnimator *)0)->numJoints), sizeof( ((idAnimator *)0)->numJoints ) },
	{ "idJointMat *", "joints", (intptr_t)(&((idAnimator *)0)->joints), sizeof( ((idAnimator *)0)->joints ) },
	{ "mutable int", "lastTransformTime", (intptr_t)(&((idAnimator *)0)->lastTransformTime), sizeof( ((idAnimator *)0)->lastTransformTime ) },
	{ "mutable bool", "stoppedAnimatingUpdate", (intptr_t)(&((idAnimator *)0)->stoppedAnimatingUpdate), sizeof( ((idAnimator *)0)->stoppedAnimatingUpdate ) },
	{ "bool", "removeOriginOffset", (intptr_t)(&((idAnimator *)0)->removeOriginOffset), sizeof( ((idAnimator *)0)->removeOriginOffset ) },
	{ "bool", "forceUpdate", (intptr_t)(&((idAnimator *)0)->forceUpdate), sizeof( ((idAnimator *)0)->forceUpdate ) },
	{ "idBounds", "frameBounds", (intptr_t)(&((idAnimator *)0)->frameBounds), sizeof( ((idAnimator *)0)->frameBounds ) },
	{ "float", "AFPoseBlendWeight", (intptr_t)(&((idAnimator *)0)->AFPoseBlendWeight), sizeof( ((idAnimator *)0)->AFPoseBlendWeight ) },
	{ "idList < int >", "AFPoseJoints", (intptr_t)(&((idAnimator *)0)->AFPoseJoints), sizeof( ((idAnimator *)0)->AFPoseJoints ) },
	{ "idList < idAFPoseJointMod >", "AFPoseJointMods", (intptr_t)(&((idAnimator *)0)->AFPoseJointMods), sizeof( ((idAnimator *)0)->AFPoseJointMods ) },
	{ "idList < idJointQuat >", "AFPoseJointFrame", (intptr_t)(&((idAnimator *)0)->AFPoseJointFrame), sizeof( ((idAnimator *)0)->AFPoseJointFrame ) },
	{ "idBounds", "AFPoseBounds", (intptr_t)(&((idAnimator *)0)->AFPoseBounds), sizeof( ((idAnimator *)0)->AFPoseBounds ) },
	{ "int", "AFPoseTime", (intptr_t)(&((idAnimator *)0)->AFPoseTime), sizeof( ((idAnimator *)0)->AFPoseTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAnimManager_typeInfo[] = {
	{ ": idHashTable < idMD5Anim * >", "animations", (intptr_t)(&((idAnimManager *)0)->animations), sizeof( ((idAnimManager *)0)->animations ) },
	{ "idStrList", "jointnames", (intptr_t)(&((idAnimManager *)0)->jointnames), sizeof( ((idAnimManager *)0)->jointnames ) },
	{ "idHashIndex", "jointnamesHash", (intptr_t)(&((idAnimManager *)0)->jointnamesHash), sizeof( ((idAnimManager *)0)->jointnamesHash ) },
	{ NULL, 0 }
};

static classVariableInfo_t idClipModel_typeInfo[] = {
	{ ": bool", "enabled", (intptr_t)(&((idClipModel *)0)->enabled), sizeof( ((idClipModel *)0)->enabled ) },
	{ "idEntity *", "entity", (intptr_t)(&((idClipModel *)0)->entity), sizeof( ((idClipModel *)0)->entity ) },
	{ "int", "id", (intptr_t)(&((idClipModel *)0)->id), sizeof( ((idClipModel *)0)->id ) },
	{ "idEntity *", "owner", (intptr_t)(&((idClipModel *)0)->owner), sizeof( ((idClipModel *)0)->owner ) },
	{ "idVec3", "origin", (intptr_t)(&((idClipModel *)0)->origin), sizeof( ((idClipModel *)0)->origin ) },
	{ "idMat3", "axis", (intptr_t)(&((idClipModel *)0)->axis), sizeof( ((idClipModel *)0)->axis ) },
	{ "idBounds", "bounds", (intptr_t)(&((idClipModel *)0)->bounds), sizeof( ((idClipModel *)0)->bounds ) },
	{ "idBounds", "absBounds", (intptr_t)(&((idClipModel *)0)->absBounds), sizeof( ((idClipModel *)0)->absBounds ) },
	{ "const idMaterial *", "material", (intptr_t)(&((idClipModel *)0)->material), sizeof( ((idClipModel *)0)->material ) },
	{ "int", "contents", (intptr_t)(&((idClipModel *)0)->contents), sizeof( ((idClipModel *)0)->contents ) },
	{ "cmHandle_t", "collisionModelHandle", (intptr_t)(&((idClipModel *)0)->collisionModelHandle), sizeof( ((idClipModel *)0)->collisionModelHandle ) },
	{ "int", "traceModelIndex", (intptr_t)(&((idClipModel *)0)->traceModelIndex), sizeof( ((idClipModel *)0)->traceModelIndex ) },
	{ "int", "renderModelHandle", (intptr_t)(&((idClipModel *)0)->renderModelHandle), sizeof( ((idClipModel *)0)->renderModelHandle ) },
	{ "bool", "traceRecievesCollision", (intptr_t)(&((idClipModel *)0)->traceRecievesCollision), sizeof( ((idClipModel *)0)->traceRecievesCollision ) },
	{ "clipLink_s *", "clipLinks", (intptr_t)(&((idClipModel *)0)->clipLinks), sizeof( ((idClipModel *)0)->clipLinks ) },
	{ "int", "touchCount", (intptr_t)(&((idClipModel *)0)->touchCount), sizeof( ((idClipModel *)0)->touchCount ) },
	{ NULL, 0 }
};

static classVariableInfo_t idClip_typeInfo[] = {
	{ ": int", "numClipSectors", (intptr_t)(&((idClip *)0)->numClipSectors), sizeof( ((idClip *)0)->numClipSectors ) },
	{ "clipSector_s *", "clipSectors", (intptr_t)(&((idClip *)0)->clipSectors), sizeof( ((idClip *)0)->clipSectors ) },
	{ "idBounds", "worldBounds", (intptr_t)(&((idClip *)0)->worldBounds), sizeof( ((idClip *)0)->worldBounds ) },
	{ "idClipModel", "temporaryClipModel", (intptr_t)(&((idClip *)0)->temporaryClipModel), sizeof( ((idClip *)0)->temporaryClipModel ) },
	{ "idClipModel", "defaultClipModel", (intptr_t)(&((idClip *)0)->defaultClipModel), sizeof( ((idClip *)0)->defaultClipModel ) },
	{ "mutable int", "touchCount", (intptr_t)(&((idClip *)0)->touchCount), sizeof( ((idClip *)0)->touchCount ) },
	{ "int", "numTranslations", (intptr_t)(&((idClip *)0)->numTranslations), sizeof( ((idClip *)0)->numTranslations ) },
	{ "int", "numRotations", (intptr_t)(&((idClip *)0)->numRotations), sizeof( ((idClip *)0)->numRotations ) },
	{ "int", "numMotions", (intptr_t)(&((idClip *)0)->numMotions), sizeof( ((idClip *)0)->numMotions ) },
	{ "int", "numRenderModelTraces", (intptr_t)(&((idClip *)0)->numRenderModelTraces), sizeof( ((idClip *)0)->numRenderModelTraces ) },
	{ "int", "numContents", (intptr_t)(&((idClip *)0)->numContents), sizeof( ((idClip *)0)->numContents ) },
	{ "int", "numContacts", (intptr_t)(&((idClip *)0)->numContacts), sizeof( ((idClip *)0)->numContacts ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPush_pushed_s_typeInfo[] = {
	{ "idEntity *", "ent", (intptr_t)(&((idPush::pushed_s *)0)->ent), sizeof( ((idPush::pushed_s *)0)->ent ) },
	{ "idAngles", "deltaViewAngles", (intptr_t)(&((idPush::pushed_s *)0)->deltaViewAngles), sizeof( ((idPush::pushed_s *)0)->deltaViewAngles ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPush_pushedGroup_s_typeInfo[] = {
	{ "idEntity *", "ent", (intptr_t)(&((idPush::pushedGroup_s *)0)->ent), sizeof( ((idPush::pushedGroup_s *)0)->ent ) },
	{ "float", "fraction", (intptr_t)(&((idPush::pushedGroup_s *)0)->fraction), sizeof( ((idPush::pushedGroup_s *)0)->fraction ) },
	{ "bool", "groundContact", (intptr_t)(&((idPush::pushedGroup_s *)0)->groundContact), sizeof( ((idPush::pushedGroup_s *)0)->groundContact ) },
	{ "bool", "test", (intptr_t)(&((idPush::pushedGroup_s *)0)->test), sizeof( ((idPush::pushedGroup_s *)0)->test ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPush_typeInfo[] = {
	{ "idPush::pushed_s[4096]", "pushed", (intptr_t)(&((idPush *)0)->pushed), sizeof( ((idPush *)0)->pushed ) },
	{ "int", "numPushed", (intptr_t)(&((idPush *)0)->numPushed), sizeof( ((idPush *)0)->numPushed ) },
	{ "idPush::pushedGroup_s[4096]", "pushedGroup", (intptr_t)(&((idPush *)0)->pushedGroup), sizeof( ((idPush *)0)->pushedGroup ) },
	{ "int", "pushedGroupSize", (intptr_t)(&((idPush *)0)->pushedGroupSize), sizeof( ((idPush *)0)->pushedGroupSize ) },
	{ NULL, 0 }
};

static classVariableInfo_t pvsHandle_t_typeInfo[] = {
	{ "int", "i", (intptr_t)(&((pvsHandle_t *)0)->i), sizeof( ((pvsHandle_t *)0)->i ) },
	{ "unsigned int", "h", (intptr_t)(&((pvsHandle_t *)0)->h), sizeof( ((pvsHandle_t *)0)->h ) },
	{ NULL, 0 }
};

static classVariableInfo_t pvsCurrent_t_typeInfo[] = {
	{ "pvsHandle_t", "handle", (intptr_t)(&((pvsCurrent_t *)0)->handle), sizeof( ((pvsCurrent_t *)0)->handle ) },
	{ "byte *", "pvs", (intptr_t)(&((pvsCurrent_t *)0)->pvs), sizeof( ((pvsCurrent_t *)0)->pvs ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPVS_typeInfo[] = {
	{ ": int", "numAreas", (intptr_t)(&((idPVS *)0)->numAreas), sizeof( ((idPVS *)0)->numAreas ) },
	{ "int", "numPortals", (intptr_t)(&((idPVS *)0)->numPortals), sizeof( ((idPVS *)0)->numPortals ) },
	{ "bool *", "connectedAreas", (intptr_t)(&((idPVS *)0)->connectedAreas), sizeof( ((idPVS *)0)->connectedAreas ) },
	{ "int *", "areaQueue", (intptr_t)(&((idPVS *)0)->areaQueue), sizeof( ((idPVS *)0)->areaQueue ) },
	{ "byte *", "areaPVS", (intptr_t)(&((idPVS *)0)->areaPVS), sizeof( ((idPVS *)0)->areaPVS ) },
	{ "mutable pvsCurrent_t[8]", "currentPVS", (intptr_t)(&((idPVS *)0)->currentPVS), sizeof( ((idPVS *)0)->currentPVS ) },
	{ "int", "portalVisBytes", (intptr_t)(&((idPVS *)0)->portalVisBytes), sizeof( ((idPVS *)0)->portalVisBytes ) },
	{ "int", "portalVisLongs", (intptr_t)(&((idPVS *)0)->portalVisLongs), sizeof( ((idPVS *)0)->portalVisLongs ) },
	{ "int", "areaVisBytes", (intptr_t)(&((idPVS *)0)->areaVisBytes), sizeof( ((idPVS *)0)->areaVisBytes ) },
	{ "int", "areaVisLongs", (intptr_t)(&((idPVS *)0)->areaVisLongs), sizeof( ((idPVS *)0)->areaVisLongs ) },
	{ "pvsPortal_s *", "pvsPortals", (intptr_t)(&((idPVS *)0)->pvsPortals), sizeof( ((idPVS *)0)->pvsPortals ) },
	{ "pvsArea_s *", "pvsAreas", (intptr_t)(&((idPVS *)0)->pvsAreas), sizeof( ((idPVS *)0)->pvsAreas ) },
	{ NULL, 0 }
};

static classVariableInfo_t mpPlayerState_t_typeInfo[] = {
	{ "int", "ping", (intptr_t)(&((mpPlayerState_t *)0)->ping), sizeof( ((mpPlayerState_t *)0)->ping ) },
	{ "int", "fragCount", (intptr_t)(&((mpPlayerState_t *)0)->fragCount), sizeof( ((mpPlayerState_t *)0)->fragCount ) },
	{ "int", "teamFragCount", (intptr_t)(&((mpPlayerState_t *)0)->teamFragCount), sizeof( ((mpPlayerState_t *)0)->teamFragCount ) },
	{ "int", "wins", (intptr_t)(&((mpPlayerState_t *)0)->wins), sizeof( ((mpPlayerState_t *)0)->wins ) },
	{ "playerVote_t", "vote", (intptr_t)(&((mpPlayerState_t *)0)->vote), sizeof( ((mpPlayerState_t *)0)->vote ) },
	{ "bool", "scoreBoardUp", (intptr_t)(&((mpPlayerState_t *)0)->scoreBoardUp), sizeof( ((mpPlayerState_t *)0)->scoreBoardUp ) },
	{ "bool", "ingame", (intptr_t)(&((mpPlayerState_t *)0)->ingame), sizeof( ((mpPlayerState_t *)0)->ingame ) },
	{ NULL, 0 }
};

static classVariableInfo_t mpChatLine_t_typeInfo[] = {
	{ "idStr", "line", (intptr_t)(&((mpChatLine_t *)0)->line), sizeof( ((mpChatLine_t *)0)->line ) },
	{ "short", "fade", (intptr_t)(&((mpChatLine_t *)0)->fade), sizeof( ((mpChatLine_t *)0)->fade ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMultiplayerGame_typeInfo[] = {
	{ "int", "player_red_flag", (intptr_t)(&((idMultiplayerGame *)0)->player_red_flag), sizeof( ((idMultiplayerGame *)0)->player_red_flag ) },
	{ "int", "player_blue_flag", (intptr_t)(&((idMultiplayerGame *)0)->player_blue_flag), sizeof( ((idMultiplayerGame *)0)->player_blue_flag ) },
	{ "gameState_t", "gameState", (intptr_t)(&((idMultiplayerGame *)0)->gameState), sizeof( ((idMultiplayerGame *)0)->gameState ) },
	{ "gameState_t", "nextState", (intptr_t)(&((idMultiplayerGame *)0)->nextState), sizeof( ((idMultiplayerGame *)0)->nextState ) },
	{ "int", "pingUpdateTime", (intptr_t)(&((idMultiplayerGame *)0)->pingUpdateTime), sizeof( ((idMultiplayerGame *)0)->pingUpdateTime ) },
	{ "mpPlayerState_t[32]", "playerState", (intptr_t)(&((idMultiplayerGame *)0)->playerState), sizeof( ((idMultiplayerGame *)0)->playerState ) },
	{ "vote_flags_t", "vote", (intptr_t)(&((idMultiplayerGame *)0)->vote), sizeof( ((idMultiplayerGame *)0)->vote ) },
	{ "int", "voteTimeOut", (intptr_t)(&((idMultiplayerGame *)0)->voteTimeOut), sizeof( ((idMultiplayerGame *)0)->voteTimeOut ) },
	{ "int", "voteExecTime", (intptr_t)(&((idMultiplayerGame *)0)->voteExecTime), sizeof( ((idMultiplayerGame *)0)->voteExecTime ) },
	{ "float", "yesVotes", (intptr_t)(&((idMultiplayerGame *)0)->yesVotes), sizeof( ((idMultiplayerGame *)0)->yesVotes ) },
	{ "float", "noVotes", (intptr_t)(&((idMultiplayerGame *)0)->noVotes), sizeof( ((idMultiplayerGame *)0)->noVotes ) },
	{ "idStr", "voteValue", (intptr_t)(&((idMultiplayerGame *)0)->voteValue), sizeof( ((idMultiplayerGame *)0)->voteValue ) },
	{ "idStr", "voteString", (intptr_t)(&((idMultiplayerGame *)0)->voteString), sizeof( ((idMultiplayerGame *)0)->voteString ) },
	{ "bool", "voted", (intptr_t)(&((idMultiplayerGame *)0)->voted), sizeof( ((idMultiplayerGame *)0)->voted ) },
	{ "int[32]", "kickVoteMap", (intptr_t)(&((idMultiplayerGame *)0)->kickVoteMap), sizeof( ((idMultiplayerGame *)0)->kickVoteMap ) },
	{ "int", "nextStateSwitch", (intptr_t)(&((idMultiplayerGame *)0)->nextStateSwitch), sizeof( ((idMultiplayerGame *)0)->nextStateSwitch ) },
	{ "int", "warmupEndTime", (intptr_t)(&((idMultiplayerGame *)0)->warmupEndTime), sizeof( ((idMultiplayerGame *)0)->warmupEndTime ) },
	{ "int", "matchStartedTime", (intptr_t)(&((idMultiplayerGame *)0)->matchStartedTime), sizeof( ((idMultiplayerGame *)0)->matchStartedTime ) },
	{ "int[2]", "currentTourneyPlayer", (intptr_t)(&((idMultiplayerGame *)0)->currentTourneyPlayer), sizeof( ((idMultiplayerGame *)0)->currentTourneyPlayer ) },
	{ "int", "lastWinner", (intptr_t)(&((idMultiplayerGame *)0)->lastWinner), sizeof( ((idMultiplayerGame *)0)->lastWinner ) },
	{ "idStr", "warmupText", (intptr_t)(&((idMultiplayerGame *)0)->warmupText), sizeof( ((idMultiplayerGame *)0)->warmupText ) },
	{ "bool", "one", (intptr_t)(&((idMultiplayerGame *)0)->one), sizeof( ((idMultiplayerGame *)0)->one ) },
	{ "bool", "two", (intptr_t)(&((idMultiplayerGame *)0)->two), sizeof( ((idMultiplayerGame *)0)->two ) },
	{ "bool", "three", (intptr_t)(&((idMultiplayerGame *)0)->three), sizeof( ((idMultiplayerGame *)0)->three ) },
	{ "idUserInterface *", "scoreBoard", (intptr_t)(&((idMultiplayerGame *)0)->scoreBoard), sizeof( ((idMultiplayerGame *)0)->scoreBoard ) },
	{ "idUserInterface *", "spectateGui", (intptr_t)(&((idMultiplayerGame *)0)->spectateGui), sizeof( ((idMultiplayerGame *)0)->spectateGui ) },
	{ "idUserInterface *", "guiChat", (intptr_t)(&((idMultiplayerGame *)0)->guiChat), sizeof( ((idMultiplayerGame *)0)->guiChat ) },
	{ "idUserInterface *", "mainGui", (intptr_t)(&((idMultiplayerGame *)0)->mainGui), sizeof( ((idMultiplayerGame *)0)->mainGui ) },
	{ "idListGUI *", "mapList", (intptr_t)(&((idMultiplayerGame *)0)->mapList), sizeof( ((idMultiplayerGame *)0)->mapList ) },
	{ "idUserInterface *", "msgmodeGui", (intptr_t)(&((idMultiplayerGame *)0)->msgmodeGui), sizeof( ((idMultiplayerGame *)0)->msgmodeGui ) },
	{ "int", "currentMenu", (intptr_t)(&((idMultiplayerGame *)0)->currentMenu), sizeof( ((idMultiplayerGame *)0)->currentMenu ) },
	{ "int", "nextMenu", (intptr_t)(&((idMultiplayerGame *)0)->nextMenu), sizeof( ((idMultiplayerGame *)0)->nextMenu ) },
	{ "bool", "bCurrentMenuMsg", (intptr_t)(&((idMultiplayerGame *)0)->bCurrentMenuMsg), sizeof( ((idMultiplayerGame *)0)->bCurrentMenuMsg ) },
	{ "mpChatLine_t[5]", "chatHistory", (intptr_t)(&((idMultiplayerGame *)0)->chatHistory), sizeof( ((idMultiplayerGame *)0)->chatHistory ) },
	{ "int", "chatHistoryIndex", (intptr_t)(&((idMultiplayerGame *)0)->chatHistoryIndex), sizeof( ((idMultiplayerGame *)0)->chatHistoryIndex ) },
	{ "int", "chatHistorySize", (intptr_t)(&((idMultiplayerGame *)0)->chatHistorySize), sizeof( ((idMultiplayerGame *)0)->chatHistorySize ) },
	{ "bool", "chatDataUpdated", (intptr_t)(&((idMultiplayerGame *)0)->chatDataUpdated), sizeof( ((idMultiplayerGame *)0)->chatDataUpdated ) },
	{ "int", "lastChatLineTime", (intptr_t)(&((idMultiplayerGame *)0)->lastChatLineTime), sizeof( ((idMultiplayerGame *)0)->lastChatLineTime ) },
	{ "int", "numRankedPlayers", (intptr_t)(&((idMultiplayerGame *)0)->numRankedPlayers), sizeof( ((idMultiplayerGame *)0)->numRankedPlayers ) },
	{ "idPlayer *[32]", "rankedPlayers", (intptr_t)(&((idMultiplayerGame *)0)->rankedPlayers), sizeof( ((idMultiplayerGame *)0)->rankedPlayers ) },
	{ "bool", "pureReady", (intptr_t)(&((idMultiplayerGame *)0)->pureReady), sizeof( ((idMultiplayerGame *)0)->pureReady ) },
	{ "int", "fragLimitTimeout", (intptr_t)(&((idMultiplayerGame *)0)->fragLimitTimeout), sizeof( ((idMultiplayerGame *)0)->fragLimitTimeout ) },
	{ "int[3]", "switchThrottle", (intptr_t)(&((idMultiplayerGame *)0)->switchThrottle), sizeof( ((idMultiplayerGame *)0)->switchThrottle ) },
	{ "int", "voiceChatThrottle", (intptr_t)(&((idMultiplayerGame *)0)->voiceChatThrottle), sizeof( ((idMultiplayerGame *)0)->voiceChatThrottle ) },
	{ "gameType_t", "lastGameType", (intptr_t)(&((idMultiplayerGame *)0)->lastGameType), sizeof( ((idMultiplayerGame *)0)->lastGameType ) },
	{ "int", "startFragLimit", (intptr_t)(&((idMultiplayerGame *)0)->startFragLimit), sizeof( ((idMultiplayerGame *)0)->startFragLimit ) },
	{ "idItemTeam *[2]", "teamFlags", (intptr_t)(&((idMultiplayerGame *)0)->teamFlags), sizeof( ((idMultiplayerGame *)0)->teamFlags ) },
	{ "int[2]", "teamPoints", (intptr_t)(&((idMultiplayerGame *)0)->teamPoints), sizeof( ((idMultiplayerGame *)0)->teamPoints ) },
	{ "bool", "flagMsgOn", (intptr_t)(&((idMultiplayerGame *)0)->flagMsgOn), sizeof( ((idMultiplayerGame *)0)->flagMsgOn ) },
	{ "const char *[6]", "gameTypeVoteMap", (intptr_t)(&((idMultiplayerGame *)0)->gameTypeVoteMap), sizeof( ((idMultiplayerGame *)0)->gameTypeVoteMap ) },
	{ NULL, 0 }
};

static classVariableInfo_t entityState_t_typeInfo[] = {
	{ "int", "entityNumber", (intptr_t)(&((entityState_t *)0)->entityNumber), sizeof( ((entityState_t *)0)->entityNumber ) },
	{ "idBitMsg", "state", (intptr_t)(&((entityState_t *)0)->state), sizeof( ((entityState_t *)0)->state ) },
	{ "byte[512]", "stateBuf", (intptr_t)(&((entityState_t *)0)->stateBuf), sizeof( ((entityState_t *)0)->stateBuf ) },
	{ "entityState_s *", "next", (intptr_t)(&((entityState_t *)0)->next), sizeof( ((entityState_t *)0)->next ) },
	{ NULL, 0 }
};

static classVariableInfo_t snapshot_t_typeInfo[] = {
	{ "int", "sequence", (intptr_t)(&((snapshot_t *)0)->sequence), sizeof( ((snapshot_t *)0)->sequence ) },
	{ "entityState_t *", "firstEntityState", (intptr_t)(&((snapshot_t *)0)->firstEntityState), sizeof( ((snapshot_t *)0)->firstEntityState ) },
	{ "int[128]", "pvs", (intptr_t)(&((snapshot_t *)0)->pvs), sizeof( ((snapshot_t *)0)->pvs ) },
	{ "snapshot_s *", "next", (intptr_t)(&((snapshot_t *)0)->next), sizeof( ((snapshot_t *)0)->next ) },
	{ NULL, 0 }
};

static classVariableInfo_t entityNetEvent_t_typeInfo[] = {
	{ "int", "spawnId", (intptr_t)(&((entityNetEvent_t *)0)->spawnId), sizeof( ((entityNetEvent_t *)0)->spawnId ) },
	{ "int", "event", (intptr_t)(&((entityNetEvent_t *)0)->event), sizeof( ((entityNetEvent_t *)0)->event ) },
	{ "int", "time", (intptr_t)(&((entityNetEvent_t *)0)->time), sizeof( ((entityNetEvent_t *)0)->time ) },
	{ "int", "paramsSize", (intptr_t)(&((entityNetEvent_t *)0)->paramsSize), sizeof( ((entityNetEvent_t *)0)->paramsSize ) },
	{ "byte[128]", "paramsBuf", (intptr_t)(&((entityNetEvent_t *)0)->paramsBuf), sizeof( ((entityNetEvent_t *)0)->paramsBuf ) },
	{ "entityNetEvent_s *", "next", (intptr_t)(&((entityNetEvent_t *)0)->next), sizeof( ((entityNetEvent_t *)0)->next ) },
	{ "entityNetEvent_s *", "prev", (intptr_t)(&((entityNetEvent_t *)0)->prev), sizeof( ((entityNetEvent_t *)0)->prev ) },
	{ NULL, 0 }
};

static classVariableInfo_t spawnSpot_t_typeInfo[] = {
	{ "idEntity *", "ent", (intptr_t)(&((spawnSpot_t *)0)->ent), sizeof( ((spawnSpot_t *)0)->ent ) },
	{ "int", "dist", (intptr_t)(&((spawnSpot_t *)0)->dist), sizeof( ((spawnSpot_t *)0)->dist ) },
	{ "int", "team", (intptr_t)(&((spawnSpot_t *)0)->team), sizeof( ((spawnSpot_t *)0)->team ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEventQueue_typeInfo[] = {
	{ ": entityNetEvent_t *", "start", (intptr_t)(&((idEventQueue *)0)->start), sizeof( ((idEventQueue *)0)->start ) },
	{ "entityNetEvent_t *", "end", (intptr_t)(&((idEventQueue *)0)->end), sizeof( ((idEventQueue *)0)->end ) },
	{ "idBlockAlloc < entityNetEvent_t , 32 >", "eventAllocator", (intptr_t)(&((idEventQueue *)0)->eventAllocator), sizeof( ((idEventQueue *)0)->eventAllocator ) },
	{ NULL, 0 }
};

static classVariableInfo_t timeState_t_typeInfo[] = {
	{ "int", "time", (intptr_t)(&((timeState_t *)0)->time), sizeof( ((timeState_t *)0)->time ) },
	{ "int", "previousTime", (intptr_t)(&((timeState_t *)0)->previousTime), sizeof( ((timeState_t *)0)->previousTime ) },
	{ "int", "msec", (intptr_t)(&((timeState_t *)0)->msec), sizeof( ((timeState_t *)0)->msec ) },
	{ "int", "framenum", (intptr_t)(&((timeState_t *)0)->framenum), sizeof( ((timeState_t *)0)->framenum ) },
	{ "int", "realClientTime", (intptr_t)(&((timeState_t *)0)->realClientTime), sizeof( ((timeState_t *)0)->realClientTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvmGameDelayRemoveEntry_t_typeInfo[] = {
	{ "int", "removeTime", (intptr_t)(&((rvmGameDelayRemoveEntry_t *)0)->removeTime), sizeof( ((rvmGameDelayRemoveEntry_t *)0)->removeTime ) },
	{ "idEntity *", "entity", (intptr_t)(&((rvmGameDelayRemoveEntry_t *)0)->entity), sizeof( ((rvmGameDelayRemoveEntry_t *)0)->entity ) },
	{ NULL, 0 }
};

static classVariableInfo_t idGameLocal_typeInfo[] = {
	{ ": idDict", "serverInfo", (intptr_t)(&((idGameLocal *)0)->serverInfo), sizeof( ((idGameLocal *)0)->serverInfo ) },
	{ "int", "numClients", (intptr_t)(&((idGameLocal *)0)->numClients), sizeof( ((idGameLocal *)0)->numClients ) },
	{ "idDict[32]", "userInfo", (intptr_t)(&((idGameLocal *)0)->userInfo), sizeof( ((idGameLocal *)0)->userInfo ) },
	{ "usercmd_t[32]", "usercmds", (intptr_t)(&((idGameLocal *)0)->usercmds), sizeof( ((idGameLocal *)0)->usercmds ) },
	{ "idDict[32]", "persistentPlayerInfo", (intptr_t)(&((idGameLocal *)0)->persistentPlayerInfo), sizeof( ((idGameLocal *)0)->persistentPlayerInfo ) },
	{ "idEntity *[4096]", "entities", (intptr_t)(&((idGameLocal *)0)->entities), sizeof( ((idGameLocal *)0)->entities ) },
	{ "int[4096]", "spawnIds", (intptr_t)(&((idGameLocal *)0)->spawnIds), sizeof( ((idGameLocal *)0)->spawnIds ) },
	{ "int", "firstFreeIndex", (intptr_t)(&((idGameLocal *)0)->firstFreeIndex), sizeof( ((idGameLocal *)0)->firstFreeIndex ) },
	{ "int", "num_entities", (intptr_t)(&((idGameLocal *)0)->num_entities), sizeof( ((idGameLocal *)0)->num_entities ) },
	{ "idHashIndex", "entityHash", (intptr_t)(&((idGameLocal *)0)->entityHash), sizeof( ((idGameLocal *)0)->entityHash ) },
	{ "idWorldspawn *", "world", (intptr_t)(&((idGameLocal *)0)->world), sizeof( ((idGameLocal *)0)->world ) },
	{ "idLinkList < idEntity >", "spawnedEntities", (intptr_t)(&((idGameLocal *)0)->spawnedEntities), sizeof( ((idGameLocal *)0)->spawnedEntities ) },
	{ "idLinkList < idEntity >", "activeEntities", (intptr_t)(&((idGameLocal *)0)->activeEntities), sizeof( ((idGameLocal *)0)->activeEntities ) },
	{ "int", "numEntitiesToDeactivate", (intptr_t)(&((idGameLocal *)0)->numEntitiesToDeactivate), sizeof( ((idGameLocal *)0)->numEntitiesToDeactivate ) },
	{ "bool", "sortPushers", (intptr_t)(&((idGameLocal *)0)->sortPushers), sizeof( ((idGameLocal *)0)->sortPushers ) },
	{ "bool", "sortTeamMasters", (intptr_t)(&((idGameLocal *)0)->sortTeamMasters), sizeof( ((idGameLocal *)0)->sortTeamMasters ) },
	{ "idDict", "persistentLevelInfo", (intptr_t)(&((idGameLocal *)0)->persistentLevelInfo), sizeof( ((idGameLocal *)0)->persistentLevelInfo ) },
	{ "float[1]", "globalShaderParms", (intptr_t)(&((idGameLocal *)0)->globalShaderParms), sizeof( ((idGameLocal *)0)->globalShaderParms ) },
	{ "idRandom", "random", (intptr_t)(&((idGameLocal *)0)->random), sizeof( ((idGameLocal *)0)->random ) },
	{ "idProgram", "program", (intptr_t)(&((idGameLocal *)0)->program), sizeof( ((idGameLocal *)0)->program ) },
	{ "idThread *", "frameCommandThread", (intptr_t)(&((idGameLocal *)0)->frameCommandThread), sizeof( ((idGameLocal *)0)->frameCommandThread ) },
	{ "idClip", "clip", (intptr_t)(&((idGameLocal *)0)->clip), sizeof( ((idGameLocal *)0)->clip ) },
	{ "idPush", "push", (intptr_t)(&((idGameLocal *)0)->push), sizeof( ((idGameLocal *)0)->push ) },
	{ "idPVS", "pvs", (intptr_t)(&((idGameLocal *)0)->pvs), sizeof( ((idGameLocal *)0)->pvs ) },
	{ "idTestModel *", "testmodel", (intptr_t)(&((idGameLocal *)0)->testmodel), sizeof( ((idGameLocal *)0)->testmodel ) },
	{ "idEntityFx *", "testFx", (intptr_t)(&((idGameLocal *)0)->testFx), sizeof( ((idGameLocal *)0)->testFx ) },
	{ "idStr", "sessionCommand", (intptr_t)(&((idGameLocal *)0)->sessionCommand), sizeof( ((idGameLocal *)0)->sessionCommand ) },
	{ "idMultiplayerGame", "mpGame", (intptr_t)(&((idGameLocal *)0)->mpGame), sizeof( ((idGameLocal *)0)->mpGame ) },
	{ "idSmokeParticles *", "smokeParticles", (intptr_t)(&((idGameLocal *)0)->smokeParticles), sizeof( ((idGameLocal *)0)->smokeParticles ) },
	{ "idEditEntities *", "editEntities", (intptr_t)(&((idGameLocal *)0)->editEntities), sizeof( ((idGameLocal *)0)->editEntities ) },
	{ "int", "cinematicSkipTime", (intptr_t)(&((idGameLocal *)0)->cinematicSkipTime), sizeof( ((idGameLocal *)0)->cinematicSkipTime ) },
	{ "int", "cinematicStopTime", (intptr_t)(&((idGameLocal *)0)->cinematicStopTime), sizeof( ((idGameLocal *)0)->cinematicStopTime ) },
	{ "int", "cinematicMaxSkipTime", (intptr_t)(&((idGameLocal *)0)->cinematicMaxSkipTime), sizeof( ((idGameLocal *)0)->cinematicMaxSkipTime ) },
	{ "bool", "inCinematic", (intptr_t)(&((idGameLocal *)0)->inCinematic), sizeof( ((idGameLocal *)0)->inCinematic ) },
	{ "bool", "skipCinematic", (intptr_t)(&((idGameLocal *)0)->skipCinematic), sizeof( ((idGameLocal *)0)->skipCinematic ) },
	{ "int", "framenum", (intptr_t)(&((idGameLocal *)0)->framenum), sizeof( ((idGameLocal *)0)->framenum ) },
	{ "int", "previousTime", (intptr_t)(&((idGameLocal *)0)->previousTime), sizeof( ((idGameLocal *)0)->previousTime ) },
	{ "int", "time", (intptr_t)(&((idGameLocal *)0)->time), sizeof( ((idGameLocal *)0)->time ) },
	{ "int", "msec", (intptr_t)(&((idGameLocal *)0)->msec), sizeof( ((idGameLocal *)0)->msec ) },
	{ "int", "vacuumAreaNum", (intptr_t)(&((idGameLocal *)0)->vacuumAreaNum), sizeof( ((idGameLocal *)0)->vacuumAreaNum ) },
	{ "gameType_t", "gameType", (intptr_t)(&((idGameLocal *)0)->gameType), sizeof( ((idGameLocal *)0)->gameType ) },
	{ "bool", "isMultiplayer", (intptr_t)(&((idGameLocal *)0)->isMultiplayer), sizeof( ((idGameLocal *)0)->isMultiplayer ) },
	{ "bool", "isServer", (intptr_t)(&((idGameLocal *)0)->isServer), sizeof( ((idGameLocal *)0)->isServer ) },
	{ "bool", "isClient", (intptr_t)(&((idGameLocal *)0)->isClient), sizeof( ((idGameLocal *)0)->isClient ) },
	{ "int", "localClientNum", (intptr_t)(&((idGameLocal *)0)->localClientNum), sizeof( ((idGameLocal *)0)->localClientNum ) },
	{ "idLinkList < idEntity >", "snapshotEntities", (intptr_t)(&((idGameLocal *)0)->snapshotEntities), sizeof( ((idGameLocal *)0)->snapshotEntities ) },
	{ "int", "realClientTime", (intptr_t)(&((idGameLocal *)0)->realClientTime), sizeof( ((idGameLocal *)0)->realClientTime ) },
	{ "bool", "isNewFrame", (intptr_t)(&((idGameLocal *)0)->isNewFrame), sizeof( ((idGameLocal *)0)->isNewFrame ) },
	{ "float", "clientSmoothing", (intptr_t)(&((idGameLocal *)0)->clientSmoothing), sizeof( ((idGameLocal *)0)->clientSmoothing ) },
	{ "int", "entityDefBits", (intptr_t)(&((idGameLocal *)0)->entityDefBits), sizeof( ((idGameLocal *)0)->entityDefBits ) },
	{ "idEntityPtr < idEntity >", "lastGUIEnt", (intptr_t)(&((idGameLocal *)0)->lastGUIEnt), sizeof( ((idGameLocal *)0)->lastGUIEnt ) },
	{ "int", "lastGUI", (intptr_t)(&((idGameLocal *)0)->lastGUI), sizeof( ((idGameLocal *)0)->lastGUI ) },
	{ "timeState_t", "fast", (intptr_t)(&((idGameLocal *)0)->fast), sizeof( ((idGameLocal *)0)->fast ) },
	{ "timeState_t", "slow", (intptr_t)(&((idGameLocal *)0)->slow), sizeof( ((idGameLocal *)0)->slow ) },
	{ "slowmoState_t", "slowmoState", (intptr_t)(&((idGameLocal *)0)->slowmoState), sizeof( ((idGameLocal *)0)->slowmoState ) },
	{ "float", "slowmoMsec", (intptr_t)(&((idGameLocal *)0)->slowmoMsec), sizeof( ((idGameLocal *)0)->slowmoMsec ) },
	{ "bool", "quickSlowmoReset", (intptr_t)(&((idGameLocal *)0)->quickSlowmoReset), sizeof( ((idGameLocal *)0)->quickSlowmoReset ) },
	{ "rvmNavFile *", "navFile", (intptr_t)(&((idGameLocal *)0)->navFile), sizeof( ((idGameLocal *)0)->navFile ) },
	{ "idStr", "mapFileName", (intptr_t)(&((idGameLocal *)0)->mapFileName), sizeof( ((idGameLocal *)0)->mapFileName ) },
	{ "idMapFile *", "mapFile", (intptr_t)(&((idGameLocal *)0)->mapFile), sizeof( ((idGameLocal *)0)->mapFile ) },
	{ "bool", "mapCycleLoaded", (intptr_t)(&((idGameLocal *)0)->mapCycleLoaded), sizeof( ((idGameLocal *)0)->mapCycleLoaded ) },
	{ "int", "spawnCount", (intptr_t)(&((idGameLocal *)0)->spawnCount), sizeof( ((idGameLocal *)0)->spawnCount ) },
	{ "int", "mapSpawnCount", (intptr_t)(&((idGameLocal *)0)->mapSpawnCount), sizeof( ((idGameLocal *)0)->mapSpawnCount ) },
	{ "idLocationEntity * *", "locationEntities", (intptr_t)(&((idGameLocal *)0)->locationEntities), sizeof( ((idGameLocal *)0)->locationEntities ) },
	{ "idCamera *", "camera", (intptr_t)(&((idGameLocal *)0)->camera), sizeof( ((idGameLocal *)0)->camera ) },
	{ "const idMaterial *", "globalMaterial", (intptr_t)(&((idGameLocal *)0)->globalMaterial), sizeof( ((idGameLocal *)0)->globalMaterial ) },
	{ "idEntityPtr < idActor >", "lastAIAlertEntity", (intptr_t)(&((idGameLocal *)0)->lastAIAlertEntity), sizeof( ((idGameLocal *)0)->lastAIAlertEntity ) },
	{ "int", "lastAIAlertTime", (intptr_t)(&((idGameLocal *)0)->lastAIAlertTime), sizeof( ((idGameLocal *)0)->lastAIAlertTime ) },
	{ "pvsHandle_t", "playerPVS", (intptr_t)(&((idGameLocal *)0)->playerPVS), sizeof( ((idGameLocal *)0)->playerPVS ) },
	{ "pvsHandle_t", "playerConnectedAreas", (intptr_t)(&((idGameLocal *)0)->playerConnectedAreas), sizeof( ((idGameLocal *)0)->playerConnectedAreas ) },
	{ "idVec3", "gravity", (intptr_t)(&((idGameLocal *)0)->gravity), sizeof( ((idGameLocal *)0)->gravity ) },
	{ "gameState_t", "gamestate", (intptr_t)(&((idGameLocal *)0)->gamestate), sizeof( ((idGameLocal *)0)->gamestate ) },
	{ "bool", "influenceActive", (intptr_t)(&((idGameLocal *)0)->influenceActive), sizeof( ((idGameLocal *)0)->influenceActive ) },
	{ "int", "nextGibTime", (intptr_t)(&((idGameLocal *)0)->nextGibTime), sizeof( ((idGameLocal *)0)->nextGibTime ) },
	{ "idList < int >[32]", "clientDeclRemap", (intptr_t)(&((idGameLocal *)0)->clientDeclRemap), sizeof( ((idGameLocal *)0)->clientDeclRemap ) },
	{ "entityState_t *[131072]", "clientEntityStates", (intptr_t)(&((idGameLocal *)0)->clientEntityStates), sizeof( ((idGameLocal *)0)->clientEntityStates ) },
	{ "int[4096]", "clientPVS", (intptr_t)(&((idGameLocal *)0)->clientPVS), sizeof( ((idGameLocal *)0)->clientPVS ) },
	{ "snapshot_t *[32]", "clientSnapshots", (intptr_t)(&((idGameLocal *)0)->clientSnapshots), sizeof( ((idGameLocal *)0)->clientSnapshots ) },
	{ "idBlockAlloc < entityState_t , 256 >", "entityStateAllocator", (intptr_t)(&((idGameLocal *)0)->entityStateAllocator), sizeof( ((idGameLocal *)0)->entityStateAllocator ) },
	{ "idBlockAlloc < snapshot_t , 64 >", "snapshotAllocator", (intptr_t)(&((idGameLocal *)0)->snapshotAllocator), sizeof( ((idGameLocal *)0)->snapshotAllocator ) },
	{ "idEventQueue", "eventQueue", (intptr_t)(&((idGameLocal *)0)->eventQueue), sizeof( ((idGameLocal *)0)->eventQueue ) },
	{ "idEventQueue", "savedEventQueue", (intptr_t)(&((idGameLocal *)0)->savedEventQueue), sizeof( ((idGameLocal *)0)->savedEventQueue ) },
	{ "idStaticList < spawnSpot_t , ( 1 << 12 ) >", "spawnSpots", (intptr_t)(&((idGameLocal *)0)->spawnSpots), sizeof( ((idGameLocal *)0)->spawnSpots ) },
	{ "idStaticList < idEntity * , ( 1 << 12 ) >", "initialSpots", (intptr_t)(&((idGameLocal *)0)->initialSpots), sizeof( ((idGameLocal *)0)->initialSpots ) },
	{ "int", "currentInitialSpot", (intptr_t)(&((idGameLocal *)0)->currentInitialSpot), sizeof( ((idGameLocal *)0)->currentInitialSpot ) },
	{ "idStaticList < spawnSpot_t , ( 1 << 12 ) >[2]", "teamSpawnSpots", (intptr_t)(&((idGameLocal *)0)->teamSpawnSpots), sizeof( ((idGameLocal *)0)->teamSpawnSpots ) },
	{ "idStaticList < idEntity * , ( 1 << 12 ) >[2]", "teamInitialSpots", (intptr_t)(&((idGameLocal *)0)->teamInitialSpots), sizeof( ((idGameLocal *)0)->teamInitialSpots ) },
	{ "int[2]", "teamCurrentInitialSpot", (intptr_t)(&((idGameLocal *)0)->teamCurrentInitialSpot), sizeof( ((idGameLocal *)0)->teamCurrentInitialSpot ) },
	{ "idDict", "newInfo", (intptr_t)(&((idGameLocal *)0)->newInfo), sizeof( ((idGameLocal *)0)->newInfo ) },
	{ "idStrList", "shakeSounds", (intptr_t)(&((idGameLocal *)0)->shakeSounds), sizeof( ((idGameLocal *)0)->shakeSounds ) },
	{ "byte[16384]", "lagometer", (intptr_t)(&((idGameLocal *)0)->lagometer), sizeof( ((idGameLocal *)0)->lagometer ) },
	{ "idList < rvmGameDelayRemoveEntry_t >", "delayRemoveEntities", (intptr_t)(&((idGameLocal *)0)->delayRemoveEntities), sizeof( ((idGameLocal *)0)->delayRemoveEntities ) },
	{ ": idDict", "spawnArgs", (intptr_t)(&((idGameLocal *)0)->spawnArgs), sizeof( ((idGameLocal *)0)->spawnArgs ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnFullscreenRenderTarget_typeInfo[] = {
	{ ": idImage *[4]", "albedoImage", (intptr_t)(&((DnFullscreenRenderTarget *)0)->albedoImage), sizeof( ((DnFullscreenRenderTarget *)0)->albedoImage ) },
	{ "idImage *", "depthImage", (intptr_t)(&((DnFullscreenRenderTarget *)0)->depthImage), sizeof( ((DnFullscreenRenderTarget *)0)->depthImage ) },
	{ "idRenderTexture *", "renderTexture", (intptr_t)(&((DnFullscreenRenderTarget *)0)->renderTexture), sizeof( ((DnFullscreenRenderTarget *)0)->renderTexture ) },
	{ "int", "numMultiSamples", (intptr_t)(&((DnFullscreenRenderTarget *)0)->numMultiSamples), sizeof( ((DnFullscreenRenderTarget *)0)->numMultiSamples ) },
	{ NULL, 0 }
};

static classVariableInfo_t dnEditorLight_typeInfo[] = {
	{ "qhandle_t", "renderLightHandle", (intptr_t)(&((dnEditorLight *)0)->renderLightHandle), sizeof( ((dnEditorLight *)0)->renderLightHandle ) },
	{ "renderLight_t", "renderLightParams", (intptr_t)(&((dnEditorLight *)0)->renderLightParams), sizeof( ((dnEditorLight *)0)->renderLightParams ) },
	{ NULL, 0 }
};

static classVariableInfo_t dnEditorModel_typeInfo[] = {
	{ ": qhandle_t", "renderEntityHandle", (intptr_t)(&((dnEditorModel *)0)->renderEntityHandle), sizeof( ((dnEditorModel *)0)->renderEntityHandle ) },
	{ "renderEntity_t", "renderEntityParams", (intptr_t)(&((dnEditorModel *)0)->renderEntityParams), sizeof( ((dnEditorModel *)0)->renderEntityParams ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnRenderPlatform_typeInfo[] = {
	{ "DnFullscreenRenderTarget *", "frontEndPassRenderTarget", (intptr_t)(&((DnRenderPlatform *)0)->frontEndPassRenderTarget), sizeof( ((DnRenderPlatform *)0)->frontEndPassRenderTarget ) },
	{ "DnFullscreenRenderTarget *", "frontEndPassRenderTargetResolved", (intptr_t)(&((DnRenderPlatform *)0)->frontEndPassRenderTargetResolved), sizeof( ((DnRenderPlatform *)0)->frontEndPassRenderTargetResolved ) },
	{ "DnFullscreenRenderTarget *", "ssaoRenderTarget", (intptr_t)(&((DnRenderPlatform *)0)->ssaoRenderTarget), sizeof( ((DnRenderPlatform *)0)->ssaoRenderTarget ) },
	{ "const idMaterial *", "upscaleFrontEndResolveMaterial", (intptr_t)(&((DnRenderPlatform *)0)->upscaleFrontEndResolveMaterial), sizeof( ((DnRenderPlatform *)0)->upscaleFrontEndResolveMaterial ) },
	{ "const idMaterial *", "ssaoMaterial", (intptr_t)(&((DnRenderPlatform *)0)->ssaoMaterial), sizeof( ((DnRenderPlatform *)0)->ssaoMaterial ) },
	{ "const idMaterial *", "ssaoBlurMaterial", (intptr_t)(&((DnRenderPlatform *)0)->ssaoBlurMaterial), sizeof( ((DnRenderPlatform *)0)->ssaoBlurMaterial ) },
	{ "const idMaterial *", "bloomMaterial", (intptr_t)(&((DnRenderPlatform *)0)->bloomMaterial), sizeof( ((DnRenderPlatform *)0)->bloomMaterial ) },
	{ "const idMaterial *", "blackMaterial", (intptr_t)(&((DnRenderPlatform *)0)->blackMaterial), sizeof( ((DnRenderPlatform *)0)->blackMaterial ) },
	{ NULL, 0 }
};

static classVariableInfo_t dnGameLocal_typeInfo[] = {
	{ ": DnRenderPlatform", "renderPlatform", (intptr_t)(&((dnGameLocal *)0)->renderPlatform), sizeof( ((dnGameLocal *)0)->renderPlatform ) },
	{ "idUserInterface *", "guiMainMenu", (intptr_t)(&((dnGameLocal *)0)->guiMainMenu), sizeof( ((dnGameLocal *)0)->guiMainMenu ) },
	{ "rvClientEntity *[4096]", "clientEntities", (intptr_t)(&((dnGameLocal *)0)->clientEntities), sizeof( ((dnGameLocal *)0)->clientEntities ) },
	{ "int[4096]", "clientSpawnIds", (intptr_t)(&((dnGameLocal *)0)->clientSpawnIds), sizeof( ((dnGameLocal *)0)->clientSpawnIds ) },
	{ "idLinkList < rvClientEntity >", "clientSpawnedEntities", (intptr_t)(&((dnGameLocal *)0)->clientSpawnedEntities), sizeof( ((dnGameLocal *)0)->clientSpawnedEntities ) },
	{ "int", "num_clientEntities", (intptr_t)(&((dnGameLocal *)0)->num_clientEntities), sizeof( ((dnGameLocal *)0)->num_clientEntities ) },
	{ "int", "firstFreeClientIndex", (intptr_t)(&((dnGameLocal *)0)->firstFreeClientIndex), sizeof( ((dnGameLocal *)0)->firstFreeClientIndex ) },
	{ "int", "clientSpawnCount", (intptr_t)(&((dnGameLocal *)0)->clientSpawnCount), sizeof( ((dnGameLocal *)0)->clientSpawnCount ) },
	{ "int", "entityRegisterTime", (intptr_t)(&((dnGameLocal *)0)->entityRegisterTime), sizeof( ((dnGameLocal *)0)->entityRegisterTime ) },
	{ "idSysMutex", "clientGamePhysicsMutex", (intptr_t)(&((dnGameLocal *)0)->clientGamePhysicsMutex), sizeof( ((dnGameLocal *)0)->clientGamePhysicsMutex ) },
	{ "idParallelJobList *", "clientPhysicsJob", (intptr_t)(&((dnGameLocal *)0)->clientPhysicsJob), sizeof( ((dnGameLocal *)0)->clientPhysicsJob ) },
	{ "idList < rvClientEntity * >", "clientEntityThreadWork", (intptr_t)(&((dnGameLocal *)0)->clientEntityThreadWork), sizeof( ((dnGameLocal *)0)->clientEntityThreadWork ) },
	{ NULL, 0 }
};

static classVariableInfo_t idGameError_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t stateParms_t_typeInfo[] = {
	{ "int", "blendFrames", (intptr_t)(&((stateParms_t *)0)->blendFrames), sizeof( ((stateParms_t *)0)->blendFrames ) },
	{ "int", "time", (intptr_t)(&((stateParms_t *)0)->time), sizeof( ((stateParms_t *)0)->time ) },
	{ "int", "stage", (intptr_t)(&((stateParms_t *)0)->stage), sizeof( ((stateParms_t *)0)->stage ) },
	{ "int", "substage", (intptr_t)(&((stateParms_t *)0)->substage), sizeof( ((stateParms_t *)0)->substage ) },
	{ "float", "param1", (intptr_t)(&((stateParms_t *)0)->param1), sizeof( ((stateParms_t *)0)->param1 ) },
	{ "float", "param2", (intptr_t)(&((stateParms_t *)0)->param2), sizeof( ((stateParms_t *)0)->param2 ) },
	{ "float", "subparam1", (intptr_t)(&((stateParms_t *)0)->subparam1), sizeof( ((stateParms_t *)0)->subparam1 ) },
	{ "float", "subparam2", (intptr_t)(&((stateParms_t *)0)->subparam2), sizeof( ((stateParms_t *)0)->subparam2 ) },
	{ NULL, 0 }
};

static classVariableInfo_t stateCall_t_typeInfo[] = {
	{ "idStr", "state", (intptr_t)(&((stateCall_t *)0)->state), sizeof( ((stateCall_t *)0)->state ) },
	{ "idLinkList < stateCall_t >", "node", (intptr_t)(&((stateCall_t *)0)->node), sizeof( ((stateCall_t *)0)->node ) },
	{ "int", "flags", (intptr_t)(&((stateCall_t *)0)->flags), sizeof( ((stateCall_t *)0)->flags ) },
	{ "int", "delay", (intptr_t)(&((stateCall_t *)0)->delay), sizeof( ((stateCall_t *)0)->delay ) },
	{ "stateParms_t", "parms", (intptr_t)(&((stateCall_t *)0)->parms), sizeof( ((stateCall_t *)0)->parms ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvStateThread_flags_typeInfo[] = {
//	{ "bool", "stateCleared", (intptr_t)(&((rvStateThread::flags *)0)->stateCleared), sizeof( ((rvStateThread::flags *)0)->stateCleared ) },
//	{ "bool", "stateInterrupted", (intptr_t)(&((rvStateThread::flags *)0)->stateInterrupted), sizeof( ((rvStateThread::flags *)0)->stateInterrupted ) },
//	{ "bool", "executing", (intptr_t)(&((rvStateThread::flags *)0)->executing), sizeof( ((rvStateThread::flags *)0)->executing ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvStateThread_typeInfo[] = {
	{ "rvStateThread::flags", "fl", (intptr_t)(&((rvStateThread *)0)->fl), sizeof( ((rvStateThread *)0)->fl ) },
	{ "idStr", "name", (intptr_t)(&((rvStateThread *)0)->name), sizeof( ((rvStateThread *)0)->name ) },
	{ "idClass *", "owner", (intptr_t)(&((rvStateThread *)0)->owner), sizeof( ((rvStateThread *)0)->owner ) },
	{ "idLinkList < stateCall_t >", "states", (intptr_t)(&((rvStateThread *)0)->states), sizeof( ((rvStateThread *)0)->states ) },
	{ "idLinkList < stateCall_t >", "interrupted", (intptr_t)(&((rvStateThread *)0)->interrupted), sizeof( ((rvStateThread *)0)->interrupted ) },
	{ "stateCall_t *", "insertAfter", (intptr_t)(&((rvStateThread *)0)->insertAfter), sizeof( ((rvStateThread *)0)->insertAfter ) },
	{ "stateResult_t", "lastResult", (intptr_t)(&((rvStateThread *)0)->lastResult), sizeof( ((rvStateThread *)0)->lastResult ) },
	{ NULL, 0 }
};

static classVariableInfo_t idForce_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idForce_Constant_typeInfo[] = {
	{ ": idVec3", "force", (intptr_t)(&((idForce_Constant *)0)->force), sizeof( ((idForce_Constant *)0)->force ) },
	{ "idPhysics *", "physics", (intptr_t)(&((idForce_Constant *)0)->physics), sizeof( ((idForce_Constant *)0)->physics ) },
	{ "int", "id", (intptr_t)(&((idForce_Constant *)0)->id), sizeof( ((idForce_Constant *)0)->id ) },
	{ "idVec3", "point", (intptr_t)(&((idForce_Constant *)0)->point), sizeof( ((idForce_Constant *)0)->point ) },
	{ NULL, 0 }
};

static classVariableInfo_t idForce_Drag_typeInfo[] = {
	{ ": float", "damping", (intptr_t)(&((idForce_Drag *)0)->damping), sizeof( ((idForce_Drag *)0)->damping ) },
	{ "idPhysics *", "physics", (intptr_t)(&((idForce_Drag *)0)->physics), sizeof( ((idForce_Drag *)0)->physics ) },
	{ "int", "id", (intptr_t)(&((idForce_Drag *)0)->id), sizeof( ((idForce_Drag *)0)->id ) },
	{ "idVec3", "p", (intptr_t)(&((idForce_Drag *)0)->p), sizeof( ((idForce_Drag *)0)->p ) },
	{ "idVec3", "dragPosition", (intptr_t)(&((idForce_Drag *)0)->dragPosition), sizeof( ((idForce_Drag *)0)->dragPosition ) },
	{ NULL, 0 }
};

static classVariableInfo_t idForce_Grab_typeInfo[] = {
	{ ": float", "damping", (intptr_t)(&((idForce_Grab *)0)->damping), sizeof( ((idForce_Grab *)0)->damping ) },
	{ "idVec3", "goalPosition", (intptr_t)(&((idForce_Grab *)0)->goalPosition), sizeof( ((idForce_Grab *)0)->goalPosition ) },
	{ "float", "distanceToGoal", (intptr_t)(&((idForce_Grab *)0)->distanceToGoal), sizeof( ((idForce_Grab *)0)->distanceToGoal ) },
	{ "idPhysics *", "physics", (intptr_t)(&((idForce_Grab *)0)->physics), sizeof( ((idForce_Grab *)0)->physics ) },
	{ "int", "id", (intptr_t)(&((idForce_Grab *)0)->id), sizeof( ((idForce_Grab *)0)->id ) },
	{ NULL, 0 }
};

static classVariableInfo_t idForce_Field_typeInfo[] = {
	{ ": forceFieldType", "type", (intptr_t)(&((idForce_Field *)0)->type), sizeof( ((idForce_Field *)0)->type ) },
	{ "forceFieldApplyType", "applyType", (intptr_t)(&((idForce_Field *)0)->applyType), sizeof( ((idForce_Field *)0)->applyType ) },
	{ "float", "magnitude", (intptr_t)(&((idForce_Field *)0)->magnitude), sizeof( ((idForce_Field *)0)->magnitude ) },
	{ "idVec3", "dir", (intptr_t)(&((idForce_Field *)0)->dir), sizeof( ((idForce_Field *)0)->dir ) },
	{ "float", "randomTorque", (intptr_t)(&((idForce_Field *)0)->randomTorque), sizeof( ((idForce_Field *)0)->randomTorque ) },
	{ "bool", "playerOnly", (intptr_t)(&((idForce_Field *)0)->playerOnly), sizeof( ((idForce_Field *)0)->playerOnly ) },
	{ "bool", "monsterOnly", (intptr_t)(&((idForce_Field *)0)->monsterOnly), sizeof( ((idForce_Field *)0)->monsterOnly ) },
	{ "idClipModel *", "clipModel", (intptr_t)(&((idForce_Field *)0)->clipModel), sizeof( ((idForce_Field *)0)->clipModel ) },
	{ NULL, 0 }
};

static classVariableInfo_t idForce_Spring_typeInfo[] = {
	{ ": float", "Kstretch", (intptr_t)(&((idForce_Spring *)0)->Kstretch), sizeof( ((idForce_Spring *)0)->Kstretch ) },
	{ "float", "Kcompress", (intptr_t)(&((idForce_Spring *)0)->Kcompress), sizeof( ((idForce_Spring *)0)->Kcompress ) },
	{ "float", "damping", (intptr_t)(&((idForce_Spring *)0)->damping), sizeof( ((idForce_Spring *)0)->damping ) },
	{ "float", "restLength", (intptr_t)(&((idForce_Spring *)0)->restLength), sizeof( ((idForce_Spring *)0)->restLength ) },
	{ "idPhysics *", "physics1", (intptr_t)(&((idForce_Spring *)0)->physics1), sizeof( ((idForce_Spring *)0)->physics1 ) },
	{ "int", "id1", (intptr_t)(&((idForce_Spring *)0)->id1), sizeof( ((idForce_Spring *)0)->id1 ) },
	{ "idVec3", "p1", (intptr_t)(&((idForce_Spring *)0)->p1), sizeof( ((idForce_Spring *)0)->p1 ) },
	{ "idPhysics *", "physics2", (intptr_t)(&((idForce_Spring *)0)->physics2), sizeof( ((idForce_Spring *)0)->physics2 ) },
	{ "int", "id2", (intptr_t)(&((idForce_Spring *)0)->id2), sizeof( ((idForce_Spring *)0)->id2 ) },
	{ "idVec3", "p2", (intptr_t)(&((idForce_Spring *)0)->p2), sizeof( ((idForce_Spring *)0)->p2 ) },
	{ NULL, 0 }
};

static classVariableInfo_t impactInfo_t_typeInfo[] = {
	{ "float", "invMass", (intptr_t)(&((impactInfo_t *)0)->invMass), sizeof( ((impactInfo_t *)0)->invMass ) },
	{ "idMat3", "invInertiaTensor", (intptr_t)(&((impactInfo_t *)0)->invInertiaTensor), sizeof( ((impactInfo_t *)0)->invInertiaTensor ) },
	{ "idVec3", "position", (intptr_t)(&((impactInfo_t *)0)->position), sizeof( ((impactInfo_t *)0)->position ) },
	{ "idVec3", "velocity", (intptr_t)(&((impactInfo_t *)0)->velocity), sizeof( ((impactInfo_t *)0)->velocity ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t staticPState_t_typeInfo[] = {
	{ "idVec3", "origin", (intptr_t)(&((staticPState_t *)0)->origin), sizeof( ((staticPState_t *)0)->origin ) },
	{ "idMat3", "axis", (intptr_t)(&((staticPState_t *)0)->axis), sizeof( ((staticPState_t *)0)->axis ) },
	{ "idVec3", "localOrigin", (intptr_t)(&((staticPState_t *)0)->localOrigin), sizeof( ((staticPState_t *)0)->localOrigin ) },
	{ "idMat3", "localAxis", (intptr_t)(&((staticPState_t *)0)->localAxis), sizeof( ((staticPState_t *)0)->localAxis ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_Static_typeInfo[] = {
	{ ": idEntity *", "self", (intptr_t)(&((idPhysics_Static *)0)->self), sizeof( ((idPhysics_Static *)0)->self ) },
	{ "staticPState_t", "current", (intptr_t)(&((idPhysics_Static *)0)->current), sizeof( ((idPhysics_Static *)0)->current ) },
	{ "idClipModel *", "clipModel", (intptr_t)(&((idPhysics_Static *)0)->clipModel), sizeof( ((idPhysics_Static *)0)->clipModel ) },
	{ "bool", "hasMaster", (intptr_t)(&((idPhysics_Static *)0)->hasMaster), sizeof( ((idPhysics_Static *)0)->hasMaster ) },
	{ "bool", "isOrientated", (intptr_t)(&((idPhysics_Static *)0)->isOrientated), sizeof( ((idPhysics_Static *)0)->isOrientated ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_StaticMulti_typeInfo[] = {
	{ ": idEntity *", "self", (intptr_t)(&((idPhysics_StaticMulti *)0)->self), sizeof( ((idPhysics_StaticMulti *)0)->self ) },
	{ "idList < staticPState_t >", "current", (intptr_t)(&((idPhysics_StaticMulti *)0)->current), sizeof( ((idPhysics_StaticMulti *)0)->current ) },
	{ "idList < idClipModel * >", "clipModels", (intptr_t)(&((idPhysics_StaticMulti *)0)->clipModels), sizeof( ((idPhysics_StaticMulti *)0)->clipModels ) },
	{ "bool", "hasMaster", (intptr_t)(&((idPhysics_StaticMulti *)0)->hasMaster), sizeof( ((idPhysics_StaticMulti *)0)->hasMaster ) },
	{ "bool", "isOrientated", (intptr_t)(&((idPhysics_StaticMulti *)0)->isOrientated), sizeof( ((idPhysics_StaticMulti *)0)->isOrientated ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_Base_typeInfo[] = {
	{ ": idEntity *", "self", (intptr_t)(&((idPhysics_Base *)0)->self), sizeof( ((idPhysics_Base *)0)->self ) },
	{ "int", "clipMask", (intptr_t)(&((idPhysics_Base *)0)->clipMask), sizeof( ((idPhysics_Base *)0)->clipMask ) },
	{ "idVec3", "gravityVector", (intptr_t)(&((idPhysics_Base *)0)->gravityVector), sizeof( ((idPhysics_Base *)0)->gravityVector ) },
	{ "idVec3", "gravityNormal", (intptr_t)(&((idPhysics_Base *)0)->gravityNormal), sizeof( ((idPhysics_Base *)0)->gravityNormal ) },
	{ "idList < contactInfo_t >", "contacts", (intptr_t)(&((idPhysics_Base *)0)->contacts), sizeof( ((idPhysics_Base *)0)->contacts ) },
	{ "idList < idEntityPtr < idEntity > >", "contactEntities", (intptr_t)(&((idPhysics_Base *)0)->contactEntities), sizeof( ((idPhysics_Base *)0)->contactEntities ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_Actor_typeInfo[] = {
	{ ": idClipModel *", "clipModel", (intptr_t)(&((idPhysics_Actor *)0)->clipModel), sizeof( ((idPhysics_Actor *)0)->clipModel ) },
	{ "idMat3", "clipModelAxis", (intptr_t)(&((idPhysics_Actor *)0)->clipModelAxis), sizeof( ((idPhysics_Actor *)0)->clipModelAxis ) },
	{ "float", "mass", (intptr_t)(&((idPhysics_Actor *)0)->mass), sizeof( ((idPhysics_Actor *)0)->mass ) },
	{ "float", "invMass", (intptr_t)(&((idPhysics_Actor *)0)->invMass), sizeof( ((idPhysics_Actor *)0)->invMass ) },
	{ "idEntity *", "masterEntity", (intptr_t)(&((idPhysics_Actor *)0)->masterEntity), sizeof( ((idPhysics_Actor *)0)->masterEntity ) },
	{ "float", "masterYaw", (intptr_t)(&((idPhysics_Actor *)0)->masterYaw), sizeof( ((idPhysics_Actor *)0)->masterYaw ) },
	{ "float", "masterDeltaYaw", (intptr_t)(&((idPhysics_Actor *)0)->masterDeltaYaw), sizeof( ((idPhysics_Actor *)0)->masterDeltaYaw ) },
	{ "idEntityPtr < idEntity >", "groundEntityPtr", (intptr_t)(&((idPhysics_Actor *)0)->groundEntityPtr), sizeof( ((idPhysics_Actor *)0)->groundEntityPtr ) },
	{ NULL, 0 }
};

static classVariableInfo_t monsterPState_t_typeInfo[] = {
	{ "int", "atRest", (intptr_t)(&((monsterPState_t *)0)->atRest), sizeof( ((monsterPState_t *)0)->atRest ) },
	{ "bool", "onGround", (intptr_t)(&((monsterPState_t *)0)->onGround), sizeof( ((monsterPState_t *)0)->onGround ) },
	{ "idVec3", "origin", (intptr_t)(&((monsterPState_t *)0)->origin), sizeof( ((monsterPState_t *)0)->origin ) },
	{ "idVec3", "velocity", (intptr_t)(&((monsterPState_t *)0)->velocity), sizeof( ((monsterPState_t *)0)->velocity ) },
	{ "idVec3", "localOrigin", (intptr_t)(&((monsterPState_t *)0)->localOrigin), sizeof( ((monsterPState_t *)0)->localOrigin ) },
	{ "idVec3", "pushVelocity", (intptr_t)(&((monsterPState_t *)0)->pushVelocity), sizeof( ((monsterPState_t *)0)->pushVelocity ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_Monster_typeInfo[] = {
	{ ": monsterPState_t", "current", (intptr_t)(&((idPhysics_Monster *)0)->current), sizeof( ((idPhysics_Monster *)0)->current ) },
	{ "monsterPState_t", "saved", (intptr_t)(&((idPhysics_Monster *)0)->saved), sizeof( ((idPhysics_Monster *)0)->saved ) },
	{ "float", "maxStepHeight", (intptr_t)(&((idPhysics_Monster *)0)->maxStepHeight), sizeof( ((idPhysics_Monster *)0)->maxStepHeight ) },
	{ "float", "minFloorCosine", (intptr_t)(&((idPhysics_Monster *)0)->minFloorCosine), sizeof( ((idPhysics_Monster *)0)->minFloorCosine ) },
	{ "idVec3", "delta", (intptr_t)(&((idPhysics_Monster *)0)->delta), sizeof( ((idPhysics_Monster *)0)->delta ) },
	{ "bool", "forceDeltaMove", (intptr_t)(&((idPhysics_Monster *)0)->forceDeltaMove), sizeof( ((idPhysics_Monster *)0)->forceDeltaMove ) },
	{ "bool", "fly", (intptr_t)(&((idPhysics_Monster *)0)->fly), sizeof( ((idPhysics_Monster *)0)->fly ) },
	{ "bool", "useVelocityMove", (intptr_t)(&((idPhysics_Monster *)0)->useVelocityMove), sizeof( ((idPhysics_Monster *)0)->useVelocityMove ) },
	{ "bool", "noImpact", (intptr_t)(&((idPhysics_Monster *)0)->noImpact), sizeof( ((idPhysics_Monster *)0)->noImpact ) },
	{ "monsterMoveResult_t", "moveResult", (intptr_t)(&((idPhysics_Monster *)0)->moveResult), sizeof( ((idPhysics_Monster *)0)->moveResult ) },
	{ "idEntity *", "blockingEntity", (intptr_t)(&((idPhysics_Monster *)0)->blockingEntity), sizeof( ((idPhysics_Monster *)0)->blockingEntity ) },
	{ NULL, 0 }
};

static classVariableInfo_t playerPState_t_typeInfo[] = {
	{ "idVec3", "origin", (intptr_t)(&((playerPState_t *)0)->origin), sizeof( ((playerPState_t *)0)->origin ) },
	{ "idVec3", "velocity", (intptr_t)(&((playerPState_t *)0)->velocity), sizeof( ((playerPState_t *)0)->velocity ) },
	{ "idVec3", "localOrigin", (intptr_t)(&((playerPState_t *)0)->localOrigin), sizeof( ((playerPState_t *)0)->localOrigin ) },
	{ "idVec3", "pushVelocity", (intptr_t)(&((playerPState_t *)0)->pushVelocity), sizeof( ((playerPState_t *)0)->pushVelocity ) },
	{ "float", "stepUp", (intptr_t)(&((playerPState_t *)0)->stepUp), sizeof( ((playerPState_t *)0)->stepUp ) },
	{ "int", "movementType", (intptr_t)(&((playerPState_t *)0)->movementType), sizeof( ((playerPState_t *)0)->movementType ) },
	{ "int", "movementFlags", (intptr_t)(&((playerPState_t *)0)->movementFlags), sizeof( ((playerPState_t *)0)->movementFlags ) },
	{ "int", "movementTime", (intptr_t)(&((playerPState_t *)0)->movementTime), sizeof( ((playerPState_t *)0)->movementTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_Player_typeInfo[] = {
	{ ": playerPState_t", "current", (intptr_t)(&((idPhysics_Player *)0)->current), sizeof( ((idPhysics_Player *)0)->current ) },
	{ "playerPState_t", "saved", (intptr_t)(&((idPhysics_Player *)0)->saved), sizeof( ((idPhysics_Player *)0)->saved ) },
	{ "float", "walkSpeed", (intptr_t)(&((idPhysics_Player *)0)->walkSpeed), sizeof( ((idPhysics_Player *)0)->walkSpeed ) },
	{ "float", "crouchSpeed", (intptr_t)(&((idPhysics_Player *)0)->crouchSpeed), sizeof( ((idPhysics_Player *)0)->crouchSpeed ) },
	{ "float", "maxStepHeight", (intptr_t)(&((idPhysics_Player *)0)->maxStepHeight), sizeof( ((idPhysics_Player *)0)->maxStepHeight ) },
	{ "float", "maxJumpHeight", (intptr_t)(&((idPhysics_Player *)0)->maxJumpHeight), sizeof( ((idPhysics_Player *)0)->maxJumpHeight ) },
	{ "int", "debugLevel", (intptr_t)(&((idPhysics_Player *)0)->debugLevel), sizeof( ((idPhysics_Player *)0)->debugLevel ) },
	{ "usercmd_t", "command", (intptr_t)(&((idPhysics_Player *)0)->command), sizeof( ((idPhysics_Player *)0)->command ) },
	{ "idAngles", "viewAngles", (intptr_t)(&((idPhysics_Player *)0)->viewAngles), sizeof( ((idPhysics_Player *)0)->viewAngles ) },
	{ "int", "framemsec", (intptr_t)(&((idPhysics_Player *)0)->framemsec), sizeof( ((idPhysics_Player *)0)->framemsec ) },
	{ "float", "frametime", (intptr_t)(&((idPhysics_Player *)0)->frametime), sizeof( ((idPhysics_Player *)0)->frametime ) },
	{ "float", "playerSpeed", (intptr_t)(&((idPhysics_Player *)0)->playerSpeed), sizeof( ((idPhysics_Player *)0)->playerSpeed ) },
	{ "idVec3", "viewForward", (intptr_t)(&((idPhysics_Player *)0)->viewForward), sizeof( ((idPhysics_Player *)0)->viewForward ) },
	{ "idVec3", "viewRight", (intptr_t)(&((idPhysics_Player *)0)->viewRight), sizeof( ((idPhysics_Player *)0)->viewRight ) },
	{ "bool", "walking", (intptr_t)(&((idPhysics_Player *)0)->walking), sizeof( ((idPhysics_Player *)0)->walking ) },
	{ "bool", "groundPlane", (intptr_t)(&((idPhysics_Player *)0)->groundPlane), sizeof( ((idPhysics_Player *)0)->groundPlane ) },
	{ "trace_t", "groundTrace", (intptr_t)(&((idPhysics_Player *)0)->groundTrace), sizeof( ((idPhysics_Player *)0)->groundTrace ) },
	{ "const idMaterial *", "groundMaterial", (intptr_t)(&((idPhysics_Player *)0)->groundMaterial), sizeof( ((idPhysics_Player *)0)->groundMaterial ) },
	{ "bool", "ladder", (intptr_t)(&((idPhysics_Player *)0)->ladder), sizeof( ((idPhysics_Player *)0)->ladder ) },
	{ "idVec3", "ladderNormal", (intptr_t)(&((idPhysics_Player *)0)->ladderNormal), sizeof( ((idPhysics_Player *)0)->ladderNormal ) },
	{ "waterLevel_t", "waterLevel", (intptr_t)(&((idPhysics_Player *)0)->waterLevel), sizeof( ((idPhysics_Player *)0)->waterLevel ) },
	{ "int", "waterType", (intptr_t)(&((idPhysics_Player *)0)->waterType), sizeof( ((idPhysics_Player *)0)->waterType ) },
	{ NULL, 0 }
};

static classVariableInfo_t parametricPState_t_typeInfo[] = {
	{ "int", "time", (intptr_t)(&((parametricPState_t *)0)->time), sizeof( ((parametricPState_t *)0)->time ) },
	{ "int", "atRest", (intptr_t)(&((parametricPState_t *)0)->atRest), sizeof( ((parametricPState_t *)0)->atRest ) },
	{ "idVec3", "origin", (intptr_t)(&((parametricPState_t *)0)->origin), sizeof( ((parametricPState_t *)0)->origin ) },
	{ "idAngles", "angles", (intptr_t)(&((parametricPState_t *)0)->angles), sizeof( ((parametricPState_t *)0)->angles ) },
	{ "idMat3", "axis", (intptr_t)(&((parametricPState_t *)0)->axis), sizeof( ((parametricPState_t *)0)->axis ) },
	{ "idVec3", "localOrigin", (intptr_t)(&((parametricPState_t *)0)->localOrigin), sizeof( ((parametricPState_t *)0)->localOrigin ) },
	{ "idAngles", "localAngles", (intptr_t)(&((parametricPState_t *)0)->localAngles), sizeof( ((parametricPState_t *)0)->localAngles ) },
	{ "idExtrapolate < idVec3 >", "linearExtrapolation", (intptr_t)(&((parametricPState_t *)0)->linearExtrapolation), sizeof( ((parametricPState_t *)0)->linearExtrapolation ) },
	{ "idExtrapolate < idAngles >", "angularExtrapolation", (intptr_t)(&((parametricPState_t *)0)->angularExtrapolation), sizeof( ((parametricPState_t *)0)->angularExtrapolation ) },
	{ "idInterpolateAccelDecelLinear < idVec3 >", "linearInterpolation", (intptr_t)(&((parametricPState_t *)0)->linearInterpolation), sizeof( ((parametricPState_t *)0)->linearInterpolation ) },
	{ "idInterpolateAccelDecelLinear < idAngles >", "angularInterpolation", (intptr_t)(&((parametricPState_t *)0)->angularInterpolation), sizeof( ((parametricPState_t *)0)->angularInterpolation ) },
	{ "idCurve_Spline < idVec3 > *", "spline", (intptr_t)(&((parametricPState_t *)0)->spline), sizeof( ((parametricPState_t *)0)->spline ) },
	{ "idInterpolateAccelDecelLinear < float >", "splineInterpolate", (intptr_t)(&((parametricPState_t *)0)->splineInterpolate), sizeof( ((parametricPState_t *)0)->splineInterpolate ) },
	{ "bool", "useSplineAngles", (intptr_t)(&((parametricPState_t *)0)->useSplineAngles), sizeof( ((parametricPState_t *)0)->useSplineAngles ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_Parametric_typeInfo[] = {
	{ ": parametricPState_t", "current", (intptr_t)(&((idPhysics_Parametric *)0)->current), sizeof( ((idPhysics_Parametric *)0)->current ) },
	{ "parametricPState_t", "saved", (intptr_t)(&((idPhysics_Parametric *)0)->saved), sizeof( ((idPhysics_Parametric *)0)->saved ) },
	{ "bool", "isPusher", (intptr_t)(&((idPhysics_Parametric *)0)->isPusher), sizeof( ((idPhysics_Parametric *)0)->isPusher ) },
	{ "idClipModel *", "clipModel", (intptr_t)(&((idPhysics_Parametric *)0)->clipModel), sizeof( ((idPhysics_Parametric *)0)->clipModel ) },
	{ "int", "pushFlags", (intptr_t)(&((idPhysics_Parametric *)0)->pushFlags), sizeof( ((idPhysics_Parametric *)0)->pushFlags ) },
	{ "trace_t", "pushResults", (intptr_t)(&((idPhysics_Parametric *)0)->pushResults), sizeof( ((idPhysics_Parametric *)0)->pushResults ) },
	{ "bool", "isBlocked", (intptr_t)(&((idPhysics_Parametric *)0)->isBlocked), sizeof( ((idPhysics_Parametric *)0)->isBlocked ) },
	{ "bool", "hasMaster", (intptr_t)(&((idPhysics_Parametric *)0)->hasMaster), sizeof( ((idPhysics_Parametric *)0)->hasMaster ) },
	{ "bool", "isOrientated", (intptr_t)(&((idPhysics_Parametric *)0)->isOrientated), sizeof( ((idPhysics_Parametric *)0)->isOrientated ) },
	{ NULL, 0 }
};

static classVariableInfo_t rigidBodyIState_t_typeInfo[] = {
	{ "idVec3", "position", (intptr_t)(&((rigidBodyIState_t *)0)->position), sizeof( ((rigidBodyIState_t *)0)->position ) },
	{ "idMat3", "orientation", (intptr_t)(&((rigidBodyIState_t *)0)->orientation), sizeof( ((rigidBodyIState_t *)0)->orientation ) },
	{ "idVec3", "linearMomentum", (intptr_t)(&((rigidBodyIState_t *)0)->linearMomentum), sizeof( ((rigidBodyIState_t *)0)->linearMomentum ) },
	{ "idVec3", "angularMomentum", (intptr_t)(&((rigidBodyIState_t *)0)->angularMomentum), sizeof( ((rigidBodyIState_t *)0)->angularMomentum ) },
	{ NULL, 0 }
};

static classVariableInfo_t rigidBodyPState_t_typeInfo[] = {
	{ "int", "atRest", (intptr_t)(&((rigidBodyPState_t *)0)->atRest), sizeof( ((rigidBodyPState_t *)0)->atRest ) },
	{ "float", "lastTimeStep", (intptr_t)(&((rigidBodyPState_t *)0)->lastTimeStep), sizeof( ((rigidBodyPState_t *)0)->lastTimeStep ) },
	{ "idVec3", "localOrigin", (intptr_t)(&((rigidBodyPState_t *)0)->localOrigin), sizeof( ((rigidBodyPState_t *)0)->localOrigin ) },
	{ "idMat3", "localAxis", (intptr_t)(&((rigidBodyPState_t *)0)->localAxis), sizeof( ((rigidBodyPState_t *)0)->localAxis ) },
	{ "idVec6", "pushVelocity", (intptr_t)(&((rigidBodyPState_t *)0)->pushVelocity), sizeof( ((rigidBodyPState_t *)0)->pushVelocity ) },
	{ "idVec3", "externalForce", (intptr_t)(&((rigidBodyPState_t *)0)->externalForce), sizeof( ((rigidBodyPState_t *)0)->externalForce ) },
	{ "idVec3", "externalTorque", (intptr_t)(&((rigidBodyPState_t *)0)->externalTorque), sizeof( ((rigidBodyPState_t *)0)->externalTorque ) },
	{ "rigidBodyIState_t", "i", (intptr_t)(&((rigidBodyPState_t *)0)->i), sizeof( ((rigidBodyPState_t *)0)->i ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_RigidBody_typeInfo[] = {
	{ ": rigidBodyPState_t", "current", (intptr_t)(&((idPhysics_RigidBody *)0)->current), sizeof( ((idPhysics_RigidBody *)0)->current ) },
	{ "rigidBodyPState_t", "saved", (intptr_t)(&((idPhysics_RigidBody *)0)->saved), sizeof( ((idPhysics_RigidBody *)0)->saved ) },
	{ "float", "linearFriction", (intptr_t)(&((idPhysics_RigidBody *)0)->linearFriction), sizeof( ((idPhysics_RigidBody *)0)->linearFriction ) },
	{ "float", "angularFriction", (intptr_t)(&((idPhysics_RigidBody *)0)->angularFriction), sizeof( ((idPhysics_RigidBody *)0)->angularFriction ) },
	{ "float", "contactFriction", (intptr_t)(&((idPhysics_RigidBody *)0)->contactFriction), sizeof( ((idPhysics_RigidBody *)0)->contactFriction ) },
	{ "float", "bouncyness", (intptr_t)(&((idPhysics_RigidBody *)0)->bouncyness), sizeof( ((idPhysics_RigidBody *)0)->bouncyness ) },
	{ "idClipModel *", "clipModel", (intptr_t)(&((idPhysics_RigidBody *)0)->clipModel), sizeof( ((idPhysics_RigidBody *)0)->clipModel ) },
	{ "float", "mass", (intptr_t)(&((idPhysics_RigidBody *)0)->mass), sizeof( ((idPhysics_RigidBody *)0)->mass ) },
	{ "float", "inverseMass", (intptr_t)(&((idPhysics_RigidBody *)0)->inverseMass), sizeof( ((idPhysics_RigidBody *)0)->inverseMass ) },
	{ "idVec3", "centerOfMass", (intptr_t)(&((idPhysics_RigidBody *)0)->centerOfMass), sizeof( ((idPhysics_RigidBody *)0)->centerOfMass ) },
	{ "idMat3", "inertiaTensor", (intptr_t)(&((idPhysics_RigidBody *)0)->inertiaTensor), sizeof( ((idPhysics_RigidBody *)0)->inertiaTensor ) },
	{ "idMat3", "inverseInertiaTensor", (intptr_t)(&((idPhysics_RigidBody *)0)->inverseInertiaTensor), sizeof( ((idPhysics_RigidBody *)0)->inverseInertiaTensor ) },
	{ "idODE *", "integrator", (intptr_t)(&((idPhysics_RigidBody *)0)->integrator), sizeof( ((idPhysics_RigidBody *)0)->integrator ) },
	{ "bool", "dropToFloor", (intptr_t)(&((idPhysics_RigidBody *)0)->dropToFloor), sizeof( ((idPhysics_RigidBody *)0)->dropToFloor ) },
	{ "bool", "testSolid", (intptr_t)(&((idPhysics_RigidBody *)0)->testSolid), sizeof( ((idPhysics_RigidBody *)0)->testSolid ) },
	{ "bool", "noImpact", (intptr_t)(&((idPhysics_RigidBody *)0)->noImpact), sizeof( ((idPhysics_RigidBody *)0)->noImpact ) },
	{ "bool", "noContact", (intptr_t)(&((idPhysics_RigidBody *)0)->noContact), sizeof( ((idPhysics_RigidBody *)0)->noContact ) },
	{ "bool", "hasMaster", (intptr_t)(&((idPhysics_RigidBody *)0)->hasMaster), sizeof( ((idPhysics_RigidBody *)0)->hasMaster ) },
	{ "bool", "isOrientated", (intptr_t)(&((idPhysics_RigidBody *)0)->isOrientated), sizeof( ((idPhysics_RigidBody *)0)->isOrientated ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_constraintFlags_s_typeInfo[] = {
//	{ "bool", "allowPrimary", (intptr_t)(&((idAFConstraint::constraintFlags_s *)0)->allowPrimary), sizeof( ((idAFConstraint::constraintFlags_s *)0)->allowPrimary ) },
//	{ "bool", "frameConstraint", (intptr_t)(&((idAFConstraint::constraintFlags_s *)0)->frameConstraint), sizeof( ((idAFConstraint::constraintFlags_s *)0)->frameConstraint ) },
//	{ "bool", "noCollision", (intptr_t)(&((idAFConstraint::constraintFlags_s *)0)->noCollision), sizeof( ((idAFConstraint::constraintFlags_s *)0)->noCollision ) },
//	{ "bool", "isPrimary", (intptr_t)(&((idAFConstraint::constraintFlags_s *)0)->isPrimary), sizeof( ((idAFConstraint::constraintFlags_s *)0)->isPrimary ) },
//	{ "bool", "isZero", (intptr_t)(&((idAFConstraint::constraintFlags_s *)0)->isZero), sizeof( ((idAFConstraint::constraintFlags_s *)0)->isZero ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_typeInfo[] = {
	{ ": constraintType_t", "type", (intptr_t)(&((idAFConstraint *)0)->type), sizeof( ((idAFConstraint *)0)->type ) },
	{ "idStr", "name", (intptr_t)(&((idAFConstraint *)0)->name), sizeof( ((idAFConstraint *)0)->name ) },
	{ "idAFBody *", "body1", (intptr_t)(&((idAFConstraint *)0)->body1), sizeof( ((idAFConstraint *)0)->body1 ) },
	{ "idAFBody *", "body2", (intptr_t)(&((idAFConstraint *)0)->body2), sizeof( ((idAFConstraint *)0)->body2 ) },
	{ "idPhysics_AF *", "physics", (intptr_t)(&((idAFConstraint *)0)->physics), sizeof( ((idAFConstraint *)0)->physics ) },
	{ "idMatX", "J1", (intptr_t)(&((idAFConstraint *)0)->J1), sizeof( ((idAFConstraint *)0)->J1 ) },
	{ "idMatX", "J2", (intptr_t)(&((idAFConstraint *)0)->J2), sizeof( ((idAFConstraint *)0)->J2 ) },
	{ "idVecX", "c1", (intptr_t)(&((idAFConstraint *)0)->c1), sizeof( ((idAFConstraint *)0)->c1 ) },
	{ "idVecX", "c2", (intptr_t)(&((idAFConstraint *)0)->c2), sizeof( ((idAFConstraint *)0)->c2 ) },
	{ "idVecX", "lo", (intptr_t)(&((idAFConstraint *)0)->lo), sizeof( ((idAFConstraint *)0)->lo ) },
	{ "idVecX", "hi", (intptr_t)(&((idAFConstraint *)0)->hi), sizeof( ((idAFConstraint *)0)->hi ) },
	{ "idVecX", "e", (intptr_t)(&((idAFConstraint *)0)->e), sizeof( ((idAFConstraint *)0)->e ) },
	{ "idAFConstraint *", "boxConstraint", (intptr_t)(&((idAFConstraint *)0)->boxConstraint), sizeof( ((idAFConstraint *)0)->boxConstraint ) },
	{ "int[6]", "boxIndex", (intptr_t)(&((idAFConstraint *)0)->boxIndex), sizeof( ((idAFConstraint *)0)->boxIndex ) },
	{ "idMatX", "invI", (intptr_t)(&((idAFConstraint *)0)->invI), sizeof( ((idAFConstraint *)0)->invI ) },
	{ "idMatX", "J", (intptr_t)(&((idAFConstraint *)0)->J), sizeof( ((idAFConstraint *)0)->J ) },
	{ "idVecX", "s", (intptr_t)(&((idAFConstraint *)0)->s), sizeof( ((idAFConstraint *)0)->s ) },
	{ "idVecX", "lm", (intptr_t)(&((idAFConstraint *)0)->lm), sizeof( ((idAFConstraint *)0)->lm ) },
	{ "int", "firstIndex", (intptr_t)(&((idAFConstraint *)0)->firstIndex), sizeof( ((idAFConstraint *)0)->firstIndex ) },
	{ "idAFConstraint::constraintFlags_s", "fl", (intptr_t)(&((idAFConstraint *)0)->fl), sizeof( ((idAFConstraint *)0)->fl ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Fixed_typeInfo[] = {
	{ ": idVec3", "offset", (intptr_t)(&((idAFConstraint_Fixed *)0)->offset), sizeof( ((idAFConstraint_Fixed *)0)->offset ) },
	{ "idMat3", "relAxis", (intptr_t)(&((idAFConstraint_Fixed *)0)->relAxis), sizeof( ((idAFConstraint_Fixed *)0)->relAxis ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_BallAndSocketJoint_typeInfo[] = {
	{ ": idVec3", "anchor1", (intptr_t)(&((idAFConstraint_BallAndSocketJoint *)0)->anchor1), sizeof( ((idAFConstraint_BallAndSocketJoint *)0)->anchor1 ) },
	{ "idVec3", "anchor2", (intptr_t)(&((idAFConstraint_BallAndSocketJoint *)0)->anchor2), sizeof( ((idAFConstraint_BallAndSocketJoint *)0)->anchor2 ) },
	{ "float", "friction", (intptr_t)(&((idAFConstraint_BallAndSocketJoint *)0)->friction), sizeof( ((idAFConstraint_BallAndSocketJoint *)0)->friction ) },
	{ "idAFConstraint_ConeLimit *", "coneLimit", (intptr_t)(&((idAFConstraint_BallAndSocketJoint *)0)->coneLimit), sizeof( ((idAFConstraint_BallAndSocketJoint *)0)->coneLimit ) },
	{ "idAFConstraint_PyramidLimit *", "pyramidLimit", (intptr_t)(&((idAFConstraint_BallAndSocketJoint *)0)->pyramidLimit), sizeof( ((idAFConstraint_BallAndSocketJoint *)0)->pyramidLimit ) },
	{ "idAFConstraint_BallAndSocketJointFriction *", "fc", (intptr_t)(&((idAFConstraint_BallAndSocketJoint *)0)->fc), sizeof( ((idAFConstraint_BallAndSocketJoint *)0)->fc ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_BallAndSocketJointFriction_typeInfo[] = {
	{ ": idAFConstraint_BallAndSocketJoint *", "joint", (intptr_t)(&((idAFConstraint_BallAndSocketJointFriction *)0)->joint), sizeof( ((idAFConstraint_BallAndSocketJointFriction *)0)->joint ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_UniversalJoint_typeInfo[] = {
	{ ": idVec3", "anchor1", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->anchor1), sizeof( ((idAFConstraint_UniversalJoint *)0)->anchor1 ) },
	{ "idVec3", "anchor2", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->anchor2), sizeof( ((idAFConstraint_UniversalJoint *)0)->anchor2 ) },
	{ "idVec3", "shaft1", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->shaft1), sizeof( ((idAFConstraint_UniversalJoint *)0)->shaft1 ) },
	{ "idVec3", "shaft2", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->shaft2), sizeof( ((idAFConstraint_UniversalJoint *)0)->shaft2 ) },
	{ "idVec3", "axis1", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->axis1), sizeof( ((idAFConstraint_UniversalJoint *)0)->axis1 ) },
	{ "idVec3", "axis2", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->axis2), sizeof( ((idAFConstraint_UniversalJoint *)0)->axis2 ) },
	{ "float", "friction", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->friction), sizeof( ((idAFConstraint_UniversalJoint *)0)->friction ) },
	{ "idAFConstraint_ConeLimit *", "coneLimit", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->coneLimit), sizeof( ((idAFConstraint_UniversalJoint *)0)->coneLimit ) },
	{ "idAFConstraint_PyramidLimit *", "pyramidLimit", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->pyramidLimit), sizeof( ((idAFConstraint_UniversalJoint *)0)->pyramidLimit ) },
	{ "idAFConstraint_UniversalJointFriction *", "fc", (intptr_t)(&((idAFConstraint_UniversalJoint *)0)->fc), sizeof( ((idAFConstraint_UniversalJoint *)0)->fc ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_UniversalJointFriction_typeInfo[] = {
	{ ": idAFConstraint_UniversalJoint *", "joint", (intptr_t)(&((idAFConstraint_UniversalJointFriction *)0)->joint), sizeof( ((idAFConstraint_UniversalJointFriction *)0)->joint ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_CylindricalJoint_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Hinge_typeInfo[] = {
	{ ": idVec3", "anchor1", (intptr_t)(&((idAFConstraint_Hinge *)0)->anchor1), sizeof( ((idAFConstraint_Hinge *)0)->anchor1 ) },
	{ "idVec3", "anchor2", (intptr_t)(&((idAFConstraint_Hinge *)0)->anchor2), sizeof( ((idAFConstraint_Hinge *)0)->anchor2 ) },
	{ "idVec3", "axis1", (intptr_t)(&((idAFConstraint_Hinge *)0)->axis1), sizeof( ((idAFConstraint_Hinge *)0)->axis1 ) },
	{ "idVec3", "axis2", (intptr_t)(&((idAFConstraint_Hinge *)0)->axis2), sizeof( ((idAFConstraint_Hinge *)0)->axis2 ) },
	{ "idMat3", "initialAxis", (intptr_t)(&((idAFConstraint_Hinge *)0)->initialAxis), sizeof( ((idAFConstraint_Hinge *)0)->initialAxis ) },
	{ "float", "friction", (intptr_t)(&((idAFConstraint_Hinge *)0)->friction), sizeof( ((idAFConstraint_Hinge *)0)->friction ) },
	{ "idAFConstraint_ConeLimit *", "coneLimit", (intptr_t)(&((idAFConstraint_Hinge *)0)->coneLimit), sizeof( ((idAFConstraint_Hinge *)0)->coneLimit ) },
	{ "idAFConstraint_HingeSteering *", "steering", (intptr_t)(&((idAFConstraint_Hinge *)0)->steering), sizeof( ((idAFConstraint_Hinge *)0)->steering ) },
	{ "idAFConstraint_HingeFriction *", "fc", (intptr_t)(&((idAFConstraint_Hinge *)0)->fc), sizeof( ((idAFConstraint_Hinge *)0)->fc ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_HingeFriction_typeInfo[] = {
	{ ": idAFConstraint_Hinge *", "hinge", (intptr_t)(&((idAFConstraint_HingeFriction *)0)->hinge), sizeof( ((idAFConstraint_HingeFriction *)0)->hinge ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_HingeSteering_typeInfo[] = {
	{ ": idAFConstraint_Hinge *", "hinge", (intptr_t)(&((idAFConstraint_HingeSteering *)0)->hinge), sizeof( ((idAFConstraint_HingeSteering *)0)->hinge ) },
	{ "float", "steerAngle", (intptr_t)(&((idAFConstraint_HingeSteering *)0)->steerAngle), sizeof( ((idAFConstraint_HingeSteering *)0)->steerAngle ) },
	{ "float", "steerSpeed", (intptr_t)(&((idAFConstraint_HingeSteering *)0)->steerSpeed), sizeof( ((idAFConstraint_HingeSteering *)0)->steerSpeed ) },
	{ "float", "epsilon", (intptr_t)(&((idAFConstraint_HingeSteering *)0)->epsilon), sizeof( ((idAFConstraint_HingeSteering *)0)->epsilon ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Slider_typeInfo[] = {
	{ ": idVec3", "axis", (intptr_t)(&((idAFConstraint_Slider *)0)->axis), sizeof( ((idAFConstraint_Slider *)0)->axis ) },
	{ "idVec3", "offset", (intptr_t)(&((idAFConstraint_Slider *)0)->offset), sizeof( ((idAFConstraint_Slider *)0)->offset ) },
	{ "idMat3", "relAxis", (intptr_t)(&((idAFConstraint_Slider *)0)->relAxis), sizeof( ((idAFConstraint_Slider *)0)->relAxis ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Line_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Plane_typeInfo[] = {
	{ ": idVec3", "anchor1", (intptr_t)(&((idAFConstraint_Plane *)0)->anchor1), sizeof( ((idAFConstraint_Plane *)0)->anchor1 ) },
	{ "idVec3", "anchor2", (intptr_t)(&((idAFConstraint_Plane *)0)->anchor2), sizeof( ((idAFConstraint_Plane *)0)->anchor2 ) },
	{ "idVec3", "planeNormal", (intptr_t)(&((idAFConstraint_Plane *)0)->planeNormal), sizeof( ((idAFConstraint_Plane *)0)->planeNormal ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Spring_typeInfo[] = {
	{ ": idVec3", "anchor1", (intptr_t)(&((idAFConstraint_Spring *)0)->anchor1), sizeof( ((idAFConstraint_Spring *)0)->anchor1 ) },
	{ "idVec3", "anchor2", (intptr_t)(&((idAFConstraint_Spring *)0)->anchor2), sizeof( ((idAFConstraint_Spring *)0)->anchor2 ) },
	{ "float", "kstretch", (intptr_t)(&((idAFConstraint_Spring *)0)->kstretch), sizeof( ((idAFConstraint_Spring *)0)->kstretch ) },
	{ "float", "kcompress", (intptr_t)(&((idAFConstraint_Spring *)0)->kcompress), sizeof( ((idAFConstraint_Spring *)0)->kcompress ) },
	{ "float", "damping", (intptr_t)(&((idAFConstraint_Spring *)0)->damping), sizeof( ((idAFConstraint_Spring *)0)->damping ) },
	{ "float", "restLength", (intptr_t)(&((idAFConstraint_Spring *)0)->restLength), sizeof( ((idAFConstraint_Spring *)0)->restLength ) },
	{ "float", "minLength", (intptr_t)(&((idAFConstraint_Spring *)0)->minLength), sizeof( ((idAFConstraint_Spring *)0)->minLength ) },
	{ "float", "maxLength", (intptr_t)(&((idAFConstraint_Spring *)0)->maxLength), sizeof( ((idAFConstraint_Spring *)0)->maxLength ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Contact_typeInfo[] = {
	{ ": contactInfo_t", "contact", (intptr_t)(&((idAFConstraint_Contact *)0)->contact), sizeof( ((idAFConstraint_Contact *)0)->contact ) },
	{ "idAFConstraint_ContactFriction *", "fc", (intptr_t)(&((idAFConstraint_Contact *)0)->fc), sizeof( ((idAFConstraint_Contact *)0)->fc ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_ContactFriction_typeInfo[] = {
	{ ": idAFConstraint_Contact *", "cc", (intptr_t)(&((idAFConstraint_ContactFriction *)0)->cc), sizeof( ((idAFConstraint_ContactFriction *)0)->cc ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_ConeLimit_typeInfo[] = {
	{ ": idVec3", "coneAnchor", (intptr_t)(&((idAFConstraint_ConeLimit *)0)->coneAnchor), sizeof( ((idAFConstraint_ConeLimit *)0)->coneAnchor ) },
	{ "idVec3", "coneAxis", (intptr_t)(&((idAFConstraint_ConeLimit *)0)->coneAxis), sizeof( ((idAFConstraint_ConeLimit *)0)->coneAxis ) },
	{ "idVec3", "body1Axis", (intptr_t)(&((idAFConstraint_ConeLimit *)0)->body1Axis), sizeof( ((idAFConstraint_ConeLimit *)0)->body1Axis ) },
	{ "float", "cosAngle", (intptr_t)(&((idAFConstraint_ConeLimit *)0)->cosAngle), sizeof( ((idAFConstraint_ConeLimit *)0)->cosAngle ) },
	{ "float", "sinHalfAngle", (intptr_t)(&((idAFConstraint_ConeLimit *)0)->sinHalfAngle), sizeof( ((idAFConstraint_ConeLimit *)0)->sinHalfAngle ) },
	{ "float", "cosHalfAngle", (intptr_t)(&((idAFConstraint_ConeLimit *)0)->cosHalfAngle), sizeof( ((idAFConstraint_ConeLimit *)0)->cosHalfAngle ) },
	{ "float", "epsilon", (intptr_t)(&((idAFConstraint_ConeLimit *)0)->epsilon), sizeof( ((idAFConstraint_ConeLimit *)0)->epsilon ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_PyramidLimit_typeInfo[] = {
	{ ": idVec3", "pyramidAnchor", (intptr_t)(&((idAFConstraint_PyramidLimit *)0)->pyramidAnchor), sizeof( ((idAFConstraint_PyramidLimit *)0)->pyramidAnchor ) },
	{ "idMat3", "pyramidBasis", (intptr_t)(&((idAFConstraint_PyramidLimit *)0)->pyramidBasis), sizeof( ((idAFConstraint_PyramidLimit *)0)->pyramidBasis ) },
	{ "idVec3", "body1Axis", (intptr_t)(&((idAFConstraint_PyramidLimit *)0)->body1Axis), sizeof( ((idAFConstraint_PyramidLimit *)0)->body1Axis ) },
	{ "float[2]", "cosAngle", (intptr_t)(&((idAFConstraint_PyramidLimit *)0)->cosAngle), sizeof( ((idAFConstraint_PyramidLimit *)0)->cosAngle ) },
	{ "float[2]", "sinHalfAngle", (intptr_t)(&((idAFConstraint_PyramidLimit *)0)->sinHalfAngle), sizeof( ((idAFConstraint_PyramidLimit *)0)->sinHalfAngle ) },
	{ "float[2]", "cosHalfAngle", (intptr_t)(&((idAFConstraint_PyramidLimit *)0)->cosHalfAngle), sizeof( ((idAFConstraint_PyramidLimit *)0)->cosHalfAngle ) },
	{ "float", "epsilon", (intptr_t)(&((idAFConstraint_PyramidLimit *)0)->epsilon), sizeof( ((idAFConstraint_PyramidLimit *)0)->epsilon ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFConstraint_Suspension_typeInfo[] = {
	{ ": idVec3", "localOrigin", (intptr_t)(&((idAFConstraint_Suspension *)0)->localOrigin), sizeof( ((idAFConstraint_Suspension *)0)->localOrigin ) },
	{ "idMat3", "localAxis", (intptr_t)(&((idAFConstraint_Suspension *)0)->localAxis), sizeof( ((idAFConstraint_Suspension *)0)->localAxis ) },
	{ "float", "suspensionUp", (intptr_t)(&((idAFConstraint_Suspension *)0)->suspensionUp), sizeof( ((idAFConstraint_Suspension *)0)->suspensionUp ) },
	{ "float", "suspensionDown", (intptr_t)(&((idAFConstraint_Suspension *)0)->suspensionDown), sizeof( ((idAFConstraint_Suspension *)0)->suspensionDown ) },
	{ "float", "suspensionKCompress", (intptr_t)(&((idAFConstraint_Suspension *)0)->suspensionKCompress), sizeof( ((idAFConstraint_Suspension *)0)->suspensionKCompress ) },
	{ "float", "suspensionDamping", (intptr_t)(&((idAFConstraint_Suspension *)0)->suspensionDamping), sizeof( ((idAFConstraint_Suspension *)0)->suspensionDamping ) },
	{ "float", "steerAngle", (intptr_t)(&((idAFConstraint_Suspension *)0)->steerAngle), sizeof( ((idAFConstraint_Suspension *)0)->steerAngle ) },
	{ "float", "friction", (intptr_t)(&((idAFConstraint_Suspension *)0)->friction), sizeof( ((idAFConstraint_Suspension *)0)->friction ) },
	{ "bool", "motorEnabled", (intptr_t)(&((idAFConstraint_Suspension *)0)->motorEnabled), sizeof( ((idAFConstraint_Suspension *)0)->motorEnabled ) },
	{ "float", "motorForce", (intptr_t)(&((idAFConstraint_Suspension *)0)->motorForce), sizeof( ((idAFConstraint_Suspension *)0)->motorForce ) },
	{ "float", "motorVelocity", (intptr_t)(&((idAFConstraint_Suspension *)0)->motorVelocity), sizeof( ((idAFConstraint_Suspension *)0)->motorVelocity ) },
	{ "idClipModel *", "wheelModel", (intptr_t)(&((idAFConstraint_Suspension *)0)->wheelModel), sizeof( ((idAFConstraint_Suspension *)0)->wheelModel ) },
	{ "idVec3", "wheelOffset", (intptr_t)(&((idAFConstraint_Suspension *)0)->wheelOffset), sizeof( ((idAFConstraint_Suspension *)0)->wheelOffset ) },
	{ "trace_t", "trace", (intptr_t)(&((idAFConstraint_Suspension *)0)->trace), sizeof( ((idAFConstraint_Suspension *)0)->trace ) },
	{ "float", "epsilon", (intptr_t)(&((idAFConstraint_Suspension *)0)->epsilon), sizeof( ((idAFConstraint_Suspension *)0)->epsilon ) },
	{ NULL, 0 }
};

static classVariableInfo_t AFBodyPState_t_typeInfo[] = {
	{ "idVec3", "worldOrigin", (intptr_t)(&((AFBodyPState_t *)0)->worldOrigin), sizeof( ((AFBodyPState_t *)0)->worldOrigin ) },
	{ "idMat3", "worldAxis", (intptr_t)(&((AFBodyPState_t *)0)->worldAxis), sizeof( ((AFBodyPState_t *)0)->worldAxis ) },
	{ "idVec6", "spatialVelocity", (intptr_t)(&((AFBodyPState_t *)0)->spatialVelocity), sizeof( ((AFBodyPState_t *)0)->spatialVelocity ) },
	{ "idVec6", "externalForce", (intptr_t)(&((AFBodyPState_t *)0)->externalForce), sizeof( ((AFBodyPState_t *)0)->externalForce ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFBody_bodyFlags_s_typeInfo[] = {
//	{ "bool", "clipMaskSet", (intptr_t)(&((idAFBody::bodyFlags_s *)0)->clipMaskSet), sizeof( ((idAFBody::bodyFlags_s *)0)->clipMaskSet ) },
//	{ "bool", "selfCollision", (intptr_t)(&((idAFBody::bodyFlags_s *)0)->selfCollision), sizeof( ((idAFBody::bodyFlags_s *)0)->selfCollision ) },
//	{ "bool", "spatialInertiaSparse", (intptr_t)(&((idAFBody::bodyFlags_s *)0)->spatialInertiaSparse), sizeof( ((idAFBody::bodyFlags_s *)0)->spatialInertiaSparse ) },
//	{ "bool", "useFrictionDir", (intptr_t)(&((idAFBody::bodyFlags_s *)0)->useFrictionDir), sizeof( ((idAFBody::bodyFlags_s *)0)->useFrictionDir ) },
//	{ "bool", "useContactMotorDir", (intptr_t)(&((idAFBody::bodyFlags_s *)0)->useContactMotorDir), sizeof( ((idAFBody::bodyFlags_s *)0)->useContactMotorDir ) },
//	{ "bool", "isZero", (intptr_t)(&((idAFBody::bodyFlags_s *)0)->isZero), sizeof( ((idAFBody::bodyFlags_s *)0)->isZero ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFBody_typeInfo[] = {
	{ ": idStr", "name", (intptr_t)(&((idAFBody *)0)->name), sizeof( ((idAFBody *)0)->name ) },
	{ "idAFBody *", "parent", (intptr_t)(&((idAFBody *)0)->parent), sizeof( ((idAFBody *)0)->parent ) },
	{ "idList < idAFBody * >", "children", (intptr_t)(&((idAFBody *)0)->children), sizeof( ((idAFBody *)0)->children ) },
	{ "idClipModel *", "clipModel", (intptr_t)(&((idAFBody *)0)->clipModel), sizeof( ((idAFBody *)0)->clipModel ) },
	{ "idAFConstraint *", "primaryConstraint", (intptr_t)(&((idAFBody *)0)->primaryConstraint), sizeof( ((idAFBody *)0)->primaryConstraint ) },
	{ "idList < idAFConstraint * >", "constraints", (intptr_t)(&((idAFBody *)0)->constraints), sizeof( ((idAFBody *)0)->constraints ) },
	{ "idAFTree *", "tree", (intptr_t)(&((idAFBody *)0)->tree), sizeof( ((idAFBody *)0)->tree ) },
	{ "float", "linearFriction", (intptr_t)(&((idAFBody *)0)->linearFriction), sizeof( ((idAFBody *)0)->linearFriction ) },
	{ "float", "angularFriction", (intptr_t)(&((idAFBody *)0)->angularFriction), sizeof( ((idAFBody *)0)->angularFriction ) },
	{ "float", "contactFriction", (intptr_t)(&((idAFBody *)0)->contactFriction), sizeof( ((idAFBody *)0)->contactFriction ) },
	{ "float", "bouncyness", (intptr_t)(&((idAFBody *)0)->bouncyness), sizeof( ((idAFBody *)0)->bouncyness ) },
	{ "int", "clipMask", (intptr_t)(&((idAFBody *)0)->clipMask), sizeof( ((idAFBody *)0)->clipMask ) },
	{ "idVec3", "frictionDir", (intptr_t)(&((idAFBody *)0)->frictionDir), sizeof( ((idAFBody *)0)->frictionDir ) },
	{ "idVec3", "contactMotorDir", (intptr_t)(&((idAFBody *)0)->contactMotorDir), sizeof( ((idAFBody *)0)->contactMotorDir ) },
	{ "float", "contactMotorVelocity", (intptr_t)(&((idAFBody *)0)->contactMotorVelocity), sizeof( ((idAFBody *)0)->contactMotorVelocity ) },
	{ "float", "contactMotorForce", (intptr_t)(&((idAFBody *)0)->contactMotorForce), sizeof( ((idAFBody *)0)->contactMotorForce ) },
	{ "float", "mass", (intptr_t)(&((idAFBody *)0)->mass), sizeof( ((idAFBody *)0)->mass ) },
	{ "float", "invMass", (intptr_t)(&((idAFBody *)0)->invMass), sizeof( ((idAFBody *)0)->invMass ) },
	{ "idVec3", "centerOfMass", (intptr_t)(&((idAFBody *)0)->centerOfMass), sizeof( ((idAFBody *)0)->centerOfMass ) },
	{ "idMat3", "inertiaTensor", (intptr_t)(&((idAFBody *)0)->inertiaTensor), sizeof( ((idAFBody *)0)->inertiaTensor ) },
	{ "idMat3", "inverseInertiaTensor", (intptr_t)(&((idAFBody *)0)->inverseInertiaTensor), sizeof( ((idAFBody *)0)->inverseInertiaTensor ) },
	{ "AFBodyPState_t[2]", "state", (intptr_t)(&((idAFBody *)0)->state), sizeof( ((idAFBody *)0)->state ) },
	{ "AFBodyPState_t *", "current", (intptr_t)(&((idAFBody *)0)->current), sizeof( ((idAFBody *)0)->current ) },
	{ "AFBodyPState_t *", "next", (intptr_t)(&((idAFBody *)0)->next), sizeof( ((idAFBody *)0)->next ) },
	{ "AFBodyPState_t", "saved", (intptr_t)(&((idAFBody *)0)->saved), sizeof( ((idAFBody *)0)->saved ) },
	{ "idVec3", "atRestOrigin", (intptr_t)(&((idAFBody *)0)->atRestOrigin), sizeof( ((idAFBody *)0)->atRestOrigin ) },
	{ "idMat3", "atRestAxis", (intptr_t)(&((idAFBody *)0)->atRestAxis), sizeof( ((idAFBody *)0)->atRestAxis ) },
	{ "idMatX", "inverseWorldSpatialInertia", (intptr_t)(&((idAFBody *)0)->inverseWorldSpatialInertia), sizeof( ((idAFBody *)0)->inverseWorldSpatialInertia ) },
	{ "idMatX", "I", (intptr_t)(&((idAFBody *)0)->I), sizeof( ((idAFBody *)0)->I ) },
	{ "idMatX", "invI", (intptr_t)(&((idAFBody *)0)->invI), sizeof( ((idAFBody *)0)->invI ) },
	{ "idMatX", "J", (intptr_t)(&((idAFBody *)0)->J), sizeof( ((idAFBody *)0)->J ) },
	{ "idVecX", "s", (intptr_t)(&((idAFBody *)0)->s), sizeof( ((idAFBody *)0)->s ) },
	{ "idVecX", "totalForce", (intptr_t)(&((idAFBody *)0)->totalForce), sizeof( ((idAFBody *)0)->totalForce ) },
	{ "idVecX", "auxForce", (intptr_t)(&((idAFBody *)0)->auxForce), sizeof( ((idAFBody *)0)->auxForce ) },
	{ "idVecX", "acceleration", (intptr_t)(&((idAFBody *)0)->acceleration), sizeof( ((idAFBody *)0)->acceleration ) },
	{ "float *", "response", (intptr_t)(&((idAFBody *)0)->response), sizeof( ((idAFBody *)0)->response ) },
	{ "int *", "responseIndex", (intptr_t)(&((idAFBody *)0)->responseIndex), sizeof( ((idAFBody *)0)->responseIndex ) },
	{ "int", "numResponses", (intptr_t)(&((idAFBody *)0)->numResponses), sizeof( ((idAFBody *)0)->numResponses ) },
	{ "int", "maxAuxiliaryIndex", (intptr_t)(&((idAFBody *)0)->maxAuxiliaryIndex), sizeof( ((idAFBody *)0)->maxAuxiliaryIndex ) },
	{ "int", "maxSubTreeAuxiliaryIndex", (intptr_t)(&((idAFBody *)0)->maxSubTreeAuxiliaryIndex), sizeof( ((idAFBody *)0)->maxSubTreeAuxiliaryIndex ) },
	{ "idAFBody::bodyFlags_s", "fl", (intptr_t)(&((idAFBody *)0)->fl), sizeof( ((idAFBody *)0)->fl ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFTree_typeInfo[] = {
	{ ": idList < idAFBody * >", "sortedBodies", (intptr_t)(&((idAFTree *)0)->sortedBodies), sizeof( ((idAFTree *)0)->sortedBodies ) },
	{ NULL, 0 }
};

static classVariableInfo_t AFPState_t_typeInfo[] = {
	{ "int", "atRest", (intptr_t)(&((AFPState_t *)0)->atRest), sizeof( ((AFPState_t *)0)->atRest ) },
	{ "float", "noMoveTime", (intptr_t)(&((AFPState_t *)0)->noMoveTime), sizeof( ((AFPState_t *)0)->noMoveTime ) },
	{ "float", "activateTime", (intptr_t)(&((AFPState_t *)0)->activateTime), sizeof( ((AFPState_t *)0)->activateTime ) },
	{ "float", "lastTimeStep", (intptr_t)(&((AFPState_t *)0)->lastTimeStep), sizeof( ((AFPState_t *)0)->lastTimeStep ) },
	{ "idVec6", "pushVelocity", (intptr_t)(&((AFPState_t *)0)->pushVelocity), sizeof( ((AFPState_t *)0)->pushVelocity ) },
	{ NULL, 0 }
};

static classVariableInfo_t AFCollision_t_typeInfo[] = {
	{ "trace_t", "trace", (intptr_t)(&((AFCollision_t *)0)->trace), sizeof( ((AFCollision_t *)0)->trace ) },
	{ "idAFBody *", "body", (intptr_t)(&((AFCollision_t *)0)->body), sizeof( ((AFCollision_t *)0)->body ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPhysics_AF_typeInfo[] = {
	{ ": idList < idAFTree * >", "trees", (intptr_t)(&((idPhysics_AF *)0)->trees), sizeof( ((idPhysics_AF *)0)->trees ) },
	{ "idList < idAFBody * >", "bodies", (intptr_t)(&((idPhysics_AF *)0)->bodies), sizeof( ((idPhysics_AF *)0)->bodies ) },
	{ "idList < idAFConstraint * >", "constraints", (intptr_t)(&((idPhysics_AF *)0)->constraints), sizeof( ((idPhysics_AF *)0)->constraints ) },
	{ "idList < idAFConstraint * >", "primaryConstraints", (intptr_t)(&((idPhysics_AF *)0)->primaryConstraints), sizeof( ((idPhysics_AF *)0)->primaryConstraints ) },
	{ "idList < idAFConstraint * >", "auxiliaryConstraints", (intptr_t)(&((idPhysics_AF *)0)->auxiliaryConstraints), sizeof( ((idPhysics_AF *)0)->auxiliaryConstraints ) },
	{ "idList < idAFConstraint * >", "frameConstraints", (intptr_t)(&((idPhysics_AF *)0)->frameConstraints), sizeof( ((idPhysics_AF *)0)->frameConstraints ) },
	{ "idList < idAFConstraint_Contact * >", "contactConstraints", (intptr_t)(&((idPhysics_AF *)0)->contactConstraints), sizeof( ((idPhysics_AF *)0)->contactConstraints ) },
	{ "idList < int >", "contactBodies", (intptr_t)(&((idPhysics_AF *)0)->contactBodies), sizeof( ((idPhysics_AF *)0)->contactBodies ) },
	{ "idList < AFCollision_t >", "collisions", (intptr_t)(&((idPhysics_AF *)0)->collisions), sizeof( ((idPhysics_AF *)0)->collisions ) },
	{ "bool", "changedAF", (intptr_t)(&((idPhysics_AF *)0)->changedAF), sizeof( ((idPhysics_AF *)0)->changedAF ) },
	{ "float", "linearFriction", (intptr_t)(&((idPhysics_AF *)0)->linearFriction), sizeof( ((idPhysics_AF *)0)->linearFriction ) },
	{ "float", "angularFriction", (intptr_t)(&((idPhysics_AF *)0)->angularFriction), sizeof( ((idPhysics_AF *)0)->angularFriction ) },
	{ "float", "contactFriction", (intptr_t)(&((idPhysics_AF *)0)->contactFriction), sizeof( ((idPhysics_AF *)0)->contactFriction ) },
	{ "float", "bouncyness", (intptr_t)(&((idPhysics_AF *)0)->bouncyness), sizeof( ((idPhysics_AF *)0)->bouncyness ) },
	{ "float", "totalMass", (intptr_t)(&((idPhysics_AF *)0)->totalMass), sizeof( ((idPhysics_AF *)0)->totalMass ) },
	{ "float", "forceTotalMass", (intptr_t)(&((idPhysics_AF *)0)->forceTotalMass), sizeof( ((idPhysics_AF *)0)->forceTotalMass ) },
	{ "idVec2", "suspendVelocity", (intptr_t)(&((idPhysics_AF *)0)->suspendVelocity), sizeof( ((idPhysics_AF *)0)->suspendVelocity ) },
	{ "idVec2", "suspendAcceleration", (intptr_t)(&((idPhysics_AF *)0)->suspendAcceleration), sizeof( ((idPhysics_AF *)0)->suspendAcceleration ) },
	{ "float", "noMoveTime", (intptr_t)(&((idPhysics_AF *)0)->noMoveTime), sizeof( ((idPhysics_AF *)0)->noMoveTime ) },
	{ "float", "noMoveTranslation", (intptr_t)(&((idPhysics_AF *)0)->noMoveTranslation), sizeof( ((idPhysics_AF *)0)->noMoveTranslation ) },
	{ "float", "noMoveRotation", (intptr_t)(&((idPhysics_AF *)0)->noMoveRotation), sizeof( ((idPhysics_AF *)0)->noMoveRotation ) },
	{ "float", "minMoveTime", (intptr_t)(&((idPhysics_AF *)0)->minMoveTime), sizeof( ((idPhysics_AF *)0)->minMoveTime ) },
	{ "float", "maxMoveTime", (intptr_t)(&((idPhysics_AF *)0)->maxMoveTime), sizeof( ((idPhysics_AF *)0)->maxMoveTime ) },
	{ "float", "impulseThreshold", (intptr_t)(&((idPhysics_AF *)0)->impulseThreshold), sizeof( ((idPhysics_AF *)0)->impulseThreshold ) },
	{ "float", "timeScale", (intptr_t)(&((idPhysics_AF *)0)->timeScale), sizeof( ((idPhysics_AF *)0)->timeScale ) },
	{ "float", "timeScaleRampStart", (intptr_t)(&((idPhysics_AF *)0)->timeScaleRampStart), sizeof( ((idPhysics_AF *)0)->timeScaleRampStart ) },
	{ "float", "timeScaleRampEnd", (intptr_t)(&((idPhysics_AF *)0)->timeScaleRampEnd), sizeof( ((idPhysics_AF *)0)->timeScaleRampEnd ) },
	{ "float", "jointFrictionScale", (intptr_t)(&((idPhysics_AF *)0)->jointFrictionScale), sizeof( ((idPhysics_AF *)0)->jointFrictionScale ) },
	{ "float", "jointFrictionDent", (intptr_t)(&((idPhysics_AF *)0)->jointFrictionDent), sizeof( ((idPhysics_AF *)0)->jointFrictionDent ) },
	{ "float", "jointFrictionDentStart", (intptr_t)(&((idPhysics_AF *)0)->jointFrictionDentStart), sizeof( ((idPhysics_AF *)0)->jointFrictionDentStart ) },
	{ "float", "jointFrictionDentEnd", (intptr_t)(&((idPhysics_AF *)0)->jointFrictionDentEnd), sizeof( ((idPhysics_AF *)0)->jointFrictionDentEnd ) },
	{ "float", "jointFrictionDentScale", (intptr_t)(&((idPhysics_AF *)0)->jointFrictionDentScale), sizeof( ((idPhysics_AF *)0)->jointFrictionDentScale ) },
	{ "float", "contactFrictionScale", (intptr_t)(&((idPhysics_AF *)0)->contactFrictionScale), sizeof( ((idPhysics_AF *)0)->contactFrictionScale ) },
	{ "float", "contactFrictionDent", (intptr_t)(&((idPhysics_AF *)0)->contactFrictionDent), sizeof( ((idPhysics_AF *)0)->contactFrictionDent ) },
	{ "float", "contactFrictionDentStart", (intptr_t)(&((idPhysics_AF *)0)->contactFrictionDentStart), sizeof( ((idPhysics_AF *)0)->contactFrictionDentStart ) },
	{ "float", "contactFrictionDentEnd", (intptr_t)(&((idPhysics_AF *)0)->contactFrictionDentEnd), sizeof( ((idPhysics_AF *)0)->contactFrictionDentEnd ) },
	{ "float", "contactFrictionDentScale", (intptr_t)(&((idPhysics_AF *)0)->contactFrictionDentScale), sizeof( ((idPhysics_AF *)0)->contactFrictionDentScale ) },
	{ "bool", "enableCollision", (intptr_t)(&((idPhysics_AF *)0)->enableCollision), sizeof( ((idPhysics_AF *)0)->enableCollision ) },
	{ "bool", "selfCollision", (intptr_t)(&((idPhysics_AF *)0)->selfCollision), sizeof( ((idPhysics_AF *)0)->selfCollision ) },
	{ "bool", "comeToRest", (intptr_t)(&((idPhysics_AF *)0)->comeToRest), sizeof( ((idPhysics_AF *)0)->comeToRest ) },
	{ "bool", "linearTime", (intptr_t)(&((idPhysics_AF *)0)->linearTime), sizeof( ((idPhysics_AF *)0)->linearTime ) },
	{ "bool", "noImpact", (intptr_t)(&((idPhysics_AF *)0)->noImpact), sizeof( ((idPhysics_AF *)0)->noImpact ) },
	{ "bool", "worldConstraintsLocked", (intptr_t)(&((idPhysics_AF *)0)->worldConstraintsLocked), sizeof( ((idPhysics_AF *)0)->worldConstraintsLocked ) },
	{ "bool", "forcePushable", (intptr_t)(&((idPhysics_AF *)0)->forcePushable), sizeof( ((idPhysics_AF *)0)->forcePushable ) },
	{ "AFPState_t", "current", (intptr_t)(&((idPhysics_AF *)0)->current), sizeof( ((idPhysics_AF *)0)->current ) },
	{ "AFPState_t", "saved", (intptr_t)(&((idPhysics_AF *)0)->saved), sizeof( ((idPhysics_AF *)0)->saved ) },
	{ "idAFBody *", "masterBody", (intptr_t)(&((idPhysics_AF *)0)->masterBody), sizeof( ((idPhysics_AF *)0)->masterBody ) },
	{ "idLCP *", "lcp", (intptr_t)(&((idPhysics_AF *)0)->lcp), sizeof( ((idPhysics_AF *)0)->lcp ) },
	{ NULL, 0 }
};

static classVariableInfo_t singleSmoke_t_typeInfo[] = {
	{ "singleSmoke_s *", "next", (intptr_t)(&((singleSmoke_t *)0)->next), sizeof( ((singleSmoke_t *)0)->next ) },
	{ "int", "privateStartTime", (intptr_t)(&((singleSmoke_t *)0)->privateStartTime), sizeof( ((singleSmoke_t *)0)->privateStartTime ) },
	{ "int", "index", (intptr_t)(&((singleSmoke_t *)0)->index), sizeof( ((singleSmoke_t *)0)->index ) },
	{ "idRandom", "random", (intptr_t)(&((singleSmoke_t *)0)->random), sizeof( ((singleSmoke_t *)0)->random ) },
	{ "idVec3", "origin", (intptr_t)(&((singleSmoke_t *)0)->origin), sizeof( ((singleSmoke_t *)0)->origin ) },
	{ "idMat3", "axis", (intptr_t)(&((singleSmoke_t *)0)->axis), sizeof( ((singleSmoke_t *)0)->axis ) },
	{ "int", "timeGroup", (intptr_t)(&((singleSmoke_t *)0)->timeGroup), sizeof( ((singleSmoke_t *)0)->timeGroup ) },
	{ NULL, 0 }
};

static classVariableInfo_t activeSmokeStage_t_typeInfo[] = {
	{ "const idParticleStage *", "stage", (intptr_t)(&((activeSmokeStage_t *)0)->stage), sizeof( ((activeSmokeStage_t *)0)->stage ) },
	{ "singleSmoke_t *", "smokes", (intptr_t)(&((activeSmokeStage_t *)0)->smokes), sizeof( ((activeSmokeStage_t *)0)->smokes ) },
	{ NULL, 0 }
};

static classVariableInfo_t idSmokeParticles_typeInfo[] = {
	{ ": bool", "initialized", (intptr_t)(&((idSmokeParticles *)0)->initialized), sizeof( ((idSmokeParticles *)0)->initialized ) },
	{ "renderEntity_t", "renderEntity", (intptr_t)(&((idSmokeParticles *)0)->renderEntity), sizeof( ((idSmokeParticles *)0)->renderEntity ) },
	{ "int", "renderEntityHandle", (intptr_t)(&((idSmokeParticles *)0)->renderEntityHandle), sizeof( ((idSmokeParticles *)0)->renderEntityHandle ) },
	{ "singleSmoke_t[10000]", "smokes", (intptr_t)(&((idSmokeParticles *)0)->smokes), sizeof( ((idSmokeParticles *)0)->smokes ) },
	{ "idList < activeSmokeStage_t >", "activeStages", (intptr_t)(&((idSmokeParticles *)0)->activeStages), sizeof( ((idSmokeParticles *)0)->activeStages ) },
	{ "singleSmoke_t *", "freeSmokes", (intptr_t)(&((idSmokeParticles *)0)->freeSmokes), sizeof( ((idSmokeParticles *)0)->freeSmokes ) },
	{ "int", "numActiveSmokes", (intptr_t)(&((idSmokeParticles *)0)->numActiveSmokes), sizeof( ((idSmokeParticles *)0)->numActiveSmokes ) },
	{ "int", "currentParticleTime", (intptr_t)(&((idSmokeParticles *)0)->currentParticleTime), sizeof( ((idSmokeParticles *)0)->currentParticleTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t signal_t_typeInfo[] = {
	{ "int", "threadnum", (intptr_t)(&((signal_t *)0)->threadnum), sizeof( ((signal_t *)0)->threadnum ) },
	{ "const function_t *", "function", (intptr_t)(&((signal_t *)0)->function), sizeof( ((signal_t *)0)->function ) },
	{ NULL, 0 }
};

static classVariableInfo_t signalList_t_typeInfo[] = {
	{ ": idList < signal_t >[10]", "signal", (intptr_t)(&((signalList_t *)0)->signal), sizeof( ((signalList_t *)0)->signal ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEntity_entityFlags_s_typeInfo[] = {
//	{ "bool", "notarget", (intptr_t)(&((idEntity::entityFlags_s *)0)->notarget), sizeof( ((idEntity::entityFlags_s *)0)->notarget ) },
//	{ "bool", "noknockback", (intptr_t)(&((idEntity::entityFlags_s *)0)->noknockback), sizeof( ((idEntity::entityFlags_s *)0)->noknockback ) },
//	{ "bool", "takedamage", (intptr_t)(&((idEntity::entityFlags_s *)0)->takedamage), sizeof( ((idEntity::entityFlags_s *)0)->takedamage ) },
//	{ "bool", "hidden", (intptr_t)(&((idEntity::entityFlags_s *)0)->hidden), sizeof( ((idEntity::entityFlags_s *)0)->hidden ) },
//	{ "bool", "bindOrientated", (intptr_t)(&((idEntity::entityFlags_s *)0)->bindOrientated), sizeof( ((idEntity::entityFlags_s *)0)->bindOrientated ) },
//	{ "bool", "solidForTeam", (intptr_t)(&((idEntity::entityFlags_s *)0)->solidForTeam), sizeof( ((idEntity::entityFlags_s *)0)->solidForTeam ) },
//	{ "bool", "forcePhysicsUpdate", (intptr_t)(&((idEntity::entityFlags_s *)0)->forcePhysicsUpdate), sizeof( ((idEntity::entityFlags_s *)0)->forcePhysicsUpdate ) },
//	{ "bool", "selected", (intptr_t)(&((idEntity::entityFlags_s *)0)->selected), sizeof( ((idEntity::entityFlags_s *)0)->selected ) },
//	{ "bool", "neverDormant", (intptr_t)(&((idEntity::entityFlags_s *)0)->neverDormant), sizeof( ((idEntity::entityFlags_s *)0)->neverDormant ) },
//	{ "bool", "isDormant", (intptr_t)(&((idEntity::entityFlags_s *)0)->isDormant), sizeof( ((idEntity::entityFlags_s *)0)->isDormant ) },
//	{ "bool", "hasAwakened", (intptr_t)(&((idEntity::entityFlags_s *)0)->hasAwakened), sizeof( ((idEntity::entityFlags_s *)0)->hasAwakened ) },
//	{ "bool", "networkSync", (intptr_t)(&((idEntity::entityFlags_s *)0)->networkSync), sizeof( ((idEntity::entityFlags_s *)0)->networkSync ) },
//	{ "bool", "grabbed", (intptr_t)(&((idEntity::entityFlags_s *)0)->grabbed), sizeof( ((idEntity::entityFlags_s *)0)->grabbed ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEntity_typeInfo[] = {
	{ "int", "entityNumber", (intptr_t)(&((idEntity *)0)->entityNumber), sizeof( ((idEntity *)0)->entityNumber ) },
	{ "int", "entityDefNumber", (intptr_t)(&((idEntity *)0)->entityDefNumber), sizeof( ((idEntity *)0)->entityDefNumber ) },
	{ "idLinkList < idEntity >", "spawnNode", (intptr_t)(&((idEntity *)0)->spawnNode), sizeof( ((idEntity *)0)->spawnNode ) },
	{ "idLinkList < idEntity >", "activeNode", (intptr_t)(&((idEntity *)0)->activeNode), sizeof( ((idEntity *)0)->activeNode ) },
	{ "idLinkList < idEntity >", "snapshotNode", (intptr_t)(&((idEntity *)0)->snapshotNode), sizeof( ((idEntity *)0)->snapshotNode ) },
	{ "int", "snapshotSequence", (intptr_t)(&((idEntity *)0)->snapshotSequence), sizeof( ((idEntity *)0)->snapshotSequence ) },
	{ "int", "snapshotBits", (intptr_t)(&((idEntity *)0)->snapshotBits), sizeof( ((idEntity *)0)->snapshotBits ) },
	{ "idStr", "name", (intptr_t)(&((idEntity *)0)->name), sizeof( ((idEntity *)0)->name ) },
	{ "idDict", "spawnArgs", (intptr_t)(&((idEntity *)0)->spawnArgs), sizeof( ((idEntity *)0)->spawnArgs ) },
	{ "idScriptObject", "scriptObject", (intptr_t)(&((idEntity *)0)->scriptObject), sizeof( ((idEntity *)0)->scriptObject ) },
	{ "int", "thinkFlags", (intptr_t)(&((idEntity *)0)->thinkFlags), sizeof( ((idEntity *)0)->thinkFlags ) },
	{ "int", "dormantStart", (intptr_t)(&((idEntity *)0)->dormantStart), sizeof( ((idEntity *)0)->dormantStart ) },
	{ "bool", "cinematic", (intptr_t)(&((idEntity *)0)->cinematic), sizeof( ((idEntity *)0)->cinematic ) },
	{ "renderView_t *", "renderView", (intptr_t)(&((idEntity *)0)->renderView), sizeof( ((idEntity *)0)->renderView ) },
	{ "idEntity *", "cameraTarget", (intptr_t)(&((idEntity *)0)->cameraTarget), sizeof( ((idEntity *)0)->cameraTarget ) },
	{ "idList < idEntityPtr < idEntity > >", "targets", (intptr_t)(&((idEntity *)0)->targets), sizeof( ((idEntity *)0)->targets ) },
	{ "int", "health", (intptr_t)(&((idEntity *)0)->health), sizeof( ((idEntity *)0)->health ) },
	{ "idEntity::entityFlags_s", "fl", (intptr_t)(&((idEntity *)0)->fl), sizeof( ((idEntity *)0)->fl ) },
	{ "int", "timeGroup", (intptr_t)(&((idEntity *)0)->timeGroup), sizeof( ((idEntity *)0)->timeGroup ) },
	{ "bool", "noGrab", (intptr_t)(&((idEntity *)0)->noGrab), sizeof( ((idEntity *)0)->noGrab ) },
	{ "renderEntity_t", "xrayEntity", (intptr_t)(&((idEntity *)0)->xrayEntity), sizeof( ((idEntity *)0)->xrayEntity ) },
	{ "qhandle_t", "xrayEntityHandle", (intptr_t)(&((idEntity *)0)->xrayEntityHandle), sizeof( ((idEntity *)0)->xrayEntityHandle ) },
	{ "const idDeclSkin *", "xraySkin", (intptr_t)(&((idEntity *)0)->xraySkin), sizeof( ((idEntity *)0)->xraySkin ) },
	{ ": renderEntity_t", "renderEntity", (intptr_t)(&((idEntity *)0)->renderEntity), sizeof( ((idEntity *)0)->renderEntity ) },
	{ "int", "modelDefHandle", (intptr_t)(&((idEntity *)0)->modelDefHandle), sizeof( ((idEntity *)0)->modelDefHandle ) },
	{ "refSound_t", "refSound", (intptr_t)(&((idEntity *)0)->refSound), sizeof( ((idEntity *)0)->refSound ) },
	{ ": idLinkList < rvClientEntity >", "clientEntities", (intptr_t)(&((idEntity *)0)->clientEntities), sizeof( ((idEntity *)0)->clientEntities ) },
	{ ": idEntity *", "bindMaster", (intptr_t)(&((idEntity *)0)->bindMaster), sizeof( ((idEntity *)0)->bindMaster ) },
	{ "jointHandle_t", "bindJoint", (intptr_t)(&((idEntity *)0)->bindJoint), sizeof( ((idEntity *)0)->bindJoint ) },
	{ ": idPhysics_Static", "defaultPhysicsObj", (intptr_t)(&((idEntity *)0)->defaultPhysicsObj), sizeof( ((idEntity *)0)->defaultPhysicsObj ) },
	{ "idPhysics *", "physics", (intptr_t)(&((idEntity *)0)->physics), sizeof( ((idEntity *)0)->physics ) },
	{ "int", "bindBody", (intptr_t)(&((idEntity *)0)->bindBody), sizeof( ((idEntity *)0)->bindBody ) },
	{ "idEntity *", "teamMaster", (intptr_t)(&((idEntity *)0)->teamMaster), sizeof( ((idEntity *)0)->teamMaster ) },
	{ "idEntity *", "teamChain", (intptr_t)(&((idEntity *)0)->teamChain), sizeof( ((idEntity *)0)->teamChain ) },
	{ "int", "numPVSAreas", (intptr_t)(&((idEntity *)0)->numPVSAreas), sizeof( ((idEntity *)0)->numPVSAreas ) },
	{ "int[4]", "PVSAreas", (intptr_t)(&((idEntity *)0)->PVSAreas), sizeof( ((idEntity *)0)->PVSAreas ) },
	{ "signalList_t *", "signals", (intptr_t)(&((idEntity *)0)->signals), sizeof( ((idEntity *)0)->signals ) },
	{ "int", "mpGUIState", (intptr_t)(&((idEntity *)0)->mpGUIState), sizeof( ((idEntity *)0)->mpGUIState ) },
	{ "idList < DnComponent * >", "entityComponents", (intptr_t)(&((idEntity *)0)->entityComponents), sizeof( ((idEntity *)0)->entityComponents ) },
	{ NULL, 0 }
};

static classVariableInfo_t damageEffect_t_typeInfo[] = {
	{ "jointHandle_t", "jointNum", (intptr_t)(&((damageEffect_t *)0)->jointNum), sizeof( ((damageEffect_t *)0)->jointNum ) },
	{ "idVec3", "localOrigin", (intptr_t)(&((damageEffect_t *)0)->localOrigin), sizeof( ((damageEffect_t *)0)->localOrigin ) },
	{ "idVec3", "localNormal", (intptr_t)(&((damageEffect_t *)0)->localNormal), sizeof( ((damageEffect_t *)0)->localNormal ) },
	{ "int", "time", (intptr_t)(&((damageEffect_t *)0)->time), sizeof( ((damageEffect_t *)0)->time ) },
	{ "const idDeclParticle *", "type", (intptr_t)(&((damageEffect_t *)0)->type), sizeof( ((damageEffect_t *)0)->type ) },
	{ "damageEffect_s *", "next", (intptr_t)(&((damageEffect_t *)0)->next), sizeof( ((damageEffect_t *)0)->next ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAnimatedEntity_typeInfo[] = {
	{ ": idAnimator", "animator", (intptr_t)(&((idAnimatedEntity *)0)->animator), sizeof( ((idAnimatedEntity *)0)->animator ) },
	{ "damageEffect_t *", "damageEffects", (intptr_t)(&((idAnimatedEntity *)0)->damageEffects), sizeof( ((idAnimatedEntity *)0)->damageEffects ) },
	{ NULL, 0 }
};

static classVariableInfo_t SetTimeState_typeInfo[] = {
	{ ": bool", "activated", (intptr_t)(&((SetTimeState *)0)->activated), sizeof( ((SetTimeState *)0)->activated ) },
	{ "bool", "previousFast", (intptr_t)(&((SetTimeState *)0)->previousFast), sizeof( ((SetTimeState *)0)->previousFast ) },
	{ "bool", "fast", (intptr_t)(&((SetTimeState *)0)->fast), sizeof( ((SetTimeState *)0)->fast ) },
	{ NULL, 0 }
};

static classVariableInfo_t idCursor3D_typeInfo[] = {
	{ "idForce_Drag", "drag", (intptr_t)(&((idCursor3D *)0)->drag), sizeof( ((idCursor3D *)0)->drag ) },
	{ "idVec3", "draggedPosition", (intptr_t)(&((idCursor3D *)0)->draggedPosition), sizeof( ((idCursor3D *)0)->draggedPosition ) },
	{ NULL, 0 }
};

static classVariableInfo_t idDragEntity_typeInfo[] = {
	{ ": idEntityPtr < idEntity >", "dragEnt", (intptr_t)(&((idDragEntity *)0)->dragEnt), sizeof( ((idDragEntity *)0)->dragEnt ) },
	{ "jointHandle_t", "joint", (intptr_t)(&((idDragEntity *)0)->joint), sizeof( ((idDragEntity *)0)->joint ) },
	{ "int", "id", (intptr_t)(&((idDragEntity *)0)->id), sizeof( ((idDragEntity *)0)->id ) },
	{ "idVec3", "localEntityPoint", (intptr_t)(&((idDragEntity *)0)->localEntityPoint), sizeof( ((idDragEntity *)0)->localEntityPoint ) },
	{ "idVec3", "localPlayerPoint", (intptr_t)(&((idDragEntity *)0)->localPlayerPoint), sizeof( ((idDragEntity *)0)->localPlayerPoint ) },
	{ "idStr", "bodyName", (intptr_t)(&((idDragEntity *)0)->bodyName), sizeof( ((idDragEntity *)0)->bodyName ) },
	{ "idCursor3D *", "cursor", (intptr_t)(&((idDragEntity *)0)->cursor), sizeof( ((idDragEntity *)0)->cursor ) },
	{ "idEntityPtr < idEntity >", "selected", (intptr_t)(&((idDragEntity *)0)->selected), sizeof( ((idDragEntity *)0)->selected ) },
	{ NULL, 0 }
};

static classVariableInfo_t selectedTypeInfo_t_typeInfo[] = {
	{ "idTypeInfo *", "typeInfo", (intptr_t)(&((selectedTypeInfo_t *)0)->typeInfo), sizeof( ((selectedTypeInfo_t *)0)->typeInfo ) },
	{ "idStr", "textKey", (intptr_t)(&((selectedTypeInfo_t *)0)->textKey), sizeof( ((selectedTypeInfo_t *)0)->textKey ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEditEntities_typeInfo[] = {
	{ ": int", "nextSelectTime", (intptr_t)(&((idEditEntities *)0)->nextSelectTime), sizeof( ((idEditEntities *)0)->nextSelectTime ) },
	{ "idList < selectedTypeInfo_t >", "selectableEntityClasses", (intptr_t)(&((idEditEntities *)0)->selectableEntityClasses), sizeof( ((idEditEntities *)0)->selectableEntityClasses ) },
	{ "idList < idEntity * >", "selectedEntities", (intptr_t)(&((idEditEntities *)0)->selectedEntities), sizeof( ((idEditEntities *)0)->selectedEntities ) },
	{ NULL, 0 }
};

static classVariableInfo_t jointConversion_t_typeInfo[] = {
	{ "int", "bodyId", (intptr_t)(&((jointConversion_t *)0)->bodyId), sizeof( ((jointConversion_t *)0)->bodyId ) },
	{ "jointHandle_t", "jointHandle", (intptr_t)(&((jointConversion_t *)0)->jointHandle), sizeof( ((jointConversion_t *)0)->jointHandle ) },
	{ "AFJointModType_t", "jointMod", (intptr_t)(&((jointConversion_t *)0)->jointMod), sizeof( ((jointConversion_t *)0)->jointMod ) },
	{ "idVec3", "jointBodyOrigin", (intptr_t)(&((jointConversion_t *)0)->jointBodyOrigin), sizeof( ((jointConversion_t *)0)->jointBodyOrigin ) },
	{ "idMat3", "jointBodyAxis", (intptr_t)(&((jointConversion_t *)0)->jointBodyAxis), sizeof( ((jointConversion_t *)0)->jointBodyAxis ) },
	{ NULL, 0 }
};

static classVariableInfo_t afTouch_t_typeInfo[] = {
	{ "idEntity *", "touchedEnt", (intptr_t)(&((afTouch_t *)0)->touchedEnt), sizeof( ((afTouch_t *)0)->touchedEnt ) },
	{ "idClipModel *", "touchedClipModel", (intptr_t)(&((afTouch_t *)0)->touchedClipModel), sizeof( ((afTouch_t *)0)->touchedClipModel ) },
	{ "idAFBody *", "touchedByBody", (intptr_t)(&((afTouch_t *)0)->touchedByBody), sizeof( ((afTouch_t *)0)->touchedByBody ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAF_typeInfo[] = {
	{ ": idStr", "name", (intptr_t)(&((idAF *)0)->name), sizeof( ((idAF *)0)->name ) },
	{ "idPhysics_AF", "physicsObj", (intptr_t)(&((idAF *)0)->physicsObj), sizeof( ((idAF *)0)->physicsObj ) },
	{ "idEntity *", "self", (intptr_t)(&((idAF *)0)->self), sizeof( ((idAF *)0)->self ) },
	{ "idAnimator *", "animator", (intptr_t)(&((idAF *)0)->animator), sizeof( ((idAF *)0)->animator ) },
	{ "int", "modifiedAnim", (intptr_t)(&((idAF *)0)->modifiedAnim), sizeof( ((idAF *)0)->modifiedAnim ) },
	{ "idVec3", "baseOrigin", (intptr_t)(&((idAF *)0)->baseOrigin), sizeof( ((idAF *)0)->baseOrigin ) },
	{ "idMat3", "baseAxis", (intptr_t)(&((idAF *)0)->baseAxis), sizeof( ((idAF *)0)->baseAxis ) },
	{ "idList < jointConversion_t >", "jointMods", (intptr_t)(&((idAF *)0)->jointMods), sizeof( ((idAF *)0)->jointMods ) },
	{ "idList < int >", "jointBody", (intptr_t)(&((idAF *)0)->jointBody), sizeof( ((idAF *)0)->jointBody ) },
	{ "int", "poseTime", (intptr_t)(&((idAF *)0)->poseTime), sizeof( ((idAF *)0)->poseTime ) },
	{ "int", "restStartTime", (intptr_t)(&((idAF *)0)->restStartTime), sizeof( ((idAF *)0)->restStartTime ) },
	{ "bool", "isLoaded", (intptr_t)(&((idAF *)0)->isLoaded), sizeof( ((idAF *)0)->isLoaded ) },
	{ "bool", "isActive", (intptr_t)(&((idAF *)0)->isActive), sizeof( ((idAF *)0)->isActive ) },
	{ "bool", "hasBindConstraints", (intptr_t)(&((idAF *)0)->hasBindConstraints), sizeof( ((idAF *)0)->hasBindConstraints ) },
	{ NULL, 0 }
};

static classVariableInfo_t idIK_typeInfo[] = {
	{ ": bool", "initialized", (intptr_t)(&((idIK *)0)->initialized), sizeof( ((idIK *)0)->initialized ) },
	{ "bool", "ik_activate", (intptr_t)(&((idIK *)0)->ik_activate), sizeof( ((idIK *)0)->ik_activate ) },
	{ "idEntity *", "self", (intptr_t)(&((idIK *)0)->self), sizeof( ((idIK *)0)->self ) },
	{ "idAnimator *", "animator", (intptr_t)(&((idIK *)0)->animator), sizeof( ((idIK *)0)->animator ) },
	{ "int", "modifiedAnim", (intptr_t)(&((idIK *)0)->modifiedAnim), sizeof( ((idIK *)0)->modifiedAnim ) },
	{ "idVec3", "modelOffset", (intptr_t)(&((idIK *)0)->modelOffset), sizeof( ((idIK *)0)->modelOffset ) },
	{ NULL, 0 }
};

static classVariableInfo_t idIK_Walk_typeInfo[] = {
	{ "idClipModel *", "footModel", (intptr_t)(&((idIK_Walk *)0)->footModel), sizeof( ((idIK_Walk *)0)->footModel ) },
	{ "int", "numLegs", (intptr_t)(&((idIK_Walk *)0)->numLegs), sizeof( ((idIK_Walk *)0)->numLegs ) },
	{ "int", "enabledLegs", (intptr_t)(&((idIK_Walk *)0)->enabledLegs), sizeof( ((idIK_Walk *)0)->enabledLegs ) },
	{ "jointHandle_t[8]", "footJoints", (intptr_t)(&((idIK_Walk *)0)->footJoints), sizeof( ((idIK_Walk *)0)->footJoints ) },
	{ "jointHandle_t[8]", "ankleJoints", (intptr_t)(&((idIK_Walk *)0)->ankleJoints), sizeof( ((idIK_Walk *)0)->ankleJoints ) },
	{ "jointHandle_t[8]", "kneeJoints", (intptr_t)(&((idIK_Walk *)0)->kneeJoints), sizeof( ((idIK_Walk *)0)->kneeJoints ) },
	{ "jointHandle_t[8]", "hipJoints", (intptr_t)(&((idIK_Walk *)0)->hipJoints), sizeof( ((idIK_Walk *)0)->hipJoints ) },
	{ "jointHandle_t[8]", "dirJoints", (intptr_t)(&((idIK_Walk *)0)->dirJoints), sizeof( ((idIK_Walk *)0)->dirJoints ) },
	{ "jointHandle_t", "waistJoint", (intptr_t)(&((idIK_Walk *)0)->waistJoint), sizeof( ((idIK_Walk *)0)->waistJoint ) },
	{ "idVec3[8]", "hipForward", (intptr_t)(&((idIK_Walk *)0)->hipForward), sizeof( ((idIK_Walk *)0)->hipForward ) },
	{ "idVec3[8]", "kneeForward", (intptr_t)(&((idIK_Walk *)0)->kneeForward), sizeof( ((idIK_Walk *)0)->kneeForward ) },
	{ "float[8]", "upperLegLength", (intptr_t)(&((idIK_Walk *)0)->upperLegLength), sizeof( ((idIK_Walk *)0)->upperLegLength ) },
	{ "float[8]", "lowerLegLength", (intptr_t)(&((idIK_Walk *)0)->lowerLegLength), sizeof( ((idIK_Walk *)0)->lowerLegLength ) },
	{ "idMat3[8]", "upperLegToHipJoint", (intptr_t)(&((idIK_Walk *)0)->upperLegToHipJoint), sizeof( ((idIK_Walk *)0)->upperLegToHipJoint ) },
	{ "idMat3[8]", "lowerLegToKneeJoint", (intptr_t)(&((idIK_Walk *)0)->lowerLegToKneeJoint), sizeof( ((idIK_Walk *)0)->lowerLegToKneeJoint ) },
	{ "float", "smoothing", (intptr_t)(&((idIK_Walk *)0)->smoothing), sizeof( ((idIK_Walk *)0)->smoothing ) },
	{ "float", "waistSmoothing", (intptr_t)(&((idIK_Walk *)0)->waistSmoothing), sizeof( ((idIK_Walk *)0)->waistSmoothing ) },
	{ "float", "footShift", (intptr_t)(&((idIK_Walk *)0)->footShift), sizeof( ((idIK_Walk *)0)->footShift ) },
	{ "float", "waistShift", (intptr_t)(&((idIK_Walk *)0)->waistShift), sizeof( ((idIK_Walk *)0)->waistShift ) },
	{ "float", "minWaistFloorDist", (intptr_t)(&((idIK_Walk *)0)->minWaistFloorDist), sizeof( ((idIK_Walk *)0)->minWaistFloorDist ) },
	{ "float", "minWaistAnkleDist", (intptr_t)(&((idIK_Walk *)0)->minWaistAnkleDist), sizeof( ((idIK_Walk *)0)->minWaistAnkleDist ) },
	{ "float", "footUpTrace", (intptr_t)(&((idIK_Walk *)0)->footUpTrace), sizeof( ((idIK_Walk *)0)->footUpTrace ) },
	{ "float", "footDownTrace", (intptr_t)(&((idIK_Walk *)0)->footDownTrace), sizeof( ((idIK_Walk *)0)->footDownTrace ) },
	{ "bool", "tiltWaist", (intptr_t)(&((idIK_Walk *)0)->tiltWaist), sizeof( ((idIK_Walk *)0)->tiltWaist ) },
	{ "bool", "usePivot", (intptr_t)(&((idIK_Walk *)0)->usePivot), sizeof( ((idIK_Walk *)0)->usePivot ) },
	{ "int", "pivotFoot", (intptr_t)(&((idIK_Walk *)0)->pivotFoot), sizeof( ((idIK_Walk *)0)->pivotFoot ) },
	{ "float", "pivotYaw", (intptr_t)(&((idIK_Walk *)0)->pivotYaw), sizeof( ((idIK_Walk *)0)->pivotYaw ) },
	{ "idVec3", "pivotPos", (intptr_t)(&((idIK_Walk *)0)->pivotPos), sizeof( ((idIK_Walk *)0)->pivotPos ) },
	{ "bool", "oldHeightsValid", (intptr_t)(&((idIK_Walk *)0)->oldHeightsValid), sizeof( ((idIK_Walk *)0)->oldHeightsValid ) },
	{ "float", "oldWaistHeight", (intptr_t)(&((idIK_Walk *)0)->oldWaistHeight), sizeof( ((idIK_Walk *)0)->oldWaistHeight ) },
	{ "float[8]", "oldAnkleHeights", (intptr_t)(&((idIK_Walk *)0)->oldAnkleHeights), sizeof( ((idIK_Walk *)0)->oldAnkleHeights ) },
	{ "idVec3", "waistOffset", (intptr_t)(&((idIK_Walk *)0)->waistOffset), sizeof( ((idIK_Walk *)0)->waistOffset ) },
	{ NULL, 0 }
};

static classVariableInfo_t idIK_Reach_typeInfo[] = {
	{ "int", "numArms", (intptr_t)(&((idIK_Reach *)0)->numArms), sizeof( ((idIK_Reach *)0)->numArms ) },
	{ "int", "enabledArms", (intptr_t)(&((idIK_Reach *)0)->enabledArms), sizeof( ((idIK_Reach *)0)->enabledArms ) },
	{ "jointHandle_t[2]", "handJoints", (intptr_t)(&((idIK_Reach *)0)->handJoints), sizeof( ((idIK_Reach *)0)->handJoints ) },
	{ "jointHandle_t[2]", "elbowJoints", (intptr_t)(&((idIK_Reach *)0)->elbowJoints), sizeof( ((idIK_Reach *)0)->elbowJoints ) },
	{ "jointHandle_t[2]", "shoulderJoints", (intptr_t)(&((idIK_Reach *)0)->shoulderJoints), sizeof( ((idIK_Reach *)0)->shoulderJoints ) },
	{ "jointHandle_t[2]", "dirJoints", (intptr_t)(&((idIK_Reach *)0)->dirJoints), sizeof( ((idIK_Reach *)0)->dirJoints ) },
	{ "idVec3[2]", "shoulderForward", (intptr_t)(&((idIK_Reach *)0)->shoulderForward), sizeof( ((idIK_Reach *)0)->shoulderForward ) },
	{ "idVec3[2]", "elbowForward", (intptr_t)(&((idIK_Reach *)0)->elbowForward), sizeof( ((idIK_Reach *)0)->elbowForward ) },
	{ "float[2]", "upperArmLength", (intptr_t)(&((idIK_Reach *)0)->upperArmLength), sizeof( ((idIK_Reach *)0)->upperArmLength ) },
	{ "float[2]", "lowerArmLength", (intptr_t)(&((idIK_Reach *)0)->lowerArmLength), sizeof( ((idIK_Reach *)0)->lowerArmLength ) },
	{ "idMat3[2]", "upperArmToShoulderJoint", (intptr_t)(&((idIK_Reach *)0)->upperArmToShoulderJoint), sizeof( ((idIK_Reach *)0)->upperArmToShoulderJoint ) },
	{ "idMat3[2]", "lowerArmToElbowJoint", (intptr_t)(&((idIK_Reach *)0)->lowerArmToElbowJoint), sizeof( ((idIK_Reach *)0)->lowerArmToElbowJoint ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMultiModelAF_typeInfo[] = {
	{ ": idPhysics_AF", "physicsObj", (intptr_t)(&((idMultiModelAF *)0)->physicsObj), sizeof( ((idMultiModelAF *)0)->physicsObj ) },
	{ ": idList < idRenderModel * >", "modelHandles", (intptr_t)(&((idMultiModelAF *)0)->modelHandles), sizeof( ((idMultiModelAF *)0)->modelHandles ) },
	{ "idList < int >", "modelDefHandles", (intptr_t)(&((idMultiModelAF *)0)->modelDefHandles), sizeof( ((idMultiModelAF *)0)->modelDefHandles ) },
	{ NULL, 0 }
};

static classVariableInfo_t idChain_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idAFAttachment_typeInfo[] = {
	{ ": idEntity *", "body", (intptr_t)(&((idAFAttachment *)0)->body), sizeof( ((idAFAttachment *)0)->body ) },
	{ "idClipModel *", "combatModel", (intptr_t)(&((idAFAttachment *)0)->combatModel), sizeof( ((idAFAttachment *)0)->combatModel ) },
	{ "int", "idleAnim", (intptr_t)(&((idAFAttachment *)0)->idleAnim), sizeof( ((idAFAttachment *)0)->idleAnim ) },
	{ "jointHandle_t", "attachJoint", (intptr_t)(&((idAFAttachment *)0)->attachJoint), sizeof( ((idAFAttachment *)0)->attachJoint ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_Base_typeInfo[] = {
	{ ": idAF", "af", (intptr_t)(&((idAFEntity_Base *)0)->af), sizeof( ((idAFEntity_Base *)0)->af ) },
	{ "idClipModel *", "combatModel", (intptr_t)(&((idAFEntity_Base *)0)->combatModel), sizeof( ((idAFEntity_Base *)0)->combatModel ) },
	{ "int", "combatModelContents", (intptr_t)(&((idAFEntity_Base *)0)->combatModelContents), sizeof( ((idAFEntity_Base *)0)->combatModelContents ) },
	{ "idVec3", "spawnOrigin", (intptr_t)(&((idAFEntity_Base *)0)->spawnOrigin), sizeof( ((idAFEntity_Base *)0)->spawnOrigin ) },
	{ "idMat3", "spawnAxis", (intptr_t)(&((idAFEntity_Base *)0)->spawnAxis), sizeof( ((idAFEntity_Base *)0)->spawnAxis ) },
	{ "int", "nextSoundTime", (intptr_t)(&((idAFEntity_Base *)0)->nextSoundTime), sizeof( ((idAFEntity_Base *)0)->nextSoundTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_Gibbable_typeInfo[] = {
	{ ": idRenderModel *", "skeletonModel", (intptr_t)(&((idAFEntity_Gibbable *)0)->skeletonModel), sizeof( ((idAFEntity_Gibbable *)0)->skeletonModel ) },
	{ "int", "skeletonModelDefHandle", (intptr_t)(&((idAFEntity_Gibbable *)0)->skeletonModelDefHandle), sizeof( ((idAFEntity_Gibbable *)0)->skeletonModelDefHandle ) },
	{ "bool", "gibbed", (intptr_t)(&((idAFEntity_Gibbable *)0)->gibbed), sizeof( ((idAFEntity_Gibbable *)0)->gibbed ) },
	{ "bool", "wasThrown", (intptr_t)(&((idAFEntity_Gibbable *)0)->wasThrown), sizeof( ((idAFEntity_Gibbable *)0)->wasThrown ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_Generic_typeInfo[] = {
	{ "bool", "keepRunningPhysics", (intptr_t)(&((idAFEntity_Generic *)0)->keepRunningPhysics), sizeof( ((idAFEntity_Generic *)0)->keepRunningPhysics ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_WithAttachedHead_typeInfo[] = {
	{ ": idEntityPtr < idAFAttachment >", "head", (intptr_t)(&((idAFEntity_WithAttachedHead *)0)->head), sizeof( ((idAFEntity_WithAttachedHead *)0)->head ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_Vehicle_typeInfo[] = {
	{ ": idPlayer *", "player", (intptr_t)(&((idAFEntity_Vehicle *)0)->player), sizeof( ((idAFEntity_Vehicle *)0)->player ) },
	{ "jointHandle_t", "eyesJoint", (intptr_t)(&((idAFEntity_Vehicle *)0)->eyesJoint), sizeof( ((idAFEntity_Vehicle *)0)->eyesJoint ) },
	{ "jointHandle_t", "steeringWheelJoint", (intptr_t)(&((idAFEntity_Vehicle *)0)->steeringWheelJoint), sizeof( ((idAFEntity_Vehicle *)0)->steeringWheelJoint ) },
	{ "float", "wheelRadius", (intptr_t)(&((idAFEntity_Vehicle *)0)->wheelRadius), sizeof( ((idAFEntity_Vehicle *)0)->wheelRadius ) },
	{ "float", "steerAngle", (intptr_t)(&((idAFEntity_Vehicle *)0)->steerAngle), sizeof( ((idAFEntity_Vehicle *)0)->steerAngle ) },
	{ "float", "steerSpeed", (intptr_t)(&((idAFEntity_Vehicle *)0)->steerSpeed), sizeof( ((idAFEntity_Vehicle *)0)->steerSpeed ) },
	{ "const idDeclParticle *", "dustSmoke", (intptr_t)(&((idAFEntity_Vehicle *)0)->dustSmoke), sizeof( ((idAFEntity_Vehicle *)0)->dustSmoke ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_VehicleSimple_typeInfo[] = {
	{ ": idClipModel *", "wheelModel", (intptr_t)(&((idAFEntity_VehicleSimple *)0)->wheelModel), sizeof( ((idAFEntity_VehicleSimple *)0)->wheelModel ) },
	{ "idAFConstraint_Suspension *[4]", "suspension", (intptr_t)(&((idAFEntity_VehicleSimple *)0)->suspension), sizeof( ((idAFEntity_VehicleSimple *)0)->suspension ) },
	{ "jointHandle_t[4]", "wheelJoints", (intptr_t)(&((idAFEntity_VehicleSimple *)0)->wheelJoints), sizeof( ((idAFEntity_VehicleSimple *)0)->wheelJoints ) },
	{ "float[4]", "wheelAngles", (intptr_t)(&((idAFEntity_VehicleSimple *)0)->wheelAngles), sizeof( ((idAFEntity_VehicleSimple *)0)->wheelAngles ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_VehicleFourWheels_typeInfo[] = {
	{ ": idAFBody *[4]", "wheels", (intptr_t)(&((idAFEntity_VehicleFourWheels *)0)->wheels), sizeof( ((idAFEntity_VehicleFourWheels *)0)->wheels ) },
	{ "idAFConstraint_Hinge *[2]", "steering", (intptr_t)(&((idAFEntity_VehicleFourWheels *)0)->steering), sizeof( ((idAFEntity_VehicleFourWheels *)0)->steering ) },
	{ "jointHandle_t[4]", "wheelJoints", (intptr_t)(&((idAFEntity_VehicleFourWheels *)0)->wheelJoints), sizeof( ((idAFEntity_VehicleFourWheels *)0)->wheelJoints ) },
	{ "float[4]", "wheelAngles", (intptr_t)(&((idAFEntity_VehicleFourWheels *)0)->wheelAngles), sizeof( ((idAFEntity_VehicleFourWheels *)0)->wheelAngles ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_VehicleSixWheels_typeInfo[] = {
	{ "float", "force", (intptr_t)(&((idAFEntity_VehicleSixWheels *)0)->force), sizeof( ((idAFEntity_VehicleSixWheels *)0)->force ) },
	{ "float", "velocity", (intptr_t)(&((idAFEntity_VehicleSixWheels *)0)->velocity), sizeof( ((idAFEntity_VehicleSixWheels *)0)->velocity ) },
	{ "float", "steerAngle", (intptr_t)(&((idAFEntity_VehicleSixWheels *)0)->steerAngle), sizeof( ((idAFEntity_VehicleSixWheels *)0)->steerAngle ) },
	{ ": idAFBody *[6]", "wheels", (intptr_t)(&((idAFEntity_VehicleSixWheels *)0)->wheels), sizeof( ((idAFEntity_VehicleSixWheels *)0)->wheels ) },
	{ "idAFConstraint_Hinge *[4]", "steering", (intptr_t)(&((idAFEntity_VehicleSixWheels *)0)->steering), sizeof( ((idAFEntity_VehicleSixWheels *)0)->steering ) },
	{ "jointHandle_t[6]", "wheelJoints", (intptr_t)(&((idAFEntity_VehicleSixWheels *)0)->wheelJoints), sizeof( ((idAFEntity_VehicleSixWheels *)0)->wheelJoints ) },
	{ "float[6]", "wheelAngles", (intptr_t)(&((idAFEntity_VehicleSixWheels *)0)->wheelAngles), sizeof( ((idAFEntity_VehicleSixWheels *)0)->wheelAngles ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_VehicleAutomated_typeInfo[] = {
	{ ": idEntity *", "waypoint", (intptr_t)(&((idAFEntity_VehicleAutomated *)0)->waypoint), sizeof( ((idAFEntity_VehicleAutomated *)0)->waypoint ) },
	{ "float", "steeringSpeed", (intptr_t)(&((idAFEntity_VehicleAutomated *)0)->steeringSpeed), sizeof( ((idAFEntity_VehicleAutomated *)0)->steeringSpeed ) },
	{ "float", "currentSteering", (intptr_t)(&((idAFEntity_VehicleAutomated *)0)->currentSteering), sizeof( ((idAFEntity_VehicleAutomated *)0)->currentSteering ) },
	{ "float", "idealSteering", (intptr_t)(&((idAFEntity_VehicleAutomated *)0)->idealSteering), sizeof( ((idAFEntity_VehicleAutomated *)0)->idealSteering ) },
	{ "float", "originHeight", (intptr_t)(&((idAFEntity_VehicleAutomated *)0)->originHeight), sizeof( ((idAFEntity_VehicleAutomated *)0)->originHeight ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_SteamPipe_typeInfo[] = {
	{ ": int", "steamBody", (intptr_t)(&((idAFEntity_SteamPipe *)0)->steamBody), sizeof( ((idAFEntity_SteamPipe *)0)->steamBody ) },
	{ "float", "steamForce", (intptr_t)(&((idAFEntity_SteamPipe *)0)->steamForce), sizeof( ((idAFEntity_SteamPipe *)0)->steamForce ) },
	{ "float", "steamUpForce", (intptr_t)(&((idAFEntity_SteamPipe *)0)->steamUpForce), sizeof( ((idAFEntity_SteamPipe *)0)->steamUpForce ) },
	{ "idForce_Constant", "force", (intptr_t)(&((idAFEntity_SteamPipe *)0)->force), sizeof( ((idAFEntity_SteamPipe *)0)->force ) },
	{ "renderEntity_t", "steamRenderEntity", (intptr_t)(&((idAFEntity_SteamPipe *)0)->steamRenderEntity), sizeof( ((idAFEntity_SteamPipe *)0)->steamRenderEntity ) },
	{ "qhandle_t", "steamModelDefHandle", (intptr_t)(&((idAFEntity_SteamPipe *)0)->steamModelDefHandle), sizeof( ((idAFEntity_SteamPipe *)0)->steamModelDefHandle ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_ClawFourFingers_typeInfo[] = {
	{ ": idAFConstraint_Hinge *[4]", "fingers", (intptr_t)(&((idAFEntity_ClawFourFingers *)0)->fingers), sizeof( ((idAFEntity_ClawFourFingers *)0)->fingers ) },
	{ NULL, 0 }
};

static classVariableInfo_t idHarvestable_typeInfo[] = {
	{ ": idEntityPtr < idEntity >", "parentEnt", (intptr_t)(&((idHarvestable *)0)->parentEnt), sizeof( ((idHarvestable *)0)->parentEnt ) },
	{ "float", "triggersize", (intptr_t)(&((idHarvestable *)0)->triggersize), sizeof( ((idHarvestable *)0)->triggersize ) },
	{ "idClipModel *", "trigger", (intptr_t)(&((idHarvestable *)0)->trigger), sizeof( ((idHarvestable *)0)->trigger ) },
	{ "float", "giveDelay", (intptr_t)(&((idHarvestable *)0)->giveDelay), sizeof( ((idHarvestable *)0)->giveDelay ) },
	{ "float", "removeDelay", (intptr_t)(&((idHarvestable *)0)->removeDelay), sizeof( ((idHarvestable *)0)->removeDelay ) },
	{ "bool", "given", (intptr_t)(&((idHarvestable *)0)->given), sizeof( ((idHarvestable *)0)->given ) },
	{ "idEntityPtr < idPlayer >", "player", (intptr_t)(&((idHarvestable *)0)->player), sizeof( ((idHarvestable *)0)->player ) },
	{ "int", "startTime", (intptr_t)(&((idHarvestable *)0)->startTime), sizeof( ((idHarvestable *)0)->startTime ) },
	{ "bool", "fxFollowPlayer", (intptr_t)(&((idHarvestable *)0)->fxFollowPlayer), sizeof( ((idHarvestable *)0)->fxFollowPlayer ) },
	{ "idEntityPtr < idEntityFx >", "fx", (intptr_t)(&((idHarvestable *)0)->fx), sizeof( ((idHarvestable *)0)->fx ) },
	{ "idStr", "fxOrient", (intptr_t)(&((idHarvestable *)0)->fxOrient), sizeof( ((idHarvestable *)0)->fxOrient ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAFEntity_Harvest_typeInfo[] = {
	{ ": idEntityPtr < idHarvestable >", "harvestEnt", (intptr_t)(&((idAFEntity_Harvest *)0)->harvestEnt), sizeof( ((idAFEntity_Harvest *)0)->harvestEnt ) },
	{ NULL, 0 }
};

static classVariableInfo_t idSpawnableEntity_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idPlayerStart_typeInfo[] = {
	{ ": int", "teleportStage", (intptr_t)(&((idPlayerStart *)0)->teleportStage), sizeof( ((idPlayerStart *)0)->teleportStage ) },
	{ NULL, 0 }
};

static classVariableInfo_t idActivator_typeInfo[] = {
	{ ": bool", "stay_on", (intptr_t)(&((idActivator *)0)->stay_on), sizeof( ((idActivator *)0)->stay_on ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPathCorner_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idDamagable_typeInfo[] = {
	{ ": int", "count", (intptr_t)(&((idDamagable *)0)->count), sizeof( ((idDamagable *)0)->count ) },
	{ "int", "nextTriggerTime", (intptr_t)(&((idDamagable *)0)->nextTriggerTime), sizeof( ((idDamagable *)0)->nextTriggerTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idExplodable_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idSpring_typeInfo[] = {
	{ ": idEntity *", "ent1", (intptr_t)(&((idSpring *)0)->ent1), sizeof( ((idSpring *)0)->ent1 ) },
	{ "idEntity *", "ent2", (intptr_t)(&((idSpring *)0)->ent2), sizeof( ((idSpring *)0)->ent2 ) },
	{ "int", "id1", (intptr_t)(&((idSpring *)0)->id1), sizeof( ((idSpring *)0)->id1 ) },
	{ "int", "id2", (intptr_t)(&((idSpring *)0)->id2), sizeof( ((idSpring *)0)->id2 ) },
	{ "idVec3", "p1", (intptr_t)(&((idSpring *)0)->p1), sizeof( ((idSpring *)0)->p1 ) },
	{ "idVec3", "p2", (intptr_t)(&((idSpring *)0)->p2), sizeof( ((idSpring *)0)->p2 ) },
	{ "idForce_Spring", "spring", (intptr_t)(&((idSpring *)0)->spring), sizeof( ((idSpring *)0)->spring ) },
	{ NULL, 0 }
};

static classVariableInfo_t idForceField_typeInfo[] = {
	{ ": idForce_Field", "forceField", (intptr_t)(&((idForceField *)0)->forceField), sizeof( ((idForceField *)0)->forceField ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAnimated_typeInfo[] = {
	{ ": int", "num_anims", (intptr_t)(&((idAnimated *)0)->num_anims), sizeof( ((idAnimated *)0)->num_anims ) },
	{ "int", "current_anim_index", (intptr_t)(&((idAnimated *)0)->current_anim_index), sizeof( ((idAnimated *)0)->current_anim_index ) },
	{ "int", "anim", (intptr_t)(&((idAnimated *)0)->anim), sizeof( ((idAnimated *)0)->anim ) },
	{ "int", "blendFrames", (intptr_t)(&((idAnimated *)0)->blendFrames), sizeof( ((idAnimated *)0)->blendFrames ) },
	{ "jointHandle_t", "soundJoint", (intptr_t)(&((idAnimated *)0)->soundJoint), sizeof( ((idAnimated *)0)->soundJoint ) },
	{ "idEntityPtr < idEntity >", "activator", (intptr_t)(&((idAnimated *)0)->activator), sizeof( ((idAnimated *)0)->activator ) },
	{ "bool", "activated", (intptr_t)(&((idAnimated *)0)->activated), sizeof( ((idAnimated *)0)->activated ) },
	{ NULL, 0 }
};

static classVariableInfo_t idStaticEntity_typeInfo[] = {
	{ "int", "spawnTime", (intptr_t)(&((idStaticEntity *)0)->spawnTime), sizeof( ((idStaticEntity *)0)->spawnTime ) },
	{ "bool", "active", (intptr_t)(&((idStaticEntity *)0)->active), sizeof( ((idStaticEntity *)0)->active ) },
	{ "idVec4", "fadeFrom", (intptr_t)(&((idStaticEntity *)0)->fadeFrom), sizeof( ((idStaticEntity *)0)->fadeFrom ) },
	{ "idVec4", "fadeTo", (intptr_t)(&((idStaticEntity *)0)->fadeTo), sizeof( ((idStaticEntity *)0)->fadeTo ) },
	{ "int", "fadeStart", (intptr_t)(&((idStaticEntity *)0)->fadeStart), sizeof( ((idStaticEntity *)0)->fadeStart ) },
	{ "int", "fadeEnd", (intptr_t)(&((idStaticEntity *)0)->fadeEnd), sizeof( ((idStaticEntity *)0)->fadeEnd ) },
	{ "bool", "runGui", (intptr_t)(&((idStaticEntity *)0)->runGui), sizeof( ((idStaticEntity *)0)->runGui ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFuncEmitter_typeInfo[] = {
	{ ": bool", "hidden", (intptr_t)(&((idFuncEmitter *)0)->hidden), sizeof( ((idFuncEmitter *)0)->hidden ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFuncSmoke_typeInfo[] = {
	{ ": int", "smokeTime", (intptr_t)(&((idFuncSmoke *)0)->smokeTime), sizeof( ((idFuncSmoke *)0)->smokeTime ) },
	{ "const idDeclParticle *", "smoke", (intptr_t)(&((idFuncSmoke *)0)->smoke), sizeof( ((idFuncSmoke *)0)->smoke ) },
	{ "bool", "restart", (intptr_t)(&((idFuncSmoke *)0)->restart), sizeof( ((idFuncSmoke *)0)->restart ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFuncSplat_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTextEntity_typeInfo[] = {
	{ ": idStr", "text", (intptr_t)(&((idTextEntity *)0)->text), sizeof( ((idTextEntity *)0)->text ) },
	{ "bool", "playerOriented", (intptr_t)(&((idTextEntity *)0)->playerOriented), sizeof( ((idTextEntity *)0)->playerOriented ) },
	{ NULL, 0 }
};

static classVariableInfo_t idLocationEntity_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idLocationSeparatorEntity_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idVacuumSeparatorEntity_typeInfo[] = {
	{ ": qhandle_t", "portal", (intptr_t)(&((idVacuumSeparatorEntity *)0)->portal), sizeof( ((idVacuumSeparatorEntity *)0)->portal ) },
	{ NULL, 0 }
};

static classVariableInfo_t idVacuumEntity_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idBeam_typeInfo[] = {
	{ "idEntityPtr < idBeam >", "target", (intptr_t)(&((idBeam *)0)->target), sizeof( ((idBeam *)0)->target ) },
	{ "idEntityPtr < idBeam >", "master", (intptr_t)(&((idBeam *)0)->master), sizeof( ((idBeam *)0)->master ) },
	{ NULL, 0 }
};

static classVariableInfo_t idLiquid_typeInfo[] = {
	{ "idRenderModelLiquid *", "model", (intptr_t)(&((idLiquid *)0)->model), sizeof( ((idLiquid *)0)->model ) },
	{ NULL, 0 }
};

static classVariableInfo_t idShaking_typeInfo[] = {
	{ ": idPhysics_Parametric", "physicsObj", (intptr_t)(&((idShaking *)0)->physicsObj), sizeof( ((idShaking *)0)->physicsObj ) },
	{ "bool", "active", (intptr_t)(&((idShaking *)0)->active), sizeof( ((idShaking *)0)->active ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEarthQuake_typeInfo[] = {
	{ ": int", "nextTriggerTime", (intptr_t)(&((idEarthQuake *)0)->nextTriggerTime), sizeof( ((idEarthQuake *)0)->nextTriggerTime ) },
	{ "int", "shakeStopTime", (intptr_t)(&((idEarthQuake *)0)->shakeStopTime), sizeof( ((idEarthQuake *)0)->shakeStopTime ) },
	{ "float", "wait", (intptr_t)(&((idEarthQuake *)0)->wait), sizeof( ((idEarthQuake *)0)->wait ) },
	{ "float", "random", (intptr_t)(&((idEarthQuake *)0)->random), sizeof( ((idEarthQuake *)0)->random ) },
	{ "bool", "triggered", (intptr_t)(&((idEarthQuake *)0)->triggered), sizeof( ((idEarthQuake *)0)->triggered ) },
	{ "bool", "playerOriented", (intptr_t)(&((idEarthQuake *)0)->playerOriented), sizeof( ((idEarthQuake *)0)->playerOriented ) },
	{ "bool", "disabled", (intptr_t)(&((idEarthQuake *)0)->disabled), sizeof( ((idEarthQuake *)0)->disabled ) },
	{ "float", "shakeTime", (intptr_t)(&((idEarthQuake *)0)->shakeTime), sizeof( ((idEarthQuake *)0)->shakeTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFuncPortal_typeInfo[] = {
	{ ": qhandle_t", "portal", (intptr_t)(&((idFuncPortal *)0)->portal), sizeof( ((idFuncPortal *)0)->portal ) },
	{ "bool", "state", (intptr_t)(&((idFuncPortal *)0)->state), sizeof( ((idFuncPortal *)0)->state ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFuncRadioChatter_typeInfo[] = {
	{ ": float", "time", (intptr_t)(&((idFuncRadioChatter *)0)->time), sizeof( ((idFuncRadioChatter *)0)->time ) },
	{ NULL, 0 }
};

static classVariableInfo_t idShockwave_typeInfo[] = {
	{ "bool", "isActive", (intptr_t)(&((idShockwave *)0)->isActive), sizeof( ((idShockwave *)0)->isActive ) },
	{ "int", "startTime", (intptr_t)(&((idShockwave *)0)->startTime), sizeof( ((idShockwave *)0)->startTime ) },
	{ "int", "duration", (intptr_t)(&((idShockwave *)0)->duration), sizeof( ((idShockwave *)0)->duration ) },
	{ "float", "startSize", (intptr_t)(&((idShockwave *)0)->startSize), sizeof( ((idShockwave *)0)->startSize ) },
	{ "float", "endSize", (intptr_t)(&((idShockwave *)0)->endSize), sizeof( ((idShockwave *)0)->endSize ) },
	{ "float", "currentSize", (intptr_t)(&((idShockwave *)0)->currentSize), sizeof( ((idShockwave *)0)->currentSize ) },
	{ "float", "magnitude", (intptr_t)(&((idShockwave *)0)->magnitude), sizeof( ((idShockwave *)0)->magnitude ) },
	{ "float", "height", (intptr_t)(&((idShockwave *)0)->height), sizeof( ((idShockwave *)0)->height ) },
	{ "bool", "playerDamaged", (intptr_t)(&((idShockwave *)0)->playerDamaged), sizeof( ((idShockwave *)0)->playerDamaged ) },
	{ "float", "playerDamageSize", (intptr_t)(&((idShockwave *)0)->playerDamageSize), sizeof( ((idShockwave *)0)->playerDamageSize ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFuncMountedObject_typeInfo[] = {
	{ ": int", "harc", (intptr_t)(&((idFuncMountedObject *)0)->harc), sizeof( ((idFuncMountedObject *)0)->harc ) },
	{ "int", "varc", (intptr_t)(&((idFuncMountedObject *)0)->varc), sizeof( ((idFuncMountedObject *)0)->varc ) },
	{ ": bool", "isMounted", (intptr_t)(&((idFuncMountedObject *)0)->isMounted), sizeof( ((idFuncMountedObject *)0)->isMounted ) },
	{ "function_t *", "scriptFunction", (intptr_t)(&((idFuncMountedObject *)0)->scriptFunction), sizeof( ((idFuncMountedObject *)0)->scriptFunction ) },
	{ "idPlayer *", "mountedPlayer", (intptr_t)(&((idFuncMountedObject *)0)->mountedPlayer), sizeof( ((idFuncMountedObject *)0)->mountedPlayer ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFuncMountedWeapon_typeInfo[] = {
	{ ": idEntity *", "turret", (intptr_t)(&((idFuncMountedWeapon *)0)->turret), sizeof( ((idFuncMountedWeapon *)0)->turret ) },
	{ "idVec3", "muzzleOrigin", (intptr_t)(&((idFuncMountedWeapon *)0)->muzzleOrigin), sizeof( ((idFuncMountedWeapon *)0)->muzzleOrigin ) },
	{ "idMat3", "muzzleAxis", (intptr_t)(&((idFuncMountedWeapon *)0)->muzzleAxis), sizeof( ((idFuncMountedWeapon *)0)->muzzleAxis ) },
	{ "float", "weaponLastFireTime", (intptr_t)(&((idFuncMountedWeapon *)0)->weaponLastFireTime), sizeof( ((idFuncMountedWeapon *)0)->weaponLastFireTime ) },
	{ "float", "weaponFireDelay", (intptr_t)(&((idFuncMountedWeapon *)0)->weaponFireDelay), sizeof( ((idFuncMountedWeapon *)0)->weaponFireDelay ) },
	{ "const idDict *", "projectile", (intptr_t)(&((idFuncMountedWeapon *)0)->projectile), sizeof( ((idFuncMountedWeapon *)0)->projectile ) },
	{ "const idSoundShader *", "soundFireWeapon", (intptr_t)(&((idFuncMountedWeapon *)0)->soundFireWeapon), sizeof( ((idFuncMountedWeapon *)0)->soundFireWeapon ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPortalSky_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idAnimState_typeInfo[] = {
	{ ": bool", "idleAnim", (intptr_t)(&((idAnimState *)0)->idleAnim), sizeof( ((idAnimState *)0)->idleAnim ) },
	{ "int", "animBlendFrames", (intptr_t)(&((idAnimState *)0)->animBlendFrames), sizeof( ((idAnimState *)0)->animBlendFrames ) },
	{ "int", "lastAnimBlendFrames", (intptr_t)(&((idAnimState *)0)->lastAnimBlendFrames), sizeof( ((idAnimState *)0)->lastAnimBlendFrames ) },
	{ ": idEntity *", "self", (intptr_t)(&((idAnimState *)0)->self), sizeof( ((idAnimState *)0)->self ) },
	{ "idAnimator *", "animator", (intptr_t)(&((idAnimState *)0)->animator), sizeof( ((idAnimState *)0)->animator ) },
	{ "int", "channel", (intptr_t)(&((idAnimState *)0)->channel), sizeof( ((idAnimState *)0)->channel ) },
	{ "bool", "disabled", (intptr_t)(&((idAnimState *)0)->disabled), sizeof( ((idAnimState *)0)->disabled ) },
	{ "rvStateThread", "stateThread", (intptr_t)(&((idAnimState *)0)->stateThread), sizeof( ((idAnimState *)0)->stateThread ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAttachInfo_typeInfo[] = {
	{ ": idEntityPtr < idEntity >", "ent", (intptr_t)(&((idAttachInfo *)0)->ent), sizeof( ((idAttachInfo *)0)->ent ) },
	{ "int", "channel", (intptr_t)(&((idAttachInfo *)0)->channel), sizeof( ((idAttachInfo *)0)->channel ) },
	{ NULL, 0 }
};

static classVariableInfo_t copyJoints_t_typeInfo[] = {
	{ "jointModTransform_t", "mod", (intptr_t)(&((copyJoints_t *)0)->mod), sizeof( ((copyJoints_t *)0)->mod ) },
	{ "jointHandle_t", "from", (intptr_t)(&((copyJoints_t *)0)->from), sizeof( ((copyJoints_t *)0)->from ) },
	{ "jointHandle_t", "to", (intptr_t)(&((copyJoints_t *)0)->to), sizeof( ((copyJoints_t *)0)->to ) },
	{ NULL, 0 }
};

static classVariableInfo_t idActor_typeInfo[] = {
	{ "int", "team", (intptr_t)(&((idActor *)0)->team), sizeof( ((idActor *)0)->team ) },
	{ "int", "rank", (intptr_t)(&((idActor *)0)->rank), sizeof( ((idActor *)0)->rank ) },
	{ "idMat3", "viewAxis", (intptr_t)(&((idActor *)0)->viewAxis), sizeof( ((idActor *)0)->viewAxis ) },
	{ "idLinkList < idActor >", "enemyNode", (intptr_t)(&((idActor *)0)->enemyNode), sizeof( ((idActor *)0)->enemyNode ) },
	{ "idLinkList < idActor >", "enemyList", (intptr_t)(&((idActor *)0)->enemyList), sizeof( ((idActor *)0)->enemyList ) },
	{ "float", "fovDot", (intptr_t)(&((idActor *)0)->fovDot), sizeof( ((idActor *)0)->fovDot ) },
	{ "idVec3", "eyeOffset", (intptr_t)(&((idActor *)0)->eyeOffset), sizeof( ((idActor *)0)->eyeOffset ) },
	{ "idVec3", "modelOffset", (intptr_t)(&((idActor *)0)->modelOffset), sizeof( ((idActor *)0)->modelOffset ) },
	{ "idAngles", "deltaViewAngles", (intptr_t)(&((idActor *)0)->deltaViewAngles), sizeof( ((idActor *)0)->deltaViewAngles ) },
	{ "int", "pain_debounce_time", (intptr_t)(&((idActor *)0)->pain_debounce_time), sizeof( ((idActor *)0)->pain_debounce_time ) },
	{ "int", "pain_delay", (intptr_t)(&((idActor *)0)->pain_delay), sizeof( ((idActor *)0)->pain_delay ) },
	{ "int", "pain_threshold", (intptr_t)(&((idActor *)0)->pain_threshold), sizeof( ((idActor *)0)->pain_threshold ) },
	{ "idStrList", "damageGroups", (intptr_t)(&((idActor *)0)->damageGroups), sizeof( ((idActor *)0)->damageGroups ) },
	{ "idList < float >", "damageScale", (intptr_t)(&((idActor *)0)->damageScale), sizeof( ((idActor *)0)->damageScale ) },
	{ "bool", "use_combat_bbox", (intptr_t)(&((idActor *)0)->use_combat_bbox), sizeof( ((idActor *)0)->use_combat_bbox ) },
	{ "idEntityPtr < idAFAttachment >", "head", (intptr_t)(&((idActor *)0)->head), sizeof( ((idActor *)0)->head ) },
	{ "idList < copyJoints_t >", "copyJoints", (intptr_t)(&((idActor *)0)->copyJoints), sizeof( ((idActor *)0)->copyJoints ) },
	{ "const function_t *", "state", (intptr_t)(&((idActor *)0)->state), sizeof( ((idActor *)0)->state ) },
	{ "const function_t *", "idealState", (intptr_t)(&((idActor *)0)->idealState), sizeof( ((idActor *)0)->idealState ) },
	{ "jointHandle_t", "leftEyeJoint", (intptr_t)(&((idActor *)0)->leftEyeJoint), sizeof( ((idActor *)0)->leftEyeJoint ) },
	{ "jointHandle_t", "rightEyeJoint", (intptr_t)(&((idActor *)0)->rightEyeJoint), sizeof( ((idActor *)0)->rightEyeJoint ) },
	{ "jointHandle_t", "soundJoint", (intptr_t)(&((idActor *)0)->soundJoint), sizeof( ((idActor *)0)->soundJoint ) },
	{ "idIK_Walk", "walkIK", (intptr_t)(&((idActor *)0)->walkIK), sizeof( ((idActor *)0)->walkIK ) },
	{ "idStr", "animPrefix", (intptr_t)(&((idActor *)0)->animPrefix), sizeof( ((idActor *)0)->animPrefix ) },
	{ "idStr", "painAnim", (intptr_t)(&((idActor *)0)->painAnim), sizeof( ((idActor *)0)->painAnim ) },
	{ "int", "blink_anim", (intptr_t)(&((idActor *)0)->blink_anim), sizeof( ((idActor *)0)->blink_anim ) },
	{ "int", "blink_time", (intptr_t)(&((idActor *)0)->blink_time), sizeof( ((idActor *)0)->blink_time ) },
	{ "int", "blink_min", (intptr_t)(&((idActor *)0)->blink_min), sizeof( ((idActor *)0)->blink_min ) },
	{ "int", "blink_max", (intptr_t)(&((idActor *)0)->blink_max), sizeof( ((idActor *)0)->blink_max ) },
	{ "idThread *", "scriptThread", (intptr_t)(&((idActor *)0)->scriptThread), sizeof( ((idActor *)0)->scriptThread ) },
	{ "idStr", "waitState", (intptr_t)(&((idActor *)0)->waitState), sizeof( ((idActor *)0)->waitState ) },
	{ "idAnimState", "headAnim", (intptr_t)(&((idActor *)0)->headAnim), sizeof( ((idActor *)0)->headAnim ) },
	{ "idAnimState", "torsoAnim", (intptr_t)(&((idActor *)0)->torsoAnim), sizeof( ((idActor *)0)->torsoAnim ) },
	{ "idAnimState", "legsAnim", (intptr_t)(&((idActor *)0)->legsAnim), sizeof( ((idActor *)0)->legsAnim ) },
	{ "bool", "allowPain", (intptr_t)(&((idActor *)0)->allowPain), sizeof( ((idActor *)0)->allowPain ) },
	{ "bool", "allowEyeFocus", (intptr_t)(&((idActor *)0)->allowEyeFocus), sizeof( ((idActor *)0)->allowEyeFocus ) },
	{ "bool", "finalBoss", (intptr_t)(&((idActor *)0)->finalBoss), sizeof( ((idActor *)0)->finalBoss ) },
	{ "int", "painTime", (intptr_t)(&((idActor *)0)->painTime), sizeof( ((idActor *)0)->painTime ) },
	{ "rvStateThread", "stateThread", (intptr_t)(&((idActor *)0)->stateThread), sizeof( ((idActor *)0)->stateThread ) },
	{ "idList < idAttachInfo >", "attachments", (intptr_t)(&((idActor *)0)->attachments), sizeof( ((idActor *)0)->attachments ) },
	{ "int", "damageCap", (intptr_t)(&((idActor *)0)->damageCap), sizeof( ((idActor *)0)->damageCap ) },
	{ NULL, 0 }
};

static classVariableInfo_t idProjectile_projectileFlags_s_typeInfo[] = {
//	{ "bool", "detonate_on_world", (intptr_t)(&((idProjectile::projectileFlags_s *)0)->detonate_on_world), sizeof( ((idProjectile::projectileFlags_s *)0)->detonate_on_world ) },
//	{ "bool", "detonate_on_actor", (intptr_t)(&((idProjectile::projectileFlags_s *)0)->detonate_on_actor), sizeof( ((idProjectile::projectileFlags_s *)0)->detonate_on_actor ) },
//	{ "bool", "randomShaderSpin", (intptr_t)(&((idProjectile::projectileFlags_s *)0)->randomShaderSpin), sizeof( ((idProjectile::projectileFlags_s *)0)->randomShaderSpin ) },
//	{ "bool", "isTracer", (intptr_t)(&((idProjectile::projectileFlags_s *)0)->isTracer), sizeof( ((idProjectile::projectileFlags_s *)0)->isTracer ) },
//	{ "bool", "noSplashDamage", (intptr_t)(&((idProjectile::projectileFlags_s *)0)->noSplashDamage), sizeof( ((idProjectile::projectileFlags_s *)0)->noSplashDamage ) },
	{ NULL, 0 }
};

static classVariableInfo_t idProjectile_typeInfo[] = {
	{ ": idEntityPtr < idEntity >", "owner", (intptr_t)(&((idProjectile *)0)->owner), sizeof( ((idProjectile *)0)->owner ) },
	{ "idProjectile::projectileFlags_s", "projectileFlags", (intptr_t)(&((idProjectile *)0)->projectileFlags), sizeof( ((idProjectile *)0)->projectileFlags ) },
	{ "float", "thrust", (intptr_t)(&((idProjectile *)0)->thrust), sizeof( ((idProjectile *)0)->thrust ) },
	{ "int", "thrust_end", (intptr_t)(&((idProjectile *)0)->thrust_end), sizeof( ((idProjectile *)0)->thrust_end ) },
	{ "float", "damagePower", (intptr_t)(&((idProjectile *)0)->damagePower), sizeof( ((idProjectile *)0)->damagePower ) },
	{ "renderLight_t", "renderLight", (intptr_t)(&((idProjectile *)0)->renderLight), sizeof( ((idProjectile *)0)->renderLight ) },
	{ "qhandle_t", "lightDefHandle", (intptr_t)(&((idProjectile *)0)->lightDefHandle), sizeof( ((idProjectile *)0)->lightDefHandle ) },
	{ "idVec3", "lightOffset", (intptr_t)(&((idProjectile *)0)->lightOffset), sizeof( ((idProjectile *)0)->lightOffset ) },
	{ "int", "lightStartTime", (intptr_t)(&((idProjectile *)0)->lightStartTime), sizeof( ((idProjectile *)0)->lightStartTime ) },
	{ "int", "lightEndTime", (intptr_t)(&((idProjectile *)0)->lightEndTime), sizeof( ((idProjectile *)0)->lightEndTime ) },
	{ "idVec3", "lightColor", (intptr_t)(&((idProjectile *)0)->lightColor), sizeof( ((idProjectile *)0)->lightColor ) },
	{ "idForce_Constant", "thruster", (intptr_t)(&((idProjectile *)0)->thruster), sizeof( ((idProjectile *)0)->thruster ) },
	{ "idPhysics_RigidBody", "physicsObj", (intptr_t)(&((idProjectile *)0)->physicsObj), sizeof( ((idProjectile *)0)->physicsObj ) },
	{ "const idDeclParticle *", "smokeFly", (intptr_t)(&((idProjectile *)0)->smokeFly), sizeof( ((idProjectile *)0)->smokeFly ) },
	{ "int", "smokeFlyTime", (intptr_t)(&((idProjectile *)0)->smokeFlyTime), sizeof( ((idProjectile *)0)->smokeFlyTime ) },
	{ "int", "originalTimeGroup", (intptr_t)(&((idProjectile *)0)->originalTimeGroup), sizeof( ((idProjectile *)0)->originalTimeGroup ) },
	{ "projectileState_t", "state", (intptr_t)(&((idProjectile *)0)->state), sizeof( ((idProjectile *)0)->state ) },
	{ ": bool", "netSyncPhysics", (intptr_t)(&((idProjectile *)0)->netSyncPhysics), sizeof( ((idProjectile *)0)->netSyncPhysics ) },
	{ NULL, 0 }
};

static classVariableInfo_t idGuidedProjectile_typeInfo[] = {
	{ ": float", "speed", (intptr_t)(&((idGuidedProjectile *)0)->speed), sizeof( ((idGuidedProjectile *)0)->speed ) },
	{ "idEntityPtr < idEntity >", "enemy", (intptr_t)(&((idGuidedProjectile *)0)->enemy), sizeof( ((idGuidedProjectile *)0)->enemy ) },
	{ ": idAngles", "rndScale", (intptr_t)(&((idGuidedProjectile *)0)->rndScale), sizeof( ((idGuidedProjectile *)0)->rndScale ) },
	{ "idAngles", "rndAng", (intptr_t)(&((idGuidedProjectile *)0)->rndAng), sizeof( ((idGuidedProjectile *)0)->rndAng ) },
	{ "idAngles", "angles", (intptr_t)(&((idGuidedProjectile *)0)->angles), sizeof( ((idGuidedProjectile *)0)->angles ) },
	{ "int", "rndUpdateTime", (intptr_t)(&((idGuidedProjectile *)0)->rndUpdateTime), sizeof( ((idGuidedProjectile *)0)->rndUpdateTime ) },
	{ "float", "turn_max", (intptr_t)(&((idGuidedProjectile *)0)->turn_max), sizeof( ((idGuidedProjectile *)0)->turn_max ) },
	{ "float", "clamp_dist", (intptr_t)(&((idGuidedProjectile *)0)->clamp_dist), sizeof( ((idGuidedProjectile *)0)->clamp_dist ) },
	{ "bool", "burstMode", (intptr_t)(&((idGuidedProjectile *)0)->burstMode), sizeof( ((idGuidedProjectile *)0)->burstMode ) },
	{ "bool", "unGuided", (intptr_t)(&((idGuidedProjectile *)0)->unGuided), sizeof( ((idGuidedProjectile *)0)->unGuided ) },
	{ "float", "burstDist", (intptr_t)(&((idGuidedProjectile *)0)->burstDist), sizeof( ((idGuidedProjectile *)0)->burstDist ) },
	{ "float", "burstVelocity", (intptr_t)(&((idGuidedProjectile *)0)->burstVelocity), sizeof( ((idGuidedProjectile *)0)->burstVelocity ) },
	{ NULL, 0 }
};

static classVariableInfo_t idSoulCubeMissile_typeInfo[] = {
	{ ": idVec3", "startingVelocity", (intptr_t)(&((idSoulCubeMissile *)0)->startingVelocity), sizeof( ((idSoulCubeMissile *)0)->startingVelocity ) },
	{ "idVec3", "endingVelocity", (intptr_t)(&((idSoulCubeMissile *)0)->endingVelocity), sizeof( ((idSoulCubeMissile *)0)->endingVelocity ) },
	{ "float", "accelTime", (intptr_t)(&((idSoulCubeMissile *)0)->accelTime), sizeof( ((idSoulCubeMissile *)0)->accelTime ) },
	{ "int", "launchTime", (intptr_t)(&((idSoulCubeMissile *)0)->launchTime), sizeof( ((idSoulCubeMissile *)0)->launchTime ) },
	{ "bool", "killPhase", (intptr_t)(&((idSoulCubeMissile *)0)->killPhase), sizeof( ((idSoulCubeMissile *)0)->killPhase ) },
	{ "bool", "returnPhase", (intptr_t)(&((idSoulCubeMissile *)0)->returnPhase), sizeof( ((idSoulCubeMissile *)0)->returnPhase ) },
	{ "idVec3", "destOrg", (intptr_t)(&((idSoulCubeMissile *)0)->destOrg), sizeof( ((idSoulCubeMissile *)0)->destOrg ) },
	{ "idVec3", "orbitOrg", (intptr_t)(&((idSoulCubeMissile *)0)->orbitOrg), sizeof( ((idSoulCubeMissile *)0)->orbitOrg ) },
	{ "int", "orbitTime", (intptr_t)(&((idSoulCubeMissile *)0)->orbitTime), sizeof( ((idSoulCubeMissile *)0)->orbitTime ) },
	{ "int", "smokeKillTime", (intptr_t)(&((idSoulCubeMissile *)0)->smokeKillTime), sizeof( ((idSoulCubeMissile *)0)->smokeKillTime ) },
	{ "const idDeclParticle *", "smokeKill", (intptr_t)(&((idSoulCubeMissile *)0)->smokeKill), sizeof( ((idSoulCubeMissile *)0)->smokeKill ) },
	{ NULL, 0 }
};

static classVariableInfo_t beamTarget_t_typeInfo[] = {
	{ "idEntityPtr < idEntity >", "target", (intptr_t)(&((beamTarget_t *)0)->target), sizeof( ((beamTarget_t *)0)->target ) },
	{ "renderEntity_t", "renderEntity", (intptr_t)(&((beamTarget_t *)0)->renderEntity), sizeof( ((beamTarget_t *)0)->renderEntity ) },
	{ "qhandle_t", "modelDefHandle", (intptr_t)(&((beamTarget_t *)0)->modelDefHandle), sizeof( ((beamTarget_t *)0)->modelDefHandle ) },
	{ NULL, 0 }
};

static classVariableInfo_t idBFGProjectile_typeInfo[] = {
	{ ": idList < beamTarget_t >", "beamTargets", (intptr_t)(&((idBFGProjectile *)0)->beamTargets), sizeof( ((idBFGProjectile *)0)->beamTargets ) },
	{ "renderEntity_t", "secondModel", (intptr_t)(&((idBFGProjectile *)0)->secondModel), sizeof( ((idBFGProjectile *)0)->secondModel ) },
	{ "qhandle_t", "secondModelDefHandle", (intptr_t)(&((idBFGProjectile *)0)->secondModelDefHandle), sizeof( ((idBFGProjectile *)0)->secondModelDefHandle ) },
	{ "int", "nextDamageTime", (intptr_t)(&((idBFGProjectile *)0)->nextDamageTime), sizeof( ((idBFGProjectile *)0)->nextDamageTime ) },
	{ "idStr", "damageFreq", (intptr_t)(&((idBFGProjectile *)0)->damageFreq), sizeof( ((idBFGProjectile *)0)->damageFreq ) },
	{ NULL, 0 }
};

static classVariableInfo_t idHomingProjectile_typeInfo[] = {
	{ ": float", "speed", (intptr_t)(&((idHomingProjectile *)0)->speed), sizeof( ((idHomingProjectile *)0)->speed ) },
	{ "idEntityPtr < idEntity >", "enemy", (intptr_t)(&((idHomingProjectile *)0)->enemy), sizeof( ((idHomingProjectile *)0)->enemy ) },
	{ "idVec3", "seekPos", (intptr_t)(&((idHomingProjectile *)0)->seekPos), sizeof( ((idHomingProjectile *)0)->seekPos ) },
	{ ": idAngles", "rndScale", (intptr_t)(&((idHomingProjectile *)0)->rndScale), sizeof( ((idHomingProjectile *)0)->rndScale ) },
	{ "idAngles", "rndAng", (intptr_t)(&((idHomingProjectile *)0)->rndAng), sizeof( ((idHomingProjectile *)0)->rndAng ) },
	{ "idAngles", "angles", (intptr_t)(&((idHomingProjectile *)0)->angles), sizeof( ((idHomingProjectile *)0)->angles ) },
	{ "float", "turn_max", (intptr_t)(&((idHomingProjectile *)0)->turn_max), sizeof( ((idHomingProjectile *)0)->turn_max ) },
	{ "float", "clamp_dist", (intptr_t)(&((idHomingProjectile *)0)->clamp_dist), sizeof( ((idHomingProjectile *)0)->clamp_dist ) },
	{ "bool", "burstMode", (intptr_t)(&((idHomingProjectile *)0)->burstMode), sizeof( ((idHomingProjectile *)0)->burstMode ) },
	{ "bool", "unGuided", (intptr_t)(&((idHomingProjectile *)0)->unGuided), sizeof( ((idHomingProjectile *)0)->unGuided ) },
	{ "float", "burstDist", (intptr_t)(&((idHomingProjectile *)0)->burstDist), sizeof( ((idHomingProjectile *)0)->burstDist ) },
	{ "float", "burstVelocity", (intptr_t)(&((idHomingProjectile *)0)->burstVelocity), sizeof( ((idHomingProjectile *)0)->burstVelocity ) },
	{ NULL, 0 }
};

static classVariableInfo_t idDebris_typeInfo[] = {
	{ ": idEntityPtr < idEntity >", "owner", (intptr_t)(&((idDebris *)0)->owner), sizeof( ((idDebris *)0)->owner ) },
	{ "idPhysics_RigidBody", "physicsObj", (intptr_t)(&((idDebris *)0)->physicsObj), sizeof( ((idDebris *)0)->physicsObj ) },
	{ "const idDeclParticle *", "smokeFly", (intptr_t)(&((idDebris *)0)->smokeFly), sizeof( ((idDebris *)0)->smokeFly ) },
	{ "int", "smokeFlyTime", (intptr_t)(&((idDebris *)0)->smokeFlyTime), sizeof( ((idDebris *)0)->smokeFlyTime ) },
	{ "const idSoundShader *", "sndBounce", (intptr_t)(&((idDebris *)0)->sndBounce), sizeof( ((idDebris *)0)->sndBounce ) },
	{ "idRenderModel *", "dynamicModel", (intptr_t)(&((idDebris *)0)->dynamicModel), sizeof( ((idDebris *)0)->dynamicModel ) },
	{ NULL, 0 }
};

static classVariableInfo_t WeaponParticle_t_typeInfo[] = {
	{ "char[64]", "name", (intptr_t)(&((WeaponParticle_t *)0)->name), sizeof( ((WeaponParticle_t *)0)->name ) },
	{ "char[128]", "particlename", (intptr_t)(&((WeaponParticle_t *)0)->particlename), sizeof( ((WeaponParticle_t *)0)->particlename ) },
	{ "bool", "active", (intptr_t)(&((WeaponParticle_t *)0)->active), sizeof( ((WeaponParticle_t *)0)->active ) },
	{ "int", "startTime", (intptr_t)(&((WeaponParticle_t *)0)->startTime), sizeof( ((WeaponParticle_t *)0)->startTime ) },
	{ "jointHandle_t", "joint", (intptr_t)(&((WeaponParticle_t *)0)->joint), sizeof( ((WeaponParticle_t *)0)->joint ) },
	{ "bool", "smoke", (intptr_t)(&((WeaponParticle_t *)0)->smoke), sizeof( ((WeaponParticle_t *)0)->smoke ) },
	{ "const idDeclParticle *", "particle", (intptr_t)(&((WeaponParticle_t *)0)->particle), sizeof( ((WeaponParticle_t *)0)->particle ) },
	{ "idFuncEmitter *", "emitter", (intptr_t)(&((WeaponParticle_t *)0)->emitter), sizeof( ((WeaponParticle_t *)0)->emitter ) },
	{ NULL, 0 }
};

static classVariableInfo_t WeaponLight_t_typeInfo[] = {
	{ "char[64]", "name", (intptr_t)(&((WeaponLight_t *)0)->name), sizeof( ((WeaponLight_t *)0)->name ) },
	{ "bool", "active", (intptr_t)(&((WeaponLight_t *)0)->active), sizeof( ((WeaponLight_t *)0)->active ) },
	{ "int", "startTime", (intptr_t)(&((WeaponLight_t *)0)->startTime), sizeof( ((WeaponLight_t *)0)->startTime ) },
	{ "jointHandle_t", "joint", (intptr_t)(&((WeaponLight_t *)0)->joint), sizeof( ((WeaponLight_t *)0)->joint ) },
	{ "int", "lightHandle", (intptr_t)(&((WeaponLight_t *)0)->lightHandle), sizeof( ((WeaponLight_t *)0)->lightHandle ) },
	{ "renderLight_t", "light", (intptr_t)(&((WeaponLight_t *)0)->light), sizeof( ((WeaponLight_t *)0)->light ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvmWeaponObject_typeInfo[] = {
	{ ": idWeapon *", "owner", (intptr_t)(&((rvmWeaponObject *)0)->owner), sizeof( ((rvmWeaponObject *)0)->owner ) },
	{ ": rvStateThread", "stateThread", (intptr_t)(&((rvmWeaponObject *)0)->stateThread), sizeof( ((rvmWeaponObject *)0)->stateThread ) },
	{ "float", "next_attack", (intptr_t)(&((rvmWeaponObject *)0)->next_attack), sizeof( ((rvmWeaponObject *)0)->next_attack ) },
	{ NULL, 0 }
};

static classVariableInfo_t idWeapon_typeInfo[] = {
	{ ": idVec3", "gun_decl_offset", (intptr_t)(&((idWeapon *)0)->gun_decl_offset), sizeof( ((idWeapon *)0)->gun_decl_offset ) },
	{ "idVec3", "gun_decl_flashoffset", (intptr_t)(&((idWeapon *)0)->gun_decl_flashoffset), sizeof( ((idWeapon *)0)->gun_decl_flashoffset ) },
	{ "idStr", "fx_muzzleflash", (intptr_t)(&((idWeapon *)0)->fx_muzzleflash), sizeof( ((idWeapon *)0)->fx_muzzleflash ) },
	{ "idEntityPtr < idEntityFx >[2]", "muzzleFireFX", (intptr_t)(&((idWeapon *)0)->muzzleFireFX), sizeof( ((idWeapon *)0)->muzzleFireFX ) },
	{ "int", "animBlendFrames", (intptr_t)(&((idWeapon *)0)->animBlendFrames), sizeof( ((idWeapon *)0)->animBlendFrames ) },
	{ "int", "animDoneTime", (intptr_t)(&((idWeapon *)0)->animDoneTime), sizeof( ((idWeapon *)0)->animDoneTime ) },
	{ "bool", "isPlayerFlashlight", (intptr_t)(&((idWeapon *)0)->isPlayerFlashlight), sizeof( ((idWeapon *)0)->isPlayerFlashlight ) },
	{ "bool", "isLinked", (intptr_t)(&((idWeapon *)0)->isLinked), sizeof( ((idWeapon *)0)->isLinked ) },
	{ "int", "vertexAnimatedFrameStart", (intptr_t)(&((idWeapon *)0)->vertexAnimatedFrameStart), sizeof( ((idWeapon *)0)->vertexAnimatedFrameStart ) },
	{ "int", "vertexAnimatedNumFrames", (intptr_t)(&((idWeapon *)0)->vertexAnimatedNumFrames), sizeof( ((idWeapon *)0)->vertexAnimatedNumFrames ) },
	{ "bool", "vertrexAnimatedRepeat", (intptr_t)(&((idWeapon *)0)->vertrexAnimatedRepeat), sizeof( ((idWeapon *)0)->vertrexAnimatedRepeat ) },
	{ "idEntity *", "projectileEnt", (intptr_t)(&((idWeapon *)0)->projectileEnt), sizeof( ((idWeapon *)0)->projectileEnt ) },
	{ "idPlayer *", "owner", (intptr_t)(&((idWeapon *)0)->owner), sizeof( ((idWeapon *)0)->owner ) },
	{ "idEntityPtr < idAnimatedEntity >", "worldModel", (intptr_t)(&((idWeapon *)0)->worldModel), sizeof( ((idWeapon *)0)->worldModel ) },
	{ "int", "hideTime", (intptr_t)(&((idWeapon *)0)->hideTime), sizeof( ((idWeapon *)0)->hideTime ) },
	{ "float", "hideDistance", (intptr_t)(&((idWeapon *)0)->hideDistance), sizeof( ((idWeapon *)0)->hideDistance ) },
	{ "int", "hideStartTime", (intptr_t)(&((idWeapon *)0)->hideStartTime), sizeof( ((idWeapon *)0)->hideStartTime ) },
	{ "float", "hideStart", (intptr_t)(&((idWeapon *)0)->hideStart), sizeof( ((idWeapon *)0)->hideStart ) },
	{ "float", "hideEnd", (intptr_t)(&((idWeapon *)0)->hideEnd), sizeof( ((idWeapon *)0)->hideEnd ) },
	{ "float", "hideOffset", (intptr_t)(&((idWeapon *)0)->hideOffset), sizeof( ((idWeapon *)0)->hideOffset ) },
	{ "bool", "hide", (intptr_t)(&((idWeapon *)0)->hide), sizeof( ((idWeapon *)0)->hide ) },
	{ "bool", "disabled", (intptr_t)(&((idWeapon *)0)->disabled), sizeof( ((idWeapon *)0)->disabled ) },
	{ "bool", "isFlashLight", (intptr_t)(&((idWeapon *)0)->isFlashLight), sizeof( ((idWeapon *)0)->isFlashLight ) },
	{ "int", "berserk", (intptr_t)(&((idWeapon *)0)->berserk), sizeof( ((idWeapon *)0)->berserk ) },
	{ "idVec3", "playerViewOrigin", (intptr_t)(&((idWeapon *)0)->playerViewOrigin), sizeof( ((idWeapon *)0)->playerViewOrigin ) },
	{ "idMat3", "playerViewAxis", (intptr_t)(&((idWeapon *)0)->playerViewAxis), sizeof( ((idWeapon *)0)->playerViewAxis ) },
	{ "idVec3", "viewWeaponOrigin", (intptr_t)(&((idWeapon *)0)->viewWeaponOrigin), sizeof( ((idWeapon *)0)->viewWeaponOrigin ) },
	{ "idMat3", "viewWeaponAxis", (intptr_t)(&((idWeapon *)0)->viewWeaponAxis), sizeof( ((idWeapon *)0)->viewWeaponAxis ) },
	{ "idVec3", "muzzleOrigin", (intptr_t)(&((idWeapon *)0)->muzzleOrigin), sizeof( ((idWeapon *)0)->muzzleOrigin ) },
	{ "idMat3", "muzzleAxis", (intptr_t)(&((idWeapon *)0)->muzzleAxis), sizeof( ((idWeapon *)0)->muzzleAxis ) },
	{ "idVec3", "pushVelocity", (intptr_t)(&((idWeapon *)0)->pushVelocity), sizeof( ((idWeapon *)0)->pushVelocity ) },
	{ "const idDeclEntityDef *", "weaponDef", (intptr_t)(&((idWeapon *)0)->weaponDef), sizeof( ((idWeapon *)0)->weaponDef ) },
	{ "const idDeclEntityDef *", "meleeDef", (intptr_t)(&((idWeapon *)0)->meleeDef), sizeof( ((idWeapon *)0)->meleeDef ) },
	{ "idDict", "projectileDict", (intptr_t)(&((idWeapon *)0)->projectileDict), sizeof( ((idWeapon *)0)->projectileDict ) },
	{ "float", "meleeDistance", (intptr_t)(&((idWeapon *)0)->meleeDistance), sizeof( ((idWeapon *)0)->meleeDistance ) },
	{ "idStr", "meleeDefName", (intptr_t)(&((idWeapon *)0)->meleeDefName), sizeof( ((idWeapon *)0)->meleeDefName ) },
	{ "idDict", "brassDict", (intptr_t)(&((idWeapon *)0)->brassDict), sizeof( ((idWeapon *)0)->brassDict ) },
	{ "int", "brassDelay", (intptr_t)(&((idWeapon *)0)->brassDelay), sizeof( ((idWeapon *)0)->brassDelay ) },
	{ "idStr", "icon", (intptr_t)(&((idWeapon *)0)->icon), sizeof( ((idWeapon *)0)->icon ) },
	{ "idStr", "pdaIcon", (intptr_t)(&((idWeapon *)0)->pdaIcon), sizeof( ((idWeapon *)0)->pdaIcon ) },
	{ "idStr", "displayName", (intptr_t)(&((idWeapon *)0)->displayName), sizeof( ((idWeapon *)0)->displayName ) },
	{ "idStr", "itemDesc", (intptr_t)(&((idWeapon *)0)->itemDesc), sizeof( ((idWeapon *)0)->itemDesc ) },
	{ "renderLight_t", "guiLight", (intptr_t)(&((idWeapon *)0)->guiLight), sizeof( ((idWeapon *)0)->guiLight ) },
	{ "int", "guiLightHandle", (intptr_t)(&((idWeapon *)0)->guiLightHandle), sizeof( ((idWeapon *)0)->guiLightHandle ) },
	{ "renderLight_t", "muzzleFlash", (intptr_t)(&((idWeapon *)0)->muzzleFlash), sizeof( ((idWeapon *)0)->muzzleFlash ) },
	{ "int", "muzzleFlashHandle", (intptr_t)(&((idWeapon *)0)->muzzleFlashHandle), sizeof( ((idWeapon *)0)->muzzleFlashHandle ) },
	{ "renderLight_t", "worldMuzzleFlash", (intptr_t)(&((idWeapon *)0)->worldMuzzleFlash), sizeof( ((idWeapon *)0)->worldMuzzleFlash ) },
	{ "int", "worldMuzzleFlashHandle", (intptr_t)(&((idWeapon *)0)->worldMuzzleFlashHandle), sizeof( ((idWeapon *)0)->worldMuzzleFlashHandle ) },
	{ "float", "fraccos", (intptr_t)(&((idWeapon *)0)->fraccos), sizeof( ((idWeapon *)0)->fraccos ) },
	{ "float", "fraccos2", (intptr_t)(&((idWeapon *)0)->fraccos2), sizeof( ((idWeapon *)0)->fraccos2 ) },
	{ "idVec3", "flashColor", (intptr_t)(&((idWeapon *)0)->flashColor), sizeof( ((idWeapon *)0)->flashColor ) },
	{ "int", "muzzleFlashEnd", (intptr_t)(&((idWeapon *)0)->muzzleFlashEnd), sizeof( ((idWeapon *)0)->muzzleFlashEnd ) },
	{ "int", "flashTime", (intptr_t)(&((idWeapon *)0)->flashTime), sizeof( ((idWeapon *)0)->flashTime ) },
	{ "bool", "lightOn", (intptr_t)(&((idWeapon *)0)->lightOn), sizeof( ((idWeapon *)0)->lightOn ) },
	{ "bool", "silent_fire", (intptr_t)(&((idWeapon *)0)->silent_fire), sizeof( ((idWeapon *)0)->silent_fire ) },
	{ "bool", "allowDrop", (intptr_t)(&((idWeapon *)0)->allowDrop), sizeof( ((idWeapon *)0)->allowDrop ) },
	{ "bool", "hasBloodSplat", (intptr_t)(&((idWeapon *)0)->hasBloodSplat), sizeof( ((idWeapon *)0)->hasBloodSplat ) },
	{ "int", "kick_endtime", (intptr_t)(&((idWeapon *)0)->kick_endtime), sizeof( ((idWeapon *)0)->kick_endtime ) },
	{ "int", "muzzle_kick_time", (intptr_t)(&((idWeapon *)0)->muzzle_kick_time), sizeof( ((idWeapon *)0)->muzzle_kick_time ) },
	{ "int", "muzzle_kick_maxtime", (intptr_t)(&((idWeapon *)0)->muzzle_kick_maxtime), sizeof( ((idWeapon *)0)->muzzle_kick_maxtime ) },
	{ "idAngles", "muzzle_kick_angles", (intptr_t)(&((idWeapon *)0)->muzzle_kick_angles), sizeof( ((idWeapon *)0)->muzzle_kick_angles ) },
	{ "idVec3", "muzzle_kick_offset", (intptr_t)(&((idWeapon *)0)->muzzle_kick_offset), sizeof( ((idWeapon *)0)->muzzle_kick_offset ) },
	{ "ammo_t", "ammoType", (intptr_t)(&((idWeapon *)0)->ammoType), sizeof( ((idWeapon *)0)->ammoType ) },
	{ "int", "ammoRequired", (intptr_t)(&((idWeapon *)0)->ammoRequired), sizeof( ((idWeapon *)0)->ammoRequired ) },
	{ "int", "clipSize", (intptr_t)(&((idWeapon *)0)->clipSize), sizeof( ((idWeapon *)0)->clipSize ) },
	{ "idPredictedValue < int >", "ammoClip", (intptr_t)(&((idWeapon *)0)->ammoClip), sizeof( ((idWeapon *)0)->ammoClip ) },
	{ "int", "lowAmmo", (intptr_t)(&((idWeapon *)0)->lowAmmo), sizeof( ((idWeapon *)0)->lowAmmo ) },
	{ "bool", "powerAmmo", (intptr_t)(&((idWeapon *)0)->powerAmmo), sizeof( ((idWeapon *)0)->powerAmmo ) },
	{ "bool", "isFiring", (intptr_t)(&((idWeapon *)0)->isFiring), sizeof( ((idWeapon *)0)->isFiring ) },
	{ "int", "zoomFov", (intptr_t)(&((idWeapon *)0)->zoomFov), sizeof( ((idWeapon *)0)->zoomFov ) },
	{ "jointHandle_t", "barrelJointView", (intptr_t)(&((idWeapon *)0)->barrelJointView), sizeof( ((idWeapon *)0)->barrelJointView ) },
	{ "jointHandle_t", "flashJointView", (intptr_t)(&((idWeapon *)0)->flashJointView), sizeof( ((idWeapon *)0)->flashJointView ) },
	{ "jointHandle_t", "ejectJointView", (intptr_t)(&((idWeapon *)0)->ejectJointView), sizeof( ((idWeapon *)0)->ejectJointView ) },
	{ "jointHandle_t", "guiLightJointView", (intptr_t)(&((idWeapon *)0)->guiLightJointView), sizeof( ((idWeapon *)0)->guiLightJointView ) },
	{ "jointHandle_t", "ventLightJointView", (intptr_t)(&((idWeapon *)0)->ventLightJointView), sizeof( ((idWeapon *)0)->ventLightJointView ) },
	{ "jointHandle_t", "flashJointWorld", (intptr_t)(&((idWeapon *)0)->flashJointWorld), sizeof( ((idWeapon *)0)->flashJointWorld ) },
	{ "jointHandle_t", "barrelJointWorld", (intptr_t)(&((idWeapon *)0)->barrelJointWorld), sizeof( ((idWeapon *)0)->barrelJointWorld ) },
	{ "jointHandle_t", "ejectJointWorld", (intptr_t)(&((idWeapon *)0)->ejectJointWorld), sizeof( ((idWeapon *)0)->ejectJointWorld ) },
	{ "jointHandle_t", "smokeJointView", (intptr_t)(&((idWeapon *)0)->smokeJointView), sizeof( ((idWeapon *)0)->smokeJointView ) },
	{ "idHashTable < WeaponParticle_t >", "weaponParticles", (intptr_t)(&((idWeapon *)0)->weaponParticles), sizeof( ((idWeapon *)0)->weaponParticles ) },
	{ "idHashTable < WeaponLight_t >", "weaponLights", (intptr_t)(&((idWeapon *)0)->weaponLights), sizeof( ((idWeapon *)0)->weaponLights ) },
	{ "const idSoundShader *", "sndHum", (intptr_t)(&((idWeapon *)0)->sndHum), sizeof( ((idWeapon *)0)->sndHum ) },
	{ "const idDeclParticle *", "weaponSmoke", (intptr_t)(&((idWeapon *)0)->weaponSmoke), sizeof( ((idWeapon *)0)->weaponSmoke ) },
	{ "int", "weaponSmokeStartTime", (intptr_t)(&((idWeapon *)0)->weaponSmokeStartTime), sizeof( ((idWeapon *)0)->weaponSmokeStartTime ) },
	{ "bool", "continuousSmoke", (intptr_t)(&((idWeapon *)0)->continuousSmoke), sizeof( ((idWeapon *)0)->continuousSmoke ) },
	{ "const idDeclParticle *", "strikeSmoke", (intptr_t)(&((idWeapon *)0)->strikeSmoke), sizeof( ((idWeapon *)0)->strikeSmoke ) },
	{ "int", "strikeSmokeStartTime", (intptr_t)(&((idWeapon *)0)->strikeSmokeStartTime), sizeof( ((idWeapon *)0)->strikeSmokeStartTime ) },
	{ "idVec3", "strikePos", (intptr_t)(&((idWeapon *)0)->strikePos), sizeof( ((idWeapon *)0)->strikePos ) },
	{ "idMat3", "strikeAxis", (intptr_t)(&((idWeapon *)0)->strikeAxis), sizeof( ((idWeapon *)0)->strikeAxis ) },
	{ "int", "nextStrikeFx", (intptr_t)(&((idWeapon *)0)->nextStrikeFx), sizeof( ((idWeapon *)0)->nextStrikeFx ) },
	{ "bool", "nozzleFx", (intptr_t)(&((idWeapon *)0)->nozzleFx), sizeof( ((idWeapon *)0)->nozzleFx ) },
	{ "int", "nozzleFxFade", (intptr_t)(&((idWeapon *)0)->nozzleFxFade), sizeof( ((idWeapon *)0)->nozzleFxFade ) },
	{ "int", "lastAttack", (intptr_t)(&((idWeapon *)0)->lastAttack), sizeof( ((idWeapon *)0)->lastAttack ) },
	{ "renderLight_t", "nozzleGlow", (intptr_t)(&((idWeapon *)0)->nozzleGlow), sizeof( ((idWeapon *)0)->nozzleGlow ) },
	{ "int", "nozzleGlowHandle", (intptr_t)(&((idWeapon *)0)->nozzleGlowHandle), sizeof( ((idWeapon *)0)->nozzleGlowHandle ) },
	{ "idVec3", "nozzleGlowColor", (intptr_t)(&((idWeapon *)0)->nozzleGlowColor), sizeof( ((idWeapon *)0)->nozzleGlowColor ) },
	{ "const idMaterial *", "nozzleGlowShader", (intptr_t)(&((idWeapon *)0)->nozzleGlowShader), sizeof( ((idWeapon *)0)->nozzleGlowShader ) },
	{ "float", "nozzleGlowRadius", (intptr_t)(&((idWeapon *)0)->nozzleGlowRadius), sizeof( ((idWeapon *)0)->nozzleGlowRadius ) },
	{ "int", "weaponAngleOffsetAverages", (intptr_t)(&((idWeapon *)0)->weaponAngleOffsetAverages), sizeof( ((idWeapon *)0)->weaponAngleOffsetAverages ) },
	{ "float", "weaponAngleOffsetScale", (intptr_t)(&((idWeapon *)0)->weaponAngleOffsetScale), sizeof( ((idWeapon *)0)->weaponAngleOffsetScale ) },
	{ "float", "weaponAngleOffsetMax", (intptr_t)(&((idWeapon *)0)->weaponAngleOffsetMax), sizeof( ((idWeapon *)0)->weaponAngleOffsetMax ) },
	{ "float", "weaponOffsetTime", (intptr_t)(&((idWeapon *)0)->weaponOffsetTime), sizeof( ((idWeapon *)0)->weaponOffsetTime ) },
	{ "float", "weaponOffsetScale", (intptr_t)(&((idWeapon *)0)->weaponOffsetScale), sizeof( ((idWeapon *)0)->weaponOffsetScale ) },
	{ ": rvmWeaponObject *", "currentWeaponObject", (intptr_t)(&((idWeapon *)0)->currentWeaponObject), sizeof( ((idWeapon *)0)->currentWeaponObject ) },
	{ "bool", "OutOfAmmo", (intptr_t)(&((idWeapon *)0)->OutOfAmmo), sizeof( ((idWeapon *)0)->OutOfAmmo ) },
	{ NULL, 0 }
};

static classVariableInfo_t idLight_typeInfo[] = {
	{ ": renderLight_t", "renderLight", (intptr_t)(&((idLight *)0)->renderLight), sizeof( ((idLight *)0)->renderLight ) },
	{ "idVec3", "localLightOrigin", (intptr_t)(&((idLight *)0)->localLightOrigin), sizeof( ((idLight *)0)->localLightOrigin ) },
	{ "idMat3", "localLightAxis", (intptr_t)(&((idLight *)0)->localLightAxis), sizeof( ((idLight *)0)->localLightAxis ) },
	{ "qhandle_t", "lightDefHandle", (intptr_t)(&((idLight *)0)->lightDefHandle), sizeof( ((idLight *)0)->lightDefHandle ) },
	{ "idStr", "brokenModel", (intptr_t)(&((idLight *)0)->brokenModel), sizeof( ((idLight *)0)->brokenModel ) },
	{ "int", "levels", (intptr_t)(&((idLight *)0)->levels), sizeof( ((idLight *)0)->levels ) },
	{ "int", "currentLevel", (intptr_t)(&((idLight *)0)->currentLevel), sizeof( ((idLight *)0)->currentLevel ) },
	{ "idVec3", "baseColor", (intptr_t)(&((idLight *)0)->baseColor), sizeof( ((idLight *)0)->baseColor ) },
	{ "bool", "breakOnTrigger", (intptr_t)(&((idLight *)0)->breakOnTrigger), sizeof( ((idLight *)0)->breakOnTrigger ) },
	{ "int", "count", (intptr_t)(&((idLight *)0)->count), sizeof( ((idLight *)0)->count ) },
	{ "int", "triggercount", (intptr_t)(&((idLight *)0)->triggercount), sizeof( ((idLight *)0)->triggercount ) },
	{ "idEntity *", "lightParent", (intptr_t)(&((idLight *)0)->lightParent), sizeof( ((idLight *)0)->lightParent ) },
	{ "idVec4", "fadeFrom", (intptr_t)(&((idLight *)0)->fadeFrom), sizeof( ((idLight *)0)->fadeFrom ) },
	{ "idVec4", "fadeTo", (intptr_t)(&((idLight *)0)->fadeTo), sizeof( ((idLight *)0)->fadeTo ) },
	{ "int", "fadeStart", (intptr_t)(&((idLight *)0)->fadeStart), sizeof( ((idLight *)0)->fadeStart ) },
	{ "int", "fadeEnd", (intptr_t)(&((idLight *)0)->fadeEnd), sizeof( ((idLight *)0)->fadeEnd ) },
	{ "bool", "soundWasPlaying", (intptr_t)(&((idLight *)0)->soundWasPlaying), sizeof( ((idLight *)0)->soundWasPlaying ) },
	{ NULL, 0 }
};

static classVariableInfo_t idWorldspawn_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idItem_typeInfo[] = {
	{ ": idVec3", "orgOrigin", (intptr_t)(&((idItem *)0)->orgOrigin), sizeof( ((idItem *)0)->orgOrigin ) },
	{ "bool", "spin", (intptr_t)(&((idItem *)0)->spin), sizeof( ((idItem *)0)->spin ) },
	{ "bool", "pulse", (intptr_t)(&((idItem *)0)->pulse), sizeof( ((idItem *)0)->pulse ) },
	{ "bool", "canPickUp", (intptr_t)(&((idItem *)0)->canPickUp), sizeof( ((idItem *)0)->canPickUp ) },
	{ "int", "itemShellHandle", (intptr_t)(&((idItem *)0)->itemShellHandle), sizeof( ((idItem *)0)->itemShellHandle ) },
	{ "const idMaterial *", "shellMaterial", (intptr_t)(&((idItem *)0)->shellMaterial), sizeof( ((idItem *)0)->shellMaterial ) },
	{ "mutable bool", "inView", (intptr_t)(&((idItem *)0)->inView), sizeof( ((idItem *)0)->inView ) },
	{ "mutable int", "inViewTime", (intptr_t)(&((idItem *)0)->inViewTime), sizeof( ((idItem *)0)->inViewTime ) },
	{ "mutable int", "lastCycle", (intptr_t)(&((idItem *)0)->lastCycle), sizeof( ((idItem *)0)->lastCycle ) },
	{ "mutable int", "lastRenderViewTime", (intptr_t)(&((idItem *)0)->lastRenderViewTime), sizeof( ((idItem *)0)->lastRenderViewTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idItemPowerup_typeInfo[] = {
	{ ": int", "time", (intptr_t)(&((idItemPowerup *)0)->time), sizeof( ((idItemPowerup *)0)->time ) },
	{ "int", "type", (intptr_t)(&((idItemPowerup *)0)->type), sizeof( ((idItemPowerup *)0)->type ) },
	{ NULL, 0 }
};

static classVariableInfo_t idObjective_typeInfo[] = {
	{ ": idVec3", "playerPos", (intptr_t)(&((idObjective *)0)->playerPos), sizeof( ((idObjective *)0)->playerPos ) },
	{ NULL, 0 }
};

static classVariableInfo_t idVideoCDItem_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idPDAItem_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idMoveableItem_typeInfo[] = {
	{ ": idPhysics_RigidBody", "physicsObj", (intptr_t)(&((idMoveableItem *)0)->physicsObj), sizeof( ((idMoveableItem *)0)->physicsObj ) },
	{ "idClipModel *", "trigger", (intptr_t)(&((idMoveableItem *)0)->trigger), sizeof( ((idMoveableItem *)0)->trigger ) },
	{ "const idDeclParticle *", "smoke", (intptr_t)(&((idMoveableItem *)0)->smoke), sizeof( ((idMoveableItem *)0)->smoke ) },
	{ "int", "smokeTime", (intptr_t)(&((idMoveableItem *)0)->smokeTime), sizeof( ((idMoveableItem *)0)->smokeTime ) },
	{ "int", "nextSoundTime", (intptr_t)(&((idMoveableItem *)0)->nextSoundTime), sizeof( ((idMoveableItem *)0)->nextSoundTime ) },
	{ "bool", "repeatSmoke", (intptr_t)(&((idMoveableItem *)0)->repeatSmoke), sizeof( ((idMoveableItem *)0)->repeatSmoke ) },
	{ NULL, 0 }
};

static classVariableInfo_t idItemTeam_typeInfo[] = {
	{ ": int", "team", (intptr_t)(&((idItemTeam *)0)->team), sizeof( ((idItemTeam *)0)->team ) },
	{ "bool", "carried", (intptr_t)(&((idItemTeam *)0)->carried), sizeof( ((idItemTeam *)0)->carried ) },
	{ "bool", "dropped", (intptr_t)(&((idItemTeam *)0)->dropped), sizeof( ((idItemTeam *)0)->dropped ) },
	{ ": idVec3", "returnOrigin", (intptr_t)(&((idItemTeam *)0)->returnOrigin), sizeof( ((idItemTeam *)0)->returnOrigin ) },
	{ "idMat3", "returnAxis", (intptr_t)(&((idItemTeam *)0)->returnAxis), sizeof( ((idItemTeam *)0)->returnAxis ) },
	{ "int", "lastDrop", (intptr_t)(&((idItemTeam *)0)->lastDrop), sizeof( ((idItemTeam *)0)->lastDrop ) },
	{ "const idDeclSkin *", "skinDefault", (intptr_t)(&((idItemTeam *)0)->skinDefault), sizeof( ((idItemTeam *)0)->skinDefault ) },
	{ "const idDeclSkin *", "skinCarried", (intptr_t)(&((idItemTeam *)0)->skinCarried), sizeof( ((idItemTeam *)0)->skinCarried ) },
	{ "const function_t *", "scriptTaken", (intptr_t)(&((idItemTeam *)0)->scriptTaken), sizeof( ((idItemTeam *)0)->scriptTaken ) },
	{ "const function_t *", "scriptDropped", (intptr_t)(&((idItemTeam *)0)->scriptDropped), sizeof( ((idItemTeam *)0)->scriptDropped ) },
	{ "const function_t *", "scriptReturned", (intptr_t)(&((idItemTeam *)0)->scriptReturned), sizeof( ((idItemTeam *)0)->scriptReturned ) },
	{ "const function_t *", "scriptCaptured", (intptr_t)(&((idItemTeam *)0)->scriptCaptured), sizeof( ((idItemTeam *)0)->scriptCaptured ) },
	{ "renderLight_t", "itemGlow", (intptr_t)(&((idItemTeam *)0)->itemGlow), sizeof( ((idItemTeam *)0)->itemGlow ) },
	{ "int", "itemGlowHandle", (intptr_t)(&((idItemTeam *)0)->itemGlowHandle), sizeof( ((idItemTeam *)0)->itemGlowHandle ) },
	{ "int", "lastNuggetDrop", (intptr_t)(&((idItemTeam *)0)->lastNuggetDrop), sizeof( ((idItemTeam *)0)->lastNuggetDrop ) },
	{ "const char *", "nuggetName", (intptr_t)(&((idItemTeam *)0)->nuggetName), sizeof( ((idItemTeam *)0)->nuggetName ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMoveablePDAItem_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idItemRemover_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idObjectiveComplete_typeInfo[] = {
	{ ": idVec3", "playerPos", (intptr_t)(&((idObjectiveComplete *)0)->playerPos), sizeof( ((idObjectiveComplete *)0)->playerPos ) },
	{ NULL, 0 }
};

static classVariableInfo_t idItemInfo_typeInfo[] = {
	{ "idStr", "name", (intptr_t)(&((idItemInfo *)0)->name), sizeof( ((idItemInfo *)0)->name ) },
	{ "idStr", "icon", (intptr_t)(&((idItemInfo *)0)->icon), sizeof( ((idItemInfo *)0)->icon ) },
	{ NULL, 0 }
};

static classVariableInfo_t idObjectiveInfo_typeInfo[] = {
	{ "idStr", "title", (intptr_t)(&((idObjectiveInfo *)0)->title), sizeof( ((idObjectiveInfo *)0)->title ) },
	{ "idStr", "text", (intptr_t)(&((idObjectiveInfo *)0)->text), sizeof( ((idObjectiveInfo *)0)->text ) },
	{ "idStr", "screenshot", (intptr_t)(&((idObjectiveInfo *)0)->screenshot), sizeof( ((idObjectiveInfo *)0)->screenshot ) },
	{ NULL, 0 }
};

static classVariableInfo_t idLevelTriggerInfo_typeInfo[] = {
	{ "idStr", "levelName", (intptr_t)(&((idLevelTriggerInfo *)0)->levelName), sizeof( ((idLevelTriggerInfo *)0)->levelName ) },
	{ "idStr", "triggerName", (intptr_t)(&((idLevelTriggerInfo *)0)->triggerName), sizeof( ((idLevelTriggerInfo *)0)->triggerName ) },
	{ NULL, 0 }
};

static classVariableInfo_t RechargeAmmo_t_typeInfo[] = {
	{ "int", "ammo", (intptr_t)(&((RechargeAmmo_t *)0)->ammo), sizeof( ((RechargeAmmo_t *)0)->ammo ) },
	{ "int", "rechargeTime", (intptr_t)(&((RechargeAmmo_t *)0)->rechargeTime), sizeof( ((RechargeAmmo_t *)0)->rechargeTime ) },
	{ "char[128]", "ammoName", (intptr_t)(&((RechargeAmmo_t *)0)->ammoName), sizeof( ((RechargeAmmo_t *)0)->ammoName ) },
	{ NULL, 0 }
};

static classVariableInfo_t WeaponToggle_t_typeInfo[] = {
	{ "char[64]", "name", (intptr_t)(&((WeaponToggle_t *)0)->name), sizeof( ((WeaponToggle_t *)0)->name ) },
	{ "idList < int >", "toggleList", (intptr_t)(&((WeaponToggle_t *)0)->toggleList), sizeof( ((WeaponToggle_t *)0)->toggleList ) },
	{ NULL, 0 }
};

static classVariableInfo_t idInventory_typeInfo[] = {
	{ ": int", "maxHealth", (intptr_t)(&((idInventory *)0)->maxHealth), sizeof( ((idInventory *)0)->maxHealth ) },
	{ "int", "weapons", (intptr_t)(&((idInventory *)0)->weapons), sizeof( ((idInventory *)0)->weapons ) },
	{ "int", "powerups", (intptr_t)(&((idInventory *)0)->powerups), sizeof( ((idInventory *)0)->powerups ) },
	{ "int", "armor", (intptr_t)(&((idInventory *)0)->armor), sizeof( ((idInventory *)0)->armor ) },
	{ "int", "maxarmor", (intptr_t)(&((idInventory *)0)->maxarmor), sizeof( ((idInventory *)0)->maxarmor ) },
	{ "int[16]", "ammo", (intptr_t)(&((idInventory *)0)->ammo), sizeof( ((idInventory *)0)->ammo ) },
	{ "int[32]", "clip", (intptr_t)(&((idInventory *)0)->clip), sizeof( ((idInventory *)0)->clip ) },
	{ "int[8]", "powerupEndTime", (intptr_t)(&((idInventory *)0)->powerupEndTime), sizeof( ((idInventory *)0)->powerupEndTime ) },
	{ "RechargeAmmo_t[16]", "rechargeAmmo", (intptr_t)(&((idInventory *)0)->rechargeAmmo), sizeof( ((idInventory *)0)->rechargeAmmo ) },
	{ "int", "ammoPredictTime", (intptr_t)(&((idInventory *)0)->ammoPredictTime), sizeof( ((idInventory *)0)->ammoPredictTime ) },
	{ "int", "deplete_armor", (intptr_t)(&((idInventory *)0)->deplete_armor), sizeof( ((idInventory *)0)->deplete_armor ) },
	{ "float", "deplete_rate", (intptr_t)(&((idInventory *)0)->deplete_rate), sizeof( ((idInventory *)0)->deplete_rate ) },
	{ "int", "deplete_ammount", (intptr_t)(&((idInventory *)0)->deplete_ammount), sizeof( ((idInventory *)0)->deplete_ammount ) },
	{ "int", "nextArmorDepleteTime", (intptr_t)(&((idInventory *)0)->nextArmorDepleteTime), sizeof( ((idInventory *)0)->nextArmorDepleteTime ) },
	{ "int[4]", "pdasViewed", (intptr_t)(&((idInventory *)0)->pdasViewed), sizeof( ((idInventory *)0)->pdasViewed ) },
	{ "int", "selPDA", (intptr_t)(&((idInventory *)0)->selPDA), sizeof( ((idInventory *)0)->selPDA ) },
	{ "int", "selEMail", (intptr_t)(&((idInventory *)0)->selEMail), sizeof( ((idInventory *)0)->selEMail ) },
	{ "int", "selVideo", (intptr_t)(&((idInventory *)0)->selVideo), sizeof( ((idInventory *)0)->selVideo ) },
	{ "int", "selAudio", (intptr_t)(&((idInventory *)0)->selAudio), sizeof( ((idInventory *)0)->selAudio ) },
	{ "bool", "pdaOpened", (intptr_t)(&((idInventory *)0)->pdaOpened), sizeof( ((idInventory *)0)->pdaOpened ) },
	{ "bool", "turkeyScore", (intptr_t)(&((idInventory *)0)->turkeyScore), sizeof( ((idInventory *)0)->turkeyScore ) },
	{ "idList < idDict * >", "items", (intptr_t)(&((idInventory *)0)->items), sizeof( ((idInventory *)0)->items ) },
	{ "idStrList", "pdas", (intptr_t)(&((idInventory *)0)->pdas), sizeof( ((idInventory *)0)->pdas ) },
	{ "idStrList", "pdaSecurity", (intptr_t)(&((idInventory *)0)->pdaSecurity), sizeof( ((idInventory *)0)->pdaSecurity ) },
	{ "idStrList", "videos", (intptr_t)(&((idInventory *)0)->videos), sizeof( ((idInventory *)0)->videos ) },
	{ "idStrList", "emails", (intptr_t)(&((idInventory *)0)->emails), sizeof( ((idInventory *)0)->emails ) },
	{ "bool", "ammoPulse", (intptr_t)(&((idInventory *)0)->ammoPulse), sizeof( ((idInventory *)0)->ammoPulse ) },
	{ "bool", "weaponPulse", (intptr_t)(&((idInventory *)0)->weaponPulse), sizeof( ((idInventory *)0)->weaponPulse ) },
	{ "bool", "armorPulse", (intptr_t)(&((idInventory *)0)->armorPulse), sizeof( ((idInventory *)0)->armorPulse ) },
	{ "int", "lastGiveTime", (intptr_t)(&((idInventory *)0)->lastGiveTime), sizeof( ((idInventory *)0)->lastGiveTime ) },
	{ "idList < idLevelTriggerInfo >", "levelTriggers", (intptr_t)(&((idInventory *)0)->levelTriggers), sizeof( ((idInventory *)0)->levelTriggers ) },
	{ "int", "nextItemPickup", (intptr_t)(&((idInventory *)0)->nextItemPickup), sizeof( ((idInventory *)0)->nextItemPickup ) },
	{ "int", "nextItemNum", (intptr_t)(&((idInventory *)0)->nextItemNum), sizeof( ((idInventory *)0)->nextItemNum ) },
	{ "int", "onePickupTime", (intptr_t)(&((idInventory *)0)->onePickupTime), sizeof( ((idInventory *)0)->onePickupTime ) },
	{ "idList < idItemInfo >", "pickupItemNames", (intptr_t)(&((idInventory *)0)->pickupItemNames), sizeof( ((idInventory *)0)->pickupItemNames ) },
	{ "idList < idObjectiveInfo >", "objectiveNames", (intptr_t)(&((idInventory *)0)->objectiveNames), sizeof( ((idInventory *)0)->objectiveNames ) },
	{ NULL, 0 }
};

static classVariableInfo_t loggedAccel_t_typeInfo[] = {
	{ "int", "time", (intptr_t)(&((loggedAccel_t *)0)->time), sizeof( ((loggedAccel_t *)0)->time ) },
	{ "idVec3", "dir", (intptr_t)(&((loggedAccel_t *)0)->dir), sizeof( ((loggedAccel_t *)0)->dir ) },
	{ NULL, 0 }
};

static classVariableInfo_t aasLocation_t_typeInfo[] = {
	{ "int", "areaNum", (intptr_t)(&((aasLocation_t *)0)->areaNum), sizeof( ((aasLocation_t *)0)->areaNum ) },
	{ "idVec3", "pos", (intptr_t)(&((aasLocation_t *)0)->pos), sizeof( ((aasLocation_t *)0)->pos ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPlayer_typeInfo[] = {
	{ "usercmd_t", "usercmd", (intptr_t)(&((idPlayer *)0)->usercmd), sizeof( ((idPlayer *)0)->usercmd ) },
	{ "bool", "noclip", (intptr_t)(&((idPlayer *)0)->noclip), sizeof( ((idPlayer *)0)->noclip ) },
	{ "bool", "godmode", (intptr_t)(&((idPlayer *)0)->godmode), sizeof( ((idPlayer *)0)->godmode ) },
	{ "bool", "spawnAnglesSet", (intptr_t)(&((idPlayer *)0)->spawnAnglesSet), sizeof( ((idPlayer *)0)->spawnAnglesSet ) },
	{ "idAngles", "spawnAngles", (intptr_t)(&((idPlayer *)0)->spawnAngles), sizeof( ((idPlayer *)0)->spawnAngles ) },
	{ "idAngles", "viewAngles", (intptr_t)(&((idPlayer *)0)->viewAngles), sizeof( ((idPlayer *)0)->viewAngles ) },
	{ "idAngles", "cmdAngles", (intptr_t)(&((idPlayer *)0)->cmdAngles), sizeof( ((idPlayer *)0)->cmdAngles ) },
	{ "int", "buttonMask", (intptr_t)(&((idPlayer *)0)->buttonMask), sizeof( ((idPlayer *)0)->buttonMask ) },
	{ "int", "oldButtons", (intptr_t)(&((idPlayer *)0)->oldButtons), sizeof( ((idPlayer *)0)->oldButtons ) },
	{ "int", "oldFlags", (intptr_t)(&((idPlayer *)0)->oldFlags), sizeof( ((idPlayer *)0)->oldFlags ) },
	{ "int", "lastHitTime", (intptr_t)(&((idPlayer *)0)->lastHitTime), sizeof( ((idPlayer *)0)->lastHitTime ) },
	{ "int", "lastSndHitTime", (intptr_t)(&((idPlayer *)0)->lastSndHitTime), sizeof( ((idPlayer *)0)->lastSndHitTime ) },
	{ "int", "lastSavingThrowTime", (intptr_t)(&((idPlayer *)0)->lastSavingThrowTime), sizeof( ((idPlayer *)0)->lastSavingThrowTime ) },
	{ "bool", "AI_FORWARD", (intptr_t)(&((idPlayer *)0)->AI_FORWARD), sizeof( ((idPlayer *)0)->AI_FORWARD ) },
	{ "bool", "AI_BACKWARD", (intptr_t)(&((idPlayer *)0)->AI_BACKWARD), sizeof( ((idPlayer *)0)->AI_BACKWARD ) },
	{ "bool", "AI_STRAFE_LEFT", (intptr_t)(&((idPlayer *)0)->AI_STRAFE_LEFT), sizeof( ((idPlayer *)0)->AI_STRAFE_LEFT ) },
	{ "bool", "AI_STRAFE_RIGHT", (intptr_t)(&((idPlayer *)0)->AI_STRAFE_RIGHT), sizeof( ((idPlayer *)0)->AI_STRAFE_RIGHT ) },
	{ "bool", "AI_ATTACK_HELD", (intptr_t)(&((idPlayer *)0)->AI_ATTACK_HELD), sizeof( ((idPlayer *)0)->AI_ATTACK_HELD ) },
	{ "bool", "AI_WEAPON_FIRED", (intptr_t)(&((idPlayer *)0)->AI_WEAPON_FIRED), sizeof( ((idPlayer *)0)->AI_WEAPON_FIRED ) },
	{ "bool", "AI_JUMP", (intptr_t)(&((idPlayer *)0)->AI_JUMP), sizeof( ((idPlayer *)0)->AI_JUMP ) },
	{ "bool", "AI_CROUCH", (intptr_t)(&((idPlayer *)0)->AI_CROUCH), sizeof( ((idPlayer *)0)->AI_CROUCH ) },
	{ "bool", "AI_ONGROUND", (intptr_t)(&((idPlayer *)0)->AI_ONGROUND), sizeof( ((idPlayer *)0)->AI_ONGROUND ) },
	{ "bool", "AI_ONLADDER", (intptr_t)(&((idPlayer *)0)->AI_ONLADDER), sizeof( ((idPlayer *)0)->AI_ONLADDER ) },
	{ "bool", "AI_DEAD", (intptr_t)(&((idPlayer *)0)->AI_DEAD), sizeof( ((idPlayer *)0)->AI_DEAD ) },
	{ "bool", "AI_RUN", (intptr_t)(&((idPlayer *)0)->AI_RUN), sizeof( ((idPlayer *)0)->AI_RUN ) },
	{ "bool", "AI_PAIN", (intptr_t)(&((idPlayer *)0)->AI_PAIN), sizeof( ((idPlayer *)0)->AI_PAIN ) },
	{ "bool", "AI_HARDLANDING", (intptr_t)(&((idPlayer *)0)->AI_HARDLANDING), sizeof( ((idPlayer *)0)->AI_HARDLANDING ) },
	{ "bool", "AI_SOFTLANDING", (intptr_t)(&((idPlayer *)0)->AI_SOFTLANDING), sizeof( ((idPlayer *)0)->AI_SOFTLANDING ) },
	{ "bool", "AI_RELOAD", (intptr_t)(&((idPlayer *)0)->AI_RELOAD), sizeof( ((idPlayer *)0)->AI_RELOAD ) },
	{ "bool", "AI_TELEPORT", (intptr_t)(&((idPlayer *)0)->AI_TELEPORT), sizeof( ((idPlayer *)0)->AI_TELEPORT ) },
	{ "bool", "AI_TURN_LEFT", (intptr_t)(&((idPlayer *)0)->AI_TURN_LEFT), sizeof( ((idPlayer *)0)->AI_TURN_LEFT ) },
	{ "bool", "AI_TURN_RIGHT", (intptr_t)(&((idPlayer *)0)->AI_TURN_RIGHT), sizeof( ((idPlayer *)0)->AI_TURN_RIGHT ) },
	{ "idInventory", "inventory", (intptr_t)(&((idPlayer *)0)->inventory), sizeof( ((idPlayer *)0)->inventory ) },
	{ "idEntityPtr < idWeapon >", "weapon", (intptr_t)(&((idPlayer *)0)->weapon), sizeof( ((idPlayer *)0)->weapon ) },
	{ "idUserInterface *", "hud", (intptr_t)(&((idPlayer *)0)->hud), sizeof( ((idPlayer *)0)->hud ) },
	{ "idUserInterface *", "objectiveSystem", (intptr_t)(&((idPlayer *)0)->objectiveSystem), sizeof( ((idPlayer *)0)->objectiveSystem ) },
	{ "bool", "objectiveSystemOpen", (intptr_t)(&((idPlayer *)0)->objectiveSystemOpen), sizeof( ((idPlayer *)0)->objectiveSystemOpen ) },
	{ "int", "weapon_soulcube", (intptr_t)(&((idPlayer *)0)->weapon_soulcube), sizeof( ((idPlayer *)0)->weapon_soulcube ) },
	{ "int", "weapon_pda", (intptr_t)(&((idPlayer *)0)->weapon_pda), sizeof( ((idPlayer *)0)->weapon_pda ) },
	{ "int", "weapon_fists", (intptr_t)(&((idPlayer *)0)->weapon_fists), sizeof( ((idPlayer *)0)->weapon_fists ) },
	{ "int", "heartRate", (intptr_t)(&((idPlayer *)0)->heartRate), sizeof( ((idPlayer *)0)->heartRate ) },
	{ "idInterpolate < float >", "heartInfo", (intptr_t)(&((idPlayer *)0)->heartInfo), sizeof( ((idPlayer *)0)->heartInfo ) },
	{ "int", "lastHeartAdjust", (intptr_t)(&((idPlayer *)0)->lastHeartAdjust), sizeof( ((idPlayer *)0)->lastHeartAdjust ) },
	{ "int", "lastHeartBeat", (intptr_t)(&((idPlayer *)0)->lastHeartBeat), sizeof( ((idPlayer *)0)->lastHeartBeat ) },
	{ "int", "lastDmgTime", (intptr_t)(&((idPlayer *)0)->lastDmgTime), sizeof( ((idPlayer *)0)->lastDmgTime ) },
	{ "int", "deathClearContentsTime", (intptr_t)(&((idPlayer *)0)->deathClearContentsTime), sizeof( ((idPlayer *)0)->deathClearContentsTime ) },
	{ "bool", "doingDeathSkin", (intptr_t)(&((idPlayer *)0)->doingDeathSkin), sizeof( ((idPlayer *)0)->doingDeathSkin ) },
	{ "int", "lastArmorPulse", (intptr_t)(&((idPlayer *)0)->lastArmorPulse), sizeof( ((idPlayer *)0)->lastArmorPulse ) },
	{ "float", "stamina", (intptr_t)(&((idPlayer *)0)->stamina), sizeof( ((idPlayer *)0)->stamina ) },
	{ "float", "healthPool", (intptr_t)(&((idPlayer *)0)->healthPool), sizeof( ((idPlayer *)0)->healthPool ) },
	{ "int", "nextHealthPulse", (intptr_t)(&((idPlayer *)0)->nextHealthPulse), sizeof( ((idPlayer *)0)->nextHealthPulse ) },
	{ "bool", "healthPulse", (intptr_t)(&((idPlayer *)0)->healthPulse), sizeof( ((idPlayer *)0)->healthPulse ) },
	{ "bool", "healthTake", (intptr_t)(&((idPlayer *)0)->healthTake), sizeof( ((idPlayer *)0)->healthTake ) },
	{ "int", "nextHealthTake", (intptr_t)(&((idPlayer *)0)->nextHealthTake), sizeof( ((idPlayer *)0)->nextHealthTake ) },
	{ "bool", "hiddenWeapon", (intptr_t)(&((idPlayer *)0)->hiddenWeapon), sizeof( ((idPlayer *)0)->hiddenWeapon ) },
	{ "idEntityPtr < idProjectile >", "soulCubeProjectile", (intptr_t)(&((idPlayer *)0)->soulCubeProjectile), sizeof( ((idPlayer *)0)->soulCubeProjectile ) },
	{ "int", "spectator", (intptr_t)(&((idPlayer *)0)->spectator), sizeof( ((idPlayer *)0)->spectator ) },
	{ "idVec3", "colorBar", (intptr_t)(&((idPlayer *)0)->colorBar), sizeof( ((idPlayer *)0)->colorBar ) },
	{ "int", "colorBarIndex", (intptr_t)(&((idPlayer *)0)->colorBarIndex), sizeof( ((idPlayer *)0)->colorBarIndex ) },
	{ "bool", "scoreBoardOpen", (intptr_t)(&((idPlayer *)0)->scoreBoardOpen), sizeof( ((idPlayer *)0)->scoreBoardOpen ) },
	{ "bool", "forceScoreBoard", (intptr_t)(&((idPlayer *)0)->forceScoreBoard), sizeof( ((idPlayer *)0)->forceScoreBoard ) },
	{ "bool", "forceRespawn", (intptr_t)(&((idPlayer *)0)->forceRespawn), sizeof( ((idPlayer *)0)->forceRespawn ) },
	{ "bool", "spectating", (intptr_t)(&((idPlayer *)0)->spectating), sizeof( ((idPlayer *)0)->spectating ) },
	{ "int", "lastSpectateTeleport", (intptr_t)(&((idPlayer *)0)->lastSpectateTeleport), sizeof( ((idPlayer *)0)->lastSpectateTeleport ) },
	{ "bool", "lastHitToggle", (intptr_t)(&((idPlayer *)0)->lastHitToggle), sizeof( ((idPlayer *)0)->lastHitToggle ) },
	{ "bool", "forcedReady", (intptr_t)(&((idPlayer *)0)->forcedReady), sizeof( ((idPlayer *)0)->forcedReady ) },
	{ "bool", "wantSpectate", (intptr_t)(&((idPlayer *)0)->wantSpectate), sizeof( ((idPlayer *)0)->wantSpectate ) },
	{ "bool", "weaponGone", (intptr_t)(&((idPlayer *)0)->weaponGone), sizeof( ((idPlayer *)0)->weaponGone ) },
	{ "bool", "useInitialSpawns", (intptr_t)(&((idPlayer *)0)->useInitialSpawns), sizeof( ((idPlayer *)0)->useInitialSpawns ) },
	{ "int", "latchedTeam", (intptr_t)(&((idPlayer *)0)->latchedTeam), sizeof( ((idPlayer *)0)->latchedTeam ) },
	{ "int", "tourneyRank", (intptr_t)(&((idPlayer *)0)->tourneyRank), sizeof( ((idPlayer *)0)->tourneyRank ) },
	{ "int", "tourneyLine", (intptr_t)(&((idPlayer *)0)->tourneyLine), sizeof( ((idPlayer *)0)->tourneyLine ) },
	{ "int", "spawnedTime", (intptr_t)(&((idPlayer *)0)->spawnedTime), sizeof( ((idPlayer *)0)->spawnedTime ) },
	{ "bool", "carryingFlag", (intptr_t)(&((idPlayer *)0)->carryingFlag), sizeof( ((idPlayer *)0)->carryingFlag ) },
	{ "idEntityPtr < idEntity >", "teleportEntity", (intptr_t)(&((idPlayer *)0)->teleportEntity), sizeof( ((idPlayer *)0)->teleportEntity ) },
	{ "int", "teleportKiller", (intptr_t)(&((idPlayer *)0)->teleportKiller), sizeof( ((idPlayer *)0)->teleportKiller ) },
	{ "bool", "lastManOver", (intptr_t)(&((idPlayer *)0)->lastManOver), sizeof( ((idPlayer *)0)->lastManOver ) },
	{ "bool", "lastManPlayAgain", (intptr_t)(&((idPlayer *)0)->lastManPlayAgain), sizeof( ((idPlayer *)0)->lastManPlayAgain ) },
	{ "bool", "lastManPresent", (intptr_t)(&((idPlayer *)0)->lastManPresent), sizeof( ((idPlayer *)0)->lastManPresent ) },
	{ "bool", "isLagged", (intptr_t)(&((idPlayer *)0)->isLagged), sizeof( ((idPlayer *)0)->isLagged ) },
	{ "bool", "isChatting", (intptr_t)(&((idPlayer *)0)->isChatting), sizeof( ((idPlayer *)0)->isChatting ) },
	{ "int", "minRespawnTime", (intptr_t)(&((idPlayer *)0)->minRespawnTime), sizeof( ((idPlayer *)0)->minRespawnTime ) },
	{ "int", "maxRespawnTime", (intptr_t)(&((idPlayer *)0)->maxRespawnTime), sizeof( ((idPlayer *)0)->maxRespawnTime ) },
	{ "idVec3", "firstPersonViewOrigin", (intptr_t)(&((idPlayer *)0)->firstPersonViewOrigin), sizeof( ((idPlayer *)0)->firstPersonViewOrigin ) },
	{ "idMat3", "firstPersonViewAxis", (intptr_t)(&((idPlayer *)0)->firstPersonViewAxis), sizeof( ((idPlayer *)0)->firstPersonViewAxis ) },
	{ "idDragEntity", "dragEntity", (intptr_t)(&((idPlayer *)0)->dragEntity), sizeof( ((idPlayer *)0)->dragEntity ) },
	{ "idFuncMountedObject *", "mountedObject", (intptr_t)(&((idPlayer *)0)->mountedObject), sizeof( ((idPlayer *)0)->mountedObject ) },
	{ "idEntityPtr < idLight >", "enviroSuitLight", (intptr_t)(&((idPlayer *)0)->enviroSuitLight), sizeof( ((idPlayer *)0)->enviroSuitLight ) },
	{ "bool", "healthRecharge", (intptr_t)(&((idPlayer *)0)->healthRecharge), sizeof( ((idPlayer *)0)->healthRecharge ) },
	{ "int", "lastHealthRechargeTime", (intptr_t)(&((idPlayer *)0)->lastHealthRechargeTime), sizeof( ((idPlayer *)0)->lastHealthRechargeTime ) },
	{ "int", "rechargeSpeed", (intptr_t)(&((idPlayer *)0)->rechargeSpeed), sizeof( ((idPlayer *)0)->rechargeSpeed ) },
	{ "float", "new_g_damageScale", (intptr_t)(&((idPlayer *)0)->new_g_damageScale), sizeof( ((idPlayer *)0)->new_g_damageScale ) },
	{ "bool", "bloomEnabled", (intptr_t)(&((idPlayer *)0)->bloomEnabled), sizeof( ((idPlayer *)0)->bloomEnabled ) },
	{ "float", "bloomSpeed", (intptr_t)(&((idPlayer *)0)->bloomSpeed), sizeof( ((idPlayer *)0)->bloomSpeed ) },
	{ "float", "bloomIntensity", (intptr_t)(&((idPlayer *)0)->bloomIntensity), sizeof( ((idPlayer *)0)->bloomIntensity ) },
	{ ": jointHandle_t", "hipJoint", (intptr_t)(&((idPlayer *)0)->hipJoint), sizeof( ((idPlayer *)0)->hipJoint ) },
	{ "jointHandle_t", "chestJoint", (intptr_t)(&((idPlayer *)0)->chestJoint), sizeof( ((idPlayer *)0)->chestJoint ) },
	{ "jointHandle_t", "headJoint", (intptr_t)(&((idPlayer *)0)->headJoint), sizeof( ((idPlayer *)0)->headJoint ) },
	{ "idPhysics_Player", "physicsObj", (intptr_t)(&((idPlayer *)0)->physicsObj), sizeof( ((idPlayer *)0)->physicsObj ) },
	{ "idList < aasLocation_t >", "aasLocation", (intptr_t)(&((idPlayer *)0)->aasLocation), sizeof( ((idPlayer *)0)->aasLocation ) },
	{ "int", "bobFoot", (intptr_t)(&((idPlayer *)0)->bobFoot), sizeof( ((idPlayer *)0)->bobFoot ) },
	{ "float", "bobFrac", (intptr_t)(&((idPlayer *)0)->bobFrac), sizeof( ((idPlayer *)0)->bobFrac ) },
	{ "float", "bobfracsin", (intptr_t)(&((idPlayer *)0)->bobfracsin), sizeof( ((idPlayer *)0)->bobfracsin ) },
	{ "int", "bobCycle", (intptr_t)(&((idPlayer *)0)->bobCycle), sizeof( ((idPlayer *)0)->bobCycle ) },
	{ "float", "xyspeed", (intptr_t)(&((idPlayer *)0)->xyspeed), sizeof( ((idPlayer *)0)->xyspeed ) },
	{ "int", "stepUpTime", (intptr_t)(&((idPlayer *)0)->stepUpTime), sizeof( ((idPlayer *)0)->stepUpTime ) },
	{ "float", "stepUpDelta", (intptr_t)(&((idPlayer *)0)->stepUpDelta), sizeof( ((idPlayer *)0)->stepUpDelta ) },
	{ "float", "idealLegsYaw", (intptr_t)(&((idPlayer *)0)->idealLegsYaw), sizeof( ((idPlayer *)0)->idealLegsYaw ) },
	{ "float", "legsYaw", (intptr_t)(&((idPlayer *)0)->legsYaw), sizeof( ((idPlayer *)0)->legsYaw ) },
	{ "bool", "legsForward", (intptr_t)(&((idPlayer *)0)->legsForward), sizeof( ((idPlayer *)0)->legsForward ) },
	{ "float", "oldViewYaw", (intptr_t)(&((idPlayer *)0)->oldViewYaw), sizeof( ((idPlayer *)0)->oldViewYaw ) },
	{ "idAngles", "viewBobAngles", (intptr_t)(&((idPlayer *)0)->viewBobAngles), sizeof( ((idPlayer *)0)->viewBobAngles ) },
	{ "idVec3", "viewBob", (intptr_t)(&((idPlayer *)0)->viewBob), sizeof( ((idPlayer *)0)->viewBob ) },
	{ "int", "landChange", (intptr_t)(&((idPlayer *)0)->landChange), sizeof( ((idPlayer *)0)->landChange ) },
	{ "int", "landTime", (intptr_t)(&((idPlayer *)0)->landTime), sizeof( ((idPlayer *)0)->landTime ) },
	{ "int", "currentWeapon", (intptr_t)(&((idPlayer *)0)->currentWeapon), sizeof( ((idPlayer *)0)->currentWeapon ) },
	{ "int", "idealWeapon", (intptr_t)(&((idPlayer *)0)->idealWeapon), sizeof( ((idPlayer *)0)->idealWeapon ) },
	{ "int", "previousWeapon", (intptr_t)(&((idPlayer *)0)->previousWeapon), sizeof( ((idPlayer *)0)->previousWeapon ) },
	{ "int", "weaponSwitchTime", (intptr_t)(&((idPlayer *)0)->weaponSwitchTime), sizeof( ((idPlayer *)0)->weaponSwitchTime ) },
	{ "bool", "weaponEnabled", (intptr_t)(&((idPlayer *)0)->weaponEnabled), sizeof( ((idPlayer *)0)->weaponEnabled ) },
	{ "bool", "showWeaponViewModel", (intptr_t)(&((idPlayer *)0)->showWeaponViewModel), sizeof( ((idPlayer *)0)->showWeaponViewModel ) },
	{ "const idDeclSkin *", "skin", (intptr_t)(&((idPlayer *)0)->skin), sizeof( ((idPlayer *)0)->skin ) },
	{ "const idDeclSkin *", "powerUpSkin", (intptr_t)(&((idPlayer *)0)->powerUpSkin), sizeof( ((idPlayer *)0)->powerUpSkin ) },
	{ "idStr", "baseSkinName", (intptr_t)(&((idPlayer *)0)->baseSkinName), sizeof( ((idPlayer *)0)->baseSkinName ) },
	{ "int", "numProjectilesFired", (intptr_t)(&((idPlayer *)0)->numProjectilesFired), sizeof( ((idPlayer *)0)->numProjectilesFired ) },
	{ "int", "numProjectileHits", (intptr_t)(&((idPlayer *)0)->numProjectileHits), sizeof( ((idPlayer *)0)->numProjectileHits ) },
	{ "bool", "airless", (intptr_t)(&((idPlayer *)0)->airless), sizeof( ((idPlayer *)0)->airless ) },
	{ "int", "airTics", (intptr_t)(&((idPlayer *)0)->airTics), sizeof( ((idPlayer *)0)->airTics ) },
	{ "int", "lastAirDamage", (intptr_t)(&((idPlayer *)0)->lastAirDamage), sizeof( ((idPlayer *)0)->lastAirDamage ) },
	{ "bool", "gibDeath", (intptr_t)(&((idPlayer *)0)->gibDeath), sizeof( ((idPlayer *)0)->gibDeath ) },
	{ "bool", "gibsLaunched", (intptr_t)(&((idPlayer *)0)->gibsLaunched), sizeof( ((idPlayer *)0)->gibsLaunched ) },
	{ "idVec3", "gibsDir", (intptr_t)(&((idPlayer *)0)->gibsDir), sizeof( ((idPlayer *)0)->gibsDir ) },
	{ "idInterpolate < float >", "zoomFov", (intptr_t)(&((idPlayer *)0)->zoomFov), sizeof( ((idPlayer *)0)->zoomFov ) },
	{ "idInterpolate < float >", "centerView", (intptr_t)(&((idPlayer *)0)->centerView), sizeof( ((idPlayer *)0)->centerView ) },
	{ "bool", "fxFov", (intptr_t)(&((idPlayer *)0)->fxFov), sizeof( ((idPlayer *)0)->fxFov ) },
	{ "float", "influenceFov", (intptr_t)(&((idPlayer *)0)->influenceFov), sizeof( ((idPlayer *)0)->influenceFov ) },
	{ "int", "influenceActive", (intptr_t)(&((idPlayer *)0)->influenceActive), sizeof( ((idPlayer *)0)->influenceActive ) },
	{ "idEntity *", "influenceEntity", (intptr_t)(&((idPlayer *)0)->influenceEntity), sizeof( ((idPlayer *)0)->influenceEntity ) },
	{ "const idMaterial *", "influenceMaterial", (intptr_t)(&((idPlayer *)0)->influenceMaterial), sizeof( ((idPlayer *)0)->influenceMaterial ) },
	{ "float", "influenceRadius", (intptr_t)(&((idPlayer *)0)->influenceRadius), sizeof( ((idPlayer *)0)->influenceRadius ) },
	{ "const idDeclSkin *", "influenceSkin", (intptr_t)(&((idPlayer *)0)->influenceSkin), sizeof( ((idPlayer *)0)->influenceSkin ) },
	{ "idCamera *", "privateCameraView", (intptr_t)(&((idPlayer *)0)->privateCameraView), sizeof( ((idPlayer *)0)->privateCameraView ) },
	{ "idAngles[64]", "loggedViewAngles", (intptr_t)(&((idPlayer *)0)->loggedViewAngles), sizeof( ((idPlayer *)0)->loggedViewAngles ) },
	{ "loggedAccel_t[16]", "loggedAccel", (intptr_t)(&((idPlayer *)0)->loggedAccel), sizeof( ((idPlayer *)0)->loggedAccel ) },
	{ "int", "currentLoggedAccel", (intptr_t)(&((idPlayer *)0)->currentLoggedAccel), sizeof( ((idPlayer *)0)->currentLoggedAccel ) },
	{ "idEntity *", "focusGUIent", (intptr_t)(&((idPlayer *)0)->focusGUIent), sizeof( ((idPlayer *)0)->focusGUIent ) },
	{ "idUserInterface *", "focusUI", (intptr_t)(&((idPlayer *)0)->focusUI), sizeof( ((idPlayer *)0)->focusUI ) },
	{ "idAI *", "focusCharacter", (intptr_t)(&((idPlayer *)0)->focusCharacter), sizeof( ((idPlayer *)0)->focusCharacter ) },
	{ "int", "talkCursor", (intptr_t)(&((idPlayer *)0)->talkCursor), sizeof( ((idPlayer *)0)->talkCursor ) },
	{ "int", "focusTime", (intptr_t)(&((idPlayer *)0)->focusTime), sizeof( ((idPlayer *)0)->focusTime ) },
	{ "idAFEntity_Vehicle *", "focusVehicle", (intptr_t)(&((idPlayer *)0)->focusVehicle), sizeof( ((idPlayer *)0)->focusVehicle ) },
	{ "idUserInterface *", "cursor", (intptr_t)(&((idPlayer *)0)->cursor), sizeof( ((idPlayer *)0)->cursor ) },
	{ "int", "oldMouseX", (intptr_t)(&((idPlayer *)0)->oldMouseX), sizeof( ((idPlayer *)0)->oldMouseX ) },
	{ "int", "oldMouseY", (intptr_t)(&((idPlayer *)0)->oldMouseY), sizeof( ((idPlayer *)0)->oldMouseY ) },
	{ "idStr", "pdaAudio", (intptr_t)(&((idPlayer *)0)->pdaAudio), sizeof( ((idPlayer *)0)->pdaAudio ) },
	{ "idStr", "pdaVideo", (intptr_t)(&((idPlayer *)0)->pdaVideo), sizeof( ((idPlayer *)0)->pdaVideo ) },
	{ "idStr", "pdaVideoWave", (intptr_t)(&((idPlayer *)0)->pdaVideoWave), sizeof( ((idPlayer *)0)->pdaVideoWave ) },
	{ "bool", "tipUp", (intptr_t)(&((idPlayer *)0)->tipUp), sizeof( ((idPlayer *)0)->tipUp ) },
	{ "bool", "objectiveUp", (intptr_t)(&((idPlayer *)0)->objectiveUp), sizeof( ((idPlayer *)0)->objectiveUp ) },
	{ "int", "lastDamageDef", (intptr_t)(&((idPlayer *)0)->lastDamageDef), sizeof( ((idPlayer *)0)->lastDamageDef ) },
	{ "idVec3", "lastDamageDir", (intptr_t)(&((idPlayer *)0)->lastDamageDir), sizeof( ((idPlayer *)0)->lastDamageDir ) },
	{ "int", "lastDamageLocation", (intptr_t)(&((idPlayer *)0)->lastDamageLocation), sizeof( ((idPlayer *)0)->lastDamageLocation ) },
	{ "int", "smoothedFrame", (intptr_t)(&((idPlayer *)0)->smoothedFrame), sizeof( ((idPlayer *)0)->smoothedFrame ) },
	{ "bool", "smoothedOriginUpdated", (intptr_t)(&((idPlayer *)0)->smoothedOriginUpdated), sizeof( ((idPlayer *)0)->smoothedOriginUpdated ) },
	{ "idVec3", "smoothedOrigin", (intptr_t)(&((idPlayer *)0)->smoothedOrigin), sizeof( ((idPlayer *)0)->smoothedOrigin ) },
	{ "idAngles", "smoothedAngles", (intptr_t)(&((idPlayer *)0)->smoothedAngles), sizeof( ((idPlayer *)0)->smoothedAngles ) },
	{ "idHashTable < WeaponToggle_t >", "weaponToggles", (intptr_t)(&((idPlayer *)0)->weaponToggles), sizeof( ((idPlayer *)0)->weaponToggles ) },
	{ "int", "hudPowerup", (intptr_t)(&((idPlayer *)0)->hudPowerup), sizeof( ((idPlayer *)0)->hudPowerup ) },
	{ "int", "lastHudPowerup", (intptr_t)(&((idPlayer *)0)->lastHudPowerup), sizeof( ((idPlayer *)0)->lastHudPowerup ) },
	{ "int", "hudPowerupDuration", (intptr_t)(&((idPlayer *)0)->hudPowerupDuration), sizeof( ((idPlayer *)0)->hudPowerupDuration ) },
	{ "bool", "ready", (intptr_t)(&((idPlayer *)0)->ready), sizeof( ((idPlayer *)0)->ready ) },
	{ "bool", "respawning", (intptr_t)(&((idPlayer *)0)->respawning), sizeof( ((idPlayer *)0)->respawning ) },
	{ "bool", "leader", (intptr_t)(&((idPlayer *)0)->leader), sizeof( ((idPlayer *)0)->leader ) },
	{ "int", "lastSpectateChange", (intptr_t)(&((idPlayer *)0)->lastSpectateChange), sizeof( ((idPlayer *)0)->lastSpectateChange ) },
	{ "int", "lastTeleFX", (intptr_t)(&((idPlayer *)0)->lastTeleFX), sizeof( ((idPlayer *)0)->lastTeleFX ) },
	{ "unsigned int", "lastSnapshotSequence", (intptr_t)(&((idPlayer *)0)->lastSnapshotSequence), sizeof( ((idPlayer *)0)->lastSnapshotSequence ) },
	{ "bool", "weaponCatchup", (intptr_t)(&((idPlayer *)0)->weaponCatchup), sizeof( ((idPlayer *)0)->weaponCatchup ) },
	{ "int", "MPAim", (intptr_t)(&((idPlayer *)0)->MPAim), sizeof( ((idPlayer *)0)->MPAim ) },
	{ "int", "lastMPAim", (intptr_t)(&((idPlayer *)0)->lastMPAim), sizeof( ((idPlayer *)0)->lastMPAim ) },
	{ "int", "lastMPAimTime", (intptr_t)(&((idPlayer *)0)->lastMPAimTime), sizeof( ((idPlayer *)0)->lastMPAimTime ) },
	{ "int", "MPAimFadeTime", (intptr_t)(&((idPlayer *)0)->MPAimFadeTime), sizeof( ((idPlayer *)0)->MPAimFadeTime ) },
	{ "bool", "MPAimHighlight", (intptr_t)(&((idPlayer *)0)->MPAimHighlight), sizeof( ((idPlayer *)0)->MPAimHighlight ) },
	{ "bool", "isTelefragged", (intptr_t)(&((idPlayer *)0)->isTelefragged), sizeof( ((idPlayer *)0)->isTelefragged ) },
	{ "bool", "selfSmooth", (intptr_t)(&((idPlayer *)0)->selfSmooth), sizeof( ((idPlayer *)0)->selfSmooth ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMover_moveState_t_typeInfo[] = {
	{ "moveStage_t", "stage", (intptr_t)(&((idMover::moveState_t *)0)->stage), sizeof( ((idMover::moveState_t *)0)->stage ) },
	{ "int", "acceleration", (intptr_t)(&((idMover::moveState_t *)0)->acceleration), sizeof( ((idMover::moveState_t *)0)->acceleration ) },
	{ "int", "movetime", (intptr_t)(&((idMover::moveState_t *)0)->movetime), sizeof( ((idMover::moveState_t *)0)->movetime ) },
	{ "int", "deceleration", (intptr_t)(&((idMover::moveState_t *)0)->deceleration), sizeof( ((idMover::moveState_t *)0)->deceleration ) },
	{ "idVec3", "dir", (intptr_t)(&((idMover::moveState_t *)0)->dir), sizeof( ((idMover::moveState_t *)0)->dir ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMover_rotationState_t_typeInfo[] = {
	{ "moveStage_t", "stage", (intptr_t)(&((idMover::rotationState_t *)0)->stage), sizeof( ((idMover::rotationState_t *)0)->stage ) },
	{ "int", "acceleration", (intptr_t)(&((idMover::rotationState_t *)0)->acceleration), sizeof( ((idMover::rotationState_t *)0)->acceleration ) },
	{ "int", "movetime", (intptr_t)(&((idMover::rotationState_t *)0)->movetime), sizeof( ((idMover::rotationState_t *)0)->movetime ) },
	{ "int", "deceleration", (intptr_t)(&((idMover::rotationState_t *)0)->deceleration), sizeof( ((idMover::rotationState_t *)0)->deceleration ) },
	{ "idAngles", "rot", (intptr_t)(&((idMover::rotationState_t *)0)->rot), sizeof( ((idMover::rotationState_t *)0)->rot ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMover_typeInfo[] = {
	{ "idPhysics_Parametric", "physicsObj", (intptr_t)(&((idMover *)0)->physicsObj), sizeof( ((idMover *)0)->physicsObj ) },
	{ "moveState_t", "move", (intptr_t)(&((idMover *)0)->move), sizeof( ((idMover *)0)->move ) },
	{ ": rotationState_t", "rot", (intptr_t)(&((idMover *)0)->rot), sizeof( ((idMover *)0)->rot ) },
	{ "int", "move_thread", (intptr_t)(&((idMover *)0)->move_thread), sizeof( ((idMover *)0)->move_thread ) },
	{ "int", "rotate_thread", (intptr_t)(&((idMover *)0)->rotate_thread), sizeof( ((idMover *)0)->rotate_thread ) },
	{ "idAngles", "dest_angles", (intptr_t)(&((idMover *)0)->dest_angles), sizeof( ((idMover *)0)->dest_angles ) },
	{ "idAngles", "angle_delta", (intptr_t)(&((idMover *)0)->angle_delta), sizeof( ((idMover *)0)->angle_delta ) },
	{ "idVec3", "dest_position", (intptr_t)(&((idMover *)0)->dest_position), sizeof( ((idMover *)0)->dest_position ) },
	{ "idVec3", "move_delta", (intptr_t)(&((idMover *)0)->move_delta), sizeof( ((idMover *)0)->move_delta ) },
	{ "float", "move_speed", (intptr_t)(&((idMover *)0)->move_speed), sizeof( ((idMover *)0)->move_speed ) },
	{ "int", "move_time", (intptr_t)(&((idMover *)0)->move_time), sizeof( ((idMover *)0)->move_time ) },
	{ "int", "deceltime", (intptr_t)(&((idMover *)0)->deceltime), sizeof( ((idMover *)0)->deceltime ) },
	{ "int", "acceltime", (intptr_t)(&((idMover *)0)->acceltime), sizeof( ((idMover *)0)->acceltime ) },
	{ "bool", "stopRotation", (intptr_t)(&((idMover *)0)->stopRotation), sizeof( ((idMover *)0)->stopRotation ) },
	{ "bool", "useSplineAngles", (intptr_t)(&((idMover *)0)->useSplineAngles), sizeof( ((idMover *)0)->useSplineAngles ) },
	{ "idEntityPtr < idEntity >", "splineEnt", (intptr_t)(&((idMover *)0)->splineEnt), sizeof( ((idMover *)0)->splineEnt ) },
	{ "moverCommand_t", "lastCommand", (intptr_t)(&((idMover *)0)->lastCommand), sizeof( ((idMover *)0)->lastCommand ) },
	{ "float", "damage", (intptr_t)(&((idMover *)0)->damage), sizeof( ((idMover *)0)->damage ) },
	{ "qhandle_t", "areaPortal", (intptr_t)(&((idMover *)0)->areaPortal), sizeof( ((idMover *)0)->areaPortal ) },
	{ "idList < idEntityPtr < idEntity > >", "guiTargets", (intptr_t)(&((idMover *)0)->guiTargets), sizeof( ((idMover *)0)->guiTargets ) },
	{ NULL, 0 }
};

static classVariableInfo_t idSplinePath_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t floorInfo_s_typeInfo[] = {
	{ "idVec3", "pos", (intptr_t)(&((floorInfo_s *)0)->pos), sizeof( ((floorInfo_s *)0)->pos ) },
	{ "idStr", "door", (intptr_t)(&((floorInfo_s *)0)->door), sizeof( ((floorInfo_s *)0)->door ) },
	{ "int", "floor", (intptr_t)(&((floorInfo_s *)0)->floor), sizeof( ((floorInfo_s *)0)->floor ) },
	{ NULL, 0 }
};

static classVariableInfo_t idElevator_typeInfo[] = {
	{ "elevatorState_t", "state", (intptr_t)(&((idElevator *)0)->state), sizeof( ((idElevator *)0)->state ) },
	{ "idList < floorInfo_s >", "floorInfo", (intptr_t)(&((idElevator *)0)->floorInfo), sizeof( ((idElevator *)0)->floorInfo ) },
	{ "int", "currentFloor", (intptr_t)(&((idElevator *)0)->currentFloor), sizeof( ((idElevator *)0)->currentFloor ) },
	{ "int", "pendingFloor", (intptr_t)(&((idElevator *)0)->pendingFloor), sizeof( ((idElevator *)0)->pendingFloor ) },
	{ "int", "lastFloor", (intptr_t)(&((idElevator *)0)->lastFloor), sizeof( ((idElevator *)0)->lastFloor ) },
	{ "bool", "controlsDisabled", (intptr_t)(&((idElevator *)0)->controlsDisabled), sizeof( ((idElevator *)0)->controlsDisabled ) },
	{ "float", "returnTime", (intptr_t)(&((idElevator *)0)->returnTime), sizeof( ((idElevator *)0)->returnTime ) },
	{ "int", "returnFloor", (intptr_t)(&((idElevator *)0)->returnFloor), sizeof( ((idElevator *)0)->returnFloor ) },
	{ "int", "lastTouchTime", (intptr_t)(&((idElevator *)0)->lastTouchTime), sizeof( ((idElevator *)0)->lastTouchTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMover_Binary_typeInfo[] = {
	{ ": idVec3", "pos1", (intptr_t)(&((idMover_Binary *)0)->pos1), sizeof( ((idMover_Binary *)0)->pos1 ) },
	{ "idVec3", "pos2", (intptr_t)(&((idMover_Binary *)0)->pos2), sizeof( ((idMover_Binary *)0)->pos2 ) },
	{ "moverState_t", "moverState", (intptr_t)(&((idMover_Binary *)0)->moverState), sizeof( ((idMover_Binary *)0)->moverState ) },
	{ "idMover_Binary *", "moveMaster", (intptr_t)(&((idMover_Binary *)0)->moveMaster), sizeof( ((idMover_Binary *)0)->moveMaster ) },
	{ "idMover_Binary *", "activateChain", (intptr_t)(&((idMover_Binary *)0)->activateChain), sizeof( ((idMover_Binary *)0)->activateChain ) },
	{ "int", "soundPos1", (intptr_t)(&((idMover_Binary *)0)->soundPos1), sizeof( ((idMover_Binary *)0)->soundPos1 ) },
	{ "int", "sound1to2", (intptr_t)(&((idMover_Binary *)0)->sound1to2), sizeof( ((idMover_Binary *)0)->sound1to2 ) },
	{ "int", "sound2to1", (intptr_t)(&((idMover_Binary *)0)->sound2to1), sizeof( ((idMover_Binary *)0)->sound2to1 ) },
	{ "int", "soundPos2", (intptr_t)(&((idMover_Binary *)0)->soundPos2), sizeof( ((idMover_Binary *)0)->soundPos2 ) },
	{ "int", "soundLoop", (intptr_t)(&((idMover_Binary *)0)->soundLoop), sizeof( ((idMover_Binary *)0)->soundLoop ) },
	{ "float", "wait", (intptr_t)(&((idMover_Binary *)0)->wait), sizeof( ((idMover_Binary *)0)->wait ) },
	{ "float", "damage", (intptr_t)(&((idMover_Binary *)0)->damage), sizeof( ((idMover_Binary *)0)->damage ) },
	{ "int", "duration", (intptr_t)(&((idMover_Binary *)0)->duration), sizeof( ((idMover_Binary *)0)->duration ) },
	{ "int", "accelTime", (intptr_t)(&((idMover_Binary *)0)->accelTime), sizeof( ((idMover_Binary *)0)->accelTime ) },
	{ "int", "decelTime", (intptr_t)(&((idMover_Binary *)0)->decelTime), sizeof( ((idMover_Binary *)0)->decelTime ) },
	{ "idEntityPtr < idEntity >", "activatedBy", (intptr_t)(&((idMover_Binary *)0)->activatedBy), sizeof( ((idMover_Binary *)0)->activatedBy ) },
	{ "int", "stateStartTime", (intptr_t)(&((idMover_Binary *)0)->stateStartTime), sizeof( ((idMover_Binary *)0)->stateStartTime ) },
	{ "idStr", "team", (intptr_t)(&((idMover_Binary *)0)->team), sizeof( ((idMover_Binary *)0)->team ) },
	{ "bool", "enabled", (intptr_t)(&((idMover_Binary *)0)->enabled), sizeof( ((idMover_Binary *)0)->enabled ) },
	{ "int", "move_thread", (intptr_t)(&((idMover_Binary *)0)->move_thread), sizeof( ((idMover_Binary *)0)->move_thread ) },
	{ "int", "updateStatus", (intptr_t)(&((idMover_Binary *)0)->updateStatus), sizeof( ((idMover_Binary *)0)->updateStatus ) },
	{ "idStrList", "buddies", (intptr_t)(&((idMover_Binary *)0)->buddies), sizeof( ((idMover_Binary *)0)->buddies ) },
	{ "idPhysics_Parametric", "physicsObj", (intptr_t)(&((idMover_Binary *)0)->physicsObj), sizeof( ((idMover_Binary *)0)->physicsObj ) },
	{ "qhandle_t", "areaPortal", (intptr_t)(&((idMover_Binary *)0)->areaPortal), sizeof( ((idMover_Binary *)0)->areaPortal ) },
	{ "bool", "blocked", (intptr_t)(&((idMover_Binary *)0)->blocked), sizeof( ((idMover_Binary *)0)->blocked ) },
	{ "bool", "playerOnly", (intptr_t)(&((idMover_Binary *)0)->playerOnly), sizeof( ((idMover_Binary *)0)->playerOnly ) },
	{ "idList < idEntityPtr < idEntity > >", "guiTargets", (intptr_t)(&((idMover_Binary *)0)->guiTargets), sizeof( ((idMover_Binary *)0)->guiTargets ) },
	{ NULL, 0 }
};

static classVariableInfo_t idDoor_typeInfo[] = {
	{ ": float", "triggersize", (intptr_t)(&((idDoor *)0)->triggersize), sizeof( ((idDoor *)0)->triggersize ) },
	{ "bool", "crusher", (intptr_t)(&((idDoor *)0)->crusher), sizeof( ((idDoor *)0)->crusher ) },
	{ "bool", "noTouch", (intptr_t)(&((idDoor *)0)->noTouch), sizeof( ((idDoor *)0)->noTouch ) },
	{ "bool", "aas_area_closed", (intptr_t)(&((idDoor *)0)->aas_area_closed), sizeof( ((idDoor *)0)->aas_area_closed ) },
	{ "idStr", "buddyStr", (intptr_t)(&((idDoor *)0)->buddyStr), sizeof( ((idDoor *)0)->buddyStr ) },
	{ "idClipModel *", "trigger", (intptr_t)(&((idDoor *)0)->trigger), sizeof( ((idDoor *)0)->trigger ) },
	{ "idClipModel *", "sndTrigger", (intptr_t)(&((idDoor *)0)->sndTrigger), sizeof( ((idDoor *)0)->sndTrigger ) },
	{ "int", "nextSndTriggerTime", (intptr_t)(&((idDoor *)0)->nextSndTriggerTime), sizeof( ((idDoor *)0)->nextSndTriggerTime ) },
	{ "idVec3", "localTriggerOrigin", (intptr_t)(&((idDoor *)0)->localTriggerOrigin), sizeof( ((idDoor *)0)->localTriggerOrigin ) },
	{ "idMat3", "localTriggerAxis", (intptr_t)(&((idDoor *)0)->localTriggerAxis), sizeof( ((idDoor *)0)->localTriggerAxis ) },
	{ "idStr", "requires", (intptr_t)(&((idDoor *)0)->requires), sizeof( ((idDoor *)0)->requires ) },
	{ "int", "removeItem", (intptr_t)(&((idDoor *)0)->removeItem), sizeof( ((idDoor *)0)->removeItem ) },
	{ "idStr", "syncLock", (intptr_t)(&((idDoor *)0)->syncLock), sizeof( ((idDoor *)0)->syncLock ) },
	{ "int", "normalAxisIndex", (intptr_t)(&((idDoor *)0)->normalAxisIndex), sizeof( ((idDoor *)0)->normalAxisIndex ) },
	{ "idDoor *", "companionDoor", (intptr_t)(&((idDoor *)0)->companionDoor), sizeof( ((idDoor *)0)->companionDoor ) },
	{ NULL, 0 }
};

static classVariableInfo_t idPlat_typeInfo[] = {
	{ ": idClipModel *", "trigger", (intptr_t)(&((idPlat *)0)->trigger), sizeof( ((idPlat *)0)->trigger ) },
	{ "idVec3", "localTriggerOrigin", (intptr_t)(&((idPlat *)0)->localTriggerOrigin), sizeof( ((idPlat *)0)->localTriggerOrigin ) },
	{ "idMat3", "localTriggerAxis", (intptr_t)(&((idPlat *)0)->localTriggerAxis), sizeof( ((idPlat *)0)->localTriggerAxis ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMover_Periodic_typeInfo[] = {
	{ ": idPhysics_Parametric", "physicsObj", (intptr_t)(&((idMover_Periodic *)0)->physicsObj), sizeof( ((idMover_Periodic *)0)->physicsObj ) },
	{ "float", "damage", (intptr_t)(&((idMover_Periodic *)0)->damage), sizeof( ((idMover_Periodic *)0)->damage ) },
	{ NULL, 0 }
};

static classVariableInfo_t idRotater_typeInfo[] = {
	{ ": idEntityPtr < idEntity >", "activatedBy", (intptr_t)(&((idRotater *)0)->activatedBy), sizeof( ((idRotater *)0)->activatedBy ) },
	{ NULL, 0 }
};

static classVariableInfo_t idBobber_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idPendulum_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idRiser_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idCamera_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idCameraView_typeInfo[] = {
	{ "float", "fov", (intptr_t)(&((idCameraView *)0)->fov), sizeof( ((idCameraView *)0)->fov ) },
	{ "idEntity *", "attachedTo", (intptr_t)(&((idCameraView *)0)->attachedTo), sizeof( ((idCameraView *)0)->attachedTo ) },
	{ "idEntity *", "attachedView", (intptr_t)(&((idCameraView *)0)->attachedView), sizeof( ((idCameraView *)0)->attachedView ) },
	{ NULL, 0 }
};

static classVariableInfo_t cameraFrame_t_typeInfo[] = {
	{ "idCQuat", "q", (intptr_t)(&((cameraFrame_t *)0)->q), sizeof( ((cameraFrame_t *)0)->q ) },
	{ "idVec3", "t", (intptr_t)(&((cameraFrame_t *)0)->t), sizeof( ((cameraFrame_t *)0)->t ) },
	{ "float", "fov", (intptr_t)(&((cameraFrame_t *)0)->fov), sizeof( ((cameraFrame_t *)0)->fov ) },
	{ NULL, 0 }
};

static classVariableInfo_t idCameraAnim_typeInfo[] = {
	{ ": int", "threadNum", (intptr_t)(&((idCameraAnim *)0)->threadNum), sizeof( ((idCameraAnim *)0)->threadNum ) },
	{ "idVec3", "offset", (intptr_t)(&((idCameraAnim *)0)->offset), sizeof( ((idCameraAnim *)0)->offset ) },
	{ "int", "frameRate", (intptr_t)(&((idCameraAnim *)0)->frameRate), sizeof( ((idCameraAnim *)0)->frameRate ) },
	{ "int", "starttime", (intptr_t)(&((idCameraAnim *)0)->starttime), sizeof( ((idCameraAnim *)0)->starttime ) },
	{ "int", "cycle", (intptr_t)(&((idCameraAnim *)0)->cycle), sizeof( ((idCameraAnim *)0)->cycle ) },
	{ "idList < int >", "cameraCuts", (intptr_t)(&((idCameraAnim *)0)->cameraCuts), sizeof( ((idCameraAnim *)0)->cameraCuts ) },
	{ "idList < cameraFrame_t >", "camera", (intptr_t)(&((idCameraAnim *)0)->camera), sizeof( ((idCameraAnim *)0)->camera ) },
	{ "idEntityPtr < idEntity >", "activator", (intptr_t)(&((idCameraAnim *)0)->activator), sizeof( ((idCameraAnim *)0)->activator ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMoveable_typeInfo[] = {
	{ ": idPhysics_RigidBody", "physicsObj", (intptr_t)(&((idMoveable *)0)->physicsObj), sizeof( ((idMoveable *)0)->physicsObj ) },
	{ "idStr", "brokenModel", (intptr_t)(&((idMoveable *)0)->brokenModel), sizeof( ((idMoveable *)0)->brokenModel ) },
	{ "idStr", "damage", (intptr_t)(&((idMoveable *)0)->damage), sizeof( ((idMoveable *)0)->damage ) },
	{ "idStr", "monsterDamage", (intptr_t)(&((idMoveable *)0)->monsterDamage), sizeof( ((idMoveable *)0)->monsterDamage ) },
	{ "idEntity *", "attacker", (intptr_t)(&((idMoveable *)0)->attacker), sizeof( ((idMoveable *)0)->attacker ) },
	{ "idStr", "fxCollide", (intptr_t)(&((idMoveable *)0)->fxCollide), sizeof( ((idMoveable *)0)->fxCollide ) },
	{ "int", "nextCollideFxTime", (intptr_t)(&((idMoveable *)0)->nextCollideFxTime), sizeof( ((idMoveable *)0)->nextCollideFxTime ) },
	{ "float", "minDamageVelocity", (intptr_t)(&((idMoveable *)0)->minDamageVelocity), sizeof( ((idMoveable *)0)->minDamageVelocity ) },
	{ "float", "maxDamageVelocity", (intptr_t)(&((idMoveable *)0)->maxDamageVelocity), sizeof( ((idMoveable *)0)->maxDamageVelocity ) },
	{ "idCurve_Spline < idVec3 > *", "initialSpline", (intptr_t)(&((idMoveable *)0)->initialSpline), sizeof( ((idMoveable *)0)->initialSpline ) },
	{ "idVec3", "initialSplineDir", (intptr_t)(&((idMoveable *)0)->initialSplineDir), sizeof( ((idMoveable *)0)->initialSplineDir ) },
	{ "bool", "explode", (intptr_t)(&((idMoveable *)0)->explode), sizeof( ((idMoveable *)0)->explode ) },
	{ "bool", "unbindOnDeath", (intptr_t)(&((idMoveable *)0)->unbindOnDeath), sizeof( ((idMoveable *)0)->unbindOnDeath ) },
	{ "bool", "allowStep", (intptr_t)(&((idMoveable *)0)->allowStep), sizeof( ((idMoveable *)0)->allowStep ) },
	{ "bool", "canDamage", (intptr_t)(&((idMoveable *)0)->canDamage), sizeof( ((idMoveable *)0)->canDamage ) },
	{ "int", "nextDamageTime", (intptr_t)(&((idMoveable *)0)->nextDamageTime), sizeof( ((idMoveable *)0)->nextDamageTime ) },
	{ "int", "nextSoundTime", (intptr_t)(&((idMoveable *)0)->nextSoundTime), sizeof( ((idMoveable *)0)->nextSoundTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idBarrel_typeInfo[] = {
	{ ": float", "radius", (intptr_t)(&((idBarrel *)0)->radius), sizeof( ((idBarrel *)0)->radius ) },
	{ "int", "barrelAxis", (intptr_t)(&((idBarrel *)0)->barrelAxis), sizeof( ((idBarrel *)0)->barrelAxis ) },
	{ "idVec3", "lastOrigin", (intptr_t)(&((idBarrel *)0)->lastOrigin), sizeof( ((idBarrel *)0)->lastOrigin ) },
	{ "idMat3", "lastAxis", (intptr_t)(&((idBarrel *)0)->lastAxis), sizeof( ((idBarrel *)0)->lastAxis ) },
	{ "float", "additionalRotation", (intptr_t)(&((idBarrel *)0)->additionalRotation), sizeof( ((idBarrel *)0)->additionalRotation ) },
	{ "idMat3", "additionalAxis", (intptr_t)(&((idBarrel *)0)->additionalAxis), sizeof( ((idBarrel *)0)->additionalAxis ) },
	{ NULL, 0 }
};

static classVariableInfo_t idExplodingBarrel_typeInfo[] = {
	{ "explode_state_t", "state", (intptr_t)(&((idExplodingBarrel *)0)->state), sizeof( ((idExplodingBarrel *)0)->state ) },
	{ "idVec3", "spawnOrigin", (intptr_t)(&((idExplodingBarrel *)0)->spawnOrigin), sizeof( ((idExplodingBarrel *)0)->spawnOrigin ) },
	{ "idMat3", "spawnAxis", (intptr_t)(&((idExplodingBarrel *)0)->spawnAxis), sizeof( ((idExplodingBarrel *)0)->spawnAxis ) },
	{ "qhandle_t", "particleModelDefHandle", (intptr_t)(&((idExplodingBarrel *)0)->particleModelDefHandle), sizeof( ((idExplodingBarrel *)0)->particleModelDefHandle ) },
	{ "qhandle_t", "lightDefHandle", (intptr_t)(&((idExplodingBarrel *)0)->lightDefHandle), sizeof( ((idExplodingBarrel *)0)->lightDefHandle ) },
	{ "renderEntity_t", "particleRenderEntity", (intptr_t)(&((idExplodingBarrel *)0)->particleRenderEntity), sizeof( ((idExplodingBarrel *)0)->particleRenderEntity ) },
	{ "renderLight_t", "light", (intptr_t)(&((idExplodingBarrel *)0)->light), sizeof( ((idExplodingBarrel *)0)->light ) },
	{ "int", "particleTime", (intptr_t)(&((idExplodingBarrel *)0)->particleTime), sizeof( ((idExplodingBarrel *)0)->particleTime ) },
	{ "int", "lightTime", (intptr_t)(&((idExplodingBarrel *)0)->lightTime), sizeof( ((idExplodingBarrel *)0)->lightTime ) },
	{ "float", "time", (intptr_t)(&((idExplodingBarrel *)0)->time), sizeof( ((idExplodingBarrel *)0)->time ) },
	{ "bool", "isStable", (intptr_t)(&((idExplodingBarrel *)0)->isStable), sizeof( ((idExplodingBarrel *)0)->isStable ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_Remove_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_Show_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_Damage_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SessionCommand_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_EndLevel_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_WaitForButton_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetGlobalShaderTime_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetShaderParm_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetShaderTime_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_FadeEntity_typeInfo[] = {
	{ ": idVec4", "fadeFrom", (intptr_t)(&((idTarget_FadeEntity *)0)->fadeFrom), sizeof( ((idTarget_FadeEntity *)0)->fadeFrom ) },
	{ "int", "fadeStart", (intptr_t)(&((idTarget_FadeEntity *)0)->fadeStart), sizeof( ((idTarget_FadeEntity *)0)->fadeStart ) },
	{ "int", "fadeEnd", (intptr_t)(&((idTarget_FadeEntity *)0)->fadeEnd), sizeof( ((idTarget_FadeEntity *)0)->fadeEnd ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_LightFadeIn_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_LightFadeOut_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_Give_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_GiveEmail_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetModel_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t SavedGui_t_typeInfo[] = {
	{ "idUserInterface *[1]", "gui", (intptr_t)(&((SavedGui_t *)0)->gui), sizeof( ((SavedGui_t *)0)->gui ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetInfluence_typeInfo[] = {
	{ "idList < int >", "lightList", (intptr_t)(&((idTarget_SetInfluence *)0)->lightList), sizeof( ((idTarget_SetInfluence *)0)->lightList ) },
	{ "idList < int >", "guiList", (intptr_t)(&((idTarget_SetInfluence *)0)->guiList), sizeof( ((idTarget_SetInfluence *)0)->guiList ) },
	{ "idList < int >", "soundList", (intptr_t)(&((idTarget_SetInfluence *)0)->soundList), sizeof( ((idTarget_SetInfluence *)0)->soundList ) },
	{ "idList < int >", "genericList", (intptr_t)(&((idTarget_SetInfluence *)0)->genericList), sizeof( ((idTarget_SetInfluence *)0)->genericList ) },
	{ "float", "flashIn", (intptr_t)(&((idTarget_SetInfluence *)0)->flashIn), sizeof( ((idTarget_SetInfluence *)0)->flashIn ) },
	{ "float", "flashOut", (intptr_t)(&((idTarget_SetInfluence *)0)->flashOut), sizeof( ((idTarget_SetInfluence *)0)->flashOut ) },
	{ "float", "delay", (intptr_t)(&((idTarget_SetInfluence *)0)->delay), sizeof( ((idTarget_SetInfluence *)0)->delay ) },
	{ "idStr", "flashInSound", (intptr_t)(&((idTarget_SetInfluence *)0)->flashInSound), sizeof( ((idTarget_SetInfluence *)0)->flashInSound ) },
	{ "idStr", "flashOutSound", (intptr_t)(&((idTarget_SetInfluence *)0)->flashOutSound), sizeof( ((idTarget_SetInfluence *)0)->flashOutSound ) },
	{ "idEntity *", "switchToCamera", (intptr_t)(&((idTarget_SetInfluence *)0)->switchToCamera), sizeof( ((idTarget_SetInfluence *)0)->switchToCamera ) },
	{ "idInterpolate < float >", "fovSetting", (intptr_t)(&((idTarget_SetInfluence *)0)->fovSetting), sizeof( ((idTarget_SetInfluence *)0)->fovSetting ) },
	{ "bool", "soundFaded", (intptr_t)(&((idTarget_SetInfluence *)0)->soundFaded), sizeof( ((idTarget_SetInfluence *)0)->soundFaded ) },
	{ "bool", "restoreOnTrigger", (intptr_t)(&((idTarget_SetInfluence *)0)->restoreOnTrigger), sizeof( ((idTarget_SetInfluence *)0)->restoreOnTrigger ) },
	{ "idList < SavedGui_t >", "savedGuiList", (intptr_t)(&((idTarget_SetInfluence *)0)->savedGuiList), sizeof( ((idTarget_SetInfluence *)0)->savedGuiList ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetKeyVal_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetFov_typeInfo[] = {
	{ ": idInterpolate < int >", "fovSetting", (intptr_t)(&((idTarget_SetFov *)0)->fovSetting), sizeof( ((idTarget_SetFov *)0)->fovSetting ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_SetPrimaryObjective_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_LockDoor_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_CallObjectFunction_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_EnableLevelWeapons_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_Tip_typeInfo[] = {
	{ ": idVec3", "playerPos", (intptr_t)(&((idTarget_Tip *)0)->playerPos), sizeof( ((idTarget_Tip *)0)->playerPos ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_GiveSecurity_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_RemoveWeapons_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_LevelTrigger_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_EnableStamina_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTarget_FadeSoundClass_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_typeInfo[] = {
	{ "const function_t *", "scriptFunction", (intptr_t)(&((idTrigger *)0)->scriptFunction), sizeof( ((idTrigger *)0)->scriptFunction ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_Multi_typeInfo[] = {
	{ ": float", "wait", (intptr_t)(&((idTrigger_Multi *)0)->wait), sizeof( ((idTrigger_Multi *)0)->wait ) },
	{ "float", "random", (intptr_t)(&((idTrigger_Multi *)0)->random), sizeof( ((idTrigger_Multi *)0)->random ) },
	{ "float", "delay", (intptr_t)(&((idTrigger_Multi *)0)->delay), sizeof( ((idTrigger_Multi *)0)->delay ) },
	{ "float", "random_delay", (intptr_t)(&((idTrigger_Multi *)0)->random_delay), sizeof( ((idTrigger_Multi *)0)->random_delay ) },
	{ "int", "nextTriggerTime", (intptr_t)(&((idTrigger_Multi *)0)->nextTriggerTime), sizeof( ((idTrigger_Multi *)0)->nextTriggerTime ) },
	{ "idStr", "requires", (intptr_t)(&((idTrigger_Multi *)0)->requires), sizeof( ((idTrigger_Multi *)0)->requires ) },
	{ "int", "removeItem", (intptr_t)(&((idTrigger_Multi *)0)->removeItem), sizeof( ((idTrigger_Multi *)0)->removeItem ) },
	{ "bool", "touchClient", (intptr_t)(&((idTrigger_Multi *)0)->touchClient), sizeof( ((idTrigger_Multi *)0)->touchClient ) },
	{ "bool", "touchOther", (intptr_t)(&((idTrigger_Multi *)0)->touchOther), sizeof( ((idTrigger_Multi *)0)->touchOther ) },
	{ "bool", "triggerFirst", (intptr_t)(&((idTrigger_Multi *)0)->triggerFirst), sizeof( ((idTrigger_Multi *)0)->triggerFirst ) },
	{ "bool", "triggerWithSelf", (intptr_t)(&((idTrigger_Multi *)0)->triggerWithSelf), sizeof( ((idTrigger_Multi *)0)->triggerWithSelf ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_EntityName_typeInfo[] = {
	{ ": float", "wait", (intptr_t)(&((idTrigger_EntityName *)0)->wait), sizeof( ((idTrigger_EntityName *)0)->wait ) },
	{ "float", "random", (intptr_t)(&((idTrigger_EntityName *)0)->random), sizeof( ((idTrigger_EntityName *)0)->random ) },
	{ "float", "delay", (intptr_t)(&((idTrigger_EntityName *)0)->delay), sizeof( ((idTrigger_EntityName *)0)->delay ) },
	{ "float", "random_delay", (intptr_t)(&((idTrigger_EntityName *)0)->random_delay), sizeof( ((idTrigger_EntityName *)0)->random_delay ) },
	{ "int", "nextTriggerTime", (intptr_t)(&((idTrigger_EntityName *)0)->nextTriggerTime), sizeof( ((idTrigger_EntityName *)0)->nextTriggerTime ) },
	{ "bool", "triggerFirst", (intptr_t)(&((idTrigger_EntityName *)0)->triggerFirst), sizeof( ((idTrigger_EntityName *)0)->triggerFirst ) },
	{ "idStr", "entityName", (intptr_t)(&((idTrigger_EntityName *)0)->entityName), sizeof( ((idTrigger_EntityName *)0)->entityName ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_Timer_typeInfo[] = {
	{ ": float", "random", (intptr_t)(&((idTrigger_Timer *)0)->random), sizeof( ((idTrigger_Timer *)0)->random ) },
	{ "float", "wait", (intptr_t)(&((idTrigger_Timer *)0)->wait), sizeof( ((idTrigger_Timer *)0)->wait ) },
	{ "bool", "on", (intptr_t)(&((idTrigger_Timer *)0)->on), sizeof( ((idTrigger_Timer *)0)->on ) },
	{ "float", "delay", (intptr_t)(&((idTrigger_Timer *)0)->delay), sizeof( ((idTrigger_Timer *)0)->delay ) },
	{ "idStr", "onName", (intptr_t)(&((idTrigger_Timer *)0)->onName), sizeof( ((idTrigger_Timer *)0)->onName ) },
	{ "idStr", "offName", (intptr_t)(&((idTrigger_Timer *)0)->offName), sizeof( ((idTrigger_Timer *)0)->offName ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_Count_typeInfo[] = {
	{ ": int", "goal", (intptr_t)(&((idTrigger_Count *)0)->goal), sizeof( ((idTrigger_Count *)0)->goal ) },
	{ "int", "count", (intptr_t)(&((idTrigger_Count *)0)->count), sizeof( ((idTrigger_Count *)0)->count ) },
	{ "float", "delay", (intptr_t)(&((idTrigger_Count *)0)->delay), sizeof( ((idTrigger_Count *)0)->delay ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_Hurt_typeInfo[] = {
	{ ": bool", "on", (intptr_t)(&((idTrigger_Hurt *)0)->on), sizeof( ((idTrigger_Hurt *)0)->on ) },
	{ "float", "delay", (intptr_t)(&((idTrigger_Hurt *)0)->delay), sizeof( ((idTrigger_Hurt *)0)->delay ) },
	{ "int", "nextTime", (intptr_t)(&((idTrigger_Hurt *)0)->nextTime), sizeof( ((idTrigger_Hurt *)0)->nextTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_Fade_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_Touch_typeInfo[] = {
	{ ": idClipModel *", "clipModel", (intptr_t)(&((idTrigger_Touch *)0)->clipModel), sizeof( ((idTrigger_Touch *)0)->clipModel ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTrigger_Flag_typeInfo[] = {
	{ ": int", "team", (intptr_t)(&((idTrigger_Flag *)0)->team), sizeof( ((idTrigger_Flag *)0)->team ) },
	{ "bool", "player", (intptr_t)(&((idTrigger_Flag *)0)->player), sizeof( ((idTrigger_Flag *)0)->player ) },
	{ "const idEventDef *", "eventFlag", (intptr_t)(&((idTrigger_Flag *)0)->eventFlag), sizeof( ((idTrigger_Flag *)0)->eventFlag ) },
	{ NULL, 0 }
};

static classVariableInfo_t idSound_typeInfo[] = {
	{ ": float", "lastSoundVol", (intptr_t)(&((idSound *)0)->lastSoundVol), sizeof( ((idSound *)0)->lastSoundVol ) },
	{ "float", "soundVol", (intptr_t)(&((idSound *)0)->soundVol), sizeof( ((idSound *)0)->soundVol ) },
	{ "float", "random", (intptr_t)(&((idSound *)0)->random), sizeof( ((idSound *)0)->random ) },
	{ "float", "wait", (intptr_t)(&((idSound *)0)->wait), sizeof( ((idSound *)0)->wait ) },
	{ "bool", "timerOn", (intptr_t)(&((idSound *)0)->timerOn), sizeof( ((idSound *)0)->timerOn ) },
	{ "idVec3", "shakeTranslate", (intptr_t)(&((idSound *)0)->shakeTranslate), sizeof( ((idSound *)0)->shakeTranslate ) },
	{ "idAngles", "shakeRotate", (intptr_t)(&((idSound *)0)->shakeRotate), sizeof( ((idSound *)0)->shakeRotate ) },
	{ "int", "playingUntilTime", (intptr_t)(&((idSound *)0)->playingUntilTime), sizeof( ((idSound *)0)->playingUntilTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t idFXLocalAction_typeInfo[] = {
	{ "renderLight_t", "renderLight", (intptr_t)(&((idFXLocalAction *)0)->renderLight), sizeof( ((idFXLocalAction *)0)->renderLight ) },
	{ "qhandle_t", "lightDefHandle", (intptr_t)(&((idFXLocalAction *)0)->lightDefHandle), sizeof( ((idFXLocalAction *)0)->lightDefHandle ) },
	{ "renderEntity_t", "renderEntity", (intptr_t)(&((idFXLocalAction *)0)->renderEntity), sizeof( ((idFXLocalAction *)0)->renderEntity ) },
	{ "int", "modelDefHandle", (intptr_t)(&((idFXLocalAction *)0)->modelDefHandle), sizeof( ((idFXLocalAction *)0)->modelDefHandle ) },
	{ "float", "delay", (intptr_t)(&((idFXLocalAction *)0)->delay), sizeof( ((idFXLocalAction *)0)->delay ) },
	{ "int", "particleSystem", (intptr_t)(&((idFXLocalAction *)0)->particleSystem), sizeof( ((idFXLocalAction *)0)->particleSystem ) },
	{ "int", "start", (intptr_t)(&((idFXLocalAction *)0)->start), sizeof( ((idFXLocalAction *)0)->start ) },
	{ "bool", "soundStarted", (intptr_t)(&((idFXLocalAction *)0)->soundStarted), sizeof( ((idFXLocalAction *)0)->soundStarted ) },
	{ "bool", "shakeStarted", (intptr_t)(&((idFXLocalAction *)0)->shakeStarted), sizeof( ((idFXLocalAction *)0)->shakeStarted ) },
	{ "bool", "decalDropped", (intptr_t)(&((idFXLocalAction *)0)->decalDropped), sizeof( ((idFXLocalAction *)0)->decalDropped ) },
	{ "bool", "launched", (intptr_t)(&((idFXLocalAction *)0)->launched), sizeof( ((idFXLocalAction *)0)->launched ) },
	{ NULL, 0 }
};

static classVariableInfo_t idEntityFx_typeInfo[] = {
	{ "int", "started", (intptr_t)(&((idEntityFx *)0)->started), sizeof( ((idEntityFx *)0)->started ) },
	{ "int", "nextTriggerTime", (intptr_t)(&((idEntityFx *)0)->nextTriggerTime), sizeof( ((idEntityFx *)0)->nextTriggerTime ) },
	{ "const idDeclFX *", "fxEffect", (intptr_t)(&((idEntityFx *)0)->fxEffect), sizeof( ((idEntityFx *)0)->fxEffect ) },
	{ "idList < idFXLocalAction >", "actions", (intptr_t)(&((idEntityFx *)0)->actions), sizeof( ((idEntityFx *)0)->actions ) },
	{ "idStr", "systemName", (intptr_t)(&((idEntityFx *)0)->systemName), sizeof( ((idEntityFx *)0)->systemName ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTeleporter_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idSecurityCamera_typeInfo[] = {
	{ "float", "angle", (intptr_t)(&((idSecurityCamera *)0)->angle), sizeof( ((idSecurityCamera *)0)->angle ) },
	{ "float", "sweepAngle", (intptr_t)(&((idSecurityCamera *)0)->sweepAngle), sizeof( ((idSecurityCamera *)0)->sweepAngle ) },
	{ "int", "modelAxis", (intptr_t)(&((idSecurityCamera *)0)->modelAxis), sizeof( ((idSecurityCamera *)0)->modelAxis ) },
	{ "bool", "flipAxis", (intptr_t)(&((idSecurityCamera *)0)->flipAxis), sizeof( ((idSecurityCamera *)0)->flipAxis ) },
	{ "float", "scanDist", (intptr_t)(&((idSecurityCamera *)0)->scanDist), sizeof( ((idSecurityCamera *)0)->scanDist ) },
	{ "float", "scanFov", (intptr_t)(&((idSecurityCamera *)0)->scanFov), sizeof( ((idSecurityCamera *)0)->scanFov ) },
	{ "float", "sweepStart", (intptr_t)(&((idSecurityCamera *)0)->sweepStart), sizeof( ((idSecurityCamera *)0)->sweepStart ) },
	{ "float", "sweepEnd", (intptr_t)(&((idSecurityCamera *)0)->sweepEnd), sizeof( ((idSecurityCamera *)0)->sweepEnd ) },
	{ "bool", "negativeSweep", (intptr_t)(&((idSecurityCamera *)0)->negativeSweep), sizeof( ((idSecurityCamera *)0)->negativeSweep ) },
	{ "bool", "sweeping", (intptr_t)(&((idSecurityCamera *)0)->sweeping), sizeof( ((idSecurityCamera *)0)->sweeping ) },
	{ "int", "alertMode", (intptr_t)(&((idSecurityCamera *)0)->alertMode), sizeof( ((idSecurityCamera *)0)->alertMode ) },
	{ "float", "stopSweeping", (intptr_t)(&((idSecurityCamera *)0)->stopSweeping), sizeof( ((idSecurityCamera *)0)->stopSweeping ) },
	{ "float", "scanFovCos", (intptr_t)(&((idSecurityCamera *)0)->scanFovCos), sizeof( ((idSecurityCamera *)0)->scanFovCos ) },
	{ "idVec3", "viewOffset", (intptr_t)(&((idSecurityCamera *)0)->viewOffset), sizeof( ((idSecurityCamera *)0)->viewOffset ) },
	{ "int", "pvsArea", (intptr_t)(&((idSecurityCamera *)0)->pvsArea), sizeof( ((idSecurityCamera *)0)->pvsArea ) },
	{ "idPhysics_RigidBody", "physicsObj", (intptr_t)(&((idSecurityCamera *)0)->physicsObj), sizeof( ((idSecurityCamera *)0)->physicsObj ) },
	{ "idTraceModel", "trm", (intptr_t)(&((idSecurityCamera *)0)->trm), sizeof( ((idSecurityCamera *)0)->trm ) },
	{ NULL, 0 }
};

static classVariableInfo_t shard_t_typeInfo[] = {
	{ "idClipModel *", "clipModel", (intptr_t)(&((shard_t *)0)->clipModel), sizeof( ((shard_t *)0)->clipModel ) },
	{ "idFixedWinding", "winding", (intptr_t)(&((shard_t *)0)->winding), sizeof( ((shard_t *)0)->winding ) },
	{ "idList < idFixedWinding * >", "decals", (intptr_t)(&((shard_t *)0)->decals), sizeof( ((shard_t *)0)->decals ) },
	{ "idList < bool >", "edgeHasNeighbour", (intptr_t)(&((shard_t *)0)->edgeHasNeighbour), sizeof( ((shard_t *)0)->edgeHasNeighbour ) },
	{ "idList < struct shard_s * >", "neighbours", (intptr_t)(&((shard_t *)0)->neighbours), sizeof( ((shard_t *)0)->neighbours ) },
	{ "idPhysics_RigidBody", "physicsObj", (intptr_t)(&((shard_t *)0)->physicsObj), sizeof( ((shard_t *)0)->physicsObj ) },
	{ "int", "droppedTime", (intptr_t)(&((shard_t *)0)->droppedTime), sizeof( ((shard_t *)0)->droppedTime ) },
	{ "bool", "atEdge", (intptr_t)(&((shard_t *)0)->atEdge), sizeof( ((shard_t *)0)->atEdge ) },
	{ "int", "islandNum", (intptr_t)(&((shard_t *)0)->islandNum), sizeof( ((shard_t *)0)->islandNum ) },
	{ NULL, 0 }
};

static classVariableInfo_t idBrittleFracture_typeInfo[] = {
	{ ": const idMaterial *", "material", (intptr_t)(&((idBrittleFracture *)0)->material), sizeof( ((idBrittleFracture *)0)->material ) },
	{ "const idMaterial *", "decalMaterial", (intptr_t)(&((idBrittleFracture *)0)->decalMaterial), sizeof( ((idBrittleFracture *)0)->decalMaterial ) },
	{ "float", "decalSize", (intptr_t)(&((idBrittleFracture *)0)->decalSize), sizeof( ((idBrittleFracture *)0)->decalSize ) },
	{ "float", "maxShardArea", (intptr_t)(&((idBrittleFracture *)0)->maxShardArea), sizeof( ((idBrittleFracture *)0)->maxShardArea ) },
	{ "float", "maxShatterRadius", (intptr_t)(&((idBrittleFracture *)0)->maxShatterRadius), sizeof( ((idBrittleFracture *)0)->maxShatterRadius ) },
	{ "float", "minShatterRadius", (intptr_t)(&((idBrittleFracture *)0)->minShatterRadius), sizeof( ((idBrittleFracture *)0)->minShatterRadius ) },
	{ "float", "linearVelocityScale", (intptr_t)(&((idBrittleFracture *)0)->linearVelocityScale), sizeof( ((idBrittleFracture *)0)->linearVelocityScale ) },
	{ "float", "angularVelocityScale", (intptr_t)(&((idBrittleFracture *)0)->angularVelocityScale), sizeof( ((idBrittleFracture *)0)->angularVelocityScale ) },
	{ "float", "shardMass", (intptr_t)(&((idBrittleFracture *)0)->shardMass), sizeof( ((idBrittleFracture *)0)->shardMass ) },
	{ "float", "density", (intptr_t)(&((idBrittleFracture *)0)->density), sizeof( ((idBrittleFracture *)0)->density ) },
	{ "float", "friction", (intptr_t)(&((idBrittleFracture *)0)->friction), sizeof( ((idBrittleFracture *)0)->friction ) },
	{ "float", "bouncyness", (intptr_t)(&((idBrittleFracture *)0)->bouncyness), sizeof( ((idBrittleFracture *)0)->bouncyness ) },
	{ "idStr", "fxFracture", (intptr_t)(&((idBrittleFracture *)0)->fxFracture), sizeof( ((idBrittleFracture *)0)->fxFracture ) },
	{ "bool", "isXraySurface", (intptr_t)(&((idBrittleFracture *)0)->isXraySurface), sizeof( ((idBrittleFracture *)0)->isXraySurface ) },
	{ "idPhysics_StaticMulti", "physicsObj", (intptr_t)(&((idBrittleFracture *)0)->physicsObj), sizeof( ((idBrittleFracture *)0)->physicsObj ) },
	{ "idList < shard_t * >", "shards", (intptr_t)(&((idBrittleFracture *)0)->shards), sizeof( ((idBrittleFracture *)0)->shards ) },
	{ "idBounds", "bounds", (intptr_t)(&((idBrittleFracture *)0)->bounds), sizeof( ((idBrittleFracture *)0)->bounds ) },
	{ "bool", "disableFracture", (intptr_t)(&((idBrittleFracture *)0)->disableFracture), sizeof( ((idBrittleFracture *)0)->disableFracture ) },
	{ "mutable int", "lastRenderEntityUpdate", (intptr_t)(&((idBrittleFracture *)0)->lastRenderEntityUpdate), sizeof( ((idBrittleFracture *)0)->lastRenderEntityUpdate ) },
	{ "mutable bool", "changed", (intptr_t)(&((idBrittleFracture *)0)->changed), sizeof( ((idBrittleFracture *)0)->changed ) },
	{ NULL, 0 }
};

static classVariableInfo_t dnWeaponMightyFoot_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t dnWeaponPistol_typeInfo[] = {
	{ ": float", "spread", (intptr_t)(&((dnWeaponPistol *)0)->spread), sizeof( ((dnWeaponPistol *)0)->spread ) },
	{ "const idSoundShader *", "fireSound", (intptr_t)(&((dnWeaponPistol *)0)->fireSound), sizeof( ((dnWeaponPistol *)0)->fireSound ) },
	{ "const idSoundShader *", "snd_lowammo", (intptr_t)(&((dnWeaponPistol *)0)->snd_lowammo), sizeof( ((dnWeaponPistol *)0)->snd_lowammo ) },
	{ NULL, 0 }
};

static classVariableInfo_t dnWeaponShotgun_typeInfo[] = {
	{ ": float", "spread", (intptr_t)(&((dnWeaponShotgun *)0)->spread), sizeof( ((dnWeaponShotgun *)0)->spread ) },
	{ "const idSoundShader *", "fireSound", (intptr_t)(&((dnWeaponShotgun *)0)->fireSound), sizeof( ((dnWeaponShotgun *)0)->fireSound ) },
	{ "const idSoundShader *", "snd_lowammo", (intptr_t)(&((dnWeaponShotgun *)0)->snd_lowammo), sizeof( ((dnWeaponShotgun *)0)->snd_lowammo ) },
	{ NULL, 0 }
};

static classVariableInfo_t dnWeaponM16_typeInfo[] = {
	{ ": float", "spread", (intptr_t)(&((dnWeaponM16 *)0)->spread), sizeof( ((dnWeaponM16 *)0)->spread ) },
	{ "const idSoundShader *", "fireSound", (intptr_t)(&((dnWeaponM16 *)0)->fireSound), sizeof( ((dnWeaponM16 *)0)->fireSound ) },
	{ "const idSoundShader *", "snd_lowammo", (intptr_t)(&((dnWeaponM16 *)0)->snd_lowammo), sizeof( ((dnWeaponM16 *)0)->snd_lowammo ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnItem_typeInfo[] = {
	{ "idVec3", "orgOrigin", (intptr_t)(&((DnItem *)0)->orgOrigin), sizeof( ((DnItem *)0)->orgOrigin ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnItemShotgun_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t idTestModel_typeInfo[] = {
	{ ": idEntityPtr < idEntity >", "head", (intptr_t)(&((idTestModel *)0)->head), sizeof( ((idTestModel *)0)->head ) },
	{ "idAnimator *", "headAnimator", (intptr_t)(&((idTestModel *)0)->headAnimator), sizeof( ((idTestModel *)0)->headAnimator ) },
	{ "idAnim", "customAnim", (intptr_t)(&((idTestModel *)0)->customAnim), sizeof( ((idTestModel *)0)->customAnim ) },
	{ "idPhysics_Parametric", "physicsObj", (intptr_t)(&((idTestModel *)0)->physicsObj), sizeof( ((idTestModel *)0)->physicsObj ) },
	{ "idStr", "animname", (intptr_t)(&((idTestModel *)0)->animname), sizeof( ((idTestModel *)0)->animname ) },
	{ "int", "anim", (intptr_t)(&((idTestModel *)0)->anim), sizeof( ((idTestModel *)0)->anim ) },
	{ "int", "headAnim", (intptr_t)(&((idTestModel *)0)->headAnim), sizeof( ((idTestModel *)0)->headAnim ) },
	{ "int", "mode", (intptr_t)(&((idTestModel *)0)->mode), sizeof( ((idTestModel *)0)->mode ) },
	{ "int", "frame", (intptr_t)(&((idTestModel *)0)->frame), sizeof( ((idTestModel *)0)->frame ) },
	{ "int", "starttime", (intptr_t)(&((idTestModel *)0)->starttime), sizeof( ((idTestModel *)0)->starttime ) },
	{ "int", "animtime", (intptr_t)(&((idTestModel *)0)->animtime), sizeof( ((idTestModel *)0)->animtime ) },
	{ "idList < copyJoints_t >", "copyJoints", (intptr_t)(&((idTestModel *)0)->copyJoints), sizeof( ((idTestModel *)0)->copyJoints ) },
	{ NULL, 0 }
};

static classVariableInfo_t idMoveState_typeInfo[] = {
	{ "moveType_t", "moveType", (intptr_t)(&((idMoveState *)0)->moveType), sizeof( ((idMoveState *)0)->moveType ) },
	{ "moveCommand_t", "moveCommand", (intptr_t)(&((idMoveState *)0)->moveCommand), sizeof( ((idMoveState *)0)->moveCommand ) },
	{ "moveStatus_t", "moveStatus", (intptr_t)(&((idMoveState *)0)->moveStatus), sizeof( ((idMoveState *)0)->moveStatus ) },
	{ "idVec3", "moveDest", (intptr_t)(&((idMoveState *)0)->moveDest), sizeof( ((idMoveState *)0)->moveDest ) },
	{ "idVec3", "moveDir", (intptr_t)(&((idMoveState *)0)->moveDir), sizeof( ((idMoveState *)0)->moveDir ) },
	{ "idEntityPtr < idEntity >", "goalEntity", (intptr_t)(&((idMoveState *)0)->goalEntity), sizeof( ((idMoveState *)0)->goalEntity ) },
	{ "idVec3", "goalEntityOrigin", (intptr_t)(&((idMoveState *)0)->goalEntityOrigin), sizeof( ((idMoveState *)0)->goalEntityOrigin ) },
	{ "int", "toAreaNum", (intptr_t)(&((idMoveState *)0)->toAreaNum), sizeof( ((idMoveState *)0)->toAreaNum ) },
	{ "int", "startTime", (intptr_t)(&((idMoveState *)0)->startTime), sizeof( ((idMoveState *)0)->startTime ) },
	{ "int", "duration", (intptr_t)(&((idMoveState *)0)->duration), sizeof( ((idMoveState *)0)->duration ) },
	{ "float", "speed", (intptr_t)(&((idMoveState *)0)->speed), sizeof( ((idMoveState *)0)->speed ) },
	{ "float", "range", (intptr_t)(&((idMoveState *)0)->range), sizeof( ((idMoveState *)0)->range ) },
	{ "float", "wanderYaw", (intptr_t)(&((idMoveState *)0)->wanderYaw), sizeof( ((idMoveState *)0)->wanderYaw ) },
	{ "int", "nextWanderTime", (intptr_t)(&((idMoveState *)0)->nextWanderTime), sizeof( ((idMoveState *)0)->nextWanderTime ) },
	{ "int", "blockTime", (intptr_t)(&((idMoveState *)0)->blockTime), sizeof( ((idMoveState *)0)->blockTime ) },
	{ "idEntityPtr < idEntity >", "obstacle", (intptr_t)(&((idMoveState *)0)->obstacle), sizeof( ((idMoveState *)0)->obstacle ) },
	{ "idVec3", "lastMoveOrigin", (intptr_t)(&((idMoveState *)0)->lastMoveOrigin), sizeof( ((idMoveState *)0)->lastMoveOrigin ) },
	{ "int", "lastMoveTime", (intptr_t)(&((idMoveState *)0)->lastMoveTime), sizeof( ((idMoveState *)0)->lastMoveTime ) },
	{ "int", "anim", (intptr_t)(&((idMoveState *)0)->anim), sizeof( ((idMoveState *)0)->anim ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnRand_typeInfo[] = {
	{ ": int", "randomseed", (intptr_t)(&((DnRand *)0)->randomseed), sizeof( ((DnRand *)0)->randomseed ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnAI_typeInfo[] = {
	{ "idActor *", "target", (intptr_t)(&((DnAI *)0)->target), sizeof( ((DnAI *)0)->target ) },
	{ "idVec3", "targetLastSeenLocation", (intptr_t)(&((DnAI *)0)->targetLastSeenLocation), sizeof( ((DnAI *)0)->targetLastSeenLocation ) },
	{ "bool", "isTargetVisible", (intptr_t)(&((DnAI *)0)->isTargetVisible), sizeof( ((DnAI *)0)->isTargetVisible ) },
	{ "idPhysics_Monster", "physicsObj", (intptr_t)(&((DnAI *)0)->physicsObj), sizeof( ((DnAI *)0)->physicsObj ) },
	{ "float", "ideal_yaw", (intptr_t)(&((DnAI *)0)->ideal_yaw), sizeof( ((DnAI *)0)->ideal_yaw ) },
	{ "float", "current_yaw", (intptr_t)(&((DnAI *)0)->current_yaw), sizeof( ((DnAI *)0)->current_yaw ) },
	{ "float", "turnRate", (intptr_t)(&((DnAI *)0)->turnRate), sizeof( ((DnAI *)0)->turnRate ) },
	{ "float", "turnVel", (intptr_t)(&((DnAI *)0)->turnVel), sizeof( ((DnAI *)0)->turnVel ) },
	{ "idList < idVec3 >", "pathWaypoints", (intptr_t)(&((DnAI *)0)->pathWaypoints), sizeof( ((DnAI *)0)->pathWaypoints ) },
	{ "int", "waypointId", (intptr_t)(&((DnAI *)0)->waypointId), sizeof( ((DnAI *)0)->waypointId ) },
	{ "bool", "AI_PAIN", (intptr_t)(&((DnAI *)0)->AI_PAIN), sizeof( ((DnAI *)0)->AI_PAIN ) },
	{ "idMoveState", "move", (intptr_t)(&((DnAI *)0)->move), sizeof( ((DnAI *)0)->move ) },
	{ "bool", "AI_ONGROUND", (intptr_t)(&((DnAI *)0)->AI_ONGROUND), sizeof( ((DnAI *)0)->AI_ONGROUND ) },
	{ "bool", "AI_BLOCKED", (intptr_t)(&((DnAI *)0)->AI_BLOCKED), sizeof( ((DnAI *)0)->AI_BLOCKED ) },
	{ "idStr", "currentAnimation", (intptr_t)(&((DnAI *)0)->currentAnimation), sizeof( ((DnAI *)0)->currentAnimation ) },
	{ ": int", "EgoKillValue", (intptr_t)(&((DnAI *)0)->EgoKillValue), sizeof( ((DnAI *)0)->EgoKillValue ) },
	{ "bool", "startedDeath", (intptr_t)(&((DnAI *)0)->startedDeath), sizeof( ((DnAI *)0)->startedDeath ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnPigcop_typeInfo[] = {
	{ ": const idSoundShader *", "pig_roam1", (intptr_t)(&((DnPigcop *)0)->pig_roam1), sizeof( ((DnPigcop *)0)->pig_roam1 ) },
	{ "const idSoundShader *", "pig_roam2", (intptr_t)(&((DnPigcop *)0)->pig_roam2), sizeof( ((DnPigcop *)0)->pig_roam2 ) },
	{ "const idSoundShader *", "pig_roam3", (intptr_t)(&((DnPigcop *)0)->pig_roam3), sizeof( ((DnPigcop *)0)->pig_roam3 ) },
	{ "const idSoundShader *", "pig_awake", (intptr_t)(&((DnPigcop *)0)->pig_awake), sizeof( ((DnPigcop *)0)->pig_awake ) },
	{ "const idSoundShader *", "fire_sound", (intptr_t)(&((DnPigcop *)0)->fire_sound), sizeof( ((DnPigcop *)0)->fire_sound ) },
	{ "const idSoundShader *", "death_sound", (intptr_t)(&((DnPigcop *)0)->death_sound), sizeof( ((DnPigcop *)0)->death_sound ) },
	{ "DnMeshComponent", "shotgunMeshComponent", (intptr_t)(&((DnPigcop *)0)->shotgunMeshComponent), sizeof( ((DnPigcop *)0)->shotgunMeshComponent ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnLiztroop_typeInfo[] = {
	{ ": const idSoundShader *", "troop_awake", (intptr_t)(&((DnLiztroop *)0)->troop_awake), sizeof( ((DnLiztroop *)0)->troop_awake ) },
	{ "const idSoundShader *", "fire_sound", (intptr_t)(&((DnLiztroop *)0)->fire_sound), sizeof( ((DnLiztroop *)0)->fire_sound ) },
	{ "const idSoundShader *", "death_sound", (intptr_t)(&((DnLiztroop *)0)->death_sound), sizeof( ((DnLiztroop *)0)->death_sound ) },
	{ NULL, 0 }
};

static classVariableInfo_t DnCivilian_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t DukePlayer_typeInfo[] = {
	{ "const idSoundShader *", "dukeTauntShader", (intptr_t)(&((DukePlayer *)0)->dukeTauntShader), sizeof( ((DukePlayer *)0)->dukeTauntShader ) },
	{ "const idSoundShader *", "dukePainShader", (intptr_t)(&((DukePlayer *)0)->dukePainShader), sizeof( ((DukePlayer *)0)->dukePainShader ) },
	{ "idList < const idSoundShader * >", "dukeJumpSounds", (intptr_t)(&((DukePlayer *)0)->dukeJumpSounds), sizeof( ((DukePlayer *)0)->dukeJumpSounds ) },
	{ "bool", "firstSwearTaunt", (intptr_t)(&((DukePlayer *)0)->firstSwearTaunt), sizeof( ((DukePlayer *)0)->firstSwearTaunt ) },
	{ "float", "bob", (intptr_t)(&((DukePlayer *)0)->bob), sizeof( ((DukePlayer *)0)->bob ) },
	{ "float", "lastAppliedBobCycle", (intptr_t)(&((DukePlayer *)0)->lastAppliedBobCycle), sizeof( ((DukePlayer *)0)->lastAppliedBobCycle ) },
	{ "idStr", "currentAnimation", (intptr_t)(&((DukePlayer *)0)->currentAnimation), sizeof( ((DukePlayer *)0)->currentAnimation ) },
	{ "rvmDeclRenderParam *", "guiCrosshairColorParam", (intptr_t)(&((DukePlayer *)0)->guiCrosshairColorParam), sizeof( ((DukePlayer *)0)->guiCrosshairColorParam ) },
	{ NULL, 0 }
};

static classVariableInfo_t dnDecoration_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t rvClientEntity_typeInfo[] = {
	{ ": int", "entityNumber", (intptr_t)(&((rvClientEntity *)0)->entityNumber), sizeof( ((rvClientEntity *)0)->entityNumber ) },
	{ "idLinkList < rvClientEntity >", "spawnNode", (intptr_t)(&((rvClientEntity *)0)->spawnNode), sizeof( ((rvClientEntity *)0)->spawnNode ) },
	{ "idLinkList < rvClientEntity >", "bindNode", (intptr_t)(&((rvClientEntity *)0)->bindNode), sizeof( ((rvClientEntity *)0)->bindNode ) },
	{ "idDict", "spawnArgs", (intptr_t)(&((rvClientEntity *)0)->spawnArgs), sizeof( ((rvClientEntity *)0)->spawnArgs ) },
	{ ": idPhysics_Static", "defaultPhysicsObj", (intptr_t)(&((rvClientEntity *)0)->defaultPhysicsObj), sizeof( ((rvClientEntity *)0)->defaultPhysicsObj ) },
	{ "idPhysics *", "physics", (intptr_t)(&((rvClientEntity *)0)->physics), sizeof( ((rvClientEntity *)0)->physics ) },
	{ "idVec3", "worldOrigin", (intptr_t)(&((rvClientEntity *)0)->worldOrigin), sizeof( ((rvClientEntity *)0)->worldOrigin ) },
	{ "idVec3", "worldVelocity", (intptr_t)(&((rvClientEntity *)0)->worldVelocity), sizeof( ((rvClientEntity *)0)->worldVelocity ) },
	{ "idMat3", "worldAxis", (intptr_t)(&((rvClientEntity *)0)->worldAxis), sizeof( ((rvClientEntity *)0)->worldAxis ) },
	{ "idVec3", "worldOriginThread", (intptr_t)(&((rvClientEntity *)0)->worldOriginThread), sizeof( ((rvClientEntity *)0)->worldOriginThread ) },
	{ "idVec3", "worldVelocityThread", (intptr_t)(&((rvClientEntity *)0)->worldVelocityThread), sizeof( ((rvClientEntity *)0)->worldVelocityThread ) },
	{ "idMat3", "worldAxisThread", (intptr_t)(&((rvClientEntity *)0)->worldAxisThread), sizeof( ((rvClientEntity *)0)->worldAxisThread ) },
	{ "bool", "hasThreadedResults", (intptr_t)(&((rvClientEntity *)0)->hasThreadedResults), sizeof( ((rvClientEntity *)0)->hasThreadedResults ) },
	{ "idEntity *", "bindMaster", (intptr_t)(&((rvClientEntity *)0)->bindMaster), sizeof( ((rvClientEntity *)0)->bindMaster ) },
	{ "idVec3", "bindOrigin", (intptr_t)(&((rvClientEntity *)0)->bindOrigin), sizeof( ((rvClientEntity *)0)->bindOrigin ) },
	{ "idMat3", "bindAxis", (intptr_t)(&((rvClientEntity *)0)->bindAxis), sizeof( ((rvClientEntity *)0)->bindAxis ) },
	{ "jointHandle_t", "bindJoint", (intptr_t)(&((rvClientEntity *)0)->bindJoint), sizeof( ((rvClientEntity *)0)->bindJoint ) },
	{ "bool", "bindOrientated", (intptr_t)(&((rvClientEntity *)0)->bindOrientated), sizeof( ((rvClientEntity *)0)->bindOrientated ) },
	{ "refSound_t", "refSound", (intptr_t)(&((rvClientEntity *)0)->refSound), sizeof( ((rvClientEntity *)0)->refSound ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvClientPhysics_typeInfo[] = {
	{ "int", "currentEntityNumber", (intptr_t)(&((rvClientPhysics *)0)->currentEntityNumber), sizeof( ((rvClientPhysics *)0)->currentEntityNumber ) },
	{ ": idEntity *", "pushedBindMaster", (intptr_t)(&((rvClientPhysics *)0)->pushedBindMaster), sizeof( ((rvClientPhysics *)0)->pushedBindMaster ) },
	{ "jointHandle_t", "pushedBindJoint", (intptr_t)(&((rvClientPhysics *)0)->pushedBindJoint), sizeof( ((rvClientPhysics *)0)->pushedBindJoint ) },
	{ "idVec3", "pushedOrigin", (intptr_t)(&((rvClientPhysics *)0)->pushedOrigin), sizeof( ((rvClientPhysics *)0)->pushedOrigin ) },
	{ "idMat3", "pushedAxis", (intptr_t)(&((rvClientPhysics *)0)->pushedAxis), sizeof( ((rvClientPhysics *)0)->pushedAxis ) },
	{ "bool", "pushedOrientated", (intptr_t)(&((rvClientPhysics *)0)->pushedOrientated), sizeof( ((rvClientPhysics *)0)->pushedOrientated ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvClientModel_typeInfo[] = {
	{ "renderEntity_t", "renderEntity", (intptr_t)(&((rvClientModel *)0)->renderEntity), sizeof( ((rvClientModel *)0)->renderEntity ) },
	{ "int", "entityDefHandle", (intptr_t)(&((rvClientModel *)0)->entityDefHandle), sizeof( ((rvClientModel *)0)->entityDefHandle ) },
	{ "idStr", "classname", (intptr_t)(&((rvClientModel *)0)->classname), sizeof( ((rvClientModel *)0)->classname ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvAnimatedClientEntity_typeInfo[] = {
	{ ": idAnimator", "animator", (intptr_t)(&((rvAnimatedClientEntity *)0)->animator), sizeof( ((rvAnimatedClientEntity *)0)->animator ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvClientMoveable_typeInfo[] = {
	{ ": renderEntity_t", "renderEntity", (intptr_t)(&((rvClientMoveable *)0)->renderEntity), sizeof( ((rvClientMoveable *)0)->renderEntity ) },
	{ "int", "entityDefHandle", (intptr_t)(&((rvClientMoveable *)0)->entityDefHandle), sizeof( ((rvClientMoveable *)0)->entityDefHandle ) },
	{ "float", "trailAttenuateSpeed", (intptr_t)(&((rvClientMoveable *)0)->trailAttenuateSpeed), sizeof( ((rvClientMoveable *)0)->trailAttenuateSpeed ) },
	{ "idPhysics_RigidBody", "physicsObj", (intptr_t)(&((rvClientMoveable *)0)->physicsObj), sizeof( ((rvClientMoveable *)0)->physicsObj ) },
	{ "int", "bounceSoundTime", (intptr_t)(&((rvClientMoveable *)0)->bounceSoundTime), sizeof( ((rvClientMoveable *)0)->bounceSoundTime ) },
	{ "const idSoundShader *", "bounceSoundShader", (intptr_t)(&((rvClientMoveable *)0)->bounceSoundShader), sizeof( ((rvClientMoveable *)0)->bounceSoundShader ) },
	{ "bool", "mPlayBounceSoundOnce", (intptr_t)(&((rvClientMoveable *)0)->mPlayBounceSoundOnce), sizeof( ((rvClientMoveable *)0)->mPlayBounceSoundOnce ) },
	{ "bool", "mHasBounced", (intptr_t)(&((rvClientMoveable *)0)->mHasBounced), sizeof( ((rvClientMoveable *)0)->mHasBounced ) },
	{ "idInterpolate < float >", "scale", (intptr_t)(&((rvClientMoveable *)0)->scale), sizeof( ((rvClientMoveable *)0)->scale ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvClientAFEntity_typeInfo[] = {
	{ ": idAF", "af", (intptr_t)(&((rvClientAFEntity *)0)->af), sizeof( ((rvClientAFEntity *)0)->af ) },
	{ "idClipModel *", "combatModel", (intptr_t)(&((rvClientAFEntity *)0)->combatModel), sizeof( ((rvClientAFEntity *)0)->combatModel ) },
	{ "int", "combatModelContents", (intptr_t)(&((rvClientAFEntity *)0)->combatModelContents), sizeof( ((rvClientAFEntity *)0)->combatModelContents ) },
	{ "idVec3", "spawnOrigin", (intptr_t)(&((rvClientAFEntity *)0)->spawnOrigin), sizeof( ((rvClientAFEntity *)0)->spawnOrigin ) },
	{ "idMat3", "spawnAxis", (intptr_t)(&((rvClientAFEntity *)0)->spawnAxis), sizeof( ((rvClientAFEntity *)0)->spawnAxis ) },
	{ "int", "nextSoundTime", (intptr_t)(&((rvClientAFEntity *)0)->nextSoundTime), sizeof( ((rvClientAFEntity *)0)->nextSoundTime ) },
	{ NULL, 0 }
};

static classVariableInfo_t rvClientAFAttachment_typeInfo[] = {
	{ ": idAnimatedEntity *", "body", (intptr_t)(&((rvClientAFAttachment *)0)->body), sizeof( ((rvClientAFAttachment *)0)->body ) },
	{ "idClipModel *", "combatModel", (intptr_t)(&((rvClientAFAttachment *)0)->combatModel), sizeof( ((rvClientAFAttachment *)0)->combatModel ) },
	{ "int", "idleAnim", (intptr_t)(&((rvClientAFAttachment *)0)->idleAnim), sizeof( ((rvClientAFAttachment *)0)->idleAnim ) },
	{ "jointHandle_t", "damageJoint", (intptr_t)(&((rvClientAFAttachment *)0)->damageJoint), sizeof( ((rvClientAFAttachment *)0)->damageJoint ) },
	{ "idList < copyJoints_t >", "copyJoints", (intptr_t)(&((rvClientAFAttachment *)0)->copyJoints), sizeof( ((rvClientAFAttachment *)0)->copyJoints ) },
	{ NULL, 0 }
};

static classVariableInfo_t opcode_t_typeInfo[] = {
	{ "char *", "name", (intptr_t)(&((opcode_t *)0)->name), sizeof( ((opcode_t *)0)->name ) },
	{ "char *", "opname", (intptr_t)(&((opcode_t *)0)->opname), sizeof( ((opcode_t *)0)->opname ) },
	{ "int", "priority", (intptr_t)(&((opcode_t *)0)->priority), sizeof( ((opcode_t *)0)->priority ) },
	{ "bool", "rightAssociative", (intptr_t)(&((opcode_t *)0)->rightAssociative), sizeof( ((opcode_t *)0)->rightAssociative ) },
	{ "idVarDef *", "type_a", (intptr_t)(&((opcode_t *)0)->type_a), sizeof( ((opcode_t *)0)->type_a ) },
	{ "idVarDef *", "type_b", (intptr_t)(&((opcode_t *)0)->type_b), sizeof( ((opcode_t *)0)->type_b ) },
	{ "idVarDef *", "type_c", (intptr_t)(&((opcode_t *)0)->type_c), sizeof( ((opcode_t *)0)->type_c ) },
	{ NULL, 0 }
};

static classVariableInfo_t idCompiler_typeInfo[] = {
	{ "idParser", "parser", (intptr_t)(&((idCompiler *)0)->parser), sizeof( ((idCompiler *)0)->parser ) },
	{ "idParser *", "parserPtr", (intptr_t)(&((idCompiler *)0)->parserPtr), sizeof( ((idCompiler *)0)->parserPtr ) },
	{ "idToken", "token", (intptr_t)(&((idCompiler *)0)->token), sizeof( ((idCompiler *)0)->token ) },
	{ "idTypeDef *", "immediateType", (intptr_t)(&((idCompiler *)0)->immediateType), sizeof( ((idCompiler *)0)->immediateType ) },
	{ "eval_t", "immediate", (intptr_t)(&((idCompiler *)0)->immediate), sizeof( ((idCompiler *)0)->immediate ) },
	{ "bool", "eof", (intptr_t)(&((idCompiler *)0)->eof), sizeof( ((idCompiler *)0)->eof ) },
	{ "bool", "console", (intptr_t)(&((idCompiler *)0)->console), sizeof( ((idCompiler *)0)->console ) },
	{ "bool", "callthread", (intptr_t)(&((idCompiler *)0)->callthread), sizeof( ((idCompiler *)0)->callthread ) },
	{ "int", "braceDepth", (intptr_t)(&((idCompiler *)0)->braceDepth), sizeof( ((idCompiler *)0)->braceDepth ) },
	{ "int", "loopDepth", (intptr_t)(&((idCompiler *)0)->loopDepth), sizeof( ((idCompiler *)0)->loopDepth ) },
	{ "int", "currentLineNumber", (intptr_t)(&((idCompiler *)0)->currentLineNumber), sizeof( ((idCompiler *)0)->currentLineNumber ) },
	{ "int", "currentFileNumber", (intptr_t)(&((idCompiler *)0)->currentFileNumber), sizeof( ((idCompiler *)0)->currentFileNumber ) },
	{ "int", "errorCount", (intptr_t)(&((idCompiler *)0)->errorCount), sizeof( ((idCompiler *)0)->errorCount ) },
	{ "idVarDef *", "scope", (intptr_t)(&((idCompiler *)0)->scope), sizeof( ((idCompiler *)0)->scope ) },
	{ "const idVarDef *", "basetype", (intptr_t)(&((idCompiler *)0)->basetype), sizeof( ((idCompiler *)0)->basetype ) },
	{ NULL, 0 }
};

static classVariableInfo_t prstack_t_typeInfo[] = {
	{ "int", "s", (intptr_t)(&((prstack_t *)0)->s), sizeof( ((prstack_t *)0)->s ) },
	{ "const function_t *", "f", (intptr_t)(&((prstack_t *)0)->f), sizeof( ((prstack_t *)0)->f ) },
	{ "int", "stackbase", (intptr_t)(&((prstack_t *)0)->stackbase), sizeof( ((prstack_t *)0)->stackbase ) },
	{ NULL, 0 }
};

static classVariableInfo_t idInterpreter_typeInfo[] = {
	{ ": prstack_t[64]", "callStack", (intptr_t)(&((idInterpreter *)0)->callStack), sizeof( ((idInterpreter *)0)->callStack ) },
	{ "int", "callStackDepth", (intptr_t)(&((idInterpreter *)0)->callStackDepth), sizeof( ((idInterpreter *)0)->callStackDepth ) },
	{ "int", "maxStackDepth", (intptr_t)(&((idInterpreter *)0)->maxStackDepth), sizeof( ((idInterpreter *)0)->maxStackDepth ) },
	{ "byte[6144]", "localstack", (intptr_t)(&((idInterpreter *)0)->localstack), sizeof( ((idInterpreter *)0)->localstack ) },
	{ "int", "localstackUsed", (intptr_t)(&((idInterpreter *)0)->localstackUsed), sizeof( ((idInterpreter *)0)->localstackUsed ) },
	{ "int", "localstackBase", (intptr_t)(&((idInterpreter *)0)->localstackBase), sizeof( ((idInterpreter *)0)->localstackBase ) },
	{ "int", "maxLocalstackUsed", (intptr_t)(&((idInterpreter *)0)->maxLocalstackUsed), sizeof( ((idInterpreter *)0)->maxLocalstackUsed ) },
	{ "const function_t *", "currentFunction", (intptr_t)(&((idInterpreter *)0)->currentFunction), sizeof( ((idInterpreter *)0)->currentFunction ) },
	{ "int", "instructionPointer", (intptr_t)(&((idInterpreter *)0)->instructionPointer), sizeof( ((idInterpreter *)0)->instructionPointer ) },
	{ "int", "popParms", (intptr_t)(&((idInterpreter *)0)->popParms), sizeof( ((idInterpreter *)0)->popParms ) },
	{ "const idEventDef *", "multiFrameEvent", (intptr_t)(&((idInterpreter *)0)->multiFrameEvent), sizeof( ((idInterpreter *)0)->multiFrameEvent ) },
	{ "idEntity *", "eventEntity", (intptr_t)(&((idInterpreter *)0)->eventEntity), sizeof( ((idInterpreter *)0)->eventEntity ) },
	{ "idThread *", "thread", (intptr_t)(&((idInterpreter *)0)->thread), sizeof( ((idInterpreter *)0)->thread ) },
	{ ": bool", "doneProcessing", (intptr_t)(&((idInterpreter *)0)->doneProcessing), sizeof( ((idInterpreter *)0)->doneProcessing ) },
	{ "bool", "threadDying", (intptr_t)(&((idInterpreter *)0)->threadDying), sizeof( ((idInterpreter *)0)->threadDying ) },
	{ "bool", "terminateOnExit", (intptr_t)(&((idInterpreter *)0)->terminateOnExit), sizeof( ((idInterpreter *)0)->terminateOnExit ) },
	{ "bool", "debug", (intptr_t)(&((idInterpreter *)0)->debug), sizeof( ((idInterpreter *)0)->debug ) },
	{ NULL, 0 }
};

static classVariableInfo_t idThread_typeInfo[] = {
	{ "idThread *", "waitingForThread", (intptr_t)(&((idThread *)0)->waitingForThread), sizeof( ((idThread *)0)->waitingForThread ) },
	{ "int", "waitingFor", (intptr_t)(&((idThread *)0)->waitingFor), sizeof( ((idThread *)0)->waitingFor ) },
	{ "int", "waitingUntil", (intptr_t)(&((idThread *)0)->waitingUntil), sizeof( ((idThread *)0)->waitingUntil ) },
	{ "idInterpreter", "interpreter", (intptr_t)(&((idThread *)0)->interpreter), sizeof( ((idThread *)0)->interpreter ) },
	{ "idDict", "spawnArgs", (intptr_t)(&((idThread *)0)->spawnArgs), sizeof( ((idThread *)0)->spawnArgs ) },
	{ "int", "threadNum", (intptr_t)(&((idThread *)0)->threadNum), sizeof( ((idThread *)0)->threadNum ) },
	{ "idStr", "threadName", (intptr_t)(&((idThread *)0)->threadName), sizeof( ((idThread *)0)->threadName ) },
	{ "int", "lastExecuteTime", (intptr_t)(&((idThread *)0)->lastExecuteTime), sizeof( ((idThread *)0)->lastExecuteTime ) },
	{ "int", "creationTime", (intptr_t)(&((idThread *)0)->creationTime), sizeof( ((idThread *)0)->creationTime ) },
	{ "bool", "manualControl", (intptr_t)(&((idThread *)0)->manualControl), sizeof( ((idThread *)0)->manualControl ) },
	{ NULL, 0 }
};

static classTypeInfo_t classTypeInfo[] = {
	{ "DnComponent", "", sizeof(DnComponent), DnComponent_typeInfo },
	{ "DnMeshComponent", "DnComponent", sizeof(DnMeshComponent), DnMeshComponent_typeInfo },
	{ "DnLightComponent", "DnComponent", sizeof(DnLightComponent), DnLightComponent_typeInfo },
	{ "idEventDef", "", sizeof(idEventDef), idEventDef_typeInfo },
	{ "idEvent", "", sizeof(idEvent), idEvent_typeInfo },
	{ "idEventArg", "", sizeof(idEventArg), idEventArg_typeInfo },
	{ "idAllocError", "idException", sizeof(idAllocError), idAllocError_typeInfo },
	{ "idClass", "", sizeof(idClass), idClass_typeInfo },
	{ "idTypeInfo", "", sizeof(idTypeInfo), idTypeInfo_typeInfo },
	{ "idSaveGame", "", sizeof(idSaveGame), idSaveGame_typeInfo },
	{ "idRestoreGame", "", sizeof(idRestoreGame), idRestoreGame_typeInfo },
	{ "idDebugGraph", "", sizeof(idDebugGraph), idDebugGraph_typeInfo },
	{ "function_t", "", sizeof(function_t), function_t_typeInfo },
	{ "eval_t", "", sizeof(eval_t), eval_t_typeInfo },
	{ "idTypeDef", "", sizeof(idTypeDef), idTypeDef_typeInfo },
	{ "idScriptObject", "", sizeof(idScriptObject), idScriptObject_typeInfo },
//	{ "idScriptVariable< class type , etype_t etype , class returnType >", "", sizeof(idScriptVariable< class type , etype_t etype , class returnType >), idScriptVariable_class_type_etype_t_etype_class_returnType__typeInfo },
	{ "idCompileError", "idException", sizeof(idCompileError), idCompileError_typeInfo },
	{ "varEval_t", "", sizeof(varEval_t), varEval_t_typeInfo },
	{ "idVarDef", "", sizeof(idVarDef), idVarDef_typeInfo },
	{ "idVarDefName", "", sizeof(idVarDefName), idVarDefName_typeInfo },
	{ "statement_t", "", sizeof(statement_t), statement_t_typeInfo },
	{ "idProgram", "", sizeof(idProgram), idProgram_typeInfo },
	{ "frameBlend_t", "", sizeof(frameBlend_t), frameBlend_t_typeInfo },
	{ "jointAnimInfo_t", "", sizeof(jointAnimInfo_t), jointAnimInfo_t_typeInfo },
	{ "jointInfo_t", "", sizeof(jointInfo_t), jointInfo_t_typeInfo },
	{ "jointMod_t", "", sizeof(jointMod_t), jointMod_t_typeInfo },
	{ "frameLookup_t", "", sizeof(frameLookup_t), frameLookup_t_typeInfo },
//	{ "class_28::class_28", "", sizeof(class_28::class_28), class_28_class_28_typeInfo },
	{ "frameCommand_t", "", sizeof(frameCommand_t), frameCommand_t_typeInfo },
	{ "animFlags_t", "", sizeof(animFlags_t), animFlags_t_typeInfo },
	{ "idMD5Anim", "", sizeof(idMD5Anim), idMD5Anim_typeInfo },
	{ "idAnim", "", sizeof(idAnim), idAnim_typeInfo },
	{ "idDeclModelDef", "idDecl", sizeof(idDeclModelDef), idDeclModelDef_typeInfo },
	{ "idAnimBlend", "", sizeof(idAnimBlend), idAnimBlend_typeInfo },
	{ "idAFPoseJointMod", "", sizeof(idAFPoseJointMod), idAFPoseJointMod_typeInfo },
	{ "idAnimator", "", sizeof(idAnimator), idAnimator_typeInfo },
	{ "idAnimManager", "", sizeof(idAnimManager), idAnimManager_typeInfo },
	{ "idClipModel", "", sizeof(idClipModel), idClipModel_typeInfo },
	{ "idClip", "", sizeof(idClip), idClip_typeInfo },
	{ "idPush::pushed_s", "", sizeof(idPush::pushed_s), idPush_pushed_s_typeInfo },
	{ "idPush::pushedGroup_s", "", sizeof(idPush::pushedGroup_s), idPush_pushedGroup_s_typeInfo },
	{ "idPush", "", sizeof(idPush), idPush_typeInfo },
	{ "pvsHandle_t", "", sizeof(pvsHandle_t), pvsHandle_t_typeInfo },
	{ "pvsCurrent_t", "", sizeof(pvsCurrent_t), pvsCurrent_t_typeInfo },
	{ "idPVS", "", sizeof(idPVS), idPVS_typeInfo },
	{ "mpPlayerState_t", "", sizeof(mpPlayerState_t), mpPlayerState_t_typeInfo },
	{ "mpChatLine_t", "", sizeof(mpChatLine_t), mpChatLine_t_typeInfo },
	{ "idMultiplayerGame", "", sizeof(idMultiplayerGame), idMultiplayerGame_typeInfo },
	{ "entityState_t", "", sizeof(entityState_t), entityState_t_typeInfo },
	{ "snapshot_t", "", sizeof(snapshot_t), snapshot_t_typeInfo },
	{ "entityNetEvent_t", "", sizeof(entityNetEvent_t), entityNetEvent_t_typeInfo },
	{ "spawnSpot_t", "", sizeof(spawnSpot_t), spawnSpot_t_typeInfo },
	{ "idEventQueue", "", sizeof(idEventQueue), idEventQueue_typeInfo },
//	{ "idEntityPtr< class type >", "", sizeof(idEntityPtr< class type >), idEntityPtr_class_type__typeInfo },
	{ "timeState_t", "", sizeof(timeState_t), timeState_t_typeInfo },
	{ "rvmGameDelayRemoveEntry_t", "", sizeof(rvmGameDelayRemoveEntry_t), rvmGameDelayRemoveEntry_t_typeInfo },
	{ "idGameLocal", "idGame", sizeof(idGameLocal), idGameLocal_typeInfo },
	{ "DnFullscreenRenderTarget", "", sizeof(DnFullscreenRenderTarget), DnFullscreenRenderTarget_typeInfo },
	{ "dnEditorLight", "dnEditorEntity", sizeof(dnEditorLight), dnEditorLight_typeInfo },
	{ "dnEditorModel", "dnEditorEntity", sizeof(dnEditorModel), dnEditorModel_typeInfo },
	{ "DnRenderPlatform", "", sizeof(DnRenderPlatform), DnRenderPlatform_typeInfo },
	{ "dnGameLocal", "idGameLocal", sizeof(dnGameLocal), dnGameLocal_typeInfo },
	{ "idGameError", "idException", sizeof(idGameError), idGameError_typeInfo },
	{ "stateParms_t", "", sizeof(stateParms_t), stateParms_t_typeInfo },
	{ "stateCall_t", "", sizeof(stateCall_t), stateCall_t_typeInfo },
	{ "rvStateThread::flags", "", sizeof(rvStateThread::flags), rvStateThread_flags_typeInfo },
	{ "rvStateThread", "", sizeof(rvStateThread), rvStateThread_typeInfo },
	{ "idForce", "idClass", sizeof(idForce), idForce_typeInfo },
	{ "idForce_Constant", "idForce", sizeof(idForce_Constant), idForce_Constant_typeInfo },
	{ "idForce_Drag", "idForce", sizeof(idForce_Drag), idForce_Drag_typeInfo },
	{ "idForce_Grab", "idForce", sizeof(idForce_Grab), idForce_Grab_typeInfo },
	{ "idForce_Field", "idForce", sizeof(idForce_Field), idForce_Field_typeInfo },
	{ "idForce_Spring", "idForce", sizeof(idForce_Spring), idForce_Spring_typeInfo },
	{ "impactInfo_t", "", sizeof(impactInfo_t), impactInfo_t_typeInfo },
	{ "idPhysics", "idClass", sizeof(idPhysics), idPhysics_typeInfo },
	{ "staticPState_t", "", sizeof(staticPState_t), staticPState_t_typeInfo },
	{ "idPhysics_Static", "idPhysics", sizeof(idPhysics_Static), idPhysics_Static_typeInfo },
	{ "idPhysics_StaticMulti", "idPhysics", sizeof(idPhysics_StaticMulti), idPhysics_StaticMulti_typeInfo },
	{ "idPhysics_Base", "idPhysics", sizeof(idPhysics_Base), idPhysics_Base_typeInfo },
	{ "idPhysics_Actor", "idPhysics_Base", sizeof(idPhysics_Actor), idPhysics_Actor_typeInfo },
	{ "monsterPState_t", "", sizeof(monsterPState_t), monsterPState_t_typeInfo },
	{ "idPhysics_Monster", "idPhysics_Actor", sizeof(idPhysics_Monster), idPhysics_Monster_typeInfo },
	{ "playerPState_t", "", sizeof(playerPState_t), playerPState_t_typeInfo },
	{ "idPhysics_Player", "idPhysics_Actor", sizeof(idPhysics_Player), idPhysics_Player_typeInfo },
	{ "parametricPState_t", "", sizeof(parametricPState_t), parametricPState_t_typeInfo },
	{ "idPhysics_Parametric", "idPhysics_Base", sizeof(idPhysics_Parametric), idPhysics_Parametric_typeInfo },
	{ "rigidBodyIState_t", "", sizeof(rigidBodyIState_t), rigidBodyIState_t_typeInfo },
	{ "rigidBodyPState_t", "", sizeof(rigidBodyPState_t), rigidBodyPState_t_typeInfo },
	{ "idPhysics_RigidBody", "idPhysics_Base", sizeof(idPhysics_RigidBody), idPhysics_RigidBody_typeInfo },
	{ "idAFConstraint::constraintFlags_s", "", sizeof(idAFConstraint::constraintFlags_s), idAFConstraint_constraintFlags_s_typeInfo },
	{ "idAFConstraint", "", sizeof(idAFConstraint), idAFConstraint_typeInfo },
	{ "idAFConstraint_Fixed", "idAFConstraint", sizeof(idAFConstraint_Fixed), idAFConstraint_Fixed_typeInfo },
	{ "idAFConstraint_BallAndSocketJoint", "idAFConstraint", sizeof(idAFConstraint_BallAndSocketJoint), idAFConstraint_BallAndSocketJoint_typeInfo },
	{ "idAFConstraint_BallAndSocketJointFriction", "idAFConstraint", sizeof(idAFConstraint_BallAndSocketJointFriction), idAFConstraint_BallAndSocketJointFriction_typeInfo },
	{ "idAFConstraint_UniversalJoint", "idAFConstraint", sizeof(idAFConstraint_UniversalJoint), idAFConstraint_UniversalJoint_typeInfo },
	{ "idAFConstraint_UniversalJointFriction", "idAFConstraint", sizeof(idAFConstraint_UniversalJointFriction), idAFConstraint_UniversalJointFriction_typeInfo },
	{ "idAFConstraint_CylindricalJoint", "idAFConstraint", sizeof(idAFConstraint_CylindricalJoint), idAFConstraint_CylindricalJoint_typeInfo },
	{ "idAFConstraint_Hinge", "idAFConstraint", sizeof(idAFConstraint_Hinge), idAFConstraint_Hinge_typeInfo },
	{ "idAFConstraint_HingeFriction", "idAFConstraint", sizeof(idAFConstraint_HingeFriction), idAFConstraint_HingeFriction_typeInfo },
	{ "idAFConstraint_HingeSteering", "idAFConstraint", sizeof(idAFConstraint_HingeSteering), idAFConstraint_HingeSteering_typeInfo },
	{ "idAFConstraint_Slider", "idAFConstraint", sizeof(idAFConstraint_Slider), idAFConstraint_Slider_typeInfo },
	{ "idAFConstraint_Line", "idAFConstraint", sizeof(idAFConstraint_Line), idAFConstraint_Line_typeInfo },
	{ "idAFConstraint_Plane", "idAFConstraint", sizeof(idAFConstraint_Plane), idAFConstraint_Plane_typeInfo },
	{ "idAFConstraint_Spring", "idAFConstraint", sizeof(idAFConstraint_Spring), idAFConstraint_Spring_typeInfo },
	{ "idAFConstraint_Contact", "idAFConstraint", sizeof(idAFConstraint_Contact), idAFConstraint_Contact_typeInfo },
	{ "idAFConstraint_ContactFriction", "idAFConstraint", sizeof(idAFConstraint_ContactFriction), idAFConstraint_ContactFriction_typeInfo },
	{ "idAFConstraint_ConeLimit", "idAFConstraint", sizeof(idAFConstraint_ConeLimit), idAFConstraint_ConeLimit_typeInfo },
	{ "idAFConstraint_PyramidLimit", "idAFConstraint", sizeof(idAFConstraint_PyramidLimit), idAFConstraint_PyramidLimit_typeInfo },
	{ "idAFConstraint_Suspension", "idAFConstraint", sizeof(idAFConstraint_Suspension), idAFConstraint_Suspension_typeInfo },
	{ "AFBodyPState_t", "", sizeof(AFBodyPState_t), AFBodyPState_t_typeInfo },
	{ "idAFBody::bodyFlags_s", "", sizeof(idAFBody::bodyFlags_s), idAFBody_bodyFlags_s_typeInfo },
	{ "idAFBody", "", sizeof(idAFBody), idAFBody_typeInfo },
	{ "idAFTree", "", sizeof(idAFTree), idAFTree_typeInfo },
	{ "AFPState_t", "", sizeof(AFPState_t), AFPState_t_typeInfo },
	{ "AFCollision_t", "", sizeof(AFCollision_t), AFCollision_t_typeInfo },
	{ "idPhysics_AF", "idPhysics_Base", sizeof(idPhysics_AF), idPhysics_AF_typeInfo },
	{ "singleSmoke_t", "", sizeof(singleSmoke_t), singleSmoke_t_typeInfo },
	{ "activeSmokeStage_t", "", sizeof(activeSmokeStage_t), activeSmokeStage_t_typeInfo },
	{ "idSmokeParticles", "", sizeof(idSmokeParticles), idSmokeParticles_typeInfo },
	{ "signal_t", "", sizeof(signal_t), signal_t_typeInfo },
	{ "signalList_t", "", sizeof(signalList_t), signalList_t_typeInfo },
	{ "idEntity::entityFlags_s", "", sizeof(idEntity::entityFlags_s), idEntity_entityFlags_s_typeInfo },
	{ "idEntity", "idClass", sizeof(idEntity), idEntity_typeInfo },
	{ "damageEffect_t", "", sizeof(damageEffect_t), damageEffect_t_typeInfo },
	{ "idAnimatedEntity", "idEntity", sizeof(idAnimatedEntity), idAnimatedEntity_typeInfo },
	{ "SetTimeState", "", sizeof(SetTimeState), SetTimeState_typeInfo },
	{ "idCursor3D", "idEntity", sizeof(idCursor3D), idCursor3D_typeInfo },
	{ "idDragEntity", "", sizeof(idDragEntity), idDragEntity_typeInfo },
	{ "selectedTypeInfo_t", "", sizeof(selectedTypeInfo_t), selectedTypeInfo_t_typeInfo },
	{ "idEditEntities", "", sizeof(idEditEntities), idEditEntities_typeInfo },
	{ "jointConversion_t", "", sizeof(jointConversion_t), jointConversion_t_typeInfo },
	{ "afTouch_t", "", sizeof(afTouch_t), afTouch_t_typeInfo },
	{ "idAF", "", sizeof(idAF), idAF_typeInfo },
	{ "idIK", "", sizeof(idIK), idIK_typeInfo },
	{ "idIK_Walk", "idIK", sizeof(idIK_Walk), idIK_Walk_typeInfo },
	{ "idIK_Reach", "idIK", sizeof(idIK_Reach), idIK_Reach_typeInfo },
	{ "idMultiModelAF", "idEntity", sizeof(idMultiModelAF), idMultiModelAF_typeInfo },
	{ "idChain", "idMultiModelAF", sizeof(idChain), idChain_typeInfo },
	{ "idAFAttachment", "idAnimatedEntity", sizeof(idAFAttachment), idAFAttachment_typeInfo },
	{ "idAFEntity_Base", "idAnimatedEntity", sizeof(idAFEntity_Base), idAFEntity_Base_typeInfo },
	{ "idAFEntity_Gibbable", "idAFEntity_Base", sizeof(idAFEntity_Gibbable), idAFEntity_Gibbable_typeInfo },
	{ "idAFEntity_Generic", "idAFEntity_Gibbable", sizeof(idAFEntity_Generic), idAFEntity_Generic_typeInfo },
	{ "idAFEntity_WithAttachedHead", "idAFEntity_Gibbable", sizeof(idAFEntity_WithAttachedHead), idAFEntity_WithAttachedHead_typeInfo },
	{ "idAFEntity_Vehicle", "idAFEntity_Base", sizeof(idAFEntity_Vehicle), idAFEntity_Vehicle_typeInfo },
	{ "idAFEntity_VehicleSimple", "idAFEntity_Vehicle", sizeof(idAFEntity_VehicleSimple), idAFEntity_VehicleSimple_typeInfo },
	{ "idAFEntity_VehicleFourWheels", "idAFEntity_Vehicle", sizeof(idAFEntity_VehicleFourWheels), idAFEntity_VehicleFourWheels_typeInfo },
	{ "idAFEntity_VehicleSixWheels", "idAFEntity_Vehicle", sizeof(idAFEntity_VehicleSixWheels), idAFEntity_VehicleSixWheels_typeInfo },
	{ "idAFEntity_VehicleAutomated", "idAFEntity_VehicleSixWheels", sizeof(idAFEntity_VehicleAutomated), idAFEntity_VehicleAutomated_typeInfo },
	{ "idAFEntity_SteamPipe", "idAFEntity_Base", sizeof(idAFEntity_SteamPipe), idAFEntity_SteamPipe_typeInfo },
	{ "idAFEntity_ClawFourFingers", "idAFEntity_Base", sizeof(idAFEntity_ClawFourFingers), idAFEntity_ClawFourFingers_typeInfo },
	{ "idHarvestable", "idEntity", sizeof(idHarvestable), idHarvestable_typeInfo },
	{ "idAFEntity_Harvest", "idAFEntity_WithAttachedHead", sizeof(idAFEntity_Harvest), idAFEntity_Harvest_typeInfo },
	{ "idSpawnableEntity", "idEntity", sizeof(idSpawnableEntity), idSpawnableEntity_typeInfo },
	{ "idPlayerStart", "idEntity", sizeof(idPlayerStart), idPlayerStart_typeInfo },
	{ "idActivator", "idEntity", sizeof(idActivator), idActivator_typeInfo },
	{ "idPathCorner", "idEntity", sizeof(idPathCorner), idPathCorner_typeInfo },
	{ "idDamagable", "idEntity", sizeof(idDamagable), idDamagable_typeInfo },
	{ "idExplodable", "idEntity", sizeof(idExplodable), idExplodable_typeInfo },
	{ "idSpring", "idEntity", sizeof(idSpring), idSpring_typeInfo },
	{ "idForceField", "idEntity", sizeof(idForceField), idForceField_typeInfo },
	{ "idAnimated", "idAFEntity_Gibbable", sizeof(idAnimated), idAnimated_typeInfo },
	{ "idStaticEntity", "idEntity", sizeof(idStaticEntity), idStaticEntity_typeInfo },
	{ "idFuncEmitter", "idStaticEntity", sizeof(idFuncEmitter), idFuncEmitter_typeInfo },
	{ "idFuncSmoke", "idEntity", sizeof(idFuncSmoke), idFuncSmoke_typeInfo },
	{ "idFuncSplat", "idFuncEmitter", sizeof(idFuncSplat), idFuncSplat_typeInfo },
	{ "idTextEntity", "idEntity", sizeof(idTextEntity), idTextEntity_typeInfo },
	{ "idLocationEntity", "idEntity", sizeof(idLocationEntity), idLocationEntity_typeInfo },
	{ "idLocationSeparatorEntity", "idEntity", sizeof(idLocationSeparatorEntity), idLocationSeparatorEntity_typeInfo },
	{ "idVacuumSeparatorEntity", "idEntity", sizeof(idVacuumSeparatorEntity), idVacuumSeparatorEntity_typeInfo },
	{ "idVacuumEntity", "idEntity", sizeof(idVacuumEntity), idVacuumEntity_typeInfo },
	{ "idBeam", "idEntity", sizeof(idBeam), idBeam_typeInfo },
	{ "idLiquid", "idEntity", sizeof(idLiquid), idLiquid_typeInfo },
	{ "idShaking", "idEntity", sizeof(idShaking), idShaking_typeInfo },
	{ "idEarthQuake", "idEntity", sizeof(idEarthQuake), idEarthQuake_typeInfo },
	{ "idFuncPortal", "idEntity", sizeof(idFuncPortal), idFuncPortal_typeInfo },
	{ "idFuncRadioChatter", "idEntity", sizeof(idFuncRadioChatter), idFuncRadioChatter_typeInfo },
	{ "idShockwave", "idEntity", sizeof(idShockwave), idShockwave_typeInfo },
	{ "idFuncMountedObject", "idEntity", sizeof(idFuncMountedObject), idFuncMountedObject_typeInfo },
	{ "idFuncMountedWeapon", "idFuncMountedObject", sizeof(idFuncMountedWeapon), idFuncMountedWeapon_typeInfo },
	{ "idPortalSky", "idEntity", sizeof(idPortalSky), idPortalSky_typeInfo },
	{ "idAnimState", "", sizeof(idAnimState), idAnimState_typeInfo },
	{ "idAttachInfo", "", sizeof(idAttachInfo), idAttachInfo_typeInfo },
	{ "copyJoints_t", "", sizeof(copyJoints_t), copyJoints_t_typeInfo },
	{ "idActor", "idAFEntity_Gibbable", sizeof(idActor), idActor_typeInfo },
	{ "idProjectile::projectileFlags_s", "", sizeof(idProjectile::projectileFlags_s), idProjectile_projectileFlags_s_typeInfo },
	{ "idProjectile", "idEntity", sizeof(idProjectile), idProjectile_typeInfo },
	{ "idGuidedProjectile", "idProjectile", sizeof(idGuidedProjectile), idGuidedProjectile_typeInfo },
	{ "idSoulCubeMissile", "idGuidedProjectile", sizeof(idSoulCubeMissile), idSoulCubeMissile_typeInfo },
	{ "beamTarget_t", "", sizeof(beamTarget_t), beamTarget_t_typeInfo },
	{ "idBFGProjectile", "idProjectile", sizeof(idBFGProjectile), idBFGProjectile_typeInfo },
	{ "idHomingProjectile", "idProjectile", sizeof(idHomingProjectile), idHomingProjectile_typeInfo },
	{ "idDebris", "idEntity", sizeof(idDebris), idDebris_typeInfo },
//	{ "idPredictedValue< class type_ >", "", sizeof(idPredictedValue< class type_ >), idPredictedValue_class_type___typeInfo },
	{ "WeaponParticle_t", "", sizeof(WeaponParticle_t), WeaponParticle_t_typeInfo },
	{ "WeaponLight_t", "", sizeof(WeaponLight_t), WeaponLight_t_typeInfo },
	{ "rvmWeaponObject", "idClass", sizeof(rvmWeaponObject), rvmWeaponObject_typeInfo },
	{ "idWeapon", "idAnimatedEntity", sizeof(idWeapon), idWeapon_typeInfo },
	{ "idLight", "idEntity", sizeof(idLight), idLight_typeInfo },
	{ "idWorldspawn", "idEntity", sizeof(idWorldspawn), idWorldspawn_typeInfo },
	{ "idItem", "idEntity", sizeof(idItem), idItem_typeInfo },
	{ "idItemPowerup", "idItem", sizeof(idItemPowerup), idItemPowerup_typeInfo },
	{ "idObjective", "idItem", sizeof(idObjective), idObjective_typeInfo },
	{ "idVideoCDItem", "idItem", sizeof(idVideoCDItem), idVideoCDItem_typeInfo },
	{ "idPDAItem", "idItem", sizeof(idPDAItem), idPDAItem_typeInfo },
	{ "idMoveableItem", "idItem", sizeof(idMoveableItem), idMoveableItem_typeInfo },
	{ "idItemTeam", "idMoveableItem", sizeof(idItemTeam), idItemTeam_typeInfo },
	{ "idMoveablePDAItem", "idMoveableItem", sizeof(idMoveablePDAItem), idMoveablePDAItem_typeInfo },
	{ "idItemRemover", "idEntity", sizeof(idItemRemover), idItemRemover_typeInfo },
	{ "idObjectiveComplete", "idItemRemover", sizeof(idObjectiveComplete), idObjectiveComplete_typeInfo },
	{ "idItemInfo", "", sizeof(idItemInfo), idItemInfo_typeInfo },
	{ "idObjectiveInfo", "", sizeof(idObjectiveInfo), idObjectiveInfo_typeInfo },
	{ "idLevelTriggerInfo", "", sizeof(idLevelTriggerInfo), idLevelTriggerInfo_typeInfo },
	{ "RechargeAmmo_t", "", sizeof(RechargeAmmo_t), RechargeAmmo_t_typeInfo },
	{ "WeaponToggle_t", "", sizeof(WeaponToggle_t), WeaponToggle_t_typeInfo },
	{ "idInventory", "", sizeof(idInventory), idInventory_typeInfo },
	{ "loggedAccel_t", "", sizeof(loggedAccel_t), loggedAccel_t_typeInfo },
	{ "aasLocation_t", "", sizeof(aasLocation_t), aasLocation_t_typeInfo },
	{ "idPlayer", "idActor", sizeof(idPlayer), idPlayer_typeInfo },
	{ "idMover::moveState_t", "", sizeof(idMover::moveState_t), idMover_moveState_t_typeInfo },
	{ "idMover::rotationState_t", "", sizeof(idMover::rotationState_t), idMover_rotationState_t_typeInfo },
	{ "idMover", "idEntity", sizeof(idMover), idMover_typeInfo },
	{ "idSplinePath", "idEntity", sizeof(idSplinePath), idSplinePath_typeInfo },
	{ "floorInfo_s", "", sizeof(floorInfo_s), floorInfo_s_typeInfo },
	{ "idElevator", "idMover", sizeof(idElevator), idElevator_typeInfo },
	{ "idMover_Binary", "idEntity", sizeof(idMover_Binary), idMover_Binary_typeInfo },
	{ "idDoor", "idMover_Binary", sizeof(idDoor), idDoor_typeInfo },
	{ "idPlat", "idMover_Binary", sizeof(idPlat), idPlat_typeInfo },
	{ "idMover_Periodic", "idEntity", sizeof(idMover_Periodic), idMover_Periodic_typeInfo },
	{ "idRotater", "idMover_Periodic", sizeof(idRotater), idRotater_typeInfo },
	{ "idBobber", "idMover_Periodic", sizeof(idBobber), idBobber_typeInfo },
	{ "idPendulum", "idMover_Periodic", sizeof(idPendulum), idPendulum_typeInfo },
	{ "idRiser", "idMover_Periodic", sizeof(idRiser), idRiser_typeInfo },
	{ "idCamera", "idEntity", sizeof(idCamera), idCamera_typeInfo },
	{ "idCameraView", "idCamera", sizeof(idCameraView), idCameraView_typeInfo },
	{ "cameraFrame_t", "", sizeof(cameraFrame_t), cameraFrame_t_typeInfo },
	{ "idCameraAnim", "idCamera", sizeof(idCameraAnim), idCameraAnim_typeInfo },
	{ "idMoveable", "idEntity", sizeof(idMoveable), idMoveable_typeInfo },
	{ "idBarrel", "idMoveable", sizeof(idBarrel), idBarrel_typeInfo },
	{ "idExplodingBarrel", "idBarrel", sizeof(idExplodingBarrel), idExplodingBarrel_typeInfo },
	{ "idTarget", "idEntity", sizeof(idTarget), idTarget_typeInfo },
	{ "idTarget_Remove", "idTarget", sizeof(idTarget_Remove), idTarget_Remove_typeInfo },
	{ "idTarget_Show", "idTarget", sizeof(idTarget_Show), idTarget_Show_typeInfo },
	{ "idTarget_Damage", "idTarget", sizeof(idTarget_Damage), idTarget_Damage_typeInfo },
	{ "idTarget_SessionCommand", "idTarget", sizeof(idTarget_SessionCommand), idTarget_SessionCommand_typeInfo },
	{ "idTarget_EndLevel", "idTarget", sizeof(idTarget_EndLevel), idTarget_EndLevel_typeInfo },
	{ "idTarget_WaitForButton", "idTarget", sizeof(idTarget_WaitForButton), idTarget_WaitForButton_typeInfo },
	{ "idTarget_SetGlobalShaderTime", "idTarget", sizeof(idTarget_SetGlobalShaderTime), idTarget_SetGlobalShaderTime_typeInfo },
	{ "idTarget_SetShaderParm", "idTarget", sizeof(idTarget_SetShaderParm), idTarget_SetShaderParm_typeInfo },
	{ "idTarget_SetShaderTime", "idTarget", sizeof(idTarget_SetShaderTime), idTarget_SetShaderTime_typeInfo },
	{ "idTarget_FadeEntity", "idTarget", sizeof(idTarget_FadeEntity), idTarget_FadeEntity_typeInfo },
	{ "idTarget_LightFadeIn", "idTarget", sizeof(idTarget_LightFadeIn), idTarget_LightFadeIn_typeInfo },
	{ "idTarget_LightFadeOut", "idTarget", sizeof(idTarget_LightFadeOut), idTarget_LightFadeOut_typeInfo },
	{ "idTarget_Give", "idTarget", sizeof(idTarget_Give), idTarget_Give_typeInfo },
	{ "idTarget_GiveEmail", "idTarget", sizeof(idTarget_GiveEmail), idTarget_GiveEmail_typeInfo },
	{ "idTarget_SetModel", "idTarget", sizeof(idTarget_SetModel), idTarget_SetModel_typeInfo },
	{ "SavedGui_t", "", sizeof(SavedGui_t), SavedGui_t_typeInfo },
	{ "idTarget_SetInfluence", "idTarget", sizeof(idTarget_SetInfluence), idTarget_SetInfluence_typeInfo },
	{ "idTarget_SetKeyVal", "idTarget", sizeof(idTarget_SetKeyVal), idTarget_SetKeyVal_typeInfo },
	{ "idTarget_SetFov", "idTarget", sizeof(idTarget_SetFov), idTarget_SetFov_typeInfo },
	{ "idTarget_SetPrimaryObjective", "idTarget", sizeof(idTarget_SetPrimaryObjective), idTarget_SetPrimaryObjective_typeInfo },
	{ "idTarget_LockDoor", "idTarget", sizeof(idTarget_LockDoor), idTarget_LockDoor_typeInfo },
	{ "idTarget_CallObjectFunction", "idTarget", sizeof(idTarget_CallObjectFunction), idTarget_CallObjectFunction_typeInfo },
	{ "idTarget_EnableLevelWeapons", "idTarget", sizeof(idTarget_EnableLevelWeapons), idTarget_EnableLevelWeapons_typeInfo },
	{ "idTarget_Tip", "idTarget", sizeof(idTarget_Tip), idTarget_Tip_typeInfo },
	{ "idTarget_GiveSecurity", "idTarget", sizeof(idTarget_GiveSecurity), idTarget_GiveSecurity_typeInfo },
	{ "idTarget_RemoveWeapons", "idTarget", sizeof(idTarget_RemoveWeapons), idTarget_RemoveWeapons_typeInfo },
	{ "idTarget_LevelTrigger", "idTarget", sizeof(idTarget_LevelTrigger), idTarget_LevelTrigger_typeInfo },
	{ "idTarget_EnableStamina", "idTarget", sizeof(idTarget_EnableStamina), idTarget_EnableStamina_typeInfo },
	{ "idTarget_FadeSoundClass", "idTarget", sizeof(idTarget_FadeSoundClass), idTarget_FadeSoundClass_typeInfo },
	{ "idTrigger", "idEntity", sizeof(idTrigger), idTrigger_typeInfo },
	{ "idTrigger_Multi", "idTrigger", sizeof(idTrigger_Multi), idTrigger_Multi_typeInfo },
	{ "idTrigger_EntityName", "idTrigger", sizeof(idTrigger_EntityName), idTrigger_EntityName_typeInfo },
	{ "idTrigger_Timer", "idTrigger", sizeof(idTrigger_Timer), idTrigger_Timer_typeInfo },
	{ "idTrigger_Count", "idTrigger", sizeof(idTrigger_Count), idTrigger_Count_typeInfo },
	{ "idTrigger_Hurt", "idTrigger", sizeof(idTrigger_Hurt), idTrigger_Hurt_typeInfo },
	{ "idTrigger_Fade", "idTrigger", sizeof(idTrigger_Fade), idTrigger_Fade_typeInfo },
	{ "idTrigger_Touch", "idTrigger", sizeof(idTrigger_Touch), idTrigger_Touch_typeInfo },
	{ "idTrigger_Flag", "idTrigger_Multi", sizeof(idTrigger_Flag), idTrigger_Flag_typeInfo },
	{ "idSound", "idEntity", sizeof(idSound), idSound_typeInfo },
	{ "idFXLocalAction", "", sizeof(idFXLocalAction), idFXLocalAction_typeInfo },
	{ "idEntityFx", "idEntity", sizeof(idEntityFx), idEntityFx_typeInfo },
	{ "idTeleporter", "idEntityFx", sizeof(idTeleporter), idTeleporter_typeInfo },
	{ "idSecurityCamera", "idEntity", sizeof(idSecurityCamera), idSecurityCamera_typeInfo },
	{ "shard_t", "", sizeof(shard_t), shard_t_typeInfo },
	{ "idBrittleFracture", "idEntity", sizeof(idBrittleFracture), idBrittleFracture_typeInfo },
	{ "dnWeaponMightyFoot", "rvmWeaponObject", sizeof(dnWeaponMightyFoot), dnWeaponMightyFoot_typeInfo },
	{ "dnWeaponPistol", "rvmWeaponObject", sizeof(dnWeaponPistol), dnWeaponPistol_typeInfo },
	{ "dnWeaponShotgun", "rvmWeaponObject", sizeof(dnWeaponShotgun), dnWeaponShotgun_typeInfo },
	{ "dnWeaponM16", "rvmWeaponObject", sizeof(dnWeaponM16), dnWeaponM16_typeInfo },
	{ "DnItem", "idEntity", sizeof(DnItem), DnItem_typeInfo },
	{ "DnItemShotgun", "DnItem", sizeof(DnItemShotgun), DnItemShotgun_typeInfo },
	{ "idTestModel", "idAnimatedEntity", sizeof(idTestModel), idTestModel_typeInfo },
	{ "idMoveState", "", sizeof(idMoveState), idMoveState_typeInfo },
	{ "DnRand", "", sizeof(DnRand), DnRand_typeInfo },
	{ "DnAI", "idActor", sizeof(DnAI), DnAI_typeInfo },
	{ "DnPigcop", "DnAI", sizeof(DnPigcop), DnPigcop_typeInfo },
	{ "DnLiztroop", "DnAI", sizeof(DnLiztroop), DnLiztroop_typeInfo },
	{ "DnCivilian", "DnAI", sizeof(DnCivilian), DnCivilian_typeInfo },
	{ "DukePlayer", "idPlayer", sizeof(DukePlayer), DukePlayer_typeInfo },
	{ "dnDecoration", "idMover", sizeof(dnDecoration), dnDecoration_typeInfo },
	{ "rvClientEntity", "idClass", sizeof(rvClientEntity), rvClientEntity_typeInfo },
//	{ "rvClientEntityPtr< class type >", "", sizeof(rvClientEntityPtr< class type >), rvClientEntityPtr_class_type__typeInfo },
	{ "rvClientPhysics", "idEntity", sizeof(rvClientPhysics), rvClientPhysics_typeInfo },
	{ "rvClientModel", "rvClientEntity", sizeof(rvClientModel), rvClientModel_typeInfo },
	{ "rvAnimatedClientEntity", "rvClientModel", sizeof(rvAnimatedClientEntity), rvAnimatedClientEntity_typeInfo },
	{ "rvClientMoveable", "rvClientEntity", sizeof(rvClientMoveable), rvClientMoveable_typeInfo },
	{ "rvClientAFEntity", "rvAnimatedClientEntity", sizeof(rvClientAFEntity), rvClientAFEntity_typeInfo },
	{ "rvClientAFAttachment", "rvClientAFEntity", sizeof(rvClientAFAttachment), rvClientAFAttachment_typeInfo },
	{ "opcode_t", "", sizeof(opcode_t), opcode_t_typeInfo },
	{ "idCompiler", "", sizeof(idCompiler), idCompiler_typeInfo },
	{ "prstack_t", "", sizeof(prstack_t), prstack_t_typeInfo },
	{ "idInterpreter", "", sizeof(idInterpreter), idInterpreter_typeInfo },
	{ "idThread", "idClass", sizeof(idThread), idThread_typeInfo },
	{ NULL, NULL, 0, NULL }
};

#endif /* !__GAMETYPEINFO_H__ */
