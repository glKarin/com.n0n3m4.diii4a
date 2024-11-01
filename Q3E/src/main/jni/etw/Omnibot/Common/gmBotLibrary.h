#ifndef __GMBOTLIBRARY_H__
#define __GMBOTLIBRARY_H__

#define GM_CHECK_GAMEENTITY_FROM_PARAM(ent, paramnum) \
	if(a_thread->ParamType(paramnum) == GM_ENTITY) \
	{ \
	ent.FromInt(a_thread->Param(paramnum).m_value.m_enthndl);  \
	} \
	else if(a_thread->ParamType(paramnum) == GM_INT) \
	{ \
	ent = g_EngineFuncs->EntityFromID(a_thread->Param(paramnum).m_value.m_int); \
	} \
	else \
	{ \
	GM_EXCEPTION_MSG("expecting param %d gameentity or int param. got %s", paramnum, GM_THREAD_ARG->GetMachine()->GetTypeName(GM_THREAD_ARG->ParamType(paramnum))); \
	return GM_EXCEPTION; \
	} \

#define GM_GAMEENTITY_FROM_PARAM(ent, paramnum, def) \
	if(a_thread->ParamType(paramnum) == GM_ENTITY) \
	{ \
	ent.FromInt(a_thread->Param(paramnum).m_value.m_enthndl);  \
	} \
	else if(a_thread->ParamType(paramnum) == GM_INT) \
	{ \
	ent = g_EngineFuncs->EntityFromID(a_thread->Param(paramnum).m_value.m_int); \
	} \
	else \
	{ \
	ent = def; \
	} \
class gmMachine;

void gmBindBotLib(gmMachine * a_machine);
void MapDebugPrint(const char *message);
void MapDebugPrint(gmThread *a_thread, const char *message);
int GM_CDECL GetMapGoals(gmThread *a_thread, Client *client);

#endif
