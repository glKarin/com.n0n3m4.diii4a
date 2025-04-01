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

// per-pixel cubic parallax-corrected reflection map calculation

in vec2 var_uvNormal;
in vec3 var_globalEye;
in mat3 var_tbn;
in vec3 var_worldPos;
in vec3 var_cubeMapCapturePos;
in vec3 var_proxyAABBMin;
in vec3 var_proxyAABBMax;
in float var_normalWeight;

out vec4 draw_Color;

// texture 0 is the environment cube map
// texture 1 is the normal map
uniform samplerCube u_texture0;
uniform sampler2D u_texture1;

vec3 parallaxCorrect(vec3 reflected) {
    vec3 nDir = normalize(reflected);
    vec3 firstPlaneIntersect = (var_proxyAABBMin - var_worldPos) / nDir;
    vec3 secondPlaneIntersect = (var_proxyAABBMax - var_worldPos) / nDir;
    
    vec3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    
    float dist = min(min(furthestPlane.x, furthestPlane.y), furthestPlane.z);
    vec3 intersectPos = var_worldPos + nDir * dist;
    
    return normalize(intersectPos - var_cubeMapCapturePos);
}

void main() {
	vec2 rimStrength = vec2(3.0, 0.4);
	
    vec3 globalNormal = var_tbn[2];
    if (var_normalWeight > 0) {
        // load the filtered normal map, then normalize to full scale
	    vec3 localNormal;
        localNormal.xy = 2 * texture(u_texture1, var_uvNormal).xy - 1;
        localNormal.z = sqrt(max(0, 1 - localNormal.x*localNormal.x - localNormal.y*localNormal.y));
        localNormal = normalize(localNormal);
        
	    // transform the surface normal by the local tangent space
        globalNormal = var_tbn * localNormal;
        // interpolate between vertex and mapped normal by weight factor
        globalNormal = normalize(mix(var_tbn[2], globalNormal, var_normalWeight));
    }
	
	// normalize vector to eye
	vec3 globalEye = normalize(var_globalEye);
	// calculate reflection vector
    float reflectCoeff = dot(globalEye, globalNormal);
    vec3 reflected = reflectCoeff * globalNormal * 2 - globalEye;
	// Calculate fresnel reflectance.
    float fresnel = pow(1 - reflectCoeff, 3) * rimStrength.x + rimStrength.y;

    // parallax-correct the reflection vector
    vec3 parallaxCorrected = parallaxCorrect(reflected);
	// read the environment map with the parallax-corrected reflection vector
	vec4 color = texture(u_texture0, parallaxCorrected);
	color *= fresnel;
    
    //---------------------------------------------------------
	// Tone Map to convert HDR values to range 0.0 - 1.0
	//---------------------------------------------------------
	draw_Color.xyz = color.xyz / (color.xyz + vec3(1.0));
}
