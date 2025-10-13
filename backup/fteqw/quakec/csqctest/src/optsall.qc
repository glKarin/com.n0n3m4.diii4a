//which engines do we want to support?
#define FTE
//#define QSS
//#define DP

//#define OWNPLAYERPHYSICS	//run our own prediction code, instead of the engine-supplied default

#define AUTOCVAR

#define MD3PMODELS	//support Q3 segmented player models
#define HLPMODELS		//support HalfLife skeletal models
#define POWERUP_SHELLS	//show shells around players for powerups
#define Q4PMODELS
//#define NOEXTENSIONS

#ifdef NOEXTENSIONS
#undef HLPMODELS
#undef MD3PMODELS
#endif

#ifdef DP
	#define WORKINDP	//do various workarounds for DP. don't use any opcode extensions
#elifdef QSS
//	#pragma TARGET QSS	//FTE also supports this.
	#define NOT_DP		//mute deprecation warnings about DP, we don't care.
#else
	#pragma TARGET FTE	//neither of the other engines are targetted, go crazy with it.
	#define NOT_QSS		//mute deprecation warnings about QSS, we don't care.
	#define NOT_DP		//mute deprecation warnings about DP, we don't care.
#endif

//just mute engine-dependancy deprecation warnings for now.
#define NOT_QSS
#define NOT_DP



















#ifdef AUTOCVAR
#define DEFCVAR_FLOAT(n,d) var float autocvar_##n = d;
#define DEFCVAR_STRING(n,d) var string autocvar_##n = d;
#define CVARF(n) autocvar_##n
#define CVARS(n) autocvar_##n
#else
#define DEFCVAR_FLOAT(n,d)
#define DEFCVAR_STRING(n,d)
#define CVARF(n) cvar(#n)
#define CVARS(n) cvar_string(#n)
#endif
