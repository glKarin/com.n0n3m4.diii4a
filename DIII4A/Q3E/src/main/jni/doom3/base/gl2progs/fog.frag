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

varying vec2 var_TexFog;            // input Fog TexCoord
varying vec2 var_TexFogEnter;       // input FogEnter TexCoord

uniform sampler2D u_fragmentMap0;   // Fog Image
uniform sampler2D u_fragmentMap1;   // Fog Enter Image
uniform lowp vec4 u_fogColor;       // Fog Color

void main(void)
{
  gl_FragColor = texture2D( u_fragmentMap0, var_TexFog ) * texture2D( u_fragmentMap1, var_TexFogEnter ) * vec4(u_fogColor.rgb, 1.0);
}