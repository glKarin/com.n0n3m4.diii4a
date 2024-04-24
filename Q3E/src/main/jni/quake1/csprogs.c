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

#include "quakedef.h"
#include "progsvm.h"
#include "clprogdefs.h"
#include "csprogs.h"
#include "cl_collision.h"
#include "snd_main.h"
#include "clvm_cmds.h"
#include "prvm_cmds.h"

//============================================================================
// Client prog handling
//[515]: omg !!! optimize it ! a lot of hacks here and there also :P

#define CSQC_RETURNVAL	prog->globals.fp[OFS_RETURN]

void CL_VM_PreventInformationLeaks(void)
{
	prvm_prog_t *prog = CLVM_prog;

	if(!prog->loaded)
		return;

	VM_ClearTraceGlobals(prog);
	PRVM_clientglobalfloat(trace_networkentity) = 0;
}


/** Previous DP versions declined to load csprogs if it lacked any of:
 * CSQC_Init, CSQC_InputEvent, CSQC_UpdateView, CSQC_ConsoleCommand
 * whereas in FTE and QSS-based engines the minimum is either CSQC_UpdateView
 * or CSQC_DrawHud (only called in CSQC_SIMPLE aka hud-only mode)
 * and the other funcs are optional, so we now behave the same here.
 */
static void CL_CheckRequiredFuncs(prvm_prog_t *prog, const char *filename)
{
	if (PRVM_ED_FindFunction(prog, "CSQC_UpdateView"))
		return;
	else if (PRVM_ED_FindFunction(prog, "CSQC_DrawHud"))
		prog->flag |= PRVM_CSQC_SIMPLE;
	else
		prog->error_cmd("%s: no CSQC_UpdateView (EXT_CSQC) or CSQC_DrawHud (CSQC_SIMPLE) function found in %s", prog->name, filename);
}

#define CL_REQFIELDS (sizeof(cl_reqfields) / sizeof(prvm_required_field_t))

