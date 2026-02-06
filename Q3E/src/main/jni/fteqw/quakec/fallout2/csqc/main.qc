entity viewentity;
float viewmodelindex;

.float lerptime;

void() DoThatViewModelThing =
{
	float newframe, newmodel;

	newframe = getstati(STAT_WEAPONFRAME);
	newmodel = getstati(STAT_WEAPONMODEL);

	if (newmodel != viewmodelindex)
	{	//changed entirly.
		viewmodelindex = newmodel;
		self.frame2 = self.frame = newframe;
		self.lerptime = time;
	}
	else if (newframe != self.frame)
	{
		self.frame2 = self.frame;
		self.frame = newframe;
		self.lerptime = time;
	}
	self.lerpfrac = 1-(time-self.lerptime)*10;

	if (self.lerpfrac < 0) self.lerpfrac = 0;


	self.modelindex = viewmodelindex;
};

void() CSQC_Init =
{
	viewentity = spawn();
	viewentity.predraw = DoThatViewModelThing;
	viewentity.drawmask = MASK_NORMAL;
	viewentity.renderflags = RF_VIEWMODEL | RF_DEPTHHACK;


	FigureOutButtons();
};



void(float do2d) CSQC_UpdateView =
{
	float width,height;
	float usehud;
	width = cvar("vid_conwidth");
	height = cvar("vid_conheight");

	clearscene();

	if (0)//!cvar("cg_usehud"))
	{
		setviewprop(VF_DRAWENGINESBAR, 0);
		usehud = true;
	}
	else
	{
		setviewprop(VF_DRAWENGINESBAR, 1);
	}

	setviewprop(VF_DRAWCROSSHAIR, 1);

	//force fullscreen views.
	setviewprop(VF_MIN, '0 0 0');
	setviewprop(VF_SIZE_X, width);
	setviewprop(VF_SIZE_Y, height);

	addentities(MASK_ENGINE | MASK_NORMAL);

	renderscene();

//	if (usehud)
//		Hud_Draw();

	if (show_inventory)
		Invent_Draw();
};