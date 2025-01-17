/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_main.c -- server main program

#include "quakedef.h"
#include "sv_demo.h"
#include "libcurl.h"
#include "csprogs.h"
#include "thread.h"

// current client
client_t *host_client;

static void SV_SaveEntFile_f(cmd_state_t *cmd);
static void SV_StartDownload_f(cmd_state_t *cmd);
static void SV_Download_f(cmd_state_t *cmd);
static void SV_VM_Setup(void);
extern cvar_t net_connecttimeout;

cvar_t sv_worldmessage = {CF_SERVER | CF_READONLY, "sv_worldmessage", "", "title of current level"};
cvar_t sv_worldname = {CF_SERVER | CF_READONLY, "sv_worldname", "", "name of current worldmodel"};
cvar_t sv_worldnamenoextension = {CF_SERVER | CF_READONLY, "sv_worldnamenoextension", "", "name of current worldmodel without extension"};
cvar_t sv_worldbasename = {CF_SERVER | CF_READONLY, "sv_worldbasename", "", "name of current worldmodel without maps/ prefix or extension"};

cvar_t sv_disablenotify = {CF_SERVER, "sv_disablenotify", "0", "suppress broadcast prints when certain cvars are changed (CF_NOTIFY flag in engine code)"};
cvar_t coop = {CF_SERVER, "coop","0", "coop mode, 0 = no coop, 1 = coop mode, multiple players playing through the singleplayer game (coop mode also shuts off deathmatch)"};
cvar_t deathmatch = {CF_SERVER, "deathmatch","0", "deathmatch mode, values depend on mod but typically 0 = no deathmatch, 1 = normal deathmatch with respawning weapons, 2 = weapons stay (players can only pick up new weapons)"};
cvar_t fraglimit = {CF_SERVER | CF_NOTIFY, "fraglimit","0", "ends level if this many frags is reached by any player"};
cvar_t gamecfg = {CF_SERVER, "gamecfg", "0", "unused cvar in quake, can be used by mods"};
cvar_t noexit = {CF_SERVER | CF_NOTIFY, "noexit","0", "kills anyone attempting to use an exit"};
cvar_t nomonsters = {CF_SERVER, "nomonsters", "0", "unused cvar in quake, can be used by mods"};
cvar_t pausable = {CF_SERVER, "pausable","1", "allow players to pause or not (otherwise, only the server admin can)"};
cvar_t pr_checkextension = {CF_SERVER | CF_READONLY, "pr_checkextension", "1", "indicates to QuakeC that the standard quakec extensions system is available (if 0, quakec should not attempt to use extensions)"};
cvar_t samelevel = {CF_SERVER | CF_NOTIFY, "samelevel","0", "repeats same level if level ends (due to timelimit or someone hitting an exit)"};
cvar_t skill = {CF_SERVER, "skill","1", "difficulty level of game, affects monster layouts in levels, 0 = easy, 1 = normal, 2 = hard, 3 = nightmare (same layout as hard but monsters fire twice)"};
cvar_t host_timescale = {CF_CLIENT | CF_SERVER, "host_timescale", "1.0", "controls game speed, 0.5 is half speed, 2 is double speed"};

cvar_t sv_accelerate = {CF_SERVER, "sv_accelerate", "10", "rate at which a player accelerates to sv_maxspeed"};
cvar_t sv_aim = {CF_SERVER | CF_ARCHIVE, "sv_aim", "2", "maximum cosine angle for quake's vertical autoaim, a value above 1 completely disables the autoaim, quake used 0.93"};
cvar_t sv_airaccel_qw = {CF_SERVER, "sv_airaccel_qw", "1", "ratio of QW-style air control as opposed to simple acceleration; when < 0, the speed is clamped against the maximum allowed forward speed after the move"};
cvar_t sv_airaccel_qw_stretchfactor = {CF_SERVER, "sv_airaccel_qw_stretchfactor", "0", "when set, the maximum acceleration increase the player may get compared to forward-acceleration when strafejumping"};
cvar_t sv_airaccel_sideways_friction = {CF_SERVER, "sv_airaccel_sideways_friction", "", "anti-sideways movement stabilization (reduces speed gain when zigzagging); when < 0, only so much friction is applied that braking (by accelerating backwards) cannot be stronger"};
cvar_t sv_airaccelerate = {CF_SERVER, "sv_airaccelerate", "-1", "rate at which a player accelerates to sv_maxairspeed while in the air, if less than 0 the sv_accelerate variable is used instead"};
cvar_t sv_airstopaccelerate = {CF_SERVER, "sv_airstopaccelerate", "0", "when set, replacement for sv_airaccelerate when moving backwards"};
cvar_t sv_airspeedlimit_nonqw = {CF_SERVER, "sv_airspeedlimit_nonqw", "0", "when set, this is a soft speed limit while in air when using airaccel_qw not equal to 1"};
cvar_t sv_airstrafeaccelerate = {CF_SERVER, "sv_airstrafeaccelerate", "0", "when set, replacement for sv_airaccelerate when just strafing"};
cvar_t sv_maxairstrafespeed = {CF_SERVER, "sv_maxairstrafespeed", "0", "when set, replacement for sv_maxairspeed when just strafing"};
cvar_t sv_airstrafeaccel_qw = {CF_SERVER, "sv_airstrafeaccel_qw", "0", "when set, replacement for sv_airaccel_qw when just strafing"};
cvar_t sv_aircontrol = {CF_SERVER, "sv_aircontrol", "0", "CPMA-style air control"};
cvar_t sv_aircontrol_power = {CF_SERVER, "sv_aircontrol_power", "2", "CPMA-style air control exponent"};
cvar_t sv_aircontrol_penalty = {CF_SERVER, "sv_aircontrol_penalty", "0", "deceleration while using CPMA-style air control"};
cvar_t sv_allowdownloads = {CF_SERVER, "sv_allowdownloads", "1", "whether to allow clients to download files from the server (does not affect http downloads)"};
cvar_t sv_allowdownloads_archive = {CF_SERVER, "sv_allowdownloads_archive", "0", "whether to allow downloads of archives (pak/pk3)"};
cvar_t sv_allowdownloads_config = {CF_SERVER, "sv_allowdownloads_config", "0", "whether to allow downloads of config files (cfg)"};
cvar_t sv_allowdownloads_dlcache = {CF_SERVER, "sv_allowdownloads_dlcache", "0", "whether to allow downloads of dlcache files (dlcache/)"};
cvar_t sv_allowdownloads_inarchive = {CF_SERVER, "sv_allowdownloads_inarchive", "0", "whether to allow downloads from archives (pak/pk3)"};
cvar_t sv_areagrid_link_SOLID_NOT = {CF_SERVER | CF_NOTIFY, "sv_areagrid_link_SOLID_NOT", "1", "set to 0 to prevent SOLID_NOT entities from being linked to the area grid, and unlink any that are already linked (in the code paths that would otherwise link them), for better performance"};
cvar_t sv_areagrid_mingridsize = {CF_SERVER | CF_NOTIFY, "sv_areagrid_mingridsize", "128", "minimum areagrid cell size, smaller values work better for lots of small objects, higher values for large objects"};
cvar_t sv_checkforpacketsduringsleep = {CF_SERVER, "sv_checkforpacketsduringsleep", "0", "uses select() function to wait between frames which can be interrupted by packets being received, instead of Sleep()/usleep()/SDL_Sleep() functions which do not check for packets"};
cvar_t sv_clmovement_enable = {CF_SERVER, "sv_clmovement_enable", "1", "whether to allow clients to use cl_movement prediction, which can cause choppy movement on the server which may annoy other players"};
cvar_t sv_clmovement_minping = {CF_SERVER, "sv_clmovement_minping", "0", "if client ping is below this time in milliseconds, then their ability to use cl_movement prediction is disabled for a while (as they don't need it)"};
cvar_t sv_clmovement_minping_disabletime = {CF_SERVER, "sv_clmovement_minping_disabletime", "1000", "when client falls below minping, disable their prediction for this many milliseconds (should be at least 1000 or else their prediction may turn on/off frequently)"};
cvar_t sv_clmovement_inputtimeout = {CF_SERVER, "sv_clmovement_inputtimeout", "0.1", "when a client does not send input for this many seconds (max 0.1), force them to move anyway (unlike QuakeWorld)"};
cvar_t sv_cullentities_nevercullbmodels = {CF_SERVER, "sv_cullentities_nevercullbmodels", "0", "if enabled the clients are always notified of moving doors and lifts and other submodels of world (warning: eats a lot of network bandwidth on some levels!)"};
cvar_t sv_cullentities_pvs = {CF_SERVER, "sv_cullentities_pvs", "1", "fast but loose culling of hidden entities"};
cvar_t sv_cullentities_stats = {CF_SERVER, "sv_cullentities_stats", "0", "displays stats on network entities culled by various methods for each client"};
cvar_t sv_cullentities_trace = {CF_SERVER, "sv_cullentities_trace", "0", "somewhat slow but very tight culling of hidden entities, minimizes network traffic and makes wallhack cheats useless"};
cvar_t sv_cullentities_trace_delay = {CF_SERVER, "sv_cullentities_trace_delay", "1", "number of seconds until the entity gets actually culled (also applies to portal camera eyes even if sv_cullentities_trace is 0)"};
cvar_t sv_cullentities_trace_delay_players = {CF_SERVER, "sv_cullentities_trace_delay_players", "0.2", "number of seconds until the entity gets actually culled if it is a player entity"};
cvar_t sv_cullentities_trace_enlarge = {CF_SERVER, "sv_cullentities_trace_enlarge", "0", "box enlargement for entity culling (also applies to portal camera eyes even if sv_cullentities_trace is 0)"};
cvar_t sv_cullentities_trace_expand = {CF_SERVER, "sv_cullentities_trace_expand", "0", "box is expanded by this many units for entity culling (also applies to portal camera eyes even if sv_cullentities_trace is 0)"};
cvar_t sv_cullentities_trace_eyejitter = {CF_SERVER, "sv_cullentities_trace_eyejitter", "16", "jitter the eye by this much for each trace (also applies to portal camera eyes even if sv_cullentities_trace is 0)"};
cvar_t sv_cullentities_trace_prediction = {CF_SERVER, "sv_cullentities_trace_prediction", "1", "also trace from the predicted player position"};
cvar_t sv_cullentities_trace_prediction_time = {CF_SERVER, "sv_cullentities_trace_prediction_time", "0.2", "maximum ping time to predict in seconds"};
cvar_t sv_cullentities_trace_entityocclusion = {CF_SERVER, "sv_cullentities_trace_entityocclusion", "0", "also check if doors and other bsp models are in the way"};
cvar_t sv_cullentities_trace_samples = {CF_SERVER, "sv_cullentities_trace_samples", "2", "number of samples to test for entity culling"};
cvar_t sv_cullentities_trace_samples_extra = {CF_SERVER, "sv_cullentities_trace_samples_extra", "2", "number of samples to test for entity culling when the entity affects its surroundings by e.g. dlight (also applies to portal camera eyes even if sv_cullentities_trace is 0)"};
cvar_t sv_cullentities_trace_samples_players = {CF_SERVER, "sv_cullentities_trace_samples_players", "8", "number of samples to test for entity culling when the entity is a player entity"};
cvar_t sv_cullentities_trace_spectators = {CF_SERVER, "sv_cullentities_trace_spectators", "0", "enables trace entity culling for clients that are spectating"};
cvar_t sv_debugmove = {CF_SERVER | CF_NOTIFY, "sv_debugmove", "0", "disables collision detection optimizations for debugging purposes"};
cvar_t sv_dedicated = {CF_SERVER | CF_READONLY, "sv_dedicated", "0", "for scripts and SVQC to detect when they're running on a dedicated server"};
cvar_t sv_echobprint = {CF_SERVER | CF_ARCHIVE, "sv_echobprint", "1", "prints gamecode bprint() calls to server console"};
cvar_t sv_edgefriction = {CF_SERVER, "edgefriction", "1", "how much you slow down when nearing a ledge you might fall off, multiplier of sv_friction (Quake used 2, QuakeWorld used 1 due to a bug in physics code)"};
cvar_t sv_entpatch = {CF_SERVER, "sv_entpatch", "1", "enables loading of .ent files to override entities in the bsp (for example Threewave CTF server pack contains .ent patch files enabling play of CTF on id1 maps)"};
cvar_t sv_freezenonclients = {CF_SERVER | CF_NOTIFY, "sv_freezenonclients", "0", "freezes time, except for players, allowing you to walk around and take screenshots of explosions"};
cvar_t sv_friction = {CF_SERVER | CF_NOTIFY, "sv_friction","4", "how fast you slow down"};
cvar_t sv_gameplayfix_blowupfallenzombies = {CF_SERVER, "sv_gameplayfix_blowupfallenzombies", "1", "causes findradius to detect SOLID_NOT entities such as zombies and corpses on the floor, allowing splash damage to apply to them"};
cvar_t sv_gameplayfix_consistentplayerprethink = {CF_SERVER, "sv_gameplayfix_consistentplayerprethink", "0", "improves fairness in multiplayer by running all PlayerPreThink functions (which fire weapons) before performing physics, then running all PlayerPostThink functions"};
cvar_t sv_gameplayfix_delayprojectiles = {CF_SERVER, "sv_gameplayfix_delayprojectiles", "1", "causes entities to not move on the same frame they are spawned, meaning that projectiles wait until the next frame to perform their first move, giving proper interpolation and rocket trails, but making weapons harder to use at low framerates"};
cvar_t sv_gameplayfix_droptofloorstartsolid = {CF_SERVER, "sv_gameplayfix_droptofloorstartsolid", "1", "prevents items and monsters that start in a solid area from falling out of the level (makes droptofloor treat trace_startsolid as an acceptable outcome)"};
cvar_t sv_gameplayfix_droptofloorstartsolid_nudgetocorrect = {CF_SERVER, "sv_gameplayfix_droptofloorstartsolid_nudgetocorrect", "1", "tries to nudge stuck items and monsters out of walls before droptofloor is performed"};
cvar_t sv_gameplayfix_easierwaterjump = {CF_SERVER, "sv_gameplayfix_easierwaterjump", "1", "changes water jumping to make it easier to get out of water (exactly like in QuakeWorld)"};
cvar_t sv_gameplayfix_findradiusdistancetobox = {CF_SERVER, "sv_gameplayfix_findradiusdistancetobox", "1", "causes findradius to check the distance to the corner of a box rather than the center of the box, makes findradius detect bmodels such as very large doors that would otherwise be unaffected by splash damage"};
cvar_t sv_gameplayfix_gravityunaffectedbyticrate = {CF_SERVER, "sv_gameplayfix_gravityunaffectedbyticrate", "0", "fix some ticrate issues in physics."};
cvar_t sv_gameplayfix_grenadebouncedownslopes = {CF_SERVER, "sv_gameplayfix_grenadebouncedownslopes", "1", "prevents MOVETYPE_BOUNCE (grenades) from getting stuck when fired down a downward sloping surface"};
cvar_t sv_gameplayfix_impactbeforeonground = {CF_SERVER, "sv_gameplayfix_impactbeforeonground", "0", "enables a bug from old DP versions in which entity .touch functions are called before FL_ONGROUND is set when a collision is detected in SV_FlyMove() (used by MOVETYPE_WALK and MOVETYPE_STEP), Quake 1.5 and Combat+ mods require this, it breaks id1 fiends"};
cvar_t sv_gameplayfix_multiplethinksperframe = {CF_SERVER, "sv_gameplayfix_multiplethinksperframe", "1", "allows entities to think more often than the server framerate, primarily useful for very high fire rate weapons"};
cvar_t sv_gameplayfix_noairborncorpse = {CF_SERVER, "sv_gameplayfix_noairborncorpse", "1", "causes entities (corpses, items, etc) sitting ontop of moving entities (players) to fall when the moving entity (player) is no longer supporting them"};
cvar_t sv_gameplayfix_noairborncorpse_allowsuspendeditems = {CF_SERVER, "sv_gameplayfix_noairborncorpse_allowsuspendeditems", "1", "causes entities sitting ontop of objects that are instantaneously remove to float in midair (special hack to allow a common level design trick for floating items)"};
cvar_t sv_gameplayfix_nudgeoutofsolid = {CF_SERVER, "sv_gameplayfix_nudgeoutofsolid", "0", "attempts to fix physics errors where an object ended up in solid for some reason, smarter than sv_gameplayfix_unstick* except on Q1BSP with mod_q1bsp_polygoncollisions disabled (there it falls back to the unsticking method)"};
cvar_t sv_gameplayfix_nudgeoutofsolid_separation = {CF_SERVER, "sv_gameplayfix_nudgeoutofsolid_separation", "0.03125", "keep objects this distance apart to prevent collision issues on seams"};
cvar_t sv_gameplayfix_q2airaccelerate = {CF_SERVER, "sv_gameplayfix_q2airaccelerate", "0", "Quake2-style air acceleration"};
cvar_t sv_gameplayfix_nogravityonground = {CF_SERVER, "sv_gameplayfix_nogravityonground", "0", "turn off gravity when on ground (to get rid of sliding)"};
cvar_t sv_gameplayfix_setmodelrealbox = {CF_SERVER, "sv_gameplayfix_setmodelrealbox", "1", "fixes a bug in Quake that made setmodel always set the entity box to ('-16 -16 -16', '16 16 16') rather than properly checking the model box, breaks some poorly coded mods"};
cvar_t sv_gameplayfix_slidemoveprojectiles = {CF_SERVER, "sv_gameplayfix_slidemoveprojectiles", "1", "allows MOVETYPE_FLY/FLYMISSILE/TOSS/BOUNCE/BOUNCEMISSILE entities to finish their move in a frame even if they hit something, fixes 'gravity accumulation' bug for grenades on steep slopes"};
cvar_t sv_gameplayfix_stepdown = {CF_SERVER, "sv_gameplayfix_stepdown", "0", "attempts to step down stairs, not just up them (prevents the familiar thud..thud..thud.. when running down stairs and slopes)"};
cvar_t sv_gameplayfix_stepmultipletimes = {CF_SERVER, "sv_gameplayfix_stepmultipletimes", "0", "applies step-up onto a ledge more than once in a single frame, when running quickly up stairs"};
cvar_t sv_gameplayfix_nostepmoveonsteepslopes = {CF_SERVER, "sv_gameplayfix_nostepmoveonsteepslopes", "0", "crude fix which prevents MOVETYPE_STEP (not swimming or flying) to move on slopes whose angle is bigger than 45 degree"};
cvar_t sv_gameplayfix_swiminbmodels = {CF_SERVER, "sv_gameplayfix_swiminbmodels", "1", "causes pointcontents (used to determine if you are in a liquid) to check bmodel entities as well as the world model, so you can swim around in (possibly moving) water bmodel entities"};
cvar_t sv_gameplayfix_upwardvelocityclearsongroundflag = {CF_SERVER, "sv_gameplayfix_upwardvelocityclearsongroundflag", "1", "prevents monsters, items, and most other objects from being stuck to the floor when pushed around by damage, and other situations in mods"};
cvar_t sv_gameplayfix_downtracesupportsongroundflag = {CF_SERVER, "sv_gameplayfix_downtracesupportsongroundflag", "1", "prevents very short moves from clearing onground (which may make the player stick to the floor at high netfps), fixes groundentity not being set when walking onto a mover with sv_gameplayfix_nogravityonground"};
cvar_t sv_gameplayfix_q1bsptracelinereportstexture = {CF_SERVER, "sv_gameplayfix_q1bsptracelinereportstexture", "1", "enables mods to get accurate trace_texture results on q1bsp by using a surface-hitting traceline implementation rather than the standard solidbsp method, q3bsp always reports texture accurately"};
cvar_t sv_gameplayfix_unstickplayers = {CF_SERVER, "sv_gameplayfix_unstickplayers", "1", "big hack to try and fix the rare case of MOVETYPE_WALK entities getting stuck in the world clipping hull. Quake did something similar."};
cvar_t sv_gameplayfix_unstickentities = {CF_SERVER, "sv_gameplayfix_unstickentities", "0", "hack to check if entities are crossing world collision hull and try to move them to the right position. Quake didn't do this so maps shouldn't depend on it."};
cvar_t sv_gameplayfix_fixedcheckwatertransition = {CF_SERVER, "sv_gameplayfix_fixedcheckwatertransition", "1", "fix two very stupid bugs in SV_CheckWaterTransition when watertype is CONTENTS_EMPTY (the bugs causes waterlevel to be 1 on first frame, -1 on second frame - the fix makes it 0 on both frames)"};
cvar_t sv_gameplayfix_nosquashentities = {CF_SERVER, "sv_gameplayfix_nosquashentities", "0", "Entity hitboxes will not be resized or disabled when they are crushed by movers, and will continue to be affected by movers."};
cvar_t sv_gravity = {CF_SERVER | CF_NOTIFY, "sv_gravity","800", "how fast you fall (512 = roughly earth gravity)"};
cvar_t sv_init_frame_count = {CF_SERVER, "sv_init_frame_count", "2", "number of frames to run to allow everything to settle before letting clients connect"};
cvar_t sv_idealpitchscale = {CF_SERVER, "sv_idealpitchscale","0.8", "how much to look up/down slopes and stairs when not using freelook"};
cvar_t sv_jumpstep = {CF_SERVER | CF_NOTIFY, "sv_jumpstep", "0", "whether you can step up while jumping"};
cvar_t sv_jumpvelocity = {CF_SERVER, "sv_jumpvelocity", "270", "cvar that can be used by QuakeC code for jump velocity"};
cvar_t sv_legacy_bbox_expand = {CF_SERVER, "sv_legacy_bbox_expand", "1", "before linking an entity to the area grid, decrease its mins and increase its maxs by '1 1 1', or '15 15 1' if it has flag FL_ITEM (this is the Quake/QuakeWorld behaviour); disable to make SVQC bboxes consistent with CSQC which never does this expansion"};
cvar_t sv_maxairspeed = {CF_SERVER, "sv_maxairspeed", "30", "maximum speed a player can accelerate to when airborn (note that it is possible to completely stop by moving the opposite direction)"};
cvar_t sv_maxrate = {CF_SERVER | CF_ARCHIVE | CF_NOTIFY, "sv_maxrate", "1000000", "upper limit on client rate cvar, should reflect your network connection quality"};
cvar_t sv_maxspeed = {CF_SERVER | CF_NOTIFY, "sv_maxspeed", "320", "maximum speed a player can accelerate to when on ground (can be exceeded by tricks)"};
cvar_t sv_maxvelocity = {CF_SERVER | CF_NOTIFY, "sv_maxvelocity","2000", "universal speed limit on all entities"};
cvar_t sv_nostep = {CF_SERVER | CF_NOTIFY, "sv_nostep","0", "prevents MOVETYPE_STEP entities (monsters) from moving"};
cvar_t sv_playerphysicsqc = {CF_SERVER | CF_NOTIFY, "sv_playerphysicsqc", "1", "enables QuakeC function to override player physics"};
cvar_t sv_progs = {CF_SERVER, "sv_progs", "progs.dat", "selects which quakec progs.dat file to run" };
cvar_t sv_protocolname = {CF_SERVER, "sv_protocolname", "DP7", "selects network protocol to host for (values include QUAKE, QUAKEDP, NEHAHRAMOVIE, DP1 and up)"};
cvar_t sv_qcstats = {CF_SERVER, "sv_qcstats", "0", "Disables engine sending of stats 220 and above, for use by certain games such as Xonotic, NOTE: it's strongly recommended that SVQC send correct STAT_MOVEVARS_TICRATE and STAT_MOVEVARS_TIMESCALE"};
cvar_t sv_random_seed = {CF_SERVER, "sv_random_seed", "", "random seed; when set, on every map start this random seed is used to initialize the random number generator. Don't touch it unless for benchmarking or debugging"};
cvar_t host_limitlocal = {CF_SERVER, "host_limitlocal", "0", "whether to apply rate limiting to the local player in a listen server (only useful for testing)"};
cvar_t sv_sound_land = {CF_SERVER, "sv_sound_land", "demon/dland2.wav", "sound to play when MOVETYPE_STEP entity hits the ground at high speed (empty cvar disables the sound)"};
cvar_t sv_sound_watersplash = {CF_SERVER, "sv_sound_watersplash", "misc/h2ohit1.wav", "sound to play when MOVETYPE_FLY/TOSS/BOUNCE/STEP entity enters or leaves water (empty cvar disables the sound)"};
cvar_t sv_stepheight = {CF_SERVER | CF_NOTIFY, "sv_stepheight", "18", "how high you can step up (TW_SV_STEPCONTROL extension)"};
cvar_t sv_stopspeed = {CF_SERVER | CF_NOTIFY, "sv_stopspeed","100", "how fast you come to a complete stop"};
cvar_t sv_wallfriction = {CF_SERVER | CF_NOTIFY, "sv_wallfriction", "1", "how much you slow down when sliding along a wall"};
cvar_t sv_wateraccelerate = {CF_SERVER, "sv_wateraccelerate", "-1", "rate at which a player accelerates to sv_maxspeed while in water, if less than 0 the sv_accelerate variable is used instead"};
cvar_t sv_waterfriction = {CF_SERVER | CF_NOTIFY, "sv_waterfriction","-1", "how fast you slow down in water, if less than 0 the sv_friction variable is used instead"};
cvar_t sv_warsowbunny_airforwardaccel = {CF_SERVER, "sv_warsowbunny_airforwardaccel", "1.00001", "how fast you accelerate until you reach sv_maxspeed"};
cvar_t sv_warsowbunny_accel = {CF_SERVER, "sv_warsowbunny_accel", "0.1585", "how fast you accelerate until after reaching sv_maxspeed (it gets harder as you near sv_warsowbunny_topspeed)"};
cvar_t sv_warsowbunny_topspeed = {CF_SERVER, "sv_warsowbunny_topspeed", "925", "soft speed limit (can get faster with rjs and on ramps)"};
cvar_t sv_warsowbunny_turnaccel = {CF_SERVER, "sv_warsowbunny_turnaccel", "0", "max sharpness of turns (also master switch for the sv_warsowbunny_* mode; set this to 9 to enable)"};
cvar_t sv_warsowbunny_backtosideratio = {CF_SERVER, "sv_warsowbunny_backtosideratio", "0.8", "lower values make it easier to change direction without losing speed; the drawback is \"understeering\" in sharp turns"};
cvar_t sv_onlycsqcnetworking = {CF_SERVER, "sv_onlycsqcnetworking", "0", "disables legacy entity networking code for higher performance (except on clients, which can still be legacy)"};
cvar_t sv_areadebug = {CF_SERVER, "sv_areadebug", "0", "disables physics culling for debugging purposes (only for development)"};

