#include "quakedef.h"

#if !defined(CLIENTONLY) && defined(NETPREPARSE)
/*Testing this code should typically be done with the three following mods:
Prydon gate
Nexuiz
FrikBots (both NQ+QW).

If those 3 mods work, then pretty much everything else will
*/

//#define NEWPREPARSE

/*I want to rewrite this
to use something like
*/

#ifdef NEWPREPARSE
enum protocol_type
{
	PPT_FLOAT,
	PPT_ENT,
	PPT_COORD,
	PPT_ANGLE,
	PPT_BYTE,
	PPT_SHORT,
	PPT_LONG,
	PPT_STRING
};

#define PPT_POS PPT_COORD,PPT_COORD,PPT_COORD

union protocol_data
{
	float	fd;
	int		id;
	unsigned char	*str;
};

static union protocol_data pp_data[1024];
static enum protocol_type pp_temptypes[1024], *pp_types;
static unsigned char pp_sdata[4096];
static unsigned int pp_sdata_offset;
static int pp_dest;
static qboolean pp_fault;

static unsigned int pp_expectedelements;
static unsigned int pp_receivedelements;
static qboolean (*pp_curdecision) (enum protocol_type *pt, union protocol_data *pd);

static enum protocol_type pp_root[] = {PPT_BYTE};
static qboolean pp_root_decide(enum protocol_type *pt, union protocol_data *pd);

static void decide(enum protocol_type *types, unsigned int numtypes, qboolean (*newdecision) (enum protocol_type *pt, union protocol_data *pd))
{
	pp_types = types;
	pp_expectedelements = numtypes;
	pp_curdecision = newdecision;
}

static void pp_flush(multicast_t to, vec3_t origin, void (*flushfunc)(client_t *cl, sizebuf_t *msg, enum protocol_type *pt, union protocol_data *pd), enum protocol_type *pt, union protocol_data *pd)
{

	client_t	*client;
	qbyte		*mask;
	int			leafnum;
	int			j;
	qboolean	reliable;

	decide(pp_root, 1, pp_root_decide);



	{
		reliable = false;

		switch (to)
		{
		case MULTICAST_ALL_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_ALL:
			mask = NULL;
			break;

		case MULTICAST_PHS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PHS:
			if (!sv.phs)
				mask = NULL;
			else
			{
				cluster = sv.world.worldmodel->funcs.LeafnumForPoint(sv.world.worldmodel, origin);
				if (cluster >= 0)
					mask = sv.phs + cluster * 4*((sv.world.worldmodel->numclusters+31)>>5);
				else
					mask = NULL;
			}
			break;

		case MULTICAST_PVS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PVS:
			cluster = sv.world.worldmodel->funcs.LeafnumForPoint(sv.world.worldmodel, origin);
			if (cluster >= 0)
				mask = sv.pvs + cluster * 4*((sv.world.worldmodel->numclusters+31)>>5);
			else
				mask = NULL;
			break;

		default:
			return;
		}

		// send the data to all relevent clients
		for (j = 0, client = svs.clients; j < sv.allocated_client_slots; j++, client++)
		{
			if (client->state != cs_spawned)
				continue;

			if (client->controller)
				continue;	//FIXME: send if at least one of the players is near enough.

			if (!((int)client->edict->xv->dimension_see & (int)pr_global_struct->dimension_send))
				continue;

			if (to == MULTICAST_PHS_R || to == MULTICAST_PHS)
			{
				vec3_t delta;
				VectorSubtract(origin, client->edict->v->origin, delta);
				if (Length(delta) <= 1024)
					goto inrange;
			}

			if (mask)
			{
				cluster = sv.world.worldmodel->funcs.ClusterForPoint (sv.world.worldmodel, client->edict->v->origin);
				if (cluster >= 0 && !(mask[leafnum>>3] & (1<<(leafnum&7)) ) )
				{
					continue;
				}
			}

inrange:
			if (client->protocol == SCP_BAD)
			{
				/*bot*/
				continue;
			}
			if (reliable)
				flushfunc(client, &client->netchan.message, pt, pd);
			else
				flushfunc(client, &client->datagram, pt, pd);
		}
	}
/*
	if (sv.mvdrecording && !with)	//mvds don't get the pext stuff
	{
		flushfunc(&dem.recorder,
		if (reliable)
		{
			MVDWrite_Begin(dem_all, 0, sv.multicast.cursize);
			SZ_Write(&demo.dbuf->sb, sv.multicast.data, sv.multicast.cursize);
		} else
			SZ_Write(&demo.datagram, sv.multicast.data, sv.multicast.cursize);
	}*/

	pp_sdata_offset = 0;
	pp_receivedelements = 0;
}

#define DECIDE(t) decide(t, sizeof(t)/sizeof(*t), t##_decide)
#define DECIDE2(t,f) decide(t, sizeof(t)/sizeof(*t), f)

static void pp_identity_flush(client_t *cl, sizebuf_t *msg, enum protocol_type *pt, union protocol_data *pd)
{
	unsigned int i;
	for (i = 0; i < pp_receivedelements; i++)
	{
		switch(pt[i])
		{
		case PPT_BYTE:
			MSG_WriteByte(msg, pd[i].id);
			break;
		case PPT_ENT:
			MSG_WriteEntity(msg, pd[i].id);
			break;
		case PPT_SHORT:
			MSG_WriteShort(msg, pd[i].id);
			break;
		case PPT_COORD:
			MSG_WriteCoord(msg, pd[i].fd);
			break;
		case PPT_ANGLE:
			MSG_WriteAngle(msg, pd[i].fd);
			break;
		case PPT_STRING:
			MSG_WriteString(msg, pd[i].str);
			break;
		}
	}
}

/*flush is our last attempt to cope with unrecognised/invalid messages, it will send stuff though as it was, and most likely get things wrong*/
void NPP_Flush(void)
{
	pp_fault = false;
	pp_flush(MULTICAST_ALL_R, NULL, pp_identity_flush, pp_types, pp_data);
}

static void pp_entry(int dest, enum protocol_type pt, union protocol_data pd)
{
	if (pp_receivedelements)
	{
		if (pp_dest != dest)
		{
			if (!pp_fault)
				Con_Printf("Preparse: MSG destination changed in the middle of a packet 0x%x.\n", pp_data[0].id);
			NPP_Flush();
		}
	}
	pp_dest = dest;

	if (pp_fault)
	{
		pp_types[pp_receivedelements] = pt;
		pp_data[pp_receivedelements] = pd;
		pp_receivedelements++;
	}
	else if (pp_types[pp_receivedelements] != pt)
	{
		Con_Printf("Preparse: Unmatched expectation at entry %i in svc 0x%x.\n", pp_receivedelements+1, pp_data[0].id);

		pp_types[pp_receivedelements] = pt;
		pp_data[pp_receivedelements] = pd;
		pp_receivedelements++;

faulted:
		pp_fault = true;
		if (pp_temptypes != pp_types)
		{
			memcpy(pp_temptypes, pp_types, sizeof(*pp_temptypes)*pp_receivedelements);
			pp_types = pp_temptypes;
		}
	}
	else
	{
		pp_data[pp_receivedelements++] = pd;

		if (pp_expectedelements == pp_receivedelements)
		{
			if (!pp_curdecision(pp_types, pp_data))
			{
				if (pp_types[pp_receivedelements-1] == PPT_BYTE)
					Con_Printf("Preparse: Unhandled byte %i@%i in svc%i.\n", pd.id, pp_receivedelements, pp_data[0].id);
				else
					Con_Printf("Preparse: Unhandled data @%i in svc%i.\n", pp_receivedelements, pp_data[0].id);
				goto faulted;
			}
		}
	}
}

void NPP_NQWriteByte(int dest, qbyte data)
{
	union protocol_data pd;
	pd.id = data;
	pp_entry(dest, PPT_BYTE, pd);
}
void NPP_NQWriteChar(int dest, char data)
{
	union protocol_data pd;
	pd.id = (unsigned char)data;
	pp_entry(dest, PPT_BYTE, pd);
}
void NPP_NQWriteShort(int dest, short data)
{
	union protocol_data pd;
	pd.id = (unsigned char)data;
	pp_entry(dest, PPT_SHORT, pd);
}
void NPP_NQWriteLong(int dest, long data)
{
	union protocol_data pd;
	pd.id = data;
	pp_entry(dest, PPT_LONG, pd);
}
void NPP_NQWriteAngle(int dest, float data)
{
	union protocol_data pd;
	pd.fd = data;
	pp_entry(dest, PPT_ANGLE, pd);
}
void NPP_NQWriteCoord(int dest, float data)
{
	union protocol_data pd;
	pd.fd = data;
	pp_entry(dest, PPT_COORD, pd);
}
void NPP_NQWriteString(int dest, char *data)
{
	unsigned int l;
	union protocol_data pd;
	l = strlen(data)+1;
	if (pp_sdata_offset + l > sizeof(pp_sdata))
		SV_Error("preparse string overflow\n");
	pd.str = pp_sdata + pp_sdata_offset;
	memcpy(pd.str, data, l);
	pp_entry(dest, PPT_STRING, pd);
}
void NPP_NQWriteEntity(int dest, short data)
{
	union protocol_data pd;
	pd.id = (unsigned short)data;
	pp_entry(dest, PPT_ENTITY, pd);
}

void NPP_QWWriteByte(int dest, qbyte data)
{
	NPP_NQWriteByte(dest, data);
}
void NPP_QWWriteChar(int dest, char data)
{
	NPP_NQWriteChar(dest, data);
}
void NPP_QWWriteShort(int dest, short data)
{
	NPP_NQWriteShort(dest, data);
}
void NPP_QWWriteLong(int dest, long data)
{
	NPP_NQWriteLong(dest, data);
}
void NPP_QWWriteAngle(int dest, float data)
{
	NPP_NQWriteAngle(dest, data);
}
void NPP_QWWriteCoord(int dest, float data)
{
	NPP_NQWriteCoord(dest, data);
}
void NPP_QWWriteString(int dest, char *data)
{
	NPP_NQWriteString(dest, data);
}
void NPP_QWWriteEntity(int dest, int data)
{
	NPP_NQWriteEntity(dest, data);
}











static enum protocol_type pp_svc_temp_entity_beam[] = {PPT_BYTE, PPT_BYTE, PPT_ENT, PPT_POS, PPT_POS};
static void pp_svc_temp_entity_beam_flush(client_t *cl, sizebuf_t *msg, enum protocol_type *pt, union protocol_data *pd)
{
	MSG_WriteByte(msg, pd[0].id);
	MSG_WriteByte(msg, pd[1].id);
	MSG_WriteEntity(msg, pd[2].id);
	MSG_WriteCoord(msg, pd[3].fd);
	MSG_WriteCoord(msg, pd[4].fd);
	MSG_WriteCoord(msg, pd[5].fd);
	MSG_WriteCoord(msg, pd[6].fd);
	MSG_WriteCoord(msg, pd[7].fd);
	MSG_WriteCoord(msg, pd[8].fd);
}
static qboolean pp_svc_temp_entity_beam_decide(enum protocol_type *pt, union protocol_data *pd)
{
	vec3_t org;
	org[0] = pd[3].fd;
	org[1] = pd[4].fd;
	org[2] = pd[5].fd;
	pp_flush(MULTICAST_PHS, org, pp_svc_temp_entity_beam_flush, pt, pd);
	return true;
}

static void pp_svc_temp_entity_gunshot_flush(client_t *cl, sizebuf_t *msg, enum protocol_type *pt, union protocol_data *pd)
{
	int offset = (progstype == PROG_QW)?3:2;
	int count = (offset == 3)?pd[2].id:1;

	while (count > 0)
	{
		MSG_WriteByte(msg, pd[0].id);
		MSG_WriteByte(msg, pd[1].id);
		if (cl->protocol == SCP_QUAKEWORLD)
		{
			if (count > 255)
			{
				MSG_WriteByte(msg, 255);
				count-=255;
			}
			else
			{
				MSG_WriteByte(msg, count);
				count = 0;
			}
		}
		else
			count--;
		MSG_WriteCoord(msg, pd[offset+0].fd);
		MSG_WriteCoord(msg, pd[offset+1].fd);
		MSG_WriteCoord(msg, pd[offset+2].fd);
	}
}
static qboolean pp_svc_temp_entity_gunshot(enum protocol_type *pt, union protocol_data *pd)
{
	vec3_t org;
	int offset = (progstype == PROG_QW)?3:2;
	org[0] = pd[offset+0].fd;
	org[1] = pd[offset+1].fd;
	org[2] = pd[offset+2].fd;
	pp_flush(MULTICAST_PHS, org, pp_svc_temp_entity_gunshot_flush, pt, pd);
	return true;
}

