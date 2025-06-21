#pragma once
#ifndef BASE_ARRAY_MODEL_H
#define BASE_ARRAY_MODEL_H

#include "BaseModel.h"

class CMenuBaseArrayModel : public CMenuBaseModel
{
public:
	// every array model must implement these methods
	virtual const char *GetText( int line ) = 0;

	// final methods
	int GetColumns() const final override
	{
		return 1;
	}

	const char *GetCellText( int line, int ) final override
	{
		return GetText( line );
	}
};

#endif // BASE_ARRAY_MODEL_H
