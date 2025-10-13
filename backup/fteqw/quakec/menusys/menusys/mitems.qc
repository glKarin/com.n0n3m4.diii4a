#ifdef MENU
#define cltime time
#endif

#ifdef CSQC
#define csqconly
#else
#define csqconly __strip
#endif
#ifdef MENU
#define menuonly
#else
#define menuonly __strip
#endif



//#include "mitems.qh"

__strip var float dp_workarounds;

#define IF_SELECTABLE		(1<<0)	//can accept KFOCUSED/MFOCUSED and key events etc. cannot be selected otherwise.
#define IF_INTERACT		(1<<1)	//generic interaction flag for use by the widgets themselves.
#define IF_RESIZABLE		(1<<2)	//may be resized, either by parent (it takes the full space) or by user.
#define IF_MFOCUSED		(1<<3)	//mouse is currently sitting over it
#define IF_KFOCUSED		(1<<4)	//has keyboard focus
#define IF_NOKILL		(1<<5)	//kill button is disabled. move to frame/menu?
#define IF_DRAGGABLE		(1<<6)	//can be dragged. this is stupid.
#define IF_DROPPABLE		(1<<7)	//dragged items can be dropped on this item.
#define IF_CLIENTMOVED		(1<<8)	//recalc required client dimensions (and toggle scrollbars if needed). move to frame?
#define IF_CENTERALIGN		(1<<9)	//
#define IF_RIGHTALIGN		(1<<10)	//
#define IF_NOCURSOR		(1<<11)	//when mgrabbed, the cursor will not be shown
#define IF_ISFRAME		(1<<12)	//is derived from mitem_frame (helps debugging recurion).
#define IF_FOCUSFOLLOWSMOUSE 	(1<<13)	//on frames, child keyboard focus (mostly) follows the mouse cursor. not like windows, but handy on things with lots of buttons. annoying on the desktop. move to frame?
#define IF_INVISIBLE		(1<<14)

#define RS_X_MIN_PARENT_MIN	0x0000
#define RS_X_MIN_PARENT_MID	0x0001
#define RS_X_MIN_PARENT_MAX	0x0002
#define RS_X_FRACTION			0x0004

#define RS_X_MAX_OWN_MIN		0x0000
#define RS_X_MAX_PARENT_MIN	0x0010
#define RS_X_MAX_PARENT_MID	0x0020
#define RS_X_MAX_PARENT_MAX	0x0040

#define RS_Y_MIN_PARENT_MIN	0x0000
#define RS_Y_MIN_PARENT_MID	0x0100
#define RS_Y_MIN_PARENT_MAX	0x0200
#define RS_Y_FRACTION			0x0400

#define RS_Y_MAX_OWN_MIN		0x0000
#define RS_Y_MAX_PARENT_MIN	0x1000
#define RS_Y_MAX_PARENT_MID	0x2000
#define RS_Y_MAX_PARENT_MAX	0x4000

//the 3d effect needs some sort of fake lighting values.
//these are for bumps. invert for inset things.
#define TD_TOP '0.8 0.8 0.8'
#define TD_LFT '0.6 0.6 0.6'
#define TD_RGT '0.2 0.2 0.2'
#define TD_BOT '0.0 0.0 0.0'

#ifndef MENUBACK_RGB
#define MENUBACK_RGB '0.4 0.365 0.29'
//#define MENUBACK_RGB '0.5 0.5 0.6'
#endif
#ifndef MENUBACK_ALPHA
#define MENUBACK_ALPHA 0.8
#endif

//input helpers - gamepads need 3 different 'keys' for natural movement through menus. :(
#define ISUPARROW(scan)   (scan == K_UPARROW || scan == K_GP_DPAD_UP || scan == K_GP_LTHUMB_UP || scan == K_GP_RTHUMB_UP)
#define ISDOWNARROW(scan) (scan == K_DOWNARROW || scan == K_GP_DPAD_DOWN || scan == K_GP_LTHUMB_DOWN || scan == K_GP_RTHUMB_DOWN)
#define ISLEFTARROW(scan) (scan == K_LEFTARROW || scan == K_GP_DPAD_LEFT || scan == K_GP_LTHUMB_LEFT || scan == K_GP_RTHUMB_LEFT)
#define ISRIGHTARROW(scan) (scan == K_RIGHTARROW || scan == K_GP_DPAD_RIGHT || scan == K_GP_LTHUMB_RIGHT || scan == K_GP_RTHUMB_RIGHT)
#define ISCONFIRMKEY(scan) (scan == K_ENTER || scan == K_GP_A || scan == K_GP_START)
#define ISCANCELKEY(scan) (scan == K_ENTER || scan == K_GP_B || scan == K_GP_VIEW)

//#ifdef TARGET_FTE
//#pragma TARGET FTE
//#endif

//#define HEIRACHYDEBUG	//enable this and press f1 to print out the current menuitem tree

