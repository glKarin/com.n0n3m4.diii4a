//common builtins.

void(vector ang)	makevectors		= #1;		// sets v_forward, etc globals
void(entity e, vector o) setorigin	= #2;
void(entity e, string m) setmodel	= #3;		// set movetype and solid first
void(entity e, vector min, vector max) setsize = #4;

void() break						= #6;
float() random						= #7;		// returns 0 - 1
void(entity e, float chan, string samp, float vol, float atten) sound = #8;
vector(vector v) normalize			= #9;
void(string e) error				= #10;
void(string e) objerror				= #11;
float(vector v) vlen				= #12;
float(vector v) vectoyaw			= #13;
entity() spawn						= #14;
void(entity e) remove				= #15;

// sets trace_* globals
// nomonsters can be:
// An entity will also be ignored for testing if forent == test,
// forent->owner == test, or test->owner == forent
// a forent of world is ignored
void(vector v1, vector v2, float nomonsters, entity forent) traceline = #16;


entity(entity start, .string fld, string match) find = #18;
string(string s) precache_sound		= #19;
string(string s) precache_model		= #20;
void(entity client, string s)stuffcmd = #21;
entity(vector org, float rad) findradius = #22;

void(string s, ...) dprint				= #25;
string(float f) ftos				= #26;
string(vector v) vtos				= #27;
void() coredump						= #28;		// prints all edicts
void() traceon						= #29;		// turns statment trace on
void() traceoff						= #30;
void(entity e) eprint				= #31;		// prints an entire edict

float() droptofloor= #34;	// TRUE if landed on floor

float(float v) rint					= #36;		// round to nearest int
float(float v) floor				= #37;		// largest integer <= v
float(float v) ceil					= #38;		// smallest integer >= v

float(vector v) pointcontents		= #41;		// returns a CONTENT_*

float(float f) fabs = #43;

float(string s) cvar = #45;						// return cvar.value
void(string s, ...) localcmd = #46;					// put string into local que
entity(entity e) nextent = #47;					// for looping through all ents
void(vector o, vector d, float color, float count) particle = #48;// start a particle effect
void() ChangeYaw = #49;						// turn towards self.ideal_yaw
											// at self.yaw_speed

vector(vector v) vectoangles			= #51;




void(string var, string val) cvar_set = #72;	// sets cvar.value



//string(float modelindex) name_for_modelindex;
//float(string modelname) modelindex_for_name;

string(string s) precache_file          = #68;  // no effect except for -copy
string(string s) precache_model2        = #75;          // registered version only
string(string s) precache_sound2        = #76;          // registered version only
string(string s) precache_file2         = #77;          // registered version only



float(string s) stof	= #81;


void(vector v1, vector min, vector max, vector v2, float nomonsters, entity forent) tracebox = #90;
void(string name, string value) registercvar = #93;

entity(entity start, .entity fld, entity match) findentity = #98;
entity(entity start, .float fld, float match) findfloat = #98;



float(string filename, float mode) fopen = #110;
void(float fhandle) fclose = #111;
string(float fhandle) fgets = #112;
void(float fhandle, string s) fputs = #113;
float(string s) strlen = #114;
string(...) strcat = #115;
string(string s, float start, float length) substring = #116;
vector(string s) stov = #117;
string(string s) strzone = #118;
void(string s) strunzone = #119;
float FILE_READ = 0;
float FILE_WRITE = 1;
float FILE_APPEND = 2;











float(string s) tokenize = #441;
string(float argnum) argv = #442;


//DP_QC_SINCOSSQRTPOW
//idea: id Software, LordHavoc
//darkplaces implementation: id Software, LordHavoc
//builtin definitions:
float(float val) sin = #60;
float(float val) cos = #61;
float(float val) sqrt = #62;
float(float a, float b) pow = #97;
//description:
//useful math functions, sine of val, cosine of val, square root of val, and raise a to power b, respectively.




//FTE_STRINGS
float(string str, string match) strstrofs = #221;
float(string str, float ofs) str2chr = #222;
string(float c, ...) chr2str = #223;
string(float ccase, float redalpha, float redchars, string str) strconv = #224;
string(string info, string key, string value) infoadd = #226;
string(string info, string key) infoget = #227;
float(string s1, string s2, float len) strncmp = #228;
float(string s1, string s2) strcasecmp = #229;
float(string s1, string s2, float len) strncasecmp = #230;


float(entity ent, string tagname) gettagindex = #451;
vector(entity ent, float tagnum) RotateVectorsByTag = #234;
void(vector angle) RotateVectorsByAngle = #235;
void(vector fwd, vector right, vector up) RotateVectorsByVectors = #236;

float(float modelindex, string skinname) skinforname = #237;
float(string skinname) shaderforname = #238;
