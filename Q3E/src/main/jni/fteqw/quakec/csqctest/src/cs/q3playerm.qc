//This file provides support for Q3-compatible player models.

/*
	This module provides:
		bool(string newmodelskin) Anim_SetModel
			This function applies the named model (eg: "sarge" or "sarge/red") to self.
			Returns false on failure. If it fails, the entity will be untouched.
		void() Anim_Draw
			This function adds self's segmented models to the scene.
			This is intended to be called from inside a predraw function.
		void() Anim_UnsetModel
			This frees everything assosiated with self, but Does not flush the animation file.
		entity() Anim_DupModel
			Duplicates the animation state/model from self entity to a newly spawned entity.
			This is expected to be used for bodyqueues.
		float(string fname) Anim_ReadAnimationFile
			An explicit call is not required, except as perhaps a precache.
			fname is eg: "sarge", but NOT "sarge/red".
			Returns -1 on failure.
			Fails if list is full or animation.cfg is not found. Does not fail if a model is not found.
		float() Anim_GetGender
			Returns which gender the 'self' player model is.
		float(string skinname) Anim_GetHeadModelIndex
			Returns a modelindex assosiated with the given model/skin name.
			Intended for the hud.
		float(string skinname) Anim_GetHeadSkinNumber
			Returns a skin number for use with the modelindex returned by Anim_GetHeadModelIndex to give the correct model/skin for the given model/skin parameter.
			Intended for the hud.
		vector(string skinname) Anim_GetHeadOffset
			Retrieves the q3 head offset for displaying the head in a nice neat box on the hud.

	if only I were allowed to use pointers... these three head functions would be a single more efficient function

*/

#ifdef MD3PMODELS

//these are the animation sequence names used in quake3.
enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,
	LEGS_TURN,

	NUMANIMS
};


//we use qc arrays to store all the information. this results in a lot of prog space used.
#define MAXMODELS 50	//if progs size becomes an issue, reduce this.

//a size optimisation.
#define anim_firstframe A
#define anim_numframes B
#define anim_loopingframes C
#define anum_framespersecond D

//a q3 animation.cfg file contains 4 items per sequence.
nosave float anim_firstframe[NUMANIMS*MAXMODELS];	//first frame in the sequence
nosave float anim_numframes[NUMANIMS*MAXMODELS];	//number of frames in this sequence.
nosave float anim_loopingframes[NUMANIMS*MAXMODELS];	//number of frames to play while looping (jumps to total-looping)
nosave float anum_framespersecond[NUMANIMS*MAXMODELS];//number of frames to play per second.

nosave float anim_gender[MAXMODELS];	//one of the SEX_ enum values.
nosave string anim_name[MAXMODELS];		//names the (skinless) player model, so they can be cached and shared.
nosave float anim_headmodel[MAXMODELS];
nosave vector anim_headoffset[MAXMODELS];

//note: q3 assumes male.
//we assume male too, due to the player model that is typically used if we have no q3 models.
#define GENDER_DEFAULT GENDER_MALE


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//This block of code is to replace builtins which may be missing in your engine of choice.
//Its all generic maths/tag stuff, so consider it just noise :)
//this doesn't affect how the model is built.

#ifdef WORKINDP
vector getlerpedtaginfo(entity ent, float tagnum)
{
	//personally I consider it a bug that this function is needed.
	//this function exactly duplicates the gettaginfo builtin
	//but ensures that interpolation based on frame2 happens.
	//this matches how the renderer will lerp frames.

	//at the time of writing, this is needed for DP to be smooth.
	//fte does not support gettaginfo at all (I've been lazy due to the heirachical nature).
	//this is called by rotatevectorsbytag_GTI, so it not required in fte as FTE has a working rotatevectorsbytag instead.

	float frame2ness = ent.lerpfrac;
	float frame1ness = 1-frame2ness;
	float f1=ent.frame;
	float f2=ent.frame2;
//it doesn't matter what the ent's lerpfrac currently is.
	vector f1o, f1f, f1r, f1u;
	vector f2o, f2f, f2r, f2u;
	vector v_origin;

	//make sure both frames are set, in case the builtin is fixed
	ent.frame = f1;
	ent.frame2 = f1;
	f1o = gettaginfo(ent, tagnum);
	f1r=v_right;f1f=v_forward;f1u=v_up;

	ent.frame = f2;
	ent.frame2 = f2;
	v_origin = gettaginfo(ent, tagnum);

	//restore the entity
	ent.frame = f1;
	ent.frame2 = f2;

	//lerp the current frame2 with the cached frame1 values
	v_forward = f1f*frame1ness+v_forward*frame2ness;
	v_right = f1r*frame1ness+v_right*frame2ness;
	v_up = f1u*frame1ness+v_up*frame2ness;
	v_origin = f1o*frame1ness+v_origin*frame2ness;

	return v_origin;
}

