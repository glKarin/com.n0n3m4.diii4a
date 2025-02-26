/***************************************************************************
Basic frame, containing sub-items.
Logically the parent of menu objects. Also used for tabs.
*/

#ifndef MITEM_FRAME_QC__
#define MITEM_FRAME_QC__

class mitem_vslider : mitem
{
	virtual float(vector pos, float scan, float char, float down) item_keypress;
	virtual void(vector pos) item_draw;
	void() mitem_vslider;
	float val;
	float minv;
	float maxv;
	float stride;	//size of one 'notch'
};

class mitem_frame : mitem
{
	virtual void(mitem newfocus, float changedflag)				item_focuschange;
	virtual float(vector pos, float scan, float char, float down)	item_keypress;
	virtual void()			item_resized;
	virtual void(vector pos)	item_draw;
	virtual void()			item_remove;
	virtual void(mitem fromitem, string cmd)	item_execcommand;

	mitem 		item_children;
	mitem 		item_mactivechild;
	mitem 		item_kactivechild;
	mitem		item_exclusive;

	mitem_vslider	vslider;	//displayed if any client item's max[y] > our item_size[y]
	vector 	item_framesize;	//x=sides, y=top, z=bottom
	float frame_stdheight;	//adjust RS_Y_MIN_PARENT_MID to pinch inwards when the frame is smaller than this. Child items crossing the mid-point must size gracefully (like subframes).
	float frame_hasscroll;

	static mitem(string title) findchildtext;
	static mitem(string title) findchildcmd;
	static void(mitem newitem, float originflags, vector originmin, vector originmax) add;
	static void(mitem newitem, vector pos) adda;
	static void(mitem newitem, vector originmin, vector originmax) addm;
	static void(mitem newitem, float originflags, vector originmin, vector originmax) addr;
	static void(mitem newitem, float ypos) addc;

	void() mitem_frame =
	{
		item_flags |= IF_ISFRAME;
	};
};

void(mitem ch, vector parentsize) mitem_parentresized =
{
	float f = ch.resizeflags;

	if (f & RS_X_FRACTION)
		ch.item_position[0] = parentsize[0] * ch.mins[0];
	else if (f & RS_X_MIN_PARENT_MAX)
		ch.item_position[0] = parentsize[0] + ch.mins[0];
	else if (f & RS_X_MIN_PARENT_MID)
		ch.item_position[0] = parentsize[0]/2 + ch.mins[0];
	else //if (f & RS_X_MIN_PARENT_MIN)
		ch.item_position[0] = ch.mins[0];

	if (f & RS_X_FRACTION)
		ch.item_position[0] = parentsize[0] * ch.maxs[0];
	else if (f & RS_X_MAX_PARENT_MIN)
		ch.item_size[0] = ch.maxs[0] - ch.item_position[0];
	else if (f & RS_X_MAX_PARENT_MID)
		ch.item_size[0] = ch.maxs[0]+parentsize[0]/2 - ch.item_position[0];
	else if (f & RS_X_MAX_PARENT_MAX)
		ch.item_size[0] = ch.maxs[0]+parentsize[0] - ch.item_position[0];
	else //if (f & RS_X_MAX_OWN_MIN)
		ch.item_size[0] = ch.maxs[0];

	if (f & RS_Y_FRACTION)
		ch.item_position[1] = parentsize[1] * ch.mins[1];
	else if (f & RS_Y_MIN_PARENT_MAX)
		ch.item_position[1] = parentsize[1] + ch.mins[1];
	else if (f & RS_Y_MIN_PARENT_MID)
	{
		ch.item_position[1] = parentsize[1]/2 + ch.mins[1];
		if (ch.item_parent.frame_stdheight)
		if (parentsize[1] < ch.item_parent.frame_stdheight)
		{
			if (ch.item_position[1] < parentsize[1]*0.5)
				ch.item_position[1] += (ch.item_parent.frame_stdheight-parentsize[1])*0.5;
			else
				ch.item_position[1] -= (ch.item_parent.frame_stdheight-parentsize[1])*0.5;
		}
	}
	else //if (f & RS_Y_MIN_PARENT_MIN)
		ch.item_position[1] = ch.mins[1];

	if (f & RS_Y_FRACTION)
		ch.item_position[1] = parentsize[1] * ch.maxs[1];
	else if (f & RS_Y_MAX_PARENT_MIN)
		ch.item_size[1] = ch.maxs[1] - ch.item_position[1];
	else if (f & RS_Y_MAX_PARENT_MID)
	{
		ch.item_size[1] = ch.maxs[1]+parentsize[1]/2 - ch.item_position[1];
		if (ch.item_parent.frame_stdheight)
		if (parentsize[1] < ch.item_parent.frame_stdheight)
		{
			if (ch.item_position[1]+ch.item_size[1] < parentsize[1]*0.5)
				ch.item_size[1] += (ch.item_parent.frame_stdheight-parentsize[1])*0.5;
			else
				ch.item_size[1] -= (ch.item_parent.frame_stdheight-parentsize[1])*0.5;
		}
	}
	else if (f & RS_Y_MAX_PARENT_MAX)
		ch.item_size[1] = ch.maxs[1]+parentsize[1] - ch.item_position[1];
	else //if (f & RS_Y_MAX_OWN_MIN)
		ch.item_size[1] = ch.maxs[1];
};

