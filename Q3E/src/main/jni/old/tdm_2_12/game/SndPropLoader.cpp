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
/******************************************************************************/
/*                                                                            */
/*         Dark Mod Sound Propagation (C) by Chris Sarantos in USA 2005		  */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
*
* DESCRIPTION: Sound propagation class for compiling sound propagation data
* from a Mapfile and write it to a file.  Also used to read the file on map init.
*
*****************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"



#pragma warning(disable : 4996)

#include "SndPropLoader.h"
#include "MatrixSq.h"
#include "Misc.h"

class idLocationEntity;

// TODO: Write the mapfile timestamp to the .spr file and compare them

const float s_DOOM_TO_METERS = 0.0254f;					// doom to meters
const float s_METERS_TO_DOOM = (1.0f/DOOM_TO_METERS);	// meters to doom


const float s_DBM_TO_M = 1.0/(10*log10( idMath::E )); // convert between dB/m and 1/m

void SsndPGlobals::Save(idSaveGame *savefile) const
{
	savefile->WriteString(AreaPropName);
	savefile->WriteString(fileExt);
	savefile->WriteInt(MaxPaths);
	savefile->WriteFloat(DoorExpand);
	savefile->WriteFloat(Falloff_Outd);
	savefile->WriteFloat(Falloff_Ind);
	savefile->WriteFloat(kappa0);
	savefile->WriteFloat(DefaultDoorLoss);
	savefile->WriteFloat(DefaultThreshold);
	savefile->WriteFloat(Vol);
	savefile->WriteFloat(MaxRange);
	savefile->WriteFloat(MaxRangeCalVol);
	savefile->WriteFloat(MaxEnvRange);
	savefile->WriteBool(bDebug);
}

void SsndPGlobals::Restore(idRestoreGame *savefile)
{
	savefile->ReadString(AreaPropName);
	savefile->ReadString(fileExt);
	savefile->ReadInt(MaxPaths);
	savefile->ReadFloat(DoorExpand);
	savefile->ReadFloat(Falloff_Outd);
	savefile->ReadFloat(Falloff_Ind);
	savefile->ReadFloat(kappa0);
	savefile->ReadFloat(DefaultDoorLoss);
	savefile->ReadFloat(DefaultThreshold);
	savefile->ReadFloat(Vol);
	savefile->ReadFloat(MaxRange);
	savefile->ReadFloat(MaxRangeCalVol);
	savefile->ReadFloat(MaxEnvRange);
	savefile->ReadBool(bDebug);
}

/*********************************************************
*
*	CsndPropBase Implementation
*
**********************************************************/

void CsndPropBase::Save(idSaveGame *savefile) const
{
	m_SndGlobals.Save(savefile);
	savefile->WriteBool(m_bLoadSuccess);
	savefile->WriteBool(m_bDefaultSpherical);
	savefile->WriteInt(m_numAreas);
	savefile->WriteInt(m_numPortals);

	for (int area = 0; area < m_numAreas; area++)
	{
		savefile->WriteFloat(m_sndAreas[area].LossMult);
		savefile->WriteFloat(m_sndAreas[area].VolMod);
		savefile->WriteInt(m_sndAreas[area].numPortals);
		savefile->WriteVec3(m_sndAreas[area].center);

		for (int portal = 0; portal < m_sndAreas[area].numPortals; portal++)
		{
			SsndPortal_s& soundportal = m_sndAreas[area].portals[portal];

			savefile->WriteInt(soundportal.handle);
			savefile->WriteInt(soundportal.portalNum);
			savefile->WriteInt(soundportal.from);
			savefile->WriteInt(soundportal.to);
			savefile->WriteVec3(soundportal.center);
			savefile->WriteVec3(soundportal.normal);
			// greebo: Don't save winding pointer, gets restored from idRenderWorld.
		}

		m_sndAreas[area].portalDists->Save(savefile);
	}

	savefile->WriteInt(m_AreaPropsG.Num());
	for (int i = 0; i < m_AreaPropsG.Num(); i++)
	{
		savefile->WriteInt(m_AreaPropsG[i].area);
		savefile->WriteFloat(m_AreaPropsG[i].LossMult);
		savefile->WriteFloat(m_AreaPropsG[i].VolMod);
		savefile->WriteBool(m_AreaPropsG[i].DataEntered);
	}
	
	for (int i = 0; i < m_numPortals; i++)
	{
		savefile->WriteInt(m_PortData[i].LocalIndex[0]);
		savefile->WriteInt(m_PortData[i].LocalIndex[1]);
		savefile->WriteInt(m_PortData[i].Areas[0]);
		savefile->WriteInt(m_PortData[i].Areas[1]);
		savefile->WriteFloat(m_PortData[i].lossAI); // grayman #3042
		savefile->WriteFloat(m_PortData[i].lossPlayer); // grayman #3042
	}
}