#define rotatevectorsbytag rotatevectorsbytag_GTI
vector rotatevectorsbytag_GTI(entity ent, float tagnum)
{
	vector saveang=ent.angles;
	vector saveorg=ent.origin;

	vector oldx=v_forward, oldy=('0 0 0'-v_right), oldz=v_up;
	vector oldo=ent.origin;

	ent.angles = '0 0 0';
	ent.origin = '0 0 0';
	vector ango=getlerpedtaginfo(ent, tagnum);
	ent.angles = saveang;
	ent.origin = saveorg;
//note: v_right is actually left, in tags.
	vector angx=v_forward, angy=v_right, angz=v_up;

	angx = normalize(angx);
	angy = normalize(angy);
	angz = normalize(angz);

	//multiply out the tag matrix with the previous matrix.
	v_forward_x = angx_x*oldx_x + angx_y*oldy_x + angx_z*oldz_x;
	v_forward_y = angx_x*oldx_y + angx_y*oldy_y + angx_z*oldz_y;
	v_forward_z = angx_x*oldx_z + angx_y*oldy_z + angx_z*oldz_z;
	v_right_x   = angy_x*oldx_x + angy_y*oldy_x + angy_z*oldz_x;
	v_right_y   = angy_x*oldx_y + angy_y*oldy_y + angy_z*oldz_y;
	v_right_z   = angy_x*oldx_z + angy_y*oldy_z + angy_z*oldz_z;
	v_up_x      = angz_x*oldx_x + angz_y*oldy_x + angz_z*oldz_x;
	v_up_y      = angz_x*oldx_y + angz_y*oldy_y + angz_z*oldz_y;
	v_up_z      = angz_x*oldx_z + angz_y*oldy_z + angz_z*oldz_z;

	v_right = '0 0 0'-v_right;

	//transform the tag's origin
	saveorg = oldo;
	saveorg_x += ango_x * oldx_x;
	saveorg_y += ango_x * oldx_y;
	saveorg_z += ango_x * oldx_z;
	saveorg_x += ango_y * oldy_x;
	saveorg_y += ango_y * oldy_y;
	saveorg_z += ango_y * oldy_z;
	saveorg_x += ango_z * oldz_x;
	saveorg_y += ango_z * oldz_y;
	saveorg_z += ango_z * oldz_z;
	return saveorg;
}
#define rotatevectorsbyangle rotatevectorsbyangle_QC
void rotatevectorsbyangle_QC(vector angle)
{
	vector oldx=v_forward, oldy='0 0 0'-v_right, oldz=v_up;
	angle_x = -angle_x;
	makevectors(angle);
	vector angx=v_forward, angy='0 0 0'-v_right, angz=v_up;

	v_forward_x = angx_x*oldx_x + angx_y*oldy_x + angx_z*oldz_x;
	v_forward_y = angx_x*oldx_y + angx_y*oldy_y + angx_z*oldz_y;
	v_forward_z = angx_x*oldx_z + angx_y*oldy_z + angx_z*oldz_z;
	v_right_x   = angy_x*oldx_x + angy_y*oldy_x + angy_z*oldz_x;
	v_right_y   = angy_x*oldx_y + angy_y*oldy_y + angy_z*oldz_y;
	v_right_z   = angy_x*oldx_z + angy_y*oldy_z + angy_z*oldz_z;
	v_up_x      = angz_x*oldx_x + angz_y*oldy_x + angz_z*oldz_x;
	v_up_y      = angz_x*oldx_y + angz_y*oldy_y + angz_z*oldz_y;
	v_up_z      = angz_x*oldx_z + angz_y*oldy_z + angz_z*oldz_z;

	v_right = '0 0 0'-v_right;
}
#define RotateVectorsByVectors RotateVectorsByVectorsDP

//
#define skinforname(i,n) 0
#endif

//end noisy maths.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//a utility function required by sexedsound so we don't try playing missing sounds (its common for male models to be missing some)
float(string str) fileexists =
{
	//we use searches because its more likly to be cached than reading a 200kb file from the disk every time.
	local float search;
	search = search_begin(str, false, true);
	if (search < 0)
	{
		return false;
	}
	search_end(search);
	return true;
};

//plays a CHAN_VOICE sound around the given entity.
void(entity ent, string soundname) sexedsound =
{
	string str, full;
	if (self == player_local)
		if (CVARF(cg_noselfsounds))
			return;	//option to disable sounds from local player

	str = strcat("player/", anim_name[self.modelnum], "/", soundname);
	full = strcat("sound/", str);
	if (fileexists(full))
	{
		precache_sound(str);
		sound(self, CHAN_VOICE, str, 1, 1, 95 + random()*10);
	}
	else
	{
		str = strcat("player/sarge/", soundname);	// :(
		precache_sound(str);
		sound(self, CHAN_VOICE, str, 1, 1, 95 + random()*10);
	}
};


//our player's frames changed to some new sequence.
//our animation function will animate acordingly.

static void(float anum) ForceToAnim =
{
	if (anum <= TORSO_STAND2)
	{
		self.torsoent.animnum = anum;
		self.torsoent.framechangetime = time;
	}
	if (anum <= BOTH_DEAD3 || anum >= LEGS_WALKCR)
	{
		self.legsent.animnum = anum;
		self.legsent.framechangetime = time;
	}
};

