
typedef struct
{
	int pwidth;
	int pheight;
	int pwidthmask;
	int pheightmask;
	int pitch;
	unsigned int data[1];
} swimage_t;

typedef struct 
{
	float matrix[16];
	vec4_t viewplane;
} swuniforms_t;

typedef struct
{
	volatile unsigned int readpoint;	//the command queue point its reading from
	void *thread;

#ifdef _DEBUG
	float idletime;
	float activetime;
#endif

	unsigned int interlaceline;
	unsigned int interlacemod;
	unsigned int threadnum;	//for relocating viewport info
	unsigned int *vpdbuf;
	unsigned int *vpcbuf;
	unsigned int vpwidth;
	unsigned int vpheight;
	swuniforms_t u;
	qintptr_t vpcstride;
	struct workqueue_s *wq;
} swthread_t;

typedef struct
{
	int scoord[2];
	float zicoord;
	vec4_t vcoord;
	vec2_t tccoord;
	vec2_t lmcoord;
	byte_vec4_t colour;
	unsigned int clipflags;	/*1=left,2=right,4=top,8=bottom,16=near*/
} swvert_t;

#define WQ_SIZE 1024*1024*8
#define WQ_MASK (WQ_SIZE-1)
#define WQ_MAXTHREADS 64
struct workqueue_s
{
	unsigned int numthreads;
	qbyte queue[WQ_SIZE];
	volatile unsigned int pos;

	swthread_t swthreads[WQ_MAXTHREADS];
};
extern struct workqueue_s commandqueue;



enum wqcmd_e
{
	WTC_DIE,
	WTC_SYNC,
	WTC_NEWFRAME,
	WTC_NOOP,
	WTC_VIEWPORT,
	WTC_TRIFAN,
	WTC_TRISOUP,
	WTC_SPANS,
	WTC_UNIFORMS
};

enum
{
	CLIP_LEFT_FLAG		= 1,
	CLIP_RIGHT_FLAG		= 2,
	CLIP_TOP_FLAG		= 4,
	CLIP_BOTTOM_FLAG	= 8,
	CLIP_NEAR_FLAG		= 16,
	CLIP_FAR_FLAG		= 32
};

typedef union
{
	unsigned char align[16];

	struct wqcom_s
	{
		enum wqcmd_e command;
		unsigned int cmdsize;
	} com;
	struct
	{
		struct wqcom_s com;

		swimage_t *texture;
		int numverts;
		swvert_t verts[1];
	} trifan;
	struct
	{
		struct wqcom_s com;

		swimage_t *texture;
		int numverts;
		int numidx;
		swvert_t verts[1];
	} trisoup;
	struct
	{
		struct wqcom_s com;

		unsigned int *cbuf;
		unsigned int *dbuf;
		unsigned int width;
		unsigned int height;
		qintptr_t stride;
		unsigned int interlace;
		unsigned int framenum;

		qboolean cleardepth;
		qboolean clearcolour;
	} viewport;
	struct
	{
		struct wqcom_s com;
		swuniforms_t u;
	} uniforms;
	struct
	{
		int foo;
	} spans;
} wqcom_t;



void SWRast_EndCommand(struct workqueue_s *wq, wqcom_t *com);
wqcom_t *SWRast_BeginCommand(struct workqueue_s *wq, int cmdtype, unsigned int size);
void SWRast_Sync(struct workqueue_s *wq);



qboolean SW_VID_Init(rendererstate_t *info, unsigned char *palette);
void SW_VID_DeInit(void);
qboolean SW_VID_ApplyGammaRamps		(unsigned int rampcount, unsigned short *ramps);
char *SW_VID_GetRGBInfo(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt);
void SW_VID_SetWindowCaption(const char *msg);
void SW_VID_SwapBuffers(void);
void SW_VID_UpdateViewport(wqcom_t *com);




void		SW_UpdateFiltering		(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float anis);
qboolean	SW_LoadTextureMips		(texid_t tex, const struct pendingtextureinfo *mips);
void		SW_DestroyTexture		(texid_t tex);


void SWBE_SelectMode(backendmode_t mode);
void SWBE_DrawMesh_List(shader_t *shader, int nummeshes, struct mesh_s **mesh, struct vbo_s *vbo, struct texnums_s *texnums, unsigned int be_flags);
void SWBE_DrawMesh_Single(shader_t *shader, struct mesh_s *meshchain, struct vbo_s *vbo, unsigned int be_flags);
void SWBE_SubmitBatch(struct batch_s *batch);
struct batch_s *SWBE_GetTempBatch(void);
void SWBE_DrawWorld(batch_t **worldbatches);
void SWBE_Init(void);
void SWBE_GenBrushModelVBO(struct model_s *mod);
void SWBE_ClearVBO(struct vbo_s *vbo, qboolean dataonly);
void SWBE_UploadAllLightmaps(void);
void SWBE_SelectEntity(struct entity_s *ent);
qboolean SWBE_SelectDLight(struct dlight_s *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode);
qboolean SWBE_LightCullModel(vec3_t org, struct model_s *model);
void SWBE_RenderToTextureUpdate2d(qboolean destchanged);
void SWBE_Set2D(void);
