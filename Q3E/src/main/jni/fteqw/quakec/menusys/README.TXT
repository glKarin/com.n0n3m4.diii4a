This directory contains my menusys mod (with menusys qc library).
This mod is provided to serve as a base mod for menuqc or menus within csqc.


License:
There is no explicit license attached to this code. This means you're free to do what you want with it, however...
1) there is absolutely no warrenty at all. it works for me, but might not even compile for you.
2) I make no assurances that small sections of code, comments, or constants, or really anything, wasn't inadvertantly copied from a GPL source. I'm not aware of anything not otherwise obvious from looking at a config or just running the game (ie: cvar/setting names). Really I'm just trying to cover my arse here.
3) people that don't share their code are selfish, and deserve their inevitable HD crashes! yay backups :)



Quick Start:
To add a new item onto the menus, you can open up the relavent menu/*.qc file, find an item of the same widget type, and dupe then edit that line for your new item, following the example given.
There's often a little extra logic used to center the menu vertically, so you may also need to tweak some numbers before anything is added.
You should NOT need to edit menusys/*.qc - you can if you want, but I wouldn't recommend it to beginners.
To add a new menu, open your .src file, find the concommandslist macro and add an extra line following the example given. This provides you with console command -> function mappings (as well as including the right qc file, if specified - hurrah for each menu being its own qc file). Then you can just copy+paste one of my menus and tweak it a little.

Some more detailed info on menusys:
The menusys library is heirachical. It uses the concept of a 'desktop' object as the root node (in csqc, this would normally be expected to provide the draw calls for the actual game).
Then top-level windows/menus are embedded upon that desktop. And then sliders and buttons etc are normally embedded in those windows/menus. Note that you could directly embed your widgets on the desktop, but this can make using the mouse awkward as there's no easy way to release the mouse so you can interact with things.
Menusys also uses inheritance. Thus, you can easily create your own 'frame' object that inherits from mitem_frame, and thereby acts as a container for other widgets (for example).


DP compatibility:
This mod should also work in DP, however there are some exceptions.
Things not supported by DP obviously cannot be used with DP. This includes 3d models in menuqc, server browsers in csqc, etc.
My personal focus is on FTE, and thus you will likely want to research DP cvars in order to include any additional DP-only cvars that you may wish to include.
I've used a variable called 'dp_workarounds' in many places in order to work around the DP bugs / incompatibilities that are obvious enough for me to notice.
Feel free to provide diffs to fix anything that you notice (make sure they work in FTE too though).


MenuQC:
The presence of menu.dat causes the engine's built-in menus to become inaccessible. This means that any mod wishing to provide a single extra option on the menus needs to rewrite the entire thing from scratch...
And menuqc is somewhat mythical as a result - hopefully this mod will challenge that perception by making it slightly more accessible.


CSQC:
The CSQC version of this mod provides a 'full' set of the menus equivelent to the menuqc ones. This typically means that using both becomes redundant. Thus you may well wish to remove most of the menus from csqc so that you can use only the menuqc ones. If you want in-game menus like class customisation then you will likely want those menus in csqc in order to more easily communicate with your server.
Alternatively, if you set pr_csqcformenus to 1 in your default.cfg, then FTE will use your csprogs.dat instead of any menu.dat. This allows you to implement ALL your menus within a single module, which you may find to be a simplification. Note that this does not bypass cheat protections, and thus the csqc version will still generally not work on public servers. However, if you're making a stand-alone mod, this means that you can completely disregard menu.dat module (NOTE: this is only an option in FTE, not DP).
If your entire game is strictly single player, then look up my PureCSQC mod some time, which does not involve ssqc or even a server at all. Using PureCSQC as a base mod may help some headaches, however you'll need to merge the entry points yourself somehow (PureCSQC incorporates its own partial menu system more as an example of 2d input/drawing than actual menus).
When both modules are loaded, the csqc receives escape events and thus invokes its own menu. Because the menus are linked via console commands, you can easily implement the in-game menu in csqc, and all other menus in menuqc.


Your mod:
If you're creating a separate mod with its own special defaults then it is recommended to provide your own default.cfg
Your default.cfg should not only contain bind commands for your default key settings, and defaults for engine cvars, but you should also use the set or seta commands for your own cvars.
(note that if you use '-v -v' on fteqcc's commandline, it will generate a full autocvar listing from each module, including default values and some descriptions - just change any you wish to be flagged for archiving to use seta instead of set).


Keybinds:
One common thing modders want is new keybinds.
you can either open menus/options_keys.qc and edit the array there.
Alternatively, you can create a bindlist.lst file with <command> <description> quoted pairs on each line. If the command is a hyphen then its treated as a caption.
If there are too many keys, the frame menuitem thing will add a scrollbar for you, so go ahead and define a hundred different keys.


A Mod Options Menu:
The easy way to add a menu is to just copy+paste an existing one, like the video menu.
First, open up the appropriate src file and add a line like the following in the place that should be obvious:
cmd("m_mod",		M_Options_Mod,	menu/options_mod.qc)	\
That should create the console command for you, associate that with a function, AND include your menu's qc file into the progs.
Now copy menu/options_video.qc to the obvious name, and start editing it.
Okay, these lines:
	mitem_exmenu m;
	m = spawn(mitem_exmenu, item_text:_("Video Options"), item_flags:IF_SELECTABLE, item_command:"m_options");
	desktop.add(m, RS_X_MIN_PARENT_MIN|RS_Y_MIN_PARENT_MIN | RS_X_MAX_PARENT_MAX|RS_Y_MAX_PARENT_MAX, '0 0', '0 0');
	desktop.item_focuschange(m, IF_KFOCUSED);
	m.totop();
creates a fullscreen menu thing, they add it into the desktop, they give focus to it, and they make sure that it appears on top of everything else.
the spawn call is some special class stuff, with named field initialisations. The 'Video Options' thing isn't actually visible, but hey. The m_options string says which console command to issue when the menu is closed (by right-click or escape), this allows you to return to the previous menu.
And the following lines:
	float h = 200 * 0.5;
	mitem_pic banner = spawn(mitem_pic, item_text:"gfx/p_option.lmp", item_size_y:24, item_flags:IF_CENTERALIGN);
	m.add(banner, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_OWN_MIN|RS_Y_MAX_PARENT_MID, [(160-160-banner.item_size_x)*0.5, -h-32], [banner.item_size_x, -h-8]);
adds a static image. woo.
The RS_ constants are 'ReSize' constants. They say how to calculate the items top-left and bottom-right positions whenever the item is added or its parent is resized. The two vectors are additional offsets from the reference points that the RS flags give.
In this case, it just centers the image above some 200qu-high area in the middle of the screen.
And these lines are fun:
	mitem_frame fr = spawn(mitem_frame, item_flags: IF_SELECTABLE, frame_hasscroll:TRUE);
	m.add(fr, RS_X_MIN_PARENT_MID|RS_Y_MIN_PARENT_MID | RS_X_MAX_PARENT_MID|RS_Y_MAX_OWN_MIN, [-160, -h], [160, h*2]);
they an invisible 'frame' that then holds the widgits that you're actually going to interact with.
While that sounds a little useless at first, this provides two things: 1) if there are two many items, the frame_hasscroll value causes it to include a scrollbar, allowing the items to be scrolled without affecting any of the artwork attached to the menu itself (like that title image). and 2) child items are clipped to this frame.
This line:
	addmenuback(m);
quite simply just creates a widgit on the menu that darkens the background (insertion order affects draw order, this should be last-ish, so that it is drawn BEFORE any of the other widgits, such that it darkens everything other than the menu.
Lines like this:
	fr.add(menuitemcombo_spawn(_("Video Width"),	"vid_width",					'280 8', _(
						"0	\"Default\" "
						"640	\"640\" "
						"800	\"800\" "
						"1024	\"1024\" "
						"1280 \"1280\" "
						"1920	\"1920\" "
						)),	fl, [0, pos], [0, 8]); pos += 8;
just adds a single widgit into the frame's area (its the same add method as before, with the same RS_ flags in the fl argument), so I only really need to explain menuitemcombo_spawn here.
menuitemcombo_spawn's first argument is the text to put next to it. The _("") weirdness provides some internationalisation thing that I'm far too lazy to go into right now (ask divverent or something for tools to use this properly).
The second argument is the 'cvar' that its attached to (actually, you can subclass the parent item and handle the get+set events yourself).
The third is the size of the widget. Generally they should be wide enough for both the text and the value. And about 8 vpixels high.
And finally, the fourth argument is the list of possible values, and the text that should be shown if that value is selected. Easy, right?
Alternatively, this line:
	fr.add(menuitemslider_spawn(_("View Size"),	"viewsize",				'50 120 10', '280 8'),	fl, [0, pos], [0, 8]); pos += 8;
spawns a 'slider' opject. Its basically the same as combos, except that the 3rd argument is the mix, max, and step-size values. Note that min+max can be inverted if you want (you should also negate the graduation in this case).
And this line:
	fr.add(menuitemcheck_spawn(_("Show Framerate"),	dp("showfps", "show_fps"), 	'280 8'),	fl, [0, pos], [0, 8]); pos += 8;
is how you add a checkbox. There is no 'third' argument in this case, just that fourth (okay, so it does have a third, but shush).
The 'dp' function you see there is part of how my code handles cvar differences between DP and other engines. If cvar_type reports that the first cvar exists, then it uses that. Otherwise it uses the second. Simple.
Unfortunately this only detects whether a cvar exists or not, and can't handle different values very well, which is what the vid_fullscreen hack is all about. Yes, that sort of thing can be really awkward...
And finally, in order to add a link to your new menu from the main or options menu or whereever, just open the qc file for that menu and copy one of the add+spawn lines to provide a textitem that invokes your menu's console command when clicked.
Easy.


Server browser:
QC-based server browsers have a significant limitation - they have no access to the system clipboard. You can't copy+paste ip numbers etc.
This is why I've decided to just utilise FTE's normal server browser even with menuqc active. This isn't an option in DP unfortunately, and your mod might be sufficiently different for the engine's to not work too well. Or you might want to exclude other servers...
If you want to tie your server browser to a specific gamedir, for instance, you can find the MOD_GAMEDIR define, set it to something, and enable it.
My code uses lots of preprocessor tricks too, sorry about that. You'll get used to them eventually, and hopefully understand why I used them too!...
You'll likely need to polish it quite a lot before its truely friendly.
If you do polish it, feel free to send me a patch (assuming it's not too specific to your mod).