//this function is per-segment.
//it updates the frame/frame2/lerpfrac values and loops/caps the animation.
//returns true if the end of the current animation was reached.
float(entity ent) animate =
{
//true if looped
	float anum;
	float ft;
	float fnum;
	float numframes;
	float fps;
	float loopingframes;
	float ret;

	anum = (ent.modelnum * NUMANIMS) + ent.animnum;

	ft = time - ent.framechangetime;
	numframes = anim_numframes[anum];
	fps = anum_framespersecond[anum];

	ft *= fps;
	fnum = floor(ft);
	ft = ft - fnum;	//time into the frame.
	ret = fnum >= numframes;
	if (ret)
	{
		loopingframes = anim_loopingframes[anum];

		if (loopingframes)
		{
			fnum -= numframes - loopingframes;
			fnum = fnum - floor(fnum/loopingframes)*loopingframes;
			fnum += numframes - loopingframes;
		}
		else
		{	//stop at the end of it.
			fnum = numframes-1;
			ft = 1;
		}

		if (numframes == 1)
			ent.frame2 = ent.frame;
	}
	fnum += anim_firstframe[anum];

	if (ent.frame2 == -1)
	{	//special flag to make it snap for the initial frame
		ent.frame = fnum;
		ent.frame2 = fnum;
	}
	else if (ent.frame != fnum)
	{
		ent.frame2 = ent.frame;
		ent.frame = fnum;
	}
	ent.lerpfrac = 1-ft;
	if (ent.lerpfrac<=0)
		ent.frame2 = ent.frame;

	return ret;
};

.float wasinair;
//this function checks to see if the player is jumping/flying/dead/moving, and just updates the legs to match the current events and time.
void() LegsUpdateAnim =
{
	float inair;

	tracebox(self.origin, self.mins, self.maxs, self.origin-'0 0 2', FALSE, self);

	if (trace_fraction == 1)
		inair = self.velocity_z;
	else
		inair = 0;

	if (self.legsent.animnum > BOTH_DEAD3)
	if ((self.wasinair!=0) != (inair!=0))
	{
		if (trace_fraction == 1)	//in air
		{
			if (self.velocity_z > 5)
			{
				makevectors(self.angles);
				if (v_forward * self.velocity >= 0)
					ForceToAnim(LEGS_JUMP);
				else
					ForceToAnim(LEGS_JUMPB);
			}
		}
		else
		{
			if (inair < -5 || self.wasinair < -5)
			{
				if (self.animnum == LEGS_JUMPB)
					ForceToAnim(LEGS_LANDB);
				else
					ForceToAnim(LEGS_LAND);
			}
		}
		self.wasinair = inair;
	}

	if (vlen(self.velocity) > 0.1)
	{
		if (self.legsent.animnum == LEGS_IDLE)
		{
			if (self.velocity * v_forward > 0)
				ForceToAnim(LEGS_RUN);
			else
				ForceToAnim(LEGS_BACK);
		}
	}
	else
	{
		if (self.legsent.animnum == LEGS_RUN || self.legsent.animnum == LEGS_BACK)
			ForceToAnim(LEGS_IDLE);
	}

	if (animate(self.legsent))
	{
		if (self.legsent.animnum <= BOTH_DEAD3)
		{
			return;	//don't play the idle anim when dead.
		}
		if (self.wasinair)
		{
			return;	//don't change the legs anim whilst flying.
		}

		if (self.velocity_x || self.velocity_y)
		{
			if (self.velocity * v_forward > 0)
				ForceToAnim(LEGS_RUN);
			else
				ForceToAnim(LEGS_BACK);
		}
		else
		{
			ForceToAnim(LEGS_IDLE);
		}

		animate(self.legsent);
	}
};

//updates the torso's animations. basically just shoots/gestures/or otherwise just updates to the current time.
void() TorsoUpdateAnim =
{
	if (animate(self.torsoent))
	{
		playerframe f = self.frame;
		if (f >= playerframe::axdeth1 && f <= playerframe::deathe9)
			return;	//dead.

		if (f == playerframe::nailatt1 || f == playerframe::nailatt2 ||
		    f == playerframe::light1 || f == playerframe::light2)
			ForceToAnim(TORSO_ATTACK);	//these ones loop
		else if (random() < 0.005 && !(self.velocity_x || self.velocity_y) && (chasecam || self != player_local))
		{	//randomly taunt, when standing still, when not first-person (making the sounds is just confusing when first person)
			sexedsound(self, "taunt.wav");
			ForceToAnim(TORSO_GESTURE);
		}
		else if ((f >= playerframe::axrun1 && f <= playerframe::axrun6) ||
			 (f >= playerframe::axstnd1 && f <= playerframe::axstnd12) ||
			 (f >= playerframe::axpain1 && f <= playerframe::axpain6))
			ForceToAnim(TORSO_STAND2);
		else
			ForceToAnim(TORSO_STAND);
		animate(self.legsent);
	}
};

//adds a q3-shell around the model based on which powerups they have active.
//and just basically acts as a wrapper to addentity.
void(entity ent) AddModelWithEffects =
{
	if (self.sveffects & SVE_INVIS)
		ent.forceshader = shaderforname("powerups/invisibility");

	addentity(ent);
	if (self.sveffects & SVE_QUAD)
	{
		ent.fatness = -2;
		ent.forceshader = shaderforname("powerups/quad");
		addentity(ent);
	}
	if (self.sveffects & SVE_GOD)
	{
		ent.fatness = -2;
		ent.forceshader = shaderforname("powerups/regen");
		addentity(ent);
	}
	ent.fatness = 0;
	ent.forceshader = 0;
};

