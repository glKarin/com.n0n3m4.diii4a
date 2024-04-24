/*
	mdfour.h

	an implementation of MD4 designed for use in the SMB authentication
	protocol

	Copyright (C) Andrew Tridgell 1997-1998

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

#ifndef _MDFOUR_H
#define _MDFOUR_H

#include <stdint.h>

struct mdfour_s {
	uint32_t A, B, C, D;
	uint32_t totalN;
};

void mdfour_begin(struct mdfour_s *md); // old: MD4Init
void mdfour_update(struct mdfour_s *md, const unsigned char *in, int n); //old: MD4Update
void mdfour_result(struct mdfour_s *md, unsigned char *out); // old: MD4Final
void mdfour(unsigned char *out, const unsigned char *in, int n);

#endif	// _MDFOUR_H

