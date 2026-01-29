mitem_desktop thedesktop;
void(mitem_desktop desktop) M_Pop =
{
	mitem it = desktop.item_kactivechild;
	if (it)
		it.item_remove();
};
void(mitem_desktop desktop) M_ToggleMenu =
{
	mitem it = desktop.item_kactivechild;
	if (it)
		it.item_remove();
	else
		M_Main(desktop);
};


float(string str) CSQC_ConsoleCommand =
{
	local float args;
	args = tokenize(str);

	switch(argv(0))
	{
#define cmd(n,fnc,inc) case n: fnc(thedesktop); return TRUE;
	concommandslist
#undef cmd
	default:
		return FALSE;
	}
	return TRUE;
};

float(float isnew) updateplayer =
{
	self.drawmask = 1;
	if (self.entnum == player_localentnum)
		self.renderflags = RF_EXTERNALMODEL;
	else
		self.renderflags = 0;
	return TRUE;
};
//void(float apilevel, string enginename, float engineversion) CSQC_Init =
//{
//	deltalisten("progs/player.mdl", updateplayer, 0);
//};

float (float event, float parama, float paramb, float devid) CSQC_InputEvent =
{
	if (!thedesktop)
		return event!=IE_KEYUP;
	if (items_keypress(thedesktop, event, parama, paramb, devid))
		return TRUE;
#ifdef CSQC_SIMPLE
	if (event == IE_KEYDOWN && parama == K_ESCAPE)
	{
		M_Main(thedesktop);
		return TRUE;
	}
#endif
	return FALSE;
};

#ifdef CSQC_SIMPLE
//simplecsqc doesn't give us access to the 3d scene.
//instead we have hud+scoreboard entrypoints.
//we're a menu, so we use the scoreboard/overlay entrypoint (one that's skipped when a menu is shown).
//we do need to implement a real scoreboard though. :(
void(vector virtsize, float showscores) CSQC_DrawScores =
{
	items_draw(thedesktop, virtsize);
};
#define mydesktop mitem_desktop
#else
/*The desktop object will not normally draw anything, but you can get the desktop object to do the drawing by overriding its 'drawgame' method.
The primary advantage of doing the drawing this way is that the menu system can properly handle mouse positions in 3d space with multiple views. The menu system also handles splitscreen efficiently. Note that the menu system will handle clearing the scene and adding entities before this function is called.
You could instead draw the game then draw the menusystem over the top, if you're more comfortable with that.
*/
class mydesktop : mitem_desktop
{
	virtual void(float seat, vector minpos, vector size) drawgame =
	{
		setproperty(VF_DRAWENGINESBAR, TRUE);
/*
		entity playerent = findfloat(world, entnum, player_localentnum);
		if (playerent)
		{
			vector vorg = playerent.origin - playerent.gravitydir*getstat(STAT_VIEWHEIGHT);
			vector vang = playerent.v_angle;
			setproperty(VF_ORIGIN, vorg);
			setproperty(VF_ANGLES, vang);

			makevectors(vang);
			SetListener(vorg, v_forward, v_right, v_up, playerent.waterlevel==3);
		}
*/

		renderscene();
	};
};

void(float width, float height, float do2d) CSQC_UpdateView =
{
	items_draw(thedesktop, [width, height]);
};
//void(float width, float height, float do2d) CSQC_UpdateView_Loading = CSQC_UpdateView;
#endif	



//var float autocvar_dp_workarounds_allow = TRUE;
//var float autocvar_dp_workarounds_force = FALSE;

void(float apilevel, string enginename, float engineversion) CSQC_Init =
{
	dprint(sprintf("CSQC: Running on \"%s\" ver=%g, api=%g\n", enginename, engineversion, apilevel));

	FingerprintEngine();

	//make sure the engine knows what commands we want to handle
#define cmd(n,fnc,inc) registercommand(n);
	concommandslist
#undef cmd

	thedesktop = spawn(mydesktop);
#ifdef CSQC_SIMPLE
	Hud_Init();	//make sure the hud images are precached properly, so there's no stalls.
#endif
};