cvar_t sys_ticrate = {CF_SERVER | CF_ARCHIVE, "sys_ticrate","0.01388889", "how long a server frame is in seconds, 0.05 is 20fps server rate, 0.1 is 10fps (can not be set higher than 0.1), 0 runs as many server frames as possible (makes games against bots a little smoother, overwhelms network players), 1/72 matches QuakeWorld physics"};
cvar_t sv_maxphysicsframesperserverframe = {CF_SERVER, "sv_maxphysicsframesperserverframe","10", "maximum number of physics frames per server frame"};
cvar_t sv_lagreporting_always = {CF_SERVER, "sv_lagreporting_always", "0", "report lag even in singleplayer, listen, an empty dedicated server, or during intermission"};
cvar_t sv_lagreporting_strict = {CF_SERVER, "sv_lagreporting_strict", "0", "log any extra frames run to catch up after a holdup (only applies when sv_maxphysicsframesperserverframe > 1)"};
cvar_t sv_threaded = {CF_SERVER, "sv_threaded", "0", "enables a separate thread for server code, improving performance, especially when hosting a game while playing, EXPERIMENTAL, may be crashy"};

cvar_t teamplay = {CF_SERVER | CF_NOTIFY, "teamplay","0", "teamplay mode, values depend on mod but typically 0 = no teams, 1 = no team damage no self damage, 2 = team damage and self damage, some mods support 3 = no team damage but can damage self"};
cvar_t timelimit = {CF_SERVER | CF_NOTIFY, "timelimit","0", "ends level at this time (in minutes)"};

cvar_t sv_rollspeed = {CF_CLIENT, "sv_rollspeed", "200", "how much strafing is necessary to tilt the view"};
cvar_t sv_rollangle = {CF_CLIENT, "sv_rollangle", "2.0", "how much to tilt the view when strafing"};

cvar_t saved1 = {CF_SERVER | CF_ARCHIVE, "saved1", "0", "unused cvar in quake that is saved to config.cfg on exit, can be used by mods"};
cvar_t saved2 = {CF_SERVER | CF_ARCHIVE, "saved2", "0", "unused cvar in quake that is saved to config.cfg on exit, can be used by mods"};
cvar_t saved3 = {CF_SERVER | CF_ARCHIVE, "saved3", "0", "unused cvar in quake that is saved to config.cfg on exit, can be used by mods"};
cvar_t saved4 = {CF_SERVER | CF_ARCHIVE, "saved4", "0", "unused cvar in quake that is saved to config.cfg on exit, can be used by mods"};
cvar_t savedgamecfg = {CF_SERVER | CF_ARCHIVE, "savedgamecfg", "0", "unused cvar in quake that is saved to config.cfg on exit, can be used by mods"};
cvar_t scratch1 = {CF_SERVER, "scratch1", "0", "unused cvar in quake, can be used by mods"};
cvar_t scratch2 = {CF_SERVER,"scratch2", "0", "unused cvar in quake, can be used by mods"};
cvar_t scratch3 = {CF_SERVER, "scratch3", "0", "unused cvar in quake, can be used by mods"};
cvar_t scratch4 = {CF_SERVER, "scratch4", "0", "unused cvar in quake, can be used by mods"};
cvar_t temp1 = {CF_SERVER, "temp1","0", "general cvar for mods to use, in stock id1 this selects which death animation to use on players (0 = random death, other values select specific death scenes)"};