void(mitem fromitem, string cmd) mitem_frame::item_execcommand =
{
	if (item_parent)
		item_parent.item_execcommand(fromitem, cmd);
	else
		localcmd(strcat(cmd, "\n"));
};

mitem(string title) mitem_frame::findchildtext =
{
	mitem ch;
	for (ch = item_children; ch; ch = ch.item_next)
	{
		if (ch.item_text == title)
			return ch;
	}
	return __NULL__;
};
mitem(string title) mitem_frame::findchildcmd =
{
	mitem ch;
	for (ch = item_children; ch; ch = ch.item_next)
	{
		if (ch.item_command == title)
			return ch;
	}
	return __NULL__;
};


//adds an item with the desired origin settings
void(mitem newitem, float originflags, vector originmin, vector originmax) mitem_frame::add =
{
	newitem.item_next = item_children;
	item_children = newitem;
	newitem.item_parent = this;

	//set up position and resize
	newitem.resizeflags = originflags;
	newitem.mins = originmin;
	newitem.maxs = originmax;
	local vector parentsize = [item_size[0] - item_framesize[0]*2, item_size[1] - (item_framesize[1] + item_framesize[2])];
	mitem_parentresized(newitem, parentsize);
	newitem.item_resized();
	
	item_flags |= IF_CLIENTMOVED;	//make sure it happens.

//	if (!item_kactivechild && (newitem.item_flags & IF_SELECTABLE))
//		item_focuschange(newitem, IF_KFOCUSED);
};

//adds an item to the parent with an absolute position relative to the parent's top-left. objects are expected to already have a size specified.
void(mitem newitem, vector pos) mitem_frame::adda =
{
	local vector parentsize = [item_size[0] - item_framesize[0]*2, item_size[1] - (item_framesize[1] + item_framesize[2])];
	newitem.item_next = item_children;
	item_children = newitem;
	newitem.item_parent = this;

	newitem.resizeflags = RS_X_MIN_PARENT_MIN | RS_X_MAX_OWN_MIN | RS_Y_MIN_PARENT_MIN | RS_Y_MAX_OWN_MIN;
	newitem.mins = pos;
	newitem.maxs = newitem.item_size;

	mitem_parentresized(newitem, parentsize);
	newitem.item_resized();
	
	item_flags |= IF_CLIENTMOVED;	//make sure it happens.

//	if (!item_kactivechild && (newitem.item_flags & IF_SELECTABLE))
//		item_focuschange(newitem, IF_KFOCUSED);
};

//adds an item to the parent in reverse order (ie: at the tail, so the actual order in code)
void(mitem newitem, float originflags, vector originmin, vector originmax) mitem_frame::addr =
{
	local vector parentsize = [item_size[0] - item_framesize[0]*2, item_size[1] - (item_framesize[1] + item_framesize[2])];
	if (item_children)
	{
		local mitem prev;
		for (prev = item_children; prev.item_next; prev = prev.item_next)
			;
		prev.item_next = newitem;
		newitem.item_next = __NULL__;
	}
	else
	{
		newitem.item_next = item_children;
		item_children = newitem;
	}
	newitem.item_parent = this;

	newitem.resizeflags = originflags;
	newitem.mins = originmin;
	newitem.maxs = originmax;
	mitem_parentresized(newitem, parentsize);
	newitem.item_resized();
	
	item_flags |= IF_CLIENTMOVED;	//make sure it happens.

//	if (!item_kactivechild && (newitem.item_flags & IF_SELECTABLE))
//		item_focuschange(newitem, IF_KFOCUSED);
};

