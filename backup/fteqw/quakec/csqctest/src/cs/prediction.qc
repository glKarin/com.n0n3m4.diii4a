/*
UpdateLocalMovement: runs the local prediction (called from player.qc - local players are drawn for mirrors etc)
	out: vieworg (view origin, with chasecam/viewheight added)
	out: ` (raw player ent origin)

PlayerUpdated: updates internal state, called from player.qc any time we got an update from the server.
	in: self (the player ent origin/velocity/mins/maxs)

ResetPlayerPrediction: call if you broke the special pmove globals
*/

#define ERRORTIME 20
#define STEPTIME 8

vector player_org;
vector player_vel;
float player_pmflags;
float player_sequence;

float player_steptime;
float player_step;

vector pmove_error;
float pmove_errortime;

float pmove_step;
float pmove_steptime;
float pmove_step_oldz;

.float pmove_flags;

float pmoveframe;
nonstatic void(entity ent) Pred_ResetPlayerPrediction = 
{
//reset the pmove to lerp from the new position
	ent.origin = player_org;
	ent.velocity = player_vel;
	pmoveframe = player_sequence+1;	//+1 because the recieved frame has the move already done (server side)
	ent.pmove_flags = player_pmflags;

	if (pmoveframe < clientcommandframe-128)
		pmoveframe = clientcommandframe-128;	//avoid an infinate loop
};

//called from map.qc when a teleport trigger was predicted to be touched.
void(vector newteleorg, vector newtelevel, vector newteleang) Pred_Predict_Teleport =
{
	float teleframe;
	teleframe = pmoveframe;

	//if this is the first client frame that shows us moving into the teleporter
	if (teleframe == clientcommandframe-1)
	{
		//update the client's view angles
		setviewprop(33, newteleang);
		view_angles = newteleang;
	}
	other.origin = newteleorg;
	other.velocity = newtelevel;
	input_angles = newteleang;
};

void(entity ent, float endframe) Pred_RunMovement;

nonstatic void(entity ent) Pred_PlayerUpdated =
{
	local float noerror;
	local vector o;
	local vector v;
	local float pmf;

	o = ent.origin;
	v = ent.velocity;
	pmf = ent.pmove_flags;

	noerror = CVARF(cg_noerror);

	//reset the prediction to last-known-good state
	Pred_ResetPlayerPrediction(ent);
	Pred_RunMovement(ent, servercommandframe+1);

	player_step = pmove_step;
	player_steptime = pmove_steptime;

	//pull out the new values
	player_org = o;
	player_vel = v;
	player_sequence = servercommandframe;
	player_pmflags = pmf;

	if (noerror)
	{
		pmove_error = '0 0 0';
		pmove_errortime = time;

		Pred_ResetPlayerPrediction(ent);
	}
	else
	{
		Pred_RunMovement(ent, clientcommandframe); //make sure we're up to date
		o = ent.origin;	//save off the old for the teleport check below.

		//reset it, then update to now to guage how much our previous prediction was incorrect by
		Pred_ResetPlayerPrediction(ent);
		Pred_RunMovement(ent, clientcommandframe);

		if (vlen(o - ent.origin) > 64)
		{//teleport
			pmove_error = '0 0 0';
			pmove_errortime = time;
		}
		else
		{	//figure out the error ammount, and lerp back to it, without forgetting about any current inaccuracies.
			pmove_error = (pmove_errortime - time)*ERRORTIME * pmove_error + (o - ent.origin);
			if (vlen(pmove_error) < 1)
				pmove_error = '0 0 0';
			pmove_errortime = time + 1/ERRORTIME;
		}
	}
};

