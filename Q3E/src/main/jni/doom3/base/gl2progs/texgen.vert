/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;
attribute lowp vec4 attr_Color;

uniform lowp vec4 u_colorAdd;
uniform lowp vec4 u_colorModulate;
uniform vec4 u_texgenS;
uniform vec4 u_texgenT;
uniform vec4 u_texgenQ;
uniform mat4 u_textureMatrix;
uniform highp mat4 u_modelViewProjectionMatrix;

varying vec4 var_TexCoord;
varying lowp vec4 var_Color;

void main(void)
{
	gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

	vec4 texcoord0 = vec4(dot( u_texgenS, attr_Vertex ), dot( u_texgenT, attr_Vertex ), 0.0, dot( u_texgenQ, attr_Vertex )); 

	// multiply the texture matrix in
	var_TexCoord = vec4(dot( u_textureMatrix[0], texcoord0 ), dot( u_textureMatrix[1], texcoord0), texcoord0.z, texcoord0.w);

	// compute vertex modulation
	var_Color = (attr_Color / 255.0) * u_colorModulate + u_colorAdd;
}