//adds an item on the parent with the x coord centered, and absolute y.
//if multiple items are at the same ypos, it will recenter all with respect to the others
void(mitem newitem, float ypos) mitem_frame::addc =
{
	float w, c;
	local mitem prev;
	local vector parentsize = [item_size[0] - item_framesize[0]*2, item_size[1] - (item_framesize[1] + item_framesize[2])];

	newitem.item_next = item_children;
	item_children = newitem;
	newitem.item_position[1] = ypos;
	newitem.item_position[0] = (parentsize[0] - newitem.item_size[0])*0.5;
	newitem.item_parent = this;


	newitem.resizeflags = RS_X_MIN_PARENT_MID | RS_X_MAX_OWN_MIN | RS_Y_MIN_PARENT_MIN | RS_Y_MAX_OWN_MIN;
	newitem.mins[0] = 0;
	newitem.mins[1] = ypos;
	newitem.maxs = newitem.item_size;

	//count the width of the other items at this height.
	w = 0;
	c = 0;
	for (prev = item_children; prev; prev = prev.item_next)
		if (prev.resizeflags == newitem.resizeflags && prev.mins[1] == ypos && prev.maxs[1] == newitem.maxs[1])
		{
			w += prev.maxs[0];
			c += 1;
		}

	//distribute them evenly (from the right, because its add-at-head)
	w += (c-1)*16;	//this much gap space
	for (prev = item_children; prev; prev = prev.item_next)
	{
		if (prev.resizeflags == newitem.resizeflags && prev.mins[1] == ypos && prev.maxs[1] == newitem.maxs[1])
		{
			w -= prev.maxs[0];
			prev.mins[0] = (w+prev.maxs[0])/-2;
			mitem_parentresized(prev, parentsize);
			w -= 16;
		}
	}
	newitem.item_resized();
	
	item_flags |= IF_CLIENTMOVED;	//make sure it happens.

//	if (!item_kactivechild && (newitem.item_flags & IF_SELECTABLE))
//		item_focuschange(newitem, IF_KFOCUSED);
};

//Adds the item in the exact middle of the parent, in both axis
void(mitem newitem, vector min, vector max) mitem_frame::addm =
{
	this.add(newitem, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_PARENT_MID, min, max);
};

//updates this.item_activechild, and calls focus notifications to ensure things get the message. 
//flag should be IF_MFOCUSED or IF_KFOCUSED
void(mitem newfocus, float flag) mitem_frame::item_focuschange =
{
	local mitem it;

	if (newfocus == this)
	{
		//our focus didn't change, but the parent is telling us that *it* got changed while we're the focus
		if (flag & IF_MFOCUSED)
		{
			if (item_parent.item_flags & IF_MFOCUSED && item_parent.item_mactivechild == this)
				item_flags |= IF_MFOCUSED;
			else
				item_flags = item_flags - (item_flags&IF_MFOCUSED);
			//and tell the child
			it = item_mactivechild;
			if (it)
				if (it.item_focuschange)
					it.item_focuschange(it, IF_MFOCUSED);
		}
		if (flag & IF_KFOCUSED)
		{
			if (item_parent.item_flags & IF_KFOCUSED && item_parent.item_kactivechild == this)
				item_flags |= IF_KFOCUSED;
			else
				item_flags = item_flags - (item_flags&IF_KFOCUSED);
			//and tell the child
			it = item_kactivechild;
			if (it)
				if (it.item_focuschange)
					it.item_focuschange(it, IF_KFOCUSED);
		}
		return;
	}

	if ((flag & IF_MFOCUSED) && item_mactivechild != newfocus)
	{
		//make key focus follow the mouse cursor. this should probably only apply to button menus or something? :s
		if (newfocus && (item_flags & IF_FOCUSFOLLOWSMOUSE))
			flag |= IF_KFOCUSED;

		it = item_mactivechild;
		item_mactivechild = newfocus;
		if (it)
		{
			it.item_flags = it.item_flags - (it.item_flags&IF_MFOCUSED);
			if (it.item_focuschange)
				it.item_focuschange(it, IF_MFOCUSED);
		}
		it = item_mactivechild;
		if (it)
		{
			it.item_flags = it.item_flags | (item_flags & IF_MFOCUSED);
			if (it.item_focuschange)
				it.item_focuschange(it, IF_MFOCUSED);
		}
	}

	if ((flag & IF_KFOCUSED) && item_kactivechild != newfocus)
	{
		it = item_kactivechild;
		item_kactivechild = newfocus;
		if (it)
		{
			it.item_flags = it.item_flags - (it.item_flags&IF_KFOCUSED);
			if (it.item_focuschange)
				it.item_focuschange(it, IF_KFOCUSED);
		}
		it = item_kactivechild;
		if (it)
		{
			it.item_flags = it.item_flags | (item_flags & IF_KFOCUSED);
			if (it.item_focuschange)
				it.item_focuschange(it, IF_KFOCUSED);
		}
	}
};


