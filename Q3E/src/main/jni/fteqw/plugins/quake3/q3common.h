#ifndef FTEPLUGIN
#define FTEPLUGIN
#endif
#include "../plugins/plugin.h"
#include "clq3defs.h"

#define VM_TOSTRCACHE(a) VMQ3_StringToHandle(VM_POINTER(a))
#define VM_FROMSTRCACHE(a) VMQ3_StringFromHandle(a)
char *VMQ3_StringFromHandle(int handle);
int VMQ3_StringToHandle(char *str);
void VMQ3_FlushStringHandles(void);

//#define Q3_NOENCRYPT	//a debugging property, makes it incompatible with q3

#ifdef HAVE_CLIENT
extern plug2dfuncs_t	*drawfuncs;
extern plug3dfuncs_t	*scenefuncs;
extern pluginputfuncs_t	*inputfuncs;
extern plugaudiofuncs_t	*audiofuncs;
extern plugmasterfuncs_t*masterfuncs;
extern plugclientfuncs_t*clientfuncs;
#endif

extern plugq3vmfuncs_t	*vmfuncs;
extern plugfsfuncs_t	*fsfuncs;
extern plugmsgfuncs_t	*msgfuncs;
extern plugworldfuncs_t	*worldfuncs;
extern plugthreadfuncs_t	*threadfuncs;

extern cvar_t *sv_maxclients;
extern cvar_t *cl_shownet_ptr, *cl_c2sdupe_ptr, *cl_nodelta_ptr;

#define Q_snprintfz Q_snprintf
#define Sys_Error plugfuncs->Error

#define Z_Malloc plugfuncs->Malloc
#define Z_Free plugfuncs->Free

typedef struct	//merge?
{
	int					flags;
	int					areabytes;
	qbyte				areabits[MAX_Q2MAP_AREAS/8];		// portalarea visibility bits
	q3playerState_t		ps;
	int					num_entities;
	int					first_entity;		// into the circular sv_packet_entities[]
	int					senttime;			// for ping calculations


	int				serverMessageNum;
	int				serverCommandNum;
	int				serverTime;		// server time the message is valid for (in msec)
	int				localTime;
	int				deltaFrame;
} q3client_frame_t;



#ifdef HAVE_SERVER
typedef struct
{
	world_t *world;
	model_t *models[1u<<MODELINDEX_BITS];

	qboolean restarting;
	float restartedtime;

	vec2_t			gridbias;
	vec2_t			gridscale;
	size_t			gridsize[2];
	areagridlink_t	*gridareas;		//[gridsize[0]*gridsize[1]]
	areagridlink_t	jumboarea;		//node containing ents too large to fit.
	int				areagridsequence;

	server_static_t	*server_state_static;
	server_t		*server_state;
} q3serverstate_t;
extern q3serverstate_t sv3;
#undef CALCAREAGRIDBOUNDS
#endif