//this function is called inside your base entity's predraw function (which should then return false)
nonstatic void() Anim_Draw =
{
	vector tf, tr, tu;
	vector ang;
	float move;
	string weaponname = 0;

	self.modelindex = 0;	//should never have been anything else.


	if (player_local == self && !chasecam)
	{
		//we still add the local player entity, although only to mirrors.
		self.legsent.renderflags = RF_USEAXIS|RF_EXTERNALMODEL;
		self.torsoent.renderflags = RF_USEAXIS|RF_EXTERNALMODEL;
		self.headent.renderflags = RF_USEAXIS|RF_EXTERNALMODEL;
		self.weaponent.renderflags = RF_USEAXIS|RF_EXTERNALMODEL;
	}
	else
	{
		self.legsent.renderflags = RF_USEAXIS;
		self.torsoent.renderflags = RF_USEAXIS;
		self.headent.renderflags = RF_USEAXIS;
		self.weaponent.renderflags = RF_USEAXIS;
	}

	if (!(self.frame >= playerframe::axdeth1 && self.frame <= playerframe::deathe9))
	{
		//if they're still alive
		//animate the legs a bit so they turn to the player, but not for 1-degree turns. little more realism there.

		ang = [0,self.angles_y,0];
		if (self.velocity_x||self.velocity_y)
		{
			makevectors(ang);
			tf = self.velocity;
			tf_z = 0;
			tf = normalize(tf);
			if (v_forward*tf > -0.01)
				self.legsent.ideal_yaw = vectoyaw(tf);
			else
				self.legsent.ideal_yaw = vectoyaw(tf)+180;
		}
		else
		{
			move = ang_y - self.legsent.ideal_yaw;
			if (move < -180)
				move += 360;
			if (move > 180)
				move -= 360;

			if (move*move > 30*30)
				ForceToAnim(LEGS_TURN);

			if (self.legsent.animnum == LEGS_TURN || self.legsent.animnum == LEGS_JUMP)
				self.legsent.ideal_yaw = ang_y;
		}
		other = self;
		self = self.legsent;
		self.yaw_speed = 5;//180*frametime;

		//clamp the legs to never be more than 90 degrees
		move = other.angles_y-self.angles_y;
		if (move < -180)
			move += 360;
		if (move > 180)
			move -= 360;
		//turn the legs
		if (move*move > 95*95)	//snap
		{
			if (move>0)
				move=20;
			else
				move=-20;
			self.angles_y = other.angles_y - move;
			self = other;
		}
		else
			ChangeYaw();
		self = other;
	}
	

	if (self.frame != self.frame2)
	{
	//see if the player changed to any animations.
		if (self.frame >= playerframe::axdeth1 && self.frame <= playerframe::axdeth8 && self.legsent.animnum != BOTH_DEATH1)
		{
			sexedsound(self, "death1.wav");
			ForceToAnim(BOTH_DEATH1);
		}
		else if (self.frame == playerframe::axdeth9 && self.frame2 != playerframe::axdeth8)
			ForceToAnim(BOTH_DEAD1);	//suddenly appeared with this frame (otherwise we just flow into it)

		else if (self.frame == playerframe::deatha1 && self.frame <= playerframe::deatha10 && self.legsent.animnum != BOTH_DEATH2)
		{
			sexedsound(self, "death2.wav");
			ForceToAnim(BOTH_DEATH2);
		}
		else if (self.frame == playerframe::deatha11 && self.frame2 != playerframe::deatha10)
			ForceToAnim(BOTH_DEAD2);	//suddenly appeared with this frame (otherwise we just flow into it)

		else if (self.frame == playerframe::deathb1 && self.frame <= playerframe::deathb8 && self.legsent.animnum != BOTH_DEATH3)
		{
			sexedsound(self, "death3.wav");
			ForceToAnim(BOTH_DEATH3);
		}
		else if (self.frame == playerframe::deathb9 && self.frame2 != playerframe::deathb8)
			ForceToAnim(BOTH_DEAD3);	//suddenly appeared with this frame (otherwise we just flow into it)

		else if (self.frame == playerframe::deathc1 && self.frame <= playerframe::deathc14 && self.legsent.animnum != BOTH_DEATH1)
		{
			sexedsound(self, "death1.wav");
			ForceToAnim(BOTH_DEATH1);
		}
		else if (self.frame == playerframe::deathc15 && self.frame2 != playerframe::deathc14)
			ForceToAnim(BOTH_DEAD1);	//suddenly appeared with this frame (otherwise we just flow into it)

		else if (self.frame == playerframe::deathd1 && self.frame <= playerframe::deathd8 && self.legsent.animnum != BOTH_DEATH2)
		{
			sexedsound(self, "death2.wav");
			ForceToAnim(BOTH_DEATH2);
		}
		else if (self.frame == playerframe::deathd9 && self.frame2 != playerframe::deathd8)
			ForceToAnim(BOTH_DEAD2);	//suddenly appeared with this frame (otherwise we just flow into it)

		else if (self.frame == playerframe::deathe1 && self.frame <= playerframe::deathe8 && self.legsent.animnum != BOTH_DEATH3)
		{
			sexedsound(self, "death3.wav");
			ForceToAnim(BOTH_DEATH3);
		}
		else if (self.frame == playerframe::deathe9 && self.frame2 != playerframe::deathe8)
			ForceToAnim(BOTH_DEAD3);	//suddenly appeared with this frame (otherwise we just flow into it)

		else if ((self.frame  == playerframe::nailatt1 || self.frame  == playerframe::nailatt2) &&
			 (self.frame2 != playerframe::nailatt1 && self.frame2 != playerframe::nailatt2))
			ForceToAnim(TORSO_ATTACK);
		else if ((self.frame  == playerframe::light1 || self.frame  == playerframe::light2) &&
			 (self.frame2 != playerframe::light1 && self.frame2 != playerframe::light2))
			ForceToAnim(TORSO_ATTACK);
		else if (self.frame == playerframe::rockatt1)
			ForceToAnim(TORSO_ATTACK);
		else if (self.frame == playerframe::shotatt1)
			ForceToAnim(TORSO_ATTACK);
		else if (self.frame == playerframe::axatt1)
			ForceToAnim(TORSO_ATTACK2);
		else if (self.frame == playerframe::axattb1)
			ForceToAnim(TORSO_ATTACK2);
		else if (self.frame == playerframe::axattc1)
			ForceToAnim(TORSO_ATTACK2);
		else if (self.frame == playerframe::axattd1)
			ForceToAnim(TORSO_ATTACK2);
		else if (self.frame2 >= playerframe::axdeth1 && self.frame2 <= playerframe::deathe9 && !(self.frame >= playerframe::axdeth1 && self.frame <= playerframe::deathe9))
		{	//respawned
			ForceToAnim(LEGS_IDLE);
			ForceToAnim(TORSO_STAND);
		}
		self.frame2 = self.frame;
	}

	//add a dynamic light around the player if they have any powerups.
	ang = '0 0 0';
	if (self.sveffects & SVE_QUAD)
		ang_z = 1;
	if (self.sveffects & SVE_GOD)
		ang_x = 1;
	if (ang != '0 0 0')
	{
		dynamiclight_add(self.origin, 400, ang);
	}

	//if they're meant to be transparent, propogate that.
	self.legsent.alpha = self.torsoent.alpha = self.headent.alpha = self.weaponent.alpha = self.alpha;
	//LegsUpdateAnim needs its origin early, for stff
	self.legsent.origin = self.origin;

	//update the animation lerps
	makevectors(self.legsent.angles);
	LegsUpdateAnim();
	TorsoUpdateAnim();

	//add the legs into the scene, with their lagged angles.
	AddModelWithEffects(self.legsent);
	//legs are in place, matricies are set up for them.

	//work out where to put the torso by querying the tag
	self.torsoent.origin = rotatevectorsbytag(self.legsent, self.torsoent.tag_index);//figure out the torso position
	//torso's angles are not lagged, and must always point directly at the target, so rotate by the extra angles.
	ang = self.angles;
	ang_y = self.angles_y - self.legsent.angles_y;	//keep the angle on the legs
	ang_x*=2;
	if (self.legsent.animnum > BOTH_DEAD3)
		rotatevectorsbyangle(ang);	//rotate the torso (when dead the whole thing acts as one model with no custom angles inside)
	AddModelWithEffects(self.torsoent);
	//torso is now added to the scene.

	//save the direction we're looking in for the weapon which is added after the head.
	tf = v_forward;
	tr = v_right;
	tu = v_up;

	//now work out where to put the head
	self.headent.origin = rotatevectorsbytag(self.torsoent, self.headent.tag_index);//
	ang_y = sin(time)*22.5;
	ang_x = cos(time*0.4532)*11.25;
	if (self.legsent.animnum > BOTH_DEAD3)
		rotatevectorsbyangle(ang);	//make the head around a bit
	AddModelWithEffects(self.headent);
	//head is now in place

	if (self.frame >= playerframe::axdeth1 && self.frame <= playerframe::deathe9)
		return;	//don't show the weapon in death frames.

	//and revert the matrix back to how it was before the head.
	v_forward = tf;
	v_right = tr;
	v_up = tu;

	move = self.sveffects&15;
	switch (move)
	{
	case 12:	//axe
		weaponname = "models/weapons2/gauntlet/gauntlet";
		break;
	default:
	case 0:	//shotgun
		weaponname = "models/weapons2/railgun/railgun";	//well... the sniper's prefered weapon, at least. :p
		break;
	case 1:	//super shotgun
		weaponname = "models/weapons2/shotgun/shotgun";
		break;
	case 2:	//nailgun
		weaponname = "models/weapons2/machinegun/machinegun";
		break;
	case 3:	//super nailgun
		weaponname = "models/weapons2/plasma/plasma";
		break;
	case 4: //grenade launcher
		weaponname = "models/weapons2/grenadel/grenadel";
		break;
	case 5: //rocket launcher
		weaponname = "models/weapons2/rocketl/rocketl";
		break;
	case 6: //lightning gun
		weaponname = "models/weapons2/lightning/lightning";
		break;
	}

	setmodel(self.weaponent, strcat(weaponname, ".md3"));

	//rotate by a tag on the torso
	self.weaponent.origin = rotatevectorsbytag(self.torsoent, self.weaponent.tag_index);//place the weapon in the hand
	//and add it.
	AddModelWithEffects(self.weaponent);

	//this check is a little wrong.
	if (self.torsoent.animnum == TORSO_ATTACK || self.torsoent.animnum == TORSO_ATTACK2)
	{
		move = gettagindex(self.weaponent, "tag_flash");
		if (move)
		{
			//they're shooting something. make a muzzleflash appear at the end of their weapon.
			self.weaponent.origin = rotatevectorsbytag(self.weaponent, move);
			setmodel(self.weaponent, strcat(weaponname, "_flash.md3"));
			AddModelWithEffects(self.weaponent);
		}
	}
};

