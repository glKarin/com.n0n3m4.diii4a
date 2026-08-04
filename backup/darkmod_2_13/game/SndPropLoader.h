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
* DESCRIPTION: Sound propagation class for propagating suspicious sounds
* to AI Used on map initialization.
*
* This class will be only for map loading and generation of the Loss Matrix.  
* Actual gameplay functions will be handled by another class, CsndProp, for 
* ease of maintenance.
*
*****************************************************************************/

#ifndef SNDPROPLOADER_H
#define SNDPROPLOADER_H

#include "precompiled.h"
#include "DarkModGlobals.h"

template <class type> 
class CMatRUT;

/**
* Global soundprop settings
* Populated from entityDef atdm:soundprop_globals
**/
class SsndPGlobals
{
public:
	// names
	idStr AreaPropName; // classname of area properties entity
	idStr fileExt; // soundprop file extension
	
	// Map compiling:
	// Maximum number of paths to calculate for a given loss matrix element
	int MaxPaths;

	/**
	* Amount to expand the bounding box of a door model in its thinnest dimension
	* when checking for portals in doors.
	**/
	float DoorExpand;

	/**************
	* The following settings affect both gameplay and compiling:
	* They are not set const so they may be changed dynamically,
	* but keep in mind the compiled soundprop file for the map
	* will NOT change until recompiled.
	***************/

	float Falloff_Outd; // outdoor sound falloff, multiplied by log(dist^2)
	float Falloff_Ind; // indoor sound falloff, multiplied by log(dist^2)
	
	/**
	* Kappa0 is the decay constant [db/M] for a sound wave in air
    * In reality this depends on freqeuncy, humidity, etc.
    * But the effet is small compared to geometric spreading, (~1%)
	* So we assume it's a constant.
	**/
	float kappa0;

	/*************
	* The following settings effect only gameplay, and can
	* safely be linked to cVars if we want.
	**************/

	/**
	* If a func_door is not a func_darkmod_door, this is the default
	* loss that will be applied when sound propagates thru the door.
	**/
	float DefaultDoorLoss;

	// AI settings:
	
	/**
	* Default threshold of hearing for an AI (SPL)
	* This corresponds to the minimum audible loudness before
	* an AI starts getting alerted by a sound.
	* (Think of leaves rustling or a very soft whisper)
	* In reality this is about 20dB SPL for the average human
	**/
	float DefaultThreshold;

	/**
	* AI Volume: All sounds propagated to AI will be scaled
	* by this amount.  This is intended for gameplay balance,
	* and should be linked to a cVar for realtime testing.
	* This is in dB, so the default value is zero.
	**/
	float Vol;

	
	float MaxRange; // Range in meters above which sound propagation is not calculated
	float MaxRangeCalVol; // volume value to normalize by for the max soundrange (in dB)
	// NOT YET IMPLEMENTED:
	float MaxEnvRange; // Range in which environmental sounds are heard by AI

	bool bDebug;

	void	Save(idSaveGame *savefile) const;
	void	Restore(idRestoreGame *savefile);
};

//TODO: Move this stuff to documentation

/** 
* NOTE: MaxRange is the maximum distance (in meters) that a sound with
* volume equal to MaxRangeCalVol will be propagated.
* At the default values, these settings are calibrated so that the cutoff
* for a whisper (30dB SWL) is 2 meters (~6 ft)
*
* Formula for cutoff range:
* For a given volume vol0,
* Max Range is calculate with the following "empirical" formula,loosely based on
* the fact that sounds sound twice as loud when they differ by 10 dB
*
* range = pow(2, ((vol0 - MaxRangeCalVol) / 7) ) * MaxRange
*
* The penalty for overestimated the cutoff range is more AI included in the computation
* The penalty for underestimating is AI not hearing a sound that they should have heard.
* Ideally, you want the maxrange to be just a few meters past where the sound falls
* below the AI's threshold volume.
**/

/**
* ADDITIONAL NOTES:
* The formula for calculating how much an AI is alerted from a sound is:
*
* psychLoud [in "alert" units] = 1 + (propParms->loudness - threshold) 
*
* An alert unit of 1 is a barely heard sound (whisper) heard for ~100mS
* An alert unit of 10 corresponds to hearing twice as loud a sound.
**/

/**
* Portal data stored in portal data array indexed by handles
* Contains local portal numbers and attenuation for that portal
**/
typedef struct SPortData_s
{
	// indices of the portal in the two areas connected by it
	// order is arbitrary
	int LocalIndex[2];
	int Areas[2];		// area numbers that match up with the local index numbers
	float lossAI;		// grayman #3042 - acoustical loss [dB] to AI when going through the portal
	float lossPlayer;	// grayman #3042 - acoustical loss [dB] to Player when going through the portal
} SPortData;