prvm_required_field_t cl_reqfields[] =
{
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x)
#define PRVM_DECLARE_clientglobalvector(x)
#define PRVM_DECLARE_clientglobalstring(x)
#define PRVM_DECLARE_clientglobaledict(x)
#define PRVM_DECLARE_clientglobalfunction(x)
#define PRVM_DECLARE_menuglobalfloat(x)
#define PRVM_DECLARE_menuglobalvector(x)
#define PRVM_DECLARE_menuglobalstring(x)
#define PRVM_DECLARE_menuglobaledict(x)
#define PRVM_DECLARE_menuglobalfunction(x)
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x) {ev_float, #x },
#define PRVM_DECLARE_clientfieldvector(x) {ev_vector, #x },
#define PRVM_DECLARE_clientfieldstring(x) {ev_string, #x },
#define PRVM_DECLARE_clientfieldedict(x) {ev_entity, #x },
#define PRVM_DECLARE_clientfieldfunction(x) {ev_function, #x },
#define PRVM_DECLARE_menufieldfloat(x)
#define PRVM_DECLARE_menufieldvector(x)
#define PRVM_DECLARE_menufieldstring(x)
#define PRVM_DECLARE_menufieldedict(x)
#define PRVM_DECLARE_menufieldfunction(x)
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

#define CL_REQGLOBALS (sizeof(cl_reqglobals) / sizeof(prvm_required_field_t))

prvm_required_field_t cl_reqglobals[] =
{
#define PRVM_DECLARE_serverglobalfloat(x)
#define PRVM_DECLARE_serverglobalvector(x)
#define PRVM_DECLARE_serverglobalstring(x)
#define PRVM_DECLARE_serverglobaledict(x)
#define PRVM_DECLARE_serverglobalfunction(x)
#define PRVM_DECLARE_clientglobalfloat(x) {ev_float, #x},
#define PRVM_DECLARE_clientglobalvector(x) {ev_vector, #x},
#define PRVM_DECLARE_clientglobalstring(x) {ev_string, #x},
#define PRVM_DECLARE_clientglobaledict(x) {ev_entity, #x},
#define PRVM_DECLARE_clientglobalfunction(x) {ev_function, #x},
#define PRVM_DECLARE_menuglobalfloat(x)
#define PRVM_DECLARE_menuglobalvector(x)
#define PRVM_DECLARE_menuglobalstring(x)
#define PRVM_DECLARE_menuglobaledict(x)
#define PRVM_DECLARE_menuglobalfunction(x)
#define PRVM_DECLARE_serverfieldfloat(x)
#define PRVM_DECLARE_serverfieldvector(x)
#define PRVM_DECLARE_serverfieldstring(x)
#define PRVM_DECLARE_serverfieldedict(x)
#define PRVM_DECLARE_serverfieldfunction(x)
#define PRVM_DECLARE_clientfieldfloat(x)
#define PRVM_DECLARE_clientfieldvector(x)
#define PRVM_DECLARE_clientfieldstring(x)
#define PRVM_DECLARE_clientfieldedict(x)
#define PRVM_DECLARE_clientfieldfunction(x)
#define PRVM_DECLARE_menufieldfloat(x)
#define PRVM_DECLARE_menufieldvector(x)
#define PRVM_DECLARE_menufieldstring(x)
#define PRVM_DECLARE_menufieldedict(x)
#define PRVM_DECLARE_menufieldfunction(x)
#define PRVM_DECLARE_serverfunction(x)
#define PRVM_DECLARE_clientfunction(x)
#define PRVM_DECLARE_menufunction(x)
#define PRVM_DECLARE_field(x)
#define PRVM_DECLARE_global(x)
#define PRVM_DECLARE_function(x)
#include "prvm_offsets.h"
#undef PRVM_DECLARE_serverglobalfloat
#undef PRVM_DECLARE_serverglobalvector
#undef PRVM_DECLARE_serverglobalstring
#undef PRVM_DECLARE_serverglobaledict
#undef PRVM_DECLARE_serverglobalfunction
#undef PRVM_DECLARE_clientglobalfloat
#undef PRVM_DECLARE_clientglobalvector
#undef PRVM_DECLARE_clientglobalstring
#undef PRVM_DECLARE_clientglobaledict
#undef PRVM_DECLARE_clientglobalfunction
#undef PRVM_DECLARE_menuglobalfloat
#undef PRVM_DECLARE_menuglobalvector
#undef PRVM_DECLARE_menuglobalstring
#undef PRVM_DECLARE_menuglobaledict
#undef PRVM_DECLARE_menuglobalfunction
#undef PRVM_DECLARE_serverfieldfloat
#undef PRVM_DECLARE_serverfieldvector
#undef PRVM_DECLARE_serverfieldstring
#undef PRVM_DECLARE_serverfieldedict
#undef PRVM_DECLARE_serverfieldfunction
#undef PRVM_DECLARE_clientfieldfloat
#undef PRVM_DECLARE_clientfieldvector
#undef PRVM_DECLARE_clientfieldstring
#undef PRVM_DECLARE_clientfieldedict
#undef PRVM_DECLARE_clientfieldfunction
#undef PRVM_DECLARE_menufieldfloat
#undef PRVM_DECLARE_menufieldvector
#undef PRVM_DECLARE_menufieldstring
#undef PRVM_DECLARE_menufieldedict
#undef PRVM_DECLARE_menufieldfunction
#undef PRVM_DECLARE_serverfunction
#undef PRVM_DECLARE_clientfunction
#undef PRVM_DECLARE_menufunction
#undef PRVM_DECLARE_field
#undef PRVM_DECLARE_global
#undef PRVM_DECLARE_function
};

void CL_VM_UpdateDmgGlobals (int dmg_take, int dmg_save, vec3_t dmg_origin)
{
	prvm_prog_t *prog = CLVM_prog;

	if(prog->loaded)
	{
		PRVM_clientglobalfloat(dmg_take) = dmg_take;
		PRVM_clientglobalfloat(dmg_save) = dmg_save;
		VectorCopy(dmg_origin, PRVM_clientglobalvector(dmg_origin));
	}
}

void CSQC_UpdateNetworkTimes(double newtime, double oldtime)
{
	prvm_prog_t *prog = CLVM_prog;

	if(!prog->loaded)
		return;

	PRVM_clientglobalfloat(servertime) = newtime;
	PRVM_clientglobalfloat(serverprevtime) = oldtime;
	PRVM_clientglobalfloat(serverdeltatime) = newtime - oldtime;
}

//[515]: set globals before calling R_UpdateView, WEIRD CRAP
static void CSQC_SetGlobals (double frametime)
{
	vec3_t pmove_org;
	prvm_prog_t *prog = CLVM_prog;

	PRVM_clientglobalfloat(time) = cl.time;
	PRVM_clientglobalfloat(cltime) = host.realtime; // Spike named it that way.
	PRVM_clientglobalfloat(frametime) = frametime;
	PRVM_clientglobalfloat(servercommandframe) = cls.servermovesequence;
	PRVM_clientglobalfloat(clientcommandframe) = cl.movecmd[0].sequence;
	VectorCopy(cl.viewangles, PRVM_clientglobalvector(input_angles));
	// // FIXME: this actually belongs into getinputstate().. [12/17/2007 Black]
	PRVM_clientglobalfloat(input_buttons) = cl.movecmd[0].buttons;
	VectorSet(PRVM_clientglobalvector(input_movevalues), cl.movecmd[0].forwardmove, cl.movecmd[0].sidemove, cl.movecmd[0].upmove);
	VectorCopy(cl.csqc_vieworiginfromengine, cl.csqc_vieworigin);
	VectorCopy(cl.csqc_viewanglesfromengine, cl.csqc_viewangles);

	// LadyHavoc: Spike says not to do this, but without pmove_org the
	// CSQC is useless as it can't alter the view origin without
	// completely replacing it
	Matrix4x4_OriginFromMatrix(&cl.entities[cl.viewentity].render.matrix, pmove_org);
	VectorCopy(pmove_org, PRVM_clientglobalvector(pmove_org));
	VectorCopy(cl.movement_velocity, PRVM_clientglobalvector(pmove_vel));
	PRVM_clientglobalfloat(pmove_onground) = cl.onground;
	PRVM_clientglobalfloat(pmove_inwater) = cl.inwater;

	VectorCopy(cl.viewangles, PRVM_clientglobalvector(view_angles));
	VectorCopy(cl.punchangle, PRVM_clientglobalvector(view_punchangle));
	VectorCopy(cl.punchvector, PRVM_clientglobalvector(view_punchvector));
	PRVM_clientglobalfloat(maxclients) = cl.maxclients;

	PRVM_clientglobalfloat(player_localentnum) = cl.viewentity;

	CSQC_R_RecalcView();
}

void CSQC_Predraw (prvm_edict_t *ed)
{
	prvm_prog_t *prog = CLVM_prog;
	int b;
	if(!PRVM_clientedictfunction(ed, predraw))
		return;
	b = PRVM_clientglobaledict(self);
	PRVM_clientglobaledict(self) = PRVM_EDICT_TO_PROG(ed);
	prog->ExecuteProgram(prog, PRVM_clientedictfunction(ed, predraw), "CSQC_Predraw: NULL function\n");
	PRVM_clientglobaledict(self) = b;
}

void CSQC_Think (prvm_edict_t *ed)
{
	prvm_prog_t *prog = CLVM_prog;
	int b;
	if(PRVM_clientedictfunction(ed, think))
	if(PRVM_clientedictfloat(ed, nextthink) && PRVM_clientedictfloat(ed, nextthink) <= PRVM_clientglobalfloat(time))
	{
		PRVM_clientedictfloat(ed, nextthink) = 0;
		b = PRVM_clientglobaledict(self);
		PRVM_clientglobaledict(self) = PRVM_EDICT_TO_PROG(ed);
		prog->ExecuteProgram(prog, PRVM_clientedictfunction(ed, think), "CSQC_Think: NULL function\n");
		PRVM_clientglobaledict(self) = b;
	}
}

extern cvar_t cl_noplayershadow;
qbool CSQC_AddRenderEdict(prvm_edict_t *ed, int edictnum)
{
	prvm_prog_t *prog = CLVM_prog;
	int renderflags;
	int c;
	float scale;
	entity_render_t *entrender;
	model_t *model;
	prvm_vec3_t modellight_origin;

	model = CL_GetModelFromEdict(ed);
	if (!model)
		return false;

	if (edictnum)
	{
		if (r_refdef.scene.numentities >= r_refdef.scene.maxentities)
			return false;
		entrender = cl.csqcrenderentities + edictnum;
		r_refdef.scene.entities[r_refdef.scene.numentities++] = entrender;
		entrender->entitynumber = edictnum + MAX_EDICTS;
		//entrender->shadertime = 0; // shadertime was set by spawn()
		entrender->flags = 0;
		entrender->effects = 0;
		entrender->alpha = 1;
		entrender->scale = 1;
		VectorSet(entrender->colormod, 1, 1, 1);
		VectorSet(entrender->glowmod, 1, 1, 1);
		entrender->allowdecals = true;
	}
	else
	{
		entrender = CL_NewTempEntity(0);
		if (!entrender)
			return false;
	}

	entrender->userwavefunc_param[0] = PRVM_clientedictfloat(ed, userwavefunc_param0);
	entrender->userwavefunc_param[1] = PRVM_clientedictfloat(ed, userwavefunc_param1);
	entrender->userwavefunc_param[2] = PRVM_clientedictfloat(ed, userwavefunc_param2);
	entrender->userwavefunc_param[3] = PRVM_clientedictfloat(ed, userwavefunc_param3);

	entrender->model = model;
	entrender->skinnum = (int)PRVM_clientedictfloat(ed, skin);
	entrender->effects |= entrender->model->effects;
	renderflags = (int)PRVM_clientedictfloat(ed, renderflags);
	entrender->alpha = PRVM_clientedictfloat(ed, alpha);
	entrender->scale = scale = PRVM_clientedictfloat(ed, scale);
	VectorCopy(PRVM_clientedictvector(ed, colormod), entrender->colormod);
	VectorCopy(PRVM_clientedictvector(ed, glowmod), entrender->glowmod);
	if(PRVM_clientedictfloat(ed, effects))	entrender->effects |= (int)PRVM_clientedictfloat(ed, effects);
	if (!entrender->alpha)
		entrender->alpha = 1.0f;
	if (!entrender->scale)
		entrender->scale = scale = 1.0f;
	if (!VectorLength2(entrender->colormod))
		VectorSet(entrender->colormod, 1, 1, 1);
	if (!VectorLength2(entrender->glowmod))
		VectorSet(entrender->glowmod, 1, 1, 1);

	// LadyHavoc: use the CL_GetTagMatrix function on self to ensure consistent behavior (duplicate code would be bad)
	// this also sets the custommodellight_origin for us
	CL_GetTagMatrix(prog, &entrender->matrix, ed, 0, modellight_origin);
	VectorCopy(modellight_origin, entrender->custommodellight_origin);

	// set up the animation data
	VM_GenerateFrameGroupBlend(prog, ed->priv.server->framegroupblend, ed);
	VM_FrameBlendFromFrameGroupBlend(ed->priv.server->frameblend, ed->priv.server->framegroupblend, model, cl.time);
	VM_UpdateEdictSkeleton(prog, ed, model, ed->priv.server->frameblend);
	if (PRVM_clientedictfloat(ed, shadertime)) // hack for csprogs.dat files that do not set shadertime, leaves the value at entity spawn time
		entrender->shadertime = PRVM_clientedictfloat(ed, shadertime);

	// transparent offset
	if (renderflags & RF_USETRANSPARENTOFFSET)
		entrender->transparent_offset = PRVM_clientglobalfloat(transparent_offset);

	// model light
	if (renderflags & RF_MODELLIGHT)
	{
		if (PRVM_clientedictvector(ed, modellight_ambient)) VectorCopy(PRVM_clientedictvector(ed, modellight_ambient), entrender->custommodellight_ambient); else VectorClear(entrender->custommodellight_ambient);
		if (PRVM_clientedictvector(ed, modellight_diffuse)) VectorCopy(PRVM_clientedictvector(ed, modellight_diffuse), entrender->custommodellight_diffuse); else VectorClear(entrender->custommodellight_diffuse);
		if (PRVM_clientedictvector(ed, modellight_dir))     VectorCopy(PRVM_clientedictvector(ed, modellight_dir), entrender->custommodellight_lightdir);    else VectorClear(entrender->custommodellight_lightdir);
		entrender->flags |= RENDER_CUSTOMIZEDMODELLIGHT;
	}

	if(renderflags)
	{
		if(renderflags & RF_VIEWMODEL) entrender->flags |= RENDER_VIEWMODEL | RENDER_NODEPTHTEST;
		if(renderflags & RF_EXTERNALMODEL) entrender->flags |= RENDER_EXTERIORMODEL;
		if(renderflags & RF_WORLDOBJECT) entrender->flags |= RENDER_WORLDOBJECT;
		if(renderflags & RF_DEPTHHACK) entrender->flags |= RENDER_NODEPTHTEST;
		if(renderflags & RF_ADDITIVE) entrender->flags |= RENDER_ADDITIVE;
		if(renderflags & RF_DYNAMICMODELLIGHT) entrender->flags |= RENDER_DYNAMICMODELLIGHT;
	}

	c = (int)PRVM_clientedictfloat(ed, colormap);
	if (c <= 0)
		CL_SetEntityColormapColors(entrender, -1);
	else if (c <= cl.maxclients && cl.scores != NULL)
		CL_SetEntityColormapColors(entrender, cl.scores[c-1].colors);
	else
		CL_SetEntityColormapColors(entrender, c);

	entrender->flags &= ~(RENDER_SHADOW | RENDER_LIGHT | RENDER_NOSELFSHADOW);
	// either fullbright or lit
	if(!r_fullbright.integer)
	{
		if (!(entrender->effects & EF_FULLBRIGHT) && !(renderflags & RF_FULLBRIGHT))
			entrender->flags |= RENDER_LIGHT;
	}
	// hide player shadow during intermission or nehahra movie
	if (!(entrender->effects & (EF_NOSHADOW | EF_ADDITIVE | EF_NODEPTHTEST))
	 &&  (entrender->alpha >= 1)
	 && !(renderflags & RF_NOSHADOW)
	 && !(entrender->flags & RENDER_VIEWMODEL)
	 && (!(entrender->flags & RENDER_EXTERIORMODEL) || (!cl.intermission && cls.protocol != PROTOCOL_NEHAHRAMOVIE && !cl_noplayershadow.integer)))
		entrender->flags |= RENDER_SHADOW;
	if (entrender->flags & RENDER_VIEWMODEL)
		entrender->flags |= RENDER_NOSELFSHADOW;
	if (entrender->effects & EF_NOSELFSHADOW)
		entrender->flags |= RENDER_NOSELFSHADOW;
	if (entrender->effects & EF_NODEPTHTEST)
		entrender->flags |= RENDER_NODEPTHTEST;
	if (entrender->effects & EF_ADDITIVE)
		entrender->flags |= RENDER_ADDITIVE;
	if (entrender->effects & EF_DOUBLESIDED)
		entrender->flags |= RENDER_DOUBLESIDED;
	if (entrender->effects & EF_DYNAMICMODELLIGHT)
		entrender->flags |= RENDER_DYNAMICMODELLIGHT;

	// make the other useful stuff
	memcpy(entrender->framegroupblend, ed->priv.server->framegroupblend, sizeof(ed->priv.server->framegroupblend));
	CL_UpdateRenderEntity(entrender);

	// override animation data with full control
	memcpy(entrender->frameblend, ed->priv.server->frameblend, sizeof(ed->priv.server->frameblend));
	if (ed->priv.server->skeleton.relativetransforms)
		entrender->skeleton = &ed->priv.server->skeleton;
	else
		entrender->skeleton = NULL;

	return true;
}

// 0 = keydown, key, character (EXT_CSQC)
// 1 = keyup, key, character (EXT_CSQC)
// 2 = mousemove relative, x, y (EXT_CSQC)
// 3 = mousemove absolute, x, y (DP_CSQC)
qbool CL_VM_InputEvent (int eventtype, float x, float y)
{
	prvm_prog_t *prog = CLVM_prog;
	qbool r;

	if(!prog->loaded)
		return false;

	if (!PRVM_clientfunction(CSQC_InputEvent))
		r = false;
	else
	{
		PRVM_clientglobalfloat(time) = cl.time;
		PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
		PRVM_G_FLOAT(OFS_PARM0) = eventtype;
		PRVM_G_FLOAT(OFS_PARM1) = x; // key or x
		PRVM_G_FLOAT(OFS_PARM2) = y; // ascii or y
		prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_InputEvent), "QC function CSQC_InputEvent is missing");
		r = CSQC_RETURNVAL != 0;
	}
	return r;
}

