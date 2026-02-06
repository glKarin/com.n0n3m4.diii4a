
void() T_MissileTouch;
void() info_player_start;
void(entity targ, entity attacker) ClientObituary;
void(entity inflictor, entity attacker, float damage, entity ignore, string dtype) T_RadiusDamage;

void() monster_death_use;

//============================================================================

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
float(entity targ, entity inflictor) CanDamage =
{
// bmodels need special checking because their origin is 0,0,0
	if (targ.movetype == MOVETYPE_PUSH)
	{
		traceline(inflictor.origin, 0.5 * (targ.absmin + targ.absmax), TRUE, self);
		if (trace_fraction == 1)
			return TRUE;
		if (trace_ent == targ)
			return TRUE;
		return FALSE;
	}
	
	traceline(inflictor.origin, targ.origin, TRUE, self);
	if (trace_fraction == 1)
		return TRUE;
	traceline(inflictor.origin, targ.origin + '15 15 0', TRUE, self);
	if (trace_fraction == 1)
		return TRUE;
	traceline(inflictor.origin, targ.origin + '-15 -15 0', TRUE, self);
	if (trace_fraction == 1)
		return TRUE;
	traceline(inflictor.origin, targ.origin + '-15 15 0', TRUE, self);
	if (trace_fraction == 1)
		return TRUE;
	traceline(inflictor.origin, targ.origin + '15 -15 0', TRUE, self);
	if (trace_fraction == 1)
		return TRUE;

	return FALSE;
};


/*
============
Killed
============
*/
void(entity targ, entity attacker) Killed =
{
	local entity oself;

	oself = self;
	self = targ;
	
	if (self.health < -99)
		self.health = -99;              // don't let sbar look bad if a player

	if (self.movetype == MOVETYPE_PUSH || self.movetype == MOVETYPE_NONE)
	{       // doors, triggers, etc
		self.th_die ();
		self = oself;
		return;
	}

	self.enemy = attacker;

	targ.grab = 0;
	targ.equipment_slot = 0;         //turn off stealth-boys, climbing gear, etc

// bump the monster counter
	if (self.flags & FL_MONSTER)
	{
		killed_monsters = killed_monsters + 1;
		WriteByte (MSG_ALL, SVC_KILLEDMONSTER);
	}

	ClientObituary(self, attacker);
	
	if (self.classname == "player")  //so dead players can spectate
	{
		self.ghost = 1;
		self.flags = self.flags | FL_FINDABLE_NONSOLID;	//so tracelines can find them
	}

	self.takedamage = DAMAGE_NO;
	self.touch = SUB_Null;
	self.effects = 0;

	monster_death_use();

	self.th_die ();
	
	self = oself;
};


/*
============
T_Damage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
============
*/

