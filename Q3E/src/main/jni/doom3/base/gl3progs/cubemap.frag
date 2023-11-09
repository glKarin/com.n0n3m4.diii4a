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

#version 300 es
//#pragma optimize(off)

precision mediump float;

in vec3 var_TexCoord;
in lowp vec4 var_Color;

uniform samplerCube u_fragmentCubeMap0;
uniform lowp vec4 u_glColor;

out vec4 _gl_FragColor;

void main(void)
{
  _gl_FragColor = texture(u_fragmentCubeMap0, var_TexCoord) * u_glColor * var_Color;
}