static qboolean pp_decide_pvs_2(enum protocol_type *pt, union protocol_data *pd)
{
	vec3_t org;
	org[0] = pd[2].fd;
	org[1] = pd[3].fd;
	org[2] = pd[4].fd;
	pp_flush(MULTICAST_PVS, org, pp_identity_flush, pt, pd);
	return true;
}
static qboolean pp_decide_phs_2(enum protocol_type *pt, union protocol_data *pd)
{
	vec3_t org;
	org[0] = pd[2].fd;
	org[1] = pd[3].fd;
	org[2] = pd[4].fd;
	pp_flush(MULTICAST_PHS, org, pp_identity_flush, pt, pd);
	return true;
}

static enum protocol_type pp_svc_temp_entity[] = {PPT_BYTE, PPT_BYTE};
static qboolean pp_svc_temp_entity_decide(enum protocol_type *pt, union protocol_data *pd)
{
	switch(pd[1].id)
	{
	case TE_LIGHTNING1:
	case TE_LIGHTNING2:
	case TE_LIGHTNING3:
		DECIDE(pp_svc_temp_entity_beam);
		return true;
	case TE_EXPLOSION:
	case TEDP_EXPLOSIONQUAD:
	case TE_SPIKE:
	case TE_SUPERSPIKE:
	case TEDP_SPIKEQUAD:
	case TEDP_SUPERSPIKEQUAD:
	case TEDP_SMALLFLASH:
		{
			static enum protocol_type fmt[] = {PPT_BYTE, PPT_BYTE, PPT_POS};
			DECIDE2(fmt, pp_decide_phs_2);
		}
		return true;

	case TEDP_GUNSHOTQUAD:
	case TE_TAREXPLOSION:
	case TE_WIZSPIKE:
	case TE_KNIGHTSPIKE:
	case TE_LAVASPLASH:
	case TE_TELEPORT:
		{
			static enum protocol_type fmt[] = {PPT_BYTE, PPT_BYTE, PPT_POS};
			DECIDE2(fmt, pp_decide_pvs_2);
		}
		return true;

	case TE_GUNSHOT:
		if (progstype == PROG_QW)
		{
			static enum protocol_type fmt[] = {PPT_BYTE, PPT_BYTE, PPT_BYTE, PPT_POS};
			DECIDE2(fmt, pp_svc_temp_entity_gunshot);
		}
		else
		{
			static enum protocol_type fmt[] = {PPT_BYTE, PPT_BYTE, PPT_POS};
			DECIDE2(fmt, pp_svc_temp_entity_gunshot);
		}
		return true;

	case 12:
		if (progstype == PROG_QW)
		{
			/*TEQW_BLOOD*/
		}
		else
		{
			/*TENQ_EXPLOSION2*/
		}
		return false;

	case 13:
		if (progstype == PROG_QW)
		{
			/*TEQW_LIGHTNINGBLOOD*/
		}
		else
		{
			/*TENQ_BEAM*/
		}
		return false;

	case TEDP_BLOOD:
	case TEDP_SPARK:
		{
			static enum protocol_type fmt[] = {PPT_BYTE, PPT_BYTE, PPT_POS, PPT_BYTE,PPT_BYTE,PPT_BYTE, PPT_BYTE};
			DECIDE2(fmt, pp_decide_pvs_2);
		}
		return true;

	case TE_BULLET:
	case TE_SUPERBULLET:

	case TE_RAILTRAIL:

		// hexen 2
	case TEH2_STREAM_CHAIN:
	case TEH2_STREAM_SUNSTAFF1:
	case TEH2_STREAM_SUNSTAFF2:
	case TEH2_STREAM_LIGHTNING:
	case TEH2_STREAM_COLORBEAM:
	case TEH2_STREAM_ICECHUNKS:
	case TEH2_STREAM_GAZE:
	case TEH2_STREAM_FAMINE:

	case TEDP_BLOODSHOWER:
	case TEDP_EXPLOSIONRGB:
	case TEDP_PARTICLECUBE:
	case TEDP_PARTICLERAIN: // [vector] min [vector] max [vector] dir [short] count [byte] color
	case TEDP_PARTICLESNOW: // [vector] min [vector] max [vector] dir [short] count [byte] color
	case TEDP_CUSTOMFLASH:
	case TEDP_FLAMEJET:
	case TEDP_PLASMABURN:
	case TEDP_TEI_G3:
	case TEDP_SMOKE:
	case TEDP_TEI_BIGEXPLOSION:
	case TEDP_TEI_PLASMAHIT:
	default:
		return false;
	}
}

qboolean pp_root_decide(enum protocol_type *pt, union protocol_data *pd)
{
	switch (pd[0].id)
	{
	case svc_temp_entity:
		DECIDE(pp_svc_temp_entity);
		return true;
	default:
		return false;
	}
}





#else

static sizebuf_t	*writedest;
static client_t		*cldest;
struct netprim_s *destprim;
static int majortype;
#ifdef NQPROT
static int minortype;
#endif
static int protocollen;

static qbyte buffer[MAX_OVERALLMSGLEN];
static int bufferlen;
static int nullterms;

static int multicastpos;	//writecoord*3 offset
static int multicasttype;
static int requireextension;
static qboolean ignoreprotocol;
static int te_515sevilhackworkaround;

#define svc_setfrags 14
#define svc_updatecolors 17

#ifdef HEXEN2
#define svch2_setviewflags		40	//hexen2.
#define svch2_clearviewflags	41	//hexen2.
#define svch2_setviewtint		46	//hexen2.
#define svch2_setangles_lerp	50
#endif

//these are present in the darkplaces engine.
//I wanna knick their mods.
#define svcdp_skybox	37

#define	svcdp_showlmp			35		// [string] slotname [string] lmpfilename [short] x [short] y
#define	svcdp_hidelmp			36		// [string] slotname

//#define	TE_RAILTRAIL_NEH		15 // [vector] origin [coord] red [coord] green [coord] blue	(fixme: ignored)
#define	TE_EXPLOSION3_NEH		16 // [vector] origin [coord] red [coord] green [coord] blue	(fixme: ignored)
#define TE_LIGHTNING4_NEH		17 // [string] model [entity] entity [vector] start [vector] end
#define TE_SMOKE_NEH			18
#define TE_EXPLOSIONSMALL2		20	//	org.

client_t *Write_GetClient(void);
sizebuf_t *QWWriteDest (int dest);
#ifdef NQPROT
sizebuf_t *NQWriteDest (int dest);
#endif

void NPP_SetInfo(client_t *cl, char *key, char *value)
{
	int i;

	//its common for bots to be set up by renaming players randomly etc
	//such bots should appear that way in server browsers.
	//on the other hand, players might end up with postfixes on their names.
	//this will break clientside name/stats parsing of course... not much we can do about that other than fixing the mods.
	
	if (cl->netchan.remote_address.type != NA_INVALID)
	{
		int vlen = strlen(value);
		if (!strcmp(key, "name") && 7+vlen+1 <= sizeof(buffer))
		{
			if (progstype != PROG_QW)
			{
				memmove(buffer+7, value, vlen+1);
				buffer[0] = svc_setinfo;
				buffer[1] = cl - svs.clients;
				buffer[2] = 'n';
				buffer[3] = 'a';
				buffer[4] = 'm';
				buffer[5] = 'e';
				buffer[6] = 0;
				bufferlen = 7+ vlen+1;
				return;
			}
		}
	}

	bufferlen = 0;

	if (!strcmp(key, "colours"))
	{
		i = atoi(value);
		InfoBuf_SetKey (&cl->userinfo, "bottomcolor", va("%i", i&15));
		InfoBuf_SetKey (&cl->userinfo, "topcolor", va("%i", i>>4));
	}
	InfoBuf_SetKey (&cl->userinfo, key, value);
	if (!*InfoBuf_ValueForKey (&cl->userinfo, "name"))
		cl->name[0] = '\0';
	else // process any changed values
		SV_ExtractFromUserinfo (cl, false);

	i = cl - svs.clients;
	if (!strcmp(key, "colours"))
	{
		MSG_WriteByte (&sv.reliable_datagram, svc_setinfo);
		MSG_WriteByte (&sv.reliable_datagram, i);
		MSG_WriteString (&sv.reliable_datagram, "bottomcolor");
		MSG_WriteString (&sv.reliable_datagram, InfoBuf_ValueForKey(&cl->userinfo, "bottomcolor"));

		MSG_WriteByte (&sv.reliable_datagram, svc_setinfo);
		MSG_WriteByte (&sv.reliable_datagram, i);
		MSG_WriteString (&sv.reliable_datagram, "topcolor");
		MSG_WriteString (&sv.reliable_datagram, InfoBuf_ValueForKey(&cl->userinfo, "topcolor"));
	}
	else
	{
		MSG_WriteByte (&sv.reliable_datagram, svc_setinfo);
		MSG_WriteByte (&sv.reliable_datagram, i);
		MSG_WriteString (&sv.reliable_datagram, key);
		MSG_WriteString (&sv.reliable_datagram, InfoBuf_ValueForKey(&cl->userinfo, key));
	}
}