extern r_refdef_view_t csqc_original_r_refdef_view;
extern r_refdef_view_t csqc_main_r_refdef_view;
qbool CL_VM_UpdateView (double frametime)
{
	prvm_prog_t *prog = CLVM_prog;
	vec3_t emptyvector;
	emptyvector[0] = 0;
	emptyvector[1] = 0;
	emptyvector[2] = 0;
//	vec3_t oldangles;

	if(!prog->loaded)
		return false;

	R_TimeReport("pre-UpdateView");

	csqc_original_r_refdef_view = r_refdef.view;
	csqc_main_r_refdef_view = r_refdef.view;
	//VectorCopy(cl.viewangles, oldangles);
	PRVM_clientglobalfloat(time) = cl.time;
	PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
	CSQC_SetGlobals(frametime);
	// clear renderable entity and light lists to prevent crashes if the
	// CSQC_UpdateView function does not call R_ClearScene as it should
	r_refdef.scene.numentities = 0;
	r_refdef.scene.numlights = 0;
	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = false;
	// free memory for resources that are no longer referenced
	PRVM_GarbageCollection(prog);
	// pass in width and height and menu/focus state as parameters (EXT_CSQC_1)
	PRVM_G_FLOAT(OFS_PARM0) = vid.mode.width;
	PRVM_G_FLOAT(OFS_PARM1) = vid.mode.height;
	/*
	 * This should be fine for now but FTEQW uses flags for keydest
	 * and checks that an array called "eyeoffset" is 0
	 *
	 * Just a note in case there's compatibility problems later
	 */
	PRVM_G_FLOAT(OFS_PARM2) = key_dest == key_game;
	prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_UpdateView), "QC function CSQC_UpdateView is missing");
	//VectorCopy(oldangles, cl.viewangles);
	// Dresk : Reset Dmg Globals Here
	CL_VM_UpdateDmgGlobals(0, 0, emptyvector);
	r_refdef.view = csqc_main_r_refdef_view;
	R_RenderView_UpdateViewVectors(); // we have to do this, as we undid the scene render doing this for us

	R_TimeReport("UpdateView");
	return true;
}

