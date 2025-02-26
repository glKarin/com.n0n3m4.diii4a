entity tfdetect;

.float items_allowed;
.string broadcast;

entity(float no) FindItem;

//#define dprint(x)

float(entity trigger, entity blame) triggercantouch =
{
	if (blame.classname != "player")
	{
//		dprint("Not a player\n");
		return false;
	}
	if (trigger.team_no)
		if (blame.team_no != trigger.team_no)
		{
//			dprint("Wrong team\n");
			return false;
		}
	if (trigger.items_allowed)
		if ((FindItem(trigger.items_allowed)).owner != blame)
		{
//			dprint("missing an item\n");
			return false;
		}

	return true;
};



/*
TeamFortress maps will be automatically detected by the TeamFortress patch
if you put an entity in the map with a classname of "info_tfdetect".
This makes TeamFortress automatically turn on the FORTRESSMAP toggleflag,
and turn on teamplay.

Also, once the TeamFortress map has been detected, the patch will look
for any spawnpoints dedicated to teams (see below).
If it finds any, it will look for the highest team number that is used.
Once found, it will limit anybody attempting to join a team to that 
number.
At the moment, Maps are limited to a maximum of 4 teams.
E.g. If the highest team number used by a Team Spawnpoint is 3, then
	 the patch will only allow players to join teams 1, 2, and 3.


The detection entity can also do the following:
	- specify a version string in the "broadcast" variable.
	  This will be compared with the version string for the TeamFortress
	  patch. If the don't match, a warning will be displayed to the
	  player that their patch and the map are incompatible.
	  The version string is in the format as follows:
		TeamFortress v2.1
	  The string is case sensitive, so make sure you've got it right.
	  Don't put a \n on the end of it.

	- Set the state of the toggleflags when the map starts up. The
	  value of the toggleflags should be in the "impulse" variable.
          The bits for the toggleflags are:

    Bit 1 (1)   :   Off - ClasSkin              , On - Multiskin
    Bit 2 (2)   :   Off - ClassPersistence Off  , On - ClassPersistence On
    Bit 3 (4)   :   Off - CheatChecking Off     , On - CheatChecking On
    Bit 4 (8)   :   Off - FortressMap Off       , On - FortressMap On
    Bit 5 (16)  :   Off - RespawnDelay Off      , On - RespawnDelay (See below)
    Bit 6 (32)  :   Off - RespawnDelay Off      , On - RespawnDelay (See below)
    Bit 7 (64)  :   Off - AutoTeam Off          , On - AutoTeam On
	Bit 8 (128)	:	Off - Individual Frags		, On - Frags = TeamScore

        N.B. FortressMap will be set On automatically by the the
             Detection entity anyway, so just ignore that Bit.

		N.B. The RespawnDelay settings takes 2 bits. The value of both of
			 them determines the level of respawn delay, as follows:
				Bit 5  		Bit 6		Result
				 Off		 Off		No Respawn delays
				 On			 Off		5 Second respawn delay
				 Off		 On			10 Second respawn delay
				 On			 On			20 Second respawn delay

	- Specify a string which is then "localcmd"ed. This allows you
	  to automatically set gravity, friction, etc.
	  The string should be stored in the "message" variable.

	- Put a limit on the number of lives each player has. When a player
	  runs out of lives, they are stuck in observer mode for the rest
	  of the level.
	  Lives for each player depend on the setting for the team they
	  belong to. The number of lives players of each team be should
	  be specified in the following variables:
	  	Team 1	:	"ammo_shells"
	  	Team 2	:	"ammo_nails"
	  	Team 3	:	"ammo_rockets"
	  	Team 4	:	"ammo_cells"
	  If the value of any team is 0, then the players of that team get
	  infinite lives.

	- Specify any playerclass that is _not_ allowed on this map
	  for each particular team.
	  The bits of the four variables are used for this.
	  The variables are as follows:
	  	Team 1	:	"maxammo_shells"
	  	Team 2	:	"maxammo_nails"
	  	Team 3	:	"maxammo_rockets"
	  	Team 4	:	"maxammo_cells"
	  Also, the "playerclass" variable can be used to restrict classes
	  for all teams.
	  The bits for all the variables are as follows:
      	Bit 1 (1)	:   No Scout            
        Bit 2 (2)	:	No Sniper        
        Bit 3 (4)	:	No Soldier       
        Bit 4 (8)	:	No Demolitions Man
        Bit 5 (16)	:	No Combat Medic
        Bit 6 (32)	:	No Heavy Weapons Guy
        Bit 7 (64)	:	No Pyro
        Bit 8 (128)	:	No Random PlayerClass
		Bit 9 (256) :	No Spy
		Bit 10(512) :	No Engineer
	  E.g. If the "maxammo_nails" variable is set to 3, then players 
	       in Team 2 will be unable to play Scouts or Snipers.

	  Finally, if you want the Team to be a special team for a fancy
	  map, such as the President in President-Quake, then you can 
	  restrict the team to only play Civilian Class. Do this by setting
	  the team's variable to "-1".

Notes:
	When using the "message" variable to set do localcmd's, you
	can issue more than one command by seperating them with \n
	Make sure you end it with a \n too.
	E.g. The following changes the gravity and the friction.
		"message" "sv_gravity 200\nsv_friction .5\n"
*/


