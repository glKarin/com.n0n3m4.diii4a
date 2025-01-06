// Copyright (C) 1999-2000 Id Software, Inc.


#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "ui_public.h"
#undef UI_API_VERSION
#define UI_API_VERSION	4
#include "../client/keycodes.h"
#include "../game/bg_public.h"

typedef void (*voidfunc_f)(void);

// QSandbox version string
#define QS_VERSION "SourceTech"

// An additional font size tag for use with
// proportional strings. The original values are
// in q_shared.h, but a UI only mod shouldn't really
// be touching that file
#define UI_MEDIUMFONT 0x000080
#define PROP_MEDIUM_SIZE_SCALE	0.87


// model screen position
#define PLAYERMODEL_X 360
#define PLAYERMODEL_Y  -40-25
#define PLAYERMODEL_WIDTH 32*10
#define PLAYERMODEL_HEIGHT 56*10
#define PLAYERMODEL_TEXTHEIGHT  370-25

// status bar text buffer
#define MAX_STATUSBAR_TEXT 120
#define STATUSBAR_FADETIME 1500

// map lists
#define MAX_MAPS_LIST 1024


// logo art, all are 128x32 but they view very well at 64x16
// define NO_UIE_MINILOGO to remove 
#define UIE_LOGO_POWERED "menu/uie_art/uie_powered"
#define UIE_LOGO_ASSISTED "menu/uie_art/uie_assisted"
#define UIE_LOGO_INCLUDE "menu/uie_art/uie_include"
#define UIE_LOGO_IMPROVED "menu/uie_art/uie_improved"
#define UIE_LOGO_USING "menu/uie_art/uie_using"
#define UIE_LOGO_NAME "menu/dmod_logo"
#define AU_LOGO_NAME "menu/default/unlimitedlogo"

#define UIE_LOGO_X 570
#define UIE_LOGO_Y 400


//
// ui_main.c
//

extern	vmCvar_t 	cl_propsmallsizescale;
extern	vmCvar_t 	cl_propheight;
extern	vmCvar_t 	cl_propspacewidth;
extern	vmCvar_t 	cl_propgapwidth;
extern	vmCvar_t 	cl_smallcharwidth;
extern	vmCvar_t 	cl_smallcharheight;
extern	vmCvar_t 	cl_bigcharwidth;
extern	vmCvar_t 	cl_bigcharheight;
extern	vmCvar_t 	cl_giantcharwidth;
extern	vmCvar_t 	cl_giantcharheight;

//QSandbox Sandbox
extern vmCvar_t	sb_private;
extern vmCvar_t	sb_texture;
extern vmCvar_t	sb_texturename;
extern vmCvar_t	sb_grid;
extern vmCvar_t	sb_modelnum;
extern vmCvar_t	sb_classnum;
extern vmCvar_t	sb_texturenum;
extern vmCvar_t	sb_tab;
extern vmCvar_t	spawn_preset;
extern vmCvar_t	tool_spawnpreset;
extern vmCvar_t	tool_modifypreset;
extern vmCvar_t	tool_modifypreset2;
extern vmCvar_t	tool_modifypreset3;
extern vmCvar_t	tool_modifypreset4;

extern vmCvar_t	sb_ctab_1;
extern vmCvar_t	sb_ctab_2;
extern vmCvar_t	sb_ctab_3;
extern vmCvar_t	sb_ctab_4;
extern vmCvar_t	sb_ctab_5;
extern vmCvar_t	sb_ctab_6;
extern vmCvar_t	sb_ctab_7;
extern vmCvar_t	sb_ctab_8;
extern vmCvar_t	sb_ctab_9;
extern vmCvar_t	sb_ctab_10;

extern vmCvar_t	toolgun_toolset1;
extern vmCvar_t	toolgun_toolset2;
extern vmCvar_t	toolgun_toolset3;
extern vmCvar_t	toolgun_toolset4;
extern vmCvar_t	toolgun_toolset5;
extern vmCvar_t	toolgun_toolset6;
extern vmCvar_t	toolgun_toolset7;
extern vmCvar_t	toolgun_toolset8;
extern vmCvar_t	toolgun_toolset9;
extern vmCvar_t	toolgun_toolset10;
extern vmCvar_t	toolgun_toolset11;
extern vmCvar_t	toolgun_toolset12;
extern vmCvar_t	toolgun_toolset13;
extern vmCvar_t	toolgun_toolset14;
extern vmCvar_t	toolgun_toolset15;
extern vmCvar_t	toolgun_toolset16;
extern vmCvar_t	toolgun_toolset17;
extern vmCvar_t	toolgun_toolset18;

