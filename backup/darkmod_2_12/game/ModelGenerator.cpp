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

/*
	ModelGenerator

	Manipulate, combine or generate models at run time.

	Copyright (C) 2010-2011 Tels (Donated to The Dark Mod Team)

TODO: If a material casts a shadow (but is not textures/common/shadow*), but the model
	  should not cast a shadow after combine, then clone the material (keep track of
	  all clone materials, Save/Restore/Destroy them) and set noshadows on the clone,
	  then use it in a new surface.
TODO: Call FinishSurfaces() for all orginal models, then cache their shadow vertexes,
	  and omit FinishSurfaces() on the combined model. Might speed it up a lot.
*/

#include "precompiled.h"
#pragma hdrstop



#include "ModelGenerator.h"

// uncomment to have debug printouts
//#define M_DEBUG 1

// uncomment to get detailed timing info
//#define M_TIMINGS 1

#ifdef M_TIMINGS
static idTimer timer_combinemodels, timer_copymodeldata, timer_finishsurfaces, timer_dupmodel, timer_dupverts, timer_dupindexes;
int model_combines = 0;
#endif

/*
===============
CModelGenerator::CModelGenerator
===============
*/
CModelGenerator::CModelGenerator( void ) {
	m_LODList.Clear();
	m_LODList.SetGranularity(32);

//	gameLocal.Printf("Size of LOD data struct: %i bytes\n", sizeof(lod_data_t));
}

CModelGenerator::~CModelGenerator()
{
	Print();
	Shutdown();
}

/*
================
CModelGenerator::SaveLOD
================
*/
void CModelGenerator::SaveLOD( idSaveGame *savefile, const lod_data_t * m_LOD ) const
{
	// save shared data
	savefile->WriteInt( m_LOD->DistCheckInterval );
	savefile->WriteBool( m_LOD->bDistCheckXYOnly );
	savefile->WriteInt( m_LOD->noshadowsLOD );
	savefile->WriteFloat( m_LOD->fLODFadeOutRange );
	savefile->WriteFloat( m_LOD->fLODFadeInRange );
	savefile->WriteFloat( m_LOD->fLODNormalDistance );

	for (int i = 0; i < LOD_LEVELS; i++)
	{
		savefile->WriteFloat( m_LOD->DistLODSq[i] );
		// only save these if the stage is used
		if ( (i == 0) || m_LOD->DistLODSq[i] > 0.0f)
		{
			savefile->WriteString( m_LOD->ModelLOD[i] );
			savefile->WriteString( m_LOD->SkinLOD[i] );
			savefile->WriteVec3( m_LOD->OffsetLOD[i] );
		}
	}
}

/*
===============
CModelGenerator::Save
===============
*/
void CModelGenerator::Save( idSaveGame *savefile ) const {
	int n = m_LODList.Num();
	savefile->WriteInt(n);
	for (int i = 0; i < n; i++)
	{
		savefile->WriteInt(m_LODList[i].users);
		if (m_LODList[i].users > 0)
		{
			// Write LOD data
			SaveLOD( savefile, m_LODList[i].LODPtr );
		}
	}
//	Print();
}

/*
================
CModelGenerator::RestoreLOD
================
*/
void CModelGenerator::RestoreLOD( idRestoreGame *savefile, lod_data_t * m_LOD )
{
	// restore shared data
	savefile->ReadInt( m_LOD->DistCheckInterval );
	savefile->ReadBool( m_LOD->bDistCheckXYOnly );
	savefile->ReadInt( m_LOD->noshadowsLOD );
	savefile->ReadFloat( m_LOD->fLODFadeOutRange );
	savefile->ReadFloat( m_LOD->fLODFadeInRange );
	savefile->ReadFloat( m_LOD->fLODNormalDistance );

	for (int i = 0; i < LOD_LEVELS; i++)
	{
		savefile->ReadFloat( m_LOD->DistLODSq[i] );
		// only save these if the stage is used
		if ((i == 0) || m_LOD->DistLODSq[i] > 0.0f)
		{
			savefile->ReadString( m_LOD->ModelLOD[i] );
			savefile->ReadString( m_LOD->SkinLOD[i] );
			savefile->ReadVec3( m_LOD->OffsetLOD[i] );
		}
		else
		{
			m_LOD->ModelLOD[i] = "";
			m_LOD->SkinLOD[i] = "";
			m_LOD->OffsetLOD[i] = idVec3(0,0,0);
		}
	}
}

/*
===============
CModelGenerator::Restore
===============
*/
void CModelGenerator::Restore( idRestoreGame *savefile ) {
	m_shadowTexturePrefix = "textures/common/shadow";

	Clear();

	int n;
	savefile->ReadInt(n);
	m_LODList.SetNum(n);
	m_LODList.SetGranularity(32);
	for (int i = 0; i < n; i++)
	{
		savefile->ReadInt(m_LODList[i].users);
		m_LODList[i].LODPtr = NULL;
		if (m_LODList[i].users > 0)
		{
			// Read LOD data
			if ((m_LODList[i].LODPtr = new lod_data_t) == NULL)
			{
				gameLocal.Error("Could not allocate %i bytes for a new LOD struct.\n", int(sizeof(lod_data_t)));
			}
			RestoreLOD( savefile, m_LODList[i].LODPtr );
		}
	}
//	Print();
}

/*
===============
ModelGenerator::Init
===============
*/
void CModelGenerator::Init( void ) {
	m_shadowTexturePrefix = "textures/common/shadow";
}

/*
===============
CModelGenerator::Clear
===============
*/
void CModelGenerator::Clear( void ) {

#ifdef M_DEBUG
	gameLocal.Printf("ModelGenerator::Clear()\n");
#endif
	int n = m_LODList.Num();
	for (int i = 0; i < n; i++)
	{
		if (m_LODList[i].LODPtr)
		{
			delete m_LODList[i].LODPtr;
			m_LODList[i].LODPtr = NULL;
		}
	}
	m_LODList.ClearFree();
#ifdef M_DEBUG
	gameLocal.Printf("ModelGenerator::Clear() done.\n");
#endif
}

