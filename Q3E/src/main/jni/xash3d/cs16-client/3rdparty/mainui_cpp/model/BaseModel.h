/*
BaseModel.h -- base model for simple MVC
Copyright (C) 2017-2018 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef BASE_MODEL_H
#define BASE_MODEL_H

#include "extdll_menu.h"

enum ECellType
{
	CELL_TEXT = 0,
	CELL_IMAGE_DEFAULT,
	CELL_IMAGE_ADDITIVE,
	CELL_IMAGE_TRANS,
	CELL_IMAGE_HOLES,
	// CELL_ITEM,
};

class CMenuBaseModel
{
public:
	virtual ~CMenuBaseModel()  { }

	// every model must implement these methods
	virtual void Update() = 0;
	virtual int GetColumns() const = 0;
	virtual int GetRows() const = 0;
	virtual const char *GetCellText( int line, int column ) = 0;

	// customization
	virtual void OnDeleteEntry( int line ) { }
	virtual void OnActivateEntry( int line ) { }
	virtual unsigned int GetAlignmentForColumn( int column ) const { return QM_LEFT; }
	virtual ECellType GetCellType( int line, int column ) { return CELL_TEXT; }
	virtual bool GetLineColor( int line, unsigned int &fillColor, bool &force ) const { return false; }
	virtual bool GetCellColors( int line, int column, unsigned int &textColor, bool &force ) const { return false; }
	virtual bool IsCellTextWrapped( int line, int column ) { return true; }
	// virtual CMenuBaseItem *GetCellItem( int line, int column ) { return NULL; }

	// sorting
	virtual bool Sort( int column, bool ascend ) { return false; } // false means no sorting support for column
};

#endif // BASE_MODEL_H
