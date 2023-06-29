/*
 * This file is part of the D3wasm project (http://www.continuation-labs.com/projects/d3wasm)
 * Copyright (c) 2019 Gabriel Cuvillier.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#version 100
//#pragma optimize(off)

precision mediump float;

attribute highp vec4 attr_Vertex;
attribute vec4 attr_TexCoord;

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;
uniform vec4 u_clipPlane;

varying vec2 var_TexDiffuse;
varying vec2 var_TexClip;

void main(void)
{
	var_TexDiffuse = (u_textureMatrix * attr_TexCoord).xy;

  var_TexClip = vec2( dot( u_clipPlane, attr_Vertex), 0.5 );

  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}