/*
===============
CModelGenerator::Shutdown
===============
*/
void CModelGenerator::Shutdown( void ) {
	//Print();
	Clear();
}

/*
* Compares two LOD structs and returns true if their contents are equal.
*/
bool CModelGenerator::CompareLODData( const lod_data_t *mLOD, const lod_data_t *mLOD2 ) const {

	if (!mLOD || !mLOD2)
	{
		return false;
	}
	if ( (mLOD->DistCheckInterval	!= mLOD2->DistCheckInterval) ||
		 (mLOD->bDistCheckXYOnly	!= mLOD2->bDistCheckXYOnly) ||
		 (mLOD->noshadowsLOD		!= mLOD2->noshadowsLOD) ||
		 (mLOD->fLODFadeOutRange	!= mLOD2->fLODFadeOutRange) ||
		 (mLOD->fLODFadeInRange		!= mLOD2->fLODFadeInRange) ||
		 (mLOD->fLODNormalDistance	!= mLOD2->fLODNormalDistance) 
	   )
	{
		// something is not equal
		return false;
	}

	for (int i = 0; i < LOD_LEVELS; i++)
	{
		if ( (mLOD->ModelLOD[i]		!= mLOD2->ModelLOD[i]) ||
			 (mLOD->SkinLOD[i]		!= mLOD2->SkinLOD[i]) ||
			 (mLOD->OffsetLOD[i]	!= mLOD2->OffsetLOD[i]) ||
			 (mLOD->DistLODSq[i]	!= mLOD2->DistLODSq[i])
		   )
		{
			// something is not equal
			return false;
		}
	}

	return true;
}

/*
* Presents the ModelManager with the LOD data for this entity, and returns
* a handle (handle >= 0) with that the entity can later access this data. 
*/
lod_handle CModelGenerator::RegisterLODData( const lod_data_t *mLOD ) {

	int n = m_LODList.Num();
	int smallestFree = n;						// default is no free entries at all
	// compare to each of our own used entries
	for (int i = 0; i < n; i++)
	{
		if (m_LODList[i].users > 0 && m_LODList[i].LODPtr)
		{
			if (CompareLODData( m_LODList[i].LODPtr, mLOD ))
			{
				// found an equal entry
				m_LODList[i].users ++;
				// return the index as handle
				return i + 1;
			}
		}
		else
		{
			// remember this free entry if it is the smallest free one
			if (i < smallestFree) { smallestFree = i; }
		}
	}

	// no equal entry, add a new one at position smallestFree
	if (smallestFree >= m_LODList.Num())
	{
		m_LODList.AssureSize(smallestFree + 1);
	}

	m_LODList[smallestFree].users = 1;
	m_LODList[smallestFree].LODPtr = new lod_data_t;

	if (m_LODList[smallestFree].LODPtr == NULL)
	{
		gameLocal.Error("Could not allocate %i bytes for a new LOD struct.\n", int(sizeof(lod_data_t)));
	}
#ifdef M_DEBUG
		gameLocal.Printf("DistCheckInterval %i fLODNormalDistance %0.02f\n",
					mLOD->DistCheckInterval, mLOD->fLODNormalDistance );
#endif
	// copy data
	lod_data_t *l = m_LODList[smallestFree].LODPtr;
	l->DistCheckInterval		= mLOD->DistCheckInterval;
	l->bDistCheckXYOnly			= mLOD->bDistCheckXYOnly;
	l->fLODFadeOutRange			= mLOD->fLODFadeOutRange;
	l->fLODFadeInRange			= mLOD->fLODFadeInRange;
	l->fLODNormalDistance		= mLOD->fLODNormalDistance;
	l->noshadowsLOD				= mLOD->noshadowsLOD;
	for (int i = 0; i < LOD_LEVELS; i++)
	{
		l->ModelLOD[i] = mLOD->ModelLOD[i];
		l->SkinLOD[i] = mLOD->SkinLOD[i];
		l->OffsetLOD[i] = mLOD->OffsetLOD[i];
		l->DistLODSq[i] = mLOD->DistLODSq[i];
	}
	
#ifdef M_DEBUG
	gameLocal.Printf("ModelGenerator: Registered LOD handle %i, n = %i\n", smallestFree + 1, m_LODList.Num());
	gameLocal.Printf("ModelGenerator: Model %s, dist %0.2f.\n", l->ModelLOD[0].c_str(), l->DistLODSq[0] );
	// report memory usage
	Print();
#endif

	return (lod_handle) (smallestFree + 1);
}

/*
* Asks the ModelManager to register a new user for LOD data with the given handle,
* and returns the same handle, or -1 for error.
*/
lod_handle CModelGenerator::RegisterLODData( const lod_handle handle ) {
	int n = m_LODList.Num();

	if (handle == 0 || handle > (unsigned int)n)
	{
		// handle out of range
		gameLocal.Error("ModelGenerator::RegisterLODData: Handle %i out of range 1..%i.", handle, n);
		return static_cast<lod_handle>(-1);
	}

	int h = handle - 1;
	if (m_LODList[h].users <= 0)
	{
		// not registered
		gameLocal.Error("ModelGenerator::RegisterLODData: LOD data %i has no users.", handle);
		return static_cast<lod_handle>(-1);
	}
	m_LODList[h].users ++;

	return handle;
}

/*
* Unregister the LOD data for this handle (returned by RegisterLODData).
* Returns true for success, and false for "not found".
*/
bool CModelGenerator::UnregisterLODData( const lod_handle handle )
{
	int n = m_LODList.Num();

//	gameLocal.Printf("ModelGenerator::UnregisterLODData: Handle %i, n=%i.\n", handle,n);

	if (handle == 0 || handle > (unsigned int)n)
	{
		// handle out of range
		gameLocal.Warning("ModelGenerator::UnregisterLODData: Handle %i out of range 1..%i.", handle, n);
		return false;
	}

	int h = handle - 1;
	if (m_LODList[h].users <= 0)
	{
		// not registered
		gameLocal.Warning("ModelGenerator::UnregisterLODData: LOD data %i has no users.", handle);
		return false;
	}

	// decrement user count
	m_LODList[h].users --;
	
	if (m_LODList[h].users == 0)
	{
		// free memory
#ifdef M_DEBUG
		gameLocal.Printf("Freed LOD memory for handle %i, users %i.\n", handle, m_LODList[h].users);
#endif
		delete m_LODList[h].LODPtr;
		m_LODList[h].LODPtr = NULL;
	}

	// TODO: If we freed the last entry, remove it from the list to save memory?

	return true;
}

