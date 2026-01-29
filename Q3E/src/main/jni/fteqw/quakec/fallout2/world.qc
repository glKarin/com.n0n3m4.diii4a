
void() InitBodyQue;

void() sci_think;
void() hostage_pain;
void() hostage_think;
void() hostage_die;
void(entity stuff) spawn_zombie;
void() GameControl;
void() VoteBoy;

void () create_referees =
{
	newmis = spawn ();
	newmis.owner = self;
	newmis.movetype = MOVETYPE_NONE;
	setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);
	newmis.solid = SOLID_BBOX;
	newmis.velocity = VEC_ORIGIN;
	newmis.touch = SUB_Null;
	setorigin (newmis, '0 0 -300');
	newmis.nextthink = (time + WEAPON_SHOTGUN);
	newmis.think = GameControl;
	newmis.classname = "referee";
	newmis = spawn ();
	newmis.owner = self;
	newmis.movetype = MOVETYPE_NONE;
	setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);
	newmis.solid = SOLID_BBOX;
	newmis.velocity = VEC_ORIGIN;
	newmis.touch = SUB_Null;
	setorigin (newmis, '0 0 -300');
	newmis.nextthink = (time + MULTICAST_PVS_R);
	newmis.think = VoteBoy;
	newmis.classname = "voteboy";
	newmis.vote_time = (time + MULTICAST_PVS_R);
};