class mitem_frame;
.vector		mins;		//gravity mins
.vector		maxs;		//gravity mins
class mitem
{
	void() 	mitem;
	virtual void(vector pos)										item_draw;
	virtual float(vector pos, float scan, float char, float down)	item_keypress = {return FALSE;};
	virtual void(mitem newfocus, float changedflag)					item_focuschange;	//move into frame?
	virtual string(string key)										get;
	virtual void(string key, string newval)							set;
	virtual float(string key)										isvalid;
	virtual void() 													item_remove;
	virtual void() 													item_resized;
	float 			item_flags;		//contains IF_ flags
	vector 		item_position;		//relative to the parent's client area. which is admittedly not well defined...
	vector 		item_size;			//interaction bounding box.
	float 			item_scale;		//text etc scale
	vector 		item_rgb;			//colours!
	float 			item_alpha;		//transparency value
	string 		item_text;			//used as a unique identifier
	string 		item_command;		//something to do when clicked. move out?
	mitem_frame	item_parent;		//the item that contains us. make mitem_frame?
	mitem 		item_next;		//the next child within the parent

	float			resizeflags;


	static void() totop;
};

//this struct is used to access the various drawing functions. this allows the functions to be replaced for worldspace stuff
typedef struct uiinfo_s
{
	void(float min_x, float min_y, float max_x, float max_y)							setcliparea;
	float(vector min, string imagename, vector size, vector col, float alph, float drawflag)	drawpic;
	float(vector min, vector size, vector col, float alph, float drawflag)				drawfill;
	float(vector min, float charcode, vector scale, vector col, float alph, float drawflag)	drawcharacter;
	float(vector min, string text, vector scale, vector col, float alph, float drawflag)		drawstring;

	mitem kgrabs;		//kfocused or mfocused or both or none to say what sort of grabs is in effect.
	mitem mgrabs;		//says who has stolen all input events.

	float ctrlheld;		//ctrl is held.
	float shiftheld;	//shift is held.
	float mousedown;	//which mouse buttons are currently held.
	vector oldmousepos;	//old mouse position, to track whether its moved.
	vector mousepos;	//current mouse position.
	vector screensize;	//total screen size
	vector drawrectmin;	//minimum scissor region, to clamp children to within the confines of its parent.
	vector drawrectmax;	//maximum scissor region

#ifndef MENU
	//menuqc has no concept of the world and cannot display or query 3d positions nor projections. Any related UI elements are thus not available to menuqc.
	//these globals are not part of the ui struct either, because they're illegal in world UIs.
	float havemouseworld;		//whether the mouseworld stuff is valid - ie: that the cursor is in a 3d view
	vector mouseworldnear;	//position of the mouse cursor upon the near clip plane in world space
	vector mouseworldfar;		//position of the mouse cursor upon the far(ish) clip plane in world space
#endif
} uiinfo_t;
var uiinfo_t ui =
{
	drawsetcliparea,
	drawpic,
	drawfill,
	drawcharacter,
	drawstring
};



void() queryscreensize =
{
#pragma warning disable F333
#ifdef MENU
	//there is no proper way to do this.
	//fte thus has special checks for these cvars, and they should not be autocvars if you want them to work properly.
	ui.screensize[0] = cvar("vid_conwidth");
	ui.screensize[1] = cvar("vid_conheight");
	ui.screensize[2] = 0;
#endif
#ifdef CSQC
	#ifndef CSQC_SIMPLE
		//make sure the screensize is set.
		normalize('0 0 1');	//when getproperty fails to return a meaningful value, make sure that we don't read some random stale value.
		ui.screensize = (vector)getproperty(VF_SCREENVSIZE);
		if (ui.screensize[2])	//lingering return value from normalize.
		{
			ui.screensize[2] = 0;
			ui.screensize[0] = cvar("vid_conwidth");
			ui.screensize[1] = cvar("vid_conheight");
		}
	#endif
#endif
#pragma warning enable F333
};

//helper function
float(vector minp, vector sz) mouseinbox =
{
	if (ui.mousepos[0] < minp_x)
		return FALSE;
	if (ui.mousepos[1] < minp_y)
		return FALSE;
	if (ui.mousepos[0] >= minp_x + sz_x)
		return FALSE;
	if (ui.mousepos[1] >= minp_y + sz_y)
		return FALSE;

	return TRUE;
};


void(mitem ch, vector parentsize) mitem_parentresized;

#include "mitem_frame.qc"
void() mitem::item_resized =
{
	if (this.item_parent)
		this.item_parent.item_flags |= IF_CLIENTMOVED;
};