/*
* Get a pointer to the LOD data for this handle.
*/
const lod_data_t* CModelGenerator::GetLODDataPtr( const lod_handle handle ) const
{
	int n = m_LODList.Num();

	if (handle == 0 || handle > (unsigned int)n)
	{
		// handle out of range
		gameLocal.Error("ModelGenerator::GetLODDataPtr: Handle %i out of range (1..%i).", handle, n);
		return NULL;
	}

	int h = handle - 1;
	if (m_LODList[h].users <= 0)
	{
		// not registered
		gameLocal.Error("ModelGenerator::GetLODDataPtr: LOD data %i has no users.", handle);
		return NULL;
	}
	return m_LODList[h].LODPtr;
}

/*
* Print memory usage.
*/
void CModelGenerator::Print( void ) const {

	int memory = static_cast<int>(m_LODList.MemoryUsed());
	int memory_saved = 0;
	int users = 0;

	int n = m_LODList.Num();

	if (n == 0)
	{
		gameLocal.Printf("ModelGenerator memory: No LOD entries.\n" );
		return;
	}
	for (int i = 0; i < n; i++)
	{
#ifdef M_DEBUG
		gameLocal.Printf(" LOD %i: users %i data %p\n", i, m_LODList[i].users, m_LODList[i].LODPtr );
#endif
		if (m_LODList[i].users > 0 && m_LODList[i].LODPtr)
		{
			// the struct itself
			int this_memory = (int)sizeof(lod_data_t);
			lod_data_t *l = m_LODList[i].LODPtr;
#ifdef M_DEBUG
			gameLocal.Printf(" DistCheckInterval %i bDistCheckXYOnly %s noshadowsLOD %04x fLODFadeOutRange %0.02f fLODFadeInRange %0.02f fLODNormalDistance %0.02f\n",
				l->DistCheckInterval,
				l->bDistCheckXYOnly ? "yes" : "no",
				l->noshadowsLOD,
				l->fLODFadeOutRange,
				l->fLODFadeInRange,
				l->fLODNormalDistance );
#endif
			for (int j = 0; j < LOD_LEVELS; j++)
			{
#ifdef M_DEBUG
				gameLocal.Printf(" LOD %i Stage %i: model %s skin %s dist %0.2f offset %s\n", 
						i, j, l->ModelLOD[j].c_str(), l->SkinLOD[j].c_str(), l->DistLODSq[j], l->OffsetLOD[j].ToString() );
#endif
				this_memory += l->ModelLOD[j].Length() + 1;
				this_memory += l->SkinLOD[j].Length() + 1;
			}
			memory += this_memory;
			memory_saved += this_memory * (m_LODList[i].users - 1);
			users += m_LODList[i].users;
		}
	}
	if (memory_saved > 0)
	{
		gameLocal.Printf("ModelGenerator memory: %i LOD entries with %d users using %d bytes, memory saved: %d bytes.\n", n, users, memory, memory_saved);
	}
	else
	{
		gameLocal.Printf("ModelGenerator memory: %i LOD entries with %d users using %d bytes.\n", n, users, memory);
	}
}

/* Given a rendermodel and a surface index, checks if that surface is two-sided, and if, tries
   to find the backside for this surface, e.g. the surface which was copied and flipped. Returns
   either the surface index number, or -1 for "not twosided or not found":
*/
int CModelGenerator::GetBacksideForSurface( const idRenderModel * source, const int surfaceIdx ) const {
	const modelSurface_t *firstSurf;
	const idMaterial *firstShader;

	// no source model?
	if (!source) { return -1; }
		
	int numSurfaces = source->NumSurfaces();

	if ( surfaceIdx >= numSurfaces || surfaceIdx < 0)
	{
		// surfaceIdx must be valid
		return -1;
	}

	firstSurf = source->Surface( surfaceIdx );
	if (!firstSurf) { return -1; }
	firstShader = firstSurf->material;
	if (!firstShader) { return -1; }

	// If this is the last surface, it cannot have a flipped backside, because that
	// should come after it. We will fall through the loop and return -1 at the end:

	// Run through all surfaces, starting with the one we have + 1
	for (int s = surfaceIdx + 1; s < numSurfaces; s++)
	{
		// skip self
//		if (s == surfaceIdx) { continue; }	// can't happen here as s > surfaceIdx

		// get each surface
		const modelSurface_t *surf = source->Surface( s );

		// if the original creates backsides, the clone must do so, too, because it uses the same shader
		if (!surf || !surf->material->ShouldCreateBackSides()) { continue; }

		// check if they have the same shader
		if (surf->material == firstShader)
		{
			// same shader, but has it the same size?
			if (surf->geometry && firstSurf->geometry && 
					surf->geometry->numIndexes == firstSurf->geometry->numIndexes &&
					surf->geometry->numVerts   == firstSurf->geometry->numVerts)
			{
				// TODO: more tests to see that this is the real flipped surface and not just a double material?
				// found it
				return s;
			}
		}
	}
	// not found
	return -1;
}

/* Given a rendermodel, returns true if this model has any surface that would cast a shadow.
*/
bool CModelGenerator::ModelHasShadow( const idRenderModel * source ) const {

	// no source model?
	if (!source) { return false; }
		
	int numSurfaces = source->NumSurfaces();

	// Run through all surfaces
	for (int s = 0; s < numSurfaces; s++)
	{
		// get each surface
		const modelSurface_t *surf = source->Surface( s );
		if (!surf) { continue; }

		const idMaterial *shader = surf->material;
		if ( shader->SurfaceCastsShadow() )
		{
			// found at least one surface casting a shadow
			return true;
		}
	}
	return false;
}

