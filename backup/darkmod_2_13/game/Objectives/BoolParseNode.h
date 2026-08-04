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

#ifndef TDM_OBJECTIVE_BOOLPARSENODE_H
#define TDM_OBJECTIVE_BOOLPARSENODE_H

#include "precompiled.h"

/**
* Structure for parsing boolean logic
**/
struct SBoolParseNode
{
	int Ident;
	bool bNotted; // set to true if this node is NOTed

	/* 
	*  Note from SteveL #4251. The following couple of lines are dangerous. We should not store 
	*  nodes that hold raw pointers to one another in idLists, which will reallocate their dynamic memory 
	*  if they grow too big, thus invalidating the pointers. Rather than redesign the decision tree, which would 
	*  be a big change given the lack of container classes that both allow random access and guarantee no memory 
	*  movement, we'll override assignment and copying, and repair the pointers for any node that gets copied.  
	*/
	idList< idList< SBoolParseNode > > Cols; // list of columns, each can contain a different number of rows

	SBoolParseNode* PrevNode; // Link back to previous node this one branched off from

	// matrix coordinates of this node within the matrix of the previous node
	int PrevCol; 
	int PrevRow;

	SBoolParseNode()
	{ 
		Clear();
	}

	~SBoolParseNode()
	{
		Clear();
	}

	bool IsEmpty() const
	{ 
		return (Cols.Num() == 0 && Ident == -1);
	}

	/**
	* Clear the parse node
	**/
	void Clear( void )
	{
		Ident = -1;
		PrevCol = -1;
		PrevRow = -1;

		bNotted = false;
		Cols.ClearFree();
		PrevNode = NULL;
	}

	/*
	* PrevNode pointers in this node's children will be left dangling if this node gets 
	* copied to a new memory location. Ensure they are fixed by overriding assignment and copying.
	* See notes above. Added by SteveL #4251.
	*/
	SBoolParseNode& operator=(const SBoolParseNode& other)
	{
		if ( this != &other )
		{
			// Make the copy
			Ident = other.Ident;
			bNotted = other.bNotted; // set to true if this node is NOTed
			Cols = other.Cols; // list of columns, each can contain a different number of rows
			PrevNode = other.PrevNode;
			PrevCol = other.PrevCol; 
			PrevRow = other.PrevRow;
		
			// Then fix the pointers
			for ( int col = 0; col < Cols.Num(); ++col )
			{
				for (int row = 0; row < Cols[col].Num(); ++row )
				{
					SBoolParseNode* child = &Cols[col][row];
					child->PrevNode = this;
				}
			}
		}
		return *this;
	}

	SBoolParseNode(const SBoolParseNode& other)
	{
		operator=(other);
	}
};

#endif