void CL_VM_DrawHud(double frametime)
{
	prvm_prog_t *prog = CLVM_prog;

	R_TimeReport("pre-DrawHud");

	PRVM_clientglobalfloat(time) = cl.time;
	PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
	CSQC_SetGlobals(frametime);

	PRVM_GarbageCollection(prog);

	// width and height parameters are virtual in CSQC_SIMPLE engines
	VectorSet(PRVM_G_VECTOR(OFS_PARM0), vid_conwidth.integer, vid_conheight.integer, 0);
	PRVM_G_FLOAT(OFS_PARM1) = sb_showscores;
	prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_DrawHud), "QC function CSQC_DrawHud is missing");

	if (PRVM_clientfunction(CSQC_DrawScores))
	{
		VectorSet(PRVM_G_VECTOR(OFS_PARM0), vid_conwidth.integer, vid_conheight.integer, 0);
		PRVM_G_FLOAT(OFS_PARM1) = sb_showscores;
		if (key_dest != key_menu)
			prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_DrawScores), "QC function CSQC_DrawScores is missing");
	}
	else if (sb_showscores || (cl.stats[STAT_HEALTH] <= 0 && cl_deathscoreboard.integer))
		if (!cl.islocalgame) // LadyHavoc: changed to draw the deathmatch overlays in any multiplayer mode
			Sbar_DeathmatchOverlay ();

	R_TimeReport("DrawHud");
}


qbool CL_VM_ConsoleCommand(const char *text, size_t textlen)
{
	prvm_prog_t *prog = CLVM_prog;
	return PRVM_ConsoleCommand(prog, text, textlen, &prog->funcoffsets.CSQC_ConsoleCommand, false, cl.csqc_server2csqcentitynumber[cl.playerentity], cl.time, "QC function CSQC_ConsoleCommand is missing");
}

qbool CL_VM_Parse_TempEntity (void)
{
	prvm_prog_t *prog = CLVM_prog;
	int t;
	qbool r = false;

	if(!prog->loaded)
		return false;

	if(PRVM_clientfunction(CSQC_Parse_TempEntity))
	{
		t = cl_message.readcount;
		PRVM_clientglobalfloat(time) = cl.time;
		PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
		prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Parse_TempEntity), "QC function CSQC_Parse_TempEntity is missing");
		r = CSQC_RETURNVAL != 0;
		if(!r)
		{
			cl_message.readcount = t;
			cl_message.badread = false;
		}
	}
	return r;
}

void CL_VM_Parse_StuffCmd(const char *msg, size_t msg_len)
{
	prvm_prog_t *prog = CLVM_prog;
	int restorevm_tempstringsbuf_cursize;

	if(msg[0] == 'c')
	if(msg[1] == 's')
	if(msg[2] == 'q')
	if(msg[3] == 'c')
	{
		// if this is setting a csqc variable, deprotect csqc_progcrc
		// temporarily so that it can be set by the cvar command,
		// and then reprotect it afterwards
		int crcflags = csqc_progcrc.flags;
		csqc_progcrc.flags &= ~CF_READONLY;
		csqc_progsize.flags &= ~CF_READONLY;
		Cmd_ExecuteString(cmd_local, msg, msg_len, src_local, true);
		csqc_progcrc.flags = csqc_progsize.flags = crcflags;
		return;
	}

	if(!prog->loaded)
	{
		Cbuf_AddText(cmd_local, msg);
		return;
	}

	if(PRVM_clientfunction(CSQC_Parse_StuffCmd))
	{
		PRVM_clientglobalfloat(time) = cl.time;
		PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
		restorevm_tempstringsbuf_cursize = prog->tempstringsbuf.cursize;
		PRVM_G_INT(OFS_PARM0) = PRVM_SetTempString(prog, msg, msg_len);
		prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Parse_StuffCmd), "QC function CSQC_Parse_StuffCmd is missing");
		prog->tempstringsbuf.cursize = restorevm_tempstringsbuf_cursize;
	}
	else
		Cbuf_AddText(cmd_local, msg);
}

