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

/**
 * This is a 9-tap Gaussian blur accelerated by making use of the GPU filtering hardware
 * See: http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
 */

out vec4 FragColor;

uniform sampler2D u_source;

const float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
const float weight[3] = float[](0.227027027, 0.3162162162, 0.0702702703);

// (1, 0) or (0, 1)
uniform vec2 u_axis;

vec2 invImageSize = vec2(1) / vec2(textureSize(u_source, 0));


void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    FragColor = texture(u_source, fragCoord * invImageSize) * weight[0];
    
    for (int i = 1; i < 3; ++i) {
        FragColor += texture(u_source, (fragCoord + u_axis * offset[i]) * invImageSize) * weight[i];
        FragColor += texture(u_source, (fragCoord - u_axis * offset[i]) * invImageSize) * weight[i];
    }
}