extern vmCvar_t	toolgun_disabledarg1;
extern vmCvar_t	toolgun_disabledarg2;
extern vmCvar_t	toolgun_disabledarg3;
extern vmCvar_t	toolgun_disabledarg4;

extern vmCvar_t	cl_sprun;

extern vmCvar_t	sbt_color0_0;
extern vmCvar_t	sbt_color0_1;
extern vmCvar_t	sbt_color0_2;
extern vmCvar_t	sbt_color0_3;
extern vmCvar_t	sbt_color1_0;
extern vmCvar_t	sbt_color1_1;
extern vmCvar_t	sbt_color1_2;
extern vmCvar_t	sbt_color1_3;
extern vmCvar_t	sbt_color2_0;
extern vmCvar_t	sbt_color2_1;
extern vmCvar_t	sbt_color2_2;
extern vmCvar_t	sbt_color2_3;
extern vmCvar_t	sbt_color3_0;
extern vmCvar_t	sbt_color3_1;
extern vmCvar_t	sbt_color3_2;
extern vmCvar_t	sbt_color3_3;
extern vmCvar_t	sbt_wallpaper;

extern vmCvar_t	ui_3dmap;

extern vmCvar_t	ui_singlemode;
extern vmCvar_t	legsskin;
extern vmCvar_t	sensitivitymenu;
extern vmCvar_t	team_legsskin;
extern vmCvar_t	cl_selectedmod;
extern vmCvar_t	cl_language;
extern vmCvar_t	ui_loaded;
extern vmCvar_t	cl_screenoffset;
extern vmCvar_t	ui_backcolors;
extern vmCvar_t	ui_ffa_fraglimit;
extern vmCvar_t	ui_ffa_timelimit;

extern vmCvar_t	ui_tourney_fraglimit;
extern vmCvar_t	ui_tourney_timelimit;

extern vmCvar_t	ui_team_fraglimit;
extern vmCvar_t	ui_team_timelimit;
extern vmCvar_t	ui_team_friendly;

extern vmCvar_t	ui_ctf_capturelimit;
extern vmCvar_t	ui_ctf_timelimit;
extern vmCvar_t	ui_ctf_friendly;

extern vmCvar_t	ui_arenasFile;
extern vmCvar_t	ui_botsFile;
extern vmCvar_t	ui_spScores1;
extern vmCvar_t	ui_spScores2;
extern vmCvar_t	ui_spScores3;
extern vmCvar_t	ui_spScores4;
extern vmCvar_t	ui_spScores5;
extern vmCvar_t	ui_spAwards;
extern vmCvar_t	ui_spVideos;
extern vmCvar_t	ui_spSkill;

extern vmCvar_t	ui_spSelection;

extern vmCvar_t	ui_browserMaster;
extern vmCvar_t	ui_browserGameType;
extern vmCvar_t	ui_browserSortKey;
extern vmCvar_t	ui_browserShowFull;
extern vmCvar_t	ui_browserShowEmpty;

extern vmCvar_t	ui_brassTime;
extern vmCvar_t	ui_drawCrosshair;
extern vmCvar_t	ui_drawCrosshairNames;
extern vmCvar_t	ui_marks;

extern vmCvar_t	ui_server1;
extern vmCvar_t	ui_server2;
extern vmCvar_t	ui_server3;
extern vmCvar_t	ui_server4;
extern vmCvar_t	ui_server5;
extern vmCvar_t	ui_server6;
extern vmCvar_t	ui_server7;
extern vmCvar_t	ui_server8;
extern vmCvar_t	ui_server9;
extern vmCvar_t	ui_server10;
extern vmCvar_t	ui_server11;
extern vmCvar_t	ui_server12;
extern vmCvar_t	ui_server13;
extern vmCvar_t	ui_server14;
extern vmCvar_t	ui_server15;
extern vmCvar_t	ui_server16;
extern vmCvar_t	ui_server17;
extern vmCvar_t	ui_server18;
extern vmCvar_t	ui_server19;
extern vmCvar_t	ui_server20;
extern vmCvar_t	ui_server21;
extern vmCvar_t	ui_server22;
extern vmCvar_t	ui_server23;
extern vmCvar_t	ui_server24;
extern vmCvar_t	ui_server25;
extern vmCvar_t	ui_server26;
extern vmCvar_t	ui_server27;
extern vmCvar_t	ui_server28;
extern vmCvar_t	ui_server29;
extern vmCvar_t	ui_server30;
extern vmCvar_t	ui_server31;
extern vmCvar_t	ui_server32;

