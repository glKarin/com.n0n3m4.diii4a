/*
==============================================================================

			SOURCE FOR GLOBALVARS_T C STRUCTURE
			MUST NOT BE MODIFIED, OR CRC ERRORS WILL APPEAR

==============================================================================
*/

//
// system globals
//
entity		self;
entity		other;
entity		world;
float		time;
float		frametime;

float 		player_localentnum;	//the entnum
float 		player_localnum;	//the playernum
float		maxclients;	//a constant filled in by the engine. gah, portability eh?

float		clientcommandframe;	//player movement
float		servercommandframe;	//clientframe echoed off the server

string		mapname;

//
// global variables set by built in functions
//
vector		v_forward, v_up, v_right;	// set by makevectors()

// set by traceline / tracebox
float		trace_allsolid;
float		trace_startsolid;
float		trace_fraction;
vector		trace_endpos;
vector		trace_plane_normal;
float		trace_plane_dist;
entity		trace_ent;
float		trace_inopen;
float		trace_inwater;

//
// prog functions called by engine
//
void  CSQC_Init(float apilevel, string enginename, float engineversion);
void  CSQC_Shutdown();
float CSQC_InputEvent(float evtype, float scanx, float chary);
void  CSQC_UpdateView(float vid_width, float vid_height, float notmenu); // required for EXT_CSQC (preferred)
float CSQC_ConsoleCommand(string cmdstr);
#ifdef CSQC_SIMPLE // hud-only CSQC
	void CSQC_DrawHud(vector virtsize, float showscores); // required for CSQC_SIMPLE (fallback)
	void CSQC_DrawScores(vector virtsize, float showscores);
#endif

//these fields are read and set by the default player physics
vector		pmove_org;
vector		pmove_vel;
vector		pmove_mins;
vector		pmove_maxs;
//retrieved from the current movement commands (read by player physics)
float		input_timelength;
vector		input_angles;
vector		input_movevalues;	//forwards, right, up.
float		input_buttons;		//attack, use, jump (default physics only uses jump)

float		movevar_gravity;
float		movevar_stopspeed;
float		movevar_maxspeed;
float		movevar_spectatormaxspeed;	//used by NOCLIP movetypes.
float		movevar_accelerate;
float		movevar_airaccelerate;
float		movevar_wateraccelerate;
float		movevar_friction;
float		movevar_waterfriction;
float		movevar_entgravity;	//the local player's gravity field. Is a multiple (1 is the normal value)

//================================================
void		end_sys_globals;		// flag for structure dumping
//================================================

/*
==============================================================================

			SOURCE FOR ENTVARS_T C STRUCTURE
			MUST NOT BE MODIFIED, OR CRC ERRORS WILL APPEAR

==============================================================================
*/

//
// system fields (*** = do not set in prog code, maintained by C code)
//
.float		modelindex;		// *** model index in the precached list
.vector		absmin, absmax;	// *** origin + mins / maxs

.float		entnum;	// *** the ent number as on the server
.float		drawmask;
.void()		predraw;

.float		movetype;
.float		solid;

.vector		origin;			// ***
.vector		oldorigin;		// ***
.vector		velocity;
.vector		angles;
.vector		avelocity;

.string		classname;		// spawn function
.string		model;
.float		frame;
.float		skin;
.float		effects;

.vector		mins, maxs;		// bounding box extents reletive to origin
.vector		size;			// maxs - mins

.void()		touch;
.void()		use;
.void()		think;
.void()		blocked;		// for doors or plats, called when can't push other

.float		nextthink;

.entity		chain;

.string		netname;

.entity 	enemy;

.float		flags;

.float		colormap;

.entity		owner;		// who launched a missile

//================================================
void		end_sys_fields;			// flag for structure dumping
//================================================

/*
==============================================================================

			OPTIONAL FIELDS AND GLOBALS

==============================================================================
*/

// Additional OPTIONAL Fields and Globals
float		intermission; // indicates intermission state (0 = normal, 1 = scores, 2 = finale text)

vector		view_angles; // same as input_angles
vector		view_punchangle; // from server
vector		view_punchvector; // from server

/*
==============================================================================

			CONSTANT DEFINITIONS

==============================================================================
*/

const float MASK_ENGINE			= 1;
const float MASK_ENGINEVIEWMODELS	= 2;
const float MASK_NORMAL			= 4;

const float RF_VIEWMODEL	= 1;
const float RF_EXTERNALMODEL	= 2;
const float RF_DEPTHHACK	= 4;
const float RF_ADDITIVE		= 8;
const float RF_USEAXIS		= 16;

const float VF_MIN		= 1;	//(vector)
const float VF_MIN_X		= 2;	//(float)
const float VF_MIN_Y		= 3;	//(float)
const float VF_SIZE		= 4;	//(vector) (viewport size)
const float VF_SIZE_Y		= 5;	//(float)
const float VF_SIZE_X		= 6;	//(float)
const float VF_VIEWPORT		= 7;	//(vector, vector)
const float VF_FOV		= 8;	//(vector)
const float VF_FOVX		= 9;	//(float)
const float VF_FOVY		= 10;	//(float)
const float VF_ORIGIN		= 11;	//(vector)
const float VF_ORIGIN_X		= 12;	//(float)
const float VF_ORIGIN_Y		= 13;	//(float)
const float VF_ORIGIN_Z		= 14;	//(float)
const float VF_ANGLES		= 15;	//(vector)
const float VF_ANGLES_X		= 16;	//(float)
const float VF_ANGLES_Y		= 17;	//(float)
const float VF_ANGLES_Z		= 18;	//(float)
const float VF_DRAWWORLD	= 19;	//(float)
const float VF_DRAWENGINESBAR	= 20;	//(float)
const float VF_DRAWCROSSHAIR	= 21;	//(float)

