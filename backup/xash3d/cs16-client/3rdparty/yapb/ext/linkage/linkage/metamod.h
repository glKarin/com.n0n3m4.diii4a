/*
 * Copyright (c) 2005-2006 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

 //
 // Heavily stripped down Metamod SDK Headers amalgamated into a single include,
 // usable only for simple extension libraries.
 //
 // License: https://raw.githubusercontent.com/alliedmodders/metamod-hl1/master/GPL.txt
 //

#pragma once

constexpr auto META_INTERFACE_VERSION = "5:13";
constexpr auto MAX_LOGMSG_LEN = 1024;

enum PL_UNLOAD_REASON {
   PNL_NULL = 0,
   PNL_INI_DELETED,
   PNL_FILE_NEWER,
   PNL_COMMAND,
   PNL_CMD_FORCED,
   PNL_DELAYED,
   PNL_PLUGIN,
   PNL_PLG_FORCED,
   PNL_RELOAD
};

enum META_RES {
   MRES_UNSET = 0,
   MRES_IGNORED,
   MRES_HANDLED,
   MRES_OVERRIDE,
   MRES_SUPERCEDE
};

enum PLUG_LOADTIME {
   PT_NEVER = 0,
   PT_STARTUP,
   PT_CHANGELEVEL,
   PT_ANYTIME,
   PT_ANYPAUSE
};


enum ginfo_t {
   GINFO_NAME = 0,
   GINFO_DESC,
   GINFO_GAMEDIR,
   GINFO_DLL_FULLPATH,
   GINFO_DLL_FILENAME,
   GINFO_REALDLL_FULLPATH
};

typedef int (*GETENTITYAPI_FN)(gamefuncs_t *, int );
typedef int (*GETENTITYAPI2_FN)(gamefuncs_t *, int *);
typedef int (*GETNEWDLLFUNCTIONS_FN)(newgamefuncs_t *, int *);
typedef int (*GET_ENGINE_FUNCTIONS_FN)(enginefuncs_t *, int *);

struct plugin_info_t {
   char const *ifvers {};
   char const *name {};
   char const *version {};
   char const *date {};
   char const *author {};
   char const *url {};
   char const *logtag {};
   PLUG_LOADTIME loadable {};
   PLUG_LOADTIME unloadable {};
};

extern plugin_info_t Plugin_info;
typedef plugin_info_t *plid_t;

#define PLID &Plugin_info

struct meta_globals_t {
   META_RES mres {};
   META_RES prev_mres {};
   META_RES status {};
   void *orig_ret {};
   void *override_ret {};
};

extern meta_globals_t *gpMetaGlobals;

#define RETURN_META(result)           \
    {                                 \
        gpMetaGlobals->mres = result; \
        return;                       \
    }

#define RETURN_META_VALUE(result, value) \
    {                                    \
        gpMetaGlobals->mres = result;    \
        return value;                    \
    }

#define META_RESULT_STATUS gpMetaGlobals->status
#define META_RESULT_PREVIOUS gpMetaGlobals->prev_mres
#define META_RESULT_ORIG_RET(type) *reinterpret_cast <type *> (gpMetaGlobals->orig_ret)
#define META_RESULT_OVERRIDE_RET(type) *reinterpret_cast <type *> (gpMetaGlobals->override_ret)

struct metamod_funcs_t {
   GETENTITYAPI_FN pfnGetEntityAPI {};
   GETENTITYAPI_FN pfnGetEntityAPI_Post {};
   GETENTITYAPI2_FN pfnGetEntityAPI2 {};
   GETENTITYAPI2_FN pfnGetEntityAPI2_Post {};
   GETNEWDLLFUNCTIONS_FN pfnGetNewDLLFunctions {};
   GETNEWDLLFUNCTIONS_FN pfnGetNewDLLFunctions_Post {};
   GET_ENGINE_FUNCTIONS_FN pfnGetEngineFunctions {};
   GET_ENGINE_FUNCTIONS_FN pfnGetEngineFunctions_Post {};
};

struct mutil_funcs_t {
   void (*pfnLogConsole)(plid_t plid, const char *szFormat, ...);
   void (*pfnLogMessage)(plid_t plid, const char *szFormat, ...);
   void (*pfnLogError)(plid_t plid, const char *szFormat, ...);
   void (*pfnLogDeveloper)(plid_t plid, const char *szFormat, ...);
   void (*pfnCenterSay)(plid_t plid, const char *szFormat, ...);
   void (*pfnCenterSayParms)(plid_t plid, hudtextparms_t tparms, const char *szFormat, ...);
   void (*pfnCenterSayVarargs)(plid_t plid, hudtextparms_t tparms, const char *szFormat, va_list ap);
   int (*pfnCallGameEntity)(plid_t plid, const char *entStr, entvars_t *pev);
   int (*pfnGetUserMsgID)(plid_t plid, const char *msgname, int *size);
   const char *(*pfnGetUserMsgName)(plid_t plid, int msgid, int *size);
   const char *(*pfnGetPluginPath)(plid_t plid);
   const char *(*pfnGetGameInfo)(plid_t plid, ginfo_t tag);
   int (*pfnLoadPlugin)(plid_t plid, const char *cmdline, PLUG_LOADTIME now, void **plugin_handle);
   int (*pfnUnloadPlugin)(plid_t plid, const char *cmdline, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
   int (*pfnUnloadPluginByHandle)(plid_t plid, void *plugin_handle, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
   const char *(*pfnIsQueryingClienCVar)(plid_t plid, const edict_t *player);
   int (*pfnMakeRequestID)(plid_t plid);
   void (*pfnGetHookTables)(plid_t plid, enginefuncs_t **peng, gamefuncs_t **pdll, newgamefuncs_t **pnewdll);
};

struct gamedll_funcs_t {
   gamefuncs_t *dllapi_table {};
   newgamefuncs_t *newapi_table {};
};

extern gamedll_funcs_t *gpGamedllFuncs;
extern mutil_funcs_t *gpMetaUtilFuncs;
extern metamod_funcs_t gMetaFunctionTable;

#define DECLAREL_FN(prefix, table, fn) \
namespace mdll { \
constexpr struct __metamod___apicall__##prefix##__##fn { \
   template <typename ...Args> constexpr decltype (auto) operator() (Args &&...args) const noexcept { \
      return table->pfn##fn (cr::forward <Args> (args)...); \
   } \
} prefix##_##fn; \
} \
using mdll::prefix##_##fn

#define MDLL_FN(fn) \
   DECLAREL_FN(MDLL, gpGamedllFuncs->dllapi_table, fn)

#define MDLL_UTIL(fn) \
   DECLAREL_FN(MUTIL, gpMetaUtilFuncs, fn)

MDLL_FN (ClientPutInServer);
MDLL_FN (ClientConnect);
MDLL_FN (ClientCommand);
MDLL_FN (ClientKill);
MDLL_FN (Use);
MDLL_FN (Spawn);
MDLL_FN (Touch);
MDLL_FN (KeyValue);

MDLL_UTIL (GetUserMsgID);
MDLL_UTIL (CallGameEntity);
