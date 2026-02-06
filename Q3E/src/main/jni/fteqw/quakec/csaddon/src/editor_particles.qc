
static float cureffect;
static float curmodified;
const textfield_linedesc_t fields[100] =
{
	{"shader", "shader <shadername>\\n{\\nshaderbody\\n}"},
	{"texture", "texture <shadername>"},
	{"tcoords", "tcoords <s1> <t1> <s2> <t2> [coordscale] [randcount] [randsinc]\ncoordscale is an optional divisor for more readable numbers.\nrandcount is a the number of images used in particlefont image.\nrandsinc is the amount to add to s coords to display each sequential randomized image."},
	{"rotationstart", "rotationstart <minangle> [maxangle]\nan angle of 45 will give you a particle aligned to screen coords"},
	{"rotationspeed", "rotationspeed <min> <max>\nspecifies a range of rotational speeds. You probably want to set the min value as negative."},
	{"beamtexstep",	"beamtexstep <documentme>"},
	{"scale",	"scale <min> [max]\nValues are 4 times higher than quake units"},
	{"scalerand",	"use scale instead"},
	{"scalefactor",	"scalefactor <factor>\nHow much the particle shrinks with distance.\n1=true scaling"},
	{"scaledelta",	"scaledelta <changepersecond>\nHow much the particle's scale changes per second"},
	{"step",	"step <dist>\nworld distance between each particle within a particle trail."},
	{"count",	"count <multiplier>\nProvides a multiplier value independant from the particle() builtin or other mechanisms"},
	{"alpha",	"alpha <val>\nInitial opacity of particle. 0 is invisible, 1 is fully visible."},
	{"alphachange",	"depricated. use alphadelta"},
	{"alphadelta",	"alphadelta <alphapersecond>\n How much the alpha value changes per second"},
	{"die",	"die <min> [max]\nHow long the particle lives for"},
	{"diesubrand",	"depricated. use die"},
	{"randomvel",	"<documentme>"},
	{"veladd",	"<documentme>"},
	{"orgadd",	"<documentme>"},
	{"friction",	"<documentme>"},
	{"gravity",	"gravity <grav>\nParticle's downwards velocity will be increased by this much per second"},
	{"clipbounce",	"<documentme>"},
	{"assoc",	"assoc <othereffectname>\nWhen the effect is spawned, the associated effect will also be spawned. Can chain."},
	{"inwater",	"inwater <othereffectname>\nIf the particle effect is spawned underwater, the named effect will be used instead."},
	{"model",	"model <modelname> <firstframe> <lastframe> <framerate> <alpha>\nSpawns a the sprite/model when the effect is spawned.\nCan be specified multiple times and a random model line will be used.\nModels with only 1 frame will animate skins instead."},
	{"colorindex",	"colourindex <min> <max>\nIf set, the particle's colour will be set to this palette index instead of the particle's rgb field."},
	{"colorrand",	"depricated"},
	{"citracer",	"citracer\nFlag that syncronizes colorindex-based palette indexes with spawn ordering ala quake."},
	{"red",	"depricated, use rgb."},
	{"green",	"depricated, use rgb."},
	{"blue",	"depricated, use rgb."},
	{"rgb",	"rgb <red> <green> <blue>\nInitial colour of particle, in a scale of 0 to 255."},
	{"reddelta",	"depricated, use rgbdelta."},
	{"greendelta",	"depricated, use rgbdelta."},
	{"bluedelta",	"depricated, use rgbdelta."},
	{"rgbdelta",	"rgbdelta <red> <green> <blue>\nSpecifies the change in particle colours over a second of time. The effective range is 0 to 255, but larger values might be needed for short-lived effects."},
	{"rgbdeltatime",	"rgbdeltatime <age>\nOnce the particle reaches this age, its rgb values will stop changing."},
	{"redrand",	"depricated"},
	{"greenrand",	"depricated"},
	{"bluerand",	"depricated"},
	{"rgbrand",	"rgbrand <red> <green> <blue>\nThis rgb value will be multiplied by a random value between 0 and 1 and added to the particle's initial rgb value. All three channels channels are scaled independantly."},
	{"rgbrandsync",	"rgbrandsync <red> <green> <blue>\nThis rgb value will be multiplied by a random value between 0 and 1 and added to the particle's initial rgb value. All three channels will be scaled the same."},
	{"redrandsync",	"depricated"},
	{"greenrandsync",	"depricated"},
	{"bluerandsync",	"depricated"},
	{"stains",	"stains <radius>\nSpecifies a multiplier for the radius of a stain if the particle impacts upon a wall"},
	{"blend",	"blend <blendmode>\nOne of add, subtract, blendcolour, or blendalpha. Particles with additive blend modes will render fastest due to no depth sorting requirement."},
	{"spawnmode",	"<documentme>"},
	{"type",	"type <rendertype>\nOne of:\nbeam - quads generated between particles\nspark - a single line drawn in direction of motion\nsparkfan - triangle fan oriented in direction of motion\ntexturedspark - quads extruded along line of motion\ndecal - motionless effect that attaches to surfaces around the center of the effect\nnormal - simple screen-facing quad"},
	{"isbeam",	"depricated, use type."},
	{"spawntime",	"<documentme>"},
	{"spawnchance",	"<documentme>"},
	{"cliptype",	"<documentme>"},
	{"clipcount",	"<documentme>"},
	{"emit",	"<documentme>"},
	{"emitinterval",	"<documentme>"},
	{"emitintervalrand",	"<documentme>"},
	{"emitstart",	"<documentme>"},
	{"areaspread",	"<documentme>"},
	{"areaspreadvert",	"<documentme>"},
	{"offsetspread",	"<documentme>"},
	{"offsetspreadvert",	"<documentme>"},
	{"spawnorg",	"<documentme>"},
	{"spawnvel",	"<documentme>"},
	{"spawnparm1",	"<documentme>"},
	{"spawnparm2",	"<documentme>"},
	{"up",	"<documentme>"},
	{"rampmode",	"<documentme>"},
	{"rampindexlist",	"<documentme>"},
	{"rampindex",	"<documentme>"},
	{"ramp",	"<documentme>"},
	{"perframe",	"<documentme>"},
	{"averageout",	"<documentme>"},
	{"nostate",	"<documentme>"},
	{"nospreadfirst",	"<documentme>"},
	{"nospreadlast",	"<documentme>"},
	{"lightradius",	"<documentme>"},
	{"lightradiusfade",	"<documentme>"},
	{"lightradiusrgb",	"<documentme>"},
	{"lightradiusrgbfade",	"<documentme>"},
	{"lighttime",	"<documentme>"},
	{__NULL__}
};

