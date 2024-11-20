#include "PrecompCommon.h"
#include "gmTriggerInfo.h"
#include "gmGameEntity.h"

// script: TriggerInfo
//		Script bindings for <TriggerInfo>

GMBIND_INIT_TYPE( gmTriggerInfo, "TriggerInfo" );

// property: Name
//		Name of the trigger.

// property: Action
//		Action name of the trigger.

// property: Activator
//		Activator of the trigger.

// property: Entity
//		The entity that the trigger is on.

GMBIND_PROPERTY_MAP_BEGIN( gmTriggerInfo )
	GMBIND_PROPERTY( "Name", getName, NULL )
	GMBIND_PROPERTY( "Action", getAction, NULL )
	GMBIND_PROPERTY( "Activator", getActivator, NULL )
	GMBIND_PROPERTY( "Entity", getEntity, NULL )
GMBIND_PROPERTY_MAP_END();

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

TriggerInfo *gmTriggerInfo::Constructor(gmThread *a_thread)
{
	return NULL;
}

void gmTriggerInfo::Destructor(TriggerInfo *_native)
{
}

//////////////////////////////////////////////////////////////////////////
// Functions

//////////////////////////////////////////////////////////////////////////
// Property Accessors/Modifiers

bool gmTriggerInfo::getName( TriggerInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native->m_TagName)
	{
		gmStringObject *pString = a_thread->GetMachine()->AllocStringObject(a_native->m_TagName);
		a_operands[0].SetString(pString);
	}
	else
		a_operands[0].Nullify();
	return true;
}

bool gmTriggerInfo::getAction( TriggerInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native->m_Action)
	{
		gmStringObject *pString = a_thread->GetMachine()->AllocStringObject(a_native->m_Action);
		a_operands[0].SetString(pString);
	}
	else
		a_operands[0].Nullify();
	return true;
}

bool gmTriggerInfo::getActivator( TriggerInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native->m_Activator.IsValid())
	{
		gmVariable v;
		a_operands[0].SetEntity(a_native->m_Activator.AsInt());
	}
	else
		a_operands[0].Nullify();
	return true;
}

bool gmTriggerInfo::getEntity( TriggerInfo *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_native->m_Entity.IsValid())
	{
		gmVariable v;
		a_operands[0].SetEntity(a_native->m_Entity.AsInt());
	}
	else
		a_operands[0].Nullify();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Operators