cvar_t nehx00 = {CF_SERVER, "nehx00", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx01 = {CF_SERVER, "nehx01", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx02 = {CF_SERVER, "nehx02", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx03 = {CF_SERVER, "nehx03", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx04 = {CF_SERVER, "nehx04", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx05 = {CF_SERVER, "nehx05", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx06 = {CF_SERVER, "nehx06", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx07 = {CF_SERVER, "nehx07", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx08 = {CF_SERVER, "nehx08", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx09 = {CF_SERVER, "nehx09", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx10 = {CF_SERVER, "nehx10", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx11 = {CF_SERVER, "nehx11", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx12 = {CF_SERVER, "nehx12", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx13 = {CF_SERVER, "nehx13", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx14 = {CF_SERVER, "nehx14", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx15 = {CF_SERVER, "nehx15", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx16 = {CF_SERVER, "nehx16", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx17 = {CF_SERVER, "nehx17", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx18 = {CF_SERVER, "nehx18", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t nehx19 = {CF_SERVER, "nehx19", "0", "nehahra data storage cvar (used in singleplayer)"};
cvar_t cutscene = {CF_SERVER, "cutscene", "1", "enables cutscenes in nehahra, can be used by other mods"};

cvar_t sv_autodemo_perclient = {CF_SERVER | CF_ARCHIVE, "sv_autodemo_perclient", "0", "set to 1 to enable autorecorded per-client demos (they'll start to record at the beginning of a match); set it to 2 to also record client->server packets (for debugging)"};
cvar_t sv_autodemo_perclient_nameformat = {CF_SERVER | CF_ARCHIVE, "sv_autodemo_perclient_nameformat", "sv_autodemos/%Y-%m-%d_%H-%M", "The format of the sv_autodemo_perclient filename, followed by the map name, the client number and the IP address + port number, separated by underscores (the date is encoded using strftime escapes)" };
cvar_t sv_autodemo_perclient_discardable = {CF_SERVER | CF_ARCHIVE, "sv_autodemo_perclient_discardable", "0", "Allow game code to decide whether a demo should be kept or discarded."};

cvar_t halflifebsp = {CF_SERVER, "halflifebsp", "0", "indicates the current map is hlbsp format (useful to know because of different bounding box sizes)"};
cvar_t sv_mapformat_is_quake2 = {CF_SERVER, "sv_mapformat_is_quake2", "0", "indicates the current map is q2bsp format (useful to know because of different entity behaviors, .frame on submodels and other things)"};
cvar_t sv_mapformat_is_quake3 = {CF_SERVER, "sv_mapformat_is_quake3", "0", "indicates the current map is q3bsp format (useful to know because of different entity behaviors)"};

cvar_t sv_writepicture_quality = {CF_SERVER | CF_ARCHIVE, "sv_writepicture_quality", "10", "WritePicture quality offset (higher means better quality, but slower)"};

cvar_t sv_sendentities_csqc_randomize_order = {CF_SERVER, "sv_sendentities_csqc_randomize_order", "1", "Randomize the order of sending CSQC entities (should behave better when packet size or bandwidth limits are exceeded)."};

server_t sv;
server_static_t svs;

mempool_t *sv_mempool = NULL;

extern cvar_t host_timescale;
extern float		scr_centertime_off;

// MUST match effectnameindex_t in client.h
static const char *standardeffectnames[EFFECT_TOTAL] =
{
	"",
	"TE_GUNSHOT",
	"TE_GUNSHOTQUAD",
	"TE_SPIKE",
	"TE_SPIKEQUAD",
	"TE_SUPERSPIKE",
	"TE_SUPERSPIKEQUAD",
	"TE_WIZSPIKE",
	"TE_KNIGHTSPIKE",
	"TE_EXPLOSION",
	"TE_EXPLOSIONQUAD",
	"TE_TAREXPLOSION",
	"TE_TELEPORT",
	"TE_LAVASPLASH",
	"TE_SMALLFLASH",
	"TE_FLAMEJET",
	"EF_FLAME",
	"TE_BLOOD",
	"TE_SPARK",
	"TE_PLASMABURN",
	"TE_TEI_G3",
	"TE_TEI_SMOKE",
	"TE_TEI_BIGEXPLOSION",
	"TE_TEI_PLASMAHIT",
	"EF_STARDUST",
	"TR_ROCKET",
	"TR_GRENADE",
	"TR_BLOOD",
	"TR_WIZSPIKE",
	"TR_SLIGHTBLOOD",
	"TR_KNIGHTSPIKE",
	"TR_VORESPIKE",
	"TR_NEHAHRASMOKE",
	"TR_NEXUIZPLASMA",
	"TR_GLOWTRAIL",
	"SVC_PARTICLE"
};


static void SV_CheckRequiredFuncs(prvm_prog_t *prog, const char *filename)
{
	// no required funcs?!
}

#define SV_REQFIELDS (sizeof(sv_reqfields) / sizeof(prvm_required_field_t))

prvm_required_field_t sv_reqfields[] =
{
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x)
#define PRVM_DECLARE_menuglobalvector(x)
#define PRVM_DECLARE_menuglobalstring(x)
#define PRVM_DECLARE_menuglobaledict(x)
#define PRVM_DECLARE_menuglobalfunction(x)
#define PRVM_DECLARE_serverfieldfloat(x) {ev_float, #x},
#define PRVM_DECLARE_serverfieldvector(x) {ev_vector, #x},
#define PRVM_DECLARE_serverfieldstring(x) {ev_string, #x},
#define PRVM_DECLARE_serverfieldedict(x) {ev_entity, #x},
#define PRVM_DECLARE_serverfieldfunction(x) {ev_function, #x},
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x)
#define PRVM_DECLARE_menufieldvector(x)
#define PRVM_DECLARE_menufieldstring(x)
#define PRVM_DECLARE_menufieldedict(x)
#define PRVM_DECLARE_menufieldfunction(x)
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

#define SV_REQGLOBALS (sizeof(sv_reqglobals) / sizeof(prvm_required_field_t))

prvm_required_field_t sv_reqglobals[] =
{
#define PRVM_DECLARE_serverglobalfloat(x) {ev_float, #x},
#define PRVM_DECLARE_serverglobalvector(x) {ev_vector, #x},
#define PRVM_DECLARE_serverglobalstring(x) {ev_string, #x},
#define PRVM_DECLARE_serverglobaledict(x) {ev_entity, #x},
#define PRVM_DECLARE_serverglobalfunction(x) {ev_function, #x},
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x)
#define PRVM_DECLARE_menuglobalvector(x)
#define PRVM_DECLARE_menuglobalstring(x)
#define PRVM_DECLARE_menuglobaledict(x)
#define PRVM_DECLARE_menuglobalfunction(x)
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x)
#define PRVM_DECLARE_menufieldvector(x)
#define PRVM_DECLARE_menufieldstring(x)
#define PRVM_DECLARE_menufieldedict(x)
#define PRVM_DECLARE_menufieldfunction(x)
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

static void Host_Timescale_c(cvar_t *var)
{
	if(var->value < 0.00001 && var->value != 0)
		Cvar_SetValueQuick(var, 0);
}

//============================================================================

static void SV_AreaStats_f(cmd_state_t *cmd)
{
	World_PrintAreaStats(&sv.world, "server");
}

static void SV_ServerOptions (void)
{
	int i;

	// general default
	svs.maxclients = 8;

// COMMANDLINEOPTION: Server: -dedicated [playerlimit] starts a dedicated server (with a command console), default playerlimit is 8
// COMMANDLINEOPTION: Server: -listen [playerlimit] starts a multiplayer server with graphical client, like singleplayer but other players can connect, default playerlimit is 8
	// if no client is in the executable or -dedicated is specified on
	// commandline, start a dedicated server
	i = Sys_CheckParm ("-dedicated");
	if (i || !cl_available)
	{
		// check for -dedicated specifying how many players
		if (i && i + 1 < sys.argc && atoi (sys.argv[i+1]) >= 1)
			svs.maxclients = atoi (sys.argv[i+1]);
		if (Sys_CheckParm ("-listen"))
			Con_Printf ("Only one of -dedicated or -listen can be specified\n");
		// default sv_public on for dedicated servers (often hosted by serious administrators), off for listen servers (often hosted by clueless users)
		Cvar_SetQuick(&sv_public, "1");
		Cvar_SetQuick(&sv_dedicated, "1");
	}
	else if (cl_available)
	{
		// client exists and not dedicated, check if -listen is specified
		cls.state = ca_disconnected;
		i = Sys_CheckParm ("-listen");
		if (i)
		{
			// default players unless specified
			if (i + 1 < sys.argc && atoi (sys.argv[i+1]) >= 1)
				svs.maxclients = atoi (sys.argv[i+1]);
		}
		else
		{
			// default players in some games, singleplayer in most
			if (gamemode != GAME_GOODVSBAD2 && !IS_NEXUIZ_DERIVED(gamemode) && gamemode != GAME_BATTLEMECH)
				svs.maxclients = 1;
		}
	}

	svs.maxclients = svs.maxclients_next = bound(1, svs.maxclients, MAX_SCOREBOARD);

	svs.clients = (client_t *)Mem_Alloc(sv_mempool, sizeof(client_t) * svs.maxclients);

	if (svs.maxclients > 1 && !deathmatch.integer && !coop.integer)
		Cvar_SetValueQuick(&deathmatch, 1);
}

/*
===============
SV_Init
===============
*/
void SV_Init (void)
{
	// init the csqc progs cvars, since they are updated/used by the server code
	// TODO: fix this since this is a quick hack to make some of [515]'s broken code run ;) [9/13/2006 Black]
	extern cvar_t csqc_progname;	//[515]: csqc crc check and right csprogs name according to progs.dat
	extern cvar_t csqc_progcrc;
	extern cvar_t csqc_progsize;
	extern cvar_t csqc_usedemoprogs;

	Cvar_RegisterVariable(&sv_worldmessage);
	Cvar_RegisterVariable(&sv_worldname);
	Cvar_RegisterVariable(&sv_worldnamenoextension);
	Cvar_RegisterVariable(&sv_worldbasename);

	Cvar_RegisterVariable (&csqc_progname);
	Cvar_RegisterVariable (&csqc_progcrc);
	Cvar_RegisterVariable (&csqc_progsize);
	Cvar_RegisterVariable (&csqc_usedemoprogs);

	Cmd_AddCommand(CF_SHARED, "sv_saveentfile", SV_SaveEntFile_f, "save map entities to .ent file (to allow external editing)");
	Cmd_AddCommand(CF_SHARED, "sv_areastats", SV_AreaStats_f, "prints statistics on entity culling during collision traces");
	Cmd_AddCommand(CF_CLIENT | CF_SERVER_FROM_CLIENT, "sv_startdownload", SV_StartDownload_f, "begins sending a file to the client (network protocol use only)");
	Cmd_AddCommand(CF_CLIENT | CF_SERVER_FROM_CLIENT, "download", SV_Download_f, "downloads a specified file from the server");

	Cvar_RegisterVariable (&sv_disablenotify);
	Cvar_RegisterVariable (&coop);
	Cvar_RegisterVariable (&deathmatch);
	Cvar_RegisterVariable (&fraglimit);
	Cvar_RegisterVariable (&gamecfg);
	Cvar_RegisterVariable (&noexit);
	Cvar_RegisterVariable (&nomonsters);
	Cvar_RegisterVariable (&pausable);
	Cvar_RegisterVariable (&pr_checkextension);
	Cvar_RegisterVariable (&samelevel);
	Cvar_RegisterVariable (&skill);
	Cvar_RegisterVariable (&host_timescale);
	Cvar_RegisterCallback (&host_timescale, Host_Timescale_c);
	Cvar_RegisterVirtual (&host_timescale, "slowmo");
	Cvar_RegisterVirtual (&host_timescale, "timescale");
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_aim);
	Cvar_RegisterVariable (&sv_airaccel_qw);
	Cvar_RegisterVariable (&sv_airaccel_qw_stretchfactor);
	Cvar_RegisterVariable (&sv_airaccel_sideways_friction);
	Cvar_RegisterVariable (&sv_airaccelerate);
	Cvar_RegisterVariable (&sv_airstopaccelerate);
	Cvar_RegisterVariable (&sv_airstrafeaccelerate);
	Cvar_RegisterVariable (&sv_maxairstrafespeed);
	Cvar_RegisterVariable (&sv_airstrafeaccel_qw);
	Cvar_RegisterVariable (&sv_airspeedlimit_nonqw);
	Cvar_RegisterVariable (&sv_aircontrol);
	Cvar_RegisterVariable (&sv_aircontrol_power);
	Cvar_RegisterVariable (&sv_aircontrol_penalty);
	Cvar_RegisterVariable (&sv_allowdownloads);
	Cvar_RegisterVariable (&sv_allowdownloads_archive);
	Cvar_RegisterVariable (&sv_allowdownloads_config);
	Cvar_RegisterVariable (&sv_allowdownloads_dlcache);
	Cvar_RegisterVariable (&sv_allowdownloads_inarchive);
	Cvar_RegisterVariable (&sv_areagrid_link_SOLID_NOT);
	Cvar_RegisterVariable (&sv_areagrid_mingridsize);
	Cvar_RegisterVariable (&sv_checkforpacketsduringsleep);
	Cvar_RegisterVariable (&sv_clmovement_enable);
	Cvar_RegisterVariable (&sv_clmovement_minping);
	Cvar_RegisterVariable (&sv_clmovement_minping_disabletime);
	Cvar_RegisterVariable (&sv_clmovement_inputtimeout);
	Cvar_RegisterVariable (&sv_cullentities_nevercullbmodels);
	Cvar_RegisterVariable (&sv_cullentities_pvs);
	Cvar_RegisterVariable (&sv_cullentities_stats);
	Cvar_RegisterVariable (&sv_cullentities_trace);
	Cvar_RegisterVariable (&sv_cullentities_trace_delay);
	Cvar_RegisterVariable (&sv_cullentities_trace_delay_players);
	Cvar_RegisterVariable (&sv_cullentities_trace_enlarge);
	Cvar_RegisterVariable (&sv_cullentities_trace_expand);
	Cvar_RegisterVariable (&sv_cullentities_trace_eyejitter);
	Cvar_RegisterVariable (&sv_cullentities_trace_entityocclusion);
	Cvar_RegisterVariable (&sv_cullentities_trace_prediction);
	Cvar_RegisterVariable (&sv_cullentities_trace_prediction_time);
	Cvar_RegisterVariable (&sv_cullentities_trace_samples);
	Cvar_RegisterVariable (&sv_cullentities_trace_samples_extra);
	Cvar_RegisterVariable (&sv_cullentities_trace_samples_players);
	Cvar_RegisterVariable (&sv_cullentities_trace_spectators);
	Cvar_RegisterVariable (&sv_debugmove);
	Cvar_RegisterVariable (&sv_dedicated);
	Cvar_RegisterVariable (&sv_echobprint);
	Cvar_RegisterVariable (&sv_edgefriction);
	Cvar_RegisterVariable (&sv_entpatch);
	Cvar_RegisterVariable (&sv_freezenonclients);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_gameplayfix_blowupfallenzombies);
	Cvar_RegisterVariable (&sv_gameplayfix_consistentplayerprethink);
	Cvar_RegisterVariable (&sv_gameplayfix_delayprojectiles);
	Cvar_RegisterVariable (&sv_gameplayfix_droptofloorstartsolid);
	Cvar_RegisterVariable (&sv_gameplayfix_droptofloorstartsolid_nudgetocorrect);
	Cvar_RegisterVariable (&sv_gameplayfix_easierwaterjump);
	Cvar_RegisterVariable (&sv_gameplayfix_findradiusdistancetobox);
	Cvar_RegisterVariable (&sv_gameplayfix_gravityunaffectedbyticrate);
	Cvar_RegisterVariable (&sv_gameplayfix_grenadebouncedownslopes);
	Cvar_RegisterVariable (&sv_gameplayfix_impactbeforeonground);
	Cvar_RegisterVariable (&sv_gameplayfix_multiplethinksperframe);
	Cvar_RegisterVariable (&sv_gameplayfix_noairborncorpse);
	Cvar_RegisterVariable (&sv_gameplayfix_noairborncorpse_allowsuspendeditems);
	Cvar_RegisterVariable (&sv_gameplayfix_nudgeoutofsolid);
	Cvar_RegisterVariable (&sv_gameplayfix_nudgeoutofsolid_separation);
	Cvar_RegisterVariable (&sv_gameplayfix_q2airaccelerate);
	Cvar_RegisterVariable (&sv_gameplayfix_nogravityonground);
	Cvar_RegisterVariable (&sv_gameplayfix_setmodelrealbox);
	Cvar_RegisterVariable (&sv_gameplayfix_slidemoveprojectiles);
	Cvar_RegisterVariable (&sv_gameplayfix_stepdown);
	Cvar_RegisterVariable (&sv_gameplayfix_stepmultipletimes);
	Cvar_RegisterVariable (&sv_gameplayfix_nostepmoveonsteepslopes);
	Cvar_RegisterVariable (&sv_gameplayfix_swiminbmodels);
	Cvar_RegisterVariable (&sv_gameplayfix_upwardvelocityclearsongroundflag);
	Cvar_RegisterVariable (&sv_gameplayfix_downtracesupportsongroundflag);
	Cvar_RegisterVariable (&sv_gameplayfix_q1bsptracelinereportstexture);
	Cvar_RegisterVariable (&sv_gameplayfix_unstickplayers);
	Cvar_RegisterVariable (&sv_gameplayfix_unstickentities);
	Cvar_RegisterVariable (&sv_gameplayfix_fixedcheckwatertransition);
	Cvar_RegisterVariable (&sv_gameplayfix_nosquashentities);
	Cvar_RegisterVariable (&sv_qcstats);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_init_frame_count);
	Cvar_RegisterVariable (&sv_idealpitchscale);
	Cvar_RegisterVariable (&sv_jumpstep);
	Cvar_RegisterVariable (&sv_jumpvelocity);
	Cvar_RegisterVariable (&sv_legacy_bbox_expand);
	Cvar_RegisterVariable (&sv_maxairspeed);
	Cvar_RegisterVariable (&sv_maxrate);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_nostep);
	Cvar_RegisterVariable (&sv_playerphysicsqc);
	Cvar_RegisterVariable (&sv_progs);
	Cvar_RegisterVariable (&sv_protocolname);
	Cvar_RegisterVariable (&sv_random_seed);
	Cvar_RegisterVariable (&host_limitlocal);
	Cvar_RegisterVirtual(&host_limitlocal, "sv_ratelimitlocalplayer");
	Cvar_RegisterVariable (&sv_sound_land);
	Cvar_RegisterVariable (&sv_sound_watersplash);
	Cvar_RegisterVariable (&sv_stepheight);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_wallfriction);
	Cvar_RegisterVariable (&sv_wateraccelerate);
	Cvar_RegisterVariable (&sv_waterfriction);
	Cvar_RegisterVariable (&sv_warsowbunny_airforwardaccel);
	Cvar_RegisterVariable (&sv_warsowbunny_accel);
	Cvar_RegisterVariable (&sv_warsowbunny_topspeed);
	Cvar_RegisterVariable (&sv_warsowbunny_turnaccel);
	Cvar_RegisterVariable (&sv_warsowbunny_backtosideratio);
	Cvar_RegisterVariable (&sv_onlycsqcnetworking);
	Cvar_RegisterVariable (&sv_areadebug);

	Cvar_RegisterVariable (&sys_ticrate);
	Cvar_RegisterVariable (&sv_maxphysicsframesperserverframe);
	Cvar_RegisterVariable (&sv_lagreporting_always);
	Cvar_RegisterVariable (&sv_lagreporting_strict);
	Cvar_RegisterVariable (&sv_threaded);

	Cvar_RegisterVariable (&teamplay);
	Cvar_RegisterVariable (&timelimit);

	Cvar_RegisterVariable (&sv_rollangle);
	Cvar_RegisterVariable (&sv_rollspeed);

	Cvar_RegisterVariable (&saved1);
	Cvar_RegisterVariable (&saved2);
	Cvar_RegisterVariable (&saved3);
	Cvar_RegisterVariable (&saved4);
	Cvar_RegisterVariable (&savedgamecfg);
	Cvar_RegisterVariable (&scratch1);
	Cvar_RegisterVariable (&scratch2);
	Cvar_RegisterVariable (&scratch3);
	Cvar_RegisterVariable (&scratch4);
	Cvar_RegisterVariable (&temp1);

	// LadyHavoc: Nehahra uses these to pass data around cutscene demos
	Cvar_RegisterVariable (&nehx00);
	Cvar_RegisterVariable (&nehx01);
	Cvar_RegisterVariable (&nehx02);
	Cvar_RegisterVariable (&nehx03);
	Cvar_RegisterVariable (&nehx04);
	Cvar_RegisterVariable (&nehx05);
	Cvar_RegisterVariable (&nehx06);
	Cvar_RegisterVariable (&nehx07);
	Cvar_RegisterVariable (&nehx08);
	Cvar_RegisterVariable (&nehx09);
	Cvar_RegisterVariable (&nehx10);
	Cvar_RegisterVariable (&nehx11);
	Cvar_RegisterVariable (&nehx12);
	Cvar_RegisterVariable (&nehx13);
	Cvar_RegisterVariable (&nehx14);
	Cvar_RegisterVariable (&nehx15);
	Cvar_RegisterVariable (&nehx16);
	Cvar_RegisterVariable (&nehx17);
	Cvar_RegisterVariable (&nehx18);
	Cvar_RegisterVariable (&nehx19);
	Cvar_RegisterVariable (&cutscene); // for Nehahra but useful to other mods as well

	Cvar_RegisterVariable (&sv_autodemo_perclient);
	Cvar_RegisterVariable (&sv_autodemo_perclient_nameformat);
	Cvar_RegisterVariable (&sv_autodemo_perclient_discardable);

	Cvar_RegisterVariable (&halflifebsp);
	Cvar_RegisterVariable (&sv_mapformat_is_quake2);
	Cvar_RegisterVariable (&sv_mapformat_is_quake3);

	Cvar_RegisterVariable (&sv_writepicture_quality);

	Cvar_RegisterVariable (&sv_sendentities_csqc_randomize_order);

	SV_InitOperatorCommands();
	host.hook.SV_Shutdown = SV_Shutdown;

	sv_mempool = Mem_AllocPool("server", 0, NULL);

	SV_ServerOptions();
	Cvar_Callback(&sv_netport);
}

static void SV_SaveEntFile_f(cmd_state_t *cmd)
{
	char vabuf[1024];
	if (!sv.active || !sv.worldmodel)
	{
		Con_Print("Not running a server\n");
		return;
	}
	FS_WriteFile(va(vabuf, sizeof(vabuf), "%s.ent", sv.worldnamenoextension), sv.worldmodel->brush.entities, (fs_offset_t)strlen(sv.worldmodel->brush.entities));
}

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo (client_t *client)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	char message[128];
	char vabuf[1024];

	// we know that this client has a netconnection and thus is not a bot

	// edicts get reallocated on level changes, so we need to update it here
	client->edict = PRVM_EDICT_NUM((client - svs.clients) + 1);

	// clear cached stuff that depends on the level
	client->weaponmodel[0] = 0;
	client->weaponmodelindex = 0;

	// LadyHavoc: clear entityframe tracking
	client->latestframenum = 0;

	// initialize the movetime, so a speedhack can't make use of the time before this client joined
	client->cmd.time = sv.time;

	if (client->entitydatabase)
		EntityFrame_FreeDatabase(client->entitydatabase);
	if (client->entitydatabase4)
		EntityFrame4_FreeDatabase(client->entitydatabase4);
	if (client->entitydatabase5)
		EntityFrame5_FreeDatabase(client->entitydatabase5);

	memset(client->stats, 0, sizeof(client->stats));
	memset(client->statsdeltabits, 0, sizeof(client->statsdeltabits));

	if (sv.protocol != PROTOCOL_QUAKE && sv.protocol != PROTOCOL_QUAKEDP && sv.protocol != PROTOCOL_NEHAHRAMOVIE && sv.protocol != PROTOCOL_NEHAHRABJP && sv.protocol != PROTOCOL_NEHAHRABJP2 && sv.protocol != PROTOCOL_NEHAHRABJP3)
	{
		if (sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3)
			client->entitydatabase = EntityFrame_AllocDatabase(sv_mempool);
		else if (sv.protocol == PROTOCOL_DARKPLACES4)
			client->entitydatabase4 = EntityFrame4_AllocDatabase(sv_mempool);
		else
			client->entitydatabase5 = EntityFrame5_AllocDatabase(sv_mempool);
	}

	// reset csqc entity versions
	for (i = 0;i < prog->max_edicts;i++)
	{
		client->csqcentityscope[i] = 0;
		client->csqcentitysendflags[i] = 0xFFFFFF;
	}
	for (i = 0;i < NUM_CSQCENTITYDB_FRAMES;i++)
	{
		client->csqcentityframehistory[i].num = 0;
		client->csqcentityframehistory[i].framenum = -1;
	}
	client->csqcnumedicts = 0;
	client->csqcentityframehistory_next = 0;

	SZ_Clear (&client->netconnection->message);
	MSG_WriteByte (&client->netconnection->message, svc_print);
	dpsnprintf (message, sizeof (message), "\nServer: %s (progs %i crc)\n", engineversion, prog->filecrc);
	MSG_WriteString (&client->netconnection->message,message);

	SV_StopDemoRecording(client); // to split up demos into different files
	if(sv_autodemo_perclient.integer)
	{
		char demofile[MAX_OSPATH];
		char ipaddress[MAX_QPATH];
		char timestring[128];
		size_t j;

		// start a new demo file
		LHNETADDRESS_ToString(&(client->netconnection->peeraddress), ipaddress, sizeof(ipaddress), true);
		for(j = 0; ipaddress[j]; ++j)
			if(!isalnum(ipaddress[j]))
				ipaddress[j] = '-';
		Sys_TimeString(timestring, sizeof(timestring), sv_autodemo_perclient_nameformat.string);
		dpsnprintf(demofile, sizeof(demofile), "%s_%s_%d_%s.dem", timestring,
				sv.worldbasename, PRVM_NUM_FOR_EDICT(client->edict), ipaddress);

		SV_StartDemoRecording(client, demofile, -1);
	}

	//[515]: init csprogs according to version of svprogs, check the crc, etc.
	if (sv.csqc_progname[0])
	{
		Con_DPrintf("sending csqc info to client (\"%s\" with size %i and crc %i)\n", sv.csqc_progname, sv.csqc_progsize, sv.csqc_progcrc);
		MSG_WriteByte (&client->netconnection->message, svc_stufftext);
		MSG_WriteString (&client->netconnection->message, va(vabuf, sizeof(vabuf), "csqc_progname %s\n", sv.csqc_progname));
		MSG_WriteByte (&client->netconnection->message, svc_stufftext);
		MSG_WriteString (&client->netconnection->message, va(vabuf, sizeof(vabuf), "csqc_progsize %i\n", sv.csqc_progsize));
		MSG_WriteByte (&client->netconnection->message, svc_stufftext);
		MSG_WriteString (&client->netconnection->message, va(vabuf, sizeof(vabuf), "csqc_progcrc %i\n", sv.csqc_progcrc));

		if(client->sv_demo_file != NULL)
		{
			int k;
			static char buf[NET_MAXMESSAGE];
			sizebuf_t sb;

			sb.data = (unsigned char *) buf;
			sb.maxsize = sizeof(buf);
			k = 0;
			while(MakeDownloadPacket(sv.csqc_progname, svs.csqc_progdata, sv.csqc_progsize, sv.csqc_progcrc, k++, &sb, sv.protocol))
				SV_WriteDemoMessage(client, &sb, false);
		}

		//[515]: init stufftext string (it is sent before svc_serverinfo)
		if (PRVM_GetString(prog, PRVM_serverglobalstring(SV_InitCmd)))
		{
			MSG_WriteByte (&client->netconnection->message, svc_stufftext);
			MSG_WriteString (&client->netconnection->message, va(vabuf, sizeof(vabuf), "%s\n", PRVM_GetString(prog, PRVM_serverglobalstring(SV_InitCmd))));
		}
	}

	//if (sv_allowdownloads.integer)
	// always send the info that the server supports the protocol, even if downloads are forbidden
	// only because of that, the CSQC exception can work
	{
		MSG_WriteByte (&client->netconnection->message, svc_stufftext);
		MSG_WriteString (&client->netconnection->message, "cl_serverextension_download 2\n");
	}

	// send at this time so it's guaranteed to get executed at the right time
	{
		client_t *save;
		save = host_client;
		host_client = client;
		Curl_SendRequirements();
		host_client = save;
	}

	MSG_WriteByte (&client->netconnection->message, svc_serverinfo);
	MSG_WriteLong (&client->netconnection->message, Protocol_NumberForEnum(sv.protocol));
	MSG_WriteByte (&client->netconnection->message, svs.maxclients);

	if (!coop.integer && deathmatch.integer)
		MSG_WriteByte (&client->netconnection->message, GAME_DEATHMATCH);
	else
		MSG_WriteByte (&client->netconnection->message, GAME_COOP);

	MSG_WriteString (&client->netconnection->message,PRVM_GetString(prog, PRVM_serveredictstring(prog->edicts, message)));

	for (i = 1;i < MAX_MODELS && sv.model_precache[i][0];i++)
		MSG_WriteString (&client->netconnection->message, sv.model_precache[i]);
	MSG_WriteByte (&client->netconnection->message, 0);

	for (i = 1;i < MAX_SOUNDS && sv.sound_precache[i][0];i++)
		MSG_WriteString (&client->netconnection->message, sv.sound_precache[i]);
	MSG_WriteByte (&client->netconnection->message, 0);

// send music
	MSG_WriteByte (&client->netconnection->message, svc_cdtrack);
	MSG_WriteByte (&client->netconnection->message, (int)PRVM_serveredictfloat(prog->edicts, sounds));
	MSG_WriteByte (&client->netconnection->message, (int)PRVM_serveredictfloat(prog->edicts, sounds));

// set view
// store this in clientcamera, too
	client->clientcamera = PRVM_NUM_FOR_EDICT(client->edict);
	MSG_WriteByte (&client->netconnection->message, svc_setview);
	MSG_WriteShort (&client->netconnection->message, client->clientcamera);

	MSG_WriteByte (&client->netconnection->message, svc_signonnum);
	MSG_WriteByte (&client->netconnection->message, 1);

	client->prespawned = false;		// need prespawn, spawn, etc
	client->spawned = false;		// need prespawn, spawn, etc
	client->begun = false;			// need prespawn, spawn, etc
	client->sendsignon = 1;			// send this message, and increment to 2, 2 will be set to 0 by the prespawn command

	// clear movement info until client enters the new level properly
	memset(&client->cmd, 0, sizeof(client->cmd));
	client->movesequence = 0;
	client->movement_highestsequence_seen = 0;
	memset(&client->movement_count, 0, sizeof(client->movement_count));
	client->ping = 0;

	// allow the client some time to send his keepalives, even if map loading took ages
	client->netconnection->timeout = host.realtime + net_connecttimeout.value;
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient (int clientnum, netconn_t *netconnection)
{
	prvm_prog_t *prog = SVVM_prog;
	client_t		*client;
	int				i;

	client = svs.clients + clientnum;

// set up the client_t
	if (sv.loadgame)
	{
		float backupparms[NUM_SPAWN_PARMS];
		memcpy(backupparms, client->spawn_parms, sizeof(backupparms));
		memset(client, 0, sizeof(*client));
		memcpy(client->spawn_parms, backupparms, sizeof(backupparms));
	}
	else
		memset(client, 0, sizeof(*client));
	client->active = true;
	client->netconnection = netconnection;

	Con_DPrintf("Client %s connected\n", client->netconnection ? client->netconnection->address : "botclient");

	if(client->netconnection && client->netconnection->crypto.authenticated)
	{
		Con_Printf("%s connection to %s has been established: client is %s@%s%.*s, I am %.*s@%s%.*s\n",
				client->netconnection->crypto.use_aes ? "Encrypted" : "Authenticated",
				client->netconnection->address,
				client->netconnection->crypto.client_idfp[0] ? client->netconnection->crypto.client_idfp : "-",
				(client->netconnection->crypto.client_issigned || !client->netconnection->crypto.client_keyfp[0]) ? "" : "~",
				crypto_keyfp_recommended_length, client->netconnection->crypto.client_keyfp[0] ? client->netconnection->crypto.client_keyfp : "-",
				crypto_keyfp_recommended_length, client->netconnection->crypto.server_idfp[0] ? client->netconnection->crypto.server_idfp : "-",
				(client->netconnection->crypto.server_issigned || !client->netconnection->crypto.server_keyfp[0]) ? "" : "~",
				crypto_keyfp_recommended_length, client->netconnection->crypto.server_keyfp[0] ? client->netconnection->crypto.server_keyfp : "-"
				);
	}

	dp_strlcpy(client->name, "unconnected", sizeof(client->name));
	dp_strlcpy(client->old_name, "unconnected", sizeof(client->old_name));
	client->prespawned = false;
	client->spawned = false;
	client->begun = false;
	client->edict = PRVM_EDICT_NUM(clientnum+1);
	if (client->netconnection)
		client->netconnection->message.allowoverflow = true;		// we can catch it
	// prepare the unreliable message buffer
	client->unreliablemsg.data = client->unreliablemsg_data;
	client->unreliablemsg.maxsize = sizeof(client->unreliablemsg_data);
	// updated by receiving "rate" command from client, this is also the default if not using a DP client
	client->rate = 1000000000;
	client->connecttime = host.realtime;

	if (!sv.loadgame)
	{
		// call the progs to get default spawn parms for the new client
		// set self to world to intentionally cause errors with broken SetNewParms code in some mods
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = 0;
		prog->ExecuteProgram(prog, PRVM_serverfunction(SetNewParms), "QC function SetNewParms is missing");
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			client->spawn_parms[i] = (&PRVM_serverglobalfloat(parm1))[i];

		// set up the entity for this client (including .colormap, .team, etc)
		PRVM_ED_ClearEdict(prog, client->edict);
	}

	// don't call SendServerinfo for a fresh botclient because its fields have
	// not been set up by the qc yet
	if (client->netconnection)
		SV_SendServerinfo (client);
	else
		client->prespawned = client->spawned = client->begun = true;
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (leaving = true), don't bother sending signofs
=====================
*/
void SV_DropClient(qbool leaving, const char *fmt, ... )
{
	prvm_prog_t *prog = SVVM_prog;
	int i;

	va_list argptr;
	char reason[512] = "";

	Con_Printf("Client \"%s\" dropped", host_client->name);

	if(fmt)
	{
		va_start(argptr, fmt);
		dpvsnprintf(reason, sizeof(reason), fmt, argptr);
		va_end(argptr);

		Con_Printf(" (%s)\n", reason);
	}
	else
	{
		Con_Printf(" \n");
	}

	SV_StopDemoRecording(host_client);

	// make sure edict is not corrupt (from a level change for example)
	host_client->edict = PRVM_EDICT_NUM(host_client - svs.clients + 1);

	if (host_client->netconnection)
	{
		// tell the client to be gone
		if (!leaving)
		{
			// LadyHavoc: no opportunity for resending, so use unreliable 3 times
			unsigned char bufdata[520]; // Disconnect reason string can be 512 characters
			sizebuf_t buf;
			memset(&buf, 0, sizeof(buf));
			buf.data = bufdata;
			buf.maxsize = sizeof(bufdata);
			MSG_WriteByte(&buf, svc_disconnect);
			if(fmt)
			{
				if(sv.protocol == PROTOCOL_DARKPLACES8)
					MSG_WriteString(&buf, reason);
				else
					SV_ClientPrintf("%s\n", reason);
			}
			NetConn_SendUnreliableMessage(host_client->netconnection, &buf, sv.protocol, 10000, 0, false);
			NetConn_SendUnreliableMessage(host_client->netconnection, &buf, sv.protocol, 10000, 0, false);
			NetConn_SendUnreliableMessage(host_client->netconnection, &buf, sv.protocol, 10000, 0, false);
		}
	}

	// call qc ClientDisconnect function
	// LadyHavoc: don't call QC if server is dead (avoids recursive
	// Host_Error in some mods when they run out of edicts)
	if (host_client->clientconnectcalled && sv.active && host_client->edict)
	{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
		int saveSelf = PRVM_serverglobaledict(self);
		host_client->clientconnectcalled = false;
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, PRVM_serverfunction(ClientDisconnect), "QC function ClientDisconnect is missing");
		PRVM_serverglobaledict(self) = saveSelf;
	}

	if (host_client->netconnection)
	{
		// break the net connection
		NetConn_Close(host_client->netconnection);
		host_client->netconnection = NULL;

		if(fmt)
			SV_BroadcastPrintf("\003^3%s left the game (%s)\n", host_client->name, reason);
		else
			SV_BroadcastPrintf("\003^3%s left the game\n", host_client->name);
	}

	// if a download is active, close it
	if (host_client->download_file)
	{
		Con_DPrintf("Download of %s aborted when %s dropped\n", host_client->download_name, host_client->name);
		FS_Close(host_client->download_file);
		host_client->download_file = NULL;
		host_client->download_name[0] = 0;
		host_client->download_expectedposition = 0;
		host_client->download_started = false;
	}

	// remove leaving player from scoreboard
	host_client->name[0] = 0;
	host_client->colors = 0;
	host_client->frags = 0;
	// send notification to all clients
	// get number of client manually just to make sure we get it right...
	i = host_client - svs.clients;
	MSG_WriteByte (&sv.reliable_datagram, svc_updatename);
	MSG_WriteByte (&sv.reliable_datagram, i);
	MSG_WriteString (&sv.reliable_datagram, host_client->name);
	MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
	MSG_WriteByte (&sv.reliable_datagram, i);
	MSG_WriteByte (&sv.reliable_datagram, host_client->colors);
	MSG_WriteByte (&sv.reliable_datagram, svc_updatefrags);
	MSG_WriteByte (&sv.reliable_datagram, i);
	MSG_WriteShort (&sv.reliable_datagram, host_client->frags);

	// free the client now
	if (host_client->entitydatabase)
		EntityFrame_FreeDatabase(host_client->entitydatabase);
	if (host_client->entitydatabase4)
		EntityFrame4_FreeDatabase(host_client->entitydatabase4);
	if (host_client->entitydatabase5)
		EntityFrame5_FreeDatabase(host_client->entitydatabase5);

	if (sv.active)
	{
		// clear a fields that matter to DP_SV_CLIENTNAME and DP_SV_CLIENTCOLORS, and also frags
		PRVM_ED_ClearEdict(prog, host_client->edict);
	}

	// clear the client struct (this sets active to false)
	memset(host_client, 0, sizeof(*host_client));

	// update server listing on the master because player count changed
	// (which the master uses for filtering empty/full servers)
	NetConn_Heartbeat(1);

	if (sv.loadgame)
	{
		for (i = 0;i < svs.maxclients;i++)
			if (svs.clients[i].active && !svs.clients[i].spawned)
				break;
		if (i == svs.maxclients)
		{
			Con_Printf("Loaded game, everyone rejoined - unpausing\n");
			sv.paused = sv.loadgame = false; // we're basically done with loading now
		}
	}
}

static void SV_StartDownload_f(cmd_state_t *cmd)
{
	if (host_client->download_file)
		host_client->download_started = true;
}

/*
 * Compression extension negotiation:
 *
 * Server to client:
 *   cl_serverextension_download 2
 *
 * Client to server:
 *   download <filename> <list of zero or more suppported compressions in order of preference>
 * e.g.
 *   download maps/map1.bsp lzo deflate huffman
 *
 * Server to client:
 *   cl_downloadbegin <compressed size> <filename> <compression method actually used>
 * e.g.
 *   cl_downloadbegin 123456 maps/map1.bsp deflate
 *
 * The server may choose not to compress the file by sending no compression name, like:
 *   cl_downloadbegin 345678 maps/map1.bsp
 *
 * NOTE: the "download" command may only specify compression algorithms if
 *       cl_serverextension_download is 2!
 *       If cl_serverextension_download has a different value, the client must
 *       assume this extension is not supported!
 */

static void Download_CheckExtensions(cmd_state_t *cmd)
{
	int i;
	int argc = Cmd_Argc(cmd);

	// first reset them all
	host_client->download_deflate = false;

	for(i = 2; i < argc; ++i)
	{
		if(!strcmp(Cmd_Argv(cmd, i), "deflate"))
		{
			host_client->download_deflate = true;
			break;
		}
	}
}

static void SV_Download_f(cmd_state_t *cmd)
{
	const char *whichpack, *whichpack2, *extension;
	qbool is_csqc; // so we need to check only once

	if (Cmd_Argc(cmd) < 2)
	{
		SV_ClientPrintf("usage: download <filename> {<extensions>}*\n");
		SV_ClientPrintf("       supported extensions: deflate\n");
		return;
	}

	if (FS_CheckNastyPath(Cmd_Argv(cmd, 1), false))
	{
		SV_ClientPrintf("Download rejected: nasty filename \"%s\"\n", Cmd_Argv(cmd, 1));
		return;
	}

	if (host_client->download_file)
	{
		// at this point we'll assume the previous download should be aborted
		Con_DPrintf("Download of %s aborted by %s starting a new download\n", host_client->download_name, host_client->name);
		SV_ClientCommands("\nstopdownload\n");

		// close the file and reset variables
		FS_Close(host_client->download_file);
		host_client->download_file = NULL;
		host_client->download_name[0] = 0;
		host_client->download_expectedposition = 0;
		host_client->download_started = false;
	}

	is_csqc = (sv.csqc_progname[0] && strcmp(Cmd_Argv(cmd, 1), sv.csqc_progname) == 0);

	if (!sv_allowdownloads.integer && !is_csqc)
	{
		SV_ClientPrintf("Downloads are disabled on this server\n");
		SV_ClientCommands("\nstopdownload\n");
		return;
	}

	Download_CheckExtensions(cmd);

	dp_strlcpy(host_client->download_name, Cmd_Argv(cmd, 1), sizeof(host_client->download_name));
	extension = FS_FileExtension(host_client->download_name);

	// host_client is asking to download a specified file
	if (developer_extra.integer)
		Con_DPrintf("Download request for %s by %s\n", host_client->download_name, host_client->name);

	if(is_csqc)
	{
		char extensions[MAX_QPATH]; // make sure this can hold all extensions
		extensions[0] = '\0';

		if(host_client->download_deflate)
			dp_strlcat(extensions, " deflate", sizeof(extensions));

		Con_DPrintf("Downloading %s to %s\n", host_client->download_name, host_client->name);

		if(host_client->download_deflate && svs.csqc_progdata_deflated)
			host_client->download_file = FS_FileFromData(svs.csqc_progdata_deflated, svs.csqc_progsize_deflated, true);
		else
			host_client->download_file = FS_FileFromData(svs.csqc_progdata, sv.csqc_progsize, true);

		// no, no space is needed between %s and %s :P
		SV_ClientCommands("\ncl_downloadbegin %i %s%s\n", (int)FS_FileSize(host_client->download_file), host_client->download_name, extensions);

		host_client->download_expectedposition = 0;
		host_client->download_started = false;
		host_client->sendsignon = true; // make sure this message is sent
		return;
	}

	if (!FS_FileExists(host_client->download_name))
	{
		SV_ClientPrintf("Download rejected: server does not have the file \"%s\"\nYou may need to separately download or purchase the data archives for this game/mod to get this file\n", host_client->download_name);
		SV_ClientCommands("\nstopdownload\n");
		return;
	}

	// check if the user is trying to download part of registered Quake(r)
	whichpack = FS_WhichPack(host_client->download_name);
	whichpack2 = FS_WhichPack("gfx/pop.lmp");
	if ((whichpack && whichpack2 && !strcasecmp(whichpack, whichpack2)) || FS_IsRegisteredQuakePack(host_client->download_name))
	{
		SV_ClientPrintf("Download rejected: file \"%s\" is part of registered Quake(r)\nYou must purchase Quake(r) from id Software or a retailer to get this file\nPlease go to http://www.idsoftware.com/games/quake/quake/index.php?game_section=buy\n", host_client->download_name);
		SV_ClientCommands("\nstopdownload\n");
		return;
	}

	// check if the server has forbidden archive downloads entirely
	if (!sv_allowdownloads_inarchive.integer)
	{
		whichpack = FS_WhichPack(host_client->download_name);
		if (whichpack)
		{
			SV_ClientPrintf("Download rejected: file \"%s\" is in an archive (\"%s\")\nYou must separately download or purchase the data archives for this game/mod to get this file\n", host_client->download_name, whichpack);
			SV_ClientCommands("\nstopdownload\n");
			return;
		}
	}

	if (!sv_allowdownloads_config.integer)
	{
		if (!strcasecmp(extension, "cfg"))
		{
			SV_ClientPrintf("Download rejected: file \"%s\" is a .cfg file which is forbidden for security reasons\nYou must separately download or purchase the data archives for this game/mod to get this file\n", host_client->download_name);
			SV_ClientCommands("\nstopdownload\n");
			return;
		}
	}

	if (!sv_allowdownloads_dlcache.integer)
	{
		if (!strncasecmp(host_client->download_name, "dlcache/", 8))
		{
			SV_ClientPrintf("Download rejected: file \"%s\" is in the dlcache/ directory which is forbidden for security reasons\nYou must separately download or purchase the data archives for this game/mod to get this file\n", host_client->download_name);
			SV_ClientCommands("\nstopdownload\n");
			return;
		}
	}

	if (!sv_allowdownloads_archive.integer)
	{
		if (!strcasecmp(extension, "pak") || !strcasecmp(extension, "pk3") || !strcasecmp(extension, "dpk"))
		{
			SV_ClientPrintf("Download rejected: file \"%s\" is an archive\nYou must separately download or purchase the data archives for this game/mod to get this file\n", host_client->download_name);
			SV_ClientCommands("\nstopdownload\n");
			return;
		}
	}

	host_client->download_file = FS_OpenVirtualFile(host_client->download_name, true);
	if (!host_client->download_file)
	{
		SV_ClientPrintf("Download rejected: server could not open the file \"%s\"\n", host_client->download_name);
		SV_ClientCommands("\nstopdownload\n");
		return;
	}

	if (FS_FileSize(host_client->download_file) > 1<<30)
	{
		SV_ClientPrintf("Download rejected: file \"%s\" is very large\n", host_client->download_name);
		SV_ClientCommands("\nstopdownload\n");
		FS_Close(host_client->download_file);
		host_client->download_file = NULL;
		return;
	}

	if (FS_FileSize(host_client->download_file) < 0)
	{
		SV_ClientPrintf("Download rejected: file \"%s\" is not a regular file\n", host_client->download_name);
		SV_ClientCommands("\nstopdownload\n");
		FS_Close(host_client->download_file);
		host_client->download_file = NULL;
		return;
	}

	Con_DPrintf("Downloading %s to %s\n", host_client->download_name, host_client->name);

	/*
	 * we can only do this if we would actually deflate on the fly
	 * which we do not (yet)!
	{
		char extensions[MAX_QPATH]; // make sure this can hold all extensions
		extensions[0] = '\0';

		if(host_client->download_deflate)
			strlcat(extensions, " deflate", sizeof(extensions));

		// no, no space is needed between %s and %s :P
		SV_ClientCommands("\ncl_downloadbegin %i %s%s\n", (int)FS_FileSize(host_client->download_file), host_client->download_name, extensions);
	}
	*/
	SV_ClientCommands("\ncl_downloadbegin %i %s\n", (int)FS_FileSize(host_client->download_file), host_client->download_name);

	host_client->download_expectedposition = 0;
	host_client->download_started = false;
	host_client->sendsignon = true; // make sure this message is sent

	// the rest of the download process is handled in SV_SendClientDatagram
	// and other code dealing with svc_downloaddata and clc_ackdownloaddata
	//
	// no svc_downloaddata messages will be sent until sv_startdownload is
	// sent by the client
}

/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex(const char *s, int precachemode)
{
	int i, limit = ((sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE) ? 256 : MAX_MODELS);
	char filename[MAX_QPATH];
	if (!s || !*s)
		return 0;
	// testing
	//if (precachemode == 2)
	//	return 0;
	dp_strlcpy(filename, s, sizeof(filename));
	for (i = 2;i < limit;i++)
	{
		if (!sv.model_precache[i][0])
		{
			if (precachemode)
			{
				if (sv.state != ss_loading && (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3 || sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4 || sv.protocol == PROTOCOL_DARKPLACES5))
				{
					Con_Printf("SV_ModelIndex(\"%s\"): precache_model can only be done in spawn functions\n", filename);
					return 0;
				}
				if (precachemode == 1)
					Con_Printf("SV_ModelIndex(\"%s\"): not precached (fix your code), precaching anyway\n", filename);
				dp_strlcpy(sv.model_precache[i], filename, sizeof(sv.model_precache[i]));
				if (sv.state == ss_loading)
				{
					// running from SV_SpawnServer which is launched from the client console command interpreter
					sv.models[i] = Mod_ForName (sv.model_precache[i], true, false, s[0] == '*' ? sv.worldname : NULL);
				}
				else
				{
					if (svs.threaded)
					{
						// this is running on the server thread, we can't load a model here (it would crash on renderer calls), so only look it up, the svc_precache will cause it to be loaded when it reaches the client
						sv.models[i] = Mod_FindName (sv.model_precache[i], s[0] == '*' ? sv.worldname : NULL);
					}
					else
					{
						// running single threaded, so we can load the model here
						sv.models[i] = Mod_ForName (sv.model_precache[i], true, false, s[0] == '*' ? sv.worldname : NULL);
					}
					MSG_WriteByte(&sv.reliable_datagram, svc_precache);
					MSG_WriteShort(&sv.reliable_datagram, i);
					MSG_WriteString(&sv.reliable_datagram, filename);
				}
				return i;
			}
			Con_Printf("SV_ModelIndex(\"%s\"): not precached\n", filename);
			return 0;
		}
		if (!strcmp(sv.model_precache[i], filename))
			return i;
	}
	Con_Printf("SV_ModelIndex(\"%s\"): i (%i) == MAX_MODELS (%i)\n", filename, i, MAX_MODELS);
	return 0;
}

/*
================
SV_SoundIndex

================
*/
int SV_SoundIndex(const char *s, int precachemode)
{
	int i, limit = ((sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP) ? 256 : MAX_SOUNDS);
	char filename[MAX_QPATH];
	if (!s || !*s)
		return 0;
	// testing
	//if (precachemode == 2)
	//	return 0;
	dp_strlcpy(filename, s, sizeof(filename));
	for (i = 1;i < limit;i++)
	{
		if (!sv.sound_precache[i][0])
		{
			if (precachemode)
			{
				if (sv.state != ss_loading && (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3 || sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4 || sv.protocol == PROTOCOL_DARKPLACES5))
				{
					Con_Printf("SV_SoundIndex(\"%s\"): precache_sound can only be done in spawn functions\n", filename);
					return 0;
				}
				if (precachemode == 1)
					Con_Printf("SV_SoundIndex(\"%s\"): not precached (fix your code), precaching anyway\n", filename);
				dp_strlcpy(sv.sound_precache[i], filename, sizeof(sv.sound_precache[i]));
				if (sv.state != ss_loading)
				{
					MSG_WriteByte(&sv.reliable_datagram, svc_precache);
					MSG_WriteShort(&sv.reliable_datagram, i + 32768);
					MSG_WriteString(&sv.reliable_datagram, filename);
				}
				return i;
			}
			Con_Printf("SV_SoundIndex(\"%s\"): not precached\n", filename);
			return 0;
		}
		if (!strcmp(sv.sound_precache[i], filename))
			return i;
	}
	Con_Printf("SV_SoundIndex(\"%s\"): i (%i) == MAX_SOUNDS (%i)\n", filename, i, MAX_SOUNDS);
	return 0;
}

/*
================
SV_ParticleEffectIndex

================
*/
int SV_ParticleEffectIndex(const char *name)
{
	int i, argc, linenumber, effectnameindex;
	int filepass;
	fs_offset_t filesize;
	unsigned char *filedata;
	const char *text;
	const char *textstart;
	//const char *textend;
	char argv[16][1024];
	char filename[MAX_QPATH];
	if (!sv.particleeffectnamesloaded)
	{
		sv.particleeffectnamesloaded = true;
		memset(sv.particleeffectname, 0, sizeof(sv.particleeffectname));
		for (i = 0;i < EFFECT_TOTAL;i++)
			dp_strlcpy(sv.particleeffectname[i], standardeffectnames[i], sizeof(sv.particleeffectname[i]));
		for (filepass = 0;;filepass++)
		{
			if (filepass == 0)
				dpsnprintf(filename, sizeof(filename), "effectinfo.txt");
			else if (filepass == 1)
				dpsnprintf(filename, sizeof(filename), "%s_effectinfo.txt", sv.worldnamenoextension);
			else
				break;
			filedata = FS_LoadFile(filename, tempmempool, true, &filesize);
			if (!filedata)
				continue;
			textstart = (const char *)filedata;
			//textend = (const char *)filedata + filesize;
			text = textstart;
			for (linenumber = 1;;linenumber++)
			{
				argc = 0;
				for (;;)
				{
					if (!COM_ParseToken_Simple(&text, true, false, true) || !strcmp(com_token, "\n"))
						break;
					if (argc < 16)
					{
						dp_strlcpy(argv[argc], com_token, sizeof(argv[argc]));
						argc++;
					}
				}
				if (com_token[0] == 0)
					break; // if the loop exited and it's not a \n, it's EOF
				if (argc < 1)
					continue;
				if (!strcmp(argv[0], "effect"))
				{
					if (argc == 2)
					{
						for (effectnameindex = 1;effectnameindex < MAX_PARTICLEEFFECTNAME;effectnameindex++)
						{
							if (sv.particleeffectname[effectnameindex][0])
							{
								if (!strcmp(sv.particleeffectname[effectnameindex], argv[1]))
									break;
							}
							else
							{
								dp_strlcpy(sv.particleeffectname[effectnameindex], argv[1], sizeof(sv.particleeffectname[effectnameindex]));
								break;
							}
						}
						// if we run out of names, abort
						if (effectnameindex == MAX_PARTICLEEFFECTNAME)
						{
							Con_Printf("%s:%i: too many effects!\n", filename, linenumber);
							break;
						}
					}
				}
			}
			Mem_Free(filedata);
		}
	}
	// search for the name
	for (effectnameindex = 1;effectnameindex < MAX_PARTICLEEFFECTNAME && sv.particleeffectname[effectnameindex][0];effectnameindex++)
		if (!strcmp(sv.particleeffectname[effectnameindex], name))
			return effectnameindex;
	// return 0 if we couldn't find it
	return 0;
}

model_t *SV_GetModelByIndex(int modelindex)
{
	return (modelindex > 0 && modelindex < MAX_MODELS) ? sv.models[modelindex] : NULL;
}

model_t *SV_GetModelFromEdict(prvm_edict_t *ed)
{
	prvm_prog_t *prog = SVVM_prog;
	int modelindex;
	if (!ed || ed->free)
		return NULL;
	modelindex = (int)PRVM_serveredictfloat(ed, modelindex);
	return (modelindex > 0 && modelindex < MAX_MODELS) ? sv.models[modelindex] : NULL;
}

/*
================
SV_CreateBaseline

================
*/
static void SV_CreateBaseline (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, entnum, large;
	prvm_edict_t *svent;

	// LadyHavoc: clear *all* baselines (not just active ones)
	for (entnum = 0;entnum < prog->max_edicts;entnum++)
	{
		// get the current server version
		svent = PRVM_EDICT_NUM(entnum);

		// LadyHavoc: always clear state values, whether the entity is in use or not
		svent->priv.server->baseline = defaultstate;

		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !PRVM_serveredictfloat(svent, modelindex))
			continue;

		// create entity baseline
		VectorCopy (PRVM_serveredictvector(svent, origin), svent->priv.server->baseline.origin);
		VectorCopy (PRVM_serveredictvector(svent, angles), svent->priv.server->baseline.angles);
		svent->priv.server->baseline.frame = (int)PRVM_serveredictfloat(svent, frame);
		svent->priv.server->baseline.skin = (int)PRVM_serveredictfloat(svent, skin);
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			svent->priv.server->baseline.colormap = entnum;
			svent->priv.server->baseline.modelindex = SV_ModelIndex("progs/player.mdl", 1);
		}
		else
		{
			svent->priv.server->baseline.colormap = 0;
			svent->priv.server->baseline.modelindex = (int)PRVM_serveredictfloat(svent, modelindex);
		}

		large = false;
		if (svent->priv.server->baseline.modelindex & 0xFF00 || svent->priv.server->baseline.frame & 0xFF00)
		{
			large = true;
			if (sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
				large = false;
		}

		// add to the message
		if (large)
			MSG_WriteByte (&sv.signon, svc_spawnbaseline2);
		else
			MSG_WriteByte (&sv.signon, svc_spawnbaseline);
		MSG_WriteShort (&sv.signon, entnum);

		if (large)
		{
			MSG_WriteShort (&sv.signon, svent->priv.server->baseline.modelindex);
			MSG_WriteShort (&sv.signon, svent->priv.server->baseline.frame);
		}
		else if (sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
		{
			MSG_WriteShort (&sv.signon, svent->priv.server->baseline.modelindex);
			MSG_WriteByte (&sv.signon, svent->priv.server->baseline.frame);
		}
		else
		{
			MSG_WriteByte (&sv.signon, svent->priv.server->baseline.modelindex);
			MSG_WriteByte (&sv.signon, svent->priv.server->baseline.frame);
		}
		MSG_WriteByte (&sv.signon, svent->priv.server->baseline.colormap);
		MSG_WriteByte (&sv.signon, svent->priv.server->baseline.skin);
		for (i=0 ; i<3 ; i++)
		{
			MSG_WriteCoord(&sv.signon, svent->priv.server->baseline.origin[i], sv.protocol);
			MSG_WriteAngle(&sv.signon, svent->priv.server->baseline.angles[i], sv.protocol);
		}
	}
}

/*
================
SV_Prepare_CSQC

Load csprogs.dat and comperss it so it doesn't need to be
reloaded on request.
================
*/
static void SV_Prepare_CSQC(void)
{
	fs_offset_t progsize;

	if(svs.csqc_progdata)
	{
		Con_DPrintf("Unloading old CSQC data.\n");
		Mem_Free(svs.csqc_progdata);
		if(svs.csqc_progdata_deflated)
			Mem_Free(svs.csqc_progdata_deflated);
	}

	svs.csqc_progdata = NULL;
	svs.csqc_progdata_deflated = NULL;

	sv.csqc_progname[0] = 0;
	svs.csqc_progdata = FS_LoadFile(csqc_progname.string, sv_mempool, false, &progsize);

	if(progsize > 0)
	{
		size_t deflated_size;

		sv.csqc_progsize = (int)progsize;
		sv.csqc_progcrc = CRC_Block(svs.csqc_progdata, progsize);
		dp_strlcpy(sv.csqc_progname, csqc_progname.string, sizeof(sv.csqc_progname));
		Con_DPrintf("server detected csqc progs file \"%s\" with size %i and crc %i\n", sv.csqc_progname, sv.csqc_progsize, sv.csqc_progcrc);

		Con_DPrint("Compressing csprogs.dat\n");
		//unsigned char *FS_Deflate(const unsigned char *data, size_t size, size_t *deflated_size, int level, mempool_t *mempool);
		svs.csqc_progdata_deflated = FS_Deflate(svs.csqc_progdata, progsize, &deflated_size, -1, sv_mempool);
		svs.csqc_progsize_deflated = (int)deflated_size;
		if(svs.csqc_progdata_deflated)
		{
			Con_DPrintf("Deflated: %g%%\n", 100.0 - 100.0 * (deflated_size / (float)progsize));
			Con_DPrintf("Uncompressed: %u\nCompressed:   %u\n", (unsigned)sv.csqc_progsize, (unsigned)svs.csqc_progsize_deflated);
		}
		else
			Con_DPrintf("Cannot compress - need zlib for this. Using uncompressed progs only.\n");
	}
}

/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int		i, j;

	svs.serverflags = (int)PRVM_serverglobalfloat(serverflags);

	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
	{
		if (!host_client->active)
			continue;

	// call the progs to get default spawn parms for the new client
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
		prog->ExecuteProgram(prog, PRVM_serverfunction(SetChangeParms), "QC function SetChangeParms is missing");
		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
			host_client->spawn_parms[j] = (&PRVM_serverglobalfloat(parm1))[j];
	}
}

// Returns 1 if we're singleplayer, > 1 if we're a listen server
int SV_IsLocalServer(void)
{
	return (host_isclient.integer && sv.active ? svs.maxclients : 0);
}

static void SV_VM_Shutdown(qbool prog_reset)
{
	prvm_prog_t *prog = SVVM_prog;

	if(prog->loaded && PRVM_serverfunction(SV_Shutdown))
	{
		func_t s = PRVM_serverfunction(SV_Shutdown);
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverfunction(SV_Shutdown) = 0; // prevent it from getting called again
		prog->ExecuteProgram(prog, s,"SV_Shutdown() required");
	}
	if (prog_reset)
		PRVM_Prog_Reset(prog);
}

/*
================
SV_SpawnServer

This is called at the start of each level
================
*/

void SV_SpawnServer (const char *map)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ent;
	int i;
	char *entities;
	model_t *worldmodel;
	char modelname[sizeof(sv.worldname)];
	const char *canonicalname;
	char vabuf[1024];

	Con_Printf("SpawnServer: %s\n", map);

	dpsnprintf (modelname, sizeof(modelname), "maps/%s.bsp", map);

	if (!(canonicalname = FS_FileExists(modelname)))
	{
		dpsnprintf (modelname, sizeof(modelname), "maps/%s", map);
		if (!(canonicalname = FS_FileExists(modelname)))
		{
			Con_Printf(CON_ERROR "SpawnServer: no map file named %s.bsp\n", modelname);
			return;
		}
	}
	// if it's not in a pak canonicalname will be the same pointer as modelname
	// if it's in a pak canonicalname may differ by case
	if (modelname != canonicalname)
		dp_strlcpy(modelname, canonicalname, sizeof(modelname));

//	SV_LockThreadMutex();

	if(!host_isclient.integer)
		Sys_MakeProcessNice();
	else
	{
		SCR_BeginLoadingPlaque(false);
		S_StopAllSounds();
	}

	if(sv.active)
		SV_VM_Shutdown(false);

	// free q3 shaders so that any newly downloaded shaders will be active
	Mod_FreeQ3Shaders();

	worldmodel = Mod_ForName(modelname, false, developer.integer > 0, NULL);
	if (!worldmodel || !worldmodel->TraceBox)
	{
		Con_Printf(CON_ERROR "Couldn't load map %s\n", modelname);

		if(!host_isclient.integer)
			Sys_MakeProcessMean();

//		SV_UnlockThreadMutex();

		return;
	}

	Collision_Cache_Reset(true);

	// let's not have any servers with no name
	if (hostname.string[0] == 0)
		Cvar_SetQuick(&hostname, "UNNAMED");
	scr_centertime_off = 0;

	svs.changelevel_issued = false;		// now safe to issue another

	// make the map a required file for clients
	Curl_ClearRequirements();
	Curl_RequireFile(modelname);

//
// tell all connected clients that we are going to a new level
//
	if (sv.active)
	{
		client_t *client;
		for (i = 0, client = svs.clients;i < svs.maxclients;i++, client++)
		{
			if (client->netconnection)
			{
				MSG_WriteByte(&client->netconnection->message, svc_stufftext);
				MSG_WriteString(&client->netconnection->message, "reconnect\n");
			}
		}
	}
	else
	{
		// open server port
		NetConn_OpenServerPorts(true);
	}

//
// make cvars consistant
//
	if (coop.integer)
		Cvar_SetValueQuick(&deathmatch, 0);
	// LadyHavoc: it can be useful to have skills outside the range 0-3...
	//current_skill = bound(0, (int)(skill.value + 0.5), 3);
	//Cvar_SetValue ("skill", (float)current_skill);
	current_skill = (int)(skill.value + 0.5);

//
// set up the new server
//
	memset (&sv, 0, sizeof(sv));

	// tell SV_Frame() to reset its timers
	sv.spawnframe = host.framecount;

	// if running a local client, make sure it doesn't try to access the last
	// level's data which is no longer valiud
	cls.signon = 0;

	Cvar_SetValueQuick(&halflifebsp, worldmodel->brush.ishlbsp);
	Cvar_SetValueQuick(&sv_mapformat_is_quake2, worldmodel->brush.isq2bsp);
	Cvar_SetValueQuick(&sv_mapformat_is_quake3, worldmodel->brush.isq3bsp);

	if(*sv_random_seed.string)
	{
		srand(sv_random_seed.integer);
		Con_Printf(CON_WARN "NOTE: random seed is %d; use for debugging/benchmarking only!\nUnset sv_random_seed to get real random numbers again.\n", sv_random_seed.integer);
	}

	SV_VM_Setup();

	sv.active = true;

	// set level base name variables for later use
	dp_strlcpy(sv.worldname, modelname, sizeof(sv.worldname));
	FS_StripExtension(sv.worldname, sv.worldnamenoextension, sizeof(sv.worldnamenoextension));
	dp_strlcpy(sv.worldbasename, !strncasecmp(sv.worldnamenoextension, "maps/", 5) ? sv.worldnamenoextension + 5 : sv.worldnamenoextension, sizeof(sv.worldbasename));
//	dp_strlcpy(sv.name, sv.worldbasename, sizeof (sv.name)); // TODO can we just remove this now?
	//Cvar_SetQuick(&sv_worldmessage, sv.worldmessage); // set later after QC is spawned
	Cvar_SetQuick(&sv_worldname, sv.worldname);
	Cvar_SetQuick(&sv_worldnamenoextension, sv.worldnamenoextension);
	Cvar_SetQuick(&sv_worldbasename, sv.worldbasename);

	sv.protocol = Protocol_EnumForName(sv_protocolname.string);
	if (sv.protocol == PROTOCOL_UNKNOWN)
	{
		char buffer[1024];
		Protocol_Names(buffer, sizeof(buffer));
		Con_Printf(CON_ERROR "Unknown sv_protocolname \"%s\", valid values are:\n%s\n", sv_protocolname.string, buffer);
		sv.protocol = PROTOCOL_QUAKE;
	}

// load progs to get entity field count
	//PR_LoadProgs ( sv_progs.string );

	sv.datagram.maxsize = sizeof(sv.datagram_buf);
	sv.datagram.cursize = 0;
	sv.datagram.data = sv.datagram_buf;

	sv.reliable_datagram.maxsize = sizeof(sv.reliable_datagram_buf);
	sv.reliable_datagram.cursize = 0;
	sv.reliable_datagram.data = sv.reliable_datagram_buf;

	sv.signon.maxsize = sizeof(sv.signon_buf);
	sv.signon.cursize = 0;
	sv.signon.data = sv.signon_buf;

// leave slots at start for clients only
	//prog->num_edicts = svs.maxclients+1;

	sv.state = ss_loading;
	prog->allowworldwrites = true;
	sv.paused = false;

	sv.time = 1.0;

	Mod_ClearUsed();
	worldmodel->used = true;

	sv.worldmodel = worldmodel;
	sv.models[1] = sv.worldmodel;

//
// clear world interaction links
//
	World_SetSize(&sv.world, sv.worldname, sv.worldmodel->normalmins, sv.worldmodel->normalmaxs, prog);
	World_Start(&sv.world);

	dp_strlcpy(sv.sound_precache[0], "", sizeof(sv.sound_precache[0]));

	dp_strlcpy(sv.model_precache[0], "", sizeof(sv.model_precache[0]));
	dp_strlcpy(sv.model_precache[1], sv.worldname, sizeof(sv.model_precache[1]));
	for (i = 1;i < sv.worldmodel->brush.numsubmodels && i+1 < MAX_MODELS;i++)
	{
		dpsnprintf(sv.model_precache[i+1], sizeof(sv.model_precache[i+1]), "*%i", i);
		sv.models[i+1] = Mod_ForName (sv.model_precache[i+1], false, false, sv.worldname);
	}
	if(i < sv.worldmodel->brush.numsubmodels)
		Con_Printf(CON_WARN "Too many submodels (MAX_MODELS is %i)\n", MAX_MODELS);

//
// load the rest of the entities
//
	// AK possible hack since num_edicts is still 0
	ent = PRVM_EDICT_NUM(0);
	memset (ent->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));
	ent->free = false;
	PRVM_serveredictstring(ent, model) = PRVM_SetEngineString(prog, sv.worldname);
	PRVM_serveredictfloat(ent, modelindex) = 1;		// world model
	PRVM_serveredictfloat(ent, solid) = SOLID_BSP;
	PRVM_serveredictfloat(ent, movetype) = MOVETYPE_PUSH;
	VectorCopy(sv.world.mins, PRVM_serveredictvector(ent, mins));
	VectorCopy(sv.world.maxs, PRVM_serveredictvector(ent, maxs));
	VectorCopy(sv.world.mins, PRVM_serveredictvector(ent, absmin));
	VectorCopy(sv.world.maxs, PRVM_serveredictvector(ent, absmax));

	if (coop.value)
		PRVM_serverglobalfloat(coop) = coop.integer;
	else
		PRVM_serverglobalfloat(deathmatch) = deathmatch.integer;

	PRVM_serverglobalstring(mapname) = PRVM_SetEngineString(prog, sv.worldbasename);

// serverflags are for cross level information (sigils)
	PRVM_serverglobalfloat(serverflags) = svs.serverflags;

	// we need to reset the spawned flag on all connected clients here so that
	// their thinks don't run during startup (before PutClientInServer)
	// we also need to set up the client entities now
	// and we need to set the ->edict pointers to point into the progs edicts
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
	{
		host_client->begun = false;
		host_client->edict = PRVM_EDICT_NUM(i + 1);
		PRVM_ED_ClearEdict(prog, host_client->edict);
	}

	// load replacement entity file if found
	if (sv_entpatch.integer && (entities = (char *)FS_LoadFile(va(vabuf, sizeof(vabuf), "%s.ent", sv.worldnamenoextension), tempmempool, true, NULL)))
	{
		Con_Printf("Loaded %s.ent\n", sv.worldnamenoextension);
		PRVM_ED_LoadFromFile(prog, entities);
		Mem_Free(entities);
	}
	else
		PRVM_ED_LoadFromFile(prog, sv.worldmodel->brush.entities);


	// LadyHavoc: clear world angles (to fix e3m3.bsp)
	VectorClear(PRVM_serveredictvector(prog->edicts, angles));

// all setup is completed, any further precache statements are errors
//	sv.state = ss_active; // LadyHavoc: workaround for svc_precache bug
	prog->allowworldwrites = false;

// run two frames to allow everything to settle
	sv.time = 1.0001;
	for (i = 0;i < sv_init_frame_count.integer;i++)
	{
		sv.frametime = 0.1;
		SV_Physics ();
	}

	// Once all init frames have been run, we consider svqc code fully initialized.
	prog->inittime = host.realtime;

	if(!host_isclient.integer)
		Mod_PurgeUnused();

// create a baseline for more efficient communications
	if (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
		SV_CreateBaseline ();

	sv.state = ss_active; // LadyHavoc: workaround for svc_precache bug

// send serverinfo to all connected clients, and set up botclients coming back from a level change
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
	{
		host_client->clientconnectcalled = false; // do NOT call ClientDisconnect if he drops before ClientConnect!
		if (!host_client->active)
			continue;
		if (host_client->netconnection)
			SV_SendServerinfo(host_client);
		else
		{
			int j;
			// if client is a botclient coming from a level change, we need to
			// set up client info that normally requires networking

			// copy spawn parms out of the client_t
			for (j=0 ; j< NUM_SPAWN_PARMS ; j++)
				(&PRVM_serverglobalfloat(parm1))[j] = host_client->spawn_parms[j];

			// call the spawn function
			host_client->clientconnectcalled = true;
			PRVM_serverglobalfloat(time) = sv.time;
			PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(host_client->edict);
			prog->ExecuteProgram(prog, PRVM_serverfunction(ClientConnect), "QC function ClientConnect is missing");
			prog->ExecuteProgram(prog, PRVM_serverfunction(PutClientInServer), "QC function PutClientInServer is missing");
			host_client->begun = true;
		}
	}

	// update the map title cvar (not related to filename)
	Cvar_SetQuick(&sv_worldmessage, PRVM_GetString(prog, PRVM_serveredictstring(prog->edicts, message)));

	Con_Printf("Server spawned.\n");
	NetConn_Heartbeat (2);

	if(!host_isclient.integer)
		Sys_MakeProcessMean();

//	SV_UnlockThreadMutex();
}

/*
==================
SV_Shutdown

This only happens at the end of a game, not between levels
==================
*/
void SV_Shutdown(void)
{
	int i;

	SV_LockThreadMutex();

	if (!sv.active)
		goto end;

	Con_DPrintf("SV_Shutdown\n");

	NetConn_Heartbeat(2);
	NetConn_Heartbeat(2);

	// make sure all the clients know we're disconnecting
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
		if (host_client->active)
			SV_DropClient(false, "Server shutting down"); // server shutdown

	SV_VM_Shutdown(true);

	NetConn_CloseServerPorts();

	sv.active = false;
//
// clear structures
//
	memset(&sv, 0, sizeof(sv));
	memset(svs.clients, 0, svs.maxclients*sizeof(client_t));
end:
	SV_UnlockThreadMutex();
}

/////////////////////////////////////////////////////
// SV VM stuff

static void SVVM_begin_increase_edicts(prvm_prog_t *prog)
{
	// links don't survive the transition, so unlink everything
	World_UnlinkAll(&sv.world);
}

static void SVVM_end_increase_edicts(prvm_prog_t *prog)
{
	int i;
	prvm_edict_t *ent;

	// link every entity except world
	for (i = 1, ent = prog->edicts;i < prog->num_edicts;i++, ent++)
		if (!ent->free)
			SV_LinkEdict(ent);
}

static void SVVM_init_edict(prvm_prog_t *prog, prvm_edict_t *e)
{
	// LadyHavoc: for consistency set these here
	int num = PRVM_NUM_FOR_EDICT(e) - 1;

	e->priv.server->move = false; // don't move on first frame

	if (num >= 0 && num < svs.maxclients)
	{
		// set colormap and team on newly created player entity
		PRVM_serveredictfloat(e, colormap) = num + 1;
		PRVM_serveredictfloat(e, team) = (svs.clients[num].colors & 15) + 1;
		// set netname/clientcolors back to client values so that
		// DP_SV_CLIENTNAME and DP_SV_CLIENTCOLORS will not immediately
		// reset them
		PRVM_serveredictstring(e, netname) = PRVM_SetEngineString(prog, svs.clients[num].name);
		PRVM_serveredictfloat(e, clientcolors) = svs.clients[num].colors;
		// NEXUIZ_PLAYERMODEL and NEXUIZ_PLAYERSKIN
		PRVM_serveredictstring(e, playermodel) = PRVM_SetEngineString(prog, svs.clients[num].playermodel);
		PRVM_serveredictstring(e, playerskin) = PRVM_SetEngineString(prog, svs.clients[num].playerskin);
		// Assign netaddress (IP Address, etc)
		if(svs.clients[num].netconnection != NULL)
		{
			// Acquire Readable Address
			LHNETADDRESS_ToString(&svs.clients[num].netconnection->peeraddress, svs.clients[num].netaddress, sizeof(svs.clients[num].netaddress), false);
			PRVM_serveredictstring(e, netaddress) = PRVM_SetEngineString(prog, svs.clients[num].netaddress);
		}
		else
			PRVM_serveredictstring(e, netaddress) = PRVM_SetEngineString(prog, "null/botclient");
		if(svs.clients[num].netconnection != NULL && svs.clients[num].netconnection->crypto.authenticated && svs.clients[num].netconnection->crypto.client_idfp[0])
			PRVM_serveredictstring(e, crypto_idfp) = PRVM_SetEngineString(prog, svs.clients[num].netconnection->crypto.client_idfp);
		else
			PRVM_serveredictstring(e, crypto_idfp) = 0;
		PRVM_serveredictfloat(e, crypto_idfp_signed) = (svs.clients[num].netconnection != NULL && svs.clients[num].netconnection->crypto.authenticated && svs.clients[num].netconnection->crypto.client_issigned);
		if(svs.clients[num].netconnection != NULL && svs.clients[num].netconnection->crypto.authenticated && svs.clients[num].netconnection->crypto.client_keyfp[0])
			PRVM_serveredictstring(e, crypto_keyfp) = PRVM_SetEngineString(prog, svs.clients[num].netconnection->crypto.client_keyfp);
		else
			PRVM_serveredictstring(e, crypto_keyfp) = 0;
		if(svs.clients[num].netconnection != NULL && svs.clients[num].netconnection->crypto.authenticated && svs.clients[num].netconnection->crypto.server_keyfp[0])
			PRVM_serveredictstring(e, crypto_mykeyfp) = PRVM_SetEngineString(prog, svs.clients[num].netconnection->crypto.server_keyfp);
		else
			PRVM_serveredictstring(e, crypto_mykeyfp) = 0;
		if(svs.clients[num].netconnection != NULL && svs.clients[num].netconnection->crypto.authenticated && svs.clients[num].netconnection->crypto.use_aes)
			PRVM_serveredictstring(e, crypto_encryptmethod) = PRVM_SetEngineString(prog, "AES128");
		else
			PRVM_serveredictstring(e, crypto_encryptmethod) = 0;
		if(svs.clients[num].netconnection != NULL && svs.clients[num].netconnection->crypto.authenticated)
			PRVM_serveredictstring(e, crypto_signmethod) = PRVM_SetEngineString(prog, "HMAC-SHA256");
		else
			PRVM_serveredictstring(e, crypto_signmethod) = 0;
	}
}

static void SVVM_free_edict(prvm_prog_t *prog, prvm_edict_t *ed)
{
	int i;
	int e;

	World_UnlinkEdict(ed);		// unlink from world bsp

	PRVM_serveredictstring(ed, model) = 0;
	PRVM_serveredictfloat(ed, takedamage) = 0;
	PRVM_serveredictfloat(ed, modelindex) = 0;
	PRVM_serveredictfloat(ed, colormap) = 0;
	PRVM_serveredictfloat(ed, skin) = 0;
	PRVM_serveredictfloat(ed, frame) = 0;
	VectorClear(PRVM_serveredictvector(ed, origin));
	VectorClear(PRVM_serveredictvector(ed, angles));
	PRVM_serveredictfloat(ed, nextthink) = -1;
	PRVM_serveredictfloat(ed, solid) = 0;

	VM_RemoveEdictSkeleton(prog, ed);
#ifdef USEODE
	World_Physics_RemoveFromEntity(&sv.world, ed);
	World_Physics_RemoveJointFromEntity(&sv.world, ed);
#endif
	// make sure csqc networking is aware of the removed entity
	e = PRVM_NUM_FOR_EDICT(ed);
	sv.csqcentityversion[e] = 0;
	for (i = 0;i < svs.maxclients;i++)
		svs.clients[i].csqcentitysendflags[e] = 0xFFFFFF;
}

static void SVVM_count_edicts(prvm_prog_t *prog)
{
	int		i;
	prvm_edict_t	*ent;
	int		active, models, solid, step;

	active = models = solid = step = 0;
	for (i=0 ; i<prog->num_edicts ; i++)
	{
		ent = PRVM_EDICT_NUM(i);
		if (ent->free)
			continue;
		active++;
		if (PRVM_serveredictfloat(ent, solid))
			solid++;
		if (PRVM_serveredictstring(ent, model))
			models++;
		if (PRVM_serveredictfloat(ent, movetype) == MOVETYPE_STEP)
			step++;
	}

	Con_Printf("num_edicts:%3i\n", prog->num_edicts);
	Con_Printf("active    :%3i\n", active);
	Con_Printf("view      :%3i\n", models);
	Con_Printf("touch     :%3i\n", solid);
	Con_Printf("step      :%3i\n", step);
}

static qbool SVVM_load_edict(prvm_prog_t *prog, prvm_edict_t *ent)
{
	// remove things from different skill levels or deathmatch
	if (gamemode != GAME_TRANSFUSION) //Transfusion does this in QC
	{
		if (deathmatch.integer)
		{
			if (((int)PRVM_serveredictfloat(ent, spawnflags) & SPAWNFLAG_NOT_DEATHMATCH))
			{
				return false;
			}
		}
		else if ((current_skill <= 0 && ((int)PRVM_serveredictfloat(ent, spawnflags) & SPAWNFLAG_NOT_EASY  ))
			|| (current_skill == 1 && ((int)PRVM_serveredictfloat(ent, spawnflags) & SPAWNFLAG_NOT_MEDIUM))
			|| (current_skill >= 2 && ((int)PRVM_serveredictfloat(ent, spawnflags) & SPAWNFLAG_NOT_HARD  )))
		{
			return false;
		}
	}
	return true;
}

static void SV_VM_Setup(void)
{
	prvm_prog_t *prog = SVVM_prog;
	PRVM_Prog_Init(prog, cmd_local);

	// allocate the mempools
	// TODO: move the magic numbers/constants into #defines [9/13/2006 Black]
	prog->progs_mempool = Mem_AllocPool("Server Progs", 0, NULL);
	prog->builtins = vm_sv_builtins;
	prog->numbuiltins = vm_sv_numbuiltins;
	prog->max_edicts = 512;
	if (sv.protocol == PROTOCOL_QUAKE)
		prog->limit_edicts = 640; // before quake mission pack 1 this was 512
	else if (sv.protocol == PROTOCOL_QUAKEDP)
		prog->limit_edicts = 2048; // guessing
	else if (sv.protocol == PROTOCOL_NEHAHRAMOVIE)
		prog->limit_edicts = 2048; // guessing!
	else if (sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
		prog->limit_edicts = 4096; // guessing!
	else
		prog->limit_edicts = MAX_EDICTS;
	prog->reserved_edicts = svs.maxclients;
	prog->edictprivate_size = sizeof(edict_engineprivate_t);
	prog->name = "server";
	prog->extensionstring = vm_sv_extensions;
	prog->loadintoworld = true;

	// all callbacks must be defined (pointers are not checked before calling)
	prog->begin_increase_edicts = SVVM_begin_increase_edicts;
	prog->end_increase_edicts   = SVVM_end_increase_edicts;
	prog->init_edict            = SVVM_init_edict;
	prog->free_edict            = SVVM_free_edict;
	prog->count_edicts          = SVVM_count_edicts;
	prog->load_edict            = SVVM_load_edict;
	prog->init_cmd              = SVVM_init_cmd;
	prog->reset_cmd             = SVVM_reset_cmd;
	prog->error_cmd             = Host_Error;
	prog->ExecuteProgram        = SVVM_ExecuteProgram;

	PRVM_Prog_Load(prog, sv_progs.string, NULL, 0, SV_CheckRequiredFuncs, SV_REQFIELDS, sv_reqfields, SV_REQGLOBALS, sv_reqglobals);

	// some mods compiled with scrambling compilers lack certain critical
	// global names and field names such as "self" and "time" and "nextthink"
	// so we have to set these offsets manually, matching the entvars_t
	// but we only do this if the prog header crc matches, otherwise it's totally freeform
	if (prog->progs_crc == PROGHEADER_CRC || prog->progs_crc == PROGHEADER_CRC_TENEBRAE)
	{
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, modelindex);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, absmin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, absmax);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ltime);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, movetype);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, solid);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, origin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, oldorigin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, velocity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, angles);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, avelocity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, punchangle);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, classname);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, model);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, frame);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, skin);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, effects);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, mins);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, maxs);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, size);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, touch);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, use);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, think);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, blocked);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, nextthink);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, groundentity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, health);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, frags);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, weapon);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, weaponmodel);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, weaponframe);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, currentammo);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_shells);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_nails);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_rockets);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ammo_cells);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, items);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, takedamage);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, chain);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, deadflag);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, view_ofs);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, button0);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, button1);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, button2);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, impulse);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, fixangle);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, v_angle);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, idealpitch);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, netname);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, enemy);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, flags);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, colormap);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, team);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, max_health);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, teleport_time);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, armortype);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, armorvalue);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, waterlevel);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, watertype);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, ideal_yaw);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, yaw_speed);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, aiment);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, goalentity);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, spawnflags);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, target);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, targetname);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, dmg_take);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, dmg_save);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, dmg_inflictor);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, owner);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, movedir);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, message);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, sounds);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise1);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise2);
		PRVM_ED_FindFieldOffset_FromStruct(entvars_t, noise3);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, self);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, other);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, world);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, time);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, frametime);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, force_retouch);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, mapname);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, deathmatch);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, coop);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, teamplay);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, serverflags);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, total_secrets);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, total_monsters);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, found_secrets);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, killed_monsters);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm1);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm2);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm3);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm4);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm5);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm6);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm7);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm8);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm9);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm10);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm11);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm12);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm13);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm14);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm15);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, parm16);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, v_forward);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, v_up);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, v_right);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_allsolid);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_startsolid);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_fraction);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_endpos);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_plane_normal);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_plane_dist);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_ent);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_inopen);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, trace_inwater);
		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, msg_entity);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, main);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, StartFrame);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, PlayerPreThink);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, PlayerPostThink);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, ClientKill);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, ClientConnect);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, PutClientInServer);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, ClientDisconnect);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, SetNewParms);