const float VF_CL_VIEWANGLES	= 33;	//(vector)
const float VF_CL_VIEWANGLES_X	= 34;	//(float)
const float VF_CL_VIEWANGLES_Y	= 35;	//(float)
const float VF_CL_VIEWANGLES_Z	= 36;	//(float)

const float VF_PERSPECTIVE      = 200;

const float STAT_HEALTH		= 0;
const float STAT_WEAPONMODEL	= 2;
const float STAT_AMMO		= 3;
const float STAT_ARMOR		= 4;
const float STAT_WEAPONFRAME	= 5;
const float STAT_SHELLS		= 6;
const float STAT_NAILS		= 7;
const float STAT_ROCKETS	= 8;
const float STAT_CELLS		= 9;
const float STAT_ACTIVEWEAPON	= 10;
const float STAT_TOTALSECRETS	= 11;
const float STAT_TOTALMONSTERS	= 12;
const float STAT_SECRETS		= 13;
const float STAT_MONSTERS		= 14;
const float STAT_ITEMS		= 15;
const float STAT_VIEWHEIGHT	= 16;

// Quake Sound Constants
const float CHAN_AUTO			= 0;
const float CHAN_WEAPON			= 1;
const float CHAN_VOICE			= 2;
const float CHAN_ITEM			= 3;
const float CHAN_BODY			= 4;

const float ATTN_NONE			= 0;
const float ATTN_NORM			= 1;
const float ATTN_IDLE			= 2;
const float ATTN_STATIC			= 3;

// Frik File Constants
const float FILE_READ			= 0;
const float FILE_APPEND			= 1;
const float FILE_WRITE			= 2;

// Quake Point Contents
const float CONTENT_EMPTY					= -1;
const float CONTENT_SOLID					= -2;
const float CONTENT_WATER					= -3;
const float CONTENT_SLIME					= -4;
const float CONTENT_LAVA					= -5;
const float CONTENT_SKY						= -6;

// Quake Solid Constants
const float SOLID_NOT						= 0;
const float SOLID_TRIGGER					= 1;
const float SOLID_BBOX						= 2;
const float SOLID_SLIDEBOX					= 3;
const float SOLID_BSP						= 4;
const float SOLID_CORPSE					= 5;

// Quake Move Constants
const float MOVE_NORMAL						= 0;
const float MOVE_NOMONSTERS					= 1;
const float MOVE_MISSILE					= 2;

// Boolean Constants
const float true	= 1;
const float false	= 0;
const float TRUE	= 1;
const float FALSE	= 0;

const float EXTRA_LOW = -99999999;
const float EXTRA_HIGH = 99999999;

const vector VEC_1	= '1 1 1';
const vector VEC_0	= '0 0 0';
const vector VEC_M1	= '-1 -1 -1';

const float M_PI = 3.14159265358979323846;

vector	VEC_HULL_MIN = '-16 -16 -24';
vector	VEC_HULL_MAX = '16 16 32';

// Quake Temporary Entity Constants
const float TE_SPIKE						= 0;
const float TE_SUPERSPIKE					= 1;
const float TE_GUNSHOT						= 2;
const float TE_EXPLOSION					= 3;
const float TE_TAREXPLOSION					= 4;
const float TE_LIGHTNING1					= 5;
const float TE_LIGHTNING2					= 6;
const float TE_WIZSPIKE						= 7;
const float TE_KNIGHTSPIKE					= 8;
const float TE_LIGHTNING3					= 9;
const float TE_LAVASPLASH					= 10;
const float TE_TELEPORT						= 11;
const float TE_EXPLOSION2					= 12;
	// Darkplaces Additions
	const float TE_EXPLOSIONRGB					= 53;
	const float TE_GUNSHOTQUAD					= 57;
	const float TE_EXPLOSIONQUAD 				= 70;
	const float TE_SPIKEQUAD					= 58;
	const float TE_SUPERSPIKEQUAD				= 59;

// PFlags for Dynamic Lights
const float PFLAGS_NOSHADOW					= 1;
const float PFLAGS_CORONA					= 2;
const float PFLAGS_FULLDYNAMIC				= 128;

const float EF_ADDITIVE		= 32;
const float EF_BLUE		= 64;
const float EF_FLAME		= 1024;
const float EF_FULLBRIGHT	= 512;
const float EF_NODEPTHTEST	= 8192;
const float EF_NODRAW		= 16;
const float EF_NOSHADOW		= 4096;
const float EF_RED		= 128;
const float EF_STARDUST		= 2048;
const float EF_SELECTABLE 	= 16384;

const float PFL_ONGROUND	= 1;
const float PFL_CROUCH		= 2;
const float PFL_DEAD		= 4;
const float PFL_GIBBED		= 8;

// draw flags
const float DRAWFLAG_NORMAL = 0;
const float DRAWFLAG_ADDITIVE = 1;
const float DRAWFLAG_MODULATE = 2;
const float DRAWFLAG_2XMODULATE = 3;
const float DRAWFLAG_SCREEN = 4;
const float DRAWFLAG_MIPMAP = 0x100; // only for R_BeginPolygon

/*
==============================================================================

			BUILTIN DEFINITIONS
			EXTENSIONS ARE NOT ADDED HERE, BUT BELOW!

==============================================================================
*/

void(vector ang) makevectors = #1;
void(entity e, vector o) setorigin = #2;
void(entity e, string m) setmodel = #3;
void(entity e, vector min, vector max) setsize = #4;

void() break_to_debugger = #6;
float() random = #7; // Returns a random number > 0 and < 1
void(entity e, float chan, string samp, float volume, float atten, ...) sound = #8;
vector(vector v) normalize = #9;
void(string e) error = #10;
void(string e) objerror = #11;
float(vector v) vlen = #12;
float(vector v) vectoyaw = #13;
entity() spawn = #14;
void(entity e) remove = #15;
float(vector v1, vector v2, float tryents, entity ignoreentity) traceline = #16;

entity(entity start, .string fld, string match) find = #18;
void(string s) precache_sound = #19;
void(string s) precache_model = #20;