/*
===============
CModelGenerator::DuplicateModel - Duplicate a render model

If given a target model, will replace the contents of this model, otherwise allocate a new model.

Returns the given (or allocated) model.

===============
*/
idRenderModel* CModelGenerator::DuplicateModel (const idRenderModel* source, const char* snapshotName, idRenderModel* hModel, const idVec3 *scale, const bool noshadow) const {

	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf;
	modelSurface_s newSurf;
	bool needScale = false;
	idList< bool > backsides;

#ifdef M_TIMINGS
	timer_dupmodel.Start();
#endif

	if (NULL == source)
	{
		gameLocal.Error("ModelGenerator: Dup with NULL source model (snapshotName = %s).\n", snapshotName);
	}

	// allocate memory for the model?
	if (NULL == hModel)
	{
		hModel = renderModelManager->AllocModel();
		if (NULL == hModel)
		{
			gameLocal.Error("ModelGenerator: Could not allocate new model.\n");
		}
	}

	// and init it as dynamic empty model
	hModel->InitEmpty( snapshotName );

	bool needFinish = false;
	// if !noshadow & this model has a shadow casting surface, we need to call FinishSurfaces():
	if (!noshadow && ModelHasShadow( source ))
	{
		needFinish = true;
	}

	numVerts = 0;
	numIndexes = 0;
	numSurfaces = source->NumSurfaces();

	// scale matrix
	if (scale && (scale->x != 1.0f || scale->y != 1.0f || scale->z != 1.0f))
	{
		needScale = true;
	}

	gameLocal.Warning("Source with %i surfaces. snapshot %s, scaling: %s, needFinish: %s", numSurfaces, snapshotName, needScale ? "yes" : "no", needFinish ? "yes" : "no");
#ifdef M_DEBUG
#endif

	backsides.Clear();

	// If we need to call FinishSurfaces(), we need to skip all backsides. So lets
	// find out which surfaces are cloned frontsides:
	if (needFinish)
	{
		backsides.SetNum(numSurfaces);
		for (int i = 0; i < numSurfaces; i++)
		{
			backsides[i] = false;
		}
		// for each needed surface
		for (int i = 0; i < numSurfaces; i++)
		{
			// find its backside, if it has one
	
			// if this surface is already marked as backside, skip this test
			if (backsides[i]) { continue; }
			int backsideIdx = GetBacksideForSurface( source, i );
			if (backsideIdx > 0 && backsideIdx < numSurfaces)
			{
				backsides[backsideIdx] = true;
			}
		}
	}

	// Now that we know which surface is a backside, we can copy all (or only the frontsides)
	// for each needed surface
	for (int i = 0; i < numSurfaces; i++)
	{
		//gameLocal.Warning("Duplicating surface %i.", i);
		surf = source->Surface( i );

		// if we need to call FinishSurface() and this is a backside, skip it
		if (!surf || (needFinish && backsides[i]))
		{
			continue;
		}

		//gameLocal.Printf("numSilEdges %i silEdges %p\n", surf->geometry->numSilEdges, surf->geometry->silEdges);

		// If we don't need shadows, and this is a pure shadow caster (e.g. otherwise invisible)
		// then skip it. Can only happen if noshadows = true:
		if (!needFinish && surf->material->SurfaceCastsShadow())
		{
			idStr shaderName = surf->material->GetName();
			if (shaderName.Left( m_shadowTexturePrefix.Length() ) == m_shadowTexturePrefix )
			{
				continue;
			}
		}

		numVerts += surf->geometry->numVerts; 
		numIndexes += surf->geometry->numIndexes;

		// copy the material
		newSurf.material = surf->material;
		//gameLocal.Warning("Duplicating %i verts and %i indexes.", surf->geometry->numVerts, surf->geometry->numIndexes );

		newSurf.geometry = hModel->AllocSurfaceTriangles( numVerts, numIndexes );

		int nV = 0;		// vertexes
		int nI = 0;		// indexes

		if (needScale)
		{
			// a direct copy with scaling
			for (int j = 0; j < surf->geometry->numVerts; j++)
			{
				idDrawVert *v = &(newSurf.geometry->verts[nV]);
				newSurf.geometry->verts[nV++] = surf->geometry->verts[j];
				v->xyz.MulCW(*scale);
				v->normal.DivCW(*scale);
				v->tangents[0].DivCW(*scale);
				v->tangents[1].DivCW(*scale);
				v->ScaleToUnitNormal();
			}
		}
		else
		{
			// just one direct copy
			for (int j = 0; j < surf->geometry->numVerts; j++)
			{
				newSurf.geometry->verts[nV++] = surf->geometry->verts[j];
			}
		}

		// copy indexes
		glIndex_t *dst = newSurf.geometry->indexes;
		glIndex_t *src = surf->geometry->indexes;
		int imax = surf->geometry->numIndexes;
		// we are dealing with triangles, so each tris has 3 indexes
		assert( (imax % 3) == 0);
		// unrolled 3 loops into one
		for (int j = 0; j < imax; j++)
		{
			dst[nI ++] = src[j]; j++;
			dst[nI ++] = src[j]; j++;
			dst[nI ++] = src[j];
		}

		// set these so they don't get recalculated in FinishSurfaces():
		newSurf.geometry->tangentsCalculated = true;
		newSurf.geometry->facePlanesCalculated = false;
		newSurf.geometry->generateNormals = false;
		newSurf.geometry->numVerts = nV;
		newSurf.geometry->numIndexes = nI;
		// just copy bounds
		newSurf.geometry->bounds[0] = surf->geometry->bounds[0];
		newSurf.geometry->bounds[1] = surf->geometry->bounds[1];
		newSurf.id = 0;
		hModel->AddSurface( newSurf );
	}

	// generate shadow hull as well as backsides for twosided materials
	if (needFinish)
	{
#ifdef M_DEBUG
		gameLocal.Printf("Calling FinishSurfaces().\n");
#endif
		hModel->FinishSurfaces();
	}

#ifdef M_DEBUG
	hModel->Print();
#endif

#ifdef M_TIMINGS
	timer_dupmodel.Stop();
	gameLocal.Printf( "ModelGenerator: dupmodel %0.2f ms\n", timer_dupmodel.Milliseconds() );
#endif

	return hModel;
}

