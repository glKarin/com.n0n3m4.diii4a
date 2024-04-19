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
#version 140
#pragma tdm_define "BLOOM_BRIGHTPASS"

/**
 * This is the downsampling portion of the "dual filtering" blur as suggested in the Siggraph 2015 talk
 * "Bandwidth-efficient Rendering" by Marius Bjorge.
 */

uniform sampler2D u_sourceTexture;

in vec2 var_TexCoord;
out vec4 FragColor;

#ifdef BLOOM_BRIGHTPASS
const vec3 toGrayscale = vec3(0.2126, 0.7152, 0.0722);
uniform float u_brightnessThreshold;
uniform float u_thresholdFalloff;

float brightpass(vec3 color) {
	float brightness = dot(color.rgb, toGrayscale);
	return clamp(pow(brightness / u_brightnessThreshold, u_thresholdFalloff), 0, 1);
}
#endif

vec4 sampleTexture(vec2 offset) {
	return texture(u_sourceTexture, var_TexCoord + offset);
}

void main() {
	// query previous mipmap level by a full-pixel offset (corresponds to half-pixel in our output framebuffer)
	vec2 offset = vec2(1, 1) / textureSize(u_sourceTexture, 0);
	vec4 sum = sampleTexture(vec2(0, 0)) * 4;
	sum += sampleTexture(-offset);
	sum += sampleTexture(offset);
	sum += sampleTexture(vec2(offset.x, -offset.y));
	sum += sampleTexture(vec2(-offset.x, offset.y));
	FragColor = sum / 8;
#ifdef BLOOM_BRIGHTPASS
	FragColor.rgb *= brightpass(FragColor.rgb);
#endif
}
