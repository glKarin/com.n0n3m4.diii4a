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

attribute highp vec4 attr_Vertex;      // input Vertex Coordinates

uniform highp mat4 u_modelViewProjectionMatrix;
uniform mat4 u_fogMatrix;        // fogPlanes 0, 1, 3 (CATION: not 2!), 2

varying vec2 var_TexFog;         // output Fog TexCoord
varying vec2 var_TexFogEnter;    // output FogEnter TexCoord

void main(void)
{
  gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

  // What will be computed:
  //
  // var_TexFog.x      = dot(u_fogMatrix[0], attr_Vertex);
  // var_TexFog.y      = dot(u_fogMatrix[1], attr_Vertex);
  // var_TexFogEnter.x = dot(u_fogMatrix[2], attr_Vertex);
  // var_TexFogEnter.y = dot(u_fogMatrix[3], attr_Vertex);

  // Optimized version:
  var_TexFog      = vec2(dot(u_fogMatrix[0], attr_Vertex),dot(u_fogMatrix[1], attr_Vertex));
  var_TexFogEnter = vec2(dot(u_fogMatrix[2], attr_Vertex),dot(u_fogMatrix[3], attr_Vertex));
}