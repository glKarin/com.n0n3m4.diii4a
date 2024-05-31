//////////////////////////////////////////////////////////
// sys globals

entity self;

/////////////////////////////////////////////////////////
void		end_sys_globals;
/////////////////////////////////////////////////////////
// sys fields

/////////////////////////////////////////////////////////
void		end_sys_fields;
/////////////////////////////////////////////////////////
// sys functions

void() m_init;
void(float keynr, float ascii) m_keydown;
void(float width, float height) m_draw;
void(float mode) m_toggle;
void() m_shutdown;
// optional: float(float) m_gethostcachecategory;

/////////////////////////////////////////////////////////
// sys constants
///////////////////////////
// key dest constants

float KEY_UNKNOWN	= 	-1;
float KEY_GAME 		=	0;
float KEY_MENU		=	2;
float KEY_MENU_GRABBED	= 	3;

///////////////////////////
// file constants

float FILE_READ = 0;
float FILE_APPEND = 1;
float FILE_WRITE = 2;

///////////////////////////
// logical constants (just for completeness)

float TRUE 	= 1;
float FALSE = 0;

///////////////////////////
// boolean constants

float true = 1;
float false = 0;

///////////////////////////
// msg constants

float MSG_BROADCAST		= 0;		// unreliable to all
float MSG_ONE			= 1;		// reliable to one (msg_entity)
float MSG_ALL			= 2;		// reliable to all
float MSG_INIT			= 3;		// write to the init string

/////////////////////////////
// mouse target constants

float MT_MENU = 1;
float MT_CLIENT = 2;

/////////////////////////
// client state constants

float CS_DEDICATED 		= 0;
float CS_DISCONNECTED 	= 1;
float CS_CONNECTED		= 2;

///////////////////////////
// blend flags

float DRAWFLAG_NORMAL		= 0;
float DRAWFLAG_ADDITIVE 	= 1;
float DRAWFLAG_MODULATE 	= 2;
float DRAWFLAG_2XMODULATE   = 3;

///////////////////////////
// null entity (actually it is the same like the world entity)

entity null_entity;

///////////////////////////
// error constants

// file handling
float ERR_CANNOTOPEN			= -1; // fopen
float ERR_NOTENOUGHFILEHANDLES 	= -2; // fopen
float ERR_INVALIDMODE 			= -3; // fopen
float ERR_BADFILENAME 			= -4; // fopen

// drawing functions

float ERR_NULLSTRING			= -1;
float ERR_BADDRAWFLAG			= -2;
float ERR_BADSCALE				= -3;
float ERR_BADSIZE				= -3; // same as ERR_BADSCALE
float ERR_NOTCACHED				= -4;

// server list stuff
float SLIST_HOSTCACHEVIEWCOUNT  = 0;
float SLIST_HOSTCACHETOTALCOUNT = 1;
float SLIST_MASTERQUERYCOUNT 	= 2;
float SLIST_MASTERREPLYCOUNT 	= 3;
float SLIST_SERVERQUERYCOUNT 	= 4;
float SLIST_SERVERREPLYCOUNT 	= 5;
float SLIST_SORTFIELD		= 6;
float SLIST_SORTDESCENDING	= 7;
float SLIST_LEGACY_LINE1 	= 1024;
float SLIST_LEGACY_LINE2 	= 1025;
float SLIST_TEST_CONTAINS	= 0;
float SLIST_TEST_NOTCONTAIN	= 1;
float SLIST_TEST_LESSEQUAL	= 2;
float SLIST_TEST_LESS		= 3;
float SLIST_TEST_EQUAL		= 4;
float SLIST_TEST_GREATER	= 5;
float SLIST_TEST_GREATEREQUAL	= 6;
float SLIST_TEST_NOTEQUAL	= 7;
float SLIST_TEST_STARTSWITH	= 8;
float SLIST_TEST_NOTSTARTSWITH	= 9;
float SLIST_MASK_AND = 0;
float SLIST_MASK_OR  = 512;

