/*
Copyright (C) 2006-2021 DarkPlaces contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef CSPROGS_H
#define CSPROGS_H

// LadyHavoc: changed to match MAX_EDICTS
#define CL_MAX_EDICTS MAX_EDICTS

#define ENTMASK_ENGINE				1
#define ENTMASK_ENGINEVIEWMODELS	2
#define ENTMASK_NORMAL				4

#define VF_MIN			1	//(vector)
#define VF_MIN_X		2	//(float)
#define VF_MIN_Y		3	//(float)
#define VF_SIZE			4	//(vector) (viewport size)
#define VF_SIZE_X		5	//(float)
#define VF_SIZE_Y		6	//(float)
#define VF_VIEWPORT		7	//(vector, vector)
#define VF_FOV			8	//(vector)
#define VF_FOVX			9	//(float)
#define VF_FOVY			10	//(float)
#define VF_ORIGIN		11	//(vector)
#define VF_ORIGIN_X		12	//(float)
#define VF_ORIGIN_Y		13	//(float)
#define VF_ORIGIN_Z		14	//(float)
#define VF_ANGLES		15	//(vector)
#define VF_ANGLES_X		16	//(float)
#define VF_ANGLES_Y		17	//(float)
#define VF_ANGLES_Z		18	//(float)

#define VF_DRAWWORLD		19	//(float)	//actually world model and sky
#define VF_DRAWENGINESBAR	20	//(float)
#define VF_DRAWCROSSHAIR	21	//(float)

#define VF_CL_VIEWANGLES	33	//(vector)	//sweet thing for RPGs/...
#define VF_CL_VIEWANGLES_X	34	//(float)
#define VF_CL_VIEWANGLES_Y	35	//(float)
#define VF_CL_VIEWANGLES_Z	36	//(float)

// FTEQW's extension range
#define VF_PERSPECTIVE		200 //(float)

// what is this doing here? This is a DP extension introduced by Black, should be in 4xx range
#define VF_CLEARSCREEN		201 //(float)

// what is this doing here? This is a DP extension introduced by VorteX, should be in 4xx range
#define VF_FOG_DENSITY		202 //(float)
#define VF_FOG_COLOR		203 //(vector)
#define VF_FOG_COLOR_R		204 //(float)
#define VF_FOG_COLOR_G		205 //(float)
#define VF_FOG_COLOR_B		206 //(float)
#define VF_FOG_ALPHA		207 //(float)
#define VF_FOG_START		208 //(float)
#define VF_FOG_END   		209 //(float)
#define VF_FOG_HEIGHT		210 //(float)
#define VF_FOG_FADEDEPTH	211 //(float)

// DP's extension range
#define VF_MAINVIEW		400 //(float)
#define VF_MINFPS_QUALITY	401 //(float)

#define RF_VIEWMODEL		1	// The entity is never drawn in mirrors. In engines with realtime lighting, it casts no shadows.
#define RF_EXTERNALMODEL	2	// The entity is appears in mirrors but not in the normal view. It does still cast shadows in engines with realtime lighting.
#define RF_DEPTHHACK		4	// The entity appears closer to the view than normal, either by scaling it wierdly or by just using a depthrange. This will usually be found in conjunction with RF_VIEWMODEL
#define RF_ADDITIVE			8	// Add the entity acording to it's alpha values instead of the normal blend
#define RF_USEAXIS			16	// When set, the entity will use the v_forward, v_right and v_up globals instead of it's angles field for orientation. Angles will be ignored compleatly.
								// Note that to use this properly, you'll NEED to use the predraw function to set the globals.
//#define RF_DOUBLESIDED		32
#define RF_USETRANSPARENTOFFSET 64   // Allows QC to customize origin used for transparent sorting via transparent_origin global, helps to fix transparent sorting bugs on a very large entities
#define RF_WORLDOBJECT          128  // for large outdoor entities that should not be culled
#define RF_MODELLIGHT           4096 // CSQC-set model light
#define RF_DYNAMICMODELLIGHT    8192 // origin-dependent model light

#define RF_FULLBRIGHT			256
#define RF_NOSHADOW				512

extern cvar_t csqc_progname;	//[515]: csqc crc check and right csprogs name according to progs.dat
extern cvar_t csqc_progcrc;
extern cvar_t csqc_progsize;
extern cvar_t csqc_polygons_defaultmaterial_nocullface;
extern cvar_t csqc_lowres;

void CL_VM_PreventInformationLeaks(void);

qbool MakeDownloadPacket(const char *filename, unsigned char *data, size_t len, int crc, int cnt, sizebuf_t *buf, int protocol);

qbool CL_VM_GetEntitySoundOrigin(int entnum, vec3_t out);

qbool CL_VM_TransformView(int entnum, matrix4x4_t *viewmatrix, mplane_t *clipplane, vec3_t visorigin);

void CL_VM_Init(void);
void CL_VM_ShutDown(void);
void CL_VM_UpdateIntermissionState(int intermission);
void CL_VM_UpdateShowingScoresState(int showingscores);
qbool CL_VM_InputEvent(int eventtype, float x, float y);
qbool CL_VM_ConsoleCommand(const char *text, size_t textlen);
void CL_VM_UpdateDmgGlobals(int dmg_take, int dmg_save, vec3_t dmg_origin);
void CL_VM_UpdateIntermissionState(int intermission);
qbool CL_VM_Event_Sound(int sound_num, float volume, int channel, float attenuation, int ent, vec3_t pos, int flags, float speed);
qbool CL_VM_Parse_TempEntity(void);
void CL_VM_Parse_StuffCmd(const char *msg, size_t msg_len);
void CL_VM_Parse_CenterPrint(const char *msg, size_t msg_len);
int CL_GetPitchSign(prvm_prog_t *prog, prvm_edict_t *ent);
int CL_GetTagMatrix(prvm_prog_t *prog, matrix4x4_t *out, prvm_edict_t *ent, int tagindex, prvm_vec_t *shadingorigin);
void CL_GetEntityMatrix(prvm_prog_t *prog, prvm_edict_t *ent, matrix4x4_t *out, qbool viewmatrix);
void QW_CL_StartUpload(unsigned char *data, int size);

void CSQC_UpdateNetworkTimes(double newtime, double oldtime);
void CSQC_AddPrintText(const char *msg, size_t msg_len);
void CSQC_ReadEntities(void);
void CSQC_RelinkAllEntities(int drawmask);
void CSQC_RelinkCSQCEntities(void);
void CSQC_Predraw(prvm_edict_t *ed);
void CSQC_Think(prvm_edict_t *ed);
qbool CSQC_AddRenderEdict(prvm_edict_t *ed, int edictnum);//csprogs.c
void CSQC_R_RecalcView(void);

model_t *CL_GetModelByIndex(int modelindex);

int CL_VM_GetViewEntity(void);

#endif