//remove our attached models, restoring the player model to being a boring player.
nonstatic void() Anim_UnsetModel =
{
	if (self.torsoent)
		remove(self.torsoent);
	if (self.headent)
		remove(self.headent);
	if (self.legsent)
		remove(self.legsent);
	if (self.weaponent)
		remove(self.weaponent);

	self.torsoent = world;
	self.headent = world;
	self.legsent = world;
	self.weaponent = world;
	self.modelnum = -1;

	setmodel(self, self.model);
};

nonstatic float() Anim_GetGender =
{
	if (self.headent)
		return anim_gender[self.headent.modelnum];
	return GENDER_DEFAULT;
};

//Attempts to read the animation file for the named q3 player model
//-1 on failure.
nonstatic float(string modname) Anim_ReadAnimationFile =
{
	local float modnum;
	local string str;
	local float file;
	local float sequencenum = 0;
	local float stupid;

	if (modname == "")
		return -1;

	for (modnum = 0; modnum < MAXMODELS; modnum++)
	{
		if (anim_name[modnum] == modname)
			return modnum;	//already loaded
		if (anim_name[modnum] == "")
			break;	//empty slot
	}
	if (modnum == MAXMODELS)
	{	//list is full
		print("Too many models\n");
		return -1;
	}
	else
	{
		str = strcat("models/players/", modname, "/animation.cfg");

		file = fopen(str, FILE_READ);
		if (file < 0)
		{
			print(sprintf("fopen %S failed\n", str));
			return -1;
		}

		modname= strzone(modname);
		anim_name[modnum] = modname;

		//precache them.
		precache_model(strcat("models/players/", modname, "/upper.md3"));
		precache_model(strcat("models/players/", modname, "/lower.md3"));
		precache_model(strcat("models/players/", modname, "/head.md3"));

		anim_headmodel[modnum] = getmodelindex(strcat("models/players/", modname, "/head.md3"));

		//default general values
		anim_gender[modnum] = GENDER_DEFAULT;
		anim_headoffset[modnum] = '0 0 0';

		for (;;)
		{
			str = fgets(file);
			if not (str) break;

			tokenize(str);
			str = argv(0);
			if (str == "")
				continue;
			else if (str == "sex")
			{
				str = argv(1);
				if (str == "m" || str == "M")
					anim_gender[modnum] = GENDER_MALE;
				else if (str == "f" || str == "F")
					anim_gender[modnum] = GENDER_FEMALE;
				else	//n or N
					anim_gender[modnum] = GENDER_NEUTER;
			}
			else if (str == "headoffset")
			{
				vector v;
				v_x = stof(argv(1));
				v_y = stof(argv(2));
				v_z = stof(argv(3));
				anim_headoffset[modnum] = v;
			}
			else if (str == "footsteps")
			{
				//we don't play footsteps anyway
			}
			else if (str2chr(str,0) >= '0' && str2chr(str,0) <= '9')
			{
				if (sequencenum == NUMANIMS)
					print("animation file \"" "models/players/", modname, "/animation.cfg" "\" contains too many animations\n");
				else
				{	
					if (sequencenum == LEGS_WALKCR)	//for some stupid reason, the leg frames start where the torso leaves
					{
						stupid = stof(str);
						stupid = stupid - anim_firstframe[(modnum * NUMANIMS) + TORSO_GESTURE];
					}
					else stupid = 0;
					anim_firstframe[(modnum * NUMANIMS) + sequencenum] = stof(str) - stupid;
					anim_numframes[(modnum * NUMANIMS) + sequencenum] = stof(argv(1));
					anim_loopingframes[(modnum * NUMANIMS) + sequencenum] = stof(argv(2));
					anum_framespersecond[(modnum * NUMANIMS) + sequencenum] = stof(argv(3));
					sequencenum++;
				}
			}
			else
				print("animation.cfg contains unrecognised token ", str, "\n");
		}

		fclose(file);

		if (sequencenum != NUMANIMS)
			print("animation.cfg is incompleate\n");
	}
	return modnum;
}

