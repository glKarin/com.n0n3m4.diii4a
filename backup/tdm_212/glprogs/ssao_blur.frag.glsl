#version 140

/**
 Based on the SAO algorithm by Morgan McGuire and Michael Mara, NVIDIA Research

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

out vec3 occlusionAndDepth;

uniform sampler2D u_source;

//////////////////////////////////////////////////////////////////////////////////////////////
// Tunable Parameters:

/** Step in 2-pixel intervals since we already blurred against neighbors in the
    first AO pass.  This constant can be increased while R decreases to improve
    performance at the expense of some dithering artifacts.

    Morgan found that a scale of 3 left a 1-pixel checkerboard grid that was
    unobjectionable after shading was applied but eliminated most temporal incoherence
    from using small numbers of sample taps.
    */
#define SCALE               (2)

/** Filter radius in pixels. This will be multiplied by SCALE. */
#define R                   (4)

const float gaussian[R + 1] = float[](0.153170, 0.144893, 0.122649, 0.092902, 0.062970);  // stddev = 2.0

// (1, 0) or (0, 1)
uniform vec2 u_axis;
ivec2 axis = ivec2(u_axis);

float unpackZ(vec2 packedZ) {
	return packedZ.x * (256.0 / 257.0) + packedZ.y * (1.0 / 257.0);
}

uniform float u_edgeSharpness;

void main() {
	ivec2 ssC = ivec2(gl_FragCoord.xy);

	vec4 temp = texelFetch(u_source, ssC, 0);

	occlusionAndDepth.gb = temp.gb;
	float z = unpackZ(occlusionAndDepth.gb);

	float sum = temp.r;

	// Base weight for depth falloff.  Increase this for more blurriness,
	// decrease it for better edge discrimination
	float BASE = gaussian[0];
	float totalWeight = BASE;
	sum *= totalWeight;

	for (int r = -R; r <= R; ++r) {
		// We already handled the zero case above.  This loop should be unrolled and the static branch optimized out,
		// so the IF statement has no runtime cost
		if (r != 0) {
			temp = texelFetch(u_source, ssC + axis * (r * SCALE), 0);
			float value  = temp.r;

			// spatial domain: offset gaussian tap
			float weight = 0.3 + gaussian[abs(r)];

			float tapZ = unpackZ(temp.gb);
			// range domain (the "bilateral" weight). As depth difference increases, decrease weight.
		  	weight *= max(0.0, 1.0 - 2000 * u_edgeSharpness * abs(tapZ - z));

			sum += value * weight;
			totalWeight += weight;
		}
	}

	const float epsilon = 0.0001;
	occlusionAndDepth.r = sum / (totalWeight + epsilon);
}