//		PRVM_ED_FindGlobalOffset_FromStruct(globalvars_t, SetChangeParms);
	}
	else
		Con_DPrintf("%s: %s system vars have been modified (CRC %i != engine %i), will not load in other engines\n", prog->name, sv_progs.string, prog->progs_crc, PROGHEADER_CRC);

	// OP_STATE is always supported on server because we add fields/globals for it
	prog->flag |= PRVM_OP_STATE;

	VM_CustomStats_Clear();//[515]: csqc

	SV_Prepare_CSQC();
}

static void SV_CheckTimeouts(void)
{
	int i;

	// never timeout loopback connections
	for (i = (host_isclient.integer ? 1 : 0), host_client = &svs.clients[i]; i < svs.maxclients; i++, host_client++)
		if (host_client->netconnection && host.realtime > host_client->netconnection->timeout)
			SV_DropClient(false, "Timed out");
}

/*
==================
SV_TimeReport

Returns a time report string, for example for
==================
*/
const char *SV_TimingReport(char *buf, size_t buflen)
{
	return va(buf, buflen, "%.1f%% CPU, %.2f%% lost, offset avg %.1fms, max %.1fms, sdev %.1fms", sv.perf_cpuload * 100, sv.perf_lost * 100, sv.perf_offset_avg * 1000, sv.perf_offset_max * 1000, sv.perf_offset_sdev * 1000);
}