static void CL_VM_Parse_Print(const char *msg, size_t msg_len)
{
	prvm_prog_t *prog = CLVM_prog;
	int restorevm_tempstringsbuf_cursize;
	PRVM_clientglobalfloat(time) = cl.time;
	PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
	restorevm_tempstringsbuf_cursize = prog->tempstringsbuf.cursize;
	PRVM_G_INT(OFS_PARM0) = PRVM_SetTempString(prog, msg, msg_len);
	prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Parse_Print), "QC function CSQC_Parse_Print is missing");
	prog->tempstringsbuf.cursize = restorevm_tempstringsbuf_cursize;
}

void CSQC_AddPrintText(const char *msg, size_t msg_len)
{
	prvm_prog_t *prog = CLVM_prog;
	char *start = cl.csqc_printtextbuf + cl.csqc_printtextbuf_len;
	size_t writebytes = min(msg_len + 1, MAX_INPUTLINE - cl.csqc_printtextbuf_len);

	if(prog->loaded && PRVM_clientfunction(CSQC_Parse_Print))
	{
		if(msg[msg_len - 1] != '\n' && msg[msg_len - 1] != '\r')
		{
			if(cl.csqc_printtextbuf_len + msg_len + 1 >= MAX_INPUTLINE)
			{
				CL_VM_Parse_Print(cl.csqc_printtextbuf, cl.csqc_printtextbuf_len);
				cl.csqc_printtextbuf[0] = '\0';
				cl.csqc_printtextbuf_len = 0;
			}
			else
			{
				memcpy(start, msg, writebytes);
				cl.csqc_printtextbuf_len += msg_len;
			}
			return;
		}
		memcpy(start, msg, writebytes);
		cl.csqc_printtextbuf_len += msg_len;
		CL_VM_Parse_Print(cl.csqc_printtextbuf, cl.csqc_printtextbuf_len);
		cl.csqc_printtextbuf[0] = '\0';
		cl.csqc_printtextbuf_len = 0;
	}
	else
		Con_Print(msg);
}

void CL_VM_Parse_CenterPrint(const char *msg, size_t msg_len)
{
	prvm_prog_t *prog = CLVM_prog;
	int restorevm_tempstringsbuf_cursize;

	if(prog->loaded && PRVM_clientfunction(CSQC_Parse_CenterPrint))
	{
		PRVM_clientglobalfloat(time) = cl.time;
		PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
		restorevm_tempstringsbuf_cursize = prog->tempstringsbuf.cursize;
		PRVM_G_INT(OFS_PARM0) = PRVM_SetTempString(prog, msg, msg_len);
		prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Parse_CenterPrint), "QC function CSQC_Parse_CenterPrint is missing");
		prog->tempstringsbuf.cursize = restorevm_tempstringsbuf_cursize;
	}
	else
		SCR_CenterPrint(msg);
}

void CL_VM_UpdateIntermissionState (int intermission)
{
	prvm_prog_t *prog = CLVM_prog;

	if(prog->loaded)
		PRVM_clientglobalfloat(intermission) = intermission;
}
void CL_VM_UpdateShowingScoresState (int showingscores)
{
	prvm_prog_t *prog = CLVM_prog;

	if(prog->loaded)
		PRVM_clientglobalfloat(sb_showscores) = showingscores;
}
qbool CL_VM_Event_Sound(int sound_num, float fvolume, int channel, float attenuation, int ent, vec3_t pos, int flags, float speed)
{
	prvm_prog_t *prog = CLVM_prog;
	qbool r = false;

	if(prog->loaded)
	{
		if(PRVM_clientfunction(CSQC_Event_Sound))
		{
			PRVM_clientglobalfloat(time) = cl.time;
			PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
			PRVM_G_FLOAT(OFS_PARM0) = ent;
			PRVM_G_FLOAT(OFS_PARM1) = CHAN_ENGINE2USER(channel);
			PRVM_G_INT(OFS_PARM2) = PRVM_SetTempString(prog, cl.sound_name[sound_num], strlen(cl.sound_name[sound_num]));
			PRVM_G_FLOAT(OFS_PARM3) = fvolume;
			PRVM_G_FLOAT(OFS_PARM4) = attenuation;
			VectorCopy(pos, PRVM_G_VECTOR(OFS_PARM5) );
			PRVM_G_FLOAT(OFS_PARM6) = speed * 100.0f;
			PRVM_G_FLOAT(OFS_PARM7) = flags; // flags
			prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Event_Sound), "QC function CSQC_Event_Sound is missing");
			r = CSQC_RETURNVAL != 0;
		}
	}

	return r;
}
static void CL_VM_UpdateCoopDeathmatchGlobals (int gametype)
{
	prvm_prog_t *prog = CLVM_prog;
	// Avoid global names for clean(er) coding
	int localcoop;
	int localdeathmatch;

	if(prog->loaded)
	{
		if(gametype == GAME_COOP)
		{
			localcoop = 1;
			localdeathmatch = 0;
		}
		else if(gametype == GAME_DEATHMATCH)
		{
			localcoop = 0;
			localdeathmatch = 1;
		}
		else
		{
			// How did the ServerInfo send an unknown gametype?
			// Better just assign the globals as 0...
			localcoop = 0;
			localdeathmatch = 0;
		}
		PRVM_clientglobalfloat(coop) = localcoop;
		PRVM_clientglobalfloat(deathmatch) = localdeathmatch;
	}
}
#if 0
static float CL_VM_Event (float event)		//[515]: needed ? I'd say "YES", but don't know for what :D
{
	prvm_prog_t *prog = CLVM_prog;
	float r = 0;

	if(!prog->loaded)
		return 0;

	if(PRVM_clientfunction(CSQC_Event))
	{
		PRVM_clientglobalfloat(time) = cl.time;
		PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[cl.playerentity];
		PRVM_G_FLOAT(OFS_PARM0) = event;
		prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Event), "QC function CSQC_Event is missing");
		r = CSQC_RETURNVAL;
	}
	return r;
}
#endif