void(entity targ, entity inflictor, entity attacker, float damage) T_Damage =
{
	local   entity  oldself;
	local   float   save;
	local   float   take, severity, helm;
	local   string  attackerteam, targteam;


	if (!targ.takedamage)
		return;

// used by buttons and triggers to set activator for target firing
	damage_attacker = attacker;


// check for quad damage powerup on the attacker
	if (attacker.super_damage_finished > time && inflictor.classname != "door")
	if (deathmatch == 4)
		damage = damage * 8;
	else
		damage = damage * 4;

	if (attacker.critical == 3)//attacker scored a headshot/critical
	{
		if (attacker.critical == 3)
		{
			severity = 0 + random()*20;
			if (attacker.perk == 7)
				severity = severity + 4;

			if (attacker.class == 3)
				severity = severity + 2;

			if (severity >= 19)
				damage = (damage * 5);
			else if (severity >= 14)
				damage = (damage * 4);
			else
				damage = (damage * 3);

			helm = targ.armortype;
			if (targ.helmet == 0)
			{
				sound (targ, CHAN_BODY, "player/headshot.wav", 1, ATTN_NORM);
				helm = 0;
			}
			if (targ.helmet == AS_STRAIGHT)
			{
				sound (targ, CHAN_BODY, "weapons/helmet.wav", 1, ATTN_NORM);
				helm = 0.30;
			}
			if (targ.helmet == AS_SLIDING)
			{
				sound (targ, CHAN_BODY, "weapons/helmet.wav", 1, ATTN_NORM);
				helm = 0.45;
			}

			damage = (damage - (damage * helm));

			makevectors (targ.v_angle);
		}
	}

	if (damage <= 0)
	{
		damage = 0;
		sound (targ, CHAN_BODY, targ.armornoise, 1, ATTN_NORM);
		return;
	}

		sound (targ, CHAN_BODY, targ.armornoise, 1, ATTN_NORM);

// save damage based on the target's armor level

	save = ceil(targ.armortype*damage);
	if (save >= targ.armorvalue)
	{
		save = targ.armorvalue;
/*		targ.armortype = 0;     // lost all armor
		targ.items = targ.items - (targ.items & (IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3));
*/
	}

	
	take = ceil(damage-save);

// add to the damage total for clients, which will be sent as a single
// message at the end of the frame
// FIXME: remove after combining shotgun blasts?
	if (targ.flags & FL_CLIENT)
	{
		targ.dmg_take = targ.dmg_take + take;
		targ.dmg_save = targ.dmg_save + save;
		targ.dmg_inflictor = inflictor;
	}

	damage_inflictor = inflictor;        

/*
// figure momentum add
	if ( (inflictor != world) && (targ.movetype == MOVETYPE_WALK) )
	{
		dir = targ.origin - (inflictor.absmin + inflictor.absmax) * 0.5;
		dir = normalize(dir);
		// Set kickback for smaller weapons
//Zoid -- use normal NQ kickback
//		// Read: only if it's not yourself doing the damage
//		if ( (damage < 60) & ((attacker.classname == "player") & (targ.classname == "player")) & ( attacker.netname != targ.netname)) 
//			targ.velocity = targ.velocity + dir * damage * 11;
//		else                        
		// Otherwise, these rules apply to rockets and grenades                        
		// for blast velocity
			targ.velocity = targ.velocity + dir * damage * 8;
		
		// Rocket Jump modifiers
		if ( (rj > 1) & ((attacker.classname == "player") & (targ.classname == "player")) & ( attacker.netname == targ.netname)) 
			targ.velocity = targ.velocity + dir * damage * rj;

	}*/



// check for godmode or invincibility
	if (targ.flags & FL_GODMODE)
		return;
	if (targ.invincible_finished >= time)
	{
		if (self.invincible_sound < time)
		{
			sound (targ, CHAN_ITEM, "items/protect3.wav", 1, ATTN_NORM);
			self.invincible_sound = time + 2;
		}
		return;
	}

// team play damage avoidance
//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
	attackerteam = infokey(attacker, "team");
	targteam = infokey(targ, "team");

	if ((teamplay == 1) && (targteam == attackerteam) &&
		(attacker.classname == "player") && (attackerteam != "") &&
		inflictor.classname !="door")
		return;

	if ((teamplay == 3) && (targteam == attackerteam) &&
		(attacker.classname == "player") && (attackerteam != "") &&
		(targ != attacker)&& inflictor.classname !="door")
		return;
		

// do the damage
	//different sorts of armour simply subtract different ammounts
      //this makes armor like the force armor good against many small rounds
      //(SMG, shotguns, swords, etc) but useless against big damage (grenades)
      //power armor, which has the best of both worlds, is also the heaviest :p

	switch(ToIID(targ.islot3))
	{
	case IID_ARM_SHIRT:
		take -= 1;
		break;
	case IID_ARM_LEATHER:
		take -= 2;
		break;
	case IID_ARM_KEVLAR:
		take -= 3;
		break;
	case IID_ARM_METAL:
		take -= 4;
		break;
	case IID_ARM_COMBAT:
		take -= 5;
		break;
	case IID_ARM_BROTHERHOOD:
		take -= 6;
		break;
	case IID_ARM_FORCE:
		take -= 7;
		break;
	case IID_ARM_LPOWER:
		take -= 8;
		break;
	default:
		break;
	}


	if (take <= 0)
	{
		take = 0;
		sound (targ, CHAN_BODY, targ.armornoise, 1, ATTN_NORM);
		return;
	}


	targ.health = targ.health - take;

	if (targ.health <= 0)
	{
		Killed (targ, attacker);
		return;
	}

// react to the damage
	oldself = self;
	self = targ;

/*SERVER
	if ( (self.flags & FL_MONSTER) && attacker != world)
	{
	// get mad unless of the same class (except for soldiers)
		if (self != attacker && attacker != self.enemy)
		{
			if ( (self.classname != attacker.classname) 
			|| (self.classname == "monster" ) )
			{
				if (self.enemy.classname == "player")
					self.oldenemy = self.enemy;
				self.enemy = attacker;
				FoundTarget ();
			}
		}
	}
*/
	if (self.th_pain)
	{
		self.th_pain (attacker, take);
	}

	self = oldself;
};

