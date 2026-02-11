/*
===============================================================================
game_dda.h
	This contains the functionality for a dynamic difficulty adjustment system,
		as well as a statistic tracking system.
===============================================================================
*/
#ifndef __GAME_DDA_H__
#define __GAME_DDA_H__

// DDA_Export Methods.
enum hhDDAExport {
	DDA_ExportTEXT,
	DDA_ExportCSV,
};

typedef struct ddaDeath_s {
	int		time;
	idStr	location;
} ddaDeath_t;

const int DDA_INDEX_MAX = 16;

//======================================================================================
//======================================================================================
//
// NEW DDA SYSTEM
//
//======================================================================================
//======================================================================================

#define NUM_DAMAGES		8

class hhDDAProbability {
public:
	hhDDAProbability();

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	void AddDamage( int damage ); 
	void AddSurvivalValue( int playerHealth );

	float GetProbability( int value );
	float GetSurvivalMean( void );

	void AdjustDifficulty( float diff );
	float GetIndividualDifficulty( void ) { return individualDifficulty; }

	bool IsUsed( void ) { return bUsed; }

protected:

	void			CalculateProbabilityCurve();

	bool			bUsed; // True if this probability instance is used (so the damage probability of later creatures doesn't influence earlier levels)

	int				damages[NUM_DAMAGES];
	int				damageRover;

	float			survivalValues[NUM_DAMAGES];
	int				survivalRover;

	float			mean;
	float			stdDeviation;

	float			individualDifficulty;
};

/***********************************************************************
  hhDDAManager.
	Class that controls the DDA tracking and difficulty of the game.
***********************************************************************/
class hhDDAManager {
public:
	hhDDAManager();
	~hhDDAManager();
public:
	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );

	//Public difficulty functions/accessors..
	float		GetDifficulty();					//Called to get current difficulty rating (0.0 - 1.0)
	void		DifficultySummary();
	void		ForceDifficulty( float newDifficulty ); // Force the difficulty to this value

	void		RecalculateDifficulty( int updateFlags );

	//Public DDA Notifications. (these are called to update the DDA system about activities happening in the game)
	void		DDA_Heartbeat( hhPlayer* player );

	void		DDA_AddDamage( int ddaIndex, int damage );
	void		DDA_AddSurvivalHealth( int ddaIndex, int health );
	void		DDA_AddDeath( hhPlayer *player, idEntity *attacker );

	float		DDA_GetProbability( int ddaIndex, int value );

	//Public DDA Exporting functions.  These are called to export DDA stats to either the console or a file.
	void		Export( const char* filename );

	void		PrintDDA( void );

	//Public DDA Controls.  These are used to clear/reset the DDA system (on map-switching).
	void		ClearTracking();

protected:
	//Protected Internal functions for exporting purposes.
	void		ExportDDAData( idStr fileName, const char* header, const char *fileAddition, idList<idStr> data );
	void		ExportLINData( idStr FileName );

	//Protected Interal Difficulty 'Ticker'.  Difficulty system will re-evaluate whether to change diff here.
	//o	void		DifficultyUpdate();

	//Protected Internal Difficulty data.
	bool				bForcedDifficulty;					// Used for testing, and Wicked mode
	float				difficulty;							// Our current difficulty rating.
	hhDDAProbability	ddaProbability[DDA_INDEX_MAX]; // CJR:  array of probabilites used in the DDA system

	idList<idStr>		locationNameData;
	idList<idStr>		locationData;
	idList<int>			healthData;
	idList<idStr>		healthSpiritData;
	idList<idStr>		ammoData;
	idList<idStr>		miscData;
	idList<ddaDeath_t>	deathData;
};

#endif // __GAME_DDA_H__