void() info_tfdetect =
{
	tfdetect = self;

	//broadcast contains the version of tf that it was designed for
	if (self.broadcast != "")
	{
		dprint("map was designed for ");
		dprint(self.broadcast);
		dprint("\n");
	}

	//message states a message to localcmd
	//gravity, sv_friction, etc.
	localcmd(self.message);
	localcmd("\n");

	//ammo_shells
	//ammo_nails
	//ammo_rockets
	//ammo_cells
	//these fields specify the number of lives the players of any team have.

	//maxammo_shells
	//maxammo_nails
	//maxammo_rockets
	//maxammo_cells
	//these fields specify which teams are allowed which classes


	//team_broadcast - text for choose-team menu
	//non_team_broadcast - text for map help

	//noise1
	//noise2
	//noise3
	//noise4
	//these fields are per-team messages for choosing class.
};

string nextmap;
void() execute_changelevel;

.float goal_no;
.float group_no;
.float goal_results;
.float goal_activation;
.float owned_by;

.string team_broadcast;
.string owner_team_broadcast;
.string non_team_broadcast;

void(float t, float o, entity ignore, string toteam, string toowners, string toenemies) teambroadcast =
{
	local entity p;
	while(1)
	{
		p = find(p, classname, "player");
		if (!p)
			return;

		if (p != ignore)
		{
			if (p.team_no == t)
				sprint(p, PRINT_HIGH, toteam);
			else if (p.team_no == o)
				sprint(p, PRINT_HIGH, toowners);
			else
				sprint(p, PRINT_HIGH, toenemies);
		}
	}
};


entity(float num) FindGoal =
{
	local entity e;
	do
	{
		e = find(e, classname, "info_tfgoal");
		if (e.goal_no == num)
			return e;
	} while(e);

	return world;
};
entity(float num) FindItem =
{
	local entity e;
	do
	{
		e = find(e, classname, "item_tfgoal");
		if (e.goal_no == num)
			return e;
	} while(e);

	return world;
};

void(entity goal) goal_remove =
{
	goal.state = -1;
};

void(entity goal) goal_restore =
{
	goal.state = 0;
};

void(entity goal, entity ap) goal_inactivate =
{
	setmodel(goal, goal.mdl);
	setorigin(goal, goal.oldorigin);
	self.solid = SOLID_TRIGGER;
	goal.movetype = MOVETYPE_TOSS;
//dprint("inactivated\n");
	goal.owner = world;
	goal.state = 0;
	goal.nextthink = 0;
}

void() goal_timeout =
{
//dprint("goal_timeout\n");
	goal_inactivate(self, world);
};

void(entity player, entity goal) TakeGoalFromPlayer;
void(entity goal, entity ap) goal_activate;
void() goaldropped =
{
//dprint("goaldropped\n");
	TakeGoalFromPlayer(self, self.owner);
};

void() GoalTrackCarrier =
{
	if (self.owner.health <= 0)
	{
		goal_inactivate(self, self.owner);
		self.think = goaldropped;
		self.nextthink = time + 40;
	}
	else
		self.nextthink = time + 0.1;
};

void(entity goal, entity player) TakeGoalFromPlayer =
{
//dprint("TakeGoalFromPlayer\n");
	if (goal.owner != player)
		return;

	goal_inactivate(goal, goal.owner);
};
void(entity goal, entity player) GiveGoalToPlayer =
{
	if (goal.owner != world)
	{
		if (goal.owner == player)
			return;
		TakeGoalFromPlayer(goal.owner, goal);
	}

	goal.owner = player;
	setmodel(goal, "");
	goal.movetype = 12;//MOVETYPE_FOLLOW;
//dprint("given ");
//dprint(goal.netname);
//dprint(" to ");
//dprint(player.classname);
//dprint("\n");

	goal_activate(goal, player);

	goal.think = GoalTrackCarrier;
	goal.nextthink = time+0.1;
};