entity(vector org, float rad) findradius = #22;

void(string s, ...) dprint = #25;
string(float f) ftos = #26;
string(vector v) vtos = #27;
void() coredump = #28;
void() traceon = #29;
void() traceoff = #30;
void(entity e) eprint = #31;
// settrace optional
float(float yaw, float dist, float settrace) walkmove = #32;

float() droptofloor = #34;
void(float style, string value) lightstyle = #35;
float(float v) rint = #36;
float(float v) floor = #37;
float(float v) ceil = #38;

float(entity e) checkbottom = #40;
float(vector v) pointcontents = #41;

float(float f) fabs = #43;

float(string s) cvar = #45;
void(string s, ...) localcmd = #46;
entity(entity e) nextent = #47;
void(vector o, vector d, float color, float count) particle = #48;
void() ChangeYaw = #49;

vector(vector v) vectoangles = #51;
vector(vector v, vector w) vectoangles2 = #51;

float(float f) sin = #60;
float(float f) cos = #61;
float(float f) sqrt = #62;
void(entity ent) changepitch = #63;
void(entity e, entity ignore) tracetoss = #64;
string(entity ent) etos = #65;

string(string s) precache_file = #68;
void(entity e) makestatic = #69;

void(string var, string val) cvar_set = #72;

void(vector pos, string samp, float vol, float atten) ambientsound = #74;
string(string s) precache_model2 = #75;
string(string s) precache_sound2 = #76;
string(string s) precache_file2 = #77;

float(string s) stof = #81;


void(vector v1, vector min, vector max, vector v2, float nomonsters, entity forent) tracebox = #90;
vector() randomvec = #91;
vector(vector org) getlight = #92;
vector(vector org, float lpflags) getlight2 = #92;
vector getlight_dir;
vector getlight_ambient;
vector getlight_diffuse;
const float LP_LIGHTMAP	= 1;
const float LP_RTWORLD = 2;
const float LP_DYNLIGHT = 4;
const float LP_COMPLETE = 7;

float(string name, string value) registercvar = #93;
float( float a, ... ) min = #94;
float( float b, ... ) max = #95;
float(float minimum, float val, float maximum) bound = #96;
float(float f, float f) pow = #97;
entity(entity start, .float fld, float match) findfloat = #98;
entity(entity start, .entity fld, entity match) findentity = #98;
float(string s) checkextension = #99;
// FrikaC and Telejano range #100-#199

float(string filename, float mode) fopen = #110;
void(float fhandle) fclose = #111;
string(float fhandle) fgets = #112;
void(float fhandle, string s) fputs = #113;
float(string s) strlen = #114;
string(string s, string...) strcat = #115;
string(string s, float start, float length) substring = #116;
vector(string) stov = #117;
string(string s) strzone = #118;
void(string s) strunzone = #119;
void(string s, float chan, float vol) localsound = #177;

// FTEQW range #200-#299

float(float number, float quantity) bitshift = #218;

//float(string str, string sub[, float startpos]) strstrofs = #221;
float(string str, string sub, float startpos) strstrofs = #221;
float(string str, float ofs) str2chr = #222;
string(float c, ...) chr2str = #223;
string(float ccase, float calpha, float cnum, string s, ...) strconv = #224;
string(float chars, string s, ...) strpad = #225;
string(string info, string key, string value, ...) infoadd = #226;
string(string info, string key) infoget = #227;
float(string s1, string s2) strcmp = #228;
float(string s1, string s2, float len) strncmp = #228;
float(string s1, string s2) strcasecmp = #229;
float(string s1, string s2, float len) strncasecmp = #230;

// CSQC range #300-#399
void() clearscene = #300;
void(float mask) addentities = #301;
void(entity ent) addentity = #302;
float(float property, ...) setproperty = #303;
float(float property) getproperty = #309;
vector(float property) getpropertyvec = #309;
void() renderscene = #304;
void(vector org, float radius, vector lightcolours) adddynamiclight = #305;
void(vector org, float radius, vector lightcolours, float style, string cubemapname, float pflags) adddynamiclight2 = #305;
//void(string texturename, float flag[, float is2d, float lines]) R_BeginPolygon = #306;
void(string texturename, float flag, ...) R_BeginPolygon = #306;
void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex = #307;
void() R_EndPolygon = #308;
vector (vector v) cs_unproject = #310;
vector (vector v) cs_project = #311;

void(float width, vector pos1, vector pos2, vector rgb, float alpha, float flag) drawline = #315;
float(string name) iscachedpic = #316;
string(string name, ...) precache_pic = #317;
string(string name) precache_cubemap = #317;
vector(string picname) draw_getimagesize = #318;
void(string name) freepic = #319;
float(vector position, float character, vector scale, vector rgb, float alpha, float flag) drawcharacter = #320;
float(vector position, string text, vector scale, vector rgb, float alpha, float flag) drawstring = #321;
float(vector position, string pic, vector size, vector rgb, float alpha, float flag) drawpic = #322;
float(vector position, vector size, vector rgb, float alpha, float flag) drawfill = #323;
void(float x, float y, float width, float height) drawsetcliparea = #324;
void(void) drawresetcliparea = #325;
float(vector position, string text, vector scale, float alpha, float flag) drawcolorcodedstring = #326;
vector(vector position, string text, vector scale, vector rgb, float alpha, float flag) drawcolorcodedstring2 = #326;