//attempts to apply a player model/skin to the given entity.
//this may load the configuration.cfg
//skinname is of the form: ranger/default
nonstatic float(string skinname) Anim_SetModel =
{
	local string lowermodelname;
	local string uppermodelname;
	local string headmodelname;

	local string lowerskinname;
	local string upperskinname;
	local string headskinname;

	local float lowermodnum;
	local float uppermodnum;
	local float headmodnum;

	local float slashpos;

	tokenize(skinname);
	lowermodelname = argv(2);
	uppermodelname = argv(1);
	headmodelname = argv(0);

	slashpos = strstrofs(lowermodelname, "/");
	lowerskinname = substring(lowermodelname, slashpos+1, -1);
	lowermodelname = substring(lowermodelname, 0, slashpos);

	slashpos = strstrofs(uppermodelname, "/");
	upperskinname = substring(uppermodelname, slashpos+1, -1);
	uppermodelname = substring(uppermodelname, 0, slashpos);

	slashpos = strstrofs(headmodelname, "/");
	headskinname = substring(headmodelname, slashpos+1, -1);
	headmodelname = substring(headmodelname, 0, slashpos);

	//seeing as we support loading each part from a different player model (well, q3 does)
	//we load it three times.
	lowermodnum = Anim_ReadAnimationFile(lowermodelname);
	uppermodnum = Anim_ReadAnimationFile(uppermodelname);
	headmodnum = Anim_ReadAnimationFile(headmodelname);

	//all failed
	if (lowermodnum < 0 && uppermodnum < 0 && headmodnum < 0)
		return false;	//failed to load the animation.

	//make sure that all three parts are valid or something.
	if (headmodnum < 0)
	{
		if (lowermodnum < 0)
		{
			headmodnum = uppermodnum;
			headmodelname = uppermodelname;
			headskinname = upperskinname;
		}
		else
		{
			headmodnum = lowermodnum;
			headmodelname = lowermodelname;
			headskinname = lowerskinname;
		}
	}
	if (lowermodnum < 0)
	{
		lowermodnum = headmodnum;
		lowermodelname = headmodelname;
		lowerskinname = headskinname;
	}
	if (uppermodnum < 0)
	{
		uppermodnum = headmodnum;
		uppermodelname = headmodelname;
		upperskinname = headskinname;
	}

	//spawn the attaching ents
	if (!self.torsoent)
		self.torsoent = spawn();
	if (!self.headent)
		self.headent = spawn();
	if (!self.legsent)
		self.legsent = spawn();
	if (!self.weaponent)
		self.weaponent = spawn();

	//give them the correct model
	setmodel(self.legsent, strcat("models/players/", anim_name[lowermodnum], "/lower.md3"));
	setmodel(self.torsoent, strcat("models/players/", anim_name[uppermodnum], "/upper.md3"));
	setmodel(self.headent, strcat("models/players/", anim_name[headmodnum], "/head.md3"));

//	precache_model(AXEMODELNAME);
//	precache_model(WEAPONMODELNAME);


	self.legsent.owner = self;
	self.headent.owner = self;
	self.torsoent.owner = self;
	self.weaponent.owner = self;

	self.drawmask = MASK_NORMAL;	//general view.

	//find the tags so we don't do it every single frame
	self.torsoent.tag_index = gettagindex(self.legsent, "tag_torso");
	self.headent.tag_index = gettagindex(self.torsoent, "tag_head");
	self.weaponent.tag_index = gettagindex(self.torsoent, "tag_weapon");

	self.modelnum = headmodnum;
	self.legsent.modelnum = lowermodnum;
	self.headent.modelnum = headmodnum;
	self.torsoent.modelnum = uppermodnum;
	self.weaponent.modelnum = lowermodnum;

	self.legsent.colormap = self.colormap;
	self.headent.colormap = self.colormap;
	self.torsoent.colormap = self.colormap;
	self.weaponent.colormap = self.colormap;

	if (!lowerskinname)
		lowerskinname = "default";
	if (!upperskinname)
		upperskinname = "default";
	if (!headskinname)
		headskinname = "default";

	//find which skin number to use for the given skin file
	if (stof(lowerskinname))
		self.legsent.skin = stof(lowerskinname);
	else
		self.legsent.skin = skinforname(self.legsent.modelindex, strcat("models/players/", anim_name[lowermodnum], "/lower_", lowerskinname, ".skin"));
	if (stof(upperskinname))
		self.torsoent.skin = stof(upperskinname);
	else
		self.torsoent.skin = skinforname(self.torsoent.modelindex, strcat("models/players/", anim_name[uppermodnum], "/upper_", upperskinname, ".skin"));
	if (stof(headskinname))
		self.headent.skin = stof(headskinname);
	else
		self.headent.skin = skinforname(self.headent.modelindex, strcat("models/players/", anim_name[headmodnum], "/head_", headskinname, ".skin"));

	//setup the initial sequences.
	ForceToAnim(LEGS_IDLE);
	ForceToAnim(TORSO_STAND);

	//so it takes effect fully and immediatly... well, the first time its drawn, anyway.
	self.legsent.framechangetime = -50;
	self.torsoent.framechangetime = -50;
	self.legsent.frame2 = -1;
	self.torsoent.frame2 = -1;

	self.frame2 = -1;
	self.lerpfrac = 0;

	return true;
};