/*
===============
CModelGenerator::DuplicateLODModels - Duplicate a render model based on LOD stages.

If the given list of model_ofs_t is filled, the model will be copied X times, each time
offset and rotated by the given values, scaled, and also fills in the right vertex color.

The models in LODs do not have to be LOD stages, they can be anything as long as for
each offset the corrosponding entry in LOD exists.
===============
*/
idRenderModel * CModelGenerator::DuplicateLODModels (const idList<const idRenderModel*> *LODs,
			const char* snapshotName, const idList<model_ofs_t> *offsets, const idVec3 *origin,
			const idMaterial *shader, idRenderModel* hModel ) const {
	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf;
	modelSurface_t *newSurf;
	bool needFinish;				// do we need to call FinishSurfaces() at the end?

	// info about each model (how often used, how often shadow casting, which surfaces to copy where)
	idList<model_stage_info_t> modelStages;
	model_stage_info_t *modelStage;	// ptr to current model stage

	const model_ofs_t *op;

	idList< model_target_surf > targetSurfInfo;
	model_target_surf newTargetSurfInfo;
	model_target_surf* newTargetSurfInfoPtr;

#ifdef M_TIMINGS
	timer_combinemodels.Start();
	model_combines ++;
#endif

	// allocate memory for the model
	if (NULL == hModel)
	{
		hModel = renderModelManager->AllocModel();
		if (NULL == hModel)
		{
			gameLocal.Error("ModelGenerator: Could not allocate new model.\n");
		}
	}
	// and init it as dynamic empty model
	hModel->InitEmpty( snapshotName );

	// count the tris overall
	numIndexes = 0; numVerts = 0;

	if (NULL == offsets)
	{
		gameLocal.Error("NULL offsets in DuplicateLODModels");
	}

	int nSources = LODs->Num();
	if (nSources == 0)
	{
		gameLocal.Error("No LOD models DuplicateLODModels");
	}
	//gameLocal.Warning("Have %i LOD stages.", nSources);

	// Stages of our plan:

	/* ** 1 ** 
	* First check for each potentially-used model if it has a shadow-casting surface
	*	O(N*M) - N is number of stages/models 						 (usually 7 or less)
	*			 M is number of surfaces (on average) for each stage (usually 1..3)
	* At this point we only know that we don't need to call FinishSurfaces() if there
	* are no shadow-casting models at all, but if there, we are still not sure because
	* it can turn out the shadow-casting ones aren't actually used. So:
	*/

	/* ** 2 **
	* Then check for each offset which model stage it uses, and count them. If one of the
	* offsets uses a model with a shadow casting surface, we need to call FinishSurfaces().
	* Also correct out-of-bounds entries here.
	*		O(N) where N the number of offsets.
	*/

	/* ** 3 **
	* Now that we know whether we need FinishSurfaces(), we also know whether we
	* need to skip backsides, or not. So lets map out which surface of which stage
	* gets copied to what surface of the final model.
	*	O(N*M) - N is number of stages/models 						 (usually 7 or less)
	*			 M is number of surfaces (on average) for each stage (usually 1..3)
	*/

	/* ** 4 **
	* Allocate memory for all nec. surfaces.
	*/

	/* ** 5 **
	* Finally we can copy the data together, scaling/rotating/translating models as
	* we go.
	*	O(V) - V is number of tris we need to copy
	*/


	/* Let the play begin: */
	modelStages.SetNum(nSources);
	needFinish = false;				// do we need to call FinishSurfaces() at the end?

	/* ** Stage 1 ** */
	// For each model count how many shadow casting surfaces it has
	// And init modelStages, too.

#ifdef M_DEBUG
	gameLocal.Printf("Stage #1\n");
#endif

	// TODO: If this prove to be an expensive step (which I doubt), then we could
	//		 cache the result, because the ptr to the renderModel should be stable.
	for (int i = 0; i < nSources; i++)
	{
		modelStage = &modelStages[i];

		// init fields
		modelStage->usedShadowless = 0;
		modelStage->usedShadowing = 0;
		modelStage->couldCastShadow = false;
		modelStage->noshadowSurfaces.Clear();
		modelStage->shadowSurfaces.Clear();
		modelStage->surface_info.Clear();

		if (NULL == (modelStage->source = LODs->Ptr()[i]))
		{
			// Use the default model
			if (NULL == (modelStage->source = LODs->Ptr()[0]))
			{
				gameLocal.Warning("NULL source ptr for default LOD stage (was default for %i)", i);
				// cannot use this
				continue;
			}
		}
#ifdef M_DEBUG
		// print the source model
		//modelStage->source->Print();
#endif
		if (modelStage->source->NumSurfaces() == 0)
		{
			// ignore empty models
			modelStage->source = NULL;
			continue;
		}

		// false if there are no shadow-casting surfaces at all
		modelStage->couldCastShadow = ModelHasShadow( modelStage->source );
#ifdef M_DEBUG
		gameLocal.Printf("This stage could cast shadow: %s\n", modelStage->couldCastShadow ? "yes" : "no" );
#endif
	}

#ifdef M_DEBUG
	gameLocal.Printf("Stage #2\n");
#endif

	/* ** Stage 2 ** */
	// For each offset, count what stages it uses (and with or without shadow) and also
	// correct missing (LOD) models
	const model_ofs_t *OffsetsPtr = offsets->Ptr();

	int numOffsets = offsets->Num();
	for (int i = 0; i < numOffsets; i++)
	{
		op = &OffsetsPtr[i];
		// correct entries that are out-of-bounds
		int lod = op->lod;
		if (lod >= nSources) { lod = nSources - 1; }
		// otherwise invisible
		if (lod < 0)
		{
			continue;
		}
		// uses stage op->lod
		modelStage = &modelStages[lod];

		// the stage is used with shadow only if 
		// BOTH the model could cast a shadow and the offset says it wants a shadow
		if ( modelStage->couldCastShadow && !(op->flags & SEED_MODEL_NOSHADOW) )
		{
			modelStage->usedShadowing ++;
			// can and could cast a shadow, so we need FinishSurface()
			needFinish = true;
		}
		else
		{
			modelStage->usedShadowless ++;
		}
	}

#ifdef M_DEBUG
	gameLocal.Printf("Stage #3\n");
#endif

	/* ** Stage 3 ** */
	// If we have multiple models with different surfaces, then we need to combine
	// all surfaces from all models, so first we need to build a list of needed surfaces
	// by shader name. This ensures that if LOD 0 has A and B and LOD 1 has B and C, we
	// create only three surfaces A, B and C, and not four (A, B.1, B.2 and C). This will
	// help reducing drawcalls:
	targetSurfInfo.Clear();

	for (int i = 0; i < nSources; i++)
	{
		modelStage = &modelStages[i];

		int modelUsageCount = modelStage->usedShadowless + modelStage->usedShadowing;
		// not used at all?
		if ( (0 == modelUsageCount) || (NULL == modelStage->source) )
		{
			// skip
#ifdef M_DEBUG
			gameLocal.Printf("Stage #%i is not used at all, skipping it.\n", i);
#endif
			continue;
		}
		const idRenderModel* source = modelStage->source;

		// get the number of all surfaces
		numSurfaces = source->NumSurfaces();

#ifdef M_DEBUG
		gameLocal.Printf("At stage #%i with %i surfaces, used %i times (%i shadow, %i noshadow).\n", i, numSurfaces, modelUsageCount, modelStage->usedShadowing, modelStage->usedShadowless);
#endif

		// If we have not yet filled this array, so do it now to avoid costly rechecks:
		if (modelStage->surface_info.Num() == 0)
		{
			// Do this as extra step before, so it is completely filled before we go
			// through the surfaces and decide whether to keep them or not:
			modelStage->surface_info.SetNum( numSurfaces );
			modelStage->noshadowSurfaces.SetNum( numSurfaces );
			modelStage->shadowSurfaces.SetNum( numSurfaces );
			// init to 0
			for (int s = 0; s < numSurfaces; s++)
			{
				modelStage->surface_info[s] = 0;
				modelStage->noshadowSurfaces[s] = -1;	// default: skip
				modelStage->shadowSurfaces[s] = -1;		// default: skip
			}

			for (int s = 0; s < numSurfaces; s++)
			{
				surf = source->Surface( s );
				if (!surf) { continue; }
				const idMaterial *curShader = surf->material;

				// Surfaces that have the backside bit already set are not considered here
				// or we would find maybe their source by accident (we only want to skip
				// the backside, not the frontside)
				if ((modelStage->surface_info[s] & 1) == 0)
				{
					int backside = GetBacksideForSurface( modelStage->source, s );
					// -1 not found, 0 can't happen as we start with 0 and 0 can't be its own backside
					if (backside > 0)
					{
#ifdef M_DEBUG
						gameLocal.Printf("Surface #%i is a backside for surface #%i.\n", backside, s);
#endif
						// set bit 0 to true, so we know this is a backside
						modelStage->surface_info[backside] |= 0x1;	// 0b0001
					}
				}
				if (curShader->SurfaceCastsShadow())
				{
					// is this is a pure shadow casting surface?
					idStr shaderName = curShader->GetName();
					if (shaderName.Left( m_shadowTexturePrefix.Length() ) == m_shadowTexturePrefix )
					{
						// yes, mark it
						modelStage->surface_info[s] |= 0x2;			// 0b0010
					}
				}
			}
		}

#ifdef M_DEBUG
		gameLocal.Printf("Run through all %i surfaces\n", numSurfaces);
#endif
		// Run through all surfaces
		for (int s = 0; s < numSurfaces; s++)
		{
			// get each surface
			surf = source->Surface( s );
			if (!surf) { continue; }

			const idMaterial *curShader = surf->material;
			const int flags = modelStage->surface_info[s];
		   
			/* Two cases: 
			*  1: need to call FinishSurfaces() at the end:
			*	  1a: then we need to skip backsides (the clones of two-sided surfaces)
			*	  1b: and we also need to skip pure shadow casting if the model is used with noshadows
			*  2: no need to call FinishSurfaces() at the end:
			*	  2a: need to skip ONLY pure shadow casting surfaces (they probably would not harm, but
			*		  use up memory and time to copy)
			*/
			bool pureShadow = false;

			if (needFinish)
			{
				/* case 1a */
				if ((flags & 0x1) != 0)
				{
#ifdef M_DEBUG
					gameLocal.Printf("Surface #%i is backside, skipping since we need FinishSurfaces().\n", s);
#endif
					continue;
				}
				/* case 1b */
				if ((flags & 0x2) != 0)
				{
					pureShadow = true;
				}
			}
			else
			{
				/* case 2a: need to skip ONLY pure shadow casting surfaces */
				if ((flags & 0x2) != 0)
				{
#ifdef M_DEBUG
					gameLocal.Printf("Surface #%i is pure shadow caster, skipping it.\n", s);
#endif
					// this is a pure shadow casting surface
					continue;
				}
			}

			// Now we know that we need the surface at all, and if we need it in the case of "shadow"
			// Decide to which target surface we need to append

			// TODO: Backside surfaces get automatically added to the frontside, too. Is this ok?

			// Do we have already a surface with that shader?
			// The linear search here is ok, since most models have only a few surfaces, since every
			// surface creates one expensive draw call.
			// if given a shader, use this instead, so everyting will be in one surface:
			idStr n = shader ? shader->GetName() : curShader->GetName();
			int found = -1;
			for (int j = 0; j < targetSurfInfo.Num(); j++)
			{
				if (targetSurfInfo[j].surf.material->GetName() == n)
				{
					found = j;
					break;
				}
			}
			if (found == -1)
			{
				newTargetSurfInfo.numVerts = 0;
				newTargetSurfInfo.numIndexes = 0;
				newTargetSurfInfo.surf.geometry = NULL;
				// if given a shader, use this instead.
				newTargetSurfInfo.surf.material = shader ? shader : curShader;
				newTargetSurfInfo.surf.id = 0;
				targetSurfInfo.Append( newTargetSurfInfo );
#ifdef M_DEBUG	
				gameLocal.Warning("ModelGenerator: Need shader %s.", n.c_str() );
#endif
				found = targetSurfInfo.Num() - 1;
			}

			// Increase the nec. counts, if the stage is used X times with shadows and
			// Y times without, the surface needs to be copied X+Y times:
			int count = modelUsageCount;
			if (pureShadow)
			{
				// only count it for shadow models usage, e.g. skip it for non-shadow cases
				count = modelStage->usedShadowing;
			}
			else
			{
				// in case of noshadow, include it since it is not a pure shadow
				modelStage->noshadowSurfaces[s] = found;
				// modelStage->shadowSurfaces[s] stays as -1
			}
			// include everything in the shadow case
			modelStage->shadowSurfaces[s] = found;
			targetSurfInfo[found].numVerts += count * surf->geometry->numVerts;
			targetSurfInfo[found].numIndexes += count * surf->geometry->numIndexes;
		}
	}

	if (targetSurfInfo.Num() == 0)
	{
		// TODO: this can happen if the model contains no visible LOD stages, but wasn't
		//		 culled or made invisible yet. So allow it.
		gameLocal.Error("Dup model (%i LOD stages) with no surfaces at all.\n", nSources);
	}

#ifdef M_DEBUG	
	gameLocal.Printf("ModelGenerator: Need %i surfaces on the final model.\n", targetSurfInfo.Num() );
#endif

#ifdef M_DEBUG
	gameLocal.Printf("Stage #4\n");
#endif

	/* ** Stage 4 - allocate surface memory */
	for (int j = 0; j < targetSurfInfo.Num(); j++)
	{
#ifdef M_DEBUG	
		// debug: print info for each new surface
		gameLocal.Printf(" Allocating for surface %i: %s numVerts %i numIndexes %i\n", 
			j, targetSurfInfo[j].surf.shader->GetName(), targetSurfInfo[j].numVerts, targetSurfInfo[j].numIndexes );
#endif

		targetSurfInfo[j].surf.geometry = hModel->AllocSurfaceTriangles( targetSurfInfo[j].numVerts, targetSurfInfo[j].numIndexes );
		targetSurfInfo[j].surf.geometry->numVerts = targetSurfInfo[j].numVerts;
		targetSurfInfo[j].surf.geometry->numIndexes = targetSurfInfo[j].numIndexes;
		// use these now to track how much we already copied
		targetSurfInfo[j].numVerts = 0;
		targetSurfInfo[j].numIndexes = 0;
	}

#ifdef M_DEBUG
	gameLocal.Printf("Stage #5\n");
#endif

#ifdef M_TIMINGS
	timer_copymodeldata.Start();
#endif

	/* ** Stage 5 - now combine everything into one model */
	// for each offset
	for (int o = 0; o < numOffsets; o++)
	{
		op = &OffsetsPtr[o];
		// should be invisible, so skip
		int lod = op->lod;
		if (lod < 0)
		{
			continue;
		}
		if (lod >= nSources) { lod = nSources - 1; }

		// precompute these (stgatilov):
		// scale matrix
		idMat3 mScale(op->scale.x, 0, 0, 
					  0, op->scale.y, 0, 
					  0, 0, op->scale.z);
		// rotate matrix
		idMat3 mRot = op->angles.ToRotation().ToMat3();
		// direction transformation = scale + rotate
		idMat3 tDir = mScale * mRot;
		// normal transformation = (scale + rotate) inverse transpose
		idMat3 tNorm = tDir.Inverse().Transpose();
		// position transformation = scale + rotate + translate
		idMat4 tPos = idMat4(tDir, op->offset);

		modelStage = &modelStages[lod];

		const idRenderModel* source = modelStage->source;
		const bool noShadow = (op->flags & SEED_MODEL_NOSHADOW);

		// gameLocal.Warning("ModelGenerator: op->lod = %i si = %i", op->lod, si);

		// for each surface of the model
		numSurfaces = source->NumSurfaces();
		for (int s = 0; s < numSurfaces; s++)
		{
			//gameLocal.Warning("At surface %i of stage %i at offset %i", s, lod, o);

			surf = source->Surface( s );
			if (!surf) { continue; }

			// get the target surface
			int st = modelStage->shadowSurfaces[s];
			if (noShadow)
			{
				st = modelStage->noshadowSurfaces[s];
			}

			// -1 => skip this surface
			if (st < 0)
			{
#ifdef M_DEBUG
				gameLocal.Warning("Skipping surface %i.", s);
#endif
				continue;
			}

			newTargetSurfInfoPtr = &targetSurfInfo[st];

			if (!newTargetSurfInfoPtr)
			{
				gameLocal.Warning("newTargetSurfInfoPtr = NULL");
				continue;
			}

			// shortcut
			newSurf = &newTargetSurfInfoPtr->surf;

			int nV = newTargetSurfInfoPtr->numVerts;
			int nI = newTargetSurfInfoPtr->numIndexes;
			int vmax = surf->geometry->numVerts;

			dword newColor = op->color; 

#ifdef M_TIMINGS
			timer_dupverts.Start();
#endif
			idDrawVert *dst_v = newSurf->geometry->verts;
			idDrawVert *src_v = surf->geometry->verts;

			// copy the vertexes and modify them at the same time (scale, rotate, offset)
			for (int j = 0; j < vmax; j++)
			{
				// target
				idDrawVert *v = &dst_v[nV];
				// source
				idDrawVert *vs = &src_v[j];

				// stgatilov: fast and proper transformation
				v->xyz = tPos * vs->xyz;
				v->normal = vs->normal * tNorm;
				v->tangents[0] = vs->tangents[0] * tNorm;
				v->tangents[1] = vs->tangents[1] * tNorm;
				v->st = vs->st;
				v->SetColor(newColor);
				// some normalization
				v->ScaleToUnitNormal();
				//NOTE (in case of non-isotropic scaling):
				//if tangents are normalized then bumpmapped surface will look different

/*				if (o == 1 || o == 2)
				{
					gameLocal.Printf ("Was Vert %d (%d): xyz (%s) st (%s) tangent (%s) (%s) normal (%s) color %d %d %d %d.\n",
						j, nV, vs->xyz.ToString(), vs->st.ToString(),
						vs->tangents[0].ToString(), vs->tangents[1].ToString(), vs->normal.ToString(),
						int(vs->color[0]),
						int(vs->color[1]),
						int(vs->color[2]),
						int(vs->color[3])
						);
					gameLocal.Printf ("Now Vert %d (%d): xyz (%s) st (%s) tangent (%s) (%s) normal (%s) color %d %d %d %d.\n",
						j, nV, v->xyz.ToString(), v->st.ToString(), 
						v->tangents[0].ToString(), v->tangents[1].ToString(), v->normal.ToString(),
						int(v->color[0]),
						int(v->color[1]),
						int(v->color[2]),
						int(v->color[3])
						);
				}*/
				nV ++;

			}	// end of all verts
#ifdef M_TIMINGS
			timer_dupverts.Stop();
			timer_dupindexes.Start();
#endif
			// shortcut, this speeds up the copy process
			glIndex_t *dst = newSurf->geometry->indexes;
			glIndex_t *src = surf->geometry->indexes;

			// copy indexes
			int no = newTargetSurfInfoPtr->numVerts;			// correction factor (before adding nV!)
			int imax = surf->geometry->numIndexes;
			// we are dealing with triangles, so each tris has 3 indexes
			assert( (imax % 3) == 0);
			// unrolled 3 loops into one
			for (int j = 0; j < imax; j++)
			{
			//	newSurf->geometry->indexes[nI ++] = surf->geometry->indexes[j] + no;
				dst[nI++] = src[j] + no; j++;
				dst[nI++] = src[j] + no; j++;
				dst[nI++] = src[j] + no;
			}
#ifdef M_TIMINGS
			timer_dupindexes.Stop();
#endif
			newTargetSurfInfoPtr->numVerts += vmax;
			newTargetSurfInfoPtr->numIndexes += imax;
		} // end for each surface on this offset
	}	// end for each offset

#ifdef M_TIMINGS
	timer_copymodeldata.Stop();
#endif

	// finish the surfaces
	for (int j = 0; j < targetSurfInfo.Num(); j++)
	{
		newSurf = &targetSurfInfo[j].surf;

#ifdef M_DEBUG
		// sanity check
		if (targetSurfInfo[j].surf.geometry->numVerts != targetSurfInfo[j].numVerts)
		{
			gameLocal.Warning ("ModelGenerator: surface %i differs between allocated (%i) and created numVerts (%i)",
				j, targetSurfInfo[j].surf.geometry->numVerts, targetSurfInfo[j].numVerts );
		}
		if (targetSurfInfo[j].surf.geometry->numIndexes != targetSurfInfo[j].numIndexes)
		{
			gameLocal.Warning ("ModelGenerator: surface %i differs between allocated (%i) and created numIndexes (%i)",
				j, targetSurfInfo[j].surf.geometry->numIndexes, targetSurfInfo[j].numIndexes );
		}
#endif

		newSurf->geometry->tangentsCalculated = true;
		newSurf->geometry->facePlanesCalculated = false;
		newSurf->geometry->generateNormals = false;
		// calculate new bounds
		SIMDProcessor->MinMax( newSurf->geometry->bounds[0], newSurf->geometry->bounds[1], newSurf->geometry->verts, newSurf->geometry->numVerts );
		newSurf->id = 0;
		hModel->AddSurface( targetSurfInfo[j].surf );
		// nec.?
		targetSurfInfo[j].surf.geometry = NULL;
	}

#ifdef M_TIMINGS
	timer_finishsurfaces.Start();
#endif
	if (needFinish)
	{
		// generate shadow hull as well as tris for twosided materials
		hModel->FinishSurfaces();
	}

#ifdef M_TIMINGS
	timer_finishsurfaces.Stop();
	timer_combinemodels.Stop();

	if (model_combines % 10 == 0)
	{
		gameLocal.Printf( "ModelGenerator: combines %i, total time %0.2f ms (for each %0.2f ms), copy data %0.2f ms (for each %0.2f ms), finish surfaces %0.2f ms (for each %0.2f ms)\n",
				model_combines,
				timer_combinemodels.Milliseconds(),
				timer_combinemodels.Milliseconds() / model_combines,
				timer_copymodeldata.Milliseconds(),
				timer_copymodeldata.Milliseconds() / model_combines,
				timer_finishsurfaces.Milliseconds(),
				timer_finishsurfaces.Milliseconds() / model_combines );
		gameLocal.Printf( "ModelGenerator: dup verts total time %0.2f ms (for each %0.2f ms), dup indexes %0.2f ms (for each %0.2f ms)\n",
				timer_dupverts.Milliseconds(),
				timer_dupverts.Milliseconds() / model_combines,
				timer_dupindexes.Milliseconds(),
				timer_dupindexes.Milliseconds() / model_combines );
	}
#endif

#ifdef M_DEBUG
	hModel->Print();
#endif

	return hModel;
}

/*
===============
Returns the maximum number of models that can be combined from this model:
*/
unsigned int CModelGenerator::GetMaxModelCount( const idRenderModel* hModel ) const
{
	const modelSurface_t *surf;

	// compute vertex and index count on this model
	unsigned int numVerts = 0;
	unsigned int numIndexes = 0;

	// get the number of base surfaces (minus decals) on the old model
	int numSurfaces = hModel->NumBaseSurfaces();

	for (int i = 0; i < numSurfaces; i++)
	{
		surf = hModel->Surface( i );
		if (surf)
		{
			numVerts += surf->geometry->numVerts; 
			numIndexes += surf->geometry->numIndexes;
		}
	}

	// avoid divide by zero for empty models
	if (numVerts == 0) { numVerts = 1; }
	if (numIndexes == 0) { numIndexes = 1; }

	int v = MAX_MODEL_VERTS / numVerts;
	int i = MAX_MODEL_INDEXES / numIndexes;

	// minimum of the two
	if (v < i) { i = v; }

	return i;
}


