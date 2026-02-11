/*
** a_smartanim.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** Smart animation emulation.
**
*/

#include "actor.h"
#include "thingdef/thingdef.h"

//------------------------------------------------------------------------------
// Plasma Grenade state duration function. This calculates how many tics it
// will take to pass into the next tile. Sets the current frame to that duration
//

ACTION_FUNCTION(A_PlasmaGrenadeCalcDuration)
{
	const bool horiz = abs(self->velx) > abs(self->vely);
	const fixed velocity = horiz ? self->velx : self->vely;

	fixed distance = horiz ? self->fracx : self->fracy;
	if(velocity > 0)
		distance = FRACUNIT - distance;

	self->ticcount = distance/abs(velocity) + 1;

	return true;
}

//------------------------------------------------------------------------------
// Smart animation: Blake Stone used these to implement simple animation
// sequences. This would be fine, but they used the random number generator a
// little too much. So the main purpose of this is to hold a random number
// throughout an animation sequence.
//
// Due to technical limitations, this inconsistently takes 70hz tics instead of
// Doom style half tics.

ACTION_FUNCTION(A_InitSmartAnim)
{
	ACTION_PARAM_INT(delay, 0);
	self->temp1 = delay;
	return true;
}

ACTION_FUNCTION(A_SmartAnimDelay)
{
	self->ticcount = self->temp1;
	return true;
}