extern vmCvar_t	uie_animsfx;
extern vmCvar_t	uie_mapicons;
extern vmCvar_t	uie_autoclosebotmenu;
extern vmCvar_t	uie_ingame_dynamicmenu;

qboolean UI_IsValidCvar(const char* cvar);

//
// ui_qmenu.c
//

#define RCOLUMN_OFFSET			( BIGCHAR_WIDTH )
#define LCOLUMN_OFFSET			(-BIGCHAR_WIDTH )

#define SLIDER_RANGE			10
#define	MAX_EDIT_LINE			256

#define MAX_MENUDEPTH			8
#define MAX_MENUITEMS			256

#define MTYPE_NULL				0
#define MTYPE_SLIDER			1	
#define MTYPE_ACTION			2
#define MTYPE_SPINCONTROL		3
#define MTYPE_FIELD				4
#define MTYPE_RADIOBUTTON		5
#define MTYPE_BITMAP			6	
#define MTYPE_TEXT				7
#define MTYPE_SCROLLLIST		8
#define MTYPE_PTEXT				9
#define MTYPE_BTEXT				10
#define MTYPE_UIOBJECT			11	

#define QMF_BLINK				0x00000001
#define QMF_SMALLFONT			0x00000002
#define QMF_LEFT_JUSTIFY		0x00000004
#define QMF_CENTER_JUSTIFY		0x00000008
#define QMF_RIGHT_JUSTIFY		0x00000010
#define QMF_NUMBERSONLY			0x00000020	// edit field is only numbers
#define QMF_HIGHLIGHT			0x00000040
#define QMF_HIGHLIGHT_IF_FOCUS	0x00000080	// steady focus
#define QMF_PULSEIFFOCUS		0x00000100	// pulse if focus
#define QMF_HASMOUSEFOCUS		0x00000200
#define QMF_NOONOFFTEXT			0x00000400
#define QMF_MOUSEONLY			0x00000800	// only mouse input allowed
#define QMF_HIDDEN				0x00001000	// skips drawing
#define QMF_GRAYED				0x00002000	// grays and disables
#define QMF_INACTIVE			0x00004000	// disables any input
#define QMF_NODEFAULTINIT		0x00008000	// skip default initialization
#define QMF_OWNERDRAW			0x00010000
#define QMF_PULSE				0x00020000
#define QMF_LOWERCASE			0x00040000	// edit field is all lower case
#define QMF_UPPERCASE			0x00080000	// edit field is all upper case
#define QMF_SILENT				0x00100000

// callback notifications
#define QM_GOTFOCUS				1
#define QM_LOSTFOCUS			2
#define QM_ACTIVATED			3

// control event handler
typedef void (*callbackFunc)(void* self, int event);


typedef struct _tag_menuframework
{
	int	cursor;
	int cursor_prev;

	int	nitems;
	void *items[MAX_MENUITEMS];

	void (*draw) (void);
	sfxHandle_t (*key) (int key);

	qboolean	wrapAround;
	qboolean	fulllock;
	qboolean	fullscreen;
	qboolean	native;
	int			uplimitscroll;
	int			downlimitscroll;
	int			number;
	qboolean	showlogo;
} menuframework_s;

typedef struct
{
	int type;
	const char *name;
	char *text;
	char *picn;
	char *cmd;
	int	id;
	int x, y;
	int xoffset, yoffset;
	int left;
	int	top;
	int	right;
	int	bottom;
	float heightmod;
	menuframework_s *parent;
	int menuPosition;
	unsigned flags;

	void (*callback)( void *self, int event );
	void (*statusbar)( void *self );
	void (*ownerdraw)( void *self );
} menucommon_s;

typedef struct {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
	int		maxchars;
} mfield_t;

typedef struct
{
	menucommon_s	generic;
	mfield_t		field;
	float* 			color;
} menufield_s;