void CsndPropBase::Restore(idRestoreGame *savefile)
{
	m_SndGlobals.Restore(savefile);

	savefile->ReadBool(m_bLoadSuccess);
	savefile->ReadBool(m_bDefaultSpherical);
	savefile->ReadInt(m_numAreas);
	savefile->ReadInt(m_numPortals);

	m_sndAreas = new SsndArea[m_numAreas];

	for (int area = 0; area < m_numAreas; area++)
	{
		savefile->ReadFloat(m_sndAreas[area].LossMult);
		savefile->ReadFloat(m_sndAreas[area].VolMod);
		savefile->ReadInt(m_sndAreas[area].numPortals);
		savefile->ReadVec3(m_sndAreas[area].center);

		m_sndAreas[area].portals = new SsndPortal[m_sndAreas[area].numPortals];

		for (int portal = 0; portal < m_sndAreas[area].numPortals; portal++)
		{
			SsndPortal_s& soundportal = m_sndAreas[area].portals[portal];

			savefile->ReadInt(soundportal.handle);
			savefile->ReadInt(soundportal.portalNum);
			savefile->ReadInt(soundportal.from);
			savefile->ReadInt(soundportal.to);
			savefile->ReadVec3(soundportal.center);
			savefile->ReadVec3(soundportal.normal);

			// Restore the winding pointer from idRenderWorld
			exitPortal_t p = gameRenderWorld->GetPortal(area, portal);
			soundportal.winding = &p.w;
		}

		// Allocate and resize the triangle matrix
		m_sndAreas[area].portalDists = new CMatRUT<float>;
		m_sndAreas[area].portalDists->Restore(savefile);
	}

	int num;
	savefile->ReadInt(num);
	m_AreaPropsG.Clear();
	m_AreaPropsG.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadInt(m_AreaPropsG[i].area);
		savefile->ReadFloat(m_AreaPropsG[i].LossMult);
		savefile->ReadFloat(m_AreaPropsG[i].VolMod);
		savefile->ReadBool(m_AreaPropsG[i].DataEntered);
	}

	m_PortData = new SPortData[m_numPortals];
	for (int i = 0; i < m_numPortals; i++)
	{
		savefile->ReadInt(m_PortData[i].LocalIndex[0]);
		savefile->ReadInt(m_PortData[i].LocalIndex[1]);
		savefile->ReadInt(m_PortData[i].Areas[0]);
		savefile->ReadInt(m_PortData[i].Areas[1]);
		savefile->ReadFloat(m_PortData[i].lossAI); // grayman #3042
		savefile->ReadFloat(m_PortData[i].lossPlayer); // grayman #3042
	}
}