float(float stnum) getstatf = #330;
float(float stnum, ...) getstati = #331; // can optionally take first bit and count
string(float firststnum) getstats = #332;
void(entity e, float mdlindex) setmodelindex = #333;
string(float mdlindex) modelnameforindex = #334;
float(string effectname) particleeffectnum = #335;
void(entity ent, float effectnum, vector start, vector end) trailparticles = #336;
//void(float effectnum, vector origin [, vector dir, float count]) pointparticles = #337;
void(float effectnum, vector origin , vector dir, float count) pointparticles = #337;
void(string s, ...) centerprint = #338;
void(string s, ...) print = #339;
string(float keynum) keynumtostring = #340;
float(string keyname) stringtokeynum = #341;
string(float keynum) getkeybind = #342;
void(float usecursor) setcursormode = #343;
vector() getmousepos = #344;
float(float framenum) getinputstate = #345;
void(float sens) setsensitivityscale = #346;
void(...) runstandardplayerphysics = #347; // this may or may not take a player ent
string(float playernum, string keyname) getplayerkeyvalue = #348;
float() isdemo = #349;
float() isserver = #350;
void(vector origin, vector forward, vector right, vector up) SetListener = #351;
void(string cmdname) registercommand = #352;
float(entity ent) wasfreed = #353;
string(string key) serverkey = #354;

// Use proper case; refer to the id1 Write* functions!
float() ReadByte = #360;
float() ReadChar = #361;
float() ReadShort = #362;
float() ReadLong = #363;
float() ReadCoord = #364;
float() ReadAngle = #365;
string() ReadString = #366;
float() ReadFloat = #367;

// LadyHavoc's range #400-#499
void(entity from, entity to) copyentity = #400;

entity(.string fld, string match) findchain = #402;
entity(.float fld, float match) findchainfloat = #403;
void(vector org, string modelname, float startframe, float endframe, float framerate) effect = #404;
void(vector org, vector velocity, float howmany) te_blood = #405;
void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower = #406;
void(vector org, vector color) te_explosionrgb = #407;
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube = #408;
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain = #409;
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow = #410;
void(vector org, vector vel, float howmany) te_spark = #411;
void(vector org) te_gunshotquad = #412;
void(vector org) te_spikequad = #413;
void(vector org) te_superspikequad = #414;
void(vector org) te_explosionquad = #415;
void(vector org) te_smallflash = #416;
void(vector org, float radius, float lifetime, vector color) te_customflash = #417;
void(vector org) te_gunshot = #418;
void(vector org) te_spike = #419;
void(vector org) te_superspike = #420;
void(vector org) te_explosion = #421;
void(vector org) te_tarexplosion = #422;
void(vector org) te_wizspike = #423;
void(vector org) te_knightspike = #424;
void(vector org) te_lavasplash = #425;
void(vector org) te_teleport = #426;
void(vector org, float colorstart, float colorlength) te_explosion2 = #427;
void(entity own, vector start, vector end) te_lightning1 = #428;
void(entity own, vector start, vector end) te_lightning2 = #429;
void(entity own, vector start, vector end) te_lightning3 = #430;
void(entity own, vector start, vector end) te_beam = #431;
void(vector dir) vectorvectors = #432;
void(vector org) te_plasmaburn = #433;
float(entity e, float s) getsurfacenumpoints = #434;
vector(entity e, float s, float n) getsurfacepoint = #435;
vector(entity e, float s) getsurfacenormal = #436;
string(entity e, float s) getsurfacetexture = #437;
float(entity e, vector p) getsurfacenearpoint = #438;
vector(entity e, float s, vector p) getsurfaceclippedpoint = #439;

float(string s) tokenize = #441;
string(float n) argv = #442;
void(entity e, entity tagentity, string tagname) setattachment = #443;
float(string pattern, float caseinsensitive, float quiet) search_begin = #444;
void(float handle) search_end = #445;
float(float handle) search_getsize = #446;
string(float handle, float num) search_getfilename = #447;
string(string s) cvar_string = #448;
entity(entity start, .float fld, float match) findflags = #449;
entity(.float fld, float match) findchainflags = #450;
float(entity ent, string tagname) gettagindex = #451;
vector(entity ent, float tagindex) gettaginfo = #452;

void(vector org, vector vel, float howmany) te_flamejet = #457;

entity(float num) entitybyindex = #459;
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

float(float s) asin = #471;
float(float c) acos = #472;
float(float t) atan = #473;
float(float c, float s) atan2 = #474;
float(float a) tan = #475;
float(string s) strippedstringlen = #476;
float(string s) strlennocol = #476; // This is the correct name for the function, but not removing the strippedstringlen mapping.
string(string s) decolorizedstring = #477;
string(string s) strdecolorize = #477; // This is the correct name for the function, but not removing the decolorizedstring mapping.
string(float uselocaltime, string format, ...) strftime = #478;
string(string s) strtolower = #480;
string(string s) strtoupper = #481;
string(string s) cvar_defstring = #482;
void(vector origin, string sample, float volume, float attenuation) pointsound = #483;
string(string search, string replace, string subject) strreplace = #484;
string(string search, string replace, string subject) strireplace = #485;
vector(entity e, float s, float n, float a) getsurfacepointattribute = #486;
#ifdef SUPPORT_GECKO
float gecko_create( string name ) = #487;
void gecko_destroy( string name ) = #488;
void gecko_navigate( string name, string URI ) = #489;
float gecko_keyevent( string name, float key, float eventtype ) = #490;
void gecko_mousemove( string name, float x, float y ) = #491;
void gecko_resize( string name, float w, float h ) = #492;
vector gecko_get_texture_extent( string name ) = #493;
#else

#endif

/*
==============================================================================

			EXTENSION DEFINITIONS

==============================================================================
*/