float(vector pos, float scan, float char, float down) mitem_vslider::item_keypress =
{
	if (down != 1)
	{
		if ((scan == K_TOUCHTAP || scan == K_MOUSE1) && ui.mgrabs == this)
			ui.mgrabs = __NULL__;
		return FALSE;
	}
	if (scan == K_MWHEELDOWN)
		val = bound(minv, val+stride, maxv);
	else if (scan == K_MWHEELUP)
		val = bound(minv, val-stride, maxv);
	else if (scan == K_TOUCHTAP || scan == K_MOUSE1)
		ui.mgrabs = this;
	else
		return FALSE;
	return TRUE;
};
void(vector pos) mitem_vslider::item_draw =
{
	float f;
	float w = item_size[0];
	float h = item_size[1];
	float isize = w;
	
	vector v = dp_workarounds?'0 0 0':drawgetimagesize("scrollbars/slider.tga");

	if (v != '0 0 0')
	{
		ui.drawpic(pos, "scrollbars/arrow_up.tga", [w, w], '1 1 1', item_alpha, 0);	//top
		pos_y += w;
		h -= w*2;
		ui.drawpic(pos + [0, h], "scrollbars/arrow_down.tga", [w, w], '1 1 1', item_alpha, 0);	//bottom
		ui.drawpic(pos, "scrollbars/slidebg.tga", [w, h], '1 1 1', item_alpha, 0);	//back-middle
	
		isize = (v_y * w) / (v_x);
		if (isize > h/2)
			isize = h/2;
	}
	else
	{
//		ui.drawfill(pos, [w, w], '1 1 1', item_alpha, 0);	//top
//		pos_y += w;
//		h -= w*2;
//		ui.drawfill(pos + [0, h], [w, w], '1 1 1', item_alpha, 0);	//bottom
		ui.drawfill(pos, [w, h], '0.5 0.5 0.5', item_alpha, 0);	//back-middle
	}

	if (ui.mgrabs == this)
	{
		f = (ui.mousepos[1] - pos_y - (isize/2))/(h - isize);
		f = bound(0, f, 1);
		val = minv + (f * (maxv - minv));
	}
	else
		f = bound(0, (val - minv) / (maxv - minv), 1);

	h -= isize;
	if (v != '0 0 0')
		ui.drawpic(pos + [0, h*f], "scrollbars/slider.tga", [w, isize], '1 1 1', item_alpha, 0);	//back-middle
	else
		ui.drawfill(pos + [0, h*f], [w, isize], '1 1 1', item_alpha, 0);	//back-middle
};
void() mitem_vslider::mitem_vslider =
{
	item_size[0] = 8;
	item_size[1] = 128;
};

//does NOT wrap.
//does NOT pass go.
static mitem(mitem item, float upwards) menu_simplenextitem =
{
	mitem_frame menu = item.item_parent;
	mitem prev;
	if (upwards)
	{
		if (item)
		{
			item = item.item_next;
			if (!item)
				return __NULL__;
			return item;
		}
		else
			return __NULL__;
	}
	else
	{
		for(prev = menu.item_children; prev.item_next; prev = prev.item_next)
		{
			if (prev.item_next == item)
				return prev;
		}
		return __NULL__;
	}
};