/*
============
X_Damage

The other damage function.

============
*/

void(entity targ, entity inflictor, entity attacker, float damage) X_Damage =
{
	local   entity  oldself;
	local   float   save;
	local   float   take, severity, helm;
	local   string  attackerteam, targteam;


	if (!targ.takedamage)
		return;

// used by buttons and triggers to set activator for target firing
	damage_attacker = attacker;


// check for quad damage powerup on the attacker
	if (attacker.super_damage_finished > time && inflictor.classname != "door")
	if (deathmatch == 4)
		damage = damage * 8;
	else
		damage = damage * 4;

	if (attacker.critical == 3)//attacker scored a headshot/critical
	{
		if (attacker.critical == 3)
		{
			severity = 0 + random()*20;
			if (attacker.perk == 7)
				severity = severity + 4;

			if (attacker.class == 3)
				severity = severity + 2;

			if (severity >= 19)
				damage = (damage * 3.0);
			else if (severity >= 14)
				damage = (damage * 2.5);
			else
				damage = (damage * 2.0);

			helm = targ.armortype;
			if (targ.helmet == 0)
			{
				sound (targ, CHAN_BODY, "player/headshot.wav", 1, ATTN_NORM);
				helm = 0;
			}
			if (targ.helmet == AS_STRAIGHT)
			{
				sound (targ, CHAN_BODY, "weapons/helmet.wav", 1, ATTN_NORM);
				helm = 0.10;
			}
			if (targ.helmet == AS_SLIDING)
			{
				sound (targ, CHAN_BODY, "weapons/helmet.wav", 1, ATTN_NORM);
				helm = 0.15;
			}

			damage = (damage - (damage * helm));

			makevectors (targ.v_angle);
		}
	}

	if (random()*4<=1)
		sound (targ, CHAN_BODY, targ.armornoise, 1, ATTN_NORM);

// add to the damage total for clients, which will be sent as a single
// message at the end of the frame
// FIXME: remove after combining shotgun blasts?
	if (targ.flags & FL_CLIENT)
	{
		targ.dmg_take = targ.dmg_take + take;
		targ.dmg_save = targ.dmg_save + save;
		targ.dmg_inflictor = inflictor;
	}

	damage_inflictor = inflictor;        

/*
// figure momentum add
	if ( (inflictor != world) && (targ.movetype == MOVETYPE_WALK) )
	{
		dir = targ.origin - (inflictor.absmin + inflictor.absmax) * 0.5;
		dir = normalize(dir);
		// Set kickback for smaller weapons
//Zoid -- use normal NQ kickback
//		// Read: only if it's not yourself doing the damage
//		if ( (damage < 60) & ((attacker.classname == "player") & (targ.classname == "player")) & ( attacker.netname != targ.netname)) 
//			targ.velocity = targ.velocity + dir * damage * 11;
//		else                        
		// Otherwise, these rules apply to rockets and grenades                        
		// for blast velocity
			targ.velocity = targ.velocity + dir * damage * 8;
		
		// Rocket Jump modifiers
		if ( (rj > 1) & ((attacker.classname == "player") & (targ.classname == "player")) & ( attacker.netname == targ.netname)) 
			targ.velocity = targ.velocity + dir * damage * rj;

	}*/



// check for godmode or invincibility
	if (targ.flags & FL_GODMODE)
		return;

	if (targ.invincible_finished >= time)
	{
		if (self.invincible_sound < time)
		{
			sound (targ, CHAN_ITEM, "items/protect3.wav", 1, ATTN_NORM);
			self.invincible_sound = time + 2;
		}
		return;
	}

// team play damage avoidance
//ZOID 12-13-96: self.team doesn't work in QW.  Use keys
	attackerteam = infokey(attacker, "team");
	targteam = infokey(targ, "team");

	if ((teamplay == 1) && (targteam == attackerteam) &&
		(attacker.classname == "player") && (attackerteam != "") &&
		inflictor.classname !="door")
		return;

	if ((teamplay == 3) && (targteam == attackerteam) &&
		(attacker.classname == "player") && (attackerteam != "") &&
		(targ != attacker)&& inflictor.classname !="door")
		return;
		

// do the damage
	//different sorts of armour simply subtract different ammounts
      //this makes armor like the force armor good against many small rounds
      //(SMG, shotguns, swords, etc) but useless against big damage (grenades)
      //power armor, which has the best of both worlds, is also the heaviest :p

	switch(ToIID(targ.islot3))
	{
		case IID_ARM_BROTHERHOOD:
			take -= 1;
			break;
		case IID_ARM_FORCE:
			take -= 2;
			break;
		case IID_ARM_LPOWER:
			take -= 3;
			break;
		default:
			break;
	}


	if (take <= 0)
	{
		take = 0;
		sound (targ, CHAN_BODY, targ.armornoise, 1, ATTN_NORM);
		return;
	}


	targ.health = targ.health - take;

	if (targ.health <= 0)
	{
		Killed (targ, attacker);
		return;
	}

// react to the damage
	oldself = self;
	self = targ;

/*SERVER
	if ( (self.flags & FL_MONSTER) && attacker != world)
	{
	// get mad unless of the same class (except for soldiers)
		if (self != attacker && attacker != self.enemy)
		{
			if ( (self.classname != attacker.classname) 
			|| (self.classname == "monster" ) )
			{
				if (self.enemy.classname == "player")
					self.oldenemy = self.enemy;
				self.enemy = attacker;
				FoundTarget ();
			}
		}
	}
*/
	if (self.th_pain)
	{
		self.th_pain (attacker, take);
	}

	self = oldself;
};

