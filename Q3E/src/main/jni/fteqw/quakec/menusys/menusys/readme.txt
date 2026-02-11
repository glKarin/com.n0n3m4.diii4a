integrating into an existing mod:
the csqc/menu needs to instanciate a mitem_desktop (or derived class). items_keypress+items_draw need to be called in the relevent places with the relevent arguments, then you can start creating children and throwing them at the screen.
the desktop is the root object, and tracks whether the mouse/keys should be grabbed.

general overview:
The menu system is build from a heirachy/tree of mitem nodes.
To draw the menu, the tree is walked from the root (the 'desktop' object) through its children (pictures, text, or whatever).
Each mitem inherits its various properties from a parent object using classes. For instance, the desktop inherits from mitem_frame(read: a container object), which in turn inherits from mitem(the root type).
When a container is moved or resized, all of its children (and their children) will automatically be resized accordingly. Once the container is destroyed, the children will also be automatically destroyed.
Drawing items is fairly simple. The root node is told to draw itself. Once it has drawn its background, it walks through its children asking them to draw themselves (telling the child exactly where it is on the scree). Other containers(like windows) do the same. In this way the entire tree is drawn.
Input happens in a somewhat similar manner. The parent decides which child the mouse is over, and sends mouse or keyboard to the relevent focused child. As a special exception, if an element has set ui.*grabs to refer to itself, that object gets to snoop on input first before even the desktop gets a chance.
To make a quake-style menu, you should create an mitem_exmenu, and then call the desktop's .add method to insert it. Then you need to spawn things like cvar sliders/combos/etc and use one of the menu's add methods to place it on the menu/window. Check the example code.

Items that don't specify a method will inherit that method from its parent.
If you want only a minor change, for instance drawing a background under the item, you can make an item_draw method which draws the background then calls super::item_draw() with the same arguments in order to provide the parent's normal behaviour. Doing this with a menu's get+set methods allows you to use slider/etc mitems with things other than direct cvars (if you want an 'apply changes' button or so).


globals:
autocvar_menu_font:	defaults to 'cour', which will only work in windows, fallback font otherwise. The menu code will register+use a font with this name suitable for 8 vpixels high, and 16vpixels.
dp_workarounds:		needed to work around DP bugs. Set to 1 if you detect that the code is running inside DP.
ui:				a struct containing the current ui state. Can be temporarily overwritten for alternative menu systems (like ones drawn on walls or whatever).
				probably would have been nicer if it was a pointer, but I didn't want to depend upon those.
				Within the ui struct are a few functions which typically just map to the equivelent builtins.
				Placing a copy of these in the ui struct allows them to be overridden in order to project a menu/ui upon a wall efficiently without extra code.
				these are the functions available: setcliparea, drawpic, drawfill, drawcharacter, drawstring
ui.mousedown:		the mouse buttons that are currently pressed.
ui.oldmousepos:		the mouse position that was set in the previous frame. this can be used to detect whether the mouse has moved.
ui.mousepos:		the current virtual screen position of the mouse cursor.
ui.screensize:		the virtual resolution of the screen - the dimensions the UI needs to fill.
ui.drawrectmin:		the current viewport scissor min position (so over-sized items can draw outside of their containing frames without issue).
ui.drawrectmax:		the current viewport scissor max position (so over-sized items can draw outside of their containing frames without issue).
ui.havemouseworld:	whether the mouseworld[near/far] globals are valid - ie: that the cursor is in a 3d view. only exists in csqc.
ui.mouseworldnear:	position of the mouse cursor upon the near clip plane in world space.
ui.mouseworldfar:	position of the mouse cursor upon the far(ish) clip plane in world space. You can trace a line between these two vectors to detect which entities the mouse should interact with.
ui.kgrabs:			set this global in order to redirect all keyboard buttons to this object.
ui.mgrabs:			set this global in order to redirect all mouse buttons to this object. If the object that has focus also has item_flags&IF_NOCURSOR, the mouse cursor will be hidden.

helper functions:
mouseinbox:		returns whether ui.mousepos is within the box specified by the min+size arguments.