/**
* Structure containing special area properties
* lossMultiplier defaults to 1.0 if not set.
**/
typedef struct SAreaProp_s
{
	int area; // number of the area, for list lookup
	float LossMult; // loss multiplier
	float VolMod; // added to volume of all sounds originating in this area
	bool DataEntered; // set to true if this area has specific data (used for area<-locatoin overriding)
} SAreaProp;

/**
* SsndPortal and SSndArea are structures for storing the area/portal
*	connectivity database.  They will be copied over to the gameplay object.
**/
typedef struct SsndPortal_s
{
	qhandle_t handle; // portal handle

	int portalNum; // integer ID of the portal in the area

	int from; // area the portal is in

	int to; // area the portal goes to

	idVec3 center; // coordinates of the center of the portal

	idVec3 normal; // normal vector of portal (by convention, this points into the room)

	const idWinding *winding; // point information 	 

} SsndPortal;

typedef SsndPortal* sndPortalPtr;

/**
* Array entry for the area->portals tree
**/
typedef struct SsndArea_s 
{
	float				LossMult; // loss multiplier (in dB/meter, less than 1 => less loss)
	
	float				VolMod; // Volume offset of sounds originating in this area, in dB.

	int					numPortals;	// number of portals in this area
	
	idVec3				center; // approximate center of the area // grayman #3660 - misleading, not necessarily true, and buggy in revs < 2.02
	
	SsndPortal_s		*portals;	// array containing the portals of this area

	CMatRUT<float>		*portalDists; // acoustical distances from each portal in room to each other portal

} SsndArea;

typedef SsndArea* sndAreaPtr; 



// ====================================================================
/**
* CLASS DESCRIPTION: CsndPropBase has functions and members
* inherited by both CsndPropLoader and CsndProp (the gameplay class)
**/

class CsndPropBase {

public:
	void	Save(idSaveGame *savefile) const;
	void	Restore(idRestoreGame *savefile);

	/**
	* Structure containing global sound properties.
	**/
	SsndPGlobals		m_SndGlobals;

	/**
	* Fill the global properties structure from the
	* soundpropglobals entityDef (defined in soundprop.def.
	* This could probably be put on CsndPropLoader instead of 
	* the base class, but just in case CsndProp needs it, it's here.
	*
	* Logs a warning and sets defaults if it cannot find the def
	* for atdm:soundprop_globals.
	**/
	void GlobalsFromDef( void );

	/**
	* Insert the loss argument into the portal data array entry for 
	* the given portal handle.
	* grayman #3042 - allow AI- and Player-specific loss
	**/
	void SetPortalAILoss( int handle, float value );
	void SetPortalPlayerLoss( int handle, float value );

	/**
	* Get the acoustical loss for the given portal handle
	* Portal handle must be between 1 and the number of portals in the map
	* grayman #3042 - allow AI- and Player-specific loss
	**/
	float GetPortalAILoss( int handle );
	float GetPortalPlayerLoss( int handle );

protected:

	/**
	* Updates any global sound properties that are linked 
	* to console vars or other realtime vars
	**/
	void UpdateGlobals( void );

	/**
	* Set the globals to some default values if the atdm:soundprop_globals
	* def is missing.
	**/
	void DefaultGlobals( void );

	/**
	* Delete the m_sndAreas array
	**/
	void DestroyAreasData( void );

protected:

	/**
	* This gets set to TRUE when a .spr file is successfully loaded
	* for the current map.
	**/
	bool				m_bLoadSuccess;

	/**
	* If set to true, the default sound model will be indoor propagation
	* This bool is read from the worldspawn entity, and defaults to false
	**/
	bool				m_bDefaultSpherical;	
	
	/**
	* Count of the number of areas in a map
	**/
	int					m_numAreas;

	/**
	* Count of the number of unique portals in a map
	**/
	int					m_numPortals;


	/********************************************************************
	* GAMEPLAY MEMBERS
	* Members after this point must be passed along to be used in gameplay!
	*********************************************************************/

	/**
	* Area and portal connectivity database
	* Created by loader, used during gameplay
	**/
	SsndArea			*m_sndAreas;

	/**
	* m_AreaPropsG contains the area properties of ALL areas for use
	* during propagation.
	* defaults are loss multiplier = 1.0, and VolMod = 0.0.
	**/
	idList<SAreaProp>	 m_AreaPropsG;

	/**
	* Portal data array indexed by portal handle
	* Used to optimize lookup of local portal number
	* Also stores the current attenuation value of the portal
	**/
	SPortData			*m_PortData;
};


