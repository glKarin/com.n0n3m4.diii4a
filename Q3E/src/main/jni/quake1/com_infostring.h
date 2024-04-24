/*
Copyright (C) 2006-2021 DarkPlaces contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef INFOSTRING_H
#define INFOSTRING_H

#include "qtypes.h"
#include <stdlib.h>

/// Returns the number of bytes written to *value excluding the \0 terminator.
size_t InfoString_GetValue(const char *buffer, const char *key, char *value, size_t valuesize);
void InfoString_SetValue(char *buffer, size_t bufferlength, const char *key, const char *value);
void InfoString_Print(char *buffer);

#endif
