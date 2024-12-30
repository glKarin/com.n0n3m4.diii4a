
//DarkPlaces supported extension list, draft version 1.04

//definitions that id Software left out:
//these are passed as the 'nomonsters' parameter to traceline/tracebox (yes really this was supported in all quake engines, nomonsters is misnamed)
float MOVE_NORMAL = 0; // same as FALSE
float MOVE_NOMONSTERS = 1; // same as TRUE
float MOVE_MISSILE = 2; // save as movement with .movetype == MOVETYPE_FLYMISSILE

//checkextension function
//idea: expected by almost everyone
//darkplaces implementation: LadyHavoc
float(string s) checkextension = #99;
//description:
//check if (cvar("pr_checkextension")) before calling this, this is the only
//guaranteed extension to be present in the extension system, it allows you
//to check if an extension is available, by name, to check for an extension
//use code like this:
//// (it is recommended this code be placed in worldspawn or a worldspawn called function somewhere)
//if (cvar("pr_checkextension"))
//if (checkextension("DP_SV_SETCOLOR"))
//	ext_setcolor = TRUE;
//from then on you can check ext_setcolor to know if that extension is available

//BX_WAL_SUPPORT
//idea: id Software
//darkplaces implementation: LadyHavoc
//description:
//indicates the engine supports .wal textures for filenames in the textures/ directory
//(note: DarkPlaces has supported this since 2001 or 2002, but did not advertise it as an extension, then I noticed Betwix was advertising it and added the extension accordingly)

//DP_BUTTONCHAT
//idea: Vermeulen
//darkplaces implementation: LadyHavoc
//field definitions:
.float buttonchat;
//description:
//true if the player is currently chatting (in messagemode, menus or console) or if their game window is not active.

//DP_BUTTONUSE
//idea: id Software
//darkplaces implementation: LadyHavoc
//field definitions:
.float buttonuse;
//client console commands:
//+use
//-use
//description:
//made +use and -use commands work, they now control the .buttonuse field (.button1 was used by many mods for other purposes).

//DP_CL_LOADSKY
//idea: Nehahra, LadyHavoc
//darkplaces implementation: LadyHavoc
//client console commands:
//"loadsky" (parameters: "basename", example: "mtnsun_" would load "mtnsun_up.tga" and "mtnsun_rt.tga" and similar names, use "" to revert to quake sky, note: this is the same as Quake2 skybox naming)
//description:
//sets global skybox for the map for this client (can be stuffed to a client by QC), does not hurt much to repeatedly execute this command, please don't use this in mods if it can be avoided (only if changing skybox is REALLY needed, otherwise please use DP_GFX_SKYBOX).

//DP_CON_SET
//idea: id Software
//darkplaces implementation: LadyHavoc
//description:
//indicates this engine supports the "set" console command which creates or sets a non-archived cvar (not saved to config.cfg on exit), it is recommended that set and seta commands be placed in default.cfg for mod-specific cvars.

//DP_CON_SETA
//idea: id Software
//darkplaces implementation: LadyHavoc
//description:
//indicates this engine supports the "seta" console command which creates or sets an archived cvar (saved to config.cfg on exit), it is recommended that set and seta commands be placed in default.cfg for mod-specific cvars.

//DP_CON_ALIASPARAMETERS
//idea: many
//darkplaces implementation: Black
//description:
//indicates this engine supports aliases containing $1 through $9 parameter macros (which when called will expand to the parameters passed to the alias, for example alias test "say $2 $1", then you can type test hi there and it will execute say there hi), as well as $0 (name of the alias) and $* (all parameters $1 onward).

//DP_CON_EXPANDCVAR
//idea: many, PHP
//darkplaces implementation: Black
//description:
//indicates this engine supports console commandlines containing $cvarname which will expand to the contents of that cvar as a parameter, for instance say my fov is $fov, will say "my fov is 90", or similar.

//DP_CON_STARTMAP
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//adds two engine-called aliases named startmap_sp and startmap_dm which are called when the engine tries to start a singleplayer game from the menu (startmap_sp) or the -listen or -dedicated options are used or the engine is a dedicated server (uses startmap_dm), these allow a mod or game to specify their own map instead of start, and also distinguish between singleplayer and -listen/-dedicated, also these need not be a simple "map start" command, they can do other things if desired, startmap_sp and startmap_dm both default to "map start".

//DP_EF_ADDITIVE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_ADDITIVE     = 32;
//description:
//additive blending when this object is rendered

//DP_EF_BLUE
//idea: id Software
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_BLUE         = 64;
//description:
//entity emits blue light (used for quad)

//DP_EF_DOUBLESIDED
//idea: LadyHavoc
//darkplaces implementation: [515] and LadyHavoc
//effects bit:
float EF_DOUBLESIDED = 32768;
//description:
//render entity as double sided (backfaces are visible, I.E. you see the 'interior' of the model, rather than just the front), can be occasionally useful on transparent stuff.

//DP_EF_DYNAMICMODELLIGHT
//idea: C.Brutail, divVerent, maikmerten
//darkplaces implementation: divVerent
//effects bit:
float   EF_DYNAMICMODELLIGHT     = 131072;
//description:
//force dynamic model light on the entity, even if it's a BSP model (or anything else with lightmaps or light colors)

//DP_EF_FLAME
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_FLAME        = 1024;
//description:
//entity is on fire

//DP_EF_FULLBRIGHT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_FULLBRIGHT   = 512;
//description:
//entity is always brightly lit

//DP_EF_NODEPTHTEST
//idea: Supa
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_NODEPTHTEST       = 8192;
//description:
//makes entity show up to client even through walls, useful with EF_ADDITIVE for special indicators like where team bases are in a map, so that people don't get lost

//DP_EF_NODRAW
//idea: id Software
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_NODRAW       = 16;
//description:
//prevents server from sending entity to client (forced invisible, even if it would have been a light source or other such things)

//DP_EF_NOGUNBOB
//idea: Chris Page, Dresk
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_NOGUNBOB     = 256;
//description:
//this has different meanings depending on the entity it is used on:
//player entity - prevents gun bobbing on player.viewmodel
//viewmodelforclient entity - prevents gun bobbing on an entity attached to the player's view
//other entities - no effect
//uses:
//disabling gun bobbing on a diving mask or other model used as a .viewmodel.
//disabling gun bobbing on view-relative models meant to be part of the heads up display.  (note: if fov is changed these entities may be off-screen, or too near the center of the screen, so use fov 90 in this case)

//DP_EF_NOSHADOW
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_NOSHADOW     = 4096;
//description:
//realtime lights will not cast shadows from this entity (but can still illuminate it)

//DP_EF_RED
//idea: id Software
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_RED          = 128;
//description:
//entity emits red light (used for invulnerability)

//DP_EF_RESTARTANIM_BIT
//idea: id software
//darkplaces implementation: divVerent
//effects bit:
float   EF_RESTARTANIM_BIT = 1048576;
//description:
//when toggled, the current animation is restarted. Useful for weapon animation.
//to toggle this bit in QC, you can do:
//  self.effects += (EF_RESTARTANIM_BIT - 2 * (self.effects & EF_RESTARTANIM_BIT));

//DP_EF_STARDUST
//idea: MythWorks Inc
//darkplaces implementation: LadyHavoc
//effects bit:
float   EF_STARDUST     = 2048;
//description:
//entity emits bouncing sparkles in every direction

//DP_EF_TELEPORT_BIT
//idea: id software
//darkplaces implementation: divVerent
//effects bit:
float   EF_TELEPORT_BIT = 2097152;
//description:
//when toggled, interpolation of the entity is skipped for one frame. Useful for teleporting.
//to toggle this bit in QC, you can do:
//  self.effects += (EF_TELEPORT_BIT - 2 * (self.effects & EF_TELEPORT_BIT));

//DP_ENT_ALPHA
//idea: Nehahra
//darkplaces implementation: LadyHavoc
//fields:
.float alpha;
//description:
//controls opacity of the entity, 0.0 is forced to be 1.0 (otherwise everything would be invisible), use -1 if you want to make something invisible, 1.0 is solid (like normal).

//DP_ENT_COLORMOD
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definition:
.vector colormod;
//description:
//controls color of the entity, '0 0 0', is forced to be '1 1 1' (otherwise everything would be black), used for tinting objects, for instance using '1 0.6 0.4' on an ogre would give you an orange ogre (order is red green blue), note the colors can go up to '8 8 8' (8x as bright as normal).

//DP_ENT_CUSTOMCOLORMAP
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//if .colormap is set to 1024 + pants + shirt * 16, those colors will be used for colormapping the entity, rather than looking up a colormap by player number.

/*
//NOTE: no longer supported by darkplaces because all entities are delta compressed now
//DP_ENT_DELTACOMPRESS // no longer supported
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//effects bit:
float EF_DELTA = 8388608;
//description:
//(obsolete) applies delta compression to the network updates of the entity, making updates smaller, this might cause some unreliable behavior in packet loss situations, so it should only be used on numerous (nails/plasma shots/etc) or unimportant objects (gibs/shell casings/bullet holes/etc).
*/

//DP_ENT_EXTERIORMODELTOCLIENT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//fields:
.entity exteriormodeltoclient;
//description:
//the entity is visible to all clients with one exception: if the specified client is using first person view (not using chase_active) the entity will not be shown.  Also if tag attachments are supported any entities attached to the player entity will not be drawn in first person.

//DP_ENT_GLOW
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float glow_color;
.float glow_size;
.float glow_trail;
//description:
//customizable glowing light effect on the entity, glow_color is a paletted (8bit) color in the range 0-255 (note: 0 and 254 are white), glow_size is 0 or higher (up to the engine what limit to cap it to, darkplaces imposes a 1020 limit), if glow_trail is true it will leave a trail of particles of the same color as the light.

//DP_ENT_GLOWMOD
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definition:
.vector glowmod;
//description:
//controls color of the entity's glow texture (fullbrights), '0 0 0', is forced to be '1 1 1' (otherwise everything would be black), used for tinting objects, see colormod (same color restrictions apply).

//DP_ENT_LOWPRECISION
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//effects bit:
float EF_LOWPRECISION = 4194304;
//description:
//uses low quality origin coordinates, reducing network traffic compared to the default high precision, intended for numerous objects (projectiles/gibs/bullet holes/etc).

//DP_ENT_SCALE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float scale;
//description:
//controls rendering scale of the object, 0 is forced to be 1, darkplaces uses 1/16th accuracy and a limit of 15.9375, can be used to make an object larger or smaller.

//DP_ENT_TRAILEFFECTNUM
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float traileffectnum;
//description:
//use a custom effectinfo.txt effect on this entity, assign it like this:
//self.traileffectnum = particleeffectnum("mycustomeffect");
//this will do both the dlight and particle trail as described in the effect, basically equivalent to trailparticles() in CSQC but performed on a server entity.

//DP_ENT_VIEWMODEL
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.entity viewmodelforclient;
//description:
//this is a very special capability, attachs the entity to the view of the client specified, origin and angles become relative to the view of that client, all effects can be used (multiple skins on a weapon model etc)...  the entity is not visible to any other client.

//DP_GFX_EXTERNALTEXTURES
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//loads external textures found in various directories (tenebrae compatible)...
/*
in all examples .tga is merely the base texture, it can be any of these:
.tga (base texture)
_glow.tga (fullbrights or other glowing overlay stuff, NOTE: this is done using additive blend, not alpha)
_pants.tga (pants overlay for colormapping on models, this should be shades of grey (it is tinted by pants color) and black wherever the base texture is not black, as this is an additive blend)
_shirt.tga (same idea as pants, but for shirt color)
_diffuse.tga (this may be used instead of base texture for per pixel lighting)
_gloss.tga (specular texture for per pixel lighting, note this can be in color (tenebrae only supports greyscale))
_norm.tga (normalmap texture for per pixel lighting)
_bump.tga (bumpmap, converted to normalmap at load time, supported only for reasons of tenebrae compatibility)
_luma.tga (same as _glow but supported only for reasons of tenebrae compatibility)

due to glquake's incomplete Targa(r) loader, this section describes
required Targa(r) features support:
types:
type 1 (uncompressed 8bit paletted with 24bit/32bit palette)
type 2 (uncompressed 24bit/32bit true color, glquake supported this)
type 3 (uncompressed 8bit greyscale)
type 9 (RLE compressed 8bit paletted with 24bit/32bit palette)
type 10 (RLE compressed 24bit/32bit true color, glquake supported this)
type 11 (RLE compressed 8bit greyscale)
attribute bit 0x20 (Origin At Top Left, top to bottom, left to right)

image formats guaranteed to be supported: tga, pcx, lmp
image formats that are optional: png, jpg

mdl/spr/spr32 examples:
skins are named _A (A being a number) and skingroups are named like _A_B
these act as suffixes on the model name...
example names for skin _2_1 of model "progs/armor.mdl":
game/override/progs/armor.mdl_2_1.tga
game/textures/progs/armor.mdl_2_1.tga
game/progs/armor.mdl_2_1.tga
example names for skin _0 of the model "progs/armor.mdl":
game/override/progs/armor.mdl_0.tga
game/textures/progs/armor.mdl_0.tga
game/progs/armor.mdl_0.tga
note that there can be more skins files (of the _0 naming) than the mdl
contains, this is only useful to save space in the .mdl file if classic quake
compatibility is not a concern.

bsp/md2/md3 examples:
example names for the texture "quake" of model "maps/start.bsp":
game/override/quake.tga
game/textures/quake.tga
game/quake.tga

sbar/menu/console textures: for example the texture "conchars" (console font) in gfx.wad
game/override/gfx/conchars.tga
game/textures/gfx/conchars.tga
game/gfx/conchars.tga
*/

//DP_GFX_EXTERNALTEXTURES_PERMAPTEXTURES
//idea: Fuh?
//darkplaces implementation: LadyHavoc
//description:
//Q1BSP and HLBSP map loading loads external textures found in textures/<mapname>/ as well as textures/.
//Where mapname is the bsp filename minus the extension (typically .bsp) and minus maps/ if it is in maps/ (any other path is not removed)
//example:
//maps/e1m1.bsp uses textures in the directory textures/e1m1/ and falls back to textures/
//maps/b_batt0.bsp uses textures in the directory textures/b_batt0.bsp and falls back to textures/
//as a more extreme example:
//progs/something/blah.bsp uses textures in the directory textures/progs/something/blah/ and falls back to textures/

//DP_GFX_FOG
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//worldspawn fields:
//"fog" (parameters: "density red green blue", example: "0.1 0.3 0.3 0.3")
//description:
//global fog for the map, can not be changed by QC

