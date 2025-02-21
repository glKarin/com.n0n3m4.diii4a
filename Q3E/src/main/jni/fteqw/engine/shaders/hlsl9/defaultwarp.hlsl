!!cvarf r_wateralpha
!!samps diffuse

struct a2v {
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
};
struct v2f {
	#ifndef FRAGMENT_SHADER
	float4 pos: POSITION;
	#endif
	float2 tc: TEXCOORD0;
};
#ifdef VERTEX_SHADER
	float4x4  m_modelviewprojection;
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_modelviewprojection, inp.pos);
		outp.tc = inp.tc;
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	float cvar_r_wateralpha;
	float e_time;
	sampler s_diffuse;
	float4 main (v2f inp) : COLOR0
	{
		float2 ntc = inp.tc + sin(inp.tc.yx+e_time)*0.125;
		float3 ts = tex2D(s_diffuse, ntc).xyz;

#ifdef ALPHA
		return float4(ts, float(ALPHA));
#else
		return float4(ts, cvar_r_wateralpha);
#endif
	}
#endif
