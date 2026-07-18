

//FIXME: Use a different number of octaves based on the shader quality settings?


#if 0 //def GL_OES_texture_3D
float bandLimitedNoise( vec3 P ) {
	return texture3D( $noiseMap, P ).x;
}

float perlin(vec3 inpos) {
	vec4 octaveScales = vec4( 0.5, 0.25, 0.125, 0.0625 );
	vec4 noiseSamples;

	//FIXME: use inpos+inpos? is add still faster than a mul?
	noiseSamples.x = bandLimitedNoise( inpos );
	noiseSamples.y = bandLimitedNoise( inpos*2.0 );
	noiseSamples.z = bandLimitedNoise( inpos*4.0 );
	noiseSamples.w = bandLimitedNoise( inpos*8.0 );

	return dot( noiseSamples, octaveScales );
}
#else
float perlin(vec3 inpos) {
	vec4 noiseSamples = vec4( inpos, 1.0 );
	vec4 octaveScales = vec4( 0.5, 0.25, 0.125, 0.0625 );
	return dot( noiseSamples, octaveScales );
}
#endif