typedef struct 
{
	menucommon_s generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_s;

typedef struct
{
	menucommon_s generic;

	int	oldvalue;
	int curvalue;
	int	numitems;
	int	top;
		
	const char **itemnames;
	const char **itemnames2;

	int width;
	int height;
	int	columns;
	int	seperation;
	float* color;
} menulist_s;

typedef struct
{
	menucommon_s generic;
} menuaction_s;

typedef struct
{
	menucommon_s generic;
	int curvalue;
	float* color;
} menuradiobutton_s;

typedef struct
{
	menucommon_s	generic;
	char*			focuspic;	
	char*			errorpic;
	qhandle_t		shader;
	qhandle_t		focusshader;
	int				width;
	int				height;
	float*			focuscolor;
} menubitmap_s;

typedef struct
{
	menucommon_s	generic;
	qhandle_t		shader;
	qhandle_t		model;
	int				type;
	int				mode;
	int				width;
	int				height;
	char 			*string;
	int				style;
	int				styles;
	float*			color;
	float*			color2;
	int				corner;
	float			fontsize;
	mfield_t		field;
	
	int	oldvalue;
	int curvalue;
	int	numitems;
	int	top;
	
	const char **itemnames;
	const char **itemnames2;

	int	columns;
	int	seperation;
	
	int minvalue;
	int maxvalue;

	float range;
} menuobject_s;

typedef struct
{
	menucommon_s	generic;
	char*			string;
	int				style;
	float			customsize;
	float*			color;
} menutext_s;

extern void			Menu_Cache( void );
extern void			Menu_Focus( menucommon_s *m );
extern void			Menu_AddItem( menuframework_s *menu, void *item );
extern void			Menu_AdjustCursor( menuframework_s *menu, int dir );
extern void			Menu_Draw( menuframework_s *menu );
extern void			*Menu_ItemAtCursor( menuframework_s *m );
extern sfxHandle_t	Menu_ActivateItem( menuframework_s *s, menucommon_s* item );
extern void			Menu_SetCursor( menuframework_s *s, int cursor );
extern void			Menu_SetCursorToItem( menuframework_s *m, void* ptr );
extern sfxHandle_t	Menu_DefaultKey( menuframework_s *s, int key );
extern void			Bitmap_Init( menubitmap_s *b );
extern void			Bitmap_Draw( menubitmap_s *b );
extern void			UIObject_Init( menuobject_s *b );
extern void			UIObject_Draw( menuobject_s *b );
extern void			ScrollList_Draw( menulist_s *l );
extern sfxHandle_t	ScrollList_Key( menulist_s *l, int key );
extern sfxHandle_t	menu_in_sound;
extern sfxHandle_t	menu_move_sound;
extern sfxHandle_t	menu_out_sound;
extern sfxHandle_t	menu_buzz_sound;
extern sfxHandle_t	menu_null_sound;
extern sfxHandle_t	weaponChangeSound;
extern vec4_t		menu_text_color;
extern vec4_t		menu_grayed_color;
extern vec4_t		menu_dark_color;
extern vec4_t		menu_highlight_color;
extern vec4_t		menu_red_color;
extern vec4_t		menu_black_color;
extern vec4_t		menu_dim_color;

extern vec4_t		color_black;
extern vec4_t		color_white;
extern vec4_t		color_yellow;
extern vec4_t		color_blue;
extern vec4_t		color_grey;
extern vec4_t		color_red;
extern vec4_t		color_dim;
extern vec4_t		color_dim80;
extern vec4_t		color_green;
extern vec4_t		color_emerald;
extern vec4_t		color_bluo;
extern vec4_t		color_lightyellow;
extern vec4_t		color_highlight;

extern vec4_t		s_sandboxmain_color1;

extern vec4_t		name_color;
extern vec4_t		list_color;
extern vec4_t		pulse_color;
extern vec4_t		text_color_disabled;
extern vec4_t		text_color_normal;
extern vec4_t 		color_select_bluo;

extern void PText_Init( menutext_s *b );
extern void	ScrollList_Init( menulist_s *l );
extern void	RadioButton_Init( menuradiobutton_s *rb );
extern void	SpinControl_Init( menulist_s *s );

extern char	*ui_medalNames[];
extern char	*ui_medalPicNames[];
extern char	*ui_medalSounds[];

//
// ui_advanced.c
//
extern void UI_AdvancedMenu( void );
extern void UI_AdvancedMenu_Cache( void );

//
// ui_mfield.c
//
extern void			MField_Clear( mfield_t *edit );
extern void			MField_KeyDownEvent( mfield_t *edit, int key );
extern void			MField_CharEvent( mfield_t *edit, int ch );
extern void			MField_Draw( mfield_t *edit, int x, int y, int style, vec4_t color );
extern void			MField_DrawCustom( mfield_t *edit, int x, int y, int style, vec4_t color, float csize );
extern void			MenuField_Init( menufield_s* m );
extern void			MenuField_Draw( menufield_s *f );
extern sfxHandle_t	MenuField_Key( menufield_s* m, int* key );

//
// ui_credits.c
//
extern void UI_CreditMenu( int num );

//
// ui_ingame.c
//
extern int UI_CurrentPlayerTeam( void );
extern void InGame_Cache( void );
extern void UI_InGameMenu(void);
extern void UI_DynamicMenuCache(void);
extern void UI_BotCommandMenu_f( void );

//
// ui_confirm.c
//
extern void ConfirmMenu_Cache( void );
extern void UI_ConfirmMenu( const char *question, void (*draw)( void ), void (*action)( qboolean result ) );
extern void UI_ConfirmMenu_Style( const char *question, int style, void (*draw)( void ), void (*action)( qboolean result ) );
extern void UI_Message( const char **lines );

//
// ui_sandbox.c
//
extern void UI_SandboxMainMenu(void);

//
// ui_setup.c
//
extern void UI_SetupMenu_Cache( void );
extern void UI_SetupMenu(void);

//
// ui_team.c
//
extern void UI_TeamMainMenu( void );
extern void TeamMain_Cache( void );

//
// ui_connect.c
//
extern void UI_DrawConnectScreen( qboolean overlay );

//
// ui_controls2.c
//
extern void UI_ControlsMenu( void );
extern void Controls_Cache( void );

//
// ui_demo2.c
//
extern void UI_DemosMenu( void );
extern void Demos_Cache( void );

//
// ui_cinematics.c
//
extern void UI_CinematicsMenu( int num );
extern void UI_CinematicsMenu_f( int num );
extern void UI_CinematicsMenu_Cache( void );

//
// ui_mods.c
//
extern void UI_ModsMenu( void );
extern void UI_ModsMenu_Cache( void );

//
// ui_workshop.c
//
extern void UI_WorkshopMenu( void );
extern void UI_WorkshopMenu_Cache( void );

//
// ui_playermodel.c
//
#define LOW_MEMORY			(5 * 1024 * 1024)

extern void UI_PlayerModelMenu( void );
extern void PlayerModel_Cache( void );

//
// ui_playersettings.c
//
extern void UI_PlayerSettingsMenu( void );
extern void PlayerSettings_Cache( void );

//
// ui_preferences.c
//
extern void UI_PreferencesMenu( void );
extern void Preferences_Cache( void );

//
// ui_specifyserver.c
//
extern void UI_SpecifyServerMenu( void );
extern void SpecifyServer_Cache( void );

//
// ui_servers2.c
//
#define MAX_FAVORITESERVERS 32

extern void UI_ArenaServersMenu( void );
extern void ArenaServers_Cache( void );

//
// ui_startserver.c group of files
//
extern void UI_StartServerMenu( qboolean multiplayer );
extern void StartServer_Cache( void );
extern void UI_BotSelect( char *bot );
extern void UI_BotSelect_Cache( void );
extern int Clamp_Random(int range);
extern void UI_ServerPlayerIcon( const char *modelAndSkin, char *iconName, int iconNameMaxSize );
extern void UIE_InGame_EnabledItems(void);
extern void UIE_StartServer_RegisterDisableCvars(qboolean init);
extern const char* UIE_DefaultIconFromGameType(int gametype);

//
// ui_serverinfo.c
//
extern void UI_ServerInfoMenu( void );
extern void ServerInfo_Cache( void );

//
// ui_video.c
//
extern void UI_GraphicsOptionsMenu( void );
extern void GraphicsOptions_Cache( void );
extern void DriverInfo_Cache( void );

//
// ui_players.c
//

//FIXME ripped from cg_local.h
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct {
	// model info
	qhandle_t		legsModel;
	qhandle_t		legsSkin;
	qhandle_t		legsShader;
	lerpFrame_t		legs;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	qhandle_t		torsoShader;
	lerpFrame_t		torso;

	qhandle_t		headModel;
	qhandle_t		headSkin;
	qhandle_t		headShader;

	animation_t		animations[MAX_ANIMATIONS];

	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;
	vec3_t			flashDlightColor;
	int				muzzleFlashTime;
	int				humTimer;
	int				delayFireTimer;

	// currently in use drawing parms
	vec3_t			viewAngles;
	vec3_t			moveAngles;
	weapon_t		currentWeapon;
	int				legsAnim;
	int				torsoAnim;

	// animation vars
	weapon_t		weapon;
	weapon_t		lastWeapon;
	weapon_t		pendingWeapon;
	int				weaponTimer;
	int				pendingLegsAnim;
	int				torsoAnimationTimer;

	int				pendingTorsoAnim;
	int				legsAnimationTimer;

	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	qboolean		chat;
	qboolean		newModel;

	qboolean		barrelSpinning;
	float			barrelAngle;
	int				barrelTime;

	int				realWeapon;

	int 			oldFrame;
} playerInfo_t;


#define ANIM_IDLE		1
#define ANIM_RUN		2
#define ANIM_WALK		3
#define ANIM_BACK		4
#define ANIM_JUMP		5
#define ANIM_CROUCH		6
#define ANIM_STEPLEFT	7
#define ANIM_STEPRIGHT	8
#define ANIM_TURNLEFT	9
#define ANIM_TURNRIGHT	10
#define ANIM_LOOKUP		11
#define ANIM_LOOKDOWN	12
#define ANIM_WEAPON1	13
#define ANIM_WEAPON2	14
#define ANIM_WEAPON3	15
#define ANIM_WEAPON4	16
#define ANIM_WEAPON5	17
#define ANIM_WEAPON6	18
#define ANIM_WEAPON7	19
#define ANIM_WEAPON8	20
#define ANIM_WEAPON9	21
#define ANIM_WEAPON10	22
#define ANIM_WEAPON11	23
#define ANIM_WEAPON12	24
#define ANIM_WEAPON13	25
#define ANIM_WEAPON14	26
#define ANIM_WEAPON15	27
#define ANIM_ATTACK		28
#define ANIM_GESTURE	29
#define ANIM_DIE		30
#define ANIM_DIE2		31
#define ANIM_DIE3		32
#define ANIM_SWIM		33

// ANIM_CHAT should always be the last animation value
#define ANIM_CHAT		34

#define ANIM_MAX (ANIM_CHAT + 1)

#define MAX_RECENT_ANIMS 10
#define MODELNAME_BUFFER MAX_QPATH



typedef struct {
	menubitmap_s stop;
	menubitmap_s pause;
	menubitmap_s left;
	menubitmap_s right;

	float yaw;
	int rotate;
	qboolean paused;

	qboolean useSpin;
} modelRotate_t;



typedef struct {
	playerInfo_t player;
	char modelskin[MODELNAME_BUFFER];
	char headskin[MODELNAME_BUFFER];	// head model for 1.27
	char legsskin[MODELNAME_BUFFER];	// head model for 1.27
	char team_modelskin[MODELNAME_BUFFER];	// team model
	char team_headskin[MODELNAME_BUFFER];	// team head for 1.27
	char team_legsskin[MODELNAME_BUFFER];	// head model for 1.27
	
	menubitmap_s bitmap;

	modelRotate_t spin;

	int anim;
	int playerLegs;
	int playerTorso;
	int playerWeapon;
	qboolean playerChat;
	vec3_t viewangles;
	vec3_t moveangles;

	int cursorx;
	int cursory;
	int NextIdleAnimTime;
	qboolean bDoingIdleAnim;
	int recent_anims[MAX_RECENT_ANIMS];
	int recent_anims_index;
	int current_weapon;
	int shotsRemaining;
	int nextFireTime;

	qboolean bUnknownModel;
	qboolean bUnknownHeadModel;
	qboolean bUnknownLegsModel;
	qboolean bNoIdleAnim;
	qboolean bNoAutoUpdate;
	qboolean bForceUpdate;
	qboolean allowCursorFire;
} modelAnim_t;


// these enums are used in several places
// and match the items on drawmodel_list[]
enum {
	DRAWMODEL_DM,
	DRAWMODEL_TEAM
};


typedef void (*callbackOwnerDraw)(void* self);

// finally: function declarations
//extern void UI_DrawPlayer( float x, float y, float w, float h, playerInfo_t *pi, int time );
//extern void UI_PlayerInfo_SetInfo( playerInfo_t *pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNum, qboolean chat );

extern void UIE_PlayerInfo_ChangeTimedAnimation( modelAnim_t* m, int anim);
extern void UIE_PlayerInfo_ChangeAnimation( modelAnim_t* m, int anim);
extern void UIE_PlayerInfo_InitModel(modelAnim_t* m);
extern void UIE_PlayerInfo_AnimateModel( modelAnim_t* m);

extern const char* UIE_ModelName( const char* modelname );
extern const char* UIE_ModelSkin( const char* modelname );

extern void UIE_PlayerInfo_DrawTeamModel( modelAnim_t* m, qboolean teamModel );
extern qboolean UIE_PlayerInfo_IsTeamModel( void );

// spin/rotate controls
extern void PlayerInfo_ModelSpinEvent( modelRotate_t* rm, int id);
extern void UIE_PlayerInfo_InitRotateModel(menuframework_s* mf, modelRotate_t* rm, callbackFunc event_handler, int x, int y);

//
// ui_atoms.c
//
typedef struct {
	int					frametime;
	int					realtime;
	int					cursorx;
	int					cursory;
	qboolean 			hideCursor;
	int					menusp;
	menuframework_s*	activemenu;
	menuframework_s*	stack[MAX_MENUDEPTH];
	glconfig_t			glconfig;
	qboolean			debug;
	qhandle_t			whiteShader;
	qhandle_t			menuBlack;
	qhandle_t			menuWallpapers;
	qhandle_t			menuLoadingIcon;
	qhandle_t			charset[3];
	qhandle_t			cursor;
	qhandle_t			corner;
	qhandle_t			rb_on;
	qhandle_t			rb_off;
	float				scale;
	float				xscale;
	float				yscale;
	float				bias;
	float				sw;
	float				sh;
	int					menuscroll;
	int					sb_tab;
	int					wideoffset;
	char				theme;
	int					spawnlist_folder;
	int					texturelist_folder;
	qboolean			demoversion;
	qboolean			firstdraw;
	qboolean			onmap;
} uiStatic_t;

extern void			UI_Init( void );
extern void			UI_Shutdown( void );
extern void			UI_KeyEvent( int key, int down );
extern void			UI_MouseEvent( int dx, int dy );
extern void			UI_Refresh( int realtime );
extern qboolean		UI_ConsoleCommand( int realTime );
extern float		UI_ClampCvar( float min, float max, float value );
extern void			UI_DrawNamedPic( float x, float y, float width, float height, const char *picname );
extern void			UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader );
extern void 		UI_DrawRoundedRect(float x, float y, float width, float height, float radius, const float *color);
extern void			UI_FillRect( float x, float y, float width, float height, const float *color );
extern void			UI_DrawRect( float x, float y, float width, float height, const float *color );
extern void			UI_UpdateScreen( void );
extern void			UI_SetColor( const float *rgba );
extern void			UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
extern float		UI_ProportionalSizeScale( int style, float customsize );
extern int			UI_ProportionalStringWidth( const char* str );
extern void			UI_DrawString( int x, int y, const char* str, int style, vec4_t color );
extern void			UI_DrawStringCustom( int x, int y, const char* str, int style, vec4_t color, float csize, float width );
extern void			UI_DrawChar( int x, int y, int ch, int style, vec4_t color );
extern void			UI_DrawCharCustom( int x, int y, int ch, int style, vec4_t color, float csize );
extern qboolean 	UI_CursorInRect (int x, int y, int width, int height);
extern void			UI_AdjustFrom640( float *x, float *y, float *w, float *h );
extern qboolean		UI_IsFullscreen( void );
extern void			UI_SetActiveMenu( uiMenuCommand_t menu );
extern void			UI_PushMenu ( menuframework_s *menu );
extern void			UI_PopMenu (void);
extern void			UI_ForceMenuOff (void);
extern char			*UI_Argv( int arg );
extern char			*UI_Cvar_VariableString( const char *var_name );
extern void			UI_Refresh( int time );
extern void			UI_StartDemoLoop( void );
extern qboolean		m_entersound;
extern uiStatic_t	uis;

