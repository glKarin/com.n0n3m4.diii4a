// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GRID_H__
#define __GRID_H__

/*
===============================================================================

	3D Grid Container
	
	Allows fast iteration over all objects and can quickly gather objects
	in proximity of a specific object.

===============================================================================
*/

template< class objType > class idGrid3D;
template< class objType > class idGridNearbyObjectList;
template< class objType > class idGridBoundsObjectList;
template< class objType > class idGridLink;

//===============================================================
//
//	idGridObject
//
//===============================================================

template< class objType >
class idGridObject {
	friend class idGrid3D<objType>;
	friend class idGridNearbyObjectList<objType>;
	friend class idGridBoundsObjectList<objType>;

public:
	objType							GetObject() const { return object; }

private:
	objType							object;			// actual object
	int								index;			// index into object list
	class idGridLink<objType> *		firstCell;		// first cell this object is linked into
	int								checkCount;		// to avoid iterating objects more than once
};

//===============================================================
//
//	idGridNearbyObjectList
//
//===============================================================

template< class objType >
class idGridNearbyObjectList {
	friend class idGrid3D<objType>;

public:
	idGridObject<objType> *			GetNextObject() const;

private:
	idGridObject<objType> *			gridObject;		// grid object used to create this list
	int								checkCount;		// current check count
	mutable idGridLink<objType> *	nextObject;		// next object in cell
	mutable idGridLink<objType> *	prevObject;		// prev object in cell
	mutable idGridLink<objType> *	nextCell;		// next cell to iterate
};

//===============================================================
//
//	idGridBoundsObjectList
//
//===============================================================

template< class objType >
class idGridBoundsObjectList {
	friend class idGrid3D<objType>;

public:
	idGridObject<objType> *			GetNextObject() const;

private:
	int								checkCount;		// current check count
	mutable idGridLink<objType> *	nextObject;		// next object in cell
	mutable idList<idGridLink<objType> *>cells;		// list with cell to iterate
};

//===============================================================
//
//	idGridLink
//
//===============================================================

template< class objType >
class idGridLink {
public:
	idGridObject<objType> *			gridObject;		// the object linked in this cell
	idGridLink<objType> *			nextObject;		// next object in this cell
	idGridLink<objType> *			prevObject;		// prev object in this cell
	idGridLink<objType> *			nextCell;		// next cell this object is in
};

//===============================================================
//
//	idGrid3D
//
//===============================================================

template< class objType >
class idGrid3D {
public:
									idGrid3D();
									~idGrid3D();

	void							Clear();

	void							Init( const idBounds &bounds, float desiredCellSize );

									// Adds an object to the grid.
	idGridObject<objType> *			AddObject( objType object, const idBounds &bounds, const float epsilon );
									// Removes an object from the grid. NOTE: this may invalidate any idGridObjectLists.
	void							RemoveObject( idGridObject<objType> *gridObject );
									// Relinks the object to the grid with the new bounds.
	void							ChangeObjectBounds( idGridObject<objType> *gridObject, const idBounds &bounds, const float epsilon );

									// Returns the number of objects in the grid.
	int								GetNumObjects() const { return gridObjects.Num(); }
									// Returns the object at the given index.
	idGridObject<objType> *			GetObject( int i ) const { return gridObjects[i]; }

									// Initializes an object list with objects nearby the given object.
	void							GetNearbyObjects( idGridObject<objType> *object, idGridNearbyObjectList<objType> &list );
									// Initializes an object list with objects touching the given bounds.
	void							GetBoundsObjects( const idBounds &bounds, const float epsilon, idGridBoundsObjectList<objType> &list );

	const idBounds &				GetGridBounds() const { return gridBounds; }
	float							GetDesiredCellSize() const { return desiredCellSize; }