// font stuff
float FONT_DEFAULT     = 0;
float FONT_CONSOLE     = 1;
float FONT_SBAR        = 2;
float FONT_NOTIFY      = 3;
float FONT_CHAT        = 4;
float FONT_CENTERPRINT = 5;
float FONT_INFOBAR     = 6;
float FONT_MENU        = 7;
float FONT_USER        = 8; // add to this the index, like FONT_USER+3 = user3. At least 8 of them are supported.
float drawfont;

/* not supported at the moment
///////////////////////////
// os constants

float OS_WINDOWS	= 0;
float OS_LINUX		= 1;
float OS_MAC		= 2;
*/










//////////////////////////////////////////////////
// common cmd
//////////////////////////////////////////////////
// AK FIXME: Create perhaps a special builtin file for the common cmds

float 	checkextension(string ext) 	= #1;

// error cmds
void 	error(string err,...) 		= #2;
void 	objerror(string err,...) 	= #3;

// print

void 	print(string text,...) 		 	 = #4;
void 	bprint(string text,...)			 = #5;
void	sprint(float clientnum, string text,...) = #6;
void 	centerprint(string text,...)		 = #7;

// vector stuff

vector	normalize(vector v) 		= #8;
float 	vlen(vector v)			= #9;
float  	vectoyaw(vector v)		= #10;
vector 	vectoangles(vector v)		= #11;

float	random(void)  = #12; // Returns a random number > 0 and < 1

void	cmd(string command, ...) = #13;
void localcmd(string command, ...) = #13;

// cvar cmds

float 	cvar(string name) 			= #14;
const string str_cvar(string name)		= #71;
void	cvar_set(string name, string value)	= #15;

void	dprint(string text,...) = #16;

// conversion functions

string	ftos(float f)  	= #17;
float	fabs(float f)	= #18;
string	vtos(vector v)  = #19;
string	etos(entity e)	= #20;

float	stof(string val,...)  = #21;

entity	spawn(void) 		= #22;
void	remove(entity e) 	= #23;

entity	find(entity start, .string field, string match)  	= #24;
entity	findfloat(entity start, .float field, float match) 	= #25;
entity	findentity(entity start, .entity field, entity match) 	= #25;

entity	findchainstring(.string field, string match) 	= #26;
entity	findchainfloat(.float field, float match) 	= #27;
entity	findchainentity(.entity field, entity match)	= #27;

string	precache_file(string file) 	= #28;
string	precache_sound(string sample) 	= #29;

void	crash(void)	= #72;
void	coredump(void) 	= #30;
void	stackdump(void) = #73;
void	traceon(void) 	= #31;
void	traceoff(void) 	= #32;

void	eprint(entity e)  	= #33;
float	rint(float f) 		= #34;
float	floor(float f)  	= #35;
float	ceil(float f)  		= #36;
entity	nextent(entity e)  	= #37;
float	sin(float f)  		= #38;
float	cos(float f)  		= #39;
float	sqrt(float f)  		= #40;
vector	randomvec(void)  	= #41;

float	registercvar(string name, string value, float flags)  = #42; // returns 1 if success

float	min(float f,...)  = #43;
float	max(float f,...)  = #44;

float	bound(float min,float value, float max)  = #45;
float	pow(float a, float b)  = #46;

void	copyentity(entity src, entity dst)  = #47;

float	fopen(string filename, float mode)  = #48;
void	fclose(float fhandle)  = #49;
string	fgets(float fhandle)  = #50;
void	fputs(float fhandle, string s)  = #51;

float	strlen(string s)  = #52;
string	strcat(string s, string...)  = #53;
string	substring(string s, float start, float length)  = #54;

vector	stov(string s)  = #55;

string	strzone(string s)  = #56;
void	strunzone(string s) = #57;

float	tokenize(string s)  = #58;
string	argv(float n)  = #59;

float	isserver(void)  = #60;
float	clientcount(void)  = #61;
float	clientstate(void)  = #62;
void	clientcommand(float client, string s)  = #63;
void	changelevel(string map)  = #64;
void	localsound(string sample)  = #65;
vector	getmousepos(void)  	= #66;
float	gettime(void)		= #67;
void 	loadfromdata(string data) = #68;
void	loadfromfile(string file) = #69;

