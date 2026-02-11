/*

	return ("
weaponry\n\n
ê1ë melee          \n
ê2ë thrown         \n
ê3ë pistols&smgs   \n
ê4ë shotguns       \n
ê5ë rifles         \n
ê6ë heavy guns     \n
êeë leave          
*/

/*
ê1ë 1911a1     .45   01       5$\n
ê2ë d. eagle   .44   02       7$\n
ê3ë mk23 socom .45   02       9$\n
ê4ë h&k mp7    4mm   03      14$\n
ê5ë h&k mp5    9mm   03      17$\n
ê6ë alien blaster    02      21$\n

ê1ë pipe rifle .44   02       4$\n
ê2ë winchester 12g   03       8$\n
ê3ë mossberg   12g   04      14$\n
ê4ë citykiller 12g   05      35$\n

ê1ë rangemaster 7mm  03      11$\n
ê2ë ak-112      5mm  04      21$\n
ê3ë remington  .308  05      24$\n
ê4ë ak-74       5mm  04      27$\n
ê5ë moonlight  .223  05      36$\n
ê6ë sa-80       5mm  05      23$\n
ê7ë plasma rifle     07      41$\n
ê8ë gauss rifle 2mm  08      51$\n
*/

string () ShopString =
{
	return ("--- S H O P -------\n"
		"\n"
		"ê1ë traits         \n"
		"ê2ë perks          \n"
		"ê3ë body armour    \n"
		"ê4ë protection     \n"
		"ê5ë weapons        \n"
		"ê6ë equipment      \n"
		"ê7ë chems          \n"
		"êeë leave          \n");
};

string () WeaponString =
{
	return ("weaponry\n"
		"\n"
		"ê1ë melee          \n"
		"ê2ë thrown         \n"
		"ê3ë pistols&smgs   \n"
		"ê4ë shotguns       \n"
		"ê5ë rifles         \n"
		"êeë leave          \n");
};

string () TraitString =
{
	return ("traits\n"
		"\n"
		"ê1ë one handed     \n"
		"ê2ë small frame    \n"
		"ê3ë bruiser        \n"
		"ê4ë heavy handed   \n"
		"ê5ë bloody mess    \n"
		"ê6ë bad luck       \n"
		"êeë leave          \n");
};

string () MiscString =
{
	return ("grenades\n"
		"	GRENADE | COST		  \n"
		"\n"
		"ê1ë plasma grenade            11\n"
		"ê2ë frag grenade               5\n"
		"ê3ë emp grenade                3\n"
		"ê4ë smoke grenade              2\n"
		"ê5ë flash bang mark ii         2\n"
		"êeë exit                        \n");
};

string () BuildString =
{
	return ("BUILD A STRUCTURE\n"
		"      NAME | UPGRADE | COST	  \n"
		"\n"
		"ê1ë supply depot   ammo/hp     4\n"
		"ê2ë arms lab     weapons +10%  6\n"
		"ê3ë armory        armour +10% 10\n"
		"ê4ë proto-lab  gadgets/hrdwr  15\n"
		"êeë exit                        \n");
};

string () HelmetString =
{
	return ("helmets\nprotect you from headshots so\n"
		"make sure you use a decent one\n"
		"\n"
		"                  ABS% WT DEFLECT PRC\n"
		"ê1ë combat helm   -10% 01    5%    05\n"
		"ê2ë combat helm 2  +0% 02    5%    20\n"
		"ê3ë heavy-duty    +20% 03    5%    20\n"
		"ê4ë ceramic helm  -20% ...");
};

string () ArmorString1 =
{
	return ("body armour\n"
		"                      wt  abs  prc\n"
		"\n"
		"ê1ë vault suit        03 1/20% 03$\n"
		"ê2ë leather armor     07 2/30% 05$\n"
		"ê3ë kevlar armor      10 3/35% 08$\n"
		"ê4ë combat armor      13 4/40% 11$\n"
		"ê5ë brotherhood armor 17 5/45% 15$");
};