//finds the next/prev item through multiple children, returning NULL when it runs out of items in the sequence.
//call this with item==null to find the first item in the sequence (to handle wraps).
static mitem(mitem_frame menu, float upwards, mitem item) menu_findnextitem =
{
	mitem_frame frame;
	mitem citem;
	
	if (item && (item.item_flags & IF_ISFRAME))
	{
		frame = (mitem_frame)item;
		citem = menu_findnextitem(frame, upwards, frame.item_kactivechild?frame.item_kactivechild:frame.item_mactivechild);
		if (citem)
			return citem;
	}
	
	for(;;)
	{
		if (!item)
		{	//we go for the opposite end here, as we assume to be starting/unfocused
			item = menu.item_children;
			if (!upwards && item)
			{
				while(item.item_next)
					item = item.item_next;
			}
		}
		else
			item = menu_simplenextitem(item, upwards);
		
		if (!item)
		{	//we reached the end of the list, let the parent frame try its next
			return __NULL__;
		}
		
		if (item.item_flags & IF_ISFRAME)
		{	//if the next item is a frame, try and select its first element instead
			frame = (mitem_frame)item;
			citem = menu_findnextitem(frame, upwards, __NULL__);
			if (citem)
				return citem;
		}

		if (item.item_flags & IF_INVISIBLE)
			continue;
		if (item.item_flags & IF_SELECTABLE)
			return item;
	}
	
};
static void(mitem item) menu_deselectitem =
{
	if (!item)
		return;
	if (item && (item.item_flags & IF_ISFRAME))
	{
		mitem_frame frame = (mitem_frame)item;
		if (frame.item_kactivechild)
			menu_deselectitem(frame.item_kactivechild);
	}
	item.item_focuschange(__NULL__, IF_KFOCUSED);	//deselect the previous one	
};
static void(mitem item) menu_selectitem =
{
	if (!item)
		return;
	mitem_frame menu = item.item_parent;
	if (menu)
	{
		if (menu.item_kactivechild != item)
			menu_deselectitem(menu.item_kactivechild);
		menu_selectitem(menu);
		menu.item_focuschange(item, IF_KFOCUSED);	//focus on the new
	}
};
void(mitem_frame rootmenu, float upwards) menu_selectnextitem =
{
	mitem item = menu_findnextitem(rootmenu, upwards, rootmenu.item_kactivechild?rootmenu.item_kactivechild:rootmenu.item_mactivechild);
	if (!item)
		item = menu_findnextitem(rootmenu, upwards, __NULL__);

	menu_selectitem(item);
	item.item_focuschange(item, IF_KFOCUSED);	//focus on the new
};