extern cvar_t host_framerate;
double SV_Frame(double time)
{
	static double sv_timer;
	int i;
	char vabuf[1024];
	qbool reporting = false;

	// reset timer after level change
	if (host.framecount == sv.spawnframe || host.framecount == sv.spawnframe + 1)
		sv_timer = time = host.sleeptime = 0;

	if (!svs.threaded)
	{
		prvm_prog_t *prog = SVVM_prog;

		sv.perf_acc_sleeptime += host.sleeptime;
		sv.perf_acc_realtime += time;

		if (sv_lagreporting_always.integer)
			reporting = true;
		// bones_was_here: intermission_running isn't declared in dpdefs, but it's used by
		// id1 Quake, all Quake mods (afaict), Nexuiz and Xonotic.
		else if (cls.state == ca_dedicated && !PRVM_serverglobalfloat(intermission_running))
		{
			// Report lag if there's players, so they know it wasn't the network or their machine
			for (i = 0; i < svs.maxclients; ++i)
			{
				if (svs.clients[i].begun && svs.clients[i].netconnection)
				{
					reporting = true;
					break;
				}
			}
		}

		if(sv.perf_acc_realtime > 5)
		{
			sv.perf_cpuload = 1 - sv.perf_acc_sleeptime / sv.perf_acc_realtime;
			sv.perf_lost = sv.perf_acc_lost / sv.perf_acc_realtime;

			if(sv.perf_acc_offset_samples > 0)
			{
				sv.perf_offset_max = sv.perf_acc_offset_max;
				sv.perf_offset_avg = sv.perf_acc_offset / sv.perf_acc_offset_samples;
				sv.perf_offset_sdev = sv.perf_acc_offset_squared / sv.perf_acc_offset_samples - sv.perf_offset_avg * sv.perf_offset_avg;
				sv.perf_offset_sdev = sv.perf_offset_sdev > 0 ? sqrt(sv.perf_offset_sdev) : 0;
			}

			if (sv.perf_lost > 0 && reporting)
				SV_BroadcastPrintf(CON_WARN "Server lag report: %s\n", SV_TimingReport(vabuf, sizeof(vabuf)));

			sv.perf_acc_realtime = sv.perf_acc_sleeptime =
			sv.perf_acc_lost = sv.perf_acc_offset =
			sv.perf_acc_offset_squared = sv.perf_acc_offset_max =
			sv.perf_acc_offset_samples = 0;
		}

		/*
		 * Receive packets on each main loop iteration, as the main loop may
		 * be undersleeping due to select() detecting a new packet
		 */
		if (sv.active)
		{
			NetConn_ServerFrame();
			SV_CheckTimeouts();
		}
	}

	/*
	 * If the accumulator hasn't become positive, don't
	 * run the frame. Everything that happens before this
	 * point will happen even if we're sleeping this frame.
	 */
	if((sv_timer += time) < 0)
		return sv_timer;

	// limit the frametime steps to no more than 100ms each
	if (sv_timer > 0.1)
	{
		if (!svs.threaded)
			sv.perf_acc_lost += (sv_timer - 0.1);
		sv_timer = 0.1;
	}

	if (sv.active && sv_timer > 0 && !svs.threaded)
	{
		/*
		 * Execute one or more server frames, with an upper limit on how much
		 * execution time to spend on server frames to avoid freezing the game if
		 * the server is overloaded. This execution time limit means the game will
		 * slow down if the server is taking too long.
		 */
		int framecount, framelimit = 1;
		double advancetime, aborttime = 0;
		float offset;
		prvm_prog_t *prog = SVVM_prog;

		// run the world state
		// don't allow simulation to run too fast or too slow or logic glitches can occur

		// stop running server frames if the wall time reaches this value
		if (sys_ticrate.value <= 0)
			advancetime = sv_timer;
		else
		{
			advancetime = sys_ticrate.value;
			// listen servers can run multiple server frames per client frame
			if (sv_maxphysicsframesperserverframe.integer > 0)
				framelimit = sv_maxphysicsframesperserverframe.integer;
			aborttime = Sys_DirtyTime() + 0.1;
		}

		if(host_timescale.value > 0 && host_timescale.value < 1)
			advancetime = min(advancetime, 0.1 / host_timescale.value);
		else
			advancetime = min(advancetime, 0.1);

		if(advancetime > 0)
		{
			offset = Sys_DirtyTime() - host.dirtytime;
			if (offset < 0 || offset >= 1800)
				offset = 0;

			offset += sv_timer;
			++sv.perf_acc_offset_samples;
			sv.perf_acc_offset += offset;
			sv.perf_acc_offset_squared += offset * offset;

			if(sv.perf_acc_offset_max < offset)
				sv.perf_acc_offset_max = offset;
		}

		// only advance time if not paused
		// the game also pauses in singleplayer when menu or console is used
		sv.frametime = advancetime * host_timescale.value;
		if (host_framerate.value)
			sv.frametime = host_framerate.value;
		if (sv.paused || host.paused)
			sv.frametime = 0;

		for (framecount = 0; framecount < framelimit && sv_timer > 0; framecount++)
		{
			sv_timer -= advancetime;

			// move things around and think unless paused
			if (sv.frametime)
				SV_Physics();

			// if this server frame took too long, break out of the loop
			if (framelimit > 1 && Sys_DirtyTime() >= aborttime)
				break;
		}

		if (framecount > 1 && sv_lagreporting_strict.integer && reporting)
			SV_BroadcastPrintf(CON_WARN "Server lag report: caught up %.1fms by running %d extra frames\n", advancetime * (framecount - 1) * 1000, framecount - 1);

		R_TimeReport("serverphysics");

		// send all messages to the clients
		SV_SendClientMessages();

		if (sv.paused == 1 && host.realtime > sv.pausedstart && sv.pausedstart > 0) {
			prog->globals.fp[OFS_PARM0] = host.realtime - sv.pausedstart;
			PRVM_serverglobalfloat(time) = sv.time;
			prog->ExecuteProgram(prog, PRVM_serverfunction(SV_PausedTic), "QC function SV_PausedTic is missing");
		}

		// send an heartbeat if enough time has passed since the last one
		NetConn_Heartbeat(0);
		R_TimeReport("servernetwork");
	}
	else
	{
		// don't let r_speeds display jump around
		R_TimeReport("serverphysics");
		R_TimeReport("servernetwork");
	}

	// if there is some time remaining from this frame, reset the timer
	if (sv_timer >= 0)
	{
		if (!svs.threaded)
			sv.perf_acc_lost += sv_timer;
		sv_timer = 0;
	}

	return sv_timer;
}

