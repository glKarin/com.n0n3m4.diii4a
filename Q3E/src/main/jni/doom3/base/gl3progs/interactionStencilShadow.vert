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
/*
	macros:
		BLINN_PHONG: using blinn-phong instead phong.
		_PBR: using PBR.
		_TRANSLUCENT: for translucent stencil shadow
		_SOFT: soft stencil shadow
*/
#version 300 es
//#pragma optimize(off)

precision highp float;

//#define BLINN_PHONG
//#define _PBR
//#define _TRANSLUCENT
//#define _SOFT

out vec2 var_TexDiffuse;
out vec2 var_TexNormal;
out vec2 var_TexSpecular;
out vec4 var_TexLight;
out lowp vec4 var_Color;
out vec3 var_L;
#if defined(BLINN_PHONG) || defined(_PBR)
out vec3 var_H;
#endif
#if !defined(BLINN_PHONG) || defined(_PBR)
out vec3 var_V;
#endif
#ifdef _PBR
out vec3 var_Normal;
#endif

in vec4 attr_TexCoord;
in vec3 attr_Tangent;
in vec3 attr_Bitangent;
in vec3 attr_Normal;
in highp vec4 attr_Vertex;
in lowp vec4 attr_Color;

uniform vec4 u_lightProjectionS;
uniform vec4 u_lightProjectionT;
uniform vec4 u_lightFalloff;
uniform vec4 u_lightProjectionQ;
uniform lowp vec4 u_colorModulate;
uniform lowp vec4 u_colorAdd;
uniform lowp vec4 u_glColor;

uniform vec4 u_lightOrigin;
uniform vec4 u_viewOrigin;

uniform vec4 u_bumpMatrixS;
uniform vec4 u_bumpMatrixT;
uniform vec4 u_diffuseMatrixS;
uniform vec4 u_diffuseMatrixT;
uniform vec4 u_specularMatrixS;
uniform vec4 u_specularMatrixT;

uniform highp mat4 u_modelViewProjectionMatrix;

void main(void)
{
    mat3 M = mat3(attr_Tangent, attr_Bitangent, attr_Normal);

    var_TexNormal.x = dot(u_bumpMatrixS, attr_TexCoord);
    var_TexNormal.y = dot(u_bumpMatrixT, attr_TexCoord);

    var_TexDiffuse.x = dot(u_diffuseMatrixS, attr_TexCoord);
    var_TexDiffuse.y = dot(u_diffuseMatrixT, attr_TexCoord);

    var_TexSpecular.x = dot(u_specularMatrixS, attr_TexCoord);
    var_TexSpecular.y = dot(u_specularMatrixT, attr_TexCoord);

    var_TexLight.x = dot(u_lightProjectionS, attr_Vertex);
    var_TexLight.y = dot(u_lightProjectionT, attr_Vertex);
    var_TexLight.z = dot(u_lightFalloff, attr_Vertex);
    var_TexLight.w = dot(u_lightProjectionQ, attr_Vertex);

    vec3 L = u_lightOrigin.xyz - attr_Vertex.xyz;
    vec3 V = u_viewOrigin.xyz - attr_Vertex.xyz;
#if defined(BLINN_PHONG) || defined(_PBR)
    vec3 H = normalize(L) + normalize(V);
#endif

    var_L = L * M;
#if defined(BLINN_PHONG) || defined(_PBR)
    var_H = H * M;
#endif
#if !defined(BLINN_PHONG) || defined(_PBR)
    var_V = V * M;
#endif
#ifdef _PBR
    var_Normal = attr_Normal * M;
#endif

    var_Color = (attr_Color / 255.0) * u_colorModulate + u_colorAdd;

    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