string () ArmorString2 =
{
	return ("body armour\n"
		"                         WT ABS SPEC PRC\n"
		"\n"
		"ê1ë battle gear          08 57% NONE  50\n"
		"ê2ë battle gear ii       09 62% !EXP  60\n"
		"ê3ë high-tech suit       09 42% +ALL  60\n"
		"ê4ë carbon-fiber mesh    09 56% !ENR  70\n"
		"ê5ë pseudo-chitin suit   07...");
};

string () ArmorString3 =
{
	return ("special body armour\n"
		"                      WT ABS SPEC PRC\n"
		"ê1ë vsn force mail    07 %40 DFLC 95c\n"
		"ê2ë super chitin      08 %35 DFLC 95c\n"
		"ê3ë suit of mota      14 %70 HEAL 95c\n"
		"ê0ë previous                         \n"
		"êeë exit                           ...");
};

string () PlusString =
{
	return ("perks\n"
		"  ABILITY | FRAGS NEEDED \n"
		"\n"
		"ê1ë bonus movement       8\n"
		"ê2ë weapons handling    12\n"
		"ê3ë quick pockets       14\n"
		"ê4ë awareness           15\n"
		"ê5ë silent running      17\n"
		"ê6ë better criticals    23\n"
		"ê7ë bonus ranged damage 26\n"
		"ê8ë strong back        ...");
};

string () PlusString2 =
{
	return ("perks\n"
		"  ABILITY | FRAGS NEEDED \n"
		"\n"
		"ê1ë bonus movement       8\n"
		"ê2ë weapons handling    12\n"
		"ê3ë quick pockets       14\n"
		"ê4ë awareness           15\n"
		"ê5ë silent running      17\n"
		"ê6ë better criticals    23\n"
		"ê7ë bonus ranged damage 26\n"
		"ê8ë strong back        ...");
};
/*
ê5ë energy shield  |front :25% ê25ë
ê6ë protect ring   |damage:15% ê65ë
ê7ë dark force     |deflct:15% ê75ë
ê8ë efreeti module |blasts 40% ê85ë
ê9ë sentient cube  |regenerate ê95ë
ê0ë vampire        |space warp ê95ë
*/


string () HardwareString =
{
	return ("+ protective devices +\n\n  HARDWARE | SHIELDS VS |  PRICE   \nê1ë pro cloak      |damage:07% ê15ë\nê2ë emp shielding  |emp:  100% ê25ë\nê3ë force shield   |front: 25% ê20ë\nê4ë force field    |damage:10% ê25ë\nê5ë energy shield  |energy:23% ê25ëê5ë energy shield  |front :25% ê25ëê6ë protect ring   |damage:15% ê65ëê7ë dark force     |deflct:15% ê75ëê8ë efreeti module |blasts 40% ê85ëê9ë sentient cube  |regenerate ê95ëê0ë vampire        |space warp ê95ë");
};

string () MeleeString =
{
	return ("melee weapons\n    WEAPON | TYPE | WEIGHT | PRICE \n\nê1ë knife              melee 01 01$\nê2ë combat knife       melee 03 03$\nê3ë ripper             melee 03 07$\nê4ë power fist         melee 04 11$\nêeë exit                           \n");
};

/*
ê6ë h&k mp5   9mmP   03      17$\n
ê7ë h&k mp7  4.60mm  03      14$\n
ê8ë fn p90   5.57mm  03      22$\n
ê9ë h&k mp10  10mm   03      24$\n
ê0ë thompson  .45    03      20$\n


ê6ë h&k mp5   9mmP   03      17$\nê7ë h&k mp7  4.60mm  03      14$\nê8ë fn p90   5.57mm  03      22$\nê9ë h&k mp10  10mm   03      24$\nê0ë thompson  .45    03      20$\n
*/

string () SmallArmString1 =
{
	return ("Pistols and Submachineguns\n    WEAPON | CAL | WEIGHT | PRICE \n\nê1ë 1911a1     .45   01       5$\nê2ë d. eagle   .44   02       7$\nê3ë mk23 socom .45   02       9$\nê4ë h&k mp7    4mm   03      14$\nê5ë h&k mp5    9mm   03      17$\nê6ë alien blaster    02      21$\n");
};