void(entity ent, float endframe) Pred_RunMovement =
{
	vector oorg;
	if (servercommandframe >= player_sequence+63)
	{
		//we're meant to be updating the player faster than this
		//hopefully its just that we're throttled...

		//you can comment out this block and the player will continue to be predicted locally. But its best to freeze them

		player_sequence = servercommandframe-63;

		return;
	}

	//rewind to our last-known-good position
	Pred_ResetPlayerPrediction(ent);

	if (getstatf(STAT_HEALTH) <= 0)
	{
		pmoveframe = clientcommandframe;
		//just update the angles
		if (!getinputstate(pmoveframe-1))
			dprint(sprintf("Bad sequence %g\n", pmoveframe-1));
		return;	//dead, so don't run prediction. :D
	}

	if (CVARF(cg_nopred))
	{
		pmoveframe = clientcommandframe;
		//just update the angles
		if (!getinputstate(pmoveframe-1))
			dprint(sprintf("Bad sequence %g\n", pmoveframe-1));
		input_angles = view_angles;
		return;
	}

//	if (getinputstate(pmoveframe-1))
//		ent.pmove_flags = PMF_JUMP_HELD;
	while(pmoveframe <= endframe)
	{
		if (!getinputstate(pmoveframe))
		{
			dprint(sprintf("Bad sequence %g\n", pmoveframe));
			break;
		}
		if (pmoveframe==clientcommandframe)
			CSQC_Input_Frame();	//manipulate it like we would if we actually sent it...
		oorg = ent.origin;
#ifdef OWNPLAYERPHYSICS
		PMove(ent);
#else
		runstandardplayerphysics(ent);
#endif
		if (input_buttons & ((pmoveframe == clientcommandframe)?128:64))
		{   //some debugging info, to match up to the client. server's sequence will have no gaps.
			print(sprintf("after input_sequence:%g(%g-%g), msecs:%g, aim:%v, move:%v%s\n", input_sequence, servercommandframe, clientcommandframe, input_timelength*1000, input_angles, input_movevalues, (pmoveframe == clientcommandframe)?" \spartial":""));
			print(sprintf("  origin: %v <- %v, velocity %v\n", ent.origin, oorg, ent.velocity));
		}

		pmoveframe++;
	}

	//add in anything that was applied after (for low packet rate protocols)
	input_angles = view_angles;
};

nonstatic void(entity ent) Pred_UpdateLocalMovement =
{
	local float viewheight;

	Pred_RunMovement(ent, clientcommandframe);

	if (ent.origin_z > pmove_step_oldz+8 && ent.origin_z < pmove_step_oldz+24 && ent.velocity_z == 0)
	{
		//evaluate out the remaining old step
		if (pmove_steptime - time > 0)
			pmove_step = (pmove_steptime - time)*STEPTIME*pmove_step;
		else
			pmove_step = 0;

		//work out the new step
		pmove_step += (pmove_step_oldz-self.origin_z);
		pmove_steptime = time + 1/STEPTIME;
	}
	pmove_step_oldz = ent.origin_z;

	//allow the user to move the viewheight down 6 units so it's at +16, where projectiles come from.
	viewheight = CVARF(v_viewheight);
	if (viewheight < -7)
		viewheight = -7;
	else if (viewheight > 7)
		viewheight = 7;

	vieworg = ent.origin;	//the default view height
	vieworg_z += getstatf(STAT_VIEWHEIGHT) + viewheight;

	//correct the view position to compensate for any errors, slowly over time, 0.1 seconds.
	if (pmove_errortime - time > 0)
		vieworg += (pmove_errortime - time)*ERRORTIME * pmove_error;

	if (!CVARF(cg_nostep))
		if (pmove_steptime - time > 0)
			vieworg_z += (pmove_steptime - time)*STEPTIME * pmove_step;

	if (chasecam)
	{
		view_angles_y += CVARF(cg_thirdPersonAngle);

		makevectors(view_angles);
		traceline(self.origin, vieworg - v_forward * CVARF(cg_thirdPersonRange)+v_up*CVARF(cg_thirdPersonHeight), TRUE, self);
		vieworg = trace_endpos + v_forward*8;
	}
};