void CsndPropBase::GlobalsFromDef( void )
{
	const idDict *def;

	def = gameLocal.FindEntityDefDict( "atdm:soundprop_globals", false );

	if(!def)
	{
		gameLocal.Warning("[DarkMod Sound Prop] : Did not find def for atdm:soundprop_globals.  Bad or missing tdm_soundprop.def file.  Using default values.");
		DM_LOG(LC_SOUND, LT_ERROR)LOGSTRING("Did not find def for atdm:soundprop_globals.  Using default values.\r");
		DefaultGlobals();
		goto Quit;
	}

	m_SndGlobals.bDebug = def->GetBool("debug", "0");
	m_SndGlobals.AreaPropName = def->GetString("aprop_name", "");
	m_SndGlobals.fileExt = def->GetString("file_ext", "spr");

	m_SndGlobals.MaxPaths = def->GetInt("maxpaths", "3");
	m_SndGlobals.DoorExpand = def->GetFloat("doorexpand", "1.0");
	m_SndGlobals.Falloff_Outd = def->GetFloat("falloff_outd", "10.0"); // grayman - if implemented, this should be smaller than Falloff_Ind
	m_SndGlobals.Falloff_Ind = def->GetFloat("falloff_ind", "9.0");
	m_SndGlobals.kappa0 = def->GetFloat("kappa_dbm", "0.015");

	m_SndGlobals.DefaultDoorLoss = def->GetFloat("default_doorloss", "20");
	m_SndGlobals.MaxRange = def->GetFloat("maxrange", "2.2");
	m_SndGlobals.MaxRangeCalVol = def->GetFloat("maxrange_cal", "30");
	m_SndGlobals.MaxEnvRange = def->GetFloat("max_envrange", "50");
	
	m_SndGlobals.Vol = def->GetFloat("vol_ai", "0.0");
	m_SndGlobals.DefaultThreshold = def->GetFloat("default_thresh", "20.0");

Quit:
	return;
}

void CsndPropBase::DefaultGlobals( void )
{
	m_SndGlobals.bDebug = false;
	m_SndGlobals.AreaPropName = "";
	m_SndGlobals.fileExt = "spr";

	m_SndGlobals.MaxPaths = 3;
	m_SndGlobals.DoorExpand = 1.0f;
	m_SndGlobals.Falloff_Outd = 10.0f;
	m_SndGlobals.Falloff_Ind = 9.0f;
	m_SndGlobals.kappa0 = 0.015f;

	m_SndGlobals.DefaultDoorLoss = 20.0f;
	m_SndGlobals.MaxRange = 2.2f;
	m_SndGlobals.MaxRangeCalVol = 30;
	m_SndGlobals.MaxEnvRange = 50;
	
	m_SndGlobals.Vol = 0.0;
	m_SndGlobals.DefaultThreshold = 20.0f;
}

void CsndPropBase::UpdateGlobals( void )
{
	// link console vars to globals here
}


/*********************************************************
*
*	CsndPropLoader Implementation
*
**********************************************************/


CsndPropLoader::CsndPropLoader ( void )
{
	m_PortData = NULL;
	m_sndAreas = NULL;
	m_numAreas = 0;
	m_numPortals = 0;
	m_bDefaultSpherical = false;
	m_bLoadSuccess = false;
}

CsndPropLoader::~CsndPropLoader ( void )
{
	// Call shutdown in case it was not called before destruction
	Shutdown();
}

void CsndPropLoader::Save(idSaveGame *savefile) const
{
	// Pass the call to the base class first
	CsndPropBase::Save(savefile);

	// TODO
}

void CsndPropLoader::Restore(idRestoreGame *savefile)
{
	// Pass the call to the base class first
	CsndPropBase::Restore(savefile);

	// TODO
}

/**
* MapEntBounds PSUEDOCODE:
* DOORS WITH BRUSHES:
* For each plane: add the direction normal vector * d to the origin.  
* This sould be the point we want to add to bounds for each plane,
* to generate a bounding box containing the planes
**/

