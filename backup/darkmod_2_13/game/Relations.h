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
/*         Dark Mod AI Relationships (C) by Chris Sarantos in USA 2005		  */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
*
* DESCRIPTION: CRelations is a "relationship manager" that keeps track of and
* allows changes to the relationship between each team, including every
* AI team and the player team (team 0).
* Relationship values are integers.  negative is enemy, positive friend, zero
* neutral.  
*
* The larger the value, the more strongly an AI "cares" about the relationship.
* Large - value => will not rest until they've hunted down every last member
* of this team.  
* Large + value => these teams had best not forget their anniversary.
*
* Accessor methods should be made available to scripting, for relationship dependent
* scripts, and for changing the relationships on a map in realtime.
*
* TODO: Allow for "persistent relationships" that copy over between maps.
* This would be useful for T3-esque factions.  It might require a "faction ID"
* that is different from the "team ID" in the mapfile though.
*
*****************************************************************************/

#ifndef RELATIONS_H
#define RELATIONS_H

//#include "precompiled.h"
#include "DarkModGlobals.h"
#include "MatrixSq.h"

class CRelations : 
	public idClass
{
	CLASS_PROTOTYPE( CRelations );
public:

	typedef enum ERel_Type
	{
		E_ENEMY = -1,
		E_NEUTRAL = 0,
		E_FRIEND = 1
	} ERel_Type;

	struct SEntryData
	{
		int row;
		int col;
		int val;

		SEntryData() :
			row(-1), col(-1), val(-1)
		{}

		SEntryData(int _row, int _col, int _val) :
			row(_row), col(_col), val(_val)
		{}
	};

public:

	CRelations();
	virtual ~CRelations() override;

	void Clear();

	bool IsCleared();

	/**
	* Returns the dimension of the square relationship matrix.
	* For example, if the matrix is 3x3, it returns 3.
	**/
	int Size(void);

	/**
	* Return the integer number for the relationship between team i and team j
	**/
	int GetRelNum(int i, int j);

	/**
	* Return the type of relationship (E_ENEMY, E_NEUTRAL or E_FRIEND)
	* for the relationship between team i and team j
	**/
	int GetRelType(int i, int j);

	/**
	* Set the integer value of the relationship between team i and team j to rel
	**/
	void SetRel(int i, int j, int rel);

	/**
	* Add the integer 'offset' to the relationship between team i and team j
	* (You can add a negative offset to subtract)
	**/
	void ChangeRel( int i, int j, int offset);

	/**
	* Returns true if team i and team j are friends
	**/
	bool IsFriend( int i, int j);

	/**
	* Returns true if team i and team j are enemies
	**/
	bool IsEnemy( int i, int j);

	/**
	* Returns true if team i and team j are neutral
	**/
	bool IsNeutral( int i, int j);

	bool CheckForHostileAI(idVec3 point, int team); // grayman #3548

	/**
	* Fill the relationship matrix from the def file for the map
	* returns FALSE if there was a problem loading
	**/
	bool SetFromArgs( const idDict& args );

	/**
	* Save the current relationship matrix to a savefile
	* To be consistent w/ D3, this should be called in
	* in idGameLocal::SaveGame , where everything else is saved.
	* This is most likely necessary to save in the correct place.
	**/
	void Save( idSaveGame *save ) const;

	/**
	* Load the current relationship matrix from a savefile
	**/
	void Restore( idRestoreGame *save );

	/**
	* Output the matrix to the console and logfile for debug purposes
	**/
	void DebugPrintMat( void );

private:
	/**
	 * greebo: Extends the relations and fills in the default relations
	 * values for all the new elements.
	 */
	void ExtendRelationsMatrixToDim(int newDim);

protected:

	/**
	 * greebo: Parse the given key value for an relations entry.
	 * Will emit warnings to the console if parsing fails.
	 * 
	 * @returns: the filled in SEntryData.
	 * @throws: a std::runtime_error if the given keyvalue is not 
	 * suitable.
	 */
	SEntryData ParseEntryData(const idKeyValue* kv);

	/**
	* The relationship matrix uses class CMatrixSq to store a square matrix
	**/
	CMatrixSq<int>		m_RelMat;
};
typedef std::shared_ptr<CRelations> CRelationsPtr;

/**
 * greebo: A spawnable relation entity. At spawn time, this entity copies its 
 * values to the global relations manager. 
 *
 * After spawning, this entity removes itself from the game.
 */
class CRelationsEntity : 
	public idEntity
{
public:
	CLASS_PROTOTYPE( CRelationsEntity );

	void Spawn();
};

/**
 * greebo: A relations map entity which adds its relations settings
 * to the global relations manager when triggered.
 */
class CTarget_SetRelations : 
	public idEntity
{
public:
	CLASS_PROTOTYPE( CTarget_SetRelations );

	void Event_Activate(idEntity* activator);
};

class CTarget_SetEntityRelation : 
	public idEntity
{
public:
	CLASS_PROTOTYPE( CTarget_SetEntityRelation );

	void Event_Activate(idEntity* activator);
};

class CTarget_ChangeEntityRelation : 
	public idEntity
{
public:
	CLASS_PROTOTYPE( CTarget_ChangeEntityRelation );

	void Event_Activate(idEntity* activator);
};

#endif
