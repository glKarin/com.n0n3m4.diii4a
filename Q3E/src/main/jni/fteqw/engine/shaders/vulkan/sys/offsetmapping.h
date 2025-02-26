vec2 offsetmap(sampler2D normtex, vec2 base, vec3 eyevector)
{
#if !defined(OFFSETMAPPING_SCALE)
	#define OFFSETMAPPING_SCALE 1.0
#endif
	if (false)//(RELIEFMAPPING)
	{
		float i, f;
		vec3 OffsetVector = vec3(normalize(eyevector.xyz).xy * cvar_r_glsl_offsetmapping_scale * OFFSETMAPPING_SCALE * vec2(-1.0, 1.0), -1.0);
		vec3 RT = vec3(vec2(base.xy/* - OffsetVector.xy*OffsetMapping_Bias*/), 1.0);
		OffsetVector /= 10.0;
		for(i = 1.0; i < 10.0; ++i)
			RT += OffsetVector *  step(texture2D(normtex, RT.xy).a, RT.z);
		for(i = 0.0, f = 1.0; i < 5.0; ++i, f *= 0.5)
			RT += OffsetVector * (step(texture2D(normtex, RT.xy).a, RT.z) * f - 0.5 * f);
		return RT.xy;
	}
	else if (OFFSETMAPPING)
	{
		vec2 OffsetVector = normalize(eyevector).xy * cvar_r_glsl_offsetmapping_scale * OFFSETMAPPING_SCALE * vec2(-1.0, 1.0);
		vec2 tc = base;
		tc += OffsetVector;
		OffsetVector *= 0.333;
		tc -= OffsetVector * texture2D(normtex, tc).w;
		tc -= OffsetVector * texture2D(normtex, tc).w;
		tc -= OffsetVector * texture2D(normtex, tc).w;
		return tc;
	}
	return base;
}