strip static textfield_t tf_particle;

#define MAXPARTICLETYPES 200
typedef struct
{
	string efname;
	int start;
	int end;
} particledesc_t;
strip particledesc_t pdesc[MAXPARTICLETYPES];
int numptypes;
string particlesfile;

static entity ptrailent;

static void(string descname) saveparticle;

static void(float newef) selecteffect =
{
	if (newef == cureffect)
		return;

	if (curmodified)
		saveparticle(cvar_string("ca_particleset"));
	curmodified = FALSE;

	cureffect = floor(newef);
	if (cureffect < 0)
		cureffect = 0;

	textfield_fill(&tf_particle, substring(particlesfile, pdesc[cureffect].start, pdesc[cureffect].end - pdesc[cureffect].start));

	if (ptrailent)
		remove(ptrailent);
	ptrailent = world;
};



static int(string src, int *start) skipblock =
{
	string line;
	int ls = *start, le;
	int level = 0;
	/*parse a string from the start of one { to its closing }*/

	while(1)
	{
		le = strstrofs(particlesfile, "\n", ls);
		if (le < 0)
			break;

		line = substring(particlesfile, ls, le - ls);
		tokenize_console(line);
		line = argv(0);
		if (line == "{")
		{
			if (!level)
				*start = le+1;
			level+=1;
		}
		else if (line == "}")
		{
			if (level == 1)
				return ls;
			--level;
		}
		else if (!level)
				return le+1;

		ls = le+1;
	}
	
	return ls;
};

static void() updateloadedparticles =
{
	string line;
	int ls, le;
	int ce = cureffect;
	while(numptypes>0)
	{
		--numptypes;
		strunzone(pdesc[numptypes].efname);
	}

	ls = 0;
	while(1)
	{
		le = strstrofs(particlesfile, "\n", ls);
		if (le < 0)
			break;
//		print("line: ", substring(particlesfile, ls, le - ls), "\n");
		if (le - ls > 7)
			if (substring(particlesfile, ls, 6) == "r_part")
			{
				line = substring(particlesfile, ls, le - ls);
				tokenize_console(line);
				if (argv(0) == "r_part")
				{
					pdesc[numptypes].efname = strzone(argv(1));
					pdesc[numptypes].start = le+1;
					le = skipblock(particlesfile, &pdesc[numptypes].start);
					pdesc[numptypes].end = le;

//					print("particle: ", pdesc[numptypes].efname, "\n", substring(particlesfile, pdesc[numptypes].start, pdesc[numptypes].end - pdesc[numptypes].start), "\n");
					numptypes+=1;
				}
			}
		ls = le+1;
	}
	
	cureffect = -1;
	selecteffect(ce);
};

static void(string descname) loadparticles =
{
	filestream f;

	/*kill old string*/
	if notnull(particlesfile)
		strunzone(particlesfile);
	particlesfile = (string)0;
	cureffect = 0;

/*load new string*/
	if (descname != "")
	{
		f = fopen(strcat("particles/", descname, ".cfg"), FILE_READNL);
		if ((float)f >= 0)
		{
			particlesfile = strzone(fgets(f));
			fclose(f);
		}
	}

	/*and find out where the particles are*/
	updateloadedparticles();

	localcmd(particlesfile);
	localcmd("\n");
};