float	mod(float val, float m) = #70;

float	search_begin(string pattern, float caseinsensitive, float quiet) = #74;
void	search_end(float handle) = #75;
float	search_getsize(float handle) = #76;
string	search_getfilename(float handle, float num) = #77;

string	chr(float ascii) = #78;

/////////////////////////////////////////////////
// Write* Functions
/////////////////////////////////////////////////
void	WriteByte(float data, float dest, float desto)	= #401;
void	WriteChar(float data, float dest, float desto)	= #402;
void	WriteShort(float data, float dest, float desto)	= #403;
void	WriteLong(float data, float dest, float desto)	= #404;
void	WriteAngle(float data, float dest, float desto)	= #405;
void	WriteCoord(float data, float dest, float desto)	= #406;
void	WriteString(string data, float dest, float desto)= #407;
void	WriteEntity(entity data, float dest, float desto) = #408;

//////////////////////////////////////////////////
// Draw funtions
//////////////////////////////////////////////////

float	iscachedpic(string name)	= #451;
string	precache_pic(string name, ...)	= #452;
void	freepic(string name)		= #453;

float	drawcharacter(vector position, float character, vector scale, vector rgb, float alpha, float flag) = #454;

float	drawstring(vector position, string text, vector scale, vector rgb, float alpha, float flag) = #455;

float	drawcolorcodedstring(vector position, string text, vector scale, float alpha, float flag) = #467;

vector	drawcolorcodedstring2(vector position, string text, vector scale, vector rgb, float alpha, float flag) = #467;
 
float	drawpic(vector position, string pic, vector size, vector rgb, float alpha, float flag) = #456;

float	drawfill(vector position, vector size, vector rgb, float alpha, float flag) = #457;

void	drawsetcliparea(float x, float y, float width, float height) = #458;

void	drawresetcliparea(void) = #459;

vector  drawgetimagesize(string pic) = #460;

////////////////////////////////////////////////
// Menu functions
////////////////////////////////////////////////

void	setkeydest(float dest) 	= #601;
float	getkeydest(void)	= #602;

void	setmousetarget(float trg) = #603;
float	getmousetarget(void)	  = #604;

float	isfunction(string function_name) = #607;
void	callfunction(...) = #605;
void	writetofile(float fhandle, entity ent) = #606;
vector	getresolution(float number) = #608;
string	keynumtostring(float keynum) = #609;

float	gethostcachevalue(float type) = #611;
string	gethostcachestring(float type, float hostnr) = #612;

//DP_CSQC_BINDMAPS
//idea: daemon, motorsep
//darkplaces implementation: divVerent
//builtin definitions:
string(float key, float bindmap) getkeybind_bindmap = #342;
float(float key, string bind, float bindmap) setkeybind_bindmap = #630;
vector(void) getbindmaps = #631;
float(vector bm) setbindmaps = #632;
string(string command, float bindmap) findkeysforcommand = #610;
float(string key) stringtokeynum = #341;
//<also allowed builtin number to match EXT_CSQC> string(float keynum) keynumtostring = #340;
//description: key bind setting/getting including support for switchable
//bindmaps.