//DP_GFX_QUAKE3MODELTAGS
//idea: id Software
//darkplaces implementation: LadyHavoc
//field definitions:
.entity tag_entity; // entity this is attached to (call setattachment to set this)
.float tag_index; // which tag on that entity (0 is relative to the entity, > 0 is an index into the tags on the model if it has any) (call setattachment to set this)
//builtin definitions:
void(entity e, entity tagentity, string tagname) setattachment = #443; // attachs e to a tag on tagentity (note: use "" to attach to entity origin/angles instead of a tag)
//description:
//allows entities to be visually attached to model tags (which follow animations perfectly) on other entities, for example attaching a weapon to a player's hand, or upper body attached to lower body, allowing it to change angles and frame separately (note: origin and angles are relative to the tag, use '0 0 0' for both if you want it to follow exactly, this is similar to viewmodelforclient's behavior).
//note 2: if the tag is not found, it defaults to "" (attach to origin/angles of entity)
//note 3: attaching to world turns off attachment
//note 4: the entity that this is attached to must be visible for this to work
//note 5: if an entity is attached to the player entity it will not be drawn in first person.

//DP_GFX_SKINFILES
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//alias models (mdl, md2, md3) can have .skin files to replace conventional texture naming, these have a naming format such as:
//progs/test.md3_0.skin
//progs/test.md3_1.skin
//...
//
//these files contain replace commands (replace meshname shadername), example:
//replace "helmet" "progs/test/helmet1.tga" // this is a mesh shader replacement
//replace "teamstripes" "progs/test/redstripes.tga"
//replace "visor" "common/nodraw" // this makes the visor mesh invisible
////it is not possible to rename tags using this format
//
//Or the Quake3 syntax (100% compatible with Quake3's .skin files):
//helmet,progs/test/helmet1.tga // this is a mesh shader replacement
//teamstripes,progs/test/redstripes.tga
//visor,common/nodraw // this makes the visor mesh invisible
//tag_camera, // this defines that the first tag in the model is called tag_camera
//tag_test, // this defines that the second tag in the model is called tag_test
//
//any names that are not replaced are automatically show up as a grey checkerboard to indicate the error status, and "common/nodraw" is a special case that is invisible.
//this feature is intended to allow multiple skin sets on md3 models (which otherwise only have one skin set).
//other commands might be added someday but it is not expected.

//DP_GFX_SKYBOX
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//worldspawn fields:
//"sky" (parameters: "basename", example: "mtnsun_" would load "mtnsun_up.tga" and "mtnsun_rt.tga" and similar names, note: "sky" is also used the same way by Quake2)
//description:
//global skybox for the map, can not be changed by QC

//DP_UTF8
//idea: Blub\0, divVerent
//darkplaces implementation: Blub\0
//cvar definitions:
//   utf8_enable: enable utf8 encoding
//description: utf8 characters are allowed inside cvars, protocol strings, files, progs strings, etc., 
//and count as 1 char for string functions like strlen, substring, etc.
// note: utf8_enable is run-time cvar, could be changed during execution
// note: beware that str2chr() could return value bigger than 255 once utf8 is enabled

//DP_HALFLIFE_MAP
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//simply indicates that the engine supports HalfLife maps (BSP version 30, NOT the QER RGBA ones which are also version 30).

//DP_HALFLIFE_MAP_CVAR
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//cvars:
//halflifebsp 0/1
//description:
//engine sets this cvar when loading a map to indicate if it is halflife format or not.

//DP_HALFLIFE_SPRITE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//simply indicates that the engine supports HalfLife sprites.

//DP_INPUTBUTTONS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float button3;
.float button4;
.float button5;
.float button6;
.float button7;
.float button8;
.float button9;
.float button10;
.float button11;
.float button12;
.float button13;
.float button14;
.float button15;
.float button16;
//description:
//set to the state of the +button3, +button4, +button5, +button6, +button7, and +button8 buttons from the client, this does not involve protocol changes (the extra 6 button bits were simply not used).
//the exact mapping of protocol button bits on the server is:
//self.button0 = (bits & 1) != 0;
///* button1 is skipped because mods abuse it as a variable, and accordingly it has no bit */
//self.button2 = (bits & 2) != 0;
//self.button3 = (bits & 4) != 0;
//self.button4 = (bits & 8) != 0;
//self.button5 = (bits & 16) != 0;
//self.button6 = (bits & 32) != 0;
//self.button7 = (bits & 64) != 0;
//self.button8 = (bits & 128) != 0;

// DP_LIGHTSTYLE_STATICVALUE
// idea: VorteX
// darkplaces implementation: VorteX
// description: allows alternative 'static' lightstyle syntax : "=value"
// examples: "=0.5", "=2.0", "=2.75"
// could be used to control switchable lights or making styled lights with brightness > 2
// Warning: this extension is experimental. It safely works in CSQC, but SVQC use is limited by the fact 
// that other engines (which do not support this extension) could connect to a game and misunderstand this kind of lightstyle syntax

//DP_LITSPRITES
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//indicates this engine supports lighting on sprites, any sprite with ! in its filename (both on disk and in the qc) will be lit rather than having forced EF_FULLBRIGHT (EF_FULLBRIGHT on the entity can still force these sprites to not be lit).

//DP_LITSUPPORT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//indicates this engine loads .lit files for any quake1 format .bsp files it loads to enhance maps with colored lighting.
//implementation description: these files begin with the header QLIT followed by version number 1 (as little endian 32bit), the rest of the file is a replacement lightmaps lump, except being 3x as large as the lightmaps lump of the map it matches up with (and yes the between-lightmap padding is expanded 3x to keep this consistent), so the lightmap offset in each surface is simply multiplied by 3 during loading to properly index the lit data, and the lit file is loaded instead of the lightmap lump, other renderer changes are needed to display these of course...  see the litsupport.zip sample code (almost a tutorial) at http://icculus.org/twilight/darkplaces for more information.

//DP_MONSTERWALK
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//MOVETYPE_WALK is permitted on non-clients, so bots can move smoothly, run off ledges, etc, just like a real player.

//DP_MOVETYPEBOUNCEMISSILE
//idea: id Software
//darkplaces implementation: id Software
//movetype definitions:
//float MOVETYPE_BOUNCEMISSILE = 11; // already in defs.qc
//description:
//MOVETYPE_BOUNCE but without gravity, and with full reflection (no speed loss like grenades have), in other words - bouncing laser bolts.

//DP_MOVETYPEFLYWORLDONLY
//idea: Samual
//darkplaces implementation: Samual
//movetype definitions:
float MOVETYPE_FLY_WORLDONLY = 33;
//description:
//like MOVETYPE_FLY, but does all traces with MOVE_WORLDONLY, and is ignored by MOVETYPE_PUSH. Should only be combined with SOLID_NOT and SOLID_TRIGGER.

//DP_NULL_MODEL
//idea: Chris
//darkplaces implementation: divVerent
//definitions:
//string dp_null_model = "null";
//description:
//setmodel(e, "null"); makes an entity invisible, have a zero bbox, but
//networked. useful for shared CSQC entities.

//DP_MOVETYPEFOLLOW
//idea: id Software, LadyHavoc (redesigned)
//darkplaces implementation: LadyHavoc
//movetype definitions:
float MOVETYPE_FOLLOW = 12;
//description:
//MOVETYPE_FOLLOW implemented, this uses existing entity fields in unusual ways:
//aiment - the entity this is attached to.
//punchangle - the original angles when the follow began.
//view_ofs - the relative origin (note that this is un-rotated by punchangle, and that is actually the only purpose of punchangle).
//v_angle - the relative angles.
//here's an example of how you would set a bullet hole sprite to follow a bmodel it was created on, even if the bmodel rotates:
//hole.movetype = MOVETYPE_FOLLOW; // make the hole follow
//hole.solid = SOLID_NOT; // MOVETYPE_FOLLOW is always non-solid
//hole.aiment = bmodel; // make the hole follow bmodel
//hole.punchangle = bmodel.angles; // the original angles of bmodel
//hole.view_ofs = hole.origin - bmodel.origin; // relative origin
//hole.v_angle = hole.angles - bmodel.angles; // relative angles

//DP_QC_ASINACOSATANATAN2TAN
//idea: Urre
//darkplaces implementation: LadyHavoc
//constant definitions:
float DEG2RAD = 0.0174532925199432957692369076848861271344287188854172545609719144;
float RAD2DEG = 57.2957795130823208767981548141051703324054724665643215491602438612;
float PI      = 3.1415926535897932384626433832795028841971693993751058209749445923;
//builtin definitions:
float(float s) asin = #471; // returns angle in radians for a given sin() value, the result is in the range -PI*0.5 to PI*0.5
float(float c) acos = #472; // returns angle in radians for a given cos() value, the result is in the range 0 to PI
float(float t) atan = #473; // returns angle in radians for a given tan() value, the result is in the range -PI*0.5 to PI*0.5
float(float c, float s) atan2 = #474; // returns angle in radians for a given cos() and sin() value pair, the result is in the range -PI to PI (this is identical to vectoyaw except it returns radians rather than degrees)
float(float a) tan = #475; // returns tangent value (which is simply sin(a)/cos(a)) for the given angle in radians, the result is in the range -infinity to +infinity
//description:
//useful math functions for analyzing vectors, note that these all use angles in radians (just like the cos/sin functions) not degrees unlike makevectors/vectoyaw/vectoangles, so be sure to do the appropriate conversions (multiply by DEG2RAD or RAD2DEG as needed).
//note: atan2 can take unnormalized vectors (just like vectoyaw), and the function was included only for completeness (more often you want vectoyaw or vectoangles), atan2(v_x,v_y) * RAD2DEG gives the same result as vectoyaw(v)

//DP_QC_AUTOCVARS
//idea: divVerent
//darkplaces implementation: divVerent
//description:
//allows QC variables to be bound to cvars
//(works for float, string, vector types)
//example:
// float autocvar_developer;
// float autocvar_registered;
// string autocvar__cl_name;
//NOTE: copying a string-typed autocvar to another variable/field, and then
//changing the cvar or returning from progs is UNDEFINED. Writing to autocvar
//globals is UNDEFINED. Accessing autocvar globals after changing that cvar in
//the same frame by any means other than cvar_set() from the same QC VM is
//IMPLEMENTATION DEFINED (an implementation may either yield the previous, or
//the current, value). Changing them via cvar_set() in the same QC VM
//immediately must reflect on the autocvar globals. Whether autocvar globals,
//after restoring a savegame, have the cvar's current value, or the original
//value at time of saving, is UNDEFINED. Restoring a savegame however must not
//restore the cvar values themselves.
//In case the cvar does NOT exist, then it is automatically created with the
//value of the autocvar initializer, if given. This is possible with e.g.
//frikqcc and fteqcc the following way:
// var float autocvar_whatever = 42;
//If no initializer is given, the cvar will be initialized to a string
//equivalent to the NULL value of the given data type, that is, the empty
//string, 0, or '0 0 0'. However, when automatic cvar creation took place, a
//warning is printed to the game console.
//NOTE: to prevent an ambiguity with float names for vector types, autocvar
//names MUST NOT end with _x, _y or _z!

//DP_QC_CHANGEPITCH
//idea: id Software
//darkplaces implementation: id Software
//field definitions:
.float idealpitch;
.float pitch_speed;
//builtin definitions:
void(entity ent) changepitch = #63;
//description:
//equivalent to changeyaw, ent is normally self. (this was a Q2 builtin)

//DP_QC_COPYENTITY
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(entity from, entity to) copyentity = #400;
//description:
//copies all data in the entity to another entity.

//DP_QC_CRC16
//idea: divVerent
//darkplaces implementation: divVerent
//Some hash function to build hash tables with. This has to be be the CRC-16-CCITT that is also required for the QuakeWorld download protocol.
//When caseinsensitive is set, the CRC is calculated of the lower cased string.
float(float caseinsensitive, string s, ...) crc16 = #494;

//DP_QC_CVAR_DEFSTRING
//idea: id Software (Doom3), LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
string(string s) cvar_defstring = #482;
//description:
//returns the default value of a cvar, as a tempstring.

//DP_QC_CVAR_DESCRIPTION
//idea: divVerent
//DarkPlaces implementation: divVerent
//builtin definitions:
string(string name) cvar_description = #518;
//description:
//returns the description of a cvar

//DP_QC_CVAR_STRING
//idea: VorteX
//DarkPlaces implementation: VorteX, LadyHavoc
//builtin definitions:
string(string s) cvar_string = #448;
//description:
//returns the value of a cvar, as a tempstring.

//DP_QC_CVAR_TYPE
//idea: divVerent
//DarkPlaces implementation: divVerent
//builtin definitions:
float(string name) cvar_type = #495;
float CVAR_TYPEFLAG_EXISTS = 1;
float CVAR_TYPEFLAG_SAVED = 2;
float CVAR_TYPEFLAG_PRIVATE = 4;
float CVAR_TYPEFLAG_ENGINE = 8;
float CVAR_TYPEFLAG_HASDESCRIPTION = 16;
float CVAR_TYPEFLAG_READONLY = 32;

//DP_QC_DIGEST
//idea: motorsep, Spike
//DarkPlaces implementation: divVerent
//builtin definitions:
string(string digest, string data, ...) digest_hex = #639;
//description:
//returns a given hex digest of given data
//the returned digest is always encoded in hexadecimal
//only the "MD4" digest is always supported!
//if the given digest is not supported, string_null is returned
//the digest string is matched case sensitively, use "MD4", not "md4"!

//DP_QC_DIGEST_SHA256
//idea: motorsep, Spike
//DarkPlaces implementation: divVerent
//description:
//"SHA256" is also an allowed digest type

//DP_QC_EDICT_NUM
//idea: 515
//DarkPlaces implementation: LadyHavoc
//builtin definitions:
entity(float entnum) edict_num = #459;
float(entity ent) wasfreed = #353; // same as in EXT_CSQC extension
//description:
//edict_num returns the entity corresponding to a given number, this works even for freed entities, but you should call wasfreed(ent) to see if is currently active.
//wasfreed returns whether an entity slot is currently free (entities that have never spawned are free, entities that have had remove called on them are also free).

//DP_QC_ENTITYDATA
//idea: KrimZon
//darkplaces implementation: KrimZon
//builtin definitions:
float() numentityfields = #496;
string(float fieldnum) entityfieldname = #497;
float(float fieldnum) entityfieldtype = #498;
string(float fieldnum, entity ent) getentityfieldstring = #499;
float(float fieldnum, entity ent, string s) putentityfieldstring = #500;
//constants:
//Returned by entityfieldtype
float FIELD_STRING   = 1;
float FIELD_FLOAT    = 2;
float FIELD_VECTOR   = 3;
float FIELD_ENTITY   = 4;
float FIELD_FUNCTION = 6;
//description:
//Versatile functions intended for storing data from specific entities between level changes, but can be customized for some kind of partial savegame.
//WARNING: .entity fields cannot be saved and restored between map loads as they will leave dangling pointers.
//numentityfields returns the number of entity fields. NOT offsets. Vectors comprise 4 fields: v, v_x, v_y and v_z.
//entityfieldname returns the name as a string, eg. "origin" or "classname" or whatever.
//entityfieldtype returns a value that the constants represent, but the field may be of another type in more exotic progs.dat formats or compilers.
//getentityfieldstring returns data as would be written to a savegame, eg... "0.05" (float), "0 0 1" (vector), or "Hello World!" (string). Function names can also be returned.
//putentityfieldstring puts the data returned by getentityfieldstring back into the entity.

