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
#version 330

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

out float var_Intensity;
uniform vec2 u_extrusion;
uniform int u_hard;

uniform sampler2D u_diffuse;
uniform float u_alphaTest;
in vec2 var_TexCoord[];

vec2 xyPos(int i) {
    return gl_in[i].gl_Position.xy / gl_in[i].gl_Position.w;
}

void emit(int i, vec2 xy, float intensity) {
    vec4 incoming = gl_in[i].gl_Position;
    gl_Position = vec4(xy * incoming.w, incoming.z, incoming.w);
    var_Intensity = intensity;
    EmitVertex();
}

void emitEdge(int i, int j, int k) {
    vec2 a = xyPos(i);
    vec2 b = xyPos(j);
    vec2 c = xyPos(k);

    vec2 ab = normalize(b - a);
    vec2 cb = normalize(b - c);
    vec2 ac = normalize(c - a);
    vec2 nab = vec2(-ab.y, ab.x);
    vec2 bout = normalize(ab + cb);
    vec2 aout = -normalize(ab + ac);

    float strength = 1.0;
    if (u_alphaTest >= 0) {
        //if middle of edge fails alpha test
        //then don't render its outline
        vec4 tex = texture(u_diffuse, mix(var_TexCoord[i], var_TexCoord[j], 0.4137));
        if (tex.a <= u_alphaTest)
            strength = 0.0;
    }
    float outer = 0.0;
    if (u_hard != 0)
        outer = strength;
    
    vec2 ext = u_extrusion;
    emit(i, a + ext*aout, outer);
    emit(i, a + ext*nab, outer);
    emit(i, a, strength);
    emit(j, b + ext*nab, outer);
    emit(j, b, strength);
    emit(j, b + ext*bout, outer);
    EndPrimitive();
}

void main() {
    emitEdge(0, 1, 2);
    emitEdge(1, 2, 0);
    emitEdge(2, 0, 1);
}