static int SV_ThreadFunc(void *voiddata)
{
	prvm_prog_t *prog = SVVM_prog;
	qbool playing = false;
	double sv_timer = 0;
	double sv_deltarealtime, sv_oldrealtime, sv_realtime;
	int i;
	char vabuf[1024];
	sv_realtime = Sys_DirtyTime();
	while (!svs.threadstop)
	{
		// FIXME: we need to handle Host_Error in the server thread somehow
//		if (setjmp(sv_abortframe))
//			continue;			// something bad happened in the server game

		sv_oldrealtime = sv_realtime;
		sv_realtime = Sys_DirtyTime();
		sv_deltarealtime = sv_realtime - sv_oldrealtime;
		if (sv_deltarealtime < 0 || sv_deltarealtime >= 1800) sv_deltarealtime = 0;

		sv_timer += sv_deltarealtime;

		sv.perf_acc_realtime += sv_deltarealtime;

		// at this point we start doing real server work, and must block on any client activity pertaining to the server (such as executing SV_SpawnServer)
		SV_LockThreadMutex();

		// Look for clients who have spawned
		playing = false;
		if (sv.active)
			for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
				if(host_client->begun)
					if(host_client->netconnection)
						playing = true;
		if(sv.time < 10)
		{
			// don't accumulate time for the first 10 seconds of a match
			// so things can settle
			sv.perf_acc_realtime = sv.perf_acc_sleeptime = sv.perf_acc_lost = sv.perf_acc_offset = sv.perf_acc_offset_squared = sv.perf_acc_offset_max = sv.perf_acc_offset_samples = 0;
		}
		else if(sv.perf_acc_realtime > 5)
		{
			sv.perf_cpuload = 1 - sv.perf_acc_sleeptime / sv.perf_acc_realtime;
			sv.perf_lost = sv.perf_acc_lost / sv.perf_acc_realtime;
			if(sv.perf_acc_offset_samples > 0)
			{
				sv.perf_offset_max = sv.perf_acc_offset_max;
				sv.perf_offset_avg = sv.perf_acc_offset / sv.perf_acc_offset_samples;
				sv.perf_offset_sdev = sqrt(sv.perf_acc_offset_squared / sv.perf_acc_offset_samples - sv.perf_offset_avg * sv.perf_offset_avg);
			}
			if(sv.perf_lost > 0 && developer_extra.integer)
				if(playing)
					Con_DPrintf("Server can't keep up: %s\n", SV_TimingReport(vabuf, sizeof(vabuf)));
			sv.perf_acc_realtime = sv.perf_acc_sleeptime = sv.perf_acc_lost = sv.perf_acc_offset = sv.perf_acc_offset_squared = sv.perf_acc_offset_max = sv.perf_acc_offset_samples = 0;
		}

		// get new packets
		if (sv.active)
		{
			NetConn_ServerFrame();
			SV_CheckTimeouts();
		}

		// if the accumulators haven't become positive yet, wait a while
		if (sv_timer < 0)
		{
			SV_UnlockThreadMutex(); // don't keep mutex locked while sleeping
			sv.perf_acc_sleeptime += Sys_Sleep(-sv_timer);
			continue;
		}

		if (sv.active && sv_timer > 0)
		{
			// execute one server frame
			double advancetime;
			float offset;

			if (sys_ticrate.value <= 0)
				advancetime = min(sv_timer, 0.1); // don't step more than 100ms
			else
				advancetime = sys_ticrate.value;

			if(advancetime > 0)
			{
				offset = sv_timer + (Sys_DirtyTime() - sv_realtime); // LadyHavoc: FIXME: I don't understand this line
				++sv.perf_acc_offset_samples;
				sv.perf_acc_offset += offset;
				sv.perf_acc_offset_squared += offset * offset;
				if(sv.perf_acc_offset_max < offset)
					sv.perf_acc_offset_max = offset;
			}

			// only advance time if not paused
			// the game also pauses in singleplayer when menu or console is used
			sv.frametime = advancetime * host_timescale.value;
			if (host_framerate.value)
				sv.frametime = host_framerate.value;
			if (sv.paused || host.paused)
				sv.frametime = 0;

			sv_timer -= advancetime;

			// move things around and think unless paused
			if (sv.frametime)
				SV_Physics();

			// send all messages to the clients
			SV_SendClientMessages();

			if (sv.paused == 1 && sv_realtime > sv.pausedstart && sv.pausedstart > 0)
			{
				PRVM_serverglobalfloat(time) = sv.time;
				prog->globals.fp[OFS_PARM0] = sv_realtime - sv.pausedstart;
				prog->ExecuteProgram(prog, PRVM_serverfunction(SV_PausedTic), "QC function SV_PausedTic is missing");
			}

			// send an heartbeat if enough time has passed since the last one
			NetConn_Heartbeat(0);

		}

		// we're back to safe code now
		SV_UnlockThreadMutex();

		// if there is some time remaining from this frame, reset the timers
		if (sv_timer >= 0)
		{
			sv.perf_acc_lost += sv_timer;
			sv_timer = 0;
		}
	}
	return 0;
}

void SV_StartThread(void)
{
	if (!sv_threaded.integer || !Thread_HasThreads())
		return;
	svs.threaded = true;
	svs.threadstop = false;
	svs.threadmutex = Thread_CreateMutex();
	svs.thread = Thread_CreateThread(SV_ThreadFunc, NULL);
}

void SV_StopThread(void)
{
	if (!svs.threaded)
		return;
	svs.threadstop = true;
	Thread_WaitThread(svs.thread, 0);
	Thread_DestroyMutex(svs.threadmutex);
	svs.threaded = false;
}