//DP_CRYPTO
//idea: divVerent
//darkplaces implementation: divVerent
//field definitions: (MENUQC)
string(string serveraddress) crypto_getkeyfp = #633; // retrieves the cached host key's CA fingerprint of a server given by IP address
string(string serveraddress) crypto_getidfp = #634; // retrieves the cached host key fingerprint of a server given by IP address
float(string serveraddress) crypto_getidstatus = #643; // retrieves the cached host key's key status. See below for CRYPTO_IDSTATUS_ defines.
string(string serveraddress) crypto_getencryptlevel = #635; // 0 if never encrypting, 1 supported, 2 requested, 3 required, appended by list of allowed methods in order of preference ("AES128"), preceded by a space each
string(float i) crypto_getmykeyfp = #636; // retrieves the CA key fingerprint of a given CA slot, or "" if slot is unused but more to come, or string_null if end of list
string(float i) crypto_getmyidfp = #637; // retrieves the ID fingerprint of a given CA slot, or "" if slot is unused but more to come, or string_null if end of list
float CRYPTO_IDSTATUS_OUTOFRANGE = -1;
float CRYPTO_IDSTATUS_EMPTY = 0;
float CRYPTO_IDSTATUS_UNSIGNED = 1;
float CRYPTO_IDSTATUS_SIGNED = 2;
float(float i) crypto_getmyidstatus = #641; // retrieves the ID's status of a given CA slot, or 0 if slot is unused but more to come, or -1 if end of list
float(string url, float id, string content_type, string delim, float buf, float keyid) crypto_uri_postbuf = #513;
//description:
//use -1 as buffer handle to justs end delim as postdata

//DP_GECKO_SUPPORT
//idea: Res2k, BlackHC
//darkplaces implementation: Res2k, BlackHC
//constant definitions:
float GECKO_BUTTON_DOWN         = 0;
float GECKO_BUTTON_UP           = 1;
// either use down and up or just press but not all of them!
float GECKO_BUTTON_PRESS        = 2;
// use this for mouse events if needed?
float GECKO_BUTTON_DOUBLECLICK  = 3;
//builtin definitions:
float gecko_create( string name ) = #487;
void gecko_destroy( string name ) = #488;
void gecko_navigate( string name, string URI ) = #489;
float gecko_keyevent( string name, float key, float eventtype ) = #490;
void gecko_mousemove( string name, float x, float y ) = #491;
void gecko_resize( string name, float w, float h ) = #492;
vector gecko_get_texture_extent( string name ) = #493;
//engine-called QC prototypes:
//string(string name, string query) Qecko_Query;
//description:
//provides an interface to the offscreengecko library and allows for internet browsing in games

//FTE_STRINGS
//idea: many
//darkplaces implementation: KrimZon
//description:
//various string manipulation functions
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

//DP_PRECACHE_PIC_FLAGS
//idea: divVerent
//darkplaces implementation: divVerent
//constant definitions:
float PRECACHE_PIC_FROMWAD = 1; // this one actually is part of EXT_CSQC
float PRECACHE_PIC_NOTPERSISTENT = 2; // picture may get deallocated when unused
float PRECACHE_PIC_MIPMAP = 8; // mipmap the texture for possibly better downscaling at memory expense
//notes: these constants are given as optional second argument to precache_pic()

//DP_QC_CRC16
//idea: div0
//darkplaces implementation: div0
//Some hash function to build hash tables with. This has to be be the CRC-16-CCITT that is also required for the QuakeWorld download protocol.
//When caseinsensitive is set, the CRC is calculated of the lower cased string.
float(float caseinsensitive, string s, ...) crc16 = #494;

//DP_QC_CVAR_TYPE
float(string name) cvar_type = #495;
float CVAR_TYPEFLAG_EXISTS = 1;
float CVAR_TYPEFLAG_SAVED = 2;
float CVAR_TYPEFLAG_PRIVATE = 4;
float CVAR_TYPEFLAG_ENGINE = 8;
float CVAR_TYPEFLAG_HASDESCRIPTION = 16;
float CVAR_TYPEFLAG_READONLY = 32;

//DP_QC_STRINGBUFFERS
//idea: ??
//darkplaces implementation: LadyHavoc
//functions to manage string buffer objects - that is, arbitrary length string arrays that are handled by the engine
float() buf_create = #440;
void(float bufhandle) buf_del = #441;
float(float bufhandle) buf_getsize = #442;
void(float bufhandle_from, float bufhandle_to) buf_copy = #443;
void(float bufhandle, float sortpower, float backward) buf_sort = #444;
string(float bufhandle, string glue) buf_implode = #445;
string(float bufhandle, float string_index) bufstr_get = #446;
void(float bufhandle, float string_index, string str) bufstr_set = #447;
float(float bufhandle, string str, float order) bufstr_add = #448;
void(float bufhandle, float string_index) bufstr_free = #449;
void(float bufhandle, string pattern, string antipattern) buf_cvarlist = #517;

