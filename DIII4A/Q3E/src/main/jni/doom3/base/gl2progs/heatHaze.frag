/*
 * Copyright (C) 2012  Oliver McFadden <omcfadde@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#version 100
#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform sampler2D u_fragmentMap1;
uniform sampler2D u_fragmentMap2;
uniform sampler2D u_fragmentMap3;

void main(void)
{
	//gl_FragColor = texture2D(u_fragmentMap0, vec2(0.5, 0.5));
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