void() main =
{
	dprint ("main function\n");
	
// these are just commands the the prog compiler to copy these files

	precache_file ("progs.dat");
	precache_file ("gfx.wad");
	precache_file ("quake.rc");
	precache_file ("default.cfg");

	precache_file ("end1.bin");
	precache_file2 ("end2.bin");

	precache_file ("demo1.dem");
	precache_file ("demo2.dem");
	precache_file ("demo3.dem");

//
// these are all of the lumps from the cached.ls files
//
	precache_file ("gfx/palette.lmp");
	precache_file ("gfx/colormap.lmp");

	precache_file2 ("gfx/pop.lmp");

	precache_file ("gfx/complete.lmp");
	precache_file ("gfx/inter.lmp");

	precache_file ("gfx/ranking.lmp");
	precache_file ("gfx/vidmodes.lmp");
	precache_file ("gfx/finale.lmp");
	precache_file ("gfx/conback.lmp");
	precache_file ("gfx/qplaque.lmp");

	precache_file ("gfx/menudot1.lmp");
	precache_file ("gfx/menudot2.lmp");
	precache_file ("gfx/menudot3.lmp");
	precache_file ("gfx/menudot4.lmp");
	precache_file ("gfx/menudot5.lmp");
	precache_file ("gfx/menudot6.lmp");

	precache_file ("gfx/menuplyr.lmp");
	precache_file ("gfx/bigbox.lmp");
	precache_file ("gfx/dim_modm.lmp");
	precache_file ("gfx/dim_drct.lmp");
	precache_file ("gfx/dim_ipx.lmp");
	precache_file ("gfx/dim_tcp.lmp");
	precache_file ("gfx/dim_mult.lmp");
	precache_file ("gfx/mainmenu.lmp");
	
	precache_file ("gfx/box_tl.lmp");
	precache_file ("gfx/box_tm.lmp");
	precache_file ("gfx/box_tr.lmp");
	
	precache_file ("gfx/box_ml.lmp");
	precache_file ("gfx/box_mm.lmp");
	precache_file ("gfx/box_mm2.lmp");
	precache_file ("gfx/box_mr.lmp");
	
	precache_file ("gfx/box_bl.lmp");
	precache_file ("gfx/box_bm.lmp");
	precache_file ("gfx/box_br.lmp");
	
	precache_file ("gfx/sp_menu.lmp");
	precache_file ("gfx/ttl_sgl.lmp");
	precache_file ("gfx/ttl_main.lmp");
	precache_file ("gfx/ttl_cstm.lmp");
	
	precache_file ("gfx/mp_menu.lmp");
	
	precache_file ("gfx/netmen1.lmp");
	precache_file ("gfx/netmen2.lmp");
	precache_file ("gfx/netmen3.lmp");
	precache_file ("gfx/netmen4.lmp");
	precache_file ("gfx/netmen5.lmp");
	
	precache_file ("gfx/sell.lmp");
	
	precache_file ("gfx/help0.lmp");
	precache_file ("gfx/help1.lmp");
	precache_file ("gfx/help2.lmp");
	precache_file ("gfx/help3.lmp");
	precache_file ("gfx/help4.lmp");
	precache_file ("gfx/help5.lmp");

	precache_file ("gfx/pause.lmp");
	precache_file ("gfx/loading.lmp");

	precache_file ("gfx/p_option.lmp");
	precache_file ("gfx/p_load.lmp");
	precache_file ("gfx/p_save.lmp");
	precache_file ("gfx/p_multi.lmp");

// sounds loaded by C code
	precache_sound ("misc/menu1.wav");
	precache_sound ("misc/menu2.wav");
	precache_sound ("misc/menu3.wav");

	precache_sound ("ambience/water1.wav");
	precache_sound ("ambience/wind2.wav");

// shareware
	precache_file ("maps/start.bsp");

	precache_file ("maps/e1m1.bsp");
	precache_file ("maps/e1m2.bsp");
	precache_file ("maps/e1m3.bsp");
	precache_file ("maps/e1m4.bsp");
	precache_file ("maps/e1m5.bsp");
	precache_file ("maps/e1m6.bsp");
	precache_file ("maps/e1m7.bsp");
	precache_file ("maps/e1m8.bsp");

// registered
	precache_file2 ("gfx/pop.lmp");

	precache_file2 ("maps/e2m1.bsp");
	precache_file2 ("maps/e2m2.bsp");
	precache_file2 ("maps/e2m3.bsp");
	precache_file2 ("maps/e2m4.bsp");
	precache_file2 ("maps/e2m5.bsp");
	precache_file2 ("maps/e2m6.bsp");
	precache_file2 ("maps/e2m7.bsp");

	precache_file2 ("maps/e3m1.bsp");
	precache_file2 ("maps/e3m2.bsp");
	precache_file2 ("maps/e3m3.bsp");
	precache_file2 ("maps/e3m4.bsp");
	precache_file2 ("maps/e3m5.bsp");
	precache_file2 ("maps/e3m6.bsp");
	precache_file2 ("maps/e3m7.bsp");

	precache_file2 ("maps/e4m1.bsp");
	precache_file2 ("maps/e4m2.bsp");
	precache_file2 ("maps/e4m3.bsp");
	precache_file2 ("maps/e4m4.bsp");
	precache_file2 ("maps/e4m5.bsp");
	precache_file2 ("maps/e4m6.bsp");
	precache_file2 ("maps/e4m7.bsp");
	precache_file2 ("maps/e4m8.bsp");

	precache_file2 ("maps/end.bsp");

	precache_file2 ("maps/dm1.bsp");
	precache_file2 ("maps/dm2.bsp");
	precache_file2 ("maps/dm3.bsp");
	precache_file2 ("maps/dm4.bsp");
	precache_file2 ("maps/dm5.bsp");
	precache_file2 ("maps/dm6.bsp");
};


void () GameControl =
{
	local float pcount, pdead;
	local entity te;

	te = find(world, classname, "player");
	while (te)
	{

		if (te.health > 0)
			pcount = pcount + 1;
		if (te.health <= 0)
			pdead = pdead + 1;

		te = find(te, classname, "player");
	}

	if (pcount == 0 && pdead > 0) //everyone died
	{
		if (endgame_timer == 0)
			bprint(2, "^4** ^bTHE PLAYERS WERE SLAIN^b **\n");
		if (endgame_timer == 1)
			bprint(2, "^4** ^bTHIS AREA WILL RESTART^b **\n");
		if (endgame_timer == 7)
			bprint(2, "^4** 3 **\n");
		if (endgame_timer == 8)
			bprint(2, "^4** 2 **\n");
		if (endgame_timer == 9)
			bprint(2, "^4** 1 **\n");
		if (endgame_timer == 10)
			localcmd("restart\n");

		sound (world, CHAN_BODY, "misc/menu3.wav", 1, ATTN_NONE);
	}

	if (pcount == 0 && pdead > 0)
		endgame_timer = endgame_timer + 1;

	self.think = GameControl;
	self.nextthink = time + 1;
};