//DP_QC_ENTITYSTRING
void(string s) loadfromdata = #529;
void(string s) loadfromfile = #530;
void(string s) callfunction = #605;
void(float fh, entity e) writetofile = #606;
float(string s) isfunction = #607;
void(entity e, string s) parseentitydata = #608;

//DP_QC_ETOS
//idea: id Software
//darkplaces implementation: id Software
//builtin definitions:
string(entity ent) etos = #65;
//description:
//prints "entity 1" or similar into a string. (this was a Q2 builtin)

//DP_QC_EXTRESPONSEPACKET
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
string(void) getextresponse = #624;
//description:
//returns a string of the form "\"ipaddress:port\" data...", or the NULL string
//if no packet was found. Packets can be queued into the client/server by
//sending a packet starting with "\xFF\xFF\xFF\xFFextResponse " to the
//listening port.

//DP_QC_FINDCHAIN
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
entity(.string fld, string match) findchain = #402;
//description:
//similar to find() but returns a chain of entities like findradius.

//DP_QC_FINDCHAIN_TOFIELD
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
entity(vector org, float rad, .entity tofield) findradius_tofield = #22;
entity(.string fld, string match, .entity tofield) findchain_tofield = #402;
entity(.float fld, float match, .entity tofield) findchainflags_tofield = #450;
entity(.float fld, float match, .entity tofield) findchainfloat_tofield = #403;
//description:
//similar to findchain() etc, but stores the chain into .tofield instead of .chain
//actually, the .entity tofield is an optional field of the the existing findchain* functions

//DP_QC_FINDCHAINFLAGS
//idea: Sajt
//darkplaces implementation: LadyHavoc
//builtin definitions:
entity(.float fld, float match) findchainflags = #450;
//description:
//similar to findflags() but returns a chain of entities like findradius.

//DP_QC_FINDCHAINFLOAT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
entity(.entity fld, entity match) findchainentity = #403;
entity(.float fld, float match) findchainfloat = #403;
//description:
//similar to findentity()/findfloat() but returns a chain of entities like findradius.

//DP_QC_FINDFLAGS
//idea: Sajt
//darkplaces implementation: LadyHavoc
//builtin definitions:
entity(entity start, .float fld, float match) findflags = #449;
//description:
//finds an entity with the specified flag set in the field, similar to find()

//DP_QC_FINDFLOAT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
entity(entity start, .entity fld, entity match) findentity = #98;
entity(entity start, .float fld, float match) findfloat = #98;
//description:
//finds an entity or float field value, similar to find(), but for entity and float fields.

//DP_QC_FS_SEARCH
//idea: Black
//darkplaces implementation: Black
//builtin definitions:
float(string pattern, float caseinsensitive, float quiet) search_begin = #444;
void(float handle) search_end = #445;
float(float handle) search_getsize = #446;
string(float handle, float num) search_getfilename = #447;
//description:
//search_begin performs a filename search with the specified pattern (for example "maps/*.bsp") and stores the results in a search slot (minimum of 128 supported by any engine with this extension), the other functions take this returned search slot number, be sure to search_free when done (they are also freed on progs reload).
//search_end frees a search slot (also done at progs reload).
//search_getsize returns how many filenames were found.
//search_getfilename returns a filename from the search.

//DP_QC_GETLIGHT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
vector(vector org) getlight = #92;
//description:
//returns the lighting at the requested location (in color), 0-255 range (can exceed 255).

//DP_QC_GETSURFACE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
float(entity e, float s) getsurfacenumpoints = #434;
vector(entity e, float s, float n) getsurfacepoint = #435;
vector(entity e, float s) getsurfacenormal = #436;
string(entity e, float s) getsurfacetexture = #437;
float(entity e, vector p) getsurfacenearpoint = #438;
vector(entity e, float s, vector p) getsurfaceclippedpoint = #439;
//description:
//functions to query surface information.

//DP_QC_GETSURFACEPOINTATTRIBUTE
//idea: BlackHC
//darkplaces implementation: BlackHC
// constants
float SPA_POSITION = 0;
float SPA_S_AXIS = 1;
float SPA_T_AXIS = 2;
float SPA_R_AXIS = 3; // same as SPA_NORMAL
float SPA_TEXCOORDS0 = 4;
float SPA_LIGHTMAP0_TEXCOORDS = 5;
float SPA_LIGHTMAP0_COLOR = 6;
//builtin definitions:
vector(entity e, float s, float n, float a) getsurfacepointattribute = #486;

//description:
//function to query extended information about a point on a certain surface

//DP_QC_GETSURFACETRIANGLE
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
float(entity e, float s) getsurfacenumtriangles = #628;
vector(entity e, float s, float n) getsurfacetriangle = #629;
//description:
//function to query triangles of a surface

//DP_QC_GETTAGINFO
//idea: VorteX, LadyHavoc
//DarkPlaces implementation: VorteX
//builtin definitions:
float(entity ent, string tagname) gettagindex = #451;
vector(entity ent, float tagindex) gettaginfo = #452;
//description:
//gettagindex returns the number of a tag on an entity, this number is the same as set by setattachment (in the .tag_index field), allowing the qc to save a little cpu time by keeping the number around if it wishes (this could already be done by calling setattachment and saving off the tag_index).
//gettaginfo returns the origin of the tag in worldspace and sets v_forward, v_right, and v_up to the current orientation of the tag in worldspace, this automatically resolves all dependencies (attachments, including viewmodelforclient), this means you could fire a shot from a tag on a gun entity attached to the view for example.

//DP_QC_GETTAGINFO_BONEPROPERTIES
//idea: daemon
//DarkPlaces implementation: divVerent
//global definitions:
float gettaginfo_parent;
string gettaginfo_name;
vector gettaginfo_offset;
vector gettaginfo_forward;
vector gettaginfo_right;
vector gettaginfo_up;
//description:
//when this extension is present, gettaginfo fills in some globals with info about the bone that had been queried
//gettaginfo_parent is set to the number of the parent bone, or 0 if it is a root bone
//gettaginfo_name is set to the name of the bone whose index had been specified in gettaginfo
//gettaginfo_offset, gettaginfo_forward, gettaginfo_right, gettaginfo_up contain the transformation matrix of the bone relative to its parent. Note that the matrix may contain a scaling component.

//DP_QC_GETTIME
//idea: tZork
//darkplaces implementation: tZork, divVerent
//constant definitions:
float GETTIME_FRAMESTART = 0; // time of start of frame relative to an arbitrary point in time (time of worldspawn)
float GETTIME_REALTIME = 1; // current absolute time (OS specific)
float GETTIME_HIRES = 2; // like REALTIME, but relative to time at start of frame (thus can be higher precision)
float GETTIME_UPTIME = 3; // time of start of frame relative to start of the engine
//builtin definitions:
float(float tmr) gettime = #519;
//description:
//some timers to query...

//DP_QC_GETTIME_CDTRACK
//idea: divVerent
//darkplaces implementation: divVerent
//constant definitions:
float GETTIME_CDTRACK = 4;
//description:
//returns the playing time of the current cdtrack when passed to gettime()
//see DP_END_GETSOUNDTIME for similar functionality but for entity sound channels

//DP_QC_I18N
//idea: divVerent
//darkplaces implementation: divVerent
//description:
//
//The engine supports translating by gettext compatible .po files.
//progs.dat uses progs.dat.<LANGUAGE>.po
//menu.dat uses menu.dat.<LANGUAGE>.po
//csprogs.dat uses csprogs.dat.<LANGUAGE>.po
//
//To create a string that can be translated, define it as
//  string dotranslate_FILENOTFOUND = "File not found";
//Note: if the compiler does constant folding, this will only work if there is
//no other "File not found" string in the progs!
//
//Alternatively, if using the Xonotic patched fteqcc compiler, you can simplify
//this by using _("File not found") directly in the source code.
//
//The language is set by the "prvm_language" cvar: if prvm_language is set to
//"de", it will read progs.dat.de.po for translating strings in progs.dat.
//
//If prvm_language is set to the special name "dump", progs.dat.pot will be
//written, which is a translation template to be edited by filling out the
//msgstr entries.

//DP_QC_LOG
//darkplaces implementation: divVerent
//builtin definitions:
float log(float f) = #532;
//description:
//logarithm

//DP_QC_MINMAXBOUND
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
float(float a, float b, ...) min = #94;
float(float a, float b, float c) min3 = #94;
float(float a, float b, float c, float d) min4 = #94;
float(float a, float b, float c, float d, float e) min5 = #94;
float(float a, float b, float c, float d, float e, float f) min6 = #94;
float(float a, float b, float c, float d, float e, float f, float g) min7 = #94;
float(float a, float b, float c, float d, float e, float f, float g, float h) min8 = #94;
float(float a, float b, ...) max = #95;
float(float a, float b, float c) max3 = #95;
float(float a, float b, float c, float d) max4 = #95;
float(float a, float b, float c, float d, float e) max5 = #95;
float(float a, float b, float c, float d, float e, float f) max6 = #95;
float(float a, float b, float c, float d, float e, float f, float g) max7 = #95;
float(float a, float b, float c, float d, float e, float f, float g, float h) max8 = #95;
float(float minimum, float val, float maximum) bound = #96;
//description:
//min returns the lowest of all the supplied numbers.
//max returns the highest of all the supplied numbers.
//bound clamps the value to the range and returns it.

//DP_QC_MULTIPLETEMPSTRINGS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//this extension makes all builtins returning tempstrings (ftos for example)
//cycle through a pool of multiple tempstrings (at least 16), allowing
//multiple ftos results to be gathered before putting them to use, normal
//quake only had 1 tempstring, causing many headaches.
//
//Note that for longer term storage (or compatibility on engines having
//FRIK_FILE but not this extension) the FRIK_FILE extension's
//strzone/strunzone builtins provide similar functionality (slower though).
//
//NOTE: this extension is superseded by DP_QC_UNLIMITEDTEMPSTRINGS

//DP_QC_NUM_FOR_EDICT
//idea: Blub\0
//darkplaces implementation: Blub\0
//Function to get the number of an entity - a clean way.
float(entity num) num_for_edict = #512;

//DP_QC_RANDOMVEC
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
vector() randomvec = #91;
//description:
//returns a vector of length < 1, much quicker version of this QC: do {v_x = random()*2-1;v_y = random()*2-1;v_z = random()*2-1;} while(vlen(v) > 1)

//DP_QC_SINCOSSQRTPOW
//idea: id Software, LadyHavoc
//darkplaces implementation: id Software, LadyHavoc
//builtin definitions:
float(float val) sin = #60;
float(float val) cos = #61;
float(float val) sqrt = #62;
float(float a, float b) pow = #97;
//description:
//useful math functions, sine of val, cosine of val, square root of val, and raise a to power b, respectively.

//DP_QC_SPRINTF
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
string(string format, ...) sprintf = #627;
//description:
//you know sprintf :P
//supported stuff:
//  %
//  optional: <argpos>$ for the argument to format (the arg counter then is not increased)
//  flags: #0- +
//  optional: <width>, *, or *<argpos>$ for the field width (width is read before value and precision)
//  optional: .<precision>, .*, or .*<argpos>$ for the precision (precision is read before value)
//  length modifiers: h for forcing a float, l for forcing an entity (by default, %d etc. cast a float to int), ll for forcing an int
//  conversions:
//    d takes a float if no length is specified or h is, and an entity if l is specified as length, and an int if ll is specified as length, and cast it to an int
//    i takes an entity if no length is specified or l is, and a float if h is specified as length, and an int if ll is specified as length, and cast it to an int
//    ouxXc take a float if no length is specified or h is, and an entity if l is specified as length, and an int if ll is specified as length, and cast it to an unsigned int
//    eEfFgG take a float if no length is specified or h is, and an entity if l is specified as length, and an int if ll is specified as length, and cast it to a double
//    s takes a string
//    vV takes a vector, and processes the three components as if it were a gG for all three components, separated by space
//    For conversions s and c, the flag # makes precision and width interpreted
//      as byte count, by default it is interpreted as character count in UTF-8
//      enabled engines. No other conversions can create wide characters, and #
//      has another meaning in these. When in character count mode, color codes
//      are ignored. To get UTF-8 semantics WITHOUT color code parsing, use
//      the + flag.

//DP_QC_STRFTIME
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
string(float uselocaltime, string format, ...) strftime = #478;
//description:
//provides the ability to get the local (in your timezone) or world (Universal Coordinated Time) time as a string using the formatting of your choice:
//example: "%Y-%m-%d %H:%M:%S"   (result looks like: 2007-02-08 01:03:15)
//note: "%F %T" gives the same result as "%Y-%m-%d %H:%M:%S" (ISO 8601 date format and 24-hour time)
//for more format codes please do a web search for strftime 3 and you should find the man(3) pages for this standard C function.
//
//practical uses:
//changing day/night cycle (shops closing, monsters going on the prowl) in an RPG, for this you probably want to use s = strftime(TRUE, "%H");hour = stof(s);
//printing current date/time for competitive multiplayer games, such as the beginning/end of each round in real world time.
//activating eastereggs in singleplayer games on certain dates.
//
//note: some codes such as %x and %X use your locale settings and thus may not make sense to international users, it is not advisable to use these as the server and clients may be in different countries.
//note: if you display local time to a player, it would be a good idea to note whether it is local time using a string like "%F %T (local)", and otherwise use "%F %T (UTC)".
//note: be aware that if the game is saved and reloaded a week later the date and time will be different, so if activating eastereggs in a singleplayer game or something you may want to only check when a level is loaded and then keep the same easteregg state throughout the level so that the easteregg does not deactivate when reloading from a savegame (also be aware that precaches should not depend on such date/time code because reloading a savegame often scrambles the precaches if so!).
//note: this function can return a NULL string (you can check for it with if (!s)) if the localtime/gmtime functions returned NULL in the engine code, such as if those functions don't work on this platform (consoles perhaps?), so be aware that this may return nothing.

//DP_QC_STRINGCOLORFUNCTIONS
//idea: Dresk
//darkplaces implementation: Dresk
//builtin definitions:
float(string s) strlennocol = #476; // returns how many characters are in a string, minus color codes
string(string s) strdecolorize = #477; // returns a string minus the color codes of the string provided
//description:
//provides additional functionality to strings by supporting functions that isolate and identify strings with color codes

//DP_QC_STRING_CASE_FUNCTIONS
//idea: Dresk
//darkplaces implementation: LadyHavoc / Dresk
//builtin definitions:
string(string s) strtolower = #480; // returns the passed in string in pure lowercase form
string(string s) strtoupper = #481; // returns the passed in string in pure uppercase form
//description:
//provides simple string uppercase and lowercase functions