#ifdef HEIRACHYDEBUG
void mitem_printnode(float depth, mitem root, string prefix)
{
	mitem ch;
	string col = "";
	if (root.item_flags & IF_KFOCUSED && root.item_flags & IF_MFOCUSED)
		col = "^2";
	else if (root.item_flags & IF_KFOCUSED)
		col = "^5";
	else if (root.item_flags & IF_MFOCUSED)
		col = "^3";

	print(sprintf("%s%s%i:%s%s\n", col, strpad(depth, ""), root, prefix, root.item_text));

	//can only recurse into items which are actually frames.
	if (root.item_flags & IF_ISFRAME)
	{
		mitem_frame fr = (mitem_frame)root;
		depth+=1;
		for(ch = fr.item_children; ch; ch = ch.item_next)
		{
			if (ch.item_parent != root)
				print("corrupt parent\n");
			if (ch.item_next == ch)
			{
				print("infinite loop\n");
				break;
			}
			string pre = "";
			if (fr.item_mactivechild == ch)
				pre = strcat(pre, "m");
			if (fr.item_kactivechild == ch)
				pre = strcat(pre, "k");
			if (pre)
				pre = strcat(pre, " ");
			mitem_printnode(depth, ch, pre);
		}
	}
};
void mitem_printtree(mitem_frame root, string from, float line)
{
	print(sprintf("%s:%g\n", from, line));
	mitem_printnode(0, root, "");
}
#endif

string(string key) mitem::get =
{
	//walk through to the parent for menu parents to track all this.
	if (this.item_parent)
		return this.item_parent.get(key);
	else	//no parent, just assume its a cvar.
		return cvar_string(key);
};
void(string key, string newval) mitem::set =
{
	//walk through to the parent for menu parents to track all this.
	if (this.item_parent)
		this.item_parent.set(key, newval);
	else	//no parent, just assume its a cvar.
		cvar_set(key, newval);
};
float(string key) mitem::isvalid =
{
	//walk through to the parent for menu parents to track all this.
	if (this.item_parent)
		return this.item_parent.isvalid(key);
	else	//no parent, just assume its a cvar.
	{
		float t = cvar_type(key);
		return (t & CVAR_TYPEFLAG_EXISTS) && !(t & CVAR_TYPEFLAG_READONLY);
	}
};


/***************************************************************************
basic 'null' item, for completeness, every other item logically inherits from this.
drawing this just shows its text right-aligned at w/2. most things will want to override this.
*/
void(mitem newfocus, float flag) mitem::item_focuschange =
{
};

//z order is determined by the list order. the ones at the end (oldest) are on top.
void() mitem::totop =
{
	mitem_frame p = item_parent;
	mitem prev;
	//unlink it
	if (p.item_children == this)
		prev = p.item_children = item_next;
	else
	{
		for (prev = p.item_children; prev; prev = prev.item_next)
		{
			if (prev.item_next == this)
			{
				prev.item_next = item_next;
				break;
			}
		}
	}

	item_next = __NULL__;
	if (prev)
	{
		//add it on the end
		while(prev.item_next)
			prev = prev.item_next;
		prev.item_next = this;
	}
	else
		p.item_children = this;
};

vector(mitem it) menuitem_textcolour =
{
	local vector col;
	col = it.item_rgb;
	if (!(it.item_flags & IF_SELECTABLE) && it.item_keypress && it.item_command != "")
		col *= 0.2;
	else
	{
		if (it.item_flags & IF_MFOCUSED)
			col_z = 0;
		if (it.item_flags & IF_KFOCUSED)
			col_x = 0;
	}
	return col;
};
void(vector pos) mitem::item_draw =
{
	vector col;
	pos_x += this.item_size[0] / 2 - stringwidth(this.item_text, TRUE, '1 1 0'*this.item_scale) - 8;
	col = menuitem_textcolour(this);
	ui.drawstring(pos, this.item_text, '1 1 0' * this.item_scale, col, this.item_alpha, 0);
};
void() mitem::item_remove =
{
	local mitem ch;
	//any children got removed by the frame code.

	//make sure the item is removed from its parent.
	local mitem_frame p = this.item_parent;
	if (p)
	{
		if (p.item_exclusive == this)
			p.item_exclusive = __NULL__;

		if (p.item_children == this)
			p.item_children = this.item_next;
		else
		{
			for(ch = p.item_children; ch; ch = ch.item_next)
			{
				if (ch.item_next == this)
				{
					ch.item_next = this.item_next;
					break;
				}
			}
		}
	}

	//pick a different item on the parent.
	if (p.item_mactivechild == this)
		p.item_focuschange(__NULL__, IF_MFOCUSED);
	if (p.item_kactivechild == this)
	{
		for (ch = p.item_children; ch; ch = ch.item_next)
		{
			if (ch.item_flags & IF_SELECTABLE)
				break;
		}
		p.item_focuschange(ch, IF_KFOCUSED);
	}
	if (ui.kgrabs == this)
		ui.kgrabs = __NULL__;
	if (ui.mgrabs == this)
		ui.mgrabs = __NULL__;
	remove((entity)this);

	//force mousefocus to update
	ui.oldmousepos = '-1 -1';
};
void() mitem::mitem =
{
	if (!this.item_scale)
		this.item_scale = 1;
	if (!this.item_rgb)
		this.item_rgb = '1 1 1';
	if (!this.item_alpha)
		this.item_alpha = 1;

	//force mousefocus to update
	ui.oldmousepos = '-1 -1';
};