/**
* CLASS DESCRIPTION: CsndPropLoader class.  Handles parsing of mapfile
* into sound prop data, precalculating portal losses.  Also handles
* loading of existing sound propagation files (.spr) for a given map,
* and writing of these files after the mapfile compilation is done.
* 
**/

class CsndPropLoader : public CsndPropBase {
public:

	friend class CsndProp;

	CsndPropLoader ( void );
	~CsndPropLoader ( void );

	void	Save(idSaveGame *savefile) const;
	void	Restore(idRestoreGame *savefile);

	/**
	* Load sound prop system from a loaded mapfile.
	* This does the actual parsing of the map file and will do
	* the generating of the Loss Matrix, as opposed to LoadSprFile
	* which just loads the file for the map.
	* For now, this is run in game_local every map load as a test
	* When the actual system is in place, we must check to see if the map
	* has an existing .spr file, and load that instead of running this.
	* 
	* This should probably be renamed to CompileMap. This is what we
	* call from the editor console to compile the .spr AFTER compiling
	* the map with dmap.  
	*
	* Also, this function will have to load the mapfile
	* idMapFile pointer for the appropriate map itself instead of relying on
	* something else to pass it.
	**/
	void CompileMap( idMapFile *MapFile );
	
	/**
	* Destroy sound prop data when switching to a new map, game ends, etc
	**/
	void Shutdown( void );

	/**
	* To be run AFTER entities spawn.  Goes through the location areas and fills
	* in location data for each area.  Note area data objects override location data.
	**/
	void FillLocationData( void );

private:
	
	/**
	* The following functions go through the mapfile and identify map entities
	* that are associated with sound prop. ParseMapEntities calls ParseWorldSpawn,
	* ParseAreapropEnt and ParseDoor.
	**/

	void ParseMapEntities ( idMapFile *MapFile );

	/**
	* ParseWorldSpawn gets map-wide sound information from the worldspawn entity
	* This information includes whether the default sound prop model should be
	* indoor or outdoor (whether the map is predominantly indoor or outdoor)
	**/
	void ParseWorldSpawn ( idDict args );

	/**
	* Area property entities are parsed to add their properties to 
	* the area properties array.
	**/
	void ParseAreaPropEnt ( idDict args );
	
	/**
	* Searches the provided area number for the portal handle pHandle.
	* Then returns the integer index of the portal within the portal list 
	* of that area. (As defined in gameRenderWorld->GetPortal(area num,portal num))
	* Returns -1 if the portal is not found.
	**/
	int  FindSndPortal( int area, qhandle_t pHandle );

	/**
	* Finds the bounds of a map entity and writes it to bounds argument.
	* 
	* If the map entity has a model, the bounds of the collision model are returned.
	* If no collision model is available for the given model, the rendermodel
	* is used as the collision model.  This is usually not good since rendermodels can
	* be high poly.
	* 
	* If the map entity has no model, the first primitive brush found is used to
	* generate a bounding box.  Primitive patches are ignored.
	*
	* This function is currently only used for finding the bounds of doors
	* to check if a given door contains a portal.
	**/
	bool MapEntBounds( idBounds &bounds, idMapEntity *mapEnt );
	
	/**
	* Helper function : TODO : move to a more general library, perhaps idBounds
	* Expand the bounds only in the direction that they are the thinnest.
	* Expands by percent * the thinnest width of the box (note 1.0 = 100%)
	**/
	void ExpandBoundsMinAxis( idBounds *bounds, float percent );
	
	/**
	* Create the Areas array that stores which portals connect which areas
	* as well as area sound loss multipliers.
	* NOTE: Destroy is on base class
	**/
	void CreateAreasData ( void );

	/**
	* Precalculated the portal-to-portal losses and write them to m_sndAreas
	* To be called by or after CreateAreasData
	**/
	void WritePortLosses( void );

	/**
	* Calculate the distance a sound wave would travel between two
	* portal centers Often this is a straight line, but sometimes it bounces.
	**/
	float CalcPortDist( int area, int port1, int port2 );

	/**
	* Fill the m_AreaPropsG array from the m_AreaProps array.
	* Default loss multiplier = 1.0, default sound model = indoor
	**/
	void FillAPGfromAP ( int numAreas );

protected:

	/************************************************************
	* PRE-GAMEPLAY MEMBERS
	* These members are only used by the pre-gameplay object and don't need
	* to be copied to the gameplay object.
	***********************************************************/

	/**
	* List of area properties.
	* Only contains areas with non-default properties.
	**/
	idList<SAreaProp>	m_AreaProps;

};

#endif
