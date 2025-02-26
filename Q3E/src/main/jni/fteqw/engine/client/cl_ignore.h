/*

Copyright (C) 2001-2002       A Nourai

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __IGNORE_H_

#define __IGNORE_H_	


#define NO_IGNORE_NO_ADD	0
#define	NO_IGNORE_ADD		1
#define	IGNORE_NO_ADD		2

void Ignore_Init(void);
qboolean Ignore_Message(const char *sendername, const char *s, int flags);
char Ignore_Check_Flood(player_info_t *sender, const char *s, int flags);
void Ignore_Flood_Add(player_info_t *sender, const char *s);
void Ignore_ResetFloodList(void);


#define	PLAYER_ID_NOMATCH		-1
#define	PLAYER_NAME_NOMATCH		-2
#define	PLAYER_NUM_NOMATCH		-3
int Player_StringtoSlot(const char *arg);

#endif
