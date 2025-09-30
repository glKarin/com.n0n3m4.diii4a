/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"


int GetTeamAtRank( int rank ){
	int		i, j, count;
	int		ranks[4];
	int		counts[4];

	for (i = 0; i < 4; i++){
		counts[i] = TeamCount( -1, TEAM_RED + i );
		ranks[i] = 0;
	}

	for (i = 0; i < 4; i++){
		if (!counts[i]) continue;

		count = 0;
		for (j = 0; j < 4; j++){
			if (!counts[j]) continue;

			if (isRallyRace()){
				if (level.teamTimes[i + TEAM_RED] > level.teamTimes[j + TEAM_RED]) count++;
			}
			else if (level.teamScores[i + TEAM_RED] < level.teamScores[j + TEAM_RED]) count++;
		}

		while( count < 4 && ranks[count] ) count++; // rank is taken so move to the next one
		if (count < 4)
			ranks[count] = TEAM_RED + i;
	}

	if (g_gametype.integer == GT_CTF && rank > 2){
		return -1;
	}
	else {
		return ranks[rank-1];
	}
}


// UPDATE - send as command string instead?
void Cmd_RacePositions_f( void ) {
	char			entry[1024];
	char			string[1400];
	gentity_t		*player;
	int				i, count, j, stringlength;

	string[0] = 0;
	stringlength = 0;

	for(i = 0, count = 0; i < level.maxclients; i++){
		player = &g_entities[i];
		if (!player->inuse) continue;
		if (!player->client) continue;

		Com_sprintf (entry, sizeof(entry)," %i %i", player->s.clientNum, player->client->ps.stats[STAT_POSITION]);
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;

		count++;
	}

	G_LogPrintf("%s\n", va("positions %i%s", count, string));
	trap_SendServerCommand( -1, va("positions %i%s\n", count, string) );
}


void Cmd_Times_f( gentity_t *ent ) {
/*
	gentity_t		*player;
	int				times[4];
	int				i, count;

	for(i = 0; i < 4; i++){
		times[i] = 0;
	}

	for(i = 0; i < MAX_CLIENTS; i++){
		player = &g_entities[i];
		if (!player->inuse) continue;
		if (!player->client) continue;
		if (player->client->sess.sessionTeam == TEAM_SPECTATOR) continue;
		if (player->client->sess.sessionTeam == TEAM_FREE) continue;
		if (!level.startRaceTime) continue;

		if (player->client->finishRaceTime){
			times[player->client->sess.sessionTeam - TEAM_RED] +=
				(player->client->finishRaceTime - level.startRaceTime);
		}
		else {
			times[player->client->sess.sessionTeam - TEAM_RED] +=
				(level.time - level.startRaceTime);
		}
	}

	if (g_gametype.integer == GT_TEAM_RACING_DM){
		for(i = 0; i < 4; i++){
			if (level.teamScores[i + TEAM_RED] > 0){
				times[i] -= level.teamScores[i + TEAM_RED] * TIME_BONUS_PER_FRAG;
			}

			if (times[i] < 0)
				times[i] = 0;

			count = TeamCount( -1, TEAM_RED+i );
			if (count){
				times[i] /= count;
			}
		}
	}
	else {
		for(i = 0; i < 4; i++){
			if (times[i] < 0)
				times[i] = 0;

			count = TeamCount( -1, TEAM_RED+i );
			if (count){
				times[i] /= count;
			}
		}
	}

	trap_SendServerCommand( ent-g_entities, va("times %i %i %i %i\n",
		times[0], times[1], times[2], times[3]) );
*/
}


/*
================================================================================
GetDistanceToMarker

 Used to calculate how far a player is from the marker.
 Called to find out race positions of players.
================================================================================
*/
float GetDistanceToMarker( gentity_t *player, float markerNumber )
{
	gentity_t		*ent = NULL;
	vec3_t			dist;

	if ( !markerNumber )
		return 1<<30;

	while ( (ent = G_Find (ent, FOFS(classname), "rally_checkpoint")) != NULL )
	{
		if( ent->number == markerNumber )
			break;
	}

	if ( ent )
	{
		VectorSubtract(player->r.currentOrigin, ent->s.origin, dist);
		return VectorLength(dist);
	}
	else
		return 1<<30;
}