//DP_QC_STRING_CASE_FUNCTIONS
//idea: Dresk
//darkplaces implementation: LadyHavoc / Dresk
//builtin definitions:
string(string s) strtolower = #480; // returns the passed in string in pure lowercase form
string(string s) strtoupper = #481; // returns the passed in string in pure uppercase form
//description:
//provides simple string uppercase and lowercase functions

//DP_QC_CVAR_DESCRIPTION
//idea: divVerent
//DarkPlaces implementation: divVerent
//builtin definitions:
string(string name) cvar_description = #518;
//description:
//returns the description of a cvar

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
//idea: div0
//darkplaces implementation: div0
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

//DP_COVERAGE
//idea: divVerent
//darkplaces implementation: divVerent
//function definitions:
void coverage() = #642;  // Reports a coverage event. The engine counts for each of the calls to this builtin whether it has been called.

//DP_QC_FS_SEARCH_PACKFILE
//idea: Mario
//darkplaces implementation: Mario
//builtin definitions:
float(string pattern, float caseinsensitive, float quiet, string packfile) search_packfile_begin = #74;
//description:
//extension to search_begin (DP_QC_FS_SEARCH), performs a filename search with the specified pattern (for example "maps/*.bsp") and stores the results in a search slot (minimum of 128 supported by any engine with this extension), the other functions take this returned search slot number, be sure to search_free when done (they are also freed on progs reload).
//only searches for files within the specified packfile, which is expected to match the results of whichpack().

// assorted undocumented extensions
string(string, float) netaddress_resolve = #625;
string(string search, string replace, string subject) strreplace = #484;
string(string search, string replace, string subject) strireplace = #485;
string(float uselocaltime, string format, ...) strftime = #478;
float(string s) tokenize_console = #514;
float(float i) argv_start_index = #515;
float(float i) argv_end_index = #516;
string(float, float) getgamedirinfo = #626;
#define GETGAMEDIRINFO_NAME 0
#define GETGAMEDIRINFO_DESCRIPTION 1
float log(float f) = #532;
string(string format, ...) sprintf = #627;
string(string s) strdecolorize = #477;
entity	findflags(entity start, .float field, float match) = #87;
entity	findchainflags(.float field, float match) = #88;
float(string s, string separator1, ...) tokenizebyseparator = #479;
float 	etof(entity ent) = #79;
entity 	ftoe(float num)	 = #80;
float   validstring(string str) = #81;
float 	altstr_count(string str) = #82;
string  altstr_prepare(string str) = #83;
string  altstr_get(string str, float num) = #84;
string  altstr_set(string str, float num, string set) = #85;
string 	altstr_ins(string str, float num, string set) = #86;
float	isdemo() = #349;
float	drawsubpic(vector position, vector size, string pic, vector srcPosition, vector srcSize, vector rgb, float alpha, float flag) = #469;
//vector	getresolution(float number, ...) = #608; // optional argument "isfullscreen"
void 	parseentitydata(entity ent, string data) = #613;
void 	resethostcachemasks(void) = #615;
void 	sethostcachemaskstring(float mask, float fld, string str, float op) = #616;
void	sethostcachemasknumber(float mask, float fld, float num, float op) = #617;
void 	resorthostcache(void) = #618;
float SLSF_DESCENDING = 1;
float SLSF_FAVORITES = 2;
float SLSF_CATEGORIES = 4;
void	sethostcachesort(float fld, float slsf) = #619;
void	refreshhostcache(...) = #620;  // optional boolean argument "clear_list"
float	gethostcachenumber(float fld, float hostnr) = #621;
float	gethostcacheindexforkey(string key) = #622;
void	addwantedhostcachekey(string key) = #623;
string	getextresponse(void) = #624;
const string cvar_string(string name) = #71;
const string cvar_defstring(string name) = #89;
float	stringwidth(string text, float handleColors, vector size) = #468;
