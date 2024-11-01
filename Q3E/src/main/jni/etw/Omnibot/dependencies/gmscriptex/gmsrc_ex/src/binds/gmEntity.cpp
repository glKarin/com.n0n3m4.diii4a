#include "gmConfig.h"

#if(GM_USE_ENTITY_STACK)

#include "gmEntity.h"
#include "gmThread.h"

//////////////////////////////////////////////////////////////////////////
// Operators

static int gmEntityOpEQ(gmThread * a_thread, gmVariable * a_operands)
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
		case GM_NULL:
			{
				a_operands[0].SetInt(0);
				return GM_OK;
			}
		}
	}

	return GM_EXCEPTION;
}

static int gmEntityOpNEQ(gmThread * a_thread, gmVariable * a_operands)
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
		case GM_NULL:
			{
				a_operands[0].SetInt(1);
				return GM_OK;
			}
		}
	}
	return GM_EXCEPTION;
}

static int gmEntityOpNOT(gmThread * a_thread, gmVariable * a_operands)
{
	if(a_operands[0].m_type == GM_NULL)
		a_operands[0].SetInt(1);
	else
		a_operands[0].SetInt(0);
	a_operands[0].m_type = GM_INT;
	return GM_OK;
}

static int gmEntityOpBOOL(gmThread * a_thread, gmVariable * a_operands)
{
	a_operands[0].SetInt( !a_operands[0].IsNull() );
	return GM_OK;
}

void BindEntityStack(gmMachine *a_machine)
{	
	a_machine->RegisterTypeOperator(GM_ENTITY, O_EQ, 0, gmEntityOpEQ);
	a_machine->RegisterTypeOperator(GM_ENTITY, O_NEQ, 0, gmEntityOpNEQ);
	a_machine->RegisterTypeOperator(GM_ENTITY, O_NOT, 0, gmEntityOpNOT);

#if GM_BOOL_OP
	a_machine->RegisterTypeOperator(GM_ENTITY, O_BOOL, 0, gmEntityOpBOOL);
#endif	
}

#endif