/*
================================================================================
IsCarAhead

 Returns true if player one is ahead of two.
================================================================================
*/
qboolean IsCarAhead(gentity_t *one, gentity_t *two){
	float		dist1, dist2;
	int			time1, time2;

	if (one->client->finishRaceTime && two->client->finishRaceTime){
		time1 = one->client->finishRaceTime - level.startRaceTime;
		if (one->client->ps.persistant[PERS_SCORE] > 0 && !isRallyNonDMRace()){
			time1 -= (one->client->ps.persistant[PERS_SCORE] * TIME_BONUS_PER_FRAG);
		}

		time2 = two->client->finishRaceTime - level.startRaceTime;
		if (two->client->ps.persistant[PERS_SCORE] > 0 && !isRallyNonDMRace()){
			time2 -= (two->client->ps.persistant[PERS_SCORE] * TIME_BONUS_PER_FRAG);
		}

		if (time1 < time2){ // use frag modified times
//			Com_Printf("Car 1 finished the race with less time than car 2\n");
			return qtrue;
		}
		else {
//			Com_Printf("Car 2 finished the race with less time than car 1\n");
			return qfalse;
		}
	}
	else if (one->client->finishRaceTime){
//		Com_Printf("Car 1 finished the race, car 2 hasn't\n");
		return qtrue;
	}
	else if (two->client->finishRaceTime){
//		Com_Printf("Car 2 finished the race, car 1 hasn't\n");
		return qfalse;
	}
	else if (one->currentLap < two->currentLap){
//		Com_Printf("Car 1 is a lap behind car 2\n");
		return qfalse;
	}
	else if (one->currentLap == two->currentLap && one->number < two->number){
//		Com_Printf("Car 1 hat a target marker that is behind car 2's\n");
		return qfalse;
	}
	else if (one->currentLap == two->currentLap && one->number == two->number){
		dist1 = GetDistanceToMarker( one, one->number );
		dist2 = GetDistanceToMarker( two, two->number );

		if (dist1 > dist2){
//			Com_Printf("Car 1 is %f to marker %i and car 2 is %f\n", dist1, one->number, dist2);
			return qfalse;
		}
	}

	return qtrue;
}


/*
================================================================================
CalculatePlayerPositions

 Calculates the order of all racers
================================================================================
*/
void CalculatePlayerPositions( void )
{
	gentity_t	*ent, *leader, *cur, *last;
	int			position;
	qboolean	positionChanged;

//	if (level.startRaceTime + FRAMETIME > level.time || level.startRaceTime == 0){
//		return;
//	}
	if (!isRallyRace()){
		return;
	}

	positionChanged = qfalse;
	leader = ent = last = NULL;
	while ( (ent = G_Find (ent, FOFS(classname), "player")) != NULL )
	{
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) continue;
//		if ( isRaceObserver(ent->s.number) ) continue;

		ent->carBehind = NULL;

		if ( leader == NULL )
		{
			leader = ent;
			continue;
		}

		cur = leader;
		if ( IsCarAhead( ent, cur ) )
		{
			ent->carBehind = cur;
			leader = ent;
			continue;
		}

		while ( cur->carBehind != NULL )
		{
			if ( IsCarAhead( ent, cur->carBehind ) )
			{
//				ent->carBehind = cur->carBehind;
//				cur->carBehind = ent;
				last = cur;
				cur = cur->carBehind;
				break;
			}

			last = cur;
			cur = cur->carBehind;
		}

		if ( IsCarAhead( ent, cur ) )
		{
//			cur->carBehind = NULL;
			ent->carBehind = cur;
			if (last) {
				last->carBehind = ent;
			}
		}
		else {
			cur->carBehind = ent;
			ent->carBehind = NULL;
		}
	}

	if ( leader == NULL )
		return;

	cur = leader;
	position = 1;

	while( cur->carBehind != NULL )
	{
		if ( position != cur->client->ps.stats[STAT_POSITION] && cur->client ){
			cur->client->ps.stats[STAT_POSITION] = position;

			positionChanged = qtrue;
		}

		cur = cur->carBehind;
		position++;
	}

	if ( position != cur->client->ps.stats[STAT_POSITION] && cur->client ){
		cur->client->ps.stats[STAT_POSITION] = position;

		positionChanged = qtrue;
	}

	if ( positionChanged )
	{
		Cmd_RacePositions_f();
		CalculateRanks();
	}
}


void RallyRace_Think( gentity_t *ent ){
	ent->nextthink = level.time + 200;

	CalculatePlayerPositions();
}

void RaceCountdown( char *s, int secondsLeft ){
	trap_SendServerCommand( -1, va("rc \"%s\" %d", s, secondsLeft) );
}

