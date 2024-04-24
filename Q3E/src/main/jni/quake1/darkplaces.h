/*
Copyright (C) 2020 Ashley Rose Hale (LadyHavoc)
Copyright (C) 2020 David Knapp (Cloudwalk)

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

/* darkplaces.h - Master definitions file for Darkplaces engine */

#ifndef DARKPLACES_H
#define DARKPLACES_H

extern const char *buildstring;
extern char engineversion[128];

#ifdef __APPLE__
# include <TargetConditionals.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "sys.h"
#include "qtypes.h"
#include "qdefs.h"
#include "zone.h"
#include "thread.h"
#include "com_game.h"
#include "com_infostring.h"
#include "common.h"
#include "filematch.h"
#include "fs.h"
#include "host.h"
#include "cvar.h"
#include "cmd.h"
#include "console.h"
#include "lhnet.h"
#include "mathlib.h"
#include "matrixlib.h"

#endif