void CSQC_ReadEntities (void)
{
	prvm_prog_t *prog = CLVM_prog;
	unsigned short entnum, oldself, realentnum;

	if(!prog->loaded)
	{
		Host_Error ("CSQC_ReadEntities: CSQC is not loaded");
		return;
	}

	PRVM_clientglobalfloat(time) = cl.time;
	oldself = PRVM_clientglobaledict(self);
	while(1)
	{
		entnum = MSG_ReadShort(&cl_message);
		if(!entnum || cl_message.badread)
			break;
		realentnum = entnum & 0x7FFF;
		PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[realentnum];
		if(entnum & 0x8000)
		{
			if(PRVM_clientglobaledict(self))
			{
				prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Ent_Remove), "QC function CSQC_Ent_Remove is missing");
				cl.csqc_server2csqcentitynumber[realentnum] = 0;
			}
			else
			{
				// LadyHavoc: removing an entity that is already gone on
				// the csqc side is possible for legitimate reasons (such
				// as a repeat of the remove message), so no warning is
				// needed
				//Con_Printf("Bad csqc_server2csqcentitynumber map\n");	//[515]: never happens ?
			}
		}
		else
		{
			if(!PRVM_clientglobaledict(self))
			{
				if(!PRVM_clientfunction(CSQC_Ent_Spawn))
				{
					prvm_edict_t	*ed;
					ed = PRVM_ED_Alloc(prog);
					PRVM_clientedictfloat(ed, entnum) = realentnum;
					PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[realentnum] = PRVM_EDICT_TO_PROG(ed);
				}
				else
				{
					// entity( float entnum ) CSQC_Ent_Spawn;
					// the qc function should set entnum, too (this way it also can return world [2/1/2008 Andreas]
					PRVM_G_FLOAT(OFS_PARM0) = (float) realentnum;
					// make sure no one gets wrong ideas
					PRVM_clientglobaledict(self) = 0;
					prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Ent_Spawn), "QC function CSQC_Ent_Spawn is missing");
					PRVM_clientglobaledict(self) = cl.csqc_server2csqcentitynumber[realentnum] = PRVM_EDICT( PRVM_G_INT( OFS_RETURN ) );
				}
				PRVM_G_FLOAT(OFS_PARM0) = 1;
				prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Ent_Update), "QC function CSQC_Ent_Update is missing");
			}
			else {
				PRVM_G_FLOAT(OFS_PARM0) = 0;
				prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Ent_Update), "QC function CSQC_Ent_Update is missing");
			}
		}
	}
	PRVM_clientglobaledict(self) = oldself;
}

static void CLVM_begin_increase_edicts(prvm_prog_t *prog)
{
	// links don't survive the transition, so unlink everything
	World_UnlinkAll(&cl.world);
}

static void CLVM_end_increase_edicts(prvm_prog_t *prog)
{
	int i;
	prvm_edict_t *ent;

	// link every entity except world
	for (i = 1, ent = prog->edicts;i < prog->num_edicts;i++, ent++)
		if (!ent->free)
			CL_LinkEdict(ent);
}

static void CLVM_init_edict(prvm_prog_t *prog, prvm_edict_t *e)
{
	int edictnum = PRVM_NUM_FOR_EDICT(e);
	entity_render_t *entrender;
	CL_ExpandCSQCRenderEntities(edictnum);
	entrender = cl.csqcrenderentities + edictnum;
	e->priv.server->move = false; // don't move on first frame
	memset(entrender, 0, sizeof(*entrender));
	entrender->shadertime = cl.time;
}

static void CLVM_free_edict(prvm_prog_t *prog, prvm_edict_t *ed)
{
	entity_render_t *entrender = cl.csqcrenderentities + PRVM_NUM_FOR_EDICT(ed);
	R_DecalSystem_Reset(&entrender->decalsystem);
	memset(entrender, 0, sizeof(*entrender));
	World_UnlinkEdict(ed);
	memset(ed->fields.fp, 0, prog->entityfields * sizeof(prvm_vec_t));
	VM_RemoveEdictSkeleton(prog, ed);
#ifdef USEODE
	World_Physics_RemoveFromEntity(&cl.world, ed);
	World_Physics_RemoveJointFromEntity(&cl.world, ed);
#endif
}

static void CLVM_count_edicts(prvm_prog_t *prog)
{
	int		i;
	prvm_edict_t	*ent;
	int		active = 0, models = 0, solid = 0;

	for (i=0 ; i<prog->num_edicts ; i++)
	{
		ent = PRVM_EDICT_NUM(i);
		if (ent->free)
			continue;
		active++;
		if (PRVM_clientedictfloat(ent, solid))
			solid++;
		if (PRVM_clientedictstring(ent, model))
			models++;
	}

	Con_Printf("num_edicts:%3i\n", prog->num_edicts);
	Con_Printf("active    :%3i\n", active);
	Con_Printf("view      :%3i\n", models);
	Con_Printf("touch     :%3i\n", solid);
}

static qbool CLVM_load_edict(prvm_prog_t *prog, prvm_edict_t *ent)
{
	return true;
}

// returns true if the packet is valid, false if end of file is reached
// used for dumping the CSQC download into demo files
qbool MakeDownloadPacket(const char *filename, unsigned char *data, size_t len, int crc, int cnt, sizebuf_t *buf, int protocol)
{
	int packetsize = buf->maxsize - 7; // byte short long
	int npackets = ((int)len + packetsize - 1) / (packetsize);
	char vabuf[1024];

	if(protocol == PROTOCOL_QUAKEWORLD)
		return false; // CSQC can't run in QW anyway

	SZ_Clear(buf);
	if(cnt == 0)
	{
		MSG_WriteByte(buf, svc_stufftext);
		MSG_WriteString(buf, va(vabuf, sizeof(vabuf), "\ncl_downloadbegin %lu %s\n", (unsigned long)len, filename));
		return true;
	}
	else if(cnt >= 1 && cnt <= npackets)
	{
		unsigned long thispacketoffset = (cnt - 1) * packetsize;
		int thispacketsize = (int)len - thispacketoffset;
		if(thispacketsize > packetsize)
			thispacketsize = packetsize;

		MSG_WriteByte(buf, svc_downloaddata);
		MSG_WriteLong(buf, thispacketoffset);
		MSG_WriteShort(buf, thispacketsize);
		SZ_Write(buf, data + thispacketoffset, thispacketsize);

		return true;
	}
	else if(cnt == npackets + 1)
	{
		MSG_WriteByte(buf, svc_stufftext);
		MSG_WriteString(buf, va(vabuf, sizeof(vabuf), "\ncl_downloadfinished %lu %d\n", (unsigned long)len, crc));
		return true;
	}
	return false;
}