// DP_CSQC_SPAWNPARTICLE
// idea: VorteX
// darkplaces implementation: VorteX
// constant definitions:
// particle base behavior:
float PT_ALPHASTATIC = 1;
float PT_STATIC = 2;
float PT_SPARK = 3;
float PT_BEAM = 4;
float PT_RAIN = 5;
float PT_RAINDECAL = 6;
float PT_SNOW = 7;
float PT_BUBBLE = 8;
float PT_BLOOD = 9;
float PT_SMOKE = 10;
float PT_DECAL = 11;
float PT_ENTITYPARTICLE = 12;
// particle blendtypes:
float PBLEND_ALPHA = 0;
float PBLEND_ADD = 1;
float PBLEND_INVMOD = 2;
// particle orientation:
float PARTICLE_BILLBOARD = 0;
float PARTICLE_SPARK = 1;
float PARTICLE_ORIENTED_DOUBLESIDED = 2;
float PARTICLE_BEAM = 3;
// global definitions:
float particle_type; // one of PT_
float particle_blendmode; // one of PBLEND_ values
float particle_orientation; // one of PARTICLE_ values
vector particle_color1;
vector particle_color2;
float particle_tex; // number of chunk in particlefont
float particle_size;
float particle_sizeincrease;
float particle_alpha;
float particle_alphafade;
float particle_time;
float particle_gravity;
float particle_bounce;
float particle_airfriction;
float particle_liquidfriction;
float particle_originjitter;
float particle_velocityjitter;
float particle_qualityreduction; // enable culling of this particle when FPS is low
float particle_stretch;
vector particle_staincolor1;
vector particle_staincolor2;
float particle_staintex;
float particle_stainalpha;
float particle_stainsize;
float particle_delayspawn;
float particle_delaycollision;
float particle_angle;
float particle_spin;
// builtin definitions:
float(float max_themes) initparticlespawner = #522; // check fields/globals for integration and enable particle spawner, return 1 is succeded, otherwise returns 0
void() resetparticle = #523; // reset p_ globals to default theme #0
void(float theme) particletheme = #524; // restore p_ globals from saved theme
float() particlethemesave = #525; // save p_ globals to new particletheme and return it's index
void(float theme) particlethemeupdate = #525; // save p_ globals to new particletheme and return it's index
void(float theme) particlethemefree = #526; // delete a particle theme
float(vector org, vector vel) spawnparticle = #527; // returns 0 when failed, 1 when spawned
float(vector org, vector vel, float theme) quickparticle = #527; // not reading globals, just theme, returns 0 when failed, 1 when spawned
float(vector org, vector vel, float delay, float collisiondelay) delayedparticle = #528;
float(vector org, vector vel, float delay, float collisiondelay, float theme) quickdelayedparticle = #528;
// description: this builtin provides an easy and flexible way to spawn particles,
// it is not created as replace for DP_SV_POINTPARTICLES but as an addition to it.
// With this extension you can create a specific particles like rain particles, or entity particles
// notes:
// 1) 0 is default particle template, it could be changed
// 2) color vectors could have value 0-255 of each component
// restrictions: max themes could be between 4 and 2048
// warning: you should call initparticlespawner() at very beginning BEFORE all other particle spawner functions
// function to query particle info
// don't remove this function as it protects all particle_ globals from FTEQCC/FRIKQCC non-referenced removal optimisation
void() printparticle =
{
	// vortex: this also protects from 'non-referenced' optimisation on some compilers
	print("PARTICLE:\n");
	print(strcat("  type: ", ftos(particle_type), "\n"));
	print(strcat("  blendmode: ", ftos(particle_blendmode), "\n"));
	print(strcat("  orientation: ", ftos(particle_orientation), "\n"));
	print(strcat("  color1: ", vtos(particle_color1), "\n"));
	print(strcat("  color2: ", vtos(particle_color2), "\n"));
	print(strcat("  tex: ", ftos(particle_tex), "\n"));
	print(strcat("  size: ", ftos(particle_size), "\n"));
	print(strcat("  sizeincrease: ", ftos(particle_sizeincrease), "\n"));
	print(strcat("  alpha: ", ftos(particle_alpha), "\n"));
	print(strcat("  alphafade: ", ftos(particle_alphafade), "\n"));
	print(strcat("  time: ", ftos(particle_time), "\n"));
	print(strcat("  gravity: ", ftos(particle_gravity), "\n"));
	print(strcat("  bounce: ", ftos(particle_bounce), "\n"));
	print(strcat("  airfriction: ", ftos(particle_airfriction), "\n"));
	print(strcat("  liquidfriction: ", ftos(particle_liquidfriction), "\n"));
	print(strcat("  originjitter: ", ftos(particle_originjitter), "\n"));
	print(strcat("  velocityjitter: ", ftos(particle_velocityjitter), "\n"));
	print(strcat("  qualityreduction: ", ftos(particle_qualityreduction), "\n"));
	print(strcat("  stretch: ", ftos(particle_stretch), "\n"));
	print(strcat("  staincolor1: ", vtos(particle_staincolor1), "\n"));
	print(strcat("  staincolor2: ", vtos(particle_staincolor2), "\n"));
	print(strcat("  staintex: ", ftos(particle_staintex), "\n"));
	print(strcat("  stainalpha: ", ftos(particle_stainalpha), "\n"));
	print(strcat("  stainsize: ", ftos(particle_stainsize), "\n"));
	print(strcat("  delayspawn: ", ftos(particle_delayspawn), "\n"));
	print(strcat("  delaycollision: ", ftos(particle_delaycollision), "\n"));
	print(strcat("  angle: ", ftos(particle_angle), "\n"));
	print(strcat("  spin: ", ftos(particle_spin), "\n"));
}

// DP_CSQC_ENTITYTRANSPARENTSORTING_OFFSET
// idea: VorteX
// darkplaces implementation: VorteX
float RF_USETRANSPARENTOFFSET = 64; // enables transparent origin offsetting
// global definitions
float transparent_offset; // should be set before entity is added
// description: offset a model's meshes origin used for transparent sorting. Could be used to tweak sorting bugs on very large transparent entities or hacking transparent sorting order for certain objects
// example: transparent_offset = 1000000; // entity always appear on background of other transparents
// note: offset is done in view forward axis

// DP_CSQC_ENTITYWORLDOBJECT
// idea: VorteX
// darkplaces implementation: VorteX
const float RF_WORLDOBJECT = 128;
// description: when renderflag is set, engine will not use culling methods for this entity, e.g. it will always be drawn
// useful for large outdoor objects (like asteroids on sky horizon or sky models)

