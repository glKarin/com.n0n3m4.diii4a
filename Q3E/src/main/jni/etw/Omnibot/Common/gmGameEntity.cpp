#include "PrecompCommon.h"
#include "ScriptManager.h"

//////////////////////////////////////////////////////////////////////////
// application level overrides, since we treat integers interchangably with game entities
static int gmEntityOpEQ2(gmThread * a_thread, gmVariable * a_operands)
{
	if(a_operands[0].m_type == GM_ENTITY)
	{
		switch(a_operands[1].m_type)
		{
		case GM_ENTITY:
			{
				a_operands[0].SetInt(a_operands[0].m_value.m_enthndl == a_operands[1].m_value.m_enthndl ? 1 : 0);
				return GM_OK;
			}
		case GM_INT:
			{
				GameEntity ent0; 
				ent0.FromInt(a_operands[0].m_value.m_enthndl);
				GameEntity ent1 = g_EngineFuncs->EntityFromID(a_operands[1].m_value.m_int);
				a_operands[0].SetInt(ent0 == ent1 ? 1 : 0);
				return GM_OK;
			}
		case GM_NULL:
			{
				a_operands[0].SetInt(0);
				return GM_OK;
			}
		}
	}
	else if(a_operands[1].m_type == GM_ENTITY)
	{
		switch(a_operands[0].m_type)
		{
		case GM_INT:
			{
				GameEntity ent0 = g_EngineFuncs->EntityFromID(a_operands[0].m_value.m_int);
				GameEntity ent1; 
				ent1.FromInt(a_operands[1].m_value.m_enthndl);
				a_operands[0].SetInt(ent0 == ent1 ? 1 : 0);
				return GM_OK;
			}
		case GM_NULL:
			{
				a_operands[0].SetInt(0);
				return GM_OK;
			}
		}
	}

	return GM_EXCEPTION;
}

static int gmEntityOpNEQ2(gmThread * a_thread, gmVariable * a_operands)
{
	if(a_operands[0].m_type == GM_ENTITY)
	{
		switch(a_operands[1].m_type)
		{
		case GM_ENTITY:
			{
				a_operands[0].SetInt(a_operands[0].m_value.m_enthndl != a_operands[1].m_value.m_enthndl ? 1 : 0);
				return GM_OK;
			}
		case GM_INT:
			{
				GameEntity ent0; 
				ent0.FromInt(a_operands[0].m_value.m_enthndl);
				GameEntity ent1 = g_EngineFuncs->EntityFromID(a_operands[1].m_value.m_int);
				a_operands[0].SetInt(ent0 != ent1 ? 1 : 0);
				return GM_OK;
			}
		case GM_NULL:
			{
				a_operands[0].SetInt(1);
				return GM_OK;
			}
		}
	}
	else if(a_operands[1].m_type == GM_ENTITY)
	{
		switch(a_operands[0].m_type)
		{
		case GM_INT:
			{
				GameEntity ent0 = g_EngineFuncs->EntityFromID(a_operands[0].m_value.m_int);
				GameEntity ent1; 
				ent1.FromInt(a_operands[1].m_value.m_enthndl);
				a_operands[0].SetInt(ent0 != ent1 ? 1 : 0);
				return GM_OK;
			}
		case GM_NULL:
			{
				a_operands[0].SetInt(1);
				return GM_OK;
			}
		}
	}

	return GM_EXCEPTION;
}

void BindEntityStackCustom(gmMachine *a_machine)
{	
	a_machine->RegisterTypeOperator(GM_ENTITY, O_EQ, 0, gmEntityOpEQ2);
	a_machine->RegisterTypeOperator(GM_ENTITY, O_NEQ, 0, gmEntityOpNEQ2);
}