void () GameTimer =
{
	local entity te, ze;
	local float switched;
	local float c, z;


	switched = 0;

	te = findradius(self.origin, 25000);
	while (c < 300 && switched == 0)
	{
		if (te.classname == "raider" && te.enemy != world && te.processed == 0 && te.active == 0 && switched == 0)
		{
			te.active = 1;
			te.processed = 1;
			bprint(2, "it is now ");
			bprint(2, te.netname);
			bprint(2, "'s turn.\n");
			te.maxspeed = 300;
			switched = 1;

			ze = findradius(self.origin, 25000);
			z = 0;
			while (z < 300)
			{
				if ((ze.classname == "player" || ze.classname == "raider") && ze != te)
				{
					ze.active = 0;
					ze.maxspeed = 0;
				}
				z = z + 1;
				ze = ze.chain;
			}
		}
		if (te.classname == "player" && te.processed == 0 && te.active == 0 && switched == 0)
		{
			te.active = 1;
			te.processed = 1;
			bprint(2, "it is now ");
			bprint(2, te.netname);
			bprint(2, "'s turn.\n");
			te.maxspeed = 300;
			switched = 1;

			ze = findradius(self.origin, 25000);
			z = 0;
			while (z < 300)
			{
				if ((ze.classname == "player" || ze.classname == "raider") && ze != te)
				{
					ze.active = 0;
					ze.maxspeed = 0;
				}
				z = z + 1;
				ze = ze.chain;
			}
		}
		c = c + 1;
		te = te.chain;
	}

	c = 0;
	if (switched == 0) // no entities left, turn is over
	{
		bprint(2, "round is now over.\n");

		te = findradius(self.origin, 25000);
		while (c < 300)
		{
			if (te.classname == "player" || te.classname == "raider")
			{
				te.active = 0;
				te.processed = 0;
				te.maxspeed = 0;
			}

			c = c + 1;
			te = te.chain;
		}
	}

	self.think = GameTimer;
	self.nextthink = time + 1;
};

void () create_timer =
{
	newmis = spawn ();
	newmis.owner = self;
	newmis.movetype = MOVETYPE_NONE;
	setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);
	newmis.solid = SOLID_BBOX;
	newmis.velocity = VEC_ORIGIN;
	newmis.touch = SUB_Null;
	setorigin (newmis, '0 0 -300');
	newmis.nextthink = (time + 1);
	newmis.think = GameTimer;
	newmis.classname = "referee";
};

entity  lastspawn;