void NPP_NQFlush(void)
{
	if (!bufferlen)
		return;

	switch(majortype)
	{
	case svc_cdtrack:
		if (bufferlen!=protocollen)
			Con_Printf("NQFlush: svc_cdtrack wasn't the right length\n");
		else
			bufferlen-=1;
		break;
	case svc_updatefrags:
		break;
		//ignore these.
	case svc_print:
	case svcdp_skybox:
		bufferlen = 0;
		break;
	case svc_updatename:
		NPP_SetInfo(&svs.clients[buffer[1]], "name", buffer+2);
		break;
	case svc_updatecolors:
		NPP_SetInfo(&svs.clients[buffer[1]], "colours", va("%i", buffer[2]));
		break;
	case svc_intermission:
//		if (writedest == &sv.reliable_datagram)
		{
			sizebuf_t *msg;
			client_t *cl;
			int i;
#ifdef HEXEN2
			char *h2finale = NULL;
			char *h2title = NULL;
			if (progstype == PROG_H2)
			{
				/*FIXME: hexen2 intermission+finale includes the viewheight. NQ does not, and QW has explicit position with scores but no finale*/
				/*hexen2 does something like this in the client, but we don't support those protocols, so translate to something usable*/
				char *title[13] = {"gfx/finale.lmp", "gfx/meso.lmp", "gfx/egypt.lmp", "gfx/roman.lmp", "gfx/castle.lmp", "gfx/castle.lmp", "gfx/end-1.lmp", "gfx/end-2.lmp", "gfx/end-3.lmp", "gfx/castle.lmp", "gfx/mpend.lmp", "gfx/mpmid.lmp", "gfx/end-3.lmp"};
				int lookup[13] = {394, 395, 396, 397, 358, strcmp(T_GetString(400+5*2+1), "BAD STRING")?400+5*2+1:400+4*2, 386+6, 386+7, 386+8, 391, 538, 545, 561};
				//5 is the demo sell screen, which changes depending on hexen2 vs portals.
				if (buffer[1] < 13)
				{
					h2title = title[buffer[1]];
					h2finale = T_GetString(lookup[buffer[1]]);
				}
			}
#endif

#if defined(HAVE_CLIENT)
			Log_MapNowCompleted();
#endif

			for (i = 0, cl = svs.clients; i < sv.allocated_client_slots; i++, cl++)
			{
				if (cl->state == cs_spawned && ISQWCLIENT(cl))
				{
#ifdef HEXEN2
					if (h2finale)
					{
						msg = ClientReliable_StartWrite(cl, 6 + strlen(h2title) + 3 + strlen(h2finale) + 1);
						MSG_WriteByte(msg, svc_finale);

						MSG_WriteByte(msg, '/');
						MSG_WriteByte(msg, 'F');
						MSG_WriteByte(msg, 'f');	//hexen2-style finale

						MSG_WriteByte(msg, '/');
						MSG_WriteByte(msg, 'I');
						SZ_Write(msg, h2title, strlen(h2title));
						MSG_WriteByte(msg, ':');	//image

						MSG_WriteByte(msg, '/');
						MSG_WriteByte(msg, 'P');	//image should be a background.

						MSG_WriteString(msg, h2finale);
					}
					else
#endif
						if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
					{	//special intermission mode to leave the view attached to the viewentity (as required for nq - especially rogue's finale) instead of hacking it to some specific point
						msg = ClientReliable_StartWrite(cl, 5);
						MSG_WriteByte(msg, svc_finale);
						MSG_WriteString(msg, "/FI");
					}
					else
					{
						msg = ClientReliable_StartWrite(cl, 23);
						MSG_WriteByte(msg, svc_intermission);
						MSG_WriteCoord(msg, cl->edict->v->origin[0]);
						MSG_WriteCoord(msg, cl->edict->v->origin[1]);
						MSG_WriteCoord(msg, cl->edict->v->origin[2]+cl->edict->v->view_ofs[2]);
						MSG_WriteAngle(msg, cl->edict->v->angles[0]);
						MSG_WriteAngle(msg, cl->edict->v->angles[1]);
						MSG_WriteAngle(msg, cl->edict->v->angles[2]);
					}
					ClientReliable_FinishWrite(cl);
				}
			}
			bufferlen = 0;
			protocollen=0;
			writedest = NULL;
		}
		break;
//	case svc_finale:	//finale does exist in vanilla qw. apparently. hurrah.
//		bufferlen = 0;
//		break;
	case svc_cutscene:
		//finale with no text. and explicitly no flags (which mostly prevents the 'completed' banner appearing), should be equivelent.
		buffer[0] = svc_finale;
		buffer[1] = '/';
		buffer[2] = '.';
		buffer[3] = 0;
		bufferlen = 4;
		break;
	case svc_setview:
		requireextension = PEXT_SETVIEW;

		if (cldest)	//catch it to work with all clients
		{
			cldest->viewent = *(unsigned short*)&buffer[1];
//			bufferlen = 0;
			if (cldest->viewent == (cldest - svs.clients)+1)
				cldest->viewent = 0;	//self is the same as none
		}
//		bufferlen = 0;
		break;
	case svcdp_hidelmp:
		if (progstype == PROG_TENEBRAE)
		{
			bufferlen = 0;
			break;
		}
		requireextension = PEXT_SHOWPIC;
		buffer[0] = svcfte_hidepic;
		break;
	case svcdp_showlmp:
		if (progstype == PROG_TENEBRAE)
		{
			bufferlen = 0;
			break;
		}
		requireextension = PEXT_SHOWPIC;
		memmove(buffer+2, buffer+1, bufferlen-1);
		bufferlen++;
		buffer[0] = svcfte_showpic;
		buffer[1] = 0;	//top left
		//pad the bytes to shorts.
		buffer[bufferlen] = buffer[bufferlen-1];
		buffer[bufferlen-1] = 0;
		buffer[bufferlen+1] = 0;
		bufferlen+=2;
		break;

	case svcfte_cgamepacket:
		requireextension = PEXT_CSQC;
		if (sv_csqcdebug.ival || writedest != &sv.multicast)
		{
			if (writedest != &sv.multicast)
				Con_Printf(CON_WARNING"Warning: svc_cgamepacket used outside of a multicast\n");
			/*shift the data up by two bytes*/
			memmove(buffer+3, buffer+1, bufferlen-1);

			buffer[0] = svcfte_cgamepacket_sized;
			/*add a length in the 2nd/3rd bytes*/
			buffer[1] = (bufferlen-1);
			buffer[2] = (bufferlen-1) >> 8;

			bufferlen += 2;
			if(multicastpos) multicastpos += 2;
		}
		break;
	case svc_temp_entity:
		if (sv_csqcdebug.ival)
		{
			if (te_515sevilhackworkaround && writedest != &sv.multicast)
				Con_Printf(CON_WARNING"Warning: unknown svc_temp_entity used outside of a multicast\n");
			/*shift the data up by two bytes, but don't care about the first byte*/
			memmove(buffer+3, buffer+1, bufferlen-1);

			buffer[0] = svcfte_temp_entity_sized;
			/*add a length in the 2nd/3rd bytes, if needed*/
			buffer[1] =  (bufferlen-1) & 0xff;
			buffer[2] = ((bufferlen-1) >> 8);
			if (te_515sevilhackworkaround)	//we don't know how to translate it, so the qw receiver must be told to interpret as the nq version
				buffer[2] |= 0x80;

			bufferlen += 2;
			if(multicastpos) multicastpos += 2;
			break;
		}
		switch (buffer[1])
		{
		default:
			if (te_515sevilhackworkaround)
			{
				if (writedest != &sv.multicast)
					Con_Printf(CON_WARNING"Warning: unknown svc_temp_entity used outside of a multicast\n");
				/*shift the data up by two bytes, but don't care about the first byte*/
				memmove(buffer+3, buffer+1, bufferlen-1);

				buffer[0] = svcfte_temp_entity_sized;
				/*add a length in the 2nd/3rd bytes, if needed*/
				buffer[1] =  (bufferlen-1) & 0xff;
				buffer[2] = ((bufferlen-1) >> 8);
				if (te_515sevilhackworkaround)	//we don't know how to translate it, so the qw receiver must be told to interpret as the nq version
					buffer[2] |= 0x80;

				bufferlen += 2;
				if(multicastpos) multicastpos += 2;
			}
			break;
		case TENQ_NQEXPLOSION:
			if (writedest == &sv.datagram)
			{	//for old clients, use a te_explosion.
				//for clients that support it, use a TEQW_EXPLOSIONNOSPRITE
				vec3_t org;
				int coordsize = (destprim->coordtype&0xf);
				coorddata cd;
				if (sv.multicast.cursize + bufferlen > sv.multicast.maxsize)
					SV_FlushBroadcasts();
				SZ_Write(&sv.multicast, buffer, bufferlen);

				memcpy(&cd, &buffer[2+coordsize*0], coordsize);
				org[0] = MSG_FromCoord(cd, coordsize);
				memcpy(&cd, &buffer[2+coordsize*1], coordsize);
				org[1] = MSG_FromCoord(cd, coordsize);
				memcpy(&cd, &buffer[2+coordsize*2], coordsize);
				org[2] = MSG_FromCoord(cd, coordsize);

				requireextension = PEXT_TE_BULLET;
				SV_MulticastProtExt(org, multicasttype, pr_global_struct->dimension_send, 0, requireextension);
				buffer[1] = TEQW_NQEXPLOSION;
			}
			break;
		case TENQ_BEAM:
			requireextension = PEXT_TE_BULLET;
			//should translate it to lightning or something for old clients
			buffer[1] = TEQW_BEAM;
			break;
		case TENQ_EXPLOSION2:	//happens with rogue.
			requireextension = PEXT_TE_BULLET;
			if (writedest == &sv.datagram || writedest == &sv.multicast)
			{
				vec3_t org;
				int coordsize = (destprim->coordtype&0xf);
				coorddata cd;
				memcpy(&cd, &buffer[2+coordsize*0], coordsize);
				org[0] = MSG_FromCoord(cd, coordsize);
				memcpy(&cd, &buffer[2+coordsize*1], coordsize);
				org[1] = MSG_FromCoord(cd, coordsize);
				memcpy(&cd, &buffer[2+coordsize*2], coordsize);
				org[2] = MSG_FromCoord(cd, coordsize);

				buffer[1] = TEQW_QWEXPLOSION;					//use a generic crappy explosion
				SZ_Write(&sv.multicast, buffer, bufferlen-2);	//trim the two trailing colour bytes
				SV_MulticastProtExt(org, multicasttype, pr_global_struct->dimension_send, 0, requireextension);
			}
			buffer[1] = TEQW_EXPLOSION2;	//TENQ_EXPLOSION2 conflicts with TEQW_BLOOD
			break;
		}
		break;
	}
	if (ignoreprotocol)
	{
		ignoreprotocol=false;
		bufferlen = 0;
	}




	if (cldest)
	{
		if (!requireextension || cldest->fteprotocolextensions & requireextension)
		if (bufferlen && ISQWCLIENT(cldest))
		{
			sizebuf_t *msg = ClientReliable_StartWrite(cldest, bufferlen);
			SZ_Write(msg, buffer, bufferlen);
			ClientReliable_FinishWrite(cldest);
		}
		cldest = NULL;
	}
	else
	{
		if (multicastpos && (writedest == &sv.datagram || writedest == &sv.multicast))
			writedest = &sv.multicast;
		else
			multicastpos = 0;
		if (bufferlen)
		{
			if (writedest->cursize + bufferlen > writedest->maxsize)
			{
				SV_FlushBroadcasts();
			}
			SZ_Write(writedest, buffer, bufferlen);
		}

		if (multicastpos)
		{
			vec3_t org;
			int coordsize = (destprim->coordtype&0xf);
			coorddata cd;

			memcpy(&cd, &buffer[multicastpos+coordsize*0], coordsize);
			org[0] = MSG_FromCoord(cd, coordsize);
			memcpy(&cd, &buffer[multicastpos+coordsize*1], coordsize);
			org[1] = MSG_FromCoord(cd, coordsize);
			memcpy(&cd, &buffer[multicastpos+coordsize*2], coordsize);
			org[2] = MSG_FromCoord(cd, coordsize);

			SV_MulticastProtExt(org, multicasttype, pr_global_struct->dimension_send, requireextension, 0);
		}
		writedest = NULL;
	}
	bufferlen = 0;
	protocollen=0;
	nullterms = 0;
	multicastpos=0;
	requireextension=0;
}
void NPP_NQCheckFlush(void)
{
	if (bufferlen >= protocollen && protocollen && !nullterms)
		NPP_NQFlush();
}

void NPP_NQCheckDest(int dest)
{
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("Not a client\n");
			return;
		}
		if (bufferlen && ((cldest && cldest != cl) || writedest))
		{
			Con_Printf("MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_NQFlush();
		}
		writedest = NULL;
		cldest = cl;
		destprim = &cldest->netchan.message.prim;
	}
	else
	{
		sizebuf_t	*ndest = QWWriteDest(dest);
		if (bufferlen && (cldest || (writedest && writedest != ndest)))
		{
			Con_DPrintf("NQCheckDest: MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_NQFlush();
		}
		cldest = NULL;
		writedest = ndest;
		destprim = &writedest->prim;
	}
}
void NPP_AddData(const void *data, int len)
{
	if (bufferlen+len > sizeof(buffer))
	{
		bufferlen = 0;
		SV_Error("Preparse buffer was filled\n");
	}
	memcpy(buffer+bufferlen, data, len);
	bufferlen+=len;
}

