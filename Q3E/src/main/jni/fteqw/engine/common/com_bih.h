#if 1
	#define BIH_USEBIH
#else
	#define BIH_USEBVH
#endif

typedef struct q2mapsurface_s  // used internally due to name len probs //ZOID
{
	q2csurface_t	c;
	char		rname[32];
} q2mapsurface_t;
typedef struct
{
	mplane_t	*plane;
	q2mapsurface_t	*surface;
} q2cbrushside_t;
typedef struct q2cbrush_s
{
	int			checkcount;		// to avoid repeated testings
	int			contents;
	vec3_t		absmins;
	vec3_t		absmaxs;
	int			numsides;
	q2cbrushside_t *brushside;
} q2cbrush_t;
typedef struct
{
	vec3_t		absmins, absmaxs;

	vecV_t		*xyz_array;
	size_t numverts;
	index_t		*indicies;
	size_t numincidies;

	q2mapsurface_t	*surface;
	int			checkcount;		// to avoid repeated testings
} q3cmesh_t;
typedef struct
{
	vec3_t		absmins, absmaxs;

	int			numfacets;
	q2cbrush_t	*facets;
	q2mapsurface_t	*surface;
	int			checkcount;		// to avoid repeated testings
} q3cpatch_t;


enum bihtype_e
{
	//node types
#ifdef BIH_USEBIH
	BIH_X,
	BIH_Y,
	BIH_Z,
#endif
#ifdef BIH_USEBVH
	BVH_X,
	BVH_Y,
	BVH_Z,
#endif
	BIH_GROUP,


	//leaf types
#if defined(Q2BSPS) || defined(Q3BSPS)
	BIH_BRUSH,
#endif
#ifdef Q3BSPS
	BIH_PATCHBRUSH,
	BIH_TRISOUP,
#endif
	BIH_TRIANGLE,
	BIH_MODEL,
};
struct bihdata_s
{
	unsigned int contents;
	union {
#if defined(Q2BSPS) || defined(Q3BSPS)
		q2cbrush_t *brush;
#endif
#ifdef Q3BSPS
		q2cbrush_t *patchbrush;
		q3cmesh_t *cmesh;
#endif
		struct {
//			vec3_t norm;
			index_t *indexes;	//might be better to just bake 3 indexes instead of using a pointer to them
			vecV_t *xyz;
		} tri;
		struct {
			model_t *model;
			struct bihtransform_s
			{
				vec3_t axis[3];
				vec3_t origin;
			} *tr;
		} mesh;
	};
};
struct bihleaf_s
{
	enum bihtype_e type;
	vec3_t mins;
	vec3_t maxs;
	struct bihdata_s data;
};

struct galiasinfo_s;

//generates a BIH tree and updates mod->funcs.NativeTrace|NativeContents funcs
void BIH_Build (model_t *mod, struct bihleaf_s *items, size_t numitems);
void BIH_BuildAlias (model_t *mod, struct galiasinfo_s *meshes);