float(vector pos, float scan, float char, float down) mitem_frame::item_keypress =
{
	mitem ch;
	float handled = FALSE;

	//compensate for the frame
	pos[0] += item_framesize[0];
	pos[1] += item_framesize[1];

	if ((scan >= K_MOUSE1 && scan <= K_MOUSE5 && scan != K_MWHEELUP && scan != K_MWHEELDOWN) || scan == K_TOUCHTAP)
	{
		if (item_exclusive)
			ch = item_exclusive;
		else 
			ch = item_mactivechild;
		if (down)	//keyboard focus follows on mouse click.
		{
			item_focuschange(ch, IF_KFOCUSED);
		}
	}
	else
	{
		ch = item_kactivechild;
		if (!ch && down)
		{
			//if there's no key child active, then go and pick one so you can just start using keyboard access etc.
			if (item_exclusive)
				ch = item_exclusive;
			else if (item_mactivechild)
				ch = item_mactivechild;
			else
			{
				mitem c;
				for (c = item_children; c; c = c.item_next)
				{
					if (c.item_flags & IF_SELECTABLE)
						ch = c;
				}
			}
			if (ch)
				item_focuschange(ch, IF_KFOCUSED);
			ch = item_kactivechild;
		}
	}

	if (vslider)
		pos[1] -= vslider.val;

	if (ch)
	{
		if (ch.item_keypress)
			handled = ch.item_keypress(pos + ch.item_position, scan, char, down);
	}
	if (vslider && !handled && (scan == K_MWHEELUP || scan == K_MWHEELDOWN))	//give mwheel to the slider if its visible.
		handled = vslider.item_keypress(pos + vslider.item_position, scan, char, down);

	return handled;
};
void() mitem_frame::item_remove =
{
	local mitem ch;
	while (this.item_children)
	{
		ch = this.item_children;
		this.item_children = ch.item_next;
		ch.item_remove();
	}
	super::item_remove();
};
void() mitem_frame::item_resized =
{
	mitem ch;
	vector framemax = [item_size[0] - item_framesize[0]*2, item_size[1] - (item_framesize[1] + item_framesize[2])];
	for (ch = this.item_children; ch; ch = ch.item_next)
	{
		vector os = ch.item_size;
		mitem_parentresized(ch, framemax);
		if (ch.item_resized && ch.item_size != os)
			ch.item_resized();
	}
	item_flags |= IF_CLIENTMOVED;	//make sure it happens.

	super::item_resized();
};
void(vector pos) mitem_frame::item_draw =
{
	local vector omin = ui.drawrectmin, omax = ui.drawrectmax;
	local mitem ch = __NULL__;
	local vector clientpos;
	local vector clientsize;

	if (frame_hasscroll && (item_flags & IF_CLIENTMOVED))
	{
		//if a client object moved, make sure the scrollbar is updated
		item_flags &~= IF_CLIENTMOVED;
		clientsize= '0 0';
		for(ch = item_children; ch; ch = ch.item_next)
		{
			if (clientsize[0] < ch.item_position[0] + ch.item_size[0])
				clientsize[0] = ch.item_position[0] + ch.item_size[0];
			if (clientsize[1] < ch.item_position[1] + ch.item_size[1])
				clientsize[1] = ch.item_position[1] + ch.item_size[1];
		}
//		if (clientsize[0] > item_size[0] - item_framesize[0]*2)
//			//add hscroll
		if (clientsize[1] > item_size[1] - (item_framesize[1]+item_framesize[2]))
		{
			if (!vslider)
			{
				local mitem_vslider tmp;
				tmp = spawn(mitem_vslider, val:0, stride:8);
				vslider = tmp;
			}
			vslider.maxv = clientsize[1] - (item_size[1] - (item_framesize[1]+item_framesize[2]));
		}
		else if (vslider)
		{
			vslider.item_remove();
			vslider = (mitem_vslider)__NULL__;
		}
	}
	else if (!frame_hasscroll && this.vslider)
	{
		vslider.item_remove();
		vslider = (mitem_vslider)__NULL__;
	}


	//compensate for the frame
	pos[0] += item_framesize[0];
	pos[1] += item_framesize[1];

	clientpos = pos;
	clientsize = this.item_size;
	clientsize[0] -= item_framesize[0]*2;
	clientsize[1] -= (item_framesize[1]+item_framesize[2]);

	if (vslider)
	{
		//scroll+shrink the client area to fit the slider on it.
		clientpos[1] -= vslider.val;
		clientsize[0] -= vslider.item_size[0];
	}


	if (ui.mousepos != ui.oldmousepos)
	{
		local mitem newfocus = __NULL__;
		//if the mouse moved, select the new item
		if (item_exclusive)
			newfocus = item_exclusive;
		else
		{
			for(ch = item_children; ch; ch = ch.item_next)
			{
				if (ch.item_flags & IF_SELECTABLE)
				if (mouseinbox(clientpos + ch.item_position, ch.item_size))
				{
					newfocus = ch;
				}
			}
		}
		if (vslider)
			if (mouseinbox(pos + [clientsize[0], 0], vslider.item_size))
				newfocus = vslider;
		this.item_focuschange(newfocus, IF_MFOCUSED);
	}

	//clip the draw rect to our area, so children don't draw outside it. don't draw if its inverted.
	if (pos_x > ui.drawrectmin[0])
		ui.drawrectmin[0] = pos_x;
	if (pos_y > ui.drawrectmin[1])
		ui.drawrectmin[1] = pos_y;
	if (pos_x+clientsize_x < ui.drawrectmax[0])
		ui.drawrectmax[0] = pos_x+clientsize_x;
	if (pos_y+clientsize_y < ui.drawrectmax[1])
		ui.drawrectmax[1] = pos_y+clientsize[1];
	if (ui.drawrectmax[0] > ui.drawrectmin[0] && ui.drawrectmax[1] > ui.drawrectmin[1])
	{
		ui.setcliparea(ui.drawrectmin[0], ui.drawrectmin[1], ui.drawrectmax[0] - ui.drawrectmin[0], ui.drawrectmax[1] - ui.drawrectmin[1]);
		if (item_exclusive)
			item_exclusive.item_draw(clientpos + ch.item_position);
		else
		{
			for(ch = item_children; ch; ch = ch.item_next)
			{
				if not (ch.item_flags & IF_INVISIBLE)
					ch.item_draw(clientpos + ch.item_position);
			}
		}
		ui.setcliparea(omin_x, omin_y, omax_x - omin_x, omax_y - omin_y);

		if (vslider)
		{
			vslider.item_size[1] = clientsize[1];
			vslider.item_draw(pos + [clientsize[0], 0]);
		}
	}
	ui.drawrectmin = omin;
	ui.drawrectmax = omax;
};
#define menuitemframe_spawn(sz) spawn(mitem_frame, item_size:sz)

#endif //MITEM_FRAME_QC__