extern cvar_t csqc_usedemoprogs;
void CL_VM_Init (void)
{
	prvm_prog_t *prog = CLVM_prog;
	const char* csprogsfn = NULL;
	unsigned char *csprogsdata = NULL;
	fs_offset_t csprogsdatasize = 0;
	int csprogsdatacrc, requiredcrc;
	int requiredsize;
	char vabuf[1024];

	// reset csqc_progcrc after reading it, so that changing servers doesn't
	// expect csqc on the next server
	requiredcrc = csqc_progcrc.integer;
	requiredsize = csqc_progsize.integer;
	Cvar_SetValueQuick(&csqc_progcrc, -1);
	Cvar_SetValueQuick(&csqc_progsize, -1);

	// if the server is not requesting a csprogs, then we're done here
	if (requiredcrc < 0)
		return;

	// see if the requested csprogs.dat file matches the requested crc
	if (!cls.demoplayback || csqc_usedemoprogs.integer)
	{
		csprogsfn = va(vabuf, sizeof(vabuf), "dlcache/%s.%i.%i", csqc_progname.string, requiredsize, requiredcrc);
		if(cls.caughtcsprogsdata && cls.caughtcsprogsdatasize == requiredsize && CRC_Block(cls.caughtcsprogsdata, (size_t)cls.caughtcsprogsdatasize) == requiredcrc)
		{
			Con_DPrintf("Using buffered \"%s\"\n", csprogsfn);
			csprogsdata = cls.caughtcsprogsdata;
			csprogsdatasize = cls.caughtcsprogsdatasize;
			cls.caughtcsprogsdata = NULL;
			cls.caughtcsprogsdatasize = 0;
		}
		else
		{
			Con_DPrintf("Not using buffered \"%s\" (buffered: %p, %d)\n", csprogsfn, (void *)cls.caughtcsprogsdata, (int) cls.caughtcsprogsdatasize);
			csprogsdata = FS_LoadFile(csprogsfn, tempmempool, true, &csprogsdatasize);
		}
	}
	if (!csprogsdata)
	{
		csprogsfn = csqc_progname.string;
		csprogsdata = FS_LoadFile(csprogsfn, tempmempool, true, &csprogsdatasize);
	}
	if (csprogsdata)
	{
		csprogsdatacrc = CRC_Block(csprogsdata, (size_t)csprogsdatasize);
		if (csprogsdatacrc != requiredcrc || csprogsdatasize != requiredsize)
		{
			if (cls.demoplayback)
			{
				Con_Printf(CON_WARN "Warning: Your %s is not the same version as the demo was recorded with (CRC/size are %i/%i but should be %i/%i)\n", csqc_progname.string, csprogsdatacrc, (int)csprogsdatasize, requiredcrc, requiredsize);
				// Mem_Free(csprogsdata);
				// return;
				// We WANT to continue here, and play the demo with different csprogs!
				// After all, this is just a warning. Sure things may go wrong from here.
			}
			else
			{
				Mem_Free(csprogsdata);
				CL_DisconnectEx(false, "Your %s is not the same version as the server (CRC is %i/%i but should be %i/%i)\n", csqc_progname.string, csprogsdatacrc, (int)csprogsdatasize, requiredcrc, requiredsize);
				return;
			}
		}
	}
	else
	{
		if (requiredcrc >= 0)
			CL_DisconnectEx(false, CON_ERROR "CL_VM_Init: %s requires CSQC, but \"%s\" wasn't found\n", cls.demoplayback ? "demo" : "server", csqc_progname.string);
		return;
	}

	PRVM_Prog_Init(prog, cmd_local);

	// allocate the mempools
	prog->progs_mempool = Mem_AllocPool(csqc_progname.string, 0, NULL);
	prog->edictprivate_size = 0; // no private struct used
	prog->name = "client";
	prog->num_edicts = 1;
	prog->max_edicts = 512;
	prog->limit_edicts = CL_MAX_EDICTS;
	prog->reserved_edicts = 0;
	prog->edictprivate_size = sizeof(edict_engineprivate_t);
	// TODO: add a shared extension string #define and add real support for csqc extension strings [12/5/2007 Black]
	prog->extensionstring = vm_sv_extensions;
	prog->builtins = vm_cl_builtins;
	prog->numbuiltins = vm_cl_numbuiltins;

	// all callbacks must be defined (pointers are not checked before calling)
	prog->begin_increase_edicts = CLVM_begin_increase_edicts;
	prog->end_increase_edicts   = CLVM_end_increase_edicts;
	prog->init_edict            = CLVM_init_edict;
	prog->free_edict            = CLVM_free_edict;
	prog->count_edicts          = CLVM_count_edicts;
	prog->load_edict            = CLVM_load_edict;
	prog->init_cmd              = CLVM_init_cmd;
	prog->reset_cmd             = CLVM_reset_cmd;
	prog->error_cmd             = Host_Error;
	prog->ExecuteProgram        = CLVM_ExecuteProgram;

	PRVM_Prog_Load(prog, csprogsfn, csprogsdata, csprogsdatasize, CL_CheckRequiredFuncs, CL_REQFIELDS, cl_reqfields, CL_REQGLOBALS, cl_reqglobals);

	if (!prog->loaded)
	{
		Mem_Free(csprogsdata);
		Host_Error("CSQC %s failed to load\n", csprogsfn);
	}

	if(cls.demorecording)
	{
		if(cls.demo_lastcsprogssize != csprogsdatasize || cls.demo_lastcsprogscrc != csprogsdatacrc)
		{
			int i;
			static char buf[NET_MAXMESSAGE];
			sizebuf_t sb;
			unsigned char *demobuf; fs_offset_t demofilesize;

			sb.data = (unsigned char *) buf;
			sb.maxsize = sizeof(buf);
			i = 0;

			CL_CutDemo(&demobuf, &demofilesize);
			while(MakeDownloadPacket(csqc_progname.string, csprogsdata, (size_t)csprogsdatasize, csprogsdatacrc, i++, &sb, cls.protocol))
				CL_WriteDemoMessage(&sb);
			CL_PasteDemo(&demobuf, &demofilesize);

			cls.demo_lastcsprogssize = csprogsdatasize;
			cls.demo_lastcsprogscrc = csprogsdatacrc;
		}
	}
	Mem_Free(csprogsdata);

	// check if OP_STATE animation is possible in this dat file
	if (prog->fieldoffsets.nextthink >= 0 && prog->fieldoffsets.frame >= 0 && prog->fieldoffsets.think >= 0 && prog->globaloffsets.self >= 0)
		prog->flag |= PRVM_OP_STATE;

	// set time
	PRVM_clientglobalfloat(time) = cl.time;
	PRVM_clientglobaledict(self) = 0;

	PRVM_clientglobalstring(mapname) = PRVM_SetEngineString(prog, cl.worldbasename);
	PRVM_clientglobalfloat(player_localnum) = cl.realplayerentity - 1;
	PRVM_clientglobalfloat(player_localentnum) = cl.viewentity;

	// set map description (use world entity 0)
	PRVM_clientedictstring(prog->edicts, message) = PRVM_SetEngineString(prog, cl.worldmessage);
	VectorCopy(cl.world.mins, PRVM_clientedictvector(prog->edicts, mins));
	VectorCopy(cl.world.maxs, PRVM_clientedictvector(prog->edicts, maxs));
	VectorCopy(cl.world.mins, PRVM_clientedictvector(prog->edicts, absmin));
	VectorCopy(cl.world.maxs, PRVM_clientedictvector(prog->edicts, absmax));
	PRVM_clientedictfloat(prog->edicts, solid) = SOLID_BSP;
	PRVM_clientedictfloat(prog->edicts, modelindex) = 1;
	PRVM_clientedictstring(prog->edicts, model) = PRVM_SetEngineString(prog, cl.worldmodel->name);

	// call the prog init if it exists
	if (PRVM_clientfunction(CSQC_Init))
	{
		PRVM_G_FLOAT(OFS_PARM0) = 1.0f; // CSQC_SIMPLE engines always pass 0, FTE always passes 1
		PRVM_G_INT(OFS_PARM1) = PRVM_SetEngineString(prog, gamename);
		PRVM_G_FLOAT(OFS_PARM2) = 1.0f; // TODO DP versions...
		prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Init), "QC function CSQC_Init is missing");
	}

	// Once CSQC_Init was called, we consider csqc code fully initialized.
	prog->inittime = host.realtime;

	cl.csqc_vidvars.drawcrosshair = false;
	cl.csqc_vidvars.drawenginesbar = false;

	// Update Coop and Deathmatch Globals (at this point the client knows them from ServerInfo)
	CL_VM_UpdateCoopDeathmatchGlobals(cl.gametype);
}

