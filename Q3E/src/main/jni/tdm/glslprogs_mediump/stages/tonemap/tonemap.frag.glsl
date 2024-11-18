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

precision mediump float;

in vec2 var_TexCoord;
out vec4 draw_Color;
uniform sampler2D u_texture;

uniform float u_gamma, u_brightness;
uniform float u_desaturation;
uniform float u_colorCurveBias;
uniform float u_colorCorrection, u_colorCorrectBias;

uniform int u_sharpen;
uniform float u_sharpness;

uniform float u_ditherInput;
uniform float u_ditherOutput;
uniform sampler2D u_noiseImage;

/**
 * Contrast-adaptive sharpening from AMD's FidelityFX.
 * Adapted from Marty McFly's port for Reshade:
 * https://gist.github.com/martymcmodding/30304c4bffa6e2bd2eb59ff8bb09d135
 *
 * Note this is only the most basic form of CAS. The AMD original
 * can do more, including up- and downscaling. As that's harder to implement,
 * we're keeping it simple here.
 *
 * FidelityFX CAS is licensed under the terms of the MIT license:
 *
 * Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
vec3 sharpen(vec2 texcoord) {
	// fetch a 3x3 neighborhood around the pixel 'e',
	//  a b c
	//  d(e)f
	//  g h i
	vec3 a = textureOffset(u_texture, texcoord, ivec2(-1, -1)).rgb;
	vec3 b = textureOffset(u_texture, texcoord, ivec2(0, -1)).rgb;
	vec3 c = textureOffset(u_texture, texcoord, ivec2(1, -1)).rgb;
	vec3 d = textureOffset(u_texture, texcoord, ivec2(-1, 0)).rgb;
	vec3 e = textureOffset(u_texture, texcoord, ivec2(0, 0)).rgb;
	vec3 f = textureOffset(u_texture, texcoord, ivec2(1, 0)).rgb;
	vec3 g = textureOffset(u_texture, texcoord, ivec2(-1, 1)).rgb;
	vec3 h = textureOffset(u_texture, texcoord, ivec2(0, 1)).rgb;
	vec3 i = textureOffset(u_texture, texcoord, ivec2(1, 1)).rgb;

	// Soft min and max.
	//  a b c             b
	//  d e f * 0.5  +  d e f * 0.5
	//  g h i             h
	// These are 2.0x bigger (factored out the extra multiply).
	vec3 mnRGB = min(min(min(d, e), min(f, b)), h);
	vec3 mnRGB2 = min(mnRGB, min(min(a, c), min(g, i)));
	mnRGB += mnRGB2;

	vec3 mxRGB = max(max(max(d, e), max(f, b)), h);
	vec3 mxRGB2 = max(mxRGB, max(max(a, c), max(g, i)));
	mxRGB += mxRGB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	vec3 rcpMRGB = vec3(1) / mxRGB;
	vec3 ampRGB = clamp(min(mnRGB, 2.0 - mxRGB) * rcpMRGB, vec3(0.0), vec3(1.0));

	// Shaping amount of sharpening.
	ampRGB = inversesqrt(ampRGB);

	float peak = 8.0 - 3.0 * u_sharpness;
	vec3 wRGB = -vec3(1) / (ampRGB * peak);

	vec3 rcpWeightRGB = vec3(1) / (1.0 + 4.0 * wRGB);

	//                          0 w 0
	//  Filter shape:           w 1 w
	//                          0 w 0
	vec3 window = (b + d) + (f + h);
	vec3 outColor = clamp((window * wRGB + e) * rcpWeightRGB, vec3(0.0), vec3(1.0));

	return outColor;
}

float mapColorComponent(float value) {
	float color = max(value, 0.0);

	//stgatilov: apply traditional gamma/brightness settings
	color = pow(color, 1.0/u_gamma);
	color *= u_brightness;

	if (u_colorCurveBias != 0.0) {
		//---------------------------------------------------------
		//  Apply Smooth Exponential color falloff
		//---------------------------------------------------------
		float reduced1 = 1.0 - pow(2.718282, -3.0 * color * color);
		color = mix(color, reduced1, u_colorCurveBias);
	}
	if (u_colorCorrectBias != 0.0) {
		//---------------------------------------------------------
		//  Apply Smooth Exponential color correction with a bias
		//---------------------------------------------------------
		float reduced2 = 1.0 - pow(2.718282, -u_colorCorrection * color);
		color = mix(color, reduced2, u_colorCorrectBias);
	}
	return color;
}

vec3 ditherColor(vec3 value, float strength) {
	vec2 tc = gl_FragCoord.xy / vec2(textureSize(u_noiseImage, 0));
	vec3 noiseColor = textureLod(u_noiseImage, tc, 0.0).rgb;
	value += (noiseColor - vec3(0.5)) * strength;
	return value;
}

void main() {
	vec3 color;
	if (u_sharpen != 0) {
		color = sharpen(var_TexCoord);
	} else {
		color = texture(u_texture, var_TexCoord).rgb;
	}

	if (u_ditherInput > 0.0)
		color = ditherColor(color, -u_ditherInput);

	color.r = mapColorComponent(color.r);
	color.g = mapColorComponent(color.g);
	color.b = mapColorComponent(color.b);

	if (u_desaturation != 0.0) {
		float luma = clamp(dot(vec3(0.2125, 0.7154, 0.0721), color.rgb), 0.0, 1.0);
		color = mix(color, vec3(luma), u_desaturation);
	}

	if (u_ditherOutput > 0.0)
		color = ditherColor(color, u_ditherOutput);

	draw_Color = vec4(color, 1);
}