mitems.qc:class mitem
	root menuitem object that all else inherits from

	EVENTS:
	void item_draw(pos): override this to replace how the item displays. the default is quite lame. pos is the virtual screen position.
	float item_keypress(vector pos,float scan,float char, float down): the object got a keyboard or mouse click. pos is the virtual screen position. scan is the K_FOO scancode constant. char is the unicode value of the codepoint. either may be 0. some systems might always use 0 for one of scan+char. down says that the key was just pressed, false means it just got released.
	void item_focuschange(mitem newfocus, float changedflag): the object's focus changed. newfocus is the item that gained focus, changedflag is either IF_MFOCUSED, IF_KFOCUSED, or both. check this against newfocus to see if this object gained focus.
	string(string key) get:	helper function. calls the parent's get method. 
	void(string key, string newval) set:	helper function. calls the parent's set method. 
	void() item_remove: the item got removed. this callback can be used to free memory. be sure to notify super.
	void() item_resized: the item has been resized or moved. provided to resize+move helper objects to match a parent, or recalculate cached extents or whatever.

	FIELDS:
	item_flags: various flags
		IF_SELECTABLE: item can be selected, either with mouse or keyboard.
		IF_INTERACT: flag reserved for child classes to use.
		IF_RESIZABLE: item can be resized. menus will show resize corners.
		IF_MFOCUSED: item has mouse focus (parents will retain the flag)
		IF_KFOCUSED: item has keyboard focus (parents will retain the flag)
		IF_NOKILL: item has been marked as resisting closure. This applies to menus and will disable the implicit 'x' button.
		IF_NOCURSOR: if the item grabs the mouse, the cursor will be hidden, allowing mlook to still work when some widget is grabbing everything (eg: an exmenu using right-click to look).
	item_position: the (virtual) position of the object relative to its parent.
	item_size: the current (virtual) size of the object.
	item_scale: typically serves to rescale text. not used by the menu framework itself.
	item_rgb: the colour field for the item. typically defaults to '1 1 1' if not otherwise set.
	item_alpha: the alpha value for the item. typically defaults to 1.
	item_text: typically used as the caption. also used as a searchable item name.
	item_command: the cvar or console command associated with the object. potentially other uses. yay for repurposing fields.
	item_parent: the frame that contains this object.
	item_next: the next sibling within the parent.
	mins: the resize gravity bias for the 'min' pos.
	maxs: the resize gravity bias for the 'max' pos.
	resizeflags: controls the meaning of the mins+maxs positions.
		RS_[X/Y]_[MIN/MAX]_PARENT_MIN: the min/max position is relative to the top/left of the parent.
		RS_[X/Y]_[MIN/MAX]_PARENT_MID: the min/max position is relative to the center of the parent.
		RS_[X/Y]_[MIN/MAX]_PARENT_MAX: the min/max position is relative to the bottom/right of the parent.
		RS_[X/Y]_FRACTION: the min+max positions are 0-1 values scaled within the min/max position of the parent.
		RS_[X/Y]_MAX_OWN_MIN: the max position is set relative to the objects own minimum size, giving an absolute object size.

	static functions:
	totop(): moves the object to the top of the parent's z-order. drawn last, this object will now appear over everything else. does not affect focus.

	non-member functions (FIXME):
	mitem_printtree: debug function to print out a list of the items children+siblings.
	queryscreensize: updates the screensize global.
	mitem_parentresized: updates an item's position and size according to its resizeflags.
	menuitem_textcolour: helper function. determines the text colour to use for cvar widgets based upon selectable and mouse/keyboard focus.

mitem_frame.qc:class mitem_frame : mitem
	generic borderless container object for other items.

	FIELDS:
	item_framesize_x: border width.	children will not overlap this.
	item_framesize_y: border at top. children will not overlap this.
	item_framesize_z: border at bottom. children will not overlap this.
	frame_hasscroll: if true, enables a vertical slider so the frame is still usable if the screen is too small.
	item_children: the first child object of the frame
	item_mactivechild: the current child that has the mouse over it.
	item_kactivechild: the current child that has keyboard focus.

	static functions:
	findchild(string title): scans through a frame's children looking for an item with a matching item_text.
	add(mitem newitem, float resizeflags, vector originmin, vector originmax): adds an item to a frame, with specified gravity+position etc settings. see mitem::resizeflags for details.
	adda(mitem newitem, vector pos): adds an item to a frame with preset item_size and position relative to the top-left of the parent frame.
	addr(mitem newitem, float resizeflags, vector originmin, vector originmax): adds an item to a frame, with specified gravity+position etc settings, with reversed z order. this item will be drawn over the top of the previous objects, and is the more natural ordering.
	addc(mitem newitem, float ypos): adds an item to a frame with a preset item_size at a specific y position. multiple objects with the same mins_y+maxs_y will automatically be spread across horizontally with a gap between each.

mitem_bind.qc:class mitem_bind : mitem
	a widget to change a key binding. up to two keys will be listed. additional keys will currently remain hidden.
	item_text: the description of the key binding (ie: "Attack")
	item_command: the console command to bind the key to (ie: "+attack")