void(entity goal, entity ap) goal_activate =
{

	if (goal.state != 0)
		return;	//active already or removed.


	sprint(ap, PRINT_HIGH, goal.message);
	teambroadcast(goal.team_no, goal.owned_by, ap, goal.team_broadcast, goal.owner_team_broadcast, goal.non_team_broadcast);
	sound (other, CHAN_VOICE, self.noise, 1, ATTN_NORM);

	goal.state = 1;

	if (goal.goal_results & 4)
	{
		nextmap = "$host_mapname";
		execute_changelevel();
	}

//activated goals shouldn't affect players if (!(goal_result&2))
//activated goals don't trigger others if goal_result&8

	if (goal.classname == "item_tfgoal")
		GiveGoalToPlayer(goal, ap);
	else if (ap != world)
	{
//		if (goal.items)
//			GiveGoalToPlayer(FindItem(goal.items), ap);
		if (goal.axhitme)
			TakeGoalFromPlayer(FindItem(goal.axhitme), ap);
	}
/*
	if (goal.activate_goal_no)
		goal_activate(FindGoal(goal.activate_goal_no));
	if (goal.inactivate_goal_no)
		goal_inactivate(FindGoal(goal.inactivate_goal_no));
	if (goal.remove_goal_no)
		goal_remove(FindGoal(goal.remove_goal_no));
	if (goal.restore_goal_no)
		goal_restore(FindGoal(goal.restore_goal_no));
*/
//goal_results & 16 takes away stealth status.

	ap.frags = ap.frags + goal.frags;
	ap.health = ap.health + goal.health;

	if (goal.wait == -1)
		goal.nextthink = 0;	//permanent
	else if (goal.wait > 0)
	{
		goal.nextthink = time + goal.wait;
		setmodel(goal, "");
	}
	else	//instant.
	{
		goal.nextthink = 0;
		goal_timeout();
		return;
	}
	goal.think = goal_timeout;

	if (goal.goal_results & 1)
		goal_remove(goal);
};

void() goal_touch =
{
	local float act;
	if (self.owner != world)
		return;
	if (self.state != 0)
		return;
	if (!(other.flags & FL_CLIENT))
		return;

	if (!(self.goal_activation & 1))	//you're only allowed to touch if 1
		return;

//	dprint(self.netname);
//	dprint(":\n");

	act = triggercantouch(self, other);
	if (self.classname == "item_tfgoal")
	{
		if ((self.goal_activation & 64))//64 inverts weather to act
			act = 1 - act;
	}
	else
	{
		if ((self.goal_activation & 4))	//4 inverts weather to act
			act = 1 - act;
	}

	if (act) goal_activate(self, other);
};

void() StartItem;
void() info_tfgoal =
{
	self.oldorigin = self.origin;
	if (self.mdl != "")
	{
		precache_model(self.mdl);
		setmodel(self, self.mdl);
		setsize(self, '-16 -16 -24', '16 16 32');
	}
	if (self.noise != "")
		precache_sound(self.noise);

	self.touch = goal_touch;

	StartItem();
};
void() item_tfgoal =
{
	self.wait = -1;
	info_tfgoal();
};

void() info_tfgoal_timer =
{
};

void() i_t_g = 
{
	self.classname = "info_tfgoal";
	info_tfgoal();
};
void() i_t_t = 
{
	self.classname = "info_tfgoal_timer";
	info_tfgoal_timer();
};






void() info_player_teamspawn =
{
	if (self.team_no == 1)
		self.classname = "spawn1";
	else if (self.team_no == 2)
		self.classname = "spawn2";
	else if (self.team_no == 3)
		self.classname = "spawn3";
	else if (self.team_no == 4)
		self.classname = "spawn4";
	else
	{
		dprint("info_player_teamspawn with team_no ");
		dprint(ftos(self.team_no));
		dprint("\n");
		self.classname = "info_player_deathmatch";
	}
};






/*
	- Since quake has a limit on the size of the entity data in a map,
	  abbreviations for some of the common entity fields were created.
	  The Abbreviations are as follows:
*/

void() i_p_t = 
{
	self.classname = "info_player_teamspawn";
	info_player_teamspawn();
};


/*
var .float g_a = goal_activation;
var .float g_e = goal_effects;

var .float h_i_g = has_item_from_group"
var .float r_i_g = remove_item_group"

var .float a_s = ammo_shells"
var .float a_n = ammo_nails"
var .float a_r = ammo_rockets"
var .float a_c = ammo_cells"

var .float rv_s_h = remove_spawngroup"
var .float rs_s_h = restore_spawngroup"
var .float rv_gr = remove_group_no"
var .float rs_gr = restore_group_no"
var .float rv_g = remove_goal_no"
var .float rs_g = restore_goal_no"


var .string t_s_h = team_str_home;
var .string t_s_m = team_str_moved;
var .string t_s_c = team_str_carried;
var .string n_s_h = non_team_str_home;
var .string n_s_m = non_team_str_moved;
var .string n_s_c = non_team_str_carried;

var .string b_b = broadcast;
var .string b_t = team_broadcast;
var .string b_n = non_team_broadcast;
var .string b_o = owners_team_broadcast;
var .string n_b = netname_broadcast;
var .string n_t = netname_team_broadcast;
var .string n_n = netname_non_team_broadcast;
var .string n_o = netname_owners_team_broadcast;

var .string d_t = team_drop;
var .string d_n = non_team_drop;
var .string d_n_t = netname_team_drop;
var .string d_n_n = netname_non_team_drop;
*/

//pointless entity
.string map_name;
.string alias;
.string realteam;
.float impulse_value;
void() map_candidate =
{
	remove(self);
};




