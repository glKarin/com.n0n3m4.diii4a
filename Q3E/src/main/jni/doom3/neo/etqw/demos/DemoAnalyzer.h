// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_DEMOS_DEMOANALYZER_H__
#define __GAME_DEMOS_DEMOANALYZER_H__

class sdDemoAnalyzer {
public:
					sdDemoAnalyzer();

	void			Start();
	void			Stop();

	void			RunFrame();

	bool			IsActive() const { return sectors != NULL; }

	void			LogPlayerDeath( idPlayer* player, idEntity* inflictor, idEntity* attacker );

private:
	struct playerTeamClassStats_t {
		unsigned int	playerLocation;
		unsigned int	playerTransportLocation;
		unsigned int	playerDeath;
	};

	struct sector_t {
		playerTeamClassStats_t**	playerTeamClassStats;	// [team][class]
	};

	sector_t*		GetSector( const idVec3& origin );

private:
	static idCVar	g_demoAnalysisSectorSize;

	sector_t*		sectors;
	float			sectorSize;
	int				sectorDimX;
	int				sectorDimY;

	int*			classIndexToTeamClassIndex;
	int**			teamClassIndexToClassIndex;

	// logging
	bool			playerHasDied[ MAX_CLIENTS ];

	// for normalization purposes
	unsigned int	maxPlayerLocation;
	unsigned int	maxTransportLocation;
	unsigned int	maxPlayerDeath;
};

#endif // __GAME_DEMOS_DEMOMANAGER_H__
