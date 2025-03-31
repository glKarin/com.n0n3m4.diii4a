/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "RawVector.h"

CRawVector::CRawVector()
	: m_Pointer(0)
	, m_Size(0), m_Capacity(0)
{
	reallocate(INITIAL_CAPACITY);
}

CRawVector::~CRawVector()
{
	free(m_Pointer);
}

void CRawVector::reallocate(int newSize) {
	int newCapacity = m_Capacity << 1;
	if (newCapacity < newSize) newCapacity = newSize;

	char *newbuffer = (char*)realloc(m_Pointer, newCapacity);
	if (!newbuffer)
		common->FatalError("CRawBuffer::resize: realloc failed (from %d to %d bytes)", m_Capacity, newCapacity);

	m_Pointer = newbuffer;
	m_Capacity = newCapacity;
}

void CRawVector::clear() {
	resize(0);
}

namespace std {
	void swap(CRawVector &a, CRawVector &b) {
		static const int RVS = sizeof(CRawVector);
		char buffer[RVS];
		memcpy(buffer, &a, RVS);
		memcpy(&a, &b, RVS);
		memcpy(&b, buffer, RVS);
	}
}
