#version 320 es
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

#extension GL_EXT_clip_cull_distance : enable

precision mediump float;

uniform mat4 u_modelMatrix;
uniform vec4 u_lightOrigin;
uniform float u_maxLightDistance;
uniform mat4 u_textureMatrix;

in vec4 attr_Position;
in vec4 attr_TexCoord;
in int attr_DrawId;

out vec2 texCoord;
// out float gl_ClipDistance[5];

const mat3 cubicTransformations[6] = mat3[6] (
    mat3(
        0.0, 0.0, -1.0,
        0.0, -1.0, 0.0,
        -1.0, 0.0, 0.0
    ),
    mat3(
        0.0, 0.0, 1.0,
        0.0, -1.0, 0.0,
        1.0, 0.0, 0.0
    ),
    mat3(
        1.0, 0.0, 0.0,
        0.0, 0.0, -1.0,
        0.0, 1.0, 0.0
    ),
    mat3(
        1.0, 0.0, 0.0,
        0.0, 0.0, 1.0,
        0.0, -1.0, 0.0
    ),
    mat3(
        1.0, 0.0, 0.0,
        0.0, -1.0, 0.0,
        0.0, 0.0, -1.0
    ),
    mat3(
        -1.0, 0.0, 0.0,
        0.0, -1.0, 0.0,
        0.0, 0.0, 1.0
    )
);

const float clipEps = 0e-2;
const vec4 ClipPlanes[4] = vec4[4] (
    vec4(1.0, 0.0, -1.0, clipEps),
    vec4(-1.0, 0.0, -1.0, clipEps),
    vec4(0.0, 1.0, -1.0, clipEps),
    vec4(0.0, -1.0, -1.0, clipEps)
);

void main() {
    texCoord = (u_textureMatrix * attr_TexCoord).xy;
    vec4 lightSpacePos = u_modelMatrix * attr_Position - u_lightOrigin;
    vec4 fragPos = vec4(cubicTransformations[gl_InstanceID] * lightSpacePos.xyz, 1.0);
    gl_Position.x = fragPos.x / 6.0 + fragPos.z * 5.0/6.0 - fragPos.z / 3.0 * float(gl_InstanceID);
    gl_Position.y = fragPos.y;
    gl_Position.z = -fragPos.z - 2.0;
    gl_Position.w = -fragPos.z;
#ifdef GL_EXT_clip_cull_distance
    gl_ClipDistance[0] = dot(fragPos, ClipPlanes[0]);
    gl_ClipDistance[1] = dot(fragPos, ClipPlanes[1]);
    gl_ClipDistance[2] = dot(fragPos, ClipPlanes[2]);
    gl_ClipDistance[3] = dot(fragPos, ClipPlanes[3]);
    gl_ClipDistance[4] = u_maxLightDistance - (-fragPos.z);
#endif
}