static void(string descname) saveparticle =
{
	filestream f;
	string newfile;

	/*only do this if there's something to save*/
	if (cureffect < 0)
		return;
		
	newfile = textfield_strcat(&tf_particle);
	newfile = strcat(substring(particlesfile, 0, pdesc[cureffect].start), newfile, substring(particlesfile, pdesc[cureffect].end, -1));

	strunzone(particlesfile);
	particlesfile = "";

	particlesfile = strzone(newfile);

	if (descname != "")
	{
		f = fopen(strcat("particles/", descname, ".cfg"), FILE_WRITE);
		if ((float)f >= 0)
		{
			fputs(f, particlesfile);
			fclose(f);
		}
	}

	/*and recalculate where the particles are*/
	updateloadedparticles();
};

static void() applyparticles =
{
	if (tf_particle.dirty)
	{
		string sln;
		int i;
		if (cureffect >= numptypes)
			return;

		localcmd("r_part \"");
		localcmd(pdesc[cureffect].efname);
		localcmd("\"\n{\n");
		textfield_localcmd(&tf_particle);
		localcmd("}\n");

		sln = strcat("+", pdesc[cureffect].efname);
		for (i = cureffect + 1; i < numptypes; i+=1)
		{
			if (pdesc[i].efname == sln)
			{
				localcmd("r_part \"");
				localcmd(pdesc[i].efname);
				localcmd("\"\n{\n");
				localcmd(substring(particlesfile, pdesc[i].start, pdesc[i].end - pdesc[i].start));
				localcmd("}\n");
			}
		}

		tf_particle.dirty = FALSE;

		curmodified = TRUE;
	}
};

float(float keyc, float unic, vector m) editor_particles_key =
{
	if (m_y > 16 && m_y < 24 && m_x < 128)
	{
		string oval = strcat(cvar_string("ca_particleset"));
		if (keyc == 127)
			cvar_set("ca_particleset", substring(oval, 0, -2));
		else if (unic)
			cvar_set("ca_particleset", strcat(oval, chr2str(unic)));
		else
			return FALSE;

		applyparticles();
		if (curmodified)
			saveparticle(oval);
		curmodified = FALSE;

		oval = cvar_string("ca_particleset");
		loadparticles(oval);
		return TRUE;
	}

	if (m_y < 32)
		return FALSE;

	/*middle mouse*/
	if (keyc == 514)
	{
		mousedown |= 4;

		applyparticles();
	}
	else if (m_x < 128)
	{
		if (keyc == 512)
		{
			float ef, y;
			ef = cureffect - 10;
			if (ef < 0)
				ef = 0;
			y = 32;

			selecteffect(ef + (m_y - y)/8);
		}
		return TRUE;
	}
	else
	{
		return textfield_key(&tf_particle, keyc, unic, m - '128 32 0');
	}
	return FALSE;
};

void(vector mousepos) editor_particles_overlay =
{
	string n;
	float ef;
	vector y;
	vector col;
	string setname = cvar_string("ca_particleset");

	if (!numptypes)
	{
		loadparticles(setname);
	}

	if (mousedown & 4 && cureffect >= 0 && cureffect < numptypes)
	{
		vector t = unproject(mousepos + '0 0 8192');
		vector o = unproject(mousepos);
		traceline(o, t, TRUE, world);

		trace_endpos += trace_plane_normal * 4;
		if (!ptrailent)
		{
			ptrailent = spawn();
			ptrailent.origin = trace_endpos;

			pointparticles(particleeffectnum(pdesc[cureffect].efname), trace_endpos, '0 0 -1', 1);
		}
		
		//fixme: should ONLY be done if we started dragging.
		trailparticles(particleeffectnum(pdesc[cureffect].efname), ptrailent, ptrailent.origin, trace_endpos);
		ptrailent.origin = trace_endpos;
	}
	else	if (ptrailent)
	{
		remove(ptrailent);
		ptrailent = world;
	}

	if (mousepos_y > 16 && mousepos_y < 24 && mousepos_x < 128)
		drawstring('0 16 0', strcat(setname, ((time*4)&1)?"^1\x0b":""), '8 8 0', '1 1 1', 1, 0);
	else
		drawstring('0 16 0', ((setname=="")?"NO SET LOADED":setname), '8 8 0', '1 1 1', 1, 0);

	ef = cureffect - 10;
	if (ef < 0)
		ef = 0;
	y = '0 32 0';
	for (; ; ef+=1)
	{
		if (ef >= numptypes)
			break;
		n = pdesc[ef].efname;

		col = '1 1 1';
		if (ef == cureffect)
			col = '1 0 0';
		drawrawstring(y, n, '8 8 0', col, 1, 0);
		y_y += 8;
	}

	textfield_draw('128 32 0', &tf_particle, &fields[0]);
};
