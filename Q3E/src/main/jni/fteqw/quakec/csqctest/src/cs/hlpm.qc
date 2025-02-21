#ifdef HLPMODELS

.float oldframe;

#pragma noref 1
//FTE_CSQC_BASEFRAME
.float basebone;
.float baseframe;
.float baseframe2;
.float baseframe1time;
.float baseframe2time;
.float subblendfrac;
.float baselerpfrac;

//FTE_CSQC_HALFLIFE_MODELS
.float bonecontrol1, bonecontrol2, bonecontrol3, bonecontrol4;

#pragma noref 0

.float starttime;

static void(string framename) ForceToAnim =
{
	float a, s;
	float newframe;

	self.basebone = gettagindex(self, "Bip01 Spine");	//use this bone to split base/normal frames

	newframe = frameforname(self.modelindex, framename);
	self.frame = newframe;
	self.frame2 = newframe;

	newframe = frameforname(self.modelindex, "run2");
	self.baseframe = newframe;
	self.baseframe2 = newframe;
	self.baselerpfrac = 0;

	self.baseframe1time = time;

	if (self.velocity_x == 0 && self.velocity_y == 0)
	{
		a = 0;
		s = 0;
	}
	else
	{
		a = self.angles_y - vectoyaw(self.velocity);
		s = vlen(self.velocity);
		if (s < 100)
			a *= s/100;
	}
	s /= 400;
	if (a < -180)
		a += 360;
	if (a > 180)
		a -= 360;

	if (a > 120)
		a = 120;
	if (a < -120)
		a = -120;
	self.bonecontrol1 = self.bonecontrol2 = self.bonecontrol3 = self.bonecontrol4 = (a)/150;///4;
	self.angles_y -= a;

	self.baseframe1time += (time - self.starttime)*s;
	self.starttime = time;

	self.subblendfrac = self.angles_x*(1/15);
	self.angles_x = 0;

	//halflife players were made for bigger maps.
	self.scale = 52/72.0;
	self.origin_z += 12;
};

void() updatesequences =
{
	if (self.frame == playerframe::nailatt1 || self.frame == playerframe::nailatt2 ||
	    self.frame == playerframe::light1 || self.frame == playerframe::light2)
		ForceToAnim ("ref_shoot_onehanded");	//these ones loop
//	else if (random() < 0.005 && !(self.velocity_x || self.velocity_y) && (chasecam || self != player_local))
//	{	//randomly taunt, when standing still, when not first-person (making the sounds is just confusing when first person)
//		sexedsound(self, "taunt.wav");
//		ForceToAnim (TORSO_GESTURE);
//	}
	else if ((self.frame >= playerframe::axrun1 && self.frame <= playerframe::axrun6) ||
		 (self.frame >= playerframe::axstnd1 && self.frame <= playerframe::axstnd12) ||
		 (self.frame >= playerframe::axpain1 && self.frame <= playerframe::axpain6))
		ForceToAnim ("ref_aim_crowbar");
	else
		ForceToAnim ("ref_aim_onehanded");
}

void() HLPM_Draw =
{
	setmodel(self, self.model);
	updatesequences();
};

float(string skinname) HLPM_SetModel =
{
	string mname = strcat("models/player/", skinname, "/", skinname, ".mdl");

	//check to see if it exists (this is too slow for my liking really)
	if (whichpack(mname) == "")
		return false;

	setmodel(self, mname);

	self.oldframe = -1;
	self.scale = 20/72.0;

	return true;
};

void() HLPM_UnsetModel =
{
	self.scale = 0;
};

entity() HLPM_DupModel =
{
	return spawn();
};
#endif
