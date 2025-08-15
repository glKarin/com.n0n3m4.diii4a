/*
 * Copyright (C) 2017 Daniele Pantaleone <danielepantaleone@me.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "server.h"


#define EVT_GENERAL_SOUND  59

int mx = 123456789;
int my = 362436069;
int mz = 521288629;
int mw = 886751235;

/**
 * Find the configstring index for the given entry.
 */
int SV_FindConfigstringIndex(char *name, int start, int max, qboolean create) {

	int i;
	char s[MAX_STRING_CHARS];

	if (!name || !name[0]) {
		return 0;
	}

	for (i = 1 ;i < max; i++) {
		SV_GetConfigstring(start + i, s, sizeof(s));
		if (!s[0])
			break;
		if (!strcmp(s, name))
			return i;
	}

	if (!create) {
		return 0;
	}

	if (i == max) {
		Com_Printf("SV_FindConfigStringIndex: overflow");
	}

	SV_SetConfigstring(start + i, name);

	return i;

}

/**
 * Log in the same file as the game module.
 */
void QDECL SV_LogPrintf(const char *fmt, ...) {

	va_list argptr;
	fileHandle_t file;
	fsMode_t mode;
	char *filename;
	char buffer[MAX_STRING_CHARS];
	int min, tens, sec;

	filename = Cvar_VariableString("g_log");
	if (!filename[0]) {
		return;
	}

	mode = Cvar_VariableIntegerValue("g_logSync") ? FS_APPEND_SYNC : FS_APPEND;

	FS_FOpenFileByMode(filename, &file, mode);
	if (!file) {
		return;
	}

	sec  = sv.time / 1000;
	min  = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf(buffer, sizeof(buffer), "%3i:%i%i ", min, tens, sec);

	va_start(argptr, fmt);
	vsprintf(buffer + 7, fmt, argptr);
	va_end(argptr);

	FS_Write(buffer, strlen(buffer), file);
	FS_FCloseFile(file);

}

/**
 * Send the given client scores single to all connected clients.
 *
 * @author Daniele Pantaleone
 */
void SV_SendScoreboardSingleMessageToAllClients(client_t *cl, playerState_t *ps) {

	int i;
	char *score;
	client_t *dst;

#ifdef USE_AUTH
	score = va("scoress %i %i %i %i %i %i %i %i %i %i %i %i %s", (int)(cl - svs.clients),
			   ps->persistant[PERS_SCORE], cl->ping, sv.time / 60000, 0,
			   ps->persistant[PERS_KILLED], 0, 0, 0, 0, 0, 0, cl->auth);
#else
	score = va("scoress %i %i %i %i %i %i %i %i %i %i %i %i ---", (int)(cl - svs.clients),
			   ps->persistant[PERS_SCORE], cl->ping, sv.time / 60000, 0,
			   ps->persistant[PERS_KILLED], 0, 0, 0, 0, 0, 0);
#endif

	for (i = 0, dst = svs.clients; i < sv_maxclients->integer; i++, dst++) {
		if (dst->state != CS_ACTIVE)
			continue;
		SV_SendServerCommand(dst, "%s", score);
	}

}

/**
 * Broadcast a specific sound to the given client.
 */
void SV_SendSoundToClient(client_t *cl, char *name) {

	int bits;
	int index;
	playerState_t *ps;

	index = SV_FindConfigstringIndex(name, CS_SOUNDS, MAX_SOUNDS, qtrue);
	if (!index) {
		return;
	}

	ps = SV_GameClientNum(cl - svs.clients);
	bits = ps->externalEvent & EV_EVENT_BITS;
	bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
	ps->externalEvent = EVT_GENERAL_SOUND | bits;
	ps->externalEventParm = index;
	ps->externalEventTime = sv.time;

}

/**
 * Approx Quake3Units to Meters conversion.
 */
int SV_UnitsToMeters(float distance) {
	return (int)((distance / 8) * 0.3048);
}

/**
 * A better random number generation function.
 */
int SV_XORShiftRand(void) {
	int t = (mx ^ (mx << 11)) & RAND_MAX;
	mx = my; my = mz; mz = mw;
	mw = (mw ^ (mw >> 19) ^ (t ^ (t >> 8)));
	return mw;
}

/**
 * Generates a random number between the given min-max values.
 */
float SV_XORShiftRandRange(float min, float max) {
	return min + ((float)SV_XORShiftRand() / (float)RAND_MAX) * (max - min);
}

/**
 * Sets the initial seed for the XORShiftRand function.
 */
void SV_XORShiftRandSeed(unsigned int seed) {
	mw = seed;
}