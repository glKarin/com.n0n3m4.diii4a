float chasecam;	//chasecam active
entity player_local;	//handle to the local player entity
float isdp;	//if we're running under DP
string levelname;

//q3playerm
.entity headent;
.entity torsoent;
.entity legsent;
.entity weaponent;
.float modelnum;
.float animnum;
.float framechangetime;

//player
.float lerptime;
.void() removefunc;
.vector lastorg;
.vector lastvel;
.float haddied;
.float modelstyle;
.string oldskin;

//the genders you can have.
//we're generous with three.
enum {
	GENDER_NEUTER,
	GENDER_MALE,
	GENDER_FEMALE
};

//
.float ideal_yaw;
.float yaw_speed;
.float tag_index;


.float sveffects;	//lower 4 is weapon, next is nopred, upper 3 is powerups
#define SVE_INVIS 128
#define SVE_GOD 64
#define SVE_QUAD 32
#define SVE_MOVETYPE 16
#define SVE_WEAPONSMASK 15

.float removetime;
.float gibbable;

//q3playerm
float(string newmodelskin) Anim_SetModel;
void() Anim_Draw;
void() Anim_UnsetModel;
entity() Anim_DupModel;
float(string fname) Anim_ReadAnimationFile;
float() Anim_GetGender;
float(string skinname) Anim_GetHeadModelIndex;
float(string skinname) Anim_GetHeadSkinNumber;
vector(string skinname) Anim_GetHeadOffset;

//prediction
void(entity ent) Pred_PlayerUpdated;
void(entity ent) Pred_UpdateLocalMovement;
vector vieworg;

//menu
void(void(float, float, float, float) fnc) Menu_Activate;
void() Menu_Main;
void() Menu_Think;
var void(float event, float button, float mousex, float mousey) MenuEventFunc;

void() Movetype_Bounce;

DEFCVAR_FLOAT(cg_editor, 0)
DEFCVAR_FLOAT(cg_hudtype, 2)
DEFCVAR_FLOAT(cg_noerror, 0)
DEFCVAR_FLOAT(cg_nostep, 0)
DEFCVAR_FLOAT(cg_nopred, 0)
DEFCVAR_FLOAT(cg_thirdPerson, 0)
DEFCVAR_FLOAT(cg_thirdPersonAngle, 0)
DEFCVAR_FLOAT(cg_thirdPersonRange, 96)
DEFCVAR_FLOAT(cg_thirdPersonHeight, 24)
DEFCVAR_FLOAT(cg_noselfsounds, 0)
DEFCVAR_FLOAT(cg_noselfjumpsound, 0)
DEFCVAR_FLOAT(v_viewheight, 0)
DEFCVAR_STRING(cg_forceskin, "")