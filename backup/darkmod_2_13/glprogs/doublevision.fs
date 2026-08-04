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
#version 140

in vec4 var_tc0;
out vec4 draw_Color;

uniform sampler2D u_texture0;
uniform vec4 u_scaleWindowToUnit;
uniform vec4 u_localParam0;

void main() {
	vec2 tc = gl_FragCoord.xy * u_scaleWindowToUnit.xy;
	float shift = u_localParam0.x;
	vec2 tcMin = tc * (1 - shift);
	vec2 tcMax = tcMin + vec2(shift);
	vec4 color00 = texture(u_texture0, tcMin);
	vec4 color11 = texture(u_texture0, tcMax);
	vec4 color10 = texture(u_texture0, vec2(tcMax.x, tcMin.y));
	vec4 color01 = texture(u_texture0, vec2(tcMin.x, tcMax.y));
	// note: original Doom 3 effect mixed only color00 and color11
	draw_Color = 0.25 * (color00 + color11 + color10 + color01);
}