entity(entity src) CloneModel =
{
	local entity dest;
	dest = spawn();

	dest.modelnum = src.modelnum;
	dest.animnum = src.animnum;
	dest.colormap = src.colormap;
	dest.tag_index = src.tag_index;
	dest.skin = src.skin;
	dest.predraw = src.predraw;
	dest.frame = src.frame;
	dest.frame2 = src.frame2;
	dest.model = src.model;
	dest.modelindex = src.modelindex;
	dest.drawmask = src.drawmask;
	dest.origin = src.origin;
	dest.angles = src.angles;
	dest.mins = src.mins;
	dest.maxs = src.maxs;
	
	return dest;
};

nonstatic entity() Anim_DupModel =
{
	local entity o, n;
	o = self;
	n = self = CloneModel(o);

	self.legsent = CloneModel(o.legsent);
	self.headent = CloneModel(o.headent);
	self.torsoent = CloneModel(o.torsoent);
	self.weaponent = CloneModel(o.weaponent);
	self.velocity = o.velocity;


	self = o;
	return n;
};

nonstatic float(string skinname) Anim_GetHeadModelIndex =
{
	float slashpos;
	string modelname;
	float modnum;

	tokenize(skinname);
	modelname = argv(0);
	if (modelname == "")
		return 0;	//an invalid modelindex.

	slashpos = strstrofs(modelname, "/");
	modelname = substring(modelname, 0, slashpos);

	//seeing as we support loading each part from a different player model (well, q3 does)
	//we load it three times.
	modnum = Anim_ReadAnimationFile(modelname);
	if (modnum < 0)
		return 0;

	return anim_headmodel[modnum];
};

