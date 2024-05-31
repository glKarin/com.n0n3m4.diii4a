/*
Simple helper macros to time blocks or statements.

Copyright (C) 2007 Frank Richter

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

#ifndef __TIMING_H__
#define __TIMING_H__

#if defined(DO_TIMING)

#define TIMING_BEGIN	double _timing_end_, _timing_start_ = Sys_DirtyTime();
#define TIMING_END_STR(S)		\
  _timing_end_ = Sys_DirtyTime();	\
  Con_Printf ("%s: %.3g s\n", S, _timing_end_ - _timing_start_);
#define TIMING_END	TIMING_END_STR(__FUNCTION__)

#define TIMING_INTERMEDIATE(S)						\
  {									\
    double currentTime = Sys_DirtyTime();				\
    Con_Printf ("%s: %.3g s\n", S, currentTime - _timing_start_);	\
  }
  
#define TIMING_TIMESTATEMENT(Stmt)	\
  {					\
    TIMING_BEGIN			\
    Stmt;				\
    TIMING_END_STR(#Stmt);		\
  }

#else

#define TIMING_BEGIN
#define TIMING_END_STR(S)
#define TIMING_END
#define TIMING_INTERMEDIATE(S)
#define TIMING_TIMESTATEMENT(Stmt)	Stmt

#endif

#endif // __TIMING_H__

