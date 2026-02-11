//addons cannot use certain optimisations - the engine MUST be able to find+fix their string+function indexes.
#pragma optimise addon	//disables any optimisations that would break addons.

//#define CAMQUAKE

//with addons, the engine only tracks one set of globals for most things.
//thus we use pointers in the addon to refer to that set of globals.
//this avoids having to make things shared, and having to pay for every transition.

vector *ptr_trace_endpos;
#define trace_endpos (*ptr_trace_endpos)

vector *ptr_trace_plane_normal;
#define trace_plane_normal (*ptr_trace_plane_normal)

entity *ptr_trace_ent;
#define trace_ent (*ptr_trace_ent)

entity *ptr_self;
#define self 		(*ptr_self)

void() csfixups =
{
	ptr_trace_endpos		= (vector*)externvalue(0, "&trace_endpos");
	ptr_trace_plane_normal	= (vector*)externvalue(0, "&trace_plane_normal");
	ptr_trace_ent			= (entity*)externvalue(0, "&trace_ent");
	ptr_self				= (entity*)externvalue(0, "&self");
};

int *world_hitcontentsmaski; //evilness, so we can assign to world without errors.

vector mousenear;
vector mousefar;
float mousedown;
float shiftdown;
float ctrldown;
float altdown;
vector curmousepos;



enum
{
	MODE_OPTIONS,

	MODE_LIGHTEDIT,
	MODE_SPLINEEDIT,
	MODE_TERRAINEDIT,
	MODE_BRUSHEDIT,
	MODE_PARTICLEEDIT,
	MODE_ENTSEDIT,

	MODE_COUNT
};

var string editornames[MODE_COUNT] = {"OPTIONS","LIGHTS","SPLINES", "TERRAIN", "BRUSHES", "PARTICLES", "ENTITIES"};

var float(float vismode) ca_checksave;


#define isnull(v) (!(int)(v))
#define notnull(v) ((int)(v))