void NPP_NQWriteByte(int dest, qbyte data)	//replacement write func (nq to qw)
{
	NPP_NQCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD) // is a bot
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(qbyte));
				ClientReliableWrite_Byte(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteByte (NQWriteDest(dest), data);
#endif

	if (!bufferlen)	//new message section
	{
		majortype = data;
		switch(data)
		{
		case svcdp_showlmp:
		case svcdp_hidelmp:
		case svc_sound:
			break;
		case svc_temp_entity:
			te_515sevilhackworkaround = false;
			break;
		case svc_setangle:
			protocollen = sizeof(qbyte) + destprim->anglesize*3;
			break;
		case svc_setview:
			protocollen = sizeof(qbyte)*1 + sizeof(short);
			break;
		case svc_updatename:
			nullterms = 1;
			break;
		case svc_setfrags:
			protocollen = 4;
			break;
		case svc_updatecolors:
			protocollen = 3;
			break;
		case svc_print:
			protocollen = 3;
			nullterms = 1;
			break;
		case svc_cdtrack:
			if (progstype == PROG_QW)
				protocollen = 2;
			else
				protocollen = 3;
			break;
		case svc_killedmonster:
			protocollen = 1;
			break;
		case svc_foundsecret:
			protocollen = 1;
			break;
		case svc_intermission:
			if (progstype == PROG_H2)
				protocollen = 2;
			else
				protocollen = 1;
			break;
		case svc_finale:
			nullterms = 1;
			protocollen = 2;
			break;
		case svcdp_skybox:
			protocollen = 2;//it's just a string
			break;
		case svcnq_updatestatlong:	//insta fixup
			data = svcqw_updatestatlong;	//ho hum... let it through (should check size later.)
			protocollen = 6;
			break;
		case svcqex_achievement:
			ignoreprotocol = true;
			nullterms = 1;
			break;
		case svc_stufftext:
		case svc_centerprint:
			nullterms = 1;
			break;
#ifdef HEXEN2
		case svch2_setviewflags:	//sets some viewmodel drawflag
		case svch2_clearviewflags:	//undoes svch2_setviewflags
		case svch2_setviewtint:		//tints the viewmodel (tied to hexen2's weird colormap stuff)
			if (progstype == PROG_H2)
			{
				protocollen = 2;
				ignoreprotocol = true;
			}
			else
			{
				Con_DPrintf("NQWriteByte: bad protocol %i\n", (int)data);
				protocollen = sizeof(buffer);
			}
			break;
		case svch2_setangles_lerp:
			if (progstype == PROG_H2)
			{
				majortype = data = svc_setangle;
				protocollen = sizeof(qbyte) + destprim->anglesize*3;
			}
			else
			{
				Con_DPrintf("NQWriteByte: bad protocol %i\n", (int)data);
				protocollen = sizeof(buffer);
			}
			break;
#endif
		case svc_cutscene:
			nullterms = 1;
			break;
		case svcdp_updatestatbyte:
			protocollen = 3;
			ignoreprotocol = true;
			break;
		case svcfte_cgamepacket:
			protocollen = sizeof(buffer);
			break;
		default:
			if (data & 128)
			{
				majortype = 128;	//nq 'fast update'
				ignoreprotocol = true;
				//this handling is explicitly for hipnotic.
				//hipnotic sends a camera entity this way so that svc_setview knows where to position itself.
				//we emulate this with quakeworld protocols, so we don't necessarily need to support it other than to strip it.
				//we're probably screwed if we're acting as an nq server, but few people will use that.
			}
			else
			{
				Con_DPrintf("NQWriteByte: bad protocol %i\n", (int)data);
				protocollen = sizeof(buffer);
			}
			break;
		}
	}
	if (bufferlen == 1 && !protocollen)	//some of them depend on the following bytes for size.
	{
		switch(majortype)
		{
		case 128:
#ifdef NQPROT
			{
				unsigned int bits = buffer[0];
				protocollen = 1;
				if (bits & NQU_MOREBITS)
				{
					bits |= data<<8;
					protocollen+=1;
				}

				if (bits != (NQU_SIGNAL|NQU_MOREBITS|NQU_ORIGIN1|NQU_ORIGIN2|NQU_ORIGIN3|NQU_LONGENTITY))
				{	//this mask is what hipnotic uses. warn if anyone else tries it.
					Con_Printf("fast update will be stripped\n");
					PR_StackTrace(svprogfuncs, false);
				}
				if (bits & NQU_ORIGIN1)
					protocollen+=2;
				if (bits & NQU_ORIGIN2)
					protocollen+=2;
				if (bits & NQU_ORIGIN3)
					protocollen+=2;
				if (bits & NQU_ANGLE2)
					protocollen+=1;
				//nolerp no data
				if (bits & NQU_FRAME)
					protocollen+=1;

				if (bits & NQU_ANGLE1)
					protocollen+=1;
				if (bits & NQU_ANGLE3)
					protocollen+=1;
				if (bits & NQU_MODEL)
					protocollen+=1;
				if (bits & NQU_COLORMAP)
					protocollen+=1;
				if (bits & NQU_SKIN)
					protocollen+=1;
				if (bits & NQU_EFFECTS)
					protocollen+=1;
				if (bits & NQU_LONGENTITY)
					protocollen+=2;
				else
					protocollen+=1;
				if (bits & DPU_EXTEND1)
					Con_DPrintf("NQWriteByte: fast update with extension bits is not supported\n");
			}
#else
			ignoreprotocol = true;
#endif
			break;
		case svc_sound:
			protocollen = 5+(destprim->coordtype&0xf)*3;
			if (data & NQSND_VOLUME)
				protocollen++;
			if (data & NQSND_ATTENUATION)
				protocollen++;
			if (data & NQSND_LARGEENTITY)//extension
				protocollen++;
			if (data & NQSND_LARGESOUND)//extension
				protocollen++;
#ifdef warningmsg
#pragma warningmsg("NPP_NQWriteByte: this ignores SVC_SOUND from nq mods (nexuiz)")
#endif
			ignoreprotocol = true;
			break;
		case svc_temp_entity:
			switch(data)
			{
			case TENQ_BEAM:
			case TE_LIGHTNING1:
			case TE_LIGHTNING2:
			case TE_LIGHTNING3:
				multicastpos=4;
				multicasttype=MULTICAST_PHS;
				protocollen = (destprim->coordtype&0xf)*6+sizeof(short)+sizeof(qbyte)*2;
				break;
			case TENQ_NQGUNSHOT:
				multicastpos=3;
				multicasttype=MULTICAST_PVS;
				//we need to emit another qbyte here. QuakeWorld has a number of particles.
				//emit it here and we don't need to remember to play with temp_entity later
				NPP_AddData(&data, sizeof(qbyte));
				data = 1;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*3;
				break;
			case TENQ_NQEXPLOSION:
			case TE_SPIKE:
			case TE_SUPERSPIKE:
				multicastpos=2;
				multicasttype=MULTICAST_PHS;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*2;
				break;
			case TE_LAVASPLASH:
				multicastpos=2;
				multicasttype=MULTICAST_ALL;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*2;
				break;
			case TE_TAREXPLOSION:
			case TE_WIZSPIKE:
			case TE_KNIGHTSPIKE:
			case TE_TELEPORT:
				multicastpos=2;
				multicasttype=MULTICAST_PVS;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*2;
				break;
			case TE_EXPLOSION3_NEH:
				protocollen = sizeof(qbyte) + (destprim->coordtype&0xf)*6;
				ignoreprotocol = true;
				break;
			case TENQ_EXPLOSION2:
				protocollen = sizeof(qbyte)*4 + (destprim->coordtype&0xf)*3;
				multicastpos=2;
				multicasttype=MULTICAST_PHS;
				break;
			case TE_EXPLOSIONSMALL2:
				data = TEQW_QWEXPLOSION;
				protocollen = sizeof(qbyte)*2 + (destprim->coordtype&0xf)*3;
				multicastpos=2;
				multicasttype=MULTICAST_PHS;
				break;
			case TEQW_RAILTRAIL:
				protocollen = (destprim->coordtype&0xf)*6+sizeof(qbyte)*1;
				multicastpos=2;
				multicasttype=MULTICAST_PHS;
				break;
			case TEH2_STREAM_LIGHTNING_SMALL:
			case TEH2_STREAM_CHAIN:
			case TEH2_STREAM_SUNSTAFF1:
			case TEH2_STREAM_SUNSTAFF2:
			case TEH2_STREAM_LIGHTNING:
			case TEH2_STREAM_ICECHUNKS:
			case TEH2_STREAM_GAZE:
			case TEH2_STREAM_FAMINE:
				protocollen = (destprim->coordtype&0xf)*6+sizeof(short)+sizeof(qbyte)*(2+2);
				multicastpos = 8;
				multicasttype=MULTICAST_PHS;
				break;
			case TEH2_STREAM_COLORBEAM:
				protocollen = (destprim->coordtype&0xf)*6+sizeof(short)+sizeof(qbyte)*(3+2);
				multicastpos = 8;
				multicasttype=MULTICAST_PHS;
				break;

			case TEDP_FLAMEJET:	//TE_FLAMEJET
				protocollen = (destprim->coordtype&0xf)*6 +sizeof(qbyte)*3;
				multicastpos = 2;
				multicasttype=MULTICAST_PVS;
				break;

			case TEDP_TEI_G3:
				protocollen = (destprim->coordtype&0xf)*9+sizeof(qbyte)*2;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			case TEDP_SMOKE:
				protocollen = (destprim->coordtype&0xf)*6+sizeof(qbyte)*3;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			case TEDP_TEI_BIGEXPLOSION:
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*2;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			case TEDP_TEI_PLASMAHIT:
				protocollen = (destprim->coordtype&0xf)*6+sizeof(qbyte)*3;
				multicastpos = 2;
				multicasttype=MULTICAST_PHS;
				break;

			default:
				protocollen = sizeof(buffer);
				if (dest == MSG_MULTICAST)
				{
					Con_DPrintf("NQWriteByte: unknown tempentity %i\n", data);
				}
				else
				{
					te_515sevilhackworkaround = true;
					if (!ssqc_deprecated_warned)
					{
						ssqc_deprecated_warned = true;
						Con_Printf("NQWriteByte: invalid tempentity %i. Future errors will be dprinted. You may need to enable sv_csqcdebug.\n", data);
						PR_StackTrace(svprogfuncs, false);
					}
					else
						Con_DPrintf("NQWriteByte: unknown tempentity %i\n", data);
				}
				break;
			}
			break;
		case svcqex_achievement:
		case svc_updatename:
		case svc_stufftext:
		case svc_centerprint:
			break;
		default:
			Con_Printf("NQWriteByte: Non-Implemented svc\n");
			protocollen = sizeof(buffer);
			break;
		}
	}
	if (!protocollen)	//these protocols take strings, and are thus dynamically sized.
	{
		switch(majortype)
		{
		case svc_updatename:
			if (bufferlen < 2)
				break;	//don't truncate the name if the mod is sending the slot number
		case svcqex_achievement:
		case svc_stufftext:
		case svc_centerprint:
		case svc_cutscene:
			if (!data)
				protocollen = bufferlen;
			break;
		case svcdp_hidelmp:
			//tenebrae compat:
			if (progstype == PROG_TENEBRAE)
			{
				//svc, coord6, byte, long, long, effectname
				if (bufferlen >= sizeof(qbyte)*2+(destprim->coordtype&0xf)*6+sizeof(int)*2 && !data)
					protocollen = bufferlen;
				break;
			}
			//nehahra/dp compat
			if (!data)
				protocollen = bufferlen;
			break;
		case svcdp_showlmp:
			//tenebrae compat:
			if (progstype == PROG_TENEBRAE)
			{
				//svc, coord3, byte, effectname
				if (bufferlen >= sizeof(qbyte)*2+(destprim->coordtype&0xf)*3 && !data)
					protocollen = bufferlen;
				break;
			}
			// [string] slotname [string] lmpfilename [byte] x [byte] y
			//note: nehara uses bytes!
			//and the rest of dp uses shorts. how nasty is that?
			if (!data)
			{	//second string, plus 2 bytes.
				int i;
				for (i = 0; i < bufferlen; i++)
					if (!buffer[i])
						protocollen = bufferlen+2;
			}
			break;
		}
	}

	NPP_AddData(&data, sizeof(qbyte));
	if (!data && bufferlen>=protocollen)
		if (nullterms)
			nullterms--;
	NPP_NQCheckFlush();
}

void NPP_NQWriteChar(int dest, char data)	//replacement write func (nq to qw)
{
	NPP_NQWriteByte(dest, (qbyte)data);
	return;
	/*
	NPP_NQCheckDest(dest);
	if (!bufferlen)
	{
		NPP_NQWriteByte(dest, (qbyte)data);
		return;
	}

#ifdef NQPROT
	if (dest == MSG_ONE) {
		client_t *cl = Write_GetClient();
		if (cl && cl->nqprot)
		{
			ClientReliableCheckBlock(cl, sizeof(char));
			ClientReliableWrite_Char(cl, data);
		}
	} else
		MSG_WriteChar (NQWriteDest(dest), data);
#endif

	NPP_AddData(&data, sizeof(char));
	NPP_NQCheckFlush();*/
}

void NPP_NQWriteShort(int dest, short data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[2];
		short s;
	} u;
	u.s = LittleShort(data);
	NPP_NQWriteByte(dest, u.b[0]);
	NPP_NQWriteByte(dest, u.b[1]);
}