	static void						Test();

private:
	idBounds						gridBounds;
	float							desiredCellSize;
	float							cellSize[3];
	float							invCellSize[3];
	int								cellsPerAxis[3];
	idGridLink<objType> ***			grid;
	idList<idGridObject<objType> *>	gridObjects;
	int								checkCount;

	idBlockAlloc<idGridObject<objType>, 1024>	gridObjectAllocator;
	idBlockAlloc<idGridLink<objType>, 1024>		gridLinkAllocator;

	void							GetGridCells( const idBounds &bounds, int iBounds[2][3] ) const;
	void							LinkObject( idGridObject<objType> *gridObject, const idBounds &bounds, const float epsilon );
	void							UnlinkObject( idGridObject<objType> *gridObject );
};

//===============================================================
//
//	Implementation
//
//===============================================================

template< class objType >
ID_INLINE idGridObject<objType> * idGridNearbyObjectList<objType>::GetNextObject() const {
	idGridObject<objType> *object;

	do {
		if ( nextObject != NULL ) {
			object = nextObject->gridObject;
			nextObject = nextObject->nextObject;
		} else if ( prevObject != NULL ) {
			object = prevObject->gridObject;
			prevObject = prevObject->prevObject;
		} else if ( nextCell != NULL ) {
			object = NULL;
			nextObject = nextCell->nextObject;
			prevObject = nextCell->prevObject;
			nextCell = nextCell->nextCell;
		} else {
			return NULL;
		}
	} while( object == NULL || object->checkCount == checkCount || object == gridObject );

	object->checkCount = checkCount;

	return object;
}

template< class objType >
ID_INLINE idGridObject<objType> * idGridBoundsObjectList<objType>::GetNextObject() const {
	idGridObject<objType> *object;

	do {
		if ( nextObject != NULL ) {
			object = nextObject->gridObject;
			nextObject = nextObject->nextObject;
		} else if ( cells.Num() > 0 ) {
			object = NULL;
			nextObject = cells[cells.Num() - 1]->nextObject;
			cells.SetNum( cells.Num() - 1 );
		} else {
			return NULL;
		}
	} while( object == NULL || object->checkCount == checkCount );

	object->checkCount = checkCount;

	return object;
}

template< class objType >
ID_INLINE idGrid3D<objType>::idGrid3D() {
	gridBounds.Clear();
	desiredCellSize = 1.0f;
	cellSize[0] = cellSize[1] = cellSize[2] = 0.0f;
	invCellSize[0] = invCellSize[1] = invCellSize[2] = 0.0f;
	cellsPerAxis[0] = cellsPerAxis[1] = cellsPerAxis[2] = 128;
	grid = NULL;
	gridObjects.SetGranularity( 1024 );
}

template< class objType >
ID_INLINE idGrid3D<objType>::~idGrid3D() {
	Clear();
}

template< class objType >
ID_INLINE void idGrid3D<objType>::Clear( ) {
	Mem_Free( grid );
	grid = NULL;
	gridObjectAllocator.Shutdown();
	gridLinkAllocator.Shutdown();

	gridBounds.Clear();
	cellSize[0] = cellSize[1] = cellSize[2] = 0.0f;
	invCellSize[0] = invCellSize[1] = invCellSize[2] = 0.0f;
	cellsPerAxis[0] = cellsPerAxis[1] = cellsPerAxis[2] = 128;
	gridObjects.Clear();
}

