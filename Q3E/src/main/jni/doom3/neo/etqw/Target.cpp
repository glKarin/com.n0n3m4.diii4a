// Copyright (C) 2007 Id Software, Inc.
//

/*

Invisible entities that affect other entities or the world when activated.

*/

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Target.h"

/*
===============================================================================

idTarget

===============================================================================
*/

CLASS_DECLARATION( idEntity, idTarget )
END_CLASS

/*
===============================================================================

idTarget_Null

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_Null )
END_CLASS

/*
================
idTarget_Null::Spawn
================
*/
void idTarget_Null::Spawn() {
	PostEventMS( &EV_Remove, 100 );
}
