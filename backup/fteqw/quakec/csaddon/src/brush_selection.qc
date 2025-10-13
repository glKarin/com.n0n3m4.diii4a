var float 	autocvar_ca_brush_view = 0;		//0=normal, 1=x, 2=y, 3=z
var float 	autocvar_ca_brush_viewsize = 1024;	//for different views.
var string	autocvar_ca_newbrushtexture = "metal4_2";
var float	autocvar_ca_newbrushheight = 64;
var float	autocvar_ca_grid = 16;
var float	autocvar_ca_explicitpatch = 0;
var float	autocvar_ca_cpwidth = 2;
var float	autocvar_ca_cpheight = 2;


#define EPSILON (1.0 / 32)	//inprecision sucks.

#define MAX_FACEPOINTS 64
vector facepoints[MAX_FACEPOINTS];	//for temp usage

#define MAX_BRUSHFACES 64
struct
{
	//brush state
	brushface_t faces[MAX_BRUSHFACES];
	int numfaces;
	int contents;

	//patch state
	patchvert_t *cp;
	int numcp, maxcp;
	patchinfo_t patchinfo;
} tmp;



//int selected;
int brushlist[2048];

enum : int
{
	BT_NONE,	//selection
	BT_MOVE = BT_NONE,
	BT_ROTATE,
	BT_MERGE,
	BT_PUSHFACE,
	BT_CREATEBRUSH,
	BT_CREATEPATCH,
	BT_CREATEDRAG,
	BT_CLONEDISPLACE,
	BT_SLICE,
	BT_MOVETEXTURE,
	BT_VERTEXEDIT
};
int mousetool;
int brushtool;
int bt_points;
vector bt_displace;
vector bt_point[64];
int nogrid;	//+nogrid, temporarily disables grid locks





typedef struct
{
	int model;
	int id;
	__uint64 facemask;	//MAX_BRUSHFACES is 64... handy.
} selbrush_t;
selbrush_t *selectedbrushes;
int selectedbrushcount;

var int selectedbrushmodel = 1;	//by default, the worldmodel. this is tracked to know which submodel to insert new brushes into.

int(int modelidx, int brushid) brush_isselected =	//0 for faceid means 'all'
{
	for (int i = 0; i < selectedbrushcount; i++)
		if (selectedbrushes[i].id == brushid)
			if (selectedbrushes[i].model == modelidx)
				return i+1;
	return 0;
};
int(int modelidx, int brushid, int faceid) brushface_isselected =	//0 for faceid means 'all'
{
	for (int i = 0; i < selectedbrushcount; i++)
		if (selectedbrushes[i].id == brushid)
			if (selectedbrushes[i].model == modelidx)
				return !!(selectedbrushes[i].facemask&(1ull<<(faceid-1i)));
	return 0;
};
int(int modelidx, int brushid) brush_deselect =
{
	int i = brush_isselected(modelidx, brushid);
	if (!i)
		return FALSE;
	brush_selected(modelidx, brushid, -1, FALSE);
	memcpy(&selectedbrushes[i-1], &selectedbrushes[i], sizeof(selbrush_t)*(selectedbrushcount-i));
	selectedbrushcount--;
	return TRUE;
};
int(int modelidx, int brushid, int face) brushface_deselect =
{
	//deselecting a single face
	int i = brush_isselected(modelidx, brushid);	//get the brush selection index
	if (!i)
		return FALSE;
	i--;
	face--;
	if (!selectedbrushes[i].facemask)	//no faces means all..
		selectedbrushes[i].facemask = ~0ul;
	selectedbrushes[i].facemask &= ~(1ul<<face);
	if (!selectedbrushes[i].facemask)
	{	//no faces left selected.
		brush_selected(modelidx, brushid, -1, FALSE);
		memcpy(&selectedbrushes[i], &selectedbrushes[i+1], sizeof(selbrush_t)*(selectedbrushcount-i));
		selectedbrushcount--;
	}
	return TRUE;
};
void() brush_deselectall =
{
	for (int i = 0; i < selectedbrushcount; i++)
		brush_selected(selectedbrushes[i].model, selectedbrushes[i].id, -1, FALSE);

	selectedbrushcount = 0;
};
void(int modelidx, int brushid) brush_select =
{
	if (!brush_isselected(modelidx, brushid))
	{
		selbrush_t *n = memalloc(sizeof(selbrush_t) * (selectedbrushcount+1));
		memcpy(n, selectedbrushes, sizeof(selbrush_t)*selectedbrushcount);
		memfree(selectedbrushes);
		selectedbrushes = n;
		n[selectedbrushcount].model = modelidx;
		n[selectedbrushcount].id = brushid;
		n[selectedbrushcount].facemask = ~0ul;
		selectedbrushcount++;
		brush_selected(modelidx, brushid, -1, TRUE);
	}
	selectedbrushmodel = modelidx;
};
void(int modelidx, int brushid, int faceid) brushface_select =
{
	faceid--;
	int i = brush_isselected(modelidx, brushid);	//get the brush selection index
	if (!i)
	{	//brush not selected yet
		selbrush_t *n = memalloc(sizeof(selbrush_t) * (selectedbrushcount+1));
		memcpy(n, selectedbrushes, sizeof(selbrush_t)*selectedbrushcount);
		memfree(selectedbrushes);
		selectedbrushes = n;
		n[selectedbrushcount].model = modelidx;
		n[selectedbrushcount].id = brushid;
		n[selectedbrushcount].facemask = 0;
		selectedbrushcount++;
		brush_selected(modelidx, brushid, -1, TRUE);
		i = selectedbrushcount;
	}
	selectedbrushes[i-1].facemask |= 1ul<<faceid;
	selectedbrushmodel = modelidx;
};