bool CsndPropLoader::MapEntBounds( idBounds &bounds, idMapEntity *mapEnt )
{
	bool			returnval;
	idMapPrimitive	*testPrim;
	idMapBrush		*brush(NULL);
	idMapBrushSide	*face;
	idPlane			plane;
	int				numFaces, numPrim;
	idVec3			norm, *addpoints, debugCenter;
	idMat3			rotation;
	float			dist;
	const char      *modelName;
	cmHandle_t		cmHandle; // collision model handle for getting bounds

	idDict args = mapEnt->epairs;
	const idVec3 origin = args.GetVector("origin","0 0 0");
	modelName = args.GetString("model");
	
	// if a door doesn't have a model, the modelname will be the same as the doorname
	if( strcmp(modelName,args.GetString("name")) )
	{
		// NOTE: In the LoadModel call, Precache is set to false currnetly.  If it was set to
		// TRUE, this would force the door model to have a .cm file, otherwise
		// LoadModel would return 0 in this case. (precache = true, no .cm file)
		cmHandle = collisionModelManager->LoadModel( modelName, false );
		if ( cmHandle < 0 )
		{
			DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Failed to load collision model for entity %s with model %s.  Entity will be ignored.\r", args.GetString("name"), modelName);
			returnval = false;
			goto Quit;
		}

		collisionModelManager->GetModelBounds( cmHandle, bounds );
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found bounds with volume %f for entity %s with model %s.\r", bounds.GetVolume(), args.GetString("name"), modelName);
		gameEdit->ParseSpawnArgsToAxis( &args, rotation );

		bounds.RotateSelf(rotation);
		// Translate and rotate the bounds to be in sync with the model
		bounds.TranslateSelf(origin);
		// NOTE FOR FUTURE REFERENCE: MUST ROTATE THEN TRANSLATE
		/**
		* Global DoorExpand is applied to correct door bound inaccuracies.
		**/
		ExpandBoundsMinAxis(&bounds, m_SndGlobals.DoorExpand);
		debugCenter = bounds.GetCenter();
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Model Bounds center: %s , MapEntity origin: %s\r",debugCenter.ToString(),origin.ToString());
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Bounds rotation: %s\r", rotation.ToString());
		returnval = true;
		goto Quit;
		// Brian says it's okay if LoadModel is run twice, it just won't load it the 2nd time.
		// Therefore we won't worry about freeing the model now. (this caused a crash when I tried)
	}

	// Continue on if the door does not have a model:

	if( (numPrim = mapEnt->GetNumPrimitives()) == 0 )
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Door %s has no primitive data.  Door will be ignored.\r", args.GetString("name"));
		returnval = false;
		goto Quit;
	}
	for (int j=0; j < numPrim; j++)
	{
		testPrim = mapEnt->GetPrimitive(j);
		if ( testPrim->GetType() == testPrim->TYPE_BRUSH )
		{
			brush = static_cast<idMapBrush *>(testPrim);
			break;
		}
	}
	if (brush == NULL)
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Door %s does not have a brush primitive.  Door will be ignored\r", args.GetString("name"));
		returnval = false;
		goto Quit;
	}

	numFaces = brush->GetNumSides();
	addpoints = new idVec3[numFaces];
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("MapEntBounds: Door %s has %d faces\r", mapEnt->epairs.GetString("name"), numFaces );

	for(int i = 0; i < numFaces; i++)
	{
		face = brush->GetSide(i);
		plane = face->GetPlane();
		norm = plane.Normal();
		dist = plane.Dist();
		norm.Normalize();
		//addpoints[i] = ( norm/norm.Normalize() * dist ) + origin;
		//TODO: Make sure this change works correctly:
		addpoints[i] = ( norm * dist ) + origin;
		//DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Added point: %s to bounds for %s\r", addpoints[i].ToString(), mapEnt->epairs.GetString("name"));
	}

	bounds.FromPoints(static_cast<const idVec3*>(addpoints), static_cast<const int>(numFaces));
	
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Entity %s has bounds with volume %f\r", args.GetString("name"), bounds.GetVolume() );
    
	delete[] addpoints;
	
	returnval = true;

Quit:
	return returnval;
}

void CsndPropLoader::ExpandBoundsMinAxis( idBounds *bounds, float percent )
{
	idVec3 points[8], diff, mindiff, addpoints[2], oppPoint;
	bounds->ToPoints(points);
	float diffDist, mindiffDist(100000.0f); // initialize mindiffDist to a really big number
	// find the minimum axis and direction vector between minimum points
	for( int i = 1; i<8; i++ )
	{
		diff = points[0] - points[i];
		diffDist = diff.LengthFast();
		if (diffDist < mindiffDist)
		{
			mindiffDist = diffDist;
			mindiff = diff;
			oppPoint = points[i];
		}
	}
	// expand the axis by adding points along that axis
	addpoints[0] = points[0] + mindiff * percent;
	addpoints[1] = oppPoint - mindiff * percent;
	bounds->AddPoint(addpoints[0]);
	bounds->AddPoint(addpoints[1]);
}