//DP_QC_TOKENIZEBYSEPARATOR
//idea: Electro, SavageX, LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
float(string s, string separator1, ...) tokenizebyseparator = #479;
//description:
//this function returns tokens separated by any of the supplied separator strings, example:
//numnumbers = tokenizebyseparator("10.2.3.4", ".");
//returns 4 and the tokens are "10" "2" "3" "4"
//possibly useful for parsing IPv4 addresses (such as "1.2.3.4") and IPv6 addresses (such as "[1234:5678:9abc:def0:1234:5678:9abc:def0]:26000")

//DP_QC_TOKENIZE_CONSOLE
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
float(string s) tokenize_console = #514;
float(float i) argv_start_index = #515;
float(float i) argv_end_index = #516;
//description:
//this function returns tokens separated just like the console does
//also, functions are provided to get the index of the first and last character of each token in the original string
//Passing negative values to them, or to argv, will be treated as indexes from the LAST token (like lists work in Perl). So argv(-1) will return the LAST token.

//DP_QC_TRACEBOX
//idea: id Software
//darkplaces implementation: id Software
//builtin definitions:
void(vector v1, vector min, vector max, vector v2, float nomonsters, entity forent) tracebox = #90;
//description:
//similar to traceline but much more useful, traces a box of the size specified (technical note: in quake1 and halflife bsp maps the mins and maxs will be rounded up to one of the hull sizes, quake3 bsp does not have this problem, this is the case with normal moving entities as well).

//DP_QC_TRACETOSS
//idea: id Software
//darkplaces implementation: id Software
//builtin definitions:
void(entity ent, entity ignore) tracetoss = #64;
//description:
//simulates movement of the entity as if it is MOVETYPE_TOSS and starting with it's current state (location, velocity, etc), returns relevant trace_ variables (trace_fraction is always 0, all other values are supported - trace_ent, trace_endpos, trace_plane_normal), does not actually alter the entity.

//DP_QC_TRACE_MOVETYPE_HITMODEL
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//constant definitions:
float MOVE_HITMODEL = 4;
//description:
//allows traces to hit alias models (not sprites!) instead of entity boxes, use as the nomonsters parameter to trace functions, note that you can hit invisible model entities (alpha < 0 or EF_NODRAW or model "", it only checks modelindex)

//DP_QC_TRACE_MOVETYPE_WORLDONLY
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//constant definitions:
float MOVE_WORLDONLY = 3;
//description:
//allows traces to hit only world (ignoring all entities, unlike MOVE_NOMONSTERS which hits all bmodels), use as the nomonsters parameter to trace functions

//DP_QC_UNLIMITEDTEMPSTRINGS
//idea: divVerent
//darkplaces implementation: LadyHavoc
//description:
//this extension alters Quake behavior such that instead of reusing a single
//tempstring (or multiple) there are an unlimited number of tempstrings, which
//are removed only when a QC function invoked by the engine returns,
//eliminating almost all imaginable concerns with string handling in QuakeC.
//
//in short:
//you can now use and abuse tempstrings as much as you like, you still have to
//use strzone (FRIK_FILE) for permanent storage however.
//
//
//
//implementation notes for other engine coders:
//these tempstrings are expected to be stored in a contiguous buffer in memory
//which may be fixed size or controlled by a cvar, or automatically grown on
//demand (in the case of DarkPlaces).
//
//this concept is similar to quake's Zone system, however these are all freed
//when the QC interpreter returns.
//
//this is basically a poor man's garbage collection system for strings.

//DP_QC_VECTOANGLES_WITH_ROLL
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
vector(vector forward, vector up) vectoangles2 = #51; // same number as vectoangles
//description:
//variant of vectoangles that takes an up vector to calculate roll angle (also uses this to calculate yaw correctly if the forward is straight up or straight down)
//note: just like normal vectoangles you need to negate the pitch of the returned angles if you want to feed them to makevectors or assign to self.v_angle

//DP_QC_VECTORVECTORS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector dir) vectorvectors = #432;
//description:
//creates v_forward, v_right, and v_up vectors given a forward vector, similar to makevectors except it takes a forward direction vector instead of angles.

//DP_QC_WHICHPACK
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
string(string filename) whichpack = #503;
//description:
//for files in a pak/pk3/whatever, returns the pack's file name in FRIK_FILE name space.
//for physical files, returns "".
//in case of error, returns string_null.

//DP_QC_URI_ESCAPE
//idea: divVerent
//darkplaces implementation: divVerent
//URI::Escape's functionality
string(string in) uri_escape = #510;
string(string in) uri_unescape = #511;

//DP_QC_URI_GET
//idea: divVerent
//darkplaces implementation: divVerent
//loads text from an URL into a string
//returns 1 on success of initiation, 0 if there are too many concurrent
//connections already or if the URL is invalid
//the following callback will receive the data and MUST exist!
//  void(float id, float status, string data) URI_Get_Callback;
//status is either
//  negative for an internal error,
//  0 for success, or
//  the HTTP response code on server error (e.g. 404)
//if 1 is returned by uri_get, the callback will be called in the future
float(string url, float id) uri_get = #513;

//DP_QC_URI_POST
//idea: divVerent
//darkplaces implementation: divVerent
//loads text from an URL into a string after POSTing via HTTP
//works like uri_get, but uri_post sends data with Content-Type: content_type to the server
//and uri_post sends the string buffer buf, joined using the delimiter delim
float(string url, float id, string content_type, string data) uri_post = #513;
float(string url, float id, string content_type, string delim, float buf) uri_postbuf = #513;

//DP_SKELETONOBJECTS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//this extension indicates that FTE_CSQC_SKELETONOBJECTS functionality is available in server QC (as well as CSQC).

//DP_SV_SPAWNFUNC_PREFIX
//idea: divVerent
//darkplaces implementation: divVerent
//Make functions whose name start with spawnfunc_ take precedence as spawn function for loading entities.
//Useful if you have a field ammo_shells (required in any Quake mod) but want to support spawn functions called ammo_shells (like in Q3A).
//Optionally, you can declare a global "float require_spawnfunc_prefix;" which will require ANY spawn function to start with that prefix.


//DP_QUAKE2_MODEL
//idea: quake community
//darkplaces implementation: LadyHavoc
//description:
//shows that the engine supports Quake2 .md2 files.

//DP_QUAKE2_SPRITE
//idea: LadyHavoc
//darkplaces implementation: Elric
//description:
//shows that the engine supports Quake2 .sp2 files.

//DP_QUAKE3_MAP
//idea: quake community
//darkplaces implementation: LadyHavoc
//description:
//shows that the engine supports Quake3 .bsp files.

//DP_QUAKE3_MODEL
//idea: quake community
//darkplaces implementation: LadyHavoc
//description:
//shows that the engine supports Quake3 .md3 files.

//DP_REGISTERCVAR
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
float(string name, string value) registercvar = #93;
//description:
//adds a new console cvar to the server console (in singleplayer this is the player's console), the cvar exists until the mod is unloaded or the game quits.
//NOTE: DP_CON_SET is much better.

//DP_SND_DIRECTIONLESSATTNNONE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//make sounds with ATTN_NONE have no spatialization (enabling easy use as music sources).

//DP_SND_FAKETRACKS
//idea: requested

//darkplaces implementation: Elric
//description:
//the engine plays sound/cdtracks/track001.wav instead of cd track 1 and so on if found, this allows games and mods to have music tracks without using ambientsound.
//Note: also plays .ogg with DP_SND_OGGVORBIS extension.

//DP_SND_SOUND7_WIP1
//DP_SND_SOUND7_WIP2
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
void(entity e, float chan, string samp, float vol, float atten, float speed, float flags) sound7 = #8;
float SOUNDFLAG_RELIABLE = 1;
//description:
//plays a sound, with some more flags
//extensions to sound():
//- channel may be in the range from -128 to 127; channels -128 to 0 are "auto",
//  i.e. support multiple sounds at once, but cannot be stopped/restarted
//- a value 0 in the speed parameter means no change; otherwise, it is a
//  percentage of playback speed ("pitch shifting"). 100 is normal pitch, 50 is
//  half speed, 200 is double speed, etc. (DP_SND_SOUND7_WIP2)
//- the flag SOUNDFLAG_RELIABLE can be specified, which makes the sound send
//  to MSG_ALL (reliable) instead of MSG_BROADCAST (unreliable, default);
//  similarily, SOUNDFLAG_RELIABLE_TO_ONE sends to MSG_ONE
//- channel 0 is controlled by snd_channel0volume; channel 1 and -1 by
//  snd_channel1volume, etc. (so, a channel shares the cvar with its respective
//  auto-channel); however, the mod MUST define snd_channel8volume and upwards
//  in default.cfg if they are to be used, as the engine does not create them
//  to not litter the cvar list
//- this extension applies to CSQC as well; CSQC_Event_Sound will get speed and
//  flags as extra 7th and 8th argument
//- WIP2 ideas: SOUNDFLAG_RELIABLE_TO_ONE, SOUNDFLAG_NOPHS, SOUNDFLAG_FORCELOOP
//- NOTE: to check for this, ALSO OR a check with DP_SND_SOUND7 to also support
//  the finished extension once done

//DP_SND_OGGVORBIS
//idea: Transfusion
//darkplaces implementation: Elric
//description:
//the engine supports loading Ogg Vorbis sound files.  Use either the .ogg filename directly, or a .wav of the same name (will try to load the .wav first and then .ogg).

//DP_SND_STEREOWAV
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//the engine supports stereo WAV files.  (useful with DP_SND_DIRECTIONLESSATTNNONE for music)

//DP_SND_GETSOUNDTIME
//idea: VorteX
//darkplaces implementation: VorteX
//constant definitions:
float(entity e, float channel) getsoundtime = #533; // get currently sound playing position on entity channel, -1 if not playing or error
float(string sample) soundlength = #534; // returns length of sound sample in seconds, -1 on error (sound not precached, sound system not initialized etc.)
//description: provides opportunity to query length of sound samples and realtime tracking of sound playing on entities (similar to DP_GETTIME_CDTRACK)
//note: beware dedicated server not running sound engine at all, so in dedicated mode this builtins will not work in server progs
//note also: menu progs not supporting getsoundtime() (will give a warning) since it has no sound playing on entities
//examples of use:
//  - QC-driven looped sounds
//  - QC events when sound playing is finished
//  - toggleable ambientsounds
//  - subtitles

//DP_VIDEO_DPV
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//console commands:
//  playvideo <videoname> - start playing video
//  stopvideo - stops current video
//description: indicated that engine support playing videos in DPV format

//DP_VIDEO_SUBTITLES
//idea: VorteX
//darkplaces implementation: VorteX
//cvars:
//  cl_video_subtitles - toggles subtitles showing
//  cl_video_subtitles_lines - how many lines to reserve for subtitles
//  cl_video_subtitles_textsize - font size
//console commands:
//  playvideo <videoname> <custom_subtitles_file> - start playing video
//  stopvideo - stops current video
//description: indicates that engine support subtitles on videos
//subtitles stored in external text files, each video file has it's default subtitles file ( <videoname>.dpsubs )
//example: for video/act1.dpv default subtitles file will be video/act1.dpsubs
//also video could be played with custom subtitles file by utilizing second parm of playvideo command
//syntax of .dpsubs files: each line in .dpsubs file defines 1 subtitle, there are three tokens:
//   <start> <end> "string"
//   start: subtitle start time in seconds
//     end: subtitle time-to-show in seconds, if 0 - subtitle will be showed until next subtitle is started, 
//          if below 0 - show until next subtitles minus this number of seconds
//    text: subtitle text, color codes (Q3-style and ^xRGB) are allowed
//example of subtitle file:
//   3	0	"Vengeance! Vengeance for my eternity of suffering!"
//   9	0	"Whelp! As if you knew what eternity was!"
//   13	0	"Grovel before your true master."
//   17	0	"Never!" 
//   18	7	"I'll hack you from crotch to gizzard and feed what's left of you to your brides..."

//DP_SOLIDCORPSE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//solid definitions:
float SOLID_CORPSE = 5;
//description:
//the entity will not collide with SOLID_CORPSE and SOLID_SLIDEBOX entities (and likewise they will not collide with it), this is useful if you want dead bodies that are shootable but do not obstruct movement by players and monsters, note that if you traceline with a SOLID_SLIDEBOX entity as the ignoreent, it will ignore SOLID_CORPSE entities, this is desirable for visibility and movement traces, but not for bullets, for the traceline to hit SOLID_CORPSE you must temporarily force the player (or whatever) to SOLID_BBOX and then restore to SOLID_SLIDEBOX after the traceline.

//DP_SPRITE32
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//the engine supports .spr32 sprites.

//DP_SV_BOTCLIENT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//constants:
float CLIENTTYPE_DISCONNECTED = 0;
float CLIENTTYPE_REAL = 1;
float CLIENTTYPE_BOT = 2;
float CLIENTTYPE_NOTACLIENT = 3;
//builtin definitions:
entity() spawnclient = #454; // like spawn but for client slots (also calls relevant connect/spawn functions), returns world if no clients available
float(entity clent) clienttype = #455; // returns one of the CLIENTTYPE_* constants
//description:
//spawns a client with no network connection, to allow bots to use client slots with no hacks.
//How to use:
/*
	// to spawn a bot
	local entity oldself;
	oldself = self;
	self = spawnclient();
	if (!self)
	{
		bprint("Can not add bot, server full.\n");
		self = oldself;
		return;
	}
	self.netname = "Yoyobot";
	self.clientcolors = 12 * 16 + 4; // yellow (12) shirt and red (4) pants
	ClientConnect();
	PutClientInServer();
	self = oldself;

	// to remove all bots
	local entity head;
	head = find(world, classname, "player");
	while (head)
	{
		if (clienttype(head) == CLIENTTYPE_BOT)
			dropclient(head);
		head = find(head, classname, "player");
	}

	// to identify if a client is a bot (for example in PlayerPreThink)
	if (clienttype(self) == CLIENTTYPE_BOT)
		botthink();
*/
//see also DP_SV_CLIENTCOLORS DP_SV_CLIENTNAME DP_SV_DROPCLIENT
//How it works:
//creates a new client, calls SetNewParms and stores the parms, and returns the client.
//this intentionally does not call SV_SendServerinfo to allow the QuakeC a chance to set the netname and clientcolors fields before actually spawning the bot by calling ClientConnect and PutClientInServer manually
//on level change ClientConnect and PutClientInServer are called by the engine to spawn in the bot (this is why clienttype() exists to tell you on the next level what type of client this is).
//parms work the same on bot clients as they do on real clients, and do carry from level to level

//DP_SV_BOUNCEFACTOR
//idea: divVerent
//darkplaces implementation: divVerent
//field definitions:
.float bouncefactor; // velocity multiplier after a bounce
.float bouncestop; // bounce stops if velocity to bounce plane is < bouncestop * gravity AFTER the bounce
//description:
//allows qc to customize MOVETYPE_BOUNCE a bit

//DP_SV_CLIENTCAMERA
//idea: LadyHavoc, others
//darkplaces implementation: Black
//field definitions:
.entity clientcamera; // override camera entity
//description:
//allows another entity to be the camera for a client, for example a remote camera, this is similar to sending svc_setview manually except that it also changes the network culling appropriately.