//
// ui_syscalls.c
//
void			trap_Print( const char *string );
void			trap_Error( const char *string );
int				trap_Milliseconds( void );
void			trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void			trap_Cvar_Update( vmCvar_t *vmCvar );
void			trap_Cvar_Set( const char *var_name, const char *value );
float			trap_Cvar_VariableValue( const char *var_name );
void			trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void			trap_Cvar_SetValue( const char *var_name, float value );
void			trap_Cvar_Reset( const char *name );
void			trap_Cvar_Create( const char *var_name, const char *var_value, int flags );
void			trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize );
int				trap_Argc( void );
void			trap_Argv( int n, char *buffer, int bufferLength );
void			trap_Cmd_ExecuteText( int exec_when, const char *text );	// don't use EXEC_NOW!
int				trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void			trap_FS_Read( void *buffer, int len, fileHandle_t f );
void			trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void			trap_FS_FCloseFile( fileHandle_t f );
int				trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
int				trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t
qhandle_t		trap_R_RegisterModel( const char *name );
qhandle_t		trap_R_RegisterSkin( const char *name );
qhandle_t		trap_R_RegisterShaderNoMip( const char *name );
void			trap_R_ClearScene( void );
void			trap_R_AddRefEntityToScene( const refEntity_t *re );
void			trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void			trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void			trap_R_RenderScene( const refdef_t *fd );
void			trap_R_SetColor( const float *rgba );
void			trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void			trap_UpdateScreen( void );
int				trap_CM_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName );
void			trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );
void			trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void			trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void			trap_Key_SetBinding( int keynum, const char *binding );
qboolean		trap_Key_IsDown( int keynum );
qboolean		trap_Key_GetOverstrikeMode( void );
void			trap_Key_SetOverstrikeMode( qboolean state );
void			trap_Key_ClearStates( void );
int				trap_Key_GetCatcher( void );
void			trap_Key_SetCatcher( int catcher );
void			trap_GetClipboardData( char *buf, int bufsize );
void			trap_GetClientState( uiClientState_t *state );
void			trap_GetGlconfig( glconfig_t *glconfig );
int				trap_GetConfigString( int index, char* buff, int buffsize );
int				trap_LAN_GetServerCount( int source );
void			trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void			trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int				trap_LAN_GetPingQueueCount( void );
int				trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
void			trap_LAN_ClearPing( int n );
void			trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime );
void			trap_LAN_GetPingInfo( int n, char *buf, int buflen );
int				trap_MemoryRemaining( void );
void			trap_GetCDKey( char *buf, int buflen );
void			trap_SetCDKey( char *buf );