void CsndPropLoader::ParseMapEntities ( idMapFile *MapFile )
{
	int			i;
	//int count(0), missedCount(0);
	idDict		args;
	
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Soundprop: Parsing Map entities\r");
	for (i = 0; i < ( MapFile->GetNumEntities() ); i++ )
	{
		idMapEntity *mapEnt = MapFile->GetEntity( i );
		args = mapEnt->epairs;
		const char *classname = args.GetString("classname");

		if( !strcmp(classname,m_SndGlobals.AreaPropName) )
		{
			ParseAreaPropEnt(args);
		}
		
		if( !strcmp(classname,"worldspawn") )
		{
			ParseWorldSpawn(args);
		}
	}

	m_AreaProps.Condense();

	FillAPGfromAP( gameRenderWorld->NumAreas() );

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Finished parsing map entities\r");
}

void CsndPropLoader::ParseWorldSpawn ( idDict args )
{
	bool SpherDefault;

	SpherDefault = args.GetBool("outdoor_propmodel","0");

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Parsing worldspawn sound prop data...\r" );
	if(SpherDefault)
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Using outdoor sound prop model as the default for this map\r");
	}
	else
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Using indoor sound prop model as the default for this map\r" );
	}
	m_bDefaultSpherical = SpherDefault;
}

void CsndPropLoader::ParseAreaPropEnt ( idDict args )
{
	int area;
	float lossMult, VolMod;
//	bool SpherSpread(false);
	SAreaProp propEntry;
	idStr lossvalue, VolOffset;

	if ( ( area = gameRenderWorld->GetAreaAtPoint(args.GetVector("origin")) ) == -1 )
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Warning: Sound area properties entity %s is not placed in any area.  It will be ignored\r", args.GetString("name") );
		goto Quit;
	}
	
	lossvalue = args.GetString("sound_loss_mult", "1.0");

	if(!( lossvalue.IsNumeric() ))
	{
		lossMult = 1.0;
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Warning: Non-numeric loss_mult value on area data entity: %s.  Default value assumed\r", args.GetString("name") );
	}
	else
		lossMult = fabs(atof(lossvalue));

	VolOffset = args.GetString("sound_vol_offset", "0.0");

	if (!( VolOffset.IsNumeric() ))
	{
		VolMod = 0.0;
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Warning: Non-numeric volume offset value on area data entity: %s.  Default value assumed\r", args.GetString("name") );
	}
	else
	{
		VolMod = atof(VolOffset);
	}

	// multiply Loss Mult by default attenuation constant
	propEntry.LossMult = lossMult * m_SndGlobals.kappa0;
	propEntry.VolMod = VolMod;
	propEntry.area = area;
	propEntry.DataEntered = false; // greebo: Initialised to false to fix gcc warning

	//add to the area properties list
	m_AreaProps.Append( static_cast<const SAreaProp>(propEntry) );
			
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Entity %s is a sound area entity.  Applied loss multiplier %f, volume modifier %f\r", args.GetString("name"), lossMult, VolMod );
	
Quit:
	return;
}

void CsndPropLoader::FillAPGfromAP ( int numAreas )
{
	int i, j, area(0);

	m_AreaPropsG.Clear();
	m_AreaPropsG.SetNum( numAreas );

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Allocated m_AreaPropsG for %d areas\r", numAreas );

	// set default values on each area
	for (i=0; i<numAreas; i++)
	{
		m_AreaPropsG[i].LossMult = 1.0 * m_SndGlobals.kappa0;
		m_AreaPropsG[i].VolMod = 0.0;
		m_AreaPropsG[i].DataEntered = false;
	}

	for(j=0; j < m_AreaProps.Num(); j++)
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Applying area property entity %d to gameplay properties array\r", j );
		
		area = m_AreaProps[j].area;
		m_AreaPropsG[area].LossMult = m_AreaProps[j].LossMult;
		m_AreaPropsG[area].VolMod = m_AreaProps[j].VolMod;
		m_AreaPropsG[area].DataEntered = true;
	}

	return;
}