//DP_SV_CLIENTCOLORS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float clientcolors; // colors of the client (format: pants + shirt * 16)
//description:
//allows qc to read and modify the client colors associated with a client entity (not particularly useful on other entities), and automatically sends out any appropriate network updates if changed

//DP_SV_CLIENTNAME
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//description:
//allows qc to modify the client's .netname, and automatically sends out any appropriate network updates if changed

//DP_SV_CUSTOMIZEENTITYFORCLIENT
//idea: LadyHavoc
//darkplaces implementation: [515] and LadyHavoc
//field definitions:
.float() customizeentityforclient; // self = this entity, other = client entity
//description:
//allows qc to modify an entity before it is sent to each client, the function returns TRUE if it should send, FALSE if it should not, and is fully capable of editing the entity's fields, this allows cloaked players to appear less transparent to their teammates, navigation markers to only show to their team, etc
//tips on writing customize functions:
//it is a good idea to return FALSE early in the function if possible to reduce cpu usage, because this function may be called many thousands of times per frame if there are many customized entities on a 64+ player server.
//you are free to change anything in self, but please do not change any other entities (the results may be very inconsistent).
//example ideas for use of this extension:
//making icons over teammates' heads which are only visible to teammates.  for exasmple: float() playericon_customizeentityforclient = {return self.owner.team == other.team;};
//making cloaked players more visible to their teammates than their enemies.  for example: float() player_customizeentityforclient = {if (self.items & IT_CLOAKING) {if (self.team == other.team) self.alpha = 0.6;else self.alpha = 0.1;} return TRUE;};
//making explosion models that face the viewer (does not work well with chase_active).  for example: float() explosion_customizeentityforclient = {self.angles = vectoangles(other.origin + other.view_ofs - self.origin);self.angles_x = 0 - self.angles_x;};
//implementation notes:
//entity customization is done before per-client culling (visibility for instance) because the entity may be doing setorigin to display itself in different locations on different clients, may be altering its .modelindex, .effects and other fields important to culling, so customized entities increase cpu usage (non-customized entities can use all the early culling they want however, as they are not changing on a per client basis).

//DP_SV_DISABLECLIENTPREDICTION
//idea: LadyHavoc, Mario
//darkplaces implementation: LadyHavoc, Mario
//field definitions:
.float disableclientprediction;
//description:
//By default, player entities are enabled for prediction by the engine if the
//engine assumes the client can sensibly predict them. As the NQ and DarkPlaces
//protocol does not network movetype, this in particular allows for client
//prediction only if movetype == MOVETYPE_WALK.
//Setting this field to 1 disables prediction in any case - this is useful when
//the client cannot sensibly predict the server's idea of how the player moves
//(common in case of pure serverside grappling hook or jetpack
//implementations).
//Setting this field to -1 forces prediction even if the server assumes the
//client cannot predict the current movetype of a player entity (obviously,
//this then requires matching client-side prediction code in CSQC, as the
//engine's own client prediction will sure not handle these cases right due to
//not knowing the serverside value of movetype). This is allowed in combination
//with the following movetypes:
// MOVETYPE_NONE (useful to have full QC control over movement)
// MOVETYPE_WALK (redundant but harmless)
// MOVETYPE_STEP
// MOVETYPE_FLY (useful for spectators)
// MOVETYPE_TOSS
// MOVETYPE_NOCLIP (useful for spectators)
// MOVETYPE_FLYMISSILE
// MOVETYPE_BOUNCE
// MOVETYPE_BOUNCEMISSILE
// MOVETYPE_FLY_WORLDONLY (useful for spectators)

//DP_SV_DISCARDABLEDEMO
//idea: parasti
//darkplaces implementation: parasti
//field definitions:
.float discardabledemo;
//description:
//when this field is set to a non-zero value on a player entity, a possibly recorded server-side demo for the player is discarded
//Note that this extension only works if:
//  auto demos are enabled (the cvar sv_autodemo_perclient is set)
//  discarding demos is enabled (the cvar sv_autodemo_perclient_discardable is set)

//DP_SV_DRAWONLYTOCLIENT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.entity drawonlytoclient;
//description:
//the entity is only visible to the specified client.

//DP_SV_DROPCLIENT
//idea: FrikaC
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(entity clent) dropclient = #453;
//description:
//causes the server to immediately drop the client, more reliable than stuffcmd(clent, "disconnect\n"); which could be intentionally ignored by the client engine

//DP_SV_EFFECT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org, string modelname, float startframe, float endframe, float framerate) effect = #404;
//SVC definitions:
//float svc_effect = #52; // [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
//float svc_effect2 = #53; // [vector] org [short] modelindex [byte] startframe [byte] framecount [byte] framerate
//description:
//clientside playback of simple custom sprite effects (explosion sprites, etc).

//DP_SV_ENTITYCONTENTSTRANSITION
//idea: Dresk
//darkplaces implementation: Dresk
//field definitions:
.void(float nOriginalContents, float nNewContents) contentstransition;
//description:
//This field function, when provided, is triggered on an entity when the contents (ie. liquid / water / etc) is changed.
//The first parameter provides the entities original contents, prior to the transition.  The second parameter provides the new contents.
//NOTE: If this field function is provided on an entity, the standard watersplash sound IS SUPPRESSED to allow for authors to create their own transition sounds.

//DP_SV_MOVETYPESTEP_LANDEVENT
//idea: Dresk
//darkplaces implementation: Dresk
//field definitions:
.void(vector vImpactVelocity) movetypesteplandevent;
//description:
//This field function, when provided, is triggered on a MOVETYPE_STEP entity when it experiences  "land event".
// The standard engine behavior for this event is to play the sv_sound_land CVar sound.
//The parameter provides the velocity of the entity at the time of the impact.  The z value may therefore be used to calculate how "hard" the entity struck the surface.
//NOTE: If this field function is provided on a MOVETYPE_STEP entity, the standard sv_sound_land sound IS SUPPRESSED to allow for authors to create their own feedback.

//DP_SV_POINTSOUND
//idea: Dresk
//darkplaces implementation: Dresk
//builtin definitions:
void(vector origin, string sample, float volume, float attenuation) pointsound = #483;
//description:
//Similar to the standard QC sound function, this function takes an origin instead of an entity and omits the channel parameter.
// This allows sounds to be played at arbitrary origins without spawning entities.

//DP_SV_ONENTITYNOSPAWNFUNCTION
//idea: Dresk
//darkplaces implementation: Dresk
//engine-called QC prototypes:
//void() SV_OnEntityNoSpawnFunction;
//description:
// This function is called whenever an entity on the server has no spawn function, and therefore has no defined QC behavior.
// You may as such dictate the behavior as to what happens to the entity.
// To mimic the engine's default behavior, simply call remove(self).

//DP_SV_ONENTITYPREPOSTSPAWNFUNCTION
//idea: divVerent
//darkplaces implementation: divVerent
//engine-called QC prototypes:
//void() SV_OnEntityPreSpawnFunction;
//void() SV_OnEntityPostSpawnFunction;
//description:
// These functions are called BEFORE or AFTER an entity is spawned the regular way.
// You may as such dictate the behavior as to what happens to the entity.
// SV_OnEntityPreSpawnFunction is called before even looking for the spawn function, so you can even change its classname in there. If it remove()s the entity, the spawn function will not be looked for.
// SV_OnEntityPostSpawnFunction is called ONLY after its spawn function or SV_OnEntityNoSpawnFunction was called, and skipped if the entity got removed by either.

//DP_SV_MODELFLAGS_AS_EFFECTS
//idea: LadyHavoc, Dresk
//darkplaces implementation: LadyHavoc
//field definitions:
.float modelflags;
//constant definitions:
float EF_NOMODELFLAGS = 8388608; // ignore any effects in a model file and substitute your own
float MF_ROCKET  =   1; // leave a trail
float MF_GRENADE =   2; // leave a trail
float MF_GIB     =   4; // leave a trail
float MF_ROTATE  =   8; // rotate (bonus items)
float MF_TRACER  =  16; // green split trail
float MF_ZOMGIB  =  32; // small blood trail
float MF_TRACER2 =  64; // orange split trail
float MF_TRACER3 = 128; // purple trail
//description:
//this extension allows model flags to be specified on entities so you can add a rocket trail and glow to any entity, etc.
//setting any of these will override the flags the model already has, to disable the model's flags without supplying any of your own you must use EF_NOMODELFLAGS.
//
//silly example modification #1 to W_FireRocket in weapons.qc:
//missile.effects = EF_NOMODELFLAGS; // rocket without a glow/fire trail
//silly example modification #2 to W_FireRocket in weapons.qc:
//missile.modelflags = MF_GIB; // leave a blood trail instead of glow/fire trail
//
//note: you can not combine multiple kinds of trail, only one of them will be active, you can combine MF_ROTATE and the other MF_ flags however, and using EF_NOMODELFLAGS along with these does no harm.
//
//note to engine coders: they are internally encoded in the protocol as extra EF_ flags (shift the values left 24 bits and send them in the protocol that way), so no protocol change was required (however 32bit effects is a protocol extension itself), within the engine they are referred to as EF_ for the 24bit shifted values.

//DP_SV_NETADDRESS
//idea: Dresk
//darkplaces implementation: Dresk
//field definitions:
.string netaddress;
//description:
// provides the netaddress of the associated entity (ie. 127.0.0.1) and "null/botclient" if the netconnection of the entity is invalid

//DP_SV_NODRAWTOCLIENT
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.entity nodrawtoclient;
//description:
//the entity is not visible to the specified client.

//DP_SV_PING
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float ping;
//description:
//continuously updated field indicating client's ping (based on average of last 16 packet time differences).

//DP_SV_PING_PACKETLOSS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float ping_packetloss;
.float ping_movementloss;
//description:
//continuously updated field indicating client's packet loss, and movement loss (i.e. packet loss affecting player movement).

//DP_SV_POINTPARTICLES
//idea: Spike
//darkplaces implementation: LadyHavoc
//function definitions:
float(string effectname) particleeffectnum = #335; // same as in CSQC
void(entity ent, float effectnum, vector start, vector end) trailparticles = #336; // same as in CSQC
void(float effectnum, vector org, vector vel, float howmany) pointparticles = #337; // same as in CSQC
//SVC definitions:
//float svc_trailparticles = 60; // [short] entnum [short] effectnum [vector] start [vector] end
//float svc_pointparticles = 61; // [short] effectnum [vector] start [vector] velocity [short] count
//float svc_pointparticles1 = 62; // [short] effectnum [vector] start, same as svc_pointparticles except velocity is zero and count is 1
//description:
//provides the ability to spawn non-standard particle effects, typically these are defined in a particle effect information file such as effectinfo.txt in darkplaces.
//this is a port of particle effect features from clientside QC (EXT_CSQC) to server QC, as these effects are potentially useful to all games even if they do not make use of EXT_CSQC.
//warning: server must have same order of effects in effectinfo.txt as client does or the numbers would not match up, except for standard quake effects which are always the same numbers.

//DP_SV_PUNCHVECTOR
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.vector punchvector;
//description:
//offsets client view in worldspace, similar to view_ofs but all 3 components are used and are sent with at least 8 bits of fraction, this allows the view to be kicked around by damage or hard landings or whatever else, note that unlike punchangle this is not faded over time, it is up to the mod to fade it (see also DP_SV_PLAYERPHYSICS).

//DP_SV_PLAYERPHYSICS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.vector movement;
//cvar definitions:
//"sv_playerphysicsqc" (0/1, default 1, allows user to disable qc player physics)
//engine-called QC prototypes:
//void() SV_PlayerPhysics;
//description:
//.movement vector contains the movement input from the player, allowing QC to do as it wishs with the input, and SV_PlayerPhysics will completely replace the player physics if present (works for all MOVETYPE's), see darkplaces mod source for example of this function (in playermovement.qc, adds HalfLife ladders support, as well as acceleration/deceleration while airborn (rather than the quake sudden-stop while airborn), and simplifies the physics a bit)

//DP_PHYSICS_ODE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//globals:
//new movetypes:
const float MOVETYPE_PHYSICS = 32; // need to be set before any physics_* builtins applied
//new solid types (deprecated):
const float SOLID_PHYSICS_BOX = 32;
const float SOLID_PHYSICS_SPHERE = 33;
const float SOLID_PHYSICS_CAPSULE = 34;
const float SOLID_PHYSICS_TRIMESH = 35;
const float SOLID_PHYSICS_CYLINDER = 36;
//geometry types:
const float GEOMTYPE_NONE = -1;       // entity will be entirely skipped by ODE
const float GEOMTYPE_SOLID = 0;       // geometry type will be set based on .solid field
const float GEOMTYPE_BOX = 1;         // entity bound box
const float GEOMTYPE_SPHERE = 2;      // sphere with radius picked from x axis of entity bound box
const float GEOMTYPE_CAPSULE = 3;     // with leading axis automatically determined from longest one, radius is picked as minimal of the rest 2 axes
const float GEOMTYPE_TRIMESH = 4;     // triangle mesh
const float GEOMTYPE_CYLINDER = 5;    // like capsule but not capped
                                      // note that ODE's builtin cylinder support is experimental, somewhat bugged and unfinished (no cylinder-cylinder collision)
									  // to use properly working cylinder should build ODE with LIBCCD extension
const float GEOMTYPE_CAPSULE_X = 6;   // capsule with fixed leading axis
const float GEOMTYPE_CAPSULE_Y = 7;
const float GEOMTYPE_CAPSULE_Z = 8;
const float GEOMTYPE_CYLINDER_X	= 9;  // cylinder with fixed leading axis
const float GEOMTYPE_CYLINDER_Y	= 10;
const float GEOMTYPE_CYLINDER_Z	= 11;
//joint types:
const float JOINTTYPE_NONE = 0;
const float JOINTTYPE_POINT = 1;
const float JOINTTYPE_HINGE = 2;
const float JOINTTYPE_SLIDER = 3;
const float JOINTTYPE_UNIVERSAL = 4;
const float JOINTTYPE_HINGE2 = 5;
const float JOINTTYPE_FIXED = -1;
//force types:
const float FORCETYPE_NONE = 0;
const float FORCETYPE_FORCE = 1; // applied at center of mass
const float FORCETYPE_FORCEATPOS = 2;
const float FORCETYPE_TORQUE = 3;
// common joint properties:
// .entity aiment; // connected objects
// .entity enemy; // connected objects, forces
// .vector movedir;
//   for a spring:
//     movedir_x = spring constant (force multiplier, must be > 0)
//     movedir_y = spring dampening constant to prevent oscillation (must be > 0)
//     movedir_z = spring stop position (+/-)
//   for a motor:
//     movedir_x = desired motor velocity
//     movedir_y = -1 * max motor force to use
//     movedir_z = stop position (+/-), set to 0 for no stop
//   note that ODE does not support both in one anyway
//   for a force:
//     force vector to apply
//field definitions:
.float  geomtype;     // see GEOMTYPE_*, a more correct way to set collision shape, allows to set SOLID_CORPSE and trimesh collisions
.float  maxcontacts;  // maximum number of contacts to make for this object, lesser = faster (but setting it too low will could make object pass though walls), default is 16, maximum is 32
.float  mass;         // ODE mass, standart value is 1
.vector massofs;      // offsets a mass center out of object center, if not set a center of model bounds is used
.float  friction;     // a friction of object, get multiplied by second objects's friction on contact
.float  bouncefactor;
.float  bouncestop; 
.float  jointtype;    // type of joint
.float  forcetype;    // type of force
.float  erp;          // error restitution parameter, makes ODE solver attempt to fix errors in contacts, 
                      // bringing together 2 joints or fixing object being stuch in other object, 
			          // a value of 0.1 will fix slightly, a value of 1.0 attempts to fix whole error in one frame
			          // use with care as high values makes system unstable and likely to explode