// DP_CSQC_ENTITYMODELLIGHT
// idea: VorteX
// darkplaces implementation: VorteX
const float RF_MODELLIGHT = 4096;
.vector modellight_ambient;
.vector modellight_diffuse;
.vector modellight_dir;
// description: allows CSQC to override directional model lightning on entity

// DP_CSQC_SETPAUSE
// idea: VorteX
// darkplaces implementation: VorteX
// builtin definitions:
void(float ispaused) setpause = #531;
// description: provides ability to set pause in local games (similar to one set once console is activated)
// not stopping sound/cd track, useful for inventory screens, ingame menus with input etc.

// DP_CSQC_QUERYRENDERENTITY
// idea: VorteX
// darkplaces implementation: VorteX
// constant definitions:
// render entity fields:
float E_ACTIVE 	    = 0; // float 0/1
float E_ORIGIN      = 1; // vector
float E_FORWARD     = 2; // vector
float E_RIGHT       = 3; // vector
float E_UP          = 4; // vector
float E_SCALE       = 5; // float
float E_ORIGINANDVECTORS = 6; // returns origin, + sets v_* vectors to orientation
float E_ALPHA       = 7; // float
float E_COLORMOD    = 8; // vector
float E_PANTSCOLOR  = 9; // vector
float E_SHIRTCOLOR  = 10; // vector
float E_SKIN        = 11; // float
float E_MINS        = 12; // vector
float E_MAXS        = 13; // vector
float E_ABSMIN      = 14; // vector
float E_ABSMAX      = 15; // vector
float E_LIGHT       = 16; // vector - modellight
// builtin definitions:
float(float entitynum, float fldnum) getentity = #504;
vector(float entitynum, float fldnum) getentityvec = #504;
// description: allows to query parms from render entities, especially useful with attaching CSQC ents to
// server entities networked and interpolated by engine (monsters, players), number of entity is it's SVQC number
// you can send it via tempentity/CSQC entity message. Note that this builtin doesnt know about entity removing/reallocating
// so it's meaning to work for short period of time, dont use it on missiles/grenades whatever will be removed next five seconds

//DP_GFX_FONTS
//idea: Blub\0, divVerent
//darkplaces implementation: Blub\0
//console commands:
//  loadfont fontname fontmaps size1 size2 ...
//   A font can simply be gfx/tgafile (freetype fonts doent need extension),
//   or alternatively you can specify multiple fonts and faces
//   Like this: gfx/vera-sans:2,gfx/fallback:1
//   to load face 2 of the font gfx/vera-sans and use face 1
//   of gfx/fallback as fallback font
//   You can also specify a list of font sizes to load, like this:
//   loadfont console gfx/conchars,gfx/fallback 8 12 16 24 32
//   In many cases, 8 12 16 24 32 should be a good choice.
//   for slots see:
//constant definitions:
float drawfont;            // set it before drawstring()/drawchar() calls
float FONT_DEFAULT = 0;    // 'default'
float FONT_CONSOLE = 1;    // 'console', REALLY should be fixed width (ls!)
float FONT_SBAR = 2;       // 'sbar', used on hud, must be fixed width
float FONT_NOTIFY = 3;     // 'notify', used on sprint/bprint
float FONT_CHAT = 4;       // 'chat'
float FONT_CENTERPRINT = 5;// 'centerprint'
float FONT_INFOBAR = 6;    // 'infobar'
float FONT_MENU = 7;       // 'menu', should be fixed width
float FONT_USER0 = 8;      // 'user0', userdefined fonts
float FONT_USER1 = 9;      // 'user1', userdefined fonts
float FONT_USER2 = 10;     // 'user2', userdefined fonts
float FONT_USER3 = 11;     // 'user3', userdefined fonts
float FONT_USER4 = 12;     // 'user4', userdefined fonts
float FONT_USER5 = 13;     // 'user5', userdefined fonts
float FONT_USER6 = 14;     // 'user6', userdefined fonts
float FONT_USER7 = 15;     // 'user7' slot, userdefined fonts
//builtin definitions:
float findfont(string s) = #356; // find font by fontname and return it's index
float loadfont(string fontname, string fontmaps, string sizes, float slot, float fix_scale, float fix_voffset) = #357;
// loads font immediately so stringwidth() function can be used just after builtin call
// returns a font slotnum (which is used to set drawfont to)
// first 3 parms are identical to "loadfont" console command ones
// slot could be one of FONT_ constants or result of findfont() or -1 to not use it
// if slot is given, font will be loaded to this slotnum and fontname become new title for it
// this way you can rename user* fonts to something more usable
// fix_* parms let you fix badly made fonts by applying some transformations to them
// fix_scale : per-character center-oriented scale (doesn't change line height at all)
// fix_voffset : vertical offset for each character, it's a multiplier to character height
float stringwidth(string text, float allowColorCodes, vector size) = #327; // get a width of string with given font and char size
float stringwidth_menu(string text, float allowColorCodes, vector size) = #468; // in menu.dat it has different builtin #
//description: engine support for custom fonts in console, hud, qc etc.
// limits:
//  max 128 chars for font name
//  max 3 font fallbacks
//  max 8 sizes per font

//DP_GFX_FONTS_FREETYPE
//idea: Blub\0, divVerent
//darkplaces implementation: Blub\0
//cvar definitions:
//   r_font_disable_freetype 0/1 : disable freetype fonts loading (uttetly disables freetype library initialization)
//   r_font_antialias 0/1 : antialiasing when loading font
//   r_font_hint 0/1/2/3  : hinting when loading font, 0 is no hinting, 1 light autohinting , 2 full autohinting, 3 full hinting
//   r_font_postprocess_blur X      : font outline blur amount
//   r_font_postprocess_outline X   : font outline width
//   r_font_postprocess_shadow_x X  : font outline shadow x shift amount, applied during outlining
//   r_font_postprocess_shadow_y X  : font outline shadow y shift amount, applied during outlining
//   r_font_postprocess_shadow_z X  : font outline shadow z shift amount, applied during blurring
//description: engine support for truetype/freetype fonts
//so .AFM+.PFB/.OTF/.TTF files could be stuffed as fontmaps in loadfont()
//(console command version will support them as well)