mitem_checkbox.qc:class mitem_check : mitem
	a true/false cvar checkbox.
	item_text: the description of the setting
	item_command: the name of the cvar to toggle

	optional factory: mitem_check(string text, string command, vector sz) menuitemcheck_spawn;

mitem_colours.qc:class mitem_colours : mitem
	A simple colour picker. supports only hue.
	item_text: the description of the setting
	item_command: the name of the cvar to change (sets to 0xRRGGBB notation).

	factory: mitem_colours(string text, string command, vector sz) menuitemcolour_spawn;

mitem_combo.qc:class mitem_combo : mitem
	multiple choice widget.
	item_text: the description of the setting
	mstrlist: a list of the valid settings, in "\"value\" \"description\" \"value\" \"description\"" notation.
	FIXME: must spawn through: mitem_combo(string text, string command, vector sz, string valuelist) menuitemcombo_spawn;

mitem_combo.qc:class mitem_combo_popup : mitem
	internal friend class of the combo.
	this holds the list of options when clicked.
	you should not use this class directly.

mitem_desktop.qc:class mitem_desktop : mitem_frame
	the root item that should be used as the parent of all other elements in order to be visible.
	forces itself to fullscreen, handles grabs, etc.
	in csqc, will display the default game view (including split screen views).

	csqc
	float(mitem_desktop desktop, float evtype, float scanx, float chary, float devid) items_keypress
	menuqc:
	float(mitem_desktop desktop, float scan, float char, float down) items_keypress
	both:
	void(mitem_desktop desktop) items_draw


mitem_edittext.qc:class mitem_edit : mitem
	text entry widget (typically) attached to a cvar.
	item_text: the short description of the setting.
	item_command: the name of the cvar to receive a new value/be displayed

mitem_exmenu.qc:class mitem_exmenu : mitem_frame
	'exclusive' menu object.
	this container has no borders.
	does not provide a background. use a mitem_pic or mitem_fill for that.
	not much more than a frame, but can take unhandled escape/right click to close the menu.
	typically used fullscreen.
	item_text: a searchable identifier for the object
	item_command: the console command to execute when the item is removed. used to restore the parent menu.

mitem_frame.qc:class mitem_vslider : mitem
	vertical up/down slider not attached to a cvar.
	used to scroll frames up+down for when you have too many items in there for the current video mode.
	fixme: document

	factory macro: menuitemframe_spawn(sz)
	typically created via inheritance.

mitem_menu.qc:class mitem_menu : mitem_frame
	movable resizable window

	item_flags:
		IF_RESIZABLE: enables resize handles on left+bottom+right+lower corners.
		IF_NOKILL: disables the 'x' button on the top-right.
	non-member construtor:
	menu_spawn(mitem_frame desktop, string mname, vector sz): centers and automatically adds the object. required for the default border size. do not call any of the add* methods yourself if you use this.

mitem_slider.qc:class mitem_hslider : mitem
	horizontal slider attached to a cvar.
	item_text: the short description of the setting.
	item_command: the name of the cvar to change.
	item_slidercontrols_x: the minimum value.
	item_slidercontrols_y: the maximum value.
	item_slidercontrols_z: how much to change the cvar by each time the user uses the left/right arrows to change its value.

mitem_tabs.qc:class mitem_tabs : mitem_frame
	tabstrip object that appears presenting the various tab options.
	acts as a container only for mitem_tab objects which are the real containers.

mitem_tabs.qc:class mitem_tab : mitem_frame
	child of a tabstrip that contains the various elements sited upon a single tab.
	item_text: the caption displayed for the tab.

mitems_common.qc:class mitem_fill : mitem
	fills the screen region with a block of colour.
	item_rgb: colour to fill it with.
	item_alpha: alpha blended by this much. 1 for full block colour.

mitems_common.qc:class mitem_pic : mitem
	simple image display.
	item_text: the default image to display.
	item_text_mactive: if specified, the image to display when the object has mouse focus.
	item_command: console command to execute when clicked.

mitems_common.qc:class mitem_text : mitem
	simple plain text
	item_text: the text to display.
	item_command: the console command to execute when clicked.
	item_scale: the height of the text, in virtual pixels, so ideally 8 or more.

mitems_common.qc:class mitem_button : mitem
	centered text with a noticable border that just seems to say 'click me'...
	item_text: the text to display.
	item_command: the console command to execute when clicked.
	item_scale: the height of the text, in virtual pixels, so ideally 8 or more.