void NPP_NQWriteLong(int dest, long data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[4];
		int l;
	} u;
	u.l = LittleLong(data);
	NPP_NQWriteByte(dest, u.b[0]);
	NPP_NQWriteByte(dest, u.b[1]);
	NPP_NQWriteByte(dest, u.b[2]);
	NPP_NQWriteByte(dest, u.b[3]);
}
void NPP_NQWriteFloat(int dest, float data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[4];
		float f;
	} u;
	u.f = LittleFloat(data);
	NPP_NQWriteByte(dest, u.b[0]);
	NPP_NQWriteByte(dest, u.b[1]);
	NPP_NQWriteByte(dest, u.b[2]);
	NPP_NQWriteByte(dest, u.b[3]);
}
void NPP_NQWriteAngle(int dest, float in)	//replacement write func (nq to qw)
{
	char data = (int)(in*256/360) & 255;
	NPP_NQCheckDest(dest);

#ifdef NQPROT
	if (cldest)
	{
		if (cldest->protocol == SCP_BAD)
			return;
		else if (!ISQWCLIENT(cldest))
		{
			ClientReliableCheckBlock(cldest, sizeof(char));
			ClientReliableWrite_Angle(cldest, in);
			return;
		}
	}
	else
		MSG_WriteAngle (NQWriteDest(dest), in);
#endif

	if (!bufferlen)
	{
		Con_Printf("NQWriteAngle: Messages should start with WriteByte (last was %i)\n", majortype);
		PR_StackTrace(svprogfuncs, false);
	}

	if (destprim->anglesize==2)
	{
		coorddata cd = MSG_ToAngle(in, destprim->anglesize);
		NPP_AddData(&cd.b2, sizeof(cd.b2));
	}
	else
		NPP_AddData(&data, sizeof(char));
	NPP_NQCheckFlush();
}
void NPP_NQWriteCoord(int dest, float in)	//replacement write func (nq to qw)
{
	NPP_NQCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(float));
				ClientReliableWrite_Coord(cl, in);
				return;
			}
		}
	}
	else
		MSG_WriteCoord (NQWriteDest(dest), in);
#endif

	if (!bufferlen)
	{
		Con_Printf("NQWriteCoord: Messages should start with WriteByte\n");
		PR_StackTrace(svprogfuncs, false);
	}

	if (destprim->coordtype==COORDTYPE_FLOAT_32)
	{
		float dataf = in;

		dataf = LittleFloat(dataf);
		NPP_AddData(&dataf, sizeof(float));
	}
	else
	{
		short datas = (int)(in*8)&0xffff;

		datas = LittleShort(datas);
		NPP_AddData(&datas, sizeof(short));
	}
	NPP_NQCheckFlush();
}
void NPP_NQWriteString(int dest, const char *data)	//replacement write func (nq to qw)
{
	NPP_NQCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, strlen(data)+1);
				ClientReliableWrite_String(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteString (NQWriteDest(dest), data);
#endif

	if (!bufferlen)
	{
		Con_Printf("NQWriteString: Messages should start with WriteByte\n");
		PR_StackTrace(svprogfuncs, false);
	}

	NPP_AddData(data, strlen(data)+1);

	if (!protocollen)	//these protocols take strings, and are thus dynamically sized.
	{
		switch(majortype)
		{
		case svc_updatename:
		case svc_stufftext:
		case svc_centerprint:
		case svc_cutscene:
			protocollen = bufferlen;
			break;
		}
	}

	if (nullterms)
		nullterms--;
	NPP_NQCheckFlush();
}
void NPP_NQWriteEntity(int dest, int data)	//replacement write func (nq to qw)
{
	NPP_NQCheckDest(dest);

	if (majortype == svc_temp_entity && data > 0 && data <= sv.allocated_client_slots)
		if (svs.clients[data-1].viewent)
			data = svs.clients[data-1].viewent;

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (!ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(short));
				ClientReliableWrite_Entity(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteEntity (NQWriteDest(dest), data);
#endif

	if (!bufferlen)
	{
		Con_Printf("NQWriteEntity: Messages should start with WriteByte\n");
		PR_StackTrace(svprogfuncs, false);
	}


	NPP_AddData(&data, sizeof(short));
	NPP_NQCheckFlush();
}
















#ifdef NQPROT

float NPP_ReadFloat(qbyte *buf)
{
	union
	{
		float f;
		qbyte b[4];
	} u;
	memcpy(u.b, buf, sizeof(u.f));
	return LittleFloat(u.f);
}
short NPP_ReadShort(qbyte *buf)
{
	union
	{
		short s;
		qbyte b[2];
	} u;
	memcpy(u.b, buf, sizeof(u.s));
	return LittleShort(u.s);
}
unsigned short NPP_ReadUShort(qbyte *buf)
{
	union
	{
		unsigned short s;
		qbyte b[2];
	} u;
	memcpy(u.b, buf, sizeof(u.s));
	return LittleShort(u.s);
}

//qw to nq translation is only useful if we allow nq clients to connect.

void NPP_QWFlush(void)
{
	qbyte b;
	if (!bufferlen)
		return;

	switch(majortype)
	{
	case svc_updatename:	//not a standard feature, but hey, if a progs wants bots.
		NPP_SetInfo(&svs.clients[buffer[1]], "name", buffer+2);
		break;
	case svc_updatecolors:
		NPP_SetInfo(&svs.clients[buffer[1]], "colours", va("%i", buffer[2]));
		break;
	case svc_cdtrack:
		if (bufferlen!=protocollen)
			Con_Printf("QWFlush: svc_cdtrack wasn't the right length\n");
		else
		{
			//qw cdtracks have only a loop byte.
			//nq has initial+loop values.
			b = (bufferlen==2)?buffer[1]:0;
			NPP_AddData(&b, sizeof(qbyte));
		}
		break;
		//ignore these.
	case svc_intermission:
#if defined(HAVE_CLIENT)
		Log_MapNowCompleted();
#endif
//		if (writedest == &sv.reliable_datagram)
		{
			client_t *cl;
			int i;
			for (i = 0, cl = svs.clients; i < sv.allocated_client_slots; i++, cl++)
			{
				if (cl->state == cs_spawned && ISNQCLIENT(cl))
				{
					vec3_t org, ang;

					if (cl->zquake_extensions & Z_EXT_SERVERTIME)
					{
						ClientReliableCheckBlock(cl, 6);
						ClientReliableWrite_Byte(cl, svcqw_updatestatlong);
						ClientReliableWrite_Byte(cl, STAT_TIME);
						ClientReliableWrite_Long(cl, (int)(sv.world.physicstime * 1000));
						cl->nextservertimeupdate = sv.world.physicstime+10;
					}

					ClientReliableCheckBlock(cl, 1);
					ClientReliableWrite_Byte(cl, svc_intermission);

					i = 1;
					if (destprim->coordtype==COORDTYPE_FLOAT_32)
					{
						org[0] = NPP_ReadFloat(buffer+i+0);
						org[1] = NPP_ReadFloat(buffer+i+4);
						org[2] = NPP_ReadFloat(buffer+i+8);
						i += 12;
					}
					else
					{
						org[0] = NPP_ReadShort(buffer+i+0) / 8.0;
						org[1] = NPP_ReadShort(buffer+i+2) / 8.0;
						org[2] = NPP_ReadShort(buffer+i+4) / 8.0;
						i += 6;
					}

					if (destprim->anglesize == 2)
					{
						ang[0] = NPP_ReadUShort(buffer+i+0)*360.0/0xffff;
						ang[1] = NPP_ReadUShort(buffer+i+2)*360.0/0xffff;
						ang[2] = NPP_ReadUShort(buffer+i+4)*360.0/0xffff;
						i += 6;
					}
					else
					{
						ang[0] = (*(qbyte*)&buffer[i+0])*360.0/0xff;
						ang[1] = (*(qbyte*)&buffer[i+1])*360.0/0xff;
						ang[2] = (*(qbyte*)&buffer[i+2])*360.0/0xff;
						i += 3;
					}

					//move nq players to origin + angle
					VectorCopy(org, cl->edict->v->origin);
					VectorCopy(ang, cl->edict->v->angles);
					cl->edict->v->angles[0]*=r_meshpitch.value;
				}
			}
		}
		bufferlen = 0;
		protocollen=0;
		writedest = NULL;
//	case svc_finale:
//		bufferlen = 0;
		break;
	case svc_setview:
		requireextension = PEXT_SETVIEW;
//		bufferlen = 0;
		break;
	case svc_muzzleflash:
		if (bufferlen < 3)
			Con_Printf("Dodgy muzzleflash\n");
		else
		{
			short data;
			float org[3];
			edict_t *ent = EDICT_NUM_UB(svprogfuncs, LittleShort((*(short*)&buffer[1])));
			ent->muzzletime = sv.world.physicstime+host_frametime;	//flag the entity as needing an EF_MUZZLEFLASH
			VectorCopy(ent->v->origin, org);

			//we need to make a fake muzzleflash position for multicast to work properly.
			multicastpos = 4;
			data = LittleShort((short)(org[0]*8));
			NPP_AddData(&data, sizeof(short));
			data = LittleShort((short)(org[1]*8));
			NPP_AddData(&data, sizeof(short));
			data = LittleShort((short)(org[2]*8));
			NPP_AddData(&data, sizeof(short));
		}
		bufferlen = 0;	//can't send this to nq. :(
		break;
	case svc_smallkick:
	case svc_bigkick:
		bufferlen = 0;
		break;
	case svcqw_updatestatlong:
		buffer[0] = svcnq_updatestatlong;
		break;
	case svc_updateuserinfo:
		if (buffer[6])
		{
			unsigned int j = buffer[1];
			if (j < sv.allocated_client_slots)
			{
				InfoBuf_FromString(&svs.clients[j].userinfo, buffer+6, false);
				if (*InfoBuf_ValueForKey(&svs.clients[j].userinfo, "name"))
					SV_ExtractFromUserinfo(&svs.clients[j], false);
				else
					*svs.clients[j].name = '\0';
			}
		}
		else
		{
			unsigned int j = buffer[1];
			if (j < sv.allocated_client_slots)
			{
				*svs.clients[j].name = '\0';
				InfoBuf_Clear(&svs.clients[j].userinfo, true);
			}
		}

		break;
	case svcfte_cgamepacket:
		requireextension = PEXT_CSQC;
		if (sv_csqcdebug.ival || writedest != &sv.nqmulticast)
		{
			if (writedest != &sv.nqmulticast)
				Con_Printf(CON_WARNING"Warning: svc_cgamepacket used outside of a multicast\n");
			/*shift the data up by two bytes*/
			memmove(buffer+3, buffer+1, bufferlen-1);

			buffer[0] = svcfte_cgamepacket_sized;
			/*add a length in the 2nd/3rd bytes*/
			buffer[1] = (bufferlen-1);
			buffer[2] = (bufferlen-1) >> 8;

			bufferlen += 2;
			if(multicastpos) multicastpos += 2;
		}
		break;
	case svc_temp_entity:
		if (sv_csqcdebug.ival)
		{
			if (te_515sevilhackworkaround && writedest != &sv.multicast)
				Con_Printf(CON_WARNING"Warning: unknown svc_temp_entity used outside of a multicast\n");
			/*shift the data up by two bytes, but don't care about the first byte*/
			memmove(buffer+3, buffer+1, bufferlen-1);

			buffer[0] = svcfte_temp_entity_sized;
			/*add a length in the 2nd/3rd bytes, if needed*/
			buffer[1] = (bufferlen-1) & 0xff;
			buffer[2] = (bufferlen-1) >> 8;
			if (!te_515sevilhackworkaround)	//we translated it from qw, let the client know that its now the nq version
				buffer[2] |= 0x80;

			bufferlen += 2;
			if(multicastpos) multicastpos += 2;
			break;
		}
		switch(minortype)
		{
		default:
			if (te_515sevilhackworkaround)
			{
				Con_Printf(CON_WARNING"Warning: unknown svc_temp_entity used outside of a multicast\n");
				/*shift the data up by two bytes*/
				memmove(buffer+3, buffer+1, bufferlen-1);

				buffer[0] = svcfte_temp_entity_sized;
				/*add a length in the 2nd/3rd bytes*/
				buffer[1] = (bufferlen-1);
				buffer[2] = (bufferlen-1) >> 8;
				if (!te_515sevilhackworkaround)	//we translated it from qw, let the client know that its now the nq version
					buffer[2] |= 0x80;

				bufferlen += 2;
				if(multicastpos) multicastpos += 2;
			}
			break;
		case TEQW_LIGHTNINGBLOOD:
		case TEQW_QWBLOOD:		//needs to be converted to an svc_particle
			{
				vec3_t org;
				qbyte count;
				qbyte colour;
				char dir[3];
				short s;
				int v;
				int i;
				qbyte svc;
				svc = svc_particle;
				org[0] = (*(short*)&buffer[multicastpos])/8.0f;
				org[1] = (*(short*)&buffer[multicastpos+2])/8.0f;
				org[2] = (*(short*)&buffer[multicastpos+4])/8.0f;
				count = bound(0, buffer[2]*20, 254);	//255 is taken to mean an explosion, for some reason.
				if (minortype == TEQW_LIGHTNINGBLOOD)
					colour = 225;
				else
					colour = 73;

				for (i=0 ; i<3 ; i++)
				{
					v = 0*16;
					if (v > 127)
						v = 127;
					else if (v < -128)
						v = -128;
					dir[i] = v;
				}

				bufferlen = 0;		//restart
				protocollen = 1000;

				multicastpos = 1;

				NPP_AddData(&svc, sizeof(qbyte));
				for (i = 0; i < 3; i++)
				{
					if (destprim->coordtype==COORDTYPE_FLOAT_32)
						NPP_AddData(&org[i], sizeof(float));
					else
					{
						s = org[i]*8;
						NPP_AddData(&s, sizeof(short));
					}
				}
				NPP_AddData(&dir[0], sizeof(char));
				NPP_AddData(&dir[1], sizeof(char));
				NPP_AddData(&dir[2], sizeof(char));
				NPP_AddData(&count, sizeof(qbyte));
				NPP_AddData(&colour, sizeof(qbyte));
			}
			break;
		case TEQW_QWGUNSHOT:	//needs byte 3 removed
			if (bufferlen >= 3)
			{
				memmove(buffer+2, buffer+3, bufferlen-3);
				bufferlen--;
			}
			break;
		}
	}
	if (ignoreprotocol)
	{
		ignoreprotocol=false;
		bufferlen = 0;
	}




	if (cldest)
	{
		if (!requireextension || cldest->fteprotocolextensions & requireextension)
		if (bufferlen && !ISQWCLIENT(cldest))
		{
			sizebuf_t *msg = ClientReliable_StartWrite(cldest, bufferlen);
			SZ_Write(msg, buffer, bufferlen);
			ClientReliable_FinishWrite(cldest);
		}
		cldest = NULL;
	}
	else
	{
		if (multicastpos && (writedest == &sv.nqdatagram || writedest == &sv.nqmulticast))
			writedest = &sv.nqmulticast;
		else
			multicastpos = 0;
		if (bufferlen)
			SZ_Write(writedest, buffer, bufferlen);

		if (multicastpos)
		{
			int qwsize;
			vec3_t org;
			int coordsize = (destprim->coordtype&0xf);
			coorddata cd;

			memcpy(&cd, &buffer[multicastpos+coordsize*0], coordsize);
			org[0] = MSG_FromCoord(cd, coordsize);
			memcpy(&cd, &buffer[multicastpos+coordsize*1], coordsize);
			org[1] = MSG_FromCoord(cd, coordsize);
			memcpy(&cd, &buffer[multicastpos+coordsize*2], coordsize);
			org[2] = MSG_FromCoord(cd, coordsize);

			qwsize = sv.multicast.cursize;
			sv.multicast.cursize = 0;
			SV_MulticastProtExt(org, multicasttype, pr_global_struct->dimension_send, requireextension, 0);
			sv.multicast.cursize = qwsize;
		}
		writedest = NULL;
	}
	bufferlen = 0;
	nullterms=0;
	protocollen=0;
	multicastpos=0;
	requireextension=0;
}
void NPP_QWCheckFlush(void)
{
	if (bufferlen >= protocollen && protocollen && !nullterms)
		NPP_QWFlush();
}

void NPP_QWCheckDest(int dest)
{
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("Not a client\n");
			return;
		}
		if (bufferlen && ((cldest && cldest != cl) || writedest))
		{
			Con_Printf("MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_QWFlush();
		}
		writedest = NULL;
		cldest = cl;
		destprim = &cldest->netchan.message.prim;
	}
	else
	{
		sizebuf_t	*ndest = NQWriteDest(dest);
		if (bufferlen && (cldest || (writedest && writedest != ndest)))
		{
			Con_DPrintf("QWCheckDest: MSG destination changed in the middle of a packet %i.\n", (int)*buffer);
			NPP_QWFlush();
		}
		cldest = NULL;
		writedest = ndest;
		destprim = &writedest->prim;
	}
}




