/*
physint.h - Server Physics Interface
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

//
// Heavily stripped down Xash3D FWGS SDK Headers amalgamated into a single include,
// usable only for simple extension libraries.
//
// License: https://www.gnu.org/licenses/gpl-3.0-standalone.html
//

#pragma once

constexpr auto SV_PHYSICS_INTERFACE_VERSION = 6;

constexpr auto SERVER_DEAD = 0;
constexpr auto SERVER_LOADING = 1;
constexpr auto SERVER_ACTIVE = 2;

constexpr auto LUMP_LOAD_OK = 0;
constexpr auto LUMP_LOAD_COULDNT_OPEN = 1;
constexpr auto LUMP_LOAD_BAD_HEADER = 2;
constexpr auto LUMP_LOAD_BAD_VERSION = 3;
constexpr auto LUMP_LOAD_NO_EXTRADATA = 4;
constexpr auto LUMP_LOAD_INVALID_NUM = 5;
constexpr auto LUMP_LOAD_NOT_EXIST = 6;
constexpr auto LUMP_LOAD_MEM_FAILED = 7;
constexpr auto LUMP_LOAD_CORRUPTED = 8;
constexpr auto LUMP_SAVE_OK = 0;
constexpr auto LUMP_SAVE_COULDNT_OPEN = 1;
constexpr auto LUMP_SAVE_BAD_HEADER = 2;
constexpr auto LUMP_SAVE_BAD_VERSION = 3;
constexpr auto LUMP_SAVE_NO_EXTRADATA = 4;
constexpr auto LUMP_SAVE_INVALID_NUM = 5;
constexpr auto LUMP_SAVE_ALREADY_EXIST = 6;
constexpr auto LUMP_SAVE_NO_DATA = 7;
constexpr auto LUMP_SAVE_CORRUPTED = 8;

struct areanode_t {
   int axis;
   float dist;
   areanode_t *children[2];
   link_t trigger_edicts;
   link_t solid_edicts;
   link_t portal_edicts;
};

struct server_physics_api_t {
   void (*pfnLinkEdict)(edict_t *ent, qboolean touch_triggers);
   double (*pfnGetServerTime)(void);
   double (*pfnGetFrameTime)(void);
   void *(*pfnGetModel)(int modelindex);
   areanode_t *(*pfnGetHeadnode)(void);
   int (*pfnServerState)(void);
   void (*pfnHost_Error)(const char *error, ...);
   struct triangleapi_s *pTriAPI;
   int (*pfnDrawConsoleString)(int x, int y, char *string);
   void (*pfnDrawSetTextColor)(float r, float g, float b);
   void (*pfnDrawConsoleStringLen)(const char *string, int *length, int *height);
   void (*Con_NPrintf)(int pos, const char *fmt, ...);
   void (*Con_NXPrintf)(struct con_nprint_s *info, const char *fmt, ...);
   const char *(*pfnGetLightStyle)(int style);
   void (*pfnUpdateFogSettings)(unsigned int packed_fog);
   char **(*pfnGetFilesList)(const char *pattern, int *numFiles, int gamedironly);
   struct msurface_s *(*pfnTraceSurface)(edict_t *pTextureEntity, const float *v1, const float *v2);
   const byte *(*pfnGetTextureData)(unsigned int texnum);
   void *(*pfnMemAlloc)(size_t cb, const char *filename, const int fileline);
   void (*pfnMemFree)(void *mem, const char *filename, const int fileline);
   int (*pfnMaskPointContents)(const float *pos, int groupmask);
   struct trace_t (*pfnTrace)(const float *p0, float *mins, float *maxs, const float *p1, int type, edict_t *e);
   struct trace_t (*pfnTraceNoEnts)(const float *p0, float *mins, float *maxs, const float *p1, int type, edict_t *e);
   int (*pfnBoxInPVS)(const float *org, const float *boxmins, const float *boxmaxs);
   void (*pfnWriteBytes)(const byte *bytes, int count);
   int (*pfnCheckLump)(const char *filename, const int lump, int *lumpsize);
   int (*pfnReadLump)(const char *filename, const int lump, void **lumpdata, int *lumpsize);
   int (*pfnSaveLump)(const char *filename, const int lump, void *lumpdata, int lumpsize);
   int (*pfnSaveFile)(const char *filename, const void *data, int len);
   const byte *(*pfnLoadImagePixels)(const char *filename, int *width, int *height);
   const char *(*pfnGetModelName)(int modelindex);
   void *(*pfnGetNativeObject)(const char *object);
};

struct physics_interface_t {
   int version;
   int (*SV_CreateEntity)(edict_t *pent, const char *szName);
   int (*SV_PhysicsEntity)(edict_t *pEntity);
   int (*SV_LoadEntities)(const char *mapname, char *entities);
   void (*SV_UpdatePlayerBaseVelocity)(edict_t *ent);
   int (*SV_AllowSaveGame)(void);
   int (*SV_TriggerTouch)(edict_t *pent, edict_t *trigger);
   unsigned int (*SV_CheckFeatures)(void);
   void (*DrawDebugTriangles)(void);
   void (*DrawNormalTriangles)(void);
   void (*DrawOrthoTriangles)(void);
   void (*ClipMoveToEntity)(edict_t *ent, const float *start, float *mins, float *maxs, const float *end, trace_t *trace);
   void (*ClipPMoveToEntity)(struct physent_s *pe, const float *start, float *mins, float *maxs, const float *end, struct pmtrace_s *tr);
   void (*SV_EndFrame)(void);
   void (*pfnPrepWorldFrame)(void);
   void (*pfnCreateEntitiesInRestoreList)(struct SAVERESTOREDATA *pSaveData, int levelMask, qboolean create_world);
   string_t (*pfnAllocString)(const char *szValue);
   string_t (*pfnMakeString)(const char *szValue);
   const char *(*pfnGetString)(string_t iString);
   int (*pfnRestoreDecal)(struct decallist_s *entry, edict_t *pEdict, qboolean adjacent);
   void (*PM_PlayerTouch)(struct playermove_s *ppmove, edict_t *client);
   void (*Mod_ProcessUserData)(model_t *mod, qboolean create, const byte *buffer);
   void *(*SV_HullForBsp)(edict_t *ent, const float *mins, const float *maxs, float *offset);
   int (*SV_PlayerThink)(edict_t *ent, float frametime, double time);
};