//DP_CSQC_BINDMAPS
//idea: daemon, motorsep
//darkplaces implementation: divVerent
//builtin definitions:
string(float key, float bindmap) getkeybind_bindmap = #342;
float(float key, string bind, float bindmap) setkeybind_bindmap = #630;
vector(void) getbindmaps = #631;
float(vector bm) setbindmaps = #632;
string(string command, float bindmap) findkeysforcommand = #610;
//<already in EXT_CSQC> float(string key) stringtokeynum = #341;
//<already in EXT_CSQC> string(float keynum) keynumtostring = #340;
//description: key bind setting/getting including support for switchable
//bindmaps.

//DP_CRYPTO
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions: (CSQC)
float(string url, float id, string content_type, string delim, float buf, float keyid) crypto_uri_postbuf = #513;
//description:
//use -1 as buffer handle to justs end delim as postdata

//DP_CSQC_MAINVIEW
//idea: divVerent
//darkplaces implementation: divVerent
//constant definitions:
const float VF_MAINVIEW         = 400;
//use setproperty(VF_MAINVIEW, 1); before calling R_RenderView for the render
//that shall become the "main" view, which is e.g. used by PRYDON_CLIENTCURSOR
//this flag is set for the first scene, and not cleared by R_ClearScene
//this flag is automatically cleared by R_RenderView
//so when not using this extension, the first view rendered is the main view

//DP_CSQC_MINFPS_QUALITY
//idea: divVerent
//darkplaces implementation: divVerent
//constant definitions:
const float VF_MINFPS_QUALITY   = 401;
//use getproperty(VF_MINFPS_QUALITY); to do CSQC based LOD based on cl_minfps
//1 should lead to an unmodified view

//DP_CSQC_V_CALCREFDEF_WIP1
//DP_CSQC_V_CALCREFDEF_WIP2
//idea: divVerent
//darkplaces implementation: divVerent
//builtin definitions:
void(entity e, float refdefflags) V_CalcRefdef = #640;
//constant definitions:
float PMF_DUCKED = 4;
float PMF_ONGROUND = 8;
float REFDEFFLAG_TELEPORTED = 1;
float REFDEFFLAG_JUMPING = 2;
float REFDEFFLAG_DEAD = 4;
float REFDEFFLAG_INTERMISSION = 8;
//- use this on the player entity after performing prediction
//- pass REFDEFFLAG_TELEPORTED if the player teleported since last frame
//- pass REFDEFFLAG_JUMPING if jump button is pressed
//- pass REFDEFFLAG_DEAD if dead (DP_CSQC_V_CALCREFDEF_WIP2)
//- pass REFDEFFLAG_INTERMISSION if in intermission (DP_CSQC_V_CALCREFDEF_WIP2)
//- the player entity needs to have origin, velocity, pmove_flags set according
//  to prediction (the above two PMF_ flags are used in the player's pmove_flags)
//- NOTE: to check for this, ALSO OR a check with DP_CSQC_V_CALCREFDEF to also support
//  the finished extension once done

// assorted builtins
float	drawsubpic(vector position, vector size, string pic, vector srcPosition, vector srcSize, vector rgb, float alpha, float flag) = #328;
vector	drawgetimagesize(string pic) = #318;
#define SPA_POSITION 0
#define SPA_S_AXIS 1
#define SPA_T_AXIS 2
#define SPA_R_AXIS 3
#define SPA_TEXCOORDS0 4
#define SPA_LIGHTMAP0_TEXCOORDS 5
#define SPA_LIGHTMAP_COLOR 6
// float (entity e, float s)						getsurfacenumpoints = #434;
// vector (entity e, float s, float n)					getsurfacepoint = #435;
// vector (entity e, float s)						getsurfacenormal = #436;
// string (entity e, float s)						getsurfacetexture = #437;
// float (entity e, vector p)						getsurfacenearpoint = #438;
// vector (entity e, float s, vector p)					getsurfaceclippedpoint = #439;
// vector(entity e, float s, float n, float a) getsurfacepointattribute = #486;
float(entity e, float s) getsurfacenumtriangles = #628;
vector(entity e, float s, float n) getsurfacetriangle = #629;

//DP_QC_ASINACOSATANATAN2TAN
//idea: Urre
//darkplaces implementation: LadyHavoc
//constant definitions:
float DEG2RAD = 0.0174532925199432957692369076848861271344287188854172545609719144;
float RAD2DEG = 57.2957795130823208767981548141051703324054724665643215491602438612;
float PI      = 3.1415926535897932384626433832795028841971693993751058209749445923;
//builtin definitions:
/* -Wdouble-declaration
float(float s) asin = #471; // returns angle in radians for a given sin() value, the result is in the range -PI*0.5 to PI*0.5
float(float c) acos = #472; // returns angle in radians for a given cos() value, the result is in the range 0 to PI
float(float t) atan = #473; // returns angle in radians for a given tan() value, the result is in the range -PI*0.5 to PI*0.5
float(float c, float s) atan2 = #474; // returns angle in radians for a given cos() and sin() value pair, the result is in the range -PI to PI (this is identical to vectoyaw except it returns radians rather than degrees)
float(float a) tan = #475; // returns tangent value (which is simply sin(a)/cos(a)) for the given angle in radians, the result is in the range -infinity to +infinity
*/
//description:
//useful math functions for analyzing vectors, note that these all use angles in radians (just like the cos/sin functions) not degrees unlike makevectors/vectoyaw/vectoangles, so be sure to do the appropriate conversions (multiply by DEG2RAD or RAD2DEG as needed).
//note: atan2 can take unnormalized vectors (just like vectoyaw), and the function was included only for completeness (more often you want vectoyaw or vectoangles), atan2(v_x,v_y) * RAD2DEG gives the same result as vectoyaw(v)

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
//  length modifiers: h for forcing a float, l for forcing an int/entity (by default, %d etc. cast a float to int)
//  conversions:
//    d takes a float if no length is specified or h is, and an int/entity if l is specified as length, and cast it to an int
//    i takes an int/entity if no length is specified or i is, and a float if h is specified as length, and cast it to an int
//    ouxXc take a float if no length is specified or h is, and an int/entity if l is specified as length, and cast it to an unsigned int
//    eEfFgG take a float if no length is specified or h is, and an int/entity if l is specified as length, and cast it to a double
//    s takes a string
//    vV takes a vector, and processes the three components as if it were a gG for all three components, separated by space
//    For conversions s and c, the flag # makes precision and width interpreted
//      as byte count, by default it is interpreted as character count in UTF-8
//      enabled engines. No other conversions can create wide characters, and #
//      has another meaning in these.