string () SmallArmString2 =
{
	return ("shotguns\n    WEAPON | TYPE | WEIGHT | PRICE \n\nê1ë pipe rifle .44   02       4$\nê2ë winchester 12g   03       8$\nê3ë mossberg   12g   04      14$\nê4ë citykiller 12g   05      35$\n");
};

/*
ê6ë dks-1      .338   bolt 08 32$\n
ê7ë moonlight  .223   auto 06 54$\n
ê8ë xl70e3      5mm   auto 08 27$\n
ê9ë fn-fal     .308   auto 04 51$\n
ê0ë sa-80      .223   auto 07 45$\n

*/
string () SmallArmString3 =
{
	return ("rifles\n   RIFLE | TYPE | WEIGHT | PRICE \n\nê1ë rangemaster 7mm  03      11$\nê2ë ak-112      5mm  04      21$\nê3ë remington  .308  05      24$\nê4ë ak-74       5mm  04      27$\nê5ë moonlight  .223  05      36$\nê6ë sa-80       5mm  05      23$\nê7ë plasma rifle     07      41$\nê8ë gauss rifle 2mm  08      51$\n");
};

string () DrugString1 =
{
	return ("drugs\n     DRUG  |   EFFECTS   |  PRICE\n"
		"\n""ê1ë stimpack   heals 40        3$\n"
		"ê2ë medkit+    heals 20+50     5$\n"
		"ê3ë superstim* heals 40+60    12$\n"
		"êeë exit                         \n"
		"\n"
		"\n"
		"+ requires medic                 \n"
		"* requires medic with 7+ kills");
};

string () DrugString2 =
{
	return (
		"drugs\n     DRUG  |   EFFECTS   |  PRICE\n"
		"\n"
		"ê1ë stimpack   heals 40        3$\n"
		"ê2ë adrenaline +60 speed/jump  7$\n"
		"ê3ë psycho+    +60 hp/no pain 15$\n"
		"ê4ë medkit*    heals 20+50    12$\n"
		"ê5ë berserk*   adren+psycho   21$\n"
		"êeë exit                         \n");
};

string () EnergyWeaponsString =
{
	return ("high-tech weaponry\n  WEAPON | TYPE | WEIGHT | PRICE \n\nê1ë [*] flash gun         semi 03 21$\nê2ë [&] plasma rifle      semi 08 34$\nê3ë [*] laser rifle       semi 11 40$\nê4ë [*] laser carbine     auto 06 57$\nê5ë [?] alien blaster     semi 02 72$\n");
};

//ê6ë bozar                    14 81$\nê7ë firestorm                12 97$

string () HeavyGunsString =
{
	return ("heavy guns\n    WEAPON | TYPE | WEIGHT | PRICE \n\nê1ë light support weapon     15 55$\nê2ë rocket launcher          11 75$\nê3ë 50oc flamethrower        16 35$\nê4ë steyr amr .50 flechette  17 72$\nê5ë m72 gauss rifle 2mm      12 81$\nê6ë bozar                    14 81$\nê7ë firestorm                12 97$");
};

string () de_dust =
{
	return ("             DE_DUST          \n        (BOMB/DEFUSE MAP)     \n\nrangers have obtained two ufos\nthat have crash landed in the \ndesert. raiders must blow them\nup with c4 before its too late\n\n(activate electronic tools and\nc4 by pressing 4)         ...");
};

/*
ê4ë motion sensor                 ê20cë\n
ê5ë extra magazines               ê20cë\n
ê6ë electronic tools mark ii      ê30cë\n
ê7ë climbing gear                 ê30cë\n
ê8ë remote camera                 ê40cë\n
ê9ë cooling module                ê50cë\n
ê0ë laser defense field           ê50cë\n
*/



string () GadgetString =
{
	return ("+ special equipment +\npress your c key to activate!\n\nê1ë stealth boy                   ê20cë\nê2ë displacer cloak               ê20cë\nê3ë security alarm                ê20cë\nê4ë motion sensor                 ê20cë\nê5ë extra magazines               ê20cë\nê6ë electronic tools mark ii      ê30cë\nê7ë climbing gear                 ê30cë\nê8ë remote camera                 ê40cë\nê9ë cooling module                ê50cë\nê0ë laser defense field           ê50cë\n");
};