/*
============
T_RadiusDamage
============
*/
void(entity inflictor, entity attacker, float damage, entity ignore, string dtype) T_RadiusDamage =
{
	local   float   points;
	local   entity  head;
	local   vector  org;

	head = findradius(inflictor.origin, damage+40);
	
	while (head)
	{
		//bprint (PRINT_HIGH, head.classname);
		//bprint (PRINT_HIGH, " | ");
		//bprint (PRINT_HIGH, head.netname);
		//bprint (PRINT_HIGH, "\n");
	
		if (head != ignore)
		{
			if (head.takedamage)
			{
				org = head.origin + (head.mins + head.maxs)*0.5;
				points = 0.5*vlen (inflictor.origin - org);
				if (points < 0)
					points = 0;
				points = damage - points;
				
				if (head == attacker)
					points = points * 0.5;
				if (points > 0)
				{
					if (CanDamage (head, inflictor))
					{
						head.deathtype = dtype;
						T_Damage (head, inflictor, attacker, points);
					}
				}
			}
		}
		head = head.chain;
	}
};

/*
============
T_BeamDamage
============
*/
void(entity attacker, float damage) T_BeamDamage =
{
	local   float   points;
	local   entity  head;
	
	head = findradius(attacker.origin, damage+40);
	
	while (head)
	{
		if (head.takedamage)
		{
			points = 0.5*vlen (attacker.origin - head.origin);
			if (points < 0)
				points = 0;
			points = damage - points;
			if (head == attacker)
				points = points * 0.5;
			if (points > 0)
			{
				if (CanDamage (head, attacker))
					T_Damage (head, attacker, attacker, points);
			}
		}
		head = head.chain;
	}
};