//builtin definitions:
void(entity e, float physics_enabled) physics_enable = #540; // enable or disable physics on object
void(entity e, vector force, vector force_pos) physics_addforce = #541; // deprecated, apply a force from certain origin, length of force vector is power of force
void(entity e, vector torque) physics_addtorque = #542; // deprecated, add relative torque
//description: provides Open Dynamics Engine support, requires extenal dll to be present or engine compiled with statical link option
//be sure to checkextension for it to know if library is loaded and ready, also to enable physics set "physics_ode" cvar to 1
//note: this extension is highly experimental and may be unstable

//DP_SV_PRINT
//idea: id Software (QuakeWorld Server)
//darkplaces implementation: Black, LadyHavoc
void(string s, ...) print = #339; // same number as in EXT_CSQC
//description:
//this is identical to dprint except that it always prints regardless of the developer cvar.

//DP_SV_PRECACHEANYTIME
//idea: id Software (Quake2)
//darkplaces implementation: LadyHavoc
//description:
//this extension allows precache_model and precache_sound (and any variants) to be used during the game (with automatic messages to clients to precache the new model/sound indices), also setmodel/sound/ambientsound can be called without precaching first (they will cause an automatic precache).

//DP_SV_QCSTATUS
//idea: divVerent
//darkplaces implementation: divVerent
//1. A global variable
string worldstatus;
//Its content is set as "qcstatus" field in the rules.
//It may be at most 255 characters, and must not contain newlines or backslashes.
//2. A per-client field
.string clientstatus;
//It is sent instead of the "frags" in status responses.
//It should always be set in a way so that stof(player.clientstatus) is a meaningful score value. Other info may be appended. If used this way, the cvar sv_status_use_qcstatus may be set to 1, and then this value will replace the frags in "status".
//Currently, qstat does not support this and will not show player info if used and set to anything other than ftos(some integer).

//DP_SV_ROTATINGBMODEL
//idea: id Software
//darkplaces implementation: LadyHavoc
//description:
//this extension merely indicates that MOVETYPE_PUSH supports avelocity, allowing rotating brush models to be created, they rotate around their origin (needs rotation supporting qbsp/light utilities because id ones expected bmodel entity origins to be '0 0 0', recommend setting "origin" key in the entity fields in the map before compiling, there may be other methods depending on your qbsp, most are more complicated however).
//tip: level designers can create a func_wall with an origin, and avelocity (for example "avelocity" "0 90 0"), and "nextthink" "99999999" to make a rotating bmodel without any qc modifications, such entities will be solid in stock quake but will not rotate)

//DP_SV_SETCOLOR
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(entity ent, float colors) setcolor = #401;
//engine called QC functions (optional):
//void(float color) SV_ChangeTeam;
//description:
//setcolor sets the color on a client and updates internal color information accordingly (equivalent to stuffing a "color" command but immediate)
//SV_ChangeTeam is called by the engine whenever a "color" command is recieved, it may decide to do anything it pleases with the color passed by the client, including rejecting it (by doing nothing), or calling setcolor to apply it, preventing team changes is one use for this.
//the color format is pants + shirt * 16 (0-255 potentially)

//DP_SV_SLOWMO
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//cvars:
//"slowmo" (0+, default 1)
//description:
//sets the time scale of the server, mainly intended for use in singleplayer by the player, however potentially useful for mods, so here's an extension for it.
//range is 0 to infinite, recommended values to try are 0.1 (very slow, 10% speed), 1 (normal speed), 5 (500% speed).

//DP_SV_WRITEPICTURE
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
void(float to, string s, float sz) WritePicture = #501;
//description:
//writes a picture to the data stream so CSQC can read it using ReadPicture, which has the definition
//  string(void) ReadPicture = #501;
//The picture data is sent as at most sz bytes, by compressing to low quality
//JPEG. The data being sent will be equivalent to:
//  WriteString(to, s);
//  WriteShort(to, imagesize);
//  for(i = 0; i < imagesize; ++i)
//    WriteByte(to, [the i-th byte of the compressed JPEG image]);

//DP_SV_WRITEUNTERMINATEDSTRING
//idea: FrikaC
//darkplaces implementation: Sajt
//builtin definitions:
void(float to, string s) WriteUnterminatedString = #456;
//description:
//like WriteString, but does not write a terminating 0 after the string. This means you can include things like a player's netname in the middle of a string sent over the network. Just be sure to end it up with either a call to WriteString (which includes the trailing 0) or WriteByte(0) to terminate it yourself.
//A historical note: this extension was suggested by FrikaC years ago, more recently Shadowalker has been badmouthing LadyHavoc and Spike for stealing 'his' extension writestring2 which does exactly the same thing but uses a different builtin number and name and extension string, this argument hinges on the idea that it was his idea in the first place, which is incorrect as FrikaC first suggested it and used a rough equivalent of it in his FrikBot mod years ago involving WriteByte calls on each character.

//DP_TE_BLOOD
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org, vector velocity, float howmany) te_blood = #405;
//temp entity definitions:
float TE_BLOOD = 50;
//protocol:
//vector origin
//byte xvelocity (-128 to +127)
//byte yvelocity (-128 to +127)
//byte zvelocity (-128 to +127)
//byte count (0 to 255, how much blood)
//description:
//creates a blood effect.

//DP_TE_BLOODSHOWER
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower = #406;
//temp entity definitions:
//float TE_BLOODSHOWER = 52;
//protocol:
//vector mins (minimum corner of the cube)
//vector maxs (maximum corner of the cube)
//coord explosionspeed (velocity of blood particles flying out of the center)
//short count (number of blood particles)
//description:
//creates an exploding shower of blood, for making gibbings more convincing.

//DP_TE_CUSTOMFLASH
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org, float radius, float lifetime, vector color) te_customflash = #417;
//temp entity definitions:
//float TE_CUSTOMFLASH = 73;
//protocol:
//vector origin
//byte radius ((MSG_ReadByte() + 1) * 8, meaning 8-2048 unit radius)
//byte lifetime ((MSG_ReadByte() + 1) / 256.0, meaning approximately 0-1 second lifetime)
//byte red (0.0 to 1.0 converted to 0-255)
//byte green (0.0 to 1.0 converted to 0-255)
//byte blue (0.0 to 1.0 converted to 0-255)
//description:
//creates a customized light flash.

//DP_TE_EXPLOSIONRGB
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org, vector color) te_explosionrgb = #407;
//temp entity definitions:
//float TE_EXPLOSIONRGB = 53;
//protocol:
//vector origin
//byte red (0.0 to 1.0 converted to 0 to 255)
//byte green (0.0 to 1.0 converted to 0 to 255)
//byte blue (0.0 to 1.0 converted to 0 to 255)
//description:
//creates a colored explosion effect.

//DP_TE_FLAMEJET
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org, vector vel, float howmany) te_flamejet = #457;
//temp entity definitions:
//float TE_FLAMEJET = 74;
//protocol:
//vector origin
//vector velocity
//byte count (0 to 255, how many flame particles)
//description:
//creates a single puff of flame particles.  (not very useful really)

//DP_TE_PARTICLECUBE
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube = #408;
//temp entity definitions:
//float TE_PARTICLECUBE = 54;
//protocol:
//vector mins (minimum corner of the cube)
//vector maxs (maximum corner of the cube)
//vector velocity
//short count
//byte color (palette color)
//byte gravity (TRUE or FALSE, FIXME should this be a scaler instead?)
//coord randomvel (how much to jitter the velocity)
//description:
//creates a cloud of particles, useful for forcefields but quite customizable.

//DP_TE_PARTICLERAIN
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain = #409;
//temp entity definitions:
//float TE_PARTICLERAIN = 55;
//protocol:
//vector mins (minimum corner of the cube)
//vector maxs (maximum corner of the cube)
//vector velocity (velocity of particles)
//short count (number of particles)
//byte color (8bit palette color)
//description:
//creates a shower of rain, the rain will appear either at the top (if falling down) or bottom (if falling up) of the cube.

//DP_TE_PARTICLESNOW
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow = #410;
//temp entity definitions:
//float TE_PARTICLERAIN = 56;
//protocol:
//vector mins (minimum corner of the cube)
//vector maxs (maximum corner of the cube)
//vector velocity (velocity of particles)
//short count (number of particles)
//byte color (8bit palette color)
//description:
//creates a shower of snow, the snow will appear either at the top (if falling down) or bottom (if falling up) of the cube, low velocities are advisable for convincing snow.

//DP_TE_PLASMABURN
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org) te_plasmaburn = #433;
//temp entity definitions:
//float TE_PLASMABURN = 75;
//protocol:
//vector origin
//description:
//creates a small light flash (radius 200, time 0.2) and marks the walls.

//DP_TE_QUADEFFECTS1
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org) te_gunshotquad = #412;
void(vector org) te_spikequad = #413;
void(vector org) te_superspikequad = #414;
void(vector org) te_explosionquad = #415;
//temp entity definitions:
//float   TE_GUNSHOTQUAD  = 57; // [vector] origin
//float   TE_SPIKEQUAD    = 58; // [vector] origin
//float   TE_SUPERSPIKEQUAD = 59; // [vector] origin
//float   TE_EXPLOSIONQUAD = 70; // [vector] origin
//protocol:
//vector origin
//description:
//all of these just take a location, and are equivalent in function (but not appearance :) to the original TE_GUNSHOT, etc.

//DP_TE_SMALLFLASH
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org) te_smallflash = #416;
//temp entity definitions:
//float TE_SMALLFLASH = 72;
//protocol:
//vector origin
//description:
//creates a small light flash (radius 200, time 0.2).

//DP_TE_SPARK
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org, vector vel, float howmany) te_spark = #411;
//temp entity definitions:
//float TE_SPARK = 51;
//protocol:
//vector origin
//byte xvelocity (-128 to 127)
//byte yvelocity (-128 to 127)
//byte zvelocity (-128 to 127)
//byte count (number of sparks)
//description:
//creates a shower of sparks and a smoke puff.

//DP_TE_STANDARDEFFECTBUILTINS
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
void(vector org) te_gunshot = #418;
void(vector org) te_spike = #419;
void(vector org) te_superspike = #420;
void(vector org) te_explosion = #421;
void(vector org) te_tarexplosion = #422;
void(vector org) te_wizspike = #423;
void(vector org) te_knightspike = #424;
void(vector org) te_lavasplash = #425;
void(vector org) te_teleport = #426;
void(vector org, float color, float colorlength) te_explosion2 = #427;
void(entity own, vector start, vector end) te_lightning1 = #428;
void(entity own, vector start, vector end) te_lightning2 = #429;
void(entity own, vector start, vector end) te_lightning3 = #430;
void(entity own, vector start, vector end) te_beam = #431;
//description:
//to make life easier on mod coders.

//DP_TRACE_HITCONTENTSMASK_SURFACEINFO
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//globals:
.float dphitcontentsmask; // if non-zero on the entity passed to traceline/tracebox/tracetoss this will override the normal collidable contents rules and instead hit these contents values (for example AI can use tracelines that hit DONOTENTER if it wants to, by simply changing this field on the entity passed to traceline), this affects normal movement as well as trace calls
float trace_dpstartcontents; // DPCONTENTS_ value at start position of trace
float trace_dphitcontents; // DPCONTENTS_ value of impacted surface (not contents at impact point, just contents of the surface that was hit)
float trace_dphitq3surfaceflags; // Q3SURFACEFLAG_ value of impacted surface
string trace_dphittexturename; // texture name of impacted surface
//constants:
float DPCONTENTS_SOLID = 1; // hit a bmodel, not a bounding box
float DPCONTENTS_WATER = 2;
float DPCONTENTS_SLIME = 4;
float DPCONTENTS_LAVA = 8;
float DPCONTENTS_SKY = 16;
float DPCONTENTS_BODY = 32; // hit a bounding box, not a bmodel
float DPCONTENTS_CORPSE = 64; // hit a SOLID_CORPSE entity
float DPCONTENTS_NODROP = 128; // an area where backpacks should not spawn
float DPCONTENTS_PLAYERCLIP = 256; // blocks player movement
float DPCONTENTS_MONSTERCLIP = 512; // blocks monster movement
float DPCONTENTS_DONOTENTER = 1024; // AI hint brush
float DPCONTENTS_LIQUIDSMASK = 14; // WATER | SLIME | LAVA
float DPCONTENTS_BOTCLIP = 2048; // AI hint brush
float DPCONTENTS_OPAQUE = 4096; // only fully opaque brushes get this (may be useful for line of sight checks)
float Q3SURFACEFLAG_NODAMAGE = 1;
float Q3SURFACEFLAG_SLICK = 2; // low friction surface
float Q3SURFACEFLAG_SKY = 4; // sky surface (also has NOIMPACT and NOMARKS set)
float Q3SURFACEFLAG_LADDER = 8; // climbable surface
float Q3SURFACEFLAG_NOIMPACT = 16; // projectiles should remove themselves on impact (this is set on sky)
float Q3SURFACEFLAG_NOMARKS = 32; // projectiles should not leave marks, such as decals (this is set on sky)
float Q3SURFACEFLAG_FLESH = 64; // projectiles should do a fleshy effect (blood?) on impact
float Q3SURFACEFLAG_NODRAW = 128; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_HINT = 256; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_SKIP = 512; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_NOLIGHTMAP = 1024; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_POINTLIGHT = 2048; // compiler hint (not important to qc)
float Q3SURFACEFLAG_METALSTEPS = 4096; // walking on this surface should make metal step sounds
float Q3SURFACEFLAG_NOSTEPS = 8192; // walking on this surface should not make footstep sounds
float Q3SURFACEFLAG_NONSOLID = 16384; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_LIGHTFILTER = 32768; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_ALPHASHADOW = 65536; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_NODLIGHT = 131072; // compiler hint (not important to qc)
//float Q3SURFACEFLAG_DUST = 262144; // translucent 'light beam' effect (not important to qc)
//description:
//adds additional information after a traceline/tracebox/tracetoss call.
//also (very important) sets trace_* globals before calling .touch functions,
//this allows them to inspect the nature of the collision (for example
//determining if a projectile hit sky), clears trace_* variables for the other
//object in a touch event (that is to say, a projectile moving will see the
//trace results in its .touch function, but the player it hit will see very
//little information in the trace_ variables as it was not moving at the time)