qboolean    trap_VerifyCDKey( const char *key, const char *chksum); // bk001208 - RC4

int				trap_RealTime(qtime_t *qtime);

void			trap_SetPbClStatus( int status );

//
// ui_addbots.c
//
void UI_AddBots_Cache( void );
void UI_AddBotsMenu( void );

//
// ui_removebots.c
//
enum {
	RBM_KICKBOT,
	RBM_CALLVOTEKICK,
	RBM_CALLVOTELEADER
};

void UI_RemoveBots_Cache( void );
void UI_RemoveBotsMenu( int menutype );

//
// ui_teamorders.c
//
extern void UI_TeamOrdersMenu( void );
extern void UI_TeamOrdersMenu_f( void );
extern void UI_TeamOrdersMenu_Cache( void );

//
// ui_loadconfig.c
//
typedef qboolean (*configCallback)(const char* filename);

void LoadConfig_SetStatusText(const char* text);
void UI_LoadConfig_Cache( void );
void UI_LoadConfigMenu( void );
void UI_SaveConfigMenu( void );
void UI_ConfigMenu(const char* title, qboolean load, configCallback handler);

//
// ui_display.c
//
void UI_DisplayOptionsMenu_Cache( void );
void UI_DisplayOptionsMenu( void );

//
// ui_sound.c
//
void UI_SoundOptionsMenu_Cache( void );
void UI_SoundOptionsMenu( void );