nonstatic float(string skinname) Anim_GetHeadSkinNumber =
{
	float slashpos;
	string modelname;
	float modnum;

	tokenize(skinname);
	modelname = argv(0);
	if (modelname == "")
		return 0;	//0 = default

	slashpos = strstrofs(modelname, "/");
	skinname = substring(modelname, slashpos+1, -1);
	modelname = substring(modelname, 0, slashpos);

	if (stof(skinname))
		return stof(skinname);

	//seeing as we support loading each part from a different player model (well, q3 does)
	//we load it three times.
	modnum = Anim_ReadAnimationFile(modelname);
	if (modnum < 0)
		return 0;

	if (!skinname)
		skinname = "default";

	return skinforname(anim_headmodel[modnum], strcat("models/players/", modelname, "/head_", skinname, ".skin"));
};

nonstatic vector(string skinname) Anim_GetHeadOffset =
{
	float slashpos;
	string modelname;
	float modnum;

	tokenize(skinname);
	modelname = argv(0);
	if (modelname == "")
		return '0 0 0';	//an invalid modelindex.

	slashpos = strstrofs(modelname, "/");
	modelname = substring(modelname, 0, slashpos);

	//seeing as we support loading each part from a different player model (well, q3 does)
	//we load it three times.
	modnum = Anim_ReadAnimationFile(modelname);
	if (modnum < 0)
		return '0 0 0';
	return anim_headoffset[modnum];
}



static float(float channel, string soundname, vector pos, float vol, float attenuation, float flags) ServerSoundStartRequest =
{	//the server started a sound on an entity that the csqc has control over.
	if (!self.headent)
	{
		return false;
	}

	if (soundname == "player/plyrjmp8.wav")
	{
		if (self == player_local)
			if (CVARF(cg_noselfjumpsound))
				return true;	//option to disable jump noise for local player.
		sexedsound(self, "jump1.wav");
		return true;
	}
	if (soundname == "player/gasp1.wav")
	{
		sexedsound(self, "gasp.wav");
		return true;
	}
	if (soundname == "player/gasp2.wav")
	{
		sexedsound(self, "gasp.wav");
		return true;
	}
	if (soundname == "player/land.wav")
	{
		sexedsound(self, "fall1.wav");
		return true;
	}
	if (soundname == "player/land2.wav")
	{
		sexedsound(self, "fall1.wav");
		return true;
	}

	if (soundname == "player/pain1.wav")
	{
		sexedsound(self, "pain25_1.wav");
		return true;
	}
	if (soundname == "player/pain2.wav")
	{
		sexedsound(self, "pain50_1.wav");
		return true;
	}
	if (soundname == "player/pain3.wav")
	{
		sexedsound(self, "pain75_1.wav");
		return true;
	}
	if (soundname == "player/pain4.wav")
	{
		sexedsound(self, "pain100_1.wav");
		return true;
	}
	if (soundname == "player/pain5.wav")
	{
		sexedsound(self, "pain75_1.wav");
		return true;
	}
	if (soundname == "player/pain6.wav")
	{
		sexedsound(self, "pain100_1.wav");
		return true;
	}

	if (soundname == "player/h2odeath.wav")
	{
		sexedsound(self, "drown.wav");
		return true;
	}
	if (soundname == "player/death1.wav")	//normal deaths come from the player animations.
		return true;
	if (soundname == "player/death2.wav")
		return true;
	if (soundname == "player/death3.wav")
		return true;
	if (soundname == "player/death4.wav")
		return true;
	if (soundname == "player/death5.wav")
		return true;

	return false;
};

//version that breaks when the ent is not necessarily known to us, called only when 'self' is a known CSQC entity.
float(float channel, string soundname, vector org, float vol, float attenuation, float flags) CSQC_ServerSound =
{
	return ServerSoundStartRequest(channel, soundname, org, vol, attenuation, flags);
}
//version that lets the qc handle any ent fixups.
float(float sventnum, float channel, string soundname, float vol, float att, vector org, float pitchmod, float flags) CSQC_Event_Sound =
{
	if (autocvar(developer, 0))
		print(sprintf("CSQC_Event_Sound(%S): chan:%g\n", soundname, channel));

	self = findfloat(world, entnum, sventnum);
	if (self)
		return ServerSoundStartRequest(channel, soundname, org, vol, att, flags);
	return false;
};
#endif