template< class objType >
ID_INLINE void idGrid3D<objType>::Init( const idBounds &bounds, float desiredCellSize ) {
	int i, j;
	float size;
	byte *data;

	Clear();

	this->gridBounds = bounds;
	this->desiredCellSize = desiredCellSize;

	for ( i = 0; i < 3; i++ ) {
		size = gridBounds[1][i] - gridBounds[0][i];
		cellsPerAxis[i] = idMath::Ftoi( size / desiredCellSize );
		if ( cellsPerAxis[i] < 2 ) {
			cellsPerAxis[i] = 2;
		} else if ( cellsPerAxis[i] > 128 ) {
			cellsPerAxis[i] = 128;
		}
		cellSize[i] = size / cellsPerAxis[i];
		invCellSize[i] = 1.0f / cellSize[i];
	}

	data = (byte *)Mem_Alloc(	cellsPerAxis[0] * sizeof( idGridLink<objType> ** ) +
								cellsPerAxis[0] * cellsPerAxis[1] * sizeof( idGridLink<objType> * ) +
								cellsPerAxis[0] * cellsPerAxis[1] * cellsPerAxis[2] * sizeof( idGridLink<objType> ) );

	grid = (idGridLink<objType> ***) data;
	data += cellsPerAxis[0] * sizeof( idGridLink<objType> ** );
	for ( i = 0; i < cellsPerAxis[0]; i++ ) {
		grid[i] = (idGridLink<objType> **) data;
		data += cellsPerAxis[1] * sizeof( idGridLink<objType> * );
		for ( j = 0; j < cellsPerAxis[1]; j++ ) {
			grid[i][j] = (idGridLink<objType> *) data;
			data += cellsPerAxis[2] * sizeof( idGridLink<objType> );
			memset( grid[i][j], 0, cellsPerAxis[2] * sizeof( idGridLink<objType> ) );
		}
	}

	checkCount = 0;
}

template< class objType >
ID_INLINE void idGrid3D<objType>::GetGridCells( const idBounds &bounds, int iBounds[2][3] ) const {
	int i;

	for ( i = 0; i < 3; i++ ) {
		iBounds[0][i] = idMath::Ftoi( ( bounds[0][i] - gridBounds[0][i] - 0.001f ) * invCellSize[i] );
		if ( iBounds[0][i] < 0 ) {
			iBounds[0][i] = 0;
		} else if ( iBounds[0][i] >= cellsPerAxis[i] ) {
			iBounds[0][i] = cellsPerAxis[i] - 1;
		}

		iBounds[1][i] = idMath::Ftoi( ( bounds[1][i] - gridBounds[0][i] + 0.001f ) * invCellSize[i] );
		if ( iBounds[1][i] < 0 ) {
			iBounds[1][i] = 0;
		} else if ( iBounds[1][i] >= cellsPerAxis[i] ) {
			iBounds[1][i] = cellsPerAxis[i] - 1;
		}
	}
}

template< class objType >
ID_INLINE void idGrid3D<objType>::LinkObject( idGridObject<objType> *gridObject, const idBounds &bounds, const float epsilon ) {
	int i, j, k;
	int iBounds[2][3];
	idGridLink<objType> *gridLink;

	assert( gridObject->firstCell == NULL );

	GetGridCells( bounds.Expand( epsilon ), iBounds );

	// add the links
	for ( i = iBounds[0][0]; i <= iBounds[1][0]; i++ ) {
		for ( j = iBounds[0][1]; j <= iBounds[1][1]; j++ ) {
			for ( k = iBounds[0][2]; k <= iBounds[1][2]; k++ ) {
				gridLink = gridLinkAllocator.Alloc();
				gridLink->gridObject = gridObject;
				gridLink->nextObject = grid[i][j][k].nextObject;
				gridLink->prevObject = &grid[i][j][k];
				if ( gridLink->nextObject != NULL ) {
					gridLink->nextObject->prevObject = gridLink;
				}
				gridLink->nextCell = gridObject->firstCell;
				gridObject->firstCell = gridLink;
				grid[i][j][k].nextObject = gridLink;
			}
		}
	}
}

template< class objType >
ID_INLINE void idGrid3D<objType>::UnlinkObject( idGridObject<objType> *gridObject ) {
	idGridLink<objType> *gridLink, *nextGridLink;

	assert( gridObject->firstCell != NULL );

	for ( gridLink = gridObject->firstCell; gridLink != NULL; gridLink = nextGridLink ) {
		nextGridLink = gridLink->nextCell;
		if ( gridLink->nextObject != NULL ) {
			gridLink->nextObject->prevObject = gridLink->prevObject;
		}
		if ( gridLink->prevObject != NULL ) {
			gridLink->prevObject->nextObject = gridLink->nextObject;
		}
		gridLinkAllocator.Free( gridLink );
	}
	gridObject->firstCell = NULL;
}

