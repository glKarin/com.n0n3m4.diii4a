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

#ifndef AUTOMATION_H
#define AUTOMATION_H


extern idCVar com_automation;
extern idCVar com_automation_port;

class Automation;
extern Automation *automation;

//perform all the regular processing (communicate with script, send commands, etc.)
void Auto_Think();

//check if automation locks user's gameplay input, and return that input if it does
bool Auto_GetUsercmd(usercmd_t &cmd);

#endif