int CsndPropLoader::FindSndPortal(int area, qhandle_t pHandle)
{
	int np, val(-1);
		
	np = gameRenderWorld->NumPortalsInArea(area);
	for (int i = 0; i < np; i++)
	{
		auto portalTmp = gameRenderWorld->GetPortal(area,i);
		//DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("FindSndPortal: Desired handle %d, handle of portal %d: %d\r", pHandle, i, areaP->portals[i].handle ); //Uncomment for portal handle debugging
		if(portalTmp.portalHandle == pHandle)
		{
			val = i;
			goto Quit;
		}
	}
Quit:
	return val;
}

void CsndPropLoader::CreateAreasData ( void )
{
	int i, j, k, np, anum, propscount(0), numAreas(0), numPortals(0), PortIndex;
	sndAreaPtr area;
	idVec3 pCenters;

	numAreas = gameRenderWorld->NumAreas();
	numPortals = gameRenderWorld->NumPortals();
	//pCenters.Zero(); // grayman #3660 - move down, gets initialized for each area

	m_sndAreas = new SsndArea[m_numAreas];
	m_PortData = new SPortData[m_numPortals];

	// Initialize portal data array
	for ( int k2 = 0 ; k2 < m_numPortals ; k2++ )
	{
		m_PortData[k2].lossAI = 0; // grayman #3042
		m_PortData[k2].lossPlayer = 0; // grayman #3042
		m_PortData[k2].LocalIndex[0] = -1;
		m_PortData[k2].LocalIndex[1] = -1;
		m_PortData[k2].Areas[0] = -1;
		m_PortData[k2].Areas[1] = -1;
	}
	
	for ( i = 0 ; i < m_numAreas ; i++ ) 
	{
		pCenters.Zero(); // grayman #3660 - must be initialized for each area
		area = &m_sndAreas[i];
		area->LossMult = 1.0;
		np = gameRenderWorld->NumPortalsInArea(i);
		area->numPortals = np;
		area->portalDists = nullptr;

		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Number of Portals in Area %d = %d\r", i, np);

		area->portals = new SsndPortal[np];
		for ( j = 0 ; j < np ; j++ ) 
		{
			auto portalTmp = gameRenderWorld->GetPortal(i,j);
			
			area->portals[j].portalNum = j;
			area->portals[j].handle = portalTmp.portalHandle;
			area->portals[j].from = portalTmp.areas[0]; // areas[0] is the 'from' area
			area->portals[j].to = portalTmp.areas[1];
			area->portals[j].center = portalTmp.w.GetCenter();
			area->portals[j].winding = &portalTmp.w;
			
			pCenters += area->portals[j].center;

			// enter the data into the portal data array
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Setting up portal handle %d from area %d\r",portalTmp.portalHandle, i);
			SPortData *pPortData = &m_PortData[ portalTmp.portalHandle - 1 ];
			
			// make sure we don't overwrite the data from the area on the other side
			if (pPortData->Areas[0] == -1 )
			{
				PortIndex = 0;
			}
			else
			{
				PortIndex = 1;
			}

			pPortData->Areas[ PortIndex ] = i;
			pPortData->LocalIndex[ PortIndex ] = j;
		}
		
		// average the portal center coordinates to obtain the area center

		// grayman #3660 - The term "area center" is misleading. It means
		// "the average center of all the centers of the area's portals".
		// This may or may not be near the geographic center of the sound area.
		if ( np )
		{
			area->center = pCenters / np;
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Area %d has approximate average portal center %s\r", i, area->center.ToString() );
		}
	}

	// Apply special area Properties
	for (k = 0 ; k < m_AreaProps.Num() ; k++ )
	{
		anum = m_AreaProps[k].area;
		m_sndAreas[anum].LossMult = m_AreaProps[k].LossMult;
		m_sndAreas[anum].VolMod = m_AreaProps[k].VolMod;
		propscount++;
	}

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("%d Area specific losses applied\r", propscount);

	// calculate the portal losses and populate the losses array for each area
	WritePortLosses();

    DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Create Areas array finished.\r");
}