void NPP_QWWriteByte(int dest, qbyte data)	//replacement write func (nq to qw)
{
	NPP_QWCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD) // is a bot
				return;
			else if (ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, sizeof(qbyte));
				ClientReliableWrite_Byte(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteByte (QWWriteDest(dest), data);
#endif
	if (!bufferlen)	//new message section
	{
		switch(data)
		{
		case svc_temp_entity:
			break;
		case svc_setangle:
			protocollen = sizeof(qbyte) + destprim->anglesize*3;
			break;
		case svc_setview:
			protocollen = sizeof(qbyte)*1 + sizeof(short);
			break;
		case svc_cdtrack:
			protocollen = sizeof(qbyte)*2;
			break;
		case svc_killedmonster:
			protocollen = 1;
			break;
		case svc_foundsecret:
			protocollen = 1;
			break;
		case svc_intermission:
			protocollen = 1 + (destprim->coordtype&0xf)*3 + destprim->anglesize*3;
			break;
		case svc_finale:
			nullterms = 1;
			protocollen = 2;
			break;
		case svc_updatepl:
		case svc_muzzleflash:
			protocollen = 3;
			break;
		case svc_smallkick:
		case svc_bigkick:
			protocollen = 1;
			break;
		case svc_print:
			protocollen = 2;
			nullterms=1;
			break;
		case svc_setinfo:
			protocollen = 2;
			nullterms = 2;
			break;
		case svc_centerprint:
		case svc_stufftext:
			protocollen = 1;
			nullterms=1;
			break;
		case svcqw_updatestatbyte:
			protocollen = 3;
			break;
		case svc_updateping:
		case svc_updatefrags:
			protocollen = 4;
			break;
		case svc_updateentertime:
			protocollen = 6;
			break;
		case svc_updateuserinfo:
			nullterms = 1;
			protocollen = 6+nullterms;
			break;
		case svcqw_updatestatlong:
			protocollen = 6;
			break;
		case svc_setpause:
			protocollen = 2;
			break;
		case svcfte_cgamepacket:
			protocollen = sizeof(buffer);
			break;
		default:
			Con_DPrintf("QWWriteByte: bad protocol %i\n", (int)data);
			protocollen = sizeof(buffer);
			break;
		}
		majortype = data;
	}
	if (bufferlen == 1 && !protocollen)	//some of them depend on the following bytes for size.
	{
		switch(majortype)
		{
		case svc_temp_entity:
			minortype = data;
			switch(data)
			{
			case TE_LIGHTNING1:
			case TE_LIGHTNING2:
			case TE_LIGHTNING3:
				multicastpos=4;
				multicasttype=MULTICAST_PHS;
				protocollen = (destprim->coordtype&0xf)*6+sizeof(short)+sizeof(qbyte)*2;
				break;
			case TEQW_QWBLOOD:		//needs to be converted to a particle
			case TEQW_QWGUNSHOT:	//needs qbyte 2 removed
				multicastpos=3;
				multicasttype=MULTICAST_PVS;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*3;
				break;
			case TEQW_LIGHTNINGBLOOD:
			case TEQW_QWEXPLOSION:
			case TE_SPIKE:
			case TE_SUPERSPIKE:
				multicastpos=2;
				multicasttype=MULTICAST_PHS_R;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*2;
				break;
			case TE_TAREXPLOSION:
			case TE_WIZSPIKE:
			case TE_KNIGHTSPIKE:
			case TE_LAVASPLASH:
			case TE_TELEPORT:
				multicastpos=2;
				multicasttype=MULTICAST_PVS;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*2;
				break;
			case TEQW_RAILTRAIL:
				multicastpos=1;
				multicasttype=MULTICAST_PVS;
				protocollen = (destprim->coordtype&0xf)*3+sizeof(qbyte)*1;
				break;
			default:
				protocollen = sizeof(buffer);
				if (writedest != &sv.nqmulticast)
					Con_Printf("QWWriteByte: bad tempentity - %i\n", data);
				te_515sevilhackworkaround = true;
				break;
			}
			break;
		default:
			Con_Printf("QWWriteByte: Non-Implemented svc\n");
			protocollen = sizeof(buffer);
			break;
		}
	}

	NPP_AddData(&data, sizeof(qbyte));
	if (!data && bufferlen>=protocollen)
		if (nullterms)
			nullterms--;
	NPP_QWCheckFlush();
}

void NPP_QWWriteChar(int dest, char data)	//replacement write func (nq to qw)
{
	NPP_QWWriteByte(dest, (qbyte)data);
}

void NPP_QWWriteShort(int dest, short data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[2];
		short s;
	} u;
	if (bufferlen == 2 && majortype == svc_temp_entity && (minortype == TE_LIGHTNING1 || minortype == TE_LIGHTNING2 || minortype == TE_LIGHTNING3))
		NPP_QWWriteEntity(dest, data);
	else
	{
		u.s = LittleShort(data);
		NPP_QWWriteByte(dest, u.b[0]);
		NPP_QWWriteByte(dest, u.b[1]);
	}
}

void NPP_QWWriteFloat(int dest, float data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[4];
		float f;
	} u;
	u.f = LittleFloat(data);
	NPP_QWWriteByte(dest, u.b[0]);
	NPP_QWWriteByte(dest, u.b[1]);
	NPP_QWWriteByte(dest, u.b[2]);
	NPP_QWWriteByte(dest, u.b[3]);
}

void NPP_QWWriteLong(int dest, long data)	//replacement write func (nq to qw)
{
	union {
		qbyte b[4];
		int l;
	} u;
	u.l = LittleLong(data);
	NPP_QWWriteByte(dest, u.b[0]);
	NPP_QWWriteByte(dest, u.b[1]);
	NPP_QWWriteByte(dest, u.b[2]);
	NPP_QWWriteByte(dest, u.b[3]);
}
void NPP_QWWriteAngle(int dest, float in)	//replacement write func (nq to qw)
{
	if (destprim->anglesize==1)
	{
		char data = (int)(in*256/360) & 255;
		NPP_QWWriteChar(dest, data);
	}
	else
	{
		short data = (int)(in*0xffff/360) & 0xffff;
		NPP_QWWriteShort(dest, data);
	}
}
void NPP_QWWriteCoord(int dest, float in)	//replacement write func (nq to qw)
{
	if (destprim->coordtype==COORDTYPE_FLOAT_32)
	{
		NPP_QWWriteFloat(dest, in);
	}
	else
	{
		short datas = (int)(in*8);
		NPP_QWWriteShort(dest, datas);
	}
}
void NPP_QWWriteString(int dest, const char *data)	//replacement write func (nq to qw)
{
#if 0
	//the slow but guarenteed routine
	while(*data)
		NPP_QWWriteByte(dest, *data++);
	NPP_QWWriteByte(dest, 0);	//and the null terminator
#else
	//the fast-track, less reliable routine


	NPP_QWCheckDest(dest);

#ifdef NQPROT
	if (dest == MSG_ONE)
	{
		client_t *cl = Write_GetClient();
		if (!cl)
		{
			Con_Printf("msg_entity: not a client\n");
			return;
		}
		else
		{
			if (cl->protocol == SCP_BAD)
				return;
			else if (ISQWCLIENT(cl))
			{
				ClientReliableCheckBlock(cl, strlen(data)+1);
				ClientReliableWrite_String(cl, data);
				return;
			}
		}
	}
	else
		MSG_WriteString (QWWriteDest(dest), data);
#endif

	if (!bufferlen)
	{
		Con_Printf("QWWriteString: Messages should start with WriteByte (last was %i)\n", majortype);
		PR_StackTrace(svprogfuncs, false);
	}

	NPP_AddData(data, strlen(data)+1);
	if (nullterms)
		nullterms--;
	NPP_QWCheckFlush();
#endif
}
void NPP_QWWriteEntity(int dest, int data)	//replacement write func (nq to qw)
{
	if (data >= 0x8000)
	{
		NPP_QWWriteByte(dest, ((data>> 0) & 0xff));
		NPP_QWWriteByte(dest, ((data>> 8) & 0x7f) | 0x80);
		NPP_QWWriteByte(dest, ((data>>15) & 0xff));
	}
	else
	{
		NPP_QWWriteByte(dest, (data>>0) & 0xff);
		NPP_QWWriteByte(dest, (data>>8) & 0x7f);
	}
}
#endif



















