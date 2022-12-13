
/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "CollisionModel_local.h"

/*
===============================================================================

Retrieving contacts

===============================================================================
*/

/*
==================
idCollisionModelManagerLocal::Contacts
==================
*/
int idCollisionModelManagerLocal::Contacts( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
								const idTraceModel *trm, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &origin, const idMat3 &modelAxis ) {
	trace_t results;
	idVec3 end;

	// same as Translation but instead of storing the first collision we store all collisions as contacts
	idCollisionModelManagerLocal::getContacts = true;
	idCollisionModelManagerLocal::contacts = contacts;
	idCollisionModelManagerLocal::maxContacts = maxContacts;
	idCollisionModelManagerLocal::numContacts = 0;
	end = start + dir.SubVec3(0) * depth;
	idCollisionModelManagerLocal::Translation( &results, start, end, trm, trmAxis, contentMask, model, origin, modelAxis );
	if ( dir.SubVec3(1).LengthSqr() != 0.0f ) {
		// FIXME: rotational contacts
	}
	idCollisionModelManagerLocal::getContacts = false;
	idCollisionModelManagerLocal::maxContacts = 0;

	return idCollisionModelManagerLocal::numContacts;
}
