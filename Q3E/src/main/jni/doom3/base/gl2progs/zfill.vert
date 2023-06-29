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
//#pragma optimize(off)

precision mediump float;

attribute vec4 attr_TexCoord;
attribute highp vec4 attr_Vertex;

uniform mat4 u_textureMatrix;
uniform highp mat4 u_modelViewProjectionMatrix;

varying vec2 var_TexDiffuse;

void main(void)
{
	// var_TexDiffuse = attr_TexCoord.xy; // origin
	var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;

	gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