//
// ui_network.c
//
void UI_NetworkOptionsMenu_Cache( void );
void UI_NetworkOptionsMenu( void );



//
// ui_menu.c
//

extern vec4_t		color_translucent;


extern void MainMenu_Cache( void );
extern void UI_MainMenu(void);
extern void MainMenu_ReloadGame(void);
extern void UI_RegisterCvars( void );
extern void UI_UpdateCvars( void );
extern void UI_SetDefaultCvar(const char* cvar, const char* value);

//
// ui_gameinfo.c
//
const char *UI_GetArenaInfoByNumber( int num );
const char *UI_GetArenaInfoByMap( const char *map );
const char *UI_GetSpecialArenaInfo( const char *tag );
int UI_GetNumArenas( void );

char *UI_GetBotInfoByNumber( int num );
char *UI_GetBotInfoByName( const char *name );
int UI_GetBotNumByName( const char *name );
int UI_GetNumBots( void );

void UI_InitGameinfo( void );

//
// ui_loadMAP.c
//
void UI_loadMapEd_Cache( void );
void UI_loadMapEdMenu( void );

//
// ui_saveMAP.c
//
void UI_saveMapEdMenu_Cache( void );
void UI_saveMapEdMenu( void );

//
// ui_ingame_mapvote.c
//

void UI_MapCallVote( void );


#endif
