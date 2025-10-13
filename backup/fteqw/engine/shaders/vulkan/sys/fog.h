#ifdef FRAGMENT_SHADER
	#define w_fogcolour		w_fogcolours.rgb
	#define w_fogalpha		w_fogcolours.a

	vec3 fog3(in vec3 regularcolour)
	{
		if (!FOG)
			return regularcolour;

		float z;
		float fac;

		if (cvar_r_fog_linear) {
			z = gl_FragCoord.z / gl_FragCoord.w;
			fac = (w_fogdensity - z) / (w_fogdensity - w_fogdepthbias);
		} else {
			z = w_fogdensity * gl_FragCoord.z / gl_FragCoord.w;
			z = max(0.0,z-w_fogdepthbias);
			if (cvar_r_fog_exp2)
				z *= z;
			fac = exp2(-(z * 1.442695));
		}

		fac = (1.0-w_fogalpha) + (clamp(fac, 0.0, 1.0)*w_fogalpha);
		return mix(w_fogcolour, regularcolour, fac);
	}
	vec3 fog3additive(in vec3 regularcolour)
	{
		if (!FOG)
			return regularcolour;

		float z;
		float fac;

		if (cvar_r_fog_linear) {
			z = gl_FragCoord.z / gl_FragCoord.w;
			fac = (w_fogdensity - z) / (w_fogdensity - w_fogdepthbias);
		} else {
			z = w_fogdensity * gl_FragCoord.z / gl_FragCoord.w;
			z = max(0.0,z-w_fogdepthbias);
			if (cvar_r_fog_exp2)
				z *= z;
			fac = exp2(-(z * 1.442695));
		}

		fac = (1.0-w_fogalpha) + (clamp(fac, 0.0, 1.0)*w_fogalpha);
		return regularcolour * fac;
	}
	vec4 fog4(in vec4 regularcolour)
	{
		if (!FOG)
			return regularcolour;
		return vec4(fog3(regularcolour.rgb), 1.0) * regularcolour.a;
	}
	vec4 fog4additive(in vec4 regularcolour)
	{
		if (!FOG)
			return regularcolour;

		float z;
		float fac;

		if (cvar_r_fog_linear) {
			z = gl_FragCoord.z / gl_FragCoord.w;
			fac = (w_fogdensity - z) / (w_fogdensity - w_fogdepthbias);
		} else {
			z = w_fogdensity * gl_FragCoord.z / gl_FragCoord.w;
			z = max(0.0,z-w_fogdepthbias);
			if (cvar_r_fog_exp2)
				z *= z;
			fac = exp2(-(z * 1.442695));
		}

		fac = (1.0-w_fogalpha) + (clamp(fac, 0.0, 1.0)*w_fogalpha);
		return regularcolour * vec4(fac, fac, fac, 1.0);
	}
	vec4 fog4blend(in vec4 regularcolour)
	{
		if (!FOG)
			return regularcolour;

		float z;
		float fac;

		if (cvar_r_fog_linear) {
			z = gl_FragCoord.z / gl_FragCoord.w;
			fac = (w_fogdensity - z) / (w_fogdensity - w_fogdepthbias);
		} else {
			z = w_fogdensity * gl_FragCoord.z / gl_FragCoord.w;
			z = max(0.0,z-w_fogdepthbias);
			if (cvar_r_fog_exp2)
				z *= z;
			fac = exp2(-(z * 1.442695));
		}

		fac = (1.0-w_fogalpha) + (clamp(fac, 0.0, 1.0)*w_fogalpha);
		return regularcolour * vec4(1.0, 1.0, 1.0, fac);
	}
#endif