#ifdef SERVER_DEMO_PLAYBACK





#define DF_ORIGIN	1
#define DF_ANGLES	(1<<3)
#define DF_EFFECTS	(1<<6)
#define DF_SKINNUM	(1<<7)
#define DF_DEAD		(1<<8)
#define DF_GIB		(1<<9)
#define DF_WEAPONFRAME (1<<10)
#define DF_MODEL	(1<<11)

#define	PF_MSEC			(1<<0)
#define	PF_COMMAND		(1<<1)
#define	PF_VELOCITY1	(1<<2)
#define	PF_VELOCITY2	(1<<3)
#define	PF_VELOCITY3	(1<<4)
#define	PF_MODEL		(1<<5)
#define	PF_SKINNUM		(1<<6)
#define	PF_EFFECTS		(1<<7)
#define	PF_WEAPONFRAME	(1<<8)		// only sent for view player
#define	PF_DEAD			(1<<9)		// don't block movement any more
#define	PF_GIB			(1<<10)		// offset the view height differently




int sv_demo_spikeindex;




void NPP_MVDFlush(void)
{
	if (!bufferlen)
		return;

	switch(majortype)
	{
	case svc_spawnbaseline:
		SV_FlushDemoSignon();
		cldest = NULL;
		writedest = &sv.demosignon;

		if (1)
		{
			int entnum, i;
			mvdentity_state_t *ent;

			if (!sv.demobaselines)
			{
				sv.demobaselines = (mvdentity_state_t*)BZ_Malloc(sizeof(mvdentity_state_t)*MAX_EDICTS);
				sv.demostatevalid = true;
			}
			entnum = buffer[1] + (buffer[2]<<8);
//			if (entnum < svs.allocated_client_slots)
//				break;
			ent = &sv.demobaselines[entnum];

			ent->modelindex = buffer[3];
			ent->frame = buffer[4];
			ent->colormap = buffer[5];
			ent->skinnum = buffer[6];

			for (i=0 ; i<3 ; i++)
			{
				ent->origin[i] = (short)(buffer[7+i*3] + (buffer[8+i*3]<<8))/8.0f;
				ent->angles[i] = buffer[9+i*3];
			}
		}
		break;
	case svc_spawnstatic:
		SV_FlushDemoSignon();
		cldest = NULL;
		writedest = &sv.demosignon;
		break;
	case svc_modellist:
		ignoreprotocol=true;
		{
			int i;
			int rpos, s;
			i = buffer[1];
			rpos = 2;
			while (1)
			{
				i++;
				s = rpos;
				while(buffer[rpos])
					rpos++;

				if (rpos == s)	//end
					break;

				strcpy(sv.demmodel_precache[i], buffer+s);
				if (!strcmp(sv.demmodel_precache[i], "progs/spike.mdl"))
					sv_demo_spikeindex = i;
				rpos++;
			}
		}
		break;
	case svc_soundlist:
		ignoreprotocol=true;
		{
			int i;
			int rpos, s;
			i = buffer[1];
			rpos = 2;
			while (1)
			{
				i++;
				s = rpos;
				while(buffer[rpos])
					rpos++;

				if (rpos == s)	//end
					break;

				strcpy(sv.demsound_precache[i], buffer+s);
				rpos++;
			}
		}
		break;
	case svc_serverdata:
		{
			int i;
			sv_demo_spikeindex = 0;	//new map, new precaches.

			i = 9;
			strcpy(sv.demgamedir, buffer+i);
			for(;i < bufferlen && buffer[i];i++)
				;
			i++;
			i+=4;
			Q_strncpyz(sv.demfullmapname, buffer+i, sizeof(sv.demfullmapname));
			for(;i < bufferlen && buffer[i];i++)
				;
			i+=4*10;
		}
		ignoreprotocol=true;
		break;
	case svc_lightstyle:
		sv.demolightstyles[buffer[1]] = Hunk_Alloc(strlen(buffer+2)+1);
		strcpy(sv.demolightstyles[buffer[1]], buffer+2);
		break;

	case svc_updatestat:
	case svc_updatestatlong:	//make sure we update the running players stats properly.
		{
			int v, s;
			if (majortype == svc_updatestat)
				v = buffer[2];
			else
				v = buffer[2] | (buffer[3]<<8) | (buffer[4]<<16) | (buffer[5]<<24);
			s = buffer[1];

			if (sv.lastto < 32)	//dem_multicast could be used at the wrong time...
				sv.recordedplayer[sv.lastto].stats[s] = v;

			ignoreprotocol=true;
		}
		break;
	case svc_packetentities:
	case svc_deltapacketentities:	//read the delta in to the array.
		ignoreprotocol=true;		//a bug exists in that the delta MUST have been reliably recorded.
		{
			int i;
			int entnum;
			mvdentity_state_t *ents;
			unsigned short s;
			if (!sv.demostate)
			{
				sv.demostate = BZ_Malloc(sizeof(mvdentity_state_t)*MAX_EDICTS);
				sv.demostatevalid = true;
			}
			i = majortype-svc_packetentities+1;
			while (1)
			{
				s = buffer[i] + buffer[i+1]*256;
				i+=2;
				if (!s)
				{
					break;
				}
				else
				{
					entnum = s&511;
					s &= ~511;

					if (entnum > sv.demomaxents)
						sv.demomaxents = entnum;

					ents = &sv.demostate[entnum];
					if (s & U_REMOVE)
					{	//this entity went from the last packet
						ents->modelindex = 0;
						ents->effects = 0;
						continue;
					}

					if (!ents->modelindex && !ents->effects && sv.demobaselines)
					{	//new entity, reset to baseline
						memcpy(ents, &sv.demobaselines[entnum], sizeof(mvdentity_state_t));
					}

					if (s & U_MOREBITS)
					{
						s |= buffer[i];
						i++;
					}

					if (s & U_MODEL)
					{
						ents->modelindex = buffer[i];
						i++;
					}

					if (s & U_FRAME)
					{
						ents->frame = buffer[i];
						i++;
					}

					if (s & U_COLORMAP)
					{
						ents->colormap = buffer[i];
						i++;
					}

					if (s & U_SKIN)
					{
						ents->skinnum = buffer[i];
						i++;
					}

					if (s & U_EFFECTS)
					{
						ents->effects = buffer[i];
						i++;
					}

					if (s & U_ORIGIN1)
					{
						ents->origin[0] = (short)(buffer[i]+buffer[i+1]*256) /8.0f;
						i+=2;
					}

					if (s & U_ANGLE1)
					{
						ents->angles[0] = (unsigned char)(buffer[i]);//	* (360.0/256);
						i++;
					}

					if (s & U_ORIGIN2)
					{
						ents->origin[1] = (short)(buffer[i]+buffer[i+1]*256) /8.0f;
						i+=2;
					}

					if (s & U_ANGLE2)
					{
						ents->angles[1] = (unsigned char)(buffer[i]);//	* (360.0/256);
						i++;
					}

					if (s & U_ORIGIN3)
					{
						ents->origin[2] = (short)(buffer[i]+buffer[i+1]*256) /8.0f;
						i+=2;
					}

					if (s & U_ANGLE3)
					{
						ents->angles[2] = (unsigned char)(buffer[i]);//	* (360.0/256);
						i++;
					}
				}
			}
		}
		break;
	case svc_playerinfo:
		ignoreprotocol=true;
		{
			int i, j;
			unsigned short flags;
			mvdentity_state_t *ents;
			int playernum;
			vec3_t oldang;

			if (!sv.demostate)
				sv.demostate = BZ_Malloc(sizeof(entity_state_t)*MAX_EDICTS);

			sv.demostatevalid = true;

			flags = buffer[2] + buffer[3]*256;

			playernum = buffer[1];
			ents = &sv.demostate[playernum+1];
			ents->frame = buffer[4];

//			ents->colormap=playernum+1;

			VectorCopy(ents->origin, sv.recordedplayer[playernum].oldorg);
			VectorCopy(ents->angles, sv.recordedplayer[playernum].oldang);

			i = 5;
			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ORIGIN << j))
				{
					ents->origin[j] = (signed short)(buffer[i] + (buffer[i+1]<<8))/8.0f;
					i+=2;
				}

			VectorCopy(ents->angles, oldang);
			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ANGLES << j))
				{
					//FIXME: angle truncation here.
					ents->angles[j] = (char)((int)(buffer[i] + (buffer[i+1]<<8))/256.0f);
					i+=2;
				}

			if (flags & (DF_ANGLES << 0))	//'stupid quake bug' I believe is the correct quote...
				ents->angles[0] = ents->angles[0]*-1/3.0f; //also scale pitch down as well as invert

			if (flags & DF_MODEL)
			{
				ents->modelindex = buffer[i];
				i+=1;
			}
			if (flags & DF_SKINNUM)
			{
				ents->skinnum = buffer[i];
				i+=1;
			}
			if (flags & DF_EFFECTS)
			{
				ents->effects = buffer[i];
				i+=1;
			}
			if (flags & DF_WEAPONFRAME)
			{	//mvds are deltas remember, this is really the only place where that fact is all that important.
				sv.recordedplayer[playernum].weaponframe = buffer[i];
				i+=1;
			}

			sv.recordedplayer[playernum].updatetime = realtime;

			ignoreprotocol=true;
		}
		break;

	case svc_nails:
	case svc_nails2:
		sv.numdemospikes = buffer[1];
		{
			qboolean hasid = (majortype==svc_nails2);
			char *bits;
			int i;
			bits = buffer+2;
			for (i = 0; i < sv.numdemospikes; i++)
			{
				if (hasid)
				{
					sv.demospikes[i].id = *bits;
					bits++;
				}
				else
					sv.demospikes[i].id = 0;
				sv.demospikes[i].modelindex = sv_demo_spikeindex;
				sv.demospikes[i].org[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
				sv.demospikes[i].org[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
				sv.demospikes[i].org[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
				sv.demospikes[i].pitch = (bits[4]>>4);
				sv.demospikes[i].yaw = bits[5];

				bits+=6;
			}
		}
		ignoreprotocol=true;
		break;

	case svc_stufftext:
		ignoreprotocol = true;
		Cmd_TokenizeString(buffer+1, false, false);
		if (!stricmp(Cmd_Argv(0), "fullserverinfo"))
		{
			Q_strncpyz(sv.demoinfo, Cmd_Argv(1), sizeof(sv.demoinfo));
			break;
		}
		break;
	case svc_updateping:
		{
			int j;
			j = buffer[1];
			sv.recordedplayer[j].ping = buffer[2] | (buffer[3]<<8);
		}
		break;
	case svc_updatepl:
		{
			int j;
			j = buffer[1];
			sv.recordedplayer[j].pl = buffer[2] | (buffer[3]<<8);
		}
		break;
	case svc_updatefrags:
		{
			int j;
			j = buffer[1];
			sv.recordedplayer[j].frags = buffer[2] | (buffer[3]<<8);
		}
		break;
	case svc_setinfo:
//		ignoreprotocol = true;
		{
			int j;
			j = buffer[1];
			Info_SetValueForStarKey(sv.recordedplayer[j].userinfo, buffer+2, buffer+2+strlen(buffer+2)+1, sizeof(sv.recordedplayer[j].userinfo));
		}
		break;
	case svc_updateuserinfo:
//		ignoreprotocol = true;
		{
			unsigned int j;
			j = buffer[1];
			if (j < sv.allocated_client_slots)
			{
				sv.recordedplayer[j].userid = buffer[2] | (buffer[3]<<8) | (buffer[4]<<16) | (buffer[5]<<24);
				Q_strncpyz(sv.recordedplayer[j].userinfo, buffer+6, sizeof(sv.recordedplayer[j].userinfo));
			}
		}
		break;
	case svc_setangle:	//FIXME: forward on to trackers.
		ignoreprotocol = true;
		break;

	case svc_disconnect:
		svs.spawncount++;
		SV_BroadcastCommand("changing\n");
		SV_BroadcastCommand ("reconnect\n");
		ignoreprotocol = true;
		break;
	}
	if (ignoreprotocol)
	{
		ignoreprotocol=false;
		bufferlen = 0;
	}




	if (cldest)
	{
		if (!requireextension || cldest->fteprotocolextensions & requireextension)
		if (bufferlen)
		{
			ClientReliableCheckBlock(cldest, bufferlen);
			ClientReliableWrite_SZ(cldest, buffer, bufferlen);
		}
		cldest = NULL;
	}
	else
	{
		if (multicastpos && (writedest == &sv.datagram || writedest == &sv.multicast))
			writedest = &sv.multicast;
		else
			multicastpos = 0;

		if (writedest == &sv.reliable_datagram)
		{
			writedest = &sv.multicast;
			multicasttype = MULTICAST_ALL_R;
		}

		if (bufferlen)
			SZ_Write(writedest, buffer, bufferlen);

		if (multicastpos)
		{
			vec3_t org;
			coorddata cd;

			memcpy(&cd, &buffer[multicastpos+sizeofcoord*0], sizeofcoord);
			org[0] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*1], sizeofcoord);
			org[1] = MSG_FromCoord(cd, sizeofcoord);
			memcpy(&cd, &buffer[multicastpos+sizeofcoord*2], sizeofcoord);
			org[2] = MSG_FromCoord(cd, sizeofcoord);

			SV_MulticastProtExt(org, multicasttype, FULLDIMENSIONMASK, requireextension, 0);
		}
		else if (writedest == &sv.multicast)
			SV_MulticastProtExt(vec3_origin, multicasttype, FULLDIMENSIONMASK, requireextension, 0);
		writedest = NULL;
	}
	bufferlen = 0;
	protocollen=0;
	multicastpos=0;
	requireextension=0;
}
void NPP_MVDForceFlush(void)
{
	if (bufferlen)
	{
		Con_Printf("Forcing flush mvd->qw prot\n");
		Con_Printf("(last was %i)\n", (int)majortype);
		NPP_MVDFlush();
	}
}
void NPP_MVDCheckFlush(void)
{
	if (bufferlen >= protocollen && protocollen)
		NPP_MVDFlush();
}