void RallyStarter_Think( gentity_t *ent ){
	gentity_t		*player, *t;
	int				i, count;
	qboolean	start;

	if (level.startRaceTime){
		return;
	}

	// if no checkpoints dont do start sequence
	if (isRallyRace()){
		t = NULL;
		t = G_Find (t, FOFS(classname), "rally_checkpoint");
		if (t == NULL){
			// start race right away
			level.startRaceTime = level.time;
			trap_SendServerCommand( -1, va("raceTime %i", level.startRaceTime) );
			CenterPrint_All("GO..");

			G_FreeEntity( ent );
			return;
		}
	}
	ent->nextthink = level.time + 1000;
	t = NULL;

	if ( ent->number == 0 ){

		if( level.time - level.startTime < 7500 )
			return;

		start = qtrue;
		for (i = 0, count = 0; i < MAX_CLIENTS; i++){
			player = &g_entities[i];
			if (!player->inuse) continue;
			if (!player->client) continue;
			if (player->client->sess.sessionTeam == TEAM_SPECTATOR) continue;
			// bots are always ready

			count++;

			if (player->r.svFlags & SVF_BOT) continue;

			if ( !player->ready ){
				start = qfalse;
				break;
			}
		}

		if ( !count ){
			return;
		}
		else if ( start && count ){
			ent->number = 3;
		}
		else if ( level.time >= level.startTime + (g_forceEngineStart.integer * 1000) ) {
			ent->number = 3; // force race start
		}
		else if (ent->number == 0 && level.time > level.startTime + (g_forceEngineStart.integer * 1000) - 10000){
			CenterPrint_All( va("Forced engine start in %i...", 10 - ((level.time - (level.startTime + (g_forceEngineStart.integer * 1000) - 10000)) / 1000)) );
			return;
		}
		else {
			return;
		}
	}

	if ( ent->pain_debounce_time == 0 )
		ent->pain_debounce_time = level.time;

	if ( level.time > ent->pain_debounce_time + 5000 ){
		level.startRaceTime = level.time;

		trap_SendServerCommand( -1, va("raceTime %i", level.startRaceTime) );
		RaceCountdown("GO!", 0);

		Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/go.wav") );

		if (g_gametype.integer != GT_DERBY)
			ent->think = RallyRace_Think;
	}
	else if ( level.time > ent->pain_debounce_time + 4000 ){
		RaceCountdown("1", 1);

		Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/one.wav") );
		ent->number = -1;
	}
	else if ( level.time > ent->pain_debounce_time + 3000 ){
		RaceCountdown("2", 2);

		Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/two.wav") );
		ent->number = 1;
	}
	else if ( level.time > ent->pain_debounce_time + 2000 ){
		RaceCountdown("3", 3);

		Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/three.wav") );
		ent->number = 2;
	}
	else {
		CenterPrint_All("Starting Race...");
	}
}

void CreateRallyStarter( void ) {
	gentity_t		*ent;

	ent = G_Spawn();

	ent->think = RallyStarter_Think;
	ent->nextthink = level.time + 2000;
	ent->number = 0;
	ent->classname = "rally_starter";
}


/*
===========
SelectLastMarkerForSpawn

  Places cars at the last marker they visited during a race

============
*/
gentity_t *SelectLastMarkerForSpawn( gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot ) {
	gentity_t	*spot;
	int			lastMarker;

	spot = NULL;
	lastMarker = ent->number - 1;
	if (lastMarker <= 0){
		lastMarker = level.numCheckpoints;
	}

	while ((spot = G_Find (spot, FOFS(classname), "rally_checkpoint")) != NULL) {
		if ( spot->number == lastMarker) {
			break;
		}
	}

	if ( !spot ) {
		return SelectSpawnPoint( vec3_origin, origin, angles, isbot );
	}

	// spawn at last checkpoint
	VectorCopy (spot->s.origin, origin);
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectGridPositionSpawn

  Places cars at the start line in order, so that no one is telefragged

============
*/
gentity_t *SelectGridPositionSpawn( gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot ) {
	gentity_t	*spot;
	int			gridPosition;

	spot = NULL;
	gridPosition = 1;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL) {
		if ( (spot->number == gridPosition || !spot->number) && !SpotWouldTelefrag( spot )) {
			break;
		}
		else if (spot->number == gridPosition){
			spot = NULL; // found spawn but someone is already there so restart search
			gridPosition++;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		// FIXME: put into spectator mode instead?
		G_Printf("Warning: No info_player_start found for race spawn, trying info_player_deathmatch\n");
		return SelectSpawnPoint( vec3_origin, origin, angles, isbot );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

