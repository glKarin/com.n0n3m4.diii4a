!!samps 2

	struct a2v
	{
		float4 pos: POSITION;
	};
	struct v2f
	{
#ifndef FRAGMENT_SHADER
		float4 pos: POSITION;
#endif
		float3 vpos: TEXCOORD0;
	};

#ifdef VERTEX_SHADER
	float4x4  m_modelviewprojection;
	v2f main (in a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_modelviewprojection, inp.pos);
		outp.vpos = inp.pos.xyz;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	float e_time;
	float3 e_eyepos;

	sampler s_diffuse; /*diffuse*/
	sampler s_fullbright; /*normal*/
	float4 main (v2f inp) : COLOR0
	{
		float2 tccoord;

		float3 dir = inp.vpos - e_eyepos;

		dir.z *= 3.0;
		dir.xy /= 0.5*length(dir);

		tccoord = (dir.xy + e_time*0.03125);
		float4 solid = tex2D(s_diffuse, tccoord);

		tccoord = (dir.xy + e_time*0.0625);
		float4 clouds = tex2D(s_fullbright, tccoord);
		return float4((solid.rgb*(1.0-clouds.a)) + (clouds.a*clouds.rgb), 1);
	}
#endif