//DP_VIEWZOOM
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//field definitions:
.float viewzoom;
//description:
//scales fov and sensitivity of player, valid range is 0 to 1 (intended for sniper rifle zooming, and such)

//EXT_BITSHIFT
//idea: Spike
//darkplaces implementation: [515]
//builtin definitions:
float(float number, float quantity) bitshift = #218;
//description:
//multiplies number by a power of 2 corresponding to quantity (0 = *1, 1 = *2, 2 = *4, 3 = *8, -1 = /2, -2 = /4x, etc), and rounds down (due to integer math) like other bit operations do (& and | and the like).
//note: it is faster to use multiply if you are shifting by a constant, avoiding the quakec function call cost, however that does not do a floor for you.

//FRIK_FILE
//idea: FrikaC
//darkplaces implementation: LadyHavoc
//builtin definitions:
float(string s) stof = #81; // get numerical value from a string
float(string filename, float mode) fopen = #110; // opens a file inside quake/gamedir/data/ (mode is FILE_READ, FILE_APPEND, or FILE_WRITE), returns fhandle >= 0 if successful, or fhandle < 0 if unable to open file for any reason
void(float fhandle) fclose = #111; // closes a file
string(float fhandle) fgets = #112; // reads a line of text from the file and returns as a tempstring
void(float fhandle, string s, ...) fputs = #113; // writes a line of text to the end of the file
float(string s) strlen = #114; // returns how many characters are in a string
string(string s, string...) strcat = #115; // concatenates 1 to 8 strings (for example "abc", "def" would return "abcdef") and returns as a tempstring
string(string s, float start, float length) substring = #116; // returns a section of a string as a tempstring - see FTE_STRINGS for enhanced version
vector(string s) stov = #117; // returns vector value from a string
string(string s, ...) strzone = #118; // makes a copy of a string into the string zone and returns it, this is often used to keep around a tempstring for longer periods of time (tempstrings are replaced often)
void(string s) strunzone = #119; // removes a copy of a string from the string zone (you can not use that string again or it may crash!!!)
//constants:
float FILE_READ = 0;
float FILE_APPEND = 1;
float FILE_WRITE = 2;
//cvars:
//pr_zone_min_strings : default 64 (64k), min 64 (64k), max 8192 (8mb)
//description:
//provides text file access functions and string manipulation functions, note that you may want to set pr_zone_min_strings in the worldspawn function if 64k is not enough string zone space.
//
//NOTE: strzone functionality is partially superseded by
//DP_QC_UNLIMITEDTEMPSTRINGS when longterm storage is not needed
//NOTE: substring is upgraded by FTE_STRINGS extension with negative start/length handling identical to php 5.2.0

//FTE_CSQC_SKELETONOBJECTS
//idea: Spike, LadyHavoc
//darkplaces implementation: LadyHavoc
//builtin definitions:
// all skeleton numbers are 1-based (0 being no skeleton)
// all bone numbers are 1-based (0 being invalid)
float(float modlindex) skel_create = #263; // create a skeleton (be sure to assign this value into .skeletonindex for use), returns skeleton index (1 or higher) on success, returns 0 on failure (for example if the modelindex is not skeletal), it is recommended that you create a new skeleton if you change modelindex, as the skeleton uses the hierarchy from the model.
float(float skel, entity ent, float modlindex, float retainfrac, float firstbone, float lastbone) skel_build = #264; // blend in a percentage of standard animation, 0 replaces entirely, 1 does nothing, 0.5 blends half, etc, and this only alters the bones in the specified range for which out of bounds values like 0,100000 are safe (uses .frame, .frame2, .frame3, .frame4, .lerpfrac, .lerpfrac3, .lerpfrac4, .frame1time, .frame2time, .frame3time, .frame4time), returns skel on success, 0 on failure
float(float skel) skel_get_numbones = #265; // returns how many bones exist in the created skeleton, 0 if skeleton does not exist
string(float skel, float bonenum) skel_get_bonename = #266; // returns name of bone (as a tempstring), "" if invalid bonenum (< 1 for example) or skeleton does not exist
float(float skel, float bonenum) skel_get_boneparent = #267; // returns parent num for supplied bonenum, 0 if bonenum has no parent or bone does not exist (returned value is always less than bonenum, you can loop on this)
float(float skel, string tagname) skel_find_bone = #268; // get number of bone with specified name, 0 on failure, bonenum (1-based) on success, same as using gettagindex but takes modelindex instead of entity
vector(float skel, float bonenum) skel_get_bonerel = #269; // get matrix of bone in skeleton relative to its parent - sets v_forward, v_right, v_up, returns origin (relative to parent bone)
vector(float skel, float bonenum) skel_get_boneabs = #270; // get matrix of bone in skeleton in model space - sets v_forward, v_right, v_up, returns origin (relative to entity)
void(float skel, float bonenum, vector org) skel_set_bone = #271; // set matrix of bone relative to its parent, reads v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
void(float skel, float bonenum, vector org) skel_mul_bone = #272; // transform bone matrix (relative to its parent) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
void(float skel, float startbone, float endbone, vector org) skel_mul_bones = #273; // transform bone matrices (relative to their parents) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bones)
void(float skeldst, float skelsrc, float startbone, float endbone) skel_copybones = #274; // copy bone matrices (relative to their parents) from one skeleton to another, useful for copying a skeleton to a corpse
void(float skel) skel_delete = #275; // deletes skeleton at the beginning of the next frame (you can add the entity, delete the skeleton, renderscene, and it will still work)
float(float modlindex, string framename) frameforname = #276; // finds number of a specified frame in the animation, returns -1 if no match found
float(float modlindex, float framenum) frameduration = #277; // returns the intended play time (in seconds) of the specified framegroup, if it does not exist the result is 0, if it is a single frame it may be a small value around 0.1 or 0.
//fields:
.float skeletonindex; // active skeleton overriding standard animation on model
.float frame; // primary framegroup animation (strength = 1 - lerpfrac - lerpfrac3 - lerpfrac4)
.float frame2; // secondary framegroup animation (strength = lerpfrac)
.float frame3; // tertiary framegroup animation (strength = lerpfrac3)
.float frame4; // quaternary framegroup animation (strength = lerpfrac4)
.float lerpfrac; // strength of framegroup blend
.float lerpfrac3; // strength of framegroup blend
.float lerpfrac4; // strength of framegroup blend
.float frame1time; // start time of framegroup animation
.float frame2time; // start time of framegroup animation
.float frame3time; // start time of framegroup animation
.float frame4time; // start time of framegroup animation
//description:
//this extension provides a way to do complex skeletal animation on an entity.
//
//see also DP_SKELETONOBJECTS (this extension implemented on server as well as client)
//
//notes:
//each model contains its own skeleton, reusing a skeleton with incompatible models will yield garbage (or not render).
//each model contains its own animation data, you can use animations from other model files (for example saving out all character animations as separate model files).
//if an engine supports loading an animation-only file format such as .md5anim in FTEQW, it can be used to animate any model with a compatible skeleton.
//proper use of this extension may require understanding matrix transforms (v_forward, v_right, v_up, origin), and you must keep in mind that v_right is negative for this purpose.
//
//features include:
//multiple animations blended together.
//animating a model with animations from another model with a compatible skeleton.
//restricting animation blends to certain bones of a model - for example independent animation of legs, torso, head.
//custom bone controllers - for example making eyes track a target location.
//
//
//
//example code follows...
//
//this helper function lets you identify (by parentage) what group a bone
//belongs to - for example "torso", "leftarm", would return 1 ("torso") for
//all children of the bone named "torso", unless they are children of
//"leftarm" (which is a child of "torso") which would return 2 instead...
float(float skel, float bonenum, string g1, string g2, string g3, string g4, string g5, string g6) example_skel_findbonegroup =
{
	local string bonename;
	while (bonenum >= 0)
	{
		bonename = skel_get_bonename(skel, bonenum);
		if (bonename == g1) return 1;
		if (bonename == g2) return 2;
		if (bonename == g3) return 3;
		if (bonename == g4) return 4;
		if (bonename == g5) return 5;
		if (bonename == g6) return 6;
		bonenum = skel_get_boneparent(skel, bonenum);
	}
	return 0;
};
// create a skeletonindex for our player using current modelindex
void() example_skel_player_setup =
{
	self.skeletonindex = skel_create(self.modelindex);
};
// setup bones of skeleton based on an animation
// note: animmodelindex can be a different model than self.modelindex
void(float animmodelindex, float framegroup, float framegroupstarttime) example_skel_player_update_begin =
{
	// start with our standard animation
	self.frame = framegroup;
	self.frame2 = 0;
	self.frame3 = 0;
	self.frame4 = 0;
	self.frame1time = framegroupstarttime;
	self.frame2time = 0;
	self.frame3time = 0;
	self.frame4time = 0;
	self.lerpfrac = 0;
	self.lerpfrac3 = 0;
	self.lerpfrac4 = 0;
	skel_build(self.skeletonindex, self, animmodelindex, 0, 0, 100000);
};
// apply a different framegroup animation to bones with a specified parent
void(float animmodelindex, float framegroup, float framegroupstarttime, float blendalpha, string groupbonename, string excludegroupname1, string excludegroupname2) example_skel_player_update_applyoverride =
{
	local float bonenum;
	local float numbones;
	self.frame = framegroup;
	self.frame2 = 0;
	self.frame3 = 0;
	self.frame4 = 0;
	self.frame1time = framegroupstarttime;
	self.frame2time = 0;
	self.frame3time = 0;
	self.frame4time = 0;
	self.lerpfrac = 0;
	self.lerpfrac3 = 0;
	self.lerpfrac4 = 0;
	bonenum = 0;
	numbones = skel_get_numbones(self.skeletonindex);
	while (bonenum < numbones)
	{
		if (example_skel_findbonegroup(self.skeletonindex, bonenum, groupbonename, excludegroupname1, excludegroupname2, "", "", "") == 1)
			skel_build(self.skeletonindex, self, animmodelindex, 1 - blendalpha, bonenum, bonenum + 1);
		bonenum = bonenum + 1;
	}
};
// make eyes point at a target location, be sure v_forward, v_right, v_up are set correctly before calling
void(vector eyetarget, string bonename) example_skel_player_update_eyetarget =
{
	local float bonenum;
	local vector ang;
	local vector oldforward, oldright, oldup;
	local vector relforward, relright, relup, relorg;
	local vector boneforward, boneright, boneup, boneorg;
	local vector parentforward, parentright, parentup, parentorg;
	local vector u, v;
	local vector modeleyetarget;
	bonenum = skel_find_bone(self.skeletonindex, bonename) - 1;
	if (bonenum < 0)
		return;
	oldforward = v_forward;
	oldright = v_right;
	oldup = v_up;
	v = eyetarget - self.origin;
	modeleyetarget_x =   v * v_forward;
	modeleyetarget_y = 0-v * v_right;
	modeleyetarget_z =   v * v_up;
	// this is an eyeball, make it point at the target location
	// first get all the data we can...
	relorg = skel_get_bonerel(self.skeletonindex, bonenum);
	relforward = v_forward;
	relright = v_right;
	relup = v_up;
	boneorg = skel_get_boneabs(self.skeletonindex, bonenum);
	boneforward = v_forward;
	boneright = v_right;
	boneup = v_up;
	parentorg = skel_get_boneabs(self.skeletonindex, skel_get_boneparent(self.skeletonindex, bonenum));
	parentforward = v_forward;
	parentright = v_right;
	parentup = v_up;
	// get the vector from the eyeball to the target
	u = modeleyetarget - boneorg;
	// now transform it inversely by the parent matrix to produce new rel vectors
	v_x = u * parentforward;
	v_y = u * parentright;
	v_z = u * parentup;
	ang = vectoangles2(v, relup);
	ang_x = 0 - ang_x;
	makevectors(ang);
	// set the relative bone matrix
	skel_set_bone(self.skeletonindex, bonenum, relorg);
	// restore caller's v_ vectors
	v_forward = oldforward;
	v_right = oldright;
	v_up = oldup;
};
// delete skeleton when we're done with it
// note: skeleton remains valid until next frame when it is really deleted
void() example_skel_player_delete =
{
	skel_delete(self.skeletonindex);
	self.skeletonindex = 0;
};
//
// END OF EXAMPLES FOR FTE_CSQC_SKELETONOBJECTS
//

//KRIMZON_SV_PARSECLIENTCOMMAND
//idea: KrimZon
//darkplaces implementation: KrimZon, LadyHavoc
//engine-called QC prototypes:
//void(string s) SV_ParseClientCommand;
//builtin definitions:
void(entity e, string s) clientcommand = #440;
float(string s) tokenize = #441;
string(float n) argv = #442;
//description:
//provides QC the ability to completely control server interpretation of client commands ("say" and "color" for example, clientcommand is necessary for this and substring (FRIK_FILE) is useful) as well as adding new commands (tokenize, argv, and stof (FRIK_FILE) are useful for this)), whenever a clc_stringcmd is received the QC function is called, and it is up to the QC to decide what (if anything) to do with it

//NEH_CMD_PLAY2
//idea: Nehahra
//darkplaces implementation: LadyHavoc
//description:
//shows that the engine supports the "play2" console command (plays a sound without spatialization).

//NEH_RESTOREGAME
//idea: Nehahra
//darkplaces implementation: LadyHavoc
//engine-called QC prototypes:
//void() RestoreGame;
//description:
//when a savegame is loaded, this function is called

//NEXUIZ_PLAYERMODEL
//idea: Nexuiz
//darkplaces implementation: Black
//console commands:
//playermodel <name> - FIXME: EXAMPLE NEEDED
//playerskin <name> - FIXME: EXAMPLE NEEDED
//field definitions:
.string playermodel; // name of player model sent by client
.string playerskin; // name of player skin sent by client
//description:
//these client properties are used by Nexuiz.

//NXQ_GFX_LETTERBOX
//idea: nxQuake
//darkplaces implementation: LadyHavoc
//description:
//shows that the engine supports the "r_letterbox" console variable, set to values in the range 0-100 this restricts the view vertically (and turns off sbar and crosshair), value is a 0-100 percentage of how much to constrict the view, <=0 = normal view height, 25 = 75% of normal view height, 50 = 50%, 75 = 25%, >=100 = no view