//DP_QC_GETTIME
//idea: tZork
//darkplaces implementation: tZork, divVerent
//constant definitions:
float GETTIME_FRAMESTART = 0; // time of start of frame relative to an arbitrary point in time (time of worldspawn)
float GETTIME_REALTIME = 1; // current time (may be OS specific)
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

//DP_PRECACHE_PIC_FLAGS
//idea: divVerent
//darkplaces implementation: divVerent
//constant definitions:
float PRECACHE_PIC_FROMWAD = 1; // this one actually is part of EXT_CSQC
float PRECACHE_PIC_NOTPERSISTENT = 2; // picture may get deallocated when unused
float PRECACHE_PIC_MIPMAP = 8; // mipmap the texture for possibly better downscaling at memory expense
//notes: these constants are given as optional second argument to precache_pic()

//DP_QC_TRACE_MOVETYPE_WORLDONLY
//idea: LadyHavoc
//darkplaces implementation: LadyHavoc
//constant definitions:
float MOVE_WORLDONLY = 3;
//description:
//allows traces to hit only world (ignoring all entities, unlike MOVE_NOMONSTERS which hits all bmodels), use as the nomonsters parameter to trace functions

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

//DP_QC_NUM_FOR_EDICT
//idea: Blub\0
//darkplaces implementation: Blub\0
//Function to get the number of an entity - a clean way.
float(entity num) num_for_edict = #512;

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

//DP_QC_CRC16
//idea: divVerent
//darkplaces implementation: divVerent
//Some hash function to build hash tables with. This has to be be the CRC-16-CCITT that is also required for the QuakeWorld download protocol.
//When caseinsensitive is set, the CRC is calculated of the lower cased string.
float(float caseinsensitive, string s, ...) crc16 = #494;

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

//DP_QC_LOG
//darkplaces implementation: divVerent
//builtin definitions:
float log(float f) = #532;
//description:
//logarithm

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

//DP_QC_FINDCHAIN_TOFIELD
//see: dpextensions.qc
entity(vector org, float rad, .entity tofield) findradius_tofield = #22;
entity(.string fld, string match, .entity tofield) findchain_tofield = #402;
entity(.float fld, float match, .entity tofield) findchainflags_tofield = #450;
entity(.float fld, float match, .entity tofield) findchainfloat_tofield = #403;

//DP_COVERAGE
//idea: divVerent
//darkplaces implementation: divVerent
//function definitions:
void coverage() = #642;  // Reports a coverage event. The engine counts for each of the calls to this builtin whether it has been called.

//DP_RM_CLIPGROUP
//see: dpextensions.qc
.float clipgroup;

//DP_QC_FS_SEARCH_PACKFILE
//idea: Mario
//darkplaces implementation: Mario
//builtin definitions:
float(string pattern, float caseinsensitive, float quiet, string packfile) search_packfile_begin = #444;
//description:
//extension to search_begin (DP_QC_FS_SEARCH), performs a filename search with the specified pattern (for example "maps/*.bsp") and stores the results in a search slot (minimum of 128 supported by any engine with this extension), the other functions take this returned search slot number, be sure to search_free when done (they are also freed on progs reload).
//only searches for files within the specified packfile, which is expected to match the results of whichpack().

//DP_QC_FINDBOX
//see: dpextensions.qc
entity(vector mins, vector maxs) findbox = #566;
entity(vector mins, vector maxs, .entity tofield) findbox_tofield = #566;

//DP_QC_NUDGEOUTOFSOLID
//see: dpextensions.qc
float(entity ent) nudgeoutofsolid = #567;

// assorted builtins
const float		STAT_MOVEVARS_TICRATE           = 240;
const float		STAT_MOVEVARS_TIMESCALE         = 241;
const float		STAT_FRAGLIMIT					= 235;
const float		STAT_TIMELIMIT					= 236;
const float     STAT_MOVEVARS_GRAVITY           = 242;
string(void)						ReadPicture = #501;
float PARTICLES_USEALPHA = 1;
float particles_alphamin, particles_alphamax;
float PARTICLES_USECOLOR = 2;
vector particles_colormin, particles_colormax;
float PARTICLES_USEFADE = 4;  // fades the COUNT (fade alpha using alphamin/alphamax)
float particles_fade;
float PARTICLES_DRAWASTRAIL = 128;
void(float effectindex, entity own, vector org_from, vector org_to, vector dir_from, vector dir_to, float countmultiplier, float flags) boxparticles = #502;
float trace_networkentity;
const float RF_FULLBRIGHT	= 256;
const float RF_NOSHADOW	= 512;
float RF_DYNAMICMODELLIGHT = 8192;

float gettaginfo_parent;
string gettaginfo_name;
vector gettaginfo_offset;
vector gettaginfo_forward;
vector gettaginfo_right;
vector gettaginfo_up;
float checkpvs(vector viewpos, entity viewee) = #240;
float mod(float dividend, float divisor) = #245;