void CL_VM_ShutDown (void)
{
	prvm_prog_t *prog = CLVM_prog;
	Cmd_ClearCSQCCommands(cmd_local);

	//Cvar_SetValueQuick(&csqc_progcrc, -1);
	//Cvar_SetValueQuick(&csqc_progsize, -1);
	if (prog->loaded)
	{
		PRVM_clientglobalfloat(time) = cl.time;
		PRVM_clientglobaledict(self) = 0;
		if (PRVM_clientfunction(CSQC_Shutdown))
			prog->ExecuteProgram(prog, PRVM_clientfunction(CSQC_Shutdown), "QC function CSQC_Shutdown is missing");
	}
	PRVM_Prog_Reset(prog);
	Con_DPrint("CSQC ^1unloaded\n");
}

qbool CL_VM_GetEntitySoundOrigin(int entnum, vec3_t out)
{
	prvm_prog_t *prog = CLVM_prog;
	prvm_edict_t *ed;
	model_t *mod;
	matrix4x4_t matrix;
	qbool r = 0;

	ed = PRVM_EDICT_NUM(entnum - MAX_EDICTS);

	if(!ed->free)
	{
		mod = CL_GetModelFromEdict(ed);
		VectorCopy(PRVM_clientedictvector(ed, origin), out);
		if(CL_GetTagMatrix(prog, &matrix, ed, 0, NULL) == 0)
			Matrix4x4_OriginFromMatrix(&matrix, out);
		if (mod && mod->soundfromcenter)
			VectorMAMAM(1.0f, out, 0.5f, mod->normalmins, 0.5f, mod->normalmaxs, out);
		r = 1;
	}

	return r;
}

qbool CL_VM_TransformView(int entnum, matrix4x4_t *viewmatrix, mplane_t *clipplane, vec3_t visorigin)
{
	prvm_prog_t *prog = CLVM_prog;
	qbool ret = false;
	prvm_edict_t *ed;
	vec3_t forward, left, up, origin, ang;
	matrix4x4_t mat, matq;

	ed = PRVM_EDICT_NUM(entnum);
	// camera:
	//   camera_transform
	if(PRVM_clientedictfunction(ed, camera_transform))
	{
		ret = true;
		if(viewmatrix && clipplane && visorigin)
		{
			Matrix4x4_ToVectors(viewmatrix, forward, left, up, origin);
			AnglesFromVectors(ang, forward, up, false);
			PRVM_clientglobalfloat(time) = cl.time;
			PRVM_clientglobaledict(self) = entnum;
			VectorCopy(origin, PRVM_G_VECTOR(OFS_PARM0));
			VectorCopy(ang, PRVM_G_VECTOR(OFS_PARM1));
			VectorCopy(forward, PRVM_clientglobalvector(v_forward));
			VectorScale(left, -1, PRVM_clientglobalvector(v_right));
			VectorCopy(up, PRVM_clientglobalvector(v_up));
			VectorCopy(origin, PRVM_clientglobalvector(trace_endpos));
			prog->ExecuteProgram(prog, PRVM_clientedictfunction(ed, camera_transform), "QC function e.camera_transform is missing");
			VectorCopy(PRVM_G_VECTOR(OFS_RETURN), origin);
			VectorCopy(PRVM_clientglobalvector(v_forward), forward);
			VectorScale(PRVM_clientglobalvector(v_right), -1, left);
			VectorCopy(PRVM_clientglobalvector(v_up), up);
			VectorCopy(PRVM_clientglobalvector(trace_endpos), visorigin);
			Matrix4x4_Invert_Full(&mat, viewmatrix);
			Matrix4x4_FromVectors(viewmatrix, forward, left, up, origin);
			Matrix4x4_Concat(&matq, viewmatrix, &mat);
			Matrix4x4_TransformPositivePlane(&matq, clipplane->normal[0], clipplane->normal[1], clipplane->normal[2], clipplane->dist, clipplane->normal_and_dist);
		}
	}

	return ret;
}

int CL_VM_GetViewEntity(void)
{
	if(cl.csqc_server2csqcentitynumber[cl.viewentity])
		return cl.csqc_server2csqcentitynumber[cl.viewentity] + MAX_EDICTS;
	return cl.viewentity;
}
