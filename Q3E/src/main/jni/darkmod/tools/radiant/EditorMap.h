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

extern	char		currentmap[1024];

// head/tail of doubly linked lists
extern	brush_t	active_brushes;	// brushes currently being displayed
extern	brush_t	selected_brushes;	// highlighted


extern CPtrArray& g_ptrSelectedFaces;
extern CPtrArray& g_ptrSelectedFaceBrushes;
//extern	face_t	*selected_face;
//extern	brush_t	*selected_face_brush;
extern	brush_t	filtered_brushes;	// brushes that have been filtered or regioned

extern	entity_t	entities;
extern	entity_t	*world_entity;	// the world entity is NOT included in
									// the entities chain

extern	int modified;		// for quit confirmations

extern	idVec3	region_mins, region_maxs;
extern	bool	region_active;

void 	Map_LoadFile (const char *filename);
bool 	Map_SaveFile (const char *filename, bool use_region, bool autosave = false);
void	Map_New (void);
void	Map_BuildBrushData(void);

void	Map_RegionOff (void);
void	Map_RegionXY (void);
void	Map_RegionTallBrush (void);
void	Map_RegionBrush (void);
void	Map_RegionSelectedBrushes (void);
bool	Map_IsBrushFiltered (brush_t *b);

void	Map_SaveSelected(CMemFile* pMemFile, CMemFile* pPatchFile = NULL);
void	Map_ImportBuffer (char* buf, bool renameEntities = true);
int		Map_GetUniqueEntityID(const char *prefix, const char *eclass);

idMapPrimitive *BrushToMapPrimitive( const brush_t *b, const idVec3 &origin );