//=======================
/*QUAKED worldspawn (0 0 0) ?
Only used for the world entity.
Set message to the level name.
Set sounds to the cd track to play.

World Types:
0: medieval
1: metal
2: base
*/
//=======================
void() worldspawn =
{
	lastspawn = world;
	InitBodyQue ();
	coop = !cvar("deathmatch");
	create_referees();
	SetupStats();

// custom map attributes

	if (self.model == "maps/e1m8.bsp")
		cvar_set ("sv_gravity", "100");
	else
		cvar_set ("sv_gravity", "800");



// the area based ambient sounds MUST be the first precache_sounds

// player precaches     
	W_Precache ();                  // get weapon precaches

// sounds used from C physics code
	precache_sound ("demon/dland2.wav");            // landing thud
	precache_sound ("misc/h2ohit1.wav");            // landing splash

// setup precaches allways needed
	precache_sound ("items/itembk2.wav");           // item respawn sound
	precache_sound ("player/plyrjmp8.wav");         // player jump
	precache_sound ("player/land.wav");                     // player landing
	precache_sound ("player/land2.wav");            // player hurt landing
	precache_sound ("player/drown1.wav");           // drowning pain
	precache_sound ("player/drown2.wav");           // drowning pain
	precache_sound ("player/gasp1.wav");            // gasping for air
	precache_sound ("player/gasp2.wav");            // taking breath
	precache_sound ("player/h2odeath.wav");         // drowning death

	precache_sound ("misc/talk.wav");                       // talk
	precache_sound ("player/teledth1.wav");         // telefrag
	precache_sound ("misc/r_tele1.wav");            // teleport sounds
	precache_sound ("misc/r_tele2.wav");
	precache_sound ("misc/r_tele3.wav");
	precache_sound ("misc/r_tele4.wav");
	precache_sound ("misc/r_tele5.wav");
	precache_sound ("weapons/lock4.wav");           // ammo pick up
	precache_sound ("weapons/pkup.wav");            // weapon up
	precache_sound ("items/armor1.wav");            // armor up
	precache_sound ("weapons/lhit.wav");            //lightning
	precache_sound ("weapons/lstart.wav");          //lightning start
	precache_sound ("items/damage3.wav");

	precache_sound ("misc/power.wav");                      //lightning for boss

// player gib sounds
	precache_sound ("player/gib.wav");                      // player gib sound
	precache_sound ("player/udeath.wav");           // player gib sound
	precache_sound ("player/tornoff2.wav");         // gib sound

// player pain sounds

	precache_sound ("player/pain1.wav");
	precache_sound ("player/pain2.wav");
	precache_sound ("player/pain3.wav");
	precache_sound ("player/pain4.wav");
	precache_sound ("player/pain5.wav");
	precache_sound ("player/pain6.wav");

// player death sounds
	precache_sound ("player/death1.wav");
	precache_sound ("player/death2.wav");
	precache_sound ("player/death3.wav");
	precache_sound ("player/death4.wav");
	precache_sound ("player/death5.wav");

	precache_sound ("boss1/sight1.wav");

// ax sounds    
	precache_sound ("weapons/ax1.wav");                     // ax swoosh
	precache_sound ("player/axhit1.wav");           // ax hit meat
	precache_sound ("player/axhit2.wav");           // ax hit world

	precache_sound ("player/h2ojump.wav");          // player jumping into water
	precache_sound ("player/slimbrn2.wav");         // player enter slime
	precache_sound ("player/inh2o.wav");            // player enter water
	precache_sound ("player/inlava.wav");           // player enter lava
	precache_sound ("misc/outwater.wav");           // leaving water sound

	precache_sound ("player/lburn1.wav");           // lava burn
	precache_sound ("player/lburn2.wav");           // lava burn

	precache_sound ("misc/water1.wav");                     // swimming
	precache_sound ("misc/water2.wav");                     // swimming

// Invulnerability sounds
	precache_sound ("items/protect.wav");
	precache_sound ("items/protect2.wav");
	precache_sound ("items/protect3.wav");

	precache_model ("progs/player.mdl");
	precache_model ("progs/guy.mdl");
	precache_model ("progs/lay.mdl");
	precache_model ("progs/eyes.mdl");
	precache_model ("progs/h_player.mdl");
	precache_model ("progs/gib1.mdl");
	precache_model ("progs/gib2.mdl");
	precache_model ("progs/gib3.mdl");

	precache_model ("progs/s_bubble.spr");  // drowning bubbles
	precache_model ("progs/s_explod.spr");  // sprite explosion

	precache_model ("progs/v_axe.mdl");
	precache_model ("progs/v_shot.mdl");
	precache_model ("progs/v_span.mdl");
	precache_model ("progs/v_nail.mdl");
	precache_model ("progs/v_rock.mdl");
	precache_model ("progs/v_shot2.mdl");
	precache_model ("progs/v_nail2.mdl");
	precache_model ("progs/v_rock2.mdl");
	precache_model ("progs/v_fist.mdl");
	precache_model ("progs/v_knife.mdl");
	precache_model ("progs/v_1911.mdl");
	precache_model ("progs/v_ak47.mdl");
	precache_model ("progs/v_smg.mdl");
	precache_model ("progs/v_rangem.mdl");
	precache_model ("progs/v_pipe.mdl");
	precache_model ("progs/v_shotgun.mdl");
	precache_model ("progs/v_double.mdl");
	precache_model ("progs/v_mp9.mdl");
	precache_model ("progs/v_sa80.mdl");
	precache_model ("progs/v_deagle.mdl");
	precache_model ("progs/v_alien.mdl");
	precache_model ("progs/v_srifle.mdl");
	precache_model ("progs/v_night.mdl");
	precache_model ("progs/v_piperifle.mdl");
	precache_model ("progs/v_handgren.mdl");
	precache_model ("progs/v_jackhammer.mdl");
	precache_model ("progs/v_rocket.mdl");

	precache_model ("progs/handgren.mdl");
	precache_model ("progs/plasma.mdl");
	precache_model ("progs/ray.mdl");
	precache_model ("progs/sneak.mdl");
	precache_model ("progs/dead.mdl");
	precache_model ("progs/hbar.spr");
	precache_model ("progs/blast.mdl");
	precache_model ("progs/station.mdl");
	precache_model ("progs/camera.mdl");
	precache_model ("progs/tehghoul.mdl");
	precache_model ("progs/ray.mdl");

	precache_model ("progs/w_knife.mdl");
	precache_model ("progs/w_pipe.mdl");
	precache_model ("progs/w_shotgun.mdl");
	precache_model ("progs/w_axe.mdl");
	precache_model ("progs/w_1911.mdl");
	precache_model ("progs/w_deagle.mdl");
	precache_model ("progs/w_alien.mdl");
	precache_model ("progs/w_mp7.mdl");
	precache_model ("progs/w_mp9.mdl");
	precache_model ("progs/w_jackhammer.mdl");
	precache_model ("progs/w_rangem.mdl");
	precache_model ("progs/w_sa80.mdl");
	precache_model ("progs/w_ak47.mdl");
	precache_model ("progs/w_night.mdl");
	precache_model ("progs/w_srifle.mdl");
	precache_model ("progs/w_gauss.mdl");
	precache_model ("progs/w_carbine.mdl");
	precache_model ("progs/w_laser.mdl");
	precache_model ("progs/w_rocket.mdl");

	precache_model ("progs/grenade2.mdl");
	precache_model ("maps/b_bh10.bsp");
	precache_model ("maps/b_bh25.bsp");
	precache_model ("maps/b_bh100.bsp");
	precache_model ("progs/s_light.spr");

	precache_sound ("misc/thud.wav");
	precache_sound ("player/step1.wav");
	precache_sound ("player/step2.wav");
	precache_sound ("player/step3.wav");
	precache_sound ("player/step4.wav");
	precache_sound ("player/step1a.wav");
	precache_sound ("player/step2a.wav");
	precache_sound ("player/step3a.wav");
	precache_sound ("player/step4a.wav");
	precache_sound ("dog/dattack1.wav");
	precache_sound ("dog/ddeath.wav");
	precache_sound ("dog/dpain1.wav");
	precache_sound ("dog/dsight.wav");
	precache_sound ("dog/idle.wav");
	precache_sound ("weapons/1911.wav");
	precache_sound ("weapons/auto.wav");
	precache_sound ("weapons/auto2.wav");
	precache_sound ("weapons/ak112.wav");
	precache_sound ("weapons/ak47.wav");
	precache_sound ("weapons/amr.wav");
	precache_sound ("weapons/blaster.wav");
	precache_sound ("weapons/blaster2.wav");
	precache_sound ("weapons/blowoff.wav");
	precache_sound ("weapons/bounce.wav");
	precache_sound ("weapons/burst.wav");
	precache_sound ("weapons/caws.wav");
	precache_sound ("weapons/click.wav");
	precache_sound ("weapons/deagle.wav");
	precache_sound ("weapons/dks-1.wav");
	precache_sound ("weapons/energy1.wav");
	precache_sound ("weapons/energy2.wav");
	precache_sound ("weapons/energy3.wav");
	precache_sound ("weapons/energy4.wav");
	precache_sound ("weapons/flamer.wav");
	precache_sound ("weapons/fnfal.wav");
	precache_sound ("weapons/g11.wav");
	precache_sound ("weapons/gauss.wav");
	precache_sound ("weapons/gl-1.wav");
	precache_sound ("weapons/gpull.wav");
	precache_sound ("weapons/helmet.wav");
	precache_sound ("weapons/jackhammer.wav");
	precache_sound ("weapons/laser.wav");
	precache_sound ("weapons/m249.wav");
	precache_sound ("weapons/m4-nw.wav");
	precache_sound ("weapons/needler.wav");
	precache_sound ("weapons/shotgun1.wav");
	precache_sound ("weapons/shotgun2.wav");
	precache_sound ("weapons/mp5.wav");
	precache_sound ("weapons/mp7.wav");
	precache_sound ("weapons/rangem.wav");
	precache_sound ("weapons/ric1.wav");
	precache_sound ("weapons/ric2.wav");
	precache_sound ("weapons/ric3.wav");
	precache_sound ("weapons/ric4.wav");
	precache_sound ("weapons/ric5.wav");
	precache_sound ("weapons/tink1.wav");
	precache_sound ("weapons/sa80-1.wav");
	precache_sound ("weapons/shell.wav");
	precache_sound ("weapons/reload.wav");
	precache_sound ("misc/build1.wav");
	precache_sound ("misc/build2.wav");
	precache_sound ("misc/build3.wav");
	precache_sound ("player/agdie1.wav");
	precache_sound ("player/agdie2.wav");
	precache_sound ("player/agdie3.wav");
	precache_sound ("player/agdie4.wav");
	precache_sound ("player/agdie5.wav");
	precache_sound ("player/berserk.wav");
	precache_sound ("player/breathe.wav");
	precache_sound ("player/headshot.wav");
	precache_sound ("player/hit1.wav");
	precache_sound ("player/paina.wav");
	precache_sound ("player/painb.wav");
	precache_model ("progs/bolt.mdl");              // for lightning gun
	precache_model ("progs/bolt2.mdl");             // for lightning gun
	precache_model ("progs/bolt3.mdl");             // for boss shock
	precache_model ("progs/lavaball.mdl");  // for testing
	
	precache_model ("progs/missile.mdl");
	precache_model ("progs/grenade.mdl");
	precache_model ("progs/spike.mdl");
	precache_model ("progs/s_spike.mdl");

	precache_model ("progs/backpack.mdl");

	precache_model ("progs/zom_gib.mdl");

	precache_model ("progs/v_light.mdl");
	


//
// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
//

	// 0 normal
	lightstyle(0, "m");
	
	// 1 FLICKER (first variety)
	lightstyle(1, "mmnmmommommnonmmonqnmmo");
	
	// 2 SLOW STRONG PULSE
	lightstyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	
	// 3 CANDLE (first variety)
	lightstyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	
	// 4 FAST STROBE
	lightstyle(4, "mamamamamama");
	
	// 5 GENTLE PULSE 1
	lightstyle(5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	
	// 6 FLICKER (second variety)
	lightstyle(6, "nmonqnmomnmomomno");
	
	// 7 CANDLE (second variety)
	lightstyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");
	
	// 8 CANDLE (third variety)
	lightstyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	
	// 9 SLOW STROBE (fourth variety)
	lightstyle(9, "aaaaaaaazzzzzzzz");
	
	// 10 FLUORESCENT FLICKER
	lightstyle(10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	lightstyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	
	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	lightstyle(63, "a");
};

void() StartFrame =
{
	timelimit = cvar("timelimit") * 60;
	fraglimit = cvar("fraglimit");
	teamplay = cvar("teamplay");
	deathmatch = !coop;
	
	framecount = framecount + 1;
};

/*
==============================================================================

BODY QUE

==============================================================================
*/

entity  bodyque_head;

void() bodyque =
{       // just here so spawn functions don't complain after the world
	// creates bodyques
};

void() InitBodyQue =
{
	bodyque_head = spawn();
	bodyque_head.classname = "bodyque";
	bodyque_head.owner = spawn();
	bodyque_head.owner.classname = "bodyque";
	bodyque_head.owner.owner = spawn();
	bodyque_head.owner.owner.classname = "bodyque";
	bodyque_head.owner.owner.owner = spawn();
	bodyque_head.owner.owner.owner.classname = "bodyque";
	bodyque_head.owner.owner.owner.owner = bodyque_head;
};


// make a body que entry for the given ent so the ent can be
// respawned elsewhere
void(entity ent) CopyToBodyQue =
{
	bodyque_head.angles = ent.angles;
	bodyque_head.model = ent.model;
	bodyque_head.modelindex = ent.modelindex;
	bodyque_head.frame = ent.frame;
	bodyque_head.colormap = ent.colormap;
	bodyque_head.movetype = ent.movetype;
	bodyque_head.velocity = ent.velocity;
	bodyque_head.flags = 0;
	setorigin (bodyque_head, ent.origin);
	setsize (bodyque_head, ent.mins, ent.maxs);
	bodyque_head = bodyque_head.owner;
};