void CsndPropLoader::WritePortLosses( void )
{
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Writing portal losses...\r");
	
	int row, col, area(0), numPorts(0);
	float lossval(0);

	for( area=0; area < m_numAreas; area++ )
	{
		numPorts = m_sndAreas[area].numPortals;

		m_sndAreas[area].portalDists = new CMatRUT<float>;

		// no need to write a matrix if the area only has one portal
		if (numPorts == 1)
			continue;

		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Initializing area %d with %d portals\r", area, numPorts);

		// initialize the RUT matrix to the right size
		m_sndAreas[area].portalDists->Init( numPorts );

		// fill the RUT matrix
		for( row=0; row < numPorts; row++ )
		{	
			for( col=(row + 1); col < numPorts; col++ )
			{
				DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Setting loss for portal %d to portal %d in area %d\r", row, col, area );
				// grayman #3660 - "lossval" is the distance in meters from one portal to another in this area
				lossval = CalcPortDist( area, row, col );

				m_sndAreas[area].portalDists->Set( row, col, lossval );
			}
		}
	}
}

float CsndPropLoader::CalcPortDist(	int area, int port1, int port2)
{
	float dist;
	idVec3 center1, center2, delta;
	// TODO: PHASE 3 SOUNDPROP: Implement design for geometrically calculating loss between portals
	// for now, just take center to center distance, correct in a lot of cases
	center1 = m_sndAreas[area].portals[port1].center;
	center2 = m_sndAreas[area].portals[port2].center;

	delta = center1 - center2;
	//TODO: If optimization is needed, use delta.LengthFast()
	dist  = delta.Length();
	dist *= s_DOOM_TO_METERS;

	return dist;
}

void CsndPropBase::DestroyAreasData( void )
{
	int i;
	SsndPortal *portalPtr;

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Clearing m_sndAreas \r");
	if( m_sndAreas )
	{
		for( i=0; i < m_numAreas; i++ )
		{
			portalPtr = m_sndAreas[i].portals;
			delete[] portalPtr;
	
			m_sndAreas[i].portalDists->Clear();
		}
	
		delete[] m_sndAreas;
		m_sndAreas = NULL;
		m_numAreas = 0;
	}
	
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Clearing m_PortData with %d portals \r", m_numPortals);
	if( m_PortData )
	{
		delete[] m_PortData;
		m_PortData = NULL;
		m_numPortals = 0;
	}

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Destroy Areas data finished.\r");
}

void CsndPropBase::SetPortalAILoss( int handle, float value )
{
	// make sure the handle is valid
	if ( ( handle < 1 ) || ( handle > gameRenderWorld->NumPortals() ) )
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("SetPortalAILoss called with invalid portal handle %d.\r", handle );
		gameLocal.Warning( "SetPortalAILoss called with invalid portal handle %d.", handle );

		return;
	}

	// grayman #3042 - separate loss values for AI and player
	m_PortData[ handle - 1 ].lossAI = value;

	// grayman #3042 - no need to tell the engine about this value, since the engine doesn't use it.
}

void CsndPropBase::SetPortalPlayerLoss( int handle, float value )
{
	// make sure the handle is valid
	if ( ( handle < 1 ) || ( handle > gameRenderWorld->NumPortals() ) )
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("SetPortalPlayerLoss called with invalid portal handle %d.\r", handle );
		gameLocal.Warning( "SetPortalPlayerLoss called with invalid portal handle %d.", handle );

		return;
	}

	// grayman #3042 - separate loss values for AI and player, and tell the engine
	m_PortData[ handle - 1 ].lossPlayer = value;
	gameRenderWorld->SetPortalPlayerLoss(handle,value);
}

float CsndPropBase::GetPortalAILoss( int handle )
{
	float returnval = 0.0;

	// make sure the handle is valid
	if ( ( handle < 1 ) || ( handle > gameRenderWorld->NumPortals() ) )
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("GetPortalAILoss called with invalid portal handle %d, returning zero loss.\r", handle );
		gameLocal.Warning( "GetPortalAILoss called with invalid portal handle %d, returning zero loss.", handle );
	}
	else
	{
		returnval = m_PortData[ handle - 1 ].lossAI; // grayman #3042
	}

	return returnval;
}


