
//csqc only builtins.

void() clearscene			= #300; // (EXT_CSQC)
void(float mask) addentities		= #301; //  (EXT_CSQC)
void(entity ent) addentity		= #302; //  (EXT_CSQC)
float(float property, ...) setviewprop	= #303; //  (EXT_CSQC)
void() renderscene			= #304; //  (EXT_CSQC)

void(vector org, float radius, vector lightcolours) adddynamiclight		= #305; //  (EXT_CSQC)

//void(string texturename) R_BeginPolygon	= #306; //  (EXT_CSQC_???)
//void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex	= #307; //  (EXT_CSQC_???)
//void() R_EndPolygon 			= #308; //  (EXT_CSQC_???)

//maths stuff that uses the current view settings.
vector (vector v) unproject 		= #310; //  (EXT_CSQC)
vector (vector v) project 		= #311; //  (EXT_CSQC)

//2d (immediate) operations
void(float width, vector pos1, vector pos2) drawline = #315; //  (EXT_CSQC)
float(string name) iscachedpic 			= #316; //  (EXT_CSQC)
string(string name, float trywad) precache_pic 	= #317; //  (EXT_CSQC)
vector(string picname) draw_getimagesize 	= #318; //  (EXT_CSQC)
void(string name) freepic 			= #319; //  (EXT_CSQC)
float(vector position, float character, vector scale, vector rgb, float alpha, ...) drawcharacter	= #320; //  (EXT_CSQC, [EXT_CSQC_???])
float(vector position, string text, vector scale, vector rgb, float alpha, ...) drawstring		= #321; //  (EXT_CSQC, [EXT_CSQC_???])
float(vector position, string pic, vector size, vector rgb, float alpha, ...) drawpic			= #322; //  (EXT_CSQC, [EXT_CSQC_???])
float(vector position, vector size, vector rgb, float alpha, ...) drawfill				= #323; //  (EXT_CSQC, [EXT_CSQC_???])
void(float x, float y, float width, float height) drawsetcliparea 					= #324; //  (EXT_CSQC_???)
void(void) drawresetcliparea 			= #325; //  (EXT_CSQC_???)

float(float stnum) getstatf							= #330; //  (EXT_CSQC)
float(float stnum) getstati							= #331; //  (EXT_CSQC)
float(float stnum, float first, float count) getstatbits			= #331; //  (EXT_CSQC)
string(float firststnum) getstats						= #332; //  (EXT_CSQC)
void(entity e, float mdlindex) setmodelindex					= #333; //  (EXT_CSQC)
string(float mdlindex) modelnameforindex 					= #334; //  (EXT_CSQC)

float(string effectname) particleeffectforname					= #335; //  (EXT_CSQC)
void(float effectnum, entity ent, vector start, vector end) trailparticles	= #336; //  (EXT_CSQC),
void(float effectnum, vector origin, ...) pointparticles			= #337; //  (EXT_CSQC)

void(string s, ...) cprint		= #338; //  (EXT_CSQC)
void(string s, ...) print		= #339; //  (EXT_CSQC)

string(float keynum) keynumtostring	= #340; //  (EXT_CSQC)
float(string keyname) stringtokeynum	= #341; //  (EXT_CSQC)
string(float keynum) getkeybind		= #342; //  (EXT_CSQC)

float(float framenum) getinputstate	= #345; //  (EXT_CSQC)
void(float sens) setsensitivityscaler	= #346; //  (EXT_CSQC)

void() runstandardplayerphysics		= #347; //  (EXT_CSQC)

string(float playernum, string keyname) getplayerkeyvalue			= #348; //  (EXT_CSQC)

float() isdemo				= #349; //  (EXT_CSQC)
float() isserver			= #350; //  (EXT_CSQC)

void(vector origin, vector forward, vector right, vector up) SetListener	= #351; //  (EXT_CSQC)
void(string cmdname) registercommand						= #352; //  (EXT_CSQC)
float(entity ent) wasfreed							= #353; //  (EXT_CSQC) (should be availabe on server too)

string(string keyname) getserverinfo			= #354; //  (EXT_CSQC)

//note that 'ReadEntity' is pretty hard to implement reliably. Modders should use a combination of ReadShort, and findfloat, and remember that it might not be known clientside (pvs culled or other reason)
float() readbyte			= #360; //  (EXT_CSQC)
float() readchar			= #361; //  (EXT_CSQC)
float() readshort			= #362; //  (EXT_CSQC)
float() readlong			= #363; //  (EXT_CSQC)
float() readcoord			= #364; //  (EXT_CSQC)
float() readangle			= #365; //  (EXT_CSQC)
string() readstring			= #366; //  (EXT_CSQC)
string() readfloat			= #367; //  (EXT_CSQC)