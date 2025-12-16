#pragma once
#ifndef STRINGVECTORMODEL_H
#define STRINGVECTORMODEL_H

#include "BaseArrayModel.h"
#include "utlvector.h"
#include "utlstring.h"

class CStringVectorModel :
		public CMenuBaseArrayModel,
		public CUtlVector<CUtlString>
{
public:
	CStringVectorModel( int growSize = 0, int initSize = 0 ) :
		CUtlVector<CUtlString>( growSize, initSize )
	{

	}

	// no need to update
	void Update() override {}

	const char *GetText( int line ) final override
	{
		if( line < 0 )
			return NULL;

		return Element( line ).String();
	}

	int GetRows() const final override
	{
		return Count();
	}
};
#endif // STRINGVECTORMODEL_H