template< class objType >
ID_INLINE idGridObject<objType> * idGrid3D<objType>::AddObject( objType	object, const idBounds &bounds, const float epsilon ) {
	idGridObject<objType> *gridObject;

	assert( grid != NULL );		// make sure the grid is initialized

	gridObject = gridObjectAllocator.Alloc();
	gridObject->checkCount = -1;
	gridObject->firstCell = NULL;
	gridObject->object = object;
	gridObject->index = gridObjects.Append( gridObject );

	LinkObject( gridObject, bounds, epsilon );

	return gridObject;
}

template< class objType >
ID_INLINE void idGrid3D<objType>::RemoveObject( idGridObject<objType> *gridObject ) {

	assert( grid != NULL );		// make sure the grid is initialized

	UnlinkObject( gridObject );

	// move the last object to the empty spot
	gridObjects[ gridObject->index ] = gridObjects[ gridObjects.Num() - 1 ];
	gridObjects[ gridObject->index ]->index = gridObject->index;
	gridObjects.SetNum( gridObjects.Num() - 1, false );

	gridObjectAllocator.Free( gridObject );
}

template< class objType >
ID_INLINE void idGrid3D<objType>::ChangeObjectBounds( idGridObject<objType> *gridObject, const idBounds &bounds, const float epsilon ) {
	assert( grid != NULL );		// make sure the grid is initialized
	UnlinkObject( gridObject );
	LinkObject( gridObject, bounds, epsilon );
}

template< class objType >
ID_INLINE void idGrid3D<objType>::GetNearbyObjects( idGridObject<objType> *object, idGridNearbyObjectList<objType> &list ) {
	list.gridObject = object;
	list.checkCount = checkCount++;
	list.nextObject = object->firstCell->nextObject;
	list.prevObject = object->firstCell->prevObject;
	list.nextCell = object->firstCell->nextCell;
}

template< class objType >
ID_INLINE void idGrid3D<objType>::GetBoundsObjects( const idBounds &bounds, const float epsilon, idGridBoundsObjectList<objType> &list ) {
	int i, j, k;
	int iBounds[2][3];

	GetGridCells( bounds.Expand( epsilon ), iBounds );

	list.nextObject = NULL;
	list.checkCount = checkCount++;
	list.cells.SetNum( 0 );
	for ( i = iBounds[0][0]; i <= iBounds[1][0]; i++ ) {
		for ( j = iBounds[0][1]; j <= iBounds[1][1]; j++ ) {
			for ( k = iBounds[0][2]; k <= iBounds[1][2]; k++ ) {
				list.cells.Append( &grid[i][j][k] );
			}
		}
	}

	if ( list.cells.Num() > 0 ) {
		list.nextObject = list.cells[list.cells.Num() - 1]->nextObject;
		list.cells.SetNum( list.cells.Num() - 1 );
	}
}

template< class objType >
ID_INLINE void idGrid3D<objType>::Test() {
	int i;
	idGrid3D<int> grid;
	idGridObject<int> *obj1, *obj2;
	idGridNearbyObjectList<int> list;

	grid.Init( idBounds( idVec3( -8 ), idVec3( 8 ) ), 4.0f );

	for ( i = 0; i < grid.GetNumObjects(); i++ ) {
		obj1 = grid.GetObject( i );

		grid.GetNearbyObjects( obj1, list );

		for ( obj2 = list.GetNextObject(); obj2 != NULL; obj2 = list.GetNextObject() ) {
		}
	}
}

#endif /* !__GRID_H__ */