float CsndPropBase::GetPortalPlayerLoss( int handle )
{
	float returnval = 0.0;

	// make sure the handle is valid
	if ( ( handle < 1 ) || ( handle > gameRenderWorld->NumPortals() ) )
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("GetPortalPlayerLoss called with invalid portal handle %d, returning zero loss.\r", handle );
		gameLocal.Warning( "GetPortalPlayerLoss called with invalid portal handle %d, returning zero loss.", handle );
	}
	else
	{
		returnval = m_PortData[ handle - 1 ].lossPlayer; // grayman #3042
	}

	return returnval;
}

// ======================= CsndPropLoader =============================

void CsndPropLoader::CompileMap( idMapFile *MapFile  )
{
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Sound propagation system initializing...\r");

	// Just in case this was somehow not done before now
	DestroyAreasData();

	// clear the area properties
	m_AreaProps.Clear();

	m_numAreas = gameRenderWorld->NumAreas();

	m_numPortals = gameRenderWorld->NumPortals();

	ParseMapEntities(MapFile);

	CreateAreasData();

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Sound propagation system finished loading.\r");

	m_bLoadSuccess = true;
}

void CsndPropLoader::FillLocationData( void )
{
	idLocationEntity *pLocEnt;
	SAreaProp *pAreaProp;

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Filling location soundprop data.\r");

	if ( !m_AreaPropsG.Num() )
		goto Quit;

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Checking %d areas\r", gameRenderWorld->NumAreas() );
	for(int i = 0; i < gameRenderWorld->NumAreas(); i++)
	{
		pAreaProp = &m_AreaPropsG[i];
		if( pAreaProp->DataEntered )
		{
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Data already entered for are %d, skipping\r", i );
			continue;
		}

		pLocEnt = gameLocal.LocationForArea( i );
		if ( !pLocEnt )
		{
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("No location for area %d, skipping\r", i );
			continue;
		}

		// grayman #3660
		// This line:
		// pAreaProp->LossMult = pLocEnt->m_SndLossMult;
		// was wrong in that LossMult is supposed to be m_SndLossMult * kappa0.  This bug caused attenuation
		// to be 66.67x larger when calculating sound loss in maps that use location entities. Propagated sounds
		// weren't carrying as far as they should.  This was an old bug, affecting every release up to and including 2.01.
		// To solve this, include the missing multiplication by kappa0.
		// However, this is a major change in sound propagation, and missions released on 2.01 and earlier will
		// not benefit from this fix, since it will be in 2.02. As a workaround hack, we tell mappers working on new missions to set the
		// "sound_loss_mult" spawnarg on their location entities to "0.014", then look for that here. If we see it,
		// don't multiply by kappa0, since the mapper has already handled that manually. If we see anything else (probably the
		// default value of "1.00") then do the multiplication. This way, new missions that come out before 2.02 can benefit
		// from the bug discovery, and when 2.02 comes out, the missions don't need to be updated. All missions released after 2.02
		// can leave out the hack setting of "sound_loss_mult" to "0.014".
		// ("0.014" is chosen because it's close to 0.015, and players won't notice the difference in attenuation.)
		float sound_loss_mult = pLocEnt->m_SndLossMult;
		if ( !(abs(sound_loss_mult - 0.014f) < 0.0001)  ) // because we shouldn't trust floating point equality checks
		{
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("'%s' has sound_loss_mult %f, multiplying by %f\r", pLocEnt->GetName(), sound_loss_mult, m_SndGlobals.kappa0);
			sound_loss_mult *= m_SndGlobals.kappa0;
		}
		else
		{
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("'%s' has sound_loss_mult %f, NOT multiplying by %f\r", pLocEnt->GetName(), sound_loss_mult, m_SndGlobals.kappa0);
		}
		pAreaProp->LossMult = sound_loss_mult;

		pAreaProp->VolMod = pLocEnt->m_SndVolMod;
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found location data for area %d, entering lossmult %f, volmod %f\r", i, pAreaProp->LossMult, pAreaProp->VolMod );
	}

Quit:
	return;
}

void CsndPropLoader::Shutdown( void )
{
	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Clearing sound propagation data loader.\r");
	DestroyAreasData();

	// clear the area properties
	m_AreaProps.ClearFree();

	m_bDefaultSpherical = false;
	m_bLoadSuccess = false;

	m_AreaPropsG.ClearFree();
}