void NPP_MVDCheckDest(client_t *cl, int broadcast)
{
	if (!broadcast)
	{
		if (!cl)
		{
			Con_Printf("Not a client\n");
			return;
		}
		if ((cldest && cldest != cl) || writedest)
		{
			Con_Printf("MSG destination changed in the middle of a packet.\n");
			NPP_MVDFlush();
		}
		cldest = cl;
	}
	else
	{
		sizebuf_t	*ndest = &sv.reliable_datagram;
		if (cldest || (writedest && writedest != ndest))
		{
			Con_Printf("MSG destination changed in the middle of a packet.\n");
			NPP_MVDFlush();
		}
		writedest = ndest;
	}
}

void NPP_MVDWriteByte(qbyte data, client_t *to, int broadcast)	//replacement write func (nq to qw)
{
	int i;
	NPP_MVDCheckDest(to, broadcast);

	if (!bufferlen)	//new message section
	{
		switch(data)
		{
		case svc_temp_entity://depends on following bytes
			break;
		case svc_serverinfo:
		case svc_print:
		case svc_sound:
		case svc_serverdata:
		case svc_stufftext:
		case svc_modellist:
		case svc_soundlist:
		case svc_updateuserinfo:
		case svc_playerinfo:
		case svc_packetentities:
		case svc_deltapacketentities:
		case svc_lightstyle:
		case svc_nails:
		case svc_nails2:
		case svc_centerprint:
		case svc_setinfo:
			break;
		case svc_setangle:
			if (sv.mvdplayback)
				protocollen = sizeof(qbyte)*5;	//MVDSV writes an extra client num too.
			else
				protocollen = sizeof(qbyte)*4;	//MVDSV writes an extra client num too.
			break;
		case svc_setview:
			protocollen = sizeof(qbyte)*1 + sizeof(short);
			break;
		case svc_cdtrack:
			protocollen = sizeof(qbyte)*2;
			break;
		case svc_killedmonster:
			protocollen = 1;
			break;
		case svc_foundsecret:
			protocollen = 1;
			break;
//		case svc_intermission:
//			protocollen = 1;
//			break;
//		case svc_finale:
//			protocollen = 2;
//			break;
		case svc_muzzleflash:
			protocollen = 3;
			break;
		case svc_smallkick:
		case svc_bigkick:
			protocollen = 1;
			break;

		case svc_spawnstaticsound:
			protocollen = 10;
			break;
		case svc_spawnstatic:
			protocollen = 14;
			break;
		case svc_spawnbaseline:
			protocollen = 16;
			break;
		case svc_updateping:
		case svc_updatefrags:
			protocollen = 4;
			break;
		case svc_updatestat:
		case svc_updatepl:
			protocollen = 3;
			break;
		case svc_updatestatlong:
			protocollen = 6;
			break;
		case svc_updateentertime:
			protocollen = 6;
			break;
		case svc_intermission:
			protocollen = 10;
			break;
		case svc_disconnect:
			protocollen = 2+strlen("EndOfDemo");
			break;
		case svc_chokecount:
			protocollen = 2;
			break;
		case svc_damage:
			protocollen = 9;
			break;
		default:
			Con_Printf("mvd: bad protocol %i\n", (int)data);
			Con_Printf("(last was %i)\n", (int)majortype);
			protocollen = sizeof(buffer);
			net_message.cursize=0;
			data = svc_nop;
			protocollen = 1;
			break;
		}
		majortype = data;
	}
	else if (!protocollen)
	{
		switch(majortype)
		{
		case svc_temp_entity:
			if (bufferlen == 1)
			{
				minortype = data;
				switch(data)
				{
				case TE_LIGHTNING1:
				case TE_LIGHTNING2:
				case TE_LIGHTNING3:
					multicastpos=4;
					multicasttype=MULTICAST_PHS;
					protocollen = sizeof(short)*6+sizeof(short)+sizeof(qbyte)*2;
					break;
				case TE_BLOOD:		//needs to be converted to a particle
				case TE_GUNSHOT:	//needs qbyte 2 removed
					multicastpos=3;
					multicasttype=MULTICAST_PVS;
					protocollen = sizeof(short)*3+sizeof(qbyte)*3;
					break;
				case TE_LIGHTNINGBLOOD:
				case TE_EXPLOSION:
				case TE_SPIKE:
				case TE_SUPERSPIKE:
					multicastpos=2;
					multicasttype=MULTICAST_PHS_R;
					protocollen = sizeof(short)*3+sizeof(qbyte)*2;
					break;
				case TE_TAREXPLOSION:
				case TE_WIZSPIKE:
				case TE_KNIGHTSPIKE:
				case TE_LAVASPLASH:
				case TE_TELEPORT:
					multicastpos=2;
					multicasttype=MULTICAST_PVS;
					protocollen = sizeof(short)*3+sizeof(qbyte)*2;
					break;
				default:
					protocollen = sizeof(buffer);
					Con_Printf("bad tempentity\n");
					break;
				}
			}
			break;
		case svc_serverdata:
			if (bufferlen > 9)
			{
				i = 9;
				for(;i < bufferlen && buffer[i];i++)
					;
				i++;
				i+=4;
				for(;i < bufferlen && buffer[i];i++)
					;
				i+=4*10;
				if (i <= bufferlen)
					protocollen = i;
			}

			break;
		case svc_stufftext:
			if (!data)	//terminated by a null term
				protocollen = bufferlen;
			break;

		case svc_modellist:
		case svc_soundlist:
			if (!data)
				if (!buffer[bufferlen-1])	//two null bytes marks the last string
					protocollen = bufferlen+2;
			break;

		case svc_updateuserinfo:	//6 bytes then a string
			if (!data && bufferlen>=6)
				protocollen = bufferlen+1;
			break;

		case svc_playerinfo:
			if (bufferlen==4)
			{
				unsigned short pflags;
				int j;
				ignoreprotocol=true;
				pflags = buffer[2] + (buffer[3]*256);	//little endian

				protocollen = 4+1;

				for (j=0 ; j<3 ; j++)
					if (pflags & (DF_ORIGIN << j))
						protocollen += 2;

				for (j=0 ; j<3 ; j++)
					if (pflags & (DF_ANGLES << j))
						protocollen += 2;


				if (pflags & DF_MODEL)
					protocollen += 1;

				if (pflags & DF_SKINNUM)
					protocollen += 1;

				if (pflags & DF_EFFECTS)
					protocollen += 1;

				if (pflags & DF_WEAPONFRAME)
					protocollen += 1;

/*
				if (pflags & PF_MSEC)
					protocollen+=1;

				if (pflags & PF_COMMAND)
				{
					Con_Printf("svc_playerinfo PF_COMMAND not expected in mvd\n");
					protocollen+=500;
				}

				for (i=0 ; i<3 ; i++)
					if (pflags & (PF_VELOCITY1<<i) )
						protocollen+=2;

				if (pflags & PF_MODEL)
					protocollen+=1;

				if (pflags & PF_SKINNUM)
					protocollen+=1;

				if (pflags & PF_EFFECTS)
					protocollen+=1;

				if (pflags & PF_WEAPONFRAME)
					protocollen+=1;
*/
			}
			break;

		case svc_packetentities:
		case svc_deltapacketentities:
			if (!data)	//two last bytes are 0.
			{
				unsigned short s;
				i = majortype-svc_packetentities+1;
				buffer[bufferlen] = data;
				bufferlen++;
				while (1)
				{
					s = buffer[i] + buffer[i+1]*256;
					i+=2;
					if (i > bufferlen)
						break;
					if (!s)
					{
						if (i <= bufferlen)
							protocollen = bufferlen;
						break;
					}
					else
					{
						s &= ~511;
						if (s & U_MOREBITS)
						{
							s |= buffer[i];
							i++;
						}

						if (s & U_MODEL)
							i++;

						if (s & U_FRAME)
							i++;

						if (s & U_COLORMAP)
							i++;

						if (s & U_SKIN)
							i++;

						if (s & U_EFFECTS)
							i++;

						if (s & U_ORIGIN1)
							i+=2;

						if (s & U_ANGLE1)
							i++;

						if (s & U_ORIGIN2)
							i+=2;

						if (s & U_ANGLE2)
							i++;

						if (s & U_ORIGIN3)
							i+=2;

						if (s & U_ANGLE3)
							i++;
					}
				}
				bufferlen--;
			}
			break;

		case svc_lightstyle:
			if (!data && bufferlen>=2)
				protocollen = bufferlen+1;
			break;

		case svc_serverinfo:	//looking for two null terminators
			if (!data)	//gotta be the second.
			{
				for (i = 0; i < bufferlen; i++)
				{
					if (!buffer[i])	//we already wrote one.
					{
						protocollen = bufferlen+1;	//so this is the last qbyte
						break;
					}
				}
			}
			break;

		case svc_sound:
			if (bufferlen == 3) //decide after 3
			{
				unsigned short s;
				i = 10;
				s = buffer[1] + buffer[2]*256;
				if (s & SND_VOLUME)
					i++;
				if (s & SND_ATTENUATION)
					i++;
				protocollen = i;
			}
			break;

		case svc_setinfo:
			if (!data && bufferlen>2)
			{
				for (i = 2; i < bufferlen; i++)
				{
					if (!buffer[i])
					{
						protocollen = bufferlen+1;
						break;
					}
				}
			}
			break;

		case svc_centerprint:
			if (!data && bufferlen>=1)
				protocollen = bufferlen+1;
			break;
		case svc_print:
			if (!data && bufferlen>=2)
				protocollen = bufferlen+1;
			break;

		case svc_nails:
			if (bufferlen == 1)
			{
				i = data;	//first qbyte is id, second, (the current, about to be written) is number of nails.
				protocollen = (i * 6) + 2;
			}
			break;
		case svc_nails2:
			if (bufferlen == 1)
			{
				i = data;	//first qbyte is id, second, (the current, about to be written) is number of nails.
				protocollen = (i * 7) + 2;
			}
			break;
		default:
			Con_Printf("mvd: bad protocol %i\n", (int)data);
			protocollen = sizeof(buffer);
			net_message.cursize=0;
			data = svc_nop;
			protocollen = 1;
			break;
		}
	}


	NPP_AddData(&data, sizeof(qbyte));
	NPP_MVDCheckFlush();
}
#endif //SERVER_DEMO_PLAYBACK

void NPP_Flush(void)
{
	if (progstype != PROG_QW)
		NPP_NQFlush();
#ifdef NQPROT
	else
		NPP_QWFlush();
#endif
}
#endif
#endif