//PRYDON_CLIENTCURSOR
//idea: FrikaC
//darkplaces implementation: LadyHavoc
//effects bit:
float EF_SELECTABLE = 16384; // allows cursor to highlight entity (brighten)
//field definitions:
.float cursor_active; // true if cl_prydoncursor mode is on
.vector cursor_screen; // screen position of cursor as -1 to +1 in _x and _y (_z unused)
.vector cursor_trace_start; // position of camera
.vector cursor_trace_endpos; // position of cursor in world (as traced from camera)
.entity cursor_trace_ent; // entity the cursor is pointing at (server forces this to world if the entity is currently free at time of receipt)
//cvar definitions:
//cl_prydoncursor (0/1+, default 0, 1 and above use cursors named gfx/prydoncursor%03i.lmp - or .tga and such if DP_GFX_EXTERNALTEXTURES is implemented)
//description:
//shows that the engine supports the cl_prydoncursor cvar, this puts a clientside mouse pointer on the screen and feeds input to the server for the QuakeC to use as it sees fit.
//the mouse pointer triggers button4 if cursor is at left edge of screen, button5 if at right edge of screen, button6 if at top edge of screen, button7 if at bottom edge of screen.
//the clientside trace skips transparent entities (except those marked EF_SELECTABLE).
//the selected entity highlights only if EF_SELECTABLE is set, a typical selection method would be doubling the brightness of the entity by some means (such as colormod[] *= 2).
//intended to be used by Prydon Gate.

//TENEBRAE_GFX_DLIGHTS
//idea: Tenebrae
//darkplaces implementation: LadyHavoc
//fields:
.float light_lev; // radius (does not affect brightness), typical value 350
.vector color; // color (does not affect radius), typical value '1 1 1' (bright white), can be up to '255 255 255' (nuclear blast)
.float style; // light style (like normal light entities, flickering torches or switchable, etc)
.float pflags; // flags (see PFLAGS_ constants)
.vector angles; // orientation of the light
.float skin; // cubemap filter number, can be 1-255 (0 is assumed to be none, and tenebrae only allows 16-255), this selects a projective light filter, a value of 1 loads cubemaps/1posx.tga and cubemaps/1negx.tga and posy, negy, posz, and negz, similar to skybox but some sides need to be rotated or flipped
//constants:
float PFLAGS_NOSHADOW = 1; // light does not cast shadows
float PFLAGS_CORONA = 2; // light has a corona flare
float PFLAGS_FULLDYNAMIC = 128; // light enable (without this set no light is produced!)
//description:
//more powerful dynamic light settings
//warning: it is best not to use cubemaps on a light entity that has a model, as using a skin number that a model does not have will cause issues in glquake, and produce warnings in darkplaces (use developer 1 to see them)
//changes compared to tenebrae (because they're too 'leet' for standards):
//note: networking should send entities with PFLAGS_FULLDYNAMIC set even if they have no model (lights in general do not have a model, nor should they)
//EF_FULLDYNAMIC effects flag replaced by PFLAGS_FULLDYNAMIC flag (EF_FULLDYNAMIC conflicts with EF_NODRAW)

//TW_SV_STEPCONTROL
//idea: Transfusion
//darkplaces implementation: LadyHavoc
//cvars:
//sv_jumpstep (0/1, default 1)
//sv_stepheight (default 18)
//description:
//sv_jumpstep allows stepping up onto stairs while airborn, sv_stepheight controls how high a single step can be.

//FTE_QC_CHECKPVS
//idea: Urre
//darkplaces implementation: divVerent
//builtin definitions:
float checkpvs(vector viewpos, entity viewee) = #240;
//description:
//returns true if viewee can be seen from viewpos according to PVS data

//FTE_STRINGS
//idea: many
//darkplaces implementation: KrimZon
//builtin definitions:
float(string str, string sub, float startpos) strstrofs = #221; // returns the offset into a string of the matching text, or -1 if not found, case sensitive
float(string str, float ofs) str2chr = #222; // returns the character at the specified offset as an integer, or 0 if an invalid index, or byte value - 256 if the engine supports UTF8 and the byte is part of an extended character
string(float c, ...) chr2str = #223; // returns a string representing the character given, if the engine supports UTF8 this may be a multi-byte sequence (length may be more than 1) for characters over 127.
string(float ccase, float calpha, float cnum, string s, ...) strconv = #224; // reformat a string with special color characters in the font, DO NOT USE THIS ON UTF8 ENGINES (if you are lucky they will emit ^4 and such color codes instead), the parameter values are 0=same/1=lower/2=upper for ccase, 0=same/1=white/2=red/5=alternate/6=alternate-alternate for redalpha, 0=same/1=white/2=red/3=redspecial/4=whitespecial/5=alternate/6=alternate-alternate for rednum.
string(float chars, string s, ...) strpad = #225; // pad string with spaces to a specified length, < 0 = left padding, > 0 = right padding
string(string info, string key, string value, ...) infoadd = #226; // sets or adds a key/value pair to an infostring - note: forbidden characters are \ and "
string(string info, string key) infoget = #227; // gets a key/value pair in an infostring, returns value or null if not found
float(string s1, string s2) strcmp = #228; // compare two strings
float(string s1, string s2, float len) strncmp = #228; // compare two strings up to the specified number of characters, if their length differs and is within the specified limit the result will be negative, otherwise it is the difference in value of their first non-matching character.
float(string s1, string s2) strcasecmp = #229; // compare two strings with case-insensitive matching, characters a-z are considered equivalent to the matching A-Z character, no other differences, and this does not consider special characters equal even if they look similar
float(string s1, string s2, float len) strncasecmp = #230; // same as strcasecmp but with a length limit, see strncmp
//string(string s, float start, float length) substring = #116; // see note below
//description:
//various string manipulation functions
//note: substring also exists in FRIK_FILE but this extension adds negative start and length as valid cases (see note above), substring is consistent with the php 5.2.0 substr function (not 5.2.3 behavior)
//substring returns a section of a string as a tempstring, if given negative
// start the start is measured back from the end of the string, if given a
// negative length the length is the offset back from the end of the string to
// stop at, rather than being relative to start, if start is negative and
// larger than length it is treated as 0.
// examples of substring:
// substring("blah", -3, 3) returns "lah"
// substring("blah", 3, 3) returns "h"
// substring("blah", -10, 3) returns "bla"
// substring("blah", -10, -3) returns "b"

//DP_CON_BESTWEAPON
//idea: many
//darkplaces implementation: divVerent
//description:
//allows QC to register weapon properties for use by the bestweapon command, for mods that change required ammo count or type for the weapons
//it is done using console commands sent via stuffcmd:
//  register_bestweapon quake
//  register_bestweapon clear
//  register_bestweapon <shortname> <impulse> <itemcode> <activeweaponcode> <ammostat> <ammomin>
//for example, this is what Quake uses:
//  register_bestweapon 1 1 4096 4096 6 0 // STAT_SHELLS is 6
//  register_bestweapon 2 2    1    1 6 1
//  register_bestweapon 3 3    2    2 6 1
//  register_bestweapon 4 4    4    4 7 1 // STAT_NAILS is 7
//  register_bestweapon 5 5    8    8 7 1
//  register_bestweapon 6 6   16   16 8 1 // STAT_ROCKETS is 8
//  register_bestweapon 7 7   32   32 8 1
//  register_bestweapon 8 8   64   64 9 1 // STAT_CELLS is 9
//after each map client initialization, this is reset back to Quake settings. So you should send these data in ClientConnect.
//also, this extension introduces a new "cycleweapon" command to the user.

//DP_QC_STRINGBUFFERS
//idea: ??
//darkplaces implementation: LadyHavoc
//functions to manage string buffer objects - that is, arbitrary length string arrays that are handled by the engine
float() buf_create = #460;
void(float bufhandle) buf_del = #461;
float(float bufhandle) buf_getsize = #462;
void(float bufhandle_from, float bufhandle_to) buf_copy = #463;
void(float bufhandle, float sortpower, float backward) buf_sort = #464;
string(float bufhandle, string glue) buf_implode = #465;
string(float bufhandle, float string_index) bufstr_get = #466;
void(float bufhandle, float string_index, string str) bufstr_set = #467;
float(float bufhandle, string str, float order) bufstr_add = #468;
void(float bufhandle, float string_index) bufstr_free = #469;

//DP_QC_STRINGBUFFERS_CVARLIST
//idea: divVerent
//darkplaces implementation: divVerent
//functions to list cvars and store their names into a stringbuffer
//cvars that start with pattern but not with antipattern will be stored into the buffer
void(float bufhandle, string pattern, string antipattern) buf_cvarlist = #517;

//DP_QC_STRINGBUFFERS_EXT_WIP
//idea: VorteX
//darkplaces implementation: VorteX
//constant definitions:
const float MATCH_AUTO = 0;
const float MATCH_WHOLE = 1;
const float MATCH_LEFT = 2;
const float MATCH_RIGHT = 3;
const float MATCH_MIDDLE = 4;
const float MATCH_PATTERN = 5;
//builtin definitions:
float(string filename, float bufhandle) buf_loadfile = #535; // append each line of file as new buffer string, return 1 if succesful
float(float filehandle, float bufhandle, float startpos, float numstrings) buf_writefile = #536; // writes buffer strings as lines, returns 1 if succesful
float(float bufhandle, string match, float matchrule, float startpos, float step) bufstr_find = #537; // returns string index
float(string s, string pattern, float matchrule) matchpattern = #538; // returns 0/1
float(string s, string pattern, float matchrule, float pos) matchpatternofs = #538;
//description:
//provides a set of functions to manipulate with string buffers
//pattern wildcards: * - any character (or no characters), ? - any 1 character
//Warning: This extension is work-in-progress, it may be changed/revamped/removed at any time, dont use it if you dont want any trouble
//wip note: UTF8 is not supported yet

//DP_QC_STRREPLACE
//idea: Sajt
//darkplaces implementation: Sajt
//builtin definitions:
string(string search, string replace, string subject) strreplace = #484;
string(string search, string replace, string subject) strireplace = #485;
//description:
//strreplace replaces all occurrences of 'search' with 'replace' in the string 'subject', and returns the result as a tempstring.
//strireplace does the same but uses case-insensitive matching of the 'search' term

//DP_SV_SHUTDOWN
//idea: divVerent
//darkplaces implementation: divVerent
//A function that gets called just before progs exit. To save persistent data from.
//It is not called on a crash or error.
//void SV_Shutdown();

//EXT_CSQC
// #232 void(float index, float type, .void field) SV_AddStat (EXT_CSQC)
void(float index, float type, ...) addstat = #232;

//ZQ_PAUSE
//idea: ZQuake
//darkplaces implementation: GreEn`mArine
//builtin definitions:
void(float pause) setpause = #531;
//function definitions:
//void(float elapsedtime) SV_PausedTic;
//description:
//during pause the world is not updated (time does not advance), SV_PausedTic is the only function you can be sure will be called at regular intervals during the pause, elapsedtime is the current system time in seconds since the pause started (not affected by slowmo or anything else).
//
//calling setpause(0) will end a pause immediately.
//
//Note: it is worth considering that network-related functions may be called during the pause (including customizeentityforclient for example), and it is also important to consider the continued use of the KRIMZON_SV_PARSECLIENTCOMMAND extension while paused (chatting players, etc), players may also join/leave during the pause.  In other words, the only things that are not called are think and other time-related functions.

//DP_COVERAGE
//idea: divVerent
//darkplaces implementation: divVerent
//function definitions:
void coverage() = #642;  // Reports a coverage event. The engine counts for each of the calls to this builtin whether it has been called.


// EXPERIMENTAL (not finalized) EXTENSIONS:

//DP_CRYPTO
//idea: divVerent
//darkplaces implementation: divVerent
//field definitions: (SVQC)
.string crypto_keyfp; // fingerprint of CA key the player used to authenticate
.string crypto_mykeyfp; // fingerprint of CA key the server used to authenticate to the player
.string crypto_idfp; // fingerprint of ID used by the player entity, or string_null if not identified
.float crypto_idfp_signed; // set if the player's ID has been signed
.string crypto_encryptmethod; // the string "AES128" if encrypting, and string_null if plaintext
.string crypto_signmethod; // the string "HMAC-SHA256" if signing, and string_null if plaintext
// there is no field crypto_myidfp, as that info contains no additional information the QC may have a use for
//builtin definitions: (SVQC)
float(string url, float id, string content_type, string delim, float buf, float keyid) crypto_uri_postbuf = #513;
//description:
//use -1 as buffer handle to justs end delim as postdata

//DP_USERMOVETYPES
//idea: divVerent
//darkplaces implementation: Mario
//movetype definitions:
float MOVETYPE_USER_FIRST = 128;
float MOVETYPE_USER_LAST = 191;
//description:
//user defined movetypes can be added between the start and end points, without producing unknown movetype warnings

//DP_RM_CLIPGROUP
//idea: Akari
//darkplaces implementation: Akari
//field definitions:
.float clipgroup;
//description:
//If two entities have this field set to the same non-zero integer value, they won't collide with each other.

//idea: eukara
//darkplaces implementation: Cloudwalk
//Do NOT use in production yet. Uncomment if you intend to use it
//string __fullspawndata;
//description:
//http://icculus.org/finger/marco?date=2019-01-25&time=05-38-02

//DP_QC_FS_SEARCH_PACKFILE
//idea: Mario
//darkplaces implementation: Mario
//builtin definitions:
float(string pattern, float caseinsensitive, float quiet, string packfile) search_packfile_begin = #444;
//description:
//extension to search_begin (DP_QC_FS_SEARCH), performs a filename search with the specified pattern (for example "maps/*.bsp") and stores the results in a search slot (minimum of 128 supported by any engine with this extension), the other functions take this returned search slot number, be sure to search_free when done (they are also freed on progs reload).
//only searches for files within the specified packfile, which is expected to match the results of whichpack().

//EXT_CSQC (registercommand for CSQC, now also available in SVQC)
//idea: probably Spoike
//darkplaces implementation: Cloudwalk
//builtin definitions:
void(string cmdname) registercommand = #352;
//engine-called QC prototypes:
//float CSQC_ConsoleCommand(string command);
//float ConsoleCmd(string command);
//description:
//Adds a new console command which will take priority over a previous command of the same name (including engine commands) and in CSQC is removed when the VM shuts down. This will call CSQC_ConsoleCommand(string command) or ConsoleCmd(string command) in SVQC.  Return value should be true if QC handled the command, otherwise return false to have the engine handle it.

//DP_QC_FINDBOX
//idea: Mario
//darkplaces implementation: bones_was_here
//builtin definitions:
entity(vector mins, vector maxs) findbox = #566;
entity(vector mins, vector maxs, .entity tofield) findbox_tofield = #566;
//description:
//Returns a chain of entities that are touching a box (a simpler findradius); supports DP_QC_FINDCHAIN_TOFIELD

//DP_QC_NUDGEOUTOFSOLID
//idea: LadyHavoc, bones_was_here
//darkplaces implementation: LadyHavoc, bones_was_here
//builtin definitions:
float(entity ent) nudgeoutofsolid = #567;
//cvar definitions:
//sv_gameplayfix_nudgeoutofsolid_separation
//description:
//Attempts to move a stuck entity out of solid brushes, returning 1 if successful, 0 if it remains stuck, -1 if it wasn't stuck.
//Note: makes only one tracebox call if the entity isn't stuck, so don't call tracebox just to see if you should call nudgeoutofsolid.
//Uses a "smart" method considering surface properties and supporting multiple surfaces,
//except on Q1BSP with mod_q1bsp_polygoncollisions 0 (there it falls back to the unsticking method).


float(float dividend, float divisor) mod = #245;
