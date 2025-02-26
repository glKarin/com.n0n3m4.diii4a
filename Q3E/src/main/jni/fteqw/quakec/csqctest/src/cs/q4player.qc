enum
{
ANIM_FORWARD,
ANIM_BACK,
ANIM_LEFT,
ANIM_RIGHT,
ANIM_IDLE,
ANIM_MAX
};

float anim[ANIM_MAX];

.vector lastorg;

.float aweight[ANIM_MAX];
.float atime[ANIM_MAX];

.float rangle;

/*pick an angle for the legs to run in such that the movement is in 0, 90, 180, -90 angles from movement direction. attempt to clamp past a full circle*/
float() anglequadrant =
{
	float ideal, diff;

	if (!self.velocity_x && !self.velocity_y)
		ideal = self.angles_y;
	else
		ideal = vectoyaw(self.velocity);
	diff = ideal - self.angles_y;
	if (diff > 180)
		diff -= 360;
	if (diff < -180)
		diff += 360;

	if (diff < -180)
		return self.angles_y + 180;
	else if (diff > 180)
		return self.angles_y - 180;
	else if (diff < -150)
		return ideal + 180;
	else if (diff > 150)
		return ideal - 180;
	else if (diff > 75)
		return ideal - 90;
	else if (diff < -75)
		return ideal + 90;
	else
		return ideal;
};

void(float ideal, float speed) mychangeyaw =
{
	float move;
	move = ideal - self.angles_y;
	speed *= frametime;
	if (move > 180)
		move -= 360;
	if (move < -180)
		move += 360;
	if (move >= 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	self.angles_y = self.angles_y + move;
};

void() Q4PM_Draw =
{
	float fw, rt;
	float tweight;
	float i;
	float ideal;
	vector aimang;

	vector move = self.origin - self.lastorg;
	self.lastorg = self.origin;

	if (!self.skeletonindex)
	{
		self.skeletonindex = skel_create(self.modelindex, FALSE);
		if (!self.skeletonindex)
			return;
	}

	ideal = anglequadrant();
	aimang = [0, self.angles_y, self.angles_x*-3];

	self.angles = [0, self.rangle, 0];
	mychangeyaw(ideal, 360);
	self.rangle = self.angles_y;

	makevectors(self.angles);
	
	fw = (move * v_forward)/(320);
	rt = (move * v_right)/(320);

	aimang_y -= self.angles_y;
	aimang_x = 0;
	if (aimang_y > 180)
		aimang_y -= 360;
	if (aimang_y < -180)
		aimang_y += 360;
	makevectors(aimang*0.333);

	if (fw*fw > 0)
	{
		if (fw > 0)
			self.aweight[ANIM_FORWARD] += fw*32;
		else
			self.aweight[ANIM_BACK] -= fw*32;
	}
	if (fw > 0)
		self.atime[ANIM_FORWARD] += fw;
	else
		self.atime[ANIM_BACK] -= fw;
	if (rt*rt > 0)
	{
		if (rt > 0)
			self.aweight[ANIM_RIGHT] += rt*32;
		else
			self.aweight[ANIM_LEFT] -= rt*32;
	}
	if (rt > 0)
		self.atime[ANIM_RIGHT] += rt;
	else
		self.atime[ANIM_LEFT] -= rt;
	if (move_x == 0 && move_y == 0)
		self.aweight[ANIM_IDLE] += frametime*4;
	self.atime[ANIM_IDLE] += frametime;

	tweight = self.aweight[ANIM_FORWARD]+self.aweight[ANIM_BACK]+self.aweight[ANIM_LEFT]+self.aweight[ANIM_RIGHT]+self.aweight[ANIM_IDLE];
	if (tweight > 1)
	{
		tweight = 1/tweight;
		self.aweight[ANIM_FORWARD] *= tweight;
		self.aweight[ANIM_BACK] *= tweight;
		self.aweight[ANIM_LEFT] *= tweight;
		self.aweight[ANIM_RIGHT] *= tweight;
		self.aweight[ANIM_IDLE] *= tweight;
	}
	else
	{
		self.aweight[ANIM_IDLE] += 1-tweight;
		self.atime[ANIM_IDLE] += frametime;
	}

	self.origin_z -= 24;//self.mins_z;
	setmodel(self, self.model);
	for (i = 0; i < ANIM_MAX; i++)
	{
		self.frame = 0;
		self.frame2 = 0;
		self.lerpfrac = 0;
		self.frame1time = self.atime[i];
		self.frame2time = self.atime[i];
		skel_build(self.skeletonindex, self, anim[i], ((i==0)?0:1), 0, -1, self.aweight[i]);
	}

	skel_premul_bone(self.skeletonindex, 18, '0 0 0', v_forward, v_right, v_up);
	skel_premul_bone(self.skeletonindex, 19, '0 0 0', v_forward, v_right, v_up);
	skel_premul_bone(self.skeletonindex, 20, '0 0 0', v_forward, v_right, v_up);

	//obliterate the bone movement, so that it doesn't move forwards.
	skel_get_bonerel(self.skeletonindex, 1);
	skel_set_bone(self.skeletonindex, 1, '0 0 0', v_forward*(64/72), v_right*(64/72), v_up*(64/72));

	self.renderflags = 0;
	addentity(self);

	self.origin = self.lastorg;
};

float(string skinname) Q4PM_SetModel =
{
	string mname = strcat("models/characters/player/", skinname, ".md5mesh");

	//check to see if it exists
	if not (whichpack(mname))
	{
print(sprintf("File %s doesn't exist\n", mname));
		return false;
	}

	anim[ANIM_IDLE]    = getmodelindex("models/characters/player/idle.md5anim");
	anim[ANIM_FORWARD] = getmodelindex("models/characters/player/run.md5anim");
	anim[ANIM_BACK]    = getmodelindex("models/characters/player/run_backwards.md5anim");
	anim[ANIM_RIGHT]   = getmodelindex("models/characters/player/strafe_right.md5anim");
	anim[ANIM_LEFT]    = getmodelindex("models/characters/player/strafe_left.md5anim");

	setmodel(self, mname);

	self.rangle = self.angles_y;

	return true;
};

void() Q4PM_UnsetModel =
{
	if (self.skeletonindex)
		skel_delete(self.skeletonindex);
	self.skeletonindex = 0;
};

entity() Q4PM_DupModel
{
	return spawn();
};
